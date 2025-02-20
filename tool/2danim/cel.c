/*
 *
 * Functionality for 2D rendering
 *
 * Copyright (c) 2001 Criterion Software Ltd.
 */

/****************************************************************************
 *                                                                          *
 *  Module  :   cel.c                                                       *
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
#include "message.h"
#include "cel.h"
#include "button.h"
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

/****************************************************************************
 Functions.
 */

/****************************************************************************
 *
 * Cels.
 *
 ****************************************************************************/

Rt2dCel *
_rt2dCelInit(Rt2dCel *cel)
{
    RWFUNCTION(RWSTRING("_rt2dCelInit"));

    RWASSERT(cel);

    cel->strLabelIndex = -1;
    cel->celIndex = -1;
    cel->messageListIndex = -1;

    if ((cel->buttonIndices = _rwSListCreate(sizeof(RwInt32),
            rwID_2DANIM | rwMEMHINTDUR_EVENT)) == NULL)
    {
        RWRETURN((Rt2dCel *)NULL);
    }

    RWRETURN(cel);
}

Rt2dCel *
_rt2dCelDestruct(Rt2dCel *cel)
{
    RWFUNCTION(RWSTRING("_rt2dCelDestruct"));

    RWASSERT(cel);

    if (cel->buttonIndices)
    {
        _rwSListDestroy(cel->buttonIndices);
    }

    cel->buttonIndices = (RwSList *)NULL;

    cel->strLabelIndex = -1;
    cel->celIndex = -1;
    cel->messageListIndex = -1;

    RWRETURN(cel);
}

Rt2dCel *
_rt2dCelCopy(Rt2dCel *dstCel, Rt2dCel *srcCel)
{
    RwInt32             num, *srcButtonIndices, *dstButtonIndices;

    RWFUNCTION(RWSTRING("_rt2dCelCopy"));

    dstCel->strLabelIndex = srcCel->strLabelIndex;
    dstCel->celIndex = srcCel->celIndex;
    dstCel->messageListIndex = srcCel->messageListIndex;

    /* Are there any button indices to copy ? */
    if (srcCel->buttonIndices)
    {
        /* Allocate the memory in the dst cel */
        if (dstCel->buttonIndices == NULL)
        {
            if ((dstCel->buttonIndices = _rwSListCreate(sizeof(RwInt32),
                    rwID_2DANIM | rwMEMHINTDUR_EVENT)) == NULL)
            {
                RWRETURN((Rt2dCel *)NULL);
            }
        }

        num = _rwSListGetNumEntries(srcCel->buttonIndices);

        if (num > 0)
        {
            srcButtonIndices = (RwInt32 *)_rwSListGetArray(srcCel->buttonIndices);

            RWASSERT(srcButtonIndices);

            if ((dstButtonIndices = (RwInt32 *)_rwSListGetNewEntries(
                                                dstCel->buttonIndices, num,
                                                rwID_2DANIM |
                                                rwMEMHINTDUR_EVENT)) == NULL)
            {
                RWRETURN((Rt2dCel *)NULL);
            }

            memcpy(dstButtonIndices, srcButtonIndices, (num * sizeof(RwInt32)));
        }
    }

    RWRETURN(srcCel);
}

RwInt32
_rt2dCelStreamGetSize(Rt2dMaestro *maestro __RWUNUSEDRELEASE__, Rt2dCel *cel)
{
    RwInt32                 size, num;

    RWFUNCTION(RWSTRING("_rt2dCelStreamGetSize"));

    RWASSERT(maestro);
    RWASSERT(cel);

    /* String label index. */
    size = sizeof(RwInt32);

    /* The cell index. */
    size += sizeof(RwInt32);

    /* Count and size of the button indices. */
    num = _rwSListGetNumEntries(cel->buttonIndices);
    size += sizeof(RwInt32);
    size += num * sizeof(RwInt32);

    /* Message list index. */
    size += sizeof(RwInt32);

    RWRETURN(size);
}

Rt2dCel *
_rt2dCelStreamRead(Rt2dMaestro *maestro __RWUNUSEDRELEASE__, Rt2dCel *cel, RwStream *stream)
{
    RwInt32         num, *buttonIndices;

    RWFUNCTION(RWSTRING("_rt2dCelStreamRead"));

    RWASSERT(maestro);
    RWASSERT(stream);

    /* Read in the string label index. */
    if (RwStreamReadInt32(stream, &cel->strLabelIndex, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dCel *)NULL);
    }

    /* The cell index. */
    if (RwStreamReadInt32(stream, &cel->celIndex, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dCel *)NULL);
    }

    /* Read in the button index. */
    if (RwStreamReadInt32(stream, &num, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dCel *)NULL);
    }

    if (num > 0)
    {
        if (cel->buttonIndices == NULL)
        {
            cel->buttonIndices = _rwSListCreate(sizeof(RwInt32),
                rwID_2DANIM | rwMEMHINTDUR_EVENT);
        }

        if (cel->buttonIndices == NULL)
        {
            RWRETURN((Rt2dCel *)NULL);
        }

        if ((buttonIndices =
                (RwInt32 *)_rwSListGetNewEntries(cel->buttonIndices, num,
                    rwID_2DANIM | rwMEMHINTDUR_EVENT)) == NULL)
        {
            RWRETURN((Rt2dCel *)NULL);
        }

        if (RwStreamReadInt32(stream, buttonIndices, (num * sizeof(RwInt32))) == NULL)
        {
            RWRETURN((Rt2dCel *)NULL);
        }
    }

    /* Read in the message list index. */
    if (RwStreamReadInt32(stream, &cel->messageListIndex, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dCel *)NULL);
    }

    RWRETURN(cel);
}

Rt2dCel *
_rt2dCelStreamWrite(Rt2dMaestro *maestro __RWUNUSEDRELEASE__, Rt2dCel *cel, RwStream *stream)
{
    RwInt32         size, num, *buttonIndices;

    RWFUNCTION(RWSTRING("_rt2dCelStreamWrite"));

    RWASSERT(maestro);
    RWASSERT(cel);
    RWASSERT(stream);

    /* String label index. */
    if (RwStreamWriteInt32(stream, &cel->strLabelIndex, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dCel *)NULL);
    }

    /* The cell index. */
    if (RwStreamWriteInt32(stream, &cel->celIndex, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dCel *)NULL);
    }

    /* Count and size of the button indices. */
    num = _rwSListGetNumEntries(cel->buttonIndices);
    if (num > 0)
    {
        if (RwStreamWriteInt32(stream, &num, sizeof(RwInt32)) == NULL)
        {
            RWRETURN((Rt2dCel *)NULL);
        }

        buttonIndices = (RwInt32 *)_rwSListGetArray(cel->buttonIndices);
        if (RwStreamWriteInt32(stream, buttonIndices, (num * sizeof(RwInt32))) == NULL)
        {
            RWRETURN((Rt2dCel *)NULL);
        }
    }
    else
    {
        size = 0;
        if (RwStreamWriteInt32(stream, &size, sizeof(RwInt32)) == NULL)
        {
            RWRETURN((Rt2dCel *)NULL);
        }
    }

    /* Message list index. */
    if (RwStreamWriteInt32(stream, &cel->messageListIndex, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dCel *)NULL);
    }

    RWRETURN(cel);
}


/****************************************************************************/

/**
 * \ingroup rt2dcel
 * \ref Rt2dCelCreate creates a new empty cel. The name will be added to the
 * maestro's label table with entity type of rt2dANIMLABELTYPEFRAME. If a label
 * entry already exists in the maestro, then the new cel will not be created.
 *
 * \param maestro              Pointer to the parent maestro.
 * \param name                  Name of the new cel.
 * \param celIndex              Index of the cel.
 * \param messageListIndex      Index to list of messages for the cel.
 *
 * \return Returns a pointer to the new cel if successful, NULL otherwise.
 *
 * \see Rt2dCelDestroy
 */

Rt2dCel *
Rt2dCelCreate(Rt2dMaestro *maestro,
              const RwChar *name,
              RwInt32 celIndex, RwInt32 messageListIndex)
{
    RwInt32         strLabelIndex;
    Rt2dCel         *cel;

    RWAPIFUNCTION(RWSTRING("Rt2dCelCreate"));

    RWASSERT(maestro);

    if ((cel = (Rt2dCel *)RwFreeListAlloc(&Rt2dAnimGlobals.celFreeList,
                           rwID_2DANIM | rwMEMHINTDUR_EVENT)) == NULL)
    {
        RWRETURN((Rt2dCel *)NULL);
    }

    _rt2dCelInit(cel);

    if (name)
    {
        if (Rt2dMaestroFindStringLabel(maestro, rt2dANIMLABELTYPEFRAME,
                                        name, &strLabelIndex) == NULL)
        {
            RwFreeListFree(&Rt2dAnimGlobals.celFreeList, cel);

            RWRETURN((Rt2dCel *)NULL);
        }

        cel->strLabelIndex = strLabelIndex;
    }

    cel->celIndex = celIndex;
    cel->messageListIndex = messageListIndex;

    RWRETURN(cel);
}

/**
 * \ingroup rt2dcel
 * \ref Rt2dCelDestroy is called to destroy a cel. The cel's label in the maestro
 * is not destroyed however.
 *
 * Only cels created with \ref Rt2dCelCreate can be destroyed. Other cels cannot
 * be destroyed this way because they form part of an integral data structure.
 *
 * \param maestro              Pointer to the parent maestro.
 * \param cel                   Pointer to cel to be destroyed.
 *
 * \return Returns TRUE if successful, FALSE otherwise.
 *
 * \see Rt2dCelCreate
 */
RwBool
Rt2dCelDestroy( Rt2dMaestro * maestro __RWUNUSED__,
                Rt2dCel *cel )
{
    RWAPIFUNCTION(RWSTRING("Rt2dCelDestroy"));

    RWASSERT(cel);

    _rt2dCelDestruct(cel);

    RwFreeListFree(&Rt2dAnimGlobals.celFreeList, cel);

    RWRETURN(TRUE);
}

/**
 * \ingroup rt2dcel
 * \ref Rt2dCelAddButtonIndex is used to assign a button's index to the
 * cel. The index is the index in the maestro.
 *
 * Each cel can have a number of buttons associated with it. These buttons
 * are stored in the maestro and is referenced by its index. Each button
 * in a cel also have another index, local to the cel. This is the index
 * of the button within the cel.
 *
 * The button's index in the cel is returned if index is not NULL.
 *
 * \param cel                   Pointer to the cel.
 * \param buttonIndex           Index of the button in the maestro.
 * \param index                 Pointer to a RwInt32 to return the button's index in the cel.
 *
 * \return Returns a pointer to the cel if successful, NULL otherwise.
 *
 * \see Rt2dCelCreate
 * \see Rt2dMaestroAddButton
 */

Rt2dCel *
Rt2dCelAddButtonIndex(Rt2dCel * cel, RwInt32 buttonIndex, RwInt32 *index)
{
    RwInt32         *newIndex;

    RWAPIFUNCTION(RWSTRING("Rt2dCelAddButtonIndex"));

    RWASSERT(cel);
    RWASSERT(cel->buttonIndices);

    if (index)
    {
        *index = _rwSListGetNumEntries(cel->buttonIndices);
    }

    if ((newIndex = (RwInt32 *)_rwSListGetNewEntry(cel->buttonIndices,
            rwID_2DANIM | rwMEMHINTDUR_EVENT)) == NULL)
    {
       RWRETURN((Rt2dCel *)NULL);
    }

    *newIndex = buttonIndex;

    RWRETURN(cel);
}

/**
 * \ingroup rt2dcel
 * \ref Rt2dCelGetStringLabelIndex is used to return the label's index. This is
 * an index into the cel's parent maestro's label table. The label is used
 * for cel identification.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param cel                   Pointer to the cel.
 *
 * \return Returns the label's index if successful, -1 otherwise.
 *
 * \see Rt2dCelCreate
 * \see Rt2dCelSetStringLabelIndex
 * \see Rt2dMaestroAddStringLabel
 */

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
extern RwInt32
Rt2dCelGetStringLabelIndex(Rt2dCel *cel)
{
    RwInt32         index;

    RWAPIFUNCTION(RWSTRING("Rt2dCelGetStringLabelIndex"));

    RWASSERT(cel);

    index = _rt2dCelGetStringLabelIndexMacro(cel);

    RWRETURN(index);
}
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/**
 * \ingroup rt2dcel
 * \ref Rt2dCelSetStringLabelIndex is used to set a cel's label index.  This is
 * an index into the cel's parent maestro's label table. The label is used
 * for cel identification.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param cel                   Pointer to the cel.
 * \param index                 Label index.
 *
 * \see Rt2dCelCreate
 * \see Rt2dCelGetStringLabelIndex
 * \see Rt2dMaestroAddStringLabel
 * \see Rt2dMaestroGetStringLabelByIndex
 */

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
extern void
Rt2dCelSetStringLabelIndex(Rt2dCel *cel, RwInt32 index)
{
    RWAPIFUNCTION(RWSTRING("Rt2dCelSetStringLabelIndex"));

    RWASSERT(cel);

    _rt2dCelSetStringLabelIndexMacro(cel, index);

    RWRETURNVOID();
}
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/**
 * \ingroup rt2dcel
 * \ref Rt2dCelGetCelIndex is used to retrieve the cel's index.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param cel                   Pointer to the cel.
 *
 * \return Returns the cel's index if successful, -1 otherwise.
 *
 * \see Rt2dCelCreate
 * \see Rt2dCelSetCelIndex
 */

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
extern RwInt32
Rt2dCelGetCelIndex(Rt2dCel *cel)
{
    RwInt32         index;

    RWAPIFUNCTION(RWSTRING("Rt2dCelGetCelIndex"));

    RWASSERT(cel);

    index = _rt2dCelGetCelIndexMacro(cel);

    RWRETURN(index);
}
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/**
 * \ingroup rt2dcel
 * \ref Rt2dCelSetCelIndex is used to set the cel's index.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param cel   Pointer to the cel
 * \param index Cel index
 *
 * \see Rt2dCelCreate
 * \see Rt2dCelSetCelIndex
 */

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
extern void
Rt2dCelSetCelIndex(Rt2dCel *cel, RwInt32 index)
{
    RWAPIFUNCTION(RWSTRING("Rt2dCelSetCelIndex"));

    RWASSERT(cel);

    _rt2dCelSetCelIndexMacro(cel, index);

    RWRETURNVOID();
}
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/**
 * \ingroup rt2dcel
 * \ref Rt2dCelGetMessageListIndex is used to retrieve the cel's message list
 * index. This index should be the index returned by \ref Rt2dMaestroAddMessageList.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param cel                   Pointer to the cel.
 *
 * \return Returns the cel's message list index, -1 otherwise.
 *
 * \see Rt2dCelCreate
 * \see Rt2dCelSetMessageListIndex
 * \see Rt2dMaestroAddMessageList
 */

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
extern RwInt32
Rt2dCelGetMessageListIndex(Rt2dCel *cel)
{
    RwInt32             index;

    RWAPIFUNCTION(RWSTRING("Rt2dCelGetMessageListIndex"));

    RWASSERT(cel);

    index = _rt2dCelGetMessageListIndexMacro(cel);

    RWRETURN(index);
}
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/**
 * \ingroup rt2dcel
 * \ref Rt2dCelSetMessageListIndex is used to set the cel's message list
 * index. This index should be the index returned by \ref Rt2dMaestroAddMessageList.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param cel                   Pointer to the cel.
 * \param index                 Message list index.
 *
 * \see Rt2dCelCreate
 * \see Rt2dCelGetMessageListIndex
 * \see Rt2dMaestroAddMessageList
 */

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
extern void
Rt2dCelSetMessageListIndex(Rt2dCel *cel, RwInt32 index)
{
    RWAPIFUNCTION(RWSTRING("Rt2dCelSetMessageListIndex"));

    RWASSERT(cel);

    _rt2dCelSetMessageListIndexMacro(cel, index);

    RWRETURNVOID();
}
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/****************************************************************************
 *
 * Cel lists.
 *
 ****************************************************************************/

Rt2dCelList *
_rt2dCelListInit(Rt2dCelList *celList)
{
    RWFUNCTION(RWSTRING("_rt2dCelListInit"));

    RWASSERT(celList);

    if ((celList->cel = _rwSListCreate(sizeof(Rt2dCel),
        rwID_2DANIM | rwMEMHINTDUR_EVENT)) == NULL)
    {
        RWRETURN((Rt2dCelList *)NULL);
    }

    celList->flag = rt2dCELLISTFLAGLOCK;

    RWRETURN(celList);
}

Rt2dCelList *
_rt2dCelListDestruct(Rt2dCelList *celList)
{
    RwInt32         i, num;
    Rt2dCel         *cel;

    RWFUNCTION(RWSTRING("_rt2dCelListDestruct"));

    RWASSERT(celList);

    if (celList->cel)
    {
        num = _rwSListGetNumEntries(celList->cel);
        cel = (Rt2dCel *)_rwSListGetArray(celList->cel);

        if ((num > 0) && (cel != NULL))
        {
            for (i = 0; i < num; i++)
            {
                _rt2dCelDestruct(cel);

                cel++;
            }
        }

        _rwSListDestroy(celList->cel);

        celList->cel = (RwSList *)NULL;
    }

    celList->flag = 0;

    RWRETURN(celList);
}

RwInt32
_rt2dCelListStreamGetSize(Rt2dMaestro *maestro, Rt2dCelList *celList)
{
    RwInt32                     i, size, num;
    Rt2dCel                     *cel;

    RWFUNCTION(RWSTRING("_rt2dCelListStreamGetSize"));

    RWASSERT(maestro);
    RWASSERT(celList);

    size = 0;

    /* Cel list count */
    size += sizeof(RwInt32);

    if (celList->cel)
    {
        num = _rwSListGetNumEntries(celList->cel);

        if (num > 0)
        {
            cel = (Rt2dCel *)_rwSListGetArray(celList->cel);

            RWASSERT(cel);

            for (i = 0; i < num; i++)
            {
                size += _rt2dCelStreamGetSize(maestro, cel);

                cel++;
            }
        }
    }

    RWRETURN(size);
}

Rt2dCelList *
_rt2dCelListStreamRead(Rt2dMaestro *maestro, Rt2dCelList *celList, RwStream *stream)
{
    RwInt32                     i, num;
    Rt2dCel                     *cel;

    RWFUNCTION(RWSTRING("_rt2dCelListStreamRead"));

    RWASSERT(maestro);
    RWASSERT(stream);

    /* Create a new celList required. */
    if (celList == NULL)
    {
        if ((celList = Rt2dCelListCreate()) == NULL)
            RWRETURN((Rt2dCelList *)NULL);

        Rt2dCelListUnlock(celList);
    }

    RWASSERT(celList->flag & rt2dCELLISTFLAGLOCK);

    /* Read in the number of cels */
    if (RwStreamReadInt32(stream, &num, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dCelList *)NULL);
    }

    if (num > 0)
    {
        if ((cel = (Rt2dCel *)_rwSListGetNewEntries(celList->cel, num,
                rwID_2DANIM | rwMEMHINTDUR_EVENT)) == NULL)
        {
            RWRETURN((Rt2dCelList *)NULL);
        }

        for (i = 0; i < num; i++)
        {
            _rt2dCelInit(cel);

            if (_rt2dCelStreamRead(maestro, cel, stream) == NULL)
            {
                RWRETURN((Rt2dCelList *)NULL);
            }

            cel++;
        }
    }

    RWRETURN(celList);
}

Rt2dCelList *
_rt2dCelListStreamWrite(Rt2dMaestro *maestro, Rt2dCelList *celList, RwStream *stream)
{
    RwInt32                     i, num;
    Rt2dCel                     *cel;

    RWFUNCTION(RWSTRING("_rt2dCelListStreamWrite"));

    RWASSERT(maestro);
    RWASSERT(celList);
    RWASSERT(stream);

    /* Write out the number of cels in the list. */
    if (celList->cel)
    {
        num = _rwSListGetNumEntries(celList->cel);
        if (RwStreamWriteInt32(stream, &num, sizeof(RwInt32)) ==  NULL)
        {
            RWRETURN((Rt2dCelList *)NULL);
        }

        /* Write out the cels. */
        if (num > 0)
        {
            cel = (Rt2dCel *)_rwSListGetArray(celList->cel);

            RWASSERT(cel);

            for (i = 0; i < num; i++)
            {
                if (_rt2dCelStreamWrite(maestro, cel, stream) == NULL)
                {
                    RWRETURN((Rt2dCelList *)NULL);
                }

                cel++;
            }
        }
    }
    else
    {
        num = 0;
        if (RwStreamWriteInt32(stream, &num, sizeof(RwInt32)) ==  NULL)
        {
            RWRETURN((Rt2dCelList *)NULL);
        }
    }

    RWRETURN(celList);
}

Rt2dCel *
_rt2dCelListGetCelByIndex(Rt2dCelList *celList, RwInt32 index)
{
    Rt2dCel         *cel;

    RWFUNCTION(RWSTRING("_rt2dCelListGetCelByIndex"));

    RWASSERT(celList);

    if (celList->cel == NULL)
    {
        RWRETURN((Rt2dCel *)NULL);
    }

    cel = (Rt2dCel *)_rwSListGetArray(celList->cel);
    cel += index;

    RWRETURN(cel);
}

Rt2dCelList *
_rt2dCelListCopy(Rt2dCelList *dstCelList, Rt2dCelList *srcCelList)
{
    RwInt32                 i, numCel;
    Rt2dCel                 *srcCel, *dstCel;

    RWFUNCTION(RWSTRING("_rt2dCelListCopy"));

    RWASSERT(srcCelList);
    RWASSERT(dstCelList);

    numCel = _rwSListGetNumEntries(srcCelList->cel);

    if (numCel > 0)
    {
        /* Check we hav enough memory for the copy. */
        if (_rwSListGetNewEntries(dstCelList->cel, numCel,
                rwID_2DANIM | rwMEMHINTDUR_EVENT) == NULL)
        {
            RWRETURN((Rt2dCelList *)NULL);
        }

        srcCel = (Rt2dCel *)_rwSListGetArray(srcCelList->cel);
        dstCel = (Rt2dCel *)_rwSListGetArray(dstCelList->cel);

        RWASSERT(srcCel);
        RWASSERT(dstCel);

        for (i = 0; i < numCel; i++)
        {
            _rt2dCelInit(dstCel);

            if (_rt2dCelCopy(dstCel, srcCel) == NULL)
            {
                RWRETURN((Rt2dCelList *)NULL);
            }

            srcCel++;
            dstCel++;
        }
    }

    RWRETURN(dstCelList);
}

/****************************************************************************/

/**
 * \ingroup rt2dcel
 * \ref Rt2dCelListCreate creates a new empty cellist.
 *
 * \return Returns a pointer to a cel list if successful, NULL otherwise.
 *
 * \see Rt2dCelListDestroy
 */

Rt2dCelList *
Rt2dCelListCreate( void )
{
    Rt2dCelList             *celList;

    RWAPIFUNCTION(RWSTRING("Rt2dCelListCreate"));

    if ((celList = (Rt2dCelList *) RwFreeListAlloc(&Rt2dAnimGlobals.celListFreeList,
                                       rwID_2DANIM | rwMEMHINTDUR_EVENT)) == NULL)
    {
        RWRETURN( (Rt2dCelList *)NULL);
    }

    if (_rt2dCelListInit(celList) == NULL)
    {
        _rt2dCelListDestruct(celList);

        RwFreeListFree(&Rt2dAnimGlobals.celListFreeList, celList);

        celList = (Rt2dCelList *)NULL;
    }

    RWRETURN(celList);
}

/**
 * \ingroup rt2dcel
 * \ref Rt2dCelListDestroy destroys a cel list and all its contents. Any cels
 * in cel list are also destroyed.
 *
 * \param celList           Pointer to the cellist to be destroyed.
 *
 * \return Returns TRUE if successful, FALSE otherwise.
 *
 * \see Rt2dCelListCreate
 */

RwBool
Rt2dCelListDestroy(Rt2dCelList *celList)
{
    RWAPIFUNCTION(RWSTRING("Rt2dCelListDestroy"));

    RWASSERT(celList);

    _rt2dCelListDestruct(celList);

    RwFreeListFree(&Rt2dAnimGlobals.celListFreeList, celList);

    RWRETURN(TRUE);
}

/**
 * \ingroup rt2dcel
 * \ref Rt2dCelListLock is used to lock a cel list. A cel list must be locked
 * before any modifcations can be made and unlocked afterwards. Cel lists needs
 * to be in the unlocked form during playback.
 *
 * A cel list can only be locked once. Attempts to lock a locked cel list will do
 * nothing.
 *
 * \param celList               Pointer to cel list to be locked.
 *
 * \return Returns a pointer to the cel list if successful, NULL otherwise.
 *
 * \see Rt2dCelListUnlock
 */

Rt2dCelList *
Rt2dCelListLock(Rt2dCelList *celList)
{
    RWAPIFUNCTION(RWSTRING("Rt2dCelListLock"));

    RWASSERT(celList);

    /* Check if the cel list is locked, if not then lock it. */
    if (!(celList->flag & rt2dCELLISTFLAGLOCK))
    {
        celList->flag |= rt2dCELLISTFLAGLOCK;
    }

    RWRETURN(celList);
}

/**
 * \ingroup rt2dcel
 * \ref Rt2dCelListUnlock is used to unlock a locked cel list. A cel lists must
 * be unlocked during playback.
 *
 * Only locked list can be unlocked. Attempts to unlock an unlocked cel list will do
 * nothing.
 *
 * \param celList               Pointer to a cellist to be unlocked.
 *
 * \return Returns a pointer to the cel list if successful, NULL otherwise.
 *
 * \see Rt2dCelListLock
 */

Rt2dCelList *
Rt2dCelListUnlock(Rt2dCelList * celList)
{
    RWAPIFUNCTION(RWSTRING("Rt2dCelListUnlock"));

    RWASSERT(celList);

    /* Check if the cel list is locked, if so then unlock it. */
    if (celList->flag & rt2dCELLISTFLAGLOCK)
    {
        /* Set flag as unlocked */

        celList->flag &= ~rt2dCELLISTFLAGLOCK;
    }

    RWRETURN(celList);
}

/**
 * \ingroup rt2dcel
 * \ref Rt2dCelListCopy is used to duplicate an existing cel list. Cels within the
 * cel list are also duplicated, not referenced. Indexed data, such as buttons, are not
 * copied, but the indices are. Thus a copied cel will share the same indexed data as
 * the original cel.
 *
 * \param srcCelList               Pointer to a cel list to be copied.
 *
 * \return Returns a pointer to the new cel list if successful, NULL otherwise.
 *
 * \see Rt2dCelListLock
 */

Rt2dCelList *
Rt2dCelListCopy(Rt2dCelList *srcCelList)
{
    RwInt32                 numCel;
    Rt2dCelList             *dstCelList;

    RWAPIFUNCTION(RWSTRING("Rt2dCelListCopy"));

    RWASSERT(srcCelList);

    if ((dstCelList = Rt2dCelListCreate()) == NULL)
    {
        RWRETURN((Rt2dCelList *)NULL);
    }

    numCel = _rwSListGetNumEntries(srcCelList->cel);

    if (numCel > 0)
    {
        if (_rt2dCelListCopy(dstCelList, srcCelList) == NULL)
        {
            RWRETURN((Rt2dCelList *)NULL);
        }
    }

    RWRETURN(dstCelList);
}

/**
 * \ingroup rt2dcel
 * \ref Rt2dCelListAddCel is used add a cel to the cel list. A new cel in the cel list
 * is created and the given cel is copied into it. The given cel is not referenced by
 * the new cel or the cel list.
 *
 * The cel list must be locked before a new cel can be added.
 *
 * \param celList               Pointer to the parent cel list.
 * \param cel                   Pointer to the cel to be added.
 * \param index                 Pointer to a RwInt32 to return the cel's index.
 *
 * \return Returns a pointer to the cel list if successful, NULL otherwise.
 *
 * \see Rt2dCelCreate
 * \see Rt2dCelListCreate
 * \see Rt2dCelListDestroy
 * \see Rt2dCelListLock
 * \see Rt2dCelListUnlock
 */

Rt2dCelList *
Rt2dCelListAddCel(Rt2dCelList *celList, Rt2dCel *cel, RwInt32 *index)
{
    Rt2dCel             *newCel;

    RWAPIFUNCTION(RWSTRING("Rt2dCelListAddCel"));

    RWASSERT(celList);
    RWASSERT(cel);

    RWASSERT(celList->flag & rt2dCELLISTFLAGLOCK);

    if (index)
    {
        *index = _rwSListGetNumEntries(celList->cel);
    }

    if ((newCel = (Rt2dCel *)_rwSListGetNewEntry(celList->cel,
            rwID_2DANIM | rwMEMHINTDUR_EVENT)) == NULL)
    {
        RWRETURN((Rt2dCelList *)NULL);
    }

    _rt2dCelInit(newCel);

    if (_rt2dCelCopy(newCel, cel) == NULL)
    {
        RWRETURN((Rt2dCelList *)NULL);
    }

    RWRETURN(celList);
}

/**
 * \ingroup rt2dcel
 * \ref Rt2dCelListCelGetStringLabelIndex is used to retrieve the cel's label index
 *
 * \param celList               Pointer to the parent cel list.
 * \param celIndex              Pointer to the cel index to be added.
 * \param index                 Pointer to a RwInt32 to return the cel's index.
 *
 * \return Returns a pointer to the cel list if successful, NULL otherwise.
 *
 * \see Rt2dCelCreate
 * \see Rt2dCelListAddCel
 */

Rt2dCelList *
Rt2dCelListCelGetStringLabelIndex(Rt2dCelList *celList, RwInt32 celIndex, RwInt32 *index)
{
    Rt2dCel             *cel;

    RWAPIFUNCTION(RWSTRING("Rt2dCelListCelGetStringLabelIndex"));

    RWASSERT(celList);

    if ((cel = (Rt2dCel *)_rwSListGetEntry(celList->cel, celIndex)) == NULL)
    {
        RWRETURN((Rt2dCelList *)NULL);
    }

    *index = cel->strLabelIndex;

    RWRETURN(celList);
}

/**
 * \ingroup rt2dcel
 * \ref Rt2dCelListCelButtonGetDisplayVersion is used to retrieve the cel's button's label index.
 *
 * \param maestro              Pointer to parent maestro.
 * \param celList               Pointer to the parent cel list.
 * \param celIndex              Cel's index.
 * \param celButtonIndex        Cel's button index.
 * \param index                 Pointer to a RwInt32 to return the cel's button's label index.
 *
 * \return Returns a pointer to the cel list if successful, NULL otherwise.
 *
 * \see Rt2dCelAddButtonIndex
 * \see Rt2dCelListAddCel
 */

Rt2dCelList *
Rt2dCelListCelButtonGetDisplayVersion(Rt2dMaestro *maestro, Rt2dCelList * celList,
                                      RwInt32 celIndex, RwInt32 celButtonIndex, RwInt32 *index)
{
    RwInt32             *buttonIndex;
    Rt2dCel             *cel;
    Rt2dButton          *button;

    RWAPIFUNCTION(RWSTRING("Rt2dCelListCelButtonGetDisplayVersion"));

    RWASSERT(maestro);
    RWASSERT(celList);

    if ((cel = (Rt2dCel *)_rwSListGetEntry(celList->cel, celIndex)) == NULL)
    {
        RWRETURN((Rt2dCelList *)NULL);
    }

    if ((buttonIndex = (RwInt32 *)_rwSListGetEntry(cel->buttonIndices, celButtonIndex)) == NULL)
    {
        RWRETURN((Rt2dCelList *)NULL);
    }

    if ((button = (Rt2dButton *)_rwSListGetEntry(maestro->buttons, *buttonIndex)) == NULL)
    {
        RWRETURN((Rt2dCelList *)NULL);
    }

    *index = button->strLabelIndex;

    RWRETURN(celList);
}

/****************************************************************************/

/**
 * \ingroup rt2dcel
 * \ref Rt2dMaestroGetCelListByIndex is used to retrieve the maestro's
 * cel list given by index. This is the index returned by \ref Rt2dMaestroAddAnimations.
 *
 * \param maestro              Pointer to parent maestro.
 * \param index                 Cel list's index in the maestro.
 *
 * \return Returns a pointer to the cel list if successful, NULL otherwise.
 *
 * \see Rt2dMaestroAddAnimations
 * \see Rt2dMaestroGetAnimationsByIndex
 * \see Rt2dMaestroGetAnimPropsByIndex
 * \see Rt2dMaestroGetAnimSceneByIndex
 */

Rt2dCelList *
Rt2dMaestroGetCelListByIndex(Rt2dMaestro *maestro, RwInt32 index)
{
    Rt2dAnimNode        *animNode;

    RWAPIFUNCTION(RWSTRING("Rt2dMaestroGetCelListByIndex"));

    RWASSERT(maestro);

    animNode = _rt2dMaestroGetAnimNodeByIndex(maestro, index);

    RWRETURN(animNode->celList);
}


#if defined (__MWERKS__)
#if (defined(RWVERBOSE))
#pragma message (__FILE__ "/" _SKY_EXPAND(__LINE__) ": __MWERKS__ == " _SKY_EXPAND(__MWERKS__))
#endif /* (defined (__MWERKS__)) */
#if (__option (global_optimizer))
#pragma always_inline on
#endif /* (__option (global_optimizer)) */
#endif /*  defined (__MWERKS__) */

