/*
 * Arithmetic and Conversion functionality for Barycentric weights
 *
 * Copyright (c) Criterion Software Limited
 */

/**
 * \ingroup rtbary
 * \page rtbaryoverview RtBary Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rtbary.h
 * \li \b Libraries: rwcore, rtbary
 * \li \b Plugin \b attachments: None
 *
 * \subsection baryoverview Overview
 * This toolkit provides functionality for mapping points between
 * the barycentric space defined by a triangle and Cartesian space.
 * Given a triangle with vertices (v0, v1, v2), the barycentric
 * weights (w0, w1, w2)  correspond with the Cartesian
 * point w0*v0 + w1*v1 + w2*v2 where (w0+w1+w2) == 1
 */

/****************************************************************************
 *
 * Barycentric API
 * Copyright (C) 2001 Criterion Technologies
 *
 * Author  : John Spackman
 *
 * Module  : rtbary.c
 *
 * Purpose : Functions for for mapping points between the
 *           barycentric space defined by a triangle and Cartesian space.
 *
 ****************************************************************************/

/****************************************************************************
 Includes
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "rpplugin.h"
#include <rpdbgerr.h>
#include <rwcore.h>
#include <rtbary.h>

#if (defined(RWDEBUG))
long rtBarycentricStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

#define _AREA2_EPSILON (((RwReal)1)/((RwReal)(1<<12)))

/****************************************************************************
 Local Types
 */

/**
 * \ingroup rtbary
 * \ref RtBaryGetTransform finds the 4x4 transform matrix mapping a point
 * from homogeneous Cartesian space to the barycentric space defined by
 * a triangle.  The final column of the matrix holds the equation
 * of the plane supporting the triangle.
 *
 * \param xform  receives the 4x4 transform matrix
 * \param area   receives the area of the triangle
 * \param v0     first vertex of triangle
 * \param v1     second vertex of triangle
 * \param v2     third vertex of triangle
 *
 * \return Flags success
 *
 * \see RtBaryWeightsFromV3d
 * \see RtBaryV3dFromWeights
 */
RwBool
RtBaryGetTransform(RtBaryTransform xform,
                   RwReal * const area,
                   const RwV3d * const v0,
                   const RwV3d * const v1,
                   const RwV3d * const v2)
{
    RwReal              area2;
    RwBool              hasBarySupport;
    RwReal              normalize;
    RwV3d               b0, b1;
    RwV3d               centroid;
    RwV3d               e0, e1;
    RwV3d               n;

    RWAPIFUNCTION(RWSTRING("RtBaryGetTransform"));

    RWASSERT((RwReal *) NULL != area);
    RWASSERT((const RwV3d *) NULL != v0);
    RWASSERT((const RwV3d *) NULL != v1);
    RWASSERT((const RwV3d *) NULL != v2);

    /* Find edges
     *
     *         V0 o---------------o V1
     *             \             /
     *              \           /
     *               \ __    | /
     *            e1 |\      |/__  e0
     *               | \     /
     *                  \   /
     *                   \ /
     *                    o
     *                   V2
     *
     */

    RwV3dSub(&e0, v1, v2);
    RwV3dSub(&e1, v2, v0);

    RwV3dCrossProduct(&n, &e0, &e1);
    area2 = RwV3dDotProduct(&n, &n);
    hasBarySupport = (_AREA2_EPSILON < area2);
    if (hasBarySupport)
    {
#if (defined(RW_USE_SPF))
        normalize = (RwReal)sqrtf(area2);
#else /* (defined(RW_USE_SPF)) */
        normalize = (RwReal)sqrt(area2);
#endif /* (defined(RW_USE_SPF)) */

        *area = (RwReal) normalize;
        normalize = 1 / normalize;
    }
    else
    {
        normalize = 0;
        *area = (RwReal) normalize;
    }

    RwV3dScale(&n, &n, normalize);

    RwV3dCrossProduct(&b0, &e0, &n);
    RwV3dScale(&b0, &b0, normalize);

    RwV3dCrossProduct(&b1, &e1, &n);
    RwV3dScale(&b1, &b1, normalize);

    RwV3dAdd(&centroid, v0, v1);
    RwV3dAdd(&centroid, &centroid, v2);
    RwV3dScale(&centroid, &centroid, (RwReal) (1.0 / 3.0));

    xform[0][0] = b0.x;
    xform[0][1] = b1.x;
    xform[0][2] = -(b0.x + b1.x);
    xform[0][3] = n.x;

    xform[1][0] = b0.y;
    xform[1][1] = b1.y;
    xform[1][2] = -(b0.y + b1.y);
    xform[1][3] = n.y;

    xform[2][0] = b0.z;
    xform[2][1] = b1.z;
    xform[2][2] = -(b0.z + b1.z);
    xform[2][3] = n.z;

    xform[3][0] = -RwV3dDotProduct(&b0, v2);
    xform[3][1] = -RwV3dDotProduct(&b1, v0);
    xform[3][2] = ((RwReal) 1) - (xform[3][0] + xform[3][1]);
    xform[3][3] = -RwV3dDotProduct(&n, &centroid);

    RWRETURN(hasBarySupport);
}

/**
 * \ingroup rtbary
 * \ref RtBaryNormalize
 *
 * Internal Use
 */
void
RtBaryNormalize(RtBaryTransform bary)
{
    RwReal             length2;

    RWAPIFUNCTION(RWSTRING("RtBaryNormalize"));

    length2 = (bary[0][0] * bary[0][0] +
               bary[1][0] * bary[1][0] + bary[2][0] * bary[2][0]);
    if (0 < length2)
    {
        RwReal             factor;

        RWASSERT(0 < length2);

#if (defined(RW_USE_SPF))
        factor = (RwReal)sqrtf(1 / length2);
#else /* (defined(RW_USE_SPF)) */
        factor = (RwReal)sqrt(1 / length2);
#endif /* (defined(RW_USE_SPF)) */

        bary[0][0] *= factor;
        bary[1][0] *= factor;
        bary[2][0] *= factor;
        bary[3][0] *= factor;
    }

    length2 = (bary[0][1] * bary[0][1] +
               bary[1][1] * bary[1][1] + bary[2][1] * bary[2][1]);
    if (0 < length2)
    {
        RwReal             factor;

        RWASSERT(0 < length2);

#if (defined(RW_USE_SPF))
        factor = (RwReal)sqrtf(1 / length2);
#else /* (defined(RW_USE_SPF)) */
        factor = (RwReal)sqrt(1 / length2);
#endif /* (defined(RW_USE_SPF)) */

        bary[0][1] *= factor;
        bary[1][1] *= factor;
        bary[2][1] *= factor;
        bary[3][1] *= factor;
    }

    length2 = (bary[0][2] * bary[0][2] +
               bary[1][2] * bary[1][2] + bary[2][2] * bary[2][2]);

    if (0 < length2)
    {
        RwReal             factor;

        RWASSERT(0 < length2);

#if (defined(RW_USE_SPF))
        factor = (RwReal)sqrtf(1 / length2);
#else /* (defined(RW_USE_SPF)) */
        factor = (RwReal)sqrt(1 / length2);
#endif /* (defined(RW_USE_SPF)) */

        bary[0][2] *= factor;
        bary[1][2] *= factor;
        bary[2][2] *= factor;
        bary[3][2] *= factor;
    }

    RWRETURNVOID();
}

#if ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) )

/**
 * \ingroup rtbary
 * \ref RtBaryWeightsFromV3d finds the barycentric weights corresponding
 * to a specifed point in a triangle with a given 4x4 barycentric
 * transform matrix.  The first three returned weights w0, w1, w2 apply to
 * the triangle vertices v0, v1, v2 specified to \ref RtBaryGetTransform.
 * The final weight holds the height of the point above the
 * plane supporting the triangle.
 *
 * \see RtBaryWeightsFromV3d
 * \see RtBaryV3dFromWeights
 */
void
RtBaryWeightsFromV3d(RtBaryV4d out,
                     RtBaryTransform mat,
                     const RwV3d * const in)
{
    RWAPIFUNCTION(RWSTRING("RtBaryWeightsFromV3d"));

    RtBaryWeightsFromV3dMacro(out, mat, in);

    RWRETURNVOID();
}

/**
 * \ingroup rtbary
 * \ref RtBaryWeightsFromEdge
 *
 * TODO!!!
 */
void
RtBaryWeightsFromEdge(RtBaryV4d out,
                      RtBaryTransform mat,
                      const RwV3d * const in)
{
    RWAPIFUNCTION(RWSTRING("RtBaryWeightsFromEdge"));

    RtBaryWeightsFromEdgeMacro(out, mat, in);

    RWRETURNVOID();
}

/**
 * \ingroup rtbary
 * \ref RtBaryV3dFromWeights finds the point corresponding
 * to the specified barycentric weights for a specified triangle.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param out     receives the point
 * \param weights barycentric weights
 * \param v0      first vertex of triangle
 * \param v1      second vertex of triangle
 * \param v2      third vertex of triangle
 *
 * \see RtBaryGetTransform
 * \see RtBaryWeightsFromV3d
 */
void
RtBaryV3dFromWeights(RwV3d * const out,
                     const RtBaryV4d weights,
                     const RwV3d * const v0,
                     const RwV3d * const v1,
                     const RwV3d * const v2)
{

    RWAPIFUNCTION(RWSTRING("RtBaryV3dFromWeights"));

    RtBaryV3dFromWeightsMacro(out, weights, v0, v1, v2);

    RWRETURNVOID();
}


#endif /* ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) ) */


