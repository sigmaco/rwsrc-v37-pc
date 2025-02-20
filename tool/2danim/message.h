
/***************************************************************************
 *                                                                         *
 * Module  : message.h                                                      *
 *                                                                         *
 * Purpose :                                                               *
 *                                                                         *
 **************************************************************************/

#ifndef MESSAGE_H
#define MESSAGE_H

/****************************************************************************
 Includes
 */
#include <rwcore.h>
#include "rt2danim.h"

#ifdef    __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/****************************************************************************
 Defines
 */

#define RT2D_MESSAGE_MAX_STATES                     64
#define RT2D_MESSAGE_MAX_STATE_TRANSITIONS          64

#define RT2D_MESSAGE_MAX_MESSAGES                   17

#define RT2D_MESSAGE_DEFAULT_MAX_QUEUE              64

/****************************************************************************
 Global Types
 */

struct Rt2dMessageList
{
    RwInt32                messageIdx;
    RwInt32                length;
};

struct Rt2dMessageDescrip
{
    Rt2dMessageStreamGetSizeCallBack    funcStreamGetSize;
    Rt2dMessageStreamReadCallBack       funcStreamRead;
    Rt2dMessageStreamWriteCallBack      funcStreamWrite;
};

typedef struct Rt2dMessageDescrip Rt2dMessageDescrip;

struct Rt2dMessageQueue
{
    Rt2dMessage         *messageQueue;
    RwInt32             messageTop;
    RwInt32             messageBot;
    RwUInt32            size;
};

typedef struct Rt2dMessageQueue Rt2dMessageQueue;

/****************************************************************************
 Global vars
 */
extern Rt2dMessageDescrip _rt2dMessageDescripTable[RT2D_MESSAGE_MAX_MESSAGES];

/****************************************************************************
 Function prototypes
 */

/* Message descrip table initialisation. */
extern Rt2dMessageDescrip *
_rt2dMessageDescripTableInit(Rt2dMessageDescrip *messageDescripTable);


extern Rt2dMessageStreamGetSizeCallBack
_rt2dMessageTypeGetStreamGetSizeCallBack(
    Rt2dMessageDescrip                  *messageDescripTable,
    Rt2dMessageType                     messageType
);

extern Rt2dMessageDescrip *
_rt2dMessageTypeSetStreamGetSizeCallBack(
    Rt2dMessageDescrip                  *messageDescripTable,
    Rt2dMessageType                     messageType,
    Rt2dMessageStreamGetSizeCallBack    funcStreamGetSize
);


extern Rt2dMessageStreamReadCallBack
_rt2dMessageTypeGetStreamReadCallBack(
    Rt2dMessageDescrip                  *messageDescripTable,
    Rt2dMessageType                     messageType
);

extern Rt2dMessageDescrip *
_rt2dMessageTypeSetStreamReadCallBack(
    Rt2dMessageDescrip                  *messageDescripTable,
    Rt2dMessageType                     messageType,
    Rt2dMessageStreamReadCallBack       funcStreamRead
);


extern Rt2dMessageStreamWriteCallBack
_rt2dMessageTypeGetStreamWriteCallBack(
    Rt2dMessageDescrip                  *messageDescripTable,
    Rt2dMessageType                     messageType
);

extern Rt2dMessageDescrip *
_rt2dMessageTypeSetStreamWriteCallBack(
    Rt2dMessageDescrip                  *messageDescripTable,
    Rt2dMessageType                     messageType,
    Rt2dMessageStreamWriteCallBack      funcStreamWrite
);

/* Message functions. */

extern Rt2dMessage *
_rt2dMessageInit(Rt2dMessage *message);

extern Rt2dMessage *
_rt2dMessageDestruct(Rt2dMessage *message);

extern Rt2dMessage *
_rt2dMessageCreate(void);

extern RwBool
_rt2dMessageDestroy(Rt2dMessage *message);

/* Special message functions. */

extern Rt2dMessage *
_rt2dMessageSpecialMessageTellTarget(
    Rt2dMaestro *maestro, Rt2dMessage *message, RwInt32 currAnim, void *pData);

/* Message streaming functions. */

extern RwInt32
_rt2dMessageStreamGetSize(
    Rt2dMaestro *maestro, Rt2dMessage *message);

extern Rt2dMessage *
_rt2dMessageStreamWrite(
    Rt2dMaestro *maestro, Rt2dMessage *message, RwStream *stream);

extern Rt2dMessage *
_rt2dMessageStreamRead(
    Rt2dMaestro *maestro, Rt2dMessage *message, RwStream *stream);


/* Default callbacks for the message descript table. */

extern RwInt32
_rt2dMessageStreamGetSizeDefaultCallBack(
    Rt2dMaestro *maestro, Rt2dMessage *message);

extern Rt2dMessage *
_rt2dMessageStreamWriteDefaultCallBack(
    Rt2dMaestro *maestro, Rt2dMessage *message, RwStream *stream);

extern Rt2dMessage *
_rt2dMessageStreamReadDefaultCallBack(
    Rt2dMaestro *maestro, Rt2dMessage *message, RwStream *stream);


/* Message store functions. */

extern RwSList *
_rt2dMessageStoreCreate(void);

extern RwBool
_rt2dMessageStoreDestroy(RwSList *messageStore);

extern RwSList *
_rt2dMessageStoreAddMessages(
    RwSList *messageStore, Rt2dMessage *message, RwInt32 length, RwInt32 *idx);

extern Rt2dMessage *
_rt2dMessageStoreGetMessageByIndex(RwSList *messageStore, RwInt32 idx);

/*
 * Maestro's message functions.
 */

extern Rt2dMaestro *
_rt2dMaestroCreateMessageList(Rt2dMaestro *maestro);

extern Rt2dMaestro *
_rt2dMaestroDestroyMessageList(Rt2dMaestro *maestro);


extern RwInt32
_rt2dMaestroMessageListStreamGetSize(Rt2dMaestro *maestro);

extern Rt2dMaestro *
_rt2dMaestroMessageListStreamRead(Rt2dMaestro *maestro, RwStream *stream);

extern Rt2dMaestro *
_rt2dMaestroMessageListStreamWrite(Rt2dMaestro *maestro, RwStream *stream);


extern Rt2dMessage *
_rt2dMaestroMessageListGetByIndex(Rt2dMaestro *maestro, RwInt32 index);

extern Rt2dMaestro *
_rt2dMaestroMessageListPostByIndex(Rt2dMaestro *maestro, RwInt32 index);


extern RwInt32
_rt2dMaestroMessageStoreStreamGetSize(Rt2dMaestro *maestro);

extern Rt2dMaestro *
_rt2dMaestroMessageStoreStreamRead(Rt2dMaestro *maestro, RwStream *stream);

extern Rt2dMaestro *
_rt2dMaestroMessageStoreStreamWrite(Rt2dMaestro *maestro, RwStream *stream);

/*
 *  Message queue functions.
 */

extern Rt2dMessageQueue *
_rt2dMessageQueueInit(Rt2dMessageQueue *messageQueue);

extern Rt2dMessageQueue *
_rt2dMessageQueueDestruct(Rt2dMessageQueue *messageQueue);

extern Rt2dMessageQueue *
_rt2dMessageQueueCreate(void);

extern RwBool
_rt2dMessageQueueDestroy(Rt2dMessageQueue *messageQueue);

extern Rt2dMessageQueue *
_rt2dMessageQueueAddMessages(
    Rt2dMaestro *maestro,
    Rt2dMessageQueue *messageQueue,
    Rt2dMessage *message, RwInt32 num, RwInt32 currAnim);

extern Rt2dMessage *
_rt2dMessageQueueGetMessage(Rt2dMessageQueue *messageStore);

/*
 *  Maestro's queue functions.
 */

extern Rt2dMaestro *
_rt2dMaestroCreateMessageQueue(Rt2dMaestro *maestro);

extern Rt2dMaestro *
_rt2dMaestroDestroyMessageQueue(Rt2dMaestro *maestro);

#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#endif /* MESSAGE_H */
