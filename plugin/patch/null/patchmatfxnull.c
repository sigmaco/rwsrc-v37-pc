/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpmatfx.h"
#include "rppatch.h"

#include "patch.h"
#include "patchmesh.h"
#include "patchmatfx.h"

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

    if(pipes & rpPATCHPIPELINEMATFX)
    {
        pipe = RxPipelineCreate();
        RWASSERT(NULL != pipe);

        pipe->pluginId = rwID_PATCHPLUGIN;
        pipe->pluginData = rpPATCHTYPEMATFX;

         _rpPatchPipeline(rpPATCHNULLPIPELINEMATFX) = pipe;
        RWASSERT(NULL != _rpPatchPipeline(rpPATCHNULLPIPELINEMATFX));
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
                         RpPatchType type )
{
    RpPatchMesh *patchMesh;

    RwBool uv2;
    RwBool matfx;

    RpPatchNullPipeline iPipeline;
    RxPipeline *pipeline;

    RWFUNCTION(RWSTRING("_rpPatchPipelinesAttach"));
    RWASSERT(NULL != atomic);

    /* Get the patch mesh. */
    patchMesh = RpPatchAtomicGetPatchMesh(atomic);
    RWASSERT(NULL != patchMesh);

    /* Get the uv2. */
    uv2 = (RpPatchMeshGetNumTexCoordSets(patchMesh) > 1);

    /* Is the patch mesh matfxed. */
    matfx = (_rpPatchMeshSearchForEffect(patchMesh) != rpMATFXEFFECTNULL);

    /* Do we have a hint? */
    if(type == rpNAPATCHTYPE)
    {
        iPipeline = rpPATCHNULLPIPELINEGENERIC;

        /* Is the patch mesh matfxed or uv2. */
        /* We only allow uv2 if matfx is on. */
        iPipeline = (RpPatchNullPipeline)
            ((uv2 * matfx) + matfx + (RwUInt32)iPipeline);
    }
    else
    {
        /*Ok we've been given a hint. */
        switch(type)
        {
            case rpPATCHTYPEGENERIC:
            case rpPATCHTYPESKIN:
                iPipeline = rpPATCHNULLPIPELINEGENERIC;
                break;
            case rpPATCHTYPEMATFX:
            case rpPATCHTYPESKINMATFX:
                iPipeline = rpPATCHNULLPIPELINEMATFX;
                break;
            default:
                iPipeline = rpNAPATCHNULLPIPELINE;
                break;
        }
    }

    RWASSERT(rpNAPATCHNULLPIPELINE < iPipeline);
    RWASSERT(rpPATCHNULLPIPELINEMAX > iPipeline);

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
