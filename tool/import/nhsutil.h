/***************************************************************************
 *                                                                         *
 * Module  : nhsutl.h                                                      *
 *                                                                         *
 * Purpose : World handling functions.                                     *
 *                                                                         *
 **************************************************************************/

#ifndef RTNHSUTL_H
#define RTNHSUTL_H

/****************************************************************************
 Includes
 */

#include "rwcore.h"

#include "nhsworld.h"
#include "rtimport.h"

/****************************************************************************
 Defines
 */

#define RwApplyTan(_x) RwQuadTan(_x)
#define RwApplyATan(_t) RwQuadATan(_t)
#define RwApplyATan2(_s, _c) RwQuadATan2(_s, _c)


#define RwBBoxAddPointMacro(_boundBox, _vertex) \
MACRO_START                                     \
{                                               \
    if ((_boundBox)->inf.x > (_vertex)->x)      \
    {                                           \
        (_boundBox)->inf.x = (_vertex)->x;      \
    }                                           \
    if ((_boundBox)->inf.y > (_vertex)->y)      \
    {                                           \
        (_boundBox)->inf.y = (_vertex)->y;      \
    }                                           \
    if ((_boundBox)->inf.z > (_vertex)->z)      \
    {                                           \
        (_boundBox)->inf.z = (_vertex)->z;      \
    }                                           \
                                                \
    if ((_boundBox)->sup.x < (_vertex)->x)      \
    {                                           \
        (_boundBox)->sup.x = (_vertex)->x;      \
    }                                           \
    if ((_boundBox)->sup.y < (_vertex)->y)      \
    {                                           \
        (_boundBox)->sup.y = (_vertex)->y;      \
    }                                           \
    if ((_boundBox)->sup.z < (_vertex)->z)      \
    {                                           \
        (_boundBox)->sup.z = (_vertex)->z;      \
    }                                           \
}                                               \
MACRO_STOP


#if (!defined(RwApplyTan))
#define RwApplyTan(_x) RwTan(_x)
#endif /* (!defined(RwApplyTan)) */

#if (!defined(RwApplyATan))
#define RwApplyATan(_t) RwATan(_t)
#endif /* (!defined(RwApplyATan)) */

#if (!defined(RwApplyATan))
#define RwApplyATan2(_s, _c) RwATan2(_s, _c)
#endif /* (!defined(RwApplyATan)) */

/****************************************************************************
 Global types
 */

/****************************************************************************
 Function prototypes
 */

#ifdef    __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/* Build sector stuff */
extern RtWorldImportBuildSector *
_rtImportBuildSectorCreate(void);

extern RwReal
_rtImportBuildSectorFindBBoxVolume(RwBBox *boundingBox);
extern RwReal
_rtImportBuildSectorFindBBoxDiagonal(RwBBox *boundingBox);
extern RwReal
_rtImportBuildSectorFindBBoxArea(RwBBox *boundingBox);
extern RwReal
_rtWorldImportBuildSectorAreaOfWall(RwBBox *boundingBox, RwInt32 type);

extern RwBool
_rtImportBuildSectorDestroy(RtWorldImportBuildSector *buildSector,
                      RtWorldImportUserdataCallBacks *_rtWorldImportUserDataCallBacks);

extern RwInt32
_rtImportBuildSectorFindNumTriangles(RtWorldImportBuildSector *buildSector);

extern RtWorldImportBuildSector *
_rtImportBuildSectorTriangulize(RtWorldImportBuildSector * buildSector, RwInt32 numTriangles,
                          RtWorldImportUserdataCallBacks *_rtWorldImportUserDataCallBacks);


/* Build plane sector and plane sector stuff */
extern RtWorldImportBuildPlaneSector *
_rtImportBuildPlaneSectorCreate(RwInt32 type,RwReal value);

extern RwBool
_rtImportPlaneSectorDestroy(RtWorldImportBuildPlaneSector *sector);

extern void
_rtImportPlaneSectorDestroyTree(RtWorldImportBuildPlaneSector *sector,
                               RtWorldImportUserdataCallBacks *_rtWorldImportUserDataCallBacks);

/* World sector stuff */
extern RpWorldSector *
_rtImportWorldSectorInitialize(RpWorldSector *worldSector,
                               RpWorld *world,
                               RwBBox *bbpBox,
                               RwInt32 numTriangles,
                               RwInt32 numVertices);

extern RwBool
_rtImportWorldSectorCheck(RpWorldSector *worldSector);

extern void
_rtImportBuildSectorCheck(RtWorldImportBuildSector *buildSector);

#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#define _rtImportBuildPlaneSectorDestroy(_sector)       \
        _rtImportPlaneSectorDestroy(_sector)

#define _rtImportBuildPlaneSectorDestroyTree(_sector,                           \
                                             _rtWorldImportUserDataCallBacks)   \
        _rtImportPlaneSectorDestroyTree(_sector,                                \
                                        _rtWorldImportUserDataCallBacks)

#endif /* RTNHSUTL_H */
