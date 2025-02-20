/**********************************************************************
 *
 * File :     rtbmp.c
 *
 * Abstract : Read BMP files in as RwImages
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

/**
 * \ingroup rtbmp
 * \page rtbmpoverview RtBMP Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rtbmp.h
 * \li \b Libraries: rwcore, rtbmp
 * \li \b Plugin \b attachments: None
 *
 * \subsection bmpoverview Overview
 * The RtBMP toolkit provides reading and writing functions for the
 * BMP image format.
 *
 * BMP stands for Bitmap, and is a standard Microsoft Windows image format.
 * Supports paletted, 24 bit RGB color, and greyscale images. This format
 * does not support alpha channels.
 *
 * 20/03/03: Added support for loading 32 bit RGB color BMPs, it's unclear
 * whether these are valid in the BMP format without extended Bitmap headers
 * specifying bit masks. However adding support was a trivial case of fixing
 * the hard coded 3 byte per pixel scanline malloc which also has the benefit
 * of reducing memory overhead for 4 and 8 bit Bitmap reading
 *
 */

/****************************************************************************
 Includes
 */

#include <string.h> /* Needed for memcpy & memset */

#include "rwcore.h"
#include "rpdbgerr.h"

/* Image handling */
#include "rtbmp.h"


/****************************************************************************
 Local Types
 */

/* BMP file  header format ------------------------------------------- */
typedef struct RwBmpHeader RwBmpHeader;
struct RwBmpHeader
{
    RwUInt8             caDummy[2];
    RwInt32             nFileSize;
    RwInt16             nResv1;
    RwInt16             nResv2;
    RwInt32             nOffset;

    RwInt32             nHeadSize;
    RwInt32             width;
    RwInt32             height;
    RwInt16             nPlanes;
    RwInt16             nBitsPixel;

    RwInt32             nCompress;
    RwInt32             nSize;
    RwInt32             nPixMeterX;
    RwInt32             nPixMeterY;
    RwInt32             nColsUsed;
    RwInt32             nImportantCols;
};

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */

/* Magic definitions */
#define rwBMPMAGIC          0x00004d42
#define rwBMPMAGICMASK      0x0000ffff

/* Helper definitions */
#define rwPutUInt8(stream, v_int)                               \
MACRO_START                                                     \
{                                                               \
    RwUInt8 v = (RwUInt8) (v_int);                              \
                                                                \
    RwStreamWrite(stream, &v, sizeof(v));                       \
}                                                               \
MACRO_STOP

#define rwPutUInt16(stream, v_int)                              \
MACRO_START                                                     \
{                                                               \
    RwUInt16 v = (RwUInt16) (v_int);                            \
                                                                \
    RwStreamWriteInt16(stream, (const RwInt16 *)&v, sizeof(v)); \
}                                                               \
MACRO_STOP

#define rwPutUInt32(stream, v_int)                              \
MACRO_START                                                     \
{                                                               \
    RwUInt32 v = (RwUInt32) (v_int);                            \
                                                                \
    RwStreamWriteInt32(stream, (const RwInt32 *)&v, sizeof(v));   \
}                                                               \
MACRO_STOP

/* Get numbers in little endian [intel] format */

#define CHAROFF(p, off)     ((RwUInt8 *)(p) + (off))

#ifdef rwLITTLEENDIAN

/* We can just pull from memory */
#define RWRWINT32GETLITTLE(p, off)  (*(RwInt32 *)(CHAROFF(p, off)))
#define RWRWINT16GETLITTLE(p, off)  (*(RwInt16 *)(CHAROFF(p, off)))

#else /* rwLITTLEENDIAN */

/* We have to shuffle the bits */
#define RWRWINT32GETLITTLE(p, off)                              \
        ((((RwInt32)(*CHAROFF(p, off))))         |              \
         (((RwInt32)(*CHAROFF(p, off+1))) << 8)  |              \
         (((RwInt32)(*CHAROFF(p, off+2))) << 16) |              \
         (((RwInt32)(*CHAROFF(p, off+3))) << 24))

#define RWRWINT16GETLITTLE(p, off)                              \
        ((((RwInt16)(*CHAROFF(p, off))))         |              \
         (((RwInt16)(*CHAROFF(p, off+1))) << 8))

#endif /* rwLITTLEENDIAN */


/* BMPImageSetSpan helper definition */
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
 ***********************************************************************
 * BMP output
 */


static              RwUInt32
BMPLenRGBTable(RwUInt32 bpp)
{
    RWFUNCTION(RWSTRING("BMPLenRGBTable"));

    if (bpp <= 8)
    {
        RWRETURN((1<<bpp) * 4);
    }
    else
    {
        RWRETURN(0);
    }
}

/* return the offset to the BMP image bits */
static              RwUInt32
BMPOffBits(RwUInt32 bpp)
{
    RwUInt32            result;

    RWFUNCTION(RWSTRING("BMPOffBits"));

    result = 14 +   /* header */
             40 +   /* info */
             BMPLenRGBTable(bpp);

    RWRETURN(result);
}

/*
 * length, in bytes, of a line of the image
 *
 * Evidently each row is padded on the right as needed to make it a
 * multiple of 4 bytes long.  This appears to be true of both
 * OS/2 and Windows BMP files.
 */
static              RwUInt32
BMPLenLine(RwUInt32 bpp, RwUInt32 w)
{
    RWFUNCTION(RWSTRING("BMPLenLine"));

    RWRETURN((((w * bpp) + 31) & -32) >> 3);
}

/* return the number of bytes used to store the image bits */
static              RwUInt32
BMPLenBits(RwUInt32 bpp, RwUInt32 w, RwUInt32 h)
{
    RWFUNCTION(RWSTRING("BMPLenBits"));

    RWRETURN(h * BMPLenLine(bpp, w));
}

/* return the size of the BMP file in bytes */
static              RwUInt32
BMPLenFile(RwUInt32 bpp, RwImage *image)
{
    RwUInt32            result;

    RWFUNCTION(RWSTRING("BMPLenFile"));

    result = BMPOffBits(bpp) +
             BMPLenBits(bpp, RwImageGetWidth(image), RwImageGetHeight(image));

    RWRETURN(result);
}

/*
 * BMP writing
 */

/*
 * returns the number of bytes written.
 */
static              RwInt32
BMPWriteFileHeader(RwStream * stream, RwUInt32 bpp, RwImage *image)
{
    RWFUNCTION(RWSTRING("BMPWriteFileHeader"));

    rwPutUInt8(stream, 'B');
    rwPutUInt8(stream, 'M');

    /* cbSize */
    rwPutUInt32(stream, (RwUInt32)BMPLenFile(bpp, image));

    /* xHotSpot */
    rwPutUInt16(stream, 0);

    /* yHotSpot */
    rwPutUInt16(stream, 0);

    /* offBits */
    rwPutUInt32(stream, (RwUInt32) BMPOffBits(bpp));

    RWRETURN(14);
}

/*
 * returns the number of bytes written.
 */
static              RwInt32
BMPWriteInfoHeader(RwStream * stream, RwUInt32 bpp, RwImage *image)
{
    RwInt32 cbFix;

    RWFUNCTION(RWSTRING("BMPWriteInfoHeader"));

    cbFix = 40;
    rwPutUInt32(stream, cbFix);

    /* cx */
    rwPutUInt32(stream, (RwUInt32) RwImageGetWidth(image));
    /* cy */
    rwPutUInt32(stream, (RwUInt32) RwImageGetHeight(image));
    /* cPlanes */
    rwPutUInt16(stream, 1);
    /* cBitCount */
    rwPutUInt16(stream, (RwUInt16) bpp);

    /*
     * We've written 16 bytes so far, need to write 24 more
     * for the required total of 40.
     */
    rwPutUInt32(stream, 0);
    rwPutUInt32(stream, 0);
    rwPutUInt32(stream, 0);
    rwPutUInt32(stream, 0);

    if (bpp <= 8)
    {
        RwRGBA *pal = RwImageGetPalette(image);
        RwUInt8 col[4];
        RwInt32 x;

        rwPutUInt32(stream, 1<<bpp);
        rwPutUInt32(stream, 1<<bpp);
        for (x=0; x<1<<bpp; x++)
        {
            col[0] = pal[x].blue;
            col[1] = pal[x].green;
            col[2] = pal[x].red;
            col[3] = 255;
            RwStreamWrite(stream, col, sizeof(col));
            cbFix += sizeof(col);
        }
    }
    else
    {
        rwPutUInt32(stream, 0);
        rwPutUInt32(stream, 0);
    }

    RWRETURN(cbFix);
}

/*
 * returns the number of bytes written.
 */
static              RwInt32
BMPWriteRow(RwStream * stream,
            RwImage * image,
            RwInt32 y,
            RwUInt32 bpp, RwUInt8 *packed)
{
    RwUInt32            nbyte = 0;
    RwInt32            x;

    RWFUNCTION(RWSTRING("BMPWriteRow"));

    /* Write into the array, then write whole array */
    if (bpp == 4)
    {
        RwUInt8 *pixel = (RwUInt8 *)(RwImageGetPixels(image) + y * RwImageGetStride(image));

        for (x=0; x<RwImageGetWidth(image); x+=2)
        {
            packed[x>>1] = pixel[x]<<4 | pixel[x+1];
        }
        nbyte += RwImageGetWidth(image) >> 1;
    }
    else
    if (bpp == 8)
    {
        RwUInt8 *pixel = (RwUInt8 *)(RwImageGetPixels(image) + y * RwImageGetStride(image));

        memcpy(packed, pixel, RwImageGetWidth(image));
        nbyte += RwImageGetWidth(image);
    }
    else
    {
        RwRGBA *pixel = (RwRGBA *)(RwImageGetPixels(image) + y * RwImageGetStride(image));
        for (x = 0; x < RwImageGetWidth(image); x++)
        {
            packed[x*3+2] = pixel[x].red;
            packed[x*3+1] = pixel[x].green;
            packed[x*3+0] = pixel[x].blue;
        }
        nbyte += RwImageGetWidth(image) * 3;
    }

    RwStreamWrite(stream, packed, nbyte);

    /*
     * Make sure we write a multiple of 4 bytes.
     */
    while (nbyte & 3)
    {
        rwPutUInt8(stream, 0);
        nbyte++;
    }

    RWRETURN(nbyte);
}

/*
 * returns the number of bytes written, or -1 on error.
 */
static              RwInt32
BMPWriteBits(RwStream * stream, RwUInt32 bpp, RwImage * image)
{
    RwInt32             nbyte = 0;
    RwInt32             y;
    RwUInt8             *packed;

    RWFUNCTION(RWSTRING("BMPWriteBits"));

    /* Try to malloc an array of pixels for the row */
    packed = (RwUInt8 *) RwMalloc(sizeof(RwUInt8) * 3 * RwImageGetWidth(image), rwMEMHINTDUR_FUNCTION);

    /* The picture is stored bottom line first, top line last */
    for (y = RwImageGetHeight(image) - 1; y >= 0; y--)
    {
        nbyte += BMPWriteRow(stream, image, y, bpp, packed);
    }

    /* Free the array */
    RwFree(packed);

    RWRETURN(nbyte);
}

/*
 * Write a BMP file of the given category.
 *
 * Note that we must have 'colors' in order to know exactly how many
 * colors are in the R, G, B, arrays.  Entries beyond those in the
 * arrays are undefined.
 */
static void
BMPEncode(RwStream * stream, RwImage * image)
{
    RwUInt32 bpp;
    RwUInt32 nbyte = 0;

    RWFUNCTION(RWSTRING("BMPEncode"));

    /* what depth to write? */
    bpp = 24;
    if (RwImageGetPalette(image))
    {
        bpp = RwImageGetDepth(image);
    }

    nbyte += BMPWriteFileHeader(stream, bpp, image);
    nbyte += BMPWriteInfoHeader(stream, bpp, image);
    nbyte += BMPWriteBits(stream, bpp, image);

    if (nbyte != BMPLenFile(bpp, image))
    {
        RWMESSAGE((RWSTRING("error - BMPEncode")));
    }

    RWRETURNVOID();
}

/****************************************************************************
 BMPImageSetSpan

 Expands a span into the appropriate format

 On entry   : Image
            : Y coord of span
            : Data
            : Depth
            : TRUE if RGB (or GBR)
 On exit    : TRUE on success
 */
static              RwBool
BMPImageSetSpan(RwImage * image, RwInt32 y, RwUInt8 * cpSpan,
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

    RWFUNCTION(RWSTRING("BMPImageSetSpan"));
    RWASSERT(image);
    RWASSERT(cpSpan);

    cpDestin = (RwUInt8 *) (image->cpPixels + (image->stride * y));
    rpDestin = (RwRGBA *) cpDestin;

    switchKey = GENSWITCHKEY(depth, RwImageGetDepth(image));
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
 * \ingroup rtbmp
 * \ref RtBMPImageRead is used to read a BMP file to create an RwImage.
 *
 * The file name can either be absolute or relative to the directory the
 * application is running from.  The image search path is not used to find
 * image files and no gamma correction is applied to the image as it is read.
 *
 * \param imageName Pointer to the filename of the BMP file to read.
 *
 * \return Returns an image on success, or NULL on failure.
 *
 * \see RtBMPImageWrite
 * \see RwImageRead
 * \see RwImageWrite
 * \see RwImageRegisterImageFormat
 */
RwImage            *
RtBMPImageRead(const RwChar * imageName)
{
    RwStream           *stream;
	RwImage	           *image = NULL;

    RWAPIFUNCTION(RWSTRING("RtBMPImageRead"));
    RWASSERT(imageName);

    stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, imageName);
    if (stream)
    {
		image = RtBMPImageReadStream(stream);
        RwStreamClose(stream, NULL);

		return image;
    }

    /* Can't find the file */
    RWRETURN((RwImage *)NULL);
}

RwImage	*RtBMPImageReadStream(RwStream * stream)
{
    RwUInt32            marker;
    RwImage            *image;
    RwBmpHeader         bhHeader;
    RwUInt8             caClutBuffer[256 * 4];
    RwInt32             position;
    RwInt32             stride;
    RwUInt8            *scanLineBuf;
    RwInt32             nX, y;
    RwUInt32            size;
    RwUInt32            targetDepth;

    RWAPIFUNCTION(RWSTRING("RtBMPImageReadStream"));

    /* Find out what kind of image we have */
    if (RwStreamRead(stream, &marker, sizeof(marker)) !=
        sizeof(marker))
    {
        RWRETURN((RwImage *)NULL);
    }

    (void)RwMemNative32(&marker, sizeof(marker));

    /* Is this a bmp file ? */
    if ((marker & rwBMPMAGICMASK) != rwBMPMAGIC)
    {
        /* Not the right type */
        RWRETURN((RwImage *)NULL);
    }

    /* Read the header - halfword address so we can read longs from long boundaries */
    if (RwStreamRead(stream, &caClutBuffer[2], 14) != 14)
    {
        RWRETURN((RwImage *)NULL);
    }
    position = 18;

    bhHeader.nColsUsed = 0;

    bhHeader.nOffset = RWRWINT32GETLITTLE(caClutBuffer, 8);
    bhHeader.nHeadSize = RWRWINT32GETLITTLE(caClutBuffer, 12);

    size = bhHeader.nHeadSize - 4;
    if (RwStreamRead(stream, &caClutBuffer[0], size) != size)
    {
        RWRETURN((RwImage *)NULL);
    }
    position += size;

    if (bhHeader.nHeadSize == 12)
    {
        bhHeader.width = RWRWINT16GETLITTLE(caClutBuffer, 0);
        bhHeader.height = RWRWINT16GETLITTLE(caClutBuffer, 2);
        bhHeader.nPlanes = RWRWINT16GETLITTLE(caClutBuffer, 4); /* assert planes == 1 */
        bhHeader.nBitsPixel = RWRWINT16GETLITTLE(caClutBuffer, 6);
        bhHeader.nColsUsed = 0;
        bhHeader.nCompress = 0;
    }
    else
    {
        bhHeader.width = RWRWINT32GETLITTLE(caClutBuffer, 0);
        bhHeader.height = RWRWINT32GETLITTLE(caClutBuffer, 4);
        bhHeader.nPlanes = (RwInt16) RWRWINT16GETLITTLE(caClutBuffer, 8);
        bhHeader.nBitsPixel = (RwInt16) RWRWINT16GETLITTLE(caClutBuffer, 10);
        bhHeader.nCompress = RWRWINT32GETLITTLE(caClutBuffer, 12);
        bhHeader.nColsUsed = RWRWINT32GETLITTLE(caClutBuffer, 28);

        /* we cannot handle 4 bit RLE */
        if (bhHeader.nCompress == 2)
        {
            RWERROR((E_RW_INVIMAGEFORMAT));
            RWRETURN((RwImage *)NULL);
        }
    }

    /* Check sanity of of ColsUsed field */
    if ((bhHeader.nColsUsed <= 0) ||
        (bhHeader.nColsUsed > (1 << bhHeader.nBitsPixel)))
    {
        bhHeader.nColsUsed = (1 << bhHeader.nBitsPixel);
        bhHeader.nImportantCols = bhHeader.nColsUsed;
    }

    /* Create the image */
    switch (bhHeader.nBitsPixel)
    {
        case (1):
        case (4):
            {
                targetDepth = 4;
                break;
            }
        case (8):
            {
                targetDepth = 8;
                break;
            }
        case (15):
        case (16):
        case (24):
        case (32):
        default:
            {
                targetDepth = 32;
                break;
            }
    }

    /* Allocate the image */
    image = RwImageCreate(bhHeader.width, bhHeader.height, targetDepth);
    if (!image)
    {
        RWRETURN((RwImage *)NULL);
    }

    if (!RwImageAllocatePixels(image))
    {
        RwImageDestroy(image);
        RWRETURN((RwImage *)NULL);
    }

    /* Deal with a palette? */
    if (bhHeader.nBitsPixel <= 8)
    {
        RwRGBA *rpPal = RwImageGetPalette(image);

        if (bhHeader.nHeadSize == 12)
        {
            /* OS/2.1 format */
            size = bhHeader.nColsUsed * 3;
            if (RwStreamRead(stream, caClutBuffer, size) != size)
            {
                RWRETURN((RwImage *)NULL);
            }
            position += size;

            for (nX = 0; nX < bhHeader.nColsUsed; nX++)
            {
                rpPal[nX].red = caClutBuffer[nX * 3 + 2];
                rpPal[nX].green = caClutBuffer[nX * 3 + 1];
                rpPal[nX].blue = caClutBuffer[nX * 3 + 0];
                rpPal[nX].alpha = 0xff;
            }
        }
        else if (bhHeader.nHeadSize == 40)
        {
            /* Win3.x format */
            size = bhHeader.nColsUsed * 4;
            if (RwStreamRead(stream, caClutBuffer, size) != size)
            {
                RWRETURN((RwImage *)NULL);
            }
            position += size;

            for (nX = 0; nX < bhHeader.nColsUsed; nX++)
            {
                rpPal[nX].red = caClutBuffer[nX * 4 + 2];
                rpPal[nX].green = caClutBuffer[nX * 4 + 1];
                rpPal[nX].blue = caClutBuffer[nX * 4 + 0];
                rpPal[nX].alpha = 0xff;
            }
        }
        else
        {
            /* Beats me then */
            RWERROR((E_RW_INVIMAGEFORMAT));
            RWRETURN((RwImage *)NULL);
        }
    }

    /* Skip to the raster data itself */
    if (!RwStreamSkip(stream, bhHeader.nOffset - position))
    {
        RwImageDestroy(image);
        RWRETURN((RwImage *)NULL);
    }

    stride = (bhHeader.width * bhHeader.nBitsPixel + 7) / 8;
    stride = (stride + 3) & -4;

    /* Allocate a scanline */
    scanLineBuf =
        (RwUInt8 *) RwMalloc(((bhHeader.width + 7) & -8) *
                                ((bhHeader.nBitsPixel + 7) / 8),
                                rwMEMHINTDUR_FUNCTION);
    if (!scanLineBuf)
    {
        RWERROR((E_RW_NOMEM, (((bhHeader.width + 7) & -8) *
                                ((bhHeader.nBitsPixel + 7) / 8))));
        RwImageDestroy(image);
        RWRETURN((RwImage *)NULL);
    }

    /* Read the image */
    /* Check if 24 or 32 bit non compressed */
    if ((bhHeader.nCompress == 0) || (bhHeader.nBitsPixel == 24) ||
        (bhHeader.nBitsPixel == 32))
    {
        for (y = 0; y < bhHeader.height; y++)
        {
            if (RwStreamRead(stream, scanLineBuf, stride) !=
                (RwUInt32) stride)
            {
                RwFree(scanLineBuf);
                RwImageDestroy(image);
                RWRETURN((RwImage *)NULL);
            }

            BMPImageSetSpan(image, (bhHeader.height - 1) - y,
                            scanLineBuf, bhHeader.nBitsPixel,
                            FALSE);
        }
    }
    else
    if (bhHeader.nCompress == 1)
    {
        for (y = 0; y < bhHeader.height; y++)
        {
            RwBool              bEofLine;

            /* 8-bit RLE */
            nX = 0;
            bEofLine = FALSE;
            while (!bEofLine)
            {
                RwUInt8             caValues[2];

                size = sizeof(caValues);
                if (RwStreamRead(stream, caValues, size) != size)
                {
                    RwFree(scanLineBuf);
                    RwImageDestroy(image);
                    RWRETURN((RwImage *)NULL);
                }

                if (caValues[0] != 0)
                {
                    /* repeat group - duplicate the byte */
                    memset(&scanLineBuf[nX], caValues[1], caValues[0]);
                    nX += caValues[0];
                }
                else
                if (caValues[1] > 2)
                {
                    /* literal group */
                    size = caValues[1];
                    if (RwStreamRead(stream, &scanLineBuf[nX], size) != size)
                    {
                        RwFree(scanLineBuf);
                        RwImageDestroy(image);
                        RWRETURN((RwImage *)NULL);
                    }
                    nX += caValues[1];

                    if (caValues[1] & 1) /* padding byte */
                    {
                        size = sizeof(RwUInt8);
                        if (RwStreamRead(stream, caValues, size) != size)
                        {
                            RwFree(scanLineBuf);
                            RwImageDestroy(image);
                            RWRETURN((RwImage *)NULL);
                        }
                    }
                }
                else
                if (caValues[1] == 0)
                {
                    /* special group - end of line */
                    bEofLine = TRUE;
                }
                else
                if (caValues[1] == 1)
                {
                    /* special group - end of image */
                    bEofLine = TRUE;
                    y = bhHeader.height - 1;
                }
                else
                {
                    /* we cannot handle deltas (type 2) */
                    /* or any other type */
                    RwFree(scanLineBuf);
                    RwImageDestroy(image);
                    RWRETURN((RwImage *)NULL);
                }
            }

            /* Save the data */
            BMPImageSetSpan(image, (bhHeader.height - 1) - y,
                            scanLineBuf, bhHeader.nBitsPixel, TRUE);

            /* Onto the next line */
        }
    }
    else
    {
        /* unknown compression */
        RWERROR((E_RW_INVIMAGEFORMAT));
        RwFree(scanLineBuf);
        RwImageDestroy(image);
        RWRETURN((RwImage *)NULL);
    }

    /* No longer need the scan line */
    RwFree(scanLineBuf);

    /* Loaded the image */
    RWRETURN(image);
}
/**
 * \ingroup rtbmp
 * \ref RtBMPImageWrite is used to write an image as a BMP file.
 *
 * The image file name can either be absolute or relative to the directory the
 * application is running from.
 *
 * Note that if the image has been gamma corrected using
 * \ref RwImageGammaCorrect, the gamma correction is not removed from
 * the image before writing it to disk.
 *
 * \param image Pointer to the image to write
 * \param imageName Pointer to the filename of the BMP file to write.
 *
 * \return Returns an image on success, or NULL on failure.
 *
 * \see RtBMPImageRead
 * \see RwImageRead
 * \see RwImageWrite
 * \see RwImageRegisterImageFormat
 */
RwImage            *
RtBMPImageWrite(RwImage * image, const RwChar * imageName)
{
    RwStream           *stream;

    RWAPIFUNCTION(RWSTRING("RtBMPImageWrite"));
    RWASSERT(image);
    RWASSERT(imageName);

    stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMWRITE, imageName);
    if (stream)
    {
        /* always write WIN format (not OS/2) */
        BMPEncode(stream, image);

        RwStreamClose(stream, NULL);
        RWRETURN(image);
    }

    /* Failed to open the stream */
    RWRETURN((RwImage *)NULL);
}
