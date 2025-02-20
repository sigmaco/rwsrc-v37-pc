/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpmatfx.h"

#include "patch.h"
#include "patchmesh.h"
#include "patchmatfx.h"

#include "noded3d9patchatomicinstance.h"

#include "patchd3d9.h"
#include "patchd3d9matfx.h"

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
 _rxD3D9PatchMatFXPipelineNodeInitFn

 Initializes the private data (refinement ON by default)

 Inputs :
 Outputs :
 */
static RwBool
D3D9PatchMatFXPipelineNodeInitFn( RxPipelineNode *self )
{
    RWFUNCTION(RWSTRING("D3D9PatchMatFXPipelineNodeInitFn"));

    if (self)
    {
        RpNodePatchData *data = ((RpNodePatchData *) self->privateData);

        data->patchOn = TRUE;
        data->numExtraUVs = 0;
        data->numExtraRGBAs = 0;

        /*
         * Use matfx render callback
         */
        data->renderCallback = _rwD3D9AtomicMatFXRenderCallback;

        RWRETURN(TRUE);
    }

    RWRETURN(FALSE);
}


/*****************************************************************************
 NodeDefinitionGetD3D9PatchMatFXAtomicInstance

 Returns a pointer to a node to instance a matfx patch atomic in a form suitable for
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

 Returns a pointer to node for matfx patch faceting custom pipelines on success,
 or NULL otherwise
*/
static RxNodeDefinition   *
NodeDefinitionGetD3D9PatchMatFXAtomicInstance(void)
{
    static RwChar _PatchAtomic_csl[] = RWSTRING("PatchMatFXAtomic.csl");

    static RxNodeDefinition nodeD3D9PatchAtomic = { /* */
        _PatchAtomic_csl,                           /* Name */
        {                                           /* Nodemethods */
            _rwD3D9PatchAtomicAllInOneNodeNode,     /* +-- nodebody */
            NULL,                                   /* +-- nodeinit */
            NULL,                                   /* +-- nodeterm */
            D3D9PatchMatFXPipelineNodeInitFn,       /* +-- pipelinenodeinit */
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

    RWFUNCTION(RWSTRING("NodeDefinitionGetD3D9PatchMatFXAtomicInstance"));

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
    RxPipeline *pipe;
    RxLockedPipe *lpipe;

    RWFUNCTION(RWSTRING("_rwD3D9PatchPipelineCreateMatFX"));

    pipe = RxPipelineCreate();
    RWASSERT(NULL != pipe);

    pipe->pluginId = rwID_PATCHPLUGIN;
    pipe->pluginData = rpPATCHTYPEMATFX;

    lpipe = RxPipelineLock(pipe);
    RWASSERT(NULL != lpipe);

    lpipe = RxLockedPipeAddFragment(lpipe,
        NULL,
        NodeDefinitionGetD3D9PatchMatFXAtomicInstance(),
        NULL);
    RWASSERT(NULL != lpipe);

    lpipe = RxLockedPipeUnlock(lpipe);

    RWASSERT(pipe == (RxPipeline *)lpipe);

    RWRETURN(pipe);
}

/*****************************************************************************
 _rwD3D9PatchPipelineCreateSkin
 */
RxPipeline *
_rwD3D9PatchPipelineCreateSkin(void)
{
    RWFUNCTION(RWSTRING("_rwD3D9PatchPipelineCreateSkin"));

    RWRETURN(NULL);
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
    RwBool      matfx;

    RWFUNCTION(RWSTRING("_rpPatchPipelinesAttach"));
    RWASSERT(NULL != atomic);

    /* Get the patch mesh. */
    patchMesh = RpPatchAtomicGetPatchMesh(atomic);
    RWASSERT(NULL != patchMesh);

    /* Is the patch mesh matfxed. */
    matfx = (_rpPatchMeshSearchForEffect(patchMesh) != rpMATFXEFFECTNULL);

    /* Do we have a hint? */
    if(type == rpNAPATCHTYPE)
    {
        if (matfx)
        {
            pipeline = _rpPatchGlobals.platform.pipelines[rpPATCHD3D9PIPELINEMATFX];
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
            case rpPATCHTYPESKIN:
                pipeline = _rpPatchGlobals.platform.pipelines[rpPATCHD3D9PIPELINEGENERIC];
                break;

            case rpPATCHTYPEMATFX:
            case rpPATCHTYPESKINMATFX:
                pipeline = _rpPatchGlobals.platform.pipelines[rpPATCHD3D9PIPELINEMATFX];
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
