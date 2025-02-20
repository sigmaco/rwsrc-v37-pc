/*
 *  multiTexEffect.c
 *
 *  Multi-texturing effects: 'base class' functionality.
 */

/******************************************************************************
 *  Includes
 */
#include <string.h>

#include "rpplugin.h"
#include "rpdbgerr.h"
#include "rwcore.h"
#include "rpworld.h"

#include "multiTex.h"
#include "multiTexEffect.h"

/******************************************************************************
 *  Defines
 */
#define MAXPLATFORMID  (rwID_PCD3D9)
#define DEFAULTPATHSIZE 256

/******************************************************************************
 *  Local types
 */

typedef union RpPtrMTEffect RpPtrMTEffect;
union RpPtrMTEffect
{
    RpMTEffect       *ptrMTEffect;
    const RpMTEffect *constptrMTEffect;
};

typedef union RpPtrMTEffectDict RpPtrMTEffectDict;
union RpPtrMTEffectDict
{
    RpMTEffectDict       *ptrMTEffectDict;
    const RpMTEffectDict *constptrMTEffectDict;
};

typedef struct EffectRegEntry EffectRegEntry;
struct EffectRegEntry
{
    RwPlatformID    platformID;

    rpMTEffectDestroyCallBack       destroy;
    rpMTEffectStreamReadCallBack    streamRead;
    rpMTEffectStreamWriteCallBack   streamWrite;
    rpMTEffectStreamGetSizeCallBack streamGetSize;
};

typedef struct BinaryEffectDict BinaryEffectDict;
struct BinaryEffectDict
{
    RwUInt32    numEffects;
};

typedef struct BinaryEffect BinaryEffect;
struct BinaryEffect
{
    RwUInt32    platformID;
};

typedef struct Iterator Iterator;
struct Iterator
{
    const RwLLLink *cur;
    const RwLLLink *end;
};

/******************************************************************************
 *  Local variables
 */
static EffectRegEntry   EffectRegEntries[MAXPLATFORMID+1];

static RpMTEffectDict   *DummyDict = (RpMTEffectDict *) NULL;

/******************************************************************************
 *  Macros
 */

#define rwLLLinkGetConstData(linkvar,type,entry)        \
    ((const type *)(((const RwUInt8 *)(linkvar))-offsetof(type,entry)))

/* Linked-list iterator */
#define IteratorInit(_iter, _list)                      \
MACRO_START                                             \
{                                                       \
    (_iter).end = rwLinkListGetTerminator(&(_list));    \
    (_iter).cur = rwLinkListGetFirstLLLink(&(_list));   \
}                                                       \
MACRO_STOP

#define IteratorValid(_iter)     ((_iter).cur != (_iter).end)
#define IteratorNext(_iter)      ((_iter).cur = rwLLLinkGetNext((_iter).cur))

#define IteratorGetEffect(_iter) \
            (rwLLLinkGetData((_iter).cur, RpMTEffect, dictLink))

#define IteratorGetDict(_iter)   \
            (rwLLLinkGetData((_iter).cur, RpMTEffectDict, dictListLink))

#define IteratorGetConstEffect(_iter) \
            (rwLLLinkGetConstData((_iter).cur, RpMTEffect, dictLink))

#define IteratorGetConstDict(_iter)   \
            (rwLLLinkGetConstData((_iter).cur, RpMTEffectDict, dictListLink))

/*
 *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *      SPI functions.
 *
 *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/******************************************************************************
 */
RwBool
_rpMTEffectSystemInit()
{
    RWFUNCTION(RWSTRING("_rpMTEffectSystemInit"));

    /* Initialize the registry */
    memset(EffectRegEntries, 0, sizeof(EffectRegEntries));

    RWRETURN(TRUE);
}

/******************************************************************************
 */
RwBool
_rpMTEffectRegisterPlatform(RwPlatformID  platformID,
                            rpMTEffectStreamReadCallBack      streamRead,
                            rpMTEffectStreamWriteCallBack     streamWrite,
                            rpMTEffectStreamGetSizeCallBack   streamGetSize,
                            rpMTEffectDestroyCallBack         destroy)
{
    EffectRegEntry  *regEntry;

    RWFUNCTION(RWSTRING("_rpMTEffectRegisterPlatform"));
    RWASSERT(platformID && platformID < MAXPLATFORMID);
    RWASSERT(streamRead);
    RWASSERT(streamWrite);
    RWASSERT(streamGetSize);

    regEntry = &EffectRegEntries[platformID];
    regEntry->platformID = platformID;
    regEntry->streamRead = streamRead;
    regEntry->streamWrite = streamWrite;
    regEntry->streamGetSize = streamGetSize;
    regEntry->destroy = destroy;

    RWRETURN(TRUE);
}

/******************************************************************************
 */
RwBool
_rpMTEffectOpen()
{
    RwUInt32    pathSize, totalSize;
    RwChar     *path;

    RWFUNCTION(RWSTRING("_rpMTEffectOpen"));

    rwLinkListInitialize(&RPMULTITEXTUREGLOBAL(effect.dictList));

    /* Create a default dictionary */
    DummyDict = RpMTEffectDictCreate();
    if (!DummyDict)
    {
        RWRETURN(FALSE);
    }

    RPMULTITEXTUREGLOBAL(effect.currentDict) = DummyDict;

    /* Set up a default effects path and scratch mem for filenames */
    pathSize = sizeof(RwChar) * DEFAULTPATHSIZE;
    totalSize = 2*pathSize + sizeof(RwChar) * rpMTEFFECTNAMELENGTH;
    path = (RwChar *) RwMalloc(totalSize,
        rwID_MULTITEXPLUGIN | rwMEMHINTDUR_GLOBAL);
    if (!path)
    {
        RpMTEffectDictDestroy(RPMULTITEXTUREGLOBAL(effect.currentDict));
        RWERROR((E_RW_NOMEM, totalSize));
        RWRETURN(FALSE);
    }

    memset(path, 0, totalSize);
    RPMULTITEXTUREGLOBAL(effect.path) = path;
    RPMULTITEXTUREGLOBAL(effect.scratchMem) = path + DEFAULTPATHSIZE;
    RPMULTITEXTUREGLOBAL(effect.pathSize) = pathSize;

    RWRETURN(TRUE);
}

/******************************************************************************
 */
RwBool
_rpMTEffectClose()
{
    Iterator    iter;
    RwBool      effectDictsExistOnShutdown;

    RWFUNCTION(RWSTRING("_rpMTEffectClose"));

    /* Free the effect path and scratch mem */
    if (RPMULTITEXTUREGLOBAL(effect.path))
    {
        RwFree(RPMULTITEXTUREGLOBAL(effect.path));
        RPMULTITEXTUREGLOBAL(effect.path) = (RwChar *) NULL;
        RPMULTITEXTUREGLOBAL(effect.scratchMem) = (RwChar *) NULL;
        RPMULTITEXTUREGLOBAL(effect.pathSize) = 0;
    }

    /*
     *  Look for the dummy dictionary we created in _rpMTEffectOpen and
     *  destroy it if it still exists. Any other dictionaries should have
     *  been destroyed by the app (where they must have been created), so
     *  assert if there are any.
     */
    IteratorInit(iter, RPMULTITEXTUREGLOBAL(effect.dictList));
    while (IteratorValid(iter))
    {
        const RpMTEffectDict *dict = IteratorGetConstDict(iter);

        if (dict == DummyDict)
        {
            RpMTEffectDictDestroy(DummyDict);
            DummyDict = (RpMTEffectDict *) NULL;
            break;
        }

        IteratorNext(iter);
    }

    /* Check if anything left in the list */
    effectDictsExistOnShutdown =
       !rwLinkListEmpty(&RPMULTITEXTUREGLOBAL(effect.dictList));

    RWASSERT(FALSE == effectDictsExistOnShutdown);
    RWRETURN(TRUE);
}

/******************************************************************************
 */
RpMTEffect *
_rpMTEffectInit(RpMTEffect *effect, RwPlatformID platformID)
{
    RWFUNCTION(RWSTRING("_rpMTEffectInit"));
    RWASSERT(effect);

    memset(effect, 0, sizeof(RpMTEffect));

    effect->platformID = platformID;
    effect->refCount = 1;
    rwLLLinkInitialize(&effect->dictLink);

    /*
     * Add the effect to the current dictionary unless this
     * is a dummy effect (no platform ID).
     */
    if (platformID && RPMULTITEXTUREGLOBAL(effect.currentDict))
    {
        RpMTEffectDictAddEffect(
            RPMULTITEXTUREGLOBAL(effect.currentDict), effect);
    }

    RWRETURN(effect);
}

/*
 *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *      API functions
 *
 *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectSetPath
 * adds a path to the search paths for the effect and effects dictionary files
 * \param path The path to be added to the search paths
 * \return A valid \ref RwChar pointer if successful, NULL otherwise
 * \see RpMTEffectGetPath
 */
const RwChar *
RpMTEffectSetPath(const RwChar *path)
{
    RwInt32 newPathSize;

    RWAPIFUNCTION(RWSTRING("RpMTEffectSetPath"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(path);

    /* How big is the string (incl NULL) */
    newPathSize = (rwstrlen(path) + 1) * sizeof(RwChar);

    /* Is the buffer big enough */
    if (newPathSize > RPMULTITEXTUREGLOBAL(effect.pathSize))
    {
        RwUInt32    totalSize;
        RwChar     *newPath;

        /* Need to realloc */
        totalSize = 2*newPathSize + sizeof(RwChar)*rpMTEFFECTNAMELENGTH;
        newPath =  (RwChar *)
            RwRealloc(RPMULTITEXTUREGLOBAL(effect.path), totalSize,
                      rwID_MULTITEXPLUGIN | rwMEMHINTDUR_GLOBAL |
                      rwMEMHINTFLAG_RESIZABLE);

        if (!newPath)
        {
            RWERROR((E_RW_NOMEM, totalSize));
            RWRETURN((const char *)NULL);
        }

        RPMULTITEXTUREGLOBAL(effect.path) = newPath;
        RPMULTITEXTUREGLOBAL(effect.scratchMem) =
            (RwChar *)((RwUInt8*)newPath + newPathSize);
        RPMULTITEXTUREGLOBAL(effect.pathSize) = newPathSize;
    }

    /* Copy the path (including NUL terminator) */
    memcpy(RPMULTITEXTUREGLOBAL(effect.path), path, newPathSize);

    RWRETURN(path);
}

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectGetPath
 * get the current search paths for effect files and effects dictionary files
 * \return A valid \ref RwChar pointer to the path
 * \see RpMTEffectSetPath
 */
RwChar *
RpMTEffectGetPath(void)
{
    RWAPIFUNCTION(RWSTRING("RpMTEffectGetPath"));
    RWASSERT(_rpMultiTextureModule.numInstances);

    RWRETURN(RPMULTITEXTUREGLOBAL(effect.path));
}

/******************************************************************************
 *  RpMTEffectDict functions
 */

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectDictStreamGetSize
 * gets the size of the effect dictionary file for streaming.
 * \param dict The RpMTEffectDict we need the information for.
 * \return A \ref RwUInt32, the size of the effect dictionary.
 * \see RpMTEffectDictStreamWrite
 * \see RpMTEffectDictStreamRead
 */
RwUInt32
RpMTEffectDictStreamGetSize(const RpMTEffectDict *dict)
{
    RwUInt32    size;
    Iterator    iter;

    RWAPIFUNCTION(RWSTRING("RpMTEffectDictStreamGetSize"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(dict);

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe Check
	THREADSAFE_CHECK_ISCALLEDMAIN();
	//@} DDonSS

    /* First get the size of the effectDictionary without the extension chunks */
    size = rwCHUNKHEADERSIZE + sizeof(BinaryEffectDict);

    /* Now find out the size of all the textures */
    IteratorInit(iter, dict->effectList);
    while (IteratorValid(iter))
    {
        const RpMTEffect *effect = IteratorGetConstEffect(iter);

        size += rwCHUNKHEADERSIZE + RpMTEffectStreamGetSize(effect);

        IteratorNext(iter);
    }

    RWRETURN(size);
}

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectDictStreamWrite
 * writes an effect dictionary to a stream.
 * \param dict The RpMTEffectDict to be written to the stream.
 * \param stream The \ref RwStream to be written to.
 * \return A valid RpMTEffectDict pointer if successful, NULL otherwise.
 * \see RpMTEffectDictStreamRead
 * \see RpMTEffectDictStreamGetSize
 */
const RpMTEffectDict *
RpMTEffectDictStreamWrite(const RpMTEffectDict *dict, RwStream *stream)
{
    RwUInt32            length;
    BinaryEffectDict    binDict;
    Iterator            iter;

    RWAPIFUNCTION(RWSTRING("RpMTEffectDictStreamWrite"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(dict);
    RWASSERT(stream);

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe Check
	THREADSAFE_CHECK_ISCALLEDMAIN();
	//@} DDonSS

    length = RpMTEffectDictStreamGetSize(dict);
    if (!length)
    {
        RWRETURN((const RpMTEffectDict *)NULL);
    }

    if (!RwStreamWriteChunkHeader(stream, rwID_MTEFFECTDICT, length))
    {
        RWRETURN((const RpMTEffectDict *)NULL);
    }

    if (!RwStreamWriteChunkHeader(stream, rwID_STRUCT, sizeof(RwUInt32)))
    {
        RWRETURN((const RpMTEffectDict *)NULL);
    }

    /* Write out the header */
    binDict.numEffects = 0;

    IteratorInit(iter, dict->effectList);
    while(IteratorValid(iter))
    {
        binDict.numEffects++;
        IteratorNext(iter);
    }

    (void)RwMemLittleEndian32(&binDict, sizeof(binDict));
    if (!RwStreamWrite(stream, &binDict, sizeof(binDict)))
    {
        RWRETURN((const RpMTEffectDict *)NULL);
    }

    /* Write all the effects */
    IteratorInit(iter, dict->effectList);
    while(IteratorValid(iter))
    {
        const RpMTEffect *effect = IteratorGetConstEffect(iter);

        if (!RpMTEffectStreamWrite(effect, stream))
        {
            RWRETURN((const RpMTEffectDict *) NULL);
        }

        IteratorNext(iter);
    }

    /* And we're all done */
    RWRETURN(dict);
}

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectDictStreamRead
 * reads an effect dictionary from a stream.
 *
 * The sequence to locate and read a multi-texture effect dictionary
 * from a binary stream is as follows:
 * \code
   RwStream *stream;
   RpMTEffectDict *newMTEffectDict;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_MTEFFECTDICT, NULL, NULL) )
       {
           newMTEffectDict = RpMTEffectDictStreamRead(stream);
       }

       RwStreamClose(stream, NULL);
   }
   \endcode
 *
 * \param stream The stream the effect dicitonary is to be read from.
 *
 * \return A valid pointer to a RpMTEffectDict if successful, NULL otherwise.
 *
 * \see RpMTEffectDictStreamWrite
 * \see RpMTEffectDictStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RwStreamFindChunk
 *
 */
RpMTEffectDict *
RpMTEffectDictStreamRead(RwStream *stream)
{
    RwUInt32            size, version;
    BinaryEffectDict    binDict;
    RpMTEffectDict     *dict;

    RWAPIFUNCTION(RWSTRING("RpMTEffectDictStreamRead"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(stream);

    /* Read header */
    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version))
    {
        RWRETURN((RpMTEffectDict *)NULL);
    }

    if (!RwStreamRead(stream, &binDict, size))
    {
        RWRETURN((RpMTEffectDict *)NULL);
    }

    (void)RwMemNative32(&binDict, sizeof(binDict));

    /* Create a new dictionary */
    dict = RpMTEffectDictCreate();
    if (!dict)
    {
        RWRETURN((RpMTEffectDict *)NULL);
    }

    /* Read in the effects */
    while (binDict.numEffects--)
    {
        RpMTEffect  *effect;

        if (!RwStreamFindChunk(stream, rwID_MTEFFECTNATIVE, &size, &version)
            || !(effect = RpMTEffectStreamRead(stream)))
        {
            /* Tidy up and exit */
            RpMTEffectDictDestroy(dict);
            RWRETURN((RpMTEffectDict *)NULL);
        }

        RpMTEffectDictAddEffect(dict, effect);

        /* Dict holds ref count */
        RpMTEffectDestroy(effect);
    }

    /* And we're all done */
    RWRETURN(dict);
}

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectDictCreate
 * creates an empty effect dictionary. \ref RpMTEffectDictAddEffect can be used to add
 * effects to the effect dictionary
 * \return A valid pointer to a RpMTEffectDict if successful, NULL otherwise
 * \see RpMTEffectDictDestroy
 * \see RpMTEffectDictAddEffect
 */
RpMTEffectDict *
RpMTEffectDictCreate(void)
{
    RpMTEffectDict     *dict;
    RwUInt32            size;

    RWAPIFUNCTION(RWSTRING("RpMTEffectDictCreate"));
    RWASSERT(_rpMultiTextureModule.numInstances);

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe Check
	THREADSAFE_CHECK_ISCALLEDMAIN();
	//@} DDonSS

    /* Allocate the memory */
    size = sizeof(RpMTEffectDict);
    dict = (RpMTEffectDict *) RwMalloc(size,
        rwID_MULTITEXPLUGIN | rwMEMHINTDUR_EVENT);
    if (!dict)
    {
        RWERROR((E_RW_NOMEM, size));
        RWRETURN((RpMTEffectDict *) NULL);
    }

    rwLinkListInitialize(&dict->effectList);

    /* Add to global list of dictionaries */
    rwLinkListAddLLLink(
        &RPMULTITEXTUREGLOBAL(effect.dictList), &dict->dictListLink);

    RWRETURN(dict);
}

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectDictDestroy
 * destroys an effect dictionary. The reference counts of all the effects contained in the
 * dictionary are decreased and the memory associated with the dictionary freed.
 * \param dict The dictionary to be destroyed
 * \see RpMTEffectDictCreate
 */
void
RpMTEffectDictDestroy(RpMTEffectDict *dict)
{
    Iterator    iter;

    RWAPIFUNCTION(RWSTRING("RpMTEffectDictDestroy"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(dict);

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe Check
	THREADSAFE_CHECK_ISCALLEDMAIN();
	//@} DDonSS

    /* Unset current dict if it's this one */
    if (dict == RPMULTITEXTUREGLOBAL(effect.currentDict))
    {
        RPMULTITEXTUREGLOBAL(effect.currentDict) = (RpMTEffectDict *) NULL;
    }

    /* Remove all the effects (handles ref counts) */
    IteratorInit(iter, dict->effectList);
    while (IteratorValid(iter))
    {
        RpPtrMTEffect PtrMTEffect;

        PtrMTEffect.constptrMTEffect = IteratorGetConstEffect(iter);

        IteratorNext(iter);

        RpMTEffectDictRemoveEffect(PtrMTEffect.ptrMTEffect);
    }

    /* Remove from global list of dictionaries */
    rwLinkListRemoveLLLink(&dict->dictListLink);

    /* Free the memory */
    RwFree(dict);

    RWRETURNVOID();
}

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectDictForAllDictionaries
 * executes a callback function for all active effect dictionaries. The callback function
 * can have specified user datas.
 * \param callBack The callback function to be executed for all dictionaries.
 * \param data The user specified data passed on to the callback function for each dictionary.
 * \return A \ref RwBool (always TRUE at the moment)
 * \see RpMTEffectDictForAllEffects
 */
RwBool
RpMTEffectDictForAllDictionaries(RpMTEffectDictCallBack callBack,
                                 void *data)
{
    Iterator    iter;

    RWAPIFUNCTION(RWSTRING("RpMTEffectDictForAllDictionaries"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(callBack);

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe Check
	THREADSAFE_CHECK_ISCALLEDMAIN();
	//@} DDonSS

    IteratorInit(iter, RPMULTITEXTUREGLOBAL(effect.dictList));
    while (IteratorValid(iter))
    {
        RpPtrMTEffectDict PtrMTEffectDict;

        PtrMTEffectDict.constptrMTEffectDict = IteratorGetConstDict(iter);

        IteratorNext(iter);
        if (!callBack(PtrMTEffectDict.ptrMTEffectDict, data))
        {
            /* Early out */
            RWRETURN(TRUE);
        }
    }

    RWRETURN(TRUE);
}

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectDictForAllEffects
 * executes a callback for every effect contained in an effect dictionary.
 * \param dict A pointer to the RpMTEffectDict.
 * \param callBack The callback function to be executed for every effect
 * \param data The user specified data passed on to the callback function for each effect.
 * \return A valid pointer to a RpMTEffectDict.
 * \see RpMTEffectDictForAllDictionaries
 */
const RpMTEffectDict *
RpMTEffectDictForAllEffects(const RpMTEffectDict *dict,
                            RpMTEffectCallBack    callBack,
                            void                 *data)
{
    Iterator    iter;

    RWAPIFUNCTION(RWSTRING("RpMTEffectDictForAllEffects"));
    RWASSERT(_rpMultiTextureModule.numInstances);;
    RWASSERT(dict);
    RWASSERT(callBack);

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe Check
	THREADSAFE_CHECK_ISCALLEDMAIN();
	//@} DDonSS
	
    IteratorInit(iter, dict->effectList);
    while (IteratorValid(iter))
    {
        RpPtrMTEffect PtrMTEffect;

        PtrMTEffect.constptrMTEffect = IteratorGetConstEffect(iter);

        IteratorNext(iter);
        if (!callBack(PtrMTEffect.ptrMTEffect, data))
        {
            /* Early out */
            RWRETURN(dict);
        }
    }

    RWRETURN(dict);
}

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectDictAddEffect
 * adds an effect to an effect dictionary. The dictionary must have been initialized before
 * calling this function
 * \param dict The effect dictionary the effect it to be added to
 * \param effect The effect to be added to the effect dictionary
 * \return A valid pointer to the RpMTEffectDict
 * \see RpMTEffectDictForAllEffects
 * \see RpMTEffectDictCreate
 */
RpMTEffectDict *
RpMTEffectDictAddEffect(RpMTEffectDict *dict, RpMTEffect *effect)
{
    RWAPIFUNCTION(RWSTRING("RpMTEffectDictAddEffect"));
    RWASSERT(_rpMultiTextureModule.numInstances);;
    RWASSERT(effect);
    RWASSERT(dict);

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe Check
	THREADSAFE_CHECK_ISCALLEDMAIN();
	//@} DDonSS

    /* Is it already in a dictionary? */
    if (effect->dictLink.next)
    {
        /* Remove and kill ref */
        rwLinkListRemoveLLLink(&effect->dictLink);
        RpMTEffectDestroy(effect);
    }

    /* Add to new dictionary, incrementing ref count */
    rwLinkListAddLLLink(&dict->effectList, &effect->dictLink);
    RpMTEffectAddRef(effect);

    RWRETURN(dict);
}

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectDictRemoveEffect
 * removes an effect from the current effect dictionary. The effect's reference count is decreased.
 * If the effect's reference count reaches zero, the effect is destroyed
 * \param effect The effect to be removed
 * \return A valid pointer to the RpMTEffect if its refence count is not zero, NULL
 * otherwise
 * \see RpMTEffectDictAddEffect
 */
RpMTEffect *
RpMTEffectDictRemoveEffect(RpMTEffect *effect)
{
    RWAPIFUNCTION(RWSTRING("RpMTEffectDictRemoveEffect"));
    RWASSERT(_rpMultiTextureModule.numInstances);;
    RWASSERT(effect);

    /* Is it in a dictionary? */
    if (effect->dictLink.next)
    {
        /* Remove and kill ref */
        rwLinkListRemoveLLLink(&effect->dictLink);
        RpMTEffectDestroy(effect);
    }

    RWRETURN(effect);
}

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectDictFindNamedEffect
 * finds an effect in an effect dictionary using the effect's name.
 * \param dict The effect dictionary to be looked into.
 * \param name The name of the effect to look for.
 * \return A valid pointer to a RpMTEffect if the effect is found,
 * NULL otherwise
 * \see RpMTEffectFind
 * \see RpMTEffectSetName
 */
RpMTEffect *
RpMTEffectDictFindNamedEffect(const RpMTEffectDict *dict,
                              const RwChar *name)
{
    Iterator    iter;

    RWAPIFUNCTION(RWSTRING("RpMTEffectDictFindNamedEffect"));
    RWASSERT(_rpMultiTextureModule.numInstances);;
    RWASSERT(dict);
    RWASSERT(name);

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe Check
	THREADSAFE_CHECK_ISCALLEDMAIN();
	//@} DDonSS

    IteratorInit(iter, dict->effectList);
    while (IteratorValid(iter))
    {
        RpPtrMTEffect PtrMTEffect;

        PtrMTEffect.constptrMTEffect = IteratorGetConstEffect(iter);

        if (0 == rwstrcmp(PtrMTEffect.constptrMTEffect->name, name))
        {
            RWRETURN(PtrMTEffect.ptrMTEffect);
        }

        IteratorNext(iter);
    }

    /* Not found */
    RWRETURN((RpMTEffect *) NULL);
}

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectDictSetCurrent
 * Sets the current effect dictionary.
 * \param dict The dictionary to be set as current
 * \return A valid pointer to a RpMTEffectDict
 * \see RpMTEffectDictGetCurrent
 */
RpMTEffectDict *
RpMTEffectDictSetCurrent(RpMTEffectDict *dict)
{
    RWAPIFUNCTION(RWSTRING("RpMTEffectDictSetCurrent"));
    RWASSERT(_rpMultiTextureModule.numInstances);

    RPMULTITEXTUREGLOBAL(effect.currentDict) = dict;

    RWRETURN(dict);
}

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectDictGetCurrent
 * Gets the current dictionary.
 * \return A pointer to the current RpMTEffectDict.
 * \see RpMTEffectDictSetCurrent
 */
RpMTEffectDict *
RpMTEffectDictGetCurrent(void)
{
    RWAPIFUNCTION(RWSTRING("RpMTEffectDictGetCurrent"));
    RWASSERT(_rpMultiTextureModule.numInstances);;

    RWRETURN(RPMULTITEXTUREGLOBAL(effect.currentDict));
}

/******************************************************************************
 *  RpMTEffect functions
 */

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectCreateDummy
 * Creates a dummy effect. The effect is not named and the initial setup is a
 * default one.
 * \return A pointer to the created RpMTEffect
 * \see RpMTEffectSetName
 * \see RpMTEffectDestroy
 */
RpMTEffect *
RpMTEffectCreateDummy(void)
{
    RpMTEffect   *effect;
    RwUInt32                size;

    RWAPIFUNCTION(RWSTRING("RpMTEffectCreateDummy"));
    RWASSERT(_rpMultiTextureModule.numInstances);

    /* Allocate memory */
    size = sizeof(RpMTEffect);
    effect = (RpMTEffect *) RwMalloc(size,
        rwID_MULTITEXPLUGIN | rwMEMHINTDUR_EVENT);
    if (!effect)
    {
        RWERROR((E_RW_NOMEM, size));
        RWRETURN((RpMTEffect *) NULL);
    }

    /* Initialize the data */
    _rpMTEffectInit(effect, (RwPlatformID) 0);

    RWRETURN(effect);
}

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectDestroy
 * destroys the effect if it's the last reference to it, otherwise decreases the
 * reference count of the effect.
 * \param effect The effect to be destroyed.
 * \see RpMTEffectCreateDummy
 */
void
RpMTEffectDestroy(RpMTEffect *effect)
{
    RWAPIFUNCTION(RWSTRING("RpMTEffectDestroy"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(effect);

    --effect->refCount;

    /* Is this the last reference? */
    if (0 == effect->refCount)
    {
        RpMTEffectDictRemoveEffect(effect);

        if (effect->platformID)
        {
            /* Check for platform specific destructor */
            EffectRegEntry *regEntry = &EffectRegEntries[effect->platformID];
            RWASSERT(regEntry);
            if (regEntry->destroy)
            {
                regEntry->destroy(effect);
                RWRETURNVOID();
            }
        }

        /* It's just a single block of memory */
        RwFree(effect);
    }

    RWRETURNVOID();
}

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectStreamGetSize
 * Gets the size of the effect for streaming use.
 * \param effect The effect we want the size of.
 * \return The size of the effect, a \ref RwUInt32
 * \see RpMTEffectStreamWrite
 * \see RpMTEffectStreamRead
 */
RwUInt32
RpMTEffectStreamGetSize(const RpMTEffect *effect)
{
    EffectRegEntry *regEntry;
    RwInt32         size;

    RWAPIFUNCTION(RWSTRING("RpMTEffectStreamGetSize"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(effect);
    RWASSERT(effect->platformID);

    regEntry = &EffectRegEntries[effect->platformID];
    RWASSERT(regEntry->streamGetSize);

    /* Header size  - platform ID and name */
    size = rwCHUNKHEADERSIZE
         + sizeof(BinaryEffect)
         + rwCHUNKHEADERSIZE
         + _rwStringStreamGetSize(effect->name);

    /* Platform specific data */
    RWASSERT(regEntry->streamGetSize);
    size += rwCHUNKHEADERSIZE
         +  regEntry->streamGetSize(effect);

    RWRETURN(size);
}

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectStreamWrite
 * writes an effect to a stream.
 * \param effect The effect to be written
 * \param stream The \ref RwStream to write to.
 * \return A valid pointer to the RpMTEffect if successful, NULL otherwise.
 * \see RpMTEffectStreamGetSize
 * \see RpMTEffectStreamRead
 */
const RpMTEffect *
RpMTEffectStreamWrite(const RpMTEffect *effect, RwStream *stream)
{
    const RpMTEffect       *effectOut = NULL;
    RwInt32                 chunkSize;
    BinaryEffect            binEffect;
    EffectRegEntry  *regEntry;

    RWAPIFUNCTION(RWSTRING("RpMTEffectStreamWrite"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(effect);
    RWASSERT(stream);

    /* Write the header */
    chunkSize = RpMTEffectStreamGetSize(effect);
    binEffect.platformID = (RwInt32) effect->platformID;
    (void)RwMemLittleEndian32(&binEffect, sizeof(binEffect));

    if (   !RwStreamWriteChunkHeader(stream, rwID_MTEFFECTNATIVE, chunkSize)
        || !RwStreamWriteChunkHeader(stream, rwID_STRUCT, sizeof(binEffect))
        || !RwStreamWrite(stream, &binEffect, sizeof(binEffect)))
    {
        RWRETURN((RpMTEffect *) NULL);
    }

    /* Write the effect name */
    if (!_rwStringStreamWrite(effect->name, stream))
    {
        RWRETURN((RpMTEffect *) NULL);
    }

    /* Write platform specific extension */
    regEntry = &EffectRegEntries[effect->platformID];
    RWASSERT(regEntry->streamWrite);

    effectOut = regEntry->streamWrite(effect, stream);

    RWRETURN(effectOut);
}

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectStreamRead
 * reads an effect from a stream.
 *
 * The sequence to locate and read a multi-texture effect from a binary
 * stream is as follows:
 * \code
   RwStream *stream;
   RpMTEffect *newMTEffect;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_MTEFFECTNATIVE, NULL, NULL) )
       {
           newMTEffect = RpMTEffectStreamRead(stream);
       }

       RwStreamClose(stream, NULL);
   }
   \endcode
 *
 * \param stream The stream to read from
 *
 * \return A valid pointer to a RpMTEffect if successful, NULL otherwise.
 *
 * \see RpMTEffectStreamWrite
 * \see RpMTEffectStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RwStreamFindChunk
 */
RpMTEffect *
RpMTEffectStreamRead(RwStream *stream)
{
    BinaryEffect    binEffect;
    EffectRegEntry *regEntry;
    RpMTEffect     *effect;
    RwChar          name[rpMTEFFECTNAMELENGTH];
    RwUInt32        version, length;

    RWAPIFUNCTION(RWSTRING("RpMTEffectStreamRead"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(stream);

    /* Read header */
    if (   !RwStreamFindChunk(stream, rwID_STRUCT, NULL, NULL)
        || !RwStreamRead(stream, &binEffect, sizeof(binEffect)))
    {
        RWRETURN((RpMTEffect *) NULL);
    }

    /* Get registry entry for this platform */
    (void)RwMemNative32(&binEffect, sizeof(binEffect));
    RWASSERT(binEffect.platformID && (binEffect.platformID <= MAXPLATFORMID));
    regEntry = &EffectRegEntries[binEffect.platformID];
    if (!regEntry->streamRead)
    {
        RWRETURN((RpMTEffect *) NULL);
    }

    /* Read the effect name */
    if (!_rwStringStreamFindAndRead(name, stream))
    {
        RWRETURN((RpMTEffect *) NULL);
    }

    /* Now read platform specific extension */
    if (!RwStreamFindChunk(stream, rwID_EXTENSION, &length, &version))
    {
        RWRETURN((RpMTEffect *) NULL);
    }

    effect = regEntry->streamRead(stream, binEffect.platformID, version, length);
    if (!effect)
    {
        RWRETURN((RpMTEffect *) NULL);
    }

    /* Copy across the name */
    RpMTEffectSetName(effect, name);

    RWRETURN(effect);
}

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectFind
 * If an effect is found in a dictionary, it's reference count will be
 * increased, or otherwise will be created with a reference count of one.
 *
 * \param name The name of the effect to be found.
 * \return A valid pointer to the RpMTEffect if successful, NULL otherwise
 * \see RpMTEffectCreateDummy
 * \see RpMTEffectSetName
 */
RpMTEffect *
RpMTEffectFind(RwChar *name)
{
    RpMTEffect  *effect = (RpMTEffect *) NULL;
    RwStream    *stream;
    RwChar      *scratch;

    RWAPIFUNCTION(RWSTRING("RpMTEffectFind"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(name);
    RWASSERT(rwstrlen(name) < rpMTEFFECTNAMELENGTH);

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe Check
	THREADSAFE_CHECK_ISCALLEDMAIN();
	//@} DDonSS

    /* Look in the current effect dictionary if one is defined */
    if (RPMULTITEXTUREGLOBAL(effect.currentDict))
    {
        effect = RpMTEffectDictFindNamedEffect(
                    RPMULTITEXTUREGLOBAL(effect.currentDict), name);
    }
    else
    {
        Iterator    iter;

        /* Try searching all dictionaries */
        IteratorInit(iter, RPMULTITEXTUREGLOBAL(effect.dictList));
        while (IteratorValid(iter))
        {
            const RpMTEffectDict *dict = IteratorGetConstDict(iter);

            effect = RpMTEffectDictFindNamedEffect(dict, name);
            if (effect)
            {
                break;
            }

            IteratorNext(iter);
        }
    }

    /* Return if we found it */
    if (effect)
    {
        RpMTEffectAddRef(effect);
        RWRETURN(effect);
    }

    /* Otherwise try opening a file */
    scratch = RPMULTITEXTUREGLOBAL(effect.scratchMem);
    rwstrcpy(scratch, RPMULTITEXTUREGLOBAL(effect.path));
    rwstrncat(scratch, name, rpMTEFFECTNAMELENGTH - 1);
    stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, scratch);
    if (!stream)
    {
        RWRETURN((RpMTEffect *) NULL);
    }

    /* Look for native multitexture effect chunk */
    if (!RwStreamFindChunk(stream, rwID_MTEFFECTNATIVE, NULL, NULL))
    {
        RwStreamClose(stream, NULL);
        RWRETURN((RpMTEffect *) NULL);
    }

    /* Read effect - it will be automatically added to the dictionary */
    effect = RpMTEffectStreamRead(stream);
    RwStreamClose(stream, NULL);

    RWRETURN(effect);
}

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectWrite
 * writes an effect to a file. The name used for the file is the name of the effect,
 * as defined by \ref RpMTEffectSetName.
 * \param effect The effect to be written.
 * \return A valid pointer to the RpMTEffect if successful, NULL otherwise.
 * \see RpMTEffectSetName
 * \see RpMTEffectStreamWrite
 * \see RpMTEffectStreamGetSize
 * \see RpMTEffectStreamRead
 */
const RpMTEffect *
RpMTEffectWrite(const RpMTEffect *effect)
{
    const RpMTEffect   *effectOut = NULL;
    RwStream           *stream;

    RWAPIFUNCTION(RWSTRING("RpMTEffectWrite"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(effect);
    RWASSERT(rwstrlen(effect->name) > 0);
    RWASSERT(effect->platformID);

    /* Open the file */
    stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMWRITE, effect->name);
    if (stream)
    {
        /* Write the effect */
        effectOut = RpMTEffectStreamWrite(effect, stream);

        RwStreamClose(stream, NULL);
    }

    RWRETURN(effectOut);
}

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectSetName
 * sets the name of the effect. The name is used for writing the effect to a file
 * and find functions
 * \param effect The effect to be named
 * \param name The name of the effect
 * \return A valid pointer to the RpMTEffect
 * \see RpMTEffectWrite
 * \see RpMTEffectFind
 * \see RpMTEffectDictFindNamedEffect
 * \see RpMTEffectGetName
 */
RpMTEffect *
RpMTEffectSetName(RpMTEffect *effect, RwChar *name)
{
    RWAPIFUNCTION(RWSTRING("RpMTEffectSetName"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(effect);
    RWASSERT(name && rwstrlen(name) < rpMTEFFECTNAMELENGTH);

    rwstrncpy(effect->name, name, rpMTEFFECTNAMELENGTH - 1);

    RWRETURN(effect);
}

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectGetName
 * gets the name of the effect.
 * \param effect The effect to be queried.
 * \return A pointer to a \ref RwChar containing the name of the effect.
 * \see RpMTEffectSetName
 * \see RpMTEffectWrite
 * \see RpMTEffectFind
 * \see RpMTEffectDictFindNamedEffect
 */
RwChar *
RpMTEffectGetName(RpMTEffect *effect)
{
    RWAPIFUNCTION(RWSTRING("RpMTEffectGetName"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(effect);

    RWRETURN(effect->name);
}

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectAddRef
 * increases the reference count of the effect.
 * \param effect A pointer to the RpMTEffect.
 * \return A valid pointer to the RpMTEffect.
 * \see RpMTEffectDestroy
 */
RpMTEffect *
RpMTEffectAddRef(RpMTEffect *effect)
{
    RWAPIFUNCTION(RWSTRING("RpMTEffectAddRef"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(effect);

    ++effect->refCount;

    RWRETURN(effect);
}
