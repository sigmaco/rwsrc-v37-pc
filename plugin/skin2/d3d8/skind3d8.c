/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <d3d8.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpskin.h"

#include "skin.h"

#include "x86matbl.h"

#if !defined(NOSSEASM)
#include "ssematbl.h"
#endif

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/
#define NUMPRIMTYPES    7
static const D3DPRIMITIVETYPE _RwD3D8PrimConvTable[NUMPRIMTYPES] =
{
    (D3DPRIMITIVETYPE)0,    /* rwPRIMTYPENAPRIMTYPE */
    D3DPT_LINELIST,         /* rwPRIMTYPELINELIST */
    D3DPT_LINESTRIP,        /* rwPRIMTYPEPOLYLINE */
    D3DPT_TRIANGLELIST,     /* rwPRIMTYPETRILIST */
    D3DPT_TRIANGLESTRIP,    /* rwPRIMTYPETRISTRIP */
    D3DPT_TRIANGLEFAN,      /* rwPRIMTYPETRIFAN */
    D3DPT_POINTLIST
};

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
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
SkinGlobals _rpSkinGlobals =
{
    0,
    0,
    0,
    { (RwMatrix *)NULL, NULL },
    0,
    (RwFreeList *)NULL,
    { 0, 0 },
    {                           /* SkinGlobalPlatform  platform    */
        {
            (RxPipeline *)NULL, /*  rpSKIND3D8PIPELINEGENERIC */
            (RxPipeline *)NULL, /*  rpSKIND3D8PIPELINEMATFX   */
            (RxPipeline *)NULL, /*  rpSKIND3D8PIPELINETOON   */
            (RxPipeline *)NULL, /*  rpSKIND3D8PIPELINEMATFXTOON */
        },
        NULL,                   /*  Last hierarchy used */
        0,                      /*  Last render frame */
        NULL,                   /*  Last frame used */
        FALSE,                  /*  Hardware T&L */
        FALSE                   /*  Hardware VS */
    },
    (SkinSplitData *) NULL
};

static const RwReal One = 1.f;
static const RwReal Zero = 0.f;

/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/

/****************************************************************************
 _rwD3D8MatrixMultiplyTranspose

 On entry   : Dest matrix pointer, two source matrix pointers
 On exit    : Matrix pointer contains transposed result
 */
void
_rwD3D8MatrixMultiplyTranspose(_rwD3D8MatrixTransposed *dstMat,
               const RwMatrix *matA, const RwMatrix *matB)
{
    RWFUNCTION(RWSTRING("_rwD3D8MatrixMultiplyTranspose"));
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
 _rwD3D8MatrixCopyTranspose
 */
void
_rwD3D8MatrixCopyTranspose(_rwD3D8MatrixTransposed *dstMat,
               const RwMatrix *mat)
{
    RWFUNCTION(RWSTRING("_rwD3D8MatrixCopyTranspose"));
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
 D3D8SkinPrepareAtomicMatrixTransposed

 Purpose: Setup matrices for skinning.

 On entry: atomic    - Pointer to RpAtomic containing geometry data
           skin      - Pointer to RpSkin containing bones and weights data
           hierarchy - Pointer to RpHAnimHierarchy
 On exit : Pointer to RwMatrix, which is the base of the array of skinning matrices
           or NULL if there is no skinning data.
*/
static void
D3D8SkinPrepareAtomicMatrixTransposed(RpAtomic *atomic, RpSkin *skin, RpHAnimHierarchy *hierarchy)
{
    _rwD3D8MatrixTransposed  *matrixArray = NULL;

    RWFUNCTION(RWSTRING("D3D8SkinPrepareAtomicMatrixTransposed"));
    RWASSERT(atomic);

    if(NULL != hierarchy)
    {
        RwInt32         i;
        const RwMatrix  *skinToBone;

        RwMatrix    inverseAtomicLTM;
        RwMatrix    temmatrix;

        matrixArray = (_rwD3D8MatrixTransposed *)_rpSkinGlobals.matrixCache.aligned;

        /* Get the bone information. */
        skinToBone = RpSkinGetSkinToBoneMatrices(skin);
        RWASSERT(NULL != skinToBone);

        if(hierarchy->flags & rpHANIMHIERARCHYNOMATRICES)
        {
            RwMatrixInvert(&inverseAtomicLTM,
                           RwFrameGetLTM(RpAtomicGetFrame(atomic)));

            for( i = 0; i < hierarchy->numNodes; i++ )
            {
                const RwMatrix *ltm =
                                RwFrameGetLTM(hierarchy->pNodeInfo[i].pFrame);
                RWASSERT(NULL != ltm);

                RwMatrixMultiply( &temmatrix,
                                  &skinToBone[i],
                                  ltm );

                _rwD3D8MatrixMultiplyTranspose(&matrixArray[i],
                                               &temmatrix,
                                               &inverseAtomicLTM );
            }
        }
        else if(hierarchy->flags & rpHANIMHIERARCHYLOCALSPACEMATRICES)
        {
            for( i = 0; i < hierarchy->numNodes; i++ )
            {
                _rwD3D8MatrixMultiplyTranspose( &matrixArray[i],
                                                &skinToBone[i],
                                                &(hierarchy->pMatrixArray[i]) );
            }
        }
        else
        {
            RwMatrixInvert(&inverseAtomicLTM,
                           RwFrameGetLTM(RpAtomicGetFrame(atomic)));

            for( i = 0; i < hierarchy->numNodes; i++)
            {
                RwMatrixMultiply( &temmatrix,
                                  &skinToBone[i],
                                  &(hierarchy->pMatrixArray[i]) );

                _rwD3D8MatrixMultiplyTranspose(&matrixArray[i],
                                               &temmatrix,
                                               &inverseAtomicLTM );
            }
        }
    }

    RWRETURNVOID();
}

/****************************************************************************
 D3D8SkinGenericMatrixBlend

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
static void
D3D8SkinGenericMatrixBlend(RwInt32 numVertices,
                          const RwMatrixWeights *matrixWeightsMap,
                          const RwUInt32 *matrixIndexMap,
                          const _rwD3D8MatrixTransposed *matrixArray,
                          RwUInt8 *vertices,
                          const RwV3d *originalVertices,
                          RwUInt8 *normals,
                          const RwV3d *originalNormals,
                          RwUInt32 stride)
{
    RwV3d vertex, normal;
    RwInt32 i = 0;

    RWFUNCTION(RWSTRING("D3D8SkinGenericMatrixBlend"));

    for ( i = 0; i < numVertices; i++ )
    {
        const RwMatrixWeights           *matrixWeights = &(matrixWeightsMap[i]);
        const RwUInt32                  matrixIndicesTemp = matrixIndexMap[i];
        const _rwD3D8MatrixTransposed   *matrix = &matrixArray[matrixIndicesTemp & 0xFF];

        /* Hideously slow matrix operations follow... */
        if (FLOATASINT(matrixWeights->w0) >= FLOATASINT(One))
        {
            MATRIXSKINNOW( ((RwV3d *)vertices),
                        matrix,
                        originalVertices );

            vertices += stride;
            originalVertices++;

            if(NULL != normals)
            {
                MATRIXNORMALSKINNOW( ((RwV3d *)normals),
                                  matrix,
                                  originalNormals );

                normals += stride;
                originalNormals++;
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
            originalVertices++;

            if(NULL != normals)
            {
                *((RwV3d *)normals) = normal;
                normals += stride;
                originalNormals++;
            }
        }
    }

    RWRETURNVOID();
}

/****************************************************************************
 D3D8SkinGenericMatrixBlend1Weight

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
static void
D3D8SkinGenericMatrixBlend1Weight(RwInt32 numVertices,
                          const RwUInt32 *matrixIndexMap,
                          const _rwD3D8MatrixTransposed *matrixArray,
                          RwUInt8 *vertices,
                          const RwV3d *originalVertices,
                          RwUInt8 *normals,
                          const RwV3d *originalNormals,
                          RwUInt32 stride)
{
    RWFUNCTION(RWSTRING("D3D8SkinGenericMatrixBlend1Weight"));

    do
    {
        const RwUInt32                  matrixIndicesTemp = *matrixIndexMap++;
        const _rwD3D8MatrixTransposed   *matrix = &matrixArray[matrixIndicesTemp & 0xFF];

        MATRIXSKINNOW( ((RwV3d *)vertices),
                    matrix,
                    originalVertices );

        vertices += stride;
        originalVertices++;

        if(NULL != normals)
        {
            MATRIXNORMALSKINNOW( ((RwV3d *)normals),
                              matrix,
                              originalNormals );

            normals += stride;
            originalNormals++;
        }
    }
    while (--numVertices);

    RWRETURNVOID();
}

/****************************************************************************
 D3D8SkinGenericMatrixBlend1Matrix

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
static void
D3D8SkinGenericMatrixBlend1Matrix(RwInt32 numVertices,
                          const _rwD3D8MatrixTransposed *matrix,
                          RwUInt8 *vertices,
                          const RwV3d *originalVertices,
                          RwUInt8 *normals,
                          const RwV3d *originalNormals,
                          RwUInt32 stride)
{
    RWFUNCTION(RWSTRING("D3D8SkinGenericMatrixBlend1Matrix"));

    do
    {
        MATRIXSKINNOW( ((RwV3d *)vertices),
                    matrix,
                    originalVertices );

        vertices += stride;
        originalVertices++;

        if(NULL != normals)
        {
            MATRIXNORMALSKINNOW( ((RwV3d *)normals),
                              matrix,
                              originalNormals );

            normals += stride;
            originalNormals++;
        }
    }
    while (--numVertices);

    RWRETURNVOID();
}

/****************************************************************************
 D3D8SkinGenericMatrixBlend2Weights

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
static void
D3D8SkinGenericMatrixBlend2Weights(RwInt32 numVertices,
                          const RwMatrixWeights *matrixWeightsMap,
                          const RwUInt32 *matrixIndexMap,
                          const _rwD3D8MatrixTransposed *matrixArray,
                          RwUInt8 *vertices,
                          const RwV3d *originalVertices,
                          RwUInt8 *normals,
                          const RwV3d *originalNormals,
                          RwUInt32 stride)
{
    RwV3d vertex, normal;
    RwInt32 i = 0;

    RWFUNCTION(RWSTRING("D3D8SkinGenericMatrixBlend2Weights"));

    for ( i = 0; i < numVertices; i++ )
    {
        const RwMatrixWeights           *matrixWeights = &(matrixWeightsMap[i]);
        const RwUInt32                  matrixIndicesTemp = matrixIndexMap[i];
        const _rwD3D8MatrixTransposed   *matrix1 = &matrixArray[matrixIndicesTemp & 0xFF];
        const _rwD3D8MatrixTransposed   *matrix2 = &matrixArray[(matrixIndicesTemp >> 8) & 0xFF];

        /* Hideously slow matrix operations follow... */
        if (FLOATASINT(matrixWeights->w0) >= FLOATASINT(One))
        {
            MATRIXSKINNOW( ((RwV3d *)vertices),
                        matrix1,
                        originalVertices );

            vertices += stride;
            originalVertices++;

            if(NULL != normals)
            {
                MATRIXNORMALSKINNOW( ((RwV3d *)normals),
                                  matrix1,
                                  originalNormals );

                normals += stride;
                originalNormals++;
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
            originalVertices++;

            if(NULL != originalNormals)
            {
                MATRIXNORMALENDPLUSSKIN( ((RwV3d *)normals),
                                         (&normal),
                                         matrix2,
                                         originalNormals,
                                         matrixWeights->w1 );
                normals += stride;
                originalNormals++;
            }
        }
    }

    RWRETURNVOID();
}

/*****************************************************************************
 D3D8SkinDestroyVertexBuffer
 Destroy all the buffer memory
 Inputs :
 Outputs :
 */
static void
D3D8SkinDestroyVertexBuffer( RwResEntry *repEntry )
{
    RxD3D8ResEntryHeader    *resEntryHeader;
    RxD3D8InstanceData      *instancedData;
    LPDIRECT3DVERTEXBUFFER8 vertexBuffer;
    RwUInt32                numMeshes;
    RwUInt32                fvf;
    RwUInt32                stride;
    RwUInt32                numVertices;
    RwUInt32                baseIndex;

    RWFUNCTION(RWSTRING("D3D8SkinDestroyVertexBuffer"));

    resEntryHeader = (RxD3D8ResEntryHeader *)(repEntry + 1);

    /* Get the instanced data structures */
    instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

    /* Save shared data */
    if (instancedData->managed)
    {
        vertexBuffer = (LPDIRECT3DVERTEXBUFFER8)instancedData->vertexBuffer;
        fvf = instancedData->vertexShader;
        stride = instancedData->stride;
        baseIndex = (instancedData->baseIndex - instancedData->minVert);
    }
    else
    {
        vertexBuffer = NULL;
        fvf = 0;
        stride = 0;
        baseIndex = 0;
    }

    /* Get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;
    numVertices = 0;

    while (numMeshes--)
    {
        /* Destroy the index buffer */
        if (instancedData->indexBuffer)
        {
            IDirect3DIndexBuffer8_Release((LPDIRECT3DINDEXBUFFER8)instancedData->indexBuffer);
            instancedData->indexBuffer = NULL;
        }

        instancedData->vertexBuffer = NULL;

        numVertices += instancedData->numVertices;

        /* On to the next RxD3D8InstanceData */
        instancedData++;
    }

    /* Destroy the only one vertex buffer that we have */
    if (vertexBuffer)
    {
        _rxD3D8VertexBufferManagerDestroy(fvf, stride * numVertices,
                                            vertexBuffer, baseIndex);
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rxD3D8SkinAtomicCreateVertexBuffer

 Purpose:   Just create the vertex buffer
 On entry:
 On exit :
*/
static RwBool
_rxD3D8SkinAtomicCreateVertexBuffer(const RpAtomic *atomic, RxD3D8ResEntryHeader *resEntryHeader, RwBool hasHierarchy)
{
    const RpGeometry        *geometry;
    RpGeometryFlag          flags;
    RwUInt32                stride;
    RwUInt32                fvf;
    RwUInt32                vbSize;
    LPDIRECT3DVERTEXBUFFER8 vertexBuffer;
    RwUInt32                baseIndex;
    RxD3D8InstanceData      *instancedData;
    RwUInt32                numMeshes;

    RWFUNCTION(RWSTRING("_rxD3D8SkinAtomicCreateVertexBuffer"));

    geometry = (const RpGeometry *)RpAtomicGetGeometry(atomic);

    flags = (RpGeometryFlag)RpGeometryGetFlags(geometry);

    /*
     * Calculate the stride of the vertex
     */
    /* Positions */
    stride = sizeof(RwV3d);
    fvf = D3DFVF_XYZ;

    /* Normals */
    if (flags & rxGEOMETRY_NORMALS)
    {
        stride += sizeof(RwV3d);
        fvf |= D3DFVF_NORMAL;
    }

    /* Pre-lighting */
    if (flags & rxGEOMETRY_PRELIT)
    {
        stride += sizeof(RwRGBA);
        fvf |= D3DFVF_DIFFUSE;
    }

    /* Texture coordinates */
    switch(RpGeometryGetNumTexCoordSets(geometry))
    {
        case 1:
            stride += 1 * sizeof(RwTexCoords);
            fvf |= D3DFVF_TEX1;
            break;
        case 2:
            stride += 2 * sizeof(RwTexCoords);
            fvf |= D3DFVF_TEX2;
            break;
        case 3:
            stride += 3 * sizeof(RwTexCoords);
            fvf |= D3DFVF_TEX3;
            break;
        case 4:
            stride += 4 * sizeof(RwTexCoords);
            fvf |= D3DFVF_TEX4;
            break;
        case 5:
            stride += 5 * sizeof(RwTexCoords);
            fvf |= D3DFVF_TEX5;
            break;
        case 6:
            stride += 6 * sizeof(RwTexCoords);
            fvf |= D3DFVF_TEX6;
            break;
        case 7:
            stride += 7 * sizeof(RwTexCoords);
            fvf |= D3DFVF_TEX7;
            break;
        case 8:
            stride += 8 * sizeof(RwTexCoords);
            fvf |= D3DFVF_TEX8;
            break;
    }

    /*
     * Create the vertex buffer
     */
    vbSize = stride * geometry->numVertices;

    if( hasHierarchy == FALSE ||
        _rwD3D8SkinNeedsAManagedVertexBuffer(atomic, resEntryHeader))
    {
        if (FALSE == _rxD3D8VertexBufferManagerCreate(fvf,
                                                    vbSize,
                                                    (void **)&vertexBuffer,
                                                    &baseIndex))
        {
            RWRETURN(FALSE);
        }
    }
    else
    {
        /* We will use dynamic locks */
        vertexBuffer = NULL;
        baseIndex = 0;
    }

    /* Get the first instanced data structures */
    instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

    /* Get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;

    do
    {
        instancedData->stride = stride;

        instancedData->vertexShader = fvf;

        if (vertexBuffer == NULL)
        {
            instancedData->managed = FALSE;
        }
        else
        {
            instancedData->managed = TRUE;

            instancedData->vertexBuffer = vertexBuffer;
            instancedData->baseIndex = baseIndex + instancedData->minVert;
        }

        instancedData++;
    }
    while (--numMeshes);

    RWRETURN(TRUE);
}

/****************************************************************************
 _rxD3D8SkinGeometryInstanceStaticInfo

 Purpose:   Fill the vertex buffer with the static info: texcoords and prelit
 On entry:
 On exit :
*/
static void
_rxD3D8SkinGeometryInstanceStaticInfo(const RpGeometry *geometry, RxD3D8ResEntryHeader *resEntryHeader, RwUInt8 *vertexData)
{
    RwUInt32            numTextureCoords;
    RxD3D8InstanceData  *instancedData;

    RWFUNCTION(RWSTRING("_rxD3D8SkinGeometryInstanceStaticInfo"));

    /* Get the first instanced data structures */
    instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

    /* Get number of texture coordinates */
    numTextureCoords = RpGeometryGetNumTexCoordSets(geometry);

    /* Texture coordinates */
    if (numTextureCoords)
    {
        const RwUInt32 stride = instancedData->stride;
        RwUInt32    offset;
        RwUInt8     *vertexBuffer;
        RwUInt32    numVertices;
        RwUInt32    n;

        offset = sizeof(RwV3d);

        if (rpGEOMETRYNORMALS & RpGeometryGetFlags(geometry))
        {
            offset += sizeof(RwV3d);
        }

        if (rpGEOMETRYPRELIT & RpGeometryGetFlags(geometry))
        {
            offset += sizeof(RwRGBA);
        }

        for (n = 0; n < numTextureCoords; n++)
        {
            const RwTexCoords *texCoord = (const RwTexCoords *)((const RwUInt8 *)(geometry->texCoords[n]));

            vertexBuffer = vertexData + offset;
            numVertices = geometry->numVertices;

#if defined(NOASM)
            do
            {
                *((RwTexCoords *)vertexBuffer) = *texCoord;
                vertexBuffer += stride;
                texCoord++;
            }
            while (--numVertices);
#else
            _asm
            {
                mov esi, texCoord
                mov edi, vertexBuffer
                mov edx, stride
                mov ecx, numVertices
beginloop:
                mov eax, dword ptr[esi]
                mov ebx, dword ptr[esi+4]
                mov dword ptr[edi], eax
                mov dword ptr[edi+4], ebx
                add esi, 8
                add edi, edx
                dec ecx
                jnz beginloop
            }
#endif

            offset += sizeof(RwTexCoords);
        }
    }

    /*
     * PreLight need to be done per mesh
     */
    if (rpGEOMETRYPRELIT & RpGeometryGetFlags(geometry))
    {
        /* Get the number of meshes */
        RwUInt32 numMeshes = resEntryHeader->numMeshes;

        do
        {
            const RwUInt32 stride = instancedData->stride;
            RwInt32         offset;
            const RwRGBA    *color;
            const RwRGBA    *matColor;
            RwInt32         alpha;
            RwUInt8         *vertexBuffer;
            RwUInt32        numVertices;

            offset = sizeof(RwV3d);

            if (rpGEOMETRYNORMALS & RpGeometryGetFlags(geometry))
            {
                offset += sizeof(RwV3d);
            }

            color = (const RwRGBA *)(((const RwUInt8 *)(&geometry->preLitLum[instancedData->minVert])));
            matColor = RpMaterialGetColor(instancedData->material);

            vertexBuffer = vertexData + offset + (instancedData->minVert * stride);
            numVertices = instancedData->numVertices;

            alpha = 0xff;

            if ((rpGEOMETRYMODULATEMATERIALCOLOR & RpGeometryGetFlags(geometry)) &&
                (*((const RwUInt32 *)matColor) != 0xffffffff))
            {
                while (numVertices--)
                {
                    *((RwUInt32 *)vertexBuffer) =
                        (((color->alpha * (matColor->alpha + 1)) & 0xff00) << (24-8)) |
                        (((color->red * (matColor->red + 1)) & 0xff00) << (16-8)) |
                        (((color->green * (matColor->green + 1)) & 0xff00) << (8-8)) |
                        ((color->blue * (matColor->blue + 1)) >> 8);

                    /* Does the pre-light contain alpha */
                    alpha &= color->alpha;

                    vertexBuffer += stride;
                    color++;
                }
            }
            else
            {
                while (numVertices--)
                {
                    /* Does the pre-light contain alpha */
                    alpha &= color->alpha;

                    *((RwUInt32 *)vertexBuffer) = ((color->alpha << 24) |
                                                   (color->red << 16) |
                                                   (color->green << 8) |
                                                   (color->blue));

                    vertexBuffer += stride;
                    color++;
                }
            }

            instancedData->vertexAlpha = (alpha != 0xff);

            instancedData++;
        }
        while (--numMeshes);
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rxD3D8SkinAtomicInstance

 Purpose:   Fill the vertex buffer
 On entry:
 On exit :
*/
static RwBool
_rxD3D8SkinAtomicInstance(const RpAtomic *atomic, RxD3D8ResEntryHeader *resEntryHeader, RwBool hasHierarchy)
{
    const RpGeometry    *geometry;
    RxD3D8InstanceData  *instancedData;

    RWFUNCTION(RWSTRING("_rxD3D8SkinAtomicInstance"));

    /* Create vertex buffer */
    _rxD3D8SkinAtomicCreateVertexBuffer(atomic, resEntryHeader, hasHierarchy);

    geometry = (const RpGeometry *)RpAtomicGetGeometry(atomic);

    /* Get the first instanced data structures */
    instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

    if (instancedData->managed)
    {
        const RwUInt32  stride = instancedData->stride;
        RwUInt8         *vertexData;

        /*
         * Fill static information of the vertex buffer
         */
        if (SUCCEEDED(IDirect3DVertexBuffer8_Lock((LPDIRECT3DVERTEXBUFFER8)instancedData->vertexBuffer,
                                                 (instancedData->baseIndex - instancedData->minVert) * stride,
                                                 geometry->numVertices * stride, &vertexData,
                                                 D3DLOCK_NOSYSLOCK)))
        {
            RwUInt32    numVertices;
            RwUInt8     *vertexBuffer;
            const RwV3d *pos;

            /* Positions */
            pos = geometry->morphTarget[0].verts;

            vertexBuffer = vertexData;
            numVertices = geometry->numVertices;
            while (numVertices--)
            {
                *((RwV3d *)vertexBuffer) = *pos;
                vertexBuffer += stride;
                pos++;
            }

            /* Normals */
            if (rpGEOMETRYNORMALS & RpGeometryGetFlags(geometry))
            {
                RwUInt32    offset;
                const RwV3d *normal;

                offset = sizeof(RwV3d);

                normal = (const RwV3d *)((const RwUInt8 *)(geometry->morphTarget[0].normals));

                vertexBuffer = vertexData + offset;
                numVertices = geometry->numVertices;
                while (numVertices--)
                {
                    *((RwV3d *)vertexBuffer) = *normal;
                    vertexBuffer += stride;
                    normal++;
                }
            }

            _rxD3D8SkinGeometryInstanceStaticInfo(geometry, resEntryHeader, vertexData);

            IDirect3DVertexBuffer8_Unlock((LPDIRECT3DVERTEXBUFFER8)instancedData->vertexBuffer);
        }
    }

    RWRETURN(TRUE);
}

/****************************************************************************
 - SkinMatBlendAtomicRender

 - Matrix blending Atomic render function - performs weighted transform of
   an atomic's vertices according to the attached RpSkin data.

 - If NOASM=0 then an Intel x86 assembler function is used for matrix blending.
   Otherwise...

 - Inputs :   RpAtomic *    A pointer to the atomic.
 - Outputs:   RwBool        TRUE if the atomic has a skin, a hierarchy an
                            everything works right
 */
static void
SkinMatBlendAtomicRender(RpAtomic *atomic)
{
    RpGeometry  *geometry;
    RpSkin      *skin;

    RWFUNCTION(RWSTRING("SkinMatBlendAtomicRender"));
    RWASSERT(NULL != atomic);

    geometry = RpAtomicGetGeometry(atomic);
    RWASSERT(NULL != geometry);

    skin = *RPSKINGEOMETRYGETDATA(geometry);

    if(NULL != skin)
    {
        RpHAnimHierarchy    *hierarchy;

        /* Perform matrix blending */
        hierarchy = RPSKINATOMICGETDATA(atomic)->hierarchy;

        if ( NULL != hierarchy )
        {
            RpMeshHeader            *meshHeader;
            RwResEntry              *resEntry;
            RxD3D8ResEntryHeader    *resEntryHeader;
            RxD3D8InstanceData      *instancedData;
            const RwV3d             *originalVertices;
            const RwV3d             *originalNormals;
            RwBool                  fillStaticInfo;
            RwUInt8                 *vertexData;
            RwUInt32                stride;
            LPDIRECT3DVERTEXBUFFER8 vertexBuffer;

            /* Get mesh info */
            meshHeader = geometry->mesh;

            /* Get instanced data */
            if (geometry->numMorphTargets != 1)
            {
                resEntry = atomic->repEntry;
            }
            else
            {
                resEntry = geometry->repEntry;
            }

            resEntryHeader = (RxD3D8ResEntryHeader *)(resEntry + 1);
            instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

            /* Get original vertex information */
            originalVertices = (const RwV3d *)
                               (geometry->morphTarget->verts);

            if (rpGEOMETRYNORMALS & RpGeometryGetFlags(geometry))
            {
                originalNormals = (const RwV3d *)
                                  (geometry->morphTarget->normals);
            }
            else
            {
                originalNormals = NULL;
            }

            /* Check right vertexbuffer */
            fillStaticInfo = ( (rpGEOMETRYPRELIT & RpGeometryGetFlags(geometry)) != 0 ||
                                RpGeometryGetNumTexCoordSets(geometry) > 0);

            vertexData = NULL;

            stride = instancedData->stride;

            if(_rwD3D8SkinNeedsAManagedVertexBuffer(atomic, resEntryHeader))
            {
                if (!instancedData->managed)
                {
                    _rxD3D8SkinAtomicCreateVertexBuffer(atomic, resEntryHeader, TRUE);
                }
                else
                {
                    fillStaticInfo = FALSE;
                }

                vertexBuffer = (LPDIRECT3DVERTEXBUFFER8)instancedData->vertexBuffer;

                IDirect3DVertexBuffer8_Lock( vertexBuffer,
                                             (instancedData->baseIndex - instancedData->minVert) * stride,
                                             geometry->numVertices * stride, &vertexData,
                                             D3DLOCK_NOSYSLOCK);
            }
            else
            {
                RwUInt32    baseIndex;
                RwUInt32    numMeshes;

                if (instancedData->managed)
                {
                    _rxD3D8VertexBufferManagerDestroy(instancedData->vertexShader,
                                                        stride * geometry->numVertices,
                                                        instancedData->vertexBuffer,
                                                        (instancedData->baseIndex - instancedData->minVert));
                }

                RwD3D8DynamicVertexBufferLock(stride,
                                              geometry->numVertices,
                                              (void **)&vertexBuffer,
                                              (void **)&vertexData,
                                              &baseIndex);
                /* Fix the base index */
                numMeshes = resEntryHeader->numMeshes;

                do
                {
                    instancedData->managed = FALSE;
                    instancedData->vertexBuffer = vertexBuffer;
                    instancedData->baseIndex = baseIndex + instancedData->minVert;

                    instancedData++;
                }
                while (--numMeshes);
            }

            if (vertexData != NULL)
            {
                RwUInt8 *vertices;
                RwUInt8 *normals;

                /* Find Destination offset */
                vertices = vertexData;

                if (originalNormals != NULL)
                {
                    normals = vertexData + sizeof(RwV3d);
                }
                else
                {
                    normals = NULL;
                }

#if !defined(NOASM) && !defined(NOSSEASM)
                if (_rwIntelSSEsupported())
                {
                    RwMatrix *matrixArray;

                    matrixArray = _rwD3D8SkinPrepareMatrix(atomic, skin, hierarchy);

                    if (skin->vertexMaps.maxWeights > 2)
                    {
                        _rpSkinIntelSSEMatrixBlend(geometry->numVertices,
                                                   skin->vertexMaps.matrixWeights,
                                                   skin->vertexMaps.matrixIndices,
                                                   matrixArray,
                                                   vertices,
                                                   originalVertices,
                                                   normals,
                                                   originalNormals,
                                                   stride);
                    }
                    else if(skin->vertexMaps.maxWeights > 1)
                    {
                        _rpSkinIntelSSEMatrixBlend2Weights(geometry->numVertices,
                                                           skin->vertexMaps.matrixWeights,
                                                           skin->vertexMaps.matrixIndices,
                                                           matrixArray,
                                                           vertices,
                                                           originalVertices,
                                                           normals,
                                                           originalNormals,
                                                           stride);
                    }
                    else
                    {
                        if (skin->boneData.numUsedBones == 1)
                        {
                            _rpSkinIntelSSEMatrixBlend1Matrix( geometry->numVertices,
                                                               &(matrixArray[skin->boneData.usedBoneList[0]]),
                                                               vertices,
                                                               originalVertices,
                                                               normals,
                                                               originalNormals,
                                                               stride);
                        }
                        else
                        {
                            _rpSkinIntelSSEMatrixBlend1Weight( geometry->numVertices,
                                                               skin->vertexMaps.matrixIndices,
                                                               matrixArray,
                                                               vertices,
                                                               originalVertices,
                                                               normals,
                                                               originalNormals,
                                                               stride);
                        }
                    }
                }
                else
#endif
                {
                    _rwD3D8MatrixTransposed  *matrixArray;

                    matrixArray = _rwD3D8SkinPrepareMatrixTransposed(atomic, skin, hierarchy);

                    /* Vertex blending */
                    if (skin->vertexMaps.maxWeights > 2)
                    {
                        D3D8SkinGenericMatrixBlend(geometry->numVertices,
                                                   skin->vertexMaps.matrixWeights,
                                                   skin->vertexMaps.matrixIndices,
                                                   matrixArray,
                                                   vertices,
                                                   originalVertices,
                                                   normals,
                                                   originalNormals,
                                                   stride);
                    }
                    else if(skin->vertexMaps.maxWeights > 1)
                    {
                        D3D8SkinGenericMatrixBlend2Weights(geometry->numVertices,
                                                           skin->vertexMaps.matrixWeights,
                                                           skin->vertexMaps.matrixIndices,
                                                           matrixArray,
                                                           vertices,
                                                           originalVertices,
                                                           normals,
                                                           originalNormals,
                                                           stride);
                    }
                    else
                    {
                        if (skin->boneData.numUsedBones == 1)
                        {
                            D3D8SkinGenericMatrixBlend1Matrix( geometry->numVertices,
                                                               &(matrixArray[skin->boneData.usedBoneList[0]]),
                                                               vertices,
                                                               originalVertices,
                                                               normals,
                                                               originalNormals,
                                                               stride);
                        }
                        else
                        {
                            D3D8SkinGenericMatrixBlend1Weight( geometry->numVertices,
                                                               skin->vertexMaps.matrixIndices,
                                                               matrixArray,
                                                               vertices,
                                                               originalVertices,
                                                               normals,
                                                               originalNormals,
                                                               stride);
                        }
                    }
                }

                /* Fill texture coords and prelit */
                if (fillStaticInfo)
                {
                    _rxD3D8SkinGeometryInstanceStaticInfo(geometry, resEntryHeader, vertexData);
                }

                IDirect3DVertexBuffer8_Unlock(vertexBuffer);
            }

            atomic->interpolator.flags &= ~rpINTERPOLATORDIRTYINSTANCE;
        }
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rxD3D8SkinInstance

 Purpose:   To instance.

 On entry:

 On exit :
*/
static RwResEntry *
_rxD3D8SkinInstance(const RpAtomic *atomic,
              void *owner,
              RwResEntry **resEntryPointer,
              RpMeshHeader *meshHeader,
              _rxD3D8SkinInstanceNodeData *privateData __RWUNUSED__,
              RwUInt32 flags __RWUNUSED__,
              RwBool reinstance __RWUNUSED__)
{
    RxD3D8ResEntryHeader    *resEntryHeader;
    RxD3D8InstanceData      *instancedData;
    RwResEntry              *resEntry;
    RpMesh                  *mesh;
    RwInt16                 numMeshes;
    RwUInt32                size;

    RWFUNCTION(RWSTRING("_rxD3D8SkinInstance"));

    /*
     * Calculate the amount of memory to allocate
     */

    /* RxD3D8ResEntryHeader, stores serialNumber & numMeshes */
    size = sizeof(RxD3D8ResEntryHeader);

    /* RxD3D8InstanceData structures, one for each mesh */
    size += sizeof(RxD3D8InstanceData) * meshHeader->numMeshes;

    /*
     * Allocate the resource entry
     */
    resEntry = RwResourcesAllocateResEntry(owner,
                                           resEntryPointer,
                                           size,
                                           D3D8SkinDestroyVertexBuffer);
    RWASSERT(NULL != resEntry);

    /* Blank the RxD3D8ResEntryHeader & RxD3D8InstanceData's to '0' */
    memset((resEntry + 1), 0, size);

    /*
     * Initialize the RxD3D8ResEntryHeader
     */
    resEntryHeader = (RxD3D8ResEntryHeader *)(resEntry + 1);

    /* Set the serial number */
    resEntryHeader->serialNumber = meshHeader->serialNum;

    /* Set the number of meshes */
    resEntryHeader->numMeshes = meshHeader->numMeshes;

    /* Get the first RxD3D8InstanceData pointer */
    instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

    mesh = (RpMesh *)(meshHeader + 1);
    numMeshes = meshHeader->numMeshes;
    while (numMeshes--)
    {
        RwUInt32 numIndices = mesh->numIndices;

        /*
         * Number of vertices and Min vertex index,
         * (needed for instancing & reinstancing)
         */
        _rwD3D8MeshGetNumVerticesMinIndex(mesh->indices, numIndices,
                                &instancedData->numVertices,
                                &instancedData->minVert);

        /* Primitive type */
        instancedData->primType = _RwD3D8PrimConvTable[RpMeshHeaderGetPrimType(meshHeader)];

        /* The number of indices */
        instancedData->numIndices = numIndices;

        /* Material */
        instancedData->material = mesh->material;

        /* Vertex shader */
        instancedData->vertexShader = 0;

        /* The vertex format stride */
        instancedData->stride = 0;

        /* Initialize the vertex buffers pointers */
        instancedData->vertexBuffer = NULL;

        /* Initialize vertex buffer managed to FALSE */
        instancedData->baseIndex = 0;

        instancedData->managed = FALSE;

        /* Initialize vertex alpha to FALSE */
        instancedData->vertexAlpha = FALSE;

        /*
         * Set the index buffer
         */

        /* Initialize the index buffers pointers */
        if (RwD3D8IndexBufferCreate(numIndices, &(instancedData->indexBuffer)))
        {
            RxVertexIndex   *indexBuffer;

             if (D3D_OK == IDirect3DIndexBuffer8_Lock((LPDIRECT3DINDEXBUFFER8)instancedData->indexBuffer,
                                                    0, 0, (RwUInt8 **)&indexBuffer, 0))
             {
                if(instancedData->minVert)
                {
                    RxVertexIndex   *indexSrc;

                    indexSrc = mesh->indices;

                    while (numIndices--)
                    {
                        *indexBuffer = (RxVertexIndex)((*indexSrc) - (RxVertexIndex)instancedData->minVert);

                        indexBuffer++;
                        indexSrc++;
                    }
                }
                else
                {
                    memcpy(indexBuffer, mesh->indices, sizeof(RxVertexIndex) * numIndices);
                }

                 IDirect3DIndexBuffer8_Unlock((LPDIRECT3DINDEXBUFFER8)instancedData->indexBuffer);
             }
        }

        instancedData++;
        mesh++;
    }

    /*
     * Call the instance callback
     */
    _rxD3D8SkinAtomicInstance(atomic, resEntryHeader, (RPSKINATOMICGETCONSTDATA(atomic)->hierarchy != NULL));

    RWRETURN(resEntry);
}

/****************************************************************************
 _rwSkinD3D8AtomicAllInOneNode
 */
RwBool
_rwSkinD3D8AtomicAllInOneNode(RxPipelineNodeInstance *self,
                        const RxPipelineNodeParam *params)
{
    _rxD3D8SkinInstanceNodeData *privateData;
    RpAtomic                *atomic;
    RpGeometry              *geometry;
    RwResEntry              *resEntry;
    RpMeshHeader            *meshHeader;
    RwUInt32                geomFlags;
    RwMatrix                *matrix;
    RwUInt32                lighting;

    RWFUNCTION(RWSTRING("_rwSkinD3D8AtomicAllInOneNode"));
    RWASSERT(NULL != self);
    RWASSERT(NULL != params);

    privateData = (_rxD3D8SkinInstanceNodeData *)self->privateData;

    atomic = (RpAtomic *)RxPipelineNodeParamGetData(params);
    RWASSERT(NULL != atomic);

    geometry = RpAtomicGetGeometry(atomic);
    RWASSERT(NULL != geometry);

    /* If there ain't vertices, we cain't make packets... */
    if (geometry->numVertices <= 0)
    {
        /* Don't execute the rest of the pipeline */
        RWRETURN(TRUE);
    }

    meshHeader = geometry->mesh;

    /* Early out if no meshes */
    if (meshHeader->numMeshes <= 0)
    {
        /* If the app wants to use plugin data to make packets, it
         * should use its own instancing function. If we have verts
         * here, we need meshes too in order to build a packet. */
        RWRETURN(TRUE);
    }

    /* If the geometry has more than one morph target the resEntry in the
     * atomic is used else the resEntry in the geometry */
    if (geometry->numMorphTargets != 1)
    {
        resEntry = atomic->repEntry;
    }
    else
    {
        resEntry = geometry->repEntry;
    }

    /* If the meshes have changed we should re-instance */
    if (resEntry)
    {
        RxD3D8ResEntryHeader    *resEntryHeader;

        resEntryHeader = (RxD3D8ResEntryHeader *)(resEntry + 1);
        if (resEntryHeader->serialNumber != meshHeader->serialNum)
        {
            /* Destroy resources to force reinstance */
            RwResourcesFreeResEntry(resEntry);
            resEntry = NULL;
        }
    }

    geomFlags = RpGeometryGetFlags(geometry);

    /* Check to see if a resource entry already exists */
    if (!resEntry)
    {
        RwResEntry  **resEntryPointer;
        void        *owner;

        meshHeader = geometry->mesh;

        if (geometry->numMorphTargets != 1)
        {
            owner = (void *)atomic;
            resEntryPointer = &atomic->repEntry;
        }
        else
        {
            owner = (void *)geometry;
            resEntryPointer = &geometry->repEntry;
        }

        /*
         * Create vertex buffers and instance
         */
        resEntry = _rxD3D8SkinInstance(atomic, owner, resEntryPointer,
                                 meshHeader, privateData, geomFlags, FALSE);
        if (!resEntry)
        {
            RWRETURN(FALSE);
        }

        /* The geometry is up to date */
        geometry->lockedSinceLastInst = 0;
    }
    else
    {
        /* We have a resEntry so use it */
        RwResourcesUseResEntry(resEntry);
    }

    /* Update skin vertices */
    SkinMatBlendAtomicRender(atomic);

    /*
     * Set up lights
     */
    if (privateData->lightingCallback)
    {
        privateData->lightingCallback((void *)atomic);
    }

    /*
     * Set the world transform to identity because all the akin matrix were in world space
     */
    matrix = RwFrameGetLTM(RpAtomicGetFrame(atomic));

    RwD3D8SetTransformWorld(matrix);

    /* Check if we need to normalize */
    RwD3D8GetRenderState(D3DRS_LIGHTING, &lighting);

    if (lighting)
    {
        if (_rpSkinGlobals.platform.hardwareTL)
        {
            RwD3D8SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
        }
        else if (!rwMatrixTestFlags(matrix, (rwMATRIXTYPENORMAL | rwMATRIXINTERNALIDENTITY)))
        {
            const RwReal minlimit = 0.9f;
            const RwReal maxlimit = 1.1f;
            RwReal length;

            length = RwV3dDotProduct(&(matrix->right), &(matrix->right));

            if ( (FLOATASINT(length) > FLOATASINT(maxlimit)) || (FLOATASINT(length) < FLOATASINT(minlimit)) )
            {
                RwD3D8SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
            }
            else
            {
                length = RwV3dDotProduct(&(matrix->up), &(matrix->up));

                if ( (FLOATASINT(length) > FLOATASINT(maxlimit)) || (FLOATASINT(length) < FLOATASINT(minlimit)) )
                {
                    RwD3D8SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
                }
                else
                {
                    length = RwV3dDotProduct(&(matrix->at), &(matrix->at));

                    if ( (FLOATASINT(length) > FLOATASINT(maxlimit)) || (FLOATASINT(length) < FLOATASINT(minlimit)) )
                    {
                        RwD3D8SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
                    }
                    else
                    {
                        RwD3D8SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);
                    }
                }
            }
        }
        else
        {
            RwD3D8SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);
        }
    }
    else
    {
        RwD3D8SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);
    }

    /*
     * Render
     */
    if (privateData->renderCallback)
    {
        privateData->renderCallback(resEntry, (void *)atomic, rpATOMIC, geomFlags);
    }

#ifdef RWMETRICS
    /* Now update our metrics statistics */
    RWSRCGLOBAL(metrics)->numVertices  += RpGeometryGetNumVertices(geometry);
    RWSRCGLOBAL(metrics)->numTriangles += RpGeometryGetNumTriangles(geometry);
#endif

    RWRETURN(TRUE);
}

/****************************************************************************
 _rwD3D8SkinPrepareMatrix
  Inputs :
 Outputs:   return the aligned matrix cache
 */
RwMatrix *
_rwD3D8SkinPrepareMatrix(RpAtomic *atomic, RpSkin *skin, RpHAnimHierarchy *hierarchy)
{
    RWFUNCTION(RWSTRING("_rwD3D8SkinPrepareMatrix"));

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
            matrix++;
        }

        _rpSkinGlobals.platform.lastHierarchyUsed = (const RpHAnimHierarchy *)hierarchy;
        _rpSkinGlobals.platform.lastRenderFrame = RWSRCGLOBAL(renderFrame);
        _rpSkinGlobals.platform.lastFrame = RpAtomicGetFrame(atomic);
    }

    RWRETURN(_rpSkinGlobals.matrixCache.aligned);
}

/****************************************************************************
 _rwD3D8SkinPrepareMatrixTransposed
  Inputs :
 Outputs:   return the aligned matrix cache
 */
_rwD3D8MatrixTransposed *
_rwD3D8SkinPrepareMatrixTransposed(RpAtomic *atomic, RpSkin *skin, RpHAnimHierarchy *hierarchy)
{
    RWFUNCTION(RWSTRING("_rwD3D8SkinPrepareMatrixTransposed"));

    if (_rpSkinGlobals.platform.lastHierarchyUsed != hierarchy ||
        _rpSkinGlobals.platform.lastRenderFrame != RWSRCGLOBAL(renderFrame) ||
        _rpSkinGlobals.platform.lastFrame != RpAtomicGetFrame(atomic))
    {
        D3D8SkinPrepareAtomicMatrixTransposed(atomic, skin, hierarchy);

        _rpSkinGlobals.platform.lastHierarchyUsed = (const RpHAnimHierarchy *)hierarchy;
        _rpSkinGlobals.platform.lastRenderFrame = RWSRCGLOBAL(renderFrame);
        _rpSkinGlobals.platform.lastFrame = RpAtomicGetFrame(atomic);
    }

    RWRETURN((_rwD3D8MatrixTransposed *)_rpSkinGlobals.matrixCache.aligned);
}

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

/****************************************************************************
 _rpSkinPipelinesDestroy
 Destroy the skinning pipelines.
 Inputs :   None
 Outputs:   RwBool - on success.
 */
RwBool
_rpSkinPipelinesDestroy(void)
{
    RwInt32 pipeIndex;

    RWFUNCTION(RWSTRING("_rpSkinPipelinesDestroy"));
/*
    if (_rpSkinPipeline(rpSKIND3D8PIPELINEGENERIC))
    {
        RxPipelineDestroy(_rpSkinPipeline(rpSKIND3D8PIPELINEGENERIC));

        _rpSkinPipeline(rpSKIND3D8PIPELINEGENERIC) = NULL;
    }

    if (_rpSkinPipeline(rpSKIND3D8PIPELINEMATFX))
    {
        RxPipelineDestroy(_rpSkinPipeline(rpSKIND3D8PIPELINEMATFX));

        _rpSkinPipeline(rpSKIND3D8PIPELINEMATFX) = NULL;
    }

    if (_rpSkinPipeline(rpSKIND3D8PIPELINETOON))
    {
        RxPipelineDestroy(_rpSkinPipeline(rpSKIND3D8PIPELINETOON));

        _rpSkinPipeline(rpSKIND3D8PIPELINETOON) = NULL;
    }
*/
    for (pipeIndex = rpSKIND3D8PIPELINEGENERIC; pipeIndex < rpSKIND3D8PIPELINEMAX; pipeIndex++)
    {
        if (_rpSkinPipeline(pipeIndex))
        {
            RxPipelineDestroy(_rpSkinPipeline(pipeIndex));

            _rpSkinPipeline(pipeIndex) = NULL;
        }
    }

    RWRETURN(TRUE);
}

/****************************************************************************
 BonesUsedSort
 */
int
BonesUsedSort(const void *data0, const void *data1)
{
    const RwUInt8 *index0 = (const RwUInt8 *)data0;
    const RwUInt8 *index1 = (const RwUInt8 *)data1;

    RWFUNCTION(RWSTRING("BonesUsedSort"));

    RWRETURN(*index0 - *index1);
}

/****************************************************************************
 _rpSkinInitialize

 Initialise an atomic's matrix-blending skin data.

 Inputs :   RpGeometry * - Pointer to a skinned geometry.
 Outputs:   RpGeometry * - Pointer to the skinned geometry on success.
 */
RpGeometry *
_rpSkinInitialize(RpGeometry *geometry)
{
    RpSkin *skin;

    RWFUNCTION(RWSTRING("_rpSkinInitialize"));
    RWASSERT(NULL != geometry);

    skin = *RPSKINGEOMETRYGETDATA(geometry);

    if(skin != NULL)
    {
        RwInt32 n;
        RwMatrixWeights *matrixWeights;
        RwUInt32 *matrixIndexMap;

        matrixWeights = skin->vertexMaps.matrixWeights;

        matrixIndexMap = skin->vertexMaps.matrixIndices;

        for (n = 0; n < geometry->numVertices; n++)
        {
            RWASSERT(FLOATASINT(matrixWeights->w0) > FLOATASINT(Zero));

            if (FLOATASINT(matrixWeights->w0) < FLOATASINT(One))
            {
                RwBool bCheck;

                /* Sort indices and weights */
                do
                {
                    bCheck = FALSE;

                    if (FLOATASINT(matrixWeights->w0) < FLOATASINT(matrixWeights->w1))
                    {
                        RwUInt8 *indices = (RwUInt8 *)(matrixIndexMap + n);

                        const RwUInt8 temp = indices[0];
                        const RwReal tempf = matrixWeights->w0;

                        indices[0] = indices[1];
                        indices[1] = temp;

                        matrixWeights->w0 = matrixWeights->w1;
                        matrixWeights->w1 = tempf;

                        bCheck = TRUE;
                    }

                    if (FLOATASINT(matrixWeights->w1) < FLOATASINT(matrixWeights->w2))
                    {
                        RwUInt8 *indices = (RwUInt8 *)(matrixIndexMap + n);

                        const RwUInt8 temp = indices[1];
                        const RwReal tempf = matrixWeights->w1;

                        indices[1] = indices[2];
                        indices[2] = temp;

                        matrixWeights->w1 = matrixWeights->w2;
                        matrixWeights->w2 = tempf;

                        bCheck = TRUE;
                    }

                    if (FLOATASINT(matrixWeights->w2) < FLOATASINT(matrixWeights->w3))
                    {
                        RwUInt8 *indices = (RwUInt8 *)(matrixIndexMap + n);

                        const RwUInt8 temp = indices[2];
                        const RwReal tempf = matrixWeights->w2;

                        indices[2] = indices[3];
                        indices[3] = temp;

                        matrixWeights->w2 = matrixWeights->w3;
                        matrixWeights->w3 = tempf;

                        bCheck = TRUE;
                    }
                }
                while(bCheck);
            }

            matrixWeights++;
        }
    }

    /* Sort the used bones list */
    if (skin->boneData.usedBoneList != NULL)
    {
        qsort(skin->boneData.usedBoneList,
              skin->boneData.numUsedBones,
              sizeof(RwUInt8),
              BonesUsedSort);
    }

    RWRETURN(geometry);
}


/****************************************************************************
 _rpSkinDeinitialize
 Platform specific deinitialize function for skinned geometry's.
 Inputs :  *geometry    - Pointer to the skinned geometry.
 Outputs:  RpGeometry * - The geometry which has been deinitialized.
 */
RpGeometry *
_rpSkinDeinitialize(RpGeometry *geometry)
{
    RpSkin *skin;

    RWFUNCTION(RWSTRING("_rpSkinDeinitialize"));
    RWASSERT(NULL != geometry);

    skin = *RPSKINGEOMETRYGETDATA(geometry);

    if(NULL != skin)
    {
    }

    RWRETURN(geometry);
}

/*===========================================================================*
 *--- Plugin Native Serialization Functions ---------------------------------*
 *===========================================================================*/

/****************************************************************************
 _rpSkinGeometryNativeSize
 */
RwInt32
_rpSkinGeometryNativeSize(const RpGeometry *geometry __RWUNUSED__)
{
#if 0
    RwInt32         size;
    const RpSkin    *skin;
#endif /* 0 */

    RWFUNCTION(RWSTRING("_rpSkinGeometryNativeSize"));

#if 0
    RWASSERT(geometry);

    /* Chunk header for version number + platform specific ID */
    size = rwCHUNKHEADERSIZE + sizeof(RwPlatformID);

    /* Num bones. */
    size += sizeof(RwUInt32);

    /* ... */

    RWRETURN(size);
#endif /* 0 */

    RWRETURN(0);
}

/****************************************************************************
 _rpSkinGeometryNativeWrite
 */
RwStream *
_rpSkinGeometryNativeWrite(RwStream *stream, const RpGeometry *geometry)
{
    RwPlatformID    id = rwID_PCD3D8;
    RwInt32         size;
    const RpSkin    *skin;

    RWFUNCTION(RWSTRING("_rpSkinGeometryNativeWrite"));
    RWASSERT(stream);
    RWASSERT(geometry);

    /* Get size of data to write */
    size = _rpSkinGeometryNativeSize(geometry) - rwCHUNKHEADERSIZE;

    /* Write a chunk header so we get a VERSION NUMBER */
    if (!RwStreamWriteChunkHeader(stream, rwID_STRUCT, size))
    {
        RWRETURN((RwStream *)NULL);
    }

    /* Write a platform unique identifier */
    if (!RwStreamWriteInt32(stream, (RwInt32 *)&id, sizeof(RwPlatformID)))
    {
        RWRETURN((RwStream *)NULL);
    }

    skin = *RPSKINGEOMETRYGETCONSTDATA(geometry);

    /*
     * Write the number of bones
     */
    if (!RwStreamWriteInt32(stream,
                            (const RwInt32 *)&(skin->boneData.numBones),
                            sizeof(RwUInt32)))
    {
        RWRETURN((RwStream *)NULL);
    }

    RWRETURN(stream);
}

/****************************************************************************
 _rpSkinGeometryNativeRead
 */
RwStream *
_rpSkinGeometryNativeRead(RwStream *stream, RpGeometry *geometry)
{
    RwUInt32        version, size;
    RwPlatformID    id;
    RpSkin          *skin;

    RWFUNCTION(RWSTRING("_rpSkinGeometryNativeRead"));
    RWASSERT(stream);
    RWASSERT(geometry);

    if (!RwStreamFindChunk(stream, rwID_STRUCT, (RwUInt32 *)&size, &version))
    {
        RWRETURN((RwStream *)NULL);
    }

    if ((version < rwLIBRARYBASEVERSION) ||
        (version > rwLIBRARYCURRENTVERSION))
    {
        RWERROR((E_RW_BADVERSION));
        RWRETURN((RwStream *)NULL);
    }

    /* Read the platform unique identifier */
    if (!RwStreamReadInt32(stream, (RwInt32 *)&id, sizeof(RwPlatformID)))
    {
        RWRETURN((RwStream *)NULL);
    }

    /* Check this data is funky for this platform */
    if (rwID_PCD3D8 != id)
    {
        RWASSERT(rwID_PCD3D8 == id);

        RWRETURN((RwStream *)NULL);
    }

    /* Create a new skin */
    skin = (RpSkin *)RwFreeListAlloc(_rpSkinGlobals.freeList,
                      rwID_SKINPLUGIN | rwMEMHINTDUR_EVENT);
    RWASSERT(NULL != skin);

    /* Clean the skin. */
    memset(skin, 0, sizeof(RpSkin));

    /* Read the number of bones */
    if (!RwStreamReadInt32(stream,
                           (RwInt32 *)&skin->boneData.numBones,
                           sizeof(RwInt32)))
    {
        RWRETURN((RwStream *)NULL);
    }

    RpSkinGeometrySetSkin(geometry, skin);

    RWRETURN(stream);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/
