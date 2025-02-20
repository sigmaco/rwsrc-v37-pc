/*
 *
 * Functionality for 2D rendering
 *
 * Copyright (c) 1998 Criterion Software Ltd.
 */

/****************************************************************************
 *                                                                          *
 *  Module  :   font.c                                                      *
 *                                                                          *
 *  Purpose :   graphics state                                              *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
 */
#include <string.h>

#include "rwcore.h"
#include "rpdbgerr.h"
#include "rpworld.h"
#include "rtfsyst.h"

#include "rt2d.h"
#include "path.h"
#include "font.h"
#include "tri.h"
#include "gstate.h"
#include "brush.h"

#if (defined(SKY2_DRVMODEL_H))
#include "rppds.h"
#endif /* (defined(SKY2_DRVMODEL_H)) */

/****************************************************************************
 Local Types
 */
typedef RwChar      RwStringFilename[128];
typedef RwChar      RwStringLine[256];

/****************************************************************************
 Local (Static) Prototypes
 */
static Rt2dFont   *
FontDefaultRead(const RwChar * name);

/****************************************************************************
 Local Defines
 */

#define RwIsSpace(_c)       (0x20 == (_c))

#define RwIsNewLine(_c)     (0x0A == (_c))

#define rt2dFONTCHARCODEMAX     0xFFFF

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */

/* UTF-8 decode macro. This code is copied into a function doe DEBUG builds. */

#if ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) )

#else /* ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) ) */

#define FontDecodeUTF8(_c, _utf, _stride) \
    if (((_utf)[0] & 0x80) == 0x00) \
    { \
        *(_c) = (_utf)[0]; \
        *(_stride) = 1; \
    } \
    else if (((_utf)[0] & 0xE0) == 0xC0) \
    { \
        *(_c) = (((_utf)[0] & 0x1F) << 6) | \
                 ((_utf)[1] & 0x3F); \
        *(_stride) = 2; \
    } \
    else if (((_utf)[0] & 0xF0) == 0xE0) \
    { \
        *(_c) = (((_utf)[0] & 0x0F) << 12) | \
                (((_utf)[1] & 0x3F) <<  6) | \
                 ((_utf)[2] & 0x3F); \
        *(_stride) = 3; \
    } \
    else if (((_utf)[0] & 0xF8) == 0xF0) \
    { \
        *(_c) = (((_utf)[0] & 0x07) << 18) | \
                (((_utf)[1] & 0x3F) << 12) | \
                (((_utf)[2] & 0x3F) <<  6) | \
                 ((_utf)[3] & 0x3F); \
        *(_stride) = 4; \
    } \
    else if (((_utf)[0] & 0xFC) == 0xF8) \
    { \
        *(_c) = (((_utf)[0] & 0x03) << 24) | \
                (((_utf)[1] & 0x3F) << 18) | \
                (((_utf)[2] & 0x3F) << 12) | \
                (((_utf)[3] & 0x3F) <<  6) | \
                 ((_utf)[4] & 0x3F); \
        *(_stride) = 5; \
    } \
    else if (((_utf)[0] & 0xFE) == 0xFC) \
    { \
        *(_c) = (((_utf)[0] & 0x01) << 30) | \
                (((_utf)[1] & 0x3F) << 24) | \
                (((_utf)[2] & 0x3F) << 18) | \
                (((_utf)[3] & 0x3F) << 12) | \
                (((_utf)[4] & 0x3F) <<  6) | \
                 ((_utf)[5] & 0x3F); \
        *(_stride) = 6; \
    } \
    else \
    { \
        *(_stride) = -1; \
    }


#endif /* ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) ) */


/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   Functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/****************************************************************************/

#if ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) )

static void
FontDecodeUTF8(RwInt32 *c, RwChar *utf, RwInt32 *stride)
{
    RWFUNCTION(RWSTRING("FontDecodeUTF8"));

    if ((utf[0] & 0x80) == 0x00)
    {
        *c = utf[0];
        *stride = 1;
    }
    else if ((utf[0] & 0xE0) == 0xC0)
    {
        RWASSERT((utf[1] & 0xC0) == 0x80);

        *c = ((utf[0] & 0x1F) << 6) |
              (utf[1] & 0x3F);
        *stride = 2;
    }
    else if ((utf[0] & 0xF0) == 0xE0)
    {
        RWASSERT((utf[1] & 0xC0) == 0x80);
        RWASSERT((utf[2] & 0xC0) == 0x80);

        *c = ((utf[0] & 0x0F) << 12) |
             ((utf[1] & 0x3F) <<  6) |
              (utf[2] & 0x3F);
        *stride = 3;
    }
    else if ((utf[0] & 0xF8) == 0xF0)
    {
        RWASSERT((utf[1] & 0xC0) == 0x80);
        RWASSERT((utf[2] & 0xC0) == 0x80);
        RWASSERT((utf[3] & 0xC0) == 0x80);

        *c = ((utf[0] & 0x07) << 18) |
             ((utf[1] & 0x3F) << 12) |
             ((utf[2] & 0x3F) <<  6) |
              (utf[3] & 0x3F);
        *stride = 4;
    }
    else if ((utf[0] & 0xFC) == 0xF8)
    {
        RWASSERT((utf[1] & 0xC0) == 0x80);
        RWASSERT((utf[2] & 0xC0) == 0x80);
        RWASSERT((utf[3] & 0xC0) == 0x80);
        RWASSERT((utf[4] & 0xC0) == 0x80);

        *c = ((utf[0] & 0x03) << 24) |
             ((utf[1] & 0x3F) << 18) |
             ((utf[2] & 0x3F) << 12) |
             ((utf[3] & 0x3F) <<  6) |
              (utf[4] & 0x3F);
        *stride = 5;
    }
    else if ((utf[0] & 0xFE) == 0xFC)
    {
        RWASSERT((utf[1] & 0xC0) == 0x80);
        RWASSERT((utf[2] & 0xC0) == 0x80);
        RWASSERT((utf[3] & 0xC0) == 0x80);
        RWASSERT((utf[4] & 0xC0) == 0x80);
        RWASSERT((utf[5] & 0xC0) == 0x80);

        *c = ((utf[0] & 0x01) << 30) |
             ((utf[1] & 0x3F) << 24) |
             ((utf[2] & 0x3F) << 18) |
             ((utf[3] & 0x3F) << 12) |
             ((utf[4] & 0x3F) <<  6) |
              (utf[5] & 0x3F);
        *stride = 6;
    }
    else
    {
        *stride = -1;
    }

    RWRETURNVOID();
}

#endif /* ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) ) */

/****************************************************************************/

static Rt2dFont    *
rt2dFontShowOutline(Rt2dFont * font,
                    const RwChar * string,
                    RwReal height, RwV2d * anchor, Rt2dBrush * brush)
{
    RwReal              layerDepth;
    const RwUInt8       *tmpstring;
    RwUInt8             charMask;
    rt2dCharRect        *charMap;
    RwInt32             character, charStride, charCode, uCharOffset, uCharShift0, uCharShift1;

    RWFUNCTION(RWSTRING("rt2dFontShowOutline"));

    layerDepth = Rt2dGlobals.layerDepth;

    if (font)
    {
        charMap = (rt2dCharRect *) rwSListGetArray(font->charMap);

        /* If the font is multibyte, then convert the input string to wide string */
        if (font->flag & rtFONTFLAG_UNICODE)
        {
            charStride = 2;
            charMask = (RwUInt8) 0xFF;
            uCharOffset = 1;
            uCharShift0 = 8;
            uCharShift1 = 0;
         }
        else
        {
            charStride = 1;
            charMask = (RwUInt8) 0x0;
            uCharOffset = 0;
            uCharShift0 = 0;
            uCharShift1 = 0;
        }

        tmpstring = (const RwUInt8 *) string;

        /* establish geometry space */
        Rt2dCTMPush();
        Rt2dCTMTranslate(anchor->x, anchor->y);
        Rt2dCTMScale(height, height);
        Rt2dCTMTranslate(((RwReal) 0), -font->baseline);

        FontStrGetChar(character, tmpstring, charMask, uCharOffset, uCharShift0, uCharShift1);

        while (character > 0)
        {
            rt2dCharRect       *cr;

            FontCharGetCode(charCode, character, font);

            tmpstring += charStride;

            if (charCode >= 0)
            {
                cr = &charMap[charCode];

                if (cr->tex == NULL)
                {
                    Rt2dPathStroke(cr->path, brush);
                    Rt2dCTMTranslate(cr->width + font->intergap, ((RwReal) 0));
                }
                else
                {
                    /* use cached bitmap */
                }
            }

            FontStrGetChar(character, tmpstring, charMask, uCharOffset, uCharShift0, uCharShift1);
        }

        Rt2dCTMPop();
    }

    RWRETURN(font);
}

static Rt2dFont    *
rt2dFontShowBitmap(Rt2dFont * font,
                   const RwChar * string,
                   RwReal height, RwV2d * anchor, Rt2dBrush * brush)
{
    RwIm3DVertex       *vdst;
    RwReal              x, y;
    RwReal              baseu;
    RwReal              oobaseu;
    RwReal              layerDepth;
    RwTexture          *currTex;
    RwInt32             vindex = 0, character, charCode, charStride, uCharOffset, uCharShift0, uCharShift1;
    const RwUInt8       *tmpstring;
    RwUInt8              charMask;
    rt2dCharRect        *charMap;

    RWFUNCTION(RWSTRING("rt2dFontShowBitmap"));

    layerDepth = Rt2dGlobals.layerDepth;

    if (font)
    {
        RwMatrix           *ctm;

        charMap = (rt2dCharRect *) rwSListGetArray(font->charMap);

        /* If the font is multibyte, then convert the input string to wide string */
        if (font->flag & rtFONTFLAG_UNICODE)
        {
            charStride = 2;
            charMask = (RwUInt8) 0xFF;
            uCharOffset = 1;
            uCharShift0 = 8;
            uCharShift1 = 0;
        }
        else
        {
            charStride = 1;
            charMask = (RwUInt8) 0x0;
            uCharOffset = 0;
            uCharShift0 = 0;
            uCharShift1 = 0;
        }

        tmpstring = (const RwUInt8 *) string;

        /* establish geometry space */
        Rt2dCTMPush();
        Rt2dCTMTranslate(anchor->x, anchor->y);
        Rt2dCTMScale(height, height);
        Rt2dCTMTranslate(((RwReal) 0), -font->baseline);

        ctm = _rt2dCTMGet();

        /* establish colour space */
        baseu = Rt2dFontGetStringWidth(font, (const RwChar *) string, ((RwReal) 1));
        oobaseu = ((RwReal) 1) / baseu;
        baseu = ((RwReal) 0);

        x = ((RwReal) 0);
        y = ((RwReal) 0);
        currTex = (RwTexture *)NULL;
        vdst = Rt2dGlobals.brushVertexCache;
        vindex = 0;

        FontStrGetChar(character, tmpstring, charMask, uCharOffset, uCharShift0, uCharShift1);

        while (character > 0)
        {
            rt2dCharRect       *cr;
            rt2dShadeParameters sp;
            RwInt32             red;
            RwInt32             green;
            RwInt32             blue;
            RwInt32             alpha;

            FontCharGetCode(charCode, character, font);

            tmpstring += charStride;

            if (charCode >= 0)
            {
                cr = &charMap[charCode];

                /* do we need to flush? */
                if ((cr->tex != currTex) || (vindex > MAXVINDEX - 6))
                {
                    /* anything to render? */
                    if (vindex)
                    {

                        if (RwIm3DTransform(Rt2dGlobals.brushVertexCache,
                            vdst - Rt2dGlobals.brushVertexCache,
                            ctm, rwIM3D_VERTEXUV | rwIM3D_NOCLIP))
                        {
                            RwIm3DRenderIndexedPrimitive
                                (rwPRIMTYPETRILIST, Rt2dGlobals.fonttopo, vindex);
                            RwIm3DEnd();

#ifdef AMB_SPECIFICxx
#ifdef RWDEBUG
                            WireRender2d(Rt2dGlobals.brushVertexCache,
                                        vdst - Rt2dGlobals.brushVertexCache,
                                        fonttopo, vindex);
#endif
#endif
                        }
                    }

                    if (cr->tex != currTex)
                    {
                        RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void *) RwTextureGetFilterMode(cr->tex));
                        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RwTextureGetRaster(cr->tex));
                        currTex = cr->tex;
                    }

                    vdst = Rt2dGlobals.brushVertexCache;
                    vindex = 0;

                    /* we're going to overwrite the UVs */
                    if (brush->texture && (brush->flag & FIELDUV))
                    {
                        brush->flag &= ~FIELDUV;
                    }
                }

                RwIm3DVertexSetPos(vdst, x, y, layerDepth);
                RwIm3DVertexSetU(vdst, cr->uv[0].x);
                RwIm3DVertexSetV(vdst, cr->uv[1].y);
                if (brush->flag & FIELDRGBA)
                {
                    RwRGBARealScale(&sp.col, &brush->dbottom.col, baseu * oobaseu);
                    RwRGBARealAdd(&sp.col, &brush->bottom.col, &sp.col);

                    red   = RwInt32FromRealMacro(sp.col.red);
                    green = RwInt32FromRealMacro(sp.col.green);
                    blue  = RwInt32FromRealMacro(sp.col.blue);
                    alpha = RwInt32FromRealMacro(sp.col.alpha);

                    RwIm3DVertexSetRGBA(vdst,
                        (RwUInt8) red, (RwUInt8) green, (RwUInt8) blue, (RwUInt8) alpha);
                }
                else
                {
                    RwIm3DVertexSetRGBA(vdst,
                                        (RwUInt8) brush->colorCache.red,
                                        (RwUInt8) brush->colorCache.green,
                                        (RwUInt8) brush->colorCache.blue,
                                        (RwUInt8) brush->colorCache.alpha);
                }
                vdst++;

                RwIm3DVertexSetPos(vdst, x, y + ((RwReal) 1), layerDepth);
                RwIm3DVertexSetU(vdst, cr->uv[0].x);
                RwIm3DVertexSetV(vdst, cr->uv[0].y);
                if (brush->flag & FIELDRGBA)
                {
                    RwRGBARealScale(&sp.col, &brush->dtop.col, baseu * oobaseu);
                    RwRGBARealAdd(&sp.col, &brush->top.col, &sp.col);

                    red   = RwInt32FromRealMacro(sp.col.red);
                    green = RwInt32FromRealMacro(sp.col.green);
                    blue  = RwInt32FromRealMacro(sp.col.blue);
                    alpha = RwInt32FromRealMacro(sp.col.alpha);

                    RwIm3DVertexSetRGBA(vdst,
                        (RwUInt8) red, (RwUInt8) green, (RwUInt8) blue, (RwUInt8) alpha);
                }
                else
                {
                    RwIm3DVertexSetRGBA(vdst,
                                        (RwUInt8) brush->colorCache.red,
                                        (RwUInt8) brush->colorCache.green,
                                        (RwUInt8) brush->colorCache.blue,
                                        (RwUInt8) brush->colorCache.alpha);
                }
                vdst++;

                RwIm3DVertexSetPos(vdst, x + cr->width, y, layerDepth);
                RwIm3DVertexSetU(vdst, cr->uv[1].x);
                RwIm3DVertexSetV(vdst, cr->uv[1].y);
                if (brush->flag & FIELDRGBA)
                {
                    RwRGBARealScale(&sp.col, &brush->dbottom.col, (baseu + cr->width) * oobaseu);
                    RwRGBARealAdd(&sp.col, &brush->bottom.col, &sp.col);

                    red   = RwInt32FromRealMacro(sp.col.red);
                    green = RwInt32FromRealMacro(sp.col.green);
                    blue  = RwInt32FromRealMacro(sp.col.blue);
                    alpha = RwInt32FromRealMacro(sp.col.alpha);

                    RwIm3DVertexSetRGBA(vdst,
                        (RwUInt8) red, (RwUInt8) green, (RwUInt8) blue, (RwUInt8) alpha);
                }
                else
                {
                    RwIm3DVertexSetRGBA(vdst,
                                        (RwUInt8) brush->colorCache.red,
                                        (RwUInt8) brush->colorCache.green,
                                        (RwUInt8) brush->colorCache.blue,
                                        (RwUInt8) brush->colorCache.alpha);
                }
                vdst++;

                RwIm3DVertexSetPos(vdst, x + cr->width, y + ((RwReal) 1), layerDepth);
                RwIm3DVertexSetU(vdst, cr->uv[1].x);
                RwIm3DVertexSetV(vdst, cr->uv[0].y);
                if (brush->flag & FIELDRGBA)
                {
                    RwRGBARealScale(&sp.col, &brush->dtop.col, (baseu + cr->width) * oobaseu);
                    RwRGBARealAdd(&sp.col, &brush->top.col, &sp.col);

                    red   = RwInt32FromRealMacro(sp.col.red);
                    green = RwInt32FromRealMacro(sp.col.green);
                    blue  = RwInt32FromRealMacro(sp.col.blue);
                    alpha = RwInt32FromRealMacro(sp.col.alpha);

                    RwIm3DVertexSetRGBA(vdst,
                        (RwUInt8) red, (RwUInt8) green, (RwUInt8) blue, (RwUInt8) alpha);
                }
                else
                {
                    RwIm3DVertexSetRGBA(vdst,
                                        (RwUInt8) brush->colorCache.red,
                                        (RwUInt8) brush->colorCache.green,
                                        (RwUInt8) brush->colorCache.blue,
                                        (RwUInt8) brush->colorCache.alpha);
                }
                vdst++;

                vindex += 6;

                /* move to next character */
                x += cr->width + font->intergap;

                /* move to next shading param */
                baseu += cr->width + font->intergap;
            }
            FontStrGetChar(character, tmpstring, charMask, uCharOffset, uCharShift0, uCharShift1);
        }

        /* render scragend */
        if (vindex)
        {

            if (RwIm3DTransform(Rt2dGlobals.brushVertexCache,
                    vdst - Rt2dGlobals.brushVertexCache, ctm, rwIM3D_VERTEXUV | rwIM3D_NOCLIP))
            {
                RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, Rt2dGlobals.fonttopo, vindex);
                RwIm3DEnd();

#ifdef AMB_SPECIFICxx
#ifdef RWDEBUG
                WireRender2d(Rt2dGlobals.brushVertexCache,
                    vdst - Rt2dGlobals.brushVertexCache, fonttopo, vindex);
#endif
#endif
            }
        }

        /* write back final position */
        anchor->x += x * height;

        Rt2dCTMPop();
    }

    RWRETURN(font);
}

static Rt2dFont *
FontCompressIndex( Rt2dFont * font )
{
    RwInt16         *newIndex;
    RwInt32          i, j, minIndex, maxIndex;

    RWFUNCTION(RWSTRING("FontCompressIndex"));

    minIndex = rt2dFONTCHARCODEMAX;
    maxIndex = 0;

    /* Find the max and min index */
    j = 0;
    i = 0;
    while (i < (RwInt32) font->charCount)
    {
        if (Rt2dGlobals.uCharIndex[i] > 0)
        {
            if (Rt2dGlobals.uCharIndex[i] < minIndex)
                minIndex = (RwInt32) Rt2dGlobals.uCharIndex[i];

            if (Rt2dGlobals.uCharIndex[i] > maxIndex)
                maxIndex = (RwInt32) Rt2dGlobals.uCharIndex[i];

            j = 1;
        }

        i++;
    }

    if (j > 0)
    {
        RWASSERT(minIndex <= maxIndex);

        newIndex = (RwInt16 *) RwMalloc((maxIndex - minIndex + 1) * sizeof(RwInt16),
                                        rwMEMHINTDUR_EVENT | rwID_2DPLUGIN);

        if (newIndex == NULL)
            RWRETURN(NULL);

        /* Initialise the index */
        for (i = 0; i < (maxIndex - minIndex + 1); i++)
        {
            newIndex[i] = -1;
        }

        /* Fill in the index */
        for (i = 0; i < font->charCount; i++)
        {
            j = Rt2dGlobals.uCharIndex[i];

            if (j > 0)
                newIndex[(j - minIndex)] = (RwInt16) i;
        }

        font->uCharIndexCount = (maxIndex - minIndex + 1);
        font->uCharOffset = minIndex;
        font->uCharIndex = newIndex;
    }

    RWRETURN(font);
}

static Rt2dFont    *
FontReadMetrics2d1(Rt2dFont * const font,
                   RwStringFilename * filename,
                   void *fp, RwStringLine * line)
{
    RwInt32             baseline;
    RwInt32             rasterWidth, rasterHeight;
    RwInt32             numtextures;
    RwChar              *mask;
    RwInt32             i, character, charMax, charCount, newUCharIndexSize, *uCharIndex, *newUCharIndex;
    RwInt16             *aCharIndex;
    rt2dCharRect        *charMap;
    RwTexDictionary     *texDict;
    Rt2dFont            *result;

    RWFUNCTION(RWSTRING("FontReadMetrics2d1"));

    /* Switch tex dictionary */
    texDict = RwTexDictionaryGetCurrent();
    RwTexDictionarySetCurrent(Rt2dGlobals.currFontTexDictionary);

    result = font;

    font->isOutline = FALSE;

    charCount = 0;
    charMax = font->charCount;
    if (charMax == 0)
        charMax = rtFONTCHARCOUNT;

    aCharIndex = font->aCharIndex;
    uCharIndex = Rt2dGlobals.uCharIndex;

    rwSListGetNewEntries(font->charMap, charMax,
        rwMEMHINTDUR_EVENT | rwID_2DPLUGIN);
    charMap = (rt2dCharRect *) rwSListGetArray(font->charMap);
    memset(charMap, 0, charMax * sizeof(rt2dCharRect));

    RwFgets(&line[0][0], sizeof(*line), fp);

    mask = Rt2dGlobals.filename;
    numtextures = rwsscanf(&line[0][0], RWSTRING("%s %s\n"), filename, mask);

    if ((numtextures == 1) || (numtextures == 2))
    {
        RwReal              oowidth, ooheight;
        RwInt32             nLeft, nRight, nTop, nBottom;
        RwRaster           *raster;
        RwTexture          *texture;

        /* read bitmap info */
        RwImageSetPath(Rt2dGlobals.fontPath);

        /*
         * No mask specified. Check if there is 'm' prefix file and use it
         * as the mask. This is to support old method of mask loading.
         */
        if (numtextures == 1)
        {
            rwsprintf(mask, RWSTRING("%sm%s"), Rt2dGlobals.fontPath, filename);

            if (RwFexist(mask))
            {
                rwsprintf(mask, RWSTRING("m%s"), filename);

                texture = RwTextureRead(&filename[0][0], mask);
            }
            else
            {
                texture = RwTextureRead(&filename[0][0], NULL);
            }
        }
        else
        {
            texture = RwTextureRead(&filename[0][0], mask);
        }

        if (texture)
        {
            font->textures[0] = texture;

            raster = RwTextureGetRaster(texture);

            rasterWidth = RwRasterGetWidth(raster);
            rasterHeight = RwRasterGetHeight(raster);

            /* leading info */
            RwFgets(&line[0][0], sizeof(*line), fp);
            rwsscanf(&line[0][0], RWSTRING("%d"), &baseline);

            /* read character placement info */
            oowidth = ((RwReal) 1) / (RwReal) rasterWidth;
            ooheight = ((RwReal) 1) / (RwReal) rasterHeight;

            while ((result == font) && (RwFgets(&line[0][0], sizeof(*line), fp)))
            {
                if (rwsscanf(&line[0][0], RWSTRING("%i %i %i %i %i\n"),
                            &character, &nLeft, &nTop, &nRight, &nBottom) == 5)
                {
                    rt2dCharRect       *pChar;

                    if (character > rt2dFONTCHARCODEMAX)
                    {
                        /* There was an error in decoding the char. */
                        RWMESSAGE((RWSTRING("Unsupported character code found.")));
                        result = NULL;
                    }
                    else
                    {

                         /* Need to grow array */
                        if (charCount >= Rt2dGlobals.uCharIndexSize)
                        {
                            newUCharIndexSize = Rt2dGlobals.uCharIndexSize + rtFONTCHARBATCH;

                            newUCharIndex = (RwInt32 *) RwMalloc(newUCharIndexSize * sizeof(RwInt32),
                                                                rwMEMHINTDUR_EVENT | rwID_2DPLUGIN);

                            memcpy((RwChar *) newUCharIndex, (RwChar *) uCharIndex,
                                Rt2dGlobals.uCharIndexSize * sizeof(RwInt32));

                            for (i = Rt2dGlobals.uCharIndexSize ; i < newUCharIndexSize; i++)
                                newUCharIndex[i] = -1;

                            if (uCharIndex)
                                RwFree(uCharIndex);

                            uCharIndex = newUCharIndex;

                            Rt2dGlobals.uCharIndex = uCharIndex;
                            Rt2dGlobals.uCharIndexSize = newUCharIndexSize;
                        }

                        /* Is this a unicode char ? */
                        if (character >= 0x80)
                        {
                            font->flag |= rtFONTFLAG_UNICODE;

                            uCharIndex[charCount] = character;
                        }
                        else
                        {
                            aCharIndex[character] = (RwInt16) charCount;
                        }

                        pChar = &charMap[(RwUInt32) charCount];

                        pChar->uv[0].x = (((RwReal) nLeft) + (RwReal) 0.5) * oowidth;
                        pChar->uv[0].y = (((RwReal) nTop) + (RwReal) 0.5) * ooheight;
                        pChar->uv[1].x = (((RwReal) nRight) + (RwReal) 0.5) * oowidth;
                        pChar->uv[1].y = (((RwReal) nBottom) + (RwReal) 0.5) * ooheight;

                        pChar->tex = texture;
                        pChar->width = ((RwReal) (nRight - nLeft)) / ((RwReal) (nBottom - nTop));
                        pChar->charpage = 0;

                        charCount++;

                        /* Need to grow array */
                        if (charCount >= charMax)
                        {
                            rwSListGetNewEntries(font->charMap, rtFONTCHARBATCH,
                                rwMEMHINTDUR_EVENT | rwID_2DPLUGIN);

                            charMap = (rt2dCharRect *) rwSListGetArray(font->charMap);

                            charMax += rtFONTCHARBATCH;

                            memset(&charMap[charCount], 0, rtFONTCHARBATCH * sizeof(rt2dCharRect));
                        }
                    }
                }
            }

            /* Compact the index array */
            if (result == font)
            {
                font->charCount = charCount;
                FontCompressIndex(font);

                if (Rt2dGlobals.uCharIndex)
                    memset(Rt2dGlobals.uCharIndex, -1, Rt2dGlobals.uCharIndexSize * sizeof(RwInt32));

                /* font info */
                font->height = (RwReal) (nBottom - nTop);
                font->baseline = (RwReal) baseline / font->height;
                font->intergap = ((RwReal) 0);
                /* font->flag |= rtFONTFLAG_SINGLEPAGE; */

#if (defined(SKY2_DRVMODEL_H))
                font->fontshowCB = _rt2dPS2FontShow;
#endif /* (defined(SKY2_DRVMODEL_H)) */
            }
        }
        else
        {
            /* Error */
            result = NULL;
        }
    }

    /* Restore the tex dictionary */
    RwTexDictionarySetCurrent(texDict);

    RWRETURN(result);
}

static Rt2dFont    *
FontReadMetrics2d2(Rt2dFont * const font,
                   RwStringFilename * filename,
                   void *fp, RwStringLine * line)
{
    RwImage            *img;
    RwInt32             imgWidth, imgHeight, baseline;
    RwInt32             rasterWidth, rasterHeight;
    RwChar              *mask;
    RwUInt32            pagenum = 0, bufferSize, bufferLen;
    RwInt32             i, character, charMax, numChar, charStride, charCount, newUCharIndexSize;
    RwInt32             *uCharIndex, *newUCharIndex;
    RwInt16             *aCharIndex;
    rt2dCharRect        *charMap;
    RwUInt32            numtextures;
    RwChar              *utf;
    RwTexDictionary     *texDict;
    Rt2dFont            *result;

    RWFUNCTION(RWSTRING("FontReadMetrics2d2"));

    /* Switch tex dictionary */
    texDict = RwTexDictionaryGetCurrent();
    RwTexDictionarySetCurrent(Rt2dGlobals.currFontTexDictionary);

    result = font;

    font->isOutline = FALSE;

    numChar = 0;
    charCount = 0;
    charMax = font->charCount;
    if (charMax == 0)
        charMax = rtFONTCHARCOUNT;

    utf = NULL;
    bufferSize = (sizeof(*line) >> 1);
    bufferLen = 0;

    aCharIndex = font->aCharIndex;
    uCharIndex = Rt2dGlobals.uCharIndex;

    rwSListGetNewEntries(font->charMap, charMax,
        rwMEMHINTDUR_EVENT | rwID_2DPLUGIN);
    charMap = (rt2dCharRect *) rwSListGetArray(font->charMap);
    memset(charMap, 0, charMax * sizeof(rt2dCharRect));

    mask = Rt2dGlobals.filename;

    img = NULL;
    imgHeight = 0;
    imgWidth = 0;

    while ((pagenum < rtFONTMAXPAGES) &&
           RwFgets(&line[0][0], sizeof(*line), fp) &&
           (numtextures = rwsscanf(&line[0][0], RWSTRING("%s %s\n"), filename, mask)))
    {
        RwReal              oowidth, ooheight;
        RwInt32             depth, x, y, nHeight;
        RwInt32             nLeft, nRight, nTop, nBottom;
        RwUInt8            *pix, *testpix;
        RwRaster           *raster;
        RwTexture          *texture;

        if ((numtextures != 1) && (numtextures != 2))
        {
            break;
        }

        /* read bitmap info */
        RwImageSetPath(Rt2dGlobals.fontPath);

        if (numtextures == 1)
        {
            texture = RwTextureRead(&filename[0][0], NULL);
        }
        else
        {
            texture = RwTextureRead(&filename[0][0], mask);
        }

        if (texture)
        {
            font->textures[pagenum] = texture;

            raster = RwTextureGetRaster(texture);

            rasterWidth = RwRasterGetWidth(raster);
            rasterHeight = RwRasterGetHeight(raster);

            if (NULL != img)
            {
                if (imgWidth != rasterWidth)
                {
                    RwImageDestroy(img);
                    img = NULL;
                }
                else if (imgHeight != rasterHeight)
                {
                    RwImageDestroy(img);
                    img = NULL;
                }
            }

            if (NULL == img)
            {
                img = RwImageCreate(rasterWidth, rasterHeight, 32);

                RWASSERT(img);

                RwImageAllocatePixels(img);
            }

            RwImageSetFromRaster(img, raster);

            imgWidth = rasterWidth;
            imgHeight = rasterHeight;

            oowidth = ((RwReal) 1) / (RwReal) imgWidth;
            ooheight = ((RwReal) 1) / (RwReal) imgHeight;

            /* leading info */
            RwFgets(&line[0][0], sizeof(*line), fp);
            rwsscanf(&line[0][0], "%d", &baseline);

            /* extract character placements */
            pix = RwImageGetPixels(img);
            depth = RwImageGetDepth(img) >> 3;
            nHeight = 0;

            for (y = 1; y < imgHeight; y++)
            {
                testpix = pix + RwImageGetStride(img) * y;

                if (memcmp(pix, testpix, depth) == 0)
                {
                    nHeight = y;
                    break;
                }
            }

            /* Make sure we have a valid height */
            RWASSERT(nHeight > 0);

            if (nHeight > 0)
            {
                for (y = 0; (y < imgHeight && result == font); y += nHeight)
                {
                    nLeft = 0;
                    for (x = 1; (x < imgWidth && result == font); x++)
                    {
                        testpix = pix + RwImageGetStride(img) * y + depth * x;
                        if (memcmp(pix, testpix, depth) == 0)
                        {
                            /* zero width marks end of line */
                            if (x - nLeft == 1)
                            {
                                break;
                            }
                            else
                            {
                                rt2dCharRect       *pChar;

                                nRight = x;
                                nTop = y;
                                nBottom = nTop + nHeight;

                                /* Is the buffer empty ? */
                                if ((utf == NULL) || (utf[0] == '\n') || (utf[0] == '\0'))
                                {
                                    RwFgets(&line[0][0], sizeof(*line), fp);

                                    utf = &line[0][0];
                                }
                                else if ((utf - (RwChar *) (&line[0][0])) >= (RwInt32) (sizeof(*line) >> 1))
                                {
                                    /* Top up the buffer if needed. */
                                    bufferLen = rwstrlen(utf);

                                    if( (bufferLen + utf - (&line[0][0])) == (sizeof(*line) - 1) )
                                    {
                                        /* Transfer the remaining string to the
                                        * head of the buffer.
                                        */
                                        memcpy(&line[0][0], utf, bufferLen + 1);

                                        utf = &line[0][0];

                                        /* Check if the last char is a newline, if so
                                        * strip it.
                                        */
                                        if (utf[(bufferLen - 1)] == '\n')
                                            bufferLen--;

                                        /* Top up the buffer. */
                                        bufferSize = sizeof(*line) - bufferLen;

                                        RwFgets(&utf[bufferLen], bufferSize, fp);
                                    }
                                }
                                else if ((utf - (RwChar *) (&line[0][0])) >= (RwInt32) (sizeof(*line) >> 1))
                                {
                                    /* Top up the buffer. */

                                    /* Transfer the remaining string to the
                                     * head of the buffer.
                                     */
                                    bufferLen = rwstrlen(utf);
                                    memcpy(&line[0][0], utf, bufferLen + 1);

                                    utf = &line[0][0];

                                    /* Check if the last char is a newline, if so
                                     * strip it.
                                     */
                                    if (utf[(bufferLen - 1)] == '\n')
                                        bufferLen--;

                                    /* Top up the buffer. */
                                    bufferSize = sizeof(*line) - bufferLen;

                                    RwFgets(&utf[bufferLen], bufferSize, fp);
                                }

                                FontDecodeUTF8(&character, utf, &charStride);

                                /* Check if the character was decoded successfully. */
                                if ((charStride > 0) && (character <= rt2dFONTCHARCODEMAX))
                                {
                                    utf += charStride;

                                    /* Need to grow array */
                                    if (charCount >= Rt2dGlobals.uCharIndexSize)
                                    {
                                        newUCharIndexSize = Rt2dGlobals.uCharIndexSize + rtFONTCHARBATCH;

                                        newUCharIndex = (RwInt32 *) RwMalloc(newUCharIndexSize * sizeof(RwInt32),
                                                                            rwMEMHINTDUR_EVENT | rwID_2DPLUGIN);

                                        memcpy((RwChar *) newUCharIndex, (RwChar *) uCharIndex,
                                            Rt2dGlobals.uCharIndexSize * sizeof(RwInt32));

                                        for (i = Rt2dGlobals.uCharIndexSize ; i < newUCharIndexSize; i++)
                                            newUCharIndex[i] = -1;

                                        if (uCharIndex)
                                            RwFree(uCharIndex);

                                        uCharIndex = newUCharIndex;

                                        Rt2dGlobals.uCharIndex = uCharIndex;
                                        Rt2dGlobals.uCharIndexSize = newUCharIndexSize;
                                    }

                                    /* Is this a unicode char ? */
                                    if (character >= 0x80)
                                    {
                                        font->flag |= rtFONTFLAG_UNICODE;

                                        uCharIndex[charCount] = character;
                                    }
                                    else
                                    {
                                        aCharIndex[character] = (RwInt16) charCount;
                                    }

                                    pChar = &charMap[(RwUInt32) charCount];

                                    /* Explanation.
                                     *
                                     * The co-ordinates are specified in the following manner
                                     *
                                     *     x0   x1   x2   x3   x4   x5
                                     *  y0 +----+----+----+----+----+
                                     *     | A  |    |    |    |    |
                                     *  y1 +----+----+----+----+----+
                                     *     |    |    |    | B  |    |
                                     *  y2 +----+----+----+----+----+
                                     *     |    |    |    |    |    |
                                     *  y3 +----+----+----+----+----+
                                     *
                                     * Generally, a texel's co-ordinate is at its top left corner, texel A
                                     * is at (x0, y0)
                                     *
                                     * For the left and the top, we consider the pixels to the right and
                                     * below.
                                     * For the right and bottom, we consider the pixels to the left and
                                     * above.
                                     *
                                     *
                                     * Each char is surrounded by a border to mark its boundaries. The
                                     * borders should not be rendered so we need to inset the boundaries
                                     * to exclude them. Excluding just by one pixels is not enough due to
                                     * filtering so we need to inset by 2 pixels.
                                     * To sample at the center of the texel, we need to further offset
                                     * by 0.5.
                                     */
                                    pChar->uv[0].x = (((RwReal) nLeft) + (RwReal) 2.5) * oowidth;
                                    pChar->uv[0].y = (((RwReal) nTop) + (RwReal) 2.5) * ooheight;

                                    /*
                                     * The right and bottom also needs to be inset BUT only by one
                                     * texel not two. Why ? Because the boundary pixels at the right
                                     * and the bottom are part of the next char.
                                     *
                                     *          x1        x1'       x2'  x2'' x2
                                     *     +----+----+----+----+----+----+----+----+
                                     *     |    | t1 |    | t1'|    | t2'|    | t2 |
                                     *
                                     * Consider x1 and x2 marking the vertical boundaries of the char. The
                                     * border texels would be at t1 and t2.
                                     * t1 is part of the current char and t2 is part of the next char.
                                     * t1' and t2' are the texels of the char at x1' and x2' respectively.
                                     * However, when rendering the right edge, the left pixel is
                                     * filled only. So the right co-ordinate for the char is x2'', not x2'.
                                     *
                                     * Finally we also need to offset the right co-ordinate by 0.5 to ensure the
                                     * texel is sampled at the center.
                                     *
                                     * The same is also applied to the bottom boundary.
                                     */
                                    pChar->uv[1].x = (((RwReal) nRight) - (RwReal) 0.5) * oowidth;
                                    pChar->uv[1].y = (((RwReal) nBottom) - (RwReal) 0.5) * ooheight;
                                    pChar->tex = texture;

                                    /* Finally, the width of the char is the right pos - left pos -
                                     * the number of inset pixels. (2 from the left and 1 from the right.
                                     *
                                     * The width is expressed relative to the height.
                                     */
                                    pChar->width = ((RwReal) (nRight - nLeft - 3)) /
                                                   ((RwReal) (nBottom - nTop - 3));

                                    pChar->charpage = (RwChar) pagenum;

                                    charCount++;

                                    /* Need to grow array */
                                    if (charCount >= charMax)
                                    {
                                        rwSListGetNewEntries(font->charMap, rtFONTCHARBATCH,
                                            rwMEMHINTDUR_EVENT | rwID_2DPLUGIN);

                                        charMap = (rt2dCharRect *) rwSListGetArray(font->charMap);

                                        charMax += rtFONTCHARBATCH;

                                        memset(&charMap[charCount], 0, rtFONTCHARBATCH * sizeof(rt2dCharRect));
                                    }
                                }
                                else
                                {
                                    /* There was an error in decoding the char. */
                                    RWMESSAGE((RWSTRING("Unsupported character code found.")));

                                    /* error. Use page num to break out */
                                    pagenum = rtFONTMAXPAGES;
                                    result = NULL;
                                }

                                nLeft = nRight;
                            }
                        }
                    }
                }

                /* font info */

                utf = NULL;

                font->height = (RwReal) nHeight - 3;

                font->baseline = (RwReal) baseline / font->height;
                font->intergap = ((RwReal) 0);

                /* onto next page */
                pagenum++;
            }
            else
            {
                /* error. Use page num to break out */
                pagenum = rtFONTMAXPAGES;
                result = NULL;
            }
        }
        else
        {
            /* error. Use page num to break out */
            pagenum = rtFONTMAXPAGES;
            result = NULL;
        }
    }

    if (img != NULL)
        RwImageDestroy(img);

    /* Compress the index array */
    if (result == font)
    {
        font->charCount = charCount;
        FontCompressIndex(font);

        if (Rt2dGlobals.uCharIndex)
            memset(Rt2dGlobals.uCharIndex, -1, Rt2dGlobals.uCharIndexSize * sizeof(RwInt32));

        /*
        * If there is only one page, we don't need to check for textures during rendering.
        * This eliminates one loop.
        */
#if (defined(SKY2_DRVMODEL_H))
        font->fontshowCB = _rt2dPS2FontShow;
#endif /* (defined(SKY2_DRVMODEL_H)) */
    }

    /* Restore the tex dictionary */
    RwTexDictionarySetCurrent(texDict);

    RWRETURN(result);
}

static Rt2dFont    *
FontReadMetrics2d3(Rt2dFont * const font,
                   RwStringFilename * filename __RWUNUSED__,
                   void *fp, RwStringLine * line)
{
    Rt2dFont           *result;
    Rt2dPath           *path;
    RwChar              facename[128];
    RwInt32             i, character, charStride, charMax, charCount, newUCharIndexSize;
    RwInt32             *uCharIndex, *newUCharIndex;
    RwInt16             *aCharIndex;
    rt2dCharRect        *charMap;

    RWFUNCTION(RWSTRING("FontReadMetrics2d3"));

    /* Assume every is okay. */
    result = font;

    font->isOutline = TRUE;
    font->height = ((RwReal) 1);
    font->baseline = ((RwReal) 0);
    font->intergap = ((RwReal) 0);

    path = (Rt2dPath *)NULL;
    RwFgets(&line[0][0], sizeof(*line), fp);

    charCount = 0;
    charMax = font->charCount;
    if (charMax == 0)
        charMax = rtFONTCHARCOUNT;

    aCharIndex = font->aCharIndex;
    uCharIndex = Rt2dGlobals.uCharIndex;

    rwSListGetNewEntries(font->charMap, charMax,
        rwMEMHINTDUR_EVENT | rwID_2DPLUGIN);
    charMap = (rt2dCharRect *) (rwSListGetArray(font->charMap));

    memset(charMap, 0, charMax * sizeof(rt2dCharRect));

    if (rwsscanf(&line[0][0], RWSTRING("%[^\n]"), facename) == 1)
    {
        character = 0;

        while ((result == font) && (RwFgets(&line[0][0], sizeof(*line), fp)))
        {
            RwReal              x[3], y[3];

            if (character == 0)
            {
                FontDecodeUTF8(&character, &line[0][1], &charStride);

                if ((charStride < 0) || (character > rt2dFONTCHARCODEMAX))
                {
                    /* There was an error in decoding the char. */
                    RWMESSAGE((RWSTRING("Unsupported character code found.")));
                    result = NULL;
                }
            }
            else
            {
                if (!rwstrcmp(&line[0][0], RWSTRING("begin\n")))
                {
                    path = Rt2dPathCreate();
                    Rt2dPathLock(path);
                }
                else if (!rwstrcmp(&line[0][0], RWSTRING("end\n")))
                {
                    rt2dCharRect       *pChar;

                    path = _rt2dPathOptimize(path);

                    Rt2dPathUnlock(path);

                     /* Need to grow array */
                    if (charCount >= Rt2dGlobals.uCharIndexSize)
                    {
                        newUCharIndexSize = Rt2dGlobals.uCharIndexSize + rtFONTCHARBATCH;

                        newUCharIndex = (RwInt32 *) RwMalloc(newUCharIndexSize * sizeof(RwInt32),
                                                             rwMEMHINTDUR_EVENT | rwID_2DPLUGIN);

                        memcpy((RwChar *)newUCharIndex, (RwChar *)uCharIndex,
                            Rt2dGlobals.uCharIndexSize * sizeof(RwInt32));

                        for (i = Rt2dGlobals.uCharIndexSize ; i < newUCharIndexSize; i++)
                            newUCharIndex[i] = -1;

                        if (uCharIndex)
                            RwFree(uCharIndex);

                        uCharIndex = newUCharIndex;

                        Rt2dGlobals.uCharIndex = uCharIndex;
                        Rt2dGlobals.uCharIndexSize = newUCharIndexSize;
                    }

                    /* Is this a unicode char ? */
                    if (character >= 0x80)
                    {
                        font->flag |= rtFONTFLAG_UNICODE;

                        uCharIndex[charCount] = character;
                    }
                    else
                    {
                        aCharIndex[character] = (RwInt16) charCount;
                    }

                    pChar = &charMap[(RwUInt32) charCount];

                    pChar->path = path;
                    pChar->width = x[0];
                    character = 0;
                    path = (Rt2dPath *)NULL;

                    charCount++;

                    /* Need to grow array */
                    if (charCount >= charMax)
                    {
                        rwSListGetNewEntries(font->charMap, rtFONTCHARBATCH,
                            rwMEMHINTDUR_EVENT | rwID_2DPLUGIN);

                        charMap = (rt2dCharRect *) rwSListGetArray(font->charMap);

                        charMax += rtFONTCHARBATCH;

                        memset(&charMap[charCount], 0, rtFONTCHARBATCH * sizeof(rt2dCharRect));
                    }
                }
                else if (path)
                {
                    /* don't add until we have a path */
                    if (rwsscanf(&line[0][0],  RWSTRING("moveto %f %f\n"), &x[0], &y[0]) == 2)
                    {
                        Rt2dPathMoveto(path, x[0], y[0]);
                    }
                    else if (rwsscanf(&line[0][0], RWSTRING("lineto %f %f\n"), &x[0], &y[0]) == 2)
                    {
                        Rt2dPathLineto(path, x[0], y[0]);
                    }
                    else if (rwsscanf(&line[0][0], RWSTRING ("curveto %f %f %f %f %f %f\n"),
                                      &x[0], &y[0], &x[1], &y[1], &x[2], &y[2]) == 6)
                    {
                        Rt2dPathCurveto(path, x[0], y[0], x[1], y[1], x[2], y[2]);
                    }
                    else if (!rwstrcmp (&line[0][0], RWSTRING("closepath\n")))
                    {
                        Rt2dPathClose(path);
                    }
                }
            }
        }
    }

    if (result == font)
    {
        /* Compress the index array */
        font->charCount = charCount;
        FontCompressIndex(font);

        if (Rt2dGlobals.uCharIndex)
            memset(Rt2dGlobals.uCharIndex, -1, Rt2dGlobals.uCharIndexSize * sizeof(RwInt32));

        font->fontshowCB = rt2dFontShowOutline;
    }

    RWRETURN(result);
}

/****************************************************************************/

static Rt2dFont    *
FontCreate2d( void )
{
    Rt2dFont           *font;
    RwInt32         i;

    RWFUNCTION(RWSTRING("FontCreate2d"));

    font = (Rt2dFont *)RwFreeListAlloc(Rt2dGlobals.fontFreeList,
                           rwMEMHINTDUR_EVENT | rwID_2DFONT);
    if (!font)
    {
        RWRETURN((Rt2dFont *)NULL);
    }

    /* initialise */
    font->isOutline = TRUE;
    font->height = ((RwReal) 1);
    font->baseline = ((RwReal) 0);
    font->intergap = ((RwReal) 0);
    font->flag = ((RwUInt32) 0);

    for (i = 0; i < rtFONTMAXPAGES; i++)
        font->textures[i] = NULL;

    font->uCharIndexCount = 0;
    font->charCount = 0;
    font->uCharOffset = 0;

    font->uCharIndex = NULL;
    font->charMap = rwSListCreate(sizeof(rt2dCharRect),
        rwMEMHINTDUR_EVENT | rwID_2DPLUGIN);

    font->fontshowCB = rt2dFontShowBitmap;

    font->nextBatch = (Rt2dFont *)NULL;
    font->strCache = NULL;

    for (i = 0; i < rtFONTACHARINDEXSIZE; i++)
    {
        font->aCharIndex[i] = -1;
    }

    RWRETURN(font);
}

/***************************************************************************
 */
static RwUInt32
rt2dFontOutlineStreamGetSize(Rt2dFont * font)
{
    RwUInt32            i, size;
    Rt2dPath           *path;
    rt2dCharRect        *charMap;

    RWFUNCTION(RWSTRING("rt2dFontOutlineStreamGetSize"));

    size = 0;

    charMap = (rt2dCharRect *) rwSListGetArray(font->charMap);

    /* the chars. */
    for (i = 0; i < (RwUInt32) font->charCount; i++)
    {
        path = charMap[i].path;

        /* char begin & end marker. */
        size += 2 * sizeof(RwInt32);

        /* char's width. */
        size += sizeof(RwReal);

        if (path)
        {
            /* path begin & end marker. */
            size += 2 * sizeof(RwInt32);

            size += Rt2dPathStreamGetSize(path) + rwCHUNKHEADERSIZE;
        }
        else
        {
            /* end marker. */
            size += sizeof(RwInt32);
        }
    }

    RWRETURN(size);
}

static RwUInt32
rt2dFontBitmapStreamGetSize(Rt2dFont * font)
{
    RwUInt32            i, size;

    RWFUNCTION(RWSTRING("rt2dFontBitmapStreamGetSize"));

    RWASSERT(font);
    RWASSERT(!font->isOutline);

    size = 0;

    /* Char's uv and width */
    size += font->charCount * ((2 * sizeof(RwV2d)) + sizeof(RwReal));

    /* Char's page name. */
    size += font->charCount * sizeof(RwChar);

    /* Font's textures */
    size += sizeof(RwUInt32);

    for (i = 0; i < rtFONTMAXPAGES; i++)
    {
        if (NULL != font->textures[i])
            size += rwTEXTUREBASENAMELENGTH * sizeof(RwChar);
    }

    RWRETURN(size);
}

/**
 * \ingroup rt2dfont
 * \ref Rt2dFontStreamGetSize is used to  determine the size in bytes
 * of the binary representation of the given font. This value is used in
 * the binary chunk header to indicate the size of the chunk. The size of
 * the chunkheader is not included.
 *
 * \param font  Pointer to the font.
 *
 * \return Returns a RwUInt32 value equal to the chunk size (in bytes) of
 * the font or zero if there is an error.
 *
 * \see Rt2dFontStreamWrite
 * \see Rt2dFontStreamRead
 */

RwUInt32
Rt2dFontStreamGetSize(Rt2dFont * font)
{
    RwUInt32            size;

    RWAPIFUNCTION(RWSTRING("Rt2dFontStreamGetSize"));

    if (!font)
    {
        RWRETURN(0);
    }

    /* 2d font marker + version stamp */
    size = sizeof(RwInt32);

    /* Outline font flag */
    size += sizeof(RwUInt32);

   /* height, baseline, intergap */
    size += 3 * sizeof(RwReal);

    /* Font flag. */
    size += sizeof(RwUInt32);

    /* Font charCount, uCharIndexCount, uCharOffset */
    size += 3 * sizeof(RwUInt32);

    /* Font unicode char index */
    size += font->charCount * sizeof(RwInt16);

    /* Font ascii char index */
    size += rtFONTACHARINDEXSIZE * sizeof(RwInt16);

    if (font->isOutline)
    {
        size += rt2dFontOutlineStreamGetSize(font);
    }
    else
    {
        size += rt2dFontBitmapStreamGetSize(font);
    }

    RWRETURN(size);
}

/***************************************************************************
 */
static Rt2dFont    *
rt2dFontOutlineStreamWrite(Rt2dFont * font, RwStream * stream)
{
    RwInt32             i, marker;
    Rt2dPath           *path;
    rt2dCharRect        *charMap;

    RWFUNCTION(RWSTRING("rt2dFontOutlineStreamWrite"));

    charMap = (rt2dCharRect *) rwSListGetArray(font->charMap);

    /* the chars. */
    for (i = 0; i < font->charCount; i++)
    {
        /* Begin marker. */
        marker = RT2D_STREAM_FONT_BEGIN;

        if (RwStreamWriteInt32(stream, (RwInt32 *) &marker, sizeof(RwInt32)) == NULL)
            RWRETURN((Rt2dFont *)NULL);

        if (RwStreamWriteReal(stream, (RwReal *) &charMap[i].width, sizeof(RwReal)) == NULL)
            RWRETURN((Rt2dFont *) NULL);

        path = charMap[i].path;

        if (path)
        {
            /* Begin marker. */
            marker = RT2D_STREAM_PATH_BEGIN;

            if (RwStreamWrite(stream, (RwInt32 *) &marker, sizeof(RwInt32)) == NULL)
                RWRETURN((Rt2dFont *)NULL);

            if (Rt2dPathStreamWrite(path, stream) == NULL)
                RWRETURN((Rt2dFont *) NULL);

            /* end marker. */
            marker = RT2D_STREAM_PATH_END;

            if (RwStreamWriteInt32(stream, (RwInt32 *) &marker, sizeof(RwInt32)) == NULL)
                RWRETURN((Rt2dFont *)NULL);
        }
        else
        {
            /* end marker. */
            marker = RT2D_STREAM_PATH_END;

            if (RwStreamWriteInt32(stream, (RwInt32 *) &marker, sizeof(RwInt32)) == NULL)
                RWRETURN((Rt2dFont *)NULL);
        }

        /* End marker. */
        marker = RT2D_STREAM_FONT_END;

        if (RwStreamWriteInt32(stream, (RwInt32 *) &marker, sizeof(RwInt32)) == NULL)
            RWRETURN((Rt2dFont *)NULL);
    }

    RWRETURN(font);
}


static Rt2dFont           *
rt2dFontBitmapStreamWrite(Rt2dFont * font, RwStream * stream)
{
    RwInt32             i, count;
    rt2dCharRect        *charMap;
    RwChar              str[rwTEXTUREBASENAMELENGTH + 1];

    RWFUNCTION(RWSTRING("rt2dFontBitmapStreamWrite"));

    RWASSERT(font);
    RWASSERT(stream);

    charMap = (rt2dCharRect *) rwSListGetArray(font->charMap);

    /* the chars. */
    for (i = 0; i < font->charCount; i++)
    {
        /* char's uv. */
        if (RwStreamWriteReal(stream, (RwReal *) charMap[i].uv, (4 * sizeof(RwReal))) == NULL)
            RWRETURN((Rt2dFont *)NULL);

        /* char's width */
        if (RwStreamWriteReal(stream, (RwReal *) &charMap[i].width, sizeof(RwReal)) == NULL)
            RWRETURN((Rt2dFont *)NULL);

        /* char's texture. */
        if (RwStreamWrite(stream, (RwChar *) &charMap[i].charpage, sizeof(RwChar)) == NULL)
            RWRETURN((Rt2dFont *)NULL);
    }

    count = 0;
    for (i = 0; i < rtFONTMAXPAGES; i++)
    {
        if (NULL != font->textures[i])
            count++;
    }

    if (RwStreamWriteInt32(stream, (RwInt32 *)&count, sizeof(RwUInt32)) == NULL)
    {
        RWRETURN((Rt2dFont *)NULL);
    }

    for (i = 0; i < count; i++)
    {
        memset(str, 0, rwTEXTUREBASENAMELENGTH * sizeof(RwChar));
        rwsprintf(str, RwTextureGetName(font->textures[i]));

        if (RwStreamWrite(stream, str, rwTEXTUREBASENAMELENGTH) == NULL)
            RWRETURN((Rt2dFont *) NULL);
    }

    RWRETURN(font);
}

/**
 * \ingroup rt2dfont
 * \ref Rt2dFontStreamWrite is used to write the specified font to the
 * given binary stream.
 *
 * Note that the stream will have been opened prior to this function call.
 *
 * \param font    Pointer to the font.
 * \param stream  Pointer to the stream.
 *
 * \return Returns pointer to the font if successful or NULL if
 * there is an error.
 *
 * \see Rt2dFontStreamRead
 * \see Rt2dFontStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 *
 */

Rt2dFont *
Rt2dFontStreamWrite(Rt2dFont * font, RwStream * stream)
{
    RwUInt32        i, size, marker, ver;
    Rt2dFont        *result;

    RWAPIFUNCTION(RWSTRING("Rt2dFontStreamWrite"));

    RWASSERT(font);
    RWASSERT(stream);

    /* write out the font header */
    size = Rt2dFontStreamGetSize(font);
    if (RwStreamWriteChunkHeader(stream, rwID_2DFONT, size) == NULL)
        RWRETURN((Rt2dFont *)NULL);

    /* 2d font marker. */
    marker = RT2D_STREAM_FONT;
    ver = rtFONTVERSION;
    _rt2dMarkerEncode(i, ver, marker);
    if (RwStreamWriteInt32(stream, (RwInt32 *) &i, sizeof(RwInt32)) == NULL)
        RWRETURN((Rt2dFont *)NULL);

    /* outline font marker. */
    if (RwStreamWriteInt32(stream, (RwInt32 *) &font->isOutline, sizeof(RwInt32)) == NULL)
        RWRETURN((Rt2dFont *)NULL);

    /* height, baseline, intergap */
    if (RwStreamWriteReal(stream, &font->height, sizeof(RwReal)) == NULL)
        RWRETURN((Rt2dFont *)NULL);
    if (RwStreamWriteReal(stream, &font->baseline, sizeof(RwReal)) == NULL)
        RWRETURN((Rt2dFont *)NULL);
    if (RwStreamWriteReal(stream, &font->intergap, sizeof(RwReal)) == NULL)
        RWRETURN((Rt2dFont *)NULL);

    /* Font flag. */
    if (RwStreamWriteInt32(stream, (RwInt32 *) &font->flag, sizeof(RwInt32)) == NULL)
        RWRETURN((Rt2dFont *)NULL);


    /* Font char index */
    if (RwStreamWriteInt32(stream, (RwInt32 *) &font->uCharOffset, sizeof(RwInt32)) == NULL)
        RWRETURN((Rt2dFont *)NULL);
    if (RwStreamWriteInt32(stream, (RwInt32 *) &font->charCount, sizeof(RwInt32)) == NULL)
        RWRETURN((Rt2dFont *)NULL);
    if (RwStreamWriteInt32(stream, (RwInt32 *) &font->uCharIndexCount, sizeof(RwInt32)) == NULL)
        RWRETURN((Rt2dFont *)NULL);

    if (font->uCharIndexCount > 0)
    {
        if (RwStreamWriteInt16(stream,
            (RwInt16 *) font->uCharIndex, font->uCharIndexCount * sizeof(RwInt16)) == NULL)
            RWRETURN((Rt2dFont *)NULL);
    }
    if (RwStreamWriteInt16(stream,
        (RwInt16 *) font->aCharIndex, rtFONTACHARINDEXSIZE * sizeof(RwInt16)) == NULL)
        RWRETURN((Rt2dFont *)NULL);

    if (font->isOutline)
    {
        result = rt2dFontOutlineStreamWrite(font, stream);
    }
    else
    {
        result = rt2dFontBitmapStreamWrite(font, stream);
    }

    RWRETURN(font);
}

/***************************************************************************
 */

static Rt2dFont    *
rt2dFontOutlineStreamRead(Rt2dFont * font, RwStream * stream)
{
    RwInt32             i, marker;
    Rt2dPath            *path, *pathHead, *prevPath;
    rt2dCharRect        *charMap;

    RWFUNCTION(RWSTRING("rt2dFontOutlineStreamRead"));

    charMap = (rt2dCharRect *) rwSListGetArray(font->charMap);

    /* the chars. */
    for (i = 0; i < font->charCount; i++)
    {
        pathHead = (Rt2dPath *) NULL;
        path = (Rt2dPath *) NULL;
        prevPath = (Rt2dPath *) NULL;

        if (RwStreamReadInt32(stream, (RwInt32 *) &marker, sizeof(RwInt32)) == NULL)
            RWRETURN((Rt2dFont *)NULL);

        RWASSERT(marker == RT2D_STREAM_FONT_BEGIN);

        if (RwStreamReadReal(stream, (RwReal *) &charMap[i].width, sizeof(RwReal)) == NULL)
            RWRETURN((Rt2dFont *) NULL);

        if (RwStreamReadInt32(stream, (RwInt32 *) &marker, sizeof(RwInt32))== NULL)
            RWRETURN((Rt2dFont *)NULL);

        if (marker == RT2D_STREAM_PATH_BEGIN)
        {
            /* Path chunk header. */
            if (!RwStreamFindChunk(stream, rwID_2DPATH, NULL, NULL))
            {
                /* Something is wrong. */
                RWRETURN((Rt2dFont *) NULL);
            }

            pathHead = Rt2dPathStreamRead(stream);

            /* Next path or end path marker. */
            if (RwStreamReadInt32(stream, (RwInt32 *) &marker, sizeof(RwInt32)) == NULL)
                RWRETURN((Rt2dFont *)NULL);
        }

        /* font char end marker. */
        if (RwStreamReadInt32(stream, (RwInt32 *) &marker, sizeof(RwInt32)) == NULL)
            RWRETURN((Rt2dFont *)NULL);

        RWASSERT(marker == RT2D_STREAM_FONT_END);


        charMap[i].path = pathHead;
    }

    font->fontshowCB = rt2dFontShowOutline;

    RWRETURN(font);
}

static Rt2dFont           *
rt2dFontBitmapStreamRead(Rt2dFont * font, RwStream * stream)
{
    RwInt32             i, count, size;
    RwChar              str[rwTEXTUREBASENAMELENGTH + 1];
    rt2dCharRect        *charMap;

    RWFUNCTION(RWSTRING("rt2dFontBitmapStreamRead"));

    charMap = (rt2dCharRect *) rwSListGetArray(font->charMap);

    /* the chars. */
    for (i = 0; i < font->charCount; i++)
    {
        /* char's uv. */
        if (RwStreamReadReal(stream, (RwReal *) &charMap[i].uv, (4 * sizeof(RwReal))) == NULL)
            RWRETURN((Rt2dFont *)NULL);

        if (RwStreamReadReal(stream, (RwReal *) &charMap[i].width, sizeof(RwReal)) == NULL)
            RWRETURN((Rt2dFont *)NULL);

        /* char's texture index. */
        if (RwStreamRead(stream, (RwChar *) &charMap[i].charpage, sizeof(RwChar)) != sizeof(RwChar))
            RWRETURN((Rt2dFont *)NULL);

        charMap[i].path = NULL;
    }

    if (RwStreamReadInt32(stream, (RwInt32 *) &count, sizeof(RwInt32)) == NULL)
        RWRETURN((Rt2dFont *)NULL);

    size = rwTEXTUREBASENAMELENGTH * sizeof(RwChar);

    for (i = 0; i < count; i++)
    {
        if (RwStreamRead(stream, str, (RwUInt32) size) != (RwUInt32) size)
            RWRETURN((Rt2dFont *) NULL);

        if ((font->textures[i] = RwTextureRead(str, NULL)) == NULL)
            RWRETURN((Rt2dFont *) NULL);
    }

    for (i = 0; i < font->charCount; i++)
    {
        charMap[i].tex = font->textures[(RwUInt32) charMap[i].charpage];

        if (RwTextureGetRaster(charMap[i].tex) == NULL)
            RWRETURN((Rt2dFont *) NULL);
    }

    /*  The values loaded into these pointers will no longer be valid.
     *  Change them to default values to avoid crashes
     */
    font->nextBatch = (Rt2dFont *)NULL;
    font->strCache = NULL;

#if (defined(SKY2_DRVMODEL_H))
    font->fontshowCB = _rt2dPS2FontShow;
#else /* (defined(SKY2_DRVMODEL_H)) */
    font->fontshowCB = rt2dFontShowBitmap;
#endif /* (defined(SKY2_DRVMODEL_H)) */

    RWRETURN(font);
}

/**
 * \ingroup rt2dfont
 * \ref Rt2dFontStreamRead is used to read a font from the specified binary
 * stream.
 *
 * Note that prior to this function call, a binary chunk 2d chunk
 * header must be found in the stream using \ref RwStreamFindChunk API function.
 *
 * The sequence to locate and read a brush from a binary stream is
 * as follows:
 * \code
   RwStream  *stream;
   Rt2dFont *newFont;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_2DFONT, NULL, NULL) )
       {
           newFont = Rt2dFontStreamRead(stream);
       }

       RwStreamClose(stream, NULL);
   }
   \endcode
 *
 * \param stream  Pointer to the stream.
 *
 * \return Returns pointer to the font if successful or NULL if
 * there is an error.
 *
 * \see RwStreamFindChunk
 * \see Rt2dFontStreamWrite
 * \see Rt2dFontStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 */
Rt2dFont *
Rt2dFontStreamRead(RwStream * stream)
{
    Rt2dFont           *font;
    RwInt32            i, marker, ver;
    rt2dCharRect       *charMap;
    RwTexDictionary    *texDict;

    RWAPIFUNCTION(RWSTRING("Rt2dFontStreamRead"));

    RWASSERT(stream);

    font = FontCreate2d();
    if (!font)
    {
        RWRETURN((Rt2dFont *)NULL);
    }

    /* 2d font marker. */
    if (RwStreamReadInt32(stream, &i, sizeof(RwInt32)) == NULL)
        RWRETURN((Rt2dFont *)NULL);

    /* Extract the version stamp from the upper 8-bit. */
    _rt2dMarkerDecode(i, ver, marker);

    if (ver < rtFONTVERSION)
    {
        /* Cannot read older version due to texture dictionary. */
        RWMESSAGE((RWSTRING("Old version font found. Font cannot be read.")));
        RWRETURN(NULL);
    }

    RWASSERT(marker == RT2D_STREAM_FONT);

    /* outline font marker. */
    if (RwStreamReadInt32(stream, (RwInt32 *) &font->isOutline, sizeof(RwInt32)) == NULL)
        RWRETURN((Rt2dFont *)NULL);

    /* height, baseline, intergap */
    if (RwStreamReadReal(stream, &font->height, sizeof(RwReal)) == NULL)
        RWRETURN((Rt2dFont *)NULL);
    if (RwStreamReadReal(stream, &font->baseline, sizeof(RwReal)) == NULL)
        RWRETURN((Rt2dFont *)NULL);
    if (RwStreamReadReal(stream, &font->intergap, sizeof(RwReal)) == NULL)
        RWRETURN((Rt2dFont *)NULL);

    /* Font flag. */
    if (RwStreamReadInt32(stream, (RwInt32 *) &font->flag, sizeof(RwInt32)) == NULL)
        RWRETURN((Rt2dFont *)NULL);

    /* Font char index */
    if (RwStreamReadInt32(stream, (RwInt32 *) &font->uCharOffset, sizeof(RwInt32)) == NULL)
        RWRETURN((Rt2dFont *)NULL);
    if (RwStreamReadInt32(stream, (RwInt32 *) &font->charCount, sizeof(RwInt32)) == NULL)
        RWRETURN((Rt2dFont *)NULL);
    if (RwStreamReadInt32(stream, (RwInt32 *) &font->uCharIndexCount, sizeof(RwInt32)) == NULL)
        RWRETURN((Rt2dFont *)NULL);

    if (font->uCharIndexCount > 0)
    {
        font->uCharIndex = (RwInt16 *) RwMalloc(font->uCharIndexCount * sizeof(RwInt16),
                                                rwMEMHINTDUR_EVENT | rwID_2DPLUGIN);

        memset(font->uCharIndex, 0, font->uCharIndexCount * sizeof(RwInt16));

        if (font->uCharIndex == NULL)
        {
            Rt2dFontDestroy(font);

            RWRETURN(NULL);
        }

        /* Font uni char index */
        if (RwStreamReadInt16(stream,
            (RwInt16 *) font->uCharIndex, font->uCharIndexCount * sizeof(RwInt16)) == NULL)
            RWRETURN((Rt2dFont *)NULL);
    }

    /* Font ascii char index */
    if (RwStreamReadInt16(stream,
        (RwInt16 *) font->aCharIndex, rtFONTACHARINDEXSIZE * sizeof(RwInt16)) == NULL)
        RWRETURN((Rt2dFont *)NULL);

    rwSListGetNewEntries(font->charMap, font->charCount,
        rwMEMHINTDUR_EVENT | rwID_2DPLUGIN);

    charMap = (rt2dCharRect *) rwSListGetArray(font->charMap);

    RWASSERT(charMap);

    memset(charMap, 0, (font->charCount * sizeof(rt2dCharRect)));

    if (font->isOutline)
    {
        if ((rt2dFontOutlineStreamRead(font, stream)) == NULL)
        {
            Rt2dFontDestroy(font);

            font = (Rt2dFont *) NULL;
        }
    }
    else
    {
        /* Switch tex dictionary */
        texDict = RwTexDictionaryGetCurrent();
        RwTexDictionarySetCurrent(Rt2dGlobals.currFontTexDictionary);

        if ((rt2dFontBitmapStreamRead(font, stream)) == NULL)
        {
            Rt2dFontDestroy(font);

            font = (Rt2dFont *)NULL;
        }

        /* Restore tex dict */
        RwTexDictionarySetCurrent(texDict);
    }

    RWRETURN(font);
}

/****************************************************************************/

#ifdef AMB_SPECIFICxx
#ifdef RWDEBUG
static void
WireRender2d(RwIm3DVertex * vertex, int vcount,
             RwImVertexIndex * topo, int icount)
{
    RwIm3DVertex        local[VERTEXCACHESIZE];
    int                 i;

    RWFUNCTION(RWSTRING("WireRender2d"));

    for (i = 0; i < vcount; i++)
    {
        local[i] = vertex[i];
        RwIm3DVertexSetRGBA(&(local[i]), 255, 255, 0, 255);
    }

    if (RwIm3DTransform(local, vcount, _rt2dCTMGet(), 0))
    {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPEPOLYLINE, topo, icount);
        RwIm3DEnd();
    }

    RWRETURNVOID();
}
#endif /* RWDEBUG */
#endif /* AMB_SPECIFICxx */

/****************************************************************************/
void
_rt2dFontClose(void)
{
    _rt2dFontDictionaryNode *node;

    RWFUNCTION(RWSTRING("_rt2dFontClose"));

    /* Font dictionary stuff */
    node = Rt2dGlobals.fontDictionary;
    while(node)
    {
        _rt2dFontDictionaryNode *next;
        RwFree(node->name);
        Rt2dFontDestroy(node->font);
        next = node->next;
        RwFreeListFree(Rt2dGlobals.fontDictNodeFreeList, node);
        node=next;
    }
    Rt2dGlobals.fontDictionary = (_rt2dFontDictionaryNode *)NULL;

    if (Rt2dGlobals.fontTexDictionary != NULL)
        RwTexDictionaryDestroy(Rt2dGlobals.fontTexDictionary);
    Rt2dGlobals.fontTexDictionary = NULL;
    Rt2dGlobals.currFontTexDictionary = NULL;

    RwFreeListDestroy(Rt2dGlobals.fontDictNodeFreeList);

    Rt2dGlobals.fontDictNodeFreeList = (RwFreeList *)NULL;

    RwFreeListDestroy(Rt2dGlobals.fontFreeList);

    Rt2dGlobals.fontFreeList = (RwFreeList *)NULL;

    if (Rt2dGlobals.uCharIndex != NULL)
        RwFree(Rt2dGlobals.uCharIndex);

    Rt2dGlobals.uCharIndex = NULL;

    if (Rt2dGlobals.fontPath != NULL)
        RwFree(Rt2dGlobals.fontPath);
    Rt2dGlobals.fontPath = NULL;
    Rt2dGlobals.fontPathSize = 0;

    RWRETURNVOID();
}

static RwInt32 _rt2dFontFreeListBlockSize = 8,
               _rt2dFontFreeListPreallocBlocks = 1;
static RwFreeList _rt2dFontFreeList;

/**
 * \ingroup rt2d
 * \ref Rt2dFontSetFreeListCreateParams allows the developer to specify
 * how many \ref Rt2dFont s to preallocate space for.
 * Call before \ref RwEngineInit.
 *
 * \param blockSize  number of entries per freelist block.
 * \param numBlocksToPrealloc  number of blocks to allocate on
 * \ref RwFreeListCreateAndPreallocateSpace.
 *
 * \see RwFreeList
 *
 */
void
Rt2dFontSetFreeListCreateParams( RwInt32 blockSize, RwInt32 numBlocksToPrealloc )
{
    /* cannot use debugging macros since the debugger is not initialized before RwEngineInit */
    /*
    RWAPIFUNCTION( RWSTRING( "Rt2dFontSetFreeListCreateParams" ) );
    */

    _rt2dFontFreeListBlockSize = blockSize;
    _rt2dFontFreeListPreallocBlocks = numBlocksToPrealloc;

    /*
    RWRETURNVOID();
    */
}



static RwInt32 _rt2dFontDictNodeFreeListBlockSize = 16,
               _rt2dFontDictNodeFreeListPreallocBlocks = 1;
static RwFreeList _rt2dFontDictNodeFreeList;

/**
 * \ingroup rt2d
 * \ref Rt2dFontDictNodeSetFreeListCreateParams allows the developer to specify
 * how many _rt2dFontDictNode s to preallocate space for.
 * Call before \ref RwEngineInit.
 *
 * \param blockSize  number of entries per freelist block.
 * \param numBlocksToPrealloc  number of blocks to allocate on
 * \ref RwFreeListCreateAndPreallocateSpace.
 *
 * \see RwFreeList
 *
 */
void
Rt2dFontDictNodeSetFreeListCreateParams( RwInt32 blockSize, RwInt32 numBlocksToPrealloc )
{
    /* cannot use debugging macros since the debugger is not initialized before RwEngineInit */
    /*
    RWAPIFUNCTION( RWSTRING( "Rt2dFontDictNodeSetFreeListCreateParams" ) );
    */

    _rt2dFontDictNodeFreeListBlockSize = blockSize;
    _rt2dFontDictNodeFreeListPreallocBlocks = numBlocksToPrealloc;

    /*
    RWRETURNVOID();
    */
}

/****************************************************************************/
RwBool
_rt2dFontOpen(void)
{
    RwInt32             i;
    RwImVertexIndex     j, *fonttopo;

    RWFUNCTION(RWSTRING("_rt2dFontOpen"));

    Rt2dGlobals.fontFreeList =
        RwFreeListCreateAndPreallocateSpace(sizeof(Rt2dFont), _rt2dFontFreeListBlockSize, sizeof(RwUInt32),
        _rt2dFontFreeListPreallocBlocks, &_rt2dFontFreeList, rwMEMHINTDUR_GLOBAL | rwID_2DFONT);
    if (NULL == Rt2dGlobals.fontFreeList)
        RWRETURN(FALSE);

    Rt2dGlobals.fontDictNodeFreeList =
        RwFreeListCreateAndPreallocateSpace(sizeof(_rt2dFontDictionaryNode), _rt2dFontDictNodeFreeListBlockSize,
         sizeof(RwUInt32), _rt2dFontDictNodeFreeListPreallocBlocks, &_rt2dFontDictNodeFreeList,
         rwMEMHINTDUR_GLOBAL | rwID_2DPLUGIN);
    if (NULL == Rt2dGlobals.fontDictNodeFreeList)
        RWRETURN(FALSE);

    /* Font Path, default size 81 chars */
    Rt2dGlobals.fontPath = (RwChar *)
        RwMalloc(81 * sizeof(RwChar), rwMEMHINTDUR_GLOBAL | rwID_2DPLUGIN | rwMEMHINTFLAG_RESIZABLE);
    if (NULL == Rt2dGlobals.fontPath)
        RWRETURN(FALSE);

    Rt2dGlobals.fontPathSize = 81;
    Rt2dFontSetPath(RWSTRING("fonts\\"));

    fonttopo = Rt2dGlobals.fonttopo;

    for (i = 0, j = 0; i < MAXVINDEX; i += 6, j += 4)
    {
        fonttopo[i + 0] = j;
        fonttopo[i + 1] = j + 2;
        fonttopo[i + 2] = j + 3;
        fonttopo[i + 3] = j;
        fonttopo[i + 4] = j + 3;
        fonttopo[i + 5] = j + 1;
    }

    /* Font dictionary stuff */
    Rt2dGlobals.fontDictionary = (_rt2dFontDictionaryNode *)NULL;
    Rt2dGlobals.fontTexDictionary = RwTexDictionaryCreate();
    Rt2dGlobals.currFontTexDictionary = Rt2dGlobals.fontTexDictionary;

    /* Font tmp index */
    Rt2dGlobals.uCharIndex = NULL;
    Rt2dGlobals.uCharIndexSize = 0;

    /* Font read function */
    Rt2dFontSetReadCallBack(FontDefaultRead);


    RWRETURN(TRUE);
}

/****************************************************************************/

/**
 * \ingroup rt2dfont
 * \ref Rt2dFontSetPath
 * is used to specify the current search path for reading
 * fonts from the file system.
 *
 * The search path can be considered to be either absolute or
 * relative. In the latter case the search path is relative to the
 * directory from which the application executable is running (the
 * current directory).
 *
 * When fonts are read the image path is changed.
 *
 * Always include a trailing path separator in the directory name when
 * setting the search path.
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param path  Pointer to a string containing the current font search path.
 * \return pointer to the font path if successful or NULL if there is an
 * error.
 * \see Rt2dFontRead
 * \see RwImageSetPath
 */
const RwChar       *
Rt2dFontSetPath(const RwChar * path)
{
    RwChar          *newFontPath;
    RwUInt32        l;

    RWAPIFUNCTION(RWSTRING("Rt2dFontSetPath"));

    if (path)
    {
        l = rwstrlen(path) + 1;

        /* Need to reallocate ? */
        if (l > Rt2dGlobals.fontPathSize)
        {
            newFontPath = (RwChar *)
                RwRealloc(Rt2dGlobals.fontPath, l * sizeof(RwChar),
                    rwMEMHINTDUR_GLOBAL | rwID_2DPLUGIN | rwMEMHINTFLAG_RESIZABLE);

            if (NULL == newFontPath)
                RWRETURN((const char *)NULL);

            Rt2dGlobals.fontPath = newFontPath;
            Rt2dGlobals.fontPathSize = l;
        }

        rwstrcpy(Rt2dGlobals.fontPath, path);

        RWRETURN(path);
    }

    RWRETURN((const char *)NULL);
}

/**
 * \ingroup rt2dfont
 * \ref Rt2dFontSetReadCallBack is used to override the function that
 * will be used to read fonts.
 *
 * The default font loading mechanism can be used to read any fonts that
 * are accessible via the usual RenderWare file system.
 *
 * The format of the callback function is:
 *
 * \verbatim
   Rt2dFont * (*Rt2dFontCallBackRead) (const RwChar *name)
   \endverbatim
 *
 * \param fpCallBack  A pointer to the font-reading function
 *
 * \return Returns TRUE
 *
 * \see Rt2dFontGetReadCallBack
 * \see Rt2dFontRead
 *
 */
RwBool
Rt2dFontSetReadCallBack(Rt2dFontCallBackRead fpCallBack)
{
    RWAPIFUNCTION(RWSTRING("Rt2dFontSetReadCallBack"));
    RWASSERT(fpCallBack);

    Rt2dGlobals.fontReadFunc = fpCallBack;

    RWRETURN(TRUE);
}

/**
 * \ingroup rt2dfont
 * \ref Rt2dFontGetReadCallBack is used to enquire which function is
 * currently used for reading fonts.
 *
 * \return Returns a pointer to the user defined font reading function
 *
 * \see Rt2dFontSetReadCallBack
 * \see Rt2dFontRead
 *
 */
Rt2dFontCallBackRead
Rt2dFontGetReadCallBack(void)
{
    RWAPIFUNCTION(RWSTRING("Rt2dFontGetReadCallBack"));

    RWRETURN(Rt2dGlobals.fontReadFunc);
}

/****************************************************************************/

/**
 * \ingroup rt2dfont
 * \ref Rt2dFontGetHeight
 * is used to retrieve the 'natural' height of the
 * specified font. For outline fonts this function always returns 1.0,
 * but for bitmap fonts the returned value will depend on the current
 * view settings and the CTM. Therefore, using the bitmap font height
 * when rendering text ensures there is a one-to-one mapping to the
 * display; hence the text's rendered size remains independent of current
 * transformations.
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param font  Pointer to the font.
 * \return a RwReal value equal to the height of the font if successful
 * or zero if there is an error.
 * \see Rt2dFontGetStringWidth
 * \see Rt2dFontSetIntergapSpacing
 */
RwReal
Rt2dFontGetHeight(Rt2dFont * font)
{
    RwReal              result = ((RwReal) 0);
    RwV2d               xstep, ystep, origin;

    RWAPIFUNCTION(RWSTRING("Rt2dFontGetHeight"));

    RWASSERT(font);

    Rt2dDeviceGetStep(&xstep, &ystep, &origin);

    /*
     * rwSqrt(&result,
     *         ystep.x * ystep.x + ystep.y * ystep.y);
     * result *= font->height;
     */

    result = font->height * RwV2dLength(&ystep);

    RWRETURN(result);
}

Rt2dFont           *
_rt2dFontGetStringInfo(Rt2dFont * font, const RwChar * string,
                       RwReal * width, RwInt32 * l)
{
    RwReal              w;
    RwInt32             c;

    const RwUInt8       *tmpstring;
    RwUInt8             charMask;
    rt2dCharRect        *charMap;
    RwInt32             character, charStride, charCode, uCharOffset, uCharShift0, uCharShift1;


    RWFUNCTION(RWSTRING("_rt2dFontGetStringInfo"));

    w = ((RwReal) 0);
    c = 0;

    RWASSERT(font);

    charMap = rwSListGetArray(font->charMap);

    /* If the font is multibyte, then convert the input string to wide string */
    if (font->flag & rtFONTFLAG_UNICODE)
    {
        charStride = 2;
        charMask = (RwUInt8) 0xFF;
        uCharOffset = 1;
        uCharShift0 = 8;
        uCharShift1 = 0;
    }
    else
    {
        charStride = 1;
        charMask = (RwUInt8) 0x0;
        uCharOffset = 0;
        uCharShift0 = 0;
        uCharShift1 = 0;
    }

    tmpstring = (const RwUInt8 *) string;

    FontStrGetChar(character, tmpstring, charMask, uCharOffset, uCharShift0, uCharShift1);

    while (character > 0)
    {
        rt2dCharRect       *cr;

        FontCharGetCode(charCode, character, font);

        tmpstring += charStride;

        if (charCode >= 0)
        {
            cr = &charMap[charCode];

            w += (cr->width + font->intergap);
            c++;
        }

        FontStrGetChar(character, tmpstring, charMask, uCharOffset, uCharShift0, uCharShift1);
    }

    *width = w;
    *l = c;

    RWRETURN(font);
}

/****************************************************************************/

/**
 * \ingroup rt2dfont
 * \ref Rt2dFontGetStringWidth
 * is used to determine the width of the specified
 * string if it were to be rendered at the given height using the given
 * font.
 *
 * If the font contains just the ASCII character set, then the string is
 * assumed to be in single byte format.
 *
 * If the font contains characters outside the ASCII set, then it is assumed
 * to be Unicode and the string must be in double byte format.
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param font  Pointer to the font.
 * \param string  Pointer to the string.
 * \param height  A RwReal value equal to the height.
 * \return a RwReal value equal to the string's width if successful or
 * zero if there is an error.
 * \see Rt2dFontGetHeight
 * \see Rt2dFontSetIntergapSpacing
 * \see Rt2dFontShow
 */
RwReal
Rt2dFontGetStringWidth(Rt2dFont * font, const RwChar * string,
                       RwReal height)
{
    RwReal              width;

    const RwUInt8       *tmpstring;
    RwUInt8             charMask;
    rt2dCharRect        *charMap;
    RwInt32             character, charStride, charCode, uCharOffset, uCharShift0, uCharShift1;

    RWAPIFUNCTION(RWSTRING("Rt2dFontGetStringWidth"));

    width = ((RwReal) 0);
    RWASSERT(font);

    charMap = rwSListGetArray(font->charMap);

    /* If the font is multibyte, then convert the input string to wide string */
    if (font->flag & rtFONTFLAG_UNICODE)
    {
        charStride = 2;
        charMask = (RwUInt8) 0xFF;
        uCharOffset = 1;
        uCharShift0 = 8;
        uCharShift1 = 0;

    }
    else
    {
        charStride = 1;
        charMask = (RwUInt8) 0x0;
        uCharOffset = 0;
        uCharShift0 = 0;
        uCharShift1 = 0;
     }

    tmpstring = (const RwUInt8 *) string;

    FontStrGetChar(character, tmpstring, charMask, uCharOffset, uCharShift0, uCharShift1);

    while (character > 0)
    {
        rt2dCharRect       *cr;

        FontCharGetCode(charCode, character, font);

        tmpstring += charStride;

        if (charCode >= 0)
        {
            cr = &charMap[charCode];

            width += (cr->width + font->intergap);
        }

        FontStrGetChar(character, tmpstring, charMask, uCharOffset, uCharShift0, uCharShift1);
    }

    RWRETURN(width * height);
}

/****************************************************************************/

/****************************************************************************/

/**
 * \ingroup rt2dfont
 * \ref Rt2dFontFlow
 * is used to render the specified string in the given
 * bounding-box and have the text wrap over several lines if
 * necessary. The string is rendered using the specified font, at the
 * specified height and with the given brush. The text is justified
 * within the boundaries of the box according to the specified format.
 *
 * Note that text is rendered until it reaches the end of the string or
 * no more text can be accommodated inside the bounding-box, whichever
 * comes first.
 *
 * The brush is used to determine the color of the font during rendering.
 * The color is treated as a prelit color and is used to modulate the
 * font's bitmap. This means, if the font's bitmap is red and the color is
 * blue, the characters will appear black. Whereas, if the color was red
 * or white, the characters will appear red.
 *
 * The string being rendered is modified during rendering. If the length
 * of the string is greater than the width of the bounding box, the
 * string will be broken into substrings by inserting null characters into
 * any white space. Thus, any string to be rendered must be declared so that
 * it can be modified by the function (declared strings must reside in
 * read/write memory).
 *
 * If the font contains just the ASCII character set, the string is
 * assumed to be in single byte format. If the font contains characters
 * outside the ASCII set, it is assumed to be Unicode and the string
 * must be in double byte format.
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param font  Pointer to the font.
 * \param string  Pointer to a string containing the text to show.
 * \param height  A RwReal value equal to the height of the rendered text.
 * \param bbox  Pointer to the bounding-box.
 * \param format  Text justification.
 * \param brush  Pointer to the brush.
 * \return a pointer to the font if successful or NULL if there is an
 * error.
 * \see Rt2dFontShow
 * \see Rt2dFontRead
 */
Rt2dFont           *
Rt2dFontFlow(Rt2dFont * font,
             RwChar * string,
             RwReal height,
             Rt2dBBox * bbox, Rt2dJustificationType format,
             Rt2dBrush * brush)
{
    RwReal              bwidth;
    RwReal              justscale;
    rt2dCharRect        *charMap;
    RwInt32             character, charCode, charStride, charMask, uCharOffset, uCharShift0, uCharShift1;

    RWAPIFUNCTION(RWSTRING("Rt2dFontFlow"));

    charMap = (rt2dCharRect *) rwSListGetArray(font->charMap);

    bwidth = bbox->w / height;

    switch (format)
    {
        default:
        case rt2dJUSTIFYLEFT:
            justscale = ((RwReal) 0);
            break;

        case rt2dJUSTIFYCENTER:
            justscale = height * 0.5f;
            break;

        case rt2dJUSTIFYRIGHT:
            justscale = height;
            break;
    }

    if (font)
    {
        RwUInt8             *tmpstring, *substring;
        RwUInt8             *lastwordbreak;
        RwReal              width, breakwidth = ((RwReal) 0);
        RwV2d               anchor;
        RwBool              bFirstWhiteSpace;

         /* If the font is multibyte, then convert the input string to wide string */
        if (font->flag & rtFONTFLAG_UNICODE)
        {
            charStride = 2;
            charMask = (RwUInt8) 0xFF;
            uCharOffset = 1;
            uCharShift0 = 8;
            uCharShift1 = 0;
         }
        else
        {
            charStride = 1;
            charMask = (RwUInt8) 0x0;
            uCharOffset = 0;
            uCharShift0 = 0;
            uCharShift1 = 0;
        }

        tmpstring = (RwUInt8 *) string;
        substring = (RwUInt8 *) tmpstring;
        lastwordbreak = tmpstring;
        bFirstWhiteSpace = TRUE;
        width = ((RwReal) 0);
        anchor.y = bbox->y + bbox->h - height;

        FontStrGetChar(character, tmpstring, charMask, uCharOffset, uCharShift0, uCharShift1);

        while (character > 0)
        {
            rt2dCharRect       *cr;

            FontCharGetCode(charCode, character, font);

            if (bFirstWhiteSpace && RwIsSpace(character))
            {
                bFirstWhiteSpace = FALSE;
                lastwordbreak = tmpstring;
                breakwidth = width;
            }
            else
            {
                bFirstWhiteSpace = TRUE;

                if (RwIsNewLine(character))
                {
                    lastwordbreak = tmpstring + charStride;
                    breakwidth = width;
                }
            }

            if (charCode >= 0)
            {
                cr = &charMap[charCode];

                width += cr->width + font->intergap;
            }

            if ((width > bwidth) || RwIsNewLine(character))
            {
                RwInt32     lastCharacter;
                RwUInt8     savech[2];

                /* show substring */
                savech[0] = lastwordbreak[0];
                savech[1] = lastwordbreak[uCharOffset];

                lastwordbreak[0] = (RwChar) (0);
                lastwordbreak[uCharOffset] = (RwChar) (0);

                anchor.x = bbox->x + (bwidth - breakwidth) * justscale;

                font->fontshowCB(font, (RwChar *) substring, height, &anchor, brush);

                lastwordbreak[0] = savech[0];
                lastwordbreak[uCharOffset] = savech[1];

                FontStrGetChar(lastCharacter, lastwordbreak,  charMask, uCharOffset, uCharShift0, uCharShift1);

                /* eat white space */
                while (RwIsSpace(lastCharacter))
                {
                    lastwordbreak += charStride;

                    FontStrGetChar(lastCharacter, lastwordbreak, charMask, uCharOffset, uCharShift0, uCharShift1);
                }

                substring = lastwordbreak;
                tmpstring = lastwordbreak;

                /* bump to the next line */
                bFirstWhiteSpace = TRUE;
                width = ((RwReal) 0);
                anchor.y -= height;

                if (anchor.y < bbox->y)
                {
                    RWRETURN((Rt2dFont *)NULL);
                }
            }
            else
            {
                tmpstring += charStride;
            }

            /* Assume it is a wide string. The mask will strip the upper 8 bit if not needed */
            FontStrGetChar(character, tmpstring, charMask, uCharOffset, uCharShift0, uCharShift1);
        }

        anchor.x = bbox->x + (bwidth - width) * justscale;

        font->fontshowCB(font, (RwChar *) substring, height, &anchor, brush);
    }

    RWRETURN(font);
}

/****************************************************************************/

/**
 * \ingroup rt2dfont
 * \ref Rt2dFontDestroy
 * is used to destroy the specified font. All fonts
 * created by an application must be destroyed before the application
 * closes down.
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param font  Pointer to the font
 * \return TRUE if successful or FALSE if there is an error
 * \see Rt2dFontRead
 * \see Rt2dOpen
 * \see Rt2dClose
 */
RwBool
Rt2dFontDestroy(Rt2dFont * font)
{
    rt2dCharRect        *charMap;

    RWAPIFUNCTION(RWSTRING("Rt2dFontDestroy"));

    if (font)
    {
        RwUInt32            i;

        if (font->charMap)
        {
            charMap = (rt2dCharRect *) rwSListGetArray(font->charMap);

            for (i = 0; i < (RwUInt32) font->charCount; i++)
            {
                rt2dCharRect       *map = &charMap[i];

                if (NULL != map->path)
                {
                    Rt2dPathDestroy(map->path);
                    map->path = (Rt2dPath *)NULL;
                }
            }

            /* destroy the charn map */
            rwSListDestroy(font->charMap);

        }
        font->charMap = NULL;

        /* destroy the index */
        if (font->uCharIndex)
        {
            RwFree(font->uCharIndex);
        }
        font->uCharIndex = NULL;

        for (i = 0; i < rtFONTMAXPAGES; i++)
        {
            if (font->textures[i] != NULL)
                RwTextureDestroy(font->textures[i]);

            font->textures[i] = NULL;
        }

        RwFreeListFree(Rt2dGlobals.fontFreeList, font);

        RWRETURN(TRUE);
    }

    RWRETURN(FALSE);
}

/*
 ****************************************************************************
 */


static void *
AttemptToOpenFont( const RwChar* filename, RwChar* temporaryName )
{
    void    *fileFuncs;
    RwChar             *currentPath;
    void               *fp;
    RwInt32             indexDest;
    RwChar              currentChar;

    RWFUNCTION(RWSTRING("AttemptToOpenFont"));

    fileFuncs = NULL;//RwOsGetFileInterface();

    currentPath = Rt2dGlobals.fontPath;
    indexDest = 0;
    fp = NULL;
    do
    {
        /* Get a character from the global path */
        currentChar = *currentPath;

        /* Test if it is a separator or terminator */
        if( ( currentChar == ';' ) || ( currentChar == 0 ) )
        {
            /* NULL terminate */
            temporaryName[ indexDest ] = 0;

            /* Concatenate the strings */
            rwstrcat( temporaryName, filename );
            rwstrcat( temporaryName, ".met" );

            /* Try to load this file */
            fp = RwFopen(temporaryName, RWSTRING("r"));

            /* Start the copy into path string at the start */
            indexDest = 0;
        }
        else
        {
            /* Copy the character */
            temporaryName[ indexDest ] = currentChar;
            indexDest++;
        }

        currentPath++;

        /* Loop around until either the terminator is found or a file is opened */
    }
    while( ( currentChar != 0 ) && ( fp == NULL ) );

    RWRETURN( fp );
}

/****************************************************************************
FontDefaultRead

 On entry   : Name
 On exit    :
 */

/* this function is the default reader used by the global callback
 */
static Rt2dFont   *
FontDefaultRead(const RwChar * name)
{
    Rt2dFont           *font, *result;
    RwInt32             length;
    
#ifdef RWDEBUG
    RwChar              fontDebugString[40];
#endif /* RWDEBUG */


    RWFUNCTION(RWSTRING("FontDefaultRead"));

    font = FontCreate2d();
    if (!font)
    {
        RWRETURN((Rt2dFont *)NULL);
    }
    else
    {
        RwStringFilename    filename;
        void               *fp;
        RwChar             *temporaryName;

        length = rwstrlen( Rt2dGlobals.fontPath );
        /* Allocate enough space for the filename plus worse case scenario of path and the .met(NULL) extension */
        temporaryName = (RwChar*)RwMalloc( length + rwstrlen( name ) + 5,
            rwMEMHINTDUR_FUNCTION | rwID_2DPLUGIN );
        /* Make sure it was allocated */
        RWASSERT( temporaryName );

        /* Make sure it allocated */
        if( temporaryName )
        {
            fp = AttemptToOpenFont(name, temporaryName);

            result = NULL;
            if (fp)
            {
                RwStringLine        line;

                RwFgets(&line[0], sizeof(line), fp);

                if (!rwstrcmp(&line[0], RWSTRING("METRICS1\n")))
                {
                    result = FontReadMetrics2d1(font, &filename, fp, &line);
                }
                else if (!rwstrcmp(&line[0], RWSTRING("METRICS2\n")))
                {
                    result = FontReadMetrics2d2(font, &filename, fp, &line);
                }
                else if (!rwstrcmp(&line[0], RWSTRING("METRICS3\n")))
                {
                    result = FontReadMetrics2d3(font, &filename, fp, &line);
                }
                else
                {
                    /* Unknown format */
#ifdef RWDEBUG
                    sprintf(fontDebugString, "Unknown font format file - %s.", name);

                    RWASSERTM(FALSE, (fontDebugString));

#endif /* RWDEBUG */
                }

                RwFclose(fp);
            }

            /* Free the memory used for the temporary name */
            RwFree( temporaryName );

            /* Destroy the font if it failed to load */
            if (result != font)
            {
                Rt2dFontDestroy(font);

                font = (Rt2dFont *)NULL;
            }
        }
        else
        {
            Rt2dFontDestroy(font);

            font = (Rt2dFont *)NULL;
        }
    }

    RWRETURN(font);
}

/**
 * \ingroup rt2dfont
 * \ref Rt2dFontRead
 * is used to create a font by reading the information
 * contained in the specified font metrics file (the file extension
 * \c .met is assumed but it should not be included in the file
 * name). The path specified using the function \ref Rt2dFontSetPath is used
 * to indicate the location of the file in the file system. Internally,
 * the full pathname of the font file is obtained by concatenating the
 * font search path with the supplied file name and appending the
 * extension \c .met (note that the search path should including a
 * trailing path separator).
 *
 * The metrics file must be in UTF-8 format. This is to support Unicode
 * characters. Each line in the metric file must be less than 255 bytes,
 * not characters, in length and terminated by a newline character.
 *
 * A side effect of this function is that the image path is changed.
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param name  Pointer to a string containing the name of the font file.
 * \return a pointer to the font if successful or NULL if there is an
 * error. (This function will also return NULL if the font's image format
 * has not been registered using \ref RwImageRegisterImageFormat).
 *
 * \see Rt2dFontSetPath
 * \see Rt2dFontShow
 * \see Rt2dFontFlow
 * \see Rt2dFontDestroy
 * \see RwImageSetPath
 */
Rt2dFont           *
Rt2dFontRead(const RwChar * name)
{
    Rt2dFont           *font;

    RWAPIFUNCTION(RWSTRING("Rt2dFontRead"));

    font = Rt2dGlobals.fontReadFunc (name);

    RWRETURN(font);
}

/**
 * \ingroup rt2dfont
 * \ref Rt2dFontShow
 * is used to render the specified string using the given
 * font and brush. The height and anchor parameters specify the size and
 * position of the string text when it is rendered; the anchor defines
 * the position of lower-left corner of the string's bounding-box.
 * The anchor is updated to return the position at the end of the
 * string.
 *
 * Note that heights and positions are defined in absolute coordinates
 * and are subject to the current transformation matrix (CTM).
 *
 * The brush is used to determine the color of the font during rendering.
 * The color is treated as a prelit color and is used to modulate the
 * font's bitmap. This means, if the font's bitmap is red and the color is
 * blue, the characters will appear black. Whereas, if the color was red
 * or white, the characters will appear red.
 *
 * If the font contains just the ASCII character set, the string is
 * assumed to be in single byte format. If the font contains characters
 * outside the ASCII set, it is assumed to be Unicode and the string
 * must be in double byte format.
 *
 * Rt2dFontShow will ignore any characters that are not defined in
 * the .met file.  This includes characters such as the linefeed '\n' character.
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param font  Pointer to the font.
 * \param string  Pointer to the string.
 * \param height  A RwReal value equal to the height of the rendered text.
 * \param anchor  A RwV2d value specifying the position for the rendered text. It
 *                also returns the position of the end of the string on screen.
 * \param brush  Pointer to the brush.
 * \return a pointer to the font if successful or NULL if there is an
 * error.
 * \see Rt2dFontRead
 * \see Rt2dFontFlow
 * \see Rt2dFontGetHeight
 * \see Rt2dFontGetStringWidth
 */
Rt2dFont           *
Rt2dFontShow(Rt2dFont * font,
             const RwChar * string,
             RwReal height, RwV2d * anchor, Rt2dBrush * brush)
{
    RWAPIFUNCTION(RWSTRING("Rt2dFontShow"));

    RWRETURN(font->fontshowCB(font, string, height, anchor, brush));
}

/****************************************************************************/

/**
 * \ingroup rt2dfont
 * \ref Rt2dFontSetIntergapSpacing
 * is used to define the horizontal spacing of
 * characters when a string is rendered using the specified
 * font. Essentially, an application can use this function to control the
 * spread of rendered strings.
 *
 * The default spacing is zero, that is, no spreading.
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 *
 *
 * \param font  A pointer to the font.
 * \param gap  A RwReal value equal to the intergap spacing.
 * \return a pointer to the font if successful or NULL if there is an
 * error.
 * \see Rt2dFontGetHeight
 * \see Rt2dFontGetStringWidth
 */
Rt2dFont           *
Rt2dFontSetIntergapSpacing(Rt2dFont * font, RwReal gap)
{
    RWAPIFUNCTION(RWSTRING("Rt2dFontSetIntergapSpacing"));

    RWASSERT(font);

    font->intergap = gap;

    RWRETURN(font);
}

/****************************************************************************/

RwUInt32
_rt2dFontGetCharacterSetCount(Rt2dFont * font)
{
    RwUInt32            count;

    RWFUNCTION(RWSTRING("_rt2dFontGetCharacterSetCount"));

    RWASSERT(font);

    count = font->charCount;

    RWRETURN(count);
}

RwUInt32
_rt2dFontGetCharacterSet(Rt2dFont *font, RwInt32 *charSet)
{
    RwUInt32            i, j, count;

    RWFUNCTION(RWSTRING("_rt2dFontGetCharacterSet"));

    RWASSERT(font);
    RWASSERT(charSet);

    j = 0;
    count = font->charCount;

    /* Collect the represented ASCII chars */
    for (i = 0; i < rtFONTACHARINDEXSIZE; i++)
    {
        if (font->aCharIndex[i] >= 0)
        {
            charSet[j] = i;
            j++;
        }
    }

    /* Collect the represented Unicode chars */
    for (i = 0; i < (RwUInt32) font->uCharIndexCount; i++)
    {
        if (font->uCharIndex[i] >= 0)
        {
            charSet[j] = i + font->uCharOffset;
            j++;
        }
    }

    RWASSERT(j == count);

    RWRETURN(j);
}

/****************************************************************************/

static _rt2dFontDictionaryNode *
_rt2dFontDictionaryFindNamedNode(const RwChar *fontName)
{
    _rt2dFontDictionaryNode *node;

    RWFUNCTION(RWSTRING("_rt2dFontDictionaryFindNamedNode"));
    RWASSERT(fontName);

    /* look for the font */
    node = Rt2dGlobals.fontDictionary;
    while (node)
    {
        if (rwstrcmp(fontName, node->name)==0)
        {
            RWRETURN(node);
        }
        node = node->next;
    }

    RWRETURN((_rt2dFontDictionaryNode *)NULL);
}

static _rt2dFontDictionaryNode *
_rt2dFontDictionaryFindFontNode(Rt2dFont *font,
                                _rt2dFontDictionaryNode **prevResult)
{
    _rt2dFontDictionaryNode *node;
    _rt2dFontDictionaryNode *prev = NULL;

    RWFUNCTION(RWSTRING("_rt2dFontDictionaryFindFontNode"));
    RWASSERT(font);

    /* look for the font */
    node = Rt2dGlobals.fontDictionary;

    while (node)
    {
        if (font == node->font)
        {
            if (prevResult)
            {
                *prevResult = prev;
            }
            RWRETURN(node);
        }
        prev = node;
        node = node->next;
    }

    RWRETURN((_rt2dFontDictionaryNode *)NULL);
}

static _rt2dFontDictionaryNode *
_rt2dFontDictionaryAddNode(Rt2dFont *font, const RwChar *fontName)
{
    _rt2dFontDictionaryNode *node;
    RwInt32 namelen;

    RWFUNCTION(RWSTRING("_rt2dFontDictionaryAddNode"));

    RWASSERT(font);
    RWASSERT(fontName);
    RWASSERTM((!_rt2dFontDictionaryFindNamedNode(fontName)),
              (RWSTRING("Font is already in dictionary")));

    /* Allocate the new node */
    node = (_rt2dFontDictionaryNode *)RwFreeListAlloc(Rt2dGlobals.fontDictNodeFreeList,
                                                rwMEMHINTDUR_EVENT | rwID_2DPLUGIN);

    /* Set up the node */
    node->font=font;
    namelen = rwstrlen(fontName);
    node->name= (RwChar*)RwMalloc(namelen+1, rwMEMHINTDUR_EVENT | rwID_2DPLUGIN);
    rwstrcpy(node->name, fontName);

    /* Add it to the list */
    node->next = Rt2dGlobals.fontDictionary;
    Rt2dGlobals.fontDictionary = node;

    RWRETURN(node);
}

Rt2dFont *
_rt2dFontDictionaryAddFont( Rt2dFont *font, const RwChar * fontName )
{
    _rt2dFontDictionaryNode *node;

    RWFUNCTION(RWSTRING("_rt2dFontDictionaryAddFont"));

    RWASSERT(font);
    RWASSERT(fontName);

    node = _rt2dFontDictionaryAddNode( font, fontName );

    RWRETURN(node->font);
}

Rt2dFont *
_rt2dFontDictionaryRemoveFont( Rt2dFont *font )
{
    _rt2dFontDictionaryNode *node;
    _rt2dFontDictionaryNode *prev;

    RWFUNCTION(RWSTRING("_rt2dFontDictionaryRemoveFont"));

    RWASSERT(font);

    /* look for the font */
    node = _rt2dFontDictionaryFindFontNode(font, &prev);

    if (node)
    {
        if (prev)
        {
            prev->next = node->next;
        }
        else
        {
            /* It's the first node */
            Rt2dGlobals.fontDictionary = node->next;
        }
        RwFree(node->name);
        RwFreeListFree(Rt2dGlobals.fontDictNodeFreeList, node);
    }
    else
    {
        RWASSERTM((FALSE),
                  (RWSTRING("Tried to remove font from dictionary when font ")
                   RWSTRING("was not in dictionary")));
        RWRETURN((Rt2dFont *)NULL);
    }

    RWRETURN(font);
}

_rt2dFontDictionaryNode *
_rt2dFontDictionaryLookup(const RwChar* fontName)
{
    Rt2dFont *font= (Rt2dFont *) NULL;
    _rt2dFontDictionaryNode *node;

    RWFUNCTION(RWSTRING("_rt2dFontDictionaryLookup"));
    RWASSERT(fontName);

    /* look for the node */
    node = _rt2dFontDictionaryFindNamedNode(fontName);

    if (!node)
    {
        char imagePathStore[128];

        /* Save the current image path */
        RWASSERT(rwstrlen(RwImageGetPath())<127);
        rwstrcpy(imagePathStore, RwImageGetPath());

        /* Load the font */
        font = Rt2dFontRead(fontName);

        if(NULL != font)
        {
            node = _rt2dFontDictionaryAddNode(font, fontName);
        }
        else
        {
            /* Error! */
            node = (_rt2dFontDictionaryNode *)NULL;
        }

        /* Restore old image path */
        RwImageSetPath(imagePathStore);
    }

    RWRETURN(node);
}

/**
 * \ingroup rt2dfont
 * \ref Rt2dFontGetFont
 * is used to query if the font is in the font dictionary.
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param fontName      Name of the font.
 * \return A pointer to the \ref Rt2dFont if the font is found, NULL
 * otherwise.
 * \see Rt2dObjectStringGetFont
 * \see Rt2dObjectStringSetFont
 */

Rt2dFont *
Rt2dFontGetFont( RwChar * fontName )
{
    _rt2dFontDictionaryNode *node;

    RWAPIFUNCTION(RWSTRING("Rt2dFontGetFont"));

    RWASSERT(fontName);

    /* look for the font */
    node = _rt2dFontDictionaryFindNamedNode(fontName);

    if (node)
    {
        RWRETURN(node->font);
    }

    RWRETURN((Rt2dFont *)NULL);
}

/****************************************************************************/

/**
 * \ingroup rt2dfont
 * \ref Rt2dFontIsOutline
 * is used to query if the font is an outline or a bitmap font. This affects
 * the way the font is rendered and appearance. Outline fonts are geometric
 * fonts and can be scaled without visual artifacts. Bitmap fonts used a
 * bitmap for displaying the character set. It is quicker than outline fonts
 * but will produce visual artifacts it is not displayed at its recommended
 * height. See \ref Rt2dFontGetHeight for details.
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param font  Pointer to the font.
 * \return TRUE if the font is Outline, FALSE otherwise.
 * \see Rt2dFontFlow
 * \see Rt2dFontShow
 * \see Rt2dFontGetHeight
 */
RwBool
Rt2dFontIsOutline(Rt2dFont * font)
{
    RWAPIFUNCTION(RWSTRING("Rt2dFontIsOutline"));

    RWASSERT(font);

    RWRETURN(font->isOutline);
}

/**
 * \ingroup rt2dfont
 * \ref Rt2dFontIsUnicode
 * is used to query if the font is plain ASCII or Unicode. This affects the
 * way strings are treated internally. Plain ASCII fonts treat strings as single
 * bytes. Whereas Unicode fonts requires strings in double byte format.
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param font  Pointer to the font.
 * \return TRUE if the font is Unicode, FALSE otherwise.
 * \see Rt2dFontGetStringWidth
 * \see Rt2dFontFlow
 * \see Rt2dFontShow
 */
RwBool
Rt2dFontIsUnicode(Rt2dFont * font)
{
    RwBool      unicode;

    RWAPIFUNCTION(RWSTRING("Rt2dFontIsUnicode"));

    RWASSERT(font);

    unicode = (font->flag & rtFONTFLAG_UNICODE) ? TRUE : FALSE;

    RWRETURN(unicode);
}

/**
 * \ingroup rt2dfont
 * \ref Rt2dFontTexDictionarySet
 * is used to set the \ref RwTexDictionary to be used for storing bitmap fonts'
 * textures.
 *
 * \param texDict   A pointer to the \ref RwTexDictionary to be used.
 *
 * \return  A pointer to the \ref RwTexDictionary.
 *
 * \see Rt2dFontTexDictionaryGet
 * \see RwTexDictionaryCreate
 * \see RwTexDictionaryDestroy
 */
RwTexDictionary *
Rt2dFontTexDictionarySet( RwTexDictionary * texDict )
{
    RWAPIFUNCTION(RWSTRING("Rt2dFontTexDictionarySet"));

    Rt2dGlobals.currFontTexDictionary = texDict;

    RWRETURN(texDict);
}

/**
 * \ingroup rt2dfont
 * \ref Rt2dFontTexDictionaryGet
 * is used to query the \ref RwTexDictionary currently used for storing the
 * bitmap fonts' textures.
 *
 * \return  A pointer to the current \ref RwTexDictionary.
 *
 * \see Rt2dFontTexDictionarySet
 * \see RwTexDictionaryCreate
 * \see RwTexDictionaryDestroy
 *
 */
RwTexDictionary *
Rt2dFontTexDictionaryGet( void )
{
    RWAPIFUNCTION(RWSTRING("Rt2dFontTexDictionaryGet"));

    RWRETURN(Rt2dGlobals.currFontTexDictionary);
}
