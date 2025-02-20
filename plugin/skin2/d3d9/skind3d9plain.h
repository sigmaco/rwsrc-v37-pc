#ifndef _SKIND3D9PLAIN_H
#define _SKIND3D9PLAIN_H

/*===========================================================================*
 *--- Global Functions -----------------------------------------------------*
 *===========================================================================*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern RwBool
_rwD3D9SkinNeedsAManagedVertexBuffer(const RpAtomic *atomic,
                                     const RxD3D9ResEntryHeader *resEntryHeader);

extern RxPipeline *
_rpSkinD3D9CreatePlainPipe();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SKIND3D9PLAIN_H */
