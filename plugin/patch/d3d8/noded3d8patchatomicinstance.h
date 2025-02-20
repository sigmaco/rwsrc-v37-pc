#ifndef _NODED3D8PATCHATOMICINSTANCE_H
#define _NODED3D8PATCHATOMICINSTANCE_H
/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/

#include <d3d8.h>

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
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Functions -----------------------------------------------------*
 *===========================================================================*/

extern RwBool
_rwD3D8IndexBuffer32bitsCreate(RwUInt32 numIndices, void **indexBuffer);

extern RwUInt32
_rwD3D8PatchGetNumTotalVertices(const RwResEntry *repEntry);

extern void
_rwD3D8PatchDestroyVertexBuffer(RwResEntry *repEntry);

extern RwBool
_rpD3D8PatchInstanceAtomic( RpPatchInstanceAtomic *instAtomic,
                            RpGeometry *geom,
                            RwInt32 res );

extern RwBool
_rwD3D8PatchAtomicAllInOneNodeNode( RxPipelineNodeInstance *self,
                                    const RxPipelineNodeParam *params );

/* external world functions */
extern void 
_rwD3D8AtomicDefaultLightingCallback(void *object);

extern RwBool
_rxD3D8VertexBufferManagerCreate(RwUInt32 fvf,
                               RwUInt32 size,
                               void **vertexBuffer,
                               RwUInt32 *baseIndex);

extern void
_rxD3D8VertexBufferManagerDestroy(RwUInt32 fvf,
                                  RwUInt32 size,
                                  void *vertexBuffer,
                                  RwUInt32 baseIndex);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _NODED3D8PATCHATOMICINSTANCE_H */
