/*
 *
 * Functionality for 2D rendering
 *
 * Copyright (c) 1998 Criterion Software Ltd.
 */

/****************************************************************************
 *                                                                          *
 *  Module  :   path.c                                                      *
 *                                                                          *
 *  Purpose :   Build paths                                                 *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
 */
#include <string.h>

#include "rwcore.h"
#include "rpdbgerr.h"
#include "rpworld.h"

#include "rt2d.h"
#include "font.h"
#include "path.h"
#include "tri.h"
#include "gstate.h"
#include "path.h"
#include "stroke.h"
#include "fill.h"
#include "shape.h"
#include "rt2d.h"

#if defined (__MWERKS__)
#if (defined(RWVERBOSE))
#pragma message (__FILE__ "/" _SKY_EXPAND(__LINE__) ": __MWERKS__ == " _SKY_EXPAND(__MWERKS__))
#endif /* (defined (__MWERKS__)) */
#if (__option (global_optimizer))
#pragma always_inline off
#endif /* (__option (global_optimizer)) */
#endif /*  defined (__MWERKS__) */

/****************************************************************************
 Local Types
 */

typedef RwV2d       RtCubicBezier[4];

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */
#define FILE_CURRENT_VERSION 0x01
#define FILE_LAST_SUPPORTED_VERSION 0x01
/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */

/**
 * \ingroup rt2dpath
 * \page rt2dpathoverview Rt2dPath Overview
 *
 * This overview explains the basics about creating, rendering and destroying
 * paths. Refer to the API reference for more path functions. The User Guide
 * chapter \e Using \e the \e 2D \e Toolkit explains the Rt2d toolkit in more
 * detail.
 *
 * \par Creating a Path
 *
 * To create a path follow these steps:
 *
 * -#  \ref Rt2dPathCreate is used to create a new path.
 * -#  \ref Rt2dPathLock locks the path.
 * -#  Use one or more of the following functions to create the lines
 *     required:
 *     \li \ref Rt2dPathMoveto is used to define the current point for the
 *      specified path using the given coordinates.
 *      \li \ref Rt2dPathCurveto is used to add a curved line segment to the
 *      specified path. Specifies absolute coordinates.
 *      \li \ref Rt2dPathLineto is used to add a straight line segment to the
 *      specified path that extends from the current point to a point
 *      with the absolute coordinates.
 *      \li \ref Rt2dPathRCurveto is used to add a curved line segment to the
 *      specified path. Specifies relative coordinates from the previous
 *      point in the path.
 *      \li \ref Rt2dPathRLineto is used to add a straight line segment to the
 *      specified path that extends from the current point to a point
 *      with relative coordinates from the previous point in the path.
 *      \li  \ref Rt2dPathClose is used to add a line segment to the specified path
 *      that joins the current point with the first point defined for this
 *      path. This is optional.
 *
 * -#  \ref Rt2dPathUnlock unlocks the path.
 *
 *
 * \par Creating Rectangles and Ovals
 *
 * To create rectangles or ovals follow these steps on an existing path:
 *
 * -# \ref Rt2dPathLock locks the path.
 * -# Create the required object using:
 *    \li \ref Rt2dPathRect creates a rectangle.
 *    \li \ref Rt2dPathRoundRect creates a rectangle with rounded corners.
 *    \li \ref Rt2dPathOval creates an outline oval to the specified path
 *        that has the given position, width and height.
 *
 * -# \ref Rt2dPathUnlock unlocks the path.
 *
 * \par Rendering a Path
 * \li \ref Rt2dPathFill is used to fill the specified path using the colors
 *     and texture coordinates of the given brush.
 * \li \ref Rt2dPathStroke is used to paint the specified path using the given
 *     brush.
 *
 * \par Destroying a Path
 * \li \ref Rt2dPathDestroy destroys a path.
 * \li \ref Rt2dPathEmpty deletes all data describing the path without
 *      actually destroying the path.
 *
 */

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   Functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/****************************************************************************/

/* Remove the 'x' enable SubPathPrint2d */
#define RW2Dx

#if (defined(RW2D) && defined(RWDEBUG))

static Rt2dPath    *
SubPathPrint2d(Rt2dPath * path)
{
    RWFUNCTION(RWSTRING("SubPathPrint2d"));

    if (path)
    {
        rt2dPathNode       *pnode;
        RwInt32             vcount, i;

        pnode = _rt2dPathGetSegmentArray(path);
        vcount = _rt2dPathGetNumSegment(path);

        /* copy prims */
        for (i = 0; i < vcount; i++)
        {
            switch (pnode[i].type)
            {
                case rt2dMOVE:
                    RWMESSAGE(("[%g %g] rt2dMOVE",
                               pnode[i].pos.x, pnode[i].pos.y));
                    break;
                case rt2dLINE:
                    RWMESSAGE(("[%g %g] rt2dLINE",
                               pnode[i].pos.x, pnode[i].pos.y));
                    break;
                case rt2dCURVE:
                    RWMESSAGE(("[%g %g] [%g %g] [%g %g] rt2dCURVE",
                               pnode[i].pos.x,
                               pnode[i].pos.y,
                               pnode[i + 1].pos.x,
                               pnode[i + 1].pos.y,
                               pnode[i + 2].pos.x, pnode[i + 2].pos.y));
                    i += 2;
                    break;
                default:
                    RWMESSAGE(("Unrecognized type"));
                    break;
            }
        }
        RWMESSAGE(("%s\n", path->closed ? "Closed" : "Open"));
    }

    RWRETURN(path);
}

#define RT2DSUBPATHPRINT(path) SubPathPrint2d(path)

#endif /* ( defined(RW2D) && defined(RWDEBUG) ) */

#if (!defined(RT2DSUBPATHPRINT))
#define RT2DSUBPATHPRINT(path) /* No op */
#endif /* (!defined(RT2DSUBPATHPRINT)) */

/****************************************************************************/

#define Rt2dIsFlatMacro(result, _control)                               \
MACRO_START                                                             \
{                                                                       \
    RwV2d               delta, dir;                                     \
    RwReal              Sqdist, dot;                                    \
                                                                        \
    RwV2dSub(&delta, &(_control)[3], &(_control)[0]);                   \
    Sqdist = RwV2dDotProduct(&delta, &delta);                           \
                                                                        \
    RwV2dSub(&dir, &(_control)[1], &(_control)[0]);                     \
    dot = RwV2dDotProduct(&dir, &delta);                                \
                                                                        \
    result =                                                            \
        (RwV2dDotProduct(&dir, &dir) - Rt2dGlobals.sqTolerance) *       \
        Sqdist < (dot * dot);                                           \
                                                                        \
    if (result)                                                         \
    {                                                                   \
                                                                        \
        RwV2dSub(&dir, &(_control)[2], &(_control)[0]);                 \
        dot = RwV2dDotProduct(&dir, &delta);                            \
                                                                        \
        result =                                                        \
            (RwV2dDotProduct(&dir, &dir) - Rt2dGlobals.sqTolerance) *   \
            Sqdist < (dot * dot);                                       \
                                                                        \
    }                                                                   \
}                                                                       \
MACRO_STOP

#define SubPathMoveto2dMacro(_path, _pt)                     \
MACRO_START                                                     \
{                                                               \
    rt2dPathNode         *_pnode = (rt2dPathNode *)             \
        rwSListGetNewEntry((RwSList *)(((_path))->segments),    \
                           rwID_2DPLUGIN | rwMEMHINTDUR_EVENT); \
                                                                \
    _pnode->type = rt2dMOVE;                                    \
    _pnode->pos.x = ((_pt))->x;                                 \
    _pnode->pos.y = ((_pt))->y;                                 \
    /* normal unknowable */                                     \
    _pnode->normal.x = (RwReal) 0.0;                            \
    _pnode->normal.y = (RwReal) 0.0;                            \
    _pnode->dist = 0.0f;                                        \
                                                                \
}                                                               \
MACRO_STOP

#if (0)
#define SubPathLineto2dMacro(_path, _pt)                     \
MACRO_START                                                     \
{                                                               \
    RwV2d                dir;                                   \
    RwReal               length;                                \
    rt2dPathNode         *_pnode = (rt2dPathNode *)             \
        rwSListGetNewEntry((RwSList *)(((_path))->segments),    \
                           rwID_2DPLUGIN | rwMEMHINTDUR_EVENT); \
                                                                \
    _pnode->type = rt2dLINE;                                    \
    _pnode->pos.x = ((_pt))->x;                                 \
    _pnode->pos.y = ((_pt))->y;                                 \
    dir.x = ((_pt))->y - (_pnode-1)->pos.y;                     \
    dir.y = (_pnode-1)->pos.x - ((_pt))->x;                     \
    RwV2dNormalizeMacro(length, &_pnode->normal, &dir);         \
    _pnode->dist = (_pnode-1)->dist + length;                   \
    _pnode->pos.z = _pnode->dist;                               \
}                                                               \
MACRO_STOP
#endif /* (0) */

#define SubPathLineto2dMacro(_path, _pt)                     \
MACRO_START                                                     \
{                                                               \
    RwV2d                dir;                                   \
    RwReal               l;                                     \
    RwInt32              vcount =                               \
        rwSListGetNumEntries((RwSList *)((_path)->segments));   \
    rt2dPathNode         *_pnode = (rt2dPathNode *)             \
        rwSListGetNewEntry((RwSList *)(((_path))->segments),    \
                           rwID_2DPLUGIN | rwMEMHINTDUR_EVENT); \
                                                                \
                                                                \
    _pnode->type = rt2dLINE;                                    \
    _pnode->pos.x = ((_pt))->x;                                 \
    _pnode->pos.y = ((_pt))->y;                                 \
    dir.x = ((_pt))->y - (_pnode-1)->pos.y;                     \
    dir.y = (_pnode-1)->pos.x - ((_pt))->x;                     \
    RwV2dNormalizeMacro(l, &(_pnode)->normal, &dir);            \
    _pnode->dist = (_pnode-1)->dist + l;                        \
                                                                \
    if (vcount > 1)                                             \
    {                                                           \
        RwV2dAdd(&dir, &(_pnode-1)->normal, &(_pnode)->normal); \
        l = 1.0f / RwV2dDotProduct(&(_pnode-1)->normal, &dir);  \
        RwV2dScale(&(_pnode-1)->normal, &dir, l);               \
    }                                                           \
    else                                                        \
    {                                                           \
        (_pnode-1)->normal = (_pnode)->normal;                  \
    }                                                           \
}                                                               \
MACRO_STOP

#define SubPathCurveto2dMacro(_path, _pt1,  _pt2, _pt3)      \
MACRO_START                                                     \
{                                                               \
    RwV2d                dir;                                   \
    RwReal               length;                                \
    RwInt32              vcount =                               \
        rwSListGetNumEntries((RwSList *)((_path)->segments));   \
    rt2dPathNode         *_pnode = (rt2dPathNode *)             \
        rwSListGetNewEntries((RwSList *)(((_path))->segments),  \
                         3, rwID_2DPLUGIN | rwMEMHINTDUR_EVENT);\
                                                                \
    _pnode->type = rt2dCURVE;                                   \
    _pnode->pos.x = ((_pt1))->x;                                \
    _pnode->pos.y = ((_pt1))->y;                                \
    dir.x = ((_pt1))->y - (_pnode-1)->pos.y;                    \
    dir.y = (_pnode-1)->pos.x - ((_pt1))->x;                    \
    RwV2dNormalizeMacro(length, &_pnode->normal, &dir);           \
    _pnode->dist = (_pnode-1)->dist + length;                   \
                                                                \
    if (vcount > 1)                                            \
    {                                                           \
        RwV2dAdd(&dir, &(_pnode-1)->normal, &(_pnode)->normal); \
        length = 1.0f / RwV2dDotProduct(&(_pnode-1)->normal, &dir);  \
        RwV2dScale(&(_pnode-1)->normal, &dir, length);               \
    }                                                           \
    else                                                        \
    {                                                           \
        (_pnode-1)->normal = (_pnode)->normal;                  \
    }                                                           \
                                                                \
    _pnode++;                                                   \
    _pnode->type = rt2dCURVE;                                   \
    _pnode->pos.x = ((_pt2))->x;                                \
    _pnode->pos.y = ((_pt2))->y;                                \
    dir.x = ((_pt2))->y - (_pnode-1)->pos.y;                    \
    dir.y = (_pnode-1)->pos.x - ((_pt2))->x;                    \
    RwV2dNormalizeMacro(length, &_pnode->normal, &dir);           \
    _pnode->dist = (_pnode-1)->dist + length;                   \
                                                                \
    _pnode++;                                                   \
    _pnode->type = rt2dCURVE;                                   \
    _pnode->pos.x = ((_pt3))->x;                                \
    _pnode->pos.y = ((_pt3))->y;                                \
    dir.x = ((_pt3))->y - (_pnode-1)->pos.y;                    \
    dir.y = (_pnode-1)->pos.x - ((_pt3))->x;                    \
    RwV2dNormalizeMacro(length, &_pnode->normal, &dir);         \
    _pnode->dist = (_pnode-1)->dist + length;                   \
                                                                \
    (_path)->flat = FALSE;                                      \
}                                                               \
MACRO_STOP

#define SubPathClose2dMacro(_path)           \
MACRO_START                                     \
{                                               \
    (_path)->closed = TRUE;                     \
}                                               \
MACRO_STOP

#if (! ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) ))

#define SubPathMoveto2d(_path, _pt)                          \
  SubPathMoveto2dMacro(_path, _pt)

#define SubPathLineto2d(_path, _pt)                          \
  SubPathLineto2dMacro(_path, _pt)

#define SubPathCurveto2d(_path, _pt1, _pt2, _pt3)            \
  SubPathCurveto2dMacro(_path, _pt1, _pt2, _pt3)

#define SubPathClose2d(_path)                                \
  SubPathClose2dMacro(_path)

#else /*  (! ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) )) */

/****************************************************************************/

static void
SubPathMoveto2d(Rt2dPath * const path, const RwV2d * const pt)
{
    RWFUNCTION(RWSTRING("SubPathMoveto2d"));
    RWASSERT(_rt2dPathIsLocked(path));

    SubPathMoveto2dMacro(path, pt);

    RWRETURNVOID();
}

/****************************************************************************/

static void
SubPathLineto2d(Rt2dPath * const path, const RwV2d * const pt)
{
    RWFUNCTION(RWSTRING("SubPathLineto2d"));
    RWASSERT(_rt2dPathIsLocked(path));

    RWASSERT(rwSListGetNumEntries((RwSList *)((path)->segments)) > 0);

    SubPathLineto2dMacro(path, pt);

    RWRETURNVOID();
}

/****************************************************************************/

static void
SubPathCurveto2d(Rt2dPath * const path,
                 const RwV2d * const pt1,
                 const RwV2d * const pt2, const RwV2d * const pt3)
{
    RWFUNCTION(RWSTRING("SubPathCurveto2d"));
    RWASSERT(_rt2dPathIsLocked(path));

    RWASSERT(rwSListGetNumEntries((RwSList *)((path)->segments)) > 0);

    SubPathCurveto2dMacro(path, pt1, pt2, pt3);

    RWRETURNVOID();
}

/****************************************************************************/

static void
SubPathClose2d(Rt2dPath * path)
{
    RWFUNCTION(RWSTRING("SubPathClose2d"));

    SubPathClose2dMacro(path);

    RWRETURNVOID();
}

/****************************************************************************/

#if (0)
static              RwBool
IsFlat2d(const RtCubicBezier control)
{
    RwBool              result;

    RWFUNCTION(RWSTRING("IsFlat2d"));

    Rt2dIsFlatMacro(result, control);

    RWRETURN(result);

}
#endif /* (0) */

#endif /*  (! ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) )) */

/****************************************************************************/

static Rt2dPath    *
SubPathDestroy2d(Rt2dPath * path)
{
    Rt2dPath           *next;

    RWFUNCTION(RWSTRING("SubPathDestroy2d"));

    RWASSERT(path);

    next = path->next;

    if (path->segments != NULL)
    {
        if(_rt2dPathIsLocked(path))
        {
            rwSListDestroy((RwSList *)(path->segments));
        }
        else
        {
            RwFree(path->segments);
        }
    }

    path->segments = NULL;
    RwFreeListFree(Rt2dGlobals.pathFreeList, path);
    path = next;

    /* tail of path */
    RWRETURN(path);
}

/****************************************************************************/

static void
SubPathEmpty2d(Rt2dPath * path)
{
    RWFUNCTION(RWSTRING("SubPathEmpty2d"));
    RWASSERT(path);
    RWASSERT(_rt2dPathIsLocked(path));

    rwSListEmpty((RwSList *)(path->segments));
    path->closed = FALSE;
    path->flat = TRUE;

    RWRETURNVOID();
}

/****************************************************************************/

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathEmpty
 * is used to delete all data describing the specified path
 * without actually destroying it. This leaves the path empty and ready
 * for a new path description to be constructed.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param path  Pointer to the path.
 * \return a pointer to the path if successful or NULL if there is an
 * error.
 * \see Rt2dPathCreate
 * \see Rt2dPathDestroy
 */
Rt2dPath           *
Rt2dPathEmpty(Rt2dPath * path)
{
    RWAPIFUNCTION(RWSTRING("Rt2dPathEmpty"));

    RWASSERT(NULL != path);
    RWASSERT(_rt2dPathIsLocked(path));

    Rt2dPathDestroy(path->next);
    path->next = (Rt2dPath *)NULL;

    SubPathEmpty2d(path);
    path->curr = path;

    RWRETURN(path);
}

/****************************************************************************/

static Rt2dPath    *
SubPathCopy2d(Rt2dPath * dst, const Rt2dPath * src)
{
    rt2dPathNode       *pnode, *qnode;
    RwInt32             vcount;

    RWFUNCTION(RWSTRING("SubPathCopy2d"));

    RWASSERT(src);
    RWASSERT(dst);

    pnode = _rt2dPathGetSegmentArray(src);
    vcount = _rt2dPathGetNumSegment(src);

    if(TRUE == _rt2dPathIsLocked(dst))
    {
        Rt2dPathEmpty(dst);

        rwSListGetNewEntries((RwSList *)(dst->segments),
                             vcount, rwID_2DPLUGIN | rwMEMHINTDUR_EVENT);
    }
    else
    {
        RWASSERT(vcount <= _rt2dPathGetNumSegment(dst));
    }

    qnode = _rt2dPathGetSegmentArray(dst);
    memcpy(qnode, pnode, sizeof(rt2dPathNode) * vcount);

    if (src->closed)
    {
        SubPathClose2d(dst);
    }

    dst->flat = src->flat;

    RWRETURN(dst);
}

/****************************************************************************/

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathCopy
 * is used to duplicate the definition of the source path
 * and store the result in the destination path.
 * Note that the destination path is emptied before the copy is
 * performed, therefore, the source and destination path arguments must
 * not point to the same path.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param dst  Pointer to the path that will receive the copy.
 * \param src  Pointer to the path that is to be copied.
 * \return a pointer to the destination path if successful or NULL if
 * there is an error.
 * \see Rt2dPathCreate
 * \see Rt2dPathDestroy
 */
Rt2dPath           *
Rt2dPathCopy(Rt2dPath * dst, const Rt2dPath * src)
{
    Rt2dPath           *srctarg, *dsttarg;

    RWAPIFUNCTION(RWSTRING("Rt2dPathCopy"));

    RWASSERT(src);
    RWASSERT(dst);

    Rt2dPathEmpty(dst);

    SubPathCopy2d(dst, src);
    dsttarg = dst;
    for (srctarg = src->next; srctarg; srctarg = srctarg->next)
    {
        dsttarg->next = Rt2dPathCreate();
        dsttarg = SubPathCopy2d(dsttarg->next, srctarg);
    }

    RWRETURN(dst);
}

/****************************************************************************/

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathMoveto
 * is used to define the current point for the specified
 * path using the given coordinates.
 * Note that positions are defined in absolute coordinates and are
 * subject to the current transformation matrix (CTM).
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param path  Pointer to the path.
 * \param x  A RwReal value equal to the x-coordinate of the point.
 * \param y  A RwReal value equal to the y-coordinate of the point.
 * \return a pointer to the path if successful or NULL if there is an
 * error.
 * \see Rt2dPathLineto
 * \see Rt2dPathCurveto
 * \see Rt2dPathRLineto
 * \see Rt2dPathRCurveto
 */
Rt2dPath           *
Rt2dPathMoveto(Rt2dPath * path, RwReal x, RwReal y)
{

    RWAPIFUNCTION(RWSTRING("Rt2dPathMoveto"));

    RWASSERT(NULL != path);
    RWASSERT(_rt2dPathIsLocked(path));

    if (rwSListGetNumEntries((RwSList *)(path->curr->segments)) == 0)
    {
        RwV2d               pt;

        pt.x = x;
        pt.y = y;
        SubPathMoveto2d(path->curr, &pt);
    }
    else
    {
        /* start a new subpath */
        path->curr->next = Rt2dPathCreate();
        Rt2dPathLock(path->curr->next);
        path->curr = path->curr->next;
        Rt2dPathMoveto(path, x, y);
    }

    RWRETURN(path);
}

/****************************************************************************/

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathLineto
 * is used to add a straight line segment to the specified
 * path that extends from the current point to a point with the given
 * coordinates. After this function has executed the current point is
 * equal to the point with the specified position.
 * Note that positions are defined in absolute coordinates. Use the
 * function \ref Rt2dPathRLineto to specify coordinates relative to the
 * current point.
 *
 * Also note that the positions are subject to the current transformation
 * matrix (CTM).
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param path  Pointer to the path.
 * \param x  A RwReal value equal to the x-coordinate of the line's end-point
 * \param y  A RwReal value equal to the y-coordinate of the line's end-point
 * \return a pointer to the path if successful or NULL if there is an
 * error.
 * \see Rt2dPathRLineto
 * \see Rt2dPathCurveto
 * \see Rt2dPathRCurveto
 * \see Rt2dPathMoveto
 */
Rt2dPath           *
Rt2dPathLineto(Rt2dPath * path, RwReal x, RwReal y)
{

    RwV2d               pt;

    RWAPIFUNCTION(RWSTRING("Rt2dPathLineto"));
    RWASSERT(_rt2dPathIsLocked(path));

    RWASSERT(NULL != path);

    pt.x = x;
    pt.y = y;
    SubPathLineto2d(path->curr, &pt);

    RWRETURN(path);
}

/****************************************************************************/

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathRLineto
 * is used to add a straight line segment to the
 * specified path that extends from the current point to a point with the
 * given coordinates. After this function has executed the current point
 * is equal to the point with the specified position.
 * Note that positions are defined using coordinates relative to the
 * currrent point. Use the function \ref Rt2dPathLineto to specify absolute
 * coordinates.
 *
 * Also note that the positions are subject to the current transformation
 * matrix (CTM).
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param path  Pointer to the path.
 * \param x  A RwReal value equal to the x-coordinate of the line's end-point.
 * \param y  A RwReal value equal to the y-coordinate of the line's  end-point.
 * \return a pointer to the path if successful or NULL if there is an
 * error.
 * \see Rt2dPathLineto
 * \see Rt2dPathRCurveto
 * \see Rt2dPathCurveto
 * \see Rt2dPathMoveto
 */
Rt2dPath           *
Rt2dPathRLineto(Rt2dPath * path, RwReal x, RwReal y)
{
    rt2dPathNode       *pnode;
    RwInt32             vcount;

    RWAPIFUNCTION(RWSTRING("Rt2dPathRLineto"));
    RWASSERT(_rt2dPathIsLocked(path));

    RWASSERT(NULL != path);

    vcount = rwSListGetNumEntries((RwSList *)(path->curr->segments));
    pnode = (rt2dPathNode *)
        _rwSListGetEntry((RwSList *)(path->curr->segments),
                       vcount - 1);

    Rt2dPathLineto(path, pnode->pos.x + x, pnode->pos.y + y);

    RWRETURN(path);

}

/****************************************************************************/

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathCurveto
 * is used to add a curved line segment to the specified
 * path. The curve is defined by the four control points of a cubic
 * Bezier whose positions are given by the current point and the points
 * passed to this function (in that order). After this function has
 * executed, the current point is equal to the last position specified
 * here.
 *
 * Note that positions are defined in absolute coordinates. Use the
 * function \ref Rt2dPathRCurveto to specify coordinates relative to the
 * current point.
 *
 * Also note that the positions are subject to the current transformation
 * matrix (CTM).
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param path  Pointer to the path.
 * \param x1  A RwReal value equal to the x-coordinate of the second control
 * \param y1  A RwReal value equal to the y-coordinate of the second control
 * \param x2  A RwReal value equal to the x-coordinate of the third control
 * \param y2  A RwReal value equal to the y-coordinate of the third control
 * \param x3  A RwReal value equal to the x-coordinate of the fourth control
 * \param y3  A RwReal value equal to the y-coordinate of the fourth control
 * \return a pointer to the path if successful or NULL if there is an
 * error.
 * \see Rt2dPathRCurveto
 * \see Rt2dPathLineto
 * \see Rt2dPathRLineto
 * \see Rt2dPathMoveto
 */
Rt2dPath           *
Rt2dPathCurveto(Rt2dPath * path, RwReal x1, RwReal y1, RwReal x2,
                RwReal y2, RwReal x3, RwReal y3)
{
    RwV2d               pt1, pt2, pt3;

    RWAPIFUNCTION(RWSTRING("Rt2dPathCurveto"));
    RWASSERT(_rt2dPathIsLocked(path));
    RWASSERT(NULL != path);

    pt1.x = x1;
    pt1.y = y1;

    pt2.x = x2;
    pt2.y = y2;

    pt3.x = x3;
    pt3.y = y3;

    SubPathCurveto2d(path->curr, &pt1, &pt2, &pt3);

    RWRETURN(path);
}

/****************************************************************************/

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathRCurveto
 * is used to add a curved line segment to the specified
 * path. The curve is defined by the four control points of a cubic
 * Bezier whose positions are given by the current point and the points
 * passed to this function (in that order). After this function has
 * executed, the current point is equal to the last position specified
 * here.
 *
 * Note that positions are defined using coordinates relative to the
 * currrent point. Use the function \ref Rt2dPathCurveto to specify absolute
 * coordinates.
 *
 * Also note that the positions are subject to the current transformation
 * matrix (CTM).
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param path  Pointer to the path.
 * \param x1  A RwReal value equal to the x-coordinate of the second control
 * \param y1  A RwReal value equal to the y-coordinate of the second control
 * \param x2  A RwReal value equal to the x-coordinate of the third control
 * \param y2  A RwReal value equal to the y-coordinate of the third control
 * \param x3  A RwReal value equal to the x-coordinate of the fourth control
 * \param y3  A RwReal value equal to the y-coordinate of the fourth control
 * \return a pointer to the path if successful or NULL if there is an
 * error.
 * \see Rt2dPathCurveto
 * \see Rt2dPathRLineto
 * \see Rt2dPathLineto
 * \see Rt2dPathMoveto
 */
Rt2dPath           *
Rt2dPathRCurveto(Rt2dPath * path, RwReal x1, RwReal y1, RwReal x2,
                 RwReal y2, RwReal x3, RwReal y3)
{
    rt2dPathNode       *pnode;
    RwInt32             vcount;

    RWAPIFUNCTION(RWSTRING("Rt2dPathRCurveto"));
    RWASSERT(_rt2dPathIsLocked(path));
    RWASSERT(NULL != path);

    vcount = rwSListGetNumEntries((RwSList *)(path->curr->segments));
    pnode = (rt2dPathNode *)
        rwSListGetEntry((RwSList *)(path->curr->segments), vcount - 1);

    Rt2dPathCurveto(path, pnode->pos.x + x1, pnode->pos.y + y1,
                    pnode->pos.x + x2, pnode->pos.y + y2,
                    pnode->pos.x + x3, pnode->pos.y + y3);

    RWRETURN(path);
}

/****************************************************************************/

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathClose
 * is used to add a line segment to the specified path that
 * joins the current point with the first point defined for this
 * path. This operation thus forces the path to define a closed 2D
 * region.
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 *
 * \param path  Pointer to the path.
 * \return a pointer to the path if successful or NULL if there is an
 * error.
 * \see Rt2dPathMoveto
 * \see Rt2dPathLineto
 * \see Rt2dPathCurveto
 * \see Rt2dPathRLineto
 * \see Rt2dPathRCurveto
 * \see Rt2dPathCreate
 * \see Rt2dPathDestroy
 * \see Rt2dPathEmpty
 */
Rt2dPath           *
Rt2dPathClose(Rt2dPath * path)
{
    Rt2dPath           *curr;
    RwInt32             vcount;
    rt2dPathNode       *firstnode, *lastnode;
    RwV2d               first, normal;
    RwReal              length;

    RWAPIFUNCTION(RWSTRING("Rt2dPathClose"));
    RWASSERT(_rt2dPathIsLocked(path));
    RWASSERT(path);

    curr = path->curr;
    RWASSERT(curr);

    vcount = rwSListGetNumEntries((RwSList *)(curr->segments));

    firstnode = (rt2dPathNode *)
        rwSListGetArray((RwSList *)(curr->segments));
    lastnode = (rt2dPathNode *)
        rwSListGetEntry((RwSList *)(curr->segments),
                        vcount - 1);

    first = firstnode->pos;

    normal.x = first.y - lastnode->pos.y;
    normal.y = lastnode->pos.x - first.x;

#if (0)

    if ((normal.x != 0.0f) || (normal.y != 0.0f))
    {
        RwReal              length;
        rt2dPathNode       *pnode = (rt2dPathNode *)
            rwSListGetNewEntry(curr->segments,
                rwID_2DPLUGIN | rwMEMHINTDUR_EVENT);

        pnode->type = rt2dLINE;
        pnode->pos = first;

        RwV2dNormalizeMacro(length, &pnode->normal, &normal);
        pnode->dist = (pnode - 1)->dist + length;

        /* because we may have reallocated... */
        firstnode = rwSListGetArray(curr->segments);
        lastnode = pnode;
    }

#endif /* (0) */

    if ((normal.x != 0.0f) || (normal.y != 0.0f))
    {
        SubPathLineto2d(path->curr, &first);

        firstnode = (rt2dPathNode *)
            rwSListGetArray((RwSList *)(curr->segments));
        lastnode = (rt2dPathNode *)
            rwSListGetEntry((RwSList *)(curr->segments),
                            vcount);

    }

    RwV2dAdd(&normal, &firstnode->normal, &lastnode->normal);

    length = 1.0f / RwV2dDotProduct(&firstnode->normal, &normal);
    RwV2dScale(&lastnode->normal, &normal, length);

    /* rt2dMOVE normal now knowable */
    firstnode->normal = lastnode->normal;

    curr->closed = TRUE;

    RWRETURN(path);

}

/****************************************************************************/

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathRect
 * is used to add an outline rectangle to the specified path
 * that has the given position, width and height.
 *
 * Note that positions are defined in absolute coordinates and are
 * subject to the current transformation matrix (CTM).
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param path  Pointer to the path.
 * \param x  A RwReal value equal to the x-coordinate of the lower-left corner
 *       the rectangle.
 * \param y  A RwReal value equal to the y-coordinate of the lower-left corner
 *       the rectangle.
 * \param w  A RwReal value equal to the width of the rectangle.
 * \param h  A RwReal value equal to the height of the rectangle.
 * \return a pointer to the path if successful or NULL if there is an
 * error.
 * \see Rt2dPathRoundRect
 * \see Rt2dPathOval
 */
Rt2dPath           *
Rt2dPathRect(Rt2dPath * path, RwReal x, RwReal y, RwReal w, RwReal h)
{
    RWAPIFUNCTION(RWSTRING("Rt2dPathRect"));
    RWASSERT(_rt2dPathIsLocked(path));
    RWASSERT(NULL != path);

    Rt2dPathMoveto(path, x, y);

    Rt2dPathLineto(path, x + w, y);
    Rt2dPathLineto(path, x + w, y + h);
    Rt2dPathLineto(path, x, y + h);

#if (0)

    /* special case rectangle */
    pnode = (rt2dPathNode *) rwSListGetNewEntries(path->segments,
                           3, rwID_2DPLUGIN | rwMEMHINTDUR_EVENT);

    pnode->type = rt2dLINE;
    pnode->pos.x = x + w;
    pnode->pos.y = y;
    pnode->normal.x = 0.0f;
    pnode->normal.y = -1.0f;
    pnode->dist = w;

    pnode++;
    pnode->type = rt2dLINE;
    pnode->pos.x = x + w;
    pnode->pos.y = y + h;
    pnode->normal.x = 1.0f;
    pnode->normal.y = 0.0f;
    pnode->dist = w + h;

    pnode++;
    pnode->type = rt2dLINE;
    pnode->pos.x = x;
    pnode->pos.y = y + h;
    pnode->normal.x = 0.0f;
    pnode->normal.y = 1.0f;
    pnode->dist = 2 * w + h;

#endif /* (0) */

    Rt2dPathClose(path);

    RWRETURN(path);
}

/****************************************************************************/

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathRoundRect
 * is used to add an outline rectangle with rounded
 * corners to the specified path that has the given position, width,
 * height and radius of curvature of the corners. The radius must be
 * greater than zero.
 *
 * Note that positions are defined in absolute coordinates and are
 * subject to the current transformation matrix (CTM).
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param path  Pointer to the path.
 * \param x  A RwReal value equal to the x-coordinate of the lower-left corner
 *       the rectangle.
 * \param y  A RwReal value equal to the y-coordinate of the lower-left corner
 *       the rectangle.
 * \param w  A RwReal value equal to the width of the rectangle.
 * \param h  A RwReal value equal to the height of the rectangle.
 * \param radius  A RwReal value equal to the radius of the corners.
 * \return a pointer to the path if successful or NULL if there is an
 *        error.
 * \see Rt2dPathRect
 * \see Rt2dPathOval
 */
Rt2dPath           *
Rt2dPathRoundRect(Rt2dPath * path, RwReal x, RwReal y, RwReal w,
                  RwReal h, RwReal radius)
{
    RwReal              ctrlPos = (RwReal) (0.5522847498);

    RWAPIFUNCTION(RWSTRING("Rt2dPathRoundRect"));
    RWASSERT(_rt2dPathIsLocked(path));

    if (radius <= (RwReal) (0.0))
    {
        Rt2dPathRect(path, x, y, w, h);
    }
    else
    {
        RwBool              hstrut = radius * (RwReal) (2.0) < w;
        RwBool              vstrut = radius * (RwReal) (2.0) < h;

        Rt2dPathMoveto(path, x + radius, y);

        if (hstrut)
        {
            Rt2dPathLineto(path, x + w - radius, y);
        }

        Rt2dPathCurveto(path,
                        x + w - radius * ((RwReal) (1.0) - ctrlPos), y,
                        x + w, y + radius * ((RwReal) (1.0) - ctrlPos),
                        x + w, y + radius);

        if (vstrut)
        {
            Rt2dPathLineto(path, x + w, y + h - radius);
        }

        Rt2dPathCurveto(path, x + w,
                        y + h - radius * ((RwReal) (1.0) - ctrlPos),
                        x + w - radius * ((RwReal) (1.0) - ctrlPos),
                        y + h, x + w - radius, y + h);

        if (hstrut)
        {
            Rt2dPathLineto(path, x + radius, y + h);
        }

        Rt2dPathCurveto(path, x + radius * ((RwReal) (1.0) - ctrlPos),
                        y + h, x,
                        y + h - radius * ((RwReal) (1.0) - ctrlPos), x,
                        y + h - radius);

        if (vstrut)
        {
            Rt2dPathLineto(path, x, y + radius);
        }

        Rt2dPathCurveto(path, x,
                        y + radius * ((RwReal) (1.0) - ctrlPos),
                        x + radius * ((RwReal) (1.0) - ctrlPos), y,
                        x + radius, y);

        Rt2dPathClose(path);

    }

    RWRETURN(path);

}

/****************************************************************************/

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathOval
 * is used to add an outline oval to the specified path that
 * has the given position, width and height.
 *
 * Note that positions are defined in absolute coordinates and are
 * subject to the current transformation matrix (CTM).
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param path  Pointer to the path.
 * \param x  A RwReal value equal to the x-coordinate of the lower-left corner
 *       the oval.
 * \param y  A RwReal value equal to the y-coordinate of the lower-left corner
 *       the oval.
 * \param w  A RwReal value equal to the width of the oval.
 * \param h  A RwReal value equal to the height of the oval.
 * \return a pointer to the path if successful or NULL if there is an
 *       error.
 * \see Rt2dPathRect
 * \see Rt2dPathRoundRect
 */
Rt2dPath           *
Rt2dPathOval(Rt2dPath * path, RwReal x, RwReal y, RwReal w, RwReal h)
{
    const RwReal        ctrlPos = (RwReal) (0.5522847498);

    RWAPIFUNCTION(RWSTRING("Rt2dPathOval"));

    RWASSERT(NULL != path);
    RWASSERT(_rt2dPathIsLocked(path));

    Rt2dPathMoveto(path, x + w * (RwReal) (0.5), y);

    Rt2dPathCurveto(path,
                    x + w * (RwReal) (0.5) +
                    w * (RwReal) (0.5) * ctrlPos, y, x + w,
                    y + h * (RwReal) (0.5) -
                    h * (RwReal) (0.5) * ctrlPos, x + w,
                    y + h * (RwReal) (0.5));

    Rt2dPathCurveto(path, x + w,
                    y + h * (RwReal) (0.5) +
                    h * (RwReal) (0.5) * ctrlPos,
                    x + w * (RwReal) (0.5) +
                    w * (RwReal) (0.5) * ctrlPos, y + h,
                    x + w * (RwReal) (0.5), y + h);

    Rt2dPathCurveto(path,
                    x + w * (RwReal) (0.5) -
                    w * (RwReal) (0.5) * ctrlPos, y + h, x,
                    y + h * (RwReal) (0.5) +
                    h * (RwReal) (0.5) * ctrlPos, x,
                    y + h * (RwReal) (0.5));

    Rt2dPathCurveto(path, x,
                    y + h * (RwReal) (0.5) -
                    h * (RwReal) (0.5) * ctrlPos,
                    x + w * (RwReal) (0.5) -
                    w * (RwReal) (0.5) * ctrlPos, y,
                    x + w * (RwReal) (0.5), y);

    Rt2dPathClose(path);

    RWRETURN(path);

}

/****************************************************************************/

Rt2dPath           *
_rt2dScratchPath(void)
{
    RWFUNCTION(RWSTRING("_rt2dScratchPath"));

    Rt2dPathEmpty(Rt2dGlobals.scratch);

    RWRETURN(Rt2dGlobals.scratch);
}

/****************************************************************************/

static void
Bisect2d(const RtCubicBezier control,
         RtCubicBezier left, RtCubicBezier right)
{
    RwV2d               vTemp, vTemp2;

    RWFUNCTION(RWSTRING("Bisect2d"));

    RwV2dAdd(&vTemp2, &control[1], &control[2]);
    RwV2dScale(&vTemp2, &vTemp2, (RwReal) (0.5));

    RwV2dAdd(&vTemp, &control[0], &control[1]);
    RwV2dScale(&left[1], &vTemp, (RwReal) (0.5));

    RwV2dAdd(&vTemp, &control[3], &control[2]);
    RwV2dScale(&right[2], &vTemp, (RwReal) (0.5));

    RwV2dAdd(&vTemp, &left[1], &vTemp2);
    RwV2dScale(&left[2], &vTemp, (RwReal) (0.5));

    RwV2dAdd(&vTemp, &right[2], &vTemp2);
    RwV2dScale(&right[1], &vTemp, (RwReal) (0.5));

    RwV2dAdd(&vTemp, &left[2], &right[1]);
    RwV2dScale(&left[3], &vTemp, (RwReal) (0.5));

    RwV2dAssign(&left[0], &control[0]);
    RwV2dAssign(&right[0], &left[3]);
    RwV2dAssign(&right[3], &control[3]);

    RWRETURNVOID();
}

/****************************************************************************/
static void
Flatten2d(Rt2dPath * const flat, const RtCubicBezier control, int depth)
{
    RwBool              terminate;

    RWFUNCTION(RWSTRING("Flatten2d"));
    RWASSERT(_rt2dPathIsLocked(flat));

    terminate = (--depth < 0);
    if (terminate)
    {
        SubPathLineto2d(flat, &control[3]);
    }
    else
    {
        Rt2dIsFlatMacro(terminate, control);
        if (terminate)
        {
            SubPathLineto2d(flat, &control[3]);
        }
        else
        {
            RtCubicBezier       left, right;

            Bisect2d(control, left, right);

            Flatten2d(flat, left, depth);
            Flatten2d(flat, right, depth);
        }
    }

    RWRETURNVOID();
}

/****************************************************************************/

Rt2dPath           *
_rt2dSubPathFlatten(Rt2dPath * flat, const Rt2dPath * path)
{
    rt2dPathNode       *pnode, *qnode;
    RwInt32             vcount, i;

    RWFUNCTION(RWSTRING("_rt2dSubPathFlatten"));
    RWASSERT(_rt2dPathIsLocked(flat));

    RWASSERT(NULL != path);

    /* get tolerance in object space */
    Rt2dGlobals.sqTolerance = _rt2dGetTolerance();

    pnode = _rt2dPathGetSegmentArray(path);
    vcount = _rt2dPathGetNumSegment(path);

    for (i = 0; i < vcount; i++, pnode++)
    {
        if (pnode->type == rt2dCURVE)
        {
            RwV2d               control[4];

            control[0] = (pnode - 1)->pos;
            control[1] = (pnode + 0)->pos;
            control[2] = (pnode + 1)->pos;
            control[3] = (pnode + 2)->pos;
            Flatten2d(flat, control, Rt2dGlobals.flatDepth);
            i += 2;
            pnode += 2;
        }
        else if (pnode->type == rt2dLINE)
        {
            SubPathLineto2d(flat, &pnode->pos);
        }
        else
        {
            qnode = (rt2dPathNode *)
                rwSListGetNewEntry((RwSList *)(flat->segments),
                           rwID_2DPATH | rwMEMHINTDUR_EVENT);
            *qnode = *pnode;
        }
    }

    if (path->closed)
    {
        Rt2dPathClose(flat);
    }

    RWRETURN(flat);
}

/****************************************************************************/
Rt2dBBox           *
_rt2dFlatSubPathExtendBBox(const Rt2dPath * path, Rt2dBBox * bbox)
{
    rt2dPathNode       *pnode;
    RwInt32             vcount, i;

    RWFUNCTION(RWSTRING("_rt2dFlatSubPathExtendBBox"));

    RWASSERT(NULL != path);

    pnode = _rt2dPathGetSegmentArray(path);
    vcount = _rt2dPathGetNumSegment(path);

    /* determine bbox of subpath */
    for (i = 0; i < vcount; i++, pnode++)
    {
        if (pnode->pos.x < bbox->x)
        {
            bbox->x = pnode->pos.x;
        }
        else if (pnode->pos.x > bbox->w)
        {
            bbox->w = pnode->pos.x;
        }

        if (pnode->pos.y < bbox->y)
        {
            bbox->y = pnode->pos.y;
        }
        else if (pnode->pos.y > bbox->h)
        {
            bbox->h = pnode->pos.y;
        }
    }

    RWRETURN(bbox);
}

/****************************************************************************/

static Rt2dBBox    *
PathExtendBBox2d(const Rt2dPath * path, Rt2dBBox * bbox)
{
    const Rt2dPath     *target;

    RWFUNCTION(RWSTRING("PathExtendBBox2d"));

    RWASSERT(NULL != path);
    RWASSERT(NULL != bbox);

    for (target = path; NULL != target; target = target->next)
    {
        if (!target->flat)
        {
            Rt2dPath           *const flat = _rt2dScratchPath();

            _rt2dSubPathFlatten(flat, target);
            _rt2dFlatSubPathExtendBBox(flat, bbox);
        }
        else
        {
            _rt2dFlatSubPathExtendBBox(target, bbox);
        }
    }

    RWRETURN(bbox);
}

/****************************************************************************/

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathGetBBox
 * is used to determine the parameters of an axis-aligned
 * bounding-box that completely encloses the specified path. The box
 * determined by this function is the box before the CTM has transformed
 * the path.
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param path  Pointer to the path.
 * \param bbox  Pointer to a Rt2dBBox value that will receive the path's
 *       box.
 * \return a pointer to the path if successful or NULL if there is an
 * error.
 * \see Rt2dVisible
 */
const Rt2dPath     *
Rt2dPathGetBBox(const Rt2dPath * path, Rt2dBBox * bbox)
{
    rt2dPathNode       *pnode;

    RWAPIFUNCTION(RWSTRING("Rt2dPathGetBBox"));

    RWASSERT(path);
    RWASSERT(bbox);

    pnode = (rt2dPathNode *)_rt2dPathGetSegmentArray(path);
    bbox->x = bbox->w = pnode[0].pos.x;
    bbox->y = bbox->h = pnode[0].pos.y;

    PathExtendBBox2d(path, bbox);

    bbox->w -= bbox->x;
    bbox->h -= bbox->y;

    RWRETURN(path);
}

/****************************************************************************/

Rt2dPath           *
_rt2dPathOptimize(Rt2dPath * path)
{
    RWFUNCTION(RWSTRING("_rt2dPathOptimize"));

    /* NULL path is valid */
    if (NULL != path)
    {
        RWASSERT(_rt2dPathIsLocked(path));
        path->next = _rt2dPathOptimize(path->next);

        /* delete degenerate subpaths */
        if (rwSListGetNumEntries((RwSList *)(path->segments)) == 1)
        {
            path = SubPathDestroy2d(path);
        }
    }

    RWRETURN(path);
}

#if (defined __ICL)            /* Intel compiler optimises this bit badly */
#pragma optimize ("", off)
#endif

/****************************************************************************/
static Rt2dPath    *
SubPathInset2d(Rt2dPath * path, RwReal inset)
{
    rt2dPathNode       *pnode, *nextcorner, *firstnode;
    RwInt32             vcount, i;
    RwV2d               avgnormal;
    RwV3d               pos;

    RWFUNCTION(RWSTRING("SubPathInset2d"));
    RWASSERT(NULL != path);

    RT2DSUBPATHPRINT(path);

    firstnode = pnode =
        _rt2dPathGetSegmentArray(path);
    vcount = _rt2dPathGetNumSegment(path);

    /* segment normal */
    nextcorner = pnode;
    if (!path->closed)
    {
        /* NB first path node has no normal iff path is not closed */
        nextcorner++;
    }

    for (i = 1; i < vcount; i++, pnode++)
    {
        rt2dPathNode       *corner;
        RwReal              dot;

        /* segment normal */
        corner = nextcorner;
        nextcorner = pnode + 1;

        /* corner normal */
        RwV2dAdd(&avgnormal, &corner->normal, &nextcorner->normal);
        dot = RwV2dDotProduct(&nextcorner->normal, &avgnormal);
        dot = inset / dot;
        RwV2dScale(&avgnormal, &avgnormal, dot);

        RwV2dSub((RwV2d *) & pos, &pnode->pos, &avgnormal);
    }

/*
    if (path->closed)
    {
        pnode->pos = firstnode->pos;
    }
    else
    {
        RwV2dScale(&avgnormal, &nextcorner->normal, inset);
        RwV2dSub(&pos, &pnode->pos, &avgnormal);
    }
 */
    RWRETURN(path);
}

#if (defined __ICL)            /* Intel compiler optimises this bit badly */
#pragma optimize ("", on)
#endif

/****************************************************************************/

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathInset
 * is used to scale the specified path such that the new
 * path is parallel to the original at every point. The perpendicular
 * distance between the new and old paths is given by the inset; positive
 * values result in a new path contained inside the original and vice
 * versa.
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param path  Pointer to the path.
 * \param inset  A RwReal value equal to the inset distance.
 * \return a pointer to the path if successful or NULL if there is an
 * error.
 */
Rt2dPath           *
Rt2dPathInset(Rt2dPath * path, RwReal inset)
{
    Rt2dPath           *target;

    RWAPIFUNCTION(RWSTRING("Rt2dPathInset"));

    RWASSERT(path);

    for (target = path; NULL != target; target = target->next)
    {
        target->inset = inset;

        target = SubPathInset2d(target, inset);
    }

    RWRETURN(path);
}

/****************************************************************************/

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathFlatten
 * is used to convert the specified path to use only
 * straight line segments. For the curved portions of the path, the
 * degree of flattening may be controlled with a factor specified using
 * the function \ref Rt2dDeviceSetFlat, which has a default value of 0.5. This
 * factor specifies the maximum pixel error that is tolerated as the
 * curves are tessellated.
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param path  Pointer to the path.
 * \return a pointer to the path if successful or NULL if there is an
 * error.
 * \see Rt2dDeviceSetFlat
 */
Rt2dPath           *
Rt2dPathFlatten(Rt2dPath * path)
{
    Rt2dPath           *target;

    RWAPIFUNCTION(RWSTRING("Rt2dPathFlatten"));
    RWASSERT(_rt2dPathIsLocked(path));
    RWASSERT(path);

    for (target = path; NULL != target; target = target->next)
    {
        if (!target->flat)
        {
            Rt2dPath           *flat;

            flat = _rt2dScratchPath();
            _rt2dSubPathFlatten(flat, target);
            SubPathEmpty2d(target);
            SubPathCopy2d(target, flat);
        }
    }

    RWRETURN(path);
}

/****************************************************************************/

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathDestroy
 * is used to destroy the specified path. All paths
 * created by an application must be destroyed before the application
 * closes down.
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param path  Pointer to the path.
 * \return TRUE if successful or FALSE if there is an error
 * \see Rt2dPathCreate
 * \see Rt2dOpen
 * \see Rt2dClose
 */
RwBool
Rt2dPathDestroy(Rt2dPath * path)
{
    RwBool              result;

    RWAPIFUNCTION(RWSTRING("Rt2dPathDestroy"));

    /* NULL path is valid */
    result = (NULL != path);
    if (result)
    {
        Rt2dPathDestroy(SubPathDestroy2d(path));
    }

    RWRETURN(result);
}

/****************************************************************************/
static void
rt2dPathDestruct(Rt2dPath *path)
{
    RWFUNCTION(RWSTRING("rt2dPathDestruct"));
    RWASSERT(path);

    /* NULL path is valid */
    if(_rt2dPathIsLocked(path))
    {
        rwSListDestroy((RwSList *)(path->segments));
    }
    else if(NULL != path->segments)
    {
        RwFree(path->segments);
    }

    path->segments = NULL;
    path->numSegment = 0;

    if(path->next)
    {
        rt2dPathDestruct(path->next);
        Rt2dPathDestroy(path->next);
        path->next = (Rt2dPath *)NULL;
    }

    RWRETURNVOID();
}

/****************************************************************************/

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathCreate
 * is used to create a new path. The path is initially
 * empty and locked but may be constructed using any combination of the
 * available primitive line drawing procedures. A path may be closed or open,
 * connected or disjoint (that is, have multiple boundaries), depending
 * on how it has been constructed.
 * The path should be unlocked after editing and before rendering it.
 *
 * Note that the current point for a newly created path is undefined. To
 * establish an initial current point use the function \ref Rt2dPathMoveto.
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \return a pointer to the new path if successful or NULL if there is an
 * error.
 * \see Rt2dPathDestroy
 * \see Rt2dPathMoveto
 * \see Rt2dPathLineto
 * \see Rt2dPathCurveto
 * \see Rt2dPathRLineto
 * \see Rt2dPathRCurveto
 * \see Rt2dPathRect
 * \see Rt2dPathRoundRect
 * \see Rt2dPathOval
 * \see Rt2dPathLock
 * \see Rt2dPathUnlock
 */
Rt2dPath           *
Rt2dPathCreate(void)
{
    Rt2dPath           *path;

    RWAPIFUNCTION(RWSTRING("Rt2dPathCreate"));

    path = (Rt2dPath *) RwFreeListAlloc(Rt2dGlobals.pathFreeList,
                                rwID_2DPATH | rwMEMHINTDUR_EVENT);
    if ((Rt2dPath *)NULL != path)
    {
        path->segments = NULL;
        path->numSegment = 0;
        path->closed = FALSE;
        path->flat = TRUE;
        path->next = (Rt2dPath *)NULL;
        path->curr = path;
        path->inset = 0;

        Rt2dPathLock(path);
    }

    RWRETURN(path);
}

/****************************************************************************/

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathLock
 * is used to lock a new path. This should be done before any modification
 * append to the path.
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \return a pointer to the new path if successful or NULL if there is an
 * error.
 * \see Rt2dPathCreate
 * \see Rt2dPathUnlock
 * \see Rt2dPathMoveto
 * \see Rt2dPathLineto
 * \see Rt2dPathCurveto
 * \see Rt2dPathRLineto
 * \see Rt2dPathRCurveto
 * \see Rt2dPathRect
 * \see Rt2dPathRoundRect
 * \see Rt2dPathOval
 */
Rt2dPath *
Rt2dPathLock(Rt2dPath *path)
{
    Rt2dPath *result = path;
    RWAPIFUNCTION(RWSTRING("Rt2dPathLock"));

   if(NULL != path && !_rt2dPathIsLocked(path))
   {
        RwSList *list;
        list = _rwSListCreate(sizeof(rt2dPathNode),
            rwID_2DPATH | rwMEMHINTDUR_EVENT);

        if(list)
        {
            rt2dPathNode       *pnode;
            rt2dPathNode       *dst;
            RwInt32             vcount;

            pnode = _rt2dPathGetSegmentArray(path);
            vcount = _rt2dPathGetNumSegment(path);

            if(vcount)
            {
                RWASSERT(pnode);

                dst = (rt2dPathNode *)
                    _rwSListGetNewEntries((RwSList *)list,
                                          vcount,
                                          rwID_2DPATH |
                                          rwMEMHINTDUR_EVENT);
                RWASSERT(dst);
                if(dst)
                {
                    memcpy(dst,pnode,sizeof(rt2dPathNode)*vcount);
                }

                RwFree(pnode);
            }

            path->numSegment = -1;
            path->segments = list;
            Rt2dPathLock(path->next);

        }
        else
        {
            result = (Rt2dPath *)NULL;
        }
   }



    RWRETURN(result);
}
/****************************************************************************/

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathUnlock
 * is used to unlock a previously locked path. This should be done after any modification
 * append to the path, not doing so can highly degrade performences.
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param path  Pointer to the path.
 * \return a pointer to the new path if successful or NULL if there is an
 * error.
 * \see Rt2dPathCreate
 * \see Rt2dPathLock
 * \see Rt2dPathMoveto
 * \see Rt2dPathLineto
 * \see Rt2dPathCurveto
 * \see Rt2dPathRLineto
 * \see Rt2dPathRCurveto
 * \see Rt2dPathRect
 * \see Rt2dPathRoundRect
 * \see Rt2dPathOval
 */

Rt2dPath *
Rt2dPathUnlock(Rt2dPath *path)
{
    Rt2dPath *result = path;
    RWAPIFUNCTION(RWSTRING("Rt2dPathUnlock"));

    if(NULL != path && _rt2dPathIsLocked(path))
    {
        RwInt32 vcount = rwSListGetNumEntries((RwSList *)(path->segments));
        rt2dPathNode       *dst;

        dst = (rt2dPathNode *)
            _rwSListToArray((RwSList *)(path->segments));


        /* Not needed as _rwSListToArray is destroying the list it self */
        /* _rwSListDestroy(path->segments);  */
        path->segments = dst;
        path->numSegment = vcount;

        Rt2dPathUnlock(path->next);
    }

    RWRETURN(result);
}

/****************************************************************************/

void
_rt2dPathClose(void)
{
    RWFUNCTION(RWSTRING("_rt2dPathClose"));

    Rt2dPathDestroy(Rt2dGlobals.scratch);

    RwFreeListDestroy(Rt2dGlobals.pathFreeList);

    Rt2dGlobals.pathFreeList = (RwFreeList *)NULL;

    RWRETURNVOID();
}

static RwInt32 _rt2dPathFreeListBlockSize = 32,
               _rt2dPathFreeListPreallocBlocks = 1;
static RwFreeList _rt2dPathFreeList;

/**
 * \ingroup rt2d
 * \ref Rt2dPathSetFreeListCreateParams allows the developer to specify
 * how many \ref Rt2dPath s to preallocate space for.
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
Rt2dPathSetFreeListCreateParams( RwInt32 blockSize, RwInt32 numBlocksToPrealloc )
{
    /* cannot use debugging macros since the debugger is not initialized before RwEngineInit */
    /*
    RWAPIFUNCTION( RWSTRING( "Rt2dPathSetFreeListCreateParams" ) );
    */

    _rt2dPathFreeListBlockSize = blockSize;
    _rt2dPathFreeListPreallocBlocks = numBlocksToPrealloc;

    /*
    RWRETURNVOID();
    */
}

/****************************************************************************/

RwBool
_rt2dPathOpen(void)
{
    RWFUNCTION(RWSTRING("_rt2dPathOpen"));

    Rt2dGlobals.pathFreeList =
        RwFreeListCreateAndPreallocateSpace(sizeof(Rt2dPath),
        _rt2dPathFreeListBlockSize, sizeof(RwInt32),
        _rt2dPathFreeListPreallocBlocks,
        &_rt2dPathFreeList, rwID_2DPATH | rwMEMHINTDUR_GLOBAL);

    Rt2dGlobals.scratch = Rt2dPathCreate();
    Rt2dPathLock(Rt2dGlobals.scratch);
    RWRETURN(TRUE);
}

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathStreamGetSize is used to  determine the size in bytes
 * of the binary representation of the given path. This is used in
 * the binary chunk header to indicate the size of the chunk. The size does
 * include the size of the chunk header.
 *
 *
 * \param path Pointer to the path.
 *
 * \return Returns a RwUInt32 value equal to the chunk size (in bytes) of
 * the path or zero if there is an error.
 *
 * \see Rt2dPathStreamRead
 * \see Rt2dPathStreamWrite
 *
 */
RwUInt32
Rt2dPathStreamGetSize(Rt2dPath *path)
{
    RwUInt32            size = 0;
    Rt2dPath            *wrkPath;

    RWAPIFUNCTION(RWSTRING("Rt2dPathStreamGetSize"));

    if (NULL == path)
    {
        RWRETURN(0);
    }

    // wrkPath = path->curr;
    wrkPath = path;

    while( NULL != wrkPath)
    {
        size += sizeof(_rt2streamPath);
        size += _rt2dPathGetNumSegment(wrkPath)*sizeof(rt2dPathNode);

        size += rwCHUNKHEADERSIZE;
        wrkPath = wrkPath->next;
    }


    RWRETURN(size);
}

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathStreamWrite is used to write the specified path to the
 * given binary stream.
 * Note that the stream will have been opened prior to this function call.
 *
 * \param path  Pointer to the path.
 * \param stream  Pointer to the stream.
 *
 * \return Returns pointer to the path if successful or NULL if
 * there is an error.
 *
 * \see Rt2dPathStreamRead
 * \see Rt2dPathStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 *
 */
Rt2dPath *
Rt2dPathStreamWrite(Rt2dPath *path, RwStream *stream)
{
    RwUInt32            size;
    RwInt32             numSegment;
    _rt2streamPath      sPath;
    Rt2dPath            *wrkPath;
    rt2dPathNode        *wrkSrcNode;
    rt2dPathNode        *wrkDstNode;
    rt2dPathNode        *WriteNode;
    RwInt32             i;

    RWAPIFUNCTION(RWSTRING("Rt2dPathStreamWrite"));

    if (NULL == path || NULL == stream)
    {
        RWRETURN((Rt2dPath *)NULL);
    }

    wrkPath = path;
    size = Rt2dPathStreamGetSize(wrkPath);

    if (!RwStreamWriteChunkHeader(stream, rwID_2DPATH, size))
    {
        RWRETURN((Rt2dPath *)NULL);
    }

    /* Set and Write Header */
    sPath.version = FILE_CURRENT_VERSION;
    numSegment = _rt2dPathGetNumSegment(wrkPath);
    sPath.numSegment = numSegment;
    sPath.flag = 0;

    if( TRUE == wrkPath->closed )
    {
        sPath.flag |= RT2DSTREAMPATHFLAGCLOSED;
    }

    if( TRUE == wrkPath->flat )
    {
        sPath.flag |= RT2DSTREAMPATHFLAGFLAT;
    }

    if( NULL != wrkPath->next )
    {
        sPath.flag |= RT2DSTREAMPATHFLAGGOTSUBPATH;
    }

    sPath.inset = wrkPath->inset;
    (void)RwMemRealToFloat32(&sPath.inset, sizeof(sPath.inset));
    (void)RwMemLittleEndian32(&sPath, sizeof(sPath));

    if (!RwStreamWrite(stream, &sPath, sizeof(_rt2streamPath)))
    {
        RWRETURN((Rt2dPath *)NULL);
    }

    /* Set and write PathNodes*/
    if( numSegment )
    {
        wrkSrcNode = _rt2dPathGetSegmentArray(wrkPath);;
        WriteNode = (rt2dPathNode*)RwMalloc(sizeof(rt2dPathNode)*numSegment,
                                          rwID_2DPATH | rwMEMHINTDUR_EVENT);
        if( NULL==WriteNode )
        {
            RWRETURN((Rt2dPath *)NULL);
        }

        memcpy(WriteNode,wrkSrcNode,sizeof(rt2dPathNode)*numSegment);

        wrkDstNode = WriteNode;

        for(i=0;i<numSegment;i++)
        {
            (void)RwMemRealToFloat32(&wrkDstNode->pos.x, sizeof(wrkDstNode->pos.x));
            (void)RwMemRealToFloat32(&wrkDstNode->pos.y, sizeof(wrkDstNode->pos.y));
            (void)RwMemRealToFloat32(&wrkDstNode->normal.x, sizeof(wrkDstNode->normal.x));
            (void)RwMemRealToFloat32(&wrkDstNode->normal.y, sizeof(wrkDstNode->normal.y));
            (void)RwMemRealToFloat32(&wrkDstNode->dist, sizeof(wrkDstNode->dist));

            wrkDstNode++;
        }

        (void)RwMemLittleEndian32(WriteNode, sizeof(rt2dPathNode)*numSegment);

        /* No segments, no write */

        if (!RwStreamWrite(stream, WriteNode, sizeof(rt2dPathNode)*numSegment))
        {
            RWRETURN((Rt2dPath *)NULL);
        }

        RwFree(WriteNode);
    }

    if(wrkPath->next)
    {
        wrkPath = Rt2dPathStreamWrite(wrkPath->next,stream);
    }

    if(wrkPath)
    {
        RWRETURN(path);                 /* Return the one submited rather than the actual main path */
    }

    RWRETURN((Rt2dPath *)NULL);
}

static Rt2dPath *
rt2dPathStreamRead_V0x01(Rt2dPath *path, _rt2streamPath *sPath, RwStream *stream)
{
    Rt2dPath           *wrkPath;
    rt2dPathNode        *nodes, *wrkNodes;
    RwInt32             i;

    RWFUNCTION(RWSTRING("rt2dPathStreamRead_V0x01"));
    RWASSERT(stream);
    RWASSERT(sPath);
    RWASSERT(path);

    rt2dPathDestruct(path);

    path->curr = path;

    path->numSegment = sPath->numSegment;

    path->closed = (sPath->flag & RT2DSTREAMPATHFLAGCLOSED) ? TRUE : FALSE;
    path->flat = (sPath->flag & RT2DSTREAMPATHFLAGFLAT) ? TRUE : FALSE;
    path->inset = sPath->inset;

    nodes = (rt2dPathNode *)
        RwMalloc(sizeof(rt2dPathNode) * path->numSegment,
        rwID_2DPATH | rwMEMHINTDUR_EVENT) ;
    if( NULL == nodes )
    {
        RWRETURN((Rt2dPath *)NULL);
    }

    if( path->numSegment )
    {
        if (RwStreamRead(stream, nodes, sizeof(rt2dPathNode) * path->numSegment)
            != (sizeof(rt2dPathNode) * path->numSegment))
        {
            RWRETURN((Rt2dPath *)NULL);
        }

        (void)RwMemLittleEndian32(nodes, sizeof(rt2dPathNode) * path->numSegment);
    }

    wrkNodes = nodes;
    for(i=0;i<sPath->numSegment;i++)
    {
        (void)RwMemRealToFloat32(&wrkNodes->pos.x, sizeof(wrkNodes->pos.x));
        (void)RwMemRealToFloat32(&wrkNodes->pos.y, sizeof(wrkNodes->pos.y));
        (void)RwMemRealToFloat32(&wrkNodes->normal.x, sizeof(wrkNodes->normal.x));
        (void)RwMemRealToFloat32(&wrkNodes->normal.y, sizeof(wrkNodes->normal.y));
        (void)RwMemRealToFloat32(&wrkNodes->dist, sizeof(wrkNodes->dist));

        wrkNodes++;
    }

    path->segments = nodes;

    if( sPath->flag & RT2DSTREAMPATHFLAGGOTSUBPATH )
    {
        if( RwStreamFindChunk(stream, rwID_2DPATH,
                               (RwUInt32 *)NULL, (RwUInt32 *)NULL) )
        {
            path->next = Rt2dPathStreamRead(stream);
        }
        else
        {
            RWRETURN((Rt2dPath*)NULL);
        }

    }

    wrkPath = path->next;
    while(NULL != wrkPath)
    {
        wrkPath->curr = path;
        wrkPath = wrkPath->next;
    }

    RWRETURN(path);
}

Rt2dPath *
_rt2dPathStreamReadTo(Rt2dPath *path, RwStream *stream)
{
    _rt2streamPath      sPath;
    RWFUNCTION(RWSTRING("_rt2dPathStreamReadTo"));


    /* Read the Path header */
    if (RwStreamRead(stream, &sPath, sizeof(sPath)) != sizeof(sPath))
    {
        RWRETURN((Rt2dPath *)NULL);
    }

    /* Convert it */
    (void)RwMemNative32(&sPath, sizeof(sPath));

    (void)RwMemFloat32ToReal(&sPath.inset, sizeof(sPath.inset));


    RWASSERT(sPath.version <= FILE_LAST_SUPPORTED_VERSION && sPath.version >= FILE_CURRENT_VERSION);

    switch(sPath.version)
    {
        case  0x01:                     /* FILE_CURRENT_VERSION */
            path = rt2dPathStreamRead_V0x01(path,&sPath,stream);
            break;
        default:
            RWMESSAGE(((RWSTRING("Rt2dPath Unsupported Version %d\n")), sPath.version));
            break;
    }

    RWRETURN(path);
}


/**
 * \ingroup rt2dpath
 * \ref Rt2dPathStreamRead is used to read a path object from the
 * specified binary stream. Note that prior to this function call a
 * path chunk must be found in the stream using the
 * \ref RwStreamFindChunk API function.
 *
 * The sequence to locate and read a path from a binary stream is
 * as follows:
 * \code
   RwStream *stream;
   Rt2dPath *newPath;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_2DPATH, NULL, NULL) )
       {
           newPath = Rt2dPathStreamRead(stream);
       }

       RwStreamClose(stream, NULL);
   }
   \endcode
 *
 * \param stream  Pointer to the stream.
 *
 * \return Returns pointer to the path if successful or NULL if there is an error.
 *
 * \see Rt2dPathStreamWrite
 * \see Rt2dPathStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 */
Rt2dPath *
Rt2dPathStreamRead(RwStream *stream)
{
    Rt2dPath           *path = Rt2dPathCreate();
    Rt2dPath            *result = (Rt2dPath *) NULL;
    RWAPIFUNCTION(RWSTRING("Rt2dPathStreamRead"));

    result = _rt2dPathStreamReadTo(path,stream);

    if(NULL == result)
    {
        Rt2dPathDestroy(path);
    }

    RWRETURN(result);
}

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathCreateMorphResultPath is used to create a
 * result path for a morphing operation. The created path will be used as
 * a result for morph animation, allowing to apply geometrics modification
 * of the path without the need to keep it locked
 *
 * \param path  Pointer to source path
 *
 * \return Returns pointer to the created path if successful or NULL if there is an error.
 *
 * \see Rt2dPathMorph
 * \see Rt2dPathLock
 * \see Rt2dPathUnlock
 */
Rt2dPath *
Rt2dPathCreateMorphResultPath(Rt2dPath *path)
{
    Rt2dPath           *result = (Rt2dPath *) NULL;

    RWAPIFUNCTION(RWSTRING("Rt2dPathCreateMorphResultPath"));

    if(NULL == path)
    {
        RWRETURN((Rt2dPath *)NULL);
    }

    result = Rt2dPathCreate();

    if(NULL == result)
    {
        RWRETURN((Rt2dPath *)NULL);
    }

    Rt2dPathLock(result);

    Rt2dPathCopy(result,path);

    Rt2dPathUnlock(result);


    RWRETURN(result);
}

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathMorph is used to apply a morphing animation to two path.
 * the result of this operation will be stored in a result path, created using
 * \ref Rt2dPathCreateMorphResultPath. An alpha value, indicating the degree
 * of morphing between the source and the destination path should be provided.
 * \li if Alpha = 0, the result path will be equal to the source path
 * \li if Alpha = 1, the result path will be equal to the destination path
 *
 * \param result Pointer to the result path
 * \param source  Pointer to the source path
 * \param destination  Pointer to the destination path
 * \param alpha  an RwReal value indicating the degree of morphing
 *
 * \return Returns pointer to the result path if successful or NULL if there is an error.
 *
 * \see Rt2dPathCreateMorphResultPath
 */
Rt2dPath *
Rt2dPathMorph(Rt2dPath *result, Rt2dPath *source, Rt2dPath *destination, RwReal alpha)
{
    rt2dPathNode       *dstNode, *srcNode, *resNode;
    RwInt32             dstCount, srcCount, resCount;
    RWAPIFUNCTION(RWSTRING("Rt2dPathMorph"));

    RWASSERT(result);
    RWASSERT(source);
    RWASSERT(destination);

    dstCount = _rt2dPathGetNumSegment(destination);
    srcCount = _rt2dPathGetNumSegment(source);
    resCount = _rt2dPathGetNumSegment(result);

    RWASSERT(resCount == dstCount);
    RWASSERT(srcCount == dstCount);

    dstNode = _rt2dPathGetSegmentArray(destination);
    srcNode = _rt2dPathGetSegmentArray(source);
    resNode = _rt2dPathGetSegmentArray(result);

    if(1.0f == alpha)
    {
        memcpy(resNode, dstNode, sizeof(rt2dPathNode) * resCount);
    }
    else if ( 0.0f == alpha)
    {
        memcpy(resNode, srcNode, sizeof(rt2dPathNode) * resCount);
    }
    else
    {
        RwInt32 i;
        RwV2d tmp;

        for ( i=0;i<resCount;i++ )
        {
/*            resNode.type; no change        */
            RwV2dSub (&tmp, &dstNode->pos, &srcNode->pos);
            RwV2dScale(&tmp,&tmp,alpha);
            RwV2dAdd(&resNode->pos,&srcNode->pos,&tmp);

            RwV2dSub (&tmp, &dstNode->normal, &srcNode->normal);
            RwV2dScale(&tmp,&tmp,alpha);
            RwV2dAdd(&resNode->normal,&srcNode->normal,&tmp);

            resNode->dist = srcNode->dist + (dstNode->dist - srcNode->dist)*alpha;

            dstNode++;
            srcNode++;
            resNode++;
        }
    }


    if( NULL != result->next)
    {
        if( NULL == Rt2dPathMorph(result->next, source->next, destination->next, alpha) )
        {
            RWRETURN((Rt2dPath *)NULL);
        }

    }

    RWRETURN(result);
}

#define p  point
#define p1 nodeList->pos
#define p2 nodeListb->pos

static RwBool
_RwV2dIsInPathAlgo1(RwV2d *point, Rt2dPath *path)
{
    rt2dPathNode *nodeList, *nodeListb;
    RwInt32 i;
    RwReal  xinter;
    RwInt32 counter = 0;

    RWFUNCTION(RWSTRING("_RwV2dIsInPathAlgo1"));

    nodeList = _rt2dPathGetSegmentArray(path);
    nodeListb = nodeList;

    for(i=0;i<_rt2dPathGetNumSegment(path)-1;i++)
    {
        nodeListb++;

        if( p->y > RwRealMin2(p1.y,p2.y))
        {
            if( p->y <= RwRealMax2(p1.y,p2.y))
            {
                if ( p->x <= RwRealMax2(p1.x,p2.x))
                {
                    if ( p1.y != p2.y )
                    {
                        xinter = (p->y-p1.y)*(p2.x-p1.x)/(p2.y-p1.y)+p1.x;
                        if( p1.x == p2.x || p->x <= xinter)
                        {
                            counter++;
                        }
                    }
                }
            }
        }

        nodeList++;
    }


    RWRETURN((counter % 2)!=0);
}

/**
 * \ingroup rt2drwv2d
 * \ref RwV2dIsInPath is used to test if a point is inside a path.
 * the path should be a closed flat path
 *
 * \param point Pointer to the tested point
 * \param path Pointer to the path
 *
 * \return Returns TRUE if the point is inside the path or TRUE if it is not.
 *
 * \see Rt2dPathCreateMorphResultPath
 */
RwBool
RwV2dIsInPath(RwV2d *point, Rt2dPath *path)
{
    RwBool result = FALSE;

    RWAPIFUNCTION(RWSTRING("RwV2dIsInPath"));

    RWASSERT(point);
    RWASSERT(path);
    RWASSERT(TRUE == path->closed);
    /* RWASSERT(TRUE == path->flat); */

    result = _RwV2dIsInPathAlgo1(point, path);

    RWRETURN(result);
}

/**
 * \ingroup rt2drwv2d
 * \ref RwV2dInvertTransform is used to transform a point specified in screen space
 * ( 0.0f -> 1.0f  on both axes, origin in Top Left corner )
 * into the current CTM coordinate space
 *
 * \param pointOut Pointer to the transformed point
 * \param pointIn Pointer to the point to transform
 *
 * \return Returns the pointer to the transformed point on succes, or NULL on error.
 *
 * \see Rt2dPathCreateMorphResultPath
 */
RwV2d *
RwV2dInvertTransform(RwV2d *pointOut, RwV2d *pointIn)
{
    RwV3d rpoint;
    RwMatrix mtx;
    RWAPIFUNCTION(RWSTRING("RwV2dInvertTransform"));
    RWASSERT(pointOut);
    RWASSERT(pointIn);

    memcpy(&rpoint,pointIn,sizeof(RwV2d));
    rpoint.z = 0.0f;

    RwMatrixInvert( &mtx, Rt2dGlobals.ctm[Rt2dGlobals.mtos]);
    RwV3dTransformPoint(&rpoint, &rpoint, &mtx);

    memcpy(pointOut,&rpoint,sizeof(RwV2d));

    RWRETURN(pointOut);
}


/* ongoing work.. */


/****************************************************************************/


#if (0)

static void
LineInset2d(RtCubicBezier target, const RwV2d source[2], const RwReal R)
{
    RwV2d               normal;
    RwV2d               delta;

    RWFUNCTION(RWSTRING("LineInset2d"));

    RwV2dLineNormal(&normal, &source[1], &source[2]);
    RwV2dScale(&delta, &normal, R);

    RwV2dAdd(&target[0], &source[0], &delta);
    RwV2dAdd(&target[3], &source[1], &delta);

    target[1].x = target[0].x - normal.y;
    target[1].y = target[0].y + normal.x;

    target[2].x = target[3].x + normal.y;
    target[2].y = target[3].y - normal.x;

    RWRETURNVOID();
}

/****************************************************************************/
static void
BezierInset2d(RtCubicBezier target,
              const RtCubicBezier source, const RwReal R)
{

    RwV2d               P0, D1, D2, P3;
    RwV2d               Q0, E1, E2, Q3;

    RWFUNCTION(RWSTRING("BezierInset2d"));

    P0.x = source[0].x;
    P0.y = source[0].y;
    P3.x = source[3].x;
    P3.y = source[3].y;
    D1.x = source[1].x - P0.x;
    D1.y = source[1].y - P0.y;
    D2.x = source[2].x - P3.x;
    D2.y = source[2].y - P3.y;

    Q0.x =
        P0.x + 3 * (D1.y * R) / sqrt(9 * RwPow(D1.y, 2) +
                                     9 * RwPow(D1.x, 2));
    Q0.y =
        P0.y - 3 * (D1.x * R) / sqrt(9 * RwPow(D1.y, 2) +
                                     9 * RwPow(D1.x, 2));
    Q3.x =
        2 * P0.x - 2 * P3.x + 3 * D1.x - 3 * D2.x + 3 * (P3.x - P0.x -
                                                         2 * D1.x +
                                                         D2.x) +
        3 * D1.x + P0.x +
        (3 * (2 * P0.y - 2 * P3.y + 3 * D1.y - 3 * D2.y) +
         6 * (P3.y - P0.y - 2 * D1.y + D2.y) +
         3 * D1.y) * R / sqrt(RwPow(3 *
                                    (2 * P0.y - 2 * P3.y + 3 * D1.y -
                                     3 * D2.y) + 6 * (P3.y - P0.y -
                                                      2 * D1.y + D2.y) +
                                    3 * D1.y,
                                    2) + RwPow(3 * (2 * P3.x -
                                                    2 * P0.x -
                                                    3 * D1.x +
                                                    3 * D2.x) +
                                               6 * (P0.x - P3.x +
                                                    2 * D1.x - D2.x) -
                                               3 * D1.x, 2));
    Q3.y =
        2 * P0.y - 2 * P3.y + 3 * D1.y - 3 * D2.y + 3 * (P3.y - P0.y -
                                                         2 * D1.y +
                                                         D2.y) +
        3 * D1.y + P0.y +
        (3 * (2 * P3.x - 2 * P0.x - 3 * D1.x + 3 * D2.x) +
         6 * (P0.x - P3.x + 2 * D1.x - D2.x) -
         3 * D1.x) * R / sqrt(RwPow(3 *
                                    (2 * P0.y - 2 * P3.y + 3 * D1.y -
                                     3 * D2.y) + 6 * (P3.y - P0.y -
                                                      2 * D1.y + D2.y) +
                                    3 * D1.y,
                                    2) + RwPow(3 * (2 * P3.x -
                                                    2 * P0.x -
                                                    3 * D1.x +
                                                    3 * D2.x) +
                                               6 * (P0.x - P3.x +
                                                    2 * D1.x - D2.x) -
                                               3 * D1.x, 2));
    E1.x =
        3 * D1.x +
        6 * (R * (P3.y - P0.y - 2 * D1.y + D2.y)) / sqrt(9 *
                                                         RwPow(D1.y,
                                                               2) +
                                                         9 * RwPow(D1.x,
                                                                   2)) -
        1.5 * (D1.y *
               (R *
                (36 * (D1.y * (P3.y - P0.y - 2 * D1.y + D2.y)) -
                 36 * (D1.x * (P0.x - P3.x + 2 * D1.x - D2.x))))) /
        RwPow(9 * RwPow(D1.y, 2) + 9 * RwPow(D1.x, 2), (3.0 / 2.0));
    E1.y =
        3 * D1.y +
        6 * (R * (P0.x - P3.x + 2 * D1.x - D2.x)) / sqrt(9 *
                                                         RwPow(D1.y,
                                                               2) +
                                                         9 * RwPow(D1.x,
                                                                   2)) +
        1.5 * (D1.x *
               (R *
                (36 * (D1.y * (P3.y - P0.y - 2 * D1.y + D2.y)) -
                 36 * (D1.x * (P0.x - P3.x + 2 * D1.x - D2.x))))) /
        RwPow(9 * RwPow(D1.y, 2) + 9 * RwPow(D1.x, 2), (3.0 / 2.0));
    E2.x =
        3 * (2 * P0.x - 2 * P3.x + 3 * D1.x - 3 * D2.x) + 6 * (P3.x -
                                                               P0.x -
                                                               2 *
                                                               D1.x +
                                                               D2.x) +
        3 * D1.x +
        R * (6 * (2 * P0.y - 2 * P3.y + 3 * D1.y - 3 * D2.y) +
             6 * (P3.y - P0.y - 2 * D1.y +
                  D2.y)) / sqrt(RwPow(3 * (2 * P0.y - 2 * P3.y +
                                           3 * D1.y - 3 * D2.y) +
                                      6 * (P3.y - P0.y - 2 * D1.y +
                                           D2.y) + 3 * D1.y,
                                      2) + RwPow(3 * (2 * P3.x -
                                                      2 * P0.x -
                                                      3 * D1.x +
                                                      3 * D2.x) +
                                                 6 * (P0.x - P3.x +
                                                      2 * D1.x - D2.x) -
                                                 3 * D1.x,
                                                 2)) -
        0.5 *
        ((3
          * (2 * P0.y - 2 * P3.y + 3 * D1.y - 3 * D2.y) + 6 * (P3.y -
                                                               P0.y -
                                                               2 *
                                                               D1.y +
                                                               D2.y) +
          3 * D1.y) * (R * (2 *
                            ((3
                              * (2 * P0.y - 2 * P3.y + 3 * D1.y -
                                 3 * D2.y) + 6 * (P3.y - P0.y -
                                                  2 * D1.y + D2.y) +
                              3 * D1.y) * (6 * (2 * P0.y - 2 * P3.y +
                                                3 * D1.y - 3 * D2.y) +
                                           6 * (P3.y - P0.y - 2 * D1.y +
                                                D2.y))) +
                            2 *
                            ((3
                              * (2 * P3.x - 2 * P0.x - 3 * D1.x +
                                 3 * D2.x) + 6 * (P0.x - P3.x +
                                                  2 * D1.x - D2.x) -
                              3 * D1.x) * (6 * (2 * P3.x - 2 * P0.x -
                                                3 * D1.x + 3 * D2.x) +
                                           6 * (P0.x - P3.x + 2 * D1.x -
                                                D2.x)))))) /
        RwPow(RwPow
              (3 * (2 * P0.y - 2 * P3.y + 3 * D1.y - 3 * D2.y) +
               6 * (P3.y - P0.y - 2 * D1.y + D2.y) + 3 * D1.y,
               2) + RwPow(3 * (2 * P3.x - 2 * P0.x - 3 * D1.x +
                               3 * D2.x) + 6 * (P0.x - P3.x + 2 * D1.x -
                                                D2.x) - 3 * D1.x, 2),
              (3.0 / 2.0));
    E2.y =
        3 * (2 * P0.y - 2 * P3.y + 3 * D1.y - 3 * D2.y) + 6 * (P3.y -
                                                               P0.y -
                                                               2 *
                                                               D1.y +
                                                               D2.y) +
        3 * D1.y +
        R * (6 * (2 * P3.x - 2 * P0.x - 3 * D1.x + 3 * D2.x) +
             6 * (P0.x - P3.x + 2 * D1.x -
                  D2.x)) / sqrt(RwPow(3 * (2 * P0.y - 2 * P3.y +
                                           3 * D1.y - 3 * D2.y) +
                                      6 * (P3.y - P0.y - 2 * D1.y +
                                           D2.y) + 3 * D1.y,
                                      2) + RwPow(3 * (2 * P3.x -
                                                      2 * P0.x -
                                                      3 * D1.x +
                                                      3 * D2.x) +
                                                 6 * (P0.x - P3.x +
                                                      2 * D1.x - D2.x) -
                                                 3 * D1.x,
                                                 2)) -
        0.5 *
        ((3
          * (2 * P3.x - 2 * P0.x - 3 * D1.x + 3 * D2.x) + 6 * (P0.x -
                                                               P3.x +
                                                               2 *
                                                               D1.x -
                                                               D2.x) -
          3 * D1.x) * (R * (2 *
                            ((3
                              * (2 * P0.y - 2 * P3.y + 3 * D1.y -
                                 3 * D2.y) + 6 * (P3.y - P0.y -
                                                  2 * D1.y + D2.y) +
                              3 * D1.y) * (6 * (2 * P0.y - 2 * P3.y +
                                                3 * D1.y - 3 * D2.y) +
                                           6 * (P3.y - P0.y - 2 * D1.y +
                                                D2.y))) +
                            2 *
                            ((3
                              * (2 * P3.x - 2 * P0.x - 3 * D1.x +
                                 3 * D2.x) + 6 * (P0.x - P3.x +
                                                  2 * D1.x - D2.x) -
                              3 * D1.x) * (6 * (2 * P3.x - 2 * P0.x -
                                                3 * D1.x + 3 * D2.x) +
                                           6 * (P0.x - P3.x + 2 * D1.x -
                                                D2.x)))))) /
        RwPow(RwPow
              (3 * (2 * P0.y - 2 * P3.y + 3 * D1.y - 3 * D2.y) +
               6 * (P3.y - P0.y - 2 * D1.y + D2.y) + 3 * D1.y,
               2) + RwPow(3 * (2 * P3.x - 2 * P0.x - 3 * D1.x +
                               3 * D2.x) + 6 * (P0.x - P3.x + 2 * D1.x -
                                                D2.x) - 3 * D1.x, 2),
              (3.0 / 2.0));

    target[0].x = Q0.x;
    target[0].y = Q0.y;

    target[3].x = Q3.x;
    target[3].y = Q3.y;

    target[1].x = target[0].x + E1.x * (1.0 / 3.0);
    target[1].y = target[0].y + E1.y * (1.0 / 3.0);

    target[2].x = target[3].x - E2.x * (1.0 / 3.0);
    target[2].y = target[3].y - E2.y * (1.0 / 3.0);

    RWRETURNVOID();
}
#endif /* (0) */

#if defined (__MWERKS__)
#if (defined(RWVERBOSE))
#pragma message (__FILE__ "/" _SKY_EXPAND(__LINE__) ": __MWERKS__ == " _SKY_EXPAND(__MWERKS__))
#endif /* (defined (__MWERKS__)) */
#if (__option (global_optimizer))
#pragma always_inline on
#endif /* (__option (global_optimizer)) */
#endif /*  defined (__MWERKS__) */
