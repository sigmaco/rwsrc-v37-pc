
/****************************************************************************
 *                                                                          *
 *  Module  :   render.c                                                    *
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
static RwV3d *pos;

/**********************************************************************/

/* declare standard names */
#define GREEDY
/* Untested!! #define G_CLAMP */
#define REALPROXY
#define EXACTPRESTEP
#define INTERP_COUNT_REALP  0
#define INTERP_COUNT_REAL   3
#define INTERP_COUNT_FIXED  0
#define PREFIX wci

#define TRIDECL
#define TRISETUP(Y)
#define TRIDY

#define SCANDECL
#define SCANSETUP(X)

#define SEGDECL
#define SEGBEGIN(W)
#define SEGPIXEL            \
        pos->x = SB.seg_v.part.r[0]; \
        pos->y = SB.seg_v.part.r[1]; \
        pos->z = SB.seg_v.part.r[2]; \
        pos++;

#define SEGEND

#include "prolog.c"

/* implement renderer specific functionality here */

/**********************************************************************/
RwV3d *
_rtLtMapLMWCRenderTriangle(RwV3d * wcpos, RwV2d * tri, RwV3d * vWC)
{
    RWFUNCTION(RWSTRING("_rtLtMapLMWCRenderTriangle"));

    pos = wcpos;

    EDGE[0].x = tri[0].x;
    EDGE[0].y = tri[0].y;
    EDGE[0].v.part.r[0] = vWC[0].x;
    EDGE[0].v.part.r[1] = vWC[0].y;
    EDGE[0].v.part.r[2] = vWC[0].z;

    EDGE[1].x = tri[1].x;
    EDGE[1].y = tri[1].y;
    EDGE[1].v.part.r[0] = vWC[1].x;
    EDGE[1].v.part.r[1] = vWC[1].y;
    EDGE[1].v.part.r[2] = vWC[1].z;

    EDGE[2].x = tri[2].x;
    EDGE[2].y = tri[2].y;
    EDGE[2].v.part.r[0] = vWC[2].x;
    EDGE[2].v.part.r[1] = vWC[2].y;
    EDGE[2].v.part.r[2] = vWC[2].z;

#include "body.c"

    RWRETURN(pos);
}

/**********************************************************************/
