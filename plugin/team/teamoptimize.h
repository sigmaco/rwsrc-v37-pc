#ifndef TEAMOPTIMIZE_H
#define TEAMOPTIMIZE_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpteam.h"

#include "teamlod.h"
#include "teamtypes.h"

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/

/*---------------------------------------------------------------------------*
  STATIC OPTIMIZATION DATA
 *---------------------------------------------------------------------------*/
typedef struct RpTeamRenderStatic RpTeamRenderStatic;
struct RpTeamRenderStatic
{
#if (defined(TEAMBRIDGECLONE))
    TeamSkyUnion localNormal;
    TeamSkyUnion localPosition;
    TeamSkyUnion localProjections[4];
    RwUInt32 transType;
    RwUInt8 pad[4];
#endif

    RwMatrix *matrices;
    RpTeamPlayer *player;
};

/*---------------------------------------------------------------------------*
  SKINNED OPTIMIZATION DATA
 *---------------------------------------------------------------------------*/
typedef struct RpTeamRenderSkin RpTeamRenderSkin;
struct RpTeamRenderSkin
{
#if (!defined(TEAMMATRIXCACHECACHE))
    RwMatrix *matrixCache;
#endif

    RpTeamPlayer *player;

#if (defined(TEAMBRIDGECLONE))
    RwUInt32 transType;
#endif
};

/*---------------------------------------------------------------------------*
  JOINT OPTIMIZATION DATA
 *---------------------------------------------------------------------------*/
typedef struct RpTeamOptElement RpTeamOptElement;
struct RpTeamOptElement
{
    RwUInt32 numClones;
    RwUInt32 stack;
    TeamLODAtomic *lodAtomic;

    union
    {
        RpTeamRenderStatic *staticRender;
        RpTeamRenderSkin   *skinRender;
    };
};

/*---------------------------------------------------------------------------*/
typedef struct RpTeamOptStatic RpTeamOptStatic;
struct RpTeamOptStatic
{
    RwBool optimized;
    RwUInt32 numElements;
    RpTeamOptElement *data;
};

/*---------------------------------------------------------------------------*/
typedef struct RpTeamOptSkin RpTeamOptSkin;
struct RpTeamOptSkin
{
    RwBool optimized;
    RwUInt32 numElements;
    RpTeamOptElement *data;
    RpSkin *firstSkin;
};

/*---------------------------------------------------------------------------*
  PATCH OPTIMIZATION DATA
 *---------------------------------------------------------------------------*/
typedef struct RpTeamOptPatch RpTeamOptPatch;
struct RpTeamOptPatch
{
    RwBool optimized;
};

/*---------------------------------------------------------------------------*/
typedef struct RpTeamOptElements RpTeamOptElements;
struct RpTeamOptElements
{
    /* Is the present team optimized? */
    RwBool optimized;
    /* Type data. */
    RpTeamOptStatic stat;
    RpTeamOptSkin   skin;
    RpTeamOptPatch  patch;
    /* Locked data. */
};

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/
extern RpTeam *
_rpTeamOptimizeCreate( RpTeam *team,
                       RwUInt32 type,
                       RwUInt32 locked );

extern RpTeam *
_rpTeamOptimizeDestroy( RpTeam *team,
                        RwUInt32 type,
                        RwUInt32 locked );

extern RpTeam *
_rpTeamSetElementTypeOptimized( RpTeam *team,
                                RwUInt32 type,
                                RwUInt32 locked,
                                RwBool optimized );

#endif /* TEAMOPTIMIZE_H */
