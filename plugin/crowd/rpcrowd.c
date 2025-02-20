/*
 *  rpcrowd.c : Animated crowd plugin for RenderWare.
 */

/**
 * \ingroup rpcrowd
 * \page rpcrowdoverview RpCrowd Plugin Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rprandom.h, rpcrowd.h
 * \li \b Libraries: rwcore, rpworld, rprandom, rpcrowd
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpRandomPluginAttach,
 *     \ref RpCrowdPluginAttach
 *
 * \subsection crowdoverview Overview
 * The RpCrowd plugin provides efficient rendering of animated billboard
 * crowds for use as backgrounds in stadia, racetracks etc.
 *
 * The plugin should be attached with \ref RpCrowdPluginAttach.
 *
 * <B> RpCrowd Concepts </B>
 *
 * A stadium may contain a number of crowds. These might be for
 * home and away teams, or for breaking a larger crowd into smaller
 * regions.
 *
 * Each crowd is modelled as a number of stadium seating sections (quads).
 *
 * In each section, the spectators are displayed by rendering a number of
 * rows of billboards.
 *
 * Animation is achieved by changing the texture rendered on each billboard.
 * On some platforms, this is done by simply offsetting texture coordinates
 * within a single crowd texture that contains multiple animation frames.
 *
 * <B> Crowd Texture Maps </B>
 *
 * Crowd textures consist of several rows. Each row corresponds to the same
 * spectators but in different animation states. These spectators
 * are used to generate an entire crowd, but with random offsets for each
 * billboard. See \ref RpCrowdTexture for details.
 *
 * <B> Crowd Animations </B>
 *
 * Crowd animations are required to specify how the rows of the crowd texture
 * should be applied to the billboards in sequence. See \ref RpCrowdAnimation
 * for details.
 *
 * <B> Crowd Base Atomic </B>
 *
 * RpCrowd objects are created from base atomics which specify the
 * crowd seating sections (quads). These would normally be created in a
 * modelling package where they can be aligned with a stadium model.
 * Note that atomics are exported within an \ref RpClump and should be
 * extracted with \ref RpClumpForAllAtomics. Base atomics themselves
 * are never rendered.
 *
 * The base atomic should have a tri-list geometry where pairs of triangles
 * form quads for the sections. These should be aligned with the ground as
 * follows:
 *
 * \verbatim
                 back
           u0,v0      u1,v0
         h1   ----------
             |          |
             |          |   Viewed from above.
             |          |
         h0   ----------
           u0,v1      u1,v1

             front (pitch)

      texture coordinates: u1 > u0, v1 > v0
      height: h1 > h0
   \endverbatim
 *
 * The quads need not be rectangular in shape, but the front and back edges
 * should be horizontal and parallel. The base atomic's frame is used to
 * position the crowd in world space.
 *
 * The number of rows of spectators in the quad, and the number of spectators
 * in a row is based on the texture coordinates and is controlled by
 * creation parameters. For trapezium shaped sections, the number of
 * spectators in a row will be scaled to keep a constant density, but always
 * rounded to a whole number.
 *
 * Note that quads should not share vertices with other quads.
 *
 * <B> Creation parameters </B>
 *
 * A crowd is created using \ref RpCrowdCreate which takes a pointer to
 * set of creation parameters (\ref RpCrowdParam). The parameter block
 * specifies the base atomic, the textures and the animations along with
 * other information such as the spectator density, the up-vector for the
 * billboards, and the layout of the crowd textures. See \ref RpCrowdParam
 * for the details.
 *
 * <B> Updating animation </B>
 *
 * Once a crowd has been created, the textures and animation can be updated
 * using
 *
 * \ref RpCrowdSetCurrentTexture,
 * \ref RpCrowdSetCurrentAnim,
 * \ref RpCrowdSetCurrentAnimTime
 *
 * \ref RpCrowdAddAnimTime
 *
 * \ref RpCrowdGetCurrentTexture,
 * \ref RpCrowdGetCurrentAnim,
 * \ref RpCrowdGetCurrentAnimTime
 *
 * <B> Rendering </B>
 *
 * Use \ref RpCrowdRender to render a crowd.
 *
 */

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpcrowd.h"
#include "crowdcommon.h"

#include "string.h"

/*===========================================================================*
 *--- Public Global Variables -----------------------------------------------*
 *===========================================================================*/

#if (defined(RWDEBUG))
long rpCrowdStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

RwModuleInfo _rpCrowdModule = { 0, 0 };
RwInt32      _rpCrowdEngineDataOffset = 0;

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/

/* For geometry conditioning */
typedef struct QuadPair QuadPair;
struct QuadPair
{
    int                 tri1;
    int                 tri2;
    int                 tri1v[3];
    int                 tri2v[3];
};

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

static void *
CrowdOpen( void *instance,
           RwInt32 __RWUNUSED__ offset,
           RwInt32 __RWUNUSED__ size    )
{
    RWFUNCTION(RWSTRING("CrowdOpen"));
    RWASSERT(instance);

    _rpCrowdDeviceOpen();

    /* one more module instance */
    _rpCrowdModule.numInstances++;

    RWRETURN(instance);
}

static void *
CrowdClose( void *instance,
            RwInt32 __RWUNUSED__ offset,
            RwInt32 __RWUNUSED__ size    )
{
    RWFUNCTION(RWSTRING("CrowdClose"));
    RWASSERT(instance);

    _rpCrowdDeviceClose();

    /* one less module instance */
    _rpCrowdModule.numInstances--;

    RWRETURN(instance);
}

#ifdef RWDEBUG
/******************************************************************************
 */
#define V3DNORMEPS  (0.01f)

static RwBool
V3dIsNormalized(RwV3d *vec)
{
    RwBool  result;
    RwReal  error;

    RWFUNCTION(RWSTRING("V3dIsNormalized"));

    error = RwV3dDotProduct(vec, vec) - 1.0f;
    result = (-V3DNORMEPS < error) && (error < V3DNORMEPS);

    RWRETURN(result);
}
#endif

/*===========================================================================*
 *--- Geometry conditioning -------------------------------------------------*
 *===========================================================================*/

/*
 *  The base geometry specifying the crowd sections must be conditioned
 *  for use by generic and platform specific methods. The
 *  CrowdConditionTriList function converts the triangle list and the
 *  meshes so that pairs of triangles correspond to quads made up of
 *  vertices v0, v1, v2 of triangle 1, and vertex v0 of triangle 2 as follows:
 *
 *    t1v2-----t2v0
 *      |        |
 *      |        |      (viewed from above)
 *      |        |
 *    t1v0-----t1v1
 *
 *  The lower two vertices should conventionally be the front row of the
 *  crowd section. The top-left corner would usually have uv coords (0,0).
 */

/****************************************************************************
 *  _quadPairMatSortCB
 */
static RpTriangle *gTris = (RpTriangle *)NULL;

static int
_quadPairMatSortCB(const void *data1, const void *data2)
{
    const QuadPair *elem1;
    const QuadPair *elem2;
    int value;

    RWFUNCTION(RWSTRING("_quadPairMatSortCB"));

    RWASSERT(NULL != gTris);

    elem1 = (const QuadPair *)data1;
    elem2 = (const QuadPair *)data2;

    value = ( gTris[elem1->tri1].matIndex -
              gTris[elem1->tri2].matIndex );

    RWRETURN(value);
}

/****************************************************************************
 *  CrowdGenerateQuadPairArray
 */
static              RwInt32
CrowdGenerateQuadPairArray(RpGeometry * geom, QuadPair * quadPairArray)
{
    RwBool             *usedTris =
        (RwBool *) RwMalloc(sizeof(RwBool) * geom->numTriangles,
                            rwID_CROWDPPPLUGIN | rwMEMHINTDUR_FUNCTION);
    RwInt32             numQuads = 0, i, j;
    RpTriangle         *tris = geom->triangles;

    RWFUNCTION(RWSTRING("CrowdGenerateQuadPairArray"));

    RWASSERT(NULL != usedTris);
    RWASSERTM(RpGeometryGetFlags(geom) & rpGEOMETRYTEXTURED ||
              RpGeometryGetFlags(geom) & rpGEOMETRYTEXTURED2,
              (RWSTRING("Crowd geometries must have texture coordinates")));

    for (i = 0; i < geom->numTriangles; i++)
    {
        usedTris[i] = FALSE;
    }

    while (numQuads < geom->numTriangles / 2)
    {
        i = 0;
        while (usedTris[i] == TRUE)
        {
            i++;
        }

        quadPairArray[numQuads].tri1 = i;
        usedTris[i] = TRUE;
        quadPairArray[numQuads].tri2 = -1;

        /* now find a matching tri for this quad. */
        for (j = i + 1;
             (j < geom->numTriangles) &&
             (quadPairArray[numQuads].tri2 == -1);
             j++)
        {
            RwInt32             numSharedVerts = 0;
            RwInt32             v[4], k;

            /* must share two verts */

            for (k = 0; k < 3; k++)
            {
                if (tris[i].vertIndex[0] == tris[j].vertIndex[k])
                {
                    v[numSharedVerts] = tris[i].vertIndex[0];
                    numSharedVerts++;
                }
            }
            for (k = 0; k < 3; k++)
            {
                if (tris[i].vertIndex[1] == tris[j].vertIndex[k])
                {
                    v[numSharedVerts] = tris[i].vertIndex[1];
                    numSharedVerts++;
                }
            }
            for (k = 0; k < 3; k++)
            {
                if (tris[i].vertIndex[2] == tris[j].vertIndex[k])
                {
                    v[numSharedVerts] = tris[i].vertIndex[2];
                    numSharedVerts++;
                }
            }

            if (numSharedVerts == 2)
            {
                RwBool              diagedge = TRUE;
                RwTexCoords        *vertexTexCoords = geom->texCoords[0];

                /* check it's a diagonal edge */
                /* the verts at the end of the line can't share common u or v coords */
                if ((vertexTexCoords[v[0]].u == vertexTexCoords[v[1]].u) ||
                    (vertexTexCoords[v[0]].v == vertexTexCoords[v[1]].v))
                {
                    diagedge = FALSE;
                }

                if (diagedge)
                {
                    int lowv[2], highv[2];
                    int lows = 0;
                    int highs = 0;
                    quadPairArray[numQuads].tri2 = j;

                    usedTris[j] = TRUE;

                    /* find the other verts */
                    for (k = 0; k < 3; k++)
                    {
                        if (tris[i].vertIndex[k] != v[0] &&
                            tris[i].vertIndex[k] != v[1])
                        {
                            v[2] = tris[i].vertIndex[k];
                        }
                    }
                    for (k = 0; k < 3; k++)
                    {
                        if (tris[j].vertIndex[k] != v[0] &&
                            tris[j].vertIndex[k] != v[1])
                        {
                            v[3] = tris[j].vertIndex[k];
                        }
                    }

                    /* now rearrange to get minu, maxv -> maxu, maxv -> minu, minv -> maxu, minv
                     * in the following indices:  tri1[0], tri1[1], tri1[2], tri2[0] */
                    for (k=0; k<4; k++)
                    {
                        if (((vertexTexCoords[v[k]].v < vertexTexCoords[v[(k+1)%4]].v) +
                             (vertexTexCoords[v[k]].v < vertexTexCoords[v[(k+2)%4]].v) +
                             (vertexTexCoords[v[k]].v < vertexTexCoords[v[(k+3)%4]].v)) > 1)
                        {
                            lowv[lows] = k;
                            lows++;
                        }
                        else
                        {
                            highv[highs] = k;
                            highs++;
                        }
                    }

                    if (vertexTexCoords[v[lowv[0]]].u < vertexTexCoords[v[lowv[1]]].u)
                    {
                        quadPairArray[numQuads].tri1v[2] = v[lowv[0]];
                        quadPairArray[numQuads].tri2v[1] = v[lowv[0]];
                        quadPairArray[numQuads].tri2v[0] = v[lowv[1]];
                    }
                    else
                    {
                        quadPairArray[numQuads].tri1v[2] = v[lowv[1]];
                        quadPairArray[numQuads].tri2v[1] = v[lowv[1]];
                        quadPairArray[numQuads].tri2v[0] = v[lowv[0]];
                    }

                    if (vertexTexCoords[v[highv[0]]].u < vertexTexCoords[v[highv[1]]].u)
                    {
                        quadPairArray[numQuads].tri1v[0] = v[highv[0]];
                        quadPairArray[numQuads].tri1v[1] = v[highv[1]];
                        quadPairArray[numQuads].tri2v[2] = v[highv[1]];
                    }
                    else
                    {
                        quadPairArray[numQuads].tri1v[0] = v[highv[1]];
                        quadPairArray[numQuads].tri1v[1] = v[highv[0]];
                        quadPairArray[numQuads].tri2v[2] = v[highv[0]];
                    }
                }
            }
        }
        RWASSERTM(quadPairArray[numQuads].tri2 != -1,
                  (RWSTRING("No matching triangle found for crowd quad.")));

        numQuads++;
    }

    /* Now sort by material so we can rebuild the meshes by hand easily */
    gTris = tris;
    qsort(quadPairArray, numQuads, sizeof(QuadPair), _quadPairMatSortCB);

    RwFree(usedTris);

    RWRETURN(numQuads);
}

/****************************************************************************
 *  CrowdConditionTriList
 */
static RwBool
CrowdConditionTriList(RpGeometry * baseGeom)
{
    QuadPair           *quadPairArray;
    RwInt32             quad, numQuads, meshNum, numTris, i;
    RpTriangle         *newTris, *oldTris;
    RpMesh             *mesh;

    RWFUNCTION(RWSTRING("CrowdConditionTriList"));

    RWASSERT(NULL != baseGeom);

    /* allocate a list of quadpairs */
    numTris = baseGeom->numTriangles;
    quadPairArray = (QuadPair *) RwMalloc(sizeof(QuadPair)*numTris,
                           rwID_CROWDPPPLUGIN | rwMEMHINTDUR_FUNCTION);
    RWASSERT(NULL != quadPairArray);

    /* initialise QuadPairs (which group pairs of triangles) in material order */
    numQuads = CrowdGenerateQuadPairArray(baseGeom, quadPairArray);

    newTris = (RpTriangle *) RwMalloc(sizeof(RpTriangle)*numTris,
                          rwID_CROWDPPPLUGIN | rwMEMHINTDUR_FUNCTION);
    RWASSERT(NULL != newTris);
    oldTris = baseGeom->triangles;

    /* now sort the triangle list one mesh at a time */
    quad = 0;
    mesh = (RpMesh *)(baseGeom->mesh + 1);
    for (meshNum = 0; meshNum < baseGeom->mesh->numMeshes; meshNum++)
    {
        RxVertexIndex *newMesh;

        RWASSERT(0 == (mesh->numIndices % 3));
        numTris = mesh->numIndices / 3;
        /* All meshes should be composed of quads */
        RWASSERT(0 == (numTris % 2));

        /* The meshes will be in matList order */
        mesh->material = baseGeom->matList.materials[meshNum];

        newMesh = (RxVertexIndex *) RwMalloc(sizeof(RxVertexIndex)*3*numTris,
                                  rwID_CROWDPPPLUGIN | rwMEMHINTDUR_FUNCTION);
        RWASSERT(NULL != newMesh);

        for (i = 0; i < (numTris >> 1); i++)
        {
            newTris[quad*2 + 0] = oldTris[quadPairArray[quad].tri1];
            newTris[quad*2 + 1] = oldTris[quadPairArray[quad].tri2];
            RWASSERT(newTris[quad*2 + 0].matIndex == meshNum);
            RWASSERT(newTris[quad*2 + 1].matIndex == meshNum);

            /* and reorder the verts */
            newTris[quad*2 + 0].vertIndex[0] = quadPairArray[quad].tri1v[0];
            newTris[quad*2 + 0].vertIndex[1] = quadPairArray[quad].tri1v[1];
            newTris[quad*2 + 0].vertIndex[2] = quadPairArray[quad].tri1v[2];
            newTris[quad*2 + 1].vertIndex[0] = quadPairArray[quad].tri2v[0];
            newTris[quad*2 + 1].vertIndex[1] = quadPairArray[quad].tri2v[1];
            newTris[quad*2 + 1].vertIndex[2] = quadPairArray[quad].tri2v[2];

            /* and set up the mesh indices in parallel */
            newMesh[i*6 + 0] = newTris[quad*2 + 0].vertIndex[0];
            newMesh[i*6 + 1] = newTris[quad*2 + 0].vertIndex[1];
            newMesh[i*6 + 2] = newTris[quad*2 + 0].vertIndex[2];
            newMesh[i*6 + 3] = newTris[quad*2 + 1].vertIndex[0];
            newMesh[i*6 + 4] = newTris[quad*2 + 1].vertIndex[1];
            newMesh[i*6 + 5] = newTris[quad*2 + 1].vertIndex[2];

            quad++;
        }

        memcpy((mesh + 1), newMesh, sizeof(RxVertexIndex)*3*numTris);
        RwFree(newMesh);

        mesh = (RpMesh *)(((RxVertexIndex *)(mesh + 1)) + mesh->numIndices);
    }
    RWASSERT(quad == numQuads); /* Make sure everything was accounted for */

    memcpy(oldTris, newTris,
           sizeof(RpTriangle)*RpGeometryGetNumTriangles(baseGeom));

    RwFree(newTris);
    RwFree(quadPairArray);

    baseGeom->lockedSinceLastInst = rpGEOMETRYLOCKALL;

    RWRETURN(TRUE);
}

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rpcrowd
 * \ref RpCrowdPluginAttach
 * is used to attach the Crowd plugin to the RenderWare system.
 * The plugin must be attached between initializing the system
 * with \ref RwEngineInit
 * and opening it with \ref RwEngineOpen.
 *
 * \return TRUE if successful, FALSE if an error occurs.
 *
 * \see RpCrowdCreate
 * \see \ref rpcrowdoverview
 */
RwBool
RpCrowdPluginAttach( void )
{
    RwBool result;
    RWAPIFUNCTION(RWSTRING("RpCrowdPluginAttach"));

    _rpCrowdEngineDataOffset =
        RwEngineRegisterPlugin( sizeof(CrowdEngineData),
                                rwID_CROWDPPPLUGIN,
                                CrowdOpen,
                                CrowdClose );

    result = ( _rpCrowdEngineDataOffset >= 0);

    RWRETURN(result);
}

/**
 * \ingroup rpcrowd
 * \ref RpCrowdCreate
 * creates a crowd using the given parameters. See \ref RpCrowdParam
 * for the full list of parameters that must be specified. By default,
 * the crowd is initialized to use the first texture and set to time zero
 * for the first animation.
 *
 * After this function call, the crowd object does not retain any
 * dependence on the base atomic, which may be destroyed by the
 * application.
 *
 * Also, interal reference counts are added as necessary for the
 * crowd textures, so these may also be destroyed by the application after
 * calling this function (see \ref RwTextureDestroy). Some platforms (PC)
 * convert the textures, and do not require the originals. In this case,
 * the reference count isn't incremented, and the textures will be
 * removed from the system when they are destroyed by the application.
 *
 * \param param  Parameters for the crowd.
 *
 * \return A pointer to the created crowd, or NULL if an error occurred.
 *
 * \see RpCrowdDestroy
 * \see RpCrowdSetCurrentTexture
 * \see RpCrowdGetCurrentTexture
 * \see RpCrowdSetCurrentAnim
 * \see RpCrowdGetCurrentAnim
 * \see RpCrowdSetCurrentAnimTime
 * \see RpCrowdGetCurrentAnimTime
 * \see RpCrowdAddAnimTime
 * \see RpCrowdGetParam
 * \see RpCrowdRender
 * \see RpCrowdPluginAttach
 * \see \ref rpcrowdoverview
 */
RpCrowd *
RpCrowdCreate(RpCrowdParam *param)
{
    RpCrowd     *crowd;
    RpGeometry  *baseGeom;
    RwUInt32    size;
    RwUInt32    i;

    RWAPIFUNCTION(RWSTRING("RpCrowdCreate"));
    RWASSERT(_rpCrowdModule.numInstances);
    RWASSERT(param);

    RWASSERT(param->base);
    RWASSERT(param->densityU > 0.0f);
    RWASSERT(param->densityV > 0.0f);
    RWASSERT(V3dIsNormalized(&param->up));
    RWASSERT(param->height > 0.0f);

    RWASSERT(param->numTextures > 0);
    RWASSERT(param->numTexCols > 0);
    RWASSERT(param->numTexRows > 0);
    RWASSERT(param->textures);

    RWASSERT(param->numAnims > 0);
    RWASSERT(param->anims);

    /* Get some memory for the crowd object. */
    size =  sizeof(RpCrowd)
         +  _rpCrowdDeviceDataSize
         + (sizeof(RpCrowdTexture *) * param->numTextures)
         + (sizeof(RpCrowdAnimation *) * param->numAnims);

    crowd = (RpCrowd *)RwMalloc(size,
        rwID_CROWD | rwMEMHINTDUR_EVENT);
    if (!crowd)
    {
        RWERROR((E_RW_NOMEM, size));
        RWRETURN((RpCrowd *)NULL);
    }

    memset(crowd, 0, size);

    /* Take a copy of the parameters */
    memcpy(&crowd->param, param, sizeof(RpCrowdParam));

    /* Textures */
    crowd->param.textures = (RpCrowdTexture **)
        ((RwUInt8 *)(crowd + 1) + _rpCrowdDeviceDataSize);

    for (i=0; i < param->numTextures; i++)
    {
        RWASSERT(param->textures[i]);
        crowd->param.textures[i] = param->textures[i];
        (void)RwTextureAddRef(param->textures[i]);
    }

    /* Animations */
    crowd->param.anims = (RpCrowdAnimation **)
        (crowd->param.textures + param->numTextures);

    for (i=0; i < param->numAnims; i++)
    {
        RWASSERT(param->anims[i]);
        crowd->param.anims[i] = param->anims[i];
    }

    /* First step is always to condition the base trilist geometry into
     * pairs of triangles with vertices in a well-defined order */
    baseGeom = RpAtomicGetGeometry(param->base);
    RWASSERT(baseGeom);
    RWASSERT(rwPRIMTYPETRILIST == RpMeshHeaderGetPrimType(baseGeom->mesh));
    if (!CrowdConditionTriList(baseGeom))
    {
        RwFree(crowd);
        RWRETURN((RpCrowd *)NULL);
    }

    /* Now create any platform specific data */
    if (!_rpCrowdDeviceInitCrowd(crowd))
    {
        RwFree(crowd);
        RWRETURN((RpCrowd *)NULL);
    }

    /* Should no longer reference the base geometry. Device function may
     * also have removed references to the original textures. */
    crowd->param.base  = (RpAtomic *) NULL;

    /* Initialize animation */
    crowd->currAnim = 0;
    crowd->currAnimTime = 0.0f;
    crowd->currTexture = 0;

    /* Check memory. */
    RWCRTCHECKMEMORY();

    /* And we're finished. */
    RWRETURN(crowd);
}

/**
 * \ingroup rpcrowd
 * \ref RpCrowdDestroy
 * destroys a crowd. This will also destroy any textures used by the crowd
 * (assuming they are not referenced elsewhere).
 *
 * \param crowd A pointer to the crowd to be destroyed.
 *
 * \return TRUE on success, FALSE otherwise
 *
 * \see RpCrowdCreate
 * \see RpCrowdPluginAttach
 * \see \ref rpcrowdoverview
 */
RwBool
RpCrowdDestroy(RpCrowd *crowd)
{
    RwUInt32 i;

    RWAPIFUNCTION(RWSTRING("RpCrowdDestroy"));
    RWASSERT(_rpCrowdModule.numInstances);
    RWASSERT(crowd);
    RWASSERT(crowd->param.textures);

    /* De-initialize any platform specific data */
    _rpCrowdDeviceDeInitCrowd(crowd);

    /* Destroy the textures. */
    for (i=0; i < crowd->param.numTextures; i++)
    {
        if (crowd->param.textures[i])
        {
            RwTextureDestroy(crowd->param.textures[i]);
            crowd->param.textures[i] = (RpCrowdTexture *)NULL;
        }
    }

    /* Free the crowd data. */
    RwFree(crowd);

    RWRETURN(TRUE);
}

/**
 * \ingroup rpcrowd
 * \ref RpCrowdGetParam
 * returns a pointer to the parameters giving the current state of the
 * specified crowd (read only).
 *
 * \param crowd A pointer to a crowd.
 *
 * \return Returns a pointer to the parameter for the crowd.
 *
 * \see RpCrowdCreate
 * \see RpCrowdSetCurrentTexture
 * \see RpCrowdSetCurrentAnim
 * \see RpCrowdSetCurrentAnimTime
 * \see RpCrowdAddAnimTime
 * \see RpCrowdPluginAttach
 * \see \ref rpcrowdoverview
 */
const RpCrowdParam *
RpCrowdGetParam(const RpCrowd *crowd)
{
    const RpCrowdParam *result;

    RWAPIFUNCTION(RWSTRING("RpCrowdGetParam"));
    RWASSERT(_rpCrowdModule.numInstances);
    RWASSERT(NULL != crowd);

    result = &(crowd->param);

    RWRETURN(result);
}

/**
 * \ingroup rpcrowd
 * \ref RpCrowdAddAnimTime
 * increments the current time of a crowd animation. Once the time reaches the
 * duration of the animation, it will automatically loop back to the start.
 * The current time may be retrieved using \ref RpCrowdGetCurrentAnimTime.
 *
 * \param  crowd    A pointer to the crowd.
 * \param  time     Time increment; this must be non-negative
 *
 * \return Returns a pointer to the crowd.
 *
 * \see RpCrowdSetCurrentAnimTime
 * \see RpCrowdGetCurrentAnimTime
 * \see RpCrowdSetCurrentAnim
 * \see RpCrowdGetCurrentAnim
 * \see \ref rpcrowdoverview
 */
RpCrowd *
RpCrowdAddAnimTime(RpCrowd *crowd, RwReal time)
{
    RwReal          duration;

    RWAPIFUNCTION(RWSTRING("RpCrowdAddAnimTime"));
    RWASSERT(_rpCrowdModule.numInstances);
    RWASSERT(crowd);
    RWASSERT(time >= 0.0f);

    /* Get duration for current animation */
    RWASSERT(crowd->currAnim < crowd->param.numAnims);
    RWASSERT(crowd->param.anims[crowd->currAnim]);
    duration = crowd->param.anims[crowd->currAnim]->duration;

    /* Add time, modulo duration */
    crowd->currAnimTime += time;
    while (crowd->currAnimTime > duration)
    {
        crowd->currAnimTime -= duration;
    }

    RWRETURN(crowd);
}

/**
 * \ingroup rpcrowd
 * \ref RpCrowdRender
 * renders a crowd.
 *
 * \param crowd A pointer to the crowd.
 *
 * \return Returns a pointer to the crowd.
 *
 * \see RpCrowdCreate
 * \see \ref rpcrowdoverview
 */
RpCrowd *
RpCrowdRender(RpCrowd *crowd)
{
    RWAPIFUNCTION(RWSTRING("RpCrowdRender"));
    RWASSERT(_rpCrowdModule.numInstances);
    RWASSERT(crowd);

    _rpCrowdDeviceRenderCrowd(crowd);

    RWRETURN(crowd);
}

/**
 * \ingroup rpcrowd
 * \ref RpCrowdSetCurrentTexture
 * sets the current crowd texture to one of the set of textures registered
 * with \ref RpCrowdCreate according to the specified index. This may be
 * switched at runtime to model
 * - crowd reactions to events
 * - different clothes worn by supporters of different teams
 *
 * It may be combined with a change in animation using
 * \ref RpCrowdSetCurrentAnim.
 *
 * \param  crowd  Pointer to the crowd.
 * \param  index  Index of the new texture to use.
 *
 * \return Returns a pointer to the crowd.
 *
 * \see RpCrowdGetCurrentTexture
 * \see RpCrowdSetCurrentAnim
 * \see RpCrowdGetCurrentAnim
 * \see RpCrowdGetParam
 * \see RpCrowdCreate
 * \see \ref rpcrowdoverview
 */
RpCrowd *
RpCrowdSetCurrentTexture(RpCrowd *crowd, RwUInt32 index)
{
    RWAPIFUNCTION(RWSTRING("RpCrowdSetCurrentTexture"));
    RWASSERT(_rpCrowdModule.numInstances);
    RWASSERT(crowd);
    RWASSERT(index < crowd->param.numTextures);

    crowd->currTexture = index;

    RWRETURN(crowd);
}

/**
 * \ingroup rpcrowd
 * \ref RpCrowdGetCurrentTexture
 * returns the index of the texture currently being used for displaying
 * a crowd. The texture array is registered with \ref RpCrowdCreate.
 *
 * \param  crowd  Pointer to the crowd.
 *
 * \return The index of the texture in use.
 *
 * \see RpCrowdSetCurrentTexture
 * \see RpCrowdSetCurrentAnim
 * \see RpCrowdGetCurrentAnim
 * \see RpCrowdGetParam
 * \see RpCrowdCreate
 * \see \ref rpcrowdoverview
 */
RwUInt32
RpCrowdGetCurrentTexture(RpCrowd *crowd)
{
    RWAPIFUNCTION(RWSTRING("RpCrowdGetCurrentTexture"));
    RWASSERT(_rpCrowdModule.numInstances);
    RWASSERT(crowd);

    RWRETURN(crowd->currTexture);
}

/**
 * \ingroup rpcrowd
 * \ref RpCrowdSetCurrentAnim
 * sets the crowd animation to one of the set of animations registered during
 * \ref RpCrowdCreate, according to the given index. The animation may
 * be switched at runtime to change crowd behavior, such as reactions
 * to events. This may be combined with a change in texture as set by
 * \ref RpCrowdSetCurrentTexture.
 *
 * \param  crowd    A pointer to the crowd.
 * \param  index    Index of the new animation.
 *
 * \return Returns a pointer to the crowd.
 *
 * \see RpCrowdGetCurrentAnim
 * \see RpCrowdSetCurrentAnimTime
 * \see RpCrowdGetCurrentAnimTime
 * \see RpCrowdSetCurrentTexture
 * \see RpCrowdGetCurrentTexture
 * \see RpCrowdGetParam
 * \see RpCrowdCreate
 * \see \ref rpcrowdoverview
 */
RpCrowd *
RpCrowdSetCurrentAnim(RpCrowd *crowd, RwUInt32 index)
{
    RWAPIFUNCTION(RWSTRING("RpCrowdSetCurrentAnim"));
    RWASSERT(_rpCrowdModule.numInstances);
    RWASSERT(crowd);
    RWASSERT(index < crowd->param.numAnims);

    crowd->currAnim = index;

    RWRETURN(crowd);
}

/**
 * \ingroup rpcrowd
 * \ref RpCrowdGetCurrentAnim
 * returns the index of the \ref RpCrowdAnimation currently in use by
 * an \ref RpCrowd. The array of animations is registered during a call
 * to \ref RpCrowdCreate.
 *
 * \param  crowd    A pointer to the crowd.
 *
 * \return Index of the animation currently in use.
 *
 * \see RpCrowdSetCurrentAnim
 * \see RpCrowdSetCurrentAnimTime
 * \see RpCrowdGetCurrentAnimTime
 * \see RpCrowdSetCurrentTexture
 * \see RpCrowdGetCurrentTexture
 * \see RpCrowdGetParam
 * \see RpCrowdCreate
 * \see \ref rpcrowdoverview
 */
RwUInt32
RpCrowdGetCurrentAnim(RpCrowd *crowd)
{
    RWAPIFUNCTION(RWSTRING("RpCrowdGetCurrentAnim"));
    RWASSERT(_rpCrowdModule.numInstances);
    RWASSERT(crowd);

    RWRETURN(crowd->currAnim);
}

/**
 * \ingroup rpcrowd
 * \ref RpCrowdSetCurrentAnimTime
 * sets the current time of the crowd animation. This should lie between
 * zero and the duration of the \ref RpCrowdAnimation.
 *
 * \param  crowd    A pointer to the crowd.
 * \param  time     Animation time.
 *
 * \return Returns a pointer to the crowd.
 *
 * \see RpCrowdGetCurrentAnimTime
 * \see RpCrowdAddAnimTime
 * \see RpCrowdSetCurrentAnim
 * \see RpCrowdGetCurrentAnim
 * \see RpCrowdCreate
 * \see \ref rpcrowdoverview
 */
RpCrowd *
RpCrowdSetCurrentAnimTime(RpCrowd *crowd, RwReal time)
{
    RWAPIFUNCTION(RWSTRING("RpCrowdSetCurrentAnimTime"));
    RWASSERT(_rpCrowdModule.numInstances);
    RWASSERT(crowd);
    RWASSERT(crowd->currAnim < crowd->param.numAnims);
    RWASSERT(crowd->param.anims[crowd->currAnim]);
    RWASSERT(time >= 0.0f);
    RWASSERT(time <= crowd->param.anims[crowd->currAnim]->duration);

    crowd->currAnimTime = time;

    RWRETURN(crowd);
}

/**
 * \ingroup rpcrowd
 * \ref RpCrowdGetCurrentAnimTime
 * returns the current position within an \ref RpCrowdAnimation
 * for the given \ref RpCrowd. This is updated using \ref RpCrowdAddAnimTime,
 * or set explicitly using \ref RpCrowdSetCurrentAnimTime.
 *
 * \param  crowd    A pointer to the crowd.
 *
 * \return Returns the time within the animation.
 *
 * \see RpCrowdAddAnimTime
 * \see RpCrowdSetCurrentAnimTime
 * \see RpCrowdSetCurrentAnim
 * \see RpCrowdGetCurrentAnim
 * \see \ref rpcrowdoverview
 */
RwReal
RpCrowdGetCurrentAnimTime(RpCrowd *crowd)
{
    RWAPIFUNCTION(RWSTRING("RpCrowdGetCurrentAnimTime"));
    RWASSERT(_rpCrowdModule.numInstances);
    RWASSERT(crowd);

    RWRETURN(crowd->currAnimTime);
}
