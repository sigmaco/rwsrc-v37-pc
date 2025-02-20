/*
 *
 * Functionality for 2D rendering
 *
 * Copyright (c) 2001 Criterion Software Ltd.
 */

/****************************************************************************
 *                                                                          *
 *  Module  :   anim.c                                                      *
 *                                                                          *
 *  Purpose :   simple 2d animation functionality                           *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
 */

/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
*/

#include "rwcore.h"
#include <rpdbgerr.h>
#include <rpcriter.h>
#include "rt2danim.h"
#include "anim.h"
#include "gstate.h"

#define FILE_CURRENT_VERSION 0x01
#define FILE_LAST_SUPPORTED_VERSION 0x01


/*-----------------20/08/2001 10:20 AJH -------------
 * Is that really needed ???
 * --------------------------------------------------*/
#if defined (__MWERKS__)
#if (defined(RWVERBOSE))
#pragma message (__FILE__ "/" _SKY_EXPAND(__LINE__) ": __MWERKS__ == " _SKY_EXPAND(__MWERKS__))
#endif /* (defined (__MWERKS__)) */
#if (__option (global_optimizer))
#pragma always_inline off
#endif /* (__option (global_optimizer)) */
#endif /*  defined (__MWERKS__) */

/**
 * \ingroup rt2danimsub
 * \page rt2danimsuboverview Rt2dAnim Overview
 *
 * \ref Rt2dAnim is a low level structure that can be used to coordinate an
 * animation on a single level of 2d scene. Animations are constructed out
 * of a sequence of \ref Rt2dKeyFrameList objects on the scene. Individual
 * updates to the scene are scheduled through setting changes in
 * \ref Rt2dAnimObjectUpdate objects. Most animation operations are carried
 * out in conjunction with an \ref Rt2dAnimProps structure that enables the
 * keframe lists to be constructed against an existing scene.
 *
 * \see Rt2dAnimCreate
 * \see Rt2dKeyFrameListCreate
 * \see Rt2dSceneCreate
 */

/****************************************************************************
 Local Types
 */

/****************************************************************************
 Local (Static) Prototypes
 */
/****************************************************************************
 Local Defines
 */

/****************************************************************************
 Globals (across program)
 */


/****************************************************************************
 Local (static) Globals
 */

/****************************************************************************
 * extern prototype :
 * AJH said : yes it's bad, and mean. and so ?? got somethin' t' say
 */
extern Rt2dAnimOnEndReachedCallBack _rt2dAnimOnEndReachedCallBack;
/****************************************************************************
 * Functions
 *
 ****************************************************************************/

Rt2dAnim *
_rt2dAnimInit(Rt2dAnim *anim, RwUInt32 frameCount)
{
    RWFUNCTION(RWSTRING("_rt2dAnimInit"));

    /* Initialise members */
    anim->keyframeLists = rwSListCreate(sizeof(_rt2dAnimKeyFrameSched),
                                        rwID_2DKEYFRAME | rwMEMHINTDUR_EVENT);
    RWASSERT(anim->keyframeLists);

    if (frameCount)
    {
        rwSListGetNewEntries(anim->keyframeLists, frameCount,
            rwID_2DKEYFRAME | rwMEMHINTDUR_EVENT);
    }

    anim->deltaTimeScale = 1.0f;
    anim->currentTime = 0.0f;
    anim->flag = Rt2dAnimationIsLocked;
    anim->prev = NULL;
    anim->next = NULL;
    anim->last = NULL;

    RWRETURN(anim);
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimCreate is used to create a new animation that may on act on a
 * single level of a 2d scene.
 *
 * \return Pointer to the new animation
 *
 * \see Rt2dSceneCreate
 */
Rt2dAnim *
Rt2dAnimCreate(void)
{
    Rt2dAnim *anim;
    RWAPIFUNCTION(RWSTRING("Rt2dAnimCreate"));

    anim = (Rt2dAnim*)RwFreeListAlloc(&Rt2dAnimGlobals.animFreeList,
                                      rwID_2DANIM | rwMEMHINTDUR_EVENT);
    RWASSERT(anim);

    /* Initialise members */
    _rt2dAnimInit(anim, 0);

    RWRETURN(anim);
}

static Rt2dKeyFrameList *
_rt2dAnimAdaptKeyFrameDestroy(  Rt2dAnim *anim __RWUNUSED__,
                                Rt2dAnimProps *props __RWUNUSED__,
                                Rt2dKeyFrameList *keyframeList,
                                RwReal keyframeTime __RWUNUSED__,
                                void *data)
{
    RwBool *result = (RwBool *)data;
    RWFUNCTION(RWSTRING("_rt2dAnimAdaptKeyFrameDestroy"));

    *result = Rt2dKeyFrameListDestroy(keyframeList) && *result;

    RWRETURN(keyframeList);
}

Rt2dAnim *
_rt2dAnimDestruct(Rt2dAnim *anim, Rt2dAnimProps *props)
{
    RwBool              result = TRUE;
    RWFUNCTION(RWSTRING("_rt2dAnimDestruct"));
    RWASSERT(anim);
    /* don't assert props; legally it may not be provided */

    /* Free keyframeList */
    Rt2dAnimForAllKeyFrameLists(
        anim,
        _rt2dAnimAdaptKeyFrameDestroy,
        props,
        &result);

    /* Free keyframe list */
    RWASSERT(anim->keyframeLists);
    rwSListDestroy( anim->keyframeLists );
    anim->keyframeLists = NULL;

    RWRETURN(anim);
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimDestroy destroys a 2d animation.
 *
 * \param anim  Pointer to an animation
 * \param props Pointer to the props that the animation will modify
 *
 * \return Returns TRUE if successful, FALSE otherwise.
 *
 * \see Rt2dAnimCreate
 */
RwBool
Rt2dAnimDestroy(Rt2dAnim *anim, Rt2dAnimProps *props)
{
    RwBool              result;
    RWAPIFUNCTION(RWSTRING("Rt2dAnimDestroy"));
    RWASSERT(anim);
    /* don't assert props; legally it may not be provided */

    /* NULL path is valid */
    result = (NULL != anim);

    if (result)
    {
        /* Destruct the anim of all data. */
        _rt2dAnimDestruct(anim, props);

        /* Free animation itself */
        RwFreeListFree(&Rt2dAnimGlobals.animFreeList, anim);
    }

    RWRETURN(result);
}

static Rt2dKeyFrameList *
_rt2dAnimKeyFrameListLock(Rt2dAnim *anim __RWUNUSED__ , Rt2dAnimProps *props,
                          Rt2dKeyFrameList *keyframeList,
                          RwReal keyframeTime __RWUNUSED__ ,
                          void *data __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("_rt2dAnimKeyFrameListLock"));
    RWASSERT(keyframeList);
    RWASSERT(props);

    RWRETURN(Rt2dKeyFrameListLock(keyframeList, props));
}


/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimLock locks a 2d animation so that it may be modified.
 *
 * \param anim  Pointer to an animation
 * \param props Pointer to the props that the animation will modify
 *
 * \return Returns a pointer to the locked animation if successful, NULL otherwise.
 *
 * \see Rt2dAnimUnlock
 */
Rt2dAnim *
Rt2dAnimLock(Rt2dAnim *anim, Rt2dAnimProps *props)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimLock"));
    RWASSERT(anim);

    /* Lock keyframeList */
    Rt2dAnimForAllKeyFrameLists(
        anim,
        _rt2dAnimKeyFrameListLock,
        props,
        (void *)NULL);

    /* Set the lock flag */
    anim->flag |= Rt2dAnimationIsLocked;

    RWRETURN(anim);
}

static Rt2dKeyFrameList *
_rt2dKeyFrameListUnlock(Rt2dAnim *anim __RWUNUSED__ , Rt2dAnimProps *props,
                        Rt2dKeyFrameList *keyframeList,
                        RwReal keyframeTime __RWUNUSED__ ,
                        void *data __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("_rt2dKeyFrameListUnlock"));
    RWASSERT(keyframeList);
    RWASSERT(props);

    RWRETURN(Rt2dKeyFrameListUnlock(keyframeList, props));
}


/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimUnlock unlocks a 2d animation so that it may be played or streamed out.
 *
 * \param anim  Pointer to an animation
 * \param props Pointer to the props that the animation will modify
 *
 * \return Returns a pointer to the unlocked animation if successful, NULL otherwise
 *
 * \see Rt2dAnimLock
 */
Rt2dAnim *
Rt2dAnimUnlock(Rt2dAnim *anim, Rt2dAnimProps *props)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimUnlock"));
    RWASSERT(anim);

    /* Unlock the keyframe list */
    Rt2dAnimForAllKeyFrameLists(
        anim,
        _rt2dKeyFrameListUnlock,
        props,
        NULL);

    /* Reset the lock flag */
    anim->flag &= ~((RwUInt32)(Rt2dAnimationIsLocked));

    RWRETURN(anim);
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimAddKeyFrameList adds a keyframe list of changes to be applied to a 2d
 * scene at a given time. Keyframe lists should be added in time order. Keyframe lists
 * may only be added to locked animations.
 *
 * \param anim          Pointer to an animation
 * \param keyframeList  Pointer to the keyframe list to be added
 * \param time          time at which the keyframe list will be applied during the
 *                      course of the animation
 *
 * \return Returns the 2d animation if successful, NULL otherwise
 * \see Rt2dAnimLock
 */
Rt2dAnim *
Rt2dAnimAddKeyFrameList(Rt2dAnim *anim, Rt2dKeyFrameList *keyframeList,
                        RwReal time)
{
    _rt2dAnimKeyFrameSched *sched;

    RWAPIFUNCTION(RWSTRING("Rt2dAnimAddKeyFrameList"));
    RWASSERT(anim);

    sched = (_rt2dAnimKeyFrameSched *)
                rwSListGetNewEntry(anim->keyframeLists,
                rwID_2DKEYFRAME | rwMEMHINTDUR_EVENT);
    RWASSERT(sched);

    sched->keyframeList = keyframeList;
    sched->time = time;

    /* Reasonable values for prev and next */
    anim->next = (_rt2dAnimKeyFrameSched*)
                        (rwSListGetArray(anim->keyframeLists));

    if ( (!anim->last) || (time > anim->last->time) )
    {
        anim->last = sched;
    }

    RWRETURN(anim);
}

static Rt2dKeyFrameList *
_rt2dAnimAccumulateSizeOfKeyFrameListCallBack(
                       Rt2dAnim *anim __RWUNUSED__,
                       Rt2dAnimProps *props __RWUNUSED__,
                       Rt2dKeyFrameList *keyframeList,
                       RwReal time __RWUNUSED__,
                       void *data)
{
    RWFUNCTION(RWSTRING("_rt2dAnimAccumulateSizeOfKeyFrameListCallBack"))
    RWASSERT(anim);
    RWASSERT(props);
    RWASSERT(keyframeList);

    *((RwInt32*)(data)) += Rt2dKeyFrameListStreamGetSize(keyframeList);

    RWRETURN(keyframeList);
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimStreamGetSize is used to determine the size in bytes of
 * the binary representation of a 2d animation. This value is used in the
 * binary chunk header to indicate the size of the chunk. The size includes
 * the size of the chunk header.
 *
 * \param anim  Pointer to an animation
 * \param props Pointer to the props that the animation will modify
 *
 * \return Returns a \ref RwUInt32 value equal to the chunk size (in bytes)
 * of the shape.
 *
 * \see Rt2dAnimStreamRead
 * \see Rt2dAnimStreamWrite
 */
RwUInt32
Rt2dAnimStreamGetSize(Rt2dAnim *anim, Rt2dAnimProps *props)
{
    RwUInt32 size;
    RwUInt32 sizeOfKeyframeLists=0;

    RWAPIFUNCTION(RWSTRING("Rt2dAnimStreamGetSize"));
    RWASSERT(anim);

    /* Scene is the size of the chunk header... */
    size =  rwCHUNKHEADERSIZE;

    /* ...plus the size of the frame count */
    size += sizeof(RwUInt32);

    /* ...plus the size of the frame times */
    size += sizeof(RwReal) * rwSListGetNumEntries(anim->keyframeLists);

    /* ...plus the sum of the sizes of all the keyframeLists */
    Rt2dAnimForAllKeyFrameLists(
                            anim,
                            _rt2dAnimAccumulateSizeOfKeyFrameListCallBack,
                            props,
                            &sizeOfKeyframeLists);
    size += sizeOfKeyframeLists;

    RWRETURN(size);
}


Rt2dAnim *
_rt2dAnimStreamRead(Rt2dAnim *anim, RwStream* stream,
                   Rt2dAnimProps *props __RWUNUSED__)
{
    RwInt32 keyframeCount = 0;
    _rt2dAnimKeyFrameSched *first, *finish, *keyframeSched;
    Rt2dKeyFrameList *keyframeList;
    RwReal keyframeTime;

    RWFUNCTION(RWSTRING("_rt2dAnimStreamRead"));

    RWASSERT(anim);
    RWASSERT(stream);

    /* Get number of scene children */
    if (RwStreamRead(stream, &keyframeCount, sizeof(keyframeCount))
          != sizeof(keyframeCount))
    {
        RWRETURN((Rt2dAnim *)NULL);
    }

    /* Convert to machine format */
    (void)RwMemNative32(&keyframeCount, sizeof(keyframeCount));

    _rt2dAnimInit(anim, keyframeCount);

    /* Read in frames */
    /* Cannot use ForAllKeyFrames because need to overwrite keyframe ptrs */
    first = (_rt2dAnimKeyFrameSched *)
                rwSListGetArray(anim->keyframeLists);
    finish = first + keyframeCount;

    for(keyframeSched = first; keyframeSched != finish; ++keyframeSched)
    {
        /* read time */
        if (RwStreamRead(stream, &keyframeTime, sizeof(keyframeTime))
             != sizeof(keyframeTime))
        {
            RWRETURN((Rt2dAnim *)NULL);
        }

        /* convert the time */
        (void)RwMemNative32(&keyframeTime, sizeof(keyframeTime));
        (void)RwMemFloat32ToReal(&keyframeTime, sizeof(keyframeTime));

        /* store the time */
        keyframeSched->time = keyframeTime;

        /* read keyframe */
        if( !RwStreamFindChunk(stream, rwID_2DKEYFRAME,
                        (RwUInt32 *)NULL, (RwUInt32 *)NULL) )
        {
            RWRETURN((Rt2dAnim*)NULL);
        }


        keyframeList = Rt2dKeyFrameListStreamRead(stream);
        if (!keyframeList)
        {
            RWRETURN((Rt2dAnim*)NULL);
        }

        /* store the keyframe */
        keyframeSched->keyframeList = keyframeList;

        /* Reasonable values for prev and next */
        anim->next = (_rt2dAnimKeyFrameSched*)
                            rwSListGetArray(anim->keyframeLists);

        if ( (!anim->last) || (keyframeTime > anim->last->time) )
        /* update 'last' ptr */
        {
            anim->last = keyframeSched;
        }
    }

    RWRETURN(anim);
}


/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimStreamRead reads an animation from a binary stream.
 * Note that prior to this function call a binary animation chunk must be found
 * in the stream using the \ref RwStreamFindChunk API function.
 *
 * The sequence to locate and read animation from a binary stream is
 * as follows:
 * \code
   RwStream *stream;
   Rt2dAnim *newAnimation;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_2DANIM, NULL, NULL) )
       {
           newAnimation = Rt2dAnimStreamRead(stream);
       }

       RwStreamClose(stream, NULL);
   }
   \endcode
 *
 * \param stream   Pointer to the stream the animation will be read from
 * \param props    the props object that will be modified by the animation
 *                 (currently not used)
 *
 * \return Returns a pointer to the animation if successful, NULL otherwise.
 *
 * \see Rt2dAnimStreamWrite
 * \see Rt2dAnimStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RwStreamFindChunk
 *
 */
Rt2dAnim *
Rt2dAnimStreamRead(RwStream* stream,
                   Rt2dAnimProps *props __RWUNUSED__)
{
    Rt2dAnim *anim;

    RWAPIFUNCTION(RWSTRING("Rt2dAnimStreamRead"));

    RWASSERT(stream);

    /* Initialise animation with correct number of frames */
    anim = (Rt2dAnim*)RwFreeListAlloc(&Rt2dAnimGlobals.animFreeList,
                                      rwID_2DANIM | rwMEMHINTDUR_EVENT);

    if (anim)
    {
        _rt2dAnimStreamRead(anim, stream, props);
    }

    RWRETURN(anim);
}

 static Rt2dKeyFrameList *
_rt2dAnimWriteKeyFrameListToStreamCallBack(
                       Rt2dAnim *anim __RWUNUSED__,
                       Rt2dAnimProps *props __RWUNUSED__,
                       Rt2dKeyFrameList *keyframeList,
                       RwReal time,
                       void *data)
{
    RWFUNCTION(RWSTRING("_rt2dAnimWriteKeyFrameListToStreamCallBack"))
    RWASSERT(anim);

    /* Convert the time */
    (void)RwMemRealToFloat32(&time, sizeof(time));
    (void)RwMemLittleEndian32(&time, sizeof(time));

    /* Write keyframe time */
    if (!RwStreamWrite((RwStream *)data, &time, sizeof(time)))
    {
        RWRETURN((Rt2dKeyFrameList *)NULL);
    }

    /* Write the keyframe */
    RWRETURN(Rt2dKeyFrameListStreamWrite(keyframeList, (RwStream *)data) );
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimStreamWrite is used to write the specified animation
 * to the given binary stream. Note that the stream will have been
 * opened prior to this function call.
 *
 * \param anim     Pointer to the animation to stream.
 * \param stream   Pointer to the stream to write to.
 * \param props    Pointer to the props that the animation was acting upon
 *
 * \return Returns a new animation if successful, NULL otherwise.
 *
 * \see Rt2dAnimStreamRead
 * \see Rt2dAnimStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 */
Rt2dAnim *
Rt2dAnimStreamWrite(Rt2dAnim *anim, RwStream *stream,
                        Rt2dAnimProps *props)
{
    RwUInt32 size;
    RwUInt32 frameCount;

    RWAPIFUNCTION(RWSTRING("Rt2dAnimStreamWrite"));
    RWASSERT(anim);
    RWASSERT(stream);

    /* Write header */
    size = Rt2dAnimStreamGetSize(anim, props);
    if (!RwStreamWriteChunkHeader(stream, rwID_2DANIM, size))
    {
        RWRETURN((Rt2dAnim *)NULL);
    }

    /* Convert number of frames in stream */
    frameCount = rwSListGetNumEntries(anim->keyframeLists);
    (void)RwMemLittleEndian32(&frameCount, sizeof(frameCount));

    /* Write number of keyframes in animation */
    if (!RwStreamWrite(stream, &frameCount, sizeof(frameCount)))
    {
        RWRETURN((Rt2dAnim *)NULL);
    }

    /* Write each object in scene */
    Rt2dAnimForAllKeyFrameLists(anim,
                                _rt2dAnimWriteKeyFrameListToStreamCallBack,
                                props,
                                (void *)stream);

    RWRETURN(anim);
}

/* Playback */

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimReset efficiently resets the animation back to the start
 *
 * \param anim   Pointer to the animation to be reset
 * \param props  Pointer to the props that the animation acts upon
 *
 * \return Returns the animation if successful, NULL otherwise.
 *
 */
Rt2dAnim *
Rt2dAnimReset(Rt2dAnim *anim, Rt2dAnimProps *props)
{
    RwInt32 numEntries = rwSListGetNumEntries(anim->keyframeLists);

    RWAPIFUNCTION(RWSTRING("Rt2dAnimReset"));
    RWASSERT(anim);

    anim->currentTime = 0;
    if (numEntries>=1)
    {
        anim->prev = NULL;

        anim->next = (_rt2dAnimKeyFrameSched *)
                        rwSListGetEntry(anim->keyframeLists, 0);

        anim->last = (_rt2dAnimKeyFrameSched *)
                        rwSListGetEntry(anim->keyframeLists, numEntries-1);

        /* DUMMY - May need to set all changes in props to identity before
         *         this
         */
        Rt2dKeyFrameListAdvance(anim->next->keyframeList, props);
    }
    else
    {
        anim->prev = NULL;
        anim->next = NULL;
        anim->last = NULL;
    }

    RWRETURN(anim);
}

/**
 * \ingroup rt2danimsub
 *
 * \ref Rt2dAnimSetDeltaTimeScale Sets the time scale to be applied to each
 * delta time during playback. Can be used to slow, pause or play the animation
 * in reverse. The default scale is 1.0.
 *
 * \param anim       Pointer to the animation to set the time scale of
 * \param timeScale  New time scale factor
 *
 * \return Returns the animation if successful, NULL otherwise.
 *
 * \see Rt2dAnimAddDeltaTime
 */
Rt2dAnim *
Rt2dAnimSetDeltaTimeScale(Rt2dAnim *anim, RwReal timeScale)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimSetDeltaTimeScale"));
    RWASSERT(anim);

    anim->deltaTimeScale = timeScale;

    RWRETURN(anim);
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimAddDeltaTime notifies the animation system that time has passed. Does
 * not update any 2d objects.
 *
 * \param anim       Pointer to the animation to add time to
 * \param props      Pointer to the props that the animation updates
 * \param deltaTime  Change in time to be noted
 *
 * \return Returns the animation if successful, NULL otherwise.
 *
 * \see Rt2dAnimTimeUpdate
 * \see Rt2dAnimSetDeltaTimeScale
 * \see Rt2dAnimAddDeltaFrameIndex
 * \see Rt2dAnimGotoKeyFrameListByTime
 * \see Rt2dAnimGotoKeyFrameListByIndex
 */
Rt2dAnim *
Rt2dAnimAddDeltaTime(Rt2dAnim *anim, Rt2dAnimProps *props,
                         RwReal deltaTime)
{
    RwReal newTime, scaledDeltaTime;

    RWAPIFUNCTION(RWSTRING("Rt2dAnimAddDeltaTime"));
    RWASSERT(anim);

    /* Scale the time that was given to us */
    scaledDeltaTime = deltaTime*anim->deltaTimeScale;

    /* Calculate the target time */
    newTime = anim->currentTime + scaledDeltaTime;

    /* Goto the target time */
    Rt2dAnimGotoKeyFrameListByTime(anim, props, newTime);

    RWRETURN(anim);
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimAddDeltaFrameIndex moves the animation forward by the specified number of
 * frames. Analagous to Rt2dAnimAddDeltaTime, except advances in terms of frames instead
 * of time. Does not update any 2d objects.
 *
 * \param anim        Pointer to the animation to advance or rewind
 * \param props       Pointer to the props that the animation acts upon
 * \param deltaFrame  Number of frames by which to advance the animation (may be -ve)
 *
 * \return Returns the animation if successful, NULL otherwise.
 *
 * \see Rt2dAnimTimeUpdate
 * \see Rt2dAnimAddDeltaTime
 * \see Rt2dAnimGotoKeyFrameListByTime
 * \see Rt2dAnimGotoKeyFrameListByIndex
 */
Rt2dAnim *
Rt2dAnimAddDeltaFrameIndex(Rt2dAnim *anim, Rt2dAnimProps *props,
                 RwInt32 deltaFrame)
{
    _rt2dAnimKeyFrameSched *first;

    RWAPIFUNCTION(RWSTRING("Rt2dAnimAddDeltaFrameIndex"));
    RWASSERT(anim);

    /* Note that deltaFrame is used as a counter here, and also as a loop
       check condition */

    /* Store first keyframe for checks */
    first = (_rt2dAnimKeyFrameSched*)rwSListGetArray(anim->keyframeLists);

    /* Advance (if necessary and not at end) */
    while ( (deltaFrame > 0) && (anim->next) )
    {
        anim->currentTime = anim->next->time;
        Rt2dKeyFrameListAdvance(anim->next->keyframeList, props);
        anim->prev = anim->next;
        if ( anim->next < anim->last)
            ++anim->next;
        else
            anim->next = NULL;
        --deltaFrame;
    }

    /* Rewind (if necessary and not at end) */
    while ( (deltaFrame < 0) && (anim->prev) )
    {
        anim->currentTime = anim->prev->time;
        Rt2dKeyFrameListAdvance(anim->prev->keyframeList, props);

        anim->next = anim->prev;
        if ( anim->prev > first)
            --anim->prev;
        else
            anim->prev = NULL;
        --deltaFrame;
    }

    RWRETURN(anim);
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimTimeUpdate applies the changes accumulated during time or frame
 * updates to the 2d objects in the animation props. After this operation
 * the position of objects within the props will be valid and can be tested (eg for
 * collisions etc). At this point they are also ready to be rendered.
 *
 * \param anim    Pointer to the animation to apply the time update for
 * \param props   Pointer to the props that contain the changes to applied and a
 *                reference to the scene which will be updated
 * \return Returns the animation if successful, NULL otherwise.
 *
 * \see Rt2dAnimAddDeltaTime
 * \see Rt2dAnimAddDeltaFrameIndex
 * \see Rt2dAnimGotoKeyFrameListByTime
 * \see Rt2dAnimGotoKeyFrameListByIndex
 */
Rt2dAnim *
Rt2dAnimTimeUpdate(Rt2dAnim *anim, Rt2dAnimProps *props)
{
    RwReal interp;

    RWAPIFUNCTION(RWSTRING("Rt2dAnimTimeUpdate"));
    RWASSERT(anim);

    if(Rt2dAnimIsInterpolated(anim))
    {
        /* Calculate the time length of the frame */
        if (anim->prev && anim->next)
        {
            RwReal frameLength;

            frameLength = anim->next->time - anim->prev->time;

            /* Work out fractional time of current time between previous and next
            keyframe */
            interp = (anim->currentTime - anim->prev->time) / frameLength;
        }
        else
        {
            interp = 0.0f;
        }
    }
    else
    {
        interp = 0.0f;
    }

    /* Do the time update */
    if (anim->next)
    {
        if( Rt2dKeyFrameListApply(
                 anim->next->keyframeList,
                 props,
                 interp))
        {
            RWRETURN(anim);
        }
    } else if (anim->last) {
        if( Rt2dKeyFrameListApply(
                 anim->last->keyframeList,
                 props,
                 interp))
        {
            RWRETURN(anim);
        }
    }


    RWRETURN(NULL);
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimGotoKeyFrameListByTime moves the animation to a given position in time
 * from the start of the animation. Does not update any 2d objects.
 *
 * \param anim     Pointer to the animation which is to be repositioned
 * \param props    Pointer to the props that the animation acts upon
 * \param newTime  Time to which the animation will be repositioned
 *
 * \return Returns the animation if successful, NULL otherwise.
 *
 * \see Rt2dAnimTimeUpdate
 * \see Rt2dAnimAddDeltaTime
 * \see Rt2dAnimAddDeltaFrameIndex
 * \see Rt2dAnimGotoKeyFrameListByIndex
 */
Rt2dAnim *
Rt2dAnimGotoKeyFrameListByTime(Rt2dAnim *anim, Rt2dAnimProps *props,
                               RwReal newTime)
{
    RwBool passedEnd = FALSE;
    RwReal remainingUnscaledDelta = 0.0f;
    RwInt32 deltaFrame;

    RWAPIFUNCTION(RWSTRING("Rt2dAnimGotoKeyFrameListByTime"));
    RWASSERT(anim);

    /* Which way are we going? */
    if (newTime > anim->currentTime)
    {
        deltaFrame =  1;
    } else if (newTime < anim->currentTime) {
        deltaFrame = -1;
    } else {
        /* no time advance */
        RWRETURN(anim);
    }

    /* Store whether or not we went past end, adjusting time if necessary */
    if (newTime < 0.0f )
    {
        passedEnd = TRUE;

        /* Store remaining time from point of view of caller */
        remainingUnscaledDelta = newTime / anim->deltaTimeScale;

        /* Limit new time */
        newTime = 0.0f;
    }
    else if (newTime > anim->last->time)
    {
        passedEnd = TRUE;

        /* Store remaining time from point of view of caller */
        remainingUnscaledDelta = (newTime - anim->last->time)
                                  / anim->deltaTimeScale;

        /* Limit new time */
        newTime = anim->last->time;
    }

    while (     ( anim->next && (anim->next->time <= newTime) )
            ||  ( anim->prev && (anim->prev->time > newTime) ) )
    {
        /* Frame advance */
        Rt2dAnimAddDeltaFrameIndex(anim, props, deltaFrame);
    }

    /* Set new time */
    anim->currentTime = newTime;

    /* If end was passed, need to generate callback */
    if (TRUE == passedEnd && NULL != _rt2dAnimOnEndReachedCallBack)
    {
        (*_rt2dAnimOnEndReachedCallBack)(anim, props, remainingUnscaledDelta);
    }

    RWRETURN(anim);
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimGotoKeyFrameListByIndex moves the animation to a given frame
 * from the start of the animation. Does not update any 2d objects.
 *
 * \param anim       Pointer to the animation whose position will be updated
 * \param props      Pointer to the props that the animation is applied to
 * \param frameIndex Index of the frame to go to
 *
 * \return Returns the animation if successful, NULL otherwise.
 *
 * \see Rt2dAnimTimeUpdate
 * \see Rt2dAnimAddDeltaTime
 * \see Rt2dAnimAddDeltaFrameIndex
 * \see Rt2dAnimGotoKeyFrameListByTime
 */
Rt2dAnim *
Rt2dAnimGotoKeyFrameListByIndex(Rt2dAnim *anim, Rt2dAnimProps *props,
                                RwInt32 frameIndex)
{
    _rt2dAnimKeyFrameSched *targetFrame;
    RwInt32 deltaFrame;

    RWAPIFUNCTION(RWSTRING("Rt2dAnimGotoKeyFrameListByIndex"));
    RWASSERT(anim);
    RWASSERT(frameIndex>=0);
    RWASSERT(frameIndex<rwSListGetNumEntries(anim->keyframeLists));

    /* Frame indices are not explicitly stored; work out the target frame */
    targetFrame = (_rt2dAnimKeyFrameSched*)rwSListGetArray(anim->keyframeLists)
                    + frameIndex;

    if (targetFrame < anim->prev) /* should still be NULL correct for anim->prev */
    {
        /* Need to do this otherwise don't do visibility correctly */
        Rt2dAnimReset(anim, props);
        Rt2dAnimAddDeltaFrameIndex(anim, props, 1);
    }

    /* Turn the target frame into a delta frame */
    if (anim->prev)
        deltaFrame = targetFrame - anim->prev;
    else
        deltaFrame = frameIndex;

    RWRETURN( Rt2dAnimAddDeltaFrameIndex(anim, props, deltaFrame) );
}

/* Utilities */
/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimForAllKeyFrameLists calls the given callback for every keyframe list
 * in the animation.
 *
 * \param anim     Pointer to the animation whose keyframe lists the callback will be
 *                 applied to
 * \param callBack Callback function that will be applied to each keyframe list
 * \param props    Pointer to the props object that the animation updates
 * \param data     Pointer to user defined data that will be passed to the callback
 *                 with each keyframe list
 *
 * \return Returns the animation if successful, NULL otherwise.
 *
 * \see Rt2dKeyFrameListCallBack
 */
Rt2dAnim *
Rt2dAnimForAllKeyFrameLists(Rt2dAnim *anim,
                            Rt2dKeyFrameListCallBack callBack,
                            Rt2dAnimProps *props, void *data)
{
    _rt2dAnimKeyFrameSched *first, *finish, *keyframeSched;

    RWAPIFUNCTION(RWSTRING("Rt2dAnimForAllKeyFrameLists"));
    RWASSERT(anim);
    RWASSERT(props);

    /* Limits of loop */
    first = (_rt2dAnimKeyFrameSched *)
                rwSListGetArray(anim->keyframeLists);
    finish = first + rwSListGetNumEntries(anim->keyframeLists);

    for(keyframeSched = first; keyframeSched != finish; ++keyframeSched)
    {
        (callBack)(anim, props, keyframeSched->keyframeList, keyframeSched->time,
                   data);
    }

    RWRETURN(anim);
}

/* Interrogation */
/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimGetNumberOfKeyFrames gets the number of keyframes in the animation.
 *
 * \param anim    Pointer to the animation for which the number of keyframe lists
 *                will be obtained
 *
 * \return Returns the number of key frame lists in the animation
 *
 * \see Rt2dAnimAddKeyFrameList
 */
RwInt32
Rt2dAnimGetNumberOfKeyFrames(Rt2dAnim *anim)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimGetNumberOfKeyFrames"));
    RWASSERT(anim);

    RWRETURN( rwSListGetNumEntries(anim->keyframeLists) );
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimGetKeyFrameListByIndex returns a given keyframe list by its index.
 *
 * \param anim        Pointer to the animation from which the keyframe list will be
 *                    obtained
 * \param frameIndex  Index of the frame list to be obtained
 *
 * \return Returns a pointer to the keyframe list if successful, NULL otherwise.
 */
Rt2dKeyFrameList *
Rt2dAnimGetKeyFrameListByIndex(Rt2dAnim *anim, RwUInt32 frameIndex)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimGetKeyFrameListByIndex"));
    RWASSERT(anim);

    RWRETURN( ((_rt2dAnimKeyFrameSched*)(rwSListGetEntry(anim->keyframeLists,
                                                         frameIndex)))
                  ->keyframeList
            );
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimGetCurrentTime returns the time since the start of the animation.
 *
 * \param anim    Pointer to the animation for which the current time will be
 *                obtained
 *
 * \return Returns the time since the start of the animation
 *
 * \see Rt2dAnimGetFinalKeyFrameListTime
 * \see Rt2dAnimGetPrevFrameTime
 * \see Rt2dAnimGetNextFrameTime
 */
RwReal
Rt2dAnimGetCurrentTime(Rt2dAnim *anim)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimGetCurrentTime"));
    RWASSERT(anim);

    RWRETURN( anim->currentTime );
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimGetFinalKeyFrameListTime gets the time of the final frame
 *
 * \param anim   Pointer to the animation used
 *
 * \return Returns time of the final keyframe list.
 *
 * \see Rt2dAnimGetPrevFrameTime
 * \see Rt2dAnimGetNextFrameTime
 */
RwReal
Rt2dAnimGetFinalKeyFrameListTime(Rt2dAnim *anim)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimGetFinalKeyFrameListTime"));
    RWASSERT( anim );

    RWRETURN( anim->last->time );
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimGetPrevFrameIndex gets the index of the previous frame.
 *
 * \param anim   Pointer to the animation used
 *
 * \return Returns the index of the previous frame
 *
 * \see Rt2dAnimGetNextFrameIndex
 */
RwInt32
Rt2dAnimGetPrevFrameIndex(Rt2dAnim *anim)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimGetPrevFrameIndex"));
    RWASSERT( anim );

    if (anim->prev)
        RWRETURN( anim->prev
                   - (_rt2dAnimKeyFrameSched*)rwSListGetArray(anim->keyframeLists) );
    else
        RWRETURN( -1 );

}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimGetPrevFrameTime gets the time of the previous frame.
 *
 * \param anim   Pointer to the animation used
 *
 * \return Returns the time of the previous frame
 *
 * \see Rt2dAnimGetFinalKeyFrameListTime
 * \see Rt2dAnimGetPrevFrameTime
 */
RwReal
Rt2dAnimGetPrevFrameTime(Rt2dAnim *anim)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimGetPrevFrameTime"));
    RWASSERT( anim );

    if (anim->prev)
        RWRETURN( anim->prev->time );
    else
        RWRETURN(0.0f);
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimGetNextFrameIndex gets the index of the next frame.
 *
 * \param anim   Pointer to the animation used
 *
 * \return Returns the index of the next frame
 *
 * \see Rt2dAnimGetPrevFrameIndex
 */
RwInt32
Rt2dAnimGetNextFrameIndex(Rt2dAnim *anim)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimGetNextFrameIndex"));
    RWASSERT( anim );

    if (anim->next)
        RWRETURN( anim->next
                  - (_rt2dAnimKeyFrameSched*)rwSListGetArray(anim->keyframeLists) );
    else
        RWRETURN( rwSListGetNumEntries(anim->keyframeLists) );
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimGetNextFrameTime gets the time of the next frame.
 *
 * \param anim   Pointer to the animation used
 *
 * \return Returns the time of the next frame
 *
 * \see Rt2dAnimGetPrevFrameTime
 */
RwReal
Rt2dAnimGetNextFrameTime(Rt2dAnim *anim)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimGetNextFrameTime"));
    RWASSERT( anim );

    if (anim->next)
        RWRETURN( anim->next->time );
    else
        RWRETURN( anim->last->time );
}


/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimIsInterpolated indicates if the animation is set to interpolate changes
 * between successive keyframe lists
 *
 * \param anim   Pointer to the animation used
 *
 * \return Returns TRUE if the animation is set to interpolate, FALSE otherwise.
 *
 * \see Rt2dAnimSetInterpolate
 */
RwBool
Rt2dAnimIsInterpolated(Rt2dAnim *anim)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimIsInterpolated"));

    RWRETURN((anim->flag & Rt2dAnimationIsInterpolated) ? TRUE : FALSE);
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimSetInterpolate sets the interpolation status of the animation. Setting
 * the interpolation status to TRUE causes changes to 2d objects to be interpolated
 * smoothly between keyframes. Default is FALSE.
 *
 * \param anim         Pointer to the animation used
 * \param interpolate  New interpolation status, TRUE will cause animations to be
 *                     interpolated between keyframes.
 *
 * \return Returns the animation whose interpolation status was set.
 *
 * \see Rt2dAnimIsInterpolated
 */
Rt2dAnim *
Rt2dAnimSetInterpolate(Rt2dAnim *anim, RwBool interpolate)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimSetInterpolate"));

    if(interpolate)
    {
        anim->flag |= Rt2dAnimationIsInterpolated;
    }
    else
    {
        anim->flag &= ~Rt2dAnimationIsInterpolated;
    }

    RWRETURN(anim);
}

/****************************************************************************/

Rt2dAnim *
_rt2dAnimCopy(Rt2dAnim *destAnim, Rt2dAnim *srcAnim, Rt2dAnimProps *srcProps)
{
    RwChunkHeaderInfo  chunkHeader;
    RwStream *stream;
    RwMemory buffer;

    RWFUNCTION(RWSTRING("_rt2dAnimCopy"));

    /* Copy the animation */
    buffer.length = 0;
    buffer.start = (RwUInt8 *)NULL;

    stream = RwStreamOpen(rwSTREAMMEMORY , rwSTREAMWRITE, &buffer);
    RWASSERT(stream);

    if( !Rt2dAnimStreamWrite(srcAnim, stream, srcProps) )
    {
        RWRETURN((Rt2dAnim *)NULL);
    }

    RwStreamClose(stream, &buffer);

    stream = RwStreamOpen(rwSTREAMMEMORY , rwSTREAMREAD, &buffer);
    RWASSERT(stream);

    if (RwStreamReadChunkHeaderInfo(stream, &chunkHeader) == NULL)
        RWRETURN(NULL);

    if( !_rt2dAnimStreamRead(destAnim, stream, NULL) )
    {
        RWRETURN((Rt2dAnim *)NULL);
    }

    RwStreamClose(stream, &buffer);

    RwFree(buffer.start);

    RWRETURN(srcAnim);
}


/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimCopy makes a new copy of an animation. The caller is
 * responsible for destroying the new copy.
 *
 * \param srcAnim       Pointer to a source animation
 * \param srcProps      Pointer to the source props
 *
 * \return Returns the new animation if successful, NULL otherwise.
 *
 * \see Rt2dAnimDestroy
 */
Rt2dAnim *
Rt2dAnimCopy(Rt2dAnim *srcAnim, Rt2dAnimProps *srcProps)
{
    Rt2dAnim *destAnim;

    RWAPIFUNCTION(RWSTRING("Rt2dAnimCopy"));

    RWASSERT(srcAnim);
    RWASSERT(srcProps);

    if ((destAnim = Rt2dAnimCreate()) == NULL)
    {
        RWRETURN( (Rt2dAnim *)NULL );
    }

    if (_rt2dAnimCopy(destAnim, srcAnim, srcProps) == NULL)
    {
        RWRETURN( (Rt2dAnim *)NULL );
    }

    RWRETURN(destAnim);
}


#if defined (__MWERKS__)
#if (defined(RWVERBOSE))
#pragma message (__FILE__ "/" _SKY_EXPAND(__LINE__) ": __MWERKS__ == " _SKY_EXPAND(__MWERKS__))
#endif /* (defined (__MWERKS__)) */
#if (__option (global_optimizer))
#pragma always_inline on
#endif /* (__option (global_optimizer)) */
#endif /*  defined (__MWERKS__) */
