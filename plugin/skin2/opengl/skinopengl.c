/***********************************************************************
 *
 * Module:  skinopengl.c
 *
 * Purpose:
 *
 ***********************************************************************/

/* =====================================================================
 *  Includes
 * ===================================================================== */
#include <string.h> /* for memcpy */

#include "rwcore.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "skinopengl.h"
#include "skin.h"

#if defined( _WIN32 )
#include "x86matbl.h"

#if !defined( NOSSEASM )
#include "ssematbl.h"
#endif /* !defined( NOSSEASM ) */

#elif defined( MACOS )
#include "macmatbl.h"
#endif /* (defined( _WIN32) || defined( MACOS )) */


/* =====================================================================
 *  Defines
 * ===================================================================== */
#define FLOATASINT(f) (*((const RwInt32 *)&(f)))


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */


/* =====================================================================
 *  Static variables
 * ===================================================================== */
static const RwReal                         rpSkinOpenGLUnity   = 1.0f;

static const RwReal                         rpSkinOpenGLZero    = 0.0f;

static RxOpenGLAllInOneReinstanceCallBack   openglDefaultReinstanceCB = (RxOpenGLAllInOneReinstanceCallBack)NULL;


/* =====================================================================
 *  Global variables
 * ===================================================================== */
SkinGlobals _rpSkinGlobals =
{
    0,
    0,
    0,
    { (RwMatrix *)NULL, NULL },
    0,
    (RwFreeList *)NULL,
    { 0, 0 },
    {
        {                       /* SkinGlobalPlatform OpenGL platform */
            (RxPipeline *)NULL, /*  rpSKINOPENGLPIPELINEGENERIC       */
            (RxPipeline *)NULL, /*  rpSKINOPENGLPIPELINEMATFX         */
            (RxPipeline *)NULL, /*  rpSKINOPENGLPIPELINETOON          */
            (RxPipeline *)NULL  /*  rpSKINOPENGLPIPELINETOONMATFX     */
        },
        NULL,                   /*  Last hierarchy used */
        0,                      /*  Last render frame */
        NULL                    /*  Last frame */
    },
    (SkinSplitData *) NULL
};


/* =====================================================================
 *  Static function prototypes
 * ===================================================================== */
static RwBool
openglSkinAllInOneAtomicInstanceCB( void *object,
                                    RxOpenGLMeshInstanceData *instanceData,
                                    const RwBool instanceDLandVA,
                                    const RwBool reinstance );

static RwBool
openglSkinAllInOneAtomicReinstanceCB( void *object,
                                      RwResEntry *resEntry,
                                      const RpMeshHeader *meshHeader,
                                      const RwBool instanceDLandVA,
                                      RxOpenGLAllInOneInstanceCallBack instanceCB );


/* =====================================================================
 *  Global function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: _rpSkinPipelinesDestroy
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwBool
_rpSkinPipelinesDestroy( void )
{
    RpSkinOpenGLPipeline    pipeline;


    RWFUNCTION( RWSTRING( "_rpSkinPipelinesDestroy" ) );

    for ( pipeline = rpSKINOPENGLPIPELINEGENERIC;
          pipeline < rpSKINOPENGLPIPELINEMAX;
          pipeline += (RpSkinOpenGLPipeline)(1) )
    {
        if ( NULL != _rpSkinOpenGLPipeline( pipeline ) )
        {
            RxPipelineDestroy( _rpSkinOpenGLPipeline( pipeline ) );
            _rpSkinOpenGLPipeline( pipeline ) = (RxPipeline *)NULL;
        }
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rpSkinInitialize
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RpGeometry *
_rpSkinInitialize( RpGeometry *geometry )
{
    RpSkin *skin;


    RWFUNCTION( RWSTRING( "_rpSkinInitialize" ) );

    RWASSERT( NULL != geometry );

    skin = *RPSKINGEOMETRYGETDATA( geometry );
    if ( NULL != skin )
    {
        RwMatrixWeights *matrixWeights;

        RwUInt32        *matrixIndexMap;

        RwInt32         idx;


        matrixWeights = skin->vertexMaps.matrixWeights;

        matrixIndexMap = skin->vertexMaps.matrixIndices;

        skin->platformData.maxWeights = 1;

        for ( idx = 0; idx < RpGeometryGetNumVertices( geometry ); idx += 1 )
        {
            RWASSERT( FLOATASINT(matrixWeights->w0) > FLOATASINT(rpSkinOpenGLZero) );

            if ( FLOATASINT(matrixWeights->w0) < FLOATASINT(rpSkinOpenGLUnity) )
            {
                RwBool bCheck;


                /* Sort indices and weights */
#if defined(rwBIGENDIAN)
                /* order of the indices is reversed */
                do
                {
                    bCheck = FALSE;

                    if ( FLOATASINT(matrixWeights->w0) < FLOATASINT(matrixWeights->w1) )
                    {
                        RwUInt8 *indices = (RwUInt8 *)(matrixIndexMap + idx);

                        const RwUInt8 temp = indices[3];
                        const RwReal tempf = matrixWeights->w0;

                        indices[3] = indices[2];
                        indices[2] = temp;

                        matrixWeights->w0 = matrixWeights->w1;
                        matrixWeights->w1 = tempf;

                        bCheck = TRUE;
                    }

                    if ( FLOATASINT(matrixWeights->w1) < FLOATASINT(matrixWeights->w2) )
                    {
                        RwUInt8 *indices = (RwUInt8 *)(matrixIndexMap + idx);

                        const RwUInt8 temp = indices[2];
                        const RwReal tempf = matrixWeights->w1;

                        indices[2] = indices[1];
                        indices[1] = temp;

                        matrixWeights->w1 = matrixWeights->w2;
                        matrixWeights->w2 = tempf;

                        bCheck = TRUE;
                    }

                    if ( FLOATASINT(matrixWeights->w2) < FLOATASINT(matrixWeights->w3) )
                    {
                        RwUInt8 *indices = (RwUInt8 *)(matrixIndexMap + idx);

                        const RwUInt8 temp = indices[1];
                        const RwReal tempf = matrixWeights->w2;

                        indices[1] = indices[0];
                        indices[0] = temp;

                        matrixWeights->w2 = matrixWeights->w3;
                        matrixWeights->w3 = tempf;

                        bCheck = TRUE;
                    }
                }
                while( FALSE != bCheck );
#else /* defined(rwBIGENDIAN) */
                do
                {
                    bCheck = FALSE;

                    if ( FLOATASINT(matrixWeights->w0) < FLOATASINT(matrixWeights->w1) )
                    {
                        RwUInt8 *indices = (RwUInt8 *)(matrixIndexMap + idx);

                        const RwUInt8 temp = indices[0];
                        const RwReal tempf = matrixWeights->w0;

                        indices[0] = indices[1];
                        indices[1] = temp;

                        matrixWeights->w0 = matrixWeights->w1;
                        matrixWeights->w1 = tempf;

                        bCheck = TRUE;
                    }

                    if ( FLOATASINT(matrixWeights->w1) < FLOATASINT(matrixWeights->w2) )
                    {
                        RwUInt8 *indices = (RwUInt8 *)(matrixIndexMap + idx);

                        const RwUInt8 temp = indices[1];
                        const RwReal tempf = matrixWeights->w1;

                        indices[1] = indices[2];
                        indices[2] = temp;

                        matrixWeights->w1 = matrixWeights->w2;
                        matrixWeights->w2 = tempf;

                        bCheck = TRUE;
                    }

                    if ( FLOATASINT(matrixWeights->w2) < FLOATASINT(matrixWeights->w3) )
                    {
                        RwUInt8 *indices = (RwUInt8 *)(matrixIndexMap + idx);

                        const RwUInt8 temp = indices[2];
                        const RwReal tempf = matrixWeights->w2;

                        indices[2] = indices[3];
                        indices[3] = temp;

                        matrixWeights->w2 = matrixWeights->w3;
                        matrixWeights->w3 = tempf;

                        bCheck = TRUE;
                    }
                }
                while( FALSE != bCheck );
#endif /* defined(rwBIGENDIAN) */

                /* Check maximun number of weights */
                if ( FLOATASINT(matrixWeights->w3) > FLOATASINT(rpSkinOpenGLZero) )
                {
                    skin->platformData.maxWeights = 4;
                }
                else
                {
                    if ( FLOATASINT(matrixWeights->w2) > FLOATASINT(rpSkinOpenGLZero) )
                    {
                        if (skin->platformData.maxWeights < 3)
                        {
                            skin->platformData.maxWeights = 3;
                        }
                    }
                    else
                    {
                        if (skin->platformData.maxWeights < 2)
                        {
                            skin->platformData.maxWeights = 2;
                        }
                    }
                }
            }

            matrixWeights++;
        }

#if defined(RWDEBUG)
        switch ( skin->platformData.maxWeights )
        {
        case 1:
            {
                RWMESSAGE( (RWSTRING("Only ONE weight has been detected in this geometry")) );
            }
            break;

        case 2:
            {
                RWMESSAGE( (RWSTRING("If available, a 2-weight skinning algorithm will be used for this geometry")) );
            }
            break;

        case 3:
            {
                RWMESSAGE( (RWSTRING("If available, a 3-weight skinning algorithm will be used for this geometry")) );
            }
            break;

        case 4:
            {
                RWMESSAGE( (RWSTRING("Using the default 4-weight skinner for this geometry")) );
            }
            break;

        default:
            {
                RWASSERT( ( 1 >= skin->platformData.maxWeights ) &&
                          ( 4 <= skin->platformData.maxWeights ) );
            }
        }
#endif /* defined(RWDEBUG) */
    }

    RWRETURN( geometry );
}


/* ---------------------------------------------------------------------
 *  Function: _rpSkinDeinitialize
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RpGeometry *
_rpSkinDeinitialize( RpGeometry *geometry )
{
    RWFUNCTION( RWSTRING( "_rpSkinDeinitialize" ) );

    RWASSERT( NULL != geometry );

    RWRETURN( geometry );
}



/* ---------------------------------------------------------------------
 *  Function: _rpSkinGeometryNativeSize
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwInt32
_rpSkinGeometryNativeSize( const RpGeometry *geometry __RWUNUSEDRELEASE__ )
{
#if 0
    RwInt32         size;

    const RpSkin    *skin;
#endif /* 0 */

    RWFUNCTION( RWSTRING( "_rpSkinGeometryNativeSize" ) );

    RWASSERT( NULL != geometry );

#if 0
    /* Chunk header for version number + platform specific ID */
    size = rwCHUNKHEADERSIZE + sizeof(RwPlatformID);

    /* Num bones. */
    size += sizeof(RwUInt32);

    /* ... */

    RWRETURN(size);
#endif /* 0 */

    RWRETURN( 0 );
}


/* ---------------------------------------------------------------------
 *  Function: _rpSkinGeometryNativeWrite
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwStream *
_rpSkinGeometryNativeWrite( RwStream *stream,
                            const RpGeometry *geometry )
{
    RwPlatformID    id = rwID_PCOGL;

    RwInt32         size;

    const RpSkin    *skin;


    RWFUNCTION( RWSTRING( "_rpSkinGeometryNativeWrite" ) );

    RWASSERT( NULL != stream );
    RWASSERT( NULL != geometry );

    /* get size of data to write */
    size = _rpSkinGeometryNativeSize(geometry) - rwCHUNKHEADERSIZE;

    /* write a chunk header so we get a VERSION NUMBER */
    if ( NULL == RwStreamWriteChunkHeader(stream, rwID_STRUCT, size) )
    {
        RWRETURN( (RwStream *)NULL );
    }

    /* write a platform unique identifier */
    if ( NULL == RwStreamWriteInt32(stream, (void *)&id, sizeof(RwPlatformID)) )
    {
        RWRETURN( (RwStream *)NULL );
    }

    skin = *RPSKINGEOMETRYGETCONSTDATA(geometry);

    /* write the number of bones */
    if ( NULL == RwStreamWriteInt32( stream,
                                     (const RwInt32 *)&(skin->boneData.numBones),
                                     sizeof(RwUInt32)) )
    {
        RWRETURN( (RwStream *)NULL );
    }

    RWRETURN( stream );
}


/* ---------------------------------------------------------------------
 *  Function: _rpSkinGeometryNativeRead
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwStream *
_rpSkinGeometryNativeRead( RwStream *stream,
                           RpGeometry *geometry )
{
    RwUInt32        version;

    RwUInt32        size;

    RwPlatformID    id;

    RpSkin          *skin;


    RWFUNCTION( RWSTRING( "_rpSkinGeometryNativeRead" ) );

    RWASSERT( NULL != stream );
    RWASSERT( NULL != geometry );

    if ( FALSE == RwStreamFindChunk(stream, rwID_STRUCT, (RwUInt32 *)&size, &version) )
    {
        RWRETURN( (RwStream *)NULL );
    }

    if ( (version < rwLIBRARYBASEVERSION) ||
         (version > rwLIBRARYCURRENTVERSION) )
    {
        RWERROR( (E_RW_BADVERSION) );
        RWRETURN( (RwStream *)NULL );
    }

    /* read the platform unique identifier */
    if ( NULL == RwStreamReadInt32(stream, (void *)&id, sizeof(RwPlatformID)) )
    {
        RWRETURN( (RwStream *)NULL );
    }

    /* check this data is valid for this platform */
    if ( rwID_PCOGL != id )
    {
        RWASSERT( rwID_PCOGL == id );
        RWRETURN( (RwStream *)NULL );
    }

    /* create a new skin */
    skin = (RpSkin *)RwFreeListAlloc( _rpSkinGlobals.freeList,
                          rwID_SKINPLUGIN | rwMEMHINTDUR_EVENT );
    RWASSERT( NULL != skin );

    /* clean the skin */
    memset( skin, 0, sizeof(RpSkin) );

    /* read the number of bones */
    if ( NULL == RwStreamReadInt32( stream,
                                    (void *)&skin->boneData.numBones,
                                    sizeof(RwUInt32)) )
    {
        RWRETURN( (RwStream *)NULL );
    }

    RpSkinGeometrySetSkin( geometry, skin );

    RWRETURN( stream );
}


/* ---------------------------------------------------------------------
 *  Function: _rpSkinOpenGLPipelineCreate
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RxPipeline *
_rpSkinOpenGLPipelineCreate( const RpSkinType type,
                             RxOpenGLAllInOneRenderCallBack renderCB )
{
    RxPipeline  *pipe;


    RWFUNCTION( RWSTRING( "_rpSkinOpenGLPipelineCreate" ) );

    pipe = RxPipelineCreate();
    if ( NULL != pipe )
    {
        RxLockedPipe    *lockedPipe;


        pipe->pluginId = rwID_SKINPLUGIN;
        pipe->pluginData = type;

        lockedPipe = RxPipelineLock( pipe );
        if ( NULL != lockedPipe )
        {
            RxNodeDefinition    *allinoneNodeDef;


            allinoneNodeDef = RxNodeDefinitionGetOpenGLAtomicAllInOne();
            RWASSERT( NULL != allinoneNodeDef );

            lockedPipe = RxLockedPipeAddFragment( lockedPipe,
                                                  NULL,
                                                  allinoneNodeDef,
                                                  NULL );
            RWASSERT( NULL != lockedPipe );

            lockedPipe = RxLockedPipeUnlock( lockedPipe );

            RWASSERT( lockedPipe == pipe );

            if ( NULL != lockedPipe )
            {
                RxPipelineNode  *pipeNode;


                pipeNode = RxPipelineFindNodeByName( pipe, allinoneNodeDef->name, NULL, NULL );
                RWASSERT( NULL != pipeNode );


                RxOpenGLAllInOneSetInstanceCallBack( pipeNode, openglSkinAllInOneAtomicInstanceCB );

                openglDefaultReinstanceCB = RxOpenGLAllInOneGetReinstanceCallBack( pipeNode );
                RxOpenGLAllInOneSetReinstanceCallBack( pipeNode, openglSkinAllInOneAtomicReinstanceCB );

                if ( NULL != renderCB )
                {
                    RxOpenGLAllInOneSetRenderCallBack( pipeNode, renderCB );
                }

                RWRETURN( pipe );
            }
        }

        RxPipelineDestroy( pipe );
        pipe = (RxPipeline *)NULL;
    }

    RWRETURN( pipe );
}


/* ---------------------------------------------------------------------
 *  Function: _rpSkinOpenGLPrepareAtomicMatrix
 *
 *  Purpose : Wrapper on _rpSkinPrepareAtomicMatrix to eliminate SSE
 *            exceptions.
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwMatrix *
_rpSkinOpenGLPrepareAtomicMatrix( RpAtomic *atomic,
                                  RpSkin *skin,
                                  RpHAnimHierarchy *hierarchy )
{
    RWFUNCTION( RWSTRING( "_rpSkinOpenGLPrepareAtomicMatrix" ) );

    /* only prepare matrices when we really have to */
    if ( (hierarchy != _rpSkinGlobals.platform.lastHierarchyUsed ) ||
         (RWSRCGLOBAL(renderFrame) != _rpSkinGlobals.platform.lastRenderFrame) ||
         (RpAtomicGetFrame(atomic) != _rpSkinGlobals.platform.lastFrame) )
    {
        RwMatrix    *matrix;

        RwInt32     i;


        _rpSkinPrepareAtomicMatrix( atomic, skin, hierarchy );

        matrix = _rpSkinGlobals.matrixCache.aligned;
        for ( i = 0; i < hierarchy->numNodes; i += 1 )
        {
            /* zero the flags and padding to eliminate SSE exceptions */
            matrix->flags = 0;
            matrix->pad1  = 0;
            matrix->pad2  = 0;
            matrix->pad3  = 0;

            matrix += 1;
        }

        _rpSkinGlobals.platform.lastHierarchyUsed = hierarchy;
        _rpSkinGlobals.platform.lastRenderFrame = RWSRCGLOBAL(renderFrame);
        _rpSkinGlobals.platform.lastFrame = RpAtomicGetFrame(atomic);
    }

    RWRETURN( _rpSkinGlobals.matrixCache.aligned );
}


/* ---------------------------------------------------------------------
 *  Function: _rpSkinOpenGLSkinPlatformGetMaxWeights
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwUInt32
_rpSkinOpenGLSkinPlatformGetMaxWeights( const RpSkin *skin )
{
    RWFUNCTION( RWSTRING( "_rpSkinOpenGLSkinPlatformGetMaxWeights" ) );

    RWASSERT( NULL != skin );

    RWRETURN( skin->platformData.maxWeights );
}


/* =====================================================================
 *  Static function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: openglSkinAllInOneAtomicInstanceCB
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static RwBool
openglSkinAllInOneAtomicInstanceCB( void *object,
                                    RxOpenGLMeshInstanceData *instanceData,
                                    const RwBool instanceDLandVA __RWUNUSED__,
                                    const RwBool reinstance )
{
    RpAtomic            *atomic;

    const RpGeometry    *geometry;

    RwInt32             numTexCoords;

    RpGeometryFlag      geomFlags;

    RwInt32             numMorphTargets;

    RwInt32             startMT;

    RwInt32             endMT;

    RwReal              morphScale;


    RWFUNCTION( RWSTRING( "openglSkinAllInOneAtomicInstanceCB" ) );

    RWASSERT( NULL != object );
    RWASSERT( NULL != instanceData );

    /* get the atomic */
    atomic = (RpAtomic *)object;

    /* get the atomic's geometry */
    geometry = RpAtomicGetGeometry(atomic);
    RWASSERT( NULL != geometry );

    /* get the number of texture coordinates in this geometry */
    numTexCoords = RpGeometryGetNumTexCoordSets(geometry );

    /* geometry's flags */
    geomFlags = RpGeometryGetFlags(geometry);

    /* get the vertex descriptor */
    instanceData->vertexDesc = (RwUInt32)geomFlags;

    /* get the number of morph targets */
    numMorphTargets = RpGeometryGetNumMorphTargets(geometry);

    if ( 1 != numMorphTargets )
    {
        const RpInterpolator    *interp;


        interp = (const RpInterpolator *)RpAtomicGetInterpolator( atomic );
        RWASSERT( NULL != interp );

        startMT = RpInterpolatorGetStartMorphTarget(interp);
        endMT = RpInterpolatorGetEndMorphTarget(interp);

        if ( (startMT >= numMorphTargets) ||
             (endMT >= numMorphTargets) )
        {
            /* clamp */
            startMT = endMT = 0;
        }

        morphScale = interp->recipTime * interp->position;
    }
    else
    {
        startMT = endMT = 0;
        morphScale = 0.0f;
    }

    /* if anything has changed, we have to instance our geometry */
    if ( (0 != (geometry->lockedSinceLastInst & rpGEOMETRYLOCKALL)) ||
         (FALSE == reinstance) )
    {
        RwUInt8 *baseVertexMem;

        RwUInt8 *offset;

        RwInt32 idx;


        baseVertexMem = (RwUInt8 *)NULL;

        if ( FALSE == reinstance )
        {
            /* position */
            instanceData->vertexStride = sizeof(RwV3d);

            /* normals */
            if ( 0 != (geomFlags & rxGEOMETRY_NORMALS) )
            {
                instanceData->vertexStride += sizeof(RwV3d);
            }

            /* prelight */
            if ( 0 != (geomFlags & rxGEOMETRY_PRELIT) )
            {
                instanceData->vertexStride += sizeof(RwRGBA);
            }

            /* texture coordinates */
            instanceData->vertexStride += numTexCoords * sizeof(RwTexCoords);

            instanceData->vertexDataSize = instanceData->numVertices * instanceData->vertexStride;

            /* allocate a block either in video or system memory */
            if ( _rwOpenGLVertexHeapAvailable() )
            {
                RWASSERT( NULL == instanceData->sysMemVertexData );

                if ( NULL != instanceData->vidMemVertexData )
                {
                    _rwOpenGLVertexHeapDynamicDiscard( instanceData->vidMemVertexData );
                }
                /* always generate a fence on this block */
                instanceData->vidMemVertexData = (void *)_rwOpenGLVertexHeapDynamicMalloc( instanceData->vertexDataSize, TRUE );
                instanceData->vidMemDataIsStatic = FALSE;
                RWASSERT( NULL != instanceData->vidMemVertexData );

                baseVertexMem = instanceData->vidMemVertexData;
            }
            /* TODO: ATI support */
            else
            {
                RWASSERT( NULL == instanceData->vidMemVertexData );

                instanceData->sysMemVertexData = (RwUInt8 *)RwMalloc( instanceData->vertexDataSize,
                                                              rwID_SKINPLUGIN | rwMEMHINTDUR_EVENT);
                RWASSERT( NULL != instanceData->sysMemVertexData );

                baseVertexMem = instanceData->sysMemVertexData;
            }
        }
        else
        {
            baseVertexMem = instanceData->vidMemVertexData;
            if ( NULL == baseVertexMem )
            {
                baseVertexMem = instanceData->sysMemVertexData;
            }
        }
        RWASSERT( NULL != baseVertexMem );

        _rxOpenGLAllInOneAtomicInstanceVertexArray( instanceData,
                                                    atomic,
                                                    geometry,
                                                    geomFlags,
                                                    numTexCoords,
                                                    reinstance,
                                                    NULL,
                                                    baseVertexMem );

        offset = (RwUInt8 *)baseVertexMem;

        instanceData->position = offset;
        offset += sizeof(RwV3d);

        /* normals */
        if ( 0 != (geomFlags & rxGEOMETRY_NORMALS) )
        {
            instanceData->normal = offset;
            offset += sizeof(RwV3d);
        }

        /* prelight */
        if ( 0 != (geomFlags & rxGEOMETRY_PRELIT) )
        {
            instanceData->color = offset;
            offset += sizeof(RwRGBA);
        }

        /* texture coordinates */
        for ( idx = 0; idx < numTexCoords; idx += 1 )
        {
            instanceData->texCoord[idx] = offset;
            offset += sizeof(RwTexCoords);
        }
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: openglSkinAllInOneAtomicReinstanceCB
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static RwBool
openglSkinAllInOneAtomicReinstanceCB( void *object,
                                      RwResEntry *resEntry,
                                      const RpMeshHeader *meshHeader,
                                      const RwBool instanceDLandVA,
                                      RxOpenGLAllInOneInstanceCallBack instanceCB )
{
    RpAtomic                    *atomic;

    RpGeometry                  *geometry;

    RwInt32                     geomNumVerts;

    RxOpenGLResEntryHeader      *resEntryHdr;

    RxOpenGLMeshInstanceData    *instanceData;

    RpSkin                      *skin;

    RpHAnimHierarchy            *hierarchy;

    const RwMatrix              *matrixArray;

    const RwMatrixWeights       *boneWeights;

    const RwUInt32              *boneIndices;

    RwUInt32                    numMeshes;


    RWFUNCTION( RWSTRING( "openglSkinAllInOneAtomicReinstanceCB" ) );

    RWASSERT( NULL != object );
    RWASSERT( NULL != resEntry );
    RWASSERT( NULL != meshHeader );

    /* get the atomic */
    atomic = (RpAtomic *)object;

    /* get the atomic's geometry */
    geometry = RpAtomicGetGeometry( atomic );
    RWASSERT( NULL != geometry );

    geomNumVerts = RpGeometryGetNumVertices( geometry );

    resEntryHdr = (RxOpenGLResEntryHeader *)(resEntry + 1);
    RWASSERT( NULL != resEntryHdr );

    instanceData = (RxOpenGLMeshInstanceData *)(resEntryHdr + 1);
    RWASSERT( NULL != instanceData );

    /* call the default reinstance callback to get all the other updated
     * instance data, e.g. texcoords, or prelit colors */
    openglDefaultReinstanceCB( object, resEntry, meshHeader, instanceDLandVA, instanceCB );

    skin = RpSkinGeometryGetSkin( geometry );
    if ( NULL == skin )
    {
        RWRETURN( TRUE );
    }

    /* get hierarchy info */
    hierarchy = RpSkinAtomicGetHAnimHierarchy( atomic );
    if ( NULL == hierarchy )
    {
        RWRETURN( TRUE );
    }

    /* perform matrix blending */
    matrixArray = _rpSkinOpenGLPrepareAtomicMatrix( atomic,
                                                    skin,
                                                    hierarchy );
    RWASSERT( NULL != matrixArray );

    boneWeights = RpSkinGetVertexBoneWeights( skin );
    RWASSERT( NULL != boneWeights );

    boneIndices = RpSkinGetVertexBoneIndices( skin );
    RWASSERT( NULL != boneIndices );

    numMeshes = meshHeader->numMeshes;
    while ( numMeshes-- )
    {
        const RwV3d             *originalVertices;

        const RwV3d             *originalNormals;

        RwUInt8                 *baseVertexMem;

        RwUInt8                 *vertices;

        RwUInt8                 *normals;

        const RwMatrixWeights   *meshBoneWeights;

        const RwUInt32          *meshBoneIndices;


        /* offset the original vertices for this mesh */
        originalVertices = geometry->morphTarget[0].verts + instanceData->minVertexIdx;

        /* offset the normals (if they exist) for this mesh */
        originalNormals = geometry->morphTarget[0].normals;
        if ( NULL != originalNormals )
        {
            originalNormals += instanceData->minVertexIdx;
        }

        /* get the instanced vertex array data */
        baseVertexMem = instanceData->vidMemVertexData;
        if ( NULL == baseVertexMem )
        {
            baseVertexMem = instanceData->sysMemVertexData;
        }
        else
        {
            /* skinning data cannot be static */
            RWASSERT( FALSE == instanceData->vidMemDataIsStatic );

            if ( _rwOpenGLVertexHeapAvailable() )
            {
                /* if we're using nVIDIAs vertex array range extension
                 * then ensure that the previous render has finished */
                _rwOpenGLVertexHeapFinishNVFence( baseVertexMem );
            }
        }
        RWASSERT( NULL != baseVertexMem );
        RWASSERT( GL_FALSE == glIsList( instanceData->displayList ) );

        vertices = baseVertexMem;
        if ( NULL == originalNormals )
        {
            normals = NULL;
        }
        else
        {
            normals = baseVertexMem + sizeof(RwV3d);
        }

        /* offset the bones and weights for this mesh */
        meshBoneWeights = boneWeights + instanceData->minVertexIdx;
        meshBoneIndices = boneIndices + instanceData->minVertexIdx;

#if !defined( NOASM )

#if !defined( NOSSEASM )

        if ( FALSE != _rwIntelSSEsupported() )
        {
            if ( skin->platformData.maxWeights > 2 )
            {
                _rpSkinIntelSSEMatrixBlend( instanceData->numVertices,
                                            meshBoneWeights,
                                            meshBoneIndices,
                                            matrixArray,
                                            vertices,
                                            originalVertices,
                                            normals,
                                            originalNormals,
                                            instanceData->vertexStride );
            }
            else if ( skin->platformData.maxWeights > 1 )
            {
                _rpSkinIntelSSEMatrixBlend2Weights( instanceData->numVertices,
                                                    meshBoneWeights,
                                                    meshBoneIndices,
                                                    matrixArray,
                                                    vertices,
                                                    originalVertices,
                                                    normals,
                                                    originalNormals,
                                                    instanceData->vertexStride );
            }
            else
            {
                /* weight is unity */
                _rpSkinIntelSSEMatrixBlend1Weight( instanceData->numVertices,
                                                   meshBoneIndices,
                                                   matrixArray,
                                                   vertices,
                                                   originalVertices,
                                                   normals,
                                                   originalNormals,
                                                   instanceData->vertexStride );
            }
        }
        else
        {

#endif /* !defined( NOSSEASM ) */

            _rpSkinIntelx86MatrixBlend( instanceData->numVertices,
                                        meshBoneWeights,
                                        meshBoneIndices,
                                        matrixArray,
                                        (RwV3d *)vertices,
                                        originalVertices,
                                        (RwV3d *)normals,
                                        originalNormals,
                                        instanceData->vertexStride );

#if !defined( NOSSEASM )

        }

#endif /* !defined( NOSSEASM ) */

#else /* !defined( NOASM ) */

        _rpSkinGenericMatrixBlend( instanceData->numVertices,
                                   meshBoneWeights,
                                   meshBoneIndices,
                                   matrixArray,
                                   vertices,
                                   originalVertices,
                                   normals,
                                   originalNormals,
                                   instanceData->vertexStride );

#endif /* !defined( NOASM ) */

        /* next RxOpenGLMeshInstanceData */
        instanceData += 1;
    }

    RWRETURN( TRUE );
}

