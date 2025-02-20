/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/

#include <d3d9.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpteam.h"

#include "teampipes.h"
#include "teamshadow.h"
#include "team.h"

#include "../../plugin/skin2/d3d9/skind3d9.h"

#include "skindefs.h"

#define NUMCLUSTERSOFINTEREST   0
#define NUMOUTPUTS              0

/*
 *

  CUSTOM FAST SHADOWS D3D9

 *
 */

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/

typedef struct _rpTeamD3D9ShadowLocalData rpTeamD3D9ShadowLocalData;
struct _rpTeamD3D9ShadowLocalData
{
    RwUInt32    shadeMode;
    RwUInt32    zFunc;
    RwUInt32    srcBlend, dstBlend;
    RwUInt32    alphaBlend, alphaTest;
    RwUInt32    fogColor;
};

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
static rpTeamD3D9ShadowLocalData   rpTeamD3D9ShadowData;

static TeamPipeIndex TeamShadowStaticPipelineIndex = (TeamPipeIndex)TEAMPIPENULL;
static TeamPipeIndex TeamShadowSkinedPipelineIndex = (TeamPipeIndex)TEAMPIPENULL;

/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

static void
TeamD3D9ShadowPipeOpen(RwBool fogEnabled)
{
    RWFUNCTION(RWSTRING("TeamD3D9ShadowPipeOpen"));

    /* Remove any texture */
    RwD3D9SetTexture(NULL, 0);

    /*
    set up texturing modes so shadows are black!
    */
    RwD3D9GetRenderState( D3DRS_SHADEMODE, &rpTeamD3D9ShadowData.shadeMode );
    RwD3D9SetRenderState( D3DRS_SHADEMODE, D3DSHADE_FLAT );

    /* change zfunc to remove z artifacts */
    RwD3D9GetRenderState( D3DRS_ZFUNC, &rpTeamD3D9ShadowData.zFunc );
    RwD3D9SetRenderState( D3DRS_ZFUNC, D3DCMP_LESS );

    /* setup blend mode for shadow  */
    RwD3D9GetRenderState( D3DRS_SRCBLEND,  &rpTeamD3D9ShadowData.srcBlend );
    RwD3D9GetRenderState( D3DRS_DESTBLEND, &rpTeamD3D9ShadowData.dstBlend );

    RwD3D9SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_DESTCOLOR );
    RwD3D9SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );

    /* enable alpha blend */
    RwD3D9GetRenderState( D3DRS_ALPHABLENDENABLE, &rpTeamD3D9ShadowData.alphaBlend);
    RwD3D9SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE);

    /* Disable alpha test */
    RwD3D9GetRenderState( D3DRS_ALPHATESTENABLE, &rpTeamD3D9ShadowData.alphaTest);
    RwD3D9SetRenderState( D3DRS_ALPHATESTENABLE, FALSE);

    /* Set correct fog color */
    if (fogEnabled)
    {
        RwD3D9GetRenderState( D3DRS_FOGCOLOR, &rpTeamD3D9ShadowData.fogColor);
        RwD3D9SetRenderState( D3DRS_FOGCOLOR, 0xffffffff);
    }

    /* Set lighting state */
    RwD3D9SetRenderState(D3DRS_LIGHTING, FALSE);

    RwD3D9SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);

    /* Use only texture factor color */
    RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);

    RwD3D9SetPixelShader(NULL);

    /*
    Use the stencil buffer so we don't render over top
    of same shadow twice and make it too dark
    */
    RwRenderStateSet( rwRENDERSTATESTENCILENABLE, (void *)TRUE );
    RwRenderStateSet( rwRENDERSTATESTENCILPASS, (void *)rwSTENCILOPERATIONREPLACE );
    RwRenderStateSet( rwRENDERSTATESTENCILFUNCTION, (void *)rwSTENCILFUNCTIONNOTEQUAL );

    RwRenderStateSet( rwRENDERSTATESTENCILFUNCTIONREF, (void *)1 );

    RWRETURNVOID();
}

static void
TeamD3D9ShadowPipeClose(RwBool fogEnabled)
{
    RWFUNCTION(RWSTRING("TeamD3D9ShadowPipeClose"));

    RwRenderStateSet( rwRENDERSTATESTENCILENABLE, FALSE );

    RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    if (fogEnabled)
    {
        RwD3D9SetRenderState( D3DRS_FOGCOLOR, rpTeamD3D9ShadowData.fogColor);
    }

    RwD3D9SetRenderState( D3DRS_ALPHABLENDENABLE, rpTeamD3D9ShadowData.alphaBlend);
    RwD3D9SetRenderState( D3DRS_ALPHATESTENABLE, rpTeamD3D9ShadowData.alphaTest);

    RwD3D9SetRenderState( D3DRS_SHADEMODE, rpTeamD3D9ShadowData.shadeMode );

    RwD3D9SetRenderState( D3DRS_ZFUNC, rpTeamD3D9ShadowData.zFunc );

    RwD3D9SetRenderState( D3DRS_SRCBLEND, rpTeamD3D9ShadowData.srcBlend );
    RwD3D9SetRenderState( D3DRS_DESTBLEND, rpTeamD3D9ShadowData.dstBlend );

    RWRETURNVOID();
}

/*===========================================================================*
 *--- Shadows  --------------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Skinned Shadows  ------------------------------------------------------*
 *===========================================================================*/

/****************************************************************************
 _rwTeamD3D9SkinnedShadowsRenderCallback
 */
void
_rwTeamD3D9SkinnedShadowsRenderCallback(RwResEntry *repEntry,
                              void *object,
                              RwUInt8 type,
                              RwUInt32 flags)
{
    const RpTeamShadowRenderData  *shadow;

    RWFUNCTION(RWSTRING("_rwTeamD3D9SkinnedShadowsRenderCallback"));

    shadow = (const RpTeamShadowRenderData *)_rpTeamShadowGetCurrentShadowData();

    if (shadow->renderNumShadows > 0)
    {
        RwBool                  fogEnabled;

        RpAtomic                    *atomic;

        const RxD3D9ResEntryHeader  *resEntryHeader;
        const RxD3D9InstanceData    *instancedData;
        RwInt32                     numMeshes;

        const RwMatrix          *ltmMtx;

        const RpTeamPlayer      *player;
        RwMatrix                shadowMtx, worldMatrix;

        const RwV3d             *projPlaneNormal;
        RwReal                  projPlaneD;
        RwUInt32                i;
        RwUInt32                shadowCol;

        RwV3d                   *shadowRight, *shadowUp, *shadowAt, *shadowPos;

        _rpTeamD3D9MatrixTransposed shadowMtxTransposed;

        /* Set shadows renderstates */
        RwD3D9GetRenderState(D3DRS_FOGENABLE, &fogEnabled);

        TeamD3D9ShadowPipeOpen(fogEnabled);

        /*
         * Parallel projection matrix onto shadow plane (cos that's what the PS2 version does)
         * See Real Time Rendering for derivation of point source projection - parallel is similar.
         */
        atomic = (RpAtomic *)object;
        RWASSERT(atomic);

        resEntryHeader = (const RxD3D9ResEntryHeader *)(repEntry + 1);

        ltmMtx = TeamFrameGetLTM(RpAtomicGetFrame(atomic));

        player = _rpTeamPlayerGetCurrentPlayer();

        shadowRight = RwMatrixGetRight(&shadowMtx);
        shadowUp    = RwMatrixGetUp(&shadowMtx);
        shadowAt    = RwMatrixGetAt(&shadowMtx);
        shadowPos   = RwMatrixGetPos(&shadowMtx);

        projPlaneNormal = &(shadow->worldPlaneNormal.v3d.v);
        projPlaneD = RwV3dDotProduct(projPlaneNormal, &(shadow->worldPlanePos.v3d.v));

        shadowCol = 0xffffffff;

        /* Set the shared Index buffer */
        RwD3D9SetIndices(resEntryHeader->indexBuffer);

        /* Set the stream source */
        _rwD3D9SetStreams(resEntryHeader->vertexStream,
                          resEntryHeader->useOffsets);

        /* Set the Vertex declaration */
        RwD3D9SetVertexDeclaration(resEntryHeader->vertexDeclaration);

        /* Render shadows */
        for( i = 0; i < shadow->renderNumShadows; i++ )
        {
            const RwV3d *projL = &(shadow->worldProjection[i].v3d.v);

            shadowRight->x = 1.0f - projPlaneNormal->x * projL->x;
            shadowRight->y =      - projPlaneNormal->x * projL->y;
            shadowRight->z =      - projPlaneNormal->x * projL->z;
            shadowUp->x    =      - projPlaneNormal->y * projL->x;
            shadowUp->y    = 1.0f - projPlaneNormal->y * projL->y;
            shadowUp->z    =      - projPlaneNormal->y * projL->z;
            shadowAt->x    =      - projPlaneNormal->z * projL->x;
            shadowAt->y    =      - projPlaneNormal->z * projL->y;
            shadowAt->z    = 1.0f - projPlaneNormal->z * projL->z;
            shadowPos->x   =               projPlaneD * projL->x;
            shadowPos->y   =               projPlaneD * projL->y;
            shadowPos->z   =               projPlaneD * projL->z;

            shadowMtx.flags = 0;

            /* Get the instanced data */
            instancedData = (const RxD3D9InstanceData *)(resEntryHeader + 1);

            /*
             * Vertex shader
             */
            if (_rpTeamD3D9UsingVertexShader)
            {
                if (_rpTeamD3D9UsingLocalSpace)
                {
                    RwMatrixMultiply(&worldMatrix, ltmMtx, &shadowMtx);

                    _rwTeamD3D9SkinMatrixMultiplyTranspose(
                                                    &shadowMtxTransposed,
                                                    &worldMatrix,
                                                    &_rpTeamD3D9projViewMatrix);
                }
                else
                {
                    _rwTeamD3D9SkinMatrixMultiplyTranspose(
                                                    &shadowMtxTransposed,
                                                    &shadowMtx,
                                                    &_rpTeamD3D9projViewMatrix);
                }

                RwD3D9SetVertexShaderConstant(VSCONST_REG_TRANSFORM_OFFSET,
                                  (const void *)&shadowMtxTransposed,
                                  VSCONST_REG_TRANSFORM_SIZE);

                if (fogEnabled)
                {
                    RwD3D9SetVertexShader(_rpTeamD3D9UnlitSkinVertexShader);
                }
                else
                {
                    RwD3D9SetVertexShader(_rpTeamD3D9UnlitNoFogSkinVertexShader);
                }
            }
            else
            {
                RwMatrixMultiply(&worldMatrix, ltmMtx, &shadowMtx);

                RwD3D9SetTransformWorld(&worldMatrix);

                RwD3D9SetVertexShader(instancedData->vertexShader);
            }

            /* Shadow col */
            shadowCol = RwFastRealToUInt32(
                        (((RwReal)1.0 - shadow->renderValues[i]) * (RwReal)255.0) );

            shadowCol = (   ((RwUInt32)(shadowCol) << 24) |
                            ((RwUInt32)(shadowCol) << 16) |
                            ((RwUInt32)(shadowCol) << 8) |
                            shadowCol   );

            RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR,  shadowCol);

            /* Check clipping state */
            RwD3D9SetRenderState(D3DRS_CLIPPING, (shadow->shadowClip & (1 << i)) != 0);

            /*
            Shadow region of each light needs unique ID in stencil buffer
            this isn't quite right cause rpteam culls shadows in way that we can't
            determine the original light, but it seems to work well enough...
            */
            /*RwRenderStateSet( rwRENDERSTATESTENCILFUNCTIONREF, (void *)(i+1) );*/

            /* Get the number of meshes */
            numMeshes = resEntryHeader->numMeshes;
            while (numMeshes--)
            {

                /* Draw the indexed primitive */
                RwD3D9DrawIndexedPrimitive((D3DPRIMITIVETYPE)resEntryHeader->primType,
                                           instancedData->baseIndex,
                                           0, instancedData->numVertices,
                                           instancedData->startIndex,
                                           instancedData->numPrimitives);

                /* Move onto the next instancedData */
                instancedData++;
            }
        }

        TeamD3D9ShadowPipeClose(fogEnabled);
    }

    RWRETURNVOID();
}

/****************************************************************************
 TeamShadowSkinD3D9AtomicAllInOnePipelineInit
 */
static RwBool
TeamShadowSkinD3D9AtomicAllInOnePipelineInit(RxPipelineNode *node)
{
    _rxD3D9SkinInstanceNodeData *instanceData;

    RWFUNCTION(RWSTRING("TeamShadowSkinD3D9AtomicAllInOnePipelineInit"));

    instanceData = (_rxD3D9SkinInstanceNodeData *)node->privateData;

    instanceData->renderCallback = _rwTeamD3D9SkinnedShadowsRenderCallback;

    instanceData->lightingCallback = NULL;

    RWRETURN(TRUE);
}

/*
 * NodeDefinitionGetD3D9TeamShadowSkinAtomicAllInOne returns a
 * pointer to the \ref RxNodeDefinition associated with
 * the Skin Atomic version of PowerPipe.
 */
static RxNodeDefinition *
NodeDefinitionGetD3D9TeamShadowSkinAtomicAllInOne(void)
{
    static RwChar _TeamShadowSkinAtomicInstance_csl[] = RWSTRING("nodeTeamShadowSkinAtomicAllInOne.csl");

    static RxNodeDefinition nodeD3D9SkinAtomicAllInOneCSL = { /* */
        _TeamShadowSkinAtomicInstance_csl,                    /* Name */
        {                                           /* Nodemethods */
            _rxTeamD3D9SkinnedAtomicAllInOneNode,   /* +-- nodebody */
            NULL,                                   /* +-- nodeinit */
            NULL,                                   /* +-- nodeterm */
            TeamShadowSkinD3D9AtomicAllInOnePipelineInit,  /* +-- pipelinenodeinit */
            NULL,                                   /* +-- pipelineNodeTerm */
            NULL,                                   /* +-- pipelineNodeConfig */
            NULL,                                   /* +-- configMsgHandler */
        },
        {                                           /* Io */
            NUMCLUSTERSOFINTEREST,                  /* +-- NumClustersOfInterest */
            NULL,                                   /* +-- ClustersOfInterest */
            NULL,                                   /* +-- InputRequirements */
            NUMOUTPUTS,                             /* +-- NumOutputs */
            NULL                                    /* +-- Outputs */
        },
        (RwUInt32)sizeof(_rxD3D9SkinInstanceNodeData),/* PipelineNodePrivateDataSize */
        (RxNodeDefEditable)FALSE,                                      /* editable */
        0                                           /* inPipes */
    };

    RWFUNCTION(RWSTRING("NodeDefinitionGetD3D9TeamShadowSkinAtomicAllInOne"));

    RWRETURN(&nodeD3D9SkinAtomicAllInOneCSL);
}

static RxPipeline *
TeamD3D9SkinnedShadowPipelineCreate(void)
{
    RxPipeline  *pipe;

    RWFUNCTION(RWSTRING("TeamD3D9SkinnedShadowPipelineCreate"));

    pipe = RxPipelineCreate();

    if (pipe)
    {
        RxLockedPipe    *lpipe;

        pipe->pluginId = rwID_TEAMPLUGIN;
        pipe->pluginData = TEAMD3D9PIPEID_SKINNED_SHADOW;

        if ((lpipe = RxPipelineLock(pipe)) != NULL)
        {
            RxNodeDefinition    *instanceNode;

            /*
             * Get the default instance node definition
             */
            instanceNode = NodeDefinitionGetD3D9TeamShadowSkinAtomicAllInOne();
            RWASSERT(NULL != instanceNode);

            lpipe = RxLockedPipeAddFragment(lpipe, NULL,
                                           instanceNode,
                                           NULL);

            lpipe = RxLockedPipeUnlock(lpipe);

            RWASSERT(pipe == (RxPipeline *)lpipe);
            RWRETURN(pipe);
        }

        RxPipelineDestroy(pipe);

        pipe = NULL;
    }

    RWRETURN(pipe);
}

/*===========================================================================*
 *--- Static Shadows  -------------------------------------------------------*
 *===========================================================================*/

void
_rwTeamD3D9StaticShadowsRenderCallback(RwResEntry *repEntry,
                              void *object,
                              RwUInt8 type,
                              RwUInt32 flags)
{
    const RpTeamShadowRenderData  *shadow;

    RWFUNCTION(RWSTRING("_rwTeamD3D9StaticShadowsRenderCallback"));

    shadow = (const RpTeamShadowRenderData *) _rpTeamShadowGetCurrentShadowData();

    if (shadow->renderNumShadows > 0)
    {
        RpAtomic                    *atomic;

        RwBool                      fogEnabled;

        const RxD3D9ResEntryHeader  *resEntryHeader;
        const RxD3D9InstanceData    *instancedData;
        RwInt32                     numMeshes;

        const RpTeamPlayer     *player;

        RwUInt32                i;
        RwUInt32                shadowCol;

        const RwMatrix          *ltmMtx;
        const RwV3d             *projPlaneNormal;
        RwReal                  projPlaneD;
        RwMatrix                shadowMtx, worldMatrix;

        RwV3d                  *shadowRight, *shadowUp, *shadowAt, *shadowPos;

        atomic = (RpAtomic *)object;
        RWASSERT(atomic);

        resEntryHeader = (RxD3D9ResEntryHeader *)(repEntry + 1);

        shadowCol = 0xffffffff;

         player = _rpTeamPlayerGetCurrentPlayer();

        /* Set shadows renderstates */
        RwD3D9GetRenderState(D3DRS_FOGENABLE, &fogEnabled);

        TeamD3D9ShadowPipeOpen(fogEnabled);

        /*
         * Parallel projection matrix onto shadow plane (cos that's what the PS2 version does)
         * See Real Time Rendering for derivation of point source projection - parallel is similar.
         */

        ltmMtx = TeamFrameGetLTM(RpAtomicGetFrame(atomic));

        shadowRight = RwMatrixGetRight(&shadowMtx);
        shadowUp    = RwMatrixGetUp(&shadowMtx);
        shadowAt    = RwMatrixGetAt(&shadowMtx);
        shadowPos   = RwMatrixGetPos(&shadowMtx);

        projPlaneNormal = &(shadow->worldPlaneNormal.v3d.v);

        projPlaneD = RwV3dDotProduct(projPlaneNormal, &(shadow->worldPlanePos.v3d.v));

        /* Set the shared Index buffer */
        RwD3D9SetIndices(resEntryHeader->indexBuffer);

        /* Set the stream source */
        _rwD3D9SetStreams(resEntryHeader->vertexStream,
                          resEntryHeader->useOffsets);

        /* Set the Vertex declaration */
        RwD3D9SetVertexDeclaration(resEntryHeader->vertexDeclaration);

        /* Render shadows */
        for (i = 0; i < shadow->renderNumShadows; i++)
        {
            const RwV3d *projL = &(shadow->worldProjection[i].v3d.v);

            /*
            * Calculate the transform matrix.
            */
            shadowRight->x = 1.0f - projPlaneNormal->x * projL->x;
            shadowRight->y =      - projPlaneNormal->x * projL->y;
            shadowRight->z =      - projPlaneNormal->x * projL->z;
            shadowUp->x    =      - projPlaneNormal->y * projL->x;
            shadowUp->y    = 1.0f - projPlaneNormal->y * projL->y;
            shadowUp->z    =      - projPlaneNormal->y * projL->z;
            shadowAt->x    =      - projPlaneNormal->z * projL->x;
            shadowAt->y    =      - projPlaneNormal->z * projL->y;
            shadowAt->z    = 1.0f - projPlaneNormal->z * projL->z;
            shadowPos->x   =               projPlaneD * projL->x;
            shadowPos->y   =               projPlaneD * projL->y;
            shadowPos->z   =               projPlaneD * projL->z;

            RwMatrixUpdate(&shadowMtx);

            RwMatrixMultiply(&worldMatrix, ltmMtx, &shadowMtx);

            RwD3D9SetTransformWorld(&worldMatrix);

            shadowCol = RwFastRealToUInt32(
                        (((RwReal)1.0 - shadow->renderValues[i]) * (RwReal)255.0) );

            shadowCol = (   ((RwUInt32)(shadowCol) << 24) |
                            ((RwUInt32)(shadowCol) << 16) |
                            ((RwUInt32)(shadowCol) << 8) |
                            shadowCol   );

            RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR,  shadowCol);

            /* Check clipping state */
            RwD3D9SetRenderState(D3DRS_CLIPPING, (shadow->shadowClip & (1 << i)) != 0);

            /*
            Shadow region of each light needs unique ID in stencil buffer
            this isn't quite right cause rpteam culls shadows in way that we can't
            determine the original light, but it seems to work well enough...
            */
            /*RwRenderStateSet( rwRENDERSTATESTENCILFUNCTIONREF, (void *)(i+1) );*/

            /* Get the instanced data */
            instancedData = (const RxD3D9InstanceData *)(resEntryHeader + 1);

            /*
             * Vertex shader
             */
            RwD3D9SetVertexShader(instancedData->vertexShader);

            /* Get the number of meshes */
            numMeshes = resEntryHeader->numMeshes;
            while (numMeshes--)
            {

                /* Draw the indexed primitive */
                RwD3D9DrawIndexedPrimitive((D3DPRIMITIVETYPE)resEntryHeader->primType,
                                           instancedData->baseIndex,
                                           0, instancedData->numVertices,
                                           instancedData->startIndex,
                                           instancedData->numPrimitives);

                /* Move onto the next instancedData */
                instancedData++;
            }
        }

        TeamD3D9ShadowPipeClose(fogEnabled);
    }

    RWRETURNVOID();
}

static RxPipeline *
TeamD3D9StaticShadowPipelineCreate(void)
{
    RxPipeline  *pipe;

    RWFUNCTION(RWSTRING("TeamD3D9StaticShadowPipelineCreate"));

    pipe = RxPipelineCreate();

    if (pipe)
    {
        RxLockedPipe    *lpipe;
        RxPipelineNode  *plnode;

        pipe->pluginId = rwID_TEAMPLUGIN;
        pipe->pluginData = TEAMD3D9PIPEID_STATIC_SHADOW;

        if ((lpipe = RxPipelineLock(pipe)) != NULL)
        {
            RxNodeDefinition    *instanceNode;

            /*
             * Get the default instance node definition
             */
            instanceNode = RxNodeDefinitionGetD3D9AtomicAllInOne();
            RWASSERT(NULL != instanceNode);

            lpipe = RxLockedPipeAddFragment(lpipe, NULL,
                                           instanceNode,
                                           NULL);

            lpipe = RxLockedPipeUnlock(lpipe);

            RWASSERT(pipe == (RxPipeline *)lpipe);

            if (lpipe != NULL)
            {
                plnode = RxPipelineFindNodeByName( lpipe,
                                               instanceNode->name,
                                               (RxPipelineNode *)NULL,
                                               (RwInt32 *)NULL );

                RxD3D9AllInOneSetRenderCallBack(plnode,
                                        _rwTeamD3D9StaticShadowsRenderCallback);

                RxD3D9AllInOneSetLightingCallBack(plnode, NULL);

                RWRETURN(pipe);
            }
        }

        RxPipelineDestroy(pipe);

        pipe = NULL;
    }

    RWRETURN(pipe);
}

/*---- CREATE PIPELINE -------------------------------------------------*/
RwBool
_rpTeamShadowPipelineCreate(void)
{
    RxPipeline *pipe;

    RWFUNCTION(RWSTRING("_rpTeamShadowPipelineCreate"));

    pipe = TeamD3D9SkinnedShadowPipelineCreate();

    if (pipe)
    {
        TeamShadowSkinedPipelineIndex =
            _rpTeamPipeAddPipeDefinition(pipe,
                                        (TeamPipeOpen)NULL,
                                        (TeamPipeClose)NULL);
        RWASSERT(TEAMPIPENULL != TeamShadowSkinedPipelineIndex);

        _rpTeamD3D9SetRenderPipeline(pipe, TEAMD3D9PIPEID_SKINNED_SHADOW);
    }

    pipe = TeamD3D9StaticShadowPipelineCreate();

    if (pipe)
    {
        TeamShadowStaticPipelineIndex =
            _rpTeamPipeAddPipeDefinition(pipe,
                                          (TeamPipeOpen)NULL,
                                          (TeamPipeClose)NULL);
        RWASSERT(TEAMPIPENULL != TeamShadowStaticPipelineIndex);

        _rpTeamD3D9SetRenderPipeline(pipe, TEAMD3D9PIPEID_STATIC_SHADOW);
    }

    RWRETURN(TRUE);
}

/*----------------------------------------------------------------------*/

/*---- DESTROY PIPELINE ------------------------------------------------*/
void
_rpTeamShadowPipelineDestroy(void)
{
    RWFUNCTION(RWSTRING("_rpTeamShadowPipelineDestroy"));

    RWASSERT(TEAMPIPENULL != TeamShadowSkinedPipelineIndex);
    _rpTeamPipeRemovePipeDefinition(TeamShadowSkinedPipelineIndex);

    RWASSERT(TEAMPIPENULL != TeamShadowStaticPipelineIndex);
    _rpTeamPipeRemovePipeDefinition(TeamShadowStaticPipelineIndex);

    TeamShadowStaticPipelineIndex = (TeamPipeIndex)TEAMPIPENULL;
    TeamShadowSkinedPipelineIndex = (TeamPipeIndex)TEAMPIPENULL;

    _rpTeamD3D9SetRenderPipeline(NULL, TEAMD3D9PIPEID_SKINNED_SHADOW);
    _rpTeamD3D9SetRenderPipeline(NULL, TEAMD3D9PIPEID_STATIC_SHADOW);

    RWRETURNVOID();
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
RxPipeline *
_rpTeamShadowGetPipeline(RpTeamElementType type, RwBool allInOne)
{
    RxPipeline *pipeline;
    TeamPipeIndex index;

    RWFUNCTION(RWSTRING("_rpTeamShadowGetPipeline"));

    pipeline = (RxPipeline *)NULL;

    switch(type)
    {
        case rpTEAMELEMENTSTATIC:
        {
            index = TeamShadowStaticPipelineIndex;
            break;
        }
        case rpTEAMELEMENTSKINNED:
        {
            index = TeamShadowSkinedPipelineIndex;
            break;
        }
        default:
        {
            RWASSERT(FALSE);
            RWRETURN(NULL);
        }
    }

    pipeline = _rpTeamPipeGetPipeline(index);
    RWASSERT(NULL != pipeline);

    RWRETURN(pipeline);
}

/*----------------------------------------------------------------------*/

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/


