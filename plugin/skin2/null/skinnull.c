/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpskin.h"

#include "skin.h"

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

/****************************************************************************
 _rpSkinPipelinesCreate

 Create the skinning pipelines.
 We create the generic pipelines by cloning the generic rendering pipelines
 and attaching the pluginId and pluginData fields to the clone.

 Inputs :   None
 Outputs:   RwBool - on success.
 */
RwBool
_rpSkinPipelinesCreate(RwUInt32 pipes)
{
    RxPipeline *pipe;
    RxPipeline *clone;

    RWFUNCTION(RWSTRING("_rpSkinPipelinesCreate"));

    if(pipes & rpSKINPIPELINESKINGENERIC)
    {
        /* We create the GENERIC pipe by cloning the default pipe. */
        pipe = RpAtomicGetDefaultPipeline();
        RWASSERT(NULL != pipe);

        clone = RxPipelineClone(pipe);
        RWASSERT(NULL != clone);

        clone->pluginId = rwID_SKINPLUGIN;
        clone->pluginData = rpSKINTYPEGENERIC;

        _rpSkinPipeline(rpSKINNULLPIPELINEGENERIC) = clone;
        RWASSERT(NULL != _rpSkinPipeline(rpSKINNULLPIPELINEGENERIC));
    }

    RWRETURN(TRUE);
}


/****************************************************************************
 _rpSkinPipelinesDestroy
 Destroy the skinning pipelines.
 Inputs :   None
 Outputs:   RwBool - on success.
 */
RwBool
_rpSkinPipelinesDestroy(void)
{
    RWFUNCTION(RWSTRING("_rpSkinPipelinesDestroy"));

    if(NULL != _rpSkinPipeline(rpSKINNULLPIPELINEGENERIC))
    {
        RxPipelineDestroy(_rpSkinPipeline(rpSKINNULLPIPELINEGENERIC));
        _rpSkinPipeline(rpSKINNULLPIPELINEGENERIC) = (RxPipeline *)NULL;
    }

    RWRETURN(TRUE);
}


/****************************************************************************
 _rpSkinPipelinesAttach

 Attach the generic skinning pipeline to an atomic.

 Inputs :   atomic *   - Pointer to the atomic.
            type       - RpSkinType requested.
 Outputs:   RpAtomic * - Pointer to the atomic on success.
 */
RpAtomic *
_rpSkinPipelinesAttach( RpAtomic *atomic,
                        RpSkinType type __RWUNUSED__ )
{
    RxPipeline *pipeline;

    RWFUNCTION(RWSTRING("_rpSkinPipelinesAttach"));
    RWASSERT(NULL != atomic);

    pipeline = _rpSkinPipeline(rpSKINNULLPIPELINEGENERIC);
    RWASSERT(NULL != pipeline);

    atomic = RpAtomicSetPipeline(atomic, pipeline);
    RWASSERT(NULL != atomic);

    RWRETURN(atomic);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

