/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   rtltmap.c                                                  -*
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

#include "rpcollis.h"
#include "rtbary.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rppvs.h"
#include "rpmipkl.h"

#include "rpltmap.h"
#include "rtltmap.h"

#include "ltmapvar.h"

/**
 * \defgroup rtltmapps2 PlayStation 2
 * \ingroup rtltmap
 *
 * Lightmap Generation Toolkit for RenderWare Graphics.
 *
 * PlayStation 2 Specific.
 */

/**
 * \ingroup rtltmapps2
 * \page rtltmapps2overview RtLtMap PlayStation 2 Overview
 *
 * The proprietary two-pass algorithm used for lightmap rendering on PlayStation 2
 * (much more efficient than typical four-pass solutions for full-color
 * lightmapping) has certain requirements and restrictions.
 *
 * Firstly, lightmaps must be inverted to 'darkmaps',
 * (see \ref RtLtMapLightingSessionLightMapProcess and
 * \ref RtLtMapSkyLightMapMakeDarkMap).
 *
 * Secondly, base textures must be pre-processed
 * (see \ref RtLtMapSkyLightingSessionBaseTexturesProcess).
 * This has the restriction that base textures may not contain alpha - the
 * pre-processing stage overwrites the alpha channel. Material alpha will,
 * however, give the expected result. A full 8 bits of alpha are required,
 * by the pre-processing, in base textures, so 16-bit textures will be
 * converted to a 32-bit texel format. Palettised textures are fully
 * supported and alpha-masking will also work (i.e base texture texels
 * whose alpha is zero will produces holes in the texture). Lightmap
 * rendering performed with base textures which have not been pre-processed
 * will tend to look 'dirty' or 'burned'.
 *
 * Thirdly, only tristrip rendering is currently supported by the lightmap
 * vector code pipelines. It is simple to convert trilist geometry to
 * tristrips (though this may result in a large number of wasted, degenerate
 * triangles being created), but tristrips are much more efficient to render
 * on PlayStation 2.
 */

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local defines ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local types -----------------------------------------------------------*
 *===========================================================================*/


/*===========================================================================*
 *--- Local variables -------------------------------------------------------*
 *===========================================================================*/


/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/

/* PS2-specific 'darkmap' functions: */

/****************************************************************************
 LtMapInitializeWhiteTex
 */

/****************************************************************************
 LtMapSkyMatLuminanceCB
 */
static RpMaterial *
LtMapSkyMatLuminanceCB(RpMaterial *mat, void __RWUNUSED__ *data)
{
    RwTexture *tex = RpMaterialGetTexture(mat);
    RWFUNCTION(RWSTRING("LtMapSkyMatLuminanceCB"));

    if (NULL !=  tex &&
        NULL != RwTextureGetRaster(tex) &&
        0 != RwRasterGetWidth(RwTextureGetRaster(tex)) &&
        0 != RwRasterGetHeight(RwTextureGetRaster(tex))
        )
    {
        RtLtMapSkyBaseTextureProcess(RpMaterialGetTexture(mat));
    }

    RWRETURN(mat);
}

static RpMesh *
LtMapSkyMeshLuminanceCB(RpMesh *mesh, RpMeshHeader __RWUNUSED__ *header, void *data)
{
    RWFUNCTION(RWSTRING("LtMapSkyMeshLuminanceCB"));

    LtMapSkyMatLuminanceCB(mesh->material, data);

    RWRETURN(mesh);
}

/**
 * \ingroup rtltmapps2
 * \ref RtLtMapSkyLumCalcSigmaCallBack processes a scanline, for rendering with
 * the PlayStation 2 lightmap pipeline.
 *
 * This callback function computes the 'luminance' value for each texels from
 * the RGB components and stores it in the alpha component.
 *
 * The luminance is calculated from the vector product of the RGB vector and
 * the RGB co-efficient vector.
 *
 * This is the default callback used.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  scanline  A pointer to a scanline of \ref RwRGBA data.
 * \param  width     Width of the scanline, in pixels.
 *
 * \return A pointer to the scanline on success, NULL otherwise.
 *
 * \see RtLtMapSkyBaseTextureProcess
 * \see RtLtMapSkyLightingSessionBaseTexturesProcess
 * \see RtLtMapSkyLightMapMakeDarkMap
 * \see RtLtMapSkyLumCalcMaxCallBack
 * \see RtLtMapSkySetLumCalcCallBack
 * \see RtLtMapSkyGetLumCalcCallBack
 * \see RtLtMapSkyLumResetCallBack
 */
RwRGBA *
RtLtMapSkyLumCalcSigmaCallBack( RwRGBA *scanline, RwUInt32 width )
{
    RwReal    lum[3] = {0.299f, 0.587f, 0.114f};
    RwReal    r1, r2, g1, g2, b1, b2, d;
    RwReal    lc  = 251.0f / 255.0f;
    RwReal    luminance;
    RwRGBA    *rgba;
    RwUInt32  i;

    /* This function calulates a 'luminance' value from each texel's
     * RGB values and stores this in the alpha channel.
     */
    RWAPIFUNCTION(RWSTRING("RtLtMapSkyLumCalcSigmaCallBack"));

    RWASSERT(scanline);

    rgba = scanline;

    for (i = 0; i < width; i++)
    {

        if (rgba[i].alpha > 0)
        {
            r1 = rgba[i].red;
            g1 = rgba[i].green;
            b1 = rgba[i].blue;

            r2 = r1*lum[0];
            g2 = g1*lum[1];
            b2 = b1*lum[2];

            d = r2 + g2 + b2;
            if(d > 0.0f)
            {
                luminance  = r2*r1 + g2*g1 + b2*b1;
                luminance *= lc / d;
                luminance += 4.0f;
            }
            else
            {
                luminance = 4.0f;
            }

            rgba[i].alpha = (RwUInt8)RwFastRealToUInt32(luminance);

        }
    }

    RWRETURN(scanline);
}

/**
 * \ingroup rtltmapps2
 * \ref RtLtMapSkyLumCalcMaxCallBack processes a scanline, for rendering with
 * the PlayStation 2 lightmap pipeline.
 *
 * This callback function computes the 'luminance' value for each texels from
 * the RGB components and stores it in the alpha component.
 *
 * The luminance value is computed from the maximum of the RGB component.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  scanline  A pointer to a scanline of \ref RwRGBA data.
 * \param  width     Width of the scanline, in pixels.
 *
 * \return A pointer to the scanline on success, NULL otherwise.
 *
 * \see RtLtMapSkyBaseTextureProcess
 * \see RtLtMapSkyLightingSessionBaseTexturesProcess
 * \see RtLtMapSkyLightMapMakeDarkMap
 * \see RtLtMapSkyLumCalcSigmaCallBack
 * \see RtLtMapSkySetLumCalcCallBack
 * \see RtLtMapSkyGetLumCalcCallBack
 * \see RtLtMapSkyLumResetCallBack
 */
RwRGBA *
RtLtMapSkyLumCalcMaxCallBack( RwRGBA *scanline, RwUInt32 width )
{
    RwReal    lum[3] = {0.299f, 0.587f, 0.114f};
    RwReal    r1, r2, g1, g2, b1, b2, d;
    RwReal    lc  = 251.0f / 255.0f;
    RwReal    luminance;
    RwRGBA    *rgba;
    RwUInt32  i;

    /* This function calulates a 'luminance' value from each texel's
     * RGB values and stores this in the alpha channel.
     */
    RWAPIFUNCTION(RWSTRING("RtLtMapSkyLumCalcMaxCallBack"));

    RWASSERT(scanline);

    rgba = scanline;

    for (i = 0; i < width; i++)
    {
        if (rgba[i].alpha > 0)
        {
            r1 = rgba[i].red;
            g1 = rgba[i].green;
            b1 = rgba[i].blue;

            r2 = r1*lum[0];
            g2 = g1*lum[1];
            b2 = b1*lum[2];

            d = r2 + g2 + b2;
            if (d > 0.0f)
            {
                /* Luminance correction with max RGB */
                luminance = r1;

                if(g1 > luminance)
                {
                    luminance = g1;
                }
                if(b1 > luminance)
                {
                    luminance = b1;
                }

                luminance *= lc;
                luminance += 4.0f;
            }
            else
            {
                luminance = 4.0f;
            }
            rgba[i].alpha = (RwUInt8)RwFastRealToUInt32(luminance);
        }
    }

    RWRETURN(scanline);
}

/**
 * \ingroup rtltmapps2
 * \ref RtLtMapSkyLumResetCallBack processes a scanline, for rendering with
 * the PlayStation 2 lightmap pipeline.
 *
 * At present, only the PlayStation 2 requires a luminance value to be present
 * in the alpha channel of the base texture. If this base texture is used on
 * other platform, background color leakage will occur, resulting in an
 * odd looking image.
 *
 * This callback function resets the luminance value for each texels back to
 * 255.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  scanline  A pointer to a scanline of \ref RwRGBA data.
 * \param  width     Width of the scanline, in pixels.
 *
 * \return A pointer to the scanline on success, NULL otherwise.
 *
 * \see RtLtMapSkyBaseTextureProcess
 * \see RtLtMapSkyLightingSessionBaseTexturesProcess
 * \see RtLtMapSkyLightMapMakeDarkMap
 * \see RtLtMapSkyLumCalcSigmaCallBack
 * \see RtLtMapSkyLumCalcMaxCallBack
 * \see RtLtMapSkySetLumCalcCallBack
 * \see RtLtMapSkyGetLumCalcCallBack
 */
RwRGBA *
RtLtMapSkyLumResetCallBack( RwRGBA *scanline, RwUInt32 width )
{
    RwRGBA    *rgba;
    RwUInt32  i;

    /* This function resets the alpha component back to 255.
     */
    RWAPIFUNCTION(RWSTRING("RtLtMapSkyLumResetCallBack"));

    RWASSERT(scanline);

    rgba = scanline;

    for (i = 0; i < width; i++)
    {
        if (rgba[i].alpha > 0)
        {
            rgba[i].alpha = (RwUInt8)255;
        }
    }

    RWRETURN(scanline);
}

/**
 * \ingroup rtltmapps2
 * \ref RtLtMapSkyBaseTextureProcess processes a texture, for rendering with
 * the PlayStation 2 lightmap pipeline.
 *
 * The proprietary, two-pass algorithm, used to render full-color lightmapped
 * objects on PlayStation 2, requires some pre-processing to be performed on the base
 * texture. This is in addition to the inversion from lightmap to 'darkmap'
 * (see \ref RtLtMapSkyLightMapMakeDarkMap).
 *
 * Be sure to save your base texture dictionary to disk once the pre-processing
 * has been performed. If lightmaps are rendered on base textures which have
 * not been pre-processed, the results may look normal (depending on the brightness
 * of lighting in the scene) but will usually look 'dirty' or 'burned'.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  texture  A pointer to a texture
 *
 * \return A pointer to the texture on success, NULL otherwise.
 *
 * \see RtLtMapSkyAtomicBaseTexturesProcess
 * \see RtLtMapSkyWorldSectorBaseTexturesProcess
 * \see RtLtMapSkyLightingSessionBaseTexturesProcess
 * \see RtLtMapSkyLightMapMakeDarkMap
 */
RwTexture *
RtLtMapSkyBaseTextureProcess(RwTexture *texture)
{
    RwImage  **mipArray;
    RwRaster *raster = NULL;
    RwUInt32  format, depth, mipLevel, autoMip, j;
    RwInt32   nativeMipLevels;

    /* This function calulates a 'luminance' value from each texel's
     * RGB values and stores this in the alpha channel. If this is
     * not done (or a base texture with no alpha channel is used),
     * the lightmap will produce fairly odd-looking results ('burned'
     * is a good description). */
    RWAPIFUNCTION(RWSTRING("RtLtMapSkyBaseTextureProcess"));

    raster = RwTextureGetRaster(texture);
    RWASSERT(NULL != raster);

    if( 0 == RwRasterGetWidth(raster) )
    {
        RWASSERT(0 != RwRasterGetWidth(raster));
        RWRETURN((RwTexture *) NULL);
    }

    if( 0 == RwRasterGetHeight(raster) )
    {
        RWASSERT(0 != RwRasterGetHeight(raster));
        RWRETURN((RwTexture *) NULL);
    }

    format = RwRasterGetFormat(raster);
    if (format & (rwRASTERFORMATPAL8|rwRASTERFORMATPAL4))
    {
        /* If we start with a 4/8-bit palette, that's what we end up with. */
        depth = RwRasterGetDepth(raster);
    }
    else
    {
        /* We need 8 bits of alpha (i.e. 8888), so non-palettised
         * rasters will always end up being full 32-bit. */
        depth = 32;
    }

    /* Copy all miplevels of the original raster into images */
    mipLevel = 0;
    nativeMipLevels = RwRasterGetNumLevels(raster);

    mipArray = NULL;
    if (nativeMipLevels <= 0)
    {
        /* No mip levels ? */
        RWRETURN(texture);
    }

    mipArray = (RwImage **) RwMalloc(nativeMipLevels * sizeof(RwImage **),
        (rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION));

    if (mipArray == NULL)
    {
        /* Memory failure. */
        RWRETURN((RwTexture *) NULL);
    }

    for (mipLevel = 0;mipLevel < (RwUInt32) nativeMipLevels;mipLevel++)
    {
        /* Check if there are pixel data. */
        if (RwRasterLock(raster, (RwUInt8)mipLevel, rwRASTERLOCKREAD))
        {
            mipArray[mipLevel] = RwImageCreate(
                RwRasterGetWidth(raster), RwRasterGetHeight(raster), depth);
            RwImageAllocatePixels(mipArray[mipLevel]);
            RwImageSetFromRaster(mipArray[mipLevel], raster);
            RwRasterUnlock(raster);
        }
        else
        {
            /* No pixel data for this mipmap. */
            mipArray[mipLevel] = NULL;
        }
    }

    /* Alter the raster format to have an 8-bit alpha component */
    if ((format & rwRASTERFORMATPIXELFORMATMASK) != rwRASTERFORMAT8888)
    {
        RwRaster *newRaster;
        RwReal    mipK;
        RwUInt32  mipL;

        /* If the old raster was palettised, the new one
         * should be. Likewise, preserve mipmapping */
        if ((format & rwRASTERFORMATPIXELFORMATMASK) == rwRASTERFORMAT1555)
        {
            format &= ~rwRASTERFORMAT1555;
            format |=  rwRASTERFORMAT8888;
        }
        else
        if ((format & rwRASTERFORMATPIXELFORMATMASK) == rwRASTERFORMAT888)
        {
            format &= ~rwRASTERFORMAT888;
            format |=  rwRASTERFORMAT8888;
        }

        newRaster = RwRasterCreate(RwRasterGetWidth(raster),
                                   RwRasterGetHeight(raster),
                                   depth,
                                   RwRasterGetType(raster) | format);

        mipK = RpMipmapKLTextureGetK(texture);
        mipL = RpMipmapKLTextureGetL(texture);

        RwTextureSetRaster(texture, newRaster);

        RpMipmapKLTextureSetK(texture, mipK);
        RpMipmapKLTextureSetL(texture, mipL);

        RwRasterDestroy(raster);
        raster = newRaster;
    }

    /* Process the values! */
    for (mipLevel = 0;mipLevel < (RwUInt32) nativeMipLevels;mipLevel++)
    {
        if (mipArray[mipLevel])
        {
            if (format & (rwRASTERFORMATPAL8|rwRASTERFORMATPAL4))
            {
                RwRGBA *pal = RwImageGetPalette(mipArray[mipLevel]);

                RWASSERT((4 == depth) || (8 == depth));

                (rtLtMapGlobals.skyLumCalcCallBack)(pal, ((RwUInt32)1 << depth));

            }
            else
            {
                RwUInt32 width, height;
                RwRGBA  *pix;

                width  = (RwUInt32)RwImageGetWidth( mipArray[mipLevel]);
                height = (RwUInt32)RwImageGetHeight(mipArray[mipLevel]);

                /* Process texels */
                for (j = 0; j < height; j++)
                {
                    pix = (RwRGBA *)(RwImageGetPixels(mipArray[mipLevel]) +
                                    j*RwImageGetStride(mipArray[mipLevel]));

                    (rtLtMapGlobals.skyLumCalcCallBack)(pix, width);

                }
            }
        }
    } /* for (mipLevel; 0 to numMipLevels) */

    /* Disable automipmapping, since we're editing mip levels by hand
     * (letting automipmap do it just doesn't work... alpha goes wrong) */
    autoMip = raster->cFormat & (rwRASTERFORMATAUTOMIPMAP >> 8);
    raster->cFormat &= ~(rwRASTERFORMATAUTOMIPMAP >> 8);

    /* Write the images back into the destination raster */
    for (mipLevel = 0;mipLevel < (RwUInt32) nativeMipLevels;mipLevel++)
    {
        if (mipArray[mipLevel])
        {
            RwRasterLock(raster, (RwUInt8)mipLevel, rwRASTERLOCKWRITE);
            RwRasterSetFromImage(raster, mipArray[mipLevel]);
            RwRasterUnlock(raster);
            RwImageDestroy(mipArray[mipLevel]);
        }
    }

    /* Restore the automipmapping flag */
    raster->cFormat |= autoMip;

    /* Clean up */
    if (mipArray)
        RwFree(mipArray);

    RWRETURN(texture);
}


/**
 * \ingroup rtltmapps2
 * \ref RtLtMapSkyAtomicBaseTexturesProcess processes the base textures of
 * a given atomic, for rendering with the PlayStation 2 lightmap pipeline.
 *
 * The proprietary, two-pass algorithm, used to render full-color lightmapped
 * objects on PlayStation 2, requires some pre-processing to be performed on the base
 * texture. This is in addition to the inversion from lightmap to 'darkmap'
 * (see \ref RtLtMapSkyLightMapMakeDarkMap).
 *
 * It is necessary to account for the gamma of the display device when performing
 * this pre-processing. Use \ref RwImageSetGamma for this purpose.
 *
 * Be sure to save your base texture dictionary to disk once the pre-processing
 * has been performed. If lightmaps are rendered on base textures which have
 * not been pre-processed, the results may look normal (depending on the brightness
 * of lighting in the scene) but will usually look 'dirty' or 'burned'.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  atomic  A pointer to an atomic
 *
 * \return A pointer to the atomic on success, NULL otherwise.
 *
 * \see RtLtMapSkyWorldSectorBaseTexturesProcess
 * \see RtLtMapSkyBaseTextureProcess
 * \see RtLtMapSkyLightingSessionBaseTexturesProcess
 * \see RtLtMapSkyLightMapMakeDarkMap
 * \see RwImageGetGamma
 * \see RwImageSetGamma
 */
RpAtomic *
RtLtMapSkyAtomicBaseTexturesProcess(RpAtomic *atomic)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapSkyAtomicBaseTexturesProcess"));

    /* Process base textures */
    RpGeometryForAllMaterials(RpAtomicGetGeometry(atomic), LtMapSkyMatLuminanceCB, NULL);

    RWRETURN(atomic);
}

/**
 * \ingroup rtltmapps2
 * \ref RtLtMapSkyWorldSectorBaseTexturesProcess processes the base textures of
 * a given world sector, for rendering with the PlayStation 2 lightmap pipeline.
 *
 * The proprietary, two-pass algorithm, used to render full-color lightmapped
 * objects on PlayStation 2, requires some pre-processing to be performed on the base
 * texture. This is in addition to the inversion from lightmap to 'darkmap'
 * (see \ref RtLtMapSkyLightMapMakeDarkMap and
 *  \ref RtLtMapLightingSessionLightMapProcess).
 *
 * It is necessary to account for the gamma of the display device when performing
 * this pre-processing. Use \ref RwImageSetGamma for this purpose.
 *
 * Be sure to save your base texture dictionary to disk once the pre-processing
 * has been performed. If lightmaps are rendered on base textures which have
 * not been pre-processed, the results may look normal (depending on the brightness
 * of lighting in the scene) but will usually look 'dirty' or 'burned'.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  sector  A pointer to a world sector
 *
 * \return A pointer to the sector on success, NULL otherwise.
 *
 * \see RtLtMapSkyAtomicBaseTexturesProcess
 * \see RtLtMapSkyBaseTextureProcess
 * \see RtLtMapSkyLightingSessionBaseTexturesProcess
 * \ref RtLtMapLightingSessionLightMapProcess
 * \see RtLtMapSkyLightMapMakeDarkMap
 * \see RwImageGetGamma
 * \see RwImageSetGamma
 */
RpWorldSector *
RtLtMapSkyWorldSectorBaseTexturesProcess(RpWorldSector *sector)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapSkyWorldSectorBaseTexturesProcess"));

    /* Process base textures */
    RpWorldSectorForAllMeshes(sector, LtMapSkyMeshLuminanceCB, NULL);

    RWRETURN(sector);
}



/**
 * \ingroup rtltmapps2
 * \ref RtLtMapSkyLightingSessionBaseTexturesProcess processes the base textures of
 * the objects specified by an \ref RtLtMapLightingSession, for rendering
 * with the PlayStation 2 lightmap pipeline.
 *
 * This function traverses the objects specified by the received
 * \ref RtLtMapLightingSession structure. Only atomics flagged as rpATOMICRENDER
 * will be processed. Note that the camera member of this structure is ignored.
 *
 * The proprietary, two-pass algorithm, used to render full-color lightmapped
 * objects on PlayStation 2, requires some pre-processing to be performed on the base
 * texture. This is in addition to the inversion from lightmap to 'darkmap'
 * (see \ref RtLtMapSkyLightMapMakeDarkMap and
 * \ref RtLtMapLightingSessionLightMapProcess).
 *
 * It is necessary to account for the gamma of the display device when performing
 * this pre-processing. Use \ref RwImageSetGamma for this purpose.
 *
 * Be sure to save your base texture dictionary to disk once the pre-processing
 * has been performed. If lightmaps are rendered on base textures which have
 * not been pre-processed, the results may look normal (depending on the brightness
 * of lighting in the scene) but will usually look 'dirty' or 'burned'.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  session  A pointer to an \ref RtLtMapLightingSession
 *
 * \return A pointer to the session on success, NULL otherwise.
 *
 * \see RtLtMapSkyAtomicBaseTexturesProcess
 * \see RtLtMapSkyWorldSectorBaseTexturesProcess
 * \ref RtLtMapLightingSessionLightMapProcess
 * \see RtLtMapSkyBaseTextureProcess
 * \see RtLtMapSkyLightMapMakeDarkMap
 * \see RwImageGetGamma
 * \see RwImageSetGamma
 * \see RtLtMapLightingSessionLightMapProcess
 */
RtLtMapLightingSession *
RtLtMapSkyLightingSessionBaseTexturesProcess(RtLtMapLightingSession *session)
{
    LtMapSessionInfo sessionInfo;
    RwUInt32 i;

    RWAPIFUNCTION(RWSTRING("RtLtMapSkyLightingSessionBaseTexturesProcess"));
    RWASSERT(NULL != session);

    /* Get local lists of sectors/atomics from the session
     * (culled w.r.t the incoming object lists, if present, not the camera) */
    _rtLtMapLightingSessionInfoCreate(&sessionInfo, session, FALSE);

    /* Process base textures */
    for (i = 0;i < (RwUInt32)sessionInfo.numSectors;i++)
    {
        RpWorldSector *sector = *(RpWorldSector **)rwSListGetEntry(sessionInfo.localSectors, i);

        if (RtLtMapWorldSectorGetFlags(sector) & rtLTMAPOBJECTLIGHTMAP)
        {
            /* Only process the base texture for lightmapped objects */
            RtLtMapSkyWorldSectorBaseTexturesProcess(sector);
        }
    }

    for (i = 0;i < (RwUInt32)sessionInfo.numAtomics;i++)
    {
        RpAtomic *atomic = *(RpAtomic **)rwSListGetEntry(sessionInfo.localAtomics, i);

        if (RpAtomicGetFlags(atomic) & rpATOMICRENDER)
        {
            if (RtLtMapAtomicGetFlags(atomic) & rtLTMAPOBJECTLIGHTMAP)
            {
                /* Only process the base texture for lightmapped objects */
                RtLtMapSkyAtomicBaseTexturesProcess(atomic);
            }
        }
    }

    _rtLtMapLightingSessionInfoDestroy(&sessionInfo);

    RwDebugSendMessage(rwDEBUGMESSAGE,
        "RtLtMapSkyLightingSessionBaseTexturesProcess",
        "The base textures of lightmapped atomics and sectors have been processed (due to PS2 rendering requirements). Ensure that these are saved in their processed form, or rendering artifacts will be observed when this world is re-loaded.");

    RWRETURN(session);
}


/**
 * \ingroup rtltmapps2
 * \ref RtLtMapSkySetLumCalcCallBack sets the callback used to compute the
 * luminance value from the RGB components.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  cBack    A pointer to callback function to be used.
 *
 * \return TRUE.
 *
 * \see RtLtMapSkyBaseTextureProcess
 * \see RtLtMapSkyLightingSessionBaseTexturesProcess
 * \see RtLtMapSkyLightMapMakeDarkMap
 * \see RtLtMapSkyLumCalcMaxCallBack
 * \see RtLtMapSkyLumCalcSigmaCallBack
 * \see RtLtMapSkyGetLumCalcCallBack
 */
RwBool
RtLtMapSkySetLumCalcCallBack( RtLtMapSkyLumCalcCallBack cBack)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapSkySetLumCalcCallBack"));

    rtLtMapGlobals.skyLumCalcCallBack = cBack;

    RWRETURN(TRUE);
}

/**
 * \ingroup rtltmapps2
 * \ref RtLtMapSkyGetLumCalcCallBack query the callback used to compute the
 * luminance value from the RGB components.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \return A pointer to \ref RtLtMapSkyLumCalcCallBack function.
 *
 * \see RtLtMapSkyBaseTextureProcess
 * \see RtLtMapSkyLightingSessionBaseTexturesProcess
 * \see RtLtMapSkyLightMapMakeDarkMap
 * \see RtLtMapSkyLumCalcMaxCallBack
 * \see RtLtMapSkyLumCalcSigmaCallBack
 * \see RtLtMapSkySetLumCalcCallBack
 */
RtLtMapSkyLumCalcCallBack
RtLtMapSkyGetLumCalcCallBack( void )
{
    RWAPIFUNCTION(RWSTRING("RtLtMapSkyGetLumCalcCallBack"));

    RWRETURN(rtLtMapGlobals.skyLumCalcCallBack);
}

/**
 * \ingroup rtltmapps2
 * \ref RtLtMapSkyLightMapMakeDarkMap is used to turn a lightmap texture
 * into a 'darkmap', for rendering with the PlayStation 2 lightmap pipeline.
 *
 * The proprietary, two-pass algorithm, used to render full-color lightmapped
 * objects on PlayStation 2, requires the inversion from lightmaps to 'darkmaps'.
 * This is in addition to the pre-processing to be performed on base
 * textures (see \ref RtLtMapSkyLightingSessionBaseTexturesProcess).
 *
 * This function can be used with \ref RtLtMapLightingSessionLightMapProcess to
 * turn all lightmap textures into dark in the current lighting session.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  lightMap  A pointer to a lightmap texture to be converted into
 *                   a PlayStation 2-specific 'darkmap'
 *
 * \return A pointer to the lightmap on success, NULL otherwise.
 *
 * \see RtLtMapSkyLightingSessionBaseTexturesProcess
 * \see RtLtMapLightingSessionLightMapProcess
 * \see RtLtMapGetLightMapProcessCallBack
 * \see RtLtMapSetLightMapProcessCallBack
 * \see RpLtMapAtomicGetLightMap
 * \see RpLtMapWorldSectorGetLightMap
 */
RwTexture *
RtLtMapSkyLightMapMakeDarkMap(RwTexture *lightMap)
{
    RwImage  *image  = NULL;
    RwRaster *raster = NULL;
    RwRGBA   *pal    = NULL;
    RwUInt32  depth;
    RwUInt32  format;

    RWAPIFUNCTION(RWSTRING("RtLtMapSkyLightMapMakeDarkMap"));
    RWASSERT(NULL != lightMap);

    /* This function clears the alpha channel of lightmaps
     * to zero and converts {R, G, B} to {1-R, 1-G, 1-B} */

    raster = RwTextureGetRaster(lightMap);
    format = RwRasterGetFormat(raster);
    if (format & (rwRASTERFORMATPAL8|rwRASTERFORMATPAL4))
    {
        /* If we start with a 4/8-bit palette, that's what we end up with. */
        depth = RwRasterGetDepth(raster);
    }
    else
    {
        /* We need 8 bits of alpha (i.e. 8888), so non-palettised
         * rasters will always end up being full 32-bit. */
        depth = 32;
    }

    image  = RwImageCreate(RwRasterGetWidth(raster),
                           RwRasterGetHeight(raster),
                           depth);
    RwImageAllocatePixels(image);
    RwImageSetFromRaster(image, raster);
    pal    = RwImageGetPalette(image);

    if(pal != NULL)
    {
        RwUInt32 i = 1 << RwImageGetDepth(image);
        while (i--)
        {
            pal->alpha = 0;
            pal->red   = 255 - pal->red;
            pal->green = 255 - pal->green;
            pal->blue  = 255 - pal->blue;
            pal++;
        }
    }
    else
    {
        RwUInt8 *pixels;
        RwRGBA  *texel;
        RwUInt32 width, height, stride, i, j;

        pixels = RwImageGetPixels(image);
        RWASSERT(NULL != pixels);
        width  = RwImageGetWidth(image);
        height = RwImageGetHeight(image);
        stride = RwImageGetStride(image);

        for (j = 0; j < height; j++)
        {
            texel = (RwRGBA *)(pixels + j*stride);
            for (i = 0;i < width;i++)
            {
                texel->alpha = 0;
                texel->red   = 255 - texel->red;
                texel->green = 255 - texel->green;
                texel->blue  = 255 - texel->blue;
                texel++;
            }
        }
    }

    RwRasterSetFromImage(raster, image);
    RwImageDestroy(image);

    RWRETURN(lightMap);
}

