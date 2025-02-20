/****************************************************************************
 *                                                                          *
 *  Module  :   x86matbl.h                                                  *
 *                                                                          *
 *  Purpose :   Intel x86 matrix-blending functions                         *
 *                                                                          *
 ****************************************************************************/

#ifndef X86MATBL_H
#define X86MATBL_H

/****************************************************************************
 Includes
 */

/****************************************************************************
 Defines
 */

/****************************************************************************
 Global Types
 */


/****************************************************************************
 Function prototypes
 */

#ifdef    __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


extern RwMatrix *
_rpSkinPrepareAtomicMatrix(RpAtomic *atomic, RpSkin *skin, RpHAnimHierarchy *hierarchy);

extern RwMatrix *
_rpSkinPrepareAtomicMatrixWorldSpace(RpAtomic *atomic, RpSkin *skin, RpHAnimHierarchy *hierarchy);


#if !defined( NOASM )

#if defined( RWDEBUG )

extern void _rpSkinIntelx86ConfirmConstants(void);

#endif /* defined( RWDEBUG ) */


extern void _rpSkinIntelx86MatrixBlend( RwInt32 numVertices,
                                        const RwMatrixWeights *matrixWeightsMap,
                                        const RwUInt32 *matrixIndexMap,
                                        const RwMatrix *matrixArray,
                                        RwV3d *vertices,
                                        const RwV3d *originalVertices,
                                        RwV3d *normals,
                                        const RwV3d *originalNormals,
                                        RwUInt32 stride);

#else /* !defined( NOASM ) */

extern void _rpSkinGenericMatrixBlend(RwInt32 numVertices,
                                      const RwMatrixWeights *matrixWeightsMap,
                                      const RwUInt32 *matrixIndexMap,
                                      const RwMatrix *matrixArray,
                                      RwUInt8 *vertices,
                                      const RwV3d *originalVertices,
                                      RwUInt8 *normals,
                                      const RwV3d *originalNormals,
                                      RwUInt32 stride);

#endif /* !defined( NOASM ) */

#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#endif /* X86MATBL_H */
