/***************************************************************************
 *                                                                         *
 * Module  : basprigl.c                                                    *
 *                                                                         *
 * Purpose : OpenGL sprite handling                                        *
 *                                                                         *
 **************************************************************************/

/****************************************************************************
 Includes
 */

#if defined(WIN32)
#include <windows.h>
#endif /* defined(WIN32) */
#include <GL/gl.h>

#include "barwtyp.h"
#include "baimage.h"
#include "batextur.h"

#include "baogl.h"
#include "bastdogl.h"
#include "drvfns.h"

#include "barastgl.h"
#include "barstate.h"



/****************************************************************************
 Global prototypes
 */

/****************************************************************************
 Local Types
 */

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */
#define COLORSCALAR 0.003921568627450980392156862745098f /* 1.0f / 255.0f */


 /****************************************************************************
 Local (static) Globals
 */

/* Current destination raster */
static RwRaster *_rwOpenGLContextRaster = NULL;


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLSetRasterContext
 *
 *  Purpose : Set the current context raster.
 *
 *  On entry: inOut - Void pointer
 *
 *            raster - Pointer to RwRaster that is the context raster
 *
 *            flags - RwInt32
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
RwBool 
_rwOpenGLSetRasterContext( void *inOut __RWUNUSED__,
                           RwRaster *raster,
                           RwInt32 flags __RWUNUSED__ )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLSetRasterContext" ) );

    _rwOpenGLContextRaster = raster;

    RWRETURN( TRUE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRasterClear
 *
 *  Purpose : Clear the current context raster to the pixel color.
 *
 *  On entry: inOut - Void pointer
 *
 *            in - Void pointer
 *
 *            pixelColor - RwInt32 containing the packed pixel color.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
RwBool
_rwOpenGLRasterClear( void *inOut __RWUNUSED__,
                      void *in __RWUNUSED__,
                      RwInt32 pixelColor )
{
    RwRect  rect;


    RWFUNCTION( RWSTRING( "_rwOpenGLRasterClear" ) );

    RWASSERT( NULL != _rwOpenGLContextRaster );

    rect.x = 0;
    rect.y = 0;
    rect.w = RwRasterGetWidth( _rwOpenGLContextRaster );
    rect.h = RwRasterGetHeight( _rwOpenGLContextRaster );

    RWRETURN( _rwOpenGLRasterClearRect( NULL, &rect, pixelColor ) );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRasterClearRect
 *
 *  Purpose : Clear a region of the current context raster to the pixel color.
 *
 *  On entry: inOut - Void pointer
 *
 *            rect - Pointer to RwRect containing the area definition to clear.
 *
 *            pixelColor - RwInt32 containing the packed pixel color.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
RwBool 
_rwOpenGLRasterClearRect( void *inOut __RWUNUSED__,
                          RwRect *rect,
                          RwInt32 pixelColor )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLRasterClearRect" ) );

    RWASSERT( NULL != _rwOpenGLContextRaster );
    RWASSERT( NULL != rect );

    switch ( _rwOpenGLContextRaster->cType )
    {
    case rwRASTERTYPENORMAL:
        /* fall thru */
    case rwRASTERTYPETEXTURE:
        /* fall thru */
    case rwRASTERTYPECAMERATEXTURE:
        {
            RwUInt8             *pixelData;

            _rwOpenGLRasterExt  *rasterExt;

            RwInt32             y;

            RwInt32             x;


            pixelData = RwRasterLock( _rwOpenGLContextRaster, 0, rwRASTERLOCKWRITE );
            if ( NULL == pixelData )
            {
                RWRETURN( FALSE );
            }

            rasterExt = RASTEREXTFROMRASTER( _rwOpenGLContextRaster );
            RWASSERT( NULL != rasterExt );

            switch ( _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(_rwOpenGLContextRaster->cFormat)].clientPixelFormat )
            {
            case GL_RGB:
                {
                    RWASSERT( 3 == _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(_rwOpenGLContextRaster->cFormat)].clientBytesPerPixel );

                    /* apply the appropriate offset for the rectangle top left corner */
                    pixelData += _rwOpenGLContextRaster->stride * rect->y + 3 * rect->x;

                    /* alpha is the most significant byte
                       red is the least significant byte */
                    for ( y = 0; y < rect->h; y += 1 )
                    {
                        for ( x = 0; x < rect->w; x += 1 )
                        {
                            *(pixelData + x * 3 + 0) = (pixelColor)       & 0xFF;   /* red   */
                            *(pixelData + x * 3 + 1) = (pixelColor >> 8)  & 0xFF;   /* green */
                            *(pixelData + x * 3 + 2) = (pixelColor >> 16) & 0xFF;   /* blue  */
                        }

                        pixelData += _rwOpenGLContextRaster->stride;
                    }
                }
                break;

            case GL_RGBA:
                {
                    RWASSERT( 4 == _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(_rwOpenGLContextRaster->cFormat)].clientBytesPerPixel );

                    /* apply the appropriate offset for the rectangle top left corner */
                    pixelData += _rwOpenGLContextRaster->stride * rect->y + 4 * rect->x;

                    /* alpha is the most significant byte
                       red is the least significant byte */
                    for ( y = 0; y < rect->h; y += 1 )
                    {
                        for ( x = 0; x < rect->w; x += 1 )
                        {
                            *(pixelData + x * 4 + 0) = (pixelColor)       & 0xFF;   /* red   */
                            *(pixelData + x * 4 + 1) = (pixelColor >> 8)  & 0xFF;   /* green */
                            *(pixelData + x * 4 + 2) = (pixelColor >> 16) & 0xFF;   /* blue  */
                            *(pixelData + x * 4 + 3) = (pixelColor >> 24) & 0xFF;   /* alpha */
                        }

                        pixelData += _rwOpenGLContextRaster->stride;
                    }
                }
                break;

            default:
                RWERROR( (E_RW_INVRASTERFORMAT) );
                RWRETURN( FALSE );
            }

            RwRasterUnlock( _rwOpenGLContextRaster );
        }
        break;

    case rwRASTERTYPEZBUFFER:
        {
            GLboolean   depthMask;


            if ( NULL != _rwOpenGLContextRaster->cpPixels )
            {
#if defined(RWDEBUG)
                RWMESSAGE( (RWSTRING("Unable to clear a locked Z buffer raster")) );
#endif /* defined(RWDEBUG) */
                RWRETURN( FALSE );
            }

            _rwOpenGLMakeCurrent();

            /* clamp to the area defined for clearing */
            RwOpenGLEnable( rwGL_SCISSOR_TEST );
            glScissor( rect->x,
                       _rwOpenGLGetEngineWindowHeight() - (rect->y + rect->h), 
                       rect->w, 
                       rect->h );

            /* save the current depth mask */
            glGetBooleanv( GL_DEPTH_WRITEMASK, &depthMask );

            /* enable writes to the z buffer */
            glDepthMask( TRUE );

            glClear( GL_DEPTH_BUFFER_BIT );

            /* restore depth mask as necessary */
            if ( GL_FALSE == depthMask )
            {
                glDepthMask( FALSE );
            }

            RwOpenGLDisable( rwGL_SCISSOR_TEST );

            GL_ERROR_CHECK();
        }
        break;

    case rwRASTERTYPECAMERA:
        {
            GLfloat prevClearColor[4];

            RwRGBA  clearColor;


            if ( NULL != _rwOpenGLContextRaster->cpPixels )
            {
#if defined(RWDEBUG)
                RWMESSAGE( (RWSTRING("Unable to clear a locked camera raster")) );
#endif /* defined(RWDEBUG) */
                RWRETURN( FALSE );
            }

            _rwOpenGLMakeCurrent();

            /* clamp to the area defined for clearing */
            RwOpenGLEnable( rwGL_SCISSOR_TEST );
            glScissor( rect->x,
                       _rwOpenGLGetEngineWindowHeight() - (rect->y + rect->h), 
                       rect->w, 
                       rect->h );

            /* remember the current clear color */
            glGetFloatv( GL_COLOR_CLEAR_VALUE, prevClearColor );

            RwRGBASetFromPixel( &clearColor, pixelColor, RwRasterGetFormat(_rwOpenGLContextRaster) );

            glClearColor( clearColor.red * COLORSCALAR,
                          clearColor.green * COLORSCALAR,
                          clearColor.blue * COLORSCALAR,
                          clearColor.alpha * COLORSCALAR );

            glClear( GL_COLOR_BUFFER_BIT );

            /* restore the clear color */
            glClearColor( prevClearColor[0],
                          prevClearColor[1],
                          prevClearColor[2],
                          prevClearColor[3] );

            RwOpenGLDisable( rwGL_SCISSOR_TEST );

            GL_ERROR_CHECK();
        }
        break;

    default:
        RWERROR( (E_RW_INVRASTERFORMAT) );
        RWRETURN( FALSE );
    }

    RWRETURN( TRUE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRasterRenderTexturedQuad
 *
 *  Purpose : Render textured quad
 *
 *  On entry: raster - Pointer to RwRaster containing the raster to blit.
 *
 *            rect - Pointer to RwRect containing the area definition to blit to.
 *
 *            scale - Constant RwBool, TRUE to scale.
 *
 *            useAlpha - Constant RwBool, TRUE to use alpha.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
static RwBool
_rwOpenGLRasterRenderTexturedQuad( RwRaster *raster,
                                   RwRect *rect,
                                   const RwBool scale,
                                   const RwBool useAlpha )
{
    RwRaster                *srcParent;

    RwUInt32                twoN;

    GLfloat                 parActualWidth;
    
    GLfloat                 parActualHeight;

    GLfloat                 xorig;
    
    GLfloat                 yorig;
    
    GLfloat                 xdelta;
    
    GLfloat                 ydelta;
    
    GLfloat                 blitx;
    
    GLfloat                 blity;
    
    GLfloat                 blitw;
    
    GLfloat                 blith;

    _rwOpenGLRasterExt      *rasterExt;

    RwTextureAddressMode    addressModeU;

    RwTextureAddressMode    addressModeV;

    RwTextureFilterMode     filterMode;

    RwBool                  zWriteEnabled;

    RwBool                  zTestEnabled;


    RWFUNCTION( RWSTRING( "_rwOpenGLRasterRenderTexturedQuad" ) );

    RWASSERT( NULL != _rwOpenGLContextRaster );
    RWASSERT( NULL != raster );
    RWASSERT( NULL != rect );

    /* find the parent raster for the raster to be blitted */
    srcParent = raster;
    while ( srcParent != srcParent->parent )
    {
        srcParent = srcParent->parent;
    }

    twoN = _rwOpenGLNextPowerOf2(srcParent->width);
    parActualWidth  = (GLfloat)twoN;

    twoN = _rwOpenGLNextPowerOf2(srcParent->height);
    parActualHeight = (GLfloat)twoN;

    xorig  = (GLfloat)raster->nOffsetX / parActualWidth;
    yorig  = (GLfloat)raster->nOffsetY / parActualHeight;
    xdelta = (GLfloat)raster->width  / (GLfloat)parActualWidth;
    ydelta = (GLfloat)raster->height / (GLfloat)parActualHeight;

    blitx = (((GLfloat)rect->x / (GLfloat)_rwOpenGLContextRaster->width)  * 2) - 1;
    blity = -((((GLfloat)rect->y / (GLfloat)_rwOpenGLContextRaster->height) * 2) - 1);

    if ( FALSE != scale )
    {
        blitw =  ((GLfloat)(rect->w * 2)) / (GLfloat)_rwOpenGLContextRaster->width;
        blith = -((GLfloat)(rect->h * 2)) / (GLfloat)_rwOpenGLContextRaster->height;
    }
    else
    {
        blitw =  ((GLfloat)(raster->width  * 2)) / (GLfloat)_rwOpenGLContextRaster->width;
        blith = -((GLfloat)(raster->height * 2)) / (GLfloat)_rwOpenGLContextRaster->height;
    }

    _rwOpenGLMakeCurrent();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    rasterExt = RASTEREXTFROMRASTER(srcParent);

    /* set the raster */
    addressModeU = rasterExt->addressModeU;
    addressModeV = rasterExt->addressModeV;
    filterMode   = rasterExt->filterMode;

    if ( (FALSE != scale) &&
         ((rect->w == raster->width) ||
          (rect->h == raster->height)) )
    {
        rasterExt->filterMode = rwFILTERNEAREST;
    }
    else
    {
        rasterExt->filterMode = rwFILTERLINEAR;
    }

    rasterExt->addressModeU = rwTEXTUREADDRESSCLAMP;
    rasterExt->addressModeV = rwTEXTUREADDRESSCLAMP;

    _rwOpenGLSetRasterForceTextureParams( raster );

    rasterExt->addressModeU = addressModeU;
    rasterExt->addressModeV = addressModeV;
    rasterExt->filterMode   = filterMode;

    /* turn off z-buffer writes and tests */
    _rwOpenGLGetRenderState( rwRENDERSTATEZWRITEENABLE, (void *)&zWriteEnabled );
    _rwOpenGLGetRenderState( rwRENDERSTATEZTESTENABLE, (void *)&zTestEnabled );

    _rwOpenGLSetRenderState( rwRENDERSTATEZWRITEENABLE, (void *)FALSE );
    _rwOpenGLSetRenderState( rwRENDERSTATEZTESTENABLE, (void *)FALSE );

    /* do we need blending? */
    if ( FALSE == useAlpha )
    {
        _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)FALSE );
    }
    else
    {
        _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE );
        _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND, (void *)rwBLENDSRCALPHA );
        _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND, (void *)rwBLENDINVSRCALPHA );
    }

    glBegin( GL_QUADS );
        glColor4f( 1, 1, 1, 1 );

        glTexCoord2f( xorig, yorig );
        glVertex2f( blitx, blity );

        glTexCoord2f( xorig, yorig + ydelta );
        glVertex2f( blitx, blity + blith );

        glTexCoord2f( xorig + xdelta, yorig + ydelta );
        glVertex2f( blitx + blitw, blity + blith );

        glTexCoord2f( xorig + xdelta, yorig );
        glVertex2f( blitx + blitw, blity );
    glEnd();

    /* restore z-buffer writes and tests */
    _rwOpenGLSetRenderState( rwRENDERSTATEZWRITEENABLE, (void *)zWriteEnabled );
    _rwOpenGLSetRenderState( rwRENDERSTATEZTESTENABLE, (void *)zTestEnabled );

    GL_ERROR_CHECK();

    RWRETURN( TRUE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRasterRenderCopy
 *
 *  Purpose : Copy raster data to another.
 *
 *  On entry: raster - Pointer to RwRaster containing the raster to blit.
 *
 *            rect - Pointer to RwRect containing the area definition to blit to.
 *
 *            useAlpha - Constant RwBool, TRUE to use alpha.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
static RwBool
_rwOpenGLRasterRenderCopy( RwRaster *raster,
                           RwRect *rect,
                           const RwBool useAlpha )
{
    RwRaster            *contextParent;

    _rwOpenGLRasterExt  *contextRasterExt;

    RwUInt8             *contextPixels;

    RwRaster            *sourceParent;

    _rwOpenGLRasterExt  *sourceRasterExt;

    RwUInt8             *srcPixels;

    RwInt32             x;

    RwInt32             y;


    RWFUNCTION( RWSTRING( "_rwOpenGLRasterRenderCopy" ) );

    RWASSERT( NULL != _rwOpenGLContextRaster );
    RWASSERT( NULL != raster );
    RWASSERT( NULL != rect );

    contextParent = _rwOpenGLContextRaster;
    while ( contextParent->parent != contextParent )
    {
        contextParent = contextParent->parent;
    }

    contextRasterExt = RASTEREXTFROMRASTER(contextParent);

    contextPixels = RwRasterLock( _rwOpenGLContextRaster, 0, rwRASTERLOCKWRITE );
    RWASSERT( NULL != contextPixels );
    if ( NULL == contextPixels )
    {
        RWRETURN( FALSE );
    }

    sourceParent = raster;
    while ( sourceParent->parent != sourceParent )
    {
        sourceParent = sourceParent->parent;
    }

    sourceRasterExt = RASTEREXTFROMRASTER(sourceParent);

    srcPixels = RwRasterLock( raster, 0, rwRASTERLOCKREAD );
    RWASSERT( NULL != srcPixels );
    if ( NULL == srcPixels )
    {
        RwRasterUnlock( _rwOpenGLContextRaster );
        RWRETURN( FALSE );
    }

    switch ( _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(contextParent->cFormat)].clientPixelFormat )
    {
    case GL_RGB:
        {
            RWASSERT( 3 == _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(_rwOpenGLContextRaster->cFormat)].clientBytesPerPixel );

            contextPixels += _rwOpenGLContextRaster->stride * rect->y + 3 * rect->x;

            switch ( _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(sourceParent->cFormat)].clientPixelFormat )
            {
            case GL_RGB:
                {
                    RWASSERT( 3 == _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(raster->cFormat)].clientBytesPerPixel );

                    for ( y = 0; y < raster->height; y += 1 )
                    {
                        for ( x = 0; x < raster->width; x += 1 )
                        {
                            *(contextPixels + x * 3 + 0) = *(srcPixels + x * 3 + 0);    /* red   */
                            *(contextPixels + x * 3 + 1) = *(srcPixels + x * 3 + 1);    /* green */
                            *(contextPixels + x * 3 + 2) = *(srcPixels + x * 3 + 2);    /* blue  */
                        }

                        contextPixels += _rwOpenGLContextRaster->stride;
                        srcPixels += raster->stride;
                    }
                }
                break;

            case GL_RGBA:
                {
                    RWASSERT( 4 == _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(raster->cFormat)].clientBytesPerPixel );

                    for ( y = 0; y < raster->height; y += 1 )
                    {
                        for ( x = 0; x < raster->width; x += 1 )
                        {
                            *(contextPixels + x * 3 + 0) = *(srcPixels + x * 4 + 0);    /* red   */
                            *(contextPixels + x * 3 + 1) = *(srcPixels + x * 4 + 1);    /* green */
                            *(contextPixels + x * 3 + 2) = *(srcPixels + x * 4 + 2);    /* blue  */
                        }

                        contextPixels += _rwOpenGLContextRaster->stride;
                        srcPixels += raster->stride;
                    }
                }
                break;

            default:
                RWERROR( (E_RW_INVRASTERFORMAT) );
                RWRETURN( FALSE );
            }
        }
        break;

    case GL_RGBA:
        {
            RWASSERT( 4 == _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(_rwOpenGLContextRaster->cFormat)].clientBytesPerPixel );

            contextPixels += _rwOpenGLContextRaster->stride * rect->y + 4 * rect->x;

            switch ( _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(sourceParent->cFormat)].clientPixelFormat )
            {
            case GL_RGB:
                {
                    RWASSERT( 3 == _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(raster->cFormat)].clientBytesPerPixel );

                    for ( y = 0; y < raster->height; y += 1 )
                    {
                        for ( x = 0; x < raster->width; x += 1 )
                        {
                            *(contextPixels + x * 4 + 0) = *(srcPixels + x * 3 + 0);    /* red   */
                            *(contextPixels + x * 4 + 1) = *(srcPixels + x * 3 + 1);    /* green */
                            *(contextPixels + x * 4 + 2) = *(srcPixels + x * 3 + 2);    /* blue  */
                            *(contextPixels + x * 4 + 3) = 0xFF;                        /* alpha */
                        }

                        contextPixels += _rwOpenGLContextRaster->stride;
                        srcPixels += raster->stride;
                    }
                }
                break;

            case GL_RGBA:
                {
                    RWASSERT( 4 == _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(raster->cFormat)].clientBytesPerPixel );

                    if ( FALSE == useAlpha )
                    {
                        for ( y = 0; y < raster->height; y += 1 )
                        {
                            for ( x = 0; x < raster->width; x += 1 )
                            {
                                *(contextPixels + x * 4 + 0) = *(srcPixels + x * 4 + 0);    /* red   */
                                *(contextPixels + x * 4 + 1) = *(srcPixels + x * 4 + 1);    /* green */
                                *(contextPixels + x * 4 + 2) = *(srcPixels + x * 4 + 2);    /* blue  */
                                *(contextPixels + x * 4 + 3) = 0xFF;                        /* alpha */
                            }

                            contextPixels += _rwOpenGLContextRaster->stride;
                            srcPixels += raster->stride;
                        }
                    }
                    else
                    {
                        for ( y = 0; y < raster->height; y += 1 )
                        {
                            for ( x = 0; x < raster->width; x += 1 )
                            {
                                *(contextPixels + x * 4 + 0) = *(srcPixels + x * 4 + 0);    /* red   */
                                *(contextPixels + x * 4 + 1) = *(srcPixels + x * 4 + 1);    /* green */
                                *(contextPixels + x * 4 + 2) = *(srcPixels + x * 4 + 2);    /* blue  */
                                *(contextPixels + x * 4 + 3) = *(srcPixels + x * 4 + 3);    /* alpha */
                            }

                            contextPixels += _rwOpenGLContextRaster->stride;
                            srcPixels += raster->stride;
                        }
                    }
                }
                break;

            default:
                RWERROR( (E_RW_INVRASTERFORMAT) );
                RWRETURN( FALSE );
            }
        }
        break;

    default:
        RWERROR( (E_RW_INVRASTERFORMAT) );
        RWRETURN( FALSE );
    }

    /* TODO: 3D labs Wildcat bug causing corruption when the raster is unlocked
     * reproducable sent to 3D labs and they have a bug logged
     * - fixed in beta drivers 0569
     * - should be fixed in official 0621 drivers but untested
     * BZ#7100 */
    RwRasterUnlock( raster );
    RwRasterUnlock( _rwOpenGLContextRaster );

    RWRETURN( TRUE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRasterRenderGeneric
 *
 *  Purpose : Generic raster blitting.
 *
 *  On entry: raster - Pointer to RwRaster containing the raster to blit.
 *
 *            rect - Pointer to RwRect containing the area definition to blit to.
 *
 *            scale - Constant RwBool, TRUE to scale.
 *
 *            useAlpha - Constant RwBool, TRUE to use alpha.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
static RwBool
_rwOpenGLRasterRenderGeneric( RwRaster *raster,
                              RwRect *rect,
                              const RwBool scale,
                              const RwBool useAlpha )
{
    RwBool  success = FALSE;

    GLint   savedViewport[4];


    RWFUNCTION( RWSTRING( "_rwOpenGLRasterRenderGeneric" ) );

    RWASSERT( NULL != raster );
    RWASSERT( NULL != rect );

    if ( _rwOpenGLContextRaster->parent != _rwOpenGLContextRaster )
    {
        /* restrict camera raster blitting to only the region of the
         * client window that the raster occupies */
        RwOpenGLEnable( rwGL_SCISSOR_TEST );

        glScissor( _rwOpenGLContextRaster->nOffsetX,
                   _rwOpenGLGetEngineWindowHeight() - (_rwOpenGLContextRaster->nOffsetY + _rwOpenGLContextRaster->height),
                   _rwOpenGLContextRaster->width,
                   _rwOpenGLContextRaster->height );

        glGetIntegerv( GL_VIEWPORT, savedViewport );

        glViewport( _rwOpenGLContextRaster->nOffsetX,
                    _rwOpenGLGetEngineWindowHeight() - (_rwOpenGLContextRaster->nOffsetY + _rwOpenGLContextRaster->height),
                    _rwOpenGLContextRaster->width,
                    _rwOpenGLContextRaster->height );
    }

    /* what type of raster are we blitting to? */
    switch ( _rwOpenGLContextRaster->cType )
    {
    case rwRASTERTYPENORMAL:
        /* fall thru */
    case rwRASTERTYPETEXTURE:
        /* fall thru */
    case rwRASTERTYPECAMERATEXTURE:
        {
            switch ( raster->cType )
            {
                case rwRASTERTYPENORMAL:
                    /* fall thru */
                case rwRASTERTYPETEXTURE:
                    /* fall thru */
                case rwRASTERTYPECAMERATEXTURE:
                    /* fall thru */
                case rwRASTERTYPECAMERA:
                    {
                        if ( FALSE == scale )
                        {
                            success = _rwOpenGLRasterRenderCopy( raster, rect, useAlpha );
                        }
#if defined(RWDEBUG)
                        else
                        {
                            RWMESSAGE( (RWSTRING( "Unable to scale a blit to a rwRASTERTYPENORMAL, "\
                                                  "rwRASTERTYPETEXTURE or rwRASTERTYPECAMERATEXTURE raster" )) );
                        }
#endif /* defined(RWDEBUG) */
                    }
                    break;

                case rwRASTERTYPEZBUFFER:
                    /* fall thru */
                default:
                    RWERROR( (E_RW_INVRASTERFORMAT) );
            }
        }
        break;

    case rwRASTERTYPECAMERA:
        {
            switch ( raster->cType )
            {
                case rwRASTERTYPENORMAL:
                    /* fall thru */
                case rwRASTERTYPETEXTURE:
                    /* fall thru */
                case rwRASTERTYPECAMERATEXTURE:
                    success = _rwOpenGLRasterRenderTexturedQuad( raster, rect, scale, useAlpha );
                    break;

                case rwRASTERTYPECAMERA:
                    {
                        if ( FALSE == scale )
                        {
                            success = _rwOpenGLRasterRenderCopy( raster, rect, useAlpha );
                        }
#if defined(RWDEBUG)
                        else
                        {
                            RWMESSAGE( (RWSTRING( "Unable to scale a blit from a rwRASTERTYPECAMERA to a rwRASTERTYPECAMERA" )) );
                        }
#endif /* defined(RWDEBUG) */
                    }
                    break;

                case rwRASTERTYPEZBUFFER:
                    /* fall thru */
                default:
                    RWERROR( (E_RW_INVRASTERFORMAT) );
            }
        }
        break;

    case rwRASTERTYPEZBUFFER:
        /* fall thru */
    default:
        RWERROR( (E_RW_INVRASTERFORMAT) );
    }

    if ( _rwOpenGLContextRaster->parent != _rwOpenGLContextRaster )
    {
        glViewport( savedViewport[0],
                    savedViewport[1],
                    savedViewport[2],
                    savedViewport[3] );

        RwOpenGLDisable( rwGL_SCISSOR_TEST );
    }

    RWRETURN( success );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRasterRender
 *
 *  Purpose : Blit a raster to the current context, taking alpha into account.
 *
 *  On entry: raster - Pointer to RwRaster containing the raster to blit.
 *
 *            rect - Pointer to RwRect containing the area definition to blit to.
 *
 *            flags - RwInt32.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
RwBool 
_rwOpenGLRasterRender( RwRaster *raster,
                       RwRect *rect,
                       RwInt32 flags __RWUNUSED__ )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLRasterRender" ) );

    RWASSERT( NULL != raster );
    RWASSERT( NULL != rect );

    rect->w = 0;
    rect->h = 0;

    RWRETURN( _rwOpenGLRasterRenderGeneric( raster, rect, FALSE, TRUE ) );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRasterRenderFast
 *
 *  Purpose : Blit a raster to the current context, ignoring alpha.
 *
 *  On entry: raster - Pointer to RwRaster containing the raster to blit.
 *
 *            rect - Pointer to RwRect containing the area definition to blit to.
 *
 *            flags - RwInt32.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
RwBool 
_rwOpenGLRasterRenderFast( RwRaster *raster,
                           RwRect *rect, 
                           RwInt32 flags __RWUNUSED__ )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLRasterRenderFast" ) );

    RWASSERT( NULL != raster );
    RWASSERT( NULL != rect );

    rect->w = 0;
    rect->h = 0;

    RWRETURN( _rwOpenGLRasterRenderGeneric( raster, rect, FALSE, FALSE ) );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRasterRenderScaled
 *
 *  Purpose : Blit a scaled raster to the current context.
 *
 *  On entry: raster - Pointer to RwRaster containing the raster to blit.
 *
 *            rect - Pointer to RwRect containing the area definition to blit to.
 *
 *            flags - RwInt32.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
RwBool 
_rwOpenGLRasterRenderScaled( RwRaster *raster,
                             RwRect *rect,
                             RwInt32 flags __RWUNUSED__ )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLRasterRenderScaled" ) );

    RWASSERT( NULL != raster );
    RWASSERT( NULL != rect );

    if ( (raster->width == rect->w) &&
         (raster->height == rect->h) )
    {
        RWRETURN( _rwOpenGLRasterRenderGeneric( raster, rect, FALSE, TRUE ) );
    }
    else
    {
        RWRETURN( _rwOpenGLRasterRenderGeneric( raster, rect, TRUE, TRUE ) );
    }
}
