/**********************************************************************
 *
 * File : param.c
 *
 * Support for parameterized keyframes
 *
 * See Also : rwsdk/plugin/anim
 *
 **********************************************************************
 *
 * This file is a product of Criterion Software Ltd.
 *
 * This file is provided as is with no warranties of any kind and is
 * provided without any obligation on Criterion Software Ltd. or
 * Canon Inc. to assist in its use or modification.
 *
 * Criterion Software Ltd. will not, under any
 * circumstances, be liable for any lost revenue or other damages arising
 * from the use of this file.
 *
 * Copyright (c) 1999 Criterion Software Ltd.
 * All Rights Reserved.
 *
 * RenderWare is a trademark of Canon Inc.
 *
 ************************************************************************/

/****************************************************************************
 Includes
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rpplugin.h"
#include <rpdbgerr.h>
#include <rwcore.h>
#include <rpworld.h>

#include <rtanim.h>
#include <rpuvanim.h>


/****************************************************************************
 Defines
 */

/****************************************************************************
 Local types
 */

/****************************************************************************
 Local Defines
 */
#define UVAnimParamKeyFrameTransInterpolate(o, a, s, b)                      \
MACRO_START                                                                  \
{                                                                            \
    /* Limit destTheta is not to be more that PI radians from sourceTheta */ \
    RwReal deltaTheta = (b)->theta - (a)->theta;                             \
    if (deltaTheta < -rwPI)                                                  \
    {                                                                        \
        deltaTheta += 2.0f*rwPI;                                             \
    }                                                                        \
    else if (deltaTheta > rwPI)                                              \
    {                                                                        \
        deltaTheta -= 2.0f*rwPI;                                             \
    }                                                                        \
    (o)->theta = (((a)->theta) + ((s)) * deltaTheta);                        \
    (o)->s0    = (((a)->s0)    + ((s)) * (((b)->s0)    - ((a)->s0)));    \
    (o)->s1    = (((a)->s1)    + ((s)) * (((b)->s1)    - ((a)->s1)));    \
    (o)->skew  = (((a)->skew)  + ((s)) * (((b)->skew)  - ((a)->skew)));  \
    (o)->x     = (((a)->x)     + ((s)) * (((b)->x)     - ((a)->x)));     \
    (o)->y     = (((a)->y)     + ((s)) * (((b)->y)     - ((a)->y)));     \
}                                                                        \
MACRO_STOP

/****************************************************************************
 Local (static) globals
 */

/****************************************************************************
 Local Function Prototypes
 */

/****************************************************************************
 Functions
 */

/**
 *
 * \ingroup uvanim
 * \ref RpUVAnimParamKeyFrameApply
 * converts a parameterized interpolated keyframe to a matrix
 *
 * \param pMatrix A pointer to the output matrix
 * \param pVoidIFrame A pointer to the input frame
 *
 * \return None
 */
void
RpUVAnimParamKeyFrameApply(void *pMatrix, void *pVoidIFrame)
{
    RWAPIFUNCTION(RWSTRING("RpUVAnimParamKeyFrameApply"));
    RWASSERT(pMatrix);
    RWASSERT(pVoidIFrame);

    RpUVAnimParamKeyFrameToMatrixMacro((RwMatrix *)pMatrix, pVoidIFrame, &rpUVAnimPivot, &rpUVAnimInvPivot);

    RWRETURNVOID();
}

/**
 *
 * \ingroup uvanim
 * \ref RpUVAnimParamKeyFrameInterpolate
 *  interpolates between two parameter interpolated keyframes
 * and returns the result.
 *
 * \param pVoidOut A pointer to the output frame
 * \param pVoidIn1 A pointer to the first input frame
 * \param pVoidIn2 A pointer to the second input frame
 * \param time The time to which to interpolate
 * \param customData A pointer to custom data for the animation, unused
 * in this keyframe scheme
 *
 * \return None
 */
void
RpUVAnimParamKeyFrameInterpolate(void * pVoidOut, void * pVoidIn1,
                           void * pVoidIn2, RwReal time,
                           void *customData __RWUNUSED__)
{
    RpUVAnimInterpFrame * pOut = (RpUVAnimInterpFrame *)pVoidOut;
    RpUVAnimKeyFrame * pIn1 = (RpUVAnimKeyFrame *)pVoidIn1;
    RpUVAnimKeyFrame * pIn2 = (RpUVAnimKeyFrame *)pVoidIn2;

    RpUVAnimParamKeyFrameData *kOut = &pOut->data.param;
    RpUVAnimParamKeyFrameData *kIn1 = &pIn1->data.param;
    RpUVAnimParamKeyFrameData *kIn2 = &pIn2->data.param;

    /* Direct param interpolation of the bits we care about
     */
    RwReal              fAlpha = ((time - pIn1->time) /
                                  (pIn2->time - pIn1->time));

    RWAPIFUNCTION(RWSTRING("RpUVAnimParamKeyFrameInterpolate"));
    RWASSERT(pOut);
    RWASSERT(pIn1);
    RWASSERT(pIn2);
    RWASSERT(pIn1->time <= time);
    RWASSERT(pIn2->time >= time);

    /* NOTE: we must not modify the header of the interpolated keyframe
     * as this is used internally by the RtAnimInterpolator to cache
     * current keyframe pointers.
     */

    /* Parametric interpolate */
    UVAnimParamKeyFrameTransInterpolate(kOut, kIn1, fAlpha, kIn2);

    RWRETURNVOID();
}

/**
 * \ingroup rpuvanim
 * \ref RpUVAnimParamKeyFrameBlend
 * blends between two parametric interpolated \ref RpUVAnimInterpFrame objects
 * using a given blend factor.
 *
 * \param pVoidOut A pointer to the output frame.
 * \param pVoidIn1 A pointer to the first input frame.
 * \param pVoidIn2 A pointer to the second input frame.
 * \param fAlpha The blending factor.
 *
 * \return None
 */
void
RpUVAnimParamKeyFrameBlend(void * pVoidOut,
                     void * pVoidIn1,
                     void * pVoidIn2,
                     RwReal fAlpha)
{
    RpUVAnimInterpFrame * pOut = (RpUVAnimInterpFrame *)pVoidOut;
    RpUVAnimInterpFrame * pIn1 = (RpUVAnimInterpFrame *)pVoidIn1;
    RpUVAnimInterpFrame * pIn2 = (RpUVAnimInterpFrame *)pVoidIn2;

    RpUVAnimParamKeyFrameData *kOut = &pOut->data.param;
    RpUVAnimParamKeyFrameData *kIn1 = &pIn1->data.param;
    RpUVAnimParamKeyFrameData *kIn2 = &pIn2->data.param;

    RWAPIFUNCTION(RWSTRING("RpUVAnimParamKeyFrameBlend"));
    RWASSERT(pOut);
    RWASSERT(pIn1);
    RWASSERT(pIn2);

    /* Parametrically interpolate */
    UVAnimParamKeyFrameTransInterpolate(kOut, kIn1, fAlpha, kIn2);

    RWRETURNVOID();
}

/**
 *
 * \ingroup rpuvanim
 * \ref RpUVAnimParamKeyFrameMulRecip multiplies one parametrically interpolated
 * keyframe by the reciprocal of another (in place).
 *
 * \param pVoidFrame A void pointer to the keyframe to modify.
 * \param pVoidStart A void pointer to the start keyframe to multiply by.
 *
 * \return None
 *
 */
void
RpUVAnimParamKeyFrameMulRecip(void * pVoidFrame __RWUNUSED__, void * pVoidStart __RWUNUSED__)
{
/*    RpUVAnimKeyFrame * pAnimFrame = (RpUVAnimKeyFrame *)pVoidFrame;
    RpUVAnimKeyFrame * pStartFrame = (RpUVAnimKeyFrame *)pVoidStart;

    RpUVAnimParamKeyFrameData *kStart = &pStartFrame->data.param;
    RpUVAnimParamKeyFrameData *kNew = &pAnimFrame->data.param;

    RwReal det;
    RwReal *uvStart = kStart->uv;
    RwReal *uvNew = kNew->uv;*/

    RWAPIFUNCTION(RWSTRING("RpUVAnimParamKeyFrameMulRecip"));
    RWASSERT(pVoidFrame);
    RWASSERT(pVoidStart);

    RWASSERTM(FALSE, (RWSTRING("Not implemented")));

    /* Transform animation frame by inverse of start */
/*    det = uvStart[0] * uvStart[3] - uvStart[1] * uvStart[2];
    if (det)
    {
        RwReal invDet = 1.0f / det;
        RwReal uvInv[4];
        RwReal uvOrig[4];

        uvOrig[0] = uvNew[0];
        uvOrig[1] = uvNew[1];
        uvOrig[2] = uvNew[2];
        uvOrig[3] = uvNew[3];

        uvInv[0] =  uvStart[3] * invDet;
        uvInv[1] = -uvStart[1] * invDet;
        uvInv[2] = -uvStart[2] * invDet;
        uvInv[3] =  uvStart[0] * invDet;

        uvNew[0] = uvInv[0] * uvOrig[0] + uvInv[1] * uvOrig[2];
        uvNew[1] = uvInv[0] * uvOrig[1] + uvInv[1] * uvOrig[3];;
        uvNew[2] = uvInv[2] * uvOrig[0] + uvInv[2] * uvOrig[2];;
        uvNew[3] = uvInv[2] * uvOrig[1] + uvInv[2] * uvOrig[3];;
    }

    uvNew[4] -= uvStart[4];
    uvNew[5] -= uvStart[5];*/

    RWRETURNVOID();
}

/**
 *
 * \ingroup rpuvanim
 * \ref RpUVAnimParamKeyFrameAdd adds two parametrically interpolated
 * keyframes together.
 *
 * \param pVoidOut A void pointer to the output frame.
 * \param pVoidIn1 A void pointer to the first frame to add.
 * \param pVoidIn2 A void pointer to the second frame to add.
 *
 * \return None
 *
 */
void
RpUVAnimParamKeyFrameAdd(void * pVoidOut __RWUNUSEDRELEASE__, void * pVoidIn1 __RWUNUSEDRELEASE__,
                      void * pVoidIn2 __RWUNUSEDRELEASE__)
{
/*    RpUVAnimInterpFrame * pOut = (RpUVAnimInterpFrame *)pVoidOut;
    RpUVAnimInterpFrame * pIn1 = (RpUVAnimInterpFrame *)pVoidIn1;
    RpUVAnimInterpFrame * pIn2 = (RpUVAnimInterpFrame *)pVoidIn2;

    RpUVAnimParamKeyFrameData *kOut = &pOut->data.param;
    RpUVAnimParamKeyFrameData *kIn1 = &pIn1->data.param;
    RpUVAnimParamKeyFrameData *kIn2 = &pIn2->data.param;*/

    RWAPIFUNCTION(RWSTRING("RpUVAnimParamKeyFrameAdd"));
    RWASSERT(pVoidOut);
    RWASSERT(pVoidIn1);
    RWASSERT(pVoidIn2);

    RWASSERTM(FALSE, (RWSTRING("Not implemented")));

    RWRETURNVOID();
}

/**
 *
 * \ingroup rpuvanim
 * \ref RpUVAnimParamKeyFrameDataInitFromMatrix sets up parameterized keyframe data
 * from a matrix.
 * Note that this keyframe must live inside an animation that has been setup
 * to use parameterized keyframes.
 *
 * \param data The parameterized keyframe to setup
 * \param matrix The matrix to set the keyframe from
 *
 * \return The keyframe on success, NULL on failure
 *
 */
RpUVAnimParamKeyFrameData *
RpUVAnimParamKeyFrameDataInitFromMatrix(
                              RpUVAnimParamKeyFrameData *data,
                              const RwMatrix *matrix)
{
    RwMatrix m;

    RWAPIFUNCTION(RWSTRING("RpUVAnimParamKeyFrameDataInitFromMatrix"));
    RWASSERT(data);

    RwMatrixCopy(&m, matrix);
    {
        RwV3d ZAxis = {0.0f, 0.0f, 1.0f};
        RwReal a = m.right.x;
        RwReal c = m.up.x;

        data->theta = (RwReal)RwATan2(-c, a);
        RwMatrixTranslate(&m, &rpUVAnimPivot, rwCOMBINEPOSTCONCAT);
        RwMatrixRotate(&m, &ZAxis, -data->theta/rwPI*180.0f, rwCOMBINEPOSTCONCAT);
        RwMatrixTranslate(&m, &rpUVAnimInvPivot, rwCOMBINEPOSTCONCAT);

        data->s0    = m.right.x;
        data->skew  = m.right.y;
        data->s1    = m.up.y;
        data->x     = m.pos.x;
        data->y     = m.pos.y;
    }

    RWRETURN(data);
}
