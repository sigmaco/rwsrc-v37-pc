/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <d3d9.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpskin.h"

#include "skin.h"

#include "x86matbl.h"

#include "skind3d9generic.h"

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/

#define FLOATASINT(f) (*((const RwInt32 *)&(f)))

#define MATRIXPROCESSX(m, o, w)                                     \
    ( ( (m->right_x * o->x) +                                        \
        (m->up_x    * o->y) +                                        \
        (m->at_x    * o->z) +                                        \
        (m->pos_x) ) * w )

#define MATRIXPROCESSY(m, o, w)                                     \
    ( ( (m->right_y * o->x) +                                        \
        (m->up_y    * o->y) +                                        \
        (m->at_y    * o->z) +                                        \
        (m->pos_y) ) * w )

#define MATRIXPROCESSZ(m, o, w)                                     \
    ( ( (m->right_z * o->x) +                                        \
        (m->up_z    * o->y) +                                        \
        (m->at_z    * o->z) +                                        \
        (m->pos_z) ) * w )

#define MATRIXPROCESSXNOW(m, o)                                     \
      ( (m->right_x * o->x) +                                        \
        (m->up_x    * o->y) +                                        \
        (m->at_x    * o->z) +                                        \
        (m->pos_x) )

#define MATRIXPROCESSYNOW(m, o)                                     \
      ( (m->right_y * o->x) +                                        \
        (m->up_y    * o->y) +                                        \
        (m->at_y    * o->z) +                                        \
        (m->pos_y) )

#define MATRIXPROCESSZNOW(m, o)                                     \
      ( (m->right_z * o->x) +                                        \
        (m->up_z    * o->y) +                                        \
        (m->at_z    * o->z) +                                        \
        (m->pos_z) )

#define MATRIXSKIN(v, m, o, w)                                      \
MACRO_START                                                         \
{                                                                   \
    v->x = MATRIXPROCESSX( m, o, w );                                \
    v->y = MATRIXPROCESSY( m, o, w );                                \
    v->z = MATRIXPROCESSZ( m, o, w );                                \
}                                                                   \
MACRO_STOP

#define MATRIXSKINNOW(v, m, o)                                    \
MACRO_START                                                       \
{                                                                 \
    v->x = MATRIXPROCESSXNOW( m, o );                             \
    v->y = MATRIXPROCESSYNOW( m, o );                             \
    v->z = MATRIXPROCESSZNOW( m, o );                             \
}                                                                 \
MACRO_STOP

#define MATRIXPLUSSKIN(v, m, o, w)                                  \
MACRO_START                                                         \
{                                                                   \
    v->x += MATRIXPROCESSX( m, o, w );                               \
    v->y += MATRIXPROCESSY( m, o, w );                               \
    v->z += MATRIXPROCESSZ( m, o, w );                               \
}                                                                   \
MACRO_STOP

#define MATRIXENDPLUSSKIN(fv, v, m, o, w)                           \
MACRO_START                                                         \
{                                                                   \
    fv->x = v->x + MATRIXPROCESSX( m, o, w );                       \
    fv->y = v->y + MATRIXPROCESSY( m, o, w );                       \
    fv->z = v->z + MATRIXPROCESSZ( m, o, w );                       \
}                                                                   \
MACRO_STOP

#define MATRIXNORMALPROCESSX(m, o, w)                               \
    ( ( (m->right_x * o->x) +                                        \
        (m->up_x    * o->y) +                                        \
        (m->at_x    * o->z) ) * w )

#define MATRIXNORMALPROCESSY(m, o, w)                               \
    ( ( (m->right_y * o->x) +                                        \
        (m->up_y    * o->y) +                                        \
        (m->at_y    * o->z) ) * w )

#define MATRIXNORMALPROCESSZ(m, o, w)                               \
    ( ( (m->right_z * o->x) +                                        \
        (m->up_z    * o->y) +                                        \
        (m->at_z    * o->z) ) * w )

#define MATRIXNORMALPROCESSXNOW(m, o)                               \
      ( (m->right_x * o->x) +                                        \
        (m->up_x    * o->y) +                                        \
        (m->at_x    * o->z) )

#define MATRIXNORMALPROCESSYNOW(m, o)                               \
      ( (m->right_y * o->x) +                                        \
        (m->up_y    * o->y) +                                        \
        (m->at_y    * o->z) )

#define MATRIXNORMALPROCESSZNOW(m, o)                               \
      ( (m->right_z * o->x) +                                        \
        (m->up_z    * o->y) +                                        \
        (m->at_z    * o->z) )

#define MATRIXNORMALSKIN(n, m, o, w)                                \
MACRO_START                                                         \
{                                                                   \
    n->x = MATRIXNORMALPROCESSX( m, o, w );                          \
    n->y = MATRIXNORMALPROCESSY( m, o, w );                          \
    n->z = MATRIXNORMALPROCESSZ( m, o, w );                          \
}                                                                   \
MACRO_STOP

#define MATRIXNORMALSKINNOW(n, m, o)                                \
MACRO_START                                                         \
{                                                                   \
    n->x = MATRIXNORMALPROCESSXNOW( m, o );                         \
    n->y = MATRIXNORMALPROCESSYNOW( m, o );                         \
    n->z = MATRIXNORMALPROCESSZNOW( m, o );                         \
}                                                                   \
MACRO_STOP

#define MATRIXNORMALPLUSSKIN(n, m, o, w)                            \
MACRO_START                                                         \
{                                                                   \
    n->x += MATRIXNORMALPROCESSX( m, o, w );                         \
    n->y += MATRIXNORMALPROCESSY( m, o, w );                         \
    n->z += MATRIXNORMALPROCESSZ( m, o, w );                         \
}                                                                   \
MACRO_STOP

#define MATRIXNORMALENDPLUSSKIN(nf, n, m, o, w)                     \
MACRO_START                                                         \
{                                                                   \
    nf->x = n->x + MATRIXNORMALPROCESSX( m, o, w );                 \
    nf->y = n->y + MATRIXNORMALPROCESSY( m, o, w );                 \
    nf->z = n->z + MATRIXNORMALPROCESSZ( m, o, w );                 \
}                                                                   \
MACRO_STOP

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/

static const RwReal One = 1.f;

/*===========================================================================*
 *--- Functions -------------------------------------------------------*
 *===========================================================================*/

/****************************************************************************
 _rpD3D9SkinGenericMatrixBlend

 Purpose: Generic skin matrix blending

 Inputs :   pSkin->totalVertices        number of vertices
            pSkin->pMatrixWeightsMap    array of matrix weights
            pSkin->pMatrixIndexMap      array of matrix indices
            pMatrixArray                array of matrices
            pVertices                   array of vertices to write to
            pOriginalVertices           array of vertices to read from
            pNormals                    array of normals to write to
            pOriginalNormals            array of normals to read from

 Outputs:   Nothing
 *
 */
void
_rpD3D9SkinGenericMatrixBlend(RwInt32 numVertices,
                              const RwMatrixWeights *matrixWeightsMap,
                              const RwUInt32 *matrixIndexMap,
                              const _rwD3D9MatrixTransposed *matrixArray,
                              RwUInt8 *vertices,
                              const RwV3d *originalVertices,
                              RwUInt8 *normals,
                              const RwV3d *originalNormals,
                              RwUInt32 stride)
{
    RwV3d vertex, normal;
    RwInt32 i = 0;

    RWFUNCTION(RWSTRING("_rpD3D9SkinGenericMatrixBlend"));

    for ( i = 0; i < numVertices; ++i )
    {
        const RwMatrixWeights           *matrixWeights = &(matrixWeightsMap[i]);
        const RwUInt32                  matrixIndicesTemp = matrixIndexMap[i];
        const _rwD3D9MatrixTransposed   *matrix = &matrixArray[matrixIndicesTemp & 0xFF];

        /* Hideously slow matrix operations follow... */
        if (FLOATASINT(matrixWeights->w0) >= FLOATASINT(One))
        {
            MATRIXSKINNOW( ((RwV3d *)vertices),
                        matrix,
                        originalVertices );

            vertices += stride;
            ++originalVertices;

            if(NULL != normals)
            {
                MATRIXNORMALSKINNOW( ((RwV3d *)normals),
                                  matrix,
                                  originalNormals );

                normals += stride;
                ++originalNormals;
            }
        }
        else
        {
            MATRIXSKIN( (&vertex),
                        matrix,
                        originalVertices,
                        matrixWeights->w0 );

            if(NULL != originalNormals)
            {
                MATRIXNORMALSKIN( (&normal),
                                  matrix,
                                  originalNormals,
                                  matrixWeights->w0 );
            }

            matrix = &matrixArray[(matrixIndicesTemp >> 8) & 0xFF];

            MATRIXPLUSSKIN( (&vertex),
                            matrix,
                            originalVertices,
                            matrixWeights->w1 );

            if(NULL != originalNormals)
            {
                MATRIXNORMALPLUSSKIN( (&normal),
                                      matrix,
                                      originalNormals,
                                      matrixWeights->w1 );
            }

            if (FLOATASINT(matrixWeights->w2) > 0)
            {
                matrix = &matrixArray[(matrixIndicesTemp >> 16) & 0xFF];

                MATRIXPLUSSKIN( (&vertex),
                                matrix,
                                originalVertices,
                                matrixWeights->w2 );

                if(NULL != originalNormals)
                {
                    MATRIXNORMALPLUSSKIN( (&normal),
                                          matrix,
                                          originalNormals,
                                          matrixWeights->w2 );
                }

                if (FLOATASINT(matrixWeights->w3) > 0)
                {
                    matrix = &matrixArray[(matrixIndicesTemp >> 24) & 0xFF];

                    MATRIXPLUSSKIN( (&vertex),
                                    matrix,
                                    originalVertices,
                                    matrixWeights->w3 );

                    if(NULL != originalNormals)
                    {
                        MATRIXNORMALPLUSSKIN( (&normal),
                                              matrix,
                                              originalNormals,
                                              matrixWeights->w3 );
                    }
                }
            }

            *((RwV3d *)vertices) = vertex;
            vertices += stride;
            ++originalVertices;

            if(NULL != normals)
            {
                *((RwV3d *)normals) = normal;
                normals += stride;
                ++originalNormals;
            }
        }
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpD3D9SkinGenericMatrixBlend1Weight

 Purpose: Generic skin matrix blending

 Inputs :   pSkin->totalVertices        number of vertices
            pSkin->pMatrixIndexMap      array of matrix indices
            pMatrixArray                array of matrices
            pVertices                   array of vertices to write to
            pOriginalVertices           array of vertices to read from
            pNormals                    array of normals to write to
            pOriginalNormals            array of normals to read from

 Outputs:   Nothing
 *
 */
void
_rpD3D9SkinGenericMatrixBlend1Weight(RwInt32 numVertices,
                                     const RwUInt32 *matrixIndexMap,
                                     const _rwD3D9MatrixTransposed *matrixArray,
                                     RwUInt8 *vertices,
                                     const RwV3d *originalVertices,
                                     RwUInt8 *normals,
                                     const RwV3d *originalNormals,
                                     RwUInt32 stride)
{
    RWFUNCTION(RWSTRING("_rpD3D9SkinGenericMatrixBlend1Weight"));

    do
    {
        const RwUInt32                  matrixIndicesTemp = *matrixIndexMap++;
        const _rwD3D9MatrixTransposed   *matrix = &matrixArray[matrixIndicesTemp & 0xFF];

        MATRIXSKINNOW( ((RwV3d *)vertices),
                    matrix,
                    originalVertices );

        vertices += stride;
        ++originalVertices;

        if(NULL != normals)
        {
            MATRIXNORMALSKINNOW( ((RwV3d *)normals),
                              matrix,
                              originalNormals );

            normals += stride;
            ++originalNormals;
        }
    }
    while (--numVertices);

    RWRETURNVOID();
}

/****************************************************************************
 _rpD3D9SkinGenericMatrixBlend1Matrix

 Purpose: Generic skin matrix blending

 Inputs :   pSkin->totalVertices        number of vertices
            pMatrixArray                array of matrices
            pVertices                   array of vertices to write to
            pOriginalVertices           array of vertices to read from
            pNormals                    array of normals to write to
            pOriginalNormals            array of normals to read from

 Outputs:   Nothing
 *
 */
void
_rpD3D9SkinGenericMatrixBlend1Matrix(RwInt32 numVertices,
                                     const _rwD3D9MatrixTransposed *matrix,
                                     RwUInt8 *vertices,
                                     const RwV3d *originalVertices,
                                     RwUInt8 *normals,
                                     const RwV3d *originalNormals,
                                     RwUInt32 stride)
{
    RWFUNCTION(RWSTRING("_rpD3D9SkinGenericMatrixBlend1Matrix"));

    do
    {
        MATRIXSKINNOW( ((RwV3d *)vertices),
                    matrix,
                    originalVertices );

        vertices += stride;
        ++originalVertices;

        if(NULL != normals)
        {
            MATRIXNORMALSKINNOW( ((RwV3d *)normals),
                              matrix,
                              originalNormals );

            normals += stride;
            ++originalNormals;
        }
    }
    while (--numVertices);

    RWRETURNVOID();
}

/****************************************************************************
 _rpD3D9SkinGenericMatrixBlend2Weights

 Purpose: Generic skin matrix blending

 Inputs :   pSkin->totalVertices        number of vertices
            pSkin->pMatrixWeightsMap    array of matrix weights
            pSkin->pMatrixIndexMap      array of matrix indices
            pMatrixArray                array of matrices
            pVertices                   array of vertices to write to
            pOriginalVertices           array of vertices to read from
            pNormals                    array of normals to write to
            pOriginalNormals            array of normals to read from

 Outputs:   Nothing
 *
 */
void
_rpD3D9SkinGenericMatrixBlend2Weights(RwInt32 numVertices,
                                     const RwMatrixWeights *matrixWeightsMap,
                                     const RwUInt32 *matrixIndexMap,
                                     const _rwD3D9MatrixTransposed *matrixArray,
                                     RwUInt8 *vertices,
                                     const RwV3d *originalVertices,
                                     RwUInt8 *normals,
                                     const RwV3d *originalNormals,
                                     RwUInt32 stride)
{
    RwV3d vertex, normal;
    RwInt32 i = 0;

    RWFUNCTION(RWSTRING("_rpD3D9SkinGenericMatrixBlend2Weights"));

    for ( i = 0; i < numVertices; ++i )
    {
        const RwMatrixWeights           *matrixWeights = &(matrixWeightsMap[i]);
        const RwUInt32                  matrixIndicesTemp = matrixIndexMap[i];
        const _rwD3D9MatrixTransposed   *matrix1 = &matrixArray[matrixIndicesTemp & 0xFF];
        const _rwD3D9MatrixTransposed   *matrix2 = &matrixArray[(matrixIndicesTemp >> 8) & 0xFF];

        /* Hideously slow matrix operations follow... */
        if (FLOATASINT(matrixWeights->w0) >= FLOATASINT(One))
        {
            MATRIXSKINNOW( ((RwV3d *)vertices),
                        matrix1,
                        originalVertices );

            vertices += stride;
            ++originalVertices;

            if(NULL != normals)
            {
                MATRIXNORMALSKINNOW( ((RwV3d *)normals),
                                  matrix1,
                                  originalNormals );

                normals += stride;
                ++originalNormals;
            }
        }
        else
        {
            MATRIXSKIN( (&vertex),
                        matrix1,
                        originalVertices,
                        matrixWeights->w0 );

            if(NULL != originalNormals)
            {
                MATRIXNORMALSKIN( (&normal),
                                  matrix1,
                                  originalNormals,
                                  matrixWeights->w0 );
            }

            MATRIXENDPLUSSKIN( ((RwV3d *)vertices),
                               (&vertex),
                               matrix2,
                               originalVertices,
                               matrixWeights->w1 );

            vertices += stride;
            ++originalVertices;

            if(NULL != originalNormals)
            {
                MATRIXNORMALENDPLUSSKIN( ((RwV3d *)normals),
                                         (&normal),
                                         matrix2,
                                         originalNormals,
                                         matrixWeights->w1 );
                normals += stride;
                ++originalNormals;
            }
        }
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rwD3D9MatrixMultiplyTranspose

 On entry   : Dest matrix pointer, two source matrix pointers
 On exit    : Matrix pointer contains transposed result
 */
void
_rwD3D9MatrixMultiplyTranspose(_rwD3D9MatrixTransposed *dstMat,
                               const RwMatrix *matA,
                               const RwMatrix *matB)
{
    RWFUNCTION(RWSTRING("_rwD3D9MatrixMultiplyTranspose"));
    RWASSERT(dstMat);
    RWASSERT(matA);
    RWASSERT(matB);

    /* Multiply out right */
    dstMat->right_x =
        (matA->right.x * matB->right.x) +
        (matA->right.y * matB->up.x) + (matA->right.z * matB->at.x);
    dstMat->right_y =
        (matA->right.x * matB->right.y) +
        (matA->right.y * matB->up.y) + (matA->right.z * matB->at.y);
    dstMat->right_z =
        (matA->right.x * matB->right.z) +
        (matA->right.y * matB->up.z) + (matA->right.z * matB->at.z);

    /* Then up */
    dstMat->up_x =
        (matA->up.x * matB->right.x) +
        (matA->up.y * matB->up.x) + (matA->up.z * matB->at.x);
    dstMat->up_y =
        (matA->up.x * matB->right.y) +
        (matA->up.y * matB->up.y) + (matA->up.z * matB->at.y);
    dstMat->up_z =
        (matA->up.x * matB->right.z) +
        (matA->up.y * matB->up.z) + (matA->up.z * matB->at.z);

    /* Then at */
    dstMat->at_x =
        (matA->at.x * matB->right.x) +
        (matA->at.y * matB->up.x) + (matA->at.z * matB->at.x);
    dstMat->at_y =
        (matA->at.x * matB->right.y) +
        (matA->at.y * matB->up.y) + (matA->at.z * matB->at.y);
    dstMat->at_z =
        (matA->at.x * matB->right.z) +
        (matA->at.y * matB->up.z) + (matA->at.z * matB->at.z);

    /* Then pos - this is different because there is an extra add
     * (implicit 1 (one) in bottom right of matrix)
     */
    dstMat->pos_x =
        (matA->pos.x * matB->right.x) +
        (matA->pos.y * matB->up.x) +
        (matA->pos.z * matB->at.x) + ( /* (1*) */ matB->pos.x);
    dstMat->pos_y =
        (matA->pos.x * matB->right.y) +
        (matA->pos.y * matB->up.y) +
        (matA->pos.z * matB->at.y) + ( /* (1*) */ matB->pos.y);
    dstMat->pos_z =
        (matA->pos.x * matB->right.z) +
        (matA->pos.y * matB->up.z) +
        (matA->pos.z * matB->at.z) + ( /* (1*) */ matB->pos.z);

    /* And that's all folks */
    RWRETURNVOID();
}

/****************************************************************************
 _rwD3D9MatrixCopyTranspose
 */
void
_rwD3D9MatrixCopyTranspose(_rwD3D9MatrixTransposed *dstMat,
                           const RwMatrix *mat)
{
    RWFUNCTION(RWSTRING("_rwD3D9MatrixCopyTranspose"));
    RWASSERT(dstMat);
    RWASSERT(mat);

    dstMat->right_x = mat->right.x;
    dstMat->right_y = mat->right.y;
    dstMat->right_z = mat->right.z;

    dstMat->up_x = mat->up.x;
    dstMat->up_y = mat->up.y;
    dstMat->up_z = mat->up.z;

    dstMat->at_x = mat->at.x;
    dstMat->at_y = mat->at.y;
    dstMat->at_z = mat->at.z;

    dstMat->pos_x = mat->pos.x;
    dstMat->pos_y = mat->pos.y;
    dstMat->pos_z = mat->pos.z;

    RWRETURNVOID();
}

/****************************************************************************
 D3D9SkinPrepareAtomicMatrixTransposed

 Purpose: Setup matrices for skinning.

 On entry: atomic    - Pointer to RpAtomic containing geometry data
           skin      - Pointer to RpSkin containing bones and weights data
           hierarchy - Pointer to RpHAnimHierarchy
 On exit : Pointer to RwMatrix, which is the base of the array of skinning matrices
           or NULL if there is no skinning data.
*/
static void
D3D9SkinPrepareAtomicMatrixTransposed(RpAtomic *atomic,
                                      RpSkin *skin,
                                      RpHAnimHierarchy *hierarchy)
{
    _rwD3D9MatrixTransposed  *matrixArray = NULL;

    RWFUNCTION(RWSTRING("D3D9SkinPrepareAtomicMatrixTransposed"));
    RWASSERT(atomic);

    if(NULL != hierarchy)
    {
        RwInt32         i;
        const RwMatrix  *skinToBone;

        RwMatrix    inverseAtomicLTM;
        RwMatrix    temmatrix;

        matrixArray = (_rwD3D9MatrixTransposed *)_rpSkinGlobals.matrixCache.aligned;

        /* Get the bone information. */
        skinToBone = RpSkinGetSkinToBoneMatrices(skin);
        RWASSERT(NULL != skinToBone);

        if(hierarchy->flags & rpHANIMHIERARCHYNOMATRICES)
        {
            RwMatrixInvert(&inverseAtomicLTM,
                           RwFrameGetLTM(RpAtomicGetFrame(atomic)));

            for( i = 0; i < hierarchy->numNodes; ++i )
            {
                const RwMatrix *ltm =
                                RwFrameGetLTM(hierarchy->pNodeInfo[i].pFrame);
                RWASSERT(NULL != ltm);

                RwMatrixMultiply( &temmatrix,
                                  &skinToBone[i],
                                  ltm );

                _rwD3D9MatrixMultiplyTranspose(&matrixArray[i],
                                               &temmatrix,
                                               &inverseAtomicLTM );
            }
        }
        else if(hierarchy->flags & rpHANIMHIERARCHYLOCALSPACEMATRICES)
        {
            for( i = 0; i < hierarchy->numNodes; ++i )
            {
                _rwD3D9MatrixMultiplyTranspose( &matrixArray[i],
                                                &skinToBone[i],
                                                &(hierarchy->pMatrixArray[i]) );
            }
        }
        else
        {
            RwMatrixInvert(&inverseAtomicLTM,
                           RwFrameGetLTM(RpAtomicGetFrame(atomic)));

            for( i = 0; i < hierarchy->numNodes; ++i)
            {
                RwMatrixMultiply( &temmatrix,
                                  &skinToBone[i],
                                  &(hierarchy->pMatrixArray[i]) );

                _rwD3D9MatrixMultiplyTranspose(&matrixArray[i],
                                               &temmatrix,
                                               &inverseAtomicLTM );
            }
        }
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rwD3D9SkinPrepareMatrixTransposed
  Inputs :
 Outputs:   return the aligned matrix cache
 */
_rwD3D9MatrixTransposed *
_rwD3D9SkinPrepareMatrixTransposed(RpAtomic *atomic,
                                   RpSkin *skin,
                                   RpHAnimHierarchy *hierarchy)
{
    RWFUNCTION(RWSTRING("_rwD3D9SkinPrepareMatrixTransposed"));

    if (_rpSkinGlobals.platform.lastHierarchyUsed != hierarchy ||
        _rpSkinGlobals.platform.lastRenderFrame != RWSRCGLOBAL(renderFrame) ||
        _rpSkinGlobals.platform.lastFrame != RpAtomicGetFrame(atomic))
    {
        D3D9SkinPrepareAtomicMatrixTransposed(atomic, skin, hierarchy);

        _rpSkinGlobals.platform.lastHierarchyUsed = (const RpHAnimHierarchy *)hierarchy;
        _rpSkinGlobals.platform.lastRenderFrame = RWSRCGLOBAL(renderFrame);
        _rpSkinGlobals.platform.lastFrame = RpAtomicGetFrame(atomic);
    }

    RWRETURN((_rwD3D9MatrixTransposed *)_rpSkinGlobals.matrixCache.aligned);
}

/****************************************************************************
 _rwD3D9SkinPrepareMatrix
  Inputs :
 Outputs:   return the aligned matrix cache
 */
RwMatrix *
_rwD3D9SkinPrepareMatrix(RpAtomic *atomic,
                         RpSkin *skin,
                         RpHAnimHierarchy *hierarchy)
{
    RWFUNCTION(RWSTRING("_rwD3D9SkinPrepareMatrix"));

    if (_rpSkinGlobals.platform.lastHierarchyUsed != hierarchy ||
        _rpSkinGlobals.platform.lastRenderFrame != RWSRCGLOBAL(renderFrame) ||
        _rpSkinGlobals.platform.lastFrame != RpAtomicGetFrame(atomic))
    {
        RwInt32  i;
        RwMatrix *matrix;

        _rpSkinPrepareAtomicMatrix(atomic, skin, hierarchy);

        matrix = _rpSkinGlobals.matrixCache.aligned;
        for( i = 0; i < hierarchy->numNodes; i++)
        {
            matrix->flags=0;
            matrix->pad1=0;
            matrix->pad2=0;
            matrix->pad3=0;
            ++matrix;
        }

        _rpSkinGlobals.platform.lastHierarchyUsed = (const RpHAnimHierarchy *)hierarchy;
        _rpSkinGlobals.platform.lastRenderFrame = RWSRCGLOBAL(renderFrame);
        _rpSkinGlobals.platform.lastFrame = RpAtomicGetFrame(atomic);
    }

    RWRETURN(_rpSkinGlobals.matrixCache.aligned);
}
