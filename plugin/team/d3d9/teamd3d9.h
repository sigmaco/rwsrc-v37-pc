#ifndef TEAMD3D9_H
#define TEAMD3D9_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "teamstaticd3d9.h"
#include "teamskind3d9.h"
#include "teammatfxd3d9.h"
#include "teamshadowd3d9.h"

#define TEAMSHADOWALLINONE       /* Render shadows as all in one. */
#define TEAMSHADOWSPRIVATELOOPx  /* Render shadows all together.  */

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/
typedef enum TeamD3D9PipeId
{
    TEAMD3D9PIPEID_NAPIPE = 0,
    TEAMD3D9PIPEID_STATIC,
    TEAMD3D9PIPEID_SKINNED,
    TEAMD3D9PIPEID_STATIC_MATFX,
    TEAMD3D9PIPEID_SKINNED_MATFX,
    TEAMD3D9PIPEID_STATIC_SHADOW,
    TEAMD3D9PIPEID_SKINNED_SHADOW,
    TEAMD3D9PIPEID_MAX
} TeamD3D9PipeId;

/*===========================================================================*
 *--- Private Functions -----------------------------------------------------*
 *===========================================================================*/

extern void
_rpTeamD3D9SetRenderPipeline(RxPipeline *pipe, TeamD3D9PipeId pipeId);

#endif /* TEAMD3D9_H */
