#ifndef TEAMMATFX_H
#define TEAMMATFX_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpteam.h"
#include "rpmatfx.h"

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/
extern RwBool
_rpTeamMatFXPipelineCreate(void);

extern void
_rpTeamMatFXPipelineDestroy(void);

extern RxPipeline *
_rpTeamMatFXGetPipeline(RpTeamElementType type, RwBool uv2);

extern RpAtomic *
_rpTeamMatFXAttachPipeline(RpAtomic *atomic, RpTeamElementType type);

extern RwBool
_rpTeamMatFXEffectsEnabled(RpAtomic *atomic);

#endif /* TEAMMATFX_H */
