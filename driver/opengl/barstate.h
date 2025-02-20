/***********************************************************************
 *
 * Module:  barstate.h
 *
 * Purpose: OpenGL render states
 *
 ***********************************************************************/

#if !defined( BARSTATE_H )
#define BARSTATE_H

/* =====================================================================
 *  Includes
 * ===================================================================== */


/* =====================================================================
 *  Defines
 * ===================================================================== */

/* RWPUBLIC */

#define RwOpenGLEnableMacro(_token)                 \
MACRO_START                                         \
if ( FALSE == RwOpenGLStateCache[_token].enabled )  \
{                                                   \
    glEnable( RwOpenGLStateCache[_token].glToken ); \
    RwOpenGLStateCache[_token].enabled = TRUE;      \
}                                                   \
MACRO_STOP

#define RwOpenGLDisableMacro(_token)                    \
MACRO_START                                             \
if ( FALSE != RwOpenGLStateCache[_token].enabled )      \
{                                                       \
    glDisable( RwOpenGLStateCache[_token].glToken );    \
    RwOpenGLStateCache[_token].enabled = FALSE;         \
}                                                       \
MACRO_STOP

#define RwOpenGLIsEnabledMacro(_token)                  \
    RwOpenGLStateCache[_token].enabled

#if !(defined(RWDEBUG) || defined(RWSUPPRESSINLINE))

#define RwOpenGLEnable      RwOpenGLEnableMacro
#define RwOpenGLDisable     RwOpenGLDisableMacro
#define RwOpenGLIsEnabled   RwOpenGLIsEnabledMacro

#endif /* !(defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/* RWPUBLICEND */


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */

/* RWPUBLIC */

/**
 * \ingroup rwengineopengl
 *  RwOpenGLStateToken 
 *  This type represents the RenderWare Graphics' tokens corresponding to
 *  some of OpenGL's state tokens.
 */
enum RwOpenGLStateToken
{
    rwGL_STATENA = 0,           /**< Internal use only */
                            
    rwGL_ALPHA_TEST,            /**< Equivalent to \c GL_ALPHA_TEST */
    rwGL_BLEND,                 /**< Equivalent to \c GL_BLEND */
    rwGL_COLOR_MATERIAL,        /**< Equivalent to \c GL_COLOR_MATERIAL */
    rwGL_CULL_FACE,             /**< Equivalent to \c GL_CULL_FACE */
    rwGL_DEPTH_TEST,            /**< Equivalent to \c GL_DEPTH_TEST */
    rwGL_FOG,                   /**< Equivalent to \c GL_FOG */
    rwGL_LIGHT0,                /**< Equivalent to \c GL_LIGHT0 */
    rwGL_LIGHT1,                /**< Equivalent to \c GL_LIGHT1 */
    rwGL_LIGHT2,                /**< Equivalent to \c GL_LIGHT2 */
    rwGL_LIGHT3,                /**< Equivalent to \c GL_LIGHT3 */
    rwGL_LIGHT4,                /**< Equivalent to \c GL_LIGHT4 */
    rwGL_LIGHT5,                /**< Equivalent to \c GL_LIGHT5 */
    rwGL_LIGHT6,                /**< Equivalent to \c GL_LIGHT6 */
    rwGL_LIGHT7,                /**< Equivalent to \c GL_LIGHT7 */
    rwGL_LIGHTING,              /**< Equivalent to \c GL_LIGHTING */
    rwGL_NORMALIZE,             /**< Equivalent to \c GL_NORMALIZE */
    rwGL_SCISSOR_TEST,          /**< Equivalent to \c GL_SCISSOR_TEST */
    rwGL_STENCIL_TEST,          /**< Equivalent to \c GL_STENCIL_TEST */
    rwGL_TEXTURE0_2D,           /**< Equivalent to \c GL_TEXTURE_2D for texture unit 0 */
    rwGL_TEXTURE1_2D,           /**< Equivalent to \c GL_TEXTURE_2D for texture unit 1 */
    rwGL_TEXTURE2_2D,           /**< Equivalent to \c GL_TEXTURE_2D for texture unit 2 */
    rwGL_TEXTURE3_2D,           /**< Equivalent to \c GL_TEXTURE_2D for texture unit 3 */
    rwGL_TEXTURE4_2D,           /**< Equivalent to \c GL_TEXTURE_2D for texture unit 4 */
    rwGL_TEXTURE5_2D,           /**< Equivalent to \c GL_TEXTURE_2D for texture unit 5 */
    rwGL_TEXTURE6_2D,           /**< Equivalent to \c GL_TEXTURE_2D for texture unit 6 */
    rwGL_TEXTURE7_2D,           /**< Equivalent to \c GL_TEXTURE_2D for texture unit 7 */
    rwGL_TEXTURE0_GEN_S,        /**< Equivalent to \c GL_TEXTURE_GEN_S for texture unit 0 */
    rwGL_TEXTURE1_GEN_S,        /**< Equivalent to \c GL_TEXTURE_GEN_S for texture unit 1 */
    rwGL_TEXTURE2_GEN_S,        /**< Equivalent to \c GL_TEXTURE_GEN_S for texture unit 2 */
    rwGL_TEXTURE3_GEN_S,        /**< Equivalent to \c GL_TEXTURE_GEN_S for texture unit 3 */
    rwGL_TEXTURE4_GEN_S,        /**< Equivalent to \c GL_TEXTURE_GEN_S for texture unit 4 */
    rwGL_TEXTURE5_GEN_S,        /**< Equivalent to \c GL_TEXTURE_GEN_S for texture unit 5 */
    rwGL_TEXTURE6_GEN_S,        /**< Equivalent to \c GL_TEXTURE_GEN_S for texture unit 6 */
    rwGL_TEXTURE7_GEN_S,        /**< Equivalent to \c GL_TEXTURE_GEN_S for texture unit 7 */
    rwGL_TEXTURE0_GEN_T,        /**< Equivalent to \c GL_TEXTURE_GEN_T for texture unit 0 */
    rwGL_TEXTURE1_GEN_T,        /**< Equivalent to \c GL_TEXTURE_GEN_T for texture unit 1 */
    rwGL_TEXTURE2_GEN_T,        /**< Equivalent to \c GL_TEXTURE_GEN_T for texture unit 2 */
    rwGL_TEXTURE3_GEN_T,        /**< Equivalent to \c GL_TEXTURE_GEN_T for texture unit 3 */
    rwGL_TEXTURE4_GEN_T,        /**< Equivalent to \c GL_TEXTURE_GEN_T for texture unit 4 */
    rwGL_TEXTURE5_GEN_T,        /**< Equivalent to \c GL_TEXTURE_GEN_T for texture unit 5 */
    rwGL_TEXTURE6_GEN_T,        /**< Equivalent to \c GL_TEXTURE_GEN_T for texture unit 6 */
    rwGL_TEXTURE7_GEN_T,        /**< Equivalent to \c GL_TEXTURE_GEN_T for texture unit 7 */
    rwGL_TEXTURE0_GEN_R,        /**< Equivalent to \c GL_TEXTURE_GEN_R for texture unit 0 */
    rwGL_TEXTURE1_GEN_R,        /**< Equivalent to \c GL_TEXTURE_GEN_R for texture unit 1 */
    rwGL_TEXTURE2_GEN_R,        /**< Equivalent to \c GL_TEXTURE_GEN_R for texture unit 2 */
    rwGL_TEXTURE3_GEN_R,        /**< Equivalent to \c GL_TEXTURE_GEN_R for texture unit 3 */
    rwGL_TEXTURE4_GEN_R,        /**< Equivalent to \c GL_TEXTURE_GEN_R for texture unit 4 */
    rwGL_TEXTURE5_GEN_R,        /**< Equivalent to \c GL_TEXTURE_GEN_R for texture unit 5 */
    rwGL_TEXTURE6_GEN_R,        /**< Equivalent to \c GL_TEXTURE_GEN_R for texture unit 6 */
    rwGL_TEXTURE7_GEN_R,        /**< Equivalent to \c GL_TEXTURE_GEN_R for texture unit 7 */
    rwGL_TEXTURE0_GEN_Q,        /**< Equivalent to \c GL_TEXTURE_GEN_Q for texture unit 0 */
    rwGL_TEXTURE1_GEN_Q,        /**< Equivalent to \c GL_TEXTURE_GEN_Q for texture unit 1 */
    rwGL_TEXTURE2_GEN_Q,        /**< Equivalent to \c GL_TEXTURE_GEN_Q for texture unit 2 */
    rwGL_TEXTURE3_GEN_Q,        /**< Equivalent to \c GL_TEXTURE_GEN_Q for texture unit 3 */
    rwGL_TEXTURE4_GEN_Q,        /**< Equivalent to \c GL_TEXTURE_GEN_Q for texture unit 4 */
    rwGL_TEXTURE5_GEN_Q,        /**< Equivalent to \c GL_TEXTURE_GEN_Q for texture unit 5 */
    rwGL_TEXTURE6_GEN_Q,        /**< Equivalent to \c GL_TEXTURE_GEN_Q for texture unit 6 */
    rwGL_TEXTURE7_GEN_Q,        /**< Equivalent to \c GL_TEXTURE_GEN_Q for texture unit 7 */
    rwGL_TEXTURE_CUBE_MAP_ARB,  /**< Equivalent to \c GL_TEXTURE_CUBE_MAP_ARB */

    rwGL_NUMSTATES,             /**< Internal use only */

    rwGL_STATEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT /**< Internal use only */
};
typedef enum RwOpenGLStateToken RwOpenGLStateToken;

typedef struct RwOpenGLStateData RwOpenGLStateData;
/** 
 * \ingroup rwengineopengl
 * \struct RwOpenGLStateData
 *
 * This type contains RenderWare Graphics' state cache data.
 *
 */
struct RwOpenGLStateData
{
    RwUInt32    glToken;    /**< Internal use only */

    RwBool      enabled;    /**< Internal use only */
};

/* RWPUBLICEND */

/* =====================================================================
 *  Extern variables
 * ===================================================================== */

/* RWPUBLIC */

#if defined( __cplusplus )
extern "C"
{
#endif /* defined( __cplusplus ) */

extern RwOpenGLStateData   RwOpenGLStateCache[rwGL_NUMSTATES];

/* RWPUBLICEND */

/* =====================================================================
 *  Extern function prototypes
 * ===================================================================== */

extern void
_rwOpenGLRSUnbindRasterBeforeDestroy( RwRaster *raster );

extern void
_rwOpenGLSetTextureAnisotropyOffset( RwInt32 anisotTextureOffset );

extern void
_rwOpenGLRenderStateClose( void );

extern void
_rwOpenGLRenderStateOpen( void );

extern RwBool
_rwOpenGLSetRenderState( RwRenderState state,
                         void *param );

extern RwBool
_rwOpenGLGetRenderState( RwRenderState state,
                         void *param );

extern void
_rwOpenGLSetRaster( RwRaster *raster );

extern RwBool
_rwOpenGLSetRasterForceTextureParams( void *rasterVoid );

/* RWPUBLIC */

extern void
RwOpenGLSetStencilClear( RwUInt32 stencilValue );

extern RwUInt32
RwOpenGLGetStencilClear( void );

extern void
RwOpenGLSetTexture( void *textureVoid );

extern RwUInt32
RwOpenGLSetActiveTextureUnit( const RwUInt32 textureUnit );

extern RwUInt32
RwOpenGLGetActiveTextureUnit( void );

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
extern void
RwOpenGLEnable( const RwOpenGLStateToken token );

extern void
RwOpenGLDisable( const RwOpenGLStateToken token );

extern RwBool
RwOpenGLIsEnabled( const RwOpenGLStateToken token );
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

/* RWPUBLICEND */

#endif /* !defined( BARSTATE_H ) */
