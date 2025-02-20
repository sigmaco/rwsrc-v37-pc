/***********************************************************************
 *
 * Module:  nodeOpenGLAtomicAllInOne.h
 *
 * Purpose: OpenGL All-In-One world atomic pipelines
 *
 ***********************************************************************/

#if !defined( NODEOPENGLATOMICALLINONE_H )
#define NODEOPENGLATOMICALLINONE_H

/* =====================================================================
 *  Includes
 * ===================================================================== */
#include "openglpipe.h"


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

/* RWPUBLIC */

#if defined( __cplusplus )
extern "C"
{
#endif /* defined( __cplusplus ) */

/* RWPUBLICEND */

extern void
_rxOpenGLAllInOneAtomicInstanceVertexArray( RxOpenGLMeshInstanceData *instanceData,
                                            RpAtomic *atomic,
                                            const RpGeometry *geometry,
                                            RpGeometryFlag geomFlags,
                                            RwInt32 numTexCoords,
                                            const RwBool reinstance,
                                            RxVertexIndex *remapTable,
                                            RwUInt8 *baseVertexMem );

extern void
_rxOpenGLDefaultAllInOneAtomicLightingCB( void *object );

extern RwBool
_rxOpenGLDefaultAtomicAllInOneNode( RxPipelineNode * self,
                                    const RxPipelineNodeParam *params );

/* RWPUBLIC */

extern RxNodeDefinition *
RxNodeDefinitionGetOpenGLAtomicAllInOne( void );

#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

/* RWPUBLICEND */

#endif /* !defined( NODEOPENGLATOMICALLINONE_H ) */
