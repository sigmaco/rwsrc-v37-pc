/***************************************************************************
 *                                                                         *
 * Module  : barastgl.c                                                    *
 *                                                                         *
 * Purpose : OpenGL raster functionality                                   *
 *                                                                         *
 **************************************************************************/

/****************************************************************************
 Includes
 */
#include <string.h>

#if defined(WIN32)
#include <windows.h>
#endif /* defined(WIN32) */
#include <GL/gl.h>

#include "barwtyp.h"
#include "baimage.h"
#include "batextur.h"
#include "babinary.h"
#include "baimras.h"
#include "baresamp.h"
#include "osintf.h"

#include "baogl.h"
#include "bastdogl.h"

#include "barastgl.h"
#include "barstate.h"
#include "badxtgl.h"


/****************************************************************************
 Global prototypes
 */

/****************************************************************************
 Local Types
 */
typedef struct DDS_PIXELFORMAT DDS_PIXELFORMAT;
struct DDS_PIXELFORMAT
{
    RwUInt32 dwSize;
    RwUInt32 dwFlags;
    RwUInt32 dwFourCC;
    RwUInt32 dwRGBBitCount;
    RwUInt32 dwRBitMask;
    RwUInt32 dwGBitMask;
    RwUInt32 dwBBitMask;
    RwUInt32 dwABitMask;
};

typedef struct DDS_HEADER DDS_HEADER;
struct DDS_HEADER
{
    RwUInt32 dwSize;
    RwUInt32 dwFlags;
    RwUInt32 dwHeight;
    RwUInt32 dwWidth;
    RwUInt32 dwPitchOrLinearSize;
    RwUInt32 dwDepth;
    RwUInt32 dwMipMapCount;
    RwUInt32 dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    RwUInt32 dwCaps1;
    RwUInt32 dwCaps2;
    RwUInt32 dwReserved2[3];
};


/****************************************************************************
 Local (Static) Prototypes
 */


/****************************************************************************
 Local Defines
 */
#define NUMRASTERFORMATS (11)

#define rwSEARCHPATH_SEPARATOR ';' /* Borrowed from baimage.c */


/****************************************************************************
 Globals (across program)
 */

/* Raster extensions offset */
RwInt32 _rwOpenGLRasterExtOffset;

_rwOpenGLRasterFmt _rwOpenGLRasterFormatInfo[NUMRASTERFORMATS] =
/*  [bit depth][client byte depth][client format] */
{
    { 0 , 0, 0       },  /* rwRASTERFORMATDEFAULT */
    { 16, 4, GL_RGBA },  /* rwRASTERFORMAT1555    */
    { 16, 3, GL_RGB  },  /* rwRASTERFORMAT565     */
    { 16, 4, GL_RGBA },  /* rwRASTERFORMAT4444    */
    { 0,  0, 0       },  /* rwRASTERFORMATLUM8 (unsupported)   */
    { 32, 4, GL_RGBA },  /* rwRASTERFORMAT8888    */
    { 24, 3, GL_RGB  },  /* rwRASTERFORMAT888     */
    { 16, 3, GL_RGB  },  /* rwRASTERFORMAT16      */
    { 24, 3, GL_RGB  },  /* rwRASTERFORMAT24      */
    { 32, 4, GL_RGBA },  /* rwRASTERFORMAT32      */
    { 16, 3, GL_RGB  }   /* rwRASTERFORMAT555     */
};


/****************************************************************************
 Local (static) Globals
 */

static RwBool   _rwOpenGLUseGenericTextureCompression = FALSE;

/* raster format conversions from RenderWare Graphics to OpenGL server formats */
static RwUInt32 _rwOpenGLRasterUnCmpServerFormat[NUMRASTERFORMATS] =
{
    0,          /* rwRASTERFORMATDEFAULT */
    GL_RGB5_A1, /* rwRASTERFORMAT1555    */
    GL_RGB5,    /* rwRASTERFORMAT565     */
    GL_RGBA4,   /* rwRASTERFORMAT4444    */
    0,          /* rwRASTERFORMATLUM8 (unsupported)   */
    GL_RGBA8,   /* rwRASTERFORMAT8888    */
    GL_RGB8,    /* rwRASTERFORMAT888     */
    GL_RGB5,    /* rwRASTERFORMAT16      */
    GL_RGB8,    /* rwRASTERFORMAT24      */
    GL_RGBA8,   /* rwRASTERFORMAT32      */
    GL_RGB5,    /* rwRASTERFORMAT555     */ /* closest approximation I'm aware of */
};

/* generic compressed raster format conversions from RenderWare Graphics
 * to OpenGL server formats */
static RwUInt32 _rwOpenGLRasterGenCmpServerFormat[NUMRASTERFORMATS] =
{
    0,                      /* rwRASTERFORMATDEFAULT */
    GL_COMPRESSED_RGBA_ARB, /* rwRASTERFORMAT1555    */
    GL_COMPRESSED_RGB_ARB,  /* rwRASTERFORMAT565     */
    GL_COMPRESSED_RGBA_ARB, /* rwRASTERFORMAT4444    */
    0,                      /* rwRASTERFORMATLUM8 (unsupported) */
    GL_COMPRESSED_RGBA_ARB, /* rwRASTERFORMAT8888    */
    GL_COMPRESSED_RGB_ARB,  /* rwRASTERFORMAT888     */
    GL_COMPRESSED_RGB_ARB,  /* rwRASTERFORMAT16      */
    GL_COMPRESSED_RGB_ARB,  /* rwRASTERFORMAT24      */
    GL_COMPRESSED_RGBA_ARB, /* rwRASTERFORMAT32      */
    GL_COMPRESSED_RGB_ARB,  /* rwRASTERFORMAT555     */
};

static RwInt32 _rwOpenGLMaxTextureSize      = 0;


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRasterOpen
 *
 *  Purpose : Open the OpenGL raster module.
 *
 *  On entry: None.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
RwBool
_rwOpenGLRasterOpen( void )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLRasterOpen" ) );

    glGetIntegerv( GL_MAX_TEXTURE_SIZE, (GLint *)&_rwOpenGLMaxTextureSize );

    RwOpenGLRasterSetGenericTextureCompression( FALSE );

    /* TODO: seeing some performance reduction on ATI Radeon 9700 (~ 5-10%)
     * visible on the lightmap example... reproducible sent to ATI which
     * they have tested and are looking at
     * addendum: driver bug found by ATI - this fix will be in the 3.9
     * Catalyst drivers
     * Reportedly fixed in 7.95 beta drivers but I can't confirm it. BZ#7098.
     * Tested in Catalyst 3.9, and not yet fixed. */
    if ( FALSE != _rwOpenGLExt.GenerateMipmapSGIS )
    {
        glHint( GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST );
        GL_ERROR_CHECK();
    }

    RWRETURN( TRUE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRasterClose
 *
 *  Purpose : Close the OpenGL raster module.
 *
 *  On entry: None
 *
 *  On exit : Nothing.
 * ------------------------------------------------------------------------- */
void
_rwOpenGLRasterClose( void )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLRasterClose" ) );

    RWRETURNVOID();
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRasterGetMipLevels
 *
 *  Purpose : Get the number of miplevels available for a raster.
 *
 *  On entry: numMipLevels - Pointer to RwInt32 to return the number of miplevels
 *            for the raster
 *
 *            raster - Pointer to RwRaster to find the number of miplevels for.
 *
 *            flags - RwInt32 unused.
 *
 *  On exit : RwBool, TRUE.
 * ------------------------------------------------------------------------- */
RwBool 
_rwOpenGLRasterGetMipLevels( RwInt32 *numMipLevels,
                             RwRaster *raster,
                             RwInt32 flags __RWUNUSED__ )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLRasterGetMipLevels" ) );

    RWASSERT( NULL != raster );
    RWASSERT( NULL != numMipLevels );

    if ( rwRASTERTYPETEXTURE == RwRasterGetType( raster ) )
    {
        _rwOpenGLRasterExt  *rasterExt;


        rasterExt = RASTEREXTFROMRASTER( raster );
        RWASSERT( NULL != rasterExt );

        if ( FALSE == rasterExt->hardwareMipGen )
        {
            *numMipLevels = _rwOpenGLDetermineMaxMipLevel( RwRasterGetWidth(raster),
                                                           RwRasterGetHeight(raster) );
        }
        else
        {
            *numMipLevels = 1;
        }
    }
    else
    {
        *numMipLevels = 1;
    }

    RWRETURN( TRUE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRasterSubRaster
 *
 *  Purpose : Define a subraster.
 *
 *  On entry: subRaster - Pointer to RwRaster that is the sub raster.
 *
 *            raster - Pointer to RwRaster that is the parent raster.
 *
 *            flags - RwInt32 unused.
 *
 *  On exit : RwBool, TRUE if successful, FALSE otherwise.
 * ------------------------------------------------------------------------- */
RwBool 
_rwOpenGLRasterSubRaster( RwRaster *subRaster,
                          RwRaster *raster,
                          RwInt32 flags __RWUNUSED__ )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLRasterSubRaster" ) );

    /* core already set up offset and size */

    /* inherit some things */
    subRaster->stride  = raster->stride;
    subRaster->depth   = raster->depth;
    subRaster->cType   = raster->cType;
    subRaster->cFormat = raster->cFormat;

    /* generate a texture object - a side effect of the lock/unlock */
    if ( (rwRASTERTYPETEXTURE == subRaster->cType) ||
         (rwRASTERTYPENORMAL == subRaster->cType) )
    {
        RwRasterLock( subRaster, 0, rwRASTERLOCKWRITE );
        RwRasterUnlock( subRaster );
    }
    /* sub-rasters of a camera texture must have their vertical offset flipped */
    else if ( rwRASTERTYPECAMERATEXTURE == subRaster->cType )
    {
        RWASSERT( rwRASTERTYPECAMERATEXTURE == raster->cType );
        subRaster->nOffsetY = raster->height - subRaster->nOffsetY - subRaster->height;
    }

    RWRETURN( TRUE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLDetermineMaxMipLevel
 *
 *  Purpose : Find the maximum number of mip levels for a raster given
 *            a width and a height.
 *
 *  On entry: width - RwInt32 containing the width of the raster.
 *
 *            height - RwInt32 containing the height of the raster.
 *
 *  On exit : RwInt32 containing the maximum number of mip levels.
 * ------------------------------------------------------------------------- */
RwInt32
_rwOpenGLDetermineMaxMipLevel( RwInt32 width,
                               RwInt32 height )
{
    RwInt32 currentMipLevel;


    RWFUNCTION( RWSTRING( "_rwOpenGLDetermineMaxMipLevel" ) );

    currentMipLevel = 0;
    while( FALSE != _rwOpenGLDescribeMipLevel( currentMipLevel,
                                               width,
                                               height,
                                               1,
                                               NULL,
                                               NULL,
                                               NULL ) )
    {
        currentMipLevel++;
    }

    RWRETURN( currentMipLevel );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLDescribeMipLevel
 *
 *  Purpose : Describe the traits of a particular mip level given the top
 *            level traits.
 *
 *  On entry: mipLevel - RwInt32 containing the mip level.
 *
 *            width - RwInt32 containing the width of the top level mip.
 *
 *            height - RwInt32 containing the height of the top level mip.
 *
 *            bytesPerPixel - RwInt32 containing the bytes per pixel of
 *                            the client raster storage.
 *
 *            mipWidth - Pointer to RwInt32 to return the mip level's width.
 *
 *            mipHeight - Pointer to RwInt32 to return the mip level's height.
 *
 *            stride - Pointer to RwInt32 to return the mip level's stride.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 *
 * ------------------------------------------------------------------------- */
RwBool
_rwOpenGLDescribeMipLevel( RwInt32 mipLevel, 
                           RwInt32 width,
                           RwInt32 height,
                           RwInt32 bytesPerPixel,
                           RwInt32 *mipWidth,
                           RwInt32 *mipHeight, 
                           RwInt32 *stride )
{
    RwInt32 tempWidth;
    RwInt32 tempHeight;

    RWFUNCTION(RWSTRING("_rwOpenGLDescribeMipLevel"));

    tempWidth = width >> mipLevel;
    tempHeight = height >> mipLevel;

    if ((tempWidth == 0) && (tempHeight == 0))
    {
        RWRETURN(FALSE);
    }

    /* set actual values if required */
    if ( NULL != mipWidth )
    {
        *mipWidth = tempWidth ? tempWidth : 1;
    }

    if ( NULL != mipHeight )
    {
        *mipHeight = tempHeight ? tempHeight : 1;
    }

    if ( NULL != stride )
    {
        RWASSERT( bytesPerPixel > 0 );
        RWASSERT( bytesPerPixel <= 4 );

        *stride = bytesPerPixel * (*mipWidth);
    }

    RWRETURN(TRUE);
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRasterSetFormat
 *
 *  Purpose : Set a raster's format
 *
 *  On entry: raster - Pointer to RwRaster that has been created.
 *
 *            flags - RwInt32 containing flags describing the raster.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
RwBool
_rwOpenGLRasterSetFormat( RwRaster *raster,
                          RwInt32 flags )
{
    _rwOpenGLRasterExt  *rasterExt;

    RwRasterFormat      format;

    RwRasterFormat      pixelFormat;


    RWFUNCTION( RWSTRING( "_rwOpenGLRasterSetFormat" ) );

    RWASSERT( NULL != raster );

    rasterExt = RASTEREXTFROMRASTER( raster );
    RWASSERT( NULL != rasterExt );

    /* raster type and flags */
    raster->cType    = (RwUInt8)(flags & rwRASTERTYPEMASK);
    raster->cFlags   = (RwUInt8)(flags & ~rwRASTERTYPEMASK);

    format = flags & rwRASTERFORMATMASK;
    if ( 0 != (format & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) )
    {
#if defined(RWDEBUG)
        RWMESSAGE( (RWSTRING("Palettized raster formats unsupported on OpenGL.")) );
#endif /* defined(RWDEBUG) */

        RWRETURN( FALSE );
    }

    pixelFormat = format & rwRASTERFORMATPIXELFORMATMASK;

    switch ( raster->cType )
    {
    case rwRASTERTYPENORMAL:
        {
            if ( rwRASTERFORMATDEFAULT == pixelFormat )
            {
                RwRasterFormat  newFormat;


                switch ( _rwOpenGLGetEngineColorDepth() )
                {
                case 16:
                    newFormat = rwRASTERFORMAT4444;
                    break;

                case 24:
                    newFormat = rwRASTERFORMAT888;
                    break;

                case 32:
                    newFormat = rwRASTERFORMAT8888;
                    break;

                default:
                    RWERROR( (E_RW_INVRASTERFORMAT) );
                    RWRETURN( FALSE );
                }

                format                        |= newFormat;
                raster->depth                  = _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(newFormat)].colorDepth;
                raster->stride                 = raster->width * _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(newFormat)].clientBytesPerPixel;
                rasterExt->serverPixelFormat   = _rwOpenGLRasterUnCmpServerFormat[RWPIXELFORMAT2GL(newFormat)];
            }
            else
            {
                switch ( pixelFormat )
                {
                case rwRASTERFORMAT565:
                case rwRASTERFORMAT16:
                case rwRASTERFORMAT555:
                case rwRASTERFORMAT1555:
                case rwRASTERFORMAT4444:
                case rwRASTERFORMAT8888:
                case rwRASTERFORMAT32:
                case rwRASTERFORMAT888:
                case rwRASTERFORMAT24:
                    {
                        raster->depth                  = _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(pixelFormat)].colorDepth;
                        raster->stride                 = raster->width * _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(pixelFormat)].clientBytesPerPixel;
                        rasterExt->serverPixelFormat   = _rwOpenGLRasterUnCmpServerFormat[RWPIXELFORMAT2GL(pixelFormat)];
                    }
                    break;

                case rwRASTERFORMATLUM8:
                default:
                    RWERROR( (E_RW_INVRASTERFORMAT) );
                    RWRETURN( FALSE );
                }
            }
        }
        break;

    case rwRASTERTYPEZBUFFER:
        {
            const RwInt32   zDepth = _rwOpenGLGetEngineZBufferDepth();


            if ( rwRASTERFORMATDEFAULT == pixelFormat )
            {
                switch ( zDepth )
                {
                case 16:
                    format = rwRASTERFORMAT16;
                    break;

                case 24:
                    format = rwRASTERFORMAT24;
                    break;

                case 32:
                    format = rwRASTERFORMAT32;
                    break;

                default:
                    RWERROR( (E_RW_INVRASTERFORMAT) );
                    RWRETURN( FALSE );
                }

                raster->depth                  = _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(format)].colorDepth;
                raster->stride                 = raster->width * _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(format)].clientBytesPerPixel;
                rasterExt->serverPixelFormat   = _rwOpenGLRasterUnCmpServerFormat[RWPIXELFORMAT2GL(format)];
            }
            else
            {
                if ( (rwRASTERFORMAT16 == format) &&
                     (16 == zDepth) )
                {
                    raster->depth                  = _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(rwRASTERFORMAT16)].colorDepth;
                    raster->stride                 = raster->width * _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(rwRASTERFORMAT16)].clientBytesPerPixel;
                    rasterExt->serverPixelFormat   = _rwOpenGLRasterUnCmpServerFormat[RWPIXELFORMAT2GL(rwRASTERFORMAT16)];
                }
                else if ( (rwRASTERFORMAT24 == format) &&
                          (24 == zDepth) )
                {
                    raster->depth                  = _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(rwRASTERFORMAT24)].colorDepth;
                    raster->stride                 = raster->width * _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(rwRASTERFORMAT24)].clientBytesPerPixel;
                    rasterExt->serverPixelFormat   = _rwOpenGLRasterUnCmpServerFormat[RWPIXELFORMAT2GL(rwRASTERFORMAT24)];
                }
                else if ( (rwRASTERFORMAT32 == format) &&
                          (32 == zDepth) )
                {
                    raster->depth                  = _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(rwRASTERFORMAT32)].colorDepth;
                    raster->stride                 = raster->width * _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(rwRASTERFORMAT32)].clientBytesPerPixel;
                    rasterExt->serverPixelFormat   = _rwOpenGLRasterUnCmpServerFormat[RWPIXELFORMAT2GL(rwRASTERFORMAT32)];
                }
                else
                {
                    RWERROR( (E_RW_INVRASTERFORMAT) );
                    RWRETURN( FALSE );
                }
            }
        }
        break;

    case rwRASTERTYPECAMERA:
        {
            if ( rwRASTERFORMATDEFAULT == pixelFormat )
            {
                switch ( _rwOpenGLGetEngineColorDepth() )
                {
                case 16:
                    format = rwRASTERFORMAT565;
                    break;

                case 24:
                    format = rwRASTERFORMAT888;
                    break;

                case 32:
                    format = rwRASTERFORMAT8888;
                    break;
                    
                default:
                    RWERROR( (E_RW_INVRASTERFORMAT) );
                    RWRETURN( FALSE );
                }

                raster->depth                  = _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(format)].colorDepth;
                raster->stride                 = raster->width * _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(format)].clientBytesPerPixel;
                rasterExt->serverPixelFormat   = _rwOpenGLRasterUnCmpServerFormat[RWPIXELFORMAT2GL(format)];
            }
            else
            {
                switch ( pixelFormat )
                {
                case rwRASTERFORMAT565:
                case rwRASTERFORMAT888:
                case rwRASTERFORMAT8888:
                    raster->depth                  = _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(pixelFormat)].colorDepth;
                    raster->stride                 = raster->width * _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(pixelFormat)].clientBytesPerPixel;
                    rasterExt->serverPixelFormat   = _rwOpenGLRasterUnCmpServerFormat[RWPIXELFORMAT2GL(pixelFormat)];
                    break;

                default:
                    RWERROR( (E_RW_INVRASTERFORMAT) );
                    RWRETURN( FALSE );
                }
            }
        }
        break;

    case rwRASTERTYPETEXTURE:
        {
            RwUInt32  *rasterServerFormat;


            RWASSERT( 0 == (format & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) );

            /* choose uncompressed or compressed */
            if ( FALSE != _rwOpenGLUseGenericTextureCompression )
            {
                rasterServerFormat = _rwOpenGLRasterGenCmpServerFormat;
            }
            else
            {
                rasterServerFormat = _rwOpenGLRasterUnCmpServerFormat;
            }

            if ( rwRASTERFORMATDEFAULT == pixelFormat )
            {
                RwRasterFormat  newFormat;


                switch ( _rwOpenGLGetEngineColorDepth() )
                {
                case 16:
                    newFormat = rwRASTERFORMAT4444;
                    break;

                case 24:
                    newFormat = rwRASTERFORMAT888;
                    break;

                case 32:
                    newFormat = rwRASTERFORMAT8888;
                    break;

                default:
                    RWERROR( (E_RW_INVRASTERFORMAT) );
                    RWRETURN( FALSE );
                }

                format                        |= newFormat;
                raster->depth                  = _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(newFormat)].colorDepth;
                raster->stride                 = raster->width * _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(newFormat)].clientBytesPerPixel;
                rasterExt->serverPixelFormat   = rasterServerFormat[RWPIXELFORMAT2GL(newFormat)];
            }
            else
            {
                switch ( pixelFormat )
                {
                case rwRASTERFORMAT565:
                case rwRASTERFORMAT16:
                case rwRASTERFORMAT555:
                case rwRASTERFORMAT1555:
                case rwRASTERFORMAT4444:
                case rwRASTERFORMAT8888:
                case rwRASTERFORMAT32:
                case rwRASTERFORMAT888:
                case rwRASTERFORMAT24:
                    {
                        raster->depth                  = _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(pixelFormat)].colorDepth;
                        raster->stride                 = raster->width * _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(pixelFormat)].clientBytesPerPixel;
                        rasterExt->serverPixelFormat   = rasterServerFormat[RWPIXELFORMAT2GL(pixelFormat)];
                    }
                    break;

                case rwRASTERFORMATLUM8:
                default:
                    RWERROR( (E_RW_INVRASTERFORMAT) );
                    RWRETURN( FALSE );
                }
            }
        }
        break;

    case rwRASTERTYPECAMERATEXTURE:
        {
            RWASSERT( 0 == (format & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) );

            if ( rwRASTERFORMATDEFAULT == pixelFormat )
            {
                RwRasterFormat  newFormat;


                switch ( _rwOpenGLGetEngineColorDepth() )
                {
                case 16:
                    newFormat = rwRASTERFORMAT565;
                    break;

                case 24:
                    newFormat = rwRASTERFORMAT888;
                    break;

                case 32:
                    newFormat = rwRASTERFORMAT8888;
                    break;

                default:
                    RWERROR( (E_RW_INVRASTERFORMAT) );
                    RWRETURN( FALSE );
                }

                format                        |= newFormat;
                raster->depth                  = _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(newFormat)].colorDepth;
                raster->stride                 = raster->width * _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(newFormat)].clientBytesPerPixel;
                rasterExt->serverPixelFormat   = _rwOpenGLRasterUnCmpServerFormat[RWPIXELFORMAT2GL(newFormat)];
            }
            else
            {
                switch ( pixelFormat )
                {
                case rwRASTERFORMAT565:
                case rwRASTERFORMAT16:
                case rwRASTERFORMAT555:
                case rwRASTERFORMAT1555:
                case rwRASTERFORMAT4444:
                case rwRASTERFORMAT8888:
                case rwRASTERFORMAT32:
                case rwRASTERFORMAT888:
                case rwRASTERFORMAT24:
                    {
                        raster->depth                  = _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(pixelFormat)].colorDepth;
                        raster->stride                 = raster->width * _rwOpenGLRasterFormatInfo[RWPIXELFORMAT2GL(pixelFormat)].clientBytesPerPixel;
                        rasterExt->serverPixelFormat   = _rwOpenGLRasterUnCmpServerFormat[RWPIXELFORMAT2GL(pixelFormat)];
                    }
                    break;

                case rwRASTERFORMATLUM8:
                default:
                    RWERROR( (E_RW_INVRASTERFORMAT) );
                    RWRETURN( FALSE );
                }
            }
        }
        break;

    default:
        RWERROR( (E_RW_INVRASTERFORMAT) );
        RWRETURN( FALSE );
    }

    raster->cFormat = (RwUInt8)(format >> 8);

    RWRETURN( TRUE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRasterCreate
 *
 *  Purpose : OpenGL specific raster creation
 *
 *  On entry: inOut - Must be NULL
 *
 *            raster - Pointer to RwRaster that has been created.
 *
 *            flags - RwInt32 containing flags describing the raster.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
RwBool
_rwOpenGLRasterCreate( void *inOut __RWUNUSEDRELEASE__,
                       RwRaster *raster,
                       RwInt32 flags )
{
    _rwOpenGLRasterExt  *rasterExt;

    RwUInt32            idx;


    RWFUNCTION( RWSTRING( "_rwOpenGLRasterCreate" ) );

    RWASSERT( NULL == inOut );
    RWASSERT( NULL != raster );

    if ( (raster->width > _rwOpenGLMaxTextureSize) ||
         (raster->height > _rwOpenGLMaxTextureSize) )
    {
#if defined(RWDEBUG)
        RWMESSAGE( (RWSTRING("System only supports textures up to a maximum side of %d. Requested size was %d x %d"),
                   _rwOpenGLMaxTextureSize, raster->width, raster->height) );
#endif /* defined(RWDEBUG) */

        RWRETURN( FALSE );
    }

    rasterExt = RASTEREXTFROMRASTER( raster );
    RWASSERT( NULL != rasterExt );

    /* initialize raster extension data */
    rasterExt->textureID           = 0;
    rasterExt->addressModeU        = 0;
    rasterExt->addressModeV        = 0;
    rasterExt->filterMode          = 0;
    rasterExt->lockedMipLevel      = -1;
    for ( idx = 0; idx < MAX_MIPLEVELS; idx += 1 )
    {
        rasterExt->canFetchServerData[idx]  = FALSE;
    }
    rasterExt->serverPixelFormat   = 0;
    rasterExt->dxtCompressedType   = 0;
    rasterExt->hardwareMipGen      = FALSE;
    rasterExt->cubeMap             = FALSE;

    /* set the raster's format */
    if ( FALSE == _rwOpenGLRasterSetFormat( raster, flags ) )
    {
        RWRETURN( FALSE );
    }

    if ( (0 != raster->width) && (0 != raster->height) )
    {
        switch ( raster->cType )
        {
        case rwRASTERTYPEZBUFFER:
            {
                /* do nothing */
            }
            break;

        case rwRASTERTYPECAMERA:
            {
                /* do nothing */
            }
            break;

        case rwRASTERTYPENORMAL:
            {
                /* we don't mipmap normal rasters */
                if ( (rwRASTERFORMATAUTOMIPMAP | rwRASTERFORMATMIPMAP) == (flags & (rwRASTERFORMATAUTOMIPMAP | rwRASTERFORMATMIPMAP)) )
                {
#if defined(RWDEBUG)
                    RWMESSAGE( (RWSTRING( "Normal rasters cannot be mipmapped.")) );
#endif /* defined(RWDEBUG) */

                    RWRETURN( FALSE );
                }

                _rwOpenGLMakeCurrent();

                glGenTextures( 1, (GLuint *)&(rasterExt->textureID) );

                GL_ERROR_CHECK();
            }
            break;

        case rwRASTERTYPETEXTURE:
            {
                _rwOpenGLMakeCurrent();

                glGenTextures( 1, (GLuint *)&(rasterExt->textureID) );

                /* if we're mipmapping, can we generate them in hardware? */
                if ( (rwRASTERFORMATAUTOMIPMAP | rwRASTERFORMATMIPMAP) == (flags & (rwRASTERFORMATAUTOMIPMAP | rwRASTERFORMATMIPMAP)) )
                {
                    if ( FALSE != _rwOpenGLExt.GenerateMipmapSGIS )
                    {
                        rasterExt->hardwareMipGen = TRUE;
                    }
                }

                GL_ERROR_CHECK();
            }
            break;

        case rwRASTERTYPECAMERATEXTURE:
            {
                RwInt32 boundTexId;

                RwUInt8 *pixelData;


                _rwOpenGLMakeCurrent();

                /* save the currently bound texture as we are invalidating the state cache */
                glGetIntegerv( GL_TEXTURE_BINDING_2D, &boundTexId );

                glGenTextures( 1, (GLuint *)&(rasterExt->textureID) );
                glBindTexture( GL_TEXTURE_2D, rasterExt->textureID );

                /* camera textures won't necessarily be locked/unlocked before use
                 * so let's give them some pixel data */
                pixelData = (RwUInt8 *)RwDriverMalloc( raster->stride * raster->height,
                                                       rwID_DRIVERMODULE );
                RWASSERT( NULL != pixelData );
                if ( NULL == pixelData )
                {
                    RWERROR( (E_RW_NOMEM, raster->stride * raster->height) );
                    RWRETURN( FALSE );
                }

                /* number of pixels in a row */
                glPixelStorei( GL_UNPACK_ROW_LENGTH, raster->width );

                /* byte alignment */
                glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

                glTexImage2D( GL_TEXTURE_2D,
                              0,
                              rasterExt->serverPixelFormat,
                              raster->width,
                              raster->height,
                              0,
                              _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(raster->cFormat)].clientPixelFormat,
                              GL_UNSIGNED_BYTE,
                              pixelData );

                GL_ERROR_CHECK();

                RwDriverFree( pixelData );

                rasterExt->canFetchServerData[0] = TRUE;

                /* restore the bound texture to re-validate the state cache */
                glBindTexture( GL_TEXTURE_2D, boundTexId );
            }
            break;

        default:
            RWERROR( (E_RW_INVRASTERFORMAT) );
            RWRETURN( FALSE );
        }
    }
    else
    {
        /* Not allocated */
        raster->cFlags = rwRASTERDONTALLOCATE;
    }

    RWRETURN( TRUE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRasterDestroy
 *
 *  Purpose : OpenGL specific raster destruction
 *
 *  On entry: inOut - Must be NULL
 *
 *            raster - Pointer to RwRaster to destroy.
 *
 *            flags - RwInt32 containing 0.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
RwBool
_rwOpenGLRasterDestroy( void *inOut __RWUNUSEDRELEASE__,
                        RwRaster *raster,
                        RwInt32 flags __RWUNUSEDRELEASE__ )
{
    _rwOpenGLRasterExt  *rasterExt;

    
    RWFUNCTION( RWSTRING( "_rwOpenGLRasterDestroy" ) );

    RWASSERT( NULL == inOut );
    RWASSERT( NULL != raster );
    RWASSERT( 0 == flags );

    rasterExt = RASTEREXTFROMRASTER( raster );
    RWASSERT( NULL != rasterExt );

    /* if this raster is currently bound, we need to make sure
     * we don't try to do anything with it */
    _rwOpenGLRSUnbindRasterBeforeDestroy( raster );

    /* if this is the parent raster, delete any memeory */
    if ( raster == raster->parent )
    {
        if ( 0 != rasterExt->textureID )
        {
            _rwOpenGLMakeCurrent();

#if defined(RWDEBUG)
            if ( GL_FALSE == glIsTexture( rasterExt->textureID ) )
            {
                if ( NULL != RWSRCGLOBAL(debugFunction) )
                {
                    RwChar buffer[256];

                    rwsprintf( buffer, RWSTRING( "Texture %d was never bound" ), rasterExt->textureID );
                    RWSRCGLOBAL(debugFunction) (rwDEBUGMESSAGE, buffer);
                }
            }
#endif /* defined(RWDEBUG) */

            glDeleteTextures( 1, (GLuint *)&(rasterExt->textureID) );
            rasterExt->textureID = 0;

            GL_ERROR_CHECK();
        }

        if ( NULL != raster->cpPixels )
        {
            RwDriverFree( raster->cpPixels );
            raster->cpPixels = NULL;
        }

        if ( NULL != raster->palette )
        {
            RwDriverFree( raster->palette );
            raster->palette = NULL;
        }

        rasterExt->serverPixelFormat = 0;
        rasterExt->dxtCompressedType = 0;
        rasterExt->hardwareMipGen    = FALSE;
        rasterExt->cubeMap           = FALSE;
    }

    RWRETURN( TRUE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRasterLock
 *
 *  Purpose : OpenGL specific raster lock
 *
 *  On entry: pixels - Double pointer to RwUInt8 to return the pixel memory.
 *
 *            raster - Pointer to RwRaster to lock.
 *
 *            access - RwInt32 containing lock mode and mip level to lock.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
RwBool 
_rwOpenGLRasterLock( RwUInt8 **pixels,
                     RwRaster *raster,
                     RwInt32 access )
{
    RwRaster            *topRaster;

    _rwOpenGLRasterExt  *rasterExt;

    RwUInt8             mipLevel;

    
    RWFUNCTION( RWSTRING( "_rwOpenGLRasterLock" ) );

    RWASSERT( NULL != pixels );
    RWASSERT( NULL != raster );

    *pixels = NULL;

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
        RWMESSAGE( (RWSTRING( "System does not support locking a compressed raster." )) );
#endif /* defined(RWDEBUG) */

        RWRETURN( FALSE );
    }

    /* get the mip level to lock */
    mipLevel = (RwUInt8)((access & (RwInt32)0xFF00) >> 8);

    if ( (mipLevel > 0) && (FALSE != rasterExt->hardwareMipGen) )
    {
#if defined(RWDEBUG)
        RWMESSAGE( (RWSTRING( "Unable to lock mip level %d of a raster with hardware generated miplevels." ), mipLevel) );
#endif /* defined(RWDEBUG) */

        RWRETURN( FALSE );
    }

    switch ( raster->cType & rwRASTERTYPEMASK )
    {
        case rwRASTERTYPEZBUFFER:
            {
                RWERROR( (E_RW_INVRASTERLOCKREQ) );
                RWRETURN( FALSE );
            }
            break;

        case rwRASTERTYPECAMERA:
            {
                RwInt32 y;

                RwUInt8 *dstPixels;


                RWASSERT( NULL == raster->cpPixels );
                RWASSERT( 0 == mipLevel );

                if ( NULL != raster->cpPixels )
                {
#if defined(RWDEBUG)
                    RWMESSAGE( (RWSTRING( "Camera raster appears to be locked already" )) );
#endif /* defined(RWDEBUG) */

                    RWRETURN( FALSE );
                }
                if ( mipLevel > 0 )
                {
#if defined(RWDEBUG)
                    RWMESSAGE( (RWSTRING( "Camera rasters cannot be locked at miplevel %d" ), mipLevel) );
#endif /* defined(RWDEBUG) */
                    RWRETURN( FALSE );
                }

                topRaster->cpPixels = (RwUInt8 *)RwDriverMalloc( topRaster->stride * topRaster->height,
                                                                 rwID_DRIVERMODULE | rwMEMHINTDUR_EVENT );
                if ( NULL == topRaster->cpPixels )
                {
                    RWERROR( (E_RW_NOMEM, topRaster->stride * topRaster->height) );
                    RWRETURN( FALSE );
                }

                dstPixels = topRaster->cpPixels;

                glPushAttrib(GL_ALL_ATTRIB_BITS);

                    /* Now set up all the characteristics of our frame buffer transfer */
                    glPixelStorei( GL_PACK_SWAP_BYTES,  FALSE );
                    glPixelStorei( GL_PACK_LSB_FIRST,   FALSE );
                    glPixelStorei( GL_PACK_ROW_LENGTH,  0 );
                    glPixelStorei( GL_PACK_SKIP_PIXELS, 0 );
                    glPixelStorei( GL_PACK_SKIP_ROWS,   0 );
                    glPixelStorei( GL_PACK_ALIGNMENT,   4 );

                    for ( y = raster->height - 1; y >= 0; y -= 1 )
                    {
                        glReadPixels( 0,
                                      y,
                                      (GLsizei)topRaster->width,
                                      1,
                                      _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(topRaster->cFormat)].clientPixelFormat,
                                      GL_UNSIGNED_BYTE,
                                      (GLvoid *)dstPixels );

                        GL_ERROR_CHECK();

                        dstPixels += topRaster->stride;
                    }

                glPopAttrib();
            }
            break;

        case rwRASTERTYPENORMAL:
            /* fall thru */
        case rwRASTERTYPETEXTURE:
            /* fall thru */
        case rwRASTERTYPECAMERATEXTURE:
            {
                topRaster->originalWidth  = topRaster->width;
                topRaster->originalHeight = topRaster->height;
                topRaster->originalStride = topRaster->stride;

                /* get the mip level dimensions */
                if ( FALSE == _rwOpenGLDescribeMipLevel( mipLevel,
                                                         topRaster->originalWidth,
                                                         topRaster->originalHeight,
                                                         _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(topRaster->cFormat)].clientBytesPerPixel,
                                                         &(topRaster->width),
                                                         &(topRaster->height),
                                                         &(topRaster->stride) ) )
                {
                    RWERROR( (E_RW_INVRASTERMIPLEVEL) );
                    RWRETURN( FALSE );
                }

                /* camera textures and ordinary textures are guaranteed to be square */
                topRaster->cpPixels = (RwUInt8 *)RwDriverMalloc( topRaster->stride * topRaster->height,
                                                                 rwID_DRIVERMODULE | rwMEMHINTDUR_EVENT );
                if ( NULL == topRaster->cpPixels )
                {
                    topRaster->width = topRaster->originalWidth;
                    topRaster->height = topRaster->originalHeight;
                    topRaster->stride = topRaster->originalStride;

                    RWERROR( (E_RW_NOMEM, topRaster->stride * topRaster->height) );
                    RWRETURN( FALSE );
                }

                RWASSERT( mipLevel < MAX_MIPLEVELS );
                if ( (FALSE != rasterExt->canFetchServerData[mipLevel]) &&
                     (0 == (access & rwRASTERLOCKNOFETCH)) )
                {
                    RwInt32 boundTexId;

        
                    RWASSERT( 0 != _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(topRaster->cFormat)].clientBytesPerPixel );

                    _rwOpenGLMakeCurrent();

                    glPixelStorei( GL_PACK_ROW_LENGTH, topRaster->width );
                    glPixelStorei( GL_PACK_ALIGNMENT, 1 );

                    /* save the currently bound texture as we are invalidating the state cache */
                    glGetIntegerv( GL_TEXTURE_BINDING_2D, &boundTexId );

                    glBindTexture( GL_TEXTURE_2D, rasterExt->textureID );
                    glGetTexImage( GL_TEXTURE_2D,
                                   mipLevel,
                                   _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(topRaster->cFormat)].clientPixelFormat,
                                   GL_UNSIGNED_BYTE,
                                   topRaster->cpPixels );

                    GL_ERROR_CHECK();

                    /* restore the bound texture to re-validate the state cache */
                    glBindTexture( GL_TEXTURE_2D, boundTexId );
                }
            }
            break;

        default:
            RWERROR( (E_RW_INVRASTERFORMAT) );
            RWRETURN( FALSE );
    }

    /* set the private flags */
    if ( rwRASTERLOCKREAD == (access & rwRASTERLOCKREAD) )
    {
        raster->privateFlags |= rwRASTERPIXELLOCKEDREAD;
    }
    if ( rwRASTERLOCKWRITE == (access & rwRASTERLOCKWRITE) )
    {
        raster->privateFlags |= rwRASTERPIXELLOCKEDWRITE;
    }

    /* which mip level have we locked? */
    rasterExt->lockedMipLevel = mipLevel;

    /* take subrasters into account */
    raster->cpPixels = topRaster->cpPixels +
                       raster->stride * raster->nOffsetY +
                       (raster->depth >> 3) * raster->nOffsetX;

    /* take a copy of the pixel data address */
    *pixels = raster->cpPixels;

    RWRETURN( TRUE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRasterUnlock
 *
 *  Purpose : OpenGL specific raster unlock
 *
 *  On entry: inOut - Must be NULL
 *
 *            raster - Pointer to RwRaster to unlock.
 *
 *            access - RwInt32 equal to 0.
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
RwBool 
_rwOpenGLRasterUnlock( void *inOut __RWUNUSEDRELEASE__,
                       RwRaster *raster,
                       RwInt32 access __RWUNUSEDRELEASE__ )
{
    RwRaster            *topRaster;

    _rwOpenGLRasterExt  *rasterExt;

    
    RWFUNCTION( RWSTRING( "_rwOpenGLRasterUnlock" ) );

    RWASSERT( NULL == inOut );
    RWASSERT( NULL != raster );
    RWASSERT( 0 == access );
    RWASSERT( NULL != raster->cpPixels );

    /* only the top-level parent raster has a valid texture Id */
    topRaster = raster;
    while ( topRaster != topRaster->parent )
    {
        topRaster = topRaster->parent;
    }

    rasterExt = RASTEREXTFROMRASTER( topRaster );
    RWASSERT( NULL != rasterExt );

    switch ( raster->cType & rwRASTERTYPEMASK )
    {
        case rwRASTERTYPEZBUFFER:
            {
                RWERROR( (E_RW_INVRASTERLOCKREQ) );
                RWRETURN( FALSE );
            }
            break;

        case rwRASTERTYPECAMERA:
            {
                /* do we have to upload the data? */
                if ( rwRASTERPIXELLOCKEDWRITE == (raster->privateFlags & rwRASTERPIXELLOCKEDWRITE) )
                {
                    RwInt32 winHeight;

                    RwInt32 starty;

                    RwInt32 endy;

                    RwUInt8 *srcPixels;

                    RwInt32 y;


                    RWASSERT( NULL != topRaster->cpPixels );
                    RWASSERT( NULL != raster->cpPixels );

                    if ( NULL == topRaster->cpPixels )
                    {
#if defined(RWDEBUG)
                        RWMESSAGE( (RWSTRING("Camera raster's parent has no locked pixel data")) );
#endif /* defined(RWDEBUG) */

                        RWRETURN( FALSE );
                    }
                    if ( NULL == raster->cpPixels )
                    {
#if defined(RWDEBUG)
                        RWMESSAGE( (RWSTRING("Camera raster has no locked pixel data")) );
#endif /* defined(RWDEBUG) */

                        RWRETURN( FALSE );
                    }

                    winHeight = _rwOpenGLGetEngineWindowHeight();
                    starty    = winHeight - raster->nOffsetY;
                    endy      = winHeight - (raster->nOffsetY + raster->height),

                    srcPixels = raster->cpPixels;

                    glPushAttrib(GL_ALL_ATTRIB_BITS);

                        glViewport( 0,
                                    0,
                                    topRaster->width,
                                    topRaster->height );

                        glMatrixMode( GL_PROJECTION );
                        glLoadIdentity();
                        glOrtho( 0,
                                 topRaster->width,
                                 0,
                                 topRaster->height,
                                 -1.0,
                                 1.0 );

                        glMatrixMode(GL_MODELVIEW);
                        glLoadIdentity();

                        /* Now set up all the characteristics of our frame buffer transfer */
                        glPixelStorei( GL_UNPACK_SWAP_BYTES,  FALSE );
                        glPixelStorei( GL_UNPACK_LSB_FIRST,   FALSE );
                        glPixelStorei( GL_UNPACK_ROW_LENGTH,  0 );
                        glPixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
                        glPixelStorei( GL_UNPACK_SKIP_ROWS,   0 );
                        glPixelStorei( GL_UNPACK_ALIGNMENT,   4 );

                        for ( y = starty - 1; y >= endy; y -= 1 )
                        {
                            GLboolean validPosition;


                            glRasterPos2i( raster->nOffsetX, y );

                            glGetBooleanv( GL_CURRENT_RASTER_POSITION_VALID, &validPosition );
                            if ( GL_FALSE != validPosition )
                            {
                                glDrawPixels( raster->width,
                                              1,
                                              _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(topRaster->cFormat)].clientPixelFormat,
                                              GL_UNSIGNED_BYTE,
                                              (const GLvoid*)srcPixels );
                            }

                            srcPixels += raster->stride;
                        }

                    glPopAttrib();
                }
            }
            break;

        case rwRASTERTYPENORMAL:
            /* fall thru */
        case rwRASTERTYPETEXTURE:
            /* fall thru */
        case rwRASTERTYPECAMERATEXTURE:
            {
                /* do we have to upload the data? */
                if ( rwRASTERPIXELLOCKEDWRITE == (raster->privateFlags & rwRASTERPIXELLOCKEDWRITE) )
                {
                    RwInt32     boundTexId;


                    RWASSERT( 0 != rasterExt->serverPixelFormat );
                    RWASSERT( 0 != _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(topRaster->cFormat)].clientPixelFormat );

                    _rwOpenGLMakeCurrent();

                    /* save the currently bound texture as we are invalidating the state cache */
                    glGetIntegerv( GL_TEXTURE_BINDING_2D, &boundTexId );

                    glBindTexture( GL_TEXTURE_2D, rasterExt->textureID );

                    /* number of pixels in a row */
                    glPixelStorei( GL_UNPACK_ROW_LENGTH, topRaster->width );

                    /* byte alignment */
                    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

                    if ( FALSE != rasterExt->hardwareMipGen )
                    {
                        glTexParameteri( GL_TEXTURE_2D,
                                         GL_GENERATE_MIPMAP_SGIS,
                                         TRUE );
                    }

                    glTexImage2D( GL_TEXTURE_2D,
                                  rasterExt->lockedMipLevel,
                                  rasterExt->serverPixelFormat,
                                  topRaster->width,
                                  topRaster->height,
                                  0,
                                  _rwOpenGLRasterFormatInfo[RASTERCFORMAT2GL(topRaster->cFormat)].clientPixelFormat,
                                  GL_UNSIGNED_BYTE,
                                  topRaster->cpPixels );

                    if ( FALSE != rasterExt->hardwareMipGen )
                    {
                        glTexParameteri( GL_TEXTURE_2D,
                                         GL_GENERATE_MIPMAP_SGIS,
                                         FALSE );
                    }

                    GL_ERROR_CHECK();

                    RWASSERT( rasterExt->lockedMipLevel < MAX_MIPLEVELS );
                    rasterExt->canFetchServerData[rasterExt->lockedMipLevel] = TRUE;

                    /* restore the bound texture to re-validate the state cache */
                    glBindTexture( GL_TEXTURE_2D, boundTexId );
                }

                topRaster->width  = topRaster->originalWidth; 
                topRaster->height = topRaster->originalHeight;
                topRaster->stride = topRaster->originalStride;
            }
            break;

        default:
            RWERROR( (E_RW_INVRASTERFORMAT) );
            RWRETURN( FALSE );
    }

    rasterExt->lockedMipLevel = -1;

    RwDriverFree( topRaster->cpPixels );
    topRaster->cpPixels = NULL;
    raster->cpPixels = NULL;

    if ( rwRASTERPIXELLOCKEDWRITE == (raster->privateFlags & rwRASTERPIXELLOCKEDWRITE) )
    {
        raster->privateFlags = raster->privateFlags & ~rwRASTERPIXELLOCKED;

        if ( 0 != (raster->cFormat & (rwRASTERFORMATAUTOMIPMAP>>8)) )
        {
            if ( FALSE == rasterExt->hardwareMipGen )
            {
                RwTextureRasterGenerateMipmaps( raster, NULL );
            }
        }
    }
    else
    {
        raster->privateFlags = raster->privateFlags & ~rwRASTERPIXELLOCKED;
    }

    RWRETURN( TRUE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRasterGetDXTRasterSize
 *
 *  Purpose : Get the size of a DXT compressed raster.
 *
 *  On entry: width - Constant RwUInt32 containing the width of the raster.
 *
 *            height - Constant RwUInt32 containing the height of the raster.
 *
 *            dxtCompressedType - Constant RwUInt32 containing an enumeration
 *            of the DXT compression type.
 *
 *  On exit : RwUInt32 containing the size of the DXT compressed raster
 * ------------------------------------------------------------------------- */
RwUInt32
_rwOpenGLRasterGetDXTRasterSize( const RwUInt32 width,
                                 const RwUInt32 height,
                                 const RwUInt32 dxtCompressedType )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLRasterGetDXTRasterSize" ) );

    RWRETURN( ((width + 3) / 4) * ((height + 3) / 4) * (1 == dxtCompressedType ? 8 : 16) );
}


/** 
 * \ingroup rwrasteropengl
 * \ref RwOpenGLRasterStreamReadDDS creates an \ref RwRaster from a \c DXTn
 * compressed texture contained in a \c .dds file. Prior to calling this
 * function, the \c .dds file must have been opened for reading with the
 * RenderWare Graphics streaming API.
 *
 * This function may also be used to stream cube map rasters.
 *
 * If the current OpenGL system does not support \c DXTn texture compression
 * then the \c DXTn image will be decompressed in software into an 
 * uncompressed \ref RwRaster.
 *
 * Only \c DXT1, \c DXT3 and \c DXT5 compressed textures are supported.
 *
 * This function must be called after \ref RwEngineStart.
 *
 * \param stream Pointer to \ref RwStream that is an open stream
 *        to a \c .dds file.
 *
 * \return Pointer to the \ref RwRaster (cast as a void pointer).
 *
 * \see \ref RwStreamOpen
 */
void *
RwOpenGLRasterStreamReadDDS( RwStream *stream )
{
    const RwUInt32      ddsFileMarker = 0x20534444;

    RwUInt32            fileMarker;

    DDS_HEADER          ddsHeader;

    RwBool              cubeMap;

    RwRasterFormat      mipmapped;

    RwUInt32            numMips;

    RwUInt32            serverFormat;

    RwRaster            *raster;

    _rwOpenGLRasterExt  *rasterExt;

    RwUInt32            idx;

    RwUInt32            dxtCompressedType;


    RWAPIFUNCTION( RWSTRING( "RwOpenGLRasterStreamReadDDS" ) );

    RWASSERT( NULL != stream );

    /* read in the file marker */
    if ( sizeof(RwUInt32) != RwStreamRead( stream, (void *)&fileMarker, sizeof(RwUInt32) ) )
    {
        RWRETURN( (void *)NULL );
    }

    /* check it's a DDS file */
    if ( (RwUInt32)ddsFileMarker != fileMarker )
    {
#if defined(RWDEBUG)
        RWMESSAGE( (RWSTRING("Stream does not contain a 'DDS chunk'")) );
#endif /* defined(RWDEBUG) */

        RWRETURN( (void *)NULL );
    }

    /* read the DDS header */
    if ( sizeof(DDS_HEADER) != RwStreamRead( stream, (void *)&ddsHeader, sizeof(DDS_HEADER) ) )
    {
#if defined(RWDEBUG)
        RWMESSAGE( (RWSTRING("Stream does not contain a DDS header")) );
#endif /* defined(RWDEBUG) */

        RWRETURN( (void *)NULL );
    }

    /* ensure endianness is correct */
    (void)RwMemNative32( &(ddsHeader.dwSize), sizeof(RwUInt32) );
    (void)RwMemNative32( &(ddsHeader.dwFlags), sizeof(RwUInt32) );
    (void)RwMemNative32( &(ddsHeader.dwHeight), sizeof(RwUInt32) );
    (void)RwMemNative32( &(ddsHeader.dwWidth), sizeof(RwUInt32) );
    (void)RwMemNative32( &(ddsHeader.dwPitchOrLinearSize), sizeof(RwUInt32) );
    (void)RwMemNative32( &(ddsHeader.dwMipMapCount), sizeof(RwUInt32) );
    (void)RwMemNative32( &(ddsHeader.ddspf.dwSize), sizeof(RwUInt32) );
    (void)RwMemNative32( &(ddsHeader.ddspf.dwFlags), sizeof(RwUInt32) );
    (void)RwMemNative32( &(ddsHeader.ddspf.dwFourCC), sizeof(RwUInt32) );
    (void)RwMemNative32( &(ddsHeader.ddspf.dwRGBBitCount), sizeof(RwUInt32) );
    (void)RwMemNative32( &(ddsHeader.dwCaps1), sizeof(RwUInt32) );
    (void)RwMemNative32( &(ddsHeader.dwCaps2), sizeof(RwUInt32) );

    if ( 0 == (ddsHeader.dwCaps1 & DDSCAPS_TEXTURE) )
    {
#if defined(RWDEBUG)
        RWMESSAGE( (RWSTRING("DDS file does not contain any textures")) );
#endif /* defined(RWDEBUG) */

        RWRETURN( (void *)NULL );
    }

    if ( DDSCAPS2_VOLUME == (ddsHeader.dwCaps2 & DDSCAPS2_VOLUME) )
    {
#if defined(RWDEBUG)
        RWMESSAGE( (RWSTRING("DDS volume textures are not supported.")) );
#endif /* defined(RWDEBUG) */

        RWRETURN( (void *)NULL );
    }

    /* is it a cubemap? */
    if ( DDSCAPS2_CUBEMAP == (ddsHeader.dwCaps2 & DDSCAPS2_CUBEMAP) )
    {
        if ( FALSE == _rwOpenGLExt.TextureCubeMapARB )
        {
#if defined(RWDEBUG)
            RWMESSAGE( (RWSTRING("System does not support cubemaps. Unable to load a cubemap.")) );
#endif /* defined(RWDEBUG) */

            RWRETURN( (void *)NULL );
        }

        if ( (NULL == _rwOpenGLExt.CompressedTexImage2DARB) ||
             (FALSE == _rwOpenGLExt.TextureCompressionS3TCEXT) )
        {
#if defined(RWDEBUG)
            RWMESSAGE( (RWSTRING("System does not support texture compression. Unable to load a cubemap.")) );
#endif /* defined(RWDEBUG) */

            RWRETURN( (void *)NULL );
        }

        if ( (ddsHeader.dwWidth > _rwOpenGLExt.MaxCubeMapSize) ||
             (ddsHeader.dwHeight > _rwOpenGLExt.MaxCubeMapSize) )
        {
#if defined(RWDEBUG)
            RWMESSAGE( (RWSTRING("System does not support cube maps exceeding a size of %d. Requested size was %d x %d"),
                       _rwOpenGLExt.MaxCubeMapSize, ddsHeader.dwWidth, ddsHeader.dwHeight) );
#endif /* defined(RWDEBUG) */

            RWRETURN( (void *)NULL );
        }
        
        cubeMap = TRUE;

        /* ddsHeader.dwCaps2 contains all the remaining face flags - but check anyway */
        ddsHeader.dwCaps2 &= ~DDSCAPS2_CUBEMAP;

        if ( ddsHeader.dwCaps2 != (DDSCAPS2_CUBEMAP_POSITIVEX |
                                   DDSCAPS2_CUBEMAP_NEGATIVEX |
                                   DDSCAPS2_CUBEMAP_POSITIVEY |
                                   DDSCAPS2_CUBEMAP_NEGATIVEY |
                                   DDSCAPS2_CUBEMAP_POSITIVEZ |
                                   DDSCAPS2_CUBEMAP_NEGATIVEZ) )
        {
#if defined(RWDEBUG)
            RWMESSAGE( (RWSTRING("Cube map does not contain all 6 faces.")) );
#endif /* defined(RWDEBUG) */

            RWRETURN( (void *)NULL );
        }
    }
    else
    {
        cubeMap = FALSE;
    }

    /* is it mipmapped? */
    mipmapped = ((ddsHeader.dwCaps1 & DDSCAPS_MIPMAP) ? rwRASTERFORMATMIPMAP : 0);
    if ( 0 == mipmapped )
    {
        numMips = 1;
    }
    else
    {
        numMips = ddsHeader.dwMipMapCount;
    }

    /* what's the image format? */
    serverFormat = 0;
    dxtCompressedType = 0;
    if ( 0 != (ddsHeader.ddspf.dwFlags & DDS_FOURCC) )
    {
        switch ( ddsHeader.ddspf.dwFourCC )
        {
        case FOURCC_DXT1:
            {
                serverFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                dxtCompressedType = 1;
            }
            break;

        case FOURCC_DXT3:
            {
                serverFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                dxtCompressedType = 3;
            }
            break;

        case FOURCC_DXT5:
            {
                serverFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                dxtCompressedType = 5;
            }
            break;

        default:
            RWRETURN( (void *)NULL );
        }
    }
    else
    {
        RWASSERT( FALSE );
    }

    /* can we load a compressed DXT texture? */
    if ( (NULL == _rwOpenGLExt.CompressedTexImage2DARB) ||
         (FALSE == _rwOpenGLExt.TextureCompressionS3TCEXT) )
    {
        RwImage     **imageMips;

        RwUInt32    rasterWidth;

        RwUInt32    rasterHeight;

        RwUInt32    rasterDepth;

        RwUInt32    rasterFormat;


        imageMips = (RwImage **)RwDriverMalloc( numMips * sizeof(RwImage *), rwID_DRIVERMODULE );
        RWASSERT( NULL != imageMips );
        if ( NULL == imageMips )
        {
            RWRETURN( (void *)NULL );
        }

        /* read all the mip levels into an array of RwImages */
        for ( idx = 0; idx < numMips; idx += 1 )
        {
            RwUInt32    width;

            RwUInt32    height;

            RwUInt8     *imPixels;

            RwUInt32    size;

            RwUInt8     *pixels;


            if ( FALSE == _rwOpenGLDescribeMipLevel( idx,
                                                     ddsHeader.dwWidth,
                                                     ddsHeader.dwHeight,
                                                     0,
                                                     &width,
                                                     &height,
                                                     NULL ) )
            {
                RWRETURN( (void *)NULL );
            }

            imageMips[idx] = RwImageCreate( width, height, 32 );
            RWASSERT( NULL != imageMips[idx] );
            if ( NULL == imageMips[idx] )
            {
                RwUInt32    idx2;


                for ( idx2 = 0; idx2 < idx; idx2 += 1 )
                {
                    RwImageDestroy( imageMips[idx2] );
                }

                RwDriverFree( imageMips );
                RWRETURN( (void *)NULL );
            }

            if ( NULL == RwImageAllocatePixels( imageMips[idx] ) )
            {
                RwUInt32    idx2;


                for ( idx2 = 0; idx2 < idx; idx2 += 1 )
                {
                    RwImageDestroy( imageMips[idx2] );
                }

                RwDriverFree( imageMips );
                RWRETURN( (void *)NULL );
            }

            imPixels = RwImageGetPixels( imageMips[idx] );
            RWASSERT( NULL != imPixels );

            /* calculate the size of the compressed miplevel */
            size = _rwOpenGLRasterGetDXTRasterSize( width, height, dxtCompressedType );

            pixels = (RwUInt8 *)RwDriverMalloc( size, rwID_DRIVERMODULE );
            RWASSERT( NULL != pixels );
            if ( NULL == pixels )
            {
                RwUInt32    idx2;


                for ( idx2 = 0; idx2 < idx; idx2 += 1 )
                {
                    RwImageDestroy( imageMips[idx2] );
                }

                RwDriverFree( imageMips );

                RWERROR( (E_RW_NOMEM, size) );
                RWRETURN( (void *)NULL );
            }

            /* read the mip level */
            if ( size != RwStreamRead( stream,
                                       (void *)pixels,
                                       size ) )
            {
                RwUInt32    idx2;


                for ( idx2 = 0; idx2 < idx; idx2 += 1 )
                {
                    RwImageDestroy( imageMips[idx2] );
                }

                RwDriverFree( imageMips );
                RwDriverFree( pixels );
                RWRETURN( (void *)NULL );
            }

            switch ( dxtCompressedType )
            {
            case 1:
                {
#if defined(RWDEBUG)
                    RWMESSAGE( (RWSTRING("Decompressing DXT1 data into an uncompressed raster...")) );
#endif /* defined(RWDEBUG) */

                    _rwOpenGLDecompressDXT1toARGB( width,
                                                   height,
                                                   imPixels,
                                                   pixels );
                }
                break;

            case 3:
                {
#if defined(RWDEBUG)
                    RWMESSAGE( (RWSTRING("Decompressing DXT3 data into an uncompressed raster...")) );
#endif /* defined(RWDEBUG) */

                    _rwOpenGLDecompressDXT3toARGB( width,
                                                   height,
                                                   imPixels,
                                                   pixels );
                }
                break;

            case 5:
                {
#if defined(RWDEBUG)
                    RWMESSAGE( (RWSTRING("Decompressing DXT5 data into an uncompressed raster...")) );
#endif /* defined(RWDEBUG) */

                    _rwOpenGLDecompressDXT5toARGB( width,
                                                   height,
                                                   imPixels,
                                                   pixels );
                }
                break;
            }

            RwDriverFree( pixels );
        }

        /* find the best format for the raster from the image */
        RwImageFindRasterFormat( imageMips[0],
                                 rwRASTERTYPETEXTURE,
                                 &rasterWidth,
                                 &rasterHeight,
                                 &rasterDepth,
                                 &rasterFormat );

        /* set mipmapping flags */
        if ( numMips > 1 )
        {
            rasterFormat |= rwRASTERFORMATMIPMAP;
            rasterFormat &= ~rwRASTERFORMATAUTOMIPMAP;
        }

        raster = RwRasterCreate( rasterWidth,
                                 rasterHeight,
                                 rasterDepth,
                                 rasterFormat );
        RWASSERT( NULL != raster );
        if ( NULL == raster )
        {
            RwUInt32    idx2;


            for ( idx2 = 0; idx2 < idx; idx2 += 1 )
            {
                RwImageDestroy( imageMips[idx2] );
            }

            RwDriverFree( imageMips );
            RWRETURN( (void *)NULL );
        }

        /* do the top level mips in the image and raster differ in size?
         * if they do, then so will the other mip levels */
        if ( (RwImageGetWidth(imageMips[0]) != RwRasterGetWidth(raster)) ||
             (RwImageGetHeight(imageMips[0]) != RwRasterGetHeight(raster)) )
        {
            RwUInt32    rasterWidth = RwRasterGetWidth(raster);

            RwUInt32    rasterHeight = RwRasterGetHeight(raster);


            for ( idx = 0; idx < numMips; idx += 1 )
            {
                RwImage *resample;


                resample = RwImageCreateResample( imageMips[idx],
                                                  rasterWidth,
                                                  rasterHeight );
                RWASSERT( NULL != resample );
                if ( NULL == resample )
                {
                    RwUInt32    idx2;


                    for ( idx2 = 0; idx2 < idx; idx2 += 1 )
                    {
                        RwImageDestroy( imageMips[idx2] );
                    }

                    RwDriverFree( imageMips );
                    RWRETURN( (void *)NULL );
                }

                RwImageDestroy( imageMips[idx] );
                imageMips[idx] = resample;
            }
        }

        for ( idx = 0; idx < numMips; idx += 1 )
        {
            if ( NULL != RwRasterLock( raster, idx, rwRASTERLOCKWRITE ) )
            {
                RwRasterSetFromImage( raster, imageMips[idx] );
                RwImageDestroy( imageMips[idx] );
                imageMips[idx] = (RwImage *)NULL;
                RwRasterUnlock( raster );
            }
            else
            {
                RwUInt32    idx2;


                for ( idx2 = idx; idx2 < numMips; idx2 += 1 )
                {
                    RwImageDestroy( imageMips[idx2] );
                }

                RwDriverFree( imageMips );
                RWRETURN( (void *)NULL );
            }
        }

        RwDriverFree( imageMips );
    }
    else
    {
        RwInt32 boundTexId;


        /* create the raster */
        raster = RwRasterCreate( ddsHeader.dwWidth,
                                 ddsHeader.dwHeight,
                                 0,
                                 rwRASTERTYPETEXTURE | mipmapped );
        RWASSERT( NULL != raster );
        if ( NULL == raster )
        {
            RWRETURN( (void *)NULL );
        }

        rasterExt = RASTEREXTFROMRASTER( raster );
        RWASSERT( NULL != rasterExt );

        /* adjust the server pixel format */
        rasterExt->serverPixelFormat = serverFormat;
        rasterExt->dxtCompressedType = dxtCompressedType;
        rasterExt->cubeMap           = cubeMap;

        /* save the currently bound texture as we are invalidating the state cache */
        glGetIntegerv( GL_TEXTURE_BINDING_2D, &boundTexId );

        /* are we binding a cubemap or a regular texture? */
        if ( FALSE == cubeMap )
        {
            glBindTexture( GL_TEXTURE_2D, rasterExt->textureID );
        }
        else
        {
            glBindTexture( GL_TEXTURE_CUBE_MAP_ARB, rasterExt->textureID );
        }

        do
        {
            RwUInt32    texUploadTarget = GL_TEXTURE_2D;


            if ( FALSE != cubeMap )
            {
                if ( DDSCAPS2_CUBEMAP_POSITIVEX == (ddsHeader.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEX) )
                {
                    texUploadTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB;
                    ddsHeader.dwCaps2 &= ~DDSCAPS2_CUBEMAP_POSITIVEX;
                }
                else if ( DDSCAPS2_CUBEMAP_NEGATIVEX == (ddsHeader.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEX) )
                {
                    texUploadTarget = GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB;
                    ddsHeader.dwCaps2 &= ~DDSCAPS2_CUBEMAP_NEGATIVEX;
                }
                else if ( DDSCAPS2_CUBEMAP_POSITIVEY == (ddsHeader.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEY) )
                {
                    texUploadTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB;
                    ddsHeader.dwCaps2 &= ~DDSCAPS2_CUBEMAP_POSITIVEY;
                }
                else if ( DDSCAPS2_CUBEMAP_NEGATIVEY == (ddsHeader.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEY) )
                {
                    texUploadTarget = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB;
                    ddsHeader.dwCaps2 &= ~DDSCAPS2_CUBEMAP_NEGATIVEY;
                }
                else if ( DDSCAPS2_CUBEMAP_POSITIVEZ == (ddsHeader.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEZ) )
                {
                    texUploadTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB;
                    ddsHeader.dwCaps2 &= ~DDSCAPS2_CUBEMAP_POSITIVEZ;
                }
                else if ( DDSCAPS2_CUBEMAP_NEGATIVEZ == (ddsHeader.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEZ) )
                {
                    texUploadTarget = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB;
                    ddsHeader.dwCaps2 &= ~DDSCAPS2_CUBEMAP_NEGATIVEZ;
                }
#if defined(RWDEBUG)
                else
                {
                    RWASSERT( 0 == ddsHeader.dwCaps2 );
                    break;
                }
#endif /* defined(RWDEBUG) */
            }

            /* read the pixel data */
            for ( idx = 0; idx < numMips; idx += 1 )
            {
                RwUInt8     *pixelData;

                RwUInt32    width;

                RwUInt32    height;

                RwUInt32    size;


                if ( FALSE == _rwOpenGLDescribeMipLevel( idx,
                                                         ddsHeader.dwWidth,
                                                         ddsHeader.dwHeight,
                                                         0,
                                                         &width,
                                                         &height,
                                                         NULL ) )
                {
                    RwRasterDestroy( raster );

                    RWRETURN( (void *)NULL );
                }

                /* calculate the size of the compressed miplevel */
                size = _rwOpenGLRasterGetDXTRasterSize( width, height, dxtCompressedType );

                /* allocate some temporary storage for the image data */
                pixelData = (RwUInt8 *)RwDriverMalloc( size, rwID_DRIVERMODULE );
                RWASSERT( NULL != pixelData );
                if ( NULL == pixelData )
                {
                    RwRasterDestroy( raster );

                    RWRETURN( (void *)NULL );
                }

                if ( size != RwStreamRead( stream, (void *)pixelData, size ) )
                {
#if defined(RWDEBUG)
                    RWMESSAGE( (RWSTRING("Stream does not contain image mip level %d"), idx) );
#endif /* defined(RWDEBUG) */

                    RwDriverFree( pixelData );
                    RwRasterDestroy( raster );

                    RWRETURN( (void *)NULL );
                }

                _rwOpenGLExt.CompressedTexImage2DARB( texUploadTarget,
                                                      idx,
                                                      rasterExt->serverPixelFormat,
                                                      width,
                                                      height,
                                                      0,
                                                      size,
                                                      pixelData );
                RwDriverFree( pixelData );
            }
        }
        while ( (FALSE != cubeMap) &&
                (0 != ddsHeader.dwCaps2) );

        if ( FALSE != cubeMap )
        {
            glBindTexture( GL_TEXTURE_CUBE_MAP_ARB, 0 );
        }

        /* restore the bound texture to re-validate the state cache */
        glBindTexture( GL_TEXTURE_2D, boundTexId );

        GL_ERROR_CHECK();
    }

    RWRETURN( (void *)raster );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLDDSTextureRead
 *
 *  Purpose : Open a stream containing the .dds file and create a texture from it.
 *
 *  On entry: name - Constant pointer to RwChars containing the name of the texture.
 *
 *            fullPathName - Constant pointer to RwChars containing the full path
 *                           name to the .dds file.
 *
 *  On exit : Pointer to RwTexture containing the texture, or NULL if there was
 *            an error.
 * ------------------------------------------------------------------------- */
static RwTexture *
_rwOpenGLDDSTextureRead( const RwChar *name,
                         const RwChar *fullPathName )
{
    RwStream        *stream;

    RwRaster        *raster;

    RwTexture       *texture;


    RWFUNCTION( RWSTRING( "_rwOpenGLDDSTextureRead" ) );

    RWASSERT( NULL != name );
    RWASSERT( NULL != fullPathName );

    if ( FALSE == RwFexist(fullPathName) )
    {
        RWRETURN( (RwTexture *)NULL );
    }

    stream = RwStreamOpen( rwSTREAMFILENAME, rwSTREAMREAD, fullPathName );
    if ( NULL == stream )
    {
        RWRETURN( (RwTexture *)NULL );
    }

    /* Create raster from stream */
    raster = RwOpenGLRasterStreamReadDDS(stream);

    /* Close the stream */
    RwStreamClose(stream, NULL);

    /* Check raster */
    if ( NULL == raster )
    {
        RWRETURN( (RwTexture *)NULL );
    }

    /* Create the texture */
    texture = RwTextureCreate(raster);
    RWASSERT( NULL != texture );
    if ( NULL == texture )
    {
        RWRETURN( (RwTexture *)NULL );
    }

    if ( rwRASTERFORMATMIPMAP == (RwRasterGetFormat(raster) & rwRASTERFORMATMIPMAP) )
    {
        RwTextureSetFilterMode( texture, rwFILTERLINEARMIPLINEAR );
    }
    else
    {
        RwTextureSetFilterMode( texture, rwFILTERLINEAR );
    }

    /* Set the textures name */
    RwTextureSetName( texture, name );

    RWRETURN( texture );
}


/** 
 * \ingroup rwrasteropengl
 * \ref RwOpenGLDDSTextureRead creates an \ref RwTexture from the image in
 * the \c .dds file specified in \e name. \e maskname is unused but reserved for
 * future compatability.
 *
 * If \e name does not specify an absolute filename then the image path set
 * by \ref RwImageSetPath is used to search for the \c .dds file.
 *
 * This function may also be used to load cube maps into an \ref RwTexture's.
 *
 * If the current OpenGL system does not support \c DXTn texture compression
 * then the \c DXTn image will be decompressed in software into an 
 * uncompressed \ref RwRaster.
 *
 * Only \c DXT1, \c DXT3 and \c DXT5 compressed textures are supported.
 *
 * This function must be called after \ref RwEngineStart.
 *
 * \param name Constant pointer to \ref RwChar's containing the name of the
 *             \c .dds file to read from.
 *
 * \param maskname Unused constant pointer to \ref RwChar's.
 *
 * \return Pointer to the \ref RwTexture (cast as a void pointer).
 *
 * \see \ref RwImageSetPath
 * \see \ref RwOpenGLRasterStreamReadDDS
 */
void *
RwOpenGLDDSTextureRead( const RwChar *name,
                        const RwChar *maskname __RWUNUSED__ )
{
    RwChar      *imagePath;

    RwChar      fullPathName[256];


    RWAPIFUNCTION( RWSTRING( "RwOpenGLDDSTextureRead" ) );

    RWASSERT( NULL != name );

    imagePath = RwImageGetPath();

    /* if we have an absolute path, ignore the search directory.
     * if we have no search path, use the current directory.
     * either way, just copy the name. */
    if ( (FALSE != _rwpathisabsolute(name)) ||
         (NULL == imagePath) ||
         ('\0' == imagePath[0]) )
    {
        const RwChar    *curChar;


        curChar = name + rwstrlen(name);

        do
        {
            curChar--;
        }
        while (curChar >= name &&
               '\\' != *curChar);

        rwstrcpy(fullPathName, name);
        if ( NULL == rwstrstr( name, ".dds" ) )
        {
            rwstrcat(fullPathName, ".dds");
        }

        /* Try to load the texture */
        RWRETURN( _rwOpenGLDDSTextureRead(curChar + 1, fullPathName) );
    }
    else
    {
        /* While we have a search path to try */
        while (imagePath && imagePath[0] != '\0')
        {
            RwChar      *nextPathElement = rwstrchr(imagePath, rwSEARCHPATH_SEPARATOR);

            RwInt32     pathElementLength;

            RwTexture   *texture;


            nextPathElement = rwstrchr(imagePath, rwSEARCHPATH_SEPARATOR);
            if ( NULL != nextPathElement )
            {
                /* Length of pathstring (in bytes) */
                pathElementLength = (RwUInt8 *)nextPathElement - (RwUInt8*)imagePath;
                nextPathElement++;  /* Skip the separator */
            }
            else
            {
                pathElementLength = rwstrlen(imagePath) * sizeof(RwChar);
            }

            /* Full path + full name */
            memcpy(fullPathName, imagePath, pathElementLength);
            rwstrcpy((RwChar *)(((RwUInt8 *)fullPathName) + pathElementLength), name);
            if ( NULL == rwstrstr( name, ".dds" ) )
            {
                rwstrcat(fullPathName, ".dds");
            }

            /* Try to load the texture */
            texture = _rwOpenGLDDSTextureRead(name, fullPathName);
            if ( NULL != texture )
            {
                RWRETURN( texture );
            }

            /* On to the next path */
            imagePath = nextPathElement;
        }
    }

    RWRETURN( NULL );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRasterLockPalette
 *
 *  Purpose : Lock a palette's raster.
 *
 *  On entry: palette - Pointer to RwUInt32 * to return the palette in.
 *
 *            raster - Pointer to RwRaster containing the palette.
 *
 *            accessMode - RwInt32 containing palette access mode.
 *
 *  On exit : FALSE - you can't lock a palette.
 * ------------------------------------------------------------------------- */
RwBool 
_rwOpenGLRasterLockPalette( RwUInt8 **palette __RWUNUSEDRELEASE__,
                            RwRaster *raster __RWUNUSEDRELEASE__,
                            RwInt32 accessMode __RWUNUSED__ )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLRasterLockPalette" ) );

    RWASSERT( NULL != palette );
    RWASSERT( NULL != raster );

    *palette = (RwUInt8 *)NULL;

    RWRETURN( FALSE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRasterUnlockPalette
 *
 *  Purpose : Unlock a palette's raster.
 *
 *  On entry: inOut - Void pointer.
 *
 *            raster - Pointer to RwRaster containing the palette.
 *
 *            flags - RwInt32.
 *
 *  On exit : NULL - you can't lock a palette.
 * ------------------------------------------------------------------------- */
RwBool 
_rwOpenGLRasterUnlockPalette( void *inOut __RWUNUSEDRELEASE__,
                              RwRaster *raster __RWUNUSEDRELEASE__,
                              RwInt32 flags __RWUNUSED__ )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLRasterUnlockPalette" ) );

    RWASSERT( NULL == inOut );
    RWASSERT( NULL != raster );

    RWRETURN( FALSE );
}


/** 
 * \ingroup rwrasteropengl
 * \ref RwOpenGLRasterSetGenericTextureCompression enables or disables the
 * selection of OpenGL generic compressed texture formats for rasters of the
 * type \ref rwRASTERTYPETEXTURE.
 *
 * By default, the selection of compressed formats is \e disabled.
 *
 * This function only has an effect if the \c GL_ARB_texture_compression extension
 * is supported.
 *
 * This function must be called after \ref RwEngineStart.
 *
 * \param enable Constant \ref RwBool containing TRUE to enable compressed
 *        texture format selection, or FALSE to disable it.
 *
 * \see \ref RwRasterCreate
 * \see \ref RwOpenGLRasterGetGenericTextureCompression
 *
 */
void
RwOpenGLRasterSetGenericTextureCompression( const RwBool enable )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLRasterSetGenericTextureCompression" ) );

    if ( NULL != _rwOpenGLExt.CompressedTexImage1DARB )
    {
        _rwOpenGLUseGenericTextureCompression = enable;

#if defined(RWDEBUG)
        if ( FALSE == enable )
        {
            RWMESSAGE( (RWSTRING("Disabling rwRASTERTYPETEXTURE generic texture compression selection.")) );
        }
        else
        {
            RWMESSAGE( (RWSTRING("Enabling rwRASTERTYPETEXTURE generic texture compression selection.")) );
        }
#endif /* defined(RWDEBUG) */
    }
#if defined(RWDEBUG)
    else if ( FALSE != enable )
    {
        RWMESSAGE( (RWSTRING("Texture compression not supported. Unable to select generic compressed formats.")) );
    }
#endif /* defined(RWDEBUG) */

    RWRETURNVOID();
}


/** 
 * \ingroup rwrasteropengl
 * \ref RwOpenGLRasterGetGenericTextureCompression determines whether generic
 * compressed texture formats are selected for new \ref rwRASTERTYPETEXTURE
 * rasters.
 *
 * This function must be called after \ref RwEngineStart.
 *
 * \return \ref RwBool containing TRUE if generic compressed formats will be
 *         selected, or FALSE if uncompressed formats will be selected.
 *
 * \see \ref RwOpenGLRasterSetGenericTextureCompression
 *
 */
RwBool
RwOpenGLRasterGetGenericTextureCompression( void )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLRasterGetGenericTextureCompression" ) );

    RWRETURN( _rwOpenGLUseGenericTextureCompression );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLTextureSetRaster
 *
 *  Purpose : Set a texture's raster.
 *
 *  On entry: texture - Pointer to RwTexture to set the raster for.
 *
 *            raster - Pointer to RwRaster.
 *
 *            flags - RwInt32.
 *
 *  On exit : RwBool, TRUE
 * ------------------------------------------------------------------------- */
RwBool 
_rwOpenGLTextureSetRaster( RwTexture *texture,
                           RwRaster *raster,
                           RwInt32 flags __RWUNUSED__ )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLTextureSetRaster" ) );

    RWASSERT( NULL != texture );
    RWASSERT( NULL != raster );

    texture->raster = raster;

    RWRETURN( TRUE );
}


/** 
 * \ingroup rwrasteropengl
 * \ref RwOpenGLRasterIsCubeMap queries whether an \ref RwRaster is a cube
 * map.
 *
 * \param rasterVoid Void pointer to the \ref RwRaster to query.
 *
 * \return \ref RwBool, containing TRUE if the \ref RwRaster is a cube map
 *         or FALSE if it is not.
 */
RwBool
RwOpenGLRasterIsCubeMap( void *rasterVoid )
{
    RwRaster            *raster;

    _rwOpenGLRasterExt  *rasterExt;


    RWAPIFUNCTION( RWSTRING( "RwOpenGLRasterIsCubeMap" ) );

    RWASSERT( NULL != rasterVoid );

    raster = (RwRaster *)rasterVoid;

    rasterExt = RASTEREXTFROMRASTER( raster );
    RWASSERT( NULL != rasterExt );

    RWRETURN( rasterExt->cubeMap );
}
