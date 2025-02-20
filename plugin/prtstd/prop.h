/**********************************************************************
 *
 * File :     prtprop.c
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

#ifndef PROP_HDR
#define PROP_HDR

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

#ifdef    __cplusplus
extern "C"
{
#endif         /* __cplusplus */


extern RpPrtStdPropertyTable *
_rpPrtStdPropTabAdd(RpPrtStdPropertyTable *propTabHead,
                    RpPrtStdPropertyTable *propTab);

extern RwBool
_rpPrtStdPropTabCompareProp(RpPrtStdPropertyTable *propTab,
                            RwInt32 numProp, RwInt32 *propID, RwInt32 *propStride);

extern RpPrtStdPropertyTable *
_rpPrtStdPropTabCtor(RpPrtStdPropertyTable *prtPropTab);

extern RpPrtStdPropertyTable *
_rpPrtStdPropTabDtor(RpPrtStdPropertyTable *prtPropTab);


#ifdef    __cplusplus
}
#endif         /* __cplusplus */

#endif /* PROP_HDR */


