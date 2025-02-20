/**********************************************************************
 *
 * File : rtcmpkey.c
 *
 * Abstract : RtAnim keyframe scheme supporting compressed matrix animations
 *
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

/**
 * \ingroup rtcmpkey
 * \page rtcmpkeyoverview RtCmpKey Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rtanim.h, rtcmpkey.h, rtquat.h
 * \li \b Libraries: rwcore, rtanim, rtcmpkey, rtquat
 * \li \b Plugin \b attachments: \ref RtAnimInitialize
 *
 * \subsection cmpkeyoverview Overview
 *
 * This toolkits provides a set of functions and data structures for keyframe
 * animation using compressed data. It is designed to be used with the \ref
 * rtanim animation toolkit.
 *
 * \see \ref rphanimoverview
 * \see \ref rtanimoverview
 */


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

#include "rtcmpkey.h"


/****************************************************************************
 Local types
 */


/****************************************************************************
 Local Defines
 */

#define rtCOMPRESSEDANIMKEYFRAMESIZE sizeof(RtCompressedKeyFrame)
#define rtCOMPRESSEDINTERPKEYFRAMESIZE sizeof(RtCompressedInterpKeyFrame)

#define RtCompressedKeyFrameTransInterpolate(o, a, s, b)          \
MACRO_START                                                     \
{                                                               \
    (o)->x = (((a)->x) + ((s)) * (((b)->x) - ((a)->x)));        \
    (o)->y = (((a)->y) + ((s)) * (((b)->y) - ((a)->y)));        \
    (o)->z = (((a)->z) + ((s)) * (((b)->z) - ((a)->z)));        \
}                                                               \
MACRO_STOP

#define _EPSILON          ((RwReal)(0.001))
#define _TOL_COS_ZERO     (((RwReal)1) - _EPSILON)
#define RwCosecMinusPiToPiMacro(result, x)      \
MACRO_START                                     \
{                                               \
    RwSinMinusPiToPiMacro(result, x);           \
    result = ((RwReal)1) / (result);            \
}                                               \
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

/* Keyframe compression and decompression */

/**
 * \ingroup rtcmpkey
 * \ref RtCompressedKeyFrameCompressFloat
 *  converts a float into a compressed 16bit integer representation.
 *
 * \param I float to compress
 *
 * \return compressed value
 */
RwUInt16
RtCompressedKeyFrameCompressFloat(RwReal I)
{
    volatile RtCompressedKeyFloatInt tmp;
    RwUInt16 O;

    RWAPIFUNCTION(RWSTRING("RtCompressedKeyFrameCompressFloat"));

    tmp.f = (I);
    if ((tmp.i & 0xfff) >= 0x800)
    {
        /* attempt to round rather than truncate */
        int e;

        e = (tmp.i & 0x7f800000)>>23;
        if ((e > 12 ) && ((e < 254) || ((tmp.i & 0x007ff000) != 0x007ff000)))
        {
            RtCompressedKeyFloatInt tmp1;

            e = e - 12;
            tmp1.i = tmp.i & 0x80000000;
            tmp1.i = tmp1.i | (e << 23);
            tmp.f = tmp.f + tmp1.f;
        }
    }
    /* Not strictly required */
    tmp.i = tmp.i & ~0xfff;

    if ((tmp.i & 0x7f800000) > 0x3f800000)
    {
        RwDebugSendMessage(rwDEBUGMESSAGE,
                        "RtCompressedKeyFrameCompressFloat",
                        "Input value out of range.");
        /* Max compressed float */
        tmp.i = (tmp.i & 0x80000000) | 0x7ffff000;
    }
    if ((tmp.i & 0x7f800000) < 0x38000000)
    {
        /* signed 0 to avoid denormals */
        tmp.i = tmp.i & 0x80000000;
    }

    O = (tmp.i >> 16 ) & 0x8000;
    if (tmp.i & 0x7fffffff)
    {
        O |= (((tmp.i >> 23) & 0xff) - 112) << 11;
        O |= (tmp.i >> 12) & 0x7ff;
    }

    RWRETURN(O);
}

/**
 *
 * \ingroup rtcmpkey
 * \ref RtCompressedKeyFrameApply
 *  converts a compressed interpolated keyframe to a matrix.
 *
 * \param matrix A pointer to the output matrix
 * \param voidIFrame A pointer to the input frame
 *
 * \return None
 */
void
RtCompressedKeyFrameApply(void *matrix, void *voidIFrame)
{
    RWAPIFUNCTION(RWSTRING("RtCompressedKeyFrameApply"));
    RWASSERT(matrix);
    RWASSERT(voidIFrame);

    RpHAnimKeyFrameToMatrixMacro((RwMatrix *)matrix, voidIFrame);

    RWRETURNVOID();
}

/**
 *
 * \ingroup rtcmpkey
 * \ref RtCompressedKeyFrameInterpolate
 *  interpolates between two compressed keyframes
 * and returns the result.
 *
 * \param voidOut A pointer to the output frame
 * \param voidIn1 A pointer to the first input frame
 * \param voidIn2 A pointer to the second input frame
 * \param time The time to which to interpolate
 * \param customData A pointer to a \ref RtCompressedKeyFrameCustomData
 * containing extra data for interpolating the compressed key frames.
 *
 * \return None
 */

void
RtCompressedKeyFrameInterpolate(void *voidOut,
                                void *voidIn1,
                                void *voidIn2,
                                RwReal time,
                                void *customData)
{
    RpHAnimInterpFrame  * pOut = (RpHAnimInterpFrame *)voidOut;
    RtCompressedKeyFrame * pIn1 = ((RtCompressedKeyFrame *)voidIn1);
    RtCompressedKeyFrame * pIn2 = ((RtCompressedKeyFrame *)voidIn2);
    RtCompressedKeyFrameCustomData *custData = (RtCompressedKeyFrameCustomData *)customData;
    RtQuat  qIn1;
    RtQuat  qIn2;
    RwV3d   tIn1;
    RwV3d   tIn2;
    RwReal  fCosTheta;
    RwReal  fAlpha;
    RwReal  fBeta;
    RwBool  bObtuseTheta;
    RwBool  bNearlyZeroTheta;

    RWAPIFUNCTION(RWSTRING("RtCompressedKeyFrameInterpolate"));
    RWASSERT(pOut);
    RWASSERT(pIn1);
    RWASSERT(pIn2);
    RWASSERT(pIn1->time <= time);
    RWASSERT(pIn2->time >= time);

    /* NOTE: we must not modify the header of the interpolated keyframe
     * as this is used internally by the RtAnimInterpolator to cache
     * current keyframe pointers.
     */

    /* First decompress the keyframe data */
    RtCompressedKeyFrameDecompressFloat(pIn1->qx, qIn1.imag.x);
    RtCompressedKeyFrameDecompressFloat(pIn1->qy, qIn1.imag.y);
    RtCompressedKeyFrameDecompressFloat(pIn1->qz, qIn1.imag.z);
    RtCompressedKeyFrameDecompressFloat(pIn1->qr, qIn1.real);
    RtCompressedKeyFrameDecompressFloat(pIn1->tx, tIn1.x);
    RtCompressedKeyFrameDecompressFloat(pIn1->ty, tIn1.y);
    RtCompressedKeyFrameDecompressFloat(pIn1->tz, tIn1.z);
    RtCompressedKeyFrameDecompressFloat(pIn2->qx, qIn2.imag.x);
    RtCompressedKeyFrameDecompressFloat(pIn2->qy, qIn2.imag.y);
    RtCompressedKeyFrameDecompressFloat(pIn2->qz, qIn2.imag.z);
    RtCompressedKeyFrameDecompressFloat(pIn2->qr, qIn2.real);
    RtCompressedKeyFrameDecompressFloat(pIn2->tx, tIn2.x);
    RtCompressedKeyFrameDecompressFloat(pIn2->ty, tIn2.y);
    RtCompressedKeyFrameDecompressFloat(pIn2->tz, tIn2.z);


    /* Compute dot product
     * (equal to cosine of the angle between quaternions)
     */
    fCosTheta = (RwV3dDotProduct(&qIn1.imag, &qIn2.imag) +
                                 qIn1.real * qIn2.real);
    fAlpha = ((time - pIn1->time) / (pIn2->time - pIn1->time));


    /* Linearly interpolate positions */
    RtCompressedKeyFrameTransInterpolate(&pOut->t, &tIn1, fAlpha, &tIn2);
    /* And scale by our scalar and apply the offset */
    pOut->t.x = pOut->t.x * custData->scalar.x;
    pOut->t.y = pOut->t.y * custData->scalar.y;
    pOut->t.z = pOut->t.z * custData->scalar.z;
    RwV3dAdd(&pOut->t, &pOut->t, &(custData->offset));

    /* Check angle to see if quaternions are in opposite hemispheres */
    bObtuseTheta = (fCosTheta < ((RwReal) 0));

    if (bObtuseTheta)
    {
        /* If so, flip one of the quaterions */
        fCosTheta = -fCosTheta;
        RwV3dNegate(&qIn2.imag, &qIn2.imag);
        qIn2.real = -qIn2.real;
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
    pOut->q.imag.x = fBeta * qIn1.imag.x + fAlpha * qIn2.imag.x;
    pOut->q.imag.y = fBeta * qIn1.imag.y + fAlpha * qIn2.imag.y;
    pOut->q.imag.z = fBeta * qIn1.imag.z + fAlpha * qIn2.imag.z;
    pOut->q.real = fBeta * qIn1.real + fAlpha * qIn2.real;

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
 * \ingroup rtcmpkey
 * \ref RtCompressedKeyFrameBlend
 * blends between two \ref RpHAnimInterpFrame objects using a given
 * blend factor.
 *
 * \param voidOut A pointer to the output frame.
 * \param voidIn1 A pointer to the first input frame.
 * \param voidIn2 A pointer to the second input frame.
 * \param alpha The blending factor.
 *
 * \return None
 */
void
RtCompressedKeyFrameBlend(void *voidOut,
                          void *voidIn1,
                          void *voidIn2,
                          RwReal alpha)
{
    RpHAnimInterpFrame * pOut = (RpHAnimInterpFrame *)voidOut;
    RpHAnimInterpFrame * pIn1 = (RpHAnimInterpFrame *)voidIn1;
    RpHAnimInterpFrame * pIn2 = (RpHAnimInterpFrame *)voidIn2;

    RwReal              fBeta, fTheta;
    /* Compute dot product
     * (equal to cosine of the angle between quaternions)
     */
    RwReal              fCosTheta =
        RwV3dDotProduct(&pIn1->q.imag, &pIn2->q.imag) +
        pIn1->q.real * pIn2->q.real;
    RwBool              bObtuseTheta;
    RwBool              bNearlyZeroTheta;

    RWAPIFUNCTION(RWSTRING("RtCompressedKeyFrameBlend"));
    RWASSERT(pOut);
    RWASSERT(pIn1);
    RWASSERT(pIn2);

    /* Linearly interpolate positions */
    RtCompressedKeyFrameTransInterpolate(&pOut->t, &pIn1->t, alpha, &pIn2->t);

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
    fBeta = 1.0f - alpha;

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

        alpha *=  fTheta;
        RwSinMinusPiToPiMacro(alpha, alpha);
        alpha *=  fCosecTheta;
    }

    /* Do the interpolation */
    pOut->q.imag.x = fBeta * pIn1->q.imag.x + alpha * pIn2->q.imag.x;
    pOut->q.imag.y = fBeta * pIn1->q.imag.y + alpha * pIn2->q.imag.y;
    pOut->q.imag.z = fBeta * pIn1->q.imag.z + alpha * pIn2->q.imag.z;
    pOut->q.real = fBeta * pIn1->q.real + alpha * pIn2->q.real;

    RWRETURNVOID();
}

/**
 *
 * \ingroup rtcmpkey
 * \ref RtCompressedKeyFrameStreamRead reads a hierarchical compressed animation
 *  from a stream.
 *
 * \param stream A pointer to the stream to be read from.
 * \param animation A pointer to the animation into which to read the data
 *
 * \return A pointer to the animation, or NULL if an error occurs
 *
 */
RtAnimAnimation *
RtCompressedKeyFrameStreamRead(RwStream *stream,
                               RtAnimAnimation *animation)
{
    RwInt32                 i;
    RwInt32                 temp;
    RtCompressedKeyFrame    *pFrames;
    RtCompressedKeyFrameCustomData *customData = NULL;

    RWAPIFUNCTION(RWSTRING("RtCompressedKeyFrameStreamRead"));
    RWASSERT(stream);
    RWASSERT(animation->pFrames);

    pFrames = (RtCompressedKeyFrame *)animation->pFrames;
    customData = (RtCompressedKeyFrameCustomData *)animation->customData;

    for (i = 0; i < animation->numFrames; i++)
    {
        if (!RwStreamReadReal(
                stream, (RwReal *) & (pFrames[i].time), sizeof(RwReal)))
        {
            RWRETURN((RtAnimAnimation *)NULL);
        }

        if (!RwStreamReadInt16(
                stream, (RwInt16 *)(&(pFrames[i].qx)), sizeof(RwUInt16) * 7))
        {
            RWRETURN((RtAnimAnimation *)NULL);
        }

        if (!RwStreamReadInt(stream, (RwInt32 *) & temp, sizeof(RwInt32)))
        {
            RWRETURN((RtAnimAnimation *)NULL);
        }

        pFrames[i].prevFrame = &pFrames[temp / sizeof(RtCompressedKeyFrame)];
    }

    if (!RwStreamReadReal(
            stream, (RwReal *)customData, sizeof(RtCompressedKeyFrameCustomData)))
    {
        RWRETURN((RtAnimAnimation *)NULL);
    }

    RWRETURN(animation);
}

/**
 *
 * \ingroup rtcmpkey
 * \ref RtCompressedKeyFrameStreamWrite writes a hierarchical compressed
 *  animation to a stream.
 *
 * \param animation A pointer to the animation to be written.
 * \param stream A pointer to the stream to be written to.
 *
 * \return TRUE on success, or FALSE if an error occurs.
 *
 */
RwBool
RtCompressedKeyFrameStreamWrite(const RtAnimAnimation *animation,
                                RwStream *stream)
{
    RwInt32                 i;
    RwInt32                 temp;
    RtCompressedKeyFrame    *pFrames;
    RtCompressedKeyFrameCustomData *customData = NULL;

    RWAPIFUNCTION(RWSTRING("RtCompressedKeyFrameStreamWrite"));
    RWASSERT(animation);
    RWASSERT(stream);

    pFrames = (RtCompressedKeyFrame *)animation->pFrames;
    customData = (RtCompressedKeyFrameCustomData *)animation->customData;

    for (i = 0; i < animation->numFrames; i++)
    {
        if (!RwStreamWriteReal(
                stream, (RwReal *)(&(pFrames[i].time)), sizeof(RwReal)))
        {
            RWRETURN(FALSE);
        }

        if (!RwStreamWriteInt16(
                stream, (RwInt16 *)(&(pFrames[i].qx)), sizeof(RwUInt16) * 7))
        {
            RWRETURN(FALSE);
        }

        temp = (RwInt32)(pFrames[i].prevFrame) - (RwInt32)(pFrames);

        if (!RwStreamWriteInt(stream, (RwInt32 *) & temp, sizeof(RwInt32)))
        {
            RWRETURN(FALSE);
        }
    }

    if (!RwStreamWriteReal(
            stream, (RwReal *)customData, sizeof(RtCompressedKeyFrameCustomData)))
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}

/**
 *
 * \ingroup rtcmpkey
 * \ref RtCompressedKeyFrameStreamGetSize calculates the size of a hierarchical
 * compressed animations data (keyframes & custom data) when written to a stream.
 *
 * \param animation A pointer to the hierarchical animation.
 *
 * \return Size of the hierarchical compressed animation data in bytes.
 *
 */
RwInt32
RtCompressedKeyFrameStreamGetSize(const RtAnimAnimation *animation)
{
    RwInt32 size;

    RWAPIFUNCTION(RWSTRING("RtCompressedKeyFrameStreamGetSize"));
    RWASSERT(NULL != animation);

    size = sizeof(RwReal) + sizeof(RwInt32) + (sizeof(RwUInt16) * 7);
    size *= animation->numFrames; /* keyframe data */
    size += sizeof(RtCompressedKeyFrameCustomData); /* custom data */

    RWRETURN(size);
}

/**
 *
 * \ingroup rtcmpkey
 * \ref RtCompressedKeyFrameMulRecip multiplies one keyframe by the reciprocal of
 * another (in place).
 *
 * \param voidFrame A void pointer to the keyframe to modify.
 * \param voidStart A void pointer to the start keyframe to multiply by.
 *
 * \return None
 *
 */
void
RtCompressedKeyFrameMulRecip(void *voidFrame, void *voidStart)
{
    RtCompressedKeyFrame *pAnimFrame = (RtCompressedKeyFrame *)voidFrame;
    RtCompressedKeyFrame *pStartFrame = (RtCompressedKeyFrame *)voidStart;
    RtQuat              qInverse, qFrame, qStart, qResult;
    RwV3d               tStart, tFrame;
    RWAPIFUNCTION(RWSTRING("RtCompressedKeyFrameMulRecip"));
    RWASSERT(voidFrame);
    RWASSERT(voidStart);

    /* Uncompress the start frame quat */
    RtCompressedKeyFrameDecompressFloat(pStartFrame->qx, qStart.imag.x);
    RtCompressedKeyFrameDecompressFloat(pStartFrame->qy, qStart.imag.y);
    RtCompressedKeyFrameDecompressFloat(pStartFrame->qz, qStart.imag.z);
    RtCompressedKeyFrameDecompressFloat(pStartFrame->qr, qStart.real);

    /* Get inverse of hierarchy frame */
    RtQuatReciprocal(&qInverse, &qStart);

    /* Uncompress the frame quat */
    RtCompressedKeyFrameDecompressFloat(pAnimFrame->qx, qFrame.imag.x);
    RtCompressedKeyFrameDecompressFloat(pAnimFrame->qy, qFrame.imag.y);
    RtCompressedKeyFrameDecompressFloat(pAnimFrame->qz, qFrame.imag.z);
    RtCompressedKeyFrameDecompressFloat(pAnimFrame->qr, qFrame.real);
    /* Transform animation frame by inverse of hierarchy frame */
    RtQuatMultiply(&qResult, &qInverse, &qFrame);

    /* Store compressed result frame quat */
    pAnimFrame->qx = RtCompressedKeyFrameCompressFloat(qResult.imag.x);
    pAnimFrame->qy = RtCompressedKeyFrameCompressFloat(qResult.imag.y);
    pAnimFrame->qz = RtCompressedKeyFrameCompressFloat(qResult.imag.z);
    pAnimFrame->qr = RtCompressedKeyFrameCompressFloat(qResult.real);

    /* Decompress start and frame translations */
    RtCompressedKeyFrameDecompressFloat(pStartFrame->tx, tStart.x);
    RtCompressedKeyFrameDecompressFloat(pStartFrame->ty, tStart.y);
    RtCompressedKeyFrameDecompressFloat(pStartFrame->tz, tStart.z);
    RtCompressedKeyFrameDecompressFloat(pAnimFrame->tx, tFrame.x);
    RtCompressedKeyFrameDecompressFloat(pAnimFrame->ty, tFrame.y);
    RtCompressedKeyFrameDecompressFloat(pAnimFrame->tz, tFrame.z);

    /* Calculate offset */
    RwV3dSub(&tFrame, &tFrame, &tStart);

    /* Store compressed result */
    pAnimFrame->tx = RtCompressedKeyFrameCompressFloat(tFrame.x);
    pAnimFrame->ty = RtCompressedKeyFrameCompressFloat(tFrame.y);
    pAnimFrame->tz = RtCompressedKeyFrameCompressFloat(tFrame.z);


    RWRETURNVOID();
}

/**
 *
 * \ingroup rtcmpkey
 * \ref RtCompressedKeyFrameAdd adds two interpolated
 * keyframes together.
 *
 * \param voidOut A void pointer to the output frame.
 * \param voidIn1 A void pointer to the first frame to add.
 * \param voidIn2 A void pointer to the second frame to add.
 *
 * \return None
 *
 */
void
RtCompressedKeyFrameAdd(void *voidOut,
                        void *voidIn1,
                        void *voidIn2)
{
    RpHAnimInterpFrame * pOut = (RpHAnimInterpFrame *)voidOut;
    RpHAnimInterpFrame * pIn1 = (RpHAnimInterpFrame *)voidIn1;
    RpHAnimInterpFrame * pIn2 = (RpHAnimInterpFrame *)voidIn2;

    RWAPIFUNCTION(RWSTRING("RtCompressedKeyFrameAdd"));
    RWASSERT(voidOut);
    RWASSERT(voidIn1);
    RWASSERT(voidIn2);

    RtQuatMultiply(&pOut->q, &pIn1->q, &pIn2->q);
    RwV3dAdd(&pOut->t, &pIn1->t, &pIn2->t);

    RWRETURNVOID();
}


/**
 *
 * \ingroup rtcmpkey
 * \ref RtCompressedKeyFrameRegister registers the compressed keyframe
 *  animation scheme with \ref rtanim.
 *
 * \return TRUE on success, or FALSE if an error occurs.
 */
RwBool
RtCompressedKeyFrameRegister(void)
{
    RtAnimInterpolatorInfo     info;

    RWAPIFUNCTION(RWSTRING("RtCompressedKeyFrameRegister"));
    /*
     *  Register the custom keyframe interpolation scheme.
     */
    info.typeID = rtANIMCOMPRESSEDKEYFRAMETYPEID;
    info.animKeyFrameSize = rtCOMPRESSEDANIMKEYFRAMESIZE;
    info.interpKeyFrameSize = rtCOMPRESSEDINTERPKEYFRAMESIZE;
    info.keyFrameApplyCB           = RtCompressedKeyFrameApply;
    info.keyFrameBlendCB           = RtCompressedKeyFrameBlend;
    info.keyFrameInterpolateCB     = RtCompressedKeyFrameInterpolate;
    info.keyFrameAddCB             = RtCompressedKeyFrameAdd;
    info.keyFrameMulRecipCB        = RtCompressedKeyFrameMulRecip;
    info.keyFrameStreamReadCB      = RtCompressedKeyFrameStreamRead;
    info.keyFrameStreamWriteCB     = RtCompressedKeyFrameStreamWrite;
    info.keyFrameStreamGetSizeCB   = RtCompressedKeyFrameStreamGetSize;
    info.customDataSize            = sizeof(RwV3d) * 2;

    if (!RtAnimRegisterInterpolationScheme(&info))
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}
