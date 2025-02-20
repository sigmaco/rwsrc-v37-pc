/***********************************************************************
 *
 * Module:  teamshadowopengl.h
 *
 * Purpose: 
 *
 ***********************************************************************/

#if !defined( TEAMSHADOWOPENGL_H )
#define TEAMSHADOWOPENGL_H

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

extern void
_rpTeamOpenGLSkinShadowRenderCB( RwResEntry *repEntry,
                                 void *object,
                                 const RwUInt8 type,
                                 const RwUInt32 flags );

#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

#endif /* !defined( TEAMSHADOWOPENGL_H ) */
