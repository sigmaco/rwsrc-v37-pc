
/***************************************************************************
 *                                                                         *
 * Module  : nhsscheme.h                                                   *
 *                                                                         *
 * Purpose : .                                                             *
 *                                                                         *
 **************************************************************************/

#ifndef RTNHSSCHEME_H
#define RTNHSSCHEME_H

/****************************************************************************
 Includes
 */

#include "rwcore.h"
#include "nhsworld.h"
#include "nhssplit.h"
#include "rtimport.h"

/****************************************************************************
 Defines
 */

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

extern RwBool 
_rtWorldImportGuidedKDStackPartitionTerminator(RtWorldImportBuildSector * buildSector,
                                               RtWorldImportBuildStatus * status,
                                               void * userData);

extern void 
_rtWorldImportBuildSchemeCacheCleanUp(void);


extern RwBool
_rtWorldImportCullMiddleMaterialBBox(RtWorldImportBuildSector *buildSector,
                                     RwBBox *materialBBox,
                                     RwBBox *newBBox,
                                     RwInt32 matIndex);

extern RwReal
_rtWorldImportGuidedKDPartitionSelector(RtWorldImportBuildSector *buildSector,
                                             RtWorldImportBuildStatus * buildStatus,
                                             RtWorldImportPartition *partition,
                                             void *userData);

extern RwReal
_rtWorldImportFuzzyBalancedPartitionSelector(RtWorldImportBuildSector *buildSector,
                                             RtWorldImportBuildStatus *buildStatus,
                                             RtWorldImportPartition *partition,
                                             void *userData);


#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* RTNHSSPLIT_H */
