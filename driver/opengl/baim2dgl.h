/***************************************************************************
 *                                                                         *
 * Module  : baim2dgl.h                                                    *
 *                                                                         *
 * Purpose : 2D immediate mode functionality                               *
 *                                                                         *
 **************************************************************************/

#ifndef BAIM2DGL_H
#define BAIM2DGL_H

/****************************************************************************
 Includes
 */

#include "batypes.h"

/****************************************************************************
 Function prototypes
 */

#ifdef    __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

extern void
_rwOpenGLIm2DOpen( RwDevice *device );

extern void
_rwOpenGLIm2DClose( void );

extern void
_rwOpenGLIm2DSetSubmitFuncs( RwBool gouraudOn,
                             RwBool textureOn );

extern void
_rwOpenGLIm2DSetVertexFuncs( RwBool zTestOn,
                             RwBool zWriteOn,
                             RwBool alphaVertexOn,
                             RwBool fogOn );

#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#endif /* BAIM2DGL_H */
