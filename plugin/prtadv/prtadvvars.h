/**********************************************************************
 *
 * File :     prtadvvars.h
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


#ifndef PRTADVVARS_HDR
#define PRTADVVARS_HDR

#include <rwcore.h>
#include <rpprtstd.h>

#include "rpprtadv.h"

/************************************************************************
 *
 *
 ************************************************************************/


#define RPPRTADVPROP(_ptr, _offset) \
    (((char *)(_ptr)) + (_offset))

#define CONSTRPPRTADVPROP(_ptr, _offset) \
    (((const char *)(_ptr)) + (_offset))

#define RPPRTADVATOMICPROP(_ptr, _offset) \
    (*(void **)(((char *)(_ptr)) + (_offset)))

#define CONSTRPPRTADVATOMICPROP(_ptr, _offset) \
    (*(void **)(((const char *)(_ptr)) + (_offset)))


/************************************************************************
 *
 *
 ************************************************************************/

typedef struct RpPrtAdvGlobalVars RpPrtAdvGlobalVars;

struct RpPrtAdvGlobalVars
{
    RwInt32                     engineOffset;  /* Offset into global data */
    RwInt32                     atmOffset;     /* Offset into global data */

    /* Offset cache */

    RpPrtStdPropertyTable       *emtPropTabCurr;
    RpPrtStdPropertyTable       *prtPropTabCurr;
    RpPrtStdPropertyTable       *emtPropTab2;

    RwInt32                     offsetEmtStd;
    RwInt32                     offsetEmtPrtCol;
    RwInt32                     offsetEmtPrtTex;
    RwInt32                     offsetEmtPrtSize;
    RwInt32                     offsetEmtPTank;
    RwInt32                     offsetEmtPrtOffset;
    RwInt32                     offsetEmtPrtChain;
    RwInt32                     offsetEmtPrtEmitter;
    RwInt32                     offsetEmtStd2;
    RwInt32                     offsetEmtPrtMultiCol;
    RwInt32                     offsetEmtPrtMultiTex;
    RwInt32                     offsetEmtPrtMultiTexStep;
    RwInt32                     offsetEmtPrtMultiSize;

    RwInt32                     offsetPrtPos;
    RwInt32                     offsetPrtStd;
    RwInt32                     offsetPrtCol;
    RwInt32                     offsetPrtTex;
    RwInt32                     offsetPrtSize;
    RwInt32                     offsetPrtVel;
    RwInt32                     offsetPrtTime;
    RwInt32                     offsetPrtChain;
    RwInt32                     offsetPrtEmitter;
    RwInt32                     offsetPrtMultiCol;
    RwInt32                     offsetPrtMultiTex;
    RwInt32                     offsetPrtMultiTexStep;
    RwInt32                     offsetPrtMultiSize;

    RwInt32                     offsetEmtPtList;
    RwInt32                     offsetEmtCircle;
    RwInt32                     offsetEmtSphere;

    RwInt32                     idxPrtPos;
    RwInt32                     idxPrtStd;
    RwInt32                     idxPrtCol;
    RwInt32                     idxPrtTex;
    RwInt32                     idxPrtSize;
    RwInt32                     idxPrtVel;
    RwInt32                     idxPrtTime;
    RwInt32                     idxPrtChain;
    RwInt32                     idxPrtEmitter;
    RwInt32                     idxPrtMultiCol;
    RwInt32                     idxPrtMultiTex;
    RwInt32                     idxPrtMultiSize;
};


extern RpPrtAdvGlobalVars rpPrtAdvGlobals;

/************************************************************************
 *
 *
 ************************************************************************/

#ifdef    __cplusplus
extern "C"
{
#endif         /* __cplusplus */

extern RpPrtStdEmitter    *
_rpPrtAdvEmitterGetPropOffset(RpPrtStdEmitter * emt);


#ifdef    __cplusplus
}
#endif         /* __cplusplus */

#endif /* PRTADVVARS_HDR */

