#include <d3d9.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

//#include "rpmatfx.h"

#include "skin.h"

#include "skind3d9.h"
#include "rptoon.h"
#include "../toon/d3d9/d3d9toon.h"

#define NUMCLUSTERSOFINTEREST   0
#define NUMOUTPUTS              0


extern void
_rpD3D9SkinVertexShaderBeginCallBack(void *object,
                                     RwUInt32 type,
                                     _rpD3D9VertexShaderDescriptor  *desc);

/****************************************************************************
 _rxSkinD3D9AtomicAllInOnePipelineInitToon
 */
static RwBool
_rxSkinD3D9AtomicAllInOnePipelineInitToon(RxPipelineNode *node)
{
    _rxD3D9SkinInstanceNodeData *instanceData;

    RWFUNCTION(RWSTRING("_rxSkinD3D9AtomicAllInOnePipelineInitToon"));

    instanceData = (_rxD3D9SkinInstanceNodeData *)node->privateData;

    /* we want all the skin instancing & animation stuff, but render it toony */
    instanceData->renderCallback = NULL;
	//instanceData->renderCallback = _rpToonD3D9RenderCallback;

    instanceData->vertexShaderNode.beginCallback = NULL;
    instanceData->vertexShaderNode.lightingCallback = NULL;
    instanceData->vertexShaderNode.getmaterialshaderCallback = NULL;
    instanceData->vertexShaderNode.meshRenderCallback = NULL;
    instanceData->vertexShaderNode.endCallback = NULL;

    RWRETURN(TRUE);
}

/*
 * _rxNodeDefinitionGetD3D9SkinAtomicAllInOneToon returns a
 * pointer to the \ref RxNodeDefinition associated with
 * the Skin Atomic version of PowerPipe.
 */
static RxNodeDefinition *
_rxNodeDefinitionGetD3D9SkinAtomicAllInOneToon(void)
{
    static RwChar _SkinAtomicInstance_csl[] = RWSTRING("nodeD3D9SkinAtomicAllInOne.csl");

    static RxNodeDefinition nodeD3D9SkinAtomicAllInOneCSL = { /* */
        _SkinAtomicInstance_csl,                    /* Name */
        {                                           /* Nodemethods */
            _rwSkinD3D9AtomicAllInOneNode,          /* +-- nodebody */
            NULL,                                   /* +-- nodeinit */
            NULL,                                   /* +-- nodeterm */
            _rxSkinD3D9AtomicAllInOnePipelineInitToon,  /* +-- pipelinenodeinit */
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
        (RwUInt32)sizeof(_rxD3D9SkinInstanceNodeData),/* PipelineNodePrivateDataSize */
        (RxNodeDefEditable)FALSE,                   /* editable */
        0                                           /* inPipes */
    };

    RWFUNCTION(RWSTRING("_rxNodeDefinitionGetD3D9SkinAtomicAllInOneToon"));

    RWRETURN(&nodeD3D9SkinAtomicAllInOneCSL);
}

RxPipeline *
_rpSkinD3D9CreateToonPipe()
{
    RxPipeline  *pipeline;

    RWFUNCTION(RWSTRING("_rpSkinD3D9CreateToonPipe"));

    pipeline = RxPipelineCreate();

    if (pipeline)
    {
        RxLockedPipe    *lpipe;

        pipeline->pluginId = rwID_SKINPLUGIN;
        pipeline->pluginData = rpSKINTYPETOON;

        if (NULL != (lpipe = RxPipelineLock(pipeline)))
        {
            lpipe = RxLockedPipeAddFragment(lpipe,
                NULL,
                _rxNodeDefinitionGetD3D9SkinAtomicAllInOneToon(),
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
_rwD3D9SkinToonNeedsAManagedVertexBuffer(const RpAtomic *atomic)
{
    RWFUNCTION(RWSTRING("_rwD3D9SkinToonNeedsAManagedVertexBuffer"));

    if (!_rpSkinGlobals.platform.hardwareTL ||
        !_rpSkinGlobals.platform.hardwareVS)
    {
        RWRETURN(TRUE);
    }
    else
    {
        if (atomic->pipeline != NULL &&
            atomic->pipeline->pluginData == rpSKINTYPETOON &&
            atomic->renderCallBack != RpD3D9ToonFastSilhouetteAtomicRenderCallback)
        {
            RWRETURN(TRUE);
        }
    }

    RWRETURN(FALSE);
}
