/**
 * \ingroup rt2dstringlabel
 * \page rt2stringlabeloverview Rt2dStringLabel Overview
 *
 * Rt2dStringLabel is a string reference structure that is used by \ref Rt2dMaestro
 * to allow linking of internal and external data by name without a performance
 * hit.
 *
 * \ref Rt2dMaestro stores a table of string labels. When a maestro is created or
 * streamed in, the string label table is populated. The calling function can then
 * look up names of interest within the table. The index that indicates where the
 * name was found can be used as a handle to identify that name.
 *
 * Additionally, an identifier is stored within the table to note what kind of data
 * is being referenced by the name.
 *
 * The user may store additional data in the table against each name. This provides
 * a convenient location to place callbacks or flag locations. This would then be
 * used by a custom message handler hooked to the \ref Rt2dMaestro.
 *
 * \see Rt2dMaestroCreate
 * \see Rt2dMessage
 * \see Rt2dStringLabel
 */

/*
 *
 * Functionality for 2D rendering
 *
 * Copyright (c) 2001 Criterion Software Ltd.
 */

/****************************************************************************
 *                                                                          *
 *  Module  :   strlabel.c                                                  *
 *                                                                          *
 *  Purpose :   simple 2d animation maestro functionality                  *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
 */

#include <string.h>
#include <rwcore.h>
#include <rpdbgerr.h>
#include <rpcriter.h>

#include "rt2danim.h"
#include "anim.h"
#include "props.h"
#include "maestro.h"
#include "bucket.h"
#include "strlabel.h"
#include "gstate.h"

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
 Local Defines
 */

/****************************************************************************
 Local Types
 */

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */

/****************************************************************************
 Functions.
 */

/****************************************************************************
 *
 *
 *
 *
 ****************************************************************************/

Rt2dStringLabel *
_rt2dStringLabelInit(Rt2dStringLabel *strLabel)
{
    RWFUNCTION(RWSTRING("_rt2dStringLabelInit"));

    RWASSERT(strLabel);

    strLabel->entityType = rt2dANIMLABELTYPENONE;
    strLabel->nameIndex = -1;
    strLabel->internalData = NULL;
    strLabel->userData = NULL;

    RWRETURN(strLabel);
}

RwBool
_rt2dStringLabelDestruct(Rt2dStringLabel *strLabel)
{
    RwBool              result;

    RWFUNCTION(RWSTRING("_rt2dStringLabelDestruct"));

    result = TRUE;

    RWASSERT(strLabel);

    strLabel->entityType = rt2dANIMLABELTYPENONE;
    strLabel->nameIndex = -1;
    strLabel->internalData = NULL;
    strLabel->userData = NULL;

    RWRETURN(result);
}

/****************************************************************************/

Rt2dStringLabel *
_rt2dStringLabelCreate( void )
{
    Rt2dStringLabel     *strLabel;

    RWFUNCTION(RWSTRING("_rt2dStringLabelCreate"));

    if ((strLabel = (Rt2dStringLabel *) RwFreeListAlloc(&Rt2dAnimGlobals.stringLabelFreeList,
                                                rwID_2DANIM | rwMEMHINTDUR_EVENT)) == NULL)
    {
        RWRETURN((Rt2dStringLabel *)NULL);
    }

    /* Initialise members */
    if (_rt2dStringLabelInit(strLabel) == NULL)
    {
        _rt2dStringLabelDestruct(strLabel);

        RwFreeListFree(&Rt2dAnimGlobals.stringLabelFreeList, strLabel);

        strLabel = (Rt2dStringLabel *)NULL;
    }

    RWRETURN(strLabel);
}

RwBool
_rt2dStringLabelDestroy(Rt2dStringLabel *strLabel)
{
    RwBool                  result;

    RWFUNCTION(RWSTRING("_rt2dStringLabelDestroy"));

    result = TRUE;

    RWASSERT(strLabel);

    _rt2dStringLabelDestruct(strLabel);

    /* Free string label itself */
    RwFreeListFree(&Rt2dAnimGlobals.stringLabelFreeList, strLabel);

    RWRETURN(result);
}

/****************************************************************************/

RwUInt32
_rt2dStringLabelStreamGetSize(Rt2dMaestro *maestro, Rt2dStringLabel *strLabel)
{
    RwUInt32    size;
    RwChar     *name;

    RWFUNCTION(RWSTRING("_rt2dStringLabelStreamGetSize"));

    RWASSERT(maestro);
    RWASSERT(strLabel);

    size = 0;

    /* Size of the string. */
    size += sizeof(RwInt32);
    if (strLabel->nameIndex >= 0)
    {
        RWASSERT(maestro->byteBucket);

        name = (RwChar *)_rt2dByteBucketGetDataByIndex(
                                maestro->byteBucket, strLabel->nameIndex);

        size += (strlen(name) + 1);
    }

    /* An enum for the entity type. */
    size += sizeof(RwInt32);

    /* An int for the Internal data. */
    size += sizeof(RwInt32);

    RWRETURN(size);
}

Rt2dStringLabel *
_rt2dStringLabelStreamRead(Rt2dMaestro *maestro, Rt2dStringLabel *strLabel, RwStream *stream)
{
    RwInt32             size;

    RWFUNCTION(RWSTRING("_rt2dStringLabelStreamRead"));

    RWASSERT(maestro);
    RWASSERT(stream);

    if (strLabel == NULL)
    {
        if ((strLabel = _rt2dStringLabelCreate()) == NULL)
        {
            RWRETURN((Rt2dStringLabel *)NULL);
        }
    }

    /* Read in the string. */
    if (RwStreamReadInt32(stream, &size, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dStringLabel *)NULL);
    }
    if (size > 0)
    {
        RwInt32 index =
            _rt2dByteBucketAddData(maestro->byteBucket, NULL, size);

        if (-1 == index)
        {
            RWRETURN((Rt2dStringLabel *)NULL);
        }

        if ((RwInt32) RwStreamRead(
                        stream,
                        _rt2dByteBucketGetDataByIndex(maestro->byteBucket, index),
                        size) != size)
        {
            RWRETURN((Rt2dStringLabel *)NULL);
        }

        strLabel->nameIndex = index;
    }
    else
    {
        strLabel->nameIndex = -1;
    }

    /* Read the entity type. */
    if (RwStreamReadInt32(stream, (RwInt32 *)&strLabel->entityType, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dStringLabel *)NULL);
    }

    /* Read the internal data */
    if (RwStreamReadInt32(stream, (RwInt32 *)&strLabel->internalData, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dStringLabel *)NULL);
    }

    RWRETURN(strLabel);
}

Rt2dStringLabel *
_rt2dStringLabelStreamWrite(Rt2dMaestro *maestro, Rt2dStringLabel *strLabel, RwStream *stream)
{
    RwInt32         size;
    RwChar          *name;

    RWFUNCTION(RWSTRING("_rt2dStringLabelStreamWrite"));

    RWASSERT(maestro);
    RWASSERT(strLabel);
    RWASSERT(stream);

    /* Write out the string. */
    if (strLabel->nameIndex >= 0)
    {
        RWASSERT(maestro->byteBucket);

        name = (RwChar*)_rt2dByteBucketGetDataByIndex(
                            maestro->byteBucket,
                            strLabel->nameIndex);
        size = (strlen(name) + 1);

        if (RwStreamWriteInt32(stream, &size, sizeof(RwInt32)) == NULL)
        {
            RWRETURN((Rt2dStringLabel *)NULL);
        }

        if (RwStreamWrite(stream, name, size) == NULL)
        {
            RWRETURN((Rt2dStringLabel *)NULL);
        }
    }
    else
    {
        /* Empty string. */
        size = 0;
        if (RwStreamWriteInt32(stream, &size, sizeof(RwInt32)) == NULL)
        {
            RWRETURN((Rt2dStringLabel *)NULL);
        }
    }

    /* Write out the enity type. */
    if (RwStreamWriteInt32(stream, (RwInt32 *)&strLabel->entityType, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dStringLabel *)NULL);
    }

    /* Write out the internal data */
    if (RwStreamWriteInt32(stream, (RwInt32 *)&strLabel->internalData, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dStringLabel *)NULL);
    }

    RWRETURN(strLabel);
}

/****************************************************************************/

/**
 * \ingroup rt2dstringlabel
 * \ref Rt2dStringLabelGetStringLabelType is used to retrieve the label type of
 * the string label, (\ref Rt2dStringLabelType).
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param strLabel          Pointer to the string label.
 *
 * \return Returns the entity label type of the string label.
 *
 * \see Rt2dStringLabelSetStringLabelType
 * \see Rt2dMaestroAddStringLabel
 */

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
Rt2dStringLabelType
Rt2dStringLabelGetStringLabelType(Rt2dStringLabel *strLabel)
{
    Rt2dStringLabelType      entityType;

    RWAPIFUNCTION(RWSTRING("Rt2dStringLabelGetStringLabelType"));

    RWASSERT(strLabel);

    entityType = (Rt2dStringLabelType)
            _rt2dStringLabelGetStringLabelTypeMacro(strLabel);

    RWRETURN(entityType);
}
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/**
 * \ingroup rt2dstringlabel
 * \ref Rt2dStringLabelSetStringLabelType is used set the label type of
 * the string label, (\ref Rt2dStringLabelType).
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param strLabel          Pointer to the string label.
 * \param entityType        Entity label type.
 *
 * \see Rt2dStringLabelGetStringLabelType
 * \see Rt2dMaestroAddStringLabel
 */

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
void
Rt2dStringLabelSetStringLabelType(Rt2dStringLabel *strLabel,
                                      Rt2dStringLabelType entityType)
{
    RWAPIFUNCTION(RWSTRING("Rt2dStringLabelSetStringLabelType"));

    RWASSERT(strLabel);

    _rt2dStringLabelSetStringLabelTypeMacro(strLabel, entityType);

    RWRETURNVOID();
}
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/**
 * \ingroup rt2dstringlabel
 * \ref Rt2dStringLabelGetNameIndex is used to retrieve the name
 * index of the string label. The name of the label is not stored
 * with the string label but in a string table in the parent maestro.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param strLabel          Pointer to the string label.
 *
 * \return Returns the name index of the string label.
 *
 * \see Rt2dStringLabelSetNameIndex
 * \see Rt2dMaestroAddStringLabel
 */

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
RwInt32
Rt2dStringLabelGetNameIndex(Rt2dStringLabel *strLabel)
{
    RwInt32     index;

    RWAPIFUNCTION(RWSTRING("Rt2dStringLabelGetNameIndex"));

    RWASSERT(strLabel);

    index = _rt2dStringLabelGetNameIndexMacro(strLabel);

    RWRETURN(index);
}
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/**
 * \ingroup rt2dstringlabel
 * \ref Rt2dStringLabelSetNameIndex is used to set the name index
 * of the string label. The name of  the label is not stored with the
 * string label but in a string table in the parent maestro.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param strLabel          Pointer to the string label.
 * \param index             Name index of the string label.
 *
 * \see Rt2dStringLabelGetNameIndex
 * \see Rt2dMaestroAddStringLabel
 */

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
void
Rt2dStringLabelSetNameIndex(Rt2dStringLabel *strLabel, RwInt32 index)
{
    RWAPIFUNCTION(RWSTRING("Rt2dStringLabelSetNameIndex"));

    RWASSERT(strLabel);

    _rt2dStringLabelSetNameIndexMacro(strLabel, index);

    RWRETURNVOID();
}
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/**
 * \ingroup rt2dstringlabel
 * \ref Rt2dStringLabelGetInternalData is used to retrieve the internal data
 * that is attached the string label. This internal data may be used by
 * any entity that uses the string label.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param strLabel          Pointer to the string label.
 *
 * \return Returns a pointer to the internal data attached to the string label.
 *
 * \see Rt2dStringLabelSetInternalData
 * \see Rt2dMaestroAddStringLabel
 */

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
void *
Rt2dStringLabelGetInternalData(Rt2dStringLabel *strLabel)
{
    void        *internalData;
    RWAPIFUNCTION(RWSTRING("Rt2dStringLabelGetInternalData"));

    RWASSERT(strLabel);

    internalData = _rt2dStringLabelGetInternalDataMacro(strLabel);

    RWRETURN(internalData);
}
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/**
 * \ingroup rt2dstringlabel
 * \ref Rt2dStringLabelSetInternalData is used to attach internal data
 * to the string label. The internal data may be used by any entity that
 * uses the string label.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param strLabel          Pointer to the string label.
 * \param internalData      Pointer to the internal data.
 *
 * \see Rt2dStringLabelGetInternalData
 * \see Rt2dMaestroAddStringLabel
 */

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
void
Rt2dStringLabelSetInternalData(Rt2dStringLabel *strLabel, void *internalData)
{
    RWAPIFUNCTION(RWSTRING("Rt2dStringLabelSetInternalData"));

    RWASSERT(strLabel);

    _rt2dStringLabelSetInternalDataMacro(strLabel, internalData);

    RWRETURNVOID();
}
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/**
 * \ingroup rt2dstringlabel
 * \ref Rt2dStringLabelGetUserData is used to retrieve the private data
 * to the string label. The private data may be used by any entity that
 * uses the string label.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param strLabel          Pointer to the string label.
 *
 * \return Returns a pointer to the private data attached to the string label.
 *
 * \see Rt2dStringLabelSetUserData
 * \see Rt2dMaestroAddStringLabel
 */

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
void *
Rt2dStringLabelGetUserData(Rt2dStringLabel *strLabel)
{
    void            *userData;

    RWAPIFUNCTION(RWSTRING("Rt2dStringLabelGetUserData"));

    RWASSERT(strLabel);

    userData = _rt2dStringLabelGetUserDataMacro(strLabel);

    RWRETURN(userData);
}
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/**
 * \ingroup rt2dstringlabel
 * \ref Rt2dStringLabelSetUserData is used to attach private data
 * to the string label. The private data may be used by any entity that
 * uses the string label
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param strLabel          Pointer to the string label.
 * \param userData          Pointer to the user data.
 *
 * \see Rt2dStringLabelGetUserData
 * \see Rt2dMaestroAddStringLabel
 */

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
void
Rt2dStringLabelSetUserData(Rt2dStringLabel *strLabel, void *userData)
{
    RWAPIFUNCTION(RWSTRING("Rt2dStringLabelSetUserData"));

    RWASSERT(strLabel);

    _rt2dStringLabelSetUserDataMacro(strLabel, userData);

    RWRETURNVOID();
}
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/****************************************************************************
 *
 * Maestro's string label functions.
 *
 ****************************************************************************/

Rt2dMaestro *
_rt2dMaestroCreateStringLabels(Rt2dMaestro *maestro)
{
    Rt2dMaestro    *result;

    RWFUNCTION(RWSTRING("_rt2dMaestroCreateStringLabels"));

    RWASSERT(maestro);
    RWASSERT((maestro->labels == NULL));

    maestro->labels = _rwSListCreate(sizeof(Rt2dStringLabel),
                            rwID_2DANIM | rwMEMHINTDUR_EVENT);

    result =    (maestro->labels == NULL)
              ? ( (Rt2dMaestro *)NULL )
              : maestro;

    RWRETURN(result);
}

Rt2dMaestro *
_rt2dMaestroDestroyStringLabels(Rt2dMaestro *maestro)
{
    RwInt32                 num;
    Rt2dStringLabel     *strLabel, *beginStrLabel, *endStrLabel;

    RWFUNCTION(RWSTRING("_rt2dMaestroDestroyStringLabels"));

    RWASSERT(maestro);

    /* Check and destroy the label. */
    if (maestro->labels)
    {
        num = _rwSListGetNumEntries(maestro->labels);

        if (num > 0)
        {
            beginStrLabel = (Rt2dStringLabel *)
                                _rwSListGetArray(maestro->labels);
            endStrLabel = beginStrLabel + num;

            for (strLabel = beginStrLabel; strLabel != endStrLabel; ++strLabel)
            {
                _rt2dStringLabelDestruct(strLabel);
            }

        }
        _rwSListDestroy(maestro->labels);

        maestro->labels = (RwSList*)NULL;
    }

    RWRETURN(maestro);
}

RwUInt32
_rt2dMaestroStringLabelsStreamGetSize(Rt2dMaestro *maestro)
{
    RwInt32                size, numStrLabel;
    Rt2dStringLabel     *strLabel, *beginStrLabel, *endStrLabel;

    RWFUNCTION(RWSTRING("_rt2dMaestroStringLabelsStreamGetSize"));

    RWASSERT(maestro);

    size = 0;

    /* Number of labels. */
    size += sizeof(RwInt32);

    /* Size of the label SList. */
    if (maestro->labels)
    {
        numStrLabel = _rwSListGetNumEntries(maestro->labels);

        if (numStrLabel > 0)
        {
            beginStrLabel =(Rt2dStringLabel *) _rwSListGetArray(maestro->labels);
            endStrLabel = beginStrLabel + numStrLabel;

            RWASSERT(beginStrLabel);

            for (strLabel = beginStrLabel; strLabel != endStrLabel; ++strLabel)
            {
                size += _rt2dStringLabelStreamGetSize(maestro, strLabel);
            }
        }
    }

    RWRETURN(size);
}

Rt2dMaestro *
_rt2dMaestroStringLabelsStreamRead(Rt2dMaestro *maestro, RwStream *stream)
{
    RwInt32                 numStrLabels;
    Rt2dStringLabel     *strLabel, *beginStrLabel, *endStrLabel;

    RWFUNCTION(RWSTRING("_rt2dMaestroStringLabelsStreamRead"));

    RWASSERT(maestro);
    RWASSERT(stream);

    /* Read in the num of string labels. */
    if (RwStreamReadInt32(stream, &numStrLabels, sizeof(RwInt32)) == NULL)
    {
        RWRETURN( (Rt2dMaestro *)NULL);
    }

    if (numStrLabels > 0)
    {
        /* Create an SList for labels. */
        if (maestro->labels == NULL)
        {
            _rt2dMaestroCreateStringLabels(maestro);
        }

        /* Read in the buttons. */
        if (maestro->labels)
        {
            if ((beginStrLabel =
                (Rt2dStringLabel *)
                    _rwSListGetNewEntries(maestro->labels, numStrLabels,
                             rwID_2DANIM | rwMEMHINTDUR_EVENT)) == NULL)
            {
                RWRETURN( (Rt2dMaestro *)NULL);
            }

            endStrLabel = beginStrLabel + numStrLabels;

            for (strLabel = beginStrLabel; strLabel != endStrLabel; ++strLabel)
            {
                _rt2dStringLabelInit(strLabel);

                if (_rt2dStringLabelStreamRead(maestro, strLabel, stream) == NULL)
                {
                    RWRETURN( (Rt2dMaestro *)NULL);
                }
            }
        }
        else
        {
            /* Failed to create the SList for labels. */
            RWRETURN( (Rt2dMaestro *)NULL);
        }
    }

    RWRETURN(maestro);
}

Rt2dMaestro *
_rt2dMaestroStringLabelsStreamWrite(Rt2dMaestro *maestro, RwStream *stream)
{
    RwInt32                 numStrLabel;
    Rt2dStringLabel     *strLabel, *beginStrLabel, *endStrLabel;

    RWFUNCTION(RWSTRING("_rt2dMaestroStringLabelsStreamWrite"));

    RWASSERT(maestro);
    RWASSERT(stream);

    if (maestro->labels)
    {
        /* Write out the number of labels. */
        numStrLabel = _rwSListGetNumEntries(maestro->labels);
        if (RwStreamWriteInt32(stream, &numStrLabel, sizeof(RwInt32)) == NULL)
        {
            RWRETURN( (Rt2dMaestro *)NULL);
        }

        if (numStrLabel)
        {
            beginStrLabel = (Rt2dStringLabel *)_rwSListGetArray(maestro->labels);
            endStrLabel = beginStrLabel + numStrLabel;

            RWASSERT(beginStrLabel);

            for (strLabel = beginStrLabel; strLabel != endStrLabel; ++strLabel)
            {
                if (_rt2dStringLabelStreamWrite(maestro, strLabel, stream) == NULL)
                {
                    RWRETURN( (Rt2dMaestro *)NULL);
                }
            }
        }
    }
    else
    {
        numStrLabel = 0;
        if (RwStreamWriteInt32(stream, &numStrLabel, sizeof(RwInt32)) == NULL)
        {
            RWRETURN( (Rt2dMaestro *)NULL);
        }
    }

    RWRETURN(maestro);
}

/****************************************************************************/

/**
 * \ingroup rt2dstringlabel
 * \ref Rt2dMaestroAddStringLabel is used to add a label entry into a table
 * referencible by a string and an entity type. The table is used to store a
 * small set of data that is common and can be shared.
 *
 * It is also used to associate a name with a Rt2dAnim entity type, such as
 * a button or cel. A string name can exist more than once providing it is
 * for different entity type. If an entry already exist for name and entity
 * type, the existing entry is updated and its index returned.
 *
 * The maestro, enitityType and name parameters are mandatory. internalData
 * and index are optional. The index parameter is used to return the index
 * in the table if not NULL.
 *
 * \param maestro      Pointer to the parent maestro.
 * \param entityType    Entity type of the label.
 * \param name          String name of the label.
 * \param internalData  Private data to be added
 * \param index         Pointer to a RwInt32 to return the index in the table.
 *
 * \return Returns a pointer to the maestro if successful, NULL otherwise.
 *
 * \see Rt2dMaestroFindStringLabel
 * \see Rt2dMaestroGetStringLabelByIndex
 */
Rt2dMaestro *
Rt2dMaestroAddStringLabel(Rt2dMaestro *maestro,
                            Rt2dStringLabelType entityType, const RwChar *name, void *internalData,
                            RwInt32 *index)
{
    RwUInt32                 length;
    Rt2dStringLabel     *strLabel;

    RWAPIFUNCTION(RWSTRING("Rt2dMaestroAddStringLabel"));

    RWASSERT(maestro);
    RWASSERT(name);

    /* Check if the string label already exist by searching the name. */
    if ((strLabel = Rt2dMaestroFindStringLabel(maestro, entityType, name, index)) == NULL)
    {
        if (index)
        {
            *index = _rwSListGetNumEntries(maestro->labels);
        }

        if ((strLabel = (Rt2dStringLabel *)
                            _rwSListGetNewEntry(maestro->labels,
                            rwID_2DANIM | rwMEMHINTDUR_EVENT)) == NULL)
        {
            RWRETURN( (Rt2dMaestro *)NULL);
        }

        _rt2dStringLabelInit(strLabel);

        /* Add the string to the ByteBucket. */
        length = rwstrlen(name) + 1;

        strLabel->nameIndex =
            _rt2dByteBucketAddData(maestro->byteBucket, (const void *) name, length);
    }

    strLabel->entityType = entityType;
    strLabel->internalData = internalData;

    RWRETURN(maestro);
}


/**
 * \ingroup rt2dstringlabel
 * \ref Rt2dMaestroFindStringLabel is used to search the label table for an
 * entry with the given name and entity type.
 *
 * The search is done on the name - entity type pairs. A string can appear
 * appear more than once in the label table. Once for each entity type. Therefore
 * both the string and the entity type must be given.
 *
 * \param maestro      Pointer to the parent maestro.
 * \param entityType    Entity type to be searched.
 * \param lookupName    String name to be searched.
 * \param index         Pointer to a RwInt32 to return the index in the table.
 *
 * \return Returns the string label if found, NULL otherwise.
 *
 * \see Rt2dMaestroAddStringLabel
 * \see Rt2dMaestroGetStringLabelByIndex
 */
Rt2dStringLabel *
Rt2dMaestroFindStringLabel(Rt2dMaestro *maestro,
                            Rt2dStringLabelType entityType, const RwChar *lookupName,
                            RwInt32 *index)
{
    Rt2dStringLabel     *strLabel, *result;
    RwInt32                 i, num;
    RwChar                  *name;

    RWAPIFUNCTION(RWSTRING("Rt2dMaestroFindStringLabel"));

    RWASSERT(maestro);
    RWASSERT(lookupName);

    num = 0;
    result = (Rt2dStringLabel *)NULL;

    if (maestro->labels)
    {
        RWASSERT(maestro->byteBucket);

        num = _rwSListGetNumEntries(maestro->labels);

        if (num > 0)
        {
            strLabel = (Rt2dStringLabel *)_rwSListGetArray(maestro->labels);

            RWASSERT(strLabel);

            for (i = 0; i < num; i++)
            {
                name = (RwChar *)
                    _rt2dByteBucketGetDataByIndex(maestro->byteBucket, strLabel->nameIndex);

                if ((strLabel->entityType == (RwUInt32) entityType) &&
                    (rwstrcmp(name, lookupName) == 0))
                {
                    result = strLabel;

                    if (index)
                        *index = i;

                    break;
                }

                strLabel++;
            }
        }
    }

    RWRETURN(result);
}

/**
 * \ingroup rt2dstringlabel
 * \ref Rt2dMaestroGetStringLabelByIndex is used to return a label entry
 * by using an index into the table. This index should be the index returned
 * by \ref Rt2dMaestroAddStringLabel or \ref Rt2dMaestroFindStringLabel.
 *
 * \param maestro      Pointer to the parent maestro.
 * \param index         Index into the label table.
 *
 * \return Returns the string label of the index, NULL otherwise.
 *
 * \see Rt2dMaestroAddStringLabel
 * \see Rt2dMaestroFindStringLabel
 */
Rt2dStringLabel *
Rt2dMaestroGetStringLabelByIndex(Rt2dMaestro *maestro, RwInt32 index)
{
    Rt2dStringLabel     *strLabels = (Rt2dStringLabel *)NULL;
    Rt2dStringLabel     *beginStrLabels;
    RwInt32                 num;

    RWAPIFUNCTION(RWSTRING("Rt2dMaestroGetStringLabelByIndex"));

    RWASSERT(maestro);

    if (maestro->labels)
    {
        num = _rwSListGetNumEntries(maestro->labels);

        if ((num > 0) && (index < num))
        {
            beginStrLabels = (Rt2dStringLabel *)_rwSListGetArray(maestro->labels);
            strLabels = beginStrLabels + index;
        }
    }

    RWRETURN(strLabels);
}

/**
 * \ingroup rt2dstringlabel
 * \ref Rt2dMaestroGetStringLabelName is used to retreive the actual label
 * pointed by a \ref Rt2dStringLabel.
 *
 * \param maestro      Pointer to the parent maestro.
 * \param strLabel     Pointer to the Rt2dStringLabel.
 *
 * \return Returns a const pointer to the label, NULL otherwise
 *
 * \see Rt2dMaestroAddStringLabel
 * \see Rt2dMaestroGetStringLabelByIndex
 * \see Rt2dMaestroFindStringLabel
 */
const RwChar *
Rt2dMaestroGetStringLabelName(Rt2dMaestro *maestro,
                                Rt2dStringLabel *strLabel)
{
    RwChar *result = (RwChar *)NULL;
    RWAPIFUNCTION(RWSTRING("Rt2dMaestroGetStringLabelName"));
    RWASSERT(strLabel);

    result = (RwChar*)_rt2dByteBucketGetDataByIndex(maestro->byteBucket,
                                                    strLabel->nameIndex);

    RWRETURN(result);
}

#if defined (__MWERKS__)
#if (defined(RWVERBOSE))
#pragma message (__FILE__ "/" _SKY_EXPAND(__LINE__) ": __MWERKS__ == " _SKY_EXPAND(__MWERKS__))
#endif /* (defined (__MWERKS__)) */
#if (__option (global_optimizer))
#pragma always_inline on
#endif /* (__option (global_optimizer)) */
#endif /*  defined (__MWERKS__) */
