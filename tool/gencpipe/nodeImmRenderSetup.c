/*
 * Setting up packets to be scattered to an Im3D render pipeline
 *
 * Copyright (c) Criterion Software Limited
 */

/****************************************************************************
 *                                                                          *
 * module : nodeImmRenderSetup.c                                            *
 *                                                                          *
 * purpose: Sets up packets to be scattered to an Im3D render pipeline.     *
 *          Should run just after a RwIm3DRender[Indexed]Primitive() call.  *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 includes
 */

#include "rwcore.h"

#include "rpdbgerr.h"

#include "rtgncpip.h"

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/*****************************************************************************
 ImmRenderSetupNode

 Sets up packets for an Im3D render pipeline. It inspects the current renderstate
 and sets each packet's renderstate on that basis. It also creates a scatter
 cluster and sends each packet to the appropriate rwIm3DRenderPipeline by copying
 that into the privatedata of the subsequent scatter node.

 It also creates a cluster for the indices (referenced atm, so rxCLFLAGS_EXTERNAL
 the cluster) and puts the appropriate primitive type in the meshstate.

 on entry: - (void *)Data is a pointer to an ImmRenderSetup struct
 on exit : -
*/

static              RwBool
_ImmRenderSetupNode(RxPipelineNodeInstance * self,
                    const RxPipelineNodeParam * params)
{
    _rwIm3DPoolStash    *stash;
    RxRenderStateVector *rsvp;
    RxPacket           *packet;
    RxCluster          *clObjVerts, *clCamVerts, *clDevVerts,
        *clMeshState, *clRenderState, *clIndices;
    RwInt32             NumElements;

    RWFUNCTION(RWSTRING("_ImmRenderSetupNode"));

    RWASSERT(NULL != self);

    stash = (_rwIm3DPoolStash *) RxPipelineNodeParamGetData(params);
    RWASSERT(NULL != stash);

    if (stash->objVerts == NULL)
    {
        /* okay, what's happened here is that the stash node has
         * not been executed.
         *
         * most likely reason for non-execution of the stash node
         * is that transform dispatched down its "all vertices rejected"
         * output
         *
         * we respond by effecting a soft terminate of the render pipe
         */

        RWRETURN(TRUE);
    }

    /* Get current renderstate into a renderstatevector here */
    rsvp = RxRenderStateVectorCreate(TRUE);
    RWASSERT(NULL != rsvp);

    packet = RxPacketCreate(self);
    RWASSERT(NULL != packet);

    clObjVerts = RxClusterLockWrite(packet, 0, self);
    if (clObjVerts)
    {
        /* These are only used in a H/W T&L path so they're not
         * always gonna be present */
        clObjVerts = RxClusterSetExternalData(clObjVerts,
                                              stash->objVerts,
                                              sizeof(RxObjSpace3DVertex),
                                              stash->numVerts);
        RWASSERT(NULL != clObjVerts);
    }

    /* These don't get filled on HW T&L paths... */
    if (NULL != stash->camVerts)
    {
        clCamVerts = RxClusterLockWrite(packet, 1, self);
        RWASSERT(NULL != clCamVerts);
        clCamVerts = RxClusterSetExternalData(clCamVerts,
                                              stash->camVerts,
                                              sizeof(RxCamSpace3DVertex),
                                              stash->numVerts);
        RWASSERT(NULL != clCamVerts);
    }

    /* These don't get filled on HW T&L paths... */
    if (NULL != stash->devVerts)
    {
        clDevVerts = RxClusterLockWrite(packet, 2, self);
        RWASSERT(NULL != clDevVerts);
        clDevVerts = RxClusterSetExternalData(clDevVerts,
                                              stash->devVerts,
                                              sizeof(RxScrSpace2DVertex),
                                              stash->numVerts);
        RWASSERT(NULL != clDevVerts);
    }

    RWASSERT(NULL != stash->meshState);
    clMeshState = RxClusterLockWrite(packet, 3, self);
    RWASSERT(NULL != clMeshState);

    /* Set up the mesh data */
    clMeshState = RxClusterSetExternalData(clMeshState,
                                           stash->meshState,
                                           sizeof(RxMeshStateVector), 1);
    RWASSERT(NULL != clMeshState);

    RWASSERT(rwPRIMTYPENAPRIMTYPE != stash->primType);

    switch (stash->primType)
    {
        case rwPRIMTYPETRISTRIP:
            NumElements = stash->numIndices - 2;
            break;
        case rwPRIMTYPETRILIST:
            NumElements = stash->numIndices / 3;
            break;
        case rwPRIMTYPEPOINTLIST:
            /* We set it up for custom nodes to use but our later
             * nodes (inc. submit nodes) don't handle it */
            NumElements = stash->numIndices;
            break;
        case rwPRIMTYPELINELIST:
            NumElements = stash->numIndices >> 1;
            break;
        case rwPRIMTYPEPOLYLINE:
            NumElements = stash->numIndices - 1;
            break;
        case rwPRIMTYPETRIFAN:
            NumElements = stash->numIndices - 2;
            break;
        default:
            RWRETURN(FALSE);
            break;
    }
    (stash->meshState)->NumElements = NumElements;
    (stash->meshState)->PrimType = stash->primType;

#ifdef RWMETRICS
    /* We don't count lines/points */
    if ((rwPRIMTYPETRILIST == stash->primType) ||
        (rwPRIMTYPETRIFAN == stash->primType) ||
        (rwPRIMTYPETRISTRIP == stash->primType))
    {
        RWSRCGLOBAL(metrics)->numTriangles += NumElements;
    }
#endif /* RWMETRICS */

    clRenderState = RxClusterLockWrite(packet, 4, self);
    RWASSERT(NULL != clRenderState);

    if (NULL != stash->renderState)
    {
        RwBool              flag = FALSE;

        clRenderState = RxClusterSetExternalData(clRenderState,
                                                 stash->renderState,
                                                 sizeof(RxRenderStateVector),
                                                 1);
        RWASSERT(NULL != clRenderState);

        /* sdm - so... we actually always use
         * current device renderstate, simply modifying to
         * reflect vertexalpha flag as at end transform pipe...?
         */

        /* Vertex Alpha enabling was setup in
         * nodeImmInstanceCSL - it can be turned off by the
         * current renderstate but not back on */

        if ((stash->renderState)->Flags & rxRENDERSTATEFLAG_VERTEXALPHAENABLE)
        {
            flag = TRUE;
        }

        /* Copy across the current renderstate */
        *(stash->renderState) = *rsvp;

        /* nodeImmInstanceCSL says off off off! */
        if (flag == FALSE)
        {
            (stash->renderState)->Flags &= ~rxRENDERSTATEFLAG_VERTEXALPHAENABLE;
        }
    }
    else
    {
        clRenderState =
            RxClusterInitializeData(clRenderState, 1,
                                    sizeof(RxRenderStateVector));
        RWASSERT(NULL != clRenderState);
        RWASSERT(NULL !=
                 RxClusterGetCursorData(clRenderState, RxRenderStateVector));
        *RxClusterGetCursorData(clRenderState, RxRenderStateVector) = *rsvp;
        clRenderState->numUsed++;
    }

    RxRenderStateVectorDestroy(rsvp);

    if (NULL != stash->indices)
    {
        clIndices = RxClusterLockWrite(packet, 5, self);
        RWASSERT(NULL != clIndices);
        clIndices = RxClusterSetExternalData(clIndices, stash->indices,
                                             sizeof(RxVertexIndex),
                                             stash->numIndices);
        RWASSERT(NULL != clIndices);

        RxPacketDispatch(packet, 0, self);
    }
    else
    {
        /* OooOOooh, an unindexed primitive! */
        RxPacketDispatch(packet, 1, self);
    }

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionGetImmRenderSetup returns a pointer to a
 * node to set up packets in an Im3D render pipeline.
 *
 * This node sets the primitive type of packet (in its mesh state
 * cluster), adds indices to the packet (those passed in to the RwIm3D
 * render function, if any), sets the render state cluster to that set
 * up by the prior RwIm3D transform pipeline or to the current driver
 * state if none was set previously.
 *
 * This is first node of the default generic Im3D render pipeline. It
 * expects to receive a pointer to an _rwIm3DPoolStash struct through
 * its (void *)data parameter. This contains the primitive type, rwIM3D
 * flags, an (optional) \ref RwMatrix, the number of indices and vertices,
 * pointers to indices, object-space vertices, optionally (not on
 * platforms utilising hardware transformation) camera-space and
 * screen-space vertices, an \ref RxMeshStateVector and optionally an
 * \ref RxRenderStateVector.
 *
 * This node has two outputs. Packets with indices pass through the
 * first output and packets without indices pass through the second
 * output.
 * The input requirements of this node:
 *
 *\verbatim
   RxClObjSpace3DVertices - don't want
   RxClCamSpace3DVertices - don't want
   RxClScrSpace2DVertices - don't want
   RxClMeshState          - don't want
   RxClRenderState        - don't want
   RxClIndices            - don't want
  \endverbatim
 *
 * The characteristics of this node's first output:
 *
 *\verbatim
   RxClObjSpace3DVertices - valid
   RxClCamSpace3DVertices - valid
   RxClScrSpace2DVertices - valid
   RxClMeshState          - valid
   RxClRenderState        - valid
   RxClIndices            - valid
  \endverbatim
 *
 * The characteristics of this node's second output:
 *
 *\verbatim
   RxClObjSpace3DVertices - valid
   RxClCamSpace3DVertices - valid
   RxClScrSpace2DVertices - valid
   RxClMeshState          - valid
   RxClRenderState        - valid
   RxClIndices            - invalid
   \endverbatim
 *
 * \return A pointer to a node to set up packets in an Im3D render pipeline.
 *
 * \see RxNodeDefinitionGetClipLine
 * \see RxNodeDefinitionGetClipTriangle
 * \see RxNodeDefinitionGetCullTriangle
 * \see RxNodeDefinitionGetImmInstance
 * \see RxNodeDefinitionGetImmMangleLineIndices
 * \see RxNodeDefinitionGetImmMangleTriangleIndices
 * \see RxNodeDefinitionGetScatter
 * \see RxNodeDefinitionGetSubmitLine
 * \see RxNodeDefinitionGetSubmitTriangle
 * \see RxNodeDefinitionGetTransform
 * \see RxNodeDefinitionGetUVInterp
 *
 */

RxNodeDefinition   *
RxNodeDefinitionGetImmRenderSetup(void)
{
    static RxClusterRef N1clofinterest[] = { /* */
        /* 0 */ {&RxClObjSpace3DVertices, rxCLALLOWABSENT,
                 rxCLRESERVED},
        /* 1 */ {&RxClCamSpace3DVertices, rxCLALLOWABSENT,
                 rxCLRESERVED},
        /* 2 */ {&RxClScrSpace2DVertices, rxCLALLOWABSENT,
                 rxCLRESERVED},
        /* 3 */ {&RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED},
        /* 4 */ {&RxClRenderState, rxCLALLOWABSENT, rxCLRESERVED},
        /* 5 */ {&RxClIndices, rxCLALLOWABSENT, rxCLRESERVED}
    };

#define NUMCLUSTERSOFINTEREST \
        ((sizeof(N1clofinterest))/(sizeof(N1clofinterest[0])))

    static RxClusterValidityReq N1inputreqs[NUMCLUSTERSOFINTEREST] = { /* */
        rxCLREQ_DONTWANT,
        rxCLREQ_DONTWANT,
        rxCLREQ_DONTWANT,
        rxCLREQ_DONTWANT,
        rxCLREQ_DONTWANT,
        rxCLREQ_DONTWANT
    };

    static RxClusterValid N1outcl1[NUMCLUSTERSOFINTEREST] = { /* */
        /* Note: if the objVerts cluster is not present (no subsequent
         * node uses it) then it won't actually be valid, but since no
         * other node uses it, it doesn't matter! Neat, huh? :) */
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_VALID
    };

    static RxClusterValid N1outcl2[NUMCLUSTERSOFINTEREST] = { /* */
        /* Note: if the objVerts cluster is not present (no subsequent
         * node uses it) then it won't actually be valid, but since no
         * other node uses it, it doesn't matter! Neat, huh? :) */
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_INVALID
    };

    static RwChar       _ImmRenderSetupOut[] = RWSTRING("ImmRenderSetupOut");

    static RwChar       _ImmRenderSetupOutUnindexed[] =
        RWSTRING("ImmRenderSetupOutUnindexed");

    static RxOutputSpec N1outputs[] = { /* */
        {_ImmRenderSetupOut,
         N1outcl1,
         rxCLVALID_INVALID},
        {_ImmRenderSetupOutUnindexed,
         N1outcl2,
         rxCLVALID_INVALID}
    };

#define NUMOUTPUTS \
        ((sizeof(N1outputs))/(sizeof(N1outputs[0])))

    static RwChar       _ImmRenderSetup_csl[] =

        RWSTRING("ImmRenderSetup.csl");

    static RxNodeDefinition nodeImmRenderSetupCSL = {
        _ImmRenderSetup_csl,
        {_ImmRenderSetupNode,
         (RxNodeInitFn)NULL,
         (RxNodeTermFn)NULL,
         (RxPipelineNodeInitFn)NULL,
         (RxPipelineNodeTermFn)NULL,
         (RxPipelineNodeConfigFn)NULL,
         (RxConfigMsgHandlerFn)NULL},
        {NUMCLUSTERSOFINTEREST,
         N1clofinterest,
         N1inputreqs,
         NUMOUTPUTS,
         N1outputs},
        0,
        (RxNodeDefEditable) FALSE,
        0
    };

    RxNodeDefinition   *result = &nodeImmRenderSetupCSL;

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionGetImmRenderSetup"));

    /*RWMESSAGE((RWSTRING("result %p"), result)); */

    RWRETURN(result);
}
