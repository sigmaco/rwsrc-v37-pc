#include <d3d8.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "skin.h"

#include "skind3d8.h"
#include "skind3d8plain.h"
#include "skind3d8toon.h"
#include "rptoon.h"
#include "../toon/d3d8/d3d8toon.h"
#include "../toon/pipes.h"

/****************************************************************************
 _rpSkinPipelinesCreate
 Create the skinning pipelines.
 Inputs :   None
 Outputs:   RwBool - on success.
 */
RwBool
_rpSkinPipelinesCreate(RwUInt32 pipes __RWUNUSED__)
{
    RwBool      rv = TRUE;
    RxPipeline  *pipeline;

    RWFUNCTION(RWSTRING("_rpSkinPipelinesCreate"));

    if(pipes & rpSKINPIPELINESKINGENERIC)
    {
        pipeline = _rpSkinD3D8CreatePlainPipe();
        if (!pipeline)
        {
            rv = FALSE;
        }

        _rpSkinPipeline(rpSKIND3D8PIPELINEGENERIC) = pipeline;
    }

    if(pipes & rpSKINPIPELINESKINTOON)
    {
        pipeline = _rpSkinD3D8CreateToonPipe();
        if (!pipeline)
        {
            rv = FALSE;
        }

        _rpSkinPipeline(rpSKIND3D8PIPELINETOON) = pipeline;
    }

    RWRETURN(rv);
}

/****************************************************************************
 _rpSkinPipelinesAttach
 Attach the generic skinning pipeline to an atomic.
 Inputs :   atomic *   - Pointer to the atomic.
 Outputs:   RpAtomic * - Pointer to the atomic on success.
 */
RpAtomic *
_rpSkinPipelinesAttach(RpAtomic *atomic, RpSkinType type )
{
    RpSkinD3D8Pipeline iPipeline;
    RxPipeline *pipeline;

    RWFUNCTION(RWSTRING("_rpSkinPipelinesAttach"));
    RWASSERT(NULL != atomic);

    /*Ok we've been given a hint. */
    switch(type)
    {
        case rpSKINTYPETOON:
            iPipeline = rpSKIND3D8PIPELINETOON;
            _rpToonAtomicChainSkinnedAtomicRenderCallback(atomic);
            break;
            /* We haven't got it so you get generic instead */
        default:
            iPipeline = rpSKIND3D8PIPELINEGENERIC;
            break;
    }

    RWASSERT(rpNASKIND3D8PIPELINE < iPipeline);
    RWASSERT(rpSKIND3D8PIPELINEMAX > iPipeline);

    pipeline = _rpSkinPipeline(iPipeline);
    RWASSERT(NULL != pipeline);

    atomic = RpAtomicSetPipeline(atomic, pipeline);
    RWASSERT(NULL != atomic);

    RWRETURN(atomic);
}

RwBool
_rwD3D8SkinNeedsAManagedVertexBuffer(const RpAtomic *atomic,
                                     const RxD3D8ResEntryHeader *resEntryHeader __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("_rwD3D8SkinNeedsAManagedVertexBuffer"));

    RWRETURN(_rwD3D8SkinToonNeedsAManagedVertexBuffer(atomic));
}
