/*
 * nodeCullTriangle
 *
 * Triangle culling in custom pipelines
 *
 * Copyright (c) Criterion Software Limited
 */

/****************************************************************************
 * module : nodeCullTriangle.c                                              *
 *                                                                          *
 * purpose: Culls triangles in 2D or 3D depending on their clip flags       *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 includes
 */

/* #include <assert.h> */

#include "rwcore.h"

#include "rpdbgerr.h"

#include "rtgncpip.h"

typedef RwBool (*CullProcessPacketFunc) (RxPipelineNodeInstance *Self, RxPacket * Packet);

/****************************************************************************
 local defines
 */

#define MESSAGE(_string)                                               \
    RwDebugSendMessage(rwDEBUGMESSAGE, "nodeCullTriangleCSL", _string)

#if 1

/*
 *  assumes triangles are 6 bytes (3 * 2)
*/
#define DELETETRIANGLE()                                                    \
MACRO_START                                                                 \
{                                                                           \
      RwUInt32  numTris = MeshData->NumElements;                            \
      RxVertexIndex *end     =                                              \
          ((RxVertexIndex *) Indices->data) +                               \
          ((numTris << 1) + numTris - 3);                                   \
      RxClusterGetCursorData(Indices, RxVertexIndex)[0] = *end++;           \
      RxClusterGetCursorData(Indices, RxVertexIndex)[1] = *end++;           \
      RxClusterGetCursorData(Indices, RxVertexIndex)[2] = *end;             \
      i--;                                                                  \
      MeshData->NumElements--;                                              \
}                                                                           \
MACRO_STOP

#else /* 1/0 */

#define DELETETRIANGLE()                                                \
  MACRO_START                                                           \
  {                                                                     \
      /*                                                                \
       * Copy the end triangle over this one and                        \
       * decrement i and NumRxTriangles                                 \
       */                                                               \
      memcpy((void *)RxClusterGetCursorData( Indices, RxVertexIndex),   \
             (void *)RxClusterGetIndexedData(Indices, RxVertexIndex,    \
                         (MeshData->NumElements - 1) * 3),              \
             Indices->stride * 3);                                      \
      i--;                                                              \
      MeshData->NumElements--;                                          \
  }                                                                     \
  MACRO_STOP

#endif /* 1/0 */

/****************************************************************************
 local (static) globals
 */

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

static              RwBool
ParallelCullProcessPacket(RxPipelineNodeInstance * Self, RxPacket * Packet)
{
    RxCluster          *DevVerts;
    RxCluster          *CamVerts;
    RxCluster          *Indices;
    RxCluster          *MeshState;
    RxMeshStateVector  *MeshData;
    RwInt32             IndicesStride;
    RxVertexIndex      *IndexPtr;
    RxVertexIndex       Index1;
    RxVertexIndex       Index2;
    RxVertexIndex       Index3;
    RwV3d              *V1;
    RwV3d              *V2;
    RwV3d              *V3;
    RwV3d               Edge1;
    RwV3d               Edge2;
    RwSplitBits         Side;
    RwUInt32            i;

    RWFUNCTION(RWSTRING("ParallelCullProcessPacket"));

    CamVerts = RxClusterLockRead(Packet, 0);
    DevVerts = RxClusterLockRead(Packet, 1);
    Indices = RxClusterLockWrite(Packet, 2, Self);
    MeshState = RxClusterLockWrite(Packet, 3, Self);

    RWASSERT((CamVerts != NULL) && (CamVerts->numUsed > 0));
    RWASSERT((DevVerts != NULL) && (DevVerts->numUsed > 0));
    RWASSERT((Indices != NULL) && (Indices->numUsed > 0));
    RWASSERT(MeshState != NULL);

    MeshData = RxClusterGetCursorData(MeshState, RxMeshStateVector);
    RWASSERT(MeshData != NULL);

    /* Parallel projection culling is the same in 2D and 3D */

    RWASSERT(MeshData->PrimType == rwPRIMTYPETRILIST);
    /* CullTriangle can only handle rwPRIMTYPETRILIST primitives as of yet */

    IndicesStride = Indices->stride;
    for (i = 0; i < MeshData->NumElements; i++)
    {
        IndexPtr = RxClusterGetCursorData(Indices, RxVertexIndex);
        Index1 = IndexPtr[0];
        Index2 = IndexPtr[1];
        Index3 = IndexPtr[2];

        V1 =
            &(RxClusterGetIndexedData
              (CamVerts, RxCamSpace3DVertex, Index1)->cameraVertex);
        V2 /* Mmm. */  =
            &(RxClusterGetIndexedData
              (CamVerts, RxCamSpace3DVertex, Index2)->cameraVertex);
        V3 =
            &(RxClusterGetIndexedData
              (CamVerts, RxCamSpace3DVertex, Index3)->cameraVertex);

        Edge1.x = V1->x - V2->x;
        Edge1.y = V1->y - V2->y;
        Edge2.x = V3->x - V2->x;
        Edge2.y = V3->y - V2->y;
        Side.nReal = Edge1.x * Edge2.y - Edge1.y * Edge2.x;

        if (Side.nInt < 0)
        {
            DELETETRIANGLE();
        }
        else
        {
            /* Onto the next entry */
            RxClusterIncCursorByStride(Indices, IndicesStride * 3);
        }
    }

    /* Compact the array if any space was freed */
    if (MeshData->NumElements < Indices->numUsed * 3)
    {
        Indices->numUsed = MeshData->NumElements * 3;
#if (0)
        /*
         * SDM: commented out Resize, coz I doubt it's beneficial -
         * in fact, if we don't compact here,
         * we will likely ensure RxTriangles cluster
         *  doesn't have to be grown during clipping
         */
        RxClusterResizeData(Indices, MeshData->NumRxTriangles * 3);
#endif /* (0) */
    }

    if (MeshData->NumElements == 0)
    {
        /* This packet's triangles have all been culled, so send
         * it to the node's second output (which is most likely
         * left unconnected so that the packet will be destroyed) */
        RxPacketDispatch(Packet, 1, Self);
    }
    else
    {
        /* Output the packet to the first (default) output of this Node */
        RxPacketDispatch(Packet, 0, Self);
    }

    RWRETURN(TRUE);
}

static              RwBool
PerspectiveCullProcessPacket(RxPipelineNodeInstance * Self, RxPacket * Packet)
{
    RxCluster          *DevVerts;
    RxCluster          *CamVerts;
    RxCluster          *Indices;
    RxCluster          *MeshState;
    RxMeshStateVector  *MeshData;
    RwInt32             IndicesStride;
    RxVertexIndex      *IndexPtr;
    RxVertexIndex       Index1;
    RxVertexIndex       Index2;
    RxVertexIndex       Index3;
    RxScrSpace2DVertex *DevVert1;
    RxScrSpace2DVertex *DevVert2;
    RxScrSpace2DVertex *DevVert3;
    RxCamSpace3DVertex *CamVert1;
    RxCamSpace3DVertex *CamVert2;
    RxCamSpace3DVertex *CamVert3;
    RwV3d              *V1;
    RwV3d              *V2;
    RwV3d              *V3;
    RwUInt8             c1;
    RwUInt8             c2;
    RwUInt8             c3;
    RwV3d               Edge1;
    RwV3d               Edge2;
    RwV3d               Normal;
    RwSplitBits         Side;
    RwUInt32            i;

    RWFUNCTION(RWSTRING("PerspectiveCullProcessPacket"));

    CamVerts = RxClusterLockRead(Packet, 0);
    DevVerts = RxClusterLockRead(Packet, 1);
    Indices = RxClusterLockWrite(Packet, 2, Self);
    MeshState = RxClusterLockWrite(Packet, 3, Self);

    RWASSERT((CamVerts != NULL) && (CamVerts->numUsed > 0));
    RWASSERT((DevVerts != NULL) && (DevVerts->numUsed > 0));
    RWASSERT((Indices != NULL) && (Indices->numUsed > 0));
    RWASSERT(MeshState != NULL);

    MeshData = RxClusterGetCursorData(MeshState, RxMeshStateVector);
    RWASSERT(MeshData != NULL);

    IndicesStride = Indices->stride;

    RWASSERT(MeshData->PrimType == rwPRIMTYPETRILIST);
    /* CullTriangle can only handle rwPRIMTYPETRILIST primitives as of yet */

    if (MeshData->ClipFlagsOr == 0)
    {
        /* All the triangles are all fully onscreen */

        for (i = 0; i < MeshData->NumElements; i++)
        {
            IndexPtr = RxClusterGetCursorData(Indices, RxVertexIndex);
            Index1 = IndexPtr[0];
            Index2 = IndexPtr[1];
            Index3 = IndexPtr[2];

            DevVert1 =
                RxClusterGetIndexedData(DevVerts, RxScrSpace2DVertex, Index1);
            DevVert2 =
                RxClusterGetIndexedData(DevVerts, RxScrSpace2DVertex, Index2);
            DevVert3 =
                RxClusterGetIndexedData(DevVerts, RxScrSpace2DVertex, Index3);

            /* We can cull in screen space, 'cos we know it's been projected */
            Edge1.x =
                RwIm2DVertexGetScreenX(DevVert1) -
                RwIm2DVertexGetScreenX(DevVert2);
            Edge1.y =
                RwIm2DVertexGetScreenY(DevVert1) -
                RwIm2DVertexGetScreenY(DevVert2);
            Edge2.x =
                RwIm2DVertexGetScreenX(DevVert3) -
                RwIm2DVertexGetScreenX(DevVert2);
            Edge2.y =
                RwIm2DVertexGetScreenY(DevVert3) -
                RwIm2DVertexGetScreenY(DevVert2);
            Side.nReal = Edge1.x * Edge2.y - Edge1.y * Edge2.x;

            if (Side.nInt <= 0)
            {
                DELETETRIANGLE();
            }
            else
            {

                /* Onto the next entry */
                RxClusterIncCursorByStride(Indices, IndicesStride * 3);
            }
        }
    }
    else
    {
        /* Some triangles are at least partially off screen */

        for (i = 0; i < MeshData->NumElements; i++)
        {
            IndexPtr = RxClusterGetCursorData(Indices, RxVertexIndex);
            Index1 = IndexPtr[0];
            Index2 = IndexPtr[1];
            Index3 = IndexPtr[2];

            CamVert1 =
                RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex, Index1);
            CamVert2 =
                RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex, Index2);
            CamVert3 =
                RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex, Index3);
            c1 = CamVert1->clipFlags;
            c2 = CamVert2->clipFlags;
            c3 = CamVert3->clipFlags;

            /* Completely off screen? */
            if (!(c1 & c2 & c3))
            {
                /* Need to clip? */
                if (c1 | c2 | c3)
                {
                    /*
                     * * Need to cull in camera space -
                     * * it's partially off screen
                     */
                    V1 = &(CamVert1->cameraVertex);
                    V2 /* Mmm. */  = &(CamVert2->cameraVertex);
                    V3 = &(CamVert3->cameraVertex);
                    RwV3dSub(&Edge1, V1, V2);
                    RwV3dSub(&Edge2, V3, V2);
                    RwV3dCrossProduct(&Normal, &Edge1, &Edge2);
                    Side.nReal = RwV3dDotProduct(&Normal, V1);

                    if (Side.nInt <= 0)
                    {
                        {
                            RwUInt32            numTris =

                                MeshData->NumElements;
                            RxVertexIndex      *end =
                                ((RxVertexIndex *) Indices->data) +
                                ((numTris << 1) + numTris - 3);

                            RxClusterGetCursorData(Indices,
                                                   RxVertexIndex)[0] = *end++;
                            RxClusterGetCursorData(Indices,
                                                   RxVertexIndex)[1] = *end++;
                            RxClusterGetCursorData(Indices,
                                                   RxVertexIndex)[2] = *end;
                            i--;
                            MeshData->NumElements--;
                        }

                        /* DELETETRIANGLE(); */
                    }
                    else
                    {
                        /* Onto the next entry */
                        RxClusterIncCursorByStride(Indices,
                                                   IndicesStride * 3);
                    }
                }
                else
                {
                    /* It's been projected, so cull it in screen space */
                    DevVert1 =
                        RxClusterGetIndexedData(DevVerts, RxScrSpace2DVertex,
                                                Index1);
                    DevVert2 =
                        RxClusterGetIndexedData(DevVerts, RxScrSpace2DVertex,
                                                Index2);
                    DevVert3 =
                        RxClusterGetIndexedData(DevVerts, RxScrSpace2DVertex,
                                                Index3);
                    Edge1.x =
                        RwIm2DVertexGetScreenX(DevVert1) -
                        RwIm2DVertexGetScreenX(DevVert2);
                    Edge1.y =
                        RwIm2DVertexGetScreenY(DevVert1) -
                        RwIm2DVertexGetScreenY(DevVert2);
                    Edge2.x =
                        RwIm2DVertexGetScreenX(DevVert3) -
                        RwIm2DVertexGetScreenX(DevVert2);
                    Edge2.y =
                        RwIm2DVertexGetScreenY(DevVert3) -
                        RwIm2DVertexGetScreenY(DevVert2);
                    Side.nReal = Edge1.x * Edge2.y - Edge1.y * Edge2.x;

                    if (Side.nInt <= 0)
                    {
                        {
                            RwUInt32            numTris =

                                MeshData->NumElements;
                            RxVertexIndex      *end =
                                ((RxVertexIndex *) Indices->data) +
                                ((numTris << 1) + numTris - 3);

                            RxClusterGetCursorData(Indices,
                                                   RxVertexIndex)[0] = *end++;
                            RxClusterGetCursorData(Indices,
                                                   RxVertexIndex)[1] = *end++;
                            RxClusterGetCursorData(Indices,
                                                   RxVertexIndex)[2] = *end;
                            i--;
                            MeshData->NumElements--;
                        }
                        /* DELETETRIANGLE(); */
                    }
                    else
                    {
                        /* Onto the next entry */
                        RxClusterIncCursorByStride(Indices,
                                                   IndicesStride * 3);
                    }
                }
            }
            else
            {
                /* Completely offscreen */
                DELETETRIANGLE();
            }
        }
    }

    /* Compact the array if any space was freed */
    if (MeshData->NumElements < Indices->numUsed * 3)
    {
        Indices->numUsed = MeshData->NumElements * 3;
#if (0)
        /*
         * SDM: commented out Resize, coz I doubt it's beneficial -
         * in fact, if we don't compact here,
         * we will likely ensure RxTriangles cluster
         *  doesn't have to be grown during clipping
         */
        RxClusterResizeData(RxTriangles, MeshData->NumRxTriangles);
#endif /* (0) */
    }

    if (MeshData->NumElements == 0)
    {
        /* This packet's triangles have all been culled, so send
         * it to the node's second output (which is most likely
         * left unconnected so that the packet will be destroyed) */
        RxPacketDispatch(Packet, 1, Self);
    }
    else
    {
        /* Output the packet to the first (default) output of this Node */
        RxPacketDispatch(Packet, 0, Self);
    }

    RWRETURN(TRUE);
}

/****************************************************************************
 CullNode

 Perform 2D or 3D culling on the basis of triangle clipflags and remove all
 culled triangles (that's back-faced OR completely offscreen).

 on entry: Current PipelineNodeInstance,
         : heap (still vestigial MmoryArena currently),
         : (void *)data (owning object of pipeline usually, or somethign related to it)
 on exit : TRUE or FALSE
*/

static              RwBool
_CullTriangleNode( RxPipelineNodeInstance * self,
                   const RxPipelineNodeParam * params __RWUNUSED__ )
{
    RwBool              result = TRUE;
    RwUInt32            persp;
    RxPacket           *packet;
    CullProcessPacketFunc func;
    static CullProcessPacketFunc CullFunc[2] = {
        ParallelCullProcessPacket,
        PerspectiveCullProcessPacket
    };

    RWFUNCTION(RWSTRING("_CullTriangleNode"));

    persp = (rwPERSPECTIVE ==
             ((RwCamera *) RWSRCGLOBAL(curCamera))->projectionType) ? 1 : 0;

    func = CullFunc[persp];

    packet = (RxPacket *)RxPacketFetch(self);
    RWASSERT(NULL != packet);

    result = func(self, packet);

    /* RWCRTCHECKMEMORY(); */

    RWRETURN(result);
}

/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionGetCullTriangle returns a pointer to a node
 * implementing triangle culling in custom pipelines.
 *
 * This node removes triangles from the indices cluster if they are
 * back-facing with respect to the current camera.
 *
 * This node is assumed to be used after a transform node, so that the
 * screen-space and camera-space vertex clusters have been filled with
 * valid data and the clip flags of the camera-space vertices have been
 * set. For triangles wholly onscreen (the clip flags are used to determine
 * when this is the case), or parallel-projected, 2D back-face culling is
 * performed, otherwise 3D culling is performed (the former is faster).
 * Triangles wholly offscreen are also deleted - this case is detected
 * conservatively if the bitwise AND of the clipflags of all three vertices
 * of a triangle is non-zero (it means that all three vertices lie beyond
 * the same one or more planes of the frustum). Triangles found to be culled
 * are deleted from the triangles cluster (the way this is done changes the
 * order of the triangles, which is assumed to be unimportant).
 *
 * If all the triangles in a packet are culled then the packet is sent to
 * the node's second output (which is usually left disconnected, resulting
 * in the packet's destruction. In some cases, however, it may be desirable
 * to connect this output to subsequent nodes if the geometry's extent is
 * known to be modified further down the pipeline if, for example, vertex
 * normals are added, to be rendered as lines), but otherwise packets are
 * sent to the node's first output.
 *
 * The node has two outputs. Packets in which all triangles are culled are
 * sent to the second output.
 * The input requirements of this node:
 *
 * \verbatim
   RxClCamSpace3DVertices - required
   RxClScrSpace2DVertices - required
   RxClIndices            - required
   RxClMeshState          - required
   \endverbatim
 *
 * The characteristics of the first of this node's outputs:
 *
 * \verbatim
   RxClCamSpace3DVertices - no change
   RxClScrSpace2DVertices - no change
   RxClIndices            - valid
   RxClMeshState          - valid
   \endverbatim
 *
 * The characteristics of the second of this node's outputs:
 *
 * \verbatim
   RxClCamSpace3DVertices - no change
   RxClScrSpace2DVertices - no change
   RxClIndices            - invalid
   RxClMeshState          - valid
   \endverbatim
 *
 * \return A pointer to the node for triangle culling in custom pipelines or
 *  NULL if there is an error
 *
 * \see RxNodeDefinitionGetClipLine
 * \see RxNodeDefinitionGetClipTriangle
 * \see RxNodeDefinitionGetImmInstance
 * \see RxNodeDefinitionGetImmMangleLineIndices
 * \see RxNodeDefinitionGetImmMangleTriangleIndices
 * \see RxNodeDefinitionGetImmRenderSetup
 * \see RxNodeDefinitionGetScatter
 * \see RxNodeDefinitionGetSubmitLine
 * \see RxNodeDefinitionGetSubmitTriangle
 * \see RxNodeDefinitionGetTransform
 * \see RxNodeDefinitionGetUVInterp
 */

RxNodeDefinition   *
RxNodeDefinitionGetCullTriangle(void)
{

    static RxClusterRef N1clofinterest[] = {
        {&RxClCamSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClScrSpace2DVertices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClIndices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED}
    };

#define NUMCLUSTERSOFINTEREST \
        ((sizeof(N1clofinterest))/(sizeof(N1clofinterest[0])))

    static RxClusterValidityReq N1inputreqs[NUMCLUSTERSOFINTEREST] = {
        rxCLREQ_REQUIRED,
        rxCLREQ_REQUIRED,
        rxCLREQ_REQUIRED,
        rxCLREQ_REQUIRED
    };

    static RxClusterValid N1outcl1[NUMCLUSTERSOFINTEREST] = {
        rxCLVALID_NOCHANGE,
        rxCLVALID_NOCHANGE,
        rxCLVALID_VALID,
        rxCLVALID_VALID
    };

    static RxClusterValid N1outcl2[NUMCLUSTERSOFINTEREST] = {
        rxCLVALID_NOCHANGE,
        rxCLVALID_NOCHANGE,
        rxCLVALID_INVALID,
        rxCLVALID_VALID
    };

    static RwChar       _CullTriangleOut[] = RWSTRING("CullTriangleOut");
    static RwChar       _CullTriangleAllCulled[] =
        RWSTRING("CullTriangleAllCulled");

    static RxOutputSpec N1outputs[] = {
        {_CullTriangleOut,
         N1outcl1,
         rxCLVALID_NOCHANGE},
        {_CullTriangleAllCulled,
         N1outcl2,
         rxCLVALID_NOCHANGE}
    };

#define NUMOUTPUTS \
        ((sizeof(N1outputs))/(sizeof(N1outputs[0])))

    static RwChar       _CullTriangle_csl[] = RWSTRING("CullTriangle.csl");

    static RxNodeDefinition nodeCullTriangleCSL = {
        _CullTriangle_csl,
        {_CullTriangleNode,
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

    RxNodeDefinition   *result = &nodeCullTriangleCSL;

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionGetCullTriangle"));

    /*RWMESSAGE((RWSTRING("result %p"), result)); */

    RWRETURN(result);

}
