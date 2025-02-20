/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   image.c                                                    -*
 *-                                                                         -*
 *-  Purpose :   RtLtMap toolkit                                            -*
 *-                                                                         -*
 *===========================================================================*/

#ifndef IMAGE_H
#define IMAGE_H

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

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/


/*===========================================================================*
 *--- Local defines ---------------------------------------------------------*
 *===========================================================================*/


/*===========================================================================*
 *--- Toolkit SPI Functions -------------------------------------------------*
 *===========================================================================*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern RwBool
_rtLtMapLightMapImageSetFromTexture(RwTexture *lightMap,
                                 RwImage *image, rpLtMapSampleMap *sampleMap);

extern RwBool
_rtLtMapTextureSetFromLightMapImage(RwTexture *lightMap,
                                 RwImage *image, rpLtMapSampleMap *sampleMap);

extern RwTexture *
_rtLtMapDilate(RwTexture *lightMap, rpLtMapSampleMap *sampleMap);

extern RwBool
_rtLtMapLightMapClear(RwTexture *lightMap,
                     RwRGBA *colour, rpLtMapSampleMap *sampleMap);

#if (defined(SHOWLIGHTMAPUSAGE))

extern void
ImageRectClear(RwImage *image, RwRect *rect, RwRGBA colour);

extern void
RasterRectClear(RwRaster *raster, RwRect *rect, RwRGBA colour);

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* IMAGE_H */

