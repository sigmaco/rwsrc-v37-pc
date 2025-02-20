/***********************************************************************
 *
 * Module:  teamskinopengl.c
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

#include "teamskinopengl.h"
#include "teamshadowopengl.h"
#include "teamrenderopengl.h"

#include "../skin2/skin.h"


/* =====================================================================
 *  Defines
 * ===================================================================== */


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */


/* =====================================================================
 *  Static variables
 * ===================================================================== */
static TeamPipeIndex                        openglTeamSkinPipeIdx = \
            (TeamPipeIndex)TEAMPIPENULL;


/* =====================================================================
 *  Global variables
 * ===================================================================== */
RxOpenGLAllInOneReinstanceCallBack   _rpTeamOpenGLSkinDefaultReinstanceCB = \
            (RxOpenGLAllInOneReinstanceCallBack)NULL;

extern RwBool
_rwOpenGLSetRenderState( RwRenderState state, void *param );


/* =====================================================================
 *  Static function prototypes
 * ===================================================================== */


/* =====================================================================
 *  Global function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: _rpTeamSkinCustomPipelineCreate
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwBool
_rpTeamSkinCustomPipelineCreate( void )
{
    RxPipeline  *pipe;


    RWFUNCTION( RWSTRING( "_rpTeamSkinCustomPipelineCreate" ) );

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

        /* we want Team to handle all the lighting */
        RxOpenGLAllInOneSetLightingCallBack(pipeNode,_rwOpenGLTeamLightingCB);

        openglTeamSkinPipeIdx = _rpTeamPipeAddPipeDefinition( pipe,
                                                              (TeamPipeOpen)NULL,
                                                              (TeamPipeClose)NULL );

        _rpTeamOpenGLSetRenderPipeline( pipe, TEAMOPENGLPIPEID_SKINNED );
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rpTeamSkinCustomPipelineDestroy
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
void
_rpTeamSkinCustomPipelineDestroy( void )
{
    RWFUNCTION( RWSTRING( "_rpTeamSkinCustomPipelineDestroy" ) );

    if ( TEAMPIPENULL != openglTeamSkinPipeIdx )
    {
        _rpTeamPipeRemovePipeDefinition( openglTeamSkinPipeIdx );
        openglTeamSkinPipeIdx = (TeamPipeIndex)TEAMPIPENULL;

        _rpTeamOpenGLSetRenderPipeline( NULL, TEAMOPENGLPIPEID_SKINNED );
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rpTeamSkinGetCustomPipeline
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RxPipeline *
_rpTeamSkinGetCustomPipeline( void )
{
    RWFUNCTION( RWSTRING( "_rpTeamSkinGetCustomPipeline" ) );

    RWRETURN( _rpTeamPipeGetPipeline( openglTeamSkinPipeIdx ) );
}


/* ---------------------------------------------------------------------
 *  Function: _rpTeamSkinMatBlendUpdating
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwMatrix *
_rpTeamSkinMatBlendUpdating( RpSkin *skin,
                             RwFrame *frame,
                             RpHAnimHierarchy *hierarchy,
                             RwMatrix *matrixArray )
{
    RWFUNCTION(RWSTRING("_rpTeamSkinMatBlendUpdating"));

    if ( NULL != hierarchy )
    {
        RwInt32         i;

        const RwMatrix  *skinToBone;

        matrixArray = _rpSkinGlobals.matrixCache.aligned;

        /* get the bone information */
        skinToBone = RpSkinGetSkinToBoneMatrices(skin);
        RWASSERT( NULL != skinToBone );

        if ( 0 != (hierarchy->flags & rpHANIMHIERARCHYNOMATRICES) )
        {
            RwMatrix *ltm;

            RwMatrix inverseAtomicLTM;

            RwMatrix temmatrix;

            RwInt32 i;


            ltm = RwFrameGetLTM(frame);
            RWASSERT(NULL != ltm);

            RwMatrixInvert(&inverseAtomicLTM, ltm);

            for ( i = 0; i < hierarchy->numNodes; i++ )
            {
                RwFrame *node;
                RwMatrix *ltm;

                node = hierarchy->pNodeInfo[i].pFrame;
                RWASSERT(NULL != node);
                ltm = RwFrameGetLTM(node);
                RWASSERT(NULL != ltm);

                RwMatrixMultiply( &temmatrix,
                                  &skinToBone[i],
                                  ltm );
                RwMatrixMultiply( &matrixArray[i],
                                  &temmatrix,
                                  &inverseAtomicLTM );
            }
        }
        else if ( 0 != (hierarchy->flags & rpHANIMHIERARCHYLOCALSPACEMATRICES) )
        {
            for ( i = 0; i < hierarchy->numNodes; i++ )
            {
                RwMatrixMultiply( &matrixArray[i],
                                  &skinToBone[i],
                                  &(hierarchy->pMatrixArray[i]) );
            }
        }
        else
        {
            RwMatrix inverseAtomicLTM;

            RwMatrix temmatrix;


            RwMatrixInvert(&inverseAtomicLTM,
                           RwFrameGetLTM(frame));

            for ( i = 0; i < hierarchy->numNodes; i++ )
            {
                RwMatrixMultiply( &temmatrix,
                                  &skinToBone[i],
                                  &(hierarchy->pMatrixArray[i]) );
                RwMatrixMultiply( &matrixArray[i],
                                  &temmatrix,
                                  &inverseAtomicLTM );
            }
        }
    }

    RWRETURN( matrixArray );
}


/* ---------------------------------------------------------------------
 *  Function: _rpTeamOpenGLSkinAllInOneAtomicReinstanceCB
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwBool
_rpTeamOpenGLSkinAllInOneAtomicReinstanceCB( void *object,
                                          RwResEntry *resEntry,
                                          const RpMeshHeader *meshHeader,
                                          const RwBool instanceDLandVA,
                                          RxOpenGLAllInOneInstanceCallBack instanceCB )
{
    RpAtomic            *atomic;

    RwMatrix            *savedSkinMatrixArray;

    RpHAnimHierarchy    *hierarchy;

    RwBool              returnVal;


    RWFUNCTION( RWSTRING( "_rpTeamOpenGLSkinAllInOneAtomicReinstanceCB" ) );

    RWASSERT( NULL != object );
    RWASSERT( NULL != resEntry );
    RWASSERT( NULL != meshHeader );

    atomic = (RpAtomic *)object;

    /* save the global matrix cache */
    savedSkinMatrixArray = _rpSkinGlobals.matrixCache.aligned;

    hierarchy = RpHAnimFrameGetHierarchy( _rpTeamPlayerGetCurrentPlayer()->frame );
    RPSKINATOMICGETDATA( atomic )->hierarchy = hierarchy;

    /* force to use the already calculated matrix array */
    _rpSkinGlobals.matrixCache.aligned = _rpTeamSkinGetCurrentMatrixCache();

    /* normal skinning re-instance function */
    returnVal = _rpTeamOpenGLSkinDefaultReinstanceCB( object, resEntry, meshHeader, instanceDLandVA, instanceCB );

    /* restore the saved global matrix cache */
    _rpSkinGlobals.matrixCache.aligned = savedSkinMatrixArray;

    RWRETURN( returnVal );
}


/* ---------------------------------------------------------------------
 *  Function: _rpTeamOpenGLSkinAllInOneRenderCB
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
void
_rpTeamOpenGLSkinAllInOneRenderCB( RwResEntry *repEntry,
                                   void *object,
                                   const RwUInt8 type,
                                   const RwUInt32 flags )
{
    RxOpenGLResEntryHeader      *resEntryHeader;

    RxOpenGLMeshInstanceData    *instanceData;

    RwBool                      lightingEnabled;

    RwUInt16                    numMeshes;


    RWFUNCTION( RWSTRING( "_rpTeamOpenGLSkinAllInOneRenderCB" ) );

    /* get the instanced data */
    resEntryHeader = (RxOpenGLResEntryHeader *)(repEntry + 1);
    RWASSERT( NULL != resEntryHeader );

    instanceData = (RxOpenGLMeshInstanceData *)(resEntryHeader + 1);
    RWASSERT( NULL != instanceData );

    lightingEnabled = RwOpenGLIsEnabled( rwGL_LIGHTING );

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
                                                        _rpTeamPlayerGetCurrentPlayer() );

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
            RwTexture   *playerTexture;


            playerTexture = RpTeamMaterialGetPlayerTexture( instanceData->material,
                                                            _rpTeamPlayerGetCurrentPlayer() );

            RwOpenGLSetTexture( playerTexture );
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

        (void)RpMaterialSetColor( instanceData->material, &savedMatColor );

        /* next RxOpenGLMeshInstanceData pointer */
        instanceData += 1;
    }

#if defined(TEAMSHADOWALLINONE)
    _rpTeamOpenGLSkinShadowRenderCB( repEntry, object, type, flags );
#endif /* defined(TEAMSHADOWALLINONE) */

    RWRETURNVOID();
}



/* =====================================================================
 *  Static function definitions
 * ===================================================================== */

