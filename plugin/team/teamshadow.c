/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>
#include "rpdbgerr.h"

#include "rpteam.h"

#include "teamshadow.h"
#include "team.h"

/*
 *

  CUSTOM FAST SHADOWS

 *
 */

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/
#if (defined(TEAMGLOBALPOINTS))
/* const */
RpTeamShadowRenderData * _rpTeamShadowRenderData =
    (RpTeamShadowRenderData *)NULL;
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

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

/*---------------------- ATTACH PIPELINE -------------------------------*/
RpAtomic *
_rpTeamShadowAttachPipeline( RpAtomic *atomic,
                             RpTeamElementType type,
                             RwBool allInOne )
{
    RxPipeline *pipeline;

    RWFUNCTION(RWSTRING("_rpTeamShadowAttachPipeline"));
    RWASSERT(NULL != atomic);

    pipeline = _rpTeamShadowGetPipeline(type, allInOne);
    RWASSERT(NULL != pipeline);

    /* Need to attach the custom pipeline. */
    atomic = RpAtomicSetPipeline(atomic, pipeline);
    RWASSERT(NULL != atomic);

    RWRETURN(atomic);
}
/*----------------------------------------------------------------------*/

/*--------------------- SET CLIP FLAGS ---------------------------------*/
RwFrustumTestResult
_rpTeamShadowCalcRenderData(RpTeamPlayer *player)
{
    /* RwFrustumTestResults:
     * rwSPHEREOUTSIDE  = 0,
     * rwSPHEREBOUNDARY = 1,
     * rwSPHEREINSIDE   = 2,
     */

    /* Player discarded                = 0
     * Player visible and not clipped  = 1
     * Player visible and clipped      = 3
     */
    static RwUInt32 playerClip[3] = { 0, 3, 1 };

    /* Shadow 1 2 3 4 not clipped  = 0
     * Shadow 1 2 3 4 clipped      = 1 + 2 + 4 + 8
     */
    static RwUInt32 shadowClip[3] = { 1, 1, 0 };

    RpTeamShadowRenderData *renderData;
    RwCamera *camera;
    RwSphere boundingSphere;
    RwFrustumTestResult result;
    RwMatrix *playerLTM;
    RwMatrix orthoLTM;

    RpTeam *team;

    RwUInt32 ret;

    RWFUNCTION(RWSTRING("_rpTeamShadowCalcRenderData"));
    RWASSERT(NULL != player);
    RWASSERT(NULL != player->frame);

    /* Get the shadow render data. */
    renderData = &(player->shadowData);
    RWASSERT(NULL != renderData);

    /* Get the team. */
    team = player->team;
    RWASSERT(NULL != team);

    /* Setup the team shadow data. */
    renderData->shadows = team->shadows;

    /* Also grab the camera, need it for the view frustrum. */
    camera = (RwCamera *)RWSRCGLOBAL(curCamera);
    RWASSERT(NULL != camera);

    /* Get object to world space transform. */
    playerLTM = TeamFrameGetLTM(player->frame);
    if (rwMatrixTestFlags(playerLTM, rwMATRIXTYPEORTHONORMAL) ==
                                                    rwMATRIXTYPEORTHONORMAL)
    {
        RwMatrixCopy(&orthoLTM, playerLTM);
    }
    else
    {
        RwMatrixOrthoNormalize(&orthoLTM, playerLTM);
    }

    /* Get the bounding sphere. */
    boundingSphere.radius = player->boundingSphere.radius;

    RwV3dTransformPoint( &(boundingSphere.center),
                         &(player->boundingSphere.center),
                         &orthoLTM );
    /*  TeamV3dTransformPoints( &(boundingSphere.center),
     *                          &(player->boundingSphere.center),
     *                          &orthoLTM );
     */

    /* First we test the whole player. */

    /* This frustrum test is user overloadable. */
    RWASSERT(NULL != team->frustumTest.callBack);
    result = (team->frustumTest.callBack)
                 ( team,
                   _rpTeamPlayerGetPlayerIndex(player),
                   camera,
                   &boundingSphere,
                   team->frustumTest.data );
    RWASSERT( (result == rwSPHEREOUTSIDE) ||
              (result == rwSPHEREBOUNDARY) ||
              (result == rwSPHEREINSIDE) );

    /* Setup the player clip flags. */
    renderData->playerClip = playerClip[result];

    /* Setup the shadow clip flags. */

    /* Is the player shadowed? */
    if( ((player->flags & TEAMPLAYERSHADOW) == TEAMPLAYERSHADOW) &&
        (NULL != renderData->shadows) )
    {
        RwMatrix *shadowPlaneLTM;

        RwMatrix playerInverse;

        RwV3d objPlaneNormal   = { (RwReal)0.0f, (RwReal)0.0f, (RwReal)0.0f };
        RwV3d objPlanePosition = { (RwReal)0.0f, (RwReal)0.0f, (RwReal)0.0f };
        RwV3d objProjection;

        RwV3d *worldPlaneNormal;
        RwV3d *worldPlanePosition;
        RwV3d worldProjection;

        RwV3d *shadowCenter;

        RwUInt32 i, j;

        /* Data we need:
         *  + Per player / atomic:
         *    o Number of shadows to render.
         *    o Shadow plane normal.
         *    o Shadow plane position.
         *    o Shadow and player clip flags.
         *  + Per shadow:
         *    o Projection vector of shadow.
         *    o Projection scale.
         *    o Shadow values.
         */

        /* Ok first we build the clip flags. */
        /* We start by projecting the bounding sphere onto the plane. */
        /* And frustum testing the resultant ellipse. */

        /* Get shadow plane world space matrix. */
        shadowPlaneLTM = TeamFrameGetLTM(renderData->shadows->shadowPlane);

        /* Grab the frames normal. */
        worldPlaneNormal = &(shadowPlaneLTM->up);

        /* Grab the position of the plane. */
        worldPlanePosition = &(shadowPlaneLTM->pos);

        /* Calculate world to object space transform. */
        TeamMatrixInvert(&playerInverse, playerLTM);

        /* Setup the player center. */
        shadowCenter = &(boundingSphere.center);

        /*-------------------------------------------------*/

        /* We do the following transforms for to prepare for rendering. */

        /* Transform shadow plane normal to object space. */
        TeamV3dTransformVectors( &objPlaneNormal,
                                 worldPlaneNormal,
                                 &playerInverse );

        /* Normalise it, (should hopefully still be normal). */
        TeamV3dNormalize(&objPlaneNormal, &objPlaneNormal);

        /* Transform position of plane to object space. */
        TeamV3dTransformPoints( &objPlanePosition,
                                worldPlanePosition,
                                &playerInverse );

        /*-------------------------------------------------*/

        /* Reset the shadow clip flag. */
        renderData->shadowClip = 0;

        /* Test each shadow to set it's clipping flag. */
        for ( i = 0, j = 0; j < renderData->shadows->number; j++ )
        {
            RwMatrix *shadow;

            /* Projection variables. */
            RwV3d projection;
            RwV3d projectionScaled;
            RwReal projScale;
            RwV3d projShadowCenter;
            RwReal projLengthSqu;
            RwReal projRadius;
            RwReal projShadowSqu;

            RwV3d worldProjectionRender;

            RwV3d difference;
            RwReal shadowScale;
            RwV3d projScaleVector;

            /* Frustum testing. */
            const RwFrustumPlane *frustum;
            RwInt32 plane;

            /* Ok first we need to calculate the projection data. */

            /* Get world space position of shadow frame. */
            shadow = TeamFrameGetLTM(renderData->shadows->shadowFrames[j]);

            /* Calculate projection vector from frame to center. */
            RwV3dSub(&projection, shadowCenter, &(shadow->pos));

            /* Calculate projection scale. */
            projScale = 1.0f / RwV3dDotProduct(&projection, worldPlaneNormal);

            /* Scale projection vector. */
            projScaleVector.x = projScale * RwV3dLength(&(playerLTM->right));
            projScaleVector.y = projScale * RwV3dLength(&(playerLTM->up));
            projScaleVector.z = projScale * RwV3dLength(&(playerLTM->at));

            /* However, the world projection vector we use for rendering
             * must contain the scale factor, else the rendering will
             * be incorrect.
             */
            worldProjectionRender.x = projection.x * projScaleVector.x;
            worldProjectionRender.y = projection.y * projScaleVector.y;
            worldProjectionRender.z = projection.z * projScaleVector.z;

            /* The world projection vector we use for the clipping test,
             * doesn't contain (possibly) a scale. Hence we use the
             * uncorrect projScale.
             */
            RwV3dScale(&worldProjection, &projection, projScale);

            /*-------------------------------------------------*/

            /* Again we do the following transform for rendering. */

            /* Transform (render) projection into object space. */
            TeamV3dTransformVectors( &objProjection,
                                     &worldProjectionRender,
                                     &playerInverse );

            /*-------------------------------------------------*/

            /* Now time to calculate the clipping flags. */

            /* Need to project the center. */
            RwV3dSub(&difference, worldPlanePosition, shadowCenter);

            /* Dot product the normal. */
            shadowScale = RwV3dDotProduct(&difference, worldPlaneNormal);

            /* Scale the projection by the shadow scale. */
            RwV3dScale(&projectionScaled, &worldProjection, shadowScale);

            /* Translate the sphere center. */
            RwV3dAdd(&projShadowCenter, shadowCenter, &projectionScaled);

            /* Now calculate the projected shadow radius. */

            /* Calculate length of projection, and radius */
            projLengthSqu = RwV3dDotProduct(&projection, &projection);
            projRadius = RwV3dDotProduct(&projection, worldPlaneNormal);

            /* Calculate projected shadow radius. */
            projShadowSqu = ( ( boundingSphere.radius *
                                boundingSphere.radius ) *
                              projLengthSqu ) /
                            ( projRadius * projRadius );

            /*-------------------------------------------------*/

            /* Now need to frustrum test the shadow. */

            /* Assume inside until we decide otherwise */
            result = rwSPHEREINSIDE;

            /* Grab the frustum. */
            frustum = camera->frustumPlanes;

            /* 0: far  1: near  2: left 3: top  4: right 5: bottom */
            for( plane = 0; plane < 6; plane++ )
            {
                /* Distance of center to plane. */
                RwReal planeDist;
                RwReal planeDistSqu;

                /* Distance of center to shadow edge. */
                RwReal circleDistSqu;

                RwReal dot;

                /*-------------------------------------------------*/

                /* Calculate the plane center distance. */
                planeDist = RwV3dDotProduct( &projShadowCenter,
                                             &(frustum[plane].plane.normal) );
                planeDist -= frustum[plane].plane.distance;

                /* Square the distance. */
                planeDistSqu = planeDist * planeDist;

                /* Calculate the shadow center distance. */

                /* Dot the two normals. */
                dot = RwV3dDotProduct( worldPlaneNormal,
                                       &(frustum[plane].plane.normal) );

                /* Calculate the projected circle distance. */
                circleDistSqu = projShadowSqu * (1.0f - (dot * dot));

                /*-------------------------------------------------*/

                if (circleDistSqu > planeDistSqu)
                {
                    result = rwSPHEREBOUNDARY;
                }
                else if (planeDist > 0)
                {
                    result = rwSPHEREOUTSIDE;
                    break;
                }
            }

            /* Do we need to draw the shadow? */
            if (result != rwSPHEREOUTSIDE)
            {
                /* Setup the render data. */
                RwV3dAssign( &(renderData->localProjection[i].v3d.v),
                             &objProjection );

                RwV3dAssign( &(renderData->worldProjection[i].v3d.v),
                             &worldProjection );

                /* Set the render shadow value. */
                renderData->renderValues[i] =
                    renderData->shadows->shadowValues[j];

                /* Increase the shadow clip flag for his shadow. */
                renderData->shadowClip += (shadowClip[result] << i);

                /* Ok move on to the next shadow slot. */
                i++;
            }
        }

        /* Hold the number of shadows to render this frame. */
        renderData->renderNumShadows = i;

        /* Setup the render data. */
        RwV3dAssign( &(renderData->localPlaneNormal.v3d.v),
                     &objPlaneNormal );

        RwV3dAssign( &(renderData->localPlanePos.v3d.v),
                     &objPlanePosition );

        RwV3dAssign( &(renderData->worldPlaneNormal.v3d.v),
                     worldPlaneNormal );

        RwV3dAssign( &(renderData->worldPlanePos.v3d.v),
                     worldPlanePosition );
    }
    else
    {
        /* Set the number of shadows to zero. */
        renderData->renderNumShadows = 0;
    }

    ret = renderData->playerClip + renderData->renderNumShadows;

#if (0 && defined(RWDEBUG))
    {
        static RwUInt32 count = 0;
        static RwChar names[][20] = { "Discarded", "Not Clipped",
                                      "ASSERT!",   "Clipped" };

        if(count ++ > 300)
        {
            printf("\nShadow clip data\n================\n");
            printf("# of shadows: %i\n", renderData->renderNumShadows);
            printf("player clip flag: %s\n", names[renderData->playerClip]);
            printf("shadow clip flag: %i\n", renderData->shadowClip);

            count = 0;
        }
    }
#endif /* (0 && defined(RWDEBUG)) */

    RWRETURN((RwFrustumTestResult)ret);
}
/*----------------------------------------------------------------------*/

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rpteam
 * \ref RpTeamSetShadows is used to set the shadow data for a team. Each team
 * object requires it's own shadow data, although teams may share shadow data.
 *
 * The team should be locked before it's shadow data is attached with
 * \ref RpTeamLockMode rpTEAMLOCKSHADOWS.
 *
 * \param team Pointer to the team.
 * \param shadows
 *
 * \return Returns a pointer to the atomic if successful or NULL if there is an
 * error.
 *
 * \see RpTeamRender
 * \see RpTeamLock
 * \see RpTeamUnlock
 * \see RpTeamGetShadows
 */
RpTeam *
RpTeamSetShadows( RpTeam *team,
                  RpTeamShadows *shadows )
{
    RWAPIFUNCTION(RWSTRING("RpTeamSetShadows"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != team);
    RWASSERT(team->lockFlags & rpTEAMLOCKSHADOWS);

    team->shadows = shadows;

    RWRETURN(team);
}

/**
 * \ingroup rpteam
 * \ref RpTeamGetShadows is used to get the shadow data for a team.
 *
 * \param team Pointer to the team.
 *
 * \return Returns a pointer to the shadow data if successful or NULL if none
 * has been set.
 *
 * \see RpTeamRender
 * \see RpTeamSetShadows
 */
RpTeamShadows *
RpTeamGetShadows(const RpTeam *team)
{
    RpTeamShadows *shadows;

    RWAPIFUNCTION(RWSTRING("RpTeamGetShadows"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != team);

    shadows = team->shadows;

    RWRETURN(shadows);
}
