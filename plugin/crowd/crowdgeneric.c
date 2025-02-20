/*
 *  crowdgeneric.c
 *
 *  Generic implementation of RpCrowd
 *
 *  Uses the static billboard system (see crowdstatic.c|h);
 */

/******************************************************************************
 *  Includes
 */

#include "rwcore.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpcrowd.h"
#include "crowdcommon.h"
#include "crowdstaticbb.h"

/******************************************************************************
 *  Defines
 */

/*
 *  NUMMATPERSEQ is the number of points in a sequence of an
 *  RpCrowdAnimation which are simultaneously referenced by materials
 *  of the crowd. This should be <= rpCROWDANIMATIONMAXSTEPS. Large
 *  numbers give a crowd more randomness, whereas smaller numbers might
 *  be more efficient.
 */
#define NUMMATPERSEQ  rpCROWDANIMATIONMAXSTEPS

/******************************************************************************
 *  Global variables
 */

/* RpCrowd device extension data */
RwUInt32 _rpCrowdDeviceDataSize = sizeof(rpCrowdStaticBB);

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
    rpCrowdStaticBB *staticBB = (rpCrowdStaticBB *)CROWDGETDEVICEDATA(crowd);

    RWFUNCTION(RWSTRING("_rpCrowdDeviceInitCrowd"));
    RWASSERT(crowd);

    if (!_rpCrowdStaticBBCreate(crowd, staticBB, NUMMATPERSEQ))
    {
        RWRETURN((RpCrowd *)NULL);
    }

    RWRETURN(crowd);
}

/******************************************************************************
 */
RwBool
_rpCrowdDeviceDeInitCrowd(RpCrowd *crowd)
{
    rpCrowdStaticBB *staticBB = (rpCrowdStaticBB *)CROWDGETDEVICEDATA(crowd);

    RWFUNCTION(RWSTRING("_rpCrowdDeviceDeInitCrowd"));
    RWASSERT(crowd);

    _rpCrowdStaticBBDestroy(crowd, staticBB);

    RWRETURN(TRUE);
}

/******************************************************************************
 */
RpCrowd *
_rpCrowdDeviceRenderCrowd(RpCrowd *crowd)
{
    rpCrowdStaticBB *staticBB = (rpCrowdStaticBB *)CROWDGETDEVICEDATA(crowd);

    RWFUNCTION(RWSTRING("_rpCrowdDeviceRenderCrowd"));
    RWASSERT(crowd);

    _rpCrowdStaticBBSetTextures(crowd, staticBB);

    /* Render the atomic */
    RpAtomicRender(staticBB->atomic);

    RWRETURN(crowd);
}
