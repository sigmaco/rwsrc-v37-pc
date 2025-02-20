/***********************************************************************
 *
 * Module:  ptankopengl.h
 *
 * Purpose: RpPTank for OpenGL
 *
 ***********************************************************************/

#if !defined( PTANKOPENGL_H )
#define PTANKOPENGL_H

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
extern RpPTankCallBacks     defaultCB;

extern RwOpenGLExtensions   _rwOpenGLExt;


/* =====================================================================
 *  Extern function prototypes
 * ===================================================================== */

extern RwBool
_rwOpenGLSetRenderState( RwRenderState state, void *param );

extern RwBool
_rwOpenGLGetRenderState( RwRenderState state, void *param );

extern void
_rxOpenGLDefaultAllInOneAtomicLightingCB( void *object );

#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

#endif /* !defined( PTANKOPENGL_H ) */
