/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   normmapd3d9.h                                              -*
 *-                                                                         -*
 *-  Purpose :   Normal Maps D3D9 plugin internal API                       -*
 *-                                                                         -*
 *===========================================================================*/

#ifndef NORMMAPD3D9_H
#define NORMMAPD3D9_H

#include <d3d9.h>

#include "rwcore.h"
#include "rpworld.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

struct NormalMapPixelShaderConstants
{
    RwV4d colorDirLight;
    RwV4d colorAmbientLight;
};
typedef struct NormalMapPixelShaderConstants NormalMapPixelShaderConstants;

/*===========================================================================*
 *--- Private Variables -----------------------------------------------------*
 *===========================================================================*/

extern void *NormalMapPixelShader;
extern void *NormalMapPrelitPixelShader;
extern void *NormalMapEnvMapPixelShader;
extern void *NormalMapEnvMapPrelitPixelShader;

extern void *NormalMapNoTexturePixelShader;
extern void *NormalMapPrelitNoTexturePixelShader;
extern void *NormalMapEnvMapNoTexturePixelShader;
extern void *NormalMapEnvMapPrelitNoTexturePixelShader;

extern void *NormalMapEnvMapMetalPixelShader;
extern void *NormalMapEnvMapMetalPrelitPixelShader;

extern const RpLight    *NormalMapMainLight;
extern RwRGBAReal       NormalMapAmbientColor;

extern const RwMatrix NormalMapEnvMapTexMat;

extern RwBool NormalMapSupported;

/*===========================================================================*
 *--- Private Functions -----------------------------------------------------*
 *===========================================================================*/

#ifdef    __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

extern RwBool
NormalMapCreatePipelines(void);

extern RxPipeline *
NormalMapCreateAtomicPipeline(void);

extern RxPipeline *
NormalMapCreateWorldSectorPipeline(void);

extern void
NormalMapDestroyPipelines(void);

/* External utility functions */
extern RwBool
_rwD3D9TextureHasAlpha(const RwTexture *texture);

#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#endif /* NORMMAPD3D9_H */