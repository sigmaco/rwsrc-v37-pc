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

/**
 * \ingroup rphanim
 * \page rphanimoverview RpHAnim Plugin Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rtquat.h, rtanim.h, rphanim.h
 * \li \b Libraries: rwcore, rpworld, rtquat, rtanim, rphanim
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach,
 * \ref RtAnimInitialize, \ref RpHAnimPluginAttach
 *
 * \subsection hanimoverview Overview
 * The RpHAnim plugin is designed to offer hierarchical animation that can
 * drive RenderWare \ref RwFrame hierarchies and RenderWare skinned (\ref
 * RpSkin) atomics.
 *
 * The main objects managed and used by RpHAnim are the \ref RpHAnimHierarchy
 * and the \ref RtAnimAnimation. The \ref RpHAnimHierarchy data structure
 * describes the order of the joints, the shoulder, elbow and so on, that
 * control the model's animation. Its topology is a tree structure, or
 * hierarchy of nodes that represent the model's animated joints. The \ref
 * RtAnimAnimation holds the state of the animation as it is being run.
 * So it stores the current position of each joint as it is interpolated
 * between key frames at the current point in the animation.
 *
 * A sub-hierarchy can be defined by specifying a node in the main hierarchy.
 * All nodes that are descendants of it (children, grandchildren etc) become
 * part of this sub-hierarchy. This is useful for running more than one
 * animation at a time on a single model. The main hierarchy normally has an
 * animation that can animate the joints of the whole model (or at least the
 * part of the model not animated by a sub-hierarchy). Any sub-hierarchies can
 * have an animation, and this animation replaces the main hierarchy animation
 * for the sub-hierarchy (i.e. that branch of the main hierarchy) to which it
 * is applied. It does this by overwriting the joint matrices held in the main
 * hierarchy data structure directly.
 *
 * For each node (i.e. joint) in the hierarchy (or sub-hierarchy) that the
 * animation is applied to, the \ref RtAnimAnimation contains a start keyframe,
 * an end keyframe, and any number of intermediate keyframes as necessary. Each
 * keyframe consists of a quaternion (the positional information for the joint)
 * and a time, at which the joint is at exactly that position. This is normally
 * created by loading an ANM file. They can be run on multiple hierarchies at
 * once as long as all the hierarchies were created with the same topology.
 * When the joint is at a time where no keyframe is applied the joint is
 * positioned by linear interpolation between the two adjacent keyframes. It is
 * possible to use non-linear interpolations, see overloadable interpolation
 * schemes in \ref rtanim.
 *
 * \ref RpHAnimHierarchy's can be attached and detached from a standard \ref
 * RwFrame hierarchy and when attached flags can be set so that the \ref
 * RwFrame's modeling matrices and/or LTMs (Local Transform Matrices) are
 * updated when the animation is being run. This allows the animation system
 * to provide animation to atomics attached to the \ref RwFrame hierarchy. The
 * \ref RpHAnimHierarchy also (optionally) maintains an array of matrices held
 * as either world space or local hierarchy space LTMs. These are used to
 * drive \ref RpSkin atomics as skinned models. A Hierarchy that is attached
 * to \ref RwFrame, updating \ref RwFrame and attached to a skin will provide
 * animation to both atomic hierarchies and skinned atomics whilst only
 * needing to update a single animation.
 *
 * Alternatively, the \ref RtAnimAnimation system may be completely
 * overridden for animating skinned atomics by accessing the array of
 * matrices in the \ref RpHAnimHierarchy directly. See \ref
 * RpHAnimHierarchyGetMatrixArray in the API Reference Guide.
 *
 * \par Quick how-to:
 * -# Export the animated model with HAnim data to create an \c .rws file
 * containing an \ref rwID_CLUMP and an \ref rwID_HANIMANIMATION chunk
 * representing the model and its animation. For more complex animations, there
 * may be more than one \ref rwID_CLUMP chunk (you only need one main one), and
 * more than one \ref rwID_HANIMANIMATION (animations to play back each hierarchy
 * or subhierarchy) in the \c .rws file.
 * -# Open the .rws for read-only streaming, and find and read the \ref rwID_CLUMP
 * to create a skinned character with the hierarchy in it.
 * -# If necessary, create any sub-hierarchies, by marking the nodes of the
 * main hierarchy to be the roots of the sub-hierarchies. This not required
 * for simple cases.
 * -# Create animation(s) from the \ref rwID_HANIMANIMATION chunk(s) in the \c .rws
 * file.
 * -# Attach the animation to the hierarchy or sub-hierarchy it is intended
 * to animate.
 * -# Start rolling time forward for the hierarchy...
 * -# ...whilst rendering the thing on the screen of course.
 * -# When an animation is finished, load (or use from memory) the next one
 * and blend from the old one to the new one before starting to play the new
 * one in earnest. This will involve creating a blend position from the old
 * and new animations and applying it to the hierarchy.
 *
 * \see \ref rtcmpkey
 */

/**
 * \ingroup rphanimchanges
 * \page rphanimframestonodes Keyframing support moved to RtAnim
 *
 * In order to provide a default keyframing engine, the HAnim plugin had been
 * separated in two differents units : \ref rtanim a keyframing toolkit and
 * \ref rphanim a hierarchical animation plugin.
 *
 * RtAnim is based on \ref RtAnimAnimation, which describes an animation with
 * attached keyframes. To play an animation using \ref rtanim, you'll have to
 * create a \ref RtAnimInterpolator object which will contain the current
 * state of the animation, and the tables needed to apply and interpolate
 * keyframes.
 *
 * \ref rphanim now relies on \ref rtanim to provide it's keyframing and
 * interpolating schemes. Where, in previous versions, a \ref RpHAnimHierarchy
 * held the animation's current state and the hierarchical data, it now
 * contains only a reference to an \ref RtAnimInterpolator object and the
 * hierarchical data (matrix arrays, hierarchy definitions, ...).
 * The update process is let to the \ref RtAnimAnimation user (for \ref rphanim,
 * see \ref RpHAnimHierarchyUpdateMatrices) as the object does not contain any
 * information about the result's data format.
 *
 * A collection of legacy macros and typedefs are provided to ease the move to
 * the new system, though direct access to member variables will need to be
 * updated.
 *
 * The file formats are compatible, and there is no need to re-export data to
 * fit the new system.
 *
 * The main porting tasks are :
 * \li Linking the \ref rtanim library
 * \li Calling \ref RtAnimInitialize before \ref RpHAnimPluginAttach.
 *
 *
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


#define RPHANIMFRAMEGETDATA(frame)                      \
    ( (void *)(((RwUInt8 *)frame) +                     \
               RpHAnimAtomicGlobals.engineOffset) )

#define RPHANIMFRAMEGETCONSTDATA(frame)                         \
    ( (const void *)(((const RwUInt8 *)frame) +                 \
                     RpHAnimAtomicGlobals.engineOffset) )

/****************************************************************************
 Defines
 */

#if (0 && defined (RWDEBUG) && !defined(MEMORYMESSAGE))
#define MEMORYMESSAGE(_x) RWMESSAGE(_x)
#endif /* (0 && defined (RWDEBUG)) */

#if (!defined(MEMORYMESSAGE))
#define MEMORYMESSAGE(_x)      /* No op */
#endif /* (!defined(MEMORYMESSAGE)) */

/****************************************************************************
 Local types
 */

typedef struct FrameLinkToHierarchyData FrameLinkToHierarchyData;
struct FrameLinkToHierarchyData
{
        RpHAnimHierarchy *pHierarchy;
        RwInt32 id;
};

#define HANIMMATRIXSTACKHEIGHT 32

/****************************************************************************
 Local Defines
 */

#define _EPSILON          ((RwReal)(0.001))
#define _TOL_COS_ZERO     (((RwReal)1) - _EPSILON)
#define RwCosec(_x) \
   ( ((RwReal)1) / RwSin((_x)) )
#define ROUNDUP16(x)      (((RwUInt32)(x) + 16 - 1) & ~(16 - 1))

/* Default frame ID */
#define rpHANIMDEFAULTFRAMEID -1

/****************************************************************************
 Local (static) globals
 */

#if (defined(RWDEBUG))
long                rpHANIMStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

/****************************************************************************
 Globals
 */

RpHAnimAtomicGlobalVars RpHAnimAtomicGlobals;

/****************************************************************************
 Local Function Prototypes
 */

/****************************************************************************
 Functions
 */
static RwFrame *
HAnimFramesLinkToHierarchy(RwFrame *rootFrame, void *pData)
{
    RpHAnimHierarchy *pHierarchy = (RpHAnimHierarchy *)pData;
    int i;
    RWFUNCTION(RWSTRING("HAnimFramesLinkToHierarchy"));
    RWASSERT(rootFrame);
    RWASSERT(pData);

    for (i=0; i<pHierarchy->numNodes; ++i)
    {
        if (RpHAnimFrameGetID(rootFrame) == pHierarchy->pNodeInfo[i].nodeID)
        {
            pHierarchy->pNodeInfo[i].pFrame = rootFrame;
        }
    }

    RwFrameUpdateObjects(rootFrame);
    RwFrameForAllChildren(rootFrame, HAnimFramesLinkToHierarchy, pData);

    RWRETURN(rootFrame);
}

static RwFrame *
HAnimFrameLinkToHierarchy(RwFrame *rootFrame, void *pData)
{
    FrameLinkToHierarchyData *fdata = (FrameLinkToHierarchyData *)pData;
    RpHAnimHierarchy *pHierarchy = fdata->pHierarchy;
    RwInt32 i;

    RWFUNCTION(RWSTRING("HAnimFrameLinkToHierarchy"));
    RWASSERT(rootFrame);
    RWASSERT(pData);

    if (RpHAnimFrameGetID(rootFrame) == fdata->id)
    {
        for (i=0; i<pHierarchy->numNodes; ++i)
        {
            if (pHierarchy->pNodeInfo[i].nodeID == fdata->id)
            {
                pHierarchy->pNodeInfo[i].pFrame = rootFrame;
            }
        }
    }

    RwFrameUpdateObjects(rootFrame);
    RwFrameForAllChildren(rootFrame, HAnimFrameLinkToHierarchy, pData);

    RWRETURN(rootFrame);
}


/*----------------------------------------------------------------------*/

/*                              - ENGINE -                              */

/*----------------------------------------------------------------------*/

static RwInt32 _rpHAnimHierarchyFreeListBlockSize = 128,
               _rpHAnimHierarchyFreeListPreallocBlocks = 1;
static RwFreeList _rpHAnimHierarchyFreeList;

/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchySetFreeListCreateParams allows the developer to specify
 * how many \ref RpHAnimHierarchy s to preallocate space for.
 * Call before \ref RwEngineInit.
 *
 * \param blockSize  number of entries per freelist block.
 * \param numBlocksToPrealloc  number of blocks to allocate on
 * \ref RwFreeListCreateAndPreallocateSpace.
 *
 * \see RwFreeList
 *
 */
void
RpHAnimHierarchySetFreeListCreateParams(RwInt32 blockSize,
                                        RwInt32 numBlocksToPrealloc)
{
#if 0
    /*
     * Can not use debugging macros since the debugger is not initialized before
     * RwEngineInit.
     */
    /* Do NOT comment out RWAPIFUNCTION as gnumake verify will not function */
    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchySetFreeListCreateParams"));
#endif
    _rpHAnimHierarchyFreeListBlockSize = blockSize;
    _rpHAnimHierarchyFreeListPreallocBlocks = numBlocksToPrealloc;
#if 0
    RWRETURNVOID();
#endif
}




static void        *
HAnimOpen(void *instance,
          RwInt32 __RWUNUSED__ offset,
          RwInt32 __RWUNUSED__ size)
{
    RtAnimInterpolatorInfo     interpInfo;

    RWFUNCTION(RWSTRING("HAnimOpen"));
    RWASSERT(instance);

    {
        RpHAnimAtomicGlobals.HAnimFreeList =
            RwFreeListCreateAndPreallocateSpace(sizeof(RpHAnimHierarchy),
            _rpHAnimHierarchyFreeListBlockSize, sizeof(RwInt32),
            _rpHAnimHierarchyFreeListPreallocBlocks, &_rpHAnimHierarchyFreeList,
            rwID_HANIMPLUGIN | rwMEMHINTDUR_GLOBAL);

        if (NULL == RpHAnimAtomicGlobals.HAnimFreeList)
        {
            instance = NULL;
        }
    }

    /* setup our default interpolation scheme */
    interpInfo.typeID = rpHANIMSTDKEYFRAMETYPEID;
    interpInfo.interpKeyFrameSize = sizeof(RpHAnimKeyFrame);
    interpInfo.animKeyFrameSize = sizeof(RpHAnimKeyFrame);
    interpInfo.keyFrameApplyCB = RpHAnimKeyFrameApply;
    interpInfo.keyFrameBlendCB = RpHAnimKeyFrameBlend;
    interpInfo.keyFrameInterpolateCB = RpHAnimKeyFrameInterpolate;
    interpInfo.keyFrameAddCB = RpHAnimKeyFrameAdd;
    interpInfo.keyFrameMulRecipCB = RpHAnimKeyFrameMulRecip;
    interpInfo.keyFrameStreamReadCB = RpHAnimKeyFrameStreamRead;
    interpInfo.keyFrameStreamWriteCB = RpHAnimKeyFrameStreamWrite;
    interpInfo.keyFrameStreamGetSizeCB = RpHAnimKeyFrameStreamGetSize;
    interpInfo.customDataSize = 0;

    RtAnimRegisterInterpolationScheme(&interpInfo);

    RWRETURN(instance);

}

static void        *
HAnimClose(void *instance,
           RwInt32 __RWUNUSED__ offset,
           RwInt32 __RWUNUSED__ size)
{
    RWFUNCTION(RWSTRING("HAnimClose"));
    RWASSERT(instance);

    if (NULL != RpHAnimAtomicGlobals.HAnimFreeList)
    {
        RwFreeListDestroy(RpHAnimAtomicGlobals.HAnimFreeList);
        RpHAnimAtomicGlobals.HAnimFreeList = (RwFreeList *)NULL;
    }

    RWRETURN(instance);
}


/*----------------------------------------------------------------------*/

/*                             - ATOMIC -                               */

/*----------------------------------------------------------------------*/

static void        *
HAnimConstructor(void *object,
                 RwInt32 __RWUNUSED__ offset,
                 RwInt32 __RWUNUSED__ size)
{
    RpHAnimFrameExtension *frameExt;
    RWFUNCTION(RWSTRING("HAnimConstructor"));
    RWASSERT(object);

    frameExt = (RpHAnimFrameExtension *)RPHANIMFRAMEGETDATA(object);
    frameExt->hierarchy = (RpHAnimHierarchy *)NULL;
    frameExt->id        = rpHANIMDEFAULTFRAMEID;

    RWRETURN(object);
}

static void        *
HAnimDestructor(void *object,
                RwInt32 __RWUNUSED__ offset,
                RwInt32 __RWUNUSED__ size)
{
    RpHAnimFrameExtension *frameExt;

    RWFUNCTION(RWSTRING("HAnimDestructor"));
    RWASSERT(object);

    frameExt = (RpHAnimFrameExtension *)RPHANIMFRAMEGETDATA(object);
    if (frameExt->hierarchy)
    {
        RpHAnimHierarchy *pHierarchy = frameExt->hierarchy;
        RwInt32 frameNum;
        /* run through all the frames this hierarchy
           points at since they also point back at the
           hierarchy. Set the pointers to NULL and then
           destroy the hierarchy */
        for (frameNum = 0; frameNum < pHierarchy->numNodes; frameNum++)
        {
            if (pHierarchy->pNodeInfo[frameNum].pFrame)
            {
                /* TODO:
                   In here we ultimately check if this pointer is a sub-hierarchy
                   root, if so destroy the sub-hierarchy */
            }

            pHierarchy->pNodeInfo[frameNum].pFrame = (RwFrame *)NULL;
        }

        if (pHierarchy->parentFrame == (RwFrame *)object)
        {
            RpHAnimHierarchyDestroy(pHierarchy);
        }

        frameExt->hierarchy = (RpHAnimHierarchy *)NULL;
    }
    frameExt->id = rpHANIMDEFAULTFRAMEID;

    RWRETURN(object);
}

static void        *
HAnimCopy(void *dstObject,
          const void * srcObject,
          RwInt32 __RWUNUSED__ offset,
          RwInt32 __RWUNUSED__ size)
{
    const RpHAnimFrameExtension *srcFrameExt;
    RpHAnimFrameExtension *dstFrameExt;

    RWFUNCTION(RWSTRING("HAnimCopy"));
    RWASSERT(srcObject);
    RWASSERT(dstObject);

    srcFrameExt = (const RpHAnimFrameExtension *)
        RPHANIMFRAMEGETCONSTDATA(srcObject);
    dstFrameExt = (RpHAnimFrameExtension *)
        RPHANIMFRAMEGETDATA(dstObject);

    dstFrameExt->id = srcFrameExt->id;
    if (srcFrameExt->hierarchy)
    {
        RpHAnimHierarchy *srcHierarchy = srcFrameExt->hierarchy;
        if (!(srcHierarchy->flags & rpHANIMHIERARCHYSUBHIERARCHY))
        {
            RpHAnimHierarchy *dstHierarchy;
            RwInt32 i;

            dstHierarchy =
                RpHAnimHierarchyCreate(srcHierarchy->numNodes,
                                    (RwUInt32 *)NULL,
                                    (RwInt32 *)NULL,
                                    (RpHAnimHierarchyFlag)srcHierarchy->flags,
                                    srcHierarchy->currentAnim->maxInterpKeyFrameSize);

            for(i=0; i<dstHierarchy->numNodes; ++i)
            {
                dstHierarchy->pNodeInfo[i].pFrame = (RwFrame *)NULL;
                dstHierarchy->pNodeInfo[i].flags = srcHierarchy->pNodeInfo[i].flags;
                dstHierarchy->pNodeInfo[i].nodeIndex = srcHierarchy->pNodeInfo[i].nodeIndex;
                dstHierarchy->pNodeInfo[i].nodeID = srcHierarchy->pNodeInfo[i].nodeID;
            }

            dstFrameExt->hierarchy = dstHierarchy;
            dstHierarchy->parentFrame = (RwFrame *)dstObject;
        }
    }

    RWRETURN(dstObject);
}

/****************************************************************************
 HAnimWrite

 Writes animation hierarchy data to a stream

 Inputs :   RwStream *  Stream to write to
            RwInt32     Size of meshed when serialised (in bytes) (not used)
            void *      Object (atomic)
            RwInt32     Plugin data offset (not used)
            RwInt32     Plugin data size (not used)
 Outputs:   RwStream *  Stream pointer on success
 */

static RwStream    *
HAnimWrite(RwStream * stream,
           RwInt32 __RWUNUSED__ binaryLength,
           const void *object,
           RwInt32 __RWUNUSED__ offsetInObject,
           RwInt32 __RWUNUSED__  sizeInObject)
{
    RwInt32             i;
    const RpHAnimFrameExtension *frameExt;
    RpHAnimNodeInfo     *pNodeInfo;
    RpHAnimHierarchy    *animHierarchy;
    RwInt32              streamVersion = rpHANIMSTREAMCURRENTVERSION;

    RWFUNCTION(RWSTRING("HAnimWrite"));
    RWASSERT(stream);
    RWASSERT(object);

    if (!RwStreamWriteInt
        (stream, (RwInt32 *) &streamVersion, sizeof(RwInt32)))
    {
        RWRETURN((RwStream *)NULL);
    }

    frameExt = (const RpHAnimFrameExtension *)RPHANIMFRAMEGETCONSTDATA(object);

    if (!RwStreamWriteInt
        (stream, (const RwInt32 *) &frameExt->id, sizeof(RwInt32)))
    {
        RWRETURN((RwStream *)NULL);
    }

    animHierarchy = frameExt->hierarchy;

    /* is this a valid root hierarchy */
    if (animHierarchy &&
        !(animHierarchy->flags & rpHANIMHIERARCHYSUBHIERARCHY))
    {
        /* Output hierarchy paramters - numNodes, totalVertices, vertex weights, vertex matrix indices */
        if (!RwStreamWriteInt
            (stream, (const RwInt32 *) &animHierarchy->numNodes,
             sizeof(RwInt32)))
        {
            RWRETURN((RwStream *)NULL);
        }

        /* write out the flags */
        if (!RwStreamWriteInt
            (stream, (const RwInt32 *) &animHierarchy->flags,
             sizeof(RwInt32)))
        {
            RWRETURN((RwStream *)NULL);
        }

        /* write out the maxInterpKeyFrameSize */
        if (!RwStreamWriteInt
            (stream, (const RwInt32 *) &animHierarchy->currentAnim->maxInterpKeyFrameSize,
             sizeof(RwInt32)))
        {
            RWRETURN((RwStream *)NULL);
        }

        /* Write the node info */

        pNodeInfo = animHierarchy->pNodeInfo;

        for (i = 0; i < animHierarchy->numNodes; ++i)
        {
            if (!RwStreamWriteInt
                (stream, (RwInt32 *) & pNodeInfo->nodeID,
                 sizeof(RwInt32)))
            {
                RWRETURN((RwStream *)NULL);
            }

            if (!RwStreamWriteInt
                (stream, (RwInt32 *) & pNodeInfo->nodeIndex,
                 sizeof(RwInt32)))
            {
                RWRETURN((RwStream *)NULL);
            }

            if (!RwStreamWriteInt
                (stream, (RwInt32 *) & pNodeInfo->flags,
                 sizeof(RwInt32)))
            {
                RWRETURN((RwStream *)NULL);
            }

            pNodeInfo++;
        }

    }
    else
    {
        RwInt32 numNodes = 0;

        if (!RwStreamWriteInt
            (stream, (const RwInt32 *) &numNodes,
             sizeof(RwInt32)))
        {
            RWRETURN((RwStream *)NULL);
        }
    }

    RWRETURN(stream);
}

/****************************************************************************
 HAnimRead

 Reads a frames animation hierarchy data from a stream

 Inputs :   RwStream *  Stream to read from
            RwInt32     Size of animation hierarchy data (in bytes)
            void *      Object (atomic)
            RwInt32     Plugin data offset (not used)
            RwInt32     Plugin data size (not used)
 Outputs:   RwStream *  Stream pointer on success
 */

static RwStream    *
HAnimRead(RwStream * stream,
          RwInt32 __RWUNUSED__ binaryLength,
          void *object,
          RwInt32 __RWUNUSED__ offsetInObject,
          RwInt32 __RWUNUSED__ sizeInObject)
{
    RwInt32             i;
    RwInt32             numNodes;
    RwInt32             streamVersion;
    RpHAnimNodeInfo     *pNodeInfo;
    RpHAnimHierarchy      *pHierarchy;
    RpHAnimFrameExtension *frameExt = (RpHAnimFrameExtension *)RPHANIMFRAMEGETDATA(object);

    RWFUNCTION(RWSTRING("HAnimRead"));
    RWASSERT(stream);
    RWASSERT(object);

    if (!RwStreamReadInt
        (stream, (RwInt32 *) &streamVersion, sizeof(RwInt32)))
    {
        RWRETURN((RwStream *)NULL);
    }

    if (streamVersion != 0x100)
    {
        RWRETURN((RwStream *)NULL);
    }

    if (!RwStreamReadInt
        (stream, (RwInt32 *) &frameExt->id, sizeof(RwInt32)))
    {
        RWRETURN((RwStream *)NULL);
    }

    if (!RwStreamReadInt
        (stream, (RwInt32 *) &numNodes, sizeof(RwInt32)))
    {
        RWRETURN((RwStream *)NULL);
    }

    if (numNodes > 0)
    {
        RpHAnimHierarchyFlag flags;
        RwInt32 maxInterpKeyFrameSize;

        if (!RwStreamReadInt
            (stream, (RwInt32 *) &flags, sizeof(RwInt32)))
        {
            RWRETURN((RwStream *)NULL);
        }

        if (!RwStreamReadInt
            (stream, (RwInt32 *) &maxInterpKeyFrameSize, sizeof(RwInt32)))
        {
            RWRETURN((RwStream *)NULL);
        }

        /* we know the maxInterpKeyFrameSize now so allocate a structure of appropriate
           size */
        pHierarchy = (RpHAnimHierarchy *)
                RwFreeListAlloc(RpHAnimAtomicGlobals.HAnimFreeList,
                    rwID_HANIMPLUGIN | rwMEMHINTDUR_EVENT);

        memset(pHierarchy, 0, sizeof(RpHAnimHierarchy));

        RWASSERT(pHierarchy);

        if (pHierarchy)
        {
            void *ptr = NULL;

            pHierarchy->currentAnim = RtAnimInterpolatorCreate(numNodes,maxInterpKeyFrameSize);

            pHierarchy->flags = flags;
            pHierarchy->parentFrame = (RwFrame *)object;
            /* Input hierarchy paramters - numNodes, totalVertices, vertex weights, vertex matrix indices */
            pHierarchy->numNodes = numNodes;

            /* We'll always default the current key size to the max size */

            /* And root hierarchies point to themselves, we don't
               yet support streaming sub-hierarachies so this must be a root */
            pHierarchy->parentHierarchy = pHierarchy;

            /* Read the node info */

            if (pHierarchy->flags & rpHANIMHIERARCHYNOMATRICES)
            {
                pHierarchy->pMatrixArray = (RwMatrix *) NULL;
                pHierarchy->pMatrixArrayUnaligned = NULL;
            }
            else
            {
                ptr = RwMalloc(sizeof(RwMatrix) * numNodes + 15,
                    rwID_HANIMPLUGIN | rwMEMHINTDUR_EVENT);
                pHierarchy->pMatrixArray = (RwMatrix *) ROUNDUP16(ptr);
                pHierarchy->pMatrixArrayUnaligned = ptr;
            }

            pHierarchy->pNodeInfo = (RpHAnimNodeInfo *) RwMalloc(sizeof(RpHAnimNodeInfo) * numNodes,
                                                         rwID_HANIMPLUGIN | rwMEMHINTDUR_EVENT);

            pNodeInfo = pHierarchy->pNodeInfo;

            for (i = 0; i < pHierarchy->numNodes; ++i)
            {
                if (!RwStreamReadInt
                    (stream, (RwInt32 *) & pNodeInfo->nodeID,
                     sizeof(RwInt32)))
                {
                    RWRETURN((RwStream *)NULL);
                }

                if (!RwStreamReadInt
                    (stream, (RwInt32 *) & pNodeInfo->nodeIndex,
                     sizeof(RwInt32)))
                {
                    RWRETURN((RwStream *)NULL);
                }

                if (!RwStreamReadInt
                    (stream, (RwInt32 *) & pNodeInfo->flags,
                     sizeof(RwInt32)))
                {
                    RWRETURN((RwStream *)NULL);
                }

                pNodeInfo->pFrame = (RwFrame *)NULL;

                ++pNodeInfo;
            }

        }
        frameExt->hierarchy = pHierarchy;
    }

    RWRETURN(stream);
}

/****************************************************************************
 HAnimSize

 Gets the size of a frames animation hierarchy (when serialized)

 Inputs :   void *      Object (atomic)
            RwInt32     Plugin data offset (not used)
            RwInt32     Plugin data size (not used)
 Outputs:   RwInt32     Size of mesh when serialised (in bytes)
 */

static              RwInt32
HAnimSize(const void *object,
          RwInt32 __RWUNUSED__ offsetInObject,
          RwInt32 __RWUNUSED__ sizeInObject)
{
    RwBool              needToStream;

    const RpHAnimFrameExtension *frameExt = (const RpHAnimFrameExtension *)
        RPHANIMFRAMEGETCONSTDATA(object);

    RWFUNCTION(RWSTRING("HAnimSize"));

    /* Frame needs extension data only if it has an id or hierarchy */
    needToStream =    (rpHANIMDEFAULTFRAMEID != frameExt->id )
                   || (frameExt->hierarchy);

    if (needToStream)
    {
        RwInt32 size;

        size = sizeof(RwInt32);        /* Stream version no */
        size += sizeof(RwInt32);       /* frame ID */
        size += sizeof(RwInt32);       /* Num frames in hierarchy (0 no hierarchy) */

        if (frameExt->hierarchy &&
            !(frameExt->hierarchy->flags & rpHANIMHIERARCHYSUBHIERARCHY))
        {
            size += sizeof(RwInt32);   /* Hierarchy flags for recreation */
            size += sizeof(RwInt32);   /* Hierarchy max keyframe size */
            size +=
                frameExt->hierarchy->numNodes * (sizeof(RwInt32) * 3);
        }

        RWRETURN(size);
    }

    /* By default, do not need to stream extension data */
    RWRETURN(0);
}


/**
 * \ingroup rphanim
 * \ref RpHAnimPluginAttach
 *  is used to attach the Animation plugin to the
 * RenderWare system to enable the manipulation of
 * animating node hierarchies.
 * The plugin must be attached between initializing the system
 * with \ref RwEngineInit
 * and opening it with \ref RwEngineOpen.
 *
 * \return TRUE if successful, FALSE if an error occurs.
 *
 */

RwBool
RpHAnimPluginAttach(void)
{
    RwInt32             success = 0;
    RWAPIFUNCTION(RWSTRING("RpHAnimPluginAttach"));

    /* Register the plugIn */

    if (RwEngineRegisterPlugin(0,
                               rwID_HANIMPLUGIN,
                               HAnimOpen, HAnimClose) < 0)
    {
        RWRETURN(FALSE);
    }

    /* Extend RwFrames to hold ID and hierarchy ptr */
    RpHAnimAtomicGlobals.engineOffset =
        RwFrameRegisterPlugin(sizeof(RpHAnimFrameExtension),
                              rwID_HANIMPLUGIN,
                              HAnimConstructor, HAnimDestructor,
                              HAnimCopy);

    /* Attach the stream handling functions */
    success =
        RwFrameRegisterPluginStream(rwID_HANIMPLUGIN,
                                    HAnimRead, HAnimWrite, HAnimSize);

    RWRETURN((success >= 0) && (RpHAnimAtomicGlobals.engineOffset >= 0));
}


/*----------------------------------------------------------------------*/

/*                              - Hierarchy -                           */

/*                                 DEBUG                                */

/*----------------------------------------------------------------------*/
#if (defined(RWDEBUG))

/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchySetFlags
 * allows you to set flags defining how the \ref RpHAnimHierarchy was created
 * and how it should be updated. Only the flags defining update modes should
 * be changed after creation.
 *
 * \param hierarchy A pointer to the hierarchy to set the flags on
 * \param flags Flags to set on the hierarchy
 *
 * \return A pointer to the hierarchy on success
 *
 * \see RpHAnimHierarchyGetFlags
 */
RpHAnimHierarchy *
RpHAnimHierarchySetFlags(RpHAnimHierarchy *hierarchy,
                         RpHAnimHierarchyFlag flags)
{
    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchySetFlags"));
    RWASSERT(hierarchy);

    RpHAnimHierarchySetFlagsMacro(hierarchy, flags);

    RWRETURN(hierarchy);
}

/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchyGetFlags
 * allows you flags defining how the \ref RpHAnimHierarchy was created and
 * how it should be updated. Only the flags defining update modes should
 * be changed after creation.
 *
 * \param hierarchy A pointer to the hierarchy to get the flags from.
 *
 * \return the flags set on the hierarchy
 *
 * \see RpHAnimHierarchySetFlags
 */
RpHAnimHierarchyFlag
RpHAnimHierarchyGetFlags(RpHAnimHierarchy *hierarchy)
{
    RpHAnimHierarchyFlag result;

    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchyGetFlags"));
    RWASSERT(hierarchy);

    result = (RpHAnimHierarchyFlag)
        RpHAnimHierarchyGetFlagsMacro(hierarchy);

    RWRETURN(result);
}

#endif /* (defined(RWDEBUG)) */

/*----------------------------------------------------------------------*/

/*                              - Hierarchy -                           */

/*----------------------------------------------------------------------*/

/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchyCreate
 * creates and returns a standard RpHAnimHierarchy . This takes the number of
 * nodes in the hierarchy and arrays of push/pop node flags and node IDs. It also
 * takes hierarchy flags and a maximum keyframe size.
 *
 * The flags are used to define certain creation time aspects and update aspects
 * of a hierarchy (see below for description)
 *
 * \param numNodes          The number of nodes required in the hierarchy.
 * \param nodeFlags         The push/pop flags should be setup such that any
 *                          node with no children has a POP flag and all but
 *                          the last sibling of a level of nodes has a push
 *                          flag.
 * \param nodeIDs           The nodeIDs are one per node and should match up
 *                          to the ids set on RwFrames with
 *                          \ref RpHAnimFrameSetID, if these are setup matching
 *                          \ref RpHAnimHierarchyAttach is able to attach all
 *                          the RwFrames to allow their update.
 * \param flags             Defines the type and specific properties of the
 *                          \ref RpHAnimHierarchy
 * \param maxInterpKeyFrameSize   Defines the size of the biggest type of keyframe
 *                          you wish to use on this hierarchy, currently there
 *                          is only one keyframe type whose size is
 *                          rpHANIMSTDKEYFRAMESIZE. Overloaded interpolation
 *                          schemes will be available which may use larger
 *                          more complex schemes or support compressed data.
 *
 * The flags include:
 *
 * Creation time flags:
 *
 * \li rpHANIMHIERARCHYSUBHIERARCHY This defines that this hierarchy is a sub-hierarchy,
 * meaning that it represents a branch of a full hierarchy. The sub-hierarchy is linked
 * to the main hierarchy by the matrix array. This means the sub-hierarchy holds it's own
 * arrays of interpolated keys and only updates the main hierarchy when you update the
 * sub-hierarchy matrix array.
 * \li rpHANIMHIERARCHYNOMATRICES this flag specifies that the hierarchy should be created
 * with no local matrix array. This type of hierarchy will directly update an RwFrame hierarchy
 * but not store extra matrices. Skinning will then extract the matrices it needs from RwFrame,
 * this is slightly less efficient but will save memory.
 *
 * Update flags
 *
 * \li rpHANIMHIERARCHYUPDATEMODELLINGMATRICES
 * \li rpHANIMHIERARCHYUPDATELTMS
 * These two flags specify whether a hierarchy should update the modeling matrices and/or
 * LTMs in an RwFrame hierarchy, for the RwFrames to be updated you must first attach the hierarchy
 * using \ref RpHAnimHierarchyAttach. The choice of which matrices in RwFrames to update is down to the
 * applications use of the results. If  you are likely to modify the RwFrames yourself forcing
 * hierarchies to be resynchronized it's important to update the modeling matrices since otherwise
 * the resynchronized hierarchies will contain the wrong LTMs. If you are going to modify lots of
 * the hierarchy or root nodes it's probably sensible just to update the modeling matrices.
 * If you plan on only updating a few lower down matrices (procedural IK on arms/legs etc)
 * you can update both sets of matrices this way only those you procedurally modify will be
 * resynchronized. If you can guarantee no updates between animation and usage of the RwFrames in
 * rendering you can update only the LTMs.
 * \li  rpHANIMHIERARCHYLOCALSPACEMATRICES  this flag causes the local copy of
 * the matrices to be updated as LTMs relative to the \ref RwFrame the
 * hierarchy is attached to. This will provide the most optimal method
 * for animating skinned objects with \ref RpSkin but will add slightly more
 * complication to any procedural animation you may wish to apply.
 *
 *
 * \return A pointer to the new hierarchy on success, NULL if there is an error.
 *
 * \see RpHAnimHierarchyDestroy
 * \see RtAnimInterpolatorCreate
 * \see RtAnimInterpolatorDestroy
 */

RpHAnimHierarchy *
RpHAnimHierarchyCreate(RwInt32 numNodes, RwUInt32 *nodeFlags, RwInt32 *nodeIDs,
                       RpHAnimHierarchyFlag flags, RwInt32 maxInterpKeyFrameSize)
{
    void               *ptr;
    RpHAnimHierarchy    *pHierarchy;
    int                 node;

    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchyCreate"));
    RWASSERT(numNodes >= 1);

    ptr = RwFreeListAlloc(RpHAnimAtomicGlobals.HAnimFreeList,
        rwID_HANIMPLUGIN | rwMEMHINTDUR_EVENT);

    pHierarchy = (RpHAnimHierarchy *) ptr;

    pHierarchy->currentAnim = RtAnimInterpolatorCreate(numNodes,maxInterpKeyFrameSize);

    pHierarchy->flags = flags;
    pHierarchy->numNodes = numNodes;
    pHierarchy->parentFrame = (RwFrame *)NULL;

    if (!(flags & rpHANIMHIERARCHYNOMATRICES))
    {
        ptr = RwMalloc(sizeof(RwMatrix) * numNodes + 15,
            rwID_HANIMPLUGIN | rwMEMHINTDUR_EVENT);
        pHierarchy->pMatrixArray = (RwMatrix *) ROUNDUP16(ptr);
        pHierarchy->pMatrixArrayUnaligned = ptr;
    }
    else
    {
        pHierarchy->pMatrixArray = NULL;
        pHierarchy->pMatrixArrayUnaligned = NULL;
    }


    pHierarchy->pNodeInfo = (RpHAnimNodeInfo *)  RwMalloc(sizeof(RpHAnimNodeInfo) * numNodes,
                                                      rwID_HANIMPLUGIN | rwMEMHINTDUR_EVENT);

    for (node = 0; node < numNodes; ++node)
    {
        pHierarchy->pNodeInfo[node].pFrame = (RwFrame *)NULL;
        if (nodeIDs)
        {
            pHierarchy->pNodeInfo[node].nodeID = nodeIDs[node];
        }
        pHierarchy->pNodeInfo[node].nodeIndex = node;
        if (nodeFlags)
        {
            pHierarchy->pNodeInfo[node].flags = nodeFlags[node];
        }
    }

    /* root/main hierarchies point to themselves with a zero offset */
    pHierarchy->parentHierarchy = pHierarchy;

    RWRETURN(pHierarchy);
}


/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchyDestroy
 *  destroys a hierarchy
 *
 * \param hierarchy A pointer to the hierarchy to be destroyed.
 *
 * \return NULL
 *
 * \see RpHAnimHierarchyCreate
 * \see RtAnimInterpolatorCreate
 * \see RtAnimInterpolatorDestroy
 *
 */
RpHAnimHierarchy *
RpHAnimHierarchyDestroy(RpHAnimHierarchy *hierarchy)
{
    RwFrame *parentFrame;

    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchyDestroy"));
    RWASSERT(NULL != hierarchy);

    parentFrame = hierarchy->parentFrame;
    if (!(hierarchy->flags & rpHANIMHIERARCHYSUBHIERARCHY))
    {
        if (!(hierarchy->flags & rpHANIMHIERARCHYNOMATRICES))
        {
            RwFree(hierarchy->pMatrixArrayUnaligned);
        }
        RwFree(hierarchy->pNodeInfo);
    }

    hierarchy->pMatrixArrayUnaligned = NULL;
    hierarchy->pMatrixArray = (RwMatrix *)NULL;
    hierarchy->pNodeInfo = (RpHAnimNodeInfo *)NULL;
	//@{ Jaewon 20050511
	// Sync 'numNodes' with 'pNodeInfo'...=0=
	hierarchy->numNodes = 0;
	//@} Jaewon

    MEMORYMESSAGE(("RwFree(%p)", hierarchy));


    RtAnimInterpolatorDestroy(hierarchy->currentAnim);

    RwFreeListFree(RpHAnimAtomicGlobals.HAnimFreeList,hierarchy);
    hierarchy = (RpHAnimHierarchy *)NULL;

    if (parentFrame)
    {
        RpHAnimFrameExtension *frameExt =
            (RpHAnimFrameExtension *)RPHANIMFRAMEGETDATA(parentFrame);
        frameExt->hierarchy = (RpHAnimHierarchy *)NULL;
    }

    RWRETURN(hierarchy);
}


/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchyCreateSubHierarchy
 * This function creates a sub-hierarchy of a given main hierarchy. The
 * parent-hierarchy and the offset into the hierarchy are given along with
 * flags defining the sub-hierarchies settings. You can specify a different
 * maxkeyframesize for the sub-hierarchy and use different types of keyframe
 * on sub-hierarchies and main hierarchies, specifying -1 will adopt the
 * parent hierarchies size settings.
 *
 * The sub-hierarchy can then be used to run a different animation from the
 * base hierarchy. The general loop process would be as follows.
 *
 * \li \ref RpHAnimHierarchyAddAnimTime (mainHierarchy, time);
 * \li \ref RpHAnimHierarchyUpdateMatrices (mainHierarchy);
 * \li \ref RpHAnimHierarchyAddAnimTime (subHierarchy, time);
 * \li \ref RpHAnimHierarchyUpdateMatrices (subHierarchy);
 *
 * Here we update the main hierarchy interpolated keyframe array
 * and then convert the keyframes into the matrix array. We then update the
 * sub-hierarchies keyframes locally to the sub-hierarchy and the update
 * matrices overwrites the appropriate matrices in the main hierarchy, because
 * of this you cannot update LTM's of a sub-hierarchy if the main hierarchy LTM's
 * were not updated.
 *
 * \param parentHierarchy the parent hierarchy to be used for this sub-hierarchy
 * \param startNode the index of the node representing the root of the new
 *        sub-hierarchy
 * \param flags the flags to be used in the new sub-hierarchy, these can be copied
 *        from the source hierarchy.
 * \param maxInterpKeyFrameSize the maximum keyframe size to allow in the sub-hierarchy,
 *        passing -1 will copy the parent hierarchies settings.
 *
 * \return A pointer to the new sub-hierarchy on success
 *
 * \see RtAnimInterpolatorCreateSubInterpolator
 *
 */
RpHAnimHierarchy     *
RpHAnimHierarchyCreateSubHierarchy(RpHAnimHierarchy *parentHierarchy,
                                   RwInt32 startNode,
                                   RpHAnimHierarchyFlag flags,
                                   RwInt32 maxInterpKeyFrameSize)
{
    void               *ptr;
    RpHAnimHierarchy    *pHierarchy;
    RwInt32             numNodes;
    RwInt32             framesOnStack;
    RpHAnimNodeInfo    *pNodeInfo;

    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchyCreateSubHierarchy"));
    RWASSERT(parentHierarchy);
    RWASSERT(parentHierarchy->numNodes > startNode);

    /* calculate the size of the sub hierarchy, by running through the
       push pop flags in the parent hierarchy */
    numNodes = 1;
    framesOnStack = 0;
    pNodeInfo = &parentHierarchy->pNodeInfo[startNode];
    if (!(pNodeInfo->flags & rpHANIMPOPPARENTMATRIX))
    {
        framesOnStack = 1;
        ++pNodeInfo;
    }
    while (framesOnStack)
    {
        if (pNodeInfo->flags & rpHANIMPUSHPARENTMATRIX)
        {
            ++framesOnStack;
        }
        if (pNodeInfo->flags & rpHANIMPOPPARENTMATRIX)
        {
            --framesOnStack;
        }
        ++numNodes;
        ++pNodeInfo;
    }

    ptr = RwFreeListAlloc(RpHAnimAtomicGlobals.HAnimFreeList,
                          rwID_HANIMPLUGIN | rwMEMHINTDUR_EVENT);

    pHierarchy = (RpHAnimHierarchy *) ptr;
    pHierarchy->flags = flags | rpHANIMHIERARCHYSUBHIERARCHY;
    pHierarchy->numNodes = numNodes;

    pHierarchy->currentAnim = RtAnimInterpolatorCreateSubInterpolator(
                                        parentHierarchy->currentAnim,
                                        startNode,
                                        numNodes,
                                        maxInterpKeyFrameSize);

    /* TODO: Should this find the correct frame? If the parent hierarchy is
       unattached */

    /* find the offset in the parent hierarchy of the parent of the sub
       hierarchy's root node */
    if (startNode == 0)
    {
        pHierarchy->rootParentOffset = -1;
    }
    else
    {
        RwInt32 i;
        RwInt32 pNodeStack[32];
        RwInt32 *pNodeStackPtr = &pNodeStack[0];
        RwInt32 parentNode = -1;

        for (i = 0; i < startNode; ++i)
        {
            if (rpHANIMPUSHPARENTMATRIX & parentHierarchy->pNodeInfo[i].flags)
            {
                *pNodeStackPtr = parentNode;
                ++pNodeStackPtr;
            }

            parentNode = ( (rpHANIMPOPPARENTMATRIX & parentHierarchy->pNodeInfo[i].flags) ?
                      *(--pNodeStackPtr):
                      i );
        }

        pHierarchy->rootParentOffset = parentNode;
    }

    pHierarchy->parentFrame = parentHierarchy->pNodeInfo[startNode].pFrame;

    pHierarchy->pMatrixArray = &parentHierarchy->pMatrixArray[startNode];
    pHierarchy->pMatrixArrayUnaligned = NULL;

    pHierarchy->pNodeInfo = &parentHierarchy->pNodeInfo[startNode];

    pHierarchy->parentHierarchy = parentHierarchy;

    RWRETURN(pHierarchy);
}


/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchyCreateFromHierarchy
 *  creates a hierarchy with the same structure as the source hierarchy.
 *  This can be used to create multiple secondary hierarchies for blending.
 *
 *  Note that it is not possible to create a new hierarchy from an existing
 *  sub-hierarchy. To perform this operation create a new sub-hierarchy from
 *  the original parent.
 *
 * \param hierarchy The source hierarchy to copy info from
 * \param flags Flags for the new hierarchy, if cloning a hierarchy simply
 *              pass in the source hierarchy flags
 * \param maxInterpKeyFrameSize specifies the maximum size of keyframes that
 *        will be usable on the new hierarchy
 *
 * \return A pointer to the new hierarchy on success,
 * NULL if there is an error.
 *
 */
RpHAnimHierarchy *
RpHAnimHierarchyCreateFromHierarchy(RpHAnimHierarchy *hierarchy,
                                    RpHAnimHierarchyFlag flags,
                                    RwInt32 maxInterpKeyFrameSize)
{
    RpHAnimHierarchy    *pNewHierarchy;
    RwInt32             numNodes;
    RwInt32             i;

    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchyCreateFromHierarchy"));
    RWASSERT(hierarchy);
    RWASSERT((hierarchy->flags & rpHANIMHIERARCHYSUBHIERARCHY) == 0);

    numNodes = hierarchy->numNodes;
    pNewHierarchy = RpHAnimHierarchyCreate(numNodes,
                                           (RwUInt32 *)NULL,
                                           (RwInt32 *)NULL,
                                           flags,
                                           maxInterpKeyFrameSize);

    /* now copy over the pNodeInfo data */
    for(i=0; i<pNewHierarchy->numNodes; ++i)
    {
        pNewHierarchy->pNodeInfo[i].pFrame = (RwFrame *)NULL;
        pNewHierarchy->pNodeInfo[i].flags = hierarchy->pNodeInfo[i].flags;
        pNewHierarchy->pNodeInfo[i].nodeIndex = hierarchy->pNodeInfo[i].nodeIndex;
        pNewHierarchy->pNodeInfo[i].nodeID = hierarchy->pNodeInfo[i].nodeID;
    }

    RWRETURN(pNewHierarchy);
}


/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchyAttach
 * attaches hierarchies from RwFrame. When attached the \ref RpHAnimHierarchy's can
 * update RwFrame directly through animation updates. The attachment is done by linking
 * ID's set on the RwFrame with \ref RpHAnimFrameSetID and the ID's for the nodes passed
 * into \ref RpHAnimHierarchyCreate.
 *
 * \param hierarchy The hierarchy which you want to attach to an RwFrame hierarchy.
 *
 * \return A pointer to the hierarchy on success
 * \see RpHAnimHierarchyDetach
 */
RpHAnimHierarchy *
RpHAnimHierarchyAttach(RpHAnimHierarchy *hierarchy)
{
    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchyAttach"));
    RWASSERT(hierarchy);

    HAnimFramesLinkToHierarchy(hierarchy->parentFrame, hierarchy);

    RWRETURN(hierarchy);
}


/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchyDetach
 * detaches hierarchies from RwFrame. When attached the \ref RpHAnimHierarchy's
 * can update \ref RwFrame directly through anim updates. Hierarchies are
 * automatically detached when they are destroyed, however currently if you
 * destroy an RwFrame which is referenced by an AnimHierarchy but not at the
 * root of a hierarchy things may break. Since in general RwFrame hierarchies
 * are destroyed in one go rather than one at a time this shouldn't be a huge
 * issue.
 *
 * \param hierarchy The hierarchy to be detached.
 *
 * \return A pointer to the hierarchy on success.
 *
 * \see RpHAnimHierarchyAttach
 */
RpHAnimHierarchy *
RpHAnimHierarchyDetach(RpHAnimHierarchy *hierarchy)
{
    RwInt32 i;

    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchyDetach"));
    RWASSERT(hierarchy);

    for (i=0; i<hierarchy->numNodes; ++i)
    {
        hierarchy->pNodeInfo[i].pFrame = (RwFrame *)NULL;
    }

    RWRETURN(hierarchy);
}


/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchyAttachFrameIndex attaches a single node within
 * the hierarchy to an RwFrame. This allows you to setup RwFrame updates
 * on only a few necessary RwFrames rather than the whole RwFrame hierarchy.
 *
 * \param hierarchy The hierarchy in which you want to attach to an RwFrame
 * \param nodeIndex The index of the node you wish to be attached
 *
 * \return A pointer to the hierarchy on success
 *
 * \see RpHAnimHierarchyAttach
 * \see RpHAnimIDGetIndex
 */
RpHAnimHierarchy *
RpHAnimHierarchyAttachFrameIndex(RpHAnimHierarchy *hierarchy,
                                 RwInt32 nodeIndex)
{
    FrameLinkToHierarchyData linkInfo;

    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchyAttachFrameIndex"));
    RWASSERT(hierarchy);

    linkInfo.id = hierarchy->pNodeInfo[nodeIndex].nodeID;
    linkInfo.pHierarchy = hierarchy;
    HAnimFrameLinkToHierarchy(hierarchy->parentFrame, &linkInfo);

    RWRETURN(hierarchy);
}


/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchyDetachFrameIndex detaches a single node within
 * the hierarchy from an RwFrame.
 *
 * \param hierarchy The hierarchy in which you want to detach a RwFrame
 * \param nodeIndex The index of the node you wish to be detached
 *
 * \return A pointer to the hierarchy on success
 *
 * \see RpHAnimHierarchyDetach
 * \see RpHAnimIDGetIndex
 */
RpHAnimHierarchy *
RpHAnimHierarchyDetachFrameIndex(RpHAnimHierarchy *hierarchy, RwInt32 nodeIndex)
{
    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchyDetachFrameIndex"));
    RWASSERT(hierarchy);

    hierarchy->pNodeInfo[nodeIndex].pFrame = (RwFrame *)NULL;

    RWRETURN(hierarchy);
}


/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchyGetMatrixArray function call access to the pMatrixArray
 * in a hierarchy.
 *
 * \param hierarchy pointer to the hierarchy to retrieve the
 *        matrix array pointer.
 *
 * \return A pointer to the hierarchies matrix array on success
 *
 */
RwMatrix               *
RpHAnimHierarchyGetMatrixArray(RpHAnimHierarchy *hierarchy)
{
    RwMatrix           *result = (RwMatrix *)NULL;

    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchyGetMatrixArray"));
    RWASSERT(hierarchy);

    result = hierarchy->pMatrixArray;

    RWRETURN(result);
}


/**
 * \ingroup rphanim
 * \ref RpHAnimFrameSetHierarchy
 *  sets the RwFrame to point to a hierarchy allowing it to be
 *  streamed out
 *
 * \param frame A pointer to the root RwFrame for which
 *        the hierarchy is to be used.
 * \param hierarchy A pointer to the hierarchy to use
 *        for animating the RwFrame hierarchy
 *
 * \return TRUE on success, FALSE if an error occurs.
 *
 * \see RpHAnimFrameGetHierarchy
 */

RwBool
RpHAnimFrameSetHierarchy(RwFrame *frame, RpHAnimHierarchy *hierarchy)
{
    RpHAnimFrameExtension *frameExt;
    RWAPIFUNCTION(RWSTRING("RpHAnimFrameSetHierarchy"));
    RWASSERT(frame);

    frameExt = (RpHAnimFrameExtension *)RPHANIMFRAMEGETDATA(frame);

    /* Disconnect any existing hierarchy */
    if (frameExt->hierarchy)
    {
        frameExt->hierarchy->parentFrame = (RwFrame *) NULL;
    }

    frameExt->hierarchy = hierarchy;
    if (hierarchy)
    {
        hierarchy->parentFrame = frame;
    }

    RWRETURN(TRUE);
}


/**
 * \ingroup rphanim
 * \ref RpHAnimFrameGetHierarchy
 *  gets the hierarchy pointed to by this RwFrame.
 *
 * \param frame A pointer to the RwFrame for which the hierarchy is to be got.
 *
 * \return A pointer to the hierarchy attached to the RwFrame
 *
 * \see RpHAnimFrameSetHierarchy
 */

RpHAnimHierarchy     *
RpHAnimFrameGetHierarchy(RwFrame *frame)
{
    RpHAnimFrameExtension *frameExt;
    RpHAnimHierarchy     *result = (RpHAnimHierarchy *)NULL;

    RWAPIFUNCTION(RWSTRING("RpHAnimFrameGetHierarchy"));
    RWASSERT(frame);

    frameExt = (RpHAnimFrameExtension *)RPHANIMFRAMEGETDATA(frame);
    result = frameExt->hierarchy;

    RWRETURN(result);
}


/**
 * \ingroup rphanim
 * \ref RpHAnimFrameSetID
 *  sets the ID stored with the RwFrame, these are used to attach hierarchies
 *  to RwFrames.
 *
 * \param frame A pointer to the RwFrame to set the ID on.
 * \param id The ID to set.
 *
 * \return TRUE on success, FALSE if an error occurs.
 *
 * \see RpHAnimFrameGetHierarchy
 */

RwBool
RpHAnimFrameSetID(RwFrame *frame, RwInt32 id)
{
    RpHAnimFrameExtension *frameExt;
    RWAPIFUNCTION(RWSTRING("RpHAnimFrameSetID"));
    RWASSERT(frame);

    frameExt = (RpHAnimFrameExtension *)RPHANIMFRAMEGETDATA(frame);
    frameExt->id = id;

    RWRETURN(TRUE);
}


/**
 * \ingroup rphanim
 * \ref RpHAnimFrameGetID
 *  gets the ID stored with the RwFrame, these are used to attach hierarchies
 *  to RwFrames.
 *
 * \param frame pointer to the RwFrame from which the ID is to be retrieved.
 *
 * \return The ID stored on the RwFrame.
 *
 * \see RpHAnimFrameSetHierarchy
 */

RwInt32
RpHAnimFrameGetID(RwFrame *frame)
{
    RwInt32 result = rpHANIMDEFAULTFRAMEID;
    RpHAnimFrameExtension *frameExt;

    RWAPIFUNCTION(RWSTRING("RpHAnimFrameGetID"));
    RWASSERT(frame);

    frameExt = (RpHAnimFrameExtension *)RPHANIMFRAMEGETDATA(frame);
    result = frameExt->id;

    RWRETURN(result);
}

/**
 * \ingroup rphanim
 * \ref RpHAnimIDGetIndex
 * returns the index of a node in the node arrays based on
 * a given node ID. This is useful for procedurally animating nodes
 * since you need the index to access the matrices/keyframes. This is
 * also very useful for creating sub-hierarchies. Given the ID on the root
 * of the sub-hierarchy this gives you the index to pass into
 * \ref RpHAnimHierarchyCreateSubHierarchy
 *
 * \param hierarchy Hierarchy to find the node index within
 * \param ID  ID of the node for which you want the index
 *
 * \return The index of the node in the node arrays, based on a given node ID
 */
RwInt32
RpHAnimIDGetIndex(RpHAnimHierarchy *hierarchy, RwInt32 ID)
{
    RwInt32             result = -1;
    RwInt32             i;
    RpHAnimNodeInfo   *pNodeInfo;

    RWAPIFUNCTION(RWSTRING("RpHAnimIDGetIndex"));
    RWASSERT(hierarchy);

    pNodeInfo = hierarchy->pNodeInfo;

    for (i = 0; i < hierarchy->numNodes; ++i)
    {
        if (ID == pNodeInfo[i].nodeID)
        {
            result = i;
            break;
        }
    }

    RWRETURN(result);
}

/*******************************************************************************
 *
 * HAnimHierarchyUpdateFramesOnly
 *
 * This update function is applied instead of RpHAnimHierarchyUpdateMatrices
 * whenever the rpHANIMHIERARCHYNOMATRICES flag is set. The
 * RpHAnimHierarchyUpdateMatrices function is optimized for when the hierarchy
 * has a matrix array, and uses a stack of pointers to reference parent
 * matrices within that array.
 *
 * In this function, we must have an actual stack of matrices in which to
 * store the parents, as we have nowhere else to store them. Potentially, they
 * could be stored in the external frame LTMs, but we are not guaranteed to
 * have every node linked to an RwFrame.
 *
 * This method involves one or two matrix copies for several nodes. For best
 * performance, a hierarchy with a matrix array should therefore be used
 * instead.
 */
static RwBool
HAnimHierarchyUpdateFramesOnly(RpHAnimHierarchy *hierarchy)
{
    RwUInt32                    flags;
    RwFrame                    *parentFrame;

    RwInt32                     keyFrameSize;
    RtAnimKeyFrameApplyCallBack keyFrameApplyCB;

    RwMatrix                    parentStack[HANIMMATRIXSTACKHEIGHT];
    RwMatrix                   *stackTop;
    RwMatrix                    parentMatrix;

    RwInt32                     iNode;
    RpHAnimNodeInfo            *nodeInfo;
    void                       *nodeInterpFrame;

    RWFUNCTION(RWSTRING("HAnimHierarchyUpdateFramesOnly"));
    RWASSERT(hierarchy);

    flags = hierarchy->flags;

    /* Local space hierarchy is meaningless without a matrix array */
    RWASSERT(!(flags & rpHANIMHIERARCHYLOCALSPACEMATRICES));

    /* The initial parent matrix is the LTM of the hierarchy (or sub-hierarchy) */
    RWASSERT(hierarchy->parentFrame);
    parentFrame = RwFrameGetParent(hierarchy->parentFrame);
    if (parentFrame)
    {
        /* Don't use RwFrameGetLTM since we don't want to sync everything */
        if (!RwFrameDirty(parentFrame))
        {
             RwMatrixCopy(&parentMatrix, RwFrameGetLTM(parentFrame));
        }
        else
        {
            RwMatrix tempMatrix;

            RwMatrixCopy(&parentMatrix, RwFrameGetMatrix(parentFrame));

            parentFrame = RwFrameGetParent(parentFrame);
            while (parentFrame)
            {
                RwMatrixCopy(&tempMatrix, &parentMatrix);
                RwMatrixMultiply(&parentMatrix, &tempMatrix,
                                 RwFrameGetMatrix(parentFrame));
                parentFrame = RwFrameGetParent(parentFrame);
            }
        }
    }
    else
    {
        RwMatrixSetIdentity(&parentMatrix);
    }

    /* Setup sync flags for attached frame hierarchy */
    if (flags & rpHANIMHIERARCHYUPDATELTMS)
    {
        /* we need to make sure these frames get sync'd to the world without
           recalculating LTMs */
        if (!(rwObjectGetPrivateFlags(hierarchy->parentFrame->root) &
            (rwFRAMEPRIVATEHIERARCHYSYNCLTM | rwFRAMEPRIVATEHIERARCHYSYNCOBJ)))
        {
			//@{ 20050513 DDonSS : Threadsafe
			// Frame Lock
			CS_FRAME_LOCK();
			//@} DDonSS

            /* Not in the dirty list - add it */
            rwLinkListAddLLLink(&RWSRCGLOBAL(dirtyFrameList),
                                &hierarchy->parentFrame->root->inDirtyListLink);

            rwObjectSetPrivateFlags(hierarchy->parentFrame->root,
                    rwObjectGetPrivateFlags(hierarchy->parentFrame->root) |
                    (rwFRAMEPRIVATEHIERARCHYSYNCOBJ /*| rwFRAMEPRIVATESUBTREESYNCLTM*/));

			/* 2006. 3. 6. Nonstopdj */
			/* change unlock positon */
			//@{ 20050513 DDonSS : Threadsafe
			// Frame Unlock
			CS_FRAME_UNLOCK();
			//@} DDonSS
        }
    }

    /* Cache anim info */
    keyFrameApplyCB = hierarchy->currentAnim->keyFrameApplyCB;
    keyFrameSize = hierarchy->currentAnim->currentInterpKeyFrameSize;

    /* Iterate over the nodes of the hierarchy */
    stackTop = parentStack + 1; /* Allow 1 place for last pop */
    nodeInfo = hierarchy->pNodeInfo;
    nodeInterpFrame = rtANIMGETINTERPFRAME(hierarchy->currentAnim, 0);

    for (iNode = 0; iNode < hierarchy->numNodes; ++iNode)
    {
        RwMatrix    nodeModelMatrix, nodeMatrix;
        RwFrame    *nodeFrame;

        RWASSERTM(stackTop > parentStack,
            (RWSTRING("Invalid hierarchy node flags.")));

        /* Convert this node's interpolated anim frame to modelling matrix */
        if (RpHAnimKeyFrameApply == keyFrameApplyCB)
        {
            RpHAnimKeyFrameToMatrixMacro(&nodeModelMatrix, nodeInterpFrame);
        }
        else
        {
            keyFrameApplyCB(&nodeModelMatrix, nodeInterpFrame);
        }

        /* Concatenate with parent to get the node matrix in hierarchy space */
        RwMatrixMultiply(&nodeMatrix, &nodeModelMatrix, &parentMatrix);

        /* Update any attached frames */
        nodeFrame = nodeInfo->pFrame;
        if (nodeFrame != NULL)
        {
            if (flags & rpHANIMHIERARCHYUPDATEMODELLINGMATRICES)
            {
                RwMatrixCopy(RwFrameGetMatrix(nodeFrame), &nodeModelMatrix);
                if (!(flags & rpHANIMHIERARCHYUPDATELTMS))
                {
                    RwFrameUpdateObjects(nodeFrame);
                }
            }

            if (flags & rpHANIMHIERARCHYUPDATELTMS)
            {
                RwMatrixCopy(&nodeFrame->ltm, &nodeMatrix);

                /* Mark to sync objects but not the LTM */
                rwObjectSetPrivateFlags(nodeFrame,
                    (rwObjectGetPrivateFlags(nodeFrame)
                        & (~rwFRAMEPRIVATESUBTREESYNCLTM))
                  | rwFRAMEPRIVATESUBTREESYNCOBJ);
            }
        }

        /* What happens next? */
        switch (nodeInfo->flags & (rpHANIMPUSHPARENTMATRIX|rpHANIMPOPPARENTMATRIX))
        {
        case 0:
            /* We have children but no siblings, so no need to save our parent. */
            RwMatrixCopy(&parentMatrix, &nodeMatrix);
            break;

        case (rpHANIMPOPPARENTMATRIX):
            /* We are childless, next node is back up in the tree, so recover parent. */
            RwMatrixCopy(&parentMatrix, --stackTop);
            break;

        case (rpHANIMPUSHPARENTMATRIX):
            /* We have children but other siblings need our parent, so save it. */
            RWASSERT(stackTop < &parentStack[HANIMMATRIXSTACKHEIGHT]);
            RwMatrixCopy(stackTop++, &parentMatrix);
            RwMatrixCopy(&parentMatrix, &nodeMatrix);
            break;

        case (rpHANIMPUSHPARENTMATRIX|rpHANIMPOPPARENTMATRIX):
            /* We are childless, next is sibling, same parent, so do nothing. */
            break;
        }

        nodeInterpFrame = (void *)((RwUInt8 *)nodeInterpFrame + keyFrameSize);
        ++nodeInfo;
    }

    RWRETURN(TRUE);
}


/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchyUpdateMatrices
 * iterates over all the nodes in the hierarchy updating a combination
 * of three members those being the local array of matrices, the modeling
 * matrices in \ref RwFrame's and the LTMs in RwFrame. If this hierarchy
 * is a sub-hierarchy then the matrices updated will be a sub range of the
 * parent hierarchies matrices.
 *
 * \param hierarchy A pointer to a hierarchy
 *
 * \return TRUE on success, FALSE if an error occurs.
 */
RwBool
RpHAnimHierarchyUpdateMatrices(RpHAnimHierarchy *hierarchy)
{
    RwUInt32                    flags;
    RwBool                      needHierarchyLTM = FALSE;
    RwFrame                    *hierarchyRootFrame = (RwFrame *)NULL;
    RwMatrix                    hierarchyLTM;
    RwMatrix                    identity;

    RwInt32                     keyFrameSize;
    RtAnimKeyFrameApplyCallBack keyFrameApplyCB;

    RwMatrix                   *parentStack[HANIMMATRIXSTACKHEIGHT];
    RwMatrix                  **stackTop;
    RwMatrix                   *parentMatrix;

    RwInt32                     iNode;
    RpHAnimNodeInfo            *nodeInfo;
    RwMatrix                   *nodeMatrix;
    void                       *nodeInterpFrame;

    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchyUpdateMatrices"));
    RWASSERT(hierarchy);

    flags = hierarchy->flags;

    /* Use a separate function when the hierarchy has no matrix array as we
     * cannot simply use our stack of matrix pointers to reference parent
     * matrices in the array.
     */
    if (flags & rpHANIMHIERARCHYNOMATRICES)
    {
        RWRETURN(HAnimHierarchyUpdateFramesOnly(hierarchy));
    }

    /*
     *  We need to find an initial parent matrix before iterating through
     *  the nodes of the hierarchy.
     */

    /* Is this a true sub-hierarchy? */
    if ((flags & rpHANIMHIERARCHYSUBHIERARCHY)
        && (hierarchy->rootParentOffset != -1))
    {
        /* Get the parent from the matrix array */
        parentMatrix =
            &hierarchy->parentHierarchy->pMatrixArray[hierarchy->rootParentOffset];

        /* Do we need an overall hierarchy LTM for updating Frame LTMs? */
        if ((flags & rpHANIMHIERARCHYUPDATELTMS)
            && (flags & rpHANIMHIERARCHYLOCALSPACEMATRICES))
        {
            /* If we supported nested subhierarchies, we'd recurse here */
            needHierarchyLTM = TRUE;
            hierarchyRootFrame = hierarchy->parentHierarchy->parentFrame;
        }
    }
    else
    {
        if (flags & rpHANIMHIERARCHYLOCALSPACEMATRICES)
        {
            RwMatrixSetIdentity(&identity);
            parentMatrix = &identity;

            if (flags & rpHANIMHIERARCHYUPDATELTMS)
            {
                needHierarchyLTM = TRUE;
                hierarchyRootFrame = hierarchy->parentFrame;
            }
        }
        else
        {
            needHierarchyLTM = TRUE;
            hierarchyRootFrame = hierarchy->parentFrame;
            parentMatrix = &hierarchyLTM;
        }
    }

    /* Get the hierarchy LTM if required */
    if (needHierarchyLTM)
    {
        if (hierarchyRootFrame && RwFrameGetParent(hierarchyRootFrame))
        {
            RwFrame *parentFrame = RwFrameGetParent(hierarchyRootFrame);

            /* Don't use RwFrameGetLTM since we don't want to sync everything */
            if (!RwFrameDirty(parentFrame))
            {
                 RwMatrixCopy(&hierarchyLTM, RwFrameGetLTM(parentFrame));
            }
            else
            {
                RwMatrix    tempMatrix;

                RwMatrixCopy(&hierarchyLTM, RwFrameGetMatrix(parentFrame));

                parentFrame = RwFrameGetParent(parentFrame);
                while (parentFrame)
                {
                    RwMatrixCopy(&tempMatrix, &hierarchyLTM);
                    RwMatrixMultiply(&hierarchyLTM, &tempMatrix,
                                     RwFrameGetMatrix(parentFrame));
                    parentFrame = RwFrameGetParent(parentFrame);
                }
            }
        }
        else
        {
            RwMatrixSetIdentity(&hierarchyLTM);
        }
    }

    /* Setup sync flags for attached frame hierarchy */
    if (flags & rpHANIMHIERARCHYUPDATELTMS)
    {
        /* we need to make sure these frames get sync'd to the world without
           recalculating LTMs */
        if (!(rwObjectGetPrivateFlags(hierarchy->parentFrame->root) &
            (rwFRAMEPRIVATEHIERARCHYSYNCLTM | rwFRAMEPRIVATEHIERARCHYSYNCOBJ)))
        {
			//@{ 20050513 DDonSS : Threadsafe
			// Frame Lock
			CS_FRAME_LOCK();
			//@} DDonSS

            /* Not in the dirty list - add it */
            rwLinkListAddLLLink(&RWSRCGLOBAL(dirtyFrameList),
                                &hierarchy->parentFrame->root->inDirtyListLink);

            rwObjectSetPrivateFlags(hierarchy->parentFrame->root,
                    rwObjectGetPrivateFlags(hierarchy->parentFrame->root) |
                    (rwFRAMEPRIVATEHIERARCHYSYNCOBJ /*| rwFRAMEPRIVATESUBTREESYNCLTM*/));

			/* 2006. 3. 6. Nonstopdj */
			/* change unlock positon */
			//@{ 20050513 DDonSS : Threadsafe
			// Frame Unlock
			CS_FRAME_UNLOCK();
			//@} DDonSS
        }
    }

    /*
     *  Now iterate over the nodes and update matrices.
     */

    /* Cache anim info */
    keyFrameApplyCB = hierarchy->currentAnim->keyFrameApplyCB;
    keyFrameSize = hierarchy->currentAnim->currentInterpKeyFrameSize;

    /* Do the iteration */
    stackTop = parentStack + 1; /* Allow 1 place for last pop */
    nodeInfo = hierarchy->pNodeInfo;
    nodeMatrix = hierarchy->pMatrixArray;
    nodeInterpFrame = rtANIMGETINTERPFRAME(hierarchy->currentAnim, 0);

    for (iNode = 0; iNode < hierarchy->numNodes; ++iNode)
    {
        RwMatrix    nodeModelMatrix;
        RwFrame    *nodeFrame;

        RWASSERTM(stackTop > parentStack,
            (RWSTRING("Invalid hierarchy node flags.")));

        /* Convert this node's interpolated anim frame to modelling matrix */
        if (RpHAnimKeyFrameApply == keyFrameApplyCB)
        {
            RpHAnimKeyFrameToMatrixMacro(&nodeModelMatrix, nodeInterpFrame);
        }
        else
        {
            keyFrameApplyCB(&nodeModelMatrix, nodeInterpFrame);
        }

        /* Concatenate with parent to get the node matrix in hierarchy space */
        RwMatrixMultiply(nodeMatrix, &nodeModelMatrix, parentMatrix);

        /* Update any attached frames */
        nodeFrame = nodeInfo->pFrame;
        if (nodeFrame)
        {
            if (flags & rpHANIMHIERARCHYUPDATEMODELLINGMATRICES)
            {
                RwMatrixCopy(RwFrameGetMatrix(nodeFrame), &nodeModelMatrix);
                if (!(flags & rpHANIMHIERARCHYUPDATELTMS))
                {
                    RwFrameUpdateObjects(nodeFrame);
                }
            }

            if (flags & rpHANIMHIERARCHYUPDATELTMS)
            {
                if (flags & rpHANIMHIERARCHYLOCALSPACEMATRICES)
                {
                    RwMatrixMultiply(&nodeFrame->ltm, nodeMatrix, &hierarchyLTM);
                }
                else
                {
                    RwMatrixCopy(&nodeFrame->ltm, nodeMatrix);
                }

                /* Mark to sync objects but not the LTM */
                rwObjectSetPrivateFlags(nodeFrame,
                    (rwObjectGetPrivateFlags(nodeFrame)
                        & (~rwFRAMEPRIVATESUBTREESYNCLTM))
                  | rwFRAMEPRIVATESUBTREESYNCOBJ);
            }
        }

        /* What happens next? */
        switch (nodeInfo->flags & (rpHANIMPOPPARENTMATRIX|rpHANIMPUSHPARENTMATRIX))
        {
        case 0:
            /* We have children but no siblings, so no need to save our parent. */
            parentMatrix = nodeMatrix;
            break;

        case (rpHANIMPOPPARENTMATRIX):
            /* We are childless, next node is back up in the tree, so recover parent. */
            parentMatrix = *(--stackTop);
            break;

        case (rpHANIMPUSHPARENTMATRIX):
            /* We have children but other siblings need our parent, so save it. */
            RWASSERT(stackTop < &parentStack[HANIMMATRIXSTACKHEIGHT]);
            *(stackTop++) = parentMatrix;
            parentMatrix = nodeMatrix;
            break;

        case (rpHANIMPOPPARENTMATRIX|rpHANIMPUSHPARENTMATRIX):
            /* We are childless, next is sibling, same parent, so do nothing. */
            break;
        }

        nodeInterpFrame = (void *)((RwUInt8 *)nodeInterpFrame + keyFrameSize);
        ++nodeMatrix;
        ++nodeInfo;
    }

    RWRETURN(TRUE);
}


/*
 *  Utility Functions
 */
#ifdef RWDEBUG

/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchySetCurrentAnim
 * Sets the current animation on the hierarchy. It is assumed that the
 * animation is designed for the hierarchy it is being set on and no
 * hierarchy structure comparisons are made. The animation defines the
 * interpolation schemes used and they will be setup on the hierarchy
 * at this call. The max keyframe size of the hierarchy must be sufficient
 * to support the keyframe size required by the interpolation scheme.
 * The animation is initialized to time zero at this call.
 *
 * If the hierarchy is attached to a skin, the skin will not update until
 * \ref RpHAnimHierarchyUpdateMatrices is called.
 *
 * \param hierarchy A pointer to the hierarchy
 * \param anim A pointer to the animation
 *
 * \return TRUE on success, FALSE if an error occurs.
 *
 * \see RtAnimInterpolatorSetCurrentAnim
 *
 */
RwBool
RpHAnimHierarchySetCurrentAnim(RpHAnimHierarchy *hierarchy,
                                RtAnimAnimation *anim)
{
    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchySetCurrentAnim"));
    RWASSERT(hierarchy);
    RWASSERT(hierarchy->currentAnim);
    RWASSERT(anim);

    RWRETURN(RpHAnimHierarchySetCurrentAnimMacro(hierarchy,anim));
}

/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchyGetCurrentAnim
 * Returns the current animation on the hierarchy.
 *
 * \param hierarchy A pointer to the hierarchy
 *
 * \return a pointer to the current \ref RtAnimAnimation if any.
 *
 * \see RpHAnimHierarchySetCurrentAnim
 * \see RtAnimInterpolatorSetCurrentAnim
 * \see RtAnimInterpolatorGetCurrentAnim
 *
 */
RtAnimAnimation *
RpHAnimHierarchyGetCurrentAnim(RpHAnimHierarchy *hierarchy)
{
    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchyGetCurrentAnim"));
    RWASSERT(hierarchy);

    RWRETURN(RpHAnimHierarchyGetCurrentAnimMacro(hierarchy));
}

/**
 * \ingroup rphanim
 *  \ref RpHAnimHierarchySetCurrentAnimTime
 *  sets the current animation time
 *  of a hierarchy.
 *
 * If the hierarchy is attached to a skin, the skin will not update until
 * \ref RpHAnimHierarchyUpdateMatrices is called.
 *
 *  \param hierarchy A pointer to a hierarchy.
 *  \param time The time to which to set the current animation.
 *
 *  \return TRUE on success, FALSE otherwise.
 *
 * \see RpHAnimHierarchyUpdateMatrices
 * \see RpHAnimHierarchyAddAnimTime
 * \see RpHAnimHierarchySubAnimTime
 * \see RtAnimInterpolatorAddAnimTime
 * \see RtAnimInterpolatorSubAnimTime
 */
RwBool
RpHAnimHierarchySetCurrentAnimTime(RpHAnimHierarchy *hierarchy,
                                RwReal time)
{
    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchySetCurrentAnimTime"));
    RWASSERT(hierarchy);
    RWASSERT(hierarchy->currentAnim);

    RWRETURN(RpHAnimHierarchySetCurrentAnimTimeMacro(hierarchy,time));
}


/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchyAddAnimTime
 * add anim time will step through the keyframes to find the current
 * pair for each node to interpolate. These will then be interpolated by
 * calling the appropriate overloaded interpolation scheme functions. If the
 * animation loops or passes the time set in \ref RpHAnimHierarchySetKeyFrameCallBacks
 * then the appropriate callback will be called. By default the animation will
 * simply loop continuously.
 *
 * This function does not make assumptions about the direction of animation prior
 * to calling. It is not therefore necessary for the user to keep track of the
 * direction of play, nor handle a change of direction explicitly.
 *
 * The hierarchy's matrices and attached RwFrames will not update until
 * \ref RpHAnimHierarchyUpdateMatrices is called.
 *
 * \param hierarchy A pointer to a hierarchy.
 * \param time The amount of time to add to the animation.
 *
 * \return TRUE on success
 *
 * \see RpHAnimHierarchyAddAnimTime
 * \see RpHAnimHierarchySetCurrentAnim
 * \see RpHAnimHierarchyGetCurrentAnim
 * \see RpHAnimHierarchySetCurrentAnimTime
 * \see RpHAnimHierarchySubAnimTime
 * \see RpHAnimHierarchyUpdateMatrices
 * \see RtAnimInterpolatorAddAnimTime
 * \see RtAnimInterpolatorSetCurrentAnim
 * \see RtAnimInterpolatorGetCurrentAnim
 * \see RtAnimInterpolatorSetCurrentTime
 * \see RtAnimInterpolatorSubAnimTime
 */
RwBool
RpHAnimHierarchyAddAnimTime(RpHAnimHierarchy *hierarchy,
                                RwReal time)
{
    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchyAddAnimTime"));
    RWASSERT(hierarchy);
    RWASSERT(hierarchy->currentAnim);

    RWRETURN(RpHAnimHierarchyAddAnimTimeMacro(hierarchy,time));
}

/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchySubAnimTime
 * subtract anim time will step through the keyframes to find the current
 * pair for each node to interpolate. These will then be interpolated by
 * calling the appropriate overloaded interpolation scheme functions. If the
 * animation loops or passes the time set in \ref RpHAnimHierarchySetAnimCallBack
 * then the appropriate callback will be called. By default the animation will
 * simply loop continuously.
 *
 * If the hierarchy is attached to a skin, the skin will not update until
 * \ref RpHAnimHierarchyUpdateMatrices is called.
 *
 * \param hierarchy A pointer to a hierarchy
 * \param time The amount of time to subtract from the animation
 *
 * \return TRUE on success
 *
 * \see RpHAnimHierarchyUpdateMatrices
 * \see RpHAnimHierarchyAddAnimTime
 * \see RpHAnimHierarchySetCurrentAnimTime
 * \see RtAnimInterpolatorAddAnimTime
 * \see RtAnimInterpolatorSetCurrentTime
 * \see RtAnimInterpolatorSubAnimTime
 */
RwBool
RpHAnimHierarchySubAnimTime(RpHAnimHierarchy *hierarchy,
                                RwReal time)
{
    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchySubAnimTime"));
    RWASSERT(hierarchy);
    RWASSERT(hierarchy->currentAnim);

    RWRETURN(RpHAnimHierarchySubAnimTimeMacro(hierarchy,time));
}

/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchySetKeyFrameCallBacks
 * Sets up the callbacks to be used by the hierarchy. These callbacks
 * perform keyframe blending, interpolation and conversion to matrices.
 * The callback setup is implicitly performed when setting the current
 * animation on a hierarchy, however if you wish to fully procedurally
 * drive the keyframe data this function allows the callbacks to be
 * setup so that \ref RpHAnimHierarchyUpdateMatrices performs correctly.
 *
 *
 * \param hierarchy A pointer to the hierarchy
 * \param keyFrameTypeID ID of the keyframe type to retrieve the callbacks for
 *
 * \return TRUE on success, FALSE if an error occurs.
 *
 * \see RtAnimInterpolatorSetKeyFrameCallBacks
 */
RwBool
RpHAnimHierarchySetKeyFrameCallBacks(RpHAnimHierarchy *hierarchy,
                                     RwInt32 keyFrameTypeID)
{
    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchySetKeyFrameCallBacks"));
    RWASSERT(hierarchy);
    RWASSERT(hierarchy->currentAnim);

    RWRETURN(RpHAnimHierarchySetKeyFrameCallBacksMacro(hierarchy,keyFrameTypeID));
}

/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchyBlend interpolates between two hierarchies.The result
 * will be interpolated between the current state of the two input hierarchies.
 *
 * The most common use of this function will probably be to blend from the
 * end of one animation to the start of the next. This would be done
 * with 3 hierarchies, anim1skel, hanimskel and tempskel.

 * Initially anim1 would be  running on anim1skel which is attached to the skin.
 * When the blend is required hanim is attached to hanimskel and tempskel
 * is now attached to the skin. For each update during the blend duration
 * \ref RpHAnimHierarchyBlend is used to blend between anim1skel and hanimskel, storing
 * the results in tempskel. At the end of the blend hanimskel is attached to
 * the skin.

 * \param outHierarchy A pointer to a hierarchy in which the result is returned.
 * \param inHierarchy1 A pointer to the first input hierarchy
 * \param inHierarchy2 A pointer to the second input hierarchy
 * \param alpha The blending parameter
 * \li 0.0 returns inHierarchy1
 * \li 1.0 returns inHierarchy2
 *
 * \return TRUE on success, FALSE otherwise.
 *
 * \see RtAnimInterpolatorBlend
 *
 */
RwBool
RpHAnimHierarchyBlend(RpHAnimHierarchy *outHierarchy,
                      RpHAnimHierarchy *inHierarchy1,
                      RpHAnimHierarchy *inHierarchy2,
                      RwReal alpha)
{
    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchyBlend"));
    RWASSERT(outHierarchy);
    RWASSERT(inHierarchy1);
    RWASSERT(inHierarchy2);

    RWRETURN(RpHAnimHierarchyBlendMacro(outHierarchy,
                                        inHierarchy1,
                                        inHierarchy2,
                                        alpha));
}

/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchyAddTogether
 *  adds together two hierarchies.  Each
 * hierarchy should have an animation attached, and the result will be
 * the addition of the two input hierarchies.  This is mainly useful where
 * the second hierarchy's pose is a delta from a set pose to be added to the
 * pose held in the first hierarchy.  For example, a walk animation could have
 * a crouch pose added to it as a delta from a standing pose.
 * \note outHierarchy, inHierarchy1 and inHierarchy2 MUST be different.
 *
 * \param outHierarchy A pointer to a hierarchy in which the result is returned.
 * \param inHierarchy1 A pointer to the first input hierarchy
 * \param inHierarchy2 A pointer to the second input hierarchy
 *
 * \return TRUE on success, FALSE otherwise.
 *
 * \see RtAnimAnimationMakeDelta
 * \see RtAnimAnimationAddTogether
 *
 */
RwBool
RpHAnimHierarchyAddTogether(RpHAnimHierarchy *outHierarchy,
                      RpHAnimHierarchy *inHierarchy1,
                      RpHAnimHierarchy *inHierarchy2)
{
    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchyAddTogether"));
    RWASSERT(outHierarchy);
    RWASSERT(inHierarchy1);
    RWASSERT(inHierarchy2);

    RWRETURN(RpHAnimHierarchyAddTogetherMacro(outHierarchy,
                                        inHierarchy1,
                                        inHierarchy2));
}

/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchySetAnimCallBack
 *  is used to set a callback function
 * that will be called when a hierarchy reaches a specified time.  The
 * callback function should return a pointer to the hierarchy if the function
 * should continue to be called, or NULL if it should never be called again.
 *
 * \param hierarchy A pointer to a hierarchy.
 * \param callBack A pointer to a callback function.
 * \param time The time at which the callback function should be called.
 * \param data A pointer to a block of data to be passed into the callback function
 *
 * \return Nothing.
 *
 */

void
RpHAnimHierarchySetAnimCallBack(RpHAnimHierarchy *hierarchy,
                                RtAnimCallBack callBack,
                                RwReal time, void *data)
{
    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchySetAnimCallBack"));

    RWASSERT(hierarchy);

    RpHAnimHierarchySetAnimCallBackMacro(hierarchy,callBack,time, data);

    RWRETURNVOID();
}

/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchySetAnimLoopCallBack
 *  is used to set a callback function
 * that will be called when a hierarchy's animation loops.  The
 * callback function should return a pointer to the hierarchy if the function
 * should continue to be called, or NULL if it should never be called again.
 *
 * \param hierarchy A pointer to a hierarchy.
 * \param callBack A pointer to a callback function.
 * \param data A pointer to a block of data to be passed into the callback function
 *
 * \return Nothing.
 *
 */
void
RpHAnimHierarchySetAnimLoopCallBack(RpHAnimHierarchy *hierarchy,
                                    RtAnimCallBack callBack,
                                    void *data)
{
    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchySetAnimLoopCallBack"));

    RWASSERT(hierarchy);

    RpHAnimHierarchySetAnimLoopCallBackMacro(hierarchy,callBack,data);

    RWRETURNVOID();
}

/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchyBlendSubHierarchy
 *  interpolates the nodes in a sub-hierarchy with it parent hierarchy.  Each
 * hierarchy should have an animation attached, and the result will be
 * interpolated between the current state of the two input hierarchies. Either
 * source hierarchy can be the sub-hierarchy, the blend is performed from
 * inHierarchy1 to inHierarchy2. The output hierarchy must be identical in
 * structure to either the main or sub-hierarchy.
 *
 * \param outHierarchy A pointer to a hierarchy in which the result is returned.
 * \param inHierarchy1 A pointer to the first source hierarchy
 * \param inHierarchy2 A pointer to the second source hierarchy
 * \param alpha The blending parameter
 * \li 0.0 returns inHierarchy1
 * \li 1.0 returns inHierarchy2
 *
 * \return TRUE on success, FALSE otherwise.
 *
 */
RwBool
RpHAnimHierarchyBlendSubHierarchy(RpHAnimHierarchy *outHierarchy,
                      RpHAnimHierarchy *inHierarchy1,
                      RpHAnimHierarchy *inHierarchy2,
                      RwReal alpha)
{
    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchyBlendSubHierarchy"));
    RWASSERT(outHierarchy);
    RWASSERT(inHierarchy1);
    RWASSERT(inHierarchy2);

    RWRETURN(RpHAnimHierarchyBlendSubHierarchyMacro(outHierarchy,
                                        inHierarchy1,
                                        inHierarchy2,
                                        alpha));
}

/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchyAddSubHierarchy
 *  adds together two hierarchies.  Each
 * hierarchy should have an animation attached, and the result will be
 * the addition of the two input hierarchies. The output hierarchy must
 * be identical in structure to either the main or sub-hierarchy.
 *
 * \param outHierarchy A pointer to a hierarchy in which the result is returned.
 * \param mainHierarchy A pointer to the parent hierarchy
 * \param subHierarchy A pointer to the sub hierarchy
 * \note outHierarchy, mainHierarchy and subHierarchy MUST be different.
 *
 * \return TRUE on success, FALSE otherwise.
 *
 * \see RpHAnimAnimationMakeDelta
 *
 */
RwBool
RpHAnimHierarchyAddSubHierarchy(RpHAnimHierarchy *outHierarchy,
                      RpHAnimHierarchy *mainHierarchy,
                      RpHAnimHierarchy *subHierarchy)
{
    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchyAddSubHierarchy"));
    RWASSERT(outHierarchy);
    RWASSERT(mainHierarchy);
    RWASSERT(subHierarchy);

    RWRETURN(RpHAnimHierarchyAddSubHierarchyMacro(outHierarchy, mainHierarchy, subHierarchy));
}

/**
 * \ingroup rphanim
 * \ref RpHAnimHierarchyCopy
 *  copies state of nodes in the in-hierarchy into
 *                              the out-hierarchy.
 *
 * \param outHierarchy A pointer to a hierarchy in which the result is returned.
 * \param inHierarchy A pointer to the input hierarchy
 *
 * \return TRUE on success, FALSE otherwise.
 *
 */
RwBool
RpHAnimHierarchyCopy(RpHAnimHierarchy *outHierarchy,
                     RpHAnimHierarchy *inHierarchy)
{
    RWAPIFUNCTION(RWSTRING("RpHAnimHierarchyCopy"));
    RWASSERT(outHierarchy);
    RWASSERT(inHierarchy);
    RWASSERT(outHierarchy->numNodes == inHierarchy->numNodes);

    RWRETURN(RpHAnimHierarchyCopyMacro(outHierarchy, inHierarchy));
}
#endif /* #ifdef RWDEBUG */
