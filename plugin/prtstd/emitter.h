/**********************************************************************
 *
 * File :     emitter.h
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


#ifndef EMITTER_HDR
#define EMITTER_HDR

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
_rpPrtStdParticleBatchCtor(RpPrtStdParticleBatch *prtBatch);

extern RpPrtStdParticleBatch *
_rpPrtStdParticleBatchDtor(RpPrtStdParticleBatch *prtBatch);

extern RpPrtStdParticleBatch *
_rpPrtStdParticleBatchCreate(RpPrtStdParticleClass *pClass, RwInt32 maxPrt);

extern RwBool
_rpPrtStdParticleDestroy(RpPrtStdParticleBatch *prtBatch);

extern RpPrtStdParticleBatch *
_rpPrtStdParticleBatchAddBatch(RpPrtStdParticleBatch *prtBatchHead,
                               RpPrtStdParticleBatch *prtBatch);

/************************************************************************/

extern RpPrtStdEmitter *
_rpPrtStdEmitterCtor(RpPrtStdEmitter *prtEmt, void * data);

extern RpPrtStdEmitter *
_rpPrtStdEmitterDtor(RpPrtStdEmitter *prtEmt);

/************************************************************************/

extern RpPrtStdEmitter *
_rpPrtStdEmitterForAllParticleBatch(RpPrtStdEmitter *prtEmt,
                                    RpPrtStdParticleCallBack callback,
                                    void *data);


extern RpPrtStdParticleBatch *
_rpPrtStdEmitterNewParticleBatch(RpPrtStdEmitter *prtEmt);

extern RpPrtStdParticleBatch *
_rpPrtStdEmitterCreateParticleBatch(RpPrtStdEmitter *prtEmt);

extern RpPrtStdEmitter *
_rpPrtStdEmitterDestroyParticleBatch(RpPrtStdEmitter *prtEmt);

extern RpPrtStdEmitter *
_rpPrtStdEmitterAddParticleBatch(RpPrtStdEmitter *prtEmt,
                                 RpPrtStdParticleBatch *prtBatch);

/************************************************************************/

extern RpPrtStdEmitter *
_rpPrtStdEmitterUpdate(RpAtomic *atomic,
                       RpPrtStdEmitter *prtEmt, void *data);

extern RpPrtStdEmitter *
_rpPrtStdEmitterRender(RpAtomic *atomic,
                       RpPrtStdEmitter *prtEmt, void *data);

/************************************************************************/

extern RpPrtStdEmitter *
_rpPrtStdEmitterStreamRead(RpAtomic * atomic, RwStream *stream);

extern RpPrtStdEmitter *
_rpPrtStdEmitterStreamWrite(RpAtomic * atomic,
                            RpPrtStdEmitter *emt, void *data);

extern RpPrtStdEmitter *
_rpPrtStdEmitterStreamGetSize(RpAtomic * atomic,
                              RpPrtStdEmitter *emt, void *data);

/************************************************************************/

extern RpPrtStdEmitter *
_rpPrtStdEmitterGetPropOffset(RpPrtStdEmitter *emt);

#ifdef    __cplusplus
}
#endif         /* __cplusplus */

#endif /* EMITTER_HDR */

