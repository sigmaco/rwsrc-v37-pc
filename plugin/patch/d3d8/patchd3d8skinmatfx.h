#ifndef _PATCHD3D8SKINMATFX_H
#define _PATCHD3D8SKINMATFX_H

/*===========================================================================*
 *--- Global Functions -----------------------------------------------------*
 *===========================================================================*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern void
_rwD3D8AtomicMatFXRenderCallback(RwResEntry *repEntry,
                              void *object,
                              RwUInt8 type,
                              RwUInt32 flags);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PATCHD3D8SKINMATFX_H */
