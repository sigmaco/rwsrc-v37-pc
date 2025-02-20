#ifndef _SKIND3D8PLAIN_H
#define _SKIND3D8PLAIN_H

/*===========================================================================*
 *--- Global Functions -----------------------------------------------------*
 *===========================================================================*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern RwBool
_rwD3D8SkinNeedsAManagedVertexBuffer(const RpAtomic *atomic,
                                     const RxD3D8ResEntryHeader *resEntryHeader);

extern RxPipeline *
_rpSkinD3D8CreatePlainPipe();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SKIND3D8PLAIN_H */
