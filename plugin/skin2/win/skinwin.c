/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpskin.h"

#include "skin.h"

#include "x86matbl.h"


/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/
static RpAtomicCallBackRender defRenderCallback = NULL;

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
SkinGlobals _rpSkinGlobals =
{
    0,
    0,
    0,
    { (RwMatrix *)NULL, NULL },
    0,
    (RwFreeList *)NULL,
    { 0, 0 },
    {                                 /* SkinGlobalPlatform  platform    */
        0
    },
    (SkinSplitData *) NULL
};

/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/

/****************************************************************************
 - SkinMatBlendAtomicRender

 - Matrix blending Atomic render function - performs weighted transform of
   an atomic's vertices according to the attached RpSkin data.

 - If NOASM=0 then an Intel x86 assembler function is used for matrix blending.
   Otherwise...

   THIS IS A VERY SLOW, GENERIC "C" IMPLEMENTATION AND SHOULD BE OVERLOADED
   WITH SOMETHING FAST, SEXY AND PLATFORM-SPECIFIC.

 - Inputs :   RpAtomic *    A pointer to the atomic.
 - Outputs:   RwBool        TRUE on success
 */
static RpAtomic *
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

        matrixArray = _rpSkinPrepareAtomicMatrix(atomic, skin, hierarchy);

                /* Vertex blending */
#if !defined( NOASM )

        _rpSkinIntelx86MatrixBlend( RpGeometryGetNumVertices( geometry ),
                                    skin->vertexMaps.matrixWeights,
                                    skin->vertexMaps.matrixIndices,
                                    matrixArray,
                                    (RwV3d *)vertices,
                                    originalVertices,
                                    (RwV3d *)normals,
                                    originalNormals,
                                    sizeof(RwV3d) );

#else /* !defined( NOASM ) */

        _rpSkinGenericMatrixBlend( RpGeometryGetNumVertices( geometry ),
                                   skin->vertexMaps.matrixWeights,
                                   skin->vertexMaps.matrixIndices,
                                   matrixArray,
                                   (RwUInt8 *)vertices,
                                   originalVertices,
                                   (RwUInt8 *)normals,
                                   originalNormals,
                                   sizeof(RwV3d) );

#endif /* !defined( NOASM ) */

        RpGeometryUnlock(geometry);

        (*defRenderCallback)(atomic);
    }

    RWRETURN(atomic);
}


/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

/****************************************************************************
 _rpSkinPipelinesCreate
 Create the skinning pipelines.
 Inputs :   None
 Outputs:   RwBool - on success.
 */
RwBool
_rpSkinPipelinesCreate(RwUInt32 pipes __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("_rpSkinPipelinesCreate"));

#if !defined( NOASM ) && defined( RWDEBUG )

    _rpSkinIntelx86ConfirmConstants();

#endif /* !defined( NOASM ) && defined( RWDEBUG ) */

    RWRETURN(TRUE);
}


/****************************************************************************
 _rpSkinPipelinesDestroy
 Destroy the skinning pipelines.
 Inputs :   None
 Outputs:   RwBool - on success.
 */
RwBool
_rpSkinPipelinesDestroy(void)
{
    RWFUNCTION(RWSTRING("_rpSkinPipelinesDestroy"));

    RWRETURN(TRUE);
}


/****************************************************************************
 _rpSkinPipelinesAttach
 Attach the generic skinning pipeline to an atomic.
 Inputs :   atomic *   - Pointer to the atomic.
 Outputs:   RpAtomic * - Pointer to the atomic on success.
 */
RpAtomic *
_rpSkinPipelinesAttach(RpAtomic *atomic)
{
    RWFUNCTION(RWSTRING("_rpSkinPipelinesAttach"));

    /*
     * Obviously these don't chain, as we should probably store each atomic's
     * atomic render callback.
     */
    if(defRenderCallback == NULL)
    {
        defRenderCallback = RpAtomicGetRenderCallBack(atomic);
    }

    RpAtomicSetRenderCallBack(atomic, SkinMatBlendAtomicRender);

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

        for( i = 0; i < numVertices; i++ )
        {
            originalVertices[i] = srcVertices[i];
        }

        if(NULL != originalNormals)
        {
            for( i = 0; i < numVertices; i++ )
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
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

