
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

#include "rasterng.h"

/**********************************************************************/

/****************************************************************************
 External variables
 */

/****************************************************************************
 Local variables
 */
static RwImage *dstImage;
static RwImage *srcImage;
static RwUInt32 srcImgWidth;
static RwUInt32 srcImgHeight;

/****************************************************************************
 Debug Variables
 */


/****************************************************************************
 rasterizer setup
 */


/* declare standard names */
#define REALPROXY
#define EXACTPRESTEP
#define INTERP_COUNT_REALP  0
#define INTERP_COUNT_REAL   2
#define INTERP_COUNT_FIXED  0
#define PREFIX cnvNG

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
    RwUInt32 offset = ((RwUInt32)RwInt32FromRealMacro(SB.seg_v.part.r[1]))*\
                      RwImageGetStride(srcImage) +\
                      ((RwUInt32)RwInt32FromRealMacro(SB.seg_v.part.r[0])*4);\
    RwRGBA *src = (RwRGBA*)( (RwUInt32)RwImageGetPixels(srcImage) + offset );\
    *scan = *src;\
    scan++;\
}


#include "prolog.c"

/* implement renderer specific functionality here */
#define TESTEDGE(edg1,edg2)\
    (( (edg1.x == edg2.x) && (edg1.y == edg2.y) ) ||\
    ( (edg1.v.part.r[0] == edg2.v.part.r[0] ) &&\
      (edg1.v.part.r[1] == edg2.v.part.r[1] ) ))

/**********************************************************************/
void
_rtltmapUVRemaperNonGreedy(RwImage *dstImg,
                            RwTexCoords *dstUV,
                            RwImage *srcImg,
                            RwTexCoords *srcUV,
                            RwReal rejectionSize)
{
    RwInt32 i;

    RwUInt32 dstImgWidth = RwImageGetWidth(dstImg)-1;
    RwUInt32 dstImgHeight = RwImageGetHeight(dstImg)-1;

    RWFUNCTION(RWSTRING("_rtltmapUVRemaperNonGreedy"));

    srcImgWidth = RwImageGetWidth(srcImg)-1;
    srcImgHeight = RwImageGetHeight(srcImg)-1;

    dstImage = dstImg;
    srcImage = srcImg;

    for(i=0;i<3;i++)
    {
        EDGE[i].x = dstImgWidth * dstUV[i].u;
        EDGE[i].y = dstImgHeight * dstUV[i].v;
        EDGE[i].v.part.r[0] = (srcUV[i].u * (RwReal)srcImgWidth);
        EDGE[i].v.part.r[1] = (srcUV[i].v * (RwReal)srcImgHeight);
    }

    if( TESTEDGE(EDGE[0],EDGE[1]) ||
        TESTEDGE(EDGE[1],EDGE[2]) ||
        TESTEDGE(EDGE[2],EDGE[0]) )
    {
        RWRETURNVOID();
    }

    /* We don't do small size */
    if( ((RwRealMax3(EDGE[0].x,EDGE[1].x,EDGE[2].x) -
          RwRealMin3(EDGE[0].x,EDGE[1].x,EDGE[2].x)) < rejectionSize) ||
        ((RwRealMax3(EDGE[0].y,EDGE[1].y,EDGE[2].y) -
          RwRealMin3(EDGE[0].y,EDGE[1].y,EDGE[2].y)) < rejectionSize) )
    {
        RWRETURNVOID();
    }

#include "body.c"

    RWRETURNVOID();
}




