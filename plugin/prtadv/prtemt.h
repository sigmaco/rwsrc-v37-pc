/**********************************************************************
 *
 * File :     prtemt.h
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

#ifndef PRTEMT_HDR
#define PRTEMT_HDR

#include <rwcore.h>
#include <rpprtstd.h>

#include "rpprtadv.h"



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

#ifdef    __cplusplus
extern "C"
{
#endif         /* __cplusplus */

/************************************************************************/


extern RpPrtStdPropertyTable *
_rpPrtStdEmitterStandardPropertyTableCreate( void );

extern RpPrtStdPropertyTable *
_rpPrtStdParticleStandardPropertyTableCreate( void );

/* From prtstd/emitter.h  */
extern RpPrtStdEmitter *
_rpPrtStdEmitterUpdate(RpAtomic *atomic,
                       RpPrtStdEmitter *prtEmt, void *data);

extern RpPrtStdEmitter *
_rpPrtStdEmitterGetPropOffset(RpPrtStdEmitter *emt);


#ifdef    __cplusplus
}
#endif         /* __cplusplus */


#endif /* PRTEMT_HDR */


