/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpskin.h"

#include "skincommon.h"
#include "skin.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/
RpAtomicCallBackRender defRenderCallback = NULL;

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/
#define MATRIXPROCESSX(m, o, w)                                     \
    ( ( (m->right.x * o.x) +                                        \
        (m->up.x    * o.y) +                                        \
        (m->at.x    * o.z) +                                        \
        (m->pos.x) ) * w )

#define MATRIXPROCESSY(m, o, w)                                     \
    ( ( (m->right.y * o.x) +                                        \
        (m->up.y    * o.y) +                                        \
        (m->at.y    * o.z) +                                        \
        (m->pos.y) ) * w )

#define MATRIXPROCESSZ(m, o, w)                                     \
    ( ( (m->right.z * o.x) +                                        \
        (m->up.z    * o.y) +                                        \
        (m->at.z    * o.z) +                                        \
        (m->pos.z) ) * w )

#define MATRIXSKIN(v, m, o, w)                                      \
MACRO_START                                                         \
{                                                                   \
    v.x = MATRIXPROCESSX( m, o, w );                                \
    v.y = MATRIXPROCESSY( m, o, w );                                \
    v.z = MATRIXPROCESSZ( m, o, w );                                \
}                                                                   \
MACRO_STOP

#define MATRIXPLUSSKIN(v, m, o, w)                                  \
MACRO_START                                                         \
{                                                                   \
    v.x += MATRIXPROCESSX( m, o, w );                               \
    v.y += MATRIXPROCESSY( m, o, w );                               \
    v.z += MATRIXPROCESSZ( m, o, w );                               \
}                                                                   \
MACRO_STOP

#define MATRIXNORMALPROCESSX(m, o, w)                               \
    ( ( (m->right.x * o.x) +                                        \
        (m->up.x    * o.y) +                                        \
        (m->at.x    * o.z) ) * w )

#define MATRIXNORMALPROCESSY(m, o, w)                               \
    ( ( (m->right.y * o.x) +                                        \
        (m->up.y    * o.y) +                                        \
        (m->at.y    * o.z) ) * w )

#define MATRIXNORMALPROCESSZ(m, o, w)                               \
    ( ( (m->right.z * o.x) +                                        \
        (m->up.z    * o.y) +                                        \
        (m->at.z    * o.z) ) * w )

#define MATRIXNORMALSKIN(n, m, o, w)                                \
MACRO_START                                                         \
{                                                                   \
    n.x = MATRIXNORMALPROCESSX( m, o, w );                          \
    n.y = MATRIXNORMALPROCESSY( m, o, w );                          \
    n.z = MATRIXNORMALPROCESSZ( m, o, w );                          \
}                                                                   \
MACRO_STOP

#define MATRIXNORMALPLUSSKIN(n, m, o, w)                            \
MACRO_START                                                         \
{                                                                   \
    n.x += MATRIXNORMALPROCESSX( m, o, w );                         \
    n.y += MATRIXNORMALPROCESSY( m, o, w );                         \
    n.z += MATRIXNORMALPROCESSZ( m, o, w );                         \
}                                                                   \
MACRO_STOP

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/


/****************************************************************************
 - SkinMatBlendAtomicRender

 - Matrix blending Atomic render function - performs weighted transform of
   an atomic's vertices according to the attached RpSkin data.

   THIS IS A VERY SLOW, GENERIC "C" IMPLEMENTATION AND SHOULD BE OVERLOADED
   WITH SOMETHING FAST, SEXY AND PLATFORM-SPECIFIC.

 - Inputs :   RpAtomic *    A pointer to the atomic.
 - Outputs:   RwBool        TRUE on success
 */
RpAtomic *
SkinMatBlendAtomicRender(RpAtomic *atomic)
{
    RpGeometry *geometry;
    RpSkin *skin;

    RWFUNCTION(RWSTRING("SkinMatBlendAtomicRender"));
    RWASSERT(NULL != atomic);
    geometry = RpAtomicGetGeometry(atomic);
    RWASSERT(NULL != geometry);
    skin = *RPSKINGEOMETRYGETDATA(geometry);

    if(NULL != skin)
    {
        RpMorphTarget *morphTarget;
        RpHAnimHierarchy *hierarchy;

        RwV3d *vertices;
        RwV3d *normals;

        RwV3d *originalVertices;
        RwV3d *originalNormals;

        RwMatrix *matrixArray = NULL;

        RwInt32 i;

        const RwMatrix *skinToBone;

        /* Get the bone information. */
        skinToBone = RpSkinGetSkinToBoneMatrices(skin);
        RWASSERT(NULL != skinToBone);

        /* Perform matrix blending */
        RpGeometryLock( geometry,
                        rpGEOMETRYLOCKVERTICES | rpGEOMETRYLOCKNORMALS);

        morphTarget = RpGeometryGetMorphTarget(geometry, 0);
        RWASSERT(NULL != morphTarget);

        vertices = RpMorphTargetGetVertices(morphTarget);
        normals = RpMorphTargetGetVertexNormals(morphTarget);

        originalVertices = skin->platformData.vertices;
        originalNormals = skin->platformData.normals;

        hierarchy = RPSKINATOMICGETDATA(atomic)->hierarchy;

        if(NULL != hierarchy)
        {
            matrixArray = _rpSkinGlobals.matrixCache.aligned;

            if(hierarchy->flags & rpHANIMHIERARCHYNOMATRICES)
            {
                RwFrame *frame;
                RwMatrix *ltm;

                RwMatrix inverseAtomicLTM;
                RwMatrix temmatrix;
                RwInt32 i;

                frame = RpAtomicGetFrame(atomic);
                RWASSERT(NULL != frame);
                ltm = RwFrameGetLTM(frame);
                RWASSERT(NULL != ltm);

                RwMatrixInvert(&inverseAtomicLTM, ltm);

                for( i = 0; i < hierarchy->numNodes; ++i )
                {
                    RwFrame *node;
                    RwMatrix *ltm;

                    node = hierarchy->pNodeInfo[i].pFrame;
                    RWASSERT(NULL != node);
                    ltm = RwFrameGetLTM(node);
                    RWASSERT(NULL != ltm);

                    RwMatrixMultiply( &temmatrix,
                                      &skinToBone[i],
                                      ltm );
                    RwMatrixMultiply( &matrixArray[i],
                                      &temmatrix,
                                      &inverseAtomicLTM );
                }
            }
            else if(hierarchy->flags & rpHANIMHIERARCHYLOCALSPACEMATRICES)
            {
                for( i = 0; i < hierarchy->numNodes; ++i )
                {
                    RwMatrixMultiply( &matrixArray[i],
                                      &skinToBone[i],
                                      &(hierarchy->pMatrixArray[i]) );
                }
            }
            else
            {
                RwMatrix inverseAtomicLTM;
                RwMatrix temmatrix;

                RwMatrixInvert(&inverseAtomicLTM,
                               RwFrameGetLTM(RpAtomicGetFrame
                                             (atomic)));

                for( i = 0; i < hierarchy->numNodes; ++i)
                {
                    RwMatrixMultiply( &temmatrix,
                                      &skinToBone[i],
                                      &(hierarchy->pMatrixArray[i]) );
                    RwMatrixMultiply( &matrixArray[i],
                                      &temmatrix,
                                      &inverseAtomicLTM );
                }
            }
        }

        for( i = 0; i < RpGeometryGetNumVertices(geometry); ++i )
        {
            RwMatrix *matrix;
            RwUInt32 matrixIndices;
            RwMatrixWeights matrixWeights;

            matrixIndices = skin->vertexMaps.matrixIndices[i];
            matrixWeights = skin->vertexMaps.matrixWeights[i];

            /* Hideously slow matrix operations follow... */
            if(matrixWeights.w0 > (RwReal) 0.0)
            {
                matrix = &matrixArray[matrixIndices & 0xFF];

                MATRIXSKIN( vertices[i],
                            matrix,
                            originalVertices[i],
                            matrixWeights.w0 );

                if(NULL != normals)
                {
                    MATRIXNORMALSKIN( normals[i],
                                      matrix,
                                      originalNormals[i],
                                      matrixWeights.w0 );
                }
            }
            else
            {
                continue;
            }

            if(matrixWeights.w1 > (RwReal) 0.0)
            {
                matrix = &matrixArray[(matrixIndices >> 8) & 0xFF];

                MATRIXPLUSSKIN( vertices[i],
                                matrix,
                                originalVertices[i],
                                matrixWeights.w1 );

                if(NULL != normals)
                {
                    MATRIXNORMALPLUSSKIN( normals[i],
                                          matrix,
                                          originalNormals[i],
                                          matrixWeights.w1 );
                }
            }
            else
            {
                continue;
            }

            if(matrixWeights.w2 > (RwReal) 0.0)
            {
                matrix = &matrixArray[(matrixIndices >> 16) & 0xFF];

                MATRIXPLUSSKIN( vertices[i],
                                matrix,
                                originalVertices[i],
                                matrixWeights.w2 );

                if(NULL != normals)
                {
                    MATRIXNORMALPLUSSKIN( normals[i],
                                          matrix,
                                          originalNormals[i],
                                          matrixWeights.w2 );
                }
            }
            else
            {
                continue;
            }

            if(matrixWeights.w3 > (RwReal) 0.0)
            {
                matrix = &matrixArray[(matrixIndices >> 24) & 0xFF];

                MATRIXPLUSSKIN( vertices[i],
                                matrix,
                                originalVertices[i],
                                matrixWeights.w3 );

                if(NULL != normals)
                {
                    MATRIXNORMALPLUSSKIN( normals[i],
                                          matrix,
                                          originalNormals[i],
                                          matrixWeights.w3 );
                }
            }
            else
            {
                continue;
            }
        }

        RpGeometryUnlock(geometry);

        (*defRenderCallback)(atomic);
    }

    RWRETURN(atomic);
}

/****************************************************************************
 _rpSkinInitialize

 Initialise an atomic's matrix-blending skin data.

 Inputs :   RpGeometry * - Pointer to a skinned geometry.
 Outputs:   RpGeometry * - Pointer to the skinned geometry on success.
 */
RpGeometry *
_rpSkinInitialize(RpGeometry *geometry)
{
    RpSkin *skin;

    RWFUNCTION(RWSTRING("_rpSkinInitialize"));
    RWASSERT(NULL != geometry);

    skin = *RPSKINGEOMETRYGETDATA(geometry);

    if(skin != NULL)
    {
        RpMorphTarget *morphTarget;
        RwUInt32 numVertices;

        RwV3d *originalVertices;
        RwV3d *originalNormals;
        RwV3d *srcVertices;
        RwV3d *srcNormals;
        RwUInt32 i;

        RpGeometryLock( geometry,
                        rpGEOMETRYLOCKVERTICES | rpGEOMETRYLOCKNORMALS );

        numVertices = RpGeometryGetNumVertices(geometry);

        /* Add a copy of the original vertices and normals to
         * the platform-specific skin data pointer */
        morphTarget = RpGeometryGetMorphTarget(geometry, 0);
        RWASSERT(NULL != morphTarget);

        srcVertices = RpMorphTargetGetVertices(morphTarget);
        srcNormals = RpMorphTargetGetVertexNormals(morphTarget);

        if(NULL != srcNormals)
        {
            RwUInt32 bytes;

            bytes = sizeof(RwV3d) * numVertices * 2;
            originalVertices = (RwV3d *)RwMalloc(bytes,
                rwID_SKINPLUGIN | rwMEMHINTDUR_EVENT);
            RWASSERT(NULL != originalVertices);

            memset(originalVertices, 0, bytes);

            originalNormals = &(originalVertices[numVertices]);
        }
        else
        {
            RwUInt32 bytes;

            bytes = sizeof(RwV3d) * numVertices;
            originalVertices = (RwV3d *)RwMalloc(bytes,
                rwID_SKINPLUGIN | rwMEMHINTDUR_EVENT);
            RWASSERT(NULL != originalVertices);

            memset(originalVertices, 0, bytes);

            originalNormals = (RwV3d *)NULL;
        }

        skin->platformData.vertices = originalVertices;
        skin->platformData.normals = originalNormals;

        for( i = 0; i < numVertices; ++i )
        {
            originalVertices[i] = srcVertices[i];
        }

        if(NULL != originalNormals)
        {
            for( i = 0; i < numVertices; ++i )
            {
                originalNormals[i] = srcNormals[i];
            }
        }

        RpGeometryUnlock(geometry);
    }

    RWRETURN(geometry);
}


/****************************************************************************
 _rpSkinDeinitialize
 Platform specific deinitialize function for skinned geometry's.
 Inputs :  *geometry    - Pointer to the skinned geometry.
 Outputs:  RpGeometry * - The geometry which has been deinitialized.
 */
RpGeometry *
_rpSkinDeinitialize(RpGeometry *geometry)
{
    RpSkin *skin;

    RWFUNCTION(RWSTRING("_rpSkinDeinitialize"));
    RWASSERT(NULL != geometry);

    skin = *RPSKINGEOMETRYGETDATA(geometry);

    if(NULL != skin)
    {
        if(NULL != skin->platformData.vertices)
        {
            RwFree(skin->platformData.vertices);
        }

        skin->platformData.vertices = (RwV3d *)NULL;
        skin->platformData.normals = (RwV3d *)NULL;
    }

    RWRETURN(geometry);
}

/*===========================================================================*
 *--- Plugin Native Serialization Functions ---------------------------------*
 *===========================================================================*/

/****************************************************************************
 _rpSkinGeometryNativeSize
 */
RwInt32
_rpSkinGeometryNativeSize(const RpGeometry *geometry)
{
    RWFUNCTION(RWSTRING("_rpSkinGeometryNativeSize"));
    RWASSERT(geometry);

    RWRETURN(0);
}

/****************************************************************************
 _rpSkinGeometryNativeWrite
 */
RwStream *
_rpSkinGeometryNativeWrite(RwStream *stream, const RpGeometry *geometry)
{
    RWFUNCTION(RWSTRING("_rpSkinGeometryNativeWrite"));
    RWASSERT(stream);
    RWASSERT(geometry);

    RWRETURN(stream);
}

/****************************************************************************
 _rpSkinGeometryNativeRead
 */
RwStream *
_rpSkinGeometryNativeRead(RwStream *stream, RpGeometry *geometry)
{
    RWFUNCTION(RWSTRING("_rpSkinGeometryNativeRead"));
    RWASSERT(stream);
    RWASSERT(geometry);

    RWRETURN(stream);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

