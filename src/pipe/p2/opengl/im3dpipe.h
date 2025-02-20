/***********************************************************************
 *
 * Module:  im3dpipe.h
 *
 * Purpose: OpenGL immediate mode pipelines
 *
 ***********************************************************************/

#if !defined( IM3DPIPE_H )
#define IM3DPIPE_H

/* =====================================================================
 *  Includes
 * ===================================================================== */
#include "batypes.h"
#include "p2core.h"
#include "baim3d.h"


/* =====================================================================
 *  Defines
 * ===================================================================== */


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */


/* =====================================================================
 *  Extern variables
 * ===================================================================== */


/* =====================================================================
 *  Extern function prototypes
 * ===================================================================== */

#if defined( __cplusplus )
extern "C"
{
#endif /* defined( __cplusplus ) */

extern RwBool
_rwIm3DCreatePlatformTransformPipeline( RxPipeline **globalPipe );

extern RwBool
_rwIm3DCreatePlatformRenderPipelines( rwIm3DRenderPipelines *globalPipes );

extern void
_rwIm3DDestroyPlatformTransformPipeline( RxPipeline **globalPipe );

extern void
_rwIm3DDestroyPlatformRenderPipelines( rwIm3DRenderPipelines *globalPipes );

#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

#endif /* !defined( IM3DPIPE_H ) */
