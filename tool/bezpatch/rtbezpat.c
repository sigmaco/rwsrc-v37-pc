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
 * \ref RtBezierQuadFromTriangle takes a matrix of 10 control points for
 * a tri patch, and returns a matrix of 16 control points for a
 * quad patch. The side of the tri patch defined by control points 9-8-6-3
 * becomes quad patch's diagonal 12-9-6-3.
 *
 * \param Q   Output control points of a Bezier quadrilateral patch
 * \param T   Input  control points of a Bezier triangular patch
 */
void
RtBezierQuadFromTriangle(RtBezierMatrix Q, RtBezierMatrix T)
{
    RWAPIFUNCTION(RWSTRING("RtBezierQuadFromTriangle"));

    RtBezierQuadFromTriangleMacro(Q, T);

    RWRETURNVOID();
}

#if (defined(RWDEBUG) && defined(RWVERBOSE))

#include <stdio.h>

void
_rtBezierGnuPlot(RtBezierMatrix B, RwChar * name, RwChar * title)
{
    FILE               *fptr;

    RWFUNCTION(RWSTRING("_rtBezierGnuPlot"));

    fptr = fopen(name, "w");

    if (fptr)
    {
        fprintf(fptr, "## Control Points\n");
        fprintf(fptr, "\n");

        fprintf(fptr, "# Control Points Row 0\n");
        fprintf(fptr, "\n");

        fprintf(fptr, "X00 = %f\n", B[0][0].x);
        fprintf(fptr, "Y00 = %f\n", B[0][0].y);
        fprintf(fptr, "Z00 = %f\n", B[0][0].z);
        fprintf(fptr, "\n");

        fprintf(fptr, "X01 = %f\n", B[0][1].x);
        fprintf(fptr, "Y01 = %f\n", B[0][1].y);
        fprintf(fptr, "Z01 = %f\n", B[0][1].z);
        fprintf(fptr, "\n");

        fprintf(fptr, "X02 = %f\n", B[0][2].x);
        fprintf(fptr, "Y02 = %f\n", B[0][2].y);
        fprintf(fptr, "Z02 = %f\n", B[0][2].z);
        fprintf(fptr, "\n");

        fprintf(fptr, "X03 = %f\n", B[0][3].x);
        fprintf(fptr, "Y03 = %f\n", B[0][3].y);
        fprintf(fptr, "Z03 = %f\n", B[0][3].z);
        fprintf(fptr, "\n");

        fprintf(fptr, "# Control Points Row 1\n");
        fprintf(fptr, "\n");

        fprintf(fptr, "X10 = %f\n", B[1][0].x);
        fprintf(fptr, "Y10 = %f\n", B[1][0].y);
        fprintf(fptr, "Z10 = %f\n", B[1][0].z);
        fprintf(fptr, "\n");

        fprintf(fptr, "X11 = %f\n", B[1][1].x);
        fprintf(fptr, "Y11 = %f\n", B[1][1].y);
        fprintf(fptr, "Z11 = %f\n", B[1][1].z);
        fprintf(fptr, "\n");

        fprintf(fptr, "X12 = %f\n", B[1][2].x);
        fprintf(fptr, "Y12 = %f\n", B[1][2].y);
        fprintf(fptr, "Z12 = %f\n", B[1][2].z);
        fprintf(fptr, "\n");

        fprintf(fptr, "X13 = %f\n", B[1][3].x);
        fprintf(fptr, "Y13 = %f\n", B[1][3].y);
        fprintf(fptr, "Z13 = %f\n", B[1][3].z);
        fprintf(fptr, "\n");

        fprintf(fptr, "# Control Points Row 2\n");
        fprintf(fptr, "\n");

        fprintf(fptr, "X20 = %f\n", B[2][0].x);
        fprintf(fptr, "Y20 = %f\n", B[2][0].y);
        fprintf(fptr, "Z20 = %f\n", B[2][0].z);
        fprintf(fptr, "\n");

        fprintf(fptr, "X21 = %f\n", B[2][1].x);
        fprintf(fptr, "Y21 = %f\n", B[2][1].y);
        fprintf(fptr, "Z21 = %f\n", B[2][1].z);
        fprintf(fptr, "\n");

        fprintf(fptr, "X22 = %f\n", B[2][2].x);
        fprintf(fptr, "Y22 = %f\n", B[2][2].y);
        fprintf(fptr, "Z22 = %f\n", B[2][2].z);
        fprintf(fptr, "\n");

        fprintf(fptr, "X23 = %f\n", B[2][3].x);
        fprintf(fptr, "Y23 = %f\n", B[2][3].y);
        fprintf(fptr, "Z23 = %f\n", B[2][3].z);
        fprintf(fptr, "\n");

        fprintf(fptr, "# Control Points Row 3\n");
        fprintf(fptr, "\n");

        fprintf(fptr, "X30 = %f\n", B[3][0].x);
        fprintf(fptr, "Y30 = %f\n", B[3][0].y);
        fprintf(fptr, "Z30 = %f\n", B[3][0].z);
        fprintf(fptr, "\n");

        fprintf(fptr, "X31 = %f\n", B[3][1].x);
        fprintf(fptr, "Y31 = %f\n", B[3][1].y);
        fprintf(fptr, "Z31 = %f\n", B[3][1].z);
        fprintf(fptr, "\n");

        fprintf(fptr, "X32 = %f\n", B[3][2].x);
        fprintf(fptr, "Y32 = %f\n", B[3][2].y);
        fprintf(fptr, "Z32 = %f\n", B[3][2].z);
        fprintf(fptr, "\n");

        fprintf(fptr, "X33 = %f\n", B[3][3].x);
        fprintf(fptr, "Y33 = %f\n", B[3][3].y);
        fprintf(fptr, "Z33 = %f\n", B[3][3].z);
        fprintf(fptr, "\n");

        fprintf(fptr, "## Weighted Control Points\n");
        fprintf(fptr, "# Weighted Control Points Row 0\n");
        fprintf(fptr,
                "WX00 = 3*X02+X00-3*X01-X03+3*(3*X11-X10-3*X12+X13)-3*(3*X21-X20-3*X22+X23)+3*X31-X30-3*X32+X33\n");
        fprintf(fptr,
                "WY00 = 3*Y02+Y00-3*Y01-Y03+3*(3*Y11-Y10-3*Y12+Y13)-3*(3*Y21-Y20-3*Y22+Y23)+3*Y31-Y30-3*Y32+Y33\n");
        fprintf(fptr,
                "WZ00 = 3*Z02+Z00-3*Z01-Z03+3*(3*Z11-Z10-3*Z12+Z13)-3*(3*Z21-Z20-3*Z22+Z23)+3*Z31-Z30-3*Z32+Z33\n");
        fprintf(fptr, "\n");

        fprintf(fptr,
                "WX01 = 6*X01-3*X00-3*X02+3*(3*X10-6*X11+3*X12)-3*(3*X20-6*X21+3*X22)+3*X30-6*X31+3*X32\n");
        fprintf(fptr,
                "WY01 = 6*Y01-3*Y00-3*Y02+3*(3*Y10-6*Y11+3*Y12)-3*(3*Y20-6*Y21+3*Y22)+3*Y30-6*Y31+3*Y32\n");
        fprintf(fptr,
                "WZ01 = 6*Z01-3*Z00-3*Z02+3*(3*Z10-6*Z11+3*Z12)-3*(3*Z20-6*Z21+3*Z22)+3*Z30-6*Z31+3*Z32\n");
        fprintf(fptr, "\n");

        fprintf(fptr,
                "WX02 = 3*X00-3*X01+3*(3*X11-3*X10)+3*(3*X20-3*X21)+3*X31-3*X30\n");
        fprintf(fptr,
                "WY02 = 3*Y00-3*Y01+3*(3*Y11-3*Y10)+3*(3*Y20-3*Y21)+3*Y31-3*Y30\n");
        fprintf(fptr,
                "WZ02 = 3*Z00-3*Z01+3*(3*Z11-3*Z10)+3*(3*Z20-3*Z21)+3*Z31-3*Z30\n");
        fprintf(fptr, "\n");

        fprintf(fptr, "WX03 = 3*X10-X00-3*X20+X30\n");
        fprintf(fptr, "WY03 = 3*Y10-Y00-3*Y20+Y30\n");
        fprintf(fptr, "WZ03 = 3*Z10-Z00-3*Z20+Z30\n");
        fprintf(fptr, "\n");

        fprintf(fptr, "# Weighted Control Points Row 1\n");
        fprintf(fptr,
                "WX10 = 3*(3*X01-X00-3*X02+X03)-6*(3*X11-X10-3*X12+X13)+3*(3*X21-X20-3*X22+X23)\n");
        fprintf(fptr,
                "WY10 = 3*(3*Y01-Y00-3*Y02+Y03)-6*(3*Y11-Y10-3*Y12+Y13)+3*(3*Y21-Y20-3*Y22+Y23)\n");
        fprintf(fptr,
                "WZ10 = 3*(3*Z01-Z00-3*Z02+Z03)-6*(3*Z11-Z10-3*Z12+Z13)+3*(3*Z21-Z20-3*Z22+Z23)\n");
        fprintf(fptr, "\n");

        fprintf(fptr,
                "WX11 = 3*(3*X00-6*X01+3*X02)-6*(3*X10-6*X11+3*X12)+3*(3*X20-6*X21+3*X22)\n");
        fprintf(fptr,
                "WY11 = 3*(3*Y00-6*Y01+3*Y02)-6*(3*Y10-6*Y11+3*Y12)+3*(3*Y20-6*Y21+3*Y22)\n");
        fprintf(fptr,
                "WZ11 = 3*(3*Z00-6*Z01+3*Z02)-6*(3*Z10-6*Z11+3*Z12)+3*(3*Z20-6*Z21+3*Z22)\n");
        fprintf(fptr, "\n");

        fprintf(fptr,
                "WX12 = 3*(3*X01-3*X00)+6*(3*X10-3*X11)+3*(3*X21-3*X20)\n");
        fprintf(fptr,
                "WY12 = 3*(3*Y01-3*Y00)+6*(3*Y10-3*Y11)+3*(3*Y21-3*Y20)\n");
        fprintf(fptr,
                "WZ12 = 3*(3*Z01-3*Z00)+6*(3*Z10-3*Z11)+3*(3*Z21-3*Z20)\n");
        fprintf(fptr, "\n");

        fprintf(fptr, "WX13 = 3*X00-6*X10+3*X20\n");
        fprintf(fptr, "WY13 = 3*Y00-6*Y10+3*Y20\n");
        fprintf(fptr, "WZ13 = 3*Z00-6*Z10+3*Z20\n");
        fprintf(fptr, "\n");

        fprintf(fptr, "# Weighted Control Points Row 2\n");
        fprintf(fptr,
                "WX20 = 3*(3*X11-X10-3*X12+X13)-3*(3*X01-X00-3*X02+X03)\n");
        fprintf(fptr,
                "WY20 = 3*(3*Y11-Y10-3*Y12+Y13)-3*(3*Y01-Y00-3*Y02+Y03)\n");
        fprintf(fptr,
                "WZ20 = 3*(3*Z11-Z10-3*Z12+Z13)-3*(3*Z01-Z00-3*Z02+Z03)\n");
        fprintf(fptr, "\n");

        fprintf(fptr,
                "WX21 = 3*(3*X10-6*X11+3*X12)-3*(3*X00-6*X01+3*X02)\n");
        fprintf(fptr,
                "WY21 = 3*(3*Y10-6*Y11+3*Y12)-3*(3*Y00-6*Y01+3*Y02)\n");
        fprintf(fptr,
                "WZ21 = 3*(3*Z10-6*Z11+3*Z12)-3*(3*Z00-6*Z01+3*Z02)\n");
        fprintf(fptr, "\n");

        fprintf(fptr, "WX22 = 3*(3*X00-3*X01)+3*(3*X11-3*X10)\n");
        fprintf(fptr, "WY22 = 3*(3*Y00-3*Y01)+3*(3*Y11-3*Y10)\n");
        fprintf(fptr, "WZ22 = 3*(3*Z00-3*Z01)+3*(3*Z11-3*Z10)\n");
        fprintf(fptr, "\n");

        fprintf(fptr, "WX23 = 3*X10-3*X00\n");
        fprintf(fptr, "WY23 = 3*Y10-3*Y00\n");
        fprintf(fptr, "WZ23 = 3*Z10-3*Z00\n");
        fprintf(fptr, "\n");

        fprintf(fptr, "# Weighted Control Points Row 3\n");
        fprintf(fptr, "WX30 = 3*X01-X00-3*X02+X03\n");
        fprintf(fptr, "WY30 = 3*Y01-Y00-3*Y02+Y03\n");
        fprintf(fptr, "WZ30 = 3*Z01-Z00-3*Z02+Z03\n");
        fprintf(fptr, "\n");

        fprintf(fptr, "WX31 = 3*X00-6*X01+3*X02\n");
        fprintf(fptr, "WY31 = 3*Y00-6*Y01+3*Y02\n");
        fprintf(fptr, "WZ31 = 3*Z00-6*Z01+3*Z02\n");
        fprintf(fptr, "\n");

        fprintf(fptr, "WX32 = 3*X01-3*X00\n");
        fprintf(fptr, "WY32 = 3*Y01-3*Y00\n");
        fprintf(fptr, "WZ32 = 3*Z01-3*Z00\n");
        fprintf(fptr, "\n");

        fprintf(fptr, "WX33 = X00\n");
        fprintf(fptr, "WY33 = Y00\n");
        fprintf(fptr, "WZ33 = Z00\n");
        fprintf(fptr, "\n");

        fprintf(fptr, "## Bezier Patch\n");
        fprintf(fptr, "\n");

        fprintf(fptr,
                "BezX(u,v) =                                         \\\n");
        fprintf(fptr,
                "    u**3*(WX00*v**3+WX01*v**2+WX02*v+WX03)          \\\n");
        fprintf(fptr,
                "   +u**2*(WX10*v**3+WX11*v**2+WX12*v+WX13)          \\\n");
        fprintf(fptr,
                "   +u*(WX20*v**3+WX21*v**2+WX22*v+WX23)+WX30*v**3   \\\n");
        fprintf(fptr, "   +   WX31*v**2+WX32*v+WX33\n");
        fprintf(fptr, "\n");

        fprintf(fptr,
                "BezY(u,v) =                                         \\\n");
        fprintf(fptr,
                "    u**3*(WY00*v**3+WY01*v**2+WY02*v+WY03)          \\\n");
        fprintf(fptr,
                "   +u**2*(WY10*v**3+WY11*v**2+WY12*v+WY13)          \\\n");
        fprintf(fptr,
                "   +u*(WY20*v**3+WY21*v**2+WY22*v+WY23)+WY30*v**3   \\\n");
        fprintf(fptr, "   +   WY31*v**2+WY32*v+WY33\n");
        fprintf(fptr, "\n");

        fprintf(fptr,
                "BezZ(u,v) =                                         \\\n");
        fprintf(fptr,
                "    u**3*(WZ00*v**3+WZ01*v**2+WZ02*v+WZ03)          \\\n");
        fprintf(fptr,
                "   +u**2*(WZ10*v**3+WZ11*v**2+WZ12*v+WZ13)          \\\n");
        fprintf(fptr,
                "   +u*(WZ20*v**3+WZ21*v**2+WZ22*v+WZ23)+WZ30*v**3   \\\n");
        fprintf(fptr, "   +   WZ31*v**2+WZ32*v+WZ33\n");
        fprintf(fptr, "\n");

        fprintf(fptr, "set parametric\n");
        fprintf(fptr, "set urange [0:1]\n");
        fprintf(fptr, "set vrange [0:1]\n");
        fprintf(fptr, "set title \"%s\"\n", title);
        fprintf(fptr, "set size square\n");
        fprintf(fptr, "set nokey\n");
        fprintf(fptr, "set isosamples 16,16\n");
        fprintf(fptr, "set view 30,60\n");

        fprintf(fptr,
                "splot BezX(u,v), BezY(u,v), BezZ(u,v) with lines,\\\n");
        fprintf(fptr, "      X00, Y00, Z00 with points,\\\n");
        fprintf(fptr, "      X01, Y01, Z01 with points,\\\n");
        fprintf(fptr, "      X02, Y02, Z02 with points,\\\n");
        fprintf(fptr, "      X03, Y03, Z03 with points,\\\n");
        fprintf(fptr, "      X10, Y10, Z10 with points,\\\n");
        fprintf(fptr, "      X11, Y11, Z11 with points,\\\n");
        fprintf(fptr, "      X12, Y12, Z12 with points,\\\n");
        fprintf(fptr, "      X13, Y13, Z13 with points,\\\n");
        fprintf(fptr, "      X20, Y20, Z20 with points,\\\n");
        fprintf(fptr, "      X21, Y21, Z21 with points,\\\n");
        fprintf(fptr, "      X22, Y22, Z22 with points,\\\n");
        fprintf(fptr, "      X23, Y23, Z23 with points,\\\n");
        fprintf(fptr, "      X30, Y30, Z30 with points,\\\n");
        fprintf(fptr, "      X31, Y31, Z31 with points,\\\n");
        fprintf(fptr, "      X32, Y32, Z32 with points,\\\n");
        fprintf(fptr, "      X33, Y33, Z33 with points\n");
        fprintf(fptr, "\n");

        fprintf(fptr, "pause -1 \"Press return to continue \"\n");

        fclose(fptr);
    }

    RWRETURNVOID();
}
#endif /* (defined(RWDEBUG) && defined(RWVERBOSE)) */

/**
 * \ingroup rtbezpatch
 * \page rtbezpatchoverview RtBezPat Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rtbezpat.h
 * \li \b Libraries: rwcore, rtbezpat
 * \li \b Plugin \b attachments: None
 *
 * \subsection bezpatoverview Overview
 * The toolkit is a group of void functions in RenderWare Graphics that
 * support the Bezier patch code. The functions are useful beyond the
 * facilities that RenderWare supports, so they are provided and documented in
 * the Toolkit for the developer to use. They use three data types in addition
 * to those documented in the Bezier Patch plugin, RpPatch
 *
 * -# \ref RtBezierV4d is the RenderWare vector that holds four values.
 *     The first three would typically be positional x, y and z.
 * -# \ref RtBezierRow is an array of four \ref RtBezierV4d's,
 *         one for each control
 *     point of a Bezier spline.
 *    Thus any row of control points across a patch can be stored in
 *    an \ref RtBezierRow.
 * -# \ref RtBezierMatrix is an array of four \ref RtBezierRow's.
 *    So the matrix holds all sixteen control points of a patch.
 *    It can hold the coordinates of quad patches or tri patches, and the
 *    vectors could represent surface points instead of control points.
 *    Alternatively it can represent normal vectors, tangents and
 *    "difference" values; all these are used in the Bezier Toolkit
 *    functions. \par
 *
 * A Bezier curve can be measured in "u". At the first control point u=0.
 * At the last, u=1 and at the other two control points, u=1/3 and u=2/3.
 * This is applied to patches, so that one edge is measured in "u".
 * The transverse edge is also a Bezier curve measured in "v",
 * with control points at v=0, v=1/3, v=2/3 and v=1.
 * This notation is used in some functions.
 *
 * The Toolkit functions fall into four groups. \par
 *
 * -# \ref RtBezierQuadFromTriangle is given the control points for a tri
 *    patch and returns an \ref RtBezierMatrix containing the control
 *    points for an equivalent Bezier quad patch.  This is a utility function
 *    to serve several other functions that take only quad patches as
 *    parameters.
 * -# \ref RtBezierQuadControlFit3d and \ref RtBezierTriangleControlFit3d
 *    calculate the control points that generate surfaces passing
 *    through the supplied on-curve sample control points.
 *    \ref RtBezierQuadSample3d converts from control points to surface points.
 *    This calculates any point on the patch's surface.
 * -# \ref RtBezierQuadBernsteinWeight3d and
 *    \ref RtBezierQuadBernsteinWeight4d
 *    pre-calculate many of the Bernstein matrix-multiplications needed
 *    for patches.
 *    The former addresses x, y, and z whilst
 *    the latter addresses x, y, z and an extra parameter, w.
 *    \ref RtBezierQuadPointDifference3d and
 *    \ref RtBezierQuadPointDifference4d
 *     calculate "forward differences" starting
 *    at an arbitrary parameter point.
 *    The former addresses x, y, and z whilst
 *    the latter addresses x, y, z and w.
 *     \ref RtBezierQuadOriginDifference3d and
 *     \ref RtBezierQuadOriginDifference4d
 *    calculate forward differences starting
 *    at the parameter origin.
 *    The former addresses x, y, and z whilst
 *    the latter addresses x, y, z and w.
 *    These speed up the calculation of patch coordinates.
 *    They are used with
 *    \ref RtBezierQuadDifferenceStepU3d \ref RtBezierQuadDifferenceStepU4d
 *    and
 *    \ref RtBezierQuadDifferenceStepV3d \ref RtBezierQuadDifferenceStepV4d
 *    which update values when stepping across or down a patch.
 * -# \ref RtBezierQuadTangent, \ref RtBezierQuadTangentPair and \ref
 *    RtBezierQuadGetNormals calculate the directions of normal vectors and
 *    of tangents at each vertex of a patch.
 */
