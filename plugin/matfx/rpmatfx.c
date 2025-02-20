/**
 * \ingroup rpmatfx
 * \page rpmatfxoverview RpMatFX Plugin Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rpmatfx.h
 * \li \b Libraries: rwcore, rpworld, rpmatfx
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpMatFXPluginAttach
 *
 * \subsection matfxoverview Overview
 * This plugin provides facilities for material effects. Material effects are
 * off the shelf effects such as:
 * \li Environment mapping,
 * \li Bump mapping,
 * \li Dual pass,
 * \li Texture coordinate transformation (changing the transform matrix each
 *      frame allows texture coordinate animation),
 * \li Dual pass texture coordination transformation (a different
 *      transformation may be applied to each pass).
 *
 * Before any of the plugin functions are used, the plugin
 * should be attached using \ref RpMatFXPluginAttach.
 *
 * \if sky2 \note The RpMatFX plugin is designed to be used with only textured
 * materials. \endif
 *
 * \if d3d8 \note The RpMatFX plugin is designed to be used with only textured
 * materials. \endif
 *
 * \if d3d9 \note The RpMatFX plugin is designed to be used with only textured
 * materials. \endif
 *
 * \if gcn \note The RpMatFX plugin is designed to be used with only textured
 * materials. \endif
 *
 * \if opengl \note The RpMatFX plugin is designed to be used with only textured
 * materials. \endif
 *
 * \par Material Effect Atomics
 *
 * The material effects pipeline must be initialized with
 * \ref RpMatFXAtomicEnableEffects for each atomic that contains material
 * effects.
 *
 * \par Material Effect World Sectors
 *
 * The material effects pipeline must be initialized with
 * \ref RpMatFXWorldSectorEnableEffects for each world sector that contains
 * material effects.
 *
 * \par Material Effects
 *
 * Material effects need to be setup for each material. Before setting any
 * of the effects individual attributes the material should be initialized
 * for the specific material effect with \ref RpMatFXMaterialSetEffects.
 *
 * Supported material effect types are:
 *
 * \li \ref rpMATFXEFFECTNULL            No material effect.
 * \li \ref rpMATFXEFFECTBUMPMAP         Bump mapping effect.
 * \li \ref rpMATFXEFFECTENVMAP          Environment mapping effect.
 * \li \ref rpMATFXEFFECTBUMPENVMAP      Bump and environment mapping effect.
 * \li \ref rpMATFXEFFECTDUAL            Dual pass rendering.
 * \li \ref rpMATFXEFFECTUVTRANSFORM     Single pass texture coordination
 *                                          transformation.
 * \li \ref rpMATFXEFFECTDUALUVTRANSFORM Dual texture coordination
 *                                          transformation (two passes).
 *
 * The materials effect can be queried with \ref RpMatFXMaterialGetEffects.
 *
 * Once the material has been setup with a specific effect the attributes
 * should be initialized with the following functions:
 *
 * \li \ref RpMatFXMaterialSetupBumpMap
 * \li \ref RpMatFXMaterialSetupEnvMap
 * \li \ref RpMatFXMaterialSetupDualTexture
 *
 * \par Bump Mapping
 *
 * These functions are available:
 *
 * \li \ref RpMatFXMaterialSetBumpMapTexture.
 * \li \ref RpMatFXMaterialSetBumpMapFrame.
 * \li \ref RpMatFXMaterialSetBumpMapCoefficient.
 * \li \ref RpMatFXMaterialGetBumpMapTexture.
 * \li \ref RpMatFXMaterialGetBumpMapBumpedTexture.
 * \li \ref RpMatFXMaterialGetBumpMapFrame.
 * \li \ref RpMatFXMaterialGetBumpMapCoefficient.
 *
 * \par Environment Mapping
 *
 * These functions are available:
 *
 * \li \ref RpMatFXMaterialSetEnvMapTexture.
 * \li \ref RpMatFXMaterialSetEnvMapFrame.
 * \li \ref RpMatFXMaterialSetEnvMapFrameBufferAlpha.
 * \li \ref RpMatFXMaterialSetEnvMapCoefficient.
 * \li \ref RpMatFXMaterialGetEnvMapTexture.
 * \li \ref RpMatFXMaterialGetEnvMapFrame.
 * \li \ref RpMatFXMaterialGetEnvMapFrameBufferAlpha.
 * \li \ref RpMatFXMaterialGetEnvMapCoefficient.
 *
 * \par Dual Pass
 *
 * These functions are available:
 *
 * \li \ref RpMatFXMaterialSetDualTexture.
 * \li \ref RpMatFXMaterialSetDualBlendModes.
 * \li \ref RpMatFXMaterialGetDualTexture.
 * \li \ref RpMatFXMaterialGetDualBlendModes.
 *
 * \par Texture Coordinate UV transformation:
 *
 * These functions are available:
 *
 * \li \ref RpMatFXMaterialSetUVTransformMatrices
 * \li \ref RpMatFXMaterialGetUVTransformMatrices
 */

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <string.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "matfx.h"
#include "effectPipes.h"
#include "multiTex.h"

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/
typedef struct MatFXWorldSectorData MatFXWorldSectorData;
struct MatFXWorldSectorData
{
        RwBool              enabled;
};

typedef struct MatFXAtomicData MatFXAtomicData;
struct MatFXAtomicData
{
        RwBool              enabled;
};


/*===========================================================================*
 *--- Public Types ----------------------------------------------------------*
 *===========================================================================*/
/*===========================================================================*
 *--- Public Global Variables -----------------------------------------------*
 *===========================================================================*/
#if (defined(RWDEBUG))
long                rpMatFXStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

RwInt32             MatFXMaterialDataOffset = 0;

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/

RwMatFXInfo MatFXInfo = { { 0, 0 },
                          ( RwFreeList *) NULL };

static RwInt32      MatFXAtomicDataOffset = 0;
static RwInt32      MatFXWorldSectorDataOffset = 0;

/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/
#define MATFXBUMPTEXTURENAMEEXTENSION "_B"

/* Default platform specific setup: do nothing. */
#ifndef _rpMatFXMaterialPlatformInitialize
#define _rpMatFXMaterialPlatformInitialize(material)
#endif
#ifndef _rpMatFXMaterialPlatformEnvMapSetBlend
#define _rpMatFXMaterialPlatformEnvMapSetBlend(material)
#endif
#ifndef _rpMatFXMaterialPlatformDualSetBlend
#define _rpMatFXMaterialPlatformDualSetBlend(material)
#endif

/*---------------------------------------------------------------------------*
 *    Atomic, WorldSector, get macros                                        *
 *---------------------------------------------------------------------------*/
#define MATFXATOMICGETDATA(atomic)                      \
    ((MatFXAtomicData *)(((RwUInt8 *)atomic)+           \
                         MatFXAtomicDataOffset))

#define MATFXATOMICGETCONSTDATA(atomic)                         \
    ((const MatFXAtomicData *)(((const RwUInt8 *)atomic)+       \
                               MatFXAtomicDataOffset))

#define MATFXWORLDSECTORGETDATA(worldSector)                    \
    ((MatFXWorldSectorData *)(((RwUInt8 *)worldSector)+         \
                              MatFXWorldSectorDataOffset))

#define MATFXWORLDSECTORGETCONSTDATA(worldSector)                       \
    ((const MatFXWorldSectorData *)(((const RwUInt8 *)worldSector)+     \
                                    MatFXWorldSectorDataOffset))

/*---------------------------------------------------------------------------*
 *    BumpMap, EnvMap, Dual, get macros                                      *
 *---------------------------------------------------------------------------*/
#define MATFXBUMPMAPGETDATA(material)                                       \
    ((MatFXBumpMapData *)                                                   \
        MatFXGetData(material, rpMATFXEFFECTBUMPMAP))

#define MATFXBUMPMAPGETCONSTDATA(material)                                  \
    ((const MatFXBumpMapData *)                                             \
        MatFXGetConstData(material, rpMATFXEFFECTBUMPMAP))

#define MATFXENVMAPGETDATA(material)                                        \
    ((MatFXEnvMapData *)                                                    \
        MatFXGetData(material, rpMATFXEFFECTENVMAP))

#define MATFXENVMAPGETCONSTDATA(material)                                   \
    ((const MatFXEnvMapData *)                                              \
        MatFXGetConstData(material, rpMATFXEFFECTENVMAP))

#define MATFXDUALGETDATA(material)                                          \
    ((MatFXDualData *)                                                      \
        MatFXGetData(material, rpMATFXEFFECTDUAL))

#define MATFXDUALGETCONSTDATA(material)                                     \
    ((const MatFXDualData *)                                                \
        MatFXGetConstData(material, rpMATFXEFFECTDUAL))

#define MATFXUVANIMGETDATA(material)                                        \
    ((MatFXUVAnimData *)                                                    \
        MatFXGetData(material, rpMATFXEFFECTUVTRANSFORM))

#define MATFXUVANIMGETCONSTDATA(material)                                   \
    ((const MatFXUVAnimData *)                                              \
        MatFXGetConstData(material, rpMATFXEFFECTUVTRANSFORM))

#define rpMATFXENTRIESPERBLOCK 128

/*===========================================================================*
 *--- External functions ----------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*---------------------------------------------------------------------------*
 *    Material Effects: get effect data                                      *
 *---------------------------------------------------------------------------*/
static MatFXEffectUnion *
MatFXGetData(RpMaterial * material, RpMatFXMaterialFlags flags)
{
    rpMatFXMaterialData *materialData;

    RwUInt8             pass;

    RWFUNCTION(RWSTRING("MatFXGetData"));
    RWASSERT(material);

    materialData = (rpMatFXMaterialData *)
        *MATFXMATERIALGETDATA(material);

    RWASSERT(materialData);
    /* RWASSERT(RPMATFXALIGNMENT(materialData)); */

    for (pass = 0; pass < rpMAXPASS; ++pass)
    {
        if (materialData->data[pass].flag == flags)
        {
            RWRETURN(&(materialData->data[pass].data));
        }
    }

    /* We should probably never get here. */
    RWASSERT(FALSE);
    RWRETURN((MatFXEffectUnion *)NULL);
}

static const MatFXEffectUnion *
MatFXGetConstData(const RpMaterial * material,
                  RpMatFXMaterialFlags flags)
{
    const rpMatFXMaterialData *materialData;

    RwUInt8             pass;

    RWFUNCTION(RWSTRING("MatFXGetConstData"));
    RWASSERT(material);

    materialData = (const rpMatFXMaterialData *)
        *MATFXMATERIALGETCONSTDATA(material);
    RWASSERT(materialData);
    /* RWASSERT(RPMATFXALIGNMENT(materialData)); */

    for (pass = 0; pass < rpMAXPASS; ++pass)
    {
        if (materialData->data[pass].flag == flags)
        {
            RWRETURN(&(materialData->data[pass].data));
        }
    }

    /* We should probably never get here. */
    RWASSERT(FALSE);
    RWRETURN((MatFXEffectUnion *)NULL);
}

/*---------------------------------------------------------------------------*
 *    Material Effects Plugins: open, close                                  *
 *---------------------------------------------------------------------------*/
static void        *
MatFXClose(void *instance,
           RwInt32 offset __RWUNUSED__,
           RwInt32 size __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("MatFXClose"));
    RWASSERT(instance);

    /* one less module instance */
    --MatFXInfo.Module.numInstances;

    if (MatFXInfo.Module.numInstances == 0)
    {
        /* Destroy the MatFX pipeline. */
        _rpMatFXPipelinesDestroy();
    }

    if (MatFXInfo.MaterialData)
    {
        RwFreeListDestroy(MatFXInfo.MaterialData);
        MatFXInfo.MaterialData = (RwFreeList *)NULL;
    }

    RWRETURN(instance);
}


static RwInt32 _rpMatFXMaterialDataFreeListBlockSize = rpMATFXENTRIESPERBLOCK,
               _rpMatFXMaterialDataFreeListPreallocBlocks = 1;
static RwFreeList _rpMatFXMaterialDataFreeList;

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialDataSetFreeListCreateParams allows the developer to specify
 * how many rpMatFXMaterialData s to preallocate space for.
 * Call before \ref RwEngineInit.
 *
 * \param blockSize  number of entries per freelist block.
 * \param numBlocksToPrealloc  number of blocks to allocate on
 * \ref RwFreeListCreateAndPreallocateSpace.
 *
 * \see RwFreeList
 *
 */
void
RpMatFXMaterialDataSetFreeListCreateParams(RwInt32 blockSize,
                                           RwInt32 numBlocksToPrealloc)
{
#if 0
    /*
     * Can not use debugging macros since the debugger is not initialized before
     * RwEngineInit.
     */
    /* Do NOT comment out RWAPIFUNCTION as gnumake verify will not function */
    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialDataSetFreeListCreateParams"));
#endif
    _rpMatFXMaterialDataFreeListBlockSize = blockSize;
    _rpMatFXMaterialDataFreeListPreallocBlocks = numBlocksToPrealloc;
#if 0
    RWRETURNVOID();
#endif
}

static void        *
MatFXOpen(void *instance,
          RwInt32 offset __RWUNUSED__,
          RwInt32 size __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("MatFXOpen"));
    RWASSERT(instance);

    /* Create the MatFX pipeline, only once. */
    if (MatFXInfo.Module.numInstances == 0)
    {
        MatFXInfo.MaterialData =
            RwFreeListCreateAndPreallocateSpace(sizeof(rpMatFXMaterialData),
                             _rpMatFXMaterialDataFreeListBlockSize,
                             rpMATFXALIGNMENT,
                             _rpMatFXMaterialDataFreeListPreallocBlocks,
                             &_rpMatFXMaterialDataFreeList,
                             rwID_MATERIALEFFECTSPLUGIN | rwMEMHINTDUR_GLOBAL);

        if (MatFXInfo.MaterialData == NULL)
        {
            instance = NULL;
            RWRETURN(instance);
        }

        /* Set up the pipeline. */
        if (FALSE == _rpMatFXPipelinesCreate())
        {
            instance = NULL;
            RWRETURN(instance);
        }

    }

    ++MatFXInfo.Module.numInstances;

    RWRETURN(instance);
}

/*---------------------------------------------------------------------------*
 *    Material: constructor, destructor, copy, get data,                     *
 *              stream read, stream write, stream size.                      *
 *---------------------------------------------------------------------------*/
static void        *
MatFXMaterialConstructor(void *object,
                         RwInt32 offset __RWUNUSED__,
                         RwInt32 size __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("MatFXMaterialConstructor"));
    RWASSERT(object);

    *MATFXMATERIALGETDATA(object) = (rpMatFXMaterialData *)
        NULL;

    RWRETURN(object);
}

static rpMatFXMaterialData *
MatFXMaterialDataClean(rpMatFXMaterialData * materialData)
{
    RwUInt8             pass;

    RWFUNCTION(RWSTRING("MatFXMaterialDataClean"));
    RWASSERT(materialData);

    for (pass = 0; pass < rpMAXPASS; ++pass)
    {
        switch (materialData->data[pass].flag)
        {
            case rpMATFXEFFECTNULL:
                {
                    /* This pass is not in use, so no data to free. */
                    break;
                }
            case rpMATFXEFFECTBUMPMAP:
                {
                    MatFXBumpMapData   *bumpMapData;

                    bumpMapData =
                        &(materialData->data[pass].data.bumpMap);

                    if (NULL != bumpMapData->texture)
                    {
                        RwTextureDestroy(bumpMapData->texture);
                    }

                    if (NULL != bumpMapData->bumpTexture)
                    {
                        RwTextureDestroy(bumpMapData->bumpTexture);
                    }

                    break;
                }
            case rpMATFXEFFECTENVMAP:
                {
                    MatFXEnvMapData    *envMapData;

                    envMapData =
                        &(materialData->data[pass].data.envMap);

                    if (NULL != envMapData->texture)
                    {
                        RwTextureDestroy(envMapData->texture);
                    }

                    break;
                }
            case rpMATFXEFFECTDUAL:
                {
                    MatFXDualData      *dualData;

                    dualData = &(materialData->data[pass].data.dual);

                    if (NULL != dualData->texture)
                    {
                        RwTextureDestroy(dualData->texture);
                    }

                    break;
                }
            case rpMATFXEFFECTUVTRANSFORM:
                {
                    MatFXUVAnimData *uvAnimData;

                    uvAnimData = &(materialData->data[pass].data.uvAnim);

                    uvAnimData->baseTransform = (RwMatrix *)NULL;
                    uvAnimData->dualTransform = (RwMatrix *)NULL;

                    break;
                }
            default:
                {
                    /* We shouldn't get here. */
                    RWASSERT(FALSE);
                    break;
                }
        }
    }

    /* Now clean the entire data area. */
    memset(materialData, 0, sizeof(rpMatFXMaterialData));

    RWRETURN(materialData);
}

static void        *
MatFXMaterialDestructor(void *object,
                        RwInt32 offset __RWUNUSED__,
                        RwInt32 size __RWUNUSED__)
{
    rpMatFXMaterialData *materialData;

    RWFUNCTION(RWSTRING("MatFXMaterialDestructor"));
    RWASSERT(object);

    materialData = (rpMatFXMaterialData *)
        *MATFXMATERIALGETDATA(object);
    /* RWASSERT(RPMATFXALIGNMENT(materialData)); */

    if (materialData)
    {
        MatFXMaterialDataClean(materialData);

        RWASSERT(RPMATFXALIGNMENT(materialData));

        RwFreeListFree(MatFXInfo.MaterialData,
                       materialData);

        *MATFXMATERIALGETDATA(object) = (rpMatFXMaterialData *)
            NULL;
    }

    RWRETURN(object);
}

static rpMatFXMaterialData *
MatFXMaterialGetData(RpMaterial * material)
{
    rpMatFXMaterialData *materialData;

    RWFUNCTION(RWSTRING("MatFXMaterialGetData"));
    RWASSERT(material);

    materialData = (rpMatFXMaterialData *)
        *MATFXMATERIALGETDATA(material);

    /* RWASSERT(RPMATFXALIGNMENT(materialData)); */

    if (NULL == materialData)
    {

        materialData = (rpMatFXMaterialData *)
            RwFreeListAlloc(MatFXInfo.MaterialData,
                rwID_MATERIALEFFECTSPLUGIN | rwMEMHINTDUR_EVENT);

        RWASSERT(RPMATFXALIGNMENT(materialData));

        if (NULL == materialData)
        {
            RWRETURN((rpMatFXMaterialData *) NULL);
        }

        /* Initalise data to zero. */
        memset(materialData, 0, sizeof(*materialData));

        /* RWASSERT(RPMATFXALIGNMENT(materialData)); */

        *MATFXMATERIALGETDATA(material) = (rpMatFXMaterialData *)
            materialData;

        /*
         * We do not setup any default values at this point.
         * This is left for RpMatFXMaterialSetEffects.
         */
    }

    RWRETURN(materialData);
}

static void        *
MatFXMaterialCopy(void *dstObject,
                  const void *srcObject,
                  RwInt32 offset __RWUNUSED__,
                  RwInt32 size __RWUNUSED__)
{
    const RpMaterial   *srcMaterial;
    const rpMatFXMaterialData *srcMaterialData;

    RpMaterial         *dstMaterial;
    rpMatFXMaterialData *dstMaterialData;

    RpMaterial         *ret;

    RwUInt8             pass;

    RWFUNCTION(RWSTRING("MatFXMaterialCopy"));
    RWASSERT(dstObject);
    RWASSERT(srcObject);

    srcMaterial = (const RpMaterial *) srcObject;
    dstMaterial = (RpMaterial *) dstObject;

    srcMaterialData = (const rpMatFXMaterialData *)
        *MATFXMATERIALGETCONSTDATA(srcMaterial);
    /* RWASSERT(RPMATFXALIGNMENT(srcMaterialData)); */

    /*
     * Don't assume there is matfx data on the material just because the
     * plugin is attached...
     */
    if( (rpMatFXMaterialData *)NULL == srcMaterialData )
    {
        RWRETURN(NULL);
    }

    dstMaterialData = MatFXMaterialGetData(dstMaterial);
    RWASSERT(dstMaterialData);
    /* RWASSERT(RPMATFXALIGNMENT(dstMaterialData)); */

    if (NULL == dstMaterialData)
    {
        RWRETURN(NULL);
    }

    /* Lets first initalize the new material. */
    ret = RpMatFXMaterialSetEffects(dstMaterial, srcMaterialData->flags);
    RWASSERT(ret);

    /* Then the effects. */
    for (pass = 0; pass < rpMAXPASS; ++pass)
    {
        RpMatFXMaterialFlags effect;

        /* Lets grab the effect. */
        effect = srcMaterialData->data[pass].flag;

        /* And switch on it. */
        switch (effect)
        {
            case rpMATFXEFFECTNULL:
                {
                    /* Empty pass. */
                    break;
                }
            case rpMATFXEFFECTBUMPMAP:
                {
                    /* BumpMap effect. */
                    const MatFXBumpMapData *srcBumpMapData;
                    MatFXBumpMapData *dstBumpMapData;

                    RwFrame *frame;
                    RwReal coef;

                    srcBumpMapData = &(srcMaterialData->data[pass].data.bumpMap);
                    dstBumpMapData = &(dstMaterialData->data[pass].data.bumpMap);

                    frame = RpMatFXMaterialGetBumpMapFrame(srcMaterial);
                    coef  = RpMatFXMaterialGetBumpMapCoefficient(srcMaterial);

                    ret = RpMatFXMaterialSetBumpMapFrame(dstMaterial, frame);
                    RWASSERT(ret);

                    ret = RpMatFXMaterialSetBumpMapCoefficient(dstMaterial, coef);
                    RWASSERT(ret);

                    dstBumpMapData->texture = srcBumpMapData->texture;
                    dstBumpMapData->bumpTexture = srcBumpMapData->bumpTexture;

                    if (NULL != dstBumpMapData->texture)
                    {
                        (void)RwTextureAddRef(dstBumpMapData->texture);
                    }

                    if (NULL != dstBumpMapData->bumpTexture)
                    {
                        (void)RwTextureAddRef(dstBumpMapData->bumpTexture);
                    }

                    break;
                }
            case rpMATFXEFFECTENVMAP:
                {
                    /* EnvMap effect. */
                    RwTexture *texture;
                    RwFrame   *frame;
                    RwReal    coef;
                    RwBool    useFrameBufferAlpha;

                    texture = RpMatFXMaterialGetEnvMapTexture(srcMaterial);
                    frame   = RpMatFXMaterialGetEnvMapFrame(srcMaterial);
                    coef    = RpMatFXMaterialGetEnvMapCoefficient(srcMaterial);
                    useFrameBufferAlpha =
                        RpMatFXMaterialGetEnvMapFrameBufferAlpha(srcMaterial);

                    if (NULL != texture)
                    {
                        ret = RpMatFXMaterialSetEnvMapTexture(dstMaterial,
                                                              texture);
                        RWASSERT(ret);
                    }

                    ret = RpMatFXMaterialSetEnvMapFrame(dstMaterial, frame);
                    RWASSERT(ret);

                    ret = RpMatFXMaterialSetEnvMapFrameBufferAlpha(
                                                                   dstMaterial, useFrameBufferAlpha);
                    RWASSERT(ret);

                    ret = RpMatFXMaterialSetEnvMapCoefficient(dstMaterial, coef);
                    RWASSERT(ret);

                    break;
                }
            case rpMATFXEFFECTDUAL:
                {
                    /* Dual effect. */
                    RwTexture *texture;
                    RwBlendFunction srcBlendMode;
                    RwBlendFunction dstBlendMode;

                    texture = RpMatFXMaterialGetDualTexture(srcMaterial);
                    RpMatFXMaterialGetDualBlendModes(srcMaterial,
                                                     &srcBlendMode,
                                                     &dstBlendMode);

                    if (NULL != texture)
                    {
                        ret = RpMatFXMaterialSetDualTexture(dstMaterial,
                                                            texture);
                        RWASSERT(ret);
                    }

                    ret = RpMatFXMaterialSetDualBlendModes(dstMaterial,
                                                           srcBlendMode,
                                                           dstBlendMode);
                    RWASSERT(ret);

                    break;
                }
            case rpMATFXEFFECTUVTRANSFORM:
                {
                    /* UV Anim effect. */
                    RwMatrix *baseTransform;
                    RwMatrix *dualTransform;

                    srcMaterial = RpMatFXMaterialGetUVTransformMatrices(
                                      srcMaterial,
                                      &baseTransform,
                                      &dualTransform );
                    RWASSERT(NULL != srcMaterial);

                    dstMaterial = RpMatFXMaterialSetUVTransformMatrices(
                                      dstMaterial,
                                      baseTransform,
                                      dualTransform );
                    RWASSERT(NULL != dstMaterial);

                    break;
                }
            default:
                {
                    /* We shouldn't get here. */
                    RWASSERT(FALSE);
                    break;
                }
        }
    }

    RWRETURN(dstObject);
}

RwStream *
_rpMatFXStreamWriteTexture(RwStream *stream, const RwTexture *texture)
{
    RwBool present;

    RWFUNCTION(RWSTRING("_rpMatFXStreamWriteTexture"));
    RWASSERT(stream);

    present = (NULL != texture);

    /* Write out the flag. */
    if (!RwStreamWriteInt32(stream, (RwInt32 *)&present, sizeof(RwBool)))
    {
        RWRETURN((RwStream *)NULL);
    }

    /* Write texture if present */
    if (present)
    {
        if (!RwTextureStreamWrite(texture, stream))
        {
            RWRETURN((RwStream *)NULL);
        }
    }

    RWRETURN(stream);
}

RwStream *
_rpMatFXStreamReadTexture(RwStream *stream, RwTexture **texture)
{
    RwBool       present;

    RWFUNCTION(RWSTRING("_rpMatFXStreamReadTexture"));
    RWASSERT(stream);
    RWASSERT(texture);

    /* Read the flag */
    if (!RwStreamReadInt32(stream, (RwInt32 *)&present, sizeof(RwBool)))
    {
        RWRETURN((RwStream *)NULL);
    }

    /* Read texture if present */
    if (present)
    {
        RwError err;

        if (!RwStreamFindChunk(
                stream, rwID_TEXTURE, (RwUInt32 *)NULL, (RwUInt32 *)NULL))
        {
            RWRETURN((RwStream *)NULL);
        }

        /* We rely on error mechanism to recover from benign non-existence of
         * bumped texture. First clear any current error (presumably also benign).
         */
        RwErrorGet(&err);

        *texture = RwTextureStreamRead(stream);

        if (!(*texture))
        {
            /* Recover if just failed to read texture */
            RwErrorGet(&err);
            if ((err.errorCode != E_RW_NOERROR)
                && (err.errorCode != E_RW_READTEXMASK))
            {
                /* Something else went wrong, so lets put back the error and abort */
                RwErrorSet(&err);
                RWRETURN((RwStream *)NULL);
            }
        }
    }
    else
    {
        *texture = (RwTexture *)NULL;
    }

    RWRETURN(stream);
}

RwUInt32
_rpMatFXStreamSizeTexture(const RwTexture * texture)
{
    RwUInt32 size;

    RWFUNCTION(RWSTRING("_rpMatFXStreamSizeTexture"));

    /* We always write a flag for the texture. */
    size = sizeof(RwBool);

    /* Add texture size if present */
    if (texture)
    {
        size += RwTextureStreamGetSize(texture) + rwCHUNKHEADERSIZE;
    }

    RWRETURN(size);
}

static RwStream    *
MatFXMaterialStreamWrite(RwStream      *stream,
                         RwInt32        binaryLength   __RWUNUSEDRELEASE__,
                         const void    *object,
                         RwInt32        offsetInObject __RWUNUSED__,
                         RwInt32        sizeInObject   __RWUNUSED__)
{
    const RpMaterial  *material = (const RpMaterial *) object;
    const rpMatFXMaterialData *materialData;
    RwUInt8                 pass;
    RwInt32                 temp;

    RWFUNCTION(RWSTRING("MatFXMaterialStreamWrite"));
    RWASSERT(stream);
    RWASSERT(object);
    RWASSERT(binaryLength > 0);

    /* Lets grap the material and it's data. */
    materialData = (const rpMatFXMaterialData *)
        *MATFXMATERIALGETCONSTDATA(material);

    /* Must have material data (GetSize returned > 0) */
    RWASSERT(materialData);
    RWASSERT(materialData->flags != rpMATFXEFFECTNULL);

    /* Write our material effect. */
    temp = materialData->flags;
    if (!RwStreamWriteInt32(stream, &temp, sizeof(RwInt32)))
    {
        RWRETURN((RwStream *)NULL);
    }

    for (pass = 0; pass < rpMAXPASS; ++pass)
    {
        RpMatFXMaterialFlags effect;

        /* Write the effect type */
        effect = materialData->data[pass].flag;
        if (!RwStreamWriteInt32(stream, (RwInt32 *)&effect, sizeof(RwInt32)))
        {
            RWRETURN((RwStream *)NULL);
        }

        /* Now lets write the pass' individual data. */
        switch (effect)
        {
            case rpMATFXEFFECTNULL:
            case rpMATFXEFFECTUVTRANSFORM: /* No data to save. */
            {
                /* Empty pass. */
                break;
            }
            case rpMATFXEFFECTBUMPMAP:
            {
                /* BumpMap effect. */
                const MatFXBumpMapData *bumpData;
                RwReal coef;

                bumpData = &(materialData->data[pass].data.bumpMap);

                /* Undo negative coef. */
                coef = - bumpData->coef;

                /* 1st we write the coefficient ... */
                if (!RwStreamWriteReal(stream, &coef, sizeof(RwReal)))
                {
                    RWRETURN((RwStream *)NULL);
                }

                /* ... 2nd we write the texture ... */
                if (!_rpMatFXStreamWriteTexture(stream, bumpData->texture))
                {
                    RWRETURN((RwStream *)NULL);
                }

                /* ... 3rd we write the bump texture ... */
                if (!_rpMatFXStreamWriteTexture(stream, bumpData->bumpTexture))
                {
                    RWRETURN((RwStream *)NULL);
                }

                break;
            }
            case rpMATFXEFFECTENVMAP:
            {
                const MatFXEnvMapData *envData;

                envData = &(materialData->data[pass].data.envMap);

                /* First we write the coefficient ... */
                if (!RwStreamWriteReal(
                        stream, &(envData->coef), sizeof(RwReal)))
                {
                    RWRETURN((RwStream *)NULL);
                }

                /* ... and then the 'useFrameBufferAlpha' ... */
                temp = envData->useFrameBufferAlpha;
                if (!RwStreamWriteInt32(stream, &temp, sizeof(RwInt32)))
                {
                    RWRETURN((RwStream *)NULL);
                }

                /* ... and finally the texture ... */
                if (!_rpMatFXStreamWriteTexture(stream, envData->texture))
                {
                    RWRETURN((RwStream *)NULL);
                }

                break;
            }
            case rpMATFXEFFECTDUAL:
            {
                const MatFXDualData *dualData;
                dualData = &(materialData->data[pass].data.dual);

                /* First we write the blend modes ... */
                temp = dualData->srcBlendMode;
                if (!RwStreamWriteInt32(stream, &temp, sizeof(RwInt32)))
                {
                    RWRETURN((RwStream *)NULL);
                }

                temp = dualData->dstBlendMode;
                if (!RwStreamWriteInt32(stream, &temp, sizeof(RwInt32)))
                {
                    RWRETURN((RwStream *)NULL);
                }

                if (!_rpMatFXStreamWriteTexture(stream, dualData->texture))
                {
                    RWRETURN((RwStream *)NULL);
                }

                break;
            }
            default:
            {
                /* We shouldn't get here. */
                RWASSERT(FALSE);
                break;
            }
        }
    }

    /* And we're spent. */
    RWRETURN(stream);
}

static RwStream    *
MatFXMaterialStreamRead(RwStream   *stream,
                        RwInt32     binaryLength    __RWUNUSEDRELEASE__,
                        void       *object,
                        RwInt32     offsetInObject  __RWUNUSED__,
                        RwInt32     sizeInObject    __RWUNUSED__)
{
    RpMaterial             *material;
    rpMatFXMaterialData    *materialData;
    RpMatFXMaterialFlags    flags;
    RwUInt8                 pass;

    RWFUNCTION(RWSTRING("MatFXMaterialStreamRead"));
    RWASSERT(stream);
    RWASSERT(object);
    RWASSERT(binaryLength > 0);

    /* Lets grap the material and it's data. */
    material = (RpMaterial *) object;
    materialData = MatFXMaterialGetData(material);
    if (NULL == materialData)
    {
        /* Something failed */
        RWRETURN((RwStream *) NULL);
    }

    /* Read effect type */
    if (!RwStreamReadInt32(stream, (RwInt32 *)&flags, sizeof(RwInt32)))
    {
        RWRETURN((RwStream *)NULL);
    }

    /* Data should never have been written if NULL */
    RWASSERT(flags != rpMATFXEFFECTNULL);

    /* Set the effects. */
    RpMatFXMaterialSetEffects(material, flags);

    for (pass = 0; pass < rpMAXPASS; ++pass)
    {
        RpMatFXMaterialFlags effect;

        /* Lets read in the material effect. */
        if (!RwStreamReadInt32(stream, (RwInt32 *)&effect, sizeof(RwInt32)))
        {
            RWRETURN((RwStream *)NULL);
        }

        /* ... and remember it.
         *
         * materialData->data[pass].flag = effect;
         *
         * Not necessary as we've already set these up. */

        /* Now lets read the pass' individual data. */
        switch (effect)
        {
            case rpMATFXEFFECTNULL:
            case rpMATFXEFFECTUVTRANSFORM: /* No data to save. */
            {
                /* Empty pass. */
                break;
            }
            case rpMATFXEFFECTBUMPMAP:
            {
                /* BumpMap effect. */
                RwReal              coef;
                RwTexture          *texture = NULL;
                RwTexture          *bumpTexture = NULL;
                MatFXBumpMapData   *bumpMapData;

                bumpMapData = &(materialData->data[pass].data.bumpMap);

                /* First we read the coefficient ... */
                if (!RwStreamReadReal(stream, &coef, sizeof(RwReal)))
                {
                    RWRETURN((RwStream *)NULL);
                }

                /* ... 2nd we read the texture ...*/
                if (!_rpMatFXStreamReadTexture(stream, &texture))
                {
                    RWRETURN((RwStream *)NULL);
                }

                /* ... 3rd we read the bumpTexture ...*/
                if (!_rpMatFXStreamReadTexture(stream, &bumpTexture))
                {
                    if (texture)
                    {
                        /* Cleanup the other texture */
                        RwTextureDestroy(texture);
                    }

                    RWRETURN((RwStream *)NULL);
                }

                /* Setup the effect. */

                /* Were we able to load the bumped texture? */
                if (texture != NULL)
                {
                    RwRaster    *raster;
                    RwInt32     nWidth;
                    RwReal      width;

                    /* Yes. Then lets just pointer patch up. */
                    bumpMapData->texture = texture;
                    bumpMapData->bumpTexture = bumpTexture;

                    /* Grab it's width and calculate the invBumpWidth. */
                    RWASSERT(bumpMapData->texture);
                    raster = RwTextureGetRaster(bumpMapData->texture);
                    RWASSERT(raster);

                    nWidth = RwRasterGetWidth(raster);
                    width = (RwReal) nWidth;

                    bumpMapData->invBumpWidth = 1.0f / width;
                }
                else if (bumpTexture != NULL)
                {
                    /* We must rebuild our bump texture. */
                    RpMatFXMaterialSetBumpMapTexture(material, bumpTexture);

                    /* This has incremented the texture reference count
                     * so correct the reference count */
                    RwTextureDestroy(bumpTexture);
                }
                else
                {
                    /* No textures so lets just use the base. */
                    bumpMapData->texture = (RwTexture *)NULL;
                    bumpMapData->bumpTexture = (RwTexture *)NULL;
                }

                RpMatFXMaterialSetBumpMapCoefficient(material, coef);

                break;
            }
            case rpMATFXEFFECTENVMAP:
            {
                /* EnvMap effect. */
                RwReal              coef;
                RwBool              useFrameBufferAlpha;
                RwTexture          *texture=0;

                /* First we read the coefficient ... */
                if (!RwStreamReadReal(stream, &coef, sizeof(RwReal)))
                {
                    RWRETURN((RwStream *)NULL);
                }

                /* ... and then the 'useFrameBufferAlpha' ... */
                if (!RwStreamReadInt32(stream,
                        (RwInt32 *)&useFrameBufferAlpha, sizeof(RwInt32)))
                {
                    RWRETURN((RwStream *)NULL);
                }

                /* ... and finally the texture ... */
                if (!_rpMatFXStreamReadTexture(stream, &texture))
                {
                    RWRETURN((RwStream *)NULL);
                }

                /* Setup the effect. */
                if (texture != NULL)
                {
                    RpMatFXMaterialSetEnvMapTexture(material, texture);

                    /* This has incremented the texture reference count
                     * so correct the reference count */
                    RwTextureDestroy(texture);
                }

                RpMatFXMaterialSetEnvMapCoefficient(material, coef);
                RpMatFXMaterialSetEnvMapFrameBufferAlpha(material,
                                                         useFrameBufferAlpha);

                break;
            }
            case rpMATFXEFFECTDUAL:
            {
                /* Dual effect. */
                RwBlendFunction     blendFuncs[2];
                RwTexture          *texture=0;

                /* First we read the blend modes ... */
                if (!RwStreamReadInt32(stream,
                        (RwInt32 *)blendFuncs, sizeof(blendFuncs)))
                {
                    RWRETURN((RwStream *)NULL);
                }

                /* Second we write the texture ... */
                if (!_rpMatFXStreamReadTexture(stream, &texture))
                {
                    RWRETURN((RwStream *)NULL);
                }

                /* Setup the effect. */
                if (texture != NULL)
                {
                    RpMatFXMaterialSetDualTexture(material, texture);

                    /* This has incremented the texture reference count
                     * so correct the reference count */
                    RwTextureDestroy(texture);
                }

                RpMatFXMaterialSetDualBlendModes(
                    material, blendFuncs[0], blendFuncs[1]);

                break;
            }
            default:
            {
                /* We shouldn't get here. */
                RWASSERT(FALSE);
                break;
            }
        }
    }

    /* And we're spent. */
    RWRETURN(stream);
}

static RwInt32
MatFXMaterialStreamGetSize(const void  *object,
                           RwInt32      offsetInObject  __RWUNUSED__,
                           RwInt32      sizeInObject    __RWUNUSED__)
{
    const RpMaterial   *material;
    const rpMatFXMaterialData *materialData;

    RwInt32             size;
    RwUInt8             pass;

    RWFUNCTION(RWSTRING("MatFXMaterialStreamGetSize"));
    RWASSERT(object);

    /* Lets grap the material and it's data. */
    material = (const RpMaterial *) object;
    materialData = (const rpMatFXMaterialData *)
        *MATFXMATERIALGETCONSTDATA(material);

    /* Do we have anything else to count? */
    if (NULL == materialData || rpMATFXEFFECTNULL == materialData->flags)
    {
        RWRETURN(0);
    }

    /* Start with effects flag */
    size = sizeof(RpMatFXMaterialFlags);

    for (pass = 0; pass < rpMAXPASS; ++pass)
    {
        RpMatFXMaterialFlags effect;

        /* Lets grab the effect ... */
        effect = materialData->data[pass].flag;

        size += sizeof(RpMatFXMaterialFlags);

        /* Now lets count the pass' individual data. */
        switch (effect)
        {
            case rpMATFXEFFECTNULL:
            case rpMATFXEFFECTUVTRANSFORM: /* No data to save. */
            {
                /* Empty pass. */
                break;
            }
            case rpMATFXEFFECTBUMPMAP:
            {
                /* First we count the coefficient ... */
                size += sizeof(RwReal);

                /* ... then the texture ... */
                size += _rpMatFXStreamSizeTexture(
                            materialData->data[pass].data.bumpMap.texture);

                /* ... then the bumpTexture ... */
                size += _rpMatFXStreamSizeTexture(
                            materialData->data[pass].data.bumpMap.bumpTexture);

                /* ... and that's it. */
                break;
            }
            case rpMATFXEFFECTENVMAP:
            {
                /* First we count the coefficient ... */
                size += sizeof(RwReal);

                /* ... and then the 'useFrameBufferAlpha' ... */
                size += sizeof(RwBool);

                /* ... and then the texture ... */
                size += _rpMatFXStreamSizeTexture(
                            materialData->data[pass].data.envMap.texture);

                /* ... that's it. */
                break;
            }
            case rpMATFXEFFECTDUAL:
            {
                /* First we count the blend modes ... */
                size += 2 * sizeof(RwBlendFunction);

                /* ... and then the texture ... */
                size += _rpMatFXStreamSizeTexture(
                            materialData->data[pass].data.dual.texture);

                /* ... and that's it. */
                break;
            }
            default:
            {
                /* We shouldn't get here. */
                RWASSERT(FALSE);
                break;
            }
        }
    }

    /* And we're spent. */
    RWRETURN(size);
}

/*---------------------------------------------------------------------------*
 *    Atomic: constructor, destructor, copy,                                 *
 *            stream read, stream write, stream size.                        *
 *---------------------------------------------------------------------------*/
static void        *
MatFXAtomicConstructor(void *object,
                       RwInt32 offset __RWUNUSED__,
                       RwInt32 size __RWUNUSED__)
{
    MatFXAtomicData    *atomicData;

    RWFUNCTION(RWSTRING("MatFXAtomicConstructor"));
    RWASSERT(object);

    atomicData = (MatFXAtomicData *)
        MATFXATOMICGETDATA((RpAtomic *) object);
    RWASSERT(atomicData);

    atomicData->enabled = FALSE;

    RWRETURN(object);
}

static void        *
MatFXAtomicDestructor(void *object,
                      RwInt32 offset __RWUNUSED__,
                      RwInt32 size __RWUNUSED__)
{
    MatFXAtomicData    *atomicData;

    RWFUNCTION(RWSTRING("MatFXAtomicDestructor"));
    RWASSERT(object);

    atomicData = (MatFXAtomicData *)
        MATFXATOMICGETDATA((RpAtomic *) object);
    RWASSERT(atomicData);

    atomicData->enabled = FALSE;

    RWRETURN(object);
}

static void        *
MatFXAtomicCopy(void *dstObject,
                const void *srcObject,
                RwInt32 offset __RWUNUSED__,
                RwInt32 size __RWUNUSED__)
{
    const RpAtomic     *srcAtomic;
    const MatFXAtomicData *srcAtomicData;

    RpAtomic           *dstAtomic;
    MatFXAtomicData *dstAtomicData;

    RWFUNCTION(RWSTRING("MatFXAtomicCopy"));
    RWASSERT(dstObject);
    RWASSERT(srcObject);

    srcAtomic = (const RpAtomic *) srcObject;
    dstAtomic = (RpAtomic *) dstObject;

    srcAtomicData = (const MatFXAtomicData *)
        MATFXATOMICGETCONSTDATA(srcAtomic);
    dstAtomicData = (MatFXAtomicData *)
        MATFXATOMICGETDATA(dstAtomic);

    /* Is the source atomic enabled ... */
    if (srcAtomicData->enabled)
    {
        /* YES, then lets enable the target atomic. */
        dstAtomicData->enabled = TRUE;
    }

    RWRETURN(dstObject);
}

static RwStream    *
MatFXAtomicStreamWrite(RwStream    *stream,
                       RwInt32      binaryLength    __RWUNUSEDRELEASE__,
                       const void  *object,
                       RwInt32      offsetInObject  __RWUNUSED__,
                       RwInt32      sizeInObject    __RWUNUSED__)
{
    const MatFXAtomicData *atomicData = MATFXATOMICGETCONSTDATA(object);
    RwStream    *streamOut;
    RwInt32     temp;

    RWFUNCTION(RWSTRING("MatFXAtomicStreamWrite"));
    RWASSERT(stream);
    RWASSERT(object);
    RWASSERT(binaryLength > 0);
    RWASSERT(atomicData->enabled);

    /* Enabled flag, always TRUE if we got here */
    temp = atomicData->enabled;
    streamOut = RwStreamWriteInt32(stream, &temp, sizeof(RwInt32));

    RWRETURN(streamOut);
}

static RwStream    *
MatFXAtomicStreamRead(RwStream *stream,
                      RwInt32   binaryLength    __RWUNUSEDRELEASE__,
                      void     *object,
                      RwInt32   offsetInObject  __RWUNUSED__,
                      RwInt32   sizeInObject    __RWUNUSED__)
{
    RpAtomic   *atomic = (RpAtomic *) object;
    RwBool      enabled;

    RWFUNCTION(RWSTRING("MatFXAtomicStreamRead"));
    RWASSERT(stream);
    RWASSERT(object);
    RWASSERT(binaryLength > 0);

    /* Read in the atomic data. */
    if (!RwStreamReadInt32(stream, (RwInt32 *)&enabled, sizeof(RwInt32)))
    {
        RWRETURN((RwStream *)NULL);
    }

    /* Do we need to enable the effects? */
    if (enabled)
    {
        RpMatFXAtomicEnableEffects(atomic);
    }

    /* And we're spent. */
    RWRETURN(stream);
}

static RwInt32
MatFXAtomicStreamGetSize(const void *object,
                         RwInt32     offsetInObject __RWUNUSED__,
                         RwInt32     sizeInObject   __RWUNUSED__)
{
    const MatFXAtomicData *atomicData = MATFXATOMICGETCONSTDATA(object);

    RWFUNCTION(RWSTRING("MatFXAtomicStreamGetSize"));
    RWASSERT(object);

    /* Don't write anything if not enabled */
    if (FALSE == atomicData->enabled)
    {
        RWRETURN(0);
    }

    /* Enabled flag */
    RWRETURN(sizeof(RwBool));
}

/*---------------------------------------------------------------------------*
 *    WorldSector: constructor, destructor, copy,                            *
 *                 stream read, stream write, stream size.                   *
 *---------------------------------------------------------------------------*/

static void        *
MatFXWorldSectorConstructor(void *object,
                            RwInt32 offset __RWUNUSED__,
                            RwInt32 size __RWUNUSED__)
{
    MatFXWorldSectorData *worldSectorData;

    RWFUNCTION(RWSTRING("MatFXWorldSectorConstructor"));
    RWASSERT(object);

    /* Grab the world sector plugins data. */
    worldSectorData = (MatFXWorldSectorData *)
        MATFXWORLDSECTORGETDATA((RpWorldSector *) object);
    RWASSERT(worldSectorData);

    worldSectorData->enabled = FALSE;

    RWRETURN(object);
}

static void        *
MatFXWorldSectorDestructor(void *object,
                           RwInt32 offset __RWUNUSED__,
                           RwInt32 size __RWUNUSED__)
{
    MatFXWorldSectorData *worldSectorData;

    RWFUNCTION(RWSTRING("MatFXWorldSectorDestructor"));
    RWASSERT(object);

    /* Grab the world sector plugins data. */
    worldSectorData = (MatFXWorldSectorData *)
        MATFXWORLDSECTORGETDATA((RpWorldSector *) object);
    RWASSERT(worldSectorData);

    worldSectorData->enabled = FALSE;

    RWRETURN(object);
}

static void        *
MatFXWorldSectorCopy(void *dstObject,
                     const void *srcObject,
                     RwInt32 offset __RWUNUSED__,
                     RwInt32 size __RWUNUSED__)
{
    const RpWorldSector *srcWorldSector;
    const MatFXWorldSectorData *srcWorldSectorData;

    RpWorldSector      *dstWorldSector;
    MatFXWorldSectorData *dstWorldSectorData;

    RWFUNCTION(RWSTRING("MatFXWorldSectorCopy"));
    RWASSERT(dstObject);
    RWASSERT(srcObject);

    srcWorldSector = (const RpWorldSector *) srcObject;
    dstWorldSector = (RpWorldSector *) dstObject;

    srcWorldSectorData = (const MatFXWorldSectorData *)
        MATFXWORLDSECTORGETCONSTDATA(srcWorldSector);
    dstWorldSectorData = (MatFXWorldSectorData *)
        MATFXWORLDSECTORGETDATA(dstWorldSector);

    /* Is the source world sector enabled ... */
    if (srcWorldSectorData->enabled)
    {
        /* YES, then lets enable the target WorldSector. */
        dstWorldSectorData->enabled = TRUE;
    }

    RWRETURN(dstObject);
}

static RwStream *
MatFXWorldSectorStreamWrite(RwStream   *stream,
                            RwInt32     binaryLength    __RWUNUSEDRELEASE__,
                            const void *object,
                            RwInt32     offsetInObject  __RWUNUSED__,
                            RwInt32     sizeInObject    __RWUNUSED__)
{
    RwStream    *streamOut;
    RwInt32     temp;
    const MatFXWorldSectorData *sectorData =
                    MATFXWORLDSECTORGETCONSTDATA(object);

    RWFUNCTION(RWSTRING("MatFXWorldSectorStreamWrite"));
    RWASSERT(stream);
    RWASSERT(object);
    RWASSERT(binaryLength > 0);

    /* Lets write out the extended data. */
    temp = sectorData->enabled;
    streamOut = RwStreamWriteInt32(stream, &temp, sizeof(RwInt32));

    RWRETURN(streamOut);
}

static RwStream *
MatFXWorldSectorStreamRead(RwStream *stream,
                           RwInt32   binaryLength   __RWUNUSEDRELEASE__,
                           void     *object,
                           RwInt32   offsetInObject __RWUNUSED__,
                           RwInt32   sizeInObject   __RWUNUSED__)
{
    RpWorldSector  *worldSector = (RpWorldSector *) object;
    RwBool          enabled;

    RWFUNCTION(RWSTRING("MatFXWorldSectorStreamRead"));
    RWASSERT(stream);
    RWASSERT(object);
    RWASSERT(binaryLength > 0);

    /* Read the worldSector data. */
    if (!RwStreamReadInt32(stream, (RwInt32 *)&enabled, sizeof(RwInt32)))
    {
        RWRETURN((RwStream *)NULL);
    }

    /* Do we need to enable the effects? */
    if (enabled)
    {
        RpMatFXWorldSectorEnableEffects(worldSector);
    }

    RWRETURN(stream);
}

static RwInt32
MatFXWorldSectorStreamGetSize(const void *object,
                              RwInt32     offsetInObject __RWUNUSED__,
                              RwInt32     sizeInObject   __RWUNUSED__)
{
    const MatFXWorldSectorData *sectorData =
            MATFXWORLDSECTORGETCONSTDATA(object);

    RWFUNCTION(RWSTRING("MatFXWorldSectorStreamGetSize"));
    RWASSERT(object);

    /* Is the world sector material effects enabled? */
    if (FALSE == sectorData->enabled)
    {
        /* Nothing to write */
        RWRETURN(0);
    }

    /* Single enabled flag. */
    RWRETURN(sizeof(RwBool));
}

/*---------------------------------------------------------------------------*
 *    Texture functions create functions                                     *
 *---------------------------------------------------------------------------*/

static void
GenBumpedTextureName(RwChar *bumpedTexName,
                     const RwTexture *baseTexture,
                     const RwTexture *maskTexture)
{
    /*
     * Generate bumped texture name from alternating characters of base
     * and mask texture names.
     */

    RwInt32             i = 0, j;
    const RwChar       *oldName[2];
    RwChar             *newPtr;

    RWFUNCTION(RWSTRING("GenBumpedTextureName"));
    RWASSERT(bumpedTexName);
    RWASSERT(maskTexture);

    newPtr = bumpedTexName;

    if (baseTexture)
    {
        oldName[0] = baseTexture->name;
        oldName[1] = maskTexture->name;
    }
    else
    {
        oldName[0] = oldName[1] = maskTexture->name;
    }

    /* Compare with length-2 when checking overrun since we
       write in pairs of characters. */
    while ((i < (rwTEXTUREBASENAMELENGTH - 2))
           && ((*oldName[0] != 0) || (*oldName[1] != 0)))
    {
        for (j = 0; j < 2; ++j)
        {
            if (*oldName[j] != 0)
            {
                *newPtr++ = *oldName[j]++;
                ++i;
            }
        }
    }

    /* Add the terminating NULL */
    *newPtr = 0;

    RWRETURNVOID();
}

RwTexture          *
_rpMatFXTextureMaskCreate(const RwTexture * baseTexture,
                          const RwTexture * maskTexture)
{
    RwRaster           *baseRaster = (RwRaster *)NULL;
    RwRaster           *maskRaster;
    RwRaster           *newRaster;

    RwInt32             maskX, maskY;
    RwInt32             baseX, baseY;

    RwImage            *maskImage;
    RwImage            *baseImage;

    RwTexture          *result;

    RwTextureFilterMode filterMode;
    RwTextureAddressMode addrsMode;

    RWFUNCTION(RWSTRING("_rpMatFXTextureMaskCreate"));
    RWASSERT(maskTexture);

    /* get the mask raster */
    maskRaster = RwTextureGetRaster(maskTexture);
    RWASSERT(maskRaster);

    maskX = RwRasterGetWidth(maskRaster);
    maskY = RwRasterGetHeight(maskRaster);
    maskImage = RwImageCreate(maskX, maskY, 32);
    RWASSERT(maskImage);

    RwImageAllocatePixels(maskImage);
    RwImageSetFromRaster(maskImage, maskRaster);

    if (baseTexture)
    {
        /* Grab the base raster. */
        baseRaster = RwTextureGetRaster(baseTexture);
        RWASSERT(baseRaster);

        baseX = RwRasterGetWidth(baseRaster);
        baseY = RwRasterGetHeight(baseRaster);
        baseImage = RwImageCreate(baseX, baseY, 32);
        RWASSERT(baseImage);

        RwImageAllocatePixels(baseImage);
        RwImageSetFromRaster(baseImage, baseRaster);
    }
    else
    {
        RwInt32             x, y;

        /* Make a white base texture */
        baseX = RwRasterGetWidth(maskRaster);
        baseY = RwRasterGetHeight(maskRaster);
        baseImage = RwImageCreate(baseX, baseY, 32);
        RWASSERT(baseImage);
        RwImageAllocatePixels(baseImage);

        /* fill in pixels */
        for (y = 0; y < baseY; ++y)
        {
            for (x = 0; x < baseX; ++x)
            {
                *(RwUInt32 *) (RwImageGetPixels(baseImage) +
                               RwImageGetStride(baseImage) * y +
                               x * 4) = 0xFfFfFfFf;
            }
        }
    }

    /* need to resize ? */
    if ((baseX != maskX) || (baseY != maskY))
    {
        RwImage            *resampleImage;

        resampleImage = RwImageCreate(baseX, baseY, 32);
        RWASSERT(resampleImage);

        RwImageAllocatePixels(resampleImage);
        RwImageResample(resampleImage, maskImage);
        RwImageDestroy(maskImage);

        maskImage = resampleImage;
    }

    /* create and apply the mask */
    RwImageMakeMask(maskImage);
    RwImageApplyMask(baseImage, maskImage);

    {
        RwInt32             rasterWidth, rasterHeight;
        RwInt32             rasterDepth, rasterFlags, rasterBaseFlags;

        /* Find the desired raster format */
        RwImageFindRasterFormat(baseImage, rwRASTERTYPETEXTURE,
                                &rasterWidth, &rasterHeight,
                                &rasterDepth, &rasterFlags);

        /* Save MIPMAPPING information. */
        if (baseTexture)
        {
            rasterBaseFlags = RwRasterGetFormat(baseRaster);
        }
        else
        {
            rasterBaseFlags = RwRasterGetFormat(maskRaster);
        }

        if (rasterBaseFlags & rwRASTERFORMATMIPMAP)
        {
            rasterFlags |= (rwRASTERFORMATAUTOMIPMAP |
                              rwRASTERFORMATMIPMAP);
        }

        newRaster = RwRasterCreate(rasterWidth, rasterHeight,
                                   rasterDepth, rasterFlags);
        RWASSERT(newRaster);
        RwRasterSetFromImage(newRaster, baseImage);
    }

    result = RwTextureCreate(newRaster);
    RWASSERT(result);

    if (baseTexture)
    {
        /* preserve the attributes of the base texture */
        filterMode = RwTextureGetFilterMode(baseTexture);
        addrsMode = RwTextureGetAddressing(baseTexture);
    }
    else
    {
        /* or the bump texture if the base doesn't exist... */
        filterMode = RwTextureGetFilterMode(maskTexture);
        addrsMode = RwTextureGetAddressing(maskTexture);
    }

    RwTextureSetAddressing(result, addrsMode);
    RwTextureSetFilterMode(result, filterMode);

    /* Clean up. */
    RwImageDestroy(baseImage);
    RwImageDestroy(maskImage);

    /* Need to name the new texture. */
    {
        RwChar              newName[rwTEXTUREBASENAMELENGTH] = { 0 };

        GenBumpedTextureName(newName, baseTexture, maskTexture);
        RwTextureSetName(result, newName);
    }

    RWRETURN(result);
}

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/*--- Plugin functions ------------------------------------------------------*/

/**
 * \ingroup rpmatfx
 * \ref RpMatFXPluginAttach is called by the application to indicate that the
 * material effects plugin should be used. The call to this function should be
 * placed between \ref RwEngineInit and \ref RwEngineOpen and the world plugin
 * must already be attached.
 *
 * \warning The include files rpmatfx.h and rpworld.h are also required and
 * must be included by an application wishing to generate material effects.
 *
 * \warning The material effects plugin requires
 * \li the material effects plugin rpmatfx.lib and
 * \li the world plugin rpworld.lib
 * to be linked into the application and attached at run time.
 *
 * \return True on success, false otherwise
 *
 * \see RpWorldPluginAttach
 * \see RpMatFXAtomicEnableEffects
 * \see RpMatFXWorldSectorEnableEffects
 * \see RpMatFXMaterialSetEffects
 */
RwBool
RpMatFXPluginAttach(void)
{
    RwInt32             offset;

    RWAPIFUNCTION(RWSTRING("RpMatFXPluginAttach"));

    /* Register the plugIn */
    if (RwEngineRegisterPlugin(0,
                               (RwUInt32) rwID_MATERIALEFFECTSPLUGIN,
                               MatFXOpen, MatFXClose) < 0)
    {
        RWRETURN(FALSE);
    }

    /* Register material plugIn */
    MatFXMaterialDataOffset =
        RpMaterialRegisterPlugin(sizeof(rpMatFXMaterialData *),
                                 (RwUInt32)rwID_MATERIALEFFECTSPLUGIN,
                                 MatFXMaterialConstructor,
                                 MatFXMaterialDestructor,
                                 MatFXMaterialCopy);

    if (MatFXMaterialDataOffset < 0)
    {
        RWRETURN(FALSE);
    }

    offset =
        RpMaterialRegisterPluginStream((RwUInt32)rwID_MATERIALEFFECTSPLUGIN,
                                       MatFXMaterialStreamRead,
                                       MatFXMaterialStreamWrite,
                                       MatFXMaterialStreamGetSize);
    if (offset < 0)
    {
        RWRETURN(FALSE);
    }

    /* Register atomic plugIn */
    MatFXAtomicDataOffset =
        RpAtomicRegisterPlugin(sizeof(RwBool),
                               (RwUInt32)rwID_MATERIALEFFECTSPLUGIN,
                               MatFXAtomicConstructor,
                               MatFXAtomicDestructor, MatFXAtomicCopy);

    if (MatFXAtomicDataOffset < 0)
    {
        RWRETURN(FALSE);
    }

    offset =
        RpAtomicRegisterPluginStream((RwUInt32)rwID_MATERIALEFFECTSPLUGIN,
                                     MatFXAtomicStreamRead,
                                     MatFXAtomicStreamWrite,
                                     MatFXAtomicStreamGetSize);
    if (offset < 0)
    {
        RWRETURN(FALSE);
    }

    /* Register worldSector plugIn */
    MatFXWorldSectorDataOffset =
        RpWorldSectorRegisterPlugin(sizeof(RwBool),
                                    (RwUInt32)rwID_MATERIALEFFECTSPLUGIN,
                                    MatFXWorldSectorConstructor,
                                    MatFXWorldSectorDestructor,
                                    MatFXWorldSectorCopy);

    if (MatFXWorldSectorDataOffset < 0)
    {
        RWRETURN(FALSE);
    }

    offset =
        RpWorldSectorRegisterPluginStream((RwUInt32)rwID_MATERIALEFFECTSPLUGIN,
                                          MatFXWorldSectorStreamRead,
                                          MatFXWorldSectorStreamWrite,
                                          MatFXWorldSectorStreamGetSize);
    if (offset < 0)
    {
        RWRETURN(FALSE);
    }

#if    defined(GCN_DRVMODEL_H) \
    || defined(NULLGCN_DRVMODEL_H) \
    || defined(NULL_DRVMODEL_H) \
    || defined(XBOX_DRVMODEL_H) \
    || defined(NULLXBOX_DRVMODEL_H)

    if (!_rpMultiTexturePlatformPluginsAttach())
    {
        RWRETURN(FALSE);
    }

#endif

    RWRETURN(TRUE);
}

/*--- Setup functions -------------------------------------------------------*/

/**
 * \ingroup rpmatfx
 * \ref RpMatFXAtomicEnableEffects is used to enable the material effects
 * object pipeline on an atomic.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param atomic Pointer to the \ref RpAtomic to enable the material effects
 *               pipeline on.
 *
 * \return The source \ref RpAtomic on success, NULL otherwise.
 *
 * \see RpMatFXPluginAttach
 * \see RpMatFXAtomicQueryEffects
 * \see RpMatFXWorldSectorEnableEffects
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialSetupBumpMap
 * \see RpMatFXMaterialSetupEnvMap
 * \see RpMatFXMaterialSetupDualTexture
 */
RpAtomic           *
RpMatFXAtomicEnableEffects(RpAtomic * atomic)
{
    MatFXAtomicData    *atomicData;

    RWAPIFUNCTION(RWSTRING("RpMatFXAtomicEnableEffects"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(atomic);

    atomicData = (MatFXAtomicData *)
        MATFXATOMICGETDATA(atomic);
    RWASSERT(atomicData);

    if (atomicData->enabled == FALSE)
    {
        if (NULL == _rpMatFXPipelineAtomicSetup(atomic))
        {
            RWRETURN((RpAtomic *) NULL);
        }

        atomicData->enabled = TRUE;
    }

    RWRETURN(atomic);
}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXAtomicQueryEffects is used to query whether material effects
 * have been enabled on the given atomic.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param atomic Pointer to an \ref RpAtomic to query for material effects.
 *
 * \return TRUE if effects are enabled, FALSE otherwise.
 *
 * \see RpMatFXPluginAttach
 * \see RpMatFXAtomicEnableEffects
 */
RwBool
RpMatFXAtomicQueryEffects(RpAtomic * atomic)
{
    MatFXAtomicData    *atomicData;

    RWAPIFUNCTION(RWSTRING("RpMatFXAtomicQueryEffects"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(atomic);

    atomicData = (MatFXAtomicData *) MATFXATOMICGETDATA(atomic);
    RWASSERT(atomicData);

    RWRETURN(atomicData->enabled);
}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXWorldSectorEnableEffects is used to enable the material effects
 * object pipeline on a world sector.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param worldSector Pointer to the \ref RpWorldSector to enable the material
 *                    effects pipeline on.
 *
 * \return The source \ref RpWorldSector on success, NULL otherwise.
 *
 * \see RpMatFXPluginAttach
 * \see RpMatFXWorldSectorQueryEffects
 * \see RpMatFXAtomicEnableEffects
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialSetupBumpMap
 * \see RpMatFXMaterialSetupEnvMap
 * \see RpMatFXMaterialSetupDualTexture
 */
RpWorldSector      *
RpMatFXWorldSectorEnableEffects(RpWorldSector * worldSector)
{
    MatFXWorldSectorData *worldSectorData;

    RWAPIFUNCTION(RWSTRING("RpMatFXWorldSectorEnableEffects"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(worldSector);

    worldSectorData = (MatFXWorldSectorData *)
        MATFXWORLDSECTORGETDATA(worldSector);
    RWASSERT(worldSectorData);

    /* Lets enable the effects. */
    if (worldSectorData->enabled == FALSE)
    {
        if (NULL == _rpMatFXPipelineWorldSectorSetup(worldSector))
        {
            RWRETURN((RpWorldSector *) NULL);
        }

        worldSectorData->enabled = TRUE;
    }

    RWRETURN(worldSector);
}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXWorldSectorQueryEffects is used to query whether material
 * effects have been enabled in the given world sector.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param worldSector Pointer to the \ref RpWorldSector to query for effects.
 *
 * \return TRUE if effects are enabled, FALSE otherwise.
 *
 * \see RpMatFXPluginAttach
 * \see RpMatFXWorldSectorEnableEffects
 */
RwBool
RpMatFXWorldSectorQueryEffects(RpWorldSector * worldSector)
{
    MatFXWorldSectorData *worldSectorData;

    RWAPIFUNCTION(RWSTRING("RpMatFXWorldSectorQueryEffects"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(worldSector);

    worldSectorData = (MatFXWorldSectorData *)
        MATFXWORLDSECTORGETDATA(worldSector);
    RWASSERT(worldSectorData);

    RWRETURN(worldSectorData->enabled);
}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialSetEffects is used to enable a particular effect on a
 * material. When an effect is selected, the material is cleaned of any old
 * effect.
 *
 * The material effect should then be setup using:
 *
 * \li \ref RpMatFXMaterialSetupBumpMap
 * \li \ref RpMatFXMaterialSetupEnvMap
 * \li \ref RpMatFXMaterialSetupDualTexture
 *
 * Which will setup the individual components of the effect type.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material Pointer to the \ref RpMaterial to set the effect type of.
 * \param flags    Flag to indicate which material type to setup.
 *
 * Supported material effect types are:
 *
 * \li \ref rpMATFXEFFECTNULL            No material effect.
 * \li \ref rpMATFXEFFECTBUMPMAP         Bump mapping effect.
 * \li \ref rpMATFXEFFECTENVMAP          Environment mapping effect.
 * \li \ref rpMATFXEFFECTBUMPENVMAP      Bump and environment mapping effect.
 * \li \ref rpMATFXEFFECTDUAL            Dual pass rendering.
 * \li \ref rpMATFXEFFECTUVTRANSFORM     Single pass texture coordination
 *                                          transformation.
 * \li \ref rpMATFXEFFECTDUALUVTRANSFORM Dual texture coordination
 *                                          transformation (2 pass).
 *
 * \return The source \ref RpMaterial on success, NULL otherwise
 *
 * \see RpMatFXPluginAttach
 * \see RpMatFXAtomicEnableEffects
 * \see RpMatFXWorldSectorEnableEffects
 * \see RpMatFXMaterialSetupBumpMap
 * \see RpMatFXMaterialSetupEnvMap
 * \see RpMatFXMaterialSetupDualTexture
 * \see RpMatFXMaterialGetEffects
 */
RpMaterial         *
RpMatFXMaterialSetEffects(RpMaterial * material,
                          RpMatFXMaterialFlags flags)
{
    rpMatFXMaterialData *materialData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialSetEffects"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);
    RWASSERT(rpMATFXEFFECTMAX > flags);

    /* Grab the materials extended data. */
    materialData = MatFXMaterialGetData(material);
    if (NULL == materialData)
    {
        RWRETURN((RpMaterial *) NULL);
    }

    /* Does the effect change need cleaning up? */
    if ((flags == rpMATFXEFFECTNULL) ||
        ((materialData->flags != rpMATFXEFFECTNULL) &&
         (materialData->flags != flags)))
    {
        MatFXMaterialDataClean(materialData);
    }

    /* Record the new flags. */
    materialData->flags = flags;

    /* Call the setup functions. */
    switch (materialData->flags)
    {
        case rpMATFXEFFECTBUMPMAP:
            {
                materialData->data[rpSECONDPASS].flag =
                    rpMATFXEFFECTBUMPMAP;

                break;
            }
        case rpMATFXEFFECTENVMAP:
            {
                materialData->data[rpSECONDPASS].flag =
                    rpMATFXEFFECTENVMAP;

                break;
            }
        case rpMATFXEFFECTBUMPENVMAP:
            {
                materialData->data[rpSECONDPASS].flag =
                    rpMATFXEFFECTBUMPMAP;
                materialData->data[rpTHIRDPASS].flag =
                    rpMATFXEFFECTENVMAP;

                break;
            }
        case rpMATFXEFFECTDUAL:
            {
                materialData->data[rpSECONDPASS].flag =
                    rpMATFXEFFECTDUAL;

                RpMatFXMaterialSetDualBlendModes(material, rwBLENDSRCALPHA,
                                                 rwBLENDINVSRCALPHA);

                break;
            }
        case rpMATFXEFFECTUVTRANSFORM:
            {
                materialData->data[rpSECONDPASS].flag =
                    rpMATFXEFFECTUVTRANSFORM;

                break;
            }
        case rpMATFXEFFECTDUALUVTRANSFORM:
            {
                materialData->data[rpSECONDPASS].flag =
                    rpMATFXEFFECTUVTRANSFORM;
                materialData->data[rpTHIRDPASS].flag =
                    rpMATFXEFFECTDUAL;

                RpMatFXMaterialSetDualBlendModes(material, rwBLENDSRCALPHA,
                                                 rwBLENDINVSRCALPHA);

                break;
            }
        case rpMATFXEFFECTNULL:
            {
                /* Nothing to do - as we've already cleaned the memory. */
                break;
            }
        default:
            {
                /* We shouldn't get here. */
                RWASSERT(FALSE);
                break;
            }
    }

    /* Platform specific effect initialization. */
    _rpMatFXMaterialPlatformInitialize(material);

    RWRETURN(material);
}

/*--- Setup Effects ---------------------------------------------------------*/
/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialSetupBumpMap is used to setup the various attributes
 * needed for the bump map material effect.
 *
 * \warning \ref RpMatFXMaterialSetupBumpMap should only be used after calling
 * \ref RpMatFXMaterialSetEffects with either:
 * \li rpMATFXEFFECTBUMPMAP
 * \li rpMATFXEFFECTBUMPENVMAP
 * as the effect type.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material Pointer to the \ref RpMaterial to setup the bump map effect
 *                 on.
 * \param texture  Pointer to the \ref RwTexture to use as the bump map.
 * \param frame    Pointer to the \ref RwFrame to simulate the location of the
 *                 light source.
 * \param coef     Coefficient controlling the bumpiness. There are no
 *                 specific limits, but useful values typically lie between
 *                 -10 and 10, depending on the scale of the bumps.
 *
 * \return The source \ref RpMaterial on success, NULL otherwise
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetBumpMapTexture
 * \see RpMatFXMaterialSetBumpMapFrame
 * \see RpMatFXMaterialSetBumpMapCoefficient
 * \see RpMatFXMaterialGetBumpMapTexture
 * \see RpMatFXMaterialGetBumpMapBumpedTexture
 * \see RpMatFXMaterialGetBumpMapFrame
 * \see RpMatFXMaterialGetBumpMapCoefficient
 */
RpMaterial         *
RpMatFXMaterialSetupBumpMap(RpMaterial * material,
                            RwTexture * texture,
                            RwFrame * frame, RwReal coef)
{
    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialSetupBumpMap"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    /* TODO[5]: MAKE THIS INTO A MACRO IN RELEASE */

    /* Lets just call the individual set functions for now. */
    if (NULL == RpMatFXMaterialSetBumpMapTexture(material, texture))
    {
        RWRETURN((RpMaterial *) NULL);
    }

    if (NULL == RpMatFXMaterialSetBumpMapFrame(material, frame))
    {
        RWRETURN((RpMaterial *) NULL);
    }

    if (NULL == RpMatFXMaterialSetBumpMapCoefficient(material, coef))
    {
        RWRETURN((RpMaterial *) NULL);
    }

    RWRETURN(material);
}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialSetupEnvMap is used to setup the various attributes
 * needed for the environment map material effect.
 *
 * \warning \ref RpMatFXMaterialSetupEnvMap should only be used after calling
 * \ref RpMatFXMaterialSetEffects with either:
 * \li rpMATFXEFFECTENVMAP
 * \li rpMATFXEFFECTBUMPENVMAP
 * as the effect type.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material            Pointer to the \ref RpMaterial to setup the
 *                            environment map effect on.
 * \param texture             Pointer to the \ref RwTexture to use as the
 *                            environment map.
 * \param frame               Pointer to the \ref RwFrame to use for the
 *                            environment map projection.
 * \param useFrameBufferAlpha \ref RwBool flag used to specify if frame buffer
 *                            alpha should be used when applying the
 *                            environment map.
 * \param coef                Specifies the environment map
 *                            coefficient. This should normally be in the
 *                            range 0 to 1. (Note: values outside this range
 *                            may be used on PlayStation 2. See the MatFX
 *                            Platform Specific section for details.)
 *
 * \return The source RpMaterial on success, NULL otherwise
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetEnvMapTexture
 * \see RpMatFXMaterialSetEnvMapFrame
 * \see RpMatFXMaterialSetEnvMapFrameBufferAlpha
 * \see RpMatFXMaterialSetEnvMapCoefficient
 * \see RpMatFXMaterialGetEnvMapTexture
 * \see RpMatFXMaterialGetEnvMapFrame
 * \see RpMatFXMaterialGetEnvMapFrameBufferAlpha
 * \see RpMatFXMaterialGetEnvMapCoefficient
 */
RpMaterial         *
RpMatFXMaterialSetupEnvMap(RpMaterial * material,
                           RwTexture * texture,
                           RwFrame * frame,
                           RwBool useFrameBufferAlpha, RwReal coef)
{
    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialSetupEnvMap"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    /* TODO[5]: MAKE THIS INTO A MACRO IN RELEASE */

    /* Lets just call the individual set functions for now. */
    if (NULL == RpMatFXMaterialSetEnvMapTexture(material, texture))
    {
        RWRETURN((RpMaterial *) NULL);
    }

    if (NULL == RpMatFXMaterialSetEnvMapFrame(material, frame))
    {
        RWRETURN((RpMaterial *) NULL);
    }

    if (NULL ==
        RpMatFXMaterialSetEnvMapFrameBufferAlpha(material,
                                                 useFrameBufferAlpha))
    {
        RWRETURN((RpMaterial *) NULL);
    }

    if (NULL == RpMatFXMaterialSetEnvMapCoefficient(material, coef))
    {
        RWRETURN((RpMaterial *) NULL);
    }

    RWRETURN(material);
}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialSetupDualTexture is used to setup the various
 * attributes needed for the dual pass material effect.
 *
 * \warning \ref RpMatFXMaterialSetupEnvMap should only be used after calling
 * \ref RpMatFXMaterialSetEffects with:
 * \li rpMATFXEFFECTDUAL
 * as the effect type.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.

 * \param material     Pointer to the \ref RpMaterial to setup the dual pass
 *                     effect on.
 * \param texture      Pointer to the \ref RwTexture to use as the dual pass
 *                     texture.
 * \param srcBlendMode \ref RwBlendFunction source blend mode to use.
 * \param dstBlendMode \ref RwBlendFunction destination blend mode to use.
 *
 * \return The source \ref RpMaterial on success, NULL otherwise
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetDualTexture
 * \see RpMatFXMaterialSetDualBlendModes
 * \see RpMatFXMaterialGetDualTexture
 * \see RpMatFXMaterialGetDualBlendModes
 */
RpMaterial         *
RpMatFXMaterialSetupDualTexture(RpMaterial * material,
                                RwTexture * texture,
                                RwBlendFunction srcBlendMode,
                                RwBlendFunction dstBlendMode )
{
    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialSetupDualTexture"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    /* TODO[5]: MAKE THIS INTO A MACRO IN RELEASE */

    /* Lets just call the individual set functions for now. */
    if (NULL == RpMatFXMaterialSetDualTexture(material, texture))
    {
        RWRETURN((RpMaterial *) NULL);
    }

    if (NULL ==
        RpMatFXMaterialSetDualBlendModes(material, srcBlendMode,
                                         dstBlendMode ))
    {
        RWRETURN((RpMaterial *) NULL);
    }

    RWRETURN(material);
}

/*--- Tinker with effects ---------------------------------------------------*/

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialGetEffects is used to get type of material effect of
 * the material set with \ref RpMatFXMaterialSetEffects.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.

 * \param material Pointer to the \ref RpMaterial.
 *
 * \return The type of effect used
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialSetupBumpMap
 * \see RpMatFXMaterialSetupEnvMap
 * \see RpMatFXMaterialSetupDualTexture
 * \see RpMatFXMaterialGetEffects
 */
RpMatFXMaterialFlags
RpMatFXMaterialGetEffects(const RpMaterial * material)
{
    const rpMatFXMaterialData *materialData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialGetEffects"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    materialData = (const rpMatFXMaterialData *)
        *MATFXMATERIALGETCONSTDATA(material);
    /* RWASSERT(RPMATFXALIGNMENT(materialData)); */

    if (NULL == materialData)
    {
        RWRETURN(rpMATFXEFFECTNULL);
    }

    RWRETURN(materialData->flags);
}

/*--- Bump Map --------------------------------------------------------------*/
/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialSetBumpMapTexture sets the bump map required for the
 * material effect.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material Pointer to the \ref RpMaterial.
 * \param bumpTexture  Pointer to the \ref RwTexture used as the bump map mask.
 *                 This texture will be merged into the alpha channel of the
 *                 with the material's base texture. The result is stored as
 *                 the texture used for bump mapping. If the pointer is NULL
 *                 then just the material's base texture will be used instead
 *                 of constructing a new texture.
 *
 * \return The material used on success, NULL otherwise.
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialSetupBumpMap
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetBumpMapFrame
 * \see RpMatFXMaterialSetBumpMapCoefficient
 * \see RpMatFXMaterialGetBumpMapTexture
 * \see RpMatFXMaterialGetBumpMapBumpedTexture
 * \see RpMatFXMaterialGetBumpMapFrame
 * \see RpMatFXMaterialGetBumpMapCoefficient
 */
RpMaterial         *
RpMatFXMaterialSetBumpMapTexture(RpMaterial * material,
                                 RwTexture * bumpTexture)
{
    MatFXBumpMapData   *bumpMapData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialSetBumpMapTexture"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    bumpMapData = MATFXBUMPMAPGETDATA(material);
    RWASSERT(bumpMapData);

    /* Do we already have an bumpTexture? */
    if (NULL != bumpMapData->bumpTexture)
    {
        /* Yes then lets get rid of it. */
        RwTextureDestroy(bumpMapData->bumpTexture);

        bumpMapData->bumpTexture = (RwTexture *)NULL;
    }

    /* Do we already have a texture? */
    if (NULL != bumpMapData->texture)
    {
        /* Yes then lets get rid of it. */
        RwTextureDestroy(bumpMapData->texture);

        bumpMapData->texture = (RwTexture *)NULL;
        bumpMapData->invBumpWidth = 0;
    }

    /* Are we be gived a texture to use? */
    if (bumpTexture)
    {
        RwBool      dummyTextures;
        RwRaster   *bumpRaster;
        RwTexture  *baseTexture;

        /* Remember the bumpmap texture */
        bumpMapData->bumpTexture = bumpTexture;
        (void)RwTextureAddRef(bumpMapData->bumpTexture);

        /* Get the bump raster and check for it being a dummy */
        bumpRaster = RwTextureGetRaster(bumpTexture);
        RWASSERT(bumpRaster);
        dummyTextures = (0 == RwRasterGetWidth(bumpRaster));

        /* Get the base texture and also check for dummy */
        baseTexture = RpMaterialGetTexture(material);
        if (!dummyTextures && baseTexture)
        {
            RwRaster   *baseRaster;
            baseRaster = RwTextureGetRaster(baseTexture);
            RWASSERT(baseRaster);
            dummyTextures = (0 == RwRasterGetWidth(baseRaster));
        }

        /* Only setup bumped texture if we don't have dummy input textures.
         * These are probably from art tools and we assume that a
         * bumped texture will get generated on a later stream read.
         */
        if (!dummyTextures)
        {
            RwChar bumpedName[rwTEXTUREBASENAMELENGTH] = { 0 };
            RwTexDictionary *dict;
            RwRaster    *bumpedRaster;

            /* First try to find the bumped texture in the current texture
             * dictionary. We need its bumped name to do this.
             * Handles null baseTexture
             */
            GenBumpedTextureName(bumpedName, baseTexture, bumpTexture);

            dict = RwTexDictionaryGetCurrent();
            bumpMapData->texture = (RwTexture *) NULL;
            if (dict)
            {
                bumpMapData->texture =
                    RwTexDictionaryFindNamedTexture(dict, bumpedName);
            }

            if (!bumpMapData->texture)
            {
                /* Failed to find it so generate it.
                 * This handles null baseTexture.
                 */
                bumpMapData->texture = _rpMatFXSetupBumpMapTexture(baseTexture,
                                                                   bumpTexture);
                if (!bumpMapData->texture)
                {
                    RWRETURN((RpMaterial *) NULL);
                }

                /* And add it to texture dictionary so it'll be streamed
                 * if the texture dictionary is
                 */
                if (dict)
                {
                    RwTexDictionaryAddTexture( dict, bumpMapData->texture );
                }
            }
            else
            {
                (void)RwTextureAddRef(bumpMapData->texture);
            }

            /* Lets grab the raster to work out the invBumpWidth. */
            bumpedRaster = RwTextureGetRaster(bumpMapData->texture);
            RWASSERT(bumpedRaster);

            /* Grab it's width and calculate the invBumpWidth. */
            bumpMapData->invBumpWidth =
                1.0f / ((RwReal) RwRasterGetWidth(bumpedRaster));
        }
    }
    else
    {
        /* No bump map. Assume that the base texture
         * in the material is to be used as the bumped texture.
         * Note: this is currently only supported on PS2. (22/08/02)
         */
        RwTexture  *baseTexture;
        RwRaster   *raster;

        baseTexture = RpMaterialGetTexture(material);
        RWASSERT(baseTexture);
        raster = RwTextureGetRaster(baseTexture);
        RWASSERT(raster);

        /* Grab it's width and calculate the invBumpWidth. */
        bumpMapData->invBumpWidth =
            1.0f / ((RwReal) RwRasterGetWidth(raster));
    }

    RWRETURN(material);
}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialSetBumpMapFrame sets the frame associated with the bump
 *  mapping material effect.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material Pointer to the \ref RpMaterial.
 * \param frame    \ref RwFrame affecting the bump map.
 *
 * \return The material used on success, NULL otherwise.
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialSetupBumpMap
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetBumpMapTexture
 * \see RpMatFXMaterialSetBumpMapCoefficient
 * \see RpMatFXMaterialGetBumpMapTexture
 * \see RpMatFXMaterialGetBumpMapBumpedTexture
 * \see RpMatFXMaterialGetBumpMapFrame
 * \see RpMatFXMaterialGetBumpMapCoefficient
 */
RpMaterial         *
RpMatFXMaterialSetBumpMapFrame(RpMaterial * material, RwFrame * frame)
{
    MatFXBumpMapData   *bumpMapData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialSetBumpMapFrame"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    bumpMapData = MATFXBUMPMAPGETDATA(material);
    RWASSERT(bumpMapData);

    bumpMapData->frame = frame;

    RWRETURN(material);
}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialSetBumpMapCoefficient controls the amount
 * of the bump effect applied to the material (the bumpiness). Useful
 * values typically lie in the range -10 to 10, depending on the
 * scale of the bumps, and the depth of the effect required.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material Pointer to the material.
 * \param coef     Bump coefficient.
 *
 * \return The material used on success, NULL otherwise.
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialSetupBumpMap
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetBumpMapTexture
 * \see RpMatFXMaterialSetBumpMapFrame
 * \see RpMatFXMaterialGetBumpMapTexture
 * \see RpMatFXMaterialGetBumpMapBumpedTexture
 * \see RpMatFXMaterialGetBumpMapFrame
 * \see RpMatFXMaterialGetBumpMapCoefficient
 */
RpMaterial         *
RpMatFXMaterialSetBumpMapCoefficient(RpMaterial * material, RwReal coef)
{
    MatFXBumpMapData   *bumpMapData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialSetBumpMapCoefficient"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    bumpMapData = MATFXBUMPMAPGETDATA(material);
    RWASSERT(bumpMapData);

    /* Store the coefficient: should this be negative? */
    bumpMapData->coef = -coef;

    RWRETURN(material);
}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialGetBumpMapBumpedTexture is used to retrieve the texture
 * used for the bump mapping material effect. If a texture was set with
 * \ref RpMatFXMaterialSetBumpMapTexture then the resulting constructed
 * texture is returned.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material Pointer to the \ref RpMaterial.
 *
 * \return The bump map texture used on success, NULL otherwise.
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialSetupBumpMap
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetBumpMapTexture
 * \see RpMatFXMaterialSetBumpMapFrame
 * \see RpMatFXMaterialSetBumpMapCoefficient
 * \see RpMatFXMaterialGetBumpMapTexture
 * \see RpMatFXMaterialGetBumpMapFrame
 * \see RpMatFXMaterialGetBumpMapCoefficient
 */
RwTexture          *
RpMatFXMaterialGetBumpMapBumpedTexture(const RpMaterial * material)
{
    const MatFXBumpMapData *bumpMapData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialGetBumpMapBumpedTexture"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    bumpMapData = MATFXBUMPMAPGETCONSTDATA(material);
    RWASSERT(bumpMapData);

    RWRETURN(bumpMapData->texture);
}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialGetBumpMapTexture is used to retrieve the texture
 * set with \ref RpMatFXMaterialSetBumpMapTexture.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material Pointer to the \ref RpMaterial.
 *
 * \return The bump map texture used on success, NULL otherwise.
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialSetupBumpMap
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetBumpMapTexture
 * \see RpMatFXMaterialSetBumpMapFrame
 * \see RpMatFXMaterialSetBumpMapCoefficient
 * \see RpMatFXMaterialGetBumpMapBumpedTexture
 * \see RpMatFXMaterialGetBumpMapFrame
 * \see RpMatFXMaterialGetBumpMapCoefficient
 */
RwTexture          *
RpMatFXMaterialGetBumpMapTexture(const RpMaterial * material)
{
    const MatFXBumpMapData *bumpMapData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialGetBumpMapTexture"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    bumpMapData = MATFXBUMPMAPGETCONSTDATA(material);
    RWASSERT(bumpMapData);

    RWRETURN(bumpMapData->bumpTexture);
}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialGetBumpMapFrame is used to retrieve the frame used
 * for the bump map material effect.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material Pointer to the \ref RpMaterial.
 *
 * \return The bump map frame used on success, NULL otherwise.
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialSetupBumpMap
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetBumpMapTexture
 * \see RpMatFXMaterialSetBumpMapFrame
 * \see RpMatFXMaterialSetBumpMapCoefficient
 * \see RpMatFXMaterialGetBumpMapTexture
 * \see RpMatFXMaterialGetBumpMapBumpedTexture
 * \see RpMatFXMaterialGetBumpMapCoefficient
 */
RwFrame            *
RpMatFXMaterialGetBumpMapFrame(const RpMaterial * material)
{
    const MatFXBumpMapData *bumpMapData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialGetBumpMapFrame"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    bumpMapData = MATFXBUMPMAPGETCONSTDATA(material);
    RWASSERT(bumpMapData);

    RWRETURN(bumpMapData->frame);
}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialGetBumpMapCoefficient is used to retrieve the bump
 * mapping coefficient.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material A pointer to a \ref RpMaterial.
 *
 * \return Returns bump map coefficient value.
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialSetupBumpMap
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetBumpMapTexture
 * \see RpMatFXMaterialSetBumpMapFrame
 * \see RpMatFXMaterialSetBumpMapCoefficient
 * \see RpMatFXMaterialGetBumpMapTexture
 * \see RpMatFXMaterialGetBumpMapBumpedTexture
 * \see RpMatFXMaterialGetBumpMapFrame
 */
RwReal
RpMatFXMaterialGetBumpMapCoefficient(const RpMaterial * material)
{
    const MatFXBumpMapData *bumpMapData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialGetBumpMapCoefficient"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    bumpMapData = MATFXBUMPMAPGETCONSTDATA(material);
    RWASSERT(bumpMapData);

    RWRETURN(-bumpMapData->coef);
}

/*--- Env Map ---------------------------------------------------------------*/

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialSetEnvMapTexture sets the texture for the environment
 * mapping material effect. The texture is rendered with dynamically generated
 * UVs using a spherical environment mapping.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material Pointer to the \ref RpMaterial.
 * \param texture  Pointer to the \ref RwTexture for the environment map.
 *
 * \return The material used on success, NULL otherwise.
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialSetupEnvMap
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetEnvMapFrame
 * \see RpMatFXMaterialSetEnvMapFrameBufferAlpha
 * \see RpMatFXMaterialSetEnvMapCoefficient
 * \see RpMatFXMaterialGetEnvMapTexture
 * \see RpMatFXMaterialGetEnvMapFrame
 * \see RpMatFXMaterialGetEnvMapFrameBufferAlpha
 * \see RpMatFXMaterialGetEnvMapCoefficient
 */
RpMaterial         *
RpMatFXMaterialSetEnvMapTexture(RpMaterial * material,
                                RwTexture * texture)
{
    MatFXEnvMapData    *envMapData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialSetEnvMapTexture"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);
    RWASSERT(texture);

    envMapData = MATFXENVMAPGETDATA(material);
    RWASSERT(envMapData);

    /* Add texture reference. */
    RwTextureAddRef(texture);

    /* Do we already have an envTexture? */
    if (NULL != envMapData->texture)
    {
        /* Yes then lets get rid of it. */
        RwTextureDestroy(envMapData->texture);

        envMapData->texture = (RwTexture *)NULL;
    }

    /* Store the texture. */
    envMapData->texture = texture;

    RWRETURN(material);
}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialSetEnvMapFrame sets the frame associated with the
 * environment mapping material effect. This will affect the position of
 * the environment map on the object.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material Pointer to the \ref RpMaterial.
 * \param frame    Pointer to the \ref RwFrame affecting environment mapping.
 *
 * \return The material used on success, NULL otherwise.
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialSetupEnvMap
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetEnvMapTexture
 * \see RpMatFXMaterialSetEnvMapFrameBufferAlpha
 * \see RpMatFXMaterialSetEnvMapCoefficient
 * \see RpMatFXMaterialGetEnvMapTexture
 * \see RpMatFXMaterialGetEnvMapFrame
 * \see RpMatFXMaterialGetEnvMapFrameBufferAlpha
 * \see RpMatFXMaterialGetEnvMapCoefficient
 */
RpMaterial         *
RpMatFXMaterialSetEnvMapFrame(RpMaterial * material, RwFrame * frame)
{
    MatFXEnvMapData    *envMapData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialSetEnvMapFrame"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    envMapData = MATFXENVMAPGETDATA(material);
    RWASSERT(envMapData);

    envMapData->frame = frame;

    RWRETURN(material);
}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialSetEnvMapFrameBufferAlpha determines whether the
 * environment map will be multiplied by frame buffer alpha before
 * being added to the rendered object. This allows for effects (for
 * example bump mapping) to attenuate the environment map on a
 * per-pixel basis, lending the rendered surface a detailed variation
 * in appearance.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material            Pointer to the \ref RpMaterial.
 * \param useFrameBufferAlpha \ref RwBool to set the flag.
 * \li TRUE to use frame buffer alpha,
 * \li FALSE to ignore it.
 *
 * \return The material used on success, NULL otherwise.
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialSetupEnvMap
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetEnvMapTexture
 * \see RpMatFXMaterialSetEnvMapFrame
 * \see RpMatFXMaterialSetEnvMapCoefficient
 * \see RpMatFXMaterialGetEnvMapTexture
 * \see RpMatFXMaterialGetEnvMapFrame
 * \see RpMatFXMaterialGetEnvMapFrameBufferAlpha
 * \see RpMatFXMaterialGetEnvMapCoefficient
 */
RpMaterial         *
RpMatFXMaterialSetEnvMapFrameBufferAlpha(RpMaterial * material,
                                         RwBool useFrameBufferAlpha)
{
    MatFXEnvMapData    *envMapData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialSetEnvMapFrameBufferAlpha"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    envMapData = MATFXENVMAPGETDATA(material);
    RWASSERT(envMapData);

    envMapData->useFrameBufferAlpha = useFrameBufferAlpha;

    _rpMatFXMaterialPlatformEnvMapSetBlend(material);

    RWRETURN(material);
}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialSetEnvMapCoefficient affects the reflectivity of the
 * material, i.e. the intensity of the environment map (which is rendered
 * additively on top of the material). The value should normally be in the
 * range 0 to 1, but note that values outside this range may be used on
 * PlayStation 2 (see the MatFX Platform Specific section for details).
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material Pointer to the \ref RpMaterial.
 * \param coef     \ref RwReal representing the reflection coefficient of
 *                 the material
 *
 * \return The material used on success, NULL otherwise.
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialSetupEnvMap
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetEnvMapTexture
 * \see RpMatFXMaterialSetEnvMapFrame
 * \see RpMatFXMaterialSetEnvMapFrameBufferAlpha
 * \see RpMatFXMaterialGetEnvMapTexture
 * \see RpMatFXMaterialGetEnvMapFrame
 * \see RpMatFXMaterialGetEnvMapFrameBufferAlpha
 * \see RpMatFXMaterialGetEnvMapCoefficient
 */
RpMaterial         *
RpMatFXMaterialSetEnvMapCoefficient(RpMaterial * material, RwReal coef)
{
    MatFXEnvMapData    *envMapData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialSetEnvMapCoefficient"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    envMapData = MATFXENVMAPGETDATA(material);
    RWASSERT(envMapData);

    envMapData->coef = coef;

    RWRETURN(material);
}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialGetEnvMapTexture is used to retrieve the texture used
 * for the environment map effect.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material A pointer to a \ref RpMaterial.
 *
 * \return Returns a pointer to the environment map's \ref RwTexture.
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialSetupEnvMap
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetEnvMapTexture
 * \see RpMatFXMaterialSetEnvMapFrame
 * \see RpMatFXMaterialSetEnvMapFrameBufferAlpha
 * \see RpMatFXMaterialSetEnvMapCoefficient
 * \see RpMatFXMaterialGetEnvMapFrame
 * \see RpMatFXMaterialGetEnvMapFrameBufferAlpha
 * \see RpMatFXMaterialGetEnvMapCoefficient
 */
RwTexture          *
RpMatFXMaterialGetEnvMapTexture(const RpMaterial * material)
{
    const MatFXEnvMapData *envMapData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialGetEnvMapTexture"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    envMapData = MATFXENVMAPGETCONSTDATA(material);
    RWASSERT(envMapData);

    RWRETURN(envMapData->texture);
}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialGetEnvMapFrame is used to retrieve the \ref RwFrame
 * used for projecting the environment map.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material A pointer to a \ref RpMaterial.
 *
 * \return Returns a pointer to the environment map's \ref RwFrame.
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialSetupEnvMap
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetEnvMapTexture
 * \see RpMatFXMaterialSetEnvMapFrame
 * \see RpMatFXMaterialSetEnvMapFrameBufferAlpha
 * \see RpMatFXMaterialSetEnvMapCoefficient
 * \see RpMatFXMaterialGetEnvMapTexture
 * \see RpMatFXMaterialGetEnvMapFrameBufferAlpha
 * \see RpMatFXMaterialGetEnvMapCoefficient
 */
RwFrame            *
RpMatFXMaterialGetEnvMapFrame(const RpMaterial * material)
{
    const MatFXEnvMapData *envMapData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialGetEnvMapFrame"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    envMapData = MATFXENVMAPGETCONSTDATA(material);
    RWASSERT(envMapData);

    RWRETURN(envMapData->frame);
}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialGetEnvMapFrameBufferAlpha is used to retreive
 * the useFrameBufferAlpha flag.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material A pointer to a \ref RpMaterial.
 *
 * \return Returns the useFrameBufferAlpha flag of the material.
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialSetupEnvMap
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetEnvMapTexture
 * \see RpMatFXMaterialSetEnvMapFrame
 * \see RpMatFXMaterialSetEnvMapFrameBufferAlpha
 * \see RpMatFXMaterialSetEnvMapCoefficient
 * \see RpMatFXMaterialGetEnvMapTexture
 * \see RpMatFXMaterialGetEnvMapFrame
 * \see RpMatFXMaterialGetEnvMapCoefficient
 */
RwBool
RpMatFXMaterialGetEnvMapFrameBufferAlpha(const RpMaterial * material)
{
    const MatFXEnvMapData *envMapData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialGetEnvMapFrameBufferAlpha"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    envMapData = MATFXENVMAPGETCONSTDATA(material);
    RWASSERT(envMapData);

    RWRETURN(envMapData->useFrameBufferAlpha);
}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialGetEnvMapCoefficient is used to retrieve the
 * environment coefficient.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material A pointer to a \ref RpMaterial.
 *
 * \return Returns environment coefficient value.
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialSetupEnvMap
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetEnvMapTexture
 * \see RpMatFXMaterialSetEnvMapFrame
 * \see RpMatFXMaterialSetEnvMapFrameBufferAlpha
 * \see RpMatFXMaterialSetEnvMapCoefficient
 * \see RpMatFXMaterialGetEnvMapTexture
 * \see RpMatFXMaterialGetEnvMapFrame
 * \see RpMatFXMaterialGetEnvMapFrameBufferAlpha
 */
RwReal
RpMatFXMaterialGetEnvMapCoefficient(const RpMaterial * material)
{
    const MatFXEnvMapData *envMapData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialGetEnvMapCoefficient"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    envMapData = MATFXENVMAPGETCONSTDATA(material);
    RWASSERT(envMapData);

    RWRETURN(envMapData->coef);
}

/*--- Dual Pass -------------------------------------------------------------*/
/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialSetDualTexture sets the texture used in the second pass
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material Pointer to the \ref RpMaterial.
 * \param texture  Pointer to the \ref RwTexture to use in the second pass.
 *
 * \return The material used on success, NULL otherwise.
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialSetupDualTexture
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetDualBlendModes
 * \see RpMatFXMaterialGetDualTexture
 * \see RpMatFXMaterialGetDualBlendModes
 */
RpMaterial         *
RpMatFXMaterialSetDualTexture(RpMaterial * material,
                              RwTexture * texture)
{
    MatFXDualData      *dualData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialSetDualTexture"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);
    RWASSERT(texture);

    dualData = MATFXDUALGETDATA(material);
    RWASSERT(dualData);

    /* Add texture reference. */
    RwTextureAddRef(texture);

    /* Do we already have an dual texture? */
    if (NULL != dualData->texture)
    {
        /* Yes then lets get rid of it. */
        RwTextureDestroy(dualData->texture);

        dualData->texture = (RwTexture *)NULL;
    }

    /* Store the texture. */
    dualData->texture = texture;

    _rpMatFXSetupDualRenderState(dualData, rwRENDERSTATETEXTUREADDRESS);
    _rpMatFXSetupDualRenderState(dualData, rwRENDERSTATETEXTURERASTER);

    RWRETURN(material);

}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialSetDualBlendModes is used to set the source and
 * destination blend modes used in the dual pass material effect.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material     A pointer to a \ref RpMaterial.
 * \param srcBlendMode A pointer to a \ref RwBlendFunction used to set the
 *                     source blend mode.
 * \param dstBlendMode A pointer to a \ref RwBlendFunction used to set the
 *                     destination blend mode.
 *
 * \return The material used on success, NULL otherwise.
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialSetupDualTexture
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetDualTexture
 * \see RpMatFXMaterialGetDualTexture
 * \see RpMatFXMaterialGetDualBlendModes
 */
RpMaterial         *
RpMatFXMaterialSetDualBlendModes(RpMaterial * material,
                                 RwBlendFunction srcBlendMode,
                                 RwBlendFunction dstBlendMode )
{
    MatFXDualData      *dualData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialSetDualBlendModes"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    dualData = MATFXDUALGETDATA(material);
    RWASSERT(dualData);

    dualData->srcBlendMode = srcBlendMode;
    dualData->dstBlendMode = dstBlendMode;

    _rpMatFXSetupDualRenderState(dualData, rwRENDERSTATESRCBLEND);
    _rpMatFXSetupDualRenderState(dualData, rwRENDERSTATEDESTBLEND);

    _rpMatFXMaterialPlatformDualSetBlend(material);

    RWRETURN(material);
}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialGetDualTexture is used to retrieve the texture
 * used in the dual pass material effect.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material A pointer to a \ref RpMaterial.
 *
 * \return The dual pass texture on success, NULL otherwise
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialSetupDualTexture
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetDualTexture
 * \see RpMatFXMaterialSetDualBlendModes
 * \see RpMatFXMaterialGetDualBlendModes
 */
RwTexture          *
RpMatFXMaterialGetDualTexture(const RpMaterial * material)
{
    const MatFXDualData *dualData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialGetDualTexture"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    dualData = MATFXDUALGETCONSTDATA(material);
    RWASSERT(dualData);

    RWRETURN(dualData->texture);
}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialGetDualBlendModes is used to retrieve the source
 * and destination blend modes used in the dual pass material effect.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material     A pointer to a \ref RpMaterial.
 * \param srcBlendMode A pointer to a \ref RwBlendFunction used to retrieve
 *                     the srource blend mode.
 * \param dstBlendMode A pointer to a \ref RwBlendFunction used to retrieve
 *                     the destination blend mode.
 *
 * \return The material used on success, NULL otherwise.
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialSetupDualTexture
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetDualTexture
 * \see RpMatFXMaterialSetDualBlendModes
 * \see RpMatFXMaterialGetDualTexture
 */
const RpMaterial *
RpMatFXMaterialGetDualBlendModes(const RpMaterial * material,
                                 RwBlendFunction *srcBlendMode,
                                 RwBlendFunction *dstBlendMode )
{
    const MatFXDualData *dualData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialGetDualBlendModes"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    dualData = MATFXDUALGETCONSTDATA(material);
    RWASSERT(dualData);

    *srcBlendMode = dualData->srcBlendMode;
    *dstBlendMode = dualData->dstBlendMode;

    RWRETURN(material);
}

/*--- UV Transform Pass ----------------------------------------------------*/
/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialSetUVTransformMatrices is used to set the base pass and
 * dual pass texture coordinate transform matrices. If the base effect is set
 * to \ref rpMATFXEFFECTUVTRANSFORM then the dualTransform should be set to
 * NULL.
 *
 * The transform matrices allow 2D rotation, scaling, and translation
 * of the texture coordines. For this purpose, only a 6-component subset of
 * the \ref RwMatrix values are used when calculating the transformed texture
 * coordinates, as follows:
 *
 * \verbatim
   (matrix) | rx ry - | (right)
            | ux uy - | (up)
            | -  -  - | (at)
            | px py - | (pos) \endverbatim
 *
 * The transformed UV texture coordinates will be:
 *
 * \verbatim
   u' = rx * u + ux * v + px
   v' = ry * u + uy * v + py \endverbatim
 *
 * Such matrices may be constructed directly, or by using the
 * \ref RwMatrixRotate (about z-axis), \ref RwMatrixTranslate, and
 * \ref RwMatrixScale functions. UV animation can be achieved by modifying
 * the matrix values each frame.
 *
 * If either of the transform matrix pointers are set to NULL, then an
 * identity transform will be assumed.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material      Pointer to a \ref RpMaterial.
 * \param baseTransform Pointer to the base pass UV transform matrix.
 * \param dualTransform Pointer to the dual pass UV transform matrix.
 *
 * \return The material used on success, NULL otherwise.
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialGetUVTransformMatrices
 * \see RpMatFXMaterialSetDualTexture
 * \see RpMatFXMaterialGetDualTexture
 * \see RpMatFXMaterialGetDualBlendModes
 * \see RpMatFXMaterialSetDualBlendModes
 */
RpMaterial *
RpMatFXMaterialSetUVTransformMatrices( RpMaterial *material,
                                       RwMatrix *baseTransform,
                                       RwMatrix *dualTransform )
{
    MatFXUVAnimData *uvAnimData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialSetUVTransformMatrices"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    uvAnimData = MATFXUVANIMGETDATA(material);
    RWASSERT(uvAnimData);

    uvAnimData->baseTransform = baseTransform;
    uvAnimData->dualTransform = dualTransform;

    RWRETURN(material);
}

/**
 * \ingroup rpmatfx
 * \ref RpMatFXMaterialGetUVTransformMatrices is used to retrieve the base
 * pass and dual pass texture coordinate transform matrices.
 *
 * The world and material effect plugins must be attached before using
 * this function. The header file rpmatfx.h is required.
 *
 * \param material      Pointer to a \ref RpMaterial.
 * \param baseTransform Pointer to retrieve the base matrix transform.
 * \param dualTransform Pointer to retrieve the dual matrix transform.
 *
 * \return The material used on success, NULL otherwise.
 *
 * \see RpMatFXMaterialSetEffects
 * \see RpMatFXMaterialGetEffects
 * \see RpMatFXMaterialSetUVTransformMatrices
 * \see RpMatFXMaterialSetDualTexture
 * \see RpMatFXMaterialGetDualTexture
 * \see RpMatFXMaterialGetDualBlendModes
 * \see RpMatFXMaterialSetDualBlendModes
 */
const RpMaterial *
RpMatFXMaterialGetUVTransformMatrices( const RpMaterial *material,
                                       RwMatrix **baseTransform,
                                       RwMatrix **dualTransform )

{
    const MatFXUVAnimData *uvAnimData;

    RWAPIFUNCTION(RWSTRING("RpMatFXMaterialGetUVTransformMatrices"));
    RWASSERT(MatFXInfo.Module.numInstances);
    RWASSERT(material);

    uvAnimData = MATFXUVANIMGETCONSTDATA(material);
    RWASSERT(uvAnimData);

    if(NULL != baseTransform)
    {
        *baseTransform = uvAnimData->baseTransform;
    }

    if(NULL != dualTransform)
    {
        *dualTransform = uvAnimData->dualTransform;
    }

    RWRETURN(material);
}
