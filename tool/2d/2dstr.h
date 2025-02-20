
/***************************************************************************
 *                                                                         *
 * Module  : 2dstr.h                                                       *
 *                                                                         *
 * Purpose :                                                               *
 *                                                                         *
 **************************************************************************/

#ifndef STRING2D_H
#define STRING2D_H

/****************************************************************************
 Function prototypes
 */
extern Rt2dObject *
_rt2dObjectStringInit(Rt2dObject *object, const RwChar *textString, const RwChar *font);

extern RwBool
_rt2dObjectStringDestruct(Rt2dObject *object);

extern Rt2dObject  *
_rt2dObjectStringStreamReadTo(Rt2dObject *object, RwStream *stream);

extern Rt2dObject *
_rt2dObjectStringStreamWrite(Rt2dObject *object, RwStream *stream);

extern RwUInt32
_rt2dObjectStringStreamGetSize(Rt2dObject *object);

#endif /* STRING2D_H */

