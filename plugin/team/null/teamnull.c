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

  NULL

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

/*---------------------------------------------------------------------------*/
RwBool
_rpTeamMatFXPipelineCreate(void)
{
    RWFUNCTION(RWSTRING("_rpTeamMatFXPipelineCreate"));
    RWRETURN(FALSE);
}

void
_rpTeamMatFXPipelineDestroy(void)
{
    RWFUNCTION(RWSTRING("_rpTeamMatFXPipelineDestroy"));
    RWRETURNVOID();
}

RxPipeline *
_rpTeamMatFXGetPipeline(RpTeamElementType type __RWUNUSED__,
                        RwBool uv2 __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("_rpTeamMatFXGetPipeline"));
    RWRETURN((RxPipeline *)NULL);
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
RwBool
_rpTeamShadowPipelineCreate(void)
{
    RWFUNCTION(RWSTRING("_rpTeamShadowPipelineCreate"));
    RWRETURN(FALSE);
}

void
_rpTeamShadowPipelineDestroy(void)
{
    RWFUNCTION(RWSTRING("_rpTeamShadowPipelineDestroy"));
    RWRETURNVOID();
}

RxPipeline *
_rpTeamShadowGetPipeline(RpTeamElementType type __RWUNUSED__,
                         RwBool allInOne __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("_rpTeamShadowGetPipeline"));
    RWRETURN((RxPipeline *)NULL);
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
RwBool
_rpTeamStaticCustomPipelineCreate(void)
{
    RWFUNCTION(RWSTRING("_rpTeamStaticCustomPipelineCreate"));
    RWRETURN(FALSE);
}

void
_rpTeamStaticCustomPipelineDestroy(void)
{
    RWFUNCTION(RWSTRING("_rpTeamStaticCustomPipelineDestroy"));
    RWRETURNVOID();
}

RxPipeline *
_rpTeamStaticGetCustomPipeline(void)
{
    RWFUNCTION(RWSTRING("_rpTeamStaticGetCustomPipeline"));
    RWRETURN((RxPipeline *)NULL);
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
RwBool
_rpTeamSkinCustomPipelineCreate(void)
{
    RWFUNCTION(RWSTRING("_rpTeamSkinCustomPipelineCreate"));
    RWRETURN(FALSE);
}

void
_rpTeamSkinCustomPipelineDestroy(void)
{
    RWFUNCTION(RWSTRING("_rpTeamSkinCustomPipelineDestroy"));
    RWRETURNVOID();
}

RxPipeline *
_rpTeamSkinGetCustomPipeline(void)
{
    RWFUNCTION(RWSTRING("_rpTeamSkinGetCustomPipeline"));
    RWRETURN((RxPipeline *)NULL);
}
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
RwMatrix *
_rpTeamSkinMatBlendUpdating( RpSkin * skin __RWUNUSED__,
                             RwFrame * frame __RWUNUSED__,
                             RpHAnimHierarchy * hierarchy __RWUNUSED__,
                             RwMatrix * pMBA __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("_rpTeamSkinMatBlendUpdating"));
    RWRETURN((RwMatrix *)NULL);
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
const RpTeam *
_rpTeamRender(const RpTeam *team)
{
    RWFUNCTION(RWSTRING("_rpTeamRender"));
    RWRETURN(team);
}
/*---------------------------------------------------------------------------*/

/****************************************************************************
 _rpTeamGetRenderPipelines

 Returns a pointer to the array of rendering pipelines.

 Inputs :
 Outputs: RxPipeline ** - Array of rendering pipelines.
 */
RxPipeline **
_rpTeamGetRenderPipelines(void)
{
    RWFUNCTION(RWSTRING("_rpTeamGetRenderPipelines"));
    RWRETURN((RxPipeline **)NULL);
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
    RWRETURN((RxPipeline **)NULL);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/
