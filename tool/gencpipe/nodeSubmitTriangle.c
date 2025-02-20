/*
 * Submitting triangles to the rasteriser in custom pipelines
 *
 * Copyright (c) Criterion Software Limited
 */

/****************************************************************************
 *                                                                          *
 * module : nodeSubmitTriangle.c                                            *
 *                                                                          *
 * purpose: Submits triangles to the rasteriser                             *
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

#define MESSAGE(_string) \
  RwDebugSendMessage(rwDEBUGMESSAGE, "RxNodeDefinitionGetSubmitTriangle()", _string)

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/****************************************************************************
 SubmitTriangleNode

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
SubmitTriangleNode( RxPipelineNodeInstance * self,
                    const RxPipelineNodeParam * params __RWUNUSED__ )
{
    RxPacket            *packet;
    RxCluster           *devVerts;
    RxCluster           *indices;
    RxCluster           *meshState;
    RxCluster           *renderState;
    RxMeshStateVector   *meshData = (RxMeshStateVector *)NULL;
    RxRenderStateVector *rsvp;

    RWFUNCTION(RWSTRING("SubmitTriangleNode"));

    packet = (RxPacket *)RxPacketFetch(self);
    RWASSERT(NULL != packet);

    devVerts = RxClusterLockRead(packet, 0);
    indices = RxClusterLockRead(packet, 1);
    meshState = RxClusterLockRead(packet, 2);
    renderState = RxClusterLockRead(packet, 3);

    RWASSERT((devVerts  != NULL) && (devVerts->data != NULL));
    RWASSERT( meshState != NULL);
    meshData = RxClusterGetCursorData(meshState,
                                      RxMeshStateVector);
    RWASSERT(meshData != NULL);

    /* TODO: we assume TriLists here atm */
    RWASSERT(meshData->NumVertices > 0);
    RWASSERT(meshData->NumElements > 0);
    RWASSERT(meshData->PrimType == rwPRIMTYPETRILIST);

    if ((renderState != NULL) && (renderState->data != NULL))
    {
        rsvp =
            RxClusterGetCursorData(renderState,
                                   RxRenderStateVector);
    }
    else
    {
        rsvp = &RXPIPELINEGLOBAL(defaultRenderState);
    }

    /* Set the appropriate texture */
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

    /* Set blending modes */
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,
                     (rxRENDERSTATEFLAG_VERTEXALPHAENABLE &rsvp->Flags) ?
                     (void *) TRUE : (void *) FALSE);

    /* NOTE:
     * the test used for determining if triangles are not present is still
     * not definitive... we need to sort out clearly how you can tell if
     * clusters are alive/dead and what to do when the number of entries in
     * a cluster drops to zero (e.g here if all verts are clipped, and all
     * triangles culled) */
    if ((indices != NULL) && (indices->numAlloced > 0))
    {
        RwIm2DVertex *vertices;
        RxVertexIndex *ind = RxClusterGetCursorData(indices, RxVertexIndex);
        RWASSERT(NULL != ind);
        RWASSERT(indices->stride == sizeof(RxVertexIndex));

        vertices = (RwIm2DVertex *) RxClusterGetCursorData(devVerts, void);

        /* Metrics updated inside here */
        RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST,
                                     vertices,
                                     meshData->NumVertices,
                                     ind,
                                     indices->numUsed);
    }
    else
    {
        /* Make sure the number of vertices corresponds to a whole number of triangles! */
        RwIm2DVertex *vertices;
        RwUInt32 numVertices = meshData->NumVertices;

        RWASSERT((numVertices % 3) == 0);

        vertices = (RwIm2DVertex *) RxClusterGetCursorData(devVerts, void);

        /* The vertices are in index order (we hope!!), so no need for indices
         * [metrics are updated inside here] */
        RwIm2DRenderPrimitive(rwPRIMTYPETRILIST,
                              vertices,
                              numVertices);
    }

    /* Output the packet to the first (and only) output of this Node.
     * (something like multi-pass rendering could be done by subsequent Nodes)
     */
    RxPacketDispatch(packet, 0, self);

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionGetSubmitTriangle returns a pointer to a node
 * to submit triangles to the rasteriser
 *
 * This node submits triangles to the rasteriser (it submits 2D triangles
 * defined by RxScrSpace2DVertexes and RxTriangles, through the RwIm2D
 * interface).
 *
 * The node as it is currently only supports submission of triangle lists
 * (indexed or unindexed). Currently, triangle strips and fans are assumed
 * to be expanded into triangle lists earlier in the pipeline (this occurs
 * in the standard atomic and world sector instancing nodes and also in the
 * Im3D node ImmMangleTriangleIndices.csl). Direct support for triangle strips
 * (and possibly fans) will be added later. If the Triangles cluster is not
 * present or contains no data then the vertices will be submitted as if they
 * form an unindexed triangle list. This is the most efficient way to render
 * a sequence of unconnected triangles - which, for example, is usually the
 * case with particle systems.
 *
 * If the render state cluster is not present (or contains no data) then the
 * node assumes that the packet's render state is the default render state,
 * obtained through RxRenderStateVectorGetDefaultRenderStateVector(). This is
 * used in this case to set render states prior to rasterisation.
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
 * \return A pointer to a node to submit triangles to the rasteriser on success,
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
 * \see RxNodeDefinitionGetSubmitLine
 * \see RxNodeDefinitionGetTransform
 * \see RxNodeDefinitionGetUVInterp
 *
 */

RxNodeDefinition   *
RxNodeDefinitionGetSubmitTriangle(void)
{
    static RxClusterRef N1clofinterest[] = { /* */
        {&RxClScrSpace2DVertices, rxCLALLOWABSENT, 0},
        {&RxClIndices, rxCLALLOWABSENT, 0},
        {&RxClMeshState, rxCLALLOWABSENT, 0},
        {&RxClRenderState, rxCLALLOWABSENT, 0}
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
        rxCLVALID_VALID,
        rxCLVALID_NOCHANGE,
        rxCLVALID_VALID,
        rxCLVALID_NOCHANGE
    };

    static RwChar       _SubmitOut[] = RWSTRING("SubmitOut");

    static RxOutputSpec N1outputs[] = { /* */
        {_SubmitOut,
         N1outcl1,
         rxCLVALID_NOCHANGE}
    };

#define NUMOUTPUTS \
        ((sizeof(N1outputs))/(sizeof(N1outputs[0])))

    static RwChar       _SubmitTriangle_csl[] =
        RWSTRING("SubmitTriangle.csl");

    static RxNodeDefinition nodeSubmitTriangleCSL = {
        _SubmitTriangle_csl,
        {SubmitTriangleNode,
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

    RxNodeDefinition   *result = &nodeSubmitTriangleCSL;

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionGetSubmitTriangle"));

#if (defined(RWSEATIDENTIFIER))
    {
        RwBool              status;
        RwUInt32            VolumeSerialNumber = 0;
        static RwChar      *RootPathName = NULL;

        status = _rwSeatIdentifier(RootPathName, &VolumeSerialNumber);
        RWMESSAGE(("VolumeSerialNumber %08lx status %ld",
                   (unsigned long) VolumeSerialNumber, (long) status));
    }
#endif /* (defined(RWSEATIDENTIFIER)) */

    RWRETURN(result);
}

