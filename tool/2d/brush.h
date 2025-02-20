
/**************************************************************************
 *                                                                        *
 * Module  : brush.h                                                      *
 *                                                                        *
 * Purpose :                                                              *
 *                                                                        *
 **************************************************************************/

#ifndef BRUSH_H
#define BRUSH_H

/****************************************************************************
 Defines
 */
#define MyRGBARealFromRGBA(o,i)                 \
MACRO_START                                     \
{                                               \
    (o)->red = (RwReal)(i)->red;                \
    (o)->green = (RwReal)(i)->green;            \
    (o)->blue = (RwReal)(i)->blue;              \
    (o)->alpha = (RwReal)(i)->alpha;            \
}                                               \
MACRO_STOP

#define MyRGBAFromRGBAReal(o,i)                                \
MACRO_START                                                    \
{                                                              \
    (o)->red   = (RwUInt8)RwInt32FromRealMacro((i)->red);       \
    (o)->green = (RwUInt8)RwInt32FromRealMacro((i)->green);     \
    (o)->blue  = (RwUInt8)RwInt32FromRealMacro((i)->blue);      \
    (o)->alpha = (RwUInt8)RwInt32FromRealMacro((i)->alpha);     \
}                                                              \
MACRO_STOP

#define RwRGBARealAssignAndClamp(d,s)                       \
MACRO_START                                                 \
{                                                           \
    (d)->red = RwRealMax2(0.0f,RwRealMin2((s)->red,255.0f));        \
    (d)->green = RwRealMax2(0.0f,RwRealMin2((s)->green,255.0f));    \
    (d)->blue = RwRealMax2(0.0f,RwRealMin2((s)->blue,255.0f));      \
    (d)->alpha = RwRealMax2(0.0f,RwRealMin2((s)->alpha,255.0f));    \
}                                                           \
MACRO_STOP

/*
 * RwRGBARealTransformAssignAndClamp
 *    d = destination RwRGBAReal
 *    s = source RwRGBA or RwRGBAReal, values between 0 and 255f
 *    m = scale RwRGBAReal, values between 0.0f and 1.0f
 *    o = offset RwRGBAReal, values between 0.0f and 255.0f
 *
 *    The odd order of the assignments (red, blue, alpha, green) was necessitated by an
 *    XBox internal compiler bug.
 */

#define RwRGBARealTransformAssignAndClamp(d,s,m,o)                                   \
MACRO_START                                                                          \
{                                                                                    \
    (d)->red   = RwRealMax2(0.0f,RwRealMin2((s)->red   * (m)->red   + (o)->red,   255.0f)); \
    (d)->green = RwRealMax2(0.0f,RwRealMin2((s)->green * (m)->green + (o)->green, 255.0f)); \
    (d)->blue  = RwRealMax2(0.0f,RwRealMin2((s)->blue  * (m)->blue  + (o)->blue,  255.0f)); \
    (d)->alpha = RwRealMax2(0.0f,RwRealMin2((s)->alpha * (m)->alpha + (o)->alpha, 255.0f)); \
}                                                                                    \
MACRO_STOP

#ifndef min
#define min(x, y) (((x) > (y))? (y): (x))
#endif

#ifndef max
#define max(x, y) (((x) < (y))? (y): (x))
#endif

#define RwRGBARealTransformAssignClampAndConvert(_d,_s,_m,_o)                       \
MACRO_START                                                                         \
{                                                                                   \
    const RwReal red   = ((_s)->red   * (_m)->red   + (_o)->red);                   \
    const RwReal green = ((_s)->green * (_m)->green + (_o)->green);                 \
    const RwReal blue  = ((_s)->blue  * (_m)->blue  + (_o)->blue);                  \
    const RwReal alpha = ((_s)->alpha * (_m)->alpha + (_o)->alpha);                 \
    const RwInt32 nred   = RwInt32FromRealMacro(red);                               \
    const RwInt32 ngreen = RwInt32FromRealMacro(green);                             \
    const RwInt32 nblue  = RwInt32FromRealMacro(blue);                              \
    const RwInt32 nalpha = RwInt32FromRealMacro(alpha);                             \
    (_d)->red   = (RwUInt8)max(0, min(nred,   255));                                \
    (_d)->green = (RwUInt8)max(0, min(ngreen, 255));                                \
    (_d)->blue  = (RwUInt8)max(0, min(nblue,  255));                                \
    (_d)->alpha = (RwUInt8)max(0, min(nalpha, 255));                                \
}                                                                                   \
MACRO_STOP

/****************************************************************************
 Global Types
 */

/* For the _rt2dBrush flag member
 *   FIELDRGBA indicates that the corner colors are the same, and
 *   interpolation isn't necessary
 */
#define FIELDRGBA 1
#define FIELDUV   2

/****************************************************************************
 Global Types
 */
typedef struct _rt2dStreamBrush _rt2dStreamBrush;
struct _rt2dStreamBrush
{
    RwUInt32            version;
    rt2dShadeParameters top;
    rt2dShadeParameters dtop;
    rt2dShadeParameters bottom;
    rt2dShadeParameters dbottom;
    RwUInt32            flag;
    RwReal              halfwidth;
    RwBool              gotTexture;
};

/****************************************************************************
 Function prototypes
 */
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern RwBool _rt2dBrushOpen(void);
extern void   _rt2dBrushClose(void);

extern Rt2dBrush *
_rt2dBrushTransformRGBARealDirect(Rt2dBrush *brush,
                                  RwRGBAReal *col0, RwRGBAReal *col1,
                                  RwRGBAReal *col2, RwRGBAReal *col3,
                                  RwRGBAReal *mult, RwRGBAReal *offset);

extern Rt2dBrush *
_rt2dBrushStreamReadTo(Rt2dBrush *brush,RwStream *stream);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRUSH_H */
