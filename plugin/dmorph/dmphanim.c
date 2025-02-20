/*
 *  dmphanim.c - delta morph animation
 */

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rpplugin.h"
#include <rpdbgerr.h>
#include <rwcore.h>
#include <rpworld.h>

#include "dmorph.h"
#include "dmphanim.h"

/*===========================================================================*
 *--- API functions ---------------------------------------------------------*
 *===========================================================================*/

/*---- DMorphAnimation functions -------------------------------------------*/

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAnimationCreate is used to create a delta morph animation
 * that may be used to control a delta morph atomic.
 *
 * The animation contains a keyframe sequence for every delta morph target
 * which specifies how the amount of that target varies over time. The
 * sequences can be created for each target using
 * \ref RpDMorphAnimationCreateFrames. A sequence may be absent for morph
 * targets which are not controlled by the animation.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param numDMorphTargets  The number of delta morph targets that the
 *                          animation will control.
 *
 * \return A pointer to the created animation, or NULL if there is an
 *         error.
 *
 * \see RpDMorphAnimationGetNumDMorphTargets
 * \see RpDMorphAnimationDestroy
 * \see RpDMorphAnimationCreateFrames
 * \see RpDMorphAnimationFrameSet
 * \see RpDMorphAnimationStreamRead
 * \see RpDMorphAnimationStreamWrite
 * \see RpDMorphAnimationRead
 * \see RpDMorphAnimationWrite
 * \see RpDMorphAtomicSetAnimation
 */
RpDMorphAnimation *
RpDMorphAnimationCreate(RwUInt32 numDMorphTargets)
{
    RpDMorphAnimation   *anim;
    RwUInt32            size;

    RWAPIFUNCTION(RWSTRING("RpDMorphAnimationCreate"));
    RWASSERT(numDMorphTargets > 0);

    /* Get size including frame sequence list */
    size = sizeof(RpDMorphAnimation) +
            numDMorphTargets * sizeof(rpDMorphFrameSequence);

    /* Get memory */
    anim = (RpDMorphAnimation *) RwMalloc(size,
        rwID_DMORPHANIMATION | rwMEMHINTDUR_EVENT);
    if( NULL == anim )
    {
        RWERROR((E_RW_NOMEM, size));
        RWRETURN((RpDMorphAnimation *)NULL);
    }

    /* Initialize */
    anim->numDMorphTargets = numDMorphTargets;
    anim->frameSequences = (rpDMorphFrameSequence *) (anim + 1);

    /* Initialize the frame sequence list */
    {
        RwUInt32        i;
        rpDMorphFrameSequence *sequence = anim->frameSequences;

        for( i = 0; i < numDMorphTargets; i++ )
        {
            sequence->numFrames = 0;
            sequence->frames = (rpDMorphFrame *) NULL;

            sequence++;
        }
    }

    RWRETURN(anim);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAnimationDestroy destroys a delta morph animation and
 * any keyframe sequences it contains.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param anim  A pointer to the delta morph animation.
 *
 * \return NULL
 *
 * \see RpDMorphAnimationCreate
 */
RpDMorphAnimation *
RpDMorphAnimationDestroy(RpDMorphAnimation *anim)
{
    RwUInt32                i;
    rpDMorphFrameSequence   *sequence;

    RWAPIFUNCTION(RWSTRING("RpDMorphAnimationDestroy"));
    RWASSERT(anim);

    /* Destroy the frame sequences */
    sequence = anim->frameSequences;
    for( i = 0; i < anim->numDMorphTargets; i++ )
    {
        if (sequence->frames)
        {
            RwFree(sequence->frames);
        }

        sequence++;
    }

    /* Now destroy the anim struct */
    RwFree(anim);
    anim = (RpDMorphAnimation *) NULL;

    RWRETURN(anim);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAnimationCreateFrames creates a keyframe sequence in a
 * delta morph animation for driving a particular morph target.
 *
 * This function must be called to create a sequence for every morph target
 * that is to be controlled by the animation. Some sequences may be left
 * absent for morph targets which are unused or which are to be procedurally
 * controlled externally.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param anim               A pointer to the delta morph animation.
 * \param dMorphTargetIndex  Index of the delta morph target.
 * \param numFrames          Number of frames to create in the sequence.
 *
 * \return Returns NULL if there is an error, or otherwise
 *         a pointer to the delta morph animation.
 *
 * \see RpDMorphAnimationDestroyFrames
 * \see RpDMorphAnimationGetNumFrames
 * \see RpDMorphAnimationFrameSet
 */
RpDMorphAnimation   *
RpDMorphAnimationCreateFrames(RpDMorphAnimation  *anim,
                              RwUInt32           dMorphTargetIndex,
                              RwUInt32           numFrames)
{
    rpDMorphFrameSequence  *sequence;
    RwUInt32                size, i;

    RWAPIFUNCTION(RWSTRING("RpDMorphAnimationCreateFrames"));
    RWASSERT(anim);
    RWASSERT(dMorphTargetIndex < anim->numDMorphTargets);
    RWASSERT(numFrames > 0);

    /* Get sequence according to the morph target index */
    sequence = anim->frameSequences + dMorphTargetIndex;
    RWASSERT(sequence->frames == NULL);

    /* Get mem size required for frames */
    size = numFrames * sizeof(rpDMorphFrame);

    /* Get memory */
    sequence->frames = (rpDMorphFrame *) RwMalloc(size,
        rwID_DMORPHPLUGIN | rwMEMHINTDUR_EVENT);
    if( sequence->frames == NULL)
    {
        RWERROR((E_RW_NOMEM, size));
        RWRETURN((RpDMorphAnimation *)NULL);
    }

    /* Setup the DMorphTarget's Frames. */
    memset(sequence->frames, 0, size);

    for( i = 0; i < numFrames; i++ )
    {
        sequence->frames[i].nextFrame = rpDMORPHNULLFRAME;
    }

    sequence->numFrames = numFrames;

    RWRETURN(anim);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAnimationDestroyFrames destroys the keyframe sequence
 * in a delta morph animation corresponding to a particular morph target.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param anim                  Pointer to the delta morph animation.
 * \param dMorphTargetIndex     Index of the delta morph target whose
 *                              sequence is to be destroyed.
 *
 * \return Returns NULL if there is an error, or otherwise
 *         a pointer to the delta morph animation.
 *
 * \see RpDMorphAnimationCreateFrames
 * \see RpDMorphAnimationGetNumFrames
 */
RpDMorphAnimation *
RpDMorphAnimationDestroyFrames(RpDMorphAnimation  *anim,
                               RwUInt32           dMorphTargetIndex)
{
    rpDMorphFrameSequence    *sequence;

    RWAPIFUNCTION(RWSTRING("RpDMorphAnimationDestroyFrames"));
    RWASSERT(anim);
    RWASSERT(dMorphTargetIndex < anim->numDMorphTargets);

    /* Get sequence according to the morph target index */
    sequence = anim->frameSequences + dMorphTargetIndex;

    if (sequence->frames)
    {
        RwFree(sequence->frames);
        sequence->frames = (rpDMorphFrame *) NULL;
        sequence->numFrames = 0;
    }

    RWRETURN(anim);
}


/**
 * \ingroup rpdmorph
 * \ref RpDMorphAnimationGetNumDMorphTargets returns the number of delta
 * morph targets which may be controlled by a delta morph animation.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param animation     Pointer to the delta morph animation.
 *
 * \return Returns the number of delta morph targets.
 *
 * \see RpDMorphAnimationCreate
 * \see RpDMorphAnimationDestroy
 */
RwUInt32
RpDMorphAnimationGetNumDMorphTargets(RpDMorphAnimation *animation)
{
    RWAPIFUNCTION(RWSTRING("RpDMorphAnimationGetNumDMorphTargets"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(animation);

    RWRETURN(animation->numDMorphTargets);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAnimationGetNumFrames returns the number of keyframes in the
 * sequence of a delta morph animation corresponding to a particular
 * morph target.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param animation             Pointer to the delta morph animation.
 * \param dMorphTargetIndex     Index of the delta morph target.
 *
 * \return Returns the number of keyframes.
 *
 * \see RpDMorphAnimationCreateFrames
 * \see RpDMorphAnimationDestroyFrames
 * \see RpDMorphAnimationFrameSet
 */
RwUInt32
RpDMorphAnimationGetNumFrames(RpDMorphAnimation *animation,
                              RwUInt32          dMorphTargetIndex)
{
    RWAPIFUNCTION(RWSTRING("RpDMorphAnimationGetNumFrames"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(animation);
    RWASSERT(dMorphTargetIndex < animation->numDMorphTargets);

    RWRETURN(animation->frameSequences[dMorphTargetIndex].numFrames);
}

/*--- Animation Stream I/O ------------------------------------------------*/

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAnimationStreamRead read a delta morph animation from the
 * specified binary stream. Note that prior to this function call, a binary
 * delta morph animation chunk (rwID_DMORPHANIMATION) must be found in the
 * stream using the \ref RwStreamFindChunk API function.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * The sequence to locate and read a delta morph animation from a binary
 * stream is as follows:
 * \code
   RwStream *stream;
   RpDMorphAnimation *newDMorphAnimation;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_DMORPHANIMATION, NULL, NULL) )
       {
           newDMorphAnimation = RpDMorphAnimationStreamRead(stream);
       }

       RwStreamClose(stream, NULL);
   }
   \endcode
 *
 * \param stream    Pointer to the stream.
 *
 * \return  Pointer to the delta morph animation read from the stream, or
 *          NULL if there is an error.
 *
 * \see RpDMorphAnimationStreamWrite
 * \see RpDMorphAnimationStreamGetSize
 * \see RpDMorphAnimationRead
 * \see RpDMorphAnimationWrite
 * \see RpDMorphAnimationDestroy
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RwStreamFindChunk
 * \see RpDMorphPluginAttach
 */
RpDMorphAnimation *
RpDMorphAnimationStreamRead(RwStream *stream)
{
    RpDMorphAnimation   *animation = (RpDMorphAnimation *) NULL;
    RwUInt32            subChunkSize;
    RwUInt32            libVersion;
    RwUInt32            animVersion;
    RwUInt32            animFlags;
    RwUInt32            numDMorphTargets;
    RwUInt32            totalNumFrames;
    RwUInt32            i;

#ifdef RWDEBUG
    RwUInt32            checkSize;
#endif /* RWDEBUG */

    RWAPIFUNCTION(RWSTRING("RpDMorphAnimationStreamRead"));
    RWASSERT(stream);

    /* Internal struct chunk header */
    if (!RwStreamFindChunk(stream, rwID_STRUCT, &subChunkSize, &libVersion))
    {
        RWRETURN((RpDMorphAnimation *)NULL);
    }

    /* Header info */
    if (    !RwStreamReadInt(
                stream, (RwInt32 *) &animVersion, sizeof(RwInt32))
        ||  !RwStreamReadInt(
                stream, (RwInt32 *) &animFlags, sizeof(RwInt32))
        ||  !RwStreamReadInt(
                stream, (RwInt32 *) &numDMorphTargets, sizeof(RwInt32))
        ||  !RwStreamReadInt(
                stream, (RwInt32 *) &totalNumFrames, sizeof(RwInt32))   )
    {
        RWRETURN((RpDMorphAnimation *)NULL);
    }

    /* Check version */
    if (animVersion > rpDMORPHANIMATIONSTREAMCURRVERSION)
    {
        RWERROR((E_RW_BADVERSION));
        RWRETURN((RpDMorphAnimation *)NULL);
    }

#ifdef RWDEBUG
    checkSize = 4*sizeof(RwInt32);
#endif /* RWDEBUG */

    /* Create the animation */
    animation = RpDMorphAnimationCreate(numDMorphTargets);
    if(!animation)
    {
        RWRETURN((RpDMorphAnimation *)NULL);
    }

    /* Read in the frame sequences */
    for( i=0; i < numDMorphTargets; i++ )
    {
        RwUInt32    numFrames;

        /* Number of frames in this sequence */
        if (!RwStreamReadInt(stream, (RwInt32 *) &numFrames, sizeof(RwInt32)))
        {
            RWRETURN((RpDMorphAnimation *)NULL);
        }

#ifdef RWDEBUG
        checkSize += sizeof(RwInt32);
#endif /* RWDEBUG */

        /* Frame data */
        if (numFrames > 0)
        {
            rpDMorphFrame   *frame;
            RwUInt32        j;

            /* Allocate memory for the frames */
            if (!RpDMorphAnimationCreateFrames(animation, i, numFrames))
            {
                RWRETURN((RpDMorphAnimation *) NULL);
            }

            frame = animation->frameSequences[i].frames;
            RWASSERT(frame != NULL);

            for( j=0; j < numFrames; j++ )
            {
                if (    !RwStreamReadReal(stream,
                            &(frame->startValue), 4*sizeof(RwReal))
                    ||  !RwStreamReadInt(stream,
                            (RwInt32 *) &(frame->nextFrame), sizeof(RwInt32)) )
                {
                    RWRETURN((RpDMorphAnimation *)NULL);
                }

#ifdef RWDEBUG
                checkSize += 4*sizeof(RwReal) + sizeof(RwInt32);
#endif /* RWDEBUG */

                frame++;
            }
        }
    }

    RWASSERT(checkSize == subChunkSize);

    RWRETURN(animation);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAnimationStreamWrite writes a delta morph animation
 * to the specified binary stream, which must have been opened prior
 * to calling this function.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param animation     Pointer to the delta morph animation.
 * \param stream        Pointer to the binary stream.
 *
 * \return A pointer to the delta morph animation, or NULL if there is
 * an error.
 *
 * \see RpDMorphAnimationStreamWrite
 * \see RpDMorphAnimationStreamGetSize
 */
RpDMorphAnimation *
RpDMorphAnimationStreamWrite(RpDMorphAnimation *animation,
                             RwStream *stream)
{
    RwUInt32    chunkSize;
    RwUInt32    version;
    RwUInt32    flags;
    RwUInt32    totalNumFrames;
    RwUInt32    i;

#ifdef RWDEBUG
    RwUInt32    checkSize;
#endif /* RWDEBUG */

    RWAPIFUNCTION(RWSTRING("RpDMorphAnimationStreamWrite"));
    RWASSERT(animation);
    RWASSERT(animation->frameSequences);
    RWASSERT(stream);

    /* Size of internal chunk data */
    chunkSize = RpDMorphAnimationStreamGetSize(animation);

    /* Public chunk header & internal struct chunk header */
    if (    !RwStreamWriteChunkHeader(
                stream, rwID_DMORPHANIMATION, chunkSize)
        ||  !RwStreamWriteChunkHeader(
                stream, rwID_STRUCT, chunkSize - rwCHUNKHEADERSIZE))
    {
        RWRETURN((RpDMorphAnimation *)NULL);
    }

    /* Header info */
    version  = rpDMORPHANIMATIONSTREAMCURRVERSION;
    flags    = 0;   /* For future use */
    totalNumFrames = 0;
    for( i=0; i < animation->numDMorphTargets; i++ )
    {
        totalNumFrames += animation->frameSequences[i].numFrames;
    }

    if (    !RwStreamWriteInt(stream, (RwInt32 *) &version, sizeof(RwInt32))
        ||  !RwStreamWriteInt(stream, (RwInt32 *) &flags, sizeof(RwInt32))
        ||  !RwStreamWriteInt(stream,
                (RwInt32 *) &animation->numDMorphTargets, sizeof(RwInt32))
        ||  !RwStreamWriteInt(stream,
                (RwInt32 *) &totalNumFrames, sizeof(RwInt32)) )
    {
        RWRETURN((RpDMorphAnimation *)NULL);
    }

#ifdef RWDEBUG
    checkSize = rwCHUNKHEADERSIZE + 4*sizeof(RwInt32);
#endif /* RWDEBUG */

    /* Write out the frame sequences */
    for( i=0; i < animation->numDMorphTargets; i++ )
    {
        rpDMorphFrameSequence   *sequence;

        sequence = &animation->frameSequences[i];

        /* Number of frames in this sequence */
        if (!RwStreamWriteInt(stream,
                (RwInt32 *) &(sequence->numFrames), sizeof(RwInt32)))
        {
            RWRETURN((RpDMorphAnimation *)NULL);
        }

#ifdef RWDEBUG
        checkSize += sizeof(RwInt32);
#endif /* RWDEBUG */

        /* Frame data */
        if (sequence->numFrames > 0)
        {
            rpDMorphFrame   *frame = sequence->frames;
            RwUInt32        j;

            RWASSERT(frame);

            for( j=0; j < sequence->numFrames; j++ )
            {
                if (    !RwStreamWriteReal(stream,
                            &(frame->startValue), 4*sizeof(RwReal))
                    ||  !RwStreamWriteInt(stream,
                            (RwInt32 *) &(frame->nextFrame), sizeof(RwInt32)))
                {
                    RWRETURN((RpDMorphAnimation *)NULL);
                }

#ifdef RWDEBUG
                checkSize += 4*sizeof(RwReal) + sizeof(RwInt32);
#endif /* RWDEBUG */

                frame++;
            }
        }
    }

    RWASSERT(checkSize == chunkSize);

    RWRETURN(animation);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAnimationStreamGetSize obtains the size in bytes of the
 * binary representation of a delta morph animation. This is used in
 * the binary chunk header to indicate the size of the chunk. The size does
 * not include the size of the chunk header.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param animation     Pointer to delta morph animation.
 *
 * \return Size in bytes of the binary representation.
 *
 * \see RpDMorphAnimationStreamWrite
 * \see RpDMorphAnimationStreamRead
 */
RwUInt32
RpDMorphAnimationStreamGetSize(RpDMorphAnimation *animation)
{
    RwUInt32    i;
    RwUInt32    size;
    RwUInt32    totalNumFrames;

    RWAPIFUNCTION(RWSTRING("RpDMorphAnimationStreamGetSize"));
    RWASSERT(animation);
    RWASSERT(animation->frameSequences);

    /* Internal struct chunk header */
    size = rwCHUNKHEADERSIZE;

    /* Header: version, flags, numDMorphTarget, totalNumFrames */
    size += 4 * sizeof(RwInt32);

    /* numFrames for each sequence */
    size += animation->numDMorphTargets * sizeof(RwInt32);

    /* Get total number of animation frames */
    totalNumFrames = 0;
    for( i=0; i < animation->numDMorphTargets; i++)
    {
        totalNumFrames += animation->frameSequences[i].numFrames;
    }

    /* Total size of frame data */
    size += totalNumFrames * (4*sizeof(RwReal) + sizeof(RwInt32));

    RWRETURN(size);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAnimationRead will read a delta morph animation from
 * the named file. Internally, this opens a stream on the file, finds the
 * delta morph animation chunk header and then calls
 * \ref RpDMorphAnimationStreamRead before closing the stream
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param filename  A pointer to the name of the file containing the
 *                  animation.
 *
 * \return  A pointer to the animation, or NULL if there is an error.
 *
 * \see RpDMorphAnimationWrite
 * \see RpDMorphAnimationStreamRead
 */
RpDMorphAnimation *
RpDMorphAnimationRead(const RwChar *filename)
{
    RpDMorphAnimation   *animation = (RpDMorphAnimation *) NULL;
    RwStream            *stream;

    RWAPIFUNCTION(RWSTRING("RpDMorphAnimationRead"));
    RWASSERT(filename);

    stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, filename);

    if (stream)
    {
        if (RwStreamFindChunk(stream, rwID_DMORPHANIMATION,
                           (RwUInt32 *)NULL, (RwUInt32 *)NULL))
        {
            animation = RpDMorphAnimationStreamRead(stream);
        }

        RwStreamClose(stream, NULL);
    }

    RWRETURN(animation);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAnimationWrite writes a delta morph animation to the
 * named file. Internally, this opens a stream on the file, calls
 * \ref RpDMorphAnimationStreamWrite, and then closes the stream.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param animation     Pointer to the animation to be written.
 * \param filename      Pointer to the name of the file.
 *
 * \return Pointer to the delta morph animation, or NULL if there is
 * an error.
 *
 * \see RpDMorphAnimationRead
 */
RpDMorphAnimation *
RpDMorphAnimationWrite(RpDMorphAnimation *animation,
                       const RwChar *filename)
{
    RpDMorphAnimation   *rtnAnim = (RpDMorphAnimation *) NULL;
    RwStream            *stream;

    RWAPIFUNCTION(RWSTRING("RpDMorphAnimationWrite"));
    RWASSERT(animation);
    RWASSERT(filename);

    stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMWRITE, filename);

    if (stream)
    {
        rtnAnim = RpDMorphAnimationStreamWrite(animation, stream);

        RwStreamClose(stream, NULL);
    }

    RWRETURN(rtnAnim);
}

/*--- AnimationFrame functions --------------------------------------------*/

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAnimationFrameSet is used to
 * set all the values of the indexed frame for
 * the indexed morph target sequence in a delta morph animation.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param anim                  Pointer to the delta morph animation.
 * \param dMorphTargetIndex     Delta morph target index.
 * \param frameIndex            Index of the frame.
 * \param startValue            Start value to be set.
 * \param endValue              End value to be set.
 * \param duration              Duration to be set.
 * \param nextFrame             Index of the next frame.
 *
 * \return Returns NULL if there is an error, or otherwise
 *         a pointer to the animation.
 *
 * \see RpDMorphAnimationFrameSetNext
 * \see RpDMorphAnimationFrameGetNext
 * \see RpDMorphAnimationFrameSetStartValue
 * \see RpDMorphAnimationFrameGetStartValue
 * \see RpDMorphAnimationFrameSetEndValue
 * \see RpDMorphAnimationFrameGetEndValue
 * \see RpDMorphAnimationFrameSetDuration
 * \see RpDMorphAnimationFrameGetDuration
 * \see RpDMorphAnimationCreateFrames
 * \see RpDMorphAnimationGetNumFrames
 */
RpDMorphAnimation *
RpDMorphAnimationFrameSet(RpDMorphAnimation  *anim,
                          RwUInt32   dMorphTargetIndex,
                          RwUInt32   frameIndex,
                          RwReal     startValue,
                          RwReal     endValue,
                          RwReal     duration,
                          RwUInt32   nextFrame )
{
    rpDMorphFrameSequence    *sequence;
    rpDMorphFrame            *frame;

    RWAPIFUNCTION(RWSTRING("RpDMorphAnimationFrameSet"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(anim);
    RWASSERT(dMorphTargetIndex < anim->numDMorphTargets);
    RWASSERT(duration > 0.0f);

    /* Get sequence for this morph target */
    sequence = anim->frameSequences + dMorphTargetIndex;
    RWASSERT(sequence->frames);
    RWASSERT(frameIndex < sequence->numFrames);
    RWASSERT((nextFrame == rpDMORPHNULLFRAME) ||
             (nextFrame < sequence->numFrames));

    /* Get the frame */
    frame = sequence->frames + frameIndex;

    /* Setup the frame. */
    frame->startValue = startValue;
    frame->endValue = endValue;
    frame->duration = duration;
    frame->recipDuration = 1.0f / duration;
    frame->nextFrame = nextFrame;

    RWRETURN(anim);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAnimationFrameSetNext is used to
 * set the next frame for a given frame
 * in a morph target sequence of a delta morph animation.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param anim                  Pointer to the delta morph animation.
 * \param dMorphTargetIndex     Delta morph target index.
 * \param frameIndex            Index of the frame.
 * \param nextFrame             Index of the next frame.
 *
 * \return Returns NULL if there is an error, or otherwise
 *         a pointer to the animation.
 *
 * \see RpDMorphAnimationFrameGetNext
 */
RpDMorphAnimation *
RpDMorphAnimationFrameSetNext(RpDMorphAnimation *anim,
                                     RwUInt32   dMorphTargetIndex,
                                     RwUInt32   frameIndex,
                                     RwUInt32   nextFrame )
{
    rpDMorphFrameSequence    *sequence;

    RWAPIFUNCTION(RWSTRING("RpDMorphAnimationFrameSetNext"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(anim);
    RWASSERT(dMorphTargetIndex < anim->numDMorphTargets);

    /* Get sequence for this morph target */
    sequence = anim->frameSequences + dMorphTargetIndex;
    RWASSERT(sequence->frames);
    RWASSERT(frameIndex < sequence->numFrames);
    RWASSERT((nextFrame == rpDMORPHNULLFRAME) ||
             (nextFrame < sequence->numFrames));

    /* Set the frames next */
    sequence->frames[frameIndex].nextFrame = nextFrame;

    RWRETURN(anim);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAnimationFrameGetNext is used to
 * get the index of the next frame for a given
 * frame in a morph target sequence of a delta morph animation.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param anim                  Pointer to the delta morph animation.
 * \param dMorphTargetIndex     Delta morph target index.
 * \param frameIndex            Index of the frame.
 *
 * \return Returns the index of the next frame.
 *
 * \see RpDMorphAnimationFrameSetNext
 */
RwUInt32
RpDMorphAnimationFrameGetNext(RpDMorphAnimation *anim,
                              RwUInt32          dMorphTargetIndex,
                              RwUInt32          frameIndex )
{
    rpDMorphFrameSequence    *sequence;

    RWAPIFUNCTION(RWSTRING("RpDMorphAnimationFrameGetNext"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(anim);
    RWASSERT(dMorphTargetIndex < anim->numDMorphTargets);

    /* Get sequence for this morph target */
    sequence = anim->frameSequences + dMorphTargetIndex;
    RWASSERT(sequence->frames);
    RWASSERT(frameIndex < sequence->numFrames);

    /* Offer up the frames duration. */
    RWRETURN(sequence->frames[frameIndex].nextFrame);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAnimationFrameSetStartValue is used to
 * set the start value of the indexed frame for the
 * indexed morph target sequence in a delta morph animation.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param anim                  Pointer to the delta morph animation.
 * \param dMorphTargetIndex     Delta morph target index.
 * \param frameIndex            Index of the frame.
 * \param startValue            Start value to be set.
 *
 * \return Returns NULL if there is an error, or otherwise
 *         a pointer to the animation.
 *
 * \see RpDMorphAnimationFrameSet
 * \see RpDMorphAnimationFrameGetStartValue
 */
RpDMorphAnimation *
RpDMorphAnimationFrameSetStartValue(RpDMorphAnimation *anim,
                                    RwUInt32   dMorphTargetIndex,
                                    RwUInt32   frameIndex,
                                    RwReal     startValue )
{
    rpDMorphFrameSequence *sequence;

    RWAPIFUNCTION(RWSTRING("RpDMorphAnimationFrameSetStartValue"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(anim);
    RWASSERT(dMorphTargetIndex < anim->numDMorphTargets);

    /* Get sequence for this morph target */
    sequence = anim->frameSequences + dMorphTargetIndex;
    RWASSERT(sequence->frames);
    RWASSERT(frameIndex < sequence->numFrames);

    /* Set the frames duration. */
    sequence->frames[frameIndex].startValue = startValue;

    RWRETURN(anim);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAnimationFrameGetStartValue is used to
 * get the start value of the indexed frame for the
 * indexed morph target sequence in a delta morph animation.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param anim                  Pointer to the delta morph animation.
 * \param dMorphTargetIndex     Delta morph target index.
 * \param frameIndex            Index of the frame.
 *
 * \return Returns the start value of the frame.
 *
 * \see RpDMorphAnimationFrameSetStartValue
 */
RwReal
RpDMorphAnimationFrameGetStartValue(RpDMorphAnimation *anim,
                                    RwUInt32 dMorphTargetIndex,
                                    RwUInt32 frameIndex )
{
    rpDMorphFrameSequence    *sequence;

    RWAPIFUNCTION(RWSTRING("RpDMorphAnimationFrameGetStartValue"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(anim);
    RWASSERT(dMorphTargetIndex < anim->numDMorphTargets);

    /* Get sequence for this morph target */
    sequence = anim->frameSequences + dMorphTargetIndex;
    RWASSERT(sequence->frames);
    RWASSERT(frameIndex < sequence->numFrames);

    /* Offer up the frames duration. */
    RWRETURN(sequence->frames[frameIndex].startValue);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAnimationFrameSetEndValue is used to
 * set the end value of the indexed frame for the
 * indexed morph target sequence in a delta morph animation.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param anim                  Pointer to the delta morph animation.
 * \param dMorphTargetIndex     Delta morph target index.
 * \param frameIndex            Index of the frame.
 * \param endValue              End value to be set.
 *
 * \return Returns NULL if there is an error, or otherwise
 *         a pointer to the animation.
 *
 * \see RpDMorphAnimationFrameSet
 * \see RpDMorphAnimationFrameGetEndValue
 */
RpDMorphAnimation *
RpDMorphAnimationFrameSetEndValue(RpDMorphAnimation *anim,
                                  RwUInt32   dMorphTargetIndex,
                                  RwUInt32   frameIndex,
                                  RwReal     endValue )
{
    rpDMorphFrameSequence *sequence;

    RWAPIFUNCTION(RWSTRING("RpDMorphAnimationFrameSetEndValue"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(anim);
    RWASSERT(dMorphTargetIndex < anim->numDMorphTargets);

    /* Get sequence for this morph target */
    sequence = anim->frameSequences + dMorphTargetIndex;
    RWASSERT(sequence->frames);
    RWASSERT(frameIndex < sequence->numFrames);

    /* Set the frames duration. */
    sequence->frames[frameIndex].endValue = endValue;

    RWRETURN(anim);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAnimationFrameGetEndValue is used to
 * get the end value of the indexed frame for the
 * indexed morph target sequence in a delta morph animation.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param anim                  Pointer to the delta morph animation.
 * \param dMorphTargetIndex     Delta morph target index.
 * \param frameIndex            Index of the frame.
 *
 * \return Returns the end value of the frame.
 *
 * \see RpDMorphAnimationFrameSetEndValue
 */
RwReal
RpDMorphAnimationFrameGetEndValue(RpDMorphAnimation *anim,
                                  RwUInt32 dMorphTargetIndex,
                                  RwUInt32 frameIndex )
{
    rpDMorphFrameSequence    *sequence;

    RWAPIFUNCTION(RWSTRING("RpDMorphAnimationFrameGetEndValue"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(anim);
    RWASSERT(dMorphTargetIndex < anim->numDMorphTargets);

    /* Get sequence for this morph target */
    sequence = anim->frameSequences + dMorphTargetIndex;
    RWASSERT(sequence->frames);
    RWASSERT(frameIndex < sequence->numFrames);

    /* Offer up the frames duration. */
    RWRETURN(sequence->frames[frameIndex].endValue);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAnimationFrameSetDuration is used to
 * set the duration of the indexed frame for the
 * indexed morph target sequence in a delta morph animation.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param anim                  Pointer to the delta morph animation.
 * \param dMorphTargetIndex     Delta morph target index.
 * \param frameIndex            Index of the frame.
 * \param duration              Duration to be set.
 *
 * \return Returns NULL if there is an error, or otherwise
 *         a pointer to the animation.
 *
 * \see RpDMorphAnimationFrameSet
 * \see RpDMorphAnimationFrameGetDuration
 */
RpDMorphAnimation *
RpDMorphAnimationFrameSetDuration(RpDMorphAnimation *anim,
                                  RwUInt32   dMorphTargetIndex,
                                  RwUInt32   frameIndex,
                                  RwReal     duration )
{
    rpDMorphFrameSequence *sequence;

    RWAPIFUNCTION(RWSTRING("RpDMorphAnimationFrameSetDuration"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(anim);
    RWASSERT(dMorphTargetIndex < anim->numDMorphTargets);
    RWASSERT(duration > 0.0f);

    /* Get sequence for this morph target */
    sequence = anim->frameSequences + dMorphTargetIndex;
    RWASSERT(sequence->frames);
    RWASSERT(frameIndex < sequence->numFrames);

    /* Set the frames duration. */
    sequence->frames[frameIndex].duration = duration;
    sequence->frames[frameIndex].recipDuration = 1.0f / duration;

    RWRETURN(anim);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAnimationFrameGetDuration is used to
 * get the duration of the indexed frame for the
 * indexed morph target sequence in a delta morph animation.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param anim                  Pointer to the delta morph animation.
 * \param dMorphTargetIndex     Delta morph target index.
 * \param frameIndex            Index of the frame.
 *
 * \return Returns the duration of the frame.
 *
 * \see RpDMorphAnimationFrameSetDuration
 */
RwReal
RpDMorphAnimationFrameGetDuration(RpDMorphAnimation *anim,
                                  RwUInt32 dMorphTargetIndex,
                                  RwUInt32 frameIndex)
{
    rpDMorphFrameSequence    *sequence;

    RWAPIFUNCTION(RWSTRING("RpDMorphAnimationFrameGetDuration"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(anim);
    RWASSERT(dMorphTargetIndex < anim->numDMorphTargets);

    /* Get sequence for this morph target */
    sequence = anim->frameSequences + dMorphTargetIndex;
    RWASSERT(sequence->frames);
    RWASSERT(frameIndex < sequence->numFrames);

    /* Offer up the frames duration. */
    RWRETURN(sequence->frames[frameIndex].duration);
}
