/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   patchplatform.h                                            -*
 *-                                                                         -*
 *-  Purpose :   D3D8 Patch pipeline extension                               -*
 *-                                                                         -*
 *===========================================================================*/

#ifndef PATCHPLATFORM_H
#define PATCHPLATFORM_H

#ifndef RPPATCH_H

/* RWPUBLIC */

/**
 * \defgroup rppatchd3d8 D3D8
 * \ingroup rppatch
 *
 * D3D8 patch pipeline extension.
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
 *--- D3D8 Defines -----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- D3D8 Global Types ------------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rppatchd3d8
 * \ref RpPatchD3D8Pipeline rendering pipelines available within the
 * \ref rppatch plugin. Use \ref RpPatchGetD3D8Pipeline to retrieve
 * the \ref RxPipeline s.
 */
enum RpPatchD3D8Pipeline
{
    rpPATCHD3D8PIPELINEGENERIC      = 0, /**<D3D8 generic
                                            patch rendering pipeline.        */
    rpPATCHD3D8PIPELINEMATFX        = 1, /**<D3D8 material effect
                                            (single set of texture coordinates)
                                            patch rendering pipeline.        */
    rpPATCHD3D8PIPELINESKINNED      = 2, /**<D3D8 skinned
                                            patch rendering pipeline.        */
    rpPATCHD3D8PIPELINESKINMATFX    = 3, /**<D3D8 skinned material effect
                                            (single set of texture coordinates)
                                            patch rendering pipeline.        */
    rpPATCHD3D8PIPELINEMAX,
    rpPATCHD3D8PIPELINEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RpPatchD3D8Pipeline RpPatchD3D8Pipeline;

/*===========================================================================*
 *--- D3D8 Plugin API Functions ----------------------------------------------*
 *===========================================================================*/
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
/*---------------------------------------------------------------------------*/

extern RxPipeline *
RpPatchGetD3D8Pipeline( RpPatchD3D8Pipeline d3d8Pipeline );

/*---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif /* __cplusplus */

/* RWPUBLICEND */

#endif /* !RPPATCH_H */

#endif /* PATCHPLATFORM_H */
