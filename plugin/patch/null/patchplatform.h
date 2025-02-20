/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   patchplatform.h                                            -*
 *-                                                                         -*
 *-  Purpose :   Platform Patch pipeline extension                          -*
 *-                                                                         -*
 *===========================================================================*/

#ifndef PATCHPLATFORM_H
#define PATCHPLATFORM_H

#ifndef RPPATCH_H

/* RWPUBLIC */

/**
 * \defgroup rppatchnull Null
 * \ingroup rppatch
 *
 * Null patch pipeline extension.
 */

/* RWPUBLICEND */

#endif /* !RPPATCH_H */

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include "rwcore.h"
#include "rpworld.h"

#ifndef RPPATCH_H

/* RWPUBLIC */

/*===========================================================================*
 *--- Null Defines ----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Null Global Types -----------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rppatchnull
 * \ref RpPatchNullPipeline dummy pipelines available within the
 * RpPatch plugin. Use \ref RpPatchGetNullPipeline to retrieve
 * the \ref RxPipeline s.
 */
enum RpPatchNullPipeline
{
    rpNAPATCHNULLPIPELINE        = 0,
    rpPATCHNULLPIPELINEGENERIC   = 1, /**<Null generic
                                          patch dummy pipeline.          */
    rpPATCHNULLPIPELINEMATFX     = 2, /**<Null material effect
                                          patch dummy pipeline.          */
    rpPATCHNULLPIPELINESKINNED   = 3, /**<Null skinned
                                          patch dummy pipeline.          */
    rpPATCHNULLPIPELINESKINMATFX = 4, /**<Null skinned material effect
                                          patch dummy pipeline.          */
    rpPATCHNULLPIPELINEMAX,
    rpPATCHNULLPIPELINEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RpPatchNullPipeline RpPatchNullPipeline;

/*===========================================================================*
 *--- PS2 Plugin API Functions ----------------------------------------------*
 *===========================================================================*/
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
/*---------------------------------------------------------------------------*/

extern RxPipeline *
RpPatchGetNullPipeline( RpPatchNullPipeline nullPipeline );

/*---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif /* __cplusplus */

/* RWPUBLICEND */

#endif /* !RPPATCH_H */

#endif /* PATCHPLATFORM_H */
