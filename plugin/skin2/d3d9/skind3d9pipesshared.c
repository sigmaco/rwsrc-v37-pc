#include <d3d9.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpmatfx.h"

#include "skin.h"

#include "skind3d9.h"

#define NUMCLUSTERSOFINTEREST   0
#define NUMOUTPUTS              0

/****************************************************************************
 _rpD3D9SkinVertexShaderBeginCallBack
 */
void
_rpD3D9SkinVertexShaderBeginCallBack(void *object,
                                     RwUInt32 type,
                                     _rpD3D9VertexShaderDescriptor  *desc)
{
    RpGeometry *geometry;
    RwUInt32 geomFlags;
    RwBool   fogEnable;

    RWFUNCTION(RWSTRING("_rpD3D9SkinVertexShaderBeginCallBack"));
    RWASSERT(type == rpATOMIC);

    /* Initialize descriptor */
    memset(desc, 0, sizeof(_rpD3D9VertexShaderDescriptor));

    geometry = RpAtomicGetGeometry((RpAtomic *)object);

    geomFlags = RpGeometryGetFlags(geometry);

    desc->numTexCoords = RpGeometryGetNumTexCoordSets(geometry);

    if (geomFlags & rxGEOMETRY_PRELIT)
    {
        desc->prelit = 1;
    }

    if (geomFlags & rxGEOMETRY_NORMALS)
    {
        desc->normals  = 1;

        desc->normalizeNormals = 1;
    }

    if (geomFlags & rxGEOMETRY_MODULATE)
    {
        desc->modulateMaterial = 1;
    }

    RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fogEnable);
    if (fogEnable)
    {
        RwFogType   fogType;

        RwRenderStateGet(rwRENDERSTATEFOGTYPE, &fogType);

        switch (fogType)
        {
            case rwFOGTYPELINEAR:
                desc->fogMode = rwD3D9VERTEXSHADERFOG_LINEAR;
                break;

            case rwFOGTYPEEXPONENTIAL:
                desc->fogMode = rwD3D9VERTEXSHADERFOG_EXP;
                break;

            case rwFOGTYPEEXPONENTIAL2:
                desc->fogMode = rwD3D9VERTEXSHADERFOG_EXP2;
                break;
        }
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rxSkinD3D9AtomicAllInOnePipelineInit
 */
static RwBool
_rxSkinD3D9AtomicAllInOnePipelineInit(RxPipelineNode *node)
{
    _rxD3D9SkinInstanceNodeData *instanceData;

    RWFUNCTION(RWSTRING("_rxSkinD3D9AtomicAllInOnePipelineInit"));

    instanceData = (_rxD3D9SkinInstanceNodeData *)node->privateData;

    instanceData->renderCallback = _rxD3D9DefaultRenderCallback;
    instanceData->lightingCallback = _rwD3D9AtomicDefaultLightingCallback;

    if (_rpSkinGlobals.platform.hardwareVS)
    {
        instanceData->vertexShaderNode.beginCallback = _rpD3D9SkinVertexShaderBeginCallBack;
        instanceData->vertexShaderNode.lightingCallback = _rxD3D9VertexShaderDefaultLightingCallBack;
        instanceData->vertexShaderNode.getmaterialshaderCallback = _rxD3D9VertexShaderDefaultGetMaterialShaderCallBack;
        instanceData->vertexShaderNode.meshRenderCallback = _rxD3D9VertexShaderDefaultMeshRenderCallBack;
        instanceData->vertexShaderNode.endCallback = _rxD3D9VertexShaderDefaultEndCallBack;
    }
    else
    {
        instanceData->vertexShaderNode.beginCallback = NULL;
        instanceData->vertexShaderNode.lightingCallback = NULL;
        instanceData->vertexShaderNode.getmaterialshaderCallback = NULL;
        instanceData->vertexShaderNode.meshRenderCallback = NULL;
        instanceData->vertexShaderNode.endCallback = NULL;
    }

    RWRETURN(TRUE);
}

/*
 * RxNodeDefinitionGetD3D9SkinAtomicAllInOne returns a
 * pointer to the \ref RxNodeDefinition associated with
 * the Skin Atomic version of PowerPipe.
 */
RxNodeDefinition *
RxNodeDefinitionGetD3D9SkinAtomicAllInOne(void)
{
    static RwChar _SkinAtomicInstance_csl[] = RWSTRING("nodeD3D9SkinAtomicAllInOne.csl");

    static RxNodeDefinition nodeD3D9SkinAtomicAllInOneCSL = { /* */
        _SkinAtomicInstance_csl,                    /* Name */
        {                                           /* Nodemethods */
            _rwSkinD3D9AtomicAllInOneNode,          /* +-- nodebody */
            NULL,                                   /* +-- nodeinit */
            NULL,                                   /* +-- nodeterm */
            _rxSkinD3D9AtomicAllInOnePipelineInit,  /* +-- pipelinenodeinit */
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

    RWFUNCTION(RWSTRING("RxNodeDefinitionGetD3D9SkinAtomicAllInOne"));

    RWRETURN(&nodeD3D9SkinAtomicAllInOneCSL);
}

RxPipeline *
_rpSkinD3D9CreatePlainPipe()
{
    RxPipeline  *pipeline;
    const D3DCAPS9 *d3dCaps;

    RWFUNCTION(RWSTRING("_rpSkinD3D9CreatePlainPipe"));

    d3dCaps = (const D3DCAPS9 *)RwD3D9GetCaps();

    _rpSkinGlobals.platform.hardwareTL = (d3dCaps->DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0;

    if (_rpSkinGlobals.platform.hardwareTL &&
        (d3dCaps->VertexShaderVersion & 0xFFFF) >= 0x0101 &&
        (d3dCaps->PixelShaderVersion & 0xFFFF) >= 0x0101) /* We require pixel shaders to remove problems with crappy video cards */
    {
        _rpSkinGlobals.platform.hardwareVS = TRUE;

        _rpSkinGlobals.platform.maxNumConstants = d3dCaps->MaxVertexShaderConst;

        /* Safe aprox. number of constants needed for something useful */
        _rpSkinGlobals.platform.maxNumBones = (_rpSkinGlobals.platform.maxNumConstants - 12) / 3;
    }
    else
    {
        _rpSkinGlobals.platform.hardwareVS = FALSE;
        _rpSkinGlobals.platform.maxNumBones = 0;
        _rpSkinGlobals.platform.maxNumConstants = 0;
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
                RxNodeDefinitionGetD3D9SkinAtomicAllInOne(),
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
