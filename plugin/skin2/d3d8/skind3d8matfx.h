#ifndef _SKIND3D8MATFX_H
#define _SKIND3D8MATFX_H

/*===========================================================================*
 *--- Global Functions -----------------------------------------------------*
 *===========================================================================*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern RxPipeline *
_rpSkinD3D8CreateMatFXPipe();

extern RwBool
_rwD3D8SkinMatFXNeedsAManagedVertexBuffer(const RxD3D8ResEntryHeader *resEntryHeader);

extern RwBool
_rwD3D8SkinNeedsAManagedVertexBuffer(const RpAtomic *atomic,
                                     const RxD3D8ResEntryHeader *resEntryHeader);

extern void
_rwD3D8AtomicMatFXRenderCallback(RwResEntry *repEntry,
                              void *object,
                              RwUInt8 type,
                              RwUInt32 flags);

extern RwBool
_rwD3D8MaterialMatFXHasBumpMap(const RpMaterial *material);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SKIND3D8MATFX_H */
