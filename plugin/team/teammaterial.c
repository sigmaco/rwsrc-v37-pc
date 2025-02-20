/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>
#include "rpdbgerr.h"

#include "rpteam.h"
#include "rpmatfx.h"

#include "teammaterial.h"
#include "team.h"

#include "../matfx/effectPipes.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/
RwInt32 TeamMaterialDataOffset = 0;

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/
static TEAMINLINE TeamMaterialTeamExt *
TeamMaterialGetTeamIndex( const TeamMaterialData *materialData,
                          const RpTeam *team )
{
    RwUInt32 i;

    RWFUNCTION(RWSTRING("TeamMaterialGetTeamIndex"));
    RWASSERT(NULL != materialData);
    RWASSERT(NULL != team);

    for( i = 0; i < materialData->numOfTeams; i++ )
    {
        if(materialData->teams[i].team == team)
        {
            RWRETURN(&(materialData->teams[i]));
        }
    }

    RWRETURN((TeamMaterialTeamExt *)NULL);
}

static TeamMaterialTeamExt *
TeamMaterialSetupMaterial(RpMaterial *material, const RpTeam *team)
{
    TeamMaterialTeamExt *materialTeamExt;
    TeamMaterialData *materialData;

    RWFUNCTION(RWSTRING("TeamMaterialSetupMaterial"));
    RWASSERT(NULL != material);
    RWASSERT(NULL != team);

    materialData = (TeamMaterialData *)TEAMMATERIALGETDATA(material);
    RWASSERT(NULL != materialData);

    if(NULL != materialData->teams)
    {
        /* Try and find our team. */
        materialTeamExt = TeamMaterialGetTeamIndex(materialData, team);

        if(NULL != materialTeamExt)
        {
            /* Found it. So return. */
            RWRETURN(materialTeamExt);
        }
        else
        {
            /* Doh need to realloc. */
            TeamMaterialTeamExt *teams;
            RwUInt32 size;
            RwUInt32 i;

            size = sizeof(TeamMaterialTeamExt) *
                   (materialData->numOfTeams + 1);
            teams = (TeamMaterialTeamExt *)RwMalloc(size,
                rwID_TEAMPLUGIN | rwMEMHINTDUR_EVENT);

            if(NULL == teams)
            {
                RWERROR((E_RW_NOMEM, size));
                RWRETURN((TeamMaterialTeamExt *)NULL);
            }
            memset(teams, 0, size);

            for( i = 0; i < materialData->numOfTeams; i++ )
            {
                TeamMaterialTeamExt *src = &(materialData->teams[i]);

                teams[i].team = src->team;
                teams[i].numOfPlayerSpecifics = src->numOfPlayerSpecifics;
                teams[i].playerTextures = src->playerTextures;
                teams[i].playerColors = src->playerColors;
            }

            /* Free the old data. */
            RwFree(materialData->teams);
            materialData->teams = teams;

            /* Mark the teamPlayerTeaxture to setup. */
            materialTeamExt = &(materialData->teams[materialData->numOfTeams]);
        }
    }
    else
    {
        /* So no teams so set one up. */
        RwUInt32 size;

        size = sizeof(TeamMaterialTeamExt) *
               (materialData->numOfTeams + 1);
        materialData->teams = (TeamMaterialTeamExt *)RwMalloc(size,
            rwID_TEAMPLUGIN | rwMEMHINTDUR_EVENT);

        if(NULL == materialData->teams)
        {
            RWERROR((E_RW_NOMEM, size));
            RWRETURN((TeamMaterialTeamExt *)NULL);
        }
        memset(materialData->teams, 0, size);

        /* Mark the teamPlayerTexture to setup. */
        materialTeamExt = &(materialData->teams[0]);
    }

    {
        RwUInt32 numOfPlayerSpecifics;
        RpMatFXMaterialFlags effect;
        RwTexture **textures;
        RwRGBA *colors;
        RwUInt32 size;
        RwUInt32 iColor;

        /* Ok time to setup the teamPlayerTexture. */
        RWASSERT(NULL != materialTeamExt);

        numOfPlayerSpecifics = team->data.numOfPlayers;
        effect = RpMatFXMaterialGetEffects(material);
        /* Reserve size for the material colors. */
        size = sizeof(RwRGBA) * numOfPlayerSpecifics;
        /* Might need more size for two sets of textures. */
        if(effect == rpMATFXEFFECTDUAL)
        {
            numOfPlayerSpecifics *= 2;
        }
        /* Reserve more space for the textures. */
        size += sizeof(RwTexture *) * numOfPlayerSpecifics;

        /* Malloc some space. */
        textures = (RwTexture **)RwMalloc(size,
            rwID_TEAMPLUGIN | rwMEMHINTDUR_EVENT);

        /* Did we get some? */
        if(NULL == textures)
        {
            RWERROR((E_RW_NOMEM, size));
            RWRETURN((TeamMaterialTeamExt *)NULL);
        }

        /* Fix up the colors pointer. */
        colors = (RwRGBA *)&(textures[numOfPlayerSpecifics]);

        /* Blank the memory. */
        memset(textures, 0, size);

        /* Initalise the colors to white. */
        for( iColor = 0; iColor < team->data.numOfPlayers; iColor++)
        {
            colors[iColor].red   = 255;
            colors[iColor].green = 255;
            colors[iColor].blue  = 255;
            colors[iColor].alpha = 255;
        }

        materialTeamExt->playerTextures = textures;
        materialTeamExt->playerColors = colors;
        materialTeamExt->numOfPlayerSpecifics = numOfPlayerSpecifics;
        materialTeamExt->team = team;

        materialData->numOfTeams++;
    }

    RWRETURN(materialTeamExt);
}

static TeamMaterialTeamExt *
TeamMaterialSetPlayerTexture( TeamMaterialTeamExt *materialTeamExt,
                              RwUInt32 index,
                              RwTexture *texture )
{
    RWFUNCTION(RWSTRING("TeamMaterialSetPlayerTexture"));
    RWASSERT(NULL != materialTeamExt);
    RWASSERT(materialTeamExt->numOfPlayerSpecifics > index);

    if(NULL != materialTeamExt->playerTextures[index])
    {
        RwTextureDestroy(materialTeamExt->playerTextures[index]);
        materialTeamExt->playerTextures[index] = (RwTexture *)NULL;
    }

    if(NULL != texture)
    {
        materialTeamExt->playerTextures[index] = texture;
        (void)RwTextureAddRef(materialTeamExt->playerTextures[index]);
    }

    RWASSERT(materialTeamExt->playerTextures[index] == texture);
    RWRETURN(materialTeamExt);
}

static TEAMINLINE RwTexture *
TeamMaterialGetPlayerTexture( TeamMaterialTeamExt *materialTeamExt,
                              RwUInt32 index )
{
    RWFUNCTION(RWSTRING("TeamMaterialGetPlayerTexture"));
    RWASSERT(NULL != materialTeamExt);
    RWASSERT(materialTeamExt->numOfPlayerSpecifics > index);

    RWRETURN(materialTeamExt->playerTextures[index]);
}

static TeamMaterialTeamExt *
TeamMaterialSetPlayerColor( TeamMaterialTeamExt *materialTeamExt,
                            RwUInt32 index,
                            const RwRGBA *color )
{
    static RwRGBA white = {255,255,255,255};

    RWFUNCTION(RWSTRING("TeamMaterialSetPlayerColor"));
    RWASSERT(NULL != materialTeamExt);
    RWASSERT(materialTeamExt->numOfPlayerSpecifics > index);

    /* If no color is specified set to white. */
    if(NULL == color)
    {
        color = &white;
    }

    materialTeamExt->playerColors[index] = *color;

    RWRETURN(materialTeamExt);
}

static TEAMINLINE RwRGBA *
TeamMaterialGetPlayerColor( TeamMaterialTeamExt *materialTeamExt,
                            RwUInt32 index )
{
    RWFUNCTION(RWSTRING("TeamMaterialGetPlayerColor"));
    RWASSERT(NULL != materialTeamExt);
    RWASSERT(materialTeamExt->numOfPlayerSpecifics > index);

    RWRETURN(&(materialTeamExt->playerColors[index]));
}

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/
void *
_rpTeamMaterialConstructor( void *object,
                            RwInt32 offset __RWUNUSED__,
                            RwInt32 size __RWUNUSED__ )
{
    TeamMaterialData *materialData;

    RWFUNCTION(RWSTRING("_rpTeamMaterialConstructor"));
    RWASSERT(NULL != object);

    materialData = (TeamMaterialData *)TEAMMATERIALGETDATA(object);
    RWASSERT(NULL != materialData);

    materialData->teams = (TeamMaterialTeamExt *)NULL;
    materialData->numOfTeams = 0;

    RWRETURN(object);
}

void *
_rpTeamMaterialDestructor( void *object,
                           RwInt32 offset __RWUNUSED__,
                           RwInt32 size __RWUNUSED__ )
{
    TeamMaterialData *materialData;

    RWFUNCTION(RWSTRING("_rpTeamMaterialDestructor"));
    RWASSERT(NULL != object);

    materialData = (TeamMaterialData *)TEAMMATERIALGETDATA(object);
    RWASSERT(NULL != materialData);

    if(0 < materialData->numOfTeams)
    {
        RwUInt32 i, j;

        for( i = 0; i < materialData->numOfTeams; i++ )
        {
            TeamMaterialTeamExt *materialTeamExt;

            materialTeamExt = &(materialData->teams[i]);
            RWASSERT(NULL != materialTeamExt);

            for( j = 0; j < materialTeamExt->numOfPlayerSpecifics; j++ )
            {
                TeamMaterialSetPlayerTexture( materialTeamExt,
                                              j, (RwTexture *)NULL );
            }

            /* This frees the playerColors as well.
             * As the memory was malloced as one. */
            RwFree(materialTeamExt->playerTextures);

            materialTeamExt->team = (RpTeam *)NULL;
            materialTeamExt->numOfPlayerSpecifics = 0;
            materialTeamExt->playerTextures = (RwTexture **)NULL;
        }

        RwFree(materialData->teams);

        materialData->teams = (TeamMaterialTeamExt *)NULL;
        materialData->numOfTeams = 0;
    }

    RWRETURN(object);
}

void *
_rpTeamMaterialCopy( void *dstObject,
                     const void *srcObject,
                     RwInt32 offset __RWUNUSED__,
                     RwInt32 size __RWUNUSED__ )
{
    const RpMaterial *srcMaterial;
    const TeamMaterialData *srcMaterialData;

    RpMaterial *dstMaterial;
    TeamMaterialData *dstMaterialData;

    RWFUNCTION(RWSTRING("_rpTeamMaterialCopy"));
    RWASSERT(NULL != dstObject);
    RWASSERT(NULL != srcObject);

    srcMaterial = (const RpMaterial *)srcObject;
    dstMaterial = (RpMaterial *)dstObject;

    srcMaterialData = (const TeamMaterialData *)
        TEAMMATERIALGETCONSTDATA(srcMaterial);
    RWASSERT(NULL != srcMaterialData);

    dstMaterialData = (TeamMaterialData *)TEAMMATERIALGETDATA(dstMaterial);
    RWASSERT(NULL != dstMaterialData);

    /* Does the source have any extension? */
    if(NULL != srcMaterialData->teams)
    {
        RwUInt32 iTeams, iTextures;
        RpMatFXMaterialFlags effect;

        effect = RpMatFXMaterialGetEffects(srcMaterial);

        for( iTeams = 0; iTeams < srcMaterialData->numOfTeams; iTeams++ )
        {
            TeamMaterialTeamExt *materialTeamExt;
            RwUInt32 firstPass;
            RwUInt32 iColor;

            materialTeamExt = &(srcMaterialData->teams[iTeams]);
            firstPass = materialTeamExt->numOfPlayerSpecifics;

            if(effect == rpMATFXEFFECTDUAL)
            {
                firstPass >>= 1;
            }

            for( iTextures = 0; iTextures < firstPass; iTextures++ )
            {
                RwTexture *texture;
                RpTeamPlayer *player;
                RwUInt32 playerIndex;

                playerIndex = iTextures;
                texture = materialTeamExt->playerTextures[iTextures];
                player = RpTeamGetPlayer(materialTeamExt->team, playerIndex);

                RpTeamMaterialSetPlayerTexture( dstMaterial,
                                                player,
                                                texture );

                RWASSERT(srcMaterialData->teams[iTeams].playerTextures[iTextures] ==
                         dstMaterialData->teams[iTeams].playerTextures[iTextures]);
            }

            for( iTextures = firstPass;
                 iTextures < materialTeamExt->numOfPlayerSpecifics;
                 iTextures++ )
            {
                RwTexture *texture;
                RpTeamPlayer *player;
                RwUInt32 playerIndex;

                playerIndex = ( materialTeamExt->numOfPlayerSpecifics
                              - firstPass ) - 1;
                texture = materialTeamExt->playerTextures[iTextures];
                player = RpTeamGetPlayer(materialTeamExt->team, playerIndex);

                RpTeamMaterialSetPlayerDualTexture( dstMaterial,
                                                    player,
                                                    texture );

                RWASSERT(srcMaterialData->teams[iTeams].playerTextures[iTextures] ==
                         dstMaterialData->teams[iTeams].playerTextures[iTextures]);
            }

            /* We also copy the material colors. */
            for( iColor = 0; iColor < firstPass; iColor++ )
            {
                RwRGBA *color;
                RpTeamPlayer *player;
                RwUInt32 playerIndex;

                playerIndex = iColor;
                color = &(materialTeamExt->playerColors[iColor]);
                player = RpTeamGetPlayer(materialTeamExt->team, playerIndex);

                RpTeamMaterialSetPlayerColor(dstMaterial, player, color);

#if (defined(RWDEBUG))
                {
                    RwRGBA *src = &(srcMaterialData->teams[iTeams].playerColors[iColor]);
                    RwRGBA *dst = &(dstMaterialData->teams[iTeams].playerColors[iColor]);

                    RWASSERT(    (src->red   ==   dst->red)
                              && (src->green == dst->green)
                              && (src->blue  ==  dst->blue)
                              && (src->alpha == dst->alpha) );
                }
#endif /* (defined(RWDEBUG)) */
            }

            RWASSERT(srcMaterialData->teams[iTeams].team ==
                     dstMaterialData->teams[iTeams].team);
            RWASSERT(srcMaterialData->teams[iTeams].numOfPlayerSpecifics ==
                     dstMaterialData->teams[iTeams].numOfPlayerSpecifics);
        }
    }

    RWASSERT(srcMaterialData->numOfTeams == dstMaterialData->numOfTeams);
    RWRETURN(dstObject);
}

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rpteam
 * \ref RpTeamMaterialSetTeamTexture is used to set a team specific base
 * texture. This allows the developer to use a single piece of geometry and
 * overload the texture that is used to render the texture with on a team
 * basis. Hence it is possible to have a number of team's using the same
 * pieces of geometry.
 *
 * If no texture is specified for a team or an individual player then the
 * base texture is used when rendering. A player specific texture is used
 * in favor of a team specific texture.
 *
 * A team specific texture can be removed by setting NULL for that team.
 *
 * \param material Pointer to the material.
 * \param team     Pointer to the team.
 * \param texture  Pointer to the texture.
 *
 * \return Returns a pointer to the material if successful or NULL if there
 * is an error.
 *
 * \see RpTeamMaterialGetTeamTexture
 * \see RpTeamMaterialSetPlayerTexture
 * \see RpTeamMaterialGetPlayerTexture
 * \see RpTeamMaterialSetPlayerDualTexture
 * \see RpTeamMaterialGetPlayerDualTexture
 */
RpMaterial *
RpTeamMaterialSetTeamTexture( RpMaterial *material,
                              const RpTeam *team,
                              RwTexture *texture )
{
    TeamMaterialData *materialData;
    TeamMaterialTeamExt *materialTeamExt;

    RWAPIFUNCTION(RWSTRING("RpTeamMaterialSetTeamTexture"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != material);
    RWASSERT((const RpTeam *)NULL != team);

    materialData = TEAMMATERIALGETDATA(material);
    RWASSERT(NULL != materialData);

    materialTeamExt = TeamMaterialSetupMaterial(material, team);
    if(NULL != materialTeamExt)
    {
        materialTeamExt->teamTexture = texture;
    }

    RWRETURN((RpMaterial *)NULL);
}

/**
 * \ingroup rpteam
 * \ref RpTeamMaterialGetTeamTexture is used to return the defined team
 * specific texture.
 *
 * \param material Pointer to the material.
 * \param team     Pointer to the team.
 *
 * \return Returns a pointer to the texture if successful or NULL if there
 * is an error.
 *
 * \see RpTeamMaterialSetTeamTexture
 * \see RpTeamMaterialSetPlayerTexture
 * \see RpTeamMaterialGetPlayerTexture
 * \see RpTeamMaterialSetPlayerDualTexture
 * \see RpTeamMaterialGetPlayerDualTexture
 */
RwTexture *
RpTeamMaterialGetTeamTexture( const RpMaterial *material,
                              const RpTeam *team )
{
    const TeamMaterialData *materialData;
    RwTexture *texture;

    RWAPIFUNCTION(RWSTRING("RpTeamMaterialGetTeamTexture"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != material);
    RWASSERT(NULL != team);

    materialData = TEAMMATERIALGETCONSTDATA(material);
    RWASSERT(NULL != materialData);

    texture = (RwTexture *)NULL;

    if(0 < materialData->numOfTeams)
    {
        TeamMaterialTeamExt *materialTeamExt;

        materialTeamExt = TeamMaterialGetTeamIndex(materialData, team);
        if(NULL != materialTeamExt)
        {
            texture = materialTeamExt->teamTexture;
        }
    }

    if(NULL == texture)
    {
        texture = RpMaterialGetTexture(material);
    }

    RWRETURN(texture);
}

/**
 * \ingroup rpteam
 * \ref RpTeamMaterialSetPlayerTexture is used to set a player specific base
 * texture. This allows the developer to use a single piece of geometry and
 * overload the texture that is used to render the texture with.
 *
 * If no texture is specified for an individual player then the team specific
 * texture is used when rendering. If no player and team specific textures
 * are defined then the base texture is used when rendering. A player
 * specific texture can be removed by setting NULL for the player.
 *
 * \param material Pointer to the material.
 * \param player   Pointer to the player.
 * \param texture  Pointer to the texture.
 *
 * \return Returns a pointer to the material if successful or NULL if there
 * is an error.
 *
 * \see RpTeamGetPlayer
 * \see RpTeamMaterialGetPlayerTexture
 * \see RpTeamMaterialSetPlayerDualTexture
 * \see RpTeamMaterialGetPlayerDualTexture
 */
RpMaterial *
RpTeamMaterialSetPlayerTexture( RpMaterial *material,
                                const RpTeamPlayer *player,
                                RwTexture *texture )
{
    TeamMaterialTeamExt *materialTeamExt;
    TeamMaterialData *materialData;

    RWAPIFUNCTION(RWSTRING("RpTeamMaterialSetPlayerTexture"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != material);
    RWASSERT((const RpTeamPlayer *)NULL != player);

    materialData = TEAMMATERIALGETDATA(material);
    RWASSERT(NULL != materialData);

    materialTeamExt = TeamMaterialSetupMaterial(material, player->team);
    if(NULL != materialTeamExt)
    {
        RpTeam *team;
        RwUInt32 playerIndex;

        team = player->team;
        RWASSERT(NULL != team);

        playerIndex = player - team->players;
        RWASSERT(team->data.numOfPlayers > playerIndex);

        TeamMaterialSetPlayerTexture(materialTeamExt, playerIndex, texture);

        RWRETURN(material);
    }

    RWRETURN((RpMaterial *)NULL);
}

/**
 * \ingroup rpteam
 * \ref RpTeamMaterialGetPlayerTexture is used to return the defined player
 * specific texture.
 *
 * \param material Pointer to the material.
 * \param player   Pointer to the player.
 *
 * \return Returns a pointer to the texture if successful or NULL if there
 * is an error.
 *
 * \see RpTeamGetPlayer
 * \see RpTeamMaterialSetPlayerTexture
 * \see RpTeamMaterialSetPlayerDualTexture
 * \see RpTeamMaterialGetPlayerDualTexture
 */
RwTexture *
RpTeamMaterialGetPlayerTexture( const RpMaterial *material,
                                const RpTeamPlayer *player )
{
    const TeamMaterialData *materialData;
    RwTexture *texture;

    RWAPIFUNCTION(RWSTRING("RpTeamMaterialGetPlayerTexture"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != material);
    RWASSERT(NULL != player);
    RWASSERT(NULL != player->team);

    materialData = TEAMMATERIALGETCONSTDATA(material);
    RWASSERT(NULL != materialData);

    texture = (RwTexture *)NULL;

    if(0 < materialData->numOfTeams)
    {
        TeamMaterialTeamExt *materialTeamExt;
        RpTeam *team;
        RwUInt32 playerIndex;

        team = player->team;
        RWASSERT(NULL != team);

        playerIndex = player - team->players;
        RWASSERT(team->data.numOfPlayers > playerIndex);

        materialTeamExt = TeamMaterialGetTeamIndex(materialData, team);
        if(NULL != materialTeamExt)
        {
            texture = TeamMaterialGetPlayerTexture( materialTeamExt,
                                                    playerIndex );
            if(NULL == texture)
            {
                texture = materialTeamExt->teamTexture;
            }
        }
    }

    if(NULL == texture)
    {
        texture = RpMaterialGetTexture(material);
    }

    RWRETURN(texture);
}

/**
 * \ingroup rpteam
 * \ref RpTeamMaterialSetPlayerDualTexture is used to set a player specific
 * second pass texture. This allows the developer to use a single piece of
 * geometry and overload the second pass texture that is used to render
 * geometry.
 *
 * If no texture is specified for an individual player then the generic
 * material effect second pass texture is used when rendering. A player
 * specific second pass texture can be removed by setting NULL for the player.
 *
 * \param material Pointer to the material.
 * \param player   Pointer to the player.
 * \param texture  Pointer to the texture.
 *
 * \return Returns a pointer to the material if successful or NULL if there
 * is an error.
 *
 * \see RpTeamGetPlayer
 * \see RpTeamMaterialSetPlayerTexture
 * \see RpTeamMaterialGetPlayerTexture
 * \see RpTeamMaterialGetPlayerDualTexture
 */
RpMaterial *
RpTeamMaterialSetPlayerDualTexture( RpMaterial *material,
                                    const RpTeamPlayer *player,
                                    RwTexture *texture )
{
    TeamMaterialTeamExt *materialTeamExt;
    TeamMaterialData *materialData;

    RWAPIFUNCTION(RWSTRING("RpTeamMaterialSetPlayerDualTexture"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != material);
    RWASSERT(NULL != player);

    materialData = TEAMMATERIALGETDATA(material);
    RWASSERT(NULL != materialData);

    materialTeamExt = TeamMaterialSetupMaterial(material, player->team);
    if(NULL != materialTeamExt)
    {
        RpTeam *team;
        RwUInt32 playerIndex;
        RwUInt32 dualIndex;

        team = player->team;
        RWASSERT(NULL != team);

        playerIndex = player - team->players;
        RWASSERT(team->data.numOfPlayers > playerIndex);

        dualIndex = materialTeamExt->numOfPlayerSpecifics - (playerIndex + 1);
        TeamMaterialSetPlayerTexture(materialTeamExt, dualIndex, texture);

        RWRETURN(material);
    }

    RWRETURN((RpMaterial *)NULL);
}

/**
 * \ingroup rpteam
 * \ref RpTeamMaterialGetPlayerDualTexture is used to return the defined
 * second pass player specific texture.
 *
 * \param material Pointer to the material.
 * \param player   Pointer to the player.
 *
 * \return Returns a pointer to the texture if successful or NULL if there
 * is an error.
 *
 * \see RpTeamGetPlayer
 * \see RpTeamMaterialSetPlayerTexture
 * \see RpTeamMaterialGetPlayerTexture
 * \see RpTeamMaterialSetPlayerDualTexture
 */
RwTexture *
RpTeamMaterialGetPlayerDualTexture( const RpMaterial *material,
                                    const RpTeamPlayer *player )
{
    const TeamMaterialData *materialData;
    RwTexture *texture;

    RWAPIFUNCTION(RWSTRING("RpTeamMaterialGetPlayerDualTexture"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != material);
    RWASSERT(NULL != player);

    materialData = TEAMMATERIALGETCONSTDATA(material);
    RWASSERT(NULL != materialData);

    texture = (RwTexture *)NULL;

    if(0 < materialData->numOfTeams)
    {
        TeamMaterialTeamExt *materialTeamExt;
        RpTeam *team;
        RwUInt32 playerIndex;
        RwUInt32 dualIndex;

        team = player->team;
        RWASSERT(NULL != team);

        playerIndex = player - team->players;
        RWASSERT(team->data.numOfPlayers > playerIndex);

        materialTeamExt = TeamMaterialGetTeamIndex(materialData, team);
        if(NULL != materialTeamExt)
        {
            dualIndex = materialTeamExt->numOfPlayerSpecifics
                      - (playerIndex + 1);
            texture = TeamMaterialGetPlayerTexture(materialTeamExt, dualIndex);
        }
    }

    if(NULL == texture)
    {
        const rpMatFXMaterialData *materialData;
        const MatFXDualData *dualData;

        materialData = *MATFXMATERIALGETCONSTDATA(material);
        RWASSERT(NULL != materialData);

        dualData = &(materialData->data[rpSECONDPASS].data.dual);
        RWASSERT(NULL != dualData);

        texture = dualData->texture;
    }

    RWRETURN(texture);
}

/*---------------------------------------------------------------------------*/

/**
 * \ingroup rpteam
 * \ref RpTeamMaterialSetPlayerColor is used to set a player specific
 * material color. This allows the developer to use a single piece of
 * geometry and overload the material color that is used to render the
 * geometry.
 *
 * If no player specific material color is specified for an individual
 * player then the color is assumed to be white. If no material color is
 * specificed for any player then the generic material color is used when
 * rendering.
 *
 * \param material Pointer to the material.
 * \param player   Pointer to the player.
 * \param color    Pointer to the base pass material color.
 *
 * \return Returns a pointer to the material if successful or NULL if
 * there is an error.
 *
 * \see RpTeamGetPlayer
 * \see RpTeamMaterialGetPlayerColor
 * \see RpMaterialSetColor
 * \see RpMaterialGetColor
 */
RpMaterial *
RpTeamMaterialSetPlayerColor( RpMaterial *material,
                              const RpTeamPlayer *player,
                              const RwRGBA *color )
{
    TeamMaterialTeamExt *materialTeamExt;
    TeamMaterialData *materialData;

    RWAPIFUNCTION(RWSTRING("RpTeamMaterialSetPlayerColor"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != material);
    RWASSERT(NULL != player);

    materialData = TEAMMATERIALGETDATA(material);
    RWASSERT(NULL != materialData);

    materialTeamExt = TeamMaterialSetupMaterial(material, player->team);
    if(NULL != materialTeamExt)
    {
        RpTeam *team;
        RwUInt32 playerIndex;

        team = player->team;
        RWASSERT(NULL != team);

        playerIndex = player - team->players;
        RWASSERT(team->data.numOfPlayers > playerIndex);

        TeamMaterialSetPlayerColor(materialTeamExt, playerIndex, color);

        RWRETURN(material);
    }

    RWRETURN((RpMaterial *)NULL);
}

/**
 * \ingroup rpteam
 * \ref RpTeamMaterialGetPlayerColor is used to retrieve the player
 * specific material color.
 *
 * If no player specific material color is specified for an individual
 * player then the color is assumed to be white. If no material color is
 * specificed for any player then the generic material color is used when
 * rendering.
 *
 * \param material Pointer to the material.
 * \param player   Pointer to the player.
 *
 * \return Returns a pointer to the color if successful or NULL if there
 * is an error.
 *
 * \see RpTeamGetPlayer
 * \see RpTeamMaterialGetPlayerColor
 * \see RpMaterialSetColor
 * \see RpMaterialGetColor
 */
const RwRGBA *
RpTeamMaterialGetPlayerColor( const RpMaterial *material,
                              const RpTeamPlayer *player )
{
    const TeamMaterialData *materialData;
    const RwRGBA *color;

    RWAPIFUNCTION(RWSTRING("RpTeamMaterialGetPlayerColor"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != material);
    RWASSERT(NULL != player);

    materialData = TEAMMATERIALGETCONSTDATA(material);
    RWASSERT(NULL != materialData);

    color = (RwRGBA *)NULL;

    if(0 < materialData->numOfTeams)
    {
        TeamMaterialTeamExt *materialTeamExt;
        RpTeam *team;
        RwUInt32 playerIndex;

        team = player->team;
        RWASSERT(NULL != team);

        playerIndex = player - team->players;
        RWASSERT(team->data.numOfPlayers > playerIndex);

        materialTeamExt = TeamMaterialGetTeamIndex(materialData, team);
        if(NULL != materialTeamExt)
        {
            color = TeamMaterialGetPlayerColor( materialTeamExt,
                                                playerIndex );
        }
    }

    if(NULL == color)
    {
        color = RpMaterialGetColor(material);
    }

    RWRETURN(color);
}
