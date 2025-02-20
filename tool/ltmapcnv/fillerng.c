
/****************************************************************************
 *                                                                          *
 *  Module  :   ztest.c                                                    *
 *                                                                          *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
 */
/*#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>*/

#include "rpplugin.h"
#include <rpdbgerr.h>
#include <rwcore.h>
#include "fastflit.h"

#include "fillerng.h"

/**********************************************************************/

/****************************************************************************
 External variables
 */

/****************************************************************************
 Local variables
 */
static RwImage *dstImage;
static RwRGBA fillColor;

/****************************************************************************
 Debug Variables
 */

/* declare standard names */
#define GREEDYx

#define REALPROXY
#define EXACTPRESTEP
#define INTERP_COUNT_REALP  0
#define INTERP_COUNT_REAL   0
#define INTERP_COUNT_FIXED  0
#define PREFIX filnG

#define TRIDECL         RwRGBA *scanline;
#define TRISETUP(Y)     SB.scanline = (RwRGBA *)(RwImageGetPixels(dstImage)\
                        + Y * RwImageGetStride(dstImage));
#define TRIDY           SB.scanline += RwImageGetStride(dstImage)>>2;
#define SCANDECL        static RwRGBA *scan;
#define SCANSETUP(X)    scan = (SB.scanline + X);
#define SEGDECL
#define SEGBEGIN(W)

#define SEGPIXEL \
{\
    *scan = fillColor;\
    scan++;\
}

#include "prolog.c"

/* implement renderer specific functionality here */
#define TESTEDGE(edg1,edg2)\
    ( (edg1.x == edg2.x) && (edg1.y == edg2.y) )
            
/**********************************************************************/
void
_rtltmapFillerNonGreedy(RwImage *dstImg, 
                            RwTexCoords *dstUV, 
                            RwRGBA  *color,
                            RwReal rejectionSize)
{
    RwInt32 i;

    RwUInt32 dstImgWidth = RwImageGetWidth(dstImg)-1;
    RwUInt32 dstImgHeight = RwImageGetHeight(dstImg)-1;

    RWFUNCTION(RWSTRING("_rtltmapUVRemaperGreedy"));
    RWASSERT(NULL != dstImg);
    RWASSERT(NULL != dstUV);

    dstImage = dstImg;
    fillColor = *color;
            
    for(i=0;i<3;i++)
    {
        EDGE[i].x = dstImgWidth * dstUV[i].u;
        EDGE[i].y = dstImgHeight * dstUV[i].v;
    }

    if( TESTEDGE(EDGE[0],EDGE[1]) ||
        TESTEDGE(EDGE[1],EDGE[2]) ||
        TESTEDGE(EDGE[2],EDGE[0]) )
    {
        return;
    }

    /* We don't do small size */
    if( ((RwRealMax3(EDGE[0].x,EDGE[1].x,EDGE[2].x) -
          RwRealMin3(EDGE[0].x,EDGE[1].x,EDGE[2].x)) < rejectionSize) ||
        ((RwRealMax3(EDGE[0].y,EDGE[1].y,EDGE[2].y) -
          RwRealMin3(EDGE[0].y,EDGE[1].y,EDGE[2].y)) < rejectionSize) )
    {
        return;
    }

#include "body.c"

    return;
}




