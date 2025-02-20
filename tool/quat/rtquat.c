/*
 * Arithmetic and Conversion functionality for Quaternions
 *
 * Data structures for Quaternions
 *
 * See
 * + Quaternions quickly transform coordinates without error buildup
 *   http://intratech.csl.com/rotate/index.html
 *   http://www.ednmag.com/ednmag/reg/1995/030295/05df3.htm
 * + Sir William Rowan Hamilton
 *   http://www-groups.dcs.st-and.ac.uk/~history/Mathematicians/Hamilton.html
 *
 * Copyright (c) Criterion Software Limited
 */

/**
 * \ingroup rtquat
 * \page rtquatoverview RtQuat Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rtquat.h
 * \li \b Libraries: rwcore, rtquat
 * \li \b Plugin \b attachments: None
 *
 * \subsection quatoverview Overview
 * This toolkit defines a new general-purpose quaternion datatype, RtQuat, and
 * provides a number of useful functions for performing quaternion arithmetic
 * and conversion to other formats.
 *
 * Legacy applications should
 * - include the rtquat.h header file, and
 * - link against rtquat.lib
 * if recompiled using this or future builds of RenderWare Graphics.
 */

/****************************************************************************
 *
 * Quaternion API
 * Copyright (C) 2000 Criterion Technologies
 *
 * Author  : John Spackman
 *
 * Module  : rtquat.c
 *
 * Purpose : Functions for creating and controlling quaternions
 *
 * Notes   : For details on interpolating rotation with
 *           Quaternions, see p360
 *           Advanced Animation and Rendering Techniques
 *           Alan Watt and Mark Watt
 *           Addison-Wesley 1993,
 *           ISBN 0-201-54412-1
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
#include <rtquat.h>

/****************************************************************************
 Local Defines
 */

#if (defined(RWDEBUG))
#define RWASSERTPOSITIVEORTHONORMAL(mpMatrix)                           \
MACRO_START                                                             \
{                                                                       \
    RwMatrixTolerance tolerance;                                        \
    RwEngineGetMatrixTolerances(&tolerance);                            \
    RWASSERT((mpMatrix) && (tolerance.Normal >=                         \
                          _rwMatrixNormalError((mpMatrix))));           \
    RWASSERT((mpMatrix) && (tolerance.Orthogonal >=                     \
                          _rwMatrixOrthogonalError((mpMatrix))));       \
    RWASSERT((mpMatrix) && ((((RwReal)1) - tolerance.Identity) <=       \
                          _rwMatrixDeterminant((mpMatrix))) );          \
}                                                                       \
MACRO_STOP
#endif /* (defined(RWDEBUG))     */

#if (!defined(RWASSERTPOSITIVEORTHONORMAL))
#define RWASSERTPOSITIVEORTHONORMAL(mpMatrix) /* No op */
#endif /* (!defined(RWASSERTPOSITIVEORTHONORMAL)) */

/****************************************************************************
 Public Globals
 */

#if (defined(RWDEBUG))
long                rtQuatStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Structures
 */

typedef RtQuat     *
    (*RwQuatFromDiagDomMatrixFunc) (RtQuat * const qpQuat,
                                    const RwMatrix * const mpMatrix);

/****************************************************************************
 Quarternion slerping code
 */

/****************************************************************************
 Local Defines
 */


#if (!defined(QUATMESSAGE))
#define QUATMESSAGE(_x)        /* do nothing */
#endif /* (!defined(QUATMESSAGE)) */

static RtQuat      *
QuatFromPositiveDiagMatrix(RtQuat * const qpQuat,
                           const RwMatrix * const mpMatrix, RwReal rTR)
{
    RwReal              rS;

    RWFUNCTION(RWSTRING("QuatFromPositiveDiagMatrix"));

    rS = rTR + (RwReal) (1);
    rwSqrt(&rS, rS);

    qpQuat->real = ((rS) * ((RwReal) (0.5f)));
    rS = (((RwReal) (0.5f)) / (rS));
    qpQuat->imag.x = (mpMatrix->up.z - mpMatrix->at.y) * rS;
    qpQuat->imag.y = (mpMatrix->at.x - mpMatrix->right.z) * rS;
    qpQuat->imag.z = (mpMatrix->right.y - mpMatrix->up.x) * rS;
    RWRETURN(qpQuat);
}

static RtQuat      *
QuatFromXDiagDomMatrix(RtQuat * const qpQuat,
                       const RwMatrix * const mpMatrix)
{

    RwReal              rS;

    RWFUNCTION(RWSTRING("QuatFromXDiagDomMatrix"));

    rS = (mpMatrix->right.x - (mpMatrix->up.y + mpMatrix->at.z)) +
        (RwReal) (1);
    rwSqrt(&rS, rS);

    qpQuat->imag.x = ((rS) * ((RwReal) (0.5f)));
    rS = (((RwReal) (0.5f)) / (rS));
    qpQuat->real = (mpMatrix->up.z - mpMatrix->at.y) * rS;
    qpQuat->imag.y = (mpMatrix->right.y + mpMatrix->up.x) * rS;
    qpQuat->imag.z = (mpMatrix->right.z + mpMatrix->at.x) * rS;

    RWRETURN(qpQuat);
}

static RtQuat      *
QuatFromYDiagDomMatrix(RtQuat * const qpQuat,
                       const RwMatrix * const mpMatrix)
{
    RwReal              rS;

    RWFUNCTION(RWSTRING("QuatFromYDiagDomMatrix"));

    rS = (mpMatrix->up.y - (mpMatrix->at.z + mpMatrix->right.x)) +
        (RwReal) (1);
    rwSqrt(&rS, rS);

    qpQuat->imag.y = ((rS) * ((RwReal) (0.5f)));
    rS = (((RwReal) (0.5f)) / (rS));
    qpQuat->real = (mpMatrix->at.x - mpMatrix->right.z) * rS;
    qpQuat->imag.z = (mpMatrix->up.z + mpMatrix->at.y) * rS;
    qpQuat->imag.x = (mpMatrix->up.x + mpMatrix->right.y) * rS;

    RWRETURN(qpQuat);
}

static RtQuat      *
QuatFromZDiagDomMatrix(RtQuat * const qpQuat,
                       const RwMatrix * const mpMatrix)
{
    RwReal              rS;

    RWFUNCTION(RWSTRING("QuatFromZDiagDomMatrix"));

    rS = (mpMatrix->at.z - (mpMatrix->right.x + mpMatrix->up.y)) +
        (RwReal) (1);
    rwSqrt(&rS, rS);

    qpQuat->imag.z = ((rS) * ((RwReal) (0.5f)));
    rS = (((RwReal) (0.5f)) / (rS));
    qpQuat->real = (mpMatrix->right.y - mpMatrix->up.x) * rS;
    qpQuat->imag.x = (mpMatrix->at.x + mpMatrix->right.z) * rS;
    qpQuat->imag.y = (mpMatrix->at.y + mpMatrix->up.z) * rS;

    RWRETURN(qpQuat);
}

/************************************************************************
 *                                                                      *
 * Function:   RtQuatConvertFromMatrix                                  *
 * Purpose:    Convert a homogeneous Cartesian into a quaternion matrix *
 * On entry:   Quaternion matrix, homogeneous Cartesian                 *
 * On exit:    Success?                                                 *
 *                                                                      *
 ************************************************************************/

/**
 * \ingroup rtquat
 * \ref RtQuatConvertFromMatrix converts to an RtQuat from an RwMatrix
 *
 * \param qpQuat  Target quaternion.
 * \param mpMatrix  Source matrix.
 *
 * \return Flags success
 *
 * \see RtQuatConvertToMatrix
 */
RwBool
RtQuatConvertFromMatrix(RtQuat * const qpQuat,
                        const RwMatrix * const mpMatrix)
{
    RwBool              result;

    RWAPIFUNCTION(RWSTRING("RtQuatConvertFromMatrix"));

    RWASSERT(qpQuat);
    RWASSERT(mpMatrix);

    RWASSERT(mpMatrix);
    RWASSERTPOSITIVEORTHONORMAL(mpMatrix);

    result = (qpQuat && mpMatrix);

    if (result)
    {
        const RwReal        rTR =
            mpMatrix->right.x + mpMatrix->up.y + mpMatrix->at.z;

        if (rTR > (RwReal) (0))
        {
            QuatFromPositiveDiagMatrix(qpQuat, mpMatrix, rTR);
        }
        else
        {
            const RwQuatFromDiagDomMatrixFunc QuatFromDiagDomMatrixFunc
                =
                ((mpMatrix->right.x >
                  mpMatrix->up.y) ? ((mpMatrix->right.x >
                                      mpMatrix->at.
                                      z) ? QuatFromXDiagDomMatrix :
                                     QuatFromZDiagDomMatrix)
                 : ((mpMatrix->up.y >
                     mpMatrix->at.
                     z) ? QuatFromYDiagDomMatrix :
                    QuatFromZDiagDomMatrix));

            QuatFromDiagDomMatrixFunc(qpQuat, mpMatrix);
        }
    }

    RWRETURN(result);
}

static RtQuat      *
QuatRotate(RtQuat * quat, const RwV3d * axis, RwReal angle)
{
    const RwReal        radians = angle * (rwPI / ((RwReal) 360));
    const RwReal        sin_radians = (RwReal) RwSin(radians);

    RWFUNCTION(RWSTRING("QuatRotate"));

    RWASSERT(quat);

    quat->real = (RwReal) RwCos(radians);

    RwV3dNormalize(&quat->imag, axis);
    RwV3dScale(&quat->imag, &quat->imag, sin_radians);

    RWRETURN(quat);
}

/**
 * \ingroup rtquat
 * \ref RtQuatRotate builds a rotation quaternion from the given axis
 * and angle of rotation.
 *
 * \param quat  Pointer to quat to apply rotation to.
 * \param axis  Pointer to vector specifying axis of rotation.
 * \param angle  Angle of rotation, in degrees.
 * \param combineOp  An \ref RwOpCombineType flag.
 *
 * Valid flags for "combineOp" are: rwCOMBINEREPLACE, rwCOMBINEPRECONCAT and rwCOMBINEPOSTCONCAT.
 *
 * \return Returns pointer to the new quat if successful or NULL if there
 * is an error.
 *
 * \see RtQuatQueryRotate
 *
 */
RtQuat             *
RtQuatRotate(RtQuat * quat,
             const RwV3d * axis, RwReal angle,
             RwOpCombineType combineOp)
{
    RWAPIFUNCTION(RWSTRING("RtQuatRotate"));
    RWASSERT(quat);
    RWASSERT(axis);

    if ((NULL != quat) && (NULL != axis))
    {
        switch (combineOp)
        {
            case rwCOMBINEREPLACE:
                quat = QuatRotate(quat, axis, angle);
                break;

            case rwCOMBINEPRECONCAT:
                {
                    RtQuat              compose;
                    RtQuat              source;

                    RtQuatAssign(&source, quat);
                    QuatRotate(&compose, axis, angle);

                    RtQuatMultiply(quat, &source, &compose);
                }
                break;

            case rwCOMBINEPOSTCONCAT:
                {
                    RtQuat              compose;
                    RtQuat              source;

                    RtQuatAssign(&source, quat);
                    QuatRotate(&compose, axis, angle);

                    RtQuatMultiply(quat, &compose, &source);
                }
                break;

            default:
                {
                    RWERROR((E_RW_BADPARAM,
                             RWSTRING("Invalid combination type")));
                    quat = (RtQuat *) NULL;
                }
                break;
        }
    }
    else
    {
        RWERROR((E_RW_NULLP));
        quat = (RtQuat *) NULL;
    }

    RWRETURN(quat);
}

/**
 * \ingroup rtquat
 * \ref RtQuatQueryRotate determines the rotation represented by a
 * quaternion.
 * The rotation is returned as a
 * unit vector along the axis of rotation, and an angle of rotation.
 * The rotation component has two possible
 * descriptions since a rotation about an axis of theta degrees is equivalent
 * to a rotation about an axis pointing in the opposite direction by an angle
 * of 360-theta in the reverse direction. The angle returned by
 * \ref RtQuatQueryRotate is always between 0 and 180 degrees and the direction
 * of the axis of rotation returned is chosen to ensure the angle is in this
 * range.
 *
 * \param quat  Pointer to quaternion to be investigated.
 * \param unitAxis  Pointer to vector which will receive
 *                  the unit direction vector along
 * axis of rotation.
 * \param angle  Pointer to RwReal which will receive
 *               angle of rotation in degrees.
 *
 * \return Returns pointer to the queried quaternion if successful or NULL if
 * there is an error.
 *
 * \see RtQuatRotate
 *
 */
const RtQuat       *
RtQuatQueryRotate(const RtQuat * quat, RwV3d * unitAxis, RwReal * angle)
{
    RWAPIFUNCTION(RWSTRING("RtQuatQueryRotate"));

    if ((NULL != quat) && (NULL != unitAxis) && (NULL != angle))
    {
        RwReal              R_sin_half_theta;
        RwReal              factor;
        RwReal              radians;

        R_sin_half_theta = RwV3dDotProduct(&quat->imag, &quat->imag);

        rwSqrt(&R_sin_half_theta, R_sin_half_theta);
        factor = ((R_sin_half_theta > (RwReal) 0) ?
                  (((RwReal) 1) / R_sin_half_theta) : ((RwReal) 0));
        radians = ((RwReal) RwATan2(R_sin_half_theta, quat->real));

        if (radians > rwPIOVER2)
        {
            const RwReal        invert = -factor;

            RwV3dScale(unitAxis, &quat->imag, invert);
            *angle = (rwPI - radians) * (((RwReal) 360) / rwPI);
        }
        else
        {
            RwV3dScale(unitAxis, &quat->imag, factor);
            *angle = radians * (((RwReal) 360) / rwPI);
        }

    }
    else
    {
        RWERROR((E_RW_NULLP));
        quat = (const RtQuat *) NULL;
    }

    RWRETURN(quat);
}

/**
 * \ingroup rtquat
 * \ref RtQuatTransformVectors uses the given quaternion describing a
 * transformation and applies it to the specified array of vectors. The
 * results are then placed in another array (which may be the same array
 * as the source).
 *
 * Note:
 * The source and destination array may be separate or coincident, but the
 * result is not defined if the source and destination overlap in any
 * other way (ie, if they are offset from one another but still overlap).
 *
 * \param vectorsOut Pointer to an array of RwV3ds which are to
 *                   receive the results of the vector transformation.
 * \param vectorsIn  Pointer to an array of RwV3ds which are to
 *                   supply to vectors to be transformed.
 * \param numPoints  Number of vectors to transform.
 * \param quat       Pointer to the quaternion supplying the transformation.
 *
 * \return pointer to the transformed vectors if successful,
 * or NULL if there is an error.
 *
 * \see RwV3dTransformPoints
 * \see RwV3dTransformVectors
 */
RwV3d              *
RtQuatTransformVectors(RwV3d * vectorsOut,
                       const RwV3d * vectorsIn,
                       const RwInt32 numPoints, const RtQuat * quat)
{
    RwV3d              *target = vectorsOut;
    const RwV3d        *source = vectorsIn;

    RWAPIFUNCTION(RWSTRING("RtQuatTransformVectors"));
    RWASSERT(vectorsOut);
    RWASSERT(vectorsIn);
    RWASSERT(quat);

    /*
     * See
     * http://intratech.csl.com/rotate/index.html
     */

    {
        const RwReal        CosTwoTheta = ((quat->real * quat->real) -
                                           RwV3dDotProduct(&quat->imag,
                                                           &quat->
                                                           imag));
        RwV3d               TwoSinThetaAxis;
        RwV3d               SinTwoThetaAxis;
        RwInt32             i;

        RwV3dAdd(&TwoSinThetaAxis, &quat->imag, &quat->imag);
        RwV3dScale(&SinTwoThetaAxis, &TwoSinThetaAxis, quat->real);

        /*
         * See
         * 4.4 Rotation of Vector by a Quaternion
         * http://intratech.csl.com/rotate/index.html#tth_sEc4.4
         */

        for (i = 0; i < numPoints; i++)
        {
            RwV3d   temp;

            const RwReal dot = RwV3dDotProduct(&TwoSinThetaAxis, source);

            /* Source and target might be coincident */
            RwV3dCrossProduct(&temp, &SinTwoThetaAxis, source);

            RwV3dAssign(target, &temp);
            RwV3dIncrementScaled(target, &quat->imag, dot);
            RwV3dIncrementScaled(target, source, CosTwoTheta);

            source++;
            target++;
        }
    }

    RWRETURN(vectorsOut);
}

/**
 * \ingroup rtquat
 * \ref RtQuatModulus returns the modulus of an RtQuat
 *
 * \param q Source Quaternion
 *
 * \return the modulus of the RtQuat
 *
 * \see RtQuatModulusSquared
 */
RwReal
RtQuatModulus(RtQuat * q)
{
    RwReal              result;

    RWAPIFUNCTION(RWSTRING("RtQuatModulus"));
    RWASSERT(NULL != q);

    RtQuatModulusMacro(result, q);

    RWRETURN(result);
}

#if ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) )

/**
 * \ingroup rtquat
 * \ref RtQuatInit initializes the components of an RtQuat
 *
 * \param result  Target Quaternion
 * \param x  First imaginary component
 * \param y  Second imaginary component
 * \param z  Third imaginary component
 * \param w  Real component
 *
 * \see RtQuatAssign
 */
void
RtQuatInit(RtQuat * result, RwReal x, RwReal y, RwReal z, RwReal w)
{
    RWAPIFUNCTION(RWSTRING("RtQuatInit"));
    RWASSERT(NULL != result);

    RtQuatInitMacro(result, x, y, z, w);

    RWRETURNVOID();
}

/**
 * \ingroup rtquat
 * \ref RtQuatAssign assigns to an RtQuat from an RtQuat
 *
 * \param to  Target Quaternion
 * \param from  Source Quaternion
 *
 * \see RtQuatInit
 */
void
RtQuatAssign(RtQuat * to, RtQuat * from)
{
    RWAPIFUNCTION(RWSTRING("RtQuatAssign"));
    RWASSERT(NULL != to);
    RWASSERT(NULL != from);

    RtQuatAssignMacro(to, from);

    RWRETURNVOID();
}

/**
 * \ingroup rtquat
 * \ref RtQuatAdd calcuates the sum of an RtQuat pair
 *
 * \param result  Target Quaternion
 * \param q1  First Source Quaternion
 * \param q2  Second Source Quaternion
 *
 * \see RtQuatSub
 */
void
RtQuatAdd(RtQuat * result, RtQuat * q1, RtQuat * q2)
{
    RWAPIFUNCTION(RWSTRING("RtQuatAdd"));
    RWASSERT(NULL != result);
    RWASSERT(NULL != q1);
    RWASSERT(NULL != q2);

    RtQuatAddMacro(result, q1, q2);

    RWRETURNVOID();
}

/**
 * \ingroup rtquat
 * \ref RtQuatIncrementRealPart increments the real component
 * of an RtQuat
 *
 * \param result  Target Quaternion
 * \param s  Scale Factor
 * \param q  Source Quaternion
 *
 * \see RtQuatDecrementRealPart
 */
void
RtQuatIncrementRealPart(RtQuat * result, RwReal s, RtQuat * q)
{
    RWAPIFUNCTION(RWSTRING("RtQuatIncrementRealPart"));
    RWASSERT(NULL != result);
    RWASSERT(NULL != q);

    RtQuatIncrementRealPartMacro(result, s, q);

    RWRETURNVOID();
}

/**
 * \ingroup rtquat
 * \ref RtQuatDecrementRealPart decrements the real component
 * of an RtQuat
 *
 * \param result  Target Quaternion
 * \param s  Shift Factor
 * \param q  Source Quaternion
 *
 * \see RtQuatIncrementRealPart
 */
void
RtQuatDecrementRealPart(RtQuat * result, RwReal s, RtQuat * q)
{
    RWAPIFUNCTION(RWSTRING("RtQuatDecrementRealPart"));
    RWASSERT(NULL != result);
    RWASSERT(NULL != q);

    RtQuatDecrementRealPartMacro(result, s, q);

    RWRETURNVOID();
}

/**
 * \ingroup rtquat
 * \ref RtQuatIncrement
 * increments an RtQuat by another RtQuat
 *
 * \param result  Target Quaternion
 * \param dq  Shift Quaternion
 *
 * \see RtQuatAdd
 */
void
RtQuatIncrement(RtQuat * result, RtQuat * dq)
{
    RWAPIFUNCTION(RWSTRING("RtQuatIncrement"));
    RWASSERT(NULL != result);
    RWASSERT(NULL != dq);

    RtQuatIncrementMacro(result, dq);

    RWRETURNVOID();
}

/**
 * \ingroup rtquat
 * \ref RtQuatSub calculates the difference of an RtQuat pair
 *
 * \param result  Target Quaternion
 * \param q1  First Source Quaternion
 * \param q2  Second Source Quaternion
 *
 * \see RtQuatAdd
 */
void
RtQuatSub(RtQuat * result, RtQuat * q1, RtQuat * q2)
{
    RWAPIFUNCTION(RWSTRING("RtQuatSub"));
    RWASSERT(NULL != result);
    RWASSERT(NULL != q1);
    RWASSERT(NULL != q2);

    RtQuatSubMacro(result, q1, q2);

    RWRETURNVOID();
}

/**
 * \ingroup rtquat
 * \ref RtQuatNegate negates an RtQuat to the additive inverse
 *
 * \param result  Target Quaternion
 * \param q  Source Quaternion
 *
 * \see RtQuatAssign
 */
void
RtQuatNegate(RtQuat * result, RtQuat * q)
{
    RWAPIFUNCTION(RWSTRING("RtQuatNegate"));
    RWASSERT(NULL != result);
    RWASSERT(NULL != q);

    RtQuatNegateMacro(result, q);

    RWRETURNVOID();
}

/**
 * \ingroup rtquat
 * \ref RtQuatConjugate conjugates an RtQuat to another RtQuat
 *
 * \param result  Target Quaternion
 * \param q  Source Quaternion
 *
 * \see RtQuatModulusSquared
 */
void
RtQuatConjugate(RtQuat * result, RtQuat * q)
{
    RWAPIFUNCTION(RWSTRING("RtQuatConjugate"));
    RWASSERT(NULL != result);
    RWASSERT(NULL != q);

    RtQuatConjugateMacro(result, q);

    RWRETURNVOID();
}

/**
 * \ingroup rtquat
 * \ref RtQuatScale scales an RtQuat by a factor to another RtQuat
 *
 * \param result  Target Quaternion
 * \param q  Source Quaternion
 * \param scale  Scale Factor
 *
 * \see RtQuatMultiply
 */
void
RtQuatScale(RtQuat * result, RtQuat * q, RwReal scale)
{
    RWAPIFUNCTION(RWSTRING("RtQuatScale"));
    RWASSERT(NULL != result);
    RWASSERT(NULL != q);

    RtQuatScaleMacro(result, q, scale);

    RWRETURNVOID();
}

/**
 * \ingroup rtquat
 * \ref RtQuatModulusSquared
 * returns the square of the modulus of an RtQuat
 *
 * \param q Source Quaternion
 *
 * \return the square of the modulus of the RtQuat
 *
 * \see RtQuatModulus
 */
RwReal
RtQuatModulusSquared(RtQuat * q)
{
    RwReal              result;

    RWAPIFUNCTION(RWSTRING("RtQuatModulusSquared"));
    RWASSERT(NULL != q);

    result = RtQuatModulusSquaredMacro(q);

    RWRETURN(result);
}

/**
 * \ingroup rtquat
 * \ref RtQuatMultiply calculates the (non-commutative) product of
 * an RtQuat pair
 *
 * \param result  Target Quaternion
 * \param q1  First Source Quaternion
 * \param q2  Second Source Quaternion
 *
 * \see RtQuatReciprocal
 */
void
RtQuatMultiply(RtQuat * result, RtQuat * q1, RtQuat * q2)
{
    RWAPIFUNCTION(RWSTRING("RtQuatMultiply"));
    RWASSERT(NULL != result);
    RWASSERT(NULL != q1);
    RWASSERT(NULL != q2);

    RtQuatMultiplyMacro(result, q1, q2);

    RWRETURNVOID();
}

/**
 * \ingroup rtquat
 * \ref RtQuatReciprocal recipricates an RtQuat to its
 * multiplicative inverse
 *
 * \param result  Target Quaternion
 * \param q  Source Quaternion
 *
 */
void
RtQuatReciprocal(RtQuat * result, RtQuat * q)
{
    RWAPIFUNCTION(RWSTRING("RtQuatReciprocal"));
    RWASSERT(NULL != result);
    RWASSERT(NULL != q);

    RtQuatReciprocalMacro(result, q);

    RWRETURNVOID();
}

/**
 * \ingroup rtquat
 * \ref RtQuatSquare calculates the square of an RtQuat
 *
 * \param result  Target Quaternion
 * \param q  Source Quaternion
 *
 * \see RtQuatSquareRoot
 */
void
RtQuatSquare(RtQuat * result, RtQuat * q)
{
    RWAPIFUNCTION(RWSTRING("RtQuatSquare"));
    RWASSERT(NULL != result);
    RWASSERT(NULL != q);

    RtQuatSquareMacro(result, q);

    RWRETURNVOID();
}

/**
 * \ingroup rtquat
 * \ref RtQuatSquareRoot calculates the square root of an RtQuat
 *
 * \param result  Target Quaternion
 * \param q  Source Quaternion
 *
 * \see RtQuatSquare
 */
void
RtQuatSquareRoot(RtQuat * result, RtQuat * q)
{
    RWAPIFUNCTION(RWSTRING("RtQuatSquareRoot"));
    RWASSERT(NULL != result);
    RWASSERT(NULL != q);

    RtQuatSquareRootMacro(result, q);

    RWRETURNVOID();
}

/**
 * \ingroup rtquat
 * \ref RtQuatLog calculates the logarithm of an RtQuat
 *
 * \param result  Target Quaternion
 * \param q  Source Quaternion
 *
 * \see RtQuatExp
 */
void
RtQuatLog(RtQuat * result, RtQuat * q)
{
    RWAPIFUNCTION(RWSTRING("RtQuatLog"));
    RWASSERT(NULL != result);
    RWASSERT(NULL != q);

    if ((NULL != result) && (NULL != q))
    {
        RtQuatLogMacro(result, q);
    }
    else
    {
        RWERROR((E_RW_NULLP));
        result = NULL;
    }

    RWRETURNVOID();
}

/**
 * \ingroup rtquat
 * \ref RtQuatExp calculates the exponential of an RtQuat
 *
 * \param result  Target Quaternion
 * \param q  Source Quaternion
 *
 * \see RtQuatLog
 */
void
RtQuatExp(RtQuat * result, RtQuat * q)
{
    RWAPIFUNCTION(RWSTRING("RtQuatExp"));
    RWASSERT(NULL != result);
    RWASSERT(NULL != q);

    if ((NULL != result) && (NULL != q))
    {
        RtQuatExpMacro(result, q);
    }
    else
    {
        RWERROR((E_RW_NULLP));
        result = NULL;
    }

    RWRETURNVOID();
}

/**
 * \ingroup rtquat
 * \ref RtQuatPow calculates the power of an RtQuat
 *
 * \param result  Target Quaternion
 * \param q  Source Quaternion
 * \param e  Power exponent
 *
 * \see RtQuatLog
 * \see RtQuatExp
 */
void
RtQuatPow(RtQuat * result, RtQuat * q, RwReal e)
{
    RWAPIFUNCTION(RWSTRING("RtQuatPow"));
    RWASSERT(NULL != result);
    RWASSERT(NULL != q);

    if ((NULL != result) && (NULL != q))
    {
        RtQuatPowMacro(result, q, e);
    }
    else
    {
        RWERROR((E_RW_NULLP));
        result = NULL;
    }

    RWRETURNVOID();
}

/**
 * \ingroup rtquat
 * \ref RtQuatUnitLog calculates the logarithm of a unitary RtQuat
 *
 * \param result  Target Quaternion
 * \param q  Source Quaternion
 *
 * \see RtQuatUnitExp
 */
void
RtQuatUnitLog(RtQuat * result, RtQuat * q)
{
    RWAPIFUNCTION(RWSTRING("RtQuatUnitLog"));
    RWASSERT(NULL != result);
    RWASSERT(NULL != q);

    if ((NULL != result) && (NULL != q))
    {
        RtQuatUnitLogMacro(result, q);
    }
    else
    {
        RWERROR((E_RW_NULLP));
        result = NULL;
    }

    RWRETURNVOID();
}

/**
 * \ingroup rtquat
 * \ref RtQuatUnitExp calculates the exponential of a unitary RtQuat
 *
 * \param result  Target Quaternion
 * \param q  Source Quaternion
 * \see RtQuatUnitLog
 *
 */
void
RtQuatUnitExp(RtQuat * result, RtQuat * q)
{
    RWAPIFUNCTION(RWSTRING("RtQuatUnitExp"));
    RWASSERT(NULL != result);
    RWASSERT(NULL != q);

    if ((NULL != result) && (NULL != q))
    {
        RtQuatUnitExpMacro(result, q);
    }
    else
    {
        RWERROR((E_RW_NULLP));
        result = NULL;
    }

    RWRETURNVOID();
}

/**
 * \ingroup rtquat
 * \ref RtQuatUnitPow calculates the power of a unitary RtQuat
 *
 * \param result  Target Quaternion
 * \param q  Source Quaternion
 * \param e  Power exponent
 *
 * \see RtQuatUnitLog
 * \see RtQuatUnitExp
 */
void
RtQuatUnitPow(RtQuat * result, RtQuat * q, RwReal e)
{
    RWAPIFUNCTION(RWSTRING("RtQuatUnitPow"));
    RWASSERT(NULL != result);
    RWASSERT(NULL != q);

    if ((NULL != result) && (NULL != q))
    {
        RtQuatUnitPowMacro(result, q, e);
    }
    else
    {
        RWERROR((E_RW_NULLP));
        result = NULL;
    }

    RWRETURNVOID();
}

/************************************************************************
 *                                                                      *
 * Function:   RtQuatConvertToMatrix                                    *
 * Purpose:    Convert a quaternion matrix into a homogeneous Cartesian *
 * On entry:   Quaternion matrix, homogeneous Cartesian                 *
 *                                                                      *
 ************************************************************************/

/**
 * \ingroup rtquat
 * \ref RtQuatConvertToMatrix converts
 * from an RtQuat to an RwMatrix
 *
 * \param qpQuat  Source quaternion
 * \param mpMatrix  Target matrix
 * \see RtQuatConvertFromMatrix
 */

void
RtQuatConvertToMatrix(const RtQuat * const qpQuat,
                      RwMatrix * const mpMatrix)
{
    RWAPIFUNCTION(RWSTRING("RtQuatConvertToMatrix"));

    RWASSERT(qpQuat);
    RWASSERT(mpMatrix);

    RtQuatConvertToMatrixMacro(qpQuat, mpMatrix);

    RWRETURNVOID();
}

/************************************************************************
 *                                                                      *
 * Function:   RtQuatUnitConvertToMatrix                                *
 * Purpose:    Convert a unitary quaternion matrix                      *
 *             into a homogeneous Cartesian                             *
 * On entry:   Quaternion matrix, homogeneous Cartesian                 *
 *                                                                      *
 ************************************************************************/

/**
 * \ingroup rtquat
 * \ref RtQuatUnitConvertToMatrix converts
 * from a unitary RtQuat to an RwMatrix
 *
 * \param qpQuat  Source quaternion
 * \param mpMatrix  Target matrix
 * \see RtQuatConvertFromMatrix
 */

void
RtQuatUnitConvertToMatrix(const RtQuat * const qpQuat,
                          RwMatrix * const mpMatrix)
{
    RWAPIFUNCTION(RWSTRING("RtQuatUnitConvertToMatrix"));

    RWASSERT(qpQuat);
    RWASSERT(mpMatrix);

    RtQuatUnitConvertToMatrixMacro(qpQuat, mpMatrix);

    RWASSERTPOSITIVEORTHONORMAL(mpMatrix);

    RWRETURNVOID();
}

#endif /* ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) ) */
