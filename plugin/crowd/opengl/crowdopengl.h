/***********************************************************************
 *
 * Module:  crowdopengl.h
 *
 * Purpose: RpCrowd for OpenGL
 *
 ***********************************************************************/

#if !defined( CROWDOPENGL_H )
#define CROWDOPENGL_H

/* =====================================================================
 *  Includes
 * ===================================================================== */


/* =====================================================================
 *  Defines
 * ===================================================================== */


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */

#if defined( __cplusplus )
extern "C"
{
#endif /* defined( __cplusplus ) */

/* =====================================================================
 *  Extern variables
 * ===================================================================== */
extern RwOpenGLExtensions   _rwOpenGLExt;


/* =====================================================================
 *  Extern function prototypes
 * ===================================================================== */

extern RwBool
_rwOpenGLSetRenderState( RwRenderState state, void *param );

extern RwBool
_rwOpenGLGetRenderState( RwRenderState state, void *param );

#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

#endif /* !defined( CROWDOPENGL_H ) */
