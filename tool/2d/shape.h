
/***************************************************************************
 *                                                                         *
 * Module  : shape.h                                                       *
 *                                                                         *
 * Purpose :                                                               *
 *                                                                         *
 **************************************************************************/

#ifndef SHAPE_H
#define SHAPE_H

/****************************************************************************
 Includes
 */

/****************************************************************************
 Defines
 */

/****************************************************************************
 Global Types
 */
typedef struct _rt2dShapeNode _rt2dShapeNode;
struct _rt2dShapeNode
{
    RwUInt32    flag;           /**< Shape's property flag, including outline or filled. */
    Rt2dPath    *path;          /**< path forming the the shape */
    Rt2dBrush   *brush;         /**< brush for drawing the shape */
};


typedef struct _rt2dStreamShape _rt2dStreamShape;
struct _rt2dStreamShape
{
    RwUInt32 version;
    RwInt32  nodeCount;
};

#define rt2dSTREAMSHAPENODEGOTFILL      0x01
#define rt2dSTREAMSHAPENODEGOTSTROKE    0x02
#define rt2dSTREAMSHAPENODEGOTPATH      0x04  /* Should be always true */

typedef struct _rt2dStreamShapeNode _rt2dStreamShapeNode;

struct _rt2dStreamShapeNode
{
    RwInt32 flags;
};

/****************************************************************************
 Functions
 */
extern RwBool
_rt2dShapeInit(Rt2dObject * shape, RwInt32 nodeCount, _rt2dShapeRep *rep);

extern RwBool
_rt2dShapeDestruct(Rt2dObject * shape);

/* inner-level stream get size; doesn't include pooled resources */
extern RwUInt32
_rt2dShapeStreamGetSize(Rt2dObject *object, _rt2dSceneResourcePool *pool);

extern Rt2dObject *
_rt2dShapeStreamReadTo(Rt2dObject *shape, RwStream *stream, _rt2dSceneResourcePool *pool);

extern Rt2dObject *
_rt2dShapeStreamWrite(Rt2dObject *shape, RwStream *stream, _rt2dSceneResourcePool *pool);

extern RwUInt32
_rt2dShapeRepStreamGetSize(_rt2dShapeRep *rep);

extern _rt2dShapeRep *
_rt2dShapeRepStreamRead(RwStream *stream);

extern _rt2dShapeRep *
_rt2dShapeRepStreamWrite(_rt2dShapeRep *rep, RwStream *stream);

extern RpAtomic *
_rt2dShapeCreateAtomic(Rt2dObject *shape);

extern Rt2dObject *
_rt2dShapeCopy(Rt2dObject *dst, Rt2dObject *src);

extern Rt2dObject *
_rt2dShapeRenderChild(Rt2dObject *dst, const RwRGBAReal *colorMult, const RwRGBAReal *colorOffs);

#endif /* SHAPE_H */
