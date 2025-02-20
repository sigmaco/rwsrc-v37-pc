#ifndef EFFECTPIPESD3D9_H
#define EFFECTPIPESD3D9_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

/*===========================================================================*
 *--- Defines ---------------------------------------------------------------*
 *===========================================================================*/

#define MATFXD3D9ENVMAPGETDATA(material, pass)                          \
  &((*((rpMatFXMaterialData **)                                         \
       (((RwUInt8 *)material) +                                         \
        MatFXMaterialDataOffset)))->data[pass].data.envMap)

#define MATFXD3D9BUMPMAPGETDATA(material)                               \
  &((*((rpMatFXMaterialData **)                                         \
       (((RwUInt8 *)material) +                                         \
        MatFXMaterialDataOffset)))->data[rpSECONDPASS].data.bumpMap)

#define MATFXD3D9DUALGETDATA(material)                                  \
  &((*((rpMatFXMaterialData **)                                         \
       (((RwUInt8 *)material) +                                         \
        MatFXMaterialDataOffset)))->data[rpSECONDPASS].data.dual)

#define MATFXD3D9UVANIMGETDATA(material)  \
  &((*((rpMatFXMaterialData **)             \
       (((RwUInt8 *)material) +             \
        MatFXMaterialDataOffset)))->data[rpSECONDPASS].data.uvAnim)

#define MATFXD3D9DUALUVANIMGETDUALDATA(material)  \
  &((*((rpMatFXMaterialData **)                     \
       (((RwUInt8 *)material) +                     \
        MatFXMaterialDataOffset)))->data[rpTHIRDPASS].data.dual)

/*===========================================================================*
 *--- Types -----------------------------------------------------------------*
 *===========================================================================*/
struct MatFXDualData
{
    RwTexture          *texture;
    RwBlendFunction     srcBlendMode;
    RwBlendFunction     dstBlendMode;

    /*--- device specific ---*/
};

struct MatFXBumpMapData
{
    RwFrame   *frame;
    RwTexture *texture;
    RwTexture *bumpTexture;
    RwReal    coef;
    RwReal    invBumpWidth;
};
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*===========================================================================*
 *--- Public Global Functions -----------------------------------------------*
 *===========================================================================*/

/* internal driver functions */
extern RwBool _rwD3D9TextureHasAlpha(const RwTexture *texture);

extern RwBool _rwD3D9RasterIsCubeRaster(const RwRaster *raster);

extern void _rwD3D9RenderStateFlushCache(void);

extern RwBool _rwD3D9RenderStateVertexAlphaEnable(RwBool enable);
extern RwBool _rwD3D9RenderStateIsVertexAlphaEnable(void);

extern RwBool _rwD3D9RenderStateSrcBlend(RwBlendFunction srcBlend);
extern RwBool _rwD3D9RenderStateDestBlend(RwBlendFunction dstBlend);

/*===========================================================================*
 *--- Public Global Variables -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Functions -------------------------------------------------------------*
 *===========================================================================*/

extern void
_rwD3D9AtomicMatFXRenderCallback(RwResEntry *repEntry,
                                 void *object,
                                 RwUInt8 type,
                                 RwUInt32 flags);

extern void
_rpMatFXD3D9AtomicMatFXDefaultRender(RxD3D9ResEntryHeader *resEntryHeader,
                                     RxD3D9InstanceData *instancedData,
                                     RwUInt32 flags,
                                     RwTexture *baseTexture);

extern void
_rpMatFXD3D9AtomicMatFXRenderBlack(RxD3D9ResEntryHeader *resEntryHeader,
                                   RxD3D9InstanceData *instancedData);

extern void
_rpMatFXD3D9AtomicMatFXDualPassRender(RxD3D9ResEntryHeader *resEntryHeader,
                                      RxD3D9InstanceData *instancedData,
                                      RwUInt32 flags,
                                      RwTexture *baseTexture,
                                      RwTexture *dualPassTexture);

extern void
_rpMatFXD3D9AtomicMatFXEnvRender(RxD3D9ResEntryHeader *resEntryHeader,
                                 RxD3D9InstanceData *instancedData,
                                 RwUInt32 flags,
                                 RwUInt32 pass,
                                 RwTexture *baseTexture,
                                 RwTexture *envMapTexture);

extern void
_rpMatFXD3D9AtomicMatFXBumpMapRender(RxD3D9ResEntryHeader *resEntryHeader,
                                     RxD3D9InstanceData *instancedData,
                                     RwUInt32 flags,
                                     RwTexture *baseTexture,
                                     RwTexture *bumpTexture,
                                     RwTexture *envMapTexture);

extern RwBool
_rwD3D9MaterialMatFXHasBumpMap(const RpMaterial *material);

/* Vertex shader pipeline */

extern void
_rpMatFXD3D9VertexShaderAtomicEnvRender(RxD3D9ResEntryHeader *resEntryHeader,
                                        RxD3D9InstanceData *instancedData,
                                        RwTexture *baseTexture,
                                        RwTexture *envMapTexture,
                                        const _rpD3D9VertexShaderDescriptor  *desc);

extern void
_rpMatFXD3D9VertexShaderAtomicDualRender(RxD3D9ResEntryHeader *resEntryHeader,
                                         RxD3D9InstanceData *instancedData,
                                         RwTexture *baseTexture,
                                         RwTexture *dualPassTexture,
                                         const _rpD3D9VertexShaderDescriptor  *desc,
                                         const _rpD3D9VertexShaderDispatchDescriptor *dispatch);

extern void
_rpMatFXD3D9VertexShaderAtomicBumpMapRender(RxD3D9ResEntryHeader *resEntryHeader,
                                            RxD3D9InstanceData *instancedData,
                                            RwTexture *baseTexture,
                                            RwTexture *bumpTexture,
                                            RwTexture *envMapTexture,
                                            const _rpD3D9VertexShaderDescriptor  *desc,
                                            const _rpD3D9VertexShaderDispatchDescriptor *dispatch);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* EFFECTPIPESD3D9_H */
