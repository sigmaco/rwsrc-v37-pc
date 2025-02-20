
/***************************************************************************
 *                                                                         *
 * Module  : keyframe.h                                                    *
 *                                                                         *
 * Purpose :                                                               *
 *                                                                         *
 **************************************************************************/

#ifndef KEYFRAME_H
#define KEYFRAME_H

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
 *  structure describing an unoptimized update to an object
 */
struct Rt2dAnimObjectUpdate
{
    RwBool hasSomething;

    Rt2dObject *object;

    RwBool hasTransform;
    RWALIGN(Rt2dKeyFrameTransform transform, rwMATRIXALIGNMENT);

    RwBool hasColorOffs;
    Rt2dKeyFrameColor colorOffs;

    RwBool hasColorMult;
    Rt2dKeyFrameColor colorMult;

    RwBool hasShow;
    Rt2dKeyFrameShow show;

    RwBool hasMorph;
    Rt2dKeyFrameMorph morph;
};

#define Rt2dFrameHasTransform   0x001
#define Rt2dFrameHasColorOffs   0x002
#define Rt2dFrameHasColorMult   0x004
#define Rt2dFrameHasShow        0x008
#define Rt2dFrameHasMorph       0x010
#define Rt2dFrameIsLast         0x020

struct Rt2dKeyFrameMorphUnlocked
{
    RwInt32 source;
    RwInt32 destination;
    RwReal  alpha;
};

/*
 *  structure describing a set of keyframe actions to be applied to a 2D object
 */
struct Rt2dKeyFrameSet
{
    RwUInt32    flag;           /*4*/
    RwInt32     objectIdx;      /*4*/
    RwUInt8     pad1[8];       /*8*/
};                             /*16*/


typedef union AnimKeyFrameData AnimKeyFrameData;

union AnimKeyFrameData {
    RwSList *locked;
    RwUInt8 *unlocked;
};

/*
 *  structure describing an entire list of keyframes
 */
struct Rt2dKeyFrameList
{
    RwReal              time;
    RwBool              isLocked;
    AnimKeyFrameData    data;
};

typedef struct _rt2dStreamKeyFrameList _rt2dStreamKeyFrameList;

struct _rt2dStreamKeyFrameList
{
    RwUInt32            version;
    RwReal              time;
    RwInt32             keyframeCount;
};

/****************************************************************************
 Function prototypes
 */
#ifdef    __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern RwSList *
_rt2dKeyFrameListCreateLockedRep(Rt2dAnimProps *props);

extern Rt2dKeyFrameList *
_rt2dKeyFrameListStreamReadTo(RwStream *stream, Rt2dKeyFrameList *keyframeList);

#ifdef    __cplusplus
}
#endif /* __cplusplus */


#endif /* KEYFRAME_H */
