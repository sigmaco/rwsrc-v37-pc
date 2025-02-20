/***********************************************************************
 *
 * Module:  effectPipesOpenGL.c
 *
 * Purpose:
 *
 ***********************************************************************/

/* =====================================================================
 *  Includes
 * ===================================================================== */
#include <string.h> /* for memset() */

#include "rwcore.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "effectPipes.h"
#include "matfxplatform.h"


/**
 * \ingroup rpmatfxopengl
 * \page rpmatfxopengloverview OpenGL RpMatFX Overview
 *
 * The \ref rpmatfx plugin, under OpenGL, mostly uses multi-pass methods to
 * achieve the material effects.
 *
 * In one exception, if the \c GL_ARB_multitexture and
 * \c GL_ARB_texture_env_combine extensions are available, and an environment
 * map with shinyness parameter set to unity is used \e without fogging,
 * multitexturing will be used for improved performance. To use multitexturing
 * exclusively for environment mapping, it is suggested that a number of
 * environment map textures be used to simulate any required variation in the
 * shinyness parameter.
 *
 * When using the environment map effect with a non-opaque base texture, the
 * results observed may not be \e exactly the same as on other platforms.
 *
 * If the \c GL_ARB_texture_cube_map extension is available, environment map
 * texture coordinates are calculated on hardware making use of the
 * \c GL_NORMAL_MAP_ARB texgen. Otherwise, a software equivalent is used.
 * Also, if the environment map is a cube map raster, the \c GL_REFLECTION_MAP_ARB
 * texgen is used to generate the appropriate texture coordinates. There is
 * no software equivalent in this case.
 *
 * Bump mapping now takes care of the type and position of lights.
 *
 * \see \ref RwOpenGLRasterIsCubeMap
 *
 */

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
typedef void (*openglMatFXEnvMapApplication)( RwMatrix *objectLTM,
                                              RxOpenGLMeshInstanceData *instanceData,
                                              const MatFXEnvMapData *envMapData );


/* =====================================================================
 *  Static variables
 * ===================================================================== */
static RxPipeline                   *openglMatFXAtomicPipeline = (RxPipeline *)NULL;

static RxPipeline                   *openglMatFXWorldSectorPipeline = (RxPipeline *)NULL;

static RwBool                       *openglBumpMapProcessedFlags = (RwBool *)NULL;

static RwTexCoords                  *openglBumpMapUVData = (RwTexCoords *)NULL;
static RwUInt32                     openglBumpMapUVDataVAO = 0;

static RwTexCoords                  *openglEnvMapUVData = (RwTexCoords *)NULL;
static RwUInt32                     openglEnvMapUVDataVAO = 0;

static openglMatFXEnvMapApplication openglMatFXEnvMapApply = (openglMatFXEnvMapApplication)NULL;

static RwReal                       openglTextureTransform[16] = { 1.0f, 0.0f, 0.0f, 0.0f,
                                                                   0.0f, 1.0f, 0.0f, 0.0f,
                                                                   0.0f, 0.0f, 1.0f, 0.0f,
                                                                   0.0f, 0.0f, 0.0f, 1.0f };


/* =====================================================================
 *  Global variables
 * ===================================================================== */
_rpMatFXOpenGLEnvMapRenderCB    _rpMatFXOpenGLEnvMapRender = (_rpMatFXOpenGLEnvMapRenderCB)NULL;

_rpMatFXOpenGLBumpMapRenderCB   _rpMatFXOpenGLBumpMapRender = (_rpMatFXOpenGLBumpMapRenderCB)NULL;

_rpMatFXOpenGLDualRenderCB      _rpMatFXOpenGLDualRender = (_rpMatFXOpenGLDualRenderCB)NULL;

_rpMatFXOpenGLUVAnimRenderCB    _rpMatFXOpenGLUVAnimRender = (_rpMatFXOpenGLUVAnimRenderCB)NULL;


/* =====================================================================
 *  Static function prototypes
 * ===================================================================== */
static RxPipeline *
openglMatFXWorldSectorPipelineCreate( void );

static void
openglMatFXApplyEnvMap( RwMatrix *objectLTM,
                        RxOpenGLMeshInstanceData *instanceData,
                        const MatFXEnvMapData *envMapData );

static void
openglMatFXApplyEnvMapSoft( RwMatrix *objectLTM,
                            RxOpenGLMeshInstanceData *instanceData,
                            const MatFXEnvMapData *envMapData );

static const RpLight *
openglMatFXFrameGetFirstLight( const RwFrame *frame );

static RwBool
openglMatFXCalcBumpUVs( RwMatrix *objectLTM,
                        RxOpenGLMeshInstanceData *instanceData,
                        const MatFXBumpMapData *bumpMapData );

static void
openglMatFXEnvMapRenderMP( RwMatrix *objectLTM,
                           RxOpenGLMeshInstanceData *instanceData,
                           RwTexture *baseTexture,
                           const RwUInt32 flags,
                           const void *envMapDataVoid );

static void
openglMatFXEnvMapRenderMTCombiner( RwMatrix *objectLTM,
                                   RxOpenGLMeshInstanceData *instanceData,
                                   RwTexture *baseTexture,
                                   const RwUInt32 flags,
                                   const void *envMapDataVoid );

static void
openglMatFXBumpMapRenderMP( RwMatrix *objectLTM,
                            RxOpenGLMeshInstanceData *instanceData,
                            RwTexture *baseTexture,
                            const RwUInt32 flags,
                            const void *bumpMapDataVoid,
                            const void *envMapDataVoid );

static void
openglMatFXDualRenderMP( RxOpenGLMeshInstanceData *instanceData,
                         RwTexture *baseTexture,
                         const RwUInt32 flags,
                         const void *dualDataVoid,
                         const void *uvanimDataVoid );

static void
openglUVAnimRender( RxOpenGLMeshInstanceData *instanceData,
                    RwTexture *baseTexture,
                    const RwUInt32 flags,
                    const void *uvanimDataVoid );

static void
openglApplyUVTransform( RwMatrix *transform );


/* =====================================================================
 *  Global function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: _rpMatFXPipelinesCreate
 *
 *  Purpose : Can't do many multitexturing effects since we lack a multiply-add
 *            texture combiner.
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwBool
_rpMatFXPipelinesCreate( void )
{
    RWFUNCTION( RWSTRING( "_rpMatFXPipelinesCreate" ) );

    /* MatFX atomic pipeline */
    openglMatFXAtomicPipeline = _rpMatFXOpenGLAtomicPipelineCreate();
    RWASSERT( NULL != openglMatFXAtomicPipeline );

    /* MatFX world sector pipeline */
    openglMatFXWorldSectorPipeline = openglMatFXWorldSectorPipelineCreate();
    RWASSERT( NULL != openglMatFXWorldSectorPipeline );

    /* environment maps */
    _rpMatFXOpenGLEnvMapRender = openglMatFXEnvMapRenderMP;
    if ( (_rwOpenGLExt.MaxTextureUnits >= 2) &&
         (FALSE != _rwOpenGLExt.TextureEnvCombineARB) )
    {
        _rpMatFXOpenGLEnvMapRender = openglMatFXEnvMapRenderMTCombiner;
    }

    /* bump maps */
    _rpMatFXOpenGLBumpMapRender = openglMatFXBumpMapRenderMP;

    /* dual pass */
    _rpMatFXOpenGLDualRender = openglMatFXDualRenderMP;

    /* uv animation */
    _rpMatFXOpenGLUVAnimRender = openglUVAnimRender;

    /* environment map texture coordinate generation */
    openglMatFXEnvMapApply = openglMatFXApplyEnvMapSoft;
    if ( FALSE != _rwOpenGLExt.TextureCubeMapARB )
    {
        openglMatFXEnvMapApply = openglMatFXApplyEnvMap;
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rpMatFXPipelinesDestroy
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwBool
_rpMatFXPipelinesDestroy( void )
{
    RWFUNCTION( RWSTRING( "_rpMatFXPipelinesDestroy" ) );

    if ( !_rwOpenGLVertexHeapAvailable() )
    {
        if ( NULL != openglBumpMapUVData )
        {
            RwFree( openglBumpMapUVData );
            openglBumpMapUVData = (RwTexCoords *)NULL;
        }

        if ( NULL != openglEnvMapUVData )
        {
            RwFree( openglEnvMapUVData );
            openglEnvMapUVData = (RwTexCoords *)NULL;
        }

        if ( NULL != _rwOpenGLExt.FreeObjectBufferATI )
        {
            if ( GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( openglEnvMapUVDataVAO ) )
            {
                _rwOpenGLExt.FreeObjectBufferATI( openglEnvMapUVDataVAO );
                openglEnvMapUVDataVAO = 0;
            }

            if ( GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( openglBumpMapUVDataVAO ) )
            {
                _rwOpenGLExt.FreeObjectBufferATI( openglBumpMapUVDataVAO );
                openglBumpMapUVDataVAO = 0;
            }
        }
    }

    if ( NULL != openglBumpMapProcessedFlags )
    {
        RwFree( openglBumpMapProcessedFlags );
        openglBumpMapProcessedFlags = (RwBool *)NULL;
    }

    if ( NULL != openglMatFXWorldSectorPipeline )
    {
        RxPipelineDestroy( openglMatFXWorldSectorPipeline );
        openglMatFXWorldSectorPipeline = (RxPipeline *)NULL;
    }

    if ( NULL != openglMatFXAtomicPipeline )
    {
        RxPipelineDestroy( openglMatFXAtomicPipeline );
        openglMatFXAtomicPipeline = (RxPipeline *)NULL;
    }

    RWRETURN( TRUE );
}

/* ---------------------------------------------------------------------
 *  Function: _rpMatFXPipelineAtomicSetup
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RpAtomic *
_rpMatFXPipelineAtomicSetup( RpAtomic *atomic )
{
    RWFUNCTION( RWSTRING( "_rpMatFXPipelineAtomicSetup" ) );

    RWASSERT( NULL != atomic );
    RpAtomicSetPipeline( atomic, openglMatFXAtomicPipeline );

    RWRETURN( atomic );
}


/* ---------------------------------------------------------------------
 *  Function: _rpMatFXPipelineWorldSectorSetup
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RpWorldSector *
_rpMatFXPipelineWorldSectorSetup( RpWorldSector *worldSector )
{
    RWFUNCTION( RWSTRING( "_rpMatFXPipelineWorldSectorSetup" ) );

    RWASSERT( NULL != worldSector );
    RpWorldSectorSetPipeline( worldSector, openglMatFXWorldSectorPipeline );

    RWRETURN( worldSector );
}


/* ---------------------------------------------------------------------
 *  Function: _rpMatFXSetupBumpMapTexture
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwTexture *
_rpMatFXSetupBumpMapTexture( const RwTexture *baseTexture,
                             const RwTexture *effectTexture )
{
    RwTexture   *texture;


    RWFUNCTION( RWSTRING( "_rpMatFXSetupBumpMapTexture" ) );

    /* baseTexture can be NULL */
    RWASSERT( NULL != effectTexture );

    texture = _rpMatFXTextureMaskCreate( baseTexture, effectTexture );

    RWRETURN( texture );
}


/* ---------------------------------------------------------------------
 *  Function: _rpMatFXSetupDualRenderState
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwBool
_rpMatFXSetupDualRenderState( MatFXDualData *dualData __RWUNUSEDRELEASE__,
                              RwRenderState nState __RWUNUSED__ )
{
    RWFUNCTION( RWSTRING( "_rpMatFXSetupDualRenderState" ) );

    RWASSERT( NULL != dualData );

    RWRETURN( TRUE );
}


/**
 * \ingroup rpmatfxopengl
 * \ref RpMatFXGetOpenGLPipeline
 *
 * Returns one of the \ref rpmatfx OpenGL pipelines.
 *
 * The \ref rpmatfx plugin must be attached before using this function.
 *
 * \param openglPipeline Type of the requested pipeline.
 *
 * \return The \ref RxPipeline requested from the plugin, or
 * NULL if the pipeline wasn't constructed.
 */
RxPipeline *
RpMatFXGetOpenGLPipeline( RpMatFXOpenGLPipeline openglPipeline )
{
    RxPipeline  *pipeline;


    RWAPIFUNCTION( RWSTRING( "RpMatFXGetOpenGLPipeline" ) );

    RWASSERT( 0 < MatFXInfo.Module.numInstances );
    RWASSERT( rpNAMATFXOPENGLPIPELINE < openglPipeline );
    RWASSERT( rpMATFXOPENGLPIPELINEMAX > openglPipeline );

    switch ( openglPipeline )
    {
    case rpMATFXOPENGLATOMICPIPELINE:
        {
            pipeline = openglMatFXAtomicPipeline;
        }
        break;

    case rpMATFXOPENGLWORLDSECTORPIPELINE:
        {
            pipeline = openglMatFXWorldSectorPipeline;
        }
        break;

    default:
        pipeline = (RxPipeline *)NULL;
    }

    RWRETURN( pipeline );
}


/* ---------------------------------------------------------------------
 *  Function: _rpMatFXOpenGLAllInOneRenderCB
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
void
_rpMatFXOpenGLAllInOneRenderCB( RwResEntry *repEntry,
                                void *object,
                                const RwUInt8 type,
                                const RwUInt32 flags )
{
    RxOpenGLResEntryHeader      *resEntryHeader;

    RxOpenGLMeshInstanceData    *instanceData;

    RwMatrix                    identityMtx;

    RwMatrix                    *objectLTM;

    RwUInt16                    numMeshes;


    RWFUNCTION( RWSTRING( "_rpMatFXOpenGLAllInOneRenderCB" ) );

    /* get the instanced data */
    resEntryHeader = (RxOpenGLResEntryHeader *)(repEntry + 1);
    RWASSERT( NULL != resEntryHeader );

    instanceData = (RxOpenGLMeshInstanceData *)(resEntryHeader + 1);
    RWASSERT( NULL != instanceData );

    RwMatrixSetIdentity( &identityMtx );

    if ( rpATOMIC == type )
    {
        RwFrame *atomicFrame;


        atomicFrame = RpAtomicGetFrame( (RpAtomic *)object );
        RWASSERT( NULL != atomicFrame );

        objectLTM = RwFrameGetLTM( atomicFrame );
    }
    else
    {
        objectLTM = &identityMtx;
    }

    numMeshes = resEntryHeader->numMeshes;
    while ( numMeshes-- )
    {
        const RwRGBA                *matColor;

        const rpMatFXMaterialData   *matfxData;

        RpMatFXMaterialFlags        effectType;


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

        if ( FALSE != RwOpenGLIsEnabled( rwGL_LIGHTING ) )
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

        matfxData = *MATFXMATERIALGETDATA( instanceData->material );
        if ( NULL == matfxData )
        {
            effectType = rpMATFXEFFECTNULL;
        }
        else
        {
            effectType = matfxData->flags;
        }

        switch ( effectType )
        {
        case rpMATFXEFFECTBUMPMAP:
            {
                const MatFXBumpMapData  *bumpMapData;


                bumpMapData = MATFXOPENGLBUMPMAPGETDATA( instanceData->material );
                RWASSERT( NULL != bumpMapData );

                RWASSERT( NULL != _rpMatFXOpenGLBumpMapRender );
                _rpMatFXOpenGLBumpMapRender( objectLTM,
                                             instanceData,
                                             RpMaterialGetTexture(instanceData->material),
                                             flags,
                                             bumpMapData,
                                             (void *)NULL );
            }
            break;

        case rpMATFXEFFECTENVMAP:
            {
                const MatFXEnvMapData   *envMapData;


                envMapData = MATFXOPENGLENVMAPGETDATA( instanceData->material, rpSECONDPASS );
                RWASSERT( NULL != envMapData );

                RWASSERT( NULL != _rpMatFXOpenGLEnvMapRender );
                _rpMatFXOpenGLEnvMapRender( objectLTM,
                                            instanceData,
                                            RpMaterialGetTexture(instanceData->material),
                                            flags,
                                            envMapData );
            }
            break;

        case rpMATFXEFFECTBUMPENVMAP:
            {
                const MatFXEnvMapData   *envMapData;

                const MatFXBumpMapData  *bumpMapData;


                envMapData = MATFXOPENGLENVMAPGETDATA( instanceData->material, rpTHIRDPASS );
                RWASSERT( NULL != envMapData );

                bumpMapData = MATFXOPENGLBUMPMAPGETDATA( instanceData->material );
                RWASSERT( NULL != bumpMapData );

                _rpMatFXOpenGLBumpMapRender( objectLTM,
                                             instanceData,
                                             RpMaterialGetTexture(instanceData->material),
                                             flags,
                                             bumpMapData,
                                             envMapData );
            }
            break;

        case rpMATFXEFFECTDUAL:
            {
                const MatFXDualData *dualData;


                dualData = MATFXOPENGLDUALGETDATA( instanceData->material );
                RWASSERT( NULL != dualData );

                _rpMatFXOpenGLDualRender( instanceData,
                                          RpMaterialGetTexture(instanceData->material),
                                          flags,
                                          dualData,
                                          NULL );
            }
            break;

        case rpMATFXEFFECTUVTRANSFORM:
            {
                const MatFXUVAnimData   *uvanimData;


                uvanimData = MATFXOPENGLUVANIMGETDATA( instanceData->material );
                RWASSERT( NULL != uvanimData );

                _rpMatFXOpenGLUVAnimRender( instanceData,
                                            RpMaterialGetTexture(instanceData->material),
                                            flags,
                                            uvanimData );
            }
            break;

        case rpMATFXEFFECTDUALUVTRANSFORM:
            {
                const MatFXDualData     *dualData;

                const MatFXUVAnimData   *uvanimData;


                dualData = MATFXOPENGLDUALUVANIMGETDUALDATA( instanceData->material );
                RWASSERT( NULL != dualData );

                uvanimData = MATFXOPENGLUVANIMGETDATA( instanceData->material );
                RWASSERT( NULL != uvanimData );

                _rpMatFXOpenGLDualRender( instanceData,
                                          RpMaterialGetTexture(instanceData->material),
                                          flags,
                                          dualData,
                                          uvanimData );
            }
            break;

        default:
            {
                _rpMatFXOpenGLDefaultRender( instanceData,
                                             RpMaterialGetTexture(instanceData->material),
                                             flags );
            }
        }

        /* next RxOpenGLMeshInstanceData pointer */
        instanceData += 1;
    }

    RWRETURNVOID();
}



/* ---------------------------------------------------------------------
 *  Function: _rpMatFXOpenGLAtomicPipelineCreate
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RxPipeline *
_rpMatFXOpenGLAtomicPipelineCreate( void )
{
    RxPipeline  *pipe;


    RWFUNCTION( RWSTRING( "_rpMatFXOpenGLAtomicPipelineCreate" ) );

    pipe = RxPipelineCreate();
    if ( NULL != pipe )
    {
        RxLockedPipe    *lockedPipe;


        pipe->pluginId = rwID_MATERIALEFFECTSPLUGIN;

        lockedPipe = RxPipelineLock( pipe );
        if ( NULL != lockedPipe )
        {
            RxNodeDefinition    *allinoneNodeDef;

            RxPipelineNode      *allinonePipeNode;


            allinoneNodeDef = RxNodeDefinitionGetOpenGLAtomicAllInOne();
            RWASSERT( NULL != allinoneNodeDef );

            lockedPipe = RxLockedPipeAddFragment( lockedPipe,
                                                  NULL,
                                                  allinoneNodeDef,
                                                  NULL );
            RWASSERT( NULL != lockedPipe );

            lockedPipe = RxLockedPipeUnlock( lockedPipe );

            RWASSERT( lockedPipe == pipe );

            allinonePipeNode = RxPipelineFindNodeByName( pipe,
                                                         allinoneNodeDef->name,
                                                         NULL,
                                                         NULL );
            RWASSERT( NULL != allinonePipeNode );

            RxOpenGLAllInOneSetRenderCallBack( allinonePipeNode, _rpMatFXOpenGLAllInOneRenderCB );

            /* we want both display lists and vertex array instancing */
            RxOpenGLAllInOneSetInstanceDLandVA( allinonePipeNode, TRUE );

            RWRETURN( pipe );

        }

        RxPipelineDestroy( pipe );
        pipe = (RxPipeline *)NULL;
    }

    RWRETURN( pipe );
}


/* ---------------------------------------------------------------------
 *  Function: _rpMatFXOpenGLDefaultRender
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
void
_rpMatFXOpenGLDefaultRender( RxOpenGLMeshInstanceData *instanceData,
                             RwTexture *baseTexture,
                             const RwUInt32 flags )
{
    RWFUNCTION( RWSTRING( "_rpMatFXOpenGLDefaultRender" ) );

    RWASSERT( NULL != instanceData );

    /* bind or unbind textures */
    if ( 0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2)) )
    {
        RwOpenGLSetTexture( baseTexture );
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
                            4,
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

    RWRETURNVOID();
}


/* =====================================================================
 *  Static function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: openglMatFXWorldSectorPipelineCreate
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static RxPipeline *
openglMatFXWorldSectorPipelineCreate( void )
{
    RxPipeline  *pipe;


    RWFUNCTION( RWSTRING( "openglMatFXWorldSectorPipelineCreate" ) );

    pipe = RxPipelineCreate();
    if ( NULL != pipe )
    {
        RxLockedPipe    *lockedPipe;


        pipe->pluginId = rwID_MATERIALEFFECTSPLUGIN;

        lockedPipe = RxPipelineLock( pipe );
        if ( NULL != lockedPipe )
        {
            RxNodeDefinition    *allinoneNodeDef;

            RxPipelineNode      *allinonePipeNode;


            allinoneNodeDef = RxNodeDefinitionGetOpenGLWorldSectorAllInOne();
            RWASSERT( NULL != allinoneNodeDef );

            lockedPipe = RxLockedPipeAddFragment( lockedPipe,
                                                  NULL,
                                                  allinoneNodeDef,
                                                  NULL );
            RWASSERT( NULL != lockedPipe );

            lockedPipe = RxLockedPipeUnlock( lockedPipe );

            RWASSERT( lockedPipe == pipe );

            allinoneNodeDef = RxNodeDefinitionGetOpenGLWorldSectorAllInOne();
            RWASSERT( NULL != allinoneNodeDef );

            allinonePipeNode = RxPipelineFindNodeByName( pipe,
                                                         allinoneNodeDef->name,
                                                         NULL,
                                                         NULL );
            RWASSERT( NULL != allinonePipeNode );

            RxOpenGLAllInOneSetRenderCallBack( allinonePipeNode, _rpMatFXOpenGLAllInOneRenderCB );

            /* we want both display lists and vertex array instancing */
            RxOpenGLAllInOneSetInstanceDLandVA( allinonePipeNode, TRUE );

            RWRETURN( pipe );

        }

        RxPipelineDestroy( pipe );
        pipe = (RxPipeline *)NULL;
    }

    RWRETURN( pipe );
}


/* ---------------------------------------------------------------------
 *  Function: openglMatFXApplyEnvMap
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglMatFXApplyEnvMap( RwMatrix *objectLTM __RWUNUSED__,
                        RxOpenGLMeshInstanceData *instanceData __RWUNUSED__,
                        const MatFXEnvMapData *envMapData )
{
    static RwMatrix texMat =
    {
        {((RwReal)0.5), ((RwReal)0.0), ((RwReal)0.0)}, 0,
        {((RwReal)0.0),-((RwReal)0.5), ((RwReal)0.0)}, 0,
        {((RwReal)0.0), ((RwReal)0.0), ((RwReal)1.0)}, 0,
        {((RwReal)0.5), ((RwReal)0.5), ((RwReal)0.0)}, 0
    };

    RwTexture   *envMapTexture;

    RwUInt32    activeTexUnit = RwOpenGLGetActiveTextureUnit();


    RWFUNCTION( RWSTRING( "openglMatFXApplyEnvMap" ) );

    envMapTexture = envMapData->texture;

    RwOpenGLSetTexture( envMapTexture );

    RwOpenGLEnable( rwGL_TEXTURE0_GEN_S + activeTexUnit );
    RwOpenGLEnable( rwGL_TEXTURE0_GEN_T + activeTexUnit );
    RwOpenGLEnable( rwGL_TEXTURE0_GEN_R + activeTexUnit );

    glMatrixMode( GL_TEXTURE );
    glPushMatrix();
    glLoadIdentity();

    if ( FALSE != RwOpenGLRasterIsCubeMap( RwTextureGetRaster( envMapTexture ) ) )
    {
        RwCamera        *camera;

        RwFrame         *camFrame;

        const RwMatrix  *camLTM;

        RwMatrix        tmpMatrix;


        glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB );
        glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB );
        glTexGeni( GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB );

        camera = RwCameraGetCurrentCamera();
        RWASSERT( NULL != camera );

        camFrame = RwCameraGetFrame( camera );
        RWASSERT( NULL != camFrame );

        camLTM = RwFrameGetLTM( camFrame );
        RWASSERT( NULL != camLTM );

        tmpMatrix.right.x = -camLTM->right.x;
        tmpMatrix.right.y = -camLTM->right.y;
        tmpMatrix.right.z = -camLTM->right.z;

        tmpMatrix.up.x = camLTM->up.x;
        tmpMatrix.up.y = camLTM->up.y;
        tmpMatrix.up.z = camLTM->up.z;

        tmpMatrix.at.x = -camLTM->at.x;
        tmpMatrix.at.y = -camLTM->at.y;
        tmpMatrix.at.z = -camLTM->at.z;

        tmpMatrix.pos.x = 0.0f;
        tmpMatrix.pos.y = 0.0f;
        tmpMatrix.pos.z = 0.0f;

        _rwOpenGLApplyRwMatrix( &tmpMatrix );
    }
    else
    {
        glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB );
        glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB );
        glTexGeni( GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB );

        if ( NULL != envMapData->frame )
        {
            RwCamera        *camera;

            RwFrame         *camFrame;

            const RwMatrix  *camLTM;

            const RwMatrix  *envLTM;

            RwMatrix        invMatrix;

            RwMatrix        tmpMatrix;

            RwMatrix        result;


            /* transform the normals back into world space */
            camera = RwCameraGetCurrentCamera();
            RWASSERT( NULL != camera );

            camFrame = RwCameraGetFrame( camera );
            RWASSERT( NULL != camFrame );

            camLTM = RwFrameGetLTM( camFrame );
            RWASSERT( NULL != camLTM );

            /* transform the normals by the inverse of the env maps frame */
            envLTM = RwFrameGetLTM( envMapData->frame );
            RWASSERT( NULL != envLTM );

            RwMatrixInvert( &invMatrix, envLTM );

            RwMatrixMultiply( &tmpMatrix, &invMatrix, camLTM );

            tmpMatrix.right.x = -tmpMatrix.right.x;
            tmpMatrix.right.y = -tmpMatrix.right.y;
            tmpMatrix.right.z = -tmpMatrix.right.z;

            tmpMatrix.at.x = -tmpMatrix.at.x;
            tmpMatrix.at.y = -tmpMatrix.at.y;
            tmpMatrix.at.z = -tmpMatrix.at.z;

            tmpMatrix.flags = 0;

            tmpMatrix.pos.x = 0.0f;
            tmpMatrix.pos.y = 0.0f;
            tmpMatrix.pos.z = 0.0f;

            RwMatrixUpdate( &tmpMatrix );

            RwMatrixMultiply( &result, &tmpMatrix, &texMat );

            _rwOpenGLApplyRwMatrix( &result );
        }
        else
        {
            _rwOpenGLApplyRwMatrix( (RwMatrix *)&texMat );
        }
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglMatFXApplyEnvMapSoft
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglMatFXApplyEnvMapSoft( RwMatrix *objectLTM,
                            RxOpenGLMeshInstanceData *instanceData,
                            const MatFXEnvMapData *envMapData )
{
    RwTexture       *envMapTexture;

    RwFrame         *envMapFrame;

    RwMatrix        invEnvMapFrameLTM;

    RwMatrix        envMapTransformMtx;

    static RwUInt32 lastEnvMapUVSize = 0;

    RwUInt8         *baseVertexMem;

    RwTexCoords     *localEnvMapUVData;

    RwUInt32        i;


    RWFUNCTION( RWSTRING( "openglMatFXApplyEnvMapSoft" ) );

    RWASSERT( NULL != instanceData );
    RWASSERT( NULL != envMapData );

    envMapTexture = envMapData->texture;

    RwOpenGLSetTexture( envMapTexture );

    envMapFrame        = envMapData->frame;

    if ( NULL != envMapFrame )
    {
        RwMatrix    *envMapFrameLTM;


        envMapFrameLTM = RwFrameGetLTM( envMapFrame );
        RWASSERT( NULL != envMapFrameLTM );

        RwMatrixInvert( &invEnvMapFrameLTM, envMapFrameLTM );
    }
    else
    {
        RwCamera    *currentCamera;

        RwFrame     *cameraFrame;

        RwMatrix    *cameraFrameMatrix;


        currentCamera = RwCameraGetCurrentCamera();
        RWASSERT( NULL != currentCamera );

        cameraFrame = RwCameraGetFrame( currentCamera );
        RWASSERT( NULL != cameraFrame );

        cameraFrameMatrix = RwFrameGetLTM( cameraFrame );
        RWASSERT( NULL != cameraFrameMatrix );

        RwMatrixInvert( &invEnvMapFrameLTM, cameraFrameMatrix );
    }

    RwMatrixMultiply( &envMapTransformMtx, objectLTM, &invEnvMapFrameLTM );

    /* allocate some storage for the env map texture coords */
    if ( _rwOpenGLVertexHeapAvailable() )
    {
        if ( NULL != openglEnvMapUVData )
        {
            _rwOpenGLVertexHeapDynamicDiscard( openglEnvMapUVData );
        }
        openglEnvMapUVData = (RwTexCoords *)_rwOpenGLVertexHeapDynamicMalloc( sizeof(RwTexCoords) * instanceData->numVertices, TRUE );
        RWASSERT( NULL != openglEnvMapUVData );
    }
    else if ( instanceData->numVertices > lastEnvMapUVSize )
    {
        if ( NULL != openglEnvMapUVData )
        {
            RwFree( openglEnvMapUVData );
        }

        if ( NULL != _rwOpenGLExt.NewObjectBufferATI )
        {
            if ( GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( openglEnvMapUVDataVAO ) )
            {
                _rwOpenGLExt.FreeObjectBufferATI( openglEnvMapUVDataVAO );
            }

            openglEnvMapUVDataVAO = _rwOpenGLExt.NewObjectBufferATI( sizeof(RwTexCoords) * instanceData->numVertices,
                                                                     NULL,
                                                                     GL_DYNAMIC_ATI );
            RWASSERT( GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( openglEnvMapUVDataVAO ) );
        }

        openglEnvMapUVData = (RwTexCoords *)RwMalloc( sizeof(RwTexCoords) * instanceData->numVertices,
                                              rwID_MATERIALEFFECTSPLUGIN | rwMEMHINTDUR_EVENT );
        RWASSERT( NULL != openglEnvMapUVData );

        lastEnvMapUVSize = instanceData->numVertices;
    }

    baseVertexMem = instanceData->vidMemVertexData;
    if ( NULL == baseVertexMem )
    {
        baseVertexMem = instanceData->sysMemVertexData;
    }
    RWASSERT( NULL != baseVertexMem );

    localEnvMapUVData = openglEnvMapUVData;
    for ( i = 0; i < instanceData->numVertices; i += 1 )
    {
        RwV3d   normal;


        RwV3dAssign( &normal, (RwV3d *)(baseVertexMem + sizeof(RwV3d)) );

        RwV3dTransformVector( &normal, &normal, &envMapTransformMtx );

        openglEnvMapUVData->u = 0.5f * (1.0f + normal.x);
        openglEnvMapUVData->v = -0.5f * (1.0f + normal.y);

        baseVertexMem += instanceData->vertexStride;
        openglEnvMapUVData += 1;
    }
    openglEnvMapUVData = localEnvMapUVData;

    if ( NULL != _rwOpenGLExt.UpdateObjectBufferATI )
    {
        RWASSERT( GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( openglEnvMapUVDataVAO ) );
        _rwOpenGLExt.UpdateObjectBufferATI( openglEnvMapUVDataVAO,
                                            0,
                                            sizeof(RwTexCoords) * instanceData->numVertices,
                                            openglEnvMapUVData,
                                            GL_DISCARD_ATI );
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglMatFXFrameGetFirstLight
 *
 *  Purpose : Find and return the first light that owns the frame
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static const RpLight *
openglMatFXFrameGetFirstLight( const RwFrame *frame )
{
    const RpLight   *light = (RpLight *)NULL;

    const RwLLLink  *current;

    const RwLLLink  *next;

    const RwLLLink  *end;

    RwInt32         objcount = 0;


    RWFUNCTION( RWSTRING( "openglMatFXFrameGetFirstLight" ) );

    current = rwLinkListGetFirstLLLink(&frame->objectList);
    end = rwLinkListGetTerminator(&frame->objectList);

    while ( current != end )
    {
        const RwObject  *object;


        next = rwLLLinkGetNext(current);
        object = (const RwObject *)
            rwLLLinkGetConstData(current,RwObjectHasFrame, lFrame);

        if (RwObjectGetType(object) == rpLIGHT)
        {
            light = (const RpLight *)object;

            break;
        }

        objcount++;
        current = next;
    }

    RWRETURN( light );
}


/* ---------------------------------------------------------------------
 *  Function: openglMatFXCalcBumpUVs
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static RwBool
openglMatFXCalcBumpUVs( RwMatrix *objectLTM,
                        RxOpenGLMeshInstanceData *instanceData,
                        const MatFXBumpMapData *bumpMapData )
{
    RwUInt32        numVerts;

    RpMaterial      *material;

    RxVertexIndex   *inds;

    RwUInt32        texCoordsOffset;

    static RwUInt32 lastBumpUVDataSize = 0;


    RWFUNCTION( RWSTRING( "openglMatFXCalcBumpUVs" ) );

    numVerts = instanceData->numVertices;

    material = instanceData->material;

    inds = instanceData->indexData;

    /* calculate the offset in bytes to the texture coordinates in this mesh */
    texCoordsOffset = sizeof(RwV3d);
    if ( 0 != (instanceData->vertexDesc & rxGEOMETRY_NORMALS) )
    {
        texCoordsOffset += sizeof(RwV3d);
    }
    if ( 0 != (instanceData->vertexDesc & rxGEOMETRY_PRELIT) )
    {
        texCoordsOffset += sizeof(RwRGBA);
    }

    /* allocate some memory for the bump UVs */
    if ( _rwOpenGLVertexHeapAvailable() )
    {
        if ( NULL != openglBumpMapUVData )
        {
            _rwOpenGLVertexHeapDynamicDiscard( openglBumpMapUVData );
        }
        openglBumpMapUVData = (RwTexCoords *)_rwOpenGLVertexHeapDynamicMalloc( sizeof(RwTexCoords) * numVerts, TRUE );
        RWASSERT( NULL != openglBumpMapUVData );
    }
    else if ( numVerts > lastBumpUVDataSize )
    {
        if ( NULL != openglBumpMapUVData )
        {
            RwFree( openglBumpMapUVData );
        }

        if ( NULL != _rwOpenGLExt.NewObjectBufferATI )
        {
            if ( GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( openglBumpMapUVDataVAO ) )
            {
                _rwOpenGLExt.FreeObjectBufferATI( openglBumpMapUVDataVAO );
            }

            openglBumpMapUVDataVAO = _rwOpenGLExt.NewObjectBufferATI( sizeof(RwTexCoords) * numVerts,
                                                                      NULL,
                                                                      GL_DYNAMIC_ATI );
            RWASSERT( GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( openglBumpMapUVDataVAO ) );
        }

        openglBumpMapUVData = (RwTexCoords *)RwMalloc( sizeof(RwTexCoords) * numVerts,
                                     rwID_MATERIALEFFECTSPLUGIN | rwMEMHINTDUR_EVENT );
        RWASSERT( NULL != openglBumpMapUVData );

        lastBumpUVDataSize = numVerts;
    }

    if ( NULL != inds )
    {
        static RwUInt32 sizeProcessedFlags = 0;

        RwReal          factor;

        RwFrame         *bumpFrame;

        RwMatrix        *bumpLTM;

        RwMatrix        worldToObj;

        const RpLight   *firstLight;

        RwV3d           lightPosObj;

        RwUInt32        i;

        RwUInt8         *baseVertexMem;


        if ( sizeof(RwBool) * numVerts > sizeProcessedFlags )
        {
            if ( NULL != openglBumpMapProcessedFlags )
            {
                RwFree( openglBumpMapProcessedFlags );
            }

            sizeProcessedFlags = sizeof(RwBool) * numVerts;

            openglBumpMapProcessedFlags = (RwBool *)RwMalloc( sizeProcessedFlags,
                                rwID_MATERIALEFFECTSPLUGIN | rwMEMHINTDUR_EVENT );
        }
        RWASSERT( NULL != openglBumpMapProcessedFlags );

        memset( openglBumpMapProcessedFlags, 0, sizeProcessedFlags );

        factor = bumpMapData->coef * bumpMapData->invBumpWidth;

        bumpFrame = bumpMapData->frame;
        if ( NULL == bumpFrame )
        {
            bumpFrame = RwCameraGetFrame( RwCameraGetCurrentCamera() );
        }
        RWASSERT( NULL != bumpFrame );

        bumpLTM = RwFrameGetLTM( bumpFrame );
        RWASSERT( NULL != bumpLTM );

        /* invert objectLTM */
        RwMatrixInvert( &worldToObj, objectLTM );

        firstLight = openglMatFXFrameGetFirstLight(bumpFrame);

        if ( (NULL != firstLight) &&
             ( (rpLIGHTDIRECTIONAL == RpLightGetType(firstLight)) ||
               (rpLIGHTSPOT        == RpLightGetType(firstLight)) ||
               (rpLIGHTSPOTSOFT    == RpLightGetType(firstLight)) ) )
        {
            const RwV3d *lightAt;

            RwV3d       lightAtInv;


            lightAt = RwMatrixGetAt( bumpLTM );

            lightAtInv.x = -lightAt->x;
            lightAtInv.y = -lightAt->y;
            lightAtInv.z = -lightAt->z;

            RwV3dTransformVector( &lightPosObj,
                                  &lightAtInv,
                                  &worldToObj );
        }
        else
        {
            const RwV3d *lightPos;


            lightPos = RwMatrixGetPos( bumpLTM );

            RwV3dTransformPoint( &lightPosObj,
                                 lightPos,
                                 &worldToObj );
        }

        baseVertexMem = instanceData->vidMemVertexData;
        if ( NULL == baseVertexMem )
        {
             baseVertexMem = instanceData->sysMemVertexData;
        }
        RWASSERT( NULL != baseVertexMem );

        for ( i = 0; i < instanceData->numIndices; i += 1 )
        {
            if ( FALSE == openglBumpMapProcessedFlags[inds[i]] )
            {
                RwUInt8 *vert1;

                RwUInt8 *vert2;

                RwUInt8 *vert3;

                RwV3d   pos1;

                RwV3d   pos2;

                RwV3d   pos3;

                RwV3d   b;

                RwV3d   t;

                RwV3d   n;

                RwV3d   temp1;

                RwV3d   temp2;

                RwV3d   l;


                if ( GL_TRIANGLES == instanceData->primType )
                {
                    vert1 = baseVertexMem + instanceData->vertexStride * inds[i];
                    vert2 = baseVertexMem + instanceData->vertexStride * inds[((i/3)*3) + (i+1)%3];
                    vert3 = baseVertexMem + instanceData->vertexStride * inds[((i/3)*3) + (i+2)%3];
                }
                else if ( GL_TRIANGLE_STRIP == instanceData->primType )
                {
                    RwInt32 i1;

                    RwInt32 i2;

                    RwInt32 i3;


                    i1 = i;

                    if ( i < 2 )
                    {
                        if ( 0 != (i % 2) )
                        {
                            i2 = i + 2;
                            i3 = i + 1;
                        }
                        else
                        {
                            i2 = i + 1;
                            i3 = i + 2;
                        }
                    }
                    else
                    {
                        if ( 0 != (i % 2) )
                        {
                            i2 = i - 1;
                            i3 = i - 2;
                        }
                        else
                        {
                            i2 = i - 2;
                            i3 = i - 1;
                        }
                    }

                    vert1 = baseVertexMem + instanceData->vertexStride * inds[i1];
                    vert2 = baseVertexMem + instanceData->vertexStride * inds[i2];
                    vert3 = baseVertexMem + instanceData->vertexStride * inds[i3];
                }
                else
                {
                    vert1 = (RwUInt8 *)NULL;
                    vert2 = (RwUInt8 *)NULL;
                    vert3 = (RwUInt8 *)NULL;
                }

                RWASSERT( NULL != vert1 );
                RWASSERT( NULL != vert2 );
                RWASSERT( NULL != vert3 );

                RwV3dAssign( &pos1, (RwV3d *)(vert1) );
                RwV3dAssign( &n,    (RwV3d *)(vert1 + sizeof(RwV3d)) );

                RwV3dSub( &l, &lightPosObj, &pos1 );
                RwV3dNormalize( &l, &l );

                /* Check to see whether the light is behind the triangle */
                if (1) /* RwV3dDotProduct(&l, &n) > 0.0f) */
                {
                    RwReal  unused;

                    RwV2d   shift;



                    RwV3dAssign( &pos2, (RwV3d *)(vert2) );
                    RwV3dAssign( &pos3, (RwV3d *)(vert3) );

                    /* a nice little algorithm to find the tangent vector */
                    RwV3dSub( &temp1, &pos2, &pos1 );
                    RwV3dSub( &temp2, &pos3, &pos1 );

                    RwV3dScale( &temp1, &temp1,
                                ((RwTexCoords *)(vert3 + texCoordsOffset))->v - ((RwTexCoords *)(vert1 + texCoordsOffset))->v );
                    RwV3dScale( &temp2, &temp2,
                                ((RwTexCoords *)(vert2 + texCoordsOffset))->v - ((RwTexCoords *)(vert1 + texCoordsOffset))->v );

                    RwV3dSub( &t, &temp1, &temp2 );
                    _rwV3dNormalizeMacro( unused, &t, &t );
                    RwV3dCrossProduct( &b, &t, &n );

                    /*
                     * So now that we have b, t and n, we have the tangent
                     * space coordinate system axes.
                     */
                    shift.x = RwV3dDotProduct( &t, &l );
                    shift.y = RwV3dDotProduct( &b, &l );

                    openglBumpMapUVData[inds[i]].u = ((RwTexCoords *)(vert1 + texCoordsOffset))->u - (shift.x * factor);
                    openglBumpMapUVData[inds[i]].v = ((RwTexCoords *)(vert1 + texCoordsOffset))->v - (shift.y * factor);
                }

                openglBumpMapProcessedFlags[inds[i]] = TRUE;
            }
        }
    }

    if ( NULL != _rwOpenGLExt.UpdateObjectBufferATI )
    {
        RWASSERT( GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( openglBumpMapUVDataVAO ) );
        _rwOpenGLExt.UpdateObjectBufferATI( openglBumpMapUVDataVAO,
                                            0,
                                            sizeof(RwTexCoords) * numVerts,
                                            openglBumpMapUVData,
                                            GL_DISCARD_ATI );
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: openglMatFXEnvMapRenderMP
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglMatFXEnvMapRenderMP( RwMatrix *objectLTM,
                           RxOpenGLMeshInstanceData *instanceData,
                           RwTexture *baseTexture,
                           const RwUInt32 flags,
                           const void *envMapDataVoid )
{
    const MatFXEnvMapData   *envMapData;

    RwTexture               *envMapTexture;

    RwUInt32                envMapCoeff;


    RWFUNCTION( RWSTRING( "openglMatFXEnvMapRenderMP" ) );

    RWASSERT( NULL != instanceData );
    RWASSERT( NULL != envMapDataVoid );

    envMapData = (const MatFXEnvMapData *)envMapDataVoid;

    envMapTexture = envMapData->texture;

    envMapCoeff = (RwFastRealToUInt32(envMapData->coef * 255) & 0xFF);

    if ( (NULL != envMapTexture) && (0 != envMapCoeff) )
    {
        RwBlendFunction srcBlend;

        RwBlendFunction dstBlend;

        RwBool          zWriteEnabled;

        RwBool          fogEnabled;

        RwUInt32        fogColor;


        /* FIRST PASS: render with the base texture */
        if ( 0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2)) )
        {
            RwOpenGLSetTexture( baseTexture );
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
                                4,
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

        /* SECOND PASS: render with the env.map texture and modulate the shinyness */
        _rwOpenGLGetRenderState( rwRENDERSTATESRCBLEND, (void *)&srcBlend );
        _rwOpenGLGetRenderState( rwRENDERSTATEDESTBLEND, (void *)&dstBlend );
        _rwOpenGLGetRenderState( rwRENDERSTATEZWRITEENABLE, (void *)&zWriteEnabled );

        openglMatFXEnvMapApply( objectLTM, instanceData, envMapData );

        _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE );

        _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND, (void *)rwBLENDSRCALPHA );
        _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND, (void *)rwBLENDONE );
        _rwOpenGLSetRenderState( rwRENDERSTATEZWRITEENABLE, (void *)FALSE );

        _rwOpenGLGetRenderState( rwRENDERSTATEFOGENABLE, (void *)&fogEnabled );
        if ( FALSE != fogEnabled )
        {
            _rwOpenGLGetRenderState( rwRENDERSTATEFOGCOLOR, (void *)&fogColor );
            _rwOpenGLSetRenderState( rwRENDERSTATEFOGCOLOR, (void *)0 );
        }

        RwOpenGLDisable( rwGL_LIGHTING );
        RwOpenGLVADisableColor();

        glColor4ub( 255, 255, 255, envMapCoeff );

        /* can't use display lists here since we modify texture coordinates */
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

            if ( openglMatFXApplyEnvMapSoft == openglMatFXEnvMapApply )
            {
                RWASSERT( GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( openglEnvMapUVDataVAO ) );
                RwOpenGLVASetTexCoordATI( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))),
                                          2,
                                          GL_FLOAT,
                                          sizeof(RwTexCoords),
                                          openglEnvMapUVDataVAO,
                                          0 );
            }
            else
            {
                /* tex coords have been texgen'd */
                RwOpenGLVADisableTexCoord( 0 );
            }

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

            if ( openglMatFXApplyEnvMapSoft == openglMatFXEnvMapApply )
            {
                RwOpenGLVASetTexCoord( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))),
                                       2,
                                       GL_FLOAT,
                                       sizeof(RwTexCoords),
                                       (RwUInt8 *)openglEnvMapUVData );
            }
            else
            {
                /* tex coords have been texgen'd */
                RwOpenGLVADisableTexCoord( 0 );
            }

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
            _rwOpenGLSetRenderState( rwRENDERSTATEFOGCOLOR, (void *)fogColor );
        }

        if ( openglMatFXApplyEnvMapSoft != openglMatFXEnvMapApply )
        {
            glPopMatrix();
            glMatrixMode( GL_MODELVIEW );

            RwOpenGLDisable( rwGL_TEXTURE0_GEN_S );
            RwOpenGLDisable( rwGL_TEXTURE0_GEN_T );
            RwOpenGLDisable( rwGL_TEXTURE0_GEN_R );
        }

        RwOpenGLEnable( rwGL_LIGHTING );

        _rwOpenGLSetRenderState( rwRENDERSTATEZWRITEENABLE, (void *)zWriteEnabled );
        _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND, (void *)srcBlend );
        _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND, (void *)dstBlend );
    }
    else
    {
        _rpMatFXOpenGLDefaultRender( instanceData, baseTexture, flags );
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglMatFXEnvMapRenderMTCombiner
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglMatFXEnvMapRenderMTCombiner( RwMatrix *objectLTM,
                                   RxOpenGLMeshInstanceData *instanceData,
                                   RwTexture *baseTexture,
                                   const RwUInt32 flags,
                                   const void *envMapDataVoid )
{
    const MatFXEnvMapData   *envMapData;

    RwTexture               *envMapTexture;

    RwUInt32                envMapCoeff;

    RwBool                  fogEnabled;


    RWFUNCTION( RWSTRING( "openglMatFXEnvMapRenderMTCombiner" ) );

    RWASSERT( NULL != instanceData );
    RWASSERT( NULL != envMapDataVoid );

    envMapData = (const MatFXEnvMapData *)envMapDataVoid;

    envMapTexture = envMapData->texture;

    envMapCoeff = (RwFastRealToUInt32(envMapData->coef * 255) & 0xFF);

    RwRenderStateGet( rwRENDERSTATEFOGENABLE, (void *)&fogEnabled );

    /* special case for shinyness == 1 - we can't do general shinyness values
     * since OpenGL lacks a multipy-add texture environment mode, nor can we
     * use multitexturing when fogging is enabled */
    if ( (255 == envMapCoeff) && (NULL != envMapTexture) && (FALSE == fogEnabled) )
    {
        /* bind or unbind the base texture */
        RwOpenGLSetActiveTextureUnit( 0 );
        if ( 0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2)) )
        {
            RwOpenGLSetTexture( baseTexture );
        }
        else
        {
            RwOpenGLSetTexture( NULL );
        }

        /* bind the environment map */
        RwOpenGLSetActiveTextureUnit( 1 );
        openglMatFXEnvMapApply( objectLTM, instanceData, envMapData );
        /* we want to add the RGB but take the base texture alpha value */
        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,  GL_COMBINE_ARB );
        glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,   GL_ADD );
        glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,   GL_PREVIOUS_ARB );
        glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,   GL_TEXTURE );
        glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE );
        glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB );

        /* can't use display lists here since we're modifying texture coordinates */
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

            if ( openglMatFXApplyEnvMapSoft == openglMatFXEnvMapApply )
            {
                RwOpenGLVASetMultiTexCoordATI( TRUE,
                                               1,
                                               2,
                                               GL_FLOAT,
                                               sizeof(RwTexCoords),
                                               openglEnvMapUVDataVAO,
                                               0 );
            }
            else
            {
                /* texture coords are auto-generated */
                RwOpenGLVADisableTexCoord( 1 );
            }

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

            if ( openglMatFXApplyEnvMapSoft == openglMatFXEnvMapApply )
            {
                RwOpenGLVASetMultiTexCoord( TRUE,
                                            1,
                                            2,
                                            GL_FLOAT,
                                            sizeof(RwTexCoords),
                                            (RwUInt8 *)openglEnvMapUVData );
            }
            else
            {
                /* texture coords are auto-generated */
                RwOpenGLVADisableTexCoord( 1 );
            }

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

        RwOpenGLSetActiveTextureUnit( 1 );
        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
        RwOpenGLSetTexture( NULL );

        if ( openglMatFXApplyEnvMapSoft != openglMatFXEnvMapApply )
        {
            glPopMatrix();
            glMatrixMode( GL_MODELVIEW );

            RwOpenGLDisable( rwGL_TEXTURE1_GEN_S );
            RwOpenGLDisable( rwGL_TEXTURE1_GEN_T );
            RwOpenGLDisable( rwGL_TEXTURE1_GEN_R );
        }

        RwOpenGLSetActiveTextureUnit( 0 );
        RwOpenGLSetTexture( NULL );
    }
    else
    {
        openglMatFXEnvMapRenderMP( objectLTM,
                                   instanceData,
                                   baseTexture,
                                   flags,
                                   envMapDataVoid );
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglMatFXBumpMapRenderMP
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglMatFXBumpMapRenderMP( RwMatrix *objectLTM,
                            RxOpenGLMeshInstanceData *instanceData,
                            RwTexture *baseTexture,
                            const RwUInt32 flags,
                            const void *bumpMapDataVoid,
                            const void *envMapDataVoid )
{
    const MatFXBumpMapData  *bumpMapData;

    const MatFXEnvMapData   *envMapData;

    RwTexture               *bumpTexture;

    RwTexture               *envMapTexture;

    RwUInt32                envMapCoeff;

    RwBool                  vertexAlphaEnabled;

    RwBlendFunction         srcBlend;

    RwBlendFunction         destBlend;

    RwBool                  zWriteEnabled;

    GLboolean               alphaTestEnabled;



    RWFUNCTION( RWSTRING( "openglMatFXBumpMapRenderMP" ) );

    RWASSERT( NULL != instanceData );
    RWASSERT( NULL != bumpMapDataVoid );

    bumpMapData = (const MatFXBumpMapData *)bumpMapDataVoid;

    bumpTexture = bumpMapData->texture;
    if ( NULL != bumpTexture )
    {
        /* bind the bump texture */
        RwOpenGLSetTexture( bumpTexture );

        _rwOpenGLGetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)&vertexAlphaEnabled );
        _rwOpenGLGetRenderState( rwRENDERSTATESRCBLEND,          (void *)&srcBlend );
        _rwOpenGLGetRenderState( rwRENDERSTATEDESTBLEND,         (void *)&destBlend );

        _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE );
        _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND,          (void *)rwBLENDINVSRCALPHA );
        _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND,         (void *)rwBLENDZERO );

        alphaTestEnabled = RwOpenGLIsEnabled( rwGL_ALPHA_TEST );
        RwOpenGLDisable( rwGL_ALPHA_TEST );

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
                                4,
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

        _rwOpenGLGetRenderState( rwRENDERSTATEZWRITEENABLE,      (void *)&zWriteEnabled );

        _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND,          (void *)rwBLENDSRCALPHA );
        _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND,         (void *)rwBLENDONE );
        _rwOpenGLSetRenderState( rwRENDERSTATEZWRITEENABLE,      (void *)FALSE );

        if ( GL_FALSE != alphaTestEnabled )
        {
            RwOpenGLEnable( rwGL_ALPHA_TEST );
        }

        /* calculate the perturbed UVs for this mesh */
        openglMatFXCalcBumpUVs( objectLTM, instanceData, bumpMapData );

        /* can't use display lists since we're adjusting the texture coordinates */
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

            RWASSERT( GL_FALSE !=
                      _rwOpenGLExt.IsObjectBufferATI( openglBumpMapUVDataVAO ) );
            RwOpenGLVASetTexCoordATI( TRUE,
                                      2,
                                      GL_FLOAT,
                                      sizeof(RwTexCoords),
                                      openglBumpMapUVDataVAO,
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
                                   2,
                                   GL_FLOAT,
                                   sizeof(RwTexCoords),
                                   (RwUInt8 *)openglBumpMapUVData );

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

        envMapData = (const MatFXEnvMapData *)envMapDataVoid;
        if ( NULL != envMapData )
        {
            envMapTexture = envMapData->texture;

            envMapCoeff = (RwFastRealToUInt32(envMapData->coef * 255) & 0xFF);
        }
        else
        {
            envMapTexture = (RwTexture *)NULL;
        }

        if ( (NULL != envMapData) && (0 != envMapCoeff) )
        {
            openglMatFXEnvMapApply( objectLTM, instanceData, envMapData );

            RwOpenGLDisable( rwGL_LIGHTING );

            glColor4ub( 255, 255, 255, envMapCoeff );

            /* we can't call a display list here since we're modifying the texture coordinates */
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

                if ( openglMatFXApplyEnvMapSoft == openglMatFXEnvMapApply )
                {
                    RWASSERT( GL_FALSE !=
                              _rwOpenGLExt.IsObjectBufferATI( openglEnvMapUVDataVAO ) );

                    RwOpenGLVASetTexCoordATI( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))),
                                              2,
                                              GL_FLOAT,
                                              sizeof(RwTexCoords),
                                              openglEnvMapUVDataVAO,
                                              0 );
                }
                else
                {
                    /* tex coords have been texgen'd */
                    RwOpenGLVADisableTexCoord( 0 );
                }

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

                if ( openglMatFXApplyEnvMapSoft == openglMatFXEnvMapApply )
                {
                    RwOpenGLVASetTexCoord( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))),
                                           2,
                                           GL_FLOAT,
                                           sizeof(RwTexCoords),
                                           (RwUInt8 *)openglEnvMapUVData );
                }
                else
                {
                    /* tex coords have been texgen'd */
                    RwOpenGLVADisableTexCoord( 0 );
                }

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

            if ( openglMatFXApplyEnvMapSoft != openglMatFXEnvMapApply )
            {
                glPopMatrix();
                glMatrixMode( GL_MODELVIEW );

                RwOpenGLDisable( rwGL_TEXTURE0_GEN_S );
                RwOpenGLDisable( rwGL_TEXTURE0_GEN_T );
                RwOpenGLDisable( rwGL_TEXTURE0_GEN_R );
            }

            RwOpenGLEnable( rwGL_LIGHTING );
        }

        _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)vertexAlphaEnabled );
        _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND,          (void *)srcBlend );
        _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND,         (void *)destBlend );
        _rwOpenGLSetRenderState( rwRENDERSTATEZWRITEENABLE,      (void *)zWriteEnabled );
    }
    else
    {
        _rpMatFXOpenGLDefaultRender( instanceData, baseTexture, flags );
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglMatFXDualRenderMP
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglMatFXDualRenderMP( RxOpenGLMeshInstanceData *instanceData,
                         RwTexture *baseTexture,
                         const RwUInt32 flags,
                         const void *dualDataVoid,
                         const void *uvanimDataVoid )
{
    const MatFXDualData     *dualData;

    const MatFXUVAnimData   *uvanimData;

    RwTexture               *dualTexture;

    RwBool                  dualUVTransformIdentity;


    RWFUNCTION( RWSTRING( "openglMatFXDualRenderMP" ) );

    RWASSERT( NULL != instanceData );
    RWASSERT( NULL != dualDataVoid );

    dualData = (const MatFXDualData *)dualDataVoid;

    dualUVTransformIdentity = TRUE;
    if ( NULL != uvanimDataVoid )
    {
        uvanimData = (const MatFXUVAnimData *)uvanimDataVoid;

        if ( (NULL != uvanimData->baseTransform) &&
             (FALSE == rwMatrixTestFlags(uvanimData->baseTransform,
                                         rwMATRIXINTERNALIDENTITY)) )
        {
            glMatrixMode( GL_TEXTURE );
            glPushMatrix();

            openglApplyUVTransform( uvanimData->baseTransform );

            _rpMatFXOpenGLDefaultRender( instanceData, baseTexture, flags );

            glPopMatrix();
            glMatrixMode( GL_MODELVIEW );
        }
        else
        {
            _rpMatFXOpenGLDefaultRender( instanceData, baseTexture, flags );
        }
    }
    else
    {
        uvanimData = (const MatFXUVAnimData *)NULL;
        _rpMatFXOpenGLDefaultRender( instanceData, baseTexture, flags );
    }

    dualTexture = dualData->texture;
    if ( NULL != dualTexture )
    {
        RwBool          vertexAlphaEnabled;

        RwBlendFunction srcBlend;

        RwBlendFunction destBlend;

        RwBool          zWriteEnabled;


        _rwOpenGLGetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)&vertexAlphaEnabled );
        _rwOpenGLGetRenderState( rwRENDERSTATESRCBLEND, (void *)&srcBlend );
        _rwOpenGLGetRenderState( rwRENDERSTATEDESTBLEND, (void *)&destBlend );
        _rwOpenGLGetRenderState( rwRENDERSTATEZWRITEENABLE, (void *)&zWriteEnabled );

        _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE );
        _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND, (void *)(dualData->srcBlendMode) );
        _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND, (void *)(dualData->dstBlendMode) );
        _rwOpenGLSetRenderState( rwRENDERSTATEZWRITEENABLE, (void *)FALSE );

        /* bind or unbind dual texture */
        if ( 0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2)) )
        {
            RwOpenGLSetTexture( dualTexture );
        }
        else
        {
            RwOpenGLSetTexture( NULL );
        }

        if ( NULL != uvanimData )
        {
            if ( NULL != uvanimData->dualTransform )
            {
                dualUVTransformIdentity = rwMatrixTestFlags(uvanimData->dualTransform,
                                                            rwMATRIXINTERNALIDENTITY);
                if ( FALSE == dualUVTransformIdentity )
                {
                    glMatrixMode( GL_TEXTURE );
                    glPushMatrix();

                    openglApplyUVTransform( uvanimData->dualTransform );
                }
            }
        }

        /* can't call the display list since we're changing the texture coordinates */
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

            /* if a second set of texture coordinates exist, use them
             * otherwise, fall back to reusing the first set */
            if ( 0 != (flags & rpGEOMETRYTEXTURED2) )
            {
                RwOpenGLVASetTexCoordATI( TRUE,
                                          2,
                                          GL_FLOAT,
                                          instanceData->vertexStride,
                                          instanceData->vaoName,
                                          instanceData->texCoord[1] );
            }
            else
            {
                RwOpenGLVASetTexCoordATI( TRUE,
                                          2,
                                          GL_FLOAT,
                                          instanceData->vertexStride,
                                          instanceData->vaoName,
                                          instanceData->texCoord[0] );
            }

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

            /* if a second set of texture coordinates exist, use them
             * otherwise, fall back to reusing the first set */
            if ( 0 != (flags & rpGEOMETRYTEXTURED2) )
            {
                RwOpenGLVASetTexCoord( TRUE,
                                       2,
                                       GL_FLOAT,
                                       instanceData->vertexStride,
                                       instanceData->texCoord[1] );
            }
            else
            {
                RwOpenGLVASetTexCoord( TRUE,
                                       2,
                                       GL_FLOAT,
                                       instanceData->vertexStride,
                                       instanceData->texCoord[0] );
            }

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

        if ( (NULL != uvanimData) &&
             (FALSE == dualUVTransformIdentity) )
        {
            glPopMatrix();
            glMatrixMode( GL_MODELVIEW );
        }

        _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)vertexAlphaEnabled );
        _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND, (void *)srcBlend );
        _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND, (void *)destBlend );
        _rwOpenGLSetRenderState( rwRENDERSTATEZWRITEENABLE, (void *)zWriteEnabled );
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglUVAnimRender
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglUVAnimRender( RxOpenGLMeshInstanceData *instanceData,
                    RwTexture *baseTexture,
                    const RwUInt32 flags,
                    const void *uvanimDataVoid )
{
    const MatFXUVAnimData *uvanimData;


    RWFUNCTION( RWSTRING( "openglUVAnimRender" ) );

    RWASSERT( NULL != instanceData );
    RWASSERT( NULL != uvanimDataVoid );

    uvanimData = (const MatFXUVAnimData *)uvanimDataVoid;

    if ( (NULL != uvanimData->baseTransform) &&
         (FALSE == rwMatrixTestFlags(uvanimData->baseTransform,
                                     rwMATRIXINTERNALIDENTITY)) )
    {
        glMatrixMode( GL_TEXTURE );
        glPushMatrix();

        openglApplyUVTransform( uvanimData->baseTransform );

        _rpMatFXOpenGLDefaultRender( instanceData, baseTexture, flags );

        glPopMatrix();
        glMatrixMode( GL_MODELVIEW );
    }
    else
    {
        _rpMatFXOpenGLDefaultRender( instanceData, baseTexture, flags );
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglApplyUVTransform
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglApplyUVTransform( RwMatrix *transform )
{
    RWFUNCTION( RWSTRING( "openglApplyUVTransform" ) );

    RWASSERT( NULL != transform );

    openglTextureTransform[0]  = transform->right.x;
    openglTextureTransform[1]  = transform->right.y;

    openglTextureTransform[4]  = transform->up.x;
    openglTextureTransform[5]  = transform->up.y;

    openglTextureTransform[12] = transform->pos.x;
    openglTextureTransform[13] = transform->pos.y;

    glMultMatrixf( openglTextureTransform );

    RWRETURNVOID();
}
