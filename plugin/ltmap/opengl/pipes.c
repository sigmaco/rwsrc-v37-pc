/***********************************************************************
 *
 * Module:  pipes.c
 *
 * Purpose: Lightmaps OpenGL Implementation
 *
 ***********************************************************************/

/* =====================================================================
 *  Includes
 * ===================================================================== */
#include "rwcore.h"
#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpltmap.h"
#include "pipes.h"


/* =====================================================================
 *  Defines
 * ===================================================================== */

#define OPENGLLTMAPATOMICPIPEIDX        (0)
#define OPENGLLTMAPWORLDSECTORPIPEIDX   (1)

#define openglLtMapPipeline(_idx)   \
    (_rpLtMapGlobals.platformPipes[_idx])

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
typedef void (*openglLtMapRenderLightMapOnlyFn)( RwResEntry *repEntry,
                                                 void *object,
                                                 const RwUInt8 type,
                                                 const RwUInt32 flags,
                                                 const RwBool lightingEnabled );

typedef void (*openglLtMapRenderWithLightMapFn)( RwResEntry *repEntry,
                                                 void *object,
                                                 const RwUInt8 type,
                                                 const RwUInt32 flags,
                                                 const RwBool lightingEnabled );

typedef void (*openglLtMapRenderWithLightMapVertexColorFn)( RwResEntry *repEntry,
                                                            void *object,
                                                            const RwUInt8 type,
                                                            const RwUInt32 flags,
                                                            const RwBool lightingEnabled );


/* =====================================================================
 *  Static variables
 * ===================================================================== */
static openglLtMapRenderLightMapOnlyFn              renderLtMapOnlyFn = \
    (openglLtMapRenderLightMapOnlyFn)NULL;

static openglLtMapRenderWithLightMapFn              renderWithLtMapFn = \
    (openglLtMapRenderWithLightMapFn)NULL;

static openglLtMapRenderWithLightMapVertexColorFn   renderWithLtMapVertexColorFn = \
    (openglLtMapRenderWithLightMapVertexColorFn)NULL;


/* =====================================================================
 *  Global variables
 * ===================================================================== */


/* =====================================================================
 *  Static function prototypes
 * ===================================================================== */
static RxPipeline *
openglLtMapPipelineCreate( const RwInt32 type );

static void
openglLtMapRenderCB( RwResEntry *repEntry,
                     void *object,
                     const RwUInt8 type,
                     const RwUInt32 flags );

static void
openglLtMapRenderBaseTextureOnly( RwResEntry *repEntry,
                                  const RwUInt8 flags,
                                  const RwBool lightingEnabled );

static RwTexture *
openglLtMapGetLightMapTexture( void *object, const RwUInt8 type );

static void
openglLtMapRenderLightmapOnlyNoADD( RwResEntry *repEntry,
                                    void *object,
                                    const RwUInt8 type,
                                    const RwUInt32 flags,
                                    const RwBool lightingEnabled );

static void
openglLtMapRenderLightmapOnlyADD( RwResEntry *repEntry,
                                  void *object,
                                  const RwUInt8 type,
                                  const RwUInt32 flags,
                                  const RwBool lightingEnabled );

static void
openglLtMapRenderWithLightmapMP( RwResEntry *repEntry,
                                 void *object,
                                 const RwUInt8 type,
                                 const RwUInt32 flags,
                                 const RwBool lightingEnabled );

static void
openglLtMapRenderWithLightmapMT( RwResEntry *repEntry,
                                 void *object,
                                 const RwUInt8 type,
                                 const RwUInt32 flags,
                                 const RwBool lightingEnabled );

static void
openglLtMapRenderWithLightMapVertexColorMPNoADD( RwResEntry *repEntry,
                                                 void *object,
                                                 const RwUInt8 type,
                                                 const RwUInt32 flags,
                                                 const RwBool lightingEnabled );

static void
openglLtMapRenderWithLightMapVertexColorMPADD( RwResEntry *repEntry,
                                               void *object,
                                               const RwUInt8 type,
                                               const RwUInt32 flags,
                                               const RwBool lightingEnabled );


static void
openglLtMapRenderWithLightMapVertexColorMTADD( RwResEntry *repEntry,
                                               void *object,
                                               const RwUInt8 type,
                                               const RwUInt32 flags,
                                               const RwBool lightingEnabled );


/* =====================================================================
 *  Global function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: _rpLtMapPlatformPipelinesCreate
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwBool
_rpLtMapPlatformPipelinesCreate( void )
{
    RWFUNCTION( RWSTRING( "_rpLtMapPlatformPipelinesCreate" ) );

    openglLtMapPipeline( OPENGLLTMAPATOMICPIPEIDX ) = openglLtMapPipelineCreate( rpATOMIC );

    openglLtMapPipeline( OPENGLLTMAPWORLDSECTORPIPEIDX ) = openglLtMapPipelineCreate( rwSECTORATOMIC );

    if ( (NULL == openglLtMapPipeline( OPENGLLTMAPATOMICPIPEIDX )) ||
         (NULL == openglLtMapPipeline( OPENGLLTMAPWORLDSECTORPIPEIDX )) )
    {
        _rpLtMapPlatformPipelinesDestroy();
        RWRETURN( FALSE );
    }

    /* rendering pipeline for lightmaps only */
    renderLtMapOnlyFn = openglLtMapRenderLightmapOnlyNoADD;
    if ( FALSE != _rwOpenGLExt.TextureEnvAddARB )
    {
        renderLtMapOnlyFn = openglLtMapRenderLightmapOnlyADD;
    }
    RWASSERT( NULL != renderLtMapOnlyFn );

    /* rendering pipeline for with lightmaps */
    renderWithLtMapFn = openglLtMapRenderWithLightmapMP;
    if ( _rwOpenGLExt.MaxTextureUnits >= 2 )
    {
        renderWithLtMapFn = openglLtMapRenderWithLightmapMT;
    }
    RWASSERT( NULL != renderWithLtMapFn );

    /* rendering pipeline for with lightmaps and vertex color */
    renderWithLtMapVertexColorFn = openglLtMapRenderWithLightMapVertexColorMPNoADD;
    if ( _rwOpenGLExt.MaxTextureUnits >= 2 )
    {
        if ( FALSE != _rwOpenGLExt.TextureEnvAddARB )
        {
            renderWithLtMapVertexColorFn = openglLtMapRenderWithLightMapVertexColorMTADD;
        }
    }
    else if ( FALSE != _rwOpenGLExt.TextureEnvAddARB )
    {
        renderWithLtMapVertexColorFn = openglLtMapRenderWithLightMapVertexColorMPADD;
    }
    RWASSERT( NULL != renderWithLtMapVertexColorFn );

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rpLtMapPlatformPipelinesDestroy
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
void
_rpLtMapPlatformPipelinesDestroy( void )
{
    RWFUNCTION( RWSTRING( "_rpLtMapPlatformPipelinesDestroy" ) );

    if ( NULL != openglLtMapPipeline( OPENGLLTMAPATOMICPIPEIDX ) )
    {
        RxPipelineDestroy( openglLtMapPipeline( OPENGLLTMAPATOMICPIPEIDX ) );
        openglLtMapPipeline( OPENGLLTMAPATOMICPIPEIDX ) = (RxPipeline *)NULL;
    }

    if ( NULL != openglLtMapPipeline( OPENGLLTMAPWORLDSECTORPIPEIDX ) )
    {
        RxPipelineDestroy( openglLtMapPipeline( OPENGLLTMAPWORLDSECTORPIPEIDX ) );
        openglLtMapPipeline( OPENGLLTMAPWORLDSECTORPIPEIDX ) = (RxPipeline *)NULL;
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rpLtMapGetPlatformAtomicPipeline
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RxPipeline *
_rpLtMapGetPlatformAtomicPipeline( void )
{
    RWFUNCTION( RWSTRING( "_rpLtMapGetPlatformAtomicPipeline" ) );

    RWASSERT( _rpLtMapGlobals.module.numInstances > 0 );

    RWRETURN( openglLtMapPipeline( OPENGLLTMAPATOMICPIPEIDX ) );
}


/* ---------------------------------------------------------------------
 *  Function: _rpLtMapGetPlatformWorldSectorPipeline
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RxPipeline *
_rpLtMapGetPlatformWorldSectorPipeline( void )
{
    RWFUNCTION( RWSTRING( "_rpLtMapGetPlatformWorldSectorPipeline" ) );

    RWASSERT( _rpLtMapGlobals.module.numInstances > 0 );

    RWRETURN( openglLtMapPipeline( OPENGLLTMAPWORLDSECTORPIPEIDX ) );
}


/* =====================================================================
 *  Static function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: openglLtMapPipelineCreate
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static RxPipeline *
openglLtMapPipelineCreate( const RwInt32 type )
{
    RxPipeline  *pipe;


    RWFUNCTION( RWSTRING( "openglLtMapPipelineCreate" ) );

    pipe = RxPipelineCreate();
    if ( NULL != pipe )
    {
        RxLockedPipe    *lockedPipe;


        pipe->pluginId = rwID_LTMAPPLUGIN;

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


                pipeNode = RxPipelineFindNodeByName( pipe, allinoneNodeDef->name, NULL, NULL );
                RWASSERT( NULL != pipeNode );

                RxOpenGLAllInOneSetRenderCallBack( pipeNode, openglLtMapRenderCB );

                /* we want to instance to both display lists and vertex arrays since
                 * we need vertex arrays for some render styles */
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
 *  Function: openglLtMapRenderCB
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglLtMapRenderCB( RwResEntry *repEntry,
                     void *object,
                     const RwUInt8 type,
                     const RwUInt32 flags )
{
    RwBool  lightingEnabled;


    RWFUNCTION( RWSTRING( "openglLtMapRenderCB" ) );

    lightingEnabled = RwOpenGLIsEnabled( rwGL_LIGHTING );

    if ( 0 != (_rpLtMapGlobals.renderStyle & rpLTMAPSTYLERENDERLIGHTMAP) )
    {
        if ( 0 != (_rpLtMapGlobals.renderStyle & rpLTMAPSTYLERENDERBASE) )
        {
            if ( (FALSE != lightingEnabled) ||
                 (0 != (flags & rxGEOMETRY_PRELIT)) )
            {
                RWASSERT( NULL != renderWithLtMapVertexColorFn );
                renderWithLtMapVertexColorFn( repEntry,
                                              object,
                                              type,
                                              flags,
                                              lightingEnabled );
            }
            else
            {
                RWASSERT( NULL != renderWithLtMapFn );
                renderWithLtMapFn( repEntry,
                                   object,
                                   type,
                                   flags,
                                   lightingEnabled );
            }
        }
        else
        {
            renderLtMapOnlyFn( repEntry,
                               object,
                               type,
                               flags,
                               lightingEnabled );
        }
    }
    else
    {
        openglLtMapRenderBaseTextureOnly( repEntry, flags, lightingEnabled );
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglLtMapRenderBaseTextureOnly
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglLtMapRenderBaseTextureOnly( RwResEntry *repEntry,
                                  const RwUInt8 flags,
                                  const RwBool lightingEnabled )
{
    RxOpenGLResEntryHeader      *resEntryHeader;

    RxOpenGLMeshInstanceData    *instanceData;

    RwUInt16                    numMeshes;


    RWFUNCTION( RWSTRING( "openglLtMapRenderBaseTextureOnly" ) );

    /* get the instanced data */
    resEntryHeader = (RxOpenGLResEntryHeader *)(repEntry + 1);
    RWASSERT( NULL != resEntryHeader );

    instanceData = (RxOpenGLMeshInstanceData *)(resEntryHeader + 1);
    RWASSERT( NULL != instanceData );

    numMeshes = resEntryHeader->numMeshes;
    while ( numMeshes-- )
    {
        const RwRGBA    *matColor;

        RwBool          vertexAlphaEnabled;


        RWASSERT( NULL != instanceData->material );

        matColor = RpMaterialGetColor( instanceData->material );
        RWASSERT( NULL != matColor );

        if ( (FALSE != instanceData->vertexAlpha) ||
             (0xFF != matColor->alpha) )
        {
            vertexAlphaEnabled = TRUE;
        }
        else
        {
            vertexAlphaEnabled = FALSE;
        }
        _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)vertexAlphaEnabled );

        if ( FALSE != lightingEnabled )
        {
            /* set up the material properties */
            RpOpenGLWorldSetMaterialProperties( instanceData->material,
                                                flags );
        }
        else
        {
            RwOpenGLDisable( rwGL_COLOR_MATERIAL );
            if ( 0 == (flags & rxGEOMETRY_PRELIT) )
            {
                /* black */
                glColor4f( 0.0f, 0.0f, 0.0f, 1.0f );
            }
        }

        /* bind or unbind textures */
        if ( 0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2)) )
        {
            RwOpenGLSetTexture( RpMaterialGetTexture( instanceData->material ) );
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
                                   ((FALSE == vertexAlphaEnabled) ? 3 : 4),
                                   GL_UNSIGNED_BYTE,
                                   instanceData->vertexStride,
                                   instanceData->vaoName,
                                   instanceData->color );

            RwOpenGLVASetTexCoordATI( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))),
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
                                ((FALSE == vertexAlphaEnabled) ? 3 : 4),
                                GL_UNSIGNED_BYTE,
                                instanceData->vertexStride,
                                instanceData->color );

            RwOpenGLVASetTexCoord( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))),
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

        /* next RxOpenGLMeshInstanceData pointer */
        instanceData += 1;
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglLtMapGetLightMapTexture
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static RwTexture *
openglLtMapGetLightMapTexture( void *object, const RwUInt8 type )
{
    RwTexture   *lightMap;


    RWFUNCTION( RWSTRING( "openglLtMapGetLightMapTexture" ) );

    RWASSERT( NULL != object );

    if ( rpATOMIC == type )
    {
        lightMap = RpLtMapAtomicGetLightMap( (RpAtomic *)object );
    }
    else
    {
        lightMap = RpLtMapWorldSectorGetLightMap( (RpWorldSector *)object );
    }

    RWRETURN( lightMap );
}


/* ---------------------------------------------------------------------
 *  Function: openglLtMapRenderLightmapOnlyNoADD
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglLtMapRenderLightmapOnlyNoADD( RwResEntry *repEntry,
                                    void *object,
                                    const RwUInt8 type,
                                    const RwUInt32 flags,
                                    const RwBool lightingEnabled )
{
    RxOpenGLResEntryHeader      *resEntryHeader;

    RxOpenGLMeshInstanceData    *instanceData;

    RwUInt16                    numMeshes;

    RwTexture                   *ltMapTexture;

    RwBlendFunction             srcBlend;

    RwBlendFunction             destBlend;

    RwBool                      zWriteEnabled;


    RWFUNCTION( RWSTRING( "openglLtMapRenderLightmapOnlyNoADD" ) );

    /* get the instanced data */
    resEntryHeader = (RxOpenGLResEntryHeader *)(repEntry + 1);
    RWASSERT( NULL != resEntryHeader );

    /* first pass - lighting, diffuse, no texturing */
    instanceData = (RxOpenGLMeshInstanceData *)(resEntryHeader + 1);
    RWASSERT( NULL != instanceData );

    RwOpenGLSetTexture( NULL );

    numMeshes = resEntryHeader->numMeshes;
    while ( numMeshes-- )
    {
        const RwRGBA    *matColor;

        RwBool          vertexAlphaEnabled;


        RWASSERT( NULL != instanceData->material );

        matColor = RpMaterialGetColor( instanceData->material );
        RWASSERT( NULL != matColor );

        if ( (FALSE != instanceData->vertexAlpha) ||
             (0xFF != matColor->alpha) )
        {
            vertexAlphaEnabled = TRUE;
        }
        else
        {
            vertexAlphaEnabled = FALSE;
        }
        _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)vertexAlphaEnabled );

        if ( FALSE != lightingEnabled )
        {
            /* set up the material properties */
            RpOpenGLWorldSetMaterialProperties( instanceData->material,
                                                flags );
        }
        else
        {
            RwOpenGLDisable( rwGL_COLOR_MATERIAL );
            if ( 0 == (flags & rxGEOMETRY_PRELIT) )
            {
                /* black */
                glColor4f( 0.0f, 0.0f, 0.0f, 1.0f );
            }
        }

        /* must use the vertex array instancing here since the texture coordinates
         * required are set two */
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
                                   ((FALSE == vertexAlphaEnabled) ? 3 : 4),
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
                                ((FALSE == vertexAlphaEnabled) ? 3 : 4),
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

        /* next RxOpenGLMeshInstanceData pointer */
        instanceData += 1;
    }

    /* second pass - enable texturing, disable lighting and diffuse coloring */
    instanceData = (RxOpenGLMeshInstanceData *)(resEntryHeader + 1);
    RWASSERT( NULL != instanceData );

    ltMapTexture = openglLtMapGetLightMapTexture( object, type );
    if ( NULL != ltMapTexture )
    {
        RwOpenGLSetTexture( ltMapTexture );
        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
    }
    else
    {
        RwOpenGLSetTexture( NULL );
    }

    /* disable diffuse, normals, and lighting */
    RwOpenGLVADisableColor();
    RwOpenGLVADisableNormal();
    RwOpenGLDisable( rwGL_LIGHTING );
    RwOpenGLDisable( rwGL_COLOR_MATERIAL );

    _rwOpenGLGetRenderState( rwRENDERSTATESRCBLEND,          (void *)&srcBlend );
    _rwOpenGLGetRenderState( rwRENDERSTATEDESTBLEND,         (void *)&destBlend );
    _rwOpenGLGetRenderState( rwRENDERSTATEZWRITEENABLE,      (void *)&zWriteEnabled );

    _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE );
    _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND,          (void *)rwBLENDONE );
    _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND,         (void *)rwBLENDONE );
    _rwOpenGLSetRenderState( rwRENDERSTATEZWRITEENABLE,      (void *)FALSE );

    numMeshes = resEntryHeader->numMeshes;
    while ( numMeshes-- )
    {
        /* must use the vertex array instancing here since the texture coordinates
         * required are set two */
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

            RwOpenGLVASetTexCoordATI( (NULL != ltMapTexture),
                                      2,
                                      GL_FLOAT,
                                      instanceData->vertexStride,
                                      instanceData->vaoName,
                                      instanceData->texCoord[1] );

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

            RwOpenGLVASetTexCoord( (NULL != ltMapTexture),
                                   2,
                                   GL_FLOAT,
                                   instanceData->vertexStride,
                                   instanceData->texCoord[1] );

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

        /* next RxOpenGLMeshInstanceData pointer */
        instanceData += 1;
    }

    _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND,          (void *)srcBlend );
    _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND,         (void *)destBlend );
    _rwOpenGLSetRenderState( rwRENDERSTATEZWRITEENABLE,      (void *)zWriteEnabled );

    if ( NULL != ltMapTexture )
    {
        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglLtMapRenderLightmapOnlyADD
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglLtMapRenderLightmapOnlyADD( RwResEntry *repEntry,
                                  void *object,
                                  const RwUInt8 type,
                                  const RwUInt32 flags,
                                  const RwBool lightingEnabled )
{
    RxOpenGLResEntryHeader      *resEntryHeader;

    RxOpenGLMeshInstanceData    *instanceData;

    RwTexture                   *ltMapTexture;

    RwUInt16                    numMeshes;


    RWFUNCTION( RWSTRING( "openglLtMapRenderLightmapOnlyADD" ) );

    /* get the instanced data */
    resEntryHeader = (RxOpenGLResEntryHeader *)(repEntry + 1);
    RWASSERT( NULL != resEntryHeader );

    instanceData = (RxOpenGLMeshInstanceData *)(resEntryHeader + 1);
    RWASSERT( NULL != instanceData );

    ltMapTexture = openglLtMapGetLightMapTexture( object, type );
    if ( NULL != ltMapTexture )
    {
        RwOpenGLSetTexture( ltMapTexture );

        /* additive texture environment */
        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD );
    }
    else
    {
        RwOpenGLSetTexture( NULL );
    }

    numMeshes = resEntryHeader->numMeshes;
    while ( numMeshes-- )
    {
        const RwRGBA    *matColor;

        RwBool          vertexAlphaEnabled;


        RWASSERT( NULL != instanceData->material );

        matColor = RpMaterialGetColor( instanceData->material );
        RWASSERT( NULL != matColor );

        if ( (FALSE != instanceData->vertexAlpha) ||
             (0xFF != matColor->alpha) )
        {
            vertexAlphaEnabled = TRUE;
        }
        else
        {
            vertexAlphaEnabled = FALSE;
        }
        _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)vertexAlphaEnabled );

        if ( FALSE != lightingEnabled )
        {
            /* set up the material properties */
            RpOpenGLWorldSetMaterialProperties( instanceData->material,
                                                flags );
        }
        else
        {
            RwOpenGLDisable( rwGL_COLOR_MATERIAL );
            if ( 0 == (flags & rxGEOMETRY_PRELIT) )
            {
                /* black */
                glColor4f( 0.0f, 0.0f, 0.0f, 1.0f );
            }
        }

        /* must use the vertex array instancing here since the texture coordinates
         * required are set two */
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
                                   ((FALSE == vertexAlphaEnabled) ? 3 : 4),
                                   GL_UNSIGNED_BYTE,
                                   instanceData->vertexStride,
                                   instanceData->vaoName,
                                   instanceData->color );

            RwOpenGLVASetTexCoordATI( (NULL != ltMapTexture),
                                      2,
                                      GL_FLOAT,
                                      instanceData->vertexStride,
                                      instanceData->vaoName,
                                      instanceData->texCoord[1] );

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
                                ((FALSE == vertexAlphaEnabled) ? 3 : 4),
                                GL_UNSIGNED_BYTE,
                                instanceData->vertexStride,
                                instanceData->color );

            RwOpenGLVASetTexCoord( (NULL != ltMapTexture),
                                   2,
                                   GL_FLOAT,
                                   instanceData->vertexStride,
                                   instanceData->texCoord[1] );

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

        /* next RxOpenGLMeshInstanceData pointer */
        instanceData += 1;
    }

    if ( NULL != ltMapTexture )
    {
        /* reset texture environment */
        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglLtMapRenderWithLightmapMP
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglLtMapRenderWithLightmapMP( RwResEntry *repEntry,
                                 void *object,
                                 const RwUInt8 type,
                                 const RwUInt32 flags,
                                 const RwBool lightingEnabled )
{
    RxOpenGLResEntryHeader      *resEntryHeader;

    RxOpenGLMeshInstanceData    *instanceData;

    RwUInt16                    numMeshes;

    RwTexture                   *ltMapTexture;


    RWFUNCTION( RWSTRING( "openglLtMapRenderWithLightmapMP" ) );

    /* get the instanced data */
    resEntryHeader = (RxOpenGLResEntryHeader *)(repEntry + 1);
    RWASSERT( NULL != resEntryHeader );

    instanceData = (RxOpenGLMeshInstanceData *)(resEntryHeader + 1);
    RWASSERT( NULL != instanceData );

    ltMapTexture = openglLtMapGetLightMapTexture( object, type );

    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );

    numMeshes = resEntryHeader->numMeshes;
    while ( numMeshes-- )
    {
        const RwRGBA    *matColor;

        RwBool          vertexAlphaEnabled;

        RwTexture       *baseTexture;

        RwBlendFunction srcBlend;

        RwBlendFunction destBlend;

        RwBool          zWriteEnabled;

        RwBool          fogEnabled;

        RwUInt32        fogColor;


        RWASSERT( NULL != instanceData->material );

        matColor = RpMaterialGetColor( instanceData->material );
        RWASSERT( NULL != matColor );

        if ( (FALSE != instanceData->vertexAlpha) ||
             (0xFF != matColor->alpha) )
        {
            vertexAlphaEnabled = TRUE;
        }
        else
        {
            vertexAlphaEnabled = FALSE;
        }
        _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)vertexAlphaEnabled );

        if ( FALSE != lightingEnabled )
        {
            /* set up the material properties */
            RpOpenGLWorldSetMaterialProperties( instanceData->material,
                                                flags );
        }
        else
        {
            RwOpenGLDisable( rwGL_COLOR_MATERIAL );
            if ( 0 == (flags & rxGEOMETRY_PRELIT) )
            {
                /* black */
                glColor4f( 0.0f, 0.0f, 0.0f, 1.0f );
            }
        }

        /* first pass - texturing and lighting only, ignore diffuse */
        baseTexture = RpMaterialGetTexture( instanceData->material );
        if ( NULL != baseTexture )
        {
            RwOpenGLSetTexture( baseTexture );

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
                                       ((FALSE == vertexAlphaEnabled) ? 3 : 4),
                                       GL_UNSIGNED_BYTE,
                                       instanceData->vertexStride,
                                       instanceData->vaoName,
                                       instanceData->color );

                RwOpenGLVASetTexCoordATI( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))),
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
                                    ((FALSE == vertexAlphaEnabled) ? 3 : 4),
                                    GL_UNSIGNED_BYTE,
                                    instanceData->vertexStride,
                                    instanceData->color );

                RwOpenGLVASetTexCoord( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))),
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
        }

        /* second pass - disable lighting and modulate lightmap */
        RwOpenGLSetTexture( ltMapTexture );

        RwOpenGLDisable( rwGL_LIGHTING );

        _rwOpenGLGetRenderState( rwRENDERSTATESRCBLEND,          (void *)&srcBlend );
        _rwOpenGLGetRenderState( rwRENDERSTATEDESTBLEND,         (void *)&destBlend );
        _rwOpenGLGetRenderState( rwRENDERSTATEZWRITEENABLE,      (void *)&zWriteEnabled );
        _rwOpenGLGetRenderState( rwRENDERSTATEFOGENABLE,         (void *)&fogEnabled );
        _rwOpenGLGetRenderState( rwRENDERSTATEFOGCOLOR,          (void *)&fogColor );

        _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE );
        _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND,          (void *)rwBLENDZERO );
        _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND,         (void *)rwBLENDSRCCOLOR );
        _rwOpenGLSetRenderState( rwRENDERSTATEZWRITEENABLE,      (void *)FALSE );
        if ( FALSE != fogEnabled )
        {
            _rwOpenGLSetRenderState( rwRENDERSTATEFOGCOLOR,      (void *)0xFFFFFFFF );
        }

        /* can't use a display list since we need the second set of texture coordinates
         * in the first texture unit */
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
                                   ((FALSE == vertexAlphaEnabled) ? 3 : 4),
                                   GL_UNSIGNED_BYTE,
                                   instanceData->vertexStride,
                                   instanceData->vaoName,
                                   instanceData->color );

            RwOpenGLVASetTexCoordATI( (NULL != ltMapTexture),
                                      2,
                                      GL_FLOAT,
                                      instanceData->vertexStride,
                                      instanceData->vaoName,
                                      instanceData->texCoord[1] );

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
                                ((FALSE == vertexAlphaEnabled) ? 3 : 4),
                                GL_UNSIGNED_BYTE,
                                instanceData->vertexStride,
                                instanceData->color );

            RwOpenGLVASetTexCoord( (NULL != ltMapTexture),
                                   2,
                                   GL_FLOAT,
                                   instanceData->vertexStride,
                                   instanceData->texCoord[1] );

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

        if ( FALSE != fogEnabled )
        {
            _rwOpenGLSetRenderState( rwRENDERSTATEFOGCOLOR,      (void *)fogColor );
        }

        if ( FALSE != lightingEnabled )
        {
            RwOpenGLEnable( rwGL_LIGHTING );
        }

        _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND,          (void *)srcBlend );
        _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND,         (void *)destBlend );
        _rwOpenGLSetRenderState( rwRENDERSTATEZWRITEENABLE,      (void *)zWriteEnabled );

        /* next RxOpenGLMeshInstanceData pointer */
        instanceData += 1;
    }

    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglLtMapRenderWithLightmapMT
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglLtMapRenderWithLightmapMT( RwResEntry *repEntry,
                                 void *object,
                                 const RwUInt8 type,
                                 const RwUInt32 flags,
                                 const RwBool lightingEnabled )
{
    RxOpenGLResEntryHeader      *resEntryHeader;

    RxOpenGLMeshInstanceData    *instanceData;

    RwTexture                   *ltMapTexture;

    RwUInt16                    numMeshes;


    RWFUNCTION( RWSTRING( "openglLtMapRenderWithLightmapMT" ) );

    /* get the instanced data */
    resEntryHeader = (RxOpenGLResEntryHeader *)(repEntry + 1);
    RWASSERT( NULL != resEntryHeader );

    instanceData = (RxOpenGLMeshInstanceData *)(resEntryHeader + 1);
    RWASSERT( NULL != instanceData );

    /* lightmap texture */
    RwOpenGLSetActiveTextureUnit( 1 );
    ltMapTexture = openglLtMapGetLightMapTexture( object, type );
    RwOpenGLSetTexture( ltMapTexture );

    RwOpenGLSetActiveTextureUnit( 0 );
    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );

    numMeshes = resEntryHeader->numMeshes;
    while ( numMeshes-- )
    {
        const RwRGBA    *matColor;

        RwBool          vertexAlphaEnabled;

        RwTexture       *baseTexture;


        RWASSERT( NULL != instanceData->material );

        matColor = RpMaterialGetColor( instanceData->material );
        RWASSERT( NULL != matColor );

        if ( (FALSE != instanceData->vertexAlpha) ||
             (0xFF != matColor->alpha) )
        {
            vertexAlphaEnabled = TRUE;
        }
        else
        {
            vertexAlphaEnabled = FALSE;
        }
        _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)vertexAlphaEnabled );

        if ( FALSE != lightingEnabled )
        {
            /* set up the material properties */
            RpOpenGLWorldSetMaterialProperties( instanceData->material,
                                                flags );
        }
        else
        {
            RwOpenGLDisable( rwGL_COLOR_MATERIAL );
            if ( 0 == (flags & rxGEOMETRY_PRELIT) )
            {
                /* black */
                glColor4f( 0.0f, 0.0f, 0.0f, 1.0f );
            }
        }

        /* base texture */
        baseTexture = RpMaterialGetTexture( instanceData->material );
        RwOpenGLSetTexture( baseTexture );

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
                                   ((FALSE == vertexAlphaEnabled) ? 3 : 4),
                                   GL_UNSIGNED_BYTE,
                                   instanceData->vertexStride,
                                   instanceData->vaoName,
                                   instanceData->color );

            /* base texture coords */
            RwOpenGLVASetMultiTexCoordATI( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))),
                                           0,
                                           2,
                                           GL_FLOAT,
                                           instanceData->vertexStride,
                                           instanceData->vaoName,
                                           instanceData->texCoord[0] );
            /* lightmap texture coords */
            RwOpenGLVASetMultiTexCoordATI( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))),
                                           1,
                                           2,
                                           GL_FLOAT,
                                           instanceData->vertexStride,
                                           instanceData->vaoName,
                                           instanceData->texCoord[1] );

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
                                ((FALSE == vertexAlphaEnabled) ? 3 : 4),
                                GL_UNSIGNED_BYTE,
                                instanceData->vertexStride,
                                instanceData->color );

            /* base texture coords */
            RwOpenGLVASetMultiTexCoord( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))),
                                        0,
                                        2,
                                        GL_FLOAT,
                                        instanceData->vertexStride,
                                        instanceData->texCoord[0] );

            /* lightmap texture coords */
            RwOpenGLVASetMultiTexCoord( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))),
                                        1,
                                        2,
                                        GL_FLOAT,
                                        instanceData->vertexStride,
                                        instanceData->texCoord[1] );

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

        /* next RxOpenGLMeshInstanceData pointer */
        instanceData += 1;
    }

    RwOpenGLSetActiveTextureUnit( 1 );
    RwOpenGLSetTexture( NULL );
    RwOpenGLSetActiveTextureUnit( 0 );
    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    RwOpenGLSetTexture( NULL );

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglLtMapRenderWithLightMapVertexColorMPNoADD
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglLtMapRenderWithLightMapVertexColorMPNoADD( RwResEntry *repEntry,
                                                 void *object,
                                                 const RwUInt8 type,
                                                 const RwUInt32 flags,
                                                 const RwBool lightingEnabled )
{
    RxOpenGLResEntryHeader      *resEntryHeader;

    RxOpenGLMeshInstanceData    *instanceData;

    RwTexture                   *ltMapTexture;

    RwUInt16                    numMeshes;

    RwBool                      fogEnabled;

    RwRaster                    *textureRaster;


    RWFUNCTION( RWSTRING( "openglLtMapRenderWithLightMapVertexColorMPNoADD" ) );

    /* get the instanced data */
    resEntryHeader = (RxOpenGLResEntryHeader *)(repEntry + 1);
    RWASSERT( NULL != resEntryHeader );

    instanceData = (RxOpenGLMeshInstanceData *)(resEntryHeader + 1);
    RWASSERT( NULL != instanceData );

    ltMapTexture = openglLtMapGetLightMapTexture( object, type );
    RWASSERT( NULL != ltMapTexture );

    _rwOpenGLGetRenderState( rwRENDERSTATEFOGENABLE, (void *)&fogEnabled );

    _rwOpenGLGetRenderState( rwRENDERSTATETEXTURERASTER, (void *)&textureRaster );

    numMeshes = resEntryHeader->numMeshes;
    while ( numMeshes-- )
    {
        const RwRGBA    *matColor;

        RwBool          vertexAlphaEnabled;

        RwBlendFunction srcBlend;

        RwBlendFunction destBlend;

        RwBool          zWriteEnabled;

        RwTexture       *baseTexture;

        RwUInt32        fogColor;


        RWASSERT( NULL != instanceData->material );

        matColor = RpMaterialGetColor( instanceData->material );
        RWASSERT( NULL != matColor );

        if ( (FALSE != instanceData->vertexAlpha) ||
             (0xFF != matColor->alpha) )
        {
            vertexAlphaEnabled = TRUE;
        }
        else
        {
            vertexAlphaEnabled = FALSE;
        }
        _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)vertexAlphaEnabled );

        if ( FALSE != lightingEnabled )
        {
            /* set up the material properties */
            RpOpenGLWorldSetMaterialProperties( instanceData->material,
                                                flags );
        }
        else
        {
            RwOpenGLDisable( rwGL_COLOR_MATERIAL );
            if ( 0 == (flags & rxGEOMETRY_PRELIT) )
            {
                /* black */
                glColor4f( 0.0f, 0.0f, 0.0f, 1.0f );
            }
        }

        /* ---- first pass, diffuse only, no fog */
        RwOpenGLSetTexture( NULL );
        _rwOpenGLSetRenderState( rwRENDERSTATEFOGENABLE, (void *)FALSE );

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
                                   ((FALSE == vertexAlphaEnabled) ? 3 : 4),
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
                                ((FALSE == vertexAlphaEnabled) ? 3 : 4),
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

        /* --- second pass, additive blending, fog, no lights, using lightmap texture */
        if ( FALSE != fogEnabled )
        {
            _rwOpenGLSetRenderState( rwRENDERSTATEFOGENABLE, (void *)TRUE );
        }

        RwOpenGLDisable( rwGL_LIGHTING );

        _rwOpenGLGetRenderState( rwRENDERSTATESRCBLEND,          (void *)&srcBlend );
        _rwOpenGLGetRenderState( rwRENDERSTATEDESTBLEND,         (void *)&destBlend );
        _rwOpenGLGetRenderState( rwRENDERSTATEZWRITEENABLE,      (void *)&zWriteEnabled );

        _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE );
        _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND,          (void *)rwBLENDONE );
        _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND,         (void *)rwBLENDONE );
        _rwOpenGLSetRenderState( rwRENDERSTATEZWRITEENABLE,      (void *)FALSE );

        RwOpenGLSetTexture( ltMapTexture );

        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );

        /* can't call a display list since we're skipping texture coordinates */
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
                                   ((FALSE == vertexAlphaEnabled) ? 3 : 4),
                                   GL_UNSIGNED_BYTE,
                                   instanceData->vertexStride,
                                   instanceData->vaoName,
                                   instanceData->color );

            RwOpenGLVASetTexCoordATI( TRUE,
                                      2,
                                      GL_FLOAT,
                                      instanceData->vertexStride,
                                      instanceData->vaoName,
                                      instanceData->texCoord[1] );

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
                                ((FALSE == vertexAlphaEnabled) ? 3 : 4),
                                GL_UNSIGNED_BYTE,
                                instanceData->vertexStride,
                                instanceData->color );

            RwOpenGLVASetTexCoord( TRUE,
                                   2,
                                   GL_FLOAT,
                                   instanceData->vertexStride,
                                   instanceData->texCoord[1] );

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

        /* --- third pass, modulate base texture, fogging set to white */
        _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND,          (void *)rwBLENDZERO );
        _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND,         (void *)rwBLENDSRCCOLOR );

        baseTexture = RpMaterialGetTexture( instanceData->material );
        RwOpenGLSetTexture( baseTexture );

        if ( FALSE != fogEnabled )
        {
            _rwOpenGLGetRenderState( rwRENDERSTATEFOGCOLOR,          (void *)&fogColor );
            _rwOpenGLSetRenderState( rwRENDERSTATEFOGCOLOR,          (void *)0xFFFFFFFF );
        }

        if ( GL_FALSE != glIsList( instanceData->displayList ) )
        {
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
                                   ((FALSE == vertexAlphaEnabled) ? 3 : 4),
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
                                ((FALSE == vertexAlphaEnabled) ? 3 : 4),
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

        if ( FALSE != lightingEnabled )
        {
            RwOpenGLEnable( rwGL_LIGHTING );
        }

        if ( FALSE != fogEnabled )
        {
            _rwOpenGLSetRenderState( rwRENDERSTATEFOGCOLOR,          (void *)fogColor );
        }

        _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND,          (void *)srcBlend );
        _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND,         (void *)destBlend );
        _rwOpenGLSetRenderState( rwRENDERSTATEZWRITEENABLE,      (void *)zWriteEnabled );

        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

        /* next RxOpenGLMeshInstanceData pointer */
        instanceData += 1;
    }

    _rwOpenGLSetRenderState( rwRENDERSTATETEXTURERASTER, (void *)textureRaster );

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglLtMapRenderWithLightMapVertexColorMPADD
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglLtMapRenderWithLightMapVertexColorMPADD( RwResEntry *repEntry,
                                               void *object,
                                               const RwUInt8 type,
                                               const RwUInt32 flags,
                                               const RwBool lightingEnabled )
{
    RxOpenGLResEntryHeader      *resEntryHeader;

    RxOpenGLMeshInstanceData    *instanceData;

    RwTexture                   *ltMapTexture;

    RwUInt16                    numMeshes;

    RwBool                      fogEnabled;


    RWFUNCTION( RWSTRING( "openglLtMapRenderWithLightMapVertexColorMPADD" ) );

    /* get the instanced data */
    resEntryHeader = (RxOpenGLResEntryHeader *)(repEntry + 1);
    RWASSERT( NULL != resEntryHeader );

    instanceData = (RxOpenGLMeshInstanceData *)(resEntryHeader + 1);
    RWASSERT( NULL != instanceData );

    ltMapTexture = openglLtMapGetLightMapTexture( object, type );
    RWASSERT( NULL != ltMapTexture );

    _rwOpenGLGetRenderState( rwRENDERSTATEFOGENABLE, (void *)&fogEnabled );

    numMeshes = resEntryHeader->numMeshes;
    while ( numMeshes-- )
    {
        const RwRGBA    *matColor;

        RwBool          vertexAlphaEnabled;

        RwBlendFunction srcBlend;

        RwBlendFunction destBlend;

        RwBool          zWriteEnabled;

        RwTexture       *baseTexture;

        RwUInt32        fogColor;


        RWASSERT( NULL != instanceData->material );

        matColor = RpMaterialGetColor( instanceData->material );
        RWASSERT( NULL != matColor );

        if ( (FALSE != instanceData->vertexAlpha) ||
             (0xFF != matColor->alpha) )
        {
            vertexAlphaEnabled = TRUE;
        }
        else
        {
            vertexAlphaEnabled = FALSE;
        }
        _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)vertexAlphaEnabled );

        if ( FALSE != lightingEnabled )
        {
            /* set up the material properties */
            RpOpenGLWorldSetMaterialProperties( instanceData->material,
                                                flags );
        }
        else
        {
            RwOpenGLDisable( rwGL_COLOR_MATERIAL );
            if ( 0 == (flags & rxGEOMETRY_PRELIT) )
            {
                /* black */
                glColor4f( 0.0f, 0.0f, 0.0f, 1.0f );
            }
        }

        /* ---- first pass, diffuse + lightmap */
        RwOpenGLSetTexture( ltMapTexture );
        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD );

        /* can't use display lists since we have to choose the second texture coordinate set */
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
                                   ((FALSE == vertexAlphaEnabled) ? 3 : 4),
                                   GL_UNSIGNED_BYTE,
                                   instanceData->vertexStride,
                                   instanceData->vaoName,
                                   instanceData->color );

            RwOpenGLVASetTexCoordATI( TRUE,
                                      2,
                                      GL_FLOAT,
                                      instanceData->vertexStride,
                                      instanceData->vaoName,
                                      instanceData->texCoord[1] );

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
                                ((FALSE == vertexAlphaEnabled) ? 3 : 4),
                                GL_UNSIGNED_BYTE,
                                instanceData->vertexStride,
                                instanceData->color );

            RwOpenGLVASetTexCoord( TRUE,
                                   2,
                                   GL_FLOAT,
                                   instanceData->vertexStride,
                                   instanceData->texCoord[1] );

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

        /* --- second pass, modulate base texture, fogging set to white */
        RwOpenGLDisable( rwGL_LIGHTING );

        _rwOpenGLGetRenderState( rwRENDERSTATESRCBLEND,          (void *)&srcBlend );
        _rwOpenGLGetRenderState( rwRENDERSTATEDESTBLEND,         (void *)&destBlend );
        _rwOpenGLGetRenderState( rwRENDERSTATEZWRITEENABLE,      (void *)&zWriteEnabled );

        _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE );
        _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND,          (void *)rwBLENDZERO );
        _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND,         (void *)rwBLENDSRCCOLOR );
        _rwOpenGLSetRenderState( rwRENDERSTATEZWRITEENABLE,      (void *)FALSE );

        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );

        baseTexture = RpMaterialGetTexture( instanceData->material );
        RwOpenGLSetTexture( baseTexture );

        if ( FALSE != fogEnabled )
        {
            _rwOpenGLGetRenderState( rwRENDERSTATEFOGCOLOR,          (void *)&fogColor );
            _rwOpenGLSetRenderState( rwRENDERSTATEFOGCOLOR,          (void *)0xFFFFFFFF );
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
                                   ((FALSE == vertexAlphaEnabled) ? 3 : 4),
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
                                ((FALSE == vertexAlphaEnabled) ? 3 : 4),
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

        if ( FALSE != lightingEnabled )
        {
            RwOpenGLEnable( rwGL_LIGHTING );
        }

        if ( FALSE != fogEnabled )
        {
            _rwOpenGLSetRenderState( rwRENDERSTATEFOGCOLOR,          (void *)fogColor );
        }

        _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND,          (void *)srcBlend );
        _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND,         (void *)destBlend );
        _rwOpenGLSetRenderState( rwRENDERSTATEZWRITEENABLE,      (void *)zWriteEnabled );

        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

        /* next RxOpenGLMeshInstanceData pointer */
        instanceData += 1;
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglLtMapRenderWithLightMapVertexColorMTADD
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglLtMapRenderWithLightMapVertexColorMTADD( RwResEntry *repEntry,
                                               void *object,
                                               const RwUInt8 type,
                                               const RwUInt32 flags,
                                               const RwBool lightingEnabled )
{
    RxOpenGLResEntryHeader      *resEntryHeader;

    RxOpenGLMeshInstanceData    *instanceData;

    RwTexture                   *ltMapTexture;

    RwUInt16                    numMeshes;


    RWFUNCTION( RWSTRING( "openglLtMapRenderWithLightMapVertexColorMTADD" ) );

    /* get the instanced data */
    resEntryHeader = (RxOpenGLResEntryHeader *)(repEntry + 1);
    RWASSERT( NULL != resEntryHeader );

    instanceData = (RxOpenGLMeshInstanceData *)(resEntryHeader + 1);
    RWASSERT( NULL != instanceData );

    /* lightmap texture */
    RwOpenGLSetActiveTextureUnit( 0 );
    ltMapTexture = openglLtMapGetLightMapTexture( object, type );
    if ( NULL != ltMapTexture )
    {
        RwOpenGLSetTexture( ltMapTexture );

        /* additive texture environment */
        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD );
    }
    else
    {
        RwOpenGLSetTexture( NULL );
    }

    RwOpenGLSetActiveTextureUnit( 1 );
    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

    numMeshes = resEntryHeader->numMeshes;
    while ( numMeshes-- )
    {
        const RwRGBA    *matColor;

        RwBool          vertexAlphaEnabled;

        RwTexture       *baseTexture;


        RWASSERT( NULL != instanceData->material );

        matColor = RpMaterialGetColor( instanceData->material );
        RWASSERT( NULL != matColor );

        if ( (FALSE != instanceData->vertexAlpha) ||
             (0xFF != matColor->alpha) )
        {
            vertexAlphaEnabled = TRUE;
        }
        else
        {
            vertexAlphaEnabled = FALSE;
        }
        _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)vertexAlphaEnabled );

        if ( FALSE != lightingEnabled )
        {
            /* set up the material properties */
            RpOpenGLWorldSetMaterialProperties( instanceData->material,
                                                flags );
        }
        else
        {
            RwOpenGLDisable( rwGL_COLOR_MATERIAL );
            if ( 0 == (flags & rxGEOMETRY_PRELIT) )
            {
                /* black */
                glColor4f( 0.0f, 0.0f, 0.0f, 1.0f );
            }
        }

        /* base texture */
        baseTexture = RpMaterialGetTexture( instanceData->material );
        RwOpenGLSetTexture( baseTexture );

        /* can't use display lists */
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
                                   ((FALSE == vertexAlphaEnabled) ? 3 : 4),
                                   GL_UNSIGNED_BYTE,
                                   instanceData->vertexStride,
                                   instanceData->vaoName,
                                   instanceData->color );

            /* base texture coords */
            RwOpenGLVASetMultiTexCoordATI( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))) ||
                                           (NULL != baseTexture) || (NULL != ltMapTexture),
                                           1,
                                           2,
                                           GL_FLOAT,
                                           instanceData->vertexStride,
                                           instanceData->vaoName,
                                           instanceData->texCoord[0] );

            /* lightmap texture coords */
            RwOpenGLVASetMultiTexCoordATI( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))) ||
                                           (NULL != baseTexture) || (NULL != ltMapTexture),
                                           0,
                                           2,
                                           GL_FLOAT,
                                           instanceData->vertexStride,
                                           instanceData->vaoName,
                                           instanceData->texCoord[1] );

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
                                ((FALSE == vertexAlphaEnabled) ? 3 : 4),
                                GL_UNSIGNED_BYTE,
                                instanceData->vertexStride,
                                instanceData->color );

            /* base texture coords */
            RwOpenGLVASetMultiTexCoord( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))) ||
                                        (NULL != baseTexture) || (NULL != ltMapTexture),
                                        1,
                                        2,
                                        GL_FLOAT,
                                        instanceData->vertexStride,
                                        instanceData->texCoord[0] );

            /* lightmap texture coords */
            RwOpenGLVASetMultiTexCoord( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))) ||
                                        (NULL != baseTexture) || (NULL != ltMapTexture),
                                        0,
                                        2,
                                        GL_FLOAT,
                                        instanceData->vertexStride,
                                        instanceData->texCoord[1] );

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

        /* next RxOpenGLMeshInstanceData pointer */
        instanceData += 1;
    }

    RwOpenGLSetTexture( NULL );

    RwOpenGLSetActiveTextureUnit( 0 );
    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    RwOpenGLSetTexture( NULL );

    RWRETURNVOID();
}
