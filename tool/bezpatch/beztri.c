/*
 * topic Bezier patch library
 *
 * This library provides effcient evaluation of
 * Cubic Bezier patches
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <rwcore.h>
#include <rpdbgerr.h>

#include "rtbezpat.h"
#include "beztri.h"

/**
 * \ingroup rtbezpatch
 * \ref RtBezierTriangleControlFit3d returns the control points
 * for a Bezier triangle fitted to pass through the supplied
 * sample points,
 * addressing x, y, and z coordinates.
 *
 * \param T    Output Bezier control point matrix
 * \param P    Input sample point matrix
 */
void
RtBezierTriangleControlFit3d(RtBezierMatrix T, RtBezierMatrix P)
{
    RWAPIFUNCTION(RWSTRING("RtBezierTriangleControlFit3d"));
    RtBezierTriangleControlFit3dMacro(T, P);
    RWRETURNVOID();
}
