/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "patch.h"

#include "noded3d8patchatomicinstance.h"

#include "patchd3d8.h"
#include "patchd3d8plain.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/

#define NUMCLUSTERSOFINTEREST 0
#define NUMOUTPUTS 0

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
/*****************************************************************************
 _rxD3D8PatchPipelineNodeInitFn

 Initializes the private data (refinement ON by default)

 Inputs :
 Outputs :
 */
static RwBool
D3D8PatchPipelineNodeInitFn( RxPipelineNode *self )
{
    RWFUNCTION(RWSTRING("D3D8PatchPipelineNodeInitFn"));

    if (self)
    {
        RpNodePatchData *data = ((RpNodePatchData *) self->privateData);

        data->patchOn = TRUE;
        data->numExtraUVs = 0;
        data->numExtraRGBAs = 0;
        data->renderCallback = _rxD3D8DefaultRenderCallback;

        RWRETURN(TRUE);
    }

    RWRETURN(FALSE);
}

/*****************************************************************************
 NodeDefinitionGetD3D8PatchAtomicInstance

 Returns a pointer to a node to instance a patch atomic in a form suitable for
 rendering by a D3D8 pipeline.

 The node has one output. Successful generation of meshes from patches are passed
 via this output.

 The input requirements of this node:
    * RxClObjSpace3DVertices   - don't want
    * RxClIndices              - don't want
    * RxClMeshState            - don't want
    * RxClRenderState          - don't want
    * RxClD3D8VertexBufferInfo - don't want

 The characteristics of this node's outputs:
    * RxClObjSpace3DVertices   - valid
    * RxClIndices              - valid
    * RxClMeshState            - valid
    * RxClRenderState          - valid
    * RxClD3D8VertexBufferInfo - valid

 Returns a pointer to node for patch faceting custom pipelines on success,
 or NULL otherwise
*/
static RxNodeDefinition   *
NodeDefinitionGetD3D8PatchAtomicInstance(void)
{
    static RwChar _PatchAtomic_csl[] = "PatchAtomic.csl";

    static RxNodeDefinition nodeD3D8PatchAtomic = { /* */
        _PatchAtomic_csl,                           /* Name */
        {                                           /* Nodemethods */
            _rwD3D8PatchAtomicAllInOneNodeNode,     /* +-- nodebody */
            NULL,                                   /* +-- nodeinit */
            NULL,                                   /* +-- nodeterm */
            D3D8PatchPipelineNodeInitFn,            /* +-- pipelinenodeinit */
            NULL,                                   /* +-- pipelineNodeTerm */
            NULL,                                   /* +-- pipelineNodeConfig */
            NULL,                                   /* +-- configMsgHandler */
        },
        {                                           /* Io */
            NUMCLUSTERSOFINTEREST,                  /* +-- NumClustersOfInterest */
            NULL,                                   /* +-- ClustersOfInterest */
            NULL,                                   /* +-- InputRequirements */
            NUMOUTPUTS,                             /* +-- NumOutputs */
            NULL                                    /* +-- Outputs */
        },
        (RwUInt32)sizeof(RpNodePatchData),          /* PipelineNodePrivateDataSize */
        (RxNodeDefEditable)FALSE,                                      /* editable */
        0                                           /* inPipes */
    };

    RWFUNCTION(RWSTRING("NodeDefinitionGetD3D8PatchAtomicInstance"));

    RWRETURN(&nodeD3D8PatchAtomic);
}

/*****************************************************************************
 _rwD3D8PatchPipelineCreateGeneric
 */
RxPipeline *
_rwD3D8PatchPipelineCreateGeneric(void)
{
    RxPipeline *pipe;
    RxLockedPipe *lpipe;

    RWFUNCTION(RWSTRING("_rwD3D8PatchPipelineCreateGeneric"));

    pipe = RxPipelineCreate();
    RWASSERT(NULL != pipe);

    pipe->pluginId = rwID_PATCHPLUGIN;
    pipe->pluginData = rpPATCHTYPEGENERIC;

    lpipe = RxPipelineLock(pipe);
    RWASSERT(NULL != lpipe);

    lpipe = RxLockedPipeAddFragment(lpipe,
        NULL,
        NodeDefinitionGetD3D8PatchAtomicInstance(),
        NULL);
    RWASSERT(NULL != lpipe);

    lpipe = RxLockedPipeUnlock(lpipe);

    RWASSERT(pipe == (RxPipeline *)lpipe);

    RWRETURN(pipe);
}

/*****************************************************************************
 _rwD3D8PatchPipelineCreateMatFX
 */
RxPipeline *
_rwD3D8PatchPipelineCreateMatFX(void)
{
    RWFUNCTION(RWSTRING("_rwD3D8PatchPipelineCreateMatFX"));

    RWRETURN(NULL);
}

/*****************************************************************************
 _rwD3D8PatchPipelineCreateSkin
 */
RxPipeline *
_rwD3D8PatchPipelineCreateSkin(void)
{
    RWFUNCTION(RWSTRING("_rwD3D8PatchPipelineCreateSkin"));

    RWRETURN(NULL);
}

/*****************************************************************************
 _rwD3D8PatchPipelineCreateSkinMatFX
 */
RxPipeline *
_rwD3D8PatchPipelineCreateSkinMatFX(void)
{
    RWFUNCTION(RWSTRING("_rwD3D8PatchPipelineCreateSkinMatFX"));

    RWRETURN(NULL);
}

/****************************************************************************
 _rpPatchPipelinesAttach

 Attach the correct patch pipeline to the atomic.

 Inputs :   atomic *   - Pointer to the atomic.
 Outputs:   RpAtomic * - Pointer to the atomic on success.
 */
RpAtomic *
_rpPatchPipelinesAttach(RpAtomic *atomic,
                        RpPatchType type __RWUNUSED__)
{
    RxPipeline *pipeline;

    RWFUNCTION(RWSTRING("_rpPatchPipelinesAttach"));
    RWASSERT(NULL != atomic);

    pipeline = _rpPatchGlobals.platform.pipelines[rpPATCHD3D8PIPELINEGENERIC];
    RWASSERT(NULL != pipeline);

    atomic = RpAtomicSetPipeline(atomic, pipeline);
    RWASSERT(NULL != atomic);

    RWRETURN(atomic);
}
