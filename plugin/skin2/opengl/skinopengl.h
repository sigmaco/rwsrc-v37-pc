/***********************************************************************
 *
 * Module:  skinopengl.h
 *
 * Purpose: 
 *
 ***********************************************************************/

#if !defined( SKINOPENGL_H )
#define SKINOPENGL_H

/* =====================================================================
 *  Includes
 * ===================================================================== */
#include "rpskin.h"


/* =====================================================================
 *  Defines
 * ===================================================================== */
#define rpSKINMAXNUMBEROFMATRICES   256

#define _rpSkinOpenGLPipeline(_idx)   \
    (_rpSkinGlobals.platform.pipelines[(_idx) - 1])


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */
struct SkinPlatformData
{
    RwUInt32    maxWeights;
};


struct SkinGlobalPlatform
{
    RxPipeline  *pipelines[rpSKINOPENGLPIPELINEMAX - 1];

    const void  *lastHierarchyUsed;

    RwUInt32    lastRenderFrame;

    const void  *lastFrame;
};


/* =====================================================================
 *  Extern variables
 * ===================================================================== */
#if !defined(TEAMOPENGL_H)
extern RwOpenGLExtensions   _rwOpenGLExt;
#endif /* !defined(TEAMOPENGL_H) */


/* =====================================================================
 *  Extern function prototypes
 * ===================================================================== */

#if defined( __cplusplus )
extern "C"
{
#endif /* defined( __cplusplus ) */

extern void
_rxOpenGLAllInOneAtomicInstanceVertexArray( RxOpenGLMeshInstanceData *instanceData,
                                            RpAtomic *atomic,
                                            const RpGeometry *geometry,
                                            RpGeometryFlag geomFlags,
                                            RwInt32 numTexCoords,
                                            const RwBool reinstance,
                                            RxVertexIndex *remapTable,
                                            RwUInt8 *baseVertexMem );

#if !defined(TEAMOPENGL_H)
extern RxPipeline *
_rpSkinOpenGLPipelineCreate( const RpSkinType type,
                             RxOpenGLAllInOneRenderCallBack renderCB );
#endif /* !defined(TEAMOPENGL_H) */

extern RwMatrix *
_rpSkinOpenGLPrepareAtomicMatrix( RpAtomic *atomic,
                                  RpSkin *skin,
                                  RpHAnimHierarchy *hierarchy );

extern RwUInt32
_rpSkinOpenGLSkinPlatformGetMaxWeights( const RpSkin *skin );

#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

#endif /* !defined( SKINOPENGL_H ) */
