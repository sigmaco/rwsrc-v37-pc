#ifndef TEAMTYPES_H
#define TEAMTYPES_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpteam.h"

/*===========================================================================*
 *--- Platform specific extention -------------------------------------------*
 *===========================================================================*/
#if (defined(SKY2_DRVMODEL_H))
#include "sky2/teamskytypes.h"
#endif

/*
 * We define the following extension to support open and closing of
 * rendering pipelines.
 *
 * I've disabled this for the PS2 as it renders with all in one pipes.
 */

#if (!defined(SKY2_DRVMODEL_H))
#if (!defined(GCN_DRVMODEL_H))
#define TEAMPIPEEXTENSIONS
#endif /* (!defined(GCN_DRVMODEL_H)) */
#define TEAMGLOBALPOINTS
#endif /* (!defined(SKY2_DRVMODEL_H)) */

/*===========================================================================*
 *--- Public Types ----------------------------------------------------------*
 *===========================================================================*/

/* If a platform hasn't defined the Unions, we define then here. */
#if (!defined(TEAMSKYUNION))
typedef struct TeamSkyUnion TeamSkyUnion;
struct TeamSkyUnion
{
    struct
    {
        RwV3d v;
    } v3d;
};
#endif /* (!defined(TEAMSKYUNION)) */

#if (!defined(TEAMSKYMATRIXUNION))
typedef struct TeamSkyMatrixUnion TeamSkyMatrixUnion;
struct TeamSkyMatrixUnion
{
    RwMatrix matrix;
};
#endif /* (!defined(TEAMSKYMATRIXUNION)) */

/*===========================================================================*
 *--- Public Global Variables -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

#endif /* TEAMTYPES_H */
