/*
 * TIFF tool
 */

/**
 * \ingroup  rttiff
 * \page rttiffoverview RtTIFF Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rttiff.h
 * \li \b Libraries: rwcore, rttiff
 * \li \b Plugin \b attachments: None
 *
 * \subsection tiffoverview Overview
 * RtTIFF loads tiff files, currently on PC and PlayStation 2 only.
 * It was added to support Maya users who wanted transparency in their textures.
 * Otherwise, you should use another image format like PNG, BMP or RAS
 * as they are more likely to work on more platforms.
 *
 * RtTIFF supports the following strip based TIFF formats only:
 * (If you use Photoshop to save your TIFFs, you should be ok.)
 * \li 8 bit RGB palette
 * \li 8 bit grayscale
 * \li 888  RGB
 * \li 8888 RGBA
 * \li LZWed, big or little endianed any of the above.
 */

/**********************************************************************
 *
 * File :     rttiff.c
 *
 * Abstract : Read TIFF files in as RwImages
 *
 **********************************************************************
 *
 * This file is a product of Criterion Software Ltd.
 *
 * This file is provided as is with no warranties of any kind and is
 * provided without any obligation on Criterion Software Ltd. or
 * Canon Inc. to assist in its use or modification.
 *
 * Criterion Software Ltd. will not, under any
 * circumstances, be liable for any lost revenue or other damages arising
 * from the use of this file.
 *
 * Copyright (c) 1998 Criterion Software Ltd.
 * All Rights Reserved.
 *
 * RenderWare is a trademark of Canon Inc.
 *
 ************************************************************************/

/****************************************************************************
 Includes
 */

/* Image handling */
#include "libtiff/tiffiop.h"

/*
tiffcomp.h Line 154 defines INLINE in a really stupid way
This screws up with the Sony headers, libdma.h, included by rwcore.h
since it defines functions dozens of times in your library.
So put it back the way Sony would have done it...
*/
#undef INLINE

/* These MUST be included AFTER libtiff or on PS2 we get weird clashing */
#include <rwcore.h>
#include <rpdbgerr.h>
#include "rttiff.h"

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

static RwBool
cvt_tiled_TIFF_to_RwImage(TIFF * tif, RwImage * image)
{
    uint8              *raster; /* retrieve RGBA image */
    uint32              width, height; /* image width & height */
    uint32              tile_xsize, tile_ysize;
    uint32              read_xsize, read_ysize;
    uint32              row, col, tile;
    int                 ok = 1;
    RWFUNCTION(RWSTRING("cvt_tiled_TIFF_to_RwImage"));

    /*
     * Verify that our request is legal - on a tile file, and on a
     * tile boundary.
     */

    if (!TIFFIsTiled(tif))
    {
        TIFFError(TIFFFileName(tif),
                  "Can't use TIFFReadRGBATile() with stripped file.");
        RWRETURN( (0) );
    }

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tile_xsize);
    TIFFGetField(tif, TIFFTAG_TILELENGTH, &tile_ysize);

    /*
     * Allocate tile buffer
     */
    raster = _TIFFmalloc(TIFFTileSize(tif));

    row = col = 0;

    /*
     * Loop over the tiles.
     */
    for (tile = 0; ok && tile < TIFFNumberOfTiles(tif); tile++)
    {
        uint32              i_row;

        /* Read the tile into a buffer */
        if (!TIFFReadEncodedTile(tif, tile, raster, -1))
        {
            ok = 0;
            break;
        }

        /*
         * Figure out the actual size of the tile
         */
        if (row + tile_ysize > height)
            read_ysize = height - row;
        else
            read_ysize = tile_ysize;
        if (col + tile_xsize > width)
            read_xsize = width - col;
        else
            read_xsize = tile_xsize;

        /*
         * Copy the result into the image
         */

        for (i_row = 0; i_row < read_ysize; i_row++)
        {
            unsigned char      *imageptr = RwImageGetPixels(image);

            imageptr +=
                (RwImageGetStride(image) * (row + i_row)) + (col);
            _TIFFmemcpy(imageptr, raster + tile_xsize * i_row,
                        read_xsize);
        }

        col += tile_xsize;
        if (col > width)
        {
            col = 0;
            row += tile_ysize;
        }
    }

    _TIFFfree(raster);

    RWRETURN( ok );
}

static RwBool
cvt_TIFF_by_strip_to_RwImage(TIFF * tif, RwImage * image)
{
    uint8              *raster; /* retrieve RGBA image */
    uint32              width, height; /* image width & height */
    uint32              strip;
    int                 ok = 1;
    uint32              rowsperstrip, rowstowrite, i_row;
    RWFUNCTION(RWSTRING("cvt_TIFF_by_strip_to_RwImage"));

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);

    if (!TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip))
    {
        TIFFError(TIFFFileName(tif), "Source image not in strips");
        RWRETURN( (0) );
    }

    /*
     * Loop over the strips.
     */
    raster = _TIFFmalloc(TIFFStripSize(tif));
    for (strip = 0; strip < TIFFNumberOfStrips(tif); strip++)
    {
        TIFFReadEncodedStrip(tif, strip, raster, -1);

        if (strip == TIFFNumberOfStrips(tif) - 1)
        {
            /* last strip */
            rowstowrite = height - (strip * rowsperstrip);
        }
        else
        {
            rowstowrite = rowsperstrip;
        }

        /*
         * Write out the result in a strip
         */

        for (i_row = 0; i_row < rowstowrite; i_row++)
        {
            unsigned char      *imageptr = RwImageGetPixels(image);

            imageptr +=
                (RwImageGetStride(image) *
                 ((strip * rowsperstrip) + i_row));
            _TIFFmemcpy(imageptr, raster + width * i_row, width);
        }
    }

    _TIFFfree(raster);

    RWRETURN( ok );
}

static RwBool
TIFF_Get_Palette(TIFF * tif, RwRGBA * imagePal)
{
    uint16             *red_orig, *green_orig, *blue_orig;
    int                 index;
    RWFUNCTION(RWSTRING("TIFF_Get_Palette"));

    if (!TIFFGetField(tif, TIFFTAG_COLORMAP,
                      &red_orig, &green_orig, &blue_orig))
    {
        TIFFError(TIFFFileName(tif),
                  "Missing required \"Colormap\" tag");
        RWRETURN( (FALSE) );
    }

    for (index = 0; index < 256; index++)
    {
        imagePal[index].red = red_orig[index] >> 8;
        imagePal[index].green = green_orig[index] >> 8;
        imagePal[index].blue = blue_orig[index] >> 8;
        imagePal[index].alpha = 255;
    }

    RWRETURN( (TRUE) );
}

static RwBool
cvt_RGBAtiled_TIFF_to_RwImage(TIFF * tif, RwImage * image)
{
    uint32             *raster; /* retrieve RGBA image */
    uint32              width, height; /* image width & height */
    uint32              tile_xsize, tile_ysize;
    uint32              read_xsize, read_ysize;
    uint32              row, col;
    int                 ok = 1;
    RWFUNCTION(RWSTRING("cvt_RGBAtiled_TIFF_to_RwImage"));

    /*
     * Verify that our request is legal - on a tile file, and on a
     * tile boundary.
     */

    if (!TIFFIsTiled(tif))
    {
        TIFFError(TIFFFileName(tif),
                  "Can't use TIFFReadRGBATile() with stripped file.");
        RWRETURN( (0) );
    }

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tile_xsize);
    TIFFGetField(tif, TIFFTAG_TILELENGTH, &tile_ysize);

    /*
     * Allocate tile buffer
     */
    raster =
        (uint32 *) _TIFFmalloc(tile_xsize * tile_ysize *
                               sizeof(uint32));
    if (raster == 0)
    {
        TIFFError(TIFFFileName(tif), "No space for raster buffer");
        RWRETURN( (0) );
    }

    /*
     * Loop over the tiles.
     */
    for (col = 0; ok && col < width; col += tile_xsize)
    {
        for (row = 0; ok && row < height; row += tile_ysize)
        {
            uint32              i_row;

            /* Read the strip into an RGBA array */
            if (!TIFFReadRGBATile(tif, col, row, raster))
            {
                ok = 0;
                break;
            }

            /*
             * Figure out the actual size of the tile
             */
            if (row + tile_ysize > height)
                read_ysize = height - row;
            else
                read_ysize = tile_ysize;
            if (col + tile_xsize > width)
                read_xsize = width - col;
            else
                read_xsize = tile_xsize;

            /*
             * Copy the result into the image
             */

            for (i_row = 0; i_row < read_ysize; i_row++)
            {
                unsigned char      *imageptr = RwImageGetPixels(image);

                imageptr +=
                    (RwImageGetStride(image) * (row + i_row)) +
                    (col * 4);
                /* TIFFReadRGBATile reads strips upside down so invert them here */
                _TIFFmemcpy(imageptr,
                            raster + tile_xsize * (read_ysize - i_row -
                                                   1), 4 * read_xsize);
            }

        }
    }

    _TIFFfree(raster);

    RWRETURN( ok );
}

static RwBool
cvt_TIFF_by_RGBAstrip_to_RwImage(TIFF * tif, RwImage * image)
{
    uint32              width, height; /* image width & height */
    int                 ok = 1;
    uint32              rowsperstrip;
    tdata_t             buf;
    tstrip_t            strip;
    RwUInt32            image_row = 0;
    RwUInt8            *imageBase = RwImageGetPixels(image);
    RwInt32             imageStride = RwImageGetStride(image);
    RwUInt16            samplesperpixel;
    RWFUNCTION(RWSTRING("cvt_TIFF_by_RGBAstrip_to_RwImage"));

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);

    if (!TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip))
    {
        TIFFError(TIFFFileName(tif), "Source image not in strips");
        RWRETURN( (0) );
    }

    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);

    buf = _TIFFmalloc(TIFFStripSize(tif));

    /* tiff is stored in a bunch of strips with several rows each */
    for (strip = 0; strip < TIFFNumberOfStrips(tif); strip++)
    {
        uint32              strip_row;
        RwInt8             *stripRow;

        /*
         * Get a strip, endian corrected for this processor (hopefully?!)
         * (This seems to be the only method in which to correctly extract alpha channels
         * from Photoshop created TIFFs - TIFFReadRGBAStrip just ignores them.)
         */
        if (!TIFFReadEncodedStrip(tif, strip, buf, (tsize_t) - 1))
        {
            ok = 0;
            break;
        }

        /* Not all strips have full # of rows! don't go past # contained in image */
        for (strip_row = 0;
             strip_row < rowsperstrip && image_row < height;
             strip_row++, image_row++)
        {
            uint32              x;

            RwUInt8            *byte =
                imageBase + imageStride * image_row;

            stripRow =
                (RwInt8 *) buf + width * samplesperpixel * strip_row;

            /* copy a row to our image */
            for (x = 0; x < width; x++)
            {
                int                 sample;

                if (samplesperpixel == 3)
                {
                    /* RGB only - make alpha opaque */
                    for (sample = 0; sample < 3; sample++)
                    {
                        byte[x * 4 + sample] =
                            stripRow[x * samplesperpixel + sample];
                    }

                    byte[x * 4 + sample] = 0xff;
                }
                else
                {
                    /* We only care about RGBA - ignore additional channels/samples */
                    for (sample = 0; sample < 4; sample++)
                    {
                        byte[x * 4 + sample] =
                            stripRow[x * samplesperpixel + sample];
                    }
                }
            }                  /* rof */
        }                      /* rof */
    }                          /* rof */

    _TIFFfree(buf);

    RWRETURN( ok );
}

/************************************************************************
 *
 *      Function:       RtTIFFImageRead
 *
 *      Description:    Read a TIFF file (using libtiff) and return as an
 *                      RwImage
 *
 *      Parameters:     name of file to load
 *
 *      Return Value:   RwImage * on success, or NULL on failure
 *
 ************************************************************************/

/**
 * \ingroup rttiff
 * \ref RtTIFFImageRead
 * reads a TIFF file (using libtiff) and return this as an * RwImage
 *
 * \param imageName  name of file to load
 *
 * \return RwImage * on success, or NULL on failure
 *
 */
RwImage            *
RtTIFFImageRead(const RwChar * imageName)
{
    RWAPIFUNCTION(RWSTRING("RtTIFFImageRead"));
    RWASSERT(imageName);

    if (imageName)
    {
        TIFF               *tif;

        tif = TIFFOpen(imageName, "rb");

        if (tif)
        {
            RwUInt16            samplesperpixel, bitspersample,
                planarconfig, photometric;

            RwInt32             width, height, depth;
            RwImage            *image;

            TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
            TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
            TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitspersample);
            TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL,
                         &samplesperpixel);
            TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &planarconfig);
            TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric);

            if (
                   /* Too much messy bitmunching required for us to bother... */
                   (bitspersample != 8)
                   /* Probably a palettized pic with separate alpha channel
                    * - messy quantization required - no way Jose!,
                    * Either that or somebody's been deleting channels in Photoshop
                    * - too weird for my liking */
                   || (samplesperpixel == 2)
                   /* Somebody's going to town with channels in Photoshop
                    * RGBA is the max useful number for us. */
                   || (samplesperpixel > 4)
                   /* Photoshop seems to write contig only
                    * - does anything useful write separated TIFFs?
                    * (our strip reader will break so we can't handle it) */
                   || (planarconfig != PLANARCONFIG_CONTIG)
                   /* weird color models passing thru our strip reader will display a bit barf */
                   || (photometric != PHOTOMETRIC_MINISWHITE
                       && photometric != PHOTOMETRIC_MINISBLACK
                       && photometric != PHOTOMETRIC_PALETTE
                       && photometric != PHOTOMETRIC_RGB))
            {
                TIFFClose(tif);
                RWRETURN(NULL);
            }

            depth = bitspersample * samplesperpixel;

            if (depth == 24)   /* Rw can't do this - use a 32 with opaque alpha */
            {
                depth = 32;
            }

            /* Allocate the image */
            image =
                RwImageCreate((RwInt32) width, (RwInt32) height,
                              (RwInt32) depth);
            if (image)
            {
                RwUInt8            *imagePixels;
                RwRGBA             *imagePal;
                RwUInt32            imageStride;
                RwBool              convertSuccess = TRUE;

                if (!RwImageAllocatePixels(image))
                {
                    RwImageDestroy(image);
                    image = NULL;

                    TIFFClose(tif);

                    RWRETURN(NULL);
                }

                imagePixels = RwImageGetPixels(image);
                imagePal = RwImageGetPalette(image);
                imageStride = RwImageGetStride(image);

                if (depth > 8)
                {
                    if (TIFFIsTiled(tif))
                    {
                        convertSuccess =
                            cvt_RGBAtiled_TIFF_to_RwImage(tif, image);
                    }
                    else
                    {
                        convertSuccess =
                            cvt_TIFF_by_RGBAstrip_to_RwImage(tif,
                                                             image);
                    }
                }
                else
                {
                    uint16              photometric;
                    int                 i;

                    TIFFGetField(tif, TIFFTAG_PHOTOMETRIC,
                                 &photometric);
                    switch (photometric)
                    {
                        case PHOTOMETRIC_MINISWHITE:
                            for (i = 0; i < 256; i++)
                            {
                                imagePal[i].red = 255 - i;
                                imagePal[i].green = 255 - i;
                                imagePal[i].blue = 255 - i;
                                imagePal[i].alpha = 255;
                            }
                            break;
                        case PHOTOMETRIC_MINISBLACK:
                            for (i = 0; i < 256; i++)
                            {
                                imagePal[i].red = i;
                                imagePal[i].green = i;
                                imagePal[i].blue = i;
                                imagePal[i].alpha = 255;
                            }
                            break;
                        case PHOTOMETRIC_PALETTE:
                            /* setup a palette */
                            TIFF_Get_Palette(tif, imagePal);
                            break;
                        default:
                            convertSuccess = FALSE;
                    }

                    if (convertSuccess)
                    {
                        if (TIFFIsTiled(tif))
                        {
                            convertSuccess =
                                cvt_tiled_TIFF_to_RwImage(tif, image);
                        }
                        else
                        {
                            convertSuccess =
                                cvt_TIFF_by_strip_to_RwImage(tif,
                                                             image);
                        }
                    }

                }

                if (!convertSuccess)
                {
                    RwImageDestroy(image);
                    image = NULL;
                    TIFFClose(tif);
                    RWRETURN(NULL);
                }
            }

            TIFFClose(tif);

            RWRETURN(image);
        }

        /* Failed to open the file */
        RWRETURN(NULL);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN(NULL);
}

RwImage            *
RtTIFFImageReadStream(RwStream *stream, const RwChar * imageName)
{
    RWAPIFUNCTION(RWSTRING("RtTIFFImageRead"));
    RWASSERT(imageName);

    if (imageName)
    {
        TIFF               *tif;

        tif = TIFFOpenStream(stream, imageName, "rb");

        if (tif)
        {
            RwUInt16            samplesperpixel, bitspersample,
                planarconfig, photometric;

            RwInt32             width, height, depth;
            RwImage            *image;

            TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
            TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
            TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitspersample);
            TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL,
                         &samplesperpixel);
            TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &planarconfig);
            TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric);

            if (
                   /* Too much messy bitmunching required for us to bother... */
                   (bitspersample != 8)
                   /* Probably a palettized pic with separate alpha channel
                    * - messy quantization required - no way Jose!,
                    * Either that or somebody's been deleting channels in Photoshop
                    * - too weird for my liking */
                   || (samplesperpixel == 2)
                   /* Somebody's going to town with channels in Photoshop
                    * RGBA is the max useful number for us. */
                   || (samplesperpixel > 4)
                   /* Photoshop seems to write contig only
                    * - does anything useful write separated TIFFs?
                    * (our strip reader will break so we can't handle it) */
                   || (planarconfig != PLANARCONFIG_CONTIG)
                   /* weird color models passing thru our strip reader will display a bit barf */
                   || (photometric != PHOTOMETRIC_MINISWHITE
                       && photometric != PHOTOMETRIC_MINISBLACK
                       && photometric != PHOTOMETRIC_PALETTE
                       && photometric != PHOTOMETRIC_RGB))
            {
                RWRETURN(NULL);
            }

            depth = bitspersample * samplesperpixel;

            if (depth == 24)   /* Rw can't do this - use a 32 with opaque alpha */
            {
                depth = 32;
            }

            /* Allocate the image */
            image =
                RwImageCreate((RwInt32) width, (RwInt32) height,
                              (RwInt32) depth);
            if (image)
            {
                RwUInt8            *imagePixels;
                RwRGBA             *imagePal;
                RwUInt32            imageStride;
                RwBool              convertSuccess = TRUE;

                if (!RwImageAllocatePixels(image))
                {
                    RwImageDestroy(image);
                    image = NULL;

                    RWRETURN(NULL);
                }

                imagePixels = RwImageGetPixels(image);
                imagePal = RwImageGetPalette(image);
                imageStride = RwImageGetStride(image);

                if (depth > 8)
                {
                    if (TIFFIsTiled(tif))
                    {
                        convertSuccess =
                            cvt_RGBAtiled_TIFF_to_RwImage(tif, image);
                    }
                    else
                    {
                        convertSuccess =
                            cvt_TIFF_by_RGBAstrip_to_RwImage(tif,
                                                             image);
                    }
                }
                else
                {
                    uint16              photometric;
                    int                 i;

                    TIFFGetField(tif, TIFFTAG_PHOTOMETRIC,
                                 &photometric);
                    switch (photometric)
                    {
                        case PHOTOMETRIC_MINISWHITE:
                            for (i = 0; i < 256; i++)
                            {
                                imagePal[i].red = 255 - i;
                                imagePal[i].green = 255 - i;
                                imagePal[i].blue = 255 - i;
                                imagePal[i].alpha = 255;
                            }
                            break;
                        case PHOTOMETRIC_MINISBLACK:
                            for (i = 0; i < 256; i++)
                            {
                                imagePal[i].red = i;
                                imagePal[i].green = i;
                                imagePal[i].blue = i;
                                imagePal[i].alpha = 255;
                            }
                            break;
                        case PHOTOMETRIC_PALETTE:
                            /* setup a palette */
                            TIFF_Get_Palette(tif, imagePal);
                            break;
                        default:
                            convertSuccess = FALSE;
                    }

                    if (convertSuccess)
                    {
                        if (TIFFIsTiled(tif))
                        {
                            convertSuccess =
                                cvt_tiled_TIFF_to_RwImage(tif, image);
                        }
                        else
                        {
                            convertSuccess =
                                cvt_TIFF_by_strip_to_RwImage(tif,
                                                             image);
                        }
                    }

                }

                if (!convertSuccess)
                {
                    RwImageDestroy(image);
                    image = NULL;
                    RWRETURN(NULL);
                }
            }

            RWRETURN(image);
        }

        /* Failed to open the file */
        RWRETURN(NULL);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN(NULL);
}
