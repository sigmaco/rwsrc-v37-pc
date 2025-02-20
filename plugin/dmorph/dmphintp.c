/*
 *  dmphintp.c - delta morph interpolation
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

#include "dmphanim.h"
#include "dmphgeom.h"
#include "dmorph.h"

#include "dmphintp.h"


/*===========================================================================*
 *--- Global variables ------------------------------------------------------*
 *===========================================================================*/

RwInt32 rpDMorphAtomicDataOffset   = 0;

/*===========================================================================*
 *--- Internal functions ----------------------------------------------------*
 *===========================================================================*/

/*--- DMorphAtomicLinearBuild ------------------------------------------
 */
static RpAtomic *
DMorphAtomicLinearBuild(RpAtomic *atomic)
{
    rpDMorphAtomicData   *atomicData;
    rpDMorphGeometryData *geometryData;

    RWFUNCTION(RWSTRING("DMorphAtomicLinearBuild"));
    RWASSERT(atomic);

    /* Get our atomic extension data. */
    atomicData = RPDMORPHATOMICGETDATA(atomic);

    /* Get our geometry extension data. */
    geometryData = *RPDMORPHGEOMETRYGETDATA(RpAtomicGetGeometry(atomic));

    if((NULL != geometryData) && (NULL != atomicData->values))
    {
        /* Perform delta blending */

        RpGeometry     *geometry;
        RpDMorphTarget *dMorphTarget;
        rpDMorphRLE      *rle;

        /* Base pointers. */
        RwV3d       *baseVertices;
        RwV3d       *baseNormals;
        RwRGBA      *basePreLightColors;
        RwTexCoords *baseTexCoords;

        /* Delta pointers. */
        RwV3d       *vertices;
        RwV3d       *normals;
        RwRGBA      *preLightColors;
        RwTexCoords *texCoords;

        RwReal dDelta;

        RwUInt32 i, j, k, m;
        RwUInt8  l;

        /* Lets get the base geometry to jiggle with. */
        geometry = RpAtomicGetGeometry(atomic);
        baseVertices = RPDMORPHGEOMETRYGETVERTICES(geometry);
        baseNormals = RPDMORPHGEOMETRYGETNORMALS(geometry);
        basePreLightColors = RPDMORPHGEOMETRYGETPRELIGHTCOLORS(geometry);
        baseTexCoords = RPDMORPHGEOMETRYGETTEXCOORDS(geometry);

        /* Might have to apply a delta for each DMorphTarget. */
        for( i = 0; i < geometryData->numDMorphTargets; i++ )
        {
            /* Are we presently delta morphing this DMorphTarget? */
            if( atomicData->values[i] != geometryData->currValues[i] )
            {
                /* Yes - we'll it's MORPHING TIME. */

                /* Let calculate the deltaDelta. */
                dDelta = atomicData->values[i] - geometryData->currValues[i];

                /* Lets grab the DMorpTarget and RLE for easy access. */
                dMorphTarget = &geometryData->dMorphTargets[i];
                rle = &(dMorphTarget->rle);

                vertices       = dMorphTarget->vertices;
                normals        = dMorphTarget->normals;
                preLightColors = dMorphTarget->preLightColors;
                texCoords      = dMorphTarget->texCoords;

                /* Lets lock the geometry. */
                RpGeometryLock( geometry, dMorphTarget->lockFlags );

                /*
                 * j: element of the code array.
                 * k: element of the geometry's vertices.
                 * l: loop to process all of the deltas for each code element.
                 * m: element in the target delta array.
                 */
                if (dMorphTarget->flags & rpGEOMETRYPOSITIONS)
                {
                    for (j = 0, k = 0, m = 0; j < rle->numCodeElements; j++)
                    {
                        if (rle->code[j] & 0x80)
                        {
                            /* We've got a delta packet. */
                            for (l = 0; l < (rle->code[j] & 0x7f); k++, l++, m++)
                            {
                                /* Lets apply the delta to the base. */
                                baseVertices[k].x += dDelta * vertices[m].x;
                                baseVertices[k].y += dDelta * vertices[m].y;
                                baseVertices[k].z += dDelta * vertices[m].z;
                            }
                        }
                        else
                        {
                            k += (rle->code[j] & 0x7f);
                        }
                    }
                }

                if (dMorphTarget->flags & rpGEOMETRYNORMALS)
                {
                    for (j = 0, k = 0, m = 0; j < rle->numCodeElements; j++)
                    {
                        if (rle->code[j] & 0x80)
                        {
                            /* We've got a delta packet. */
                            for (l = 0; l < (rle->code[j] & 0x7f); k++, l++, m++)
                            {
                                /* Lets apply the delta to the base. */
                                baseNormals[k].x += dDelta * normals[m].x;
                                baseNormals[k].y += dDelta * normals[m].y;
                                baseNormals[k].z += dDelta * normals[m].z;
                            }
                        }
                        else
                        {
                            k += (rle->code[j] & 0x7f);
                        }
                    }
                }

                if (dMorphTarget->flags & rpGEOMETRYPRELIT)
                {
                    for (j = 0, k = 0, m = 0; j < rle->numCodeElements; j++)
                    {
                        if (rle->code[j] & 0x80)
                        {
                            /* We've got a delta packet. */
                            for (l = 0; l < (rle->code[j] & 0x7f); k++, l++, m++)
                            {
                                /* Hopefully compiler should lose these. */
                                RwInt32  shift;
                                RwReal lastValue = geometryData->currValues[i];
                                RwReal thisValue = atomicData->values[i];

                                shift = RwInt32FromRealMacro(lastValue * preLightColors[m].alpha);
                                basePreLightColors[k].alpha -= (RwUInt8)shift;
                                shift = RwInt32FromRealMacro(lastValue * preLightColors[m].blue);
                                basePreLightColors[k].blue -= (RwUInt8)shift;
                                shift = RwInt32FromRealMacro(lastValue * preLightColors[m].green);
                                basePreLightColors[k].green -= (RwUInt8)shift;
                                shift = RwInt32FromRealMacro(lastValue * preLightColors[m].red);
                                basePreLightColors[k].red -= (RwUInt8)shift;

                                shift = RwInt32FromRealMacro(thisValue * preLightColors[m].alpha);
                                basePreLightColors[k].alpha += (RwUInt8)shift;
                                shift = RwInt32FromRealMacro(thisValue * preLightColors[m].blue);
                                basePreLightColors[k].blue += (RwUInt8)shift;
                                shift = RwInt32FromRealMacro(thisValue * preLightColors[m].green);
                                basePreLightColors[k].green += (RwUInt8)shift;
                                shift = RwInt32FromRealMacro(thisValue * preLightColors[m].red);
                                basePreLightColors[k].red += (RwUInt8)shift;
                            }
                        }
                        else
                        {
                            k += (rle->code[j] & 0x7f);
                        }
                    }
                }

                if (dMorphTarget->flags & rpGEOMETRYTEXTURED)
                {
                    for (j = 0, k = 0, m = 0; j < rle->numCodeElements; j++)
                    {
                        if (rle->code[j] & 0x80)
                        {
                            /* We've got a delta packet. */
                            for (l = 0; l < (rle->code[j] & 0x7f); k++, l++, m++)
                            {
                                /* Lets apply the delta to the base. */
                                baseTexCoords[k].u += dDelta * texCoords[m].u;
                                baseTexCoords[k].v += dDelta * texCoords[m].v;
                            }
                        }
                        else
                        {
                            k += (rle->code[j] & 0x7f);
                        }
                    }
                }

                /* Lets unlock the geometry. */
                RpGeometryUnlock( geometry );
            }

            /*
             * Make a checky copy of the present value for
             * the DMorphTargets present interpolator.
             */
            geometryData->currValues[i] = atomicData->values[i];
        }
    }

    /* Call the default renderer. */
    (atomicData->defRenderCallBack)(atomic);

    RWRETURN(atomic);
}

/*--- DMorphAtomicCreateDMorphValues ------------------------------------
 */
static RpAtomic *
DMorphAtomicCreateDMorphValues(RpAtomic *atomic)
{
    rpDMorphAtomicData     *atomicData = RPDMORPHATOMICGETDATA(atomic);
    RwUInt32                memSize;

    RWFUNCTION(RWSTRING("DMorphAtomicCreateDMorphValues"));
    RWASSERT(atomic);
    RWASSERT(atomic->geometry);

    /* Get required size of array */
    memSize = RpDMorphGeometryGetNumDMorphTargets(atomic->geometry)
                * sizeof(RwReal);

    /* Get memory */
    atomicData->values = (RwReal *) RwMalloc(memSize,
        rwID_DMORPHPLUGIN | rwMEMHINTDUR_EVENT);
    if( NULL == atomicData->values )
    {
        RWERROR((E_RW_NOMEM, memSize));
        RWRETURN((RpAtomic *)NULL);
    }

    /* Initialize */
    memset(atomicData->values, 0, memSize);

    RWRETURN(atomic);
}

/*--- DMorphAtomicCreateInterpolator ----------------------------------------
 */
static RpAtomic *
DMorphAtomicCreateInterpolator(RpAtomic *atomic)
{
    rpDMorphInterpolator   *interpolator;
    RwUInt32                numTargets, memSize;

    RWFUNCTION(RWSTRING("DMorphAtomicCreateInterpolator"));
    RWASSERT(atomic);
    RWASSERT(atomic->geometry);

    /* Get required size of memory block */
    numTargets = RpDMorphGeometryGetNumDMorphTargets(atomic->geometry);
    memSize = sizeof(rpDMorphInterpolator) +
              numTargets * (sizeof(rpDMorphFrame *) + sizeof(RwReal));

    /* Get memory */
    interpolator = (rpDMorphInterpolator *) RwMalloc(memSize,
                      rwID_DMORPHPLUGIN | rwMEMHINTDUR_EVENT);
    if (!interpolator)
    {
        RWERROR((E_RW_NOMEM, memSize));
        RWRETURN((RpAtomic *)NULL);
    }

    /* Initialize */
    memset(interpolator, 0, memSize);
    interpolator->currTimes = (RwReal *) (interpolator + 1);
    interpolator->currFrames =
        (rpDMorphFrame **) (interpolator->currTimes + numTargets);

    RPDMORPHATOMICGETDATA(atomic)->interpolator = interpolator;

    RWRETURN(atomic);
}

/*--- DMorphAtomicCtor -----------------------------------------------------
 */
static void *
DMorphAtomicCtor(void *object,
                 RwInt32 offset __RWUNUSED__,
                 RwInt32 size   __RWUNUSED__ )
{
    RpAtomic            *atomic;
    rpDMorphAtomicData  *atomicData;

    RWFUNCTION(RWSTRING("DMorphAtomicCtor"));
    RWASSERT(object);

    atomic = (RpAtomic *) object;
    atomicData = RPDMORPHATOMICGETDATA(atomic);

    /* Initialize atomic data - overloading render callback */
    atomicData->values = (RwReal *)NULL;
    atomicData->interpolator = (rpDMorphInterpolator *)NULL;
    atomicData->defRenderCallBack = (RpAtomicCallBackRender)NULL;

    RWRETURN(object);
}

/*--- DMorphAtomicDtor -----------------------------------------------------
 */
static void *
DMorphAtomicDtor(void *object,
                 RwInt32 offset __RWUNUSED__,
                 RwInt32 size   __RWUNUSED__ )
{
    RpAtomic            *atomic;
    rpDMorphAtomicData  *atomicData;

    RWFUNCTION(RWSTRING("DMorphAtomicDtor"));
    RWASSERT(object);

    atomic = (RpAtomic *) object;
    atomicData = RPDMORPHATOMICGETDATA(object);

    /* Destroy morph data */
    if (atomicData->values)
    {
        RwFree(atomicData->values);
        atomicData->values = (RwReal *)NULL;
    }

    /* Destroy anim data */
    if (atomicData->interpolator)
    {
        RwFree(atomicData->interpolator);
        atomicData->interpolator = (rpDMorphInterpolator *)NULL;
    }

    /* Reset the render callback. */
    atomicData->defRenderCallBack = (RpAtomicCallBackRender)NULL;

    RWRETURN(object);
}

/*--- DMorphAtomicCopy -----------------------------------------------------
 */
static void *
DMorphAtomicCopy(void *dstObject,
                 const void *srcObject,
                 RwInt32 offset __RWUNUSED__,
                 RwInt32 sizes  __RWUNUSED__ )
{
    const RpAtomic     *srcAtomic = (const RpAtomic *) srcObject;
    RpAtomic           *dstAtomic = (RpAtomic *) dstObject;
    const RpGeometry   *geom;

    RWFUNCTION(RWSTRING("DMorphAtomicCopy"));
    RWASSERT(dstObject);
    RWASSERT(srcObject);

    /* Check if the atomic has a geometry */
    geom = RpAtomicGetGeometry(srcAtomic);
    if (geom)
    {
        RwUInt32    numTargets;

        /* Check if there is any delta morph data */
        numTargets = RpDMorphGeometryGetNumDMorphTargets(geom);
        if (numTargets > 0)
        {
            const rpDMorphAtomicData   *srcAtomicData;
            rpDMorphAtomicData         *dstAtomicData;

            srcAtomicData = RPDMORPHATOMICGETCONSTDATA(srcAtomic);
            dstAtomicData = RPDMORPHATOMICGETDATA(dstAtomic);

            /* Copy any source morph values or otherwise initialize to zero */
            if (DMorphAtomicCreateDMorphValues(dstAtomic))
            {
                if (srcAtomicData->values)
                {
                    memcpy( dstAtomicData->values, srcAtomicData->values,
                        numTargets * sizeof(RwReal) );
                }
                else
                {
                    memset( dstAtomicData->values, 0,
                        numTargets * sizeof(RwReal) );
                }
            }

            /* No animation */
            dstAtomicData->interpolator = (rpDMorphInterpolator *)NULL;

            /* Set up render callback etc */
            dstAtomicData->defRenderCallBack =
                srcAtomicData->defRenderCallBack;
        }
    }

    RWRETURN(dstObject);
}

/*--- DMorphAtomicAlways ---------------------------------------------------
 */
static RwBool
DMorphAtomicAlways( void *object,
                    RwInt32 offset __RWUNUSED__,
                    RwInt32 size   __RWUNUSED__ )
{
    RpAtomic *atomic;
    RpGeometry *geometry;

    RwUInt32 numDMorphTargets;
    RwBool result = TRUE;

    RWFUNCTION(RWSTRING("DMorphAtomicAlways"));
    RWASSERT(NULL != object);

    atomic = (RpAtomic *)object;
    RWASSERT(NULL != atomic);
    geometry = RpAtomicGetGeometry(atomic);
    RWASSERT(NULL != geometry);

    /* Do we have any delta morph targets? */
    numDMorphTargets = RpDMorphGeometryGetNumDMorphTargets(geometry);
    if(numDMorphTargets > 0)
    {
        atomic = RpDMorphAtomicInitialize(atomic);
        RWASSERT(NULL != atomic);
    }

    RWRETURN(result);
}

/*--- DMorphAtomicRights ---------------------------------------------------
 */
static RwBool
DMorphAtomicRights(void *object,
                   RwInt32 offset __RWUNUSED__,
                   RwInt32 size __RWUNUSED__,
                   RwUInt32 extraData )
{
    RpAtomic        *atomic;
    RpDMorphType    type;

    RWFUNCTION(RWSTRING("DMorphAtomicRights"));
    RWASSERT(NULL != object);

    atomic = (RpAtomic *)object;
    RWASSERT(NULL != atomic);

    type = (RpDMorphType)extraData;

    RpDMorphAtomicSetType(atomic, rpDMORPHTYPEGENERIC);

    RWRETURN(TRUE);
}

/*===========================================================================*
 *--- Plugin internal functions ---------------------------------------------*
 *===========================================================================*/

/*--- _rpDMorphAtomicPluginAttach -------------------------------------------
 */
RwBool
_rpDMorphAtomicPluginAttach(void)
{
    RwInt32 success;

    RWFUNCTION(RWSTRING("_rpDMorphAtomicPluginAttach"));

    /* Register the plugin with the atomic. */
    rpDMorphAtomicDataOffset =
        RpAtomicRegisterPlugin( sizeof(rpDMorphAtomicData),
                                rwID_DMORPHPLUGIN,
                                DMorphAtomicCtor,
                                DMorphAtomicDtor,
                                DMorphAtomicCopy );

    if( 0 > rpDMorphAtomicDataOffset )
    {
        RWRETURN(FALSE);
    }

    /* Attach an always callback streaming function. */
    success = RpAtomicSetStreamAlwaysCallBack( rwID_DMORPHPLUGIN,
                                               DMorphAtomicAlways );
    RWASSERT(0 < success);

    /* Attach a rights callback streaming function. */
    success = RpAtomicSetStreamRightsCallBack( rwID_DMORPHPLUGIN,
                                               DMorphAtomicRights );
    RWASSERT(0 < success);

    RWRETURN(TRUE);
}

/*===========================================================================*
 *--- API functions ---------------------------------------------------------*
 *===========================================================================*/

/*--- DMorphAtomic functions ------------------------------------------------*/

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAtomicInitialize is used to setup the
 * \ref RpAtomic to render a delta morphed \ref RpGeometry.
 * The atomics' render callback is overloaded and chained
 * after the delta morphing render callback.
 *
 * RpDMorphAtomicInitialize should only be called once only per atomic, or after
 * atomic's render callback is changed.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param atomic Pointer to the atomic.
 *
 * \return Returns NULL if there is an error, or otherwise
 *         a pointer to the atomic.
 */
RpAtomic *
RpDMorphAtomicInitialize(RpAtomic *atomic)
{
    rpDMorphAtomicData *atomicData;

    RWAPIFUNCTION(RWSTRING("RpDMorphAtomicInitialize"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(atomic);

    atomicData = RPDMORPHATOMICGETDATA(atomic);

    RWASSERTM(atomic->renderCallBack != DMorphAtomicLinearBuild,
             (RWSTRING("RpAtomic already being processed by RpDMorph. ")
              RWSTRING("RpDMorphAtomicInitialize should be called once only ")
              RWSTRING("per atomic, or after atomic\'s render callback is changed")));

    /* Setup the atomic render callback. */
    atomicData->defRenderCallBack = atomic->renderCallBack;
    atomic->renderCallBack = DMorphAtomicLinearBuild;

    RWRETURN(atomic);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAtomicSetType
 *
 * Sets up the delta morph \ref RpAtomic with the correct rendering pipeline.
 *
 * \param atomic Pointer to the atomic.
 * \param type   Atomic rendering type.
 *
 * \return A pointer to the \ref RpAtomic if successful, or NULL
 * otherwise.
 *
 * \see RpDMorphAtomicGetType
 */
RpAtomic *
RpDMorphAtomicSetType(RpAtomic *atomic,
                      RpDMorphType type
#if !defined(PLATFORM_PIPELINES)
                                        __RWUNUSED__
#endif /* !defined(PLATFORM_PIPELINES) */
                                                     )
{
    RWAPIFUNCTION(RWSTRING("RpDMorphAtomicSetType"));
    RWASSERT(0 < rpDMorphModule.numInstances);
    RWASSERT(NULL != atomic);

#if defined(PLATFORM_PIPELINES)
    {
        rpDMorphAtomicData  *atomicData;

        atomicData = RPDMORPHATOMICGETDATA(atomic);
        RWASSERT(NULL != atomicData);

        if (atomicData->defRenderCallBack != NULL)
        {
            atomic->renderCallBack = atomicData->defRenderCallBack;
            atomicData->defRenderCallBack = NULL;
        }

        atomic = _rpDMorphPipelinesAttach(atomic, type);
    }
#endif /* defined(PLATFORM_PIPELINES) */

    RWRETURN(atomic);
}

/**
 * \ingroup rpskin
 * \ref RpDMorphAtomicGetType
 *
 * Gets the delta morph type attached to the atomic.
 *
 * \param atomic Pointer to the atomic.
 *
 * \return The \ref RpDMorphType of the pipeline attached to the atomic,
 * or \ref rpNADMORPHTYPE otherwise.
 *
 * \see RpDMorphAtomicSetType
 */
RpDMorphType
RpDMorphAtomicGetType(RpAtomic *atomic)
{
    RxPipeline      *pipeline;
    RpDMorphType    type;

    RWAPIFUNCTION(RWSTRING("RpDMorphAtomicGetType"));
    RWASSERT(0 < rpDMorphModule.numInstances);
    RWASSERT(NULL != atomic);

    RpAtomicGetPipeline(atomic, &pipeline);
    RWASSERT(NULL != pipeline);

#if defined(PLATFORM_PIPELINES)
    type = (pipeline->pluginId == rwID_DMORPHPLUGIN)
         ? (RpDMorphType)(pipeline->pluginData)
         : rpNADMORPHTYPE;
#else /* defined(PLATFORM_PIPELINES) */
    type = rpNADMORPHTYPE;
#endif /* defined(PLATFORM_PIPELINES) */

    RWRETURN(type);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAtomicGetDMorphValues is used to
 * retrieve the array of values from a delta morph atomic
 * specifying the amounts of each delta morph target in the
 * current morph state.
 *
 * The values may be altered to change the morph state. This
 * may be done in combination with a standard delta morph animation
 * (\ref RpDMorphAnimation) or as part of an alternative animation system.
 *
 * If the array does not currently exist then it will be
 * created based on the number of delta morph targets in the
 * geometry attached to the atomic.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param atomic  Pointer to the atomic.
 *
 * \return Returns NULL if there is an error, or otherwise
 *         a pointer to the array of values.
 *
 * \see RpDMorphAtomicInitialize
 * \see RpDMorphAtomicGetAnimation
 * \see RpDMorphAtomicGetAnimTime
 * \see RpDMorphAtomicGetAnimLoopCallBack
 * \see RpDMorphAtomicGetAnimFrame
 * \see RpDMorphAtomicGetAnimFrameTime
 */
RwReal *
RpDMorphAtomicGetDMorphValues(RpAtomic *atomic)
{
    rpDMorphAtomicData  *atomicData;

    RWAPIFUNCTION(RWSTRING("RpDMorphAtomicGetDMorphValues"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(atomic);

    atomicData = RPDMORPHATOMICGETDATA(atomic);

    if( NULL == atomicData->values )
    {
        DMorphAtomicCreateDMorphValues(atomic);
    }

    RWRETURN(atomicData->values);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAtomicSetAnimation is used to set the current animation
 * for a delta morph atomic. This is only applicable where the atomic
 * has a delta morph target geometry attached.
 *
 * The atomic has an animation interpolator which is set to the initial
 * frames in the delta morph animation for each morph target, and
 * interpolation times are set to zero. The morph values are set to
 * those at the start of the animation.
 *
 * It is possible to override a delta morph animation on an atomic, or
 * control the morphing without an animation, by directly accessing the
 * morph values (see \ref RpDMorphAtomicGetDMorphValues).
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param atomic        Pointer to the atomic.
 * \param animation     Pointer to the delta morph animation.
 *
 * \return A pointer to the atomic or NULL if there is an error.
 *
 * \see RpDMorphAtomicInitialize
 * \see RpDMorphAnimationCreate
 * \see RpDMorphAnimationRead
 * \see RpDMorphAtomicAddTime
 * \see RpDMorphAtomicSetAnimLoopCallBack
 * \see RpDMorphAtomicGetAnimLoopCallBack
 */
RpAtomic *
RpDMorphAtomicSetAnimation(RpAtomic *atomic, RpDMorphAnimation *animation)
{
    rpDMorphAtomicData     *atomicData = RPDMORPHATOMICGETDATA(atomic);

    RWAPIFUNCTION(RWSTRING("RpDMorphAtomicSetAnimation"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(atomic);
    RWASSERT(atomic->geometry);

    /* Are we unsetting an animation? */
    if (animation == NULL)
    {
        /* Detach interpolator */
        if (atomicData->interpolator)
        {
            RwUInt32    i;

            for (i = 0;
                 i < RpDMorphGeometryGetNumDMorphTargets(atomic->geometry);
                 i++)
            {
                atomicData->interpolator->currFrames[i] =
                    (rpDMorphFrame *)NULL;
            }

            atomicData->interpolator->animation = (RpDMorphAnimation *)NULL;
        }
    }
    else
    {
        RwUInt32                i;
        rpDMorphInterpolator    *interpolator;
        RwReal                  *values;

        /* Get interpolator and morph values */
        if (    !atomicData->interpolator
            &&  !DMorphAtomicCreateInterpolator(atomic))
        {
            RWRETURN((RpAtomic *)NULL);
        }

        if (    !atomicData->values
            &&  !DMorphAtomicCreateDMorphValues(atomic))
        {
            RWRETURN((RpAtomic *)NULL);
        }

        interpolator = atomicData->interpolator;
        values = atomicData->values;

        /* Initialize animation */
        interpolator->animation = animation;
        interpolator->animTime = 0.0f;

        for (i=0; i<RpDMorphGeometryGetNumDMorphTargets(atomic->geometry); i++)
        {
            /* Is there a sequence for this morph target? */
            if (    i < animation->numDMorphTargets
                &&  animation->frameSequences[i].frames )
            {
                interpolator->currFrames[i] =
                    animation->frameSequences[i].frames;
                interpolator->currTimes[i] = 0.0f;
                values[i] = animation->frameSequences[i].frames[0].startValue;
            }
            else
            {
                interpolator->currFrames[i] = (rpDMorphFrame *)NULL;
                interpolator->currTimes[i] = 0.0f;
                values[i] = 0.0f;
            }
        }
    }

    RWRETURN(atomic);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAtomicGetAnimation is used to retrieve the current animation
 * for a delta morph atomic. This would have previously been set using
 * \ref RpDMorphAtomicSetAnimation.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param atomic        Pointer to the atomic.
 *
 * \return A pointer to the animation, or NULL if one is not set.
 *
 * \see RpDMorphAtomicSetAnimation
 * \see RpDMorphAtomicAddTime
 */
RpDMorphAnimation *
RpDMorphAtomicGetAnimation(const RpAtomic *atomic)
{
    const rpDMorphAtomicData *atomicData = RPDMORPHATOMICGETCONSTDATA(atomic);

    RWAPIFUNCTION(RWSTRING("RpDMorphAtomicGetAnimation"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(atomic);

    if (!atomicData->interpolator)
    {
        RWRETURN((RpDMorphAnimation *)NULL);
    }

    RWRETURN(atomicData->interpolator->animation);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAtomicAddTime is used to advance the animation of
 * a delta morph atomic by the given amount of time. An animation must
 * have already been attached with \ref RpDMorphAtomicSetAnimation.
 * It is not possible to play an animation backwards, and adding negative
 * time will produce invalid results.
 *
 * Note that if the animation loops, the time returned by this function
 * does not reset to zero. It is the total time added to the animation
 * including loops.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param atomic       Pointer to the atomic.
 * \param deltaTime    The amount of time to add.
 *
 * \return Returns NULL if there is an error, or otherwise
 * a pointer to the atomic.
 *
 * \see RpDMorphAtomicSetAnimFrameTime
 * \see RpDMorphAtomicGetAnimFrameTime
 */
RpAtomic *
RpDMorphAtomicAddTime(RpAtomic *atomic, RwReal deltaTime)
{
    rpDMorphAtomicData         *atomicData = RPDMORPHATOMICGETDATA(atomic);
    rpDMorphInterpolator       *interpolator;
    rpDMorphFrame             **seqFrame;
    RwReal                     *frameTime;
    RwBool                      looped = FALSE;
    RwUInt32                    i;

    RWAPIFUNCTION(RWSTRING("RpDMorphAtomicAddTime"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(atomic);
    RWASSERT(atomic->geometry);
    RWASSERT(deltaTime >= 0.0f);

    /* Get the interpolator */
    interpolator = atomicData->interpolator;
    RWASSERT(interpolator);
    RWASSERT(interpolator->animation);
    RWASSERT(interpolator->currFrames);
    RWASSERT(interpolator->currTimes);

    /* Update record of animation time */
    interpolator->animTime += deltaTime;

    /* Update interpolation */
    seqFrame = interpolator->currFrames;
    frameTime  = interpolator->currTimes;

    for (i=0; i<RpDMorphGeometryGetNumDMorphTargets(atomic->geometry); i++)
    {
        /* Is there a frame sequence connected for this target? */
        if (*seqFrame)
        {
            /* Update the time */
            *frameTime += deltaTime;

            while(*frameTime > (*seqFrame)->duration)
            {
                *frameTime -= (*seqFrame)->duration;

                /* Check for the null frame */
                if ((*seqFrame)->nextFrame == rpDMORPHNULLFRAME)
                {
                    *seqFrame = (rpDMorphFrame *)NULL;
                    break;
                }
                else
                {
                    rpDMorphFrame   *nextFrame =
                        interpolator->animation->frameSequences[i].frames
                                    + (*seqFrame)->nextFrame;

                    /* Record whether animation has looped */
                    looped |= (nextFrame < *seqFrame);

                    /* Goto next */
                    *seqFrame = nextFrame;
                }
            }

            /* Update morph values */
            if (*seqFrame != NULL)
            {
                atomicData->values[i] = (*seqFrame)->startValue +
                    (*frameTime) *
                    (*seqFrame)->recipDuration *
                    ((*seqFrame)->endValue - (*seqFrame)->startValue);
            }
        }

        seqFrame++;
        frameTime++;
    }

    /* Do we need to call the loop callback? */
    if (looped && interpolator->loopCallBack)
    {
        interpolator->loopCallBack(atomic, interpolator->loopCallBackData);
    }

    RWRETURN(atomic);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAtomicGetAnimTime is used to obtain the total amount
 * of time added to the animation of a delta morph atomic.
 *
 * It is not possible to set the absolute animation time directly. However,
 * this may be achieved by re-setting the animation using
 * \ref RpDMorphAtomicSetAnimation and then adding the appropriate time,
 * with \ref RpDMorphAtomicAddTime.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param atomic    Pointer to the delta morph atomic.
 *
 * \return Returns the animation time.
 *
 * \see RpDMorphAtomicAddTime
 */
RwReal
RpDMorphAtomicGetAnimTime(const RpAtomic *atomic)
{
    const rpDMorphAtomicData  *atomicData = RPDMORPHATOMICGETCONSTDATA(atomic);

    RWAPIFUNCTION(RWSTRING("RpDMorphAtomicGetAnimTime"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(atomic);
    RWASSERT(atomicData->interpolator);

    RWRETURN(atomicData->interpolator->animTime);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAtomicSetAnimLoopCallBack is used to set an atomic callback
 * that will be called whenever a delta morph animation loops during
 * \ref RpDMorphAtomicAddTime. A pointer to some data may also be specified
 * for passing to the callback.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param atomic    Pointer to the atomic.
 * \param callBack  Function pointer to the callback.
 * \param data      Pointer to user data to pass to the callback.
 *
 * \return Returns NULL if there is an error, or otherwise
 * a pointer to the atomic.
 *
 * \see RpDMorphAtomicGetAnimLoopCallBack
 * \see RpDMorphAtomicAddTime
 */
RpAtomic *
RpDMorphAtomicSetAnimLoopCallBack(RpAtomic *atomic,
                                  RpAtomicCallBack callBack,
                                  void *data)
{
    rpDMorphAtomicData *atomicData = RPDMORPHATOMICGETDATA(atomic);

    RWAPIFUNCTION(RWSTRING("RpDMorphAtomicSetAnimLoopCallBack"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(atomic);
    RWASSERT(atomicData->interpolator);

    /* Set the atomics callback. */
    atomicData->interpolator->loopCallBack = callBack;
    atomicData->interpolator->loopCallBackData = data;

    RWRETURN(atomic);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAtomicGetAnimLoopCallBack is used to retrieve the
 * the function that will be called whenever a delta morph animation
 * loops. The callback data pointer may also be returned if required.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param atomic        Pointer to the atomic.
 * \param callBackData  A pointer to the loop callback data is passed
 *                      back via this pointer if not NULL.
 *
 * \return Returns NULL if there is an error, or otherwise
 * a pointer to the callback function.
 *
 * \see RpDMorphAtomicSetAnimLoopCallBack
 */
RpAtomicCallBack
RpDMorphAtomicGetAnimLoopCallBack(const RpAtomic *atomic, void **callBackData)
{
    const rpDMorphAtomicData *atomicData = RPDMORPHATOMICGETCONSTDATA(atomic);

    RWAPIFUNCTION(RWSTRING("RpDMorphAtomicGetAnimLoopCallBack"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(atomic);
    RWASSERT(atomicData->interpolator);

    /* Pass back callback data if required */
    if (callBackData)
    {
        *callBackData = atomicData->interpolator->loopCallBackData;
    }

    /* Return the callback function. */
    RWRETURN(atomicData->interpolator->loopCallBack);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAtomicSetAnimFrame will set the specified delta morph
 * target to the start of a particular frame in an animation.
 *
 * A value of rpDMORPHNULLFRAME may be specified for the frame index. This
 * effectively disconnects a particular morph target from the animation.
 * During \ref RpDMorphAtomicAddTime the value for the delta morph target
 * will not be updated. However, it may be modified directly for procedural
 * animation (see \ref RpDMorphAtomicGetDMorphValues).
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param atomic                Pointer to the atomic.
 * \param dMorphTargetIndex     Index of the delta morph target.
 * \param frameIndex            Index of the frame in the sequence.
 *
 * \return Returns NULL if there is an error, or otherwise
 * a pointer to the atomic.
 *
 * \see RpDMorphAtomicGetAnimFrame
 * \see RpDMorphAtomicSetAnimFrameTime
 */
RpAtomic *
RpDMorphAtomicSetAnimFrame(RpAtomic *atomic,
                           RwUInt32 dMorphTargetIndex,
                           RwUInt32 frameIndex )
{
    rpDMorphAtomicData      *atomicData = RPDMORPHATOMICGETDATA(atomic);
    rpDMorphInterpolator    *interpolator;

    RWAPIFUNCTION(RWSTRING("RpDMorphAtomicSetAnimFrame"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(atomic);
    RWASSERT(atomic->geometry);
    RWASSERT(dMorphTargetIndex <
        RpDMorphGeometryGetNumDMorphTargets(atomic->geometry));

    /* Get interpolator */
    interpolator = atomicData->interpolator;
    RWASSERT(interpolator);
    RWASSERT(interpolator->animation);
    RWASSERT(interpolator->currFrames);
    RWASSERT(interpolator->currTimes);
    RWASSERT(dMorphTargetIndex < interpolator->animation->numDMorphTargets);
    RWASSERT((frameIndex == rpDMORPHNULLFRAME) || frameIndex <
        interpolator->animation->frameSequences[dMorphTargetIndex].numFrames);

    /* Set the frame */
    if( frameIndex == rpDMORPHNULLFRAME )
    {
        interpolator->currFrames[dMorphTargetIndex] = (rpDMorphFrame *)NULL;
    }
    else
    {
        interpolator->currFrames[dMorphTargetIndex] =
            &(interpolator->animation->
                frameSequences[dMorphTargetIndex].frames[frameIndex]);

        /* Initialize the time and value */
        interpolator->currTimes[dMorphTargetIndex] = (RwReal) 0.0f;
        atomicData->values[dMorphTargetIndex] =
            interpolator->currFrames[dMorphTargetIndex]->startValue;
    }

    RWRETURN(atomic);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAtomicGetAnimFrame is used to obtain
 * the index of the current frame in an animation to which
 * a specific delta morph target is attached.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param atomic             Pointer to the atomic.
 * \param dMorphTargetIndex  Index of the delta morph target.
 *
 * \return Returns the index of the frame, or rpDMORPHNULLFRAME if
 *         if the morph target is not connected to the animation.
 *
 * \see RpDMorphAtomicSetAnimFrame
 */
RwUInt32
RpDMorphAtomicGetAnimFrame(const RpAtomic *atomic,
                           RwUInt32 dMorphTargetIndex )
{
    const rpDMorphAtomicData *atomicData = RPDMORPHATOMICGETCONSTDATA(atomic);
    const rpDMorphInterpolator *interpolator;
    RwUInt32                    frameIndex;

    RWAPIFUNCTION(RWSTRING("RpDMorphAtomicGetAnimFrame"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(atomic);
    RWASSERT(atomic->geometry && dMorphTargetIndex <
                RpDMorphGeometryGetNumDMorphTargets(atomic->geometry));

    /* Get interpolator */
    interpolator = atomicData->interpolator;
    RWASSERT(interpolator);
    RWASSERT(interpolator->animation);
    RWASSERT(interpolator->currTimes);
    RWASSERT(dMorphTargetIndex < interpolator->animation->numDMorphTargets);

    /* Do we have a null frame? */
    if( NULL == interpolator->currFrames[dMorphTargetIndex] )
    {
        frameIndex = rpDMORPHNULLFRAME;
    }
    else
    {
        frameIndex = interpolator->currFrames[dMorphTargetIndex] -
            interpolator->animation->frameSequences[dMorphTargetIndex].frames;
    }

    RWRETURN(frameIndex);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAtomicSetAnimFrameTime is used to
 * set the interpolation time within the current animation frame
 * for a particular delta morph target. A time of zero corresponds to the
 * start of the frame, and the time should not bet set greater than
 * the duration of the frame. The morph target value is updated
 * according to the linear interpolation at the given time.
 *
 * To set the interpolator to a different frame use
 * \ref RpDMorphAtomicSetAnimFrame.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param atomic                Pointer to the atomic.
 * \param dMorphTargetIndex     Index of the delta morph target.
 * \param time                  The value for the interpolation time.
 *
 * \return Returns NULL if there is an error, or otherwise
 * a pointer to the atomic.
 *
 * \see RpDMorphAtomicAddTime
 * \see RpDMorphAtomicGetAnimFrameTime
 * \see RpDMorphAtomicSetAnimFrame
 * \see RpDMorphAtomicGetAnimFrame
 */
RpAtomic *
RpDMorphAtomicSetAnimFrameTime(RpAtomic *atomic,
                               RwUInt32 dMorphTargetIndex,
                               RwReal   time )
{
    rpDMorphAtomicData   *atomicData = RPDMORPHATOMICGETDATA(atomic);
    rpDMorphInterpolator *interpolator;
    rpDMorphFrame        *frame;

    RWAPIFUNCTION(RWSTRING("RpDMorphAtomicSetAnimFrameTime"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(atomic);
    RWASSERT(atomic->geometry && dMorphTargetIndex <
                RpDMorphGeometryGetNumDMorphTargets(atomic->geometry));
    RWASSERT(0 <= time);

    /* Get interpolator */
    interpolator = atomicData->interpolator;
    RWASSERT(interpolator);
    RWASSERT(interpolator->animation);
    RWASSERT(interpolator->currFrames);
    RWASSERT(interpolator->currTimes);

    /* Get current frame for this morph target */
    frame = interpolator->currFrames[dMorphTargetIndex];
    if (!frame)
    {
        RWRETURN((RpAtomic *)NULL);
    }

    RWASSERT(time <= frame->duration);

    /* Set the DMorphTargets positions, depending on time */
    interpolator->currTimes[dMorphTargetIndex] = time;
    atomicData->values[dMorphTargetIndex] =
        frame->startValue + time * frame->recipDuration *
            (frame->endValue - frame->startValue);

    RWRETURN(atomic);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphAtomicGetAnimFrameTime returns
 * the interpolation time within the current animation frame
 * of the specified delta morph target. The time is zero at the
 * start of the frame and equal to the frame duration at the end.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param atomic                Pointer to the atomic.
 * \param dMorphTargetIndex     Index of the delta morph target.
 *
 * \return Returns the interpolation time.
 *
 * \see RpDMorphAtomicSetAnimation
 * \see RpDMorphAtomicAddTime
 * \see RpDMorphAtomicSetAnimFrame
 * \see RpDMorphAtomicGetAnimFrame
 * \see RpDMorphAtomicSetAnimFrameTime
 */
RwReal
RpDMorphAtomicGetAnimFrameTime(const RpAtomic *atomic,
                               RwUInt32 dMorphTargetIndex )
{
    const rpDMorphAtomicData *atomicData = RPDMORPHATOMICGETCONSTDATA(atomic);
    rpDMorphInterpolator     *interpolator;

    RWAPIFUNCTION(RWSTRING("RpDMorphAtomicGetAnimFrameTime"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(atomic);

    /* Get interpolator */
    interpolator = atomicData->interpolator;
    RWASSERT(interpolator);
    RWASSERT(interpolator->animation);
    RWASSERT(interpolator->currTimes);
    RWASSERT(dMorphTargetIndex < interpolator->animation->numDMorphTargets);

    /* Return the current time for the specified morph target */
    RWRETURN(interpolator->currTimes[dMorphTargetIndex]);
}
