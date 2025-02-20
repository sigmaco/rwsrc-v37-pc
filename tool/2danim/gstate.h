
/***************************************************************************
 *                                                                         *
 * Module  : gstate.h                                                      *
 *                                                                         *
 * Purpose :                                                            *
 *                                                                         *
 **************************************************************************/

#ifndef GSTATE_H
#define GSTATE_H

/****************************************************************************
 Includes

*/
#include <rwcore.h>

/****************************************************************************
 Defines
 */
/* see section 4.3 of the EE User's Manual (2nd ed.) for these */

/*!!! #define RT2DPS2MAXVERT      64 */


/****************************************************************************
 Global Types
 */

typedef struct Rt2dAnimGlobalVars Rt2dAnimGlobalVars;
struct Rt2dAnimGlobalVars
{
    RwFreeList         animFreeList;
    RwFreeList         buttonFreeList;
    RwFreeList         maestroFreeList;
    RwFreeList         stringLabelFreeList;
    RwFreeList         messageFreeList;
    RwFreeList         messageQueueFreeList;
    RwFreeList         animObjectUpdateFreeList;
    RwFreeList         keyFrameListFreeList;
    RwFreeList         animPropsFreeList;
    RwFreeList         celListFreeList;
    RwFreeList         celFreeList;
};

extern Rt2dAnimGlobalVars Rt2dAnimGlobals;
extern RwBool _rt2dAnimOpenCalled;

 /****************************************************************************
 Function prototypes
 */

/* init */
extern RwBool       _rt2danimGstateOpen(void);
extern void         _rt2danimGstateClose(void);

#endif /* GSTATE_H */
