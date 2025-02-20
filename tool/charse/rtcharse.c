/*
 *
 * Simple monochrome mono spaced font.
 *
 * Copyright (c) 1998 Criterion Software Ltd.
 */

/**
 * \ingroup rtcharset
 * \page rtcharsetoverview RtCharset Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rtcharse.h
 * \li \b Libraries: rwcore, rtcharse
 * \li \b Plugin \b attachments: None
 *
 * \subsection charsetoverview Overview
 * RtCharset provides a simple, low-overhead text display Toolkit.
 *
 * RtCharset provides very fast, highly-optimized support for displaying
 * text messages and is often used for displaying in-vision debugging and
 * diagnostic messages during development.
 */

/****************************************************************************
Includes
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "rpplugin.h"

#include <rpdbgerr.h>
#include <rwcore.h>
#include <rtcharse.h>

#include "chrprint.h"

/****************************************************************************
 Local Defines
 */

/****************************************************************************
 Local Types
 */

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local (static) Globals
 */

#if (defined(_IBM_CHAR))

const RtGlyphRow   *
_rtCharsetIBMFontGet(RtCharsetDesc * desc, RwInt32 * rasSize)
{
    static const RtIBMGlyph charSetBits[] =
    {
#include "ibmchars.h"
    };
    RwInt32             count;
    RwInt32             tilewidth;
    RwInt32             tileheight;
    RwInt32             size = 16;

    RWFUNCTION(RWSTRING("_rtCharsetIBMFontGet"));

    desc->width  = rtIBMCHARSETWIDTH;
    desc->height = rtIBMCHARSETHEIGHT;

    desc->width_internal =  desc->width  + CHARSET_TEXEL_BORDER_SIZE;
    desc->height_internal = desc->height + CHARSET_TEXEL_BORDER_SIZE;

    count = (sizeof(charSetBits) / sizeof(RtIBMGlyph));

    do
    {
        size += size;
        tilewidth = size / desc->width_internal;
        tileheight = (count + tilewidth - 1) / tilewidth;
    }
    while (size < (tileheight * desc->height_internal));

    desc->count = count;
    desc->tilewidth = tilewidth;
    desc->tileheight = tileheight;

    *rasSize = size;

    RWRETURN(&charSetBits[0][0]);
}

#elif (defined(_CEE_FAX_CHAR))

const RtGlyphRow   *
_rtCharsetCeeFaxFontGet(RtCharsetDesc * desc, RwInt32 * rasSize)
{
    static const RtCeeFaxGlyph charSetBits[] =
    {
#include "ceefax.h"
    };
    RwInt32             count;
    RwInt32             tilewidth;
    RwInt32             tileheight;
    RwInt32             size = 16;

    RWFUNCTION(RWSTRING("_rtCharsetCeeFaxFontGet"));

    desc->width  = rtCEEFAXCHARSETWIDTH;
    desc->height = rtCEEFAXCHARSETHEIGHT;

    desc->width_internal =  desc->width  + CHARSET_TEXEL_BORDER_SIZE;
    desc->height_internal = desc->height + CHARSET_TEXEL_BORDER_SIZE;



    count = (sizeof(charSetBits) / sizeof(RtCeeFaxGlyph));

    do
    {
        size += size;
        tilewidth = size / desc->width_internal;
        tileheight = (count + tilewidth - 1) / tilewidth;
    }
    while (size < (tileheight * desc->height_internal));

    desc->count = count;
    desc->tilewidth = tilewidth;
    desc->tileheight = tileheight;

    *rasSize = size;

    RWRETURN(&charSetBits[0][0]);
}

#else /* (defined(_IBM_CHAR)) */

const RtGlyphRow   *
_rtCharsetAtariFontGet(RtCharsetDesc * desc, RwInt32 * rasSize)
{
    static const RtAtariGlyph charSetBits[] =
    {
#include "atariset.h"
    };
    RwInt32             count;
    RwInt32             tilewidth;
    RwInt32             tileheight;
    RwInt32             size = 16;

    RWFUNCTION(RWSTRING("_rtCharsetAtariFontGet"));

    desc->width  = rtATARICHARSETWIDTH;
    desc->height = rtATARICHARSETHEIGHT;

    desc->width_internal =  desc->width  + CHARSET_TEXEL_BORDER_SIZE;
    desc->height_internal = desc->height + CHARSET_TEXEL_BORDER_SIZE;

    count = (sizeof(charSetBits) / sizeof(RtAtariGlyph));

    do
    {
        size += size;
        tilewidth = size / desc->width_internal;
        tileheight = (count + tilewidth - 1) / tilewidth;
    }
    while (size < (tileheight * desc->height_internal));

    desc->count = count;
    desc->tilewidth = tilewidth;
    desc->tileheight = tileheight;

    *rasSize = size;

    RWRETURN(&charSetBits[0][0]);
}

#endif /* (defined(_IBM_CHAR)) */

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  Opening and closing

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/*
 *  Set the colors of the character set created.
 *
 *  charBitsIn Character set to change the colors of
 *  desc Foreground color
 *  charRow Background color
 *  stride
 *
 *  Returns pointer to the character set on success, or NULL on failure.
 */
static const RtGlyphRow *
CharsetGlyphSet(const RtGlyphRow * charBitsIn,
                RtCharsetDesc * desc, RwUInt8 * charRow, RwInt32 stride)
{
    RtGlyphRow          charPixels;
    RwUInt8            *target;
    RwInt32             i;
    RwInt32             height_internal;
    RwInt32             width_internal;

    RWFUNCTION(RWSTRING("CharsetGlyphSet"));

    height_internal = desc->height_internal;
    width_internal = desc->width_internal;

    for (i = 0; i < height_internal; i++)
    {
        target = charRow;
        charRow += stride;

        memset(target, 0, width_internal);

        for (charPixels = (*charBitsIn); charPixels; charPixels >>= 1)
        {
            *target++ = (RwUInt8) (charPixels & 0x01);
        }
        charBitsIn++;
    }

    RWRETURN(charBitsIn);
}

static RwImage     *
CharsetImageSet(RwImage * image)
{
    RwUInt8            *const charBase = RwImageGetPixels(image);
    const RwUInt32      stride = RwImageGetStride(image);
    RtCharsetDesc       desc;
    RwInt32             i, j = 0;
    const RtGlyphRow   *charBitsIn = GETCHARSETBITS(&desc, &i);
    RwInt32             row = 0;
    RwInt32             column = 0;

    RWFUNCTION(RWSTRING("CharsetImageSet"));

    /* Set all of the pixels */

    while (j++ < desc.count)
    {
        RwUInt8            *charRow = &charBase[column * desc.width_internal +
            stride * row *
            desc.height_internal];
        charBitsIn =
            CharsetGlyphSet(charBitsIn, &desc, charRow, stride);

        if (desc.tilewidth == ++column)
        {
            column = 0;
            ++row;
        }

    }

    RWRETURN(image);
}

/* Public functions
 * **************** */

/**
 * \ingroup rtcharset
 * \ref RtCharsetGetDesc is used to retrieve \ref RtCharsetDesc information
 * about the specified raster character set. This includes the pixel-size of
 * each character and the number of characters in the set.
 *
 * The include file rtcharse.h and the library file rtcharse.lib are
 * required to use this function.
 *
 * \param charset  Pointer to the raster character set.
 * \param desc  Pointer to an RtCharsetDesc that will receive the information.
 *
 * \return Returns a pointer to the raster charster set if successful
 * or NULL if there is an error.
 *
 * \see RtCharsetOpen
 * \see RtCharsetClose
 * \see RtCharsetPrintBuffered
 * \see RtCharsetBufferFlush
 * \see RtCharsetCreate
 * \see RtCharsetPrint
 *
 */
RtCharset          *
RtCharsetGetDesc(RtCharset * charset, RtCharsetDesc * desc)
{
    RWAPIFUNCTION(RWSTRING("RtCharsetGetDesc"));
    RWASSERT(charset);
    RWASSERT(desc);

    if (charset && desc)
    {
        RwInt32             i;

        GETCHARSETBITS(desc, &i);

        RWRETURN(charset);
    }
    else
    {
        RWERROR((E_RW_NULLP));

        RWRETURN((RwRaster *) NULL);
    }
}

/**
 * \ingroup rtcharset
 * \ref RtCharsetSetColors is used to redefine the foreground and
 * background colors of the specified raster character set using the
 * given colors. The foreground color specifies the color of the text
 * lettering while the background specifies the color outside of the text
 * lettering.
 *
 * This function changes the color of the character set rather than the text.
 * This implies that all text rendered with the character set will be changed
 * to use the new set of colors. This includes any text rendered before the
 * function was called.
 *
 * The include file rtcharse.h and the library file rtcharse.lib are
 * required to use this function.
 *
 * \param charSet  Pointer to the raster character set.
 * \param foreGround  Pointer to a RwRGBA value equal to the foreground color.
 * \param backGround  Pointer to a RwRGBA value equal to the background color.
 *
 * \return Returns a pointer to the raster character set if successful
 * or NULL if there is an error.
 *
 * \see RtCharsetOpen
 * \see RtCharsetClose
 * \see RtCharsetPrintBuffered
 * \see RtCharsetBufferFlush
 * \see RtCharsetCreate
 * \see RtCharsetDestroy
 * \see RtCharsetPrint
 * \see RtCharsetGetDesc
 *
 */
RtCharset          *
RtCharsetSetColors(RtCharset * charSet,
                   const RwRGBA * foreGround, const RwRGBA * backGround)
{
    RtCharset          *result = (RwRaster *) NULL;

    RWAPIFUNCTION(RWSTRING("RtCharsetSetColors"));
    RWASSERT(charSet);
    RWASSERT(foreGround);
    RWASSERT(backGround);

    if (charSet && foreGround && backGround)
    {
        RwImage            *image;
        RtCharsetDesc       desc;
        RwInt32             rasSize;

        (void) GETCHARSETBITS(&desc, &rasSize);

        /* Removed call to RtCharsetBufferFlush, both because it
           could be called inside a camera begin/update block which
           is invalid, and because the documentation states that colors
           of data printed before the call will change, hence no need
           to flush the buffer */

        image = RwImageCreate(rasSize, rasSize, 8);

        if (image)
        {
            RWASSERT(RwRasterGetWidth(charSet) ==
                RwImageGetWidth(image));
            RWASSERT(RwRasterGetHeight(charSet) ==
                RwImageGetHeight(image));

            if (RwImageAllocatePixels(image))
            {
                RwRGBA             *palette = RwImageGetPalette(image);
                RwImage            *new_image;

                RwRGBAAssign(&palette[0], backGround);

                RwRGBAAssign(&palette[1], foreGround);

                /* Set the image */

                new_image = CharsetImageSet(image);

                RwRasterSetFromImage(charSet, new_image);

                /* Done */
                result = charSet;

            }
            RwImageDestroy(image);

        }
        else
        {
            /* NULL == image */
        }
    }
    else
    {
        RWERROR((E_RW_NULLP));
    }

    RWRETURN(result);
}

/**
 * \ingroup rtcharset
 * \ref RtCharsetDestroy is used to destroy the specified raster
 * character set.
 *
 * The include file rtcharse.h and the library file rtcharse.lib are
 * required to use this function.
 *
 * \param charSet  Pointer to the raster character set.
 *
 * \return Returns TRUE if successful or FALSE if there is an error
 *
 * \see RtCharsetOpen
 * \see RtCharsetClose
 * \see RtCharsetPrintBuffered
 * \see RtCharsetBufferFlush
 * \see RtCharsetCreate
 * \see RtCharsetSetColors
 * \see RtCharsetPrint
 *
 */
RwBool
RtCharsetDestroy(RtCharset * charSet)
{
    RWAPIFUNCTION(RWSTRING("RtCharsetDestroy"));
    RWASSERT(charSet);

    if (charSet)
    {
        RWASSERTM((_rtgBuffer.initialised == FALSE) ||
                  (_rtgBuffer.charSet != charSet) ||
                  (_rtgBuffer.numChars == 0),
                  (RWSTRING("The character set being destroyed still has ")
                  RWSTRING("characters buffered for printing. RtCharsetBufferFlush ")
                  RWSTRING("should be called prior to destroying a character set if ")
                  RWSTRING("there may be any buffered characters left.")));

        RwRasterDestroy((RwRaster *) charSet);

        RWRETURN(TRUE);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN(FALSE);
}

/**
 * \ingroup rtcharset
 * \ref RtCharsetCreate is used to create a character set for
 * displaying text on RenderWare rasters with the specified foreground
 * and background colors. The foreground color specifies the color of
 * the text lettering while the background specifies the color outside
 * of the text lettering.
 *
 * The include file rtcharse.h and the library file rtcharse.lib are
 * required to use this function.
 *
 * \param foreGround  Pointer to a RwRGBA value equal to the foreground color.
 * \param backGround  Pointer to a RwRGBA value equal to the background color.
 *
 * \return Returns a pointer to the new raster character set if
 * successful or NULL if there is an error.
 *
 * \see RtCharsetOpen
 * \see RtCharsetClose
 * \see RtCharsetPrintBuffered
 * \see RtCharsetBufferFlush
 * \see RtCharsetDestroy
 * \see RtCharsetSetColors
 * \see RtCharsetPrint
 * \see RtCharsetGetDesc
 *
 */
RtCharset          *
RtCharsetCreate(const RwRGBA * foreGround, const RwRGBA * backGround)
{
    RtCharset          *result = (RwRaster *) NULL;

    RWAPIFUNCTION(RWSTRING("RtCharsetCreate"));
    RWASSERT(foreGround);
    RWASSERT(backGround);

    if (foreGround && backGround)
    {
        RtCharsetDesc       desc;
        RwInt32             rasSize;
        RtCharset          *charSet;

        (void)GETCHARSETBITS(&desc, &rasSize);

        charSet = RwRasterCreate(rasSize, rasSize,
                                 0, rwRASTERTYPETEXTURE);

        if (charSet)
        {

            if (RtCharsetSetColors(charSet, foreGround, backGround))
            {
                /* Done */
                result = charSet;

            }
            else
            {
                RtCharsetDestroy(charSet);

            }

        }

    }
    else
    {

        RWERROR((E_RW_NULLP));
    }

    RWRETURN(result);
}

/**
 * \ingroup rtcharset
 * \ref RtCharsetClose is used to close the charset toolkit.
 *
 * This function should be called after any other charset functions;
 * strictly speaking it is only required if \ref RtCharsetOpen has
 * been called.
 *
 * The include file rtcharse.h and the library file rtcharse.lib are
 * required to use this function.
 *
 * \see RtCharsetOpen
 * \see RtCharsetPrintBuffered
 * \see RtCharsetBufferFlush
 * \see RtCharsetCreate
 * \see RtCharsetDestroy
 * \see RtCharsetPrint
 * \see RtCharsetGetDesc
 *
 */
void
RtCharsetClose(void)
{
    RWAPIFUNCTION(RWSTRING("RtCharsetClose"));

    RWASSERT(_rtgBuffer.initialised != FALSE);

    /* Destroy the triangle buffer */
    RwFree(_rtgBuffer.vertices);
    _rtgBuffer.vertices = (RwIm2DVertex *) NULL;
    RwFree(_rtgBuffer.indices);
    _rtgBuffer.indices = (RwImVertexIndex *) NULL;
    _rtgBuffer.initialised = FALSE;

    RWRETURNVOID();
}

/**
 * \ingroup rtcharset
 * \ref RtCharsetOpen is used to open the charset toolkit.
 *
 * This function should be called before any other charset functions
 * (strictly speaking it is only required for buffered string printing).
 *
 * The include file rtcharse.h and the library file rtcharse.lib are
 * required to use this function.
 *
 * \return Returns TRUE if successful or FALSE if there is an error
 *
 * \see RtCharsetClose
 * \see RtCharsetPrintBuffered
 * \see RtCharsetBufferFlush
 * \see RtCharsetCreate
 * \see RtCharsetDestroy
 * \see RtCharsetPrint
 * \see RtCharsetGetDesc
 */
RwBool
RtCharsetOpen(void)
{
    RWAPIFUNCTION(RWSTRING("RtCharsetOpen"));

    RWASSERT(_rtgBuffer.initialised == FALSE);

    /* Create the triangle buffer */
    _rtgBuffer.vertices = (RwIm2DVertex *)
        RwMalloc(BUFFERNUMCHARS * 4 * sizeof(RwIm2DVertex),
                 rwID_CHARSEPLUGIN | rwMEMHINTDUR_GLOBAL);
    if (NULL == _rtgBuffer.vertices)
        RWRETURN(FALSE);
    _rtgBuffer.indices = (RwImVertexIndex *)
        RwMalloc(BUFFERNUMCHARS * 6 * sizeof(RwImVertexIndex),
                 rwID_CHARSEPLUGIN | rwMEMHINTDUR_GLOBAL);
    if (NULL == _rtgBuffer.indices)
    {
        RwFree(_rtgBuffer.vertices);
        _rtgBuffer.vertices = (RwIm2DVertex *) NULL;
        RWRETURN(FALSE);
    }

    _rtgBuffer.numChars = 0;

    _rtgBuffer.charSet = (RwRaster *) NULL;
    _rtgBuffer.initialised = TRUE;

    RWRETURN(TRUE);
}
