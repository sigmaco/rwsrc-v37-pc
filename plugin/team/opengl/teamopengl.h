/***********************************************************************
 *
 * Module:  teamopengl.h
 *
 * Purpose:
 *
 ***********************************************************************/

#if !defined( TEAMOPENGL_H )
#define TEAMOPENGL_H

/* =====================================================================
 *  Includes
 * ===================================================================== */
#include "teamskinopengl.h"


/* =====================================================================
 *  Defines
 * ===================================================================== */
#define TEAMSHADOWALLINONE       /* Render shadows as all in one. */
#define TEAMSHADOWSPRIVATELOOPx  /* Render shadows all together.  */


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */
typedef enum _rpTeamOpenGLPipeId
{
    TEAMOPENGLPIPEID_NAPIPE = 0,
    TEAMOPENGLPIPEID_STATIC,
    TEAMOPENGLPIPEID_SKINNED,
    TEAMOPENGLPIPEID_STATIC_MATFX,
    TEAMOPENGLPIPEID_SKINNED_MATFX,
    TEAMOPENGLPIPEID_STATIC_SHADOW,
    TEAMOPENGLPIPEID_SKINNED_SHADOW,

    TEAMOPENGLPIPEID_MAX
} _rpTeamOpenGLPipeId;


/* =====================================================================
 *  Extern variables
 * ===================================================================== */


/* =====================================================================
 *  Extern function prototypes
 * ===================================================================== */

#if defined( __cplusplus )
extern "C"
{
#endif /* defined( __cplusplus ) */

extern void
_rpTeamOpenGLSetRenderPipeline( RxPipeline *pipe,
                                _rpTeamOpenGLPipeId pipeId );

#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

#endif /* !defined( TEAMOPENGL_H ) */
