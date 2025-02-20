/****************************************************************************
 *                                                                          *
 * module : p2clpcom.h                                                      *
 *                                                                          *
 * purpose: see p2clpcom.c (ha!)                                            *
 *                                                                          *
 ****************************************************************************/

#ifndef P2CLPCOM_H
#define P2CLPCOM_H

/****************************************************************************
 includes
 */

#include "rwcore.h"

/****************************************************************************
 global types
 */

typedef RwReal (*ClipEdgeFn)(RxCamSpace3DVertex *cvpClip,
                             RxCamSpace3DVertex *cvpThis,
                             RxCamSpace3DVertex *cvpNext,
                             RxScrSpace2DVertex *dvpClip,
                             RxScrSpace2DVertex *dvpThis,
                             RxScrSpace2DVertex *dvpNext);

/* Info used by clipper functions which it is efficient
 * to set up once per node invocation */
typedef struct RwClipInfo RwClipInfo;
struct RwClipInfo
{
    RwReal zScale,          zShift;
    RwReal nearClip,        farClip;
    RwReal camWidth,        camHeight;
    RwReal camOffsetX,      camOffsetY;
    RwReal zBufferNear,     zBufferFar;
};

typedef struct RxClipFuncs RxClipFuncs;
struct RxClipFuncs
{
    ClipEdgeFn          ZLOFunc;
    ClipEdgeFn          ZHIFunc;
    ClipEdgeFn          YLOFunc;
    ClipEdgeFn          YHIFunc;
    ClipEdgeFn          XLOFunc;
    ClipEdgeFn          XHIFunc;
};



/****************************************************************************
 global defines
 */

/* Common clipper stuff */
#define RWCALCINTERPZERO(n,f)           (((n)) / ((n)-(f)))
#define RWCALCINTERP(v,n,f)             (((v)-(n)) / ((f)-(n)))
#define RWCALCINTERPNOSUB(n,f)          (((n)) / ((f)-(n)))
#define RWGEOMCLIP(i,n,f)               ((((f)-(n)) * ((i)))+(n))
#define RWSHADCLIP(i,n,f)               ((((f)-(n)) * ((i)))+(n))

#define RwCameraVertexClipTex(out, interp, near, far)                   \
    (out)->u = RWSHADCLIP((interp), (near)->u, (far)->u);               \
    (out)->v = RWSHADCLIP((interp), (near)->v, (far)->v)


#define RWCLIPCODEPERSPX(pos)                                           \
  (((pos).x < 0) ?                                                      \
   (RwUInt8)rwXLOCLIP : (((pos).x > (pos).z) ?                          \
                         (RwUInt8)rwXHICLIP : 0))

#define RWCLIPCODEPERSPY(pos)                                           \
      (((pos).y < 0) ?                                                  \
       (RwUInt8)rwYLOCLIP : (((pos).y > (pos).z) ?                      \
                             (RwUInt8)rwYHICLIP : 0))

#define RWCLIPCODEPERSPZ(pos)                                           \
      (((pos).z < _rwClipInfoGlobal.nearClip) ?                         \
       (RwUInt8)rwZLOCLIP : (((pos).z > _rwClipInfoGlobal.farClip) ?    \
                             (RwUInt8)rwZHICLIP : 0))

#define RWCLIPCODEPARX(pos)                                             \
      (((pos).x < 0) ?                                                  \
       (RwUInt8)rwXLOCLIP : (((pos).x > 1) ?                            \
                             (RwUInt8)rwXHICLIP : 0))

#define RWCLIPCODEPARY(pos)                                             \
      (((pos).y < 0) ?                                                  \
       (RwUInt8)rwYLOCLIP : (((pos).y > 1) ?                            \
                             (RwUInt8)rwYHICLIP : 0))

#define RWCLIPCODEPARZ(pos)                                             \
      (((pos).z < _rwClipInfoGlobal.nearClip) ?                         \
       (RwUInt8)rwZLOCLIP : (((pos).z > _rwClipInfoGlobal.farClip) ?    \
                             (RwUInt8)rwZHICLIP : 0))

/****************************************************************************
 globals (across program)
 */

extern RwClipInfo _rwClipInfoGlobal;

/****************************************************************************
 global prototypes
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#if (0)
extern void _rwClipPerspRxLine(   RwInt32 vert1, RwInt32 vert2);
extern void _rwClipParallelRxLine(RwInt32 vert1, RwInt32 vert2);
#endif /* (0) */
extern RwInt32 _rwForAllEdgesWithRxInterpolant(RxCluster *CamVerts,
                                             RxCluster *DevVerts,
                                             RxCluster *RxInterpolants,
                                             RxVertexIndex *npClipBuffer,
                                             RwInt32 *npInIndex,
                                             RwInt32 *npOutIndex,
                                             RwInt32 clipCode,
                                             ClipEdgeFn clipit);

extern RwInt32 _rwForAllEdgesWithoutRxInterpolant(RxCluster *CamVerts,
                                                RxCluster *DevVerts,
                                                RxCluster *RxInterpolants,
                                                RxVertexIndex *npClipBuffer,
                                                RwInt32 *npInIndex,
                                                RwInt32 *npOutIndex,
                                                RwInt32 clipCode,
                                                ClipEdgeFn clipit);
extern RwInt32 _rwForAllEdges(RxCluster *CamVerts,
                              RxCluster *DevVerts,
                              RxCluster *RxInterpolants,
                              RxVertexIndex *npClipBuffer,
                              RwInt32 *npInIndex,
                              RwInt32 *npOutIndex,
                              RwInt32 clipCode,
                              ClipEdgeFn clipit);
extern RwInt32 _rwForOneEdge(RxCluster *CamVerts,
                             RxCluster *DevVerts,
                             RxVertexIndex *npClipBuffer,
                             RwInt32 *npInIndex,
                             RwInt32 *npOutIndex,
                             RwInt32 clipCode,
                             ClipEdgeFn clipit);
extern RwReal
_rwGeneratePerspClippedVertexZLO(RxCamSpace3DVertex *cvpClip,
                                 RxCamSpace3DVertex *cvpThis,
                                 RxCamSpace3DVertex *cvpNext,
                                 RxScrSpace2DVertex *dvpClip,
                                 RxScrSpace2DVertex *dvpThis,
                                 RxScrSpace2DVertex *dvpNext);
extern RwReal
_rwGeneratePerspClippedVertexZHI(RxCamSpace3DVertex *cvpClip,
                                 RxCamSpace3DVertex *cvpThis,
                                 RxCamSpace3DVertex *cvpNext,
                                 RxScrSpace2DVertex *dvpClip,
                                 RxScrSpace2DVertex *dvpThis,
                                 RxScrSpace2DVertex *dvpNext);
extern RwReal
_rwGeneratePerspClippedVertexYLO(RxCamSpace3DVertex *cvpClip,
                                 RxCamSpace3DVertex *cvpThis,
                                 RxCamSpace3DVertex *cvpNext,
                                 RxScrSpace2DVertex *dvpClip,
                                 RxScrSpace2DVertex *dvpThis,
                                 RxScrSpace2DVertex *dvpNext);
extern RwReal
_rwGeneratePerspClippedVertexYHI(RxCamSpace3DVertex *cvpClip,
                                 RxCamSpace3DVertex *cvpThis,
                                 RxCamSpace3DVertex *cvpNext,
                                 RxScrSpace2DVertex *dvpClip,
                                 RxScrSpace2DVertex *dvpThis,
                                 RxScrSpace2DVertex *dvpNext);
extern RwReal
_rwGeneratePerspClippedVertexXLO(RxCamSpace3DVertex *cvpClip,
                                 RxCamSpace3DVertex *cvpThis,
                                 RxCamSpace3DVertex *cvpNext,
                                 RxScrSpace2DVertex *dvpClip,
                                 RxScrSpace2DVertex *dvpThis,
                                 RxScrSpace2DVertex *dvpNext);
extern RwReal
_rwGeneratePerspClippedVertexXHI(RxCamSpace3DVertex *cvpClip,
                                 RxCamSpace3DVertex *cvpThis,
                                 RxCamSpace3DVertex *cvpNext,
                                 RxScrSpace2DVertex *dvpClip,
                                 RxScrSpace2DVertex *dvpThis,
                                 RxScrSpace2DVertex *dvpNext);
extern RwReal
_rwGenerateParallelZLOClippedVertex(RxCamSpace3DVertex *cvpClip,
                                    RxCamSpace3DVertex *cvpThis,
                                    RxCamSpace3DVertex *cvpNext,
                                    RxScrSpace2DVertex *dvpClip,
                                    RxScrSpace2DVertex *dvpThis,
                                    RxScrSpace2DVertex *dvpNext);
extern RwReal
_rwGenerateParallelZHIClippedVertex(RxCamSpace3DVertex *cvpClip,
                                    RxCamSpace3DVertex *cvpThis,
                                    RxCamSpace3DVertex *cvpNext,
                                    RxScrSpace2DVertex *dvpClip,
                                    RxScrSpace2DVertex *dvpThis,
                                    RxScrSpace2DVertex *dvpNext);
extern RwReal
_rwGenerateParallelYLOClippedVertex(RxCamSpace3DVertex *cvpClip,
                                    RxCamSpace3DVertex *cvpThis,
                                    RxCamSpace3DVertex *cvpNext,
                                    RxScrSpace2DVertex *dvpClip,
                                    RxScrSpace2DVertex *dvpThis,
                                    RxScrSpace2DVertex *dvpNext);
extern RwReal
_rwGenerateParallelYHIClippedVertex(RxCamSpace3DVertex *cvpClip,
                                    RxCamSpace3DVertex *cvpThis,
                                    RxCamSpace3DVertex *cvpNext,
                                    RxScrSpace2DVertex *dvpClip,
                                    RxScrSpace2DVertex *dvpThis,
                                    RxScrSpace2DVertex *dvpNext);
extern RwReal
_rwGenerateParallelXLOClippedVertex(RxCamSpace3DVertex *cvpClip,
                                    RxCamSpace3DVertex *cvpThis,
                                    RxCamSpace3DVertex *cvpNext,
                                    RxScrSpace2DVertex *dvpClip,
                                    RxScrSpace2DVertex *dvpThis,
                                    RxScrSpace2DVertex *dvpNext);
extern RwReal
_rwGenerateParallelXHIClippedVertex(RxCamSpace3DVertex *cvpClip,
                                    RxCamSpace3DVertex *cvpThis,
                                    RxCamSpace3DVertex *cvpNext,
                                    RxScrSpace2DVertex *dvpClip,
                                    RxScrSpace2DVertex *dvpThis,
                                    RxScrSpace2DVertex *dvpNext);

#ifdef    __cplusplus
}
#endif /* __cplusplus */

#endif /* P2CLPCOM_H */
