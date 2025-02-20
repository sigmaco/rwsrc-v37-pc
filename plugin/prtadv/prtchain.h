/**********************************************************************
 *
 * File :     prtchain.h
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rpplugin.h>
#include <rpdbgerr.h>

#include "rpprtstd.h"
#include "rpprtadv.h"

#include "prtadvvars.h"


/************************************************************************
 *
 *
 *
 ************************************************************************/


/************************************************************************
 *
 *
 *
 ************************************************************************/


/************************************************************************
 *
 *
 *
 ************************************************************************/


/************************************************************************
 *
 *
 *
 ************************************************************************/


/************************************************************************
 *
 *
 *
 ************************************************************************/

extern RpPrtStdParticleBatch *
RpPrtAdvParticleChainUpdateCB(RpPrtStdEmitter *emt,
                              RpPrtStdParticleBatch *prtBatch,
                              void *data);

extern RpPrtStdEmitter *
RpPrtAdvEmitterChainEmitCB(RpAtomic * __RWUNUSED__ atomic,
                           RpPrtStdEmitter *emt,
                           void *  __RWUNUSED__ data);

extern RpPrtStdEmitter *
RpPrtAdvEmitterChainCreateCB(RpAtomic * __RWUNUSED__ atomic,
                                RpPrtStdEmitter *emt,
                                void * __RWUNUSED__ data);

/************************************************************************
 *
 *
 *
 ************************************************************************/

extern RpPrtStdEmitter *
RpPrtAdvEmitterChainBeginUpdateCB(RpAtomic *atomic,
                                  RpPrtStdEmitter *emt, void *data);

extern RpPrtStdEmitter *
RpPrtAdvEmitterChainEndUpdateCB(RpAtomic * __RWUNUSED__ atomic,
                                   RpPrtStdEmitter *emt,
                                   void *__RWUNUSED__ data);


/************************************************************************
 *
 *
 *
 ************************************************************************/

extern RpPrtStdParticleClass *
RpPrtAdvPClassChainCreate( RwInt32 dataFlag );

extern RpPrtStdPropertyTable *
_rpPrtAdvEmitterChainPropTabCreate( void );

extern  RpPrtStdPropertyTable *
_rpPrtAdvParticleChainPropTabCreate( void );



/************************************************************************
 *
 *
 *
 ************************************************************************/


