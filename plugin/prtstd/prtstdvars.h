/**********************************************************************
 *
 * File :     prtstdvars.h
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


#ifndef PRTSTDVARS_HDR
#define PRTSTDVARS_HDR

#include <rwcore.h>

#include "rpprtstd.h"

/************************************************************************
 *
 *
 ************************************************************************/

#define ZERO_TOL                (RwReal) 1.0e-5
#define PRTSTDSCRATCHMEMSIZE          128

#define RPPRTSTDPROP(_ptr, _offset) \
    (((char *)(_ptr)) + (_offset))

#define CONSTRPPRTSTDPROP(_ptr, _offset) \
    (((const char *)(_ptr)) + (_offset))

#define RPPRTSTDATOMICPROP(_ptr, _offset) \
    (*(void **)(((RwUInt32)(_ptr)) + (_offset)))

#define CONSTRPPRTSTDATOMICPROP(_ptr, _offset) \
    (*(void *const*)(((RwUInt32)(_ptr)) + (_offset)))



#define RPPRTSTDRGBATOINT32(_rgba, _i) \
    (*(_i) = (((_rgba)->red   << 24) | \
              ((_rgba)->green << 16) | \
              ((_rgba)->blue  <<  8) | \
              ((_rgba)->alpha)))

#define RPPRTSTDINT32T0RGBA(_rgba, _i) \
MACRO_START \
{ \
    ((_rgba)->red   = (RwUInt8)((*(_i) & 0xFF000000) >> 24)); \
    ((_rgba)->green = (RwUInt8)((*(_i) & 0x00FF0000) >> 16)); \
    ((_rgba)->blue  = (RwUInt8)((*(_i) & 0x0000FF00) >>  8)); \
    ((_rgba)->alpha = (RwUInt8)((*(_i) & 0x000000FF))); \
} \
MACRO_STOP

/************************************************************************
 *
 *
 ************************************************************************/

typedef struct RpPrtStdGlobalVars RpPrtStdGlobalVars;

struct RpPrtStdGlobalVars
{
    RwInt32                     engineOffset;  /* Offset into global data */
    RwInt32                     atmOffset;     /* Offset into global data */
    RwInt32                     atmStreamOffset;
    RwInt32                     ver;

    RwChar                      buffer[80];

    RpPrtStdEmitterClass        *emtClass;
    RpPrtStdParticleClass       *prtClass;
    RpPrtStdPropertyTable       *propTab;

    RpPrtStdEClassSetupCallBack         emtClassSetupCallback;
    RpPrtStdPClassSetupCallBack         prtClassSetupCallback;

    _rpPrtStdPropTabCallBack            propTabPostReadCallBack;
    _rpPrtStdPropTabCallBack            propTabDtorCallBack;

    RwBool                      rpPrtStdStreamEmbedded;

    /* Scratch mem. */
    RwChar                      *propTabScratchMem;
    RwInt32                     propTabScratchMemSize;
    RwChar                      *propIDScratchMem;
    RwInt32                     propIDScratchMemSize;

    /* Offset cache */

    RpPrtStdPropertyTable       *emtPropTabCurr;
    RpPrtStdPropertyTable       *prtPropTabCurr;

    RwInt32                     offsetEmtStd;
    RwInt32                     offsetEmtPrtCol;
    RwInt32                     offsetEmtPrtTex;
    RwInt32                     offsetEmtPrtSize;
    RwInt32                     offsetEmtPTank;
    RwInt32                     offsetEmtPrtMtx;
    RwInt32                     offsetEmtPrt2DRot;

    RwInt32                     offsetPrtPos;
    RwInt32                     offsetPrtStd;
    RwInt32                     offsetPrtCol;
    RwInt32                     offsetPrtTex;
    RwInt32                     offsetPrtSize;
    RwInt32                     offsetPrtVel;
    RwInt32                     offsetPrtMtx;
    RwInt32                     offsetPrt2DRot;

    RwInt32                     idxPrtPos;
    RwInt32                     idxPrtStd;
    RwInt32                     idxPrtCol;
    RwInt32                     idxPrtTex;
    RwInt32                     idxPrtSize;
    RwInt32                     idxPrtVel;
    RwInt32                     idxPrtMtx;
    RwInt32                     idxPrt2DRot;
};


extern RpPrtStdGlobalVars rpPrtStdGlobals;

/************************************************************************
 *
 *
 ************************************************************************/

#ifdef    __cplusplus
extern "C"
{
#endif         /* __cplusplus */



#ifdef    __cplusplus
}
#endif         /* __cplusplus */

#endif /* PRTSTDVARS_HDR */

