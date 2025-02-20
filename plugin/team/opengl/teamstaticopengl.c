/***********************************************************************
 *
 * Module:  teamstaticopengl.c
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

#include "rpteam.h"
#include "teampipes.h"
#include "teamstatic.h"
#include "teamplayer.h"

#include "teamopengl.h"
#include "teamstaticopengl.h"
#include "teamrenderopengl.h"


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


/* =====================================================================
 *  Static variables
 * ===================================================================== */
static TeamPipeIndex    openglTeamStaticPipeIdx = (TeamPipeIndex)TEAMPIPENULL;


/* =====================================================================
 *  Global variables
 * ===================================================================== */
extern RwBool
_rwOpenGLSetRenderState( RwRenderState state, void *param );


/* =====================================================================
 *  Static function prototypes
 * ===================================================================== */
static RxPipeline *
openglTeamCreateStaticPipeline( void );

static void
openglTeamAllInOneStaticRenderCB( RwResEntry *repEntry,
                                  void *object __RWUNUSED__,
                                  const RwUInt8 type __RWUNUSED__,
                                  const RwUInt32 flags );


/* =====================================================================
 *  Global function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: _rpTeamStaticCustomPipelineCreate
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwBool
_rpTeamStaticCustomPipelineCreate( void )
{
    RxPipeline  *pipe;


    RWFUNCTION( RWSTRING( "_rpTeamStaticCustomPipelineCreate" ) );

    pipe = openglTeamCreateStaticPipeline();
    if ( NULL != pipe )
    {
        openglTeamStaticPipeIdx = _rpTeamPipeAddPipeDefinition( pipe,
                                                                (TeamPipeOpen)NULL,
                                                                (TeamPipeClose)NULL );

        _rpTeamOpenGLSetRenderPipeline( pipe, TEAMOPENGLPIPEID_STATIC );
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rpTeamStaticCustomPipelineDestroy
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
void
_rpTeamStaticCustomPipelineDestroy( void )
{
    RWFUNCTION( RWSTRING( "_rpTeamStaticCustomPipelineDestroy" ) );

    if ( TEAMPIPENULL != openglTeamStaticPipeIdx )
    {
        _rpTeamPipeRemovePipeDefinition( openglTeamStaticPipeIdx );
        openglTeamStaticPipeIdx = (TeamPipeIndex)TEAMPIPENULL;

        _rpTeamOpenGLSetRenderPipeline( NULL, TEAMOPENGLPIPEID_STATIC );
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rpTeamStaticGetCustomPipeline
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RxPipeline *
_rpTeamStaticGetCustomPipeline( void )
{
    RWFUNCTION( RWSTRING( "_rpTeamStaticGetCustomPipeline" ) );

    RWRETURN( _rpTeamPipeGetPipeline( openglTeamStaticPipeIdx )  );
}


/* =====================================================================
 *  Static function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: openglTeamCreateStaticPipeline
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static RxPipeline *
openglTeamCreateStaticPipeline( void )
{
    RxPipeline  *pipe;


    RWFUNCTION( RWSTRING( "openglTeamCreateStaticPipeline" ) );

    pipe = RxPipelineCreate();
    if ( NULL != pipe )
    {
        RxLockedPipe    *lockedPipe;


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

            RWASSERT( lockedPipe == (RxLockedPipe *)pipe );

            if ( NULL != lockedPipe )
            {
                RxPipelineNode  *pipeNode;


                pipeNode = RxPipelineFindNodeByName( pipe,
                                                     allinoneNodeDef->name,
                                                     NULL,
                                                     NULL );
                if ( NULL != pipeNode )
                {
                    /* we want Team to handle all the lighting */
                    RxOpenGLAllInOneSetLightingCallBack(pipeNode,_rwOpenGLTeamLightingCB);

                    RxOpenGLAllInOneSetRenderCallBack( pipeNode, openglTeamAllInOneStaticRenderCB );

                    RWRETURN( pipe );
                }
            }
        }

        RxPipelineDestroy( pipe );
        pipe = (RxPipeline *)NULL;
    }

    RWRETURN( pipe );
}


/* ---------------------------------------------------------------------
 *  Function: openglTeamAllInOneStaticRenderCB
 *
 *  Purpose : Team all-in-one static geometry render callback
 *
 *  On entry: repEntry - Pointer to RwResEntry.
 *
 *            object - Void pointer to the object.
 *
 *            type - Constant RwUInt32 containing rpATOMIC or rwSECTORATOMIC.
 *
 *            flags - Constant RwUInt32 containing RpGeometryFlag's or RpWorldFlag's.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
static void
openglTeamAllInOneStaticRenderCB( RwResEntry *repEntry,
                                  void *object __RWUNUSED__,
                                  const RwUInt8 type __RWUNUSED__,
                                  const RwUInt32 flags )
{
    RxOpenGLResEntryHeader      *resEntryHeader;

    RxOpenGLMeshInstanceData    *instanceData;

    RwUInt16                    numMeshes;


    RWFUNCTION( RWSTRING( "openglTeamAllInOneStaticRenderCB" ) );

    /* get the instanced data */
    resEntryHeader = (RxOpenGLResEntryHeader *)(repEntry + 1);
    RWASSERT( NULL != resEntryHeader );

    instanceData = (RxOpenGLMeshInstanceData *)(resEntryHeader + 1);
    RWASSERT( NULL != instanceData );

    numMeshes = resEntryHeader->numMeshes;
    while ( numMeshes-- )
    {
        RwRGBA          savedMatColor;

        RwRGBA          playerMatColor;

        const RwRGBA    *matColor;

        RwBool          vertexAlphaEnabled;


        RWASSERT( NULL != instanceData->material );

        savedMatColor = *(RpMaterialGetColor( instanceData->material ));

        playerMatColor = *RpTeamMaterialGetPlayerColor( instanceData->material,
                                                        _rpTeamPlayerGetCurrentPlayer());

        matColor = &playerMatColor;
        RWASSERT( NULL != matColor );

        (void)RpMaterialSetColor( instanceData->material, matColor );

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

            RwOpenGLVASetTexCoordATI( (0 != (flags & (rxGEOMETRY_TEXTURED |
                                                      rpGEOMETRYTEXTURED2))),
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

        (void)RpMaterialSetColor( instanceData->material, &savedMatColor );

        /* next RxOpenGLMeshInstanceData pointer */
        instanceData += 1;
    }

    RWRETURNVOID();
}
