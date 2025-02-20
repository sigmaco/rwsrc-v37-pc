/*
 *
 * Simple monochrome mono spaced font.
 *
 * Copyright (c) 1998 Criterion Software Ltd.
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

RtCharsetBuffer     _rtgBuffer = { FALSE,
    (RwRaster *) NULL,
    0,
    (RwIm2DVertex *) NULL,
    (RwImVertexIndex *) NULL
};

static              RwInt32
CharsetProcessString(const RwChar * string, RwUInt32 length,
                     RtCharset * charSet, RwInt32 x, RwInt32 y,
                     RwIm2DVertex * vertices, RwImVertexIndex * indices,
                     RwImVertexIndex indexOffset, RwBool hideSpaces)
{
    static RwChar       space[] = RWSTRING(" ");
    RwInt32             chHeight, chWidth;
    RwCamera           *camera;
    RwRaster           *dest;
    RwInt32             rasHeight, rasWidth;
    RwReal              yScale, xScale;
    RwRect              rectQuad, rectUV;
    RtCharsetDesc       desc;
    RwUInt32            i, j, index, numDrawnChars;
    RwReal              nearScreenZ, recipCamZ;

#if !defined(OPENGL_DRVMODEL_H)
    RwReal              fOffsetU;
    RwReal              fOffsetV;
#endif /* !defined(OPENGL_DRVMODEL_H) */

    RWFUNCTION(RWSTRING("CharsetProcessString"));
    RWASSERT(NULL != string);
    RWASSERT(length > 0);
    RWASSERT(NULL != charSet);
    RWASSERT(NULL != vertices);
    RWASSERT(NULL != indices);

    GETCHARSETBITS(&desc, &rasHeight); /* rasHeight used as a temporary */

    chHeight = RwRasterGetHeight((RwRaster *) charSet);
    chWidth = RwRasterGetWidth((RwRaster *) charSet);
    yScale = 1 / ((RwReal) chHeight);
    xScale = 1 / ((RwReal) chWidth);

#if !defined(OPENGL_DRVMODEL_H)
    fOffsetU = ( 1.0f / ((RwReal) chWidth ) ) / 2.0f;
    fOffsetV = ( 1.0f / ((RwReal) chHeight) ) / 2.0f;
#endif /* !defined(OPENGL_DRVMODEL_H) */

    camera = RwCameraGetCurrentCamera();
    RWASSERT(NULL != camera);
    dest = RwCameraGetRaster(camera);
    RWASSERT(NULL != dest);

    rasHeight = RwRasterGetHeight(dest);
    rasWidth = RwRasterGetWidth(dest);

    nearScreenZ = RwIm2DGetNearScreenZ();
    recipCamZ = (RwReal) 1 / RwCameraGetNearClipPlane(camera);

    if (hideSpaces == FALSE)
    {
        /* Terminal NULL zero never gets drawn anyway */
        space[0] = 0;
    }

    numDrawnChars = 0;

    for (i = 0; i < length; i++)
    {
        if (string[i] != space[0])
        {
            rectQuad.x = x + ( desc.width ) * i;
            rectQuad.y = y;
            /* Note that I'm using w and h as xMax and yMax, not width_internal and height_internal */
            rectQuad.w = rectQuad.x + desc.width;
            rectQuad.h = y + desc.height;

            /* Clip the character */
            if ((rectQuad.y < rasHeight) && (rectQuad.h > 0) &&
                (rectQuad.x < rasWidth) && (rectQuad.w > 0))
            {
                index = (string[i] - BASE);

                rectUV.y = index / desc.tilewidth;
                rectUV.x = index - rectUV.y * desc.tilewidth;
                rectUV.x *= desc.width_internal;
                rectUV.y *= desc.height_internal;

                /* Note that I'm using w and h as xMax and yMax, not width_internal and height_internal */
                rectUV.w = rectUV.x + desc.width;
                rectUV.h = rectUV.y + desc.height;

                if (rectQuad.y < 0)
                {
                    rectUV.y = rectUV.y - rectQuad.y;
                    rectQuad.y = 0;
                }
                if (rectQuad.h > (rasHeight - 1))
                {
                    rectUV.h =
                        rectUV.y + ((rasHeight - 1) - rectQuad.y);
                    rectQuad.h = rasHeight - 1;
                }
                if (rectQuad.x < 0)
                {
                    rectUV.x = rectUV.x - rectQuad.x;
                    rectQuad.x = 0;
                }
                if (rectQuad.w > (rasWidth - 1))
                {
                    rectUV.w = rectUV.x + ((rasWidth - 1) - rectQuad.x);
                    rectQuad.w = rasWidth - 1;
                }

                /* OpenGL does not require the offset */
#if defined(OPENGL_DRVMODEL_H)
                RwIm2DVertexSetU(&(vertices[numDrawnChars * 4 + 0]),
                                 xScale * (rectUV.x), 1);
                RwIm2DVertexSetV(&(vertices[numDrawnChars * 4 + 0]),
                                 yScale * (rectUV.y), 1);
                RwIm2DVertexSetU(&(vertices[numDrawnChars * 4 + 1]),
                                 xScale * (rectUV.x), 1);
                RwIm2DVertexSetV(&(vertices[numDrawnChars * 4 + 1]),
                                 yScale * (rectUV.h), 1);
                RwIm2DVertexSetU(&(vertices[numDrawnChars * 4 + 2]),
                                 xScale * (rectUV.w), 1);
                RwIm2DVertexSetV(&(vertices[numDrawnChars * 4 + 2]),
                                 yScale * (rectUV.h), 1);
                RwIm2DVertexSetU(&(vertices[numDrawnChars * 4 + 3]),
                                 xScale * (rectUV.w), 1);
                RwIm2DVertexSetV(&(vertices[numDrawnChars * 4 + 3]),
                                 yScale * (rectUV.y), 1);
#else/* defined(OPENGL_DRVMODEL_H) */
                RwIm2DVertexSetU(&(vertices[numDrawnChars * 4 + 0]),
                                 xScale * (rectUV.x) + fOffsetU, 1);
                RwIm2DVertexSetV(&(vertices[numDrawnChars * 4 + 0]),
                                 yScale * (rectUV.y) + fOffsetV, 1);
                RwIm2DVertexSetU(&(vertices[numDrawnChars * 4 + 1]),
                                 xScale * (rectUV.x) + fOffsetU, 1);
                RwIm2DVertexSetV(&(vertices[numDrawnChars * 4 + 1]),
                                 yScale * (rectUV.h) + fOffsetV, 1);
                RwIm2DVertexSetU(&(vertices[numDrawnChars * 4 + 2]),
                                 xScale * (rectUV.w) + fOffsetU, 1);
                RwIm2DVertexSetV(&(vertices[numDrawnChars * 4 + 2]),
                                 yScale * (rectUV.h) + fOffsetV, 1);
                RwIm2DVertexSetU(&(vertices[numDrawnChars * 4 + 3]),
                                 xScale * (rectUV.w) + fOffsetU, 1);
                RwIm2DVertexSetV(&(vertices[numDrawnChars * 4 + 3]),
                                 yScale * (rectUV.y) + fOffsetV, 1);
#endif /* defined(OPENGL_DRVMODEL_H) */

                RwIm2DVertexSetScreenX(&
                                       (vertices
                                        [numDrawnChars * 4 + 0]),
                                       (RwReal) (rectQuad.x));
                RwIm2DVertexSetScreenY(&
                                       (vertices
                                        [numDrawnChars * 4 + 0]),
                                       (RwReal) (rectQuad.y));
                RwIm2DVertexSetScreenX(&
                                       (vertices
                                        [numDrawnChars * 4 + 1]),
                                       (RwReal) (rectQuad.x));
                RwIm2DVertexSetScreenY(&
                                       (vertices
                                        [numDrawnChars * 4 + 1]),
                                       (RwReal) (rectQuad.h));
                RwIm2DVertexSetScreenX(&
                                       (vertices
                                        [numDrawnChars * 4 + 2]),
                                       (RwReal) (rectQuad.w));
                RwIm2DVertexSetScreenY(&
                                       (vertices
                                        [numDrawnChars * 4 + 2]),
                                       (RwReal) (rectQuad.h));
                RwIm2DVertexSetScreenX(&
                                       (vertices
                                        [numDrawnChars * 4 + 3]),
                                       (RwReal) (rectQuad.w));
                RwIm2DVertexSetScreenY(&
                                       (vertices
                                        [numDrawnChars * 4 + 3]),
                                       (RwReal) (rectQuad.y));

                /*Set constant characteristics */
                for (j = 0; j < 4; j++)
                {
                    RwIm2DVertexSetRecipCameraZ(&
                                                (vertices
                                                 [numDrawnChars * 4 +
                                                  j]), recipCamZ);
                    RwIm2DVertexSetScreenZ(&
                                           (vertices
                                            [numDrawnChars * 4 + j]),
                                           nearScreenZ);
                    RwIm2DVertexSetIntRGBA(&
                                           (vertices
                                            [numDrawnChars * 4 + j]),
                                           255, 255, 255, 255);
                }

                indices[numDrawnChars * 6 + 0] = (RwImVertexIndex)
                    (indexOffset + numDrawnChars * 4 + 0);
                indices[numDrawnChars * 6 + 1] = (RwImVertexIndex)
                    (indexOffset + numDrawnChars * 4 + 1);
                indices[numDrawnChars * 6 + 2] = (RwImVertexIndex)
                    (indexOffset + numDrawnChars * 4 + 2);
                indices[numDrawnChars * 6 + 3] = (RwImVertexIndex)
                    (indexOffset + numDrawnChars * 4 + 0);
                indices[numDrawnChars * 6 + 4] = (RwImVertexIndex)
                    (indexOffset + numDrawnChars * 4 + 2);
                indices[numDrawnChars * 6 + 5] = (RwImVertexIndex)
                    (indexOffset + numDrawnChars * 4 + 3);

                numDrawnChars++;
            }
        }
    }

    RWRETURN(numDrawnChars);
}

#ifdef RTCHARSE_SUBRASTERS

static RtCharset   *
CharsetPrintSubRaster(RtCharset * charSet,
                      const RwChar * string, RwInt32 x, RwInt32 y)
{
    RtCharset          *result = NULL;
    RwRaster           *const target = RwRasterCreate(0, 0, 0, 0);

    RWFUNCTION(RWSTRING("CharsetPrintSubRaster"));

    if (target)
    {
        RtCharsetDesc       desc;
        RwInt32             i;
        const RtGlyphRow   *charBitsIn __RWUNUSED__ =
            GETCHARSETBITS(&desc, &i);

        /* We can cast from (RtCharset *) to (RwRaster *)
         * because they are the same thing */
        RwRaster           *const source = (RwRaster *) charSet;
        RwRect              rRect;
        RwInt32             ch;
        RwInt32             index;

        rRect.w = desc.width_internal;
        rRect.h = desc.height_internal;

        for (ch = *string++; ch; ch = *string++)
        {
            index = ch - BASE;
            rRect.y = index / desc.tilewidth;
            rRect.x = index - rRect.y * desc.tilewidth;

            rRect.x *= desc.width_internal;
            RWASSERT(rRect.x >= 0);
            RWASSERT((rRect.x + rRect.w) <= RwRasterGetWidth(source));

            rRect.y *= desc.height_internal;
            RWASSERT(rRect.y >= 0);
            RWASSERT((rRect.y + rRect.h) <= RwRasterGetHeight(source));

            RwRasterSubRaster(target, source, &rRect);
            RwRasterRender(target, x, y);

            x += desc.width_internal;
        }

        RwRasterDestroy(target);

        result = charSet;
    }

    RWRETURN(result);
}

#endif /* RTCHARSE_SUBRASTERS */

#if (!defined(RTCHARSE_SUBRASTERS))
static RtCharset   *
CharsetPrintNoSubRaster(RtCharset * charSet,
                        const RwChar * string, RwInt32 x, RwInt32 y)
{
    RtCharset          *result;
    RwIm2DVertex       *vertices;
    RwImVertexIndex    *indices;
    RwBool              zTestEnable, zWriteEnable;
    RwInt32             srcBlend, dstBlend;
    RwRaster           *txtRaster;
    RwInt32             txtFilter;
    RwCullMode          cullMode;

    RwUInt32            numChars = strlen(string);
    RwInt32             numDrawn;

    /* Try rendering with Im2D quads instead dude, way quickah */

    RWFUNCTION(RWSTRING("CharsetPrintNoSubRaster"));

    if (numChars <= 0)
        RWRETURN(charSet);

    vertices =
        (RwIm2DVertex *) RwMalloc(4 * numChars * sizeof(RwIm2DVertex),
                                  rwID_CHARSEPLUGIN | rwMEMHINTDUR_FUNCTION);
    indices =
        (RwImVertexIndex *) RwMalloc(6 * numChars *
                                     sizeof(RwImVertexIndex),
                                     rwID_CHARSEPLUGIN | rwMEMHINTDUR_FUNCTION);

    if (!vertices || !indices)
    {
        RwUInt32            i;

        if (vertices)
            RwFree(vertices);
        if (indices)
            RwFree(indices);
        i = 4 * numChars * sizeof(RwIm2DVertex) +
            6 * numChars * sizeof(RwImVertexIndex);
        RWERROR((E_RW_NOMEM, i));
        RWRETURN((RwRaster *) NULL);
    }

    result = charSet;

    numDrawn = CharsetProcessString(string,
                                    numChars,
                                    charSet,
                                    x,
                                    y,
                                    vertices,
                                    indices, (RwImVertexIndex) 0,
                                    FALSE);
    if (numDrawn <= 0)
    {
        RwFree(indices);
        RwFree(vertices);

        RWRETURN((RwRaster *) NULL);
    }

    RwRenderStateGet(rwRENDERSTATEZTESTENABLE, (void *) &zTestEnable);
    RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, (void *) &zWriteEnable);

    RwRenderStateGet(rwRENDERSTATESRCBLEND, (void *) &srcBlend);
    RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void *) &dstBlend);

    RwRenderStateGet(rwRENDERSTATETEXTURERASTER, (void *) &txtRaster);
    RwRenderStateGet(rwRENDERSTATETEXTUREFILTER, (void *) &txtFilter);

    RwRenderStateGet(rwRENDERSTATECULLMODE, (void *) &cullMode);

    /* Set appropriate renderstates - ignore the ZBuffer completely */
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void *) FALSE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void *) FALSE);

    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER,
                     (void *) rwFILTERNEAREST);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void *) charSet);
    /* Make sure rasters of any format are going to work transparency-wise */
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *) rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,
                     (void *) rwBLENDINVSRCALPHA);

    RwRenderStateSet(rwRENDERSTATECULLMODE,
                     (void *) rwCULLMODECULLNONE);

    RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST,
                                 vertices, 4 * numDrawn,
                                 indices, 6 * numDrawn);

    /* Reset to standard renderstates */
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void *) zTestEnable);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void *) zWriteEnable);

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *) srcBlend);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *) dstBlend);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void *) txtRaster);
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void *) txtFilter);

    RwRenderStateSet(rwRENDERSTATECULLMODE, (void *) cullMode);

    RwFree(indices);
    RwFree(vertices);

    RWRETURN(result);
}
#endif /* (!defined(RTCHARSE_SUBRASTERS)) */

/* Public functions
 * **************** */

/**
 * \ingroup rtcharset
 * \ref RtCharsetBufferFlush is used to display all previously
 * buffered strings
 *
 * \ref RtCharsetPrintBuffered is used to buffer up strings for
 * display.
 *
 * is called automatically when the buffer is filled or when the charset
 * in use is changed.
 *
 * \ref RtCharsetOpen must be called before this function is used. Also,
 * the include file rtcharse.h and the library file rtcharse.lib are
 * required to use this function.
 *
 * \return Returns TRUE if successful or FALSE if there is an error
 *
 * \see RtCharsetPrintBuffered
 * \see RtCharsetOpen
 * \see RtCharsetClose
 * \see RtCharsetCreate
 * \see RtCharsetDestroy
 * \see RtCharsetGetDesc
 * \see RtCharsetSetColors
 * \see RwCameraBeginUpdate
 * \see RwCameraEndUpdate
 *
 */
RwBool
RtCharsetBufferFlush(void)
{
    RWAPIFUNCTION(RWSTRING("RtCharsetBufferFlush"));
    RWASSERT(_rtgBuffer.initialised != FALSE);

    if (_rtgBuffer.numChars > 0)
    {
        RwBool              zTestEnable, zWriteEnable;
        RwInt32             srcBlend, dstBlend;
        RwRaster           *txtRaster;
        RwInt32             txtFilter;
        RwCullMode          cullMode;

        RWASSERT(NULL != _rtgBuffer.charSet);

        RwRenderStateGet(rwRENDERSTATEZTESTENABLE,
                         (void *) &zTestEnable);
        RwRenderStateGet(rwRENDERSTATEZWRITEENABLE,
                         (void *) &zWriteEnable);

        RwRenderStateGet(rwRENDERSTATESRCBLEND, (void *) &srcBlend);
        RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void *) &dstBlend);

        RwRenderStateGet(rwRENDERSTATETEXTURERASTER,
                         (void *) &txtRaster);
        RwRenderStateGet(rwRENDERSTATETEXTUREFILTER,
                         (void *) &txtFilter);

        RwRenderStateGet(rwRENDERSTATECULLMODE, (void *) &cullMode);

        /* Set appropriate renderstates - ignore the ZBuffer completely */

        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void *) FALSE);

        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void *) FALSE);

        RwRenderStateSet(rwRENDERSTATETEXTUREFILTER,
                         (void *) rwFILTERNEAREST);
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER,
                         (void *) _rtgBuffer.charSet);
        /* Make sure rasters of any format are going to work transparency-wise */

        RwRenderStateSet(rwRENDERSTATESRCBLEND,
                         (void *) rwBLENDSRCALPHA);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND,
                         (void *) rwBLENDINVSRCALPHA);

        RwRenderStateSet(rwRENDERSTATECULLMODE,
                         (void *) rwCULLMODECULLNONE);

        RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST,
                                     _rtgBuffer.vertices,
                                     4 *
                                     _rtgBuffer.numChars,
                                     _rtgBuffer.indices,
                                     6 * _rtgBuffer.numChars);

        /* Reset to standard renderstates */

        RwRenderStateSet(rwRENDERSTATEZTESTENABLE,
                         (void *) zTestEnable);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,
                         (void *) zWriteEnable);

        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *) srcBlend);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *) dstBlend);

        RwRenderStateSet(rwRENDERSTATETEXTURERASTER,
                         (void *) txtRaster);
        RwRenderStateSet(rwRENDERSTATETEXTUREFILTER,
                         (void *) txtFilter);

        RwRenderStateSet(rwRENDERSTATECULLMODE, (void *) cullMode);
    }

    _rtgBuffer.numChars = 0;

    _rtgBuffer.charSet = (RwRaster *) NULL;

    RWRETURN(TRUE);
}

/**
 * \ingroup rtcharset
 * \ref RtCharsetPrintBuffered is used to buffer a string to
 * be printed.
 *
 * This function is similar to \ref RtCharsetPrint and takes the same
 * set of parameters. The difference is that this function buffers a
 * string for display later. If several strings are buffered before
 * display then the speed of display will be quicker overall (the
 * difference between displaying one buffer for a screen and
 * displaying each word or even character at a time is huge).
 *
 * To display strings after buffering them, call
 * \ref RtCharsetBufferFlush.
 *
 * \ref RtCharsetOpen must be called before this function is used. Also,
 * the include file rtcharse.h and the library file rtcharse.lib are
 * required to use this function.
 *
 * \param charSet  Pointer to the raster character set.
 * \param string  Pointer to a string containing the characters to buffer.
 * \param x  A RwInt32 value equal to the x-coordinate.
 * \param y  A RwInt32 value equal to the y-coordinate.
 * \param hideSpaces  A RwBool specifying whether to hide spaces or not (TRUE = yes)
 *
 * \return Returns a pointer to the raster character set if successful
 * or NULL if there is an error.
 *
 * \see RtCharsetBufferFlush
 * \see RtCharsetOpen
 * \see RtCharsetClose
 * \see RtCharsetCreate
 * \see RtCharsetDestroy
 * \see RtCharsetGetDesc
 * \see RtCharsetSetColors
 * \see RwCameraBeginUpdate
 * \see RwCameraEndUpdate
 *
 */
RtCharset          *
RtCharsetPrintBuffered(RtCharset * charSet,
                       const RwChar * string,
                       RwInt32 x, RwInt32 y, RwBool hideSpaces)
{
    RtCharset          *result = (RwRaster *) NULL;
    RwInt32             numChars = rwstrlen(string);

    RWAPIFUNCTION(RWSTRING("RtCharsetPrintBuffered"));
    RWASSERT(charSet);
    RWASSERT(string);
    RWASSERT(_rtgBuffer.initialised != FALSE);

    if (numChars)
    {
        RwUInt32            length;
        RtCharsetDesc       desc;
        RwInt32             temp;

        GETCHARSETBITS(&desc, &temp);

        if ((NULL != _rtgBuffer.charSet)
            && (charSet != _rtgBuffer.charSet))
        {
            RtCharsetBufferFlush();
        }
        _rtgBuffer.charSet = charSet;

        length = strlen(string);
        while (length > 0)
        {
            if (length >= BUFFERNUMCHARS - _rtgBuffer.numChars)
            {
                length = BUFFERNUMCHARS - _rtgBuffer.numChars;
                _rtgBuffer.numChars +=
                    CharsetProcessString(string,
                                         length,
                                         charSet,
                                         x,
                                         y,
                                         &(_rtgBuffer.
                                           vertices[4 *
                                                    _rtgBuffer.
                                                    numChars]),
                                         &(_rtgBuffer.
                                           indices[6 *
                                                   _rtgBuffer.
                                                   numChars]),
                                         (RwImVertexIndex) (4 *
                                                            _rtgBuffer.
                                                            numChars),
                                         hideSpaces);
                RtCharsetBufferFlush();
                /* Flush clear _rtgBuffer.charSet but we ain't finished! */
                _rtgBuffer.charSet = charSet;
                x += length * desc.width;
                string += length;
                length = strlen(string);
            }
            else
            {
                _rtgBuffer.numChars +=
                    CharsetProcessString(string,
                                         length,
                                         charSet,
                                         x,
                                         y,
                                         &(_rtgBuffer.
                                           vertices[4 *
                                                    _rtgBuffer.
                                                    numChars]),
                                         &(_rtgBuffer.
                                           indices[6 *
                                                   _rtgBuffer.
                                                   numChars]),
                                         (RwImVertexIndex) (4 *
                                                            _rtgBuffer.
                                                            numChars),
                                         hideSpaces);
                length = 0;
            }
        }

        if (_rtgBuffer.numChars <= 0)
        {
            /* Maybe everything was clipped... so, avoid
             * a flush next time due to charSet changing */
            _rtgBuffer.charSet = (RwRaster *) NULL;
        }

        result = charSet;
    }

    RWRETURN(result);
}

/**
 * \ingroup rtcharset
 * \ref RtCharsetPrint is used to display the text in the given
 * string on the current raster using the specified character set. The
 * top-left position of the string is specified by the x- and y-coordinates
 * on the current raster.
 *
 * The width_internal and height_internal of a character (obtained by using the API function
 * \ref RtCharsetGetDesc) may be used in conjunction with the string length
 * to determine precise text positioning if required.
 *
 * Please note: \ref RtCharsetPrint now uses immediate mode triangles to
 * keep the speed up. It is no longer required to wrap calls to this
 * function between \ref RwRasterPushContext and \ref RwRasterPopContext
 * functions. Instead, calls to \ref RtCharsetPrint should be placed between
 * \ref RwCameraBeginUpdate and \ref RwCameraEndUpdate calls.
 *
 * The include file rtcharse.h and the library file rtcharse.lib are
 * required to use this function.
 *
 * \param charSet  Pointer to the raster character set.
 * \param string  Pointer to a string containing the characters to print.
 * \param x  A RwInt32 value equal to the x-coordinate.
 * \param y  A RwInt32 value equal to the y-coordinate.
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
 * \see RtCharsetGetDesc
 * \see RtCharsetSetColors
 * \see RwCameraBeginUpdate
 * \see RwCameraEndUpdate
 *
 */
RtCharset          *
RtCharsetPrint(RtCharset * charSet,
               const RwChar * string, RwInt32 x, RwInt32 y)
{
    RtCharset          *result = (RwRaster *) NULL;

    RWAPIFUNCTION(RWSTRING("RtCharsetPrint"));
    RWASSERT(charSet);
    RWASSERT(string);

#ifdef RTCHARSE_SUBRASTERS

    result = CharsetPrintSubRaster(charSet, string, x, y);
#else /* RTCHARSE_SUBRASTERS */

    result = CharsetPrintNoSubRaster(charSet, string, x, y);
#endif /* RTCHARSE_SUBRASTERS */

    RWRETURN(result);
}
