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
#include "teamstatic.h"
#include "team.h"

#include "teamshadowd3d8.h"

/*
 *

  CUSTOM FAST SKIN

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

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
static TeamPipeIndex    TeamStaticPipelineIndex = (TeamPipeIndex)TEAMPIPENULL;

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
 _rpTeamD3D8AtomicStaticRenderCallback
 */
static void
_rpTeamD3D8AtomicStaticRenderCallback(RwResEntry *repEntry,
                                      void *object,
                                      RwUInt8 type,
                                      RwUInt32 flags)
{
    RxD3D8ResEntryHeader    *resEntryHeader;
    RxD3D8InstanceData      *instancedData;
    RwInt32                 numMeshes;
    RwBool                  lighting;
    RwBool                  vertexAlphaBlend;
    RwBool                  forceBlack;
    RwUInt32                ditherEnable;
    RwUInt32                shadeMode;
    void                    *lastVertexBuffer;
    RwRGBA                  color;

    RWFUNCTION(RWSTRING("_rpTeamD3D8AtomicStaticRenderCallback"));

    /* Get lighting state */
    RwD3D8GetRenderState(D3DRS_LIGHTING, &lighting);

    forceBlack = FALSE;

    if (lighting)
    {
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
    }
    else
    {
        if ((flags & rxGEOMETRY_PRELIT) == 0)
        {
            forceBlack = TRUE;

            RwD3D8GetRenderState(D3DRS_DITHERENABLE, &ditherEnable);
            RwD3D8GetRenderState(D3DRS_SHADEMODE, &shadeMode);

            RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, 0xff000000);
            RwD3D8SetRenderState(D3DRS_DITHERENABLE, FALSE);
            RwD3D8SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);
        }
    }

    /* Enable clipping */
    if (_rpTeamShadowGetCurrentShadowData() != NULL)
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

    /* Set texture to NULL if hasn't any texture flags */
    if ( (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2)) == 0)
    {
        RwD3D8SetTexture(NULL, 0);

        if (forceBlack)
        {
            RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
            RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

            RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);
        }
    }

    /* Get vertex alpha Blend state */
    vertexAlphaBlend = _rwD3D8RenderStateIsVertexAlphaEnable();

    /* Set Last vertex buffer to force the call */
    lastVertexBuffer = (void *)0xffffffff;

    /* Get the instanced data */
    resEntryHeader = (RxD3D8ResEntryHeader *)(repEntry + 1);
    instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

    /*
     * Data shared between meshes
     */

    /*
     * Set the Default Pixel shader
     */
    RwD3D8SetPixelShader(0);

    /*
     * Vertex shader
     */
    RwD3D8SetVertexShader(instancedData->vertexShader);

    /* Get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;
    while (numMeshes--)
    {
        RWASSERT(instancedData->material != NULL);

        color = *RpTeamMaterialGetPlayerColor(instancedData->material,
                                              _rpTeamPlayerGetCurrentPlayer());

        if ( (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2)) )
        {
            RwTexture *baseTexture = RpTeamMaterialGetPlayerTexture(instancedData->material,
                                                                    _rpTeamPlayerGetCurrentPlayer());

            RwD3D8SetTexture(baseTexture, 0);

            if (forceBlack)
            {
                /* Only change the colorop, we need to use the texture alpha channel */
                RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
                RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
            }
        }

        if (instancedData->vertexAlpha ||
            (0xFF != color.alpha))
        {
            if (!vertexAlphaBlend)
            {
                vertexAlphaBlend = TRUE;

                _rwD3D8RenderStateVertexAlphaEnable(TRUE);
            }
        }
        else
        {
            if (vertexAlphaBlend)
            {
                vertexAlphaBlend = FALSE;

                _rwD3D8RenderStateVertexAlphaEnable(FALSE);
            }
        }

        if (lighting)
        {
            if (instancedData->vertexAlpha)
            {
                RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
            }
            else
            {
                RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
            }

            RwD3D8SetSurfaceProperties(&color,
                                        &instancedData->material->surfaceProps,
                                        (flags & rxGEOMETRY_MODULATE));
        }

        /*
         * Render
         */

        /* Set the stream source */
        if (lastVertexBuffer != instancedData->vertexBuffer)
        {
            RwD3D8SetStreamSource(0, instancedData->vertexBuffer, instancedData->stride);

            lastVertexBuffer = instancedData->vertexBuffer;
        }

        /* Set the Index buffer */
        if (instancedData->indexBuffer != NULL)
        {
            RwD3D8SetIndices(instancedData->indexBuffer, instancedData->baseIndex);

            /* Draw the indexed primitive */
            RwD3D8DrawIndexedPrimitive((D3DPRIMITIVETYPE)instancedData->primType,
                                       0, instancedData->numVertices,
                                       0, instancedData->numIndices);
        }
        else
        {
            RwD3D8DrawPrimitive((D3DPRIMITIVETYPE)instancedData->primType,
                                instancedData->baseIndex,
                                instancedData->numVertices);
        }

        /* Move onto the next instancedData */
        instancedData++;
    }

    if (forceBlack)
    {
        RwD3D8SetRenderState(D3DRS_DITHERENABLE, ditherEnable);
        RwD3D8SetRenderState(D3DRS_SHADEMODE, shadeMode);

        if (_rwD3D8RWGetRasterStage(0))
        {
            RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
            RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        }
        else
        {
            RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
            RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        }
    }

    RWRETURNVOID();
}

/****************************************************************************
 TeamD3D8StaticPipelineCreate
 */
static RxPipeline *
TeamD3D8StaticPipelineCreate(void)
{
    RxPipeline  *pipe;

    RWFUNCTION(RWSTRING("TeamD3D8StaticPipelineCreate"));

    pipe = RxPipelineCreate();

    if (pipe)
    {
        RxLockedPipe    *lpipe;

        pipe->pluginId = rwID_TEAMPLUGIN;
        pipe->pluginData = TEAMD3D8PIPEID_STATIC;

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

                RxD3D8AllInOneSetLightingCallBack(plnode,
                                        _rxTeamD3D8AtomicLightingCallback);

                RxD3D8AllInOneSetRenderCallBack(plnode,
                                        _rpTeamD3D8AtomicStaticRenderCallback);

                RWRETURN(pipe);
            }
        }

        RxPipelineDestroy(pipe);

        pipe = NULL;
    }

    RWRETURN(pipe);
}

/****************************************************************************
 _rpTeamStaticCustomPipelineCreate
 */
RwBool
_rpTeamStaticCustomPipelineCreate(void)
{
    RxPipeline *pipe;

    RWFUNCTION(RWSTRING("_rpTeamStaticCustomPipelineCreate"));

    pipe = TeamD3D8StaticPipelineCreate();

    if (pipe)
    {
        TeamStaticPipelineIndex = _rpTeamPipeAddPipeDefinition(pipe,
                                                      (TeamPipeOpen)NULL,
                                                      (TeamPipeClose)NULL);

        _rpTeamD3D8SetRenderPipeline(pipe, TEAMD3D8PIPEID_STATIC);

        RWRETURN(TRUE);
    }

    RWRETURN(FALSE);
}

void
_rpTeamStaticCustomPipelineDestroy(void)
{
    RWFUNCTION(RWSTRING("_rpTeamStaticCustomPipelineDestroy"));

    RWASSERT(TEAMPIPENULL != TeamStaticPipelineIndex);
    _rpTeamPipeRemovePipeDefinition(TeamStaticPipelineIndex);
    TeamStaticPipelineIndex = (TeamPipeIndex)TEAMPIPENULL;

    _rpTeamD3D8SetRenderPipeline(NULL, TEAMD3D8PIPEID_STATIC);

    RWRETURNVOID();
}

RxPipeline *
_rpTeamStaticGetCustomPipeline(void)
{
    RWFUNCTION(RWSTRING("_rpTeamStaticGetCustomPipeline"));
    RWRETURN(_rpTeamPipeGetPipeline(TeamStaticPipelineIndex));
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/
