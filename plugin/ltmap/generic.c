/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   pipes.c                                                    -*
 *-                                                                         -*
 *-  Purpose :   Generic lightmap pipelines                                 -*
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
#include "generic.h"

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/

#define GENERICATOMICPIPE   0
#define GENERICSECTORPIPE   1

#define _rpLtMapPipeline(_index) (_rpLtMapGlobals.genericPipes[_index])

/*===========================================================================*
 *--- Plugin Internal Platform-specific Functions ---------------------------*
 *===========================================================================*/

#if 0

/****************************************************************************
 _rpLtMapGenericAtomicPipeCreate
 */
static RxPipeline *
_rpLtMapGenericAtomicPipeCreate(void)
{
    RWFUNCTION(RWSTRING("_rpLtMapGenericAtomicPipeCreate"));

    /* Not written yet */

    RWRETURN(NULL);
}

/****************************************************************************
 _rpLtMapGenericWorldSectorPipeCreate
 */
static RxPipeline *
_rpLtMapGenericWorldSectorPipeCreate(void)
{
    RWFUNCTION(RWSTRING("_rpLtMapGenericWorldSectorPipeCreate"));

    /* Not written yet */

    RWRETURN(NULL);
}

/****************************************************************************
 _rpLtMapGetGenericAtomicPipeline
 */
RxPipeline *
_rpLtMapGetGenericAtomicPipeline(void)
{
    RxPipeline *pipeline;

    RWFUNCTION(RWSTRING("_rpLtMapGetGenericAtomicPipeline"));
    RWASSERT(0 < _rpLtMapGlobals.module.numInstances);

    pipeline = _rpLtMapPipeline(GENERICATOMICPIPE);

    RWRETURN(pipeline);
}

/****************************************************************************
 _rpLtMapGetGenericWorldSectorPipeline
 */
RxPipeline *
_rpLtMapGetGenericWorldSectorPipeline(void)
{
    RxPipeline *pipeline;

    RWFUNCTION(RWSTRING("_rpLtMapGetGenericWorldSectorPipeline"));
    RWASSERT(0 < _rpLtMapGlobals.module.numInstances);

    pipeline = _rpLtMapPipeline(GENERICSECTORPIPE);

    RWRETURN(pipeline);
}

#endif /* 0 */

/****************************************************************************
 _rpLtMapGenericPipelinesDestroy
 */
void
_rpLtMapGenericPipelinesDestroy(void)
{
    RWFUNCTION(RWSTRING("_rpLtMapGenericPipelinesDestroy"));

    if (NULL != _rpLtMapPipeline(GENERICSECTORPIPE))
    {
        RxPipelineDestroy(_rpLtMapPipeline(GENERICSECTORPIPE));
         _rpLtMapPipeline(GENERICSECTORPIPE) = (RxPipeline *)NULL;
    }

    if (NULL != _rpLtMapPipeline(GENERICATOMICPIPE))
    {
        RxPipelineDestroy(_rpLtMapPipeline(GENERICATOMICPIPE));
         _rpLtMapPipeline(GENERICATOMICPIPE) = (RxPipeline *)NULL;
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpLtMapGenericPipelinesCreate
 */
RwBool
_rpLtMapGenericPipelinesCreate(void)
{
    RWFUNCTION(RWSTRING("_rpLtMapGenericPipelinesCreate"));

    /* The generic pipelines are not written yet! */
    _rpLtMapPipeline(GENERICATOMICPIPE) = NULL;
    _rpLtMapPipeline(GENERICSECTORPIPE) = NULL;
    RWRETURN(TRUE);

#if 0
    /* Create the pipelines */

    /* D3D8 atomic lightmap pipeline */
    _rpLtMapPipeline(GENERICATOMICPIPE) = _rpLtMapGenericAtomicPipeCreate();
    RWASSERT(NULL != _rpLtMapPipeline(GENERICATOMICPIPE));

    /* D3D8 sector lightmap pipeline. */
    _rpLtMapPipeline(GENERICSECTORPIPE) = _rpLtMapGenericWorldSectorPipeCreate();
    RWASSERT(NULL != _rpLtMapPipeline(GENERICSECTORPIPE));

    if ((NULL == _rpLtMapPipeline(GENERICATOMICPIPE)) ||
        (NULL == _rpLtMapPipeline(GENERICSECTORPIPE)) )
    {
        _rpLtMapGenericPipelinesDestroy();
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
#endif /* 0 */
}
