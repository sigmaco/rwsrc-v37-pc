/**
 * \ingroup rtpng
 * \page rtpngoverview RtPNG Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rtpng.h
 * \li \b Libraries: rwcore, rtpng
 * \li \b Plugin \b attachments: None
 *
 * \subsection pngoverview Overview
 * The PNG - Portable Network Graphics - image file format is a
 * relative newcomer to the graphics scene. It was designed primarily
 * in response to the unexpected decision by Unisys to enforce its patent
 * on LZW compression long after the GIF file format, which relies on LZW,
 * had entered widespread use.
 *
 * The PNG format supports both palettized (8-bit) images as well as RGBA
 * image formats. As a result, it can be used instead of either the GIF and
 * JPEG image formats for most purposes.
 *
 * Unlike the GIF-89a format, the PNG file format does not support
 * animation.
 *
 * The RtPNG toolkit provides reading and writing functions for the
 * Portable Network Graphics (‘PNG? image format.
 *
 */

/**********************************************************************
 *
 * File :     rtpng.c
 *
 * Abstract : Read PNG files in as RwImages
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

#include "rwcore.h"
#include "rpdbgerr.h"

/* Image handling */
#include "rtpng.h"
#include "png.h"

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

/************************************************************************
 *
 *      Function:       PNGMalloc
 *
 *      Description:    Custom memory allocator for libpng
 *
 *      Parameters:     png data block (unused)
 *                      amount of memory to allocate
 *
 *      Return Value:   Allocated memory, or NULL on error.
 *
 ************************************************************************/

static              png_voidp
PNGMalloc(png_structp png_ptr __RWUNUSED__, png_size_t size)
{
    png_voidp           memPtr;

    RWFUNCTION(RWSTRING("PNGMalloc"));

    memPtr = RwMalloc(size, rwID_PNGIMAGEPLUGIN | rwMEMHINTDUR_EVENT);

    RWRETURN(memPtr);
}

/************************************************************************
 *
 *      Function:       PNGFree
 *
 *      Description:    Custom memory deallocator for libpng
 *
 *      Parameters:     png data block (unused)
 *                      memory to free
 *
 *      Note:           The second parameter seems have have a weird type in
 *                      the pnglib header file.  (duplicated here to save a
 *                      warning).
 *
 *      Return Value:   None
 *
 ************************************************************************/

static void
PNGFree(png_structp png_ptr __RWUNUSED__, png_voidp memPtr)
{
    RWFUNCTION(RWSTRING("PNGFree"));
    RWASSERT(memPtr);

    RwFree(memPtr);

    RWRETURNVOID();
}

/************************************************************************
 *
 *      Function:       PNGRead
 *
 *      Description:    Custom stream read function for pnglib
 *
 *      Parameters:     png data block
 *                      destination address to read to
 *                      amount of data to read
 *
 *      Return Value:   None
 *
 ************************************************************************/

static void
PNGRead(png_structp png_ptr, png_bytep destAddr, png_size_t length)
{
    RwStream           *stream;

    RWFUNCTION(RWSTRING("PNGRead"));
    RWASSERT(png_ptr);
    RWASSERT(destAddr);

    /* Get at the stream */
    stream = (RwStream *) (png_ptr->io_ptr);

    if (stream)
    {
        RwStreamRead(stream, destAddr, length);
    }
    else
    {
        png_error(png_ptr, NULL);
    }

    RWRETURNVOID();
}

/************************************************************************
 *
 *      Function:       PNGWrite
 *
 *      Description:    Custom stream write function for pnglib
 *
 *      Parameters:     png data block
 *                      destination address to read to
 *                      amount of data to read
 *
 *      Return Value:   None
 *
 ************************************************************************/

static void
PNGWrite(png_structp png_ptr, png_bytep data, png_size_t length)
{
    RwStream           *stream;

    RWFUNCTION(RWSTRING("PNGWrite"));
    RWASSERT(png_ptr);
    RWASSERT(data);

    /* Get at the stream */
    stream = (RwStream *) (png_ptr->io_ptr);

    if (stream)
    {
        RwStreamWrite(stream, data, length);
    }
    else
    {
        png_error(png_ptr, NULL);
    }

    RWRETURNVOID();
}

/************************************************************************
 *
 *      Function:       PNGFlush
 *
 *      Description:    Custom stream flush function for pnglib
 *
 *      Parameters:     png data block
 *                      destination address to read to
 *                      amount of data to read
 *
 *      Return Value:   None
 *
 ************************************************************************/

static void
PNGFlush(png_structp png_ptr __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("PNGFlush"));

    RWRETURNVOID();
}

/**
 * \ingroup rtpng
 * \ref RtPNGImageWrite writes a PNG file, using libpng of
 * http://www.libpng.org/pub/png
 *
 * \param image Pointer to the image to be written.
 * \param imageName Pointer to the filename of the image.
 *
 * \return RwImage * on success, or NULL on failure.
 *
 * \see RtPNGImageRead
 * \see RwImageRead
 * \see RwImageWrite
 */
RwImage            *
RtPNGImageWrite(RwImage * image, const RwChar * imageName)
{
    RWAPIFUNCTION(RWSTRING("RtPNGImageWrite"));

    if ((imageName) && (image))
    {
        RwStream           *const stream =
            (RwStreamOpen(rwSTREAMFILENAME,
                          rwSTREAMWRITE,
                          imageName));

        if (stream)
        {
            volatile RwInt32    bit_depth = 0;
            volatile RwInt32    color_type = 0;
            RwInt32             depth;
            RwInt32             i;
            RwRGBA * volatile   imagePal = NULL;
            RwUInt32            imageStride;
            RwUInt8            *imagePixels;
            png_bytep           trans = NULL;
            png_colorp          palette = NULL;
            volatile png_bytepp rowPointers = NULL;
            volatile png_infop  info_ptr = NULL;
            volatile png_structp png_ptr = NULL;
            volatile png_uint_32 width = 0, height = 0;

            /* Create and initialize the png_struct with the desired error
             * handler functions.  If you want to use the default stderr
             * and longjump method, you can supply NULL for the last three
             * parameters.  We also check that the library version is
             * compatible with the one used at compile time,
             * in case we are using dynamically linked libraries.
             * REQUIRED.
             */
            png_ptr = png_create_write_struct_2(PNG_LIBPNG_VER_STRING,
                                                NULL, NULL, NULL,
                                                NULL, PNGMalloc,
                                                PNGFree);

            if (png_ptr == NULL)
            {
                RwStreamClose((RwStream *) stream, NULL);
                RWRETURN(NULL);
            }

            /* Allocate/initialize the image information data.  REQUIRED */
            info_ptr = png_create_info_struct(png_ptr);
            if (info_ptr == NULL)
            {
                RwStreamClose((RwStream *) stream, NULL);
                png_destroy_write_struct((png_structpp) & png_ptr,
                                         (png_infopp) NULL);
                RWRETURN(NULL);
            }

#ifndef PNG_SETJMP_NOT_SUPPORTED
            /* Set error handling.  REQUIRED if you aren't supplying your own
             * error hadnling functions in the png_create_write_struct() call.
             */
            if (setjmp(png_ptr->jmpbuf))
            {
                /* If we get here, we had a problem reading the file */
                RwStreamClose(stream, NULL);
                png_destroy_write_struct((png_structpp) & png_ptr,
                                         (png_infopp) NULL);
                RWRETURN(NULL);
            }
#endif /* PNG_SETJMP_NOT_SUPPORTED */

            /* If you are using replacement read functions, instead of calling
             * png_init_io() here you would call */
            png_set_write_fn(png_ptr, (void *) stream,
                             PNGWrite, PNGFlush);

            /* where user_io_ptr is a structure you want available to the
             * callbacks.
             * Set the image information here.  Width and height are up
             * to 2^31, bit_depth is one of 1, 2, 4, 8, or 16, but valid
             * values also depend on the color_type selected.
             * color_type is one of
             * PNG_COLOR_TYPE_GRAY,
             * PNG_COLOR_TYPE_GRAY_ALPHA,
             * PNG_COLOR_TYPE_PALETTE,
             * PNG_COLOR_TYPE_RGB, or
             * PNG_COLOR_TYPE_RGB_ALPHA.
             * interlace is either
             * PNG_INTERLACE_NONE or
             * PNG_INTERLACE_ADAM7,
             * and the compression_type and filter_type MUST currently be
             * PNG_COMPRESSION_TYPE_BASE and
             * PNG_FILTER_TYPE_BASE.
             * REQUIRED
             */
            depth = RwImageGetDepth(image);
            width = RwImageGetWidth(image);
            height = RwImageGetHeight(image);

            switch (depth)
            {
                case 32:
                    bit_depth = 8;
                    color_type = PNG_COLOR_TYPE_RGB_ALPHA;
                    imagePal = NULL;
                    break;

                case 8:
                    bit_depth = 8;
                    color_type = PNG_COLOR_TYPE_PALETTE;
                    imagePal = RwImageGetPalette(image);
                    break;

                case 4:
                    bit_depth = 4;
                    color_type = PNG_COLOR_TYPE_PALETTE;
                    imagePal = RwImageGetPalette(image);
                    break;

                default:
                    break;
            }

            png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth,
                         color_type,
                         PNG_INTERLACE_NONE,
                         PNG_COMPRESSION_TYPE_BASE,
                         PNG_FILTER_TYPE_BASE);

            /* set the palette if there is one.  R
             * EQUIRED for indexed-color images */
            if (color_type == PNG_COLOR_TYPE_PALETTE)
            {
                palette = (png_colorp)
                    PNGMalloc(png_ptr,
                              (1 << depth) * sizeof(png_color));

                trans = (png_bytep)
                    PNGMalloc(png_ptr, (1 << depth) * sizeof(png_byte));

                for (i = 0; i < (1 << depth); i++)
                {
                    palette[i].red = imagePal[i].red;
                    palette[i].green = imagePal[i].green;
                    palette[i].blue = imagePal[i].blue;
                    trans[i] = imagePal[i].alpha;
                }

                /* ... set palette colors ... */
                png_set_PLTE(png_ptr, info_ptr, palette, (1 << depth));

                /* ... set alpha colors ... */
                png_set_tRNS(png_ptr, info_ptr, trans, (1 << depth),
                             NULL);
            }
            else
            {
                palette = NULL;
                trans = NULL;
            }

#if (0)
            /* Not yet supported. */

            /* Optional gamma chunk is strongly suggested if you
             * have any guess as to the correct gamma of the image.
             */
            png_set_gAMA(png_ptr, info_ptr, gamma);

            /* Optionally write comments into the image */
            text_ptr[0].key = "Title";
            text_ptr[0].text = "Mona Lisa";
            text_ptr[0].compression = PNG_TEXT_COMPRESSION_NONE;
            text_ptr[1].key = "Author";
            text_ptr[1].text = "Leonardo DaVinci";
            text_ptr[1].compression = PNG_TEXT_COMPRESSION_NONE;
            text_ptr[2].key = "Description";
            text_ptr[2].text = "<long text>";
            text_ptr[2].compression = PNG_TEXT_COMPRESSION_zTXt;
            png_set_text(png_ptr, info_ptr, text_ptr, 3);

            /* other optional chunks like
             * cHRM, bKGD, tRNS, tIME, oFFs, pHYs, */
            /* note that if sRGB is present the cHRM chunk must be ignored
             * on read and must be written in accordance with the
             * sRGB profile */

#endif /* (0) */

            /* Write the file header information.  REQUIRED */
            png_write_info(png_ptr, info_ptr);

            /* Once we write out the header, the compression type on the text
             * chunks gets changed to PNG_TEXT_COMPRESSION_NONE_WR or
             * PNG_TEXT_COMPRESSION_zTXt_WR, so it doesn't get written out
             * again at the end.
             */

            /* pack pixels into bytes */
            png_set_packing(png_ptr);

#if (0)
            /* Not yet supported. */

            /* set up the transformations you want.  Note that these are
             * all optional.  Only call them if you want them.
             */


            /* flip BGR pixels to RGB */
            png_set_bgr(png_ptr);

            /* swap location of alpha bytes from ARGB to RGBA */
            png_set_swap_alpha(png_ptr);

            /* swap bytes of 16-bit files to most significant byte first */
            png_set_swap(png_ptr);

            /* swap bits of 1, 2, 4 bit packed pixel formats */
            png_set_packswap(png_ptr);

#endif /* (0) */

            /* Make up a table of pointers to the image rows */
            imagePixels = RwImageGetPixels(image);
            imageStride = RwImageGetStride(image);

            /* Make up a table of pointers to the image rows */
            i = sizeof(png_bytep) * height;
            rowPointers = (png_bytepp) PNGMalloc(NULL, i);
            if (!rowPointers)
            {
                png_destroy_write_struct((png_structpp) & png_ptr,
                                         (png_infopp) NULL);
                RwStreamClose((RwStream *) stream, NULL);
                RWRETURN(NULL);
            }

            for (i = 0; i < (signed) height; i++)
            {
                rowPointers[i] = imagePixels + (i * imageStride);
            }

            png_write_image(png_ptr, rowPointers);

            /* You can write optional chunks like tEXt, zTXt, and tIME
             * at the end as well.
             */

            /* It is REQUIRED to call this to finish writing
             * the rest of the file */
            png_write_end(png_ptr, info_ptr);

            /* if you malloced the palette, free it here */
            if (palette)
            {
                PNGFree(png_ptr, palette);
                PNGFree(png_ptr, trans);
            }

            /* if you allocated any text comments, free them here */

            /* clean up after the write, and free any memory allocated */
            PNGFree(NULL, rowPointers);

            png_destroy_write_struct((png_structpp) & png_ptr,
                                     (png_infopp) &info_ptr);

            /* close the file */
            RwStreamClose((RwStream *) stream, NULL);
        }
        else
        {
            /* We failed to open the stream for writing */
            RWRETURN((RwImage *)NULL);
        }
    }

    RWRETURN((RwImage *) image);
}


typedef union RwUnionStream RwUnionStream;
union RwUnionStream
{
    RwStream             * Stream;
    volatile RwStream    * VolatileStream;
};

typedef union RwUnionPNGUInt32 RwUnionPNGUInt32;
union RwUnionPNGUInt32
{
    png_uint_32            UInt32;
    volatile png_uint_32   VolatileUInt32;
};

/**
 * \ingroup rtpng
 * \ref RtPNGImageRead is used to read a Portable Network Graphics format
 * file from disk.  It reads a PNG file  using libpng of
 * http://www.libpng.org/pub/png
 * and return as an RwImage.
 *
 * The file name can either be absolute or relative to the
 * directory the application is running from.  The image search path is not
 * used to find image files and no gamma correction is applied to the image
 * as it is read.
 *
 * Note that the include file rtpng.h is required and must be included by an
 * application wishing to load PNG format files.  The PNG library is contained
 * in the file rtpng.lib.
 *
 * \param imageName Pointer to the filename to load.
 *
 * \return RwImage * on success, or NULL on failure.
 *
 * \see RtPNGImageWrite
 * \see RwImageRead
 * \see RwImageWrite
 * \see RwImageRegisterImageFormat
 */
RwImage            *
RtPNGImageRead(const RwChar * imageName)
{
	RwImage            *image = NULL;

	RWAPIFUNCTION(RWSTRING("RtPNGImageRead"));
    RWASSERT(imageName);

    if (imageName)
    {
        RwUnionStream UnionStream;

        UnionStream.VolatileStream =
            RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, imageName);

        if (UnionStream.VolatileStream)
        {
			image = RtPNGImageReadStream(UnionStream.Stream);
			RwStreamClose(UnionStream.Stream, NULL);

			return image;
        }

        /* Failed to open the file */
        RWRETURN(NULL);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN(NULL);
}

RwImage *RtPNGImageReadStream(RwStream * stream)
{
    RwStream           *unqualified_stream = stream;
    RwImage            *image;
    int                 bit_depth, color_type, interlace_type;
    volatile png_bytepp rowPointers = NULL;
    volatile png_infop  info_ptr = NULL;
    volatile png_structp png_ptr = NULL;
    volatile png_uint_32 depth = 0;
    RwUnionPNGUInt32 width;
    RwUnionPNGUInt32 height;

    RWAPIFUNCTION(RWSTRING("RtPNGImageReadStream"));

    width.VolatileUInt32 = 0;
    height.VolatileUInt32 = 0;

    /* Create and initialize the png_struct with the desired error
        * handler functions.  If you want to use the default stderr and
        * longjump method, you can supply NULL for the last three
        * parameters.  We also supply the the compiler header file
        * version, so that we know if the application was compiled
        * with a compatible version of the library.
        */
    png_ptr =
        png_create_read_struct_2(PNG_LIBPNG_VER_STRING, NULL,
                                    NULL, NULL, NULL, PNGMalloc,
                                    PNGFree);

    if (png_ptr == NULL)
    {
        RWRETURN(NULL);
    }

    /* Allocate/initialize the memory for image information. */
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        png_destroy_read_struct((png_structpp) & png_ptr,
                                (png_infopp) NULL,
                                (png_infopp) NULL);
        RWRETURN(NULL);
    }

#ifndef PNG_SETJMP_NOT_SUPPORTED
    /* Set error handling if you are using the setjmp/longjmp method
        * (this is the normal method of doing things with libpng).
        */
    if (setjmp(png_ptr->jmpbuf))
    {
        /* If we get here, we had a problem reading the file, so tidy
            * up.
            */
        if (rowPointers)
        {
            PNGFree(NULL, (png_voidp) rowPointers);
            rowPointers = NULL;
        }

        unqualified_stream = NULL;

        png_destroy_read_struct((png_structpp) & png_ptr,
                                (png_infopp) & info_ptr,
                                (png_infopp) NULL);
        png_ptr = NULL;
        info_ptr = NULL;

        RWRETURN(NULL);
    }
#endif /* PNG_SETJMP_NOT_SUPPORTED */

    /* If you are using replacement read functions, instead of calling
        * png_init_io() here you would call
        */
    png_set_read_fn(png_ptr, (void *) unqualified_stream,
                    PNGRead);

    /* The call to png_read_info() gives us all of the information
        * from the PNG file before the first IDAT (image data chunk).
        */
    png_read_info(png_ptr, info_ptr);

    png_get_IHDR(png_ptr,
                    info_ptr,
                    &width.UInt32,
                    &height.UInt32,
                    &bit_depth,
                    &color_type, &interlace_type, NULL, NULL);

    /* Strip down to 8 bit */
    png_set_strip_16(png_ptr);

    /* Set up the data transformations to go to the correct format  */
    switch (color_type)
    {
            /* Expand paletted colors into true RGB triplets */
        case PNG_COLOR_TYPE_PALETTE:
            depth = 8;
            if (bit_depth < 8)
            {
                /* One byte per pixel regardless,
                    * then treat as 8 bit */
                png_set_packing(png_ptr);
                depth = 4; /* We can get away with 4 bit images here */
            }
            break;

            /* Expand Grey Scales up to palettised and
                * make up a palette */
        case PNG_COLOR_TYPE_GRAY:
            depth = 8;
            if (bit_depth < 8)
            {
                /* Can't handle grey scale image less than 8-bit */
                png_set_gray_1_2_4_to_8(png_ptr);
            }
            break;

            /* Grey with an Alpha channel has to be
                * expanded to full rgb */
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            depth = 32;
            png_set_gray_to_rgb(png_ptr);
            break;

            /* Add filler byte (for alpha) after each RGB triplet */
        case PNG_COLOR_TYPE_RGB:
            depth = 32;
            png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
            break;

            /* Don't need to do much here */
        case PNG_COLOR_TYPE_RGB_ALPHA:
            depth = 32;
            break;

            /* If we get here, then we might be in trouble */
        default:
            break;
    }

    /* Allocate the image */
    image = RwImageCreate((RwInt32) width.VolatileUInt32,
                            (RwInt32) height.VolatileUInt32,
                            (RwInt32) depth);
    if (image)
    {
        RwInt32             i;
        RwUInt8            *imagePixels;
        RwRGBA             *imagePallette;
        RwUInt32            imageStride;

        if (!RwImageAllocatePixels(image))
        {
            RwImageDestroy(image);
            image = NULL;
            unqualified_stream = NULL;

            RWRETURN(NULL);
        }

        imagePixels = RwImageGetPixels(image);
        imagePallette = RwImageGetPalette(image);
        imageStride = RwImageGetStride(image);

        /* Make up a table of pointers to the image rows */
        i = sizeof(png_bytep) * height.VolatileUInt32;
        rowPointers = (png_bytepp) PNGMalloc(NULL, i);
        if (!rowPointers)
        {
            RwImageFreePixels(image);
            RwImageDestroy(image);
            image = NULL;
            unqualified_stream = NULL;

            RWRETURN(NULL);
        }

        for (i = 0; i < (signed) height.VolatileUInt32; i++)
        {
            rowPointers[i] = imagePixels + (i * imageStride);
        }

        /*if ((depth == 8) && imagePallette) */
        if (((depth == 4) || (depth == 8)) && imagePallette)
        {
            RwInt32 numImagePalletteColors = (1<<depth);

            /* Default to a grayscale palette, then override with
                * anything in the png file.
                */
            for (i = 0; i < numImagePalletteColors; i++)
            {
                imagePallette[i].red =
                    imagePallette[i].green =
                    imagePallette[i].blue = (RwUInt8) i;
                imagePallette[i].alpha = 255;
            }

            /* Get the palette */
            if (png_get_valid(png_ptr, info_ptr, PNG_INFO_PLTE))
            {
                png_colorp          palette;
                int                 num_palette;

                png_get_PLTE(png_ptr, info_ptr, &palette,
                                &num_palette);

                /* Palette we're assigning was dimensioned according
                    to color depth */
                if (numImagePalletteColors < num_palette)
                {
                    num_palette = numImagePalletteColors;
                }

                for (i = 0; i < num_palette; i++)
                {
                    imagePallette[i].red = palette[i].red;
                    imagePallette[i].green = palette[i].green;
                    imagePallette[i].blue = palette[i].blue;
                }
            }

            /* We might have transparency info too... */
            if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
            {
                png_bytep           trans;
                int                 num_trans;
                png_color_16p       trans_values;

                png_get_tRNS(png_ptr, info_ptr, &trans,
                                &num_trans, &trans_values);

                for (i = 0; i < num_trans; i++)
                {
                    imagePallette[i].alpha = trans[i];
                }
            }
        }

        /* Now it's time to read the image. */
        png_read_image(png_ptr, (png_bytepp) rowPointers);

        /* read rest of file, and get additional chunks in info_ptr */
        png_read_end(png_ptr, info_ptr);

        /* clean up after the read, and free any memory allocated */
        png_destroy_read_struct((png_structpp) & png_ptr,
                                (png_infopp) & info_ptr,
                                (png_infopp) NULL);
        png_ptr = NULL;
        info_ptr = NULL;

        PNGFree(NULL, (png_structp) rowPointers);
        rowPointers = NULL;
    }

    unqualified_stream = NULL;

    RWRETURN(image);
}

