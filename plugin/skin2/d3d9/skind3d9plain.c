#include <d3d9.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"

#include "skin.h"

#include "skind3d9.h"
#include "skind3d9plain.h"

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
        pipeline = _rpSkinD3D9CreatePlainPipe();
        if (!pipeline)
        {
            rv = FALSE;
        }

        _rpSkinPipeline(rpSKIND3D9PIPELINEGENERIC) = pipeline;
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

    RpAtomicSetPipeline(atomic, _rpSkinPipeline(rpSKIND3D9PIPELINEGENERIC));

    RWRETURN(atomic);
}

/****************************************************************************
 _rwD3D9SkinNeedsAManagedVertexBuffer
 */
RwBool
_rwD3D9SkinNeedsAManagedVertexBuffer(const RpAtomic *atomic __RWUNUSED__,
                                     const RxD3D9ResEntryHeader *resEntryHeader __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("_rwD3D9SkinNeedsAManagedVertexBuffer"));

    RWRETURN(_rwD3D9SkinUseVertexShader(atomic) ||
             _rpSkinGlobals.platform.hardwareTL == FALSE);
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
