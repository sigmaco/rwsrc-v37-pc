#ifndef _NODED3D9PATCHSKINATOMICINSTANCE_H
#define _NODED3D9PATCHSKINATOMICINSTANCE_H
/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/

#include <d3d9.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rppatch.h"

#include "patchexpander.h"

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

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

extern RwBool
_rwD3D9PatchSkinAtomicAllInOneNodeNode( RxPipelineNodeInstance *self,
                                    const RxPipelineNodeParam *params );

/* external skin plugin functions */
extern RwMatrix *
_rwD3D9SkinPrepareMatrix(RpAtomic *atomic, RpSkin *skin, RpHAnimHierarchy *hierarchy);

#if defined( NOASM )
extern void _rpSkinGenericMatrixBlend(RwInt32 numVertices,
                                      const RwMatrixWeights *matrixWeightsMap,
                                      const RwUInt32 *matrixIndexMap,
                                      const RwMatrix *matrixArray,
                                      RwUInt8 *vertices,
                                      const RwV3d *originalVertices,
                                      RwUInt8 *normals,
                                      const RwV3d *originalNormals,
                                      RwUInt32 stride);
#else
extern void _rpSkinIntelx86MatrixBlend( RwInt32 numVertices,
                                        const RwMatrixWeights *matrixWeightsMap,
                                        const RwUInt32 *matrixIndexMap,
                                        const RwMatrix *matrixArray,
                                        RwV3d *vertices,
                                        const RwV3d *originalVertices,
                                        RwV3d *normals,
                                        const RwV3d *originalNormals,
                                        RwUInt32 stride);

#if !defined(NOSSEASM)
extern void _rpSkinIntelSSEMatrixBlend( RwInt32 numVertices,
                                        const RwMatrixWeights *matrixWeightsMap,
                                        const RwUInt32 *matrixIndexMap,
                                        const RwMatrix *matrixArray,
                                        RwUInt8 *vertices,
                                        const RwV3d *originalVertices,
                                        RwUInt8 *normals,
                                        const RwV3d *originalNormals,
                                        RwUInt32 stride);
#endif

#endif

/* external world functions */
extern void 
_rwD3D9AtomicDefaultLightingCallback(void *object);

extern RwBool
_rxD3D9VertexBufferManagerCreate(RwUInt32 fvf,
                               RwUInt32 size,
                               void **vertexBuffer,
                               RwUInt32 *baseIndex);

extern void
_rxD3D9VertexBufferManagerDestroy(RwUInt32 fvf,
                                  RwUInt32 size,
                                  void *vertexBuffer,
                                  RwUInt32 baseIndex);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _NODED3D9PATCHSKINATOMICINSTANCE_H */
