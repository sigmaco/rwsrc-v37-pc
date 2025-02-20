/*
 *
 * Functionality for 2D rendering
 *
 * Copyright (c) 2001 Criterion Software Ltd.
 */

/****************************************************************************
 *                                                                          *
 *  Module  :   props.c                                                     *
 *                                                                          *
 *  Purpose :   Representation of the props for 2D animations               *
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
#include "rt2d.h"
#include "rt2danim.h"
#include <rpdbgerr.h>

#include "props.h"
#include "keyframe.h"
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
 Functions
 */


static Rt2dObject *
_rt2dAnimObjectAddToDirectory(Rt2dObject *object,
                              Rt2dObject *parent __RWUNUSED__,
                              void *data)
{
    RwSList *directory = (RwSList *)data;
    Rt2dObject **newDirectoryEntry;
    RWFUNCTION(RWSTRING("_rt2dAnimObjectAddToDirectory"));
    RWASSERT(object);
    RWASSERT(directory);

    /* Add self to directory */
    newDirectoryEntry = (Rt2dObject **)rwSListGetNewEntry(directory,
                                   rwID_2DANIM | rwMEMHINTDUR_EVENT);
    RWASSERT(newDirectoryEntry);

    *newDirectoryEntry = object;

    RWRETURN(object);
}

Rt2dAnimProps *
_rt2dAnimPropsInit(Rt2dAnimProps *props, Rt2dObject *scene)
{
    RWFUNCTION(RWSTRING("_rt2dAnimPropsInit"));

    RWASSERT(props);

    /* Doesn't own the scene */
    props->scene = scene;

    /* Create the directory of all nodes in the scene */
    props->directory = rwSListCreate(sizeof(Rt2dObject*),
                        rwID_2DANIM | rwMEMHINTDUR_EVENT);
    if (!props->directory)
    {
        RWRETURN((Rt2dAnimProps*)(NULL));
    }

    /* Make a list of all nodes in the scene */
    Rt2dSceneForAllChildren(
        props->scene,
        _rt2dAnimObjectAddToDirectory,
        props->directory
    );

    /* Can't regenerate string labels */
    props->labels=0;

    /* Create list of states of things in scene */
    props->sceneCurrentState = _rt2dKeyFrameListCreateLockedRep(props);

    RWRETURN(props);
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimPropsCreate creates a props object that represents a set of changes
 * that have been applied to a 2d scene. Rt2dProps does not become the owner of the
 * scene, but the scene must remain valid for the life of the Rt2dProps object.
 *
 * \param scene  Pointer to the scene that the props will update
 *
 * \return Returns the new props object
 *
 * \see Rt2dAnimPropsDestroy
 * \see Rt2dSceneCreate
 */
Rt2dAnimProps *
Rt2dAnimPropsCreate(Rt2dObject *scene)
{
    Rt2dAnimProps *props;

    RWAPIFUNCTION(RWSTRING("Rt2dAnimPropsCreate"));

    props = (Rt2dAnimProps*)(RwFreeListAlloc(&Rt2dAnimGlobals.animPropsFreeList,
                                             rwID_2DANIM | rwMEMHINTDUR_EVENT));
    RWASSERT(props);

    _rt2dAnimPropsInit(props, scene);

    RWRETURN(props);
}

Rt2dAnimProps *
_rt2dAnimPropsDestruct(Rt2dAnimProps *props)
{
    RWFUNCTION(RWSTRING("_rt2dAnimPropsDestruct"));

    RWASSERT(props);

    /* Remove the scene */
    props->scene = (Rt2dObject *)NULL;

    /* Destroy the directory */
    if (props->directory)
    {
        _rwSListDestroy(props->directory);
    }
    props->directory = (RwSList *)NULL;

    /* Destroy list of scene object states */
    if (props->sceneCurrentState)
    {
        _rwSListDestroy(props->sceneCurrentState);
    }
    props->sceneCurrentState = (RwSList *)NULL;

    RWRETURN(props);
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimPropsDestroy destroys a props object.
 *
 * \param props   Pointer to the props object to destroy
 *
 * \return Returns TRUE if the props were successfully destroyed, FALSE otherwise
 *
 * \see Rt2dAnimPropsCreate
 */
RwBool
Rt2dAnimPropsDestroy(Rt2dAnimProps *props)
{
    RwBool              result;

    RWAPIFUNCTION(RWSTRING("Rt2dAnimPropsDestroy"));

    /* NULL path is valid */
    result = (NULL != props);
    if (result)
    {
        _rt2dAnimPropsDestruct(props);

        /* Destroy the basic props structure */
        RwFreeListFree(&Rt2dAnimGlobals.animPropsFreeList, props);
    }

    RWRETURN(result);
}

Rt2dAnimProps *
_rt2dAnimPropsStreamRead(RwStream *stream)
{
    Rt2dObject *scene;
    Rt2dAnimProps *props;
    RWFUNCTION(RWSTRING("_rt2dAnimPropsStreamRead"));

    /* read the scene */
    scene = Rt2dSceneStreamRead(stream);
    RWASSERT(scene);

    /* create the props object */
    props = Rt2dAnimPropsCreate(scene);
    RWASSERT(props);

    RWRETURN(props);
}

RwUInt32
_rt2dAnimPropsStreamGetSize(Rt2dAnimProps *props)
{
    RwUInt32 size;

    RWFUNCTION(RWSTRING("_rt2dAnimPropsStreamGetSize"));

    RWASSERT(props);

    /* Size is that of an object */
    size = Rt2dSceneStreamGetSize(props->scene);

    /* ...plus the size of the labels */
    /* GJB - not done */

    RWRETURN(size);
}

Rt2dAnimProps *
_rt2dAnimPropsStreamWrite(Rt2dAnimProps *props, RwStream *stream)
{
    RWFUNCTION(RWSTRING("_rt2dAnimPropsStreamWrite"));
    RWASSERT(stream);
    RWASSERT(props);

    /* Write scene information */
    if (!Rt2dSceneStreamWrite(props->scene, stream))
    {
        RWRETURN((Rt2dAnimProps*)NULL);
    }

    /* Write labels */
    /* GJB - Not done yet */

    RWRETURN(props);
}

Rt2dObject *
_rt2dAnimPropsGet2dObjectByIndex(Rt2dAnimProps *props, RwInt32 index)
{
    Rt2dObject **ptr;
    RWFUNCTION(RWSTRING("_rt2dAnimPropsGet2dObjectByIndex"));
    RWASSERT(props);
    RWASSERT(index<rwSListGetNumEntries(props->directory));

    ptr = (Rt2dObject**)rwSListGetEntry(props->directory, index);

    RWRETURN((Rt2dObject*)*ptr);
}

RwUInt32
_rt2dAnimPropsGetIndexBy2dObject(Rt2dAnimProps *props, Rt2dObject *object)
{
    Rt2dObject **first, **finish, **current;
    RWFUNCTION(RWSTRING("_rt2dAnimPropsGetIndexBy2dObject"));
    RWASSERT(props);

    /* Linear search. Yuck. */
    first = (Rt2dObject **)rwSListGetArray(props->directory);
    finish = first + rwSListGetNumEntries(props->directory);

    for(current=first; current!=finish; ++current)
    {
        if (object==*current)
        {
            RWRETURN(current-first);
        }
    }

    RWMESSAGE((RWSTRING("Did not find Rt2dObject in list")));

    RWRETURN(0);
}

/****************************************************************************/



#if defined (__MWERKS__)
#if (defined(RWVERBOSE))
#pragma message (__FILE__ "/" _SKY_EXPAND(__LINE__) ": __MWERKS__ == " _SKY_EXPAND(__MWERKS__))
#endif /* (defined (__MWERKS__)) */
#if (__option (global_optimizer))
#pragma always_inline on
#endif /* (__option (global_optimizer)) */
#endif /*  defined (__MWERKS__) */
