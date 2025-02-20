/*
 * nodeMaterialScatter
 * Distributing packets to material pipelines
 *
 * Copyright (c) Criterion Software Limited
 */
/****************************************************************************
 *                                                                          *
 * module : nodeMaterialScatter.c                                           *
 *                                                                          *
 * purpose: distributes packets to material pipelines on the basis of       *
 *          the pipeline pointer in their MeshState cluster                 *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 includes
 */

/* #include <assert.h> */
#include <float.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rwcore.h"
#include "rpworld.h"

#include "rpdbgerr.h"

#include "p2stdclsw.h"

#include "rtgncpip.h"

/****************************************************************************
 local defines
 */

#if (defined(__VECTORC__) && !(defined(RWDEBUG) || defined(RWSUPPRESSINLINE)))
#define  MATERIALGETDEFAULTPIPELINE()                                    \
    (((rwPipeGlobals *)                                                  \
     (((RwUInt8 *)(RwEngineInstance)) +                                  \
      (_rxPipelineGlobalsOffset)))->currentMaterialPipeline)
#endif /* (defined(__VECTORC__) .... */

#if (!defined(MATERIALGETDEFAULTPIPELINE))
#define  MATERIALGETDEFAULTPIPELINE()           \
    (RpMaterialGetDefaultPipeline())
#endif /* (!defined(MATERIALGETDEFAULTPIPELINE)) */

#define MESSAGE(_string)                                              \
    RwDebugSendMessage(rwDEBUGMESSAGE, "MaterialScatter.csl", _string)



/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/*****************************************************************************
 MaterialScatterNode

 Scatters packets to the pipeline specified by their associated Material.
*/

static              RwBool
_MaterialScatterNode(RxPipelineNodeInstance * self __RWUNUSEDRELEASE__,
                     const RxPipelineNodeParam * params __RWUNUSED__ )
{
    RxPacket          *packet;
    RxPipeline        *pipeline;
    RxCluster         *meshState;
    RxMeshStateVector *meshData;

    RWFUNCTION(RWSTRING("_MaterialScatterNode"));

    RWASSERT(NULL != self);

    packet = (RxPacket *)RxPacketFetch(self);
    RWASSERT(NULL != packet);

    meshState = RxClusterLockRead(packet, 0);
    RWASSERT((meshState != NULL) && (meshState->numUsed > 0));

    meshData = RxClusterGetCursorData(meshState, RxMeshStateVector);

    /* Output this one to the appropriate Material pipeline */

    pipeline = meshData->Pipeline;
    if (!pipeline)
    {
        pipeline = MATERIALGETDEFAULTPIPELINE();
    }

    RWASSERT(NULL != pipeline);
    RxPacketDispatchToPipeline(packet, pipeline, self);


    RWRETURN(TRUE);
}

/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionGetMaterialScatter returns a pointer to a node
 * to distribute packets to material pipelines on the basis of the
 * pipeline pointer in their MeshState cluster.
 *
 * This node requires as optional many standard clusters such that, if they
 * are present in the pipeline, they will propagate from this pipeline to
 * the material pipeline. For any other clusters which you want to
 * propagate to the end of an object pipeline and then to material
 * pipelines, use RxPipelineNodeRequestCluster during pipeline construction
 * (before unlocking the pipeline) to change the requirements of the terminal
 * MaterialScatter.csl RxPipelineNode.
 *
 * If the mesh state of a packet has its material pipeline pointer set to
 * NULL then the packet will be sent to the default material pipeline, which
 * is accessible through RxMaterialGetDefaultPipeline.
 *
 * The node has no outputs, all packets pass from it to other pipelines.
 * The input requirements of this node:
 *      \li RxClMeshState          - required
 *      \li RxClObjSpace3DVertices - optional
 *      \li RxClIndices            - optional
 *      \li RxClRenderState        - optional
 *      \li RxClLights             - optional
 *
 * \return pointer to a node to distribute packets to material pipelines
 * on the basis of the pipeline pointer in their MeshState cluster
 *
 * \see RxNodeDefinitionGetAtomicEnumerateLights
 * \see RxNodeDefinitionGetAtomicInstance
 * \see RxNodeDefinitionGetFastPathSplitter
 * \see RxNodeDefinitionGetLight
 * \see RxNodeDefinitionGetPostLight
 * \see RxNodeDefinitionGetPreLight
 * \see RxNodeDefinitionGetWorldSectorEnumerateLights
 * \see RxNodeDefinitionGetWorldSectorInstance
 */
RxNodeDefinition *
RxNodeDefinitionGetMaterialScatter(void)
{
    static RxClusterRef N2clofinterest[] = {
      {&RxClMeshState,              rxCLALLOWABSENT, rxCLRESERVED},
      {&RxClObjSpace3DVertices,     rxCLALLOWABSENT, rxCLRESERVED},
      {&RxClIndices,                rxCLALLOWABSENT, rxCLRESERVED},
      {&RxClRenderState,            rxCLALLOWABSENT, rxCLRESERVED},
      {&RxClLights,                 rxCLALLOWABSENT, rxCLRESERVED} };

    #define NUMCLUSTERSOFINTEREST \
        ((sizeof(N2clofinterest))/(sizeof(N2clofinterest[0])))

    static RxClusterValidityReq N2inputreqs[NUMCLUSTERSOFINTEREST] = {
      rxCLREQ_REQUIRED,
      rxCLREQ_OPTIONAL,
      rxCLREQ_OPTIONAL,
      rxCLREQ_OPTIONAL,
      rxCLREQ_OPTIONAL };

    #define NUMOUTPUTS 0

    static RwChar _MaterialScatter_csl[] = RWSTRING("MaterialScatter.csl");

    static RxNodeDefinition nodeMaterialScatterCSL = {
      _MaterialScatter_csl,
      {_MaterialScatterNode,
       (RxNodeInitFn)NULL,
       (RxNodeTermFn)NULL,
       (RxPipelineNodeInitFn)NULL,
       (RxPipelineNodeTermFn)NULL,
       (RxPipelineNodeConfigFn)NULL,
       (RxConfigMsgHandlerFn)NULL },
      {NUMCLUSTERSOFINTEREST,
       N2clofinterest,
       N2inputreqs,
       NUMOUTPUTS,
       (RxOutputSpec *)NULL},
      0,
      (RxNodeDefEditable)FALSE,
      0 };

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionGetMaterialScatter"));

    /*RWMESSAGE((RWSTRING("Pipeline II node")));*/

    RWRETURN(&nodeMaterialScatterCSL);
}

