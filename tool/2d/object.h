
/***************************************************************************
 *                                                                         *
 * Module  : object.h                                                    *
 *                                                                         *
 * Purpose :                                                               *
 *                                                                         *
 **************************************************************************/

#ifndef OBJECT_H
#define OBJECT_H

/****************************************************************************
 Global Types
 */
typedef struct _rt2dStreamObject _rt2dStreamObject;
struct _rt2dStreamObject
{
    RwUInt32   version;
    RwInt32    type;            /* tag identifying type of object */
    RwInt32    depth;           /* z-order of object under parent */
    RwRGBAReal colorMult;       /* color multiply applied to object  */
    RwRGBAReal colorOffs;       /* color offset applied to object    */
};

/****************************************************************************
 Macros
 */
#define RwRGBARealTransformAssignMult(d,s,m)                         \
MACRO_START                                                          \
{                                                                    \
    (d)->red   = (s)->red   * (m)->red;                              \
    (d)->green = (s)->green * (m)->green;                            \
    (d)->blue  = (s)->blue  * (m)->blue;                             \
    (d)->alpha = (s)->alpha * (m)->alpha;                            \
}                                                                    \
MACRO_STOP

#define RwRGBARealTransformAssignOffset(d,s,m,o)                 \
MACRO_START                                                      \
{                                                                \
    (d)->red   = (s)->red/255.0f   * (m)->red   + (o)->red;      \
    (d)->green = (s)->green/255.0f * (m)->green + (o)->green;    \
    (d)->blue  = (s)->blue/255.0f  * (m)->blue  + (o)->blue;     \
    (d)->alpha = (s)->alpha/255.0f * (m)->alpha + (o)->alpha;    \
}                                                                \
MACRO_STOP

#define RwRGBARealTransformAssignOffsetFinal(d,s,m,o)           \
MACRO_START                                                     \
{                                                               \
    (d)->red   = (s)->red   * (m)->red   + (o)->red * 255.0f;   \
    (d)->green = (s)->green * (m)->green + (o)->green * 255.0f; \
    (d)->blue  = (s)->blue  * (m)->blue  + (o)->blue * 255.0f;  \
    (d)->alpha = (s)->alpha * (m)->alpha + (o)->alpha* 255.0f;  \
}                                                               \
MACRO_STOP

/****************************************************************************
 Function prototypes
 */
extern Rt2dObject *
_rt2dObjectCreate(void);

extern Rt2dObject*
_rt2dObjectInit(Rt2dObject *object, RwInt32 objectType);

extern RwBool
_rt2dObjectDestroy(Rt2dObject *object);

extern RwBool
_rt2dObjectDestruct(Rt2dObject *object);

extern void
_rt2dObjectRealloc(Rt2dObject *dst, Rt2dObject *src);

extern void
_rt2dObjectDestroyAfterRealloc(Rt2dObject *object);

extern RwUInt32
_rt2dObjectBaseStreamGetSize(Rt2dObject *object);

extern Rt2dObject *
_rt2dObjectBaseStreamWrite(Rt2dObject *object, RwStream *stream);

extern Rt2dObject *
_rt2dObjectBaseStreamReadTo(Rt2dObject *object, RwStream *stream);

extern RwUInt32
_rt2dObjectStreamGetSize(Rt2dObject *object, _rt2dSceneResourcePool *pool);

extern Rt2dObject *
_rt2dObjectStreamWrite(Rt2dObject *object, RwStream *stream, _rt2dSceneResourcePool *pool);

extern Rt2dObject *
_rt2dObjectStreamReadTo(Rt2dObject *object, RwStream *stream, _rt2dSceneResourcePool *pool);

extern Rt2dObject *
_rt2dObjectRender(Rt2dObject  *object, const RwRGBAReal *colorMult, const RwRGBAReal *colorOffs);

extern void
_rt2dObjectClose(void);

extern RwBool
_rt2dObjectOpen(void);

#endif /* OBJECT_H */
