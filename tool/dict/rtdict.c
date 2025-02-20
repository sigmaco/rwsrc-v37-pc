/**********************************************************************
 *
 * File : rtdict.c
 *
 * Abstract : A toolkit for generic dictionary support
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
 * Copyright (c) 2002 Criterion Software Ltd.
 * All Rights Reserved.
 *
 * RenderWare is a trademark of Canon Inc.
 *
 ************************************************************************/

/**
 * \ingroup rtdict
 * \page rtdictoverview RtDict Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h
 * \li \b Libraries: rwcore
 * \li \b Plugin \b attachments: None
 *
 * \subsection dictoverview Overview
 *
 * \ref rtdict is a generic dictionary toolkit, enabling you to create
 * dictionaries of arbitrary objects that may be located by name.
 *
 * The layout of a dictionary is specified by a 'layout' object, \ref RtDictSchema.
 * Placed within the \ref RtDictSchema are callbacks that allow the dictionary
 * to manage the lifetime and streaming of the contained objects. \ref RtDictSchema
 * also manages a reference to a 'current' dictionary.
 *
 * To use dictionaries, get access to or create a schema that defines the
 * particular kind of dictionary you require. Then, call \ref RtDictSchema
 * functions to perform specific operations on dictionaries of that kind.
 * \ref RtDictSchemaCreateDict may be used to create new dictionaries;
 * \ref RtDictSchemaStreamReadDict can read in a dictionary; \ref RtDictSchemaGetCurrentDict
 * obtains the current dictionary and \RtDictschemaSetCurrentDict sets it.
 *
 * Once you have a pointer to an \ref RtDict object, you may place entries
 * within the dictionary (\ref RtDictAddEntry), remove entries (\ref RtDictRemoveEntry)
 * or stream the dictionary (\ref RtDictStreamWrite, \ref RtDictStreamGetSize).
 *
**/

#include "rwcore.h"
#include "rpdbgerr.h"
#include "rtdict.h"

/******************************************************************************
 *  Defines
 */

/******************************************************************************
 *  Macros
 */

#define rwLLLinkGetConstData(linkvar,type,entry)        \
    ((const type *)(((const RwUInt8 *)(linkvar))-offsetof(type,entry))) \

/* void* RwSList iterator */
#define IteratorInit(_iter, _list)                                      \
MACRO_START                                                             \
{                                                                       \
    (_iter).end = _rwSListGetEnd((_list));                              \
    (_iter).cur = _rwSListGetBegin((_list));                            \
}                                                                       \
MACRO_STOP

#define IteratorValid(_iter)     ((_iter).cur != (_iter).end)
#define IteratorNext(_iter)      ((_iter).cur++)

#define IteratorGetEntry(_iter)   \
            (*(RtDictEntryType *)((_iter).cur))

#define IteratorGetConstEntry(_iter)   \
            (*(RtDictEntryConstType *)((_iter).cur))

#define IteratorGetDict(_iter)   \
            (*(RtDict **)((_iter).cur))

#define IteratorGetConstDict(_iter)   \
            (*(const RtDict **)((_iter).cur))


/******************************************************************************
 *  Local variables
 */



/******************************************************************************
 *  Local types
 */

typedef union _rwPtrDictEntry _rwPtrDictEntry;
union _rwPtrDictEntry
{
    RtDictEntryType       ptrEntry;
    RtDictEntryConstType constptrEntry;
};

typedef union _rwPtrDict _rwPtrDict;
union _rwPtrDict
{
    RtDict       *ptrDict;
    const RtDict *constptrDict;
};

typedef struct BinaryDict BinaryDict;
struct BinaryDict
{
    RwUInt32 size;
};

typedef struct Iterator Iterator;
struct Iterator
{
    void **cur;
    void **end;
};

/**
 * \ingroup rtdict
 * \ref RtDictSchemaInit
 * initializes an external schema and creates an internal list of all dictionaries
 * of this schema type.
 *
 * \param schema An external schema structure already initialized with appropriate
 * entry management callbacks.
 *
 * \return A valid pointer to the \ref RtDictSchema if successful, NULL otherwise
 * \see RtDictSchemaDestruct
 * \see RtDictDestroy
 * \see RtDictAddEntry
 */
RtDictSchema *
RtDictSchemaInit(RtDictSchema *schema)
{
    RWAPIFUNCTION(RWSTRING("RtDictSchemaInit"));

    schema->dictionaries = rwSListCreate(
                                sizeof(RtDict *),
                                rwMEMHINTDUR_GLOBAL | schema->dictChunkType);
    if (!schema->dictionaries)
    {
        RWRETURN((RtDictSchema *)NULL);
    }

    schema->current = NULL;

    RWRETURN(schema);
}

/**
 * \ingroup rtdict
 * \ref RtDictSchemaDestruct
 * destructs an external schema. Destroys the internal list of all dictionaries
 * of this schema type.
 *
 * \param schema The schema to be used
 *
 * \return A valid pointer to the \ref RtDictSchema if successful, NULL otherwise
 * \see RtDictSchemaDestruct
 * \see RtDictDestroy
 * \see RtDictAddEntry
 */
RwBool
RtDictSchemaDestruct(RtDictSchema *schema)
{
    RWAPIFUNCTION(RWSTRING("RtDictSchemaDestruct"));

    /* Check if anything left in the list */
    RWASSERTM((0 == rwSListGetNumEntries(schema->dictionaries)),
              (RWSTRING("Dictionary still present on shutdown."))
             );

    rwSListDestroy(schema->dictionaries);
    RWRETURN(TRUE);
}

/**
 * \ingroup rtdict
 * \ref RtDictSchemaCreateDict
 * creates an empty dictionary. \ref RtDictAddEntry can be used to add
 * entries to the dictionary
 *
 * \param schema The schema describing the dictionary layout
 *
 * \return A valid pointer to a RtDict if successful, NULL otherwise
 * \see RtDictDestroy
 * \see RtDictAddEntry
 */
RtDict *
RtDictSchemaCreateDict(RtDictSchema *schema)
{
    RtDict   *dict;
    RwUInt32   size;

    RWAPIFUNCTION(RWSTRING("RtDictSchemaCreateDict"));

    /* Allocate the memory */
    size = sizeof(RtDict);
    dict = (RtDict *) RwMalloc(size,
                               rwMEMHINTDUR_EVENT | schema->dictChunkType);
    if (!dict)
    {
        RWERROR((E_RW_NOMEM, size));
        RWRETURN((RtDict *) NULL);
    }

    if (!_rtDictSchemaInitDict(schema, dict))
    {
        RwFree(dict);
        RWRETURN((RtDict *) NULL);
    }

    RWRETURN(dict);
}

/**
 * \ingroup rtdict
 * \ref RtDictSchemaAddDict
 * adds a new dictionary to the list of those managed by the schema.
 *
 * \param schema The schema describing the dictionary layout
 * \param dict The dictionary to add to the list
 *
 * \return A valid pointer to a \ref RtDictSchema if successful, NULL otherwise
 * \see RtDictSchemaCreateDict
 * \see RtDictSchemaRemoveDict
 */
RtDictSchema *
RtDictSchemaAddDict(RtDictSchema *schema, RtDict *dict)
{
    RtDict **placeInSchemaDictList;

    RWAPIFUNCTION(RWSTRING("RtDictSchemaAddDict"));

    placeInSchemaDictList
       = (RtDict **)rwSListGetNewEntry(
                                 schema->dictionaries,
                                 rwMEMHINTDUR_EVENT | schema->dictChunkType);

    if (!placeInSchemaDictList)
    {
        RWRETURN((RtDictSchema *)NULL);
    }

    *placeInSchemaDictList = dict;

    RWRETURN(schema);
}

/**
 * \ingroup rtdict
 * \ref RtDictSchemaRemoveDict
 * removes a dictionary to the list of those managed by the schema.
 *
 * \param schema The schema describing the dictionary layout
 * \param dict The dictionary to remove from the list
 *
 * \return A valid pointer to a \ref RtDictSchema if successful, NULL otherwise
 * \see RtDictDestroy
 * \see RtDictSchemaAddDict
 */
RtDictSchema *
RtDictSchemaRemoveDict(RtDictSchema *schema, RtDict *dict)
{
    RWAPIFUNCTION(RWSTRING("RtDictSchemaRemoveDict"));
    RWASSERT(schema);
    RWASSERT(dict);
    RWASSERT(dict->schema==schema);

    {
        RtDict **current = (RtDict **)(rwSListGetBegin(schema->dictionaries));
        RtDict **end = (RtDict **)(rwSListGetEnd(schema->dictionaries));

        while (current!=end)
        {
            if (dict == *current)
            {
                /* Remove */
                *current = *(RtDict **)rwSListGetEntry(
                                        schema->dictionaries,
                                        rwSListGetNumEntries(schema->dictionaries)-1);
                rwSListDestroyEndEntries(schema->dictionaries, 1);

                RWRETURN(schema);
            }
            ++current;
        }

    }

    RWRETURN((RtDictSchema *)NULL);
}

/**
 * \ingroup rtdict
 * \ref RtDictSchemaGetCurrentDict
 * gets the current dictionary.
 *
 * \param schema The schema for which to obtain the current dictionary of
 *
 * \return A pointer to the current RtDict. This is NULL if there is no current dictionary.
 * \see RtDictSetCurrent
 */
RtDict *
RtDictSchemaGetCurrentDict(RtDictSchema *schema)
{
    RWAPIFUNCTION(RWSTRING("RtDictSchemaGetCurrentDict"));
    RWASSERT(schema);

    RWRETURN(schema->current);
}


/**
 * \ingroup rtdict
 * \ref RtDictSchemaSetCurrentDict
 * sets the current dictionary.
 *
 * \param schema The schema for which to set the current dictionary.
 * \param dict The dictionary to be set as current. NULL is valid, and indicates there is no current dictionary.
 *
 * \return A valid pointer to the schema.
 * \see RtDictGetCurrent
 */
RtDictSchema *
RtDictSchemaSetCurrentDict(RtDictSchema *schema, RtDict *dict)
{
    RWAPIFUNCTION(RWSTRING("RtDictSchemaSetCurrentDict"));
    RWASSERT(schema);

    schema->current = dict;

    RWRETURN(schema);
}


RtDict *
_rtDictSchemaInitDict(RtDictSchema *schema, RtDict *dict)
{
    RWFUNCTION(RWSTRING("_rtDictSchemaInitDict"));

    dict->entries = rwSListCreate(sizeof(RtDictEntryType),
                                  rwMEMHINTDUR_EVENT | schema->dictChunkType);

    if (!dict->entries)
    {
        RWRETURN((RtDict *)NULL);
    }

    dict->schema = schema;

    /* Add to global list of dictionaries */
    if (!RtDictSchemaAddDict(schema, dict))
    {
        rwSListDestroy(dict->entries);
        RWRETURN((RtDict *)NULL);
    }

    RWRETURN(dict);
}

RwBool
_rtDictDestruct(RtDict *dict)
{
    Iterator    iter;
    RWFUNCTION(RWSTRING("_rtDictDestruct"));
    RWASSERT(dict);

    /* Remove all the entries (handles ref counts) */
    IteratorInit(iter, dict->entries);
    while (IteratorValid(iter))
    {
        _rwPtrDictEntry ptrDictEntry;

        ptrDictEntry.ptrEntry = IteratorGetEntry(iter);

        IteratorNext(iter);

        (dict->schema->destroyCB)(ptrDictEntry.ptrEntry);
    }

    /* Remove the entries list itself */
    rwSListDestroy(dict->entries);

    RWRETURN(TRUE);
}

/**
 * \ingroup rtdict
 * \ref RtDictAddEntry
 * adds an entry to the dictionary, incrementing the reference count of that
 * entry.
 *
 * \param dict The dictionary the entry will to be added to
 * \param entry The entry to be added to the dictionary
 *
 * \return A valid pointer to the RtDict
 * \see RtDictForAllEntries
 * \see RtDictCreate
 */
RtDictEntryType
RtDictAddEntry(RtDict *dict, RtDictEntryType entry)
{
    RWAPIFUNCTION(RWSTRING("RtDictAddEntry"));
    RWASSERT(entry);
    RWASSERT(dict);

    /* Add to new dictionary, incrementing ref count */
    *(RtDictEntryType *)(rwSListGetNewEntry(
                              dict->entries,
                              rwMEMHINTDUR_EVENT | dict->schema->dictChunkType)
                        ) = entry;
    (dict->schema->addRefCB)(entry);

    RWRETURN(dict);
}


/**
 * \ingroup rtdict
 * \ref RtDictFindNamedEntry
 * finds an entry in a dictionary using the entry's name.
 *
 * \param dict The dictionary to be looked into.
 * \param name The name of the entry to look for.
 *
 * \return A valid pointer to an entry if the entry is found, NULL otherwise
 */
RtDictEntryType
RtDictFindNamedEntry(RtDict *dict, const RwChar *name)
{
    Iterator    iter;

    RWAPIFUNCTION(RWSTRING("RtDictFindNamedEntry"));
    RWASSERT(dict);
    RWASSERT(name);

    {
        RtDictEntryGetNameCallBack *getNameCB = dict->schema->getNameCB;

        IteratorInit(iter, dict->entries);
        while (IteratorValid(iter))
        {
            _rwPtrDictEntry ptrDictEntry;

            ptrDictEntry.constptrEntry = IteratorGetConstEntry(iter);

            if (0 == rwstrcmp((getNameCB)(ptrDictEntry.constptrEntry), name))
            {
                RWRETURN(ptrDictEntry.ptrEntry);
            }

            IteratorNext(iter);
        }
    }

    /* Not found */
    RWRETURN((RtDictEntryType) NULL);
}


/**
 * \ingroup rtdict
 * \ref RtDictForAllEntries
 * executes a callback for every entry in the dictionary.
 *
 * \param dict A pointer to the RtDict.
 * \param callBack The callback function to be executed for every entry
 * \param data The user specified data passed on to the callback function for each entry.
 *
 * \return A valid pointer to an RtDict.
 * \see RtDictSchemaForAllDictionaries
 */
const RtDict *
RtDictForAllEntries(const RtDict *dict, RtDictEntryCallBack *callBack, void *data)
{
    Iterator    iter;

    RWAPIFUNCTION(RWSTRING("RtDictForAllEntries"));
    RWASSERT(dict);
    RWASSERT(callBack);

    IteratorInit(iter, dict->entries);
    while (IteratorValid(iter))
    {
        _rwPtrDictEntry ptrDictEntry;

        ptrDictEntry.constptrEntry = IteratorGetConstEntry(iter);

        IteratorNext(iter);
        if (!callBack(ptrDictEntry.ptrEntry, data))
        {
            /* Early out */
            RWRETURN(dict);
        }
    }

    RWRETURN(dict);
}

/**
 * \ingroup rtdict
 * \ref RtDictRemoveEntry
 * removes an entry from the current dictionary. The entry's destroy function is called.
 * Consequently, if you wish to retain the entry you should AddRef it prior to calling
 * this function.
 *
 * \param dict The dictionary to remove the entry from
 * \param entry The entry to be removed
 *
 * \return  On success, the dictionary. NULL if the entry wasn't found in the dictionary.
 * \see RtDictAddEntry
 */
RtDict *
RtDictRemoveEntry(RtDict *dict, RtDictEntryType entry)
{
    Iterator    iter;
    RWAPIFUNCTION(RWSTRING("RtDictRemoveEntry"));
    RWASSERT(dict);
    RWASSERT(entry);

    IteratorInit(iter, dict->entries);
    while (IteratorValid(iter))
    {
        RtDictEntryType *dictEntry = &IteratorGetEntry(iter);

        if (*dictEntry == entry)
        {
            /* killref it */
            (dict->schema->destroyCB)(*dictEntry);

            /* Remove from list by replacing with last entry, then removing last entry*/
            *dictEntry = rwSListGetEntry(dict->entries, rwSListGetNumEntries(dict->entries)-1);
            rwSListDestroyEndEntries(dict->entries, 1);

            RWRETURN(dict);
            break;
        }

        IteratorNext(iter);
    }

    RWRETURN((RtDict *)NULL);
}


/**
 * \ingroup rtdict
 * \ref RtDictDestroy
 * destroys a dictionary. The reference counts of all the entries contained in the
 * dictionary are decreased via their destroy functions and the memory, associated
 * with the dictionary, freed.
 * If this dictionary is the current dictionary in its schema, the current dictionary
 * is set to NULL.
 *
 * \param dict The dictionary to be destroyed
 *
 * \return TRUE if successful, FALSE otherwise
 * \see RtDictCreate
 * \see RtDictSchemaSetCurrentDict
 */
RwBool
RtDictDestroy(RtDict *dict)
{
    RWAPIFUNCTION(RWSTRING("RtDictDestroy"));
    RWASSERT(dict);

    /* Unset current dict if it's this one */
    if (dict == dict->schema->current)
    {
        dict->schema->current = (RtDict *) NULL;
    }

    /* Remove from schema's list of dictionaries */
    RtDictSchemaRemoveDict(dict->schema, dict);

    /* Destruct the innards of the dictionary */
    _rtDictDestruct(dict);

    /* Free the memory */
    RwFree(dict);

    RWRETURN(TRUE);
}


/**
 * \ingroup rtdict
 * \ref RtDictStreamGetSize
 * gets the size of a dictionary, should it be streamed out.
 *
 * \param dict The dictionary for which to obtain the size when streamed.
 *
 * \return The size of the dictionary in a stream
 * \see RtDictStreamReadDict
 * \see RtDictStreamWrite
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RwStreamFindChunk
 */
RwUInt32
RtDictStreamGetSize(const RtDict *dict)
{
    RwUInt32    size;
    Iterator    iter;

    RWAPIFUNCTION(RWSTRING("RtDictStreamGetSize"));
    RWASSERT(dict);

    /* First get the size of the dictionary without the entries */
    size = rwCHUNKHEADERSIZE + sizeof(BinaryDict);

    /* Now find out the size of all the entries */
    IteratorInit(iter, dict->entries);
    while (IteratorValid(iter))
    {
        RtDictEntryConstType entry = IteratorGetConstEntry(iter);

        size += rwCHUNKHEADERSIZE + (dict->schema->streamGetSizeCB)(entry);

        IteratorNext(iter);
    }

    RWRETURN(size);
}

/**
 * \ingroup rtdict
 * \ref RtDictSchemaStreamReadDict
 * reads a dictionary from a stream.
 *
 * \param schema The schema for the dictionary to be created
 * \param stream The stream the dictionary is to be read from.
 *
 * \return A valid pointer to a RtDict if successful, NULL otherwise.
 *
 * \see RtDictStreamWrite
 * \see RtDictStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RwStreamFindChunk
 *
 */
RtDict *
RtDictSchemaStreamReadDict(RtDictSchema *schema, RwStream *stream)
{
    RwUInt32    size, version;
    BinaryDict  binDict;
    RtDict     *dict;

    RWAPIFUNCTION(RWSTRING("RtDictSchemaStreamReadDict"));
    RWASSERT(schema);
    RWASSERT(stream);

    /* Read header */
    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version))
    {
        RWRETURN((RtDict *)NULL);
    }

    if (version<schema->compatibilityVersion)
    {
        RWRETURN(schema->streamReadCompatibilityCB(stream));
    }

    if (!RwStreamRead(stream, &binDict, size))
    {
        RWRETURN((RtDict *)NULL);
    }

    (void)RwMemNative32(&binDict, sizeof(binDict));

    /* Create a new dictionary */
    dict = RtDictSchemaCreateDict(schema);
    if (!dict)
    {
        RWRETURN((RtDict *)NULL);
    }

    /* Read in the entries */
    while (binDict.size--)
    {
        RtDictEntryType entry;

        if (!RwStreamFindChunk(stream, schema->entryChunkType, &size, &version)
            || !(entry = (schema->streamReadCB)(stream)))
        {
            /* Tidy up and exit */
            RtDictDestroy(dict);
            RWRETURN((RtDict *)NULL);
        }

        RtDictAddEntry(dict, entry);

        /* Dict holds ref count */
        (schema->destroyCB)(entry);
    }

    /* And we're all done */
    RWRETURN(dict);
}

/**
 * \ingroup rtdict
 * \ref RtDictStreamWrite
 * writes a dictionary to a stream.
 *
 * \param dict The RtDict to be written to the stream.
 * \param stream The \ref RwStream to be written to.
 *
 * \return A valid RtDict pointer if successful, NULL otherwise.
 * \see RtDictSchemaStreamReadDict
 * \see RtDictStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 */
const RtDict *
RtDictStreamWrite(const RtDict *dict, RwStream *stream)
{
    RwUInt32     length;
    BinaryDict   binDict;
    Iterator     iter;

    RWAPIFUNCTION(RWSTRING("RtDictStreamWrite"));
    RWASSERT(dict);
    RWASSERT(stream);

    length = RtDictStreamGetSize(dict);
    if (!length)
    {
        RWRETURN((const RtDict *)NULL);
    }

    if (!RwStreamWriteChunkHeader(stream, dict->schema->dictChunkType, length))
    {
        RWRETURN((const RtDict *)NULL);
    }

    if (!RwStreamWriteChunkHeader(stream, rwID_STRUCT, sizeof(RwUInt32)))
    {
        RWRETURN((const RtDict *)NULL);
    }

    /* Write out the header */
    binDict.size = 0;

    IteratorInit(iter, dict->entries);
    while(IteratorValid(iter))
    {
        binDict.size++;
        IteratorNext(iter);
    }

    (void)RwMemLittleEndian32(&binDict, sizeof(binDict));
    if (!RwStreamWrite(stream, &binDict, sizeof(binDict)))
    {
        RWRETURN((const RtDict *)NULL);
    }

    /* Write all the entries */
    IteratorInit(iter, dict->entries);
    while(IteratorValid(iter))
    {
        RtDictEntryConstType entry = IteratorGetConstEntry(iter);

        if (!(dict->schema->streamWriteCB)(entry, stream))
        {
            RWRETURN((const RtDict *) NULL);
        }

        IteratorNext(iter);
    }

    /* And we're all done */
    RWRETURN(dict);
}

/**
 * \ingroup rtdict
 * \ref RtDictSchemaForAllDictionaries
 * executes a callback function for all active dictionaries. The callback function
 * can have specified user datas.
 *
 * \param schema The schema for the set of dictionaries to be iterated over
 * \param callBack The callback function to be executed for all dictionaries.
 * \param data The user specified data passed on to the callback function for each dictionary.
 *
 * \return A \ref RwBool (always TRUE at the moment)
 * \see RtDictForAllEntries
 */
RwBool
RtDictSchemaForAllDictionaries(RtDictSchema *schema, RtDictCallBack *callBack, void *data)
{
    Iterator    iter;

    RWAPIFUNCTION(RWSTRING("RtDictSchemaForAllDictionaries"));
    RWASSERT(schema);
    RWASSERT(callBack);

    IteratorInit(iter, schema->dictionaries);
    while (IteratorValid(iter))
    {
        _rwPtrDict ptrDict;

        ptrDict.constptrDict = IteratorGetConstDict(iter);

        IteratorNext(iter);
        if (!callBack(ptrDict.ptrDict, data))
        {
            /* Early out */
            RWRETURN(TRUE);
        }
    }

    RWRETURN(TRUE);
}

