/**********************************************************************
 *
 * File :     elass.h
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

#ifndef ECLASS_HDR
#define ECLASS_HDR


#include <rwcore.h>


#include "rpprtstd.h"

/************************************************************************
 *
 *
 ************************************************************************/

/************************************************************************
 *
 *
 ************************************************************************/

/************************************************************************
 *
 *
 ************************************************************************/

#ifdef    __cplusplus
extern "C"
{
#endif         /* __cplusplus */

/************************************************************************/

extern RpPrtStdEmitter *
_rpPrtStdEmtDefaultCB(RpAtomic *atomic,
                      RpPrtStdEmitter *prtEmt, void *data);

/************************************************************************/

extern RpPrtStdEmitterClass *
_rpPrtStdEClassCtor(RpPrtStdEmitterClass *eClass);


extern RpPrtStdEmitterClass *
_rpPrtStdEClassDtor(RpPrtStdEmitterClass *eClass);


/************************************************************************/

extern RpPrtStdEmitterClass *
_rpPrtStdEClassCreate( void );

extern RwBool
_rpPrtStdEClassDestroy(RpPrtStdEmitterClass *eClass);

extern RpPrtStdEmitterClass *
_rpPrtStdEClassAdd(RpPrtStdEmitterClass *eClassHead,
                         RpPrtStdEmitterClass *eClass);

/************************************************************************/

extern RpPrtStdEmitterClass *
_rpPrtStdEClassSetCallBack(RpPrtStdEmitterClass *eClass,
                             RpPrtStdEmitterCallBackCode emtCBCode,
                             RpPrtStdEmitterCallBack emtCB);

extern RpPrtStdEmitterClass *
_rpPrtStdEClassGetCallBack(RpPrtStdEmitterClass *eClass,
                             RpPrtStdEmitterCallBackCode emtCBCode,
                             RpPrtStdEmitterCallBack *emtCB);

#ifdef    __cplusplus
}
#endif         /* __cplusplus */

#endif /* EcLASS_HDR */

