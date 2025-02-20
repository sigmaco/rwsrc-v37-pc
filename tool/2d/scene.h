
/***************************************************************************
 *                                                                         *
 * Module  : scene.h                                                       *
 *                                                                         *
 * Purpose :                                                               *
 *                                                                         *
 **************************************************************************/

#ifndef SCENE_H
#define SCENE_H

/****************************************************************************
 Function prototypes
 */
extern Rt2dObject *
_rt2dSceneInit(Rt2dObject *object, RwInt32 objectCount);

extern RwBool
_rt2dSceneDestruct(Rt2dObject *scene);

extern Rt2dObject *
_rt2dSceneCopy(Rt2dObject *dst, Rt2dObject *src);

extern Rt2dObject *
_rt2dSceneGetNewChild(Rt2dObject *object, RwInt32 childType);

/* inner-level (takes into account shared resources) */
extern Rt2dObject *
_rt2dSceneStreamReadTo(Rt2dObject *scene, RwStream *stream, _rt2dSceneResourcePool *pool);

extern Rt2dObject *
_rt2dSceneStreamWrite(Rt2dObject *scene, RwStream *stream, _rt2dSceneResourcePool *pool);

extern RwUInt32
_rt2dSceneStreamGetSize(Rt2dObject *scene, _rt2dSceneResourcePool *pool);

extern Rt2dObject *
_rt2dSceneRenderChild(Rt2dObject *scene, const RwRGBAReal *colorMult, const RwRGBAReal *colorOffs);

#endif /* OBJECT_H */
