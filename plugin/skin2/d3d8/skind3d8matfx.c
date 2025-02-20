#include <d3d8.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpmatfx.h"

#include "skin.h"

#include "skind3d8.h"
#include "skind3d8matfx.h"
#include "skind3d8plain.h"

/****************************************************************************
 _rpSkinPipelinesCreate
 Create the skinning pipelines.
 Inputs :   None
 Outputs:   RwBool - on success.
 */
RwBool
_rpSkinPipelinesCreate(RwUInt32 pipes)
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

    if(pipes & rpSKINPIPELINESKINMATFX)
    {
        pipeline = _rpSkinD3D8CreateMatFXPipe();
        if (!pipeline)
        {
            rv = FALSE;
        }

        _rpSkinPipeline(rpSKIND3D8PIPELINEMATFX) = pipeline;
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
_rpSkinPipelinesAttach(RpAtomic *atomic,
                        RpSkinType type __RWUNUSED__)
{
    RpSkinD3D8Pipeline iPipeline;
    RxPipeline *pipeline;

    RWFUNCTION(RWSTRING("_rpSkinPipelinesAttach"));
    RWASSERT(NULL != atomic);

    /*Ok we've been given a hint. */
    switch(type)
    {
        case rpSKINTYPEMATFX:
            iPipeline = rpSKIND3D8PIPELINEMATFX;
            break;
        /* we haven't got it - you get generic */
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
_rwD3D8SkinNeedsAManagedVertexBuffer(const RpAtomic *atomic, const RxD3D8ResEntryHeader *resEntryHeader)
{
    RWFUNCTION(RWSTRING("_rwD3D8SkinNeedsAManagedVertexBuffer"));

    RWRETURN(_rwD3D8SkinMatFXNeedsAManagedVertexBuffer(resEntryHeader));
}
