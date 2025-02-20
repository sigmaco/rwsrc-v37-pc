/*
 * Morph target animation plugin
 */

/**
 * \ingroup rpmorph
 * \page rpmorphoverview RpMorph Plugin Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rpmorph.h
 * \li \b Libraries: rwcore, rpworld, rpmorph
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpMorphPluginAttach
 *
 * \subsection morphoverview Overview
 * The RpMorph plugin extends the core RenderWare Graphics API by adding morph animation
 * facilities to the developer’s arsenal.
 *
 * Morph animation is used to generate the intermediate frames needed to seamlessly
 * morph a geometry to match another. (Eg: changing a facial expression from a frown to a smile.)
 *
 * In use, the developer specifies a starting and ending RpGeometry objects for an
 * RpAtomic. These two objects are the morph targets. The RpMorph functions are then
 * used to generate interpolated new RpGeometry data from the two morph targets over time.
 */

/****************************************************************************
 *
 * Hierarchical animation controller
 * Copyright (C) 1998 Criterion Technologies
 *
 * Module  : rpmorph.c
 *
 * Purpose : Functions for creating and controlling morph target animation
 *
 ****************************************************************************/

/****************************************************************************
 Includes
 */

#include <stdlib.h>
#include <string.h>

#include "rpplugin.h"
#include "rpdbgerr.h"
#include "rpmorph.h"

/****************************************************************************
 Local Defines
 */

#define RPGEOMETRYGETANIM(geometry)                                        \
    ((RpMorphGeometryAnim *)(((RwUInt8 *)geometry) + geomAnimOffset))

#define RPGEOMETRYGETCONSTANIM(geometry)                                   \
    ((const RpMorphGeometryAnim *)(((const RwUInt8 *)geometry) +           \
                                   geomAnimOffset))

#define RWATOMICGETANIM(atomic)                                            \
    ((RpMorphAtomicAnim *)(((RwUInt8 *)atomic) + atomicAnimOffset))

#define RWATOMICGETCONSTANIM(atomic)                                       \
    ((const RpMorphAtomicAnim *)(((const RwUInt8 *)atomic) + atomicAnimOffset))

/****************************************************************************
 External non API RW functions
 */

/****************************************************************************
 Local (static) Types
 */

typedef struct RpMorphGeometryAnim RpMorphGeometryAnim;
struct RpMorphGeometryAnim
{
    /* This defines animation sequence,
     * and is an extension to RwGeometries */
    RwInt32             numInterpolators;
    RpMorphGeometryCallBack fpAnimCntrlCB;
    RpMorphInterpolator *allInterps;
};

typedef struct RpMorphAtomicAnim RpMorphAtomicAnim;
struct RpMorphAtomicAnim
{
    /* This defines the current position of the animation,
     * and is an extension to RpAtomics
     */
    RpMorphInterpolator *interp;
    RwReal              position;
    RpGeometry         *lastGeom;
};

typedef struct _rpBinaryAnimInterpolator _rpBinaryAnimInterpolator;
struct _rpBinaryAnimInterpolator
{
    /* This is for the binary storage of animation data */
    RwInt32             flags;
    RwInt32             startMorphTarget;
    RwInt32             endMorphTarget;
    RwReal              time;
    /* Even though the interpolators are joined in a loop,
     * * we don't rely on this, we store the sequence explicitly.
     * * Makes it easier later!!! */
    RwInt32             nextInterpIndex;
};

/* used for animation optimization */

typedef struct rpInterpInfo rpInterpInfo;
struct rpInterpInfo
{
    RwInt32             newIndex;
    RwInt32             refCntr;
    RwBool              optimized;
    RwBool              loopStart;
    RwBool              cut;
    RwInt32             visitedBy;
    RpMorphInterpolator *interp;
    rpInterpInfo       *next;
};

typedef struct rpKeyInfo rpKeyInfo;
struct rpKeyInfo
{
    RwBool              used;
    RwInt16             newIndex;
};

typedef             RwBool
    (*rpInterpInfoSpanTestCallBack) (rpInterpInfo ** interpSpanStart,
                                     RwInt32 interval,
                                     void *keys, RwReal delta);

/****************************************************************************
 Local (static) Globals
 */

/****************************************************************************
 Public Globals
 */

#if (defined(RWDEBUG))
long                rpMorphStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

RwModuleInfo        morphModule;

static RwInt32      atomicAnimOffset = 0;
static RwInt32      geomAnimOffset = 0;

/****************************************************************************
 Local (Static) Prototypes
 */

/* No binary stream stuff for atomics */

#if (defined(_RWSTRDUP))
static RwChar      *
_rwStrDup(RwChar * name)
{
    RwChar             *newName;

    RWFUNCTION(RWSTRING("_rwStrDup"));

    newName = RwMalloc(rwstrlen(name) + 1,
        rwID_MORPHPLUGIN | rwMEMHINTDUR_EVENT);
    if (newName)
    {
        rwstrcpy(newName, name);
    }
    RWRETURN(newName);
}
#endif /* (defined(_RWSTRDUP)) */

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   Plugin constructor/destructor functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

static void        *
AnimOpen(void *instance,
         RwInt32 __RWUNUSED__ offset, RwInt32 __RWUNUSED__ size)
{
    RWFUNCTION(RWSTRING("AnimOpen"));
    RWASSERT(instance);

    /* one more module instance */
    morphModule.numInstances++;

    RWRETURN(instance);
}

static void        *
AnimClose(void *instance,
          RwInt32 __RWUNUSED__ offset, RwInt32 __RWUNUSED__ size)
{
    RWFUNCTION(RWSTRING("AnimClose"));
    RWASSERT(instance);

    /* one less module instance */
    morphModule.numInstances--;

    RWRETURN(instance);
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   Morph target animation functions
   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

static void        *
AtomicAnimCtorDtor(void *object,
                   RwInt32 __RWUNUSED__ offset,
                   RwInt32 __RWUNUSED__ size)
{
    RpMorphAtomicAnim  *atomicAnim;

    RWFUNCTION(RWSTRING("AtomicAnimCtorDtor"));
    RWASSERT(object);

    atomicAnim = RWATOMICGETANIM(object);

    /* Most of this gets set up when we try and add time */
    /* NOTE: Constructor and Destructor are the same */
    atomicAnim->interp = (RpMorphInterpolator *)NULL;
    atomicAnim->position = (RwReal) (0.0);
    atomicAnim->lastGeom = (RpGeometry *)NULL;

    RWRETURN(object);
}

static void        *
AtomicAnimCopy(void *dstObject,
               const void *srcObject,
               RwInt32 __RWUNUSED__ offset, RwInt32 __RWUNUSED__ size)
{
    RpMorphAtomicAnim  *dstAtomicAnim;
    const RpMorphAtomicAnim *srcAtomicAnim;
    RpInterpolator     *dstInterpolator;
    const RpGeometry   *geometry;

    RWFUNCTION(RWSTRING("AtomicAnimCopy"));
    RWASSERT(dstObject);
    RWASSERT(srcObject);

    dstAtomicAnim = RWATOMICGETANIM(dstObject);
    srcAtomicAnim = RWATOMICGETCONSTANIM(srcObject);
    dstInterpolator = RpAtomicGetInterpolator((RpAtomic *) dstObject);
    geometry = RpAtomicGetGeometry((const RpAtomic *) srcObject);

    /* If the geometry has changed underneath the atomic,
     * don't copy anything */
    if (srcAtomicAnim->lastGeom != geometry)
    {
        dstAtomicAnim->interp = (RpMorphInterpolator *)NULL;
        dstAtomicAnim->position = (RwReal) (0.0);
        dstAtomicAnim->lastGeom = (RpGeometry *)NULL;
    }
    else
    {
        /* Duplicate all the fields
         * - we assume that the atomic shares the geometry */
        dstAtomicAnim->interp = srcAtomicAnim->interp;
        dstAtomicAnim->position = srcAtomicAnim->position;
        dstAtomicAnim->lastGeom = srcAtomicAnim->lastGeom;

        /* Also update the interpolator in the destination
         * if an interpolator is set up */
        if (dstAtomicAnim->interp)
        {
            RpInterpolatorSetStartMorphTarget(dstInterpolator,
                                              dstAtomicAnim->
                                              interp->startMorphTarget,
                                              (RpAtomic *)dstObject);
            RpInterpolatorSetEndMorphTarget(dstInterpolator,
                                            dstAtomicAnim->
                                            interp->endMorphTarget,
                                            (RpAtomic *)dstObject);
            RpInterpolatorSetScale(dstInterpolator,
                                   dstAtomicAnim->interp->time,
                                   (RpAtomic *)dstObject);
            RpInterpolatorSetValue(dstInterpolator,
                                   dstAtomicAnim->position,
                                   (RpAtomic *)dstObject);
        }
    }

    RWRETURN(dstObject);
}

static void        *
GeomAnimDtor(void *object,
             RwInt32 __RWUNUSED__ offset, RwInt32 __RWUNUSED__ size)
{
    RpMorphGeometryAnim *geomAnim;

    RWFUNCTION(RWSTRING("GeomAnimDtor"));
    RWASSERT(object);

    geomAnim = RPGEOMETRYGETANIM(object);

    if (geomAnim->allInterps)
    {
        /* Free up the interpolator memory
         * allocated in RpMorphGeometryCreateInterpolators */
        RwFree(geomAnim->allInterps);
    }
    geomAnim->allInterps = (RpMorphInterpolator *)NULL;
    geomAnim->numInterpolators = 0;

    RWRETURN(object);
}

static void        *
GeomAnimCopy(void *dstObject,
             const void *srcObject,
             RwInt32 __RWUNUSED__ offset, RwInt32 __RWUNUSED__ size)
{
    RpMorphGeometryAnim *dstGeomAnim;
    const RpMorphGeometryAnim *srcGeomAnim;

    RWFUNCTION(RWSTRING("GeomAnimCopy"));
    RWASSERT(dstObject);
    RWASSERT(srcObject);

    dstGeomAnim = RPGEOMETRYGETANIM(dstObject);
    srcGeomAnim = RPGEOMETRYGETCONSTANIM(srcObject);

    /* Only need to do stuff if the src has interpolators
     * - dst has already been ctor'ed */
    if (srcGeomAnim->numInterpolators > 0)
    {
        /* Duplicate the interpolators */
        RwUInt32            sizeOfInterps =
            srcGeomAnim->numInterpolators * sizeof(RpMorphInterpolator);

        dstGeomAnim->allInterps = ((RpMorphInterpolator *)
                                   RwMalloc(sizeOfInterps,
                                   rwID_MORPHPLUGIN | rwMEMHINTDUR_EVENT));
        if (!dstGeomAnim->allInterps)
        {
            /* Duplication failed due to malloc failure, send error back */
            RWRETURN(NULL);
        }

        /* Copy across the info itself */
        dstGeomAnim->numInterpolators = srcGeomAnim->numInterpolators;
        memcpy(dstGeomAnim->allInterps,
               srcGeomAnim->allInterps, sizeOfInterps);
    }

    RWRETURN(dstObject);
}

static RwStream    *
GeomAnimWriteStream(RwStream * stream,
                    RwInt32 __RWUNUSED__ binaryLength,
                    const void *object,
                    RwInt32 __RWUNUSED__ offsetInObject,
                    RwInt32 __RWUNUSED__ sizeInObject)
{
    const RpMorphGeometryAnim *geomAnim;
    RwInt32             numInterpolators;
    RwInt32             i;

    RWFUNCTION(RWSTRING("GeomAnimWriteStream"));
    RWASSERT(stream);
    RWASSERT(object);

    geomAnim = RPGEOMETRYGETCONSTANIM(object);

    numInterpolators = geomAnim->numInterpolators;
    if (!RwStreamWriteInt
        (stream, &numInterpolators, sizeof(numInterpolators)))
    {
        /* Failed to write to the stream, ooops */
        RWRETURN(FALSE);
    }

    for (i = 0; i < numInterpolators; i++)
    {
        _rpBinaryAnimInterpolator binAnimInterp;

        RWASSERT(geomAnim->allInterps);

        binAnimInterp.flags = geomAnim->allInterps[i].flags;
        binAnimInterp.startMorphTarget =
            geomAnim->allInterps[i].startMorphTarget;
        binAnimInterp.endMorphTarget =
            geomAnim->allInterps[i].endMorphTarget;
        binAnimInterp.time = geomAnim->allInterps[i].time;
        binAnimInterp.nextInterpIndex =
            geomAnim->allInterps[i].next - geomAnim->allInterps;

        (void)RwMemRealToFloat32(&binAnimInterp.time,
                           sizeof(binAnimInterp.time));
        (void)RwMemLittleEndian32(&binAnimInterp, sizeof(binAnimInterp));

        if (!RwStreamWrite
            (stream, &binAnimInterp, sizeof(binAnimInterp)))
        {
            /* Failed to write to the stream, ooops */
            RWRETURN((RwStream *)NULL);
        }
    }

    /* OK, we're all done */
    RWRETURN(stream);
}

static RwStream    *
GeomAnimReadStream(RwStream * stream,
                   RwInt32 __RWUNUSED__ binaryLength,
                   void *object,
                   RwInt32 __RWUNUSED__ offsetInObject,
                   RwInt32 __RWUNUSED__ sizeInObject)
{
    RpMorphGeometryAnim *geomAnim;
    RwInt32             numInterpolators;
    RwInt32             i;

    RWFUNCTION(RWSTRING("GeomAnimReadStream"));
    RWASSERT(stream);
    RWASSERT(object);

    geomAnim = RPGEOMETRYGETANIM(object);

    if (!RwStreamReadInt
        (stream, &numInterpolators, sizeof(numInterpolators)))
    {
        /* Failed to read the stream, ooops */
        RWRETURN((RwStream *)NULL);
    }

    if (numInterpolators > 0)
    {
        if (!RpMorphGeometryCreateInterpolators
            ((RpGeometry *) object, numInterpolators))
        {
            /* Failed, propagate error back */
            RWRETURN((RwStream *)NULL);
        }

        /* Now need to read in interpolators themselves */
        for (i = 0; i < numInterpolators; i++)
        {
            _rpBinaryAnimInterpolator binAnimInterp;
            RwUInt32            size;

            size = sizeof(binAnimInterp);
            if (RwStreamRead(stream, &binAnimInterp, size) != size)
            {
                /* Failed to read the stream, ooops */
                RWRETURN((RwStream *)NULL);
            }

            (void)RwMemNative32(&binAnimInterp, sizeof(binAnimInterp));
            (void)RwMemFloat32ToReal(&binAnimInterp.time,
                               sizeof(binAnimInterp.time));

            RWASSERT(geomAnim->allInterps);

            /* Do it this way so we can do our own thing
             * with the next pointer!! */
            geomAnim->allInterps[i].flags = binAnimInterp.flags;
            geomAnim->allInterps[i].startMorphTarget =
                (RwInt16) binAnimInterp.startMorphTarget;
            geomAnim->allInterps[i].endMorphTarget =
                (RwInt16) binAnimInterp.endMorphTarget;
            geomAnim->allInterps[i].time = binAnimInterp.time;
            geomAnim->allInterps[i].recipTime =
                ((RwReal) (1.0)) / (binAnimInterp.time);
            geomAnim->allInterps[i].next =
                &geomAnim->allInterps[binAnimInterp.nextInterpIndex];
        }
    }

    /* All done */
    RWRETURN(stream);
}

static              RwInt32
GeomAnimSizeStream(const void *object,
                   RwInt32 __RWUNUSED__ offsetInObject,
                   RwInt32 __RWUNUSED__ sizeInObject)
{
    RwBool needToStream;
    const RpMorphGeometryAnim *geomAnim =
        RPGEOMETRYGETCONSTANIM(object);

    RWFUNCTION(RWSTRING("GeomAnimSizeStream"));

    /* No interpolators, no morph data */
    needToStream = (0 != geomAnim->numInterpolators);

    if (needToStream)
    {
        RwInt32             size;
        size = sizeof(RwInt32);    /* numInterpolators */
        size +=
            (geomAnim->numInterpolators *
             sizeof(_rpBinaryAnimInterpolator));

        RWRETURN(size);
    }

    /* By default do not need to stream extension data */
    RWRETURN(0);
}

static              RwReal
DefaultGeomAnimCB(RpAtomic * atomic, RwReal position)
{
    RpMorphAtomicAnim  *atomicAnim;
    RwReal              newPos;

    RWFUNCTION(RWSTRING("DefaultGeomAnimCB"));
    RWASSERT(atomic);

    atomicAnim = RWATOMICGETANIM(atomic);
    newPos = position - atomicAnim->interp->time;
    if (newPos < (RwReal) (0.0))
    {
        newPos = (RwReal) (0.0);
    }

    atomicAnim->interp = atomicAnim->interp->next;

    RWRETURN(newPos);
}

static void        *
GeomAnimCtor(void *object,
             RwInt32 __RWUNUSED__ offset, RwInt32 __RWUNUSED__ size)
{
    RpMorphGeometryAnim *geomAnim;

    RWFUNCTION(RWSTRING("GeomAnimCtor"));
    RWASSERT(object);

    geomAnim = RPGEOMETRYGETANIM(object);

    geomAnim->numInterpolators = 0;
    geomAnim->allInterps = (RpMorphInterpolator *)NULL;
    geomAnim->fpAnimCntrlCB = DefaultGeomAnimCB;

    RWRETURN(object);
}

/**
 * \ingroup rpmorph
 * \ref RpMorphGeometryCreateInterpolators
 * is used to create the given number of interpolators in the animation
 * extension data of the specified geometry.
 *
 * The morph plugin must be attached before using this function.
 *
 * \param geometry Pointer to the geometry.
 *
 * \param numInterps  A RwInt32 value equal to the number of interpolators.
 *
 * \return
 * a pointer to the geometry if successful or NULL if there is an error.
 *
 * \see RpMorphGeometrySetInterpolator
 * \see RpMorphGeometrySetNextInterpolator
 * \see RpMorphGeometrySetCallBack
 * \see RpMorphGeometryGetCallBack
 * \see RpMorphAtomicAddTime
 * \see RpMorphAtomicSetTime
 * \see RpMorphPluginAttach
 */
RpGeometry         *
RpMorphGeometryCreateInterpolators(RpGeometry * geometry,
                                   RwInt32 numInterps)
{
    RpMorphGeometryAnim *geomAnim;

    RWAPIFUNCTION(RWSTRING("RpMorphGeometryCreateInterpolators"));
    RWASSERT(morphModule.numInstances);
    RWASSERT(geometry);
    RWASSERT(numInterps > 0);

    /* Allocate some interpolators */
    geomAnim = RPGEOMETRYGETANIM(geometry);

    /* Prevent memory leaks by freeing up old interpolators */
    if (geomAnim->allInterps)
    {
        RwFree(geomAnim->allInterps);
        geomAnim->numInterpolators = 0;
        geomAnim->allInterps = (RpMorphInterpolator *)NULL;
    }

    geomAnim->allInterps = (RpMorphInterpolator *)
        RwMalloc(numInterps * sizeof(RpMorphInterpolator),
                 rwID_MORPHPLUGIN | rwMEMHINTDUR_EVENT);

    if (!geomAnim->allInterps)
    {
        /* Malloc failure */
        RWRETURN((RpGeometry *)NULL);
    }

    /* Save the quantity */
    geomAnim->numInterpolators = numInterps;

    /* Clear it all out, so we don't do stupid things */
    memset(geomAnim->allInterps, 0,
           numInterps * sizeof(RpMorphInterpolator));

    RWRETURN(geometry);
}

/**
 * \ingroup rpmorph
 * \ref RpMorphGeometrySetInterpolator
 * is used to initialize the interpolator in the specified geometry with
 * the given index using the specified start morph target index, end
 * morph target index and time duration (in seconds) that the
 * interpolator will run for. The interpolator is specified with an index
 * into the interpolator array in the geometry's animation extension
 * data. The start and end morph targets are specified with indices into
 * the geometry's morph target array.
 *
 * The morph plugin must be attached before using this function.
 *
 * \param geometry  Pointer to the geometry.
 *
 * \param interpNum  A RwInt32 value equal to the index of the
 * interpolator.
 *
 * \param startKey  A RwInt32 value equal to the index of the start
 * morph target.
 *
 * \param endKey  A RwInt32 value equal to the index of the end morph
 * target.
 *
 * \param time  A RwReal value equal to the interpolator duration.
 *
 * \return
 * a pointer to the geometry if successful or NULL if there is an error.
 *
 * \see RpMorphGeometryCreateInterpolators
 * \see RpMorphGeometrySetNextInterpolator
 * \see RpMorphGeometrySetCallBack
 * \see RpMorphGeometryGetCallBack
 * \see RpMorphAtomicAddTime
 * \see RpMorphAtomicSetTime
 * \see RpMorphPluginAttach
 */
RpGeometry         *
RpMorphGeometrySetInterpolator(RpGeometry * geometry, RwInt32 interpNum,
                               RwInt32 startKey, RwInt32 endKey,
                               RwReal time)
{
    RpMorphGeometryAnim *geomAnim;
    RpMorphInterpolator *interpolator;

    RWAPIFUNCTION(RWSTRING("RpMorphGeometrySetInterpolator"));
    RWASSERT(morphModule.numInstances);
    RWASSERT(geometry);

    geomAnim = RPGEOMETRYGETANIM(geometry);

    RWASSERT(interpNum >= 0);
    RWASSERT(interpNum < geomAnim->numInterpolators);
    RWASSERT(geomAnim->allInterps);

    interpolator = &geomAnim->allInterps[interpNum];

    interpolator->flags = 0;
    interpolator->startMorphTarget = (RwInt16) startKey;
    interpolator->endMorphTarget = (RwInt16) endKey;
    interpolator->time = time;
    interpolator->recipTime = ((RwReal) (1.0)) / (time);

    if (interpNum == (geomAnim->numInterpolators - 1))
    {
        RWASSERT(geomAnim->allInterps);
        interpolator->next = &geomAnim->allInterps[0];
    }
    else
    {
        interpolator->next = interpolator + 1;
    }

    RWRETURN(geometry);
}

/**
 * \ingroup rpmorph
 * \ref RpMorphGeometrySetNextInterpolator
 * is used to set the interpolator that will be executed after the
 * current interpolator has expired.
 *
 * The morph plugin must be attached before using this function.
 *
 * \param geometry  Pointer to the geometry.
 *
 * \param interpNum  A RwInt32 value equal to the index of the current
 * interpolator.
 *
 * \param interpNumNext  A RwInt32 value equal to the index of the next
 * interpolator.
 *
 * \return
 * a pointer to the geometry if successful or NULL if there is an error.
 *
 * \see RpMorphGeometryCreateInterpolators
 * \see RpMorphGeometrySetInterpolator
 * \see RpMorphGeometrySetCallBack
 * \see RpMorphGeometryGetCallBack
 * \see RpMorphAtomicAddTime
 * \see RpMorphAtomicSetTime
 * \see RpMorphPluginAttach
 */
RpGeometry         *
RpMorphGeometrySetNextInterpolator(RpGeometry * geometry,
                                   RwInt32 interpNum,
                                   RwInt32 interpNumNext)
{
    RpMorphGeometryAnim *geomAnim;

    RpMorphInterpolator *interpolator;
    RpMorphInterpolator *interpolatorNext;

    RWAPIFUNCTION(RWSTRING("RpMorphGeometrySetNextInterpolator"));
    RWASSERT(morphModule.numInstances);
    RWASSERT(geometry);

    geomAnim = RPGEOMETRYGETANIM(geometry);

    RWASSERT(interpNum >= 0);
    RWASSERT(interpNum < geomAnim->numInterpolators);

    RWASSERT(geomAnim->allInterps);
    interpolator = &geomAnim->allInterps[interpNum];
    interpolatorNext = &geomAnim->allInterps[interpNumNext];

    interpolator->next = interpolatorNext;

    RWRETURN(geometry);
}

/**
 * \ingroup rpmorph
 * \ref RpMorphGeometrySetCallBack
 * is used to define the callback function associated with the specified
 * geometry.
 *
 * The morph plugin must be attached before using this function.
 *
 * \param geometry  Pointer to the geometry.
 *
 * \param animCB  Pointer to the callback function.
 *
 * \return
 * pointer to the geometry if successful or NULL if there is an error.
 *
 * \see RpMorphGeometryGetCallBack
 * \see RpMorphGeometryCreateInterpolators
 * \see RpMorphGeometrySetInterpolator
 * \see RpMorphGeometrySetNextInterpolator
 * \see RpMorphAtomicAddTime
 * \see RpMorphAtomicSetTime
 * \see RpMorphPluginAttach
 */
RpGeometry         *
RpMorphGeometrySetCallBack(RpGeometry * geometry,
                           RpMorphGeometryCallBack animCB)
{
    RpMorphGeometryAnim *geomAnim;

    RWAPIFUNCTION(RWSTRING("RpMorphGeometrySetCallBack"));

    RWASSERT(morphModule.numInstances);
    RWASSERT(geometry);
    RWASSERT(animCB);

    geomAnim = RPGEOMETRYGETANIM(geometry);

    geomAnim->fpAnimCntrlCB = animCB;

    RWRETURN(geometry);
}

/**
 * \ingroup rpmorph
 * \ref RpMorphGeometryGetCallBack
 * is used to retrieve the callback function associated with the
 * specified geometry.
 *
 * The morph plugin must be attached before using this function.
 *
 * \param geometry  Pointer to the geometry.
 *
 * \return
 * pointer to the callback function if successful or NULL if there is an
 * error.
 *
 * \see RpMorphGeometrySetCallBack
 * \see RpMorphGeometryCreateInterpolators
 * \see RpMorphGeometrySetInterpolator
 * \see RpMorphGeometrySetNextInterpolator
 * \see RpMorphAtomicAddTime
 * \see RpMorphAtomicSetTime
 * \see RpMorphPluginAttach
 */
RpMorphGeometryCallBack
RpMorphGeometryGetCallBack(const RpGeometry * geometry)
{
    const RpMorphGeometryAnim *geomAnim;

    RWAPIFUNCTION(RWSTRING("RpMorphGeometryGetCallBack"));
    RWASSERT(morphModule.numInstances);
    RWASSERT(geometry);

    geomAnim = RPGEOMETRYGETCONSTANIM(geometry);

    RWRETURN(geomAnim->fpAnimCntrlCB);
}

/**
 * \ingroup rpmorph
 * \ref RpMorphAtomicSetCurrentInterpolator
 * is used to set the specified atomic's current interpolator.
 *
 * The morph plugin must be attached before using this function.
 *
 * \param atomic  Pointer to the atomic for which the morph target animation
 * extension data is to be set.
 * \param interpNum  a RwInt32 value equal to the index of an interpolator which is
 * to become the current interpolator.
 *
 * \return
 * a pointer to the atomic if successful or NULL if there is an error.
 *
 * \see RpMorphAtomicSetCurrentInterpolator
 * \see RpMorphGeometryGetInterpolator
 * \see RpMorphAtomicAddTime
 * \see RpMorphAtomicSetTime
 * \see RpMorphGeometryCreateInterpolators
 * \see RpMorphGeometrySetInterpolator
 * \see RpMorphGeometrySetNextInterpolator
 * \see RpMorphGeometrySetCallBack
 * \see RpMorphGeometryGetCallBack
 * \see RpMorphPluginAttach
 */
RpAtomic           *
RpMorphAtomicSetCurrentInterpolator(RpAtomic * atomic,
                                    RwInt32 interpNum)
{

    RpGeometry         *geometry;

    RWAPIFUNCTION(RWSTRING("RpMorphAtomicSetCurrentInterpolator"));
    RWASSERT(morphModule.numInstances);
    RWASSERT(atomic);

    geometry = RpAtomicGetGeometry(atomic);

    if (geometry)
    {
        RpMorphAtomicAnim  *atomicAnim = RWATOMICGETANIM(atomic);
        RpMorphGeometryAnim *geomAnim = RPGEOMETRYGETANIM(geometry);
        RpInterpolator     *interpolator =
            RpAtomicGetInterpolator(atomic);

        RwInt16             start;
        RwInt16             end;
        RwReal              scale;

        RWASSERT(interpNum >= 0);
        RWASSERT(interpNum < geomAnim->numInterpolators);

        /* If the geometry has changed then initialise it */
        if ((atomicAnim->lastGeom != geometry))
        {
            atomicAnim->lastGeom = geometry;
        }

        RWASSERT(geomAnim->allInterps);
        atomicAnim->interp = &geomAnim->allInterps[interpNum];

        /* Just need to update the interpolator in the atomic */
        start = atomicAnim->interp->startMorphTarget;
        RpInterpolatorSetStartMorphTarget(interpolator, start, atomic);
        end = atomicAnim->interp->endMorphTarget;
        RpInterpolatorSetEndMorphTarget(interpolator, end, atomic);
        RpInterpolatorSetValue(interpolator, 0.0f, atomic);
        scale = atomicAnim->interp->time;
        RpInterpolatorSetScale(interpolator, scale, atomic);

        /* All done */
        RWRETURN(atomic);
    }

    /* No geometry,
     * how can we possible do any morph target animation */
    RWERROR((E_RP_MORPH_NOGEOMETRY));
    RWRETURN((RpAtomic *)NULL);
}

/**
 * \ingroup rpmorph
 * \ref RpMorphAtomicGetCurrentInterpolator
 * is used to retrieve the index of the specified atomic's current
 * interpolator.
 *
 * The morph plugin must be attached before using this function.
 *
 * \param atomic  Pointer to the atomic containing morph target animation
 * extension data.
 *
 * \return
 * a RwInt32 value equal to the index of the current interpolator if
 * successful or - 1 if there is an error.
 *
 * \see RpMorphAtomicSetCurrentInterpolator
 * \see RpMorphGeometryGetInterpolator
 * \see RpMorphAtomicAddTime
 * \see RpMorphAtomicSetTime
 * \see RpMorphGeometryCreateInterpolators
 * \see RpMorphGeometrySetInterpolator
 * \see RpMorphGeometrySetNextInterpolator
 * \see RpMorphGeometrySetCallBack
 * \see RpMorphGeometryGetCallBack
 * \see RpMorphPluginAttach
 */
RwInt32
RpMorphAtomicGetCurrentInterpolator(RpAtomic * atomic)
{
    RpGeometry         *geometry;

    RWAPIFUNCTION(RWSTRING("RpMorphAtomicGetCurrentInterpolator"));
    RWASSERT(morphModule.numInstances);
    RWASSERT(atomic);

    geometry = RpAtomicGetGeometry(atomic);

    if (geometry)
    {
        RpMorphAtomicAnim  *atomicAnim = RWATOMICGETANIM(atomic);
        RpMorphGeometryAnim *geomAnim = RPGEOMETRYGETANIM(geometry);
        RwInt32             i;

        for (i = 0; i < geomAnim->numInterpolators; i++)
        {
            RWASSERT(geomAnim->allInterps);

            if (atomicAnim->interp == &geomAnim->allInterps[i])
            {
                /* All done */
                RWRETURN(i);
            }
        }

        RWRETURN(-1);
    }

    /* No geometry,
     * how can we possible do any morph target animation */
    RWERROR((E_RP_MORPH_NOGEOMETRY));
    RWRETURN(-1);
}

/**
 * \ingroup rpmorph
 * \ref RpMorphGeometryGetInterpolator
 * is used to retrieve a pointer to the specified geometry's interpolator
 * which has the given index.
 *
 * The morph plugin must be attached before using this function.
 *
 * \param geometry  Pointer to the geometry containing morph target animation
 * extension data.
 *
 * \param interpNum  A RwInt32 value equal to the index of the
 * interpolator.
 *
 * \return
 * a pointer to the interpolator if successful or NULL if there is an
 * error.
 *
 * \see RpMorphAtomicSetCurrentInterpolator
 * \see RpMorphAtomicGetCurrentInterpolator
 * \see RpMorphAtomicAddTime
 * \see RpMorphAtomicSetTime
 * \see RpMorphGeometryCreateInterpolators
 * \see RpMorphGeometrySetInterpolator
 * \see RpMorphGeometrySetNextInterpolator
 * \see RpMorphGeometrySetCallBack
 * \see RpMorphGeometryGetCallBack
 * \see RpMorphPluginAttach
 */
RpMorphInterpolator *
RpMorphGeometryGetInterpolator(RpGeometry * geometry, RwInt32 interpNum)
{
    RpMorphGeometryAnim *geomAnim;

    RWAPIFUNCTION(RWSTRING("RpMorphGeometryGetInterpolator"));
    RWASSERT(morphModule.numInstances);
    RWASSERT(geometry);

    geomAnim = RPGEOMETRYGETANIM(geometry);

    if ((interpNum >= 0) && (interpNum < geomAnim->numInterpolators))
    {
        RWASSERT(geomAnim->allInterps);
        RWRETURN(&geomAnim->allInterps[interpNum]);
    }

    RWRETURN((RpMorphInterpolator *)NULL);
}

/* Legacy version of above */
RpMorphInterpolator *
_rpMorphAtomicGetInterpolator(RpAtomic * atomic, RwInt32 interpNum)
{
    RpGeometry         *geometry;

    RWFUNCTION(RWSTRING("_rpMorphAtomicGetInterpolator"));
    RWASSERT(morphModule.numInstances);
    RWASSERT(atomic);

    geometry = RpAtomicGetGeometry(atomic);

    if (geometry)
    {
        RWRETURN(RpMorphGeometryGetInterpolator(geometry, interpNum));
    }

    /* No geometry,
     * how can we possible do any morph target animation */
    RWERROR((E_RP_MORPH_NOGEOMETRY));
    RWRETURN((RpMorphInterpolator *)NULL);
}

/**
 * \ingroup rpmorph
 * \ref RpMorphAtomicSetTime
 * is used to define the position of the specified atomic's current
 * interpolator.
 *
 * The morph plugin must be attached before using this function.
 *
 * \param atomic  Pointer to the atomic containing morph target animation
 * extension data.
 *
 * \param time  A RwReal value equal to the time.
 *
 * \return
 * a pointer to the atomic if successful or NULL if there is an error.
 *
 * \see RpMorphAtomicAddTime
 * \see RpMorphAtomicSetCurrentInterpolator
 * \see RpMorphAtomicGetCurrentInterpolator
 * \see RpMorphGeometryGetInterpolator
 * \see RpMorphGeometryCreateInterpolators
 * \see RpMorphGeometrySetInterpolator
 * \see RpMorphGeometrySetNextInterpolator
 * \see RpMorphGeometrySetCallBack
 * \see RpMorphGeometryGetCallBack
 * \see RpMorphPluginAttach
 */
RpAtomic           *
RpMorphAtomicSetTime(RpAtomic * atomic, RwReal time)
{
    RpGeometry         *geometry;

    RWAPIFUNCTION(RWSTRING("RpMorphAtomicSetTime"));
    RWASSERT(morphModule.numInstances);
    RWASSERT(atomic);
    RWASSERT(atomic);

    geometry = RpAtomicGetGeometry(atomic);

    if (geometry)
    {
        RpMorphAtomicAnim  *atomicAnim = RWATOMICGETANIM(atomic);
        RpMorphGeometryAnim *geomAnim = RPGEOMETRYGETANIM(geometry);
        RpInterpolator     *interpolator =
            RpAtomicGetInterpolator(atomic);

        /* If the geometry has changed or
         * the interp pointer has not been initialised,
         * then initialise it to the start of the animation...
         */
        if ((atomicAnim->lastGeom != geometry) || (!atomicAnim->interp))
        {
            atomicAnim->interp = geomAnim->allInterps;
            atomicAnim->lastGeom = geometry;
        }

        /* Check the atomicAnim has an interpolators */
        if (atomicAnim->interp)
        {
            RwInt16             start;
            RwInt16             end;
            RwReal              scale;

            RWASSERT(time >= (RwReal) (0.0));
            RWASSERT(time <= atomicAnim->interp->time);

            /* Set the time */
            atomicAnim->position = time;

            /* Just need to update the interpolator
             * in the atomic */

            start = atomicAnim->interp->startMorphTarget;
            RpInterpolatorSetStartMorphTarget(interpolator, start, atomic);

            end = atomicAnim->interp->endMorphTarget;
            RpInterpolatorSetEndMorphTarget(interpolator, end, atomic);
            RpInterpolatorSetValue(interpolator, atomicAnim->position, atomic);
            scale = atomicAnim->interp->time;
            RpInterpolatorSetScale(interpolator, scale, atomic);
        }

        /* All done */
        RWRETURN(atomic);
    }

    /* No geometry,
     * how can we possible do any morph target animation */
    RWERROR((E_RP_MORPH_NOGEOMETRY));
    RWRETURN((RpAtomic *)NULL);
}

/**
 * \ingroup rpmorph
 * \ref RpMorphAtomicAddTime
 * is used to advance the morph target animation of the specified atomic
 * by the given amount. The time is measured in seconds and would
 * normally represent the time elapsed between two successive rendering
 * frames.
 *
 * The morph plugin must be attached before using this function.
 *
 * \param atomic  Pointer to the atomic containing morph target animation
 * extension data.
 *
 * \param time  A RwReal value equal to the time increment.
 *
 * \return
 * a pointer to the atomic if successful or NULL if there is an error.
 *
 * \see RpMorphAtomicSetTime
 * \see RpMorphAtomicSetCurrentInterpolator
 * \see RpMorphAtomicGetCurrentInterpolator
 * \see RpMorphGeometryGetInterpolator
 * \see RpMorphGeometryCreateInterpolators
 * \see RpMorphGeometrySetInterpolator
 * \see RpMorphGeometrySetNextInterpolator
 * \see RpMorphGeometrySetCallBack
 * \see RpMorphGeometryGetCallBack
 * \see RpMorphPluginAttach
 */
RpAtomic           *
RpMorphAtomicAddTime(RpAtomic * atomic, RwReal time)
{
    RpGeometry         *geometry;

    RWAPIFUNCTION(RWSTRING("RpMorphAtomicAddTime"));
    RWASSERT(morphModule.numInstances);
    RWASSERT(atomic);

    geometry = RpAtomicGetGeometry(atomic);

    if (geometry)
    {
        RpMorphAtomicAnim  *atomicAnim = RWATOMICGETANIM(atomic);
        RpMorphGeometryAnim *geomAnim = RPGEOMETRYGETANIM(geometry);
        RpInterpolator     *interpolator =
            RpAtomicGetInterpolator(atomic);

        /* If the geometry has changed or
         * the interp pointer has not been initialised,
         * then initialise it to the start of the animation...
         */
        if ((atomicAnim->lastGeom != geometry) || (!atomicAnim->interp))
        {
            atomicAnim->interp = geomAnim->allInterps;
            atomicAnim->lastGeom = geometry;
        }

        /* Check the atomicAnim has an interpolators */
        if (atomicAnim->interp)
        {
            RwInt16             start;
            RwInt16             end;
            RwReal              scale;

            /* Add the time */
            atomicAnim->position += time;

            /* Skip to next interpolator if necessary */
            while (atomicAnim->position > atomicAnim->interp->time)
            {
                atomicAnim->position =
                    geomAnim->fpAnimCntrlCB(atomic,
                                            atomicAnim->position);
            }

            /* Just need to update the interpolator in the atomic */
            start = atomicAnim->interp->startMorphTarget;
            RpInterpolatorSetStartMorphTarget(interpolator, start, atomic);
            end = atomicAnim->interp->endMorphTarget;
            RpInterpolatorSetEndMorphTarget(interpolator, end, atomic);
            RpInterpolatorSetValue(interpolator, atomicAnim->position, atomic);
            scale = atomicAnim->interp->time;
            RpInterpolatorSetScale(interpolator, scale, atomic);
        }

        /* All done */
        RWRETURN(atomic);
    }

    /* No geometry,
     * how can we possible do any morph target animation */
    RWERROR((E_RP_MORPH_NOGEOMETRY));
    RWRETURN((RpAtomic *)NULL);
}

/**
 * \ingroup rpmorph
 * \ref RpMorphPluginAttach
 * is used to attach the morph plugin to the RenderWare system to enable
 * the generation of morph target animations. The plugin must be
 * attached between initializing the system with RwEngineInit and opening
 * it with RwEngineOpen.
 *
 * \note The morph plugin requires the world plug-in to be
 * attached. The include file rpmorph.h is also required and must be
 * included by an application wishing to generate morph target
 * animations.
 *
 * \return
 * TRUE if successful or FALSE if there is an error
 *
 * \see RwEngineInit
 * \see RwEngineOpen
 * \see RwEngineStart
 * \see RpWorldPluginAttach
 */
RwBool
RpMorphPluginAttach(void)
{
    RwInt32             offset;

    RWAPIFUNCTION(RWSTRING("RpMorphPluginAttach"));

    offset =
        RwEngineRegisterPlugin(0,
                               rwID_MORPHPLUGIN, AnimOpen, AnimClose);

    if (offset < 0)
    {
        RWRETURN(FALSE);
    }

    /* Extend into geometries */

    /* MORPH ANIMATION */
    atomicAnimOffset =
        RpAtomicRegisterPlugin(sizeof(RpMorphAtomicAnim),
                               rwID_MORPHPLUGIN,
                               AtomicAnimCtorDtor,
                               AtomicAnimCtorDtor, AtomicAnimCopy);
    if (atomicAnimOffset < 0)
    {
        RWRETURN(FALSE);
    }

    /* No details to save from the atomic
     * - everything'll work once we add some time */

    /* MORPH ANIMATION */
    geomAnimOffset =
        RpGeometryRegisterPlugin(sizeof(RpMorphGeometryAnim),
                                 rwID_MORPHPLUGIN,
                                 GeomAnimCtor,
                                 GeomAnimDtor, GeomAnimCopy);
    if (geomAnimOffset < 0)
    {
        RWRETURN(FALSE);
    }

    offset =
        RpGeometryRegisterPluginStream(rwID_MORPHPLUGIN,
                                       GeomAnimReadStream,
                                       GeomAnimWriteStream,
                                       GeomAnimSizeStream);
    if (offset < 0)
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}
