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
#include "teammatfx.h"
#include "team.h"

#include "../../plugin/matfx/effectPipes.h"
#include "../../plugin/skin2/d3d8/skind3d8.h"

#include "teamshadowd3d8.h"
#include "teamskind3d8.h"

/*
 *

  CUSTOM FAST MATFX D3D8

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
 TeamD3D8AtomicMatFXRenderCallback
 */
static void
TeamD3D8AtomicMatFXRenderCallback(RwResEntry *repEntry,
                              void *object,
                              RwUInt8 type,
                              RwUInt32 flags)
{
    RxD3D8ResEntryHeader    *resEntryHeader;
    RxD3D8InstanceData      *instancedData;
    RpMatFXMaterialFlags    effectType;
    RwInt32                 numMeshes;
    RwBool                  lighting;
    RwBool                  forceBlack;

    const rpMatFXMaterialData   *matFXData;
    const MatFXBumpMapData      *bumpmap;
    const MatFXEnvMapData       *envMapData;

    RwTexture   *baseTexture;
    RwRGBA      oldColor;

    RWFUNCTION(RWSTRING("TeamD3D8AtomicMatFXRenderCallback"));

    if (flags & rxGEOMETRY_PRELIT)
    {
        /* Emmisive color from the vertex colors */
        RwD3D8SetRenderState(D3DRS_COLORVERTEX, TRUE);
        RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR1);
    }
    else
    {
        /* Emmisive color from material, set to black in the submit node */
        RwD3D8SetRenderState(D3DRS_COLORVERTEX, FALSE);
        RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
    }

    /* Enable clipping */
    if (_rpTeamShadowGetCurrentShadowData()!=NULL)
    {
        RwD3D8SetRenderState(D3DRS_CLIPPING, (_rpTeamShadowGetCurrentShadowData()->playerClip & 0x2) != 0);
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

        if (RwD3D8CameraIsSphereFullyInsideFrustum(cam, boundingSphere))
        {
            RwD3D8SetRenderState(D3DRS_CLIPPING, FALSE);
        }
        else
        {
            RwD3D8SetRenderState(D3DRS_CLIPPING, TRUE);
        }
    }

    /* Get the instanced data */
    resEntryHeader = (RxD3D8ResEntryHeader *)(repEntry + 1);
    instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

    /* Get lighting state */
    RwD3D8GetRenderState(D3DRS_LIGHTING, &lighting);

    if (lighting ||
        (flags & rxGEOMETRY_PRELIT) != 0 ||
        (instancedData->vertexShader & D3DFVF_RESERVED0) != 0)
    {
        forceBlack = FALSE;
    }
    else
    {
        forceBlack = TRUE;

        RwD3D8SetTexture(NULL, 0);

        RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, 0xff000000);

        RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
        RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
    }

    /* Get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;
    while (numMeshes--)
    {
        if (!forceBlack)
        {
            oldColor = instancedData->material->color;

            instancedData->material->color =
                *RpTeamMaterialGetPlayerColor(instancedData->material,
                                              _rpTeamPlayerGetCurrentPlayer());

            if ((instancedData->vertexShader & D3DFVF_RESERVED0) == 0)
            {
                if (lighting)
                {
                    RwD3D8SetSurfaceProperties(&instancedData->material->color,
                                            &instancedData->material->surfaceProps,
                                            (flags & rxGEOMETRY_MODULATE));
                }
            }
            else
            {
                _rpTeamD3D8SetVertexShaderMaterialColor(instancedData->material);
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
                    bumpmap = MATFXD3D8BUMPMAPGETDATA(instancedData->material);

                    _rpMatFXD3D8AtomicMatFXBumpMapRender(instancedData,
                                                         flags,
                                                         baseTexture,
                                                         bumpmap->texture,
                                                         NULL);
                }
                break;

            case rpMATFXEFFECTENVMAP:
                {
                    envMapData = MATFXD3D8ENVMAPGETDATA(instancedData->material, rpSECONDPASS);

                    _rpMatFXD3D8AtomicMatFXEnvRender(instancedData,
                                                     flags,
                                                     rpSECONDPASS,
                                                     baseTexture,
                                                     envMapData->texture);
                }
                break;

            case rpMATFXEFFECTBUMPENVMAP:
                {
                    bumpmap = MATFXD3D8BUMPMAPGETDATA(instancedData->material);
                    envMapData = MATFXD3D8ENVMAPGETDATA(instancedData->material, rpTHIRDPASS);

                    _rpMatFXD3D8AtomicMatFXBumpMapRender(instancedData,
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

                    _rpMatFXD3D8AtomicMatFXDualPassRender(instancedData,
                                                          flags,
                                                          baseTexture,
                                                          dualTexture);
                }
                break;

            case rpMATFXEFFECTUVTRANSFORM:
            default:
                _rpMatFXD3D8AtomicMatFXDefaultRender(instancedData,
                                                     flags,
                                                     baseTexture);
                break;
            }

            instancedData->material->color = oldColor;
        }
        else
        {
            _rpMatFXD3D8AtomicMatFXRenderBlack(instancedData);
        }

        /* Move onto the next instancedData */
        instancedData++;
    }

    if (forceBlack)
    {
        RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
        RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpTeamD3D8MatFXPipelineCreate
 */
static RxPipeline *
TeamD3D8MatFXPipelineCreate(void)
{
    RxPipeline  *pipe;

    RWFUNCTION(RWSTRING("TeamD3D8MatFXPipelineCreate"));

    pipe = RxPipelineCreate();

    if (pipe)
    {
        RxLockedPipe    *lpipe;

        pipe->pluginId = rwID_TEAMPLUGIN;
        pipe->pluginData = TEAMD3D8PIPEID_STATIC_MATFX;

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
                RxPipelineNode  *plnode;

                plnode = RxPipelineFindNodeByName( lpipe,
                                               instanceNode->name,
                                               (RxPipelineNode *)NULL,
                                               (RwInt32 *)NULL );

                RxD3D8AllInOneSetRenderCallBack(plnode,
                                        TeamD3D8AtomicMatFXRenderCallback);

                RxD3D8AllInOneSetLightingCallBack(plnode,
                                        _rxTeamD3D8AtomicLightingCallback);

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
 TeamD3D8AtomicMatFXSkinnedRenderCallback
 */
static void
TeamD3D8AtomicMatFXSkinnedRenderCallback(RwResEntry *repEntry,
                              void *object,
                              RwUInt8 type,
                              RwUInt32 flags)
{
    RWFUNCTION(RWSTRING("TeamD3D8AtomicMatFXSkinnedRenderCallback"));

    if( _rpTeamShadowGetCurrentShadowData()->playerClip != 0 )
    {
        TeamD3D8AtomicMatFXRenderCallback(repEntry, object, type, flags);
    }

    if (!_rpTeamD3D8UsingVertexShader)
    {
        _rwTeamD3D8SkinnedShadowsRenderCallback(repEntry, object, type, flags);
    }

    RWRETURNVOID();
}
#endif /*defined(TEAMSHADOWALLINONE)*/

/****************************************************************************
 _rxTeamMatFXSkinD3D8AtomicAllInOnePipelineInit
 */
static RwBool
TeamMatFXSkinD3D8AtomicAllInOnePipelineInit(RxPipelineNode *node)
{
    _rxD3D8SkinInstanceNodeData *instanceData;

    RWFUNCTION(RWSTRING("TeamMatFXSkinD3D8AtomicAllInOnePipelineInit"));

    instanceData = (_rxD3D8SkinInstanceNodeData *)node->privateData;

#if defined(TEAMSHADOWALLINONE)
    instanceData->renderCallback = TeamD3D8AtomicMatFXSkinnedRenderCallback;
#else
    instanceData->renderCallback = TeamD3D8AtomicMatFXRenderCallback;
#endif

    instanceData->lightingCallback = _rxTeamD3D8AtomicLightingCallback;

    RWRETURN(TRUE);
}

/*
 * _rxNodeDefinitionGetD3D8TeamMatFXSkinAtomicAllInOne returns a
 * pointer to the \ref RxNodeDefinition associated with
 * the Skin Atomic version of PowerPipe.
 */
static RxNodeDefinition *
NodeDefinitionGetD3D8TeamMatFXSkinAtomicAllInOne(void)
{
    static RwChar _TeamMatFXSkinAtomicInstance_csl[] = "nodeTeamMatFXSkinAtomicAllInOne.csl";

    static RxNodeDefinition nodeD3D8MatFXSkinAtomicAllInOneCSL = { /* */
        _TeamMatFXSkinAtomicInstance_csl,                    /* Name */
        {                                           /* Nodemethods */
            _rxTeamD3D8SkinnedAtomicAllInOneNode,   /* +-- nodebody */
            NULL,                                   /* +-- nodeinit */
            NULL,                                   /* +-- nodeterm */
            TeamMatFXSkinD3D8AtomicAllInOnePipelineInit,  /* +-- pipelinenodeinit */
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

    RWFUNCTION(RWSTRING("NodeDefinitionGetD3D8TeamMatFXSkinAtomicAllInOne"));

    RWRETURN(&nodeD3D8MatFXSkinAtomicAllInOneCSL);
}

static RxPipeline *
TeamD3D8MatFXSkinnedPipelineCreate(void)
{
    RxPipeline  *pipe;

    RWFUNCTION(RWSTRING("TeamD3D8MatFXSkinnedPipelineCreate"));

    pipe = RxPipelineCreate();

    if (pipe)
    {
        RxLockedPipe    *lpipe;

        pipe->pluginId = rwID_TEAMPLUGIN;
        pipe->pluginData = TEAMD3D8PIPEID_SKINNED_MATFX;

        if ((lpipe = RxPipelineLock(pipe)) != NULL)
        {
            RxNodeDefinition    *instanceNode;

            /*
             * Get the default instance node definition
             */
            instanceNode = NodeDefinitionGetD3D8TeamMatFXSkinAtomicAllInOne();
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

    pipeStatic = TeamD3D8MatFXPipelineCreate();
    TeamMatFXPipelineIndex = _rpTeamPipeAddPipeDefinition(pipeStatic,
                                  (TeamPipeOpen)NULL,
                                  (TeamPipeClose)NULL);

    pipeSkinned = TeamD3D8MatFXSkinnedPipelineCreate();
    TeamMatFXSkinnedPipelineIndex = _rpTeamPipeAddPipeDefinition(pipeSkinned,
                                  (TeamPipeOpen)NULL,
                                  (TeamPipeClose)NULL);

    _rpTeamD3D8SetRenderPipeline(pipeStatic, TEAMD3D8PIPEID_STATIC_MATFX);
    _rpTeamD3D8SetRenderPipeline(pipeSkinned, TEAMD3D8PIPEID_SKINNED_MATFX);

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

    _rpTeamD3D8SetRenderPipeline(NULL, TEAMD3D8PIPEID_STATIC_MATFX);
    _rpTeamD3D8SetRenderPipeline(NULL, TEAMD3D8PIPEID_SKINNED_MATFX);

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
