/*
 * nodeClone
 *
 * Cloning packets and sending them down certain branches of a pipeline |
 *
 * (c) Criterion Software Limited
 */

/****************************************************************************
 *                                                                          *
 * module : nodeClone.c                                                     *
 *                                                                          *
 * purpose: Clones a packet and sends it to several outputs, these          *
 *          specified in private data                                       *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 includes
 */

#include <string.h>            /* needed for memset */

#include "rwcore.h"

#include "rpdbgerr.h"

#include "rtgncpip.h"

#include "nodeClone.h"


/****************************************************************************
 local defines
 */

#define MESSAGE(_string)                                     \
    RwDebugSendMessage(rwDEBUGMESSAGE, "Clone.csl", _string)

#define RWALIGN32(n) ( ((n) + 3U) & ~3U )

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/*****************************************************************************
 _validataCloneInitData

 Checks the data passed to RxCloneDefineModes()/RxNodeDefinitionCloneCreate()
 Checks against a pipeline node if supplied.
*/
static              RwBool
_validataCloneInitData(RxNodeCloneInitData * data, RxPipelineNode * node)
{
    RwBool              error = FALSE;
    RwChar              string[256];
    RwUInt32            i, j;

    RWFUNCTION(RWSTRING("_validataCloneInitData"));

    if (data->numModes < 1)
    {
        MESSAGE("Unable to initialise clone node with less than 1 modes");
        RWRETURN(FALSE);
    }
    if (data->modes == NULL)
    {
        MESSAGE
            ("Unable to initialise clone node modes - no mode data specified");
        RWRETURN(FALSE);
    }
    if (data->modeSizes == NULL)
    {
        MESSAGE
            ("Unable to initialise clone node modes - no mode sizes array provided");
        RWRETURN(FALSE);
    }
    if (node != NULL)
    {
        if (data->numOutputs != node->numOutputs)
        {
            MESSAGE
                ("Unable to initialise clone node modes - wrong number of outputs specified");
            error = TRUE;
        }
    }
    if (data->numOutputs > RXNODEMAXOUTPUTS)
    {
        MESSAGE
            ("Unable to initialise clone node modes - too many outputs specified");
        error = TRUE;
    }
    for (i = 0; i < data->numModes; i++)
    {
        if (data->modeSizes[i] > data->numOutputs)
        {
            rwsprintf(string,
                      "Unable to initialise clone node modes - mode %d specifies too many outputs",
                      i);
            MESSAGE(string);
            error = TRUE;
        }
        if (data->modes[i] == NULL)
        {
            rwsprintf(string,
                      "Unable to initialise clone node modes - mode %d specifies no data",
                      i);
            MESSAGE(string);
            error = TRUE;
        }
        else
        {
            if (node != NULL)
            {
                for (j = 0; j > data->modeSizes[j]; j++)
                {
                    if ((data->modes[i])[j] >= node->numOutputs)
                    {
                        rwsprintf(string,
                                  "Unable to initialise clone node modes - mode %d contains an invalid output",
                                  i);
                        MESSAGE(string);
                        error = TRUE;
                    }
                }
            }
        }
    }

    RWRETURN(!error);
}

/**
 * \ingroup rtgencpipe
 * \ref RxPipelineNodeCloneDefineModes
 * is used to define the mode(s) of operation of a specified clone pipeline node.
 * This function takes a pointer to a struct of type \ref RxNodeCloneInitData
 * and from that sets up one or more modes of operation for the node.
 * The \ref RxNodeCloneInitData struct should be the same one which was passed to
 * \ref RxNodeDefinitionCloneCreate to create the \ref RxNodeDefinition for
 * this node.
 *
 * \param pipeline  A pointer to the \ref RxPipeline to which the Clone node belongs
 * \param node  A pointer to the Clone \ref RxPipelineNode
 * \param data  A pointer to the \ref RxNodeCloneInitData setup data
 *
 * \return TRUE if successful or FALSE if there is an error
 *
 * \see RxPacketCacheCreate
 * \see RxPacketCacheClone
 * \see RxPacketCacheDestroy
 * \see RxPipelineNodeCloneOptimize
 * \see RxNodeDefinitionCloneCreate
 * \see RxNodeDefinitionCloneDestroy
 * \see RxNodeDefinitionGetClipLine
 * \see RxNodeDefinitionGetClipTriangle
 * \see RxNodeDefinitionGetCullTriangle
 * \see RxNodeDefinitionGetImmInstance
 * \see RxNodeDefinitionGetImmMangleLineIndices
 * \see RxNodeDefinitionGetImmMangleTriangleIndices
 * \see RxNodeDefinitionGetImmRenderSetup
 * \see RxNodeDefinitionGetSubmitLine
 * \see RxNodeDefinitionGetSubmitTriangle
 * \see RxNodeDefinitionGetTransform
 * \see RxNodeDefinitionGetUVInterp
 *
 */
RwBool
RxPipelineNodeCloneDefineModes(RxPipeline * pipeline,
                               RxPipelineNode * node,
                               RxNodeCloneInitData * data)
{
    RWAPIFUNCTION(RWSTRING("RxPipelineNodeCloneDefineModes"));

    if (pipeline->locked == FALSE)
    {
        RxNodeCloneData    *nodeData = (RxNodeCloneData *) node->privateData;
        void               *block;
        RwUInt32            size, i;

#if (0)
        RwBool              error = FALSE;
#endif /* (0) */

        /* Validate incoming data */
        if (_validataCloneInitData(data, node) == FALSE)
            RWRETURN(FALSE);

        size = ( sizeof(RxNodeCloneInitData) +
                 data->numModes * sizeof(RwUInt32) +
                 data->numModes * sizeof(RwUInt32 *) +
                 data->numModes * data->numOutputs * sizeof(RwUInt32) );

        block = RwMalloc(size, rwID_GENCPIPETOOLKIT | rwMEMHINTDUR_EVENT);
        if (block != NULL)
        {
            /* Free existing modes data if there is any */
            if (nodeData->data != NULL)
                RwFree(nodeData->data);

            /* Make a copy of the incoming initdata */
            nodeData->data = (RxNodeCloneInitData *) block;
            nodeData->data->numModes = data->numModes;
            nodeData->data->numOutputs = data->numOutputs;
            block = (void *) (nodeData->data + 1);

            nodeData->data->modeSizes = (RwUInt32 *) block;
            memcpy(nodeData->data->modeSizes, data->modeSizes,
                   sizeof(RwUInt32) * data->numModes);
            block = (void *) (nodeData->data->modeSizes + data->numModes);

            nodeData->data->modes = (RwUInt32 **) block;
            block =
                (void *) (nodeData->data->modes + nodeData->data->numModes);

            for (i = 0; i < data->numModes; i++)
            {
                nodeData->data->modes[i] = (RwUInt32 *) block;
                memcpy(nodeData->data->modes[i], data->modes[i],
                       data->numOutputs * sizeof(RwUInt32));
                block =
                    (void *) (nodeData->data->modes[i] + data->numOutputs);
            }

            nodeData->currentMode = 0;
            nodeData->optimized = FALSE;

            RWRETURN(TRUE);
        }
        RWERROR((E_RW_NOMEM, size));
        RWRETURN(FALSE);
    }

    RWERROR((E_RX_UNLOCKEDPIPE));
    RWRETURN(FALSE);
}

/**
 * \ingroup rtgencpipe
 * \ref RxPipelineNodeCloneOptimize
 * optimises a Clone pipeline
 * node's operation by calculating which clusters propragate down which
 * outputs of the pipeline node, thus allowing it to specify
 * rxCLFLAGS_EXTERNAL as few times as possible (it can result in data
 * copying and does not need to be specified for each cluster the last
 * time it is output in a packet from the Clone pipeline node).
 *
 * This function should be called after unlocking the containing
 * pipeline to that cluster propagations is fixed in its final state.
 * The clone node needs to ensure that for each packet in which it
 * dispatches a cluster, the outgoing data is the same (i.e it is a
 * true clone of the original data). This requires that for all but
 * the last dispatch of each cluster, it flags the cluster with
 * rxCLFLAGS_EXTERNAL because this ensures that if any subsequent
 * nodes edit the cluster's data, it is copied rather than modified
 * in-place.
 *
 * \param  pipeline   A pointer to the pipeline
 *       to which the node to be optimized belongs
 * \param  node   A pointer to the pipeline node
 *         to be optimized
 *
 * \return TRUE if successful or FALSE if there is an error
 *
 * \see RxPacketCacheCreate
 * \see RxPacketCacheClone
 * \see RxPacketCacheDestroy
 * \see RxPipelineNodeCloneDefineModes
 * \see RxNodeDefinitionCloneCreate
 * \see RxNodeDefinitionCloneDestroy
 * \see RxNodeDefinitionGetClipLine
 * \see RxNodeDefinitionGetClipTriangle
 * \see RxNodeDefinitionGetCullTriangle
 * \see RxNodeDefinitionGetImmInstance
 * \see RxNodeDefinitionGetImmMangleLineIndices
 * \see RxNodeDefinitionGetImmMangleTriangleIndices
 * \see RxNodeDefinitionGetImmRenderSetup
 * \see RxNodeDefinitionGetSubmitLine
 * \see RxNodeDefinitionGetSubmitTriangle
 * \see RxNodeDefinitionGetTransform
 * \see RxNodeDefinitionGetUVInterp
 *
 */
RwBool
RxPipelineNodeCloneOptimize( RxPipeline * pipeline,
                             RxPipelineNode * node __RWUNUSEDRELEASE__ )
{
    RWAPIFUNCTION(RWSTRING("RxPipelineNodeCloneOptimize"));

    RWASSERT(NULL != pipeline);
    RWASSERT(NULL != node);

    if (pipeline->locked == FALSE)
    {
        /*
         * TODO:
         *  the extension is to do this for the last time each CLUSTER
         * is used (Simon says: Acquiring the information you want for
         * this is non-trivial.... you may be able to make use of the
         * slotsContinue bit-field, if you feel it's worth the hassle).
         * The extra data should be generated by RxPipelineNodeCloneOptimize()
         * but this stuff will also require modifcation of the packetcache
         * funcs (e.g not auto-setting packet clusters to EXTERNAL in the
         * cache creator - instead have a packetsetclusterflags(packet,
         * cache, mode, output) func which does the optimal thing.
         *
         * TODO:
         *  work out which clusters propagate and which aren't needed
         * (slotsContinue bit-fields), optimise which clusters need to
         * be sent to each output and which can be ignored here
         * (allows the optimisation at the top of giving proper flag
         * (not necessarily external) to the last use of a cluster)
         *
         * no need to actually skip clusters when cloning a packet, just
         * have RxPacketCacheClone look for the extra data generated here
         * and use it if it's there
         *
         * ((RxNodeCloneData *)node->privateData)->optimized = TRUE;
         *
         */

        RWRETURN(TRUE);
    }
    RWERROR((E_RX_LOCKEDPIPE));
    RWRETURN(FALSE);
}

/*****************************************************************************
 _RxCloneNodeInitFn

 Sets the private data of this pipeline node to something innocuous. No need
 to deinitialise this data.
*/
static              RwBool
_RxClonePipelineNodeInitFn(RxPipelineNode * Self)
{
    RxNodeCloneData    *data;
    RwUInt32           *allocResult;
    RwUInt32            size, i;
    RwUInt32            defaultNumModes = 1,
        defaultNumOutputs = 1,
        defaultModeSizes[1] = { 1 }, defaultModes[1][1] =
    {
        {
        0}
    };

    RWFUNCTION(RWSTRING("_RxClonePipelineNodeInitFn"));

    RWASSERT(NULL != Self);

    size = sizeof(RxNodeCloneInitData) +
        defaultNumModes * sizeof(RwUInt32) +
        defaultNumModes * sizeof(RwUInt32 *) +
        defaultNumModes * defaultModeSizes[0] * sizeof(RwUInt32);
    allocResult = (RwUInt32 *) RwMalloc(size,
        rwID_GENCPIPETOOLKIT | rwMEMHINTDUR_EVENT);
    if (allocResult == NULL)
    {
        RWERROR((E_RW_NOMEM, size));
        RWRETURN(FALSE);
    }

    data = (RxNodeCloneData *) Self->privateData;
    data->data = (RxNodeCloneInitData *) allocResult;
    data->data->numModes = defaultNumModes;
    data->data->numOutputs = defaultNumOutputs;
    allocResult = (RwUInt32 *) (data->data + 1);

    data->data->modeSizes = (RwUInt32 *) allocResult;
    for (i = 0; i < defaultNumModes; i++)
    {
        data->data->modeSizes[i] = defaultModeSizes[i];
    }
    allocResult = (RwUInt32 *) (data->data->modeSizes + defaultNumModes);

    data->data->modes = (RwUInt32 **) allocResult;
    allocResult = (RwUInt32 *) (data->data->modes + defaultNumModes);
    for (i = 0; i < defaultNumModes; i++)
    {
        data->data->modes[i] = allocResult;
        memcpy(data->data->modes[i], defaultModes[i],
               defaultNumOutputs * sizeof(RwUInt32));
        allocResult = (RwUInt32 *) (data->data->modes[i] + defaultNumOutputs);
    }

    data->currentMode = 0;
    data->optimized = TRUE;    /* Effectively, given only one output */

    RWRETURN(TRUE);
}

/*****************************************************************************
 _RxClonePipelineNodeTermFn

 Frees locally allocated modes data
*/
static void
_RxClonePipelineNodeTermFn(RxPipelineNode * Self)
{
    RxNodeCloneData    *data = (RxNodeCloneData *) (Self->privateData);

    RWFUNCTION(RWSTRING("_RxClonePipelineNodeTermFn"));

    RWASSERT(NULL != Self);
    RWASSERT(NULL != data);

    /* We conscientiously allocated in one big block so freeing's a doddle */
    if (data->data != NULL)
    {
        RwFree(data->data);
        data->data = (RxNodeCloneInitData *)NULL;
    }
    data->optimized = FALSE;

    RWRETURNVOID();
}

/* TODO: These following three should become API functions/structs when
 * you've got time to tidy/speed them up, add docs and decide what they
 * should do and allow the user to do via params */

/**
 * \ingroup rtgencpipe
 * \ref RxPacketCacheCreate
 * creates a \ref RxPacketCache
 * from  an \ref RxPacket
 * which can be used to clone that packet.
 *
 * Owing to the nested pipe execution mechanism (see \ref RxPipelineExecute
 * for details), only one packet can exist at a time. The \ref RxPacketCache
 * structure makes it possible to create clones of a packet within a
 * pipeline node and dispatch these, possibly to different outputs of the
 * node.
 *
 * To use the \ref RxPacketCache, you should first create it using this
 * function. Then, dispatch (or destroy, though it seems unlikely you would
 * want to do this) the original packet. Finally, repeat this procedure as
 * many times as you wish: create a clone of the original packet using
 * \ref RxPacketCacheClone, modify that packet as desired (or not) and then
 * dispatch it.
 *
 * \note There are other considerations for last time you clone a packet
 * (see \ref RxPacketCacheClone for details).
 *
 * The lifetime of an \ref RxPacketCache is up to the end of the execution
 * of the pipeline node that creates it, so that node should call
 * \ref RxPacketCacheDestroy after last using the cache.
 *
 * On creating a cache, it is assumed that you will be using it to create
 * clones of the original packet, so \ref RxPacketCacheCreate takes steps
 * to modify the flags of clusters within the original packet to make
 * this possible. For the original packet and all but the last created
 * clone (see \ref RxPacketCacheClone), the flags of all clusters within
 * them (that were active and contained data in the original packet at
 * the time of cache creation) must be set to rxCLFLAGS_EXTERNAL if they
 * are not already set thus - because all packets will reference the
 * same data, any modifications to cluster data futher down the pipeline
 * must cause the data to be copied rather than changing the original
 * data and invalidating the premise that the content of cloned packets
 * is identical to that of the original. See \ref RxClusterSetExternalData
 * for further details on external data.
 *
 * An exception to the above rule is clusters with data marked as
 * rxCLFLAGS_EXTERNALMODIFIABLE - this data is in a fixed location for
 * some (we assume) important reason, so moving it somewhere else will
 * break things. Hence, this data may change from clone to clone.
 *
 * \param  packet    A pointer to an \ref RxPacket
 * \param  node    A pointer to the calling \ref RxPipelineNode
 * \param  heap   A pointer to the current \ref RxHeap
 *
 * \return A pointer to the created \ref RxPacketCache on success,
 * or NULL if there is an error
 *
 * \see RxPacketCacheClone
 * \see RxPacketCacheDestroy
 * \see RxPipelineNodeCloneDefineModes
 * \see RxPipelineNodeCloneOptimize
 * \see RxNodeDefinitionCloneCreate
 * \see RxNodeDefinitionCloneDestroy
 * \see RxNodeDefinitionGetClipLine
 * \see RxNodeDefinitionGetClipTriangle
 * \see RxNodeDefinitionGetCullTriangle
 * \see RxNodeDefinitionGetImmInstance
 * \see RxNodeDefinitionGetImmMangleLineIndices
 * \see RxNodeDefinitionGetImmMangleTriangleIndices
 * \see RxNodeDefinitionGetImmRenderSetup
 * \see RxNodeDefinitionGetSubmitLine
 * \see RxNodeDefinitionGetSubmitTriangle
 * \see RxNodeDefinitionGetTransform
 * \see RxNodeDefinitionGetUVInterp
 *
 */

RxPacketCache      *
RxPacketCacheCreate(RxPacket * packet,
                    RxPipelineNode * node __RWUNUSED__,
                    RxHeap * heap)
{
    RxPacketCache      *cache;
    RwUInt32            numPipelineClusters, numClusters, size, i;

    RWAPIFUNCTION(RWSTRING("RxPacketCacheCreate"));

    RWASSERT(NULL != packet);

    /* TODO: RxPipelineNodeCloneOptimize() should work out which clusters
     * are active for this node a priori so we don't have to overallocate
     * here nor test rxCLFLAGS_NULL below. */
    numPipelineClusters = packet->numClusters;
    size = sizeof(RxPacketCache) +
        (numPipelineClusters - 1) * sizeof(RxPacketCacheCluster);
    cache = (RxPacketCache *) RxHeapAlloc(heap, size);
    RWASSERT(NULL != cache);

    cache->packetFlags = packet->flags;
    numClusters = 0;
    for (i = 0; i < numPipelineClusters; i++)
    {
        RxCluster          *cluster = &(packet->clusters[i]);

        /* If cluster flags is rxCLFLAGS_NULL, the rest of the members are
         * invalid and they'll have been ZEROed during packetcreate or a
         * prior destroy (or the cluster hasn't been activated yet) */
        if (cluster->flags != rxCLFLAGS_NULL)
        {
            cache->clusters[numClusters].slot = i;
            cache->clusters[numClusters].flags = cluster->flags;
            cache->clusters[numClusters].stride = cluster->stride;
            cache->clusters[numClusters].data = cluster->data;
            cache->clusters[numClusters].numAlloced = cluster->numAlloced;
            cache->clusters[numClusters].numUsed = cluster->numUsed;
            cache->clusters[numClusters].clusterRef = cluster->clusterRef;
            numClusters++;

            /* Set the flags in the original packet to EXTERNAL so they won't be
             * freed - clone only gets called when two or more copies of a packet
             * need to be submitted, in which case the first naturally needs to
             * have its data set to EXTERNAL */
            /* NOTE :you can't copy external-modifiable data because
             *       it's implicit that that data is in a fixed place
             *       for a good reason! (e.g PS2 DMA packet data) Ergo
             *       you must simply allow that data to be overwritten
             *       in place, sorry... -> don't unset MODIFIABLE bit */
            /* (TODO: not optimal... when RxPipelineNodeCloneOptimize() is
             * written, we will look for its extra data in the cache and use
             * it to set flags optimally on a per cluster basis) */
            if (cluster->data != NULL)
            {
                /* NOTE: it's harmful to flag clusters with no data as
                 *       having external data!! (it breaks stuff like
                 *       RxClusterResizeData) */
                cluster->flags |= rxCLFLAGS_EXTERNAL;
            }
        }
    }
    cache->numClusters = numClusters;
    /* After the cache has been used with (lastClone == TRUE),
     * it should not be used again!! */
    cache->lastCloneDone = FALSE;

    RWRETURN(cache);
}

/**
 * \ingroup rtgencpipe
 * \ref RxPacketCacheClone
 * creates a new \ref RxPacket
 * from an \ref RxPacketCache.
 *
 * After you have create an \ref RxPacketCache using \ref RxPacketCacheCreate,
 * you can use this function to create \ref RxPacket clones of your original
 * packet.
 *
 * The last clone that you create from an \ref RxPacketCache should be
 * treated differently, so pass in TRUE for the lastClone parameter. For
 * all clusters in the original packet that were active and had data when
 * the cache was created, it is necessary for them to have their cluster
 * data flagged as rxCLFLAGS_EXTERNAL in the cases of the original packet
 * and all but the last clone created. This is because the clusters in the
 * clone packets will reference the original data, so in order to preserve
 * the premise that clone packets contain the same data as the original
 * packet, any modifications to data further down the pipeline must be
 * made to make a copy of the data before modifying it (this is what
 * rwCLFLAGS_EXTERNAL does - see \ref RxClusterSetExternalData for details),
 * or else the data will be different for subsequent clones. In the case
 * of the last clone, the data is free to change since there are no
 * subsequent clones for this to affect.
 *
 * \param cache  A pointer to an \ref RxPacketCache
 * \param node  A pointer to the calling \ref RxPipelineNode
 * \param lastClone  An \ref RwBool value specifying whether this is the last clone to be
 * created from the \ref RxPacketCache
 *
 * \return A pointer to the new \ref RxPacket on success, or NULL if there is an error
 *
 * \see RxPacketCacheCreate
 * \see RxPacketCacheDestroy
 * \see RxPipelineNodeCloneDefineModes
 * \see RxPipelineNodeCloneOptimize
 * \see RxNodeDefinitionCloneCreate
 * \see RxNodeDefinitionCloneDestroy
 * \see RxNodeDefinitionGetClipLine
 * \see RxNodeDefinitionGetClipTriangle
 * \see RxNodeDefinitionGetCullTriangle
 * \see RxNodeDefinitionGetImmInstance
 * \see RxNodeDefinitionGetImmMangleLineIndices
 * \see RxNodeDefinitionGetImmMangleTriangleIndices
 * \see RxNodeDefinitionGetImmRenderSetup
 * \see RxNodeDefinitionGetSubmitLine
 * \see RxNodeDefinitionGetSubmitTriangle
 * \see RxNodeDefinitionGetTransform
 * \see RxNodeDefinitionGetUVInterp
 *
 */
RxPacket           *
RxPacketCacheClone(RxPacketCache * cache,
                   RxPipelineNodeInstance * node, RwBool lastClone)
{
    RxPacketCacheCluster *cacheCluster = (RxPacketCacheCluster *)NULL;
    RxPacket           *packet = (RxPacket *)NULL;
    RxCluster          *cluster = (RxCluster *)NULL;
    RwUInt32            i;

    RWAPIFUNCTION(RWSTRING("RxPacketCacheClone"));

    RWASSERT(NULL != cache);
    RWASSERT(NULL != node);

    /* After the cache has been used with (lastClone == TRUE),
     * it should not be used again!! */
    RWASSERT(FALSE == cache->lastCloneDone);

    packet = RxPacketCreate(node);
    RWASSERT(NULL != packet);

    packet->flags = cache->packetFlags;
    if (FALSE == lastClone)
    {
        cacheCluster = &(cache->clusters[0]);
        for (i = 0; i < cache->numClusters; i++)
        {
            /* TODO: optimize() should calc' offsets into the clusters array,
             *       integer multiples are expensive these days.. *sigh* */
            cluster = &(packet->clusters[cacheCluster->slot]);
            cluster->flags = cacheCluster->flags;

            /* Cos the data might be used again subsequently, we need to flag
             * data-holding clusters as having EXTERNAL data */
            if (cacheCluster->data != NULL)
            {
                /* NOTE: it's harmful to flag clusters with no data as
                 *       having external data!! */
                cluster->flags &= ~rxCLFLAGS_EXTERNALMODIFIABLE;
                cluster->flags |= rxCLFLAGS_EXTERNAL;
            }
            cluster->data = cacheCluster->data;

            cluster->stride = cacheCluster->stride;
            cluster->numAlloced = cacheCluster->numAlloced;
            cluster->numUsed = cacheCluster->numUsed;
            cluster->clusterRef = cacheCluster->clusterRef;

            cacheCluster++;
        }
    }
    else
    {
        /* After the cache has been used with (lastClone == TRUE),
         * it should not be used again!! */
        cache->lastCloneDone = TRUE;

        /* We don't flags clusters as EXTERNAL the last time
         * round (see autoduck comments above) */
        cacheCluster = &(cache->clusters[0]);
        for (i = 0; i < cache->numClusters; i++)
        {
            cluster = &(packet->clusters[cacheCluster->slot]);

            cluster->flags = cacheCluster->flags;
            cluster->stride = cacheCluster->stride;
            cluster->data = cacheCluster->data;
            cluster->numAlloced = cacheCluster->numAlloced;
            cluster->numUsed = cacheCluster->numUsed;
            cluster->clusterRef = cacheCluster->clusterRef;

            cacheCluster++;
        }
    }
    RWRETURN(packet);
}

/**
 * \ingroup rtgencpipe
 * \ref RxPacketCacheDestroy
 * Destroys an \ref RxPacketCache as
 * created by \ref RxPacketCacheCreate.
 *
 * \param cache  A pointer to the \ref RxPacketCache to destroy
 * \param heap  A pointer to the current \ref RxHeap. This should be the heap
 * which was passed to \ref RxPacketCacheCreate when this \ref RxPacketCache
 * was created.
 *
 * \see RxPacketCacheCreate
 * \see RxPacketCacheClone
 * \see RxPipelineNodeCloneDefineModes
 * \see RxPipelineNodeCloneOptimize
 * \see RxNodeDefinitionCloneCreate
 * \see RxNodeDefinitionCloneDestroy
 * \see RxNodeDefinitionGetClipLine
 * \see RxNodeDefinitionGetClipTriangle
 * \see RxNodeDefinitionGetCullTriangle
 * \see RxNodeDefinitionGetImmInstance
 * \see RxNodeDefinitionGetImmMangleLineIndices
 * \see RxNodeDefinitionGetImmMangleTriangleIndices
 * \see RxNodeDefinitionGetImmRenderSetup
 * \see RxNodeDefinitionGetSubmitLine
 * \see RxNodeDefinitionGetSubmitTriangle
 * \see RxNodeDefinitionGetTransform
 * \see RxNodeDefinitionGetUVInterp
 *
 */
void
RxPacketCacheDestroy(RxPacketCache * cache, RxHeap * heap)
{
    RWAPIFUNCTION(RWSTRING("RxPacketCacheDestroy"));

    RWASSERT(NULL != cache);

    /* Well, the last clone must have been
     * created if any were created at all! */
    RWASSERT(FALSE != cache->lastCloneDone);

    RxHeapFree(heap, cache);

    RWRETURNVOID();
}

/*****************************************************************************
 _RxCloneNode

 Clones incoming packets and outputs them to n different outputs in the order
 specified by the node's private data.
*/
static              RwBool
_RxCloneNode(RxPipelineNodeInstance * self,
             const RxPipelineNodeParam * params)
{
    RxNodeCloneData    *privateData = (RxNodeCloneData *) self->privateData;
    RxHeap             *heap;
    RxPacket           *packet;
    RwUInt32            mode, numOutputs, *outputs, i;

    RWFUNCTION(RWSTRING("_RxCloneNode"));

    mode = privateData->currentMode;
    numOutputs = privateData->data->modeSizes[mode];
    outputs = privateData->data->modes[mode];

    RWASSERT(NULL != params);
    heap = RxPipelineNodeParamGetHeap(params);
    RWASSERT(NULL != heap);

    RWASSERT(NULL != self);
    packet = (RxPacket *)RxPacketFetch(self);
    RWASSERT((RxPacket *)NULL != packet);

    if (numOutputs > 1)
    {
        RxPacketCache      *cache = RxPacketCacheCreate(packet, self, heap);

        RxPacketDispatch(packet, outputs[0], self);
        for (i = 1; i < (numOutputs - 1); i++)
        {
            packet = RxPacketCacheClone(cache, self, FALSE);
            /* TODO: we should set cluster flags here using data from
             * RxPipelineNodeCloneOptimize() - EXTERNAL is set PER CLUSTER
             * except for the last output that NEEDs the cluster! */
            RxPacketDispatch(packet, outputs[i], self);
        }
        /* The last one can use the original cluster flags instead of EXTERNAL */
        packet = RxPacketCacheClone(cache, self, TRUE);
        RxPacketDispatch(packet, outputs[i], self);
        RxPacketCacheDestroy(cache, heap);
    }
    else
    {
        /* There's no point doing any cloning stuff if we're not actually
         * duplicating the packet (we might still be using nodeClone instead
         * of nodeScatter if other modes had more than one output, see...) */
        RxPacketDispatch(packet, outputs[0], self);
    }

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionCloneCreate
 * returns a pointer to a node
 * to clone packets and send them down certain branches of a pipeline.
 *
 * This node, for each incoming packet, caches the contents of that
 * packet and sends 'clones' of the packet down several outputs. It
 * can operate in several modes, these being defined in the private
 * data of the node. The private data, for the \ref RxPipelineNode
 * referencing the \ref RxNodeDefinition
 * output by this function,
 * should be set up after the containing \ref RxPipeline has been
 * unlocked by passing \ref RxPipelineNodeCloneDefineModes the
 * same \ref RxNodeCloneInitData struct as is passed to this function.
 * Each mode specifies, in order, the outputs that the cloned packets
 * are sent to.
 *
 * If you run \ref RxPipelineNodeCloneOptimize on the same \ref RxPipelineNode,
 * it will set up some extra private data to optimize its operation such that
 * it only sets cluster flags to EXTERNAL when it absolutely has to. (This is
 * all but the last time a cluster is used in an outgoing packet, see
 * \ref RxPipelineNodeCloneOptimize for further details.) If you don't run
 * RxPipelineNodeCloneOptimize on it, it will be
 * less precise and set cluster flags to EXTERNAL in all but the last clone
 * of each packet (which could result in unnecessary copying of data).
 *
 * \param data  Pointer to a \ref RxNodeCloneInitData struct to use to set up the
 * \ref RxNodeDefinition
 *
 * \note The new node has as many outputs as are specified in the parameter data.
 * This node has no input requirements nor does it specify any output
 * characteristics. All clusters that are active on entering this node
 * will be unchanged on exit and no others will be activated.
 *
 * \return pointer to a node to clone packets and send them down certain
 * branches of a pipeline.
 *
 * \see RxNodeDefinitionCloneDestroy
 * \see RxPacketCacheCreate
 * \see RxPacketCacheClone
 * \see RxPacketCacheDestroy
 * \see RxPipelineNodeCloneDefineModes
 * \see RxPipelineNodeCloneOptimize
 * \see RxNodeDefinitionGetClipLine
 * \see RxNodeDefinitionGetClipTriangle
 * \see RxNodeDefinitionGetCullTriangle
 * \see RxNodeDefinitionGetImmInstance
 * \see RxNodeDefinitionGetImmMangleLineIndices
 * \see RxNodeDefinitionGetImmMangleTriangleIndices
 * \see RxNodeDefinitionGetImmRenderSetup
 * \see RxNodeDefinitionGetSubmitLine
 * \see RxNodeDefinitionGetSubmitTriangle
 * \see RxNodeDefinitionGetTransform
 * \see RxNodeDefinitionGetUVInterp
 */
RxNodeDefinition   *
RxNodeDefinitionCloneCreate(RxNodeCloneInitData * data)
{
    RwChar              _RxClone_csl[] = RWSTRING("Clone.csl");
    RwChar              _outputsName[] = RWSTRING("OutputNN");
    RxNodeMethods       nodeMethods = { _RxCloneNode,
                                        (RxNodeInitFn)NULL,
                                        (RxNodeTermFn)NULL,
                                        _RxClonePipelineNodeInitFn,
                                        _RxClonePipelineNodeTermFn,
                                        (RxPipelineNodeConfigFn)NULL,
                                        (RxConfigMsgHandlerFn)NULL};

    RxNodeDefinition   *nodeCloneCSL = (RxNodeDefinition *)NULL;
    RwUInt8            *block;
    RwUInt32            size, i;

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionCloneCreate"));

    if (data == NULL)
    {
        RWERROR((E_RW_NULLP));
        RWRETURN((RxNodeDefinition *)NULL);
    }
    if (_validataCloneInitData(data, (RxPipelineNode *)NULL) == FALSE)
    {
        RWRETURN(FALSE);
    }

    size = RWALIGN32(sizeof(RxNodeDefinition)) +
        RWALIGN32(1 + strlen(_RxClone_csl)) + /* Name string */
        RWALIGN32(0 * sizeof(RxClusterRef)) + /* No clusters of interest... */
        RWALIGN32(0 * sizeof(RxClusterValidityReq)) + /* ...therefore no input requirements */
        RWALIGN32(data->numOutputs * sizeof(RxOutputSpec)) + /* Output specifications */
        data->numOutputs * (RWALIGN32(1 + strlen(_outputsName)) +
                            RWALIGN32(0 * sizeof(RxClusterValid)));

    block = (RwUInt8 *) RwMalloc(size, rwID_GENCPIPETOOLKIT | rwMEMHINTDUR_EVENT);
    if (block != NULL)
    {
        /* node */
        nodeCloneCSL = (RxNodeDefinition *) block;
        block += RWALIGN32(sizeof(RxNodeDefinition));

        /* name */
        nodeCloneCSL->name = (char *) block;
        strcpy((char *) nodeCloneCSL->name, (const char *) _RxClone_csl);
        block += RWALIGN32(strlen(_RxClone_csl) + 1);

        /* outputs */
        nodeCloneCSL->io.outputs = (RxOutputSpec *) block;
        block += RWALIGN32(data->numOutputs * sizeof(RxOutputSpec));

        for (i = 0; i < data->numOutputs; i++)
        {
            /* name */
            nodeCloneCSL->io.outputs[i].name = (char *) block;
            block += RWALIGN32(1 + strlen(_outputsName));
            rwsprintf((char *) nodeCloneCSL->io.outputs[i].name,
                      "Output%d", i);

            /* outputclusters */
            nodeCloneCSL->io.outputs[i].outputClusters = (RxClusterValid *)NULL;

            /* allotherclusters */
            nodeCloneCSL->io.outputs[i].allOtherClusters = rxCLVALID_NOCHANGE;
        }

        nodeCloneCSL->nodeMethods = nodeMethods;
        nodeCloneCSL->io.numClustersOfInterest = 0;
        nodeCloneCSL->io.clustersOfInterest = (RxClusterRef *)NULL;
        nodeCloneCSL->io.inputRequirements = (RxClusterValidityReq *)NULL;
        nodeCloneCSL->io.numOutputs = data->numOutputs;
        nodeCloneCSL->pipelineNodePrivateDataSize = sizeof(RxNodeCloneData);
        nodeCloneCSL->editable = (RxNodeDefEditable) FALSE;
        nodeCloneCSL->InputPipesCnt = 0;
    }
    else
    {
        RWERROR((E_RW_NOMEM));
    }

    RWRETURN(nodeCloneCSL);
}

/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionCloneDestroy
 * destroys a node definition created by
 * \ref RxNodeDefinitionCloneCreate
 *
 * \param def  A pointer to the node definition to destroy
 *
 * \return TRUE if successful or FALSE if there is an error
 *
 * \see RxNodeDefinitionCloneCreate
 * \see RxPacketCacheCreate
 * \see RxPacketCacheClone
 * \see RxPacketCacheDestroy
 * \see RxPipelineNodeCloneDefineModes
 * \see RxPipelineNodeCloneOptimize
 * \see RxNodeDefinitionGetClipLine
 * \see RxNodeDefinitionGetClipTriangle
 * \see RxNodeDefinitionGetCullTriangle
 * \see RxNodeDefinitionGetImmInstance
 * \see RxNodeDefinitionGetImmMangleLineIndices
 * \see RxNodeDefinitionGetImmMangleTriangleIndices
 * \see RxNodeDefinitionGetImmRenderSetup
 * \see RxNodeDefinitionGetSubmitLine
 * \see RxNodeDefinitionGetSubmitTriangle
 * \see RxNodeDefinitionGetTransform
 * \see RxNodeDefinitionGetUVInterp
 *
 */
RwBool
RxNodeDefinitionCloneDestroy(RxNodeDefinition * def)
{
    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionCloneDestroy"));

    RWASSERT(NULL != def);

    /* It was all allocated in one block in RxNodeDefinitionCloneCreate(),
     * so it's pretty damn simple to gerridofit */
    RwFree(def);
    RWRETURN(TRUE);
}
