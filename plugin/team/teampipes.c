/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>
#include "rpdbgerr.h"

#include "rpteam.h"

#include "teampipes.h"

/*
 *

  CUSTOM PIPE OPEN AND CLOSE

 *
 */

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/
#if (defined(TEAMPIPEEXTENSIONS))
TeamPipeDefinition PipeData[TEAMPIPE_MAX];
TeamPipeDefinition *lastDefinition = (TeamPipeDefinition *)NULL;
#endif /* (defined(TEAMPIPEEXTENSIONS)) */

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
#if (defined(TEAMPIPEEXTENSIONS))

void
_rpTeamPipeRemovePipeDefinition(TeamPipeIndex pipeIndex)
{
    TeamPipeDefinition *definition;

    RWFUNCTION(RWSTRING("_rpTeamPipeRemovePipeDefinition"));
    RWASSERT(TEAMPIPE_MAX > pipeIndex);

    definition = _rpTeamPipeGetPipeDefinition(pipeIndex);

    if(NULL != definition->render)
    {
        RxPipelineDestroy(definition->render);
    }

    definition->render = (RxPipeline *)NULL;
    definition->open   = (TeamPipeOpen)NULL;
    definition->close  = (TeamPipeClose)NULL;

    RWRETURNVOID();
}

TeamPipeIndex
_rpTeamPipeAddPipeDefinition( RxPipeline *pipeline,
                              TeamPipeOpen openPipe,
                              TeamPipeClose closePipe )
{
    TeamPipeIndex pipeIndex;

    RWFUNCTION(RWSTRING("_rpTeamPipeAddPipeDefinition"));
    RWASSERT(NULL != pipeline);

    for( pipeIndex = 0; pipeIndex < TEAMPIPE_MAX; pipeIndex++ )
    {
        TeamPipeDefinition *definition;

        definition = _rpTeamPipeGetPipeDefinition(pipeIndex);

        if(definition->render == NULL)
        {
            definition->render = pipeline;
            definition->open   = openPipe;
            definition->close  = closePipe;

            RWRETURN(pipeIndex);
        }
    }

    RWASSERT(TEAMPIPE_MAX > pipeIndex);
    RWRETURN(TEAMPIPENULL);
}

#endif /* (defined(TEAMPIPEEXTENSIONS)) */

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/
