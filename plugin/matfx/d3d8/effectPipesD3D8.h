#ifndef EFFECTPIPESD3D8_H
#define EFFECTPIPESD3D8_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

/*===========================================================================*
 *--- Defines ---------------------------------------------------------------*
 *===========================================================================*/

#define MATFXD3D8ENVMAPGETDATA(material, pass)                          \
  &((*((rpMatFXMaterialData **)                                         \
       (((RwUInt8 *)material) +                                         \
        MatFXMaterialDataOffset)))->data[pass].data.envMap)

#define MATFXD3D8BUMPMAPGETDATA(material)                               \
  &((*((rpMatFXMaterialData **)                                         \
       (((RwUInt8 *)material) +                                         \
        MatFXMaterialDataOffset)))->data[rpSECONDPASS].data.bumpMap)

#define MATFXD3D8DUALGETDATA(material)                                  \
  &((*((rpMatFXMaterialData **)                                         \
       (((RwUInt8 *)material) +                                         \
        MatFXMaterialDataOffset)))->data[rpSECONDPASS].data.dual)

#define MATFXD3D8UVANIMGETDATA(material)  \
  &((*((rpMatFXMaterialData **)             \
       (((RwUInt8 *)material) +             \
        MatFXMaterialDataOffset)))->data[rpSECONDPASS].data.uvAnim)

#define MATFXD3D8DUALUVANIMGETDUALDATA(material)  \
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
extern RwBool _rwD3D8TextureHasAlpha(const RwTexture *texture);

extern RwBool _rwD3D8RasterIsCubeRaster(const RwRaster *raster);

extern void _rwD3D8RenderStateFlushCache(void);

extern RwBool _rwD3D8RenderStateVertexAlphaEnable(RwBool enable);
extern RwBool _rwD3D8RenderStateIsVertexAlphaEnable(void);

extern RwBool _rwD3D8RenderStateSrcBlend(RwBlendFunction srcBlend);
extern RwBool _rwD3D8RenderStateDestBlend(RwBlendFunction dstBlend);

/*===========================================================================*
 *--- Public Global Variables -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Functions -------------------------------------------------------------*
 *===========================================================================*/

extern void
_rwD3D8AtomicMatFXRenderCallback(RwResEntry *repEntry,
                              void *object,
                              RwUInt8 type,
                              RwUInt32 flags);

extern void
_rpMatFXD3D8AtomicMatFXDefaultRender(RxD3D8InstanceData *instancedData,
                                     RwUInt32 flags,
                                     RwTexture *baseTexture);

extern void
_rpMatFXD3D8AtomicMatFXRenderBlack(RxD3D8InstanceData *instancedData);

extern void
_rpMatFXD3D8AtomicMatFXDualPassRender(RxD3D8InstanceData *instancedData,
                                      RwUInt32 flags,
                                      RwTexture *baseTexture,
                                      RwTexture *dualPassTexture);

extern void
_rpMatFXD3D8AtomicMatFXEnvRender(RxD3D8InstanceData *instancedData,
                                 RwUInt32 flags,
                                 RwUInt32 pass,
                                 RwTexture *baseTexture,
                                 RwTexture *envMapTexture);

extern void
_rpMatFXD3D8AtomicMatFXBumpMapRender(RxD3D8InstanceData *instancedData,
                                     RwUInt32 flags,
                                     RwTexture *baseTexture,
                                     RwTexture *bumpTexture,
                                     RwTexture *envMapTexture);

extern RwBool
_rwD3D8MaterialMatFXHasBumpMap(const RpMaterial *material);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* EFFECTPIPESD3D8_H */
