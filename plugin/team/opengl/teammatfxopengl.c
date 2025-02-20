/***********************************************************************
 *
 * Module:  teammatfxopengl.c
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
#include "teamplayer.h"
#include "teammatfx.h"

#include "teamopengl.h"
#include "teamskinopengl.h"
#include "teammatfxopengl.h"
#include "teamshadowopengl.h"
#include "teamrenderopengl.h"

#include "../matfx/effectPipes.h"


/* =====================================================================
 *  Defines
 * ===================================================================== */


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */


/* =====================================================================
 *  Static variables
 * ===================================================================== */
static TeamPipeIndex    openglTeamMatFXPipeIdx = (TeamPipeIndex)TEAMPIPENULL;

static TeamPipeIndex    openglTeamSkinMatFXPipeIdx = (TeamPipeIndex)TEAMPIPENULL;


/* =====================================================================
 *  Global variables
 * ===================================================================== */
RxOpenGLAllInOneRenderCallBack  openglTeamMatFXDefaultRenderCB = \
        (RxOpenGLAllInOneRenderCallBack)NULL;

extern RwBool
_rwOpenGLSetRenderState( RwRenderState state, void *param );


/* =====================================================================
 *  Static function prototypes
 * ===================================================================== */


/* =====================================================================
 *  Global function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: _rpTeamMatFXPipelineCreate
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwBool
_rpTeamMatFXPipelineCreate( void )
{
    RxPipeline  *pipe;


    RWFUNCTION( RWSTRING( "_rpTeamMatFXPipelineCreate" ) );

    pipe = _rpMatFXOpenGLAtomicPipelineCreate();
    if ( NULL != pipe )
    {
        openglTeamMatFXPipeIdx = _rpTeamPipeAddPipeDefinition( pipe,
                                                               (TeamPipeOpen)NULL,
                                                               (TeamPipeClose)NULL );

        _rpTeamOpenGLSetRenderPipeline( pipe, TEAMOPENGLPIPEID_STATIC_MATFX );
    }

    pipe = _rpSkinOpenGLPipelineCreate( rpSKINTYPEMATFX, _rpMatFXOpenGLAllInOneRenderCB );
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

        /* overload the matfx render callback */
        if ( NULL == openglTeamMatFXDefaultRenderCB )
        {
            openglTeamMatFXDefaultRenderCB = RxOpenGLAllInOneGetRenderCallBack( pipeNode );
        }
        RxOpenGLAllInOneSetRenderCallBack( pipeNode, _rpTeamOpenGLMatFXRenderCB );

        /* we want Team to handle all the lighting */
        RxOpenGLAllInOneSetLightingCallBack(pipeNode,_rwOpenGLTeamLightingCB);

        openglTeamSkinMatFXPipeIdx = _rpTeamPipeAddPipeDefinition( pipe,
                                                                   (TeamPipeOpen)NULL,
                                                                   (TeamPipeClose)NULL );

        _rpTeamOpenGLSetRenderPipeline( pipe, TEAMOPENGLPIPEID_SKINNED_MATFX );
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rpTeamMatFXPipelineDestroy
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
void
_rpTeamMatFXPipelineDestroy( void )
{
    RWFUNCTION( RWSTRING( "_rpTeamMatFXPipelineDestroy" ) );

    if ( TEAMPIPENULL != openglTeamMatFXPipeIdx )
    {
        _rpTeamPipeRemovePipeDefinition( openglTeamMatFXPipeIdx );
        openglTeamMatFXPipeIdx = (TeamPipeIndex)TEAMPIPENULL;

        _rpTeamOpenGLSetRenderPipeline( NULL, TEAMOPENGLPIPEID_STATIC_MATFX );
    }

    if ( TEAMPIPENULL != openglTeamSkinMatFXPipeIdx )
    {
        _rpTeamPipeRemovePipeDefinition( openglTeamSkinMatFXPipeIdx );
        openglTeamSkinMatFXPipeIdx = (TeamPipeIndex)TEAMPIPENULL;

        _rpTeamOpenGLSetRenderPipeline( NULL, TEAMOPENGLPIPEID_SKINNED_MATFX );
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rpTeamMatFXGetPipeline
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RxPipeline *
_rpTeamMatFXGetPipeline( RpTeamElementType type,
                         RwBool __RWUNUSED__ uv2 )
{
    RxPipeline  *pipe;


    RWFUNCTION( RWSTRING( "_rpTeamMatFXGetPipeline" ) );

    pipe = (RxPipeline *)NULL;
    if ( rpTEAMELEMENTSTATIC == type )
    {
        pipe = _rpTeamPipeGetPipeline( openglTeamMatFXPipeIdx );
    }
    else if ( rpTEAMELEMENTSKINNED == type )
    {
        pipe = _rpTeamPipeGetPipeline( openglTeamSkinMatFXPipeIdx );
    }

    RWRETURN( pipe );
}


/* ---------------------------------------------------------------------
 *  Function: _rpTeamOpenGLMatFXRenderCB
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
void
_rpTeamOpenGLMatFXRenderCB( RwResEntry *repEntry,
                            void *object,
                            const RwUInt8 type,
                            const RwUInt32 flags )
{
    RxOpenGLResEntryHeader      *resEntryHeader;

    RxOpenGLMeshInstanceData    *instanceData;

    RwBool                      lightingEnabled;

    RwMatrix                    identityMtx;

    RwMatrix                    *objectLTM;

    RwUInt16                    numMeshes;


    RWFUNCTION( RWSTRING( "_rpTeamOpenGLMatFXRenderCB" ) );

    /* get the instanced data */
    resEntryHeader = (RxOpenGLResEntryHeader *)(repEntry + 1);
    RWASSERT( NULL != resEntryHeader );

    instanceData = (RxOpenGLMeshInstanceData *)(resEntryHeader + 1);
    RWASSERT( NULL != instanceData );

    lightingEnabled = RwOpenGLIsEnabled( rwGL_LIGHTING );

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
        RwRGBA                      savedMatColor;

        RwRGBA                      playerMatColor;

        const RwRGBA                *matColor;

        const rpMatFXMaterialData   *matfxData;

        RpMatFXMaterialFlags        effectType;

        RwTexture                   *playerTexture;


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

        playerTexture = RpTeamMaterialGetPlayerTexture( instanceData->material,
                                                        _rpTeamPlayerGetCurrentPlayer() );

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
                                             playerTexture,
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
                                            playerTexture,
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
                                             playerTexture,
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
                                          playerTexture,
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
                                            playerTexture,
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
                                          playerTexture,
                                          flags,
                                          dualData,
                                          uvanimData );
            }
            break;

        default:
            {
                _rpMatFXOpenGLDefaultRender( instanceData, playerTexture, flags );
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

/* ---------------------------------------------------------------------
 *  Function:
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */

