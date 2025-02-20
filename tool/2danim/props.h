
/***************************************************************************
 *                                                                         *
 * Module  : props.h                                                       *
 *                                                                         *
 * Purpose :                                                               *
 *                                                                         *
 **************************************************************************/

#ifndef PROPS_H
#define PROPS_H

/****************************************************************************
 Includes
 */
#include <rwcore.h>
#include "rt2danim.h"

/****************************************************************************
 Defines
 */

/****************************************************************************
 Global Types
 */

struct Rt2dAnimLabelNode
{
    Rt2dObject *scene;
    RwChar     *label;
};


/*
 * Structure describing the current state of a scene.
 */
struct Rt2dAnimProps
{
	Rt2dObject *scene;
    RwSList *directory;
    RwSList *labels;
    RwSList *sceneCurrentState;
};

/****************************************************************************
 Function prototypes
 */

extern Rt2dAnimProps *
_rt2dAnimPropsInit(Rt2dAnimProps *props, Rt2dObject *scene);

extern Rt2dAnimProps *
_rt2dAnimPropsDestruct(Rt2dAnimProps *props);

extern Rt2dAnimProps *
_rt2dAnimPropsStreamRead(RwStream *stream);

extern RwUInt32
_rt2dAnimPropsStreamGetSize(Rt2dAnimProps *props);

extern Rt2dAnimProps *
_rt2dAnimPropsStreamWrite(Rt2dAnimProps *props, RwStream *stream);

extern Rt2dObject *
_rt2dAnimPropsGet2dObjectByIndex(Rt2dAnimProps *props, RwInt32 index);

extern RwUInt32
_rt2dAnimPropsGetIndexBy2dObject(Rt2dAnimProps *props, Rt2dObject *object);

#endif /* OBJECT_H */
