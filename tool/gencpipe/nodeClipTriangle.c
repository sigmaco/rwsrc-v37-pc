/*
 * nodeClipTriangle
 *
 * Triangle clipping in custom pipelines
 *
 * Copyright (c) Criterion Software Limited
 */

/***************************************************************************
 * Module  : nodeClipTriangle.c                                            *
 *                                                                         *
 * Purpose : Code for clipping as used by Pipeline II                      *
 *                                                                         *
 **************************************************************************/

/****************************************************************************
 Includes
 */

/* Pick up memcpy prototype */
#include <string.h>
#if (defined(_MSC_VER))
#if (_MSC_VER>=1000)
#include <memory.h>
#endif /* (_MSC_VER>=1000) */
#endif /* (defined(_MSC_VER)) */

#include "rwcore.h"

#include <rpdbgerr.h>

#include "p2clpcom.h"
#include "rtgncpip.h"


/****************************************************************************
 local types
 */

typedef             RwBool
    (*ClipProcessPacketFunc)

    (RxPipelineNodeInstance * Self, RxPacket * Packet);

/****************************************************************************
 Local defines
 */

#define MESSAGE(_string)                                              \
    RwDebugSendMessage(rwDEBUGMESSAGE, "nodeClipTriangleCSL", _string)

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                                Functions

 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/*
 * 1. Have the clipper node optionally create a cluster (depending on
 *    whether it's needed by subsequent Nodes) of interpolation data (see
 *    below) which is passed on to...
 *
 * 2. ...a RxNodeInterpRxUVAndSwap() Node which uses the data to do clipping for
 *    a given set of RxUVs (one such cluster per pass, with an associated
 *    RenderState cluster) and saves the results in the DevVerts of a packet
 *    (the packet contains pre-clipping vertices and (after them,
 *    contiguous) clipping-generated vertices) for resubmission by a
 *    subsequent submit Node.
 *
 * struct _RxInterp
 * {
 *   RxVertexIndex originalVert;
 *   RxVertexIndex parentVert1, parentVert2;
 *   RwReal  interp;
 * };
 */

static              RwBool
PerspectiveClipProcessPacket(RxPipelineNodeInstance * Self, RxPacket * Packet)
{
    /*
     * o We check that each polygon needs clipping (i.e, cOr != 0), but that it
     *   is not obviously clipped off screen beyond one plane (i.e cAnd == 0).
     *   + We assume that we will never generate more than 12 vertices (two per
     *     plane, with six intersected planes max) and 8 triangles (a triangle
     *     can potentially intersect all six planes and each plane can cut off
     *     a corner adding one edge and two vertices, leaving a 9-sided poly;
     *     the  original triangle can be overwritten) per triangle.
     *
     * o What we have is a circular buffer of vertex indices.
     *   + Clipped vertices are appended to CamVerts and DevVerts and indices
     *     are put into the circular buffer as generated.
     *   + Clipped polygon indexes are placed directly after the non clipped
     *     polygon indexes, and the list is made circular by modulo
     *     arithmetic on the list indices.
     *   + For each plane clipped against, you generate a complete new list of
     *     indices from the existing one, so that you can keep the indices in
     *     thecorrect winding order and insert new indices as new edges are
     *     generated.
     *   + When you've finished clipping the polygon against a particular
     *     plane, because you were putting the indices after what you were
     *     clippingfrom, you just carry on, no messing about with pointers.
     *   + cOr is generated between the get and put pointers for polygon
     *     indices.
     */

    RwBool              result = TRUE;
    static const RxClipFuncs PerspClipFuncs = {
        _rwGeneratePerspClippedVertexZLO,
        _rwGeneratePerspClippedVertexZHI,
        _rwGeneratePerspClippedVertexYLO,
        _rwGeneratePerspClippedVertexYHI,
        _rwGeneratePerspClippedVertexXLO,
        _rwGeneratePerspClippedVertexXHI
    };
    RxCluster          *CamVerts;
    RxCluster          *DevVerts;
    RxCluster          *Indices;
    RxCluster          *MeshState;
    RxCluster          *RenderState;
    RxCluster          *RxInterpolants;
    RwInt32             IndicesStride;
    RxMeshStateVector  *MeshData;
    RxRenderStateVector *rsvp;
    RwUInt32            NumRxTriangles;
    RwUInt32            OldNumRxTriangles;
    RwUInt32            OldNumVertices;
    RxVertexIndex      *OriginalIndices;
    RxVertexIndex      *IndexPtr;
    RxVertexIndex       Index1;
    RxVertexIndex       Index2;
    RxVertexIndex       Index3;
    RxCamSpace3DVertex *CamVert1;
    RxCamSpace3DVertex *CamVert2;
    RxCamSpace3DVertex *CamVert3;
    RxScrSpace2DVertex *DevVert1;
    RwInt32             i, j;
    RwUInt32            Tri;
    RxVertexIndex       nClipBuffer[16];
    RwInt32             nPntInVerts, nPntOutVerts;
    RwUInt8             cOr, cAnd;
    RxVertexIndex       oldFirstVertex, firstFreeVertex;
    RxVertexIndex       pivot;  /* For fanning */

    RWFUNCTION(RWSTRING("PerspectiveClipProcessPacket"));

    CamVerts = RxClusterLockWrite(Packet, 0, Self);
    DevVerts = RxClusterLockWrite(Packet, 1, Self);
    Indices = RxClusterLockWrite(Packet, 2, Self);
    MeshState = RxClusterLockWrite(Packet, 3, Self);
    RenderState = RxClusterLockRead(Packet, 4);
    if ((RenderState != NULL) && (RenderState->data != NULL))
    {
        rsvp = RxClusterGetCursorData(RenderState, RxRenderStateVector);
    }
    else
    {
        rsvp = &RXPIPELINEGLOBAL(defaultRenderState);
    }
    RxInterpolants = RxClusterLockWrite(Packet, 5, Self);

    RWASSERT((CamVerts != NULL) && (CamVerts->numUsed > 0));
    RWASSERT((DevVerts != NULL) && (DevVerts->numUsed > 0));
    RWASSERT((Indices != NULL) && (Indices->numUsed > 0));
    RWASSERT((MeshState != NULL) && (MeshState->numUsed > 0));

    MeshData = RxClusterGetCursorData(MeshState, RxMeshStateVector);
    RWASSERT(MeshData->PrimType == rwPRIMTYPETRILIST);
    /* ClipTriangle can only handle rwPRIMTYPETRILIST primitives as of yet */

    IndicesStride = Indices->stride;

    if (RxInterpolants)
    {
        /* Empty it out if (for some reason) it has any contents */
        RxInterpolants->stride = sizeof(RxInterp);
        RxInterpolants->numUsed = 0;
    }

    if (MeshData->ClipFlagsOr)
    {
        OldNumRxTriangles = MeshData->NumElements;

        /*
         * NOTE:
         * StateData is invalidated during this loop and
         * corrected after it finishes
         */
        for (Tri = 0; Tri < OldNumRxTriangles; Tri++)
        {

            IndexPtr = RxClusterGetCursorData(Indices, RxVertexIndex);
            Index1 = IndexPtr[0];
            Index2 = IndexPtr[1];
            Index3 = IndexPtr[2];

            /* Oi!! You're accessing the CamVerts array in random-access
             * order just to retrieve ClipFlags here! Dude!! */

            CamVert1 =
                RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex, Index1);
            CamVert2 =
                RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex, Index2);
            CamVert3 =
                RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex, Index3);

            OldNumVertices = CamVerts->numUsed;
            firstFreeVertex = (RxVertexIndex)OldNumVertices;

            /* Set up cOr and cAnd */
            cOr =
                CamVert1->clipFlags | CamVert2->
                clipFlags | CamVert3->clipFlags;
            cAnd =
                CamVert1->clipFlags & CamVert2->
                clipFlags & CamVert3->clipFlags;

            NumRxTriangles = 0;
            if (cOr)
            {
                if (cAnd)
                {
                    goto ClippedOut;
                }

                /* Check we're not running out of space for
                 * new triangles */
                if (Indices->numUsed + 24 > Indices->numAlloced)
                {
                    /* TODO: Is this a good estimate? */
                    i = Indices->numAlloced + (OldNumRxTriangles - Tri) + 24;
                    result = (NULL != RxClusterResizeData(Indices, i));
                    if (!result)
                    {
                        goto ClipEnd;
                    }
                    /*
                     * Resizing the cluster resets the
                     * CurrentData pointer (and a realloc might
                     * cause a copy, too), so restore it here
                     */
                    Indices->currentData =
                        RxClusterGetIndexedData(Indices,
                                                RxVertexIndex, Tri * 3);
                }
                /* Check we're not running out of space for new vertices */
                if (CamVerts->numUsed + 12 > CamVerts->numAlloced)
                {
                    /* TODO: Is this a good estimate? */
                    i = CamVerts->numAlloced + (OldNumRxTriangles - Tri) + 12;
                    result = (NULL != RxClusterResizeData(CamVerts, i));
                    if (!result)
                    {
                        goto ClipEnd;
                    }
                }
                if (DevVerts->numUsed + 12 > DevVerts->numAlloced)
                {
                    /* TODO: Is this a good estimate? */
                    i = DevVerts->numAlloced + (OldNumRxTriangles - Tri) + 12;
                    result = (NULL != RxClusterResizeData(DevVerts, i));
                    if (!result)
                    {
                        goto ClipEnd;
                    }
                }
                if (RxInterpolants && (RxInterpolants->numUsed + 12 >
                                       RxInterpolants->numAlloced))
                {
                    void               *resultPtr;

                    /* TODO: Is this a good estimate? */
                    i = RxInterpolants->numAlloced +
                        (OldNumRxTriangles - Tri) + 12;
                    if (RxInterpolants->numAlloced <= 0)
                    {
                        resultPtr =
                            RxClusterInitializeData(RxInterpolants, i,
                                                    sizeof(RxInterp));
                    }
                    else
                    {
                        resultPtr = RxClusterResizeData(RxInterpolants, i);
                    }
                    if (resultPtr == NULL)
                    {
                        result = FALSE;
                        goto ClipEnd;
                    }
                    /* Put the pointer back at the first free spot */
                    RxInterpolants->currentData =
                        RxClusterGetIndexedData(RxInterpolants,
                                                RxInterp,
                                                RxInterpolants->numUsed);
                }

                OriginalIndices = (RxVertexIndex *) (Indices->currentData);

                /*
                 * This way we can restore the thirs vertex color for
                 * a flat shaded clipped polygon
                 */
                oldFirstVertex = Index3;

                /* Set it up */
                nPntInVerts = 0;
                nClipBuffer[0] = Index1;
                nClipBuffer[1] = Index2;
                nClipBuffer[2] = Index3;

                nPntOutVerts = 3;

                if (cOr & (rwZLOCLIP | rwZHICLIP))
                {

                    /***********************  Z LO ***********************/
                    if (cOr & (RwUInt8) rwZLOCLIP)
                    {
                        if (_rwForAllEdges
                            (CamVerts, DevVerts, RxInterpolants,
                             nClipBuffer, &nPntInVerts,
                             &nPntOutVerts, rwZLOCLIP,
                             PerspClipFuncs.ZLOFunc) < 3)
                        {
                            /* Clipped out */
                            goto ClippedOut;
                        }
                    }

                    /***********************  Z HI ***********************/
                    if (cOr & (RwUInt8) rwZHICLIP)
                    {
                        if (_rwForAllEdges
                            (CamVerts, DevVerts, RxInterpolants,
                             nClipBuffer, &nPntInVerts,
                             &nPntOutVerts, rwZHICLIP,
                             PerspClipFuncs.ZHIFunc) < 3)
                        {
                            /* Clipped out */
                            goto ClippedOut;
                        }
                    }

                    /* Oi!! To save all this indexing into CamVerts, surely it'd be better to
                     * have the new Or calculated inside _rwForAllEdges? */
                    /* Calculate new cOr */
                    i = nPntInVerts;
                    cOr = RxClusterGetIndexedData
                        (CamVerts, RxCamSpace3DVertex,
                         nClipBuffer[i])->clipFlags;
                    i = (i + 1) & 15;
                    do
                    {
                        cOr |=
                            RxClusterGetIndexedData(CamVerts,
                                                    RxCamSpace3DVertex,
                                                    nClipBuffer
                                                    [i])->clipFlags;
                        i = (i + 1) & 15;
                    }
                    while (i != nPntOutVerts);
                }

                if (cOr & (rwYLOCLIP | rwYHICLIP))
                {

                    /***********************  Y LO ***********************/
                    if (cOr & (RwUInt8) rwYLOCLIP)
                    {
                        if (_rwForAllEdges
                            (CamVerts, DevVerts, RxInterpolants,
                             nClipBuffer, &nPntInVerts,
                             &nPntOutVerts, rwYLOCLIP,
                             PerspClipFuncs.YLOFunc) < 3)
                        {
                            /* Clipped out */
                            goto ClippedOut;
                        }
                    }

                    /***********************  Y HI ***********************/
                    if (cOr & (RwUInt8) rwYHICLIP)
                    {
                        if (_rwForAllEdges
                            (CamVerts, DevVerts, RxInterpolants,
                             nClipBuffer, &nPntInVerts,
                             &nPntOutVerts, rwYHICLIP,
                             PerspClipFuncs.YHIFunc) < 3)
                        {
                            /* Clipped out */
                            goto ClippedOut;
                        }
                    }

                    /* Calculate new cOr */
                    i = nPntInVerts;
                    cOr =
                        (RxClusterGetIndexedData
                         (CamVerts, RxCamSpace3DVertex,
                          nClipBuffer[i]))->clipFlags;
                    i = (i + 1) & 15;
                    do
                    {
                        cOr |=
                            (RxClusterGetIndexedData
                             (CamVerts, RxCamSpace3DVertex,
                              nClipBuffer[i]))->clipFlags;
                        i = (i + 1) & 15;
                    }
                    while (i != nPntOutVerts);
                }

                if (cOr & (rwXLOCLIP | rwXHICLIP))
                {

                    /***********************  X LO ***********************/
                    if (cOr & (RwUInt8) rwXLOCLIP)
                    {
                        if (_rwForAllEdges
                            (CamVerts, DevVerts, RxInterpolants,
                             nClipBuffer, &nPntInVerts,
                             &nPntOutVerts, rwXLOCLIP,
                             PerspClipFuncs.XLOFunc) < 3)
                        {
                            /* Clipped out */
                            goto ClippedOut;
                        }
                    }

                    /***********************  X HI ***********************/
                    if (cOr & (RwUInt8) rwXHICLIP)
                    {
                        if (_rwForAllEdges
                            (CamVerts, DevVerts, RxInterpolants,
                             nClipBuffer, &nPntInVerts,
                             &nPntOutVerts, rwXHICLIP,
                             PerspClipFuncs.XHIFunc) < 3)
                        {
                            /* Clipped out */
                            goto ClippedOut;
                        }
                    }
                }

                /************************ Into pixel space ****************************/

                i = nPntInVerts;
                do
                {
                    RwUInt8             cFlags;

                    CamVert1 =
                        RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex,
                                                nClipBuffer[i]);

                    cFlags = CamVert1->clipFlags;

                    if (cFlags & (rwZCLIP | rwYCLIP | rwXCLIP))
                    {
                        RwReal              nX, nY, nZ, nRecipZ;

                        DevVert1 = RxClusterGetIndexedData(DevVerts,
                                                           RxScrSpace2DVertex,
                                                           nClipBuffer[i]);

                        nX = CamVert1->cameraVertex.x;
                        nY = CamVert1->cameraVertex.y;
                        nZ = CamVert1->cameraVertex.z;
                        nRecipZ = 1 / nZ;

                        /* Set up Pixel space values */
                        RwIm2DVertexSetCameraX(DevVert1, nX);
                        RwIm2DVertexSetCameraY(DevVert1, nY);
                        RwIm2DVertexSetCameraZ(DevVert1, nZ);
                        RwIm2DVertexSetRecipCameraZ(DevVert1, nRecipZ);

                        /* find screen column of pixel */
                        switch (cFlags & (RwUInt8) rwXCLIP)
                        {
                            case rwXHICLIP:
                                RwIm2DVertexSetScreenX(DevVert1,
                                                       _rwClipInfoGlobal.camWidth +
                                                       _rwClipInfoGlobal.camOffsetX);
                                break;
                            case rwXLOCLIP:
                                RwIm2DVertexSetScreenX(DevVert1,
                                                       _rwClipInfoGlobal.camOffsetX);
                                break;
                            default:
                                RwIm2DVertexSetScreenX(DevVert1,
                                                       (_rwClipInfoGlobal.camWidth *
                                                        nX * nRecipZ) +
                                                       _rwClipInfoGlobal.camOffsetX);
                                break;
                        }

                        /* find screen row of pixel */
                        switch (cFlags & (rwYCLIP | rwXCLIP))
                        {
                            case rwYHICLIP:
                                RwIm2DVertexSetScreenY(DevVert1,
                                                       _rwClipInfoGlobal.camHeight +
                                                       _rwClipInfoGlobal.camOffsetY);
                                break;
                            case rwYLOCLIP:
                                RwIm2DVertexSetScreenY(DevVert1,
                                                       _rwClipInfoGlobal.camOffsetY);
                                break;
                            default:
                                RwIm2DVertexSetScreenY(DevVert1,
                                                       (_rwClipInfoGlobal.camHeight *
                                                        nY * nRecipZ) +
                                                       _rwClipInfoGlobal.camOffsetY);
                                break;
                        }

                        /* find range of pixel */
                        switch (cFlags & (rwZCLIP | rwYCLIP | rwXCLIP))
                        {
                            case rwZHICLIP:
                                RwIm2DVertexSetScreenZ(DevVert1,
                                                       _rwClipInfoGlobal.zBufferFar);
                                break;
                            case rwZLOCLIP:
                                RwIm2DVertexSetScreenZ(DevVert1,
                                                       _rwClipInfoGlobal.zBufferNear);
                                break;
                            default:
                                RwIm2DVertexSetScreenZ(DevVert1,
                                                       (_rwClipInfoGlobal.zScale *
                                                        nRecipZ) +
                                                       _rwClipInfoGlobal.zShift);
                                break;
                        }

                        /* Do texture coordinates */
                        RwIm2DVertexSetU(DevVert1, CamVert1->u, nRecipZ);
                        RwIm2DVertexSetV(DevVert1, CamVert1->v, nRecipZ);
                    }

                    /* Onto next vertex */
                    i = (i + 1) & 15;
                }
                while (i != nPntOutVerts);

                /* Get pivot for fanning - this may change if we rotate the fan point */
                pivot = nClipBuffer[nPntInVerts];

                /* Handle case where we are not clipping color */
                if (!(rwSHADEMODEGOURAUD & rsvp->ShadeMode))
                {
                    /* TODO: What's the second test for? Rob? */
                    /* Rob's answer -
                     * Its to make sure you don't stop with the original 1st or
                     * 2nd vertex,
                     * and splat a color needed for another triangle.
                     * I haven't commented it back,
                     * because the clipper appears to` have changed subtley.
                     */
                    while ((pivot != Index3) && (pivot < firstFreeVertex))
                    {
                        /* We need to rotate the vertex indices until the first vertex is
                         * either a generated one or the original third vertex - this
                         * prevents us overwriting a vertex color that a dispatched
                         * polygon relies on.
                         */
                        nClipBuffer[nPntOutVerts] = nClipBuffer[nPntInVerts];
                        nPntOutVerts = (nPntOutVerts + 1) & 15;
                        nPntInVerts = (nPntInVerts + 1) & 15;
                        pivot = nClipBuffer[nPntInVerts];
                    }

                    if (pivot != Index3)
                    {
                        RwIm2DVertexCopyRGBA(RxClusterGetIndexedData
                                             (DevVerts, RxScrSpace2DVertex,
                                              pivot),
                                             RxClusterGetIndexedData
                                             (DevVerts, RxScrSpace2DVertex,
                                              oldFirstVertex));
                    }
                }

                /* Draw the triangles */
                i = (nPntInVerts + 1) & 15;
                j = (nPntInVerts + 2) & 15;

                OriginalIndices[0] = (RxVertexIndex) nClipBuffer[i];
                OriginalIndices[1] = (RxVertexIndex) nClipBuffer[j];
                OriginalIndices[2] = (RxVertexIndex) pivot;

                i = j;
                j = (j + 1) & 15;
                NumRxTriangles = 1;
                Indices->currentData =
                    RxClusterGetIndexedData(Indices, RxVertexIndex,
                                            (Indices->numUsed - 3));
                while (j != nPntOutVerts)
                {
                    RxClusterIncCursorByStride(Indices, IndicesStride * 3);
                    NumRxTriangles++;

                    IndexPtr = RxClusterGetCursorData(Indices, RxVertexIndex);

                    IndexPtr[0] = (RxVertexIndex) nClipBuffer[i];
                    IndexPtr[1] = (RxVertexIndex) nClipBuffer[j];
                    IndexPtr[2] = pivot;

                    i = j;
                    j = (j + 1) & 15;
                }
                Indices->currentData = OriginalIndices;
            }                  /* if (cOr) */
            else
            {
                NumRxTriangles = 1;
            }

          ClippedOut:

            /* If we clipped the triangle away completely, compact the array */
            if (NumRxTriangles == 0)
            {
                /* Copy the end triangle over this one (and then the last generated tri into the
                 * gap that that leaves - we want original and generated triangles contiguous and
                 * decrement Tri and OldNumRxTriangles */
                RxVertexIndex      *LastOriginalIndices, *EndIndices;
                RwUInt32            ReusableVerts =

                    CamVerts->numUsed - OldNumVertices;

                LastOriginalIndices =
                    RxClusterGetIndexedData(Indices, RxVertexIndex,
                                            (OldNumRxTriangles - 1) * 3);

                EndIndices =
                    RxClusterGetIndexedData(Indices, RxVertexIndex,
                                            (Indices->numUsed - 3));

                /* TODO: memcpy slow!!! Use DELETETRIANGLE macros ala nodeBackFaceCull */

                RxClusterGetCursorData(Indices, RxVertexIndex)[0] =
                    LastOriginalIndices[0];
                RxClusterGetCursorData(Indices, RxVertexIndex)[1] =
                    LastOriginalIndices[1];
                RxClusterGetCursorData(Indices, RxVertexIndex)[2] =
                    LastOriginalIndices[2];

                LastOriginalIndices[0] = EndIndices[0];
                LastOriginalIndices[1] = EndIndices[1];
                LastOriginalIndices[2] = EndIndices[2];

                Indices->numUsed -= 3;
                OldNumRxTriangles--;
                Tri--;

                /* Any vertices generated while clipping
                 * this triangle to nothing can be reused */
                CamVerts->numUsed = OldNumVertices;
                DevVerts->numUsed = OldNumVertices;
                if (RxInterpolants)
                {
                    const RwInt32       RxInterpolantsStride =

                        RxInterpolants->stride;

                    RxInterpolants->numUsed -= ReusableVerts;
                    while (ReusableVerts)
                    {
                        RxClusterDecCursorByStride(RxInterpolants,
                                                   RxInterpolantsStride);
                        ReusableVerts--;
                    }
                }
            }
            else
            {
                NumRxTriangles--; /* If we end up with 3 triangles, we only *added* two. */
                Indices->numUsed += NumRxTriangles * 3;

                /* Onto the next triangle */
                RxClusterIncCursorByStride(Indices, IndicesStride * 3);
            }

          ClipEnd:

            if (!result)
            {
                RxPacketDestroy(Packet, Self);
                RWRETURN(result);
            }
        }

        /* Bring StateData up to date */
        MeshData->NumElements = Indices->numUsed / 3;
        MeshData->NumVertices = CamVerts->numUsed;
    }                          /* if (MeshData->ClipFlagsOr) */

    if (MeshData->NumElements == 0)
    {
        /* This packet's triangles have all been clipped into nonexistence,
         * so send it to the node's second output (which is most likely
         * left unconnected so that the packet will be destroyed) */
        RxPacketDispatch(Packet, 1, Self);
    }
    else
    {
        /* Output the packet to the first (default) output of this Node */
        RxPacketDispatch(Packet, 0, Self);
    }

    RWRETURN(result);
}

static              RwBool
ParallelClipProcessPacket(RxPipelineNodeInstance * Self, RxPacket * Packet)
{
    /*
     * o We check that each polygon needs clipping (i.e, cOr != 0), but that it
     *   is not obviously clipped off screen beyond one plane (i.e cAnd == 0).
     *   + We assume that we will never generate more than 12 vertices (two per
     *     plane, with six intersected planes max) and 8 triangles (a triangle
     *     can potentially intersect all six planes and each plane can cut off
     *     a corner adding one edge and two vertices, leaving a 9-sided poly;
     *     the  original triangle can be overwritten) per triangle.
     *
     * o What we have is a circular buffer of vertex indices.
     *   + Clipped vertices are appended to CamVerts and DevVerts and indices
     *     are put into the circular buffer as generated.
     *   + Clipped polygon indexes are placed directly after the non clipped
     *     polygon indexes, and the list is made circular by modulo
     *     arithmetic on the list indices.
     *   + For each plane clipped against, you generate a complete new list of
     *     indices from the existing one, so that you can keep the indices in
     *     thecorrect winding order and insert new indices as new edges are
     *     generated.
     *   + When you've finished clipping the polygon against a particular
     *     plane, because you were putting the indices after what you were
     *     clippingfrom, you just carry on, no messing about with pointers.
     *   + cOr is generated between the get and put pointers for polygon
     *     indices.
     */

    RwBool              result = TRUE;
    static const RxClipFuncs ParaClipFuncs = {
        _rwGenerateParallelZLOClippedVertex,
        _rwGenerateParallelZHIClippedVertex,
        _rwGenerateParallelYLOClippedVertex,
        _rwGenerateParallelYHIClippedVertex,
        _rwGenerateParallelXLOClippedVertex,
        _rwGenerateParallelXHIClippedVertex
    };
    RxCluster          *CamVerts;
    RxCluster          *DevVerts;
    RxCluster          *Indices;
    RxCluster          *MeshState;
    RxCluster          *RenderState;
    RxCluster          *RxInterpolants;
    RwInt32             IndicesStride;
    RxMeshStateVector  *MeshData;
    RxRenderStateVector *rsvp;
    RwUInt32            NumRxTriangles;
    RwUInt32            OldNumRxTriangles;
    RwUInt32            OldNumVertices;
    RxVertexIndex      *OriginalIndices;
    RxVertexIndex      *IndexPtr;
    RxVertexIndex       Index1;
    RxVertexIndex       Index2;
    RxVertexIndex       Index3;
    RxCamSpace3DVertex *CamVert1;
    RxCamSpace3DVertex *CamVert2;
    RxCamSpace3DVertex *CamVert3;
    RxScrSpace2DVertex *DevVert1;
    RwInt32             i, j;
    RwUInt32            Tri;
    RxVertexIndex       nClipBuffer[16];
    RwInt32             nPntInVerts, nPntOutVerts;
    RwUInt8             cOr, cAnd;
    RxVertexIndex       oldFirstVertex, firstFreeVertex;
    RxVertexIndex       pivot;  /* For fanning */

    RWFUNCTION(RWSTRING("ParallelClipProcessPacket"));

    CamVerts = RxClusterLockWrite(Packet, 0, Self);
    DevVerts = RxClusterLockWrite(Packet, 1, Self);
    Indices = RxClusterLockWrite(Packet, 2, Self);
    MeshState = RxClusterLockWrite(Packet, 3, Self);
    RenderState = RxClusterLockRead(Packet, 4);
    if ((RenderState != NULL) && (RenderState->data != NULL))
    {
        rsvp = RxClusterGetCursorData(RenderState, RxRenderStateVector);
    }
    else
    {
        rsvp = &RXPIPELINEGLOBAL(defaultRenderState);
    }
    RxInterpolants = RxClusterLockWrite(Packet, 5, Self);

    RWASSERT((CamVerts != NULL) && (CamVerts->numUsed > 0));
    RWASSERT((DevVerts != NULL) && (DevVerts->numUsed > 0));
    RWASSERT((Indices != NULL) && (Indices->numUsed > 0));
    RWASSERT((MeshState != NULL) && (MeshState->numUsed > 0));

    MeshData = RxClusterGetCursorData(MeshState, RxMeshStateVector);
    RWASSERT(MeshData->PrimType == rwPRIMTYPETRILIST);
    /* ClipTriangle can only handle rwPRIMTYPETRILIST primitives as of yet */

    IndicesStride = Indices->stride;

    if (RxInterpolants)
    {
        /* Empty it out if (for some reason) it has any contents */
        RxInterpolants->stride = sizeof(RxInterp);
        RxInterpolants->numUsed = 0;
    }

    if (MeshData->ClipFlagsOr)
    {
        OldNumRxTriangles = MeshData->NumElements;

        /*
         * NOTE:
         * StateData is invalidated during this loop and
         * corrected after it finishes
         */
        for (Tri = 0; Tri < OldNumRxTriangles; Tri++)
        {
            IndexPtr = RxClusterGetCursorData(Indices, RxVertexIndex);
            Index1 = IndexPtr[0];
            Index2 = IndexPtr[1];
            Index3 = IndexPtr[2];

            /* TODO: You're accessing the CamVerts array in random-access
             * order just to retrieve ClipFlags here! Dude!! */

            CamVert1 =
                RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex, Index1);
            CamVert2 =
                RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex, Index2);
            CamVert3 =
                RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex, Index3);

            OldNumVertices = CamVerts->numUsed;
            firstFreeVertex = (RxVertexIndex)OldNumVertices;

            /* Set up cOr and cAnd */
            cOr =
                CamVert1->clipFlags | CamVert2->
                clipFlags | CamVert3->clipFlags;
            cAnd =
                CamVert1->clipFlags & CamVert2->
                clipFlags & CamVert3->clipFlags;

            NumRxTriangles = 0;
            if (cOr)
            {
                if (cAnd)
                {
                    goto ClippedOut;
                }

                /* Check we're not running out of space for
                 * new triangles */
                if (Indices->numUsed + 24 > Indices->numAlloced)
                {
                    /* TODO: Is this a good estimate? */
                    i = Indices->numAlloced + (OldNumRxTriangles - Tri) + 24;
                    result = (NULL != RxClusterResizeData(Indices, i));
                    if (!result)
                    {
                        goto ClipEnd;
                    }
                    /*
                     * Resizing the cluster resets the
                     * CurrentData pointer (and a realloc might
                     * cause a copy, too), so restore it here
                     */
                    Indices->currentData =
                        RxClusterGetIndexedData(Indices,
                                                RxVertexIndex, Tri * 3);
                }
                /* Check we're not running out of space for new vertices */
                if (CamVerts->numUsed + 12 > CamVerts->numAlloced)
                {
                    /* TODO: Is this a good estimate? */
                    i = CamVerts->numAlloced + (OldNumRxTriangles - Tri) + 12;
                    result = (NULL != RxClusterResizeData(CamVerts, i));
                    if (!result)
                    {
                        goto ClipEnd;
                    }
                }
                if (DevVerts->numUsed + 12 > DevVerts->numAlloced)
                {
                    /* TODO: Is this a good estimate? */
                    i = DevVerts->numAlloced + (OldNumRxTriangles - Tri) + 12;
                    result = (NULL != RxClusterResizeData(DevVerts, i));
                    if (!result)
                    {
                        goto ClipEnd;
                    }
                }

                if (RxInterpolants && (RxInterpolants->numUsed + 12 >
                                       RxInterpolants->numAlloced))
                {
                    void               *resultPtr;

                    /* TODO: Is this a good estimate? */
                    i = RxInterpolants->numAlloced +
                        (OldNumRxTriangles - Tri) + 12;
                    if (RxInterpolants->numAlloced <= 0)
                    {
                        resultPtr =
                            RxClusterInitializeData(RxInterpolants, i,
                                                    sizeof(RxInterp));
                    }
                    else
                    {
                        resultPtr = RxClusterResizeData(RxInterpolants, i);
                    }
                    if (resultPtr == NULL)
                    {
                        result = FALSE;
                        goto ClipEnd;
                    }
                    /* Put the pointer back at the first free spot */
                    RxInterpolants->currentData =
                        RxClusterGetIndexedData(RxInterpolants,
                                                RxInterp,
                                                RxInterpolants->numUsed);
                }

                OriginalIndices = (RxVertexIndex *) (Indices->currentData);

                /*
                 * This way we can restore the third vertex color for
                 * a flat shaded clipped polygon
                 */
                oldFirstVertex = Index3;

                /* Set it up */
                nPntInVerts = 0;
                nClipBuffer[0] = Index1;
                nClipBuffer[1] = Index2;
                nClipBuffer[2] = Index3;

                nPntOutVerts = 3;

                if (cOr & (rwZLOCLIP | rwZHICLIP))
                {

                    /***********************  Z LO ***********************/
                    if (cOr & (RwUInt8) rwZLOCLIP)
                    {
                        if (_rwForAllEdges
                            (CamVerts, DevVerts, RxInterpolants,
                             nClipBuffer, &nPntInVerts,
                             &nPntOutVerts, rwZLOCLIP,
                             ParaClipFuncs.ZLOFunc) < 3)
                        {
                            /* Clipped out */
                            goto ClippedOut;
                        }
                    }

                    /***********************  Z HI ***********************/
                    if (cOr & (RwUInt8) rwZHICLIP)
                    {
                        if (_rwForAllEdges
                            (CamVerts, DevVerts, RxInterpolants,
                             nClipBuffer, &nPntInVerts,
                             &nPntOutVerts, rwZHICLIP,
                             ParaClipFuncs.ZHIFunc) < 3)
                        {
                            /* Clipped out */
                            goto ClippedOut;
                        }
                    }

                    /* Oi!! To save all this indexing into CamVerts, surely it'd be better to
                     * have the new Or calculated inside _rwForAllEdges? */
                    /* Calculate new cOr */
                    i = nPntInVerts;
                    cOr =
                        (RxClusterGetIndexedData
                         (CamVerts, RxCamSpace3DVertex,
                          nClipBuffer[i]))->clipFlags;
                    i = (i + 1) & 15;
                    do
                    {
                        cOr |=
                            (RxClusterGetIndexedData
                             (CamVerts, RxCamSpace3DVertex,
                              nClipBuffer[i]))->clipFlags;
                        i = (i + 1) & 15;
                    }
                    while (i != nPntOutVerts);
                }

                if (cOr & (rwYLOCLIP | rwYHICLIP))
                {

                    /***********************  Y LO ***********************/
                    if (cOr & (RwUInt8) rwYLOCLIP)
                    {
                        if (_rwForAllEdges
                            (CamVerts, DevVerts, RxInterpolants,
                             nClipBuffer, &nPntInVerts,
                             &nPntOutVerts, rwYLOCLIP,
                             ParaClipFuncs.YLOFunc) < 3)
                        {
                            /* Clipped out */
                            goto ClippedOut;
                        }
                    }

                    /***********************  Y HI ***********************/
                    if (cOr & (RwUInt8) rwYHICLIP)
                    {
                        if (_rwForAllEdges
                            (CamVerts, DevVerts, RxInterpolants,
                             nClipBuffer, &nPntInVerts,
                             &nPntOutVerts, rwYHICLIP,
                             ParaClipFuncs.YHIFunc) < 3)
                        {
                            /* Clipped out */
                            goto ClippedOut;
                        }
                    }

                    /* Calculate new cOr */
                    i = nPntInVerts;
                    cOr =
                        (RxClusterGetIndexedData
                         (CamVerts, RxCamSpace3DVertex,
                          nClipBuffer[i]))->clipFlags;
                    i = (i + 1) & 15;
                    do
                    {
                        cOr |=
                            (RxClusterGetIndexedData
                             (CamVerts, RxCamSpace3DVertex,
                              nClipBuffer[i]))->clipFlags;
                        i = (i + 1) & 15;
                    }
                    while (i != nPntOutVerts);
                }

                if (cOr & (rwXLOCLIP | rwXHICLIP))
                {

                    /***********************  X LO ***********************/
                    if (cOr & (RwUInt8) rwXLOCLIP)
                    {
                        if (_rwForAllEdges
                            (CamVerts, DevVerts, RxInterpolants,
                             nClipBuffer, &nPntInVerts,
                             &nPntOutVerts, rwXLOCLIP,
                             ParaClipFuncs.XLOFunc) < 3)
                        {
                            /* Clipped out */
                            goto ClippedOut;
                        }
                    }

                    /***********************  X HI ***********************/
                    if (cOr & (RwUInt8) rwXHICLIP)
                    {
                        if (_rwForAllEdges
                            (CamVerts, DevVerts, RxInterpolants,
                             nClipBuffer, &nPntInVerts,
                             &nPntOutVerts, rwXHICLIP,
                             ParaClipFuncs.XHIFunc) < 3)
                        {
                            /* Clipped out */
                            goto ClippedOut;
                        }
                    }
                }

                /************************ Into pixel space ****************************/

                i = nPntInVerts;
                do
                {
                    RwUInt8             cFlags;

                    CamVert1 =
                        RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex,
                                                nClipBuffer[i]);

                    cFlags = CamVert1->clipFlags;

                    if (cFlags & (rwZCLIP | rwYCLIP | rwXCLIP))
                    {
                        RwReal              nX, nY, nZ;

                        DevVert1 =
                            RxClusterGetIndexedData(DevVerts,
                                                    RxScrSpace2DVertex,
                                                    nClipBuffer[i]);

                        nX = CamVert1->cameraVertex.x;
                        nY = CamVert1->cameraVertex.y;
                        nZ = CamVert1->cameraVertex.z;

                        /* Set up Pixel space values */
                        RwIm2DVertexSetCameraX(DevVert1, nX);
                        RwIm2DVertexSetCameraY(DevVert1, nY);
                        RwIm2DVertexSetCameraZ(DevVert1, nZ);
                        RwIm2DVertexSetRecipCameraZ(DevVert1, (RwReal) 1.0);

                        /* find screen column of pixel */
                        switch (cFlags & (RwUInt8) rwXCLIP)
                        {
                            case rwXHICLIP:
                                RwIm2DVertexSetScreenX(DevVert1,
                                                       _rwClipInfoGlobal.camWidth +
                                                       _rwClipInfoGlobal.camOffsetX);
                                break;
                            case rwXLOCLIP:
                                RwIm2DVertexSetScreenX(DevVert1,
                                                       _rwClipInfoGlobal.camOffsetX);
                                break;
                            default:
                                RwIm2DVertexSetScreenX(DevVert1,
                                                       _rwClipInfoGlobal.camWidth *
                                                       nX +
                                                       _rwClipInfoGlobal.camOffsetX);
                                break;
                        }

                        /* find screen row of pixel */
                        switch (cFlags & (rwYCLIP | rwXCLIP))
                        {
                            case rwYHICLIP:
                                RwIm2DVertexSetScreenY(DevVert1,
                                                       _rwClipInfoGlobal.camHeight +
                                                       _rwClipInfoGlobal.camOffsetY);
                                break;
                            case rwYLOCLIP:
                                RwIm2DVertexSetScreenY(DevVert1,
                                                       _rwClipInfoGlobal.camOffsetY);
                                break;
                            default:
                                RwIm2DVertexSetScreenY(DevVert1,
                                                       _rwClipInfoGlobal.camHeight *
                                                       nY +
                                                       _rwClipInfoGlobal.camOffsetY);
                                break;
                        }

                        /* find range of pixel */
                        switch (cFlags & (rwZCLIP | rwYCLIP | rwXCLIP))
                        {
                            case rwZHICLIP:
                                RwIm2DVertexSetScreenZ(DevVert1,
                                                       _rwClipInfoGlobal.zBufferFar);
                                break;
                            case rwZLOCLIP:
                                RwIm2DVertexSetScreenZ(DevVert1,
                                                       _rwClipInfoGlobal.zBufferNear);
                                break;
                            default:
                                RwIm2DVertexSetScreenZ(DevVert1,
                                                       _rwClipInfoGlobal.zScale * nZ +
                                                       _rwClipInfoGlobal.zShift);
                                break;
                        }

                        /* Do texture coordinates */
                        RwIm2DVertexSetU(DevVert1, CamVert1->u, (RwReal) 1.0);
                        RwIm2DVertexSetV(DevVert1, CamVert1->v, (RwReal) 1.0);
                    }

                    /* Onto next vertex */
                    i = (i + 1) & 15;
                }
                while (i != nPntOutVerts);

                /* Get pivot for fanning - this may change if we rotate the fan point */
                pivot = nClipBuffer[nPntInVerts];

                /* Handle case where we are not clipping color */
                if (!(rwSHADEMODEGOURAUD & rsvp->ShadeMode))
                {
                    /* TODO: What's the second test for? Rob? */
                    /* Rob's answer -
                     * Its to make sure you don't stop with the original 1st or
                     * 2nd vertex,
                     * and splat a color needed for another triangle.
                     * I haven't commented it back,
                     * because the clipper appears to have changed subtley.
                     */
                    while ((pivot != Index3) && (pivot < firstFreeVertex))
                    {
                        /* We need to rotate the vertex indices until the first vertex is
                         * either a generated one or the original third vertex - this
                         * prevents us overwriting a vertex color that a dispatched
                         * polygon relies on.
                         */
                        nClipBuffer[nPntOutVerts] = nClipBuffer[nPntInVerts];
                        nPntOutVerts = (nPntOutVerts + 1) & 15;
                        nPntInVerts = (nPntInVerts + 1) & 15;
                        pivot = nClipBuffer[nPntInVerts];
                    }

                    if (pivot != Index3)
                    {
                        RwIm2DVertexCopyRGBA(RxClusterGetIndexedData
                                             (DevVerts, RxScrSpace2DVertex,
                                              pivot),
                                             RxClusterGetIndexedData
                                             (DevVerts, RxScrSpace2DVertex,
                                              oldFirstVertex));
                    }
                }

                /* Draw the triangles */
                i = (nPntInVerts + 1) & 15;
                j = (nPntInVerts + 2) & 15;

                OriginalIndices[0] = (RxVertexIndex) nClipBuffer[i];
                OriginalIndices[1] = (RxVertexIndex) nClipBuffer[j];
                OriginalIndices[2] = (RxVertexIndex) pivot;

                i = j;
                j = (j + 1) & 15;
                NumRxTriangles = 1;
                Indices->currentData =
                    RxClusterGetIndexedData(Indices, RxVertexIndex,
                                            (Indices->numUsed - 3));
                while (j != nPntOutVerts)
                {
                    RxClusterIncCursorByStride(Indices, IndicesStride * 3);
                    NumRxTriangles++;

                    IndexPtr = RxClusterGetCursorData(Indices, RxVertexIndex);

                    IndexPtr[0] = nClipBuffer[i];
                    IndexPtr[1] = nClipBuffer[j];
                    IndexPtr[2] = pivot;

                    i = j;
                    j = (j + 1) & 15;
                }
                Indices->currentData = OriginalIndices;
            }                  /* if (cOr) */
            else
            {
                NumRxTriangles = 1;
            }

          ClippedOut:

            /* If we clipped the triangle away completely, compact the array */
            if (NumRxTriangles == 0)
            {
                /* Copy the end triangle over this one (and then the last generated tri into the
                 * gap that that leaves - we want original and generated triangles contiguous and
                 * decrement Tri and OldNumRxTriangles */
                RxVertexIndex      *LastOriginalIndices, *EndIndices;
                RwUInt32            ReusableVerts =

                    CamVerts->numUsed - OldNumVertices;

                LastOriginalIndices =
                    RxClusterGetIndexedData(Indices, RxVertexIndex,
                                            (OldNumRxTriangles - 1) * 3);

                EndIndices =
                    RxClusterGetIndexedData(Indices, RxVertexIndex,
                                            (Indices->numUsed - 3));

                /* TODO: memcpy slow!!! Use DELETETRIANGLE macros ala nodeBackFaceCull */
                RxClusterGetCursorData(Indices, RxVertexIndex)[0] =
                    LastOriginalIndices[0];
                RxClusterGetCursorData(Indices, RxVertexIndex)[1] =
                    LastOriginalIndices[1];
                RxClusterGetCursorData(Indices, RxVertexIndex)[2] =
                    LastOriginalIndices[2];

                LastOriginalIndices[0] = EndIndices[0];
                LastOriginalIndices[1] = EndIndices[1];
                LastOriginalIndices[2] = EndIndices[2];

                Indices->numUsed -= 3;
                OldNumRxTriangles--;
                Tri--;

                /* Any vertices generated while clipping
                 * this triangle to nothing can be reused */
                CamVerts->numUsed = OldNumVertices;
                DevVerts->numUsed = OldNumVertices;
                if (RxInterpolants)
                {
                    const RwInt32       RxInterpolantsStride =

                        RxInterpolants->stride;

                    RxInterpolants->numUsed -= ReusableVerts;
                    while (ReusableVerts)
                    {
                        RxClusterDecCursorByStride(RxInterpolants,
                                                   RxInterpolantsStride);
                        ReusableVerts--;
                    }
                }
            }
            else
            {
                NumRxTriangles--; /* If we end up with 3 triangles, we only *added* two. */
                Indices->numUsed += NumRxTriangles * 3;

                /* Onto the next triangle */
                RxClusterIncCursorByStride(Indices, IndicesStride * 3);
            }

          ClipEnd:

            if (!result)
            {
                RxPacketDestroy(Packet, Self);
                RWRETURN(result);
            }
        }

        /* Bring StateData up to date */
        MeshData->NumElements = Indices->numUsed / 3;
        MeshData->NumVertices = CamVerts->numUsed;
    }                          /* if (MeshData->ClipFlagsOr) */

    if (MeshData->NumElements == 0)
    {
        /* This packet's triangles have all been clipped into nonexistence,
         * so send it to the node's second output (which is most likely
         * left unconnected so that the packet will be destroyed) */
        RxPacketDispatch(Packet, 1, Self);
    }
    else
    {
        /* Output the packet to the first (default) output of this Node */
        RxPacketDispatch(Packet, 0, Self);
    }

    RWRETURN(result);
}

/****************************************************************************
 TriangleClipNode

 on entry: a packet containing perspective or parallel triangles to clip
 on exit :
 */

static RwBool
TriangleClipNode( RxPipelineNodeInstance * self,
                  const RxPipelineNodeParam * params __RWUNUSED__ )
{
    RwBool              result = TRUE;
    RwCamera           *curCamera;
    RwRaster           *rpRas;
    RwUInt32            persp;
    RxPacket           *packet;
    RwInt32             camWidth;
    RwInt32             camHeight;
    ClipProcessPacketFunc func;
    static ClipProcessPacketFunc ClipFunc[2] = {
        ParallelClipProcessPacket,
        PerspectiveClipProcessPacket
    };

    RWFUNCTION(RWSTRING("TriangleClipNode"));

    /* TODO:
     * global shared camera bad for multithreading with multiple viewports
     */
    curCamera = (RwCamera *) RWSRCGLOBAL(curCamera);
    RWASSERT(NULL != curCamera);

    rpRas = RwCameraGetRaster(curCamera);
    persp = (curCamera->projectionType == rwPERSPECTIVE) ? 1 : 0;

    func = ClipFunc[persp];

    /*
     * Set up oft-used clipping numeros
     * (TODO: shared with p2clpcom.c as a global - bad for multithreading)
     */
    _rwClipInfoGlobal.zScale = curCamera->zScale;
    _rwClipInfoGlobal.zShift = curCamera->zShift;
    camWidth = RwRasterGetWidth(rpRas);
    _rwClipInfoGlobal.camWidth = (RwReal) camWidth;
    camHeight = RwRasterGetHeight(rpRas);
    _rwClipInfoGlobal.camHeight = (RwReal) camHeight;
    _rwClipInfoGlobal.camOffsetX = (RwReal) rpRas->nOffsetX;
    _rwClipInfoGlobal.camOffsetY = (RwReal) rpRas->nOffsetY;
    _rwClipInfoGlobal.nearClip = curCamera->nearPlane;
    _rwClipInfoGlobal.farClip = curCamera->farPlane;
    _rwClipInfoGlobal.zBufferNear = RwIm2DGetNearScreenZ();
    _rwClipInfoGlobal.zBufferFar = RwIm2DGetFarScreenZ();

#ifdef SKY2_DRVMODEL_H
    /* On the PlayStation 2, we should munge the camera matrix and clipping
     * info such that devverts are clipped to the large [0,4096] overdraw
     * frustum */
#endif

    packet = (RxPacket *)RxPacketFetch(self);
    RWASSERT(NULL != packet);

    result = func(self, packet);

    /* RWCRTCHECKMEMORY(); */

    RWRETURN(result);
}

/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionGetClipTriangle returns a pointer to a node
 * implementing triangle clipping in custom pipelines
 *
 * This node clips triangles to the frustum of the current camera (as defined by
 * the camera's ViewWindow, ViewOffset and Projection type - parallel or
 * perspective).
 *
 * This node is meant to be used after a transform node, so that the camera
 * space and (un-clipped) screen space vertices will contain valid data. It
 * clips all values associated with the triangles/vertices (that is, position,
 * texture coordinates and RGBA values) in 3D camera space so that the results
 * of perspective-correct interpolation by the rasteriser will be correct.
 * Generally, any values you want clipping need to be in the camera-space
 * vertices before this node, with the exception of color which needs to be
 * in the screen-space vertices. This means that if lighting is performed,
 * the lighting nodes should come before this node, because they set up the
 * post-lighting color in the screen-space vertices. Any new vertices
 * generated during clipping are projected (so that both camera-space and
 * screen-space positions and texture coordinates are correct) and added to
 * the ends of the vertex arrays, and the mesh state cluster's NumVertices
 * and NumElements members are updated as appropriate.
 *
 * The interpolants cluster which is output is used to accelerate multipass
 * rendering. If the triangles being rendered are to be submitted more than
 * once, with different UV coordinates (or RGBA values and possibly a
 * different texture) after the first time, then it is not necessary to clip
 * the triangles a second time. The interpolant values can be used to
 * interpolate UV values (or RGBAs) for clipped triangles. UVInterp.csl
 * uses this cluster, see that for further details.
 *
 * If the render state cluster is not present (or contains no data) then the
 * node assumes that the packet's render state is the default render state,
 * obtained through RxRenderStateVectorGetDefaultRenderStateVector(). This is
 * used in this case to determine if triangles are to be gouraud shaded (hence
 * color should be interpolated when triangles are clipped) or flat shaded.
 *
 * If all the triangles in a packet are completely clipped away (they all
 * lie entirely offscreen) then the packet is sent to the node's second output
 * (which is usually left disconnected, resulting in the packet's destruction.
 * In some cases, however, it may be desirable to connect this output to
 * subsequent nodes if the geometry's extent is known to be modified further
 * down the pipeline if, for example, vertex normals are added, to be
 * rendered as lines), but otherwise packets are sent to the node's first
 * output.
 *
 * The node has two outputs. Packets in which all triangles are clipped away
 * are sent to the second output.
 * The input requirements of this node:
 *
 * \verbatim
   RxClCamSpace3DVertices - required
   RxClScrSpace2DVertices - required
   RxClIndices            - required
   RxClMeshState          - required
   RxClRenderState        - optional
   RxClInterpolants       - don't want
   \endverbatim
 *
 * The characteristics of the first of this node's output's:
 *
 * \verbatim
   RxClCamSpace3DVertices - valid
   RxClScrSpace2DVertices - valid
   RxClIndices            - valid
   RxClMeshState          - valid
   RxClRenderState        - no change
   RxClInterpolants       - valid
   \endverbatim
 *
 * The characteristics of the second of this node's output's:
 *
 * \verbatim
   RxClCamSpace3DVertices - valid
   RxClScrSpace2DVertices - valid
   RxClIndices            - invalid
   RxClMeshState          - valid
   RxClRenderState        - no change
   RxClInterpolants       - invalid
   \endverbatim
 *
 * \return A pointer to the node for triangle clipping in custom pipelines on
 * success, or NULL if there is an error
 *
 * \see RxNodeDefinitionGetClipLine
 * \see RxNodeDefinitionGetCullTriangle
 * \see RxNodeDefinitionGetImmInstance
 * \see RxNodeDefinitionGetImmMangleLineIndices
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
RxNodeDefinitionGetClipTriangle(void)
{

    static RxClusterRef N5clofinterest[] = {
        {&RxClCamSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClScrSpace2DVertices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClIndices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClRenderState, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClInterpolants, rxCLALLOWABSENT, rxCLRESERVED}
    };

#define NUMCLUSTERSOFINTEREST \
        ((sizeof(N5clofinterest))/(sizeof(N5clofinterest[0])))

    static RxClusterValidityReq N5inputreqs[NUMCLUSTERSOFINTEREST] = {
        rxCLREQ_REQUIRED,
        rxCLREQ_REQUIRED,
        rxCLREQ_REQUIRED,
        rxCLREQ_REQUIRED,
        rxCLREQ_OPTIONAL,
        rxCLREQ_OPTIONAL
    };

    static RxClusterValid N5outcl1[NUMCLUSTERSOFINTEREST] = {
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_NOCHANGE,
        rxCLVALID_VALID
    };

    static RxClusterValid N5outcl2[NUMCLUSTERSOFINTEREST] = {
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_INVALID,
        rxCLVALID_VALID,
        rxCLVALID_NOCHANGE,
        rxCLVALID_INVALID
    };

    static RwChar       _ClipOut[] = RWSTRING("ClipOut");
    static RwChar       _ClipAllClipped[] = RWSTRING("ClipAllClipped");

    static RxOutputSpec N5outputs[] = {
        {_ClipOut,
         N5outcl1,
         rxCLVALID_NOCHANGE},
        {_ClipAllClipped,
         N5outcl2,
         rxCLVALID_NOCHANGE}
    };

#define NUMOUTPUTS \
        ((sizeof(N5outputs))/(sizeof(N5outputs[0])))

    static RwChar       _ClipTriangle_csl[] = RWSTRING("ClipTriangle.csl");

    static RxNodeDefinition nodeClipTriangleCSL = {

        /* */
        _ClipTriangle_csl,
        {TriangleClipNode,
         (RxNodeInitFn)NULL,
         (RxNodeTermFn)NULL,
         (RxPipelineNodeInitFn)NULL,
         (RxPipelineNodeTermFn)NULL,
         (RxPipelineNodeConfigFn)NULL,
         (RxConfigMsgHandlerFn)NULL},
        {NUMCLUSTERSOFINTEREST,
         N5clofinterest,
         N5inputreqs,
         NUMOUTPUTS,
         N5outputs},
        0,
        (RxNodeDefEditable) FALSE,
        0
    };

    RxNodeDefinition   *result = &nodeClipTriangleCSL;

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionGetClipTriangle"));

    /*RWMESSAGE((RWSTRING("result %p"), result)); */

    RWRETURN(result);
}
