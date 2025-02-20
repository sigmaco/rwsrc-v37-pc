/**
 * \ingroup rt2dmessage
 * \page rt2dmessageoverview Rt2dMessage Overview
 *
 * Rt2dMessage is a message structure that is used to coordinate animation
 * sequences by \ref Rt2dMaestro.
 *
 * It may also be used by calling code to notify the Maestro of external events.
 * This is carried out through the use of the \ref Rt2dMaestroPostMessages and
 * \ref Rt2dMaestroProcessMessages functions.
 *
 * Additionally, if a custom message handler is hooked to \ref Rt2dMaestro,
 * \ref Rt2dMessage structures are passed to that handler. These may be examined
 * to determine when particular animation events have occurred.
 *
 * Rt2dMessage contains several pieces of information.
 * \ref Rt2dMessageType messageType
 *     - message identifier
 * \ref RwInt32 index
 *     - animation # to apply any actions to
 * \ref RwInt32 intParam1;
 *     - first param (message dependant)
 * \ref RwInt32 intParam2;
 *     - second param (message dependant)
 *
 * The messageType identifies how the message is interpreted by Rt2dMaestro. It may be one of the following:
 *
 * rt2dMESSAGETYPEPLAY
 *    Play animation
 * rt2dMESSAGETYPESTOP
 *    Stop animation
 * rt2dMESSAGETYPENEXTFRAME
 *    Advance to next frame
 * rt2dMESSAGETYPEPREVFRAME
 *    Rewind to previouse frame
 * rt2dMESSAGETYPEGOTOFRAME
 *    Advance to frame by index
 * rt2dMESSAGETYPEGOTOLABEL
 *    Advance to frame by label
 * rt2dMESSAGETYPEGETURL
 *    Get URL
 * rt2dMESSAGETYPEFOREIGN
 *    Application specific message
 * rt2dMESSAGETYPEMOUSEMOVETO
 *    Move mouse
 * rt2dMESSAGETYPEMOUSEBUTTONSTATE
 *    Mouse button up or down
 * rt2dMESSAGETYPEBUTTONBYLABEL
 *    Mouse button transition by label
 *
 * This message type identifies how the other parameters are to be interpreted.
 *
 * index is generally used to identify which animation within Rt2dMaestro that the message applies to. Messages posted externally may be sent to specific animations.
 *
 * A description of the individual message types follows:
 *
 * rt2dMESSAGETYPEPLAY
 *    Starts an animation playing.
 *    index is the animation that will start playing.
 *    intParam1, intParam2 are unused
 *
 * rt2dMESSAGETYPESTOP
 *    Stops an animation.
 *    index is the animation that will stop playing.
 *    intParam1, intParam2 are unused
 *
 * rt2dMESSAGETYPENEXTFRAME
 *    Advances an animation to the next frame.
 *    index is the animation that will be advanced.
 *    intParam1, intParam2 are unused
 *
 * rt2dMESSAGETYPEPREVFRAME
 *    Rewinds an animation to the previous frame.
 *    index is the animation that will be rewound.
 *    intParam1, intParam2 are unused
 *
 * rt2dMESSAGETYPEGOTOFRAME
 *    Goto to frame by index.
 *    index is the animation that will go to the specified frame.
 *    intParam1 is the frame number to go to, starting at frame 0.
 *    intParam2 is unused
 *
 * rt2dMESSAGETYPEGOTOLABEL
 *    Advance to frame by label.
 *    index is the animation that will go to the specified frame.
 *    intParam1 is the index of the string label that contains the frame label of the frame to go to.
 *    intParam2 is unused
 *
 * rt2dMESSAGETYPEGETURL
 *    Can be used as an extension mechanism. During content generation, a GetURL action may be specified with a string, eg GetURL("StartGame"). By hooking a custom message handler, the GetURL message may be intercepted and user to trigger in-game events. Nothing is done with this message by the default handler.
 *    index is the animation that the message was issued within.
 *    intParam1 is the index of the string label that was specified in the GetURL action.
 *    intParam2 is unused
 *
 * rt2dMESSAGETYPEDOACTION
 *    Queues an internal list of messages. Not intended for external use.
 *    index is the animation that the message was issued within.
 *    intParam1 is the index of the message list to be queued.
 *    intParam2 is unused
 *
 * rt2dMESSAGETYPEFOREIGN
 *    An extension mechanism for the inclusion of binary data. Reserved for future use.
 *    index is the animation that the message was issued within.
 *    intParam1 is the index of the binary data within an internal binary data store.
 *    intParam2 is unused
 *
 * rt2dMESSAGETYPEMOUSEMOVETO
 *    Notification that the mouse has changed position. Causes update of button states.
 *    index is unused.
 *    intParam1 is the new mouse X coordinate.
 *    intParam2 is the new mouse Y coordinate
 *
 * rt2dMESSAGETYPEMOUSEBUTTONSTATE
 *    Notification that the mouse button state has changed.
 *    index is unused.
 *    intParam1 is the button state; 0=up; 1=down.
 *    intParam2 is unused
 *
 * rt2dMESSAGETYPESPECIALTELLTARGET
 *    Used internally to control which animation stored messages are applied to. Not for external use.
 *
 * rt2dMESSAGETYPEBUTTONBYLABEL
 *    Cause a button transition on a button identified by a string label. May be used to pipe in external button presses to specific buttons identified by a name registered in a string labels. Buttons may be named in third party tools through an  'Export' option in those tools.
 *    index is the animation to send the button push to.
 *    intParam1 is the index of the string label naming the button.
 *    intParam2 is the button transition to be triggered, allowable transitions are
 *         rt2dANIMBUTTONSTATEIDLETOOVERUP
 *         rt2dANIMBUTTONSTATEOVERUPTOIDLE
 *         rt2dANIMBUTTONSTATEOVERUPTOOVERDOWN
 *         rt2dANIMBUTTONSTATEOVERDOWNTOOVERUP
 *         rt2dANIMBUTTONSTATEOVERDOWNTOOUTDOWN
 *         rt2dANIMBUTTONSTATEOUTDOWNTOOVERDOWN
 *         rt2dANIMBUTTONSTATEOUTDOWNTOIDLE
 *         rt2dANIMBUTTONSTATEIDLETOOVERDOWN
 *         rt2dANIMBUTTONSTATEOVERDOWNTOIDLE
 *
 * \see Rt2dMaestroPostMessages
 * \see Rt2dMaestroProcessMessages
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
 *  Module  :   message.c                                                   *
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
Rt2dMessageDescrip _rt2dMessageDescripTable[RT2D_MESSAGE_MAX_MESSAGES]
={
    {   &_rt2dMessageStreamGetSizeDefaultCallBack,
        &_rt2dMessageStreamReadDefaultCallBack,
        &_rt2dMessageStreamWriteDefaultCallBack    },
    {   &_rt2dMessageStreamGetSizeDefaultCallBack,
        &_rt2dMessageStreamReadDefaultCallBack,
        &_rt2dMessageStreamWriteDefaultCallBack    },
    {   &_rt2dMessageStreamGetSizeDefaultCallBack,
        &_rt2dMessageStreamReadDefaultCallBack,
        &_rt2dMessageStreamWriteDefaultCallBack    },
    {   &_rt2dMessageStreamGetSizeDefaultCallBack,
        &_rt2dMessageStreamReadDefaultCallBack,
        &_rt2dMessageStreamWriteDefaultCallBack    },
    {   &_rt2dMessageStreamGetSizeDefaultCallBack,
        &_rt2dMessageStreamReadDefaultCallBack,
        &_rt2dMessageStreamWriteDefaultCallBack    },
    {   &_rt2dMessageStreamGetSizeDefaultCallBack,
        &_rt2dMessageStreamReadDefaultCallBack,
        &_rt2dMessageStreamWriteDefaultCallBack    },
    {   &_rt2dMessageStreamGetSizeDefaultCallBack,
        &_rt2dMessageStreamReadDefaultCallBack,
        &_rt2dMessageStreamWriteDefaultCallBack    },
    {   &_rt2dMessageStreamGetSizeDefaultCallBack,
        &_rt2dMessageStreamReadDefaultCallBack,
        &_rt2dMessageStreamWriteDefaultCallBack    },
    {   &_rt2dMessageStreamGetSizeDefaultCallBack,
        &_rt2dMessageStreamReadDefaultCallBack,
        &_rt2dMessageStreamWriteDefaultCallBack    },
    {   &_rt2dMessageStreamGetSizeDefaultCallBack,
        &_rt2dMessageStreamReadDefaultCallBack,
        &_rt2dMessageStreamWriteDefaultCallBack    },
    {   &_rt2dMessageStreamGetSizeDefaultCallBack,
        &_rt2dMessageStreamReadDefaultCallBack,
        &_rt2dMessageStreamWriteDefaultCallBack    },
    {   &_rt2dMessageStreamGetSizeDefaultCallBack,
        &_rt2dMessageStreamReadDefaultCallBack,
        &_rt2dMessageStreamWriteDefaultCallBack    },
    {   &_rt2dMessageStreamGetSizeDefaultCallBack,
        &_rt2dMessageStreamReadDefaultCallBack,
        &_rt2dMessageStreamWriteDefaultCallBack    },
    {   &_rt2dMessageStreamGetSizeDefaultCallBack,
        &_rt2dMessageStreamReadDefaultCallBack,
        &_rt2dMessageStreamWriteDefaultCallBack    },
    {   &_rt2dMessageStreamGetSizeDefaultCallBack,
        &_rt2dMessageStreamReadDefaultCallBack,
        &_rt2dMessageStreamWriteDefaultCallBack    },
    {   &_rt2dMessageStreamGetSizeDefaultCallBack,
        &_rt2dMessageStreamReadDefaultCallBack,
        &_rt2dMessageStreamWriteDefaultCallBack    },
    {   &_rt2dMessageStreamGetSizeDefaultCallBack,
        &_rt2dMessageStreamReadDefaultCallBack,
        &_rt2dMessageStreamWriteDefaultCallBack    }
};


/****************************************************************************
 Local (static) Globals
 */
static RwUInt32 _rwDefaultMessageQueueSize = RT2D_MESSAGE_DEFAULT_MAX_QUEUE;

/****************************************************************************
 Functions.
 */

/**
 * \ingroup rt2dmessage
 * \ref Rt2dMessageQueueSetDefaultSize is used to set the default size of the
 * message queue that each \ref Rt2dMaestro will use.
 *
 * \param size           Size of the message queue.
 */
void
Rt2dMessageQueueSetDefaultSize(RwUInt32 size)
{
    RWAPIFUNCTION(RWSTRING("Rt2dMessageQueueSetDefaultSize"));

    _rwDefaultMessageQueueSize = size;

    RWRETURNVOID();
}



/****************************************************************************
 *
 *  Message descrip table functions.
 *
 ****************************************************************************/

Rt2dMessageDescrip *
_rt2dMessageDescripTableInit( Rt2dMessageDescrip *messageDescripTable )
{
    RwInt32             i;

    RWFUNCTION(RWSTRING("_rt2dMessageDescripTableInit"));

    RWASSERT(messageDescripTable);

    /* Load the table with default vals. */
    for (i = 0; i < RT2D_MESSAGE_MAX_MESSAGES; i++)
    {
        messageDescripTable[i].funcStreamGetSize =
            _rt2dMessageStreamGetSizeDefaultCallBack;
        messageDescripTable[i].funcStreamRead =
            _rt2dMessageStreamReadDefaultCallBack;
        messageDescripTable[i].funcStreamWrite =
            _rt2dMessageStreamWriteDefaultCallBack;
    }

    RWRETURN(messageDescripTable);
}

Rt2dMessageDescrip *
_rt2dMessageTypeSetStreamGetSizeCallBack(
    Rt2dMessageDescrip                  *messageDescripTable,
    Rt2dMessageType                     messageType,
    Rt2dMessageStreamGetSizeCallBack    funcStreamGetSize)
{
    RWFUNCTION(RWSTRING("_rt2dMessageTypeSetStreamGetSizeCallBack"));

    RWASSERT(messageDescripTable);
    RWASSERT(messageType < RT2D_MESSAGE_MAX_MESSAGES);

    /* Load the table with default vals. */
    if (funcStreamGetSize)
    {
        messageDescripTable[messageType].funcStreamGetSize =
            funcStreamGetSize;
    }
    else
    {
        messageDescripTable[messageType].funcStreamGetSize =
            _rt2dMessageStreamGetSizeDefaultCallBack;
    }

    RWRETURN(messageDescripTable);
}

Rt2dMessageStreamGetSizeCallBack
_rt2dMessageTypeGetStreamGetSizeCallBack(
    Rt2dMessageDescrip                  *messageDescripTable,
    Rt2dMessageType                     messageType)
{
    RWFUNCTION(RWSTRING("_rt2dMessageTypeGetStreamGetSizeCallBack"));

    RWASSERT(messageDescripTable);
    RWASSERT(messageType < RT2D_MESSAGE_MAX_MESSAGES);

    /* Return the message type's stream function. */
    RWRETURN(messageDescripTable[messageType].funcStreamGetSize);
}


Rt2dMessageDescrip *
_rt2dMessageTypeSetStreamReadCallBack(
    Rt2dMessageDescrip                  *messageDescripTable,
    Rt2dMessageType                     messageType,
    Rt2dMessageStreamReadCallBack       funcStreamRead)
{
    RWFUNCTION(RWSTRING("_rt2dMessageTypeSetStreamReadCallBack"));

    RWASSERT(messageDescripTable);
    RWASSERT(messageType < RT2D_MESSAGE_MAX_MESSAGES);

    /* Load the table with default vals. */
    if (funcStreamRead)
    {
        messageDescripTable[messageType].funcStreamRead =
            funcStreamRead;
    }
    else
    {
        messageDescripTable[messageType].funcStreamRead =
            _rt2dMessageStreamReadDefaultCallBack;
    }

    RWRETURN(messageDescripTable);
}

Rt2dMessageStreamReadCallBack
_rt2dMessageTypeGetStreamReadCallBack(
    Rt2dMessageDescrip                  *messageDescripTable,
    Rt2dMessageType                     messageType)
{
    RWFUNCTION(RWSTRING("_rt2dMessageTypeGetStreamReadCallBack"));

    RWASSERT(messageDescripTable);
    RWASSERT(messageType < RT2D_MESSAGE_MAX_MESSAGES);

    /* Return the message type's stream function. */
    RWRETURN(messageDescripTable[messageType].funcStreamRead);
}


Rt2dMessageDescrip *
_rt2dMessageTypeSetStreamWriteCallBack(
    Rt2dMessageDescrip                  *messageDescripTable,
    Rt2dMessageType                     messageType,
    Rt2dMessageStreamWriteCallBack      funcStreamWrite)
{
    RWFUNCTION(RWSTRING("_rt2dMessageTypeSetStreamWriteCallBack"));

    RWASSERT(messageDescripTable);
    RWASSERT(messageType < RT2D_MESSAGE_MAX_MESSAGES);

    /* Load the table with default vals. */
    if (funcStreamWrite)
    {
        messageDescripTable[messageType].funcStreamWrite =
            funcStreamWrite;
    }
    else
    {
        messageDescripTable[messageType].funcStreamWrite =
            _rt2dMessageStreamWriteDefaultCallBack;
    }

    RWRETURN(messageDescripTable);
}

Rt2dMessageStreamWriteCallBack
_rt2dMessageTypeGetStreamWriteCallBack(
    Rt2dMessageDescrip                  *messageDescripTable,
    Rt2dMessageType                     messageType)
{
    RWFUNCTION(RWSTRING("_rt2dMessageTypeGetStreamWriteCallBack"));

    RWASSERT(messageDescripTable);
    RWASSERT(messageType < RT2D_MESSAGE_MAX_MESSAGES);

    /* Return the message type's stream function. */
    RWRETURN(messageDescripTable[messageType].funcStreamWrite);
}


/****************************************************************************
 *
 *  Message functions.
 *
 ****************************************************************************/

Rt2dMessage *
_rt2dMessageInit( Rt2dMessage *message)
{
    RWFUNCTION(RWSTRING("_rt2dMessageInit"));

    RWASSERT(message);

    message->messageType = rt2dMESSAGETYPENULL;
    message->index = -1;
    message->intParam1 = 0;
    message->intParam1 = 0;

    RWRETURN(message);
}

Rt2dMessage *
_rt2dMessageDestruct( Rt2dMessage *message)
{
    RWFUNCTION(RWSTRING("_rt2dMessageDestruct"));

    RWASSERT(message);

    message->messageType = rt2dMESSAGETYPENULL;
    message->index = -1;
    message->intParam1 = 0;
    message->intParam1 = 0;

    RWRETURN(message);
}

/****************************************************************************/

Rt2dMessage *
_rt2dMessageCreate( void )
{
    Rt2dMessage         *message;

    RWFUNCTION(RWSTRING("_rt2dMessageCreate"));

    if ((message = (Rt2dMessage *)RwFreeListAlloc(
                                     &Rt2dAnimGlobals.messageFreeList,
                                     rwID_2DANIM | rwMEMHINTDUR_EVENT)) == NULL)
    {
        RWRETURN((Rt2dMessage *)NULL);
    }

        /* Initialise the members. */
    if (_rt2dMessageInit(message) == NULL)
    {
        _rt2dMessageDestruct(message);

        RwFreeListFree(&Rt2dAnimGlobals.messageFreeList, message);

        message = (Rt2dMessage *)NULL;
    }

    RWRETURN(message);
}

RwBool
_rt2dMessageDestroy( Rt2dMessage *message )
{
    RWFUNCTION(RWSTRING("_rt2dMessageDestroy"));

    RWASSERT(message);

    /* Destruct the members. */
    _rt2dMessageDestruct(message);

    RwFreeListFree(&Rt2dAnimGlobals.messageFreeList, message);

    RWRETURN(TRUE);
}

/****************************************************************************/

RwInt32
_rt2dMessageStreamGetSize(Rt2dMaestro *maestro, Rt2dMessage *message)
{
    RwInt32             size;

    RWFUNCTION(RWSTRING("_rt2dMessageStreamGetSize"));

    RWASSERT(message);

    size = 0;

    /* Size of a message. */
    size += sizeof(Rt2dMessage);

    /* Size of any private data. */
    RWASSERT(_rt2dMessageDescripTable[message->messageType].funcStreamGetSize);

    size += (_rt2dMessageDescripTable[message->messageType].funcStreamGetSize)
            (maestro, message);

    RWRETURN(size);
}

Rt2dMessage *
_rt2dMessageStreamRead(Rt2dMaestro *maestro, Rt2dMessage *message, RwStream *stream)
{
    RWFUNCTION(RWSTRING("_rt2dMessageStreamRead"));

    RWASSERT(stream);

    /* Read the messageType. */
    if (RwStreamReadInt32(stream, (RwInt32 *)&message->messageType, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dMessage *)NULL);
    }

    /* Read the animation. */
    if (RwStreamReadInt32(stream, &message->index, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dMessage *)NULL);
    }

    /* Read the intParam1. */
    if (RwStreamReadInt32(stream, &message->intParam1, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dMessage *)NULL);
    }

    /* Read the intParam2. */
    if (RwStreamReadInt32(stream, &message->intParam2, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dMessage *)NULL);
    }

    /* Read in any private data. */
    RWASSERT(_rt2dMessageDescripTable[message->messageType].funcStreamRead);

    if ((_rt2dMessageDescripTable[message->messageType].funcStreamRead)
            (maestro, message, stream) == NULL)
    {
        RWRETURN((Rt2dMessage *)NULL);
    }

    RWRETURN(message);
}

Rt2dMessage *
_rt2dMessageStreamWrite(Rt2dMaestro *maestro, Rt2dMessage *message, RwStream *stream)
{
    RWFUNCTION(RWSTRING("_rt2dMessageStreamWrite"));

    RWASSERT(message);
    RWASSERT(stream);

    /* Write out the messageType. */
    if (RwStreamWriteInt32(stream, (RwInt32 *)&message->messageType, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dMessage *)NULL);
    }

    /* Write out the animation index. */
    if (RwStreamWriteInt32(stream, &message->index, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dMessage *)NULL);
    }

    /* Write out the params. */
    if (RwStreamWriteInt32(stream, &message->intParam1, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dMessage *)NULL);
    }

    if (RwStreamWriteInt32(stream, &message->intParam2, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dMessage *)NULL);
    }

    /* Write out any private data. */
    RWASSERT(_rt2dMessageDescripTable[message->messageType].funcStreamWrite);

    if ((_rt2dMessageDescripTable[message->messageType].funcStreamWrite)
            (maestro, message, stream) == NULL)
    {
        RWRETURN((Rt2dMessage *)NULL);
    }

    RWRETURN(message);
}

/****************************************************************************/

RwInt32
_rt2dMessageStreamGetSizeDefaultCallBack(Rt2dMaestro *maestro __RWUNUSED__,
                                    Rt2dMessage *message __RWUNUSEDRELEASE__)
{
    RWFUNCTION(RWSTRING("_rt2dMessageStreamGetSizeDefaultCallBack"));

    RWASSERT(message);

    RWRETURN(0);
}

Rt2dMessage *
_rt2dMessageStreamReadDefaultCallBack(Rt2dMaestro *maestro __RWUNUSED__,
                                    Rt2dMessage *message,
                                    RwStream *stream __RWUNUSEDRELEASE__)
{
    RWFUNCTION(RWSTRING("_rt2dMessageStreamReadDefaultCallBack"));

    RWASSERT(message);
    RWASSERT(stream);

    RWRETURN(message);
}

Rt2dMessage *
_rt2dMessageStreamWriteDefaultCallBack(Rt2dMaestro *maestro __RWUNUSED__,
                                        Rt2dMessage *message,
                                        RwStream *stream __RWUNUSEDRELEASE__)
{
    RWFUNCTION(RWSTRING("_rt2dMessageStreamWriteDefaultCallBack"));

    RWASSERT(message);
    RWASSERT(stream);

    RWRETURN(message);
}

/****************************************************************************/

/**
 * \ingroup rt2dmessage
 * \ref Rt2dMessageGetMessageType is used to get the message's type, (\ref Rt2dMessageType).
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param message           Pointer to the message.
 *
 * \return Returns the message type.
 *
 * \see Rt2dMessageType
 * \see Rt2dMessageSetMessageType
 */

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
Rt2dMessageType
Rt2dMessageGetMessageType( Rt2dMessage *message )
{
    Rt2dMessageType         messageType;

    RWAPIFUNCTION(RWSTRING("Rt2dMessageGetMessageType"));

    RWASSERT(message);

    messageType = (Rt2dMessageType)_rt2dMessageGetMessageTypeMacro(message);

    RWRETURN(messageType);
}
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/**
 * \ingroup rt2dmessage
 * \ref Rt2dMessageSetMessageType is used to set the type of the message,
 * (\ref Rt2dMessageType).
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param message           Pointer to the message.
 * \param messageType       Message type.
 *
 * \see Rt2dMessageType
 * \see Rt2dMessageGetMessageType
 */

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
void
Rt2dMessageSetMessageType(Rt2dMessage *message, Rt2dMessageType messageType)
{
    RWAPIFUNCTION(RWSTRING("Rt2dMessageSetMessageType"));

    RWASSERT(message);

    _rt2dMessageSetMessageTypeMacro(message, messageType);

    RWRETURNVOID();
}
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/**
 * \ingroup rt2dmessage
 * \ref Rt2dMessageGetIndex is used retrieve the animation index which
 * the message is to be applied to.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param message           Pointer to the message.
 *
 * \return Returns the animation's index if successful, -1 otherwise.
 *
 * \see Rt2dMessageSetIndex
 * \see Rt2dMaestroAddAnimations
 */

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
RwInt32
Rt2dMessageGetIndex(Rt2dMessage *message)
{
    RwInt32         index;

    RWAPIFUNCTION(RWSTRING("Rt2dMessageGetIndex"));

    RWASSERT(message);

    index = _rt2dMessageGetIndexMacro(message);

    RWRETURN(index);
}
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/**
 * \ingroup rt2dmessage
 * \ref Rt2dMessageSetIndex is used to set the animation the message
 * is applied to. A -1 indicates it is to be applied to the current
 * animation.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param message           Pointer to the message.
 * \param index             Animation index.
 *
 * \see Rt2dMessageGetIndex
 * \see Rt2dMaestroAddAnimations
 */

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
void
Rt2dMessageSetIndex(Rt2dMessage *message, RwInt32 index)
{
    RWAPIFUNCTION(RWSTRING("Rt2dMessageSetIndex"));

    RWASSERT(message);

    _rt2dMessageSetIndexMacro(message, index);

    RWRETURNVOID();
}
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/**
 * \ingroup rt2dmessage
 * \ref Rt2dMessageGetParam is used to retrieve the message specific
 * parameters. Each message may have none, one or two additional parameters.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param message           Pointer to the message.
 * \param param1            Pointer to a RwInt32 to return the first parameter.
 * \param param2            Pointer to a RwInt32 to return the second parameter.
 *
 * \see Rt2dMessageSetParam
 */

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
void
Rt2dMessageGetParam(Rt2dMessage *message, RwInt32 *param1, RwInt32 *param2)
{
    RWAPIFUNCTION(RWSTRING("Rt2dMessageGetParam"));

    RWASSERT(message);

    _rt2dMessageGetParamMacro(message, param1, param2);

    RWRETURNVOID();
}
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/**
 * \ingroup rt2dmessage
 * \ref Rt2dMessageSetParam is used to set the message specific
 * parameters. Each message may have none, one or two additional parameters.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param message           Pointer to the message.
 * \param param1            The first addtional parameter.
 * \param param2            The second additional parameter.
 *
 * \see Rt2dMessageGetParam
 */

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
void
Rt2dMessageSetParam(Rt2dMessage *message, RwInt32 param1, RwInt32 param2)
{
    RWAPIFUNCTION(RWSTRING("Rt2dMessageSetParam"));

    RWASSERT(message);

    _rt2dMessageSetParamMacro(message, param1, param2);

    RWRETURNVOID();
}
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/****************************************************************************/

RwSList *
_rt2dMessageStoreCreate(void)
{
    RwSList             *messageStore;

    RWFUNCTION(RWSTRING("_rt2dMessageStoreCreate"));

    messageStore = _rwSListCreate(sizeof(Rt2dMessage),
        rwID_2DANIM | rwMEMHINTDUR_EVENT);

    RWRETURN(messageStore);
}

RwBool
_rt2dMessageStoreDestroy(RwSList *messageStore)
{
    RwInt32                 i, num;
    Rt2dMessage             *message;

    RWFUNCTION(RWSTRING("_rt2dMessageStoreDestroy"));

    RWASSERT(messageStore);

    num = _rwSListGetNumEntries(messageStore);
    if (num > 0)
    {
        message = (Rt2dMessage *)_rwSListGetArray(messageStore);

        RWASSERT(message);

        /* Clear down the messages in the store. */
        for (i = 0; i < num; i++)
        {
            _rt2dMessageDestruct(message);

            message++;
        }
    }

    _rwSListDestroy(messageStore);

    RWRETURN(TRUE);
}

RwSList *
_rt2dMessageStoreAddMessages(RwSList *messageStore,
                             Rt2dMessage *message, RwInt32 length,
                             RwInt32 *index)
{
    Rt2dMessage         *newMessage;

    RWFUNCTION(RWSTRING("_rt2dMessageStoreAddMessages"));

    RWASSERT(messageStore);
    /* RWASSERT(length > 0); */

    if (index)
    {
        *index = _rwSListGetNumEntries(messageStore);
    }

    if (length)
    {
        /* Ask for a new data area. */
        if ((newMessage = (Rt2dMessage *)_rwSListGetNewEntries(messageStore,
                length, rwID_2DANIM | rwMEMHINTDUR_EVENT))
              == NULL)
        {
            RWRETURN( (RwSList *) NULL);
        }

        /* copy message into storage */
        if (message)
        {
            memcpy(newMessage, message, length * sizeof(Rt2dMessage));
        }
    }

    RWRETURN(messageStore);
}

Rt2dMessage *
_rt2dMessageStoreGetMessageByIndex( RwSList * messageStore, RwInt32 idx )
{
    Rt2dMessage         *message;

    RWFUNCTION(RWSTRING("_rt2dMessageStoreGetMessageByIndex"));

    RWASSERT(messageStore);
    RWASSERT(idx >= 0);
    RWASSERT(idx < _rwSListGetNumEntries(messageStore));

    message = (Rt2dMessage *)_rwSListGetEntry(messageStore, idx);

    RWRETURN(message);
}

/****************************************************************************
 *
 * Maestro's message functions.
 *
 **************************************************************************/

Rt2dMaestro *
_rt2dMaestroCreateMessageList(Rt2dMaestro *maestro)
{
    RWFUNCTION(RWSTRING("_rt2dMaestroCreateMessageList"));

    RWASSERT(maestro);

    if ((maestro->messageList = _rwSListCreate(sizeof(RwInt32),
            rwID_2DANIM | rwMEMHINTDUR_EVENT)) == NULL)
    {
        RWRETURN((Rt2dMaestro *)NULL);
    }

    if ((maestro->messageStore = _rt2dMessageStoreCreate()) == NULL)
    {
        RWRETURN((Rt2dMaestro *)NULL);
    }

    RWRETURN(maestro);
}

Rt2dMaestro *
_rt2dMaestroDestroyMessageList(Rt2dMaestro *maestro)
{
    RWFUNCTION(RWSTRING("_rt2dMaestroDestroyMessageList"));

    RWASSERT(maestro);

    if (maestro->messageList)
    {
        _rwSListDestroy(maestro->messageList);
    }

    maestro->messageList = (RwSList *)NULL;

    if (maestro->messageStore)
    {
        _rt2dMessageStoreDestroy(maestro->messageStore);
    }

    maestro->messageStore = (RwSList *)NULL;

    RWRETURN(maestro);
}


RwInt32
_rt2dMaestroMessageListStreamGetSize(Rt2dMaestro *maestro)
{
    RwInt32             size;

    RWFUNCTION(RWSTRING("_rt2dMaestroMessageListStreamGetSize"));

    RWASSERT(maestro);

    /* Message list count. */
    size = sizeof(RwInt32);

    /* Size of the message list array. */
    if (maestro->messageList)
    {
        size += sizeof(RwInt32) * _rwSListGetNumEntries(maestro->messageList);
    }

    RWRETURN(size);
}

Rt2dMaestro *
_rt2dMaestroMessageListStreamRead(Rt2dMaestro *maestro, RwStream *stream)
{
    RwInt32             num, *beginMessageList;

    RWFUNCTION(RWSTRING("_rt2dMaestroMessageListStreamRead"));

    RWASSERT(maestro);
    RWASSERT(stream);

    /* Read the num of message lists. */
    if (RwStreamReadInt32(stream, &num, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dMaestro *)NULL);
    }

    if (num > 0)
    {
        if (maestro->messageList == NULL)
        {
            _rt2dMaestroCreateMessageList(maestro);
        }

        if (maestro->messageList)
        {
            if ((beginMessageList = (RwInt32 *)
                   _rwSListGetNewEntries(maestro->messageList, num,
                        rwID_2DANIM | rwMEMHINTDUR_EVENT)) == NULL)
            {
                RWRETURN((Rt2dMaestro *)NULL);
            }

            if (RwStreamReadInt32(stream, beginMessageList,
                num * sizeof(RwInt32)) == NULL)
            {
                RWRETURN((Rt2dMaestro *)NULL);
            }
        }
        else
        {
            /* Failed to create the message list. */
            RWRETURN((Rt2dMaestro *)NULL);
        }
    }

    RWRETURN(maestro);
}


Rt2dMaestro *
_rt2dMaestroMessageListStreamWrite(Rt2dMaestro *maestro, RwStream *stream)
{
    RwInt32             num, *messageList;

    RWFUNCTION(RWSTRING("_rt2dMaestroMessageListStreamWrite"));

    RWASSERT(maestro);
    RWASSERT(stream);

    /* Write out the messageType. */
    if (maestro->messageList)
    {
        /* Write out the number of message list. */
        num = _rwSListGetNumEntries(maestro->messageList);
        if (RwStreamWriteInt32(stream, &num, sizeof(RwInt32)) == NULL)
        {
            RWRETURN((Rt2dMaestro *)NULL);
        }

        if (num > 0)
        {
            messageList = (RwInt32 *)_rwSListGetArray(maestro->messageList);

            RWASSERT(messageList);

            if (RwStreamWriteInt32(stream, messageList,
                (num * sizeof(RwInt32))) == NULL)
            {
                RWRETURN((Rt2dMaestro *)NULL);
            }
        }
    }
    else
    {
        /* Empty message list. */
        num = 0;
        if (RwStreamWriteInt32(stream, &num, sizeof(RwInt32)) == NULL)
        {
            RWRETURN((Rt2dMaestro *)NULL);
        }
    }

    RWRETURN(maestro);
}


Rt2dMessage *
_rt2dMaestroMessageListGetByIndex(Rt2dMaestro *maestro, RwInt32 idx)
{
    Rt2dMessage             *message;
    RwInt32                 *messageList, *beginMessageList;

    RWFUNCTION(RWSTRING("_rt2dMaestroMessageListGetByIndex"));

    RWASSERT(maestro);
    RWASSERT(idx > 0);
    RWASSERT(idx < _rwSListGetNumEntries(maestro->messageList));

    RWASSERT(maestro->messageStore);
    RWASSERT(maestro->messageList);

    beginMessageList = (RwInt32 *)_rwSListGetArray(maestro->messageList);
    messageList = beginMessageList + idx;

    RWASSERT(messageList);

    message = _rt2dMessageStoreGetMessageByIndex(maestro->messageStore,
                                                 (*messageList));

    RWRETURN(message);
}

Rt2dMaestro *
_rt2dMaestroMessageListPostByIndex(Rt2dMaestro *maestro, RwInt32 index)
{
    RwInt32                     messageIdx, *beginMessageList, num;
    Rt2dMessage                 *message, *beginMessage;

    RWFUNCTION(RWSTRING("_rt2dMaestroMessageListPostByIndex"));

    RWASSERT(maestro);

    beginMessageList = (RwInt32 *)_rwSListGetArray(maestro->messageList);
    messageIdx = *(beginMessageList + index);

    beginMessage =
        _rt2dMessageStoreGetMessageByIndex(maestro->messageStore, messageIdx);
    message = beginMessage;
    messageIdx++;

    /* Count how many messages there are. */
    num = 0;
    while (message->messageType != rt2dMESSAGETYPENULL)
    {
        num++;

        message =
            _rt2dMessageStoreGetMessageByIndex(maestro->messageStore, messageIdx);

        messageIdx++;
    }

    /* Add the message to the queue. */
    if (num > 0)
    {
        if (_rt2dMessageQueueAddMessages(maestro, maestro->messageQueue,
                beginMessage, num, maestro->currAnimIndex) == NULL)
        {
            RWRETURN((Rt2dMaestro *)NULL);
        }
    }

    RWRETURN(maestro);
}

RwInt32
_rt2dMaestroMessageStoreStreamGetSize(Rt2dMaestro *maestro)
{
    RwInt32             i, size, num;
    Rt2dMessage         *message;

    RWFUNCTION(RWSTRING("_rt2dMaestroMessageStoreStreamGetSize"));

    RWASSERT(maestro);

    /* Message count. */
    size = sizeof(RwInt32);

    if (maestro->messageStore)
    {
        num = _rwSListGetNumEntries(maestro->messageStore);

        if (num > 0)
        {
            message = (Rt2dMessage *)_rwSListGetArray(maestro->messageStore);

            RWASSERT(message);

            for (i = 0; i < num; i++)
            {
                size += _rt2dMessageStreamGetSize(maestro, message);
            }
        }
    }

    RWRETURN(size);
}

Rt2dMaestro *
_rt2dMaestroMessageStoreStreamRead(Rt2dMaestro *maestro, RwStream *stream)
{
    RwInt32             i, num;
    Rt2dMessage         *message;

    RWFUNCTION(RWSTRING("_rt2dMaestroMessageStoreStreamRead"));

    RWASSERT(stream);

    /* Read the num of messages. */
    if (RwStreamReadInt32(stream, &num, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dMaestro *)NULL);
    }

    if (num > 0)
    {
        if (maestro->messageStore == NULL)
        {
            if ((_rt2dMaestroCreateMessageList(maestro)) == NULL)
            {
                RWRETURN((Rt2dMaestro *)NULL);
            }
        }

        if ((message =
             (Rt2dMessage *)_rwSListGetNewEntries(
                                    maestro->messageStore, num,
                                    rwID_2DANIM |
                                    rwMEMHINTDUR_EVENT)) == NULL)
        {
            RWRETURN((Rt2dMaestro *)NULL);
        }

        for (i = 0; i < num; i++)
        {
            _rt2dMessageInit(message);

            if (_rt2dMessageStreamRead(maestro, message, stream) == NULL)
            {
                RWRETURN((Rt2dMaestro *)NULL);
            }

            message++;
        }
    }

    RWRETURN(maestro);
}

Rt2dMaestro *
_rt2dMaestroMessageStoreStreamWrite(Rt2dMaestro *maestro, RwStream *stream)
{
    RwInt32             i, num;
    Rt2dMessage         *message;

    RWFUNCTION(RWSTRING("_rt2dMaestroMessageStoreStreamWrite"));

    RWASSERT(maestro);
    RWASSERT(stream);

    /* Write out the messageStore. */
    if (maestro->messageStore)
    {
        /* Write out the number of message list. */
        num = _rwSListGetNumEntries(maestro->messageStore);

        if (RwStreamWriteInt32(stream, &num, sizeof(RwInt32)) == NULL)
        {
            RWRETURN((Rt2dMaestro *)NULL);
        }

        if (num > 0)
        {
            message = (Rt2dMessage *)_rwSListGetArray(maestro->messageStore);

            RWASSERT(message);

            for (i = 0; i < num; i++)
            {
                if (_rt2dMessageStreamWrite(maestro, message, stream) == NULL)
                {
                    RWRETURN((Rt2dMaestro *)NULL);
                }

                message++;
            }
        }
    }
    else
    {
        /* Empty message store. */
        num = 0;
        if (RwStreamWriteInt32(stream, &num, sizeof(RwInt32)) == NULL)
        {
            RWRETURN((Rt2dMaestro *)NULL);
        }
    }

    RWRETURN(maestro);
}

/**************************************************************************/

/**
 * \ingroup rt2dmessage
 * \ref Rt2dMaestroAddMessageList is used to add a list of messages to the
 * maestro. The maestro maintains a central storage repository for all the
 * internal messages used in an animation. The messages are copied into the
 * repository and an index for retrieving the messages is returned.
 *
 * \param maestro      Pointer to the parent maestro.
 * \param message       Pointer to the list of messages.
 * \param num           Number of messages in the list.
 * \param index         Pointer to a RwInt32 to return the index.
 *
 * \return Returns a pointer to the maestro if successful, NULL otherwise.
 *
 * \see Rt2dMaestroProcessMessages
 * \see Rt2dMaestroPostMessages
 */
Rt2dMaestro *
Rt2dMaestroAddMessageList(Rt2dMaestro *maestro,
                           Rt2dMessage *message, RwInt32 num, RwInt32 *index)
{
    RwInt32             newMessageIdx, *newMessageList;
    Rt2dMessage         nullMessage;

    RWAPIFUNCTION(RWSTRING("Rt2dMaestroAddMessageList"));

    RWASSERT(maestro);
    RWASSERT(num >= 0);

    RWASSERT(maestro->messageStore);
    RWASSERT(maestro->messageList);

    /* Add the messages into the message storage area. */
    if (_rt2dMessageStoreAddMessages(maestro->messageStore,
        message, num, &newMessageIdx) == NULL)
    {
        RWRETURN((Rt2dMaestro *)NULL);
    }

    /* Add NULL message to mark the end. */
    nullMessage.messageType = rt2dMESSAGETYPENULL;
    nullMessage.index = -1;
    nullMessage.intParam1 = -1;
    nullMessage.intParam2 = -1;

    if (_rt2dMessageStoreAddMessages(maestro->messageStore,
        &nullMessage, 1, (RwInt32 *)NULL) == NULL)
    {
        RWRETURN((Rt2dMaestro *)NULL);
    }

    if (index)
    {
        *index = _rwSListGetNumEntries(maestro->messageList);
    }

    /* Add the message storage index into the message list. */
    if ((newMessageList = (RwInt32 *)_rwSListGetNewEntry(
                                        maestro->messageList,
                                        rwID_2DANIM |
                                        rwMEMHINTDUR_EVENT))
                                  == NULL)
    {
        RWRETURN((Rt2dMaestro *)NULL);
    }

    *newMessageList = newMessageIdx;

    RWRETURN(maestro);
}

/**
 * \ingroup rt2dmessage
 * \ref Rt2dMaestroPostMessages is used to add a list of messages to the
 * maestro's message queue. The maestro maintains a
 * list of messages that are to be processed in the next \ref
 * Rt2dMaestroProcessMessages. This message queue can be filled internally,
 * by other messages, or externally by the user.
 *
 * Messages in the queue are processed in sequence they are added. It is possible
 * for the list to grow during message processing. Messages can add new messages
 * for further processing. These new messages will be added to the end of the
 * queue and processed in turn rather than immediately.
 *
 * \param maestro      Pointer to parent maestro.
 * \param message       Pointer to the list of messages.
 * \param num           Number of the messages in the list.
 *
 * \return Returns a pointer to the maestro if successful, NULL otherwise.
 *
 * \see Rt2dMaestroProcessMessages
 * \see Rt2dMaestroAddMessageList
 * \see Rt2dMessageHandlerDefaultCallBack
 */
Rt2dMaestro *
Rt2dMaestroPostMessages(Rt2dMaestro *maestro, Rt2dMessage *message, RwInt32 num)
{
    RWAPIFUNCTION(RWSTRING("Rt2dMaestroPostMessages"));

    RWASSERT(maestro);
    RWASSERT(message);
    RWASSERT(num > 0);

    if (_rt2dMessageQueueAddMessages(maestro, maestro->messageQueue,
        message, num, maestro->currAnimIndex) == NULL)
    {
        RWRETURN((Rt2dMaestro *)NULL);
    }

    RWRETURN(maestro);
}

/**
 * \ingroup rt2dmessage
 * \ref Rt2dMaestroProcessMessages is used to begin processing of the messages
 * in the maestro's message queue. Once started, message processing will continue
 * until all messages in the queue are processed. The message queue can grow during processing
 * because messages can add additional messages to the queue.
 *
 * \param maestro          Pointer to the parent maestro.
 *
 * \return Returns a pointer to the maestro if successful, NULL otherwise.
 *
 * \see Rt2dMaestroAddMessageList
 * \see Rt2dMaestroPostMessages
 * \see Rt2dMessageHandlerDefaultCallBack
 */
Rt2dMaestro *
Rt2dMaestroProcessMessages(Rt2dMaestro *maestro)
{
    Rt2dMessage             *message;
    Rt2dMessageQueue        *messageQueue;
    RwCamera                *currCamera;
    RwRaster                *camRaster;
    RwInt32                 winWidth, winHeight;

    RWAPIFUNCTION(RWSTRING("Rt2dMaestroProcessMessages"));

    RWASSERT(maestro);

    /* Flag the queue as active. This prevents multiple calls to
     * Rt2dMaestroProcessMessages from processing the same queue,
     * which will lead to unexpected behaviour.
     */
    if (!(maestro->flag & RT2D_MAESTRO_FLAG_MSGQUEUE_ACTIVE))
    {
        /* Flag the queue as active */
        maestro->flag |= RT2D_MAESTRO_FLAG_MSGQUEUE_ACTIVE;

        /*
        * Update with camera's view parameter. Do it here rather than each time
        * in the message handlers.
        */
        currCamera = Rt2dDeviceGetCamera();

        if (currCamera)
        {
            const RwV2d *vw = (RwV2d *)NULL;

            camRaster = RwCameraGetRaster(currCamera);

            winWidth = RwRasterGetWidth(camRaster);
            winHeight = RwRasterGetHeight(camRaster);

            maestro->invWinW = 1.0f / (RwReal) winWidth;
            maestro->invWinH = 1.0f / (RwReal) winHeight;

            vw = RwCameraGetViewWindow(currCamera);
            maestro->winAspectRatio = (vw->x) / (vw->y);

        }
        else
        {
            maestro->invWinW = 0;
            maestro->invWinH = 0;
            maestro->winAspectRatio = 0;
        }

        /* Process the message queue. */
        RWASSERT(maestro->messageQueue);
        messageQueue = maestro->messageQueue;
        message = _rt2dMessageQueueGetMessage(messageQueue);

        while (message != NULL)
        {
            if ((*maestro->messageHandler)(maestro, message) != message)
            {
                /* Do we empty the queue on error ? */

                maestro->flag &= ~(RT2D_MAESTRO_FLAG_MSGQUEUE_ACTIVE);

                RWRETURN( (Rt2dMaestro *)NULL);
            }

            message = _rt2dMessageQueueGetMessage(messageQueue);
        }

        /* Unset the active flag */
        maestro->flag &= ~(RT2D_MAESTRO_FLAG_MSGQUEUE_ACTIVE);
    }
    else
    {
        /* Should we assert here ? */
    }

    RWRETURN(maestro);
}

/****************************************************************************
 *
 *  Message queue functions.
 *
 ****************************************************************************/

Rt2dMessage *
_rt2dMessageSpecialMessageTellTarget(Rt2dMaestro *maestro, Rt2dMessage *message,
                                     RwInt32 currAnim, void *pData)
{
    RwInt32                 i, animTarget;
    Rt2dAnimNode            *animNode;

    RWFUNCTION(RWSTRING("_rt2dMessageSpecialMessageTellTarget"));

    animTarget = currAnim;

    if (message->index>=0)  /* Hack to force lookup from string labels */
    {
        if (message->intParam1 > 0)
        {
            Rt2dStringLabel * label;
            void * internalData;

            i = message->intParam1 - 1;

            /* Current animNode. */
            animNode = _rt2dMaestroGetAnimNodeByIndex(maestro, currAnim);

            while (i > 0)
            {
                /* Get the parent. */
                animNode = _rt2dMaestroGetAnimNodeByIndex(maestro, animNode->parent);

                i--;
            }

            label = Rt2dMaestroGetStringLabelByIndex(maestro,
                                                     message->intParam2);
            internalData =  Rt2dStringLabelGetInternalData(label);


            animTarget = animNode->parent + (RwInt32)internalData;
        }
        else if (message->intParam1 == 0)
        {
            Rt2dStringLabel * label =
                Rt2dMaestroGetStringLabelByIndex(maestro,
                                                 message->intParam2);
            const void * internalData =
                Rt2dStringLabelGetInternalData(label);

            animTarget = currAnim + (RwInt32)internalData;
        }
        else if (message->intParam1 == -1)
        {
            Rt2dStringLabel * label =
                Rt2dMaestroGetStringLabelByIndex(maestro,
                                                 message->intParam2);
            const void * internalData =
                Rt2dStringLabelGetInternalData(label);

            animTarget = (RwInt32)internalData;

#if defined(RWDEBUG)
            /* !!! Replace with RWASSERTM !!! */
            if (animTarget<0)
            {
                RWMESSAGE((RWSTRING("Bad tell target address")));
            }
#endif
            RWASSERT(animTarget>=0);
        }
        else
        {
            /* Illegal param value for this message. */
            RWASSERT(FALSE);
        }
    }
    else   /* Not looking up from string labels */
    {
        if (message->intParam1 > 0)
        {
            i = message->intParam1 - 1;

            /* Current animNode. */
            animNode = _rt2dMaestroGetAnimNodeByIndex(maestro, currAnim);

            while (i > 0)
            {
                /* Get the parent. */
                animNode = _rt2dMaestroGetAnimNodeByIndex(maestro, animNode->parent);

                i--;
            }

            animTarget = animNode->parent + message->intParam2;
        }
        else if (message->intParam1 == 0)
        {
            animTarget = currAnim + message->intParam2;
        }
        else if (message->intParam1 == -1)
        {
            animTarget = message->intParam2;
        }
        else
        {
            /* Illegal param value for this message. */
            RWASSERT(FALSE);
        }
    }

    *((RwInt32 *) pData) = animTarget;

    RWRETURN(message);
}

Rt2dMessageQueue *
_rt2dMessageQueueInit(Rt2dMessageQueue *messageQueue)
{
    RWFUNCTION(RWSTRING("_rt2dMessageQueueInit"));

    RWASSERT(messageQueue);

    messageQueue->messageQueue = (Rt2dMessage *)
                                     RwMalloc(
                                         sizeof(Rt2dMessage)
                                             * _rwDefaultMessageQueueSize,
                                         rwID_2DANIM | rwMEMHINTDUR_EVENT);
    if (!messageQueue->messageQueue)
    {
        RWRETURN( (Rt2dMessageQueue *)NULL );
    }

    messageQueue->messageTop = 0;
    messageQueue->messageBot = 0;
    messageQueue->size = _rwDefaultMessageQueueSize;

    RWRETURN(messageQueue);
}

Rt2dMessageQueue *
_rt2dMessageQueueDestruct(Rt2dMessageQueue *messageQueue)
{
    RWFUNCTION(RWSTRING("_rt2dMessageQueueDestruct"));

    RWASSERT(messageQueue);

    messageQueue->messageTop = 0;
    messageQueue->messageBot = 0;

    if (messageQueue->messageQueue)
    {
        RwFree(messageQueue->messageQueue);
    }

    RWRETURN(messageQueue);
}

Rt2dMessageQueue *
_rt2dMessageQueueCreate(void)
{
    Rt2dMessageQueue        *messageQueue;

    RWFUNCTION(RWSTRING("_rt2dMessageQueueCreate"));

    if ((messageQueue = (Rt2dMessageQueue *)
                            RwFreeListAlloc(&Rt2dAnimGlobals.messageQueueFreeList,
                                            rwID_2DANIM |
                                            rwMEMHINTDUR_EVENT)) == NULL)
    {
        RWRETURN((Rt2dMessageQueue *)NULL);
    }

    if (_rt2dMessageQueueInit(messageQueue) == NULL)
    {
        RWRETURN((Rt2dMessageQueue *)NULL);
    }

    RWRETURN(messageQueue);
}

RwBool
_rt2dMessageQueueDestroy(Rt2dMessageQueue *messageQueue)
{
    RWFUNCTION(RWSTRING("_rt2dMessageQueueDestroy"));

    RWASSERT(messageQueue);

    _rt2dMessageQueueDestruct(messageQueue);

    RwFreeListFree(&Rt2dAnimGlobals.messageQueueFreeList, messageQueue);

    RWRETURN(TRUE);
}

Rt2dMessageQueue *
_rt2dMessageQueueAddMessages(Rt2dMaestro *maestro,
                             Rt2dMessageQueue *messageQueue, Rt2dMessage *message,
                             RwInt32 num, RwInt32 currAnim)
{
    RwInt32                 i, messageQueueIdx, animTarget;
    Rt2dMessage             *queueMessage;
    Rt2dStringLabel         *strLabel;

    RWFUNCTION(RWSTRING("_rt2dMessageQueueAddMessages"));

    RWASSERT(messageQueue);

    messageQueueIdx = messageQueue->messageBot;

    /* Check the message queue is not full. */
    if ((RwUInt32)(messageQueueIdx + num) > messageQueue->size)
    {
        RWMESSAGE((RWSTRING("Rt2dMaestro message queue full at size %d. ")
                   RWSTRING("Use Rt2dMessageQueueSetDefaultSize."),
                            messageQueue->size));
        RWRETURN((Rt2dMessageQueue *)NULL);
    }

    /* Add the message to the bottom. */
    queueMessage = &messageQueue->messageQueue[messageQueueIdx];

    animTarget = currAnim;

    for (i = 0; i < num; i++)
    {
        /* Check if the message is a special, if so then process it. */
        switch (message->messageType)
        {
            case rt2dMESSAGETYPESPECIALTELLTARGET :
            {
                if (_rt2dMessageSpecialMessageTellTarget(maestro,
                        message, currAnim, (void *) (&animTarget)) == NULL)
                {
                    RWRETURN((Rt2dMessageQueue *)NULL);
                }
                break;
            }

            default :
            {
                /* Normal message, copy into the message queue. */
                *queueMessage = *message;
                /* memcpy(queueMessage, message, sizeof(Rt2dMessage));  */
                messageQueue->messageBot++;

                if (queueMessage->index < 0)
                {
                    queueMessage->index = animTarget;
                }
                else
                {
                    strLabel = Rt2dMaestroGetStringLabelByIndex(maestro, queueMessage->index);

                    queueMessage->index = (RwInt32) strLabel->internalData;
                }

                queueMessage++;

                break;
            }
        }

        message++;
    }

    RWRETURN(messageQueue);
}

Rt2dMessage *
_rt2dMessageQueueGetMessage(Rt2dMessageQueue *messageQueue)
{
    Rt2dMessage             *message;

    RWFUNCTION(RWSTRING("_rt2dMessageQueueGetMessage"));

    RWASSERT(messageQueue);

    /* Check the queue is not empty. */
    if (messageQueue->messageBot == 0)
    {
        RWRETURN( (Rt2dMessage *)NULL);
    }

    /* messageTop is less than messageBot for a non-empty queue. */
    RWASSERT(messageQueue->messageTop < messageQueue->messageBot);

    /* Take a message from the top of the queue. */
    message = &messageQueue->messageQueue[messageQueue->messageTop];
    messageQueue->messageTop++;

    /* Check and update the queue if it is empty. */
    if (messageQueue->messageTop >= messageQueue->messageBot)
    {
        messageQueue->messageTop = 0;
        messageQueue->messageBot = 0;
    }

    RWRETURN(message);
}

/**************************************************************************
 *
 * Maestro's message queue functions.
 *
 **************************************************************************/

Rt2dMaestro *
_rt2dMaestroCreateMessageQueue(Rt2dMaestro *maestro)
{
    RWFUNCTION(RWSTRING("_rt2dMaestroCreateMessageQueue"));

    RWASSERT(maestro);

    if ((maestro->messageQueue = _rt2dMessageQueueCreate()) == NULL)
    {
        RWRETURN((Rt2dMaestro *)NULL);
    }

    RWRETURN(maestro);
}

Rt2dMaestro *
_rt2dMaestroDestroyMessageQueue(Rt2dMaestro *maestro)
{
    RWFUNCTION(RWSTRING("_rt2dMaestroDestroyMessageQueue"));

    RWASSERT(maestro);

    if (maestro->messageQueue)
    {
        _rt2dMessageQueueDestroy(maestro->messageQueue);
    }

    maestro->messageQueue = (Rt2dMessageQueue *)NULL;

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
