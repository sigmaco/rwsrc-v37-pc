#ifndef TEAMMATERIAL_H
#define TEAMMATERIAL_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "team.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/
typedef struct TeamMaterialTeamExt TeamMaterialTeamExt;
struct TeamMaterialTeamExt
{
    const RpTeam *team;
    RwTexture *teamTexture;
    RwUInt32 numOfPlayerSpecifics;
    RwTexture **playerTextures;
    RwRGBA *playerColors;
};

typedef struct TeamMaterialData TeamMaterialData;
struct TeamMaterialData
{
    RwUInt32 numOfTeams;
    TeamMaterialTeamExt *teams;
};

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/
extern RwInt32 TeamMaterialDataOffset;

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/

/*---- Team Material data ----*/
#define TEAMMATERIALGETDATA(material)                                \
    ((TeamMaterialData *)(((RwUInt8 *)material)+                     \
                           TeamMaterialDataOffset))

#define TEAMMATERIALGETCONSTDATA(material)                           \
    ((const TeamMaterialData *)(((const RwUInt8 *)material)+         \
                                 TeamMaterialDataOffset))

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/
extern void *
_rpTeamMaterialConstructor(void *object, RwInt32 offset, RwInt32 size);

extern void *
_rpTeamMaterialDestructor(void *object, RwInt32 offset, RwInt32 size);

extern void *
_rpTeamMaterialCopy( void *dstObject,
                     const void *srcObject,
                     RwInt32 offset,
                     RwInt32 size );

#endif /* TEAMMATERIAL_H */
