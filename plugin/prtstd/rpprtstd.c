/**********************************************************************
 *
 * File :     rpprtstd.c
 *
 * Abstract : Basic Particle Engine in RenderWare
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
 * Copyright (c) 1998 Criterion Software Ltd.
 * All Rights Reserved.
 *
 * RenderWare is a trademark of Canon Inc.
 *
 ************************************************************************/

#include <rwcore.h>
#include <rpworld.h>

#include <rpplugin.h>
#include <rpdbgerr.h>

#include "rpprtstd.h"
#include "rpplugin.h"

#include "prtstdvars.h"

#include "eclass.h"
#include "emitter.h"
#include "pclass.h"
#include "prop.h"
#include "standard.h"

#if (defined(SKY2_DRVMODEL_H))

#include "stdsky2.h"

#endif /* (defined(SKY2_DRVMODEL_H) */

#if (defined(RWDEBUG))
long                rpPrtStandardStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

/**
 * \ingroup rpprtstd
 * \page rpprtstdoverview RpPrtStd Plugin Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rpptank.h, rpprtstd.h,
 * \li \b Libraries: rwcore, rpworld, rpptank, rpprtstd
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach,
 *     \ref RpPTankPluginAttach, \ref RpPrtStdPluginAttach
 *
 * \note 3ds max and Maya uses the RpPrtAdv plugin for exporting
 *       particles. This plugin therefore needs to be attached
 *       in order to use any particle systems exported by these
 *       packages. RpPrtAdv is part of the FX Pack, a chargeable optional
 *       extra. See \ref rpprtadv for more details.
 *
 * \subsection prtstdoverview Overview
 *
 * PrtStd is a basic particle animation and management system. It allows the user
 * to design a set of emitters and particles and animate them via a series of
 * callback functions. It can use the \ref rpptank plugin for the rendering
 * of the particles emitters.
 *
 * The plugin defines two objects, an emitter and a particle. Both objects are
 * customizables and can contain user defined data. The user can provide a
 * set of callbacks for handling both objects for different events.
 *
 * An emitter is an object that emits particles. Once emitted, particles can be
 * independently animated and is always in world space. They are still attached
 * to their parent emitters for management purposes. So an emitter cannot be removed
 * without removing its child particles.
 *
 * Data for particles are split between the RpPrtStd plugin and the \ref rpptank
 * plugin. Typically rendering data are stored with the \ref rpptank, such as positions.
 * Animation data, such as velocity, are stored with the RpPrtStd plugin. Emitter
 * data are always stored in the RpPrtStd plugin.
 *
 * The emitters and particles are not fixed data structures. They contain a number of
 * properties and some of these have a bias value to vary the base value.
 * The bias value modifies the base values as follows:
 *
 * \verbatim
   specific value = base value + (bias * R)
   where R is a random number between + and -1
   \endverbatim
 *
 * See the Standard Particles user guide chapter for more information on the arrangement
 * of properties for emitters and particles.
 *
 * The operation of the particle system is generally a cycle of
 * \li \b Update: Particles are updated. This involves emitting new particles, removing dead
 * particles and update exisiting particles.
 * \li \b Rendering: Particles are rendered.
 *
 * \subsection prstdupdate Update
 * Emitters and particles are updated with the function \ref RpPrtStdAtomicUpdate. This
 * function updates the emitter and its particles. New particles can be created and dead
 * particles are removed during this stage.
 *
 * The sequence of events during an update is as follows, calling the appropiate callback
 * \verbatim

        begin emitter update
        for each particle batch
            removal empty particle batches
        for each particle batch
            update existing particles
        emit new particles
            create new particle batch
        end emitter update

   \endverbatim
 *
 * \warning
 * The function \ref RpPrtStdAtomicUpdate cannot be called between \ref RwCameraBeginUpdate
 * and \ref RwCameraEndUpdate.
 *
 * \subsection prtstdrender Render
 * Emitter and particles are rendered with the function \ref RpAtomicRender. This
 * function calls the necessary callback to render the objects.
 *
 * The sequence of events during a render is as follows, calling the appropiate callback
 * \verbatim

        begin emitter render
        for each particle batch
            render the particles
        end emitter render

   \endverbatim
 */

/************************************************************************
 *
 *
 ************************************************************************/

RpPrtStdGlobalVars rpPrtStdGlobals;

/************************************************************************
 *
 *
 ************************************************************************/

static void *
PrtStdEngineOpen( void * ptr,
                     RwInt32 offset __RWUNUSED__,
                     RwInt32 size __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("PrtStdEngineOpen"));


    RWRETURN(ptr);
}

static void *
PrtStdEngineClose( void * ptr,
                     RwInt32 offset __RWUNUSED__,
                     RwInt32 size __RWUNUSED__)
{
    RpPrtStdEmitterClass        *eClass, *activeEClass;
    RpPrtStdParticleClass       *pClass, *activePClass;
    RpPrtStdPropertyTable       *propTab, *activeTab;

    RWFUNCTION(RWSTRING("PrtStdEngineClose"));

    /* Time for a bit of cleanup */
    activeEClass = NULL;
    activePClass = NULL;
    activeTab = NULL;


    if (rpPrtStdGlobals.prtClass != NULL)
    {
        pClass = rpPrtStdGlobals.prtClass->next;

        while (pClass != activePClass)
        {
            activePClass = pClass;
            pClass = pClass->next;
            RpPrtStdPClassDestroy(activePClass);
        }

        rpPrtStdGlobals.prtClass = NULL;

    }

    if (rpPrtStdGlobals.emtClass != NULL)
    {
        eClass = rpPrtStdGlobals.emtClass->next;

        while (eClass != activeEClass)
        {
            activeEClass = eClass;
            eClass = eClass->next;
            RpPrtStdEClassDestroy(activeEClass);
        }

        rpPrtStdGlobals.emtClass = NULL;

    }

    /* Destroy the property tables last */
    if (rpPrtStdGlobals.propTab != NULL)
    {
        propTab = rpPrtStdGlobals.propTab->next;

        while (propTab != activeTab)
        {
            activeTab = propTab;
            propTab = propTab->next;
            RpPrtStdPropTabDestroy(activeTab);
        }

        rpPrtStdGlobals.propTab = NULL;
    }

    /* Destroy the scratch mem. */
    if (rpPrtStdGlobals.propTabScratchMem)
    {
        RwFree(rpPrtStdGlobals.propTabScratchMem);
    }

    rpPrtStdGlobals.propTabScratchMem = NULL;
    rpPrtStdGlobals.propTabScratchMemSize = 0;

    if (rpPrtStdGlobals.propIDScratchMem)
    {
        RwFree(rpPrtStdGlobals.propIDScratchMem);
    }

    rpPrtStdGlobals.propIDScratchMem = NULL;
    rpPrtStdGlobals.propIDScratchMemSize = 0;

    RWRETURN(ptr);
}

/************************************************************************/

static RpAtomic *
_rpPrtStdAtomicRender(RpAtomic *atomic)
{
    RpAtomic                    *result;
    RpPrtStdEmitter             *emt;

    RWFUNCTION(RWSTRING("_rpPrtStdAtomicRender"));

    RWASSERT(atomic);

    emt = (RpPrtStdEmitter *)
        RPPRTSTDATOMICPROP(atomic, rpPrtStdGlobals.atmOffset);

    RWASSERT(emt);

    result =
        RpPrtStdAtomicForAllEmitter(atomic, _rpPrtStdEmitterRender, NULL);

    RWRETURN(result);
}

/************************************************************************/


static void *
PrtStdAtomicCtor(void *obj, RwInt32 offset, RwInt32 size __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("PrtStdAtomicCtor"));

    RPPRTSTDATOMICPROP(obj, offset) = NULL;

    RWRETURN((obj));
}

static void *
PrtStdAtomicDtor(void *obj, RwInt32 offset, RwInt32 size __RWUNUSED__)
{
    RpPrtStdEmitter         *emt, *emtHead, *emtNext;

    RWFUNCTION(RWSTRING("PrtStdAtomicDtor"));

    emt = (RpPrtStdEmitter *)
        RPPRTSTDATOMICPROP(obj, offset);

    if (emt)
    {
        emtHead = emt->next;
        emt = emt->next;

        do
        {
            RWASSERT(emt);

            emtNext = emt->next;

            RpPrtStdEmitterDestroy(emt);

            emt = emtNext;
        }
        while (emt != emtHead);
    }

    RPPRTSTDATOMICPROP(obj, offset) = NULL;

    RWRETURN((obj));
}

static void *
PrtStdAtomicCopy(void *dstObj,
                    const void *srcObj,
                    RwInt32 offset, RwInt32 size __RWUNUSED__)
{
    RpPrtStdEmitter         *emt, *emtHead, *newEmt, *newEmtHead;

    RWFUNCTION(RWSTRING("PrtStdAtomicCopy"));

    emt = (RpPrtStdEmitter *)
        RPPRTSTDATOMICPROP(srcObj, offset);

    newEmtHead = NULL;

    if (emt)
    {
        emtHead = emt->next;
        emt = emt->next;

        do
        {
            RWASSERT(emt);


            newEmt = RpPrtStdEmitterClone(emt);

            if (newEmt)
            {
                newEmt->atomic = (RpAtomic *) dstObj;

                newEmtHead = RpPrtStdEmitterAddEmitter(newEmtHead, newEmt);
            }

            emt = emt->next;
        }
        while (emt != emtHead);

    }

    RPPRTSTDATOMICPROP(dstObj, offset) = newEmtHead;

    RWRETURN((dstObj));
}

/************************************************************************
 */

static RpPrtStdEmitter *
_rpPrtStdAtomicCountEmitters(RpAtomic * atomic __RWUNUSED__,
                             RpPrtStdEmitter *emt, void *data)
{
    RwInt32                 *i;

    RWFUNCTION(RWSTRING("_rpPrtStdAtomicCountEmitters"));

    RWASSERT(emt);

    /* Increment the count. */
    i = (RwInt32 *) data;

    (*i)++;

    RWRETURN(emt);
}

static RwInt32
PrtStdAtomicGetChunkSize(RpAtomic * atomic,
                            RwInt32 offset, RwInt32 size __RWUNUSED__)
{
    RpPrtStdEmitter         *emt;
    RwInt32                 emtSize;

    RWFUNCTION(RWSTRING("PrtStdAtomicGetChunkSize"));

    RWASSERT(atomic);

    emt = ((RpPrtStdEmitter *)
        RPPRTSTDATOMICPROP(atomic, offset));

    emtSize = 0;
    if (emt != NULL)
    {
        emtSize = /*rwCHUNKHEADERSIZE + */sizeof(RwInt32);

        RpPrtStdAtomicForAllEmitter(atomic,
            _rpPrtStdEmitterStreamGetSize, (void *)&emtSize);
    }

    RWRETURN(emtSize);
}

static RwStream *
PrtStdAtomicReadChunk(RwStream * stream,
                         RwInt32 len __RWUNUSED__,
                         RpAtomic * atomic,
                         RwInt32 offset __RWUNUSED__, RwInt32 size __RWUNUSED__)
{
    RwStream            *result;
    RwInt32             i, emtCount;

    RWFUNCTION(RWSTRING("PrtStdAtomicReadChunk"));

    result = stream;

    RwStreamReadInt32(stream, &emtCount, sizeof(RwInt32));

    /*
     * Extract the version from the count. A bit naughty but the prtstd
     * does not write any version info anyway and there isn't a way to
     * obtain the chunk header info.
     */
    rpPrtStdGlobals.ver = (emtCount & 0xFF000000) >> 24;
    emtCount &= ~(0xFF000000);

    /* Test if there are any emitters. */
    if (emtCount > 0)
    {
        /* Read in the emitters. */
        for (i = 0; i < emtCount; i++)
            _rpPrtStdEmitterStreamRead(atomic, stream);

        RpAtomicSetRenderCallBack(atomic, _rpPrtStdAtomicRender);
    }

    /* Reset the version stamp. */
    rpPrtStdGlobals.ver = 0;


    RWRETURN(result);
}

static RwStream *
PrtStdAtomicWriteChunk(RwStream * stream,
                          RwInt32 len __RWUNUSED__,
                          RpAtomic * atomic,
                          RwInt32 offset,
                          RwInt32 size __RWUNUSED__)
{
    RwInt32                     i;
    RpPrtStdEmitter             *emt;

    RWFUNCTION(RWSTRING("PrtStdAtomicWriteChunk"));

    emt = ((RpPrtStdEmitter *)
        RPPRTSTDATOMICPROP(atomic, offset));

    if (emt != NULL)
    {
        i = 0;

        /* Count the number of emitters. */
        RpPrtStdAtomicForAllEmitter(atomic,
            _rpPrtStdAtomicCountEmitters, (void *)&i);

        RWASSERT(i >= 0);

        /*
         * Encode the version stamp into emitter count. Prtstd does not
         * write out any chunk header info for version stamping so we need
         * to use the emitter count.
         */

        i |= (rpPRTSTDVERSIONSTAMP << 24);

        /* Write out the number of emitters. */
        RwStreamWriteInt32(stream, &i, sizeof(RwInt32));

        /* Write out the emitters. */
        RpPrtStdAtomicForAllEmitter(atomic,
            _rpPrtStdEmitterStreamWrite, (void *)stream);
    }
    else
    {
        RWRETURN((RwStream*)NULL);
    }

    RWRETURN(stream);
}

/************************************************************************
 *
 *
 ************************************************************************/
RwInt32
_rpPrtStdStreamGetVersion( void )
{
    RWFUNCTION(RWSTRING("_rpPrtStdStreamGetVersion"));

    RWRETURN(rpPrtStdGlobals.ver);
}

/************************************************************************
 *
 *
 ************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPluginAttach is called by the application to indicate
 * that the particle plugin should be used. The call
 * to this function should be placed between \ref RwEngineInit and
 * \ref RwEngineOpen and the world plugin must already be attached.
 *
 * The library rpprtstd and the header file rpprtstd.h are required.
 *
 * \return TRUE on success, FALSE otherwise
 */

RwBool
RpPrtStdPluginAttach(void)
{
    const RwUInt32 PrtStdId = rwID_PRTSTDPLUGIN;

    RWAPIFUNCTION(RWSTRING("RpPrtStdPluginAttach"));

    rpPrtStdGlobals.engineOffset =
        RwEngineRegisterPlugin(0,
                               PrtStdId,
                               (RwPluginObjectConstructor) PrtStdEngineOpen,
                               (RwPluginObjectDestructor) PrtStdEngineClose);

    rpPrtStdGlobals.atmOffset =
        RpAtomicRegisterPlugin(sizeof(void *),
                               PrtStdId,
                               PrtStdAtomicCtor,
                               PrtStdAtomicDtor,
                               PrtStdAtomicCopy);


    rpPrtStdGlobals.atmStreamOffset =
        RpAtomicRegisterPluginStream(PrtStdId,
                                     (RwPluginDataChunkReadCallBack) PrtStdAtomicReadChunk,
                                     (RwPluginDataChunkWriteCallBack) PrtStdAtomicWriteChunk,
                                     (RwPluginDataChunkGetSizeCallBack) PrtStdAtomicGetChunkSize);


    rpPrtStdGlobals.emtClass = NULL;
    rpPrtStdGlobals.prtClass = NULL;
    rpPrtStdGlobals.propTab = NULL;

    rpPrtStdGlobals.rpPrtStdStreamEmbedded = TRUE;

#if (defined(SKY2_DRVMODEL_H) )
    RpPrtStdSetEClassSetupCallBack(RpPrtStdEClassStdSetupSkyCB);
    RpPrtStdSetPClassSetupCallBack(RpPrtStdPClassStdSetupSkyCB);
#else
    RpPrtStdSetEClassSetupCallBack(RpPrtStdEClassStdSetupCB);
    RpPrtStdSetPClassSetupCallBack(RpPrtStdPClassStdSetupCB);
#endif /* (defined(SKY2_DRVMODEL_H)  */


    rpPrtStdGlobals.prtPropTabCurr = NULL;
    rpPrtStdGlobals.emtPropTabCurr = NULL;

    rpPrtStdGlobals.offsetEmtStd = -1;
    rpPrtStdGlobals.offsetEmtPrtCol = -1;
    rpPrtStdGlobals.offsetEmtPrtTex = -1;
    rpPrtStdGlobals.offsetEmtPrtSize = -1;
    rpPrtStdGlobals.offsetEmtPTank = -1;

    rpPrtStdGlobals.offsetPrtPos = -1;

    rpPrtStdGlobals.offsetPrtStd = -1;
    rpPrtStdGlobals.offsetPrtCol = -1;
    rpPrtStdGlobals.offsetPrtTex = -1;
    rpPrtStdGlobals.offsetPrtSize = -1;

    rpPrtStdGlobals.propIDScratchMem = RwMalloc(PRTSTDSCRATCHMEMSIZE,
        rwID_PRTSTDPLUGIN | rwMEMHINTDUR_GLOBAL | rwMEMHINTFLAG_RESIZABLE);
    rpPrtStdGlobals.propIDScratchMemSize = PRTSTDSCRATCHMEMSIZE;

    rpPrtStdGlobals.propTabScratchMem = RwMalloc(PRTSTDSCRATCHMEMSIZE,
        rwID_PRTSTDPLUGIN | rwMEMHINTDUR_GLOBAL | rwMEMHINTFLAG_RESIZABLE);
    rpPrtStdGlobals.propTabScratchMemSize = PRTSTDSCRATCHMEMSIZE;

    rpPrtStdGlobals.propTabPostReadCallBack = NULL;
    rpPrtStdGlobals.propTabDtorCallBack = NULL;

    RWRETURN((TRUE));
}


/************************************************************************
 *
 *
 ************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpAtomicIsParticleEmitter is called by the application to query
 * if an atomic is a particle emitter.
 *
 * The library rpprtstd and the header file rpprtstd.h are required.
 *
 * \param atomic        Pointer to an atomic.
 *
 * \return an TRUE if atomic is a particle emitter, FALSE otherwise.
 */

RwBool
RpAtomicIsParticleEmitter(RpAtomic *atomic)
{
    RpPrtStdEmitter         *emt;

    RWAPIFUNCTION(RWSTRING("RpAtomicIsParticleEmitter"));

    RWASSERT(atomic);

    emt = RPPRTSTDATOMICPROP(atomic, rpPrtStdGlobals.atmOffset);

    if (emt)
    {
        RWRETURN(TRUE);
    }

    RWRETURN(FALSE);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdAtomicCreate is called by the application to create an
 * emitter atomic.
 *
 * The atomic will have a single particle emitter attached as extension data.
 * The type of emitter is set by the eClass parameter. The class's
 * create callbacks will be called to allow custom properties to be
 * set.
 *
 * An emitter atomic is different from a normal atomic. It does not
 * contain any geometric data and its render callback is replaced.
 *
 * Depending on the emitters attached, the atomic's frame could be used
 * to position the emitters in the world. So if the emitter is not
 * stationary, the atomic's frame should be updated as with other
 * atomics.
 *
 * The atomic can be destroyed using \ref RpAtomicDestroy.
 *
 * The library rpprtstd and the header file rpprtstd.h are required.
 *
 * \param eClass        Pointer to an emitter class.
 * \param data          Private data passed to the create callbacks.
 *
 * \return an atomic on success, NULL otherwise
 */

RpAtomic *
RpPrtStdAtomicCreate(RpPrtStdEmitterClass *eClass,
                     void * data )
{
    RpPrtStdEmitter         *emt;
    RpGeometry              *geom;
    RpAtomic                *atomic;

    RWAPIFUNCTION(RWSTRING("RpPrtStdAtomicCreate"));

    RWASSERT(eClass);

    emt = RpPrtStdEmitterCreate(eClass, data);

    atomic = NULL;
    geom = NULL;
    if (emt)
    {
        geom = RpGeometryCreateSpace( (RwReal) 0.0 );

        if (geom != NULL)
        {
            atomic = RpAtomicCreate();

            if (atomic)
            {
                RPPRTSTDATOMICPROP(atomic, rpPrtStdGlobals.atmOffset) = emt;

                emt->atomic = atomic;

                RpAtomicSetRenderCallBack(atomic, _rpPrtStdAtomicRender);
                RpAtomicSetGeometry(atomic, geom, 0);

                /* Reduce the geom ref count by 1*/
                RpGeometryDestroy(geom);
            }
            else
            {
                RpGeometryDestroy(geom);
                RpPrtStdEmitterDestroy(emt);
            }
        }
        else
        {
            RpPrtStdEmitterDestroy(emt);
        }
    }

    RWRETURN(atomic);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdAtomicForAllEmitter is a utility function to apply a callback
 * function to all emitters attached to the atomic.
 *
 * If the atomic does not contain any emitters, then the function does nothing.
 *
 * The library rpprtstd and the header file rpprtstd.h are required.
 *
 * \param atomic        Pointer to an emitter atomic.
 * \param callback      Callback function to be called for the emitters.
 * \param data          Pointer to private data to be passed to the callback.
 *
 * \return an atomic on success, NULL otherwise
 *
 * \see RpPrtStdAtomicCreate
 * \see RpPrtStdAtomicAddEmitter
 */

RpAtomic *
RpPrtStdAtomicForAllEmitter(RpAtomic *atomic, RpPrtStdEmitterCallBack callback, void *data)
{
    RpPrtStdEmitter         *emt, *emtHead, *emtNext;

    RWAPIFUNCTION(RWSTRING("RpPrtStdAtomicForAllEmitter"));

    RWASSERT(atomic);
    RWASSERT(callback);

    emt = (RpPrtStdEmitter *)
        RPPRTSTDATOMICPROP(atomic, rpPrtStdGlobals.atmOffset);

    if (emt)
    {
        emtHead = emt->next;
        emt = emt->next;

        do
        {
            RWASSERT(emt);

            emtNext = emt->next;

            if ((callback)(atomic, emt, data) != emt)
                RWRETURN((RpAtomic *) NULL);

            emt = emtNext;
        }
        while (emt != emtHead);
    }

    RWRETURN(atomic);

}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdAtomicGetEmitter is a utility function to return the emitter
 * attached to the atomic.
 *
 * The library rpprtstd and the header file rpprtstd.h are required.
 *
 * \param atomic        Pointer to an emitter atomic.
 *
 * \return a pointer to an emitter on success, NULL otherwise
 *
 * \see RpPrtStdAtomicCreate
 * \see RpPrtStdAtomicAddEmitter
 * \see RpPrtStdAtomicSetEmitter
 */
RpPrtStdEmitter *
RpPrtStdAtomicGetEmitter(RpAtomic *atomic)
{
    RpPrtStdEmitter         *emt;

    RWAPIFUNCTION(RWSTRING("RpPrtStdAtomicGetEmitter"));

    RWASSERT(atomic);

    emt = (RpPrtStdEmitter *)
        RPPRTSTDATOMICPROP(atomic, rpPrtStdGlobals.atmOffset);

    RWRETURN(emt);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdAtomicSetEmitter is a utility function to attach an
 * emitter to an atomic. This replaces the existing emitter in the
 * atomic. It is the responsibility of the caller to destroy the
 * atomic's existing emitters if required.
 *
 * This function should only be used for atomics created with
 * \ref RpPrtStdAtomicCreate.
 *
 * The library rpprtstd and the header file rpprtstd.h are required.
 *
 * \param atomic        Pointer to an emitter atomic.
 * \param emt           Pointer to an emitter to be attached.
 *
 * \return a pointer to an atomic on success, NULL otherwise
 *
 * \see RpPrtStdAtomicCreate
 * \see RpPrtStdAtomicAddEmitter
 * \see RpPrtStdAtomicGetEmitter
 */
RpAtomic *
RpPrtStdAtomicSetEmitter(RpAtomic *atomic, RpPrtStdEmitter *emt)
{
    RWAPIFUNCTION(RWSTRING("RpPrtStdAtomicSetEmitter"));

    RWASSERT(atomic);

    RPPRTSTDATOMICPROP(atomic, rpPrtStdGlobals.atmOffset) = emt;

    emt->atomic = atomic;

    RWRETURN(atomic);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdAtomicAddEmitter is a utility function to add an
 * emitter to an atomic. The emitters are held in a circular linked
 * list in the order they are added to the atomic.
 *
 * This function should only be used for atomics created with
 * \ref RpPrtStdAtomicCreate.
 *
 * The library rpprtstd and the header file rpprtstd.h are required.
 *
 * \param atomic        Pointer to an emitter atomic.
 * \param emt           Pointer to an emitter to be added.
 *
 * \return a pointer to an atomic on success, NULL otherwise
 *
 * \see RpPrtStdAtomicCreate
 * \see RpPrtStdAtomicGetEmitter
 * \see RpPrtStdAtomicSetEmitter
 */
RpAtomic *
RpPrtStdAtomicAddEmitter(RpAtomic *atomic, RpPrtStdEmitter *emt)
{
    RpPrtStdEmitter             *emtHead;

    RWAPIFUNCTION(RWSTRING("RpPrtStdAtomicAddEmitter"));

    RWASSERT(atomic);
    RWASSERT(emt);

    emtHead = (RpPrtStdEmitter *)
        RPPRTSTDATOMICPROP(atomic, rpPrtStdGlobals.atmOffset);

    RPPRTSTDATOMICPROP(atomic, rpPrtStdGlobals.atmOffset) =
        RpPrtStdEmitterAddEmitter(emtHead, emt);

    emt->atomic = atomic;

    RWRETURN(atomic);
}


/************************************************************************/


/**
 * \ingroup rpprtstd
 * \ref RpPrtStdAtomicUpdate is the function to update all the emitters
 * in the atomic. This in turn updates all particle emitted by the
 * emitters. New particles are also created by emitters during the
 * emit stage. Dead emitters and particles are removed before any updates.
 *
 * The update is performed by the update callback of the emitters' and
 * particles' classes. The emitters and particles are updated in the order
 * they are added to the atomic and emitters respectively. The callbacks are
 * applied in the order they are created in the emitter and particle classes.
 *
 * The emitters are updated before any particles.
 *
 * The particles are updated in batches and all update callbacks are applied
 * to the batch before the next batch.
 *
 * The content of the data parameter is implementation specific, depending
 * on the underlying callback. This may be a simple delta time value to
 * a private data strcuture if more than one type of data are used for
 * updating.
 *
 * This function should only be used for atomics created with
 * \ref RpPrtStdAtomicCreate. It cannot be used between \ref
 * RwCameraBeginUpdate and \ref RwCameraEndUpdate. Depending on the platform,
 * it may also make use of hardware resources for optimization purposes.
 * The application needs to ensure this does not conflict with its own use of
 * these resources. See the platform's specific section for more details.
 *
 * The library rpprtstd and the header file rpprtstd.h are required.
 *
 * \param atomic        Pointer to an emitter atomic.
 * \param data          Pointer to private data for the update callbacks, this
 *                      contents of this parameter depends on the underlying
 *                      callbacks.
 *
 * \return a pointer to an atomic on success, NULL otherwise
 *
 * \see RpPrtStdAtomicCreate
 * \see RpPrtStdParticleStdUpdateCB
 * \see RpPrtStdEmitterStdEmitCB
 *
 */
RpAtomic *
RpPrtStdAtomicUpdate(RpAtomic *atomic, void *data)
{
    RpPrtStdEmitter         *emt;
    RpAtomic                *result;
    RpGeometry              *geom;
    RpMorphTarget           *morphTarget;
    RwSphere                *sphere;

    RWAPIFUNCTION(RWSTRING("RpPrtStdAtomicUpdate"));

    RWASSERT(atomic);

    result = NULL;

    emt = (RpPrtStdEmitter *)
        RPPRTSTDATOMICPROP(atomic, rpPrtStdGlobals.atmOffset);

    if (emt)
    {
        /* Reset the bounding sphere. This gets update during update */
        sphere = RpAtomicGetBoundingSphere(atomic);
        sphere->radius = (RwReal) 0.0;

        /* Remove any dead emitters. */


        /* Update  emitters. */
        result = RpPrtStdAtomicForAllEmitter(atomic,
            _rpPrtStdEmitterUpdate, data);

        geom = RpAtomicGetGeometry(atomic);
        morphTarget = RpGeometryGetMorphTarget(geom, 0);
        RpMorphTargetSetBoundingSphere(morphTarget, sphere);

        /* World bounding sphere is no longer valid */
        rwObjectSetPrivateFlags(atomic, rwObjectGetPrivateFlags(atomic) |
                                rpATOMICPRIVATEWORLDBOUNDDIRTY);
    }

    RWRETURN(result);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdGlobalDataStreamRead is the function to stream in the shared
 * data used by an emitter.
 *
 * \ref RpPrtStdGlobalDataStreamRead streams in the \ref RpPrtStdEmitterClass,
 * the \ref RpPrtStdParticleClass and the \ref RpPrtStdPropertyTable.
 * These data are stored by default with the emitters, but as these can be
 * shared by more than one emitter, they can be stored separatly using
 * \ref RpPrtStdGlobalDataStreamWrite
 *
 * When the emitters are streamed in, if there is no emitter class streamed with
 * it, it will search for any emitter class with the same identification number.
 * If it fails, then the emitter will have a NULL emitter class.
 *
 * The sequence to locate and read a emitter class from a binary stream is
 * as follows:
 * \code
   RwStream *stream;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_PRTSTDGLOBALDATA, NULL, NULL) )
       {
           if (RpPrtStdGlobalDataStreamRead(stream) != stream)
           {
               // Error during input.
           }
       }

       RwStreamClose(stream, NULL);
   }
   \endcode
 *
 * \param stream        A pointer to the input stream.
 *
 * \return a pointer to the input stream on success, NULL otherwise.
 *
 * \see RpPrtStdGlobalDataStreamGetSize
 * \see RpPrtStdGlobalDataStreamRead
 * \see RpPrtStdEClassStreamRead
 * \see RpPrtStdEClassStreamWrite
 * \see RpPrtStdEClassStreamGetSize
 * \see RpPrtStdPClassStreamRead
 * \see RpPrtStdPClassStreamWrite
 * \see RpPrtStdPClassStreamGetSize
 * \see RpPrtStdPropTabStreamRead
 * \see RpPrtStdPropTabStreamWrite
 * \see RpPrtStdPropTabStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RwStreamFindChunk
 * \see RpPrtStdPluginAttach
 */
RwStream *
RpPrtStdGlobalDataStreamRead(RwStream *stream)
{
    RwInt32                     i, count;
    RpPrtStdEmitterClass        *eClass/*, *eClassHead*/;
    RpPrtStdParticleClass       *pClass/*, *pClassHead*/;
    RpPrtStdPropertyTable       *propTab/*, *propTabHead*/;

    RWAPIFUNCTION(RWSTRING("RpPrtStdGlobalDataStreamRead"));
    RWASSERT(stream);

/*    eClassHead = rpPrtStdGlobals.emtClass;
    pClassHead = rpPrtStdGlobals.prtClass;
    propTabHead = rpPrtStdGlobals.propTab;*/

    /* Read in the version stamp */
    RwStreamReadInt32(stream, &rpPrtStdGlobals.ver, sizeof(RwInt32));

    /* Read in the number of property tables */
    RwStreamReadInt32(stream, &count, sizeof(RwInt32));

    /* Check and stream in the property tables */
    if (count > 0)
    {
        for (i = 0; i < count; i++)
        {
            /* Read in the chunk header */
            if( !RwStreamFindChunk(stream, rwID_PRTSTDGLOBALDATA,
                            (RwUInt32 *)NULL, (RwUInt32 *)NULL) )
            {
                RWRETURN ((RwStream *)(NULL));
            }

            propTab = RpPrtStdPropTabStreamRead(stream);

            /* propTabHead = _rpPrtStdPropTabAdd(propTabHead, propTab); */
        }
    }

    /* Read in the number of particle classes */
    RwStreamReadInt32(stream, &count, sizeof(RwInt32));

    /* Check and stream in the particle classes */
    if (count > 0)
    {
        for (i = 0; i < count; i++)
        {
            /* Read in the chunk header */
            if( !RwStreamFindChunk(stream, rwID_PRTSTDGLOBALDATA,
                            (RwUInt32 *)NULL, (RwUInt32 *)NULL) )
            {
                RWRETURN ((RwStream*) (NULL));
            }

            pClass = RpPrtStdPClassStreamRead(stream);

/*            pClassHead = _rpPrtStdPClassAdd(pClassHead, pClass);*/
        }
    }

    /* Read in the number of emitter classes */
    RwStreamReadInt32(stream, &count, sizeof(RwInt32));

    /* Check and stream in the emitter classes */
    if (count > 0)
    {
        for (i = 0; i < count; i++)
        {
            /* Read in the chunk header */
            if( !RwStreamFindChunk(stream, rwID_PRTSTDGLOBALDATA,
                            (RwUInt32 *)NULL, (RwUInt32 *)NULL) )
            {
                RWRETURN ((RwStream*)(NULL));
            }

            eClass = RpPrtStdEClassStreamRead(stream);

/*            eClassHead = _rpPrtStdEClassAdd(eClassHead, eClass);*/
        }
    }

/*    rpPrtStdGlobals.propTab = propTabHead;
    rpPrtStdGlobals.prtClass = pClassHead;
    rpPrtStdGlobals.emtClass = eClassHead;*/

    RWRETURN(stream);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdGlobalDataStreamWrite is the function to stream out the shared
 * data used by an emitter.
 *
 * \ref RpPrtStdGlobalDataStreamWrite streams out the \ref RpPrtStdEmitterClass,
 * the \ref RpPrtStdParticleClass and the \ref RpPrtStdPropertyTable.
 * These data are stored by default with the emitters, but as these can be
 * shared by more than one emitter, they can be stored separatly using
 * \ref RpPrtStdGlobalDataStreamWrite
 *
 * When the emitters are streamed in, if there is no emitter class streamed with
 * it, it will search for any emitter class with the same identification number.
 * If it fails, then the emitter will have a NULL emitter class.
 *
 *
 * \param stream        A pointer to the output stream.
 *
 * \return a pointer to the output stream in success, NULL otherwise.
 *
 * \see RpPrtStdGlobalDataStreamGetSize
 * \see RpPrtStdGlobalDataStreamWrite
 * \see RpPrtStdEClassStreamRead
 * \see RpPrtStdEClassStreamWrite
 * \see RpPrtStdEClassStreamGetSize
 * \see RpPrtStdPClassStreamRead
 * \see RpPrtStdPClassStreamWrite
 * \see RpPrtStdPClassStreamGetSize
 * \see RpPrtStdPropTabStreamRead
 * \see RpPrtStdPropTabStreamWrite
 * \see RpPrtStdPropTabStreamGetSize
 */
RwStream *
RpPrtStdGlobalDataStreamWrite(RwStream *stream)
{
    RwInt32                     i;
    RpPrtStdEmitterClass        *eClass, *eClassHead;
    RpPrtStdParticleClass       *pClass, *pClassHead;
    RpPrtStdPropertyTable       *propTab, *propTabHead;

    RWAPIFUNCTION(RWSTRING("RpPrtStdGlobalDataStreamWrite"));

    RWASSERT(stream);

    /* Write header */
    i = RpPrtStdGlobalDataStreamGetSize();
    if (!RwStreamWriteChunkHeader(stream, rwID_PRTSTDGLOBALDATA, i))
    {
        RWRETURN((RwStream *)NULL);
    }

    /* Write out the version stamp */
    i = rpPRTSTDVERSIONSTAMP;
    RwStreamWriteInt32(stream, &i, sizeof(RwInt32));

    /* First write out the property tables */
    propTabHead = rpPrtStdGlobals.propTab;
    if (propTabHead != NULL)
    {
        /* Count the number of prop tables */
        i = 0;
        propTab = propTabHead;
        do
        {
            RWASSERT(propTab);

            i++;

            propTab = propTab->next;
        }
        while (propTab != propTabHead);

        /* Write out the count */
        RwStreamWriteInt32(stream, &i, sizeof(RwInt32));

        /* Write out the property tables */
        propTab = propTabHead;
        do
        {
            RWASSERT(propTab);

            RpPrtStdPropTabStreamWrite(propTab, stream);

            propTab = propTab->next;
        }
        while (propTab != propTabHead);
    }
    else
    {
        i = 0;

        RwStreamWriteInt32(stream, &i, sizeof(RwInt32));
    }

    /* Stream out the particle classes */
    pClassHead = rpPrtStdGlobals.prtClass;
    if (pClassHead != NULL)
    {
        /* Count the number of particle classes */
        i = 0;
        pClass = pClassHead;
        do
        {
            RWASSERT(pClass);

            i++;

            pClass = pClass->next;
        }
        while (pClass != pClassHead);

        /* Write out the count */
        RwStreamWriteInt32(stream, &i, sizeof(RwInt32));

        /* Write out the particle classes */
        pClass = pClassHead;
        do
        {
            RWASSERT(pClass);

            RpPrtStdPClassStreamWrite(pClass, stream);

            pClass = pClass->next;
        }
        while (pClass != pClassHead);
    }
    else
    {
        i = 0;

        RwStreamWriteInt32(stream, &i, sizeof(RwInt32));
    }

    /* Stream out the emitter classes. */
    eClassHead = rpPrtStdGlobals.emtClass;
    if (eClassHead != NULL)
    {
        /* Count the number of the emitters */
        i = 0;
        eClass = eClassHead;
        do
        {
            RWASSERT(eClass);

            i++;

            eClass = eClass->next;
        }
        while (eClass != eClassHead);

        /* Write out the count */
        RwStreamWriteInt32(stream, &i, sizeof(RwInt32));

        /* Write out the emitter classes */
        eClass = eClassHead;
        do
        {
            RWASSERT(eClass);

            RpPrtStdEClassStreamWrite(eClass, stream);

            eClass = eClass->next;
        }
        while (eClass != eClassHead);
    }
    else
    {
        i = 0;

        RwStreamWriteInt32(stream, &i, sizeof(RwInt32));
    }

    RWRETURN(stream);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdGlobalDataStreamGetSize is used to determine the size in bytes of
 * the binary representation of an emitter class, particle class and property tables.
 * This value is used in the binary chunk header to indicate the size of the chunk.
 * The size includes the size of the chunk header.
 *
 * \return Returns a \ref RwInt32 value equal to the chunk size (in bytes)
 * of the emitter class.
 *
 * \see RpPrtStdGlobalDataStreamRead
 * \see RpPrtStdGlobalDataStreamWrite
 * \see RpPrtStdEClassStreamRead
 * \see RpPrtStdEClassStreamWrite
 * \see RpPrtStdEClassStreamGetSize
 * \see RpPrtStdPClassStreamRead
 * \see RpPrtStdPClassStreamWrite
 * \see RpPrtStdPClassStreamGetSize
 * \see RpPrtStdPropTabStreamRead
 * \see RpPrtStdPropTabStreamWrite
 * \see RpPrtStdPropTabStreamGetSize
 */
RwInt32
RpPrtStdGlobalDataStreamGetSize( void )
{
    RwInt32                     size;
    RpPrtStdEmitterClass        *eClass, *eClassHead;
    RpPrtStdParticleClass       *pClass, *pClassHead;
    RpPrtStdPropertyTable       *propTab, *propTabHead;

    RWAPIFUNCTION(RWSTRING("RpPrtStdGlobalDataStreamGetSize"));

    /* Chunk header */
/*    size =  rwCHUNKHEADERSIZE; */

    /* Version stamp */
    size = sizeof(RwInt32);

    /* Prop table count. */
    size += sizeof(RwInt32);

    /* Size of all the property tables. */
    propTab = rpPrtStdGlobals.propTab;
    if (propTab != NULL)
    {
        propTabHead = propTab;
        do
        {
            RWASSERT(propTab);

            size += RpPrtStdPropTabStreamGetSize(propTab);
            size += rwCHUNKHEADERSIZE;

            propTab = propTab->next;
        }
        while (propTab != propTabHead);
    }

    /* Particle class count. */
    size += sizeof(RwInt32);

    /* Size of all the particle classes. */
    pClass = rpPrtStdGlobals.prtClass;
    if (pClass != NULL)
    {
        pClassHead = pClass;
        do
        {
            RWASSERT(pClass);

            size += RpPrtStdPClassStreamGetSize(pClass);
            size += rwCHUNKHEADERSIZE;

            pClass = pClass->next;
        }
        while (pClass != pClassHead);
    }

    /* Emitter class count. */
    size += sizeof(RwInt32);

    /* Size of all the emitter classes. */
    eClass = rpPrtStdGlobals.emtClass;
    if (eClass != NULL)
    {
        eClassHead = eClass;
        do
        {
            RWASSERT(eClass);

            size += RpPrtStdEClassStreamGetSize(eClass);
            size += rwCHUNKHEADERSIZE;

            eClass = eClass->next;
        }
        while (eClass != eClassHead);
    }

    RWRETURN(size);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdSetEClassSetupCallBack is the function to set the
 * \ref RpPrtStdEClassSetupCallBack callback function.
 *
 * The \ref RpPrtStdEClassSetupCallBack is used to setup the callback function
 * for an emitter class. It is called after a \ref RpPrtStdEmitterClass is streamed
 * in. This allows the user the opportunity to setup the \ref RpPrtStdEmitterClass
 * with any custom function for the emitter class.
 *
 * \param emtCB     A \ref RpPrtStdEClassSetupCallBack to be set.
 *
 * \returns TRUE if successful, FALSE otherwise.
 * \see RpPrtStdGetEClassSetupCallBack
 * \see RpPrtStdGetPClassSetupCallBack
 * \see RpPrtStdSetPClassSetupCallBack
 */
RwBool
RpPrtStdSetEClassSetupCallBack(RpPrtStdEClassSetupCallBack emtCB)
{
    RWAPIFUNCTION(RWSTRING("RpPrtStdSetEClassSetupCallBack"));

    RWASSERT(emtCB);

    rpPrtStdGlobals.emtClassSetupCallback = emtCB;

    RWRETURN(TRUE);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdGetEClassSetupCallBack is the function to get the
 * \ref RpPrtStdEClassSetupCallBack callback function.
 *
 * The \ref RpPrtStdEClassSetupCallBack is used to setup the callback function
 * for an emitter class. It is called after a \ref RpPrtStdEmitterClass is streamed
 * in. This allows the user the opportunity to setup the \ref RpPrtStdEmitterClass
 * with any custom function for the emitter class.
 *
 * \param emtCB     A pointer to a \ref RpPrtStdEClassSetupCallBack for returning
 *                  the setup function.
 *
 * \returns TRUE if successful, FALSE otherwise.
 *
 * \see RpPrtStdSetEClassSetupCallBack
 * \see RpPrtStdGetPClassSetupCallBack
 * \see RpPrtStdSetPClassSetupCallBack
 */
RwBool
RpPrtStdGetEClassSetupCallBack(RpPrtStdEClassSetupCallBack *emtCB)
{
    RWAPIFUNCTION(RWSTRING("RpPrtStdGetEClassSetupCallBack"));

    *emtCB = rpPrtStdGlobals.emtClassSetupCallback;

    RWRETURN(TRUE);
}


/**
 * \ingroup rpprtstd
 * \ref RpPrtStdSetPClassSetupCallBack is the function to set the
 * \ref RpPrtStdPClassSetupCallBack callback function.
 *
 * The \ref RpPrtStdPClassSetupCallBack is used to setup the callback function
 * for an emitter class. It is called after a \ref RpPrtStdParticleClass is streamed
 * in. This allows the user the opportunity to setup the \ref RpPrtStdParticleClass
 * with any custom function for the particle class.
 *
 * \param prtCB     A \ref RpPrtStdPClassSetupCallBack to be set.
 *
 * \returns TRUE if successful, FALSE otherwise.
 *
 * \see RpPrtStdGetEClassSetupCallBack
 * \see RpPrtStdSetEClassSetupCallBack
 * \see RpPrtStdGetPClassSetupCallBack
 */
RwBool
RpPrtStdSetPClassSetupCallBack(RpPrtStdPClassSetupCallBack prtCB)
{
    RWAPIFUNCTION(RWSTRING("RpPrtStdSetPClassSetupCallBack"));

    rpPrtStdGlobals.prtClassSetupCallback = prtCB;

    RWRETURN(TRUE);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdGetPClassSetupCallBack is the function to get the
 * \ref RpPrtStdPClassSetupCallBack callback function.
 *
 * The \ref RpPrtStdPClassSetupCallBack is used to setup the callback function
 * for a particle class. It is called after a \ref RpPrtStdParticleClass is streamed
 * in. This allows the user the opportunity to setup the \ref RpPrtStdParticleClass
 * with any custom function for the particle class.
 *
 * \param prtCB     A pointer to a \ref RpPrtStdPClassSetupCallBack for returning
 *                  the setup function.
 *
 * \returns TRUE if successful, FALSE otherwise.
 *
 * \see RpPrtStdGetEClassSetupCallBack
 * \see RpPrtStdSetEClassSetupCallBack
 * \see RpPrtStdSetPClassSetupCallBack
 */
RwBool
RpPrtStdGetPClassSetupCallBack(RpPrtStdPClassSetupCallBack *prtCB)
{
    RWAPIFUNCTION(RWSTRING("RpPrtStdGetPClassSetupCallBack"));

    *prtCB = rpPrtStdGlobals.prtClassSetupCallback;

    RWRETURN(TRUE);
}


/**
 * \ingroup rpprtstd
 * \ref RpPrtStdGlobalDataSetStreamEmbedded is the function to set the status of
 * stream embedded flag.
 *
 * The stream embeddes flag change the way the emitters are streamed :
 * by default the emitters are streamed with there attached emitter class,
 * particle class and two property tables (one for the emitter class,
 * one for the particle class).
 * As most of the class and properties can be shared by many emitters, it is
 * possible to stream out emitters whithout those informations by setting the
 * stream embedded flag to FALSE, and to stream out all the classes at once using
 * the \ref RpPrtStdGlobalDataStreamWrite function, reducing the size of the
 * files.
 *
 * \param value     the new value of the stream embedded flag.
 *
 * \see RpPrtStdGlobalDataGetStreamEmbedded
 * \see RpPrtStdEClassStreamWrite
 * \see RpPrtStdEClassStreamRead
 * \see RpPrtStdPClassStreamWrite
 * \see RpPrtStdPClassStreamRead
 * \see RpPrtStdPropTabStreamRead
 * \see RpPrtStdPropTabStreamWrite
 * \see RpPrtStdPropTabStreamGetSize
 * \see RpPrtStdGlobalDataStreamRead
 * \see RpPrtStdGlobalDataStreamWrite
 * \see RpPrtStdGlobalDataStreamGetSize
 * \see RpPrtStdGlobalDataStreamRead
 * \see RpPrtStdGlobalDataStreamWrite
 *
 */
void
RpPrtStdGlobalDataSetStreamEmbedded(RwBool value)
{
    RWAPIFUNCTION(RWSTRING("RpPrtStdGlobalDataSetStreamEmbedded"));
    rpPrtStdGlobals.rpPrtStdStreamEmbedded = value;
    RWRETURNVOID();
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdGlobalDataGetStreamEmbedded is the function to get the status of
 * stream embedded flag.
 *
 * \returns the value of the stream embedded flag.
 *
 * \see RpPrtStdGlobalDataSetStreamEmbedded
 * \see RpPrtStdEClassStreamWrite
 * \see RpPrtStdEClassStreamRead
 * \see RpPrtStdPClassStreamWrite
 * \see RpPrtStdPClassStreamRead
 * \see RpPrtStdPropTabStreamRead
 * \see RpPrtStdPropTabStreamWrite
 * \see RpPrtStdPropTabStreamGetSize
 * \see RpPrtStdGlobalDataStreamRead
 * \see RpPrtStdGlobalDataStreamWrite
 * \see RpPrtStdGlobalDataStreamGetSize
 * \see RpPrtStdGlobalDataStreamRead
 * \see RpPrtStdGlobalDataStreamWrite
 *
 */
RwBool
RpPrtStdGlobalDataGetStreamEmbedded(void)
{
    RWAPIFUNCTION(RWSTRING("RpPrtStdGlobalDataGetStreamEmbedded"));
    RWRETURN(rpPrtStdGlobals.rpPrtStdStreamEmbedded);
}



