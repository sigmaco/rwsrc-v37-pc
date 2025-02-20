/**********************************************************************
 *
 * File : linear.c
 *
 * Support for linearly interpolated keyframes
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

#define UVAnimLinearKeyFrameAddTogether(_pOut, _pIn1, _pIn2)     \
MACRO_START                                                      \
{                                                                \
    RwReal *uvA = (_pIn1)->uv;                                   \
    RwReal *uvB = (_pIn2)->uv;                                   \
    (_pOut)->uv[0] = uvA[0] * uvB[0] + uvA[1]*uvB[2];            \
    (_pOut)->uv[1] = uvA[0] * uvB[1] + uvA[1]*uvB[3];            \
    (_pOut)->uv[2] = uvA[2] * uvB[0] + uvA[3]*uvB[2];            \
    (_pOut)->uv[3] = uvA[2] * uvB[1] + uvA[3]*uvB[3];            \
    (_pOut)->uv[4] = uvA[4]          +        uvB[4];            \
    (_pOut)->uv[5] = uvA[5]          +        uvB[5];            \
}                                                                \
MACRO_STOP

#define UVAnimLinearKeyFrameTransInterpolate(o, a, s, b)               \
MACRO_START                                                              \
{                                                                        \
    (o)->uv[0] = (((a)->uv[0]) + ((s)) * (((b)->uv[0]) - ((a)->uv[0]))); \
    (o)->uv[1] = (((a)->uv[1]) + ((s)) * (((b)->uv[1]) - ((a)->uv[1]))); \
    (o)->uv[2] = (((a)->uv[2]) + ((s)) * (((b)->uv[2]) - ((a)->uv[2]))); \
    (o)->uv[3] = (((a)->uv[3]) + ((s)) * (((b)->uv[3]) - ((a)->uv[3]))); \
    (o)->uv[4] = (((a)->uv[4]) + ((s)) * (((b)->uv[4]) - ((a)->uv[4]))); \
    (o)->uv[5] = (((a)->uv[5]) + ((s)) * (((b)->uv[5]) - ((a)->uv[5]))); \
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
 * \ref RpUVAnimLinearKeyFrameApply
 * converts a linearly interpolated keyframe to a matrix
 *
 * \param pMatrix A pointer to the output matrix
 * \param pVoidIFrame A pointer to the input frame
 *
 * \return None
 */
void
RpUVAnimLinearKeyFrameApply(void *pMatrix, void *pVoidIFrame)
{
    RWAPIFUNCTION(RWSTRING("RpUVAnimLinearKeyFrameApply"));
    RWASSERT(pMatrix);
    RWASSERT(pVoidIFrame);

    RpUVAnimLinearKeyFrameToMatrixMacro((RwMatrix *)pMatrix, pVoidIFrame);

    RWRETURNVOID();
}

/**
 *
 * \ingroup uvanim
 * \ref RpUVAnimLinearKeyFrameInterpolate
 *  interpolates between two linearly interpolated keyframes
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
RpUVAnimLinearKeyFrameInterpolate(void * pVoidOut, void * pVoidIn1,
                           void * pVoidIn2, RwReal time,
                           void *customData __RWUNUSED__)
{
    RpUVAnimInterpFrame * pOut = (RpUVAnimInterpFrame *)pVoidOut;
    RpUVAnimKeyFrame * pIn1 = (RpUVAnimKeyFrame *)pVoidIn1;
    RpUVAnimKeyFrame * pIn2 = (RpUVAnimKeyFrame *)pVoidIn2;

    RpUVAnimLinearKeyFrameData *kOut = &pOut->data.linear;
    RpUVAnimLinearKeyFrameData *kIn1 = &pIn1->data.linear;
    RpUVAnimLinearKeyFrameData *kIn2 = &pIn2->data.linear;

    /* Direct linear interpolation of the bits we care about
     */
    RwReal              fAlpha = ((time - pIn1->time) /
                                  (pIn2->time - pIn1->time));

    RWAPIFUNCTION(RWSTRING("RpUVAnimLinearKeyFrameInterpolate"));
    RWASSERT(pOut);
    RWASSERT(pIn1);
    RWASSERT(pIn2);
    RWASSERT(pIn1->time <= time);
    RWASSERT(pIn2->time >= time);

    /* NOTE: we must not modify the header of the interpolated keyframe
     * as this is used internally by the RtAnimInterpolator to cache
     * current keyframe pointers.
     */

    /* Linearly interpolate */
    UVAnimLinearKeyFrameTransInterpolate(kOut, kIn1, fAlpha, kIn2);

    RWRETURNVOID();
}

/**
 * \ingroup rpuvanim
 * \ref RpUVAnimLinearKeyFrameBlend
 * blends between two linearly interpolated \ref RpUVAnimInterpFrame objects
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
RpUVAnimLinearKeyFrameBlend(void * pVoidOut,
                     void * pVoidIn1,
                     void * pVoidIn2,
                     RwReal fAlpha)
{
    RpUVAnimInterpFrame * pOut = (RpUVAnimInterpFrame *)pVoidOut;
    RpUVAnimInterpFrame * pIn1 = (RpUVAnimInterpFrame *)pVoidIn1;
    RpUVAnimInterpFrame * pIn2 = (RpUVAnimInterpFrame *)pVoidIn2;

    RpUVAnimLinearKeyFrameData *kOut = &pOut->data.linear;
    RpUVAnimLinearKeyFrameData *kIn1 = &pIn1->data.linear;
    RpUVAnimLinearKeyFrameData *kIn2 = &pIn2->data.linear;

    RWAPIFUNCTION(RWSTRING("RpUVAnimLinearKeyFrameBlend"));
    RWASSERT(pOut);
    RWASSERT(pIn1);
    RWASSERT(pIn2);

    /* Linearly interpolate */
    UVAnimLinearKeyFrameTransInterpolate(kOut, kIn1, fAlpha, kIn2);

    RWRETURNVOID();
}

/**
 *
 * \ingroup rpuvanim
 * \ref RpUVAnimLinearKeyFrameMulRecip multiplies one linearly interpolated
 * keyframe by the reciprocal of another (in place).
 *
 * \param pVoidFrame A void pointer to the keyframe to modify.
 * \param pVoidStart A void pointer to the start keyframe to multiply by.
 *
 * \return None
 *
 */
void
RpUVAnimLinearKeyFrameMulRecip(void *pVoidFrame, void *pVoidStart)
{
    RpUVAnimKeyFrame * pAnimFrame = (RpUVAnimKeyFrame *)pVoidFrame;
    RpUVAnimKeyFrame * pStartFrame = (RpUVAnimKeyFrame *)pVoidStart;

    RpUVAnimLinearKeyFrameData *kStart = &pStartFrame->data.linear;
    RpUVAnimLinearKeyFrameData *kNew = &pAnimFrame->data.linear;

    RwReal det;
    RwReal *uvStart = kStart->uv;
    RwReal *uvNew = kNew->uv;

    RWAPIFUNCTION(RWSTRING("RpUVAnimLinearKeyFrameMulRecip"));
    RWASSERT(pVoidFrame);
    RWASSERT(pVoidStart);

    /* Transform animation frame by inverse of start */
    det = uvStart[0] * uvStart[3] - uvStart[1] * uvStart[2];
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
    uvNew[5] -= uvStart[5];

    RWRETURNVOID();
}

/**
 *
 * \ingroup rpuvanim
 * \ref RpUVAnimLinearKeyFrameAdd adds two linearly interpolated
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
RpUVAnimLinearKeyFrameAdd(void * pVoidOut, void * pVoidIn1,
                      void * pVoidIn2)
{
    RpUVAnimInterpFrame * pOut = (RpUVAnimInterpFrame *)pVoidOut;
    RpUVAnimInterpFrame * pIn1 = (RpUVAnimInterpFrame *)pVoidIn1;
    RpUVAnimInterpFrame * pIn2 = (RpUVAnimInterpFrame *)pVoidIn2;

    RpUVAnimLinearKeyFrameData *kOut = &pOut->data.linear;
    RpUVAnimLinearKeyFrameData *kIn1 = &pIn1->data.linear;
    RpUVAnimLinearKeyFrameData *kIn2 = &pIn2->data.linear;

    RWAPIFUNCTION(RWSTRING("RpUVAnimLinearKeyFrameAdd"));
    RWASSERT(pVoidOut);
    RWASSERT(pVoidIn1);
    RWASSERT(pVoidIn2);

    UVAnimLinearKeyFrameAddTogether(kOut, kIn1, kIn2);

    RWRETURNVOID();
}

/**
 *
 * \ingroup rpuvanim
 * \ref RpUVAnimLinearKeyFrameDataInitFromMatrix sets up linear keyframe data
 * from a matrix.
 * Note that this keyframe must live inside an animation that has been setup
 * to use linear keyframes
 *
 * \param data The linear keyframe to setup
 * \param matrix The matrix to set the keyframe from
 *
 * \return The keyframe on success, NULL on failure
 *
 */
RpUVAnimLinearKeyFrameData *
RpUVAnimLinearKeyFrameDataInitFromMatrix(
                              RpUVAnimLinearKeyFrameData *data,
                              const RwMatrix *matrix)
{
    RWAPIFUNCTION(RWSTRING("RpUVAnimLinearKeyFrameDataInitFromMatrix"));
    RWASSERT(data);
    RWASSERT(matrix);

    data->uv[0] = matrix->right.x;
    data->uv[1] = matrix->right.y;
    data->uv[2] = matrix->up.x;
    data->uv[3] = matrix->up.y;
    data->uv[4] = matrix->pos.x;
    data->uv[5] = matrix->pos.y;

    RWRETURN(data);
}


