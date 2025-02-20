/***************************************************************************
 *                                                                         *
 * Module  : nhsworld.h                                                    *
 *                                                                         *
 * Purpose : World handling functions.                                     *
 *                                                                         *
 **************************************************************************/

#ifndef RTNHSWORLD_H
#define RTNHSWORLD_H

/****************************************************************************
 Includes
 */

#include "rwcore.h"
#include "rtimport.h"

/****************************************************************************
 Defines
 */


#define  RtWorldImportGetMaterialMacro(_nohsworld, _matInd)             \
    ( (_nohsworld) ?                                                    \
      rpMaterialListGetMaterial(&(_nohsworld)->matList, (_matInd)):     \
      ((RpMaterial *) NULL) )


/****************************************************************************
 Global types
 */




/*
 * Build plane sector -
 * the beginning of this needs to look like a RwPlaneSector
 */
typedef struct RtWorldImportBuildPlaneSector RtWorldImportBuildPlaneSector;
struct RtWorldImportBuildPlaneSector
{
    /* Polymorphism by inclusion - this way we can make conversion easier
     * (at the cost of 12 bytes per plane sector)
     * We'll just treat this whole structure as though it were an
     * RwPlaneSector - OK, since this is a transient stage.
     */
    RpPlaneSector       planeSector;
};

/****************************************************************************
 External variables
 */

extern RwInt32 _rtWorldImportTotalPolysInWorld;
extern RwInt32 _rtWorldImportNumPolysInLeaves;
extern RwInt32 _rtWorldImportNumPolysInCompressedLeaves;

/****************************************************************************
 Function prototypes
 */

#ifdef    __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

extern RpSector *
_rtImportWorldSectorCompressTree(RpSector *rootSector,
                     RpWorld *world,
                     RpMaterialList *noHsMatList,
                     RtWorldImportUserdataCallBacks *_rtWorldImportUserDataCallBacks);

#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#endif /* RTNHSWORLD_H */

