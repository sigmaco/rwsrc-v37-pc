/**
 * \ingroup rtras
 * \page rtrasoverview RtRAS Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rtras.h
 * \li \b Libraries: rwcore, rtras
 * \li \b Plugin \b attachments: None
 *
 * \subsection rasoverview Overview
 * The RtRAS toolkit provides reading and writing functions for the
 * RAS image format.
 *
 * Sun Raster is the native bitmap format of the Sun UNIX platforms.
 * A simple bitmap format with wide distribution, particularly in the
 * UNIX world.
 *
 * Supports paletted, 32 bit BGR and RGB color, and grayscale images.
 * This format does not support alpha channels.
 *
 * Copyright (c) Criterion Software Limited
 */

/**********************************************************************
 *
 * File :     rtras.c
 *
 * Abstract : Read RAS files in as RwImages
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

#include <string.h> /* Needed for memcpy & memset */

#include "rwcore.h"
#include "rpdbgerr.h"

/* Image handling */
#include "rtras.h"

/****************************************************************************
 Local Types
 */

/* Sun raster header format ------------------------------------------- */
typedef struct RwSunRasterHeader RwSunRasterHeader;
struct RwSunRasterHeader
{
    RwInt32             magic;
    RwInt32             width;
    RwInt32             height;
    RwInt32             bitsPerPixel;
    RwInt32             imageLength;
    RwInt32             type;
    RwInt32             mapType;
    RwInt32             mapLength;
};

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */

#define rwSUNRASTERMAGIC    0x956aa659

#define RWBYTESWAPINT32(int32var)                                  \
    {                                                              \
        RwUInt8     caTmp[4];                                      \
        RwUInt8     cTmp;                                          \
                                                                   \
        *(RwInt32 *) caTmp = (int32var);                           \
        cTmp = caTmp[0];                                           \
        caTmp[0] = caTmp[3];                                       \
        caTmp[3] = cTmp;                                           \
                                                                   \
        cTmp = caTmp[1];                                           \
        caTmp[1] = caTmp[2];                                       \
        caTmp[2] = cTmp;                                           \
                                                                   \
        (int32var) = *(RwInt32 *)caTmp;                            \
    }

#define RWBYTESWAPINT16(int16var)                                  \
    {                                                              \
        RwUInt8     caTmp[2];                                      \
        RwUInt8     cTmp;                                          \
                                                                   \
        *(RwInt16 *) caTmp = (int16var);                           \
        cTmp = caTmp[0];                                           \
        caTmp[0] = caTmp[1];                                       \
        caTmp[1] = cTmp;                                           \
                                                                   \
        (int16var) = *(RwInt16 *)caTmp;                            \
    }

/* ImageSetRASSpan helper definition */
#define GENSWITCHKEY(srcDepth, destDepth) (((srcDepth) << 8) | (destDepth))

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */


/****************************************************************************
 Utility Functions
 */

/*
 * ImageWriteRAS4or8
 */
static RwImage     *
ImageWriteRAS4or8(RwImage * image, RwStream * stream, RwInt32 stride)
{
    RwInt32             i;
    RwUInt8            *cpCur;
    RwRGBA              unGammaPalette[256];
    RwRGBA             *palToWrite;
    RwInt32             palSize;

    RWFUNCTION(RWSTRING("ImageWriteRAS4or8"));
    RWASSERT(image);
    RWASSERT(stream);
    RWASSERT(stride);

    palSize = (1 << image->depth);

    /********************* 8 bit raster *************************/

    /* Assume ungammacorrected first */
    palToWrite = image->palette;
    if (image->flags & rwIMAGEGAMMACORRECTED)
    {
        /* Write out the palette - ungamma correct it first */
        _rwImageGammaUnCorrectArrayOfRGBA(unGammaPalette,
                                          image->palette, palSize);
        palToWrite = unGammaPalette;
    }

    for (i = 0; i < palSize; i++)
    {
        if (!RwStreamWrite(stream, &palToWrite[i].red, sizeof(RwUInt8)))
        {
            RwStreamClose(stream, NULL);
            RWRETURN((RwImage *)NULL);
        }
    }
    for (i = 0; i < palSize; i++)
    {
        if (!RwStreamWrite
            (stream, &palToWrite[i].green, sizeof(RwUInt8)))
        {
            RwStreamClose(stream, NULL);
            RWRETURN((RwImage *)NULL);
        }
    }
    for (i = 0; i < palSize; i++)
    {
        if (!RwStreamWrite
            (stream, &palToWrite[i].blue, sizeof(RwUInt8)))
        {
            RwStreamClose(stream, NULL);
            RWRETURN((RwImage *)NULL);
        }
    }

    /* Write the actual data */
    cpCur = image->cpPixels;

    for (i = 0; i < image->height; i++)
    {
        if (!RwStreamWrite(stream, cpCur, stride))
        {
            RwStreamClose(stream, NULL);
            RWRETURN((RwImage *)NULL);
        }

        cpCur += image->stride;
    }

    RWRETURN(image);
}

/*
 * ImageWriteRAS32
 */
static RwImage     *
ImageWriteRAS32(RwImage * image, RwStream * stream, RwInt32 stride)
{
    RwInt32             i, j;
    RwInt32             nExtra = stride % 3;

    RWFUNCTION(RWSTRING("ImageWriteRAS32"));
    RWASSERT(image);
    RWASSERT(stream);
    RWASSERT(stride);

    /********************* 32 bit raster ************************/

    /* Write the actual data */
    for (i = 0; i < image->height; i++)
    {
        RwUInt8             caRGB[3];
        RwRGBA             *rpCur =
            (RwRGBA *) (image->cpPixels + (i * image->stride));

        for (j = 0; j < (stride / 3); j++)
        {
            if (image->flags & rwIMAGEGAMMACORRECTED)
            {
                RwRGBA              unGammaPixel;

                /* Gamma correction needed */
                _rwImageGammaUnCorrectArrayOfRGBA(&unGammaPixel,
                                                  &rpCur[j], 1);
                caRGB[2] = unGammaPixel.red;
                caRGB[1] = unGammaPixel.green;
                caRGB[0] = unGammaPixel.blue;
            }
            else
            {
                /* No ungamma correction needed */
                caRGB[2] = rpCur[j].red;
                caRGB[1] = rpCur[j].green;
                caRGB[0] = rpCur[j].blue;
            }

            if (!RwStreamWrite(stream, caRGB, 3))
            {
                RwStreamClose(stream, NULL);
                RWRETURN((RwImage *)NULL);
            }
        }
        if (nExtra)
        {
            if (!RwStreamWrite(stream, caRGB, nExtra))
            {
                RwStreamClose(stream, NULL);
                RWRETURN((RwImage *)NULL);
            }
        }
    }

    RWRETURN(image);
}

/****************************************************************************
 ImageSetRASSpan

 Expands a span into the appropriate format

 On entry   : Image
            : Y coord of span
            : Data
            : Depth
            : TRUE if RGB (or GBR)
 On exit    : TRUE on success
 */
static              RwBool
ImageSetRASSpan(RwImage * image, RwInt32 y, RwUInt8 * cpSpan,
               RwInt32 depth, RwBool bRGB)
{
    RwUInt8             cCur;
    RwInt32             shift;
    RwRGBA             *rpPal = image->palette;
    RwInt32             i = image->width;
    RwInt32             j;
    RwUInt32            switchKey;
    RwUInt8            *cpDestin;
    RwRGBA             *rpDestin;

    RWFUNCTION(RWSTRING("ImageSetRASSpan"));
    RWASSERT(image);
    RWASSERT(cpSpan);

    cpDestin = (RwUInt8 *) (image->cpPixels + (image->stride * y));
    rpDestin = (RwRGBA *) cpDestin;

    switchKey = GENSWITCHKEY(depth, image->depth);
    switch (switchKey)
    {
        case (GENSWITCHKEY(1, 4)): /* 1 to 4 bit */
        case (GENSWITCHKEY(1, 8)): /* 1 to 8 bit */
            {
                shift = 7;
                cCur = *cpSpan++;

                while (i--)
                {
                    (*cpDestin) = (cCur >> shift) & 1;

                    cpDestin++;
                    shift--;

                    /* Run out of bits? */
                    if (shift < 0)
                    {
                        shift = 7;
                        cCur = *cpSpan++;
                    }
                }

                break;
            }
        case (GENSWITCHKEY(4, 4)): /* 4 to 4 bit */
        case (GENSWITCHKEY(4, 8)): /* 4 to 8 bit */
            {
                shift = 4;
                cCur = *cpSpan++;

                while (i--)
                {
                    (*cpDestin) = (cCur >> shift) & 0xf;

                    cpDestin++;
                    shift -= 4;

                    /* Run out of bits? */
                    if (shift < 0)
                    {
                        shift = 4;
                        cCur = *cpSpan++;
                    }
                }

                break;
            }
        case (GENSWITCHKEY(8, 8)): /* 8 to 8 bit */
            {
                memcpy(cpDestin, cpSpan, i);
                break;
            }
        case (GENSWITCHKEY(1, 32)):
            {
                shift = 7;
                cCur = *cpSpan++;

                while (i--)
                {
                    (*rpDestin) = rpPal[(cCur >> shift) & 1];

                    rpDestin++;
                    shift--;

                    /* Run out of bits? */
                    if (shift < 0)
                    {
                        shift = 7;
                        cCur = *cpSpan++;
                    }
                }

                break;
            }
        case (GENSWITCHKEY(4, 32)):
            {
                shift = 4;
                cCur = *cpSpan++;

                while (i--)
                {
                    (*rpDestin) = rpPal[(cCur >> shift) & 0xf];

                    rpDestin++;
                    shift -= 4;

                    /* Run out of bits? */
                    if (shift < 0)
                    {
                        shift = 4;
                        cCur = *cpSpan++;
                    }
                }

                break;
            }
        case (GENSWITCHKEY(8, 32)):
            {
                while (i--)
                {
                    *rpDestin++ = rpPal[*cpSpan++];
                }

                break;
            }
        case (GENSWITCHKEY(15, 32)):
            {

                while (i--)
                {
                    j = *(RwUInt16 *) cpSpan;

                    rpDestin->red = (RwUInt8) ((j >> 7) & 0xf8);
                    rpDestin->green = (RwUInt8) ((j >> 2) & 0xf8);
                    rpDestin->blue = (RwUInt8) ((j << 3) & 0xf8);
                    rpDestin->alpha = (RwUInt8) (0xff);

                    cpSpan += 2;
                    rpDestin++;
                }
                break;
            }
        case (GENSWITCHKEY(16, 32)):
            {
                while (i--)
                {
                    j = *(RwUInt16 *) cpSpan;

                    rpDestin->red = (RwUInt8) ((j >> 8) & 0xf8);
                    rpDestin->green = (RwUInt8) ((j >> 3) & 0xfc);
                    rpDestin->blue = (RwUInt8) ((j << 3) & 0xf8);
                    rpDestin->alpha = (RwUInt8) (0xff);

                    cpSpan += 2;
                    rpDestin++;
                }
                break;
            }
        case (GENSWITCHKEY(24, 32)):
            {
                if (bRGB)
                {
                    while (i--)
                    {
                        rpDestin->red = (*(cpSpan++));
                        rpDestin->green = (*(cpSpan++));
                        rpDestin->blue = (*(cpSpan++));
                        rpDestin->alpha = (0xff);

                        rpDestin++;
                    }
                }
                else
                {
                    while (i--)
                    {
                        rpDestin->blue = (*(cpSpan++));
                        rpDestin->green = (*(cpSpan++));
                        rpDestin->red = (*(cpSpan++));
                        rpDestin->alpha = (0xff);

                        rpDestin++;
                    }
                }

                break;
            }
        case (GENSWITCHKEY(32, 32)):
            {
                while (i--)
                {
                    rpDestin->red = (*cpSpan++);
                    rpDestin->green = (*cpSpan++);
                    rpDestin->blue = (*cpSpan++);
                    rpDestin->alpha = (0xff);

                    cpSpan++;
                    rpDestin++;
                }
                break;
            }
            /* Unhandled format or conversion */
        default:
            {
                RWERROR((E_RW_INVIMAGEDEPTH));
                RWRETURN(FALSE);
            }
    }

    RWRETURN(TRUE);
}

/**
 * \ingroup rtras
 * \ref RtRASImageRead is used to read a Sun Raster format from disk.  The
 * filename can either be absolute or relative to the directory the application
 * is running from.  The image search path is not used to find image files and
 * no gamma correction is applied to the image as it is read.
 *
 * \param imageName  Pointer to the filename of the RAS file to read.
 *
 * \return Returns an image on success, or NULL on failure.
 *
 * \see RtRASImageWrite
 * \see RwImageRead
 * \see RwImageWrite
 * \see RwImageRegisterImageFormat
 */
RwImage            *
RtRASImageRead(const RwChar * imageName)
{
    RWAPIFUNCTION(RWSTRING("RtRASImageRead"));
    RWASSERT(imageName);

    {
        RwStream           *stream;

        stream =
            RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, imageName);
        if (stream)
        {
            RwSunRasterHeader   srHeader;
            RwInt32             i, j;
            RwImage            *image;
            RwInt32             nOutDepth;
            RwBool              bRGB = FALSE;
            RwInt32             stride;
            RwUInt8            *scanLineBuf;
            RwUInt32            size;

            /* Test to make sure it's a RAS file */
            if (RwStreamRead(stream, &srHeader, sizeof(srHeader)) !=
                sizeof(srHeader))
            {
                RwStreamClose(stream, NULL);
                RWRETURN((RwImage *)NULL);
            }

            /* Warning, Sun raster file is BIGENDIAN so only convert first word */
            (void)RwMemNative32(&srHeader, sizeof(RwInt32));

            /* Is this a sun raster ? */
            if (srHeader.magic != (RwInt32) rwSUNRASTERMAGIC)
            {
                /* It's not the right type */
                RwStreamClose(stream, NULL);
                RWRETURN((RwImage *)NULL);
            }

#ifdef rwLITTLEENDIAN
            /* Make the header little endian */
            {
                RwInt32            *npData = (RwInt32 *) & srHeader;

                for (i = sizeof(srHeader) / sizeof(RwInt32); i; i--)
                {
                    RWBYTESWAPINT32((*npData));

                    npData++;
                }
            }
#endif /* rwLITTLEENDIAN */

            /* Calculate length if necessary */
            if (srHeader.imageLength)
            {
                srHeader.imageLength = srHeader.width * srHeader.height;
            }

            /* Create the image and allocate it */
            switch (srHeader.bitsPerPixel)
            {
                case 1:
                case 4:
                    {
                        nOutDepth = 4;
                        break;
                    }
                case 8:
                    {
                        nOutDepth = 8;
                        break;
                    }
                case 24:
                case 32:
                    {
                        nOutDepth = 32;
                        break;
                    }
                default:
                    {
                        RWERROR((E_RW_INVIMAGEFORMAT));
                        RwStreamClose(stream, NULL);
                        RWRETURN((RwImage *)NULL);
                    }
            }

            /* Create the image */
            image = RwImageCreate(srHeader.width, srHeader.height,
                                  nOutDepth);
            if (!image)
            {
                RWRETURN((RwImage *)NULL);
            }

            /* Allocate memory */
            if (!RwImageAllocatePixels(image))
            {
                RwImageDestroy(image);
                RwStreamClose(stream, NULL);
                RWRETURN((RwImage *)NULL);
            }

            /* Handle on header start */
            if (srHeader.mapType && srHeader.mapLength)
            {
                RwUInt8            *cpClutBuffer;
                RwInt32             nNext;

                cpClutBuffer = (RwUInt8 *) RwMalloc(srHeader.mapLength,
                            rwID_RASIMAGEPLUGIN | rwMEMHINTDUR_FUNCTION);

                if (!cpClutBuffer)
                {
                    RWERROR((E_RW_NOMEM, srHeader.mapLength));
                    RwImageFreePixels(image);
                    RwImageDestroy(image);
                    RwStreamClose(stream, NULL);
                    RWRETURN((RwImage *)NULL);
                }

                /* Read in the palette */
                size = srHeader.mapLength;
                if (RwStreamRead(stream, cpClutBuffer, size) != size)
                {
                    RwImageFreePixels(image);
                    RwImageDestroy(image);
                    RwStreamClose(stream, NULL);
                    RWRETURN((RwImage *)NULL);
                }

                /* Convert */
                switch (srHeader.mapType)
                {
                    case 1:
                        {
                            /* Straight RGB palette */
                            nNext = srHeader.mapLength / 3;
                            for (i = 0; i < nNext; i++)
                            {
                                (image->palette)[i].red =
                                    cpClutBuffer[i];
                                (image->palette)[i].green =
                                    cpClutBuffer[i + nNext];
                                (image->palette)[i].blue =
                                    cpClutBuffer[i + (nNext << 1)];
                                (image->palette)[i].alpha = 0xff;
                            }
                            break;
                        }
                    case 2:
                        {
                            /* Greyscale palette */
                            for (i = 0; i < srHeader.mapLength; i++)
                            {
                                (image->palette)[i].red =
                                    cpClutBuffer[i];
                                (image->palette)[i].green =
                                    cpClutBuffer[i];
                                (image->palette)[i].blue =
                                    cpClutBuffer[i];
                                (image->palette)[i].alpha = 0xff;
                            }
                            break;
                        }
                    default:
                        {
                            break;
                        }
                }

                /* No longer needed */

                RwFree(cpClutBuffer);
            }
            else
            {
                /* Black and white image */
                if (srHeader.bitsPerPixel == 1)
                {
                    (image->palette)[0].red = 0;
                    (image->palette)[0].green = 0;
                    (image->palette)[0].blue = 0;
                    (image->palette)[0].alpha = 255;

                    (image->palette)[1].red = 255;
                    (image->palette)[1].green = 255;
                    (image->palette)[1].blue = 255;
                    (image->palette)[1].alpha = 255;

                    srHeader.mapLength = 2;
                }
                else
                {
                    /* Greyscale ramp */
                    if (srHeader.bitsPerPixel == 8)
                    {
                        for (i = 0; i < 256; i++)
                        {
                            (image->palette)[i].red = (RwUInt8) i;
                            (image->palette)[i].green = (RwUInt8) i;
                            (image->palette)[i].blue = (RwUInt8) i;
                        }
                        srHeader.mapLength = 256;
                    }
                }
            }

            /* Calculate stride */
            stride = (srHeader.width * srHeader.bitsPerPixel + 7) / 8;
            stride = (stride + 1) & -2;

            /* Allocate scanline buffer */
            scanLineBuf = (RwUInt8 *) RwMalloc(stride,
                rwID_RASIMAGEPLUGIN | rwMEMHINTDUR_FUNCTION);

            if (!scanLineBuf)
            {
                RWERROR((E_RW_NOMEM, stride));
                RwImageFreePixels(image);
                RwImageDestroy(image);
                RwStreamClose(stream, NULL);
                RWRETURN((RwImage *)NULL);
            }

            switch (srHeader.type)
            {
                case 3:
                    {
                        bRGB = TRUE; /* its BGR */
                        /*FALLTHROUGH */
                    }
                case 0:
                    {
                        /*FALLTHROUGH */
                    }
                case 1:
                    {
                        for (i = 0; i < srHeader.height; i++)
                        {
                            if (RwStreamRead
                                (stream, scanLineBuf,
                                 stride) != (RwUInt32) stride)
                            {
                                RwFree(scanLineBuf);
                                RwImageFreePixels(image);
                                RwImageDestroy(image);
                                RwStreamClose(stream, NULL);
                                RWRETURN((RwImage *)NULL);
                            }

                            ImageSetRASSpan(image, i, scanLineBuf,
                                           srHeader.bitsPerPixel, bRGB);
                        }
                        break;
                    }
                case 2:
                    {
                        RwUInt8             cPixel;
                        RwUInt8            *cpCur;

                        i = srHeader.height;
                        j = stride; /* X position */
                        cpCur = scanLineBuf;

                        while (i)
                        {
                            if (RwStreamRead(stream, &cPixel, 1) != 1)
                            {
                                RwFree(scanLineBuf);
                                RwImageFreePixels(image);
                                RwImageDestroy(image);
                                RwStreamClose(stream, NULL);
                                RWRETURN((RwImage *)NULL);
                            }

                            if (cPixel != 0x80)
                            {
                                (*cpCur) = cPixel;
                                cpCur++;

                                if (j-- <= 0)
                                {
                                    ImageSetRASSpan(image, i,
                                                   scanLineBuf,
                                                   srHeader.
                                                   bitsPerPixel, bRGB);

                                    j = stride; /* X position */
                                    i--;
                                }
                            }
                            else
                            {
                                RwUInt8             cCount;
                                RwInt32             nCount;

                                if (RwStreamRead(stream, &cCount, 1) !=
                                    1)
                                {
                                    RwFree(scanLineBuf);
                                    RwImageFreePixels(image);
                                    RwImageDestroy(image);
                                    RwStreamClose(stream, NULL);
                                    RWRETURN((RwImage *)NULL);
                                }

                                if (cCount == 0)
                                {
                                    cPixel = 128;
                                }
                                else
                                {
                                    if (RwStreamRead(stream, &cPixel, 1)
                                        != 1)
                                    {
                                        RwFree(scanLineBuf);
                                        RwImageFreePixels(image);
                                        RwImageDestroy(image);
                                        RwStreamClose(stream, NULL);
                                        RWRETURN((RwImage *)NULL);
                                    }
                                }

                                nCount = cCount;

                                while (nCount-- >= 0)
                                {
                                    (*cpCur) = cPixel;
                                    cpCur++;

                                    if (j-- <= 0)
                                    {
                                        ImageSetRASSpan(image, i,
                                                       scanLineBuf,
                                                       srHeader.
                                                       bitsPerPixel,
                                                       bRGB);
                                        j = stride; /* X position */
                                        i--;
                                    }
                                }
                            }
                        }

                        break;
                    }
                default:
                    {
                        RWERROR((E_RW_INVIMAGEFORMAT));
                        RwFree(scanLineBuf);
                        RwImageFreePixels(image);
                        RwImageDestroy(image);
                        RwStreamClose(stream, NULL);
                        RWRETURN((RwImage *)NULL);
                    }
            }

            /* No longer need the scan line */
            RwFree(scanLineBuf);

            /* Finished with this */
            RwStreamClose(stream, NULL);

            /* All done */
            RWRETURN(image);
        }

        /* Couldn't find the file */
        RWRETURN((RwImage *)NULL);
    }
}

/**
 * \ingroup rtras
 * \ref RtRASImageWrite is used to write an image as a RAS file.
 *
 * The image file name can either be absolute or relative to the directory
 * the application is running from.
 *
 * Note that if the image has been gamma corrected using
 * \ref RwImageGammaCorrect, the gamma correction is removed from the image
 * before writing it to disk. Inverse gamma correction is performed using
 * the current gamma correction value.
 *
 * \param image  Pointer to the image to write.
 * \param imageName  Pointer to the filename of the RAS file to read.
 *
 * \return Returns an image on success, or NULL on failure.
 *
 * \see RtRASImageRead
 * \see RwImageRead
 * \see RwImageWrite
 * \see RwImageRegisterImageFormat
 */
RwImage            *
RtRASImageWrite(RwImage * image, const RwChar * imageName)
{
    RWAPIFUNCTION(RWSTRING("RtRASImageWrite"));
    RWASSERT(image);
    RWASSERT(imageName);

    {
        RwStream           *stream;

        stream =
            RwStreamOpen(rwSTREAMFILENAME, rwSTREAMWRITE, imageName);
        if (stream)
        {
            RwSunRasterHeader   srHeader;
            RwInt32             stride;
            RwInt32             i;

            switch (image->depth)
            {
                case 4:
                case 8:
                    {
                        srHeader.bitsPerPixel = image->depth;
                        srHeader.mapLength = (1 << image->depth) * 3;
                        break;
                    }
                case 32:
                    {
                        srHeader.bitsPerPixel = 24;
                        srHeader.mapLength = 0;
                        break;
                    }
                default:
                    {
                        RWERROR((E_RW_INVIMAGEDEPTH));
                        RWRETURN((RwImage *)NULL);
                    }
            }

            /* Set up the header */
            srHeader.width = image->width;
            srHeader.height = image->height;
            srHeader.type = 1; /* No compression ! */
            srHeader.mapType = 1; /* RGB ... */

            /* Calculate the stride */
            stride = (srHeader.width * srHeader.bitsPerPixel + 7) / 8;
            stride = (stride + 1) & -2;

            srHeader.imageLength = stride * srHeader.height;

            /* Put in correct format */
            {
                RwInt32            *npData = (RwInt32 *) & srHeader;

                for (i = sizeof(srHeader) / sizeof(RwInt32); i; i--)
                {
                    RWBYTESWAPINT32((*npData));

                    npData++;
                }
            }

            /* Don't flip this around */
            srHeader.magic = rwSUNRASTERMAGIC;

            /* Ready to write */
            if (!RwStreamWrite
                (stream, &srHeader, sizeof(RwSunRasterHeader)))
            {
                RWRETURN((RwImage *)NULL);
            }

            /* Deal with the data */
            switch (image->depth)
            {
                case 4:
                case 8:
                    {
                        image =
                            ImageWriteRAS4or8(image, stream, stride);
                        break;
                    }

                case 32:
                    {
                        image =
                            ImageWriteRAS32(image, stream, stride);
                        break;
                    }
            }

            RwStreamClose(stream, NULL);
            RWRETURN(image);
        }

        /* Failed to open the stream */
        RWRETURN((RwImage *)NULL);
    }
}
