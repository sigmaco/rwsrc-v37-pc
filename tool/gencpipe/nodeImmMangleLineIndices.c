/*
 * nodeImmMangleLineIndices
 *
 * Conversion of indices into the format required for RwIm3D line render pipelines, on the basis of primitive type
 *
 * nodeImmMangleLineIndices
 *
 * Copyright (c) Criterion Software Limited
 */

/****************************************************************************
 *                                                                          *
 * module : nodeImmMangleLineIndices.c                                      *
 *                                                                          *
 * purpose: Converts Indices into the format required for the current       *
 *          RwIm3D render pipeline, on the basis of primitive type.         *
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

#define MESSAGE(_string)                                                   \
    RwDebugSendMessage(rwDEBUGMESSAGE, "ImmMangleLineIndices.csl", _string)

#if (defined(RWDEBUG))
#define RWCHECKSTRIDE(RxTriangles)                                                         \
    MACRO_START                                                                            \
    {                                                                                      \
        if (0< (RxTriangles)->stride)                                                      \
            MESSAGE("Indices and RxTriangles stride mismatch - extra work done copying");  \
    }                                                                                      \
    MACRO_STOP
#endif /* (defined(RWDEBUG)) */

#if (!defined(RWCHECKSTRIDE))
#define  RWCHECKSTRIDE(RxTriangles) /* No op */
#endif /* (defined(RWCHECKSTRIDE)) */

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/*****************************************************************************
 ImmMangleLineIndicesNode

 Sets up indices for a given primitive type.
 At the moment, this merely expands polylines to linelists and
 generates indices if none are passed in.
*/

static              RwBool
_ImmMangleLineIndicesNode(RxPipelineNodeInstance * self,
                          const RxPipelineNodeParam * __RWUNUSED__ params)
{
    RxCluster          *Indices = (RxCluster *)NULL;
    RxCluster          *MeshState = (RxCluster *)NULL;
    RwUInt32            numLines = 0;
    RxPacket           *packet;
    RxMeshStateVector  *MeshData;
    RwImVertexIndex    *indexData;

    RWFUNCTION(RWSTRING("_ImmMangleLineIndicesNode"));

    packet = (RxPacket *)RxPacketFetch(self);
    RWASSERT(NULL != packet);

    MeshState = RxClusterLockWrite(packet, 0, self);
    RWASSERT(NULL != MeshState);
    MeshData = RxClusterGetCursorData(MeshState, RxMeshStateVector);
    RWASSERT(NULL != MeshData);

    /* TODO: We don't want to copy Indices (they're external)
     * here, we want the resize below to do the copy! */
    Indices = RxClusterLockRead(packet, 1);
    RWASSERT(NULL != Indices);

    /* Expand indices to linelists
     * This behaviour should be superceded by specialised pipelines/nodes which
     * handle each primitive type properly */
    if (Indices->numUsed == 0)
    {
        /* We have to generate indices, this was an unindexed primitive */
        RwUInt32            numIndices, i;

        Indices = RxClusterLockWrite(packet, 1, self);
        RWASSERT(Indices != NULL);
        if (MeshData->PrimType == rwPRIMTYPELINELIST)
        {
            RWASSERT(MeshData->NumElements > 0);
            numIndices = 2 * MeshData->NumElements;
            Indices =
                RxClusterInitializeData(Indices, numIndices,
                                        sizeof(RwImVertexIndex));
            RWASSERT(NULL != Indices);

            indexData = RxClusterGetCursorData(Indices, RwImVertexIndex);
            for (i = 0; i < numIndices; i++)
            {
                *indexData = (RwImVertexIndex) i;
                indexData++;
            }
            Indices->numUsed = numIndices;
        }
        else if (MeshData->PrimType == rwPRIMTYPEPOLYLINE)
        {
            /* We need to expand to trilists */
            RWASSERT(MeshData->NumElements > 0);
            numIndices = 2 * MeshData->NumElements;
            Indices =
                RxClusterInitializeData(Indices, numIndices,
                                        sizeof(RwImVertexIndex));
            RWASSERT(NULL != Indices);

            indexData = RxClusterGetCursorData(Indices, RwImVertexIndex);
            for (i = 0; i < MeshData->NumElements; i++)
            {
                *indexData = (RwImVertexIndex) i;
                indexData++;
                *indexData = (RwImVertexIndex) (i + 1);
                indexData++;
            }
            Indices->numUsed = numIndices;

            MeshData->PrimType = rwPRIMTYPELINELIST;
        }
        else
        {
            MESSAGE("Input data was not of a recognised primitive type");
            RxPacketDispatch(packet, 0, self);
            RWRETURN(FALSE);
        }
    }
    else
    {
        if (MeshData->PrimType == rwPRIMTYPELINELIST)
        {
            /* Nothing to do but asserts, luvvly */
            RWASSERT(Indices->numUsed >= 2);
            RWASSERT((Indices->numUsed % 2) == 0);
            RWASSERT(MeshData->NumElements = Indices->numUsed >> 1);
        }
        else if (MeshData->PrimType == rwPRIMTYPEPOLYLINE)
        {
            /* We need to expand from Indices to Lines :o( */
            RwUInt32            i;
            RwImVertexIndex    *newIndices, *ind, *line;

            RWASSERT(Indices->numUsed >= 2);

            numLines = Indices->numUsed - 1;
            RWASSERT(MeshData->NumElements = numLines);

            ind = RxClusterGetIndexedData(Indices, RwImVertexIndex,
                                          numLines - 1);

            /* Resize the indices cluster (avoiding copying
             * of the original, external index data). */
            newIndices = (RwImVertexIndex *)
                RxHeapAlloc(RxPipelineNodeParamGetHeap(params),
                            sizeof(RwImVertexIndex) * numLines * 2);
            RWASSERT(NULL != newIndices);
            Indices = RxClusterSetData(Indices,
                                       newIndices,
                                       sizeof(RwImVertexIndex), numLines * 2);
            RWASSERT(Indices != NULL);

            line = RxClusterGetIndexedData(Indices, RwImVertexIndex,
                                           (numLines - 1) << 1);

            /* Now copy from end to beginning (supports copying in-place) */
            for (i = 0; i < numLines; i++)
            {
                line[1] = ind[1];
                line[0] = ind[0];

                line -= 2;
                ind--;
            }

            Indices->numUsed = numLines << 1;

            MeshData->PrimType = rwPRIMTYPELINELIST;
        }
        else
        {
            MESSAGE("Input data was not of a recognised primitive type");
            RxPacketDispatch(packet, 0, self);
            RWRETURN(FALSE);
        }
    }

    RxPacketDispatch(packet, 0, self);

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionGetImmMangleLineIndices returns a pointer
 * to a node to convert indices into the format required for RwIm3D line
 * render pipelines, on the basis of primitive type.
 *
 * This node converts a cluster of RwImVertexIndexes into a cluster appropriate
 * for the packet's primitive type. It currently expands indices for polylines
 * into line list indices and changes the primitive type to line list. It also
 * generates indices if none are passed in. When there is a comprehensive set
 * of nodes available to deal with unindexed geometry or geometry indexed in
 * polyline order then this will be changed.
 *
 * This node is the second node in the default generic RenderWare RwIm3D line
 * render pipelines.
 *
 * The node has one output.
 * The input requirements of this node:
 *
 * \verbatim
   RxClMeshState          - required
   RxClIndices            - optional
   \endverbatim
 *
 * The characteristics of this node's first output:
 *
 * \verbatim
   RxClMeshState          - valid
   RxClIndices            - valid
   \endverbatim
 *
 * \return A pointer to the node to convert the indices into the format
 * required for lines with the current pipeline, on the basis of the
 * primitive type.
 *
 * \see RxNodeDefinitionGetClipLine
 * \see RxNodeDefinitionGetClipTriangle
 * \see RxNodeDefinitionGetCullTriangle
 * \see RxNodeDefinitionGetImmInstance
 * \see RxNodeDefinitionGetImmMangleTriangleIndices
 * \see RxNodeDefinitionGetImmRenderSetup
 * \see RxNodeDefinitionGetScatter
 * \see RxNodeDefinitionGetSubmitLine
 * \see RxNodeDefinitionGetSubmitTriangle
 * \see RxNodeDefinitionGetTransform
 * \see RxNodeDefinitionGetUVInterp
 *
 */

RxNodeDefinition   *
RxNodeDefinitionGetImmMangleLineIndices(void)
{
    static RxClusterRef N1clofinterest[] = {
        {&RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClIndices, rxCLALLOWABSENT, rxCLRESERVED}
    };

#define NUMCLUSTERSOFINTEREST \
        ((sizeof(N1clofinterest))/(sizeof(N1clofinterest[0])))

    static RxClusterValidityReq N1inputreqs[NUMCLUSTERSOFINTEREST] = {
        rxCLREQ_REQUIRED,
        rxCLREQ_OPTIONAL
    };

    static RxClusterValid N1outValid[NUMCLUSTERSOFINTEREST] = {
        rxCLVALID_VALID,
        rxCLVALID_VALID
    };

    static RwChar       _ImmRenderSetupOut[] = RWSTRING("ImmRenderSetupOut");

    static RxOutputSpec N1outputs[] = {
        {_ImmRenderSetupOut,
         N1outValid,
         rxCLVALID_NOCHANGE}
    };

#define NUMOUTPUTS \
        ((sizeof(N1outputs))/(sizeof(N1outputs[0])))

    static RwChar       _ImmMangleLineIndices_csl[] =
        RWSTRING("ImmMangleLineIndices.csl");

    static RxNodeDefinition nodeImmMangleLineIndicesCSL = {
        _ImmMangleLineIndices_csl,
        {_ImmMangleLineIndicesNode,
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

    RxNodeDefinition   *result = &nodeImmMangleLineIndicesCSL;

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionGetImmMangleLineIndices"));

    /*RWMESSAGE((RWSTRING("result %p"), result)); */

    RWRETURN(result);
}
