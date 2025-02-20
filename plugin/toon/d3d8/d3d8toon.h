#ifndef D3D8TOON_H
#define D3D8TOON_H

extern void
_rpToonD3D8RenderCallback(RwResEntry *repEntry,
                             void *object,
                             RwUInt8 type,
                             RwUInt32 flags);

extern void _rpToonD3D8BackfaceExtrudeInit();

extern void _rpToonD3D8BackfaceExtrudeDestroy();

extern void _rpToonD3D8BackfaceExtrudeRender
    (
    RwResEntry *repEntry,
    void *object,
    RwUInt8 type,
    RwUInt32 flags,
    RpToonGeo *toonGeo
    );

/* Internal driver functions */
extern RwBool _rwD3D8RenderStateVertexAlphaEnable(RwBool enable);
extern RwBool _rwD3D8RenderStateSrcBlend(RwBlendFunction srcBlend);
extern RwBool _rwD3D8RenderStateDestBlend(RwBlendFunction dstBlend);

#endif
