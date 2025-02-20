/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "skin.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

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

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rpskind3d9features
 * \page skinvertexshader RpSkin Vertex shaders
 *
 * The skin plugin uses vertex shaders when the video card supports them and
 * if the maximun number of bones used by the atomic fits in the available
 * number of vertex shader constants.
 *
 */

/**
 * \ingroup rpskind3d9features
 * \page skinassembler RpSkin Matrix-blending Code
 *
 * \note This section only applies if the RenderWare SDK has been
 * built with available assembler code. This is the default case.
 *
 * The matrix-blending code in the skin plugin has been changed in RWD3D9 to utilize x86
 * and SSE assembler in order to increase performance in the skinning calculations.
 *
 */

/**
 * \defgroup rpskinbonelimit Bone limit
 * \ingroup rpskind3d9restrictions
 *
 * \par Bone limit
 * \li If the skinning is performed on the CPU, the bone limit is 256.
 * \li If the skinning is performed using a vertex shader,
 * the bone limit depends on the available number of vertex shader constants
 * supported by the video card, and the number of lights on the scene affecting the atomic.
 *
 */

/**
 * \ingroup rpskind3d9
 * \ref RpSkinGetD3D9Pipeline
 *
 * Returns one of the \ref rpskin's internal platform specific pipelines.
 *
 * The \ref rpskin plugin must be attached before using this function.
 *
 * \param d3d9Pipeline Type of the requested pipeline.
 *
 * \return The \ref RxPipeline requested from the plugin, or
 * NULL if the pipeline wasn't constructed.
 *
 * \see RpSkinAtomicSetType
 * \see RpSkinAtomicGetType
 */
RxPipeline *
RpSkinGetD3D9Pipeline( RpSkinD3D9Pipeline d3d9Pipeline )
{
    RxPipeline *pipeline;

    RWAPIFUNCTION(RWSTRING("RpSkinGetD3D9Pipeline"));
    RWASSERT(0 < _rpSkinGlobals.module.numInstances);
    RWASSERT(rpNASKIND3D9PIPELINE < d3d9Pipeline);
    RWASSERT(rpSKIND3D9PIPELINEMAX > d3d9Pipeline);

    pipeline = _rpSkinPipeline(d3d9Pipeline);

    RWRETURN(pipeline);
}

/*
 ***************************************************************************
 */
void
_rxD3D9SkinVertexShaderSetBeginCallBack(RxPipelineNode *node,
                                _rxD3D9VertexShaderBeginCallBack beginCallback)
{
    _rxD3D9SkinInstanceNodeData *instanceData;

    RWAPIFUNCTION(RWSTRING("_rxD3D9SkinVertexShaderSetBeginCallBack"));

    instanceData = (_rxD3D9SkinInstanceNodeData *)node->privateData;

    /* Assert pipeline node private data pointer != NULL  */
    RWASSERT(NULL != instanceData);

    instanceData->vertexShaderNode.beginCallback = beginCallback;

    RWRETURNVOID();
}

/*
 ***************************************************************************
 */
void
_rxD3D9SkinVertexShaderSetLightingCallBack(RxPipelineNode *node,
                                _rxD3D9VertexShaderLightingCallBack lightingCallback)
{
    _rxD3D9SkinInstanceNodeData *instanceData;

    RWAPIFUNCTION(RWSTRING("_rxD3D9SkinVertexShaderSetBeginCallBack"));

    instanceData = (_rxD3D9SkinInstanceNodeData *)node->privateData;

    /* Assert pipeline node private data pointer != NULL  */
    RWASSERT(NULL != instanceData);

    instanceData->vertexShaderNode.lightingCallback = lightingCallback;

    RWRETURNVOID();
}

/*
 ***************************************************************************
 */
void
_rxD3D9SkinVertexShaderSetGetMaterialShaderCallBack(RxPipelineNode *node,
                                _rxD3D9VertexShaderGetMaterialShaderCallBack getmaterialshaderCallback)
{
    _rxD3D9SkinInstanceNodeData *instanceData;

    RWAPIFUNCTION(RWSTRING("_rxD3D9SkinVertexShaderSetBeginCallBack"));

    instanceData = (_rxD3D9SkinInstanceNodeData *)node->privateData;

    /* Assert pipeline node private data pointer != NULL  */
    RWASSERT(NULL != instanceData);

    instanceData->vertexShaderNode.getmaterialshaderCallback = getmaterialshaderCallback;

    RWRETURNVOID();
}

/*
 ***************************************************************************
 */
void
_rxD3D9SkinVertexShaderSetMeshRenderCallBack(RxPipelineNode *node,
                                _rxD3D9VertexShaderMeshRenderCallBack meshRenderCallback)
{
    _rxD3D9SkinInstanceNodeData *instanceData;

    RWAPIFUNCTION(RWSTRING("_rxD3D9SkinVertexShaderSetBeginCallBack"));

    instanceData = (_rxD3D9SkinInstanceNodeData *)node->privateData;

    /* Assert pipeline node private data pointer != NULL  */
    RWASSERT(NULL != instanceData);

    instanceData->vertexShaderNode.meshRenderCallback = meshRenderCallback;

    RWRETURNVOID();
}

/*
 ***************************************************************************
 */
void
_rxD3D9SkinVertexShaderSetEndCallBack(RxPipelineNode *node,
                                _rxD3D9VertexShaderEndCallBack endCallback)
{
    _rxD3D9SkinInstanceNodeData *instanceData;

    RWAPIFUNCTION(RWSTRING("_rxD3D9SkinVertexShaderSetBeginCallBack"));

    instanceData = (_rxD3D9SkinInstanceNodeData *)node->privateData;

    /* Assert pipeline node private data pointer != NULL  */
    RWASSERT(NULL != instanceData);

    instanceData->vertexShaderNode.endCallback = endCallback;

    RWRETURNVOID();
}
