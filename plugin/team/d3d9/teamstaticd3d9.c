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
#include "teamstatic.h"
#include "team.h"

#include "teamshadowd3d9.h"

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

#define RENDERFLAGS_HAS_TEXTURE  1
#define RENDERFLAGS_HAS_TFACTOR  2

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
 _rpTeamD3D9RenderBlack
 Purpose:
 On entry:
 On exit :
*/
static void
_rpTeamD3D9RenderBlack(RxD3D9ResEntryHeader *resEntryHeader,
                       RwBool hasTextureCoordinates)
{
    RxD3D9InstanceData *instancedData;
    RwUInt32    ditherEnable;
    RwUInt32    shadeMode;
    RwBool      useAlphaTexture;
    RwInt32     numMeshes;
    RwTexture   *baseTexture;

    RWFUNCTION(RWSTRING("_rpTeamD3D9RenderBlack"));

    /* Save some renderstates */
    RwD3D9GetRenderState(D3DRS_DITHERENABLE, &ditherEnable);
    RwD3D9GetRenderState(D3DRS_SHADEMODE, &shadeMode);

    /* No dither, No Gouraud, No vertex alpha */
    _rwD3D9RenderStateVertexAlphaEnable(FALSE);
    RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, 0xff000000);
    RwD3D9SetRenderState(D3DRS_DITHERENABLE, FALSE);
    RwD3D9SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);

    /* Always black color (only alpha channel may change) */
    RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);

    RwD3D9SetTexture(NULL, 0);

    /* force refresh */
    useAlphaTexture = FALSE;

    /* Get the instanced data */
    instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

    /* Get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;
    while (numMeshes--)
    {
        RWASSERT(instancedData->material != NULL);

        baseTexture = RpTeamMaterialGetPlayerTexture(instancedData->material,
                                                     _rpTeamPlayerGetCurrentPlayer());

        /* this function check internally for alpha channel */
        if (hasTextureCoordinates &&
            baseTexture &&
            _rwD3D9TextureHasAlpha(baseTexture))
        {
            RwD3D9SetTexture(baseTexture, 0);

            if (useAlphaTexture == FALSE)
            {
                useAlphaTexture = TRUE;

                /* We only use the alpha channel from the texture */
                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
            }
        }
        else
        {
            if (useAlphaTexture)
            {
                useAlphaTexture = FALSE;

                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);

                RwD3D9SetTexture(NULL, 0);
            }
        }

        /*
        * Vertex shader
        */
        RwD3D9SetVertexShader(instancedData->vertexShader);

        /*
        * Render
        */
        if (resEntryHeader->indexBuffer != NULL)
        {
            RwD3D9DrawIndexedPrimitive((D3DPRIMITIVETYPE)resEntryHeader->primType,
                                    instancedData->baseIndex,
                                    0, instancedData->numVertices,
                                    instancedData->startIndex, instancedData->numPrimitives);
        }
        else
        {
            RwD3D9DrawPrimitive((D3DPRIMITIVETYPE)resEntryHeader->primType,
                                instancedData->baseIndex,
                                instancedData->numPrimitives);
        }

        /* Move onto the next instancedData */
        instancedData++;
    }

    /* Restore some renderstates */
    RwD3D9SetRenderState(D3DRS_DITHERENABLE, ditherEnable);
    RwD3D9SetRenderState(D3DRS_SHADEMODE, shadeMode);

    RWRETURNVOID();
}

/****************************************************************************
 _rpTeamD3D9StaticRenderCallback
 Purpose:
 On entry:
 On exit :
*/
static void
_rpTeamD3D9StaticRenderCallback(RwResEntry *repEntry,
                                void *object,
                                RwUInt8 type,
                                RwUInt32 flags)
{
    RxD3D9ResEntryHeader    *resEntryHeader;
    RxD3D9InstanceData      *instancedData;
    RwInt32                 numMeshes;
    RwBool                  lighting;
    RwBool                  vertexAlphaBlend;
    RwUInt32                lastRenderFlags;
    RwTexture               *baseTexture;

    RWFUNCTION(RWSTRING("_rpTeamD3D9StaticRenderCallback"));

    /*
     * Set the Default Pixel shader
     */
    RwD3D9SetPixelShader(NULL);

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

    /* Get header */
    resEntryHeader = (RxD3D9ResEntryHeader *)(repEntry + 1);

    /*
     * Data shared between meshes
     */
    if (resEntryHeader->indexBuffer != NULL)
    {
        RwD3D9SetIndices(resEntryHeader->indexBuffer);
    }

    /* Set the stream sources */
    _rwD3D9SetStreams(resEntryHeader->vertexStream,
                      resEntryHeader->useOffsets);

    /*
    * Vertex Declaration
    */
    RwD3D9SetVertexDeclaration(resEntryHeader->vertexDeclaration);

    /* check lighting */
    RwD3D9GetRenderState(D3DRS_LIGHTING, &lighting);

    if (lighting == FALSE)
    {
        if ((flags & rxGEOMETRY_PRELIT) == 0)
        {
            _rpTeamD3D9RenderBlack(resEntryHeader,
                                   (flags & (rxGEOMETRY_TEXTURED | rxGEOMETRY_TEXTURED2)) != 0);

            RWRETURNVOID();
        }
    }

    /* Get vertex alpha Blend state */
    vertexAlphaBlend = _rwD3D9RenderStateIsVertexAlphaEnable();

    /* force refresh */
    lastRenderFlags = 0x80000000;

    /* Get the instanced data */
    instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

    /* Get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;
    while (numMeshes--)
    {
        const RpMaterial    *material;
        RwRGBA              matcolor;
        RwUInt32            currentRenderFlags = 0;

        RWASSERT(instancedData->material != NULL);

        material = instancedData->material;

        baseTexture = RpTeamMaterialGetPlayerTexture(material,
                                                     _rpTeamPlayerGetCurrentPlayer());

        matcolor = *RpTeamMaterialGetPlayerColor(material,
                                                 _rpTeamPlayerGetCurrentPlayer());

        if ((0xFF != matcolor.alpha) ||
            instancedData->vertexAlpha)
        {
            if (vertexAlphaBlend == FALSE)
            {
                vertexAlphaBlend = TRUE;

                _rwD3D9RenderStateVertexAlphaEnable(TRUE);
            }
        }
        else
        {
            if (vertexAlphaBlend != FALSE)
            {
                vertexAlphaBlend = FALSE;

                _rwD3D9RenderStateVertexAlphaEnable(FALSE);
            }
        }

        if (lighting)
        {
            RwD3D9SetSurfaceProperties(&(material->surfaceProps),
                                       &matcolor,
                                       flags);
        }
        else
        {
            if (flags & rxGEOMETRY_MODULATE)
            {
                if (*((const RwUInt32 *)&matcolor) != 0xffffffff)
                {
                    RwUInt32 tFactor;

                    currentRenderFlags |= RENDERFLAGS_HAS_TFACTOR;

                    tFactor =
                    ((((RwUInt32)matcolor.alpha)<<24)|(((RwUInt32)matcolor.red)<<16)|(((RwUInt32)matcolor.green)<<8)|((RwUInt32)matcolor.blue));

                    RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, tFactor);
                }
            }
        }

        if (baseTexture != NULL &&
            (flags & (rxGEOMETRY_TEXTURED | rxGEOMETRY_TEXTURED2)) != 0)
        {
            currentRenderFlags |= RENDERFLAGS_HAS_TEXTURE;

            RwD3D9SetTexture(baseTexture, 0);
        }
        else
        {
            RwD3D9SetTexture(NULL, 0);
        }

        if (currentRenderFlags != lastRenderFlags)
        {
            if (currentRenderFlags & RENDERFLAGS_HAS_TEXTURE)
            {
                if ((lastRenderFlags & RENDERFLAGS_HAS_TEXTURE) == 0)
                {
                    RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
                }

                if (currentRenderFlags & RENDERFLAGS_HAS_TFACTOR)
                {
                    RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                    RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
                    RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_TFACTOR);

                    RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
                    RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
                    RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
                }
                else
                {
                    if (lastRenderFlags == (RENDERFLAGS_HAS_TEXTURE | RENDERFLAGS_HAS_TFACTOR))
                    {
                        RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
                        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);
                    }
                }
            }
            else
            {
                if (currentRenderFlags & RENDERFLAGS_HAS_TFACTOR)
                {
                    RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
                    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
                }
                else
                {
                    RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
                    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
                }

                if (lastRenderFlags == (RENDERFLAGS_HAS_TEXTURE | RENDERFLAGS_HAS_TFACTOR))
                {
                    RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
                    RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);
                }
            }

            lastRenderFlags = currentRenderFlags;
        }

        /*
        * Vertex shader
        */
        RwD3D9SetVertexShader(instancedData->vertexShader);

        /*
         * Render
         */
        if (resEntryHeader->indexBuffer != NULL)
        {
            RwD3D9DrawIndexedPrimitive((D3DPRIMITIVETYPE)resEntryHeader->primType,
                                       instancedData->baseIndex,
                                       0, instancedData->numVertices,
                                       instancedData->startIndex, instancedData->numPrimitives);
        }
        else
        {
            RwD3D9DrawPrimitive((D3DPRIMITIVETYPE)resEntryHeader->primType,
                                instancedData->baseIndex,
                                instancedData->numPrimitives);
        }

        /* Move onto the next instancedData */
        instancedData++;
    }

    if (lastRenderFlags == (RENDERFLAGS_HAS_TEXTURE | RENDERFLAGS_HAS_TFACTOR))
    {
        RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);
    }

    RWRETURNVOID();
}

/****************************************************************************
 TeamD3D9StaticPipelineCreate
 */
static RxPipeline *
TeamD3D9StaticPipelineCreate(void)
{
    RxPipeline  *pipe;

    RWFUNCTION(RWSTRING("TeamD3D9StaticPipelineCreate"));

    pipe = RxPipelineCreate();

    if (pipe)
    {
        RxLockedPipe    *lpipe;

        pipe->pluginId = rwID_TEAMPLUGIN;
        pipe->pluginData = TEAMD3D9PIPEID_STATIC;

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

                RxD3D9AllInOneSetLightingCallBack(plnode,
                                        _rxTeamD3D9AtomicLightingCallback);

                RxD3D9AllInOneSetRenderCallBack(plnode,
                                        _rpTeamD3D9StaticRenderCallback);

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

    pipe = TeamD3D9StaticPipelineCreate();

    if (pipe)
    {
        TeamStaticPipelineIndex = _rpTeamPipeAddPipeDefinition(pipe,
                                                      (TeamPipeOpen)NULL,
                                                      (TeamPipeClose)NULL);

        _rpTeamD3D9SetRenderPipeline(pipe, TEAMD3D9PIPEID_STATIC);

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

    _rpTeamD3D9SetRenderPipeline(NULL, TEAMD3D9PIPEID_STATIC);

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
