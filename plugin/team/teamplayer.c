/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>
#include "rpdbgerr.h"

#include "rpteam.h"

#include "teamplayer.h"
#include "team.h"

/*
 *

     ........    ...           ......     ...   ...   ........   ........
    ========..  ===.          ======..   ===.  ===.  ========.  ========..
    =========.  ===.         ========..  ===.  ===.  ========.  =========.
     ===. ===.  ===.        ===.   ===.  ===.  ===.  ===. ===/   ===. ===.
     ===..===.  ===.        ===....===.  ===.  ===/  ===...      ===..===.
     ========/  ===.        ==========.   ===.===/   =====.      ========/
     =======/   ===.        ==========.    =====/    =====/      =======.
     ===.       ===.  ...   ===.   ===.     ===.     ===.  ...   ===. ===.
     ===..      ===..===.   ===..  ===..    ===..    ===..===.   ===.. ===.
    =====.      ========.  =====. =====.   =====.    ========.  =====.  ===.
    =====/      ========/  =====/ =====/   =====/    ========/  =====/  ===/

 *
 */

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/
#if (defined(TEAMGLOBALPOINTS))
const RpTeamPlayer *_rpTeamPlayerRenderCurrent;
#endif /* (defined(TEAMGLOBALPOINTS)) */

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
static RpTeamPlayer *
TeamPlayerSetDictionarySelectionCount( RpTeamPlayer *player,
                                       RwBool active )
{
    RwUInt32 unoptimize;
    RwUInt32 i;

    RWFUNCTION(RWSTRING("TeamPlayerSetDictionarySelectionCount"));
    RWASSERT(NULL != player);

    unoptimize = rpNATEAMELEMENTTYPE;

    for( i = 0; i < player->team->data.numOfPlayerElements; i++ )
    {
        RwUInt32 index;

        index = player->elements[i];

        if( rpTEAMELEMENTUNUSED != index )
        {
            RpTeamDictionaryElement *element;

            RWASSERT(player->team->dictionary->numOfElements > index);

            element = &(player->team->dictionary->elements[index]);

            if( (RwBool)TRUE == active )
            {
                player->team->selected[index]++;
            }
            else
            {
                player->team->selected[index]--;
            }

            /* Remember this type as changed. */
            unoptimize |= element->type;

            RWASSERT(element->refCount >= player->team->selected[index]);
        }
    }

    /* Flag the elements as no longer optimized. */
    _rpTeamSetElementTypeOptimized( player->team, unoptimize,
                                    rpNATEAMLOCKMODE, FALSE );

    RWRETURN(player);
}

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/
RpTeamPlayer *
_rpTeamPlayerDestroy( RpTeamPlayer *player )
{
    RwUInt32 i;

    RWFUNCTION(RWSTRING("_rpTeamPlayerDestroy"));
    RWASSERT(NULL != player);
    RWASSERT(NULL != player->team);

    /* Destroy the player's frame. */
    if( NULL != player->frame )
    {
        RwFrameDestroyHierarchy(player->frame->root);
    }

    /* If we've a dictionary deselect the player's elements. */
    if( NULL != player->team->dictionary )
    {
        /* Deselect any elements. */
        for( i = 0; i < player->team->data.numOfPlayerElements; i++ )
        {
            player = RpTeamPlayerSetElement(player, i, rpTEAMELEMENTUNUSED);
            RWASSERT(NULL != player);
        }
    }

    /* Don't need this for the double cache. */
#if(!defined(TEAMMATRIXCACHECACHE))
    _rpTeamSkinDestroyMatrixCache(&(player->skinCache));
#endif

    RWRETURN(player);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rpteam
 * \ref RpTeamPlayerSetElement is used to add a dictionary element to the
 * player. The players are constructed by referencing dictionary elements.
 * When the player is rendered, all the dictionary elements that they
 * are constructed from will be positioned and rendered.
 *
 * To remove a dictionary element from the players constuction set
 * rpTEAMELEMENTUNUSED for the dictionary index.
 *
 * The team should be locked before any player elements are altered with
 * \ref RpTeamLockMode rpTEAMLOCKPLAYERS.
 *
 * \param player          Pointer to the player.
 * \param index           Index of the player elements.
 * \param dictionaryIndex Dictionary index of the element to set.
 *
 * \return Returns a pointer to the player if successful or NULL if there is an
 * error.
 *
 * \see RpTeamLock
 * \see RpTeamUnlock
 * \see RpTeamGetNumberOfPlayers
 * \see RpTeamGetPlayer
 * \see RpTeamPlayerGetElement
 * \see RpTeamPlayerSetHierarchyFrame
 * \see RpTeamPlayerSetActivity
 * \see RpTeamPlayerSetRenderCallBack
 * \see RpTeamPlayerSetCurrentLOD
 * \see RpTeamPlayerSetSyncLODPerFrame
 */
RpTeamPlayer *
RpTeamPlayerSetElement( RpTeamPlayer *player,
                        RwUInt32 index,
                        RwUInt32 dictionaryIndex )
{
    RpTeamPlayer     *ret;
    RpTeamDictionary *dictionary;

    RWAPIFUNCTION(RWSTRING("RpTeamPlayerSetElement"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != player);
    RWASSERT(NULL != player->team);
    RWASSERT(player->team->lockFlags & rpTEAMLOCKPLAYERS);

    dictionary = player->team->dictionary;
    RWASSERT(NULL != dictionary);

    RWASSERT(player->team->data.numOfPlayerElements > index);

    if( rpTEAMELEMENTUNUSED != dictionaryIndex )
    {
        RWASSERT(dictionary->numOfElements > dictionaryIndex);
        player->elementFlags |= dictionary->elements[dictionaryIndex].type;
    }

    ret = _rpTeamDictionarySetElement( player, index, dictionaryIndex );

    RWRETURN(ret);
}

/**
 * \ingroup rpteam
 * \ref RpTeamPlayerGetElement is used to return the dictionary index for
 * the element that the player is constructed from.
 *
 * \param player Pointer to the player to query.
 * \param index  Index of the player's elements.
 *
 * \return Returns the dictionary index of the element.
 *
 * \see RpTeamGetNumberOfPlayers
 * \see RpTeamGetPlayer
 * \see RpTeamPlayerSetElement
 * \see RpTeamPlayerGetHierarchyFrame
 * \see RpTeamPlayerGetActivity
 * \see RpTeamPlayerGetRenderCallBack
 * \see RpTeamPlayerGetBoundingSphere
 * \see RpTeamPlayerGetCurrentLOD
 * \see RpTeamPlayerGetSyncLODPerFrame
 */
RwUInt32
RpTeamPlayerGetElement( const RpTeamPlayer *player,
                        RwUInt32 index )
{
    RwUInt32 ret;

    RWAPIFUNCTION(RWSTRING("RpTeamPlayerGetElement"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != player);
    RWASSERT(NULL != player->team);
    RWASSERT(player->team->data.numOfPlayerElements > index);

    ret = player->elements[index];

    RWRETURN(ret);
}

/**
 * \ingroup rpteam
 * \ref RpTeamPlayerSetHierarchyFrame is used to attach a hierarchy frame to
 * a player. Each player needs a unique hierarchy to define the skinning and
 * rigid element positions.
 *
 * The team should be locked before any players hierarchy frame setup is
 * altered with \ref RpTeamLockMode rpTEAMLOCKPLAYERS.
 *
 * \param player Pointer to the player.
 * \param frame  Pointer to the frame hierarchy to set.
 *
 * \return Returns a pointer to the player if successful or NULL if there is an
 * error.
 *
 * \see RpTeamLock
 * \see RpTeamUnlock
 * \see RpTeamGetNumberOfPlayers
 * \see RpTeamGetPlayer
 * \see RpTeamPlayerSetElement
 * \see RpTeamPlayerGetHierarchyFrame
 * \see RpTeamPlayerSetActivity
 * \see RpTeamPlayerSetRenderCallBack
 * \see RpTeamPlayerSetCurrentLOD
 * \see RpTeamPlayerSetSyncLODPerFrame
 */
RpTeamPlayer *
RpTeamPlayerSetHierarchyFrame( RpTeamPlayer *player,
                               RwFrame *frame )
{
    RWAPIFUNCTION(RWSTRING("RpTeamPlayerSetHierarchyFrame"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != player);
    RWASSERT(player->team->lockFlags & rpTEAMLOCKPLAYERS);

    /* Do we presently have a hierarchy? */
    if( NULL != player->frame )
    {
        /* Yes - then lets destroy it. */
        RwFrameDestroyHierarchy(player->frame->root);

        player->frame = (RwFrame *)NULL;

        /* Don't need this for the double cache. */
#if(!defined(TEAMMATRIXCACHECACHE))
        _rpTeamSkinDestroyMatrixCache(&(player->skinCache));
#endif
    }

    /* Do we have a new one to set? */
    if( NULL != frame )
    {
#if(!defined(TEAMMATRIXCACHECACHE))
        RpHAnimHierarchy *hierarchy;
#endif
        /* Copy the hierarchy. */
        player->frame = frame;
        RWASSERT(NULL != player->frame);

        /* Don't need this for the double cache. */
#if(!defined(TEAMMATRIXCACHECACHE))
        hierarchy = RpHAnimFrameGetHierarchy(frame);
        RWASSERT(NULL != hierarchy);

        _rpTeamSkinCreateMatrixCache(&(player->skinCache), hierarchy);
#endif
    }

    RWRETURN(player);
}

/**
 * \ingroup rpteam
 * \ref RpTeamPlayerGetHierarchyFrame is used to return the hierarchy frame
 * for the player.
 *
 * \param player Pointer to the player to query.
 *
 * \return Returns a pointer to the frame if successful or NULL if there is an
 * error.
 *
 * \see RpTeamGetNumberOfPlayers
 * \see RpTeamGetPlayer
 * \see RpTeamPlayerGetElement
 * \see RpTeamPlayerSetHierarchyFrame
 * \see RpTeamPlayerGetActivity
 * \see RpTeamPlayerGetRenderCallBack
 * \see RpTeamPlayerGetBoundingSphere
 * \see RpTeamPlayerGetCurrentLOD
 * \see RpTeamPlayerGetSyncLODPerFrame
 */
RwFrame *
RpTeamPlayerGetHierarchyFrame( const RpTeamPlayer *player )
{
    RWAPIFUNCTION(RWSTRING("RpTeamPlayerGetHierarchyFrame"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != player);

    RWRETURN(player->frame);
}

/**
 * \ingroup rpteam
 * \ref RpTeamPlayerSetActivity is used to define if the player should be
 * rendered. Hence it is possible to define a whole team of players and only
 * render the players who are actually 'playing'.
 *
 * The team should be locked before any players activity is altered with
 * \ref RpTeamLockMode rpTEAMLOCKPLAYERS.
 *
 * \param player Pointer to the player.
 * \param active Player's selection.
 *
 * \return Returns a pointer to the player if successful or NULL if there is an
 * error.
 *
 * \see RpTeamLock
 * \see RpTeamUnlock
 * \see RpTeamGetNumberOfPlayers
 * \see RpTeamGetPlayer
 * \see RpTeamPlayerSetElement
 * \see RpTeamPlayerSetHierarchyFrame
 * \see RpTeamPlayerGetActivity
 * \see RpTeamPlayerSetRenderCallBack
 * \see RpTeamPlayerSetCurrentLOD
 * \see RpTeamPlayerSetSyncLODPerFrame
 */
RpTeamPlayer *
RpTeamPlayerSetActivity( RpTeamPlayer *player,
                         RwBool active )
{
    RWAPIFUNCTION(RWSTRING("RpTeamPlayerSetActivity"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != player);
    RWASSERT(player->team->lockFlags & rpTEAMLOCKPLAYERS);

    if( (FALSE == active) && (player->flags & TEAMPLAYERSELECTED) )
    {
        player->flags &= ~TEAMPLAYERSELECTED;
    }
    else if( ((RwBool)TRUE == active) &&
             !(player->flags & TEAMPLAYERSELECTED) )
    {
        player->flags |= TEAMPLAYERSELECTED;
    }
    else
    {
        RWRETURN((RpTeamPlayer *)NULL);
    }

    TeamPlayerSetDictionarySelectionCount(player, active);

    RWRETURN(player);
}

/**
 * \ingroup rpteam
 * \ref RpTeamPlayerGetActivity is used to return if the player is set to be
 * rendered.
 *
 * The team should be locked before any players activity is altered with
 * \ref RpTeamLockMode rpTEAMLOCKPLAYERS.
 *
 * \param player Pointer to the player to query.
 *
 * \return Returns TRUE if the player is active and FALSE if not.
 *
 * \see RpTeamGetNumberOfPlayers
 * \see RpTeamGetPlayer
 * \see RpTeamPlayerSetActivity
 * \see RpTeamPlayerGetElement
 * \see RpTeamPlayerGetHierarchyFrame
 * \see RpTeamPlayerGetRenderCallBack
 * \see RpTeamPlayerGetBoundingSphere
 * \see RpTeamPlayerGetCurrentLOD
 * \see RpTeamPlayerGetSyncLODPerFrame
 */
RwBool
RpTeamPlayerGetActivity( const RpTeamPlayer *player )
{
    RwBool ret;

    RWAPIFUNCTION(RWSTRING("RpTeamPlayerGetActivity"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != player);

    ret = (0 != (player->flags & TEAMPLAYERSELECTED));

    RWRETURN(ret);
}

/**
 * \ingroup rpteam
 * \ref RpTeamPlayerSetRenderCallBack is used to setup a players render
 * callback. The team which the player is a member of must be locked with
 * \ref RpTeamLockMode rpTEAMLOCKPLAYERS. When an element in the dictionary,
 * that the player is generated from, is rendered the callbacks will be called
 * to allow the user to over load features of the atomic.
 *
 * If the pre-render callback function returns NULL then the post-render call
 * back will be not be processed.
 *
 * \param player       Pointer to the player.
 * \param preCallBack  Pre-render callback function, or NULL if there is
 *                     no pre-render function.
 * \param postCallBack Post-render callback function, or NULL if there is
 *                     no post-render function.
 * \param data         Pointer to user data which is passed to the callbacks.
 *
 * \return Returns a pointer to the player if successful or NULL if there is an
 * error.
 *
 * \see RpTeamLock
 * \see RpTeamUnlock
 * \see RpTeamGetNumberOfPlayers
 * \see RpTeamGetPlayer
 * \see RpTeamLock
 * \see RpTeamUnlock
 * \see RpTeamPlayerSetElement
 * \see RpTeamPlayerSetHierarchyFrame
 * \see RpTeamPlayerSetActivity
 * \see RpTeamPlayerSetRenderCallBack
 * \see RpTeamPlayerSetCurrentLOD
 * \see RpTeamPlayerSetSyncLODPerFrame
 */
RpTeamPlayer *
RpTeamPlayerSetRenderCallBack( RpTeamPlayer *player,
                               RpTeamPlayerRenderCallBack preCallBack,
                               RpTeamPlayerRenderCallBack postCallBack,
                               RpTeamRenderData data )
{
    RWAPIFUNCTION(RWSTRING("RpTeamPlayerSetRenderCallBack"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != player);

    player->callBack.preRenderCallBack = preCallBack;
    player->callBack.postRenderCallBack = postCallBack;
    player->callBack.renderData = data;

    RWRETURN(player);
}

/**
 * \ingroup rpteam
 * \ref RpTeamPlayerGetRenderCallBack is used to get the players render
 * callback set with \ref RpTeamPlayerSetRenderCallBack.
 *
 * \param player       Pointer to the player.
 * \param preCallBack  Pointer to the pre-render callback function.
 * \param postCallBack Pointer to the post-render callback function.
 * \param data         Pointer to the user data.
 *
 * \return Returns a pointer to the player if successful or NULL if there
 * is an error.
 *
 * \see RpTeamLock
 * \see RpTeamUnlock
 * \see RpTeamGetNumberOfPlayers
 * \see RpTeamGetPlayer
 * \see RpTeamPlayerSetRenderCallBack
 * \see RpTeamPlayerGetElement
 * \see RpTeamPlayerGetHierarchyFrame
 * \see RpTeamPlayerGetActivity
 * \see RpTeamPlayerGetRenderCallBack
 * \see RpTeamPlayerGetBoundingSphere
 * \see RpTeamPlayerGetCurrentLOD
 * \see RpTeamPlayerGetSyncLODPerFrame
 */
const RpTeamPlayer *
RpTeamPlayerGetRenderCallBack( const RpTeamPlayer *player,
                               RpTeamPlayerRenderCallBack *preCallBack,
                               RpTeamPlayerRenderCallBack *postCallBack,
                               RpTeamRenderData *data )
{
    RWAPIFUNCTION(RWSTRING("RpTeamPlayerGetRenderCallBack"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != player);

    if( NULL != preCallBack )
    {
        *preCallBack = player->callBack.preRenderCallBack;
    }

    if( NULL != postCallBack )
    {
        *postCallBack = player->callBack.postRenderCallBack;
    }

    if( NULL != data )
    {
        *data = player->callBack.renderData;
    }

    RWRETURN(player);
}

/**
 * \ingroup rpteam
 * \ref RpTeamPlayerGetBoundingSphere is used to get the players bounding
 * sphere. This is used during \ref RpTeamRender to determine clipping
 * and shadow rendering information. If the bounding sphere intersects the
 * the near clip plane the whole player is culled.
 *
 * \param player Pointer to the player.
 *
 * \return Returns a pointer to the bounding sphere if successful or NULL if
 * there is an error.
 *
 * \see RpTeamGetNumberOfPlayers
 * \see RpTeamGetPlayer
 * \see RpTeamPlayerGetElement
 * \see RpTeamPlayerGetHierarchyFrame
 * \see RpTeamPlayerGetActivity
 * \see RpTeamPlayerGetRenderCallBack
 * \see RpTeamPlayerGetCurrentLOD
 * \see RpTeamPlayerGetSyncLODPerFrame
 */
RwSphere *
RpTeamPlayerGetBoundingSphere(RpTeamPlayer *player)
{
    RWAPIFUNCTION(RWSTRING("RpTeamPlayerGetBoundingSphere"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != player);

    RWRETURN(&(player->boundingSphere));
}

/**
 * \ingroup rpteam
 * \ref RpTeamPlayerSetShadows is used to define if the player's shadows
 * should be rendered. Hence it is possible to disable the rendering of
 * specific player's shadows. All players shadows are enabled by default
 * when the team is created.
 *
 * The team should be locked before any player's shadows are altered with
 * \ref RpTeamLockMode rpTEAMLOCKPLAYERSHADOWS.
 *
 * \param player Pointer to the player.
 * \param active Enable or disable the player's shadows.
 *
 * \return Returns a pointer to the player if successful or NULL if there is an
 * error.
 *
 * \see RpTeamLock
 * \see RpTeamUnlock
 * \see RpTeamPlayerGetShadows
 * \see RpTeamGetNumberOfPlayers
 * \see RpTeamGetPlayer
 */
RpTeamPlayer *
RpTeamPlayerSetShadows( RpTeamPlayer *player,
                        RwBool active )
{
    RWAPIFUNCTION(RWSTRING("RpTeamPlayerSetShadows"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != player);
    RWASSERT(player->team->lockFlags & rpTEAMLOCKPLAYERSHADOWS);

    if(active)
    {
        player->flags |= TEAMPLAYERSHADOW;
    }
    else
    {
        player->flags &= ~TEAMPLAYERSHADOW;
    }

    RWRETURN(player);
}

/**
 * \ingroup rpteam
 * \ref RpTeamPlayerGetShadows is used to return if the player's shadows
 * are set to rendered.
 *
 * The team should be locked before any player's shadows are altered with
 * \ref RpTeamLockMode rpTEAMLOCKPLAYERSHADOWS.
 *
 * \param player Pointer to the player to query.
 *
 * \return Returns TRUE if the player's shadows are active and FALSE if not.
 *
 * \see RpTeamPlayerSetShadows
 * \see RpTeamGetNumberOfPlayers
 * \see RpTeamGetPlayer
 */
RwBool
RpTeamPlayerGetShadows( RpTeamPlayer *player )
{
    RwBool shadows;

    RWAPIFUNCTION(RWSTRING("RpTeamPlayerGetShadows"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != player);
    RWASSERT(player->team->lockFlags & rpTEAMLOCKPLAYERSHADOWS);

    shadows = ((player->flags & TEAMPLAYERSHADOW) == TEAMPLAYERSHADOW);

    RWRETURN(shadows);
}
