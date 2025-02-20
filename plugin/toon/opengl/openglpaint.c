/***********************************************************************
 *
 * Module:  openglpaint.c
 *
 * Purpose: OpenGL Toon Implementation
 *
 ***********************************************************************/

/* =====================================================================
 *  Includes
 * ===================================================================== */
#include "rwcore.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "toonink.h"
#include "toonpaint.h"
#include "toonmaterial.h"
#include "toongeo.h"
#include "toon.h"
#include "genericedgerender.h"
#include "brutesilhouette.h"
#include "crease.h"
#include "pipes.h"

#include "openglpaint.h"


/* =====================================================================
 *  Defines
 * ===================================================================== */
#if defined(RWMETRICS)
#define UPDATERENDERCBMETRICS(_instData)                    \
MACRO_START                                                 \
    switch ( (_instData)->primType )                        \
    {                                                       \
    case GL_TRIANGLES:                                      \
        {                                                   \
            if ( NULL != (_instData)->indexData )           \
            {                                               \
                RWSRCGLOBAL(metrics)->numProcTriangles +=   \
                    (_instData)->numIndices / 3;            \
            }                                               \
            else                                            \
            {                                               \
                RWSRCGLOBAL(metrics)->numProcTriangles +=   \
                    (_instData)->numVertices / 3;           \
            }                                               \
        }                                                   \
        break;                                              \
                                                            \
    case GL_TRIANGLE_STRIP:                                 \
    case GL_TRIANGLE_FAN:                                   \
        {                                                   \
            if ( NULL != (_instData)->indexData )           \
            {                                               \
                RWSRCGLOBAL(metrics)->numProcTriangles +=   \
                    (_instData)->numIndices - 2;            \
            }                                               \
            else                                            \
            {                                               \
                RWSRCGLOBAL(metrics)->numProcTriangles +=   \
                    (_instData)->numVertices - 2;           \
            }                                               \
        }                                                   \
        break;                                              \
    }                                                       \
MACRO_STOP
#else /* defined(RWMETRICS) */
#define UPDATERENDERCBMETRICS(_instData)
#endif /* defined(RWMETRICS) */


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */
typedef void (*openglPaintRenderTypeFn)( const RxOpenGLMeshInstanceData *instanceData,
                                         const RwUInt32 flags,
                                         const RpToonPaint *paint );


/* =====================================================================
 *  Static variables
 * ===================================================================== */
static RxPipeline               *openglToonAtomicPipeline = (RxPipeline *)NULL;

static RxPipeline               *openglToonSectorPipeline = (RxPipeline *)NULL;

static openglPaintRenderTypeFn  openglToonPaintRender[RPTOON_PAINTTYPE_COUNT];

static RwMatrix                 openglToonTextureTransform;

static RwV3d                    *openglToonPosAndNormalArray = (RwV3d *)NULL;

static RxOpenGLMeshInstanceData **openglToonSortedInstanceDataArray = (RxOpenGLMeshInstanceData **)NULL;

static RwV3d                    *openglToonTexCoordsBlock = (RwV3d *)NULL;

static RwUInt32                 openglToonUnitCamSpaceNormalsVAO = 0;



/* =====================================================================
 *  Global variables
 * ===================================================================== */


/* =====================================================================
 *  Static function prototypes
 * ===================================================================== */
static RxPipeline *
openglToonCreatePipeline( const RwUInt32 type );

static RpAtomic *
openglToonAtomicRender( RpAtomic *atomic );

static RpWorldSector *
openglToonSectorRender( RpWorldSector *sector );

static void
openglToonFlatOpen( const RxOpenGLMeshInstanceData *instanceData,
                    const RwUInt32 flags,
                    const RpToonPaint *paint );

/* multi-pass, no texture gen */
static void
openglToonShadeOpenMP( const RxOpenGLMeshInstanceData *instanceData,
                       const RwUInt32 flags,
                       const RpToonPaint *paint );

/* multi-pass, with texture gen */
static void
openglToonShadeOpenMPCM( const RxOpenGLMeshInstanceData *instanceData,
                         const RwUInt32 flags,
                         const RpToonPaint *paint );

/* multitexture, no texture gen */
static void
openglToonShadeOpenMT( const RxOpenGLMeshInstanceData *instanceData,
                       const RwUInt32 flags,
                       const RpToonPaint *paint );

/* multitexture, with texture gen */
static void
openglToonShadeOpenMTCM( const RxOpenGLMeshInstanceData *instanceData,
                         const RwUInt32 flags,
                         const RpToonPaint *paint );

static RpLight *
openglToonFindFirstGlobalLight();

static void
openglToonUpdateCameraSpaceLightMatrix( void );

static int
openglToonMeshMinVertexQSortCB( const void *elem1, const void *elem2 );

static void *
openglToonCalculateToonTexCoords( const RxOpenGLMeshInstanceData *instanceData );


/* =====================================================================
 *  Global function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: _rpToonPipelinesCreate
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwBool
_rpToonPipelinesCreate( void )
{
    RWFUNCTION( RWSTRING( "_rpToonPipelinesCreate" ) );

    /* create a toon atomic pipeline */
    openglToonAtomicPipeline = openglToonCreatePipeline( rpATOMIC );

    /* create a toon world sector pipeline */
    openglToonSectorPipeline = openglToonCreatePipeline( rwSECTORATOMIC );

    /* initialize the edge renderer */
    _rpToonGenericEdgeRenderInit();
    _rpToonBruteSilhouetteInit();

    openglToonPaintRender[RPTOON_PAINTTYPE_FLAT] = openglToonFlatOpen;

    /* choose the appropriate rendering code according to the capabilities
     * of the video card */
    if ( _rwOpenGLExt.MaxTextureUnits < 2 )
    {
        if ( FALSE == _rwOpenGLExt.TextureCubeMapARB )
        {
            if ( _rwOpenGLVertexHeapAvailable() )
            {
                RWMESSAGE( (RWSTRING( "Without the GL_TextureCubeMapARB extension, toon will run slowly since we must read from video memory" )) );
            }
            openglToonPaintRender[RPTOON_PAINTTYPE_TOONSHADE] = openglToonShadeOpenMP;
        }
        else
        {
            openglToonPaintRender[RPTOON_PAINTTYPE_TOONSHADE] = openglToonShadeOpenMPCM;
        }
    }
    else
    {
        if ( FALSE == _rwOpenGLExt.TextureCubeMapARB )
        {
            if ( _rwOpenGLVertexHeapAvailable() )
            {
                RWMESSAGE( (RWSTRING( "Without the GL_TextureCubeMapARB extension, toon will run slowly since we must read from video memory" )) );
            }
            openglToonPaintRender[RPTOON_PAINTTYPE_TOONSHADE] = openglToonShadeOpenMT;
        }
        else
        {
            openglToonPaintRender[RPTOON_PAINTTYPE_TOONSHADE] = openglToonShadeOpenMTCM;
        }
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rpToonPipelinesDestroy
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwBool
_rpToonPipelinesDestroy( void )
{
    RWFUNCTION( RWSTRING( "_rpToonPipelinesDestroy" ) );

    /* free the soft-calculated tex coords block */
    if ( NULL != openglToonTexCoordsBlock )
    {
        RwFree( openglToonTexCoordsBlock );
        openglToonTexCoordsBlock = (RwV3d *)NULL;
    }

    /* free the local position and normal arrays */
    if ( NULL != openglToonPosAndNormalArray )
    {
        RwFree( openglToonPosAndNormalArray );
        openglToonPosAndNormalArray = (RwV3d *)NULL;
    }

    if ( NULL != openglToonSortedInstanceDataArray )
    {
        RwFree( openglToonSortedInstanceDataArray );
        openglToonSortedInstanceDataArray = (RxOpenGLMeshInstanceData **)NULL;
    }

    /* free the VAO */
    if ( NULL != _rwOpenGLExt.FreeObjectBufferATI )
    {
        if ( GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( openglToonUnitCamSpaceNormalsVAO ) )
        {
            _rwOpenGLExt.FreeObjectBufferATI( openglToonUnitCamSpaceNormalsVAO );
            openglToonUnitCamSpaceNormalsVAO = 0;
        }
    }

    _rpToonBruteSilhouetteClean();
    _rpToonGenericEdgeRenderClean();

    /* destroy the toon atomic pipeline */
    if ( NULL != openglToonAtomicPipeline )
    {
        RxPipelineDestroy( openglToonAtomicPipeline );
        openglToonAtomicPipeline = (RxPipeline *)NULL;
    }

    /* destroy the toon world sector pipeline */
    if ( NULL != openglToonSectorPipeline )
    {
        RxPipelineDestroy( openglToonSectorPipeline );
        openglToonSectorPipeline = (RxPipeline *)NULL;
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rpToonAtomicPipelinesAttach
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwBool
_rpToonAtomicPipelinesAttach( RpAtomic *atomic )
{
    RpAtomic    *success;


    RWFUNCTION( RWSTRING( "_rpToonAtomicPipelinesAttach" ) );

    RWASSERT( NULL != atomic );
    RWASSERT( NULL != openglToonAtomicPipeline );

    success = RpAtomicSetPipeline( atomic, openglToonAtomicPipeline );

    _rpToonAtomicChainAtomicRenderCallback( atomic );

    RWRETURN( NULL != success );
}


/* ---------------------------------------------------------------------
 *  Function: _rpToonSectorPipelinesAttach
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwBool
_rpToonSectorPipelinesAttach( RpWorldSector *sector )
{
    RpWorldSector   *success;

    RpToonGeo       *toonGeo;

    RpWorld         *world;

    static RpWorld  *lastWorld = (RpWorld *)NULL;

    static RpWorldSectorCallBackRender  defSectorRenderCallback = (RpWorldSectorCallBackRender)NULL;


    RWFUNCTION( RWSTRING( "_rpToonSectorPipelinesAttach" ) );

    RWASSERT( NULL != sector );
    RWASSERT( NULL != openglToonSectorPipeline );

    success = RpWorldSectorSetPipeline( sector, openglToonSectorPipeline );
    if ( NULL == success )
    {
        RWRETURN( FALSE );
    }

    toonGeo = RpToonWorldSectorGetToonGeo( sector );
    RWASSERT( NULL != toonGeo );

    world = RpWorldSectorGetWorld( sector );
    RWASSERT( NULL != world );

    if ( NULL == lastWorld )
    {
        lastWorld = world;
        defSectorRenderCallback = RpWorldGetSectorRenderCallBack( world );
    }

    RWASSERT( world == lastWorld );

    toonGeo->defSectorRenderCallback = defSectorRenderCallback;

    RpWorldSetSectorRenderCallBack( world, openglToonSectorRender );

    toonGeo->defAtomicRenderCallback = (RpAtomicCallBackRender)NULL;

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rpToonAtomicChainAtomicRenderCallback
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
void
_rpToonAtomicChainAtomicRenderCallback( RpAtomic *atomic )
{
    RpGeometry  *geometry;

    RpToonGeo   *toonGeo;


    RWFUNCTION( RWSTRING( "_rpToonAtomicChainAtomicRenderCallback" ) );

    RWASSERT( NULL != atomic );

    geometry = RpAtomicGetGeometry( atomic );
    RWASSERT( NULL != geometry );

    toonGeo = RpToonGeometryGetToonGeo( geometry );
    RWASSERT( NULL != toonGeo );

    if ( openglToonAtomicRender != RpAtomicGetRenderCallBack( atomic ) )
    {
        toonGeo->defAtomicRenderCallback = RpAtomicGetRenderCallBack( atomic );
        toonGeo->defSectorRenderCallback = (RpWorldSectorCallBackRender)NULL;

        RpAtomicSetRenderCallBack( atomic, openglToonAtomicRender );
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rpToonAtomicChainSkinnedAtomicRenderCallback
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
void
_rpToonAtomicChainSkinnedAtomicRenderCallback( RpAtomic *atomic )
{
    RpGeometry  *geometry;

    RpToonGeo   *toonGeo;


    RWFUNCTION( RWSTRING( "_rpToonAtomicChainSkinnedAtomicRenderCallback" ) );

    RWASSERT( NULL != atomic );

    geometry = RpAtomicGetGeometry( atomic );
    RWASSERT( NULL != geometry );

    toonGeo = RpToonGeometryGetToonGeo( geometry );
    RWASSERT( NULL != toonGeo );

    toonGeo->isSkinned = TRUE;

    _rpToonAtomicChainAtomicRenderCallback( atomic );

    RWRETURNVOID();
}



/* ---------------------------------------------------------------------
 *  Function: _rpToonOpenGLAllInOneRenderCB
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
void
_rpToonOpenGLAllInOneRenderCB( RwResEntry *repEntry,
                               void *object,
                               const RwUInt8 type,
                               const RwUInt32 flags )
{
    RpToonGeo                   *toonGeo;

    RxOpenGLResEntryHeader      *resEntryHeader;

    RxOpenGLMeshInstanceData    *instanceData;

    RwUInt16                    numMeshes;


    RWFUNCTION( RWSTRING( "_rpToonOpenGLAllInOneRenderCB" ) );

    RWASSERT( NULL != object );

    /* get the toon geometry */
    if ( rpATOMIC == type )
    {
        RpAtomic    *atomic;

        RpGeometry  *geometry;


        atomic = (RpAtomic *)object;

        geometry = RpAtomicGetGeometry( atomic );
        RWASSERT( NULL != geometry );

        toonGeo = RpToonGeometryGetToonGeo( geometry );
        RWASSERT( NULL != toonGeo );
    }
    else
    {
        RpWorldSector   *sector;


        sector = (RpWorldSector *)object;

        toonGeo = RpToonWorldSectorGetToonGeo( sector );
        RWASSERT( NULL != toonGeo );
    }

    RWASSERT( NULL != toonGeo->paint );

    /* Update camera space light matrix if needed */
    if ( 0 != (flags & rxGEOMETRY_LIGHT) )
    {
        openglToonUpdateCameraSpaceLightMatrix();
    }

    /* get the instanced data */
    resEntryHeader = (RxOpenGLResEntryHeader *)(repEntry + 1);
    RWASSERT( NULL != resEntryHeader );

    instanceData = (RxOpenGLMeshInstanceData *)(resEntryHeader + 1);
    RWASSERT( NULL != instanceData );

    numMeshes = resEntryHeader->numMeshes;
    while ( numMeshes-- )
    {
        const RwRGBA            *matColor;

        const RpToonMaterial    *toonMaterial;

        RpToonPaint             *toonPaint;

        RpToonPaintType         toonPaintType;


        RWASSERT( NULL != instanceData->material );

        matColor = RpMaterialGetColor( instanceData->material );
        RWASSERT( NULL != matColor );

        if ( (FALSE != instanceData->vertexAlpha) ||
             (0xFF != matColor->alpha) )
        {
            _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE );
        }
        else
        {
            _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)FALSE );
        }

        toonMaterial = *RPTOONMATERIALGETCONSTDATA( instanceData->material );

        if ( (NULL != toonMaterial) &&
             (FALSE != toonMaterial->overrideGeometryPaint) &&
             (NULL != toonMaterial->paint) )
        {
            toonPaint = toonMaterial->paint;
        }
        else
        {
            toonPaint = toonGeo->paint;
        }

        toonPaintType = toonPaint->type;

        if ( (0 == (flags & rxGEOMETRY_LIGHT)) ||
             ((RPTOON_PAINTTYPE_TOONSHADE == toonPaintType) &&
              (0 == toonPaint->gradient)) )
        {
            toonPaintType = RPTOON_PAINTTYPE_FLAT;
        }

        (openglToonPaintRender[toonPaintType])( instanceData, flags, toonPaint );

        /* next RxOpenGLMeshInstanceData pointer */
        instanceData += 1;
    }

    RWRETURNVOID();
}


/* =====================================================================
 *  Static function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: openglToonCreatePipeline
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static RxPipeline *
openglToonCreatePipeline( const RwUInt32 type )
{
    RxPipeline  *pipe;


    RWFUNCTION( RWSTRING( "openglToonCreatePipeline" ) );

    pipe = RxPipelineCreate();
    if ( NULL != pipe )
    {
        RxLockedPipe    *lockedPipe;


        pipe->pluginId = rwID_TOONPLUGIN;

        lockedPipe = RxPipelineLock( pipe );
        if ( NULL != lockedPipe )
        {
            RxNodeDefinition    *allinoneNodeDef;


            if ( rpATOMIC == type )
            {
                allinoneNodeDef = RxNodeDefinitionGetOpenGLAtomicAllInOne();
            }
            else
            {
                allinoneNodeDef = RxNodeDefinitionGetOpenGLWorldSectorAllInOne();
            }
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


                pipeNode = RxPipelineFindNodeByName( pipe,
                                                     allinoneNodeDef->name,
                                                     NULL,
                                                     NULL );
                RWASSERT( NULL != pipeNode );

                /* remove the lighting callback */
                RxOpenGLAllInOneSetLightingCallBack( pipeNode,
                                                     (RxOpenGLAllInOneLightingCallBack)NULL );

                /* toon render callback */
                RxOpenGLAllInOneSetRenderCallBack( pipeNode, _rpToonOpenGLAllInOneRenderCB );

                /* we need both display lists and vertex arrays */
                RxOpenGLAllInOneSetInstanceDLandVA( pipeNode, TRUE );

                RWRETURN( pipe );
            }
        }

        RxPipelineDestroy( pipe );
        pipe = (RxPipeline *)NULL;
    }

    RWRETURN( pipe );
}


/* ---------------------------------------------------------------------
 *  Function: openglToonAtomicRender
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static RpAtomic *
openglToonAtomicRender( RpAtomic *atomic )
{
    RpGeometry      *geometry;

    RpToonGeo       *toonGeo;

    RwV3d           *positions;

    RwV3d           *normals;

    RwBool          faceNormalsInvalid;

    static RwUInt32 posAndNormalArraySize = 0;

    static RwUInt32 cachedNumMeshes = 0;

    RwFrame         *atomicFrame;

    RwMatrix        *lineTransform;


    RWFUNCTION( RWSTRING( "openglToonAtomicRender" ) );

    RWASSERT( NULL != atomic );

    geometry = RpAtomicGetGeometry( atomic );
    RWASSERT( NULL != geometry );

    toonGeo = RpToonGeometryGetToonGeo( geometry );
    RWASSERT( NULL != toonGeo );

    /* custom atomic pipeline */
    (*(toonGeo->defAtomicRenderCallback))( atomic );

    faceNormalsInvalid = FALSE;

    /* have we got static or dynamic vertices? */
    if ( (FALSE == toonGeo->isSkinned) &&
         (1 == RpGeometryGetNumMorphTargets(geometry)) )
    {
        RpMorphTarget   *morph;


        /* static */
        morph = RpGeometryGetMorphTarget( geometry, 0 );
        RWASSERT( NULL != morph );

        positions = RpMorphTargetGetVertices( morph );
        normals = toonGeo->extrusionVertexNormals;

        /* maybe dmorph or some other thing going on in the render callback chain */
        if ( FALSE != toonGeo->vertexPositionsDirty )
        {
            faceNormalsInvalid = TRUE;

            if ( NULL != RpMorphTargetGetVertexNormals( morph ) )
            {
                normals = RpMorphTargetGetVertexNormals( morph );
            }

            toonGeo->vertexPositionsDirty = FALSE;
        }
    }
    else
    {
        RwResEntry                  *resEntry;

        RxOpenGLResEntryHeader      *resEntryHeader;

        RxOpenGLMeshInstanceData    *instanceData;

        RwUInt32                    geomNumVertices;

        RwUInt32                    numMeshes;

        RwUInt32                    offset;


        /* dynamic */

        /* if the geometry has more than one morph target the resEntry in the
         * atomic is used else the resEntry in the geometry */
        if ( 1 != RpGeometryGetNumMorphTargets(geometry) )
        {
            resEntry = atomic->repEntry;
        }
        else
        {
            resEntry = geometry->repEntry;
        }
        RWASSERT( NULL != resEntry );

        /* get the instanced data */
        resEntryHeader = (RxOpenGLResEntryHeader *)(resEntry + 1);
        RWASSERT( NULL != resEntryHeader );

        instanceData = (RxOpenGLMeshInstanceData *)(resEntryHeader + 1);
        RWASSERT( NULL != instanceData );

        /* there shouldn't be a display list for dynamic data */
        RWASSERT( 0 == instanceData->displayList );

        geomNumVertices = RpGeometryGetNumVertices( geometry );

        /* re-use the position and normal arrays where possible */
        if ( 2 * geomNumVertices * sizeof(RwV3d) > posAndNormalArraySize )
        {
            posAndNormalArraySize = 2 * geomNumVertices * sizeof(RwV3d);
            if ( NULL != openglToonPosAndNormalArray )
            {
                RwFree( openglToonPosAndNormalArray );
            }
            openglToonPosAndNormalArray = (RwV3d *)RwMalloc( posAndNormalArraySize,
                rwID_TOONPLUGIN | rwMEMHINTDUR_EVENT);
            RWASSERT( NULL != openglToonPosAndNormalArray );
        }

        numMeshes = resEntryHeader->numMeshes;

        /* re-use the sorted instance data array memory where possible */
        if ( numMeshes > cachedNumMeshes )
        {
            cachedNumMeshes = numMeshes;
            if ( NULL != openglToonSortedInstanceDataArray )
            {
                RwFree( openglToonSortedInstanceDataArray );
            }
            openglToonSortedInstanceDataArray = (RxOpenGLMeshInstanceData **)RwMalloc( sizeof(RxOpenGLMeshInstanceData*) * cachedNumMeshes,
                                                 rwID_TOONPLUGIN | rwMEMHINTDUR_EVENT );
            RWASSERT( NULL != openglToonSortedInstanceDataArray );
        }

        /* fill the instance data array */
        while ( numMeshes-- )
        {
            openglToonSortedInstanceDataArray[numMeshes] = instanceData;
            instanceData += 1;
        }

        numMeshes = resEntryHeader->numMeshes;

        /* sort the array, so that the instance mesh with the largest minVertexIdx is first
         * and the instance mesh with zero minVertexIdx is last */
        qsort( (void *)openglToonSortedInstanceDataArray,
               numMeshes,
               sizeof(RxOpenGLMeshInstanceData*),
               openglToonMeshMinVertexQSortCB );

        offset = 0;
        while ( numMeshes-- )
        {
            RxOpenGLMeshInstanceData    *curMesh;

            RwUInt8     *vertexMem;

            RwUInt32    i;


            /* we access the sorted instance data array in reverse, hence the need
             * for it to be in descending order */
            curMesh = openglToonSortedInstanceDataArray[numMeshes];

            vertexMem = curMesh->vidMemVertexData;
            if ( NULL == vertexMem )
            {
                vertexMem = curMesh->sysMemVertexData;
            }
            RWASSERT( NULL != vertexMem );

            for ( i = 0; i < curMesh->numVertices; i += 1 )
            {
                openglToonPosAndNormalArray[offset + i] = *(RwV3d *)vertexMem;
                openglToonPosAndNormalArray[offset + i + geomNumVertices] = *(RwV3d *)(vertexMem + sizeof(RwV3d));

                vertexMem += curMesh->vertexStride;
            }

            offset += curMesh->numVertices;
        }
        RWASSERT( offset == geomNumVertices );

        positions = (RwV3d *)openglToonPosAndNormalArray;
        normals = (RwV3d *)(openglToonPosAndNormalArray + geomNumVertices);

        faceNormalsInvalid = TRUE;
    }

    atomicFrame = RpAtomicGetFrame( atomic );
    RWASSERT( NULL != atomicFrame );

    lineTransform = RwFrameGetLTM( atomicFrame );

    _rpToonUpdateAtomicPerspectiveScale( toonGeo, atomic );

    /* edge detection & render */
    _rpToonGenericEdgeRenderPushState();
    _rpToonBruteSilhouetteRender( toonGeo, positions, normals, faceNormalsInvalid, lineTransform );
    _rpToonCreaseRender( toonGeo, positions, lineTransform );
    _rpToonGenericEdgeRenderPopState();

    RWRETURN( atomic );
}


/* ---------------------------------------------------------------------
 *  Function: openglToonSectorRender
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static RpWorldSector *
openglToonSectorRender( RpWorldSector *sector )
{
    RpToonGeo   *toonGeo;


    RWFUNCTION( RWSTRING( "openglToonSectorRender" ) );

    RWASSERT( NULL != sector );

    toonGeo = RpToonWorldSectorGetToonGeo( sector );
    RWASSERT( NULL != toonGeo );

    /* cell shade set up paint render with custom pipeline */
    (*(toonGeo->defSectorRenderCallback))(sector);

    /* sector might be a space filling one */
    if ( 0 != toonGeo->numVerts )
    {
        RwMatrix transform;


        RwMatrixSetIdentity( &transform );

        RWASSERT( NULL != sector->vertices );

        _rpToonUpdateWorldPerspectiveScale(toonGeo);

        _rpToonGenericEdgeRenderPushState();
        _rpToonBruteSilhouetteRender( toonGeo, sector->vertices, toonGeo->extrusionVertexNormals, FALSE,  &transform );
        _rpToonCreaseRender( toonGeo, sector->vertices, &transform );
        _rpToonGenericEdgeRenderPopState();
    }

    RWRETURN( sector );
}


/* ---------------------------------------------------------------------
 *  Function: openglToonFlatOpen
 *
 *  Purpose : Flat toon renderer.
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglToonFlatOpen( const RxOpenGLMeshInstanceData *instanceData,
                    const RwUInt32 flags,
                    const RpToonPaint *paint __RWUNUSED__ )
{
    RwTexture   *baseTexture;


    RWFUNCTION( RWSTRING( "openglToonFlatOpen" ) );

    /* bind the base texture */
    baseTexture = RpMaterialGetTexture( instanceData->material );
    if ( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))) &&
         (NULL != baseTexture) )
    {
        RwOpenGLSetTexture( baseTexture );
    }
    else
    {
        RwOpenGLSetTexture( NULL );
    }

    /* use the material color or not depending on the flags */
    if ( 0 != (flags & rxGEOMETRY_MODULATE) )
    {
        glColor4ubv( (const GLubyte *)&(instanceData->material->color) );
    }
    else
    {
        glColor4ub( 0xFF, 0xFF, 0xFF, 0xFF );
    }

    if ( 0 != instanceData->displayList )
    {
        RWASSERT( GL_FALSE != glIsList( instanceData->displayList ) );

        /* execute the display list */
        glCallList( instanceData->displayList );
    }
    else if ( (NULL != _rwOpenGLExt.ArrayObjectATI) &&
              (GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName )) )
    {
        RWASSERT( GL_FALSE !=
                  _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName ) );

        /* set up client state pointers */
        RwOpenGLVASetPositionATI( 3,
                                  GL_FLOAT,
                                  instanceData->vertexStride,
                                  instanceData->vaoName,
                                  instanceData->position );

        RwOpenGLVASetNormalATI( (0 != (flags & rxGEOMETRY_NORMALS)),
                                GL_FLOAT,
                                instanceData->vertexStride,
                                instanceData->vaoName,
                                instanceData->normal );

        RwOpenGLVASetColorATI( (0 != (flags & rxGEOMETRY_PRELIT)),
                               4,
                               GL_UNSIGNED_BYTE,
                               instanceData->vertexStride,
                               instanceData->vaoName,
                               instanceData->color );

        RwOpenGLVASetTexCoordATI( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))) &&
                                  (NULL != baseTexture),
                                  2,
                                  GL_FLOAT,
                                  instanceData->vertexStride,
                                  instanceData->vaoName,
                                  instanceData->texCoord[0] );

        /* render */
        if ( NULL != instanceData->indexData )
        {
            glDrawElements( instanceData->primType,
                            instanceData->numIndices,
                            GL_UNSIGNED_INT,
                            (const GLvoid *)(instanceData->indexData) );
        }
        else
        {
            glDrawArrays( instanceData->primType,
                          instanceData->minVertexIdx,
                          instanceData->numVertices );
        }
    }
    else
    {
        /* set up client state pointers */
        RwOpenGLVASetPosition( 3,
                               GL_FLOAT,
                               instanceData->vertexStride,
                               instanceData->position );

        RwOpenGLVASetNormal( (0 != (flags & rxGEOMETRY_NORMALS)),
                             GL_FLOAT,
                             instanceData->vertexStride,
                             instanceData->normal );

        RwOpenGLVASetColor( (0 != (flags & rxGEOMETRY_PRELIT)),
                            4,
                            GL_UNSIGNED_BYTE,
                            instanceData->vertexStride,
                            instanceData->color );

        RwOpenGLVASetTexCoord( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))) &&
                               (NULL != baseTexture),
                               2,
                               GL_FLOAT,
                               instanceData->vertexStride,
                               instanceData->texCoord[0] );

        /* render */
        if ( NULL != instanceData->indexData )
        {
            glDrawElements( instanceData->primType,
                            instanceData->numIndices,
                            GL_UNSIGNED_INT,
                            (const GLvoid *)(instanceData->indexData) );
        }
        else
        {
            glDrawArrays( instanceData->primType,
                          instanceData->minVertexIdx,
                          instanceData->numVertices );
        }

        /* video memory specific additions */
        if ( NULL != instanceData->vidMemVertexData )
        {
            if ( _rwOpenGLVertexHeapAvailable() )
            {
                _rwOpenGLVertexHeapSetNVFence( instanceData->vidMemVertexData );
            }
        }
    }

    UPDATERENDERCBMETRICS(instanceData);

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglToonShadeOpenMP
 *
 *  Purpose : Multi-pass toon shader, soft-calculating toon texture coordinates.
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglToonShadeOpenMP( const RxOpenGLMeshInstanceData *instanceData,
                       const RwUInt32 flags,
                       const RpToonPaint *paint )
{
    RwBool  firstPass;


    RWFUNCTION( RWSTRING( "openglToonShadeOpenMP" ) );

    /* use the material color or not depending on the flags */
    if ( 0 != (flags & rxGEOMETRY_MODULATE) )
    {
        glColor4ubv( (const GLubyte *)&(instanceData->material->color) );
    }
    else
    {
        glColor4ub( 0xFF, 0xFF, 0xFF, 0xFF );
    }

    firstPass = FALSE;

    /* render first pass if textured */
    if ( 0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2)) )
    {
        RwOpenGLSetTexture( RpMaterialGetTexture( instanceData->material ) );

        if ( 0 != instanceData->displayList )
        {
            RWASSERT( GL_FALSE != glIsList( instanceData->displayList ) );

            /* execute the display list */
            glCallList( instanceData->displayList );
        }
        else if ( (NULL != _rwOpenGLExt.ArrayObjectATI) &&
                  (GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName )) )
        {
            RWASSERT( GL_FALSE !=
                      _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName ) );

            /* set up client state pointers */
            RwOpenGLVASetPositionATI( 3,
                                      GL_FLOAT,
                                      instanceData->vertexStride,
                                      instanceData->vaoName,
                                      instanceData->position );

            RwOpenGLVASetNormalATI( (0 != (flags & rxGEOMETRY_NORMALS)),
                                    GL_FLOAT,
                                    instanceData->vertexStride,
                                    instanceData->vaoName,
                                    instanceData->normal );

            RwOpenGLVASetColorATI( (0 != (flags & rxGEOMETRY_PRELIT)),
                                   4,
                                   GL_UNSIGNED_BYTE,
                                   instanceData->vertexStride,
                                   instanceData->vaoName,
                                   instanceData->color );

            RwOpenGLVASetTexCoordATI( TRUE,
                                      2,
                                      GL_FLOAT,
                                      instanceData->vertexStride,
                                      instanceData->vaoName,
                                      instanceData->texCoord[0] );

            /* render */
            if ( NULL != instanceData->indexData )
            {
                glDrawElements( instanceData->primType,
                                instanceData->numIndices,
                                GL_UNSIGNED_INT,
                                (const GLvoid *)(instanceData->indexData) );
            }
            else
            {
                glDrawArrays( instanceData->primType,
                              instanceData->minVertexIdx,
                              instanceData->numVertices );
            }
        }
        else
        {
            /* set up client state pointers */
            RwOpenGLVASetPosition( 3,
                                   GL_FLOAT,
                                   instanceData->vertexStride,
                                   instanceData->position );

            RwOpenGLVASetNormal( (0 != (flags & rxGEOMETRY_NORMALS)),
                                 GL_FLOAT,
                                 instanceData->vertexStride,
                                 instanceData->normal );

            RwOpenGLVASetColor( (0 != (flags & rxGEOMETRY_PRELIT)),
                                4,
                                GL_UNSIGNED_BYTE,
                                instanceData->vertexStride,
                                instanceData->color );

            RwOpenGLVASetTexCoord( TRUE,
                                   2,
                                   GL_FLOAT,
                                   instanceData->vertexStride,
                                   instanceData->texCoord[0] );

            /* render */
            if ( NULL != instanceData->indexData )
            {
                glDrawElements( instanceData->primType,
                                instanceData->numIndices,
                                GL_UNSIGNED_INT,
                                (const GLvoid *)(instanceData->indexData) );
            }
            else
            {
                glDrawArrays( instanceData->primType,
                              instanceData->minVertexIdx,
                              instanceData->numVertices );
            }

            /* video memory specific additions */
            if ( NULL != instanceData->vidMemVertexData )
            {
                if ( _rwOpenGLVertexHeapAvailable() )
                {
                    _rwOpenGLVertexHeapSetNVFence( instanceData->vidMemVertexData );
                }
            }
        }

        UPDATERENDERCBMETRICS(instanceData);

        firstPass = TRUE;
    }
    else
    {
        RwOpenGLSetTexture( NULL );
    }

    /* second pass, the toon paint */
    if ( NULL != paint->gradient )
    {
        RwTexture       *toonTexture;

        RwBool          vertexAlpha;

        RwBlendFunction srcBlend;

        RwBlendFunction dstBlend;

        RwV3d           *unitCamSpaceNormals;


        toonTexture = paint->gradient;

        RwOpenGLSetTexture( toonTexture );

        if ( FALSE != firstPass )
        {
            _rwOpenGLGetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)&vertexAlpha );
            _rwOpenGLGetRenderState( rwRENDERSTATESRCBLEND,          (void *)&srcBlend );
            _rwOpenGLGetRenderState( rwRENDERSTATEDESTBLEND,         (void *)&dstBlend );

            _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE );
            _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND,          (void *)rwBLENDZERO );
            _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND,         (void *)rwBLENDSRCCOLOR );
        }

        /* software texture generation */
        unitCamSpaceNormals = (RwV3d *)openglToonCalculateToonTexCoords( instanceData );

        glMatrixMode( GL_TEXTURE );
        glPushMatrix();

        /* set the texture transform matrix */
        glLoadIdentity();
        _rwOpenGLApplyRwMatrix( &openglToonTextureTransform );

        /* must use vertex arrays since we have to force a new set of texture coordinates */
        if ( (NULL != _rwOpenGLExt.ArrayObjectATI) &&
             (GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName )) )
        {
            RWASSERT( GL_FALSE !=
                      _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName ) );

            /* set up client state pointers */
            RwOpenGLVASetPositionATI( 3,
                                      GL_FLOAT,
                                      instanceData->vertexStride,
                                      instanceData->vaoName,
                                      instanceData->position );

            RwOpenGLVASetNormalATI( (0 != (flags & rxGEOMETRY_NORMALS)),
                                    GL_FLOAT,
                                    instanceData->vertexStride,
                                    instanceData->vaoName,
                                    instanceData->normal );

            RwOpenGLVASetColorATI( (0 != (flags & rxGEOMETRY_PRELIT)),
                                   4,
                                   GL_UNSIGNED_BYTE,
                                   instanceData->vertexStride,
                                   instanceData->vaoName,
                                   instanceData->color );

            RwOpenGLVASetTexCoordATI( TRUE,
                                      2,
                                      GL_FLOAT,
                                      sizeof(RwV3d),
                                      (const RwUInt32)unitCamSpaceNormals,
                                      0 );

            /* render */
            if ( NULL != instanceData->indexData )
            {
                glDrawElements( instanceData->primType,
                                instanceData->numIndices,
                                GL_UNSIGNED_INT,
                                (const GLvoid *)(instanceData->indexData) );
            }
            else
            {
                glDrawArrays( instanceData->primType,
                              instanceData->minVertexIdx,
                              instanceData->numVertices );
            }
        }
        else
        {
            /* set up client state pointers */
            RwOpenGLVASetPosition( 3,
                                   GL_FLOAT,
                                   instanceData->vertexStride,
                                   instanceData->position );

            RwOpenGLVASetNormal( (0 != (flags & rxGEOMETRY_NORMALS)),
                                 GL_FLOAT,
                                 instanceData->vertexStride,
                                 instanceData->normal );

            RwOpenGLVASetColor( (0 != (flags & rxGEOMETRY_PRELIT)),
                                4,
                                GL_UNSIGNED_BYTE,
                                instanceData->vertexStride,
                                instanceData->color );

            RwOpenGLVASetTexCoord( TRUE,
                                   3,
                                   GL_FLOAT,
                                   sizeof(RwV3d),
                                   (RwUInt8 *)unitCamSpaceNormals );

            /* render */
            if ( NULL != instanceData->indexData )
            {
                glDrawElements( instanceData->primType,
                                instanceData->numIndices,
                                GL_UNSIGNED_INT,
                                (const GLvoid *)(instanceData->indexData) );
            }
            else
            {
                glDrawArrays( instanceData->primType,
                              instanceData->minVertexIdx,
                              instanceData->numVertices );
            }

            /* video memory specific additions */
            if ( NULL != instanceData->vidMemVertexData )
            {
                if ( _rwOpenGLVertexHeapAvailable() )
                {
                    _rwOpenGLVertexHeapSetNVFence( instanceData->vidMemVertexData );
                }
            }
        }

        UPDATERENDERCBMETRICS(instanceData);

        if ( FALSE != firstPass )
        {
            _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)vertexAlpha );
            _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND,          (void *)srcBlend );
            _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND,         (void *)dstBlend );
        }
    }

    if ( NULL != paint->gradient )
    {
        glPopMatrix();
        glMatrixMode( GL_MODELVIEW );
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglToonShadeOpenMPCM
 *
 *  Purpose : Multi-pass toon shader, auto-generating toon texture coordinates.
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglToonShadeOpenMPCM( const RxOpenGLMeshInstanceData *instanceData,
                         const RwUInt32 flags,
                         const RpToonPaint *paint )
{
    RwBool  firstPass;


    RWFUNCTION( RWSTRING( "openglToonShadeOpenMPCM" ) );

    /* use the material color or not depending on the flags */
    if ( 0 != (flags & rxGEOMETRY_MODULATE) )
    {
        glColor4ubv( (const GLubyte *)&(instanceData->material->color) );
    }
    else
    {
        glColor4ub( 0xFF, 0xFF, 0xFF, 0xFF );
    }

    firstPass = FALSE;

    /* render first pass if textured */
    if ( 0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2)) )
    {
        RwOpenGLSetTexture( RpMaterialGetTexture( instanceData->material ) );

        if ( 0 != instanceData->displayList )
        {
            RWASSERT( GL_FALSE != glIsList( instanceData->displayList ) );

            /* execute the display list */
            glCallList( instanceData->displayList );
        }
        else if ( (NULL != _rwOpenGLExt.ArrayObjectATI) &&
                  (GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName )) )
        {
            RWASSERT( GL_FALSE !=
                      _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName ) );

            /* set up client state pointers */
            RwOpenGLVASetPositionATI( 3,
                                      GL_FLOAT,
                                      instanceData->vertexStride,
                                      instanceData->vaoName,
                                      instanceData->position );

            RwOpenGLVASetNormalATI( (0 != (flags & rxGEOMETRY_NORMALS)),
                                    GL_FLOAT,
                                    instanceData->vertexStride,
                                    instanceData->vaoName,
                                    instanceData->normal );

            RwOpenGLVASetColorATI( (0 != (flags & rxGEOMETRY_PRELIT)),
                                   4,
                                   GL_UNSIGNED_BYTE,
                                   instanceData->vertexStride,
                                   instanceData->vaoName,
                                   instanceData->color );

            RwOpenGLVASetTexCoordATI( TRUE,
                                      2,
                                      GL_FLOAT,
                                      instanceData->vertexStride,
                                      instanceData->vaoName,
                                      instanceData->texCoord[0] );

            /* render */
            if ( NULL != instanceData->indexData )
            {
                glDrawElements( instanceData->primType,
                                instanceData->numIndices,
                                GL_UNSIGNED_INT,
                                (const GLvoid *)(instanceData->indexData) );
            }
            else
            {
                glDrawArrays( instanceData->primType,
                              instanceData->minVertexIdx,
                              instanceData->numVertices );
            }
        }
        else
        {
            /* set up client state pointers */
            RwOpenGLVASetPosition( 3,
                                   GL_FLOAT,
                                   instanceData->vertexStride,
                                   instanceData->position );

            RwOpenGLVASetNormal( (0 != (flags & rxGEOMETRY_NORMALS)),
                                 GL_FLOAT,
                                 instanceData->vertexStride,
                                 instanceData->normal );

            RwOpenGLVASetColor( (0 != (flags & rxGEOMETRY_PRELIT)),
                                4,
                                GL_UNSIGNED_BYTE,
                                instanceData->vertexStride,
                                instanceData->color );

            RwOpenGLVASetTexCoord( TRUE,
                                   2,
                                   GL_FLOAT,
                                   instanceData->vertexStride,
                                   instanceData->texCoord[0] );

            /* render */
            if ( NULL != instanceData->indexData )
            {
                glDrawElements( instanceData->primType,
                                instanceData->numIndices,
                                GL_UNSIGNED_INT,
                                (const GLvoid *)(instanceData->indexData) );
            }
            else
            {
                glDrawArrays( instanceData->primType,
                              instanceData->minVertexIdx,
                              instanceData->numVertices );
            }

            /* video memory specific additions */
            if ( NULL != instanceData->vidMemVertexData )
            {
                if ( _rwOpenGLVertexHeapAvailable() )
                {
                    _rwOpenGLVertexHeapSetNVFence( instanceData->vidMemVertexData );
                }
            }
        }

        UPDATERENDERCBMETRICS(instanceData);

        firstPass = TRUE;
    }
    else
    {
        RwOpenGLSetTexture( NULL );
    }

    /* second pass, the toon paint */
    if ( NULL != paint->gradient )
    {
        RwTexture       *toonTexture;

        RwBool          vertexAlpha;

        RwBlendFunction srcBlend;

        RwBlendFunction dstBlend;


        toonTexture = paint->gradient;

        RwOpenGLSetTexture( toonTexture );

        if ( FALSE != firstPass )
        {
            _rwOpenGLGetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)&vertexAlpha );
            _rwOpenGLGetRenderState( rwRENDERSTATESRCBLEND,          (void *)&srcBlend );
            _rwOpenGLGetRenderState( rwRENDERSTATEDESTBLEND,         (void *)&dstBlend );

            _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE );
            _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND,          (void *)rwBLENDZERO );
            _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND,         (void *)rwBLENDSRCCOLOR );
        }

        /* auto-texture generation */
        RwOpenGLEnable( rwGL_NORMALIZE );

        RwOpenGLEnable( rwGL_TEXTURE0_GEN_S );
        RwOpenGLEnable( rwGL_TEXTURE0_GEN_T );
        RwOpenGLEnable( rwGL_TEXTURE0_GEN_R );

        glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB );
        glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB );
        glTexGeni( GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB );

        glMatrixMode( GL_TEXTURE );
        glPushMatrix();

        /* set the texture transform matrix */
        glLoadIdentity();
        _rwOpenGLApplyRwMatrix( &openglToonTextureTransform );

        if ( 0 != instanceData->displayList )
        {
            RWASSERT( GL_FALSE != glIsList( instanceData->displayList ) );

            /* execute the display list */
            glCallList( instanceData->displayList );
        }
        else if ( (NULL != _rwOpenGLExt.ArrayObjectATI) &&
                  (GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName )) )
        {
            RWASSERT( GL_FALSE !=
                      _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName ) );

            /* set up client state pointers */
            RwOpenGLVASetPositionATI( 3,
                                      GL_FLOAT,
                                      instanceData->vertexStride,
                                      instanceData->vaoName,
                                      instanceData->position );

            RwOpenGLVASetNormalATI( (0 != (flags & rxGEOMETRY_NORMALS)),
                                    GL_FLOAT,
                                    instanceData->vertexStride,
                                    instanceData->vaoName,
                                    instanceData->normal );

            RwOpenGLVASetColorATI( (0 != (flags & rxGEOMETRY_PRELIT)),
                                   4,
                                   GL_UNSIGNED_BYTE,
                                   instanceData->vertexStride,
                                   instanceData->vaoName,
                                   instanceData->color );

            RwOpenGLVADisableTexCoord( 0 );

            /* render */
            if ( NULL != instanceData->indexData )
            {
                glDrawElements( instanceData->primType,
                                instanceData->numIndices,
                                GL_UNSIGNED_INT,
                                (const GLvoid *)(instanceData->indexData) );
            }
            else
            {
                glDrawArrays( instanceData->primType,
                              instanceData->minVertexIdx,
                              instanceData->numVertices );
            }
        }
        else
        {
            /* set up client state pointers */
            RwOpenGLVASetPosition( 3,
                                   GL_FLOAT,
                                   instanceData->vertexStride,
                                   instanceData->position );

            RwOpenGLVASetNormal( (0 != (flags & rxGEOMETRY_NORMALS)),
                                 GL_FLOAT,
                                 instanceData->vertexStride,
                                 instanceData->normal );

            RwOpenGLVASetColor( (0 != (flags & rxGEOMETRY_PRELIT)),
                                4,
                                GL_UNSIGNED_BYTE,
                                instanceData->vertexStride,
                                instanceData->color );

            RwOpenGLVADisableTexCoord( 0 );

            /* render */
            if ( NULL != instanceData->indexData )
            {
                glDrawElements( instanceData->primType,
                                instanceData->numIndices,
                                GL_UNSIGNED_INT,
                                (const GLvoid *)(instanceData->indexData) );
            }
            else
            {
                glDrawArrays( instanceData->primType,
                              instanceData->minVertexIdx,
                              instanceData->numVertices );
            }

            /* video memory specific additions */
            if ( NULL != instanceData->vidMemVertexData )
            {
                if ( _rwOpenGLVertexHeapAvailable() )
                {
                    _rwOpenGLVertexHeapSetNVFence( instanceData->vidMemVertexData );
                }
            }
        }

        UPDATERENDERCBMETRICS(instanceData);

        if ( FALSE != firstPass )
        {
            _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)vertexAlpha );
            _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND,          (void *)srcBlend );
            _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND,         (void *)dstBlend );
        }
    }

    if ( NULL != paint->gradient )
    {
        glPopMatrix();
        glMatrixMode( GL_MODELVIEW );

        RwOpenGLDisable( rwGL_TEXTURE0_GEN_S );
        RwOpenGLDisable( rwGL_TEXTURE0_GEN_T );
        RwOpenGLDisable( rwGL_TEXTURE0_GEN_R );

        RwOpenGLDisable( rwGL_NORMALIZE );
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglToonShadeOpenMT
 *
 *  Purpose : Multitexture toon shader, soft-calculating toon texture coordinates.
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglToonShadeOpenMT( const RxOpenGLMeshInstanceData *instanceData,
                       const RwUInt32 flags,
                       const RpToonPaint *paint )
{
    RwV3d       *unitCamSpaceNormals = (RwV3d *)NULL;


    RWFUNCTION( RWSTRING( "openglToonShadeOpenMT" ) );

    /* use the material color or not depending on the flags */
    if ( 0 != (flags & rxGEOMETRY_MODULATE) )
    {
        glColor4ubv( (const GLubyte *)&(instanceData->material->color) );
    }
    else
    {
        glColor4ub( 0xFF, 0xFF, 0xFF, 0xFF );
    }

    /* set the base texture in texture unit 0 */
    RwOpenGLSetActiveTextureUnit( 0 );
    if ( 0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2)) )
    {
        RwOpenGLSetTexture( RpMaterialGetTexture( instanceData->material ) );
    }
    else
    {
        RwOpenGLSetTexture( NULL );
    }

    /* bind the toon paint to texture unit 1 */
    RwOpenGLSetActiveTextureUnit( 1 );
    if ( NULL != paint->gradient )
    {
        RwOpenGLSetTexture( paint->gradient );

        /* software texture generation */
        unitCamSpaceNormals = (RwV3d *)openglToonCalculateToonTexCoords( instanceData );

        glMatrixMode( GL_TEXTURE );
        glPushMatrix();

        /* set the texture transform matrix */
        glLoadIdentity();
        _rwOpenGLApplyRwMatrix( &openglToonTextureTransform );
    }
    else
    {
        RwOpenGLSetTexture( NULL );
    }

    /* must use vertex arrays since we have to force a new set of texture coordinates */
    if ( (NULL != _rwOpenGLExt.ArrayObjectATI) &&
         (GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName )) )
    {
        RWASSERT( GL_FALSE !=
                  _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName ) );

        /* set up client state pointers */
        RwOpenGLVASetPositionATI( 3,
                                  GL_FLOAT,
                                  instanceData->vertexStride,
                                  instanceData->vaoName,
                                  instanceData->position );

        RwOpenGLVASetNormalATI( (0 != (flags & rxGEOMETRY_NORMALS)),
                                GL_FLOAT,
                                instanceData->vertexStride,
                                instanceData->vaoName,
                                instanceData->normal );

        RwOpenGLVASetColorATI( (0 != (flags & rxGEOMETRY_PRELIT)),
                               4,
                               GL_UNSIGNED_BYTE,
                               instanceData->vertexStride,
                               instanceData->vaoName,
                               instanceData->color );

        RwOpenGLVASetMultiTexCoordATI( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))),
                                       0,
                                       2,
                                       GL_FLOAT,
                                       instanceData->vertexStride,
                                       instanceData->vaoName,
                                       instanceData->texCoord[0] );

        RwOpenGLVASetMultiTexCoordATI( (NULL != paint->gradient),
                                       1,
                                       3,
                                       GL_FLOAT,
                                       sizeof(RwV3d),
                                       (const RwUInt32)unitCamSpaceNormals,
                                       0);

        /* render */
        if ( NULL != instanceData->indexData )
        {
            glDrawElements( instanceData->primType,
                            instanceData->numIndices,
                            GL_UNSIGNED_INT,
                            (const GLvoid *)(instanceData->indexData) );
        }
        else
        {
            glDrawArrays( instanceData->primType,
                          instanceData->minVertexIdx,
                          instanceData->numVertices );
        }
    }
    else
    {
        /* set up client state pointers */
        RwOpenGLVASetPosition( 3,
                               GL_FLOAT,
                               instanceData->vertexStride,
                               instanceData->position );

        RwOpenGLVASetNormal( (0 != (flags & rxGEOMETRY_NORMALS)),
                             GL_FLOAT,
                             instanceData->vertexStride,
                             instanceData->normal );

        RwOpenGLVASetColor( (0 != (flags & rxGEOMETRY_PRELIT)),
                            4,
                            GL_UNSIGNED_BYTE,
                            instanceData->vertexStride,
                            instanceData->color );

        RwOpenGLVASetMultiTexCoord( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))),
                                    0,
                                    2,
                                    GL_FLOAT,
                                    instanceData->vertexStride,
                                    instanceData->texCoord[0] );

        RwOpenGLVASetMultiTexCoord( (NULL != paint->gradient),
                                    1,
                                    3,
                                    GL_FLOAT,
                                    sizeof(RwV3d),
                                    (RwUInt8 *)unitCamSpaceNormals );

        /* render */
        if ( NULL != instanceData->indexData )
        {
            glDrawElements( instanceData->primType,
                            instanceData->numIndices,
                            GL_UNSIGNED_INT,
                            (const GLvoid *)(instanceData->indexData) );
        }
        else
        {
            glDrawArrays( instanceData->primType,
                          instanceData->minVertexIdx,
                          instanceData->numVertices );
        }

        /* video memory specific additions */
        if ( NULL != instanceData->vidMemVertexData )
        {
            if ( _rwOpenGLVertexHeapAvailable() )
            {
                _rwOpenGLVertexHeapSetNVFence( instanceData->vidMemVertexData );
            }
        }
    }

    UPDATERENDERCBMETRICS(instanceData);

    if ( NULL != paint->gradient )
    {
        glPopMatrix();
        glMatrixMode( GL_MODELVIEW );

        RwOpenGLSetTexture( NULL );
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglToonShadeOpenMTCM
 *
 *  Purpose : Multitexture toon shader, auto-calculating toon texture coordinates.
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglToonShadeOpenMTCM( const RxOpenGLMeshInstanceData *instanceData,
                         const RwUInt32 flags,
                         const RpToonPaint *paint )
{
    RWFUNCTION( RWSTRING( "openglToonShadeOpenMTCM" ) );

    /* use the material color or not depending on the flags */
    if ( 0 != (flags & rxGEOMETRY_MODULATE) )
    {
        glColor4ubv( (const GLubyte *)&(instanceData->material->color) );
    }
    else
    {
        glColor4ub( 0xFF, 0xFF, 0xFF, 0xFF );
    }

    /* set the base texture in texture unit 0 */
    RwOpenGLSetActiveTextureUnit( 0 );
    if ( 0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2)) )
    {
        RwOpenGLSetTexture( RpMaterialGetTexture( instanceData->material ) );
    }
    else
    {
        RwOpenGLSetTexture( NULL );
    }

    /* bind the toon paint to texture unit 1 */
    RwOpenGLSetActiveTextureUnit( 1 );
    if ( NULL != paint->gradient )
    {
        RwOpenGLSetTexture( paint->gradient );

        RwOpenGLEnable( rwGL_NORMALIZE );

        RwOpenGLEnable( rwGL_TEXTURE1_GEN_S );
        RwOpenGLEnable( rwGL_TEXTURE1_GEN_T );
        RwOpenGLEnable( rwGL_TEXTURE1_GEN_R );

        glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB );
        glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB );
        glTexGeni( GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB );

        glMatrixMode( GL_TEXTURE );
        glPushMatrix();

        /* set the texture transform matrix */
        glLoadIdentity();
        _rwOpenGLApplyRwMatrix( &openglToonTextureTransform );
    }
    else
    {
        RwOpenGLSetTexture( NULL );
    }

    if ( 0 != instanceData->displayList )
    {
        RWASSERT( GL_FALSE != glIsList( instanceData->displayList ) );

        /* execute the display list */
        glCallList( instanceData->displayList );
    }
    else if ( (NULL != _rwOpenGLExt.ArrayObjectATI) &&
              (GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName )) )
    {
        RWASSERT( GL_FALSE !=
                  _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName ) );

        /* set up client state pointers */
        RwOpenGLVASetPositionATI( 3,
                                  GL_FLOAT,
                                  instanceData->vertexStride,
                                  instanceData->vaoName,
                                  instanceData->position );

        RwOpenGLVASetNormalATI( (0 != (flags & rxGEOMETRY_NORMALS)),
                                GL_FLOAT,
                                instanceData->vertexStride,
                                instanceData->vaoName,
                                instanceData->normal );

        RwOpenGLVASetColorATI( (0 != (flags & rxGEOMETRY_PRELIT)),
                               4,
                               GL_UNSIGNED_BYTE,
                               instanceData->vertexStride,
                               instanceData->vaoName,
                               instanceData->color );

        RwOpenGLVASetMultiTexCoordATI( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))),
                                       0,
                                       2,
                                       GL_FLOAT,
                                       instanceData->vertexStride,
                                       instanceData->vaoName,
                                       instanceData->texCoord[0] );

        /* render */
        if ( NULL != instanceData->indexData )
        {
            glDrawElements( instanceData->primType,
                            instanceData->numIndices,
                            GL_UNSIGNED_INT,
                            (const GLvoid *)(instanceData->indexData) );
        }
        else
        {
            glDrawArrays( instanceData->primType,
                          instanceData->minVertexIdx,
                          instanceData->numVertices );
        }
    }
    else
    {
        /* set up client state pointers */
        RwOpenGLVASetPosition( 3,
                               GL_FLOAT,
                               instanceData->vertexStride,
                               instanceData->position );

        RwOpenGLVASetNormal( (0 != (flags & rxGEOMETRY_NORMALS)),
                             GL_FLOAT,
                             instanceData->vertexStride,
                             instanceData->normal );

        RwOpenGLVASetColor( (0 != (flags & rxGEOMETRY_PRELIT)),
                            4,
                            GL_UNSIGNED_BYTE,
                            instanceData->vertexStride,
                            instanceData->color );

        RwOpenGLVASetMultiTexCoord( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))),
                                    0,
                                    2,
                                    GL_FLOAT,
                                    instanceData->vertexStride,
                                    instanceData->texCoord[0] );

        /* render */
        if ( NULL != instanceData->indexData )
        {
            glDrawElements( instanceData->primType,
                            instanceData->numIndices,
                            GL_UNSIGNED_INT,
                            (const GLvoid *)(instanceData->indexData) );
        }
        else
        {
            glDrawArrays( instanceData->primType,
                          instanceData->minVertexIdx,
                          instanceData->numVertices );
        }

        /* video memory specific additions */
        if ( NULL != instanceData->vidMemVertexData )
        {
            if ( _rwOpenGLVertexHeapAvailable() )
            {
                _rwOpenGLVertexHeapSetNVFence( instanceData->vidMemVertexData );
            }
        }
    }

    UPDATERENDERCBMETRICS(instanceData);

    if ( NULL != paint->gradient )
    {
        RwOpenGLSetActiveTextureUnit( 1 );

        glPopMatrix();
        glMatrixMode( GL_MODELVIEW );

        RwOpenGLDisable( rwGL_TEXTURE1_GEN_S );
        RwOpenGLDisable( rwGL_TEXTURE1_GEN_T );
        RwOpenGLDisable( rwGL_TEXTURE1_GEN_R );

        RwOpenGLDisable( rwGL_NORMALIZE );

        RwOpenGLSetTexture( NULL );
    }

    RwOpenGLSetActiveTextureUnit( 0 );

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglToonFindFirstGlobalLight
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static RpLight *
openglToonFindFirstGlobalLight()
{
    RpWorld     *world;

    RwLLLink    *cur;

    RwLLLink    *end;


    RWFUNCTION( RWSTRING( "openglToonFindFirstGlobalLight" ) );

    world = (RpWorld *)RWSRCGLOBAL(curWorld);

    cur = rwLinkListGetFirstLLLink(&world->directionalLightList);
    end = rwLinkListGetTerminator(&world->directionalLightList);
    while ( cur != end )
    {
        RpLight *light;


        light = rwLLLinkGetData( cur, RpLight, inWorld );

        /* NB light may actually be a dummyTie from a enclosing ForAll */
        if ( (NULL != light) &&
             (rwObjectTestFlags(light, rpLIGHTLIGHTATOMICS)) )
        {
            switch ( RpLightGetType(light) )
            {
                case rpLIGHTAMBIENT:
                    /* nothing */
                    break;

                case rpLIGHTDIRECTIONAL:
                    RWRETURN( light );
                    break;

                default:
                    RWASSERT( FALSE ); /* unsupported light type */
            }
        }

        /* next light */
        cur = rwLLLinkGetNext(cur);
    }

    RWRETURN( (RpLight *)NULL );
}


/* ---------------------------------------------------------------------
 *  Function: openglToonUpdateCameraSpaceLightMatrix
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglToonUpdateCameraSpaceLightMatrix( void )
{
    RWFUNCTION( RWSTRING( "openglToonUpdateCameraSpaceLightMatrix" ) );

    RwMatrixSetIdentity( &openglToonTextureTransform );

    /* ensure dark shading if no light is present */
    openglToonTextureTransform.right.x = 0.0f;

    if ( NULL != RWSRCGLOBAL(curWorld) )
    {
        RpLight *dirLight;


        dirLight = openglToonFindFirstGlobalLight();

        if ( NULL != dirLight )
        {
            RwCamera    *camera;

            RwFrame     *cameraFrame;

            RwV3d       *at;

            RwMatrix    inverseCamLTM;

            RwV3d       transLight;


            camera = RwCameraGetCurrentCamera();
            RWASSERT( NULL != camera );

            cameraFrame = RwCameraGetFrame( camera );
            RWASSERT( NULL != cameraFrame );

            at = RwMatrixGetAt( RwFrameGetLTM( RpLightGetFrame(dirLight) ) );

            RwMatrixInvert( &inverseCamLTM, RwFrameGetLTM(cameraFrame) );

            RwV3dTransformVector( &transLight,
                                   at,
                                   &inverseCamLTM );

            openglToonTextureTransform.right.x = transLight.x;
            openglToonTextureTransform.up.x    = -transLight.y;
            openglToonTextureTransform.at.x    = transLight.z;
        }
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglToonMeshMinVertexQSortCB
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static int
openglToonMeshMinVertexQSortCB( const void *elem1, const void *elem2 )
{
    RxOpenGLMeshInstanceData    *mesh1;

    RxOpenGLMeshInstanceData    *mesh2;


    RWFUNCTION( RWSTRING( "openglToonMeshMinVertexQSortCB" ) );

    mesh1 = *((RxOpenGLMeshInstanceData **)elem1);
    mesh2 = *((RxOpenGLMeshInstanceData **)elem2);

    RWRETURN( mesh2->minVertexIdx - mesh1->minVertexIdx );
}


/* ---------------------------------------------------------------------
 *  Function: openglToonCalculateToonTexCoords
 *
 *  Purpose : software texture coordinate generation
 *
 *  On entry:
 *
 *  On exit : Returns either a pointer to the unit camera space normals
 *            vertex array memory, or a VAO name.
 * --------------------------------------------------------------------- */
static void *
openglToonCalculateToonTexCoords( const RxOpenGLMeshInstanceData *instanceData )
{
    static RwUInt32 lastNumVertices = 0;

    static RwV3d    *normals = (RwV3d *)NULL;

    static RwV3d    *camSpaceNormals = (RwV3d *)NULL;

    static RwV3d    *unitCamSpaceNormals = (RwV3d *)NULL;

    RwUInt8         *vertexData;

    RwUInt32        normalIdx;

    GLfloat         oglModelViewMatrix[16];

    RwMatrix        rwModelViewMatrix;

    RwMatrix        invRWModelViewMatrix;


    RWFUNCTION( RWSTRING( "openglToonCalculateToonTexCoords" ) );

    if ( instanceData->numVertices > lastNumVertices )
    {
        if ( NULL != openglToonTexCoordsBlock )
        {
            RwFree( openglToonTexCoordsBlock );
        }

        if ( NULL != _rwOpenGLExt.FreeObjectBufferATI )
        {
            if ( GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( openglToonUnitCamSpaceNormalsVAO ) )
            {
                _rwOpenGLExt.FreeObjectBufferATI( openglToonUnitCamSpaceNormalsVAO );
            }
        }

        if ( _rwOpenGLVertexHeapAvailable() )
        {
            openglToonTexCoordsBlock = (RwV3d *)RwMalloc( 2 * instanceData->numVertices * sizeof(RwV3d),
                rwID_TOONPLUGIN | rwMEMHINTDUR_EVENT);
            RWASSERT( NULL != openglToonTexCoordsBlock );

            normals = openglToonTexCoordsBlock;
            camSpaceNormals = normals + instanceData->numVertices;

            if ( NULL != unitCamSpaceNormals )
            {
                _rwOpenGLVertexHeapDynamicDiscard( unitCamSpaceNormals );
            }
            unitCamSpaceNormals = (RwV3d *)_rwOpenGLVertexHeapDynamicMalloc( instanceData->numVertices * sizeof(RwV3d), TRUE );
            RWASSERT( NULL != unitCamSpaceNormals );
        }
        else
        {
            openglToonTexCoordsBlock = (RwV3d *)RwMalloc( 3 * instanceData->numVertices * sizeof(RwV3d),
                rwID_TOONPLUGIN | rwMEMHINTDUR_EVENT);
            RWASSERT( NULL != openglToonTexCoordsBlock );

            normals = openglToonTexCoordsBlock;
            camSpaceNormals = normals + instanceData->numVertices;
            unitCamSpaceNormals = camSpaceNormals + instanceData->numVertices;

            if ( NULL != _rwOpenGLExt.NewObjectBufferATI )
            {
                openglToonUnitCamSpaceNormalsVAO = _rwOpenGLExt.NewObjectBufferATI( instanceData->numVertices * sizeof(RwV3d),
                                                                          NULL,
                                                                          GL_DYNAMIC_ATI );
                RWASSERT( GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( openglToonUnitCamSpaceNormalsVAO ) );
            }
        }

        lastNumVertices = instanceData->numVertices;
    }

    /* can be slow if video memory is used since we have to read from it */
    vertexData = instanceData->vidMemVertexData;
    if ( NULL == vertexData )
    {
        vertexData = instanceData->sysMemVertexData;
    }
    RWASSERT( NULL != vertexData );

    for ( normalIdx = 0; normalIdx < instanceData->numVertices; normalIdx += 1 )
    {
        RwV3dAssign( &(normals[normalIdx]),
                     (RwV3d *)(vertexData + instanceData->vertexStride * normalIdx + sizeof(RwV3d)) );
    }

    /* get the model view matrix */
    glGetFloatv( GL_MODELVIEW_MATRIX, oglModelViewMatrix );

    /* convert it to RW format */
    _rwOpenGLMatrixToRwMatrix( &rwModelViewMatrix, oglModelViewMatrix );

    /* invert */
    RwMatrixInvert( &invRWModelViewMatrix, &rwModelViewMatrix );

    /* transform the normals to camera space */
    RwV3dTransformVectors( camSpaceNormals,
                           normals,
                           instanceData->numVertices,
                           &invRWModelViewMatrix );

    /* normalize them */
    for ( normalIdx = 0; normalIdx < instanceData->numVertices; normalIdx += 1 )
    {
        RwV3dNormalize( &(unitCamSpaceNormals[normalIdx]), &(camSpaceNormals[normalIdx]) );
    }

    if ( NULL != _rwOpenGLExt.UpdateObjectBufferATI )
    {
        _rwOpenGLExt.UpdateObjectBufferATI( openglToonUnitCamSpaceNormalsVAO,
                                            0,
                                            instanceData->numVertices * sizeof(RwV3d),
                                            unitCamSpaceNormals,
                                            GL_DISCARD_ATI );

        RWRETURN( (void *)openglToonUnitCamSpaceNormalsVAO );
    }
    else
    {
        RWRETURN( (void *)unitCamSpaceNormals );
    }
}
