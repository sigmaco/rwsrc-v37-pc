/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   ltmapvar.h                                                 -*
 *-                                                                         -*
 *-  Purpose :   RtLtMap toolkit                                            -*
 *-                                                                         -*
 *===========================================================================*/

#ifndef LTMAPVAR_H
#define LTMAPVAR_H

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
 *--- Local defines ---------------------------------------------------------*
 *===========================================================================*/

/* If this is defined, RtLtMapLightMapsCreate() will colour
 * in lightmaps to show usage and will print out statistics.
 * Yellow denotes used space, green free and red wasted. Blue
 * is uninitialised space and should not be seen. */
#define SHOWLIGHTMAPUSAGEx


/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/

typedef struct LtMapCachedTriangle LtMapCachedTriangle ;

struct LtMapCachedTriangle
{
    RpCollisionTriangle triangle;
    RwUInt32            hit;
    RwBool              sky;
};

typedef struct rtLtMapGlobalVars rtLtMapGlobalVars;

struct rtLtMapGlobalVars
{
    /* Polygon packing & UV allocation  */
    RwV3d       rightVector[6];
    RwV3d       upVector[6];
    RwV3d       sortVector;
    RwSList     *ltMapSlots;

    /* Visibility determination */
    RwUInt32                        skyFlags;
    RtLtMapIlluminateVisCallBack    visCallBack;
    RwReal                          visCallBackCollisionScalar;

    /* Sampling */
    RpWorld                 *lightWorld;
    RtLtMapAreaLightGroup   *areaLights;
    RwUInt32                lightObjectFlag;

    /* Area lights */
    RwReal                  areaLightDensity;

    /* Post process */
    RtLtMapLightMapProcessCallBack ltMapProcessCallBack;
    RtLtMapSkyLumCalcCallBack      skyLumCalcCallBack;

    LtMapCachedTriangle cachedTriangle;

#if (defined(SHOWLIGHTMAPUSAGE))
    RwImage *gLightMapUsageImage;
#endif /* (defined(SHOWLIGHTMAPUSAGE)) */

};

/*===========================================================================*
 *--- Global Variables ------------------------------------------------------*
 *===========================================================================*/

extern rtLtMapGlobalVars rtLtMapGlobals;


#endif /* LTMAPVAR_H */

