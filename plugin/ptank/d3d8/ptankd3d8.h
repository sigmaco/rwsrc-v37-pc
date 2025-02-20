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
 * ptankd3d8.h
 *
 * Copyright (C) 2001 Criterion Technologies.
 *
 * Purpose: Generic Billboard implementation
 *
 ****************************************************************************/
#ifndef PTANKD3D8_H
#define PTANKD3D8_H

extern RpPTankCallBacks defaultCB;

#ifdef    __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* Global internal world plugin functions */

extern void 
_rwD3D8AtomicDefaultLightingCallback(void *object);

#ifdef    __cplusplus
}
#endif /* __cplusplus */

#endif /* PTANKD3D8_H */
