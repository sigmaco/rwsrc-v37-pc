/***************************************************************************
 *                                                                         *
 * Module  : cel.h                                                         *
 *                                                                         *
 * Purpose :                                                               *
 *                                                                         *
 **************************************************************************/

#ifndef CEL_H
#define CEL_H

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

/* Cel List's flag bit. */
#define rt2dCELLISTFLAGLOCK             0x01

/****************************************************************************
 Typedefs
 */
struct Rt2dCelList
{
    RwSList                 *cel;
    RwUInt32                flag;
};

/****************************************************************************
 Function prototypes
 */

extern Rt2dCel *
_rt2dCelInit(Rt2dCel *cel);

extern Rt2dCel *
_rt2dCelDestruct(Rt2dCel *cel);


extern Rt2dCel *
_rt2dCelCreate(void);

extern RwBool
_rt2dCelDestroy(Rt2dCel *cel);

extern Rt2dCel *
_rt2dCelCopy(Rt2dCel *dstCel, Rt2dCel *srcCel);


extern RwInt32
_rt2dCelStreamGetSize(Rt2dMaestro *maestro, Rt2dCel *cel);

extern Rt2dCel *
_rt2dCelStreamRead(Rt2dMaestro *maestro, Rt2dCel *cel, RwStream *stream);

extern Rt2dCel *
_rt2dCelStreamWrite(Rt2dMaestro *maestro, Rt2dCel *cel, RwStream *stream);


/****************************************************************************/


extern Rt2dCelList *
_rt2dCelListInit(Rt2dCelList *celList);

extern Rt2dCelList *
_rt2dCelListDestruct(Rt2dCelList *celList);

extern Rt2dCelList *
_rt2dCelListCopy(Rt2dCelList *dstCelList, Rt2dCelList *srcCelList);

extern RwInt32
_rt2dCelListStreamGetSize(Rt2dMaestro *maestro, Rt2dCelList *celList);

extern Rt2dCelList *
_rt2dCelListStreamRead(Rt2dMaestro *maestro, Rt2dCelList *celList, RwStream *stream);

extern Rt2dCelList *
_rt2dCelListStreamWrite(Rt2dMaestro *maestro, Rt2dCelList *celList, RwStream *stream);


extern Rt2dCel *
_rt2dCelListGetCelByIndex(Rt2dCelList *celList, RwInt32 index);


#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#endif /* CEL_H */

