/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   pipes.h                                                    -*
 *-                                                                         -*
 *-  Purpose :   Generic lightmap pipelines                                 -*
 *-                                                                         -*
 *===========================================================================*/

#ifndef GENERIC_H
#define GENERIC_H

#include "rwcore.h"
#include "rpworld.h"


/*===========================================================================*
 *--- Global Functions ------------------------------------------------------*
 *===========================================================================*/
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern RwBool
_rpLtMapGenericPipelinesCreate(void);

extern void
_rpLtMapGenericPipelinesDestroy(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GENERIC_H */
