
/***************************************************************************
 *                                                                         *
 * Module  : button.h                                                      *
 *                                                                         *
 * Purpose :                                                               *
 *                                                                         *
 **************************************************************************/

#ifndef BUTTON_H
#define BUTTON_H

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

#define RT2D_BUTTON_STATE_FLAG_OVER             0x01
#define RT2D_BUTTON_STATE_FLAG_LEFTBUTTON       0x02
#define RT2D_BUTTON_STATE_FLAG_RIGHTBUTTON      0X04

enum Rt2dButtonObject
{
    rt2dANIMBUTTONOBJECTPICKREGION = 0,
    rt2dANIMBUTTONOBJECTUP,
    rt2dANIMBUTTONOBJECTDOWN,
    rt2dANIMBUTTONOBJECTOVER,

    rt2dANIMBUTTONOBJECTFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};

typedef enum Rt2dButtonObject Rt2dButtonObject;

struct Rt2dStateTransData
{
    RwInt32                     transState;                 /* Button state transition action list index */

    RwInt32                     buttonUpImg;                /* Button state image visibility flag. */
    RwInt32                     buttonDownImg;
    RwInt32                     buttonOverImg;
};

typedef struct Rt2dStateTransData Rt2dStateTransData;

/****************************************************************************
 Global Types
 */


struct Rt2dButton
{
    RwInt32                 strLabelIndex;              /* Index to name stored in string label. */
    RwInt32                 objectIndex;                /* Index within scene. */

    RwUInt32                stateFlag;
    RwUInt32                buttonState;
    RwInt32                 actionListForStateTransition[RT2D_BUTTON_MAX_STATE_TRANSITIONS];
};

/****************************************************************************
 Function prototypes
 */

extern Rt2dButton *
_rt2dButtonInit(Rt2dButton *button);

extern Rt2dButton *
_rt2dButtonDestruct(Rt2dButton *button);

extern Rt2dButton *
_rt2dButtonCreate(void);

extern RwBool
_rt2dButtonDestroy(Rt2dButton *button);

extern RwUInt32
_rt2dButtonStreamGetSize(Rt2dMaestro *maestro, Rt2dButton *button,
                         RwInt32 stateCount, RwInt32 transCount);

extern Rt2dButton *
_rt2dButtonStreamWrite(Rt2dMaestro *maestro, Rt2dButton *button, RwStream *stream,
                       RwInt32 stateCount, RwInt32 transCount);

extern Rt2dButton *
_rt2dButtonStreamRead(Rt2dMaestro *maestro, Rt2dButton *button, RwStream *stream,
                      RwInt32 stateCount, RwInt32 transCount);

/*
 * Maestro's buttons functions.
 */

extern Rt2dMaestro *
_rt2dMaestroCreateButtons(Rt2dMaestro *maestro);

extern Rt2dMaestro *
_rt2dMaestroDestroyButtons(Rt2dMaestro *maestro);

extern RwUInt32
_rt2dMaestroButtonsStreamGetSize(Rt2dMaestro *maestro);

extern Rt2dMaestro *
_rt2dMaestroButtonsStreamRead(Rt2dMaestro *maestro, RwStream *stream);

extern Rt2dMaestro *
_rt2dMaestroButtonsStreamWrite(Rt2dMaestro *maestro, RwStream *stream);

extern Rt2dButton *
_rt2dMaestroGetButtonByIndex(Rt2dMaestro *maestro, RwInt32 index);

extern Rt2dButton *
_rt2dButtonReset(Rt2dButton *button, Rt2dObject *scene);

extern Rt2dStateTransData *
_rt2dStateTransTableInit(Rt2dStateTransData stateTransTable[2][2][2][2]);

#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#endif /* BUTTON_H */
