#include <d3d9.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpmatfx.h"

#include "skin.h"

#include "skind3d9.h"
#include "skind3d9matfx.h"
#include "skind3d9plain.h"

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
        pipeline = _rpSkinD3D9CreatePlainPipe();
        if (!pipeline)
        {
            rv = FALSE;
        }

        _rpSkinPipeline(rpSKIND3D9PIPELINEGENERIC) = pipeline;
    }

    if(pipes & rpSKINPIPELINESKINMATFX)
    {
        pipeline = _rpSkinD3D9CreateMatFXPipe();
        if (!pipeline)
        {
            rv = FALSE;
        }

        _rpSkinPipeline(rpSKIND3D9PIPELINEMATFX) = pipeline;
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
    RpSkinD3D9Pipeline iPipeline;
    RxPipeline *pipeline;

    RWFUNCTION(RWSTRING("_rpSkinPipelinesAttach"));
    RWASSERT(NULL != atomic);

    /*Ok we've been given a hint. */
    switch(type)
    {
        case rpSKINTYPEMATFX:
            iPipeline = rpSKIND3D9PIPELINEMATFX;
            break;
        /* we haven't got it - you get generic */
        default:
            iPipeline = rpSKIND3D9PIPELINEGENERIC;
            break;
    }

    RWASSERT(rpNASKIND3D9PIPELINE < iPipeline);
    RWASSERT(rpSKIND3D9PIPELINEMAX > iPipeline);

    pipeline = _rpSkinPipeline(iPipeline);
    RWASSERT(NULL != pipeline);

    atomic = RpAtomicSetPipeline(atomic, pipeline);
    RWASSERT(NULL != atomic);

    RWRETURN(atomic);
}

/****************************************************************************
 _rwD3D9SkinNeedsAManagedVertexBuffer
 */
RwBool
_rwD3D9SkinNeedsAManagedVertexBuffer(const RpAtomic *atomic, const RxD3D9ResEntryHeader *resEntryHeader)
{
    RWFUNCTION(RWSTRING("_rwD3D9SkinNeedsAManagedVertexBuffer"));

    RWRETURN(_rwD3D9SkinUseVertexShader(atomic) ||
             _rwD3D9SkinMatFXNeedsAManagedVertexBuffer(resEntryHeader));
}

/****************************************************************************
 _rwD3D9SkinUseVertexShader
 */
RwBool
_rwD3D9SkinUseVertexShader(const RpAtomic *atomic)
{
    RWFUNCTION(RWSTRING("_rwD3D9SkinUseVertexShader"));

    if (_rpSkinGlobals.platform.hardwareTL &&
        _rpSkinGlobals.platform.hardwareVS)
    {
        const RpGeometry    *geometry;
        const RpSkin        *skin;

        geometry = RpAtomicGetGeometry(atomic);
        RWASSERT(NULL != geometry);

        skin = *RPSKINGEOMETRYGETCONSTDATA(geometry);

        if ( skin != NULL &&
             skin->platformData.maxNumBones <= _rpSkinGlobals.platform.maxNumBones )
        {
            RWRETURN(TRUE);
        }
    }

    RWRETURN(FALSE);
}
