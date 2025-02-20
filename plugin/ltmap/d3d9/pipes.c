/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   pipes.c                                                    -*
 *-                                                                         -*
 *-  Purpose :   D3D9 lightmap pipeline extension                           -*
 *-                                                                         -*
 *===========================================================================*/

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpltmap.h"

#include "d3d9.h"

/* internal driver functions */
extern RwBool _rwD3D9RWSetRasterStage(RwRaster *raster, RwUInt32 stage);

extern RwBool _rwD3D9RenderStateVertexAlphaEnable(RwBool enable);

extern RwBool _rwD3D9RenderStateSrcBlend(RwBlendFunction srcBlend);
extern RwBool _rwD3D9RenderStateDestBlend(RwBlendFunction dstBlend);

extern RwBool _rwD3D9TextureHasAlpha(RwTexture *texture);
extern void   _rwD3D9RasterRemoveAlphaFlag(RwRaster *raster);

extern void   _rwD3D9RenderStateFlushCache(void);

extern LPDIRECT3DDEVICE9    _RwD3DDevice;

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
static RwUInt32 VideoCardMaxTextures;
static RwBool   VideoCardSupportsPixelShaders = FALSE;

static RwBool   NeedToValidateTextureDiffuseAdd = TRUE;
static RwBool   VideoCardSupportsTextureDiffuseAdd = TRUE;

static RwBool   NeedToValidateTextureDiffuseAddModulate = TRUE;
static RwBool   VideoCardSupportsTextureDiffuseAddModulate = TRUE;

/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/

#define D3D9ATOMICPIPE 0
#define D3D9SECTORPIPE 1
#define D3D9NUMPIPES   2

#define _rpLtMapPipeline(_index) (_rpLtMapGlobals.platformPipes[_index])

/*===========================================================================*
 *--- Plugin Internal Platform-specific Functions ---------------------------*
 *===========================================================================*/

/****************************************************************************
 _rpLtMapD3D9RenderOnlyBaseTexture
 */
static void
_rpLtMapD3D9RenderOnlyBaseTexture(RxD3D9ResEntryHeader *resEntryHeader, RwUInt32 flags, RwBool lighting)
{
    RxD3D9InstanceData      *instancedData;
    RwInt32                 numMeshes;
    RwTexture               *lastTexture;

    RWFUNCTION(RWSTRING("_rpLtMapD3D9RenderOnlyBaseTexture"));
	// 내부적으로 불리는 함수이다.. lock 호출 부분에서 이미 해줬음 2005.3.31 gemani

    instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

    /* force refresh */
    lastTexture = (RwTexture *)0xffffffff;

    /* Get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;

    /* Render */
    while (numMeshes--)
    {
        RwTexture *baseTexture = instancedData->material->texture;

        /* Set base texture */
        if (lastTexture != baseTexture)
        {
            if (baseTexture != NULL &&
                (flags & (rxGEOMETRY_TEXTURED | rxGEOMETRY_TEXTURED2)) != 0)
            {
                RwD3D9SetTexture(baseTexture, 0);

                if (lastTexture == NULL ||
                    lastTexture == (RwTexture *)0xffffffff)
                {
                    RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

                    if ((flags & rxGEOMETRY_MODULATE) != 0 &&
                        lighting == FALSE)
                    {
                        RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                        RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
                        RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_TFACTOR);

                        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
                        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
                        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
                    }
                }
            }
            else
            {
                RwD3D9SetTexture(NULL, 0);

                if ((flags & rxGEOMETRY_MODULATE) != 0 &&
                    lighting == FALSE)
                {
                    RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
                    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);

                    RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
                    RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);
                }
                else
                {
                    RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
                    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
                }
            }

            lastTexture = baseTexture;
        }

        /* Set blending params */
        if (instancedData->vertexAlpha ||
            (0xFF != instancedData->material->color.alpha))
        {
            _rwD3D9RenderStateVertexAlphaEnable(TRUE);
        }
        else
        {
            _rwD3D9RenderStateVertexAlphaEnable(FALSE);
        }

        /* Set lighting material */
        if (lighting)
        {
            RwD3D9SetSurfaceProperties(&instancedData->material->surfaceProps,
                                       &instancedData->material->color,
                                       flags);
        }
        else
        {
            if (flags & rxGEOMETRY_MODULATE)
            {
                const RwRGBA *matcolor = &(instancedData->material->color);
                const D3DCOLOR color =
                ((D3DCOLOR)((((RwUInt32)matcolor->alpha)<<24)|(((RwUInt32)matcolor->red)<<16)|(((RwUInt32)matcolor->green)<<8)|((RwUInt32)matcolor->blue)));

                RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, color);
            }
        }

        RwD3D9SetVertexShader(instancedData->vertexShader);

        /* Draw geometry */
        if (resEntryHeader->indexBuffer != NULL)
        {
            /* Draw the indexed primitive */
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
        ++instancedData;
    }

    RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
    RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);

    RWRETURNVOID();
}

/****************************************************************************
 _rpLtMapD3D9RenderOnlyLightmap
 */

static void
_rpLtMapD3D9RenderOnlyLightmap(RxD3D9ResEntryHeader *resEntryHeader,
                               RwTexture *lightMap,
                               RwUInt32 flags,
                               RwBool lighting)
{
    RxD3D9InstanceData      *instancedData;
    RwInt32                 numMeshes;

    RWFUNCTION(RWSTRING("_rpLtMapD3D9RenderOnlyLightmap"));
	// 내부적으로 불리는 함수이다.. lock 호출 부분에서 이미 해줬음 2005.3.31 gemani

    /* Set the lightmap */
    RwD3D9SetTexture(lightMap, 0);

    RwD3D9SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);

    if (lighting ||
        (flags & rxGEOMETRY_PRELIT) != 0)
    {
        RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_ADD);
        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

        if ((flags & rxGEOMETRY_MODULATE) != 0 &&
            lighting == FALSE)
        {
            RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
            RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_TFACTOR);

            RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
            RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
            RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
        }
    }
    else
    {
        if (flags & rxGEOMETRY_MODULATE)
        {
            RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
        }
        else
        {
            RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        }
    }

    /* Get the instanced data */
    instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

    /* Get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;

    /* Render */
    while (numMeshes--)
    {
        /* Set blending params */
        if (instancedData->vertexAlpha ||
            (0xFF != instancedData->material->color.alpha))
        {
            _rwD3D9RenderStateVertexAlphaEnable(TRUE);
        }
        else
        {
            _rwD3D9RenderStateVertexAlphaEnable(FALSE);
        }

        /* Set lighting material */
        if (lighting)
        {
            RwD3D9SetSurfaceProperties(&instancedData->material->surfaceProps,
                                       &instancedData->material->color,
                                       flags);
        }
        else
        {
            if (flags & rxGEOMETRY_MODULATE)
            {
                const RwRGBA *matcolor = &(instancedData->material->color);

                if (*((const RwUInt32 *)matcolor) != 0xffffffff)
                {
                    const D3DCOLOR color =
                    ((D3DCOLOR)((((RwUInt32)matcolor->alpha)<<24)|(((RwUInt32)matcolor->red)<<16)|(((RwUInt32)matcolor->green)<<8)|((RwUInt32)matcolor->blue)));

                    RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, color);
                }
                else
                {
                    RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, 0xffffffff);
                }
            }
        }

        RwD3D9SetVertexShader(instancedData->vertexShader);

        /* Draw geometry */
        if (resEntryHeader->indexBuffer != NULL)
        {
            /* Draw the indexed primitive */
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
        ++instancedData;
    }

    RwD3D9SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);

    RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
    RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);

    RWRETURNVOID();
}

/****************************************************************************
 _rpLtMapD3D9RenderWithLightmap
 */
static void
_rpLtMapD3D9RenderWithLightmap(RxD3D9ResEntryHeader *resEntryHeader,
                               RwTexture *lightMap,
                               RwUInt32 flags)
{
    RwBool                  paletteLightmap;
    RxD3D9InstanceData      *instancedData;
    RwInt32                 numMeshes;

    RWFUNCTION(RWSTRING("_rpLtMapD3D9RenderWithLightmap"));
	// 내부적으로 불리는 함수이다.. lock 호출 부분에서 이미 해줬음 2005.3.31 gemani

    if (RwRasterGetFormat(RwTextureGetRaster(lightMap)) & rwRASTERFORMATPAL8)
    {
        paletteLightmap = TRUE;
    }
    else
    {
        paletteLightmap = FALSE;
    }

    /* Get the instanced data */
    instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

    /* Get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;

    /* Render */
    RwD3D9SetTexture(lightMap, 1);

    if (flags & rxGEOMETRY_MODULATE)
    {
        RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
    }
    else
    {
        RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    }

    RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE);
    RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

    RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
    RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

    while (numMeshes--)
    {
        RwTexture *baseTexture = instancedData->material->texture;

        RwD3D9SetTexture(baseTexture, 0);

        if (paletteLightmap &&
            baseTexture &&
            (RwRasterGetFormat(RwTextureGetRaster(baseTexture)) & rwRASTERFORMATPAL8) != 0 &&
            baseTexture != lightMap)
        {
            #if defined(RWDEBUG)
            RwChar buff[256];

            rwsprintf(buff,
                        "\n"
                        "\tD3D9 is unable to use more than one palettized texture at a time.\n"
                        "\tCheck textures '%s' and '%s'.",
                        baseTexture->name,
                        lightMap->name);

            RwDebugSendMessage(rwDEBUGMESSAGE, "LightMap plugin", buff);
            #endif

            /* Remove texture to convert */
            RwD3D9SetTexture(NULL, 1);

            /* Convert to non-palettized */
            _rwD3D9RasterConvertToNonPalettized(RwTextureGetRaster(lightMap));

            paletteLightmap = FALSE;

            /* Reset raster */
            RwD3D9SetTexture(lightMap, 1);
        }

        if (flags & rxGEOMETRY_MODULATE)
        {
            const RwRGBA *matcolor = &(instancedData->material->color);

            if (*((const RwUInt32 *)matcolor) != 0xffffffff)
            {
                const D3DCOLOR color =
                ((D3DCOLOR)((((RwUInt32)matcolor->alpha)<<24)|(((RwUInt32)matcolor->red)<<16)|(((RwUInt32)matcolor->green)<<8)|((RwUInt32)matcolor->blue)));

                RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, color);
            }
            else
            {
                RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, 0xffffffff);
            }
        }

        /* Check material alpha */
        if (0xFF != instancedData->material->color.alpha)
        {
            _rwD3D9RenderStateVertexAlphaEnable(TRUE);
        }
        else
        {
            _rwD3D9RenderStateVertexAlphaEnable(FALSE);
        }

        RwD3D9SetVertexShader(instancedData->vertexShader);

        /* Draw geometry */
        if (resEntryHeader->indexBuffer != NULL)
        {
            /* Draw the indexed primitive */
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
        ++instancedData;
    }

    RwD3D9SetTexture(NULL, 1);

    RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
    RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);

    RWRETURNVOID();
}

/****************************************************************************
 _rpLtMapD3D9RenderWithLightmapDualPass
 */
static void
_rpLtMapD3D9RenderWithLightmapDualPass(RxD3D9ResEntryHeader *resEntryHeader,
                                       RwTexture *lightMap,
                                       RwUInt32 flags)
{
    RwBool              fogEnabled;
    RwUInt32            fogColor;
    RxD3D9InstanceData  *instancedData;
    RwInt32             numMeshes;
    RwBlendFunction     srcBlend, destBlend;
    RwBool              zWriteEnable;

    RWFUNCTION(RWSTRING("_rpLtMapD3D9RenderWithLightmapDualPass"));
	// 내부적으로 불리는 함수이다.. lock 호출 부분에서 이미 해줬음 2005.3.31 gemani

    RwD3D9GetRenderState(D3DRS_FOGENABLE, &fogEnabled);

    if (fogEnabled)
    {
        RwD3D9GetRenderState(D3DRS_FOGCOLOR, &fogColor);
    }

    /* Get the instanced data */
    instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

    /* Get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;

    while (numMeshes--)
    {
        RwTexture *baseTexture = instancedData->material->texture;
        const RwRGBA *matcolor = &(instancedData->material->color);

        RwD3D9SetVertexShader(instancedData->vertexShader);

        /*
         * First Pass
         */
        RwD3D9SetTexture(baseTexture, 0);

        if ( (flags & rxGEOMETRY_MODULATE) != 0 &&
             *((const RwUInt32 *)matcolor) != 0xffffffff)
        {
            const D3DCOLOR color =
            ((D3DCOLOR)((((RwUInt32)matcolor->alpha)<<24)|(((RwUInt32)matcolor->red)<<16)|(((RwUInt32)matcolor->green)<<8)|((RwUInt32)matcolor->blue)));

            RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, color);

            RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
        }
        else
        {
            RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        }

        /* Check material alpha */
        if (0xFF != instancedData->material->color.alpha)
        {
            _rwD3D9RenderStateVertexAlphaEnable(TRUE);
        }
        else
        {
            _rwD3D9RenderStateVertexAlphaEnable(FALSE);
        }

        /* Draw geometry */
        if (resEntryHeader->indexBuffer != NULL)
        {
            /* Draw the indexed primitive */
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

        /*
         * Second pass
         */
        RwD3D9SetTexture(lightMap, 0);

        RwRenderStateGet(rwRENDERSTATESRCBLEND, (void *)&srcBlend);
        RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void *)&destBlend);

        RwD3D9GetRenderState(D3DRS_ZWRITEENABLE, (void *)&zWriteEnable);
        RwD3D9SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

        _rwD3D9RenderStateVertexAlphaEnable(TRUE);
        _rwD3D9RenderStateSrcBlend(rwBLENDZERO);
        _rwD3D9RenderStateDestBlend(rwBLENDSRCCOLOR);

        /* If fog enabled, use white color to emulate the correct result */
        if (fogEnabled)
        {
            RwD3D9SetRenderState(D3DRS_FOGCOLOR, 0xffffffff);
        }

        RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

        RwD3D9SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);

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

        RwD3D9SetRenderState(D3DRS_ZWRITEENABLE, zWriteEnable);

        _rwD3D9RenderStateSrcBlend(srcBlend);
        _rwD3D9RenderStateDestBlend(destBlend);

        if (fogEnabled)
        {
            RwD3D9SetRenderState(D3DRS_FOGCOLOR, fogColor);
        }

        RwD3D9SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);

        /* Move onto the next instancedData */
        ++instancedData;
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpLtMapD3D9RenderWithLightmapVertexColor
 */
static void
_rpLtMapD3D9RenderWithLightmapVertexColor(RxD3D9ResEntryHeader *resEntryHeader,
                                          RwTexture *lightMap,
                                          RwUInt32 flags,
                                          RwBool lighting)
{
    RwBool                  paletteLightmap;
    RxD3D9InstanceData      *instancedData;
    RwInt32                 numMeshes;

    RWFUNCTION(RWSTRING("_rpLtMapD3D9RenderWithLightmapVertexColor"));
	// 내부적으로 불리는 함수이다.. lock 호출 부분에서 이미 해줬음 2005.3.31 gemani

    if (RwRasterGetFormat(RwTextureGetRaster(lightMap)) & rwRASTERFORMATPAL8)
    {
        paletteLightmap = TRUE;
    }
    else
    {
        paletteLightmap = FALSE;
    }

    RwD3D9SetTexture(lightMap, 0);

    RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_ADD);
    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE);
    RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

    RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
    RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

    if ((flags & rxGEOMETRY_MODULATE) != 0 &&
        lighting == FALSE)
    {
        RwD3D9SetTextureStageState(2, D3DTSS_COLOROP,   D3DTOP_MODULATE);
        RwD3D9SetTextureStageState(2, D3DTSS_COLORARG1, D3DTA_TFACTOR);
        RwD3D9SetTextureStageState(2, D3DTSS_COLORARG2, D3DTA_CURRENT);

        RwD3D9SetTextureStageState(2, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
        RwD3D9SetTextureStageState(2, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
        RwD3D9SetTextureStageState(2, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
    }
    else
    {
        RwD3D9SetTextureStageState(2, D3DTSS_COLOROP,   D3DTOP_DISABLE);
        RwD3D9SetTextureStageState(2, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);
    }

    RwD3D9SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);
    RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 0);

    /* Get the instanced data */
    instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

    numMeshes = resEntryHeader->numMeshes;
    while (numMeshes--)
    {
        RwTexture *baseTexture = instancedData->material->texture;

        /* Set vertex bleding modes (must be sone here) */
        if ( instancedData->vertexAlpha ||
                (0xFF != instancedData->material->color.alpha) ||
                (baseTexture != NULL && _rwD3D9TextureHasAlpha(baseTexture)) )
        {
            _rwD3D9RenderStateVertexAlphaEnable(TRUE);
        }
        else
        {
            _rwD3D9RenderStateVertexAlphaEnable(FALSE);
        }

        /* Set texture stages */
        if (paletteLightmap &&
            baseTexture &&
            (RwRasterGetFormat(RwTextureGetRaster(baseTexture)) & rwRASTERFORMATPAL8) != 0 &&
            baseTexture != lightMap)
        {
            #if defined(RWDEBUG)
            RwChar buff[256];

            rwsprintf(buff,
                        "\n"
                        "\tD3D9 is unable to use more than one palettized texture at a time.\n"
                        "\tCheck textures '%s' and '%s'.",
                        baseTexture->name,
                        lightMap->name);

            RwDebugSendMessage(rwDEBUGMESSAGE, "LightMap plugin", buff);
            #endif

            /* Remove texture to convert */
            RwD3D9SetTexture(NULL, 0);

            /* Convert to non-palettized */
            _rwD3D9RasterConvertToNonPalettized(RwTextureGetRaster(lightMap));

            paletteLightmap = FALSE;

            /* Reset raster */
            RwD3D9SetTexture(lightMap, 0);
        }

        RwD3D9SetTexture(baseTexture, 1);

        /* Set light material */
        if (lighting)
        {
            RwD3D9SetSurfaceProperties(&instancedData->material->surfaceProps,
                                    &instancedData->material->color,
                                    flags);
        }
        else
        {
            if (flags & rxGEOMETRY_MODULATE)
            {
                const RwRGBA *matcolor = &(instancedData->material->color);
                if (*((const RwUInt32 *)matcolor) != 0xffffffff)
                {
                    const D3DCOLOR color =
                    ((D3DCOLOR)((((RwUInt32)matcolor->alpha)<<24)|(((RwUInt32)matcolor->red)<<16)|(((RwUInt32)matcolor->green)<<8)|((RwUInt32)matcolor->blue)));

                    RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, color);
                }
                else
                {
                    RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, 0xffffffff);
                }
            }
        }

        RwD3D9SetVertexShader(instancedData->vertexShader);

        /* Draw geometry */
        if (resEntryHeader->indexBuffer != NULL)
        {
            /* Draw the indexed primitive */
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
        ++instancedData;
    }

    RwD3D9SetTexture(NULL, 1);

    RwD3D9SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
    RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);

    RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
    RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);

    RwD3D9SetTextureStageState(2, D3DTSS_COLOROP,   D3DTOP_DISABLE);
    RwD3D9SetTextureStageState(2, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);

    RWRETURNVOID();
}

/****************************************************************************
 _rpLtMapD3D9RenderWithLightmapVertexColorDualPass
 */
static void
_rpLtMapD3D9RenderWithLightmapVertexColorDualPass(RxD3D9ResEntryHeader *resEntryHeader,
                                                  RwTexture *lightMap,
                                                  RwUInt32 flags,
                                                  RwBool lighting)
{
    RwBool              fogEnabled;
    RwUInt32            fogColor;
    RxD3D9InstanceData  *instancedData;
    RwInt32             numMeshes;
    RwBlendFunction     srcBlend, destBlend;
    RwBool              zWriteEnable;

    RWFUNCTION(RWSTRING("_rpLtMapD3D9RenderWithLightmapVertexColorDualPass"));
	// 내부적으로 불리는 함수이다.. lock 호출 부분에서 이미 해줬음 2005.3.31 gemani

    RwD3D9GetRenderState(D3DRS_FOGENABLE, &fogEnabled);

    if (fogEnabled)
    {
        RwD3D9GetRenderState(D3DRS_FOGCOLOR, &fogColor);
    }

    /* Get the instanced data */
    instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

    /* Render */
    numMeshes = resEntryHeader->numMeshes;
    while (numMeshes--)
    {
        RwTexture *baseTexture = instancedData->material->texture;

        RwD3D9SetVertexShader(instancedData->vertexShader);

        /* Set vertex bleding modes (must be done here) */
        if (instancedData->vertexAlpha ||
            (0xFF != instancedData->material->color.alpha))
        {
            _rwD3D9RenderStateVertexAlphaEnable(TRUE);
        }
        else
        {
            _rwD3D9RenderStateVertexAlphaEnable(FALSE);
        }

        /* Set light material */
        if (lighting)
        {
            RwD3D9SetSurfaceProperties(&instancedData->material->surfaceProps,
                                    &instancedData->material->color,
                                    flags);
        }
        else
        {
            if (flags & rxGEOMETRY_MODULATE)
            {
                const RwRGBA *matcolor = &(instancedData->material->color);
                if (*((const RwUInt32 *)matcolor) != 0xffffffff)
                {
                    const D3DCOLOR color =
                    ((D3DCOLOR)((((RwUInt32)matcolor->alpha)<<24)|(((RwUInt32)matcolor->red)<<16)|(((RwUInt32)matcolor->green)<<8)|((RwUInt32)matcolor->blue)));

                    RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, color);
                }
                else
                {
                    RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, 0xffffffff);
                }
            }
        }

        /*
         * First pass
         */
        RwD3D9SetTexture(baseTexture, 0);

        if (lighting)
        {
            RwD3D9SetRenderState(D3DRS_LIGHTING, FALSE);
        }

        RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

        if ((flags & rxGEOMETRY_MODULATE) != 0 &&
            lighting == FALSE)
        {
            RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TFACTOR);
            RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

            RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
            RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
            RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        }
        else
        {
            RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
            RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);
        }

        /* Draw geometry */
        if (resEntryHeader->indexBuffer != NULL)
        {
            /* Draw the indexed primitive */
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

        /*
         * Second pass
         */
        RwD3D9SetTexture(lightMap, 0);

        RwRenderStateGet(rwRENDERSTATESRCBLEND, (void *)&srcBlend);
        RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void *)&destBlend);

        RwD3D9GetRenderState(D3DRS_ZWRITEENABLE, (void *)&zWriteEnable);
        RwD3D9SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

        if (lighting)
        {
            RwD3D9SetRenderState(D3DRS_LIGHTING, TRUE);
        }

        _rwD3D9RenderStateVertexAlphaEnable(TRUE);
        _rwD3D9RenderStateSrcBlend(rwBLENDZERO);
        _rwD3D9RenderStateDestBlend(rwBLENDSRCCOLOR);

        /* If fog enabled, use white color to emulate the correct result */
        if (fogEnabled)
        {
            RwD3D9SetRenderState(D3DRS_FOGCOLOR, 0xffffffff);
        }

        RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_ADD);
        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

        RwD3D9SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);

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

        if (fogEnabled)
        {
            RwD3D9SetRenderState(D3DRS_FOGCOLOR, fogColor);
        }

        RwD3D9SetRenderState(D3DRS_ZWRITEENABLE, zWriteEnable);

        _rwD3D9RenderStateSrcBlend(srcBlend);
        _rwD3D9RenderStateDestBlend(destBlend);

        RwD3D9SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);

        /* Move onto the next instancedData */
        ++instancedData;
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpLtMapD3D9GetLightmap
 */
static RwTexture *
_rpLtMapD3D9GetLightmap(void *object, RwUInt8 type)
{
    RwTexture   *lightMap = NULL;
    RwRaster    *lightMapRaster = NULL;

    RWFUNCTION(RWSTRING("_rpLtMapD3D9GetLightmap"));

    /* Get the current object's lightmap */
    if (type == (RwUInt8)rpATOMIC)
    {
        lightMap = RpLtMapAtomicGetLightMap((RpAtomic *)object);
    }
    else
    {
        lightMap = RpLtMapWorldSectorGetLightMap((RpWorldSector *)object);
    }

    /* Be sure that the lightmap don't use it's alpha channel */
    lightMapRaster = RwTextureGetRaster(lightMap);
    _rwD3D9RasterRemoveAlphaFlag(lightMapRaster);

    RWRETURN(lightMap);
}

/****************************************************************************
 _rpLtMapD3D9ValidateTextureDiffuseAdd
 */
static RwBool
_rpLtMapD3D9ValidateTextureDiffuseAdd(RxD3D9ResEntryHeader *resEntryHeader,
                                      RwTexture *lightMap,
                                      RwBool modulate)
{
    RwBool  supported = TRUE;

    RWFUNCTION(RWSTRING("_rpLtMapD3D9ValidateTextureDiffuseAdd"));

    if (VideoCardSupportsPixelShaders == FALSE)
    {
        const RxD3D9InstanceData    *instancedData;
        RwUInt32                    numPasses;

        /* Get the instanced data */
        instancedData = (const RxD3D9InstanceData *)(resEntryHeader + 1);

        RwD3D9SetTexture(lightMap, 0);
        RwD3D9SetTexture(instancedData->material->texture, 1);

        RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_ADD);
        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

        RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE);
        RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

        if (modulate)
        {
            RwD3D9SetTextureStageState(2, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            RwD3D9SetTextureStageState(2, D3DTSS_COLORARG1, D3DTA_TFACTOR);
            RwD3D9SetTextureStageState(2, D3DTSS_COLORARG2, D3DTA_CURRENT);

            RwD3D9SetTextureStageState(2, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
            RwD3D9SetTextureStageState(2, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
            RwD3D9SetTextureStageState(2, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        }

        RwD3D9SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);

        RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 0);

        _rwD3D9RenderStateFlushCache();

        numPasses = 0;
        if (FAILED(IDirect3DDevice9_ValidateDevice(_RwD3DDevice, (DWORD *)&numPasses)) ||
            numPasses != 1)
        {
            supported = FALSE;
        }

        RwD3D9SetTexture(NULL, 1);

        RwD3D9SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
        RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);

        RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);

        RwD3D9SetTextureStageState(2, D3DTSS_COLOROP,   D3DTOP_DISABLE);
        RwD3D9SetTextureStageState(2, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);
    }

    RWRETURN(supported);
}

/****************************************************************************
 _rpLtMapD3D9PipeCreate
 */
static void
_rpLtMapD3D9Callback(RwResEntry *repEntry,
                     void *object,
                     RwUInt8 type,
                     RwUInt32 flags)
{
    RwTexture               *lightMap;
    RxD3D9ResEntryHeader    *resEntryHeader;
    RwBool                  lighting;

    RWFUNCTION(RWSTRING("_rpLtMapD3D9Callback"));
	
    /* Get lightmap */
    lightMap = _rpLtMapD3D9GetLightmap(object, type);

    /* Set clipping */
    _rwD3D9EnableClippingIfNeeded(object, type);

    /* Default pixel shader */
    RwD3D9SetPixelShader(NULL);

    /* Get the instanced data */
    resEntryHeader = (RxD3D9ResEntryHeader *)(repEntry + 1);

	//@{ 20050513 DDonSS : Threadsafe
	// ResEntry Lock
	CS_RESENTRYHEADER_LOCK( resEntryHeader );
	//@} DDonSS
	
	//>@ 2005.3.31 gemani
	if(!resEntryHeader->isLive)					//validation check
	{
		//@{ 20050513 DDonSS : Threadsafe
		// ResEntry Unlock
		CS_RESENTRYHEADER_UNLOCK( resEntryHeader );
		//@} DDonSS

		return;		
	}
	//<@

    /* Set Indices */
    if (resEntryHeader->indexBuffer != NULL)
    {
        RwD3D9SetIndices(resEntryHeader->indexBuffer);
    }

    /* Set the stream source */
    _rwD3D9SetStreams(resEntryHeader->vertexStream,
                      resEntryHeader->useOffsets);

    /* Set vertex declaration */
    RwD3D9SetVertexDeclaration(resEntryHeader->vertexDeclaration);

    /* Get lighting state */
    RwD3D9GetRenderState(D3DRS_LIGHTING, &lighting);

    /* Choose render function */
    if (_rpLtMapGlobals.renderStyle & rpLTMAPSTYLERENDERLIGHTMAP)
    {
        if (_rpLtMapGlobals.renderStyle & rpLTMAPSTYLERENDERBASE)
        {
            if (lighting || (flags & rxGEOMETRY_PRELIT) != 0)
            {
                /* Some old cards force you to use the diffuse channel in the last stage */
                if ((flags & rxGEOMETRY_MODULATE) != 0 && lighting == FALSE)
                {
                    if (NeedToValidateTextureDiffuseAddModulate)
                    {
                        VideoCardSupportsTextureDiffuseAddModulate =
                            _rpLtMapD3D9ValidateTextureDiffuseAdd(resEntryHeader, lightMap, TRUE);

                        NeedToValidateTextureDiffuseAddModulate = FALSE;
                    }

                    if (VideoCardSupportsTextureDiffuseAddModulate)
                    {
                        _rpLtMapD3D9RenderWithLightmapVertexColor(resEntryHeader, lightMap, flags, lighting);
                    }
                    else
                    {
                        _rpLtMapD3D9RenderWithLightmapVertexColorDualPass(resEntryHeader, lightMap, flags, lighting);
                    }
                }
                else
                {
                    if (NeedToValidateTextureDiffuseAdd)
                    {
                        VideoCardSupportsTextureDiffuseAdd =
                            _rpLtMapD3D9ValidateTextureDiffuseAdd(resEntryHeader, lightMap, FALSE);

                        NeedToValidateTextureDiffuseAdd = FALSE;
                    }

                    if (VideoCardSupportsTextureDiffuseAdd)
                    {
                        _rpLtMapD3D9RenderWithLightmapVertexColor(resEntryHeader, lightMap, flags, lighting);
                    }
                    else
                    {
                        _rpLtMapD3D9RenderWithLightmapVertexColorDualPass(resEntryHeader, lightMap, flags, lighting);
                    }
                }
            }
            else
            {
                if (VideoCardMaxTextures > 1)
                {
                    _rpLtMapD3D9RenderWithLightmap(resEntryHeader, lightMap, flags);
                }
                else
                {
                    _rpLtMapD3D9RenderWithLightmapDualPass(resEntryHeader, lightMap, flags);
                }
            }
        }
        else
        {
            _rpLtMapD3D9RenderOnlyLightmap(resEntryHeader, lightMap, flags, lighting);
        }
    }
    else
    {
        _rpLtMapD3D9RenderOnlyBaseTexture(resEntryHeader, flags, lighting);
    }

	//@{ 20050513 DDonSS : Threadsafe
	// ResEntry Unlock
	CS_RESENTRYHEADER_UNLOCK( resEntryHeader );
	//@} DDonSS

    RWRETURNVOID();
}

/****************************************************************************
 _rpLtMapD3D9PipeCreate
 */
static RxPipeline *
_rpLtMapD3D9PipeCreate(RwInt32 type)
{
    RxPipeline *pipe;

    RWFUNCTION(RWSTRING("_rpLtMapD3D9PipeCreate"));

    pipe = RxPipelineCreate();
    if (NULL != pipe)
    {
        RxPipelineNode   *node;
        RxLockedPipe     *lpipe;

        pipe->pluginId = rwID_LTMAPPLUGIN;

        lpipe = RxPipelineLock(pipe);
        if (NULL != lpipe)
        {
            RxNodeDefinition *instanceNode;

            /* Get the appropriate instance node definition */
            if (type == rwSECTORATOMIC)
            {
                instanceNode = RxNodeDefinitionGetD3D9WorldSectorAllInOne();
            }
            else
            {
                instanceNode = RxNodeDefinitionGetD3D9AtomicAllInOne();
                RWASSERT(type == rpATOMIC);
            }

            /* Add the node to the pipeline */
            lpipe = RxLockedPipeAddFragment(lpipe, NULL, instanceNode, NULL);
            RWASSERT(NULL != lpipe);

            /* Unlock the pipeline */
            lpipe = RxLockedPipeUnlock(lpipe);

            if (NULL != lpipe)
            {
                pipe = lpipe;

                /* Add our callback */
                node = RxPipelineFindNodeByName(pipe, instanceNode->name, NULL, NULL);
                RxD3D9AllInOneSetRenderCallBack(node, _rpLtMapD3D9Callback);

                RWRETURN(pipe);
            }
            RxPipelineDestroy(pipe);
        }
        pipe = (RxPipeline *)NULL;
    }

    RWRETURN(pipe);
}

/****************************************************************************
 _rpLtMapGetPlatformAtomicPipeline
 */
RxPipeline *
_rpLtMapGetPlatformAtomicPipeline(void)
{
    RxPipeline *pipeline;

    RWFUNCTION(RWSTRING("_rpLtMapGetPlatformAtomicPipeline"));
    RWASSERT(0 < _rpLtMapGlobals.module.numInstances);

    pipeline = _rpLtMapPipeline(D3D9ATOMICPIPE);

    RWRETURN(pipeline);
}

/****************************************************************************
 _rpLtMapGetPlatformWorldSectorPipeline
 */
RxPipeline *
_rpLtMapGetPlatformWorldSectorPipeline(void)
{
    RxPipeline *pipeline;

    RWFUNCTION(RWSTRING("_rpLtMapGetPlatformWorldSectorPipeline"));
    RWASSERT(0 < _rpLtMapGlobals.module.numInstances);

    pipeline = _rpLtMapPipeline(D3D9SECTORPIPE);

    RWRETURN(pipeline);
}

/****************************************************************************
 _rpLtMapPlatformPipelinesDestroy
 */
void
_rpLtMapPlatformPipelinesDestroy(void)
{
    RWFUNCTION(RWSTRING("_rpLtMapPlatformPipelinesDestroy"));

    if (NULL != _rpLtMapPipeline(D3D9SECTORPIPE))
    {
        RxPipelineDestroy(_rpLtMapPipeline(D3D9SECTORPIPE));
         _rpLtMapPipeline(D3D9SECTORPIPE) = (RxPipeline *)NULL;
    }

    if (NULL != _rpLtMapPipeline(D3D9ATOMICPIPE))
    {
        RxPipelineDestroy(_rpLtMapPipeline(D3D9ATOMICPIPE));
         _rpLtMapPipeline(D3D9ATOMICPIPE) = (RxPipeline *)NULL;
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpLtMapPlatformPipelinesCreate
 */
RwBool
_rpLtMapPlatformPipelinesCreate(void)
{
    const D3DCAPS9 *d3dCaps;

    RWFUNCTION(RWSTRING("_rpLtMapPlatformPipelinesCreate"));

    RWASSERT(LTMAPMAXPIPES >= D3D9NUMPIPES);

    /* D3D9 atomic lightmap pipeline */
    _rpLtMapPipeline(D3D9ATOMICPIPE) = _rpLtMapD3D9PipeCreate(rpATOMIC);
    RWASSERT(NULL != _rpLtMapPipeline(D3D9ATOMICPIPE));

    /* D3D9 sector lightmap pipeline. */
    _rpLtMapPipeline(D3D9SECTORPIPE) = _rpLtMapD3D9PipeCreate(rwSECTORATOMIC);
    RWASSERT(NULL != _rpLtMapPipeline(D3D9SECTORPIPE));

    if ((NULL == _rpLtMapPipeline(D3D9ATOMICPIPE)) ||
        (NULL == _rpLtMapPipeline(D3D9SECTORPIPE)) )
    {
        _rpLtMapPlatformPipelinesDestroy();
        RWRETURN(FALSE);
    }

    /*
     * Get some video card capabilities
     */
    d3dCaps = (const D3DCAPS9 *)RwD3D9GetCaps();
    VideoCardMaxTextures = d3dCaps->MaxSimultaneousTextures;

    VideoCardSupportsPixelShaders = (((d3dCaps->PixelShaderVersion) & 0xffff) >= 0x0101);

    RWRETURN(TRUE);
}
