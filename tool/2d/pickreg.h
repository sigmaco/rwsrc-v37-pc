
/***************************************************************************
 *                                                                         *
 * Module  : pickreg.h                                                     *
 *                                                                         *
 * Purpose :                                                               *
 *                                                                         *
 **************************************************************************/
#ifndef PICKREGION_H
#define PICKREGION_H

/****************************************************************************
 Function prototypes
 */
extern Rt2dObject *
_rt2dPickRegionInit(Rt2dObject *pickRegion);

extern RwBool
_rt2dPickRegionDestruct(Rt2dObject *pickRegion);

extern Rt2dObject  *
_rt2dPickRegionStreamReadTo( Rt2dObject *object, RwStream *stream );

extern RwUInt32
_rt2dPickRegionStreamGetSize(Rt2dObject *object);

extern Rt2dObject *
_rt2dPickRegionStreamWrite(Rt2dObject *pickRegion, RwStream *stream);

extern Rt2dObject *
_rt2dPickRegionRender(Rt2dObject *pickRegion);

#endif /* PICKREGION_H */

