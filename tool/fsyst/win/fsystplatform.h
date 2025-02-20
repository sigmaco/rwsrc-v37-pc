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
 * fsystplatform.h
 *
 * Copyright (C) 2001 Criterion Technologies.
 *
 * Purpose: Windows specific include for fsyst
 *
 ****************************************************************************/
#ifndef FSYSTPLATFORM_H
#define FSYSTPLATFORM_H

/* RWPUBLIC */
#include "rtfswin.h"

extern CRITICAL_SECTION section;

#define RTFS_INITIALIZE_INTERRUPT_HANDLING InitializeCriticalSection(&section)
#define RTFS_DISABLE_INTERRUPT EnterCriticalSection(&section)
#define RTFS_ENABLE_INTERRUPT  LeaveCriticalSection(&section)
#define RTFS_SHUTDOWN_INTERRUPT_HANDLING DeleteCriticalSection(&section)

/* RWPUBLICEND */

#endif /* FSYSTPLATFORM_H */
