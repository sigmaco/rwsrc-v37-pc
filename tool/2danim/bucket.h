/***************************************************************************
 *                                                                         *
 * Module  : bucket.h                                                      *
 *                                                                         *
 * Purpose :                                                               *
 *                                                                         *
 **************************************************************************/

#ifndef BUCKET_H
#define BUCKET_H

/****************************************************************************
 Includes
 */
#include <rwcore.h>
#include "rt2danim.h"

#ifdef    __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/****************************************************************************
 Defines
 */

#define RT2D_STRING_MAX_BUFFER              64

/****************************************************************************
 Global Types
 */


/****************************************************************************
 Function prototypes
 */


extern RwSList *
_rt2dByteBucketCreate( void );

extern RwBool
_rt2dByteBucketDestroy( RwSList * bucket );

extern RwInt32
_rt2dByteBucketAddData( RwSList * bucket, const void * data, RwInt32 length);

extern void *
_rt2dByteBucketGetDataByIndex( RwSList * bucket, RwInt32 idx );

#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#endif /* BUCKET_H */

