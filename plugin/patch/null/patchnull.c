/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rppatch.h"

#include "patch.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/
PatchGlobals _rpPatchGlobals =
{
    0,                          /* RwInt32        engineOffset      */
    0,                          /* RwInt32        atomicOffset      */
    0,                          /* RwInt32        geometryOffset    */
    { 0, 0 },                   /* RwModuleInfo   module            */
    (RwFreeList *)NULL,         /* RwFreeList    *atomicFreeList    */
    (RwFreeList *)NULL,         /* RwFreeList    *geometryFreeList  */
    {
        {                       /* PatchPlatform  platform          */
            (RxPipeline *)NULL, /*   rpPATCHNULLPIPELINEGENERIC     */
            (RxPipeline *)NULL, /*   rpPATCHNULLPIPELINEMATFX       */
            (RxPipeline *)NULL, /*   rpPATCHNULLPIPELINESKINNED     */
            (RxPipeline *)NULL  /*   rpPATCHNULLPIPELINESKINMATFX   */
        }
    }
};

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

 Create the patch pipelines.

 Inputs :   pipes - RwUInt32 bit-field defining which pipes to create
                        rpPATCHPIPELINEGENERIC = 0x01,
                        rpPATCHPIPELINESKINNED = 0x02,
                        rpPATCHPIPELINEMATFX   = 0x04,
 Outputs:   RwBool - on success.
 */
RwBool
_rpPatchPipelinesCreate(RwUInt32 pipes)
{
    RxPipeline *pipe;

    RWFUNCTION(RWSTRING("_rpPatchPipelinesCreate"));

    if(pipes & rpPATCHPIPELINEGENERIC)
    {
        pipe = RxPipelineCreate();
        RWASSERT(NULL != pipe);

        pipe->pluginId = rwID_PATCHPLUGIN;
        pipe->pluginData = rpPATCHTYPEGENERIC;

        _rpPatchPipeline(rpPATCHNULLPIPELINEGENERIC) = pipe;
        RWASSERT(NULL != _rpPatchPipeline(rpPATCHNULLPIPELINEGENERIC));
    }

    RWRETURN(TRUE);
}

/****************************************************************************
 _rpPatchPipelinesDestroy

 Destroy the patch pipelines.

 Inputs :   None
 Outputs:   RwBool - on success.
 */
RwBool
_rpPatchPipelinesDestroy(void)
{
    RpPatchNullPipeline iPipeline;

    RWFUNCTION(RWSTRING("_rpPatchPipelinesDestroy"));

    for( iPipeline = rpPATCHNULLPIPELINEGENERIC;
         iPipeline < rpPATCHNULLPIPELINEMAX;
         iPipeline = (RpPatchNullPipeline)(1 + (RwUInt32)iPipeline) )
    {
        if(NULL != _rpPatchPipeline(iPipeline))
        {
            RxPipelineDestroy(_rpPatchPipeline(iPipeline));
            _rpPatchPipeline(iPipeline) = (RxPipeline *)NULL;
        }
        RWASSERT(NULL == _rpPatchPipeline(iPipeline));
    }

    RWRETURN(TRUE);
}

/****************************************************************************
 _rpPatchPipelinesAttach

 Attach the correct patch pipeline to the atomic.

 Inputs :   atomic *   - Pointer to the atomic.
            type       - RpPatchType requested.
 Outputs:   RpAtomic * - Pointer to the atomic on success.
 */
RpAtomic *
_rpPatchPipelinesAttach( RpAtomic *atomic,
                         RpPatchType type __RWUNUSED__ )
{
    RpPatchMesh *patchMesh;

    RpPatchNullPipeline iPipeline;
    RxPipeline *pipeline;

    RWFUNCTION(RWSTRING("_rpPatchPipelinesAttach"));
    RWASSERT(NULL != atomic);

    /* Get the patch mesh. */
    patchMesh = RpPatchAtomicGetPatchMesh(atomic);
    RWASSERT(NULL != patchMesh);

    iPipeline = rpPATCHNULLPIPELINEGENERIC;

    pipeline = RpPatchGetNullPipeline(iPipeline);
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
