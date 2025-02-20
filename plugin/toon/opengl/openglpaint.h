/***********************************************************************
 *
 * Module:  openglpaint.h
 *
 * Purpose: OpenGL Toon Implementation
 *
 ***********************************************************************/

#if !defined( OPENGLPAINT_H )
#define OPENGLPAINT_H

/* =====================================================================
 *  Includes
 * ===================================================================== */


/* =====================================================================
 *  Defines
 * ===================================================================== */


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */


/* =====================================================================
 *  Extern variables
 * ===================================================================== */
extern RwOpenGLExtensions   _rwOpenGLExt;


/* =====================================================================
 *  Extern function prototypes
 * ===================================================================== */

#if defined( __cplusplus )
extern "C"
{
#endif /* defined( __cplusplus ) */

extern void
_rpToonOpenGLAllInOneRenderCB( RwResEntry *repEntry,
                               void *object,
                               const RwUInt8 type,
                               const RwUInt32 flags );

extern RwBool
_rwOpenGLSetRenderState( RwRenderState state, void *param );

extern RwBool
_rwOpenGLGetRenderState( RwRenderState state, void *param );

#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

#endif /* !defined( OPENGLPAINT_H ) */
