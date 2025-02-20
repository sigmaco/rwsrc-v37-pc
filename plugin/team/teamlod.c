/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>
#include "rpdbgerr.h"

#include "rpteam.h"

#include "teamlod.h"
#include "team.h"

/*
 *

     .......                  ...........       ................
    =======.                 ===========..     ================..
    =======.                =============..    =================..
    =======/               ===============..   ==================.
     =====.               =====.      =====.    =====.      =====.
     =====.               =====.      =====.    =====.      =====.
     =====.               =====.      =====.    =====.      =====.
     =====.               =====.      =====.    =====.      =====.
     =====.       .....   =====.      =====.    =====.      =====.
     =====.......=====.   =====.......=====/    =====.......=====/
    ==================.    ===============/    =================/
    ==================.     =============/     ================/
    ==================.      ===========/      ===============/
                 =====/
 *
 */

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

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

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/
RwUInt32
_rpTeamLODNumOf(TeamLODAtomic element)
{
    RwUInt32 numOfLOD;
    RwUInt32 i;

    RWFUNCTION(RWSTRING("_rpTeamLODNumOf"));
    RWASSERT(element);

    for( i = 0, numOfLOD = 0; i < rpTEAMELEMENTLODSUP; i++ )
    {
        if( NULL != element[i] )
        {
            numOfLOD++;
        }
    }

    RWRETURN(numOfLOD);
}

RwUInt32
_rpTeamPlayerLODDefaultSelectLODCallBack( const RpTeam  *team,
                                          RwUInt32       playerIndex,
                                          RpTeamLODData  data __RWUNUSED__ )
{
    RpTeamPlayer *player;
    RpTeamLODCamera camera;
    RwFrame *cameraFrame;
    RwMatrix *cameraMatrix;
    RwMatrix *playerMatrix;

    RwV3d playerPosition;
    RwV3d cameraPosition;
    RwV3d cameraAt;

    RwV3d positionDelta;
    RwReal angle;

    RwInt32 result;
    RwUInt32 lod;

    RWFUNCTION(RWSTRING("_rpTeamPlayerLODDefaultSelectLODCallBack"));
    RWASSERT(NULL != team);
    RWASSERT(0 < team->lod.farRange);
    RWASSERT(0 < team->lod.invFarRange);
    RWASSERT(team->data.numOfPlayers > playerIndex);

    /* Get the player. */
    player = &(team->players[playerIndex]);
    RWASSERT(NULL != player);
    RWASSERT(NULL != player->frame);

    /* Do we have a special camera for the team? */
    if(NULL == team->lod.camera)
    {
        /* No. Then use the current one. */
        camera = RwCameraGetCurrentCamera();
    }
    else
    {
        /* Yes. Then we'll use it. */
        camera = team->lod.camera;
    }

    RWASSERT(NULL != camera);
    cameraFrame = RwCameraGetFrame(camera);
    RWASSERT(NULL != cameraFrame);
    cameraMatrix = TeamFrameGetLTM(cameraFrame);
    RWASSERT(NULL != cameraMatrix);
    playerMatrix = TeamFrameGetLTM(player->frame);
    RWASSERT(NULL != playerMatrix);

    /* Right lets calculate how far the player if from the camera. */
    playerPosition = playerMatrix->pos;
    cameraPosition = cameraMatrix->pos;
    cameraAt       = cameraMatrix->at;

    RwV3dSub(&positionDelta, &playerPosition, &cameraPosition);
    angle = RwV3dDotProduct(&positionDelta, &cameraAt);

    result = RwInt32FromRealMacro(angle * team->lod.invFarRange);

    /* Check we're not naughty. */
    if( 0 > result )
    {
        lod = 0;
    }
    else if( rpTEAMELEMENTLODMAX <= result )
    {
        lod = rpTEAMELEMENTLODMAX - 1;
    }
    else
    {
        lod = (RwUInt32)result;
    }

    RWASSERT(rpTEAMELEMENTLODMAX > lod);

    RWRETURN(lod);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rpteam
 * \ref RpTeamSetLODCamera is called to define the \ref RwCamera which the
 * default LOD callback uses as the base reference frame to calculate the LOD
 * during \ref RpTeamRender. The position of the camera's frame defines the
 * origin of the LOD calculation.
 *
 * If no camera is specified the default LOD callback will use the current
 * camera, see \ref RwCameraGetCurrentCamera.
 *
 * The team should be locked before any level of detail setup is changed
 * with \ref RpTeamLockMode rpTEAMLOCKTEAMLOD.
 *
 * \param team   Pointer to the team.
 * \param camera Pointer to the camera for the default LOD test to use.
 *
 * \return Returns a pointer to the team if successful or NULL if there is an
 * error.
 *
 * \see RpTeamLock
 * \see RpTeamGetLODCamera
 * \see RpTeamSetLODFarRange
 * \see RpTeamSetLODCallBack
 */
RpTeam *
RpTeamSetLODCamera( RpTeam *team,
                    RpTeamLODCamera camera )
{
    RWAPIFUNCTION(RWSTRING("RpTeamSetLODCamera"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != team);
    RWASSERT(team->lockFlags & rpTEAMLOCKTEAMLOD);

    team->lod.camera = camera;

    RWRETURN(team);
}

/**
 * \ingroup rpteam
 * \ref RpTeamSetLODFarRange is called to define the range of the default
 * LOD callback function. The default LOD function calculates the z distance
 * of a player from the LOD camera's frame. The z distance from the camera's
 * frame is split evenly between the LODs up to the farRange. Any players
 * further from the reference \ref RwCamera than the farRange will receive
 * the maximum level of detail value.
 *
 * \verbatim
      / \
       |
       |  LOD 0 LOD 1 LOD 2  ...  LOD max
      +-> -----|-----|-----|-----|-----|-------->
     /                               farRange
    L
   \endverbatim
 *
 * The team should be locked before any level of detail setup is changed
 * with \ref RpTeamLockMode rpTEAMLOCKTEAMLOD.
 *
 * \param team     Pointer to the team.
 * \param farRange Range of the furthest LOD.
 *
 * \return Returns a pointer to the team if successful or NULL if there is an
 * error.
 *
 * \see RpTeamLock
 * \see RpTeamGetLODFarRange
 * \see RpTeamSetLODCamera
 * \see RpTeamSetLODCallBack
 */
RpTeam *
RpTeamSetLODFarRange( RpTeam *team,
                      RwReal farRange )
{
    RWAPIFUNCTION(RWSTRING("RpTeamSetLODFarRange"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != team);
    RWASSERT(team->lockFlags & rpTEAMLOCKTEAMLOD);
    RWASSERT(0 < farRange);

    team->lod.farRange = farRange;
    team->lod.invFarRange = ((RwReal) rpTEAMELEMENTLODMAX / farRange);

    RWRETURN(team);
}

/**
 * \ingroup rpteam
 * \ref RpTeamSetLODCallBack is called to overload the default LOD
 * calculation callback function. The callback is called each frame
 * for all the players who have the sync per frame enabled,
 * \ref RpTeamPlayerSetSyncLODPerFrame.
 *
 * If the default LOD callback is not overloaded then a camera
 * \ref RpTeamSetLODCamera and far range \ref RpTeamSetLODFarRange should
 * be setup.
 *
 * The team should be locked before any level of detail setup is changed
 * with \ref RpTeamLockMode rpTEAMLOCKTEAMLOD.
 *
 * \param team      Pointer to the team.
 * \param callBack  Pointer to the LOD callback function.
 * \param data      Pointer to user data which is passed to the callback
 *                  function.
 *
 * \return Returns a pointer to the team if successful or NULL if there is an
 * error.
 *
 * \see RpTeamLock
 * \see RpTeamGetLODCallBack
 */
RpTeam *
RpTeamSetLODCallBack( RpTeam *team,
                      RpTeamLODCallBack callBack,
                      RpTeamLODData data )
{
    RWAPIFUNCTION(RWSTRING("RpTeamSetLODCallBack"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != team);
    RWASSERT(team->lockFlags & rpTEAMLOCKTEAMLOD);

    team->lod.callBack = callBack;
    team->lod.data = data;

    RWRETURN(team);
}

/**
 * \ingroup rpteam
 * \ref RpTeamGetLODCamera is called to return the camera used by the default
 * LOD callback function to calculate a players LOD. The camera is set with
 * \ref RpTeamSetLODCamera.
 *
 * \param team Pointer to the team.
 *
 * \return Returns a pointer to the camera if successful or NULL if there is an
 * error.
 *
 * \see RpTeamSetLODCamera
 * \see RpTeamSetLODFarRange
 * \see RpTeamGetLODFarRange
 */
RpTeamLODCamera
RpTeamGetLODCamera( const RpTeam *team )
{
    RpTeamLODCamera camera;

    RWAPIFUNCTION(RWSTRING("RpTeamGetLODCamera"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != team);

    camera = team->lod.camera;

    RWRETURN(camera);
}

/**
 * \ingroup rpteam
 * \ref RpTeamGetLODFarRange is called to return the far range used by the
 * default LOD callback function to calculate a players LOD. The far range
 * is set with \ref RpTeamSetLODFarRange.
 *
 * \param team Pointer to the team.
 *
 * \return Returns the far range of the default LOD callback.
 *
 * \see RpTeamSetLODCamera
 * \see RpTeamGetLODCamera
 * \see RpTeamSetLODFarRange
 */
RwReal
RpTeamGetLODFarRange( const RpTeam *team )
{
    RwReal farRange;

    RWAPIFUNCTION(RWSTRING("RpTeamGetLODFarRange"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != team);

    farRange = team->lod.farRange;

    RWRETURN(farRange);
}

/**
 * \ingroup rpteam
 * \ref RpTeamGetLODCallBack is called to retrieve the team's current,
 * calculate player LOD callback function.
 *
 * \param team     Pointer to the team.
 * \param callBack Pointer to receive the callback function.
 * \param data     Pointer to receive the user data passed to the callback
 *                 function.
 *
 * \return Returns a pointer to the team if successful or NULL if there is an
 * error.
 *
 * \see RpTeamSetLODCallBack
 */
const RpTeam *
RpTeamGetLODCallBack( const RpTeam *team,
                      RpTeamLODCallBack *callBack,
                      RpTeamLODData *data )
{
    RWAPIFUNCTION(RWSTRING("RpTeamGetLODCallBack"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != team);

    if( NULL != callBack )
    {
        *callBack = team->lod.callBack;
    }

    if( NULL != data )
    {
        *data = team->lod.data;
    }

    RWRETURN(team);
}

/**
 * \ingroup rpteam
 * \ref RpTeamPlayerSetCurrentLOD is called to directly set a
 * \ref RpTeamPlayer's current LOD. If a player doesn't have it's LOD
 * calculated during \ref RpTeamRender via the LOD callback function, then
 * this function should be used.
 *
 * Any LOD value set will persist until the LOD function is called or the
 * user updates it. Therefore, it is not necessary to change the LOD values each
 * frame.
 *
 * The team should be locked before any individual player level of detail
 * are changed with \ref RpTeamLockMode rpTEAMLOCKPLAYERLODS.
 *
 * \param player Pointer to the player to set the LOD.
 * \param lod    Level of detail of the player.
 *
 * \return Returns a pointer to the player if successful or NULL if there is an
 * error.
 *
 * \see RpTeamGetPlayer
 * \see RpTeamLock
 * \see RpTeamPlayerGetCurrentLOD
 * \see RpTeamPlayerSetSyncLODPerFrame
 * \see RpTeamPlayerGetSyncLODPerFrame
 */
extern RpTeamPlayer *
RpTeamPlayerSetCurrentLOD( RpTeamPlayer *player,
                           RwUInt32 lod )
{
    RWAPIFUNCTION(RWSTRING("RpTeamPlayerSetCurrentLOD"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != player);
    RWASSERT(NULL != player->team);
    RWASSERT(player->team->lockFlags & rpTEAMLOCKPLAYERLODS);
    RWASSERT(rpTEAMELEMENTLODMAX > lod);

    player->lod.currentLOD = lod;

    RWRETURN(player);
}

/**
 * \ingroup rpteam
 * \ref RpTeamPlayerGetCurrentLOD is called to return the \ref RpTeamPlayer's
 * current LOD. If a player's LOD is not updated because the player has
 * been removed from the LOD sync list with \ref RpTeamPlayerSetSyncLODPerFrame
 * then the LOD value will be used when the player's parts are rendered.
 *
 * \param player Pointer to the player to query.
 *
 * \return Returns the player's current LOD.
 *
 * \see RpTeamGetPlayer
 * \see RpTeamLock
 * \see RpTeamPlayerSetCurrentLOD
 * \see RpTeamPlayerSetSyncLODPerFrame
 * \see RpTeamPlayerGetSyncLODPerFrame
 */
extern RwUInt32
RpTeamPlayerGetCurrentLOD( const RpTeamPlayer *player )
{
    RwUInt32 lod;

    RWAPIFUNCTION(RWSTRING("RpTeamPlayerGetCurrentLOD"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != player);

    lod = player->lod.currentLOD;
    RWASSERT(rpTEAMELEMENTLODMAX > lod);

    RWRETURN(lod);
}

/**
 * \ingroup rpteam
 * \ref RpTeamPlayerSetSyncLODPerFrame is called to specify if a
 * \ref RpTeamPlayer should have their LOD updated during \ref RpTeamRender
 * with the LOD callback function.
 *
 * The team should be locked before any individual players level of detail
 * setup is changed with \ref RpTeamLockMode rpTEAMLOCKPLAYERLODS.
 *
 * \param player          Pointer to the player.
 * \param syncLODPerFrame TRUE is the player should have their LOD
 *                        automatically updated, and FALSE otherwise.
 *
 * \return Returns a pointer to the player if successful or NULL if there is an
 * error.
 *
 * \see RpTeamGetPlayer
 * \see RpTeamLock
 * \see RpTeamPlayerSetCurrentLOD
 * \see RpTeamPlayerGetCurrentLOD
 * \see RpTeamPlayerGetSyncLODPerFrame
 */
extern RpTeamPlayer *
RpTeamPlayerSetSyncLODPerFrame( RpTeamPlayer *player,
                                RwBool syncLODPerFrame )
{
    RWAPIFUNCTION(RWSTRING("RpTeamPlayerSetSyncLODPerFrame"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != player);
    RWASSERT(NULL != player->team);
    RWASSERT(player->team->lockFlags & rpTEAMLOCKPLAYERLODS);

    player->lod.syncLODPerFrame = syncLODPerFrame;

    RWRETURN(player);
}

/**
 * \ingroup rpteam
 * \ref RpTeamPlayerGetSyncLODPerFrame is called to query if a
 * \ref RpTeamPlayer will have their LOD updated during \ref RpTeamRender
 * with the LOD callback function.
 *
 * \param player Pointer to the player to query.
 *
 * \return Returns TRUE if the player's LOD is automatically update
 * with the callback or FALSE otherwise.
 *
 * \see RpTeamGetPlayer
 * \see RpTeamLock
 * \see RpTeamPlayerSetCurrentLOD
 * \see RpTeamPlayerGetCurrentLOD
 * \see RpTeamPlayerSetSyncLODPerFrame
 */
extern RwBool
RpTeamPlayerGetSyncLODPerFrame( RpTeamPlayer *player )
{
    RwBool syncLODPerFrame;

    RWAPIFUNCTION(RWSTRING("RpTeamPlayerGetSyncLODPerFrame"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != player);

    syncLODPerFrame = player->lod.syncLODPerFrame;

    RWRETURN(syncLODPerFrame);
}
