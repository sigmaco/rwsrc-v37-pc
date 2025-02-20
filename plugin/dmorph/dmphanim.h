/*
 *  dmphanim.h - delta morph animation system
 */

#if (!defined(_DMPHANIM_H))
#define _DMPHANIM_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/

#include "rwcore.h"
#include "rpworld.h"

/*===========================================================================*
 *--- Defines ---------------------------------------------------------------*
 *===========================================================================*/

#define rpDMORPHANIMATIONSTREAMCURRVERSION (0x100)
/* RWPUBLIC */
#define rpDMORPHNULLFRAME ((RwUInt32)~0)
/* RWPUBLICEND */

/*===========================================================================*
 *--- Types -----------------------------------------------------------------*
 *===========================================================================*/

/* RWPUBLIC */
/*--- rpDMorphFrame ---------------------------------------------------------
 *
 *  Animation frame.
 */
typedef struct rpDMorphFrame rpDMorphFrame;

#if (!defined(DOXYGEN))
struct rpDMorphFrame
{
    /* Ordering must be maintained here for stream compatibility */

    /*--- Interpolation data ---*/
    RwReal   startValue;
    RwReal   endValue;
    RwReal   duration;
    /*--- Utility data ---*/
    RwReal   recipDuration;
    /*--- Linking ---*/
    RwUInt32 nextFrame;
};
#endif /* (!defined(DOXYGEN)) */

/*--- rpDMorphFrameSequence -------------------------------------------------
 *
 *  A linked sequence of animation frames.
 */
typedef struct rpDMorphFrameSequence rpDMorphFrameSequence;

#if (!defined(DOXYGEN))
struct rpDMorphFrameSequence
{
    RwUInt32         numFrames;
    rpDMorphFrame    *frames;

};
#endif /* (!defined(DOXYGEN)) */

/**
 * \ingroup rpdmorph
 * \struct RpDMorphAnimation
 * Contains frame sequences for animating delta
 * morph target objects.
 * This should be considered an opaque type.
 * Use the RpDMorphAnimation API
 * functions to access.
 */
typedef struct RpDMorphAnimation RpDMorphAnimation;

/*--- RpDMorphAnimation -----------------------------------------------------
 *
 *  An array of animation sequences, one for each morph target.
 */
#if (!defined(DOXYGEN))
struct RpDMorphAnimation
{
    RwUInt32                numDMorphTargets;
    rpDMorphFrameSequence   *frameSequences;
};
#endif /* (!defined(DOXYGEN)) */
/* RWPUBLICEND */

/* RWPUBLIC */
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/*--- Animation Functions --------------------------------------------------
 */
extern RpDMorphAnimation *
RpDMorphAnimationCreate(RwUInt32 numDMorphTargets);

extern RpDMorphAnimation *
RpDMorphAnimationDestroy(RpDMorphAnimation *anim);

extern RwUInt32
RpDMorphAnimationGetNumDMorphTargets(RpDMorphAnimation *animation);

/* Animation Frames */
extern RpDMorphAnimation *
RpDMorphAnimationCreateFrames(RpDMorphAnimation  *anim,
                              RwUInt32           dMorphTargetIndex,
                              RwUInt32           numFrames);

extern RpDMorphAnimation *
RpDMorphAnimationDestroyFrames(RpDMorphAnimation  *anim,
                               RwUInt32           dMorphTargetIndex);

extern RwUInt32
RpDMorphAnimationGetNumFrames(RpDMorphAnimation *animation,
                              RwUInt32          dMorphTargetIndex);

/* Stream I/O */
extern RpDMorphAnimation *
RpDMorphAnimationStreamRead(RwStream *stream);

extern RpDMorphAnimation *
RpDMorphAnimationStreamWrite(RpDMorphAnimation *animation,
                             RwStream *stream);

extern RwUInt32
RpDMorphAnimationStreamGetSize(RpDMorphAnimation *animation);

extern RpDMorphAnimation *
RpDMorphAnimationRead(const RwChar *filename);

extern RpDMorphAnimation *
RpDMorphAnimationWrite(RpDMorphAnimation *animation, const RwChar *filename);

/*--- Animation Frame Functions --------------------------------------------
 *
 * These functions work on the DMorphAnimationFrame level.
 * Each Frame can have a reference to the next Frame for the
 * DMorphTarget.
 */
extern RpDMorphAnimation *
RpDMorphAnimationFrameSetNext(RpDMorphAnimation *anim,
                              RwUInt32 dMorphTargetIndex,
                              RwUInt32 frameIndex,
                              RwUInt32 nextFrame );

extern RwUInt32
RpDMorphAnimationFrameGetNext(RpDMorphAnimation *anim,
                              RwUInt32 dMorphTargetIndex,
                              RwUInt32 frameIndex );

extern RpDMorphAnimation *
RpDMorphAnimationFrameSet(RpDMorphAnimation *anim,
                          RwUInt32   dMorphTargetIndex,
                          RwUInt32   frameIndex,
                          RwReal startValue,
                          RwReal endValue,
                          RwReal duration,
                          RwUInt32 nextFrame );

extern RpDMorphAnimation *
RpDMorphAnimationFrameSetStartValue(RpDMorphAnimation *anim,
                                    RwUInt32 dMorphTargetIndex,
                                    RwUInt32 frameIndex,
                                    RwReal   startValue );

extern RwReal
RpDMorphAnimationFrameGetStartValue(RpDMorphAnimation *anim,
                                    RwUInt32 dMorphTargetIndex,
                                    RwUInt32 frameIndex );

extern RpDMorphAnimation *
RpDMorphAnimationFrameSetEndValue(RpDMorphAnimation *anim,
                                  RwUInt32  dMorphTargetIndex,
                                  RwUInt32  frameIndex,
                                  RwReal    endValue );

extern RwReal
RpDMorphAnimationFrameGetEndValue(RpDMorphAnimation *anim,
                                  RwUInt32  dMorphTargetIndex,
                                  RwUInt32  frameIndex );

extern RpDMorphAnimation *
RpDMorphAnimationFrameSetDuration(RpDMorphAnimation *anim,
                                  RwUInt32  dMorphTargetIndex,
                                  RwUInt32  frameIndex,
                                  RwReal    duration );

extern RwReal
RpDMorphAnimationFrameGetDuration(RpDMorphAnimation *anim,
                                  RwUInt32 dMorphTargetIndex,
                                  RwUInt32 frameIndex );

#ifdef __cplusplus
}
#endif /* __cplusplus */
/* RWPUBLICEND */

#endif /* _DMPHANIM_H */
