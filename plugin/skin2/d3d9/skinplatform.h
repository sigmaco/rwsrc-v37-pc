/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   skinplatform.h                                             -*
 *-                                                                         -*
 *-  Purpose :   D3D9 Patch pipeline extension                               -*
 *-                                                                         -*
 *===========================================================================*/

#ifndef SKINPLATFORM_H
#define SKINPLATFORM_H

/* RWPUBLIC */

/**
 * \defgroup rpskind3d9 D3D9
 * \ingroup rpskin
 *
 * D3D9 skin pipeline extension.
 */

/**
 * \defgroup rpskind3d9features Features
 * \ingroup rpskind3d9
 *
 * D3D9 skin pipeline features.
 */

/**
 * \defgroup rpskind3d9restrictions Restrictions
 * \ingroup rpskind3d9
 *
 * D3D9 skin pipeline restrictions.
 */

/* RWPUBLICEND */

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include "rwcore.h"
#include "rpworld.h"

/* RWPUBLIC */

/*===========================================================================*
 *--- D3D9 Defines -----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- D3D9 Global Types ------------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rpskind3d9
 * \ref RpSkinD3D9Pipeline rendering pipelines available within
 * the \ref RpSkin plugin. Use \ref RpSkinGetD3D9Pipeline to
 * retrieve the \ref RxPipeline's.
 */
enum RpSkinD3D9Pipeline
{
    rpNASKIND3D9PIPELINE              = 0,
    rpSKIND3D9PIPELINEGENERIC         = 1,
    /**<D3D9 generic skin rendering pipeline.                           */
    rpSKIND3D9PIPELINEMATFX           = 2,
    /**<D3D9 material effect skin rendering pipeline.                   */
    rpSKIND3D9PIPELINETOON            = 3,
    /**<D3D9 toon skin rendering pipeline.                              */
    rpSKIND3D9PIPELINEMATFXTOON       = 4,
    /**<D3D9 toon matfx skin rendering pipeline not supported           */
    rpSKIND3D9PIPELINEMAX,
    rpSKIND3D9PIPELINEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RpSkinD3D9Pipeline RpSkinD3D9Pipeline;

/*===========================================================================*
 *--- D3D9 Plugin API Functions ----------------------------------------------*
 *===========================================================================*/
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
/*---------------------------------------------------------------------------*/

extern RxPipeline *
RpSkinGetD3D9Pipeline( RpSkinD3D9Pipeline D3D9Pipeline );

extern RxNodeDefinition *
RxNodeDefinitionGetD3D9SkinAtomicAllInOne(void);

/* Vertex shader pipeline */
extern void
_rxD3D9SkinVertexShaderSetBeginCallBack(RxPipelineNode *node,
                                _rxD3D9VertexShaderBeginCallBack beginCallback);
extern void
_rxD3D9SkinVertexShaderSetLightingCallBack(RxPipelineNode *node,
                                _rxD3D9VertexShaderLightingCallBack lightingCallback);
extern void
_rxD3D9SkinVertexShaderSetGetMaterialShaderCallBack(RxPipelineNode *node,
                                _rxD3D9VertexShaderGetMaterialShaderCallBack getmaterialshaderCallback);
extern void
_rxD3D9SkinVertexShaderSetMeshRenderCallBack(RxPipelineNode *node,
                                _rxD3D9VertexShaderMeshRenderCallBack meshRenderCallback);
extern void
_rxD3D9SkinVertexShaderSetEndCallBack(RxPipelineNode *node,
                                _rxD3D9VertexShaderEndCallBack endCallback);


/*---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif /* __cplusplus */

/* RWPUBLICEND */

#endif /* SKINPLATFORM_H */
