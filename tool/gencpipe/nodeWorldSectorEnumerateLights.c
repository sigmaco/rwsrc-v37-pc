/*
 * nodeWorldSectorEnumerateLights
 * Enumerating the lights illuminating a world sector |
 *
 * Copyright (c) Criterion Software Limited
 */
/****************************************************************************
 *                                                                          *
 * module : nodeWorldSectorEnumerateLights.c                                *
 *                                                                          *
 * purpose: Enumerate the lights illuminating a world sector.               *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 includes
 */

#include "rwcore.h"
#include "rpworld.h"

#include "rpdbgerr.h"

#include "p2stdclsw.h"

#include "rtgncpip.h"

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/*****************************************************************************
 LightsClusterAddLight
 */

static RwBool
LightsClusterAddLight(RxCluster *cl, RxLight /* == (RpLight *) */ light)
{
    RWFUNCTION(RWSTRING("LightsClusterAddLight"));

    /* grow cluster data, if required w/ granularity 8 "RxLight"s */
    if ( cl->numUsed == cl->numAlloced )
    {
        cl = RxClusterResizeData(cl, cl->numUsed + 8);
        RWASSERT(NULL != cl);
    }

    *RxClusterGetIndexedData(cl, RxLight, cl->numUsed) = light;

    cl->numUsed++;

    RWRETURN(TRUE);
}

/*****************************************************************************
 WorldLightsCB
 */

static RpLight *
WorldLightsCB(RpLight *light, void *cl)
{
    RWFUNCTION(RWSTRING("WorldLightsCB"));

    if ( rwObjectTestFlags(light, rpLIGHTLIGHTWORLD) )
    {
        LightsClusterAddLight((RxCluster *) cl, light);
    }

    RWRETURN(light);
}

/*****************************************************************************
 WorldSectorEnumerateLights
 */

static void
WorldSectorEnumerateLights(RxCluster *cl, RpWorldSector *worldsector)
{
    RpWorld *world = (RpWorld *)RWSRCGLOBAL(curWorld);

    RWFUNCTION(RWSTRING("WorldSectorEnumerateLights"));

    if (rpWORLDLIGHT & RpWorldGetFlags(world))
    {
        rpWorldForAllGlobalLights(WorldLightsCB, cl);
        rpWorldSectorForAllLocalLights(worldsector, WorldLightsCB, cl);
    }

    RWRETURNVOID();
}

/****************************************************************************
 WorldSectorEnumerateLightsNodeBody()
 */

static RwBool
WorldSectorEnumerateLightsNodeBody(RxPipelineNodeInstance *self,
                                   const RxPipelineNodeParam * params)
{
    RxPacket  *pk;
    RxCluster *cl;

    RWFUNCTION(RWSTRING("WorldSectorEnumerateLightsNodeBody"));

    RWASSERT(NULL != self);

    pk = RxPacketFetch(self);
    RWASSERT(NULL != pk);

    cl = RxClusterLockWrite(pk, 0 /* clusterLight */, self);
    RWASSERT(NULL != cl);

    /* Start a fresh array of lights */
    cl = RxClusterInitializeData(cl, 8, sizeof(RxLight));
    RWASSERT(NULL != cl);

    RWASSERT(NULL != params);
    WorldSectorEnumerateLights(
        cl, (RpWorldSector *)RxPipelineNodeParamGetData(params));

    RxPacketDispatch(pk, 0 /* DefaultOutput */, self);

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionGetWorldSectorEnumerateLights returns a pointer to
 * a node to enumerate the lights illuminating a world sector
 *
 * This node works out which lights in the world illuminate the world sector
 * from which each packet was spawned. It then puts pointers to each of these
 * lights in an RxLight cluster (the RxLight struct is just a pointer to an
 * RpLight).
 *
 * Global lights flagged as lighting world sectors will affect all world
 * sectors. Local lights flagged as lighting world sectors can only affect
 * a world sector if they are in (that is their region of influence overlaps
 * with) that world sector.
 *
 * The node has one output, through which the packets pass with their new
 * RxLight cluster.
 * The input requirements of this node:
 *      \li RxClLights             - don't want
 *
 * The characteristics of this node's first output:
 *      \li RxClLights             - valid
 *
 * \return pointer to a node to enumerate the lights illuminating
 * a world sector
 *
 * \see RxNodeDefinitionGetAtomicEnumerateLights
 * \see RxNodeDefinitionGetAtomicInstance
 * \see RxNodeDefinitionGetFastPathSplitter
 * \see RxNodeDefinitionGetLight
 * \see RxNodeDefinitionGetMaterialScatter
 * \see RxNodeDefinitionGetPostLight
 * \see RxNodeDefinitionGetPreLight
 * \see RxNodeDefinitionGetWorldSectorInstance
 */
RxNodeDefinition *
RxNodeDefinitionGetWorldSectorEnumerateLights(void)
{
    /*********************************************************/
    /**                                                     **/
    /**  WORLDSECTORENUMERATELIGHTS.CSL NODE SPECIFICATION  **/
    /**                                                     **/
    /*********************************************************/

    static RxClusterRef gNodeClusters[] =
    {
          /* 0 */
        { &RxClLights, rxCLALLOWABSENT, rxCLRESERVED  }
    };

    #define NUMCLUSTERSOFINTEREST \
        ((sizeof(gNodeClusters))/(sizeof(gNodeClusters[0])))

    /* input requirements (this array parallel to ClusterRefs) */
    static RxClusterValidityReq gNodeReqs[NUMCLUSTERSOFINTEREST] =
    {
          rxCLREQ_DONTWANT
    };

    /* output state (this array parallel to ClusterRefs) */
    static RxClusterValid gNodeOut1[NUMCLUSTERSOFINTEREST] =
    {
         rxCLVALID_VALID
    };

    static RwChar _DefaultOutput[] = RWSTRING("DefaultOutput");

    static RxOutputSpec gNodeOuts[] =
    {
        {
            _DefaultOutput,             /* Name */
            gNodeOut1,                   /* OutputClusters */
            rxCLVALID_NOCHANGE            /* AllOtherClusters */
        }
    };

    #define NUMOUTPUTS \
        ((sizeof(gNodeOuts))/(sizeof(gNodeOuts[0])))

    static RwChar _NodeName[] = RWSTRING("WorldSectorEnumerateLights.csl");

    static RxNodeDefinition NodeDefinition =
    {
        _NodeName,           /* Name */
        {                                           /* nodemethods */
            WorldSectorEnumerateLightsNodeBody,  /* +-- nodebody */
            (RxNodeInitFn)NULL,
            (RxNodeTermFn)NULL,
            (RxPipelineNodeInitFn)NULL,
            (RxPipelineNodeTermFn)NULL,
            (RxPipelineNodeConfigFn)NULL,
            (RxConfigMsgHandlerFn)NULL
        },
        {                                           /* Io */
            NUMCLUSTERSOFINTEREST,                  /* +-- NumClustersOfInterest */
            gNodeClusters,                          /* +-- ClustersOfInterest */
            gNodeReqs,                              /* +-- InputRequirements */
            NUMOUTPUTS,                             /* +-- NumOutputs */
            gNodeOuts                               /* +-- Outputs */
        },
        0,
        (RxNodeDefEditable)FALSE,
        0
    };

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionGetWorldSectorEnumerateLights"));

    /*RWMESSAGE((RWSTRING("Pipeline II node")));*/

    RWRETURN(&NodeDefinition);
}

