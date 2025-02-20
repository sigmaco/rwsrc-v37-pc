
/***************************************************************************
 *                                                                         *
 * Module  : nhsutl.h                                                      *
 *                                                                         *
 * Purpose : World handling functions.                                     *
 *                                                                         *
 **************************************************************************/

#ifndef RTNHSSPLIT_H
#define RTNHSSPLIT_H

/****************************************************************************
 Includes
 */

#include "rwcore.h"
#include "nhsworld.h"
#include "rtimport.h"


/****************************************************************************
 Global types
 */

/****************************************************************************
 Function prototypes
 */

#ifdef    __cplusplus
extern              "C"
{
#endif                          /* __cplusplus */

extern RtWorldImportBuildSector *
_rtImportBuildSectorCreateFromNoHSWorld(RtWorldImport * wpNoHS,
                                  RpMaterialList * matList,
                                  RtWorldImportUserdataCallBacks *
                                  UserDataCallBacks);

extern RpWorld            *
_rtImportWorldCreateWorld(RtWorldImport * nohsworld,
                          RwBool              *ExitRequested,
                          RtWorldImportUserdataCallBacks *
                          UserDataCallBacks);
#if 0
extern void
_rtImportBuildSectorConditionGeometry(//RtWorldImport * wpNoHS,
                                RtWorldImportBuildSector * buildSector,
                                RpMaterialList * matList);
#endif
/*extern RtWorldImportTerminationBuildCallBack _rtWorldImportTerminationCallBack;
extern RtWorldImportPartitionBuildCallBack _rtWorldImportPartitionCallBack;*/
extern RtWorldImportBuildCallBacks BuildCallBacks;

#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* RTNHSSPLIT_H */
