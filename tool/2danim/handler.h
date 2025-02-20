/***************************************************************************
 *                                                                         *
 * Module  : handler.h                                                     *
 *                                                                         *
 * Purpose :                                                               *
 *                                                                         *
 **************************************************************************/

#ifndef HANDLER_H
#define HANDLER_H

/****************************************************************************
 Includes
 */
#include <rwcore.h>
#include "rt2danim.h"

#include "message.h"
#include "handler.h"

#ifdef    __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/****************************************************************************
 Defines
 */

/****************************************************************************
 Function prototypes
 */
extern void
_rt2dApplyMouseState(Rt2dMaestro *maestro, RwV2d *point);

#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#endif /* HANDLER_H */

