#if (!defined(ANISPRIV_H))
#define ANISPRIV_H

#if defined(XBOX_DRVMODEL_H)

extern void
_rwXboxSetTextureAnisotropyOffset(RwInt32 anisotTextureOffset);

#elif defined(D3D8_DRVMODEL_H)

#include <d3d8.h>

extern void
_rwD3D8SetTextureAnisotropyOffset(RwInt32 anisotTextureOffset);

#elif defined(D3D9_DRVMODEL_H)

#include <d3d9.h>

extern void
_rwD3D9SetTextureAnisotropyOffset(RwInt32 anisotTextureOffset);

#elif defined(OPENGL_DRVMODEL_H)

extern RwOpenGLExtensions _rwOpenGLExt;

extern void
_rwOpenGLSetTextureAnisotropyOffset(RwInt32 anisotTextureOffset);

#endif /* !Xbox && !D3D8 && !D3D9 && !OpenGL */

#endif /* (!defined(ANISPRIV_H)) */
