/*
 *
 * Functionality for 2D rendering
 *
 * Copyright (c) 2001 Criterion Software Ltd.
 */

/****************************************************************************
 *                                                                          *
 *  Module  :   bucket.c                                                    *
 *                                                                          *
 *  Purpose :   simple 2d animation maestro functionality                  *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
 */

#include <string.h>

#include "rwcore.h"
#include <rpdbgerr.h>
#include <rpcriter.h>
#include "rt2danim.h"

#include "maestro.h"
#include "bucket.h"

/****************************************************************************
 Local Types
 */

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */

/****************************************************************************
 Local functions
 */

RwSList *
_rt2dByteBucketCreate(void)
{
    RwSList    *result;

    RWFUNCTION(RWSTRING("_rt2dByteBucketCreate"));

    result = _rwSListCreate(sizeof(RwChar),
        rwID_2DANIM | rwMEMHINTDUR_EVENT);

    /* Create a dummy entry to occupy position zero. */
#if (0)
    if (result)
    {
        RwChar      *tmp;

        tmp = _rwSListGetNewEntry(result);
    }
#endif /* (0) */

    RWRETURN(result);
}

RwBool
_rt2dByteBucketDestroy(RwSList * ByteBucket)
{
    RWFUNCTION(RWSTRING("_rt2dByteBucketDestroy"));

    RWASSERT(ByteBucket);

    _rwSListDestroy(ByteBucket);

    RWRETURN(TRUE);
}

RwInt32
_rt2dByteBucketAddData(RwSList * ByteBucket, const void * data, RwInt32 length)
{
    RwInt32             newIdx;
    RwChar              *newData;

    RWFUNCTION(RWSTRING("_rt2dByteBucketAddData"));

    RWASSERT(ByteBucket);
    RWASSERT(length > 0);

    newIdx = _rwSListGetNumEntries(ByteBucket);

    /* Ask for a new data area. */
    if ((newData = (RwChar *)_rwSListGetNewEntries(ByteBucket, length,
        rwID_2DANIM | rwMEMHINTDUR_EVENT)) == NULL)
    {
        RWRETURN(-1);
    }

    /* Copy the data into the bit bucket. */
    if (data)
    {
        memcpy(newData, data, length);
    }

    RWRETURN(newIdx);
}

void *
_rt2dByteBucketGetDataByIndex(RwSList * ByteBucket, RwInt32 idx)
{
    RwChar        *newData;

    RWFUNCTION(RWSTRING("_rt2dByteBucketGetDataByIndex"));

    RWASSERT(ByteBucket);
    RWASSERT(idx < _rwSListGetNumEntries(ByteBucket));

    newData = (RwChar *)_rwSListGetEntry(ByteBucket, idx);

    RWRETURN(newData);
}

/****************************************************************************
 API functions
 */

