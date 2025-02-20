/***********************************************************************
 *
 * Module:  patchopengl.c
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

#include "patch.h"
#include "patchatomic.h"
#include "patchgeom.h"

#include "patchopengl.h"


/* =====================================================================
 *  Defines
 * ===================================================================== */


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */


/* =====================================================================
 *  Static variables
 * ===================================================================== */

/* =====================================================================
 *  Global variables
 * ===================================================================== */
PatchGlobals _rpPatchGlobals =
{
    0,                     /* RwInt32        engineOffset      */
    0,                     /* RwInt32        atomicOffset      */
    0,                     /* RwInt32        geometryOffset    */
    { 0, 0 },              /* RwModuleInfo   module            */
    (RwFreeList *)NULL,    /* RwFreeList    *atomicFreeList    */
    (RwFreeList *)NULL,    /* RwFreeList    *geometryFreeList  */
    {                      /* PatchPlatform  platform          */
        {
            (RxPipeline *)NULL, /* RxPipeline * pipeline generic   */
            (RxPipeline *)NULL, /* RxPipeline * pipeline matfx     */
            (RxPipeline *)NULL, /* RxPipeline * pipeline skin      */
            (RxPipeline *)NULL, /* RxPipeline * pipeline skinmatfx */
        }
    }
};

RwUInt8 *openglPatchSkinInterleavedArrays = (RwUInt8 *)NULL;


/* =====================================================================
 *  Static function prototypes
 * ===================================================================== */


/* =====================================================================
 *  Global function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: _rpPatchPipelinesDestroy
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwBool
_rpPatchPipelinesDestroy( void )
{
    RpPatchOpenGLPipeline   openglPipe;


    RWFUNCTION( RWSTRING( "_rpPatchPipelinesDestroy" ) );

    for ( openglPipe = rpPATCHOPENGLPIPELINEGENERIC;
          openglPipe < rpPATCHOPENGLPIPELINEMAX;
          openglPipe += (RpPatchOpenGLPipeline)(1) )
    {
        if ( NULL != _rpPatchOpenGLPipeline(openglPipe) )
        {
            RxPipelineDestroy( _rpPatchOpenGLPipeline(openglPipe) );
            _rpPatchOpenGLPipeline(openglPipe) = (RxPipeline *)NULL;
        }
        RWASSERT( NULL == _rpPatchOpenGLPipeline(openglPipe) );
    }

    if ( NULL != openglPatchSkinInterleavedArrays )
    {
        RwFree( openglPatchSkinInterleavedArrays );
        openglPatchSkinInterleavedArrays = (RwUInt8 *)NULL;
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rpPatchOpenGLAtomicRenderCB
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RpAtomic*
_rpPatchOpenGLAtomicRenderCB( RpAtomic *atomic )
{
    RpGeometry      *geom;

    PatchAtomicData *atomicData;

    RpGeometry      *patchGeom;


    RWFUNCTION( RWSTRING( "_rpPatchOpenGLAtomicRenderCB" ) );

    RWASSERT( NULL != atomic );

    geom = RpAtomicGetGeometry(atomic);
    RWASSERT( NULL != geom );

    RWASSERT( 1 == RpGeometryGetNumMorphTargets(geom) );

    atomicData = PATCHATOMICGETDATA(atomic);
    RWASSERT( NULL != atomicData );

    if ( NULL == geom->repEntry )
    {
        patchGeom = _rpPatchToGeometry( atomic, NULL );
    }
    else
    {
        PatchMesh   *patchMesh;

        RwUInt32    numQuadPatches;

        RwUInt32    numTriPatches;

        RwInt32     patchLOD;

        RwUInt32    numVerts;


        (void)RwResourcesUseResEntry( geom->repEntry );

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

        /* we need to check that we don't need to recreate the patch */
        if ( (geom->mesh->serialNum != patchGeom->mesh->serialNum) ||
             (0 != geom->lockedSinceLastInst) ||
             (numVerts != _rpPatchOpenGLGetNumVerticesFromResEntry( patchGeom->repEntry )) )
        {
            /* might be better to orphan the ResEntry here */
            RwResourcesFreeResEntry( geom->repEntry );
            geom->lockedSinceLastInst = 0;
            patchGeom = _rpPatchToGeometry( atomic, NULL );
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


/* ---------------------------------------------------------------------
 *  Function: _rpPatchOpenGLCreateGenericPipe
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RxPipeline *
_rpPatchOpenGLCreateGenericPipe( void )
{
    RxPipeline  *pipe;


    RWFUNCTION( RWSTRING( "_rpPatchOpenGLCreateGenericPipe" ) );

    pipe = RxPipelineCreate();
    if ( NULL != pipe )
    {
        RxLockedPipe    *lockedPipe;


        pipe->pluginId = rwID_PATCHPLUGIN;
        pipe->pluginData = rpPATCHTYPEGENERIC;

        lockedPipe = RxPipelineLock( pipe );

        if ( NULL != lockedPipe )
        {
            lockedPipe = RxLockedPipeAddFragment( lockedPipe,
                                                  NULL,
                                                  RxNodeDefinitionGetOpenGLAtomicAllInOne(),
                                                  NULL );

            lockedPipe = RxLockedPipeUnlock(lockedPipe);

            RWASSERT( pipe == (RxPipeline *)lockedPipe );

            if ( NULL != lockedPipe )
            {
                _rpPatchOpenGLPipeline(rpPATCHOPENGLPIPELINEGENERIC) = pipe;

                RWRETURN( pipe );
            }
        }

        RxPipelineDestroy( pipe );
        pipe = (RxPipeline *)NULL;
    }

    RWRETURN( pipe );
}


/* ---------------------------------------------------------------------
 *  Function: _rpPatchOpenGLGetNumVerticesFromResEntry
 *
 *  Purpose : This is buggy!
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwUInt32
_rpPatchOpenGLGetNumVerticesFromResEntry( const RwResEntry *resEntry )
{
    const RxOpenGLResEntryHeader    *resEntryHeader;

    const RxOpenGLMeshInstanceData  *instanceData;

    RwInt32                         numMeshes;

    RwUInt32                        numVerts;


    RWFUNCTION( RWSTRING( "_rpPatchOpenGLGetNumVerticesFromResEntry" ) );

    RWASSERT( NULL != resEntry );

    /* get the instanced data */
    resEntryHeader = (const RxOpenGLResEntryHeader *)(resEntry + 1);
    RWASSERT( NULL != resEntryHeader );

    instanceData = (const RxOpenGLMeshInstanceData *)(resEntryHeader + 1);

    /* get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;
    numVerts = 0;
    while ( numMeshes-- )
    {
        numVerts += instanceData->numVertices;

        instanceData += 1;
    }

    RWRETURN( numVerts );
}



/* =====================================================================
 *  Static function definitions
 * ===================================================================== */

