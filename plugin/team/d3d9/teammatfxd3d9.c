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
#include "teammatfx.h"
#include "team.h"

#include "../../plugin/matfx/effectPipes.h"
#include "../../plugin/skin2/d3d9/skind3d9.h"

#include "teamshadowd3d9.h"
#include "teamskind3d9.h"

/*
 *

  CUSTOM FAST MATFX D3D9

 *
 */

#define NUMCLUSTERSOFINTEREST   0
#define NUMOUTPUTS              0

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

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
static TeamPipeIndex TeamMatFXPipelineIndex = (TeamPipeIndex)TEAMPIPENULL;
static TeamPipeIndex TeamMatFXSkinnedPipelineIndex = (TeamPipeIndex)TEAMPIPENULL;

/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

 /****************************************************************************
 TeamD3D9AtomicMatFXRenderCallback
 */
static void
TeamD3D9AtomicMatFXRenderCallback(RwResEntry *repEntry,
                                  void *object,
                                  RwUInt8 type,
                                  RwUInt32 flags)
{
    RxD3D9ResEntryHeader    *resEntryHeader;
    RxD3D9InstanceData      *instancedData;
    RpMatFXMaterialFlags    effectType;
    RwInt32                 numMeshes;
    RwBool                  lighting;
    RwBool                  forceBlack;

    const rpMatFXMaterialData   *matFXData;
    const MatFXBumpMapData      *bumpmap;
    const MatFXEnvMapData       *envMapData;

    RwTexture   *baseTexture;
    RwRGBA      oldColor;

    RWFUNCTION(RWSTRING("TeamD3D9AtomicMatFXRenderCallback"));

    /* Enable clipping */
    if (_rpTeamShadowGetCurrentShadowData()!=NULL)
    {
        RwD3D9SetRenderState(D3DRS_CLIPPING, (_rpTeamShadowGetCurrentShadowData()->playerClip & 0x2) != 0);
    }
    else
    {
        RpAtomic                *atomic;
        RwCamera                *cam;
        const RwSphere          *boundingSphere;

        atomic = (RpAtomic *)object;

        cam = RwCameraGetCurrentCamera();
        RWASSERT(cam);

        boundingSphere = RpAtomicGetWorldBoundingSphere(atomic);

        if (RwD3D9CameraIsSphereFullyInsideFrustum(cam, boundingSphere))
        {
            RwD3D9SetRenderState(D3DRS_CLIPPING, FALSE);
        }
        else
        {
            RwD3D9SetRenderState(D3DRS_CLIPPING, TRUE);
        }
    }

    /* Get the instanced data */
    resEntryHeader = (RxD3D9ResEntryHeader *)(repEntry + 1);
    instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

    /* Get lighting state */
    RwD3D9GetRenderState(D3DRS_LIGHTING, &lighting);

    if (lighting ||
        (flags & rxGEOMETRY_PRELIT) != 0 ||
        instancedData->vertexShader != NULL)
    {
        forceBlack = FALSE;
    }
    else
    {
        forceBlack = TRUE;

        RwD3D9SetTexture(NULL, 0);

        RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, 0xff000000);

        RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
    }

    /*
     * Set Indices
     */
    if (resEntryHeader->indexBuffer != NULL)
    {
        RwD3D9SetIndices(resEntryHeader->indexBuffer);
    }

    /* Set the stream source */
    _rwD3D9SetStreams(resEntryHeader->vertexStream,
                      resEntryHeader->useOffsets);

    /*
     * Vertex declaration
     */
    RwD3D9SetVertexDeclaration(resEntryHeader->vertexDeclaration);

    /* Get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;
    while (numMeshes--)
    {
        if (instancedData->vertexAlpha ||
            (0xFF != instancedData->material->color.alpha))
        {
            _rwD3D9RenderStateVertexAlphaEnable(TRUE);
        }
        else
        {
            _rwD3D9RenderStateVertexAlphaEnable(FALSE);
        }

        if (!forceBlack)
        {
            oldColor = instancedData->material->color;

            instancedData->material->color =
                *RpTeamMaterialGetPlayerColor(instancedData->material,
                                              _rpTeamPlayerGetCurrentPlayer());

            if (instancedData->vertexShader == NULL)
            {
                if (lighting)
                {
                    RwD3D9SetSurfaceProperties(&instancedData->material->surfaceProps,
                                               &instancedData->material->color,
                                               flags);
                }
            }
            else
            {
                _rpTeamD3D9SetVertexShaderMaterialColor(instancedData->material);
            }

            /*
             * Render
             */
            matFXData = *MATFXMATERIALGETDATA(instancedData->material);
            if (NULL == matFXData)
            {
                /* This material hasn't been set up for MatFX so we
                 * treat it as is it were set to rpMATFXEFFECTNULL */
                effectType = rpMATFXEFFECTNULL;
            }
            else
            {
                effectType = matFXData->flags;
            }

            baseTexture = RpTeamMaterialGetPlayerTexture(instancedData->material,
                                                         _rpTeamPlayerGetCurrentPlayer());

            switch (effectType)
            {
            case rpMATFXEFFECTBUMPMAP:
                {
                    bumpmap = MATFXD3D9BUMPMAPGETDATA(instancedData->material);

                    _rpMatFXD3D9AtomicMatFXBumpMapRender(resEntryHeader,
                                                         instancedData,
                                                         flags,
                                                         baseTexture,
                                                         bumpmap->texture,
                                                         NULL);
                }
                break;

            case rpMATFXEFFECTENVMAP:
                {
                    envMapData = MATFXD3D9ENVMAPGETDATA(instancedData->material, rpSECONDPASS);

                    _rpMatFXD3D9AtomicMatFXEnvRender(resEntryHeader,
                                                     instancedData,
                                                     flags,
                                                     rpSECONDPASS,
                                                     baseTexture,
                                                     envMapData->texture);
                }
                break;

            case rpMATFXEFFECTBUMPENVMAP:
                {
                    bumpmap = MATFXD3D9BUMPMAPGETDATA(instancedData->material);
                    envMapData = MATFXD3D9ENVMAPGETDATA(instancedData->material, rpTHIRDPASS);

                    _rpMatFXD3D9AtomicMatFXBumpMapRender(resEntryHeader,
                                                         instancedData,
                                                         flags,
                                                         baseTexture,
                                                         bumpmap->texture,
                                                         envMapData->texture);
                }
                break;

            case rpMATFXEFFECTDUAL:
            case rpMATFXEFFECTDUALUVTRANSFORM:
                {
                    RwTexture *dualTexture = RpTeamMaterialGetPlayerDualTexture(instancedData->material,
                                                                                _rpTeamPlayerGetCurrentPlayer() );

                    _rpMatFXD3D9AtomicMatFXDualPassRender(resEntryHeader,
                                                          instancedData,
                                                          flags,
                                                          baseTexture,
                                                          dualTexture);
                }
                break;

            case rpMATFXEFFECTUVTRANSFORM:
            default:
                _rpMatFXD3D9AtomicMatFXDefaultRender(resEntryHeader,
                                                     instancedData,
                                                     flags,
                                                     baseTexture);
                break;
            }

            instancedData->material->color = oldColor;
        }
        else
        {
            _rpMatFXD3D9AtomicMatFXRenderBlack(resEntryHeader, instancedData);
        }

        /* Move onto the next instancedData */
        instancedData++;
    }

    if (forceBlack)
    {
        RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpTeamD3D9MatFXPipelineCreate
 */
static RxPipeline *
TeamD3D9MatFXPipelineCreate(void)
{
    RxPipeline  *pipe;

    RWFUNCTION(RWSTRING("TeamD3D9MatFXPipelineCreate"));

    pipe = RxPipelineCreate();

    if (pipe)
    {
        RxLockedPipe    *lpipe;

        pipe->pluginId = rwID_TEAMPLUGIN;
        pipe->pluginData = TEAMD3D9PIPEID_STATIC_MATFX;

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
                RxPipelineNode  *plnode;

                plnode = RxPipelineFindNodeByName( lpipe,
                                               instanceNode->name,
                                               (RxPipelineNode *)NULL,
                                               (RwInt32 *)NULL );

                RxD3D9AllInOneSetRenderCallBack(plnode,
                                        TeamD3D9AtomicMatFXRenderCallback);

                RxD3D9AllInOneSetLightingCallBack(plnode,
                                        _rxTeamD3D9AtomicLightingCallback);

                RWRETURN(pipe);
            }
        }

        RxPipelineDestroy(pipe);

        pipe = NULL;
    }

    RWRETURN(pipe);
}

#if defined(TEAMSHADOWALLINONE)
/****************************************************************************
 TeamD3D9AtomicMatFXSkinnedRenderCallback
 */
static void
TeamD3D9AtomicMatFXSkinnedRenderCallback(RwResEntry *repEntry,
                              void *object,
                              RwUInt8 type,
                              RwUInt32 flags)
{
    RWFUNCTION(RWSTRING("TeamD3D9AtomicMatFXSkinnedRenderCallback"));

    if( _rpTeamShadowGetCurrentShadowData()->playerClip != 0 )
    {
        TeamD3D9AtomicMatFXRenderCallback(repEntry, object, type, flags);
    }

    if (!_rpTeamD3D9UsingVertexShader)
    {
        _rwTeamD3D9SkinnedShadowsRenderCallback(repEntry, object, type, flags);
    }

    RWRETURNVOID();
}
#endif /*defined(TEAMSHADOWALLINONE)*/

/****************************************************************************
 _rxTeamMatFXSkinD3D9AtomicAllInOnePipelineInit
 */
static RwBool
TeamMatFXSkinD3D9AtomicAllInOnePipelineInit(RxPipelineNode *node)
{
    _rxD3D9SkinInstanceNodeData *instanceData;

    RWFUNCTION(RWSTRING("TeamMatFXSkinD3D9AtomicAllInOnePipelineInit"));

    instanceData = (_rxD3D9SkinInstanceNodeData *)node->privateData;

#if defined(TEAMSHADOWALLINONE)
    instanceData->renderCallback = TeamD3D9AtomicMatFXSkinnedRenderCallback;
#else
    instanceData->renderCallback = TeamD3D9AtomicMatFXRenderCallback;
#endif

    instanceData->lightingCallback = _rxTeamD3D9AtomicLightingCallback;

    RWRETURN(TRUE);
}

/*
 * _rxNodeDefinitionGetD3D9TeamMatFXSkinAtomicAllInOne returns a
 * pointer to the \ref RxNodeDefinition associated with
 * the Skin Atomic version of PowerPipe.
 */
static RxNodeDefinition *
NodeDefinitionGetD3D9TeamMatFXSkinAtomicAllInOne(void)
{
    static RwChar _TeamMatFXSkinAtomicInstance_csl[] = RWSTRING("nodeTeamMatFXSkinAtomicAllInOne.csl");

    static RxNodeDefinition nodeD3D9MatFXSkinAtomicAllInOneCSL = { /* */
        _TeamMatFXSkinAtomicInstance_csl,                    /* Name */
        {                                           /* Nodemethods */
            _rxTeamD3D9SkinnedAtomicAllInOneNode,   /* +-- nodebody */
            NULL,                                   /* +-- nodeinit */
            NULL,                                   /* +-- nodeterm */
            TeamMatFXSkinD3D9AtomicAllInOnePipelineInit,  /* +-- pipelinenodeinit */
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

    RWFUNCTION(RWSTRING("NodeDefinitionGetD3D9TeamMatFXSkinAtomicAllInOne"));

    RWRETURN(&nodeD3D9MatFXSkinAtomicAllInOneCSL);
}

static RxPipeline *
TeamD3D9MatFXSkinnedPipelineCreate(void)
{
    RxPipeline  *pipe;

    RWFUNCTION(RWSTRING("TeamD3D9MatFXSkinnedPipelineCreate"));

    pipe = RxPipelineCreate();

    if (pipe)
    {
        RxLockedPipe    *lpipe;

        pipe->pluginId = rwID_TEAMPLUGIN;
        pipe->pluginData = TEAMD3D9PIPEID_SKINNED_MATFX;

        if ((lpipe = RxPipelineLock(pipe)) != NULL)
        {
            RxNodeDefinition    *instanceNode;

            /*
             * Get the default instance node definition
             */
            instanceNode = NodeDefinitionGetD3D9TeamMatFXSkinAtomicAllInOne();
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

/*---------------------- CREATE PIPELINE -------------------------------*/
RwBool
_rpTeamMatFXPipelineCreate(void)
{
    RxPipeline *pipeStatic, *pipeSkinned;

    RWFUNCTION(RWSTRING("_rpTeamMatFXPipelineCreate"));

    pipeStatic = TeamD3D9MatFXPipelineCreate();
    TeamMatFXPipelineIndex = _rpTeamPipeAddPipeDefinition(pipeStatic,
                                  (TeamPipeOpen)NULL,
                                  (TeamPipeClose)NULL);

    pipeSkinned = TeamD3D9MatFXSkinnedPipelineCreate();
    TeamMatFXSkinnedPipelineIndex = _rpTeamPipeAddPipeDefinition(pipeSkinned,
                                  (TeamPipeOpen)NULL,
                                  (TeamPipeClose)NULL);

    _rpTeamD3D9SetRenderPipeline(pipeStatic, TEAMD3D9PIPEID_STATIC_MATFX);
    _rpTeamD3D9SetRenderPipeline(pipeSkinned, TEAMD3D9PIPEID_SKINNED_MATFX);

    RWRETURN(TRUE);
}

/*----------------------------------------------------------------------*/

/*--------------------- DESTROY PIPELINE -------------------------------*/
void
_rpTeamMatFXPipelineDestroy(void)
{
    RWFUNCTION(RWSTRING("_rpTeamMatFXPipelineDestroy"));

    RWASSERT(TEAMPIPENULL != TeamMatFXSkinnedPipelineIndex);
    _rpTeamPipeRemovePipeDefinition(TeamMatFXSkinnedPipelineIndex);
    TeamMatFXSkinnedPipelineIndex = TEAMPIPENULL;

    RWASSERT(TEAMPIPENULL != TeamMatFXPipelineIndex);
    _rpTeamPipeRemovePipeDefinition(TeamMatFXPipelineIndex);
    TeamMatFXPipelineIndex = TEAMPIPENULL;

    _rpTeamD3D9SetRenderPipeline(NULL, TEAMD3D9PIPEID_STATIC_MATFX);
    _rpTeamD3D9SetRenderPipeline(NULL, TEAMD3D9PIPEID_SKINNED_MATFX);

    RWRETURNVOID();
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/

RxPipeline *
_rpTeamMatFXGetPipeline(RpTeamElementType type, RwBool uv2)
{
    RWFUNCTION(RWSTRING("_rpTeamMatFXGetPipeline"));

    if (type == rpTEAMELEMENTSKINNED)
    {
        /* custom skin pipe */
        RWRETURN(_rpTeamPipeGetPipeline(TeamMatFXSkinnedPipelineIndex));
    }
    else if (type == rpTEAMELEMENTSTATIC)
    {
        /* just the usual matfx pipe */
        RWRETURN(_rpTeamPipeGetPipeline(TeamMatFXPipelineIndex));
    }

    RWRETURN(NULL);
}

/*----------------------------------------------------------------------*/

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/
