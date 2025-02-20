/**********************************************************************
 *
 * File :     pclass.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rwcore.h>
#include <rpworld.h>
#include <rpptank.h>

#include <rpplugin.h>
#include <rpdbgerr.h>

#include "rpprtstd.h"

#include "emitter.h"
#include "pclass.h"
#include "prop.h"
#include "pptank.h"

#include "prtstdvars.h"

/************************************************************************
 *
 *
 ************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdParticleDefaultCB is the default empty callback for
 * the particle class.
 *
 * \param prtEmt    Pointer to the emitter.
 * \param prtBatch  Pointer to the particle batch.
 * \param data      Pointer to private data.
 *
 * \return a pointer to a particle on success, NULL otherwise.
 *
 * \see RpPrtStdPClassCreate
 * \see RpPrtStdPClassGetCallBack
 * \see RpPrtStdPClassSetCallBack
 * \see RpPrtStdPClassGetPropTab
 * \see RpPrtStdPClassSetPropTab
 * \see RpPrtStdPropTabCreate
 */
RpPrtStdParticleBatch *
RpPrtStdParticleDefaultCB(RpPrtStdEmitter * prtEmt __RWUNUSED__,
                          RpPrtStdParticleBatch *prtBatch,
                          void * data __RWUNUSED__)
{
    RWAPIFUNCTION(RWSTRING("RpPrtStdParticleDefaultCB"));

    RWRETURN(prtBatch);
}

/************************************************************************
 *
 *
 ************************************************************************/

RpPrtStdParticleClass *
_rpPrtStdPClassCtor(RpPrtStdParticleClass *pClass)
{
    RWFUNCTION(RWSTRING("_rpPrtStdPClassCtor"));

    RWASSERT(pClass);

    memset(pClass, 0, sizeof(RpPrtStdParticleClass));

    pClass->next = pClass;
    pClass->id = (RwInt32) pClass;
    pClass->refCount = 1;

    RWRETURN(pClass);
}

RpPrtStdParticleClass *
_rpPrtStdPClassDtor(RpPrtStdParticleClass *pClass)
{
    RWFUNCTION(RWSTRING("_rpPrtStdPClassDtor"));

    RWASSERT(pClass);

    if (pClass->callback)
        RwFree(pClass->callback);

    if (pClass->propTab != NULL)
        RpPrtStdPropTabDestroy(pClass->propTab);

    memset(pClass, 0, sizeof(RpPrtStdParticleClass));

    RWRETURN(pClass);
}

/************************************************************************/

RpPrtStdParticleClass *
_rpPrtStdPClassAdd(RpPrtStdParticleClass *pClassHead,
                          RpPrtStdParticleClass *pClass)
{
    RWFUNCTION(RWSTRING("_rpPrtStdPClassAdd"));

    if (pClassHead == NULL)
    {
        pClass->next = pClass;
    }
    else
    {
        pClass->next = pClassHead->next;

        pClassHead->next = pClass;
    }

    RWRETURN(pClass);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPClassCreate is the function to create a new empty
 * particle class.
 *
 * \return a pointer to a particle class on success, NULL otherwise.
 *
 * \see RpPrtStdPClassDestroy
 * \see RpPrtStdPClassGetCallBack
 * \see RpPrtStdPClassSetCallBack
 * \see RpPrtStdPClassGetPropTab
 * \see RpPrtStdPClassSetPropTab
 * \see RpPrtStdPropTabCreate
 */
RpPrtStdParticleClass *
RpPrtStdPClassCreate( void )
{
    RpPrtStdParticleClass       *pClass;

    RWAPIFUNCTION(RWSTRING("RpPrtStdPClassCreate"));

    pClass = (RpPrtStdParticleClass *) RwMalloc(sizeof(RpPrtStdParticleClass),
                                      rwID_PRTSTDPLUGIN | rwMEMHINTDUR_EVENT);

    if (pClass)
    {
        pClass = _rpPrtStdPClassCtor(pClass);

        /* Add the class to the global list. */
        rpPrtStdGlobals.prtClass = _rpPrtStdPClassAdd(rpPrtStdGlobals.prtClass, pClass);
    }

    RWRETURN(pClass);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPClassDestroy is the function to destroys a
 * particle class.
 *
 * \param pClass A pointer to the particle class.
 *
 * \return TRUE on success, FALSE otherwise.
 *
 * \see RpPrtStdPClassDestroy
 * \see RpPrtStdPClassGetCallBack
 * \see RpPrtStdPClassSetCallBack
 * \see RpPrtStdPClassGetPropTab
 * \see RpPrtStdPClassSetPropTab
 * \see RpPrtStdPropTabCreate
 */
RwBool
RpPrtStdPClassDestroy(RpPrtStdParticleClass *pClass)
{
    RwBool                  result;
    RpPrtStdParticleClass   * pClassHead;
    RpPrtStdParticleClass   * pClassCurr;
    RpPrtStdParticleClass   * pClassPrev;

    RWAPIFUNCTION(RWSTRING("RpPrtStdPClassDestroy"));

    RWASSERT(pClass);

    /* Check the refCount before destroying */
    RWASSERT(pClass->refCount > 0);

    pClass->refCount--;
    if (pClass->refCount > 0)
        RWRETURN(TRUE);

    /* First remove pClass from the list */
    if (rpPrtStdGlobals.prtClass != NULL)
    {
        if ((rpPrtStdGlobals.prtClass == pClass) && (pClass->next == pClass))
        {
            rpPrtStdGlobals.prtClass = NULL;
        }
        else
        {
            pClassHead = rpPrtStdGlobals.prtClass->next;

            pClassCurr = pClassHead;
            pClassPrev = rpPrtStdGlobals.prtClass;

            do
            {
                if (pClassCurr == pClass)
                {
                    pClassPrev->next = pClass->next;

                    pClass->next = pClass;

                    break;
                }

                pClassPrev = pClassCurr;
                pClassCurr = pClassCurr->next;
            }
            while (pClassCurr != pClassHead);

            if (rpPrtStdGlobals.prtClass == pClass)
                rpPrtStdGlobals.prtClass = pClassPrev;
        }
    }

    result = FALSE;
    if (_rpPrtStdPClassDtor(pClass) == pClass)
    {
        RwFree(pClass);

        result = TRUE;
    }

    RWRETURN(result);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPClassSetCallBack is the function to assign a set of
 * callbacks to the particle class.
 *
 * These callbacks will be called for any particles created with this particle
 * class. This will replace the previous set of callbacks attached.
 *
 * A copy of the callback array will be made and attached to the particle class.
 * So the input callback array can be destroyed after use.
 *
 * \param pClass        A pointer to the particle class.
 * \param numCallback   A \ref RwInt32 for returning the number callback arrays.
 * \param prtCB         A pointer to a callback array to be attached.
 *
 * \return a pointer to a particle class on success, NULL otherwise.
 *
 * \see RpPrtStdPClassCreate
 * \see RpPrtStdPClassDestroy
 * \see RpPrtStdPClassGetCallBack
 * \see RpPrtStdPClassGetPropTab
 * \see RpPrtStdPClassSetPropTab
 * \see RpPrtStdPropTabCreate
 */
RpPrtStdParticleClass *
RpPrtStdPClassSetCallBack(RpPrtStdParticleClass *pClass,
                                 RwInt32 numCallback,
                                 RpPrtStdParticleCallBackArray *prtCB)
{
    RwInt32                                 i, j;
    RpPrtStdParticleClass                   *result;
    RpPrtStdParticleCallBackArray           *newPrtCB;

    RWAPIFUNCTION(RWSTRING("RpPrtStdPClassSetCallBack"));

    RWASSERT(pClass);

    result = NULL;

    RWASSERT(pClass);

    newPrtCB = (RpPrtStdParticleCallBackArray *)
        RwMalloc(numCallback * sizeof(RpPrtStdParticleCallBackArray),
                 rwID_PRTSTDPLUGIN | rwMEMHINTDUR_EVENT);

    if (newPrtCB != NULL)
    {
        for (i = 0; i < numCallback; i++)
        {
            for (j = 0; j < rpPRTSTDPARTICLECALLBACKMAX; j++)
            {
                newPrtCB[i][j] = prtCB[i][j];
            }
        }

        if (pClass->callback != NULL)
            RwFree(pClass->callback);

        pClass->numCallback = numCallback;
        pClass->callback = newPrtCB;

        result = pClass;
    }

    RWRETURN(result);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPClassGetCallBack is the function to return the set of
 * callbacks attached to the particle class.
 *
 * These callbacks will be called for any particles created with this particle
 * class. This will replace the previous set of callbacks attached.
 *
 * \param pClass        A pointer to the particle class.
 * \param numCallback   A pointer to a \ref RwInt32 for returning the number
 *                      of callbacks.
 * \param prtCB         A pointer to a \ref RpPrtStdParticleCallBackArray for
 *                      returning the callback arrays.
 *
 * \return a pointer to a particle class on success, NULL otherwise.
 *
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdPClassDestroy
 * \see RpPrtStdPClassSetCallBack
 * \see RpPrtStdPClassGetPropTab
 * \see RpPrtStdPClassSetPropTab
 * \see RpPrtStdPropTabCreate
 */
RpPrtStdParticleClass *
RpPrtStdPClassGetCallBack(RpPrtStdParticleClass *pClass,
                                 RwInt32 *numCallback,
                                 RpPrtStdParticleCallBackArray **prtCB)
{
    RWAPIFUNCTION(RWSTRING("RpPrtStdPClassGetCallBack"));

    RWASSERT(pClass);

    *numCallback = pClass->numCallback;
    *prtCB = pClass->callback;

    RWRETURN(pClass);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPClassSetPropTab is the function to assign a set of
 * properties to the particle class.
 *
 * The property table describes what information are stored in the particle and
 * their location. The particle class keeps a reference to the input property
 * table.
 *
 * \param pClass        A pointer to the particle class.
 * \param propTab       A pointer to the property table to be attached.
 *
 * \return a pointer to a particle class on success, NULL otherwise.
 *
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdPClassDestroy
 * \see RpPrtStdPClassGetCallBack
 * \see RpPrtStdPClassSetCallBack
 * \see RpPrtStdPClassGetPropTab
 * \see RpPrtStdPropTabCreate
 */
RpPrtStdParticleClass *
RpPrtStdPClassSetPropTab(RpPrtStdParticleClass *pClass,
                                      RpPrtStdPropertyTable *propTab)
{
    RwInt32 i,size;

    RWAPIFUNCTION(RWSTRING("RpPrtStdPClassSetPropTab"));

    RWASSERT(pClass);

    /* Increment and decrement the refCounts */
    if (pClass->propTab != NULL)
    {
        RpPrtStdPropTabDestroy(pClass->propTab);
    }
    propTab->refCount++;

    pClass->propTab = propTab;

    size = 0;

    if( propTab )
    {
        for (i=0;i<propTab->numProp;i++)
        {
            size += propTab->propSize[i];
        }
    }

    pClass->objSize = size;

    RWRETURN(pClass);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPClassGetPropTab is the function to return
 * the property table of a particle class.
 *
 * The property table describes what information are stored in the particle and
 * their location. The particle class keeps a reference to the input property
 * table.
 *
 * \param pClass        A pointer to the particle class.
 * \param propTab       A pointer to a pointer of a property table for returning
 *                      a reference to the particle class property table.
 *
 * \return a pointer to a particle class on success, NULL otherwise.
 *
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdPClassDestroy
 * \see RpPrtStdPClassGetCallBack
 * \see RpPrtStdPClassSetCallBack
 * \see RpPrtStdPClassSetPropTab
 * \see RpPrtStdPropTabCreate
 */

RpPrtStdParticleClass *
RpPrtStdPClassGetPropTab(RpPrtStdParticleClass *pClass,
                                      RpPrtStdPropertyTable **propTab)
{
    RWAPIFUNCTION(RWSTRING("RpPrtStdPClassGetPropTab"));

    RWASSERT(pClass);

    *propTab = pClass->propTab;

    RWRETURN(pClass);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPClassGet is the function to return the particle class by
 * its id number.
 *
 * Each particle class is given an identification number that must be unique.
 * This number is primary used to identify the particle class used by an emitter.
 *
 * \param id        A particle class id number to search for.
 *
 * \return a pointer to a particle class on successful, NULL otherwise.
 *
 * \see RpPrtStdPClassGetByPropTab
 */
RpPrtStdParticleClass *
RpPrtStdPClassGet(RwInt32 id)
{
    RpPrtStdParticleClass           *pClass, *pClassHead, *pClassFound;

    RWAPIFUNCTION(RWSTRING("RpPrtStdPClassGet"));

    pClassHead = rpPrtStdGlobals.prtClass;

    pClass = pClassHead;
    pClassFound = NULL;

    if (pClass != NULL)
    {
        /* Search the particle class list for an emitter with the same id. */
        do
        {
            if (pClass->id == id)
            {
                pClassFound = pClass;
                break;
            }

            pClass = pClass->next;
        }
        while (pClass != pClassHead);
    }

    RWRETURN(pClassFound);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPClassGetByPropTab is the function to return the particle
 * class by a property table.
 *
 * Each particle class has a reference to a property table. This property
 * table is used to defined the data present in the particle.
 *
 * \param propTab        A pointer to a property table..
 *
 * \return a pointer to a particle class on successful, NULL otherwise.
 *
 * \see RpPrtStdPClassGet
 */
RpPrtStdParticleClass *
RpPrtStdPClassGetByPropTab(RpPrtStdPropertyTable *propTab)
{
    RpPrtStdParticleClass           *pClass, *pClassHead, *pClassFound;

    RWAPIFUNCTION(RWSTRING("RpPrtStdPClassGetByPropTab"));

    RWASSERT(propTab);

    pClassHead = rpPrtStdGlobals.prtClass;

    pClass = pClassHead;
    pClassFound = NULL;

    if (pClass != NULL)
    {
        /* Search the particle class list for an emitter with the same property
         * table.
         */
        do
        {
            if (pClass->propTab == propTab)
            {
                pClassFound = pClass;
                break;
            }

            pClass = pClass->next;
        }
        while (pClass != pClassHead);
    }

    RWRETURN(pClassFound);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPClassStreamWrite is the function to stream out a particle class.
 *
 * The particles classes must be streamed out before any emitters. By default
 * the emitter class is streamed out with each emitter.
 * Using the function \ref RpPrtStdGlobalDataSetStreamEmbedded this behavior can be
 * changed, allowing to stream all the class and other needed data to one stream
 * and the emitter in another one.
 *
 * When the emitters are streamed in, if there is no emitter class streamed with
 * it, it will search for any emitter class with the same identification number.
 * If it fails, then the emitter will have a NULL emitter class.
 *
 * \param pClass        A pointer to the particle class.
 * \param stream        A pointer to the output stream.
 *
 * \return a pointer to a particle class on success, NULL otherwise.
 *
 * \see RpPrtStdEClassStreamRead
 * \see RpPrtStdEClassStreamWrite
 * \see RpPrtStdEClassStreamGetSize
 * \see RpPrtStdPClassCreate
 * \see RpPrtStdPClassDestroy
 * \see RpPrtStdPClassGet
 * \see RpPrtStdPClassGetCallBack
 * \see RpPrtStdPClassGetCallBack
 * \see RpPrtStdPClassStreamRead
 * \see RpPrtStdPClassStreamGetSize
 * \see RpPrtStdGlobalDataSetStreamEmbedded
 * \see RpPrtStdGlobalDataGetStreamEmbedded
 */
RpPrtStdParticleClass *
RpPrtStdPClassStreamWrite(RpPrtStdParticleClass *pClass, RwStream *stream)
{
    RwInt32                             size;

    RWAPIFUNCTION(RWSTRING("RpPrtStdPClassStreamWrite"));

    RWASSERT(pClass);
    RWASSERT(stream);

    /* Write header */
    size = RpPrtStdPClassStreamGetSize(pClass);
    if (!RwStreamWriteChunkHeader(stream, rwID_PRTSTDGLOBALDATA, size))
    {
        RWRETURN((RpPrtStdParticleClass *)NULL);
    }

    /* Stream out the class ID */
    RwStreamWriteInt32(stream, &pClass->id, sizeof(RwInt32));

    /* Stream out the class's prop table ID */
    RwStreamWriteInt32(stream, &pClass->propTab->id, sizeof(RwInt32));

    RWRETURN(pClass);
}


/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPClassStreamRead is the function to stream in a particle class.
 *
 * The particles classes must be streamed out before any emitters. By default
 * the emitter class is streamed out with each emitter.
 * Using the function \ref RpPrtStdGlobalDataSetStreamEmbedded this behavior can be
 * changed, allowing to stream all the class and other needed data to one stream
 * and the emitter in another one.
 *
 * When the emitters are streamed in, if there is no emitter class streamed with
 * it, it will search for any emitter class with the same identification number.
 * If it fails, then the emitter will have a NULL emitter class.
 *
 * The sequence to locate and read a particle class from a binary stream is
 * as follows:
 * \code
   RwStream *stream;
   RpPrtStdParticleClass *newParticleClass;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_PRTSTDGLOBALDATA, NULL, NULL) )
       {
           newParticleClass = RpPrtStdPClassStreamRead(stream);
       }

       RwStreamClose(stream, NULL);
   }
   \endcode
 *
 * \param stream        A pointer to the input stream.
 *
 * \return a pointer to a particle class on success, NULL otherwise.
 *
 * \see RpPrtStdEClassStreamRead
 * \see RpPrtStdEClassStreamWrite
 * \see RpPrtStdEClassStreamGetSize
 * \see RpPrtStdPClassCreate
 * \see RpPrtStdPClassDestroy
 * \see RpPrtStdPClassGet
 * \see RpPrtStdPClassGetCallBack
 * \see RpPrtStdPClassGetCallBack
 * \see RpPrtStdPClassStreamWrite
 * \see RpPrtStdPClassStreamGetSize
 * \see RpPrtStdGlobalDataSetStreamEmbedded
 * \see RpPrtStdGlobalDataGetStreamEmbedded
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RwStreamFindChunk
 * \see RpPrtStdPluginAttach
 */
RpPrtStdParticleClass *
RpPrtStdPClassStreamRead(RwStream *stream)
{
    RwInt32                             propTabID,pClassID;
    RpPrtStdParticleClass               *pClass;

    RWAPIFUNCTION(RWSTRING("RpPrtStdPClassStreamRead"));

    RWASSERT(stream);

    /* Stream in the class ID */
    RwStreamReadInt32(stream, &pClassID, sizeof(RwInt32));

    /* Stream in the class's prop table ID */
    RwStreamReadInt32(stream, &propTabID, sizeof(RwInt32));


    pClass = RpPrtStdPClassGet(pClassID);
    if (NULL == pClass)
    {
        /* Create the new particle class. */
        pClass = RpPrtStdPClassCreate();

        if (pClass)
        {
            RpPrtStdPClassSetPropTab(pClass, RpPrtStdPropTabGet(propTabID));

            RWASSERT(pClass->propTab);

            pClass->id = pClassID;
            pClass->numCallback = 0;
            pClass->callback = NULL;

            /* Call the setup callback. */
            (rpPrtStdGlobals.prtClassSetupCallback)(pClass);
        }
    }
    else
    {
        pClass->refCount++;
    }

    RWRETURN(pClass);
}


/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPClassStreamGetSize is used to determine the size in bytes of
 * the binary representation of a particle class. This value is used in the
 * binary chunk header to indicate the size of the chunk. The size includes
 * the size of the chunk header.
 *
 * \param pClass        A pointer to the particle class.
 *
 * \return Returns a \ref RwUInt32 value equal to the chunk size (in bytes)
 * of the particle class.
 *
 * \see RpPrtStdEClassStreamRead
 * \see RpPrtStdEClassStreamWrite
 * \see RpPrtStdEClassStreamGetSize
 * \see RpPrtStdPClassCreate
 * \see RpPrtStdPClassDestroy
 * \see RpPrtStdPClassGet
 * \see RpPrtStdPClassGetCallBack
 * \see RpPrtStdPClassGetCallBack
 * \see RpPrtStdPClassStreamWrite
 * \see RpPrtStdPClassStreamGetSize
 */
RwInt32
RpPrtStdPClassStreamGetSize(RpPrtStdParticleClass * pClass __RWUNUSED__)
{
    RwInt32                             size;

    RWAPIFUNCTION(RWSTRING("RpPrtStdPClassStreamGetSize"));

    RWASSERT(pClass);

    /* Particle class ID, propTab ID */
    size = 2 * sizeof(RwInt32);

    RWRETURN(size);
}

