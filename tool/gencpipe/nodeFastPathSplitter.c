/*
 * nodeFastPathSplitter
 * Routing world geometry down fast paths where possible  |
 *
 * Copyright (c) Criterion Software Limited
 */
/****************************************************************************
 *                                                                          *
 * module : nodeFastPathSplitter.c                                          *
 *                                                                          *
 * purpose: yawn...                                                         *
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

#define MESSAGE(string)                                                \
    RwDebugSendMessage(rwDEBUGMESSAGE, "FastPathSplitter.csl", string)

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/****************************************************************************
 FastPathSplitterNodeFn

 -

 on entry: -
 on exit : -
*/

static              RwBool
FastPathSplitterNodeFn(RxPipelineNodeInstance * self,
                       const RxPipelineNodeParam * params)
{
    RxPacket             *packet;
    RpWorldSector        *worldSector;
    const RwFrustumPlane *plpPlanes;
    RwInt32               i;

    RWFUNCTION(RWSTRING("FastPathSplitterNodeFn"));

    RWASSERT(NULL != self);

    packet = (RxPacket *)RxPacketFetch(self);
    RWASSERT(NULL != packet);

    RWASSERT(NULL != params);
    RWASSERT(*(RwUInt8 *)RxPipelineNodeParamGetData(params) ==
             (RwUInt8) rwSECTORATOMIC);

    worldSector = (RpWorldSector *)RxPipelineNodeParamGetData(params);
    plpPlanes = ((RwCamera *)RWSRCGLOBAL(curCamera))->frustumPlanes;

/* TODO: This should use the mesh vector's clipflags, then it can work
 *       for atomics AND sectors - note it has to come after nodeTransform */

    /* Test to see if all corners of the sector's bounding box are
     * inside the frustum; if so then all geom is fully inside. */
    for (i = 0; i < 6; i++)
    {
        RwV3d vCorner;

        vCorner.x = ((RwV3d *)&(worldSector->tightBoundingBox))
                        [plpPlanes[i].closestX ^ 1].x;
        vCorner.y = ((RwV3d *)&(worldSector->tightBoundingBox))
                        [plpPlanes[i].closestY ^ 1].y;
        vCorner.z = ((RwV3d *)&(worldSector->tightBoundingBox))
                        [plpPlanes[i].closestZ ^ 1].z;

        if (RwV3dDotProduct(&vCorner, &plpPlanes[i].plane.normal) >
            plpPlanes[i].plane.distance)
        {
            break;
        }
    }

    if (i == 6)
    {
        RxPacketDispatch(packet, 1, self);
    }

    RxPacketDispatch(packet, 0, self);

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionGetFastPathSplitter returns a pointer to a
 * node to route packets down a fast path in the current pipeline,
 * where the current RpWorldSector's bounding box lies fully within
 * the view frustum and hence need not be processed by a clipping node.
 *
 * The node has two outputs. Packets are sent through the second
 * output if all vertices lie within the view frustum and hence
 * the clipping stage of the pipeline can be skipped.
 * The input requirements of this node:
 *      \li RxClMeshState          - required
 *
 * The characteristics of this node's first output:
 *      \li RxClMeshState          - no change
 *
 * The characteristics of this node's second output:
 *      \li RxClMeshState          - no change
 *
 * \return pointer to a node to route geometry down fast paths where possible
 * - for example, geometry fully inside the view fustrum need not be clipped
 *
 * \see RxNodeDefinitionGetAtomicEnumerateLights
 * \see RxNodeDefinitionGetAtomicInstance
 * \see RxNodeDefinitionGetLight
 * \see RxNodeDefinitionGetMaterialScatter
 * \see RxNodeDefinitionGetPostLight
 * \see RxNodeDefinitionGetPreLight
 * \see RxNodeDefinitionGetWorldSectorEnumerateLights
 * \see RxNodeDefinitionGetWorldSectorInstance
 */
RxNodeDefinition *
RxNodeDefinitionGetFastPathSplitter(void)
{
    /***********************************************/
    /**                                           **/
    /**  FASTPATHSPLITTER.CSL NODE SPECIFICATION  **/
    /**                                           **/
    /***********************************************/

    static RxClusterRef gNodeClusters[] = { /* */
        {&RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED} };

    #define NUMCLUSTERSOFINTEREST \
        ((sizeof(gNodeClusters))/(sizeof(gNodeClusters[0])))

    static RxClusterValidityReq gNodeReqs[NUMCLUSTERSOFINTEREST] = { /* */
        /* parallel to ClusterRefs */
        rxCLREQ_REQUIRED };

    static RxClusterValid gNodeOut1[NUMCLUSTERSOFINTEREST] = { /* */
        /* parallel to ClusterRefs */
        rxCLVALID_NOCHANGE };

    static RwChar _NeedsClipping[] = RWSTRING("NeedsClipping");

    static RwChar _WhollyInFrustum[] = RWSTRING("WhollyInFrustum");

    static RxOutputSpec gNodeOuts[] = { /* */
        {
         _NeedsClipping,          /* Name */
         gNodeOut1,                /* OutputClusters */
         rxCLVALID_NOCHANGE         /* AllOtherClusters */
         },
        {
         _WhollyInFrustum,        /* Name */
         gNodeOut1,                /* OutputClusters (referencing same cluster valid array as 1st output) */
         rxCLVALID_NOCHANGE         /* AllOtherClusters */
         } };

    #define NUMOUTPUTS \
        ((sizeof(gNodeOuts))/(sizeof(gNodeOuts[0])))

    static RwChar _FastPathSplitter_csl[] = RWSTRING("FastPathSplitter.csl");

    static RxNodeDefinition nodeFastPathSplitterCSL = { /* */
        _FastPathSplitter_csl,     /* Name */
        {                          /* nodemethods */
         FastPathSplitterNodeFn, /* +-- nodebody */
         (RxNodeInitFn)NULL,
         (RxNodeTermFn)NULL,
         (RxPipelineNodeInitFn)NULL,
         (RxPipelineNodeTermFn)NULL,
         (RxPipelineNodeConfigFn)NULL,
         (RxConfigMsgHandlerFn)NULL
        },
        {                          /* Io */
         NUMCLUSTERSOFINTEREST,    /* +-- NumClustersOfInterest */
         gNodeClusters,            /* +-- ClustersOfInterest */
         gNodeReqs,                /* +-- InputRequirements */
         NUMOUTPUTS,               /* +-- NumOutputs */
         gNodeOuts                 /* +-- Outputs */
         },
        0,
        (RxNodeDefEditable)FALSE,
        0 };

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionGetFastPathSplitter"));

    /*RWMESSAGE((RWSTRING("Pipeline II node")));*/

    RWRETURN(&nodeFastPathSplitterCSL);
}

