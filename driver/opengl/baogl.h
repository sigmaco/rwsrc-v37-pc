/***************************************************************************
 *                                                                         *
 * Module  : baogl.h                                                      *
 *                                                                         *
 * Purpose : OpenGL device header                                          *
 *                                                                         *
 **************************************************************************/

#if !defined(RWOGL_H)
#define RWOGL_H

/****************************************************************************
 Includes
 */

/****************************************************************************
 Defines
 */

/****************************************************************************
 Global Types
 */

/****************************************************************************
 Function prototypes
 */

#if defined(__cplusplus)
extern "C"
{
#endif /* defined(__cplusplus) */

extern RwOpenGLExtensions _rwOpenGLExt;

extern RwBool
_rwOpenGLSelectStencilBufferDepth( RwUInt8 depth );

extern RwBool
_rwOpenGLGetStencilBufferDepth( RwUInt8 *depth );

extern RwInt32
_rwOpenGLGetEngineWindowHeight( void );

extern RwInt32
_rwOpenGLGetEngineColorDepth( void );

extern RwInt32
_rwOpenGLGetEngineZBufferDepth( void );

extern RwBool
_rwOpenGLMakeCurrent( void );

extern RwBool
_rwOpenGLSystem( RwInt32 request,
                 void *out,
                 void *inOut,
                 RwInt32 in);

#if defined(RWDEBUG)

#define GL_ERROR_CHECK()    \
    _rwOpenGLErrorCheck(__FILE__,__LINE__)

extern void
_rwOpenGLErrorCheck( const RwChar filename[],
                     const RwUInt32 lineNumber );

#else /* defined(RWDEBUG) */

#define GL_ERROR_CHECK()  /* No op */

#endif /* defined(RWDEBUG) */

#if defined(__cplusplus)
}
#endif /* defined(__cplusplus) */

#endif /* !defined(RWOGL_H) */
