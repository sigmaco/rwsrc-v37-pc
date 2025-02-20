/***********************************************************************
 *
 * Module:  opengllights.h
 *
 * Purpose: 
 *
 ***********************************************************************/

#if !defined( OPENGLLIGHTS_H )
#define OPENGLLIGHTS_H

/* =====================================================================
 *  Includes
 * ===================================================================== */


/* =====================================================================
 *  Defines
 * ===================================================================== */

/* RWPUBLIC */

/** 
 * \ingroup rplightopengl
 * \page rpopengllightoverview OpenGL Lighting Overview
 * 
 * RenderWare Graphics for OpenGL supports as many lights as the host video
 * hardware allows. This is usually a maximum of eight with current hardware.
 * Any additional lights activated beyond this limit are ignored.
 *
 * RenderWare Graphics for OpenGL sums all ambient lights and enables the
 * summed color as the constant global ambient.
 *
 * Lights of type \ref rpLIGHTPOINT, \ref rpLIGHTSPOT, and \ref rpLIGHTSPOTSOFT
 * under OpenGL lighting do not exactly match generic RenderWare Graphics
 * lighting equations, but are close approximations.
 *
 * The OpenGL lighting attenuation model is different to that used in RenderWare
 * Graphics. The function used in OpenGL is
 * \verbatim
  
                                      1
   attenuation factor = ----------------------------
                        kC + (kL * d) + (kQ * d * d)
  
   where kC is a constant coefficient  (GL_CONSTANT_ATTENUATION)
         kL is a linear coefficient    (GL_LINEAR_ATTENUATION)
         kQ is a quadratic coefficient (GL_QUADRATIC_ATTENUATION)
         d  is the distance from the light to the vertex
   \endverbatim
 * The coefficients default to \c kC = 1, \c kL = 0, \c kQ = 5.
 * However, a RenderWare Graphics application can set and query 
 * these coefficients for an OpenGL specific light type via the functions
 * \ref RpOpenGLLightSetAttenuationParams and \ref RpOpenGLLightGetAttenuationParams.
 *
 * Additionally to OpenGL, two functions are available to set and query the
 * exponent for lights of type \ref rpLIGHTSPOTSOFT.
 *
 * \li \ref RpOpenGLLightSetSoftSpotExponent
 * \li \ref RpOpenGLLightGetSoftSpotExponent
 *
 * OpenGL uses the following to calculate the falloff factor:
 * \verbatim
                                       exponent
   falloff factor = (MAX { v . d , 0 }) 
  
   where v is the unit vector from the light source to the vertex
         d is the direction of the spotlight
         exponent is the spotlight exponent (GL_SPOT_EXPONENT)
   \endverbatim
 * 
 * \note Although this functionality is obtained via the plugin
 * mechanism of RenderWare Graphics, the plugin is attached automatically when
 * the world module plugin is attached as a result of \ref RpWorldPluginAttach.
 *
 * \see \ref RpLight
 * \see \ref RpOpenGLLightAttentuation
 * \see RpOpenGLLightSetAttenuationParams
 * \see RpOpenGLLightGetAttenuationParams
 * \see RpOpenGLLightSetSoftSpotExponent
 * \see RpOpenGLLightGetSoftSpotExponent
 */

/* Support macros for RenderWare Graphics OpenGL Lights */
#define rpOPENGLLIGHTPOINT      rpLIGHTPOINT
#define rpOPENGLLIGHTSPOT       rpLIGHTSPOT
#define rpOPENGLLIGHTSPOTSOFT   rpLIGHTSPOTSOFT


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */

typedef struct RpOpenGLLightAttentuation RpOpenGLLightAttentuation;
/**
 * \ingroup rplightopengl
 * \struct RpOpenGLLightAttentuation
 * This structure contains the attenuation parameters used by OpenGL hardware lights.
 */
struct RpOpenGLLightAttentuation
{
    RwReal  constant;   /**< Constant attenuation parameter */

    RwReal  linear;     /**< Linear attenuation parameter */

    RwReal  quadratic;  /**< Quadratic attenuation parameter */
};


#if defined( __cplusplus )
extern "C"
{
#endif /* defined( __cplusplus ) */

/* =====================================================================
 *  Extern variables
 * ===================================================================== */

/* RWPUBLICEND */

/* =====================================================================
 *  Extern function prototypes
 * ===================================================================== */

extern RwBool
_rpOpenGLLightPluginAttach( void );

extern RwBool
_rwOpenGLLightsOpen( void );

extern RwBool
_rwOpenGLLightsClose( void );

extern void
_rwOpenGLLightsEnable( const RwBool enableLighting );

extern RwBool
_rwOpenGLLightsGlobalLightsEnable(const RpLightFlag lightFlags,
                                  const RwBool initDirectionalLights);

extern RwBool
_rwOpenGLLightsLocalLightEnable( const RpLight * const localLight );

extern void
_rwOpenGLLightsAddAmbientLight(const RpLight * const ambLight);

extern void
_rwOpenGLLightsAddDirectionalLight(const RpLight * const dirLight);

/* RWPUBLIC */

extern void
RpOpenGLWorldSetMaterialProperties( const void *materialVoid,
                                    const RwUInt32 flags );

extern void
RpOpenGLLightSetAttenuationParams( void * const voidLight,
                                   const RpOpenGLLightAttentuation *params );

extern RpOpenGLLightAttentuation
RpOpenGLLightGetAttenuationParams( const void * const voidLight );

extern void
RpOpenGLLightSetSoftSpotExponent( void * const voidLight,
                                  const RwReal exponent );

extern RwReal
RpOpenGLLightGetSoftSpotExponent( const void * const voidLight );

#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

/* RWPUBLICEND */

#endif /* !defined( OPENGLLIGHTS_H ) */
