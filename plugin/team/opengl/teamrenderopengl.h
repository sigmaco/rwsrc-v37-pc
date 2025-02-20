/***********************************************************************
 *
 * Module:  teamrenderopengl.h
 *
 * Purpose: 
 *
 ***********************************************************************/

#if !defined( TEAMRENDEROPENGL_H )
#define TEAMRENDEROPENGL_H

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


/* =====================================================================
 *  Extern function prototypes
 * ===================================================================== */

#if defined( __cplusplus )
extern "C"
{
#endif /* defined( __cplusplus ) */

extern void
_rwOpenGLLightsEnable( const RwBool enableLighting );

extern RwBool
_rwOpenGLLightsGlobalLightsEnable(const RpLightFlag lightFlags,
                                  const RwBool initDirectionalLights);

extern void
_rwOpenGLLightsAddAmbientLight(const RpLight * const ambLight);

extern void
_rwOpenGLLightsAddDirectionalLight(const RpLight * const dirLight);

extern void
_rwOpenGLTeamLightingCB(void *object);

extern const RpTeam *
_rpTeamRender(const RpTeam *team);

#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

#endif /* !defined( TEAMRENDEROPENGL_H ) */
