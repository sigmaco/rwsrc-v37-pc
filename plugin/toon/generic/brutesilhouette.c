/*
 *  Brute force CPU based silhouette edge detection
 */
#include "rwcore.h"
#include "rpworld.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "toon.h"
#include "genericedgerender.h"

#include "brutesilhouette.h"

static RwV3d    *eMinusV = 0;       /* scratch area for dot product tests */
static RwUInt8  *eMinusVdotN = 0;

void
_rpToonBruteSilhouetteInit(void)
{
    RWFUNCTION(RWSTRING("_rpToonBruteSilhouetteInit"));

    eMinusV = (RwV3d *)RwMalloc(RPTOON_MAX_VERTICES * sizeof(RwV3d),
        rwID_TOONPLUGIN | rwMEMHINTDUR_GLOBAL);
    RWASSERT((eMinusV != 0) && "RwMalloc failed");

    eMinusVdotN = (RwUInt8 *)RwMalloc(RPTOON_MAX_FACES * sizeof(RwUInt8),
        rwID_TOONPLUGIN | rwMEMHINTDUR_GLOBAL);
    RWASSERT((eMinusVdotN != 0) && "RwMalloc failed");

    RWRETURNVOID();
}

void
_rpToonBruteSilhouetteClean()
{
    RWFUNCTION(RWSTRING("_rpToonBruteSilhouetteClean"));

    RwFree(eMinusVdotN);
    RwFree(eMinusV);

    RWRETURNVOID();
}

static void
_rpToonUpdateSilhouetteEdges(const RpToonGeo *toonGeo,
                             const RwV3d *verts,
                             const RwMatrix *transform)
{
    RwInt32     i;
    RwCamera    *camera;
    RwFrame     *cameraFrame;
    RwMatrix    invLTM;
    RwV3d       eyePosInObjectSpace;

    RWFUNCTION(RWSTRING("_rpToonUpdateSilhouetteEdges"));
    RWASSERT(0 != toonGeo);
    RWASSERT(0 != verts);
    RWASSERT(0 != transform);

    RWASSERT(toonGeo->numVerts <= RPTOON_MAX_VERTICES);
    RWASSERT(toonGeo->numTriangles <= RPTOON_MAX_VERTICES);

    /* get eye in object space minus vertex position for each vertex */
    RwMatrixInvert(&invLTM, transform);

    camera = RwCameraGetCurrentCamera();
    RWASSERT(0 != camera);

    cameraFrame = RwCameraGetFrame(camera);
    RWASSERT(0 != cameraFrame);

    /* Transform camera position in to object space */
    RwV3dTransformPoint(
        &eyePosInObjectSpace,
        RwMatrixGetPos(RwFrameGetLTM(cameraFrame)),
        &invLTM);

    /* Calc eye - vertex vector */
    for (i = 0; i < toonGeo->numVerts; i++)
    {
        RwV3dSub(&eMinusV[i], &eyePosInObjectSpace, &verts[i]);
    }

    /* Calc normal dot (eye - vertex) for each face */
    RWASSERT(toonGeo->faceNormals != 0);
    for (i = 0; i < toonGeo->numTriangles; i++)
    {
        RwReal  dot;

        dot =
            RwV3dDotProduct(
                &eMinusV[toonGeo->triangles[i].vertIndex[0]],
                &toonGeo->faceNormals[i]);
        eMinusVdotN[i] = (RwUInt8)(dot > 0.0f);
    }

    RWRETURNVOID();
}

static void
ComputeSilhouetteRibVertexPositions(RwV3d *camspaceInnerVertex,
                                    RwV3d *camspaceOuterVertex,
                                    rpToonVertexIndex innerVertexIndex,
                                    const RwV3d *objectSpaceInnerVertexPosition,
                                    const RwV3d *objectSpaceInnerVertexNormal,
                                    const RpToonGeo *toonGeo,
                                    const ScreenSpaceProjection *ssp,
                                    const RpToonInk *ink )
{
    RwV2d   screenSpaceInnerVertex;
    RwV2d   screenSpaceOuterVertex;
    RwV2d   screenSpaceExtrudeDir;
    RwReal  screenSpaceExtrudeDirLength;
    RwV3d   objectSpaceOuterVertex;
    RwReal  scale;

    RWFUNCTION(RWSTRING("ComputeSilhouetteRibVertexPositions"));

    /* generate screenspace & camera space positions for inner vertex from object space position */
    ScreenSpaceProjectionProjectPoint(&screenSpaceInnerVertex,
                                      camspaceInnerVertex,
                                      ssp,
                                      objectSpaceInnerVertexPosition);

    /* generate object space position of outer vertex from normal & inner vertex position
       n.b. this is constant if the mesh doesn't change */
    RwV3dAdd(&objectSpaceOuterVertex, objectSpaceInnerVertexPosition, objectSpaceInnerVertexNormal );

    /* generate screenspace & camera space positions for outer vertex from object space position */
    ScreenSpaceProjectionProjectPoint(&screenSpaceOuterVertex,
                                      camspaceOuterVertex,
                                      ssp,
                                      &objectSpaceOuterVertex);

    /* start with the object level user scale factor & accumulate more scale factors for per vertex effect */
    scale = ink->farScale_factor;

    /* apply normalizing factor to 2D extrude direction - the effects of perspective are not wanted */
    RwV2dSub(&screenSpaceExtrudeDir, &screenSpaceOuterVertex, &screenSpaceInnerVertex);

    screenSpaceExtrudeDirLength = RwV2dLength(&screenSpaceExtrudeDir);

    if (screenSpaceExtrudeDirLength > RPTOON_REALLY_SMALL_EDGE_THRESHOLD)
    {
        scale /= screenSpaceExtrudeDirLength;
    }

    /* multiply in vertex level user scale factors to generate the desired 2d length */
    scale *= toonGeo->vertexThicknesses[innerVertexIndex]
        * _rpToonComputePerspectiveScale(ink, camspaceOuterVertex->z );

    /* get desired 2d length & direction of rib */
    RwV2dScale( &screenSpaceExtrudeDir, &screenSpaceExtrudeDir, scale );

    /* now desired 2d position of rib's outer vertex */
    camspaceOuterVertex->x = screenSpaceInnerVertex.x + screenSpaceExtrudeDir.x;
    camspaceOuterVertex->y = screenSpaceInnerVertex.y + screenSpaceExtrudeDir.y;

    /*
      Give the outer vertex the z value of the inner vertex.
      Conceptually, they're in a plane parallel to the screen.
    */
    camspaceOuterVertex->z = camspaceInnerVertex->z;

    /*
      And back project it to 3D camera space so we get hardware clipping on this rib quad,
      yet it still reprojects to the desired 2D position.
    */
    BackProject( camspaceOuterVertex );

    RWRETURNVOID();
}

static void
ToonComputeFaceNormals(RwInt32 numTris,
                       const rpToonTriangle * tris,
                       const RwV3d *vertices,
                       RwV3d *faceNormals)
{
    RwInt32 i;
    RwV3d   e0;
    RwV3d   e1;

    RWFUNCTION(RWSTRING("ToonComputeFaceNormals"));

    RWASSERT( tris != 0 );
    RWASSERT( vertices != 0 );
    RWASSERT( faceNormals != 0 );

    for (i = 0; i<numTris; i++)
    {
        RwV3dSub(&e0, &vertices[ tris[i].vertIndex[1] ], &vertices[ tris[i].vertIndex[0] ]);
        RwV3dSub(&e1, &vertices[ tris[i].vertIndex[2] ], &vertices[ tris[i].vertIndex[0] ]);
        RwV3dCrossProduct(&faceNormals[i], &e0, &e1);
    }

    RWRETURNVOID();
}

/* brute force silhouette edge renderer functions - needs the generic line render code */
void
_rpToonBruteSilhouetteRender(RpToonGeo *toonGeo,
                             const RwV3d *verts,
                             const RwV3d *normals,
                             RwBool faceNormalsInvalid,
                             const RwMatrix *transform)
{
    RwInt32                 numSilhouetteEdges = 0;
    RwInt32                 i;
    RwIm3DVertex            *verticesEnd = _rpToonGenericVertexBuffer;
    ScreenSpaceProjection   ssp;
    RwCullMode              oldCullMode;

    RWFUNCTION(RWSTRING("_rpToonBruteSilhouetteRender"));

    /*
      Turn off backface culling for silhouettes because it's a bit tricky to keep the order consistent
    */
    RwRenderStateGet(rwRENDERSTATECULLMODE, (void*)&oldCullMode);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);

    RWASSERT(toonGeo != 0);

    if (faceNormalsInvalid)
    {
        ToonComputeFaceNormals(
            toonGeo->numTriangles,
            toonGeo->triangles,
            verts,
            toonGeo->faceNormals);
    }

    /* Update silhouette edges */
    _rpToonUpdateSilhouetteEdges( toonGeo, verts, transform );

    _rpToonScreenSpaceProjectionInit( &ssp, transform );

    /* Determine edge visibility & do extrusion */
    for (i = 0; i<toonGeo->numEdges; i++)
    {
        /* compare visibility of edge's two polys (if it's got two that is!) */
        if (    (toonGeo->edges[i].face[1] != RPTOONEDGE_NONEIGHBOURFACE)
                &&  (eMinusVdotN[toonGeo->edges[i].face[0]] != eMinusVdotN[toonGeo->edges[i].face[1]]))
        {
            RpToonInk *ink = toonGeo->inks[toonGeo->edgeInkIDs[i].inkId[RPTOON_INKTYPE_SILHOUETTE]];

            RwV3d camspaceInnerVertex0, camspaceInnerVertex1,
                camspaceOuterVertex0, camspaceOuterVertex1;

            rpToonVertexIndex innerVertexIndex0 = toonGeo->edges[i].v[0];
            rpToonVertexIndex innerVertexIndex1 = toonGeo->edges[i].v[1];

            ComputeSilhouetteRibVertexPositions(&camspaceInnerVertex0,
                                                &camspaceOuterVertex0,
                                                innerVertexIndex0,
                                                &verts[innerVertexIndex0],
                                                &normals[innerVertexIndex0],
                                                toonGeo,
                                                &ssp,
                                                ink );

            ComputeSilhouetteRibVertexPositions(&camspaceInnerVertex1,
                                                &camspaceOuterVertex1,
                                                innerVertexIndex1,
                                                &verts[innerVertexIndex1],
                                                &normals[innerVertexIndex1],
                                                toonGeo,
                                                &ssp,
                                                ink );

            verticesEnd =
                _rpToonGenericEdgeRenderAppendIm3dQuad(
                    verticesEnd,
                    &camspaceInnerVertex0, &camspaceInnerVertex1,
                    &camspaceOuterVertex0, &camspaceOuterVertex1,
                    ink->color);

            numSilhouetteEdges++;
        }
    }

    _rpToonGenericEdgeRenderSubmit(numSilhouetteEdges * 6, rwPRIMTYPETRILIST);

    RwRenderStateSet(rwRENDERSTATECULLMODE, (void *)oldCullMode);

    RWRETURNVOID();
}
