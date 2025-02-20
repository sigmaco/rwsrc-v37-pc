/*
 *  Brute force CPU based silhouette edge detection
 */
#include "d3d8.h"

#include "rwcore.h"
#include "rpworld.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "toon.h"
#include "edgerender.h"

#include "brutesilhouette.h"

#define FLOATASINT(f) (*((const RwInt32 *)&(f)))


_rpToonVertexCache *_rpToonD3D8VertexCache= NULL;

RwUInt8  *_rpToonD3D8IsFrontFace = NULL;       /* scratch area for dot product tests */

static RwReal OneInkCoef1 = 0.0f;
static RwReal OneInkCoef2 = 0.0f;

static RwBool UseVertexShader = FALSE;

#if !defined( NOSSEASM )
static const __declspec(align(16)) RwReal Zero = 0.f;

static RwBool UseSSE = FALSE;
#endif

/****************************************************************************
 _rpToonBruteSilhouetteInit
 */
void
_rpToonBruteSilhouetteInit(void)
{
    const D3DCAPS8      *d3dCaps;

    RWFUNCTION(RWSTRING("_rpToonBruteSilhouetteInit"));

    _rpToonD3D8VertexCache = (_rpToonVertexCache *)RwMalloc(RPTOON_MAX_VERTICES * sizeof(_rpToonVertexCache),
                                                            rwID_TOONPLUGIN | rwMEMHINTDUR_GLOBAL);
    RWASSERT((_rpToonD3D8VertexCache != 0) && "RwMalloc failed");

    _rpToonD3D8IsFrontFace = (RwUInt8 *)RwMalloc(RPTOON_MAX_FACES * sizeof(RwUInt8),
        rwID_TOONPLUGIN | rwMEMHINTDUR_GLOBAL);
    RWASSERT((_rpToonD3D8IsFrontFace != 0) && "RwMalloc failed");

    d3dCaps = (const D3DCAPS8 *)RwD3D8GetCaps();

    if ((d3dCaps->VertexShaderVersion & 0xFFFF) >= 0x0101 &&
        (d3dCaps->PixelShaderVersion & 0xFFFF) >= 0x0101)
    {
        UseVertexShader = TRUE;
    }

    #if !defined( NOSSEASM )
    UseSSE = _rwIntelSSEsupported();
    #endif

    RWRETURNVOID();
}

void
_rpToonBruteSilhouetteClean()
{
    RWFUNCTION(RWSTRING("_rpToonBruteSilhouetteClean"));

    RwFree(_rpToonD3D8IsFrontFace);
    RwFree(_rpToonD3D8VertexCache);

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
    RwV3d       eyePosInObjectSpace;

    RWFUNCTION(RWSTRING("_rpToonUpdateSilhouetteEdges"));
    RWASSERT(0 != toonGeo);
    RWASSERT(0 != verts);

    RWASSERT(toonGeo->numVerts <= RPTOON_MAX_VERTICES);
    RWASSERT(toonGeo->numTriangles <= RPTOON_MAX_VERTICES);

    /* Early out */
    if (toonGeo->numVerts == 0 || toonGeo->numTriangles == 0)
    {
        RWRETURNVOID();
    }

    camera = RwCameraGetCurrentCamera();
    RWASSERT(0 != camera);

    cameraFrame = RwCameraGetFrame(camera);
    RWASSERT(0 != cameraFrame);

    /* get eye in object space minus vertex position for each vertex */
    if (transform != NULL)
    {
        RwMatrix    invLTM;

        RwMatrixInvert(&invLTM, transform);

        /* Transform camera position in to object space */
        RwV3dTransformPoint(&eyePosInObjectSpace,
                            RwMatrixGetPos(RwFrameGetLTM(cameraFrame)),
                            &invLTM);
    }
    else
    {
        eyePosInObjectSpace = *RwMatrixGetPos(RwFrameGetLTM(cameraFrame));
    }

    /* Calc eye - vertex vector */
    for (i = 0; i < toonGeo->numVerts; i++)
    {
        RwV3dSub(&_rpToonD3D8VertexCache[i].camspaceInnerVertex,
                 &eyePosInObjectSpace,
                 &verts[i]);

        _rpToonD3D8VertexCache[i].ink = NULL;
    }

    /* Calc normal dot (eye - vertex) for each face */
    RWASSERT(toonGeo->faceNormals != 0);

    #if !defined( NOSSEASM )
    if (UseSSE)
    {
        const RwV3d *normals;

        normals = toonGeo->faceNormals;

        for (i = 0; i < toonGeo->numTriangles; i++)
        {
            const RwV3d *position;

            position = &(_rpToonD3D8VertexCache[toonGeo->triangles[i].vertIndex[0]].camspaceInnerVertex);

            _asm
            {
                mov     edi, position
                mov     esi, normals

                movss   xmm0, [edi + 0]
                movss   xmm1, [edi + 4]
                movss   xmm2, [edi + 8]

                movss   xmm3, [esi + 0]
                movss   xmm4, [esi + 4]
                movss   xmm5, [esi + 8]

                mulss   xmm0, xmm3
                mulss   xmm1, xmm4
                mulss   xmm2, xmm5

                addss   xmm0, xmm1

                mov     edi, _rpToonD3D8IsFrontFace

                addss   xmm0, xmm2

                add     edi, i

                ucomiss xmm0, Zero

                setnbe   al

                add     esi, 12

                mov     [edi], al
                mov     normals, esi
            }
        }
    }
    else
    #endif
    {
        for (i = 0; i < toonGeo->numTriangles; i++)
        {
            RwReal  dot;

            dot =
                RwV3dDotProduct(
                    &_rpToonD3D8VertexCache[toonGeo->triangles[i].vertIndex[0]].camspaceInnerVertex,
                    &toonGeo->faceNormals[i]);
            _rpToonD3D8IsFrontFace[i] = (RwUInt8)(FLOATASINT(dot) > 0);  /* Get the sign of the dot product */
        }
    }

    RWRETURNVOID();
}

/****************************************************************************
 Fast2DTransformVector
 */
static void
Fast2DTransformVector(RwV2d *pointsOut,
                      const RwV3d *pointsIn,
                      const RwMatrix * matrix)
{
    RWFUNCTION(RWSTRING("Fast2DTransformVector"));
    RWASSERT(pointsOut);
    RWASSERT(pointsIn);
    RWASSERT(matrix);

    pointsOut->x = ((pointsIn->x) * (matrix->right.x)) +
                   ((pointsIn->y) * (matrix->up.x)) +
                   ((pointsIn->z) * (matrix->at.x));

    pointsOut->y = ((pointsIn->x) * (matrix->right.y)) +
                   ((pointsIn->y) * (matrix->up.y)) +
                   ((pointsIn->z) * (matrix->at.y));

    RWRETURNVOID();
}

static void
ComputeSilhouetteRibVertexPositions(RwV3d *camspaceInnerVertex,
                                    RwV2d *camspaceOuterVertex,
                                    RwReal vertexThicknesses,
                                    const RwV3d *objectSpaceInnerVertexPosition,
                                    const RwV3d *objectSpaceInnerVertexNormal,
                                    const ScreenSpaceProjection *ssp,
                                    const RpToonInk *ink )
{
    RwV2d   cameraSpaceNormal;

    RwReal  screenSpaceNormalLengthSquare;
    RwReal  screenSpaceNormalLengthInv;

    RwReal  scale;

    RWFUNCTION(RWSTRING("ComputeSilhouetteRibVertexPositions"));

    /* generate screenspace & camera space positions for inner vertex from object space position */
    RwV3dTransformPoint(camspaceInnerVertex, objectSpaceInnerVertexPosition, &ssp->matrix);

    /* generate screenspace & camera space positions for outer vertex from object space position */
    Fast2DTransformVector(&cameraSpaceNormal, objectSpaceInnerVertexNormal, &ssp->matrix);

    /* start with the object level user scale factor & accumulate more scale factors for per vertex effect */
    scale = ink->farScale_factor;

    /* multiply in vertex level user scale factors to generate the desired 2d length */
    if (ink->perspectiveScale_b != 0.0f)
    {
        scale *= vertexThicknesses * (ink->perspectiveScale_a + ink->perspectiveScale_b * (camspaceInnerVertex->z - ink->perspectiveScale_zMin));
    }
    else
    {
        scale *= vertexThicknesses * ink->perspectiveScale_a;
    }

    /* Decrease with distance */
    scale *= camspaceInnerVertex->z;

    screenSpaceNormalLengthSquare = RwV2dDotProduct(&cameraSpaceNormal, &cameraSpaceNormal);

    if (screenSpaceNormalLengthSquare > (RPTOON_REALLY_SMALL_EDGE_THRESHOLD * RPTOON_REALLY_SMALL_EDGE_THRESHOLD))
    {
        #if !defined( NOSSEASM )
        if (UseSSE)
        {
            _asm
            {
                movss xmm0, screenSpaceNormalLengthSquare
                rsqrtss xmm0, xmm0
                movss xmm1, scale
                mulss xmm1, xmm0
                movss scale, xmm1
            }
        }
        else
        #endif
        {
            rwInvSqrtMacro(&screenSpaceNormalLengthInv, screenSpaceNormalLengthSquare);

            scale *= screenSpaceNormalLengthInv;
        }
    }

    /* now desired 2d position of rib's outer vertex and back project*/
    camspaceOuterVertex->x = camspaceInnerVertex->x + (cameraSpaceNormal.x * scale);
    camspaceOuterVertex->y = camspaceInnerVertex->y + (cameraSpaceNormal.y * scale);

    RWRETURNVOID();
}

static void
ReComputeSilhouetteRibVertexPositions(RwV2d *camspaceOuterVertex,
                                      RwReal vertexThicknesses,
                                      const RwV3d *camspaceInnerVertex,
                                      const RwV3d *objectSpaceInnerVertexNormal,
                                      const ScreenSpaceProjection *ssp,
                                      const RpToonInk *ink )
{
    RwV2d   cameraSpaceNormal;

    RwReal  screenSpaceNormalLengthSquare;
    RwReal  screenSpaceNormalLengthInv;

    RwReal  scale;

    RWFUNCTION(RWSTRING("ReComputeSilhouetteRibVertexPositions"));

    /* generate screenspace & camera space positions for outer vertex from object space position */
    Fast2DTransformVector(&cameraSpaceNormal, objectSpaceInnerVertexNormal, &ssp->matrix);

    /* start with the object level user scale factor & accumulate more scale factors for per vertex effect */
    scale = ink->farScale_factor;

    screenSpaceNormalLengthSquare = RwV2dDotProduct(&cameraSpaceNormal, &cameraSpaceNormal);

    if (screenSpaceNormalLengthSquare > (RPTOON_REALLY_SMALL_EDGE_THRESHOLD * RPTOON_REALLY_SMALL_EDGE_THRESHOLD))
    {
        #if !defined( NOSSEASM )
        if (UseSSE)
        {
            _asm
            {
                movss xmm0, screenSpaceNormalLengthSquare
                rsqrtss xmm0, xmm0
                movss xmm1, scale
                mulss xmm1, xmm0
                movss scale, xmm1
            }
        }
        else
        #endif
        {
            rwInvSqrtMacro(&screenSpaceNormalLengthInv, screenSpaceNormalLengthSquare);

            scale *= screenSpaceNormalLengthInv;
        }
    }

    /* multiply in vertex level user scale factors to generate the desired 2d length */
    scale *= vertexThicknesses * _rpToonComputePerspectiveScale(ink, camspaceInnerVertex->z );

    /* Decrease with distance */
    scale *= camspaceInnerVertex->z;

    /* now desired 2d position of rib's outer vertex and back project*/
    camspaceOuterVertex->x = camspaceInnerVertex->x + (cameraSpaceNormal.x * scale);
    camspaceOuterVertex->y = camspaceInnerVertex->y + (cameraSpaceNormal.y * scale);

    RWRETURNVOID();
}

static void
ComputeFlatSilhouetteRibVertexPositions(RwV3d *camspaceInnerVertex,
                                        RwV2d *camspaceOuterVertex,
                                        RwReal vertexThicknesses,
                                        const RwV3d *objectSpaceInnerVertexPosition,
                                        const RwV3d *objectSpaceInnerVertexNormal,
                                        const ScreenSpaceProjection *ssp)
{
    RwV2d   cameraSpaceNormal;

    RwReal  screenSpaceNormalLengthSquare;
    RwReal  screenSpaceNormalLengthInv;

    RwReal  scale;

    RWFUNCTION(RWSTRING("ComputeFlatSilhouetteRibVertexPositions"));

    /* generate screenspace & camera space positions for inner vertex from object space position */
    RwV3dTransformPoint(camspaceInnerVertex, objectSpaceInnerVertexPosition, &ssp->matrix);

    /* generate screenspace & camera space positions for outer vertex from object space position */
    Fast2DTransformVector(&cameraSpaceNormal, objectSpaceInnerVertexNormal, &ssp->matrix);

    /* start with the object level user scale factor & accumulate more scale factors for per vertex effect */
    /* multiply in vertex level user scale factors to generate the desired 2d length */
    scale = vertexThicknesses * ((camspaceInnerVertex->z) * OneInkCoef1 + OneInkCoef2);

    /* Decrease with distance */
    scale *= camspaceInnerVertex->z;

    screenSpaceNormalLengthSquare = RwV2dDotProduct(&cameraSpaceNormal, &cameraSpaceNormal);

    if (screenSpaceNormalLengthSquare > (RPTOON_REALLY_SMALL_EDGE_THRESHOLD * RPTOON_REALLY_SMALL_EDGE_THRESHOLD))
    {
        #if !defined( NOSSEASM )
        if (UseSSE)
        {
            _asm
            {
                movss xmm0, screenSpaceNormalLengthSquare
                rsqrtss xmm0, xmm0
                movss xmm1, scale
                mulss xmm1, xmm0
                movss scale, xmm1
            }
        }
        else
        #endif
        {
            rwInvSqrtMacro(&screenSpaceNormalLengthInv, screenSpaceNormalLengthSquare);

            scale *= screenSpaceNormalLengthInv;
        }
    }

    /* now desired 2d position of rib's outer vertex and back project*/
    camspaceOuterVertex->x = camspaceInnerVertex->x + (cameraSpaceNormal.x * scale);
    camspaceOuterVertex->y = camspaceInnerVertex->y + (cameraSpaceNormal.y * scale);

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

    for (i = 0; i < numTris; i++)
    {
        RwV3dSub(&e0, &vertices[ tris[i].vertIndex[1] ], &vertices[ tris[i].vertIndex[0] ]);
        RwV3dSub(&e1, &vertices[ tris[i].vertIndex[2] ], &vertices[ tris[i].vertIndex[0] ]);
        RwV3dCrossProduct(&faceNormals[i], &e0, &e1);
    }

    RWRETURNVOID();
}

/****************************************************************************
 D3D8MatrixTranspose

 On entry   : Dest matrix pointer, one source matrix pointers
 On exit    : Matrix pointer contains transposed result
 */
static void
D3D8MatrixTranspose(D3DMATRIX *dstMat,
                    const D3DMATRIX *srcmat)
{
    RWFUNCTION(RWSTRING("D3D8MatrixTranspose"));
    RWASSERT(dstMat != NULL);
    RWASSERT(srcmat != NULL);

    /* Multiply out right */
    dstMat->m[0][0] = srcmat->m[0][0];
    dstMat->m[1][0] = srcmat->m[0][1];
    dstMat->m[2][0] = srcmat->m[0][2];
    dstMat->m[3][0] = srcmat->m[0][3];

    /* Then up */
    dstMat->m[0][1] = srcmat->m[1][0];
    dstMat->m[1][1] = srcmat->m[1][1];
    dstMat->m[2][1] = srcmat->m[1][2];
    dstMat->m[3][1] = srcmat->m[1][3];

    /* Then at */
    dstMat->m[0][2] = srcmat->m[2][0];
    dstMat->m[1][2] = srcmat->m[2][1];
    dstMat->m[2][2] = srcmat->m[2][2];
    dstMat->m[3][2] = srcmat->m[2][3];

    /* Then pos */
    dstMat->m[0][3] = srcmat->m[3][0];
    dstMat->m[1][3] = srcmat->m[3][1];
    dstMat->m[2][3] = srcmat->m[3][2];
    dstMat->m[3][3] = srcmat->m[3][3];

    /* And that's all folks */
    RWRETURNVOID();
}

/****************************************************************************
 GetProjectionTransposedMatrix

*/
static void
GetProjectionTransposedMatrix(void *projectionMatrix)
{
    D3DMATRIX projection;

    RWFUNCTION(RWSTRING("GetProjectionTransposedMatrix"));

    RwD3D8GetTransform(D3DTS_PROJECTION, &projection);

    D3D8MatrixTranspose(projectionMatrix, &projection);

    RWRETURNVOID();
}

/****************************************************************************
 GetWorldViewTransposedMatrix

*/
static void
GetWorldViewTransposedMatrix(void *worldViewMatrix,
                             const RwMatrix *transform)
{
    D3DMATRIX view;

    RWFUNCTION(RWSTRING("GetWorldViewTransposedMatrix"));

    RwD3D8GetTransform(D3DTS_VIEW, &view);

    if (transform != NULL)
    {
        D3DMATRIX worldView;

        RwMatrixMultiply((RwMatrix *)&worldView, transform, (const RwMatrix *)&view);

        worldView._14 = 0.0f;
        worldView._24 = 0.0f;
        worldView._34 = 0.0f;
        worldView._44 = 1.0f;

        D3D8MatrixTranspose(worldViewMatrix, &worldView);
    }
    else
    {
        D3D8MatrixTranspose(worldViewMatrix, &view);
    }

    RWRETURNVOID();
}

/* brute force silhouette edge renderer functions - needs the line render code */
void
_rpToonBruteSilhouetteRender(RpToonGeo *toonGeo,
                             const RwV3d *verts,
                             const RwV3d *normals,
                             RwBool faceNormalsInvalid,
                             const RwMatrix *transform,
                             const ScreenSpaceProjection *ssp)
{
    RwCullMode              oldCullMode;
    RwInt32                 i;

    RWFUNCTION(RWSTRING("_rpToonBruteSilhouetteRender"));

    RWASSERT(toonGeo != 0);

    /* Early out */
    if (toonGeo->numEdges == 0)
    {
        RWRETURNVOID();
    }

    /*
      Turn off backface culling for silhouettes because it's a bit tricky to keep the order consistent
    */
    RwRenderStateGet(rwRENDERSTATECULLMODE, (void*)&oldCullMode);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);

    if (faceNormalsInvalid)
    {
        ToonComputeFaceNormals(
            toonGeo->numTriangles,
            toonGeo->triangles,
            verts,
            toonGeo->faceNormals);
    }

    /* Update silhouette edges */
    _rpToonUpdateSilhouetteEdges(toonGeo, verts, transform);

    /* Check if we have just one ink for the whole silhouette */
    if ( toonGeo->inkIDCount <= 2) /* That means one for silhouette and another for crease */
    {
        const RpToonInk *ink = toonGeo->inks[toonGeo->edgeInkIDs[0].inkId[RPTOON_INKTYPE_SILHOUETTE]];

        if (UseVertexShader)
        {
            RwV4d matrixConstants[8];

            GetProjectionTransposedMatrix(matrixConstants + 0);
            GetWorldViewTransposedMatrix(matrixConstants + 4, transform);

            matrixConstants[7].x = (ink->farScale_factor) * (ink->perspectiveScale_b);
            matrixConstants[7].y = (ink->farScale_factor) * ((ink->perspectiveScale_a) - (ink->perspectiveScale_b) * (ink->perspectiveScale_zMin));
            matrixConstants[7].z = 0.0f;
            matrixConstants[7].w = 1.0f;

            RwD3D8SetVertexShaderConstant(0, matrixConstants, 8);

            _rpToonEdgeRenderPrepareVertexShaderFlatSubmit(toonGeo->numEdges,
                                                           ink->color);

            /* Determine edge visibility & do extrusion */
            for (i = 0; i < toonGeo->numEdges; i++)
            {
                const RwUInt32 face0 = toonGeo->edges[i].face[0];
                const RwUInt32 face1 = toonGeo->edges[i].face[1];

                /* compare visibility of edge's two polys (if it's got two that is!) */
                if ( (face1 != RPTOONEDGE_NONEIGHBOURFACE) &&
                    (_rpToonD3D8IsFrontFace[face0] != _rpToonD3D8IsFrontFace[face1]) )
                {
                    const RwUInt32 innerVertexIndex0 = toonGeo->edges[i].v[0];
                    const RwUInt32 innerVertexIndex1 = toonGeo->edges[i].v[1];

                    _rpToonEdgeRenderAppendVertexShaderFlatQuad(&verts[innerVertexIndex0],
                                                                &verts[innerVertexIndex1],
                                                                &normals[innerVertexIndex0],
                                                                &normals[innerVertexIndex1],
                                                                toonGeo->vertexThicknesses[innerVertexIndex0],
                                                                toonGeo->vertexThicknesses[innerVertexIndex1]);
                }
            }

            _rpToonEdgeRenderVertexShaderFlatSubmit();
        }
        else
        {
            OneInkCoef1 = (ink->farScale_factor) * (ink->perspectiveScale_b);
            OneInkCoef2 = (ink->farScale_factor) * ((ink->perspectiveScale_a) - (ink->perspectiveScale_b) * (ink->perspectiveScale_zMin));

            _rpToonEdgeRenderPrepareFlatSubmit(toonGeo->numEdges,
                                               ink->color);

            /* Determine edge visibility & do extrusion */
            for (i = 0; i < toonGeo->numEdges; i++)
            {
                const RwUInt32 face0 = toonGeo->edges[i].face[0];
                const RwUInt32 face1 = toonGeo->edges[i].face[1];

                /* compare visibility of edge's two polys (if it's got two that is!) */
                if ( (face1 != RPTOONEDGE_NONEIGHBOURFACE) &&
                    (_rpToonD3D8IsFrontFace[face0] != _rpToonD3D8IsFrontFace[face1]) )
                {
                    const RwUInt32 innerVertexIndex0 = toonGeo->edges[i].v[0];
                    const RwUInt32 innerVertexIndex1 = toonGeo->edges[i].v[1];
                    _rpToonVertexCache *vertex0 = &(_rpToonD3D8VertexCache[innerVertexIndex0]);
                    _rpToonVertexCache *vertex1 = &(_rpToonD3D8VertexCache[innerVertexIndex1]);

                    if (vertex0->ink == NULL)
                    {
                        vertex0->ink = ink;

                        ComputeFlatSilhouetteRibVertexPositions(&(vertex0->camspaceInnerVertex),
                                                                &(vertex0->camspaceOuterVertex),
                                                                toonGeo->vertexThicknesses[innerVertexIndex0],
                                                                &verts[innerVertexIndex0],
                                                                &normals[innerVertexIndex0],
                                                                ssp);
                    }

                    if (vertex1->ink == NULL)
                    {
                        vertex1->ink = ink;

                        ComputeFlatSilhouetteRibVertexPositions(&(vertex1->camspaceInnerVertex),
                                                                &(vertex1->camspaceOuterVertex),
                                                                toonGeo->vertexThicknesses[innerVertexIndex1],
                                                                &verts[innerVertexIndex1],
                                                                &normals[innerVertexIndex1],
                                                                ssp);
                    }

                    _rpToonEdgeRenderAppendFlatQuad(&(vertex0->camspaceInnerVertex),
                                                    &(vertex1->camspaceInnerVertex),
                                                    &(vertex0->camspaceOuterVertex),
                                                    &(vertex1->camspaceOuterVertex));
                }
            }

            _rpToonEdgeRenderFlatSubmit();
        }
    }
    else
    {
        if (UseVertexShader)
        {
            RwV4d matrixConstants[8];

            GetProjectionTransposedMatrix(matrixConstants + 0);
            GetWorldViewTransposedMatrix(matrixConstants + 4, transform);

            RwD3D8SetVertexShaderConstant(0, matrixConstants, 7);

            _rpToonEdgeRenderPrepareVertexShaderSubmit(toonGeo->numEdges);

            /* Determine edge visibility & do extrusion */
            for (i = 0; i < toonGeo->numEdges; i++)
            {
                const RwUInt32 face0 = toonGeo->edges[i].face[0];
                const RwUInt32 face1 = toonGeo->edges[i].face[1];

                /* compare visibility of edge's two polys (if it's got two that is!) */
                if ( (face1 != RPTOONEDGE_NONEIGHBOURFACE) &&
                    (_rpToonD3D8IsFrontFace[face0] != _rpToonD3D8IsFrontFace[face1]) )
                {
                    const RwUInt32 innerVertexIndex0 = toonGeo->edges[i].v[0];
                    const RwUInt32 innerVertexIndex1 = toonGeo->edges[i].v[1];
                    const RpToonInk *ink = toonGeo->inks[toonGeo->edgeInkIDs[i].inkId[RPTOON_INKTYPE_SILHOUETTE]];

                    _rpToonEdgeRenderAppendVertexShaderQuad(&verts[innerVertexIndex0],
                                                            &verts[innerVertexIndex1],
                                                            &normals[innerVertexIndex0],
                                                            &normals[innerVertexIndex1],
                                                            toonGeo->vertexThicknesses[innerVertexIndex0],
                                                            toonGeo->vertexThicknesses[innerVertexIndex1],
                                                            ink);
                }
            }

            _rpToonEdgeRenderVertexShaderSubmit();
        }
        else
        {
            _rpToonEdgeRenderPrepareSubmit(toonGeo->numEdges);

            /* Determine edge visibility & do extrusion */
            for (i = 0; i < toonGeo->numEdges; i++)
            {
                const RwUInt32 face0 = toonGeo->edges[i].face[0];
                const RwUInt32 face1 = toonGeo->edges[i].face[1];

                /* compare visibility of edge's two polys (if it's got two that is!) */
                if ( (face1 != RPTOONEDGE_NONEIGHBOURFACE) &&
                    (_rpToonD3D8IsFrontFace[face0] != _rpToonD3D8IsFrontFace[face1]) )
                {
                    const RwUInt32 innerVertexIndex0 = toonGeo->edges[i].v[0];
                    const RwUInt32 innerVertexIndex1 = toonGeo->edges[i].v[1];
                    _rpToonVertexCache *vertex0 = &(_rpToonD3D8VertexCache[innerVertexIndex0]);
                    _rpToonVertexCache *vertex1 = &(_rpToonD3D8VertexCache[innerVertexIndex1]);
                    const RpToonInk *ink = toonGeo->inks[toonGeo->edgeInkIDs[i].inkId[RPTOON_INKTYPE_SILHOUETTE]];

                    if (vertex0->ink != ink)
                    {
                        if (vertex0->ink != NULL)
                        {
                            ReComputeSilhouetteRibVertexPositions(&(vertex0->camspaceOuterVertex),
                                                                toonGeo->vertexThicknesses[innerVertexIndex0],
                                                                &(vertex0->camspaceInnerVertex),
                                                                &normals[innerVertexIndex0],
                                                                ssp,
                                                                ink );
                        }
                        else
                        {
                            ComputeSilhouetteRibVertexPositions(&(vertex0->camspaceInnerVertex),
                                                                &(vertex0->camspaceOuterVertex),
                                                                toonGeo->vertexThicknesses[innerVertexIndex0],
                                                                &verts[innerVertexIndex0],
                                                                &normals[innerVertexIndex0],
                                                                ssp,
                                                                ink );
                        }

                        vertex0->ink = ink;
                    }

                    if (vertex1->ink != ink)
                    {
                        if (vertex1->ink != NULL)
                        {
                            ReComputeSilhouetteRibVertexPositions(&(vertex1->camspaceOuterVertex),
                                                                toonGeo->vertexThicknesses[innerVertexIndex1],
                                                                &(vertex1->camspaceInnerVertex),
                                                                &normals[innerVertexIndex1],
                                                                ssp,
                                                                ink );
                        }
                        else
                        {
                            ComputeSilhouetteRibVertexPositions(&(vertex1->camspaceInnerVertex),
                                                                &(vertex1->camspaceOuterVertex),
                                                                toonGeo->vertexThicknesses[innerVertexIndex1],
                                                                &verts[innerVertexIndex1],
                                                                &normals[innerVertexIndex1],
                                                                ssp,
                                                                ink );
                        }

                        vertex1->ink = ink;
                    }

                    _rpToonEdgeRenderAppendQuad(&(vertex0->camspaceInnerVertex),
                                                &(vertex1->camspaceInnerVertex),
                                                &(vertex0->camspaceOuterVertex),
                                                &(vertex1->camspaceOuterVertex),
                                                ink->color);
                }
            }

            _rpToonEdgeRenderSubmit();
        }
    }

    RwRenderStateSet(rwRENDERSTATECULLMODE, (void *)oldCullMode);

    RWRETURNVOID();
}
