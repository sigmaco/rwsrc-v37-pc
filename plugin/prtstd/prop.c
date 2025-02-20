/**********************************************************************
 *
 * File :       prop.c
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

#include "rpprtstd.h"

#include "prtstdvars.h"

#include "prop.h"
#include "standard.h"

/************************************************************************
 *
 *
 ************************************************************************/


/*
 * Post prop table read to re-adjust some property sizes.
 * Some property sizes were changed from previous versions and this can
 * cause memory problem.
 */
static RpPrtStdPropertyTable *
rpPrtStdPropTabPostRead( RpPrtStdPropertyTable * propTab )
{
    RwInt32                         numProp, size, sizeUA, oldSize, offset, i;

    RWFUNCTION(RWSTRING("rpPrtStdPropTabPostRead"));

    RWASSERT(propTab);

    /* Re-adjust some property sizes, only if it was from an older version
     * of the file.
     */
    if (_rpPrtStdStreamGetVersion() < rpPRTSTDVERSIONSTAMP)
    {
        numProp = propTab->numProp;
        offset = propTab->propSize[0];

        /* Naughty code alert.
         *
         * There isn't a way to distinguish if the property table is for
         * an emitter or a particle.
         *
         * So we look at the first property.
         * If the size matches that of the RpPrtStdParticleStandard, assume
         * it is a particle property table, otherwise it is an emitter
         * property table.
         */
        if (propTab->propSize[0] != sizeof(RpPrtStdParticleStandard))
        {
            /*
             * Assume this is emitter property table.
             */
            for (i = 1; i < numProp; i++)
            {
                oldSize = propTab->propSizeUA[i];

                if (propTab->propID[i] == rpPRTSTDPROPERTYCODEEMITTERSTANDARD)
                {
                    sizeUA = oldSize -
                            sizeof(RpPrtStdEmitterStandardOld) +
                            sizeof(RpPrtStdEmitterStandard);

                    size = (sizeUA + (rwMATRIXALIGNMENT-1)) & ~(rwMATRIXALIGNMENT-1);
                }
                else
                {
                    sizeUA = propTab->propSizeUA[i];
                    size = propTab->propSize[i];
                }

                propTab->propSizeUA[i] = sizeUA;
                propTab->propSize[i] = size;
                propTab->propOffset[i] = offset;

                offset += propTab->propSize[i];
            }

            /* Write the last offset */
            propTab->propOffset[i] = offset;
        }
    }

    RWRETURN(propTab);
}

/************************************************************************/

RpPrtStdPropertyTable *
_rpPrtStdPropTabAdd(RpPrtStdPropertyTable *propTabHead,
                          RpPrtStdPropertyTable *propTab)
{
    RWFUNCTION(RWSTRING("_rpPrtStdPropTabAdd"));

    if (propTabHead == NULL)
    {
        propTab->next = propTab;
    }
    else
    {
        propTab->next = propTabHead->next;

        propTabHead->next = propTab;
    }

    RWRETURN(propTab);
}

/************************************************************************/

RwBool
_rpPrtStdPropTabCompareProp(RpPrtStdPropertyTable *propTab,
                            RwInt32 numProp, RwInt32 *propID, RwInt32 *propStride)
{
    RwInt32                     i, j;
    RwBool                      match;

    RWFUNCTION(RWSTRING("_rpPrtStdPropTabCompareProp"));

    RWASSERT(propTab);
    RWASSERT(numProp > 0);
    RWASSERT(propID);
    RWASSERT(propStride);

    /* Assume the prop matches. */
    match = TRUE;

    /* This prop matches the num of prop. Now check the props themselves. */
    if (propTab->numProp == numProp)
    {
        i = 0;
        while ((match) && (i < numProp))
        {
            j = 0;
            while ((match) && (j < propTab->numProp))
            {
                if (propID[i] == propTab->propID[j])
                {
                    if (propStride[i] != propTab->propSize[j])
                    {
                        /* ID match but size does not. Prop tab does not match.
                         * No need to check the other props.
                         */
                        match = FALSE;
                    }
                    else
                    {
                        /* ID and size match. Exit this loop but continue to
                         * the next prop.
                         */
                        break;
                    }
                }

                j++;
            }

            /* Prop not found. Prop tad does not match. No need to check
             * the other props.
             */
            if (j >= propTab->numProp)
            {
                match = FALSE;
            }

            i++;
        }
    }

    RWRETURN(match);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPropTabCreate is the function to create a property
 * table.
 *
 * A property table is used to describes the contents in an emitter or
 * particle and their position. This allows emitters and particles to
 * be customizable to specific requirement.
 *
 * Each property must have a unique ID and cannot appear more than once
 * in a property table. Otherwise only the first appearance will be used.
 *
 * \param numProp       An \ref RwInt32 for the number of properties.
 * \param propID        A pointer to an array of property's ID.
 * \param propStride    A pointer to an array of property's size.
 *
 * \return a pointer to a property table on success, NULL otherwise.
 *
 * \see RpPrtStdPropTabAppend
 * \see RpPrtStdPropTabDestroy
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdEClassGetPropTab
 * \see RpPrtStdEClassSetPropTab
 * \see RpPrtStdPClassCreate
 * \see RpPrtStdPClassGetPropTab
 * \see RpPrtStdPClassSetPropTab
 */
RpPrtStdPropertyTable *
RpPrtStdPropTabCreate(RwInt32 numProp, RwInt32 *propID, RwInt32 *propStride)
{
    RwInt32                         *propSize, size, *propOffset, offset, i;
    RwInt32                         *propSizeUA;
    RwInt32                         *propAlignedSize, *pas;
    RwInt32                          propTabID;
    RwUInt32                        *propTabMem;
    RpPrtStdPropertyTable           *propTab;

    RWAPIFUNCTION(RWSTRING("RpPrtStdPropTabCreate"));

    propTab = NULL;

    RWASSERT(propID);
    RWASSERT(propStride);

    /* Check and resize the scratch mem */
    size = numProp * sizeof(RwInt32);

    if (size > rpPrtStdGlobals.propTabScratchMemSize)
    {
        pas = RwRealloc(rpPrtStdGlobals.propTabScratchMem, size,
            rwID_PRTSTDPLUGIN | rwMEMHINTDUR_GLOBAL |
            rwMEMHINTFLAG_RESIZABLE);

        if (NULL == pas)
        {
            RWRETURN((RpPrtStdPropertyTable*)NULL);
        }

        rpPrtStdGlobals.propTabScratchMem = (RwChar *) pas;
        rpPrtStdGlobals.propTabScratchMemSize = size;
    }
    else
        pas = (RwInt32 *) rpPrtStdGlobals.propTabScratchMem;

    propAlignedSize = pas;

    if( NULL == propAlignedSize )
    {
        RWRETURN((RpPrtStdPropertyTable*)NULL);
    }

    propTabID = 0;
    for (i = 0; i < numProp; i++)
    {
        propAlignedSize[i] = (propStride[i] + (rwMATRIXALIGNMENT-1)) & ~(rwMATRIXALIGNMENT-1);
        propTabID += propID[i];
    }

    /* Calculate the size of the arrays + the structures. */
    size = sizeof(RpPrtStdPropertyTable) +
           (((numProp + 1) * 4) * sizeof(RwInt32)) + (rwMATRIXALIGNMENT-1);

    propTabID = ( (propTabID << 10) |
                  ((RwUInt32) (((numProp + 1) * 4) * sizeof(RwInt32)) & 0x0000FFFF) |
                  (propTabID >> 10)
                );

    /* Search for a prop tab with the same property set up */
    propTab = RpPrtStdPropTabGetByProperties(numProp, propID, propStride);

    /* propTab = RpPrtStdPropTabGet(propTabID);  */

    if( NULL == propTab )
    {
        propTabMem =  (RwUInt32 *)RwMalloc(size,
            rwID_PRTSTDPLUGIN | rwMEMHINTDUR_EVENT);

        if (propTabMem != NULL)
        {
            propTab = (RpPrtStdPropertyTable *)
                (((RwUInt32)propTabMem + (rwMATRIXALIGNMENT-1)) & ~((rwMATRIXALIGNMENT-1)));

            propTab->propPtr = propTabMem;

            propTab->refCount = 1;

            propTab->propID = (RwInt32 *) (((RwChar *)propTab) + sizeof(RpPrtStdPropertyTable));
            propTab->propOffset = &propTab->propID[numProp + 1];
            propTab->propSize = &propTab->propOffset[numProp + 1];
            propTab->propSizeUA = &propTab->propSize[numProp + 1];

            memcpy(propTab->propID, propID, numProp * sizeof(RwInt32));

            propOffset = propTab->propOffset;
            propSize = propTab->propSize;
            propSizeUA = propTab->propSizeUA;
            offset = 0;

            for (i = 0; i < numProp; i++)
            {
                *propOffset = offset;
                *propSizeUA = *propStride;
                *propSize = *propAlignedSize;

                RWASSERT((*propStride) > 0);
                offset += *propAlignedSize;

                propOffset++;
                propSize++;
                propSizeUA++;

                propStride++;
                propAlignedSize++;
            }

            /* Write the last offset. */
            *propOffset = offset;

            propTab->numProp = numProp;
            propTab->next = propTab;
            propTab->id = propTabID;

            /* Add to the global list. */
            rpPrtStdGlobals.propTab = _rpPrtStdPropTabAdd(rpPrtStdGlobals.propTab, propTab);
        }
    }
    else
    {
        propTab->refCount++;
    }

    RWRETURN(propTab);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPropTabDestroy is the function to destroy a property
 * table.
 *
 * A property table is used to describes the contents in an emitter or
 * particle and their position. This allows emitters and particles to
 * be customizable to specific requirement.
 *
 * Property tables are shared and referenced by emitter and particle classes.
 * Care must taken to ensure no classes references the property table before
 * it is destroyed.
 *
 * \param propTab       A pointer to a property table.
 *
 * \return TRUE on success, FALSE otherwise.
 *
 * \see RpPrtStdPropTabAppend
 * \see RpPrtStdPropTabCreate
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdEClassGetPropTab
 * \see RpPrtStdEClassSetPropTab
 * \see RpPrtStdPClassCreate
 * \see RpPrtStdPClassGetPropTab
 * \see RpPrtStdPClassSetPropTab
 */
RwBool
RpPrtStdPropTabDestroy(RpPrtStdPropertyTable *propTab)
{
    RpPrtStdPropertyTable               *propTabCurr, *propTabPrev, *propTabHead;

    RWAPIFUNCTION(RWSTRING("RpPrtStdPropTabDestroy"));

    if (propTab == NULL)
        RWRETURN(FALSE);

    /* Check the refCount before destroying */
    RWASSERT(propTab->refCount > 0);

    propTab->refCount--;
    if (propTab->refCount > 0)
        RWRETURN(TRUE);

    /* First remove from the list */
    if (rpPrtStdGlobals.propTab != NULL)
    {
        if ((rpPrtStdGlobals.propTab == propTab) &&
            (propTab->next == propTab))
        {
            rpPrtStdGlobals.propTab = NULL;
        }
        else
        {
            propTabHead = rpPrtStdGlobals.propTab->next;

            propTabCurr = propTabHead;
            propTabPrev = rpPrtStdGlobals.propTab;

            do
            {
                if (propTabCurr == propTab)
                {
                    propTabPrev->next = propTab->next;

                    propTab->next = propTab;

                    break;
                }

                propTabPrev = propTabCurr;
                propTabCurr = propTabCurr->next;
            }
            while (propTabCurr != propTabHead);

            if (rpPrtStdGlobals.propTab == propTab)
                rpPrtStdGlobals.propTab = propTabPrev;
        }
    }

    /* Need to reset the cache prop tab ptr if it is the one being destroyed. */
    if (rpPrtStdGlobals.prtPropTabCurr == propTab)
        rpPrtStdGlobals.prtPropTabCurr = NULL;
    else if (rpPrtStdGlobals.emtPropTabCurr == propTab)
        rpPrtStdGlobals.emtPropTabCurr = NULL;

    /* Notify anyone that this is prop tab is about to be destroyed. */
    if (rpPrtStdGlobals.propTabDtorCallBack)
        (rpPrtStdGlobals.propTabDtorCallBack)(propTab);

    RwFree(propTab->propPtr);

    RWRETURN(TRUE);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPropTabAppend adds additional properties
 * to an existing property table, creating a new property table. The new
 * properties are added to the end of the existing properties. Failure will
 * occur if any additional properties already exist in the given property
 * table.
 *
 * A property table is used to describe the contents in an emitter or
 * particle and their position. This allows emitters and particles to
 * be customizable to specific requirements.
 *
 * Property tables are shared and referenced by emitter and particle classes.
 * Care must taken to ensure no classes references the property table before
 * it is destroyed.
 *
 * \param propTab       A pointer to a property table.
 * \param numProp       An \ref RwInt32 for the number of properties.
 * \param propID        A pointer to an array of property's ID.
 * \param propStride    A pointer to an array of property's size.
 *
 * \return a pointer to a property table on success, NULL otherwise.
 *
 * \see RpPrtStdPropTabCreate
 * \see RpPrtStdPropTabDestroy
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdEClassGetPropTab
 * \see RpPrtStdEClassSetPropTab
 * \see RpPrtStdPClassCreate
 * \see RpPrtStdPClassGetPropTab
 * \see RpPrtStdPClassSetPropTab
 */
RpPrtStdPropertyTable *
RpPrtStdPropTabAppend(RpPrtStdPropertyTable *propTab,
                       RwInt32 numProp, RwInt32 *propID, RwInt32 *propStride)
{
    RpPrtStdPropertyTable *propTabNew;
    RwInt32         i, j, numPropNew, *propIDNew, *propStrideNew;
    RwBool          valid;


    RWAPIFUNCTION(RWSTRING("RpPrtStdPropTabAppend"));

    RWASSERT(propTab);
    RWASSERT(numProp > 0);
    RWASSERT(propID);
    RWASSERT(propStride);

    propIDNew = NULL;
    propTabNew = NULL;

    valid = TRUE;

    /* Check none of the props are duplicated. */
    for (i = 0; i < numProp; i++)
    {
        for (j = 0; j < propTab->numProp; j++)
        {
            if (propTab->propID[j] == propID[i])
            {
#if (defined(RWDEBUG))
                RwChar      msg[40];

                /* Found a duplicate, warn and exit. */
                rwsprintf(msg, RWSTRING("Duplicate property [%d] found."), propID[i]);

                RWMESSAGE((msg));
#endif /* (define(RWDEBUG)) */

                valid = FALSE;
            }
        }
    }

    if (TRUE == valid)
    {
        numPropNew = propTab->numProp + numProp;
        propIDNew = (RwInt32 *) RwMalloc(2 * numPropNew * sizeof(RwInt32),
                              rwID_PRTSTDPLUGIN | rwMEMHINTDUR_FUNCTION);

        if (NULL != propIDNew)
        {
            propStrideNew = (RwInt32 *) &propIDNew[numPropNew];

            /* Copy the props from the prop table. */
            for (i = 0; i < propTab->numProp; i++)
            {
                propIDNew[i] = propTab->propID[i];
                propStrideNew[i] = propTab->propSize[i];
            }

            /* Copy the new props. */
            for (i = 0, j = propTab->numProp; i < numProp; i++, j++)
            {
                propIDNew[j] = propID[i];
                propStrideNew[j] = propStride[i];
            }

            propTabNew = RpPrtStdPropTabCreate(numPropNew, propIDNew, propStrideNew);
        }
    }

    if (NULL != propIDNew)
        RwFree(propIDNew);

    RWRETURN(propTabNew);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPropTabGetProperties is the function to return the properties's
 * details in a property table.
 *
 * A property table is used to describes the contents in an emitter or
 * particle and their position. This allows emitters and particles to
 * be customizable to specific requirement.
 *
 * \param propTab       A pointer to the property table.
 * \param numProp       A pointer for returning the number of properties.
 * \param propID        A pointer for returning the array of properties's ID.
 * \param propOffset    A pointer for returning the array of properties's offset.
 * \param propSize      A pointer for returning the array of properties's size.
 *
 * \return TRUE on success, FALSE otherwise.
 *
 * \see RpPrtStdPropTabCreate
 * \see RpPrtStdPropTabDestroy
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdEClassGetPropTab
 * \see RpPrtStdEClassSetPropTab
 * \see RpPrtStdPClassCreate
 * \see RpPrtStdPClassGetPropTab
 * \see RpPrtStdPClassSetPropTab
 */
RpPrtStdPropertyTable *
RpPrtStdPropTabGetProperties(RpPrtStdPropertyTable *propTab,
                                    RwInt32 *numProp,
                                    RwInt32 **propID,
                                    RwInt32 **propOffset,
                                    RwInt32 **propSize)
{
    RWAPIFUNCTION(RWSTRING("RpPrtStdPropTabGetProperties"));

    RWASSERT(propTab);

    if (numProp != NULL)
        *numProp = propTab->numProp;

    if (propID != NULL)
        *propID = propTab->propID;

    if (propOffset != NULL)
        *propOffset = propTab->propOffset;

    if (propSize)
        *propSize = propTab->propSize;

    RWRETURN(propTab);
}


/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPropTabGetPropIndex is the function to get the index
 * of a property in a  property table.
 *
 * A property table is used to describes the contents in an emitter or
 * particle and their position. This allows emitters and particles to
 * be customizable to specific requirement.
 *
 * If the property exist in the property table, a valid index is return,
 * otherwise -1.
 *
 * \param propTab       A pointer to the property table.
 * \param propID        The property's ID.
 *
 * \return a valid index on success, -1 otherwise.
 *
 * \see RpPrtStdPropTabCreate
 * \see RpPrtStdPropTabDestroy
 * \see RpPrtStdPropTabGetPropOffset
 */
RwInt32
RpPrtStdPropTabGetPropIndex(RpPrtStdPropertyTable *propTab, RwInt32 propID)
{
    RwInt32     i, index, found;

    RWAPIFUNCTION(RWSTRING("RpPrtStdPropTabGetPropIndex"));

    RWASSERT(propTab);

    index = -1;
    found = FALSE;

    i = 0;
    while (!found && (i < propTab->numProp))
    {
        if (propTab->propID[i] == propID)
        {
            found = TRUE;

            index = i;
        }
        i++;
    }

    RWRETURN(index);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPropTabGetPropOffset is the function to return the
 * offset of a property in an emitter or particle.
 *
 * A property table is used to describes the contents in an emitter or
 * particle and their position. This allows emitters and particles to
 * be customizable to specific requirement.
 *
 * If the property exist in the property table, a valid offset is is return,
 * otherwise -1.
 *
 * This offset is used to reference property's data in emitter or particle.
 *
 * \param propTab       A pointer to the property table.
 * \param propID        The property's ID.
 *
 * \return a valid offset on success, -1 otherwise.
 *
 * \see RpPrtStdPropTabCreate
 * \see RpPrtStdPropTabDestroy
 * \see RpPrtStdPropTabGetPropIndex
 */
RwInt32
RpPrtStdPropTabGetPropOffset(RpPrtStdPropertyTable *propTab, RwInt32 propID)
{
    RwInt32     index, offset;

    RWAPIFUNCTION(RWSTRING("RpPrtStdPropTabGetPropOffset"));

    RWASSERT(propTab);

    index = RpPrtStdPropTabGetPropIndex(propTab, propID);

    if (index >= 0)
    {
        offset = propTab->propOffset[index];
    }
    else
        offset = -1;

    RWRETURN(offset);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPropTabGet is the function to return the property table by
 * its id number.
 *
 * Each property table is given an identification number that must be unique.
 * This number is used to identify the property table used by an
 * emitter or particle class.
 *
 * \param id        A property table id number to search for.
 *
 * \return a pointer to a property table on successful, NULL otherwise.
 *
 * \see RpPrtStdPropTabGetByProperties
 */
RpPrtStdPropertyTable *
RpPrtStdPropTabGet(RwInt32 id)
{
    RpPrtStdPropertyTable       *propTab, *propTabHead, *propTabFound;

    RWAPIFUNCTION(RWSTRING("RpPrtStdPropTabGet"));

    propTabHead = rpPrtStdGlobals.propTab;

    propTab = propTabHead;
    propTabFound = NULL;

    if (propTab != NULL)
    {
        /* Search the emitter class list for an emitter with the same id. */
        do
        {
            if (propTab->id == id)
            {
                propTabFound = propTab;
                break;
            }

            propTab = propTab->next;
        }
        while (propTab != propTabHead);
    }

    RWRETURN(propTabFound);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPropTabGetByProperties is the function to return the property
 * table by its property contents.
 *
 * This function can be used to query the current list of property tables for
 * one that contains the same set of property ID and property size. A matching
 * property table is returned if found. The order of the properties listed is
 * not important.
 *
 * \param numProp       An \ref RwInt32 for the number of properties.
 * \param propID        A pointer to an array of property's ID.
 * \param propStride    A pointer to an array of property's size.
 *
 * \return a pointer to a property table on successful, NULL otherwise.
 *
 * \see RpPrtStdPropTabGet
 */
RpPrtStdPropertyTable *
RpPrtStdPropTabGetByProperties(RwInt32 numProp, RwInt32 *propID, RwInt32 *propStride)
{
    RpPrtStdPropertyTable       *propTab, *propTabHead, *propTabFound;

    RWAPIFUNCTION(RWSTRING("RpPrtStdPropTabGetByProperties"));

    RWASSERT(numProp > 0);
    RWASSERT(propID);
    RWASSERT(propStride);

    propTabHead = rpPrtStdGlobals.propTab;

    propTab = propTabHead;
    propTabFound = NULL;

    if (propTab != NULL)
    {
        do
        {
            RWASSERT(propTab);

            /* This prop matches the num of prop. Now check the props themselves. */
            if (propTab->numProp == numProp)
            {
                if (_rpPrtStdPropTabCompareProp(propTab, numProp, propID, propStride))
                {
                    /* Found a match. */
                    propTabFound = propTab;

                    break;
                }
            }

            propTab = propTab->next;
        }
        while (propTab != propTabHead);
    }

    RWRETURN(propTabFound);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPropTabStreamRead is the function to stream in a
 * property table.
 *
 * The property tables must be streamed out before any emitters. By default
 * the property table is streamed out with each emitter.
 * Using the function \ref RpPrtStdGlobalDataSetStreamEmbedded this behavior can be
 * changed, allowing to stream all the class and other needed data to one stream
 * and the emitter in another one.
 *
 * When the emitters are streamed in, if there is no emitter class streamed with
 * it, it will search for any emitter class with the same identification number.
 * If it fails, then the emitter will have a NULL emitter class.
 *
 * The sequence to locate and read a property table from a binary stream is
 * as follows:
 * \code
   RwStream *stream;
   RpPrtStdPropertyTable *newPropertyTable;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_PRTSTDGLOBALDATA, NULL, NULL) )
       {
           newPropertyTable = RpPrtStdPropTabStreamRead(stream);
       }

       RwStreamClose(stream, NULL);
   }
   \endcode
 *
 * \param stream        A pointer to the output stream.
 *
 * \return a pointer to a property table on success, NULL otherwise.
 *
 * \see RpPrtStdPropTabCreate
 * \see RpPrtStdPropTabDestroy
 * \see RpPrtStdPropTabGet
 * \see RpPrtStdPropTabStreamWrite
 * \see RpPrtStdPropTabStreamGetSize
 * \see RpPrtStdGlobalDataSetStreamEmbedded
 * \see RpPrtStdGlobalDataGetStreamEmbedded
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RwStreamFindChunk
 * \see RpPrtStdPluginAttach
 */
RpPrtStdPropertyTable *
RpPrtStdPropTabStreamRead(RwStream *stream)
{
    RwInt32                         id, numProp, *propID;
    RwInt32                         *propSize, size;
    RpPrtStdPropertyTable           *propTab;

    RWAPIFUNCTION(RWSTRING("RpPrtStdPropTabStreamRead"));


    /* Read the id. */
    RwStreamReadInt32(stream, &id, sizeof(RwInt32));

    /* Read the prop count. */
    RwStreamReadInt32(stream, &numProp, sizeof(RwInt32));

    if (numProp > 0)
    {
        /* Check and resize the scratch mem. */
        size = (numProp) * 2 * sizeof(RwInt32);

        if (size > rpPrtStdGlobals.propIDScratchMemSize)
        {
            propID = (RwInt32 *) RwRealloc(rpPrtStdGlobals.propIDScratchMem, size,
               rwID_PRTSTDPLUGIN | rwMEMHINTDUR_GLOBAL | rwMEMHINTFLAG_RESIZABLE);

            if (NULL == propID)
                RWRETURN(NULL);

            rpPrtStdGlobals.propIDScratchMem = (RwChar *) propID;
            rpPrtStdGlobals.propIDScratchMemSize = size;
        }
        else
            propID = (RwInt32 *) rpPrtStdGlobals.propIDScratchMem;

        propSize = (RwInt32*)((RwUInt32)propID+numProp*sizeof(RwInt32));

        /* Read the prop id and size. */
        RwStreamReadInt32(stream, propID, ((numProp) * sizeof(RwInt32)));
        RwStreamReadInt32(stream, propSize, ((numProp) * sizeof(RwInt32)));

    }
    else
    {
        propID = NULL;
        propSize = NULL;
    }

    propTab = RpPrtStdPropTabGetByProperties(numProp, propID, propSize);

    /* Create the prop table */
    if (propTab == NULL)
        propTab = RpPrtStdPropTabCreate(numProp, propID, propSize);
    else
        propTab->refCount++;

    RWASSERT(propTab->id == id);

    /* ID is set by RpPrtStdPropTabCreate */
    if (propTab != NULL)
        propTab->id = id;

    /*
     * Call the post read callback to re-adjust some offsets.
     * Some properties' size were changed and this caused old dff
     * to be incompatible. These old files needs to be re-adjusted
     * to use the new structure sizes to avoid memory problems.
     */
    rpPrtStdPropTabPostRead(propTab);

    if (rpPrtStdGlobals.propTabPostReadCallBack != NULL)
    {
        (rpPrtStdGlobals.propTabPostReadCallBack)(propTab);
    }

    RWRETURN(propTab);
}


/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPropTabStreamWrite is the function to stream out a property
 * table
 *
 * The property tables must be streamed out before any emitters. By default
 * the property table is streamed out with each emitter.
 * Using the function \ref RpPrtStdGlobalDataSetStreamEmbedded this behavior can be
 * changed, allowing to stream all the class and other needed data to one stream
 * and the emitter in another one.
 *
 * When the emitters are streamed in, if there is no emitter class streamed with
 * it, it will search for any emitter class with the same identification number.
 * If it fails, then the emitter will have a NULL emitter class.
 *
 *
 * \param propTab       A pointer to the property table.
 * \param stream        A pointer to the output stream.
 *
 * \return a pointer to a property table on success, NULL otherwise.
 *
 * \see RpPrtStdPropTabCreate
 * \see RpPrtStdPropTabDestroy
 * \see RpPrtStdPropTabGet
 * \see RpPrtStdPropTabStreamRead
 * \see RpPrtStdPropTabStreamGetSize
 * \see RpPrtStdGlobalDataSetStreamEmbedded
 * \see RpPrtStdGlobalDataGetStreamEmbedded
 */
RpPrtStdPropertyTable *
RpPrtStdPropTabStreamWrite(RpPrtStdPropertyTable *propTab, RwStream *stream)
{
    RwInt32                     size;

    RWAPIFUNCTION(RWSTRING("RpPrtStdPropTabStreamWrite"));

    RWASSERT(propTab);

    /* Write header */
    size = RpPrtStdPropTabStreamGetSize(propTab);
    if (!RwStreamWriteChunkHeader(stream, rwID_PRTSTDGLOBALDATA, size))
    {
        RWRETURN((RpPrtStdPropertyTable *)NULL);
    }

    /* Write out the id. */
    RwStreamWriteInt32(stream, &propTab->id,  sizeof(RwInt32));

    /* Write out the prop count. */
    RwStreamWriteInt32(stream, &propTab->numProp, sizeof(RwInt32));

    if (propTab->numProp > 0)
    {
        /* Write out the prop id, size and offset. */
        RwStreamWriteInt32(stream, propTab->propID, ((propTab->numProp) * sizeof(RwInt32)));
        RwStreamWriteInt32(stream, propTab->propSizeUA, ((propTab->numProp) * sizeof(RwInt32)));
    }

    RWRETURN(propTab);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPropTabStreamGetSize is used to determine the size in bytes of
 * the binary representation of a property table. This value is used in the
 * binary chunk header to indicate the size of the chunk. The size includes
 * the size of the chunk header.
 *
 * \param propTab        A pointer to the property table.
 *
 * \return Returns a \ref RwInt32 value equal to the chunk size (in bytes)
 * of the property table.
 *
 * \see RpPrtStdPropTabCreate
 * \see RpPrtStdPropTabDestroy
 * \see RpPrtStdPropTabGet
 * \see RpPrtStdPropTabStreamRead
 * \see RpPrtStdPropTabStreamWrite
 */
RwInt32
RpPrtStdPropTabStreamGetSize(RpPrtStdPropertyTable *propTab)
{
    RwInt32                         size;

    RWAPIFUNCTION(RWSTRING("RpPrtStdPropTabStreamGetSize"));

    RWASSERT(propTab);

    /* The id and prop count */
    size = 2 * sizeof(RwInt32);

    if (propTab->numProp > 0)
    {
        /* Prop id, and size*/
        size += 2 * ((propTab->numProp) * sizeof(RwInt32));
    }

    RWRETURN(size);
}


/*
 *
 */
RwBool
_rpPrtStdSetPropTabPostReadCBack(_rpPrtStdPropTabCallBack pTabCB)
{
    RWFUNCTION(RWSTRING("_rpPrtStdSetPropTabPostReadCBack"));

    rpPrtStdGlobals.propTabPostReadCallBack = pTabCB;

    RWRETURN(TRUE);
}

RwBool
_rpPrtStdGetPropTabPostReadCBack(_rpPrtStdPropTabCallBack *pTabCB)
{
    RWFUNCTION(RWSTRING("_rpPrtStdGetPropTabPostReadCBack"));

    *pTabCB = rpPrtStdGlobals.propTabPostReadCallBack;

    RWRETURN(TRUE);
}

/*
 *
 */
RwBool
_rpPrtStdSetPropTabDtorCBack(_rpPrtStdPropTabCallBack pTabCB)
{
    RWFUNCTION(RWSTRING("_rpPrtStdSetPropTabDtorCBack"));

    rpPrtStdGlobals.propTabDtorCallBack = pTabCB;

    RWRETURN(TRUE);
}

RwBool
_rpPrtStdGetPropTabDtorCBack(_rpPrtStdPropTabCallBack *pTabCB)
{
    RWFUNCTION(RWSTRING("_rpPrtStdGetPropTabDtorCBack"));

    *pTabCB = rpPrtStdGlobals.propTabDtorCallBack;

    RWRETURN(TRUE);
}

