/*
 * nodeAtomicEnumerateLights
 * Enumerating lights illuminating an atomic |
 *
 * Copyright (c) Criterion Software Limited
 */
/****************************************************************************
 *                                                                          *
 * module : nodeAtomicEnumerateLights.c                                     *
 *                                                                          *
 * purpose: Enumerate the lights illuminating an atomic.                    *
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

/*****************************************************************************
 LightsClusterAddLight
 */

static RwBool
LightsClusterAddLight(RxCluster *cl, RxLight /* == (RpLight *) */ light)
{
    RWFUNCTION(RWSTRING("LightsClusterAddLight"));

    /* Grow cluster data, if required w/ granularity 8 "RxLight"s */
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
 AtomicGlobalLightsCB
 */

static RpLight *
AtomicGlobalLightsCB(RpLight *light, void *cl)
{
    RWFUNCTION(RWSTRING("AtomicGlobalLightsCB"));

    if ( rwObjectTestFlags(light, rpLIGHTLIGHTATOMICS) )
    {
        LightsClusterAddLight((RxCluster *) cl, light);
    }

    RWRETURN(light);
}

/*****************************************************************************
 AtomicLocalLightsCB
 */

static RpLight *
AtomicLocalLightsCB(RpLight *light, void *cl)
{
    RWFUNCTION(RWSTRING("AtomicLocalLightsCB"));

    if ( light->lightFrame != RWSRCGLOBAL(lightFrame) )
    {
        if ( rwObjectTestFlags(light, rpLIGHTLIGHTATOMICS) )
        {
            /* TODO: make sure the light's region of influence overlaps
                     with the atomic's bounding sphere                  */
            LightsClusterAddLight((RxCluster *) cl, light);
        }

        light->lightFrame = RWSRCGLOBAL(lightFrame);
    }

    RWRETURN(light);
}

/****************************************************************************
 AtomicEnumerateLightsNodeBody()
 */

static RwBool
AtomicEnumerateLightsNodeBody(RxPipelineNodeInstance *self,
                              const RxPipelineNodeParam * params)
{
    RxPacket  *pk;
    RxCluster *cl;
    RpAtomic  *atomic;

    RWFUNCTION(RWSTRING("AtomicEnumerateLightsNodeBody"));

    RWASSERT(NULL != self);
    RWASSERT(NULL != params);
    atomic = (RpAtomic *)RxPipelineNodeParamGetData(params);
    RWASSERT(NULL != atomic);

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe Check
	THREADSAFE_CHECK_ISCALLEDMAIN();
	//@} DDonSS

    pk = RxPacketFetch(self);
    RWASSERT(NULL != pk);

    cl = RxClusterLockWrite(pk, 0 /* clusterLight */, self);
    RWASSERT(NULL != cl);

    /* Start a fresh array of lights */
    cl = RxClusterInitializeData(cl, 8, sizeof(RxLight));
    RWASSERT(NULL != cl);

    if ( (rpGEOMETRYLIGHT & RpGeometryGetFlags(atomic->geometry)) &&
         (NULL != RWSRCGLOBAL(curWorld)) )
    {
        RwLLLink *cur, *end;

        /* Increase the marker ! */
        RWSRCGLOBAL(lightFrame)++;

        /* Directional light it */
        rpWorldForAllGlobalLights(AtomicGlobalLightsCB, cl);

        /* For all sectors that this atomic lies in, apply all lights within */
        cur = rwLinkListGetFirstLLLink(&atomic->llWorldSectorsInAtomic);
        end = rwLinkListGetTerminator(&atomic->llWorldSectorsInAtomic);
        while ( cur != end )
        {
            RpTie *tpTie = rwLLLinkGetData(cur, RpTie, lWorldSectorInAtomic);

            /* Now apply all the lights (but this time we do the frame thing) */
            rpWorldSectorForAllLocalLights(tpTie->worldSector,
                                           AtomicLocalLightsCB,
                                           cl);

            /* Next one */
            cur = rwLLLinkGetNext(cur);
        }
    }

    RxPacketDispatch(pk, 0 /* DefaultOutput */, self);

    RWRETURN(TRUE);
}


/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionGetAtomicEnumerateLights returns a pointer to a node
 * to enumerate lights illuminating an atomic
 *
 * This node works out which lights in the world illuminate the atomic from
 * which each packet was spawned. It then puts pointers to each of these
 * lights in an RxLight cluster (the RxLight struct is just a pointer to an
 * RpLight).
 *
 * Global lights flagged as lighting atomics will affect all atomics. Local
 * lights flagged as lighting atomics can only affect atomics if they are
 * in (that is their region of influence overlaps with) at least one of the
 * world sectors which the atomic is in (those whose bounding box overlap the
 * atomic's bounding sphere).
 *
 * The node has one output, through which the packets pass with their new
 * RxLight cluster.
 * The input requirements of this node:
 *  \li RxClLights             - don't want
 *
 * The characteristics of this node's first output:
 *  \li RxClLights             - valid
 *
 * \return pointer to a node to enumerate lights illuminating an atomic
 *
 * \see RxNodeDefinitionGetAtomicInstance
 * \see RxNodeDefinitionGetFastPathSplitter
 * \see RxNodeDefinitionGetLight
 * \see RxNodeDefinitionGetMaterialScatter
 * \see RxNodeDefinitionGetPostLight
 * \see RxNodeDefinitionGetPreLight
 * \see RxNodeDefinitionGetWorldSectorEnumerateLights
 * \see RxNodeDefinitionGetWorldSectorInstance
 */
RxNodeDefinition *
RxNodeDefinitionGetAtomicEnumerateLights(void)
{
    static RxClusterRef gNodeClusters[] =
    {
          /* 0 */
        { &RxClLights, rxCLALLOWABSENT, rxCLRESERVED }
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

    static RwChar _AtomicEnumerateLights_csl[] =
        RWSTRING("AtomicEnumerateLights.csl");

    static RxNodeDefinition nodeAtomicEnumerateLightsCSL =
    {
        _AtomicEnumerateLights_csl,                /* Name */
        {                                           /* nodemethods */
            AtomicEnumerateLightsNodeBody,       /* +-- nodebody */
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

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionGetAtomicEnumerateLights"));

    /*RWMESSAGE((RWSTRING("Pipeline II node")));*/

    RWRETURN(&nodeAtomicEnumerateLightsCSL);
}

