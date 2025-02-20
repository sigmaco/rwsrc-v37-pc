
/****************************************************************************
 *                                                                          *
 *  Module  :   ztest.c                                                    *
 *                                                                          *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
 */
#include "windows.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <rwcore.h>
#include <rpworld.h>
#include "fastflit.h"


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
#define ULTRADEBUG

#ifdef ULTRADEBUG
RwUInt32        badPolyCount = 0;
RwUInt32        polyCount = 0;

RwV2d           dstXY;
static RwRGBA   defaultBadColor = {0xFF,00,00,00};
RwChar          msg[1024];

RwV2d           dstCoordinates[3];
RwV2d           srcCoordinates[3];

RwReal          dstEdgeLength[3];
RwReal          srcEdgeLength[3];

RwReal          dstArea;
RwReal          srcArea;

RwBool          badTriangle;

#define dumpUVS
#define dumpTriAreasx
#define dumpPolys

#define dumpGLOBALSTATS
#endif

/****************************************************************************
 rasterizer setup
 */


/* declare standard names */
#define GREEDYx

RwUInt32        topReadAddr;

#define REALPROXY
#define EXACTPRESTEP
#define INTERP_COUNT_REALP  0
#define INTERP_COUNT_REAL   2
#define INTERP_COUNT_FIXED  0
#define PREFIX zt

#ifdef ULTRADEBUG

#define TRIDECL             RwRGBA *scanline;
#define TRISETUP(Y)         SB.scanline = (RwRGBA *)(RwImageGetPixels(dstImage) + Y * RwImageGetStride(dstImage));dstXY.y = (RwReal)Y;
#define TRIDY               SB.scanline += RwImageGetStride(dstImage)>>2;dstXY.y++;
#define SCANDECL            static RwRGBA *scan;
#define SCANSETUP(X)        scan = (SB.scanline + X);dstXY.x=(RwReal)X;
#define SEGDECL
#define SEGBEGIN(W)


#define SEGPIXEL            \
            SegPixelFcnt(scan,SB.seg_v.part.r[0],SB.seg_v.part.r[1]);\
            scan++;\
            dstXY.x++;

#define SEGEND


#define STACKSIZE 5

RwTexCoords uvStack[STACKSIZE];

void
stackAdd(RwTexCoords *stack, RwTexCoords *value)
{
    RwInt32 i;

    for(i=STACKSIZE-1;i>0;i--)
    {
        stack[i] = stack[i-1];
    }

    stack[0] = *value;
}

void stackDump(RwTexCoords *stack)
{
    RwInt32 i;

    for(i=STACKSIZE-1;i>-1;i--)
    {
        rwsprintf(msg,"%2d: %4.4fx%4.4f\n",
        i,
        stack[i].u,
        stack[i].v);
        OutputDebugString(msg);
    }
}


static void SegPixelFcnt(RwRGBA *dst, RwReal x, RwReal y)
{
    RwTexCoords toAdd = {x,y};
    RwUInt32 offset = ((RwUInt32)y)*RwImageGetStride(srcImage) + ((RwUInt32)x*4);
    RwRGBA *src = (RwRGBA*)( (RwUInt32)RwImageGetPixels(srcImage) + offset );

    stackAdd(uvStack,&toAdd);
    if(dst->alpha != 0x0f )
    {

        if( ((RwUInt32)src > (RwUInt32)RwImageGetPixels(srcImage)) &&
            ((RwUInt32)src < topReadAddr) )
        {
                *dst = *src;
                dst->alpha = 0x0f ;
        }
        else
        {
                if( FALSE == badTriangle )
                {
                    badTriangle = TRUE;
                    badPolyCount++;
                }
                *dst = defaultBadColor;
                //dst->alpha = 0x0F;
        
    #if (defined dumpUVS)
                rwsprintf(msg,"poly %d : Read out of range:\n",badPolyCount);
                OutputDebugString(msg);

                OutputDebugString("Previous UVs\n");
                stackDump(uvStack);
                OutputDebugString("----\n");
    #endif

        }
    }
}
#else
#define TRIDECL             RwRGBA *scanline;
#define TRISETUP(Y)         SB.scanline = (RwRGBA *)(RwImageGetPixels(dstImage) + Y * RwImageGetStride(dstImage));
#define TRIDY               SB.scanline += RwImageGetStride(dstImage)>>2;
#define SCANDECL            static RwRGBA *scan;
#define SCANSETUP(X)        scan = (SB.scanline + X);
#define SEGDECL
#define SEGBEGIN(W)
    
#ifdef GREEDY
    #define SEGPIXEL \
    {\
        RwUInt32 offset = ((RwUInt32)SB.seg_v.part.r[1])*RwImageGetStride(srcImage) + ((RwUInt32)SB.seg_v.part.r[0]*4);\
        RwRGBA *src = (RwRGBA*)( (RwUInt32)RwImageGetPixels(srcImage) + offset ); \
        if( ((RwUInt32)src > (RwUInt32)RwImageGetPixels(srcImage)) && \
            ((RwUInt32)src < topReadAddr) ) \
        {\
            *scan = *src;\
        }\
        scan++;\
    }
#else
    #define SEGPIXEL \
    {\
        RwUInt32 offset = ((RwUInt32)SB.seg_v.part.r[1])*RwImageGetStride(srcImage) + ((RwUInt32)SB.seg_v.part.r[0]*4);\
        RwRGBA *src = (RwRGBA*)( (RwUInt32)RwImageGetPixels(srcImage) + offset ); \
        *scan = *src;\
        scan++;\
    }
#endif

#endif

#include "prolog.c"

/* implement renderer specific functionality here */
#define TESTEDGE(edg1,edg2)\
    (( (edg1.x == edg2.x) && (edg1.y == edg2.y) ) ||\
    ( (edg1.v.part.r[0] == edg2.v.part.r[0] ) &&\
      (edg1.v.part.r[1] == edg2.v.part.r[1] ) ))

/**********************************************************************/
void
_rtltmapUVRemaper(RwImage *dstImg, RwTexCoords *dstUV, RwImage *srcImg, RwTexCoords *srcUV, RwReal rejectionSize)
{
    RwInt32 i;

    RwUInt32 dstImgWidth = RwImageGetWidth(dstImg)-1;
    RwUInt32 dstImgHeight = RwImageGetHeight(dstImg)-1;
    
#ifdef ULTRADEBUG
    RwReal s;
    RwV2d tmp;
#endif
    srcImgWidth = RwImageGetWidth(srcImg)-1;
    srcImgHeight = RwImageGetHeight(srcImg)-1;

    dstImage = dstImg;
    srcImage = srcImg;

    topReadAddr = (RwUInt32)RwImageGetPixels(srcImage) + 
                  RwImageGetStride(srcImage)*(srcImgHeight-1) + 
                  (srcImgWidth-1)*4;

#ifdef ULTRADEBUG
    polyCount++;
#endif

    for(i=0;i<3;i++)
    {
        EDGE[i].x = dstImgWidth * dstUV[i].u;
        EDGE[i].y = dstImgHeight * dstUV[i].v;
        EDGE[i].v.part.r[0] = (srcUV[i].u * (RwReal)srcImgWidth);
        EDGE[i].v.part.r[1] = (srcUV[i].v * (RwReal)srcImgHeight);
        
#ifdef ULTRADEBUG
        dstCoordinates[i].x = EDGE[i].x;
        dstCoordinates[i].y = EDGE[i].y;

        srcCoordinates[i].x = EDGE[i].v.part.r[0];
        srcCoordinates[i].y = EDGE[i].v.part.r[1];
#endif
    }

    if( TESTEDGE(EDGE[0],EDGE[1]) || 
        TESTEDGE(EDGE[1],EDGE[2]) ||
        TESTEDGE(EDGE[2],EDGE[0]) )
    {
        return;
    }

    /* We don't do small size */
    if( ((RwRealMax3(EDGE[0].x,EDGE[1].x,EDGE[2].x) - RwRealMin3(EDGE[0].x,EDGE[1].x,EDGE[2].x)) < rejectionSize) ||
        ((RwRealMax3(EDGE[0].y,EDGE[1].y,EDGE[2].y) - RwRealMin3(EDGE[0].y,EDGE[1].y,EDGE[2].y)) < rejectionSize) )
    {
        return;
    }
    
#ifdef ULTRADEBUG
    badTriangle = FALSE;

    
    /* 0 1 */     
    RwV2dSub(&tmp,&dstCoordinates[0],&dstCoordinates[1]);
    dstEdgeLength[0] = RwV2dLength(&tmp);

    RwV2dSub(&tmp,&srcCoordinates[0],&srcCoordinates[1]);
    srcEdgeLength[0] = RwV2dLength(&tmp);

    /* 1 2 */ 
    RwV2dSub(&tmp,&dstCoordinates[1],&dstCoordinates[2]);
    dstEdgeLength[1] = RwV2dLength(&tmp);

    RwV2dSub(&tmp,&srcCoordinates[1],&srcCoordinates[2]);
    srcEdgeLength[1] = RwV2dLength(&tmp);

    /* 2 0 */
    RwV2dSub(&tmp,&dstCoordinates[2],&dstCoordinates[0]);
    dstEdgeLength[2] = RwV2dLength(&tmp);

    RwV2dSub(&tmp,&srcCoordinates[2],&srcCoordinates[0]);
    srcEdgeLength[2] = RwV2dLength(&tmp);


    s = (dstEdgeLength[0] + dstEdgeLength[1] + dstEdgeLength[2])/2.0f;
    rwSqrt(&dstArea,s);

    s = (srcEdgeLength[0] + srcEdgeLength[1] + srcEdgeLength[2])/2.0f;
    rwSqrt(&srcArea,s);
#endif

#include "body.c"

#ifdef ULTRADEBUG
    if( badTriangle )
    {
        badPolyCount++;

        rwsprintf(msg,"poly %d : Read out of range:\n",badPolyCount);

/*        if (((srcCoordinates[0].x - srcCoordinates[1].x) * (srcCoordinates[2].y - srcCoordinates[1].y)) <=
            ((srcCoordinates[0].y - srcCoordinates[1].y) * (srcCoordinates[2].x - srcCoordinates[1].x)))
        {
            OutputDebugString("CCW texture coordinates\n");
            OutputDebugString("----\n");
        }*/


#if defined dumpTriAreas
        rwsprintf(msg,"Write Triangle Area : %4.4f\n",dstArea);
        OutputDebugString(msg);

        rwsprintf(msg,"Read Triangle Area : %4.4f\n",srcArea);
        OutputDebugString(msg);

        OutputDebugString("----\n");
#endif
#if defined dumpPolys
        {       
            RwInt32 i;
            for(i=0;i<3;i++)
            {
                rwsprintf(msg,
                            "Vtx %d : %4.4fx%4.4f %4.4fx%4.4f\n",
                            i,
                            dstCoordinates[i].x,dstCoordinates[i].y,
                            srcCoordinates[i].x,srcCoordinates[i].y);
                OutputDebugString(msg);
            }
            OutputDebugString("----\n");
        }
#endif

    }

    Sleep(1);
#endif

    return;
}




