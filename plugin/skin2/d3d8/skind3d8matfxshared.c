#include <d3d8.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpmatfx.h"

#include "skin.h"

#include "skind3d8.h"
#include "skind3d8matfx.h"

#define NUMCLUSTERSOFINTEREST   0
#define NUMOUTPUTS              0

/****************************************************************************
 _rxSkinD3D8AtomicAllInOnePipelineInitMatFX
 */
static RwBool
_rxSkinD3D8AtomicAllInOnePipelineInitMatFX(RxPipelineNode *node)
{
    _rxD3D8SkinInstanceNodeData *instanceData;

    RWFUNCTION(RWSTRING("_rxSkinD3D8AtomicAllInOnePipelineInitMatFX"));

    instanceData = (_rxD3D8SkinInstanceNodeData *)node->privateData;

    instanceData->renderCallback = _rwD3D8AtomicMatFXRenderCallback;

    instanceData->lightingCallback = _rwD3D8AtomicDefaultLightingCallback;

    RWRETURN(TRUE);
}

/*
 * _rxNodeDefinitionGetD3D8SkinAtomicAllInOneMatFx returns a
 * pointer to the \ref RxNodeDefinition associated with
 * the Skin Atomic version of PowerPipe.
 */
static RxNodeDefinition *
_rxNodeDefinitionGetD3D8SkinAtomicAllInOneMatFx(void)
{
    static RwChar _SkinAtomicInstance_csl[] = "nodeD3D8SkinAtomicAllInOne.csl";

    static RxNodeDefinition nodeD3D8SkinAtomicAllInOneCSL = { /* */
        _SkinAtomicInstance_csl,                    /* Name */
        {                                           /* Nodemethods */
            _rwSkinD3D8AtomicAllInOneNode,          /* +-- nodebody */
            NULL,                                   /* +-- nodeinit */
            NULL,                                   /* +-- nodeterm */
            _rxSkinD3D8AtomicAllInOnePipelineInitMatFX,  /* +-- pipelinenodeinit */
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
        (RwUInt32)sizeof(_rxD3D8SkinInstanceNodeData),/* PipelineNodePrivateDataSize */
        (RxNodeDefEditable)FALSE,                   /* editable */
        0                                           /* inPipes */
    };

    RWFUNCTION(RWSTRING("_rxNodeDefinitionGetD3D8SkinAtomicAllInOneMatFx"));

    RWRETURN(&nodeD3D8SkinAtomicAllInOneCSL);
}

RxPipeline *
_rpSkinD3D8CreateMatFXPipe()
{
    RxPipeline  *pipeline;

    RWFUNCTION(RWSTRING("_rpSkinD3D8CreateMatFXPipe"));

    pipeline = RxPipelineCreate();

    if (pipeline)
    {
        RxLockedPipe    *lpipe;

        pipeline->pluginId = rwID_SKINPLUGIN;
        pipeline->pluginData = rpSKINTYPEMATFX;

        if (NULL != (lpipe = RxPipelineLock(pipeline)))
        {
            lpipe = RxLockedPipeAddFragment(lpipe,
                NULL,
                _rxNodeDefinitionGetD3D8SkinAtomicAllInOneMatFx(),
                NULL);

            lpipe = RxLockedPipeUnlock(lpipe);

            RWASSERT(pipeline == (RxPipeline *)lpipe);
            RWRETURN(pipeline);
        }
        else
        {
            RxPipelineDestroy(pipeline);
            pipeline = NULL;
        }
    }

    RWRETURN(pipeline);
}

RwBool
_rwD3D8SkinMatFXNeedsAManagedVertexBuffer(const RxD3D8ResEntryHeader *resEntryHeader)
{
    RWFUNCTION(RWSTRING("_rwD3D8SkinMatFXNeedsAManagedVertexBuffer"));

    if (!_rpSkinGlobals.platform.hardwareTL)
    {
        RWRETURN(TRUE);
    }
    else
    {
        if (resEntryHeader != NULL)
        {
            const RxD3D8InstanceData    *instancedData;
            RwUInt32                    numMeshes;

            /* Get the first instanced data structures */
            instancedData = (const RxD3D8InstanceData *)(resEntryHeader + 1);

            /* Get the number of meshes */
            numMeshes = resEntryHeader->numMeshes;

            do
            {
                if (instancedData->material != NULL &&
                    _rwD3D8MaterialMatFXHasBumpMap(instancedData->material))
                {
                    RWRETURN(TRUE);
                }

                instancedData++;
            }
            while (--numMeshes);
        }
    }

    RWRETURN(FALSE);
}
