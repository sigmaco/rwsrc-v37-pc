/*
 *
 * Functionality for 2D rendering
 *
 * Copyright (c) 2001 Criterion Software Ltd.
 */

/****************************************************************************
 *                                                                          *
 *  Module  :   scene.c                                                     *
 *                                                                          *
 *  Purpose :   Rt2dScene functions                                         *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
 */
#include <string.h>

#include "rwcore.h"
#include "rpdbgerr.h"
#include "rpworld.h"

#include "rt2d.h"
#include "object.h"
#include "shape.h"
#include "font.h"
#include "path.h"
#include "tri.h"
#include "gstate.h"
#include "pickreg.h"
#include "2dstr.h"
#include "scene.h"

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

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   Functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/****************************************************************************/
RwBool
_rt2dSceneResourcePoolFindShapeRep(const _rt2dSceneResourcePool * pool,
                              const _rt2dShapeRep * rep, RwInt32 * npIndex)
{
    RwUInt32             i;

    RWFUNCTION(RWSTRING("_rt2dSceneResourcePoolFindShapeRep"));
    RWASSERT(pool);
    RWASSERT(rep);

    for (i = 0; i < pool->numShapeReps; ++i)
    {
        if (pool->shapeReps[i] == rep)
        {
            if (npIndex)
                (*npIndex) = i;
            RWRETURN(TRUE);
        }
    }

    RWRETURN(FALSE);
}

static Rt2dObject *
SceneCountShapeLeaves(Rt2dObject *object, Rt2dObject *parent __RWUNUSED__, void *data)
{
    RWFUNCTION(RWSTRING("SceneCountShapeLeaves"));

    /* Recurse into sub-scenes */
    if (Rt2dObjectIsScene(object))
    {
        Rt2dSceneForAllChildren(object, SceneCountShapeLeaves, data);
    }
    /* Count each child shape */
    else if (Rt2dObjectIsShape(object))
    {
        (*((RwUInt32 *) data))++;
    }
    RWRETURN(object);
}

static Rt2dObject *
SceneAddChildShapeResourcesToPool(Rt2dObject *object, Rt2dObject *parent __RWUNUSED__, void *data)
{
    RWFUNCTION(RWSTRING("SceneAddChildShapeResourcesToPool"));

    /* Recurse into sub-scenes */
    if (Rt2dObjectIsScene(object))
    {
        Rt2dSceneForAllChildren(object, SceneAddChildShapeResourcesToPool, data);
    }
    /* Add resource to pool if unique */
    else if (Rt2dObjectIsShape(object))
    {
        _rt2dSceneResourcePool *pool = (_rt2dSceneResourcePool *) data;
        _rt2dShapeRep      *rep = object->data.shape.rep;

        /* is it already in the list? (should really mark atomic) */
        if (!_rt2dSceneResourcePoolFindShapeRep(pool, rep, (RwInt32 *)NULL))
        {
            _rt2dShapeRepAddRef(rep);
            pool->shapeReps[pool->numShapeReps++] = rep;
        }
    }
    RWRETURN(object);
}

static _rt2dSceneResourcePool *
_rt2dSceneResourcePoolInit(Rt2dObject *scene, _rt2dSceneResourcePool *pool)
{
    RwUInt32          numShapes;
    RWFUNCTION(RWSTRING("_rt2dSceneResourcePoolInit"));

    RWASSERT(pool);
    RWASSERT(scene);
    RWASSERT(((scene->type) == rt2DOBJECTTYPESCENE));

    /* Get uber-shape count */
    numShapes = 0;
    Rt2dSceneForAllChildren(scene, SceneCountShapeLeaves, &numShapes);

    /* Prepare space to hold list of resources */
    if (numShapes > 0)
    {
        pool->shapeReps =
            (_rt2dShapeRep **) RwMalloc(sizeof(_rt2dShapeRep *) * numShapes,
                                        rwID_2DPLUGIN | rwMEMHINTDUR_EVENT);
        if (!pool->shapeReps)
        {
            RWERROR((E_RW_NOMEM, (sizeof(_rt2dShapeRep *) * numShapes)));
            RWRETURN((_rt2dSceneResourcePool *)NULL);
        }
    }
    else
    {
        pool->shapeReps = (_rt2dShapeRep **)NULL;
    }
    pool->numShapeReps = 0;

    /* fill it with _unique_ shapes */
    Rt2dSceneForAllChildren(scene, SceneAddChildShapeResourcesToPool, pool);

    /* Done */
    RWRETURN(pool);
}

static RwBool
_rt2dSceneResourcePoolDestruct(_rt2dSceneResourcePool *pool)
{
    RWFUNCTION(RWSTRING("_rt2dSceneResourcePoolDestruct"));
    RWASSERT(pool);

    if (pool->numShapeReps)
    {
        RwUInt32             i;

        /* remove the reference to each shape rep */
        for (i = 0; i < pool->numShapeReps; ++i)
        {
            _rt2dShapeRepDestroy(pool->shapeReps[i]);
        }

        RwFree(pool->shapeReps);
    }

    /* Done */
    RWRETURN(TRUE);
}

static RwUInt32
_rt2dSceneResourcePoolStreamGetSize(_rt2dSceneResourcePool *pool)
{
    RwUInt32 size;
    RwUInt32 i;
    RWFUNCTION(RWSTRING("_rt2dSceneResourcePoolStreamGetSize"));

    size = sizeof(RwUInt32);

    for(i=0; i<pool->numShapeReps;++i)
    {
        size+=_rt2dShapeRepStreamGetSize(pool->shapeReps[i]);
    }

    RWRETURN(size);
}

static _rt2dSceneResourcePool *
_rt2dSceneResourcePoolStreamReadTo(_rt2dSceneResourcePool *pool, RwStream *stream)
{
    RwUInt32    i;
    RwUInt32    repCount;

    RWFUNCTION(RWSTRING("_rt2dSceneResourcePoolStreamReadTo"));

    /* Get number of shapereps */
    if( NULL == RwStreamReadInt32(stream, (RwInt32 *)&repCount, sizeof(repCount) ) )
    {
        RWRETURN((_rt2dSceneResourcePool *)NULL);
    }

#if 0
    /* No conversion needed as the ReadInt32's doing it allready */
    /* Convert to machine format */
    (void)RwMemNative32(&repCount, sizeof(repCount));
#endif

    pool->numShapeReps = repCount;

    if (repCount)
    {
        pool->shapeReps = (_rt2dShapeRep **) RwMalloc(sizeof(_rt2dShapeRep *) * repCount,
                                                     rwID_2DPLUGIN | rwMEMHINTDUR_EVENT);
        if (!pool->shapeReps)
        {
            RWERROR((E_RW_NOMEM, (sizeof(_rt2dShapeRep *) * repCount)));
            RWRETURN((_rt2dSceneResourcePool *)NULL);
        }

        /* Read in pooled resources */
        for(i=0; i<repCount; ++i)
        {
            if (!(pool->shapeReps[i] = _rt2dShapeRepStreamRead(stream)))
            {
                RWRETURN((_rt2dSceneResourcePool *)NULL);
            }
        }
    }
    else
    {
        pool->shapeReps = (_rt2dShapeRep**)NULL;
    }

    RWRETURN(pool);
}

static _rt2dSceneResourcePool *
_rt2dSceneResourcePoolStreamWrite(_rt2dSceneResourcePool *pool, RwStream *stream)
{
    RwUInt32 i, repCount = pool->numShapeReps;
    RwInt32 sRepCount;

    RWFUNCTION(RWSTRING("_rt2dSceneResourcePoolStreamWrite"));

    sRepCount = pool->numShapeReps;
#if 0
    /* No conversion needed as the WriteInt32's doing it allready */
    (void)RwMemLittleEndian32(&sRepCount, sizeof(sRepCount));
#endif

    /* Write number of reps */
    if (!RwStreamWriteInt32(stream, &sRepCount, sizeof(repCount)))
    {
        RWRETURN((_rt2dSceneResourcePool *)NULL);
    }

    /* Write out pooled resources */
    for(i=0; i<repCount; ++i)
    {
        if (!_rt2dShapeRepStreamWrite(pool->shapeReps[i], stream))
        {
            RWRETURN((_rt2dSceneResourcePool *)NULL);
        }
    }

    RWRETURN(pool);
}

/**
 * \ingroup rt2dscene
 * \page rt2dscene Rt2dScene Overview
 *
 * A scene is a container of 2d objects that can be manipulated and rendered.
 * A scene is a also a 2d object. A scene can contain shapes, object strings,
 * pick regions and other scenes.
 *
 * \par Creating a Scene
 * \li \ref Rt2dSceneCreate
 *
 * \par Adding Objects to a Scene
 * -# \ref Rt2dSceneLock locks the scene.
 * -# \ref Rt2dSceneAddChild adds an object to the scene.
 * -# \ref Rt2dSceneUnlock unlocks the scene.
 */

/**
 * \ingroup rt2dscene
 * \ref Rt2dSceneCreate allocates storage for the contents of a scene of
 * 2d objects.
 *
 * \return Returns a new scene if successful, NULL otherwise. The new
 * scene is locked, ready for modification.
 *
 * \see Rt2dSceneDestroy
 * \see Rt2dSceneAddChild
 * \see Rt2dSceneGetNewChildObjectString
 * \see Rt2dSceneGetNewChildPickRegion
 * \see Rt2dSceneGetNewChildScene
 * \see Rt2dSceneGetNewChildShape
 */
Rt2dObject *
Rt2dSceneCreate(void)
{
    Rt2dObject           *object;

    RWAPIFUNCTION(RWSTRING("Rt2dSceneCreate"));

    object = _rt2dObjectCreate();
    RWASSERT(object);
    _rt2dObjectInit(object, rt2DOBJECTTYPESCENE);

    _rt2dSceneInit(object, 0);

    RWRETURN(object);
}

Rt2dObject *
_rt2dSceneInit(Rt2dObject *object, RwInt32 objectCount)
{
    RWFUNCTION(RWSTRING("_rt2dSceneInit"));
    RWASSERT(object);

    object->data.scene.objectCount = objectCount;
    object->data.scene.objects = rwSListCreate(sizeof(Rt2dObject*),
                                rwID_2DPLUGIN | rwMEMHINTDUR_EVENT);
    if (objectCount)
    {
        rwSListGetNewEntries(object->data.scene.objects, objectCount,
            rwID_2DPLUGIN | rwMEMHINTDUR_EVENT);
        memset(rwSListGetArray(object->data.scene.objects),0,sizeof(Rt2dObject*)*objectCount);
    }
    object->data.scene.depths = (RwSList *)NULL;

    object->data.scene.isDirtyDepths = FALSE;

    object->flag |= Rt2dObjectIsLocked;

    RWRETURN(object);
}

RwBool
_rt2dSceneDestruct(Rt2dObject *object)
{
    RwBool              result = TRUE;
    Rt2dObject          *current,*end;

    RWFUNCTION(RWSTRING("_rt2dSceneDestruct"));

    RWASSERT(object);
    RWASSERT(((object->type) == rt2DOBJECTTYPESCENE));
    RWASSERT(object);

    /* Destroy depths */
    if (object->data.scene.depths)
    {
        result = rwSListDestroy(object->data.scene.depths);
    }

    /* Destruct each child in turn */
    end = (Rt2dObject*)(rwSListGetArray(object->data.scene.objects))
          +object->data.scene.objectCount;
    for(current = (Rt2dObject*)(rwSListGetArray(object->data.scene.objects));
        current !=end;
        ++current )
    {
        result = (result && _rt2dObjectDestruct(current));
    }

    /* Destroy object buffer */
    result = (result && rwSListDestroy(object->data.scene.objects));

    RWRETURN(result);
}

/**
 * \ingroup rt2dscene
 * \ref Rt2dSceneDestroy deallocates storage for a scene of 2d objects.
 * All child objects added to a scene are destroyed.
 *
 * \note Any child object in a scene needs to be unlocked before it can
 * be destroyed. If the child object is not unlocked error messages
 * will be displayed and the object \e may not be destroyed.
 *
 * \param object Pointer to the object to be destroyed.
 *
 * \return Returns TRUE if successful, FALSE otherwise.
 *
 * \see Rt2dSceneCreate
 * \see Rt2dSceneAddChild
 */
RwBool
Rt2dSceneDestroy(Rt2dObject *object)
{
    RwBool              result;

    RWAPIFUNCTION(RWSTRING("Rt2dSceneDestroy"));

    /* NULL path is valid */
    result = (NULL != object);
    if (result)
    {
        result = _rt2dSceneDestruct(object);

        /* Destroy base object */
        RwFreeListFree(Rt2dGlobals.objectFreeList, object);
    }

    RWRETURN(result);
}

Rt2dObject *
_rt2dSceneCopy(Rt2dObject *dst, Rt2dObject *src)
{
    RwUInt32 i;
    RwUInt32 childCount;
    RWFUNCTION(RWSTRING("_rt2dSceneCopy"));
    RWASSERT(dst);
    RWASSERT(src);
    RWASSERT(rt2DOBJECTTYPESCENE == src->type);
    RWASSERT(0==(src->flag & Rt2dObjectIsLocked));

    memcpy(dst, src, sizeof(Rt2dObject));

    childCount = Rt2dSceneGetChildCount(src);

    dst->data.scene.objects       = rwSListCreate(sizeof(Rt2dObject),
                                      rwID_2DPLUGIN | rwMEMHINTDUR_EVENT);            /* collection of objects in scene */
    dst->data.scene.depths        = rwSListCreate(sizeof(_rt2dDepthOfObject),
                                      rwID_2DPLUGIN | rwMEMHINTDUR_EVENT);    /* depths for depthsort */

    if (childCount)
    {
        rwSListGetNewEntries(dst->data.scene.objects, childCount,
            rwID_2DPLUGIN | rwMEMHINTDUR_EVENT);

        memcpy(rwSListGetNewEntries(dst->data.scene.depths, childCount,
            rwID_2DPLUGIN | rwMEMHINTDUR_EVENT),
               rwSListGetArray(src->data.scene.depths),
               sizeof(_rt2dDepthOfObject) * childCount);

        for(i=0; i<childCount; ++i)
        {
            if (!_rt2dObjectCopy((Rt2dObject *)rwSListGetEntry(dst->data.scene.objects, i),
                                 (Rt2dObject *)rwSListGetEntry(src->data.scene.objects, i)))
            {
                RWRETURN(NULL);
            }
        }
    }

    RWRETURN(dst);
}

/**
 * \ingroup rt2dscene
 * \ref Rt2dSceneLock
 * locks a scene for editing. A locked scene can not be streamed or rendered
 * and is provided only for editing purposes. When a scene is locked
 * child objects can be added to the scene.
 *
 * \param object Pointer to the scene.
 *
 * \return Returns the new scene if successful, NULL otherwise.
 *
 * \see Rt2dSceneDestroy
 * \see Rt2dSceneUnlock
 * \see Rt2dSceneAddChild
 * \see Rt2dSceneGetNewChildPickRegion
 * \see Rt2dSceneGetNewChildObjectString
 * \see Rt2dSceneGetNewChildScene
 * \see Rt2dSceneGetNewChildShape
 */
Rt2dObject *
Rt2dSceneLock(Rt2dObject *object)
{
    Rt2dObject *first, *finish, *current;
    Rt2dObject          **objectPtr;

    RWAPIFUNCTION(RWSTRING("Rt2dSceneLock"));
    RWASSERT(object);
    RWASSERT(rt2DOBJECTTYPESCENE==object->type);

    if((object->flag & Rt2dObjectIsLocked) != Rt2dObjectIsLocked)
    {
        RwSList *objectList = rwSListCreate(sizeof(Rt2dObject*),
                                rwID_2DOBJECT | rwMEMHINTDUR_EVENT);
        RWASSERT(objectList);

        first = (Rt2dObject*)(rwSListGetArray(object->data.scene.objects));
        finish = first + object->data.scene.objectCount;
        for( current=first; current!=finish; ++current)
        {
            objectPtr = (Rt2dObject **)rwSListGetNewEntry(objectList,
                                 rwID_2DOBJECT | rwMEMHINTDUR_EVENT);

            *objectPtr = _rt2dObjectCreate();

            _rt2dObjectRealloc(*objectPtr, current);
        }

        rwSListDestroy(object->data.scene.objects);
        rwSListDestroy(object->data.scene.depths);

        object->data.scene.depths = (RwSList *) NULL;
        object->data.scene.objects = objectList;

        object->flag |= Rt2dObjectIsLocked;

    }
    RWRETURN(object);
}

static Rt2dObject *
rt2dSceneDepthSort(Rt2dObject *object)
{
    _rt2dDepthOfObject *first, *finish, *i, *j, temp;

    RWFUNCTION(RWSTRING("rt2dSceneDepthSort"));
    RWASSERT(object);

    first = (_rt2dDepthOfObject*)
                     (rwSListGetArray(object->data.scene.depths));
    finish = first + object->data.scene.objectCount;

    /* Early out for empty scenes */
    if(first==finish)
    {
        RWRETURN(object);
    }

    /* Update z-orders */
    i = first;
    while (i!=finish)
    {
        i->depth = i->object->depth;
        i++;
    }

    /* Sort */
    for( i = first; i < finish-1; ++i)
    {
        for( j = i + 1; j < finish; ++j)
        {
            if ( i->depth > j->depth )
            {
                /* Swap em */
                temp = *i;
                *i = *j;
                *j = temp;
            }
        }
    }

    RWRETURN(object);
}

/**
 * \ingroup rt2dscene
 * \ref Rt2dSceneUnlock unlocks a scene. An unlocked scene may be
 * streamed and rendered. Objects can also be manipulated in an
 * unlocked scene.
 *
 * \param object Pointer to the scene to be unlocked.
 *
 * \return Returns the unlocked scene if successful, NULL otherwise.
 *
 * \see Rt2dSceneLock
 * \see Rt2dObjectMTMRotate
 * \see Rt2dObjectMTMScale
 * \see Rt2dObjectMTMTranslate
 * \see Rt2dObjectSetColorMultiplier
 * \see Rt2dObjectSetColorOffset
 * \see Rt2dObjectSetDepth
 * \see Rt2dObjectSetVisible
 */
Rt2dObject *
Rt2dSceneUnlock(Rt2dObject *object)
{
    Rt2dObject **first, **finish, **current;
    RwSList              *objectList;
    Rt2dObject          *objectPtr;
    Rt2dObject           *firstChildObject;
    Rt2dObject           *childObject;

    RWAPIFUNCTION(RWSTRING("Rt2dSceneUnlock"));
    RWASSERT(object);
    RWASSERT(rt2DOBJECTTYPESCENE==object->type);

    if((object->flag & Rt2dObjectIsLocked) == Rt2dObjectIsLocked)
    {
        objectList = rwSListCreate(sizeof(Rt2dObject),
            rwID_2DOBJECT | rwMEMHINTDUR_EVENT);

        /* Callback on child */
        first = (Rt2dObject**)(rwSListGetArray(object->data.scene.objects));
        finish = first + object->data.scene.objectCount;
        for( current=first; current!=finish; ++current)
        {
            objectPtr = (Rt2dObject *)rwSListGetNewEntry(objectList,
                                 rwID_2DOBJECT | rwMEMHINTDUR_EVENT);
            if( NULL != *current )
            {
                _rt2dObjectRealloc(objectPtr, *current);

                _rt2dObjectDestroyAfterRealloc(*current);
            }
        }

        rwSListDestroy(object->data.scene.objects);

        object->data.scene.objects = objectList;

        object->flag &= ~Rt2dObjectIsLocked;

        /* CreateDepth depth on locked scene */
        /* Initialise depth object pointers, but not values */
        object->data.scene.depths = rwSListCreate(sizeof(_rt2dDepthOfObject),
                                          rwID_2DPLUGIN | rwMEMHINTDUR_EVENT);

        if (object->data.scene.objectCount)
        {
            _rt2dDepthOfObject   *currentDepth, *firstDepthOfObject, *lastDepthOfObject;

            rwSListGetNewEntries(object->data.scene.depths,
                                 object->data.scene.objectCount,
                                 rwID_2DPLUGIN | rwMEMHINTDUR_EVENT);

            firstDepthOfObject = (_rt2dDepthOfObject *)
                rwSListGetArray(object->data.scene.depths);

            firstChildObject =  (Rt2dObject *)
                rwSListGetArray(object->data.scene.objects);

            lastDepthOfObject = firstDepthOfObject + object->data.scene.objectCount;

            for(currentDepth = firstDepthOfObject, childObject = firstChildObject;
                currentDepth != lastDepthOfObject;
                ++currentDepth, ++childObject)
            {
                currentDepth->object = childObject;
            }

            /* Sort z-orders */
            rt2dSceneDepthSort(object);

            object->data.scene.isDirtyDepths = FALSE;
        }

    }
    RWRETURN(object);
}

/**
 * \ingroup rt2dscene
 * \ref Rt2dSceneAddChild adds a child object to a scene. The scene must
 * be locked.
 *
 * \param scene is the scene to add the child to
 * \param object is the child to be added
 *
 * \return returns the scene if successful, NULL otherwise
 *
 * \see Rt2dSceneLock
 * \see Rt2dSceneUnlock
 * \see Rt2dObjectStringCreate
 * \see Rt2dPickRegionCreate
 * \see Rt2dSceneCreate
 * \see Rt2dShapeCreate
 */
Rt2dObject *
Rt2dSceneAddChild(Rt2dObject *scene, Rt2dObject *object)
{
    Rt2dObject **objectPtr;
    RWAPIFUNCTION(RWSTRING("Rt2dSceneAddChild"));
    RWASSERT(scene);
    RWASSERT(rt2DOBJECTTYPESCENE == scene->type);
    RWASSERT((scene->flag & Rt2dObjectIsLocked) == Rt2dObjectIsLocked);
    RWASSERT(object);

    objectPtr = (Rt2dObject **)rwSListGetNewEntry(scene->data.scene.objects,
                                       rwID_2DOBJECT | rwMEMHINTDUR_EVENT);

    if(objectPtr)
    {
        *objectPtr = object;

        scene->data.scene.objectCount++;

        RWRETURN(scene);
    }

    RWRETURN((Rt2dObject *)NULL);
}

Rt2dObject *
_rt2dSceneGetNewChild(Rt2dObject *object, RwInt32 childType)
{
    Rt2dObject *child;
    RWFUNCTION(RWSTRING("_rt2dSceneGetNewChild"));
    RWASSERT(object);
    RWASSERT(rt2DOBJECTTYPESCENE==object->type);

    child = _rt2dObjectCreate();
    _rt2dObjectInit(child, childType);

    Rt2dSceneAddChild(object,child);

    RWRETURN(child);
}

/**
 * \ingroup rt2dscene
 * \ref Rt2dSceneGetNewChildScene creates a new child scene. The
 * scene needs to be locked before a new child scene can be
 * added.
 *
 * \param object is the scene to be used
 *
 * \return Returns the new child scene.
 *
 * \see Rt2dSceneLock
 * \see Rt2dSceneUnlock
 * \see Rt2dSceneGetNewChildShape
 * \see Rt2dSceneGetNewChildPickRegion
 * \see Rt2dSceneGetNewChildObjectString
 */
Rt2dObject *
Rt2dSceneGetNewChildScene(Rt2dObject *object)
{
    Rt2dObject *child;
    RWAPIFUNCTION(RWSTRING("Rt2dSceneGetNewChildScene"));
    RWASSERT(object);
    RWASSERT(rt2DOBJECTTYPESCENE==object->type);

    child = _rt2dSceneGetNewChild(object, rt2DOBJECTTYPESCENE);

    _rt2dSceneInit(child, 0);

    RWRETURN(child);
}

/**
 * \ingroup rt2dscene
 * \ref Rt2dSceneGetNewChildShape creates a new child shape. The
 * scene needs to be locked before a new child shape can be
 * added.
 *
 * \param object is the scene to be used
 *
 * \return Returns the new child shape.
 *
 * \see Rt2dSceneLock
 * \see Rt2dSceneUnlock
 * \see Rt2dSceneGetNewChildPickRegion
 * \see Rt2dSceneGetNewChildObjectString
 * \see Rt2dSceneGetNewChildScene
 */
Rt2dObject *
Rt2dSceneGetNewChildShape(Rt2dObject *object)
{
    Rt2dObject *child;
    RWAPIFUNCTION(RWSTRING("Rt2dSceneGetNewChildShape"));
    RWASSERT(object);
    RWASSERT(rt2DOBJECTTYPESCENE==object->type);

    child = _rt2dSceneGetNewChild(object, rt2DOBJECTTYPESHAPE);

    _rt2dShapeInit(child, 0, 0);

    RWRETURN(child);
}

/**
 * \ingroup rt2dscene
 * \ref Rt2dSceneGetNewChildPickRegion creates a new child pick region.
 * The scene needs to be locked before a new child pick region can be
 * added.
 *
 * \param object is the scene to be used
 *
 * \return Returns the new child pick region.
 *
 * \see Rt2dSceneLock
 * \see Rt2dSceneUnlock
 * \see Rt2dSceneGetNewChildObjectString
 * \see Rt2dSceneGetNewChildScene
 * \see Rt2dSceneGetNewChildShape
 */
Rt2dObject *
Rt2dSceneGetNewChildPickRegion(Rt2dObject *object)
{
    Rt2dObject *child;
    RWAPIFUNCTION(RWSTRING("Rt2dSceneGetNewChildPickRegion"));
    RWASSERT(object);
    RWASSERT(rt2DOBJECTTYPESCENE==object->type);

    child = _rt2dSceneGetNewChild(object, rt2DOBJECTTYPEPICKREGION);

    _rt2dPickRegionInit(child);

    RWRETURN(child);
}

/**
 * \ingroup rt2dscene
 * \ref Rt2dSceneGetNewChildObjectString creates a new child object
 * string. The scene needs to be locked before a new child object string
 * can be added.
 *
 * \param object is the scene to be used
 * \param text is the string to be displayed
 * \param font is the font filename to be used
 *
 * \return Returns the new child object string.
 *
 * \see Rt2dSceneLock
 * \see Rt2dSceneUnlock
 * \see Rt2dSceneGetNewChildPickRegion
 * \see Rt2dSceneGetNewChildScene
 * \see Rt2dSceneGetNewChildShape
 */
Rt2dObject *
Rt2dSceneGetNewChildObjectString(Rt2dObject *object, const RwChar *text, const RwChar *font)
{
    Rt2dObject *child;
    RWAPIFUNCTION(RWSTRING("Rt2dSceneGetNewChildObjectString"));
    RWASSERT(object);
    RWASSERT(rt2DOBJECTTYPESCENE==object->type);
    RWASSERT(text);

    child = _rt2dSceneGetNewChild(object, rt2DOBJECTTYPEOBJECTSTRING);

    _rt2dObjectStringInit(child, text, font);

    RWRETURN(child);
}

/**
 * \ingroup rt2dscene
 * \ref Rt2dSceneGetChildCount is used to determine the number of
 * child objects in a scene.
 *
 * \param scene is the scene to be used
 *
 * \return Returns the number of child objects.
 *
 * \see Rt2dSceneGetChildByIndex
 * \see Rt2dSceneForAllChildren
 */
extern RwInt32
Rt2dSceneGetChildCount(Rt2dObject* scene)
{
    RWAPIFUNCTION(RWSTRING("Rt2dSceneGetChildCount"));
    RWASSERT(scene);
    RWASSERT(rt2DOBJECTTYPESCENE == scene->type);

    RWRETURN(scene->data.scene.objectCount);
}

/**
 * \ingroup rt2dscene
 * \ref Rt2dSceneGetChildByIndex
 * Gets a child of a scene by its index number.
 *
 * \param scene is the scene to be used
 * \param index is the index of the child to be obtained
 *
 * \return Returns a pointer to the specified child.
 *
 * \see Rt2dSceneGetChildCount
 * \see Rt2dSceneForAllChildren
 */
Rt2dObject *
Rt2dSceneGetChildByIndex(Rt2dObject *scene, RwInt32 index)
{
    RWAPIFUNCTION(RWSTRING("Rt2dSceneGetChildByIndex"));
    RWASSERT(scene);
    RWASSERT(rt2DOBJECTTYPESCENE == scene->type);
    RWASSERT(index >= 0);
    RWASSERT(index < rwSListGetNumEntries(scene->data.scene.objects));

    if( (scene->flag & Rt2dObjectIsLocked) == Rt2dObjectIsLocked)
    {
        RWRETURN(*(Rt2dObject**)rwSListGetEntry(scene->data.scene.objects, index));
    }

    RWRETURN((Rt2dObject*)rwSListGetEntry(scene->data.scene.objects, index));
}


/**
 * \ingroup rt2dscene
 * \ref Rt2dSceneRender renders all visible 2d objects in a scene.
 *
 * \param object is the scene to be used
 *
 * \return Returns scene if successful, NULL otherwise.
 *
 * \see Rt2dSceneCreate
 * \see Rt2dSceneUpdateLTM
 */
Rt2dObject *
Rt2dSceneRender(Rt2dObject *object)
{
    RWAPIFUNCTION(RWSTRING("Rt2dSceneRender"));

    RWRETURN(_rt2dSceneRenderChild(object, &_rt2dColorMultUnity, &_rt2dColorOffsZero));
}


Rt2dObject *
_rt2dSceneRenderChild(Rt2dObject *object, const RwRGBAReal *colorMult, const RwRGBAReal *colorOffs)
{
    RwBool DirtyScene = FALSE;
    RWFUNCTION(RWSTRING("_rt2dSceneRenderChild"));
    RWASSERT(object);
    RWASSERT(object->type == rt2DOBJECTTYPESCENE);
    RWASSERT((object->flag & Rt2dObjectIsLocked) == 0);

    /* Object level stuff */
    if((object->flag & Rt2dObjectVisible) == Rt2dObjectVisible)
    {
        _rt2dDepthOfObject *first, *finish, *i;

        if((object->flag & Rt2dObjectDirtyLTM) == Rt2dObjectDirtyLTM)
        {
            _rt2dCTMPush(&object->MTM);
            RwMatrixCopy(&object->LTM, _rt2dCTMGetDirect());

            object->flag &= ~Rt2dObjectDirtyLTM;
            DirtyScene = TRUE;
        }
        else
        {
            _rt2dCTMSet(&object->LTM);
        }

        /* Continue with rendering scene */
        {
            RwRGBAReal multColor, offsColor;
            RwRGBARealTransformAssignMult(&multColor, &object->colorMult, colorMult);
            RwRGBARealTransformAssignOffset(&offsColor, &object->colorOffs, colorMult, colorOffs);

            first = (_rt2dDepthOfObject*)
                         (rwSListGetArray(object->data.scene.depths));
            finish = first + object->data.scene.objectCount;

            /* Check if z-orders have changed */
            if (object->data.scene.isDirtyDepths )
            {
                /* Sort z-orders */
                rt2dSceneDepthSort(object);

                object->data.scene.isDirtyDepths = FALSE;
            }

            /* Render */
            for( i=first; i!=finish ; ++i)
            {
                if( DirtyScene )
                {
                    i->object->flag |= Rt2dObjectDirtyLTM;
                }
                _rt2dObjectRender( i->object, &multColor, &offsColor );
            }

            /* Restore CTM */
            Rt2dCTMPop();
        }
    }

    RWRETURN(object);
}

/**
 * \ingroup rt2dscene
 * \ref Rt2dSceneForAllChildren
 * Performs a callback on all children of a scene
 *
 * \param scene is the scene to be used
 * \param callback is the function that will be called for each child object
 * \param pData is user data that is passed to the callback function each time
 *
 * \return returns the scene used
 *
 * \see Rt2dSceneGetChildCount
 * \see Rt2dSceneGetChildByIndex
 */
Rt2dObject *
Rt2dSceneForAllChildren(Rt2dObject *scene, Rt2dObjectCallBack callback, void *pData )
{
    Rt2dObject *first, *finish, *current;

    RWAPIFUNCTION(RWSTRING("Rt2dSceneForAllChildren"));
    RWASSERT(scene);
    RWASSERT(rt2DOBJECTTYPESCENE == scene->type);
    RWASSERT(callback);

    /* Callback on child */
    first = (Rt2dObject*)(rwSListGetArray(scene->data.scene.objects));
    finish = first + scene->data.scene.objectCount;
    for( current=first; current!=finish; ++current)
    {
        (callback)(current, scene, pData);
    }

    RWRETURN(scene);
}

typedef struct StreamAndPool StreamAndPool;
struct StreamAndPool
{
    RwStream               *stream;
    _rt2dSceneResourcePool *pool;
};

static Rt2dObject *
_rt2dSceneWriteChildToStreamCallback(Rt2dObject *object, Rt2dObject *parent __RWUNUSED__, void *data)
{
    StreamAndPool *sp = (StreamAndPool *)(data);

    RWFUNCTION(RWSTRING("_rt2dSceneWriteChildToStreamCallback"));
    RWASSERT(object);
    RWRETURN(_rt2dObjectStreamWrite(object, sp->stream, sp->pool ));
}

Rt2dObject *
_rt2dSceneStreamWrite(Rt2dObject *scene, RwStream *stream, _rt2dSceneResourcePool *pool)
{
    RwInt32 objectCount;
    StreamAndPool streamAndPool;

    RWFUNCTION(RWSTRING("_rt2dSceneStreamWrite"));

    RWASSERT(scene);
    RWASSERT(rt2DOBJECTTYPESCENE == scene->type);
    RWASSERT((scene->flag & Rt2dObjectIsLocked) == 0);
    RWASSERT(stream);

    /* Write base class information */
    if (!_rt2dObjectBaseStreamWrite(scene, stream))
    {
        RWRETURN((Rt2dObject*)NULL);
    }

    /* Convert number of objects in stream */
    objectCount = scene->data.scene.objectCount;
    (void)RwMemLittleEndian32(&objectCount, sizeof(objectCount));

    /* Write number of objects in scene */
    if (!RwStreamWrite(stream, &objectCount, sizeof(objectCount)))
    {
        RWRETURN((Rt2dObject *)NULL);
    }

    streamAndPool.stream = stream;

    if (pool)
    {
        streamAndPool.pool = pool;

        /* Write each object in scene */
        Rt2dSceneForAllChildren(scene, _rt2dSceneWriteChildToStreamCallback, &streamAndPool);
    }
    else
    /* If no pool; must create it and use it */
    {
        _rt2dSceneResourcePool resourcePool;

        /* Create list of shared representations */
        if (!_rt2dSceneResourcePoolInit(scene, &resourcePool))
        {
            RWRETURN((Rt2dObject*)NULL);
        }

        /* Stream it out */
        if (!_rt2dSceneResourcePoolStreamWrite(&resourcePool, stream))
        {
            RWRETURN((Rt2dObject*)NULL);
        }

        streamAndPool.pool = &resourcePool;

        /* Write each object in scene */
        Rt2dSceneForAllChildren(scene, _rt2dSceneWriteChildToStreamCallback, &streamAndPool);

        /* Finished with the pool */
        if (!_rt2dSceneResourcePoolDestruct(&resourcePool))
        {
            RWRETURN((Rt2dObject*)NULL);
        }
    }

    RWRETURN(scene);
}

/**
* \ingroup rt2dscene
* \ref Rt2dSceneStreamWrite
* Writes a scene to a stream
*
* \param scene is the scene to be used
* \param stream is the stream to be written to
*
* \return returns the object that was used if successful, NULL otherwise
*
* \see Rt2dSceneStreamGetSize
* \see Rt2dSceneStreamRead
*/
Rt2dObject *
Rt2dSceneStreamWrite(Rt2dObject *scene, RwStream *stream)
{
    RwUInt32 size;
    RWAPIFUNCTION(RWSTRING("Rt2dSceneStreamWrite"));
    RWASSERT(scene);
    RWASSERT(rt2DOBJECTTYPESCENE == scene->type);
    RWASSERT((scene->flag & Rt2dObjectIsLocked) == 0);
    RWASSERT(stream);

    /* Write header */
    size = Rt2dSceneStreamGetSize(scene);
    if (!RwStreamWriteChunkHeader(stream, rwID_2DSCENE, size))
    {
        RWRETURN((Rt2dObject *)NULL);
    }

    /* Write out base object information */


    /* Write it out, with no pool yet */
    _rt2dSceneStreamWrite(scene, stream, NULL);

    RWRETURN(scene);
}

typedef struct SizeAndPool SizeAndPool;
struct SizeAndPool
{
    RwUInt32 size;
    _rt2dSceneResourcePool *pool;
};

static Rt2dObject *
_rt2dSceneAccumulateSizeOfChildCallback(Rt2dObject *object, Rt2dObject *parent __RWUNUSED__, void *data)
{
    SizeAndPool *sp = (SizeAndPool *)data;
    RWFUNCTION(RWSTRING("_rt2dSceneAccumulateSizeOfChildCallback"))
    RWASSERT(object);
    sp->size+=_rt2dObjectStreamGetSize(object, sp->pool);
    RWRETURN(object);
}

RwUInt32
_rt2dSceneStreamGetSize(Rt2dObject *scene, _rt2dSceneResourcePool *pool)
{
    RwUInt32 size;
    SizeAndPool sp;

    RWFUNCTION(RWSTRING("_rt2dSceneStreamGetSize"));
    RWASSERT(scene);
    RWASSERT((scene->flag & Rt2dObjectIsLocked) == 0);
    RWASSERT(rt2DOBJECTTYPESCENE == scene->type);

    /* Scene is the size of the chunk header... */
    size = rwCHUNKHEADERSIZE;

    /* ...plus the size of the base class data */
    size += _rt2dObjectBaseStreamGetSize(scene);

    /* ...plus the size of the child count */
    size += sizeof(RwInt32);

    /* resource and child sizes */
    if (pool)
    {
        sp.size = 0;
        sp.pool = pool;

        /* ...plus the sum of the sizes of its children */
        Rt2dSceneForAllChildren(scene, _rt2dSceneAccumulateSizeOfChildCallback, &sp);
        size += sp.size;
    }
    else
    {
        _rt2dSceneResourcePool resourcePool;
        /* Create a shared resource pool */
        if (!_rt2dSceneResourcePoolInit(scene, &resourcePool))
        {
            RWRETURN(0);
        }

        /* Size of the pool */
        size += _rt2dSceneResourcePoolStreamGetSize(&resourcePool) + rwCHUNKHEADERSIZE;

        /* ...plus the sum of the sizes of its children */
        sp.size = 0;
        sp.pool = &resourcePool;
        Rt2dSceneForAllChildren(scene, _rt2dSceneAccumulateSizeOfChildCallback, &sp);
        size += sp.size;

        _rt2dSceneResourcePoolDestruct(&resourcePool);
    }

    RWRETURN(size);
}

/**
 * \ingroup rt2dscene
 * \ref Rt2dSceneStreamGetSize is used to determine the size in bytes
 * of the binary representation of the given scene. This is used in
 * the binary chunk header to indicate the size of the chunk. The size does
 * include the size of the chunk header.
 *
 * \param scene Pointer to the scene.
 *
 * \return Returns a \ref RwUInt32 value equal to the chunk size (in bytes) of
 * the scene
 *
 * \see Rt2dSceneStreamRead
 * \see Rt2dSceneStreamWrite
 *
 */
RwUInt32
Rt2dSceneStreamGetSize(Rt2dObject *scene)
{
    RwUInt32 size;
    RWAPIFUNCTION(RWSTRING("Rt2dSceneStreamGetSize"));
    RWASSERT(scene);
    RWASSERT((scene->flag & Rt2dObjectIsLocked) == 0);
    RWASSERT(rt2DOBJECTTYPESCENE == scene->type);

    size = _rt2dSceneStreamGetSize(scene, NULL);

    RWRETURN(size);
}

static Rt2dObject *
_rt2dSceneStreamReadToChildCallback(Rt2dObject *object, Rt2dObject *parent __RWUNUSED__, void *data)
{
    StreamAndPool *sp=(StreamAndPool *)(data);
    RWFUNCTION(RWSTRING("_rt2dSceneStreamReadToChildCallback"));
    RWASSERT(object);

    if (!_rt2dObjectBaseStreamReadTo(object, sp->stream))
    {
        RWRETURN((Rt2dObject *)NULL);
    }

    RWRETURN( _rt2dObjectStreamReadTo(object, sp->stream, sp->pool) );
}

Rt2dObject *
_rt2dSceneStreamReadTo(Rt2dObject *scene, RwStream *stream, _rt2dSceneResourcePool *pool)
{
    RwInt32 childCount = 0;
    Rt2dObject *objectPtr;
    _rt2dDepthOfObject *first, *finish, *i;

    RWFUNCTION(RWSTRING("_rt2dSceneStreamReadTo"));
    RWASSERT(scene);

    /* Get number of scene children */
    if (RwStreamRead(stream, &childCount, sizeof(childCount)) != sizeof(childCount))
    {
        RWRETURN((Rt2dObject *)NULL);
    }

    /* Convert to machine format */
    (void)RwMemNative32(&childCount, sizeof(childCount));

    /* Initialise scene with correct number of children */
    _rt2dSceneInit(scene, childCount);
    Rt2dSceneUnlock(scene);

    /* Read in children */
    if (pool)
    {
        StreamAndPool sp;
        sp.stream = stream;
        sp.pool = pool;
        Rt2dSceneForAllChildren(scene, _rt2dSceneStreamReadToChildCallback, &sp);
    }
    else
    {
        _rt2dSceneResourcePool resourcePool;
        StreamAndPool sp;

        /* Read the resource pool */
        if (!_rt2dSceneResourcePoolStreamReadTo(&resourcePool, stream))
        {
            RWRETURN(0);
        }

        /* Read the children, using the pool */
        sp.stream = stream;
        sp.pool = &resourcePool;
        Rt2dSceneForAllChildren(scene, _rt2dSceneStreamReadToChildCallback, &sp);

        _rt2dSceneResourcePoolDestruct(&resourcePool);
    }

    /* Depth sort */
    first = (_rt2dDepthOfObject*)
                     (rwSListGetArray(scene->data.scene.depths));
    finish = first + scene->data.scene.objectCount;

    /* Store ptrs to objects for depths */
    i = (_rt2dDepthOfObject *) first;
    objectPtr = (Rt2dObject *)
        rwSListGetArray(scene->data.scene.objects);
    while (i!=finish)
    {
        i->object = objectPtr;
        i++;
        objectPtr++;
    }

    /* Update z-orders */
    rt2dSceneDepthSort(scene);

    scene->flag &= ~Rt2dObjectIsLocked;
    RWRETURN(scene);
}

/**
 * \ingroup rt2dscene
 * \ref Rt2dSceneStreamRead
 * Reads a scene object from a stream
 *
 * The sequence to locate and read a scene from a binary stream is
 * as follows:
 * \code
   RwStream *stream;
   Rt2dObject *newScene;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_2DSCENE, NULL, NULL) )
       {
           newScene = Rt2dSceneStreamRead(stream);
       }

       RwStreamClose(stream, NULL);
   }
   \endcode
 *
 * \param stream is the stream to be read from
 *
 * \return returns a new scene if successful, NULL otherwise
 *
 * \see Rt2dSceneStreamWrite
 * \see Rt2dSceneStreamGetSize
 */
Rt2dObject *
Rt2dSceneStreamRead(RwStream *stream)
{
    Rt2dObject *scene = (Rt2dObject *) _rt2dObjectCreate();
    Rt2dObject *result = (Rt2dObject *)NULL;
    RWAPIFUNCTION(RWSTRING("Rt2dSceneStreamRead"));
    RWASSERT(stream);
    RWASSERT(scene);

    _rt2dObjectInit(scene, rt2DOBJECTTYPESCENE);

    /* Read base class information */
    if (!_rt2dObjectBaseStreamReadTo(scene, stream))
    {
        RWRETURN((Rt2dObject*)NULL);
    }

    result = _rt2dSceneStreamReadTo(scene, stream, NULL);

    if( NULL == result)
    {
        Rt2dSceneDestroy(scene);
    }

    RWRETURN(result);
}


/****************************************************************************/

/**
 * \ingroup rt2dscene
 * \ref Rt2dSceneSetDepthDirty
 * Records that the depth of a child has changed and a depth sort is necessary
 *
 * \param scene is the scene of which a child has changed depth
 *
 * \return returns the scene if successful, NULL otherwise
 *
 * \see Rt2dSceneRender
 * \see Rt2dObjectSetDepth
 */
Rt2dObject *
Rt2dSceneSetDepthDirty(Rt2dObject *scene)
{
    RWAPIFUNCTION(RWSTRING("Rt2dSceneSetDepthDirty"));
    RWASSERT(scene);

    scene->data.scene.isDirtyDepths = TRUE;

    RWRETURN(scene);
}

/**
 * \ingroup rt2dscene
 * \ref Rt2dSceneUpdateLTM goes through the scene hierarchy and
 * recalculates the scene to see if the LTM needs to be updated.
 * This is only necessary if an update is required before rendering
 * as this operation is also called by \ref Rt2dSceneRender.
 *
 * \param scene is the scene which may need its LTM updating.
 *
 * \return Returns the scene if successful, NULL otherwise.
 *
 * \see Rt2dSceneRender
 * \see Rt2dObjectStringRender
 * \see Rt2dShapeRender
 */
Rt2dObject *
Rt2dSceneUpdateLTM(Rt2dObject *scene)
{
    _rt2dDepthOfObject *first, *finish, *i;
    RwBool SceneDirty = FALSE;

    RWAPIFUNCTION(RWSTRING("Rt2dSceneUpdateLTM"));
    RWASSERT(scene);
    RWASSERT(scene->type == rt2DOBJECTTYPESCENE);
    RWASSERT((scene->flag & Rt2dObjectIsLocked) == 0);

    /* Scene level stuff */
    if((scene->flag & Rt2dObjectDirtyLTM) == Rt2dObjectDirtyLTM)
    {
        _rt2dCTMPush(&scene->MTM);
        RwMatrixCopy(&scene->LTM, _rt2dCTMGetDirect());

        scene->flag &= ~Rt2dObjectDirtyLTM;
        SceneDirty = TRUE;
    }
    else
    {
        _rt2dCTMSet(&scene->LTM);
    }

    /* Continue with rendering scene */
    first = (_rt2dDepthOfObject*)
                    (rwSListGetArray(scene->data.scene.depths));
    finish = first + scene->data.scene.objectCount;

    /* Render */
    for( i=first; i!=finish ; ++i)
    {
        if(TRUE == SceneDirty)
        {
            i->object->flag |= Rt2dObjectDirtyLTM;
        }

        if(rt2DOBJECTTYPESCENE == i->object->type)
        {
                Rt2dSceneUpdateLTM(i->object);
        }
        else
        {
            if( (i->object->flag & Rt2dObjectDirtyLTM) == Rt2dObjectDirtyLTM)
            {
                _rt2dCTMPush(&i->object->MTM);
                RwMatrixCopy(&i->object->LTM, _rt2dCTMGetDirect());

                i->object->flag &= ~Rt2dObjectDirtyLTM;

            }
            else
            {
                _rt2dCTMSet(&i->object->LTM);
            }

            if(rt2DOBJECTTYPEPICKREGION == i->object->type)
            {
                RwMatrixInvert(&i->object->data.pickRegion.transformation,
                        &i->object->LTM);
            }
        }

        Rt2dCTMPop();
    }

    /* Restore CTM */
    Rt2dCTMPop();

    RWRETURN(scene);
}

#if defined (__MWERKS__)
#if (defined(RWVERBOSE))
#pragma message (__FILE__ "/" _SKY_EXPAND(__LINE__) ": __MWERKS__ == " _SKY_EXPAND(__MWERKS__))
#endif /* (defined (__MWERKS__)) */
#if (__option (global_optimizer))
#pragma always_inline on
#endif /* (__option (global_optimizer)) */
#endif /*  defined (__MWERKS__) */
