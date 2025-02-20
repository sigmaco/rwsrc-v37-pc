/*
 *  crowdnull.c
 *
 *  Null version of RpCrowd.
 */

#include "rwcore.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpcrowd.h"
#include "crowdcommon.h"

/******************************************************************************
 *  Global variables
 */

/* RpCrowd device extension data */
RwUInt32 _rpCrowdDeviceDataSize = 0;

/*
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *      Standard interface functions
 *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/******************************************************************************
 */
RwBool
_rpCrowdDeviceOpen(void)
{
    RWFUNCTION(RWSTRING("_rpCrowdDeviceOpen"));

    /* Nothing to do */

    RWRETURN(TRUE);
}

/******************************************************************************
 */
RwBool
_rpCrowdDeviceClose(void)
{
    RWFUNCTION(RWSTRING("_rpCrowdDeviceClose"));

    /* Nothing to do */

    RWRETURN(TRUE);
}

/******************************************************************************
 */
RpCrowd *
_rpCrowdDeviceInitCrowd(RpCrowd *crowd)
{
    RWFUNCTION(RWSTRING("_rpCrowdDeviceInitCrowd"));
    RWASSERT(crowd);

    /* Null */

    RWRETURN(crowd);
}

/******************************************************************************
 */
RwBool
_rpCrowdDeviceDeInitCrowd(RpCrowd *crowd)
{
    RWFUNCTION(RWSTRING("_rpCrowdDeviceDeInitCrowd"));
    RWASSERT(crowd);

    /* Null */

    RWRETURN(TRUE);
}

/******************************************************************************
 */
RpCrowd *
_rpCrowdDeviceRenderCrowd(RpCrowd *crowd)
{
    RWFUNCTION(RWSTRING("_rpCrowdDeviceRenderCrowd"));
    RWASSERT(crowd);

    /* Null */

    RWRETURN(crowd);
}
