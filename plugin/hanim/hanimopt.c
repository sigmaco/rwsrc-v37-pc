/**********************************************************************
 *
 * File : hanimopt.c
 *
 * Abstract : Additional hanim functions
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
 * Copyright (c) 2003 Criterion Software Ltd.
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

#define MAX_BONES 256
#define MAX_KILL_SIZE 256

static RwBool
HAnimKeyFrameValid(RpHAnimKeyFrame *kf)
{
    RWFUNCTION( RWSTRING( "HAnimKeyFrameValid" ) );

    RWASSERT(kf != NULL);

    if (kf->q.imag.x != kf->q.imag.x)
    {
        RWRETURN(FALSE);
    }
    if (kf->q.imag.y != kf->q.imag.y)
    {
        RWRETURN(FALSE);
    }
    if (kf->q.imag.z != kf->q.imag.z)
    {
        RWRETURN(FALSE);
    }
    if (kf->q.real != kf->q.real)
    {
        RWRETURN(FALSE);
    }

    if (kf->t.x != kf->t.x)
    {
        RWRETURN(FALSE);
    }
    if (kf->t.y != kf->t.y)
    {
        RWRETURN(FALSE);
    }
    if (kf->t.z != kf->t.z)
    {
        RWRETURN(FALSE);
    }


    if (kf->q.imag.x > 2 || kf->q.imag.y > 2 || kf->q.imag.z > 2)
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}

static void
HAnimKeyFrameMulti( RpHAnimKeyFrame *result,
                    RpHAnimKeyFrame *a,
                    RpHAnimKeyFrame *b )
{
    RwReal  mag;
    RwV3d   temp;

    RWFUNCTION( RWSTRING( "HAnimKeyFrameMulti" ) );

    RWASSERT( NULL != result );
    RWASSERT( NULL != a );
    RWASSERT( NULL != b );
    RWASSERT( a != result );
    RWASSERT( b != result );
    RWASSERT(HAnimKeyFrameValid(a));
    RWASSERT(HAnimKeyFrameValid(b));

    /* Translation first. */
    RtQuatTransformVectors(&temp, &b->t, 1, &a->q);
    RwV3dAdd(&result->t, &a->t, &temp);

    /* Rotation. */
    RtQuatMultiply(&result->q, &a->q, &b->q);

    /* unit */
    mag = RtQuatModulus(&result->q);
    RtQuatScale(&result->q, &result->q, 1.0f/mag);

    RWRETURNVOID();
}

static RtAnimAnimation *
HAnimCopy(RtAnimAnimation *srcAnim)
{
    RtAnimAnimation *dstAnim = NULL;
    RpHAnimKeyFrame *srcKeyframes = NULL, *dstKeyframes = NULL;
    RwUInt32 index, delta;

    RWFUNCTION( RWSTRING( "HAnimCopy" ) );

    RWASSERT(srcAnim != NULL);
    RWASSERT(srcAnim->customData == NULL);

    /* Create the new anim. */
    dstAnim = RtAnimAnimationCreate(rpHANIMSTDKEYFRAMETYPEID,
                                    srcAnim->numFrames,
                                    srcAnim->flags,
                                    srcAnim->duration);

    /* Copy the keyframes over. */
    srcKeyframes = (RpHAnimKeyFrame *) srcAnim->pFrames;
    dstKeyframes = (RpHAnimKeyFrame *) dstAnim->pFrames;
    index = srcAnim->numFrames;
    delta = dstKeyframes - srcKeyframes;
    
    while(index--)
    {
        dstKeyframes[index].time = srcKeyframes[index].time;
        dstKeyframes[index].q    = srcKeyframes[index].q;
        dstKeyframes[index].t    = srcKeyframes[index].t;

        if (dstKeyframes[index].time == 0.0f)
        {
            dstKeyframes[index].prevFrame = NULL;
        }
        else
        {
            dstKeyframes[index].prevFrame = srcKeyframes[index].prevFrame + delta;
        }
    }
    
    RWRETURN(dstAnim);
}

static void
HAnimBuildParentIndex(RwInt32   *parentIndex,
                      RwUInt32  *pushpops,
                      RwInt32   numBones)
{
    RwInt32 i;

    RWFUNCTION( RWSTRING( "HAnimBuildParentIndex" ) );

    RWASSERT(parentIndex != NULL);
    RWASSERT(pushpops != NULL);
    RWASSERT(numBones <= MAX_BONES);

    /* build parentIndex array from push/pop flags */
    if (pushpops)
    {
        RwInt32 parent,indexstack[128],tos;

        tos = 0;
        parent = -1;
        indexstack[++tos] = parent;
        for (i=0; i<numBones; ++i)
        {
            parentIndex[i] = parent;

            if (pushpops[i] & rpHANIMPUSHPARENTMATRIX)
            {
                indexstack[++tos] = parent;
            }

            if (pushpops[i] & rpHANIMPOPPARENTMATRIX)
            {
                parent = indexstack[tos--];
            }
            else
            {
                parent = i;
            }
        }
    }
    else
    {
        for (i=0; i<numBones; ++i)
        {
            parentIndex[i] = -1;
        }
    }

    RWRETURNVOID();
}

static void
HAnimBuildLeavesIndex(RwInt32   *leavesIndices,
                      RwInt32   *numLeaves,
                      RwInt32   boneIndex,
                      RwInt32   *parentIndices,
                      RwInt32   numBones)
{
    RwInt32 index, parent;
    RwBool descendent[MAX_BONES];
    RwUInt32 numChildren[MAX_BONES];

    RWFUNCTION( RWSTRING( "HAnimBuildLeavesIndex" ) );

    RWASSERT(leavesIndices != NULL);
    RWASSERT(numLeaves != NULL);
    RWASSERT(parentIndices != NULL);
    RWASSERT(numBones <= MAX_BONES);
    RWASSERT(boneIndex < numBones);

    memset(descendent, 0, sizeof(descendent));
    memset(numChildren, 0, sizeof(numChildren));

    /* Set up the arrays. */
    index = 0;
    while(index < numBones)
    {
        parent = parentIndices[index];
        if(parent > 0)
        {
            ++numChildren[parent];
        }

        if((index == boneIndex) || 
           ((parent >= 0) && (descendent[parent])))
        {
            descendent[index] = TRUE;
        }

        ++index;
    }

    /* Now pull out all child leaves. */
    index = 0;
    *numLeaves = 0;
    while(index < numBones)
    {
        /* Check we got all the descendents. */
        parent = parentIndices[index];

        if(parent >= 0)
        {
            RWASSERT((descendent[index] == descendent[parent]) ||
                (index == boneIndex));

            if(descendent[index] && (numChildren[index] == 0))
            {
                leavesIndices[*numLeaves] = index;
                *numLeaves = *numLeaves + 1;
            }
        }

        ++index;
    }

    RWRETURNVOID();
}

static void
HAnimBuildBoneLists(RwSList         *kflist[],
                    RtAnimAnimation *srcAnim,
                    RwInt32         numBones)
{
    RwInt32 i, j, kfcount;
    RpHAnimKeyFrame *hframe;

    RWFUNCTION( RWSTRING( "HAnimBuildBoneLists" ) );

    RWASSERT(srcAnim != NULL);
    RWASSERT(kflist != NULL);
    RWASSERT(numBones <= MAX_BONES);

    /* setup a list/bone */
    for (i=0; i<numBones; ++i)
    {
        kflist[i] = rwSListCreate(sizeof(RpHAnimKeyFrame), rwMEMHINTDUR_FUNCTION);
    }

    /* gather time ordered list of keyframes for each bone from HAnim */
    hframe = (RpHAnimKeyFrame *)srcAnim->pFrames;
    kfcount = srcAnim->numFrames;
    for (j=0; j<2; ++j)
    {
        for (i=0; i<numBones; ++i)
        {
            RpHAnimKeyFrame *newentry;

            newentry = (RpHAnimKeyFrame *)rwSListGetNewEntry(kflist[i], rwMEMHINTDUR_FUNCTION);
            *newentry = *hframe++;
            --kfcount;
        }
    }

    while (kfcount > 0)
    {
        RpHAnimKeyFrame *bestkf = NULL;
        RpHAnimKeyFrame *newentry;
        RwInt32 bestlist = 0;

        /* find which bone's keyframe finishes first */
        for (i=0; i<numBones; ++i)
        {
            RpHAnimKeyFrame *lastkf = (RpHAnimKeyFrame *)rwSListGetEntry(kflist[i], rwSListGetNumEntries(kflist[i])-1);
            
            if (!bestkf || (lastkf->time < bestkf->time))
            {
                bestkf = lastkf;
                bestlist = i;
            }
        }

        newentry = rwSListGetNewEntry(kflist[bestlist], rwMEMHINTDUR_FUNCTION);
        *newentry = *hframe++;
        --kfcount;
    }

    RWRETURNVOID();
}

static void
HAnimScatterBoneLists(RwSList         *kflist[],
                      RtAnimAnimation *dstAnim,
                      RwInt32         numBones)
{
    RpHAnimKeyFrame *hframe,*prevFrames[128];
    RwInt32 i, j, kfcount;

    RWFUNCTION( RWSTRING( "HAnimScatterBoneLists" ) );

    RWASSERT(dstAnim != NULL);
    RWASSERT(kflist != NULL);
    RWASSERT(numBones <= MAX_BONES);

    /* scatter list of keyframes for each bone back to HAnim (fixing up PrevFrame as we go) */
    memset(prevFrames, 0, sizeof(prevFrames));
    kfcount = dstAnim->numFrames;
    hframe = (RpHAnimKeyFrame *)dstAnim->pFrames;
    for (j=0; j<2; ++j)
    {
        for (i=0; i<numBones; ++i)
        {
            RpHAnimKeyFrame *kf = (RpHAnimKeyFrame *)rwSListGetEntry(kflist[i], 0);

            kf->prevFrame = prevFrames[i];
            prevFrames[i] = hframe;

            *hframe++ = *kf;
            --kfcount;
            rwSListDestroyEntry(kflist[i], 0);
        }
    }

    while (kfcount > 0)
    {
        RpHAnimKeyFrame *bestkf = NULL;
        RwInt32 bestlist;

        /* find which bone's keyframe finishes first */
        bestlist = 0;
        for (i=1; i<numBones; ++i)
        {
            if (prevFrames[i]->time < prevFrames[bestlist]->time)
            {
                bestlist = i;
            }
        }

        bestkf = (RpHAnimKeyFrame *)rwSListGetEntry(kflist[bestlist], 0);

        bestkf->prevFrame = prevFrames[bestlist];
        prevFrames[bestlist] = hframe;

        *hframe++ = *bestkf;
        --kfcount;
        rwSListDestroyEntry(kflist[bestlist], 0);
    }

    RWRETURNVOID();
}

static void
ConditionKeyframes(RwSList         *kflist[],
                    RwInt32         numBones)
{
    RwInt32 i, j;

    RWFUNCTION( RWSTRING( "ConditionKeyframes" ) );

    RWASSERT(kflist != NULL);
    RWASSERT(numBones <= MAX_BONES);

    for (i=0; i<numBones; ++i)
    {
        RwSList *akflist = kflist[i];
        RpHAnimKeyFrame *prev = (RpHAnimKeyFrame *)rwSListGetEntry(akflist, 0);
        RwReal mag;

        /* unit */
        mag = RtQuatModulus(&prev->q);
        RtQuatScale(&prev->q, &prev->q, 1.0f/mag);

        for (j=1; j<rwSListGetNumEntries(akflist); ++j)
        {
            RpHAnimKeyFrame *kf = (RpHAnimKeyFrame *)rwSListGetEntry(akflist, j);
            RwReal theta;

            /* force same hemisphere */
            theta = RwV3dDotProduct(&prev->q.imag, &kf->q.imag) + prev->q.real * kf->q.real;
            if (theta < 0)
            {
                RtQuatNegate(&kf->q, &kf->q);
            }

            /* unit */
            mag = RtQuatModulus(&kf->q);
            RtQuatScale(&kf->q, &kf->q, 1.0f/mag);

            prev = kf;
        }
    }

    RWRETURNVOID();
}

static RwReal 
KeyDiff(RpHAnimKeyFrame *first, RpHAnimKeyFrame *second, RwReal length)
{
    RwReal diff = 0.f, angle, mag;
    RwV3d  vDiff;
    RtQuat qDiff, secConj;
    
    RWFUNCTION( RWSTRING( "KeyDiff" ) );

    RWASSERT(first != NULL);
    RWASSERT(second != NULL);

    if(length < 0.f)
    {
        length = 0.f;
    }

    /* Find the quaterion that maps one keyframe to the other. */
    RtQuatConjugate(&secConj, &second->q);
    RtQuatMultiply(&qDiff, &first->q, &secConj);

    /* Normalise it. */
    mag = RtQuatModulus(&qDiff);
    RtQuatScale(&qDiff, &qDiff, 1.0f/mag);
    
    /* Check for overflow. */
    if(qDiff.real > 1.f)
    {
        qDiff.real = 1.f;
    }
    if(qDiff.real < -1.f)
    {
        qDiff.real = -1.f;
    }

    /* Find the angle of rotation. */
    angle = (RwReal) (RwACos(qDiff.real)) * 2.f;

    if(angle < 0.f)
    {
        angle = -angle;
    }

    /* Assume error = sin angle * length ~ angle * length */
    diff += length * angle;

    /* Find the difference between the translations. */
    RwV3dSub(&vDiff, &first->t, &second->t);
    diff += RwV3dLength(&vDiff);

    RWRETURN(diff);
}

static RwInt32
FilterDuplicates( RwSList *kflist[], 
                  RwInt32 numBones,
                  RwReal  tolerance,
                  RwReal  averageNodeSize)
{
    RwInt32 i, j, numFiltered = 0;

    RWFUNCTION( RWSTRING( "FilterDuplicates" ) );

    RWASSERT(kflist != NULL);
    RWASSERT(numBones <= MAX_BONES);

    /* For every bone. */
    for (i=0; i<numBones; ++i)
    {
        RwSList *akflist = kflist[i];
        RpHAnimKeyFrame *prev, *kf, *next;
        
        prev = (RpHAnimKeyFrame *)rwSListGetEntry(akflist, 0);
        kf = (RpHAnimKeyFrame *)rwSListGetEntry(akflist, 1);

        /* For every triple of keyframes. */
        for(j = 2; j < rwSListGetNumEntries(akflist); ++j)
        {
            RwBool kfremove = FALSE;
            RwReal diff1, diff2, diff3;

            next = rwSListGetEntry(akflist, j);
            
            /* find the differences between them. */
            diff1 = KeyDiff(prev, kf, averageNodeSize);
            diff2 = KeyDiff(kf, next, averageNodeSize);
            diff3 = KeyDiff(prev, next, averageNodeSize);

            /* if they are all the same. */
            if((diff1 < tolerance) &&
               (diff2 < tolerance) &&
               (diff3 < tolerance))
            {
                /* remove the middle keyframe. */
                kfremove = TRUE;
            }

            if (kfremove)
            {
                --j;
                rwSListDestroyEntry(akflist, j);
                numFiltered++;
            }
            else
            {
                prev = kf;
                kf = next;
            }
        }
    }
    
    RWRETURN(numFiltered);
}

static void
GetKeyframeWorldSpace( RpHAnimKeyFrame *kf, 
                       RwReal  time, 
                       RwInt32 boneIndex,
                       RwInt32 *parentIndices,
                       RwSList *kfList[], 
                       RtAnimAnimation *srcAnim)
{
    RwInt32 k, parent, numKeyframes;
    RpHAnimKeyFrame thisKF, *prev, *next;

    RWAPIFUNCTION( RWSTRING( "GetKeyframeWorldSpace" ) );

    RWASSERT(kf != NULL);
    RWASSERT(parentIndices != NULL);
    RWASSERT(kfList != NULL);
    RWASSERT(srcAnim != NULL);

    /* Get the keyframe for this bone. */
    prev = (RpHAnimKeyFrame *) rwSListGetEntry(kfList[boneIndex], 0);
    next = prev;

    numKeyframes = rwSListGetNumEntries(kfList[boneIndex]);

    /* find keyframe pair around kf->time */
    for (k=1; k < numKeyframes; ++k)
    {
        next = (RpHAnimKeyFrame *) rwSListGetEntry(kfList[boneIndex], k);

        if ((prev->time <=time) && (time <= next->time))
        {
            break;
        }

        prev = next;
    }

    /* Interpolate it to get the keyframe for this time. */
    RpHAnimKeyFrameInterpolate(&thisKF, prev, next, time, srcAnim->customData);
    RWASSERT(HAnimKeyFrameValid(&thisKF));

    parent = parentIndices[boneIndex];

    if(parent < 0)
    {
        *kf = thisKF;
    }
    else
    {
        RpHAnimKeyFrame tempKF;
        GetKeyframeWorldSpace(kf, time, parent, parentIndices, kfList, srcAnim);
        HAnimKeyFrameMulti(&tempKF, kf, &thisKF);
        kf->t = tempKF.t;
        kf->q = tempKF.q;
    }

    RWRETURNVOID();
}

static RwInt32
FilterInterpolatesLoopHierCheck(RwSList *kfList[], RwInt32 numBones, 
                                RwInt32 *parentIndices, RtAnimAnimation *srcAnim,
                                RwReal tolerance, RwReal averageNodeSize)
{
    RwInt32 i, j, k, l, numFiltered = 0;
    RwInt32 killSize;
    RwInt32 leaves[MAX_BONES], numLeaves;
    RwSList *kfListCopy[MAX_BONES];
    RpHAnimKeyFrame *kf;
    RtAnimAnimation *copyAnim;
    RwReal  killTimes[MAX_KILL_SIZE];

    RWFUNCTION( RWSTRING( "FilterInterpolatesLoopHierarchyCheck" ) );

    RWASSERT(kfList != NULL);
    RWASSERT(parentIndices != NULL);
    RWASSERT(srcAnim != NULL);
    RWASSERT(numBones <= MAX_BONES);

    copyAnim = HAnimCopy(srcAnim);

    /* Build a list per bone. */
    HAnimBuildBoneLists(kfListCopy, copyAnim, numBones);

    /* For every bone.
       Make sure we are going root->leave, 
       default order is good enough. */
    for (i=0; i<numBones; ++i)
    {
        RwSList *akflist = kfList[i];
        RpHAnimKeyFrame *prev, *next;
        RpHAnimKeyFrame prev2next, kfCopy;
        RwReal length;

        killSize = 0;

        /* Get the list of leaves. */
        HAnimBuildLeavesIndex( leaves, &numLeaves, i,
                               parentIndices, numBones);

        prev = (RpHAnimKeyFrame *)rwSListGetEntry(akflist, 0);
        kf = (RpHAnimKeyFrame *)rwSListGetEntry(akflist, 1);

        length = 0.f;

        if((numLeaves == 1) &&
           (leaves[0] == i))
        {
            length = averageNodeSize;
        }

        for(j = 2; j < rwSListGetNumEntries(akflist); ++j)
        {
            RwBool kill = FALSE;
            RwReal diff;

            next = rwSListGetEntry(akflist, j);
            RpHAnimKeyFrameInterpolate(&prev2next, prev, next, kf->time, srcAnim->customData);

            /* Copy the keyframe. */
            kfCopy.q = kf->q;
            kfCopy.t = kf->t;

            /* Add this keyframe to the killList and repeat. */
            if(killSize < MAX_KILL_SIZE)
            {
                /* Try removing this keyframe and adding it to the killList */
                kill = TRUE;

                killTimes[killSize] = kf->time;
                ++killSize;

                /* And replace it with the interpolated data. */
                kf->q = prev2next.q;
                kf->t = prev2next.t;

                /* Now test the killList. */
                {
                    RpHAnimKeyFrame kf1, kf2;

                    /* For each leaf. */
                    for(l = 0; l < numLeaves; ++l)
                    {
                        /* For each entry in the kill list. */
                        for(k = 0; k < killSize; ++k)
                        {
                            /* Calculate the World position of the nodes. */
                            GetKeyframeWorldSpace(&kf1, killTimes[k], leaves[l], parentIndices,
                                                    kfList, srcAnim);
                            GetKeyframeWorldSpace(&kf2, killTimes[k], leaves[l], parentIndices, 
                                                    kfListCopy, copyAnim);

                            /* Check if it is still ok. */
                            diff = KeyDiff(&kf1, &kf2, length);
                            if(diff > tolerance)
                            {
                                /* Remove the last entry. */
                                --killSize;

                                kill = FALSE;
                                break;
                            }
                        }

                        if(kill == FALSE)
                        {
                            break;
                        }
                    }
                }
            }

            if(kill)
            {
                /* properly remove it now. */
                rwSListDestroyEntry(akflist, j - 1);
                numFiltered++;
                --j;

                /* And update the pointers. */
                prev = rwSListGetEntry(akflist, j - 1);
                kf = rwSListGetEntry(akflist, j);
            }
            else
            {
                /* Add the keyframe back. */
                kf->q = kfCopy.q;
                kf->t = kfCopy.t;

                /* Reset the kill size. */
                killSize = 0;

                /* And update the pointers. */
                prev = kf;
                kf = next;
            }
        }
    }

    /* Delete the kfListCopy. */
    for (i=0; i<numBones; ++i)
    {
        rwSListDestroy(kfListCopy[i]);
    }

    RtAnimAnimationDestroy(copyAnim);
    
    RWRETURN(numFiltered);
}

/**
 * \ingroup rphanim
 * \ref RpHAnimRemoveDuplicates creates a HAnim \ref RtAnimAnimation.
 * This function will ensure consistent hemispheres between keys, and 
 * removes duplicate keys.
 *
 * \param srcAnim Pointer to \ref RtAnimAnimation to optimize.
 * \param numBones \ref RwInt32 containing the number of bones.
 * \param tolerance \ref RwReal how much keyframes can differ, in world space,
 * and still be considered the same. This value is often has to be reduced due
 * to the large effect rotational error have near the top of the hierarchy.
 * \param averageNodeSize \ref RwReal used to convert angle difference to
 * length difference. As this method does not know anything about the hierarchy
 * of a model a good starting value would be half the height of the model.
 *
 */
RtAnimAnimation *
RpHAnimRemoveDuplicates( RtAnimAnimation *srcAnim,
                         RwInt32         numBones,
                         RwReal          tolerance,
                         RwReal          averageNodeSize)
{
    RtAnimAnimation *dstAnim = NULL;

    RwSList *kflist[MAX_BONES];
    RwInt32 i, framesRemoved;

    RWAPIFUNCTION( RWSTRING( "RpHAnimRemoveDuplicates" ) );

    RWASSERT(NULL != srcAnim );
    RWASSERT(numBones < MAX_BONES);

    /* check its correct keyframe type */
    if (srcAnim->interpInfo->typeID != rpHANIMSTDKEYFRAMETYPEID)
    {
        RWRETURN(dstAnim);
    }

    /* Build a list per bone. */
    HAnimBuildBoneLists(kflist, srcAnim, numBones);

    /* firstly normalize Quaternions */
    ConditionKeyframes(kflist, numBones);

    framesRemoved = FilterDuplicates(kflist, numBones, tolerance, averageNodeSize);

    /* lastly normalize Quaternions */
    ConditionKeyframes(kflist, numBones);

    /* Create the output animation. */
    dstAnim = RtAnimAnimationCreate(rpHANIMSTDKEYFRAMETYPEID,
                                    srcAnim->numFrames - framesRemoved,
                                    srcAnim->flags,
                                    srcAnim->duration);

    /* scatter list of keyframes for each bone back to HAnim (fixing up PrevFrame as we go) */
    HAnimScatterBoneLists(kflist, dstAnim, numBones);

    /* clean up (empty) lists */
    for (i=0; i<numBones; ++i)
    {
        rwSListDestroy(kflist[i]);
    }

    RWRETURN(dstAnim);
}

/**
 * \ingroup rphanim
 * \ref RpHAnimAnimationOptimize creates a HAnim \ref RtAnimAnimation.
 * This function will ensure consistent hemispheres between keys, and 
 * removes keys that can be recovered by interpolation. It checks that
 * by removing the keyframe non of the leaves will move by more than the
 * tolerance amount. Note that this algorithm is slow, it may be better to
 * run RpHAnimRemoveDuplicates first.
 *
 * \param srcAnim Pointer to \ref RtAnimAnimation to optimize.
 * \param pushpops Pointer to \ref RwUInt32 containing a list of
 *                 HAnim push/pop flags, of length \e numBones.
 * \param numBones \ref RwInt32 containing the number of bones.
 * \param tolerance \ref RwReal how much keyframes can differ, in world space,
 * and still be considered the same.
 * \param averageNodeSize \ref RwReal used to convert angle difference to
 * length difference, because this method knows about the model hierarchy a 
 * good starting value would be the max length from a node to the furthest skin
 * vertex it directly influences in world space, for a humanoid character this
 * would be about the length of the thigh.
 * \param removeDuplicatesFirst Set this to true to call 
 * \ref RpHAnimRemoveDuplicates first, doing this massively increases 
 * performace for some anims, but produces marginally less reduction.
 *
 */
RtAnimAnimation *
RpHAnimAnimationOptimize( RtAnimAnimation *srcAnim,
                          RwUInt32        *pushpops,
                          RwInt32         numBones,
                          RwReal          tolerance,
                          RwReal          averageNodeSize,
                          RwBool          removeDuplicatesFirst)
{
    RtAnimAnimation *dstAnim = NULL;

    RwSList *kflist[128];
    RwInt32 i, parentIndex[128], framesRemoved = 0;

    RWAPIFUNCTION( RWSTRING( "RpHAnimAnimationOptimize" ) );

    RWASSERT( NULL != srcAnim );
    RWASSERT(numBones < MAX_BONES);

    memset(parentIndex, 0, sizeof(parentIndex));

   /* check its correct keyframe type */
    if (srcAnim->interpInfo->typeID != rpHANIMSTDKEYFRAMETYPEID)
    {
        RWRETURN(dstAnim);
    }

    HAnimBuildParentIndex(parentIndex, pushpops, numBones);
    
    /* Build a list per bone. */
    HAnimBuildBoneLists(kflist, srcAnim, numBones);

    /* Should we remove duplicates? */
    if(removeDuplicatesFirst)
    {        
        framesRemoved = FilterDuplicates(kflist, numBones, tolerance / 10.f, averageNodeSize * 16.f);
    }

    /* firstly normalize Quaternions */
    ConditionKeyframes(kflist, numBones);

    framesRemoved += FilterInterpolatesLoopHierCheck(kflist, numBones, 
                                       parentIndex, srcAnim, tolerance, averageNodeSize);

    /* lastly normalize Quaternions */
    ConditionKeyframes(kflist, numBones);

    /* Create the output animation. */
    dstAnim = RtAnimAnimationCreate(rpHANIMSTDKEYFRAMETYPEID,
                                    srcAnim->numFrames - framesRemoved,
                                    srcAnim->flags,
                                    srcAnim->duration);

    /* scatter list of keyframes for each bone back to HAnim (fixing up PrevFrame as we go) */
    HAnimScatterBoneLists(kflist, dstAnim, numBones);

    /* clean up (empty) lists */
    for (i=0; i<numBones; ++i)
    {
        rwSListDestroy(kflist[i]);
    }

    RWRETURN(dstAnim);
}
