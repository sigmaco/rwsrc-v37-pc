#ifndef _SKIND3D9GENERIC_H
#define _SKIND3D9GENERIC_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpskin.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/
typedef struct _rwD3D9MatrixTransposed _rwD3D9MatrixTransposed;
struct _rwD3D9MatrixTransposed
{
    RwReal right_x, up_x, at_x, pos_x;
    RwReal right_y, up_y, at_y, pos_y;
    RwReal right_z, up_z, at_z, pos_z;
    RwUInt32 pad1, pad2, pad3, pad4;
};

/*===========================================================================*
 *--- Private Functions -----------------------------------------------------*
 *===========================================================================*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* Skin blending functions */
extern void
_rpD3D9SkinGenericMatrixBlend(RwInt32 numVertices,
                          const RwMatrixWeights *matrixWeightsMap,
                          const RwUInt32 *matrixIndexMap,
                          const _rwD3D9MatrixTransposed *matrixArray,
                          RwUInt8 *vertices,
                          const RwV3d *originalVertices,
                          RwUInt8 *normals,
                          const RwV3d *originalNormals,
                          RwUInt32 stride);

extern void
_rpD3D9SkinGenericMatrixBlend1Weight(RwInt32 numVertices,
                          const RwUInt32 *matrixIndexMap,
                          const _rwD3D9MatrixTransposed *matrixArray,
                          RwUInt8 *vertices,
                          const RwV3d *originalVertices,
                          RwUInt8 *normals,
                          const RwV3d *originalNormals,
                          RwUInt32 stride);

extern void
_rpD3D9SkinGenericMatrixBlend1Matrix(RwInt32 numVertices,
                          const _rwD3D9MatrixTransposed *matrix,
                          RwUInt8 *vertices,
                          const RwV3d *originalVertices,
                          RwUInt8 *normals,
                          const RwV3d *originalNormals,
                          RwUInt32 stride);

extern void
_rpD3D9SkinGenericMatrixBlend2Weights(RwInt32 numVertices,
                          const RwMatrixWeights *matrixWeightsMap,
                          const RwUInt32 *matrixIndexMap,
                          const _rwD3D9MatrixTransposed *matrixArray,
                          RwUInt8 *vertices,
                          const RwV3d *originalVertices,
                          RwUInt8 *normals,
                          const RwV3d *originalNormals,
                          RwUInt32 stride);

/* Transposed functions */
extern void
_rwD3D9MatrixMultiplyTranspose(_rwD3D9MatrixTransposed *dstMat,
                               const RwMatrix *matA,
                               const RwMatrix *matB);

extern void
_rwD3D9MatrixCopyTranspose(_rwD3D9MatrixTransposed *dstMat,
                           const RwMatrix *mat);

extern _rwD3D9MatrixTransposed *
_rwD3D9SkinPrepareMatrixTransposed(RpAtomic *atomic,
                                   RpSkin *skin,
                                   RpHAnimHierarchy *hierarchy);

extern RwMatrix *
_rwD3D9SkinPrepareMatrix(RpAtomic *atomic,
                         RpSkin *skin,
                         RpHAnimHierarchy *hierarchy);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SKIND3D9GENERIC_H */
