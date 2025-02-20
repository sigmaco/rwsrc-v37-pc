/*
 *
 * Functionality for 2D rendering
 *
 * Copyright (c) 2001 Criterion Software Ltd.
 */

/****************************************************************************
 *                                                                          *
 *  Module  :   keyframe.c                                                  *
 *                                                                          *
 *  Purpose :   Keyframe representation                                     *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>



#include "rwcore.h"
#include <rpdbgerr.h>

#include "keyframe.h"
#include "props.h"
#include "rt2d.h"
#include "rt2danim.h"
#include "gstate.h"

/*-----------------20/08/2001 10:20 AJH -------------
 * Is that really needed ??
 * --------------------------------------------------*/
#if defined (__MWERKS__)
#if (defined(RWVERBOSE))
#pragma message (__FILE__ "/" _SKY_EXPAND(__LINE__) ": __MWERKS__ == " _SKY_EXPAND(__MWERKS__))
#endif /* (defined (__MWERKS__)) */
#if (__option (global_optimizer))
#pragma always_inline off
#endif /* (__option (global_optimizer)) */
#endif /*  defined (__MWERKS__) */

/****************************************************************************
 Local Types
 */

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */
#define FILE_CURRENT_VERSION 0x01
#define FILE_LAST_SUPPORTED_VERSION 0x01

#define GJB_DEBUGx

/****************************************************************************
 Local Macros
 */
#define LockedKeyFrameIteratorMacro(keyframe,                                  \
                                    keyframeSetAction,                         \
                                    transformAction,                           \
                                    colorOffsAction,                           \
                                    colorMultAction,                           \
                                    showAction,                                \
                                    morphAction)                               \
{                                                                              \
    RwUInt8              *actionSet;                                           \
    Rt2dKeyFrameSet  *eKeyframeSet;                                        \
                                                                               \
    actionSet = keyframe->data.unlocked;                                       \
    eKeyframeSet = (Rt2dKeyFrameSet *)actionSet;                           \
                                                                               \
    RWASSERT( actionSet );                                                     \
                                                                               \
    while(0 == (eKeyframeSet->flag & Rt2dFrameIsLast))                         \
    {                                                                          \
        /* call site  for each keyframeset*/                                   \
        keyframeSetAction;                                                     \
        /* get action Set*/                                                    \
        actionSet += sizeof(Rt2dKeyFrameSet);                              \
                                                                               \
        /* Start processing command */                                         \
        if (eKeyframeSet->flag & Rt2dFrameHasTransform)                        \
        {                                                                      \
            /* call site  for each Rt2dKeyFrameTransform*/                     \
            transformAction;                                                   \
            actionSet += sizeof(Rt2dKeyFrameTransform);                        \
        }                                                                      \
                                                                               \
        if (eKeyframeSet->flag & Rt2dFrameHasColorOffs)                        \
        {                                                                      \
            /* call site  for each Rt2dKeyFrameColoroffs*/                     \
            colorOffsAction;                                                   \
            actionSet += sizeof(Rt2dKeyFrameColor);                            \
        }                                                                      \
                                                                               \
        if (eKeyframeSet->flag & Rt2dFrameHasColorMult)                        \
        {                                                                      \
            /* call site  for each Rt2dKeyFrameColorMult*/                     \
            colorMultAction;                                                   \
            actionSet += sizeof(Rt2dKeyFrameColor);                            \
        }                                                                      \
                                                                               \
        if (eKeyframeSet->flag & Rt2dFrameHasShow)                             \
        {                                                                      \
            /* call site  for each Rt2dKeyFrameShow*/                          \
            showAction;                                                        \
            actionSet += sizeof(Rt2dKeyFrameShow);                             \
        }                                                                      \
                                                                               \
        if (eKeyframeSet->flag & Rt2dFrameHasMorph)                            \
        {                                                                      \
            /* call site  for each Rt2dKeyFrameMorph*/                         \
            morphAction;                                                       \
            actionSet += sizeof(Rt2dKeyFrameMorph);                            \
        }                                                                      \
                                                                               \
        eKeyframeSet = (Rt2dKeyFrameSet *)actionSet;                           \
                                                                               \
    }                                                                          \
                                                                               \
}

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */

/****************************************************************************
 Functions
 */

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimObjectUpdateCreate creates an 'updater' for a 2d object that
 * may contain several different kinds of 'update' actions. Rt2dAnimObjectUpdate
 * may be stored for later application to a 2d object.
 *
 * \param object    Pointer to the object to which the update will be applied
 *
 * \return returns the new 'updater'
 *
 * \see Rt2dAnimObjectUpdateDestroy
 * \see Rt2dAnimObjectUpdateClear
 */
Rt2dAnimObjectUpdate *
Rt2dAnimObjectUpdateCreate(Rt2dObject *object)
{
    Rt2dAnimObjectUpdate *update =
                (Rt2dAnimObjectUpdate *)RwFreeListAlloc(&Rt2dAnimGlobals.animObjectUpdateFreeList,
                                                        rwID_2DANIM | rwMEMHINTDUR_EVENT);
    RWAPIFUNCTION(RWSTRING("Rt2dAnimObjectUpdateCreate"));
    RWASSERT(update);

    Rt2dAnimObjectUpdateClear(update);

    Rt2dAnimObjectUpdateSetObject(update,object);

    RWRETURN(update);
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimObjectUpdateDestroy
 *
 * \param update   Pointer to the update to be destroyed
 *
 * \return returns TRUE if the update was successfully destroyed, FALSE otherwise.
 *
 * \see Rt2dAnimObjectUpdateCreate
 * \see Rt2dAnimObjectUpdateClear
 */
RwBool
Rt2dAnimObjectUpdateDestroy(Rt2dAnimObjectUpdate *update)
{
    RwBool result = FALSE;
    RWAPIFUNCTION(RWSTRING("Rt2dAnimObjectUpdateDestroy"));
    RWASSERT(update);

    result = (NULL != update);
    RwFreeListFree(&Rt2dAnimGlobals.animObjectUpdateFreeList, update);

    RWRETURN(result);
}


/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimObjectUpdateClear clears any actions stored in the update.
 *
 * \param update   Pointer to the update to be used
 *
 * \return returns the update that was cleared
 *
 * \see Rt2dAnimObjectUpdateCreate
 * \see Rt2dAnimObjectUpdateDestroy
 */
Rt2dAnimObjectUpdate *
Rt2dAnimObjectUpdateClear(Rt2dAnimObjectUpdate *update)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimObjectUpdateClear"));
    RWASSERT(update);

    update->hasSomething = FALSE;
    update->hasTransform = FALSE;
    update->hasColorOffs = FALSE;
    update->hasColorMult = FALSE;
    update->hasShow = FALSE;
    update->hasMorph = FALSE;

    RWRETURN(update);
}



/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimObjectUpdateSetTransform sets a transform change in an update.
 *
 * \param update     Pointer to the update to be used
 * \param transform  Pointer to the transform to be set
 *
 * \return returns the update with the new transform change in place
 *
 * \see Rt2dAnimObjectUpdateSetColorOffs
 * \see Rt2dAnimObjectUpdateSetColorMult
 * \see Rt2dAnimObjectUpdateSetShow
 * \see Rt2dAnimObjectUpdateSetMorph
 * \see Rt2dAnimObjectUpdateSetObject
 */
Rt2dAnimObjectUpdate*
Rt2dAnimObjectUpdateSetTransform(Rt2dAnimObjectUpdate *update,
                                 Rt2dKeyFrameTransform *transform)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimObjectUpdateSetTransform"));
    RWASSERT(update);
    RWASSERT(transform);

    memcpy( &update->transform, transform, sizeof(Rt2dKeyFrameTransform));

    update->hasTransform = TRUE;
    update->hasSomething = TRUE;

    RWRETURN(update);
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimObjectUpdateSetColorOffs sets a color offset change in an update.
 *
 * \param update   Pointer to the update to be used
 * \param color    Pointer to the color offset to be set
 *
 * \return returns the update with a new color offset change in place
 *
 * \see Rt2dAnimObjectUpdateSetTransform
 * \see Rt2dAnimObjectUpdateSetColorMult
 * \see Rt2dAnimObjectUpdateSetShow
 * \see Rt2dAnimObjectUpdateSetMorph
 * \see Rt2dAnimObjectUpdateSetObject
 */
Rt2dAnimObjectUpdate*
Rt2dAnimObjectUpdateSetColorOffs(Rt2dAnimObjectUpdate *update,
                                 Rt2dKeyFrameColor *color)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimObjectUpdateSetColorOffs"));
    RWASSERT(update);
    RWASSERT(color);

    memcpy( &update->colorOffs, color, sizeof(Rt2dKeyFrameColor));
    update->hasColorOffs = TRUE;
    update->hasSomething = TRUE;

    RWRETURN(update);
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimObjectUpdateSetColorMult sets a color multiplier change in an update.
 *
 * \param update   Pointer to the update to be used
 * \param color    Pointer to the color multiplier to be set
 *
 * \return returns the update with the new color multiplier change in place
 *
 * \see Rt2dAnimObjectUpdateSetTransform
 * \see Rt2dAnimObjectUpdateSetColorOffs
 * \see Rt2dAnimObjectUpdateSetShow
 * \see Rt2dAnimObjectUpdateSetMorph
 * \see Rt2dAnimObjectUpdateSetObject
 */
Rt2dAnimObjectUpdate*
Rt2dAnimObjectUpdateSetColorMult(Rt2dAnimObjectUpdate *update,
                                 Rt2dKeyFrameColor *color)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimObjectUpdateSetColorMult"));
    RWASSERT(update);
    RWASSERT(color);

    memcpy( &update->colorMult, color, sizeof(Rt2dKeyFrameColor));
    update->hasColorMult = TRUE;
    update->hasSomething = TRUE;

    RWRETURN(update);
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimObjectUpdateSetShow sets a show/hide change together with a depth value
 * in an update.
 *
 * \param update   Pointer to the update to be used
 * \param show     Pointer to the show/depth change to be set
 *
 * \return returns the update with the new show/depth change in place
 *
 * \see Rt2dAnimObjectUpdateSetTransform
 * \see Rt2dAnimObjectUpdateSetColorOffs
 * \see Rt2dAnimObjectUpdateSetColorMult
 * \see Rt2dAnimObjectUpdateSetMorph
 * \see Rt2dAnimObjectUpdateSetObject
 */
Rt2dAnimObjectUpdate*
Rt2dAnimObjectUpdateSetShow(Rt2dAnimObjectUpdate *update,
                            Rt2dKeyFrameShow *show)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimObjectUpdateSetShow"));
    RWASSERT(update);
    RWASSERT(show);

    memcpy( &update->show, show, sizeof(Rt2dKeyFrameShow));

    update->hasShow = TRUE;
    update->hasSomething = TRUE;

    RWRETURN(update);
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimObjectUpdateSetMorph sets a morph change in an update.
 *
 * \param update   Pointer to the update to be used
 * \param morph    Pointer to the shape morph to be set
 *
 * \return returns the update with the new morph object change in place
 *
 * \see Rt2dAnimObjectUpdateSetTransform
 * \see Rt2dAnimObjectUpdateSetColorOffs
 * \see Rt2dAnimObjectUpdateSetColorMult
 * \see Rt2dAnimObjectUpdateSetShow
 * \see Rt2dAnimObjectUpdateSetMorph
 * \see Rt2dAnimObjectUpdateSetObject
 */
Rt2dAnimObjectUpdate*
Rt2dAnimObjectUpdateSetMorph(Rt2dAnimObjectUpdate *update,
                             Rt2dKeyFrameMorph *morph)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimObjectUpdateSetMorph"));
    RWASSERT(update);
    RWASSERT(morph);

    memcpy( &update->morph, morph, sizeof(Rt2dKeyFrameMorph));
    update->hasMorph = TRUE;
    update->hasSomething = TRUE;

    RWRETURN(update);
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimObjectUpdateSetObject sets the object that is the target of the update.
 *
 * \param update   Pointer to the update to be used
 * \param object   Pointer to the target object
 *
 * \return returns the update with the new target object
 *
 * \see Rt2dAnimObjectUpdateSetTransform
 * \see Rt2dAnimObjectUpdateSetColorOffs
 * \see Rt2dAnimObjectUpdateSetColorMult
 * \see Rt2dAnimObjectUpdateSetShow
 * \see Rt2dAnimObjectUpdateSetMorph
 */
Rt2dAnimObjectUpdate *
Rt2dAnimObjectUpdateSetObject(Rt2dAnimObjectUpdate *update, Rt2dObject *object)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimObjectUpdateSetObject"));
    RWASSERT(update);

    update->object = object;

    RWRETURN(update);
}


RwSList *
_rt2dKeyFrameListCreateLockedRep(Rt2dAnimProps *props)
{
    Rt2dAnimObjectUpdate *first, *finish, *current;
    Rt2dObject **firstObject, **currentObject;
    RwSList *lockedRep;

    RWFUNCTION(RWSTRING("_rt2dKeyFrameListCreateLockedRep"));
    RWASSERT(props);

    /* Create locked representation from 2D object maestroy */
    lockedRep = rwSListCreate(sizeof(Rt2dAnimObjectUpdate),
                              rwID_2DANIM | rwMEMHINTDUR_EVENT);

    if (rwSListGetNumEntries(props->directory))
    {
        rwSListGetNewEntries(lockedRep, rwSListGetNumEntries(props->directory),
                             rwID_2DANIM | rwMEMHINTDUR_EVENT);

        first = (Rt2dAnimObjectUpdate *)rwSListGetArray( lockedRep );
        finish = first + rwSListGetNumEntries( lockedRep );
        firstObject = (Rt2dObject **)rwSListGetArray(props->directory);

        current = first;
        currentObject = firstObject;
        while (current!=finish)
        {
            Rt2dAnimObjectUpdateClear(current);
            Rt2dAnimObjectUpdateSetObject(current, *currentObject);

            ++current;
            ++currentObject;
        }
    }

    RWRETURN( lockedRep );
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dKeyFrameListCreate creates a new list of keyframes in locked form
 * in order that they may be changed.
 *
 * \param props   Pointer to the props that the keyframe will be applied to
 *
 * \return Returns the new list of keyframes.
 *
 * \see Rt2dKeyFrameListDestroy
 */
Rt2dKeyFrameList *
Rt2dKeyFrameListCreate(Rt2dAnimProps *props)
{
    Rt2dKeyFrameList *keyframeList;

    RWAPIFUNCTION(RWSTRING("Rt2dKeyFrameListCreate"));
    RWASSERT(props);

    /* Allocate basic keyframeList block */
    keyframeList = (Rt2dKeyFrameList*)RwFreeListAlloc(&Rt2dAnimGlobals.keyFrameListFreeList,
                                                      rwID_2DKEYFRAME | rwMEMHINTDUR_EVENT);
    RWASSERT(keyframeList);

    /* Create empty locked representation from props */
    keyframeList->isLocked = TRUE;
    keyframeList->data.locked=_rt2dKeyFrameListCreateLockedRep(props);

    RWRETURN( keyframeList );
}

/****************************************************************************/
/**
 * \ingroup rt2danimsub
 * \ref Rt2dKeyFrameListDestroy destroys a list of keyframes
 *
 * \param keyframeList  Pointer to the keyframeList to destroy
 *
 * \return Returns TRUE if the list of keyframes was destroyed successfully, FALSE otherwise.
 *
 * \see Rt2dKeyFrameListCreate
 */
RwBool
Rt2dKeyFrameListDestroy(Rt2dKeyFrameList *keyframeList)
{
    RwBool result;
    RWAPIFUNCTION(RWSTRING("Rt2dKeyFrameListDestroy"));

    /* NULL path is valid */
    result = (NULL != keyframeList);
    if (result)
    {
        if (keyframeList->isLocked)
        {
            rwSListDestroy(keyframeList->data.locked);
        }
        else
        {
            RwFree(keyframeList->data.unlocked);
        }

        /* Free keyframeList itself */
        RwFreeListFree(&Rt2dAnimGlobals.keyFrameListFreeList, keyframeList);
    }

    RWRETURN( result );
}


/****************************************************************************/
/**
 * \ingroup rt2danimsub
 * \ref Rt2dKeyFrameListLock locks a list of keyframes so that they may be
 * edited
 *
 * \param keyframeList  Pointer to the keyframe list to be locked
 * \param props         Pointer to the props that the list applies to
 *
 * \return Returns the locked list of keyframes.
 *
 * \see Rt2dKeyFrameListUnlock
 */
Rt2dKeyFrameList *
Rt2dKeyFrameListLock(Rt2dKeyFrameList *keyframeList, Rt2dAnimProps *props)
{
    RwSList *locked;
    Rt2dAnimObjectUpdate    *currentUpdate;
    Rt2dKeyFrameTransform   *eTransform;
    Rt2dKeyFrameColor       *eColorMult;
    Rt2dKeyFrameColor       *eColorOffs;
    Rt2dKeyFrameShow        *eShow;
    Rt2dKeyFrameMorph       *eMorph;

    RwUInt32 updateIndex;

    RWAPIFUNCTION(RWSTRING("Rt2dKeyFrameListLock"));
    RWASSERT( keyframeList );

    /* Only convert to locked rep if not already locked */
    if (!keyframeList->isLocked)
    {
        locked = _rt2dKeyFrameListCreateLockedRep(props);

        if( NULL == keyframeList->data.unlocked )
        {
            RWRETURN((Rt2dKeyFrameList*) NULL);
        }

        #define lockKeyFrameAction                                             \
            {updateIndex = eKeyframeSet->objectIdx;                            \
            currentUpdate = (Rt2dAnimObjectUpdate *)                           \
                                _rwSListGetEntry(locked, updateIndex);         \
            Rt2dAnimObjectUpdateClear(currentUpdate);                          \
            Rt2dAnimObjectUpdateSetObject(currentUpdate,                       \
            _rt2dAnimPropsGet2dObjectByIndex(props,updateIndex));}

        #define lockTransformAction                                            \
            {eTransform = (Rt2dKeyFrameTransform*)actionSet;                   \
            Rt2dAnimObjectUpdateSetTransform(currentUpdate, eTransform);}

        #define lockColorOffsAction                                            \
                {eColorOffs = (Rt2dKeyFrameColor*)actionSet;                   \
                Rt2dAnimObjectUpdateSetColorOffs(currentUpdate, eColorOffs);   \
                currentUpdate->colorOffs.color.red =                           \
                        currentUpdate->colorOffs.color.red * (1.0f/255.0f);    \
                currentUpdate->colorOffs.color.green =                         \
                        currentUpdate->colorOffs.color.green * (1.0f/255.0f);  \
                currentUpdate->colorOffs.color.blue  =                         \
                        currentUpdate->colorOffs.color.blue * (1.0f/255.0f);   \
                currentUpdate->colorOffs.color.alpha =                         \
                        currentUpdate->colorOffs.color.alpha * (1.0f/255.0f);  \
                }


        #define lockColorMultAction                                            \
                {eColorMult = (Rt2dKeyFrameColor*)actionSet;                   \
                Rt2dAnimObjectUpdateSetColorMult(currentUpdate, eColorMult);}

        #define lockShowAction                                                 \
                {eShow = (Rt2dKeyFrameShow*)actionSet;                         \
                Rt2dAnimObjectUpdateSetShow(currentUpdate, eShow);}

        #define lockMorphAction                                                \
                {eMorph = (Rt2dKeyFrameMorph*)actionSet;                       \
                Rt2dAnimObjectUpdateSetMorph(currentUpdate, eMorph);           \
                currentUpdate->morph.source =                                  \
                        _rt2dAnimPropsGet2dObjectByIndex(                      \
                                  props,(RwInt32)currentUpdate->morph.source); \
                currentUpdate->morph.destination =                             \
                        _rt2dAnimPropsGet2dObjectByIndex(                      \
                            props,(RwInt32)currentUpdate->morph.destination);}


        LockedKeyFrameIteratorMacro(keyframeList,
                                    lockKeyFrameAction,
                                    lockTransformAction,
                                    lockColorOffsAction,
                                    lockColorMultAction,
                                    lockShowAction,
                                    lockMorphAction);

        RwFree(keyframeList->data.unlocked);

        /* And store */
        keyframeList->data.locked = locked;
        keyframeList->isLocked = TRUE;

        RWRETURN( keyframeList );

    }

    RWRETURN((Rt2dKeyFrameList *)NULL );
}
/****************************************************************************/

/**
 * \ingroup rt2danimsub
 * \ref Rt2dKeyFrameListUnlock unlocks a list of keyframes in order that they
 * may be changed
 *
 * \param keyframeList    Pointer to the keyframe list to be unlocked
 * \param props           Pointer to the props that the keyframe list will
 *                        be applied to
 *
 * \return Returns the unlocked list of keyframes
 *
 * \see Rt2dKeyFrameListLock
 */
Rt2dKeyFrameList *
Rt2dKeyFrameListUnlock(Rt2dKeyFrameList *keyframeList,
                       Rt2dAnimProps *props)
{
    RwUInt8 *unlocked;
    RwUInt32 size=0;
    RwInt32 activeFrames = 0;
    Rt2dAnimObjectUpdate *current, *first, *end;
    RwUInt8 *actionSet;
    Rt2dKeyFrameTransform   *eTransform;
    Rt2dKeyFrameColor       *eColorOffs;
    Rt2dKeyFrameColor       *eColorMult;
    Rt2dKeyFrameShow        *eShow;
    Rt2dKeyFrameMorph       *eMorph;
    Rt2dKeyFrameSet         *eKeyFrameSet;

    RWAPIFUNCTION(RWSTRING("Rt2dKeyFrameListUnlock"));
    RWASSERT( keyframeList );

    /* Only convert to unlocked rep if locked */
    if (keyframeList->isLocked)
    {

        first = (Rt2dAnimObjectUpdate *)
                   rwSListGetArray(keyframeList->data.locked);
        end   = first + rwSListGetNumEntries(keyframeList->data.locked);

        /* Work out size for unlocked rep */
        current = first;
        while (current != end)
        {
            if(current->hasSomething)
            {
                activeFrames++;
                /* Header */
                size += sizeof(Rt2dKeyFrameSet);

                /* Transform */
                if (current->hasTransform)
                {
                    size += sizeof(Rt2dKeyFrameTransform);
                }

                /* Color */
                if (current->hasColorOffs)
                {
                    size += sizeof(Rt2dKeyFrameColor);
                }

                /* Color */
                if (current->hasColorMult)
                {
                    size += sizeof(Rt2dKeyFrameColor);
                }

                /* Show/hide */
                if (current->hasShow)
                {
                    size += sizeof(Rt2dKeyFrameShow);
                }
                /* Morph */
                if (current->hasMorph)
                {
                    size += sizeof(Rt2dKeyFrameMorph);
                }
            }

            ++current;
        }

        size += sizeof(Rt2dKeyFrameSet);  /* End frame */

        /* Allocate space for unlocked rep */
        unlocked = (RwUInt8 *)RwMalloc(size, rwID_2DANIM | rwMEMHINTDUR_EVENT);
        memset(unlocked,0,size);

        /* Convert unlocked rep */
        current = first;
        actionSet = unlocked;
        while (current != end)
        {
            /* Create command */
            if(current->hasSomething)
            {
                eKeyFrameSet = (Rt2dKeyFrameSet *)actionSet;
                eKeyFrameSet->flag =
                        (current->hasTransform ? Rt2dFrameHasTransform : 0) |
                        (current->hasColorOffs ? Rt2dFrameHasColorOffs : 0) |
                        (current->hasColorMult ? Rt2dFrameHasColorMult : 0) |
                        (current->hasShow      ? Rt2dFrameHasShow      : 0) |
                        (current->hasMorph     ? Rt2dFrameHasMorph     : 0);

                /* Should made into an idx using the props */
                eKeyFrameSet->objectIdx =
                        _rt2dAnimPropsGetIndexBy2dObject(props,current->object);

                /* Advance past command header */
                actionSet += sizeof(Rt2dKeyFrameSet);

                /* Individual actions */
                if (current->hasTransform)
                {
                    eTransform = (Rt2dKeyFrameTransform*)actionSet;

                    memcpy(eTransform,
                            &current->transform,
                            sizeof(Rt2dKeyFrameTransform));

                    actionSet+=sizeof(Rt2dKeyFrameTransform);
                }

                if (current->hasColorOffs)
                {
                    eColorOffs = (Rt2dKeyFrameColor*)actionSet;

                    memcpy(eColorOffs,
                            &current->colorOffs,
                            sizeof(Rt2dKeyFrameColor));

                        eColorOffs->color.red   =
                                current->colorOffs.color.red * (255.0f);
                        eColorOffs->color.green =
                                current->colorOffs.color.green * (255.0f);
                        eColorOffs->color.blue  =
                                current->colorOffs.color.blue * (255.0f);
                        eColorOffs->color.alpha =
                                current->colorOffs.color.alpha * (255.0f);

                    actionSet+=sizeof(Rt2dKeyFrameColor);
                }

                if (current->hasColorMult)
                {
                    eColorMult = (Rt2dKeyFrameColor*)actionSet;

                    memcpy(eColorMult,
                            &current->colorMult,
                            sizeof(Rt2dKeyFrameColor));

                    actionSet+=sizeof(Rt2dKeyFrameColor);
                }

                if (current->hasShow)
                {
                    eShow = (Rt2dKeyFrameShow*)actionSet;

                    memcpy(eShow,&current->show,sizeof(Rt2dKeyFrameShow));

                    actionSet+=sizeof(Rt2dKeyFrameShow);
                }

                if (current->hasMorph)
                {
                    eMorph = (Rt2dKeyFrameMorph*)actionSet;

                    memcpy(eMorph,
                            &current->morph,
                            sizeof(Rt2dKeyFrameMorph));
                    eMorph->source = (Rt2dObject *)(
                        (RwUInt32)(
                            _rt2dAnimPropsGetIndexBy2dObject(
                                        props,
                                        eMorph->source)
                        )
                    );

                    eMorph->destination = (Rt2dObject *)(
                        (RwUInt32)(
                            _rt2dAnimPropsGetIndexBy2dObject(
                                        props,
                                        eMorph->destination)
                        )
                    );

                    actionSet+=sizeof(Rt2dKeyFrameMorph);

                }

            }
            /* Next command */
            ++current;
        }

        eKeyFrameSet = (Rt2dKeyFrameSet *)actionSet;
        eKeyFrameSet->flag = Rt2dFrameIsLast;


        /* Trash locked rep */
        rwSListDestroy(keyframeList->data.locked);

        /* And store */
        keyframeList->data.unlocked = unlocked;
        keyframeList->isLocked = FALSE;

        RWRETURN( keyframeList );
    }

    RWRETURN((Rt2dKeyFrameList *)NULL );
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dKeyFrameListAddUpdateObject adds a set of updates on a particular
 * object to a keyframe list. The keyframe list must be locked first.
 *
 * \param keyframeList    Pointer to the keyframe list to use
 * \param update          Pointer to the update object that contains the changes
 *                        to be applied
 *
 * \return Returns the keyframe list the set of updates was added to if successful,
 * NULL otherwise.
 *
 * \see Rt2dKeyFrameListLock
 */
Rt2dKeyFrameList*
Rt2dKeyFrameListAddUpdateObject(Rt2dKeyFrameList *keyframeList,
                                Rt2dAnimObjectUpdate *update)
{
    Rt2dAnimObjectUpdate    *updateList ;
    RwInt32                 updateCount,i;

    RWAPIFUNCTION(RWSTRING("Rt2dKeyFrameListAddUpdateObject"));
    RWASSERT( keyframeList );
    RWASSERT( keyframeList->isLocked );
    RWASSERT( update );

    updateList = (Rt2dAnimObjectUpdate *)
                    rwSListGetArray( keyframeList->data.locked );
    updateCount = rwSListGetNumEntries( keyframeList->data.locked );

    for(i=0;i<updateCount;i++)
    {
        if( update->object == updateList->object)
        {
            memcpy(updateList,update,sizeof(Rt2dAnimObjectUpdate));
            RWRETURN( keyframeList );
        }

        updateList++;
    }


    RWRETURN((Rt2dKeyFrameList*) NULL);
}

/* Streaming */
/****************************************************************************/
static RwUInt32
rt2dKeyFrameSetStreamGetSize(Rt2dKeyFrameSet *keyframeSet __RWUNUSED__)
{
    RwUInt32 size;
    RWFUNCTION(RWSTRING("rt2dKeyFrameSetStreamGetSize"));
    RWASSERT(keyframeSet);

    size = sizeof(RwInt32)*2;

    RWRETURN(size);
}

static RwUInt32
rt2dKeyFrameTransformStreamGetSize(Rt2dKeyFrameTransform *transform)
{
    RwUInt32 size;
    RWFUNCTION(RWSTRING("rt2dKeyFrameTransformStreamGetSize"));
    RWASSERT(transform);

    size = RwMatrixStreamGetSize(&transform->matrix);

    RWRETURN(size);
}

static RwUInt32
rt2dKeyFrameColorStreamGetSize(Rt2dKeyFrameColor *color __RWUNUSED__)
{
    RwUInt32 size;
    RWFUNCTION(RWSTRING("rt2dKeyFrameColorStreamGetSize"));
    RWASSERT(color);

    size = sizeof(RwRGBAReal);

    RWRETURN(size);
}

static RwUInt32
rt2dKeyFrameShowStreamGetSize(Rt2dKeyFrameShow *show __RWUNUSED__)
{
    RwUInt32 size;
    RWFUNCTION(RWSTRING("rt2dKeyFrameShowStreamGetSize"));
    RWASSERT(show);

    size = sizeof(RwInt32)*2;

    RWRETURN(size);
}

static RwUInt32
rt2dKeyFrameMorphStreamGetSize(Rt2dKeyFrameMorph *morph __RWUNUSED__)
{
    RwUInt32 size;
    RWFUNCTION(RWSTRING("rt2dKeyFrameMorphStreamGetSize"));
    RWASSERT(morph);

    size = sizeof(RwInt32)*2;
    size += sizeof(RwReal);

    RWRETURN(size);
}

/****************************************************************************/
static Rt2dKeyFrameSet *
rt2dKeyFrameSetStreamWrite(Rt2dKeyFrameSet *keyframeSet, RwStream *stream)
{
    RwInt32 keyframeSetInfo[2];

    RWFUNCTION(RWSTRING("rt2dKeyFrameSetStreamWrite"));
    RWASSERT(keyframeSet);
    RWASSERT(stream);

    keyframeSetInfo[0] = keyframeSet->flag;
    keyframeSetInfo[1] = keyframeSet->objectIdx;

    (void)RwMemLittleEndian32(&keyframeSetInfo, sizeof(keyframeSetInfo));

    if (!RwStreamWrite(stream, &keyframeSetInfo, sizeof(keyframeSetInfo)))
    {
        RWRETURN((Rt2dKeyFrameSet *)NULL);
    }

    RWRETURN(keyframeSet);
}

static Rt2dKeyFrameTransform *
rt2dKeyFrameTransformStreamWrite(Rt2dKeyFrameTransform *transform, RwStream *stream)
{
    RWFUNCTION(RWSTRING("rt2dKeyFrameTransformStreamWrite"));
    RWASSERT(transform);
    RWASSERT(stream);

    if (!RwMatrixStreamWrite(&transform->matrix, stream))
    {
        RWRETURN((Rt2dKeyFrameTransform *)NULL);
    }

    RWRETURN(transform);
}

static Rt2dKeyFrameColor *
rt2dKeyFrameColorStreamWrite(Rt2dKeyFrameColor *color, RwStream *stream)
{
    RwReal colors[4];

    RWFUNCTION(RWSTRING("rt2dKeyFrameColorStreamWrite"));
    RWASSERT(color);
    RWASSERT(stream);

    colors[0] = color->color.red;
    colors[1] = color->color.green;
    colors[2] = color->color.blue;
    colors[3] = color->color.alpha;

    (void)RwMemRealToFloat32(&colors, sizeof(colors));
    (void)RwMemLittleEndian32(&colors, sizeof(colors));

    if (!RwStreamWrite(stream, &colors, sizeof(colors)))
    {
        RWRETURN((Rt2dKeyFrameColor *)NULL);
    }

    RWRETURN(color);
}

static Rt2dKeyFrameShow *
rt2dKeyFrameShowStreamWrite(Rt2dKeyFrameShow *show, RwStream *stream)
{
    RwInt32 showinf[2];

    RWFUNCTION(RWSTRING("rt2dKeyFrameShowStreamWrite"));
    RWASSERT(show);
    RWASSERT(stream);

    showinf[0] = show->show ? 1 : 0;
    showinf[1] = show->depth;

    (void)RwMemLittleEndian32(&showinf, sizeof(showinf));

    if (!RwStreamWrite(stream, &showinf, sizeof(showinf)))
    {
        RWRETURN((Rt2dKeyFrameShow *)NULL);
    }

    RWRETURN(show);
}

static Rt2dKeyFrameMorph *
rt2dKeyFrameMorphStreamWrite(Rt2dKeyFrameMorph *morph, RwStream *stream)
{
    Rt2dKeyFrameMorph smorph;

    RWFUNCTION(RWSTRING("rt2dKeyFrameMorphStreamWrite"));
    RWASSERT(morph);
    RWASSERT(stream);

    smorph = *morph;

    (void)RwMemRealToFloat32(&smorph.alpha, sizeof(smorph));

    (void)RwMemLittleEndian32(&smorph, sizeof(smorph) );

    if (!RwStreamWrite(stream, &smorph, sizeof(smorph)))
    {
        RWRETURN((Rt2dKeyFrameMorph *)NULL);
    }

    RWRETURN(morph);
}

/****************************************************************************/
static Rt2dKeyFrameSet *
rt2dKeyFrameSetStreamReadTo(Rt2dKeyFrameSet *keyframeSet, RwStream *stream)
{
    RwInt32 keyframeSetInfo[2];

    RWFUNCTION(RWSTRING("rt2dKeyFrameSetStreamReadTo"));
    RWASSERT(keyframeSet);
    RWASSERT(stream);

    if (RwStreamRead(stream, &keyframeSetInfo, sizeof(keyframeSetInfo))
             != sizeof(keyframeSetInfo))
    {
        RWRETURN((Rt2dKeyFrameSet *)NULL);
    }

    (void)RwMemNative32(&keyframeSetInfo, sizeof(keyframeSetInfo));

    keyframeSet->flag = keyframeSetInfo[0];
    keyframeSet->objectIdx = keyframeSetInfo[1];

    RWRETURN(keyframeSet);
}

static Rt2dKeyFrameTransform *
rt2dKeyFrameTransformStreamReadTo(Rt2dKeyFrameTransform *transform, RwStream *stream)
{
    RWFUNCTION(RWSTRING("rt2dKeyFrameTransformStreamReadTo"));
    RWASSERT(transform);
    RWASSERT(stream);

    if( !RwStreamFindChunk(stream, rwID_MATRIX,
                            (RwUInt32 *)NULL, (RwUInt32 *)NULL) )
    {
        RWRETURN((Rt2dKeyFrameTransform *)NULL);
    }

    if (!RwMatrixStreamRead(stream, &transform->matrix))
    {
        RWRETURN((Rt2dKeyFrameTransform *)NULL);
    }

    RWRETURN(transform);
}

static Rt2dKeyFrameColor *
rt2dKeyFrameColorStreamReadTo(Rt2dKeyFrameColor *color, RwStream *stream)
{
    RwReal colors[4];

    RWFUNCTION(RWSTRING("rt2dKeyFrameColorStreamReadTo"));
    RWASSERT(color);
    RWASSERT(stream);

    if (RwStreamRead(stream, &colors, sizeof(colors))
            != sizeof(colors))
    {
        RWRETURN((Rt2dKeyFrameColor *)NULL);
    }

    (void)RwMemNative32(&colors, sizeof(colors));
    (void)RwMemFloat32ToReal(&colors, sizeof(colors));

    color->color.red = colors[0];
    color->color.green = colors[1];
    color->color.blue = colors[2];
    color->color.alpha = colors[3];

    RWRETURN(color);
}

static Rt2dKeyFrameShow *
rt2dKeyFrameShowStreamReadTo(Rt2dKeyFrameShow *show, RwStream *stream)
{
    RwInt32 showinf[2];

    RWFUNCTION(RWSTRING("rt2dKeyFrameShowStreamReadTo"));
    RWASSERT(show);
    RWASSERT(stream);

    if (RwStreamRead(stream, &showinf, sizeof(showinf))
            != sizeof(showinf))
    {
        RWRETURN((Rt2dKeyFrameShow *)NULL);
    }

    (void)RwMemNative32(&showinf, sizeof(showinf));

    show->show = showinf[0] ? TRUE : FALSE;
    show->depth = showinf[1];

    RWRETURN(show);
}

static Rt2dKeyFrameMorph *
rt2dKeyFrameMorphStreamReadTo(Rt2dKeyFrameMorph *morph, RwStream *stream)
{
    Rt2dKeyFrameMorph smorph;

    RWFUNCTION(RWSTRING("rt2dKeyFrameMorphStreamReadTo"));
    RWASSERT(morph);
    RWASSERT(stream);

    if (RwStreamRead(stream, &smorph, sizeof(smorph))
            != sizeof(smorph))
    {
        RWRETURN((Rt2dKeyFrameMorph *)NULL);
    }

    (void)RwMemNative32(&smorph, sizeof(smorph) );
    (void)RwMemFloat32ToReal(&smorph.alpha, sizeof(smorph));

    *morph = smorph;

    RWRETURN(morph);
}


/****************************************************************************/
static RwUInt32
rt2dKeyFrameListStreamGetSize(Rt2dKeyFrameList *keyframeList)
{
    RwUInt32 size=0;

    RWFUNCTION(RWSTRING("rt2dKeyFrameListStreamGetSize"));
    RWASSERT( keyframeList );
    RWASSERT( FALSE == keyframeList->isLocked );

    #define SGSKeyFrameAction                                              \
        size += rt2dKeyFrameSetStreamGetSize(eKeyframeSet);

    #define SGSTransformAction                                             \
            size += rt2dKeyFrameTransformStreamGetSize(                    \
                    (Rt2dKeyFrameTransform *)actionSet);

    #define SGSColorOffsAction                                             \
            size += rt2dKeyFrameColorStreamGetSize(                        \
                    (Rt2dKeyFrameColor *)actionSet);

    #define SGSColorMultAction                                             \
            size += rt2dKeyFrameColorStreamGetSize(                        \
                    (Rt2dKeyFrameColor *)actionSet);

    #define SGSShowAction                                                  \
            size += rt2dKeyFrameShowStreamGetSize(                         \
                    (Rt2dKeyFrameShow *)actionSet);

    #define SGSMorphAction                                                 \
            size += rt2dKeyFrameMorphStreamGetSize(                        \
                    (Rt2dKeyFrameMorph *)actionSet);


    LockedKeyFrameIteratorMacro(keyframeList,
                                SGSKeyFrameAction,
                                SGSTransformAction,
                                SGSColorOffsAction,
                                SGSColorMultAction,
                                SGSShowAction,
                                SGSMorphAction);

    RWRETURN(size);
}

/****************************************************************************/
static RwUInt32
rt2dKeyFrameListGetNumKeyFrames(Rt2dKeyFrameList *keyframeList)
{
    RwInt32 count=0;

    RWFUNCTION(RWSTRING("rt2dKeyFrameListGetNumKeyFrames"));
    RWASSERT( keyframeList );
    RWASSERT( FALSE == keyframeList->isLocked );

    #define GNFKeyFrameAction                                              \
        count++;

    #define GNFTransformAction

    #define GNFColorOffsAction

    #define GNFColorMultAction

    #define GNFShowAction

    #define GNFMorphAction


    LockedKeyFrameIteratorMacro(keyframeList,
                                GNFKeyFrameAction,
                                GNFTransformAction,
                                GNFColorOffsAction,
                                GNFColorMultAction,
                                GNFShowAction,
                                GNFMorphAction);
    RWRETURN(count);
}
/****************************************************************************/

/**
 * \ingroup rt2danimsub
 * \ref Rt2dKeyFrameListStreamGetSize is used to determine the size in bytes of
 * the binary representation of a keyframe list. This value is used in the
 * binary chunk header to indicate the size of the chunk. The size includes
 * the size of the chunk header.
 *
 * \param keyframeList   Pointer to a list of keyframes
 *
 * \return Returns a \ref RwUInt32 value equal to the chunk size (in bytes)
 * of the shape.
 *
 * \see Rt2dKeyFrameListStreamRead
 * \see Rt2dKeyFrameListStreamWrite
 */
RwUInt32
Rt2dKeyFrameListStreamGetSize(Rt2dKeyFrameList *keyframeList)
{
    RwUInt32 size;

    RWAPIFUNCTION(RWSTRING("Rt2dKeyFrameListStreamGetSize"));
    RWASSERT( keyframeList );
    RWASSERT( FALSE == keyframeList->isLocked );

    size =  rwCHUNKHEADERSIZE;

    /* header */
    size += sizeof(_rt2dStreamKeyFrameList);

    /* KeyFrames */
    size += rt2dKeyFrameListStreamGetSize(keyframeList);

    RWRETURN( size );
}
/****************************************************************************/

/**
 * \ingroup rt2danimsub
 * \ref Rt2dKeyFrameListStreamWrite is used to write the specified keyframe
 * list to the given binary stream. Note that the stream will have been
 * opened prior to this function call.
 *
 * \param keyframeList   Pointer to the keyframe list to stream.
 * \param stream         Pointer to the stream to write to.
 *
 * \return Returns a pointer to the keyframe list if successful, NULL otherwise.
 *
 * \see Rt2dKeyFrameListStreamRead
 * \see Rt2dKeyFrameListStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 */
Rt2dKeyFrameList *
Rt2dKeyFrameListStreamWrite(Rt2dKeyFrameList *keyframeList, RwStream *stream)
{
    RwInt32 size;

    _rt2dStreamKeyFrameList skeyframeList;
    RWAPIFUNCTION(RWSTRING("Rt2dKeyFrameListStreamWrite"));
    RWASSERT( keyframeList );
    RWASSERT( stream );
    RWASSERT( FALSE == keyframeList->isLocked );


    /* Writing header */
    size = Rt2dKeyFrameListStreamGetSize(keyframeList);

    if (!RwStreamWriteChunkHeader(stream, rwID_2DKEYFRAME, size))
    {
        RWRETURN((Rt2dKeyFrameList *)NULL);
    }

    skeyframeList.version = FILE_CURRENT_VERSION;
    skeyframeList.time = keyframeList->time;
    skeyframeList.keyframeCount = rt2dKeyFrameListGetNumKeyFrames(keyframeList);

    (void)RwMemRealToFloat32(&skeyframeList.time, sizeof(skeyframeList.time));
    (void)RwMemLittleEndian32(&skeyframeList, sizeof(skeyframeList));

    if (!RwStreamWrite(stream, &skeyframeList, sizeof(skeyframeList)))
    {
        RWRETURN((Rt2dKeyFrameList *)NULL);
    }

    #define SWKeyFrameAction                                              \
        if( !rt2dKeyFrameSetStreamWrite(eKeyframeSet, stream))            \
        {                                                                 \
            RWRETURN((Rt2dKeyFrameList *)NULL);                           \
        }

    #define SWTransformAction                                             \
            if( !rt2dKeyFrameTransformStreamWrite(                        \
                    (Rt2dKeyFrameTransform *)actionSet, stream))          \
            {                                                             \
                RWRETURN((Rt2dKeyFrameList *)NULL);                       \
            }

    #define SWColorOffsAction                                             \
            if( !rt2dKeyFrameColorStreamWrite(                            \
                    (Rt2dKeyFrameColor *)actionSet, stream))              \
            {                                                             \
                RWRETURN((Rt2dKeyFrameList *)NULL);                       \
            }

    #define SWColorMultAction                                             \
            if( !rt2dKeyFrameColorStreamWrite(                            \
                    (Rt2dKeyFrameColor *)actionSet, stream))              \
            {                                                             \
                RWRETURN((Rt2dKeyFrameList *)NULL);                           \
            }

    #define SWShowAction                                                  \
            if( !rt2dKeyFrameShowStreamWrite(                             \
                    (Rt2dKeyFrameShow *)actionSet, stream))               \
            {                                                             \
                RWRETURN((Rt2dKeyFrameList *)NULL);                       \
            }

    #define SWMorphAction                                                 \
            if( !rt2dKeyFrameMorphStreamWrite(                            \
                    (Rt2dKeyFrameMorph *)actionSet, stream))              \
            {                                                             \
                RWRETURN((Rt2dKeyFrameList *)NULL);                       \
            }


    LockedKeyFrameIteratorMacro(keyframeList,
                                SWKeyFrameAction,
                                SWTransformAction,
                                SWColorOffsAction,
                                SWColorMultAction,
                                SWShowAction,
                                SWMorphAction);

    RWRETURN( keyframeList );
}

Rt2dKeyFrameList *
_rt2dKeyFrameListStreamReadTo(RwStream *stream, Rt2dKeyFrameList *keyframeList)
{
    _rt2dStreamKeyFrameList skeyframeList;
    RwInt32 size;
    Rt2dAnimObjectUpdate *keyframeTmp = (Rt2dAnimObjectUpdate *)NULL;
    Rt2dKeyFrameSet aKeyframeSet;
    RwInt32 i;
    RwUInt8 *actionSet = (RwUInt8 *)NULL;
    RwUInt8 *unlocked = (RwUInt8 *)NULL;
    Rt2dKeyFrameSet *eKeyFrameSet;
    Rt2dKeyFrameTransform *eTransform;
    Rt2dKeyFrameColor *eColorOffs;
    Rt2dKeyFrameColor *eColorMult;
    Rt2dKeyFrameShow *eShow;
    Rt2dKeyFrameMorph *eMorph;


    RWFUNCTION(RWSTRING("_rt2dKeyFrameListStreamReadTo"));
    RWASSERT( keyframeList );
    RWASSERT( stream );

    /* Read header */
    if (RwStreamRead(stream, &skeyframeList, sizeof(skeyframeList))
                                          != sizeof(skeyframeList))
    {
        RWRETURN((Rt2dKeyFrameList *)NULL);
    }

    (void)RwMemNative32(&skeyframeList, sizeof(skeyframeList));
    (void)RwMemFloat32ToReal(&skeyframeList.time, sizeof(skeyframeList.time));

    RWASSERT(skeyframeList.version >= FILE_LAST_SUPPORTED_VERSION
            && skeyframeList.version <=FILE_CURRENT_VERSION );

    keyframeList->time = skeyframeList.time;

    size = 0;

    if (skeyframeList.keyframeCount)
    {
        /*use Rt2dAnimObjectUpdate as it's easier to handle : the global size is
         * not known yet
         */

        keyframeTmp = (Rt2dAnimObjectUpdate *)
                            RwMalloc(sizeof(Rt2dAnimObjectUpdate)
                                     *skeyframeList.keyframeCount,
                                     rwID_2DANIM | rwMEMHINTDUR_FUNCTION);
        if( NULL == keyframeTmp )
        {
            RWRETURN((Rt2dKeyFrameList *)NULL);
        }

        for(i=0;i<skeyframeList.keyframeCount;i++)
        {
            if( !rt2dKeyFrameSetStreamReadTo(&aKeyframeSet, stream) )
            {
                RwFree(keyframeTmp);
                RWRETURN((Rt2dKeyFrameList *)NULL);
            }

            /* Header */
            size += sizeof(Rt2dKeyFrameSet);

            keyframeTmp[i].object = (Rt2dObject *)aKeyframeSet.objectIdx;
            keyframeTmp[i].hasSomething = TRUE;
            keyframeTmp[i].hasTransform = FALSE;
            keyframeTmp[i].hasColorOffs = FALSE;
            keyframeTmp[i].hasColorMult = FALSE;
            keyframeTmp[i].hasShow = FALSE;
            keyframeTmp[i].hasMorph = FALSE;

            if (aKeyframeSet.flag & Rt2dFrameHasTransform)
            {
                if( !rt2dKeyFrameTransformStreamReadTo(
                        &keyframeTmp[i].transform, stream))
                {
                    RwFree(keyframeTmp);
                    RWRETURN((Rt2dKeyFrameList *)NULL);
                }

                keyframeTmp[i].hasTransform = TRUE;

                /* Transform */
                size += sizeof(Rt2dKeyFrameTransform);
            }

            if (aKeyframeSet.flag & Rt2dFrameHasColorOffs)
            {
                if ( !rt2dKeyFrameColorStreamReadTo(
                        &keyframeTmp[i].colorOffs, stream))
                {
                    RwFree(keyframeTmp);
                    RWRETURN((Rt2dKeyFrameList *)NULL);
                }

                keyframeTmp[i].hasColorOffs = TRUE;

                /* Color */
                size += sizeof(Rt2dKeyFrameColor);
            }

            if (aKeyframeSet.flag & Rt2dFrameHasColorMult)
            {
                if ( !rt2dKeyFrameColorStreamReadTo(
                        &keyframeTmp[i].colorMult, stream))
                {
                    RwFree(keyframeTmp);
                    RWRETURN((Rt2dKeyFrameList *)NULL);
                }

                keyframeTmp[i].hasColorMult = TRUE;

                /* Color */
                size += sizeof(Rt2dKeyFrameColor);
            }


            if (aKeyframeSet.flag & Rt2dFrameHasShow)
            {
                if ( !rt2dKeyFrameShowStreamReadTo(
                        &keyframeTmp[i].show, stream))
                {
                    RwFree(keyframeTmp);
                    RWRETURN((Rt2dKeyFrameList *)NULL);
                }

                keyframeTmp[i].hasShow = TRUE;

                /* Show/hide */
                size += sizeof(Rt2dKeyFrameShow);
            }

            if (aKeyframeSet.flag & Rt2dFrameHasMorph)
            {
                if ( !rt2dKeyFrameMorphStreamReadTo(
                        &keyframeTmp[i].morph, stream))
                {
                    RwFree(keyframeTmp);
                    RWRETURN((Rt2dKeyFrameList *)NULL);
                }

                keyframeTmp[i].hasMorph = TRUE;
                /* Morph */
                size += sizeof(Rt2dKeyFrameMorph);
            }

        }
    }

    size += sizeof(Rt2dKeyFrameSet);  /* End frame */

     /* Allocate the final unlocked frame */
    unlocked = (RwUInt8 *)RwMalloc(size, rwID_2DANIM | rwMEMHINTDUR_EVENT);
    actionSet = unlocked;
    /* convert to a real unlocked representation */
    for(i=0;i<skeyframeList.keyframeCount;i++)
    {
        /* Create command */
        RWASSERT(TRUE == keyframeTmp[i].hasSomething);

        eKeyFrameSet = (Rt2dKeyFrameSet *)actionSet;

        eKeyFrameSet->flag =
                (keyframeTmp[i].hasTransform ? Rt2dFrameHasTransform : 0) |
                (keyframeTmp[i].hasColorOffs ? Rt2dFrameHasColorOffs : 0) |
                (keyframeTmp[i].hasColorMult ? Rt2dFrameHasColorMult : 0) |
                (keyframeTmp[i].hasShow      ? Rt2dFrameHasShow      : 0) |
                (keyframeTmp[i].hasMorph     ? Rt2dFrameHasMorph     : 0);

        /* Should made into an idx using the props */
        eKeyFrameSet->objectIdx = (RwInt32)keyframeTmp[i].object;

        /* Advance past command header */
        actionSet += sizeof(Rt2dKeyFrameSet);

        /* Individual actions */
        if (keyframeTmp[i].hasTransform)
        {
            eTransform = (Rt2dKeyFrameTransform*)actionSet;

            memcpy(eTransform,
                    &keyframeTmp[i].transform,
                    sizeof(Rt2dKeyFrameTransform));

            actionSet+=sizeof(Rt2dKeyFrameTransform);
        }

        if (keyframeTmp[i].hasColorOffs)
        {
            eColorOffs = (Rt2dKeyFrameColor*)actionSet;

            memcpy(eColorOffs,
                    &keyframeTmp[i].colorOffs,
                    sizeof(Rt2dKeyFrameColor));

            actionSet+=sizeof(Rt2dKeyFrameColor);
        }

        if (keyframeTmp[i].hasColorMult)
        {
            eColorMult = (Rt2dKeyFrameColor*)actionSet;

            memcpy(eColorMult,
                    &keyframeTmp[i].colorMult,
                    sizeof(Rt2dKeyFrameColor));

            actionSet+=sizeof(Rt2dKeyFrameColor);
        }

        if (keyframeTmp[i].hasShow)
        {
            eShow = (Rt2dKeyFrameShow*)actionSet;

            memcpy(eShow,&keyframeTmp[i].show,sizeof(Rt2dKeyFrameShow));

            actionSet+=sizeof(Rt2dKeyFrameShow);
        }

        if (keyframeTmp[i].hasMorph)
        {
            eMorph = (Rt2dKeyFrameMorph*)actionSet;

            memcpy(eMorph,
                    &keyframeTmp[i].morph,
                    sizeof(Rt2dKeyFrameMorph));

            actionSet+=sizeof(Rt2dKeyFrameMorph);

        }
    }

    eKeyFrameSet = (Rt2dKeyFrameSet *)actionSet;

    eKeyFrameSet->flag = Rt2dFrameIsLast;

    keyframeList->isLocked = FALSE;
    keyframeList->data.unlocked = unlocked;

    if (keyframeTmp)
    {
        RwFree(keyframeTmp);
    }

    RWRETURN(keyframeList);

}
/****************************************************************************/

/**
 * \ingroup rt2danimsub
 * \ref Rt2dKeyFrameListStreamRead reads a keyframe from a binary stream.
 * Note that prior to this function call a binary keyframe chunk must be found
 * in the stream using the \ref RwStreamFindChunk API function.
 *
 * The sequence to locate and read a keyframe from a binary stream is
 * as follows:
 * \code
   RwStream *stream;
   Rt2dKeyFrameList *newKeyFrame;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_2DKEYFRAME, NULL, NULL) )
       {
           newKeyFrame = Rt2dKeyFrameListStreamRead(stream);
       }

       RwStreamClose(stream, NULL);
   }
   \endcode
 *
 * \param stream   Pointer to the stream the keyframe list will be read from
 *
 * \return Returns a pointer to the keyframe list if successful, NULL otherwise.
 *
 * \see Rt2dKeyFrameListStreamWrite
 * \see Rt2dKeyFrameListStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RwStreamFindChunk
 *
 */
Rt2dKeyFrameList *
Rt2dKeyFrameListStreamRead(RwStream *stream)
{
    Rt2dKeyFrameList *keyframeList;

    RWAPIFUNCTION(RWSTRING("Rt2dKeyFrameListStreamRead"));
    RWASSERT( stream );

    /* Allocate basic keyframe block */
    keyframeList = (Rt2dKeyFrameList*)RwFreeListAlloc(&Rt2dAnimGlobals.keyFrameListFreeList,
                                                      rwID_2DKEYFRAME | rwMEMHINTDUR_EVENT);
    RWASSERT(keyframeList);
    keyframeList = _rt2dKeyFrameListStreamReadTo(stream, keyframeList);

    RWRETURN( keyframeList );
}


/****************************************************************************/
/* keyframe update/apply */

/**
 * \ingroup rt2danimsub
 * \ref Rt2dKeyFrameListAdvance caches a keyframe list against the
 *  provided props ready for later application to the 2d object managed by the
 *  props. The intent of this function is to allow fast paging through keyframe
 *  lists, keeping note of which changes need to be applied without actually
 *  applying them. To apply the changes, the \ref Rt2dKeyFrameListApply
 *  function must be called.
 *
 * \param keyframeList    Pointer to the keyframe list to use
 * \param props           Pointer to the props that the keyframe list will be applied to
 *
 * \return Returns the keyframe list that was advanced past.
 *
 * \see Rt2dKeyFrameListApply
 */
Rt2dKeyFrameList *
Rt2dKeyFrameListAdvance(Rt2dKeyFrameList *keyframeList,
                        Rt2dAnimProps *props)
{
    Rt2dAnimObjectUpdate *currentUpdate;
    RwUInt32 updateIndex;

    RWAPIFUNCTION(RWSTRING("Rt2dKeyFrameListAdvance"));
    RWASSERT( keyframeList );
    RWASSERT( props );
    RWASSERT( !keyframeList->isLocked );
    RWASSERT( keyframeList->data.unlocked );

    /* Write the updates that occurred in keyframe into props */
    #define AdvKeyFrameAction                                              \
            updateIndex   = eKeyframeSet->objectIdx;                       \
            currentUpdate = (Rt2dAnimObjectUpdate*)                        \
                    (rwSListGetEntry(props->sceneCurrentState, updateIndex));

    #define AdvTransformAction                                             \
            Rt2dAnimObjectUpdateSetTransform(currentUpdate,                \
                                    ((Rt2dKeyFrameTransform*)actionSet));

    #define AdvColorOffsAction                                             \
            Rt2dAnimObjectUpdateSetColorOffs(currentUpdate,                \
                                    ((Rt2dKeyFrameColor*)actionSet));

    #define AdvColorMultAction                                             \
            Rt2dAnimObjectUpdateSetColorMult(currentUpdate,                \
                                    ((Rt2dKeyFrameColor*)actionSet));

    #define AdvShowAction                                                  \
        Rt2dAnimObjectUpdateSetShow(currentUpdate,                         \
                                    ((Rt2dKeyFrameShow*)actionSet));

    #define AdvMorphAction                                                 \
        Rt2dAnimObjectUpdateSetMorph(currentUpdate,                        \
                                    ((Rt2dKeyFrameMorph*)actionSet));      \
        currentUpdate->morph.source =                                      \
                _rt2dAnimPropsGet2dObjectByIndex(                          \
                            props,(RwInt32)currentUpdate->morph.source);   \
        currentUpdate->morph.destination =                                 \
                _rt2dAnimPropsGet2dObjectByIndex(                          \
                        props,(RwInt32)currentUpdate->morph.destination);


    LockedKeyFrameIteratorMacro(keyframeList,
                                AdvKeyFrameAction,
                                AdvTransformAction,
                                AdvColorOffsAction,
                                AdvColorMultAction,
                                AdvShowAction,
                                AdvMorphAction);


    RWRETURN( keyframeList );
}

/****************************************************************************/
static Rt2dAnimObjectUpdate *
rt2dAnimApplyTransform(Rt2dAnimObjectUpdate *update,
                       Rt2dKeyFrameTransform *action,
                       RwReal alpha __RWUNUSED__)
{
    RwV3d *oPos, *dPos, *sPos;
    RwV3d *oRight, *dRight, *sRight;
    RwV3d *oUp, *dUp, *sUp;

    RWFUNCTION(RWSTRING("rt2dAnimApplyTransform"));
    RWASSERT(update);

    /* interpolation directly modify the object */

    oPos = RwMatrixGetPos(&update->object->MTM);
    dPos = RwMatrixGetPos(&action->matrix);
    sPos = RwMatrixGetPos(&update->transform.matrix );

    RwV3dSub( oPos, dPos, sPos );
    RwV3dScale(oPos, oPos, alpha);
    RwV3dAdd(oPos, oPos, sPos);

    oRight = RwMatrixGetRight(&update->object->MTM);
    dRight = RwMatrixGetRight(&action->matrix);
    sRight = RwMatrixGetRight(&update->transform.matrix );

    RwV3dSub( oRight, dRight, sRight );
    RwV3dScale(oRight, oRight, alpha);
    RwV3dAdd(oRight, oRight, sRight);

    oUp = RwMatrixGetUp(&update->object->MTM);
    dUp = RwMatrixGetUp(&action->matrix);
    sUp = RwMatrixGetUp(&update->transform.matrix );

    RwV3dSub( oUp, dUp, sUp );
    RwV3dScale(oUp, oUp, alpha);
    RwV3dAdd(oUp, oUp, sUp);

    /*AJH:
     * This is a mighty hack : we'de rather use an interpolator
     * see good toolkit RtSlerp
     * Do not know...
     */

    RwMatrixUpdate(&update->object->MTM);
    Rt2dObjectMTMChanged(update->object);

    update->hasTransform = FALSE;

    RWRETURN(update);
}

static Rt2dAnimObjectUpdate *
rt2dAnimApplyColorOffs(Rt2dAnimObjectUpdate *update,
                       Rt2dKeyFrameColor *action,
                       RwReal alpha )
{
    RWFUNCTION(RWSTRING("rt2dAnimApplyColorOffs"));
    RWASSERT(update);


    /* interpolation directly modify the object */

    update->object->colorOffs.red
            = (update->colorOffs.color.red +
                 (action->color.red - update->colorOffs.color.red)*alpha
              )*255.0f;

    update->object->colorOffs.green
            = (update->colorOffs.color.green +
                 (action->color.green - update->colorOffs.color.green)*alpha
              )*255.0f;

    update->object->colorOffs.blue
            = (update->colorOffs.color.blue +
                 (action->color.blue - update->colorOffs.color.blue)*alpha
              )*255.0f;

    update->object->colorOffs.alpha
            = (update->colorOffs.color.alpha +
                 (action->color.alpha - update->colorOffs.color.alpha)*alpha
              )*255.0f;

    update->object->flag |= Rt2dObjectDirtyColor;

    update->hasColorOffs = FALSE;

    RWRETURN(update);
}

static Rt2dAnimObjectUpdate *
rt2dAnimApplyColorMult(Rt2dAnimObjectUpdate *update,
                    Rt2dKeyFrameColor *action,
                    RwReal alpha)
{
    RWFUNCTION(RWSTRING("rt2dAnimApplyColorMult"));
    RWASSERT(update);


    /* interpolation directly modify the object */

    update->object->colorMult.red
            = update->colorMult.color.red + (
            action->color.red - update->colorMult.color.red)*alpha;

    update->object->colorMult.green
            = update->colorMult.color.green + (
            action->color.green - update->colorMult.color.green)*alpha;

    update->object->colorMult.blue
            = update->colorMult.color.blue + (
            action->color.blue - update->colorMult.color.blue)*alpha;

    update->object->colorMult.alpha
            = update->colorMult.color.alpha + (
            action->color.alpha - update->colorMult.color.alpha)*alpha;

    update->object->flag |= Rt2dObjectDirtyColor;

    update->hasColorMult = FALSE;

    RWRETURN(update);
}

static Rt2dAnimObjectUpdate *
rt2dAnimApplyMorph(Rt2dAnimObjectUpdate *update,
                    Rt2dKeyFrameMorph *action __RWUNUSED__, /* temporarily unused */
                    RwReal alpha __RWUNUSED__,   /* temporarily unused */
                    Rt2dAnimProps *props __RWUNUSED__)
{
#if 0
    RwReal dAlpha = update->morph.alpha + (
            action->alpha - update->morph.alpha)*alpha;
#endif
    RWFUNCTION(RWSTRING("rt2dAnimApplyMorph"));
    RWASSERT(update);

#if 0
    Rt2dShapeMorph(update->object,
                    update->morph.source,
                    update->morph.destination,
                    dAlpha);
#endif
    update->hasMorph = FALSE;

    RWRETURN(update);
}


static void
rt2dObjectModify(Rt2dAnimProps *props)
{
    RwInt32 objectCount,i;
    Rt2dAnimObjectUpdate *updates;

    RWFUNCTION(RWSTRING("rt2dObjectModify"));
    RWASSERT( props );

    objectCount = rwSListGetNumEntries(props->sceneCurrentState);
    updates = (Rt2dAnimObjectUpdate *)rwSListGetArray(props->sceneCurrentState);

    for(i=0;i<objectCount;i++,updates++)
    {
        if( TRUE == updates->hasSomething )
        {
            /* Transform */
            if (updates->hasTransform)
            {
                Rt2dObjectSetMTM(updates->object, &updates->transform.matrix);
                updates->hasTransform = FALSE;

            }

            /* Color */
            if (updates->hasColorOffs)
            {
                Rt2dObjectSetColorOffset(updates->object,
                                         &updates->colorOffs.color);

                updates->hasColorOffs = FALSE;
            }

            /* Color */
            if (updates->hasColorMult)
            {
                Rt2dObjectSetColorMultiplier(updates->object,
                                            &updates->colorMult.color);

                updates->hasColorMult = FALSE;
            }

            /* Show/hide */
            if (updates->hasShow)
            {
                if(updates->object->depth != updates->show.depth)
                {
                    Rt2dObjectSetDepth(updates->object, updates->show.depth);
                    Rt2dSceneSetDepthDirty(props->scene);
                }

                Rt2dObjectSetVisible(updates->object, updates->show.show);

#ifdef GJB_DEBUG
                RWMESSAGE((RWSTRING("    ShowObject %d"),
                     updates->object
                       - Rt2dSceneGetChildByIndex(props->scene, 0)));
#endif

#ifdef GJB_DEBUG
                RWMESSAGE((RWSTRING("    ShowObject %d"), updates->object - Rt2dSceneGetChildByIndex(props->scene, 0)));
#endif

                updates->hasShow = FALSE;
            }

            /* Morph */
            if (updates->hasMorph)
            {
                if( NULL != updates->morph.source )  /* First call verification */
                {
                    /*AJH:
                     */
#if 0
                    Rt2dShapeMorph(updates->object,
                                    updates->morph.source,
                                    updates->morph.destination,
                                    updates->morph.alpha);
#endif
                    updates->hasMorph = FALSE;
                }
            }

            updates->hasSomething = FALSE;
        }
    }


    RWRETURNVOID();

}
/**
 * \ingroup rt2danimsub
 * \ref Rt2dKeyFrameListApply applies a set of changes to the provided props.
 * The set of changes is interpolated between the last set of cached changes
 * and the supplied keyframe list. The alpha value ranges between 0.0 and 1.0,
 * and controls the blending between the cached changes and the supplied
 * keyframe.
 *
 * \param keyframeList    Pointer to the keyframe list use
 * \param props           Pointer to the props that the keyframe list will be applied to
 * \param alpha           Interpolation value between 0.0 and 1.0
 *
 * \return Returns the keyframe list that was updated.
 *
 * \see Rt2dKeyFrameListAdvance
 */
Rt2dKeyFrameList *
Rt2dKeyFrameListApply(Rt2dKeyFrameList *keyframeList, Rt2dAnimProps *props,
                      RwReal alpha)
{
    Rt2dAnimObjectUpdate *currentUpdate;
    Rt2dKeyFrameSet      *eKeyframeSet;
    RwUInt8              *actionSet;
    RwUInt32             updateIndex;

    RWAPIFUNCTION(RWSTRING("Rt2dKeyFrameListApply"));
    RWASSERT( keyframeList );
    RWASSERT( props );

    if(alpha > 0.0f && alpha < 1.0f)
    {

        actionSet = keyframeList->data.unlocked;
        eKeyframeSet = (Rt2dKeyFrameSet *)actionSet;

        RWASSERT( actionSet );

        #define KAKeyFrameAction                                              \
            updateIndex   = eKeyframeSet->objectIdx;                          \
            currentUpdate = (Rt2dAnimObjectUpdate*)                           \
                        (rwSListGetEntry(props->sceneCurrentState, updateIndex));

        #define KATransformAction                                             \
            rt2dAnimApplyTransform(currentUpdate,                             \
                                    ((Rt2dKeyFrameTransform*)actionSet),      \
                                    alpha);

        #define KAColorOffsAction                                             \
            rt2dAnimApplyColorOffs(currentUpdate,                             \
                                ((Rt2dKeyFrameColor*)actionSet),              \
                                alpha);

        #define KAColorMultAction                                             \
            rt2dAnimApplyColorMult(currentUpdate,                             \
                                    ((Rt2dKeyFrameColor*)actionSet),          \
                                    alpha);

        /* No interpolation for Show keyframes */
        #define KAShowAction

        #define KAMorphAction                                                 \
            rt2dAnimApplyMorph(currentUpdate,                                 \
                                ((Rt2dKeyFrameMorph*)actionSet),              \
                                alpha,                                        \
                                props);

        LockedKeyFrameIteratorMacro(keyframeList,
                                    KAKeyFrameAction,
                                    KATransformAction,
                                    KAColorOffsAction,
                                    KAColorMultAction,
                                    KAShowAction,
                                    KAMorphAction);
    }

    rt2dObjectModify(props);

    RWRETURN( keyframeList );
}


#if defined (__MWERKS__)
#if (defined(RWVERBOSE))
#pragma message (__FILE__ "/" _SKY_EXPAND(__LINE__) ": __MWERKS__ == " _SKY_EXPAND(__MWERKS__))
#endif /* (defined (__MWERKS__)) */
#if (__option (global_optimizer))
#pragma always_inline on
#endif /* (__option (global_optimizer)) */
#endif /*  defined (__MWERKS__) */


