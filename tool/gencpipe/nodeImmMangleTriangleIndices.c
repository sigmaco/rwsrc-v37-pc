/*
 * nodeImmMangleTriangleIndices
 *
 * Conversion of indices into the format required for RwIm3D triangle render pipelines, on the basis of primitive type
 *
 * (c) Criterion Software Limited
 */

/****************************************************************************
 *                                                                          *
 * module : nodeImmMangleTriangleIndices.c                                  *
 *                                                                          *
 * purpose: Converts Indices into the format required for the current       *
 *          RwIm3D triangle render pipeline, on the basis of primitive type.*
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

#define MESSAGE(_string)                                \
    RwDebugSendMessage(rwDEBUGMESSAGE,                  \
                       "ImmMangleTriangleIndices.csl",  \
                       _string)

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
 ImmMangleTriangleIndicesNode

 Sets up indices for a given primitive type.

 At the moment, this merely expands tristrip/trifan/polyline indices and
 changes the primitive type to trilist or linelist, or generates indices
 if none are passed in.
*/

static              RwBool
_ImmMangleTriangleIndicesNode(RxPipelineNodeInstance * self,
                              const RxPipelineNodeParam * params)
{
    RxCluster          *MeshState = (RxCluster *)NULL;
    RxCluster          *Indices = (RxCluster *)NULL;
    RwUInt32            numRxTriangles = 0;
    RxPacket           *packet;
    RxMeshStateVector  *MeshData;
    RwImVertexIndex    *indexData;

    RWFUNCTION(RWSTRING("_ImmMangleTriangleIndicesNode"));

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

    /* Expand indices to either
     * + trilists (for triangles) or
     * + linelists (for lines)
     * This behaviour should be superceded by specialised
     * pipelines/nodes which handle each primitive type properly */
    if (Indices->numUsed == 0)
    {
        /* We have to generate indices, this was an unindexed primitive */
        RwUInt32            numIndices, i;

        Indices = RxClusterLockWrite(packet, 1, self);
        RWASSERT(Indices != NULL);
        switch (MeshData->PrimType)
        {
            case rwPRIMTYPETRILIST:
                {
                    RWASSERT(MeshData->NumElements > 0);
                    numIndices = 3 * MeshData->NumElements;
                    Indices =
                        RxClusterInitializeData(Indices, numIndices,
                                                sizeof(RwImVertexIndex));
                    RWASSERT(NULL != Indices);

                    indexData =
                        RxClusterGetCursorData(Indices, RwImVertexIndex);
                    for (i = 0; i < numIndices; i++)
                    {
                        *indexData = (RwImVertexIndex) i;
                        indexData++;
                    }
                    Indices->numUsed = numIndices;

                    break;
                }
            case rwPRIMTYPETRIFAN:
                {
                    /* We need to expand to trilists */
                    RWASSERT(MeshData->NumElements > 0);
                    numIndices = 3 * MeshData->NumElements;
                    Indices =
                        RxClusterInitializeData(Indices, numIndices,
                                                sizeof(RwImVertexIndex));
                    RWASSERT(NULL != Indices);

                    indexData =
                        RxClusterGetCursorData(Indices, RwImVertexIndex);
                    for (i = 0; i < MeshData->NumElements; i++)
                    {
                        RwUInt32            j = i + 1;

                        *indexData = 0;
                        indexData++;
                        *indexData = (RwImVertexIndex) j++;
                        indexData++;
                        *indexData = (RwImVertexIndex) j;
                        indexData++;
                    }
                    Indices->numUsed = numIndices;

                    MeshData->PrimType = rwPRIMTYPETRILIST;

                    break;
                }
            case rwPRIMTYPETRISTRIP:
                {
                    /* We need to expand to trilists */
                    RWASSERT(MeshData->NumElements > 0);
                    numIndices = 3 * MeshData->NumElements;
                    Indices =
                        RxClusterInitializeData(Indices, numIndices,
                                                sizeof(RwImVertexIndex));
                    RWASSERT(NULL != Indices);

                    indexData =
                        RxClusterGetCursorData(Indices, RwImVertexIndex);
                    for (i = 0; i < MeshData->NumElements; i++)
                    {
                        *indexData = (RwImVertexIndex) (i + (i & 1));
                        indexData++;
                        *indexData = (RwImVertexIndex) (i + ((i & 1) ^ 1));
                        indexData++;
                        *indexData = (RwImVertexIndex) (i + 2);
                        indexData++;
                    }
                    Indices->numUsed = numIndices;

                    MeshData->PrimType = rwPRIMTYPETRILIST;

                    break;
                }
            default:
                MESSAGE("Input data was not of a recognised primitive type");
                RxPacketDispatch(packet, 0, self);
                RWRETURN(FALSE);
                break;
        }
    }
    else
    {
        switch (MeshData->PrimType)
        {
            case rwPRIMTYPETRILIST:
                {
                    /* Nothing to do but asserts, luvvly */
                    RWASSERT(Indices->numUsed >= 3);
                    RWASSERT((Indices->numUsed % 3) == 0);
                    RWASSERT(MeshData->NumElements = Indices->numUsed / 3);

                    break;
                }
            case rwPRIMTYPETRIFAN:
                {
                    /* We need to expand to trilists :o( */
                    RwUInt32            i;
                    RwImVertexIndex    *newIndices, ind0, *oldInd, *newInd;

                    RWASSERT(Indices->numUsed >= 3);

                    numRxTriangles = Indices->numUsed - 2;
                    RWASSERT(MeshData->NumElements = numRxTriangles);

                    oldInd = RxClusterGetIndexedData(Indices,
                                                     RwImVertexIndex,
                                                     Indices->numUsed - 2);
                    ind0 = *RxClusterGetCursorData(Indices, RwImVertexIndex);

                    /* Resize the indices cluster (avoiding copying
                     * of the original, external index data). */
                    newIndices = (RwImVertexIndex *)
                        RxHeapAlloc(RxPipelineNodeParamGetHeap(params),
                                    sizeof(RwImVertexIndex) * numRxTriangles *
                                    3);
                    RWASSERT(NULL != newIndices);
                    Indices = RxClusterSetData(Indices,
                                               newIndices,
                                               sizeof(RwImVertexIndex),
                                               numRxTriangles * 3);
                    RWASSERT(Indices != NULL);

                    newInd =
                        RxClusterGetIndexedData(Indices, RwImVertexIndex,
                                                (numRxTriangles - 1) * 3);

                    /* Now copy from end to beginning (supports copying in-place) */
                    for (i = 0; i < numRxTriangles; i++)
                    {
                        newInd[2] = oldInd[1];
                        newInd[1] = oldInd[0];
                        newInd[0] = ind0;

                        newInd -= 3;
                        oldInd--;
                    }

                    Indices->numUsed = numRxTriangles * 3;

                    MeshData->PrimType = rwPRIMTYPETRILIST;

                    break;
                }
            case rwPRIMTYPETRISTRIP:
                {
                    /* We need to expand from trilists :o( */
                    RwUInt32            i, flip;
                    RwImVertexIndex    *newIndices, ind0, ind1, *oldInd,

                        *newInd;

                    RWASSERT(Indices->numUsed >= 3);

                    numRxTriangles = Indices->numUsed - 2;
                    RWASSERT(MeshData->NumElements = numRxTriangles);

                    oldInd = RxClusterGetIndexedData(Indices,
                                                     RwImVertexIndex,
                                                     Indices->numUsed - 3);

                    /* Resize the indices cluster (avoiding copying
                     * of the original, external index data). */
                    newIndices = (RwImVertexIndex *)
                        RxHeapAlloc(RxPipelineNodeParamGetHeap(params),
                                    sizeof(RwImVertexIndex) * numRxTriangles *
                                    3);
                    RWASSERT(NULL != newIndices);
                    Indices = RxClusterSetData(Indices,
                                               newIndices,
                                               sizeof(RwImVertexIndex),
                                               numRxTriangles * 3);
                    RWASSERT(Indices != NULL);

                    newInd =
                        RxClusterGetIndexedData(Indices, RwImVertexIndex,
                                                (numRxTriangles - 1) * 3);

                    /* Now copy from end to beginning (supports copying in-place) */
                    flip = (numRxTriangles & 1) ? (0x01) : (0x0);

                    for (i = 0; i < numRxTriangles; i++)
                    {
                        /* Save the old value to prevent over writing as we approach the head */
                        ind1 = oldInd[0 ^ flip];
                        ind0 = oldInd[1 ^ flip];

                        newInd[2] = oldInd[2];
                        newInd[1] = ind1;
                        newInd[0] = ind0;

                        newInd -= 3;
                        oldInd--;

                        flip ^= 0x01;
                    }

                    Indices->numUsed = numRxTriangles * 3;

                    MeshData->PrimType = rwPRIMTYPETRILIST;

                    break;
                }
            default:
                MESSAGE("Input data was not of a recognised primitive type");
                RxPacketDispatch(packet, 0, self);
                RWRETURN(FALSE);
                break;
        }
    }

    RxPacketDispatch(packet, 0, self);

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionGetImmMangleTriangleIndices returns a pointer
 * to a node to convert indices into the format required for RwIm3D triangle
 * render pipelines, on the basis of primitive type.
 *
 * This node converts a cluster of RwImVertexIndexs into a cluster appropriate
 * for the packet's primitive type. It currently expands indices for triangle
 * strips and fans into triangle list indices and changes the primitive type
 * to triangle list. It also generates indices if none are passed in. When
 * there is a comprehensive set of nodes available to deal with unindexed
 * geometry or geometry indexed in triangle strip or triangle fan order then
 * this will be changed (clipping and culling triangle strips and fans is
 * quite difficult, hence these nodes are not yet available).
 *
 * This node is the second node in the default generic RenderWare RwIm3D
 * triangle render pipelines.
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
 * required for triangles with the current pipeline, on the basis of the
 * primitive type.
 *
 * \see RxNodeDefinitionGetClipLine
 * \see RxNodeDefinitionGetClipTriangle
 * \see RxNodeDefinitionGetCullTriangle
 * \see RxNodeDefinitionGetImmInstance
 * \see RxNodeDefinitionGetImmMangleLineIndices
 * \see RxNodeDefinitionGetImmRenderSetup
 * \see RxNodeDefinitionGetScatter
 * \see RxNodeDefinitionGetSubmitLine
 * \see RxNodeDefinitionGetSubmitTriangle
 * \see RxNodeDefinitionGetTransform
 * \see RxNodeDefinitionGetUVInterp
 *
 */

RxNodeDefinition   *
RxNodeDefinitionGetImmMangleTriangleIndices(void)
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

    /* Need a new output to output lines (lines VALID triangles INVALID)
     * so we can use one node for tris and lines... */

    static RwChar       _ImmRenderSetupOut[] = RWSTRING("ImmRenderSetupOut");

    static RxOutputSpec N1outputs[] = { /* */
        {_ImmRenderSetupOut,
         N1outValid,
         rxCLVALID_NOCHANGE}
    };

#define NUMOUTPUTS \
        ((sizeof(N1outputs))/(sizeof(N1outputs[0])))

    static RwChar       _NodeName[] =
        RWSTRING("ImmMangleTriangleIndices.csl");

    static RxNodeDefinition NodeDefinition =
    {
        _NodeName,
        {_ImmMangleTriangleIndicesNode,
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

    RxNodeDefinition   *result = &NodeDefinition;

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionGetImmMangleTriangleIndices"));

    /*RWMESSAGE((RWSTRING("result %p"), result)); */

    RWRETURN(result);
}
