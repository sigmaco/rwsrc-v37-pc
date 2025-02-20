#include <d3d8.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

//#include "rpmatfx.h"

#include "skin.h"

#include "skind3d8.h"
#include "rptoon.h"
#include "../toon/d3d8/d3d8toon.h"

#define NUMCLUSTERSOFINTEREST   0
#define NUMOUTPUTS              0

/****************************************************************************
 _rxSkinD3D8AtomicAllInOnePipelineInitToon
 */
static RwBool
_rxSkinD3D8AtomicAllInOnePipelineInitToon(RxPipelineNode *node)
{
    _rxD3D8SkinInstanceNodeData *instanceData;

    RWFUNCTION(RWSTRING("_rxSkinD3D8AtomicAllInOnePipelineInitToon"));

    instanceData = (_rxD3D8SkinInstanceNodeData *)node->privateData;

    /* we want all the skin instancing & animation stuff, but render it toony */
    instanceData->renderCallback = _rpToonD3D8RenderCallback;

    RWRETURN(TRUE);
}

/*
 * _rxNodeDefinitionGetD3D8SkinAtomicAllInOneToon returns a
 * pointer to the \ref RxNodeDefinition associated with
 * the Skin Atomic version of PowerPipe.
 */
static RxNodeDefinition *
_rxNodeDefinitionGetD3D8SkinAtomicAllInOneToon(void)
{
    static RwChar _SkinAtomicInstance_csl[] = "nodeD3D8SkinAtomicAllInOne.csl";

    static RxNodeDefinition nodeD3D8SkinAtomicAllInOneCSL = { /* */
        _SkinAtomicInstance_csl,                    /* Name */
        {                                           /* Nodemethods */
            _rwSkinD3D8AtomicAllInOneNode,          /* +-- nodebody */
            NULL,                                   /* +-- nodeinit */
            NULL,                                   /* +-- nodeterm */
            _rxSkinD3D8AtomicAllInOnePipelineInitToon,  /* +-- pipelinenodeinit */
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

    RWFUNCTION(RWSTRING("_rxNodeDefinitionGetD3D8SkinAtomicAllInOneToon"));

    RWRETURN(&nodeD3D8SkinAtomicAllInOneCSL);
}

RxPipeline *
_rpSkinD3D8CreateToonPipe()
{
    RxPipeline  *pipeline;

    RWFUNCTION(RWSTRING("_rpSkinD3D8CreateToonPipe"));

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
                _rxNodeDefinitionGetD3D8SkinAtomicAllInOneToon(),
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
_rwD3D8SkinToonNeedsAManagedVertexBuffer(const RpAtomic *atomic)
{
    RWFUNCTION(RWSTRING("_rwD3D8SkinToonNeedsAManagedVertexBuffer"));

    if (!_rpSkinGlobals.platform.hardwareTL ||
        !_rpSkinGlobals.platform.hardwareVS)
    {
        RWRETURN(TRUE);
    }
    else
    {
        if (atomic->pipeline != NULL &&
            atomic->pipeline->pluginData == rpSKINTYPETOON &&
            atomic->renderCallBack != RpD3D8ToonFastSilhouetteAtomicRenderCallback)
        {
            RWRETURN(TRUE);
        }
    }

    RWRETURN(FALSE);
}
