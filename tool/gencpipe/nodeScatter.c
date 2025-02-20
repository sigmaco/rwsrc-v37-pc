/*
 * Multiplexing data down certain branches of a pipeline or to other pipelines
 *
 * Copyright (c) Criterion Software Limited
 */
/****************************************************************************
 *                                                                          *
 * module : nodeScatter.c                                                   *
 *                                                                          *
 * purpose: Splits Nodes down a certain branch of a pipeline or to other    *
 *          pipelines based on data in the RxScatter cluster of each packet *
 *          or the private data of this node                                *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 includes
 */

#include "rwcore.h"

#include "rpdbgerr.h"

#include "rtgncpip.h"


/****************************************************************************
 local defines
 */


#define MESSAGE(_string)                                       \
    RwDebugSendMessage(rwDEBUGMESSAGE, "Scatter.csl", _string)


/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/*****************************************************************************
 FindOutputByNode - utility func, does what it says
 */

static              RwInt32
FindOutputByNode(RxPipelineNode *self, RxPipelineNode *dest, RxPacket *packet)
{
    RxPipeline        *pipeline = packet->pipeline;
    RwUInt32           i = 0;

    RWFUNCTION(RWSTRING("FindOutputByNode"));

    while ((i < self->numOutputs)
           && (&pipeline->nodes[self->outputs[i]] != dest))
    {
        i++;
    }

    if (i == self->numOutputs)
    {
        RWRETURN(-1);
    }
    RWRETURN(i);
}

/*****************************************************************************
 _RxScatterNodeInitFn

 Sets the private data (a RxScatter struct) of this pipeline node to something
 innocuous. No need to deinitialise this data.

 */
static              RwBool
_RxScatterPipelineNodeInitFn(RxPipelineNode * self)
{
    RxScatter        *override;

    RWFUNCTION(RWSTRING("_RxScatterPipelineNodeInitFn"));

    RWASSERT(NULL != self);

    override = (RxScatter *) self->privateData;

    override->node = (RxPipelineNode *)NULL;
    override->pipeline = (RxPipeline *)NULL;

    RWRETURN(TRUE);
}

/*****************************************************************************
 RxScatterNode

 Sends packets to an output based on each packet's RxScatter cluster - you either
 output to a pipeline or a pipelinenode (we work out which output of the current
 node it is).

 We can override the scatter clusters of nodes - the node's private data can point
 to a RxScatter struct which is used for all packets.

 on entry: -
 on exit : -
*/

static              RwBool
_RxScatterNode( RxPipelineNodeInstance * self,
                const RxPipelineNodeParam * params __RWUNUSED__ )
{
    RxScatter *override = (RxScatter *) self->privateData;
    RxPacket  *packet;
    RwInt32    overrideNode;

    RWFUNCTION(RWSTRING("_RxScatterNode"));

    packet = (RxPacket *) RxPacketFetch(self);
    RWASSERT((RxPacket *)NULL != packet);

    if (override && (override->pipeline || override->node))
    {
        /* Both a pipeline and a pipelinenode cannot simultaneously be
         * specified as override output! */
        RWASSERT(!((NULL != override->pipeline) &&
                   (NULL != override->node)));

        /* Output this one to the appropriate pipeline */
        if (override->pipeline)
        {
            RxPacketDispatchToPipeline(packet, override->pipeline, self);
        }
        else
        {
            overrideNode = FindOutputByNode(self, override->node, packet);
            RWASSERT(overrideNode != -1); /* -1 => Invalid output */
            /* Output this one to the appropriate node */
            RxPacketDispatch(packet, (RwUInt16) overrideNode, self);
        }
    }
    else
    {
        RxCluster *scatterCluster;
        RxScatter *scatterData;
        RwInt32    output;

        scatterCluster = RxClusterLockWrite(packet, 0, self);
        if (scatterCluster)
        {
            scatterData =
                RxClusterGetCursorData(scatterCluster, RxScatter);

            if (scatterData->pipeline)
            {
                /* Output this one to the appropriate pipeline */
                RxPacketDispatchToPipeline(packet, scatterData->pipeline, self);
            }
            else
            {
                output = FindOutputByNode(self, scatterData->node, packet);
                RWASSERT(output != -1); /* -1 => Invalid output */
                /* Output this one to the appropriate node */
                RxPacketDispatch(packet, (RwUInt16) output, self);
            }
        }
        else
        {
            /* Default output */
            RxPacketDispatch(packet, 0, self);
        }
    }

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionGetScatter returns a pointer to a node
 * to multiplex data down certain branches of a pipeline or
 * to other pipelines.
 *
 * This node scatters packets down certain branches of a pipeline, based
 * either on data in each packet's (optional) RxScatter cluster, or on
 * this node's private data (itself an RxScatter struct).
 *
 * The node's private data, if set, will override the data in an RxScatter
 * cluster in packets. If packets do not contain an RxScatter cluster and
 * the node's private data is not set, the packets are by default passed
 * on to the node's first output.
 *
 * The RxScatter cluster contains a pipeline pointer (NULL meaning not set)
 * and a pointer to a pipeline node (within the current pipeline, NULL
 * meaning not set). These should never both be set at the same time.
 *
 * The node has 32 outputs (the maximum allowed) to facilitate extreme
 * branching of the pipeline. None need actually be connected.
 * The input requirements of this node:
 *
 * \verbatim
   RxClScatter            - optional
   \endverbatim
 *
 * The characteristics of all this node's outputs:
 *
 * \verbatim
   RxClScatter            - no change
   \endverbatim
 *
 * \return A pointer to a node to multiplex data down certain branches of
 * a pipeline based on data in the RxScatter cluster of each packet
 *
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

RxNodeDefinition    *
RxNodeDefinitionGetScatter(void)
{
    static RxClusterRef N1clofinterest[] = { /* */
        {&RxClScatter, rxCLALLOWABSENT, rxCLRESERVED}
    };

#define NUMCLUSTERSOFINTEREST \
        ((sizeof(N1clofinterest))/(sizeof(N1clofinterest[0])))

    static RxClusterValidityReq N1inputreqs[NUMCLUSTERSOFINTEREST] = { /* */
        rxCLREQ_OPTIONAL
    };

    static RxClusterValid N1outcl1[NUMCLUSTERSOFINTEREST] = { /* */
        rxCLVALID_NOCHANGE
    };

    static RwChar _ScatterOut1[]  = RWSTRING("ScatterOut1");
    static RwChar _ScatterOut2[]  = RWSTRING("ScatterOut2");
    static RwChar _ScatterOut3[]  = RWSTRING("ScatterOut3");
    static RwChar _ScatterOut4[]  = RWSTRING("ScatterOut4");
    static RwChar _ScatterOut5[]  = RWSTRING("ScatterOut5");
    static RwChar _ScatterOut6[]  = RWSTRING("ScatterOut6");
    static RwChar _ScatterOut7[]  = RWSTRING("ScatterOut7");
    static RwChar _ScatterOut8[]  = RWSTRING("ScatterOut8");
    static RwChar _ScatterOut9[]  = RWSTRING("ScatterOut9");
    static RwChar _ScatterOut10[] = RWSTRING("ScatterOut10");
    static RwChar _ScatterOut11[] = RWSTRING("ScatterOut11");
    static RwChar _ScatterOut12[] = RWSTRING("ScatterOut12");
    static RwChar _ScatterOut13[] = RWSTRING("ScatterOut13");
    static RwChar _ScatterOut14[] = RWSTRING("ScatterOut14");
    static RwChar _ScatterOut15[] = RWSTRING("ScatterOut15");
    static RwChar _ScatterOut16[] = RWSTRING("ScatterOut16");
    static RwChar _ScatterOut17[] = RWSTRING("ScatterOut17");
    static RwChar _ScatterOut18[] = RWSTRING("ScatterOut18");
    static RwChar _ScatterOut19[] = RWSTRING("ScatterOut19");
    static RwChar _ScatterOut20[] = RWSTRING("ScatterOut20");
    static RwChar _ScatterOut21[] = RWSTRING("ScatterOut21");
    static RwChar _ScatterOut22[] = RWSTRING("ScatterOut22");
    static RwChar _ScatterOut23[] = RWSTRING("ScatterOut23");
    static RwChar _ScatterOut24[] = RWSTRING("ScatterOut24");
    static RwChar _ScatterOut25[] = RWSTRING("ScatterOut25");
    static RwChar _ScatterOut26[] = RWSTRING("ScatterOut26");
    static RwChar _ScatterOut27[] = RWSTRING("ScatterOut27");
    static RwChar _ScatterOut28[] = RWSTRING("ScatterOut28");
    static RwChar _ScatterOut29[] = RWSTRING("ScatterOut29");
    static RwChar _ScatterOut30[] = RWSTRING("ScatterOut30");
    static RwChar _ScatterOut31[] = RWSTRING("ScatterOut31");
    static RwChar _ScatterOut32[] = RWSTRING("ScatterOut32");

    /* We define 32 outputs with the same characteristics - RxClScatter is
       output as valid and all other clusters are left as they are. We don't
       have to *connect* any of these outputs but they must be defined */
    static RxOutputSpec N1outputs[] = { /* */
        {_ScatterOut1, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut2, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut3, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut4, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut5, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut6, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut7, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut8, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut9, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut10, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut11, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut12, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut13, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut14, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut15, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut16, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut17, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut18, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut19, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut20, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut21, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut22, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut23, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut24, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut25, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut26, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut27, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut28, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut29, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut30, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut31, N1outcl1, rxCLVALID_NOCHANGE},
        {_ScatterOut32, N1outcl1, rxCLVALID_NOCHANGE}
    };

#define NUMOUTPUTS \
        ((sizeof(N1outputs))/(sizeof(N1outputs[0])))

    static RwChar _RxScatter_csl[] = RWSTRING("RxScatter.csl");

    static RxNodeDefinition nodeScatterCSL=
    {
        _RxScatter_csl,
        {_RxScatterNode,
         (RxNodeInitFn)NULL,
         (RxNodeTermFn)NULL,
         _RxScatterPipelineNodeInitFn,
         (RxPipelineNodeTermFn) NULL,
         (RxPipelineNodeConfigFn) NULL,
         (RxConfigMsgHandlerFn) NULL},
        /* We don't know how many outputs the splitternode
         * will have in any given pipeline, so give it 32 outputs
         * (the max allowed for an RxNodeDefinition), since
         * it's fine to have any number of them unconnected */
        {NUMCLUSTERSOFINTEREST,
         N1clofinterest,
         N1inputreqs,
         NUMOUTPUTS,
         N1outputs},
        sizeof(RxScatter),
        (RxNodeDefEditable)FALSE,
        0
    };

    RxNodeDefinition *result = &nodeScatterCSL;

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionGetScatter"));

    /*RWMESSAGE((RWSTRING("result %p"), result));*/

    RWRETURN(result);
}

