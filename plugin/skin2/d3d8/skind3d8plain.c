#include <d3d8.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"

#include "skin.h"

#include "skind3d8.h"
#include "skind3d8plain.h"

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
    RWFUNCTION(RWSTRING("_rpSkinPipelinesAttach"));

    RpAtomicSetPipeline(atomic, _rpSkinPipeline(rpSKIND3D8PIPELINEGENERIC));

    RWRETURN(atomic);
}

/****************************************************************************
 _rwD3D8SkinNeedsAManagedVertexBuffer
 Input :
 Output: always FALSE
 */
RwBool
_rwD3D8SkinNeedsAManagedVertexBuffer(const RpAtomic *atomic __RWUNUSED__,
                                     const RxD3D8ResEntryHeader *resEntryHeader __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("_rwD3D8SkinNeedsAManagedVertexBuffer"));

    RWRETURN(!_rpSkinGlobals.platform.hardwareTL);
}
