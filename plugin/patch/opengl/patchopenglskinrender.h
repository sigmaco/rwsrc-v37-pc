/***********************************************************************
 *
 * Module:  patchopenglskinrender.h
 *
 * Purpose: 
 *
 ***********************************************************************/

#if !defined( PATCHOPENGLSKINRENDER_H )
#define PATCHOPENGLSKINRENDER_H

/* =====================================================================
 *  Includes
 * ===================================================================== */


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

extern RwMatrix *
_rpSkinOpenGLPrepareAtomicMatrix( RpAtomic *atomic,
                                  RpSkin *skin,
                                  RpHAnimHierarchy *hierarchy );

extern RwUInt32
_rpSkinOpenGLSkinPlatformGetMaxWeights( const RpSkin *skin );

#if !defined( NOASM )

extern void
_rpSkinIntelx86MatrixBlend( RwInt32 numVertices,
                            const RwMatrixWeights *matrixWeightsMap,
                            const RwUInt32 *matrixIndexMap,
                            const RwMatrix *matrixArray,
                            RwV3d *vertices,
                            const RwV3d *originalVertices,
                            RwV3d *normals,
                            const RwV3d *originalNormals,
                            RwUInt32 stride );

#if !defined( NOSSEASM )

extern void
_rpSkinIntelSSEMatrixBlend( RwInt32 numVertices,
                            const RwMatrixWeights *matrixWeightsMap,
                            const RwUInt32 *matrixIndexMap,
                            const RwMatrix *matrixArray,
                            RwUInt8 *vertices,
                            const RwV3d *originalVertices,
                            RwUInt8 *normals,
                            const RwV3d *originalNormals,
                            RwUInt32 stride );

extern void
_rpSkinIntelSSEMatrixBlend2Weights( RwInt32 numVertices,
                                    const RwMatrixWeights *matrixWeightsMap,
                                    const RwUInt32 *matrixIndexMap,
                                    const RwMatrix *matrixArray,
                                    RwUInt8 *vertices,
                                    const RwV3d *originalVertices,
                                    RwUInt8 *normals,
                                    const RwV3d *originalNormals,
                                    RwUInt32 stride );

extern void
_rpSkinIntelSSEMatrixBlend1Weight( RwInt32 numVertices,
                                   const RwUInt32 *matrixIndexMap,
                                   const RwMatrix *matrixArray,
                                   RwUInt8 *vertices,
                                   const RwV3d *originalVertices,
                                   RwUInt8 *normals,
                                   const RwV3d *originalNormals,
                                   RwUInt32 stride );

#endif /* !defined( NOSSEASM ) */

#else /* !defined( NOASM ) */

extern void
_rpSkinGenericMatrixBlend( RwInt32 numVertices,
                           const RwMatrixWeights *matrixWeightsMap,
                           const RwUInt32 *matrixIndexMap,
                           const RwMatrix *matrixArray,
                           RwUInt8 *vertices,
                           const RwV3d *originalVertices,
                           RwUInt8 *normals,
                           const RwV3d *originalNormals,
                           RwUInt32 stride );

#endif /* !defined( NOASM ) */

extern RpAtomic*
_rpPatchOpenGLSkinAtomicRenderCB( RpAtomic *atomic );

#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

#endif /* !defined( PATCHOPENGLSKINRENDER_H ) */
