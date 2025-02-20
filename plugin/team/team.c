/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>
#include "rpdbgerr.h"

#include "rpteam.h"

#include "team.h"

/*
 *

  ..................   ..............       ......        .                 .
 ==================.  ==============.      ======..      =..               =.
 ==================.  ==============.     ========..     ==..             ==.
 ==================.  ==============.    ====/ ====..    ===..           ===.
 ====/  ====.  ====/  ====.     ====/   ====/   ====.    ====..         ====.
        ====.         ====.             ====.   ====.    =====..       =====.
        ====.         ====....          ====....====.    ======..     ======.
        ====.         =======.          ============.    =======..   =======.
        ====.         =======.          ============.    ========.. ========.
        ====.         =======/          ============.    ====.====.====/====.
        ====.         ====.             ====.   ====.    ====. =======/ ====.
        ====.         ====.      ....   ====.   ====.    ====.  =====/  ====.
        ====..        ====......====.   ====..  ====..   ====..  ===/   ====..
       ======.        ==============.  ======. ======.  ======.   =/   ======.
       ======.        ==============.  ======. ======.  ======.        ======.
       ======/        ==============/  ======/ ======/  ======/        ======/

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
#if(defined(TEAMMATRIXCACHECACHE))
typedef union RwMutableSkinCyclicCache RwMutableSkinCyclicCache;
union RwMutableSkinCyclicCache
{
    SkinCyclicCache       *Ptr;
    const SkinCyclicCache *constPtr;
};
#endif /*(defined(TEAMMATRIXCACHECACHE))*/

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/
#define ROUNDUP16(x) (((RwUInt32)(x) + 16 - 1) & ~(16 - 1))

#if (defined(RWDEBUG))
#define TEAMDRAWBOUNDINGSPHERESx
#endif /* (defined(RWDEBUG)) */

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/
#if (defined(TEAMDRAWBOUNDINGSPHERES))

#define SEGMENTS (32)

static void
DrawSphere(RwV3d *pos, RwReal radius, RwRGBA *color, RwFrame *frame)
{
    RwInt32 vertNum;
    RwReal angle, angInc;
    RwMatrix *ltm;
    RwIm3DVertex verts[SEGMENTS];
    RwV3d *vertex;
    RwImVertexIndex inds[SEGMENTS + 1];

    RWFUNCTION(RWSTRING("DrawSphere"));

    angInc = (2.0f * rwPI) / (RwReal)SEGMENTS;

    ltm = RwMatrixCreate();
    RwMatrixOrthoNormalize(ltm, RwFrameGetLTM(frame));

    for (vertNum = 0; vertNum < SEGMENTS; vertNum++)
    {
        RwIm3DVertexSetRGBA(&verts[vertNum],
            color->red, color->green, color->blue, color->alpha);
    }

    for (vertNum = 0; vertNum < (SEGMENTS + 1); vertNum++)
    {
        if (vertNum == SEGMENTS)
        {
            inds[vertNum] = 0;
        }
        else
        {
            inds[vertNum] = (RwUInt16)vertNum;
        }
    }

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void *)rwSHADEMODEFLAT);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void *)TRUE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void *)TRUE);

    /* Construct x-axis ... */
    angle = 0.0f;
    for (vertNum = 0; vertNum < SEGMENTS; vertNum++)
    {
        vertex = RwIm3DVertexGetPos(&verts[vertNum]);

        vertex->x = radius * (RwReal)cos(angle);
        vertex->y = 0.0f;
        vertex->z = radius * (RwReal)sin(angle);
        RwV3dAdd(vertex, vertex, pos);

        angle += angInc;
    }

    /* ... render it ... */
    if (RwIm3DTransform(verts, SEGMENTS, ltm, rwIM3D_VERTEXUV))
    {
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPEPOLYLINE, inds, SEGMENTS + 1);

        RwIm3DEnd();
    }

    /* Construct y-axis ... */
    angle = 0.0f;
    for (vertNum = 0; vertNum < SEGMENTS; vertNum++)
    {
        vertex = RwIm3DVertexGetPos(&verts[vertNum]);

        vertex->x = radius * (RwReal)cos(angle);
        vertex->y = radius * (RwReal)sin(angle);
        vertex->z = 0.0f;
        RwV3dAdd(vertex, vertex, pos);

        angle += angInc;
    }

    /* ... render it ... */
    if (RwIm3DTransform(verts, SEGMENTS, ltm, rwIM3D_VERTEXUV))
    {
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPEPOLYLINE, inds, SEGMENTS + 1);

        RwIm3DEnd();
    }

    /* Construct z-axis ... */
    angle = 0.0f;
    for (vertNum = 0; vertNum < SEGMENTS; vertNum++)
    {
        vertex = RwIm3DVertexGetPos(&verts[vertNum]);

        vertex->x = 0.0f;
        vertex->y = radius * (RwReal)cos(angle);
        vertex->z = radius * (RwReal)sin(angle);
        RwV3dAdd(vertex, vertex, pos);

        angle += angInc;
    }

    /* ... render it ... */
    if (RwIm3DTransform(verts, SEGMENTS, ltm, rwIM3D_VERTEXUV))
    {
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPEPOLYLINE, inds, SEGMENTS + 1);

        RwIm3DEnd();
    }

    RwMatrixDestroy(ltm);

    RWRETURNVOID();
}

void
_rpTeamDrawAtomicBounds(RpAtomic *atomic)
{
    static RwRGBA color = { 255, 0, 0, 255 };
    RwSphere sphere = *RpAtomicGetBoundingSphere(atomic);

    RWFUNCTION(RWSTRING("_rpTeamDrawAtomicBounds"));
    RWASSERT(NULL != atomic);

    DrawSphere( &sphere.center,
                sphere.radius,
                &color,
                RpAtomicGetFrame(atomic) );

    RWRETURNVOID();
}

void
_rpTeamDrawPlayerBounds(RpTeamPlayer *player)
{
    static RwRGBA color = { 0, 0, 255, 255 };
    static RwV3d pos = { 0.0f, 0.0f, 0.0f };

    RWFUNCTION(RWSTRING("_rpTeamDrawPlayerBounds"));
    RWASSERT(NULL != player);

    DrawSphere( &(player->boundingSphere.center),
                player->boundingSphere.radius,
                &color,
                player->frame );

    RWRETURNVOID();
}

#endif /* (defined(TEAMDRAWBOUNDINGSPHERES)) */

static const RpTeam *
_rpTeamRenderUpdateLOD(const RpTeam *team)
{
    RwUInt32 i;

    RWFUNCTION(RWSTRING("_rpTeamRenderUpdateLOD"));
    RWASSERT(NULL != team);
    RWASSERT(NULL != team->dictionary->elements);
    RWASSERT(NULL != team->players);
    RWASSERT(rpNATEAMLOCKMODE == team->lockFlags);
    RWASSERT(team->opData.optimized);

    /* Update player lod. */
    for( i = 0; i < team->data.numOfPlayers; i++ )
    {
        RpTeamPlayer *player = &(team->players[i]);
        RWASSERT(NULL != player);

        /* Should the player be rendered? */
        if( player->flags & TEAMPLAYERSELECTED )
        {
            /* Do we need to resync the players lod level. */
            if( player->lod.syncLODPerFrame )
            {
                /* Yes. Then do it! */
                if(NULL != team->lod.callBack)
                {
                    player->lod.currentLOD =
                        (team->lod.callBack)(team, i, team->lod.data);
                }
                else
                {
                    player->lod.currentLOD = rpTEAMELEMENTLODFIRST;
                }
            }
        }
    }

    RWRETURN(team);
}

static const RpTeam *
_rpTeamRenderUpdateVisibility(const RpTeam *team)
{
    RwUInt32 i;

    RWFUNCTION(RWSTRING("_rpTeamRenderUpdateVisibility"));
    RWASSERT(NULL != team);
    RWASSERT(NULL != team->dictionary->elements);
    RWASSERT(NULL != team->players);
    RWASSERT(rpNATEAMLOCKMODE == team->lockFlags);
    RWASSERT(team->opData.optimized);

    /* Update player shadow clip flags. */
    {
        RwCamera *camera;

        /* Grab the current camera. */
        camera = (RwCamera *)RWSRCGLOBAL(curCamera);
        RWASSERT(NULL != camera);

        for( i = 0; i < team->data.numOfPlayers; i++ )
        {
            RpTeamPlayer *player = &(team->players[i]);

            /* Lets assume the players isn't rendered. */
            player->flags &= ~TEAMPLAYERRENDER;

            /* Is the player going to be clipped? */
            if(player->flags & TEAMPLAYERSELECTED)
            {
                RwFrustumTestResult result;

                result = _rpTeamShadowCalcRenderData(player);

                if( rwSPHEREOUTSIDE != result )
                {
                    player->flags |= TEAMPLAYERRENDER;

                    _rpTeamDrawPlayerBounds(player);
                }
            }
        }
    }

    RWRETURN(team);
}

static const RpTeam *
_rpTeamRenderUpdateHierarchys(const RpTeam *team)
{
    RwUInt32 i;

    RWFUNCTION(RWSTRING("_rpTeamRenderUpdateHierarchys"));
    RWASSERT(NULL != team);
    RWASSERT(NULL != team->dictionary->elements);
    RWASSERT(NULL != team->players);
    RWASSERT(rpNATEAMLOCKMODE == team->lockFlags);
    RWASSERT(team->opData.optimized);

    /* Update hierarchy optimizations. */
    for( i = 0; i < team->data.numOfPlayers; i++ )
    {
        RpTeamPlayer *player = &(team->players[i]);

        /* Is the player going to be rendered? */
        if(player->flags & TEAMPLAYERRENDER)
        {
            RpHAnimHierarchy *hierarchy;
            RwMatrix *ret;

            RpSkin *firstSkin;
            RwUInt32 iPart;

            /* Get the frames hierarchy. */
            hierarchy = RpHAnimFrameGetHierarchy(player->frame);
            RWASSERT(NULL != hierarchy);

            RWASSERT(NULL != team->opData.skin.firstSkin);
            RWASSERT(NULL != player->frame);

            /* Assume the first skin is good enough, for the player
             * built out of static objects. */
            firstSkin = team->opData.skin.firstSkin;

            for (iPart = 0; iPart < team->data.numOfPlayerElements; iPart++)
            {
                RwUInt32 dictIndex;

                dictIndex = player->elements[iPart];

                if(rpTEAMELEMENTUNUSED != dictIndex)
                {
                    RpTeamDictionaryElement *element;

                    element = &(team->dictionary->elements[dictIndex]);

                    if(rpTEAMELEMENTSKINNED == (element->type & rpTEAMELEMENTSKINNED))
                    {
                        RpAtomic *atomic;

                        atomic = element->element[rpTEAMELEMENTLODFIRST];
                        RWASSERT(NULL != atomic);
                        firstSkin = RpSkinGeometryGetSkin(atomic->geometry);
                        RWASSERT(NULL != firstSkin);

                        /* Found a skin for this player. */
                        break;
                    }
                }
            }

#if(defined(TEAMMATRIXCACHECACHE))
            /* Need to get a new matrix cache for the player. */
            {
                RpHAnimHierarchy *hierarchy;
                RwUInt32 stepMatrices;
                RwMutableSkinCyclicCache cyclicCache;

                /* Grab the cyclic cache. */
                cyclicCache.constPtr =
                    (const SkinCyclicCache *)&(team->cyclicCache);
                RWASSERT(NULL != cyclicCache.Ptr);

                hierarchy = RpHAnimFrameGetHierarchy(player->frame);
                RWASSERT(NULL != hierarchy);

                stepMatrices = ( hierarchy->numNodes +
                                 cyclicCache.Ptr->nextMatrices);

                if(stepMatrices > cyclicCache.Ptr->maxMatrices)
                {
                    player->matrixCache = cyclicCache.Ptr->matrixCache;
                    cyclicCache.Ptr->nextMatrices = hierarchy->numNodes;
                }
                else
                {
                    player->matrixCache = ( cyclicCache.Ptr->matrixCache +
                                            cyclicCache.Ptr->nextMatrices );
                    cyclicCache.Ptr->nextMatrices = stepMatrices;
                }
            }

            RWASSERT(NULL != player->matrixCache);
            ret = _rpTeamSkinMatBlendUpdating( firstSkin,
                                               player->frame,
                                               hierarchy,
                                               player->matrixCache );
#else
            RWASSERT(NULL != player->skinCache.matrixCache);
            ret = _rpTeamSkinMatBlendUpdating( firstSkin,
                                               player->frame,
                                               hierarchy,
                                               player->skinCache.matrixCache );
#endif
            RWASSERT(NULL != ret);
        }
    }

    RWRETURN(team);
}

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/
RwFrustumTestResult
_rpTeamCameraCustomTestSphere( const RpTeam *team __RWUNUSED__,
                               RwUInt32 playerIndex __RWUNUSED__,
                               const RwCamera *camera,
                               const RwSphere *sphere,
                               RpTeamCameraTestSphereData data __RWUNUSED__ )
{
    const RwFrustumPlane *frustum;
    RwFrustumTestResult result;
    RwInt32 plane;

    RWFUNCTION(RWSTRING("_rpTeamCameraCustomTestSphere"));
    RWASSERT(NULL != camera);
    RWASSERT(NULL != sphere);

    /* Ok we return values slightly different:
     * (1) anything intersecting the near plane is considered outside.
     * (2) we don't test against the far clip plane.
     */

    /* Assume inside until we decide otherwise */
    result = rwSPHEREINSIDE;
    frustum = camera->frustumPlanes;

    /* 0: far
     * 1: near
     * 2: left
     * 3: top
     * 4: right
     * 5: bottom
     */
    for( plane = 0; plane < 6; plane++)
    {
        RwReal nDot;

        nDot = RwV3dDotProductMacro( &sphere->center,
                                     &(frustum[plane].plane.normal) );
        nDot -= frustum[plane].plane.distance;

        if (nDot > sphere->radius)
        {
            /* Outside this plane, so outside frustum */
            RWRETURN(rwSPHEREOUTSIDE);
        }
        else if (nDot > (-sphere->radius))
        {
            if (2 > plane)
            {
                RWRETURN(rwSPHEREOUTSIDE);
            }

            /* Crosses the boundary, so mark it as such */
            result = rwSPHEREBOUNDARY;
        }
    }

    /* Inside all the planes */
    RWRETURN(result);
}

/*===========================================================================*
 *--- Rendering Functions ---------------------------------------------------*
 *===========================================================================*/
RpAtomic *
_rpTeamShadowRenderAtomic( RpAtomic *atomic,
                           RpTeamPlayer * player __RWUNUSEDRELEASE__)
{
    RpAtomic *result;

    RWFUNCTION(RWSTRING("_rpTeamShadowRenderAtomic"));
    RWASSERT(NULL != atomic);
    RWASSERT(NULL != player);
    RWASSERT(player->flags & TEAMPLAYERSELECTED);
    RWASSERT(player->flags & TEAMPLAYERRENDER);

    /* Select the correct shadow render data. */
    _rpTeamShadowSetCurrentShadowData(&(player->shadowData));

    /* Select the corrent player data. */
    _rpTeamPlayerSetCurrentPlayer(player);

    _rpTeamRenderAtomic(atomic, result);
    RWASSERT(NULL != result);

    RWRETURN(atomic);
}

RpAtomic *
_rpTeamPlayerRenderAtomic( RpAtomic *atomic,
                           const RpTeam *team,
                           RpTeamPlayer *player )
{
    RpAtomic *userRet = (RpAtomic *)NULL;
    RwUInt32 playerIndex;
    RpAtomic *result;

    RWFUNCTION(RWSTRING("_rpTeamPlayerRenderAtomic"));
    RWASSERT(NULL != atomic);
    RWASSERT(NULL != team);
    RWASSERT(NULL != team->players);
    RWASSERT(NULL != player);
    RWASSERT(player->flags & TEAMPLAYERSELECTED);
    RWASSERT(player->flags & TEAMPLAYERRENDER);

    /* Calculate player index. */
    playerIndex = _rpTeamPlayerGetPlayerIndex(player);

    /* Call preRenderCallBack. */
    if(NULL != player->callBack.preRenderCallBack)
    {
        userRet = (player->callBack.preRenderCallBack)( team,
                      playerIndex, atomic, player->callBack.renderData );
    }

    /* Select the correct shadow render data. */
    _rpTeamShadowSetCurrentShadowData(&(player->shadowData));

    /* Select the corrent player data. */
    _rpTeamPlayerSetCurrentPlayer(player);

    /* Render the atomic. */
    _rpTeamRenderAtomic(atomic, result);
    RWASSERT(NULL != result);
    _rpTeamDrawAtomicBounds(atomic);

    /* Call postRenderCallBack. */
    if( (NULL != player->callBack.postRenderCallBack) &&
        (NULL != userRet) )
    {
        (player->callBack.postRenderCallBack)( team,
                                               playerIndex,
                                               atomic,
                                               player->callBack.renderData );
    }

    RWRETURN(atomic);
}

RpAtomic *
_rpTeamAtomicSetFrame( RpAtomic *atomic, RwFrame *frame )
{
    RwObject *object = (RwObject *)atomic;
    RwObjectHasFrame *ohf = (RwObjectHasFrame *)object;
    
    RWFUNCTION(RWSTRING("_rpTeamAtomicSetFrame"));
    RWASSERT(NULL != atomic);

	//@{ 20050513 DDonSS : Threadsafe
	// Frame Lock
	CS_FRAME_LOCK();
	//@} DDonSS

    /* Does the atomic has a frame? */
    if(rwObjectGetParent(ohf))
    {
        /* Yes. Remove the atomic from the frames object list. */
        rwLinkListRemoveLLLink(&ohf->lFrame);
    }

    /* Set the atomics frame pointer. */
    rwObjectSetParent(object, frame);

    /* Add it to the frames list of objects */
    if(NULL != frame)
    {
        rwLinkListAddLLLink(&frame->objectList, &ohf->lFrame);
    }

    /* World bounding sphere is no longer valid */
    rwObjectSetPrivateFlags(atomic, rwObjectGetPrivateFlags(atomic) |
                            rpATOMICPRIVATEWORLDBOUNDDIRTY);

	/* 2006. 3. 6. Nonstopdj */
	/* change unlock positon */
	//@{ 20050513 DDonSS : Threadsafe
	// Frame Unlock
	CS_FRAME_UNLOCK();
	//@} DDonSS

    RWRETURN(atomic);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rpteam
 * \ref RpTeamCreate is called to create an \ref RpTeam object. The team
 * object contains players and a player element dictionary. The players are then
 * constructed from the elements in the player dictionary. The team is created
 * locked with \ref RpTeamLockMode rpTEAMLOCKALL and should be unlocked before
 * rendering.
 *
 * Once a team has been created a \ref RpTeamDictionary should be attached to
 * the team.
 *
 * \param numOfPlayers        Number of players in the team.
 * \param numOfPlayerElements Maximum number of elements a player can be
 *                            constructed from.
 *
 * \return Returns a pointer to the team if successful or NULL if there is an
 * error.
 *
 * \see RpTeamDestroy
 * \see RpTeamDictionaryCreate
 * \see RpTeamGetNumberOfPlayers
 * \see RpTeamGetNumberOfElements
 * \see RpTeamSetDictionary
 * \see RpTeamGetDictionary
 * \see RpTeamGetPlayer
 * \see RpTeamRender
 * \see RpTeamLock
 * \see RpTeamUnlock
 */
RpTeam *
RpTeamCreate( RwUInt32 numOfPlayers,
              RwUInt32 numOfPlayerElements )
{
    RpTeam *team = (RpTeam *)NULL;

    RwUInt32 sizeTeam    = 0;
    RwUInt32 sizePlayer  = 0;
    RwUInt32 sizeCache   = 0;
    RwUInt32 numMatrices = 0;

    RWAPIFUNCTION(RWSTRING("RpTeamCreate"));
    RWASSERT(0 < TeamModule.numInstances);

    /* Check our struct sizes. */

#if 0 /* This is out of date. */
#if(defined(TEAMMATRIXCACHECACHE))
    RWASSERT(124 == sizeof(RpTeam));
#else
    RWASSERT(108 == sizeof(RpTeam));
#endif
    RWASSERT(12  == sizeof(RpTeamPlayerData));
    RWASSERT(36  == sizeof(RpTeamOptElements));
    RWASSERT(12  == sizeof(RpTeamOptStatic));
    RWASSERT(16  == sizeof(RpTeamOptSkin));
    RWASSERT(4   == sizeof(RpTeamOptPatch));
    RWASSERT(20  == sizeof(RpTeamLOD));
    RWASSERT(8   == sizeof(RpTeamDictionary));
    RWASSERT(40  == sizeof(RpTeamDictionaryElement));
    RWASSERT(288 == sizeof(RpTeamPlayer));
    RWASSERT(224 == sizeof(RpTeamShadowRenderData));
    RWASSERT(8   == sizeof(RpTeamPlayerLOD));
    RWASSERT(12  == sizeof(SkinCache));
    RWASSERT(12  == sizeof(PlayerCallBackData));
    RWASSERT(16  == sizeof(RwSphere));
    /* Platform. */
    RWASSERT(16  == sizeof(TeamSkyUnion));
#endif /*0*/

#if(defined(TEAMMATRIXCACHECACHE))
    /* Calculate the maximum number of bones. */
    numMatrices = rpTEAMMAXNUMBONES *      /* Max bones. */
                  numOfPlayers *           /* NumOfPlayers. */
                  (0 + 2);       /* We double buffer. */
#endif

    /* We first calculate how much memory is needed. */
    sizeTeam =  sizeof(RpTeam) +
                /*--- Team ---*/
                (sizeof(RpTeamPlayer) * numOfPlayers);

                 /*--- TeamPlayer ---*/
    sizePlayer = (sizeof(RwUInt32) * numOfPlayerElements) *
                  numOfPlayers;

    sizeCache = (sizeof(RwMatrix) * numMatrices);

    /* Malloc some space. */
    /* Determine size with slack for 16 byte alignment. */
    team = (RpTeam *)RwMalloc(sizeTeam+sizePlayer+sizeCache+15,
                              rwID_TEAM | rwMEMHINTDUR_EVENT);

    /* Did we get some? */
    if(NULL != team)
    {
        void *memory;
        RwUInt32 i;

        /* Blank the memory. */
        memset(team, 0, sizeTeam);

        /* Pointer fixup ... */
        memory = (void *)(team + 1);

        /* ... the team players then ... */
        team->players = (RpTeamPlayer *)ROUNDUP16(memory);
        memory = (void *)(team->players + numOfPlayers);

        /* Set the rpTEAMELEMENTUNUSEDs */
        memset( memory, ((RwUInt8)~0), sizePlayer );

        /* ... each players ... */
        for( i = 0; i < numOfPlayers; i++ )
        {
            /* ... elements. */
            team->players[i].elements = (RwUInt32 *)memory;
            memory = (void *)(team->players[i].elements +
                              numOfPlayerElements);
        }

#if(defined(TEAMMATRIXCACHECACHE))
        team->cyclicCache.unaligned = memory;
        team->cyclicCache.matrixCache = (RwMatrix *)ROUNDUP16(team->cyclicCache.unaligned);

        team->cyclicCache.maxMatrices = numMatrices;
        team->cyclicCache.nextMatrices = 0;
#endif

        /* Setup the requested data. */

        /* Lock the entire team. */
        team = RpTeamLock(team, (RwUInt32)rpTEAMLOCKALL);
        RWASSERT(NULL != team);

        /* Team dictionary is NULL, until a dictionary is created. */
        team = RpTeamSetDictionary(team, (RpTeamDictionary *)NULL);
        RWASSERT(NULL != team);

        /* Cache data. */
        team->data.numOfPlayers        = numOfPlayers;
        team->data.numOfPlayerElements = numOfPlayerElements;

        /* Select the default callback. */
        team->lod.callBack = _rpTeamPlayerLODDefaultSelectLODCallBack;
        team->frustumTest.callBack = _rpTeamCameraCustomTestSphere;

        /* Setup the players. */
        for( i = 0; i < numOfPlayers; i++ )
        {
            team->players[i].team = team;
        }

        /* Extra team in the list. */
        _rpTeamAddTeamToList(team);
    }
    else
    {
        RWERROR((E_RW_NOMEM, sizeTeam+sizePlayer));
    }

    /* Return the team. */
    RWRETURN(team);
}

/**
 * \ingroup rpteam
 * \ref RpTeamDestroy is called to destroy a team object. All the players
 * will be destroyed, and the player's hierarchy frame will be destroyed.
 * The team's dictionary must be destroyed manually as it may be in use
 * by a number of teams.
 *
 * \param team Pointer to the team to destroy.
 *
 * \return Returns TRUE if successful or FALSE if there is an error.
 *
 * \see RpTeamCreate
 * \see RpTeamLock
 * \see RpTeamUnlock
 */
RwBool
RpTeamDestroy( RpTeam *team )
{
    RwUInt32 i;

    RWAPIFUNCTION(RWSTRING("RpTeamDestroy"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != team);
    RWASSERT(NULL != team->players);

    /* Start by locking the team. */
    team = RpTeamLock(team, rpTEAMLOCKALL);

    /* Now lets destroy any optimized data. */
    _rpTeamOptimizeDestroy(team, rpTEAMELEMENTALL, rpTEAMLOCKALL);

    /* Lets delete the players first. */
    for( i = 0; i < team->data.numOfPlayers; i++ )
    {
        _rpTeamPlayerDestroy(&(team->players[i]));
    }

    /* Clean up and destroy the dictionary. */
    team = RpTeamSetDictionary(team, (RpTeamDictionary *)NULL);
    RWASSERT(NULL != team);

#if(defined(TEAMMATRIXCACHECACHE))
    RWASSERT(NULL != team->cyclicCache.unaligned);
    team->cyclicCache.unaligned = NULL;
    team->cyclicCache.matrixCache = (RwMatrix *)NULL;
    team->cyclicCache.maxMatrices = 0;
    team->cyclicCache.nextMatrices = 0;
#endif

    /* Don't need this team anymore. */
    _rpTeamRemoveTeamFromList(team);

    /* Free the memory. */
    RwFree(team);

    RWRETURN(TRUE);
}

/**
 * \ingroup rpteam
 * \ref RpTeamGetNumberOfPlayers returns the number of players in a team.
 *
 * \param team Pointer to the team to query.
 *
 * \return Number of players the team was created with.
 *
 * \see RpTeamCreate
 * \see RpTeamDestroy
 * \see RpTeamGetNumberOfElements
 * \see RpTeamGetDictionary
 * \see RpTeamGetPlayer
 */
RwUInt32
RpTeamGetNumberOfPlayers( const RpTeam *team )
{
    RWAPIFUNCTION(RWSTRING("RpTeamGetNumberOfPlayers"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != team);

    RWRETURN(team->data.numOfPlayers);
}

/**
 * \ingroup rpteam
 * \ref RpTeamGetDictionary returns a pointer to the team's element
 * dictionary.
 *
 * \param team Pointer to the team.
 *
 * \return Returns a pointer to the team's dictionary if successful or NULL if
 * there is an error.
 *
 * \see RpTeamCreate
 * \see RpTeamDestroy
 * \see RpTeamSetDictionary
 * \see RpTeamGetNumberOfPlayers
 * \see RpTeamGetNumberOfElements
 * \see RpTeamGetPlayer
 * \see RpTeamRender
 * \see RpTeamLock
 * \see RpTeamUnlock
 */
RpTeamDictionary *
RpTeamGetDictionary( const RpTeam *team )
{
    RWAPIFUNCTION(RWSTRING("RpTeamGetDictionary"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != team);

    RWRETURN(team->dictionary);
}

/**
 * \ingroup rpteam
 * \ref RpTeamSetDictionary attaches a \ref RpTeamDictionary to the
 * the specified team. Currently a team dictionary may not be attached
 * to multiple team objects.
 *
 * The team should be locked before any elements are added to it's dictionary
 * with \ref RpTeamLockMode rpTEAMLOCKDICTIONARY.
 *
 * \param team       Pointer to the team.
 * \param dictionary Pointer to the dictionary to attach.
 *
 * \return Returns a pointer to the team if successful or NULL if
 * there is an error.
 *
 * \see RpTeamCreate
 * \see RpTeamDestroy
 * \see RpTeamGetDictionary
 * \see RpTeamGetNumberOfPlayers
 * \see RpTeamGetNumberOfElements
 * \see RpTeamGetPlayer
 * \see RpTeamRender
 * \see RpTeamLock
 * \see RpTeamUnlock
 */
RpTeam *
RpTeamSetDictionary(RpTeam * team, RpTeamDictionary * dictionary)
{
    void *memory;
    RwUInt32 size;

    RWAPIFUNCTION(RWSTRING("RpTeamSetDictionary"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != team);
    RWASSERT(team->lockFlags & rpTEAMLOCKDICTIONARY);

    /* Destroy any current selected array. */
    if(NULL != team->selected)
    {
        RwFree(team->selected);
    }
    team->selected = (RwUInt32 *)NULL;

    if(NULL != dictionary)
    {
        RwUInt32 numDictElements;
        RwUInt32 numPlayers;
        RwUInt32 numElements;

        numDictElements = RpTeamDictionaryGetNumOfElements(dictionary);
        numPlayers = RpTeamGetNumberOfPlayers(team);
        numElements = RpTeamGetNumberOfElements(team);
        /* Each team needs a selection array for the each element. */
        size = numDictElements * sizeof(RwUInt32);

        /* Each team needs it's optimisation data per element. */
        size += numDictElements * sizeof(RpTeamOptElement);
        size += numDictElements * sizeof(RpTeamOptElement);

        /* Each team needs render data for all the instances of an element. */
        size += (numPlayers * numElements) * sizeof(RpTeamRenderStatic);
        size += (numPlayers * numElements) * sizeof(RpTeamRenderSkin);
        size += 16; /* Round up for QW alignment */

        memory = RwMalloc(size, rwID_TEAMPLUGIN | rwMEMHINTDUR_EVENT);

        if(NULL == memory)
        {
            RWERROR((E_RW_NOMEM, size));
            RWRETURN((RpTeam *)NULL);
        }

        memset(memory, 0, size);

        /* Initalise the selection array to record the number of times
         * an element is referenced by a player. */
        team->selected = (RwUInt32 *)memory;

        /* Initalise the static and skinning rendering arrays.
         * These contain the data necessary to render the team parts. */
        team->opData.stat.data =
            (RpTeamOptElement *)ROUNDUP16(team->selected + numDictElements);

        {
            RpTeamRenderStatic *staticTemp;

            staticTemp = (RpTeamRenderStatic *)(team->opData.stat.data +
                                                numDictElements);
            staticTemp += (numPlayers * numElements);

            team->opData.skin.data = (RpTeamOptElement *)(staticTemp);
        }
    }

    team->dictionary = dictionary;

    RWRETURN(team);
}

/**
 * \ingroup rpteam
 * \ref RpTeamGetPlayer returns a pointer to the indexed player in the team.
 *
 * \param team  Pointer to the team.
 * \param index Player index in the team.
 *
 * \return Returns a pointer to the player if successful or NULL if there is an
 * error.
 *
 * \see RpTeamCreate
 * \see RpTeamDestroy
 * \see RpTeamGetNumberOfPlayers
 * \see RpTeamGetNumberOfElements
 * \see RpTeamGetDictionary
 * \see RpTeamRender
 * \see RpTeamLock
 * \see RpTeamUnlock
 */
RpTeamPlayer *
RpTeamGetPlayer( const RpTeam *team,
                 RwUInt32 index )
{
    RWAPIFUNCTION(RWSTRING("RpTeamGetPlayer"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != team);
    RWASSERT(team->data.numOfPlayers > index);
    RWASSERT(NULL != team->players);

    RWRETURN(&(team->players[index]));
}

/**
 * \ingroup rpteam
 * \ref RpTeamRender is called to render the team.
 *
 * Only the players who have been selected will be considered for rendering.
 * The team will be rendered with the correct team and player textures defined
 * by \ref RpTeamMaterialSetPlayerTexture and
 * \ref RpTeamMaterialSetTeamTexture. The player's level of detail will
 * determine which atomic element is used when rendering a player's dictionary
 * element.
 *
 * This function should only be called from within the context of a
 * \ref RwCameraBeginUpdate...\ref RwCameraEndUpdate block. This ensures that
 * any rendering that takes place is directed towards an image raster
 * connected to a camera. It also ensures that the player's local
 * transformation matrices are recalculated, if necessary, so that the
 * dictionary elements are rendered in the correct position and orientation.
 *
 * The team must be unlocked before rendering.
 *
 * The \ref RpTeamRenderMode flags allows the application to control the
 * team rendering function:
 *
 * \li \ref rpTEAMRENDERUPDATELODS
 *          enables player LOD calculation.
 * \li \ref rpTEAMRENDERUPDATEVISIBILITY
 *          calculates player and shadow visibility.
 * \li \ref rpTEAMRENDERUPDATEHIERARCHYS
 *          updates the player skinning matrices.
 * \li \ref rpTEAMRENDERTEAM
 *          renders the player elements.
 * \li \ref rpTEAMRENDERALL
 *          enables all the above features.
 *
 * \param team  Pointer to the team.
 * \param flags \ref RpTeamRenderMode flags to define rendering execution.
 *
 * \return Returns a pointer to the team if successful or NULL if there is an
 * error.
 *
 * \see RpTeamCreate
 * \see RpTeamDestroy
 * \see RpTeamLock
 * \see RpTeamUnlock
 * \see \ref RpTeamRenderMode
 */
const RpTeam *
RpTeamRender(const RpTeam *team, RwUInt32 flags)
{
    RWAPIFUNCTION(RWSTRING("RpTeamRender"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != team);
    RWASSERT(0 == (flags & ~rpTEAMRENDERALL));

    if(0 != (flags & rpTEAMRENDERUPDATELODS))
    {
        team = _rpTeamRenderUpdateLOD(team);
        RWASSERT(NULL != team);
    }

    if(0 != (flags & rpTEAMRENDERUPDATEVISIBILITY))
    {
        team = _rpTeamRenderUpdateVisibility(team);
        RWASSERT(NULL != team);
    }

    if(0 != (flags & rpTEAMRENDERUPDATEHIERARCHYS))
    {
        team = _rpTeamRenderUpdateHierarchys(team);
        RWASSERT(NULL != team);
    }

    if(0 != (flags & rpTEAMRENDERTEAM))
    {
        team = _rpTeamRender(team);
        RWASSERT(NULL != team);
    }

    RWRETURN(team);
}

/**
 * \ingroup rpteam
 * \ref RpTeamLock should be called before any of the team's internal data
 * structures are changed. This function prepares the team for unlocking
 * after its data has been modified so that the rendering optimizations can
 * be rebuilt. A newly created geometry is already locked.
 *
 * The lock flags are listed in \ref RpTeamLockMode.
 *
 * \param team     Pointer to the team to lock.
 * \param lockMode Team lock flags.
 *
 * \return Returns a pointer to the team if successful or NULL if there is an
 * error.
 *
 * \see RpTeamCreate
 * \see RpTeamDestroy
 * \see RpTeamRender
 * \see RpTeamUnlock
 */
RpTeam *
RpTeamLock(RpTeam *team, RwUInt32 lockMode)
{
    RWAPIFUNCTION(RWSTRING("RpTeamLock"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != team);
    RWASSERT(!(lockMode & ~(rpTEAMLOCKALL)));

    /* Set lock flag. */
    team->lockFlags |= lockMode;

    RWRETURN(team);
}

/**
 * \ingroup rpteam
 * \ref RpTeamUnlock is used to unlock the specified team. This function is
 * typically used to release the team's internal data structures after
 * modification so that the team's rendering data can be optimized.
 *
 * A newly created team is always locked and should be unlocked before it
 * can be used in any rendering.
 *
 * \param team Pointer to the team to unlock.
 *
 * \return Returns a pointer to the team if successful or NULL if there is an
 * error.
 *
 * \see RpTeamCreate
 * \see RpTeamDestroy
 * \see RpTeamRender
 * \see RpTeamLock
 */
RpTeam *
RpTeamUnlock(RpTeam *team)
{
    RpTeam *ret = (RpTeam *)NULL;

    RWAPIFUNCTION(RWSTRING("RpTeamUnlock"));
    RWASSERT(0 <TeamModule.numInstances);
    RWASSERT(NULL != team);

    if( (team->lockFlags & rpTEAMLOCKPLAYERS) ||
        (team->lockFlags & rpTEAMLOCKPLAYERLODS) )
    {
        ret = _rpTeamOptimizeCreate( team,
                                     rpTEAMELEMENTALL,
                                     rpNATEAMLOCKMODE );
        RWASSERT(ret == team);
    }

    if( team->lockFlags & rpTEAMLOCKTEXDICTIONARYS )
    {
        ret = _rpTeamOptimizeCreate( team,
                                     rpNATEAMELEMENTTYPE,
                                     rpTEAMLOCKTEXDICTIONARYS);
        RWASSERT(ret == team);
    }

    team->lockFlags = rpNATEAMLOCKMODE;

    RWRETURN(ret);
}

/**
 * \ingroup rpteam
 * \ref RpTeamGetNumberOfElements returns the number of elements each player
 * can be constucted from.
 *
 * \param team Pointer to the team to query.
 *
 * \return Returns the maximum number of elements a player can be
 * constructed from.
 *
 * \see RpTeamCreate
 * \see RpTeamDestroy
 * \see RpTeamGetNumberOfPlayers
 * \see RpTeamGetDictionary
 * \see RpTeamGetPlayer
 * \see RpTeamRender
 * \see RpTeamLock
 * \see RpTeamUnlock
 */
RwUInt32
RpTeamGetNumberOfElements( const RpTeam *team )
{
    RWAPIFUNCTION(RWSTRING("RpTeamGetNumberOfElements"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != team);

    RWRETURN(team->data.numOfPlayerElements);
}

/**
 * \ingroup rpteam
 * \ref RpTeamSetCameraTestSphereCallBack is called to overload
 * the default team player frustum test function. Each frame, any player
 * that is active ( \ref RpTeamPlayerSetActivity ), is considered for
 * rendering. The player's bounding sphere
 * ( \ref RpTeamPlayerGetBoundingSphere ) is tested against the
 * active camera's frustum.
 *
 * The user may want to overload this test to allow a close up as the
 * default frustum test culls players that intersect the near clip plane.
 * This stops the camera from accidently being inside the player.
 *
 * \param team     Pointer to the team.
 * \param callBack Frustum call back function.
 * \param data     User data to pass to the call back function.
 *
 * \return Returns a pointer to the team if successful or NULL if there is an
 * error.
 *
 * \see RpTeamGetCameraTestSphereCallBack
 */
RpTeam *
RpTeamSetCameraTestSphereCallBack( RpTeam *team,
                                   RpTeamCameraTestSphere callBack,
                                   RpTeamCameraTestSphereData data )
{
    RWAPIFUNCTION(RWSTRING("RpTeamSetCameraTestSphereCallBack"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != team);

    team->frustumTest.callBack = callBack;
    team->frustumTest.data = data;

    if(NULL == team->frustumTest.callBack)
    {
        team->frustumTest.callBack = _rpTeamCameraCustomTestSphere;
    }

    RWRETURN(team);
}

/**
 * \ingroup rpteam
 * \ref RpTeamGetCameraTestSphereCallBack is called to query the current
 * team player frustum test call back function.
 *
 * \param team     Pointer to the team.
 * \param callBack Pointer to receive the current call back.
 * \param data     Pointer to receive the current call back user data.
 *
 * \return Returns a pointer to the team if successful or NULL if there is an
 * error.
 *
 * \see RpTeamSetCameraTestSphereCallBack
 */
const RpTeam *
RpTeamGetCameraTestSphereCallBack( const RpTeam *team,
                                   RpTeamCameraTestSphere *callBack,
                                   RpTeamCameraTestSphereData *data )
{
    RWAPIFUNCTION(RWSTRING("RpTeamGetCameraTestSphereCallBack"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != team);

    if(NULL != callBack)
    {
        *callBack = team->frustumTest.callBack;
    }

    if(NULL != data)
    {
        *data = team->frustumTest.data;
    }

    RWRETURN(team);
}

/**
 * \ingroup rpteam
 * \ref RpTeamSetPersistentLights is called to define the
 * ambient and directional light the team should use to light the
 * players. RpTeam only supports a custom light mode of a single
 * directional and a single ambient light.
 *
 * These two lights are used for the entire team and the rendering
 * pipelines have been optimized to reflect this.  It is imperative
 * that you call this function when using RpTeam.
 *
 * Any lights added to a world will be ignored whilst rendering the
 * team.
 *
 * The team should be locked with \ref rpTEAMLOCKLIGHTS before
 * changing the lights.
 *
 * \param team        Pointer to the team to define the lights.
 * \param directional Pointer to the directional light.
 * \param ambient     Pointer to the ambient light.
 *
 * \return Returns a pointer to the team if successful or NULL if there is an
 * error.
 *
 * \see RpTeamRender
 * \see RpTeamLock
 * \see RpTeamGetPersistentLights
 */
RpTeam *
RpTeamSetPersistentLights( RpTeam *team,
                           RpLight *directional,
                           RpLight *ambient )
{
    RWAPIFUNCTION(RWSTRING("RpTeamSetPersistentLights"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != team);
    RWASSERT(team->lockFlags & rpTEAMLOCKLIGHTS);

    team->lights[rpTEAMDIRECTIONAL1LIGHT] = directional;
    team->lights[rpTEAMAMBIENTLIGHT] = ambient;
    team->lightType = rpTEAMAMBIENTDIRECTIONLIGHTING;

    RWRETURN(team);
}

/**
 * \ingroup rpteam
 * \ref RpTeamGetPersistentLights returns the persistent lights
 * currently defined for the team.
 *
 * \param team        Pointer to the team to define the lights.
 * \param directional Pointer to the receive the directional light.
 * \param ambient     Pointer to the receive the ambient light.
 *
 * \return Returns a pointer to the team if successful or NULL if there is an
 * error.
 *
 * \see RpTeamSetPersistentLights
 */
const RpTeam *
RpTeamGetPersistentLights( const RpTeam *team,
                           RpLight **directional,
                           RpLight **ambient )
{
    RWAPIFUNCTION(RWSTRING("RpTeamGetPersistentLights"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != team);

    if(NULL != directional)
    {
        *directional = team->lights[rpTEAMDIRECTIONAL1LIGHT];
    }

    if(NULL != ambient)
    {
        *ambient = team->lights[rpTEAMAMBIENTLIGHT];
    }

    RWRETURN(team);
}
