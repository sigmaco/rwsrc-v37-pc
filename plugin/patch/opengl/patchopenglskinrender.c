/***********************************************************************
 *
 * Module:  patchopenglskinrender.c
 *
 * Purpose:
 *
 ***********************************************************************/

/* =====================================================================
 *  Includes
 * ===================================================================== */
#include "rwcore.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpskin.h"

#include "patch.h"
#include "patchatomic.h"
#include "patchgeom.h"

#include "patchopenglskinrender.h"


 /* =====================================================================
 *  Defines
 * ===================================================================== */


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */


/* =====================================================================
 *  Static variables
 * ===================================================================== */
static RpAtomic*
openglPatchSkinAtomicBlend( RpAtomic *atomic, void *vertices, void *normals );

static RwUInt32 openglPatchSkinInterleavedArraySize = 0;


/* =====================================================================
 *  Global variables
 * ===================================================================== */


/* =====================================================================
 *  Static function prototypes
 * ===================================================================== */


/* =====================================================================
 *  Global function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: _rpPatchOpenGLSkinAtomicRenderCB
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RpAtomic*
_rpPatchOpenGLSkinAtomicRenderCB( RpAtomic *atomic )
{
    RpGeometry      *geom;

    RpGeometry      *patchGeom;

    PatchAtomicData *atomicData;


    RWFUNCTION( RWSTRING( "_rpPatchOpenGLSkinAtomicRenderCB" ) );

    RWASSERT( NULL != atomic );

    geom = RpAtomicGetGeometry(atomic);
    RWASSERT( NULL != geom );

    RWASSERT( 1 == RpGeometryGetNumMorphTargets(geom) );

    atomicData = PATCHATOMICGETDATA(atomic);
    RWASSERT( NULL != atomicData );

    if ( NULL == geom->repEntry )
    {
        patchGeom = _rpPatchToGeometry( atomic, openglPatchSkinAtomicBlend );
    }
    else
    {
        RpGeometry              *geometry;

        const RpSkin            *skin;

        const RpHAnimHierarchy  *hierarchy;

        RwBool                  earlyOut;


        (void)RwResourcesUseResEntry( geom->repEntry );

        /* can we early out if there's no skin or hierarchy? */
        earlyOut = FALSE;

        geometry = RpAtomicGetGeometry( atomic );
        RWASSERT( NULL != geometry );

        skin = RpSkinGeometryGetSkin( geometry );
        hierarchy = RpSkinAtomicGetHAnimHierarchy( atomic );
        if ( (NULL == skin) || (NULL == hierarchy) )
        {
            earlyOut = TRUE;
        }

        if ( FALSE != earlyOut )
        {
            PatchMesh   *patchMesh;

            RwUInt32    numQuadPatches;

            RwUInt32    numTriPatches;

            RwInt32     patchLOD;

            RwUInt32    numVerts;


            patchGeom = *(RpGeometry**)(geom->repEntry + 1);

            /* get the patch mesh info */
            patchMesh = atomicData->patchMesh;
            RWASSERT(NULL != patchMesh);

            numQuadPatches = _rpPatchMeshGetNumQuadPatches(patchMesh);
            numTriPatches = _rpPatchMeshGetNumTriPatches(patchMesh);

            /* query for the LOD */
            patchLOD = (atomicData->lod.callback)(atomic, atomicData->lod.userData);

            /* total number of verts */
            numVerts = (numQuadPatches * PATCHQUADNUMVERT(patchLOD + 1)) +
                       (numTriPatches * PATCHTRINUMVERT(patchLOD + 1));

            if ( (geom->mesh->serialNum != patchGeom->mesh->serialNum) ||
                 (0 != geom->lockedSinceLastInst) ||
                 (numVerts != _rpPatchOpenGLGetNumVerticesFromResEntry( patchGeom->repEntry )) )
            {
                /* LOD or something has changed, force a reinstance */
                earlyOut = FALSE;
            }
        }

        if ( FALSE == earlyOut )
        {
            /* might be better to orphan the ResEntry here */
            RwResourcesFreeResEntry( geom->repEntry );
            geom->lockedSinceLastInst = 0;
            patchGeom = _rpPatchToGeometry( atomic, openglPatchSkinAtomicBlend );
        }
    }

    if ( NULL == patchGeom )
    {
        RWRETURN( NULL );
    }

    /* swap geometries over */
    atomic->geometry = patchGeom;

    /* render */
    atomicData->openglLocals.renderCallback(atomic);

    /* swap back */
    atomic->geometry = geom;

    RWRETURN( atomic );
}



/* =====================================================================
 *  Static function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: openglPatchSkinAtomicBlend
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static RpAtomic*
openglPatchSkinAtomicBlend( RpAtomic *atomic, void *vertices, void *normals )
{
    RpGeometry              *geometry;

    const RwV3d             *originalVertices;

    const RwV3d             *originalNormals;

    RwUInt32                geomNumVerts;

    RpSkin                  *skin;

    RpHAnimHierarchy        *hierarchy;

    const RwMatrix          *matrixArray;

    RpGeometryFlag          geomFlags;

    RwUInt32                vertexStride;

    RwUInt32                interleavedArraySize;

    RwUInt8                 *interleavedVertices;

    RwUInt8                 *interleavedNormals;

    RwUInt32                i;


    RWFUNCTION( RWSTRING( "openglPatchSkinAtomicBlend" ) );

    RWASSERT( NULL != atomic );
    RWASSERT( NULL != vertices );

    geometry = RpAtomicGetGeometry( atomic );
    RWASSERT( NULL != geometry );

    originalVertices = geometry->morphTarget[0].verts;
    originalNormals = geometry->morphTarget[0].normals;
    if ( NULL == normals )
    {
        RWASSERT( NULL == originalNormals );
    }

    geomNumVerts = RpGeometryGetNumVertices( geometry );

    skin      = RpSkinGeometryGetSkin( geometry );
    hierarchy = RpSkinAtomicGetHAnimHierarchy( atomic );
    if ( (NULL == skin) || (NULL == hierarchy) )
    {
        /* no skin or hierarchy, just copy the vertices/normals from the geometry */
        memcpy( (void *)vertices, originalVertices, sizeof(RwV3d) * geomNumVerts );
        if ( NULL != originalNormals )
        {
            memcpy( (void *)normals, originalNormals, sizeof(RwV3d) * geomNumVerts );
        }

        RWRETURN( atomic );
    }

    /* perform matrix blending */
    matrixArray = _rpSkinOpenGLPrepareAtomicMatrix( (RpAtomic *)atomic,
                                                    skin,
                                                    hierarchy );
    RWASSERT( NULL != matrixArray );

    geomFlags = RpGeometryGetFlags( geometry );

    vertexStride = sizeof(RwV3d);
    if ( 0 != (geomFlags & rpGEOMETRYNORMALS) )
    {
        vertexStride += sizeof(RwV3d);
    }
    if ( 0 != (geomFlags & rpGEOMETRYPRELIT) )
    {
        vertexStride += sizeof(RwRGBA);
    }
    vertexStride += sizeof(RwTexCoords) * RpGeometryGetNumTexCoordSets( geometry );

    /* we are given two arrays that are streams of positions and normals, but not interleaved, and
     * the skinning blending algorithms below work on interleaved arrays. */
    interleavedArraySize = 2 * vertexStride * geomNumVerts;
    if ( interleavedArraySize > openglPatchSkinInterleavedArraySize )
    {
        if ( NULL != openglPatchSkinInterleavedArrays )
        {
            RwFree( openglPatchSkinInterleavedArrays );
        }

        openglPatchSkinInterleavedArrays = (RwUInt8 *)RwMalloc( interleavedArraySize,
                                          rwID_PATCHPLUGIN | rwMEMHINTDUR_EVENT);
        openglPatchSkinInterleavedArraySize = interleavedArraySize;
    }

    interleavedVertices = openglPatchSkinInterleavedArrays;
    interleavedNormals = (RwUInt8 *)(interleavedVertices + sizeof(RwV3d));

#if !defined( NOASM )

#if !defined( NOSSEASM )

    if ( FALSE != _rwIntelSSEsupported() )
    {
        RwUInt32    skinMaxWeights;


        skinMaxWeights = _rpSkinOpenGLSkinPlatformGetMaxWeights(skin);

        if ( skinMaxWeights > 2 )
        {
            _rpSkinIntelSSEMatrixBlend( geomNumVerts,
                                        RpSkinGetVertexBoneWeights( skin ),
                                        RpSkinGetVertexBoneIndices( skin ),
                                        matrixArray,
                                        interleavedVertices,
                                        originalVertices,
                                        interleavedNormals,
                                        originalNormals,
                                        vertexStride );
        }
        else if ( skinMaxWeights > 1 )
        {
            _rpSkinIntelSSEMatrixBlend2Weights( geomNumVerts,
                                                RpSkinGetVertexBoneWeights( skin ),
                                                RpSkinGetVertexBoneIndices( skin ),
                                                matrixArray,
                                                interleavedVertices,
                                                originalVertices,
                                                interleavedNormals,
                                                originalNormals,
                                                vertexStride );
        }
        else
        {
            /* weight is unity */
            _rpSkinIntelSSEMatrixBlend1Weight( geomNumVerts,
                                               RpSkinGetVertexBoneIndices( skin ),
                                               matrixArray,
                                               interleavedVertices,
                                               originalVertices,
                                               interleavedNormals,
                                               originalNormals,
                                               vertexStride );
        }
    }
    else
    {

#endif /* !defined( NOSSEASM ) */

        _rpSkinIntelx86MatrixBlend( geomNumVerts,
                                    RpSkinGetVertexBoneWeights( skin ),
                                    RpSkinGetVertexBoneIndices( skin ),
                                    matrixArray,
                                    (RwV3d *)interleavedVertices,
                                    originalVertices,
                                    (RwV3d *)interleavedNormals,
                                    originalNormals,
                                    vertexStride );

#if !defined( NOSSEASM )

    }

#endif /* !defined( NOSSEASM ) */

#else /* !defined( NOASM ) */

    _rpSkinGenericMatrixBlend( geomNumVerts,
                               RpSkinGetVertexBoneWeights( skin ),
                               RpSkinGetVertexBoneIndices( skin ),
                               matrixArray,
                               interleavedVertices,
                               originalVertices,
                               interleavedNormals,
                               originalNormals,
                               vertexStride );

#endif /* !defined( NOASM ) */

    /* now we need to copy the interleaved data into the streamed data */
    for ( i = 0; i < geomNumVerts; i += 1 )
    {
        RwV3dAssign( (RwV3d *)(vertices) + i, (RwV3d *)(interleavedVertices + vertexStride * i) );
        RwV3dAssign( (RwV3d *)(normals) + i, (RwV3d *)(interleavedNormals + vertexStride * i) );
    }

    RWRETURN( atomic );
}
