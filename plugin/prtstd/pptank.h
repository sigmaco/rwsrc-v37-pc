/**********************************************************************
 *
 * File :     ptank.h
 *
 * Abstract : Basic Particle Engine in RenderWare
 *
 **********************************************************************
 *
 * This file is a product of Criterion Software Ltd.
 *
 * This file is provided as is with no warranties of any kind and is
 * provided without any obligation on Criterion Software Ltd. or
 * Canon Inc. to assist in its use or modification.
 *
 * Criterion Software Ltd. will not, under any
 * circumstances, be liable for any lost revenue or other damages arising
 * from the use of this file.
 *
 * Copyright (c) 1998 Criterion Software Ltd.
 * All Rights Reserved.
 *
 * RenderWare is a trademark of Canon Inc.
 *
 ************************************************************************/

#ifndef PTANK_HDR
#define PTANK_HDR

#include <rwcore.h>

#include "rpprtstd.h"

/************************************************************************
 *
 *
 ************************************************************************/

#ifdef    __cplusplus
extern "C"
{
#endif         /* __cplusplus */

/************************************************************************/

extern RpPrtStdEmitterPTank *
_rpPrtStdPTankCtor(RpPrtStdEmitterPTank *emtPTank);

extern RpPrtStdEmitterPTank *
_rpPrtStdPTankDtor(RpPrtStdEmitterPTank *emtPTank);

extern RpPrtStdEmitterPTank *
_rpPrtStdPTankClone(RpPrtStdEmitterPTank *dstEmtPTank, RpPrtStdEmitterPTank *srcEmtPTank);

extern RpPrtStdEmitterPTank *
_rpPrtStdPTankGetDataPtr(RpPrtStdEmitterPTank *emtPTank);

extern RpPrtStdEmitterPTank *
_rpPrtStdPTankCreateDataPtr(RpPrtStdEmitterPTank *emtPTank);

extern RpPrtStdEmitterPTank *
_rpPrtStdPTankStreamRead(RwStream *stream, RpPrtStdEmitterPTank *emtPTank);

extern RpPrtStdEmitterPTank *
_rpPrtStdPTankStreamWrite(RwStream *stream, RpPrtStdEmitterPTank *emtPTank);

extern RwInt32
_rpPrtStdPTankStreamGetSize(RpPrtStdEmitterPTank *emtPTank);

extern RpPrtStdEmitter *
_rpPrtStdEmitterDestroyPTank(RpPrtStdEmitter *prtEmt);

#ifdef    __cplusplus
}
#endif         /* __cplusplus */


#endif /* PTANK_HDR */
