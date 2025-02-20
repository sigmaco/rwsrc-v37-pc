/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   patchplatform.h                                            -*
 *-                                                                         -*
 *-  Purpose :   D3D9 Patch pipeline extension                               -*
 *-                                                                         -*
 *===========================================================================*/

#ifndef PATCHPLATFORM_H
#define PATCHPLATFORM_H

#ifndef RPPATCH_H

/* RWPUBLIC */

/**
 * \defgroup rppatchd3d9 D3D9
 * \ingroup rppatch
 *
 * D3D9 patch pipeline extension.
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
 *--- D3D9 Defines -----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- D3D9 Global Types ------------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rppatchd3d9
 * \ref RpPatchD3D9Pipeline rendering pipelines available within the
 * \ref rppatch plugin. Use \ref RpPatchGetD3D9Pipeline to retrieve
 * the \ref RxPipeline s.
 */
enum RpPatchD3D9Pipeline
{
    rpPATCHD3D9PIPELINEGENERIC      = 0, /**<D3D9 generic
                                            patch rendering pipeline.        */
    rpPATCHD3D9PIPELINEMATFX        = 1, /**<D3D9 material effect
                                            (single set of texture coordinates)
                                            patch rendering pipeline.        */
    rpPATCHD3D9PIPELINESKINNED      = 2, /**<D3D9 skinned
                                            patch rendering pipeline.        */
    rpPATCHD3D9PIPELINESKINMATFX    = 3, /**<D3D9 skinned material effect
                                            (single set of texture coordinates)
                                            patch rendering pipeline.        */
    rpPATCHD3D9PIPELINEMAX,
    rpPATCHD3D9PIPELINEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RpPatchD3D9Pipeline RpPatchD3D9Pipeline;

/*===========================================================================*
 *--- D3D9 Plugin API Functions ----------------------------------------------*
 *===========================================================================*/
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
/*---------------------------------------------------------------------------*/

extern RxPipeline *
RpPatchGetD3D9Pipeline( RpPatchD3D9Pipeline d3d9Pipeline );

/*---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif /* __cplusplus */

/* RWPUBLICEND */

#endif /* !RPPATCH_H */

#endif /* PATCHPLATFORM_H */
