/**********************************************************************
 *
 * File : rphanim.c
 *
 * Abstract : A plugin that performs hiearchical animation
 *
 * Notes    : For details on interpolating rotation with
 *            Quaternions, see p360
 *            Advanced Animation and Rendering Techniques
 *            Alan Watt and Mark Watt
 *            Addison-Wesley 1993,
 *            ISBN 0-201-54412-1
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

#include <rtquat.h>
#include <rtanim.h>
#include <rphanim.h>

#include "stdkey.h"


/****************************************************************************
 Defines
 */

/****************************************************************************
 Local types
 */

/****************************************************************************
 Local Defines
 */

#define _EPSILON          ((RwReal)(0.001))
#define _TOL_COS_ZERO     (((RwReal)1) - _EPSILON)
#define RwCosecMinusPiToPiMacro(result, x)      \
MACRO_START                                     \
{                                               \
    RwSinMinusPiToPiMacro(result, x);           \
    result = ((RwReal)1) / (result);            \
}                                               \
MACRO_STOP
#define ROUNDUP16(x)      (((RwUInt32)(x) + 16 - 1) & ~(16 - 1))

#define HAnimKeyFrameAddTogether(_pOut, _pIn1, _pIn2)           \
MACRO_START                                                     \
{                                                               \
    RtQuatMultiply(&(_pOut)->q, &(_pIn1)->q, &(_pIn2)->q);      \
    RwV3dAdd(&(_pOut)->t, &(_pIn1)->t, &(_pIn2)->t);            \
}                                                               \
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
 * \ingroup rphanim
 * \ref RpHAnimKeyFrameApply
 *  converts a standard keyframe to a matrix
 *
 * \param pMatrix A pointer to the output matrix
 * \param pVoidIFrame A pointer to the input frame
 *
 * \return None
 */
void
RpHAnimKeyFrameApply(void *pMatrix, void *pVoidIFrame)
{
    RWAPIFUNCTION(RWSTRING("RpHAnimKeyFrameApply"));
    RWASSERT(pMatrix);
    RWASSERT(pVoidIFrame);

    RpHAnimKeyFrameToMatrixMacro((RwMatrix *)pMatrix, pVoidIFrame);

    RWRETURNVOID();
}

/**
 *
 * \ingroup rphanim
 * \ref RpHAnimKeyFrameInterpolate
 *  interpolates between two skin frames
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
RpHAnimKeyFrameInterpolate(void * pVoidOut, void * pVoidIn1,
                           void * pVoidIn2, RwReal time,
                           void *customData __RWUNUSED__)
{
    RpHAnimInterpFrame * pOut = (RpHAnimInterpFrame *)pVoidOut;
    RpHAnimKeyFrame * pIn1 = (RpHAnimKeyFrame *)pVoidIn1;
    RpHAnimKeyFrame * pIn2 = (RpHAnimKeyFrame *)pVoidIn2;

    /* Compute dot product
     * (equal to cosine of the angle between quaternions)
     */
    RwReal              fCosTheta = (RwV3dDotProduct(&pIn1->q.imag,
                                                     &pIn2->q.imag) +
                                     pIn1->q.real * pIn2->q.real);
    RwReal              fAlpha = ((time - pIn1->time) /

                                  (pIn2->time - pIn1->time));
    RwReal              fBeta;
    RwBool              bObtuseTheta;
    RwBool              bNearlyZeroTheta;

    RWAPIFUNCTION(RWSTRING("RpHAnimKeyFrameInterpolate"));
    RWASSERT(pOut);
    RWASSERT(pIn1);
    RWASSERT(pIn2);
    
	/*
	 * 2006. 2. 7. Nonstopdj
	RWASSERT(pIn1->time <= time);
    RWASSERT(pIn2->time >= time);
	*/

    /* NOTE: we must not modify the header of the interpolated keyframe
     * as this is used internally by the RtAnimInterpolator to cache
     * current keyframe pointers.
     */

    /* Linearly interpolate positions */
    RpHAnimKeyFrameTransInterpolate(&pOut->t, &pIn1->t, fAlpha, &pIn2->t);

    /* Check angle to see if quaternions are in opposite hemispheres */
    bObtuseTheta = (fCosTheta < ((RwReal) 0));

    if (bObtuseTheta)
    {
        /* If so, flip one of the quaterions */
        fCosTheta = -fCosTheta;
        RwV3dNegate(&pIn2->q.imag, &pIn2->q.imag);
        pIn2->q.real = -pIn2->q.real;
    }

    /* Set factors to do linear interpolation, as a special case where the */
    /* quaternions are close together. */
    fBeta = ((RwReal) 1) - fAlpha;

    /* If the quaternions aren't close, proceed with spherical interpolation */
    bNearlyZeroTheta = (fCosTheta >= _TOL_COS_ZERO);

    if (!bNearlyZeroTheta)
    {
        /* Quad trig functions disabled by default */
        /* owing to accuracy issues */

        RwReal        fTheta;
        RwReal        fCosecTheta;

        RwIEEEACosfMacro(fTheta, fCosTheta);
        RwCosecMinusPiToPiMacro(fCosecTheta, fTheta);

        fBeta *=  fTheta;
        RwSinMinusPiToPiMacro(fBeta, fBeta);
        fBeta *=  fCosecTheta;

        fAlpha *=  fTheta;
        RwSinMinusPiToPiMacro(fAlpha, fAlpha);
        fAlpha *=  fCosecTheta;

    }

    /* Do the interpolation */
    pOut->q.imag.x = fBeta * pIn1->q.imag.x + fAlpha * pIn2->q.imag.x;
    pOut->q.imag.y = fBeta * pIn1->q.imag.y + fAlpha * pIn2->q.imag.y;
    pOut->q.imag.z = fBeta * pIn1->q.imag.z + fAlpha * pIn2->q.imag.z;
    pOut->q.real = fBeta * pIn1->q.real + fAlpha * pIn2->q.real;

#if(0)
    /* Assert no worse than 5% error in length^2 of
     * spherically interpolated quaternion  */

    RWASSERT(bNearlyZeroTheta ||
             (((0.95) < RtQuatModulusSquaredMacro(&pOut->q)) &&
              (RtQuatModulusSquaredMacro(&pOut->q) < (1 / 0.95))));
#endif /* (0) */

    RWRETURNVOID();
}

/**
 * \ingroup rphanim
 * \ref RpHAnimKeyFrameBlend
 * blends between two \ref RpHAnimInterpFrame objects using a given
 * blend factor.
 *
 * \param pVoidOut A pointer to the output frame.
 * \param pVoidIn1 A pointer to the first input frame.
 * \param pVoidIn2 A pointer to the second input frame.
 * \param fAlpha The blending factor.
 *
 * \return None
 */
void
RpHAnimKeyFrameBlend(void * pVoidOut,
                     void * pVoidIn1,
                     void * pVoidIn2,
                     RwReal fAlpha)
{
    RpHAnimInterpFrame * pOut = (RpHAnimInterpFrame *)pVoidOut;
    RpHAnimInterpFrame * pIn1 = (RpHAnimInterpFrame *)pVoidIn1;
    RpHAnimInterpFrame * pIn2 = (RpHAnimInterpFrame *)pVoidIn2;

    RwReal              fBeta, fTheta;
    /* Compute dot product
     * (equal to cosine of the angle between quaternions)
     */
    RwReal              fCosTheta =
        RwV3dDotProduct(&pIn1->q.imag, &pIn2->q.imag) +
        pIn1->q.real * pIn2->q.real;
    RwBool              bObtuseTheta;
    RwBool              bNearlyZeroTheta;

    RWAPIFUNCTION(RWSTRING("RpHAnimKeyFrameBlend"));
    RWASSERT(pOut);
    RWASSERT(pIn1);
    RWASSERT(pIn2);

    /* Linearly interpolate positions */
    RpHAnimKeyFrameTransInterpolate(&pOut->t, &pIn1->t, fAlpha, &pIn2->t);

    /* Check angle to see if quaternions are in opposite hemispheres */
    bObtuseTheta = (fCosTheta < ((RwReal) 0));

    if (bObtuseTheta)
    {
        /* If so, flip one of the quaterions */
        fCosTheta = -fCosTheta;
        RwV3dNegate(&pIn2->q.imag, &pIn2->q.imag);
        pIn2->q.real = -pIn2->q.real;
    }

    /* Set factors to do linear interpolation, as a special case where the */
    /* quaternions are close together. */
    fBeta = 1.0f - fAlpha;

    /* If the quaternions aren't close, proceed with spherical interpolation */
    bNearlyZeroTheta = (fCosTheta >= _TOL_COS_ZERO);

    if (!bNearlyZeroTheta)
    {
        RwReal              fCosecTheta;

        RwIEEEACosfMacro(fTheta, fCosTheta);
        RwCosecMinusPiToPiMacro(fCosecTheta, fTheta);

        fBeta *=  fTheta;
        RwSinMinusPiToPiMacro(fBeta, fBeta);
        fBeta *=  fCosecTheta;

        fAlpha *=  fTheta;
        RwSinMinusPiToPiMacro(fAlpha, fAlpha);
        fAlpha *=  fCosecTheta;
    }

    /* Do the interpolation */
    pOut->q.imag.x = fBeta * pIn1->q.imag.x + fAlpha * pIn2->q.imag.x;
    pOut->q.imag.y = fBeta * pIn1->q.imag.y + fAlpha * pIn2->q.imag.y;
    pOut->q.imag.z = fBeta * pIn1->q.imag.z + fAlpha * pIn2->q.imag.z;
    pOut->q.real = fBeta * pIn1->q.real + fAlpha * pIn2->q.real;

    RWRETURNVOID();
}

/**
 *
 * \ingroup rphanim
 * \ref RpHAnimKeyFrameStreamRead reads a hierarchical animation from a stream.
 *
 * \param stream A pointer to the stream to be read from.
 * \param pAnimation A pointer to the animation into which to read the data
 *
 * \return A pointer to the animation, or NULL if an error occurs
 *
 */
RtAnimAnimation *
RpHAnimKeyFrameStreamRead(RwStream * stream, RtAnimAnimation *pAnimation)
{
    RwInt32             i;
    RwInt32             temp;
    RpHAnimKeyFrame  *pFrames;

    RWAPIFUNCTION(RWSTRING("RpHAnimKeyFrameStreamRead"));
    RWASSERT(stream);
    RWASSERT(pAnimation->pFrames);

    pFrames = (RpHAnimKeyFrame *)pAnimation->pFrames;

    for (i = 0; i < pAnimation->numFrames; ++i)
    {
        if (!RwStreamReadReal
            (stream, (RwReal *) & (pFrames[i].time),
             sizeof(RwReal) * 8))
        {
            RWRETURN((RtAnimAnimation *)NULL);
        }

        if (!RwStreamReadInt
            (stream, (RwInt32 *) & temp, sizeof(RwInt32)))
        {
            RWRETURN((RtAnimAnimation *)NULL);
        }

        pFrames[i].prevFrame =
            &pFrames[temp / sizeof(RpHAnimKeyFrame)];
    }

    RWRETURN(pAnimation);
}

/**
 * \ingroup rphanim
 * \ref RpHAnimInterpolatorFindNextKeyFrame find rate of keyframe.
 * \written by 2006. 2. 7. Nonstopdj
 */
RpHAnimKeyFrame     *
RpHAnimInterpolatorFindNextKeyFrame(RpHAnimKeyFrame *curFrame,
									RwInt32 frameSize,
									RwReal duration,
									RwInt32 rate)
{
	RpHAnimKeyFrame		*findNextFrame	= NULL;		/* finding keyframe */
	RpHAnimKeyFrame		*startFrame		= NULL;		/* loop start ketframe*/
	RwReal				interbalTime	= 0.0f;		/* current keyframe time leave a space nextframe's time */

	if(curFrame == NULL)
		RWRETURN((RpHAnimKeyFrame *)NULL);

	if(curFrame->time == duration)
		RWRETURN(curFrame);

	interbalTime	= curFrame->time * rate;
	startFrame		= curFrame;
	findNextFrame	= startFrame;

	while(interbalTime - findNextFrame->time > _EPSILON)	/* defined _EPSILON 0.001 */
	{
		findNextFrame = startFrame;
		/* find next frame */
		while(startFrame != findNextFrame->prevFrame)
		{
			if( findNextFrame->prevFrame->time == 0.0f 
				&& findNextFrame->time == duration)
				RWRETURN(findNextFrame);

			findNextFrame = (RpHAnimKeyFrame *)((RwUInt8 *)findNextFrame + frameSize);

			/* if duration time then always save */
			if(findNextFrame->time == duration 
				&& startFrame == findNextFrame->prevFrame)
				RWRETURN(findNextFrame);
		}

		/* next frame's next frame */
		startFrame = findNextFrame;
	}

	RWRETURN(findNextFrame);
}

/**
 * \ingroup rphanim
 * \ref RpHAnimConvertKeyFrameRate accordance to user defube keyframe rate, sort keyframeArray
 * \written by 2006. 2. 7. Nonstopdj
 */
void
RpHAnimConvertKeyFrameRate(RtAnimAnimation * pAnimation, const RwInt32 rate)
{
	RwInt32             i;
    RpHAnimKeyFrame     *pFrames, *pFindFrames;

	RWAPIFUNCTION(RWSTRING("RpHAnimConvertKeyFrameRate"));
    RWASSERT(pAnimation);

	pFrames = (RpHAnimKeyFrame *)pAnimation->pFrames;

	/* loop all keyframes */
	for (i = 0; i < pAnimation->numFrames; ++i)
	{
		if( pFrames[i].time != 0.0f )
		{
			if(pFrames[i].time == pFrames[i].prevFrame->time)
				continue;

			pFindFrames = RpHAnimInterpolatorFindNextKeyFrame( &pFrames[i], sizeof(RpHAnimKeyFrame), pAnimation->duration, rate);

			if(pFindFrames != NULL && pFrames[i].time != pFindFrames->time)
			{
				/* find sucess */
				pFrames[i].time	= pFindFrames->time;
				pFrames[i].q	= pFindFrames->q;
				pFrames[i].t	= pFindFrames->t;
			}
		}
	}

	/* counting KeyFrames */
	for (i = 0; i < pAnimation->numFrames; ++i)
	{
		if(pFrames[i].time != 0.0f && 
			pFrames[i].prevFrame->time == pAnimation->duration)
			break;
	}

	/* i is new Keyframe count */
	pAnimation->numFrames = i;
}

//RwBool
//RpHAnimKeyFrameStreamWriteEx(const RtAnimAnimation * pAnimation,
//                              RwStream * stream)
//{
//    RwInt32             i, *numFrame, temp;
//    RwInt32             rate = 2;
//    RpHAnimKeyFrame     *pFrames, *pFindFrames;
//
//    RWAPIFUNCTION(RWSTRING("RpHAnimKeyFrameStreamWriteEx"));
//    RWASSERT(pAnimation);
//    RWASSERT(stream);
//
//	pFrames = (RpHAnimKeyFrame *)pAnimation->pFrames;
//
//	/* slerp keyframe */
//	for (i = 0; i < pAnimation->numFrames; ++i)
//	{
//		if( pFrames[i].time != 0.0f )
//		{
//			if(pFrames[i].time == pFrames[i].prevFrame->time)
//				continue;
//
//			pFindFrames = RpHAnimInterpolatorFindNextKeyFrame( &pFrames[i], sizeof(RpHAnimKeyFrame), pAnimation->duration, rate);
//
//			if(pFindFrames != NULL && pFrames[i].time != pFindFrames->time)
//			{
//				pFrames[i].time	= pFindFrames->time;
//				pFrames[i].q	= pFindFrames->q;
//				pFrames[i].t	= pFindFrames->t;
//			}
//		}
//	}
//
//	/* new count numFrame */
//	for (i = 0; i < pAnimation->numFrames; ++i)
//	{
//		if(pFrames[i].time != 0.0f && 
//			pFrames[i].prevFrame->time == pAnimation->duration)
//			break;
//	}
//
//	numFrame = &pAnimation->numFrames;
//	*numFrame = i;
//
//    for (i = 0; i < pAnimation->numFrames; ++i)
//    {
//        if (!RwStreamWriteReal
//            (stream, (RwReal *) & (pFrames[i].time),
//             sizeof(RwReal) * 8))
//        {
//            RWRETURN(FALSE);
//        }
//
//        temp =
//            (RwInt32) (pFrames[i].prevFrame) -
//            (RwInt32) (pFrames);
//
//        if (!RwStreamWriteInt
//            (stream, (RwInt32 *) & temp, sizeof(RwInt32)))
//        {
//            RWRETURN(FALSE);
//        }
//    }
//
//    RWRETURN(TRUE);
//}


/**
 *
 * \ingroup rphanim
 * \ref RpHAnimKeyFrameStreamWrite writes a hierarchical animation to a stream.
 *
 * \param pAnimation A pointer to the animation to be written.
 * \param stream A pointer to the stream to be written to.
 *
 * \return TRUE on success, or FALSE if an error occurs.
 *
 */
RwBool
RpHAnimKeyFrameStreamWrite(const RtAnimAnimation * pAnimation,
                              RwStream * stream)
{
    RwInt32             i;
    RwInt32             temp;
    RpHAnimKeyFrame        *pFrames;

    RWAPIFUNCTION(RWSTRING("RpHAnimKeyFrameStreamWrite"));
    RWASSERT(pAnimation);
    RWASSERT(stream);

    pFrames = (RpHAnimKeyFrame *)pAnimation->pFrames;

    for (i = 0; i < pAnimation->numFrames; ++i)
    {
        if (!RwStreamWriteReal
            (stream, (RwReal *) & (pFrames[i].time),
             sizeof(RwReal) * 8))
        {
            RWRETURN(FALSE);
        }

        temp =
            (RwInt32) (pFrames[i].prevFrame) -
            (RwInt32) (pFrames);

        if (!RwStreamWriteInt
            (stream, (RwInt32 *) & temp, sizeof(RwInt32)))
        {
            RWRETURN(FALSE);
        }
    }

    RWRETURN(TRUE);
}

/**
 *
 * \ingroup rphanim
 * \ref RpHAnimKeyFrameStreamGetSize calculates the size of a hierarchical
 * animation keyframes when written to a stream.
 *
 * \param animation A pointer to the hierarchical animation.
 *
 * \return Size of the hierarchical animation keyframes in bytes.
 *
 */
RwInt32
RpHAnimKeyFrameStreamGetSize(const RtAnimAnimation *animation)
{
    RwInt32 size;

    RWAPIFUNCTION(RWSTRING("RpHAnimKeyFrameStreamGetSize"));
    RWASSERT(NULL != animation);

    size = (sizeof(RwReal) << 3) + sizeof(RwInt32);
    size *= animation->numFrames;

    RWRETURN(size);
}

/**
 *
 * \ingroup rphanim
 * \ref RpHAnimKeyFrameMulRecip multiplies one keyframe by the reciprocal of
 * another (in place).
 *
 * \param pVoidFrame A void pointer to the keyframe to modify.
 * \param pVoidStart A void pointer to the start keyframe to multiply by.
 *
 * \return None
 *
 */
void
RpHAnimKeyFrameMulRecip(void *pVoidFrame, void *pVoidStart)
{
    RpHAnimKeyFrame * pAnimFrame = (RpHAnimKeyFrame *)pVoidFrame;
    RpHAnimKeyFrame * pStartFrame = (RpHAnimKeyFrame *)pVoidStart;
    RtQuat              qInverse, qFrame;
    RWAPIFUNCTION(RWSTRING("RpHAnimKeyFrameMulRecip"));
    RWASSERT(pVoidFrame);
    RWASSERT(pVoidStart);

    /* Get inverse of hierarchy frame */
    RtQuatReciprocal(&qInverse, &pStartFrame->q);

    /* Transform animation frame by inverse of hierarchy frame */
    qFrame = pAnimFrame->q;
    RtQuatMultiply(&pAnimFrame->q, &qInverse, &qFrame);
    RwV3dSub(&pAnimFrame->t, &pAnimFrame->t, &pStartFrame->t);

    RWRETURNVOID();

}

/**
 *
 * \ingroup rphanim
 * \ref RpHAnimKeyFrameAdd adds two interpolated
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
RpHAnimKeyFrameAdd(void * pVoidOut, void * pVoidIn1,
                      void * pVoidIn2)
{
    RpHAnimInterpFrame * pOut = (RpHAnimInterpFrame *)pVoidOut;
    RpHAnimInterpFrame * pIn1 = (RpHAnimInterpFrame *)pVoidIn1;
    RpHAnimInterpFrame * pIn2 = (RpHAnimInterpFrame *)pVoidIn2;

    RWAPIFUNCTION(RWSTRING("RpHAnimKeyFrameAdd"));
    RWASSERT(pVoidOut);
    RWASSERT(pVoidIn1);
    RWASSERT(pVoidIn2);

    HAnimKeyFrameAddTogether(pOut, pIn1, pIn2);

    RWRETURNVOID();
}
