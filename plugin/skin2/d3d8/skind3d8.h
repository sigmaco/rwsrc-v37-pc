#ifndef _SKIND3D8_H
#define _SKIND3D8_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpskin.h"

/*===========================================================================*
 *--- Global Types ----------------------------------------------------------*
 *===========================================================================*/
struct SkinPlatformData
{
    RwUInt32 reserved;
};

struct SkinGlobalPlatform
{
    RxPipeline *pipelines[rpSKIND3D8PIPELINEMAX-1];
    const void  *lastHierarchyUsed;
    RwUInt32    lastRenderFrame;
    const void  *lastFrame;
    RwBool      hardwareTL;
    RwBool      hardwareVS;
};

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/
#define rpSKINMAXNUMBEROFMATRICES 256

#define _rpSkinPipeline(pipeline)                                       \
    (_rpSkinGlobals.platform.pipelines[pipeline - 1])

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
typedef struct __rxD3D8SkinInstanceNodeData _rxD3D8SkinInstanceNodeData;
struct __rxD3D8SkinInstanceNodeData
{
    RxD3D8AllInOneRenderCallBack    renderCallback;     /* Render callback   */
    RxD3D8AllInOneLightingCallBack  lightingCallback;   /* Lighting callback */
};

typedef struct _rwD3D8MatrixTransposed _rwD3D8MatrixTransposed;
struct _rwD3D8MatrixTransposed
{
    RwReal right_x, up_x, at_x, pos_x;
    RwReal right_y, up_y, at_y, pos_y;
    RwReal right_z, up_z, at_z, pos_z;
    RwUInt32 pad1, pad2, pad3, pad4;
};

/*===========================================================================*
 *--- Private Functions -----------------------------------------------------*
 *===========================================================================*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern RwBool
_rwSkinD3D8AtomicAllInOneNode(RxPipelineNodeInstance *self,
                        const RxPipelineNodeParam *params);

extern RwMatrix *
_rwD3D8SkinPrepareMatrix(RpAtomic *atomic, RpSkin *skin, RpHAnimHierarchy *hierarchy);

extern RwBool
_rwD3D8SkinNeedsAManagedVertexBuffer(const RpAtomic *atomic, const RxD3D8ResEntryHeader *resEntryHeader);

/* Transposed functions */

extern void
_rwD3D8MatrixMultiplyTranspose(_rwD3D8MatrixTransposed *dstMat,
               const RwMatrix *matA, const RwMatrix *matB);

extern void
_rwD3D8MatrixCopyTranspose(_rwD3D8MatrixTransposed *dstMat,
               const RwMatrix *mat);

extern _rwD3D8MatrixTransposed *
_rwD3D8SkinPrepareMatrixTransposed(RpAtomic *atomic, RpSkin *skin, RpHAnimHierarchy *hierarchy);

/* external world plugin functions */

extern void
_rwD3D8MeshGetNumVerticesMinIndex(RxVertexIndex *indices,
                        RwUInt32 numIndices,
                        RwInt32 *numVertices,
                        RwUInt32 *min);

extern void _rwD3D8ResourceEntryInstanceDataDestroy(RwResEntry *repEntry);

extern void _rwD3D8AtomicDefaultLightingCallback(void *object);

extern void
_rxD3D8DefaultRenderCallback(RwResEntry *repEntry,
                             void *object,
                             RwUInt8 type,
                             RwUInt32 flags);

extern RwBool _rxD3D8VertexBufferManagerCreate(RwUInt32 fvf,
                                               RwUInt32 size,
                                               void **vertexBuffer,
                                               RwUInt32 *baseIndex);
extern void _rxD3D8VertexBufferManagerDestroy(RwUInt32 fvf,
                                              RwUInt32 size,
                                              void *vertexBuffer,
                                              RwUInt32 baseIndex);

extern RwBool _rxD3D8VertexBufferManagerCreateNoFVF(RwUInt32 stride,
                                               RwUInt32 size,
                                               void **vertexBuffer,
                                               RwUInt32 *baseIndex);

extern void _rxD3D8VertexBufferManagerDestroyNoFVF(RwUInt32 stride,
                                              RwUInt32 size,
                                              void *vertexBuffer,
                                              RwUInt32 baseIndex);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SKIND3D8_H */
