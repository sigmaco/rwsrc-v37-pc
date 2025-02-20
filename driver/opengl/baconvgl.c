/***************************************************************************
 *                                                                         *
 * Module  : baconvgl.c                                                    *
 *                                                                         *
 * Purpose : Converting to and from images to rasters                      *
 *                                                                         *
 **************************************************************************/

/****************************************************************************
 Includes
 */

#if defined(WIN32)
#include <windows.h>
#endif /* defined(WIN32) */
#include <gl/gl.h>

#include "barwtyp.h"
#include "baimage.h"
#include "batextur.h"

#include "baogl.h"
#include "bastdogl.h"
#include "drvfns.h"

#include "barastgl.h"


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

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */

/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLFindMSB
 *
 *  Purpose : Find the bit number of MSB of an integer.
 *
 *  On entry: nInt - RwInt32 as source integer.
 *
 *  On exit : RwInt32 containing the bit number of the MSB of the input.
 * ------------------------------------------------------------------------- */
static RwInt32
_rwOpenGLFindMSB( RwInt32 nInt )
{
    RwInt32 nPos = -1;


    RWFUNCTION( RWSTRING( "_rwOpenGLFindMSB" ) );

    while (nInt)
    {
        nPos += 1;

        nInt >>= 1;
    }

    RWRETURN( nPos );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRGBRasterFromPalImage
 *
 *  Purpose : Set an RGB raster's pixels from a paletted image.
 *
 *  On entry: raster - Pointer to RwRaster to set.
 *
 *            image  - Pointer to RwImage to get pixel data from.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
static RwBool
_rwOpenGLRGBRasterFromPalImage( RwRaster *raster,
                                RwImage *image )
{
    RwInt32     x;
    
    RwInt32     y;

    RwUInt8     *imagePixels;

    RwRGBA      *imagePalette;

    RwUInt8     *rasterPixels;


    RWFUNCTION( RWSTRING( "_rwOpenGLRGBRasterFromPalImage" ) );

    RWASSERT( NULL != raster );
    RWASSERT( NULL != image );

    imagePixels = image->cpPixels;
    imagePalette = image->palette;
    rasterPixels = raster->cpPixels;

    for ( y = 0; y < image->height; y += 1 )
    {
        RwUInt8  *imgPixelsCur = imagePixels;

        RwUInt8  *rasPixelsCur = rasterPixels;


        for ( x = 0; x < image->width; x += 1 )
        {
            RwRGBA *col = imagePalette + (*imgPixelsCur);

            *(rasPixelsCur++) = col->red;
            *(rasPixelsCur++) = col->green;
            *(rasPixelsCur++) = col->blue;

            imgPixelsCur += 1;
        }

        imagePixels += image->stride;
        rasterPixels += raster->stride;
    }

    RWRETURN( TRUE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRGBARasterFromPalImage
 *
 *  Purpose : Set an RGBA raster's pixels from a paletted image.
 *
 *  On entry: raster - Pointer to RwRaster to set.
 *
 *            image  - Pointer to RwImage to get pixel data from.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
static RwBool
_rwOpenGLRGBARasterFromPalImage( RwRaster *raster,
                                 RwImage *image )
{
    RwInt32     x;
    
    RwInt32     y;

    RwUInt8     *imagePixels;

    RwRGBA      *imagePalette;

    RwUInt8     *rasterPixels;


    RWFUNCTION( RWSTRING( "_rwOpenGLRGBARasterFromPalImage" ) );

    RWASSERT( NULL != raster );
    RWASSERT( NULL != image );

    imagePixels = image->cpPixels;
    imagePalette = image->palette;
    rasterPixels = raster->cpPixels;

    for ( y = 0; y < image->height; y += 1 )
    {
        RwUInt8 *imgPixelsCur = imagePixels;

        RwRGBA  *rasPixelsCur = (RwRGBA *)rasterPixels;


        for ( x = 0; x < image->width; x += 1 )
        {
            RwRGBA *col = imagePalette + (*imgPixelsCur);

            *rasPixelsCur = *col;

            rasPixelsCur += 1;
            imgPixelsCur += 1;
        }

        imagePixels += image->stride;
        rasterPixels += raster->stride;
    }

    RWRETURN( TRUE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRGBRasterFromImage32
 *
 *  Purpose : Set an RGB raster's pixels from a 32-bit image.
 *
 *  On entry: raster - Pointer to RwRaster to set.
 *
 *            image  - Pointer to RwImage to get pixel data from.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
static RwBool
_rwOpenGLRGBRasterFromImage32( RwRaster *raster,
                               RwImage *image )
{
    RwInt32     x;
    
    RwInt32     y;

    RwUInt8     *imagePixels;

    RwUInt8     *rasterPixels;


    RWFUNCTION( RWSTRING( "_rwOpenGLRGBRasterFromImage32" ) );

    RWASSERT( NULL != raster );
    RWASSERT( NULL != image );

    imagePixels = image->cpPixels;
    rasterPixels = raster->cpPixels;

    for ( y = 0; y < image->height; y += 1 )
    {
        RwRGBA  *col = (RwRGBA *)imagePixels;

        RwUInt8 *rasPixelsCur = rasterPixels;


        for ( x = 0; x < image->width; x += 1 )
        {
            *(rasPixelsCur++) = col->red;
            *(rasPixelsCur++) = col->green;
            *(rasPixelsCur++) = col->blue;

            col += 1;
        }

        imagePixels += image->stride;
        rasterPixels += raster->stride;
    }

    RWRETURN( TRUE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRGBARasterFromImage32
 *
 *  Purpose : Set an RGBA raster's pixels from a 32-bit image.
 *
 *  On entry: raster - Pointer to RwRaster to set.
 *
 *            image  - Pointer to RwImage to get pixel data from.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
static RwBool
_rwOpenGLRGBARasterFromImage32( RwRaster *raster,
                                RwImage *image )
{
    RwInt32     x;
    
    RwInt32     y;

    RwUInt8     *imagePixels;

    RwUInt8     *rasterPixels;


    RWFUNCTION( RWSTRING( "_rwOpenGLRGBARasterFromImage32" ) );

    RWASSERT( NULL != raster );
    RWASSERT( NULL != image );

    imagePixels = image->cpPixels;
    rasterPixels = raster->cpPixels;

    for ( y = 0; y < image->height; y += 1 )
    {
        RwRGBA  *col = (RwRGBA *)imagePixels;

        RwRGBA  *rasPixelsCur = (RwRGBA *)rasterPixels;


        for ( x = 0; x < image->width; x += 1 )
        {
            *rasPixelsCur = *col;

            rasPixelsCur += 1;
            col += 1;
        }

        imagePixels += image->stride;
        rasterPixels += raster->stride;
    }

    RWRETURN( TRUE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRasterSetImage
 *
 *  Purpose : OpenGL specific raster from image.
 *
 *  On entry: raster - Pointer to RwRaster to set.
 *
 *            image - Pointer to RwImage used as source pixel data.
 *
 *            flags - RwInt32 containing 0.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
RwBool 
_rwOpenGLRasterSetImage( RwRaster *raster,
                         RwImage *image,
                         RwInt32 flags __RWUNUSEDRELEASE__ )
{
    RwBool              rasterIsPixelLocked = FALSE;

    RwRasterFormat      format;

    RwRaster            *topRaster;

    _rwOpenGLRasterExt  *rasterExt;

    RwBool              success = FALSE;


    RWFUNCTION( RWSTRING( "_rwOpenGLRasterSetImage" ) );

    RWASSERT( NULL != raster );
    RWASSERT( NULL != image );
    RWASSERT( 0 == flags );

    if ( rwRASTERPIXELLOCKEDWRITE == (raster->privateFlags & rwRASTERPIXELLOCKEDWRITE) )
    {
        rasterIsPixelLocked = TRUE;
    }
    else
    {
        if ( FALSE == RwRasterLock( raster, 0, rwRASTERLOCKWRITE | rwRASTERLOCKNOFETCH ) )
        {
            RWRETURN( FALSE );
        }
    }

    format = RwRasterGetFormat( raster );

    /* we won't have a paletted raster in OpenGL */
    RWASSERT( 0 == (format & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) );

    /* only the top-level parent raster has a valid texture Id */
    topRaster = raster;
    while ( topRaster != topRaster->parent )
    {
        topRaster = topRaster->parent;
    }

    rasterExt = RASTEREXTFROMRASTER( topRaster );
    RWASSERT( NULL != rasterExt );

    if ( (4 == image->depth) || (8 == image->depth) )
    {
        switch ( _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(topRaster->cFormat)].clientPixelFormat )
        {
        case GL_RGB:
            success = _rwOpenGLRGBRasterFromPalImage( raster, image );
            break;

        case GL_RGBA:
            success = _rwOpenGLRGBARasterFromPalImage( raster, image );
            break;

        default:
            RWERROR( (E_RW_INVRASTERFORMAT) );
            RWRETURN( FALSE );
        }
    }
    else
    {
        switch ( _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(topRaster->cFormat)].clientPixelFormat )
        {
        case GL_RGB:
            success = _rwOpenGLRGBRasterFromImage32( raster, image );
            break;

        case GL_RGBA:
            success = _rwOpenGLRGBARasterFromImage32( raster, image );
            break;

        default:
            RWERROR( (E_RW_INVRASTERFORMAT) );
            RWRETURN( FALSE );
        }
    }

    if ( FALSE == rasterIsPixelLocked )
    {
        RwRasterUnlock( raster );
    }

    RWRETURN( success );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLImageFindFormat
 *
 *  Purpose : Find a raster format suitable for an image.
 *
 *  On entry: image - Pointer to RwImage to find the format of.
 *
 *  On exit : RwRasterFormat containing the suggested raster format.
 * ------------------------------------------------------------------------- */
static RwRasterFormat
_rwOpenGLImageFindFormat( RwImage *image )
{
    RwInt32         depth;

    RwRasterFormat  format;


    RWFUNCTION( RWSTRING( "_rwOpenGLImageFindFormat" ) );

    RWASSERT( NULL != image );

    depth = RwImageGetDepth(image);

    if ( (4 == depth) || (8 == depth) )
    {
        const RwInt32   width = RwImageGetWidth(image);

        const RwInt32   height = RwImageGetHeight(image);

        const RwUInt8   *cpIn = image->cpPixels;

        const RwRGBA    *rpPal = image->palette;

        RwInt32         y;

        RwBool          paletteHasAlpha;


        /* first: check palette for transparent colors */
        paletteHasAlpha = FALSE;
        if ( 4 == depth )
        {
            for (y = 0; y < 16; y++)
            {
                if (0xFF != rpPal[y].alpha)
                {
                    paletteHasAlpha = TRUE;
                    break;
                }
            }
        }
        else
        {
            for (y = 0; y < 256; y++)
            {
                if (0xFF != rpPal[y].alpha)
                {
                    paletteHasAlpha = TRUE;
                    break;
                }
            }
        }

        if ( FALSE != paletteHasAlpha )
        {
            for (y = 0; y < height; y++)
            {
                const RwUInt8   *cpInCur = cpIn;
                RwInt32         x;

                for (x = 0; x < width; x++)
                {
                    /* Is there any alpha */
                    if (0xFF != rpPal[*cpInCur].alpha)
                    {
                        if ( _rwOpenGLGetEngineColorDepth() > 16 )
                        {
                            format = rwRASTERFORMAT8888;
                        }
                        else
                        {
                            format = rwRASTERFORMAT4444;
                        }

                        RWRETURN(format);
                    }

                    /* Next pixel */
                    cpInCur++;
                }

                cpIn += RwImageGetStride(image);
            }
        }

        if ( _rwOpenGLGetEngineColorDepth() > 16 )
        {
            format = rwRASTERFORMAT888;
        }
        else
        {
            format = rwRASTERFORMAT555;
        }
    }
    else
    {
        const RwInt32   width = RwImageGetWidth(image);
        const RwInt32   height = RwImageGetHeight(image);
        const RwUInt8   *cpIn = image->cpPixels;
        RwInt32         y;
        RwUInt32        alphaBits = 0;

        for (y = 0; y < height; y++)
        {
            const RwRGBA    *rpInCur = (const RwRGBA *)cpIn;
            RwInt32         x;

            for (x = 0; x < width; x++)
            {
                if (rpInCur->alpha < 0xff)
                {
                    /* lower 4 bits of the alpha channel are discarded in 4444 */
                    if (rpInCur->alpha > 0xf)
                    {
                        alphaBits = 8;
                        break;
                    }
                    else
                    {
                        alphaBits = 1;
                    }
                }

                /* Next pixel */
                rpInCur++;
            }

            if (alphaBits >= 8)
            {
                break;
            }

            cpIn += RwImageGetStride(image);
        }

        if (alphaBits >= 8)
        {
            if ( _rwOpenGLGetEngineColorDepth() > 16 )
            {
                format = rwRASTERFORMAT8888;
            }
            else
            {
                format = rwRASTERFORMAT4444;
            }
        }
        else if (alphaBits)
        {
            if ( _rwOpenGLGetEngineColorDepth() > 16 )
            {
                format = rwRASTERFORMAT8888;
            }
            else
            {
                format = rwRASTERFORMAT1555;
            }
        }
        else
        {
            if ( _rwOpenGLGetEngineColorDepth() > 16 )
            {
                format = rwRASTERFORMAT888;
            }
            else
            {
                format = rwRASTERFORMAT565;
            }
        }
    }

    RWRETURN(format);
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLImageFindRasterFormat
 *
 *  Purpose : Find a suitable raster format for an image, and fixup any
 *            raster type 
 *
 *  On entry: raster - Pointer to RwRaster to fill out format details for.
 *
 *            image - Pointer to RwImage to find a suitable raster format for.
 *
 *            flags - RwInt32 containing raster creation flags.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
RwBool 
_rwOpenGLImageFindRasterFormat( RwRaster *raster,
                                RwImage *image,
                                RwInt32 flags )
{
    RwRasterFormat      format;


    RWFUNCTION( RWSTRING( "_rwOpenGLImageFindRasterFormat" ) );

    RWASSERT( NULL != raster );
    RWASSERT( NULL != image );

    /* set up some defaults */
    raster->width  = image->width;
    raster->height = image->height;
    raster->depth  = 0;

    /* find a suitable format for the image */
    format = _rwOpenGLImageFindFormat( image );

    if ( (0 != raster->width) && (0 != raster->height) )
    {
        switch ( flags & rwRASTERTYPEMASK )
        {
        case rwRASTERTYPENORMAL:
            {
                /* because normal rasters are in fact textures, we must limit their size */
                RwInt32 maxTexSize = _rwOpenGLGetMaxSupportedTextureSize();

                /* lets cap it first */
                if ( raster->width > maxTexSize )
                {
                    raster->width = maxTexSize;
                }

                if ( raster->height > maxTexSize )
                {
                    raster->height = maxTexSize;
                }

                /* find the next _lowest_ resolution which is a power of 2 */
                raster->width  = 1 << _rwOpenGLFindMSB(raster->width);
                raster->height = 1 << _rwOpenGLFindMSB(raster->height);

                /* ensure mipmapping is not enabled */
                RWASSERT( 0 == (flags & (rwRASTERFORMATMIPMAP | rwRASTERFORMATAUTOMIPMAP)) );
                if ( 0 != (flags & (rwRASTERFORMATMIPMAP | rwRASTERFORMATAUTOMIPMAP)) )
                {
#if defined(RWDEBUG)
                    RWMESSAGE( (RWSTRING( "Normal rasters cannot be mipmapped.")) );
#endif /* defined(RWDEBUG) */

                    RWRETURN( FALSE );
                }
            }
            break;

        case rwRASTERTYPEZBUFFER:
            /* fall thru */
        case rwRASTERTYPECAMERA:
            {
                /* ought to have a good format from _rwOpenGLImageFindFormat */
            }
            break;

        case rwRASTERTYPETEXTURE:
            /* fall thru */
        case rwRASTERTYPECAMERATEXTURE:
            {
                RwInt32 maxTexSize = _rwOpenGLGetMaxSupportedTextureSize();


                /* lets cap it first */
                if ( raster->width > maxTexSize )
                {
                    raster->width = maxTexSize;
                }

                if ( raster->height > maxTexSize )
                {
                    raster->height = maxTexSize;
                }

                /* find the next _lowest_ resolution which is a power of 2 */
                raster->width  = 1 << _rwOpenGLFindMSB(raster->width);
                raster->height = 1 << _rwOpenGLFindMSB(raster->height);

                /* use mipmapping if required */
                format |= flags & (rwRASTERFORMATMIPMAP | rwRASTERFORMATAUTOMIPMAP);
            }
            break;

        default:
            RWERROR( (E_RW_INVRASTERFORMAT) );
            RWRETURN( FALSE );
        }
    }

    raster->cType = (RwUInt8)(flags & rwRASTERTYPEMASK);
    raster->cFlags = (RwUInt8)(flags & ~rwRASTERTYPEMASK);

    raster->cFormat = (RwUInt8)(format >> 8);

    raster->depth = _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(raster->cFormat)].colorDepth;

    RWRETURN( TRUE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGL32ImageFromRGBRaster
 *
 *  Purpose : Set a 32-bit image's pixels from an RGB raster's data.
 *
 *  On entry: image - Pointer to RwImage to set the pixel data for.
 *
 *            raster - Pointer to RwRaster to get the pixel data from.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
static RwBool
_rwOpenGL32ImageFromRGBRaster( RwImage *image,
                               RwRaster *raster )
{
    RwInt32     x;
    
    RwInt32     y;

    RwUInt8     *imagePixels;

    RwUInt8     *rasterPixels;


    RWFUNCTION( RWSTRING( "_rwOpenGL32ImageFromRGBRaster" ) );

    RWASSERT( NULL != image );
    RWASSERT( NULL != raster );

    imagePixels = image->cpPixels;
    rasterPixels = raster->cpPixels;

    for ( y = 0; y < image->height; y += 1 )
    {
        RwRGBA  *col = (RwRGBA *)imagePixels;

        RwUInt8 *rasPixelsCur = rasterPixels;


        for ( x = 0; x < image->width; x += 1 )
        {
            col->red   = *(rasPixelsCur++);
            col->green = *(rasPixelsCur++);
            col->blue  = *(rasPixelsCur++);
            col->alpha = 0xFF;

            col += 1;
        }

        imagePixels += image->stride;
        rasterPixels += raster->stride;
    }

    RWRETURN( TRUE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGL32ImageFromRGBARaster
 *
 *  Purpose : Set a 32-bit image's pixels from an RGBA raster's data.
 *
 *  On entry: image - Pointer to RwImage to set the pixel data for.
 *
 *            raster - Pointer to RwRaster to get the pixel data from.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
static RwBool
_rwOpenGL32ImageFromRGBARaster( RwImage *image,
                                RwRaster *raster )
{
    RwInt32     x;
    
    RwInt32     y;

    RwUInt8     *imagePixels;

    RwUInt8     *rasterPixels;


    RWFUNCTION( RWSTRING( "_rwOpenGL32ImageFromRGBARaster" ) );

    RWASSERT( NULL != image );
    RWASSERT( NULL != raster );

    imagePixels = image->cpPixels;
    rasterPixels = raster->cpPixels;

    for ( y = 0; y < image->height; y += 1 )
    {
        RwRGBA  *col = (RwRGBA *)imagePixels;

        RwRGBA  *rasPixelsCur = (RwRGBA *)rasterPixels;


        for ( x = 0; x < image->width; x += 1 )
        {
            *col = *rasPixelsCur;

            rasPixelsCur += 1;
            col += 1;
        }

        imagePixels += image->stride;
        rasterPixels += raster->stride;
    }

    RWRETURN( TRUE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLImageSetRaster
 *
 *  Purpose : Set an image's data from a raster.
 *
 *  On entry: image - Pointer to RwImage whose pixels are to be set.
 *
 *            raster - Pointer to RwRaster containing source pixel data.
 *
 *            flags - Unused RwInt32.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
RwBool 
_rwOpenGLImageSetRaster( RwImage *image,
                         RwRaster *raster,
                         RwInt32 flags __RWUNUSED__ )
{
    RwRaster            *topRaster;

    _rwOpenGLRasterExt  *rasterExt;

    RwBool              rasterIsPixelLocked = FALSE;

    RwRasterFormat      format;

    RwBool              success = FALSE;


    RWFUNCTION( RWSTRING( "_rwOpenGLImageSetRaster" ) );

    RWASSERT( NULL != image );
    RWASSERT( NULL != image->cpPixels );
    RWASSERT( NULL != raster );

    if ( 32 != image->depth )
    {
        RWERROR( (E_RW_DEVICEERROR,
                  RWSTRING("Conversion to palettized images is not supported")) );

        RWRETURN( FALSE );
    }

    /* only the top-level parent raster has a valid texture Id */
    topRaster = raster;
    while ( topRaster != topRaster->parent )
    {
        topRaster = topRaster->parent;
    }

    rasterExt = RASTEREXTFROMRASTER( topRaster );
    RWASSERT( NULL != rasterExt );

    if ( (0 != rasterExt->dxtCompressedType) ||
         (GL_COMPRESSED_RGB_ARB == rasterExt->serverPixelFormat) ||
         (GL_COMPRESSED_RGBA_ARB == rasterExt->serverPixelFormat) )
    {
#if defined(RWDEBUG)
        RWMESSAGE( (RWSTRING( "Unable to define RwImage pixels from a DXT compressed raster" )) );
#endif /* defined(RWDEBUG) */

        RWRETURN( FALSE );
    }

    if ( rwRASTERPIXELLOCKEDREAD == (raster->privateFlags & rwRASTERPIXELLOCKEDREAD) )
    {
        rasterIsPixelLocked = TRUE;
    }
    else
    {
        if ( FALSE == RwRasterLock( raster, 0, rwRASTERLOCKREAD ) )
        {
            RWRETURN( FALSE );
        }
    }

    format = RwRasterGetFormat( raster );

    /* we won't have a paletted raster */
    RWASSERT( 0 == (format & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) );

    /* only the top-level parent raster has a valid texture Id */
    topRaster = raster;
    while ( topRaster != topRaster->parent )
    {
        topRaster = topRaster->parent;
    }

    rasterExt = RASTEREXTFROMRASTER( topRaster );
    RWASSERT( NULL != rasterExt );

    switch ( _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(topRaster->cFormat)].clientPixelFormat )
    {
    case GL_RGB:
        success = _rwOpenGL32ImageFromRGBRaster( image, raster );
        break;

    case GL_RGBA:
        success = _rwOpenGL32ImageFromRGBARaster( image, raster );
        break;

    default:
        RWERROR( (E_RW_INVRASTERFORMAT) );
        RWRETURN( FALSE );
    }

    if ( FALSE == rasterIsPixelLocked )
    {
        RwRasterUnlock( raster );
    }

    RWRETURN( success );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRGBToPixel
 *
 *  Purpose : Convert RGBA color to a packed pixel color using a raster format
 *            as the conversion guide.
 *
 *  On entry: pixelColor - Pointer to RwUInt32 to return the packed pixel color.
 *
 *            RGBAcolor - Pointer to RwRGBA containing the RGBA color to convert.
 *
 *            rasterFormat - RwInt32 containing the raster format.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
RwBool 
_rwOpenGLRGBToPixel( RwUInt32 *pixelColor,
                     RwRGBA *RGBAcolor,
                     RwInt32 rasterFormat )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLRGBToPixel" ) );

    RWASSERT( NULL != pixelColor );
    RWASSERT( NULL != RGBAcolor );
    RWASSERT( rwRASTERFORMATDEFAULT != (rasterFormat & rwRASTERFORMATPIXELFORMATMASK) );

    /* alpha is the most significant byte
       red is the least significant byte */
    switch ( _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(rasterFormat)].clientPixelFormat )
    {
    case GL_RGB:
        *pixelColor = (0xFF             << 24) |
                      (RGBAcolor->blue  << 16) |
                      (RGBAcolor->green << 8)  |
                      (RGBAcolor->red);
        break;

    case GL_RGBA:
        *pixelColor = (RGBAcolor->alpha << 24) |
                      (RGBAcolor->blue  << 16) |
                      (RGBAcolor->green << 8)  |
                      (RGBAcolor->red);
        break;

    default:
        RWERROR( (E_RW_INVRASTERFORMAT) );
        RWRETURN( FALSE );
    }

    RWRETURN( TRUE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLPixelToRGB
 *
 *  Purpose : Convert packed pixel color into a RGBA color using a raster format
 *            as the conversion guide.
 *
 *  On entry: RGBAcolor - Pointer to RwRGBA containing the RGBA color to return
 *
 *            pixelColor - Pointer to RwUInt32 containing the packed pixel color.
 *
 *            rasterFormat - RwInt32 containing the raster format.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
RwBool 
_rwOpenGLPixelToRGB( RwRGBA *RGBAcolor,
                     RwUInt32 *pixelColor,
                     RwInt32 rasterFormat )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLPixelToRGB" ) );

    RWASSERT( NULL != RGBAcolor );
    RWASSERT( NULL != pixelColor );
    RWASSERT( rwRASTERFORMATDEFAULT != (rasterFormat & rwRASTERFORMATPIXELFORMATMASK) );

    /* alpha is the most significant byte
       red is the least significant byte */
    switch ( _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(rasterFormat)].clientPixelFormat )
    {
    case GL_RGB:
        RGBAcolor->alpha = 0xFF;
        RGBAcolor->blue  = (*pixelColor >> 16) & 0xFF;
        RGBAcolor->green = (*pixelColor >> 8) & 0xFF;
        RGBAcolor->red   = *pixelColor & 0xFF;
        break;

    case GL_RGBA:
        RGBAcolor->alpha = (*pixelColor >> 24) & 0xFF;
        RGBAcolor->blue  = (*pixelColor >> 16) & 0xFF;
        RGBAcolor->green = (*pixelColor >> 8)  & 0xFF;
        RGBAcolor->red   = (*pixelColor) & 0xFF;
        break;

    default:
        RWERROR( (E_RW_INVRASTERFORMAT) );
        RWRETURN( FALSE );
    }

    RWRETURN( TRUE );
}
