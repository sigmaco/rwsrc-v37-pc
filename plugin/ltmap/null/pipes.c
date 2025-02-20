/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   pipes.c                                                    -*
 *-                                                                         -*
 *-  Purpose :   NULL lightmap pipeline extension                           -*
 *-                                                                         -*
 *===========================================================================*/

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpltmap.h"


/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/

#define NULLATOMICPIPE 0
#define NULLSECTORPIPE 1
#define NULLNUMPIPES   2

#define _rpLtMapPipeline(_index) (_rpLtMapGlobals.platformPipes[_index])

/*===========================================================================*
 *--- Plugin Internal Platform-specific Functions ---------------------------*
 *===========================================================================*/


/****************************************************************************
 _rpLtMapGetPlatformAtomicPipeline
 */
RxPipeline *
_rpLtMapGetPlatformAtomicPipeline(void)
{
    RxPipeline *pipeline;

    RWFUNCTION(RWSTRING("_rpLtMapGetPlatformAtomicPipeline"));
    RWASSERT(0 < _rpLtMapGlobals.module.numInstances);

    pipeline = _rpLtMapPipeline(NULLATOMICPIPE);

    RWRETURN(pipeline);
}

/****************************************************************************
 _rpLtMapGetPlatformWorldSectorPipeline
 */
RxPipeline *
_rpLtMapGetPlatformWorldSectorPipeline(void)
{
    RxPipeline *pipeline;

    RWFUNCTION(RWSTRING("_rpLtMapGetPlatformWorldSectorPipeline"));
    RWASSERT(0 < _rpLtMapGlobals.module.numInstances);

    pipeline = _rpLtMapPipeline(NULLSECTORPIPE);

    RWRETURN(pipeline);
}

/****************************************************************************
 _rpLtMapPlatformPipelinesDestroy
 */
void
_rpLtMapPlatformPipelinesDestroy(void)
{
    RWFUNCTION(RWSTRING("_rpLtMapPlatformPipelinesDestroy"));

    if (NULL != _rpLtMapPipeline(NULLSECTORPIPE))
    {
        RxPipelineDestroy(_rpLtMapPipeline(NULLSECTORPIPE));
         _rpLtMapPipeline(NULLSECTORPIPE) = (RxPipeline *)NULL;
    }

    if (NULL != _rpLtMapPipeline(NULLATOMICPIPE))
    {
        RxPipelineDestroy(_rpLtMapPipeline(NULLATOMICPIPE));
         _rpLtMapPipeline(NULLATOMICPIPE) = (RxPipeline *)NULL;
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpLtMapPlatformPipelinesCreate
 */
RwBool
_rpLtMapPlatformPipelinesCreate(void)
{
    RWFUNCTION(RWSTRING("_rpLtMapPlatformPipelinesCreate"));

    RWASSERT(LTMAPMAXPIPES >= NULLNUMPIPES);

    /* NULL pipelines are (surprise surprise) NULL */
    _rpLtMapPipeline(NULLATOMICPIPE) = (RxPipeline *)NULL;
    _rpLtMapPipeline(NULLSECTORPIPE) = (RxPipeline *)NULL;

    RWRETURN(TRUE);
}
