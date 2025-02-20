/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   pipes.c                                                    -*
 *-                                                                         -*
 *-  Purpose :   D3D8 lightmap pipeline extension                           -*
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

#include "d3d8.h"

/* internal driver functions */
extern RwBool _rwD3D8RWSetRasterStage(RwRaster *raster, RwUInt32 stage);

extern RwBool _rwD3D8RenderStateVertexAlphaEnable(RwBool enable);
extern RwBool _rwD3D8RenderStateIsVertexAlphaEnable(void);

extern RwBool _rwD3D8RenderStateSrcBlend(RwBlendFunction srcBlend);
extern RwBool _rwD3D8RenderStateDestBlend(RwBlendFunction dstBlend);

extern RwBool _rwD3D8TextureHasAlpha(const RwTexture *texture);
extern void   _rwD3D8RasterRemoveAlphaFlag(RwRaster *raster);

extern void   _rwD3D8RenderStateFlushCache(void);

extern LPDIRECT3DDEVICE8    _RwD3DDevice;

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
static RwUInt32 VideoCardMaxTextureBlendStages = 0;
static RwUInt32 VideoCardMaxTextures;

static RwBool   NeedToValidateTextureDiffuseAdd = TRUE;
static RwBool   VideoCardSupportsTextureDiffuseAdd = TRUE;

/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/

#define D3D8ATOMICPIPE 0
#define D3D8SECTORPIPE 1
#define D3D8NUMPIPES   2

#define _rpLtMapPipeline(_index) (_rpLtMapGlobals.platformPipes[_index])

/*===========================================================================*
 *--- Plugin Internal Platform-specific Functions ---------------------------*
 *===========================================================================*/

/****************************************************************************
 _rpLtMapD3D8GetLightmap
 */
static RwTexture *
_rpLtMapD3D8GetLightmap(void *object, RwUInt8 type)
{
    RwTexture   *lightMap = NULL;
    RwRaster    *lightMapRaster = NULL;

    RWFUNCTION(RWSTRING("_rpLtMapD3D8GetLightmap"));

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
    _rwD3D8RasterRemoveAlphaFlag(lightMapRaster);

    RWRETURN(lightMap);
}

/****************************************************************************
 _rpLtMapD3D8ApplyMaterialAlpha
 */
static void
_rpLtMapD3D8ApplyMaterialAlpha(const RwRGBA *color, RwTexture *baseTexture)
{
    RwUInt32 d3d8Color;

    RWFUNCTION(RWSTRING("_rpLtMapD3D8ApplyMaterialAlpha"));

    d3d8Color = ((RwUInt32)(color->alpha) << 24) |
                ((RwUInt32)(color->red) << 16) |
                ((RwUInt32)(color->green) << 8) |
                ((RwUInt32)(color->blue));

    RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, d3d8Color);

    if (baseTexture && _rwD3D8TextureHasAlpha(baseTexture))
    {
        RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
        RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
    }
    else
    {
        RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
        RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpLtMapD3D8RenderOnlyBaseTexture
 */
static void
_rpLtMapD3D8RenderOnlyBaseTexture(RwResEntry *repEntry, RwUInt32 flags, RwBool lighting)
{
    RxD3D8ResEntryHeader    *resEntryHeader;
    RxD3D8InstanceData      *instancedData;
    RwInt32                 numMeshes;
    void                    *lastVertexBuffer = (void *)0xffffffff;

    RWFUNCTION(RWSTRING("_rpLtMapD3D8RenderOnlyBaseTexture"));

    /* Get the instanced data */
    resEntryHeader = (RxD3D8ResEntryHeader *)(repEntry + 1);
    instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

    RwD3D8SetPixelShader(0);
    RwD3D8SetVertexShader(instancedData->vertexShader);

    /* Get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;

    /* Render */
    while (numMeshes--)
    {
        RwTexture *baseTexture = instancedData->material->texture;

        /* Set base texture */
        RwD3D8SetTexture(baseTexture, 0);

        /* Set blending params */
        if (instancedData->vertexAlpha ||
            (0xFF != instancedData->material->color.alpha))
        {
            if (!_rwD3D8RenderStateIsVertexAlphaEnable())
            {
                _rwD3D8RenderStateVertexAlphaEnable(TRUE);
            }
        }
        else
        {
            if (_rwD3D8RenderStateIsVertexAlphaEnable())
            {
                _rwD3D8RenderStateVertexAlphaEnable(FALSE);
            }
        }

        /* Set lighting material */
        if (lighting)
        {
            RwD3D8SetSurfaceProperties(&instancedData->material->color,
                                    &instancedData->material->surfaceProps,
                                    (flags & rxGEOMETRY_MODULATE));

            if (instancedData->vertexAlpha)
            {
                RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
            }
            else
            {
                RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
            }
        }
        else
        {
            if (0xFF != instancedData->material->color.alpha &&
                !instancedData->vertexAlpha)
            {
                _rpLtMapD3D8ApplyMaterialAlpha( &(instancedData->material->color),
                                                baseTexture);
            }
        }

        /* Set the stream source */
        if (lastVertexBuffer != instancedData->vertexBuffer)
        {
            RwD3D8SetStreamSource(0, instancedData->vertexBuffer, instancedData->stride);

            lastVertexBuffer = instancedData->vertexBuffer;
        }

        /* Draw geometry */
        if (instancedData->indexBuffer != NULL)
        {
            /* Set Indices */
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

    RWRETURNVOID();
}

/****************************************************************************
 _rpLtMapD3D8RenderOnlyLightmap
 */
static void
_rpLtMapD3D8RenderOnlyLightmap(RwResEntry *repEntry,
                               void *object,
                               RwUInt8 type,
                               RwUInt32 flags,
                               RwBool lighting)
{
    RwTexture               *lightMap;
    RxD3D8ResEntryHeader    *resEntryHeader;
    RxD3D8InstanceData      *instancedData;
    RwInt32                 numMeshes;
    void                    *lastVertexBuffer = (void *)0xffffffff;

    RWFUNCTION(RWSTRING("_rpLtMapD3D8RenderOnlyLightmap"));

    /* Set the lightmap */
    lightMap = _rpLtMapD3D8GetLightmap(object, type);

    RwD3D8SetTexture(lightMap, 0);

    if (lighting || (flags & rxGEOMETRY_PRELIT))
    {
        RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_ADD);
        RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    }
    else
    {
        RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
        RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    }

    RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);

    /* Get the instanced data */
    resEntryHeader = (RxD3D8ResEntryHeader *)(repEntry + 1);
    instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

    RwD3D8SetPixelShader(0);
    RwD3D8SetVertexShader(instancedData->vertexShader);

    /* Get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;

    /* Render */
    while (numMeshes--)
    {
        /* Set blending params */
        if (instancedData->vertexAlpha ||
            (0xFF != instancedData->material->color.alpha))
        {
            if (!_rwD3D8RenderStateIsVertexAlphaEnable())
            {
                _rwD3D8RenderStateVertexAlphaEnable(TRUE);
            }
        }
        else
        {
            if (_rwD3D8RenderStateIsVertexAlphaEnable())
            {
                _rwD3D8RenderStateVertexAlphaEnable(FALSE);
            }
        }

        /* Set lighting material */
        if (lighting)
        {
            RwD3D8SetSurfaceProperties(&instancedData->material->color,
                                    &instancedData->material->surfaceProps,
                                    (flags & rxGEOMETRY_MODULATE));

            if (instancedData->vertexAlpha)
            {
                RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
            }
            else
            {
                RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
            }
        }
        else
        {
            if (0xFF != instancedData->material->color.alpha &&
                !instancedData->vertexAlpha)
            {
                _rpLtMapD3D8ApplyMaterialAlpha( &(instancedData->material->color),
                                                NULL);
            }
        }

        /* Set the stream source */
        if (lastVertexBuffer != instancedData->vertexBuffer)
        {
            RwD3D8SetStreamSource(0, instancedData->vertexBuffer, instancedData->stride);

            lastVertexBuffer = instancedData->vertexBuffer;
        }

        /* Draw geometry */
        if (instancedData->indexBuffer != NULL)
        {
            /* Set Indices */
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

    RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);

    RWRETURNVOID();
}

/****************************************************************************
 _rpLtMapD3D8RenderWithLightmap
 */
static void
_rpLtMapD3D8RenderWithLightmap(RwResEntry *repEntry,
                               void *object,
                               RwUInt8 type,
                               RwUInt32 flags)
{
    RwBool                  fogEnabled;
    RwUInt32                fogColor;
    RwTexture               *lightMap;
    RxD3D8ResEntryHeader    *resEntryHeader;
    RxD3D8InstanceData      *instancedData;
    RwInt32                 numMeshes;
    void                    *lastVertexBuffer = (void *)0xffffffff;
    RwBool                  useDualPass = TRUE;

    RWFUNCTION(RWSTRING("_rpLtMapD3D8RenderWithLightmap"));

    /* Check if fog is enabled */
    RwD3D8GetRenderState(D3DRS_FOGENABLE, &fogEnabled);

    if (fogEnabled)
    {
        RwD3D8GetRenderState(D3DRS_FOGCOLOR, &fogColor);
    }

    /* Get the lightmap */
    lightMap = _rpLtMapD3D8GetLightmap(object, type);

    /* Get the instanced data */
    resEntryHeader = (RxD3D8ResEntryHeader *)(repEntry + 1);
    instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

    /* Set common data */
    RwD3D8SetPixelShader(0);
    RwD3D8SetVertexShader(instancedData->vertexShader);

    _rwD3D8RenderStateVertexAlphaEnable(FALSE);

    /* Get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;

    /* Render */
    while (numMeshes--)
    {
        RwTexture *baseTexture = instancedData->material->texture;

        RwD3D8SetTexture(baseTexture, 0);

        if (VideoCardMaxTextures > 1)
        {
            if ((RwRasterGetFormat(RwTextureGetRaster(lightMap)) & rwRASTERFORMATPAL8) != 0 &&
                baseTexture &&
                (RwRasterGetFormat(RwTextureGetRaster(baseTexture)) & rwRASTERFORMATPAL8) != 0 &&
                baseTexture != lightMap)
            {
                #if defined(RWDEBUG)
                static RwBool done = FALSE;

                if (!done)
                {
                    RwChar buff[256];

                    rwsprintf(buff,
                                "\n"
                                "\tD3D8 is unable to use more than one palettized texture at a time.\n"
                                "\tMulti-texture is not available, using multi-pass.\n"
                                "\tCheck textures '%s' and '%s'.",
                                baseTexture->name,
                                lightMap->name);

                    RwDebugSendMessage(rwDEBUGMESSAGE, "LightMap plugin", buff);

                    done = TRUE;
                }
                #endif

                if (!useDualPass)
                {
                    _rwD3D8RWSetRasterStage(NULL, 1);

                    useDualPass = TRUE;
                }
            }
            else
            {
                if (useDualPass)
                {
                    RwD3D8SetTexture(lightMap, 1);

                    useDualPass = FALSE;
                }

                if (0xFF != instancedData->material->color.alpha)
                {
                    RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                    RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
                }
                else
                {
                    if (baseTexture && _rwD3D8TextureHasAlpha(baseTexture))
                    {
                        RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                        RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
                    }
                    else
                    {
                        RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);
                    }
                }
            }
        }
        else
        {
            if (!useDualPass)
            {
                _rwD3D8RWSetRasterStage(NULL, 1);

                useDualPass = TRUE;
            }
        }

        /* Check material alpha */
        if (0xFF != instancedData->material->color.alpha)
        {
            if (!_rwD3D8RenderStateIsVertexAlphaEnable())
            {
                _rwD3D8RenderStateVertexAlphaEnable(TRUE);
            }

            _rpLtMapD3D8ApplyMaterialAlpha( &(instancedData->material->color),
                                            baseTexture);
        }
        else
        {
            if (_rwD3D8RenderStateIsVertexAlphaEnable())
            {
                _rwD3D8RenderStateVertexAlphaEnable(FALSE);
            }
        }

        /* Set the stream source */
        if (lastVertexBuffer != instancedData->vertexBuffer)
        {
            RwD3D8SetStreamSource(0, instancedData->vertexBuffer, instancedData->stride);

            lastVertexBuffer = instancedData->vertexBuffer;
        }

        /* Draw geometry */
        if (instancedData->indexBuffer != NULL)
        {
            /* Set Indices */
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

        /* Do we need dual pass */
        if (useDualPass)
        {
            RwBlendFunction srcBlend, destBlend;
            RwBool  zWriteEnable;

            RwRenderStateGet(rwRENDERSTATESRCBLEND, (void *)&srcBlend);
            RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void *)&destBlend);

            RwD3D8GetRenderState(D3DRS_ZWRITEENABLE, (void *)&zWriteEnable);
            RwD3D8SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

            RwD3D8SetTexture(lightMap, 0);

            RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
            RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

            RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);

            _rwD3D8RenderStateVertexAlphaEnable(TRUE);
            _rwD3D8RenderStateSrcBlend(rwBLENDZERO);
            _rwD3D8RenderStateDestBlend(rwBLENDSRCCOLOR);

            /* If fog enabled, use white color to emulate the correct result */
            if (fogEnabled)
            {
                RwD3D8SetRenderState(D3DRS_FOGCOLOR, 0xffffffff);
            }

            if (instancedData->indexBuffer != NULL)
            {
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

            if (fogEnabled)
            {
                RwD3D8SetRenderState(D3DRS_FOGCOLOR, fogColor);
            }

            RwD3D8SetRenderState(D3DRS_ZWRITEENABLE, zWriteEnable);

            RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);

            _rwD3D8RenderStateVertexAlphaEnable(FALSE);
            _rwD3D8RenderStateSrcBlend(srcBlend);
            _rwD3D8RenderStateDestBlend(destBlend);
        }

        /* Move onto the next instancedData */
        instancedData++;
    }

    if (!useDualPass)
    {
        _rwD3D8RWSetRasterStage(NULL, 1);
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpLtMapD3D8RenderWithLightmapVertexColor
 */
static void
_rpLtMapD3D8RenderWithLightmapVertexColor(RwResEntry *repEntry,
                                          void *object,
                                          RwUInt8 type,
                                          RwUInt32 flags,
                                          RwBool lighting)
{
    RwBool                  fogEnabled;
    RwUInt32                fogColor;
    RwTexture               *lightMap;
    RxD3D8ResEntryHeader    *resEntryHeader;
    RxD3D8InstanceData      *instancedData;
    RwInt32                 numMeshes;
    void                    *lastVertexBuffer = (void *)0xffffffff;
    RwBool                  useDualPass = TRUE;

    RWFUNCTION(RWSTRING("_rpLtMapD3D8RenderWithLightmapVertexColor"));

    /* Check if fog is enabled */
    RwD3D8GetRenderState(D3DRS_FOGENABLE, &fogEnabled);

    if (fogEnabled)
    {
        RwD3D8GetRenderState(D3DRS_FOGCOLOR, &fogColor);
    }

    /* Get the lightmap */
    lightMap = _rpLtMapD3D8GetLightmap(object, type);

    /* Get the instanced data */
    resEntryHeader = (RxD3D8ResEntryHeader *)(repEntry + 1);
    instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

    RwD3D8SetPixelShader(0);
    RwD3D8SetVertexShader(instancedData->vertexShader);

    /* Some old cards force you to use the diffuse channel in the last stage */
    if (NeedToValidateTextureDiffuseAdd)
    {
        if (VideoCardMaxTextureBlendStages <= 3)
        {
            RwUInt32    numPasses = 0;
            RwTexture   *baseTexture = instancedData->material->texture;

            RwD3D8SetTexture(lightMap, 0);
            RwD3D8SetTexture(baseTexture, 1);

            RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_ADD);
            RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

            RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);

            RwD3D8SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 0);

            /* Set alpha channel */
            if (_rwD3D8RenderStateIsVertexAlphaEnable())
            {
                RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

                if (baseTexture && _rwD3D8TextureHasAlpha(baseTexture))
                {
                    RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
                    RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
                    RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
                }
                else
                {
                    RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                    RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
                }
            }
            else
            {
                if (baseTexture && _rwD3D8TextureHasAlpha(baseTexture))
                {
                    /* This needs to be forced here */
                    _rwD3D8RenderStateVertexAlphaEnable(TRUE);

                    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
                    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

                    RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
                    RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
                }
                else
                {
                    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);
                    RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);
                }
            }

            _rwD3D8RenderStateFlushCache();

            if (FAILED(IDirect3DDevice8_ValidateDevice(_RwD3DDevice, (DWORD *)&numPasses)) ||
                numPasses != 1)
            {
                VideoCardSupportsTextureDiffuseAdd = FALSE;

                _rwD3D8RWSetRasterStage(NULL, 1);

                RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
                RwD3D8SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
            }
            else
            {
                VideoCardSupportsTextureDiffuseAdd = TRUE;
            }

            useDualPass = (VideoCardSupportsTextureDiffuseAdd == FALSE);
        }
        else
        {
            VideoCardSupportsTextureDiffuseAdd = TRUE;
        }

        NeedToValidateTextureDiffuseAdd = FALSE;
    }

    /* Get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;

    /* Render */
    while (numMeshes--)
    {
        RwTexture *baseTexture = instancedData->material->texture;

        /* Set vertex bleding modes (must be sone here) */
        if (instancedData->vertexAlpha ||
            (0xFF != instancedData->material->color.alpha))
        {
            if (!_rwD3D8RenderStateIsVertexAlphaEnable())
            {
                _rwD3D8RenderStateVertexAlphaEnable(TRUE);
            }
        }
        else
        {
            if (_rwD3D8RenderStateIsVertexAlphaEnable())
            {
                _rwD3D8RenderStateVertexAlphaEnable(FALSE);
            }
        }

        /* Set texture stages */
        if (VideoCardSupportsTextureDiffuseAdd)
        {
            if ((RwRasterGetFormat(RwTextureGetRaster(lightMap)) & rwRASTERFORMATPAL8) != 0 &&
                baseTexture &&
                (RwRasterGetFormat(RwTextureGetRaster(baseTexture)) & rwRASTERFORMATPAL8) != 0 &&
                baseTexture != lightMap)
            {
                #if defined(RWDEBUG)
                static RwBool done = FALSE;

                if (!done)
                {
                    RwChar buff[256];

                    rwsprintf(buff,
                                "\n"
                                "\tD3D8 is unable to use more than one palettized texture at a time.\n"
                                "\tMulti-texture is not available, using multi-pass.\n"
                                "\tCheck textures '%s' and '%s'.",
                                baseTexture->name,
                                lightMap->name);

                    RwDebugSendMessage(rwDEBUGMESSAGE, "LightMap plugin", buff);

                    done = TRUE;
                }
                #endif

                RwD3D8SetTexture(baseTexture, 0);

                if (!useDualPass)
                {
                    _rwD3D8RWSetRasterStage(NULL, 1);

                    RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);

                    useDualPass = TRUE;
                }
            }
            else
            {
                if (useDualPass)
                {
                    RwD3D8SetTexture(lightMap, 0);

                    RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_ADD);
                    RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                    RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

                    RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);

                    RwD3D8SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 0);

                    useDualPass = FALSE;
                }

                RwD3D8SetTexture(baseTexture, 1);

                /* Set alpha channel */
                if (_rwD3D8RenderStateIsVertexAlphaEnable())
                {
                    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

                    if (baseTexture && _rwD3D8TextureHasAlpha(baseTexture))
                    {
                        RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
                        RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
                        RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
                    }
                    else
                    {
                        RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                        RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
                    }
                }
                else
                {
                    if (baseTexture && _rwD3D8TextureHasAlpha(baseTexture))
                    {
                        /* This needs to be forced here */
                        _rwD3D8RenderStateVertexAlphaEnable(TRUE);

                        RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
                        RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

                        RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
                        RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
                    }
                    else
                    {
                        RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);
                        RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);
                    }
                }
            }
        }
        else
        {
            RwD3D8SetTexture(baseTexture, 0);
        }

        /* Prepare dual pass */
        if (useDualPass)
        {
            if (lighting)
            {
                RwD3D8SetRenderState(D3DRS_LIGHTING, FALSE);
            }

            RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
            RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

            if (0xFF != instancedData->material->color.alpha &&
                !instancedData->vertexAlpha)
            {
                _rpLtMapD3D8ApplyMaterialAlpha( &(instancedData->material->color),
                                                baseTexture);
            }
        }

        /* Set light material */
        if (lighting)
        {
            RwD3D8SetSurfaceProperties(&instancedData->material->color,
                                    &instancedData->material->surfaceProps,
                                    (flags & rxGEOMETRY_MODULATE));

            if (instancedData->vertexAlpha)
            {
                RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
            }
            else
            {
                RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
            }
        }

        /* Set the stream source */
        if (lastVertexBuffer != instancedData->vertexBuffer)
        {
            RwD3D8SetStreamSource(0, instancedData->vertexBuffer, instancedData->stride);

            lastVertexBuffer = instancedData->vertexBuffer;
        }

        /* Draw geometry */
        if (instancedData->indexBuffer != NULL)
        {
            /* Set Indices */
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

        /* Do we need dual pass */
        if (useDualPass)
        {
            RwBlendFunction srcBlend, destBlend;
            RwBool  zWriteEnable;

            RwRenderStateGet(rwRENDERSTATESRCBLEND, (void *)&srcBlend);
            RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void *)&destBlend);

            RwD3D8GetRenderState(D3DRS_ZWRITEENABLE, (void *)&zWriteEnable);
            RwD3D8SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

            if (lighting)
            {
                RwD3D8SetRenderState(D3DRS_LIGHTING, TRUE);
            }

            RwD3D8SetTexture(lightMap, 0);

            RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_ADD);
            RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

            RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);

            _rwD3D8RenderStateVertexAlphaEnable(TRUE);
            _rwD3D8RenderStateSrcBlend(rwBLENDZERO);
            _rwD3D8RenderStateDestBlend(rwBLENDSRCCOLOR);

            /* If fog enabled, use white color to emulate the correct result */
            if (fogEnabled)
            {
                RwD3D8SetRenderState(D3DRS_FOGCOLOR, 0xffffffff);
            }

            if (instancedData->indexBuffer != NULL)
            {
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

            if (fogEnabled)
            {
                RwD3D8SetRenderState(D3DRS_FOGCOLOR, fogColor);
            }

            RwD3D8SetRenderState(D3DRS_ZWRITEENABLE, zWriteEnable);

            RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);

            _rwD3D8RenderStateSrcBlend(srcBlend);
            _rwD3D8RenderStateDestBlend(destBlend);
        }

        /* Move onto the next instancedData */
        instancedData++;
    }

    _rwD3D8RWSetRasterStage(NULL, 1);

    RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
    RwD3D8SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);

    RWRETURNVOID();
}

/****************************************************************************
 _rpLtMapD3D8PipeCreate
 */
static void
_rpLtMapD3D8Callback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
    RpWorldSector           *sector;
    RpAtomic                *atomic;
    RwCamera                *camera;
    RwBool                  clipping = TRUE;
    RwBool                  lighting;

    RWFUNCTION(RWSTRING("_rpLtMapD3D8Callback"));

    /* Set clipping */
    camera = RwCameraGetCurrentCamera();
    RWASSERT(NULL != camera);
    if (type == (RwUInt8)rpATOMIC)
    {
        atomic = (RpAtomic *)object;
        if (RwD3D8CameraIsSphereFullyInsideFrustum(
                camera, RpAtomicGetWorldBoundingSphere(atomic)))
        {
            clipping = FALSE;
        }
    }
    else
    {
        sector = (RpWorldSector *)object;
        if (RwD3D8CameraIsBBoxFullyInsideFrustum(
                camera, RpWorldSectorGetBBox(sector)))
        {
            clipping = FALSE;
        }

        RWASSERT(type == (RwUInt8)rwSECTORATOMIC);
    }

    RwD3D8SetRenderState(D3DRS_CLIPPING, clipping);

    /* Get lighting state */
    RwD3D8GetRenderState(D3DRS_LIGHTING, &lighting);

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

    /* Choose render function */
    if (_rpLtMapGlobals.renderStyle & rpLTMAPSTYLERENDERLIGHTMAP)
    {
        if (_rpLtMapGlobals.renderStyle & rpLTMAPSTYLERENDERBASE)
        {
            if (lighting || (flags & rxGEOMETRY_PRELIT) != 0)
            {
                _rpLtMapD3D8RenderWithLightmapVertexColor(repEntry, object, type, flags, lighting);
            }
            else
            {
                _rpLtMapD3D8RenderWithLightmap(repEntry, object, type, flags);
            }
        }
        else
        {
            _rpLtMapD3D8RenderOnlyLightmap(repEntry, object, type, flags, lighting);
        }
    }
    else
    {
        _rpLtMapD3D8RenderOnlyBaseTexture(repEntry, flags, lighting);
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpLtMapD3D8PipeCreate
 */
static RxPipeline *
_rpLtMapD3D8PipeCreate(RwInt32 type)
{
    RxPipeline *pipe;

    RWFUNCTION(RWSTRING("_rpLtMapD3D8PipeCreate"));

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
                instanceNode = RxNodeDefinitionGetD3D8WorldSectorAllInOne();
            }
            else
            {
                instanceNode = RxNodeDefinitionGetD3D8AtomicAllInOne();
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
                RxD3D8AllInOneSetRenderCallBack(node, _rpLtMapD3D8Callback);

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

    pipeline = _rpLtMapPipeline(D3D8ATOMICPIPE);

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

    pipeline = _rpLtMapPipeline(D3D8SECTORPIPE);

    RWRETURN(pipeline);
}

/****************************************************************************
 _rpLtMapPlatformPipelinesDestroy
 */
void
_rpLtMapPlatformPipelinesDestroy(void)
{
    RWFUNCTION(RWSTRING("_rpLtMapPlatformPipelinesDestroy"));

    if (NULL != _rpLtMapPipeline(D3D8SECTORPIPE))
    {
        RxPipelineDestroy(_rpLtMapPipeline(D3D8SECTORPIPE));
         _rpLtMapPipeline(D3D8SECTORPIPE) = (RxPipeline *)NULL;
    }

    if (NULL != _rpLtMapPipeline(D3D8ATOMICPIPE))
    {
        RxPipelineDestroy(_rpLtMapPipeline(D3D8ATOMICPIPE));
         _rpLtMapPipeline(D3D8ATOMICPIPE) = (RxPipeline *)NULL;
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpLtMapPlatformPipelinesCreate
 */
RwBool
_rpLtMapPlatformPipelinesCreate(void)
{
    RWFUNCTION(RWSTRING("_rpLtMapPlatformPipelinesCreate"));

    RWASSERT(LTMAPMAXPIPES >= D3D8NUMPIPES);

    /* D3D8 atomic lightmap pipeline */
    _rpLtMapPipeline(D3D8ATOMICPIPE) = _rpLtMapD3D8PipeCreate(rpATOMIC);
    RWASSERT(NULL != _rpLtMapPipeline(D3D8ATOMICPIPE));

    /* D3D8 sector lightmap pipeline. */
    _rpLtMapPipeline(D3D8SECTORPIPE) = _rpLtMapD3D8PipeCreate(rwSECTORATOMIC);
    RWASSERT(NULL != _rpLtMapPipeline(D3D8SECTORPIPE));

    if ((NULL == _rpLtMapPipeline(D3D8ATOMICPIPE)) ||
        (NULL == _rpLtMapPipeline(D3D8SECTORPIPE)) )
    {
        _rpLtMapPlatformPipelinesDestroy();
        RWRETURN(FALSE);
    }

    /*
     * Get some video card capabilities
     */
    VideoCardMaxTextureBlendStages = ((const D3DCAPS8 *)RwD3D8GetCaps())->MaxTextureBlendStages;
    VideoCardMaxTextures = ((const D3DCAPS8 *)RwD3D8GetCaps())->MaxSimultaneousTextures;

    RWRETURN(TRUE);
}
