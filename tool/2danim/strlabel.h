
/***************************************************************************
 *                                                                         *
 * Module  : strlabel.h                                                    *
 *                                                                         *
 * Purpose :                                                               *
 *                                                                         *
 **************************************************************************/

#ifndef STRLABEL_H
#define STRLABEL_H

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

/****************************************************************************
 Global Types
 */


/****************************************************************************
 Function prototypes
 */

extern Rt2dStringLabel *
_rt2dStringLabelInit(Rt2dStringLabel *strLabel);

extern RwBool
_rt2dStringLabelDestruct(Rt2dStringLabel *strLabel);



extern Rt2dStringLabel *
_rt2dStringLabelCreate( void );

extern RwBool
_rt2dStringLabelDestroy(Rt2dStringLabel *strLabel);



extern RwUInt32
_rt2dStringLabelStreamGetSize(Rt2dMaestro *maestro,
                                  Rt2dStringLabel *strLabel);

extern Rt2dStringLabel *
_rt2dStringLabelStreamRead(Rt2dMaestro *maestro,
                               Rt2dStringLabel *strLabel, RwStream *stream);

extern Rt2dStringLabel *
_rt2dStringLabelStreamWrite(Rt2dMaestro *maestro,
                                Rt2dStringLabel *strLabel, RwStream *stream);

/*
 * Maestro string label functions.
 */

extern Rt2dMaestro *
_rt2dMaestroCreateStringLabels(Rt2dMaestro *maestro);

extern Rt2dMaestro *
_rt2dMaestroDestroyStringLabels(Rt2dMaestro *maestro);

extern RwUInt32
_rt2dMaestroStringLabelsStreamGetSize(Rt2dMaestro *maestro);

extern Rt2dMaestro *
_rt2dMaestroStringLabelsStreamRead(Rt2dMaestro *maestro, RwStream *stream);

extern Rt2dMaestro *
_rt2dMaestroStringLabelsStreamWrite(Rt2dMaestro *maestro, RwStream *stream);

extern Rt2dStringLabel *
_rt2dMaestroFindStringLabel(Rt2dMaestro *maestro, RwChar *lookupName, RwInt32 *index);


#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#endif /* STRLABEL_H */
