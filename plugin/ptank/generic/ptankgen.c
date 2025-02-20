/****************************************************************************
 *
 * This file is a product of Criterion Software Ltd.
 *
 * This file is provided as is with no warranties of any kind and is
 * provided without any obligation on Criterion Software Ltd.
 * or Canon Inc. to assist in its use or modification.
 *
 * Criterion Software Ltd. and Canon Inc. will not, under any
 * circumstances, be liable for any lost revenue or other damages
 * arising from the use of this file.
 *
 * Copyright (c) 2001 Criterion Software Ltd.
 * All Rights Reserved.
 *
 */
/****************************************************************************
 *
 * ptankgen.c
 *
 * Copyright (C) 2001 Criterion Technologies.
 *
 *
 * Purpose: Generic Billboard implementation.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rwcore.h"
#include "rpworld.h"
#include "rpplugin.h"
#include "rpdbgerr.h"
#include "ptank.h"
#include "ptankprv.h"
#include "ptankg.h"


#include "ptankgen.h"

#define ALIGNPAD (4)


/*
 ***************************************************************************
 */
static RwBool
rpGENCreate(RpAtomic *atomic, RpPTankData *ptankGlobal __RWUNUSED__,
            RwInt32 maxPCount, RwUInt32 dataFlags,
            RwUInt32 platFlags __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("rpGENCreate"));

    RWRETURN(_rpPTankGenCreate(atomic,maxPCount, dataFlags));
}


/*
 *****************************************************************************
 */
static RwBool
rpGENInstance(RpAtomic *atomic, RpPTankData *ptankGlobal, RwInt32 actPCount, RwUInt32 instFlags)
{
    RwBool result = FALSE;
#if 0
    RwFrame *camFrame;
    RwFrame *atmFrame;
    RwMatrix    *atmMtx;
#endif

    RWFUNCTION(RWSTRING("rpGENInstance"));
    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    if((instFlags & rpPTANKIFLAGACTNUMCHG ) == rpPTANKIFLAGACTNUMCHG)
    {
        _rpPTankGENKill(atomic, actPCount,
                        RpPTankAtomicGetMaximumParticlesCount(atomic));
    }

    result = _rpPTankGENInstance(atomic, ptankGlobal,actPCount, instFlags);

    /* if we're here geometry might have been locked so just unlock it */

    RpGeometryUnlock(RpAtomicGetGeometry(atomic));

#if 0
    /* should be in the render callback but I'm a lazy f****/
    camFrame = RwCameraGetFrame(RwCameraGetCurrentCamera());
    atmFrame = RpAtomicGetFrame(atomic);

    atmMtx = RwFrameGetMatrix(atmFrame);
    RwV3dAssign(RwMatrixGetPos(atmMtx),
                RwMatrixGetPos(RwFrameGetLTM(camFrame)));

    /* update the matrix flags since the matrix pos has been changed */
    RwMatrixUpdate( atmMtx );

    RwFrameUpdateObjects(atmFrame);
#endif
    RWRETURN(result);
}


/*
 *****************************************************************************
 */
RpPTankCallBacks defaultCB =
{
    NULL,         /* though rpGENAlloc exist it's exactly
                   * the same as the default one, so let's keep using the default */
    rpGENCreate,
    rpGENInstance,
    NULL          /* no render callback */
};


/*
 *****************************************************************************
 */
void        *
PTankClose(void *object,
           RwInt32 __RWUNUSED__ offset,
           RwInt32 __RWUNUSED__ size)
{
    RWFUNCTION(RWSTRING("PTankClose"));

    RWRETURN(object);
}



/*
 *****************************************************************************
 */
void        *
PTankOpen(void *object,
          RwInt32 __RWUNUSED__ offset,
          RwInt32 __RWUNUSED__ size)
{
    RWFUNCTION(RWSTRING("PTankOpen"));

    RWRETURN(object);
}

