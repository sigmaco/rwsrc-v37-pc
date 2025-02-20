/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>
#include "rpdbgerr.h"

#include "rpteam.h"

#include "team.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/
#if (defined(RWDEBUG))
long rpTeamStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

RwModuleInfo TeamModule = { 0, 0 };

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/
typedef struct TeamEngineData TeamEngineData;
struct TeamEngineData
{
    RwLinkList  teams;
    RwUInt32    numOfTeams;
    RwBool      allOptimized;
};

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
static RwInt32 TeamEngineDataOffset = 0;

/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/
/*--- Engine data ---*/
#define TEAMENGINEGETDATA(engineInstance) \
    ((TeamEngineData *)(((RwUInt8 *)engineInstance) + TeamEngineDataOffset))

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Functions -----------------------------------------------------*
 *===========================================================================*/
void
_rpTeamAddTeamToList( RpTeam *team )
{
    TeamEngineData *engineData;

    RWFUNCTION(RWSTRING("_rpTeamAddTeamToList"));
    RWASSERT(NULL != team);

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe Check
	THREADSAFE_CHECK_ISCALLEDMAIN();
	//@} DDonSS

    engineData = TEAMENGINEGETDATA(RwEngineInstance);
    RWASSERT(NULL != engineData);

    engineData->numOfTeams++;
    engineData->allOptimized = FALSE;

    rwLinkListAddLLLink(&(engineData->teams), &(team->list));

    RWRETURNVOID();
}

void
_rpTeamRemoveTeamFromList( RpTeam *team )
{
    TeamEngineData *engineData;

    RWFUNCTION(RWSTRING("_rpTeamRemoveTeamFromList"));
    RWASSERT(NULL != team);

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe Check
	THREADSAFE_CHECK_ISCALLEDMAIN();
	//@} DDonSS

    engineData = TEAMENGINEGETDATA(RwEngineInstance);
    RWASSERT(NULL != engineData);

    RWASSERT(0 < engineData->numOfTeams);
    engineData->numOfTeams--;
    engineData->allOptimized = FALSE;

    rwLinkListRemoveLLLink(&(team->list));

    RWRETURNVOID();
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/
static void *
TeamOpen( void *instance __RWUNUSEDRELEASE__,
          RwInt32 offset __RWUNUSED__,
          RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("TeamOpen"));
    RWASSERT(NULL != instance);

    /* Create the team pipeline, only once */
    if(TeamModule.numInstances == 0)
    {
        TeamEngineData *engineData;
        RwBool ret;

        engineData = TEAMENGINEGETDATA(RwEngineInstance);

        engineData->numOfTeams = 0;
        engineData->allOptimized = FALSE;

        /* Create custom skin pipelines. */
        ret = _rpTeamSkinCustomPipelineCreate();
        RWASSERT(ret);

        /* Create custom static pipelines. */
        ret = _rpTeamStaticCustomPipelineCreate();
        RWASSERT(ret);

        /* Create custom shadow pipelines. */
        ret = _rpTeamShadowPipelineCreate();
        RWASSERT(ret);

        /* Create custom matfx pipeline. */
        ret = _rpTeamMatFXPipelineCreate();
        RWASSERT(ret);

        rwLinkListInitialize(&(engineData->teams));
    }

    /* one more module instance */
    TeamModule.numInstances++;

    RWRETURN(instance);
}

static void *
TeamClose( void *instance __RWUNUSEDRELEASE__,
           RwInt32 offset __RWUNUSED__,
           RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("TeamClose"));
    RWASSERT(NULL != instance);

    /* one less module instance */
    TeamModule.numInstances--;

    if(TeamModule.numInstances == 0)
    {
        TeamEngineData *engineData;

        engineData = TEAMENGINEGETDATA(RwEngineInstance);
        RWASSERT(NULL != engineData);

        /* Destroy custom skin pipelines. */
        _rpTeamSkinCustomPipelineDestroy();

        /* Destroy custom static pipelines. */
        _rpTeamStaticCustomPipelineDestroy();

        /* Destroy custom shadow pipelines. */
        _rpTeamShadowPipelineDestroy();

        /* Destroy custom matfx pipelines. */
        _rpTeamMatFXPipelineDestroy();

        RWASSERT(0 == engineData->numOfTeams);
    }

    RWRETURN(instance);
}

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/


/**
 * \ingroup rpteam
 * \ref RpTeamPluginAttach is called by the application to indicate that the
 * team plugin should be attached. The call to this function should be placed
 * between \ref RwEngineInit and \ref RwEngineOpen. The team plugin uses
 * functionality from the following plugins, whilst must also be attached:
 * \ref scenemanagement, \ref rphanim, \ref rpmatfx and \ref rpskin.
 *
 * \return True on success, false otherwise.
 *
 * \see RpTeamCreate
 * \see RpTeamGetDictionary
 * \see RpTeamGetPlayer
 * \see RpTeamRender
 * \see RpTeamLock
 * \see RpTeamUnlock
 * \see RpWorldPluginAttach
 * \see RpSkinPluginAttach
 * \see RpMatFXPluginAttach
 */
RwBool
RpTeamPluginAttach( void )
{
    RwInt32 success;

    RWAPIFUNCTION(RWSTRING("RpTeamPluginAttach"));

    /* Register engine plugIn */
    TeamEngineDataOffset =
        RwEngineRegisterPlugin( sizeof(TeamEngineData),
                                rwID_TEAMPLUGIN,
                                TeamOpen,
                                TeamClose );
    if(TeamEngineDataOffset < 0)
    {
        RWRETURN(FALSE);
    }

    /* Register material plugIn */
    TeamMaterialDataOffset =
        RpMaterialRegisterPlugin( sizeof(TeamMaterialData),
                                  rwID_TEAMPLUGIN,
                                  _rpTeamMaterialConstructor,
                                  _rpTeamMaterialDestructor,
                                  _rpTeamMaterialCopy );

    if (TeamMaterialDataOffset < 0)
    {
        RWRETURN(FALSE);
    }

    /* At the moment we use rwID_TEAMPLUGIN, but lets not forget about
     * rwID_TEAMDICTIONARYPLUGIN which is actually used for the
     * dictionary chunks.
     */
    /* Register an atomic extension with zero size, this allows us
     * to register a rights callback.
     */
    success = RpAtomicRegisterPlugin( 0,
                                      rwID_TEAMPLUGIN,
                                      (RwPluginObjectConstructor)NULL,
                                      (RwPluginObjectDestructor)NULL,
                                      (RwPluginObjectCopy)NULL );
    RWASSERT(0 < success);

    /* Attach a rights callback streaming function. */
    success = RpAtomicSetStreamRightsCallBack( rwID_TEAMPLUGIN,
                                               _rpTeamDictionaryAtomicRights );
    RWASSERT(0 < success);

    /* Hurrah */
    RWRETURN(TRUE);
}

/**
 * \ingroup rpteam
 * \page rpteamoverview RpTeam Plugin Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rpmatfx.h, rphanim.h, rpskin.h, rpteam.h
 * \li \b Libraries: rwcore, rpworld, rpmatfx, rphanim, rpskin, rtquat, rpteam
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpMatFXPluginAttach,
 *     \ref RpHAnimPluginAttach, \ref RpSkinPluginAttach, \ref RpTeamPluginAttach
 *
 * \subsection overview Overview
 * RpTeam is provided to develop teams of players which are similar in
 * construction. The plugin is designed to optimize the rendering of a
 * team of players.
 *
 * Before any of the plugin functions can be used, \ref RpTeamPluginAttach
 * must be called to attach the plugin. The team plugin also uses
 * functionality from the following plugins which must also be attached:
 * RpWorld, RpHAnim, RpMatFX and RpSkin.
 *
 * The RpTeam plugin contains:
 * \li Teams - collection of players
 * \li Players - constructed from dictionary elements
 * \li Dictionaries - contain an array of dictionary elements
 * \li Shadows - can be attached to individual players or teams
 * \li Levels of Detail (LOD) - defined per dictionary element
 *
 * \par Creating a Team
 * -# \ref RpTeamPluginAttach attaches the team plugin.
 * -# \ref RpTeamCreate creates a team consisting of players and player
 *    elements. The team is locked by default.
 * -# \ref RpTeamDictionaryCreate creates a team dictionary.
 * -# \ref RpTeamSetDictionary attaches the team dictionary to the team.
 * -# \ref RpTeamDictionarySetLODElement adds LOD atomics to dictionary elements.
 * -# \ref RpTeamPlayerSetElement assigns dictionary elements to players.
 * -# \ref RpTeamSetShadows sets the shadows for the team.
 * -# \ref RpTeamUnlock unlocks the team.
 * -# \ref RpTeamRender renders the team.
 *
 * \par Dictionaries
 *
 * An RpTeamDictionary contains an array of dictionary elements to construct
 * the team's players. Dictionary elements are set by calling
 * \ref RpTeamDictionarySetLODElement.
 *
 * \par Players
 *
 * Each player is generated and rendered by selecting elements from the
 * dictionary. Each player contains an array of indices which reference
 * dictionary elements.
 * \li \ref RpTeamPlayer represents a player in a team
 * \li \ref RpTeamPlayerSetElement references a dictionary element to the player
 * \li \ref RpTeamPlayerSetHierarchyFrame attaches a hierarchical frame to a
 *     player which is used to define skinning and rigid element positions
 * \li \ref RpTeamMaterialSetTeamTexture sets a team specific base texture
 * \li \ref RpTeamMaterialSetPlayerTexture overloads the material's base texture
 *     with a player specific texture
 * \li \ref RpTeamMaterialSetPlayerDualTexture overloads the material's dual pass
 *     texture with a player specific texture
 * \li \ref RpTeamPlayerGetBoundingSphere is used to get the player's bounding
 *     sphere.
 * \li \ref RpTeamPlayerSetActivity is used to determine if the player should be
 *     considered for rendering.
 * \li \ref RpTeamPlayerSetRenderCallBack is used to setup a player's render
 *     callback and to overload player specific data.
 *
 * \par Shadows
 *
 * Shadows can be assigned per player and per team.
 * \li RpTeamShadows data structure contains the data for rendering a
 *     team's player shadow.
 * \li \ref RpTeamSetShadows sets the shadow data for a team.
 * \li \ref RpTeamPlayerSetShadows sets shadow data for an individual player
 *
 * \par Level of Detail
 *
 * Each player has a current LOD.
 * \li \ref RpTeamPlayerSetSyncLODPerFrame calculates a player's LOD per frame.
 * \li \ref RpTeamPlayerSetCurrentLOD sets a player's current LOD.
 *
 * \par Other Documents
 *  For more information about the RpTeam plugin refer to the \e Team
 * \e Chapter in the User Guide.
 *
 */
