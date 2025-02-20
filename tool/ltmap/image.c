/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   image.c                                                    -*
 *-                                                                         -*
 *-  Purpose :   RtLtMap toolkit                                            -*
 *-                                                                         -*
 *===========================================================================*/

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/

/* Needed for memset */
#include "string.h"

#include "rwcore.h"
#include "rpworld.h"
#include "rprandom.h"

#include "rpcollis.h"
#include "rtbary.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rppvs.h"
/* We need to use the non-INCGEN-d version of rpltmap.h */
#include "../../plugin/ltmap/rpltmap.h"
#include "rtltmap.h"

#include "polypack.h"
#include "image.h"
#include "ltmapvar.h"

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local defines ---------------------------------------------------------*
 *===========================================================================*/


/* This highlights the border of lit regions in the lightmap */
#define SHOWDILATEPIXELSx

#define PIXELUSED(P)    (rpLTMAPUNUSEDALPHA != P.alpha)

/* If this is defined, then after each LtMapIllumination, any touched lightmap
 * images will be dumped out to file.
 */
#define LIGHTMAPDUMPIMAGEx

/*===========================================================================*
 *--- Local types -----------------------------------------------------------*
 *===========================================================================*/


/*===========================================================================*
 *--- Local variables -------------------------------------------------------*
 *===========================================================================*/


/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/

/****************************************************************************
 LtMapLightMapImageWrite
 */

#ifdef LIGHTMAPDUMPIMAGE

RwBool
LtMapLightMapImageWrite(RwImage *image, rpLtMapSampleMap *sampleMap, RwChar *imageName)
{
    RWFUNCTION(RWSTRING("LtMapLightMapImageWrite"));

    if (NULL != image)
    {
        RwImage             *oimage, *aimage;
        RwUInt32            iwidth, iheight;
        RwUInt8             *ipixels, *opixels, *apixels;
        RwRGBA              *ipix, *opix, *apix;
        RwUInt32            istride, ix, iy;
        RwChar              name[32];
        RwUInt8             *samplePixels;

        iwidth = RwImageGetWidth(image);
        iheight = RwImageGetHeight(image);

        oimage = RwImageCreate(iwidth, iheight, 32);
        RwImageAllocatePixels(oimage);
        opixels = RwImageGetPixels(oimage);

        if (NULL != sampleMap)
        {
            aimage = RwImageCreate(iwidth, iheight, 32);
            RwImageAllocatePixels(aimage);
            apixels = RwImageGetPixels(aimage);

            samplePixels = sampleMap->samplePixels;
        }
        else
        {
            aimage = NULL;
            apixels = NULL;
            samplePixels = NULL;
        }

        /* Grab the image info for the texture's lightmap image. */

        ipixels = RwImageGetPixels(image);
        istride = RwImageGetStride(image);

        ipix = NULL;
        opix = NULL;
        apix = NULL;

        /* Copy over the texture's to the texture's lightmap image. */
        for (iy = 0; iy < iheight; iy++)
        {
            ipix = (RwRGBA *) &(ipixels[iy * istride]);
            opix = (RwRGBA *) &(opixels[iy * istride]);

            if (NULL != apixels)
                apix = (RwRGBA *) &(apixels[iy * istride]);

            for (ix = 0; ix < iwidth; ix++)
            {
                opix->red   = ipix->red;
                opix->green = ipix->green;
                opix->blue  = ipix->blue;
                opix->alpha = 255;
                opix++;

                if (NULL != samplePixels)
                {
                    if (RPLTMAPSAMPLEMAPGET(samplePixels, ix, iy, iwidth))
                    {
                        apix->red   = 255;
                        apix->green = 255;
                        apix->blue  = 255;
                    }
                    else
                    {
                        apix->red   = 0;
                        apix->green = 0;
                        apix->blue  = 0;
                    }
                    apix->alpha = 255;
                    apix++;
                }

                ipix++;
            }
        }


        if (oimage)
        {
            rwsprintf(name, "%s_img.png", imageName);
            RwImageWrite(oimage, (RwChar *) name);

            RwImageDestroy(oimage);
        }

        if (aimage)
        {
            rwsprintf(name, "%s_vis.png", imageName);
            RwImageWrite(aimage, (RwChar *) name);

            RwImageDestroy(aimage);
        }
    }

    RWRETURN(TRUE);
}

#endif /* LIGHTMAPDUMPIMAGE */

/****************************************************************************
 LtMapLightMapImageSetFromTexture
 */
RwBool
_rtLtMapLightMapImageSetFromTexture(RwTexture *lightMap,
                                 RwImage *image, rpLtMapSampleMap *sampleMap)
{
    RWFUNCTION(RWSTRING("_rtLtMapLightMapImageSetFromTexture"));

    if (NULL != lightMap)
    {
        RwRaster    *raster;
        RwImage     *timage, *iimage;
        RwUInt32    scale;
        RwUInt32    twidth, theight;
        RwUInt32    iwidth, iheight;
        RwReal      fval;
        RwUInt8     *samplePixels;

        samplePixels = sampleMap->samplePixels;

        raster = RwTextureGetRaster(lightMap);

        twidth  = RwRasterGetWidth(raster);
        theight = RwRasterGetHeight(raster);

        timage = NULL;
        iimage = image;

        RWASSERT(iimage);

        iwidth = RwImageGetWidth(iimage);
        iheight = RwImageGetHeight(iimage);

        RWASSERT(iwidth >= twidth);
        RWASSERT(iheight >= theight);

        fval = ((RwReal) iwidth / (RwReal) twidth);
        scale = RwFastRealToUInt32(fval);

        {
            RwUInt8     *tpixels, *ipixels;
            RwRGBA      *tpix, *ipix;
            RwUInt32    tstride, tx, ty;
            RwUInt32    istride, ix, iy;

            /* Need to do some scaling.
             * Grab the image info for the texture's raster and so a tmp image
             * for the it.
             */
            timage = RwImageCreate(twidth, theight, 32);
            RWASSERT(NULL != timage);
            RwImageAllocatePixels(timage);

            RwImageSetFromRaster(timage, raster);

            tpixels = RwImageGetPixels(timage);
            tstride = RwImageGetStride(timage);

            /* Grab the image info for the texture's lightmap image. */

            ipixels = RwImageGetPixels(iimage);
            istride = RwImageGetStride(iimage);

            /* Copy over the texture's to the texture's lightmap image. */
            for (ty = 0; ty < theight; ty++)
            {
                for (iy = 0; iy < scale; iy++)
                {
                    tpix = (RwRGBA *) &(tpixels[ty * tstride]);
                    ipix = (RwRGBA *) &(ipixels[(ty * scale + iy) * istride]);

                    for (tx = 0; tx < twidth; tx++)
                    {
                        for (ix = 0; ix < scale; ix++)
                        {
                            ipix->red   = tpix->red;
                            ipix->green = tpix->green;
                            ipix->blue  = tpix->blue;

                            if (RPLTMAPSAMPLEMAPGET(samplePixels, tx, ty, twidth))
                            {
                                ipix->alpha = rpLTMAPUSEDALPHA;
                            }
                            else
                            {
                                ipix->alpha = rpLTMAPUNUSEDALPHA;
                            }

                            ipix++;
                        }

                        tpix++;
                    }
                }
            }
        }

        if (timage)
            RwImageDestroy(timage);

    }

    RWRETURN(TRUE);
}

/****************************************************************************
 LtMapTextureSetFromLightMapImage
 */
RwBool
_rtLtMapTextureSetFromLightMapImage(RwTexture *lightMap,
                                 RwImage *image, rpLtMapSampleMap *sampleMap)
{
    RWFUNCTION(RWSTRING("_rtLtMapTextureSetFromLightMapImage"));

    if (NULL != lightMap)
    {
        RwRaster    *raster;
        RwImage     *timage, *iimage;
        RwUInt32    scale;
        RwUInt32    twidth, theight;
        RwUInt32    iwidth, iheight;
        RwReal      fval;
        RwUInt8     *samplePixels;

#ifdef LIGHTMAPDUMPIMAGE
        {
            static RwUInt32     dumpImgCount = 0;
            RwChar              name[32];

            rwsprintf(name, "./img%04d", dumpImgCount);

            LtMapLightMapImageWrite(image, NULL, name);

            dumpImgCount++;
        }
#endif /* LIGHTMAPDUMPIMAGE */

        samplePixels = sampleMap->samplePixels;

        raster = RwTextureGetRaster(lightMap);

        twidth  = RwRasterGetWidth(raster);
        theight = RwRasterGetHeight(raster);

        timage = NULL;
        iimage = image;

        RWASSERT(iimage);

        iwidth = RwImageGetWidth(iimage);
        iheight = RwImageGetHeight(iimage);

        RWASSERT(iwidth >= twidth);
        RWASSERT(iheight >= theight);

        fval = ((RwReal) iwidth / (RwReal) twidth);
        scale = RwFastRealToUInt32(fval);

        {
            RwUInt8     *tpixels, *ipixels;
            RwRGBA      *tpix, *ipix;
            RwUInt32    tstride, tx, ty;
            RwUInt32    istride, ix, iy;
            RwUInt32    *red1, *green1, *blue1, *count;
            RwUInt32    *red2, *green2, *blue2;
            RwReal      icount;

            /* Need to do some scaling.
             * Grab the image info for the texture's raster and so a tmp image
             * for the it.
             */

            timage = RwImageCreate(twidth, theight, 32);
            RWASSERT(NULL != timage);
            RwImageAllocatePixels(timage);

            tpixels = RwImageGetPixels(timage);
            tstride = RwImageGetStride(timage);

            /* Grab the image info for the texture's lightmap image. */

            ipixels = RwImageGetPixels(iimage);
            istride = RwImageGetStride(iimage);

            /* Create a few tmp arrays as a single block */
            red1 = (RwUInt32 *) RwMalloc(iwidth * 7 * sizeof(RwUInt32),
                                 rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION); /* RGB + RGB + count */

            RWASSERT(red1);

            memset(red1, 0, (iwidth * 7 * sizeof(RwUInt32)));

            /* Set up the arrays pointers. */
            green1 = (RwUInt32 *) (((RwChar *) red1)   + (iwidth * sizeof(RwUInt32)));
            blue1 =  (RwUInt32 *) (((RwChar *) green1) + (iwidth * sizeof(RwUInt32)));
            count =  (RwUInt32 *) (((RwChar *) blue1)  + (iwidth * sizeof(RwUInt32)));

            red2 =   (RwUInt32 *) (((RwChar *) count)  + (iwidth * sizeof(RwUInt32)));
            green2 = (RwUInt32 *) (((RwChar *) red2)   + (iwidth * sizeof(RwUInt32)));
            blue2 =  (RwUInt32 *) (((RwChar *) green2) + (iwidth * sizeof(RwUInt32)));

            /* Copy over the the texture's to the texture's lightmap image. */
            for (ty = 0; ty < theight; ty++)
            {
                /* Collect the samples into tmp area. */

                for (iy = 0; iy < scale; iy++)
                {
                    ipix = (RwRGBA *) &(ipixels[(ty * scale + iy) * istride]);

                    for (tx = 0; tx < twidth; tx++)
                    {
                        for (ix = 0; ix < scale; ix++)
                        {
                            red2[tx]   += (RwUInt32) ipix->red;
                            green2[tx] += (RwUInt32) ipix->green;
                            blue2[tx]  += (RwUInt32) ipix->blue;

                            /* Is this a valid sample ? */
                            if (ipix->alpha != rpLTMAPUNUSEDALPHA)
                            {
                                red1[tx]   += (RwUInt32) ipix->red;
                                green1[tx] += (RwUInt32) ipix->green;
                                blue1[tx]  += (RwUInt32) ipix->blue;

                                count[tx]++;
                            }

                            ipix++;

                        }
                    }
                }

                /* Now transfer the samples to the texture. */
                tpix = (RwRGBA *) &(tpixels[ty * tstride]);

                for (tx = 0; tx < twidth; tx++)
                {
                    if (count[tx] > 0)
                    {
                        icount = (RwReal) 1.0 / (RwReal) count[tx];

                        fval = ((RwReal) red1[tx] * icount);
                        tpix->red = (RwUInt8) RwFastRealToUInt32(fval);

                        fval = ((RwReal) green1[tx] * icount);
                        tpix->green = (RwUInt8) RwFastRealToUInt32(fval);

                        fval = ((RwReal) blue1[tx] * icount);
                        tpix->blue = (RwUInt8) RwFastRealToUInt32(fval);

                        tpix->alpha = 255;

                        RPLTMAPSAMPLEMAPSET(samplePixels, tx, ty, twidth);
                    }
                    else
                    {
                        icount = (RwReal) 1.0 / (RwReal) (scale * scale);

                        fval = ((RwReal) red2[tx] * icount);
                        tpix->red = (RwUInt8) RwFastRealToUInt32(fval);

                        fval = ((RwReal) green2[tx] * icount);
                        tpix->green = (RwUInt8) RwFastRealToUInt32(fval);

                        fval = ((RwReal) blue2[tx] * icount);
                        tpix->blue = (RwUInt8) RwFastRealToUInt32(fval);

                        tpix->alpha = 255;

                        RPLTMAPSAMPLEMAPUNSET(samplePixels, tx, ty, twidth);
                    }

                    red1[tx] = 0;
                    green1[tx] = 0;
                    blue1[tx] = 0;
                    count[tx] = 0;

                    red2[tx] = 0;
                    green2[tx] = 0;
                    blue2[tx] = 0;

                    tpix++;
                }
            }

            if (red1)
                RwFree(red1);

            RwRasterSetFromImage(raster, timage);
        }

        if (timage)
            RwImageDestroy(timage);
    }

    RWRETURN(TRUE);
}

/****************************************************************************
 LtMapDilate
 */
RwTexture *
_rtLtMapDilate(RwTexture *lightMap, rpLtMapSampleMap *sampleMap)
{
    RwImage  *image, *dilate32;
    RwRaster *ras;
    RwUInt8  *srcPixels, *dstPixels;
    RwUInt32  width, height, stride, x, y;
    RwUInt32  unUsed = 0;
    RwUInt8  *samplePixels;

    RWFUNCTION(RWSTRING("_rtLtMapDilate"));

    /* Dilate texels in order to fill the texels of sliver polygons,
     * which cannot be trusted during lighting since they likely have
     * dodgy surface normals. For each uninitialised texel, we merely
     * copy the average colour of its initialised neighbours. Also,
     * given their high sample-to-area ratio, lighting slivers is a
     * very inefficient thing to do.
     *
     * Dilate also fills in texels just outside polysets such that
     * bilerping doesn't produce black/checkerboard halos. */

    RWASSERT(NULL != lightMap);
    RWASSERT(NULL != sampleMap);

    samplePixels = sampleMap->samplePixels;

    ras = RwTextureGetRaster(lightMap);

    width  = RwRasterGetWidth(ras);
    height = RwRasterGetHeight(ras);

    image = RwImageCreate(width, height, 32);
    RWASSERT(NULL != image);
    RwImageAllocatePixels(image);
    stride = RwImageGetStride(image);

    /* Dilate from the original image into a new one */
    srcPixels = RwImageGetPixels(image);
    RWASSERT(NULL != srcPixels);

    RwImageSetFromRaster(image, ras);

    dilate32 = RwImageCreate(width, height, 32);
    RWASSERT(NULL != dilate32);

    RwImageAllocatePixels(dilate32);
    dstPixels = RwImageGetPixels(dilate32);
    RWASSERT(NULL != dstPixels);
    RWASSERT(stride == (RwUInt32)RwImageGetStride(dilate32));

    for (y = 0; y < height; y++)
    {
        RwBool top = !y;
        RwBool bottom = (y == (height - 1));

        RwRGBA *src  = (RwRGBA *)&(srcPixels[stride*y]);
        RwRGBA *prev = (RwRGBA *)&(srcPixels[stride*(y - 1)]);
        RwRGBA *next = (RwRGBA *)&(srcPixels[stride*(y + 1)]);

        RwRGBA *dst  = (RwRGBA *)&(dstPixels[stride*y]);

        for (x = 0; x < width; x++)
        {
            RwBool left = !x;
            RwBool right = (x == (width - 1));

            RwUInt32 ired = 0, igrn = 0, iblu = 0, icount = 0;

            dst[x] = src[x];

            /* Deal only w/ unlit texels */

            if (!(RPLTMAPSAMPLEMAPGET(samplePixels, x, y, width)))
            {
                unUsed++;
                /* Sum up lit neighbours */
                if (!right && (RPLTMAPSAMPLEMAPGET(samplePixels, (x + 1), y, width)))
                {
                    ired += src[x + 1].red;
                    igrn += src[x + 1].green;
                    iblu += src[x + 1].blue;
                    icount++;
                }
                if (!left && (RPLTMAPSAMPLEMAPGET(samplePixels, (x - 1), y, width)))
                {
                    ired += src[x - 1].red;
                    igrn += src[x - 1].green;
                    iblu += src[x - 1].blue;
                    icount++;
                }
                if (!top && (RPLTMAPSAMPLEMAPGET(samplePixels, x, (y - 1), width)))
                {
                    ired += prev[x].red;
                    igrn += prev[x].green;
                    iblu += prev[x].blue;
                    icount++;
                }
                if (!bottom && (RPLTMAPSAMPLEMAPGET(samplePixels, x, (y + 1), width)))
                {
                    ired += next[x].red;
                    igrn += next[x].green;
                    iblu += next[x].blue;
                    icount++;
                }
                if (!top && !left && (RPLTMAPSAMPLEMAPGET(samplePixels, (x - 1), (y - 1), width)))
                {
                    ired += prev[x - 1].red;
                    igrn += prev[x - 1].green;
                    iblu += prev[x - 1].blue;
                    icount++;
                }
                if (!top && !right && (RPLTMAPSAMPLEMAPGET(samplePixels, (x + 1), (y - 1), width)))
                {
                    ired += prev[x + 1].red;
                    igrn += prev[x + 1].green;
                    iblu += prev[x + 1].blue;
                    icount++;
                }
                if (!bottom && !left && (RPLTMAPSAMPLEMAPGET(samplePixels, (x - 1), (y + 1), width)))
                {
                    ired += next[x - 1].red;
                    igrn += next[x - 1].green;
                    iblu += next[x - 1].blue;
                    icount++;
                }
                if (!bottom && !right && (RPLTMAPSAMPLEMAPGET(samplePixels, (x + 1), (y + 1), width)))
                {
                    ired += next[x + 1].red;
                    igrn += next[x + 1].green;
                    iblu += next[x + 1].blue;
                    icount++;
                }
                /* Write back the average of any adjacent, lit texels */
                if (icount)
                {
/* TODO[5]: USE A LUT TO REMOVE THESE THREE DIVIDES */
                    dst[x].red   = (RwUInt8)(ired / icount);
                    dst[x].green = (RwUInt8)(igrn / icount);
                    dst[x].blue  = (RwUInt8)(iblu / icount);
#if (defined(SHOWDILATEPIXELS))
                    /* For debugging - highlights the border of lit samples: */
                    dst[x].red   = 255;
                    dst[x].green = 128;
                    dst[x].blue  = 0;
#endif /* (defined(SHOWDILATEPIXELS)) */
                    /* These pixels are in the uninitialized state */
                    dst[x].alpha = 255;
                }
            }
        }
    }


    /* Update the displayed/saved lightmap raster
     * (if we're doing incremental lighting, we want
     * to be able to render the results immediately) */
    RwRasterSetFromImage(ras, dilate32);

/* TODO[5]: PROBABLY SHOULD CREATE/DESTROY DILATE32 FOR EVERY RASTER
 *          INCREASING LIGHTMAP SIZE TO 512X512 *REALLY* SLOWS IT DOWN FOR SOME REASON */
    RwImageDestroy(dilate32);
    RwImageDestroy(image);

    RWRETURN(lightMap);
}

/****************************************************************************
 ImageRectClear
 */
#if (defined(SHOWLIGHTMAPUSAGE))
void
ImageRectClear(RwImage *image, RwRect *rect, RwRGBA colour)
{
    RWFUNCTION( RWSTRING( "ImageRectClear" ) );

    if (rect->w*rect->h > 0)
    {
        RwUInt8 *pixels;
        RwInt32  x, y, stride;

        stride = RwImageGetStride(image);
        pixels = RwImageGetPixels(image);
        for (y = 0;y < rect->h;y++)
        {
            RwRGBA *col = (RwRGBA *)&(pixels[(y + rect->y)*stride]);
            for (x = 0;x < rect->w;x++)
            {
                col[x + rect->x] = colour;
            }
        }
    }

    RWRETURNVOID();
}

/****************************************************************************
 RasterRectClear
 */

void
RasterRectClear(RwRaster *raster, RwRect *rect, RwRGBA colour)
{
    RWFUNCTION( RWSTRING( "RasterRectClear" ) );

    if (rect->w*rect->h > 0)
    {
        RwImage *image;

        /* Subraster locking isn't implemented on OGL and
         * raster twiddling breaks it all on PS2 (I tried untwiddling
         * before lock and retwiddling afterwards, but no dice...) */

#if (defined(OPENGL_DRVMODEL_H) || defined(SKY2_DRVMODEL_H) || defined(NULLSKY_DRVMODEL_H))

        /* This version is *MUCH* slower when there are lots of small polysets */
        image = RwImageCreate(RwRasterGetWidth(raster),
                              RwRasterGetHeight(raster), 32);
        RwImageAllocatePixels(image);
        RwImageSetFromRaster(image, raster);
        ImageRectClear(image, rect, colour);
        RwRasterSetFromImage(raster, image);

#else  /* (defined(OPENGL_DRVMODEL_H) || defined(SKY2_DRVMODEL_H) || defined(NULLSKY_DRVMODEL_H)) */
        {
            RwRaster *subRaster;
            RwUInt8  *pixels;
            RwInt32   x, y, stride;

            image = RwImageCreate(rect->w, rect->h, 32);
            RwImageAllocatePixels(image);

            subRaster = RwRasterCreate(0, 0, RwRasterGetDepth(raster),
                rwRASTERDONTALLOCATE | RwRasterGetFormat(raster) | RwRasterGetType(raster));

            subRaster = RwRasterSubRaster(subRaster, raster, rect);
            stride = RwImageGetStride(image);
            pixels = RwImageGetPixels(image);
            for (y = 0;y < rect->h;y++)
            {
                RwRGBA *col = (RwRGBA *)&(pixels[y*stride]);
                for (x = 0;x < rect->w;x++)
                {
                   *col = colour;
                    col++;
                }
            }
            RwRasterSetFromImage(subRaster, image);
            RwRasterDestroy(subRaster);
        }
#endif /* (defined(OPENGL_DRVMODEL_H) || defined(SKY2_DRVMODEL_H) || defined(NULLSKY_DRVMODEL_H)) */
        RwImageDestroy(image);
    }

    RWRETURNVOID();
}
#endif /* (defined(SHOWLIGHTMAPUSAGE)) */

/****************************************************************************
 LtMapLightMapClear
 */
RwBool
_rtLtMapLightMapClear(RwTexture *lightMap, RwRGBA *colour, rpLtMapSampleMap *sampleMap)
{
    RWFUNCTION(RWSTRING("_rtLtMapLightMapClear"));

    if (NULL != lightMap)
    {
        _rpLtMapClearLightMap(lightMap, colour);

        if (NULL != sampleMap)
        {
            RwUInt32    w, size;

            /* Assume it is a square. */
            w = RwRasterGetWidth(RwTextureGetRaster(lightMap));

            size = RPLTMAPSAMPLEMAPSIZE(w, w);

            memset(sampleMap->samplePixels, 0, size);
        }
    }


    RWRETURN(TRUE);
}

/*===========================================================================*
 *--- Toolkit  API Functions ------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rtltmap
 * \ref RtLtMapAtomicImagePurge destroys the temporary images, used during
 * lightmap illumination, for a specified atomic.
 *
 * Each lightmap has a sample map associated with it, used to enable
 * incremental lighting (see \ref RtLtMapIlluminate). Once lighting is finished,
 * this function may be used to destroy this sample map.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  atomic  A pointer to an atomic
 *
 * \return A pointer to the atomic on success, otherwise NULL
 *
 * \see RtLtMapWorldSectorImagePurge
 * \see RtLtMapImagesPurge
 * \see RtLtMapIlluminate
 * \see RtLtMapLightMapsClear
 */
RpAtomic *
RtLtMapAtomicImagePurge(RpAtomic *atomic)
{
    LtMapObjectData     *objectData;

    RWAPIFUNCTION(RWSTRING("RtLtMapAtomicImagePurge"));
    RWASSERT(NULL != atomic);

    objectData = RPLTMAPATOMICGETDATA(atomic);

    if (NULL != objectData->sampleMap)
    {
        _rpLtMapSampleMapDestroy(objectData->sampleMap);
    }

    objectData->sampleMap = NULL;

    RWRETURN(atomic);
}

/**
 * \ingroup rtltmap
 * \ref RtLtMapWorldSectorImagePurge destroys the temporary images, used during
 * lightmap illumination, for a specified world sector.
 *
 * Each lightmap has a sample map associated with it, used to enable
 * incremental lighting (see \ref RtLtMapIlluminate). Once lighting is finished,
 * this function may be used to destroy this sample map.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  sector  A pointer to a world sector
 *
 * \return A pointer to the sector on success, otherwise NULL
 *
 * \see RtLtMapAtomicImagePurge
 * \see RtLtMapImagesPurge
 * \see RtLtMapIlluminate
 * \see RtLtMapLightMapsClear
 */
RpWorldSector *
RtLtMapWorldSectorImagePurge(RpWorldSector *sector)
{
    LtMapObjectData     *objectData;

    RWAPIFUNCTION(RWSTRING("RtLtMapWorldSectorImagePurge"));
    RWASSERT(NULL != sector);

    objectData = RPLTMAPWORLDSECTORGETDATA(sector);

    if (NULL != objectData->sampleMap)
    {
        _rpLtMapSampleMapDestroy(objectData->sampleMap);
    }

    objectData->sampleMap = NULL;

    RWRETURN(sector);
}

/**
 * \ingroup rtltmap
 * \ref RtLtMapImagesPurge destroys the temporary images, used during lightmap
 * illumination, for the objects specified by an \ref RtLtMapLightingSession.
 *
 * This function traverses the objects specified by the received
 * \ref RtLtMapLightingSession structure. Only atomics flagged as rpATOMICRENDER
 * will be used. Note that the camera member of this structure is ignored.
 *
 * Each lightmap has an \ref RwImage associated with it, used to enable
 * incremental lighting (see \ref RtLtMapIlluminate). Once lighting is finished,
 * this function may be used to destroy these images.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  session  A pointer to an \ref RtLtMapLightingSession
 *
 * \return A pointer to the session on success, otherwise NULL
 *
 * \see RtLtMapAtomicImagePurge
 * \see RtLtMapWorldSectorImagePurge
 * \see RtLtMapIlluminate
 * \see RtLtMapLightMapsClear
 */
RtLtMapLightingSession *
RtLtMapImagesPurge(RtLtMapLightingSession *session)
{
    LtMapSessionInfo sessionInfo;
    RwUInt32 i;

    RWAPIFUNCTION(RWSTRING("RtLtMapImagesPurge"));
    RWASSERT(NULL != session);

    /* Get local lists of sectors/atomics from the session
     * (culled w.r.t the incoming object lists, if present, not the camera) */
    _rtLtMapLightingSessionInfoCreate(&sessionInfo, session, FALSE);

    for (i = 0;i < (RwUInt32)sessionInfo.numSectors;i++)
    {
        RpWorldSector *sector = *(RpWorldSector **)rwSListGetEntry(sessionInfo.localSectors, i);
        /* We may try purging shared lightmaps multiple times, but no harm done */
        RtLtMapWorldSectorImagePurge(sector);
    }

    for (i = 0;i < (RwUInt32)sessionInfo.numAtomics;i++)
    {
        RpAtomic *atomic = *(RpAtomic **)rwSListGetEntry(sessionInfo.localAtomics, i);

        /* Use the rpATOMICRENDER flag consistently w.r.t everything
         * that uses the RtLtMapLightingSession */
        if (RpAtomicGetFlags(atomic) & rpATOMICRENDER)
        {
            RtLtMapAtomicImagePurge(atomic);
        }
    }

    _rtLtMapLightingSessionInfoDestroy(&sessionInfo);

    RWRETURN(session);
}


/**
 * \ingroup rtltmap
 * \ref RtLtMapAtomicLightMapClear clears the lightmap of a specified
 * atomic.
 *
 * When a lightmap is cleared, the texel values are set to either a black and
 * white checkerboard pattern (useful to clearly see where lightmap texels lie
 * on polygons), if the color parameter is NULL, or to the value of the
 * \ref RwRGBA value pointed to by the color parameter. Additionally, the
 * \ref RwImage associated with the lightmap (used for incremental lighting,
 * see \ref RtLtMapIlluminate) is also cleared.
 *
 * Note that, as several atomics and world sectors may share a single lightmap,
 * calling this function for one object may affect other objects.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  atomic  A pointer to a world atomic
 * \param  color   An optional pointer to a color to use in clearing the lightmap
 *
 * \return A pointer to the atomic on success, otherwise NULL
 *
 * \see RtLtMapWorldSectorLightMapClear
 * \see RtLtMapLightMapsClear
 * \see RtLtMapIlluminate
 * \see RtLtMapImagesPurge
 */
RpAtomic *
RtLtMapAtomicLightMapClear(RpAtomic *atomic, RwRGBA *color)
{
    LtMapObjectData *objectData;

    RWAPIFUNCTION(RWSTRING("RtLtMapAtomicLightMapClear"));

    RWASSERT(NULL != atomic);
    objectData = RPLTMAPATOMICGETDATA(atomic);

    _rtLtMapLightMapClear(objectData->lightMap, color, objectData->sampleMap);

    /* Record the color */
    if (NULL != color)
    {
        objectData->clearCol = TRUE;

        objectData->color.red   = color->red;
        objectData->color.green = color->green;
        objectData->color.blue  = color->blue;
        objectData->color.alpha = color->alpha;
    }
    else
    {
        objectData->clearCol = FALSE;
    }

    RWRETURN(atomic);
}

/**
 * \ingroup rtltmap
 * \ref RtLtMapWorldSectorLightMapClear clears the lightmap of a
 * specified world sector.
 *
 * When a lightmap is cleared, the texel values are set to either a black and
 * white checkerboard pattern (useful to clearly see where lightmap texels lie
 * on polygons), if the color parameter is NULL, or to the value of the
 * \ref RwRGBA value pointed to by the color parameter. Additionally, the
 * \ref RwImage associated with the lightmap (used for incremental lighting,
 * see \ref RtLtMapIlluminate) is also cleared.
 *
 * Note that, as several atomics and world sectors may share a single lightmap,
 * calling this function for one object may affect other objects.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  sector  A pointer to a world sector
 * \param  color   An optional pointer to a color to use in clearing the lightmap
 *
 * \return A pointer to the sector on success, otherwise NULL
 *
 * \see RtLtMapAtomicLightMapClear
 * \see RtLtMapLightMapsClear
 * \see RtLtMapIlluminate
 * \see RtLtMapImagesPurge
 */
RpWorldSector *
RtLtMapWorldSectorLightMapClear(RpWorldSector *sector, RwRGBA *color)
{
    LtMapObjectData *objectData;

    RWAPIFUNCTION(RWSTRING("RtLtMapWorldSectorLightMapClear"));

    RWASSERT(NULL != sector);
    objectData = RPLTMAPWORLDSECTORGETDATA(sector);

    _rtLtMapLightMapClear(objectData->lightMap, color, objectData->sampleMap);

    /* Record the color */
    if (NULL != color)
    {
        objectData->clearCol = TRUE;

        objectData->color.red   = color->red;
        objectData->color.green = color->green;
        objectData->color.blue  = color->blue;
        objectData->color.alpha = color->alpha;
    }
    else
    {
        objectData->clearCol = FALSE;
    }

    RWRETURN(sector);
}

/**
 * \ingroup rtltmap
 * \ref RtLtMapLightMapsClear clears the lightmaps of the objects in
 * an \ref RtLtMapLightingSession.
 *
 * This function traverses the objects specified by the received
 * \ref RtLtMapLightingSession structure. Only atomics flagged as rpATOMICRENDER
 * will be used. Note that the camera member of this structure is used.
 *
 * When a lightmap is cleared, the texel values are set to either a black and
 * white checkerboard pattern (useful to clearly see where lightmap texels lie
 * on polygons), if the color parameter is NULL, or to the value of the
 * \ref RwRGBA value pointed to by the color parameter. Additionally, the
 * \ref RwImage associated with the lightmap (used for incremental lighting,
 * see \ref RtLtMapIlluminate) is also cleared.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  session  A pointer to an \ref RtLtMapLightingSession
 * \param  color    An optional pointer to a color to use in clearing the lightmap
 *
 * \return A pointer to the session on success, otherwise NULL
 *
 * \see RtLtMapAtomicLightMapClear
 * \see RtLtMapWorldSectorLightMapClear
 * \see RtLtMapIlluminate
 * \see RtLtMapImagesPurge
 */
RtLtMapLightingSession *
RtLtMapLightMapsClear(RtLtMapLightingSession *session, RwRGBA *color)
{
    LtMapSessionInfo sessionInfo;
    LtMapObjectData *objectData;

    RwSList *lightMapList;
    RwUInt32 i, j;

    RWAPIFUNCTION(RWSTRING("RtLtMapLightMapsClear"));
    RWASSERT(NULL != session);

    /* Get local lists of sectors/atomics from the session
     * (culled w.r.t the incoming camera and object lists, if present) */
    _rtLtMapLightingSessionInfoCreate(&sessionInfo, session, TRUE);

    /* Keep track of processed lightmaps so we don't process any twice */
    lightMapList = rwSListCreate(sizeof(RwTexture *),
                                 rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION);
    RWASSERT(NULL != lightMapList);

    for (i = 0;i < (RwUInt32)sessionInfo.numSectors;i++)
    {
        RpWorldSector *sector = *(RpWorldSector **)rwSListGetEntry(sessionInfo.localSectors, i);
        RwTexture *lightMap = RpLtMapWorldSectorGetLightMap(sector);

        objectData = RPLTMAPWORLDSECTORGETDATA(sector);

        for (j = 0;j < (RwUInt32)rwSListGetNumEntries(lightMapList);j++)
        {
            if (lightMap == *(RwTexture **)rwSListGetEntry(lightMapList, j))
            {
                lightMap = (RwTexture *)NULL;
                break;
            }
        }

        if (NULL != lightMap)
        {
            /* Avoid darkmapizing (inverting) shared lightmaps more than once */
           *(RwTexture **)rwSListGetNewEntry(lightMapList,
               rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION) = lightMap;
            _rtLtMapLightMapClear(lightMap, color, objectData->sampleMap);
        }

        /* Record the color */
        if (NULL != color)
        {
            objectData->clearCol = TRUE;

            objectData->color.red   = color->red;
            objectData->color.green = color->green;
            objectData->color.blue  = color->blue;
            objectData->color.alpha = color->alpha;
        }
        else
        {
            objectData->clearCol = FALSE;
        }
    }

    for (i = 0;i < (RwUInt32)sessionInfo.numAtomics;i++)
    {
        RpAtomic *atomic = *(RpAtomic **)rwSListGetEntry(sessionInfo.localAtomics, i);
        RwTexture *lightMap = RpLtMapAtomicGetLightMap(atomic);

        objectData = RPLTMAPATOMICGETDATA(atomic);

        /* Use the rpATOMICRENDER flag consistently w.r.t everything
         * that uses the RtLtMapLightingSession */
        if (RpAtomicGetFlags(atomic) & rpATOMICRENDER)
        {
            for (j = 0;j < (RwUInt32)rwSListGetNumEntries(lightMapList);j++)
            {
                if (lightMap == *(RwTexture **)rwSListGetEntry(lightMapList, j))
                {
                    lightMap = (RwTexture *)NULL;
                    break;
                }
            }

            if (NULL != lightMap)
            {
                /* Avoid darkmapizing (inverting) shared lightmaps more than once */
                *(RwTexture **)rwSListGetNewEntry(lightMapList,
                      rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION) = lightMap;
                _rtLtMapLightMapClear(lightMap, color, objectData->sampleMap);
            }
        }

        /* Record the color */
        if (NULL != color)
        {
            objectData->clearCol = TRUE;

            objectData->color.red   = color->red;
            objectData->color.green = color->green;
            objectData->color.blue  = color->blue;
            objectData->color.alpha = color->alpha;
        }
        else
        {
            objectData->clearCol = FALSE;
        }
    }

    rwSListDestroy(lightMapList);

    _rtLtMapLightingSessionInfoDestroy(&sessionInfo);

    RWRETURN(session);
}

/**
 * \ingroup rtltmap
 * \ref RtLtMapTexDictionaryCreate creates a texture dictionary containing the
 * lightmaps associated with the objects in an \ref RtLtMapLightingSession.
 *
 * This function traverses the objects specified by the received
 * \ref RtLtMapLightingSession structure. Only atomics flagged as rpATOMICRENDER
 * will be used. Note that the camera member of this structure is ignored (it is
 * assumed that texture dictionaries are 'view-independent', as it were).
 *
 * If no lightmaps are found, this function will return NULL.
 *
 * It is important to note that the data stored in the texture dictionary is in
 * a platform-dependent form. An object's lightmap may be converted into a
 * (platform-independent) RwImage by the user (see \ref RpLtMapAtomicGetLightMap).
 *
 * Note that it is the user's responsibility to add the lightmaps back to the
 * current texture dictionary (if appropriate) and to destroy the returned
 * texture dictionary. Failing to do so correctly may result in a crash or
 * leaks on app shutdown.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  session  A pointer to an \ref RtLtMapLightingSession
 *
 * \return A pointer to a texture dictionary on success, NULL if the session's
 * objects contain no lightmaps or otherwise
 *
 * \see RpLtMapAtomicGetLightMap
 * \see RpLtMapWorldSectorGetLightMap
 */
RwTexDictionary *
RtLtMapTexDictionaryCreate(RtLtMapLightingSession *session)
{
    LtMapSessionInfo sessionInfo;
    RwTexDictionary *texDict;
    RwBool           gotSome = FALSE;
    RwUInt32         i;

    RWAPIFUNCTION(RWSTRING("RtLtMapTexDictionaryCreate"));
    RWASSERT(NULL != session);

/* TODO[3][ADK]: PERHAPS WE SHOULD EXPOSE RPLTMAPIMAGEFROMTEXTURE() AS API SO THAT USERS
 *         CAN SAVE THE *IMAGE* VERSIONS OF LIGHTMAPS, GIVEN THEY'RE PLAT-INDEPENDENT
 *         ALREADY AND 8888 (RASTERS MIGHT BE 565 AND PALETTISED -> POOR QUALITY
 *         PROPAGATES TO ALL PLATFORMS). */

    /* Get local lists of sectors/atomics from the session
     * (culled w.r.t the incoming object lists, if present) */
    _rtLtMapLightingSessionInfoCreate(&sessionInfo, session, FALSE);

    texDict = RwTexDictionaryCreate();
    RWASSERT(NULL != texDict);

    for (i = 0;i < (RwUInt32)sessionInfo.numSectors;i++)
    {
        RpWorldSector *sector = *(RpWorldSector **)rwSListGetEntry(sessionInfo.localSectors, i);
        RwTexture *lightMap = RpLtMapWorldSectorGetLightMap(sector);

        if (NULL != lightMap)
        {
            gotSome = TRUE;
            /* We may try adding shared lightmaps multiple times, but no harm done */
            RwTexDictionaryAddTexture(texDict, lightMap);
        }
    }

    for (i = 0;i < (RwUInt32)sessionInfo.numAtomics;i++)
    {
        RpAtomic *atomic = *(RpAtomic **)rwSListGetEntry(sessionInfo.localAtomics, i);
        RwTexture *lightMap = RpLtMapAtomicGetLightMap(atomic);

        if (RpAtomicGetFlags(atomic) & rpATOMICRENDER)
        {
            if (NULL != lightMap)
            {
                gotSome = TRUE;
                RwTexDictionaryAddTexture(texDict, lightMap);
            }
        }
    }

    _rtLtMapLightingSessionInfoDestroy(&sessionInfo);

    if (FALSE == gotSome)
    {
        RwTexDictionaryDestroy(texDict);
        RWRETURN((RwTexDictionary *)NULL);
    }

    RWRETURN(texDict);
}

/**
 * \ingroup rtltmap
 * \ref RtLtMapSetLightMapProcessCallBack sets the callback used to process
 * the lightmap after lighting.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  cBack    A pointer to /ref  RtLtMapLightMapProcessCallBackcallback
 *                  function to be used.
 *
 * \return TRUE.
 *
 * \see RtLtMapSkyLightMapMakeDarkMap
 * \see RtLtMapGetLightMapProcessCallBack
 * \see RtLtMapLightingSessionLightMapProcess
 * \see RtLtMapSkyLightingSessionBaseTexturesProcess
 */
RwBool
RtLtMapSetLightMapProcessCallBack(RtLtMapLightMapProcessCallBack cBack)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapSetLightMapProcessCallBack"));

    rtLtMapGlobals.ltMapProcessCallBack = cBack;

    RWRETURN(TRUE);
}

/**
 * \ingroup rtltmap
 * \ref RtLtMapGetLightMapProcessCallBack query the callback used to process
 * the lightmap after lighting.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \return A pointer to \ref RtLtMapLightMapProcessCallBack function.
 *
 * \see RtLtMapSkyLightMapMakeDarkMap
 * \see RtLtMapSetLightMapProcessCallBack
 * \see RtLtMapLightingSessionLightMapProcess
 * \see RtLtMapSkyLightingSessionBaseTexturesProcess
 */
RtLtMapLightMapProcessCallBack
RtLtMapGetLightMapProcessCallBack( void )
{
    RWAPIFUNCTION(RWSTRING("RtLtMapGetLightMapProcessCallBack"));

    RWRETURN(rtLtMapGlobals.ltMapProcessCallBack);
}

/**
 * \ingroup rtltmap
 * \ref RtLtMapLightingSessionLightMapProcess is the function to be called
 * to perform any operation on the lightmap data.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \return A pointer to \ref RtLtMapLightingSession on success, NULL otherwise.
 *
 * \see RtLtMapSkyLightMapMakeDarkMap
 * \see RtLtMapGetLightMapProcessCallBack
 * \see RtLtMapSetLightMapProcessCallBack
 * \see RtLtMapSkyLightingSessionBaseTexturesProcess
 */
RtLtMapLightingSession *
RtLtMapLightingSessionLightMapProcess( RtLtMapLightingSession *session )
{
    LtMapSessionInfo sessionInfo;
    RwSList *lightMapList;
    RwUInt32 i, j;

    RWAPIFUNCTION(RWSTRING("RtLtMapLightingSessionLightMapProcess"));
    RWASSERT(NULL != session);

    /* Get local lists of sectors/atomics from the session
     * (culled w.r.t the incoming camera and object lists, if present) */
    _rtLtMapLightingSessionInfoCreate(&sessionInfo, session, TRUE);

    /* Keep track of processed lightmaps so we don't process any twice */
    lightMapList = rwSListCreate(sizeof(RwTexture *),
        rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION);
    RWASSERT(NULL != lightMapList);

    for (i = 0;i < (RwUInt32)sessionInfo.numSectors;i++)
    {
        RpWorldSector *sector = *(RpWorldSector **)rwSListGetEntry(sessionInfo.localSectors, i);
        RwTexture *lightMap = RpLtMapWorldSectorGetLightMap(sector);

        for (j = 0;j < (RwUInt32)rwSListGetNumEntries(lightMapList);j++)
        {
            if (lightMap == *(RwTexture **)rwSListGetEntry(lightMapList, j))
            {
                lightMap = (RwTexture *)NULL;
                break;
            }
        }

        if (NULL != lightMap)
        {
            /* Avoid processing shared lightmaps more than once */
           *(RwTexture **)rwSListGetNewEntry(lightMapList,
                 rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION) = lightMap;

            /* Apply the process callback */
            if ((rtLtMapGlobals.ltMapProcessCallBack)(lightMap) != lightMap)
            {
                /* Something is wrong, return. */
                RWRETURN(NULL);
            }
        }
    }

    for (i = 0;i < (RwUInt32)sessionInfo.numAtomics;i++)
    {
        RpAtomic *atomic = *(RpAtomic **)rwSListGetEntry(sessionInfo.localAtomics, i);
        RwTexture *lightMap = RpLtMapAtomicGetLightMap(atomic);

        /* Use the rpATOMICRENDER flag consistently w.r.t everything
         * that uses the RtLtMapLightingSession */
        if (RpAtomicGetFlags(atomic) & rpATOMICRENDER)
        {
            for (j = 0;j < (RwUInt32)rwSListGetNumEntries(lightMapList);j++)
            {
                if (lightMap == *(RwTexture **)rwSListGetEntry(lightMapList, j))
                {
                    lightMap = (RwTexture *)NULL;
                    break;
                }
            }

            if (NULL != lightMap)
            {
                /* Avoid processing shared lightmaps more than once */
               *(RwTexture **)rwSListGetNewEntry(lightMapList,
                     rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION) = lightMap;

                /* Apply the process callback */
                if ((rtLtMapGlobals.ltMapProcessCallBack)(lightMap) != lightMap)
                {
                    /* Something is wrong, return. */
                    RWRETURN(NULL);
                }
            }
        }
    }

    rwSListDestroy(lightMapList);

    _rtLtMapLightingSessionInfoDestroy(&sessionInfo);

    RWRETURN(session);
}

