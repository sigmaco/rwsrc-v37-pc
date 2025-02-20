/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpskin.h"
#include "rpmatfx.h"

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

    RWFUNCTION(RWSTRING("_rpSkinPipelinesCreate"));

    if(pipes & rpSKINPIPELINESKINGENERIC)
    {
        pipe = RxPipelineCreate();
        RWASSERT(NULL != pipe);

        pipe->pluginId = rwID_SKINPLUGIN;
        pipe->pluginData = rpSKINTYPEGENERIC;

        _rpSkinPipeline(rpSKINNULLPIPELINEGENERIC) = pipe;
        RWASSERT(NULL != _rpSkinPipeline(rpSKINNULLPIPELINEGENERIC));
    }

    if(pipes & rpSKINPIPELINESKINMATFX)
    {
        pipe = RxPipelineCreate();
        RWASSERT(NULL != pipe);

        pipe->pluginId = rwID_SKINPLUGIN;
        pipe->pluginData = rpSKINTYPEMATFX;

        _rpSkinPipeline(rpSKINNULLPIPELINEMATFX) = pipe;
        RWASSERT(NULL != _rpSkinPipeline(rpSKINNULLPIPELINEMATFX));
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

    if(NULL != _rpSkinPipeline(rpSKINNULLPIPELINEMATFX))
    {
        RxPipelineDestroy(_rpSkinPipeline(rpSKINNULLPIPELINEMATFX));
        _rpSkinPipeline(rpSKINNULLPIPELINEMATFX) = (RxPipeline *)NULL;
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
                        RpSkinType type )
{
    RpSkinNullPipeline iPipeline;
    RxPipeline *pipeline;

    RWFUNCTION(RWSTRING("_rpSkinPipelinesAttach"));
    RWASSERT(NULL != atomic);

    /*Ok we've been given a hint. */
    switch(type)
    {
        case rpSKINTYPEMATFX:
            iPipeline = rpSKINNULLPIPELINEMATFX;
            break;
        default:
            /* we haven't got other stuff, you get generic */
            iPipeline = rpSKINNULLPIPELINEGENERIC;
            break;
    }

    RWASSERT(rpSKINNULLPIPELINEMAX > iPipeline);

    pipeline = RpSkinGetNullPipeline(iPipeline);
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

