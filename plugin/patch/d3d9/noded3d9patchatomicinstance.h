#ifndef _NODED3D9PATCHATOMICINSTANCE_H
#define _NODED3D9PATCHATOMICINSTANCE_H
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
_rwD3D9IndexBuffer32bitsCreate(RwUInt32 numIndices, void **indexBuffer);

extern RwUInt32
_rwD3D9PatchGetNumTotalVertices(const RwResEntry *repEntry);

extern void
_rwD3D9PatchDestroyVertexBuffer(RwResEntry *repEntry);

extern RwBool
_rpD3D9PatchInstanceAtomic( RpPatchInstanceAtomic *instAtomic,
                            RpGeometry *geom,
                            RwInt32 res );

extern RwBool
_rwD3D9PatchAtomicAllInOneNodeNode( RxPipelineNodeInstance *self,
                                    const RxPipelineNodeParam *params );

/* external world functions */
extern void 
_rwD3D9AtomicDefaultLightingCallback(void *object);

extern void
_rwD3D9ResourceEntryInstanceDataDestroy(RwResEntry *repEntry);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _NODED3D9PATCHATOMICINSTANCE_H */
