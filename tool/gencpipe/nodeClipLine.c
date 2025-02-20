/*
 * nodeClipLine
 *
 * Line clipping in custom pipelines
 *
 * Copyright (c) Criterion Software Limited
 */

/***************************************************************************
 * Module  : nodeClipLine.c                                                *
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
    (*ClipProcessPacketFunc) (RxPipelineNodeInstance * Self,
                              RxPacket * Packet);

/****************************************************************************
 Local defines
 */

#define MESSAGE(_string)                                       \
    RwDebugSendMessage(rwDEBUGMESSAGE, "ClipLine.csl", _string)

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                                Functions

 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

static              RwBool
PerspectiveClipProcessPacket(RxPipelineNodeInstance * Self,
                             RxPacket * Packet)
{
    /*
     * o We check that each line needs clipping (i.e, cOr != 0), but that it
     *   is not obviously clipped off screen beyond one plane (i.e cAnd == 0).
     *   + We assume that we will never generate more than 1 line per line :-)
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
    RwInt32             LinesStride;
    RxMeshStateVector  *MeshData;
    RxRenderStateVector *rsvp;
    RwUInt32            NumLines, OldNumLines, OldNumVertices;
    RxVertexIndex      *OriginalIndices;
    RxVertexIndex       Index1, Index2;
    RxCamSpace3DVertex *CamVert1, *CamVert2;
    RwInt32             i, j;
    RwUInt32            CurLine;
    RxVertexIndex       nClipBuffer[16];
    RwInt32             nPntInVerts, nPntOutVerts;
    RwUInt8             cOr, cAnd;

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

    RWASSERT((CamVerts != NULL) && (CamVerts->numUsed > 0));
    RWASSERT((DevVerts != NULL) && (DevVerts->numUsed > 0));
    RWASSERT((Indices != NULL) && (Indices->numUsed > 0));
    RWASSERT((MeshState != NULL) && (MeshState->numUsed > 0));

    MeshData = RxClusterGetCursorData(MeshState, RxMeshStateVector);
    RWASSERT(MeshData->PrimType == rwPRIMTYPELINELIST);
    /* ClipLine can only handle rwPRIMTYPELINELIST primitives as of yet */

    LinesStride = Indices->stride << 1;

    if (MeshData->ClipFlagsOr)
    {
        OldNumLines = MeshData->NumElements;

        /*
         * NOTE:
         * StateData is invalidated during this loop and
         * corrected after it finishes
         */

        for (CurLine = 0; CurLine < OldNumLines; CurLine++)
        {
            Index1 =
                (RxClusterGetCursorData(Indices, RxVertexIndex))[0];
            Index2 =
                (RxClusterGetCursorData(Indices, RxVertexIndex))[1];

            /* TODO: You're accessing the CamVerts array in random-access
             * order just to retrieve ClipFlags here! Dude!! */

            CamVert1 =
                RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex,
                                        Index1);
            CamVert2 =
                RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex,
                                        Index2);

            OldNumVertices = CamVerts->numUsed;

            /* Set up cOr and cAnd */
            cOr = CamVert1->clipFlags | CamVert2->clipFlags;
            cAnd = CamVert1->clipFlags & CamVert2->clipFlags;

            NumLines = 0;
            if (cOr)
            {
                if (cAnd)
                {
                    goto ClippedOut;
                }

                /* Can't get more lines out than in, but can grow vertices */

                /* Check we're not running out of space for new vertices */
                if (CamVerts->numUsed + 12 > CamVerts->numAlloced)
                {
                    /* TODO: Is this a good estimate? */
                    i = CamVerts->numAlloced + (OldNumLines - CurLine) +
                        12;
                    result = (NULL != RxClusterResizeData(CamVerts, i));
                    if (!result)
                    {
                        goto ClipEnd;
                    }
                }
                if (DevVerts->numUsed + 12 > DevVerts->numAlloced)
                {
                    /* TODO: Is this a good estimate? */
                    i = DevVerts->numAlloced + (OldNumLines - CurLine) +
                        12;
                    result = (NULL != RxClusterResizeData(DevVerts, i));
                    if (!result)
                    {
                        goto ClipEnd;
                    }
                }

                OriginalIndices =
                    (RxVertexIndex *) (Indices->currentData);

                /* Set it up */
                nPntInVerts = 0;
                nClipBuffer[0] = Index1;
                nClipBuffer[1] = Index2;

                nPntOutVerts = 2;

                if (cOr & (rwZLOCLIP | rwZHICLIP))
                {

                    /***********************  Z LO ***********************/
                    if (cOr & (RwUInt8) rwZLOCLIP)
                    {
                        if (_rwForOneEdge(CamVerts, DevVerts,
                                          nClipBuffer, &nPntInVerts,
                                          &nPntOutVerts, rwZLOCLIP,
                                          PerspClipFuncs.ZLOFunc) < 2)
                        {
                            /* Clipped out */
                            goto ClippedOut;
                        }
                    }

                    /***********************  Z HI ***********************/
                    if (cOr & (RwUInt8) rwZHICLIP)
                    {
                        if (_rwForOneEdge(CamVerts, DevVerts,
                                          nClipBuffer, &nPntInVerts,
                                          &nPntOutVerts, rwZHICLIP,
                                          PerspClipFuncs.ZHIFunc) < 2)
                        {
                            /* Clipped out */
                            goto ClippedOut;
                        }
                    }

                    /* TODO: To save all this indexing into CamVerts, surely it'd be better to
                     * have the new Or calculated inside _rwForAllEdges? */
                    /* Calculate new cOr */
                    cOr =
                        (RxClusterGetIndexedData(CamVerts,
                                                 RxCamSpace3DVertex,
                                                 nClipBuffer
                                                 [nPntInVerts]))->
                        clipFlags |
                        (RxClusterGetIndexedData
                         (CamVerts, RxCamSpace3DVertex,
                          nClipBuffer[(nPntInVerts +
                                       1) & 15]))->clipFlags;
                }

                if (cOr & (rwYLOCLIP | rwYHICLIP))
                {

                    /***********************  Y LO ***********************/
                    if (cOr & (RwUInt8) rwYLOCLIP)
                    {
                        if (_rwForOneEdge(CamVerts, DevVerts,
                                          nClipBuffer, &nPntInVerts,
                                          &nPntOutVerts, rwYLOCLIP,
                                          PerspClipFuncs.YLOFunc) < 2)
                        {
                            /* Clipped out */
                            goto ClippedOut;
                        }
                    }

                    /***********************  Y HI ***********************/
                    if (cOr & (RwUInt8) rwYHICLIP)
                    {
                        if (_rwForOneEdge(CamVerts, DevVerts,
                                          nClipBuffer, &nPntInVerts,
                                          &nPntOutVerts, rwYHICLIP,
                                          PerspClipFuncs.YHIFunc) < 2)
                        {
                            /* Clipped out */
                            goto ClippedOut;
                        }
                    }

                    /* Calculate new cOr */
                    cOr =
                        (RxClusterGetIndexedData(CamVerts,
                                                 RxCamSpace3DVertex,
                                                 nClipBuffer
                                                 [nPntInVerts]))->
                        clipFlags |
                        (RxClusterGetIndexedData
                         (CamVerts, RxCamSpace3DVertex,
                          nClipBuffer[(nPntInVerts +
                                       1) & 15]))->clipFlags;
                }

                if (cOr & (rwXLOCLIP | rwXHICLIP))
                {

                    /***********************  X LO ***********************/
                    if (cOr & (RwUInt8) rwXLOCLIP)
                    {
                        if (_rwForOneEdge(CamVerts, DevVerts,
                                          nClipBuffer, &nPntInVerts,
                                          &nPntOutVerts, rwXLOCLIP,
                                          PerspClipFuncs.XLOFunc) < 2)
                        {
                            /* Clipped out */
                            goto ClippedOut;
                        }
                    }

                    /***********************  X HI ***********************/
                    if (cOr & (RwUInt8) rwXHICLIP)
                    {
                        if (_rwForOneEdge(CamVerts, DevVerts,
                                          nClipBuffer, &nPntInVerts,
                                          &nPntOutVerts, rwXHICLIP,
                                          PerspClipFuncs.XHIFunc) < 2)
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
                        RxClusterGetIndexedData(CamVerts,
                                                RxCamSpace3DVertex,
                                                nClipBuffer[i]);

                    cFlags = CamVert1->clipFlags;

                    if (cFlags & (rwZCLIP | rwYCLIP | rwXCLIP))
                    {
                        RwReal              nX, nY, nZ, nRecipZ;
                        RxScrSpace2DVertex *DevVertex;

                        DevVertex =
                            RxClusterGetIndexedData(DevVerts,
                                                    RxScrSpace2DVertex,
                                                    nClipBuffer[i]);

                        nX = CamVert1->cameraVertex.x;
                        nY = CamVert1->cameraVertex.y;
                        nZ = CamVert1->cameraVertex.z;
                        nRecipZ = 1 / nZ;

                        /* Set up Pixel space values */
                        RwIm2DVertexSetCameraX(DevVertex, nX);
                        RwIm2DVertexSetCameraY(DevVertex, nY);
                        RwIm2DVertexSetCameraZ(DevVertex, nZ);
                        RwIm2DVertexSetRecipCameraZ(DevVertex, nRecipZ);

                        /* find screen column of pixel */
                        switch (cFlags & (RwUInt8) rwXCLIP)
                        {
                            case rwXHICLIP:
                                RwIm2DVertexSetScreenX(DevVertex,
                                                       _rwClipInfoGlobal.
                                                       camWidth +
                                                       _rwClipInfoGlobal.
                                                       camOffsetX);
                                break;
                            case rwXLOCLIP:
                                RwIm2DVertexSetScreenX(DevVertex,
                                                       _rwClipInfoGlobal.
                                                       camOffsetX);
                                break;
                            default:
                                RwIm2DVertexSetScreenX(DevVertex,
                                                       (_rwClipInfoGlobal.
                                                        camWidth * nX *
                                                        nRecipZ) +
                                                       _rwClipInfoGlobal.
                                                       camOffsetX);
                                break;
                        }

                        /* find screen row of pixel */
                        switch (cFlags & (rwYCLIP | rwXCLIP))
                        {
                            case rwYHICLIP:
                                RwIm2DVertexSetScreenY(DevVertex,
                                                       _rwClipInfoGlobal.
                                                       camHeight +
                                                       _rwClipInfoGlobal.
                                                       camOffsetY);
                                break;
                            case rwYLOCLIP:
                                RwIm2DVertexSetScreenY(DevVertex,
                                                       _rwClipInfoGlobal.
                                                       camOffsetY);
                                break;
                            default:
                                RwIm2DVertexSetScreenY(DevVertex,
                                                       (_rwClipInfoGlobal.
                                                        camHeight * nY *
                                                        nRecipZ) +
                                                       _rwClipInfoGlobal.
                                                       camOffsetY);
                                break;
                        }

                        /* find range of pixel */
                        switch (cFlags & (rwZCLIP | rwYCLIP | rwXCLIP))
                        {
                            case rwZHICLIP:
                                RwIm2DVertexSetScreenZ(DevVertex,
                                                       _rwClipInfoGlobal.
                                                       zBufferFar);
                                break;
                            case rwZLOCLIP:
                                RwIm2DVertexSetScreenZ(DevVertex,
                                                       _rwClipInfoGlobal.
                                                       zBufferNear);
                                break;
                            default:
                                RwIm2DVertexSetScreenZ(DevVertex,
                                                       (_rwClipInfoGlobal.zScale
                                                        * nRecipZ) +
                                                       _rwClipInfoGlobal.
                                                       zShift);
                                break;
                        }

                        /* Do texture coordinates */
                        RwIm2DVertexSetU(DevVertex, CamVert1->u,
                                         nRecipZ);
                        RwIm2DVertexSetV(DevVertex, CamVert1->v,
                                         nRecipZ);
                    }

                    /* Onto next vertex */
                    i = (i + 1) & 15;
                }
                while (i != nPntOutVerts);

                /* Handle case where we are not clipping color */
                if (!(rwSHADEMODEGOURAUD & rsvp->ShadeMode))
                {
                    /* TODO: What's the second test for? Rob? */
                    /* Rob's answer -
                     * Its to make sure you don't stop with the original 2nd
                     * vertex, and splat a color needed for another line.
                     * I haven't commented it back,
                     * because the clipper appears to have changed subtley.
                     */
                    while ((nClipBuffer[nPntInVerts] != Index1))
                        /* && (nClipBuffer[nPntInVerts] < (RwInt32)_rwPipeState.currentContext->firstFreeVertexIndex)) */
                    {
                        /* We need to rotate the vertex indices until the first vertex is
                         * either a generated one or the original first vertex - this
                         * prevents us overwriting a vertex color that a dispatched
                         * polygon relies on.
                         */
                        nClipBuffer[nPntOutVerts] =
                            nClipBuffer[nPntInVerts];
                        nPntOutVerts = (nPntOutVerts + 1) & 15;
                        nPntInVerts = (nPntInVerts + 1) & 15;
                    }

                    if (nClipBuffer[nPntInVerts] != Index1)
                    {
                        RwIm2DVertexCopyRGBA(RxClusterGetIndexedData
                                             (DevVerts,
                                              RxScrSpace2DVertex,
                                              nClipBuffer[nPntInVerts]),
                                             RxClusterGetIndexedData
                                             (DevVerts,
                                              RxScrSpace2DVertex,
                                              Index1));
                    }
                }

                /* Put the line back where it came from */
                i = nPntInVerts;
                j = (nPntInVerts + 1) & 15;
                OriginalIndices[0] = (RxVertexIndex) nClipBuffer[i];
                OriginalIndices[1] = (RxVertexIndex) nClipBuffer[j];

                NumLines = 1;
            }                  /* if (cOr) */
            else
            {
                NumLines = 1;
            }

          ClippedOut:

            /* If we clipped the line away completely, compact the array */
            if (NumLines == 0)
            {
                /* Copy the end line over this one and decrement Line and
                 * OldNumLines.
                 */
                RxVertexIndex      *LastOriginalLine;

                LastOriginalLine =
                    RxClusterGetIndexedData(Indices, RxVertexIndex,
                                            ((OldNumLines - 1) << 1));

                (RxClusterGetCursorData(Indices, RxVertexIndex))[0] =
                    LastOriginalLine[0];
                (RxClusterGetCursorData(Indices, RxVertexIndex))[1] =
                    LastOriginalLine[1];

                Indices->numUsed -= 2;
                OldNumLines--;
                CurLine--;

                /* Any vertices generated while clipping this line to nothing can be reused */
                CamVerts->numUsed = OldNumVertices;
                DevVerts->numUsed = OldNumVertices;
            }
            else
            {
                /* Onto the next line */
                RxClusterIncCursorByStride(Indices, LinesStride);
            }

          ClipEnd:

            if (!result)
            {
                RxPacketDestroy(Packet, Self);
                RWRETURN(result);
            }
        }

        /* Bring StateData up to date */
        MeshData->NumElements = Indices->numUsed >> 1;
        MeshData->NumVertices = CamVerts->numUsed;
    }                          /* if (MeshData->ClipFlagsOr) */

    if (MeshData->NumElements == 0)
    {
        /* This packet's lines have all been clipped into nonexistence,
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
ParallelClipProcessPacket(RxPipelineNodeInstance * Self,
                          RxPacket * Packet)
{
    /*
     * o We check that each line needs clipping (i.e, cOr != 0), but that it
     *   is not obviously clipped off screen beyond one plane (i.e cAnd == 0).
     *   + We assume that we will never generate more than 1 line per line :-)
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
    RwInt32             LinesStride;
    RxMeshStateVector  *MeshData;
    RxRenderStateVector *rsvp;
    RwUInt32            NumLines, OldNumLines, OldNumVertices;
    RxVertexIndex      *OriginalLine;
    RxVertexIndex       Index1, Index2;
    RxCamSpace3DVertex *CamVert1, *CamVert2;
    RwInt32             i, j;
    RwUInt32            CurLine;
    RxVertexIndex       nClipBuffer[16];
    RwInt32             nPntInVerts, nPntOutVerts;
    RwUInt8             cOr, cAnd;

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

    RWASSERT((CamVerts != NULL) && (CamVerts->numUsed > 0));
    RWASSERT((DevVerts != NULL) && (DevVerts->numUsed > 0));
    RWASSERT((Indices != NULL) && (Indices->numUsed > 0));
    RWASSERT((MeshState != NULL) && (MeshState->numUsed > 0));

    MeshData = RxClusterGetCursorData(MeshState, RxMeshStateVector);
    RWASSERT(MeshData->PrimType == rwPRIMTYPELINELIST);
    /* ClipLine can only handle rwPRIMTYPELINELIST primitives as of yet */

    LinesStride = Indices->stride << 1;

    if (MeshData->ClipFlagsOr)
    {
        OldNumLines = MeshData->NumElements;

        /*
         * NOTE:
         * StateData is invalidated during this loop and
         * corrected after it finishes
         */
        for (CurLine = 0; CurLine < OldNumLines; CurLine++)
        {
            Index1 =
                (RxClusterGetCursorData(Indices, RxVertexIndex))[0];
            Index2 =
                (RxClusterGetCursorData(Indices, RxVertexIndex))[1];

            /* TODO: You're accessing the CamVerts array in random-access
             * order just to retrieve ClipFlags here! Dude!! */

            CamVert1 =
                RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex,
                                        Index1);
            CamVert2 =
                RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex,
                                        Index2);

            OldNumVertices = CamVerts->numUsed;

            /* Set up cOr and cAnd */
            cOr = CamVert1->clipFlags | CamVert2->clipFlags;
            cAnd = CamVert1->clipFlags & CamVert2->clipFlags;

            NumLines = 0;
            if (cOr)
            {
                if (cAnd)
                {
                    goto ClippedOut;
                }

                /* Can't have more lines out than in, but can grow verts */

                /* Check we're not running out of space for new vertices */
                if (CamVerts->numUsed + 12 > CamVerts->numAlloced)
                {
                    /* TODO: Is this a good estimate? */
                    i = CamVerts->numAlloced + (OldNumLines - CurLine) +
                        12;
                    result = (NULL != RxClusterResizeData(CamVerts, i));
                    if (!result)
                    {
                        goto ClipEnd;
                    }
                }
                if (DevVerts->numUsed + 12 > DevVerts->numAlloced)
                {
                    /* TODO: Is this a good estimate? */
                    i = DevVerts->numAlloced + (OldNumLines - CurLine) +
                        12;
                    result = (NULL != RxClusterResizeData(DevVerts, i));
                    if (!result)
                    {
                        goto ClipEnd;
                    }
                }

                OriginalLine = (RxVertexIndex *) (Indices->currentData);

                /* Set it up */
                nPntInVerts = 0;
                nClipBuffer[0] = Index1;
                nClipBuffer[1] = Index2;

                nPntOutVerts = 2;

                if (cOr & (rwZLOCLIP | rwZHICLIP))
                {

                    /***********************  Z LO ***********************/
                    if (cOr & (RwUInt8) rwZLOCLIP)
                    {
                        if (_rwForOneEdge(CamVerts, DevVerts,
                                          nClipBuffer, &nPntInVerts,
                                          &nPntOutVerts, rwZLOCLIP,
                                          ParaClipFuncs.ZLOFunc) < 2)
                        {
                            /* Clipped out */
                            goto ClippedOut;
                        }
                    }

                    /***********************  Z HI ***********************/
                    if (cOr & (RwUInt8) rwZHICLIP)
                    {
                        if (_rwForOneEdge(CamVerts, DevVerts,
                                          nClipBuffer, &nPntInVerts,
                                          &nPntOutVerts, rwZHICLIP,
                                          ParaClipFuncs.ZHIFunc) < 2)
                        {
                            /* Clipped out */
                            goto ClippedOut;
                        }
                    }

                    /* TODO: To save all this indexing into CamVerts, surely it'd be better to
                     * have the new Or calculated inside _rwForAllEdges? */
                    /* Calculate new cOr */
                    cOr =
                        (RxClusterGetIndexedData(CamVerts,
                                                 RxCamSpace3DVertex,
                                                 nClipBuffer
                                                 [nPntInVerts]))->
                        clipFlags |
                        (RxClusterGetIndexedData
                         (CamVerts, RxCamSpace3DVertex,
                          nClipBuffer[(nPntInVerts +
                                       1) & 15]))->clipFlags;
                }

                if (cOr & (rwYLOCLIP | rwYHICLIP))
                {

                    /***********************  Y LO ***********************/
                    if (cOr & (RwUInt8) rwYLOCLIP)
                    {
                        if (_rwForOneEdge(CamVerts, DevVerts,
                                          nClipBuffer, &nPntInVerts,
                                          &nPntOutVerts, rwYLOCLIP,
                                          ParaClipFuncs.YLOFunc) < 2)
                        {
                            /* Clipped out */
                            goto ClippedOut;
                        }
                    }

                    /***********************  Y HI ***********************/
                    if (cOr & (RwUInt8) rwYHICLIP)
                    {
                        if (_rwForOneEdge(CamVerts, DevVerts,
                                          nClipBuffer, &nPntInVerts,
                                          &nPntOutVerts, rwYHICLIP,
                                          ParaClipFuncs.YHIFunc) < 2)
                        {
                            /* Clipped out */
                            goto ClippedOut;
                        }
                    }

                    /* Calculate new cOr */
                    cOr =
                        (RxClusterGetIndexedData(CamVerts,
                                                 RxCamSpace3DVertex,
                                                 nClipBuffer
                                                 [nPntInVerts]))->
                        clipFlags |
                        (RxClusterGetIndexedData
                         (CamVerts, RxCamSpace3DVertex,
                          nClipBuffer[(nPntInVerts +
                                       1) & 15]))->clipFlags;
                }

                if (cOr & (rwXLOCLIP | rwXHICLIP))
                {

                    /***********************  X LO ***********************/
                    if (cOr & (RwUInt8) rwXLOCLIP)
                    {
                        if (_rwForOneEdge(CamVerts, DevVerts,
                                          nClipBuffer, &nPntInVerts,
                                          &nPntOutVerts, rwXLOCLIP,
                                          ParaClipFuncs.XLOFunc) < 2)
                        {
                            /* Clipped out */
                            goto ClippedOut;
                        }
                    }

                    /***********************  X HI ***********************/
                    if (cOr & (RwUInt8) rwXHICLIP)
                    {
                        if (_rwForOneEdge(CamVerts, DevVerts,
                                          nClipBuffer, &nPntInVerts,
                                          &nPntOutVerts, rwXHICLIP,
                                          ParaClipFuncs.XHIFunc) < 2)
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
                        RxClusterGetIndexedData(CamVerts,
                                                RxCamSpace3DVertex,
                                                nClipBuffer[i]);

                    cFlags = CamVert1->clipFlags;

                    if (cFlags & (rwZCLIP | rwYCLIP | rwXCLIP))
                    {
                        RwReal              nX, nY, nZ;
                        RxScrSpace2DVertex *DevVertex;

                        DevVertex =
                            RxClusterGetIndexedData(DevVerts,
                                                    RxScrSpace2DVertex,
                                                    nClipBuffer[i]);

                        nX = CamVert1->cameraVertex.x;
                        nY = CamVert1->cameraVertex.y;
                        nZ = CamVert1->cameraVertex.z;

                        /* Set up Pixel space values */
                        RwIm2DVertexSetCameraX(DevVertex, nX);
                        RwIm2DVertexSetCameraY(DevVertex, nY);
                        RwIm2DVertexSetCameraZ(DevVertex, nZ);
                        RwIm2DVertexSetRecipCameraZ(DevVertex,
                                                    (RwReal) 1.0);

                        /* find screen column of pixel */
                        switch (cFlags & (RwUInt8) rwXCLIP)
                        {
                            case rwXHICLIP:
                                RwIm2DVertexSetScreenX(DevVertex,
                                                       _rwClipInfoGlobal.
                                                       camWidth +
                                                       _rwClipInfoGlobal.
                                                       camOffsetX);
                                break;
                            case rwXLOCLIP:
                                RwIm2DVertexSetScreenX(DevVertex,
                                                       _rwClipInfoGlobal.
                                                       camOffsetX);
                                break;
                            default:
                                RwIm2DVertexSetScreenX(DevVertex,
                                                       (_rwClipInfoGlobal.
                                                        camWidth * nX) +
                                                       _rwClipInfoGlobal.
                                                       camOffsetX);
                                break;
                        }

                        /* find screen row of pixel */
                        switch (cFlags & (rwYCLIP | rwXCLIP))
                        {
                            case rwYHICLIP:
                                RwIm2DVertexSetScreenY(DevVertex,
                                                       _rwClipInfoGlobal.
                                                       camHeight +
                                                       _rwClipInfoGlobal.
                                                       camOffsetY);
                                break;
                            case rwYLOCLIP:
                                RwIm2DVertexSetScreenY(DevVertex,
                                                       _rwClipInfoGlobal.
                                                       camOffsetY);
                                break;
                            default:
                                RwIm2DVertexSetScreenY(DevVertex,
                                                       (_rwClipInfoGlobal.
                                                        camHeight *
                                                        nY) +
                                                       _rwClipInfoGlobal.
                                                       camOffsetY);
                                break;
                        }

                        /* find range of pixel */
                        switch (cFlags & (rwZCLIP | rwYCLIP | rwXCLIP))
                        {
                            case rwZHICLIP:
                                RwIm2DVertexSetScreenZ(DevVertex,
                                                       _rwClipInfoGlobal.
                                                       zBufferFar);
                                break;
                            case rwZLOCLIP:
                                RwIm2DVertexSetScreenZ(DevVertex,
                                                       _rwClipInfoGlobal.
                                                       zBufferNear);
                                break;
                            default:
                                RwIm2DVertexSetScreenZ(DevVertex,
                                                       (_rwClipInfoGlobal.zScale
                                                        * nZ) +
                                                       _rwClipInfoGlobal.
                                                       zShift);
                                break;
                        }

                        /* Do texture coordinates */
                        RwIm2DVertexSetU(DevVertex, CamVert1->u,
                                         (RwReal) 1.0);
                        RwIm2DVertexSetV(DevVertex, CamVert1->v,
                                         (RwReal) 1.0);
                    }

                    /* Onto next vertex */
                    i = (i + 1) & 15;
                }
                while (i != nPntOutVerts);

                /* Handle case where we are not clipping color */
                if (!(rwSHADEMODEGOURAUD & rsvp->ShadeMode))
                {
                    /* TODO: What's the second test for? Rob? */
                    /* Rob's answer -
                     * Its to make sure you don't stop with the original 2nd
                     * vertex, and splat a color needed for another line.
                     * I haven't commented it back,
                     * because the clipper appears to have changed subtley.
                     */
                    while ((nClipBuffer[nPntInVerts] != Index1))
                        /* && (nClipBuffer[nPntInVerts] < (RwInt32)_rwPipeState.currentContext->firstFreeVertexIndex)) */
                    {
                        /* We need to rotate the vertex indices until the first vertex is
                         * either a generated one or the original first vertex - this
                         * prevents us overwriting a vertex color that a dispatched
                         * polygon relies on.
                         */
                        nClipBuffer[nPntOutVerts] =
                            nClipBuffer[nPntInVerts];
                        nPntOutVerts = (nPntOutVerts + 1) & 15;
                        nPntInVerts = (nPntInVerts + 1) & 15;
                    }

                    if (nClipBuffer[nPntInVerts] != Index1)
                    {
                        RwIm2DVertexCopyRGBA(RxClusterGetIndexedData
                                             (DevVerts,
                                              RxScrSpace2DVertex,
                                              nClipBuffer[nPntInVerts]),
                                             RxClusterGetIndexedData
                                             (DevVerts,
                                              RxScrSpace2DVertex,
                                              Index1));
                    }
                }

                /* Stick the line back over it's original */
                i = nPntInVerts;
                j = (nPntInVerts + 1) & 15;
                OriginalLine[0] = (RxVertexIndex) nClipBuffer[i];
                OriginalLine[1] = (RxVertexIndex) nClipBuffer[j];
                NumLines = 1;
            }                  /* if (cOr) */
            else
            {
                NumLines = 1;
            }

          ClippedOut:

            /* If we clipped the line away completely, compact the array */
            if (NumLines == 0)
            {
                /* Copy the end line over this one and decrement Line and
                 * OldNumLines.
                 */
                RxVertexIndex      *LastOriginalLine;

                LastOriginalLine =
                    RxClusterGetIndexedData(Indices, RxVertexIndex,
                                            (OldNumLines - 1) << 1);

                (RxClusterGetCursorData(Indices, RxVertexIndex))[0] =
                    LastOriginalLine[0];
                (RxClusterGetCursorData(Indices, RxVertexIndex))[1] =
                    LastOriginalLine[1];

                Indices->numUsed -= 2;
                OldNumLines--;
                CurLine--;

                /* Any vertices generated while clipping this line to nothing can be reused */
                CamVerts->numUsed = OldNumVertices;
                DevVerts->numUsed = OldNumVertices;
            }
            else
            {
                /* Onto the next line */
                RxClusterIncCursorByStride(Indices, LinesStride);
            }

          ClipEnd:

            if (!result)
            {
                RxPacketDestroy(Packet, Self);
                RWRETURN(result);
            }
        }

        /* Bring StateData up to date */
        MeshData->NumElements = Indices->numUsed >> 1;
        MeshData->NumVertices = CamVerts->numUsed;
    }                          /* if (MeshData->ClipFlagsOr) */

    if (MeshData->NumElements == 0)
    {
        /* This packet's lines have all been clipped into nonexistence,
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
 LineClipNode

 on entry: a packet containing perspective or parallel triangles to clip
 on exit :
 */

static RwBool
LineClipNode( RxPipelineNodeInstance * self,
              const RxPipelineNodeParam * params __RWUNUSED__ )
{
    RwBool              result = TRUE;
    RwCamera           *curCamera;
    RwRaster           *rpRas;
    RwUInt32            Persp;
    RxPacket           *Packet;
    RwInt32             width;
    RwInt32             height;
    ClipProcessPacketFunc func;
    static ClipProcessPacketFunc ClipFunc[2] = {
        ParallelClipProcessPacket,
        PerspectiveClipProcessPacket
    };

    RWFUNCTION(RWSTRING("LineClipNode"));

    /* TODO:
     * global shared camera bad for multithreading with multiple viewports
     */
    curCamera = (RwCamera *) RWSRCGLOBAL(curCamera);
    RWASSERT(NULL != curCamera);

    rpRas = RwCameraGetRaster(curCamera);
    Persp = (curCamera->projectionType == rwPERSPECTIVE) ? 1 : 0;

    func = ClipFunc[Persp];

    /*
     * Set up oft-used clipping numeros
     * (TODO: shared with p2clpcom.c as a global - bad for multithreading)
     */
    _rwClipInfoGlobal.zScale = curCamera->zScale;
    _rwClipInfoGlobal.zShift = curCamera->zShift;
    width = RwRasterGetWidth(rpRas);
    _rwClipInfoGlobal.camWidth = (RwReal) width;
    height = RwRasterGetHeight(rpRas);
    _rwClipInfoGlobal.camHeight = (RwReal) height;
    _rwClipInfoGlobal.camOffsetX = (RwReal) rpRas->nOffsetX;
    _rwClipInfoGlobal.camOffsetY = (RwReal) rpRas->nOffsetY;
    _rwClipInfoGlobal.nearClip = curCamera->nearPlane;
    _rwClipInfoGlobal.farClip = curCamera->farPlane;
    _rwClipInfoGlobal.zBufferNear = RwIm2DGetNearScreenZ();
    _rwClipInfoGlobal.zBufferFar = RwIm2DGetFarScreenZ();

#ifdef SKY2_DRVMODEL_H
    /* On the PlayStation 2, we should munge the camera matrix and
     * clipping info such that
     * devverts are clipped to the large [0,4096] overdraw frustum */
#endif

    Packet = (RxPacket *) RxPacketFetch(self);
    RWASSERT(NULL != Packet);

    result = func(self, Packet);

    /* RWCRTCHECKMEMORY(); */

    RWRETURN(result);
}

/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionGetClipLine returns a pointer to a node implementing
 * line clipping in custom pipelines.
 *
 * This node clips lines to the frustum of the current camera (as defined by
 * the camera's ViewWindow, ViewOffset and Projection type - parallel or
 * perspective).
 *
 * This node is meant to be used after a transform node, so that the camera
 * space and (un-clipped) screen space vertices will contain valid data. It
 * clips all values associated with the lines/vertices (that is, position,
 * texture coordinates and RGBA values) in 3D camera space so that the results
 * of perspective-correct interpolation by the rasterizer will be correct.
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
 * If the render state cluster is not present (or contains no data) then the
 * node assumes that the packet's render state is the default render state,
 * obtained through RxRenderStateVectorGetDefaultRenderStateVector(). This is
 * used in this case to determine if lines are to be Gouraud shaded (hence
 * color should be interpolated when lines are clipped) or flat shaded.
 *
 * If all the lines in a packet are completely clipped away (they all
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
 * \verbatim
   RxClCamSpace3DVertices - required
   RxClScrSpace2DVertices - required
   RxClIndices            - required
   RxClMeshState          - required
   RxClRenderState        - optional
   \endverbatim
 *
 * The characteristics of the first of this node's outputs:

 * \verbatim
   RxClCamSpace3DVertices - valid
   RxClScrSpace2DVertices - valid
   RxClIndices            - valid
   RxClMeshState          - valid
   RxClRenderState        - no change
   \endverbatim
 *
 * The characteristics of the second of this node's outputs:
 *
 * \verbatim
   RxClCamSpace3DVertices - valid
   RxClScrSpace2DVertices - valid
   RxClIndices            - invalid
   RxClMeshState          - valid
   RxClRenderState        - no change
   \endverbatim
 *
 * \return pointer to node for line clipping in custom pipelines on success,
 * NULL otherwise
 *
 * \see RxNodeDefinitionGetClipTriangle
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
 */

RxNodeDefinition   *
RxNodeDefinitionGetClipLine(void)
{

    static RxClusterRef N5clofinterest[] = { /* */
        {&RxClCamSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClScrSpace2DVertices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClIndices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClRenderState, rxCLALLOWABSENT, rxCLRESERVED}
    };

#define NUMCLUSTERSOFINTEREST \
        ((sizeof(N5clofinterest))/(sizeof(N5clofinterest[0])))

    static RxClusterValidityReq N5inputreqs[NUMCLUSTERSOFINTEREST] = { /* */
        rxCLREQ_REQUIRED,
        rxCLREQ_REQUIRED,
        rxCLREQ_REQUIRED,
        rxCLREQ_REQUIRED,
        rxCLREQ_OPTIONAL
    };

    static RxClusterValid N5outcl1[NUMCLUSTERSOFINTEREST] = { /* */
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_NOCHANGE
    };

    static RxClusterValid N5outcl2[NUMCLUSTERSOFINTEREST] = { /* */
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_INVALID,
        rxCLVALID_VALID,
        rxCLVALID_NOCHANGE
    };

    static RwChar       _ClipOut[] = RWSTRING("ClipOut");
    static RwChar       _ClipAllClipped[] = RWSTRING("ClipAllClipped");

    static RxOutputSpec N5outputs[] = { /* */
        {_ClipOut,
         N5outcl1,
         rxCLVALID_NOCHANGE},
        {_ClipAllClipped,
         N5outcl2,
         rxCLVALID_NOCHANGE}
    };

#define NUMOUTPUTS \
        ((sizeof(N5outputs))/(sizeof(N5outputs[0])))

    static RwChar       _ClipLine_csl[] = RWSTRING("ClipLine.csl");

    static RxNodeDefinition nodeClipLineCSL = { /* */
        _ClipLine_csl,
        {LineClipNode,
         (RxNodeInitFn) NULL,
         (RxNodeTermFn) NULL,
         (RxPipelineNodeInitFn) NULL,
         (RxPipelineNodeTermFn) NULL,
         (RxPipelineNodeConfigFn) NULL,
         (RxConfigMsgHandlerFn) NULL},
        {NUMCLUSTERSOFINTEREST,
         N5clofinterest,
         N5inputreqs,
         NUMOUTPUTS,
         N5outputs},
        0,
        (RxNodeDefEditable) FALSE,
        0
    };

    RxNodeDefinition   *result = &nodeClipLineCSL;

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionGetClipLine"));

    /*RWMESSAGE((RWSTRING("result %p"), result)); */

    RWRETURN(result);
}
