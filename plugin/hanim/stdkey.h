/******************************************
 *                                        *
 *    RenderWare(TM) Graphics Library     *
 *                                        *
 ******************************************/

/*
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
 * Copyright (c) 1998. Criterion Software Ltd.
 * All Rights Reserved.
 */

/***************************************************************************
 *                                                                         *
 * Module  : rpanim.h                                                      *
 *                                                                         *
 * Purpose : Hierarchical animation                                        *
 *                                                                         *
 **************************************************************************/

#ifndef STDKEY_H
#define STDKEY_H


/****************************************************************************
 Includes
 */

#include <rwcore.h>
#include <rpworld.h>

#include <rphanim.h>
#include <rtquat.h>

#define RpHAnimKeyFrameTransInterpolate(o, a, s, b)          \
MACRO_START                                                     \
{                                                               \
    (o)->x = (((a)->x) + ((s)) * (((b)->x) - ((a)->x)));        \
    (o)->y = (((a)->y) + ((s)) * (((b)->y) - ((a)->y)));        \
    (o)->z = (((a)->z) + ((s)) * (((b)->z) - ((a)->z)));        \
}                                                               \
MACRO_STOP


/*--- Plugin API Functions ---*/

#ifdef    __cplusplus
extern              "C"
{
#endif                          /* __cplusplus */





#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* STDKEY_H */
