/**********************************************************************
 *
 * File : rtanim.c
 *
 * Abstract : A toolkit for keyframed animation system
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
 * Copyright (c) 2002 Criterion Software Ltd.
 * All Rights Reserved.
 *
 * RenderWare is a trademark of Canon Inc.
 *
 ************************************************************************/

/**
 * \ingroup rtanim
 * \page rtanimoverview RtAnim Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rtanim.h
 * \li \b Libraries: rwcore, rtanim
 * \li \b Plugin \b attachments: \ref RtAnimInitialize
 *
 * \subsection animoverview Overview
 *
 * \ref rtanim is an animation toolkit, enabling the user to create, stream and play
 * any type of keyframed animation.
 *
 * \ref RtAnimAnimation's are a block of keyframe data representing an
 * animation. For each node (i.e. joint) in the object that the animation is
 * applied to, the .ANM file contains a \e start keyframe, an \e end keyframe, and any
 * number of \e intermediate keyframes as necessary. This is normally created by
 * loading an .ANM file. When a node is at a time where no keyframe is applied,
 * the actual state of that node will be positioned by linear interpolation
 * between the two adjacent keyframes.
 *
 * \ref rtanim supports the concept of overloadable interpolation schemes. This
 * requires specifying a custom keyframe type (identified by a unique ID) and a
 * set of callbacks for manipulating the keyframes. The callback functions will
 * implement tasks such as interpolation and converting to final result. Animations
 * are associated with a particular scheme using the ID, and the relevant
 * callbacks are automatically used whenever an animation is updated by the
 * standard \ref rtanim API interface. To support this, the keyframe types must share
 * a common header giving a pointer to the previous keyframe, and the time of
 * the keyframe. See \ref RtAnimRegisterInterpolationScheme for more
 * information.
 *
**/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "rpplugin.h"
#include <rpdbgerr.h>
#include <rwcore.h>
#include <rpworld.h>

#include <rtanim.h>

#define RTANIMINTERPOLATORINFOBLOCKMAXENTRIES 16

RwFreeList *RtAnimAnimationFreeList, RtAnimAnimationFreeListSpace;

RwInt32 RtAnimInterpolatorInfoBlockNumEntries = 0;

RtAnimInterpolatorInfo
    RtAnimInterpolatorInfoBlock[RTANIMINTERPOLATORINFOBLOCKMAXENTRIES];


static RwInt32 _rtAnimAnimationFreeListBlockSize = 128,
               _rtAnimAnimationFreeListPreallocBlocks = 1;

/******************************************************************************
 */

/* Debugging macro */
#define AnimKeyFramePtrValid(_anim, _keyFrame)                          \
    (((void*)(_keyFrame) >= (_anim)->pFrames) && \
    ((void*)(_keyFrame) < (void*)((RwUInt8*)(_anim)->pFrames  \
        + (_anim)->numFrames*(_anim)->interpInfo->animKeyFrameSize)))


/**
 * \ingroup rtanim
 * \ref RtAnimAnimationFreeListCreateParams allows the developer to specify
 * how many \ref RtAnimAnimation's to preallocate space for.
 * Call before \ref RwEngineInit.
 *
 * \param blockSize  \ref RwInt32 containing the number of entries per freelist block.
 * \param numBlocksToPrealloc  \ref RwInt32 containing the number of blocks to
 *                             allocate on \ref RwFreeListCreateAndPreallocateSpace.
 *
 * \see RwFreeList
 *
 */
void
RtAnimAnimationFreeListCreateParams(RwInt32 blockSize,
                                    RwInt32 numBlocksToPrealloc)
{
#if 0
    /*
     * Can not use debugging macros since the debugger is not initialized before
     * RwEngineInit.
     */
    /* Do NOT comment out RWAPIFUNCTION as gnumake verify will not function */
    RWAPIFUNCTION(RWSTRING("RtAnimAnimationFreeListCreateParams"));
#endif
    _rtAnimAnimationFreeListBlockSize = blockSize;
    _rtAnimAnimationFreeListPreallocBlocks = numBlocksToPrealloc;
#if 0
    RWRETURNVOID();
#endif
}


static void        *
AnimOpen(void *instance,
          RwInt32 __RWUNUSED__ offset,
          RwInt32 __RWUNUSED__ size)
{
    RWFUNCTION(RWSTRING("AnimOpen"));
    RWASSERT(instance);

    {
        RtAnimAnimationFreeList =
            RwFreeListCreateAndPreallocateSpace(sizeof(RtAnimAnimation),
            _rtAnimAnimationFreeListBlockSize, sizeof(RwInt32),
            _rtAnimAnimationFreeListPreallocBlocks,
            &RtAnimAnimationFreeListSpace,
            rwID_HANIMANIMATION | rwMEMHINTDUR_GLOBAL);

        if (NULL == RtAnimAnimationFreeList)
        {
            instance = NULL;
        }
    }

    RWRETURN(instance);

}

static void        *
AnimClose(void *instance,
           RwInt32 __RWUNUSED__ offset,
           RwInt32 __RWUNUSED__ size)
{
    RWFUNCTION(RWSTRING("AnimClose"));
    RWASSERT(instance);

    /* "Deregister" all the interpolation schemes */
    RtAnimInterpolatorInfoBlockNumEntries = 0;

    if (NULL != RtAnimAnimationFreeList)
    {
        RwFreeListDestroy(RtAnimAnimationFreeList);
        RtAnimAnimationFreeList = (RwFreeList *)NULL;
    }

    RWRETURN(instance);
}


/**
 * \ingroup rtanim
 * \ref RtAnimInitialize is used to initialize the animation toolkit with the
 * RenderWare Graphics system to enable the manipulation of keyframed animation.
 * The toolkit must be initialized between initializing the system
 * with \ref RwEngineInit and opening it with \ref RwEngineOpen.
 *
 * \return \ref RwBool, TRUE if successful, FALSE if an error occurs.
 *
 * \see RwEngineInit
 * \see RwEngineOpen
 */
RwBool
RtAnimInitialize(void)
{
    RwBool result;
    RWAPIFUNCTION(RWSTRING("RtAnimInitialize"));

    /* Register the plugIn */

    result = (RwEngineRegisterPlugin(0,
                               rwID_ANIMTOOLKIT,
                               AnimOpen, AnimClose) > 0);

    RWRETURN(result);
}


/**
 * \ingroup rtanim
 * \ref RtAnimRegisterInterpolationScheme registers a keyframe interpolation
 * scheme. The function must be called before any animations using the scheme
 * are created or read from a stream.
 *
 * An \ref RtAnimInterpolatorInfo structure should be filled with the type ID
 * of the scheme, the size of the keyframe structure and a list of
 * callbacks for processing keyframes, before being passed to this function.
 *
 * The type ID must be unique and it is recommended that values in the
 * range 1 to 255 are reserved for internal use by RenderWare Graphics. It is
 * suggested that developers construct unique IDs using
 * MAKECHUNKID(vendorID, typeID).
 * An animation using the scheme may be created by passing in the type ID.
 *
 * The keyframe structure should always begin with a standard header
 * (\ref RtAnimKeyFrameHeader), i.e. the first 4 bytes hold a pointer to the
 * previous keyframe for the particular node, and the second 4
 * bytes hold the time of the keyframe. This allows the keyframe type to
 * function with \ref RtAnimInterpolatorAddAnimTime etc. The header size
 * should be included in the \e animKeyFrameSize entry. For an interpolated
 * keyframe, a different header is used (\ref RtAnimInterpFrameHeader)
 * which has the same size and should be included in the \e interpKeyFrameSize
 * entry.
 *
 * See \ref RtAnimInterpolatorInfo for information about the callbacks.
 *
 * \param interpInfo    Pointer to the \ref RtAnimInterpolatorInfo structure
 *                      containing the interpolator information. This information
 *                      will be copied to an internal data block.
 *
 * \return \ref RwBool, TRUE if successful, or FALSE if an error occurred.
 *
 * \see RtAnimGetInterpolatorInfo
 */
RwBool
RtAnimRegisterInterpolationScheme(RtAnimInterpolatorInfo *interpInfo)
{
    RWAPIFUNCTION(RWSTRING("RtAnimRegisterInterpolationScheme"));
    RWASSERT(interpInfo);
    RWASSERT(interpInfo->animKeyFrameSize >= 0);
    RWASSERT(interpInfo->interpKeyFrameSize >= 0);

    if (RtAnimInterpolatorInfoBlockNumEntries <
            RTANIMINTERPOLATORINFOBLOCKMAXENTRIES)
    {
        RwInt32    i;

        /* Check that the ID isn't already in use */
        for (i=0; i<RtAnimInterpolatorInfoBlockNumEntries; i++)
        {
            if (RtAnimInterpolatorInfoBlock[i].typeID == interpInfo->typeID)
            {
                RWERROR((E_RT_ANIM_INTERP_IDINUSE));
                RWRETURN(FALSE);
            }
        }

        RtAnimInterpolatorInfoBlock[RtAnimInterpolatorInfoBlockNumEntries]
            = *interpInfo;
        RtAnimInterpolatorInfoBlockNumEntries++;

        RWRETURN(TRUE);
    }

    RWERROR((E_RT_ANIM_INTERP_BLOCKFULL));
    RWRETURN(FALSE);
}


/**
 * \ingroup rtanim
 * \ref RtAnimGetInterpolatorInfo retrieves the information for the
 * interpolation scheme of the given type ID. The scheme must first have been
 * registered using \ref RtAnimRegisterInterpolationScheme, unless it is
 * provided as standard. The \ref rphanim plugin provides a standard animation
 * scheme.
 *
 * \param typeID    \ref RwInt32 containing the type ID of the interpolation scheme.
 *
 * \return Pointer to the interpolator information \ref RtAnimInterpolatorInfo,
 *         or NULL if no scheme is registered under the given ID.
 *
 * \see RtAnimRegisterInterpolationScheme
 */
RtAnimInterpolatorInfo *
RtAnimGetInterpolatorInfo(RwInt32 typeID)
{
    RwInt32     i;

    RWAPIFUNCTION(RWSTRING("RtAnimGetInterpolatorInfo"));

    /* Lookup the typeID in the InterpolatorInfoBlock */
    for (i=0; i<RtAnimInterpolatorInfoBlockNumEntries; i++)
    {
        if (RtAnimInterpolatorInfoBlock[i].typeID == typeID)
        {
            RWRETURN(&RtAnimInterpolatorInfoBlock[i]);
        }
    }

    /* Unregistered animation type */
    RWERROR((E_RT_ANIM_INTERP_IDUNKNOWN));
    RWRETURN((RtAnimInterpolatorInfo *)NULL);
}


/**
 * \ingroup rtanim
 * \ref RtAnimAnimationCreate creates an animation, and allocates space for the
 * number of keyframes required. This keyframe block should be filled with
 * keyframe sequences.
 * See \ref RtAnimAnimation for details of the required layout of the keyframes.
 *
 * \param typeID    \ref RwInt32 containing the ID number specifying the
 *                  interpolation scheme to be used with this animation,
 *                  that also determines the keyframe size.
 * \param numFrames \ref RwInt32 containing the number of keyframes in the animation.
 * \param flags     \ref RwInt32 reserved for future use - should be 0.
 * \param duration  \ref RwReal containing the total length of the animation.
 *
 * \return Pointer to the created \ref RtAnimAnimationCreate, or NULL if an error occurs.
 */

RtAnimAnimation         *
RtAnimAnimationCreate(RwInt32 typeID,
                       RwInt32 numFrames,
                       RwInt32 flags,
                       RwReal duration)
{
    RtAnimAnimation           *pAnimation;
    RtAnimInterpolatorInfo    *interpInfo;

    RWAPIFUNCTION(RWSTRING("RtAnimAnimationCreate"));
    RWASSERT(numFrames > 0);

    interpInfo = RtAnimGetInterpolatorInfo(typeID);
    if (!interpInfo)
    {
        /* Unregistered animation type */
        RWRETURN((RtAnimAnimation *)NULL);
    }

    pAnimation = (RtAnimAnimation *) RwMalloc(sizeof(RtAnimAnimation) +
                                    numFrames * interpInfo->animKeyFrameSize +
                                    interpInfo->customDataSize,
                                    rwID_HANIMANIMATION | rwMEMHINTDUR_EVENT);


    pAnimation->numFrames = numFrames;
    pAnimation->duration = duration;
    pAnimation->flags = flags;
    pAnimation->interpInfo = interpInfo;
    pAnimation->pFrames = (void *)(pAnimation+1);
    if (interpInfo->customDataSize > 0)
    {
        pAnimation->customData = (void *)(((RwUInt8 *)pAnimation->pFrames) +
                                    numFrames * interpInfo->animKeyFrameSize);
    }
    else
    {
        pAnimation->customData = NULL;
    }

    RWRETURN(pAnimation);
}


/**
 * \ingroup rtanim
 * \ref RtAnimAnimationDestroy destroys an animation.
 *
 * \param animation Pointer to the \ref RtAnimAnimation to be destroyed.
 *
 * \return Pointer to \ref RtAnimAnimation, or NULL if an error occurs.
 *
 * \see RtAnimAnimationCreate
 */

RwBool
RtAnimAnimationDestroy(RtAnimAnimation *animation)
{
    RWAPIFUNCTION(RWSTRING("RtAnimAnimationDestroy"));
    RWASSERT(animation);

    RwFree(animation);

    RWRETURN(TRUE);
}


/**
 * \ingroup rtanim
 * \ref RtAnimAnimationRead is a wrapper around \ref RtAnimAnimationStreamRead.
 * Read opens the stream and searches for the appropriate chunk header and
 * then calls \ref RtAnimAnimationStreamRead.
 *
 * Reads a .ANM animation file from disk.
 *
 * \param filename Pointer to a constant \ref RwChar string containing
 *                 the name of the file to be read.
 *
 * \return Pointer to the \ref RtAnimAnimation, or NULL if an error occurs.
 *
 */

RtAnimAnimation         *
RtAnimAnimationRead(const RwChar *filename)
{
    RtAnimAnimation         *pAnimation;
    RwStream           *pStream;

    RWAPIFUNCTION(RWSTRING("RtAnimAnimationRead"));
    RWASSERT(filename);

    pStream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, filename);

    if (!pStream)
    {
        RWRETURN((RtAnimAnimation *)NULL);
    }

    if (!RwStreamFindChunk(pStream, rwID_ANIMANIMATION,
                           (RwUInt32 *)NULL, (RwUInt32 *)NULL))
    {
        RWRETURN((RtAnimAnimation *)NULL);
    }

    pAnimation = RtAnimAnimationStreamRead(pStream);

    RwStreamClose(pStream, NULL);

    RWRETURN(pAnimation);
}


/**
 * \ingroup rtanim
 * \ref RtAnimAnimationWrite is a wrapper around
 * \ref RtAnimAnimationStreamWrite. Write will open a stream and call
 * \ref RtAnimAnimationStreamWrite.
 *
 * Writes a .ANM animation file to disk.
 *
 * \param animation A pointer to the \ref RtAnimAnimation to be written.
 * \param filename A pointer to the name of the file to be written to.
 *
 * \return \ref RwBool, TRUE on success, or FALSE if an error occurs.
 *
 */

RwBool
RtAnimAnimationWrite(const RtAnimAnimation *animation, const RwChar *filename)
{
    RwStream           *pStream;

    RWAPIFUNCTION(RWSTRING("RtAnimAnimationWrite"));
    RWASSERT(animation);
    RWASSERT(filename);

    pStream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMWRITE, filename);

    if (!pStream)
    {
        RWRETURN(FALSE);
    }

    RtAnimAnimationStreamWrite(animation, pStream);

    RwStreamClose(pStream, NULL);

    RWRETURN(TRUE);
}

/**
 *
 * \ingroup rtanim
 * \ref RtAnimAnimationStreamRead creates and reads in animation data.
 * Upon reading, it will look up pointers to interpolation functions based on
 * the interpolation scheme ID which was written out to the file.
 * If the interpolation scheme is not available then the read will fail.
 *
 * This function calls the overloaded
 * \ref RtAnimKeyFrameStreamReadCallBack function for the particular keyframe
 * type allowing keyframes to be of varying sizes/data types etc.
 * This not only allows interpolation schemes to perform higher order
 * interpolation but allows them to attach extra data to achieve it or compress
 * data in order to save memory space.
 *
 * The sequence to locate and read animation data from a binary stream is
 * as follows:
 * \code
   RwStream *stream;
   RtAnimAnimation *newAnimAnimation;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_ANIMANIMATION, NULL, NULL) )
       {
           newAnimAnimation = RtAnimAnimationStreamRead(stream);
       }

       RwStreamClose(stream, NULL);
   }
   \endcode
 *
 * \param stream A pointer to the \ref RwStream to be read from.
 *
 * \return A pointer to the \ref RtAnimAnimation, or NULL if an error occurs.
 *
 * \see RtAnimAnimationStreamGetSize
 * \see RtAnimAnimationStreamWrite
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RwStreamFindChunk
 * \see RtAnimInitialize
 */

RtAnimAnimation         *
RtAnimAnimationStreamRead(RwStream *stream)
{
    RwInt32                     streamVersion;
    RwInt32                     typeID;
    RtAnimInterpolatorInfo    *interpInfo;
    RtAnimAnimation           *pAnimation;
    RwInt32                     numFrames;
    RwInt32                     flags;
    RwReal                      duration;
#ifdef RWDEBUG
    void                      *pFramesCheck;
#endif

    RWAPIFUNCTION(RWSTRING("RtAnimAnimationStreamRead"));
    RWASSERT(stream);

    if (!RwStreamReadInt
        (stream, (RwInt32 *) & (streamVersion), sizeof(RwInt32)))
    {
        RWRETURN((RtAnimAnimation *)NULL);
    }

    if (streamVersion != 0x100)
    {
        RWRETURN((RtAnimAnimation *)NULL);
    }

    if (!RwStreamReadInt
        (stream, (RwInt32 *) & (typeID), sizeof(RwInt32)))
    {
        RWRETURN((RtAnimAnimation *)NULL);
    }

    interpInfo = RtAnimGetInterpolatorInfo(typeID);
    if (!interpInfo)
    {
        RWRETURN((RtAnimAnimation *)NULL);
    }

    if (!RwStreamReadInt
        (stream, (RwInt32 *) & (numFrames), sizeof(RwInt32)))
    {
        RWRETURN((RtAnimAnimation *)NULL);
    }

    if (!RwStreamReadInt
        (stream, (RwInt32 *) & (flags), sizeof(RwInt32)))
    {
        RWRETURN((RtAnimAnimation *)NULL);
    }

    if (!RwStreamReadReal
        (stream, (RwReal *) & (duration), sizeof(RwReal)))
    {
        RWRETURN((RtAnimAnimation *)NULL);
    }

    /* allocate the animation */
    pAnimation = RtAnimAnimationCreate(typeID, numFrames, flags, duration);
    if (!pAnimation)
    {
        RWRETURN((RtAnimAnimation *)NULL);
    }

#ifdef RWDEBUG
    pFramesCheck = pAnimation->pFrames;
#endif

    RWASSERT(interpInfo->keyFrameStreamReadCB);
    interpInfo->keyFrameStreamReadCB(stream, pAnimation);

    RWASSERTM(pAnimation->pFrames == pFramesCheck,
        (RWSTRING("This function has allocated memory for keyframes, but ")
         RWSTRING("this appears to have been done a second time in the ")
         RWSTRING("keyFrameStreamReadCB.")));

    RWRETURN(pAnimation);
}

/**
 * \ingroup rtanim
 * \ref RtAnimAnimationStreamWrite writes an animation to a stream. This function
 * calls the overloaded \ref RtAnimKeyFrameStreamWriteCallBack function for the
 * particular keyframe type allowing keyframes to be of varying sizes/data types etc.
 * This not only allows interpolation schemes to perform higher order
 * interpolation but allows them to attach extra data to achieve it or compress
 * data in order to save memory space.
 *
 * \param animation A pointer to the \ref RtAnimAnimation to be written.
 * \param stream A pointer to the \ref RwStream to be written to.
 *
 * \return \ref RwBool, TRUE on success, or FALSE if an error occurs.
 *
 */

RwBool
RtAnimAnimationStreamWrite(const RtAnimAnimation *animation,
                            RwStream *stream)
{
    RwInt32 streamVersion = rtANIMSTREAMCURRENTVERSION;

    RWAPIFUNCTION(RWSTRING("RtAnimAnimationStreamWrite"));
    RWASSERT(animation);
    RWASSERT(stream);

    RwStreamWriteChunkHeader(stream, rwID_ANIMANIMATION,
                             RtAnimAnimationStreamGetSize(animation));

    if (!RwStreamWriteInt
        (stream, (RwInt32 *) & (streamVersion), sizeof(RwInt32)))
    {
        RWRETURN(FALSE);
    }

    if (!RwStreamWriteInt
        (stream, (RwInt32 *) & (animation->interpInfo->typeID), sizeof(RwInt32)))
    {
        RWRETURN(FALSE);
    }

    if (!RwStreamWriteInt
        (stream, (const RwInt32 *) & (animation->numFrames), sizeof(RwInt32)))
    {
        RWRETURN(FALSE);
    }

    if (!RwStreamWriteInt
        (stream, (const RwInt32 *) & (animation->flags), sizeof(RwInt32)))
    {
        RWRETURN(FALSE);
    }

    if (!RwStreamWriteReal
        (stream, (const RwReal *) & (animation->duration), sizeof(RwReal)))
    {
        RWRETURN(FALSE);
    }

    RWASSERT(animation->interpInfo->keyFrameStreamWriteCB);
    animation->interpInfo->keyFrameStreamWriteCB(animation, stream);

    RWRETURN(TRUE);
}

/**
 * \ingroup rtanim
 * \ref RtAnimAnimationStreamGetSize is used in stream writing and will
 * call the overloaded interpolator \ref RtAnimKeyFrameStreamGetSizeCallBack
 * function in order to acquire the size of the keyframe data.
 *
 * \param animation A pointer to the \ref RtAnimAnimation to determine the
 *                  size of its keyframe data.
 *
 * \return \ref RwInt32 containing the size of the hierarchical animation in bytes.
 *
 */
RwInt32
RtAnimAnimationStreamGetSize(const RtAnimAnimation *animation)
{
    RwInt32             size = 0;

    RWAPIFUNCTION(RWSTRING("RtAnimAnimationStreamGetSize"));

    RWASSERT(animation);

    size += sizeof(RwInt32);    /* stream version */
    size += sizeof(RwInt32);    /* animation type id */
    size += sizeof(RwInt32);    /* numframes */
    size += sizeof(RwInt32);    /* flags */
    size += sizeof(RwReal);     /* duration */

    /* and add size of the keyframes */
    RWASSERT(animation->interpInfo->keyFrameStreamGetSizeCB);
    size += animation->interpInfo->keyFrameStreamGetSizeCB(animation);

    RWRETURN(size);
}

/**
 * \ingroup rtanim
 * \ref RtAnimAnimationGetNumNodes returns the number of nodes
 * supported by the animation, that is, the number of nodes required for
 * an interpolator to read the animation.
 *
 * \param animation A pointer to the \ref RtAnimAnimation.
 *
 * \return The number of nodes
 */
RwUInt32
RtAnimAnimationGetNumNodes(const RtAnimAnimation *animation)
{
    RwUInt32                numNodes;
    RtAnimKeyFrameHeader   *kf;
    RwUInt32                kfSize;

    RWAPIFUNCTION(RWSTRING("RtAnimAnimationGetNumNodes"));
    RWASSERT(animation);
    RWASSERT(animation->pFrames);
    RWASSERT(animation->numFrames > 0);

    /* Search through initial keyframes, of which there should be one
     * for each node, until we get to the second keyframe for node 0
     * which we recognize since it refers back to the first keyframe.
     */
    numNodes = 0;
    kfSize = animation->interpInfo->animKeyFrameSize;
    kf = (RtAnimKeyFrameHeader *)animation->pFrames;

    while (kf->prevFrame != animation->pFrames)
    {
        numNodes++;
        kf = (RtAnimKeyFrameHeader *)((RwInt8 *)kf + kfSize);

        RWASSERTM((RwUInt8*)kf <
            ((RwUInt8*)animation->pFrames + animation->numFrames*kfSize),
            (RWSTRING("Invalid prevFrame pointers in animation.")));
    }

    RWASSERT(numNodes > 0);

    RWRETURN(numNodes);
}

#if defined(RWDEBUG)
/**
 * \ingroup rtanim
 * \ref RtAnimAnimationGetTypeID
 *
 * Returns the typeID of an animation
 *
 * \param animation A pointer to the \ref RtAnimAnimation.
 *
 * \return \ref RwInt32 containing the typeID of the animation passed in.
 *
 */
RwInt32
RtAnimAnimationGetTypeID(RtAnimAnimation *animation)
{
    RwInt32 typeID;

    RWAPIFUNCTION(RWSTRING("RtAnimAnimationGetTypeID"));
    RWASSERT(animation);

    typeID = animation->interpInfo->typeID;

    RWRETURN(typeID);
}

#endif /* (defined(RWDEBUG)) */

/**
 * \ingroup rtanim
 * \ref RtAnimInterpolatorCreate creates an instanced version of an
 * animation. Each \ref RtAnimAnimation object contains a full sequence of
 * keyframes to be interpolated as time advances. The \ref RtAnimInterpolator
 * object contains the current state of an animation at a particular point in
 * time. Any number of interpolators may be connected to a single \ref
 * RtAnimAnimation.
 *
 * Internally, the interpolator stores a table of interpolated keyframes.
 * When calling
 * \ref RtAnimInterpolatorAddAnimTime,
 * \ref RtAnimInterpolatorSubAnimTime, or
 * \ref RtAnimInterpolatorSetCurrentTime
 * the contents of the interpolated keyframe table is updated using the
 * callbacks registered in the corresponding \ref RtAnimInterpolatorInfo for
 * the animation.
 *
 * The interpolator also manages user callbacks that may be triggered when
 * the animation reaches a particular time, or loops back to the start.
 *
 * For advanced purposes such as procedural animation, the table of
 * interpolated frames can be accessed using the macro
 * rtANIMGETINTERPFRAME(anim, nodeIndex),
 * where \e anim is a pointer to an \ref RtAnimInterpolator, and
 * \e nodeIndex is the index of the  keyframe/node. Note, however,
 * that each interpolated frame has an \ref RtAnimInterpFrameHeader
 * rather than an \ref RtAnimKeyFrameHeader and this header data
 * <i> should not be modified </i>.
 *
 * \param numNodes \ref RwInt32 containing the number of nodes.
 *
 * \param maxInterpKeyFrameSize \ref RwInt32 containing the maximum size of an
 * interpolated keyframe.
 *
 * \return Pointer to the new \ref RtAnimInterpolator on success, NULL on
 * failure.
 *
 * \see RtAnimAnimationCreate
 * \see RtAnimAnimationDestroy
 * \see RtAnimInterpolatorCreate
 * \see RtAnimInterpolatorCreateSubInterpolator
 */
RtAnimInterpolator *
RtAnimInterpolatorCreate(RwInt32 numNodes,
                         RwInt32 maxInterpKeyFrameSize)
{
    void               *ptr;
    RtAnimInterpolator *anim;

    RWAPIFUNCTION(RWSTRING("RtAnimInterpolatorCreate"));
    RWASSERT(numNodes >= 1);

    ptr = RwMalloc(sizeof(RtAnimInterpolator) +
                   (numNodes * maxInterpKeyFrameSize),
                   rwID_HANIMANIMATION | rwMEMHINTDUR_EVENT);

    anim = (RtAnimInterpolator *) ptr;

    anim->numNodes = numNodes;

    anim->pCurrentAnim = (RtAnimAnimation *)NULL;
    anim->pNextFrame = NULL;
    anim->currentTime = 0.0f;

    anim->pAnimCallBack = (RtAnimInterpolator * (*)(RtAnimInterpolator *, void *))NULL;
    anim->animCallBackTime = (RwReal) - 1.0;
    anim->pAnimCallBackData = NULL;
    anim->pAnimLoopCallBack = (RtAnimInterpolator * (*)(RtAnimInterpolator *, void *))NULL;
    anim->pAnimLoopCallBackData = NULL;

    anim->currentInterpKeyFrameSize = maxInterpKeyFrameSize;
    anim->currentAnimKeyFrameSize = -1;
    anim->maxInterpKeyFrameSize = maxInterpKeyFrameSize;

    anim->isSubInterpolator = FALSE;
    anim->offsetInParent = 0;
    anim->parentAnimation = anim;

    anim->keyFrameApplyCB = (RtAnimKeyFrameApplyCallBack)NULL;
    anim->keyFrameInterpolateCB = (RtAnimKeyFrameInterpolateCallBack)NULL;
    anim->keyFrameBlendCB = (RtAnimKeyFrameBlendCallBack)NULL;
    anim->keyFrameAddCB = (RtAnimKeyFrameAddCallBack)NULL;

    RWRETURN(anim);
}


/**
 * \ingroup rtanim
 * \ref RtAnimInterpolatorDestroy destroys an
 * \ref RtAnimInterpolator object.
 *
 * \param anim Pointer to the \ref RtAnimAnimation to be destroyed.
 *
 * \see RtAnimAnimationCreate
 * \see RtAnimAnimationDestroy
 * \see RtAnimInterpolatorCreate
 * \see RtAnimInterpolatorCreateSubInterpolator
 */
void
RtAnimInterpolatorDestroy(RtAnimInterpolator *anim)
{
    RWAPIFUNCTION(RWSTRING("RtAnimInterpolatorDestroy"));
    RWASSERT(anim);

    RwFree(anim);

    RWRETURNVOID();
}


/**
 * \ingroup rtanim
 * \ref RtAnimInterpolatorSetCurrentAnim sets the current animation on the
 * animation interpolator. It is assumed that the animation is designed for the
 * animation interpolator it is being set on since no animation interpolator structure
 * comparisons are made. The animation defines the interpolation schemes used
 * and they will be setup on the animation interpolator at this call. The maximum
 * keyframe size of the animation interpolator must be sufficient to support the
 * keyframe size required by the interpolation scheme. The animation is
 * initialized to time zero at this call.
 *
 * \param animI Pointer to the \ref RtAnimInterpolator.
 * \param anim Pointer to the \ref RtAnimAnimation.
 *
 * \return \ref RwBool, TRUE on success, FALSE if an error occurs.
 *
 */
RwBool
RtAnimInterpolatorSetCurrentAnim(RtAnimInterpolator *animI,
                               RtAnimAnimation *anim)
{
    RtAnimInterpolatorInfo     *interpInfo;
    RwInt32                     i;
    void                       *kf1, *kf2, *interpFrame;

    RWAPIFUNCTION(RWSTRING("RtAnimInterpolatorSetCurrentAnim"));
    RWASSERT(animI);
    RWASSERT(anim);
    RWASSERT(anim->numFrames >= animI->numNodes * 2);

    animI->pCurrentAnim = anim;
    animI->currentTime = 0.0f;

    /* Get interpolator functions */
    interpInfo = anim->interpInfo;
    animI->currentInterpKeyFrameSize = interpInfo->interpKeyFrameSize;
    RWASSERT(animI->currentInterpKeyFrameSize <= animI->maxInterpKeyFrameSize);
    animI->currentAnimKeyFrameSize = interpInfo->animKeyFrameSize;

    animI->keyFrameApplyCB = interpInfo->keyFrameApplyCB;
    animI->keyFrameBlendCB = interpInfo->keyFrameBlendCB;
    animI->keyFrameInterpolateCB = interpInfo->keyFrameInterpolateCB;
    animI->keyFrameAddCB = interpInfo->keyFrameAddCB;

    /*
     * Set up initial interpolation frames for time=0
     * First copy the time=0 keyframes to the interpolated frame array
     */
    //TODO: convert these don't copy
    for (i=0; i < animI->numNodes; i++)
    {
        animI->keyFrameInterpolateCB(rtANIMGETINTERPFRAME(animI, i),
                                     ((RwUInt8 *)anim->pFrames) + (i * animI->currentAnimKeyFrameSize),
                                     ((RwUInt8 *)anim->pFrames) + ((i + animI->numNodes) * animI->currentAnimKeyFrameSize),
                                     0.0f, anim->customData);
    }

    /*
     * Now initialize the interpolated frame headers to point to the initial
     * keyframe pairs.
     */
    interpFrame = rtANIMGETINTERPFRAME(animI, 0);
    kf1 = anim->pFrames;
    kf2 = (RwInt8 *)anim->pFrames + animI->numNodes * animI->currentAnimKeyFrameSize;

    for (i=0; i < animI->numNodes; i++)
    {
        RtAnimInterpFrameHeader  *hdr = (RtAnimInterpFrameHeader *)interpFrame;

        hdr->keyFrame1 = kf1;
        hdr->keyFrame2 = kf2;

        interpFrame = (RwInt8 *)interpFrame + animI->currentInterpKeyFrameSize;
        kf1 = (RwInt8 *)kf1 + animI->currentAnimKeyFrameSize;
        kf2 = (RwInt8 *)kf2 + animI->currentAnimKeyFrameSize;
    }

    animI->pNextFrame = (RwUInt8 *)anim->pFrames + (animI->numNodes * animI->currentAnimKeyFrameSize * 2);

    RWRETURN(TRUE);
}

#if defined(RWDEBUG)
/**
 * \ingroup rtanim
 * \ref RtAnimInterpolatorGetCurrentAnim return the current animation on the
 * animation interpolator.
 *
 * \param animI Pointer to the \ref RtAnimInterpolator.
 *
 * \return a pointer to the current \ref RtAnimAnimation if any.
 *
 * \see RtAnimInterpolatorSetCurrentAnim
 *
 */
extern RtAnimAnimation *
RtAnimInterpolatorGetCurrentAnim(RtAnimInterpolator *animI)
{
    RWAPIFUNCTION(RWSTRING("RtAnimInterpolatorGetCurrentAnim"));
    RWASSERT(animI);

    RWRETURN(animI->pCurrentAnim);
}
#endif


/**
 * \ingroup rtanim
 * \ref RtAnimInterpolatorSetKeyFrameCallBacks sets up the callbacks to be
 * used by the animation interpolator. These callbacks perform keyframe blending,
 * interpolation and conversion to matrices. The callback setup is implicitly
 * performed when setting the current animation on an animation interpolator.
 * However if you wish to fully procedurally drive the keyframe data this
 * function allows the callbacks to be set up.
 *
 * \param anim Pointer to the \ref RtAnimInterpolator.
 * \param keyFrameTypeID \ref RwInt32 containing the ID of the keyframe type
 *        to retrieve the callbacks for.
 *
 * \return \ref RwBool, TRUE on success, FALSE if an error occurs.
 *
 */
RwBool
RtAnimInterpolatorSetKeyFrameCallBacks(RtAnimInterpolator *anim,
                                     RwInt32 keyFrameTypeID)
{
    RtAnimInterpolatorInfo     *interpInfo;

    RWAPIFUNCTION(RWSTRING("RtAnimInterpolatorSetKeyFrameCallBacks"));
    RWASSERT(anim);

    interpInfo = RtAnimGetInterpolatorInfo(keyFrameTypeID);
    if (!interpInfo)
    {
        /* Unregistered animation type */
        RWRETURN(FALSE);
    }

    anim->keyFrameApplyCB = interpInfo->keyFrameApplyCB;
    anim->keyFrameBlendCB = interpInfo->keyFrameBlendCB;
    anim->keyFrameInterpolateCB = interpInfo->keyFrameInterpolateCB;
    anim->keyFrameAddCB = interpInfo->keyFrameAddCB;

    RWRETURN(TRUE);
}

/**
 * \ingroup rtanim
 * \ref RtAnimInterpolatorSetAnimLoopCallBack is used to set a callback
 * function that will be called when an animation interpolator's animation loops.
 * The callback function should return a pointer to the animation interpolator if
 * the function should continue to be called, or NULL if it should never be
 * called again.
 *
 * \param anim      Pointer to an \ref RtAnimInterpolator.
 * \param callBack  Pointer to a \ref RtAnimCallBack function.
 * \param data      Void pointer to a block of data to be passed into the callback
 *                  function.
 *
 */
void
RtAnimInterpolatorSetAnimLoopCallBack(RtAnimInterpolator *anim,
                                    RtAnimCallBack callBack,
                                    void *data )
{
    RWAPIFUNCTION(RWSTRING("RtAnimInterpolatorSetAnimLoopCallBack"));

    RWASSERT(anim);

    anim->pAnimLoopCallBack = callBack;
    anim->pAnimLoopCallBackData = data;

    RWRETURNVOID();
}

/**
 * \ingroup rtanim
 * \ref RtAnimInterpolatorSetAnimCallBack is used to set a callback
 * function that will be called when an animation interpolator reaches
 * a specified time. The callback function should return a pointer to
 * the animation interpolator if the function should continue to be
 * called, or NULL if it should never be called again.
 *
 * \param anim      Pointer to an \ref RtAnimInterpolator.
 * \param callBack  Pointer to a \ref RtAnimCallBack function.
 * \param time      \ref RwReal containing the time at which the callback
 *                  function should be called.
 * \param data      Void pointer to a block of data to be passed into the
 *                  callback function.
 *
 */
void
RtAnimInterpolatorSetAnimCallBack(RtAnimInterpolator *anim,
                                RtAnimCallBack callBack,
                                RwReal time,
                                void *data )
{
    RWAPIFUNCTION(RWSTRING("RtAnimInterpolatorSetAnimCallBack"));

    RWASSERT(anim);

    anim->pAnimCallBack = callBack;
    anim->animCallBackTime = time;
    anim->pAnimCallBackData = data;

    RWRETURNVOID();
}


/**
 * \ingroup rtanim
 * \ref RtAnimInterpolatorCopy copies the state of nodes in the in-animation
 * interpolator into the out-animation interpolator.
 *
 * \param outAnim   Pointer to the \ref RtAnimInterpolator in which the result is
 *                  returned.
 * \param inAnim    Pointer to the input \ref RtAnimInterpolator.
 *
 * \return \ref RwBool, TRUE on success, FALSE otherwise.
 *
 */
RwBool
RtAnimInterpolatorCopy(RtAnimInterpolator *outAnim,
                              RtAnimInterpolator *inAnim)
{
    RWAPIFUNCTION(RWSTRING("RtAnimInterpolatorCopy"));
    RWASSERT(outAnim);
    RWASSERT(inAnim);
    RWASSERT(outAnim->numNodes == inAnim->numNodes);

    memcpy(rtANIMGETINTERPFRAME(outAnim, 0),
           rtANIMGETINTERPFRAME(inAnim, 0),
           outAnim->currentInterpKeyFrameSize * outAnim->numNodes);

    RWRETURN(TRUE);
}


/**
 * \ingroup rtanim
 * \ref RtAnimInterpolatorSubAnimTime subtracts \e time from the current
 * animation time and will step through the keyframes to find the current pair
 * for each node to interpolate. These will then be interpolated by calling
 * the appropriate overloaded interpolation scheme functions. If the animation
 * loops or passes the time set in \ref RtAnimInterpolatorSetAnimCallBack then
 * the appropriate callback will be called. By default the animation will simply
 * loop continuously.
 *
 * \param interp A pointer to \ref RtAnimInterpolator.
 * \param time \ref RwReal containing the amount of time to subtract from the animation.
 *
 * \return \ref RwBool, TRUE on success.
 *
 * \see RtAnimInterpolatorAddAnimTime
 * \see RtAnimInterpolatorSetCurrentTime
 */

RwBool
RtAnimInterpolatorSubAnimTime(RtAnimInterpolator *interp,
                              RwReal time)
{
    RtAnimAnimation    *anim;
    RwBool              triggered = FALSE;
    RwBool              looped = FALSE;

    RWAPIFUNCTION(RWSTRING("RtAnimInterpolatorSubAnimTime"));
    RWASSERT(interp);
    RWASSERT(time >= 0.0f);

    triggered |=
        ((interp->currentTime > interp->animCallBackTime) &&
         ((interp->currentTime - time) <= interp->animCallBackTime));

    interp->currentTime -= time;
    anim = interp->pCurrentAnim;

    /* Have we looped? */
    if (interp->currentTime < 0.0f)
    {
        RwReal  duration = anim->duration;

        while (interp->currentTime < 0.0f)
        {
            interp->currentTime += duration;
        }

        /* Reset and scan forwards to correct time */
        time = interp->currentTime;
        RtAnimInterpolatorSetCurrentAnim(interp, anim);
        RtAnimInterpolatorAddAnimTime(interp, time);

        triggered |= (interp->currentTime < interp->animCallBackTime);
        looped = TRUE;
    }
    else
    {
        RwInt32                     numNodes;
        RtAnimInterpFrameHeader    *interpFrameHdr;

        numNodes = interp->numNodes;
        interpFrameHdr =
            (RtAnimInterpFrameHeader *) rtANIMGETINTERPFRAME(interp, 0);

        /* For each interpolator node */
        while (numNodes--)
        {
            RWASSERT(AnimKeyFramePtrValid(anim, interpFrameHdr->keyFrame1));
            RWASSERT(AnimKeyFramePtrValid(anim, interpFrameHdr->keyFrame2));

            /* Update key frame pair if necessary */
            while (interp->currentTime < interpFrameHdr->keyFrame1->time)
            {
                interpFrameHdr->keyFrame2 = interpFrameHdr->keyFrame1;
                interpFrameHdr->keyFrame1 =
                    interpFrameHdr->keyFrame1->prevFrame;
            }

            /* Interpolate to new time */
            RtAnimKeyFrameInterpolate(interp,
                                      interpFrameHdr,
                                      interpFrameHdr->keyFrame1,
                                      interpFrameHdr->keyFrame2,
                                      interp->currentTime);

            interpFrameHdr = (RtAnimInterpFrameHeader *)
                ((RwUInt8 *)interpFrameHdr + interp->currentInterpKeyFrameSize);
        }

        /* Invalidate the "next frame" pointer
         * to force a recalculation when playing forwards */
        interp->pNextFrame = NULL;
    }

    if (triggered && interp->pAnimCallBack)
    {
        if (!interp->pAnimCallBack(interp, interp->pAnimCallBackData))
        {
            /* If the callback returns false, we don't want to call it again */
            interp->pAnimCallBack = (RtAnimCallBack)NULL;
        }
    }

    if (looped && interp->pAnimLoopCallBack)
    {
        if (!interp->pAnimLoopCallBack(interp, interp->pAnimLoopCallBackData))
        {
            /* If the callback returns false, we don't want to call it again */
            interp->pAnimLoopCallBack = (RtAnimCallBack)NULL;
        }
    }

    RWRETURN(TRUE);
}

/**
 * \ingroup rtanim
 * \ref RtAnimInterpolatorAddAnimTime adds \e time to the animation's current
 * time and will step through the keyframes to find the current pair for each
 * node to interpolate. These will then be interpolated by calling the
 * appropriate overloaded interpolation scheme functions. If the animation
 * loops or passes the time set in \ref RtAnimInterpolatorSetAnimCallBack then
 * the appropriate callback will be called. By default the animation will simply
 * loop continuously.
 *
 * The added time must be positive. For backwards playing, use
 * \ref RtAnimInterpolatorSubAnimTime. If an animation has been playing
 * backwards prior to \ref RtAnimInterpolatorAddAnimTime being called, then
 * this will be detected and the change will be handled appropriately.
 *
 * \param interp A pointer to \ref RtAnimInterpolator.
 * \param time \ref RwReal containing the amount of time to add to the animation.
 *
 * \return \ref RwBool, TRUE on success.
 *
 * \see RtAnimInterpolatorAddAnimTime
 * \see RtAnimInterpolatorSetCurrentAnim
 * \see RtAnimInterpolatorSetCurrentTime
 * \see RtAnimInterpolatorSubAnimTime
 */
RwBool
RtAnimInterpolatorAddAnimTime(RtAnimInterpolator *interp, RwReal time)
{
    RWAPIFUNCTION(RWSTRING("RtAnimInterpolatorAddAnimTime"));
    RWASSERT(interp);
    RWASSERT(time >= 0.0f);

    if (time > 0.0f)
    {
        RtAnimAnimation            *anim = interp->pCurrentAnim;
        RwReal                      duration = anim->duration;
        RwInt32                     numNodes = interp->numNodes;
        RwBool                      triggered = FALSE;
        RwBool                      looped = FALSE;
        RwInt32                     interpFrameIndex;
        RtAnimInterpFrameHeader    *interpFrame;
        void                       *endGuard;

        triggered |=
            ((interp->currentTime < interp->animCallBackTime) &&
             ((interp->currentTime + time) >= interp->animCallBackTime));

        /* Update the time */
        interp->currentTime += time;

        /* Have we looped? */
        if (interp->currentTime > duration)
        {
            RwReal saveTime;

            while (interp->currentTime > duration)
            {
                interp->currentTime -= duration;
                triggered |= (interp->currentTime >= interp->animCallBackTime);
                looped = TRUE;
            }

            saveTime = interp->currentTime;
            RtAnimInterpolatorSetCurrentAnim(interp, anim);
            interp->currentTime = saveTime;
        }

        /* Have we been playing backwards? */
        if (interp->pNextFrame == NULL)
        {
            RwReal saveTime;
            /*
             * This should only be called in rare instances
             * where the direction of the animation has
             * changed and pNextFrame still needs to be
             * set as above condition was not satisfied
             */
            saveTime = interp->currentTime;
            RtAnimInterpolatorSetCurrentAnim(interp, anim);
            interp->currentTime = saveTime;

            /* pNextFrame should should have been fixed by now */
            RWASSERT(interp->pNextFrame);
        }

        /* Update expired keyframe pairs ready for interpolation */
        endGuard = (RwUInt8*)anim->pFrames
                    + anim->numFrames*interp->currentAnimKeyFrameSize;

        interpFrameIndex = 0;
        while (interp->pNextFrame < endGuard)
        {
#ifdef RWDEBUG
            RwInt32 count = 0;
#endif
            RtAnimKeyFrameHeader *prevFrame = (RtAnimKeyFrameHeader *)
                ((RtAnimKeyFrameHeader *)interp->pNextFrame)->prevFrame;

            RWASSERT(AnimKeyFramePtrValid(anim, prevFrame));

            /* No more to update? */
            if (prevFrame->time > interp->currentTime)
            {
                break;
            }

            /*
             * Search for the interp frame whose keyframes expire, this is
             * optimized for the case when the keyframes update in sync
             * by starting with the next interpframe after the last one
             * updated.
             */
           do
            {

                RWASSERTM(count < numNodes, (RWSTRING(
                    "Infinite loop. Either an RtAnimInterpFrameHeader or ")
                    RWSTRING("RtAnimAnimation previous frame pointer is probably ")
                    RWSTRING("invalid.")));

#if defined(RWDEBUG)
                count++;
#endif /* defined(RWDEBUG) */

                interpFrame = (RtAnimInterpFrameHeader *)
                    rtANIMGETINTERPFRAME(interp, interpFrameIndex);

                RWASSERT(AnimKeyFramePtrValid(anim, interpFrame->keyFrame2));
                RWASSERT(interpFrame->keyFrame1 < interpFrame->keyFrame2);

                interpFrameIndex++;
                if (interpFrameIndex >= numNodes)
                {
                    interpFrameIndex = 0;
                }
            }
            while (interpFrame->keyFrame2 != prevFrame);

            /* Update pointers */
            interpFrame->keyFrame1 = interpFrame->keyFrame2;
            interpFrame->keyFrame2 = (RtAnimKeyFrameHeader *)interp->pNextFrame;

            interp->pNextFrame =
                (RwUInt8 *)interp->pNextFrame + interp->currentAnimKeyFrameSize;
        }

        /* Do interpolation for each keyframe pair */
        interpFrame = (RtAnimInterpFrameHeader *)rtANIMGETINTERPFRAME(interp, 0);
        for (interpFrameIndex = 0; interpFrameIndex < numNodes; interpFrameIndex++)
        {
			RtAnimKeyFrameInterpolate(interp,
				interpFrame,
				interpFrame->keyFrame1,
				interpFrame->keyFrame2,
				interp->currentTime);

            interpFrame = (RtAnimInterpFrameHeader *)
                ((RwUInt8 *)interpFrame + interp->currentInterpKeyFrameSize);
        }

        /* Time triggered callback */
        if (triggered && interp->pAnimCallBack)
        {
            if (!interp->pAnimCallBack(interp, interp->pAnimCallBackData))
            {
                /* If callback returns false, don't want to call it again */
                interp->pAnimCallBack = (RtAnimCallBack) NULL;
            }
        }

        /* Anim loop callback */
        if (looped && interp->pAnimLoopCallBack)
        {
            if (!interp->pAnimLoopCallBack(interp, interp->pAnimLoopCallBackData))
            {
                /* If callback returns false, don't want to call it again */
                interp->pAnimLoopCallBack = (RtAnimCallBack) NULL;
            }
        }
    }

    RWRETURN(TRUE);
}

/**
 * \ingroup rtanim
 * \ref RtAnimInterpolatorSetCurrentTime sets the current animation
 * time of an animation interpolator.
 *
 * \param anim A pointer to \ref RtAnimInterpolator.
 * \param time \ref RwReal containing the time to which to set the
 *             current animation.
 *
 * \return \ref RwBool, TRUE on success, FALSE otherwise.
 *
 * \see RtAnimInterpolatorAddAnimTime
 * \see RtAnimInterpolatorSubAnimTime
 * \see RtAnimInterpolatorSetCurrentTime
 */
RwBool
RtAnimInterpolatorSetCurrentTime(RtAnimInterpolator *anim,
                                   RwReal time)
{
    RwReal offset;
    RWAPIFUNCTION(RWSTRING("RtAnimInterpolatorSetCurrentTime"));
    RWASSERT(anim);

    offset = time - anim->currentTime;

    if (offset < 0.0f)
    {
        RtAnimInterpolatorSubAnimTime(anim, -offset);
    }
    else
    {
        RtAnimInterpolatorAddAnimTime(anim, offset);
    }

    RWRETURN(TRUE);
}


/**
 * \ingroup rtanim
 * \ref RtAnimAnimationMakeDelta makes a delta animation from a given
 * RtAnimAnimation. It requires the number of nodes to decode the animation
 * and a time from which to make the delta. This will give you an animation
 * which can be added on to another animation, e.g. characters leaning whilst
 * running. This calls the overloaded interpolation scheme
 * \ref RtAnimKeyFrameMulRecipCallBack which multiplies the incoming keyframe
 * by the reciprocal of the start keyframe given.
 *
 * \param animation A pointer to an \ref RtAnimAnimation.
 * \param numNodes \ref RwInt32 containing the number of node to decode the animation.
 * \param time \ref RwReal containing the time in the animation to delta from.
 *
 * \return \ref RwBool, TRUE on success, or FALSE if an error occurs.
 *
 */
RwBool
RtAnimAnimationMakeDelta(RtAnimAnimation *animation,
                          RwInt32 numNodes,
                          RwReal time)
{
    RtAnimInterpolator *pAnim;
    RwInt32                i;
    RwInt32                interpKeyFrameSize;
    RtAnimKeyFrameHeader *pAnimNodeFrame;
    RtAnimKeyFrameHeader *pAnimationNodeFrame;
    RtAnimKeyFrameHeader *pNextAnimFrame;
    RtAnimKeyFrameHeader *pSearchFrame;

    RWAPIFUNCTION(RWSTRING("RtAnimAnimationMakeDelta"));
    RWASSERT(animation);

    /* Create an animation interpolator to get the animation at desired delta time */

    interpKeyFrameSize = animation->interpInfo->interpKeyFrameSize;
    pAnim = RtAnimInterpolatorCreate(numNodes,
                                        interpKeyFrameSize);
    if (!pAnim)
    {
        RWRETURN(FALSE);
    }

    RtAnimInterpolatorSetCurrentAnim(pAnim, animation);
    RtAnimInterpolatorSetCurrentTime(pAnim, time);

    /* Start of frame list has a frame for each node (time 0) */
    pAnimNodeFrame = (RtAnimKeyFrameHeader *)
        rtANIMGETINTERPFRAME(pAnim, 0);
    pAnimationNodeFrame = (RtAnimKeyFrameHeader *)
        animation->pFrames;

    /* Track each node in animation interpolator with the frame-sequence
     * for that node in the animation */
    for (i = 0; i < numNodes; i++)
    {
        /* Now change this nodes frames in the animation */
        pNextAnimFrame = pAnimationNodeFrame;
        while (1)
        {
            /* Transform animation frame by inverse of animation interpolator frame */
            RWASSERT(animation->interpInfo->keyFrameMulRecipCB);
            animation->interpInfo->keyFrameMulRecipCB(pNextAnimFrame, pAnimNodeFrame);

            /* End of animation for this node? */
            if (pNextAnimFrame->time == animation->duration)
            {
                break;
            }

            /* Now find the next frame for this node (frame that points back to this one) */
            pSearchFrame = pNextAnimFrame;
            while (pSearchFrame->prevFrame != pNextAnimFrame)
            {
                pSearchFrame = (RtAnimKeyFrameHeader *)((RwUInt8 *)pSearchFrame + pAnim->currentAnimKeyFrameSize);
            }

            pNextAnimFrame = pSearchFrame;
        }

        /* Next node */
        pAnimNodeFrame = (RtAnimKeyFrameHeader *)((RwUInt8 *)pAnimNodeFrame + pAnim->currentInterpKeyFrameSize);
        pAnimationNodeFrame = (RtAnimKeyFrameHeader *)((RwUInt8 *)pAnimationNodeFrame + pAnim->currentAnimKeyFrameSize);

    }

    /* Done */
    RtAnimInterpolatorDestroy(pAnim);

    RWRETURN(TRUE);
}


/**
 * \ingroup rtanim
 * \ref RtAnimInterpolatorBlend interpolates between two animation
 * interpolators. The result will be interpolated between the current state of the
 * two input animation interpolators.
 *
 * \param outAnim   Pointer to an \ref RtAnimInterpolator in which the result is
 *                  returned.
 * \param inAnim1   Pointer to the first input \ref RtAnimInterpolator.
 * \param inAnim2   Pointer to the second input \ref RtAnimInterpolator.
 * \param alpha     \ref RwReal containing the blending parameter, where
 * \li 0.0 returns \e inAnim1
 * \li 1.0 returns \e inAnim2
 *
 * \return \ref RwBool, TRUE on success, FALSE otherwise.
 *
 */
RwBool
RtAnimInterpolatorBlend(RtAnimInterpolator *outAnim,
                      RtAnimInterpolator *inAnim1,
                      RtAnimInterpolator *inAnim2,
                      RwReal alpha)
{
    RwInt32             i;

    RWAPIFUNCTION(RWSTRING("RtAnimInterpolatorBlend"));
    RWASSERT(outAnim);
    RWASSERT(inAnim1);
    RWASSERT(inAnim2);
    RWASSERT((alpha >= (RwReal) 0.0) && (alpha <= (RwReal) 1.0));
    RWASSERT(outAnim->numNodes == inAnim1->numNodes);
    RWASSERT(outAnim->numNodes == inAnim2->numNodes);

    for (i = 0; i < outAnim->numNodes; i++)
    {
        RtAnimKeyFrameBlend(outAnim,
                          rtANIMGETINTERPFRAME(outAnim, i),
                          rtANIMGETINTERPFRAME(inAnim1, i),
                          rtANIMGETINTERPFRAME(inAnim2, i),
                          alpha);
    }

    RWRETURN(TRUE);
}

/**
 * \ingroup rtanim
 * \ref RtAnimInterpolatorAddTogether adds together two animation
 * interpolators. Each animation interpolator should have an animation attached, and
 * the result will be the addition of the two input animation interpolators. This is
 * mainly useful where the second animation interpolator's pose is a delta from a
 * set pose to be added to the pose held in the first animation interpolator.
 * For example, a walk animation could have a crouch pose added to it as a delta
 * from a standing pose.
 * \note \e outAnim, \e inAnim1 and \e inAnim2 MUST be different.
 *
 * \param outAnim   Pointer to an \ref RtAnimInterpolator in which the result is
 *                  returned.
 * \param inAnim1   Pointer to the first input \ref RtAnimInterpolator.
 * \param inAnim2   Pointer to the second input \ref RtAnimInterpolator.
 *
 * \return \ref RwBool, TRUE on success, FALSE otherwise.
 *
 * \see RtAnimAnimationMakeDelta
 *
 */

RwBool
RtAnimInterpolatorAddTogether(RtAnimInterpolator *outAnim,
                                        RtAnimInterpolator *inAnim1,
                                        RtAnimInterpolator *inAnim2)
{
    RwInt32             i;

    RWAPIFUNCTION(RWSTRING("RtAnimInterpolatorAddTogether"));
    RWASSERT(outAnim);
    RWASSERT(inAnim1);
    RWASSERT(inAnim2);
    RWASSERT(outAnim->numNodes == inAnim1->numNodes);
    RWASSERT(outAnim->numNodes == inAnim2->numNodes);

    for (i = 0; i < outAnim->numNodes; i++)
    {

        RtAnimKeyFrameAddTogether(outAnim,
                                rtANIMGETINTERPFRAME(outAnim, i),
                                rtANIMGETINTERPFRAME(inAnim1, i),
                                rtANIMGETINTERPFRAME(inAnim2, i));
    }

    RWRETURN(TRUE);
}


#if defined(RWDEBUG)
/**
 * \ingroup rtanim
 * \ref RtAnimKeyFrameInterpolate interpolates between two keyframes and
 * returns the result.
 *
 * \param animation Pointer to the \ref RtAnimInterpolator in use.
 * \param out Void pointer to the output keyframe.
 * \param in1 Void pointer to the first input keyframe.
 * \param in2 Void pointer to the second input keyframe.
 * \param time \ref RwReal containing the time to which to interpolate.
 */
void
RtAnimKeyFrameInterpolate(RtAnimInterpolator *animation,
                          void *out, void *in1,
                          void *in2, RwReal time)
{
    RWAPIFUNCTION(RWSTRING("RtAnimKeyFrameInterpolate"));
    RWASSERT(animation);
    RWASSERT(animation->pCurrentAnim);

    RWASSERT(animation->keyFrameInterpolateCB);
    animation->keyFrameInterpolateCB(out, in1, in2, time,
                                     animation->pCurrentAnim->customData);

    RWRETURNVOID();
}

/**
 * \ingroup rtanim
 * \ref RtAnimKeyFrameBlend blends between two interpolated
 * keyframes using a given blend factor. These would normally
 * be from different \ref RtAnimInterpolator objects, with the
 * results being stored in a third \ref RtAnimInterpolator.
 *
 * \param animation Pointer to the \ref RtAnimInterpolator in use.
 * \param out Void pointer to the output keyframe.
 * \param in1 Void pointer to the first input keyframe.
 * \param in2 Void pointer to the second input keyframe.
 * \param alpha \ref RwReal containing the blending factor.
 */
void
RtAnimKeyFrameBlend(RtAnimInterpolator *animation,
                  void *out,
                  void *in1,
                  void *in2,
                  RwReal alpha)
{
    RWAPIFUNCTION(RWSTRING("RtAnimKeyFrameBlend"));
    RWASSERT(animation);

    RWASSERT(animation->keyFrameBlendCB);
    animation->keyFrameBlendCB(out, in1, in2, alpha);

    RWRETURNVOID();
}

/**
 * \ingroup rtanim
 * \ref RtAnimKeyFrameApply converts an interpolated
 * keyframe structure into a usable format, such as a matrix in the
 * case of hierarchical animation.
 *
 * \param animation Pointer to the \ref RtAnimInterpolator in use.
 * \param result Void pointer to the output data.
 * \param iFrame Void pointer to the input keyframe.
 */
void
RtAnimKeyFrameApply(RtAnimInterpolator *animation,
                     void *result, void *iFrame)
{
    RWAPIFUNCTION(RWSTRING("RtAnimKeyFrameApply"));
    RWASSERT(animation);
    RWASSERT(result);
    RWASSERT(iFrame);

    RWASSERT(animation->keyFrameApplyCB);
    animation->keyFrameApplyCB(result, iFrame);

    RWRETURNVOID();
}

/**
 * \ingroup rtanim
 * \ref RtAnimKeyFrameAddTogether adds two interpolated
 *  keyframes together. These would normally be from two
 * different \ref RtAnimInterpolator objects, with the results
 * being stored in a third \ref RtAnimInterpolator.
 *
 * \param animation Pointer to the \ref RtAnimInterpolator in use.
 * \param out Void pointer to the output summed frame.
 * \param in1 Void pointer to the first input frame.
 * \param in2 Void pointer to the second input frame.
 */
void
RtAnimKeyFrameAddTogether(RtAnimInterpolator *animation,
                        void *out, void *in1, void *in2)
{
    RWAPIFUNCTION(RWSTRING("RtAnimKeyFrameAddTogether"));
    RWASSERT(animation);
    RWASSERT(out);
    RWASSERT(in1);
    RWASSERT(in2);

    RWASSERT(animation->keyFrameAddCB);
    animation->keyFrameAddCB(out, in1, in2);

    RWRETURNVOID();
}


#endif /* (defined(RWDEBUG)) */

/**
 * \ingroup rtanim
 * \ref RtAnimInterpolatorCreateSubInterpolator creates a
 * sub-animation interpolator of a given main animation interpolator. The
 * parent-animation interpolator and the offset into the animation interpolator are
 * provided along with flags defining the sub-animation interpolators settings. You can
 * specify a different \e maxkeyframesize for the sub-animation interpolator and use
 * different types of keyframe on sub-animation interpolators and main animation
 * interpolators. Specifying -1 will adopt the parent animation interpolators size
 * settings.
 *
 * The sub-animation interpolator can then be used to run a different animation from
 * the base animation interpolator.
 *
 * \param parentAnim        The parent \ref RtAnimInterpolator to be used for this
 *                          sub-animation interpolator.
 * \param startNode         \ref RwInt32 containing the index of the node representing
 *                          the root of the new sub-animation interpolator.
 * \param numNodes          \ref RwInt32 containing the number of nodes to create.
 * \param maxInterpKeyFrameSize   \ref RwInt32 containing the maximum keyframe size to allow
 *                          in the sub-animation interpolator, passing -1 will copy the
 *                          parent animation interpolators settings.
 *
 * \return Pointer to the new sub-\ref RtAnimInterpolator on success.
 *
 * \see RtAnimInterpolatorCreate
 * \see RtAnimAnimationDestroy
 */
RtAnimInterpolator *
RtAnimInterpolatorCreateSubInterpolator(
                                        RtAnimInterpolator *parentAnim,
                                        RwInt32 startNode,
                                        RwInt32 numNodes,
                                        RwInt32 maxInterpKeyFrameSize)
{
    RtAnimInterpolator *anim;

    RWAPIFUNCTION(RWSTRING("RtAnimInterpolatorCreateSubInterpolator"));
    RWASSERT(parentAnim);
    RWASSERT(parentAnim->numNodes > startNode);
    RWASSERT(parentAnim->numNodes > numNodes);
    RWASSERT(parentAnim->numNodes >= startNode + numNodes);

    if (maxInterpKeyFrameSize == -1)
    {
        maxInterpKeyFrameSize = parentAnim->maxInterpKeyFrameSize;
    }

    anim = RtAnimInterpolatorCreate(numNodes,
                                    maxInterpKeyFrameSize);

    anim->parentAnimation = parentAnim;
    anim->offsetInParent = startNode;
    anim->isSubInterpolator = TRUE;

    RWRETURN(anim);
}

/**
 * \ingroup rtanim
 * \ref RtAnimInterpolatorBlendSubInterpolator interpolates the nodes in a
 * sub-animation interpolator with it parent animation interpolator.  Each animation
 * interpolator should have an animation attached, and the result will be
 * interpolated between the current state of the two input animation interpolators.
 * Either source animation interpolator can be the sub-animation interpolator, the
 * blend is performed from \e inAnim1 to \e inAnim2. The output animation interpolator
 * must be identical in structure to either the main or sub-animation interpolator.
 *
 * \param outAnim   Pointer to an \ref RtAnimInterpolator in which the result is
 *                  returned.
 * \param inAnim1   Pointer to the first source \ref RtAnimInterpolator.
 * \param inAnim2   Pointer to the second source \ref RtAnimInterpolator.
 * \param alpha     \ref RwReal containing the blending parameter where:
 * \li 0.0 returns \e inAnim1
 * \li 1.0 returns \e inAnim2
 *
 * \return \ref RwBool, TRUE on success, FALSE otherwise.
 *
 */
RwBool
RtAnimInterpolatorBlendSubInterpolator(RtAnimInterpolator *outAnim,
                                            RtAnimInterpolator *inAnim1,
                                            RtAnimInterpolator *inAnim2,
                                            RwReal alpha)
{
    RwInt32             i;

    RWAPIFUNCTION(RWSTRING("RtAnimInterpolatorBlendSubInterpolator"));
    RWASSERT(outAnim);
    RWASSERT(inAnim1);
    RWASSERT(inAnim2);
    RWASSERT((alpha >= (RwReal) 0.0) && (alpha <= (RwReal) 1.0));
    RWASSERT(outAnim->numNodes == inAnim1->numNodes ||
             outAnim->numNodes == inAnim2->numNodes);

    if (TRUE == outAnim->isSubInterpolator)
    {
        RwInt32 outOffset = outAnim->offsetInParent;
        RwInt32 in1Offset = inAnim1->offsetInParent;
        RwInt32 in2Offset = inAnim2->offsetInParent;

        for (i = outAnim->offsetInParent;
             i < outAnim->numNodes+outAnim->offsetInParent;
             i++)
        {
            /* blend keyframes */
        RtAnimKeyFrameBlend(outAnim,
                              rtANIMGETINTERPFRAME(outAnim, i - outOffset),
                              rtANIMGETINTERPFRAME(inAnim1, i - in1Offset),
                              rtANIMGETINTERPFRAME(inAnim2, i - in2Offset),
                              alpha);
        }
    }
    else /* output animation interpolator is a main animation interpolator */
    {
        if (TRUE == inAnim1->isSubInterpolator)
        {
            /* memcpy keys up to the parent offset */
            memcpy(rtANIMGETINTERPFRAME(outAnim, 0),
                   rtANIMGETINTERPFRAME(inAnim2, 0),
                   outAnim->currentInterpKeyFrameSize * inAnim1->offsetInParent);

            for (i = inAnim1->offsetInParent;
                 i < inAnim1->offsetInParent+inAnim1->numNodes;
                 i++)
            {
                /* blend keyframes in the sub interpolator range */
        RtAnimKeyFrameBlend(outAnim,
                                  rtANIMGETINTERPFRAME(outAnim, i),
                                  rtANIMGETINTERPFRAME(inAnim1, i - inAnim1->offsetInParent),
                                  rtANIMGETINTERPFRAME(inAnim2, i),
                                  alpha);
            }

            /* memcpy keys past sub animation interpolator */
            memcpy(rtANIMGETINTERPFRAME(outAnim,
                                                  inAnim1->offsetInParent+inAnim1->numNodes),
                   rtANIMGETINTERPFRAME(inAnim2,
                                                  inAnim1->offsetInParent+inAnim1->numNodes),
                   outAnim->currentInterpKeyFrameSize *
                   (outAnim->numNodes-(inAnim1->offsetInParent+inAnim1->numNodes)));
        }
        else
        {
            /* memcpy keys up to the parent offset */
            memcpy(rtANIMGETINTERPFRAME(outAnim, 0),
                   rtANIMGETINTERPFRAME(inAnim1, 0),
                   outAnim->currentInterpKeyFrameSize * inAnim2->offsetInParent);

            for (i = inAnim2->offsetInParent; i < inAnim2->offsetInParent+inAnim2->numNodes; i++)
            {
                /* blend keyframes in the sub interpolator range */
        RtAnimKeyFrameBlend(outAnim,
                                  rtANIMGETINTERPFRAME(outAnim, i),
                                  rtANIMGETINTERPFRAME(inAnim1, i),
                                  rtANIMGETINTERPFRAME(inAnim2, i - inAnim2->offsetInParent),
                                  alpha);
            }

            /* memcpy keys past sub animation interpolator */
            memcpy(rtANIMGETINTERPFRAME(outAnim, inAnim2->offsetInParent+inAnim2->numNodes),
                   rtANIMGETINTERPFRAME(inAnim1, inAnim2->offsetInParent+inAnim2->numNodes),
                   outAnim->currentInterpKeyFrameSize * (outAnim->numNodes-(inAnim2->offsetInParent+inAnim2->numNodes)));
        }
    }

    RWRETURN(TRUE);
}

/**
 * \ingroup rtanim
 * \ref RtAnimInterpolatorAddSubInterpolator adds together two animation
 * interpolators. Each animation interpolator should have an animation attached, and
 * the result will be the addition of the two input animation interpolators. The
 * output animation interpolator must be identical in structure to either the main
 * or sub-animation interpolator.
 *
 * \param outAnim   Pointer to an \ref RtAnimInterpolator in which the result is
 *                  returned.
 * \param mainAnim  Pointer to the parent \ref RtAnimInterpolator.
 * \param subAnim   Pointer to the sub-\ref RtAnimInterpolator.
 * \note \e outAnim, \e mainAnim and \e subAnim MUST be different.
 *
 * \return \ref RwBool, TRUE on success, FALSE otherwise.
 *
 * \see RtAnimAnimationMakeDelta
 *
 */
RwBool
RtAnimInterpolatorAddSubInterpolator(RtAnimInterpolator *outAnim,
                                RtAnimInterpolator *mainAnim,
                                RtAnimInterpolator *subAnim)
{
    RwInt32             i;
    RwInt32             outOffset = 0;

    RWAPIFUNCTION(RWSTRING("RtAnimInterpolatorAddSubInterpolator"));
    RWASSERT(outAnim);
    RWASSERT(mainAnim);
    RWASSERT(subAnim);
    RWASSERT(outAnim->numNodes == mainAnim->numNodes ||
             outAnim->numNodes == subAnim->numNodes);

    if (FALSE == outAnim->isSubInterpolator)
    {
        outOffset = subAnim->offsetInParent;
    }

    for (i = 0; i < outAnim->numNodes; i++)
    {

        RtAnimKeyFrameAddTogether(outAnim,
                                rtANIMGETINTERPFRAME(outAnim,
                                                               i + outOffset),
                                rtANIMGETINTERPFRAME(mainAnim, i + subAnim->offsetInParent),
                                rtANIMGETINTERPFRAME(subAnim, i));
    }

    RWRETURN(TRUE);
}

