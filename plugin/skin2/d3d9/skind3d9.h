#ifndef _SKIND3D9_H
#define _SKIND3D9_H

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
    RwUInt32 maxNumBones;
    RwUInt32 useVertexShader;
};

struct SkinGlobalPlatform
{
    RxPipeline  *pipelines[rpSKIND3D9PIPELINEMAX-1];
    const void  *lastHierarchyUsed;
    RwUInt32    lastRenderFrame;
    const void  *lastFrame;
    RwBool      hardwareTL;
    RwBool      hardwareVS;
    RwUInt32    maxNumBones;
    RwUInt32    maxNumConstants;
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

typedef struct __rxD3D9SkinVertexShaderNodeData _rxD3D9SkinVertexShaderNodeData;
struct __rxD3D9SkinVertexShaderNodeData
{
    _rxD3D9VertexShaderBeginCallBack        beginCallback;
    _rxD3D9VertexShaderLightingCallBack     lightingCallback;
    _rxD3D9VertexShaderGetMaterialShaderCallBack getmaterialshaderCallback;
    _rxD3D9VertexShaderMeshRenderCallBack   meshRenderCallback;
    _rxD3D9VertexShaderEndCallBack          endCallback;
};

typedef struct __rxD3D9SkinInstanceNodeData _rxD3D9SkinInstanceNodeData;
struct __rxD3D9SkinInstanceNodeData
{
    RxD3D9AllInOneRenderCallBack    renderCallback;
    RxD3D9AllInOneLightingCallBack  lightingCallback;

    _rxD3D9SkinVertexShaderNodeData     vertexShaderNode;
};

/*===========================================================================*
 *--- Private Functions -----------------------------------------------------*
 *===========================================================================*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern RwBool
_rwSkinD3D9AtomicAllInOneNode(RxPipelineNodeInstance *self,
                              const RxPipelineNodeParam *params);

extern RwBool
_rwD3D9SkinNeedsAManagedVertexBuffer(const RpAtomic *atomic,
                                     const RxD3D9ResEntryHeader *resEntryHeader);

extern RwBool
_rwD3D9SkinUseVertexShader(const RpAtomic *atomic);

/* external world plugin functions */

extern void
_rwD3D9MeshGetNumVerticesMinIndex(const RxVertexIndex *indices,
                                  RwUInt32 numIndices,
                                  RwUInt32 *numVertices,
                                  RwUInt32 *min);

extern void
_rwD3D9SortTriListIndices(RxVertexIndex *indices, RwUInt32 numIndices);

extern RwUInt32
_rwD3D9ConvertToTriList(RxVertexIndex *indexDst,
                        const RxVertexIndex *indexSrc,
                        RwUInt32 numIndices,
                        RwUInt32 minVert);

extern void _rwD3D9ResourceEntryInstanceDataDestroy(RwResEntry *repEntry);

extern void _rwD3D9AtomicDefaultLightingCallback(void *object);

extern void
_rxD3D9DefaultRenderCallback(RwResEntry *repEntry,
                             void *object,
                             RwUInt8 type,
                             RwUInt32 flags);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SKIND3D9_H */
