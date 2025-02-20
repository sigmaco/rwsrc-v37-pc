/**
 * \ingroup rtintersection
 * \page rtintersectionoverview RtIntersection Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rtintsec.h
 * \li \b Libraries: rwcore, rtintsec
 * \li \b Plugin \b attachments: None
 *
 * \subsection intsecoverview Overview
 * This toolkit provides functions for testing whether certain geometry
 * primitives intersect. The following are provided:
 *
 * \li  \ref RtIntersectionLineTriangle tests whether a given line intersects
 *           a given triangle.
 * \li  \ref RtIntersectionSphereTriangle tests whether a given sphere
 *           intersects a given triangle.
 * \li  \ref RtIntersectionBBoxTriangle tests whether a given bounding box
 *           intersections a given triangle.
 */

/****************************************************************************
 Includes
 */

#include <rwcore.h>
#include <rpdbgerr.h>

#include "rtintsec.h"

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
 * \ingroup rtintersection
 * \ref RtIntersectionLineTriangle tests for the intersection of a line
 * with a triangle, and if found, returns the parametric distance to the
 * intersection along the line.
 *
 * This function returns TRUE if the line, specified at the starting point
 * \e lineStart (\ref RwV3d) and a \e lineDelta (\ref RwV3d) defining the
 * displacement vector, intersects with the triangle (\ref RwV3d).
 *
 * The \e lineDelta parameter is used to reduce calculation overheads when
 * processing a large number of triangles and can be obtained from
 * \ref RwV3dSub(lineDelta, &line.end, &line.start).
 *
 * This function also takes another parameter, \e distance, which will
 * hold the parametric distance to the intersection if an intersection
 * was found.
 *
 * This function use backface culling which means:
 * \li the order of the triangle vertices is very important
 * \li you will need to make two calls for two-sided tests
 *
 * \param lineStart  Line start vertex
 * \param lineDelta  Line displacement vector
 * \param v0         Triangle vertex 0
 * \param v1         Triangle vertex 1
 * \param v2         Triangle vertex 2
 * \param distance   On return, contains parameteric distance to intersection
 *
 * \return Returns TRUE if an intersection is found, or FALSE otherwise.
 *
 *
 */
RwBool
RtIntersectionLineTriangle(const RwV3d * lineStart,
                           const RwV3d * lineDelta,
                           const RwV3d * v0,
                           const RwV3d * v1,
                           const RwV3d * v2,
                           RwReal * distance)
{
    /*
     * Based on Tomas Moller's optimized Ray Triangle Interesection code at
     * http://www.ce.chalmers.se/staff/tomasm/code
     * http://www.acm.org/jgt/papers/MollerTrumbore97
     */
    RwBool              result;

    RWAPIFUNCTION(RWSTRING("RtIntersectionLineTriangle"));

    RWASSERT(lineStart);
    RWASSERT(lineDelta);
    RWASSERT(v0);
    RWASSERT(v1);
    RWASSERT(v2);
    RWASSERT(distance);

    RtIntersectionLineTriangleMacro(result,
                                    lineStart, lineDelta,
                                    v0, v1, v2, distance);

    RWRETURN(result);
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *          Sphere-Triangle Intersection
 *
 *!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

#define REALASINT32(r) ( *((RwInt32 *) &(r)) )

#define BBOXREJECT(AXIS)                                                    \
{                                                                           \
    /* The three vertices are translated so that the sphere center is at */ \
    /* (0, 0, 0), this basically generates clip flags for the 3 vertices */ \
    /* in one axis. */                                                      \
    int n = (REALASINT32(vc0.AXIS) >> 31) + (REALASINT32(vc1.AXIS) >> 31) + \
            (REALASINT32(vc2.AXIS) >> 31);                                  \
                                                                            \
    if ( n == 0 )                                                           \
    {                                                                       \
        RwReal r1 = ((vc0.AXIS) - ( sphere->radius)),                       \
               r2 = ((vc1.AXIS) - ( sphere->radius)),                       \
               r3 = ((vc2.AXIS) - ( sphere->radius));                       \
                                                                            \
        if ((REALASINT32(r1) | REALASINT32(r2) | REALASINT32(r3)) >= 0)     \
        {                                                                   \
            RWRETURN(FALSE);                                                \
        }                                                                   \
    }                                                                       \
    else if ( n == -3 )                                                     \
    {                                                                       \
        RwReal r1 = ((vc0.AXIS) + ( sphere->radius)),                       \
               r2 = ((vc1.AXIS) + ( sphere->radius)),                       \
               r3 = ((vc2.AXIS) + ( sphere->radius));                       \
                                                                            \
        if ((REALASINT32(r1) & REALASINT32(r2) & REALASINT32(r3)) < 0)      \
        {                                                                   \
            RWRETURN(FALSE);                                                \
        }                                                                   \
    }                                                                       \
}

/**
 * \ingroup rtintersection
 * \ref RtIntersectionSphereTriangle tests for the intersection of a
 * sphere with a triangle, and if found, returns the triangle normal and
 * the perpendicular distance of the sphere center from the plane of the
 * triangle.
 *
 * \param sphere  Sphere
 * \param v0  Triangle vertex 0
 * \param v1  Triangle vertex 1
 * \param v2  Triangle vertex 2
 * \param normal  If intersection is found, returns triangle normal
 * \param distance  If intersection is found, returns distance of sphere center from the
 *       plane of the triangle
 *
 * \return Returns TRUE if an intersection is found, or FALSE otherwise.
 */
RwBool
RtIntersectionSphereTriangle(const RwSphere * sphere,
                             const RwV3d * v0,
                             const RwV3d * v1,
                             const RwV3d * v2,
                             RwV3d * normal,
                             RwReal * distance)
{
    RwV3d               vc0, vc1, vc2;
    RwV3d               vAtoB;
    RwV3d               vN;
    RwReal              nDotN;
    RwReal              distToPlane;
    RwReal              sphereRadiusSquared;
    RwV3d               vTmp, vTmp2;
    RwReal              length2;
    RwReal              factor = 0;

    RWAPIFUNCTION(RWSTRING("RtIntersectionSphereTriangle"));
    RWASSERT(sphere);
    RWASSERT(normal);
    RWASSERT(v0);
    RWASSERT(v1);
    RWASSERT(v2);
    RWASSERT(distance);

    /* Subtract the center of the sphere */
    RwV3dSub(&vc0, v0, &sphere->center);
    RwV3dSub(&vc1, v1, &sphere->center);
    RwV3dSub(&vc2, v2, &sphere->center);

    /* Using COMMANDO as a test bed, statistics consistently show 75%
     * of polygons being eliminated within the next three lines
     */
    /* Bounding box test */
    BBOXREJECT(x);
    BBOXREJECT(y);
    BBOXREJECT(z);

    /* Now calculate a normal, and make sure it's not degenerate */
    RwV3dSub(&vTmp, v1, v0);
    RwV3dSub(&vTmp2, v2, v0);
    RwV3dCrossProduct(normal, &vTmp, &vTmp2);

    length2 = RwV3dDotProduct(normal, normal);
    if (length2 <= (RwReal) (0))
    {
        /* Must be degenerate */
        RWRETURN(FALSE);
    }

    /* Normalise the normal */
    rwInvSqrt(&factor, length2);
    RwV3dScale(normal, normal, factor);

    /* Normal and vc0 define first separating plane */
    distToPlane = RwV3dDotProduct(&vc0, normal);

    /* nDist to plane is negative if on the normals side of poly */
    if ((distToPlane < -sphere->radius)
        || (distToPlane > sphere->radius))
    {
        RWRETURN(FALSE);
    }

    /* From now on, we might be saying yes, instead of no,
     * better fill in a distance */
    *distance = -distToPlane;

    /* Save some square roots */
    sphereRadiusSquared = ((sphere->radius) * (sphere->radius));

    /* See if any of the vertices are inside the sphere */
    vN.x = RwV3dDotProduct(&vc0, &vc0);
    if (vN.x <= sphereRadiusSquared)
    {
        /* vc0 inside the sphere */
        RWRETURN(TRUE);
    }
    vN.y = RwV3dDotProduct(&vc1, &vc1);
    if (vN.y <= sphereRadiusSquared)
    {
        /* vc1 inside the sphere */
        RWRETURN(TRUE);
    }
    vN.z = RwV3dDotProduct(&vc2, &vc2);
    if (vN.z <= sphereRadiusSquared)
    {
        /* vc2 inside the sphere */
        RWRETURN(TRUE);
    }

    if (vN.x < vN.y)
    {
        if (vN.z < vN.x)
        {
            /* vc2 is closest */
            if ((RwV3dDotProduct(&vc2, &vc0) > vN.z) &&
                (RwV3dDotProduct(&vc2, &vc1) > vN.z))
            {
                RWRETURN(FALSE);
            }
        }
        else
        {
            /* vc0 is closest */
            if ((RwV3dDotProduct(&vc0, &vc1) > vN.x) &&
                (RwV3dDotProduct(&vc0, &vc2) > vN.x))
            {
                RWRETURN(FALSE);
            }
        }
    }
    else
    {
        if (vN.z < vN.y)
        {
            /* vc2 is closest */
            if ((RwV3dDotProduct(&vc2, &vc0) > vN.z) &&
                (RwV3dDotProduct(&vc2, &vc1) > vN.z))
            {
                RWRETURN(FALSE);
            }
        }
        else
        {
            /* vc1 is closest */
            if ((RwV3dDotProduct(&vc1, &vc0) > vN.y) &&
                (RwV3dDotProduct(&vc1, &vc2) > vN.y))
            {
                RWRETURN(FALSE);
            }
        }
    }

    /* Try a separating plane holding vc0 and vc1 */
    RwV3dSub(&vAtoB, &vc1, &vc0);
    factor =
        ((RwV3dDotProduct(&vAtoB, &vc0)) /
         (RwV3dDotProduct(&vAtoB, &vAtoB)));
    RwV3dScale(&vN, &vAtoB, factor);
    RwV3dSub(&vN, &vc0, &vN);

    nDotN = RwV3dDotProduct(&vN, &vN);

    if ((nDotN > sphereRadiusSquared)
        && (nDotN < RwV3dDotProduct(&vN, &vc2)))
    {
        /* Found a plane */
        RWRETURN(FALSE);
    }

    /* Try a separating plane holding vc1 and vc2 */
    RwV3dSub(&vAtoB, &vc2, &vc1);

    factor =
        ((RwV3dDotProduct(&vAtoB, &vc1)) /
         (RwV3dDotProduct(&vAtoB, &vAtoB)));
    RwV3dScale(&vN, &vAtoB, factor);
    RwV3dSub(&vN, &vc1, &vN);

    nDotN = RwV3dDotProduct(&vN, &vN);

    if ((nDotN > sphereRadiusSquared)
        && (nDotN < RwV3dDotProduct(&vN, &vc0)))
    {
        /* Found a plane */
        RWRETURN(FALSE);
    }

    /* Try a separating plane holding vc2 and vc0 */
    RwV3dSub(&vAtoB, &vc0, &vc2);
    factor =
        ((RwV3dDotProduct(&vAtoB, &vc2)) /
         (RwV3dDotProduct(&vAtoB, &vAtoB)));
    RwV3dScale(&vN, &vAtoB, factor);
    RwV3dSub(&vN, &vc2, &vN);

    nDotN = RwV3dDotProduct(&vN, &vN);

    if ((nDotN > sphereRadiusSquared)
        && (nDotN < RwV3dDotProduct(&vN, &vc1)))
    {
        /* Found a plane */
        RWRETURN(FALSE);
    }

    /* Must intersect */
    RWRETURN(TRUE);
}

/*
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *          Box-Triangle intersection
 *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

#define REALSIGNNEQ(x, y) ((*(RwInt32*)&(x) ^ *(RwInt32*)&(y)) < 0)

/* Which of the six BBox faces is point P outside of? */
#define BBoxPointTestMacro(_box, _p)                                    \
    ( ( ((_p)->x > (_box)->sup.x) ?                                     \
        ((RwInt32) 0x01) :                                              \
        ( ((_p)->x < (_box)->inf.x) ?                                   \
          ((RwInt32) 0x02) : ((RwInt32) 0x00) ) )                       \
    | ( ((_p)->y > (_box)->sup.y) ?                                     \
        ((RwInt32) 0x04) :                                              \
        ( ((_p)->y < (_box)->inf.y) ?                                   \
          ((RwInt32) 0x08) : ((RwInt32) 0x00) ) )                       \
    | ( ((_p)->z > (_box)->sup.z) ?                                     \
        ((RwInt32) 0x10) :                                              \
        ( ((_p)->z < (_box)->inf.z) ?                                   \
          ((RwInt32) 0x20) : ((RwInt32) 0x00) ) )                       \
    )

/* Try intersection on _b-_c plane at position _p = lo or hi on axis _a */
#define BOXFACE_LINE_TEST(_p,_a,_b,_c)                                  \
{                                                                       \
    RwReal tmp;                                                         \
    tmp = _p._a * del._b;                                               \
    if (del._a < 0.0f)                                                  \
    {                                                                   \
        if ( (tmp < lo._b * del._a) && (tmp > hi._b * del._a) )         \
        {                                                               \
            tmp = _p._a * del._c;                                       \
            if ( (tmp < lo._c * del._a)  &&  (tmp > hi._c * del._a) )   \
                RWRETURN(TRUE);                                         \
        }                                                               \
    }                                                                   \
    else                                                                \
    {                                                                   \
        if ( (tmp > lo._b * del._a) && (tmp < hi._b * del._a) )         \
        {                                                               \
            tmp = _p._a * del._c;                                       \
            if ( (tmp > lo._c * del._a)  &&  (tmp < hi._c * del._a) )   \
                RWRETURN(TRUE);                                         \
        }                                                               \
    }                                                                   \
}

#define BBoxLineTestMacro(p1, p2, box, outcodeDiff)                     \
{                                                                       \
    RwV3d   del, hi, lo;                                                \
                                                                        \
    RwV3dSub(&del, (p2), (p1));                                         \
    RwV3dSub(&hi, p1, &box->inf);                                       \
    RwV3dSub(&lo, p1, &box->sup);                                       \
                                                                        \
    /* Test faces at x inf/sup */                                       \
    if (0x01 & (outcodeDiff))                                           \
        BOXFACE_LINE_TEST(lo,x,y,z);                                    \
    if (0x02 & (outcodeDiff))                                           \
        BOXFACE_LINE_TEST(hi,x,y,z);                                    \
                                                                        \
    /* Test faces at y inf/sup */                                       \
    if (0x04 & (outcodeDiff))                                           \
        BOXFACE_LINE_TEST(lo,y,z,x);                                    \
    if (0x08 & (outcodeDiff))                                           \
        BOXFACE_LINE_TEST(hi,y,z,x);                                    \
                                                                        \
    /* Test faces at z inf/sup */                                       \
    if (0x10 & (outcodeDiff))                                           \
        BOXFACE_LINE_TEST(lo,z,x,y);                                    \
    if (0x20 & (outcodeDiff))                                           \
        BOXFACE_LINE_TEST(hi,z,x,y);                                    \
}

/**
 * \ingroup rtintersection
 * \ref RtIntersectionBBoxTriangle tests for the intersection of a
 * bounding box with a triangle.
 *
 * \param bbox  Bounding box
 * \param v0  Triangle vertex 0
 * \param v1  Triangle vertex 1
 * \param v2  Triangle vertex 2
 *
 * \return Returns TRUE if an intersection is found, or FALSE otherwise.
 */
RwBool
RtIntersectionBBoxTriangle(const RwBBox * bbox,
                           const RwV3d * v0,
                           const RwV3d * v1,
                           const RwV3d * v2)
{
    RwInt32             v0Test, v1Test, v2Test;

    RWAPIFUNCTION(RWSTRING("RtIntersectionBBoxTriangle"));

    /* Set bits in vXTest for those face-planes which point X lies outside
     * of. If all zero, then point must be inside, so return TRUE.
     */
    if ((v0Test = BBoxPointTestMacro(bbox, v0)) == 0)
        RWRETURN(TRUE);
    if ((v1Test = BBoxPointTestMacro(bbox, v1)) == 0)
        RWRETURN(TRUE);
    if ((v2Test = BBoxPointTestMacro(bbox, v2)) == 0)
        RWRETURN(TRUE);

    /* If all three vertices were outside of one or more face-planes
     * return immediately with a trivial rejection.
     */
    if (v0Test & v1Test & v2Test)
        RWRETURN(FALSE);

    /* Test each edge of the triangle for intersection with the bbox.
     * Pass the XOR of the outcode bits, so that only those cube faces which
     * are spanned by each triangle edge need to be tested.
     */
    if ((v0Test & v1Test) == 0)
        BBoxLineTestMacro(v0, v1, bbox, v0Test ^ v1Test);
    if ((v0Test & v2Test) == 0)
        BBoxLineTestMacro(v0, v2, bbox, v0Test ^ v2Test);
    if ((v1Test & v2Test) == 0)
        BBoxLineTestMacro(v1, v2, bbox, v1Test ^ v2Test);

    /* Now test for the cube intersecting the interior of the triangle. We do
     * this by looking for intersections between a cube diagonal and the
     * triangle.
     */
    {
        RwV3d               v01, v02, norm, diag, tVec;
        RwReal              det, dist;

        /* Get normal (unnormalized!) */
        RwV3dSub(&v01, v1, v0);
        RwV3dSub(&v02, v2, v0);
        RwV3dCrossProduct(&norm, &v01, &v02);

        /* Get diagonal of bbox in same octant of space as the normal. This
         * is the only one we need to test. Also get tVec which is the
         * vector from the first triangle vertex vp0 to the start of the
         * diagonal.
         */
        diag.x = bbox->sup.x - bbox->inf.x;
        tVec.x = bbox->inf.x - v0->x;

        if (REALSIGNNEQ(norm.x, norm.y))
        {
            diag.y = bbox->inf.y - bbox->sup.y;
            tVec.y = bbox->sup.y - v0->y;
        }
        else
        {
            diag.y = bbox->sup.y - bbox->inf.y;
            tVec.y = bbox->inf.y - v0->y;
        }

        if (REALSIGNNEQ(norm.x, norm.z))
        {
            diag.z = bbox->inf.z - bbox->sup.z;
            tVec.z = bbox->sup.z - v0->z;
        }
        else
        {
            diag.z = bbox->sup.z - bbox->inf.z;
            tVec.z = bbox->inf.z - v0->z;
        }

        /* Now do a variation on a Moller-Trumbore test to find out
         * if the diagonal intersects the triangle. The variables
         * det, u, v, and dist have the same meaning as in
         * RtIntersectionLineTriangleMacro().
         */
        det = -RwV3dDotProduct(&diag, &norm);
        dist = RwV3dDotProduct(&tVec, &norm);

        if (det < 0.0f)
        {
            /* Does line pass through plane of triangle? */
            if (0.0f > dist && dist > det)
            {
                RwReal              u, v;
                RwV3d               wVec;

                /* Is plane intersection within triangle? */
                RwV3dCrossProduct(&wVec, &tVec, &diag);
                u = RwV3dDotProduct(&v02, &wVec);
                if (0.0f > u && u > det)
                {
                    v = -RwV3dDotProduct(&v01, &wVec);
                    if (0.0f > v && (u + v) > det)
                        RWRETURN(TRUE);
                }
            }
        }
        else
        {
            /* Same as above with tests reversed */
            if (0.0f < dist && dist < det)
            {
                RwReal              u, v;
                RwV3d               wVec;

                RwV3dCrossProduct(&wVec, &tVec, &diag);
                u = RwV3dDotProduct(&v02, &wVec);
                if (0.0f < u && u < det)
                {
                    v = -RwV3dDotProduct(&v01, &wVec);
                    if (0.0f < v && (u + v) < det)
                        RWRETURN(TRUE);
                }
            }
        }
    }

    /* There was no intersection */
    RWRETURN(FALSE);
}
