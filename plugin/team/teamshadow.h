#ifndef TEAMSHADOW_H
#define TEAMSHADOW_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpteam.h"

#include "teamtypes.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/
typedef struct RpTeamShadowRenderData RpTeamShadowRenderData; /* 224 bytes total. */
struct RpTeamShadowRenderData
{
    /* Local Space plane data. */
    TeamSkyUnion localPlaneNormal;                       /* 16 bytes. */
    TeamSkyUnion localPlanePos;                          /* 16 bytes. */
    TeamSkyUnion localProjection[rpTEAMMAXNUMSHADOWS];   /* 64 bytes. */

    /* World Space plane data. */
    TeamSkyUnion worldPlaneNormal;                       /* 16 bytes. */
    TeamSkyUnion worldPlanePos;                          /* 16 bytes. */
    TeamSkyUnion worldProjection[rpTEAMMAXNUMSHADOWS];   /* 64 bytes. */

    RwReal renderValues[rpTEAMMAXNUMSHADOWS];            /* 16 bytes. */
    RwUInt32 renderNumShadows;                           /* 4  bytes. */

    /* Shadow and player clipping flags. */
    RwUInt32 shadowClip;                                 /* 4  bytes. */
    RwUInt32 playerClip;                                 /* 4  bytes. */

    /* Team shadow data. */
    RpTeamShadows *shadows;                              /* 4 bytes.  */
};

/*===========================================================================*
 *--- Private Variables -----------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Functions -----------------------------------------------------*
 *===========================================================================*/
extern RwBool
_rpTeamShadowPipelineCreate(void);

extern void
_rpTeamShadowPipelineDestroy(void);

extern RxPipeline *
_rpTeamShadowGetPipeline(RpTeamElementType type, RwBool allInOne);

extern RpAtomic *
_rpTeamShadowAttachPipeline( RpAtomic *atomic,
                             RpTeamElementType type,
                             RwBool allInOne );

extern RwFrustumTestResult
_rpTeamShadowCalcRenderData(RpTeamPlayer *player);

/*===========================================================================*
 *--- Pipeline access data --------------------------------------------------*
 *===========================================================================*/
#if (defined(TEAMGLOBALPOINTS))

extern /* const */ RpTeamShadowRenderData *_rpTeamShadowRenderData;

#define _rpTeamShadowGetCurrentShadowData()                          \
    _rpTeamShadowRenderData

#define _rpTeamShadowSetCurrentShadowData(shadow)                    \
    _rpTeamShadowRenderData = shadow

#else /* (defined(TEAMGLOBALPOINTS)) */

#define _rpTeamShadowGetCurrentShadowData()
#define _rpTeamShadowSetCurrentShadowData(shadow)

#endif /* (defined(TEAMGLOBALPOINTS)) */

#endif /* TEAMSHADOW_H */
