#ifndef TEAMPLAYER_H
#define TEAMPLAYER_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpteam.h"

#include "teamshadow.h"
#include "teamlod.h"
#include "teamskin.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/
typedef struct RpTeamPlayerData RpTeamPlayerData;  /* 12 bytes total. */
struct RpTeamPlayerData
{
    RwUInt32 numOfPlayerElements;      /* 4 bytes. */
    RwUInt32 numOfPlayers;             /* 4 bytes. */
    RwBool   optimized;                /* 4 bytes. */
};

enum PlayerFlags
{
    TEAMPLAYERNONE     = 0x00,
    TEAMPLAYERSELECTED = 0x01,
    TEAMPLAYERRENDER   = 0x02,
    TEAMPLAYERSHADOW   = 0x04,
    TEAMPLAYERCLONE    = 0x08,
    TEAMFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum PlayerFlags PlayerFlags;

typedef struct PlayerCallBackData PlayerCallBackData;  /* 12 bytes total. */
struct PlayerCallBackData
{
    RpTeamPlayerRenderCallBack preRenderCallBack;      /* 4 bytes. */
    RpTeamPlayerRenderCallBack postRenderCallBack;     /* 4 bytes. */
    RpTeamRenderData renderData;                       /* 4 bytes. */
};

/*===========================================================================*
 *--- Public Types ----------------------------------------------------------*
 *===========================================================================*/

/* This structure needs to be 16 bytes aligned (for the ps2). */
struct RpTeamPlayer                              /* 284 -> 288 bytes total. */
{
    RpTeamShadowRenderData shadowData;           /* 224 bytes. */
    RwSphere               boundingSphere;       /* 16  bytes. */
    RwUInt32               flags;                /* 4   bytes. */
    RwUInt32               elementFlags;         /* 4   bytes. */
    RpTeamPlayerLOD        lod;                  /* 8   bytes. */
    RpTeam                *team;                 /* 4   bytes. */
    RwUInt32              *elements;             /* 4   bytes. */
    RwFrame               *frame;                /* 4   bytes. */
#if(defined(TEAMMATRIXCACHECACHE))
    RwMatrix              *matrixCache;          /* 4   bytes. */
#else
    SkinCache              skinCache;            /* 12  bytes. */
#endif
    PlayerCallBackData     callBack;             /* 12  bytes. */
    RwUInt8                pad[4];               /* 4   bytes. */
};

/*===========================================================================*
 *--- Private Variables -----------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/
extern RpTeamPlayer *
_rpTeamPlayerDestroy( RpTeamPlayer *player );

#define _rpTeamPlayerGetPlayerIndex(player)                          \
    ((player) - (player)->team->players)

/*===========================================================================*
 *--- Pipeline access data --------------------------------------------------*
 *===========================================================================*/
#if (defined(TEAMGLOBALPOINTS))

extern const RpTeamPlayer *_rpTeamPlayerRenderCurrent;

#define _rpTeamPlayerGetCurrentPlayer()                              \
    _rpTeamPlayerRenderCurrent

#define _rpTeamPlayerSetCurrentPlayer(player)                        \
_rpTeamPlayerRenderCurrent = player

#else /* (defined(TEAMGLOBALPOINTS)) */

#define _rpTeamPlayerGetCurrentPlayer()
#define _rpTeamPlayerSetCurrentPlayer(player)

#endif /* (defined(TEAMGLOBALPOINTS)) */

#endif /* TEAMPLAYER_H */

