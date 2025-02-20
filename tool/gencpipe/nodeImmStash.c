/*
 * nodeImmStash
 *
 * Im3D "stash" node
 *
 * Copyright (c) Criterion Software Limited
 */

/****************************************************************************
 *                                                                          *
 * module : nodeImmStash.c                                                  *
 *                                                                          *
 * purpose: yawn...                                                         *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 includes
 */

#include <string.h>            /* needed for memset */

#include "rwcore.h"

#include "rpdbgerr.h"

#include "rtgncpip.h"

/* *INDENT-OFF* */

/****************************************************************************
 PL2ImmStashNodeBody()
 */

static RwBool
PL2ImmStashNodeBody(RxPipelineNodeInstance * self __RWUNUSEDRELEASE__,
                    const RxPipelineNodeParam * params)
{
    rwIm3DPool         *immPool;
    _rwIm3DPoolStash   *stash;
    RxPacket           *pk;
    RxCluster          *clObjVerts;
    RxCluster          *clCamVerts;
    RxCluster          *clDevVerts;
    RxCluster          *clMeshState;
    RxCluster          *clRenderState;

    RWFUNCTION(RWSTRING("PL2ImmStashNodeBody"));

    immPool = (rwIm3DPool *) RxPipelineNodeParamGetData(params);
    RWASSERT(immPool != NULL);

    stash = &immPool->stash;
    RWASSERT(NULL != stash);

    pk = (RxPacket *)RxPacketFetch(self);
    RWASSERT(NULL != pk);

    memset(stash, 0x00U, sizeof(_rwIm3DPoolStash));

    clObjVerts = RxClusterLockRead(pk, 0);
    RWASSERT(NULL != clObjVerts);
    stash->objVerts =
        RxClusterGetCursorData(clObjVerts, RxObjSpace3DVertex);
    /* prevent freeing of memory when packet is destroyed! */
    clObjVerts->flags |= rxCLFLAGS_EXTERNAL;

    clCamVerts = RxClusterLockRead(pk, 1);
    RWASSERT(NULL != clCamVerts);
    stash->camVerts =
        RxClusterGetCursorData(clCamVerts, RxCamSpace3DVertex);
    clCamVerts->flags |= rxCLFLAGS_EXTERNAL;

    clDevVerts = RxClusterLockRead(pk, 2);
    RWASSERT(NULL != clDevVerts);
    stash->devVerts =
        RxClusterGetCursorData(clDevVerts, RxScrSpace2DVertex);
    clDevVerts->flags |= rxCLFLAGS_EXTERNAL;

    clMeshState = RxClusterLockRead(pk, 3);
    RWASSERT(NULL != clMeshState);
    stash->meshState =
        RxClusterGetCursorData(clMeshState, RxMeshStateVector);
    stash->numVerts = stash->meshState->NumVertices;
    clMeshState->flags |= rxCLFLAGS_EXTERNAL;

    clRenderState = RxClusterLockRead(pk, 4);
    RWASSERT(NULL != clRenderState);
    stash->renderState =
        RxClusterGetCursorData(clRenderState, RxRenderStateVector);
    clRenderState->flags |= rxCLFLAGS_EXTERNAL;

    RxPacketDestroy(pk, self);

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionGetImmStash returns a pointer to a
 * node to store the results of the current Im3D transform pipeline
 * for use in Im3D render pipelines.
 *
 * The contents of the incoming packet (all the clusters listed below)
 * are 'stashed' in global state such that packets can be reconstructed
 * in subsequent Im3D render pipelines by the
 * \ref RxNodeDefinitionGetImmRenderSetup node. This state is a
 * _rwIm3DPoolStash struct which contains the primitive type, rwIM3D
 * flags, an (optional) \ref RwMatrix, the number of indices and vertices,
 * pointers to indices, object-space vertices, optionally (not on
 * platforms utilising hardware transformation) camera-space and
 * screen-space vertices, an \ref RxMeshStateVector and optionally an
 * \ref RxRenderStateVector.
 *
 * This node has no outputs. Incoming packets are destroyed after
 * their contents have been stashed.
 *
 * The input requirements of this node:
 \verbatim
   RxClObjSpace3DVertices - optional
   RxClCamSpace3DVertices - optional
   RxClScrSpace2DVertices - optional
   RxClMeshState          - optional
   RxClRenderState        - optional \endverbatim
 *
 * \return A pointer to a node to stash a packet's contents
 *
 * \see RxNodeDefinitionGetImmInstance
 * \see RxNodeDefinitionGetImmMangleLineIndices
 * \see RxNodeDefinitionGetImmMangleTriangleIndices
 * \see RxNodeDefinitionGetImmRenderSetup
 */

RxNodeDefinition   *
RxNodeDefinitionGetImmStash(void)
{
    /***************************************/
    /**                                   **/
    /**  IMMSTASH.CSL NODE SPECIFICATION  **/
    /**                                   **/
    /***************************************/

    static RxClusterRef nodeClusters[] = {
        {&RxClObjSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClCamSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClScrSpace2DVertices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClRenderState, rxCLALLOWABSENT, rxCLRESERVED}
    };

#define NUMCLUSTERSOFINTEREST \
        ((sizeof(nodeClusters))/(sizeof(nodeClusters[0])))

    /* input requirements (this array parallel to ClusterRefs) */
    static RxClusterValidityReq nodeReqs[NUMCLUSTERSOFINTEREST] = {
        rxCLREQ_OPTIONAL,
        rxCLREQ_OPTIONAL,
        rxCLREQ_OPTIONAL,
        rxCLREQ_OPTIONAL,
        rxCLREQ_OPTIONAL
    };

    static RwChar       _ImmStash_csl[] = RWSTRING("ImmStash.csl");

    static RxNodeDefinition nodeImmStashCSL = {
        _ImmStash_csl,          /* Name */
        {                      /* nodemethods */
         PL2ImmStashNodeBody,  /* +-- nodebody */
         (RxNodeInitFn) NULL,  /* +-- nodeinit */
         (RxNodeTermFn) NULL,  /* +-- nodeterm */
         (RxPipelineNodeInitFn) NULL, /* +-- pipelinenodeinit */
         (RxPipelineNodeTermFn) NULL, /* +-- pipelineNodeTerm */
         (RxPipelineNodeConfigFn) NULL, /* +--  pipelineNodeConfig */
         (RxConfigMsgHandlerFn) NULL /* +--  configMsgHandler */
         },
        {                      /* Io */
         NUMCLUSTERSOFINTEREST, /* +-- NumClustersOfInterest */
         nodeClusters,         /* +-- ClustersOfInterest */
         nodeReqs,             /* +-- InputRequirements */
         0,                    /* +-- NumOutputs */
         (RxOutputSpec *)NULL  /* +-- Outputs */
         },
        (RwUInt32) 0,          /* PipelineNodePrivateDataSize */
        (RxNodeDefEditable)FALSE,  /* editable */
        (RwInt32) 0            /* InputPipesCnt */
    };

    /***************************************/

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionGetImmStash"));

    RWRETURN(&nodeImmStashCSL);
}
