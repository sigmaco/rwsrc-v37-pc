#include <d3d8.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpmatfx.h"

#include "skin.h"

#include "skind3d8.h"

#define NUMCLUSTERSOFINTEREST   0
#define NUMOUTPUTS              0

/****************************************************************************
 _rxSkinD3D8AtomicAllInOnePipelineInit
 */
static RwBool
_rxSkinD3D8AtomicAllInOnePipelineInit(RxPipelineNode *node)
{
    _rxD3D8SkinInstanceNodeData *instanceData;

    RWFUNCTION(RWSTRING("_rxSkinD3D8AtomicAllInOnePipelineInit"));

    instanceData = (_rxD3D8SkinInstanceNodeData *)node->privateData;

    instanceData->renderCallback = _rxD3D8DefaultRenderCallback;

    instanceData->lightingCallback = _rwD3D8AtomicDefaultLightingCallback;

    RWRETURN(TRUE);
}

/*
 * _rxNodeDefinitionGetD3D8SkinAtomicAllInOne returns a
 * pointer to the \ref RxNodeDefinition associated with
 * the Skin Atomic version of PowerPipe.
 */
static RxNodeDefinition *
_rxNodeDefinitionGetD3D8SkinAtomicAllInOne(void)
{
    static RwChar _SkinAtomicInstance_csl[] = "nodeD3D8SkinAtomicAllInOne.csl";

    static RxNodeDefinition nodeD3D8SkinAtomicAllInOneCSL = { /* */
        _SkinAtomicInstance_csl,                    /* Name */
        {                                           /* Nodemethods */
            _rwSkinD3D8AtomicAllInOneNode,          /* +-- nodebody */
            NULL,                                   /* +-- nodeinit */
            NULL,                                   /* +-- nodeterm */
            _rxSkinD3D8AtomicAllInOnePipelineInit,  /* +-- pipelinenodeinit */
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

    RWFUNCTION(RWSTRING("_rxNodeDefinitionGetD3D8SkinAtomicAllInOne"));

    RWRETURN(&nodeD3D8SkinAtomicAllInOneCSL);
}

RxPipeline *
_rpSkinD3D8CreatePlainPipe()
{
    const D3DCAPS8 *d3dCaps;
    RxPipeline  *pipeline;

    RWFUNCTION(RWSTRING("_rpSkinD3D8CreatePlainPipe"));

    d3dCaps = (const D3DCAPS8 *)RwD3D8GetCaps();

    _rpSkinGlobals.platform.hardwareTL = ((d3dCaps->DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0);

    if (_rpSkinGlobals.platform.hardwareTL &&
        (d3dCaps->VertexShaderVersion & 0xFFFF) >= 0x0101)
    {
        _rpSkinGlobals.platform.hardwareVS = TRUE;
    }
    else
    {
        _rpSkinGlobals.platform.hardwareVS = FALSE;
    }

    pipeline = RxPipelineCreate();

    if (pipeline)
    {
        RxLockedPipe    *lpipe;

        pipeline->pluginId = rwID_SKINPLUGIN;
        pipeline->pluginData = rpSKINTYPEGENERIC;

        if (NULL != (lpipe = RxPipelineLock(pipeline)))
        {
            lpipe = RxLockedPipeAddFragment(lpipe,
                NULL,
                _rxNodeDefinitionGetD3D8SkinAtomicAllInOne(),
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
