/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rppatch.h"

#include "patch.h"

#include "nodeD3D8patchatomicinstance.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/
PatchGlobals _rpPatchGlobals =
{
    0,                     /* RwInt32        engineOffset      */
    0,                     /* RwInt32        atomicOffset      */
    0,                     /* RwInt32        geometryOffset    */
    { 0, 0 },              /* RwModuleInfo   module            */
    (RwFreeList *)NULL,    /* RwFreeList    *atomicFreeList    */
    (RwFreeList *)NULL,    /* RwFreeList    *geometryFreeList  */
    {                      /* PatchPlatform  platform          */
        {
            (RxPipeline *)NULL, /* RxPipeline    *pipeline generic      */
            (RxPipeline *)NULL, /* RxPipeline    *pipeline matfx        */
            (RxPipeline *)NULL, /* RxPipeline    *pipeline skin         */
            (RxPipeline *)NULL, /* RxPipeline    *pipeline skinmatfx    */
        },
        FALSE,              /* Hardware T&L */
        FALSE               /* Can use 32bits indices */
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
_rpPatchPipelinesCreate(RwUInt32 pipes __RWUNUSED__)
{
    const D3DCAPS8  *d3d8Caps;
    RwInt32         i;

    RWFUNCTION(RWSTRING("_rpPatchPipelinesCreate"));

    for (i = 0; i < rpPATCHD3D8PIPELINEMAX; i++)
    {
        _rpPatchGlobals.platform.pipelines[i] = NULL;
    }


    _rpPatchGlobals.platform.pipelines[rpPATCHD3D8PIPELINEGENERIC] =
                                            _rwD3D8PatchPipelineCreateGeneric();

    _rpPatchGlobals.platform.pipelines[rpPATCHD3D8PIPELINEMATFX] =
                                            _rwD3D8PatchPipelineCreateMatFX();

    _rpPatchGlobals.platform.pipelines[rpPATCHD3D8PIPELINESKINNED] =
                                            _rwD3D8PatchPipelineCreateSkin();

    _rpPatchGlobals.platform.pipelines[rpPATCHD3D8PIPELINESKINMATFX] =
                                            _rwD3D8PatchPipelineCreateSkinMatFX();

    RWASSERT(NULL != _rpPatchGlobals.platform.pipelines);

    d3d8Caps = (const D3DCAPS8 *)RwD3D8GetCaps();

    _rpPatchGlobals.platform.hardwareTL =
        ((d3d8Caps->DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0);

    _rpPatchGlobals.platform.use32bitsIndexBuffers =
        (_rpPatchGlobals.platform.hardwareTL &&
         (d3d8Caps->VertexShaderVersion & 0xffff) >= 0x0101);

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
    RwInt32 i;

    RWFUNCTION(RWSTRING("_rpPatchPipelinesDestroy"));

    for (i = 0; i < rpPATCHD3D8PIPELINEMAX; i++)
    {
        if(NULL != _rpPatchGlobals.platform.pipelines[i])
        {
            RxPipelineDestroy(_rpPatchGlobals.platform.pipelines[i]);
            _rpPatchGlobals.platform.pipelines[i] = (RxPipeline *)NULL;
        }
    }

    RWRETURN(TRUE);
}

/**
 * \ingroup rppatchd3d8
 * \ref RpPatchGetD3D8Pipeline returns a pointer to the \ref RxPipeline that has
 * been created for the specific type of patch renderer. \e d3d8Pipeline can
 * be one of \ref RpPatchD3D8Pipeline.
 *
 * The \ref rppatch plugin must be attached before using this function.
 *
 * \param d3d8Pipeline \ref RpPatchD3D8Pipeline
 *
 * \return Pointer to the \ref RxPipeline associated with the specified patch renderer.
 *
 * \see RpPatchAtomicSetType
 * \see RpPatchAtomicGetType
 */
RxPipeline *
RpPatchGetD3D8Pipeline( RpPatchD3D8Pipeline d3d8Pipeline )
{
    RWAPIFUNCTION(RWSTRING("RpPatchGetD3D8Pipeline"));
    RWASSERT(d3d8Pipeline < rpPATCHD3D8PIPELINEMAX);

    RWRETURN(_rpPatchGlobals.platform.pipelines[d3d8Pipeline]);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/
