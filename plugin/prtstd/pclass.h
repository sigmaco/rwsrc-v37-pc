/**********************************************************************
 *
 * File :     pclass.h
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


#ifndef PCLASS_HDR
#define PCLASS_HDR

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

extern RpPrtStdParticleBatch *
_rpPrtStdPrtBatchDefaultCB(RpPrtStdEmitter *prtEmt,
                           RpPrtStdParticleBatch *prtBatch, void *data);

/************************************************************************/

extern RpPrtStdParticleClass *
_rpPrtStdPClassCtor(RpPrtStdParticleClass *pClass);

extern RpPrtStdParticleClass *
_rpPrtStdPClassDtor(RpPrtStdParticleClass *pClass);


/************************************************************************/

extern RpPrtStdParticleClass *
_rpPrtStdPClassCreate( void );

extern RwBool
_rpPrtStdPClassDestroy(RpPrtStdParticleClass *pClass);

extern RpPrtStdParticleClass *
_rpPrtStdPClassAdd(RpPrtStdParticleClass *pClassHead,
                   RpPrtStdParticleClass *pClass);

/************************************************************************/

extern RpPrtStdParticleBatch *
_rpPrtStdPClassCreateParticleBatch(RpPrtStdParticleClass *pClass,
                                          RwInt32 numPrt);

extern RpPrtStdParticleBatch *
_rpPrtStdPClassDestroyParticleBatch(RpPrtStdParticleClass *pClass,
                                           RpPrtStdParticleBatch *prtBatch,
                                           void *data);

/************************************************************************/

extern RpPrtStdParticleClass *
_rpPrtStdPClassSetCallBack(RpPrtStdParticleClass *pClass,
                                  RpPrtStdParticleCallBackCode prtCBCode,
                                  RpPrtStdParticleCallBack prtCB);

extern RpPrtStdParticleClass *
_rpPrtStdPClassGetCallBack(RpPrtStdParticleClass *pClass,
                                  RpPrtStdParticleCallBackCode prtCBCode,
                                  RpPrtStdParticleCallBack *prtCB);


#ifdef    __cplusplus
}
#endif         /* __cplusplus */

#endif /* PCLASS_HDR */

