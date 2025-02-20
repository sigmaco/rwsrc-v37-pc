#ifndef _PATCHD3D9_H
#define _PATCHD3D9_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rppatch.h"

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/

#if (defined(__VECTORC__) && !(defined(RWDEBUG) || defined(RWSUPPRESSINLINE)))
#define  ATOMICGETDEFAULTINSTANCEPIPELINE()                              \
    (((rwPipeGlobals *)                                                  \
     (((RwUInt8 *)(RwEngineInstance)) +                                  \
      (_rxPipelineGlobalsOffset)))->currentAtomicPipeline)
#endif /* (defined(__VECTORC__) .... */

#if (!defined(ATOMICGETDEFAULTINSTANCEPIPELINE))
#define  ATOMICGETDEFAULTINSTANCEPIPELINE()           \
    (RpAtomicGetDefaultPipeline())
#endif /* (!defined(ATOMICGETDEFAULTINSTANCEPIPELINE)) */

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

typedef struct PatchPlatform PatchPlatform;
struct PatchPlatform
{
    RxPipeline *pipelines[rpPATCHD3D9PIPELINEMAX];
    RwBool      hardwareTL;
    RwBool      use32bitsIndexBuffers;
};

/*
 * The refinement node generates new vertices and so has to interpolate
 * per-vertex attributes from the original set of vertices to generate
 * values for the generated vertices. It can (barycentrically)
 * interpolate any number of additional 'passes' of UVs or RGBAs,
 * provided that:
 * (a) these extra clusters are added by the app at pipeline construction
 *     time through the use of RxPipelineNodeRequestCluster(),
 * (b) the extra UVs/RGBAs are the last clusters added to the node, and
 * (c) all the UVs come before the RGBAs (and there are no other cluster
 *     types sprinkled in amongst the UVs/RGBAs)
 */
struct RpNodePatchData
{
    RwBool      patchOn;                                /* patchOn       */
    RwInt32     numExtraUVs;                            /* numExtraUVs   */
    RwInt32     numExtraRGBAs;                          /* numExtraRGBAs */
    RxD3D9AllInOneRenderCallBack    renderCallback;     /* Render callback */
};
typedef struct RpNodePatchData RpNodePatchData;

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Functions -----------------------------------------------------*
 *===========================================================================*/
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern RxPipeline *
_rwD3D9PatchPipelineCreateGeneric(void);

extern RxPipeline *
_rwD3D9PatchPipelineCreateMatFX(void);

extern RxPipeline *
_rwD3D9PatchPipelineCreateSkin(void);

extern RxPipeline *
_rwD3D9PatchPipelineCreateSkinMatFX(void);


extern void
_rxD3D9DefaultRenderCallback(RwResEntry *repEntry,
                             void *object,
                             RwUInt8 type,
                             RwUInt32 flags);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PATCHD3D9_H */
