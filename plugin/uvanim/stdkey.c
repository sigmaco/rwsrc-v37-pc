/**********************************************************************
 *
 * File : stdkey.c
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
#define RPUVANIM_CURRENT_STREAM_VERSION 0

/****************************************************************************
 Local types
 */

/****************************************************************************
 Local Defines
 */

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
 * \ingroup rpuvanim
 * \ref RpUVAnimKeyFrameInit sets up a UV animation keyframe.
 *
 * \param animation The animation the keyframe will be present within
 * \param keyFrame The keyframe to set
 * \param prevFrame The previous keyframe in the sequence
 * \param time The time for the keyframe
 * \param matrix The matrix to convert to a keyframe
 *
 * \return A pointer to the keyframe that was set, or NULL if an error occurs
 *
 */
RpUVAnimKeyFrame *
RpUVAnimKeyFrameInit(const RtAnimAnimation *animation,
                    RpUVAnimKeyFrame *keyFrame,
                    RpUVAnimKeyFrame *prevFrame,
                    RwReal time,
                    const RwMatrix *matrix)
{
    RWAPIFUNCTION(RWSTRING("RpUVAnimKeyFrameInit"));
    RWASSERT(animation);
    RWASSERT(keyFrame);
    RWASSERT(matrix);

    keyFrame->prevFrame = prevFrame;
    keyFrame->time = time;
    if (    (animation->interpInfo->typeID == _rpUVAnimLinearInterpolatorInfo.typeID)
        && (RpUVAnimLinearKeyFrameDataInitFromMatrix(&keyFrame->data.linear, matrix)))
    {
        RWRETURN((RpUVAnimKeyFrame *)NULL);
    }
    else if (    (animation->interpInfo->typeID == _rpUVAnimParamInterpolatorInfo.typeID)
              && (RpUVAnimParamKeyFrameDataInitFromMatrix(
                        &keyFrame->data.param,
                        matrix)))
    {
        RWRETURN((RpUVAnimKeyFrame *)NULL);
    }
    else
    {
        RWRETURN((RpUVAnimKeyFrame *)NULL);
    }
    RWRETURN(keyFrame);
}

/**
 *
 * \ingroup rpuvanim
 * \ref RpUVAnimKeyFrameStreamRead reads a UV animation from a stream.
 *
 * \param stream A pointer to the stream to be read from.
 * \param pAnimation A pointer to the animation into which to read the data
 *
 * \return A pointer to the animation, or NULL if an error occurs
 *
 */
RtAnimAnimation *
RpUVAnimKeyFrameStreamRead(RwStream * stream, RtAnimAnimation *pAnimation)
{
    RwUInt32            version;
    RwInt32             i;
    RwInt32             temp;
    RpUVAnimKeyFrame  *pFrames;
    _rpUVAnimCustomData *customData;

    RWAPIFUNCTION(RWSTRING("RpUVAnimKeyFrameStreamRead"));
    RWASSERT(stream);
    RWASSERT(pAnimation);
    RWASSERT(pAnimation->pFrames);

    /* Read the streamed version */
    if (!RwStreamReadInt32(stream, (RwInt32 *) &version, sizeof(RwUInt32)))
    {
        RWRETURN((RtAnimAnimation *)NULL);
    }
    RWASSERT(RPUVANIM_CURRENT_STREAM_VERSION==version);

    /* Read the custom data */
    customData = _rpUVAnimCustomDataStreamRead(stream);
    if (!customData)
    {
        RWRETURN((RtAnimAnimation *)NULL);
    }
    pAnimation->customData = customData;

    /* Read the keyframes */
    pFrames = (RpUVAnimKeyFrame *)pAnimation->pFrames;
    for (i = 0; i < pAnimation->numFrames; ++i)
    {
        if (!RwStreamReadReal
            (stream, (RwReal *) & (pFrames[i].time),
             sizeof(RwReal) * 7))
        {
            RWRETURN((RtAnimAnimation *)NULL);
        }

        if (!RwStreamReadInt
            (stream, (RwInt32 *) & temp, sizeof(RwInt32)))
        {
            RWRETURN((RtAnimAnimation *)NULL);
        }

        pFrames[i].prevFrame = &pFrames[temp];
    }

    RWRETURN(pAnimation);
}

/**
 *
 * \ingroup rpuvanim
 * \ref RpUVAnimKeyFrameStreamWrite writes a UV animation to a stream.
 *
 * \param pAnimation A pointer to the animation to be written.
 * \param stream A pointer to the stream to be written to.
 *
 * \return TRUE on success, or FALSE if an error occurs.
 *
 */
RwBool
RpUVAnimKeyFrameStreamWrite(const RtAnimAnimation * pAnimation,
                              RwStream * stream)
{
    RwUInt32            version = RPUVANIM_CURRENT_STREAM_VERSION;
    RwInt32             i;
    RwInt32             temp;
    RpUVAnimKeyFrame        *pFrames;

    RWAPIFUNCTION(RWSTRING("RpUVAnimKeyFrameStreamWrite"));
    RWASSERT(pAnimation);
    RWASSERT(stream);

    /* Write the streamed version */
    if (!RwStreamWriteInt32(stream, (RwInt32 *) &version, sizeof(RwUInt32)))
    {
        RWRETURN(FALSE);
    }

    /* Write the custom data */
    if (!_rpUVAnimCustomDataStreamWrite(pAnimation->customData, stream))
    {
        RWRETURN(FALSE);
    }

    /* Write the keyframes */
    pFrames = (RpUVAnimKeyFrame *)pAnimation->pFrames;
    for (i = 0; i < pAnimation->numFrames; ++i)
    {
        if (!RwStreamWriteReal
            (stream, (RwReal *) & (pFrames[i].time),
             sizeof(RwReal) * 7))
        {
            RWRETURN(FALSE);
        }

        temp = pFrames[i].prevFrame - pFrames;

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
 * \ingroup rpuvanim
 * \ref RpUVAnimKeyFrameStreamGetSize calculates the size of the UV
 * animation keyframes when written to a stream.
 *
 * \param animation A pointer to the UV animation
 *
 * \return Size of the UV animation keyframes in bytes.
 *
 */
RwInt32
RpUVAnimKeyFrameStreamGetSize(const RtAnimAnimation *animation)
{
    RwInt32 size;

    RWAPIFUNCTION(RWSTRING("RpUVAnimKeyFrameStreamGetSize"));
    RWASSERT(NULL != animation);

    /* size of the streamversion */
    size = sizeof(RwUInt32);

    /* + size of the custom data */
    size += _rpUVAnimCustomDataStreamGetSize(animation->customData);

    /* + the size of the keyframes */
    size += ((sizeof(RwReal) * 7) + sizeof(RwInt32)) * animation->numFrames;

    RWRETURN(size);
}

