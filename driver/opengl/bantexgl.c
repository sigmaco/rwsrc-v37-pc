/***************************************************************************
 *                                                                         *
 * Module  : bantexgl.c                                                    *
 *                                                                         *
 * Purpose : OpenGL texture dictionaries                                   *
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

#include "baogl.h"
#include "bastdogl.h"
#include "drvfns.h"

#include "barastgl.h"
#include "barstate.h"
#include "badxtgl.h"


#define rwOPENGLTEXDICTCHANGEVERSION    (0x37001)

/* OpenGL texture dictionary stream version */
#define rwOPENGLTEXDICTSTREAMVERSION    (0x1)



/****************************************************************************
 Local types
 */

typedef struct _rwOpenGLNativeTexture _rwOpenGLNativeTexture;
struct _rwOpenGLNativeTexture
{
    RwUInt32    version;

    RwUInt32    platform;

    RwUInt32    filterAddressing;

    RwChar      name[rwTEXTUREBASENAMELENGTH];

    RwChar      mask[rwTEXTUREBASENAMELENGTH];
};

typedef struct _rwOpenGLNativeRaster _rwOpenGLNativeRaster;
struct _rwOpenGLNativeRaster
{
    RwUInt32    format;

    RwUInt32    width;

    RwUInt32    height;

    RwUInt32    numMipLevels;

    RwUInt32    dxtCompressedType;     /* 0 = not, 1 = DXT1, 3 = DXT3, 5 = DXT5 */

    RwBool      cubeMap;

    RwBool      hardwareMipGen;
};


/* LEGACY TYPES */


typedef struct
{
    RwInt32  id; /* RwPlatformID,(rwID_PCOGL or rwID_MAC) defined in batype.h */
    RwUInt32 filterMode;
    RwUInt32 addressModeU;
    RwUInt32 addressModeV;
    RwUInt32 unused[4];   /* Future proofing */
}
_rwOpenGLLegacyNativeTexture;

typedef struct
{
    RwChar  name[rwTEXTUREBASENAMELENGTH];
    RwChar  mask[rwTEXTUREBASENAMELENGTH];
}
_rwOpenGLLegacyNativeTextureNames;

typedef struct
{
    RwInt32     format;
    RwUInt16    width;
    RwUInt16    height;
    RwUInt8     depth;
    RwUInt8     numMipLevels;
    RwUInt8     type;
}
_rwOpenGLLegacyNativeRaster;


/* LEGACY FUNCTIONALITY */


/****************************************************************************
 _rwOpenGLLegacyRasterStreamRead

 Stream read an OpenGL raster.

 On entry   : Pointer to RwStream to read raster from.
 On exit    : Pointer to RwRaster read in.
 */
static RwRaster *
_rwOpenGLLegacyRasterStreamRead( RwStream *stream )
{
    _rwOpenGLLegacyNativeRaster nativeRaster;

    RwInt32                     rasterFormat;

    RwRaster                    *raster;

    RwUInt32                    autoMipmap;

    RwUInt32                    i;


    RWFUNCTION( RWSTRING( "_rwOpenGLLegacyRasterStreamRead" ) );

    RWASSERT( NULL != stream );

    if ( sizeof(_rwOpenGLLegacyNativeRaster) !=
                RwStreamRead( stream,
                              (void *)&nativeRaster,
                              sizeof(_rwOpenGLLegacyNativeRaster) ) )
    {
        RWRETURN( (RwRaster *)NULL );
    }

    /* get the best format for the raster */
    rasterFormat = nativeRaster.format;
    if ( nativeRaster.numMipLevels > 1 )
    {
        rasterFormat |= (rwRASTERFORMATAUTOMIPMAP | rwRASTERFORMATMIPMAP);
    }

    /* create the raster */
    raster = RwRasterCreate( nativeRaster.width,
                             nativeRaster.height,
                             (RwInt32)(nativeRaster.depth),
                             nativeRaster.type | rasterFormat );
    if ( NULL == raster )
    {
        RWRETURN( (RwRaster *)NULL );
    }

    /* remove AUTOMIPMAP flag to avoid unlock invoking mipmap creation */
    autoMipmap = raster->cFormat & (rwRASTERFORMATAUTOMIPMAP >> 8);
    raster->cFormat &= ~autoMipmap;

    /* load mips into raster */
    for ( i = 0; i < nativeRaster.numMipLevels; i += 1 )
    {
        RwUInt8     *pixels;

        RwUInt32    size;


        pixels = RwRasterLock( raster, (RwUInt8)i, rwRASTERLOCKWRITE);
        RWASSERT( NULL != pixels );

        /* size, in bytes, of mip to load */
        if ( sizeof(RwUInt32) !=
                RwStreamRead( stream,
                              (void *)&size,
                              sizeof(RwUInt32) ) )
        {
            RWRETURN( (RwRaster *)NULL );
        }

        /* Read the mip level */
        if ( size != RwStreamRead( stream,
                                   (void *)pixels,
                                   size ) )
        {
            RWRETURN( (RwRaster *)NULL );
        }

        RwRasterUnlock( raster );
    }

    /* restore auto mip flag */
    raster->cFormat |= autoMipmap;

    RWRETURN( raster );
}


/****************************************************************************
 _rwOpenGLLegacyNativeTextureRead

 Read a native texture from a stream (legacy code)

 On entry   : Stream handle
            : Pointer to texture handle to fill in
            : Size in the stream to read (well actually the legacy texture)
 On exit    : TRUE on success
 */
static RwBool
_rwOpenGLLegacyNativeTextureRead( RwStream *stream,
                                  RwTexture **texture )
{
    RwRaster                            *raster;

    _rwOpenGLLegacyNativeTexture        nativeTexture;

    _rwOpenGLLegacyNativeTextureNames   nTextureName;


    RWFUNCTION(RWSTRING("_rwOpenGLLegacyNativeTextureRead"));

    RWASSERT( NULL != stream );
    RWASSERT( NULL != texture );

    /* read the native texture */
    if ( sizeof(_rwOpenGLLegacyNativeTexture) !=
                RwStreamRead( stream,
                              (void *)&nativeTexture,
                              sizeof(_rwOpenGLLegacyNativeTexture) ) )
    {
        RWRETURN( FALSE );
    }

    /* legacy check */
    if ( ('O' == (RwUInt8)*( ((RwUInt8 *)&(nativeTexture.id)) + 0)) &&
         ('G' == (RwUInt8)*( ((RwUInt8 *)&(nativeTexture.id)) + 1)) &&
         ('L' == (RwUInt8)*( ((RwUInt8 *)&(nativeTexture.id)) + 2)) &&
         (0   == (RwUInt8)*( ((RwUInt8 *)&(nativeTexture.id)) + 3)) )
    {
#if defined(RWDEBUG)
        RWMESSAGE( (RWSTRING("Texture dictionary binary format is too old to be read. "\
                             "Please regenerate your texture dictionary.")) );
#endif /* defined(RWDEBUG) */

        RWRETURN( FALSE );
    }

#if defined(WIN32)
    RWASSERT( rwID_PCOGL == nativeTexture.id );
#else /* defined(WIN32) */
    RWASSERT( rwID_MAC == nativeTexture.id );
#endif /* defined(WIN32) */

    /* read the native texture names */
    if ( sizeof(_rwOpenGLLegacyNativeTextureNames) !=
                RwStreamRead( stream,
                              (void *)&nTextureName,
                              sizeof(_rwOpenGLLegacyNativeTextureNames) ) )
    {
        RWRETURN( FALSE );
    }

    raster = _rwOpenGLLegacyRasterStreamRead( stream );

    if ( NULL == raster )
    {
        RWRETURN( FALSE );
    }

    *texture = RwTextureCreate( raster );
    if ( NULL == *texture )
    {
        RwRasterDestroy( raster );

        RWRETURN( FALSE );
    }

    (void)RwTextureSetFilterMode( *texture, (nativeTexture.filterMode & 0xFF) );
    (void)RwTextureSetAddressingU( *texture,
                                   (RwTextureAddressMode)nativeTexture.addressModeU );
    (void)RwTextureSetAddressingV( *texture,
                                   (RwTextureAddressMode)nativeTexture.addressModeV );

    RwTextureSetName( *texture, nTextureName.name );
    RwTextureSetMaskName( *texture, nTextureName.mask );

    RWRETURN(TRUE);
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLNativeRasterGetSize
 *
 *  Purpose : OpenGL raster stream sizing
 *
 *  On entry: raster - Pointer to RwRaster to get the size of.
 *
 *  On exit : RwInt32 containing the size of the raster.
 * ------------------------------------------------------------------------- */
static RwInt32
_rwOpenGLNativeRasterGetSize( RwRaster *raster )
{
    RwInt32             size;

    _rwOpenGLRasterExt  *rasterExt;
    
    RwInt32             mipLevel;


    RWFUNCTION( RWSTRING( "_rwOpenGLNativeRasterGetSize" ) );

    RWASSERT( NULL != raster );

    /* initialize size with the raster header */
    size = sizeof(_rwOpenGLNativeRaster);

    rasterExt = RASTEREXTFROMRASTER( raster );
    RWASSERT( NULL != rasterExt );

    if ( 0 == rasterExt->dxtCompressedType )
    {
        /* now calculate the pixel data size */
        for ( mipLevel = 0;
              mipLevel < RwRasterGetNumLevels(raster);
              mipLevel += 1 )
        {
            /* pixel data size */
            size += sizeof(RwInt32);

            /* actual pixel data */
            if ( NULL != RwRasterLock( raster,
                                       (RwUInt8)mipLevel,
                                       rwRASTERLOCKREAD) )
            {
                RwInt32 stride;

                RwInt32 height;


                stride = RwRasterGetStride(raster);
                height = RwRasterGetHeight(raster);

                size += stride * height;

                RwRasterUnlock( raster );
            }
        }
    }
    else
    {
        /* now calculate the pixel data size */
        for ( mipLevel = 0;
              mipLevel < RwRasterGetNumLevels(raster);
              mipLevel += 1 )
        {
            RwUInt32    width;

            RwUInt32    height;

            
            if ( FALSE != _rwOpenGLDescribeMipLevel( mipLevel,
                                                     RwRasterGetWidth( raster ),
                                                     RwRasterGetHeight( raster ),
                                                     0,
                                                     &width,
                                                     &height,
                                                     NULL ) )
            {
                /* don't forget the pixel data size per miplevel */
                if ( FALSE == rasterExt->cubeMap )
                {
                    size += sizeof(RwInt32) +
                            _rwOpenGLRasterGetDXTRasterSize( width,
                                                             height,
                                                             rasterExt->dxtCompressedType );
                }
                else
                {
                    /* all cube faces must be present */
                    size += 6 * 
                            (sizeof(RwInt32) +
                             _rwOpenGLRasterGetDXTRasterSize( width,
                                                              height,
                                                              rasterExt->dxtCompressedType ) );
                }
            }
        }
    }

    RWRETURN( size );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLNativeTextureGetSize
 *
 *  Purpose : OpenGL texture stream sizing
 *
 *  On entry: size - Pointer to RwUInt32 to return the texture size in.
 *
 *            texture - Pointer to RwTexture to get the size of.
 *
 *            flags - RwInt32 (unused)
 *
 *  On exit : RwBool, TRUE if successful, FALSE otherwise.
 * ------------------------------------------------------------------------- */
RwBool 
_rwOpenGLNativeTextureGetSize( RwUInt32 *size,
                               RwTexture *texture,
                               RwInt32 flags __RWUNUSEDRELEASE__ )
{
    RwRaster    *raster;


    RWFUNCTION( RWSTRING( "_rwOpenGLNativeTextureGetSize" ) );

    RWASSERT( NULL != size );
    RWASSERT( NULL != texture );
    RWASSERT( 0 == flags );

    *size = rwCHUNKHEADERSIZE;
    *size += sizeof(_rwOpenGLNativeTexture);

    raster = RwTextureGetRaster( texture );
    RWASSERT( NULL != raster );
    if ( NULL == raster )
    {
        RWRETURN( FALSE );
    }

    *size += _rwOpenGLNativeRasterGetSize( raster );

    RWRETURN( TRUE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLNativeRasterWrite
 *
 *  Purpose : OpenGL raster writing
 *
 *  On entry: stream - Pointer to RwStream to write to.
 *
 *            raster - Pointer to RwRaster to write from.
 *
 *  On exit : RwInt32 containing the number of bytes written.
 * ------------------------------------------------------------------------- */
static RwInt32
_rwOpenGLNativeRasterWrite( RwStream *stream,
                            RwRaster *raster )
{
    RwInt32                 bytesWritten = 0;

    _rwOpenGLNativeRaster   nativeRaster;

    _rwOpenGLRasterExt      *rasterExt;


    RWFUNCTION( RWSTRING( "_rwOpenGLNativeRasterWrite" ) );

    RWASSERT( NULL != stream );
    RWASSERT( NULL != raster );

    /* initialize the native raster structure */
    memset( (void *)&nativeRaster, 0, sizeof(nativeRaster) );

    nativeRaster.format         = (RwRasterGetFormat( raster ) & rwRASTERFORMATPIXELFORMATMASK);
    nativeRaster.width          = RwRasterGetWidth( raster );
    nativeRaster.height         = RwRasterGetHeight( raster );
    nativeRaster.numMipLevels   = RwRasterGetNumLevels( raster );

    rasterExt = RASTEREXTFROMRASTER( raster );
    RWASSERT( NULL != rasterExt );

    nativeRaster.hardwareMipGen = rasterExt->hardwareMipGen;
    nativeRaster.cubeMap        = rasterExt->cubeMap;

    if ( 0 == rasterExt->dxtCompressedType )
    {
        RwUInt32    mipLevel;


        nativeRaster.dxtCompressedType = 0;

        if ( NULL == RwStreamWrite( stream,
                                    (const void *)&nativeRaster,
                                    sizeof(_rwOpenGLNativeRaster) ) )
        {
            RWRETURN( bytesWritten );
        }
        bytesWritten += sizeof(_rwOpenGLNativeRaster);

        /* write all miplevels, regardless of whether they are auto generated */
        for ( mipLevel = 0;
              mipLevel < nativeRaster.numMipLevels;
              mipLevel += 1 )
        {
            RwUInt8 *pixels;

            RwInt32 size;


            pixels = RwRasterLock( raster,
                                   (RwUInt8)mipLevel,
                                   rwRASTERLOCKREAD );

            if ( NULL == pixels )
            {
                RWRETURN( bytesWritten );
            }

            size = RwRasterGetStride(raster) * RwRasterGetHeight(raster);

            /* how big is the raster mip level data? */
            if ( NULL == RwStreamWrite( stream,
                                        &size,
                                        sizeof(RwInt32) ) )
            {
                RWRETURN( bytesWritten );
            }
            bytesWritten += sizeof(RwInt32);

            /* now write the mip level data */
            if ( NULL == RwStreamWrite( stream,
                                        pixels,
                                        size ) )
            {
                RWRETURN( bytesWritten );
            }
            bytesWritten += size;

            RwRasterUnlock( raster );
        }
    }
    else
    {
        RwUInt32    mipLevel;

        RwUInt32    faces = (DDSCAPS2_CUBEMAP_POSITIVEX |
                             DDSCAPS2_CUBEMAP_NEGATIVEX |
                             DDSCAPS2_CUBEMAP_POSITIVEY |
                             DDSCAPS2_CUBEMAP_NEGATIVEY |
                             DDSCAPS2_CUBEMAP_POSITIVEZ |
                             DDSCAPS2_CUBEMAP_NEGATIVEZ);

        RwInt32     size;

        RwUInt8     *pixels;

        RwInt32     boundTexId;


        RWASSERT( (1 == rasterExt->dxtCompressedType) ||
                  (3 == rasterExt->dxtCompressedType) ||
                  (5 == rasterExt->dxtCompressedType) );

        RWASSERT( NULL != _rwOpenGLExt.GetCompressedTexImageARB );

        nativeRaster.dxtCompressedType = rasterExt->dxtCompressedType;

        if ( NULL == RwStreamWrite( stream,
                                    (const void *)&nativeRaster,
                                    sizeof(_rwOpenGLNativeRaster) ) )
        {
            RWRETURN( bytesWritten );
        }
        bytesWritten += sizeof(_rwOpenGLNativeRaster);

        /* allocating enough memory for the top level mip will cover all miplevels and
         * all cube faces */
        size = _rwOpenGLRasterGetDXTRasterSize( nativeRaster.width,
                                                nativeRaster.height,
                                                nativeRaster.dxtCompressedType );

        pixels = (RwUInt8 *)RwDriverMalloc( size, rwID_DRIVERMODULE );
        RWASSERT( NULL != pixels );
        if ( NULL == pixels )
        {
            RWERROR( (E_RW_NOMEM, size) );
            RWRETURN( bytesWritten );
        }

        /* save the currently bound texture as we are invalidating the state cache */
        glGetIntegerv( GL_TEXTURE_BINDING_2D, &boundTexId );

        if ( FALSE != rasterExt->cubeMap )
        {
            glBindTexture( GL_TEXTURE_CUBE_MAP_ARB, rasterExt->textureID );
        }
        else
        {
            glBindTexture( GL_TEXTURE_2D, rasterExt->textureID );
        }
        GL_ERROR_CHECK();

        do
        {
            RwUInt32    texUploadTarget = GL_TEXTURE_2D;


            if ( FALSE != rasterExt->cubeMap )
            {
                if ( DDSCAPS2_CUBEMAP_POSITIVEX == (faces & DDSCAPS2_CUBEMAP_POSITIVEX) )
                {
                    texUploadTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB;
                    faces &= ~DDSCAPS2_CUBEMAP_POSITIVEX;
                }
                else if ( DDSCAPS2_CUBEMAP_NEGATIVEX == (faces & DDSCAPS2_CUBEMAP_NEGATIVEX) )
                {
                    texUploadTarget = GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB;
                    faces &= ~DDSCAPS2_CUBEMAP_NEGATIVEX;
                }
                else if ( DDSCAPS2_CUBEMAP_POSITIVEY == (faces & DDSCAPS2_CUBEMAP_POSITIVEY) )
                {
                    texUploadTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB;
                    faces &= ~DDSCAPS2_CUBEMAP_POSITIVEY;
                }
                else if ( DDSCAPS2_CUBEMAP_NEGATIVEY == (faces & DDSCAPS2_CUBEMAP_NEGATIVEY) )
                {
                    texUploadTarget = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB;
                    faces &= ~DDSCAPS2_CUBEMAP_NEGATIVEY;
                }
                else if ( DDSCAPS2_CUBEMAP_POSITIVEZ == (faces & DDSCAPS2_CUBEMAP_POSITIVEZ) )
                {
                    texUploadTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB;
                    faces &= ~DDSCAPS2_CUBEMAP_POSITIVEZ;
                }
                else if ( DDSCAPS2_CUBEMAP_NEGATIVEZ == (faces & DDSCAPS2_CUBEMAP_NEGATIVEZ) )
                {
                    texUploadTarget = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB;
                    faces &= ~DDSCAPS2_CUBEMAP_NEGATIVEZ;
                }
#if defined(RWDEBUG)
                else
                {
                    RWASSERT( 0 == faces );
                    break;
                }
#endif /* defined(RWDEBUG) */
            }

            /* write all miplevels, regardless of whether they are auto generated */
            for ( mipLevel = 0;
                  mipLevel < nativeRaster.numMipLevels;
                  mipLevel += 1 )
            {
                RwUInt32    width;

                RwUInt32    height;


                if ( FALSE == _rwOpenGLDescribeMipLevel( mipLevel,
                                                         nativeRaster.width,
                                                         nativeRaster.height,
                                                         0,
                                                         &width,
                                                         &height,
                                                         NULL ) )
                {
                    /* restore the bound texture to re-validate the state cache */
                    glBindTexture( GL_TEXTURE_2D, boundTexId );

                    RWERROR( (E_RW_INVRASTERMIPLEVEL) );
                    RWRETURN( bytesWritten );
                }

                size = _rwOpenGLRasterGetDXTRasterSize( width,
                                                        height,
                                                        nativeRaster.dxtCompressedType );

                _rwOpenGLExt.GetCompressedTexImageARB( texUploadTarget,
                                                       mipLevel,
                                                       (void *)pixels );
                GL_ERROR_CHECK();

                /* how big is the raster mip level data? */
                if ( NULL == RwStreamWrite( stream,
                                            &size,
                                            sizeof(RwInt32) ) )
                {
                    /* restore the bound texture to re-validate the state cache */
                    glBindTexture( GL_TEXTURE_2D, boundTexId );

                    RWRETURN( bytesWritten );
                }
                bytesWritten += sizeof(RwInt32);

                /* now write the mip level data */
                if ( NULL == RwStreamWrite( stream,
                                            pixels,
                                            size ) )
                {
                    /* restore the bound texture to re-validate the state cache */
                    glBindTexture( GL_TEXTURE_2D, boundTexId );

                    RWRETURN( bytesWritten );
                }
                bytesWritten += size;
            }
        }
        while ( (FALSE != rasterExt->cubeMap) &&
                (0 != faces) );

        RwDriverFree( pixels );

        /* restore the bound texture to re-validate the state cache */
        glBindTexture( GL_TEXTURE_2D, boundTexId );
    }

    RWRETURN( bytesWritten );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLNativeTextureWrite
 *
 *  Purpose : OpenGL texture writing
 *
 *  On entry: stream - Pointer to RwStream to write to.
 *
 *            texture - Pointer to RwTexture to write from.
 *
 *            length - RwUInt32 (unused)
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
RwBool 
_rwOpenGLNativeTextureWrite( RwStream *stream,
                             RwTexture *texture,
                             RwUInt32 length __RWUNUSED__ )
{
    RwInt32                 bytesRemaining;

    _rwOpenGLNativeTexture  nativeTexture;

    RwRaster                *raster;


    RWFUNCTION( RWSTRING( "_rwOpenGLNativeTextureWrite" ) );

    RWASSERT( NULL != stream );
    RWASSERT( NULL != texture );

    /* calculate the number of writes that must be written
     * minus the chunk header */
    _rwOpenGLNativeTextureGetSize( &bytesRemaining,
                                   texture,
                                   0 );
    bytesRemaining -= rwCHUNKHEADERSIZE;

    /* struct header for _rwOpenGLNativeTexture structure */
    if ( NULL == RwStreamWriteChunkHeader( stream,
                                           rwID_STRUCT,
                                           bytesRemaining ) )
    {
        RWRETURN( FALSE );
    }

    /* initialize the native texture structure */
    memset( (void *)&nativeTexture, 0, sizeof(_rwOpenGLNativeTexture) );

    /* what's the stream version of this texture dictionary */
    nativeTexture.version = rwOPENGLTEXDICTSTREAMVERSION;

    /* which OpenGL platform does it support */
#if defined(WIN32)
    nativeTexture.platform = rwID_PCOGL;
#elif defined(MACOS)
    nativeTexture.platform = rwID_MAC;
#else
#error "Undefined OpenGL platform"
#endif /* defined(WIN32) || defined(MACOS) */

    /* encoded filter and addressing modes */
    nativeTexture.filterAddressing =
        (((RwInt32)RwTextureGetFilterMode(texture)) & 0xFF) |
        ((((RwInt32)RwTextureGetAddressingU(texture)) << 8) & 0x0F00) |
        ((((RwInt32)RwTextureGetAddressingV(texture)) << 12) & 0xF000);

    /* texture name and mask name */
    rwstrcpy( nativeTexture.name, RwTextureGetName( texture ) );
    rwstrcpy( nativeTexture.mask, RwTextureGetMaskName( texture ) );

    /* write the OpenGL texture header */
    if ( NULL == RwStreamWrite( stream,
                                (const void *)&nativeTexture,
                                sizeof(_rwOpenGLNativeTexture) ) )
    {
        RWRETURN( FALSE );
    }
    bytesRemaining -= sizeof(_rwOpenGLNativeTexture);

    /* write the raster */
    raster = RwTextureGetRaster( texture );
    if ( NULL == raster )
    {
        RWRETURN( FALSE );
    }

    bytesRemaining -= _rwOpenGLNativeRasterWrite( stream, raster );

    RWASSERT( 0 == bytesRemaining );

    RWRETURN( TRUE );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLNativeRasterRead
 *
 *  Purpose : OpenGL raster reading
 *
 *  On entry: stream - Pointer to RwStream to read from.
 *
 *  On exit : Pointer to RwRaster read from the stream.
 * ------------------------------------------------------------------------- */
static RwRaster *
_rwOpenGLNativeRasterRead( RwStream *stream )
{
    _rwOpenGLNativeRaster   nativeRaster;

    RwInt32                 rasterFormat;

    RwRaster                *raster;

    RwUInt32                i;


    RWFUNCTION( RWSTRING( "_rwOpenGLNativeRasterRead" ) );

    RWASSERT( NULL != stream );

    if ( sizeof(_rwOpenGLNativeRaster) != RwStreamRead( stream,
                                                        (void *)&nativeRaster,
                                                        sizeof(_rwOpenGLNativeRaster) ) )
    {
        RWRETURN( (RwRaster *)NULL );
    }

    /* check for cube map support */
    if ( FALSE != nativeRaster.cubeMap )
    {
        if ( FALSE == _rwOpenGLExt.TextureCubeMapARB )
        {
#if defined(RWDEBUG)
            RWMESSAGE( (RWSTRING("Cubemap raster requires cube map support. "\
                                 "Failing to read, to force texture dictionary regeneration.")) );
#endif /* defined(RWDEBUG) */

            RWRETURN( (RwRaster *)NULL );
        }

        if ( (0 != nativeRaster.dxtCompressedType) &&
             ((NULL == _rwOpenGLExt.CompressedTexImage2DARB) ||
              (FALSE == _rwOpenGLExt.TextureCompressionS3TCEXT)) )
        {
#if defined(RWDEBUG)
            RWMESSAGE( (RWSTRING("Cubemap raster requires compressed texture support. "\
                                 "Failing to read, to force texture dictionary regeneration.")) );
#endif /* defined(RWDEBUG) */

            RWRETURN( (RwRaster *)NULL );
        }

        if ( (nativeRaster.width > _rwOpenGLExt.MaxCubeMapSize) ||
             (nativeRaster.height > _rwOpenGLExt.MaxCubeMapSize) )
        {
#if defined(RWDEBUG)
            RWMESSAGE( (RWSTRING("Cubemap raster dimensions exceed system limitations. "\
                                 "(%dx%d > %d) Failing to read, to force texture dictionary regeneration."),
                       _rwOpenGLExt.MaxCubeMapSize, nativeRaster.width, nativeRaster.height) );
#endif /* defined(RWDEBUG) */

            RWRETURN( (void *)NULL );
        }
    }

    /* get the best format for the raster */
    rasterFormat = nativeRaster.format;
    if ( nativeRaster.numMipLevels > 1 )
    {
        rasterFormat |= rwRASTERFORMATMIPMAP;
    }
    else if ( FALSE != nativeRaster.hardwareMipGen )
    {
        if ( FALSE == _rwOpenGLExt.GenerateMipmapSGIS )
        {
#if defined(RWDEBUG)
            RWMESSAGE( (RWSTRING("Raster requires hardware mipmap generation. "\
                                 "Failing to read, to force texture dictionary regeneration.")) );
#endif /* defined(RWDEBUG) */

            RWRETURN( (RwRaster *)NULL );
        }

        rasterFormat |= (rwRASTERFORMATMIPMAP | rwRASTERFORMATAUTOMIPMAP);
    }

    if ( 0 == nativeRaster.dxtCompressedType )
    {
        /* create the raster */
        raster = RwRasterCreate( nativeRaster.width,
                                 nativeRaster.height,
                                 0,
                                 rwRASTERTYPETEXTURE | rasterFormat );
        if ( NULL == raster )
        {
            RWRETURN( (RwRaster *)NULL );
        }

        /* load mips into raster */
        for ( i = 0; i < nativeRaster.numMipLevels; i += 1 )
        {
            RwUInt8     *pixels;

            RwUInt32    size;


            pixels = RwRasterLock( raster, (RwUInt8)i, rwRASTERLOCKWRITE);
            RWASSERT( NULL != pixels );

            /* size, in bytes, of mip to load */
            if ( sizeof(RwUInt32) != RwStreamRead( stream,
                                                   (void *)&size,
                                                   sizeof(RwUInt32) ) )
            {
                RwRasterDestroy( raster );
                RWRETURN( (RwRaster *)NULL );
            }

            /* Read the mip level */
            if ( size != RwStreamRead( stream,
                                       (void *)pixels,
                                       size ) )
            {
                RwRasterDestroy( raster );
                RWRETURN( (RwRaster *)NULL );
            }

            RwRasterUnlock( raster );
        }
    }
    else
    {
        _rwOpenGLRasterExt  *rasterExt;
    
        RwUInt32    faces = (DDSCAPS2_CUBEMAP_POSITIVEX |
                             DDSCAPS2_CUBEMAP_NEGATIVEX |
                             DDSCAPS2_CUBEMAP_POSITIVEY |
                             DDSCAPS2_CUBEMAP_NEGATIVEY |
                             DDSCAPS2_CUBEMAP_POSITIVEZ |
                             DDSCAPS2_CUBEMAP_NEGATIVEZ);

        RwUInt32    size;

        RwUInt8     *pixels;

        RwInt32     boundTexId;


        if ( FALSE == _rwOpenGLExt.TextureCompressionS3TCEXT )
        {
#if defined(RWDEBUG)
            RWMESSAGE( (RWSTRING("Raster requires DXTn texture compression. "\
                                 "Failing to read, to force texture dictionary regeneration.")) );
#endif /* defined(RWDEBUG) */

            RWRETURN( (RwRaster *)NULL );
        }

        /* create the raster */
        raster = RwRasterCreate( nativeRaster.width,
                                 nativeRaster.height,
                                 0,
                                 rwRASTERTYPETEXTURE | rasterFormat );
        if ( NULL == raster )
        {
            RWRETURN( (RwRaster *)NULL );
        }

        rasterExt = RASTEREXTFROMRASTER( raster );
        RWASSERT( NULL != rasterExt );

        switch ( nativeRaster.dxtCompressedType )
        {
        case 1:
            rasterExt->serverPixelFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            break;

        case 3:
            rasterExt->serverPixelFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            break;

        case 5:
            rasterExt->serverPixelFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            break;

        default:
            RwRasterDestroy( raster );
            RWRETURN( (RwRaster *)NULL );
        }

        rasterExt->dxtCompressedType = nativeRaster.dxtCompressedType;
        rasterExt->cubeMap           = nativeRaster.cubeMap;

        /* allocating enough memory for the top level mip will cover all miplevels and
         * all cube faces */
        size = _rwOpenGLRasterGetDXTRasterSize( nativeRaster.width,
                                                nativeRaster.height,
                                                nativeRaster.dxtCompressedType );

        pixels = (RwUInt8 *)RwDriverMalloc( size, rwID_DRIVERMODULE );
        RWASSERT( NULL != pixels );
        if ( NULL == pixels )
        {
            RwRasterDestroy( raster );

            RWERROR( (E_RW_NOMEM, size) );
            RWRETURN( (RwRaster *)NULL );
        }

        /* save the currently bound texture as we are invalidating the state cache */
        glGetIntegerv( GL_TEXTURE_BINDING_2D, &boundTexId );

        if ( FALSE == rasterExt->cubeMap )
        {
            glBindTexture( GL_TEXTURE_2D, rasterExt->textureID );
        }
        else
        {
            glBindTexture( GL_TEXTURE_CUBE_MAP_ARB, rasterExt->textureID );
        }
        GL_ERROR_CHECK();

        do
        {
            RwUInt32    texUploadTarget = GL_TEXTURE_2D;


            if ( FALSE != rasterExt->cubeMap )
            {
                if ( DDSCAPS2_CUBEMAP_POSITIVEX == (faces & DDSCAPS2_CUBEMAP_POSITIVEX) )
                {
                    texUploadTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB;
                    faces &= ~DDSCAPS2_CUBEMAP_POSITIVEX;
                }
                else if ( DDSCAPS2_CUBEMAP_NEGATIVEX == (faces & DDSCAPS2_CUBEMAP_NEGATIVEX) )
                {
                    texUploadTarget = GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB;
                    faces &= ~DDSCAPS2_CUBEMAP_NEGATIVEX;
                }
                else if ( DDSCAPS2_CUBEMAP_POSITIVEY == (faces & DDSCAPS2_CUBEMAP_POSITIVEY) )
                {
                    texUploadTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB;
                    faces &= ~DDSCAPS2_CUBEMAP_POSITIVEY;
                }
                else if ( DDSCAPS2_CUBEMAP_NEGATIVEY == (faces & DDSCAPS2_CUBEMAP_NEGATIVEY) )
                {
                    texUploadTarget = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB;
                    faces &= ~DDSCAPS2_CUBEMAP_NEGATIVEY;
                }
                else if ( DDSCAPS2_CUBEMAP_POSITIVEZ == (faces & DDSCAPS2_CUBEMAP_POSITIVEZ) )
                {
                    texUploadTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB;
                    faces &= ~DDSCAPS2_CUBEMAP_POSITIVEZ;
                }
                else if ( DDSCAPS2_CUBEMAP_NEGATIVEZ == (faces & DDSCAPS2_CUBEMAP_NEGATIVEZ) )
                {
                    texUploadTarget = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB;
                    faces &= ~DDSCAPS2_CUBEMAP_NEGATIVEZ;
                }
#if defined(RWDEBUG)
                else
                {
                    RWASSERT( 0 == faces );
                    break;
                }
#endif /* defined(RWDEBUG) */
            }

            /* load mips into raster */
            for ( i = 0; i < nativeRaster.numMipLevels; i += 1 )
            {
                RwUInt32    width;

                RwUInt32    height;


                if ( FALSE == _rwOpenGLDescribeMipLevel( i,
                                                         nativeRaster.width,
                                                         nativeRaster.height,
                                                         0,
                                                         &width,
                                                         &height,
                                                         NULL ) )
                {
                    /* restore the bound texture to re-validate the state cache */
                    glBindTexture( GL_TEXTURE_2D, boundTexId );

                    RWERROR( (E_RW_INVRASTERMIPLEVEL) );
                    RwRasterDestroy( raster );
                    RWRETURN( (RwRaster *)NULL );
                }

                /* size, in bytes, of mip to load */
                if ( sizeof(RwUInt32) != RwStreamRead( stream,
                                                       (void *)&size,
                                                       sizeof(RwUInt32) ) )
                {
                    /* restore the bound texture to re-validate the state cache */
                    glBindTexture( GL_TEXTURE_2D, boundTexId );

                    RwRasterDestroy( raster );
                    RWRETURN( (RwRaster *)NULL );
                }

#if defined(RWDEBUG)
                RWASSERT( size == _rwOpenGLRasterGetDXTRasterSize( width,
                                                                   height,
                                                                   nativeRaster.dxtCompressedType ) );
#endif /* defined(RWDEBUG) */

                /* Read the mip level */
                if ( size != RwStreamRead( stream,
                                           (void *)pixels,
                                           size ) )
                {
                    /* restore the bound texture to re-validate the state cache */
                    glBindTexture( GL_TEXTURE_2D, boundTexId );

                    RwRasterDestroy( raster );
                    RWRETURN( (RwRaster *)NULL );
                }

                _rwOpenGLExt.CompressedTexImage2DARB( texUploadTarget,
                                                      i,
                                                      rasterExt->serverPixelFormat,
                                                      width,
                                                      height,
                                                      0,
                                                      size,
                                                      pixels );
                GL_ERROR_CHECK();
            }
        }
        while ( (FALSE != rasterExt->cubeMap) &&
                (0 != faces) );

        RwDriverFree( pixels );
 
        if ( FALSE != rasterExt->cubeMap )
        {
            glBindTexture( GL_TEXTURE_CUBE_MAP_ARB, rasterExt->textureID );
        }
        GL_ERROR_CHECK();

        /* restore the bound texture to re-validate the state cache */
        glBindTexture( GL_TEXTURE_2D, boundTexId );
    }

    RWRETURN( raster );
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLNativeTextureRead
 *
 *  Purpose : OpenGL texture reading
 *
 *  On entry: stream - Pointer to RwStream to read from.
 *
 *            texture - Double pointer to RwTexture to read into.
 *
 *            size - RwUInt32 (unused)
 *
 *  On exit : RwBool, TRUE on success, FALSE otherwise.
 * ------------------------------------------------------------------------- */
RwBool 
_rwOpenGLNativeTextureRead( RwStream *stream,
                            RwTexture **texture,
                            RwUInt32 size __RWUNUSED__ )
{
    RwUInt32                length;

    RwUInt32                version;

    _rwOpenGLNativeTexture  nativeTexture;

    RwRaster                *raster = (RwRaster *)NULL;


    RWFUNCTION( RWSTRING( "_rwOpenGLNativeTextureRead" ) );

    RWASSERT( NULL != stream );
    RWASSERT( NULL != texture );

    if ( FALSE == RwStreamFindChunk( stream,
                                     rwID_STRUCT,
                                     &length,
                                     &version ) )
    {
        RWRETURN( FALSE );
    }

    RWASSERT( version >= rwLIBRARYBASEVERSION );
    RWASSERT( version <= rwLIBRARYCURRENTVERSION );
    if ( FALSE == ((version >= rwLIBRARYBASEVERSION) &&
                   (version <= rwLIBRARYCURRENTVERSION)) )
    {
        RWRETURN( FALSE );
    }

    if ( version < rwOPENGLTEXDICTCHANGEVERSION )
    {
#if defined(RWDEBUG)
        RWMESSAGE( (RWSTRING("Legacy OpenGL texture dictionary streaming...")) );
#endif /* defined(RWDEBUG) */

        RWRETURN( _rwOpenGLLegacyNativeTextureRead( stream, texture ) );
    }

    /* read the native texture */
    if ( sizeof(_rwOpenGLNativeTexture) != RwStreamRead( stream,
                                                         (void *)&nativeTexture,
                                                         sizeof(_rwOpenGLNativeTexture) ) )
    {
        RWRETURN( FALSE );
    }

    if ( rwOPENGLTEXDICTSTREAMVERSION == nativeTexture.version )
    {
        raster = _rwOpenGLNativeRasterRead( stream );
    }
    else
    {
#if defined(RWDEBUG)
        RWMESSAGE( (RWSTRING("OpenGL texture dictionary stream version unknown")) );
#endif /* defined(RWDEBUG) */

        RWRETURN( FALSE );
    }

    RWASSERT( NULL != raster );
    if ( NULL == raster )
    {
        RWRETURN( FALSE );
    }

    *texture = RwTextureCreate( raster );
    if ( NULL == *texture )
    {
        RwRasterDestroy( raster );

        RWRETURN( FALSE );
    }

    (void)RwTextureSetFilterMode( *texture, nativeTexture.filterAddressing & 0xFF );
    (void)RwTextureSetAddressingU( *texture, (nativeTexture.filterAddressing >> 8) & 0x0F );
    (void)RwTextureSetAddressingV( *texture, (nativeTexture.filterAddressing >> 12) & 0x0F );
    RwTextureSetName( *texture, nativeTexture.name );
    RwTextureSetMaskName( *texture, nativeTexture.mask );

    RWRETURN( TRUE );
}

