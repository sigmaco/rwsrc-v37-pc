#ifndef TEAMSTATICD3D9_H
#define TEAMSTATICD3D9_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpteam.h"

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

/*
 * internal driver functions; c.f
 * rwsdk/plugin/matfx/effectPipes.h
 */

#if (defined(__cplusplus))
extern "C"
{
#endif /* (defined(__cplusplus)) */
    
extern RwBool _rwD3D9RenderStateVertexAlphaEnable(RwBool enable);
extern RwBool _rwD3D9RenderStateIsVertexAlphaEnable(void);

extern RwBool _rwD3D9RenderStateSrcBlend(RwBlendFunction srcBlend);
extern RwBool _rwD3D9RenderStateDestBlend(RwBlendFunction dstBlend);

extern RwBool _rwD3D9TextureHasAlpha(const RwTexture *texture);

#if (defined(__cplusplus))
}
#endif /* (defined(__cplusplus)) */


#endif /* TEAMSTATICD3D9_H */
