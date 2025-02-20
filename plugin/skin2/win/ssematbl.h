/****************************************************************************
 *                                                                          *
 *  Module  :   ssematbl.h                                                  *
 *                                                                          *
 *  Purpose :   Intel sse matrix-blending functions                         *
 *                                                                          *
 ****************************************************************************/

#ifndef SSEMATBL_H
#define SSEMATBL_H

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

extern void _rpSkinIntelSSEMatrixBlend( RwInt32 numVertices,
                                        const RwMatrixWeights *matrixWeightsMap,
                                        const RwUInt32 *matrixIndexMap,
                                        const RwMatrix *matrixArray,
                                        RwUInt8 *vertices,
                                        const RwV3d *originalVertices,
                                        RwUInt8 *normals,
                                        const RwV3d *originalNormals,
                                        RwUInt32 stride);

extern void _rpSkinIntelSSEMatrixBlend2Weights( RwInt32 numVertices,
                                        const RwMatrixWeights *matrixWeightsMap,
                                        const RwUInt32 *matrixIndexMap,
                                        const RwMatrix *matrixArray,
                                        RwUInt8 *vertices,
                                        const RwV3d *originalVertices,
                                        RwUInt8 *normals,
                                        const RwV3d *originalNormals,
                                        RwUInt32 stride);

extern void _rpSkinIntelSSEMatrixBlend1Matrix(RwInt32 numVertices,
                                              const RwMatrix *matrix,
                                              RwUInt8 *vertices,
                                              const RwV3d *originalVertices,
                                              RwUInt8 *normals,
                                              const RwV3d *originalNormals,
                                              RwUInt32 stride);

extern void _rpSkinIntelSSEMatrixBlend1Weight(RwInt32 numVertices,
                                              const RwUInt32 *matrixIndexMap,
                                              const RwMatrix *matrixArray,
                                              RwUInt8 *vertices,
                                              const RwV3d *originalVertices,
                                              RwUInt8 *normals,
                                              const RwV3d *originalNormals,
                                              RwUInt32 stride);
#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#endif /* SSEMATBL_H */
