
/***************************************************************************
 *                                                                         *
 * Module  : maestro.h                                                    *
 *                                                                         *
 * Purpose :                                                               *
 *                                                                         *
 **************************************************************************/

#ifndef MAESTRO_H
#define MAESTRO_H

/****************************************************************************
 Includes
 */
#include <rwcore.h>
#include "rt2danim.h"

#include "anim.h"
#include "props.h"
#include "cel.h"

#include "strlabel.h"
#include "message.h"
#include "button.h"


#ifdef    __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/****************************************************************************
 Defines
 */

/* Maestro flags */
#define RT2D_MAESTRO_FLAG_LOCK                 0x01
#define RT2D_MAESTRO_FLAG_MSGQUEUE_ACTIVE      0x02

/* Anim node flags */
#define RT2D_ANIMNODE_FLAG_STOP                 0x01
#define RT2D_ANIMNODE_FLAG_DO_CEL_ACTIONS       0x02
#define RT2D_ANIMNODE_FLAG_VISIBLE              0x04

/****************************************************************************
 Global Types
 */

struct Rt2dMaestroMouseState
{
    RwV2d                   position;
    RwInt32                 button;
};

typedef struct Rt2dMaestroMouseState Rt2dMaestroMouseState;

typedef struct Rt2dAnimFlashScript Rt2dAnimFlashScript;

/* Message structs as per attached design */

struct Rt2dMaestro
{
    RwInt32                 flag;               /* Status flag */
    RwInt32                 version;            /* Version stamp */

    RwSList                 *animations;        /* SList of Rt2dAnimNode */
    RwSList                 *labels;            /* SList of string labelled entities */
    RwSList                 *buttons;           /* SList of Rt2dButtons */
    RwSList                 *messageStore;      /* SList of RtMessages */
    RwSList                 *messageList;       /* SList of message list */
    RwSList                 *byteBucket;        /* General storage area */
    Rt2dMessageQueue        *messageQueue;      /* Queue of message list to be processed */
    Rt2dObject              *scene;             /* Scene containing all objects */

    Rt2dMessageHandlerCallBack  messageHandler;     /* Message handler callback. */

    Rt2dMaestroMouseState  prevMouse;              /* Prev mouse state */

    RwReal                  invWinH;                /* Camera's view params */
    RwReal                  invWinW;
    RwReal                  winAspectRatio;

    RwInt32                 currAnimIndex;          /* Current anim index */

    Rt2dBBox                bbox;

    Rt2dStateTransData      stateTransTable[2][2][2][2];       /* State transition look up table */
};

struct Rt2dAnimNode
{
    RwInt32                 flag;
    Rt2dAnim                anim;
    Rt2dAnimProps           props;
    Rt2dCelList             *celList;

    RwInt32                 parent;
    RwInt32                 posInParentScene;
};

typedef struct Rt2dAnimNode Rt2dAnimNode;

/****************************************************************************
 Function prototypes
 */

typedef Rt2dMaestro *(*Rt2dMaestroAnimNodeCallBack)
        (Rt2dMaestro *maestro, Rt2dAnimNode *animNode, void * pData);


/****************************************************************************
 Function prototypes
 */
extern Rt2dMaestro *
_rt2dMaestroInit(Rt2dMaestro *maestro);

extern RwBool
_rt2dMaestroDestruct(Rt2dMaestro *maestro);

/*
 *
 */

extern Rt2dAnimNode *
_rt2dMaestroGetAnimNodeByIndex(Rt2dMaestro *maestro, RwInt32 index);

extern Rt2dAnimNode *
_rt2dAnimNodeInit(Rt2dAnimNode * animNode);

extern Rt2dAnimNode *
_rt2dAnimNodeDestruct(Rt2dAnimNode * animNode);

/*
 * Maestro's animations functions.
 */

extern Rt2dMaestro *
_rt2dMaestroCreateAnimations(Rt2dMaestro *maestro);

extern Rt2dMaestro *
_rt2dMaestroDestroyAnimations(Rt2dMaestro *maestro);

extern RwInt32
_rt2dMaestroAnimationsStreamGetSize(Rt2dMaestro *maestro);

extern Rt2dMaestro *
_rt2dMaestroAnimationsStreamRead(Rt2dMaestro *maestro, RwStream *stream);

extern Rt2dMaestro *
_rt2dMaestroAnimationsStreamWrite(Rt2dMaestro *maestro, RwStream *stream);

extern Rt2dMaestro *
_rt2dMaestroForAllAnimNode(Rt2dMaestro *maestro,
                            Rt2dMaestroAnimNodeCallBack callback, void *data);

extern Rt2dMaestro *
_rt2dMaestroForAllVisibleAnimNode(Rt2dMaestro *maestro,
                            Rt2dMaestroAnimNodeCallBack callback, void *data);

/*
 * Maestro's scene  functions.
 */

extern RwInt32
_rt2dMaestroSceneStreamGetSize(Rt2dMaestro *maestro);

extern Rt2dMaestro *
_rt2dMaestroSceneStreamRead(Rt2dMaestro *maestro, RwStream *stream);

extern Rt2dMaestro *
_rt2dMaestroSceneStreamWrite(Rt2dMaestro *maestro, RwStream *stream);

#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#endif /* MAESTRO_H */
