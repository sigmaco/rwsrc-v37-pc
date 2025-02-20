#ifndef TEAMSTATICD3D8_H
#define TEAMSTATICD3D8_H

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
    
extern RwBool _rwD3D8RenderStateVertexAlphaEnable(RwBool enable);
extern RwBool _rwD3D8RenderStateIsVertexAlphaEnable(void);

extern RwBool _rwD3D8RenderStateSrcBlend(RwBlendFunction srcBlend);
extern RwBool _rwD3D8RenderStateDestBlend(RwBlendFunction dstBlend);

extern RwRaster *_rwD3D8RWGetRasterStage(RwUInt32 stage);

#if (defined(__cplusplus))
}
#endif /* (defined(__cplusplus)) */


#endif /* TEAMSTATICD3D8_H */
