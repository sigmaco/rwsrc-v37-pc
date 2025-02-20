/***************************************************************************
 *                                                                         *
 * Module  :                                                               *
 *                                                                         *
 * Purpose :                                                               *
 *                                                                         *
 **************************************************************************/

/****************************************************************************
 Includes
 */

#include <string.h>

#include "rwcore.h"
#include "rpdbgerr.h"
#include "rpworld.h"

#include "rt2d.h"
#include "path.h"
#include "font.h"
#include "tri.h"
#include "gstate.h"

/****************************************************************************
 Defines
 */

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */

static RwInt32 _rt2dTriPolyFreeListBlockSize = 32,
               _rt2dTriPolyFreeListPreallocBlocks = 1;

static RwFreeList _rt2dTriPolyFreeList;

static RwInt32 _rt2dTriVertFreeListBlockSize = 32,
               _rt2dTriVertFreeListPreallocBlocks = 1;

static RwFreeList _rt2dTriVertFreeList;


/****************************************************************************
 _rt2dTriVertCreate

 On entry   :
 On exit    :
 */

static _rt2dTriVert *
rt2dTriVertCreate( void )
{
    _rt2dTriVert           *vertNew;

    RWFUNCTION(RWSTRING("rt2dTriVertCreate"));

    vertNew = RwFreeListAlloc(Rt2dGlobals.triVertFreeList,
        rwMEMHINTDUR_EVENT | rwID_2DPLUGIN);

    if (vertNew)
    {
        vertNew->next = NULL;
        vertNew->idx = -1;
    }

    RWRETURN(vertNew);
}


/****************************************************************************
 rt2dTriVertDestroy

 On entry   :
 On exit    :
 */

static RwBool
rt2dTriVertDestroy( _rt2dTriVert *vert )
{
    RWFUNCTION(RWSTRING("rt2dTriVertDestroy"));

    if (vert)
    {
        vert->next = NULL;
        vert->idx = -1;

        RwFreeListFree(Rt2dGlobals.triVertFreeList, vert);
    }

    RWRETURN(TRUE);
}

/****************************************************************************
 _rt2dTriVertAddVert

 On entry   :
 On exit    :
 */

static _rt2dTriVert *
rt2dTriVertAddVert( _rt2dTriVert *vertHead, _rt2dTriVert *vert )
{
    RWFUNCTION(RWSTRING("rt2dTriVertAddVert"));

    if (vertHead == NULL)
    {
        vert->next = vert;
    }
    else
    {
        vert->next = vertHead->next;
        vertHead->next = vert;
    }

    RWRETURN(vert);
}

/****************************************************************************
 _rt2dTriPolyCreate

 On entry   :
 On exit    :
 */

static _rt2dTriPoly *
rt2dTriPolyCreate( void )
{
    _rt2dTriPoly           *polyNew;

    RWFUNCTION(RWSTRING("rt2dTriPolyCreate"));

    polyNew = RwFreeListAlloc(Rt2dGlobals.triPolyFreeList,
        rwMEMHINTDUR_EVENT | rwID_2DPLUGIN);

    if (polyNew)
    {
        polyNew->next = NULL;
        polyNew->vert = NULL;
        polyNew->pos = NULL;
        polyNew->vCount = 0;
    }

    RWRETURN(polyNew);
}


/****************************************************************************
 _rt2dTriPolyDestroy

 On entry   :
 On exit    :
 */

static RwBool
rt2dTriPolyDestroy( _rt2dTriPoly *poly )
{
    _rt2dTriVert           *vert, *vertHead, *vertNext;

    RWFUNCTION(RWSTRING("rt2dTriPolyDestroy"));

    if (poly)
    {
        poly->next = NULL;

        if (poly->vert != NULL)
        {
            vertHead = poly->vert;
            vert = vertHead;

            do
            {
                vertNext = vert->next;

                rt2dTriVertDestroy(vert);

                vert = vertNext;

            } while (vert != vertHead);
        }

        if (poly->pos != NULL)
            RwFree(poly->pos);

        poly->vert = NULL;
        poly->pos = NULL;
        poly->vCount = 0;

        RwFreeListFree(Rt2dGlobals.triPolyFreeList, poly);
    }

    RWRETURN(TRUE);
}

/****************************************************************************
 _rt2dPointLineDist

 On entry   :
 On exit    :
 */
static RwReal
rt2dPointLineDist(_rt2dTriPoly *poly,
                   _rt2dTriVert *v, _rt2dTriVert *v0, _rt2dTriVert *v1)
{
    RwReal              dist;
    RwV2d               vec, ppd;

    RWFUNCTION(RWSTRING("rt2dPointLineDist"));

    /* Find the perpendicular */
    ppd.x = poly->pos[v1->idx].y - poly->pos[v0->idx].y;
    ppd.y = poly->pos[v0->idx].x - poly->pos[v1->idx].x;

    RwV2dNormalize(&ppd, &ppd);

    /* Find a line from the point the one of the end points */
    vec.x = poly->pos[v0->idx].x - poly->pos[v->idx].x;
    vec.y = poly->pos[v0->idx].y - poly->pos[v->idx].y;

    /* Project the line to perpendicular */
    dist = RwV2dDotProduct(&vec, &ppd);

    RWRETURN(dist);
}

/****************************************************************************
 _rt2dPointLineClassify

 On entry   :
 On exit    :
 */
static RwUInt32
rt2dPointLineClassify(_rt2dTriPoly *poly,
                       _rt2dTriVert *v, _rt2dTriVert *v0, _rt2dTriVert *v1)
{
    RwUInt32        flag;
    RwReal          dist;

    RWFUNCTION(RWSTRING("rt2dPointLineClassify"));

    dist = rt2dPointLineDist(poly, v, v0, v1);

    /*
     * The area is positive if v, v0, v1 are oriented ccw, negative if cw,
     * and zero if the points are collinear.
     */
    if (dist > 0.0f)
    {
        flag = rt2dPOINTCLASSIFYFLAGLEFT;
    }
    else if (dist < 0.0f)
    {
        flag = rt2dPOINTCLASSIFYFLAGRIGHT;
    }
    else
    {
        flag = rt2dPOINTCLASSIFYFLAGCOLINEAR;
    }

    RWRETURN(flag);
}

/****************************************************************************
 _rt2dPointTriClassify

 On entry   :
 On exit    :
 */
static RwUInt32
rt2dPointTriClassify(_rt2dTriPoly *poly,
                      _rt2dTriVert *v, _rt2dTriVert *v0, _rt2dTriVert *v1, _rt2dTriVert *v2)
{
    RwUInt32            flag, flag0, flag1, flag2;

    RWFUNCTION(RWSTRING("rt2dPointTriClassify"));

    flag0 = rt2dPointLineClassify(poly, v, v0, v1);
    flag1 = rt2dPointLineClassify(poly, v, v1, v2);
    flag2 = rt2dPointLineClassify(poly, v, v2, v0);

    if ((flag0 & flag1 & flag2) == rt2dPOINTCLASSIFYFLAGLEFT)
    {
        flag = rt2dPOINTCLASSIFYFLAGINSIDE;
    }
    else if ((flag0 | flag1 | flag2) & rt2dPOINTCLASSIFYFLAGCOLINEAR)
    {
        flag = rt2dPOINTCLASSIFYFLAGCOLINEAR;
    }
    else
    {
        flag = rt2dPOINTCLASSIFYFLAGOUTSIDE;
    }

    RWRETURN(flag);
}


/****************************************************************************
 _rt2dTriPolyFindConvexVert

 On entry   :
 On exit    :
 */
static _rt2dTriVert *
rt2dTriPolyFindConvexVert(_rt2dTriPoly *poly)
{
    _rt2dTriVert        *v, *v0, *v1, *v2;

    RWFUNCTION(RWSTRING("rt2dTriPolyFindConvexVert"));

    v = NULL;

    v0 = poly->vert;
    v1 = poly->vert->next;
    v2 = poly->vert->next->next;

    do
    {
        if (rt2dPointLineClassify(poly, v2, v0, v1) ==
            rt2dPOINTCLASSIFYFLAGLEFT)
        {
            v = v1;

            poly->vert = v0;

            break;
        }

        v0 = v1;
        v1 = v2;
        v2 = v2->next;

    } while (v0 != poly->vert);

    RWRETURN(v);
}

/****************************************************************************
 _rt2dTriPolyFindIntrudingVert

 On entry   :
 On exit    :
 */
static _rt2dTriVert *
rt2dTriPolyFindIntrudeVert(_rt2dTriPoly *poly)
{
    _rt2dTriVert            *v, *v0, *v1, *v2, *intrude;
    RwReal                  best, dist;

    RWFUNCTION(RWSTRING("rt2dTriPolyFindIntrudeVert"));

    v0 = poly->vert;
    v1 = poly->vert->next;
    v2 = poly->vert->next->next;

    intrude = NULL;

    v = v2->next;

    best = 0.0f;

    while (v != v0)
    {
        if (rt2dPointTriClassify(poly, v, v0, v1, v2) ==
            rt2dPOINTCLASSIFYFLAGINSIDE)
        {
            dist = rt2dPointLineDist(poly, v, v2, v0);

            if (dist > best)
            {
                intrude = v;

                best = dist;
            }
        }

        v = v->next;
    }

    RWRETURN(intrude);
}

/****************************************************************************
 _rt2dTriPolygonTriangulate

 On entry   :
 On exit    :
 */
static _rt2dTriPoly *
rt2dTriPolySplit( _rt2dTriPoly *polyA, _rt2dTriVert *v0, _rt2dTriVert *v1 )
{
    _rt2dTriPoly        *polyB;
    _rt2dTriVert        *vertA, *vertB, *vert, *vertNext, *vertNew;
    RwInt32             countA, countB;

    RWFUNCTION(RWSTRING("rt2dTriPolySplit"));

    RWASSERT(v0 != v1);

    countA = 0;
    countB = 0;

    polyB = rt2dTriPolyCreate();

    if (polyB)
    {
        vertA = NULL;
        vertB = NULL;

        vert = polyA->vert;

        /* Find a known starting position. Makes closing easier */
        while (vert != v0)
        {
            vert = vert->next;
        }

        /* Build the first poly */
        while (vert != v1)
        {
            vertNext = vert->next;

            vertA = rt2dTriVertAddVert(vertA, vert);

            vert = vertNext;

            countA++;
        }

        vertNew = rt2dTriVertCreate();

        vertNew->idx = v1->idx;

        vertA = rt2dTriVertAddVert(vertA, vertNew);

        countA++;

        /* Build the first poly */
        while (vert != v0)
        {
            vertNext = vert->next;

            vertB = rt2dTriVertAddVert(vertB, vert);

            vert = vertNext;

            countB++;
        }

        vertNew = rt2dTriVertCreate();

        vertNew->idx = v0->idx;

        vertB = rt2dTriVertAddVert(vertB, vertNew);

        countB++;

        /* Re-assign the vert list */
        polyA->vert = vertA;
        polyB->vert = vertB;

        polyA->vCount = countA;
        polyB->vCount = countB;

        polyB->pos = polyA->pos;
        polyB->triIndex = polyA->triIndex;
    }

    RWRETURN(polyB);
}


/****************************************************************************
 _rt2dTriPolygonTriangulate

 On entry   :
 On exit    :
 */
static _rt2dTriPoly *
rt2dTriPolyTriangulate( _rt2dTriPoly *poly )
{
    _rt2dTriPoly           *p1;
    _rt2dTriVert           *convexVert, *intrudeVert;

    RWFUNCTION(RWSTRING("rt2dTriPolyTriangulate"));

    if (poly->vCount == 3)
    {
        poly->triIndex->idx[poly->triIndex->currIdx] =
            poly->vert->idx;
        poly->triIndex->currIdx++;

        poly->triIndex->idx[poly->triIndex->currIdx] =
            poly->vert->next->idx;
        poly->triIndex->currIdx++;

        poly->triIndex->idx[poly->triIndex->currIdx] =
            poly->vert->next->next->idx;
        poly->triIndex->currIdx++;
    }
    else
    {
        convexVert = rt2dTriPolyFindConvexVert(poly);

        if (convexVert)
        {
            intrudeVert = rt2dTriPolyFindIntrudeVert(poly);

            if (intrudeVert == NULL)
            {
                p1 = rt2dTriPolySplit(poly, poly->vert, poly->vert->next->next);
            }
            else
            {
                p1 = rt2dTriPolySplit(poly, convexVert, intrudeVert);
            }

            rt2dTriPolyTriangulate(p1);

            rt2dTriPolyTriangulate(poly);

            p1->pos = NULL;
            rt2dTriPolyDestroy(p1);
        }
    }

    RWRETURN(poly);
}

/****************************************************************************
 _rt2dPathToPoly

 On entry   :
 On exit    :
 */
static _rt2dTriPoly *
rt2dPathToPoly( Rt2dPath *path )
{
    rt2dPathNode       *pNode;
    RwInt32             vCount, i;

    _rt2dTriPoly        *polyNew;
    _rt2dTriVert        *vert, *vertNew;

    RWFUNCTION(RWSTRING("rt2dPathToPoly"));

    pNode = _rt2dPathGetSegmentArray(path);

    /* Ignore the last vert */
    vCount = _rt2dPathGetNumSegment(path) - 1;

    polyNew = rt2dTriPolyCreate();

    if (polyNew)
    {
        polyNew->pos = (RwV2d *) RwMalloc(vCount * sizeof(RwV2d),
                              rwMEMHINTDUR_EVENT | rwID_2DPLUGIN);

        if (polyNew->pos != NULL)
        {
            vert = NULL;

            for (i = 0; i < vCount; i++)
            {
                vertNew = rt2dTriVertCreate();

                vertNew->idx = i;

                polyNew->pos[i] = pNode->pos;

                vert = rt2dTriVertAddVert(vert, vertNew);

                pNode++;
            }

            polyNew->vert = vert;
            polyNew->vCount = vCount;
        }
    }

    RWRETURN(polyNew);
}

/****************************************************************************
 _rt2dPathTriangulate

 On entry   :
 On exit    :
 */
Rt2dPath *
_rt2dPathTriangulate( Rt2dPath *path, RwInt32 *count, RwInt32 *index )
{
    _rt2dTriPoly        *poly;
    _rt2dTriIndex       *triIndex;

    RWFUNCTION(RWSTRING("_rt2dPathTriangulate"));

    poly = rt2dPathToPoly( path );

    if (poly)
    {
        triIndex = &Rt2dGlobals.triVertIndex;
        triIndex->idx = index;
        triIndex->currIdx = 0;

        poly->triIndex = triIndex;

        rt2dTriPolyTriangulate(poly);

        *count = triIndex->currIdx / 3;

        rt2dTriPolyDestroy(poly);
    }

    RWRETURN(path);
}

/****************************************************************************
 _rt2dTriangulateClose

 On entry   :
 On exit    :
 */

void
_rt2dTriangulateClose( void )
{
    RWFUNCTION(RWSTRING("_rt2dTriangulateClose"));

    RwFreeListDestroy(Rt2dGlobals.triVertFreeList);
    RwFreeListDestroy(Rt2dGlobals.triPolyFreeList);

    Rt2dGlobals.triVertFreeList = (RwFreeList *)NULL;
    Rt2dGlobals.triPolyFreeList = (RwFreeList *)NULL;

    RWRETURNVOID();
}

/**
 * \ingroup rt2d
 * \ref Rt2dTriVertSetFreeListCreateParams allows the developer to specify
 * how many vertices to preallocate space for. The triangulate function
 * creates intermediate internal vertices during triangulation.
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
Rt2dTriVertSetFreeListCreateParams( RwInt32 blockSize, RwInt32 numBlocksToPrealloc )
{
    /* cannot use debugging macros since the debugger is not initialized before RwEngineInit */
    /*
    RWAPIFUNCTION( RWSTRING( "Rt2dTriVertSetFreeListCreateParams" ) );
    */

    _rt2dTriVertFreeListBlockSize = blockSize;
    _rt2dTriVertFreeListPreallocBlocks = numBlocksToPrealloc;

    /*
    RWRETURNVOID();
    */
}

/**
 * \ingroup rt2d
 * \ref Rt2dTriPolySetFreeListCreateParams allows the developer to specify
 * how many polys to preallocate space for. The triangulate function
 * creates intermediate internal polygons during triangulation.
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
Rt2dTriPolySetFreeListCreateParams( RwInt32 blockSize, RwInt32 numBlocksToPrealloc )
{
    /* cannot use debugging macros since the debugger is not initialized before RwEngineInit */
    /*
    RWAPIFUNCTION( RWSTRING( "Rt2dTriPolySetFreeListCreateParams" ) );
    */

    _rt2dTriPolyFreeListBlockSize = blockSize;
    _rt2dTriPolyFreeListPreallocBlocks = numBlocksToPrealloc;

    /*
    RWRETURNVOID();
    */
}

/****************************************************************************
 _rt2dTriangulateOpen

 On entry   :
 On exit    :
 */

RwBool
_rt2dTriangulateOpen( void )
{
    RWFUNCTION(RWSTRING("_rt2dTriangulateOpen"));

    Rt2dGlobals.triVertFreeList =
        RwFreeListCreateAndPreallocateSpace(sizeof(_rt2dTriVert),
            _rt2dTriVertFreeListBlockSize, sizeof(RwInt32),
            _rt2dTriVertFreeListPreallocBlocks,
            &_rt2dTriVertFreeList,
            rwMEMHINTDUR_GLOBAL | rwID_2DPLUGIN);

    Rt2dGlobals.triPolyFreeList =
        RwFreeListCreateAndPreallocateSpace(sizeof(_rt2dTriPoly),
            _rt2dTriPolyFreeListBlockSize, sizeof(RwInt32),
            _rt2dTriPolyFreeListPreallocBlocks,
            &_rt2dTriPolyFreeList,
            rwMEMHINTDUR_GLOBAL | rwID_2DPLUGIN);

    RWRETURN(TRUE);
}


