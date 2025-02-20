/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpskin.h"

#include "patch.h"

#include "noded3d9patchatomicinstance.h"
#include "noded3d9patchskinatomicinstance.h"

#include "patchd3d9.h"
#include "patchd3d9skin.h"

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
 _rxD3D9PatchPipelineNodeInitFn

 Initializes the private data (refinement ON by default)

 Inputs :
 Outputs :
 */
static RwBool
D3D9PatchPipelineNodeInitFn( RxPipelineNode *self )
{
    RWFUNCTION(RWSTRING("D3D9PatchPipelineNodeInitFn"));

    if (self)
    {
        RpNodePatchData *data = ((RpNodePatchData *) self->privateData);

        data->patchOn = TRUE;
        data->numExtraUVs = 0;
        data->numExtraRGBAs = 0;
        data->renderCallback = _rxD3D9DefaultRenderCallback;

        RWRETURN(TRUE);
    }

    RWRETURN(FALSE);
}

/*****************************************************************************
 NodeDefinitionGetD3D9PatchAtomicInstance

 Returns a pointer to a node to instance a patch atomic in a form suitable for
 rendering by a D3D9 pipeline.

 The node has one output. Successful generation of meshes from patches are passed
 via this output.

 The input requirements of this node:
    * RxClObjSpace3DVertices   - don't want
    * RxClIndices              - don't want
    * RxClMeshState            - don't want
    * RxClRenderState          - don't want
    * RxClD3D9VertexBufferInfo - don't want

 The characteristics of this node's outputs:
    * RxClObjSpace3DVertices   - valid
    * RxClIndices              - valid
    * RxClMeshState            - valid
    * RxClRenderState          - valid
    * RxClD3D9VertexBufferInfo - valid

 Returns a pointer to node for patch faceting custom pipelines on success,
 or NULL otherwise
*/
static RxNodeDefinition   *
NodeDefinitionGetD3D9PatchAtomicInstance(void)
{
    static RwChar _PatchAtomic_csl[] = RWSTRING("PatchAtomic.csl");

    static RxNodeDefinition nodeD3D9PatchAtomic = { /* */
        _PatchAtomic_csl,                           /* Name */
        {                                           /* Nodemethods */
            _rwD3D9PatchAtomicAllInOneNodeNode,     /* +-- nodebody */
            NULL,                                   /* +-- nodeinit */
            NULL,                                   /* +-- nodeterm */
            D3D9PatchPipelineNodeInitFn,            /* +-- pipelinenodeinit */
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
        (RxNodeDefEditable)FALSE,                   /* editable */
        0                                           /* inPipes */
    };

    RWFUNCTION(RWSTRING("NodeDefinitionGetD3D9PatchAtomicInstance"));

    RWRETURN(&nodeD3D9PatchAtomic);
}


/*****************************************************************************
 NodeDefinitionGetD3D9PatchSkinAtomicInstance

 Returns a pointer to a node to instance a skin patch atomic in a form suitable for
 rendering by a D3D9 pipeline.

 The node has one output. Successful generation of meshes from patches are passed
 via this output.

 The input requirements of this node:
    * RxClObjSpace3DVertices   - don't want
    * RxClIndices              - don't want
    * RxClMeshState            - don't want
    * RxClRenderState          - don't want
    * RxClD3D9VertexBufferInfo - don't want

 The characteristics of this node's outputs:
    * RxClObjSpace3DVertices   - valid
    * RxClIndices              - valid
    * RxClMeshState            - valid
    * RxClRenderState          - valid
    * RxClD3D9VertexBufferInfo - valid

 Returns a pointer to node for skin patch faceting custom pipelines on success,
 or NULL otherwise
*/
static RxNodeDefinition   *
NodeDefinitionGetD3D9PatchSkinAtomicInstance(void)
{
    static RwChar _PatchAtomic_csl[] = RWSTRING("PatchSkinAtomic.csl");

    static RxNodeDefinition nodeD3D9PatchAtomic = { /* */
        _PatchAtomic_csl,                           /* Name */
        {                                           /* Nodemethods */
            _rwD3D9PatchSkinAtomicAllInOneNodeNode, /* +-- nodebody */
            NULL,                                   /* +-- nodeinit */
            NULL,                                   /* +-- nodeterm */
            D3D9PatchPipelineNodeInitFn,            /* +-- pipelinenodeinit */
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
        (RxNodeDefEditable)FALSE,                   /* editable */
        0                                           /* inPipes */
    };

    RWFUNCTION(RWSTRING("NodeDefinitionGetD3D9PatchSkinAtomicInstance"));

    RWRETURN(&nodeD3D9PatchAtomic);
}

/*****************************************************************************
 _rwD3D9PatchPipelineCreateGeneric
 */
RxPipeline *
_rwD3D9PatchPipelineCreateGeneric(void)
{
    RxPipeline *pipe;
    RxLockedPipe *lpipe;

    RWFUNCTION(RWSTRING("_rwD3D9PatchPipelineCreateGeneric"));

    pipe = RxPipelineCreate();
    RWASSERT(NULL != pipe);

    pipe->pluginId = rwID_PATCHPLUGIN;
    pipe->pluginData = rpPATCHTYPEGENERIC;

    lpipe = RxPipelineLock(pipe);
    RWASSERT(NULL != lpipe);

    lpipe = RxLockedPipeAddFragment(lpipe,
        NULL,
        NodeDefinitionGetD3D9PatchAtomicInstance(),
        NULL);
    RWASSERT(NULL != lpipe);

    lpipe = RxLockedPipeUnlock(lpipe);

    RWASSERT(pipe == (RxPipeline *)lpipe);

    RWRETURN(pipe);
}

/*****************************************************************************
 _rwD3D9PatchPipelineCreateMatFX
 */
RxPipeline *
_rwD3D9PatchPipelineCreateMatFX(void)
{
    RWFUNCTION(RWSTRING("_rwD3D9PatchPipelineCreateMatFX"));

    RWRETURN(NULL);
}

/*****************************************************************************
 _rwD3D9PatchPipelineCreateSkin
 */
RxPipeline *
_rwD3D9PatchPipelineCreateSkin(void)
{
    RxPipeline *pipe;
    RxLockedPipe *lpipe;

    RWFUNCTION(RWSTRING("_rwD3D9PatchPipelineCreateSkin"));

    pipe = RxPipelineCreate();
    RWASSERT(NULL != pipe);

    pipe->pluginId = rwID_PATCHPLUGIN;
    pipe->pluginData = rpPATCHTYPESKIN;

    lpipe = RxPipelineLock(pipe);
    RWASSERT(NULL != lpipe);

    lpipe = RxLockedPipeAddFragment(lpipe,
        NULL,
        NodeDefinitionGetD3D9PatchSkinAtomicInstance(),
        NULL);
    RWASSERT(NULL != lpipe);

    lpipe = RxLockedPipeUnlock(lpipe);

    RWASSERT(pipe == (RxPipeline *)lpipe);

    RWRETURN(pipe);
}

/*****************************************************************************
 _rwD3D9PatchPipelineCreateSkinMatFX
 */
RxPipeline *
_rwD3D9PatchPipelineCreateSkinMatFX(void)
{
    RWFUNCTION(RWSTRING("_rwD3D9PatchPipelineCreateSkinMatFX"));

    RWRETURN(NULL);
}

/****************************************************************************
 _rpPatchPipelinesAttach

 Attach the correct patch pipeline to the atomic.

 Inputs :   atomic *   - Pointer to the atomic.
 Outputs:   RpAtomic * - Pointer to the atomic on success.
 */
RpAtomic *
_rpPatchPipelinesAttach(RpAtomic *atomic, RpPatchType type)
{
    RxPipeline  *pipeline = NULL;
    RpPatchMesh *patchMesh;
    RwBool      skinned;

    RWFUNCTION(RWSTRING("_rpPatchPipelinesAttach"));
    RWASSERT(NULL != atomic);

    /* Get the patch mesh. */
    patchMesh = RpPatchAtomicGetPatchMesh(atomic);
    RWASSERT(NULL != patchMesh);

    /* Get the skin. */
    skinned = (RpPatchMeshGetSkin(patchMesh) != NULL);

    /* Do we have a hint? */
    if(type == rpNAPATCHTYPE)
    {
        if (skinned)
        {
            pipeline = _rpPatchGlobals.platform.pipelines[rpPATCHD3D9PIPELINESKINNED];
        }
        else
        {
            pipeline = _rpPatchGlobals.platform.pipelines[rpPATCHD3D9PIPELINEGENERIC];
        }
    }
    else
    {
        /*Ok we've been given a hint. */
        switch(type)
        {
            case rpPATCHTYPEGENERIC:
            case rpPATCHTYPEMATFX:
                pipeline = _rpPatchGlobals.platform.pipelines[rpPATCHD3D9PIPELINEGENERIC];
                break;

            case rpPATCHTYPESKIN:
            case rpPATCHTYPESKINMATFX:
                pipeline = _rpPatchGlobals.platform.pipelines[rpPATCHD3D9PIPELINESKINNED];
                break;

            default:
                pipeline = _rpPatchGlobals.platform.pipelines[rpPATCHD3D9PIPELINEGENERIC];
                break;
        }
    }

    RWASSERT(NULL != pipeline);

    atomic = RpAtomicSetPipeline(atomic, pipeline);
    RWASSERT(NULL != atomic);

    RWRETURN(atomic);
}
