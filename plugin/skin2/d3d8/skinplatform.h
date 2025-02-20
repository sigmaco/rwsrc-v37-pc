/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   skinplatform.h                                             -*
 *-                                                                         -*
 *-  Purpose :   D3D8 Patch pipeline extension                               -*
 *-                                                                         -*
 *===========================================================================*/

#ifndef SKINPLATFORM_H
#define SKINPLATFORM_H

/* RWPUBLIC */

/**
 * \defgroup rpskind3d8 D3D8
 * \ingroup rpskin
 *
 * D3D8 skin pipeline extension.
 */

/**
 * \defgroup rpskind3d8features Features
 * \ingroup rpskind3d8
 *
 * D3D8 skin pipeline features.
 */

/**
 * \defgroup rpskind3d8restrictions Restrictions
 * \ingroup rpskind3d8
 *
 * D3D8 skin pipeline restrictions.
 */

/**
 * \defgroup rpskinbonelimit Bone limit
 * \ingroup rpskind3d8restrictions
 *
 * \par Bone limit
 * The bone limit is 256 as skinning is performed on the CPU.
 *
 */

/* RWPUBLICEND */

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include "rwcore.h"
#include "rpworld.h"

/* RWPUBLIC */

/*===========================================================================*
 *--- D3D8 Defines -----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- D3D8 Global Types ------------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rpskind3d8
 * \ref RpSkinD3D8Pipeline rendering pipelines available within
 * the \ref RpSkin plugin. Use \ref RpSkinGetD3D8Pipeline to
 * retrieve the \ref RxPipeline's.
 */
enum RpSkinD3D8Pipeline
{
    rpNASKIND3D8PIPELINE              = 0,
    rpSKIND3D8PIPELINEGENERIC         = 1,
    /**<D3D8 generic skin rendering pipeline.                           */
    rpSKIND3D8PIPELINEMATFX           = 2,
    /**<D3D8 material effect skin rendering pipeline.                   */
    rpSKIND3D8PIPELINETOON            = 3,
    /**<D3D8 toon skin rendering pipeline.                              */
    rpSKIND3D8PIPELINEMATFXTOON       = 4,
    /**<D3D8 toon matfx skin rendering pipeline not supported           */
    rpSKIND3D8PIPELINEMAX,
    rpSKIND3D8PIPELINEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RpSkinD3D8Pipeline RpSkinD3D8Pipeline;

/*===========================================================================*
 *--- D3D8 Plugin API Functions ----------------------------------------------*
 *===========================================================================*/
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
/*---------------------------------------------------------------------------*/

extern RxPipeline *
RpSkinGetD3D8Pipeline( RpSkinD3D8Pipeline D3D8Pipeline );

/*---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif /* __cplusplus */

/* RWPUBLICEND */

#endif /* SKINPLATFORM_H */
