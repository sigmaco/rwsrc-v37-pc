/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>
#include "rpdbgerr.h"

#include "../../plugin/skin2/skin.h"

#include "teamrenderd3d8.h"
#include "team.h"

#include "teamskind3d8.h"

/*
 *

  D3D8 RENDER

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
/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/
/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/
const RpTeam *
_rpTeamRender(const RpTeam *team)
{
    RwUInt32 i, j;

    RWFUNCTION(RWSTRING("_rpTeamRender"));
    RWASSERT(NULL != team);
    RWASSERT(NULL != team->dictionary->elements);
    RWASSERT(NULL != team->players);
    RWASSERT(rpNATEAMLOCKMODE == team->lockFlags);
    RWASSERT(team->opData.optimized);

    /* Update lights */
    _rpTeamD3D8UpdateLights(team);

    /* Render skinned element, through the optimizations. */
    RWASSERT(team->opData.skin.optimized);
    if(NULL != team->opData.skin.data)
    {
        const RpTeamOptSkin *skinOpData = &(team->opData.skin);

        for( i = 0; i < skinOpData->numElements; i++ )
        {
            RpTeamOptElement *opData = &(skinOpData->data[i]);

            RWASSERT(NULL != opData);
            RWASSERT(0 < opData->numClones);
            RWASSERT(NULL != opData->lodAtomic);
            RWASSERT(NULL != opData->skinRender);

            for( j = 0; j < opData->numClones; j++ )
            {
                RpTeamRenderSkin *renderData;
                RpTeamPlayer *player;

                /* Get the render data. */
                renderData = &(opData->skinRender[j]);
                RWASSERT(NULL != renderData);
                RWASSERT(NULL != renderData->player);
                RWASSERT(NULL != renderData->matrixCache);

                /* Grab the player. */
                player = renderData->player;

                /* Set the matrix cache. */
                _rpTeamSkinSetCurrentMatrixCache(renderData->matrixCache);

#if (defined(TEAMSHADOWALLINONE))
                if( player->flags & TEAMPLAYERRENDER )
#else /* (defined(TEAMSHADOWALLINONE)) */
                if( player->shadowData.playerClip != 0 )
#endif /* (defined(TEAMSHADOWALLINONE)) */
                {
                    RpAtomic *atomic;
                    RwUInt32 lod;

                    /* Get the lod. */
                    lod = player->lod.currentLOD;

                    /* Grab the lod atomic. */
                    atomic = (*(opData->lodAtomic))[lod];
                    TEAMCHECKLODATOMIC(atomic);

                    /* Set the atomics frame. */
                    _rpTeamAtomicSetFrame(atomic, player->frame);

                    /* Render Away! */
                    atomic = _rpTeamPlayerRenderAtomic(atomic, team, player);
                    RWASSERT(NULL != atomic);

                    _rpTeamAtomicSetFrame(atomic, (RwFrame *)NULL);
                }
            }
        }
    }

    /* Render static element. */
    RWASSERT(team->opData.stat.optimized);
    if(NULL != team->opData.stat.data)
    {
        const RpTeamOptStatic *staticOpData = &(team->opData.stat);

        for( i = 0; i < staticOpData->numElements; i++ )
        {
            RpTeamOptElement *opData = &(staticOpData->data[i]);

            RWASSERT(NULL != opData);
            RWASSERT(0 < opData->numClones);
            RWASSERT(NULL != opData->lodAtomic);
            RWASSERT(NULL != opData->staticRender);

            for( j = 0; j < opData->numClones; j++ )
            {
                RpTeamRenderStatic *renderData;
                RpTeamPlayer *player;

                /* Get the render data. */
                renderData = &(opData->staticRender[j]);
                RWASSERT(NULL != renderData);
                RWASSERT(NULL != renderData->matrices);
                RWASSERT(NULL != renderData->player);

                /* Grab the player. */
                player = renderData->player;

                /* Render the player? */
                if( player->shadowData.playerClip != 0 )
                {
                    RpAtomic *atomic;
                    RwFrame *frame;
                    RwMatrix *matrix;
                    RwUInt32 lod;

                    /* Get the lod. */
                    lod = player->lod.currentLOD;

                    /* Grab the lod atomic. */
                    atomic = (*(opData->lodAtomic))[lod];
                    TEAMCHECKLODATOMIC(atomic);

                    /* Grab the atomics frame. */
                    frame = RpAtomicGetFrame(atomic);
                    RWASSERT(NULL != frame);

                    /* Grab the frame matrix. */
                    matrix = TeamFrameGetLTM(frame);
                    RWASSERT(NULL != matrix);

                    /* Update it's matrix. */
                    TeamMatrixCopy(matrix, renderData->matrices);

                    /* Let the pipeline update the world bounding sphere. */
                    rwObjectSetPrivateFlags( atomic,
                                             rwObjectGetPrivateFlags(atomic) |
                                             rpATOMICPRIVATEWORLDBOUNDDIRTY );

                    /* Render Away! */
                    atomic = _rpTeamPlayerRenderAtomic(atomic, team, player);
                    RWASSERT(NULL != atomic);
                }
            }
        }
    }

    if (_rpTeamD3D8MaxBonesHardwareSupported > 0)
    {
        /* Render skinned shadow element, through the optimizations. */
        RWASSERT(team->opData.skin.optimized);
        if(NULL != team->opData.skin.data)
        {
            const RpTeamOptSkin *skinOpData = &(team->opData.skin);
            RpTeamElementLOD lod;

            RWASSERT(NULL != team->shadows);
            lod = team->shadows->shadowLOD;
            RWASSERT(rpTEAMELEMENTLODSUP > lod);

            for( i = 0; i < skinOpData->numElements; i++ )
            {
                RpTeamOptElement *opData = &(skinOpData->data[i]);
                RpAtomic *atomic;

                RWASSERT(NULL != opData);
                RWASSERT(0 < opData->numClones);
                RWASSERT(NULL != opData->lodAtomic);
                RWASSERT(NULL != opData->skinRender);

                /* Grab the shadow lod atomic. */
                atomic = (*(opData->lodAtomic))[lod];
                TEAMCHECKLODATOMIC(atomic);

                for( j = 0; j < opData->numClones; j++ )
                {
                    RpTeamRenderSkin *renderData;
                    RpTeamPlayer *player;

                    /* Get the render data. */
                    renderData = &(opData->skinRender[j]);
                    RWASSERT(NULL != renderData);
                    RWASSERT(NULL != renderData->player);
                    RWASSERT(NULL != renderData->matrixCache);

                    /* Grab the player. */
                    player = renderData->player;

                    if( (player->flags & TEAMPLAYERSHADOW) &&
                        (player->shadowData.renderNumShadows > 0) )
                    {
                        const RpHAnimHierarchy  *hierarchy;
                        const RpGeometry        *geometry;
                        const RpSkin            *skin;

                        hierarchy = RpHAnimFrameGetHierarchy(player->frame);

                        geometry = RpAtomicGetGeometry(atomic);

                        skin = *RPSKINGEOMETRYGETCONSTDATA(geometry);

                        _rpTeamD3D8UsingVertexShader = (hierarchy != NULL &&
                                                        skin != NULL &&
                                                        skin->boneData.numUsedBones <=
                                                        _rpTeamD3D8MaxBonesHardwareSupported);

                        if (_rpTeamD3D8UsingVertexShader)
                        {
                            _rpTeamD3D8UsingLocalSpace = ((RwUInt32)hierarchy->numNodes >
                                                          _rpTeamD3D8MaxBonesHardwareSupported);

                            /* Set the matrix cache. */
                            _rpTeamSkinSetCurrentMatrixCache(renderData->matrixCache);

                            /* Set the atomics frame. */
                            _rpTeamAtomicSetFrame(atomic, player->frame);

                            /* Render Away! */
                            atomic = _rpTeamShadowRenderAtomic(atomic, player);
                            RWASSERT(NULL != atomic);

                            _rpTeamAtomicSetFrame(atomic, (RwFrame *)NULL);
                        }
                    }
                }
            }
        }
    }

    /* Render static shadow element. */
    RWASSERT(team->opData.stat.optimized);
    if(NULL != team->opData.stat.data)
    {
        const RpTeamOptStatic *staticOpData = &(team->opData.stat);
        RpTeamElementLOD lod;

        RWASSERT(NULL != team->shadows);
        lod = team->shadows->shadowLOD;
        RWASSERT(rpTEAMELEMENTLODSUP > lod);

        for( i = 0; i < staticOpData->numElements; i++ )
        {
            RpTeamOptElement *opData = &(staticOpData->data[i]);
            RpAtomic *atomic;
            RwFrame *frame;
            RwMatrix *matrix;

            RWASSERT(NULL != opData);
            RWASSERT(0 < opData->numClones);
            RWASSERT(NULL != opData->lodAtomic);
            RWASSERT(NULL != opData->staticRender);

            /* Grab the lod atomic. */
            atomic = (*(opData->lodAtomic))[lod];
            TEAMCHECKLODATOMIC(atomic);

            /* Grab the atomics frame. */
            frame = RpAtomicGetFrame(atomic);
            RWASSERT(NULL != frame);

            /* Grab the frame matrix. */
            matrix = TeamFrameGetLTM(frame);
            RWASSERT(NULL != matrix);

            for( j = 0; j < opData->numClones; j++ )
            {
                RpTeamRenderStatic *renderData;
                RpTeamPlayer *player;

                /* Get the render data. */
                renderData = &(opData->staticRender[j]);
                RWASSERT(NULL != renderData);
                RWASSERT(NULL != renderData->matrices);
                RWASSERT(NULL != renderData->player);

                /* Grab the player. */
                player = renderData->player;

                /* Render the player? */
                if( (player->flags & TEAMPLAYERSHADOW) &&
                    (player->shadowData.renderNumShadows > 0) )
                {

                    /* Update it's matrix. */
                    TeamMatrixCopy(matrix, renderData->matrices);

                    /* Let the pipeline update the world bounding sphere. */
                    rwObjectSetPrivateFlags( atomic,
                                             rwObjectGetPrivateFlags(atomic) |
                                             rpATOMICPRIVATEWORLDBOUNDDIRTY );

                    /* Render Away! */
                    atomic = _rpTeamShadowRenderAtomic(atomic, player);
                    RWASSERT(NULL != atomic);
                }
            }
        }
    }

    /* Close any globally open pipelines. */
    _rpTeamPipeCloseGlobalPipe();

    RWRETURN(team);
}
