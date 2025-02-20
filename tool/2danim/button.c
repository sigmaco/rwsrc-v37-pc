/*
 *
 * Functionality for 2D rendering
 *
 * Copyright (c) 2001 Criterion Software Ltd.
 */

/****************************************************************************
 *                                                                          *
 *  Module  :   button.c                                                    *
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

Rt2dButton *
_rt2dButtonInit(Rt2dButton *button)
{
    RwInt32         i;

    RWFUNCTION(RWSTRING("_rt2dButtonInit"));

    RWASSERT(button);

    button->strLabelIndex = -1;
    button->objectIndex = -1;
    button->stateFlag = 0;
    button->buttonState = 0;

    for (i = 0; i < RT2D_BUTTON_MAX_STATE_TRANSITIONS; i++)
        button->actionListForStateTransition[i] = -1;

    RWRETURN(button);
}

Rt2dButton *
_rt2dButtonDestruct(Rt2dButton *button)
{
    RwInt32         i;

    RWFUNCTION(RWSTRING("_rt2dButtonDestruct"));

    RWASSERT(button);

    button->strLabelIndex = -1;
    button->objectIndex = -1;
    button->stateFlag = 0;
    button->buttonState = 0;

    for (i = 0; i < RT2D_BUTTON_MAX_STATE_TRANSITIONS; i++)
        button->actionListForStateTransition[i] = -1;

    RWRETURN(button);
}

/****************************************************************************/

Rt2dButton *
_rt2dButtonCreate(void)
{
    Rt2dButton         *button;

    RWFUNCTION(RWSTRING("_rt2dButtonCreate"));

    if ((button = (Rt2dButton *) RwFreeListAlloc(&Rt2dAnimGlobals.buttonFreeList,
                                     rwID_2DANIM | rwMEMHINTDUR_EVENT)) == NULL)
        RWRETURN((Rt2dButton *)NULL);

    /* Initialise the members. */
    if (_rt2dButtonInit(button) == NULL)
    {
        _rt2dButtonDestruct(button);

        RwFreeListFree(&Rt2dAnimGlobals.buttonFreeList, button);

        button = (Rt2dButton *)NULL;
    }

    RWRETURN(button);
}

RwBool
_rt2dButtonDestroy(Rt2dButton *button)
{
    RWFUNCTION(RWSTRING("_rt2dButtonDestroy"));

    RWASSERT(button);

    /* Destruct the members. */
    _rt2dButtonDestruct(button);

    RwFreeListFree(&Rt2dAnimGlobals.buttonFreeList, button);

    RWRETURN(TRUE);
}

/****************************************************************************/

RwUInt32
_rt2dButtonStreamGetSize(Rt2dMaestro *maestro __RWUNUSED__,
                        Rt2dButton *button __RWUNUSEDRELEASE__,
                         RwInt32 stateCount __RWUNUSED__,
                         RwInt32 transCount)

{
    RwUInt32            size;

    RWFUNCTION(RWSTRING("_rt2dButtonStreamGetSize"));

    RWASSERT(button);

    size = 0;

    /* String label index. */
    size += sizeof(RwInt32);

    /* Object index. */
    size += sizeof(RwInt32);

    /* Flag. */
    size += sizeof(RwInt32);

    /* Write out the action list array. */
    size += transCount * sizeof(RwInt32);

    RWRETURN(size);
}

Rt2dButton *
_rt2dButtonStreamRead(Rt2dMaestro *maestro __RWUNUSED__,
                        Rt2dButton *button, RwStream *stream,
                      RwInt32 stateCount __RWUNUSED__ , RwInt32 transCount)
{
    RWFUNCTION(RWSTRING("_rt2dButtonStreamRead"));

    RWASSERT(stream);

    /* Check if we need to create a button. */
    if (button == NULL)
    {
        if ((button = _rt2dButtonCreate()) == NULL)
        {
            RWRETURN((Rt2dButton *)NULL);
        }
    }

    /* Read in the string label index. */
    if (RwStreamReadInt32(stream, &button->strLabelIndex, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dButton *)NULL);
    }

    /* Read the objectIndex. */
    if (RwStreamReadInt32(stream, &button->objectIndex, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dButton *)NULL);
    }

    /* Read the state flag. */
    if (RwStreamReadInt32(stream, (RwInt32 *)&button->stateFlag, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dButton *)NULL);
    }

    /* Read the state transition index. */
    if (RwStreamReadInt32(stream, button->actionListForStateTransition,
            (transCount * sizeof(RwInt32))) == NULL)
    {
        RWRETURN((Rt2dButton *)NULL);
    }

    RWRETURN(button);
}

Rt2dButton *
_rt2dButtonStreamWrite(Rt2dMaestro *maestro __RWUNUSED__,
                        Rt2dButton *button, RwStream *stream,
                        RwInt32 stateCount __RWUNUSED__, RwInt32 transCount)
{
    RWFUNCTION(RWSTRING("_rt2dButtonStreamWrite"));

    RWASSERT(button);
    RWASSERT(stream);

    /* Write out the strLabelIndex. */
    if (RwStreamWriteInt32(stream, &button->strLabelIndex, sizeof(RwInt32)) == NULL)
    {
            RWRETURN((Rt2dButton *)NULL);
    }

    /* Write out the objectIndex. */
    if (RwStreamWriteInt32(stream, &button->objectIndex, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dButton *)NULL);
    }

    /* Write out the state flag. */
    if (RwStreamWriteInt32(stream, (RwInt32 *)&button->stateFlag, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dButton *)NULL);
    }

    /* Write out the state transtition index. */
    if (RwStreamWriteInt32(stream, button->actionListForStateTransition,
            (transCount * sizeof(RwInt32))) == NULL)
    {
        RWRETURN((Rt2dButton *)NULL);
    }

    RWRETURN(button);
}

/****************************************************************************
 *
 * Maestro's button functions.
 *
 ****************************************************************************/

RwUInt32
_rt2dMaestroButtonsStreamGetSize(Rt2dMaestro *maestro)
{
    RwUInt32            size;
    RwInt32             num, numStates, numTransitions;
    Rt2dButton          *button;

    RWFUNCTION(RWSTRING("_rt2dMaestroButtonsStreamGetSize"));

    RWASSERT(maestro);

    size = 0;

    numStates = RT2D_BUTTON_MAX_STATES;
    numTransitions = RT2D_BUTTON_MAX_STATE_TRANSITIONS;

    /* An int for the number of buttons. */
    size += sizeof(RwInt32);

    /* An int for the number of states per button. */
    size += sizeof(RwInt32);

    /* An int for the number of state transitions per button. */
    size += sizeof(RwInt32);

    if (maestro->buttons)
    {
        num = _rwSListGetNumEntries(maestro->buttons);
        button = (Rt2dButton *)_rwSListGetArray(maestro->buttons);

        if (num > 0)
        {
            /* We are writing the whole struct array. */
            size += num * _rt2dButtonStreamGetSize(maestro, button,
                        numStates, numTransitions);
        }
    }

    RWRETURN(size);
}

Rt2dMaestro *
_rt2dMaestroButtonsStreamRead(Rt2dMaestro *maestro, RwStream *stream)
{
    RwInt32                 numButtons, numStates, numTransitions;
    Rt2dButton              *button, *beginButton, *endButton;

    RWFUNCTION(RWSTRING("_rt2dMaestroButtonsStreamRead"));

    RWASSERT(stream);

    if (maestro == NULL)
    {
        maestro = Rt2dMaestroCreate();
    }

    if (maestro == NULL)
    {
        RWRETURN((Rt2dMaestro *)NULL);
    }

    /* Read in the num of buttons. */
    if (RwStreamReadInt32(stream, &numButtons, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dMaestro *)NULL);
    }

    /* Read in the num of button's states. */
    if (RwStreamReadInt32(stream, &numStates, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dMaestro *)NULL);
    }

    /* Read in the num of button's state transtitions. */
    if (RwStreamReadInt32(stream, &numTransitions, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dMaestro *)NULL);
    }

    if (numButtons > 0)
    {
        /* Create an SList for buttons. */
        if (maestro->buttons == NULL)
            _rt2dMaestroCreateButtons(maestro);

        /* Read in the buttons. */
        if (maestro->buttons)
        {
            if ((beginButton = (Rt2dButton *)
                   _rwSListGetNewEntries(maestro->buttons, numButtons,
                        rwID_2DMAESTRO | rwMEMHINTDUR_EVENT)) == NULL)
            {
                RWRETURN((Rt2dMaestro *)NULL);
            }

            endButton = beginButton + numButtons;

            for (button = beginButton; button != endButton; ++button)
            {
                _rt2dButtonInit(button);

                if (_rt2dButtonStreamRead(maestro, button, stream,
                                          numStates, numTransitions) == NULL)
                {
                    RWRETURN((Rt2dMaestro *)NULL);
                }
            }
        }
        else
        {
            /* Failed to create the SList for buttons. */
            RWRETURN((Rt2dMaestro *)NULL);
        }
    }

    RWRETURN(maestro);
}

Rt2dMaestro *
_rt2dMaestroButtonsStreamWrite(Rt2dMaestro *maestro, RwStream *stream)
{
    Rt2dButton              *button, *beginButton, *endButton;
    RwInt32                 numButtons, numStates, numTransitions;

    RWFUNCTION(RWSTRING("_rt2dMaestroButtonsStreamWrite"));

    RWASSERT(maestro);
    RWASSERT(stream);

    numStates = RT2D_BUTTON_MAX_STATES;
    numTransitions = RT2D_BUTTON_MAX_STATE_TRANSITIONS;

    if (maestro->buttons)
    {
        /* Write out the number of buttons. */
        numButtons = _rwSListGetNumEntries(maestro->buttons);
        if (RwStreamWriteInt32(stream, &numButtons, sizeof(RwInt32)) == NULL)
        {
            RWRETURN((Rt2dMaestro *)NULL);
        }

        /* Write out the number of button's states. */
        if (RwStreamWriteInt32(stream, &numStates, sizeof(RwInt32)) == NULL)
        {
            RWRETURN((Rt2dMaestro *)NULL);
        }

        /* Write out the number of button's state transitions. */
        if (RwStreamWriteInt32(stream, &numTransitions, sizeof(RwInt32)) == NULL)
        {
            RWRETURN((Rt2dMaestro *)NULL);
        }

        if (numButtons > 0)
        {
            beginButton = (Rt2dButton *)_rwSListGetArray(maestro->buttons);
            endButton = beginButton + numButtons;

            RWASSERT(beginButton);

            for (button = beginButton; button != endButton; ++button)
            {
                if (_rt2dButtonStreamWrite(maestro, button, stream,
                                           numStates, numTransitions) == NULL)
                {
                    RWRETURN((Rt2dMaestro *)NULL);
                }
            }
        }
    }
    else
    {
        numButtons = 0;

        /* Write out the number of buttons. */
        if (RwStreamWriteInt32(stream, &numButtons, sizeof(RwInt32)) == NULL)
        {
            RWRETURN((Rt2dMaestro *)NULL);
        }

        /* Write out the number of button's states. */
        if (RwStreamWriteInt32(stream, &numButtons, sizeof(RwInt32)) == NULL)
        {
             RWRETURN((Rt2dMaestro *)NULL);
        }

        /* Write out the number of button's state transitions. */
        if (RwStreamWriteInt32(stream, &numButtons, sizeof(RwInt32)) == NULL)
        {
            RWRETURN((Rt2dMaestro *)NULL);
        }
    }

    RWRETURN(maestro);
}

Rt2dMaestro *
_rt2dMaestroCreateButtons(Rt2dMaestro *maestro)
{
    Rt2dMaestro            *result;

    RWFUNCTION(RWSTRING("_rt2dMaestroCreateButtons"));

    RWASSERT(maestro);
    RWASSERT((maestro->buttons == NULL));

    maestro->buttons = _rwSListCreate(sizeof(Rt2dButton),
        rwID_2DMAESTRO | rwMEMHINTDUR_EVENT);

    result =  (maestro->buttons == NULL)
               ? ((Rt2dMaestro *)NULL) : maestro;

    RWRETURN(result);

}

Rt2dMaestro *
_rt2dMaestroDestroyButtons(Rt2dMaestro *maestro)
{
    RWFUNCTION(RWSTRING("_rt2dMaestroDestroyButtons"));

    RWASSERT(maestro);

    if (maestro->buttons)
    {
        _rwSListDestroy(maestro->buttons);

        maestro->buttons = (RwSList *)NULL;
    }

    RWRETURN(maestro);
}

Rt2dButton *
_rt2dMaestroGetButtonByIndex(Rt2dMaestro *maestro, RwInt32 index)
{
    Rt2dButton              *button;

    RWFUNCTION(RWSTRING("_rt2dMaestroGetButtonByIndex"));

    RWASSERT(maestro);
    RWASSERT(maestro->buttons);

    button = (Rt2dButton*)_rwSListGetEntry(maestro->buttons, index);

    RWRETURN(button);
}


/****************************************************************************/

/**
 * \ingroup rt2dbutton
 * \ref Rt2dMaestroAddButton is used to add a single button to the maestro. The
 * maestro maintains a central repository for all buttons that exist in the scene.
 * An index reference for the button is returned. This index can be added to any
 * cels that requires the button, allowing the button to be shared between cels.
 *
 * A button requires a label for indentification and an object. Both are given as
 * indices to the maestro's label table and scene respectively. The stateFlag lists
 * the available states and transition states. For every possible transition there
 * must be a corresponding index in the message index list. The entries in the message
 * list index must match the order of the transition states in the stateFlag.
 *
 * The button's index in the maestro is returned if index is not NULL.
 *
 * \param maestro              Pointer to parent maestro.
 * \param strLabelIdx           Index to the label in the maestro.
 * \param objectIdx             Index to an object in the maestro's scene.
 * \param stateFlag             Button's state.
 * \param actionListIdx         List of indices to messages in the maestro.
 * \param index                 Pointer to a RwInt32 to return the button's index in the maestro.
 *
 * \return Returns a pointer to the maestro if successful, NULL otherwise.
 *
 * \see Rt2dMaestroAddStringLabel
 * \see Rt2dCelListLock
 * \see Rt2dCelAddButtonIndex
 */

Rt2dMaestro *
Rt2dMaestroAddButton(Rt2dMaestro *maestro, RwInt32 strLabelIdx, RwInt32 objectIdx,
                      RwUInt32 stateFlag, RwInt32 *actionListIdx,
                      RwInt32 *index)
{
    Rt2dButton          *newButton;

    RWAPIFUNCTION(RWSTRING("Rt2dMaestroAddButton"));

    RWASSERT(maestro);

    RWASSERT(maestro->buttons);

    if (index)
    {
        *index = _rwSListGetNumEntries(maestro->buttons);
    }

    if ((newButton =
         (Rt2dButton*)_rwSListGetNewEntry(maestro->buttons,
            rwID_2DMAESTRO | rwMEMHINTDUR_EVENT)) == NULL)
    {
        RWRETURN((Rt2dMaestro*)NULL);
    }

    if (_rt2dButtonInit(newButton) == NULL)
    {
        RWRETURN((Rt2dMaestro*)NULL);
    }

    newButton->strLabelIndex = strLabelIdx;
    newButton->objectIndex = objectIdx;
    newButton->stateFlag = stateFlag;

    memcpy(newButton->actionListForStateTransition, actionListIdx,
            (RT2D_BUTTON_MAX_STATE_TRANSITIONS * sizeof(RwUInt32)));

    RWRETURN(maestro);
}

/****************************************************************************/

Rt2dStateTransData *
_rt2dStateTransTableInit(Rt2dStateTransData stateTransTable[2][2][2][2])
{
    RWFUNCTION(RWSTRING("_rt2dStateTransTableInit"));

    RWASSERT(stateTransTable);

    /* First column is if the the PREV mouse pos was in the button. 0 <in> : 1 <out> */
    /* Second column is the PREV mouse button 0 <up> : 1 <down> */
    /* Third column is if the CURR mouse pos is in the button, 0 <in> : 1 <out> */
    /* Fourth column is the CURR MOUSE button 0 <up> : 1 <down> */

    /* Not all possible states combination are supported. */

    /* Prev pos out, prev button up, curr out, curr button up */
    stateTransTable[0][0][0][0].transState = 0;
    stateTransTable[0][0][0][0].buttonUpImg = TRUE;
    stateTransTable[0][0][0][0].buttonDownImg = FALSE;
    stateTransTable[0][0][0][0].buttonOverImg = FALSE;

    /* Prev pos out, prev button up, curr out, curr button down */
    stateTransTable[0][0][0][1].transState = 0;
    stateTransTable[0][0][0][1].buttonUpImg = TRUE;
    stateTransTable[0][0][0][1].buttonDownImg = FALSE;
    stateTransTable[0][0][0][1].buttonOverImg = FALSE;

    /* Prev pos out, prev button up, curr in, curr button up */
    stateTransTable[0][0][1][0].transState = rt2dANIMBUTTONSTATEIDLETOOVERUP;
    stateTransTable[0][0][1][0].buttonUpImg = FALSE;
    stateTransTable[0][0][1][0].buttonDownImg = FALSE;
    stateTransTable[0][0][1][0].buttonOverImg = TRUE;

    /* Prev pos out, prev button up, curr in, curr button down */
    stateTransTable[0][0][1][1].transState = 0;
    stateTransTable[0][0][1][1].buttonUpImg = FALSE;
    stateTransTable[0][0][1][1].buttonDownImg = TRUE;
    stateTransTable[0][0][1][1].buttonOverImg = FALSE;

    /*
     *
     *
     *
     */

    /* Prev pos out, prev button down, curr out, curr button up */
    stateTransTable[0][1][0][0].transState = rt2dANIMBUTTONSTATEOUTDOWNTOIDLE;
    stateTransTable[0][1][0][0].buttonUpImg = TRUE;
    stateTransTable[0][1][0][0].buttonDownImg = FALSE;
    stateTransTable[0][1][0][0].buttonOverImg = FALSE;

    /* Prev pos out, prev button down, curr out, curr button down */
    stateTransTable[0][1][0][1].transState = 0;
    stateTransTable[0][1][0][1].buttonUpImg = TRUE;
    stateTransTable[0][1][0][1].buttonDownImg = FALSE;
    stateTransTable[0][1][0][1].buttonOverImg = FALSE;

    /* Prev pos out, prev button down, curr in, curr button up */
    stateTransTable[0][1][1][0].transState = 0;
    stateTransTable[0][1][1][0].buttonUpImg = FALSE;
    stateTransTable[0][1][1][0].buttonDownImg = FALSE;
    stateTransTable[0][1][1][0].buttonOverImg = TRUE;

    /* Prev pos out, prev button down, curr in, curr button down */
    stateTransTable[0][1][1][1].transState = (rt2dANIMBUTTONSTATEOUTDOWNTOOVERDOWN |
                                              rt2dANIMBUTTONSTATEIDLETOOVERDOWN);
    stateTransTable[0][1][1][1].buttonUpImg = FALSE;
    stateTransTable[0][1][1][1].buttonDownImg = TRUE;
    stateTransTable[0][1][1][1].buttonOverImg = FALSE;

    /*
     *
     *
     *
     */

    /* Prev pos out, prev button up, curr out, curr button up */
    stateTransTable[1][0][0][0].transState = rt2dANIMBUTTONSTATEOVERUPTOIDLE;
    stateTransTable[1][0][0][0].buttonUpImg = TRUE;
    stateTransTable[1][0][0][0].buttonDownImg = FALSE;
    stateTransTable[1][0][0][0].buttonOverImg = FALSE;

    /* Prev pos out, prev button up, curr out, curr button down */
    stateTransTable[1][0][0][1].transState = 0;
    stateTransTable[1][0][0][1].buttonUpImg = FALSE;
    stateTransTable[1][0][0][1].buttonDownImg = FALSE;
    stateTransTable[1][0][0][1].buttonOverImg = TRUE;

    /* Prev pos out, prev button up, curr in, curr button up */
    stateTransTable[1][0][1][0].transState = 0;
    stateTransTable[1][0][1][0].buttonUpImg = FALSE;
    stateTransTable[1][0][1][0].buttonDownImg = FALSE;
    stateTransTable[1][0][1][0].buttonOverImg = TRUE;

    /* Prev pos out, prev button up, curr in, curr button down */
    stateTransTable[1][0][1][1].transState = rt2dANIMBUTTONSTATEOVERUPTOOVERDOWN;
    stateTransTable[1][0][1][1].buttonUpImg = FALSE;
    stateTransTable[1][0][1][1].buttonDownImg = TRUE;
    stateTransTable[1][0][1][1].buttonOverImg = FALSE;

    /*
     *
     *
     *
     */

    /* Prev pos in, prev button up, curr out, curr button up */
    stateTransTable[1][1][0][0].transState = 0;
    stateTransTable[1][1][0][0].buttonUpImg = TRUE;
    stateTransTable[1][1][0][0].buttonDownImg = FALSE;
    stateTransTable[1][1][0][0].buttonOverImg = FALSE;

    /* Prev pos in, prev button up, curr out, curr button down */
    stateTransTable[1][1][0][1].transState = (rt2dANIMBUTTONSTATEOVERDOWNTOOUTDOWN |
                                              rt2dANIMBUTTONSTATEOVERDOWNTOIDLE);
    stateTransTable[1][1][0][1].buttonUpImg = TRUE;
    stateTransTable[1][1][0][1].buttonDownImg = FALSE;
    stateTransTable[1][1][0][1].buttonOverImg = FALSE;

    /* Prev pos in, prev button up, curr in, curr button up */
    stateTransTable[1][1][1][0].transState = rt2dANIMBUTTONSTATEOVERDOWNTOOVERUP;
    stateTransTable[1][1][1][0].buttonUpImg = FALSE;
    stateTransTable[1][1][1][0].buttonDownImg = FALSE;
    stateTransTable[1][1][1][0].buttonOverImg = TRUE;

    /* Prev pos in, prev button up, curr in, curr button down */
    stateTransTable[1][1][1][1].transState = 0;
    stateTransTable[1][1][1][1].buttonUpImg = FALSE;
    stateTransTable[1][1][1][1].buttonDownImg = TRUE;
    stateTransTable[1][1][1][1].buttonOverImg = FALSE;

    RWRETURN(&stateTransTable[2][2][2][2]);
}

Rt2dButton *
_rt2dButtonReset(Rt2dButton *button, Rt2dObject *scene)
{
    RWFUNCTION(RWSTRING("_rt2dButtonReset"));

    RWASSERT(button);
    RWASSERT(scene);

    if (button->stateFlag & rt2dANIMBUTTONSTATEPICKREGION)
    {
        RwInt32 k;
        Rt2dObject *buttonScene =
            Rt2dSceneGetChildByIndex(scene, button->objectIndex);

        /* Change the up image if exist. */
        k = 1;
        if (button->stateFlag & rt2dANIMBUTTONSTATEUP)
        {
            Rt2dObjectSetVisible(
                Rt2dSceneGetChildByIndex(buttonScene, k),
                TRUE);
            k++;
        }

        /* Change the down image if exist. */
        if (button->stateFlag & rt2dANIMBUTTONSTATEDOWN)
        {
            Rt2dObjectSetVisible(
                Rt2dSceneGetChildByIndex(buttonScene, k),
                FALSE);
            k++;
        }

        /* Change the over image if exist. */
        if (button->stateFlag & rt2dANIMBUTTONSTATEOVER)
        {
            Rt2dObjectSetVisible(
                Rt2dSceneGetChildByIndex(buttonScene, k),
                FALSE);
        }

    }

    RWRETURN(button);
}

#if defined (__MWERKS__)
#if (defined(RWVERBOSE))
#pragma message (__FILE__ "/" _SKY_EXPAND(__LINE__) ": __MWERKS__ == " _SKY_EXPAND(__MWERKS__))
#endif /* (defined (__MWERKS__)) */
#if (__option (global_optimizer))
#pragma always_inline on
#endif /* (__option (global_optimizer)) */
#endif /*  defined (__MWERKS__) */
