
/***************************************************************************
 *                                                                         *
 * Module  : path.h                                                        *
 *                                                                         *
 * Purpose :                                                               *
 *                                                                         *
 **************************************************************************/

#ifndef PATH_H
#define PATH_H

/****************************************************************************
 Defines
 */
#define RT2DSTREAMPATHFLAGCLOSED        0x01
#define RT2DSTREAMPATHFLAGFLAT          0x02
#define RT2DSTREAMPATHFLAGGOTSUBPATH    0x04

#define _rt2dPathIsLocked(path) (path->numSegment == -1)

#define _rt2dPathGetNumSegment(path)               \
    (_rt2dPathIsLocked(path) ? rwSListGetNumEntries((RwSList *)(path)->segments) : path->numSegment)

#define _rt2dPathGetSegmentArray(path)               \
    ((rt2dPathNode *)(_rt2dPathIsLocked(path) ? rwSListGetArray((RwSList *)(path)->segments) : path->segments))

/****************************************************************************
 Global Types
 */
enum rt2dLinetype
{
    rt2dMOVE = 0,
    rt2dLINE = 1,
    rt2dCURVE = 2,
    rt2DLINETYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum rt2dLinetype rt2dLinetype;

typedef struct rt2dPathNode rt2dPathNode;
struct rt2dPathNode
{
    rt2dLinetype        type;
    RwV2d               pos, normal;
    RwReal              dist;
};

#include "rt2d.h"

struct Rt2dPath
{
    void                *segments;
    RwInt32             numSegment;
    RwBool              closed;
    RwBool              flat;
    RwReal              inset;
    Rt2dPath           *next;
    Rt2dPath           *curr;
};

typedef struct _rt2streamPath _rt2streamPath;
struct _rt2streamPath
{
    RwUInt32 version;
    RwInt32 numSegment;
    RwInt32 flag;
    RwReal inset;
};

/****************************************************************************
 Types for building a RpAtomic from a set of Rt2dPath and Rt2dBrush
 */

typedef struct _rt2dPathMeshNode _rt2dPathMeshNode;

struct _rt2dPathMeshNode
{
    Rt2dPath            *path;
    RwUInt32            numTri;
    RwUInt32            numVert;
    RwUInt32            numIdx;
    RwUInt32            flag;
    Rt2dBrush           *brush;
};

typedef struct _rt2dPathMeshHeader _rt2dPathMeshHeader;

struct _rt2dPathMeshHeader
{
    _rt2dPathMeshNode       *node;
    RwUInt32                numNode;
    RwUInt32                numTri;
    RwUInt32                numVert;
    RwUInt32                numIdx;
};


/****************************************************************************
 Function prototypes
 */
extern Rt2dPath    *_rt2dScratchPath(void);
extern Rt2dPath    *_rt2dSubPathFlatten(Rt2dPath * flatpath,
                                        const Rt2dPath * path);
extern Rt2dBBox    *_rt2dFlatSubPathExtendBBox(const Rt2dPath * path,
                                               Rt2dBBox * bbox);
extern Rt2dPath    *_rt2dPathOptimize(Rt2dPath * path);

extern void         _rt2dPathClose(void);
extern RwBool       _rt2dPathOpen(void);

extern Rt2dPath    *_rt2dPathStreamReadTo(Rt2dPath *path, RwStream *stream);

#endif /* PATH_H */
