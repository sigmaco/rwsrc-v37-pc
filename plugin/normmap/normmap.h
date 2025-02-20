/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   normmap.h                                                  -*
 *-                                                                         -*
 *-  Purpose :   Normal Maps plugin internal API                            -*
 *-                                                                         -*
 *===========================================================================*/

#ifndef NORMMAP_H
#define NORMMAP_H

#include "rwcore.h"
#include "rpworld.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

struct NormalMapExt
{
    RwTexture *normalMap;
    RwTexture *envMap;
    RwReal shininess;
    RwFrame *envmapFrame;
    RwBool modulateEnvMap;
};
typedef struct NormalMapExt NormalMapExt;

#define MATERIALGETNORMALMAP(_material) \
    ((NormalMapExt *)(((RwUInt8 *)(_material)) + _rpNormMapMaterialExOffset))

#define CONSTMATERIALGETNORMALMAP(_material) \
    ((const NormalMapExt *)(((const RwUInt8 *)(_material)) + _rpNormMapMaterialExOffset))

#define OBJECTGETNORMALMAP(object, _offset) \
    ((NormalMapExt *)(((RwUInt8 *)(object)) + _offset))

#define CONSTOBJECTGETNORMALMAP(object, _offset) \
    ((const NormalMapExt *)(((const RwUInt8 *)(object)) + _offset))

struct ObjectNormalMapExt
{
    RwBool enabled;
};
typedef struct ObjectNormalMapExt ObjectNormalMapExt;

#define WORLDGETNORMALMAP(_material) \
    ((ObjectNormalMapExt *)(((RwUInt8 *)(_material)) + _rpNormMapWorldExOffset))

#define CONSTWORLDGETNORMALMAP(_material) \
    ((const ObjectNormalMapExt *)(((const RwUInt8 *)(_material)) + _rpNormMapWorldExOffset))

struct NormalMapMatrixTransposed
{
    RwReal right_x, up_x, at_x, pos_x;
    RwReal right_y, up_y, at_y, pos_y;
    RwReal right_z, up_z, at_z, pos_z;
    RwReal right_w, up_w, at_w, pos_w;
};
typedef struct NormalMapMatrixTransposed NormalMapMatrixTransposed;

/*===========================================================================*
 *--- Private Variables -----------------------------------------------------*
 *===========================================================================*/

extern RwInt32 _rpNormMapMaterialExOffset;
extern RwInt32 _rpNormMapWorldExOffset;

extern RxPipeline  *_rpNormMapWorldSectorPipeline;

extern RpLight  *_rpNormMapMainLightActive;
extern RpLight  *_rpNormMapAmbientLightActive;

/*===========================================================================*
 *--- Private Functions -----------------------------------------------------*
 *===========================================================================*/

#ifdef    __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

extern void *
_rpNormMapOpen(void *instance,
              RwInt32 offset,
              RwInt32 size);

extern void *
_rpNormMapClose(void *instance,
           RwInt32 offset,
           RwInt32 size);

#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#endif /* NORMMAP_H */
