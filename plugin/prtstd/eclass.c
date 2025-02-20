/**********************************************************************
 *
 * File :     eclass.c
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

#include <rpplugin.h>
#include <rpdbgerr.h>
#include <rpcriter.h>

#include "rpprtstd.h"

#include "eclass.h"
#include "emitter.h"
#include "prop.h"

#include "prtstdvars.h"


/************************************************************************
 *
 *
 ************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterDefaultCB is the default empty callback for the emitter
 * class.
 *
 * \param atomic Pointer to the parent atomic.
 * \param prtEmt Pointer to the emitter.
 * \param data   Pointer to private data
 *
 * \return a pointer to an emitter on success, NULL otherwise.
 *
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdEClassGetCallBack
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassGetPropTab
 * \see RpPrtStdEClassSetPropTab
 * \see RpPrtStdPropTabCreate
 */
RpPrtStdEmitter *
RpPrtStdEmitterDefaultCB(RpAtomic * atomic __RWUNUSED__,
                         RpPrtStdEmitter *prtEmt,
                         void * data __RWUNUSED__)
{
    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterDefaultCB"));

    RWRETURN(prtEmt);
}

/************************************************************************
 *
 *
 ************************************************************************/

RpPrtStdEmitterClass *
_rpPrtStdEClassCtor(RpPrtStdEmitterClass *eClass)
{
    RWFUNCTION(RWSTRING("_rpPrtStdEClassCtor"));

    RWASSERT(eClass);

    memset(eClass, 0, sizeof(RpPrtStdEmitterClass));

    eClass->next = eClass;
    eClass->id = (RwInt32) &eClass;
    eClass->refCount = 1;

    RWRETURN(eClass);
}

RpPrtStdEmitterClass *
_rpPrtStdEClassDtor(RpPrtStdEmitterClass *eClass)
{
    RWFUNCTION(RWSTRING("_rpPrtStdEClassDtor"));

    RWASSERT(eClass);

    if (eClass->callback != NULL)
        RwFree(eClass->callback);

    if (eClass->propTab != NULL)
        RpPrtStdPropTabDestroy(eClass->propTab);

    /* Clear the class. */
    memset(eClass, 0, sizeof(eClass));

    RWRETURN(eClass);
}

/************************************************************************/

RpPrtStdEmitterClass *
_rpPrtStdEClassAdd(RpPrtStdEmitterClass *eClassHead,
                         RpPrtStdEmitterClass *eClass)
{
    RWFUNCTION(RWSTRING("_rpPrtStdEClassAdd"));

    if (eClassHead == NULL)
    {
        eClass->next = eClass;
    }
    else
    {
        eClass->next = eClassHead->next;

        eClassHead->next = eClass;
    }

    RWRETURN(eClass);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEClassCreate is the function to create a new empty
 * emitter class.
 *
 * \return a pointer to an emitter class on success, NULL otherwise.
 *
 * \see RpPrtStdEClassDestroy
 * \see RpPrtStdEClassGetCallBack
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassGetPropTab
 * \see RpPrtStdEClassSetPropTab
 * \see RpPrtStdPropTabCreate
 */
RpPrtStdEmitterClass *
RpPrtStdEClassCreate( void )
{
    RpPrtStdEmitterClass       *eClass;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEClassCreate"));

    eClass = (RpPrtStdEmitterClass *) RwMalloc(sizeof(RpPrtStdEmitterClass),
                                   rwID_PRTSTDPLUGIN | rwMEMHINTDUR_EVENT);

    if (eClass)
    {
        eClass = _rpPrtStdEClassCtor(eClass);

        /* Add the class to the global list. */
        rpPrtStdGlobals.emtClass = _rpPrtStdEClassAdd(rpPrtStdGlobals.emtClass, eClass);
    }

    RWRETURN(eClass);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEClassDestroy is the function to destroys an
 * emitter class.
 *
 * \param eClass A pointer to the emitter class.
 *
 * \return TRUE on success, FALSE otherwise.
 *
 * \see RpPrtStdEClassDestroy
 * \see RpPrtStdEClassGetCallBack
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassGetPropTab
 * \see RpPrtStdEClassSetPropTab
 * \see RpPrtStdPropTabCreate
 */
RwBool
RpPrtStdEClassDestroy(RpPrtStdEmitterClass *eClass)
{
    RwBool                  result;
    RpPrtStdEmitterClass  * eClassHead;
    RpPrtStdEmitterClass  * eClassCurr;
    RpPrtStdEmitterClass  * eClassPrev;
#if (0)
    RpPrtStdEmitterClass  * eClassNext;
#endif /* (0) */

    RWAPIFUNCTION(RWSTRING("RpPrtStdEClassDestroy"));

    RWASSERT(eClass);

    /* Check the refCount before destroying */
    RWASSERT(eClass->refCount > 0);

    eClass->refCount--;
    if (eClass->refCount > 0)
        RWRETURN(TRUE);

    /* First remove emitter from the list */
    if (rpPrtStdGlobals.emtClass != NULL)
    {
        if ((rpPrtStdGlobals.emtClass == eClass) && (eClass->next == eClass))
        {
            rpPrtStdGlobals.emtClass = NULL;
        }
        else
        {
            eClassHead = rpPrtStdGlobals.emtClass->next;

            eClassCurr = eClassHead;
            eClassPrev = rpPrtStdGlobals.emtClass;

            do
            {
                if (eClassCurr == eClass)
                {
                    eClassPrev->next = eClass->next;

                    eClass->next = eClass;

                    break;
                }

                eClassPrev = eClassCurr;
                eClassCurr = eClassCurr->next;
            }
            while (eClassCurr != eClassHead);

            if (rpPrtStdGlobals.emtClass == eClass)
                rpPrtStdGlobals.emtClass = eClassPrev;
        }
    }

    result = FALSE;
    if (_rpPrtStdEClassDtor(eClass) == eClass)
    {
        RwFree(eClass);

        result = TRUE;
    }

    RWRETURN(result);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEClassSetCallBack is the function to assign a set of
 * callbacks to the emitter class.
 *
 * These callbacks will be called for any emitters created with this emitter
 * class. This will replace the previous set of callbacks attached.
 *
 * A copy of the callback array will be made and attached to the emitter class.
 * So the input callback array can be destroyed after use.
 *
 * \param eClass        A pointer to the emitter class.
 * \param numCallback   A \ref RwInt32 for returning the number callback arrays.
 * \param emtCB         A pointer to a callback array to be attached.
 *
 * \return a pointer to an emitter class on success, NULL otherwise.
 *
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdEClassDestroy
 * \see RpPrtStdEClassGetCallBack
 * \see RpPrtStdEClassGetPropTab
 * \see RpPrtStdEClassSetPropTab
 * \see RpPrtStdPropTabCreate
 */
RpPrtStdEmitterClass *
RpPrtStdEClassSetCallBack(RpPrtStdEmitterClass *eClass,
                                RwInt32 numCallback,
                                RpPrtStdEmitterCallBackArray *emtCB)
{
    RwInt32                             i, j;
    RpPrtStdEmitterClass                *result;
    RpPrtStdEmitterCallBackArray        *newEmtCB;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEClassSetCallBack"));

    result = NULL;

    RWASSERT(eClass);

    newEmtCB = (RpPrtStdEmitterCallBackArray *)
        RwMalloc(numCallback * sizeof(RpPrtStdEmitterCallBackArray),
                 rwID_PRTSTDPLUGIN | rwMEMHINTDUR_EVENT);

    if (newEmtCB != NULL)
    {
        for (i = 0; i < numCallback; i++)
        {
            for (j = 0; j < rpPRTSTDEMITTERCALLBACKMAX; j++)
            {
                newEmtCB[i][j] = emtCB[i][j];
            }
        }

        if (eClass->callback != NULL)
            RwFree(eClass->callback);

        eClass->numCallback = numCallback;
        eClass->callback = newEmtCB;

        result = eClass;
    }

    RWRETURN(result);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEClassGetCallBack is the function to return the set of
 * callbacks attached to the emitter class.
 *
 * These callbacks will be called for any emitters created with this emitter
 * class. This will replace the previous set of callbacks attached.
 *
 * \param eClass        A pointer to the emitter class.
 * \param numCallback   A pointer to a \ref RwInt32 for returning the number
 *                      of callbacks.
 * \param emtCB         A pointer to a \ref RpPrtStdEmitterCallBackArray for
 *                      returning the callback arrays.
 *
 * \return a pointer to an emitter class on success, NULL otherwise.
 *
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdEClassDestroy
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassGetPropTab
 * \see RpPrtStdEClassSetPropTab
 * \see RpPrtStdPropTabCreate
 */
RpPrtStdEmitterClass *
RpPrtStdEClassGetCallBack(RpPrtStdEmitterClass *eClass,
                                RwInt32 *numCallback,
                                RpPrtStdEmitterCallBackArray **emtCB)
{
    RWAPIFUNCTION(RWSTRING("RpPrtStdEClassGetCallBack"));

    RWASSERT(eClass);

    *numCallback = eClass->numCallback;
    *emtCB = eClass->callback;

    RWRETURN(eClass);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEClassSetPropTab is the function to assign a set of
 * properties to the emitter class.
 *
 * The property table describes what information are stored in the emitter and
 * their location. The emitter class keeps a reference to the input property
 * table.
 *
 * \param eClass        A pointer to the emitter class.
 * \param propTab       A pointer to the property table to be attached.
 *
 * \return a pointer to an emitter class on success, NULL otherwise.
 *
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdEClassDestroy
 * \see RpPrtStdEClassGetCallBack
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassGetPropTab
 * \see RpPrtStdPropTabCreate
 */
RpPrtStdEmitterClass *
RpPrtStdEClassSetPropTab(RpPrtStdEmitterClass *eClass, RpPrtStdPropertyTable *propTab)
{
    RwInt32 i,size;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEClassSetPropTab"));

    RWASSERT(eClass);

    /* Increment and decrement the refCounts */
    if (eClass->propTab != NULL)
    {
        RpPrtStdPropTabDestroy(eClass->propTab);
    }
    propTab->refCount++;

    eClass->propTab = propTab;

    size = 0;

    if( propTab )
    {
        for (i=0;i<propTab->numProp;i++)
        {
            size += propTab->propSize[i];
        }
    }

    eClass->objSize = size;

    RWRETURN(eClass);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEClassGetPropTab is the function to return
 * the property table of emitter class.
 *
 * The property table describes what information are stored in the emitter and
 * their location. The emitter class keeps a reference to the input property
 * table.
 *
 * \param eClass        A pointer to the emitter class.
 * \param propTab       A pointer to a pointer the property table for returning
 *                      a reference to the property table.
 *
 * \return a pointer to an emitter class on success, NULL otherwise.
 *
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdEClassDestroy
 * \see RpPrtStdEClassGetCallBack
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassSetPropTab
 * \see RpPrtStdPropTabCreate
 */
RpPrtStdEmitterClass *
RpPrtStdEClassGetPropTab(RpPrtStdEmitterClass *eClass, RpPrtStdPropertyTable **propTab)
{
    RWAPIFUNCTION(RWSTRING("RpPrtStdEClassGetPropTab"));

    RWASSERT(eClass);

    *propTab = eClass->propTab;

    RWRETURN(eClass);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEClassGet is the function to return the emitter class by
 * its id number.
 *
 * Each emitter class is given an identification number that must be unique.
 * This number is primary used to identify the emitter class used by an emitter.
 *
 * \param id        An emitter class id number to search for.
 *
 * \return a pointer to an emitter class on successful, NULL otherwise.
 *
 * \see RpPrtStdEClassGetByPropTab
 */
RpPrtStdEmitterClass *
RpPrtStdEClassGet(RwInt32 id)
{
    RpPrtStdEmitterClass            *eClass, *eClassHead, *eClassFound;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEClassGet"));

    eClassHead = rpPrtStdGlobals.emtClass;

    eClass = eClassHead;
    eClassFound = NULL;

    if (eClass != NULL)
    {
        /* Search the emitter class list for an emitter with the same id. */
        do
        {
            if (eClass->id == id)
            {
                eClassFound = eClass;
                break;
            }

            eClass = eClass->next;
        }
        while (eClass != eClassHead);
    }

    RWRETURN(eClassFound);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEClassGetByPropTab is the function to return the emitter class
 * by the property table.
 *
 * Each emitter class has a reference to a property table. This property
 * table is used to defined the data present in the emitter.
 *
 * \param propTab           A pointer to the property table.
 *
 * \return a pointer to an emitter class on successful, NULL otherwise.
 *
 * \see RpPrtStdEClassGet
 */
RpPrtStdEmitterClass *
RpPrtStdEClassGetByPropTab(RpPrtStdPropertyTable *propTab)
{
    RpPrtStdEmitterClass            *eClass, *eClassHead, *eClassFound;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEClassGetByPropTab"));

    RWASSERT(propTab);

    eClassHead = rpPrtStdGlobals.emtClass;

    eClass = eClassHead;
    eClassFound = NULL;

    if (eClass != NULL)
    {
        /* Search the emitter class list for an emitter with the property table. */
        do
        {
            if (eClass->propTab == propTab)
            {
                eClassFound = eClass;
                break;
            }

            eClass = eClass->next;
        }
        while (eClass != eClassHead);
    }

    RWRETURN(eClassFound);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEClassStreamWrite is the function to stream out an emitter class.
 *
 * The emitter classes must be streamed out before any emitters. By default the
 * emitter class is streamed out with each emitter.
 * Using the function \ref RpPrtStdGlobalDataSetStreamEmbedded this behavior can be
 * changed, allowing to stream all the class and other needed data to one stream
 * and the emitter in another one.
 *
 * When the emitters are streamed in, if there is no emitter class streamed with
 * it, it will search for any emitter class with the same identification number.
 * If it fails, then the emitter will have a NULL emitter class.
 *
 * \param eClass        A pointer to the emitter class.
 * \param stream        A pointer to the output stream.
 *
 * \return a pointer to an emitter class on success, NULL otherwise.
 *
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdEClassDestroy
 * \see RpPrtStdEClassGet
 * \see RpPrtStdEClassGetCallBack
 * \see RpPrtStdEClassGetCallBack
 * \see RpPrtStdEClassStreamRead
 * \see RpPrtStdEClassStreamGetSize
 * \see RpPrtStdPClassStreamRead
 * \see RpPrtStdPClassStreamWrite
 * \see RpPrtStdPClassStreamGetSize
 * \see RpPrtStdGlobalDataSetStreamEmbedded
 * \see RpPrtStdGlobalDataGetStreamEmbedded
 */
RpPrtStdEmitterClass *
RpPrtStdEClassStreamWrite(RpPrtStdEmitterClass *eClass, RwStream *stream)
{
    RwInt32             size;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEClassStreamWrite"));

    RWASSERT(eClass);
    RWASSERT(stream);

    /* Write header */
    size = RpPrtStdEClassStreamGetSize(eClass);
    if (!RwStreamWriteChunkHeader(stream, rwID_PRTSTDGLOBALDATA, size))
    {
        RWRETURN((RpPrtStdEmitterClass *)NULL);
    }

    /* Stream out the class ID */
    RwStreamWriteInt32(stream, &eClass->id, sizeof(RwInt32));

    /* Stream out the class's prop table ID */
    RwStreamWriteInt32(stream, &eClass->propTab->id, sizeof(RwInt32));

    RWRETURN(eClass);
}


/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEClassStreamRead is the function to stream in an emitter class.
 *
 * The emitter classes must be streamed out before any emitters. By default the
 * emitter class is streamed out with each emitter.
 * Using the function \ref RpPrtStdGlobalDataSetStreamEmbedded this behavior can be
 * changed, allowing to stream all the class and other needed data to one stream
 * and the emitter in another one.
 *
 * When the emitters are streamed in, if there is no emitter class streamed with
 * it, it will search for any emitter class with the same identification number.
 * If it fails, then the emitter will have a NULL emitter class.
 *
 * The sequence to locate and read a emitter class from a binary stream is
 * as follows:
 * \code
   RwStream *stream;
   RpPrtStdEmitterClass *newEmitterClass;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_PRTSTDGLOBALDATA, NULL, NULL) )
       {
           newEmitterClass = RpPrtStdEClassStreamRead(stream);
       }

       RwStreamClose(stream, NULL);
   }
   \endcode
 *
 * \param stream        A pointer to the input stream.
 *
 * \return a pointer to an emitter class on success, NULL otherwise.
 *
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdEClassDestroy
 * \see RpPrtStdEClassGet
 * \see RpPrtStdEClassGetCallBack
 * \see RpPrtStdEClassGetCallBack
 * \see RpPrtStdEClassStreamWrite
 * \see RpPrtStdEClassStreamGetSize
 * \see RpPrtStdPClassStreamRead
 * \see RpPrtStdPClassStreamWrite
 * \see RpPrtStdPClassStreamGetSize
 * \see RpPrtStdGlobalDataSetStreamEmbedded
 * \see RpPrtStdGlobalDataGetStreamEmbedded
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RwStreamFindChunk
 * \see RpPrtStdPluginAttach
 */
RpPrtStdEmitterClass *
RpPrtStdEClassStreamRead(RwStream *stream)
{
    RwInt32                             propTabID,eClassID;
    RpPrtStdEmitterClass                *eClass;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEClassStreamRead"));

    RWASSERT(stream);

    /* Create the new emitter class. */
    /* Stream in the class ID */
    RwStreamReadInt32(stream, &eClassID, sizeof(RwInt32));

    /* Stream in the class's prop table ID */
    RwStreamReadInt32(stream, &propTabID, sizeof(RwInt32));

    eClass = RpPrtStdEClassGet(eClassID);
    if (NULL == eClass)
    {
        eClass = RpPrtStdEClassCreate();

        if( eClass )
        {
            RpPrtStdEClassSetPropTab(eClass, RpPrtStdPropTabGet(propTabID));

            RWASSERT(eClass->propTab);

            eClass->id = eClassID;
            eClass->numCallback = 0;
            eClass->callback = NULL;

            /* Call the setup callback. This needs to restores the render callbacks. */
            (rpPrtStdGlobals.emtClassSetupCallback)(eClass);
        }
    }
    else
    {
        eClass->refCount++;
    }

    RWRETURN(eClass);
}


/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEClassStreamGetSize is used to determine the size in bytes of
 * the binary representation of an emitter class. This value is used in the
 * binary chunk header to indicate the size of the chunk. The size includes
 * the size of the chunk header.
 *
 * \param eClass        A pointer to the emitter class.
 *
 * \return Returns a \ref RwInt32 value equal to the chunk size (in bytes)
 * of the emitter class.
 *
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdEClassDestroy
 * \see RpPrtStdEClassGet
 * \see RpPrtStdEClassGetCallBack
 * \see RpPrtStdEClassGetCallBack
 * \see RpPrtStdEClassStreamWrite
 * \see RpPrtStdEClassStreamGetSize
 * \see RpPrtStdPClassStreamRead
 * \see RpPrtStdPClassStreamWrite
 * \see RpPrtStdPClassStreamGetSize
 */
RwInt32
RpPrtStdEClassStreamGetSize(RpPrtStdEmitterClass * eClass __RWUNUSED__)
{
    RwInt32                             size;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEClassStreamGetSize"));

    RWASSERT(eClass);

    /* Emitter class ID, propTab ID */
    size = 2 * sizeof(RwInt32);

    RWRETURN(size);
}


