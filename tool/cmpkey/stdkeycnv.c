/**********************************************************************
 *
 * File : stdkeycnv.c
 *
 * Abstract : Conversion from hanim std key data to compressed key data
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
 Defines
 */

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
 * \ingroup rtcmpkey
 * \ref RtCompressedKeyConvertFromStdKey
 *  converts a standard keyframe animation to a compressed keyframe animation
 *
 * \param stdkeyanim Standard keyframe animation to convert
 *
 * \return Compressed keyframe animation
 */
RtAnimAnimation *
RtCompressedKeyConvertFromStdKey(RtAnimAnimation *stdkeyanim)
{
    RtAnimAnimation *newAnim = NULL;
    RtCompressedKeyFrame *newKeyframes = NULL;
    RpHAnimKeyFrame *oldKeyframes = NULL;
    RwV3d transScalar, transOffset;
    RwInt32 key;
    RwBBox bbox;

    RWAPIFUNCTION(RWSTRING("RtCompressedKeyConvertFromStdKey"));
    RWASSERT(stdkeyanim);
    RWASSERT(stdkeyanim->interpInfo->typeID == rpHANIMSTDKEYFRAMETYPEID);
    RWASSERT(stdkeyanim->numFrames > 0);

    newAnim = RtAnimAnimationCreate(rtANIMCOMPRESSEDKEYFRAMETYPEID,
                                    stdkeyanim->numFrames,
                                    stdkeyanim->flags,
                                    stdkeyanim->duration);
    RWASSERT(NULL != newAnim);
    newKeyframes = (RtCompressedKeyFrame *)newAnim->pFrames;
    oldKeyframes = (RpHAnimKeyFrame *)stdkeyanim->pFrames;

    /* Calculate the scalar to normalise the positions */
    RwBBoxInitialize(&bbox, &oldKeyframes[0].t);
    for(key = 1; key < newAnim->numFrames; key++)
    {
        RwBBoxAddPoint(&bbox, &oldKeyframes[key].t);
    }

    RwV3dSub(&transScalar, &bbox.sup, &bbox.inf);
    RwV3dScale(&transScalar, &transScalar, 0.5f);
    RwV3dAdd(&transOffset, &transScalar, &bbox.inf);

    /* Store the scalar and offset */
    ((RtCompressedKeyFrameCustomData *)newAnim->customData)->scalar = transScalar;
    ((RtCompressedKeyFrameCustomData *)newAnim->customData)->offset = transOffset;

    /* Set the compressed animation data */
    for(key = 0; key < newAnim->numFrames; key++)
    {
        RwV3d normalisedTrans;

        newKeyframes[key].qx = RtCompressedKeyFrameCompressFloat(oldKeyframes[key].q.imag.x);
        newKeyframes[key].qy = RtCompressedKeyFrameCompressFloat(oldKeyframes[key].q.imag.y);
        newKeyframes[key].qz = RtCompressedKeyFrameCompressFloat(oldKeyframes[key].q.imag.z);
        newKeyframes[key].qr = RtCompressedKeyFrameCompressFloat(oldKeyframes[key].q.real);

        RwV3dSub(&normalisedTrans, &oldKeyframes[key].t, &transOffset);
        normalisedTrans.x /= transScalar.x;
        normalisedTrans.y /= transScalar.y;
        normalisedTrans.z /= transScalar.z;
        newKeyframes[key].tx = RtCompressedKeyFrameCompressFloat(normalisedTrans.x);
        newKeyframes[key].ty = RtCompressedKeyFrameCompressFloat(normalisedTrans.y);
        newKeyframes[key].tz = RtCompressedKeyFrameCompressFloat(normalisedTrans.z);

        newKeyframes[key].time = oldKeyframes[key].time;
        if (newKeyframes[key].time == 0.0f)
        {
            newKeyframes[key].prevFrame = NULL;
        }
        else
        {
            newKeyframes[key].prevFrame = &newKeyframes[key] - ((RwInt32)(&oldKeyframes[key] - oldKeyframes[key].prevFrame));
        }

    }

    RWRETURN(newAnim);
}

