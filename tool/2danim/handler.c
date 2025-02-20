/*
 *
 * Functionality for 2D rendering
 *
 * Copyright (c) 2001 Criterion Software Ltd.
 */

/****************************************************************************
 *                                                                          *
 *  Module  :   handler.c                                                   *
 *                                                                          *
 *  Purpose :   simple 2d animation maestro functionality                  *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
 */


#include <rwcore.h>
#include <rpdbgerr.h>
#include <rpcriter.h>

#include "rt2danim.h"
#include "anim.h"
#include "props.h"
#include "maestro.h"
#include "bucket.h"
#include "message.h"
#include "button.h"
#include "handler.h"

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

static Rt2dMessage *
_rt2dHandlerDefaultStop(Rt2dMaestro *maestro, Rt2dMessage *message)
{
    Rt2dAnimNode            *animNode;

    RWFUNCTION(RWSTRING("_rt2dHandlerDefaultStop"));

    RWASSERT(maestro);
    RWASSERT(message);

    animNode = _rt2dMaestroGetAnimNodeByIndex(maestro, message->index);
    animNode->flag |= RT2D_ANIMNODE_FLAG_STOP;

    RWRETURN(message);
}

static Rt2dMessage *
_rt2dHandlerDefaultPlay(Rt2dMaestro *maestro, Rt2dMessage *message)
{
    Rt2dAnimNode            *animNode;

    RWFUNCTION(RWSTRING("_rt2dHandlerDefaultPlay"));

    RWASSERT(maestro);
    RWASSERT(message);

    animNode = _rt2dMaestroGetAnimNodeByIndex(maestro, message->index);

    animNode->flag &= (~RT2D_ANIMNODE_FLAG_STOP);

    RWRETURN(message);
}


static Rt2dMessage *
_rt2dHandlerDefaultNextFrame(Rt2dMaestro *maestro, Rt2dMessage *message)
{
    RwInt32                 frameIndex;
    Rt2dAnimNode            *animNode;

    RWFUNCTION(RWSTRING("_rt2dHandlerDefaultNextFrame"));
    RWASSERT(maestro);
    RWASSERT(message);
    RWASSERT(message->index>=0);

    animNode = _rt2dMaestroGetAnimNodeByIndex(maestro, message->index);

    frameIndex = Rt2dAnimGetNextFrameIndex(&animNode->anim);

    if (frameIndex<Rt2dAnimGetNumberOfKeyFrames(&animNode->anim))
    {
        Rt2dAnimGotoKeyFrameListByIndex(&animNode->anim, &animNode->props, frameIndex);
        animNode->flag |= RT2D_ANIMNODE_FLAG_DO_CEL_ACTIONS;
    }

    RWRETURN(message);
}

static Rt2dMessage *
_rt2dHandlerDefaultPrevFrame(Rt2dMaestro *maestro, Rt2dMessage *message)
{
    RwInt32                 frameIndex;
    Rt2dAnimNode            *animNode;

    RWFUNCTION(RWSTRING("_rt2dHandlerDefaultPrevFrame"));

    RWASSERT(maestro);
    RWASSERT(message);

    animNode = _rt2dMaestroGetAnimNodeByIndex(maestro, message->index);

    frameIndex = Rt2dAnimGetPrevFrameIndex(&animNode->anim) - 1;

    if (frameIndex>=0)
    {
        Rt2dAnimGotoKeyFrameListByIndex(&animNode->anim, &animNode->props, frameIndex);
        animNode->flag |= RT2D_ANIMNODE_FLAG_DO_CEL_ACTIONS;
    }

    RWRETURN(message);
}

static Rt2dMessage *
_rt2dHandlerDefaultGotoFrame(Rt2dMaestro *maestro, Rt2dMessage *message)
{
    RwInt32                 frameIndex;
    Rt2dAnimNode            *animNode;

    RWFUNCTION(RWSTRING("_rt2dHandlerDefaultGotoFrame"));

    RWASSERT(maestro);
    RWASSERT(message);
    RWASSERT(message->index>=0);

    animNode = _rt2dMaestroGetAnimNodeByIndex(maestro, message->index);

    frameIndex = message->intParam1;

    Rt2dAnimGotoKeyFrameListByIndex(&animNode->anim, &animNode->props, frameIndex);
    animNode->flag |= RT2D_ANIMNODE_FLAG_DO_CEL_ACTIONS;

    RWRETURN(message);
}


static Rt2dMessage *
_rt2dHandlerDefaultGotoLabel(Rt2dMaestro *maestro, Rt2dMessage *message)
{
    RwInt32                 frameIndex;
    Rt2dAnimNode            *animNode;
    Rt2dStringLabel         *strLabel;

    RWFUNCTION(RWSTRING("_rt2dHandlerDefaultGotoLabel"));

    RWASSERT(maestro);
    RWASSERT(message);

    animNode = _rt2dMaestroGetAnimNodeByIndex(maestro, message->index);

    strLabel = Rt2dMaestroGetStringLabelByIndex(maestro, message->intParam1);

    frameIndex = (RwInt32) strLabel->internalData;

    Rt2dAnimGotoKeyFrameListByIndex(&animNode->anim, &animNode->props, frameIndex);
    animNode->flag |= RT2D_ANIMNODE_FLAG_DO_CEL_ACTIONS;

    RWRETURN(message);
}

static Rt2dMessage *
_rt2dHandlerDefaultGetURL(Rt2dMaestro *maestro __RWUNUSED__,
                            Rt2dMessage *message)
{
    RWFUNCTION(RWSTRING("_rt2dHandlerDefaultGetURL"));

    RWRETURN(message);
}

static Rt2dMessage *
_rt2dHandlerDefaultDoAction(Rt2dMaestro *maestro __RWUNUSED__,
                            Rt2dMessage *message)
{
    RWFUNCTION(RWSTRING("_rt2dHandlerDefaultDoAction"));

    RWASSERT(maestro);
    RWASSERT(message);

    _rt2dMaestroMessageListPostByIndex(maestro, message->intParam1);

    RWRETURN(message);
}

static Rt2dMessage *
_rt2dHandlerDefaultForeign(Rt2dMaestro *maestro __RWUNUSED__, Rt2dMessage *message)
{
    RWFUNCTION(RWSTRING("_rt2dHandlerDefaultForeign"));

    RWRETURN(message);
}

void
_rt2dApplyMouseState(Rt2dMaestro *maestro, RwV2d *point)
{
    RwInt32                 keyframeIndex, i, j, k, numButton, numAnim, *buttonIndex, oldAnimIndex;
    Rt2dAnimNode            *animNode;
    Rt2dButton              *button;
    Rt2dCel                 *cel;

    Rt2dObject              *animScene;
    Rt2dObject              *buttonScene;
    Rt2dObject              *pickRegion, *pickRegionScene;

    RwInt32                 mouseOver, prevMouseOver, stateFlag, transFlag;
    Rt2dStateTransData      *stateTrans;

    RWFUNCTION(RWSTRING("_rt2dApplyMouseState"));

    /* Check for button actions. */

    numAnim = _rwSListGetNumEntries(maestro->animations);
    animNode = (Rt2dAnimNode *)_rwSListGetArray(maestro->animations);

    /* Save old anim index. */
    oldAnimIndex = maestro->currAnimIndex;

    for (i = 0; i < numAnim; i++)
    {
        maestro->currAnimIndex = i;

        animScene =
            Rt2dMaestroGetAnimSceneByIndex(maestro, i);

        keyframeIndex = Rt2dAnimGetPrevFrameIndex(&animNode->anim);

        if (keyframeIndex >= 0)
        {
            cel = _rt2dCelListGetCelByIndex(animNode->celList, keyframeIndex);

            numButton = _rwSListGetNumEntries(cel->buttonIndices);
            buttonIndex = (RwInt32 *)_rwSListGetArray(cel->buttonIndices);

            for (j = 0; j < numButton; j++)
            {
                button = _rt2dMaestroGetButtonByIndex(maestro, *buttonIndex);

                if (button->stateFlag & rt2dANIMBUTTONSTATEPICKREGION)
                {
                    buttonScene =
                        Rt2dSceneGetChildByIndex(animScene, button->objectIndex);

                    /* Assume object 0 is the pick region. */
                    pickRegionScene =
                        Rt2dSceneGetChildByIndex(buttonScene, 0);

                    mouseOver = FALSE;

                    for(k = 0; k < Rt2dSceneGetChildCount(pickRegionScene); k++)
                    {
                        pickRegion = Rt2dSceneGetChildByIndex(pickRegionScene, k);
                        mouseOver = mouseOver || Rt2dPickRegionIsPointIn(pickRegion, point);
                    }

                    prevMouseOver = (button->buttonState & RT2D_BUTTON_STATE_FLAG_OVER) ? 1 : 0;

                    /* Select the transition table. */
                    stateTrans = &maestro->stateTransTable
                                    [prevMouseOver][maestro->prevMouse.button]
                                    [mouseOver][maestro->prevMouse.button];

                    transFlag = stateTrans->transState;

                    /* Change the up image if exist. */
                    k = 1;
                    if (button->stateFlag & rt2dANIMBUTTONSTATEUP)
                    {
                        Rt2dObjectSetVisible(
                            Rt2dSceneGetChildByIndex(buttonScene, k),
                            stateTrans->buttonUpImg);
                        k++;
                    }

                    /* Change the down image if exist. */
                    if (button->stateFlag & rt2dANIMBUTTONSTATEDOWN)
                    {
                        Rt2dObjectSetVisible(
                            Rt2dSceneGetChildByIndex(buttonScene, k),
                            stateTrans->buttonDownImg);
                        k++;
                    }

                    /* Change the over image if exist. */
                    if (button->stateFlag & rt2dANIMBUTTONSTATEOVER)
                    {
                        Rt2dObjectSetVisible(
                            Rt2dSceneGetChildByIndex(buttonScene, k),
                            stateTrans->buttonOverImg);
                    }

                    /* Post any new message for this state transition. */
                    if (button->stateFlag & transFlag)
                    {
                        /* Remove the first four bits. These are trans bit. */
                        stateFlag = (button->stateFlag >> 4);
                        transFlag = transFlag >> 4;

                        /* Find the correct index for this transition in the action list. */
                        k = -1;
                        while (transFlag)
                        {
                            if (stateFlag & 0x01)
                                k++;

                            transFlag >>= 1;
                            stateFlag >>= 1;
                        }

                        RWASSERT(k >= 0);

                        /* Post the messages if it exist. */
                        if (button->actionListForStateTransition[k] >= 0)
                            _rt2dMaestroMessageListPostByIndex(maestro,
                                    button->actionListForStateTransition[k]);
                    }

                    /* Update the mouse button */
                    if (mouseOver)
                        button->buttonState |= RT2D_BUTTON_STATE_FLAG_OVER;
                    else
                        button->buttonState &= (~RT2D_BUTTON_STATE_FLAG_OVER);
                }

                buttonIndex++;
            }
        }
        else
        {
            /* Should we assert ? */
        }

        animNode++;
    }

    /* Restore anim index. */
    maestro->currAnimIndex = oldAnimIndex;

    RWRETURNVOID();
}

static Rt2dMessage *
_rt2dHandlerDefaultMouseMoveTo(Rt2dMaestro *maestro, Rt2dMessage *message)
{
    RWFUNCTION(RWSTRING("_rt2dHandlerDefaultMouseMoveTo"));

    RWASSERT(maestro);
    RWASSERT(message);

    /* Copy the old mouse state over. */

    maestro->prevMouse.position.x = ((RwReal) (message->intParam1) * maestro->invWinW) * (maestro->winAspectRatio);
    maestro->prevMouse.position.y = 1.0f - ((RwReal)(message->intParam2) * maestro->invWinH);

    _rt2dApplyMouseState(maestro, &maestro->prevMouse.position);

    RWRETURN(message);
}

static Rt2dMessage *
_rt2dHandlerDefaultMouseButtonState(Rt2dMaestro *maestro, Rt2dMessage *message)
{
    RwInt32                 keyframeIndex, i, j, k, numAnim, numButton, *buttonIndex, oldAnimIndex;
    Rt2dAnimNode            *animNode;
    Rt2dButton              *button;
    Rt2dCel                 *cel;

    Rt2dObject              *animScene;
    Rt2dObject              *buttonScene;

    RwInt32                 mouseButton, prevMouseButton, mouseOver, stateFlag, transFlag;
    Rt2dStateTransData      *stateTrans;

    RWFUNCTION(RWSTRING("_rt2dHandlerDefaultMouseButtonState"));

    RWASSERT(maestro);
    RWASSERT(message);

    /* Check for button actions. */
    mouseButton = message->intParam1;
    prevMouseButton = maestro->prevMouse.button;

    /* Check for button actions. */

    numAnim = _rwSListGetNumEntries(maestro->animations);
    animNode = (Rt2dAnimNode *)_rwSListGetArray(maestro->animations);

    /* Save the current anim index. */
    oldAnimIndex = maestro->currAnimIndex;

    for (i = 0; i < numAnim; i++)
    {
        maestro->currAnimIndex = i;

        animScene =
            Rt2dMaestroGetAnimSceneByIndex(maestro, i);

        keyframeIndex = Rt2dAnimGetPrevFrameIndex(&animNode->anim);

        cel = _rt2dCelListGetCelByIndex(animNode->celList, keyframeIndex);

        numButton = _rwSListGetNumEntries(cel->buttonIndices);
        buttonIndex = (RwInt32 *)_rwSListGetArray(cel->buttonIndices);

        for (j = 0; j < numButton; j++)
        {
            button = _rt2dMaestroGetButtonByIndex(maestro, *buttonIndex);

            if (button->stateFlag & rt2dANIMBUTTONSTATEPICKREGION)
            {
                buttonScene =
                    Rt2dSceneGetChildByIndex(animScene, button->objectIndex);

                mouseOver = (button->buttonState & RT2D_BUTTON_STATE_FLAG_OVER) ?
                        1 : 0;

                /* Select the transition table. */
                stateTrans = &maestro->stateTransTable
                                [mouseOver][prevMouseButton]
                                [mouseOver][mouseButton];

                transFlag = stateTrans->transState;

                /* Change the up image if exist. */
                k = 1;
                if (button->stateFlag & rt2dANIMBUTTONSTATEUP)
                {
                    Rt2dObjectSetVisible(
                        Rt2dSceneGetChildByIndex(buttonScene, k),
                        stateTrans->buttonUpImg);
                    k++;
                }

                /* Change the down image if exist. */
                if (button->stateFlag & rt2dANIMBUTTONSTATEDOWN)
                {
                    Rt2dObjectSetVisible(
                        Rt2dSceneGetChildByIndex(buttonScene, k),
                        stateTrans->buttonDownImg);
                    k++;
                }

                /* Change the over image if exist. */
                if (button->stateFlag & rt2dANIMBUTTONSTATEOVER)
                {
                    Rt2dObjectSetVisible(
                        Rt2dSceneGetChildByIndex(buttonScene, k),
                        stateTrans->buttonOverImg);
                }

                /* Post any new message for this state transition. */
                if (button->stateFlag & transFlag)
                {
                    /* Remove the first four bits. These are trans bit. */
                    stateFlag = (button->stateFlag >> 4);

                    /* Find the correct index for this transition in the action list. */
                    k = -1;
                    while (transFlag)
                    {
                        if (stateFlag & 0x01)
                            k++;

                        transFlag >>= 1;
                        stateFlag >>= 1;
                    }

                    RWASSERT(k >= 0);

                    /* Post the messages if it exist. */
                    if (button->actionListForStateTransition[k] >= 0)
                        _rt2dMaestroMessageListPostByIndex(maestro,
                                button->actionListForStateTransition[k]);
                }

                /* Update the mouse button */
                if (mouseButton)
                    button->buttonState |= RT2D_BUTTON_STATE_FLAG_LEFTBUTTON;
                else
                    button->buttonState &= (~RT2D_BUTTON_STATE_FLAG_LEFTBUTTON);
            }

            buttonIndex++;
        }

        animNode++;
    }

    /* Restore old anim index. */
    maestro->currAnimIndex = oldAnimIndex;

    maestro->prevMouse.button = mouseButton;

    RWRETURN(message);
}

static Rt2dMessage *
_rt2dHandlerDefaultButtonByLabel(Rt2dMaestro *maestro, Rt2dMessage *message)
{
    Rt2dAnimNode            *animNode;
    RwInt32                 buttonIndex, buttonNo, numButton, buttonLabel;
    RwInt32                 keyframeIndex;
    Rt2dCel                 *cel;

    RWFUNCTION(RWSTRING("_rt2dHandlerDefaultButtonByLabel"));

    RWASSERT(maestro);
    RWASSERT(message);

    animNode = _rt2dMaestroGetAnimNodeByIndex(maestro, message->index);

    /* Find if button is in current cel */
    if (message->intParam1<0)
    {
        RWRETURN( (Rt2dMessage *)NULL );
    }
    buttonLabel = message->intParam1;

    keyframeIndex = Rt2dAnimGetPrevFrameIndex(&animNode->anim);
    cel = _rt2dCelListGetCelByIndex(animNode->celList, keyframeIndex);
    buttonIndex = -1;
    numButton = _rwSListGetNumEntries(cel->buttonIndices);
    buttonNo = 0;

    while (buttonNo<numButton)
    {
        RwInt32 test;

        Rt2dCelListCelButtonGetDisplayVersion(
            maestro, animNode->celList, keyframeIndex, buttonNo,  &test
        );

        if (test==buttonLabel)
        {
            buttonIndex=buttonNo;
            break;
        }
        buttonNo++;
    }

    if (buttonIndex>=0)
    {
        RwInt32 *celButton = (RwInt32 *)_rwSListGetEntry(
                                    cel->buttonIndices, buttonIndex);
        RwUInt32 transition = message->intParam2;
        Rt2dButton *button = _rt2dMaestroGetButtonByIndex(maestro, *celButton);

        /* Post any new message for this state transition. */
        if (button->stateFlag & transition)
        {
            RwInt32 k;
            RwUInt32 stateFlag;

            /* Remove the first four bits. These are trans bit. */
            stateFlag = (button->stateFlag >> 4);

            /* Find the correct index for this transition in the action list. */
            k = -1;
            while (transition)
            {
                if (stateFlag & 0x01)
                    k++;

                transition >>= 1;
                stateFlag >>= 1;
            }

            RWASSERT(k >= 0);

            /* Post the messages if it exist. */
            if (button->actionListForStateTransition[k] >= 0)
            {
                RwInt32 store = maestro->currAnimIndex;
                maestro->currAnimIndex = message->index;
                _rt2dMaestroMessageListPostByIndex(maestro,
                        button->actionListForStateTransition[k]);
                maestro->currAnimIndex = store;
            }
        }
    }

    RWRETURN(message);
}

/****************************************************************************
 Functions.
 */


/**
 * \ingroup rt2dmessage
 * \ref Rt2dMessageHandlerDefaultCallBack is the default message handler. It supports most
 * message types. Messages that are foreign or require external data are ignored.
 *
 * All messages in the message queue are passed to a single message handler. It is the
 * handlers responsibility to either process or ignore messages. The maestro does not
 * filter any messages. The handler may post new messages to maestro but it must not
 * call \ref Rt2dMaestroProcessMessages.
 *
 * \param maestro              Pointer to parent maestro.
 * \param message               Pointer to the message.
 *
 * \return Returns a pointer to the message if successful, NULL otherwise.
 *
 * \see Rt2dMaestroPostMessages
 * \see Rt2dMaestroProcessMessages
 * \see Rt2dMaestroSetMessageHandler
 * \see Rt2dMaestroGetMessageHandler
 */

Rt2dMessage *
Rt2dMessageHandlerDefaultCallBack(Rt2dMaestro *maestro, Rt2dMessage *message)
{
    Rt2dMessage             *result;

    RWAPIFUNCTION(RWSTRING("Rt2dMessageHandlerDefaultCallBack"));

    RWASSERT(message);

    switch (message->messageType)
    {
        case rt2dMESSAGETYPEPLAY :
        {
            result = _rt2dHandlerDefaultPlay(maestro, message);
            break;
        }
        case rt2dMESSAGETYPESTOP :
        {
            result = _rt2dHandlerDefaultStop(maestro, message);
            break;
        }
        case rt2dMESSAGETYPENEXTFRAME :
        {
            result = _rt2dHandlerDefaultNextFrame(maestro, message);
            break;
        }
        case rt2dMESSAGETYPEPREVFRAME :
        {
            result = _rt2dHandlerDefaultPrevFrame(maestro, message);
            break;
        }
        case rt2dMESSAGETYPEGOTOFRAME :
        {
            result = _rt2dHandlerDefaultGotoFrame(maestro, message);
            break;
        }
        case rt2dMESSAGETYPEGOTOLABEL :
        {
            result = _rt2dHandlerDefaultGotoLabel(maestro, message);
            break;
        }
        case rt2dMESSAGETYPEGETURL :
        {
            result = _rt2dHandlerDefaultGetURL(maestro, message);
            break;
        }
        case rt2dMESSAGETYPEDOACTION :
        {
            result = _rt2dHandlerDefaultDoAction(maestro, message);
            break;
        }
        case rt2dMESSAGETYPEFOREIGN :
        {
            result = _rt2dHandlerDefaultForeign(maestro, message);
            break;
        }
        case rt2dMESSAGETYPEMOUSEMOVETO :
        {
            result = _rt2dHandlerDefaultMouseMoveTo(maestro, message);
            break;
        }
        case rt2dMESSAGETYPEMOUSEBUTTONSTATE :
        {
            result = _rt2dHandlerDefaultMouseButtonState(maestro, message);
            break;
        }
        case rt2dMESSAGETYPEBUTTONBYLABEL :
        {
            result = _rt2dHandlerDefaultButtonByLabel(maestro, message);
            break;
        }

        default :
        {
            /* Assert unknown message ? */
            result = NULL;
            break;
        }
    }

    RWRETURN(result);
}

/****************************************************************************
 *
 * Maestro's message handler functions.
 *
 **************************************************************************/

/**
 * \ingroup rt2dmessage
 * \ref Rt2dMaestroGetMessageHandler is used to retrieve the current message handler
 * in the maestro. There is one handler for all messages.
 *
 * \param maestro              Pointer to parent maestro.
 *
 * \return Returns a pointer to the message handler if successful, NULL otherwise.
 *
 * \see Rt2dMaestroPostMessages
 * \see Rt2dMaestroProcessMessages
 * \see Rt2dMaestroSetMessageHandler
 * \see Rt2dMessageHandlerDefaultCallBack
 */

Rt2dMessageHandlerCallBack
Rt2dMaestroGetMessageHandler(Rt2dMaestro *maestro)
{
    RWAPIFUNCTION(RWSTRING("Rt2dMaestroGetMessageHandler"));

    RWASSERT(maestro);

    RWRETURN(maestro->messageHandler);
}

/**
 * \ingroup rt2dmessage
 * \ref Rt2dMaestroSetMessageHandler is used change the current message handler to
 * the given one. There is only one message handler for all messages.
 *
 * \param maestro              Pointer to parent maestro.
 * \param handler              Pointer to message handler callback.
 *
 * \return Returns a pointer to the message handler if successful, NULL otherwise.
 *
 * \see Rt2dMaestroPostMessages
 * \see Rt2dMaestroProcessMessages
 * \see Rt2dMaestroGetMessageHandler
 * \see Rt2dMessageHandlerDefaultCallBack
 */

Rt2dMaestro *
Rt2dMaestroSetMessageHandler(Rt2dMaestro *maestro, Rt2dMessageHandlerCallBack handler)
{
    RWAPIFUNCTION(RWSTRING("Rt2dMaestroSetMessageHandler"));

    RWASSERT(maestro);

    maestro->messageHandler = handler;

    RWRETURN(maestro);
}


#if defined (__MWERKS__)
#if (defined(RWVERBOSE))
#pragma message (__FILE__ "/" _SKY_EXPAND(__LINE__) ": __MWERKS__ == " _SKY_EXPAND(__MWERKS__))
#endif /* (defined (__MWERKS__)) */
#if (__option (global_optimizer))
#pragma always_inline on
#endif /* (__option (global_optimizer)) */
#endif /*  defined (__MWERKS__) */
