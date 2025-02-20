#ifndef _SKIND3D9MATFX_H
#define _SKIND3D9MATFX_H

/*===========================================================================*
 *--- Global Functions -----------------------------------------------------*
 *===========================================================================*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern RxPipeline *
_rpSkinD3D9CreateMatFXPipe();

extern RwBool
_rwD3D9SkinMatFXNeedsAManagedVertexBuffer(const RxD3D9ResEntryHeader *resEntryHeader);

extern RwBool
_rwD3D9SkinNeedsAManagedVertexBuffer(const RpAtomic *atomic,
                                     const RxD3D9ResEntryHeader *resEntryHeader);

extern void
_rwD3D9AtomicMatFXRenderCallback(RwResEntry *repEntry,
                              void *object,
                              RwUInt8 type,
                              RwUInt32 flags);

extern RwBool
_rwD3D9MaterialMatFXHasBumpMap(const RpMaterial *material);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SKIND3D9MATFX_H */
