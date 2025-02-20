#ifndef _SKIND3D9INSTANCE_H
#define _SKIND3D9INSTANCE_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpskin.h"

/*===========================================================================*
 *--- Private Functions -----------------------------------------------------*
 *===========================================================================*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern void __ClearAddress();
extern void __SetAddress(void* address, int len, int when);
extern void __SetBits(unsigned long *dw, int lowBit, int bits, int newValue);


extern RwResEntry *
_rxD3D9SkinInstance(const RpAtomic *atomic,
                    void *owner,
                    RwResEntry **resEntryPointer,
                    RpMeshHeader *meshHeader);



extern void
_rpD3D9SkinGeometryReinstance(const RpAtomic *atomic,
                              RxD3D9ResEntryHeader *resEntryHeader,
                              const RpHAnimHierarchy *hierarchy,
                              RwUInt32 lockedSinceLastInst);

extern RwBool
_rpD3D9SkinAtomicCreateVertexBuffer(const RpAtomic *atomic,
                                    RxD3D9ResEntryHeader *resEntryHeader,
                                    RwBool hasHierarchy);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SKIND3D9INSTANCE_H */
