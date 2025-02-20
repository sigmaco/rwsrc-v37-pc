/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>
#include "rpdbgerr.h"

#include "rpteam.h"
#include "rpmatfx.h"

#include "team.h"
#include "teamstatic.h"
#include "teamskin.h"
#include "teammatfx.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
/**
 * \defgroup rpteamd3d8 D3D8
 * \ingroup rpteam
 *
 * D3D8 platform specific documentation for RpTeam.
 */

/**\ingroup rpteamd3d8
 *\page WarningD3D8ShadowsUseStencilBuffer D3D8 Shadows Use the Stencil Buffer
 *
 * The D3D8 implementation of RpTeam makes use of the stencil buffer to create
 * shadows. For it to work properly, you must clear the stencil buffer to 0
 * when you clear the main camera's raster. You can do this with this code:
 *
 *\verbatim
 #if (defined (D3D8_DRVMODEL_H))
     RwD3D8SetStencilClear(0);

     RwCameraClear(Camera, &BackgroundColor,
                  rwCAMERACLEARZ | rwCAMERACLEARIMAGE | rwCAMERACLEARSTENCIL );
 #else
     RwCameraClear(Camera, &BackgroundColor,
                  rwCAMERACLEARZ | rwCAMERACLEARIMAGE );
 #endif
 \endverbatim
 */

/**
 * \ingroup rpteamd3d8
 * \page d3d8compatibility D3D8 Compatibility Matrix
 *
 * Listed below are the supported D3D8 options for RpTeam.
 *
 * \verbatim
                           SUPPORTED
   LIGHTING
   Ambient                    Yes
   Directional                Yes
   Other                       No

   SKINNING
   Maximum Weights              2

   MATERIAL EFFECTS
   Dual                    Soft/Hard
   Environmental           Soft/Hard
   Other

   GEOMETRY PROPERTIES
   Pre-light color             No
   Pre-light color alpha       No
   Material color             Yes
   Material color alpha       Yes

   FOG                        Yes
   \endverbatim
 *
 */

/**
 * \ingroup rpteamd3d8
 * \page d3d8palettelimitations D3D8 Palettized Textures Limitations
 *
 * The D3D8 version of RpTeam does not support using two
 * palettized textures in the same material effect for skinned objects.
 *
 * \see \ref rpmatfxd3d8overview
 *
 */

static RxPipeline *TeamD3D8Pipelines[TEAMD3D8PIPEID_MAX] =
{
    (RxPipeline *)NULL,     /* TEAMD3D8PIPEID_NAPIPE            */
    (RxPipeline *)NULL,     /* TEAMD3D8PIPEID_STATIC            */
    (RxPipeline *)NULL,     /* TEAMD3D8PIPEID_SKINNED           */
    (RxPipeline *)NULL,     /* TEAMD3D8PIPEID_STATIC_MATFX      */
    (RxPipeline *)NULL,     /* TEAMD3D8PIPEID_SKINNED_MATFX     */
    (RxPipeline *)NULL,     /* TEAMD3D8PIPEID_STATIC_SHADOW     */
    (RxPipeline *)NULL,     /* TEAMD3D8PIPEID_SKINNED_SHADOW    */
};

/****************************************************************************
 _rpTeamD3D8AddRenderPipeline

 Add a render pipeline to the render pipelines array.

 Inputs RxPipeline * - pipeline to be added
 */
void
_rpTeamD3D8SetRenderPipeline(RxPipeline *pipe, TeamD3D8PipeId pipeId)
{
    RWFUNCTION(RWSTRING("_rpTeamD3D8SetRenderPipeline"));
    RWASSERT(pipeId > TEAMD3D8PIPEID_NAPIPE && pipeId < TEAMD3D8PIPEID_MAX);

    TeamD3D8Pipelines[pipeId] = pipe;

    RWRETURNVOID();
}

/****************************************************************************
 _rpTeamGetRenderPipelines

 Returns a pointer to the array of rendering pipelines.

 Inputs :
 Outputs: RxPipeline ** - Array of rendering pipelines.
 */
RxPipeline **
_rpTeamGetRenderPipelines(void)
{
    RxPipeline **pipelines;

    RWFUNCTION(RWSTRING("_rpTeamGetRenderPipelines"));

    pipelines = TeamD3D8Pipelines;

    RWRETURN(pipelines);
}

/****************************************************************************
 _rpTeamInstancePipelinesDestroy

 Destroys the instance only pipelines.

 Inputs : void
 Outputs: void
 */
void
_rpTeamInstancePipelinesDestroy(void)
{
    RWFUNCTION(RWSTRING("_rpTeamInstancePipelinesDestroy"));

    RWRETURNVOID();
}

/****************************************************************************
 _rpTeamInstancePipelinesCreate

 Destroys the instance only pipelines.

 Inputs : void
 Outputs: RxPipeline ** - Array of instance only pipelines.
 */
RxPipeline **
_rpTeamInstancePipelinesCreate(void)
{
    RWFUNCTION(RWSTRING("_rpTeamInstancePipelinesCreate"));

    RWRETURN(NULL);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/
