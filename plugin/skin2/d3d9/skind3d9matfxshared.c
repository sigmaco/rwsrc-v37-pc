#include <d3d9.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpmatfx.h"
#include "..\\..\\matfx\\effectPipes.h"

#include "skin.h"

#include "skind3d9.h"
#include "skind3d9matfx.h"

#define NUMCLUSTERSOFINTEREST   0
#define NUMOUTPUTS              0

extern void
_rpD3D9SkinVertexShaderBeginCallBack(void *object,
                                     RwUInt32 type,
                                     _rpD3D9VertexShaderDescriptor  *desc);

/****************************************************************************
 _rpD3D9SkinVertexShaderMatFXGetMaterialShaderCallBack
 */
static void *
_rpD3D9SkinVertexShaderMatFXGetMaterialShaderCallBack(const RpMaterial *material,
                                                 _rpD3D9VertexShaderDescriptor *desc,
                                                 _rpD3D9VertexShaderDispatchDescriptor *dispatch)
{
    const MatFXEnvMapData       *envMapData;

    RWFUNCTION(RWSTRING("_rpD3D9SkinVertexShaderMatFXGetMaterialShaderCallBack"));
    RWASSERT(material != NULL);
    RWASSERT(desc != NULL);
    RWASSERT(dispatch != NULL);

    desc->effect = 0;
/*
    if (desc->numDirectionalLights ||
        desc->numPointLights ||
        desc->numSpotLights ||
        desc->prelit ||
        )
*/
    {
        const rpMatFXMaterialData   *matFXData;
        RpMatFXMaterialFlags        effectType;

        matFXData = *MATFXMATERIALGETCONSTDATA(material);
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

        switch (effectType)
        {
            case rpMATFXEFFECTBUMPMAP:
                {
                    if (desc->numTexCoords > 0)
                    {
                        desc->effect = rwD3D9VERTEXSHADEREFFECT_BUMPMAP;
                    }
                }
                break;

            case rpMATFXEFFECTENVMAP:
                {
                    envMapData = MATFXD3D9ENVMAPGETDATA(material, rpSECONDPASS);

                    if (envMapData->texture)
                    {
                        if (_rwD3D9RasterIsCubeRaster(RwTextureGetRaster(envMapData->texture)))
                        {
                            desc->effect = rwD3D9VERTEXSHADEREFFECT_CUBEMAP;
                        }
                        else
                        {
                            desc->effect = rwD3D9VERTEXSHADEREFFECT_ENVMAP;
                        }
                    }
                }
                break;

            case rpMATFXEFFECTBUMPENVMAP:
                {
                    envMapData = MATFXD3D9ENVMAPGETDATA(material, rpSECONDPASS);

                    if (envMapData->texture)
                    {
                        if (_rwD3D9RasterIsCubeRaster(RwTextureGetRaster(envMapData->texture)))
                        {
                            if (desc->numTexCoords > 0)
                            {
                                desc->effect = rwD3D9VERTEXSHADEREFFECT_BUMPCUBEMAP;
                            }
                            else
                            {
                                desc->effect = rwD3D9VERTEXSHADEREFFECT_CUBEMAP;
                            }
                        }
                        else
                        {
                            if (desc->numTexCoords > 0)
                            {
                                desc->effect = rwD3D9VERTEXSHADEREFFECT_BUMPENVMAP;
                            }
                            else
                            {
                                desc->effect = rwD3D9VERTEXSHADEREFFECT_ENVMAP;
                            }
                        }
                    }
                    else
                    {
                        if (desc->numTexCoords > 0)
                        {
                            desc->effect = rwD3D9VERTEXSHADEREFFECT_BUMPMAP;
                        }
                    }
                }
                break;

            case rpMATFXEFFECTDUAL:
                {
                    /* desc->effect = rwD3D9VERTEXSHADEREFFECT_DUAL; */
                }
                break;

            case rpMATFXEFFECTDUALUVTRANSFORM:
                {
                    /* desc->effect = rwD3D9VERTEXSHADEREFFECT_DUALUVANIM; */
                    desc->effect = rwD3D9VERTEXSHADEREFFECT_UVANIM;
                }
                break;

            case rpMATFXEFFECTUVTRANSFORM:
                {
                    desc->effect = rwD3D9VERTEXSHADEREFFECT_UVANIM;
                }
                break;

            default:
                break;
        }
    }

    RWRETURN(_rpD3D9GetVertexShader(desc, dispatch));
}

/****************************************************************************
 _rpD3D9SkinVertexShaderMatMatFXMeshRenderCallBack
 */
static void
_rpD3D9SkinVertexShaderMatMatFXMeshRenderCallBack(RxD3D9ResEntryHeader *resEntryHeader,
                                          RxD3D9InstanceData *instancedMesh,
                                          const _rpD3D9VertexShaderDescriptor  *desc,
                                          const _rpD3D9VertexShaderDispatchDescriptor *dispatch)
{
    RpMaterial *material;
    const rpMatFXMaterialData   *matFXData;
    RpMatFXMaterialFlags        effectType;
    const MatFXBumpMapData      *bumpmap;
    const MatFXEnvMapData       *envMapData;
    const MatFXDualData         *dualData;
    const MatFXUVAnimData       *uvAnim;
    RwTexture                   *bumpTexture;

    RWFUNCTION(RWSTRING("_rpD3D9SkinVertexShaderMatMatFXMeshRenderCallBack"));
    RWASSERT(resEntryHeader != NULL);
    RWASSERT(instancedMesh);
    RWASSERT(desc != NULL);
    RWASSERT(dispatch != NULL);
    RWASSERT(instancedMesh->material != NULL);

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

		RWRETURNVOID();		
	}
	//<@

    material = instancedMesh->material;

    matFXData = *MATFXMATERIALGETCONSTDATA(material);
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

    switch (effectType)
    {
        case rpMATFXEFFECTBUMPMAP:
            {
                bumpmap = MATFXD3D9BUMPMAPGETDATA(material);

                bumpTexture = bumpmap->texture;

                if (bumpTexture == NULL)
                {
                    if (material->texture != NULL &&
                        _rwD3D9TextureHasAlpha(material->texture))
                    {
                        bumpTexture = material->texture;
                    }
                }

                if (bumpTexture != NULL &&
                    desc->numTexCoords > 0)
                {
                    _rpD3D9VertexShaderSetBumpMatrix(bumpmap->frame,
                                                     (bumpmap->coef) * (bumpmap->invBumpWidth),
                                                     dispatch);

                    _rpMatFXD3D9VertexShaderAtomicBumpMapRender(resEntryHeader,
                                                                instancedMesh,
                                                                material->texture,
                                                                bumpTexture,
                                                                NULL,
                                                                desc,
                                                                dispatch);
                }
                else
                {
                    _rxD3D9VertexShaderDefaultMeshRenderCallBack(resEntryHeader,
                                                                instancedMesh,
                                                                desc,
                                                                dispatch);
                }
            }
            break;

        case rpMATFXEFFECTENVMAP:
            {
                envMapData = MATFXD3D9ENVMAPGETDATA(material, rpSECONDPASS);

                if (envMapData->texture && envMapData->coef > 0.0f)
                {
                    _rpD3D9VertexShaderSetEnvMatrix(envMapData->frame,
                                                    desc,
                                                    dispatch);

                    _rpMatFXD3D9VertexShaderAtomicEnvRender(resEntryHeader,
                                                            instancedMesh,
                                                            material->texture,
                                                            envMapData->texture,
                                                            desc);
                }
                else
                {
                    _rxD3D9VertexShaderDefaultMeshRenderCallBack(resEntryHeader,
                                                                instancedMesh,
                                                                desc,
                                                                dispatch);
                }
            }
            break;

        case rpMATFXEFFECTBUMPENVMAP:
            {
                bumpmap = MATFXD3D9BUMPMAPGETDATA(material);
                envMapData = MATFXD3D9ENVMAPGETDATA(material, rpTHIRDPASS);

                _rpD3D9VertexShaderSetEnvMatrix(envMapData->frame,
                                                desc,
                                                dispatch);

                bumpTexture = bumpmap->texture;

                if (bumpTexture == NULL)
                {
                    if (material->texture != NULL &&
                        _rwD3D9TextureHasAlpha(material->texture))
                    {
                        bumpTexture = material->texture;
                    }
                }

                if (bumpTexture != NULL &&
                    desc->numTexCoords > 0)
                {
                    _rpD3D9VertexShaderSetBumpMatrix(bumpmap->frame,
                                                     (bumpmap->coef) * (bumpmap->invBumpWidth),
                                                     dispatch);

                    _rpMatFXD3D9VertexShaderAtomicBumpMapRender(resEntryHeader,
                                                                instancedMesh,
                                                                material->texture,
                                                                bumpTexture,
                                                                envMapData->texture,
                                                                desc,
                                                                dispatch);
                }
                else
                {
                    if (envMapData->texture && envMapData->coef > 0.0f)
                    {
                        _rpD3D9VertexShaderSetEnvMatrix(envMapData->frame,
                                                        desc,
                                                        dispatch);

                        _rpMatFXD3D9VertexShaderAtomicEnvRender(resEntryHeader,
                                                                instancedMesh,
                                                                material->texture,
                                                                envMapData->texture,
                                                                desc);
                    }
                    else
                    {
                        _rxD3D9VertexShaderDefaultMeshRenderCallBack(resEntryHeader,
                                                                    instancedMesh,
                                                                    desc,
                                                                    dispatch);
                    }
                }
            }
            break;

        case rpMATFXEFFECTDUAL:
            {
                dualData = MATFXD3D9DUALGETDATA(material);

                if (dualData->texture && dualData->texture->raster)
                {
                    _rpMatFXD3D9VertexShaderAtomicDualRender(resEntryHeader,
                                                             instancedMesh,
                                                             material->texture,
                                                             dualData->texture,
                                                             desc,
                                                             dispatch);
                }
                else
                {
                    _rxD3D9VertexShaderDefaultMeshRenderCallBack(resEntryHeader,
                                                                 instancedMesh,
                                                                 desc,
                                                                 dispatch);
                }
            }
            break;

        case rpMATFXEFFECTDUALUVTRANSFORM:
            {
                dualData = MATFXD3D9DUALUVANIMGETDUALDATA(material);

                if (dualData->texture && dualData->texture->raster)
                {
                    _rpMatFXD3D9VertexShaderAtomicDualRender(resEntryHeader,
                                                             instancedMesh,
                                                             material->texture,
                                                             dualData->texture,
                                                             desc,
                                                             dispatch);
                }
                else
                {
                    _rxD3D9VertexShaderDefaultMeshRenderCallBack(resEntryHeader,
                                                                 instancedMesh,
                                                                 desc,
                                                                 dispatch);
                }
            }
            break;

        case rpMATFXEFFECTUVTRANSFORM:
            {
                uvAnim = MATFXD3D9UVANIMGETDATA(material);

                _rpD3DVertexShaderSetUVAnimMatrix(uvAnim->baseTransform,
                                                  dispatch);
            }
            /* Fall to default render */

        default:
            _rxD3D9VertexShaderDefaultMeshRenderCallBack(resEntryHeader,
                                                         instancedMesh,
                                                         desc,
                                                         dispatch);
            break;
    }

	//@{ 20050513 DDonSS : Threadsafe
	// ResEntry Unlock
	CS_RESENTRYHEADER_UNLOCK( resEntryHeader );
	//@} DDonSS

    RWRETURNVOID();
}

/****************************************************************************
 _rxSkinD3D9AtomicAllInOnePipelineInitMatFX
 */
static RwBool
_rxSkinD3D9AtomicAllInOnePipelineInitMatFX(RxPipelineNode *node)
{
    _rxD3D9SkinInstanceNodeData *instanceData;

    RWFUNCTION(RWSTRING("_rxSkinD3D9AtomicAllInOnePipelineInitMatFX"));

    instanceData = (_rxD3D9SkinInstanceNodeData *)node->privateData;

    instanceData->renderCallback = _rwD3D9AtomicMatFXRenderCallback;
    instanceData->lightingCallback = _rwD3D9AtomicDefaultLightingCallback;

    if (_rpSkinGlobals.platform.hardwareVS)
    {
        instanceData->vertexShaderNode.beginCallback = _rpD3D9SkinVertexShaderBeginCallBack;
        instanceData->vertexShaderNode.lightingCallback = _rxD3D9VertexShaderDefaultLightingCallBack;
        instanceData->vertexShaderNode.getmaterialshaderCallback = _rpD3D9SkinVertexShaderMatFXGetMaterialShaderCallBack;
        instanceData->vertexShaderNode.meshRenderCallback = _rpD3D9SkinVertexShaderMatMatFXMeshRenderCallBack;
        instanceData->vertexShaderNode.endCallback = _rxD3D9VertexShaderDefaultEndCallBack;
    }
    else
    {
        instanceData->vertexShaderNode.beginCallback = NULL;
        instanceData->vertexShaderNode.lightingCallback = NULL;
        instanceData->vertexShaderNode.getmaterialshaderCallback = NULL;
        instanceData->vertexShaderNode.meshRenderCallback = NULL;
        instanceData->vertexShaderNode.endCallback = NULL;
    }

    RWRETURN(TRUE);
}

/*
 * _rxNodeDefinitionGetD3D9SkinAtomicAllInOneMatFx returns a
 * pointer to the \ref RxNodeDefinition associated with
 * the Skin Atomic version of PowerPipe.
 */
static RxNodeDefinition *
_rxNodeDefinitionGetD3D9SkinAtomicAllInOneMatFx(void)
{
    static RwChar _SkinAtomicInstance_csl[] = RWSTRING("nodeD3D9SkinAtomicAllInOne.csl");

    static RxNodeDefinition nodeD3D9SkinAtomicAllInOneCSL = { /* */
        _SkinAtomicInstance_csl,                    /* Name */
        {                                           /* Nodemethods */
            _rwSkinD3D9AtomicAllInOneNode,          /* +-- nodebody */
            NULL,                                   /* +-- nodeinit */
            NULL,                                   /* +-- nodeterm */
            _rxSkinD3D9AtomicAllInOnePipelineInitMatFX,  /* +-- pipelinenodeinit */
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
        (RxNodeDefEditable)FALSE,                   /* editable */
        0                                           /* inPipes */
    };

    RWFUNCTION(RWSTRING("_rxNodeDefinitionGetD3D9SkinAtomicAllInOneMatFx"));

    RWRETURN(&nodeD3D9SkinAtomicAllInOneCSL);
}

RxPipeline *
_rpSkinD3D9CreateMatFXPipe()
{
    RxPipeline  *pipeline;

    RWFUNCTION(RWSTRING("_rpSkinD3D9CreateMatFXPipe"));

    pipeline = RxPipelineCreate();

    if (pipeline)
    {
        RxLockedPipe    *lpipe;

        pipeline->pluginId = rwID_SKINPLUGIN;
        pipeline->pluginData = rpSKINTYPEMATFX;

        if (NULL != (lpipe = RxPipelineLock(pipeline)))
        {
            lpipe = RxLockedPipeAddFragment(lpipe,
                NULL,
                _rxNodeDefinitionGetD3D9SkinAtomicAllInOneMatFx(),
                NULL);

            lpipe = RxLockedPipeUnlock(lpipe);

            RWASSERT(pipeline == (RxPipeline *)lpipe);
            RWRETURN(pipeline);
        }
        else
        {
            RxPipelineDestroy(pipeline);
            pipeline = NULL;
        }
    }

    RWRETURN(pipeline);
}

RwBool
_rwD3D9SkinMatFXNeedsAManagedVertexBuffer(const RxD3D9ResEntryHeader *resEntryHeader)
{
    RWFUNCTION(RWSTRING("_rwD3D9SkinMatFXNeedsAManagedVertexBuffer"));

    if (!_rpSkinGlobals.platform.hardwareTL)
    {
        RWRETURN(TRUE);
    }
    else
    {
        if (resEntryHeader != NULL)
        {
            const RxD3D9InstanceData    *instancedData;
            RwUInt32                    numMeshes;

            /* Get the first instanced data structures */
            instancedData = (const RxD3D9InstanceData *)(resEntryHeader + 1);

            /* Get the number of meshes */
            numMeshes = resEntryHeader->numMeshes;

            do
            {
                if (instancedData->material != NULL &&
                    _rwD3D9MaterialMatFXHasBumpMap(instancedData->material))
                {
                    RWRETURN(TRUE);
                }

                ++instancedData;
            }
            while (--numMeshes);
        }
    }

    RWRETURN(FALSE);
}
