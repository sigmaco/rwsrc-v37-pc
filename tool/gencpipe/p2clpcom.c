/****************************************************************************
 *                                                                          *
 * module : p2clpcom.c                                                      *
 *                                                                          *
 * purpose: Clipping                                                        *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 includes
 */

/* #include <assert.h> */
#include <float.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rwcore.h"

#include "rpdbgerr.h"

#include "p2clpcom.h"

#include "rtgncpip.h"

/****************************************************************************
 local types
 */

/****************************************************************************
 local (static) prototypes
 */

/****************************************************************************
 local defines
 */

/****************************************************************************
 globals (across program)
 */

RwClipInfo          _rwClipInfoGlobal;

/****************************************************************************
 local (static) globals
 */

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

typedef   RwInt32
  (*ForAllEdgesFunc)
    (RxCluster * CamVerts, RxCluster * DevVerts,
     RxCluster * RxInterpolants, RxVertexIndex * npClipBuffer,
     RwInt32 * npInIndex, RwInt32 * npOutIndex,
     RwInt32 clipCode, ClipEdgeFn clipit);

RwInt32
_rwForAllEdgesWithRxInterpolant(RxCluster * CamVerts,
                                RxCluster * DevVerts,
                                RxCluster * RxInterpolants,
                                RxVertexIndex * npClipBuffer,
                                RwInt32 * npInIndex,
                                RwInt32 * npOutIndex,
                                RwInt32 clipCode, ClipEdgeFn clipit)
{
    RwInt32             result;
    RwInt32             nPntInVerts = *npInIndex;
    RwInt32             nPntOutVerts = *npOutIndex;
    RxCamSpace3DVertex *cvpThis, *cvpNext, *cvpClip;
    RxScrSpace2DVertex *dvpThis, *dvpNext, *dvpClip;
    RxVertexIndex       thisIndex, nextIndex, clipIndex;
    RwInt32             nInEnd = nPntOutVerts;
    RwInt32             clipTest;

    RWFUNCTION(RWSTRING("_rwForAllEdgesWithRxInterpolant"));

    thisIndex = npClipBuffer[(nPntOutVerts - 1) & 15];
    nextIndex = npClipBuffer[nPntInVerts];
    cvpThis =
        RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex, thisIndex);
    dvpThis =
        RxClusterGetIndexedData(DevVerts, RxScrSpace2DVertex, thisIndex);
    cvpNext =
        RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex, nextIndex);
    dvpNext =
        RxClusterGetIndexedData(DevVerts, RxScrSpace2DVertex, nextIndex);

    do
    {
        clipTest = (cvpThis->clipFlags & clipCode);

        if (!clipTest)
        {
            /* This point makes it */
            npClipBuffer[nPntOutVerts] = thisIndex;
            nPntOutVerts = (nPntOutVerts + 1) & 15;
        }

        clipTest = ((cvpThis->clipFlags) ^ (cvpNext->clipFlags)) & clipCode;

        if (clipTest)
        {
            RxInterp           *RxInterpolant =
                RxClusterGetCursorData(RxInterpolants, RxInterp);

            /* Generate and add a clipped vertex */
            clipIndex = (RxVertexIndex) RxClusterGetFreeIndex(CamVerts);
            RxClusterGetFreeIndex(DevVerts);
            npClipBuffer[nPntOutVerts] = clipIndex;
            nPntOutVerts = (nPntOutVerts + 1) & 15;

            cvpClip =
                RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex,
                                        clipIndex);
            dvpClip =
                RxClusterGetIndexedData(DevVerts, RxScrSpace2DVertex,
                                        clipIndex);

            /* Clip from in to out of viewport always to reduce cracking */
            clipTest = cvpThis->clipFlags & clipCode;

            if (clipTest)
            {
                RxInterpolant->interp =
                    clipit(cvpClip, cvpThis, cvpNext, dvpClip,
                           dvpThis, dvpNext);
                RxInterpolant->originalVert = clipIndex;
                RxInterpolant->parentVert1 = thisIndex;
                RxInterpolant->parentVert2 = nextIndex;
            }
            else
            {
                RxInterpolant->interp =
                    clipit(cvpClip, cvpNext, cvpThis, dvpClip,
                           dvpNext, dvpThis);
                RxInterpolant->originalVert = clipIndex;
                RxInterpolant->parentVert1 = nextIndex;
                RxInterpolant->parentVert2 = thisIndex;
            }
            RxInterpolants->numUsed++;
            RxClusterIncCursor(RxInterpolants);
        }

        /* Next edge */
        nPntInVerts = (nPntInVerts + 1) & 15;

        thisIndex = nextIndex;
        cvpThis = cvpNext;
        dvpThis = dvpNext;

        nextIndex = npClipBuffer[nPntInVerts];
        cvpNext =
            RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex, nextIndex);
        dvpNext =
            RxClusterGetIndexedData(DevVerts, RxScrSpace2DVertex, nextIndex);
    }
    while (nPntInVerts != nInEnd);

    *npInIndex = nPntInVerts;
    *npOutIndex = nPntOutVerts;

    result = ((nPntOutVerts - nPntInVerts) & 15);

    RWRETURN(result);
}

RwInt32
_rwForAllEdgesWithoutRxInterpolant(RxCluster * CamVerts,
                                   RxCluster * DevVerts,
                                   RxCluster * RxInterpolants __RWUNUSED__,
                                   RxVertexIndex * npClipBuffer,
                                   RwInt32 * npInIndex,
                                   RwInt32 * npOutIndex,
                                   RwInt32 clipCode, ClipEdgeFn clipit)
{
    RwInt32             result;
    RwInt32             nPntInVerts = *npInIndex;
    RwInt32             nPntOutVerts = *npOutIndex;
    RxCamSpace3DVertex *cvpThis, *cvpNext, *cvpClip;
    RxScrSpace2DVertex *dvpThis, *dvpNext, *dvpClip;
    RxVertexIndex       thisIndex, nextIndex, clipIndex;
    RwInt32             nInEnd = nPntOutVerts;
    RwInt32             clipTest;

    RWFUNCTION(RWSTRING("_rwForAllEdgesWithoutRxInterpolant"));

    thisIndex = npClipBuffer[(nPntOutVerts - 1) & 15];
    nextIndex = npClipBuffer[nPntInVerts];
    cvpThis =
        RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex, thisIndex);
    dvpThis =
        RxClusterGetIndexedData(DevVerts, RxScrSpace2DVertex, thisIndex);
    cvpNext =
        RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex, nextIndex);
    dvpNext =
        RxClusterGetIndexedData(DevVerts, RxScrSpace2DVertex, nextIndex);

    do
    {
        clipTest = (cvpThis->clipFlags & clipCode);

        if (!clipTest)
        {
            /* This point makes it */
            npClipBuffer[nPntOutVerts] = thisIndex;
            nPntOutVerts = (nPntOutVerts + 1) & 15;
        }

        clipTest = ((cvpThis->clipFlags) ^ (cvpNext->clipFlags)) & clipCode;

        if (clipTest)
        {
            /* Generate and add a clipped vertex */
            clipIndex = (RxVertexIndex) RxClusterGetFreeIndex(CamVerts);
            RxClusterGetFreeIndex(DevVerts);
            npClipBuffer[nPntOutVerts] = clipIndex;
            nPntOutVerts = (nPntOutVerts + 1) & 15;

            cvpClip =
                RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex,
                                        clipIndex);
            dvpClip =
                RxClusterGetIndexedData(DevVerts, RxScrSpace2DVertex,
                                        clipIndex);

            /* Clip from in to out of viewport always to reduce cracking */
            clipTest = cvpThis->clipFlags & clipCode;

            if (clipTest)
            {
                clipit(cvpClip, cvpThis, cvpNext, dvpClip, dvpThis, dvpNext);
            }
            else
            {
                clipit(cvpClip, cvpNext, cvpThis, dvpClip, dvpNext, dvpThis);
            }
        }

        /* Next edge */
        nPntInVerts = (nPntInVerts + 1) & 15;

        thisIndex = nextIndex;
        cvpThis = cvpNext;
        dvpThis = dvpNext;

        nextIndex = npClipBuffer[nPntInVerts];
        cvpNext =
            RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex, nextIndex);
        dvpNext =
            RxClusterGetIndexedData(DevVerts, RxScrSpace2DVertex, nextIndex);
    }
    while (nPntInVerts != nInEnd);

    *npInIndex = nPntInVerts;
    *npOutIndex = nPntOutVerts;

    result = ((nPntOutVerts - nPntInVerts) & 15);

    RWRETURN(result);

}

/****************************************************************************
 _rwForAllEdges

 on entry: clip vertex indices, in index, out index, clip code,
           function to make clipped vertex
 on exit : none
 */

RwInt32
_rwForAllEdges(RxCluster * CamVerts, RxCluster * DevVerts,
               RxCluster * RxInterpolants, RxVertexIndex * npClipBuffer,
               RwInt32 * npInIndex, RwInt32 * npOutIndex,
               RwInt32 clipCode, ClipEdgeFn clipit)
{
    RwInt32             result;
    ForAllEdgesFunc     func;

    RWFUNCTION(RWSTRING("_rwForAllEdges"));

    func =
        RxInterpolants ?
        _rwForAllEdgesWithRxInterpolant : _rwForAllEdgesWithoutRxInterpolant;

    result = func(CamVerts, DevVerts,
                  RxInterpolants, npClipBuffer,
                  npInIndex, npOutIndex, clipCode, clipit);

    RWRETURN(result);
}

/****************************************************************************
 _rwForOneEdge

 on entry: clip vertex indices, in index, out index, clip code, function to make clipped vertex
 on exit : none
 */

RwInt32
_rwForOneEdge(RxCluster * CamVerts, RxCluster * DevVerts,
              RxVertexIndex * npClipBuffer, RwInt32 * npInIndex,
              RwInt32 * npOutIndex, RwInt32 clipCode, ClipEdgeFn clipit)
{
    RwInt32             result;
    RwInt32             nPntInVerts = *npInIndex;
    RwInt32             nPntOutVerts = *npOutIndex;
    RxCamSpace3DVertex *cvpThis, *cvpNext, *cvpClip;
    RxScrSpace2DVertex *dvpThis, *dvpNext, *dvpClip;
    RxVertexIndex       thisIndex, nextIndex, clipIndex;
    RwInt32             nInEnd = nPntOutVerts;
    RwInt32             clipTest;

    RWFUNCTION(RWSTRING("_rwForOneEdge"));

    thisIndex = npClipBuffer[(nPntInVerts + 1) & 15];
    nextIndex = npClipBuffer[nPntInVerts];
    cvpThis =
        RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex, thisIndex);
    dvpThis =
        RxClusterGetIndexedData(DevVerts, RxScrSpace2DVertex, thisIndex);
    cvpNext =
        RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex, nextIndex);
    dvpNext =
        RxClusterGetIndexedData(DevVerts, RxScrSpace2DVertex, nextIndex);

    clipTest = cvpThis->clipFlags & clipCode;

    if (!clipTest)
    {
        /* This point makes it */
        npClipBuffer[nPntOutVerts] = thisIndex;
        nPntOutVerts = (nPntOutVerts + 1) & 15;
    }

    clipTest = ((cvpThis->clipFlags) ^ (cvpNext->clipFlags)) & clipCode;

    if (clipTest)
    {
        /* Generate and add a clipped vertex */
        clipIndex = (RxVertexIndex) RxClusterGetFreeIndex(CamVerts);
        RxClusterGetFreeIndex(DevVerts);
        npClipBuffer[nPntOutVerts] = clipIndex;
        nPntOutVerts = (nPntOutVerts + 1) & 15;

        cvpClip =
            RxClusterGetIndexedData(CamVerts, RxCamSpace3DVertex, clipIndex);
        dvpClip =
            RxClusterGetIndexedData(DevVerts, RxScrSpace2DVertex, clipIndex);

        /* Clip from in to out of viewport always to reduce cracking */
        clipTest = cvpThis->clipFlags & clipCode;

        if (clipTest)
        {
            clipit(cvpClip, cvpThis, cvpNext, dvpClip, dvpThis, dvpNext);
        }
        else
        {
            clipit(cvpClip, cvpNext, cvpThis, dvpClip, dvpNext, dvpThis);
        }
    }

    clipTest = (cvpNext->clipFlags & clipCode);

    if (!clipTest)
    {
        /* This point makes it */
        npClipBuffer[nPntOutVerts] = nextIndex;
        nPntOutVerts = (nPntOutVerts + 1) & 15;
    }

    /* Next edge */
    nPntInVerts = nInEnd;

    *npInIndex = *npOutIndex;
    *npOutIndex = nPntOutVerts;

    result = (nPntOutVerts - nPntInVerts) & 15;

    RWRETURN(result);
}

/* Perspective clipped edges */

/****************************************************************************
 _rwGeneratePerspClippedVertexZLO

 on entry: vertices for clipped, near and far, both device and camera vertices
 on exit : none
 */

RwReal
_rwGeneratePerspClippedVertexZLO(RxCamSpace3DVertex * cvpClip,
                                 RxCamSpace3DVertex * cvpThis,
                                 RxCamSpace3DVertex * cvpNext,
                                 RxScrSpace2DVertex * dvpClip,
                                 RxScrSpace2DVertex * dvpThis,
                                 RxScrSpace2DVertex * dvpNext)
{
    /* Calculate clipper value (t along edge) */
    RwReal              nRxInterp = RWCALCINTERP(_rwClipInfoGlobal.nearClip,
                                                 cvpThis->cameraVertex.z,
                                                 cvpNext->cameraVertex.z);

    RWFUNCTION(RWSTRING("_rwGeneratePerspClippedVertexZLO"));

    /* Stick in the clipped info into free */
    cvpClip->cameraVertex.x =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.x, cvpNext->cameraVertex.x);
    cvpClip->cameraVertex.y =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.y, cvpNext->cameraVertex.y);
    cvpClip->cameraVertex.z = _rwClipInfoGlobal.nearClip;

    /* Clip additional fields */
    RwCameraVertexClipTex(cvpClip, nRxInterp, cvpThis, cvpNext);
    RwIm2DVertexClipRGBA(dvpClip, nRxInterp, dvpThis, dvpNext);

    /* Work out clip codes */
    cvpClip->clipFlags = (RwUInt8) rwZLOCLIP;
    cvpClip->clipFlags |= RWCLIPCODEPERSPY(cvpClip->cameraVertex);
    cvpClip->clipFlags |= RWCLIPCODEPERSPX(cvpClip->cameraVertex);

    RWRETURN(nRxInterp);
}

/****************************************************************************
 _rwGeneratePerspClippedVertexZHI

 on entry: vertices for clipped, near and far, both device and camera vertices
 on exit : none
 */

RwReal
_rwGeneratePerspClippedVertexZHI(RxCamSpace3DVertex * cvpClip,
                                 RxCamSpace3DVertex * cvpThis,
                                 RxCamSpace3DVertex * cvpNext,
                                 RxScrSpace2DVertex * dvpClip,
                                 RxScrSpace2DVertex * dvpThis,
                                 RxScrSpace2DVertex * dvpNext)
{

    /* Calculate clipper value (t along edge) */
    RwReal              nRxInterp = RWCALCINTERP(_rwClipInfoGlobal.farClip,
                                                 cvpThis->cameraVertex.z,
                                                 cvpNext->cameraVertex.z);

    RWFUNCTION(RWSTRING("_rwGeneratePerspClippedVertexZHI"));

    /* Stick in the clipped info into free */
    cvpClip->cameraVertex.x =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.x, cvpNext->cameraVertex.x);
    cvpClip->cameraVertex.y =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.y, cvpNext->cameraVertex.y);
    cvpClip->cameraVertex.z = _rwClipInfoGlobal.farClip;

    /* Clip additional fields */
    RwCameraVertexClipTex(cvpClip, nRxInterp, cvpThis, cvpNext);
    RwIm2DVertexClipRGBA(dvpClip, nRxInterp, dvpThis, dvpNext);

    /* Work out clip codes */
    cvpClip->clipFlags = (RwUInt8) rwZHICLIP;
    cvpClip->clipFlags |= RWCLIPCODEPERSPY(cvpClip->cameraVertex);
    cvpClip->clipFlags |= RWCLIPCODEPERSPX(cvpClip->cameraVertex);

    RWRETURN(nRxInterp);
}

/****************************************************************************
 _rwGeneratePerspClippedVertexYLO

 on entry: vertices for clipped, near and far, both device and camera vertices
 on exit : none
 */

RwReal
_rwGeneratePerspClippedVertexYLO(RxCamSpace3DVertex * cvpClip,
                                 RxCamSpace3DVertex * cvpThis,
                                 RxCamSpace3DVertex * cvpNext,
                                 RxScrSpace2DVertex * dvpClip,
                                 RxScrSpace2DVertex * dvpThis,
                                 RxScrSpace2DVertex * dvpNext)
{

    /* Calculate clipper value (t along edge) */
    RwReal              nRxInterp = RWCALCINTERPZERO(cvpThis->cameraVertex.y,
                                                     cvpNext->cameraVertex.y);

    RWFUNCTION(RWSTRING("_rwGeneratePerspClippedVertexYLO"));

    /* Stick in the clipped info into free */
    cvpClip->cameraVertex.x =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.x, cvpNext->cameraVertex.x);
    cvpClip->cameraVertex.z =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.z, cvpNext->cameraVertex.z);
    cvpClip->cameraVertex.y = (RwReal) 0.0;

    /* Clip additional fields */
    RwCameraVertexClipTex(cvpClip, nRxInterp, cvpThis, cvpNext);
    RwIm2DVertexClipRGBA(dvpClip, nRxInterp, dvpThis, dvpNext);

    /* Work out clip codes */
    cvpClip->clipFlags =
        (cvpThis->clipFlags & ~(rwYCLIP | rwXCLIP)) | (RwUInt8) rwYLOCLIP;
    cvpClip->clipFlags |= RWCLIPCODEPERSPX(cvpClip->cameraVertex);

    RWRETURN(nRxInterp);
}

/****************************************************************************
 _rwGeneratePerspClippedVertexYHI

 on entry: vertices for clipped, near and far, both device and camera vertices
 on exit : none
 */

RwReal
_rwGeneratePerspClippedVertexYHI(RxCamSpace3DVertex * cvpClip,
                                 RxCamSpace3DVertex * cvpThis,
                                 RxCamSpace3DVertex * cvpNext,
                                 RxScrSpace2DVertex * dvpClip,
                                 RxScrSpace2DVertex * dvpThis,
                                 RxScrSpace2DVertex * dvpNext)
{

    /* Calculate clipper value (t along edge) */
    RwReal              nThisVal =

        cvpThis->cameraVertex.y - cvpThis->cameraVertex.z;
    RwReal              nNextVal =

        cvpNext->cameraVertex.y - cvpNext->cameraVertex.z;
    RwReal              nRxInterp = RWCALCINTERPZERO(nThisVal, nNextVal);

    RWFUNCTION(RWSTRING("_rwGeneratePerspClippedVertexYHI"));

    /* Stick in the clipped info into free */
    cvpClip->cameraVertex.x =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.x, cvpNext->cameraVertex.x);
    cvpClip->cameraVertex.z =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.z, cvpNext->cameraVertex.z);
    cvpClip->cameraVertex.y = cvpClip->cameraVertex.z;

    /* Clip additional fields */
    RwCameraVertexClipTex(cvpClip, nRxInterp, cvpThis, cvpNext);
    RwIm2DVertexClipRGBA(dvpClip, nRxInterp, dvpThis, dvpNext);

    /* Work out clip codes */
    cvpClip->clipFlags =
        (cvpThis->clipFlags & ~(rwYCLIP | rwXCLIP)) | (RwUInt8) rwYHICLIP;
    cvpClip->clipFlags |= RWCLIPCODEPERSPX(cvpClip->cameraVertex);

    RWRETURN(nRxInterp);
}

/****************************************************************************
 _rwGeneratePerspClippedVertexXLO

 on entry: vertices for clipped, near and far, both device and camera vertices
 on exit : none
 */

RwReal
_rwGeneratePerspClippedVertexXLO(RxCamSpace3DVertex * cvpClip,
                                 RxCamSpace3DVertex * cvpThis,
                                 RxCamSpace3DVertex * cvpNext,
                                 RxScrSpace2DVertex * dvpClip,
                                 RxScrSpace2DVertex * dvpThis,
                                 RxScrSpace2DVertex * dvpNext)
{

    /* Calculate clipper value (t along edge) */
    RwReal              nRxInterp = RWCALCINTERPZERO(cvpThis->cameraVertex.x,
                                                     cvpNext->cameraVertex.x);

    RWFUNCTION(RWSTRING("_rwGeneratePerspClippedVertexXLO"));

    /* Stick in the clipped info into free */
    cvpClip->cameraVertex.y =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.y, cvpNext->cameraVertex.y);
    cvpClip->cameraVertex.z =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.z, cvpNext->cameraVertex.z);
    cvpClip->cameraVertex.x = (RwReal) 0.0;

    /* Clip additional fields */
    RwCameraVertexClipTex(cvpClip, nRxInterp, cvpThis, cvpNext);
    RwIm2DVertexClipRGBA(dvpClip, nRxInterp, dvpThis, dvpNext);

    /* Work out clip codes */
    cvpClip->clipFlags =
        (cvpThis->clipFlags & ~(rwXCLIP)) | (RwUInt8) rwXLOCLIP;

    RWRETURN(nRxInterp);
}

/****************************************************************************
 _rwGeneratePerspClippedVertexXHI

 on entry: vertices for clipped, near and far, both device and camera vertices
 on exit : none
 */

RwReal
_rwGeneratePerspClippedVertexXHI(RxCamSpace3DVertex * cvpClip,
                                 RxCamSpace3DVertex * cvpThis,
                                 RxCamSpace3DVertex * cvpNext,
                                 RxScrSpace2DVertex * dvpClip,
                                 RxScrSpace2DVertex * dvpThis,
                                 RxScrSpace2DVertex * dvpNext)
{

    /* Calculate clipper value (t along edge) */
    RwReal              nThisVal =

        cvpThis->cameraVertex.x - cvpThis->cameraVertex.z;
    RwReal              nNextVal =

        cvpNext->cameraVertex.x - cvpNext->cameraVertex.z;
    RwReal              nRxInterp = RWCALCINTERPZERO(nThisVal, nNextVal);

    RWFUNCTION(RWSTRING("_rwGeneratePerspClippedVertexXHI"));

    /* Stick in the clipped info into free */
    cvpClip->cameraVertex.y =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.y, cvpNext->cameraVertex.y);
    cvpClip->cameraVertex.z =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.z, cvpNext->cameraVertex.z);
    cvpClip->cameraVertex.x = cvpClip->cameraVertex.z;

    /* Clip additional fields */
    RwCameraVertexClipTex(cvpClip, nRxInterp, cvpThis, cvpNext);
    RwIm2DVertexClipRGBA(dvpClip, nRxInterp, dvpThis, dvpNext);

    /* Work out clip codes */
    cvpClip->clipFlags =
        (cvpThis->clipFlags & ~(rwXCLIP)) | (RwUInt8) rwXHICLIP;

    RWRETURN(nRxInterp);
}

/* Parallel clipped edges */

/****************************************************************************
 _rwGenerateParallelZLOClippedVertex

 on entry: vertices for clipped, near and far, both device and camera vertices
 on exit : none
 */

RwReal
_rwGenerateParallelZLOClippedVertex(RxCamSpace3DVertex * cvpClip,
                                    RxCamSpace3DVertex * cvpThis,
                                    RxCamSpace3DVertex * cvpNext,
                                    RxScrSpace2DVertex * dvpClip,
                                    RxScrSpace2DVertex * dvpThis,
                                    RxScrSpace2DVertex * dvpNext)
{
    /* Calculate clipper value (t along edge) */
    RwReal              nRxInterp = RWCALCINTERP(_rwClipInfoGlobal.nearClip,
                                                 cvpThis->cameraVertex.z,
                                                 cvpNext->cameraVertex.z);

    RWFUNCTION(RWSTRING("_rwGenerateParallelZLOClippedVertex"));

    /* Stick in the clipped info into free */
    cvpClip->cameraVertex.x =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.x, cvpNext->cameraVertex.x);
    cvpClip->cameraVertex.y =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.y, cvpNext->cameraVertex.y);
    cvpClip->cameraVertex.z = _rwClipInfoGlobal.nearClip;

    /* Clip additional fields */
    RwCameraVertexClipTex(cvpClip, nRxInterp, cvpThis, cvpNext);
    RwIm2DVertexClipRGBA(dvpClip, nRxInterp, dvpThis, dvpNext);

    /* Work out clip codes */
    cvpClip->clipFlags = (RwUInt8) rwZLOCLIP;
    cvpClip->clipFlags |= RWCLIPCODEPARY(cvpClip->cameraVertex);
    cvpClip->clipFlags |= RWCLIPCODEPARX(cvpClip->cameraVertex);

    RWRETURN(nRxInterp);
}

/****************************************************************************
 _rwGenerateParallelZHIClippedVertex

 on entry: vertices for clipped, near and far, both device and camera vertices
 on exit : none
 */

RwReal
_rwGenerateParallelZHIClippedVertex(RxCamSpace3DVertex * cvpClip,
                                    RxCamSpace3DVertex * cvpThis,
                                    RxCamSpace3DVertex * cvpNext,
                                    RxScrSpace2DVertex * dvpClip,
                                    RxScrSpace2DVertex * dvpThis,
                                    RxScrSpace2DVertex * dvpNext)
{
    /* Calculate clipper value (t along edge) */
    RwReal              nRxInterp = RWCALCINTERP(_rwClipInfoGlobal.farClip,
                                                 cvpThis->cameraVertex.z,
                                                 cvpNext->cameraVertex.z);

    RWFUNCTION(RWSTRING("_rwGenerateParallelZHIClippedVertex"));

    /* Stick in the clipped info into free */
    cvpClip->cameraVertex.x =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.x, cvpNext->cameraVertex.x);
    cvpClip->cameraVertex.y =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.y, cvpNext->cameraVertex.y);
    cvpClip->cameraVertex.z = _rwClipInfoGlobal.farClip;

    /* Clip additional fields */
    RwCameraVertexClipTex(cvpClip, nRxInterp, cvpThis, cvpNext);
    RwIm2DVertexClipRGBA(dvpClip, nRxInterp, dvpThis, dvpNext);

    /* Work out clip codes */
    cvpClip->clipFlags = (RwUInt8) rwZHICLIP;
    cvpClip->clipFlags |= RWCLIPCODEPARY(cvpClip->cameraVertex);
    cvpClip->clipFlags |= RWCLIPCODEPARX(cvpClip->cameraVertex);

    RWRETURN(nRxInterp);
}

/****************************************************************************
 _rwGenerateParallelYLOClippedVertex

 on entry: vertices for clipped, near and far, both device and camera vertices
 on exit : none
 */

RwReal
_rwGenerateParallelYLOClippedVertex(RxCamSpace3DVertex * cvpClip,
                                    RxCamSpace3DVertex * cvpThis,
                                    RxCamSpace3DVertex * cvpNext,
                                    RxScrSpace2DVertex * dvpClip,
                                    RxScrSpace2DVertex * dvpThis,
                                    RxScrSpace2DVertex * dvpNext)
{
    /* Calculate clipper value (t along edge) */
    RwReal              nRxInterp = RWCALCINTERPZERO(cvpThis->cameraVertex.y,
                                                     cvpNext->cameraVertex.y);

    RWFUNCTION(RWSTRING("_rwGenerateParallelYLOClippedVertex"));

    /* Stick in the clipped info into free */
    cvpClip->cameraVertex.x =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.x, cvpNext->cameraVertex.x);
    cvpClip->cameraVertex.z =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.z, cvpNext->cameraVertex.z);
    cvpClip->cameraVertex.y = (RwReal) 0.0;

    /* Clip additional fields */
    RwCameraVertexClipTex(cvpClip, nRxInterp, cvpThis, cvpNext);
    RwIm2DVertexClipRGBA(dvpClip, nRxInterp, dvpThis, dvpNext);

    /* Work out clip codes */
    cvpClip->clipFlags =
        (cvpThis->clipFlags & ~(rwYCLIP | rwXCLIP)) | (RwUInt8) rwYLOCLIP;
    cvpClip->clipFlags |= RWCLIPCODEPARX(cvpClip->cameraVertex);

    RWRETURN(nRxInterp);
}

/****************************************************************************
 _rwGenerateParallelYHIClippedVertex

 on entry: vertices for clipped, near and far, both device and camera vertices
 on exit : none
 */

RwReal
_rwGenerateParallelYHIClippedVertex(RxCamSpace3DVertex * cvpClip,
                                    RxCamSpace3DVertex * cvpThis,
                                    RxCamSpace3DVertex * cvpNext,
                                    RxScrSpace2DVertex * dvpClip,
                                    RxScrSpace2DVertex * dvpThis,
                                    RxScrSpace2DVertex * dvpNext)
{
    /* Calculate clipper value (t along edge) */
    RwReal              nRxInterp =
        RWCALCINTERP((RwReal) ((1.0)), cvpThis->cameraVertex.y,
                     cvpNext->cameraVertex.y);

    RWFUNCTION(RWSTRING("_rwGenerateParallelYHIClippedVertex"));

    /* Stick in the clipped info into free */
    cvpClip->cameraVertex.x =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.x, cvpNext->cameraVertex.x);
    cvpClip->cameraVertex.z =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.z, cvpNext->cameraVertex.z);
    cvpClip->cameraVertex.y = (RwReal) 1.0;

    /* Clip additional fields */
    RwCameraVertexClipTex(cvpClip, nRxInterp, cvpThis, cvpNext);
    RwIm2DVertexClipRGBA(dvpClip, nRxInterp, dvpThis, dvpNext);

    /* Work out clip codes */
    cvpClip->clipFlags =
        (cvpThis->clipFlags & ~(rwYCLIP | rwXCLIP)) | (RwUInt8) rwYHICLIP;
    cvpClip->clipFlags |= RWCLIPCODEPARX(cvpClip->cameraVertex);

    RWRETURN(nRxInterp);
}

/****************************************************************************
 _rwGenerateParallelXLOClippedVertex

 on entry: vertices for clipped, near and far, both device and camera vertices
 on exit : none
 */

RwReal
_rwGenerateParallelXLOClippedVertex(RxCamSpace3DVertex * cvpClip,
                                    RxCamSpace3DVertex * cvpThis,
                                    RxCamSpace3DVertex * cvpNext,
                                    RxScrSpace2DVertex * dvpClip,
                                    RxScrSpace2DVertex * dvpThis,
                                    RxScrSpace2DVertex * dvpNext)
{
    /* Calculate clipper value (t along edge) */
    RwReal              nRxInterp = RWCALCINTERPZERO(cvpThis->cameraVertex.x,
                                                     cvpNext->cameraVertex.x);

    RWFUNCTION(RWSTRING("_rwGenerateParallelXLOClippedVertex"));

    /* Stick in the clipped info into free */
    cvpClip->cameraVertex.y =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.y, cvpNext->cameraVertex.y);
    cvpClip->cameraVertex.z =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.z, cvpNext->cameraVertex.z);
    cvpClip->cameraVertex.x = (RwReal) 0.0;

    /* Clip additional fields */
    RwCameraVertexClipTex(cvpClip, nRxInterp, cvpThis, cvpNext);
    RwIm2DVertexClipRGBA(dvpClip, nRxInterp, dvpThis, dvpNext);

    /* Work out clip codes */
    cvpClip->clipFlags =
        (cvpThis->clipFlags & ~(rwXCLIP)) | (RwUInt8) rwXLOCLIP;

    RWRETURN(nRxInterp);
}

/****************************************************************************
 _rwGenerateParallelXHIClippedVertex

 on entry: vertices for clipped, near and far, both device and camera vertices
 on exit : none
 */

RwReal
_rwGenerateParallelXHIClippedVertex(RxCamSpace3DVertex * cvpClip,
                                    RxCamSpace3DVertex * cvpThis,
                                    RxCamSpace3DVertex * cvpNext,
                                    RxScrSpace2DVertex * dvpClip,
                                    RxScrSpace2DVertex * dvpThis,
                                    RxScrSpace2DVertex * dvpNext)
{
    /* Calculate clipper value (t along edge) */
    RwReal              nRxInterp =
        RWCALCINTERP((RwReal) ((1.0)), cvpThis->cameraVertex.x,
                     cvpNext->cameraVertex.x);

    RWFUNCTION(RWSTRING("_rwGenerateParallelXHIClippedVertex"));

    /* Stick in the clipped info into free */
    cvpClip->cameraVertex.y =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.y, cvpNext->cameraVertex.y);
    cvpClip->cameraVertex.z =
        RWGEOMCLIP(nRxInterp,
                   cvpThis->cameraVertex.z, cvpNext->cameraVertex.z);
    cvpClip->cameraVertex.x = (RwReal) 1.0;

    /* Clip additional fields */
    RwCameraVertexClipTex(cvpClip, nRxInterp, cvpThis, cvpNext);
    RwIm2DVertexClipRGBA(dvpClip, nRxInterp, dvpThis, dvpNext);

    /* Work out clip codes */
    cvpClip->clipFlags =
        (cvpThis->clipFlags & ~(rwXCLIP)) | (RwUInt8) rwXHICLIP;

    RWRETURN(nRxInterp);
}
