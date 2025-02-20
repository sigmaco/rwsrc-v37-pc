/*
 * Submitting lines to the rasteriser in custom pipelines
 *
 * Copyright (c) Criterion Software Limited
 */
/****************************************************************************
 *                                                                          *
 * module : nodeSubmitLines.c                                               *
 *                                                                          *
 * purpose: Submits lines to the rasteriser                                 *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 includes
 */

#include "rwcore.h"

#include <rpdbgerr.h>

#include "rtgncpip.h"


/* TODO: get this to support submission of indexed/unindexed triangles
   of any primitive type (maybe leave out fans, tristrips are the important
   one) - so default trilists for atomics/worldsectors. Maybe have separate
   nodes for different primitive types, but the additional code in here
   would be very lightweight and reusing common code is best for maintenance */

/****************************************************************************
 local defines
 */

#define MESSAGE(_string)                                             \
    RwDebugSendMessage(rwDEBUGMESSAGE, "nodeSubmitLineCSL", _string)

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/

/****************************************************************************
 SubmitLineNode

 TODO: we should be able to pass in an already-constructed set of indices to
       this node, in a RxVertexIndex cluster - useful for multipass FX where indices
       need only be generated once (and triangles backfaced once).
       We require the indices cluster as CLREQOPTIONAL and test
       (indices->flags & CLFLAGSVALID) to see if it had been previously generated.
       If you want Submit to actually have different requirements depending on
       position in the pipeline (ignore incoming indices (CLREQDONTCARE), Vs
       optionally take pre-generated indices (CLREQOPTIONAL)), you need two
       NodeDefinitions (though they can refer to the same code): SubmitFirst &
       SubmitSubsequent or some such.

 on entry: -
 on exit : -
*/

static              RwBool
SubmitLineNode( RxPipelineNodeInstance * self,
                const RxPipelineNodeParam * params __RWUNUSED__ )
{
    RxPacket            *packet;
    RxCluster           *devVerts;
    RxCluster           *indices;
    RxCluster           *meshState;
    RxCluster           *renderState;
    RxMeshStateVector   *meshData;
    RxRenderStateVector *rsvp;
    RxVertexIndex       *ind;

    RWFUNCTION(RWSTRING("SubmitLineNode"));

    packet = (RxPacket *)RxPacketFetch(self);
    RWASSERT((RxPacket *)NULL != packet);

    devVerts    = RxClusterLockRead(packet, 0);
    indices     = RxClusterLockRead(packet, 1);
    meshState   = RxClusterLockRead(packet, 2);
    renderState = RxClusterLockRead(packet, 3);

    RWASSERT((devVerts  != NULL) && (devVerts->data != NULL));
    RWASSERT( meshState != NULL);
    meshData = RxClusterGetCursorData(meshState, RxMeshStateVector);
    RWASSERT(meshData != NULL);

    /* TODO: we assume LineLists here atm */
    RWASSERT(meshData->NumVertices > 0);
    RWASSERT(meshData->NumElements > 0);
    RWASSERT(meshData->PrimType == rwPRIMTYPELINELIST);

    if ((renderState != NULL) && (renderState->data != NULL))
    {
        rsvp = RxClusterGetCursorData(renderState, RxRenderStateVector);
    }
    else
    {
        rsvp = &RXPIPELINEGLOBAL(defaultRenderState);
    }

    if (rsvp->TextureRaster)
    {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER,
                         (void *)rsvp->TextureRaster);
        RwRenderStateSet(rwRENDERSTATETEXTUREFILTER,
                         (void *)rsvp->FilterMode);

        if (rsvp->AddressModeU == rsvp->AddressModeV)
        {
            RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS,
                             (void *)rsvp->AddressModeU);
        }
        else
        {
            RwRenderStateSet(rwRENDERSTATETEXTUREADDRESSU,
                             (void *)rsvp->AddressModeU);
            RwRenderStateSet(rwRENDERSTATETEXTUREADDRESSV,
                             (void *)rsvp->AddressModeV);
        }
    }
    else
    {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER,
                         (void *) NULL);
    }

    /* Set shading modes */
    RwRenderStateSet(rwRENDERSTATESHADEMODE,
                     (void *) rsvp->ShadeMode);

    /* Set blending modes */
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,
                     (rxRENDERSTATEFLAG_VERTEXALPHAENABLE &rsvp->Flags) ?
                     (void *) TRUE : (void *) FALSE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND,
                     (void *) rsvp->SrcBlend);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,
                     (void *) rsvp->DestBlend);

    /* Set ZBuffering modes */
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,
                     (rxRENDERSTATEFLAG_ZTESTENABLE &rsvp->Flags) ?
                     (void *) TRUE : (void *) FALSE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,
                     (rxRENDERSTATEFLAG_ZWRITEENABLE &rsvp->Flags) ?
                     (void *) TRUE : (void *) FALSE);

    if ((NULL != indices) && (indices->numUsed > 0))
    {
        RwIm2DVertex *vertices;

        ind = RxClusterGetCursorData(indices, RxVertexIndex);
        RWASSERT(NULL != ind);
        RWASSERT(indices->stride == sizeof(RxVertexIndex));

        vertices = (RwIm2DVertex *)(RxClusterGetCursorData(devVerts, void));
        RwIm2DRenderIndexedPrimitive(rwPRIMTYPELINELIST,
                                     vertices,
                                     meshData->NumVertices,
                                     ind,
                                     meshData->NumElements * 2);
    }
    else
    {
        RwIm2DVertex *vertices;
        vertices = (RwIm2DVertex *)(RxClusterGetCursorData(devVerts, void));
        RwIm2DRenderPrimitive(rwPRIMTYPELINELIST,
                              vertices,
                              meshData->NumVertices);
    }

    /* Output the packet to the first (and only) output of this Node.
     * (something like multi-pass rendering could be done by subsequent Nodes)
     */
    RxPacketDispatch(packet, 0, self);

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionGetSubmitLine returns a pointer to a node
 * to submit lines to the rasterizer.
 *
 * This node submits lines to the rasterizer (it submits 2D lines
 * defined by \ref RxScrSpace2DVertex'es and RxIndicess, through the
 * RwIm2D interface).
 *
 * The node as it is currently only supports submission of line lists
 * (indexed or unindexed). Currently, polylines are assumed to be
 * expanded into line lists earlier in the pipeline (this occurs
 * in the Im3D node ImmManglelineIndices.csl). Direct support for polylines
 * will be added later.
 *
 * If the render state cluster is not present (or contains no data) then the
 * node assumes that the packet's render state is the default render state,
 * obtained through \ref RxRenderStateVectorGetDefaultRenderStateVector. This is
 * used in this case to set render states prior to rasterization.
 *
 * The node has a single output and packets pass unchanged through this. The
 * purpose of this is to allow packets to be modified and submitted again
 * later on in the pipeline to perform multipass rendering.
 *
 * The node has a single output, through which packets pass unchanged.
 * The input requirements of this node:
 *
 * \verbatim
   RxClScrSpace2DVertices - required
   RxClIndices            - optional
   RxClMeshState          - required
   RxClRenderState        - optional
   \endverbatim
 *
 * The characteristics of this node's first output:
 *
 * \verbatim
   RxClScrSpace2DVertices - no change
   RxClIndices            - no change
   RxClMeshState          - no change
   RxClRenderState        - no change
   \endverbatim
 *
 * \return A pointer to a node to submit lines to the rasterizer on success
 * or NULL if there is an error
 *
 * \see RxNodeDefinitionGetClipLine
 * \see RxNodeDefinitionGetClipTriangle
 * \see RxNodeDefinitionGetCullTriangle
 * \see RxNodeDefinitionGetImmInstance
 * \see RxNodeDefinitionGetImmMangleLineIndices
 * \see RxNodeDefinitionGetImmMangleTriangleIndices
 * \see RxNodeDefinitionGetImmRenderSetup
 * \see RxNodeDefinitionGetScatter
 * \see RxNodeDefinitionGetSubmitTriangle
 * \see RxNodeDefinitionGetTransform
 * \see RxNodeDefinitionGetUVInterp
 *
 */

RxNodeDefinition    *
RxNodeDefinitionGetSubmitLine(void)
{

    static RxClusterRef N1clofinterest[] = { /* */
        {&RxClScrSpace2DVertices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClIndices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClRenderState, rxCLALLOWABSENT, rxCLRESERVED}
    };

#define NUMCLUSTERSOFINTEREST \
        ((sizeof(N1clofinterest))/(sizeof(N1clofinterest[0])))

    static RxClusterValidityReq N1inputreqs[NUMCLUSTERSOFINTEREST] = { /* */
        rxCLREQ_REQUIRED,
        rxCLREQ_OPTIONAL,
        rxCLREQ_REQUIRED,
        rxCLREQ_OPTIONAL
    };

    static RxClusterValid N1outcl1[NUMCLUSTERSOFINTEREST] = { /* */
        rxCLVALID_NOCHANGE,
        rxCLVALID_NOCHANGE,
        rxCLVALID_NOCHANGE,
        rxCLVALID_NOCHANGE
    };

    static RwChar _SubmitOut[] = RWSTRING("SubmitOut");

    static RxOutputSpec N1outputs[] = { /* */
        {_SubmitOut,
         N1outcl1,
         rxCLVALID_NOCHANGE}
    };

#define NUMOUTPUTS \
        ((sizeof(N1outputs))/(sizeof(N1outputs[0])))

    static RwChar  _SubmitLine_csl[] = RWSTRING("SubmitLine.csl");

    static RxNodeDefinition    nodeSubmitLineCSL =
    {
        _SubmitLine_csl,
        {SubmitLineNode,
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
        (RxNodeDefEditable)FALSE,
        0
    };

    RxNodeDefinition  *result = &nodeSubmitLineCSL;

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionGetSubmitLine"));

    /*RWMESSAGE((RWSTRING("result %p"), result));*/

    RWRETURN(result);
}


