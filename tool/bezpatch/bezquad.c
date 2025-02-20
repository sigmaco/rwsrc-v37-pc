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
#include "bezquad.h"

#if (!defined(S_RAD))
#define S_RAD ( rwPI * _R4 )
#endif /* (!defined(S_RAD)) */

#define INVALID_LENGTH (((RwReal)1) / ((RwReal)(1<<5)))
#define _PERTURB       (((RwReal)1) / ((RwReal)(1<<5)))


/**
 * \ingroup rtbezpatch
 * \ref RtBezierQuadControlFit3d returns the control points
 * for a Bezier quadrilateral fitted to pass through the supplied
 * sample points,
 * addressing x, y, and z coordinates.
 *
 * \param B    Output Bezier control point matrix
 * \param P    Input sample point matrix
 */
void
RtBezierQuadControlFit3d(RtBezierMatrix B, RtBezierMatrix P)
{
    RWAPIFUNCTION(RWSTRING("RtBezierQuadControlFit3d"));
    RtBezierQuadControlFit3dMacro(B, P);
    RWRETURNVOID();
}

/**
 * \ingroup rtbezpatch
 * \ref RtBezierQuadBernsteinWeight3d returns a Bernstein weighted
 * matrix for an input control point matrix,
 * addressing x, y, and z coordinates.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param W     Output difference matrix
 * \param B     Input Bezier matrix
 *
 * \see RtBezierQuadDifferenceStepU3d
 * \see RtBezierQuadDifferenceStepV3d
 * \see RtBezierQuadSample3d
 */
void
RtBezierQuadBernsteinWeight3d(RtBezierMatrix W, RtBezierMatrix B)
{
    RWAPIFUNCTION(RWSTRING("RtBezierQuadBernsteinWeight3d"));
    RtBezierQuadBernsteinWeight3dMacro(W, B);
    RWRETURNVOID();
}

/**
 * \ingroup rtbezpatch
 * \ref RtBezierQuadBernsteinWeight4d returns a Bernstein weighted
 * matrix for an input control point matrix,
 * addressing x, y, z, and w coordinates.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param W     Output difference matrix
 * \param B     Input Bezier matrix
 *
 * \see RtBezierQuadDifferenceStepU4d
 * \see RtBezierQuadDifferenceStepV4d
 * \see RtBezierQuadSample3d
 */
void
RtBezierQuadBernsteinWeight4d(RtBezierMatrix W, RtBezierMatrix B)
{
    RWAPIFUNCTION(RWSTRING("RtBezierQuadBernsteinWeight4d"));
    RtBezierQuadBernsteinWeight4dMacro(W, B);
    RWRETURNVOID();
}

/**
 * \ingroup rtbezpatch
 * \ref RtBezierQuadPointDifference3d returns a difference
 * matrix for an input Bernstein weight matrix
 * at an arbitrary initial parameter point,
 * addressing x, y, and z coordinates.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param D         Output difference matrix
 * \param W         Input Bernstein weighted matrix
 * \param PointU    Patch pointu coordinate
 * \param PointV    Patch point v coordinate
 * \param StepU     Patch u coordinate difference per step
 * \param StepV     Patch v coordinate difference per step
 *
 * \see RtBezierQuadDifferenceStepU3d
 * \see RtBezierQuadDifferenceStepV3d
 * \see RtBezierQuadSample3d
 */
void
RtBezierQuadPointDifference3d(RtBezierMatrix D, RtBezierMatrix W,
                              RwReal PointU, RwReal PointV,
                              RwReal StepU, RwReal StepV)
{
    RWAPIFUNCTION(RWSTRING("RtBezierQuadPointDifference3d"));
    RtBezierQuadPointDifference3dMacro(D, W,
                                       PointU, PointV, StepU, StepV);
    RWRETURNVOID();
}

/**
 * \ingroup rtbezpatch
 * \ref RtBezierQuadPointDifference4d returns a difference
 * matrix for an input Bernstein weight matrix
 * at an arbitrary initial parameter point,
 * addressing x, y, z, and w coordinates.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param D         Output difference matrix
 * \param W         Input Bernstein weighted matrix
 * \param PointU    Patch point u coordinate
 * \param PointV    Patch point v coordinate
 * \param StepU     Patch u coordinate difference per step
 * \param StepV     Patch v coordinate difference per step
 *
 * \see RtBezierQuadDifferenceStepU4d
 * \see RtBezierQuadDifferenceStepV4d
 */
void
RtBezierQuadPointDifference4d(RtBezierMatrix D,
                              RtBezierMatrix W,
                              RwReal PointU,
                              RwReal PointV, RwReal StepU, RwReal StepV)
{
    RWAPIFUNCTION(RWSTRING("RtBezierQuadPointDifference4d"));
    RtBezierQuadPointDifference4dMacro(D, W,
                                       PointU, PointV, StepU, StepV);
    RWRETURNVOID();
}

/**
 * \ingroup rtbezpatch
 * \ref RtBezierQuadOriginDifference3d returns a difference
 * matrix for an input Bernstein weight matrix
 * at the parameter origin,
 * addressing x, y, and z coordinates.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param D         Output difference matrix
 * \param W         Input Bernstein weighted matrix
 * \param StepU     Patch u coordinate difference per step
 * \param StepV     Patch v coordinate difference per step
 *
 * \see RtBezierQuadDifferenceStepU3d
 * \see RtBezierQuadDifferenceStepV3d
 * \see RtBezierQuadSample3d
 */
void
RtBezierQuadOriginDifference3d(RtBezierMatrix D,
                               RtBezierMatrix W,
                               RwReal StepU, RwReal StepV)
{
    RWAPIFUNCTION(RWSTRING("RtBezierQuadOriginDifference3d"));
    RtBezierQuadOriginDifference3dMacro(D, W, StepU, StepV);
    RWRETURNVOID();
}

/**
 * \ingroup rtbezpatch
 * \ref RtBezierQuadOriginDifference4d returns a difference
 * matrix for an input Bernstein weight matrix
 * at the parameter origin,
 * addressing x, y, z, and w coordinates.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param D         Output difference matrix
 * \param W         Input Bernstein weighted matrix
 * \param StepU     Patch u coordinate difference per step
 * \param StepV     Patch v coordinate difference per step
 *
 * \see RtBezierQuadDifferenceStepU4d
 * \see RtBezierQuadDifferenceStepV4d
 */
void
RtBezierQuadOriginDifference4d(RtBezierMatrix D,
                               RtBezierMatrix W,
                               RwReal StepU, RwReal StepV)
{
    RWAPIFUNCTION(RWSTRING("RtBezierQuadOriginDifference4d"));
    RtBezierQuadOriginDifference4dMacro(D, W, StepU, StepV);
    RWRETURNVOID();
}

/**
 * \ingroup rtbezpatch
 * \ref RtBezierQuadTangent returns a 16 control point matrix for
 * the tangents over a Bezier quadrilateral.
 *
 * \param D     Matrix of 16 control points for tangents
 *              in the parameter direction theta
 * \param theta Parameter direction theta
 * \param P     Matrix of 16 control points for a Bezier quadrilateral patch
 *
 */
void
RtBezierQuadTangent(RtBezierMatrix D, RwReal theta, RtBezierMatrix P)
{
    RWAPIFUNCTION(RWSTRING("RtBezierQuadTangent"));

    RtBezierQuadTangentMacro(D, theta, P);

    RWRETURNVOID();
}

/**
 * \ingroup rtbezpatch
 * \ref RtBezierQuadTangentPair returns 2 matrices, of 16 control points each,
 * the tangents over a Bezier quadrilateral patch.
 *
 * \param Dt    Matrix of 16 control points for tangents
 *              in the parameter direction theta
 * \param Dp    Matrices of 16 control points for tangents
 *              in the parameter direction at right angles to theta
 * \param theta Parameter direction theta
 * \param P     Matrix of 16 control points for a Bezier quadrilateral patch
 *
 */
void
RtBezierQuadTangentPair(RtBezierMatrix Dt, RtBezierMatrix Dp,
                        RwReal theta, RtBezierMatrix P)
{
    RWAPIFUNCTION(RWSTRING("RtBezierQuadTangentPair"));

    RtBezierQuadTangentPairMacro(Dt, Dp, theta, P);

    RWRETURNVOID();
}

/**
 * \ingroup rtbezpatch
 * \ref RtBezierQuadGetNormals calculates the surface normals
 *  corresponding to the control points of a Bezier quadrilateral patch.
 *
 * \param N     Output matrix of normals
 * \param B     Input matrix of 16 control points for a Bezier quadrilateral patch
 */
void
RtBezierQuadGetNormals(RtBezierMatrix N, RtBezierMatrix B)
{
    const RwReal        stepU = _R3;
    const RwReal        stepV = _R3;
    const RwReal        stepPerturbU =
        (((RwReal) 1) - 3 * _PERTURB) * _R3;
    const RwReal        stepPerturbV =
        (((RwReal) 1) - 3 * _PERTURB) * _R3;
    RtBezierMatrix      Weight;
    RtBezierMatrix      ForwardS;
    RtBezierMatrix      ForwardPerturbS;
    RtBezierMatrix      ForwardT;
    RtBezierMatrix      ForwardPerturbT;
    RwInt32             j;

    RWAPIFUNCTION(RWSTRING("RtBezierQuadGetNormals"));

    RtBezierQuadTangentPair(ForwardS, ForwardT, S_RAD, B);

    RtBezierQuadBernsteinWeight3d(Weight, ForwardS);
    RtBezierQuadPointDifference3d(ForwardS, Weight,
                                  0, 0, stepU, stepV);
    RtBezierQuadPointDifference3d(ForwardPerturbS,
                                  Weight,
                                  _PERTURB,
                                  _PERTURB,
                                  stepPerturbU, stepPerturbV);

    RtBezierQuadBernsteinWeight3d(Weight, ForwardT);
    RtBezierQuadPointDifference3d(ForwardT, Weight,
                                  0, 0, stepU, stepV);
    RtBezierQuadPointDifference3d(ForwardPerturbT,
                                  Weight,
                                  _PERTURB,
                                  _PERTURB,
                                  stepPerturbU, stepPerturbV);

    /* Work row by row */
    for (j = 0; j <= 3; j++)
    {
        RtBezierRow         RowS;
        RtBezierRow         RowT;
        RtBezierRow         RowPerturbS;
        RtBezierRow         RowPerturbT;
        RwInt32             i;

        /* Prime differences for this row */

        RowS[0] = ForwardS[0][0];
        RowS[1] = ForwardS[0][1];
        RowS[2] = ForwardS[0][2];
        RowS[3] = ForwardS[0][3];

        RowT[0] = ForwardT[0][0];
        RowT[1] = ForwardT[0][1];
        RowT[2] = ForwardT[0][2];
        RowT[3] = ForwardT[0][3];

        RowPerturbS[0] = ForwardPerturbS[0][0];
        RowPerturbS[1] = ForwardPerturbS[0][1];
        RowPerturbS[2] = ForwardPerturbS[0][2];
        RowPerturbS[3] = ForwardPerturbS[0][3];

        RowPerturbT[0] = ForwardPerturbT[0][0];
        RowPerturbT[1] = ForwardPerturbT[0][1];
        RowPerturbT[2] = ForwardPerturbT[0][2];
        RowPerturbT[3] = ForwardPerturbT[0][3];

        /* Work element by element */
        for (i = 0; i <= 3; i++)
        {
            RwV3d               TangentS;
            RwV3d               TangentT;
            RwV3d               Normal;
            RwReal              factor;
            RwBool              Valid;

            RwUnitV3dFromBezierV4d(&TangentS, &RowS[0]);
            RwUnitV3dFromBezierV4d(&TangentT, &RowT[0]);
            RwV3dCrossProduct(&Normal, &TangentS, &TangentT);

#if (0 && defined(MONITORNORMAL))
            RWMESSAGE(("%d %d  [ %5.2f %5.2f %5.2f ]",
                       j, i, TangentS.x, TangentS.y, TangentS.z));
            RWMESSAGE(("%d %d  [ %5.2f %5.2f %5.2f ]",
                       j, i, TangentT.x, TangentT.y, TangentT.z));
            RWMESSAGE(("%d %d  [ %5.2f %5.2f %5.2f ]",
                       j, i, Normal.x, Normal.y, Normal.z));
#endif /* (0 && defined(MONITORNORMAL)) */

            factor = RwV3dDotProduct(&Normal, &Normal);

            Valid = (INVALID_LENGTH < factor);
            if (Valid)
            {
                factor = 1.0f / factor;
                rwSqrt(&factor, factor);

                RwV3dScale(&Normal, &Normal, factor);
            }
            else
            {
                /* See if perturbed sample gives longer result */
                RwV3d               TangentPerturbS;
                RwV3d               TangentPerturbT;
                RwV3d               NormalPerturb;
                RwReal              factorPerturb;

                /* Perturb towards patch centre */
                RwUnitV3dFromBezierV4d(&TangentPerturbS,
                                       &RowPerturbS[0]);
                RwUnitV3dFromBezierV4d(&TangentPerturbT,
                                       &RowPerturbT[0]);
                RwV3dCrossProduct(&NormalPerturb, &TangentPerturbS,
                                  &TangentPerturbT);
                factorPerturb = RwV3dDotProduct(&NormalPerturb,
                                                &NormalPerturb);

#if (0 && defined(MONITORNORMAL))
                RWMESSAGE(("%d %d  [ %5.2f %5.2f %5.2f ]",
                           j, i, TangentPerturbS.x, TangentPerturbS.y,
                           TangentPerturbS.z));
                RWMESSAGE(("%d %d  [ %5.2f %5.2f %5.2f ]", j, i,
                           TangentPerturbT.x, TangentPerturbT.y,
                           TangentT.z));
                RWMESSAGE(("%d %d  [ %5.2f %5.2f %5.2f ]", j, i,
                           NormalPerturb.x, NormalPerturb.y,
                           NormalPerturb.z));
#endif /* (0 && defined(MONITORNORMAL)) */

                Valid = (factor < factorPerturb);
                if (Valid)
                {
                    factorPerturb = 1.0f / factorPerturb;
                    rwSqrt(&factorPerturb, factorPerturb);

                    RwV3dScale(&Normal, &NormalPerturb, factorPerturb);
                }
                else
                {
                    factor = 1.0f / factor;
                    rwSqrt(&factor, factor);

                    RwV3dScale(&Normal, &Normal, factor);
                }
            }

            N[i][j].x = Normal.x;
            N[i][j].y = Normal.y;
            N[i][j].z = Normal.z;

#if (0 && defined(MONITORNORMAL))
            RWMESSAGE(("%d %d %.2f <- [ %5.2f %5.2f %5.2f ]",
                       j, i, factor, N[i][j].x, N[i][j].y, N[i][j].z));
            /* RWASSERT(0 < RwV3dDotProduct(&N[i][j], &N[i][j])); */
#endif /* (0 && defined(MONITORNORMAL)) */

            RtBezierQuadDifferenceStepU3dMacro(RowS);
            RtBezierQuadDifferenceStepU3dMacro(RowPerturbS);

            RtBezierQuadDifferenceStepU3dMacro(RowT);
            RtBezierQuadDifferenceStepU3dMacro(RowPerturbT);

        }                      /* for (i = 0; i <= 3; i++) */

        RtBezierQuadDifferenceStepV3dMacro(ForwardS);
        RtBezierQuadDifferenceStepV3dMacro(ForwardPerturbS);

        RtBezierQuadDifferenceStepV3dMacro(ForwardT);
        RtBezierQuadDifferenceStepV3dMacro(ForwardPerturbT);

    }                          /* for (j = 0; j <= 3; j++) */

    RWRETURNVOID();
}

#if ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) )

/**
 * \ingroup rtbezpatch
 * \ref RtBezierQuadSample3d returns a point on a Bezier patch
 * at the specified parameter point,
 * addressing x, y, and z coordinates.
 *
 * \param out  Output point on Bezier patch
 * \param B  Matrix of Bezier control points
 * \param u  patch coordinate in [0,1]
 * \param v  patch coordinate in [0,1]
 *
 * \see RtBezierQuadDifferenceStepU3d
 * \see RtBezierQuadDifferenceStepV3d
 *
 * The following code illustrates the use of \ref RtBezierQuadSample3d
 \verbatim
    static RtBezierMatrix control =
    {
        {{00,00,00,-1}, {30,00,00,-1}, {60,00,00,-1}, {90,00,00,-1}},
        {{00,30,00,-1}, {30,30,30,-1}, {60,30,00,-1}, {90,30,00,-1}},
        {{00,60,00,-1}, {30,60,90,-1}, {60,60,60,-1}, {90,60,00,-1}},
        {{00,90,00,-1}, {30,90,00,-1}, {60,90,00,-1}, {90,90,90,-1}}
    };
    RwInt32             i;
    RwInt32             j;
    RwReal              u;
    RwReal              v;
    RwV3d               p;

    for (j = 0; j <= 8; j++)
    {
        v = ((RwReal) j) / ((RwReal) 8);

        for (i = 0; i <= 8; i++)
        {
            u = ((RwReal) i) / ((RwReal) 8);

            RtBezierQuadSample3d(&p, control, u, v);

            // p now contains the point at (u,v) on the Bezier surface
        }
    }
   \endverbatim
 */
void
RtBezierQuadSample3d(RwV3d * out, RtBezierMatrix B, RwReal u, RwReal v)
{
    RWAPIFUNCTION(RWSTRING("RtBezierQuadSample3d"));
    RtBezierQuadSample3dMacro(out, B, u, v);
    RWRETURNVOID();
}

/**
 * \ingroup rtbezpatch
 * \ref RtBezierQuadDifferenceStepU3d updates a difference row for
 * a step in the u patch parameter
 * addressing x, y, and z coordinates.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param row  Difference row to update for a step in the u patch coordinate
 *
 * \see RtBezierQuadDifferenceStepV3d
 * \see RtBezierQuadBernsteinWeight3d
 * \see RtBezierQuadOriginDifference3d
 * \see RtBezierQuadSample3d
 *
 * The following code illustrates the use of \ref RtBezierQuadDifferenceStepU3d
 \verbatim
    static RtBezierMatrix control =
    {
        {{00,00,00,-1}, {30,00,00,-1}, {60,00,00,-1}, {90,00,00,-1}},
        {{00,30,00,-1}, {30,30,30,-1}, {60,30,00,-1}, {90,30,00,-1}},
        {{00,60,00,-1}, {30,60,90,-1}, {60,60,60,-1}, {90,60,00,-1}},
        {{00,90,00,-1}, {30,90,00,-1}, {60,90,00,-1}, {90,90,90,-1}}
    };
    RwInt32             j;
    RtBezierMatrix      weight;
    RtBezierMatrix      difference;

    RtBezierQuadBernsteinWeight3d(weight, control);

    RtBezierQuadOriginDifference3d(difference, weight,
                                  ((RwReal) 1) / ((RwReal) 8),
                                  ((RwReal) 1) / ((RwReal) 8));

    for (j = 0; j <= 8; j++)
    {
        const RwReal        v = ((RwReal) j) / ((RwReal) resV);
        RtBezierRow         row;
        RwInt32             i;

        row[0] = difference[0][0];
        row[1] = difference[0][1];
        row[2] = difference[0][2];
        row[3] = difference[0][3];

        for (i = 0; i <= 8; i++)
        {
            const RwReal    u = ((RwReal) i) / ((RwReal) 8);

            // row[0] now contains the point at (u,v) on the Bezier surface

            RtBezierQuadDifferenceStepU3d(row);
        }
        RtBezierQuadDifferenceStepV3d(difference);
    }
   \endverbatim
 */
void
RtBezierQuadDifferenceStepU3d(RtBezierRow row)
{
    RWAPIFUNCTION(RWSTRING("RtBezierQuadDifferenceStepU3d"));
    RtBezierQuadDifferenceStepU3dMacro(row);
    RWRETURNVOID();
}

/**
 * \ingroup rtbezpatch
 * \ref RtBezierQuadDifferenceStepU4d updates a difference row for
 * a step in the u patch parameter,
 * addressing x, y, z, and w coordinates.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param row  Difference row to update for a step in the u patch coordinate
 *
 * \see RtBezierQuadDifferenceStepV4d
 * \see RtBezierQuadBernsteinWeight4d
 * \see RtBezierQuadOriginDifference4d
 *
 * The following code illustrates the use of \ref RtBezierQuadDifferenceStepU4d
 \verbatim
    static RtBezierMatrix control =
    {
        {{00,00,00,-1}, {30,00,00,-1}, {60,00,00,-1}, {90,00,00,-1}},
        {{00,30,00,-1}, {30,30,30,-1}, {60,30,00,-1}, {90,30,00,-1}},
        {{00,60,00,-1}, {30,60,90,-1}, {60,60,60,-1}, {90,60,00,-1}},
        {{00,90,00,-1}, {30,90,00,-1}, {60,90,00,-1}, {90,90,90,-1}}
    };
    RwInt32             j;
    RtBezierMatrix      weight;
    RtBezierMatrix      difference;

    RtBezierQuadBernsteinWeight4d(weight, control);

    RtBezierQuadOriginDifference4d(difference, weight,
                                  ((RwReal) 1) / ((RwReal) 8),
                                  ((RwReal) 1) / ((RwReal) 8));

    for (j = 0; j <= 8; j++)
    {
        const RwReal        v = ((RwReal) j) / ((RwReal) resV);
        RtBezierRow         row;
        RwInt32             i;

        row[0] = difference[0][0];
        row[1] = difference[0][1];
        row[2] = difference[0][2];
        row[3] = difference[0][3];

        for (i = 0; i <= 8; i++)
        {
            const RwReal    u = ((RwReal) i) / ((RwReal) 8);

            // row[0] now contains the point at (u,v) on the Bezier surface

            RtBezierQuadDifferenceStepU4d(row);
        }
        RtBezierQuadDifferenceStepV4d(difference);
    }
   \endverbatim
 */
void
RtBezierQuadDifferenceStepU4d(RtBezierRow row)
{
    RWAPIFUNCTION(RWSTRING("RtBezierQuadDifferenceStepU4d"));
    RtBezierQuadDifferenceStepU4dMacro(row);
    RWRETURNVOID();
}

/**
 * \ingroup rtbezpatch
 * \ref RtBezierQuadDifferenceStepV3d updates a difference matrix for
 * a step in the v patch parameter,
 * addressing x, y, and z coordinates.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param mat  Difference matrix to update for a step in the v patch coordinate
 *
 * \see RtBezierQuadDifferenceStepU3d
 * \see RtBezierQuadBernsteinWeight3d
 * \see RtBezierQuadOriginDifference3d
 * \see RtBezierQuadSample3d

 * The following code illustrates the use of \ref RtBezierQuadDifferenceStepV3d
 \verbatim
    static RtBezierMatrix control =
    {
        {{00,00,00,-1}, {30,00,00,-1}, {60,00,00,-1}, {90,00,00,-1}},
        {{00,30,00,-1}, {30,30,30,-1}, {60,30,00,-1}, {90,30,00,-1}},
        {{00,60,00,-1}, {30,60,90,-1}, {60,60,60,-1}, {90,60,00,-1}},
        {{00,90,00,-1}, {30,90,00,-1}, {60,90,00,-1}, {90,90,90,-1}}
    };
    RwInt32             j;
    RtBezierMatrix      weight;
    RtBezierMatrix      difference;

    RtBezierQuadBernsteinWeight3d(weight, control);

    RtBezierQuadOriginDifference3d(difference, weight,
                                  ((RwReal) 1) / ((RwReal) 8),
                                  ((RwReal) 1) / ((RwReal) 8));

    for (j = 0; j <= 8; j++)
    {
        const RwReal        v = ((RwReal) j) / ((RwReal) resV);
        RtBezierRow         row;
        RwInt32             i;

        row[0] = difference[0][0];
        row[1] = difference[0][1];
        row[2] = difference[0][2];
        row[3] = difference[0][3];

        for (i = 0; i <= 8; i++)
        {
            const RwReal    u = ((RwReal) i) / ((RwReal) 8);

            // row[0] now contains the point at (u,v) on the Bezier surface

            RtBezierQuadDifferenceStepU3d(row);
        }
        RtBezierQuadDifferenceStepV3d(difference);
    }
   \endverbatim
 */
void
RtBezierQuadDifferenceStepV3d(RtBezierMatrix mat)
{
    RWAPIFUNCTION(RWSTRING("RtBezierQuadDifferenceStepV3d"));
    RtBezierQuadDifferenceStepV3dMacro(mat);
    RWRETURNVOID();
}

/**
 * \ingroup rtbezpatch
 * \ref RtBezierQuadDifferenceStepV4d updates a difference matrix for
 * a step in the v patch parameter,
 * addressing x, y, z, and w coordinates.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param mat  Difference matrix to update for a step in the v patch coordinate
 *
 * \see RtBezierQuadDifferenceStepU4d
 * \see RtBezierQuadBernsteinWeight4d
 * \see RtBezierQuadOriginDifference4d
 *
 * The following code illustrates the use of \ref RtBezierQuadDifferenceStepV4d
 \verbatim
    static RtBezierMatrix control =
    {
        {{00,00,00,-1}, {30,00,00,-1}, {60,00,00,-1}, {90,00,00,-1}},
        {{00,30,00,-1}, {30,30,30,-1}, {60,30,00,-1}, {90,30,00,-1}},
        {{00,60,00,-1}, {30,60,90,-1}, {60,60,60,-1}, {90,60,00,-1}},
        {{00,90,00,-1}, {30,90,00,-1}, {60,90,00,-1}, {90,90,90,-1}}
    };
    RwInt32             j;
    RtBezierMatrix      weight;
    RtBezierMatrix      difference;

    RtBezierQuadBernsteinWeight4d(weight, control);

    RtBezierQuadOriginDifference4d(difference, weight,
                                  ((RwReal) 1) / ((RwReal) 8),
                                  ((RwReal) 1) / ((RwReal) 8));

    for (j = 0; j <= 8; j++)
    {
        const RwReal        v = ((RwReal) j) / ((RwReal) resV);
        RtBezierRow         row;
        RwInt32             i;

        row[0] = difference[0][0];
        row[1] = difference[0][1];
        row[2] = difference[0][2];
        row[3] = difference[0][3];

        for (i = 0; i <= 8; i++)
        {
            const RwReal    u = ((RwReal) i) / ((RwReal) 8);

            // row[0] now contains the point at (u,v) on the Bezier surface

            RtBezierQuadDifferenceStepU4d(row);
        }
        RtBezierQuadDifferenceStepV4d(difference);
    }
   \endverbatim
 */
void
RtBezierQuadDifferenceStepV4d(RtBezierMatrix mat)
{
    RWAPIFUNCTION(RWSTRING("RtBezierQuadDifferenceStepV4d"));
    RtBezierQuadDifferenceStepV4dMacro(mat);
    RWRETURNVOID();
}

#endif /* ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) ) */
