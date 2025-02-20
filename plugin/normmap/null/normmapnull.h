/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   normmapnull.h                                              -*
 *-                                                                         -*
 *-  Purpose :   Normal Maps Xbox plugin internal API                       -*
 *-                                                                         -*
 *===========================================================================*/

#ifndef NORMMAPNULL_H
#define NORMMAPNULL_H

#include "rwcore.h"
#include "rpworld.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Variables -----------------------------------------------------*
 *===========================================================================*/

 /*===========================================================================*
 *--- Private Functions -----------------------------------------------------*
 *===========================================================================*/

#ifdef    __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

extern RwBool
NormalMapCreatePipelines(void);

extern RxPipeline *
NormalMapCreateAtomicPipeline(void);

extern RxPipeline *
NormalMapCreateWorldSectorPipeline(void);

extern void
NormalMapDestroyPipelines(void);

#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#endif /* NORMMAPNULL_H */
