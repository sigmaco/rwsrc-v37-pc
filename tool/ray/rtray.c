/*
 * Line toolkit helper functions
 *
 * Copyright (c) 1998 Criterion Software Ltd.
 */

/**
 * \ingroup rtray
 * \page rtrayoverview RtRay Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rtray.h
 * \li \b Libraries: rwcore, rtray
 * \li \b Plugin \b attachments: None
 *
 * \subsection rayoverview Overview
 * This toolkit provides some functions for operations on lines (which
 * may be used to represent rays fired through a scene).
 *
 * These functions test for intersections with lines:
 *
 * \li \ref RtLineTriangleIntersectionTest  OBSOLETE - use
 *                      \ref RtIntersectionLineTriangle
 * \li \ref RtLineSphereIntersectionTest
 *
 * and these functions clip a line to a plane or bounding box:
 *
 * \li \ref RtLineClipPlane
 * \li \ref RtLineClipBBox
 */

/****************************************************************************
 Includes
 */

#include <stdlib.h>
#include <math.h>

#include "rwcore.h"

#include "rpdbgerr.h"

/* Ray handling */

#include "rtray.h"

/****************************************************************************
 Local Types
 */

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                             Line intersections

 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/**
 * \ingroup rtray
 *
 * OBSOLETE: Use \ref RtIntersectionLineTriangle from the rtintsec toolkit.
 *
 * \ref RtLineTriangleIntersectionTest intersects a line
 * with a triangle and return the parametric distance along the line of
 * intersection.
 *
 * Note that the header file rtray.h must be included, and the program
 * should be linked to rtray.lib.
 *
 * \param line  Line to intersect with the triangle
 * \param normal  Surface normal of the triangle
 * \param v0  Vertex 1 of the triangle
 * \param v1  Vertex 2 of the triangle
 * \param v2  Vertex 3 of the triangle
 *
 * \return Returns one of the following values:
 *
 *     \li positive - Parametric position of intersection along the line with the triangle
 *     \li -1       - On failure or if no intersection was found
 *
 * \see RtLineSphereIntersectionTest
 * \see RtLineClipPlane
 * \see RtLineClipBBox
 */

RwReal
RtLineTriangleIntersectionTest(RwLine * line, RwV3d * normal,
                               RwV3d * v0, RwV3d * v1, RwV3d * v2)
{
    RWAPIFUNCTION(RWSTRING("RtLineTriangleIntersectionTest"));
    RWASSERT(line);
    RWASSERT(normal);
    RWASSERT(v0);
    RWASSERT(v1);
    RWASSERT(v2);

#ifdef RWDEBUG
    {
        static RwUInt32    numCalls = 0;
        if (numCalls++ < 10)
        {
            RWMESSAGE((RWSTRING("OBSOLETE function - superceded by RtIntersectionLineTriangle()")));
        }
    }
#endif

    if (line && normal && v0 && v1 && v2)
    {
        RwSplitBits         sbT;
        RwV3d               lineDirection;

        /* Calc line vector */
        RwV3dSub(&lineDirection, &line->end, &line->start);

        /* Test if the line intesects the plane */
        sbT.nReal = RwV3dDotProduct(&lineDirection, normal);

        if (sbT.nInt)
        {
            RwV3d               point;
            RwSplitBits         sbA, sbB, sbC;

            RwV3dSub(&point, v0, &line->start);

            sbA.nReal = RwV3dDotProduct(normal, &point);

            /* sbA must be non-zero with sign equal to that of sbT */
            if ((sbA.nInt != 0) && (sbT.nInt ^ sbA.nInt) >= 0)
            {
                /* Calculate parametric position along the line */
                sbT.nReal = (sbA.nReal) / (sbT.nReal);

                /* Calculate point of intersection on the plane */
                RwV3dScale(&point, &lineDirection, sbT.nReal);
                RwV3dAdd(&point, &point, &line->start);

                /* We now need to work out if this is inside the triangle.
                 * This is done by using barycentric coordinates.
                 */

                if (RwRealAbs(normal->x) > RwRealAbs(normal->y))
                {
                    /* X or Z */
                    if (RwRealAbs(normal->x) > RwRealAbs(normal->z))
                    {
                        /* Lose X - do the barycentric thing */
                        sbA.nReal =
                            (point.z - v0->z) * (v1->y - v0->y) -
                            (point.y - v0->y) * (v1->z - v0->z);
                        sbB.nReal =
                            (point.z - v1->z) * (v2->y - v1->y) -
                            (point.y - v1->y) * (v2->z - v1->z);
                        sbC.nReal =
                            (point.z - v2->z) * (v0->y - v2->y) -
                            (point.y - v2->y) * (v0->z - v2->z);
                    }
                    else
                    {
                        /* Lose Z - do the barycentric thing */
                        sbA.nReal =
                            (point.x - v0->x) * (v1->y - v0->y) -
                            (point.y - v0->y) * (v1->x - v0->x);
                        sbB.nReal =
                            (point.x - v1->x) * (v2->y - v1->y) -
                            (point.y - v1->y) * (v2->x - v1->x);
                        sbC.nReal =
                            (point.x - v2->x) * (v0->y - v2->y) -
                            (point.y - v2->y) * (v0->x - v2->x);
                    }
                }
                else
                {
                    /* Y or Z */
                    if (RwRealAbs(normal->y) > RwRealAbs(normal->z))
                    {
                        /* Lose Y - do the barycentric thing */
                        sbA.nReal =
                            (point.z - v0->z) * (v1->x - v0->x) -
                            (point.x - v0->x) * (v1->z - v0->z);
                        sbB.nReal =
                            (point.z - v1->z) * (v2->x - v1->x) -
                            (point.x - v1->x) * (v2->z - v1->z);
                        sbC.nReal =
                            (point.z - v2->z) * (v0->x - v2->x) -
                            (point.x - v2->x) * (v0->z - v2->z);
                    }
                    else
                    {
                        /* Lose Z - do the barycentric thing */
                        sbA.nReal =
                            (point.x - v0->x) * (v1->y - v0->y) -
                            (point.y - v0->y) * (v1->x - v0->x);
                        sbB.nReal =
                            (point.x - v1->x) * (v2->y - v1->y) -
                            (point.y - v1->y) * (v2->x - v1->x);
                        sbC.nReal =
                            (point.x - v2->x) * (v0->y - v2->y) -
                            (point.y - v2->y) * (v0->x - v2->x);
                    }
                }

                /* Test if the intersection point is inside the triangle */

                /* If all positive, it's inside */
                /* If some are positive and some negative, it's outside */
                /* Can't all be negateive (by definition of barycentric coordinates) */
                if (((sbA.nInt ^ sbB.nInt) >= 0)
                    && ((sbB.nInt ^ sbC.nInt) >= 0))
                {
                    /* Done */
                    RWRETURN(sbT.nReal);
                }
            }
        }

        /* Not a hit */
        RWRETURN((RwReal) (-1.0));
    }

    /* Null pointer supplied */
    RWERROR((E_RW_NULLP));
    RWRETURN((RwReal) (-1.0));
}

/**
 * \ingroup rtray
 * \ref RtLineSphereIntersectionTest tests for the intersection of
 * a line with a sphere, and returns a distance parameter.
 *
 * \note The centerDist parameter is NOT the distance to the
 * surface of the sphere.
 *
 * \param line        Line to intersect with the sphere
 * \param sphere      Sphere to intersect the line with
 * \param centerDist  The relative position of the sphere's center projected
 *                    onto the line as a fraction of the lines length.
 *                    This will usually be a value between zero and
 *                    one, but can in some cases take values outside
 *                    this range. Negative values can result when
 *                    the line starts inside the sphere, and points
 *                    away from the center.
 *
 * \return Returns TRUE if an intersection is found.
 *
 * \see RtLineTriangleIntersectionTest
 * \see RtLineClipPlane
 * \see RtLineClipBBox
 */

RwBool
RtLineSphereIntersectionTest(RwLine * line,
                             RwSphere * sphere,
                             RwReal *centerDist)
{
    RwV3d               lineNormal;
    RwReal              recipLineLength;
    RwReal              atomicRadius2; /* Radius of atomic ^2 */
    RwReal              atomicDistance; /* Distance of atomic along line */
    RwV3d               vE0;
    RwReal              hyp2; /* Distance from line start to atomic center ^2 */

    RWAPIFUNCTION(RWSTRING("RtLineSphereIntersectionTest"));
    RWASSERT(line);
    RWASSERT(sphere);
    RWASSERT(centerDist);

    /* Calculate line parameters */
    RwV3dSub(&lineNormal, &line->end, &line->start);
    recipLineLength = _rwV3dNormalize(&lineNormal, &lineNormal);

    /* Get bounding sphere of atomic */
    atomicRadius2 = (sphere->radius) * (sphere->radius);

    /* Offset atomic's center so that line is effectively starting at (0,0,0) */
    RwV3dSub(&vE0, &sphere->center, &line->start);

    /* Project atomic's center distance along line */
    atomicDistance = RwV3dDotProduct(&vE0, &lineNormal);

    /* Distance to atomic center */
    hyp2 = RwV3dDotProduct(&vE0, &vE0);

    if (hyp2 < atomicRadius2)
    {
        /* Line starts inside the sphere */
        *centerDist = atomicDistance * recipLineLength;
        RWRETURN(TRUE);
    }
    else if (atomicDistance > (RwReal) (0.0))
    {
        /* It's in front of the line */
        RwReal              disc;

        /* Do pythagoras to see if we intersect the sphere */
        disc =
            atomicRadius2 - (hyp2 -
                             (atomicDistance) * (atomicDistance));

        if (disc >= (RwReal) (0.0))
        {
            *centerDist = atomicDistance * recipLineLength;
            RWRETURN(TRUE);
        }
    }

    /* No collision */
    RWRETURN(FALSE);
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                             Line Clipping

 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/**
 * \ingroup rtray
 * \ref RtLineClipPlane clips a line against a plane such that the
 * line lies entirely inside the plane.
 * The inside of the plane is defined by the normal (which points outside).
 *
 * Note that the header file rtray.h must be included, and the program
 * should be linked to rtray.lib.
 *
 * \param line  Line to clip against the plane
 * \param plane  Plane to clip against
 *
 * \return Returns one of the following values:
 *
 *      \li line - On success and there is some line left
 *      \li NULL - On failure or if no line remains after clipping
 *
 * \see RtLineTriangleIntersectionTest
 * \see RtLineSphereIntersectionTest
 * \see RtLineClipBBox
 */

RwLine             *
RtLineClipPlane(RwLine * line, RwPlane * plane)
{
    RWAPIFUNCTION(RWSTRING("RtLineClipPlane"));
    RWASSERT(line);
    RWASSERT(plane);

    if (line && plane)
    {
        RwV3d               vTmp;
        RwSplitBits         sbStart, sbEnd;

        sbStart.nReal =
            RwV3dDotProduct(&plane->normal,
                            &line->start) - plane->distance;
        sbEnd.nReal =
            RwV3dDotProduct(&plane->normal,
                            &line->end) - plane->distance;

        switch ((((RwUInt32) sbStart.nInt) >> 31) +
                ((sbEnd.nInt >> 30) & 2))
        {
            case 3:
                {
                    /* All is out */
                    RWRETURN((RwLine *)NULL);
                }
            case 0:
                {
                    /* All is in */
                    break;
                }
            case 2:
                {
                    /* Start is in end is out */
                    RwV3dSub(&vTmp, &line->end, &line->start);
                    RwV3dScale(&vTmp, &vTmp,
                               (sbEnd.nReal) / (sbEnd.nReal -
                                                sbStart.nReal));
                    RwV3dSub(&line->end, &line->end, &vTmp);

                    break;
                }
            case 1:
                {
                    /* Start is out end is in */
                    RwV3dSub(&vTmp, &line->end, &line->start);
                    RwV3dScale(&vTmp, &vTmp,
                               (sbStart.nReal) / (sbEnd.nReal -
                                                  sbStart.nReal));
                    RwV3dSub(&line->start, &line->start, &vTmp);

                    break;
                }
        }

        /* Done */
        RWRETURN(line);
    }

    /* Null pointer */
    RWERROR((E_RW_NULLP));
    RWRETURN((RwLine *)NULL);
}

/**
 * \ingroup rtray
 * \ref RtLineClipBBox clips a line against a bounding box such
 * that the resultant line lies * entirely within the box.
 *
 * Note that the header file rtray.h must be included, and the program
 * should be linked to rtray.lib.
 *
 * \param line  Line to clip against the box
 * \param box  Bounding box to clip against
 *
 * \return Returns one of the following values:
 *
 *        \li line - On success and there is some line left
 *        \li NULL - On failure or if no line remains after clipping
 *
 * \see RtLineTriangleIntersectionTest
 * \see RtLineSphereIntersectionTest
 * \see RtLineClipPlane
 */

RwLine             *
RtLineClipBBox(RwLine * line, RwBBox * box)
{
    RWAPIFUNCTION(RWSTRING("RtLineClipBBox"));
    RWASSERT(line);
    RWASSERT(box);

    if (line && box)
    {
        RwPlane             plane;

        plane.normal.x = (RwReal) (0);
        plane.normal.y = (RwReal) (0);
        plane.normal.z = (RwReal) (1);
        plane.distance = box->inf.z;
        if (!RtLineClipPlane(line, &plane))
        {
            RWRETURN((RwLine *)NULL);
        }

        plane.normal.x = (RwReal) (0);
        plane.normal.y = (RwReal) (1);
        plane.normal.z = (RwReal) (0);
        plane.distance = box->inf.y;
        if (!RtLineClipPlane(line, &plane))
        {
            RWRETURN((RwLine *)NULL);
        }

        plane.normal.x = (RwReal) (1);
        plane.normal.y = (RwReal) (0);
        plane.normal.z = (RwReal) (0);
        plane.distance = box->inf.x;
        if (!RtLineClipPlane(line, &plane))
        {
            RWRETURN((RwLine *)NULL);
        }

        plane.normal.x = (RwReal) (-1);
        plane.normal.y = (RwReal) (0);
        plane.normal.z = (RwReal) (0);
        plane.distance = -box->sup.x;
        if (!RtLineClipPlane(line, &plane))
        {
            RWRETURN((RwLine *)NULL);
        }

        plane.normal.x = (RwReal) (0);
        plane.normal.y = (RwReal) (-1);
        plane.normal.z = (RwReal) (0);
        plane.distance = -box->sup.y;
        if (!RtLineClipPlane(line, &plane))
        {
            RWRETURN((RwLine *)NULL);
        }

        plane.normal.x = (RwReal) (0);
        plane.normal.y = (RwReal) (0);
        plane.normal.z = (RwReal) (-1);
        plane.distance = -box->sup.z;
        if (!RtLineClipPlane(line, &plane))
        {
            RWRETURN((RwLine *)NULL);
        }

        /* We have a line left ! */
        RWRETURN(line);
    }

    /* Null pointer */
    RWERROR((E_RW_NULLP));
    RWRETURN((RwLine *)NULL);
}
