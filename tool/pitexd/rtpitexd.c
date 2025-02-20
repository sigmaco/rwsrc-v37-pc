/***********************************************************************
 *
 * Module:  rtpitexd.c
 *
 * Purpose: Platform Independent Texture Dictionaries
 *
 ***********************************************************************/

/* =====================================================================
 *  Includes
 * ===================================================================== */
#include <string.h>

#include <rwcore.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rtpitexd.h"


/**
 * \ingroup rtpitexd
 * \page rtpitexdoverview RtPITexD Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rtpitexd.h
 * \li \b Libraries: rwcore, rtpitexd
 * \li \b Plugin \b attachments: None
 *
 * \subsection pitexdoverview Overview
 * The RtPITexD toolkit provides reading and writing functions for Platform
 * Independent Texture Dictionaries.
 *
 * A RenderWare Graphics \ref RwTexDictionary contains platform specific
 * data for every texture that it holds. Thus, when such an object is
 * streamed to disk it will no longer be valid on other platforms.
 *
 * To aid portability, this toolkit can be used to stream platform
 * \e independent texture dictionaries to disk. These write \ref RwImage's
 * rather than platform specific data.
 *
 * When a platform independent texture dictionary is streamed out to disk, all
 * mipmap levels for each \ref RwTexture are also streamed, each as \ref
 * RwImage's. Similarly, all mipmap levels are streamed in when the platform
 * independent texture dictionary is read. If only \e one miplevel for a texture
 * is stored in the platform independent texture dictionary \e and the texture has
 * a \e mipped filter mode, then automipmapping is automatically enabled for the
 * platform specific texture raster generation. This is useful for platforms that
 * can automatically generate miplevels in hardware.
 *
 * In addition, gamma correction is removed from the images before streaming
 * out to disk, and re-applied when the platform independent texture dictionary
 * is streamed back in.
 *
 * As of RenderWare Graphics 3.5, texture plugin extension data is also streamed
 * with each \ref RwTexture. When streaming in a platform independent texture
 * dictionary, it is therefore required to have previously attached all plugins
 * that have extension data in the textures, otherwise the extension data will
 * be lost. Legacy platform independent texture dictionaries can still be
 * streamed in.
 *
 * There is a limit of 65536 textures in any one platform independent texture
 * dictionary.
 *
 * To distinguish between platform specific and platform independent texture
 * dictionaries, the chunk header of an \ref RwStream must be identified:
 *
 * \li Platform specific texture dictionaries are be identified with
 * rwID_TEXDICTIONARY.
 *
 * \li Platform independent texture dictionaries are identified with
 * rwID_PITEXDICTIONARY.

 * The toolkit API mirrors that for streaming platform specific texture
 * dictionaries.
 *
 */


/* =====================================================================
 *  Defines
 * ===================================================================== */
#define _RTPITEXDICT_CURRENTVERSION     (0x1)


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */
typedef struct rtpitexdChunkPrefix rtpitexdChunkPrefix;
struct rtpitexdChunkPrefix
{
    RwUInt16    numTextures;    /* number of textures in the dictionary */

    RwUInt16    versionNum;     /* version number of the dictionary */
};

typedef RwInt32     rtpitexdMipLevels;

typedef RwUInt32    rtpitexdTextureFlagsLegacy;

typedef struct rtpitexdTexturePrefixLegacy rtpitexdTexturePrefixLegacy;
struct rtpitexdTexturePrefixLegacy
{
    RwChar  name[rwTEXTUREBASENAMELENGTH];  /* name of the texture */

    RwChar  mask[rwTEXTUREBASENAMELENGTH];  /* name of the texture mask */

    rtpitexdMipLevels   numMipLevels;       /* number of miplevels in the texture */

    rtpitexdTextureFlagsLegacy    flags;          /* texture flags */
};

typedef struct rtpitexdTexturePrefix rtpitexdTexturePrefix;
struct rtpitexdTexturePrefix
{
    rtpitexdMipLevels   numMipLevels;       /* number of miplevels in the texture */
};

/* Texture stream format - from babintex.c */
typedef struct _rwStreamTexture rwStreamTexture;
struct _rwStreamTexture
{
    RwUInt32            filterAndAddress;
};


/* =====================================================================
 *  Static variables
 * ===================================================================== */


/* =====================================================================
 *  Global variables
 * ===================================================================== */
#if (defined( RWDEBUG ) && !defined( RPERROR_H ))

long rtPITexDictStackDepth = 0;

#endif /* (defined( RWDEBUG ) && !defined( RPERROR_H )) */


/* =====================================================================
 *  Static function prototypes
 * ===================================================================== */
static RwTexture *
rtpitexdTextureCountCallBack( RwTexture *texture,
                              void *data );

static RwTexture *
rtpitexdTextureSizeCallBack( RwTexture *texture,
                             void *data );

static void
rtpitexdImageGammaUncorrect( RwImage *image );

static RwTexture *
rtpitexdTexture2ImageWriteCallBack( RwTexture *texture,
                                    void *data );

static RwTexDictionary *
rtpitexdImage2TextureReadLegacy( RwStream *stream,
                                 RwTexDictionary **texDict );

static RwTexture *
rtpitexdTextureStreamReadPre( RwStream *stream );

static RwTexture *
rtpitexdTextureStreamReadPost( RwStream *stream,
                               RwTexture *texture );

static RwBool
rtpitexdImage2TextureRead( RwStream *stream );

static RwImage **
rtpitexdCreateImageFromRaster( RwRaster *raster,
                               rtpitexdMipLevels numMipLevels,
                               RwUInt32 *imageDepth );


/* =====================================================================
 *  Static function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: rtpitexdTextureCountCallBack
 *
 *  Purpose : Callback function to count the number of textures in a
 *            dictionary.
 *
 *  On entry: texture - Pointer to the current RwTexture
 *            data    - Void pointer to user data (address of RwUInt32
 *                      counter variable)
 *
 *  On exit : Pointer to the RwTexture passed in.
 * --------------------------------------------------------------------- */
static RwTexture *
rtpitexdTextureCountCallBack( RwTexture *texture,
                              void *data )
{
    RwUInt32    *texCount;


    RWFUNCTION( RWSTRING( "rtpitexdTextureCountCallBack" ) );

    RWASSERT( NULL != texture );
    RWASSERT( NULL != data );

    texCount = (RwUInt32 *)data;
    *texCount += 1;

    RWRETURN( texture );
}


/* ---------------------------------------------------------------------
 *  Function: rtpitexdTextureSizeCallBack
 *
 *  Purpose : Callback function to determine the chunk size for the
 *            platform independent texture dictionary.
 *
 *  On entry: texture - Pointer to the current RwTexture
 *            data    - Void pointer to user data (address of RwUInt32
 *                      the cumulative chunksize variable)
 *
 *  On exit : Pointer to the RwTexture passed in, or NULL if there was
 *            an error.
 * --------------------------------------------------------------------- */
static RwTexture *
rtpitexdTextureSizeCallBack( RwTexture *texture,
                             void *data )
{
    RwUInt32                *chunkSize;

    RwRaster                *texRaster;

    rtpitexdMipLevels       numMipLevels;

    rtpitexdMipLevels       curMipLevel;

    RwImage                 **imageMip;


    RWFUNCTION( RWSTRING( "rtpitexdTextureSizeCallBack" ) );

    RWASSERT( NULL != texture );
    RWASSERT( NULL != data );

    chunkSize = (RwUInt32 *)data;

    /* store the texture prefix */
    *chunkSize += sizeof( rtpitexdTexturePrefix );

    /* get the raster that the texture references */
    texRaster = RwTextureGetRaster( texture );
    RWASSERT( NULL != texRaster );
    if ( NULL == texRaster )
    {
        /* error! where's the raster? */
        *chunkSize = 0;
        RWRETURN( NULL );
    }

    numMipLevels = RwRasterGetNumLevels( texRaster );
    RWASSERT( -1 != numMipLevels );
    if ( -1 == numMipLevels )
    {
        RWERROR((E_RW_INVRASTERMIPLEVEL));
        *chunkSize = 0;
        RWRETURN( NULL );
    }

    /* create an array of RwImage's large enough to store all the
     * mip-levels of the texture
     * - NULL last parameter means don't return the raster depth
     * AND don't set the RwImage data from the raster miplevel
     */
    imageMip = rtpitexdCreateImageFromRaster( texRaster, numMipLevels, NULL );
    RWASSERT( NULL != imageMip );
    if ( NULL == imageMip )
    {
        /* error! */
        *chunkSize = 0;
        RWRETURN( NULL );
    }

    /* get the sizes of the RwImage mip-levels and add it to the chunksize */
    for ( curMipLevel = 0; curMipLevel < numMipLevels; curMipLevel += 1 )
    {
        *chunkSize += RwImageStreamGetSize( imageMip[curMipLevel] ) + rwCHUNKHEADERSIZE;
        RwImageDestroy( imageMip[curMipLevel] );
    }

    /* deallocate the image miplevel array */
    RwFree( imageMip );

    /* texture data */
    *chunkSize += RwTextureStreamGetSize( texture ) + rwCHUNKHEADERSIZE;

    RWRETURN( texture );
}


/* ---------------------------------------------------------------------
 *  Function: rtpitexdImageGammaUncorrect
 *
 *  Purpose : Remove gamma correction from an RwImage
 *
 *  On entry: image - RwImage to remove gamma correction from
 *
 *  On exit : Nothing
 * --------------------------------------------------------------------- */
static void
rtpitexdImageGammaUncorrect( RwImage *image )
{
    RWFUNCTION( RWSTRING( "rtpitexdImageGammaUncorrect" ) );

    RWASSERT( NULL != image );

    switch ( image->depth )
    {
    case 4:
    case 8:
        {
            RwRGBA      *palette;

            RwUInt32    palSize;


            /* just process the palette */
            palette = image->palette;

            palSize = (1 << image->depth);

            if ( NULL != palette )
            {
                _rwImageGammaUnCorrectArrayOfRGBA( palette, palette, palSize );
            }
        }
        break;

    case 32:
        {
            RwUInt8     *pixelLine;

            RwInt32     height;

            RwInt32     width;

            RwInt32     stride;

            RwInt32     row;


            pixelLine = image->cpPixels;

            height = image->height;

            width = image->width;

            stride = image->stride;

            for ( row = 0; row < height; row += 1 )
            {
                _rwImageGammaUnCorrectArrayOfRGBA( (RwRGBA *)pixelLine,
                                                   (RwRGBA *)pixelLine,
                                                   width );

                pixelLine += stride;
            }
        }
        break;

    default:
        RWERROR( (E_RW_INVIMAGEDEPTH) );
        RWRETURNVOID();
    }

    /* remove the gamma corrected flag for this image */
    image->flags &= ~rwIMAGEGAMMACORRECTED;

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: rtpitexdTexture2ImageWriteCallBack
 *
 *  Purpose : Callback function to write out RwImages that correspond to
 *            the RwTexture's in the texture dictionary
 *
 *  On entry: texture - Pointer to the current RwTexture
 *            data    - Void pointer to user data (the RwStream pointer)
 *
 *  On exit : Pointer to the RwTexture passed in, or NULL if there was
 *            an error.
 * --------------------------------------------------------------------- */
static RwTexture *
rtpitexdTexture2ImageWriteCallBack( RwTexture *texture,
                                    void *data )
{
    RwStream                *stream;

    rtpitexdTexturePrefix   texturePrefix;

    RwRaster                *texRaster;

    rtpitexdMipLevels       numMipLevels;

    rtpitexdMipLevels       curMipLevel;

    RwImage                 **imageMip;

    RwUInt32                imageDepth;


    RWFUNCTION( RWSTRING( "rtpitexdTexture2ImageWriteCallBack" ) );

    RWASSERT( NULL != texture );
    RWASSERT( NULL != data );

    stream = (RwStream *)data;

    texRaster = RwTextureGetRaster( texture );
    RWASSERT( NULL != texRaster );
    if ( NULL == texRaster )
    {
        /* error! */
        RWRETURN( (RwTexture *)NULL );
    }

    /* get the number of miplevels */
    numMipLevels = RwRasterGetNumLevels( texRaster );
    RWASSERT( -1 != numMipLevels );
    if ( -1 == numMipLevels )
    {
        /* error! */
        RWRETURN( (RwTexture *)NULL );
    }

    /* Convert to network format */
    texturePrefix.numMipLevels = numMipLevels;
    (void)RwMemLittleEndian32(&texturePrefix.numMipLevels, sizeof(texturePrefix.numMipLevels));

    /* write out the texture prefix */
    if ( NULL == RwStreamWrite( stream,
                                &texturePrefix,
                                sizeof( texturePrefix ) ) )
    {
        /* error! */
        RWRETURN( (RwTexture *)NULL );
    }

#if defined( RWDEBUG )
    RWMESSAGE( (RWSTRING("Wrote texture prefix to the stream: %d miplevels"),
               texturePrefix.numMipLevels ) );
#endif /* defined( RWDEBUG ) */


    /* create an array of RwImage's large enough to store all the
     * mip-levels of the texture
     * - non-NULL last parameter means return the raster depth
     * AND set the RwImage data from the raster miplevel
     */
    imageMip = rtpitexdCreateImageFromRaster( texRaster, numMipLevels, &imageDepth );

    /* write the RwImages to the stream */
    for ( curMipLevel = 0; curMipLevel < numMipLevels; curMipLevel += 1 )
    {
        /* we do not want to store gamma corrected images */
        if ( 0 != (imageMip[curMipLevel]->flags & rwIMAGEGAMMACORRECTED) )
        {
            rtpitexdImageGammaUncorrect( imageMip[curMipLevel] );
        }

        if ( NULL == RwImageStreamWrite( imageMip[curMipLevel], stream ) )
        {
            /* error! */
            RwFree( imageMip );
            RWRETURN( (RwTexture *)NULL );
        }
        RwImageDestroy( imageMip[curMipLevel] );
    }

    RwFree( imageMip );

#if defined( RWDEBUG )
    RWMESSAGE( (RWSTRING("Wrote %d RwImage miplevels to the stream"), numMipLevels) );
#endif /* defined( RWDEBUG ) */

    /* stream the texture data */
    /* this includes...
     * filtering, addressing and mipmapping flags
     * name and mask name
     */
    if ( NULL == RwTextureStreamWrite( texture, stream ) )
    {
        /* error! */
        RWRETURN( (RwTexture *)NULL );
    }

#if defined( RWDEBUG )
    {
        RwChar                      name[rwTEXTUREBASENAMELENGTH];

        RwChar                      mask[rwTEXTUREBASENAMELENGTH];

        rtpitexdTextureFlagsLegacy  flags;

        RwTextureAddressMode        texAddrU;

        RwTextureAddressMode        texAddrV;


        /* get the texture and mask names */
        rwstrcpy( (RwChar *)name, RwTextureGetName( texture ) );
        rwstrcpy( (RwChar *)mask, RwTextureGetMaskName( texture ) );

        /* get the texture flags (filter and addressing) */
        flags = (RwTextureGetFilterMode( texture ) & rwTEXTUREFILTERMODEMASK);

        texAddrU = RwTextureGetAddressingU( texture );
        texAddrV = RwTextureGetAddressingV( texture );

        flags |= ((texAddrU << 8) & rwTEXTUREADDRESSINGUMASK);
        flags |= ((texAddrV << 12) & rwTEXTUREADDRESSINGVMASK);

        RWMESSAGE( (RWSTRING("Wrote out texture data: name = '%s' mask = '%s' Flags = 0x%x"),
                             name, mask, flags) );
    }
#endif /* defined( RWDEBUG ) */

    RWRETURN( (RwTexture *)texture );
}


/* ---------------------------------------------------------------------
 *  Function: rtpitexdImage2TextureReadLegacy
 *
 *  Purpose : Function that is called per RwImage in a PI texture dictionary
 *            converting it to an RwTexture and adding it to a PS texture
 *            dictionary.
 *
 *  On entry: stream  - Pointer to RwStream open to read RwImage's from.
 *            texDict - Double pointer to RwTexDictionary to add RwTexture's to.
 *
 *  On exit : Pointer to the RwTexDictionary or NULL if there was an error.
 * --------------------------------------------------------------------- */
static RwTexDictionary *
rtpitexdImage2TextureReadLegacy( RwStream *stream,
                                 RwTexDictionary **texDict )
{
    rtpitexdTexturePrefixLegacy   texturePrefix;

    RwImage                 **imageMip;

    rtpitexdMipLevels       curMipLevel;

    RwUInt32                imageWidth;

    RwUInt32                imageHeight;

    RwUInt32                imageDepth;

    RwUInt32                rasterFlags;

    RwUInt32                rasterFormat;

    RwRaster                *imRaster;

    RwUInt32                autoMipmap = FALSE;

    rtpitexdMipLevels       rasterMipLevels;

    RwTexture               *texture;

    RwTextureAddressMode    texAddrU;

    RwTextureAddressMode    texAddrV;


    RWFUNCTION( RWSTRING( "rtpitexdImage2TextureReadLegacy" ) );

    RWASSERT( NULL != stream );
    RWASSERT( NULL != texDict );

    /* read in the texture prefix */
    if ( sizeof( texturePrefix ) != RwStreamRead( stream,
                                                  &texturePrefix,
                                                  sizeof( texturePrefix ) ) )
    {
        /* error! */
        RWRETURN( (RwTexDictionary *)NULL );
    }

    /* Convert from network format */
    (void)RwMemNative32(&texturePrefix.numMipLevels, sizeof(texturePrefix.numMipLevels));
    (void)RwMemNative32(&texturePrefix.flags, sizeof(texturePrefix.flags));

#if defined( RWDEBUG )
    RWMESSAGE( (RWSTRING("Read in texture prefix: name = '%s' mask = '%s' Num mip levels = %d Flags = 0x%x"),
                texturePrefix.name, texturePrefix.mask, texturePrefix.numMipLevels, texturePrefix.flags) );
#endif /* defined( RWDEBUG ) */

    /* create an array of RwImage pointers large enough to
     * cater for all the miplevels of the texture
     */
    imageMip = (RwImage **)RwMalloc( texturePrefix.numMipLevels * sizeof(RwImage *),
                                   rwID_PITEXDICTIONARYTK | rwMEMHINTDUR_FUNCTION );
    RWASSERT( NULL != imageMip );
    if ( NULL == imageMip )
    {
        RWERROR( (E_RW_NOMEM, texturePrefix.numMipLevels * sizeof(RwImage *)) );
        RWRETURN( (RwTexDictionary *)NULL );
    }

    /* read in the RwImage's from the stream */
    for ( curMipLevel = 0; curMipLevel < texturePrefix.numMipLevels; curMipLevel += 1 )
    {
        if ( FALSE == RwStreamFindChunk( stream,
                                         rwID_IMAGE,
                                         NULL,
                                         NULL ) )
        {
            /* error! */
            RWRETURN( (RwTexDictionary *)NULL );
        }

        imageMip[curMipLevel] = RwImageStreamRead( stream );
        RWASSERT( NULL != imageMip[curMipLevel] );
        if ( NULL == imageMip[curMipLevel] )
        {
            /* error! */
            RWRETURN( (RwTexDictionary *)NULL );
        }

        /* reintroduce gamma correction since the saved image does not have it */
        if ( 0 == (imageMip[curMipLevel]->flags & rwIMAGEGAMMACORRECTED) )
        {
            RwImageGammaCorrect( imageMip[curMipLevel] );
        }
    }

#if defined( RWDEBUG )
    RWMESSAGE( (RWSTRING("Read %d RwImage miplevels from the stream"), texturePrefix.numMipLevels) );
#endif /* defined( RWDEBUG ) */

    /* get some details from the top-mip RwImage */
    imageWidth  = RwImageGetWidth( imageMip[0] );
    imageHeight = RwImageGetHeight( imageMip[0] );
    imageDepth  = RwImageGetDepth( imageMip[0] );

    rasterFlags = rwRASTERTYPETEXTURE;
    if ( texturePrefix.numMipLevels > 1 )
    {
        rasterFlags |= rwRASTERFORMATMIPMAP;
    }
    else
    {
        RwTextureFilterMode     textureFilter;


        textureFilter = (texturePrefix.flags & rwTEXTUREFILTERMODEMASK);

        if ( (textureFilter != rwFILTERNEAREST) &&
             (textureFilter != rwFILTERLINEAR) )
        {
#if defined( RWDEBUG )
            RWMESSAGE( (RWSTRING("Texture has ONE miplevel but a mipped texture filter set.\n")\
                        RWSTRING("\tSetting rwRASTERFORMATAUTOMIPMAP flag. ")\
                        RWSTRING("This may not be the desired result for platforms that ")\
                        RWSTRING("do not support hardware mip generation.")) );
#endif /* defined( RWDEBUG ) */

            rasterFlags |= rwRASTERFORMATMIPMAP | rwRASTERFORMATAUTOMIPMAP;
        }
    }

    /* now choose a format */
    RwImageFindRasterFormat( imageMip[0],
                             (RwInt32)rasterFlags,
                             (RwInt32 *)&imageWidth,
                             (RwInt32 *)&imageHeight,
                             (RwInt32 *)&imageDepth,
                             (RwInt32 *)&rasterFormat );

    imRaster = RwRasterCreate( imageWidth,
                               imageHeight,
                               imageDepth,
                               rasterFlags | rasterFormat );

    RWASSERT( NULL != imRaster );
    if ( NULL == imRaster )
    {
        /* error! */
        for ( curMipLevel = 0; curMipLevel < texturePrefix.numMipLevels; curMipLevel += 1 )
        {
            RwImageDestroy( imageMip[curMipLevel] );
        }
        RwFree( imageMip );

        RWRETURN( (RwTexDictionary *)NULL );
    }

    /* ensure that the RwImage and RwRaster have the same dimensions */
    if ( (RwImageGetWidth(imageMip[0])  != RwRasterGetWidth(imRaster) ) ||
         (RwImageGetHeight(imageMip[0]) != RwRasterGetHeight(imRaster) ) )
    {
        RwInt32 rasterWidth;

        RwInt32 rasterHeight;

#if defined( RWDEBUG )
        RWMESSAGE( (RWSTRING("Image and Raster have different sizes")) );
#endif /* defined( RWDEBUG ) */

        rasterWidth = RwRasterGetWidth( imRaster );
        rasterHeight = RwRasterGetHeight( imRaster );

        if (imageDepth == 8)
        {
            RwRasterDestroy( imRaster );

            imRaster = RwRasterCreate( rasterWidth,
                                       rasterHeight,
                                       32,
                                       rasterFlags | rwRASTERFORMAT8888 );

            if ( NULL == imRaster )
            {
                imRaster = RwRasterCreate( rasterWidth,
                                           rasterHeight,
                                           16,
                                           rasterFlags | rwRASTERFORMAT1555 );
            }
        }

        /* now resample the RwImages to the required size */
        rasterMipLevels = RwRasterGetNumLevels( imRaster );
        for ( curMipLevel = 0; curMipLevel < rasterMipLevels; curMipLevel += 1 )
        {
            RwImage *tempImage;


            tempImage = RwImageCreateResample( imageMip[curMipLevel],
                                               rasterWidth,
                                               rasterHeight );
            RWASSERT( NULL != tempImage );
            if ( NULL == tempImage )
            {
                /* error! */
                RWRETURN( (RwTexDictionary *)NULL );
            }

            RwImageDestroy( imageMip[curMipLevel] );

            imageMip[curMipLevel] = tempImage;

            /* rescale width and height for mipmaps */
            if ( rasterWidth > 1 )
            {
                rasterWidth >>= 1;
            }

            if ( rasterHeight > 1 )
            {
                rasterHeight >>= 1;
            }
        }
    }

    if ( texturePrefix.numMipLevels > 1 )
    {
        /* remove AUTOMIPMAP flag to avoid unlock invoking mipmap creation
         * but only when explicit miplevels exist */
        autoMipmap = imRaster->cFormat & (rwRASTERFORMATAUTOMIPMAP >> 8);
        imRaster->cFormat &= ~autoMipmap;
    }

    rasterMipLevels = RwRasterGetNumLevels( imRaster );
    for ( curMipLevel = 0; curMipLevel < rasterMipLevels; curMipLevel += 1 )
    {
        /* ensure we don't exceed the maximum */
        if ( curMipLevel == texturePrefix.numMipLevels )
        {
            break;
        }

        if ( NULL == RwRasterLock( imRaster,
                                   (RwUInt8)curMipLevel,
                                   rwRASTERLOCKWRITE ) )
        {
            /* error! */
            RWRETURN( (RwTexDictionary *)NULL );
        }

        if ( NULL == RwRasterSetFromImage( imRaster,
                                           imageMip[curMipLevel] ) )
        {
            /* error! */
            RWRETURN( (RwTexDictionary *)NULL );
        }

        if ( NULL == RwRasterUnlock( imRaster ) )
        {
            /* error! */
            RWRETURN( (RwTexDictionary *)NULL );
        }

        RwImageDestroy( imageMip[curMipLevel] );
    }

    if ( texturePrefix.numMipLevels > 1 )
    {
        /* restore auto mip flag */
        imRaster->cFormat |= autoMipmap;
    }

    /* tidy up any spare RwImage mip levels */
    for ( ; curMipLevel < texturePrefix.numMipLevels; curMipLevel += 1 )
    {
        RwImageDestroy( imageMip[curMipLevel] );
    }

    /* and finally the array of pointers */
    RwFree( imageMip );


    /* now create the texture from the raster */
    texture = RwTextureCreate( imRaster );
    RWASSERT( NULL != texture );
    if ( NULL == texture )
    {
        /* error! */
        RWRETURN( (RwTexDictionary *)NULL );
    }

    /* set the texture details */
    RwTextureSetName( texture, texturePrefix.name );
    RwTextureSetMaskName( texture, texturePrefix.mask );
    RwTextureSetFilterMode( texture,
                            (texturePrefix.flags & rwTEXTUREFILTERMODEMASK) );

    texAddrU = ((texturePrefix.flags & rwTEXTUREADDRESSINGUMASK) >> 8);
    texAddrV = ((texturePrefix.flags & rwTEXTUREADDRESSINGVMASK) >> 12);

    if ( (rwTEXTUREADDRESSNATEXTUREADDRESS == texAddrU) &&
         (rwTEXTUREADDRESSNATEXTUREADDRESS == texAddrV) )
    {
#if defined( RWDEBUG )
        RWMESSAGE( (RWSTRING("Texture addressing for both U and V was invalid. ")\
                    RWSTRING("Defaulting both to rwTEXTUREADDRESSWRAP.")) );
#endif /* defined( RWDEBUG ) */

        texAddrU = texAddrV = rwTEXTUREADDRESSWRAP;
    }
    else if ( (rwTEXTUREADDRESSNATEXTUREADDRESS == texAddrU) &&
              (rwTEXTUREADDRESSNATEXTUREADDRESS != texAddrV) )
    {
#if defined( RWDEBUG )
        RWMESSAGE( (RWSTRING("Texture addressing for U was invalid. ")\
                    RWSTRING("Setting U addressing equal to V addressing.")) );
#endif /* defined( RWDEBUG ) */

        texAddrU = texAddrV;
    }
    else if ( (rwTEXTUREADDRESSNATEXTUREADDRESS != texAddrU) &&
              (rwTEXTUREADDRESSNATEXTUREADDRESS == texAddrV) )
    {
#if defined( RWDEBUG )
        RWMESSAGE( (RWSTRING("Texture addressing for V was invalid. ")\
                    RWSTRING("Setting V addressing equal to U addressing.")) );
#endif /* defined( RWDEBUG ) */

        texAddrV = texAddrU;
    }

    RwTextureSetAddressingU( texture, texAddrU );
    RwTextureSetAddressingV( texture, texAddrV );

    /* add the texture to the dictionary */
    RwTexDictionaryAddTexture( *texDict, texture );

    RWRETURN( *texDict );
}


/* ---------------------------------------------------------------------
 *  Function: rtpitexdTextureStreamReadPre
 *
 *  Purpose : Pre-load an RwTexture from a stream BUT do NOT load any
 *            texture plugin extension data.
 *
 *  On entry: stream - Pointer to RwStream containing the stream to read
 *                     the texture from.
 *
 *  On exit : Pointer to the RwTexture read, or NULL if there was an error.
 * --------------------------------------------------------------------- */
static RwTexture *
rtpitexdTextureStreamReadPre( RwStream *stream )
{
    RwUInt32    size;

    RwUInt32    version;


    RWFUNCTION( RWSTRING( "rtpitexdTextureStreamReadPre" ) );

    RWASSERT( NULL != stream );

    if ( FALSE == RwStreamFindChunk(stream, rwID_STRUCT, &size, &version) )
    {
        RWRETURN( (RwTexture *)NULL );
    }

    if ( (version >= rwLIBRARYBASEVERSION) &&
         (version <= rwLIBRARYCURRENTVERSION) )
    {
        RwTexture               *texture;
        RwChar                  textureName[rwTEXTUREBASENAMELENGTH*4];
        RwChar                  textureMask[rwTEXTUREBASENAMELENGTH*4];
        RwTextureFilterMode     filtering;
        RwTextureAddressMode    addressingU;
        RwTextureAddressMode    addressingV;
        rwStreamTexture         texFiltAddr;
        RwBool                  mipmapState;
        RwBool                  autoMipmapState;
        RwTextureStreamFlags    flags;

        /* Read the filtering mode */
        RWASSERT(size <= sizeof(texFiltAddr));
        memset(&texFiltAddr, 0, sizeof(texFiltAddr));
        if (RwStreamRead(stream, &texFiltAddr, size) != size)
        {
            RWRETURN((RwTexture *)NULL);
        }

        /* Convert it */
        (void)RwMemNative32(&texFiltAddr, sizeof(texFiltAddr));

        /* Extract filtering */
        filtering = (RwTextureFilterMode)
                    (texFiltAddr.filterAndAddress & rwTEXTUREFILTERMODEMASK);

        /* Extract addressing */
        addressingU = (RwTextureAddressMode)
                      ((texFiltAddr.filterAndAddress >> 8) & 0x0F);

        addressingV = (RwTextureAddressMode)
                      ((texFiltAddr.filterAndAddress >> 12) & 0x0F);

        /* Make sure addressingV is valid so files old than 3.04 still work */
        if (addressingV == rwTEXTUREADDRESSNATEXTUREADDRESS)
        {
            addressingV = addressingU;
            texFiltAddr.filterAndAddress |= ((addressingV & 0xF) << 12);
        }

        /* Extract user mipmap flags */
        flags = (RwTextureStreamFlags)((texFiltAddr.filterAndAddress >> 16) & 0xFF);

        mipmapState = RwTextureGetMipmapping();
        autoMipmapState = RwTextureGetAutoMipmapping();

        /* Use it */
        if ((filtering == rwFILTERMIPNEAREST) ||
            (filtering == rwFILTERMIPLINEAR) ||
            (filtering == rwFILTERLINEARMIPNEAREST) ||
            (filtering == rwFILTERLINEARMIPLINEAR))
        {
            /* Lets mip map it */
            RwTextureSetMipmapping(TRUE);
            if (flags & rwTEXTURESTREAMFLAGSUSERMIPMAPS)
            {
                RwTextureSetAutoMipmapping(FALSE);
            }
            else
            {
                RwTextureSetAutoMipmapping(TRUE);
            }
        }
        else
        {
            /* Lets not */
            RwTextureSetMipmapping(FALSE);
            RwTextureSetAutoMipmapping(FALSE);
        }

        /* Search for a string or a unicode string */
        if (!_rwStringStreamFindAndRead(textureName, stream))
        {
            RwTextureSetMipmapping(mipmapState);
            RwTextureSetAutoMipmapping(autoMipmapState);

            RWRETURN((RwTexture *)NULL);
        }

        /* Search for a string or a unicode string */
        if (!_rwStringStreamFindAndRead(textureMask,stream))
        {
            RwTextureSetMipmapping(mipmapState);
            RwTextureSetAutoMipmapping(autoMipmapState);
            RWRETURN((RwTexture *)NULL);
        }

        /* Get the textures */
        texture = RwTextureCreate( NULL );
        if ( NULL == texture )
        {
            /* Skip any extension chunks */
            _rwPluginRegistrySkipDataChunks(&textureTKList, stream);

            RwTextureSetMipmapping(mipmapState);
            RwTextureSetAutoMipmapping(autoMipmapState);

            RWRETURN( (RwTexture *)NULL );
        }

        RwTextureSetName( texture, textureName );
        if ( NULL != textureMask )
        {
            RwTextureSetMaskName( texture, textureMask );
        }
        else
        {
            RwTextureSetMaskName( texture, RWSTRING("") );
        }

        /* Set the filtering and addressing */
        /* By testing the reference count here,
         * we can tell if we just loaded it!!! */

        RWASSERT(0 < texture->refCount);

        if (texture->refCount == 1)
        {
            texture->filterAddressing = texFiltAddr.filterAndAddress &
                (rwTEXTUREFILTERMODEMASK | rwTEXTUREADDRESSINGMASK);
        }

        /* clean up */
        RwTextureSetMipmapping(mipmapState);
        RwTextureSetAutoMipmapping(autoMipmapState);

        RWRETURN(texture);
    }
    else
    {
        RWERROR((E_RW_BADVERSION));
        RWRETURN((RwTexture *)NULL);
    }
}


/* ---------------------------------------------------------------------
 *  Function: rtpitexdTextureStreamReadPost
 *
 *  Purpose : Post-load an RwTexture's texture plugin extension data and
 *            add it to the current texture dictionary.
 *
 *  On entry: stream - Pointer to RwStream containing the stream to read
 *                     the texture extension data from.
 *
 *  On exit : Pointer to the RwTexture read, or NULL if there was an error.
 * --------------------------------------------------------------------- */
static RwTexture *
rtpitexdTextureStreamReadPost( RwStream *stream,
                               RwTexture *texture )
{
    RwTexDictionary *curTexDict;


    RWFUNCTION( RWSTRING( "rtpitexdTextureStreamReadPost" ) );

    /* Read the extension chunks */
    if (!_rwPluginRegistryReadDataChunks(&textureTKList, stream, texture))
    {
        RWRETURN((RwTexture *)NULL);
    }

    /* add the texture to the current dictionary */
    curTexDict = RwTexDictionaryGetCurrent();
    if ( NULL != curTexDict )
    {
        RwTexDictionaryAddTexture( curTexDict,
                                   texture );
    }

    RWRETURN( texture );
}


/* ---------------------------------------------------------------------
 *  Function: rtpitexdImage2TextureRead
 *
 *  Purpose : Function that is called per RwImage in a PI texture dictionary
 *            converting it to an RwTexture and adding it to a PS texture
 *            dictionary.
 *
 *  On entry: stream  - Pointer to RwStream open to read RwImage's from.
 *
 *  On exit : RwBool, TRUE if successful, FALSE otherwise.
 * --------------------------------------------------------------------- */
static RwBool
rtpitexdImage2TextureRead( RwStream *stream )
{
    rtpitexdTexturePrefix   texturePrefix;

    RwImage                 **imageMip;

    rtpitexdMipLevels       curMipLevel;

    RwUInt32                imageWidth;

    RwUInt32                imageHeight;

    RwUInt32                imageDepth;

    RwUInt32                rasterFlags;

    RwUInt32                rasterFormat;

    RwRaster                *imRaster;

    RwUInt32                autoMipmap = FALSE;

    rtpitexdMipLevels       rasterMipLevels;

    RwTexture               *texture;


    RWFUNCTION( RWSTRING( "rtpitexdImage2TextureRead" ) );

    RWASSERT( NULL != stream );

    /* read in the texture prefix */
    if ( sizeof( texturePrefix ) != RwStreamRead( stream,
                                                  &texturePrefix,
                                                  sizeof( texturePrefix ) ) )
    {
        /* error! */
        RWRETURN( FALSE );
    }

    /* Convert from network format */
    (void)RwMemNative32(&texturePrefix.numMipLevels, sizeof(texturePrefix.numMipLevels));

    /* create an array of RwImage pointers large enough to
     * cater for all the miplevels of the texture
     */
    imageMip = (RwImage **)RwMalloc( texturePrefix.numMipLevels * sizeof(RwImage *),
                                     rwID_PITEXDICTIONARYTK | rwMEMHINTDUR_FUNCTION );
    RWASSERT( NULL != imageMip );
    if ( NULL == imageMip )
    {
        RWERROR( (E_RW_NOMEM, texturePrefix.numMipLevels * sizeof(RwImage *)) );
        RWRETURN( FALSE );
    }

    /* read in the RwImage's from the stream */
    for ( curMipLevel = 0; curMipLevel < texturePrefix.numMipLevels; curMipLevel += 1 )
    {
        if ( FALSE == RwStreamFindChunk( stream,
                                         rwID_IMAGE,
                                         NULL,
                                         NULL ) )
        {
            /* error! */
            RWRETURN( FALSE );
        }

        imageMip[curMipLevel] = RwImageStreamRead( stream );
        RWASSERT( NULL != imageMip[curMipLevel] );
        if ( NULL == imageMip[curMipLevel] )
        {
#if defined( RWDEBUG )
            RWMESSAGE( (RWSTRING("Failed to read the %i (zero based) mip level"), curMipLevel) );
#endif /* defined( RWDEBUG ) */
            /* error! */
            RWRETURN( FALSE );
        }

        /* reintroduce gamma correction since the saved image does not have it */
        if ( 0 == (imageMip[curMipLevel]->flags & rwIMAGEGAMMACORRECTED) )
        {
            RwImageGammaCorrect( imageMip[curMipLevel] );
        }
    }

    /* now stream the texture data but don't assign any raster to it yet */
    if ( FALSE == RwStreamFindChunk( stream,
                                     rwID_TEXTURE,
                                     NULL, NULL ) )
    {
        /* error! */
        RWRETURN( FALSE );
    }

    texture = rtpitexdTextureStreamReadPre( stream );
    if ( NULL == texture )
    {
        /* error! */
        RWRETURN( FALSE );
    }

    /* get some details from the top-mip RwImage */
    imageWidth  = RwImageGetWidth( imageMip[0] );
    imageHeight = RwImageGetHeight( imageMip[0] );
    imageDepth  = RwImageGetDepth( imageMip[0] );

    rasterFlags = rwRASTERTYPETEXTURE;
    if ( texturePrefix.numMipLevels > 1 )
    {
        rasterFlags |= rwRASTERFORMATMIPMAP;
    }
    else
    {
        RwTextureFilterMode     textureFilter;


        textureFilter = RwTextureGetFilterMode( texture );

        if ( (textureFilter != rwFILTERNEAREST) &&
             (textureFilter != rwFILTERLINEAR) )
        {
#if defined( RWDEBUG )
            RWMESSAGE( (RWSTRING("Texture has ONE miplevel but a mipped texture filter set.\n")\
                        RWSTRING("\tSetting rwRASTERFORMATAUTOMIPMAP flag. ")\
                        RWSTRING("This may not be the desired result for platforms that ")\
                        RWSTRING("do not support hardware mip generation.")) );
#endif /* defined( RWDEBUG ) */

            rasterFlags |= rwRASTERFORMATMIPMAP | rwRASTERFORMATAUTOMIPMAP;
        }
    }

    /* now choose a format */
    RwImageFindRasterFormat( imageMip[0],
                             (RwInt32)rasterFlags,
                             (RwInt32 *)&imageWidth,
                             (RwInt32 *)&imageHeight,
                             (RwInt32 *)&imageDepth,
                             (RwInt32 *)&rasterFormat );

    imRaster = RwRasterCreate( imageWidth,
                               imageHeight,
                               imageDepth,
                               rasterFlags | rasterFormat );

    RWASSERT( NULL != imRaster );
    if ( NULL == imRaster )
    {
        /* error! */
        for ( curMipLevel = 0; curMipLevel < texturePrefix.numMipLevels; curMipLevel += 1 )
        {
            RwImageDestroy( imageMip[curMipLevel] );
        }
        RwFree( imageMip );

        RWRETURN( FALSE );
    }

    /* ensure that the RwImage and RwRaster have the same dimensions */
    if ( (RwImageGetWidth(imageMip[0])  != RwRasterGetWidth(imRaster) ) ||
         (RwImageGetHeight(imageMip[0]) != RwRasterGetHeight(imRaster) ) )
    {
        RwInt32 rasterWidth;

        RwInt32 rasterHeight;

#if defined( RWDEBUG )
        RWMESSAGE( (RWSTRING("Image and Raster have different sizes")) );
#endif /* defined( RWDEBUG ) */

        rasterWidth = RwRasterGetWidth( imRaster );
        rasterHeight = RwRasterGetHeight( imRaster );

        if (imageDepth == 8)
        {
            RwRasterDestroy( imRaster );

            imRaster = RwRasterCreate( rasterWidth,
                                       rasterHeight,
                                       32,
                                       rasterFlags | rwRASTERFORMAT8888 );

            if ( NULL == imRaster )
            {
                imRaster = RwRasterCreate( rasterWidth,
                                           rasterHeight,
                                           16,
                                           rasterFlags | rwRASTERFORMAT1555 );
            }
        }

        /* now resample the RwImages to the required size */
        rasterMipLevels = RwRasterGetNumLevels( imRaster );
        for ( curMipLevel = 0;
			// 마고자 (2005-06-03 오후 3:20:28) : 
			// 밉맵 레벨이 이상한 경우가 있어서 프로텍션
			curMipLevel < ( rasterMipLevels < texturePrefix.numMipLevels ? rasterMipLevels : texturePrefix.numMipLevels ) ;
			curMipLevel += 1 )
        {
            RwImage *tempImage;


            tempImage = RwImageCreateResample( imageMip[curMipLevel],
                                               rasterWidth,
                                               rasterHeight );
            RWASSERT( NULL != tempImage );
            if ( NULL == tempImage )
            {
                /* error! */
                RWRETURN( FALSE );
            }

            RwImageDestroy( imageMip[curMipLevel] );

            imageMip[curMipLevel] = tempImage;

            /* rescale width and height for mipmaps */
            if ( rasterWidth > 1 )
            {
                rasterWidth >>= 1;
            }

            if ( rasterHeight > 1 )
            {
                rasterHeight >>= 1;
            }
        }
    }

    if ( texturePrefix.numMipLevels > 1 )
    {
        /* remove AUTOMIPMAP flag to avoid unlock invoking mipmap creation
         * but only when explicit miplevels exist */
        autoMipmap = imRaster->cFormat & (rwRASTERFORMATAUTOMIPMAP >> 8);
        imRaster->cFormat &= ~autoMipmap;
    }

    rasterMipLevels = RwRasterGetNumLevels( imRaster );
    for ( curMipLevel = 0;
		// 마고자 (2005-06-03 오후 3:20:28) : 
		// 밉맵 레벨이 이상한 경우가 있어서 프로텍션
		curMipLevel < ( rasterMipLevels < texturePrefix.numMipLevels ? rasterMipLevels : texturePrefix.numMipLevels );
		curMipLevel += 1 )
    {
        /* ensure we don't exceed the maximum */
        if ( curMipLevel == texturePrefix.numMipLevels )
        {
            break;
        }

        if ( NULL == RwRasterLock( imRaster,
                                   (RwUInt8)curMipLevel,
                                   rwRASTERLOCKWRITE ) )
        {
            /* error! */
            RWRETURN( FALSE );
        }

        if ( NULL == RwRasterSetFromImage( imRaster,
                                           imageMip[curMipLevel] ) )
        {
            /* error! */
            RWRETURN( FALSE );
        }

        if ( NULL == RwRasterUnlock( imRaster ) )
        {
            /* error! */
            RWRETURN( FALSE );
        }

        RwImageDestroy( imageMip[curMipLevel] );
    }

    if ( texturePrefix.numMipLevels > 1 )
    {
        /* restore auto mip flag */
        imRaster->cFormat |= autoMipmap;
    }

    /* tidy up any spare RwImage mip levels */
    for ( ; curMipLevel < texturePrefix.numMipLevels; curMipLevel += 1 )
    {
        RwImageDestroy( imageMip[curMipLevel] );
    }

    /* and finally the array of pointers */
    RwFree( imageMip );

    /* now set the raster to the texture */
    RwTextureSetRaster( texture, imRaster );

    /* now finish reading the texture extension data */
    if ( NULL == rtpitexdTextureStreamReadPost( stream, texture ) )
    {
        /* error! */
        RWRETURN( FALSE );
    }

#if defined( RWDEBUG )
    {
        RwChar                      name[rwTEXTUREBASENAMELENGTH];

        RwChar                      mask[rwTEXTUREBASENAMELENGTH];

        rtpitexdTextureFlagsLegacy  flags;

        RwTextureAddressMode        texAddrU;

        RwTextureAddressMode        texAddrV;

        RwChar                      buffer[1024];


        /* get the texture and mask names */
        rwstrcpy( (RwChar *)name, RwTextureGetName( texture ) );
        rwstrcpy( (RwChar *)mask, RwTextureGetMaskName( texture ) );

        /* get the texture flags (filter and addressing) */
        flags = (RwTextureGetFilterMode( texture ) & rwTEXTUREFILTERMODEMASK);

        texAddrU = RwTextureGetAddressingU( texture );
        texAddrV = RwTextureGetAddressingV( texture );

        flags |= ((texAddrU << 8)  & rwTEXTUREADDRESSINGUMASK);
        flags |= ((texAddrV << 12) & rwTEXTUREADDRESSINGVMASK);

        RWMESSAGE( (RWSTRING("Read in texture data: ")\
                    RWSTRING("name = '%s' ")\
                    RWSTRING("mask = '%s' ")\
                    RWSTRING("num mip levels = %d"),
                             name, mask, texturePrefix.numMipLevels) );

        rwsprintf( buffer, RWSTRING("Flags (0x%x) = "), flags );

        switch ( RwTextureGetFilterMode(texture) )
        {
        case rwFILTERNEAREST:
            rwstrcat( buffer, RWSTRING("rwFILTERNEAREST ") );
            break;

        case rwFILTERLINEAR:
            rwstrcat( buffer, RWSTRING("rwFILTERLINEAR ") );
            break;

        case rwFILTERMIPNEAREST:
            rwstrcat( buffer, RWSTRING("rwFILTERMIPNEAREST ") );
            break;

        case rwFILTERMIPLINEAR:
            rwstrcat( buffer, RWSTRING("rwFILTERMIPLINEAR ") );
            break;

        case rwFILTERLINEARMIPNEAREST:
            rwstrcat( buffer, RWSTRING("rwFILTERLINEARMIPNEAREST ") );
            break;

        case rwFILTERLINEARMIPLINEAR:
            rwstrcat( buffer, RWSTRING("rwFILTERLINEARMIPLINEAR ") );
            break;

        default:
            rwstrcat( buffer, RWSTRING("(Unknown filter) ") );
        }

        switch ( texAddrU )
        {
        case rwTEXTUREADDRESSWRAP:
            rwstrcat( buffer, RWSTRING("rwTEXTUREADDRESSWRAP[u] ") );
            break;

        case rwTEXTUREADDRESSMIRROR:
            rwstrcat( buffer, RWSTRING("rwTEXTUREADDRESSMIRROR[u] ") );
            break;

        case rwTEXTUREADDRESSCLAMP:
            rwstrcat( buffer, RWSTRING("rwTEXTUREADDRESSCLAMP[u] ") );
            break;

        case rwTEXTUREADDRESSBORDER:
            rwstrcat( buffer, RWSTRING("rwTEXTUREADDRESSBORDER[u] ") );
            break;

        default:
            rwstrcat( buffer, RWSTRING("(Unknown U texture addressing) ") );
        }

        switch ( texAddrV )
        {
        case rwTEXTUREADDRESSWRAP:
            rwstrcat( buffer, RWSTRING("rwTEXTUREADDRESSWRAP[v] ") );
            break;

        case rwTEXTUREADDRESSMIRROR:
            rwstrcat( buffer, RWSTRING("rwTEXTUREADDRESSMIRROR[v] ") );
            break;

        case rwTEXTUREADDRESSCLAMP:
            rwstrcat( buffer, RWSTRING("rwTEXTUREADDRESSCLAMP[v] ") );
            break;

        case rwTEXTUREADDRESSBORDER:
            rwstrcat( buffer, RWSTRING("rwTEXTUREADDRESSBORDER[v] ") );
            break;

        default:
            rwstrcat( buffer, RWSTRING("(Unknown V texture addressing) ") );
        }

        RWMESSAGE( (buffer) );

    }
#endif /* defined( RWDEBUG ) */

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: rtpitexdCreateImageFromRaster
 *
 *  Purpose : Create a sequence of RwImages from the mip-levels of an
 *            RwRaster.
 *
 *  On entry: raster - RwRaster to create RwImages from.
 *
 *            numMipLevels - number of mip-levels in the RwRaster.
 *
 *            rasterDepth - Pointer to RwUInt32 to return the raster depth
 *                          in and to set the RwImage from the RwRaster.
 *                          May be NULL to not return a value AND not to
 *                          set the RwImage data.
 *
 *  On exit : Double pointer to RwImage containing the mip-level image
 *            sequence.
 * --------------------------------------------------------------------- */
static RwImage **
rtpitexdCreateImageFromRaster( RwRaster *raster,
                               rtpitexdMipLevels numMipLevels,
                               RwUInt32 *imageDepth)
{
    RwImage             **imageMip;

    rtpitexdMipLevels   curMipLevel;
    RwRasterFormat      format;
    RwUInt32            depth;


    RWFUNCTION( RWSTRING( "rtpitexdCreateImageFromRaster" ) );

    RWASSERT( NULL != raster );
    RWASSERT( numMipLevels > 0 );

    /* create an array of RwImage pointers large enough to
     * cater for all the miplevels of the texture
     */
    imageMip = (RwImage **)RwMalloc( numMipLevels * sizeof(RwImage *),
                             rwID_PITEXDICTIONARYTK | rwMEMHINTDUR_EVENT);
    RWASSERT( NULL != imageMip );
    if ( NULL == imageMip )
    {
        RWERROR( (E_RW_NOMEM, numMipLevels * sizeof(RwImage *)) );
        RWRETURN( NULL );
    }

    format = (RwRasterFormat) RwRasterGetFormat(raster);

    if (format & rwRASTERFORMATPAL4)
    {
        depth = 4;
    }
    else if (format & rwRASTERFORMATPAL8)
    {
        depth = 8;
    }
    else
    {
        depth = 32;
    }

    /* loop around the miplevels */
    for ( curMipLevel = 0; curMipLevel < numMipLevels; curMipLevel += 1 )
    {


        if ( NULL == RwRasterLock( raster,
                                   (RwUInt8)curMipLevel,
                                   rwRASTERLOCKREAD ) )
        {
            /* error! accessing curMipLevel mip level */
            RwFree( imageMip );
            RWRETURN( NULL );
        }



        /* create an image with the same depth as the raster */
        imageMip[curMipLevel] = RwImageCreate( RwRasterGetWidth(raster),
                                               RwRasterGetHeight(raster),
                                               depth );
        RWASSERT( NULL != imageMip[curMipLevel] );
        if ( NULL == imageMip[curMipLevel] )
        {
            rtpitexdMipLevels   tempMipLevel;


            /* error! */
            for ( tempMipLevel = 0; tempMipLevel < curMipLevel; tempMipLevel += 1 )
            {
                RwImageDestroy( imageMip[tempMipLevel] );
            }

            RwFree( imageMip );
            RWRETURN( NULL );
        }

        /* if the depth is 4 or 8 bits, then storage for the RwImage's palette is
         * also allocated by RwImageAllocatePixels (see the API refs)
         */
        if ( NULL == RwImageAllocatePixels( imageMip[curMipLevel] ) )
        {
            rtpitexdMipLevels   tempMipLevel;


            /* error! */
            for ( tempMipLevel = 0; tempMipLevel < curMipLevel; tempMipLevel += 1 )
            {
                RwImageDestroy( imageMip[tempMipLevel] );
            }

            RwFree( imageMip );
            RWRETURN( NULL );
        }

        /* set the image from the raster if required */
        if ( NULL != imageDepth )
        {
            if ( NULL == RwImageSetFromRaster( imageMip[curMipLevel],
                                               raster ) )
            {
                rtpitexdMipLevels   tempMipLevel;


                /* error! */
                for ( tempMipLevel = 0; tempMipLevel < curMipLevel; tempMipLevel += 1 )
                {
                    RwImageDestroy( imageMip[tempMipLevel] );
                }

                RwFree( imageMip );
                RWRETURN( NULL );
            }
        }

        if ( NULL == RwRasterUnlock( raster ) )
        {
            rtpitexdMipLevels   tempMipLevel;


            /* error! */
            for ( tempMipLevel = 0; tempMipLevel <= curMipLevel; tempMipLevel += 1 )
            {
                RwImageDestroy( imageMip[tempMipLevel] );
            }

            RwFree( imageMip );
            RWRETURN( NULL );
        }
    }

    RWRETURN( imageMip );
}


/* =====================================================================
 *  Global function definitions
 * ===================================================================== */

/**
 * \ingroup rtpitexd
 * \ref RtPITexDictionaryStreamGetSize returns the size, in bytes, of the
 * platform independent texture dictionary binary that can be created from
 * the specified \ref RwTexDictionary.
 *
 * \param texDict Pointer to a constant \ref RwTexDictionary.
 *
 * \return \ref RwUInt32 containing the size, in bytes, of the platform
 * independent texture dictionary, or zero if there was an error.
 *
 * \see RwTexDictionaryStreamGetSize
 */
RwUInt32
RtPITexDictionaryStreamGetSize( const RwTexDictionary *texDict )
{
    RwUInt32    chunkSize;


    RWAPIFUNCTION( RWSTRING( "RtPITexDictionaryStreamGetSize" ) );

    RWASSERT( NULL != texDict );

    /* initialize the chunksize to the chunk prefix size we're using */
    chunkSize = sizeof( rtpitexdChunkPrefix );

    /* get the chunksize */
    RwTexDictionaryForAllTextures( texDict,
                                   rtpitexdTextureSizeCallBack,
                                   &chunkSize );

    RWRETURN( chunkSize );
}


/**
 * \ingroup rtpitexd
 * \ref RtPITexDictionaryStreamRead reads a platform independent
 * texture dictionary from a currently open \ref RwStream and converts it
 * to a platform dependent \ref RwTexDictionary.
 *
 * The sequence to locate and read platform independent texture
 * dictionary from a binary stream is as follows:
 * \code
   RwStream *stream;
   RwTexDictionary *newPITexDictionary;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_PITEXDICTIONARY, NULL, NULL) )
       {
           newPITexDictionary = RtPITexDictionaryStreamRead(stream);
       }

       RwStreamClose(stream, NULL);
   }
   \endcode
 *
 * \param stream Pointer to \ref RwStream that is open for read access and
 *               contains a platform independent texture dictionary.
 *
 * \return Pointer to the \ref RwTexDictionary that is created from the platform
 * independent texture dictionary in the \ref RwStream, or NULL if there was
 * an error.
 *
 * \see RtPITexDictionaryStreamWrite
 * \see RtPITexDictionaryStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RwStreamFindChunk
 */
RwTexDictionary *
RtPITexDictionaryStreamRead( RwStream *stream )
{
    RwTexDictionary     *texDict;

    rtpitexdChunkPrefix chunkPrefix;

    RwUInt32            curImage;


    RWAPIFUNCTION( RWSTRING( "RtPITexDictionaryStreamRead" ) );

    RWASSERT( NULL != stream );

    /* read the chunk prefix */
    if ( sizeof(rtpitexdChunkPrefix) > RwStreamRead( stream,
                                                     &chunkPrefix,
                                                     sizeof( rtpitexdChunkPrefix ) ) )
    {
        RWRETURN( (RwTexDictionary *)NULL );
    }

    /* Convert from network format */
    (void)RwMemLittleEndian16(&chunkPrefix.numTextures,
                              sizeof(chunkPrefix.numTextures));
    (void)RwMemLittleEndian16(&chunkPrefix.versionNum,
                              sizeof(chunkPrefix.versionNum));

#if defined( RWDEBUG )
    RWMESSAGE( (RWSTRING("Reading a PI texture dictionary containing %d images (creator version 0x%x)"),
                chunkPrefix.numTextures, chunkPrefix.versionNum) );
#endif /* defined( RWDEBUG ) */

    /* create the texture dictionary */
    texDict = RwTexDictionaryCreate();
    RWASSERT( NULL != texDict );
    if ( NULL == texDict )
    {
        /* error! */
        RWRETURN( (RwTexDictionary *)NULL );
    }

    if ( chunkPrefix.versionNum < _RTPITEXDICT_CURRENTVERSION )
    {
        /* loop over all the RwImage's stored */
        for ( curImage = 0; curImage < chunkPrefix.numTextures; curImage += 1 )
        {
            rtpitexdImage2TextureReadLegacy( stream, &texDict );
        }
    }
    else
    {
        RwTexDictionary     *currentTexDict;


        currentTexDict = RwTexDictionaryGetCurrent();

        RwTexDictionarySetCurrent( texDict );

        /* loop over all the RwImage's stored */
        for ( curImage = 0; curImage < chunkPrefix.numTextures; curImage += 1 )
        {
            if ( FALSE == rtpitexdImage2TextureRead( stream ) )
            {
                RwTexDictionarySetCurrent( currentTexDict );
                RWRETURN( (RwTexDictionary *)NULL );
            }
        }

        RwTexDictionarySetCurrent( currentTexDict );
    }

    RWRETURN( texDict );
}


/**
 * \ingroup rtpitexd
 * \ref RtPITexDictionaryStreamWrite writes a platform dependent \ref
 * RwTexDictionary to a currently open \ref RwStream as a platform
 * independent texture dictionary.
 *
 * \param texDict Pointer to \ref RwTexDictionary to convert
 *                to a platform independent texture dictionary.
 *
 * \param stream Pointer to a currently open \ref RwStream that can
 *               be written to.
 *
 * \return Pointer to the \ref RwTexDictionary passed to this function,
 * or NULL if there was an error.
 */
RwTexDictionary *
RtPITexDictionaryStreamWrite( RwTexDictionary *texDict,
                              RwStream *stream )
{
    RwUInt32            numTextures;

    rtpitexdChunkPrefix prefix;

    RwUInt32            chunkSize;


    RWAPIFUNCTION( RWSTRING( "RtPITexDictionaryStreamWrite" ) );

    RWASSERT( NULL != texDict );
    RWASSERT( NULL != stream );

    /* get the number of textures in the specified dictionary */
    numTextures = 0;
    RwTexDictionaryForAllTextures( texDict,
                                   rtpitexdTextureCountCallBack,
                                   &numTextures );

    /* check limits */
    if ( numTextures >= 65536 )
    {
        RWERROR( (E_RW_BADVERSION,\
                  RWSTRING("Too many textures in dictionary (> 65536)")) );

        RWRETURN( (RwTexDictionary *)NULL );
    }

    prefix.numTextures = (RwUInt16)numTextures;
    prefix.versionNum = _RTPITEXDICT_CURRENTVERSION;

#if defined( RWDEBUG )
    RWMESSAGE( (RWSTRING("There are %d textures in the dictionary"),
                prefix.numTextures) );
    RWMESSAGE( (RWSTRING("Dictionary version number is 0x%x"),
                prefix.versionNum) );
#endif /* defined( RWDEBUG ) */

    /* get the chunksize */
    chunkSize = RtPITexDictionaryStreamGetSize( texDict );

#if defined( RWDEBUG )
    RWMESSAGE( ("The chunksize of this dictionary is %d bytes", chunkSize ) );
#endif /* defined( RWDEBUG ) */

    /* write the chunk header */
    if ( NULL == RwStreamWriteChunkHeader( stream,
                                           rwID_PITEXDICTIONARY,
                                           (RwInt32)chunkSize ) )
    {
        /* error! */
        RWRETURN( (RwTexDictionary *)NULL );
    }

    /* Convert to network format */
    (void)RwMemLittleEndian16(&prefix.numTextures, sizeof(prefix.numTextures));
    (void)RwMemLittleEndian16(&prefix.versionNum, sizeof(prefix.versionNum));

    /* write the chunk prefix */
    if ( NULL == RwStreamWrite( stream,
                                (void const *)&prefix,
                                sizeof( prefix ) ) )
    {
        /* error! */
        RWRETURN( (RwTexDictionary *)NULL );
    }

    /* write out each RwImage corresponding to each RwTexture in the dictionary */
    RwTexDictionaryForAllTextures( texDict,
                                   rtpitexdTexture2ImageWriteCallBack,
                                   (void *)stream );

    RWRETURN( texDict );
}
