#ifndef TEAMLOD_H
#define TEAMLOD_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpteam.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/
typedef RpAtomic *TeamLODAtomic[rpTEAMELEMENTLODSUP];

typedef struct RpTeamLOD RpTeamLOD;
struct RpTeamLOD
{
    RpTeamLODCamera   camera;
    RwReal            farRange;
    RwReal            invFarRange;
    RpTeamLODCallBack callBack;
    RpTeamLODData     data;
};

typedef struct RpTeamPlayerLOD RpTeamPlayerLOD;
struct RpTeamPlayerLOD
{
    RwUInt32 currentLOD;
    RwBool   syncLODPerFrame;
};

/*===========================================================================*
 *--- Public Types ----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/
extern RwUInt32
_rpTeamLODNumOf( TeamLODAtomic element );

extern RwUInt32
_rpTeamPlayerLODDefaultSelectLODCallBack( const RpTeam *team,
                                          RwUInt32 playerIndex,
                                          RpTeamLODData data );

#endif /* TEAMLOD_H */

