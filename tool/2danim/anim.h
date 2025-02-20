
/***************************************************************************
 *                                                                         *
 * Module  : anim.h                                                        *
 *                                                                         *
 * Purpose :                                                               *
 *                                                                         *
 **************************************************************************/

#ifndef ANIM_H
#define ANIM_H

/****************************************************************************
 Includes
 */
#include <rwcore.h>
#include "rt2danim.h"

/****************************************************************************
 Defines
 */

/****************************************************************************
 Global Types
 */

/*
 * typedef for a time-scheduled keyframe
 */
typedef struct _rt2dAnimKeyFrameSched _rt2dAnimKeyFrameSched;

/*
 * structure for a time-scheduled keyframe
 */
struct _rt2dAnimKeyFrameSched
{
    RwReal                time;
    Rt2dKeyFrameList *keyframeList;
};

#define Rt2dAnimationIsLocked           0x01
#define Rt2dAnimationIsInterpolated     0x02

struct Rt2dAnim
{
    RwSList               *keyframeLists;
    RwReal                deltaTimeScale;
    RwReal                currentTime;
    RwUInt32              flag;
    _rt2dAnimKeyFrameSched *prev, *next, *last;
};

/****************************************************************************
 Function prototypes
 */
Rt2dAnim *
_rt2dAnimInit(Rt2dAnim *anim, RwUInt32 frameCount);

extern Rt2dAnim *
_rt2dAnimDestruct(Rt2dAnim *anim, Rt2dAnimProps *props);

extern Rt2dAnim *
_rt2dAnimStreamRead(Rt2dAnim *anim, RwStream* stream,
                   Rt2dAnimProps *props __RWUNUSED__);

extern Rt2dAnim *
_rt2dAnimCopy(Rt2dAnim *destAnim,
              Rt2dAnim *srcAnim, Rt2dAnimProps *srcProps);

#endif /* ANIM_H */
