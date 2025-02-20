#ifndef NO_LM_CREATE

/****************************************************************************
 *                                                                         *
 *  Module  :   lmren.c                                                    *
 *                                                                          *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "rpplugin.h"
#include <rwcore.h>
#include <rpdbgerr.h>

#include "rtltmap.h"
#include "fastflit.h"

/****************************************************************************
 Local variables
 */
static RwRGBA *LMcolors;
static RwImage *ipLMImage;

/**********************************************************************/
#define GREEDY
/* For now, I've implemented G_CLAMP in LtMapObjectForAllSamples() with vector maths */
/*#define G_CLAMP*/
#define REALPROXY
#define EXACTPRESTEP
#define INTERP_COUNT_REALP  0
#define INTERP_COUNT_REAL   0
#define INTERP_COUNT_FIXED  0
#define PREFIX rgbi

#define TRIDECL             RwRGBA *scanline;
#define TRISETUP(Y)         SB.scanline = (RwRGBA *)(RwImageGetPixels(ipLMImage) + Y * RwImageGetStride(ipLMImage));
#define TRIDY               SB.scanline += RwImageGetStride(ipLMImage)>>2;
#define SCANDECL            static RwRGBA *scan;
#define SCANSETUP(X)        scan = (SB.scanline + X);
#define SEGDECL
#define SEGBEGIN(W)
#define SEGEND

/* This (part of the sample-off-poly solution) now only:
 *  o overwrites unused texels w/ greedy or non-greedy ones
 *  o overwrites greedy ones w/ non-greedy ones
 *  [i.e it doesn't overwrite non-greedy ones w/ greedy ones]
 *  NOTE: we use ">=" instead of ">" so that incremental lighting
 *  works :) - i.e a re-lit greedy sample overwrites the old one */
#define SEGPIXEL                                               \
        if (LMcolors->alpha >= scan->alpha) *scan = *LMcolors; \
        LMcolors++;                                            \
        scan++;

/* Debugging - this version does additive write-back (handy for
 * visualising greedy-sample overlap between neighbour triangles): */
/*#define SEGPIXEL                             \
        if (LMcolors->alpha >= scan->alpha)  \
        {                                    \
            scan->red   += LMcolors->red;    \
            scan->green += LMcolors->green;  \
            scan->blue  += LMcolors->blue;   \
            scan->alpha  = LMcolors->alpha;  \
        }                                    \
        LMcolors++;                          \
        scan++; */


#include "prolog.c"

/* implement renderer specific functionality here */

/**********************************************************************/
void
_rtLtMapLMRGBRenderTriangle(RwImage *ipImage, RwV2d *tri, RwRGBA *colors)
{
    RWFUNCTION(RWSTRING("_rtLtMapLMRGBRenderTriangle"));

    ipLMImage = ipImage;
    LMcolors = colors;

    EDGE[0].x = tri[0].x;
    EDGE[0].y = tri[0].y;

    EDGE[1].x = tri[1].x;
    EDGE[1].y = tri[1].y;

    EDGE[2].x = tri[2].x;
    EDGE[2].y = tri[2].y;

#include "body.c"


    RWRETURNVOID();
}

#endif
