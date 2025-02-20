/***************************************************************************
 *                                                                         *
 * Module  : barstate.c                                                    *
 *                                                                         *
 * Purpose : Handling of render state                                      *
 *                                                                         *
 **************************************************************************/

/****************************************************************************
 Includes
 */

#if defined(WIN32)
#include <windows.h>
#endif /* defined(WIN32) */
#include <GL/gl.h>

/* RW type definitions */
#include "batypes.h"
#include "bamemory.h"
#include "badevice.h"
#include "batextur.h"
#include "baimmedi.h"
#include "bavector.h"

/* OpenGL driver includes */
#include "drvmodel.h"

/* Common driver includes */
#include "barwtyp.h"

/* Header for this file */
#include "drvfns.h"
#include "baogl.h"     /* For GL_ERROR_CHECK() */
#include "baintogl.h"  /* For raster extension stuff */
#include "baim2dgl.h"
#include "barstate.h"
#include "bavagl.h"
#include "barastgl.h"


/**
 * \ingroup rwenginemultitextureopengl
 * \page multitexturingoverview Overview
 *
 * OpenGL RenderWare Graphics supports multitexturing where this is supported
 * by the underlying video card driver, to improve rendering performance.
 *
 * The maximum number of texture units can be queried through the \ref
 * RwOpenGLExtensions object.
 *
 * The currently selected texture unit defaults to unit 0 when \ref
 * RwEngineStart is called. During the lifetime of the RenderWare Graphics
 * application, the current texture unit can be changed using \ref
 * RwOpenGLSetActiveTextureUnit, passing the zero-based index of the required
 * texture unit. The currently selected texture unit can be queried with
 * \ref RwOpenGLGetActiveTextureUnit.
 *
 * Once selected, the current texture unit remains selected until the next
 * successful call to \ref RwOpenGLSetActiveTextureUnit. All texturing
 * operations affect the current texture unit, including those for \ref
 * rwenginevertexarraysopengl.
 *
 * \ref RwOpenGLSetTexture may be used to bind an \ref RwTexture to the
 * currently selected texture unit. Passing a NULL \ref RwTexture pointer to
 * this function unbinds any textures from the current texture unit. Note that
 * OpenGL RenderWare Graphics enables and disable 2D texturing where required.
 *
 * \see \ref RwOpenGLExtensions
 * \see \ref RwOpenGLSetActiveTextureUnit
 * \see \ref RwOpenGLGetActiveTextureUnit
 * \see \ref RwOpenGLSetTexture
 */

/**
 * \ingroup rwrenderstateblendmodesupportopengl
 * \page sourceblendingopengl Source Blending Modes
 *
 * The supported source blending modes in OpenGL RenderWare Graphics are:
 *
 * \li \ref rwBLENDZERO
 *
 * \li \ref rwBLENDONE
 *
 * \li \ref rwBLENDSRCALPHA
 *
 * \li \ref rwBLENDINVSRCALPHA
 *
 * \li \ref rwBLENDDESTALPHA
 *
 * \li \ref rwBLENDINVDESTALPHA
 *
 * \li \ref rwBLENDDESTCOLOR
 *
 * \li \ref rwBLENDINVDESTCOLOR
 *
 * \li \ref rwBLENDSRCALPHASAT
 *
 * If the \e GL_NV_blend_square extension is available, two additional source
 * blend modes are supported:
 *
 * \li \ref rwBLENDSRCCOLOR
 *
 * \li \ref rwBLENDINVSRCCOLOR
 *
 * \see RwBlendFunction
 * \see \ref RwOpenGLExtensions
 */

/**
 * \ingroup rwrenderstateblendmodesupportopengl
 * \page destblendingopengl Destination Blending Modes
 *
 * The supported destination blending modes in OpenGL RenderWare Graphics are:
 *
 * \li \ref rwBLENDZERO
 *
 * \li \ref rwBLENDONE
 *
 * \li \ref rwBLENDSRCCOLOR
 *
 * \li \ref rwBLENDINVSRCCOLOR
 *
 * \li \ref rwBLENDSRCALPHA
 *
 * \li \ref rwBLENDINVSRCALPHA
 *
 * \li \ref rwBLENDDESTALPHA
 *
 * \li \ref rwBLENDINVDESTALPHA
 *
 * If the \e GL_NV_blend_square extension is available, two additional destination
 * blend modes are supported:
 *
 * \li \ref rwBLENDDESTCOLOR
 *
 * \li \ref rwBLENDINVDESTCOLOR
 *
 * \see RwBlendFunction
 * \see \ref RwOpenGLExtensions
 */

/**
 * \ingroup rwrenderstaterestrictionsopengl
 * \page openglrenderstaterestrictions Renderstate Restrictions
 *
 * The general rule with renderstate support is that if the video card driver supports
 * the OpenGL equivalent to RenderWare's renderstate mode, then RenderWare Graphics will
 * support it. This rule has the following exceptions:
 *
 * \li \ref rwTEXTUREADDRESSCLAMP maps to \c GL_CLAMP_TO_EDGE if OpenGL 1.2 or the 
 * \c GL_EXT_texture_edge_clamp extension is supported. Otherwise it maps to \c GL_CLAMP.
 * Note that the latter case will clamp to black rather than the edge texture coordinates.
 *
 * \li \ref rwRENDERSTATETEXTUREADDRESS, \ref rwRENDERSTATETEXTUREADDRESSU, and \ref
 * rwRENDERSTATETEXTUREADDRESSV only supports the \ref rwTEXTUREADDRESSMIRROR mode if
 * the \c GL_ARB_texture_mirrored_repeat extension is supported.
 *
 * \li \ref rwRENDERSTATETEXTUREPERSPECTIVE is \e always enabled.
 *
 * \li \ref rwRENDERSTATETEXTUREADDRESS, \ref rwRENDERSTATETEXTUREADDRESSU, and \ref
 * rwRENDERSTATETEXTUREADDRESSV do not support the \ref rwTEXTUREADDRESSBORDER mode.
 *
 * \li \ref rwRENDERSTATEBORDERCOLOR is unsupported.
 *
 * \see \ref RwRenderState
 * \see \ref RwTextureAddressMode
 *
 */




/****************************************************************************
 Defines
 */
#define COLORSCALAR 0.003921568627450980392156862745098f /* 1.0f/ 255.0f */

typedef struct _rwOpenGLTextureUnitInfo _rwOpenGLTextureUnitInfo;
struct _rwOpenGLTextureUnitInfo
{
    RwRaster                *texRaster;

    RwTextureFilterMode     filterMode;

    RwTextureAddressMode    addressModeU;

    RwTextureAddressMode    addressModeV;

    RwInt8                  maxAnisotropy;
};

typedef struct _rwOpenGLRWRenderState _rwOpenGLRWRenderState;
struct _rwOpenGLRWRenderState
{
    RwShadeMode                 shadeMode;

    /* Texture information */
    RwUInt32                     activeTexUnit;          /* current active texture unit */

    _rwOpenGLTextureUnitInfo    texUnitInfo[rwMAXTEXTURECOORDS];    /* per texture unit information */
    RwBool                      lastTextureHadAlpha;                /* Did the last bound texture have an alpha channel */

    /* Z Buffer */
    RwBool                      depthTestOn;
    RwBool                      depthWriteOn;

    /* Alpha blending */
    RwBlendFunction             srcBlend;
    RwBlendFunction             destBlend;
    RwBool                      alphaBlendingOn;        /* True if vertex alpha is enabled */

    /* Fogging */
    RwUInt32                    fogColor;
    RwFogType                   fogType;
    RwReal                      fogDensity;
    RwReal                      fogStart;
    RwReal                      fogEnd;

    RwStencilFunction           stencilFunction;    /* default is GL_ALWAYS */
    RwInt32                     stencilReference;   /* default is 0 */
    RwUInt32                    stencilMask;        /* default is all 1's */
    RwUInt32                    stencilWriteMask;   /* default is all 1's */
    RwStencilOperation          stencilFail;        /* default is GL_KEEP */
    RwStencilOperation          stencilZFail;       /* default is GL_KEEP */
    RwStencilOperation          stencilZPass;       /* default is GL_KEEP */
    RwUInt32                    stencilClearVal;    /* default is 0 */

    RwBool                      cullEnabled;        /* is culling active? - same as rwCULLMODECULLNONE but in OpenGL terms */
    RwCullMode                  cullMode;           /* which cull mode is active? */

    RwAlphaTestFunction         alphaTest;          /* alpha test function */
    RwUInt8                     alphaRef;           /* alpha reference */
    RwReal                      alphaRefCache;      /* cached RwReal value of the alpha reference */
};


/* Render state lookup tables: RW->OGL */
static GLint SrcBlendModes[] =
{
    -1, GL_ZERO, GL_ONE, -1, -1, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
    GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_DST_COLOR,
    GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA_SATURATE
};

static GLint DstBlendModes[] =
{
    -1, GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_SRC_ALPHA,
    GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, -1, -1, -1
};

static GLenum StencilOperations[] =
{
    -1, GL_KEEP, GL_ZERO, GL_REPLACE, GL_INCR, GL_DECR, GL_INVERT
};

static GLenum StencilFunctions[] =
{
    -1, GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL,
    GL_GEQUAL, GL_ALWAYS
};

static GLenum AlphaTestFunctions[] =
{
    -1, GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL,
    GL_GEQUAL, GL_ALWAYS
};

static _rwOpenGLRWRenderState   _rwRenderState;

static _rwOpenGLTextureUnitInfo *activeTexUnitInfo = (_rwOpenGLTextureUnitInfo *)NULL; 

static RwInt32 openglAnisotTextureOffset = 0; /* 0 means deactivated */

#define CONSTTEXTUREGETANISOT(tex) \
    (*((const RwUInt8*)(((const RwUInt8*)tex) + openglAnisotTextureOffset)))


RwOpenGLStateData   RwOpenGLStateCache[rwGL_NUMSTATES] = { { 0, FALSE } };

static RwUInt32 RwOpenGLStateNameConvert[rwGL_NUMSTATES] =
{
    0,
    GL_ALPHA_TEST,
    GL_BLEND,
    GL_COLOR_MATERIAL,
    GL_CULL_FACE,
    GL_DEPTH_TEST,
    GL_FOG,
    GL_LIGHT0,
    GL_LIGHT1,
    GL_LIGHT2,
    GL_LIGHT3,
    GL_LIGHT4,
    GL_LIGHT5,
    GL_LIGHT6,
    GL_LIGHT7,
    GL_LIGHTING,
    GL_NORMALIZE,
    GL_SCISSOR_TEST,
    GL_STENCIL_TEST,
    GL_TEXTURE_2D,
    GL_TEXTURE_2D,
    GL_TEXTURE_2D,
    GL_TEXTURE_2D,
    GL_TEXTURE_2D,
    GL_TEXTURE_2D,
    GL_TEXTURE_2D,
    GL_TEXTURE_2D,
    GL_TEXTURE_GEN_S,
    GL_TEXTURE_GEN_S,
    GL_TEXTURE_GEN_S,
    GL_TEXTURE_GEN_S,
    GL_TEXTURE_GEN_S,
    GL_TEXTURE_GEN_S,
    GL_TEXTURE_GEN_S,
    GL_TEXTURE_GEN_S,
    GL_TEXTURE_GEN_T,
    GL_TEXTURE_GEN_T,
    GL_TEXTURE_GEN_T,
    GL_TEXTURE_GEN_T,
    GL_TEXTURE_GEN_T,
    GL_TEXTURE_GEN_T,
    GL_TEXTURE_GEN_T,
    GL_TEXTURE_GEN_T,
    GL_TEXTURE_GEN_R,
    GL_TEXTURE_GEN_R,
    GL_TEXTURE_GEN_R,
    GL_TEXTURE_GEN_R,
    GL_TEXTURE_GEN_R,
    GL_TEXTURE_GEN_R,
    GL_TEXTURE_GEN_R,
    GL_TEXTURE_GEN_R,
    GL_TEXTURE_GEN_Q,
    GL_TEXTURE_GEN_Q,
    GL_TEXTURE_GEN_Q,
    GL_TEXTURE_GEN_Q,
    GL_TEXTURE_GEN_Q,
    GL_TEXTURE_GEN_Q,
    GL_TEXTURE_GEN_Q,
    GL_TEXTURE_GEN_Q,
    GL_TEXTURE_CUBE_MAP_ARB
};

typedef RwBool (*RenderStateSetFn)( void *param );
static RenderStateSetFn renderStateSet[rwRENDERSTATEALPHATESTFUNCTIONREF + 1] = { (RenderStateSetFn)NULL };

static RenderStateSetFn _rwOpenGLSetTextureAddressU = (RenderStateSetFn)NULL;
static RenderStateSetFn _rwOpenGLSetTextureAddressV = (RenderStateSetFn)NULL;
static RenderStateSetFn _rwOpenGLSetTextureFilter   = (RenderStateSetFn)NULL;


static RwBool
_rwOpenGLSetTextureAddressU_CubeSupport_NoEdgeClamp( void *param );

static RwBool
_rwOpenGLSetTextureAddressU_CubeSupport_EdgeClamp( void *param );

static RwBool
_rwOpenGLSetTextureAddressV_CubeSupport_NoEdgeClamp( void *param );

static RwBool
_rwOpenGLSetTextureAddressV_CubeSupport_EdgeClamp( void *param );

static RwBool
_rwOpenGLSetTextureFilter_CubeSupport( void *param );

static RwBool
_rwOpenGLSetTextureAddressU_NoCubeSupport_NoEdgeClamp( void *param );

static RwBool
_rwOpenGLSetTextureAddressU_NoCubeSupport_EdgeClamp( void *param );

static RwBool
_rwOpenGLSetTextureAddressV_NoCubeSupport_NoEdgeClamp( void *param );

static RwBool
_rwOpenGLSetTextureAddressV_NoCubeSupport_EdgeClamp( void *param );

static RwBool
_rwOpenGLSetTextureFilter_NoCubeSupport( void *param );


/****************************************************************************
 _rwOpenGLRenderStateClose

 Closes the renderstate module by free'ing arrays that have been malloc'd.

 On entry   : Nothing
 On exit    : Nothing
 */
void
_rwOpenGLRenderStateClose( void )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLRenderStateClose" ) );

    RWRETURNVOID();
}


/****************************************************************************
 _rwOpenGLRSUnbindRasterBeforeDestroy

 If the currently bound raster is that being destroyed,
 unbind it to ensure that no problems in the future occur if its
 name is referenced
 */
void
_rwOpenGLRSUnbindRasterBeforeDestroy( RwRaster *raster )
{
    if ( raster == activeTexUnitInfo->texRaster )
    {
        _rwOpenGLSetRaster( NULL );
    }

    return;
}


/****************************************************************************
 _rwOpenGLSetTextureAnisotropyOffset

 Set the anisotropy texture offset that enables the anisotropy plugin
 */
void
_rwOpenGLSetTextureAnisotropyOffset( RwInt32 anisotTextureOffset )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLSetTextureAnisotropyOffset" ) );

    RWASSERT( anisotTextureOffset > 0 );

    openglAnisotTextureOffset = anisotTextureOffset;

    RWRETURNVOID();
}


/****************************************************************************
 _rwOpenGLSetRaster

 Bind the current raster to OpenGL. Also enable/disable GL_TEXTURE_2D where
 appropriate.
 */
void
_rwOpenGLSetRaster( RwRaster *raster )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLSetRaster" ) );

    if ( NULL == raster )
    {
        if ( NULL != activeTexUnitInfo->texRaster )
        {
            RwOpenGLDisable( rwGL_TEXTURE0_2D + _rwRenderState.activeTexUnit );
            if ( FALSE != _rwOpenGLExt.TextureCubeMapARB )
            {
                RwOpenGLDisable( rwGL_TEXTURE_CUBE_MAP_ARB );
            }

            if ( FALSE != _rwRenderState.lastTextureHadAlpha )
            {
                RwOpenGLDisable( rwGL_ALPHA_TEST );
                _rwRenderState.lastTextureHadAlpha = FALSE;
                if ( FALSE == _rwRenderState.alphaBlendingOn )
                {
                    RwOpenGLDisable( rwGL_BLEND );
                }
            }

            activeTexUnitInfo->texRaster = NULL;

            _rwOpenGLIm2DSetSubmitFuncs( (_rwRenderState.shadeMode ==
                                          rwSHADEMODEGOURAUD),
                                         FALSE );
        }
    }
    else
    {
        _rwOpenGLRasterExt  *rasterExt;


        if ( NULL == activeTexUnitInfo->texRaster )
        {
            _rwOpenGLIm2DSetSubmitFuncs( (_rwRenderState.shadeMode ==
                                          rwSHADEMODEGOURAUD),
                                         TRUE );

            RwOpenGLEnable( rwGL_TEXTURE0_2D + _rwRenderState.activeTexUnit );
        }

        rasterExt = RASTEREXTFROMRASTER(raster);
        RWASSERT( NULL != rasterExt );

        if ( raster != activeTexUnitInfo->texRaster )
        {
            activeTexUnitInfo->texRaster = raster;

            switch ( _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(raster->cFormat)].clientPixelFormat )
            {
            case GL_RGBA:
                {
                    if ( FALSE == _rwRenderState.alphaBlendingOn )
                    {
                        RwOpenGLEnable( rwGL_BLEND );
                    }

                    RwOpenGLEnable( rwGL_ALPHA_TEST );

                    _rwRenderState.lastTextureHadAlpha = TRUE;
                }
                break;

            case GL_RGB:
                {
                    if ( FALSE == _rwRenderState.alphaBlendingOn )
                    {
                        RwOpenGLDisable( rwGL_BLEND );
                    }

                    RwOpenGLDisable( rwGL_ALPHA_TEST );

                    _rwRenderState.lastTextureHadAlpha = FALSE;

                    break;
                }

            default:
                RWERROR( (E_RW_INVRASTERFORMAT) );
                RWRETURNVOID();
            }

            if ( 0 != rasterExt->textureID )
            {
                /* N.B. Something here is not liked by the Apple OpenGL
                 * implementation - my niggly feeling is glAreTexturesResident()
                 * but this needs further investigation
                 */
#if !defined(macintosh)
#if defined(RWMETRICS)
                /* The code to check for texture uploads here doesn't actually
                 * work.  This is because many cards don't properly implement
                 * the call and always insist that all textures are resident
                 */
                GLboolean   residentBefore;

                GLboolean   residentAfter;


                glAreTexturesResident( 1,
                                       (const GLuint*)(&rasterExt->textureID),
                                       &residentBefore );
#endif /* defined(RWMETRICS) */
#endif /* !defined(macintosh) */

                if ( FALSE == rasterExt->cubeMap )
                {
                    if ( FALSE != _rwOpenGLExt.TextureCubeMapARB )
                    {
                        RwOpenGLDisable( rwGL_TEXTURE_CUBE_MAP_ARB );
                    }

                    glBindTexture( GL_TEXTURE_2D, rasterExt->textureID );
                }
                else
                {
                    /* this ought to be the case if rasterExt->cubeMap is not FALSE */
                    RWASSERT( FALSE != _rwOpenGLExt.TextureCubeMapARB );

                    RwOpenGLEnable( rwGL_TEXTURE_CUBE_MAP_ARB );
                    glBindTexture( GL_TEXTURE_CUBE_MAP_ARB, rasterExt->textureID );
                }

#if defined(RWMETRICS)
#if !defined(macintosh)
                /* The finish here is just me being paranoid but I want to
                 * be certain that the texture has indeed been uploaded
                 * by the time I do my next check
                 */
                glFinish();
                glAreTexturesResident(1, (const GLuint*)(&rasterExt->textureID), &residentAfter);
                if ((!residentBefore) && residentAfter)
#endif /* !defined(macintosh) */
                {
                    RWSRCGLOBAL(metrics)->numTextureUploads++;
                    RWSRCGLOBAL(metrics)->sizeTextureUploads += raster->width *
                                                                 raster->height *
                                                                  (raster->depth >> 3);
                }
#endif /* defined(RWMETRICS) */
            }
        }
    }

    GL_ERROR_CHECK();

    RWRETURNVOID();
}


/****************************************************************************
 _rwOpenGLSetRasterForceTextureParams

 Bind the current raster to OpenGL. Also enable/disable GL_TEXTURE_2D where
 appropriate.
 */
RwBool
_rwOpenGLSetRasterForceTextureParams( void *rasterVoid )
{
    RwRaster    *raster = (RwRaster *)rasterVoid;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetRasterForceTextureParams" ) );

    if ( NULL == raster )
    {
        if ( NULL != activeTexUnitInfo->texRaster )
        {
            RwOpenGLDisable( rwGL_TEXTURE0_2D + _rwRenderState.activeTexUnit );
            if ( FALSE != _rwOpenGLExt.TextureCubeMapARB )
            {
                RwOpenGLDisable( rwGL_TEXTURE_CUBE_MAP_ARB );
            }

            if ( FALSE != _rwRenderState.lastTextureHadAlpha )
            {
                RwOpenGLDisable( rwGL_ALPHA_TEST );
                _rwRenderState.lastTextureHadAlpha = FALSE;
                if ( FALSE == _rwRenderState.alphaBlendingOn )
                {
                    RwOpenGLDisable( rwGL_BLEND );
                }
            }

            activeTexUnitInfo->texRaster = NULL;

            _rwOpenGLIm2DSetSubmitFuncs( (_rwRenderState.shadeMode ==
                                          rwSHADEMODEGOURAUD),
                                         FALSE );
        }
    }
    else
    {
        _rwOpenGLRasterExt  *rasterExt;


        if ( NULL == activeTexUnitInfo->texRaster )
        {
            _rwOpenGLIm2DSetSubmitFuncs( (_rwRenderState.shadeMode ==
                                          rwSHADEMODEGOURAUD),
                                         TRUE );

            RwOpenGLEnable( rwGL_TEXTURE0_2D + _rwRenderState.activeTexUnit );
        }

        rasterExt = RASTEREXTFROMRASTER(raster);
        RWASSERT( NULL != rasterExt );

        if ( raster != activeTexUnitInfo->texRaster )
        {
            activeTexUnitInfo->texRaster = raster;

            switch ( _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(raster->cFormat)].clientPixelFormat )
            {
            case GL_RGBA:
                {
                    if ( FALSE == _rwRenderState.alphaBlendingOn )
                    {
                        RwOpenGLEnable( rwGL_BLEND );
                    }

                    RwOpenGLEnable( rwGL_ALPHA_TEST );

                    _rwRenderState.lastTextureHadAlpha = TRUE;
                }
                break;

            case GL_RGB:
                {
                    if ( FALSE == _rwRenderState.alphaBlendingOn )
                    {
                        RwOpenGLDisable( rwGL_BLEND );
                    }

                    RwOpenGLDisable( rwGL_ALPHA_TEST );

                    _rwRenderState.lastTextureHadAlpha = FALSE;

                    break;
                }

            default:
                RWERROR( (E_RW_INVRASTERFORMAT) );
                RWRETURN( FALSE );
            }

            if ( 0 != rasterExt->textureID )
            {
                /* N.B. Something here is not liked by the Apple OpenGL
                 * implementation - my niggly feeling is glAreTexturesResident()
                 * but this needs further investigation
                 */
#if !defined(macintosh)
#if defined(RWMETRICS)
                /* The code to check for texture uploads here doesn't actually
                 * work.  This is because many cards don't properly implement
                 * the call and always insist that all textures are resident
                 */
                GLboolean   residentBefore;

                GLboolean   residentAfter;


                glAreTexturesResident( 1,
                                       (const GLuint*)(&rasterExt->textureID),
                                       &residentBefore );
#endif /* defined(RWMETRICS) */
#endif /* !defined(macintosh) */

                if ( FALSE == rasterExt->cubeMap )
                {
                    if ( FALSE != _rwOpenGLExt.TextureCubeMapARB )
                    {
                        RwOpenGLDisable( rwGL_TEXTURE_CUBE_MAP_ARB );
                    }

                    glBindTexture( GL_TEXTURE_2D, rasterExt->textureID );
                }
                else
                {
                    /* this ought to be the case if rasterExt->cubeMap is not FALSE */
                    RWASSERT( FALSE != _rwOpenGLExt.TextureCubeMapARB );

                    RwOpenGLEnable( rwGL_TEXTURE_CUBE_MAP_ARB );
                    glBindTexture( GL_TEXTURE_CUBE_MAP_ARB, rasterExt->textureID );
                }

#if defined(RWMETRICS)
#if !defined(macintosh)
                /* The finish here is just me being paranoid but I want to
                 * be certain that the texture has indeed been uploaded
                 * by the time I do my next check
                 */
                glFinish();
                glAreTexturesResident(1, (const GLuint*)(&rasterExt->textureID), &residentAfter);
                if ((!residentBefore) && residentAfter)
#endif /* !defined(macintosh) */
                {
                    RWSRCGLOBAL(metrics)->numTextureUploads++;
                    RWSRCGLOBAL(metrics)->sizeTextureUploads += raster->width *
                                                                 raster->height *
                                                                  (raster->depth >> 3);
                }
#endif /* defined(RWMETRICS) */
            }
        }

        /* now force the texture params */
        _rwOpenGLSetTextureAddressU( (void *)(activeTexUnitInfo->addressModeU) );
        _rwOpenGLSetTextureAddressV( (void*)(activeTexUnitInfo->addressModeV) );
        _rwOpenGLSetTextureFilter( (void *)(activeTexUnitInfo->filterMode) );
    }

    GL_ERROR_CHECK();

    RWRETURN( TRUE );
}


/**
 * \ingroup rwenginemultitextureopengl
 * \ref RwOpenGLSetActiveTextureUnit sets the active texture unit (but \e not
 * the client active texture unit) on a multi-texturing supported system.
 * No change will be made if the requested texture unit index is out of the
 * bounds [0...\e max \e texture \e units - 1].
 *
 * Comparing this function's return value with the argument passed to it will
 * determine whether the specified texture unit was successfully set.
 *
 * On a single texture system, this function will have no effect as the
 * single texture unit is always selected.
 *
 * \param textureUnit Constant \ref RwUInt32 containing the zero-based
 *                    texture unit index to select.
 *
 * \return \ref RwUInt32 containing the index of the currently selected
 *         texture unit.
 *
 * \see RwOpenGLGetActiveTextureUnit
 * \see RwOpenGLExtensions
 *
 */
RwUInt32
RwOpenGLSetActiveTextureUnit( const RwUInt32 textureUnit )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLSetActiveTextureUnit" ) );

    /* early out */
    if ( NULL == _rwOpenGLExt.ActiveTextureARB )
    {
        RWASSERT( 0 == _rwRenderState.activeTexUnit );
        RWRETURN( _rwRenderState.activeTexUnit );
    }

    /* early out */
    if ( textureUnit == _rwRenderState.activeTexUnit )
    {
        RWRETURN( _rwRenderState.activeTexUnit );
    }

    if ( textureUnit < _rwOpenGLExt.MaxTextureUnits )
    {
        _rwOpenGLExt.ActiveTextureARB( GL_TEXTURE0_ARB + textureUnit );
        GL_ERROR_CHECK();

        _rwRenderState.activeTexUnit = textureUnit;
        activeTexUnitInfo = _rwRenderState.texUnitInfo + textureUnit;
    }
#if defined(RWDEBUG)
    else
    {
        RWMESSAGE( (RWSTRING("Cannot select texture unit %d. There "\
                             "is a maximum of %d texture units."),
                             textureUnit,
                             _rwOpenGLExt.MaxTextureUnits ) );
    }
#endif /* defined(RWDEBUG) */

    RWRETURN( _rwRenderState.activeTexUnit );
}


/**
 * \ingroup rwenginemultitextureopengl
 * \ref RwOpenGLGetActiveTextureUnit returns the index of the currently
 * selected texture unit. A single texture system will always return 0.
 *
 * \return \ref RwUInt8 containing the zero-based index of the currently
 *         selected texture unit.
 *
 * \see RwOpenGLSetActiveTextureUnit
 *
 */
RwUInt32
RwOpenGLGetActiveTextureUnit( void )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLSetActiveTextureUnit" ) );

    RWRETURN( _rwRenderState.activeTexUnit );
}


/**
 * \ingroup rwrenderstateopengl
 * \ref RwOpenGLSetTexture binds an \ref RwTexture to the currently selected
 * texture unit. If NULL is passed as the argument, then textures are unbound
 * from the currently selected texture unit.
 *
 * OpenGL 2D texturing is enabled/disabled where necessary by this function.
 *
 * \param textureVoid Pointer to \ref RwTexture to bind, or NULL to unbind
 *                    textures from the currently selected texture unit.
 *
 * \see RwOpenGLGetActiveTextureUnit
 * \see RwOpenGLSetActiveTextureUnit
 *
 */
void
RwOpenGLSetTexture( void *textureVoid )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLSetTexture" ) );

    if ( NULL != textureVoid )
    {
        RwTexture               *texture = (RwTexture *)textureVoid;

        RwTextureAddressMode    addressingU;

        RwTextureAddressMode    addressingV;

        RwTextureFilterMode     filtering;


        /* set the texture raster, ignoring addressing and filter modes */
        _rwOpenGLSetRaster( texture->raster );

        /* get and set the addressing and filter modes */
        addressingU = RwTextureGetAddressingU(texture);
        addressingV = RwTextureGetAddressingV(texture);
        filtering = RwTextureGetFilterMode(texture);

        _rwOpenGLSetTextureAddressU( (void *)addressingU );
  
        _rwOpenGLSetTextureAddressV( (void*)addressingV );

        _rwOpenGLSetTextureFilter( (void*)filtering );

        /* is texture anisotropic filtering supported? */
        if ( 0 != _rwOpenGLExt.MaxTextureAnisotropy )
        {
            RwInt8  anisotropy;


            anisotropy = 1;
            if ( openglAnisotTextureOffset > 0 )
            {
                anisotropy = CONSTTEXTUREGETANISOT(texture);
            }

            if ( anisotropy > _rwOpenGLExt.MaxTextureAnisotropy )
            {
                anisotropy = _rwOpenGLExt.MaxTextureAnisotropy;
                RWMESSAGE( (RWSTRING( "Clamping anisotropy to the "\
                                      "maximum supported, %d" ),
                            _rwOpenGLExt.MaxTextureAnisotropy) );
            }

            if ( anisotropy != activeTexUnitInfo->maxAnisotropy )
            {
                _rwOpenGLRasterExt  *rasterExt;


                rasterExt = RASTEREXTFROMRASTER(texture->raster);
                RWASSERT( NULL != rasterExt );

                if ( FALSE == rasterExt->cubeMap )
                {
                    glTexParameteri( GL_TEXTURE_2D,
                                     GL_TEXTURE_MAX_ANISOTROPY_EXT,
                                     (GLint)anisotropy );
                }
                else
                {
                    glTexParameteri( GL_TEXTURE_CUBE_MAP_ARB,
                                     GL_TEXTURE_MAX_ANISOTROPY_EXT,
                                     (GLint)anisotropy );
                }

                activeTexUnitInfo->maxAnisotropy = anisotropy;
            }
        }
    }
    else
    {
        _rwOpenGLSetRaster( NULL );
        RwOpenGLVADisableTexCoord( _rwRenderState.activeTexUnit );
    }

    GL_ERROR_CHECK();

    RWRETURNVOID();
}


/****************************************************************************
 _rwOpenGLSetFogEnable

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetFogEnable( void *param )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLSetFogEnable" ) );

    if ( FALSE == (RwBool)param )
    {
        RwOpenGLDisable( rwGL_FOG );

        _rwOpenGLIm2DSetVertexFuncs( _rwRenderState.depthTestOn,
                                     _rwRenderState.depthWriteOn,
                                     _rwRenderState.alphaBlendingOn,
                                     FALSE );
    }
    else
    {
        RwOpenGLEnable( rwGL_FOG );

        _rwOpenGLIm2DSetVertexFuncs( _rwRenderState.depthTestOn,
                                     _rwRenderState.depthWriteOn,
                                     _rwRenderState.alphaBlendingOn,
                                     TRUE );
    }

    GL_ERROR_CHECK();

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetFogColor

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetFogColor( void *param )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLSetFogColor" ) );

    if (_rwRenderState.fogColor != (RwUInt32)param)
    {
        GLfloat fogColour[4];

        /* Cache the fog color - a packed int in ARGB format */
        _rwRenderState.fogColor = (RwUInt32)param;

        fogColour[0] = (GLfloat)((_rwRenderState.fogColor >> 16) & 0x000000ff) * COLORSCALAR;
        fogColour[1] = (GLfloat)((_rwRenderState.fogColor >> 8)  & 0x000000ff) * COLORSCALAR;
        fogColour[2] = (GLfloat)((_rwRenderState.fogColor)       & 0x000000ff) * COLORSCALAR;
        fogColour[3] = (GLfloat)((_rwRenderState.fogColor >> 24) & 0x000000ff) * COLORSCALAR;

        /* Set the color */
        glFogfv(GL_FOG_COLOR, fogColour);

        GL_ERROR_CHECK();
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetFogType

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetFogType( void *param )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLSetFogType" ) );

    if (_rwRenderState.fogType != (RwFogType)param)
    {
        _rwRenderState.fogType = (RwFogType)param;

        switch (_rwRenderState.fogType)
        {
        case rwFOGTYPELINEAR:
            glFogi(GL_FOG_MODE, GL_LINEAR);
            break;
        case rwFOGTYPEEXPONENTIAL:
            glFogi(GL_FOG_MODE, GL_EXP);
            break;
        case rwFOGTYPEEXPONENTIAL2:
            glFogi(GL_FOG_MODE, GL_EXP2);
            break;
        default:
            /* Invalid fog type */
            RWRETURN(FALSE);
        }

        GL_ERROR_CHECK();
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetFogDensity

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetFogDensity( void *param )
{
    RwReal density = *(RwReal *)param;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetFogDensity" ) );

    if (_rwRenderState.fogDensity != density)
    {
        _rwRenderState.fogDensity = density;
        glFogf(GL_FOG_DENSITY, density);

        GL_ERROR_CHECK();
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetTextureAddressU_CubeSupport_NoEdgeClamp

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetTextureAddressU_CubeSupport_NoEdgeClamp( void *param )
{
    RwTextureAddressMode addressMode = (RwTextureAddressMode)param;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetTextureAddressU_CubeSupport_NoEdgeClamp" ) );

    if ( NULL != activeTexUnitInfo->texRaster )
    {
        _rwOpenGLRasterExt *rasterExt;
        
        
        rasterExt = RASTEREXTFROMRASTER(activeTexUnitInfo->texRaster);
        RWASSERT( NULL != rasterExt );

        if ( rasterExt->addressModeU != addressMode )
        {
            switch (addressMode)
            {
            case rwTEXTUREADDRESSWRAP:
                {
                    if ( FALSE == rasterExt->cubeMap )
                    {
                        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
                    }
                    else
                    {
                        glTexParameteri( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_REPEAT );
                    }
                }
                break;

            case rwTEXTUREADDRESSCLAMP:
                {
                    if ( FALSE == rasterExt->cubeMap )
                    {
                        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
                    }
                    else
                    {
                        glTexParameteri( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP );
                    }
                }
                break;

            case rwTEXTUREADDRESSMIRROR:
                if ( FALSE != _rwOpenGLExt.TextureMirroredRepeatARB )
                {
                    if ( FALSE == rasterExt->cubeMap )
                    {
                        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT_ARB );
                    }
                    else
                    {
                        glTexParameteri( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT_ARB );
                    }
                }
                else
                {
                    /* unsupported */
                    RWRETURN( FALSE );
                }
                break;

            case rwTEXTUREADDRESSBORDER:
                /* fall thru */
            default:
                {
                    /* Unsupported mapping types */
                    RWRETURN( FALSE );
                }
            }

            GL_ERROR_CHECK();

            /* Update addressing for this bound texture */
            rasterExt->addressModeU = addressMode;
        }
    }

    if ( (addressMode == rwTEXTUREADDRESSWRAP) ||
         (addressMode == rwTEXTUREADDRESSCLAMP) )
    {
        /* Update global addressing */
        activeTexUnitInfo->addressModeU = addressMode;
    }
    else if ( (addressMode == rwTEXTUREADDRESSMIRROR) &&
              (FALSE != _rwOpenGLExt.TextureMirroredRepeatARB) )
    {
        /* Update global addressing */
        activeTexUnitInfo->addressModeU = addressMode;
    }
    else
    {
        RWRETURN(FALSE);
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetTextureAddressU_CubeSupport_EdgeClamp

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetTextureAddressU_CubeSupport_EdgeClamp( void *param )
{
    RwTextureAddressMode addressMode = (RwTextureAddressMode)param;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetTextureAddressU_CubeSupport_EdgeClamp" ) );

    if ( NULL != activeTexUnitInfo->texRaster )
    {
        _rwOpenGLRasterExt *rasterExt;
        
        
        rasterExt = RASTEREXTFROMRASTER(activeTexUnitInfo->texRaster);
        RWASSERT( NULL != rasterExt );

        if ( rasterExt->addressModeU != addressMode )
        {
            switch (addressMode)
            {
            case rwTEXTUREADDRESSWRAP:
                {
                    if ( FALSE == rasterExt->cubeMap )
                    {
                        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
                    }
                    else
                    {
                        glTexParameteri( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_REPEAT );
                    }
                }
                break;

            case rwTEXTUREADDRESSCLAMP:
                {
                    if ( FALSE == rasterExt->cubeMap )
                    {
                        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE_EXT );
                    }
                    else
                    {
                        glTexParameteri( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE_EXT );
                    }
                }
                break;

            case rwTEXTUREADDRESSMIRROR:
                if ( FALSE != _rwOpenGLExt.TextureMirroredRepeatARB )
                {
                    if ( FALSE == rasterExt->cubeMap )
                    {
                        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT_ARB );
                    }
                    else
                    {
                        glTexParameteri( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT_ARB );
                    }
                }
                else
                {
                    /* unsupported */
                    RWRETURN( FALSE );
                }
                break;

            case rwTEXTUREADDRESSBORDER:
                /* fall thru */
            default:
                {
                    /* Unsupported mapping types */
                    RWRETURN( FALSE );
                }
            }

            GL_ERROR_CHECK();

            /* Update addressing for this bound texture */
            rasterExt->addressModeU = addressMode;
        }
    }

    if ( (addressMode == rwTEXTUREADDRESSWRAP) ||
         (addressMode == rwTEXTUREADDRESSCLAMP) )
    {
        /* Update global addressing */
        activeTexUnitInfo->addressModeU = addressMode;
    }
    else if ( (addressMode == rwTEXTUREADDRESSMIRROR) &&
              (FALSE != _rwOpenGLExt.TextureMirroredRepeatARB) )
    {
        /* Update global addressing */
        activeTexUnitInfo->addressModeU = addressMode;
    }
    else
    {
        RWRETURN(FALSE);
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetTextureAddressU_NoCubeSupport_NoEdgeClamp

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetTextureAddressU_NoCubeSupport_NoEdgeClamp( void *param )
{
    RwTextureAddressMode addressMode = (RwTextureAddressMode)param;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetTextureAddressU_NoCubeSupport_NoEdgeClamp" ) );

    RWASSERT( FALSE == _rwOpenGLExt.TextureCubeMapARB );

    if ( NULL != activeTexUnitInfo->texRaster )
    {
        _rwOpenGLRasterExt *rasterExt;
        
        
        rasterExt = RASTEREXTFROMRASTER(activeTexUnitInfo->texRaster);
        RWASSERT( NULL != rasterExt );
        RWASSERT( FALSE == rasterExt->cubeMap );

        if ( rasterExt->addressModeU != addressMode )
        {
            switch (addressMode)
            {
            case rwTEXTUREADDRESSWRAP:
                {
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
                }
                break;

            case rwTEXTUREADDRESSCLAMP:
                {
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
                }
                break;

            case rwTEXTUREADDRESSMIRROR:
                if ( FALSE != _rwOpenGLExt.TextureMirroredRepeatARB )
                {
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT_ARB );
                }
                else
                {
                    /* unsupported */
                    RWRETURN( FALSE );
                }
                break;

            case rwTEXTUREADDRESSBORDER:
                /* fall thru */
            default:
                {
                    /* Unsupported mapping types */
                    RWRETURN( FALSE );
                }
            }

            GL_ERROR_CHECK();

            /* Update addressing for this bound texture */
            rasterExt->addressModeU = addressMode;
        }
    }

    if ( (addressMode == rwTEXTUREADDRESSWRAP) ||
         (addressMode == rwTEXTUREADDRESSCLAMP) )
    {
        /* Update global addressing */
        activeTexUnitInfo->addressModeU = addressMode;
    }
    else if ( (addressMode == rwTEXTUREADDRESSMIRROR) &&
              (FALSE != _rwOpenGLExt.TextureMirroredRepeatARB) )
    {
        /* Update global addressing */
        activeTexUnitInfo->addressModeU = addressMode;
    }
    else
    {
        RWRETURN(FALSE);
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetTextureAddressU_NoCubeSupport_EdgeClamp

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetTextureAddressU_NoCubeSupport_EdgeClamp( void *param )
{
    RwTextureAddressMode addressMode = (RwTextureAddressMode)param;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetTextureAddressU_NoCubeSupport_EdgeClamp" ) );

    RWASSERT( FALSE == _rwOpenGLExt.TextureCubeMapARB );

    if ( NULL != activeTexUnitInfo->texRaster )
    {
        _rwOpenGLRasterExt *rasterExt;
        
        
        rasterExt = RASTEREXTFROMRASTER(activeTexUnitInfo->texRaster);
        RWASSERT( NULL != rasterExt );
        RWASSERT( FALSE == rasterExt->cubeMap );

        if ( rasterExt->addressModeU != addressMode )
        {
            switch (addressMode)
            {
            case rwTEXTUREADDRESSWRAP:
                {
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
                }
                break;

            case rwTEXTUREADDRESSCLAMP:
                {
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE_EXT );
                }
                break;

            case rwTEXTUREADDRESSMIRROR:
                if ( FALSE != _rwOpenGLExt.TextureMirroredRepeatARB )
                {
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT_ARB );
                }
                else
                {
                    /* unsupported */
                    RWRETURN( FALSE );
                }
                break;

            case rwTEXTUREADDRESSBORDER:
                /* fall thru */
            default:
                {
                    /* Unsupported mapping types */
                    RWRETURN( FALSE );
                }
            }

            GL_ERROR_CHECK();

            /* Update addressing for this bound texture */
            rasterExt->addressModeU = addressMode;
        }
    }

    if ( (addressMode == rwTEXTUREADDRESSWRAP) ||
         (addressMode == rwTEXTUREADDRESSCLAMP) )
    {
        /* Update global addressing */
        activeTexUnitInfo->addressModeU = addressMode;
    }
    else if ( (addressMode == rwTEXTUREADDRESSMIRROR) &&
              (FALSE != _rwOpenGLExt.TextureMirroredRepeatARB) )
    {
        /* Update global addressing */
        activeTexUnitInfo->addressModeU = addressMode;
    }
    else
    {
        RWRETURN(FALSE);
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetTextureAddressV_CubeSupport_NoEdgeClamp

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetTextureAddressV_CubeSupport_NoEdgeClamp( void *param )
{
    RwTextureAddressMode addressMode = (RwTextureAddressMode)param;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetTextureAddressV_CubeSupport_NoEdgeClamp" ) );

    if ( NULL != activeTexUnitInfo->texRaster )
    {
        _rwOpenGLRasterExt *rasterExt;
        
        
        rasterExt = RASTEREXTFROMRASTER(activeTexUnitInfo->texRaster);
        RWASSERT( NULL != rasterExt );

        if ( rasterExt->addressModeV != addressMode )
        {
            switch (addressMode)
            {
            case rwTEXTUREADDRESSWRAP:
                {
                    if ( FALSE == rasterExt->cubeMap )
                    {
                        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
                    }
                    else
                    {
                        glTexParameteri( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_REPEAT );
                    }
                }
                break;

            case rwTEXTUREADDRESSCLAMP:
                {
                    if ( FALSE == rasterExt->cubeMap )
                    {
                        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
                    }
                    else
                    {
                        glTexParameteri( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP );
                    }
                }
                break;

            case rwTEXTUREADDRESSMIRROR:
                if ( FALSE != _rwOpenGLExt.TextureMirroredRepeatARB )
                {
                    if ( FALSE == rasterExt->cubeMap )
                    {
                        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT_ARB );
                    }
                    else
                    {
                        glTexParameteri( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT_ARB );
                    }
                }
                else
                {
                    /* unsupported */
                    RWRETURN( FALSE );
                }
                break;

            case rwTEXTUREADDRESSBORDER:
                /* fall thru */
            default:
                {
                    /* Unsupported mapping types */
                    RWRETURN( FALSE );
                }
            }

            GL_ERROR_CHECK();

            /* Update addressing for this bound texture */
            rasterExt->addressModeV = addressMode;
        }
    }

    if ( (addressMode == rwTEXTUREADDRESSWRAP) ||
         (addressMode == rwTEXTUREADDRESSCLAMP) )
    {
        /* Update global addressing */
        activeTexUnitInfo->addressModeU = addressMode;
    }
    else if ( (addressMode == rwTEXTUREADDRESSMIRROR) &&
              (FALSE != _rwOpenGLExt.TextureMirroredRepeatARB) )
    {
        /* Update global addressing */
        activeTexUnitInfo->addressModeU = addressMode;
    }
    else
    {
        RWRETURN(FALSE);
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetTextureAddressV_CubeSupport_EdgeClamp

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetTextureAddressV_CubeSupport_EdgeClamp( void *param )
{
    RwTextureAddressMode addressMode = (RwTextureAddressMode)param;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetTextureAddressV_CubeSupport_EdgeClamp" ) );

    if ( NULL != activeTexUnitInfo->texRaster )
    {
        _rwOpenGLRasterExt *rasterExt;
        
        
        rasterExt = RASTEREXTFROMRASTER(activeTexUnitInfo->texRaster);
        RWASSERT( NULL != rasterExt );

        if ( rasterExt->addressModeV != addressMode )
        {
            switch (addressMode)
            {
            case rwTEXTUREADDRESSWRAP:
                {
                    if ( FALSE == rasterExt->cubeMap )
                    {
                        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
                    }
                    else
                    {
                        glTexParameteri( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_REPEAT );
                    }
                }
                break;

            case rwTEXTUREADDRESSCLAMP:
                {
                    if ( FALSE == rasterExt->cubeMap )
                    {
                        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE_EXT );
                    }
                    else
                    {
                        glTexParameteri( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE_EXT );
                    }
                }
                break;

            case rwTEXTUREADDRESSMIRROR:
                if ( FALSE != _rwOpenGLExt.TextureMirroredRepeatARB )
                {
                    if ( FALSE == rasterExt->cubeMap )
                    {
                        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT_ARB );
                    }
                    else
                    {
                        glTexParameteri( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT_ARB );
                    }
                }
                else
                {
                    /* unsupported */
                    RWRETURN( FALSE );
                }
                break;

            case rwTEXTUREADDRESSBORDER:
                /* fall thru */
            default:
                {
                    /* Unsupported mapping types */
                    RWRETURN( FALSE );
                }
            }

            GL_ERROR_CHECK();

            /* Update addressing for this bound texture */
            rasterExt->addressModeV = addressMode;
        }
    }

    if ( (addressMode == rwTEXTUREADDRESSWRAP) ||
         (addressMode == rwTEXTUREADDRESSCLAMP) )
    {
        /* Update global addressing */
        activeTexUnitInfo->addressModeU = addressMode;
    }
    else if ( (addressMode == rwTEXTUREADDRESSMIRROR) &&
              (FALSE != _rwOpenGLExt.TextureMirroredRepeatARB) )
    {
        /* Update global addressing */
        activeTexUnitInfo->addressModeU = addressMode;
    }
    else
    {
        RWRETURN(FALSE);
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetTextureAddressV_NoCubeSupport_NoEdgeClamp

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetTextureAddressV_NoCubeSupport_NoEdgeClamp( void *param )
{
    RwTextureAddressMode addressMode = (RwTextureAddressMode)param;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetTextureAddressV_NoCubeSupport_NoEdgeClamp" ) );

    RWASSERT( FALSE == _rwOpenGLExt.TextureCubeMapARB );

    if ( NULL != activeTexUnitInfo->texRaster )
    {
        _rwOpenGLRasterExt *rasterExt;
        
        
        rasterExt = RASTEREXTFROMRASTER(activeTexUnitInfo->texRaster);
        RWASSERT( NULL != rasterExt );
        RWASSERT( FALSE == rasterExt->cubeMap );

        if ( rasterExt->addressModeV != addressMode )
        {
            switch (addressMode)
            {
            case rwTEXTUREADDRESSWRAP:
                {
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
                }
                break;

            case rwTEXTUREADDRESSCLAMP:
                {
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
                }
                break;

            case rwTEXTUREADDRESSMIRROR:
                if ( FALSE != _rwOpenGLExt.TextureMirroredRepeatARB )
                {
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT_ARB );
                }
                else
                {
                    /* unsupported */
                    RWRETURN( FALSE );
                }
                break;

            case rwTEXTUREADDRESSBORDER:
                /* fall thru */
            default:
                {
                    /* Unsupported mapping types */
                    RWRETURN( FALSE );
                }
            }

            GL_ERROR_CHECK();

            /* Update addressing for this bound texture */
            rasterExt->addressModeV = addressMode;
        }
    }

    if ( (addressMode == rwTEXTUREADDRESSWRAP) ||
         (addressMode == rwTEXTUREADDRESSCLAMP) )
    {
        /* Update global addressing */
        activeTexUnitInfo->addressModeU = addressMode;
    }
    else if ( (addressMode == rwTEXTUREADDRESSMIRROR) &&
              (FALSE != _rwOpenGLExt.TextureMirroredRepeatARB) )
    {
        /* Update global addressing */
        activeTexUnitInfo->addressModeU = addressMode;
    }
    else
    {
        RWRETURN(FALSE);
    }

    RWRETURN( TRUE );
}



/****************************************************************************
 _rwOpenGLSetTextureAddressV_NoCubeSupport_EdgeClamp

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetTextureAddressV_NoCubeSupport_EdgeClamp( void *param )
{
    RwTextureAddressMode addressMode = (RwTextureAddressMode)param;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetTextureAddressV_NoCubeSupport_EdgeClamp" ) );

    RWASSERT( FALSE == _rwOpenGLExt.TextureCubeMapARB );

    if ( NULL != activeTexUnitInfo->texRaster )
    {
        _rwOpenGLRasterExt *rasterExt;
        
        
        rasterExt = RASTEREXTFROMRASTER(activeTexUnitInfo->texRaster);
        RWASSERT( NULL != rasterExt );
        RWASSERT( FALSE == rasterExt->cubeMap );

        if ( rasterExt->addressModeV != addressMode )
        {
            switch (addressMode)
            {
            case rwTEXTUREADDRESSWRAP:
                {
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
                }
                break;

            case rwTEXTUREADDRESSCLAMP:
                {
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE_EXT );
                }
                break;

            case rwTEXTUREADDRESSMIRROR:
                if ( FALSE != _rwOpenGLExt.TextureMirroredRepeatARB )
                {
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT_ARB );
                }
                else
                {
                    /* unsupported */
                    RWRETURN( FALSE );
                }
                break;

            case rwTEXTUREADDRESSBORDER:
                /* fall thru */
            default:
                {
                    /* Unsupported mapping types */
                    RWRETURN( FALSE );
                }
            }

            GL_ERROR_CHECK();

            /* Update addressing for this bound texture */
            rasterExt->addressModeV = addressMode;
        }
    }

    if ( (addressMode == rwTEXTUREADDRESSWRAP) ||
         (addressMode == rwTEXTUREADDRESSCLAMP) )
    {
        /* Update global addressing */
        activeTexUnitInfo->addressModeU = addressMode;
    }
    else if ( (addressMode == rwTEXTUREADDRESSMIRROR) &&
              (FALSE != _rwOpenGLExt.TextureMirroredRepeatARB) )
    {
        /* Update global addressing */
        activeTexUnitInfo->addressModeU = addressMode;
    }
    else
    {
        RWRETURN(FALSE);
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetTextureAddress

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetTextureAddress( void *param )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLSetTextureAddress" ) );

    if (!_rwOpenGLSetTextureAddressU(param))
    {
        RWRETURN(FALSE);
    }

    if (!_rwOpenGLSetTextureAddressV(param))
    {
        RWRETURN(FALSE);
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetShadeMode

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetShadeMode( void *param )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLSetShadeMode" ) );

    switch ((RwShadeMode)param)
    {
    case rwSHADEMODEFLAT:
        if (_rwRenderState.shadeMode != rwSHADEMODEFLAT)
        {
            /* Flat shaded */
            _rwRenderState.shadeMode = rwSHADEMODEFLAT;

            glShadeModel(GL_FLAT);

            _rwOpenGLIm2DSetSubmitFuncs( FALSE,
                                         (activeTexUnitInfo->texRaster != NULL) );
        }
        break;
    case rwSHADEMODEGOURAUD:
        if (_rwRenderState.shadeMode != rwSHADEMODEGOURAUD)
        {
            /* Gouraud shading */
            _rwRenderState.shadeMode = rwSHADEMODEGOURAUD;

            glShadeModel(GL_SMOOTH);

            _rwOpenGLIm2DSetSubmitFuncs( TRUE,
                                         (activeTexUnitInfo->texRaster != NULL) );
        }
        break;
    default:
        /* What is this mode you naughty programmer?? */
        RWRETURN(FALSE);
    }

    GL_ERROR_CHECK();

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetVertexAlphaEnable

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetVertexAlphaEnable( void *param )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLSetVertexAlphaEnable" ) );

    if ((RwBool)param)
    {
        if (!(_rwRenderState.alphaBlendingOn))
        {
            _rwRenderState.alphaBlendingOn = TRUE;

            /* If we've already got an alpha texture, blending is enabled */
            if (!_rwRenderState.lastTextureHadAlpha)
            {
                RwOpenGLEnable( rwGL_BLEND );
            }

            _rwOpenGLIm2DSetVertexFuncs( _rwRenderState.depthTestOn,
                                         _rwRenderState.depthWriteOn,
                                         TRUE,
                                         RwOpenGLIsEnabled( rwGL_FOG ) );
        }
    }
    else
    {
        if ((_rwRenderState.alphaBlendingOn))
        {
            glColor4f(0, 0, 0, 1.0f);

            _rwRenderState.alphaBlendingOn = FALSE;

            /* Only disable blending if we don't have an alpha texture */
            if (!_rwRenderState.lastTextureHadAlpha)
            {
                RwOpenGLDisable( rwGL_BLEND );
            }

            _rwOpenGLIm2DSetVertexFuncs( _rwRenderState.depthTestOn,
                                         _rwRenderState.depthWriteOn,
                                         FALSE,
                                         RwOpenGLIsEnabled( rwGL_FOG ) );
        }
    }

    GL_ERROR_CHECK();

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetSrcBlend

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetSrcBlend( void *param )
{
    RwBlendFunction blendFunc = (RwBlendFunction)param;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetSrcBlend" ) );

    /* Check we inside the array bounds and that this is a
     * valid blend function valid for OpenGL
     */
    if (((blendFunc < 1) || (blendFunc > 11)) ||
        (SrcBlendModes[blendFunc] == -1))
    {
        RWRETURN(FALSE);
    }

    if (blendFunc != _rwRenderState.srcBlend)
    {
        _rwRenderState.srcBlend  = blendFunc;
        glBlendFunc(SrcBlendModes[blendFunc], DstBlendModes[_rwRenderState.destBlend]);

        GL_ERROR_CHECK();
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetDestBlend

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetDestBlend( void *param )
{
    RwBlendFunction blendFunc = (RwBlendFunction)param;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetDestBlend" ) );

    /* Check we inside the array bounds and that this is a
     * valid blend function valid for OpenGL
     */
    if (((blendFunc < 1) || (blendFunc > 10)) ||
        (DstBlendModes[blendFunc] == -1))
    {
        RWRETURN(FALSE);
    }

    if (blendFunc != _rwRenderState.destBlend)
    {
        _rwRenderState.destBlend  = blendFunc;
        glBlendFunc(SrcBlendModes[_rwRenderState.srcBlend], DstBlendModes[blendFunc]);

        GL_ERROR_CHECK();
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetZWriteEnable

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetZWriteEnable( void *param )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLSetZWriteEnable" ) );

    if (_rwRenderState.depthWriteOn != (RwBool)param)
    {
        _rwRenderState.depthWriteOn = (RwBool)param;
        glDepthMask((GLboolean)_rwRenderState.depthWriteOn);

        _rwOpenGLIm2DSetVertexFuncs( _rwRenderState.depthTestOn,
                                     _rwRenderState.depthWriteOn,
                                     _rwRenderState.alphaBlendingOn,
                                     RwOpenGLIsEnabled( rwGL_FOG ) );

        GL_ERROR_CHECK();
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetZTestEnable

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetZTestEnable( void *param )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLSetZTestEnable" ) );

    if (_rwRenderState.depthTestOn != (RwBool)param)
    {
        _rwRenderState.depthTestOn = (RwBool) param;

        if (param)
        {
            /* Enable Z buffer test */
            glDepthFunc(GL_LEQUAL);
        }
        else
        {
            /* Disable Z test */
            glDepthFunc(GL_ALWAYS);
        }

        _rwOpenGLIm2DSetVertexFuncs( _rwRenderState.depthTestOn,
                                     _rwRenderState.depthWriteOn,
                                     _rwRenderState.alphaBlendingOn,
                                     RwOpenGLIsEnabled( rwGL_FOG ) );

        GL_ERROR_CHECK();
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetTextureFilter_CubeSupport

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetTextureFilter_CubeSupport( void *param )
{
    RwTextureFilterMode filter = (RwTextureFilterMode)param;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetTextureFilter_CubeSupport" ) );

    if ( NULL != activeTexUnitInfo->texRaster )
    {
        _rwOpenGLRasterExt *rasterExt;

        
        rasterExt = RASTEREXTFROMRASTER(activeTexUnitInfo->texRaster);
        RWASSERT( NULL != rasterExt );

        if ( rasterExt->filterMode != filter )
        {
            static GLenum minFilters[] =
            {
                0, GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_NEAREST,
                GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR
            };
            static GLenum magFilters[] =
            {
                0, GL_NEAREST, GL_LINEAR, GL_NEAREST, GL_LINEAR, GL_NEAREST, GL_LINEAR
            };

            RwRasterFormat format = RwRasterGetFormat(activeTexUnitInfo->texRaster);

            /* Don't set a mipmap filter for a non-mipmaped texture! */
            if (!(format & rwRASTERFORMATMIPMAP))
            {
                if ((filter == rwFILTERMIPNEAREST) || (filter == rwFILTERMIPLINEAR))
                {
                    filter = rwFILTERNEAREST;
                }
                else if ((filter == rwFILTERLINEARMIPNEAREST) || (filter == rwFILTERLINEARMIPLINEAR))
                {
                    filter = rwFILTERLINEAR;
                }
            }

            /* Check that the value of the filter is in the range of the RW enum */
            if ((filter < 1) && (filter > 6))
            {
                RWRETURN(FALSE);
            }

            if ( FALSE == rasterExt->cubeMap )
            {
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilters[filter] );
            }
            else
            {
                glTexParameteri( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, minFilters[filter] );
            }

            if ( magFilters[filter] != magFilters[rasterExt->filterMode] )
            {
                /* It's possible that the magnification filter has not changed */
                if ( FALSE == rasterExt->cubeMap )
                {
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilters[filter] );
                }
                else
                {
                    glTexParameteri( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, magFilters[filter] );
                }
            }

            GL_ERROR_CHECK();

            rasterExt->filterMode = filter;
        }
    }

    activeTexUnitInfo->filterMode = filter;

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetTextureFilter_NoCubeSupport

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetTextureFilter_NoCubeSupport( void *param )
{
    RwTextureFilterMode filter = (RwTextureFilterMode)param;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetTextureFilter_NoCubeSupport" ) );

    RWASSERT( FALSE == _rwOpenGLExt.TextureCubeMapARB );

    if ( NULL != activeTexUnitInfo->texRaster )
    {
        _rwOpenGLRasterExt *rasterExt;
        

        rasterExt = RASTEREXTFROMRASTER(activeTexUnitInfo->texRaster);
        RWASSERT( NULL != rasterExt );
        RWASSERT( FALSE == rasterExt->cubeMap );

        if ( rasterExt->filterMode != filter )
        {
            static GLenum minFilters[] =
            {
                0, GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_NEAREST,
                GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR
            };
            static GLenum magFilters[] =
            {
                0, GL_NEAREST, GL_LINEAR, GL_NEAREST, GL_LINEAR, GL_NEAREST, GL_LINEAR
            };

            RwRasterFormat format = RwRasterGetFormat(activeTexUnitInfo->texRaster);

            /* Don't set a mipmap filter for a non-mipmaped texture! */
            if (!(format & rwRASTERFORMATMIPMAP))
            {
                if ((filter == rwFILTERMIPNEAREST) || (filter == rwFILTERMIPLINEAR))
                {
                    filter = rwFILTERNEAREST;
                }
                else if ((filter == rwFILTERLINEARMIPNEAREST) || (filter == rwFILTERLINEARMIPLINEAR))
                {
                    filter = rwFILTERLINEAR;
                }
            }

            /* Check that the value of the filter is in the range of the RW enum */
            if ((filter < 1) && (filter > 6))
            {
                RWRETURN(FALSE);
            }

            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilters[filter] );

            if ( magFilters[filter] != magFilters[rasterExt->filterMode] )
            {
                /* It's possible that the magnification filter has not changed */
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilters[filter] );
            }

            GL_ERROR_CHECK();

            rasterExt->filterMode = filter;
        }
    }

    activeTexUnitInfo->filterMode = filter;

    RWRETURN( TRUE );
}



/****************************************************************************
 _rwOpenGLSetStencilEnable

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetStencilEnable( void *param )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLSetStencilEnable" ) );

    if ( FALSE != (RwBool)param )
    {
        RwUInt8     stencilDepth;


        /* Do we need to check return (in case engine not started?) */
        _rwOpenGLGetStencilBufferDepth( &stencilDepth );
        if ( 0 != stencilDepth )
        {
            RwOpenGLEnable( rwGL_STENCIL_TEST );
            RWRETURN(TRUE);
        }
        else
        {
            /* no stencil buffer present! */
            RWRETURN(FALSE);
        }
    }
    else
    {
        RwOpenGLDisable( rwGL_STENCIL_TEST );
        RWRETURN(TRUE);
    }

    GL_ERROR_CHECK();

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetStencilFail

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetStencilFail( void *param )
{
    RwStencilOperation  operation;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetStencilFail" ) );

    operation = (RwStencilOperation)param;

    if ( (operation < 1) || (operation > 6) ||
         (FALSE == RwOpenGLIsEnabled( rwGL_STENCIL_TEST )) )
    {
        RWRETURN(FALSE);
    }

    if (operation != _rwRenderState.stencilFail)
    {
        _rwRenderState.stencilFail = operation;

        glStencilOp( StencilOperations[_rwRenderState.stencilFail],
                     StencilOperations[_rwRenderState.stencilZFail],
                     StencilOperations[_rwRenderState.stencilZPass] );

        GL_ERROR_CHECK();
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetStencilZFail

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetStencilZFail( void *param )
{
    RwStencilOperation  operation;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetStencilZFail" ) );

    operation = (RwStencilOperation)param;

    if ( (operation < 1) || (operation > 6) ||
         (FALSE == RwOpenGLIsEnabled( rwGL_STENCIL_TEST )) )
    {
        RWRETURN(FALSE);
    }

    if (operation != _rwRenderState.stencilZFail)
    {
        _rwRenderState.stencilZFail = operation;

        glStencilOp( StencilOperations[_rwRenderState.stencilFail],
                     StencilOperations[_rwRenderState.stencilZFail],
                     StencilOperations[_rwRenderState.stencilZPass] );

        GL_ERROR_CHECK();
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetStencilPass

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetStencilPass( void *param )
{
    RwStencilOperation  operation;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetStencilPass" ) );

    operation = (RwStencilOperation)param;

    if ( (operation < 1) || (operation > 6) ||
         (FALSE == RwOpenGLIsEnabled( rwGL_STENCIL_TEST )) )
    {
        RWRETURN(FALSE);
    }

    if (operation != _rwRenderState.stencilZPass)
    {
        _rwRenderState.stencilZPass = operation;

        glStencilOp( StencilOperations[_rwRenderState.stencilFail],
                     StencilOperations[_rwRenderState.stencilZFail],
                     StencilOperations[_rwRenderState.stencilZPass] );

        GL_ERROR_CHECK();
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetStencilFunction

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetStencilFunction( void *param )
{
    RwStencilFunction   function;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetStencilFunction" ) );

    function = (RwStencilFunction)param;

    if ( (function < 1) || (function > 8) ||
         (FALSE == RwOpenGLIsEnabled( rwGL_STENCIL_TEST )) )
    {
        RWRETURN(FALSE);
    }

    if (function != _rwRenderState.stencilFunction)
    {
        _rwRenderState.stencilFunction = function;

        glStencilFunc( StencilFunctions[_rwRenderState.stencilFunction],
                       (GLint)_rwRenderState.stencilReference,
                       (GLuint)_rwRenderState.stencilMask );

        GL_ERROR_CHECK();
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetStencilFunctionRef

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetStencilFunctionRef( void *param )
{
    RwInt32 reference;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetStencilFunctionRef" ) );

    if ( FALSE == RwOpenGLIsEnabled( rwGL_STENCIL_TEST ) )
    {
        RWRETURN( FALSE );
    }

    reference = (RwInt32)param;

    if (reference != _rwRenderState.stencilReference)
    {
        _rwRenderState.stencilReference = reference;

        glStencilFunc( StencilFunctions[_rwRenderState.stencilFunction],
                       (GLint)_rwRenderState.stencilReference,
                       (GLuint)_rwRenderState.stencilMask );

        GL_ERROR_CHECK();
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetStencilFunctionMask

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetStencilFunctionMask( void *param )
{
    RwUInt32    mask;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetStencilFunctionMask" ) );

    if ( FALSE == RwOpenGLIsEnabled( rwGL_STENCIL_TEST ) )
    {
        RWRETURN( FALSE );
    }

    mask = (RwUInt32)param;

    if (mask != _rwRenderState.stencilMask)
    {
        _rwRenderState.stencilMask = mask;

        glStencilFunc( StencilFunctions[_rwRenderState.stencilFunction],
                       (GLint)_rwRenderState.stencilReference,
                       (GLuint)_rwRenderState.stencilMask );

        GL_ERROR_CHECK();
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetStencilFunctionWriteMask

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetStencilFunctionWriteMask( void *param )
{
    RwUInt32    writeMask;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetStencilFunctionWriteMask" ) );

    if ( FALSE == RwOpenGLIsEnabled( rwGL_STENCIL_TEST ) )
    {
        RWRETURN( FALSE );
    }

    writeMask = (RwUInt32)param;

    if ( writeMask != _rwRenderState.stencilWriteMask )
    {
        _rwRenderState.stencilWriteMask = writeMask;

        glStencilMask( (GLuint)(_rwRenderState.stencilWriteMask) );

        GL_ERROR_CHECK();
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetCullMode

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetCullMode( void *param )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLSetCullMode" ) );

    switch ((RwCullMode)param)
    {
    case rwCULLMODECULLNONE:
        {
            if ( FALSE != _rwRenderState.cullEnabled )
            {
                /* No culling */
                _rwRenderState.cullEnabled = FALSE;
                RwOpenGLDisable( rwGL_CULL_FACE );
            }
        }
        break;

    case rwCULLMODECULLBACK:
        {
            if ( FALSE == _rwRenderState.cullEnabled )
            {
                _rwRenderState.cullEnabled = TRUE;
                RwOpenGLEnable( rwGL_CULL_FACE );
            }
            if ( rwCULLMODECULLBACK != _rwRenderState.cullMode )
            {
                /* Back face culling */
                _rwRenderState.cullMode = rwCULLMODECULLBACK;
                glCullFace( GL_BACK );
            }
        }
        break;

    case rwCULLMODECULLFRONT:
        {
            if ( FALSE == _rwRenderState.cullEnabled )
            {
                _rwRenderState.cullEnabled = TRUE;
                RwOpenGLEnable( rwGL_CULL_FACE );
            }
            if ( rwCULLMODECULLFRONT != _rwRenderState.cullMode )
            {
                /* Back face culling */
                _rwRenderState.cullMode = rwCULLMODECULLFRONT;
                glCullFace( GL_FRONT );
            }
        }
        break;

    default:
        /* What is this mode you naughty programmer?? */
        RWRETURN( FALSE );
    }

    GL_ERROR_CHECK();

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetAlphaTestFunction

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetAlphaTestFunction( void *param )
{
    RwAlphaTestFunction testFunc = (RwAlphaTestFunction)param;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetAlphaTestFunction" ) );

    if ( testFunc != _rwRenderState.alphaTest )
    {
        if ( rwALPHATESTFUNCTIONALWAYS != testFunc )
        {
            RwOpenGLEnable( rwGL_ALPHA_TEST );
        }
        else
        {
            RwOpenGLDisable( rwGL_ALPHA_TEST );
        }

        glAlphaFunc( AlphaTestFunctions[testFunc],
                     _rwRenderState.alphaRefCache );

        _rwRenderState.alphaTest = testFunc;

        GL_ERROR_CHECK();
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetAlphaTestFunctionRef

 On entry   : 
            : 
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLSetAlphaTestFunctionRef( void *param )
{
    RwInt32 ref = (RwInt32)param;


    RWFUNCTION( RWSTRING( "_rwOpenGLSetAlphaTestFunctionRef" ) );
 
    if ( ref != _rwRenderState.alphaRef )
    {
        _rwRenderState.alphaRef = ref;
        _rwRenderState.alphaRefCache = ref * (1.0f / 255.0f);

        glAlphaFunc( AlphaTestFunctions[_rwRenderState.alphaTest],
                     _rwRenderState.alphaRefCache );

        GL_ERROR_CHECK();
    }

    RWRETURN( TRUE );
}


/****************************************************************************
 _rwOpenGLSetRenderState

 On entry   : State to set
            : Parameter
 On exit    : TRUE on success
 */

RwBool
_rwOpenGLSetRenderState(RwRenderState state, void *param)
{
    RWFUNCTION(RWSTRING("_rwOpenGLSetRenderState"));

    if ( NULL != renderStateSet[state] )
    {
        RWRETURN( renderStateSet[state](param) );
    }
    else
    {
        RWRETURN( FALSE );
    }
}


/****************************************************************************
 _rwOpenGLGetRenderState

 On entry   : State to set
            : Parameter (OUT)
 On exit    : TRUE on success
 */

RwBool
_rwOpenGLGetRenderState(RwRenderState state, void *param)
{
    RWFUNCTION(RWSTRING("_rwOpenGLGetRenderState"));

    switch (state)
    {
        /********************************* Fog ******************************/
        case rwRENDERSTATEFOGENABLE:
        {
            *(RwInt32 *)param = RwOpenGLIsEnabled( rwGL_FOG );
            RWRETURN(TRUE);
        }

        case rwRENDERSTATEFOGCOLOR:
        {
            *(RwUInt32 *)param = _rwRenderState.fogColor;
            RWRETURN(TRUE);
        }

        case rwRENDERSTATEFOGTYPE:
        {
            *(RwUInt32 *)param = _rwRenderState.fogType;
            RWRETURN(TRUE);
        }

        /****************** Texture addressing ******************************/

        case rwRENDERSTATETEXTUREADDRESSU:
        {
            *(RwInt32 *)param = activeTexUnitInfo->addressModeU;
            RWRETURN(TRUE);
        }

        case rwRENDERSTATETEXTUREADDRESSV:
        {
            *(RwInt32 *)param = activeTexUnitInfo->addressModeV;
            RWRETURN(TRUE);
        }

        case rwRENDERSTATETEXTUREADDRESS:
        {
            if ( activeTexUnitInfo->addressModeU ==
                 activeTexUnitInfo->addressModeV )
            {
                *(RwInt32 *)param = activeTexUnitInfo->addressModeU;
            }
            else
            {
                *(RwInt32 *)param = rwTEXTUREADDRESSNATEXTUREADDRESS;
            }

            RWRETURN(TRUE);
        }

        /****************** Texturing ***************************************/

        case rwRENDERSTATETEXTURERASTER:
        {
            /* Give the state */
            *(RwRaster **)param = activeTexUnitInfo->texRaster;
            RWRETURN(TRUE);
        }

        /****************** Shading mode ***********************************/

        case rwRENDERSTATESHADEMODE:             /* SHADEMODE */
        {
            *(RwInt32 *)param = _rwRenderState.shadeMode;
            RWRETURN(TRUE);
        }

        /******************* Blend mode ************************************/

        case rwRENDERSTATEVERTEXALPHAENABLE:      /* TRUE to enable alpha blending */
        {
            *(RwBool *)param = _rwRenderState.alphaBlendingOn;
            RWRETURN(TRUE);
        }

        /**************** Blend mode/color key *****************************/

        case rwRENDERSTATESRCBLEND:              /* D3DBLEND */
        {
            /* We know the blend function is valid - it's impossible to set a duff one */
            *(RwUInt32 *)param = _rwRenderState.srcBlend;
            RWRETURN(TRUE);
        }
        case rwRENDERSTATEDESTBLEND:
        {
            /* We know the blend function is valid - it's impossible to set a duff one */
            *(RwUInt32 *)param = _rwRenderState.destBlend;
            RWRETURN(TRUE);
        }


        /******************* Z Buffer writes *******************************/

        case rwRENDERSTATEZWRITEENABLE:          /* TRUE to enable z writes */
        {
            *(RwBool *)param = _rwRenderState.depthWriteOn;
            RWRETURN(TRUE);
        }

        /******************** Z buffer test ********************************/

        case rwRENDERSTATEZTESTENABLE:               /* TRUE to enable z test */
        {
            *(RwBool *)param = _rwRenderState.depthTestOn;
            RWRETURN(TRUE);
        }

        /****************** Filter mode ************************************/

        case rwRENDERSTATETEXTUREFILTER:            /* TEXTUREFILTER */
        {
            *(RwUInt32 *)param = activeTexUnitInfo->filterMode;
            RWRETURN(TRUE);
        }

        /******************** Always enabled ******************************/

        case rwRENDERSTATETEXTUREPERSPECTIVE:    /* TRUE for perspective correction */
        {
            *(RwUInt32 *)param = TRUE;
            RWRETURN(TRUE);
        }

        /******************** Not available on this hardware **************/

        case rwRENDERSTATEBORDERCOLOR:           /* Border color for texturing w/border */
        {
            RWRETURN(FALSE);
        }

        /******************** Stencil buffer stuff ************************/

        case rwRENDERSTATESTENCILENABLE:
        {
            *(RwBool *)param = RwOpenGLIsEnabled( rwGL_STENCIL_TEST );
            RWRETURN(TRUE);
        }
        break;

        case rwRENDERSTATESTENCILFAIL:
        {
            *(RwStencilOperation *)param = _rwRenderState.stencilFail;
            RWRETURN(TRUE);
        }
        break;

        case rwRENDERSTATESTENCILZFAIL:
        {
            *(RwStencilOperation *)param = _rwRenderState.stencilZFail;
            RWRETURN(TRUE);
        }
        break;

        case rwRENDERSTATESTENCILPASS:
        {
            *(RwStencilOperation *)param = _rwRenderState.stencilZPass;
            RWRETURN(TRUE);
        }
        break;

        case rwRENDERSTATESTENCILFUNCTION:
        {
            *(RwStencilFunction *)param = _rwRenderState.stencilFunction;
            RWRETURN(TRUE);
        }
        break;

        case rwRENDERSTATESTENCILFUNCTIONREF:
        {
            *(RwInt32 *)param = _rwRenderState.stencilReference;
            RWRETURN(TRUE);
        }
        break;

        case rwRENDERSTATESTENCILFUNCTIONMASK:
        {
            *(RwUInt32 *)param = _rwRenderState.stencilMask;
            RWRETURN(TRUE);
        }
        break;

        case rwRENDERSTATESTENCILFUNCTIONWRITEMASK:
        {
            *(RwUInt32 *)param = _rwRenderState.stencilWriteMask;
            RWRETURN(TRUE);
        }
        break;

        /****************** Cull mode ***********************************/

        case rwRENDERSTATECULLMODE:             /* CULLMODE */
        {
            if ( FALSE == _rwRenderState.cullEnabled )
            {
                *(RwInt32 *)param = rwCULLMODECULLNONE;
            }
            else
            {
                *(RwInt32 *)param = _rwRenderState.cullMode;
            }
            RWRETURN(TRUE);
        }

        /****************** Alpha test ***********************************/

        case rwRENDERSTATEALPHATESTFUNCTION:
            {
                *(RwAlphaTestFunction *)param = _rwRenderState.alphaTest;
                RWRETURN( TRUE );
            }
            break;


        case rwRENDERSTATEALPHATESTFUNCTIONREF:
            {
                *(RwInt32 *)param = (RwInt32)(_rwRenderState.alphaRef);
                RWRETURN( TRUE );
            }
            break;

        /****************** Unsupported ***********************************/

        default:
        {
            break;
        }
    }

    RWRETURN(FALSE);
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                      Setup the initial rendering state

 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/****************************************************************************
 _rwOpenGLRenderStateOpen

 On entry   :
 On exit    :
 */

void
_rwOpenGLRenderStateOpen(void)
{
    RwUInt8 idx;


    RWFUNCTION(RWSTRING("_rwOpenGLRenderStateOpen"));

    /* set up render state function pointers according to available functionality */
    if ( FALSE == _rwOpenGLExt.TextureCubeMapARB )
    {
#if defined(RWDEBUG)
        RWMESSAGE( (RWSTRING("Info: Cube map support not available")) );
#endif /* defined(RWDEBUG) */

        _rwOpenGLSetTextureFilter   = _rwOpenGLSetTextureFilter_NoCubeSupport;

        if ( FALSE == _rwOpenGLExt.TextureEdgeClampEXT )
        {
#if defined(RWDEBUG)
            RWMESSAGE( (RWSTRING("Info: GL_CLAMP_TO_EDGE_EXT support not available")) );
#endif /* defined(RWDEBUG) */

            _rwOpenGLSetTextureAddressU = _rwOpenGLSetTextureAddressU_NoCubeSupport_NoEdgeClamp;
            _rwOpenGLSetTextureAddressV = _rwOpenGLSetTextureAddressV_NoCubeSupport_NoEdgeClamp;
        }
        else
        {
            _rwOpenGLSetTextureAddressU = _rwOpenGLSetTextureAddressU_NoCubeSupport_EdgeClamp;
            _rwOpenGLSetTextureAddressV = _rwOpenGLSetTextureAddressV_NoCubeSupport_EdgeClamp;
        }
    }
    else
    {
        _rwOpenGLSetTextureFilter   = _rwOpenGLSetTextureFilter_CubeSupport;

        if ( FALSE == _rwOpenGLExt.TextureEdgeClampEXT )
        {
#if defined(RWDEBUG)
            RWMESSAGE( (RWSTRING("Info: GL_CLAMP_TO_EDGE_EXT support not available")) );
#endif /* defined(RWDEBUG) */

            _rwOpenGLSetTextureAddressU = _rwOpenGLSetTextureAddressU_CubeSupport_NoEdgeClamp;
            _rwOpenGLSetTextureAddressV = _rwOpenGLSetTextureAddressV_CubeSupport_NoEdgeClamp;
        }
        else
        {
            _rwOpenGLSetTextureAddressU = _rwOpenGLSetTextureAddressU_CubeSupport_EdgeClamp;
            _rwOpenGLSetTextureAddressV = _rwOpenGLSetTextureAddressV_CubeSupport_EdgeClamp;
        }
    }

    renderStateSet[rwRENDERSTATEFOGENABLE]                = _rwOpenGLSetFogEnable;
    renderStateSet[rwRENDERSTATEFOGCOLOR]                 = _rwOpenGLSetFogColor;
    renderStateSet[rwRENDERSTATEFOGTYPE]                  = _rwOpenGLSetFogType;
    renderStateSet[rwRENDERSTATEFOGDENSITY]               = _rwOpenGLSetFogDensity;
    renderStateSet[rwRENDERSTATETEXTUREADDRESSU]          = _rwOpenGLSetTextureAddressU;
    renderStateSet[rwRENDERSTATETEXTUREADDRESSV]          = _rwOpenGLSetTextureAddressV;
    renderStateSet[rwRENDERSTATETEXTUREADDRESS]           = _rwOpenGLSetTextureAddress;
    renderStateSet[rwRENDERSTATETEXTURERASTER]            = _rwOpenGLSetRasterForceTextureParams;
    renderStateSet[rwRENDERSTATESHADEMODE]                = _rwOpenGLSetShadeMode;
    renderStateSet[rwRENDERSTATEVERTEXALPHAENABLE]        = _rwOpenGLSetVertexAlphaEnable;
    renderStateSet[rwRENDERSTATESRCBLEND]                 = _rwOpenGLSetSrcBlend;
    renderStateSet[rwRENDERSTATEDESTBLEND]                = _rwOpenGLSetDestBlend;
    renderStateSet[rwRENDERSTATEZWRITEENABLE]             = _rwOpenGLSetZWriteEnable;
    renderStateSet[rwRENDERSTATEZTESTENABLE]              = _rwOpenGLSetZTestEnable;
    renderStateSet[rwRENDERSTATETEXTUREFILTER]            = _rwOpenGLSetTextureFilter;
    renderStateSet[rwRENDERSTATESTENCILENABLE]            = _rwOpenGLSetStencilEnable;
    renderStateSet[rwRENDERSTATESTENCILFAIL]              = _rwOpenGLSetStencilFail;
    renderStateSet[rwRENDERSTATESTENCILZFAIL]             = _rwOpenGLSetStencilZFail;
    renderStateSet[rwRENDERSTATESTENCILPASS]              = _rwOpenGLSetStencilPass;
    renderStateSet[rwRENDERSTATESTENCILFUNCTION]          = _rwOpenGLSetStencilFunction;
    renderStateSet[rwRENDERSTATESTENCILFUNCTIONREF]       = _rwOpenGLSetStencilFunctionRef;
    renderStateSet[rwRENDERSTATESTENCILFUNCTIONMASK]      = _rwOpenGLSetStencilFunctionMask;
    renderStateSet[rwRENDERSTATESTENCILFUNCTIONWRITEMASK] = _rwOpenGLSetStencilFunctionWriteMask;
    renderStateSet[rwRENDERSTATECULLMODE]                 = _rwOpenGLSetCullMode;
    renderStateSet[rwRENDERSTATEALPHATESTFUNCTION]        = _rwOpenGLSetAlphaTestFunction;
    renderStateSet[rwRENDERSTATEALPHATESTFUNCTIONREF]     = _rwOpenGLSetAlphaTestFunctionRef;

    /* can we support blend square blending functions? */
    if ( FALSE != _rwOpenGLExt.BlendSquareNV )
    {
        SrcBlendModes[rwBLENDSRCCOLOR]     = GL_SRC_COLOR;
        SrcBlendModes[rwBLENDINVSRCCOLOR]  = GL_ONE_MINUS_SRC_COLOR;

        DstBlendModes[rwBLENDDESTCOLOR]    = GL_DST_COLOR;
        DstBlendModes[rwBLENDINVDESTCOLOR] = GL_ONE_MINUS_DST_COLOR;
    }
#if defined(RWDEBUG)
    else
    {
        RWMESSAGE( (RWSTRING("Info: GL_NV_BLEND_SQUARE support not available.")) );
    }
#endif /* defined(RWDEBUG) */

    /* query the server for initial state */
    for ( idx = 1; idx < (RwUInt32)rwGL_NUMSTATES; idx += 1 )
    {
        RwOpenGLStateCache[idx].glToken = RwOpenGLStateNameConvert[idx];

        /* check for special cases requiring extensions */
        if ( GL_TEXTURE_CUBE_MAP_ARB == RwOpenGLStateCache[idx].glToken )
        {
            if ( FALSE != _rwOpenGLExt.TextureCubeMapARB )
            {
                RwOpenGLStateCache[idx].enabled = glIsEnabled( RwOpenGLStateCache[idx].glToken );
            }
            else
            {
                RwOpenGLStateCache[idx].enabled = GL_FALSE;
            }
        }
        else
        {
            RwOpenGLStateCache[idx].enabled = glIsEnabled( RwOpenGLStateCache[idx].glToken );
        }
    }

    /* First of all, set everything in the render state structure to something sensible */
    _rwRenderState.depthWriteOn     = TRUE;
    _rwRenderState.depthTestOn      = TRUE;
    _rwRenderState.srcBlend         = rwBLENDSRCALPHA;
    _rwRenderState.destBlend        = rwBLENDINVSRCALPHA;
    _rwRenderState.alphaBlendingOn  = FALSE;

    RWASSERT( _rwOpenGLExt.MaxTextureUnits > 0 );

    for ( idx = 0; idx < rwMAXTEXTURECOORDS; idx += 1 )
    {
        _rwRenderState.texUnitInfo[idx].texRaster     = (RwRaster *)NULL;
        _rwRenderState.texUnitInfo[idx].addressModeU  = rwTEXTUREADDRESSWRAP;
        _rwRenderState.texUnitInfo[idx].addressModeV  = rwTEXTUREADDRESSWRAP;
        _rwRenderState.texUnitInfo[idx].filterMode    = rwFILTERLINEAR;
        _rwRenderState.texUnitInfo[idx].maxAnisotropy = 1; /* isotropic */
    }

    /* first texture unit is used initially */
    _rwRenderState.activeTexUnit = 0;
    activeTexUnitInfo = _rwRenderState.texUnitInfo;

    _rwRenderState.lastTextureHadAlpha = FALSE;

    _rwRenderState.shadeMode        = rwSHADEMODEGOURAUD;

    _rwRenderState.stencilFunction  = rwSTENCILFUNCTIONALWAYS;
    _rwRenderState.stencilReference = 0;
    _rwRenderState.stencilMask      = 0xFFFFFFFF; /* should be 2^stencilbits - 1 */
    _rwRenderState.stencilWriteMask = 0xFFFFFFFF;
    _rwRenderState.stencilFail      = rwSTENCILOPERATIONKEEP;
    _rwRenderState.stencilZFail     = rwSTENCILOPERATIONKEEP;
    _rwRenderState.stencilZPass     = rwSTENCILOPERATIONKEEP;
    _rwRenderState.stencilClearVal  = 0;

    /* Setting these values negative should ensure an update on first camBeginUpdate */
    _rwRenderState.fogStart   = -1.0f;
    _rwRenderState.fogEnd     = -1.0f;
    _rwRenderState.fogColor   = 0;
    glFogiv( GL_FOG_COLOR, (GLint *)&(_rwRenderState.fogColor) );
    _rwRenderState.fogType    = rwFOGTYPELINEAR;
    glFogi( GL_FOG_MODE, GL_LINEAR );
    _rwRenderState.fogDensity = 1.0f;
    glFogf( GL_FOG_DENSITY, _rwRenderState.fogDensity );

    /* Set a default clear color of black */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    /* Z buffering */
    RwOpenGLEnable( rwGL_DEPTH_TEST );
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);

    /* Write RGBA */
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    /* Set up blending */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    RwOpenGLDisable( rwGL_BLEND );

    /* Set up chroma-keying */
    _rwRenderState.alphaTest = rwALPHATESTFUNCTIONGREATER;
    _rwRenderState.alphaRef  = 0;
    _rwRenderState.alphaRefCache = 0.0f;
    glAlphaFunc( AlphaTestFunctions[_rwRenderState.alphaTest],
                 _rwRenderState.alphaRefCache );
    RwOpenGLDisable( rwGL_ALPHA_TEST );

    /* Initialise OpenGL lighting state */
    {
        RwInt32 i;
        GLfloat zerocol[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

        glLightModelfv( GL_LIGHT_MODEL_AMBIENT, zerocol );

        for ( i = 0; i < 8; i += 1 )
        {
            glLightfv( GL_LIGHT0 + i, GL_AMBIENT, zerocol );
            glLightfv( GL_LIGHT0 + i, GL_DIFFUSE, zerocol );
            glLightfv( GL_LIGHT0 + i, GL_SPECULAR, zerocol );
        }

        glMaterialfv( GL_FRONT, GL_AMBIENT, zerocol );
        glMaterialfv( GL_FRONT, GL_DIFFUSE, zerocol );
        glMaterialfv( GL_FRONT, GL_SPECULAR, zerocol );
    }

    /* Counter-clockwise winding as RenderWare specifies for front facing polys */
    glFrontFace(GL_CCW);

    _rwRenderState.cullEnabled = TRUE;
    RwOpenGLEnable( rwGL_CULL_FACE );
    _rwRenderState.cullMode = rwCULLMODECULLBACK;
    glCullFace(GL_BACK);

    /* Set any hints to influence render quality */
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    /* Set up the projection information */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    GL_ERROR_CHECK();

    RWRETURNVOID();
}


/**
 * \ingroup rwrenderstateopengl
 * \ref RwOpenGLSetStencilClear sets the value to store in each
 * stencil-buffer entry when it is cleared.
 *
 * \ref rwCAMERACLEARSTENCIL can be OR'd into the camera clear flags to
 * clear the stencil buffer.
 *
 * \param stencilValue \ref RwUInt32 in the range from 0 through
 * (2 to the power \e n) - 1, where \e n is the bit depth of the stencil buffer.
 *
 * \see RwOpenGLGetStencilClear
 * \see RwCameraClear
 * \see RwCameraClearMode
 */
void
RwOpenGLSetStencilClear( RwUInt32 stencilValue )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLSetStencilClear" ) );

    if ( stencilValue != _rwRenderState.stencilClearVal )
    {
        _rwRenderState.stencilClearVal = stencilValue;

        glClearStencil( (GLint)(_rwRenderState.stencilClearVal) );
    }

    RWRETURNVOID();
}


/**
 * \ingroup rwrenderstateopengl
 * \ref RwOpenGLGetStencilClear returns the current value that will be stored in
 * each stencil-buffer entry when it is cleared.
 *
 * \returns \ref RwUInt32 containing the stencil clear value, in the range 0
 * through (2 to the power \e n) - 1, where \e n is the bit depth of the
 * stencil buffer.
 *
 * \see RwOpenGLSetStencilClear
 * \see RwCameraClear
 */
RwUInt32
RwOpenGLGetStencilClear( void )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLGetStencilClear" ) );

    RWRETURN( _rwRenderState.stencilClearVal );
}



#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))

/**
 * \ingroup rwrenderstateopengl
 * \ref RwOpenGLEnable is the RenderWare Graphics' caching function corresponding
 * to OpenGL's \c glEnable function. The argument \c token can be one of the values
 * defined by the enumeration \ref RwOpenGLStateToken.
 *
 * The current state of the token is stored internally to avoid state changes.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param token Constant token from the \ref RwOpenGLStateToken enumeration to
 *              enable the equivalent OpenGL state.
 *
 * \see \ref RwOpenGLDisable
 * \see \ref RwOpenGLIsEnabled
 */
void
RwOpenGLEnable( const RwOpenGLStateToken token )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLEnable" ) );

    RWASSERT( token > rwGL_STATENA );
    RWASSERT( token < rwGL_NUMSTATES );

    RwOpenGLEnableMacro(token);

    RWRETURNVOID();
}


/**
 * \ingroup rwrenderstateopengl
 * \ref RwOpenGLDisable is the RenderWare Graphics' caching function corresponding
 * to OpenGL's \c glDisable function. The argument \c token can be one of the values
 * defined by the enumeration \ref RwOpenGLStateToken.
 *
 * The current state of the token is stored internally to avoid state changes.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param token Constant token from the \ref RwOpenGLStateToken enumeration to
 *              disable the equivalent OpenGL state.
 *
 * \see \ref RwOpenGLEnable
 * \see \ref RwOpenGLIsEnabled
 */
void
RwOpenGLDisable( const RwOpenGLStateToken token )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLDisable" ) );

    RWASSERT( token > rwGL_STATENA );
    RWASSERT( token < rwGL_NUMSTATES );

    RwOpenGLDisableMacro(token);

    RWRETURNVOID();
}


/**
 * \ingroup rwrenderstateopengl
 * \ref RwOpenGLIsEnabled is the RenderWare Graphics' caching function corresponding
 * to OpenGL's \c glIsEnabled function. The argument \c token can be one of the values
 * defined by the enumeration \ref RwOpenGLStateToken.
 *
 * The current state of the token is stored internally to avoid state changes.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param token Constant token from the \ref RwOpenGLStateToken enumeration to
 *              disable the equivalent OpenGL state.
 *
 * \return \ref RwBool containing the current state of the requested token.
 *
 * \see \ref RwOpenGLEnable
 * \see \ref RwOpenGLDisable
 */
RwBool
RwOpenGLIsEnabled( const RwOpenGLStateToken token )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLDisable" ) );

    RWRETURN( RwOpenGLIsEnabledMacro(token) );
}
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */
