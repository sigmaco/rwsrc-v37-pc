#ifndef TEAMD3D8_H
#define TEAMD3D8_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "teamstaticd3d8.h"
#include "teamskind3d8.h"
#include "teammatfxd3d8.h"
#include "teamshadowd3d8.h"

#define TEAMSHADOWALLINONE       /* Render shadows as all in one. */
#define TEAMSHADOWSPRIVATELOOPx  /* Render shadows all together.  */

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/
typedef enum TeamD3D8PipeId
{
    TEAMD3D8PIPEID_NAPIPE = 0,
    TEAMD3D8PIPEID_STATIC,
    TEAMD3D8PIPEID_SKINNED,
    TEAMD3D8PIPEID_STATIC_MATFX,
    TEAMD3D8PIPEID_SKINNED_MATFX,
    TEAMD3D8PIPEID_STATIC_SHADOW,
    TEAMD3D8PIPEID_SKINNED_SHADOW,
    TEAMD3D8PIPEID_MAX
} TeamD3D8PipeId;

/*===========================================================================*
 *--- Private Functions -----------------------------------------------------*
 *===========================================================================*/

extern void
_rpTeamD3D8SetRenderPipeline(RxPipeline *pipe, TeamD3D8PipeId pipeId);

#endif /* TEAMD3D8_H */
