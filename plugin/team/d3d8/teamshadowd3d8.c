/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/

#include <d3d8.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpteam.h"

#include "teampipes.h"
#include "teamshadow.h"
#include "team.h"

#include "../../plugin/skin2/d3d8/skind3d8.h"

#include "skindefs.h"

#define NUMCLUSTERSOFINTEREST   0
#define NUMOUTPUTS              0

/*
 *

  CUSTOM FAST SHADOWS D3D8

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

typedef struct _rpTeamD3D8ShadowLocalData rpTeamD3D8ShadowLocalData;
struct _rpTeamD3D8ShadowLocalData
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
static rpTeamD3D8ShadowLocalData   rpTeamD3D8ShadowData;

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
TeamD3D8ShadowPipeOpen(RwBool fogEnabled)
{
    RWFUNCTION(RWSTRING("TeamD3D8ShadowPipeOpen"));

    /* Remove any texture */
    RwD3D8SetTexture(NULL, 0);

    /*
    set up texturing modes so shadows are black!
    */
    RwD3D8GetRenderState( D3DRS_SHADEMODE, &rpTeamD3D8ShadowData.shadeMode );
    RwD3D8SetRenderState( D3DRS_SHADEMODE, D3DSHADE_FLAT );

    /* change zfunc to remove z artifacts */
    RwD3D8GetRenderState( D3DRS_ZFUNC, &rpTeamD3D8ShadowData.zFunc );
    RwD3D8SetRenderState( D3DRS_ZFUNC, D3DCMP_LESS );

    /* setup blend mode for shadow  */
    RwD3D8GetRenderState( D3DRS_SRCBLEND,  &rpTeamD3D8ShadowData.srcBlend );
    RwD3D8GetRenderState( D3DRS_DESTBLEND, &rpTeamD3D8ShadowData.dstBlend );

    RwD3D8SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_DESTCOLOR );
    RwD3D8SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );

    /* enable alpha blend */
    RwD3D8GetRenderState( D3DRS_ALPHABLENDENABLE, &rpTeamD3D8ShadowData.alphaBlend);
    RwD3D8SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE);

    /* Disable alpha test */
    RwD3D8GetRenderState( D3DRS_ALPHATESTENABLE, &rpTeamD3D8ShadowData.alphaTest);
    RwD3D8SetRenderState( D3DRS_ALPHATESTENABLE, FALSE);

    /* Set correct fog color */
    if (fogEnabled)
    {
        RwD3D8GetRenderState( D3DRS_FOGCOLOR, &rpTeamD3D8ShadowData.fogColor);
        RwD3D8SetRenderState( D3DRS_FOGCOLOR, 0xffffffff);
    }

    /* Set lighting state */
    RwD3D8SetRenderState(D3DRS_LIGHTING, FALSE);

    RwD3D8SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);

    /* Use only texture factor color */
    RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
    RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);

    RwD3D8SetPixelShader(0);

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
TeamD3D8ShadowPipeClose(RwBool fogEnabled)
{
    RWFUNCTION(RWSTRING("TeamD3D8ShadowPipeClose"));

    RwRenderStateSet( rwRENDERSTATESTENCILENABLE, FALSE );

    RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
    RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

    if (fogEnabled)
    {
        RwD3D8SetRenderState( D3DRS_FOGCOLOR, rpTeamD3D8ShadowData.fogColor);
    }

    RwD3D8SetRenderState( D3DRS_ALPHABLENDENABLE, rpTeamD3D8ShadowData.alphaBlend);
    RwD3D8SetRenderState( D3DRS_ALPHATESTENABLE, rpTeamD3D8ShadowData.alphaTest);

    RwD3D8SetRenderState( D3DRS_SHADEMODE, rpTeamD3D8ShadowData.shadeMode );

    RwD3D8SetRenderState( D3DRS_ZFUNC, rpTeamD3D8ShadowData.zFunc );

    RwD3D8SetRenderState( D3DRS_SRCBLEND, rpTeamD3D8ShadowData.srcBlend );
    RwD3D8SetRenderState( D3DRS_DESTBLEND, rpTeamD3D8ShadowData.dstBlend );

    RWRETURNVOID();
}

/*===========================================================================*
 *--- Shadows  --------------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Skinned Shadows  ------------------------------------------------------*
 *===========================================================================*/

/****************************************************************************
 _rwTeamD3D8SkinnedShadowsRenderCallback
 */
void
_rwTeamD3D8SkinnedShadowsRenderCallback(RwResEntry *repEntry,
                              void *object,
                              RwUInt8 type,
                              RwUInt32 flags)
{
    const RpTeamShadowRenderData  *shadow;

    RWFUNCTION(RWSTRING("_rwTeamD3D8SkinnedShadowsRenderCallback"));

    shadow = (const RpTeamShadowRenderData *)_rpTeamShadowGetCurrentShadowData();

    if (shadow->renderNumShadows > 0)
    {
        RwBool                  fogEnabled;

        RpAtomic                    *atomic;

        const RxD3D8ResEntryHeader  *resEntryHeader;
        const RxD3D8InstanceData    *instancedData;
        RwInt32                     numMeshes;

        const RwMatrix          *ltmMtx;

        const RpTeamPlayer      *player;
        RwMatrix                shadowMtx, worldMatrix;

        const RwV3d             *projPlaneNormal;
        RwReal                  projPlaneD;
        RwUInt32                i;
        RwUInt32                shadowCol;

        RwV3d                   *shadowRight, *shadowUp, *shadowAt, *shadowPos;

        _rpTeamD3D8MatrixTransposed shadowMtxTransposed;

        /* Set shadows renderstates */
        RwD3D8GetRenderState(D3DRS_FOGENABLE, &fogEnabled);

        TeamD3D8ShadowPipeOpen(fogEnabled);

        /*
         * Parallel projection matrix onto shadow plane (cos that's what the PS2 version does)
         * See Real Time Rendering for derivation of point source projection - parallel is similar.
         */
        atomic = (RpAtomic *)object;
        RWASSERT(atomic);

        resEntryHeader = (const RxD3D8ResEntryHeader *)(repEntry + 1);

        ltmMtx = TeamFrameGetLTM(RpAtomicGetFrame(atomic));

        player = _rpTeamPlayerGetCurrentPlayer();

        shadowRight = RwMatrixGetRight(&shadowMtx);
        shadowUp    = RwMatrixGetUp(&shadowMtx);
        shadowAt    = RwMatrixGetAt(&shadowMtx);
        shadowPos   = RwMatrixGetPos(&shadowMtx);

        projPlaneNormal = &(shadow->worldPlaneNormal.v3d.v);
        projPlaneD = RwV3dDotProduct(projPlaneNormal, &(shadow->worldPlanePos.v3d.v));

        shadowCol = 0xffffffff;

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
            instancedData = (const RxD3D8InstanceData *)(resEntryHeader + 1);

            /*
             * Vertex shader
             */
            if (_rpTeamD3D8UsingVertexShader)
            {
                if (_rpTeamD3D8UsingLocalSpace)
                {
                    RwMatrixMultiply(&worldMatrix, ltmMtx, &shadowMtx);

                    _rwTeamD3D8SkinMatrixMultiplyTranspose(
                                                    &shadowMtxTransposed,
                                                    &worldMatrix,
                                                    &_rpTeamD3D8projViewMatrix);
                }
                else
                {
                    _rwTeamD3D8SkinMatrixMultiplyTranspose(
                                                    &shadowMtxTransposed,
                                                    &shadowMtx,
                                                    &_rpTeamD3D8projViewMatrix);
                }

                RwD3D8SetVertexShaderConstant(VSCONST_REG_TRANSFORM_OFFSET,
                                  (const void *)&shadowMtxTransposed,
                                  VSCONST_REG_TRANSFORM_SIZE);

                if (fogEnabled)
                {
                    RwD3D8SetVertexShader(_rpTeamD3D8UnlitSkinVertexShader);
                }
                else
                {
                    RwD3D8SetVertexShader(_rpTeamD3D8UnlitNoFogSkinVertexShader);
                }
            }
            else
            {
                RwMatrixMultiply(&worldMatrix, ltmMtx, &shadowMtx);

                RwD3D8SetTransformWorld(&worldMatrix);

                RwD3D8SetVertexShader(instancedData->vertexShader);
            }

            /* Shadow col */
            shadowCol = RwFastRealToUInt32(
                        (((RwReal)1.0 - shadow->renderValues[i]) * (RwReal)255.0) );

            shadowCol = (   ((RwUInt32)(shadowCol) << 24) |
                            ((RwUInt32)(shadowCol) << 16) |
                            ((RwUInt32)(shadowCol) << 8) |
                            shadowCol   );

            RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR,  shadowCol);

            /* Check clipping state */
            RwD3D8SetRenderState(D3DRS_CLIPPING, (shadow->shadowClip & (1 << i)) != 0);

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
                /* Set the stream source */
                RwD3D8SetStreamSource(0, instancedData->vertexBuffer, instancedData->stride);

                /* Set the Index buffer */
                RwD3D8SetIndices(instancedData->indexBuffer, instancedData->baseIndex);

                /* Draw the indexed primitive */
                RwD3D8DrawIndexedPrimitive((D3DPRIMITIVETYPE)instancedData->primType,
                                           0, instancedData->numVertices,
                                           0, instancedData->numIndices);

                /* Move onto the next instancedData */
                instancedData++;
            }
        }

        TeamD3D8ShadowPipeClose(fogEnabled);
    }

    RWRETURNVOID();
}

/****************************************************************************
 TeamShadowSkinD3D8AtomicAllInOnePipelineInit
 */
static RwBool
TeamShadowSkinD3D8AtomicAllInOnePipelineInit(RxPipelineNode *node)
{
    _rxD3D8SkinInstanceNodeData *instanceData;

    RWFUNCTION(RWSTRING("TeamShadowSkinD3D8AtomicAllInOnePipelineInit"));

    instanceData = (_rxD3D8SkinInstanceNodeData *)node->privateData;

    instanceData->renderCallback = _rwTeamD3D8SkinnedShadowsRenderCallback;

    instanceData->lightingCallback = NULL;

    RWRETURN(TRUE);
}

/*
 * NodeDefinitionGetD3D8TeamShadowSkinAtomicAllInOne returns a
 * pointer to the \ref RxNodeDefinition associated with
 * the Skin Atomic version of PowerPipe.
 */
static RxNodeDefinition *
NodeDefinitionGetD3D8TeamShadowSkinAtomicAllInOne(void)
{
    static RwChar _TeamShadowSkinAtomicInstance_csl[] = "nodeTeamShadowSkinAtomicAllInOne.csl";

    static RxNodeDefinition nodeD3D8SkinAtomicAllInOneCSL = { /* */
        _TeamShadowSkinAtomicInstance_csl,                    /* Name */
        {                                           /* Nodemethods */
            _rxTeamD3D8SkinnedAtomicAllInOneNode,   /* +-- nodebody */
            NULL,                                   /* +-- nodeinit */
            NULL,                                   /* +-- nodeterm */
            TeamShadowSkinD3D8AtomicAllInOnePipelineInit,  /* +-- pipelinenodeinit */
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
        (RwUInt32)sizeof(_rxD3D8SkinInstanceNodeData),/* PipelineNodePrivateDataSize */
        (RxNodeDefEditable)FALSE,                                      /* editable */
        0                                           /* inPipes */
    };

    RWFUNCTION(RWSTRING("NodeDefinitionGetD3D8TeamShadowSkinAtomicAllInOne"));

    RWRETURN(&nodeD3D8SkinAtomicAllInOneCSL);
}

static RxPipeline *
TeamD3D8SkinnedShadowPipelineCreate(void)
{
    RxPipeline  *pipe;

    RWFUNCTION(RWSTRING("TeamD3D8SkinnedShadowPipelineCreate"));

    pipe = RxPipelineCreate();

    if (pipe)
    {
        RxLockedPipe    *lpipe;

        pipe->pluginId = rwID_TEAMPLUGIN;
        pipe->pluginData = TEAMD3D8PIPEID_SKINNED_SHADOW;

        if ((lpipe = RxPipelineLock(pipe)) != NULL)
        {
            RxNodeDefinition    *instanceNode;

            /*
             * Get the default instance node definition
             */
            instanceNode = NodeDefinitionGetD3D8TeamShadowSkinAtomicAllInOne();
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
_rwTeamD3D8StaticShadowsRenderCallback(RwResEntry *repEntry,
                              void *object,
                              RwUInt8 type,
                              RwUInt32 flags)
{
    const RpTeamShadowRenderData  *shadow;

    RWFUNCTION(RWSTRING("_rwTeamD3D8StaticShadowsRenderCallback"));

    shadow = (const RpTeamShadowRenderData *) _rpTeamShadowGetCurrentShadowData();

    if (shadow->renderNumShadows > 0)
    {
        RpAtomic                    *atomic;

        RwBool                      fogEnabled;

        const RxD3D8ResEntryHeader  *resEntryHeader;
        const RxD3D8InstanceData    *instancedData;
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

        resEntryHeader = (RxD3D8ResEntryHeader *)(repEntry + 1);

        shadowCol = 0xffffffff;

         player = _rpTeamPlayerGetCurrentPlayer();

        /* Set shadows renderstates */
        RwD3D8GetRenderState(D3DRS_FOGENABLE, &fogEnabled);

        TeamD3D8ShadowPipeOpen(fogEnabled);

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

            RwD3D8SetTransformWorld(&worldMatrix);

            shadowCol = RwFastRealToUInt32(
                        (((RwReal)1.0 - shadow->renderValues[i]) * (RwReal)255.0) );

            shadowCol = (   ((RwUInt32)(shadowCol) << 24) |
                            ((RwUInt32)(shadowCol) << 16) |
                            ((RwUInt32)(shadowCol) << 8) |
                            shadowCol   );

            RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR,  shadowCol);

            /* Check clipping state */
            RwD3D8SetRenderState(D3DRS_CLIPPING, (shadow->shadowClip & (1 << i)) != 0);

            /*
            Shadow region of each light needs unique ID in stencil buffer
            this isn't quite right cause rpteam culls shadows in way that we can't
            determine the original light, but it seems to work well enough...
            */
            /*RwRenderStateSet( rwRENDERSTATESTENCILFUNCTIONREF, (void *)(i+1) );*/

            /* Get the instanced data */
            instancedData = (const RxD3D8InstanceData *)(resEntryHeader + 1);

            /*
             * Vertex shader
             */
            RwD3D8SetVertexShader(instancedData->vertexShader);

            /* Get the number of meshes */
            numMeshes = resEntryHeader->numMeshes;
            while (numMeshes--)
            {
                /* Set the stream source */
                RwD3D8SetStreamSource(0, instancedData->vertexBuffer, instancedData->stride);

                /* Set the Index buffer */
                RwD3D8SetIndices(instancedData->indexBuffer, instancedData->baseIndex);

                /* Draw the indexed primitive */
                RwD3D8DrawIndexedPrimitive((D3DPRIMITIVETYPE)instancedData->primType,
                                           0, instancedData->numVertices,
                                           0, instancedData->numIndices);

                /* Move onto the next instancedData */
                instancedData++;
            }
        }

        TeamD3D8ShadowPipeClose(fogEnabled);
    }

    RWRETURNVOID();
}

static RxPipeline *
TeamD3D8StaticShadowPipelineCreate(void)
{
    RxPipeline  *pipe;

    RWFUNCTION(RWSTRING("TeamD3D8StaticShadowPipelineCreate"));

    pipe = RxPipelineCreate();

    if (pipe)
    {
        RxLockedPipe    *lpipe;
        RxPipelineNode  *plnode;

        pipe->pluginId = rwID_TEAMPLUGIN;
        pipe->pluginData = TEAMD3D8PIPEID_STATIC_SHADOW;

        if ((lpipe = RxPipelineLock(pipe)) != NULL)
        {
            RxNodeDefinition    *instanceNode;

            /*
             * Get the default instance node definition
             */
            instanceNode = RxNodeDefinitionGetD3D8AtomicAllInOne();
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

                RxD3D8AllInOneSetRenderCallBack(plnode,
                                        _rwTeamD3D8StaticShadowsRenderCallback);

                RxD3D8AllInOneSetLightingCallBack(plnode, NULL);

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

    pipe = TeamD3D8SkinnedShadowPipelineCreate();

    if (pipe)
    {
        TeamShadowSkinedPipelineIndex =
            _rpTeamPipeAddPipeDefinition(pipe,
                                        (TeamPipeOpen)NULL,
                                        (TeamPipeClose)NULL);
        RWASSERT(TEAMPIPENULL != TeamShadowSkinedPipelineIndex);

        _rpTeamD3D8SetRenderPipeline(pipe, TEAMD3D8PIPEID_SKINNED_SHADOW);
    }

    pipe = TeamD3D8StaticShadowPipelineCreate();

    if (pipe)
    {
        TeamShadowStaticPipelineIndex =
            _rpTeamPipeAddPipeDefinition(pipe,
                                          (TeamPipeOpen)NULL,
                                          (TeamPipeClose)NULL);
        RWASSERT(TEAMPIPENULL != TeamShadowStaticPipelineIndex);

        _rpTeamD3D8SetRenderPipeline(pipe, TEAMD3D8PIPEID_STATIC_SHADOW);
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

    _rpTeamD3D8SetRenderPipeline(NULL, TEAMD3D8PIPEID_SKINNED_SHADOW);
    _rpTeamD3D8SetRenderPipeline(NULL, TEAMD3D8PIPEID_STATIC_SHADOW);

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


