/***********************************************************************
 *
 * Module:  teamshadowopengl.c
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
#include "team.h"
#include "teampipes.h"
#include "teamshadow.h"
#include "teamplayer.h"

#include "teamopengl.h"
#include "teamskinopengl.h"
#include "teamshadowopengl.h"


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
static TeamPipeIndex    openglTeamStaticShadowPipeIdx = (TeamPipeIndex)TEAMPIPENULL;

static TeamPipeIndex    openglTeamSkinShadowPipeIdx = (TeamPipeIndex)TEAMPIPENULL;


/* =====================================================================
 *  Global variables
 * ===================================================================== */
extern RwBool
_rwOpenGLSetRenderState( RwRenderState state, void *param );


/* =====================================================================
 *  Static function prototypes
 * ===================================================================== */
static RxPipeline *
openglTeamCreateStaticShadowPipeline( void );

static void
openglTeamStaticShadowRenderCB( RwResEntry *repEntry,
                                void *object,
                                const RwUInt8 type,
                                const RwUInt32 flags );


/* =====================================================================
 *  Global function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: _rpTeamShadowPipelineCreate
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwBool
_rpTeamShadowPipelineCreate( void )
{
    RxPipeline  *pipe;


    RWFUNCTION( RWSTRING( "_rpTeamShadowPipelineCreate" ) );

    pipe = openglTeamCreateStaticShadowPipeline();
    if ( NULL != pipe )
    {
        openglTeamStaticShadowPipeIdx = _rpTeamPipeAddPipeDefinition( pipe,
                                                                      (TeamPipeOpen)NULL,
                                                                      (TeamPipeClose)NULL );

        _rpTeamOpenGLSetRenderPipeline( pipe, TEAMOPENGLPIPEID_STATIC_SHADOW );
    }

    pipe = _rpSkinOpenGLPipelineCreate( rpSKINTYPEGENERIC, _rpTeamOpenGLSkinAllInOneRenderCB );
    if ( NULL != pipe )
    {
        RxNodeDefinition    *allinoneNodeDef;

        RxPipelineNode      *pipeNode;


        allinoneNodeDef = RxNodeDefinitionGetOpenGLAtomicAllInOne();
        RWASSERT( NULL != allinoneNodeDef );

        pipeNode = RxPipelineFindNodeByName( pipe, allinoneNodeDef->name, NULL, NULL );
        RWASSERT( NULL != pipeNode );

        /* overload the skin reinstance callback */
        if ( NULL == _rpTeamOpenGLSkinDefaultReinstanceCB )
        {
            _rpTeamOpenGLSkinDefaultReinstanceCB = RxOpenGLAllInOneGetReinstanceCallBack( pipeNode );
        }
        RxOpenGLAllInOneSetReinstanceCallBack( pipeNode, _rpTeamOpenGLSkinAllInOneAtomicReinstanceCB );

        /* shadows aren't lit */
        RxOpenGLAllInOneSetLightingCallBack(pipeNode,(RxOpenGLAllInOneLightingCallBack)NULL);

        /* replace the render callback */
        RxOpenGLAllInOneSetRenderCallBack( pipeNode, _rpTeamOpenGLSkinShadowRenderCB );

        openglTeamSkinShadowPipeIdx = _rpTeamPipeAddPipeDefinition( pipe,
                                                                    (TeamPipeOpen)NULL,
                                                                    (TeamPipeClose)NULL );

        _rpTeamOpenGLSetRenderPipeline( pipe, TEAMOPENGLPIPEID_SKINNED_SHADOW );
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rpTeamShadowPipelineDestroy
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
void
_rpTeamShadowPipelineDestroy( void )
{
    RWFUNCTION( RWSTRING( "_rpTeamShadowPipelineDestroy" ) );

    if ( TEAMPIPENULL != openglTeamStaticShadowPipeIdx )
    {
        _rpTeamPipeRemovePipeDefinition( openglTeamStaticShadowPipeIdx );
        openglTeamStaticShadowPipeIdx = (TeamPipeIndex)TEAMPIPENULL;

        _rpTeamOpenGLSetRenderPipeline( NULL, TEAMOPENGLPIPEID_STATIC_SHADOW );
    }

    if ( TEAMPIPENULL != openglTeamSkinShadowPipeIdx )
    {
        _rpTeamPipeRemovePipeDefinition( openglTeamSkinShadowPipeIdx );
        openglTeamSkinShadowPipeIdx = (TeamPipeIndex)TEAMPIPENULL;

        _rpTeamOpenGLSetRenderPipeline( NULL, TEAMOPENGLPIPEID_SKINNED_SHADOW );
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rpTeamShadowGetPipeline
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RxPipeline *
_rpTeamShadowGetPipeline( RpTeamElementType type,
                          RwBool __RWUNUSED__ allInOne )
{
    RxPipeline  *pipe;


    RWFUNCTION( RWSTRING( "_rpTeamShadowGetPipeline" ) );

    pipe = (RxPipeline *)NULL;
    if ( rpTEAMELEMENTSTATIC == type )
    {
        pipe = _rpTeamPipeGetPipeline( openglTeamStaticShadowPipeIdx );
    }
    else if ( rpTEAMELEMENTSKINNED == type )
    {
        pipe = _rpTeamPipeGetPipeline( openglTeamSkinShadowPipeIdx );
    }

    RWRETURN( pipe );
}


/* ---------------------------------------------------------------------
 *  Function: _rpTeamOpenGLSkinShadowRenderCB
 *
 *  Purpose :
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
void
_rpTeamOpenGLSkinShadowRenderCB( RwResEntry *repEntry,
                                 void *object __RWUNUSEDRELEASE__,
                                 const RwUInt8 type __RWUNUSED__,
                                 const RwUInt32 flags )
{
    const RpTeamShadowRenderData    *shadowData;

    RwUInt32                        numShadows;

    RxOpenGLResEntryHeader          *resEntryHeader;

    RxOpenGLMeshInstanceData        *instanceData;

    const RpTeamPlayer              *player;

    RwMatrix                        shadowMatrix;

    RwV3d                           *shadowRight;

    RwV3d                           *shadowUp;

    RwV3d                           *shadowAt;

    RwV3d                           *shadowPos;

    const RwV3d                     *projPlaneNormal;

    RwReal                          projPlaneD;

    RwUInt32                        idx;


    RWFUNCTION( RWSTRING( "_rpTeamOpenGLSkinShadowRenderCB" ) );

    RWASSERT( NULL != repEntry );
    RWASSERT( NULL != object );

    shadowData = (const RpTeamShadowRenderData *)_rpTeamShadowGetCurrentShadowData();
    RWASSERT( NULL != shadowData );

    numShadows = shadowData->renderNumShadows;
    if ( 0 == numShadows )
    {
        RWRETURNVOID();
    }

    /* get the res entry header */
    resEntryHeader = (RxOpenGLResEntryHeader *)(repEntry + 1);
    RWASSERT( NULL != resEntryHeader );

    RwOpenGLDisable( rwGL_LIGHTING );
    RwOpenGLDisable( rwGL_COLOR_MATERIAL );
    RwOpenGLSetTexture( NULL );
    _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE );

    /* enable the stencil buffer */
    _rwOpenGLSetRenderState( rwRENDERSTATESTENCILENABLE,      (void *)TRUE );
    _rwOpenGLSetRenderState( rwRENDERSTATESTENCILPASS,        (void *)rwSTENCILOPERATIONREPLACE );
    _rwOpenGLSetRenderState( rwRENDERSTATESTENCILFUNCTION,    (void *)rwSTENCILFUNCTIONNOTEQUAL );
    _rwOpenGLSetRenderState( rwRENDERSTATESTENCILFUNCTIONREF, (void *)1 );

    /* get some data for the shadows */
    player = _rpTeamPlayerGetCurrentPlayer();
    RWASSERT( NULL != player );

    shadowRight = RwMatrixGetRight( &shadowMatrix );
    RWASSERT( NULL != shadowRight );

    shadowUp = RwMatrixGetUp( &shadowMatrix );
    RWASSERT( NULL != shadowUp );

    shadowAt = RwMatrixGetAt( &shadowMatrix );
    RWASSERT( NULL != shadowAt );

    shadowPos = RwMatrixGetPos( &shadowMatrix );
    RWASSERT( NULL != shadowPos );

    projPlaneNormal = &(shadowData->localPlaneNormal.v3d.v);

    projPlaneD = RwV3dDotProduct( projPlaneNormal,
                                     &(shadowData->localPlanePos.v3d.v) );

    for ( idx = 0; idx < numShadows; idx += 1 )
    {
        const RwV3d *projShad;

        RwUInt16    numMeshes;


        /* calculate the shadow transform matrix */
        projShad = &(shadowData->localProjection[idx].v3d.v);

        shadowRight->x = 1.0f - projPlaneNormal->x * projShad->x;
        shadowRight->y =      - projPlaneNormal->x * projShad->y;
        shadowRight->z =      - projPlaneNormal->x * projShad->z;
        shadowUp->x    =      - projPlaneNormal->y * projShad->x;
        shadowUp->y    = 1.0f - projPlaneNormal->y * projShad->y;
        shadowUp->z    =      - projPlaneNormal->y * projShad->z;
        shadowAt->x    =      - projPlaneNormal->z * projShad->x;
        shadowAt->y    =      - projPlaneNormal->z * projShad->y;
        shadowAt->z    = 1.0f - projPlaneNormal->z * projShad->z;
        shadowPos->x   = projPlaneD * projShad->x;
        shadowPos->y   = projPlaneD * projShad->y;
        shadowPos->z   = projPlaneD * projShad->z;

        RwMatrixUpdate( &shadowMatrix );

        glPushMatrix();
        _rwOpenGLApplyRwMatrix( &shadowMatrix );

        glColor4f( 0.0, 0.0, 0.0, shadowData->renderValues[idx] );

        /* get the instanced data */
        instanceData = (RxOpenGLMeshInstanceData *)(resEntryHeader + 1);
        RWASSERT( NULL != instanceData );

        numMeshes = resEntryHeader->numMeshes;
        while ( numMeshes-- )
        {
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

            /* next RxOpenGLMeshInstanceData pointer */
            instanceData += 1;
        }

        glPopMatrix();
    }

    /* disable the stencil buffer */
    _rwOpenGLSetRenderState( rwRENDERSTATESTENCILENABLE, FALSE );

    RWRETURNVOID();
}



/* =====================================================================
 *  Static function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: openglTeamCreateStaticShadowPipeline
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static RxPipeline *
openglTeamCreateStaticShadowPipeline( void )
{
    RxPipeline  *pipe;


    RWFUNCTION( RWSTRING( "openglTeamCreateStaticShadowPipeline" ) );

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
                RxPipelineNode      *pipeNode;


                pipeNode = RxPipelineFindNodeByName( pipe, allinoneNodeDef->name, NULL, NULL );
                RWASSERT( NULL != pipeNode );

                RxOpenGLAllInOneSetLightingCallBack( pipeNode, NULL );

                RxOpenGLAllInOneSetRenderCallBack( pipeNode, openglTeamStaticShadowRenderCB );

                RWRETURN( pipe );
            }
        }

        RxPipelineDestroy( pipe );
        pipe = (RxPipeline *)NULL;
    }

    RWRETURN( pipe );
}


/* ---------------------------------------------------------------------
 *  Function: openglTeamStaticShadowRenderCB
 *
 *  Purpose :
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
void
openglTeamStaticShadowRenderCB( RwResEntry *repEntry,
                                void *object,
                                const RwUInt8 type,
                                const RwUInt32 flags )
{
    RpTeamShadowRenderData  *shadowData;

    RwUInt32                numShadows;

    const RpAtomic          *atomic;

    RwFrame                 *atomicFrame;

    const RwMatrix          *atomicLTM;

    RwMatrix                invAtomicLTM;

    RwV3d                   savedLocalNormal;

    RwV3d                   savedLocalPosition;

    RwV3d                   savedLocalProj[rpTEAMMAXNUMSHADOWS];

    RwUInt32                idx;

    RwMatrix                matrixOtho;

    RwMatrix                invOrtho;

    RwMatrix                *inverseLTM;


    RWFUNCTION( RWSTRING( "openglTeamStaticShadowRenderCB" ) );

    RWASSERT( NULL != repEntry );
    RWASSERT( NULL != object );

    shadowData = (RpTeamShadowRenderData *)_rpTeamShadowGetCurrentShadowData();
    RWASSERT( NULL != shadowData );

    numShadows = shadowData->renderNumShadows;
    if ( 0 == numShadows )
    {
        RWRETURNVOID();
    }

    /* The Team plugin doesn't compute shadows correctly for the static case.
     * Reproject from world to local space, write it into the RpTeamShadowRenderData
     * struct for rendering (so we can share the skinning render callback) and then
     * restore the old local projection data in case it is used elsewhere.
     */
    atomic = (const RpAtomic *)object;

    atomicFrame = RpAtomicGetFrame( atomic );
    RWASSERT( NULL != atomicFrame );

    atomicLTM = TeamFrameGetLTM( atomicFrame );
    RWASSERT( NULL != atomicLTM );

    TeamMatrixInvert( &invAtomicLTM, atomicLTM );

    /* if the LTM is not orthonormal... */
    if ( 0 == ( rwMATRIXTYPEORTHONORMAL & rwMatrixGetFlags(atomicLTM) ) )
    {
        RwMatrixOrthoNormalize(&matrixOtho, atomicLTM);

        TeamMatrixInvert(&invOrtho, &matrixOtho);

        inverseLTM = &invOrtho;
    }
    else
    {
        inverseLTM = &invAtomicLTM;
    }

    /* local normals... */
    savedLocalNormal = shadowData->localPlaneNormal.v3d.v;

    TeamV3dTransformVectors( &(shadowData->localPlaneNormal.v3d.v),
                             &(shadowData->worldPlaneNormal.v3d.v),
                             &invAtomicLTM );

    TeamV3dNormalize( &(shadowData->localPlaneNormal.v3d.v), &(shadowData->localPlaneNormal.v3d.v) );

    /* local positions... */
    savedLocalPosition = shadowData->localPlanePos.v3d.v;

    TeamV3dTransformPoints( &(shadowData->localPlanePos.v3d.v),
                            &(shadowData->worldPlanePos.v3d.v),
                            &invAtomicLTM );

    /* local projections... */
    for ( idx = 0; idx < shadowData->renderNumShadows; idx += 1 )
    {
        savedLocalProj[idx] = shadowData->localProjection[idx].v3d.v;

        TeamV3dTransformVectors( &(shadowData->localProjection[idx].v3d.v),
                                 &(shadowData->worldProjection[idx].v3d.v),
                                 inverseLTM );
    }

    /* static projection complete... now we can use the skinning shadow callback */
     _rpTeamOpenGLSkinShadowRenderCB( repEntry, object, type, flags );

    /* restore the saved local projection data */
    shadowData->localPlaneNormal.v3d.v = savedLocalNormal;
    shadowData->localPlanePos.v3d.v = savedLocalPosition;
    for ( idx = 0; idx < shadowData->renderNumShadows; idx += 1 )
    {
        shadowData->localProjection[idx].v3d.v = savedLocalProj[idx];
    }

    RWRETURNVOID();
}
