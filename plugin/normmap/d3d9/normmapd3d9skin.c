/****************************************************************************
 *
 * normmapd3d9skin.c
 *
 ****************************************************************************/

#include <d3d9.h>

#include "rwcore.h"
#include "rpworld.h"
#include "rpskin.h"
#include "rpdbgerr.h"

#include "rpnormmap.h"
#include "normmap.h"
#include "normmapd3d9.h"

#include "../skin2/d3d9/skind3d9.h"

/*--- Global ---*/

/*--- Local Variables ---*/

static RxPipeline   *NormalMapAtomicPipeline = NULL;
static RxPipeline   *NormalMapAtomicSkinPipeline = NULL;

/*
 ***************************************************************************
 */
void
RpNormMapAtomicInitialize(RpAtomic *atomic, RpNormMapAtomicPipeline pipeline)
{
    RWAPIFUNCTION(RWSTRING("RpNormMapAtomicInitialize"));
    RWASSERT(atomic != NULL);

    if (NormalMapSupported)
    {
        RpGeometry *geometry;
        RpD3D9GeometryUsageFlag flags;

        geometry = RpAtomicGetGeometry(atomic);

        flags = RpD3D9GeometryGetUsageFlags(geometry);

        flags |= rpD3D9GEOMETRYUSAGE_CREATETANGENTS;

        RpD3D9GeometrySetUsageFlags(geometry, flags);
    }

    if (pipeline == rpNORMMAPATOMICSKINNEDPIPELINE)
    {
        RpAtomicSetPipeline(atomic, NormalMapAtomicSkinPipeline);
    }
    else
    {
        RWASSERT(pipeline == rpNORMMAPATOMICSTATICPIPELINE);

        RpAtomicSetPipeline(atomic, NormalMapAtomicPipeline);
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpSkinD3D9EnumerateLights
 */
static RwV4d *
NormalMapSkinEnumerateLights(void *object,
                             RwUInt32 type,
                             RwV4d    *shaderConstantPtr,
                             _rpD3D9VertexShaderDescriptor  *desc)
{
    RpAtomic *atomic;

    RWFUNCTION(RWSTRING("NormalMapSkinEnumerateLights"));
    RWASSERT(type == rpATOMIC);

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe Check
	THREADSAFE_CHECK_ISCALLEDMAIN();
	//@} DDonSS

    atomic = (RpAtomic *)object;

    NormalMapMainLight = _rpNormMapMainLightActive;

    if (_rpNormMapAmbientLightActive != NULL)
    {
        const RwRGBAReal    *color;

        color = RpLightGetColor(_rpNormMapAmbientLightActive);

        /* Accumulate ambient light color */
        NormalMapAmbientColor.red = color->red;
        NormalMapAmbientColor.green = color->green;
        NormalMapAmbientColor.blue = color->blue;
    }
    else
    {
        NormalMapAmbientColor.red = 0.0f;
        NormalMapAmbientColor.green = 0.0f;
        NormalMapAmbientColor.blue = 0.0f;
    }

    NormalMapAmbientColor.alpha = 0.0f;

    desc->numDirectionalLights = 0;
    desc->numPointLights = 0;
    desc->numSpotLights = 0;

    if ( NULL != RWSRCGLOBAL(curWorld) &&
         (_rpNormMapMainLightActive == NULL ||
          _rpNormMapAmbientLightActive == NULL) )
    {
        const RpWorld   *world;
        const RwLLLink  *cur, *end;

        /*
         * Global lights, (Directional & Ambient)
         */
        world = (const RpWorld *)RWSRCGLOBAL(curWorld);

        cur = rwLinkListGetFirstLLLink(&world->directionalLightList);
        end = rwLinkListGetTerminator(&world->directionalLightList);
        while (cur != end)
        {
            const RpLight *light;

            light = rwLLLinkGetData(cur, RpLight, inWorld);

            /* NB light may actually be a dummyTie from a enclosing ForAll */
            if (light && (rwObjectTestFlags(light, rpLIGHTLIGHTATOMICS)))
            {
                if (RpLightGetType(light) == rpLIGHTDIRECTIONAL)
                {
                    /* We are just using one of them !!! */
                    if (NormalMapMainLight == NULL)
                    {
                        NormalMapMainLight = light;
                    }
                }
                else
                {
                    if (_rpNormMapAmbientLightActive == NULL)
                    {
                        const RwRGBAReal    *color;

                        color = RpLightGetColor(light);

                        /* Accumulate ambient light color */
                        NormalMapAmbientColor.red += color->red;
                        NormalMapAmbientColor.green += color->green;
                        NormalMapAmbientColor.blue += color->blue;
                    }
                }
            }

            /* Next */
            cur = rwLLLinkGetNext(cur);
        }

        /*
         * Local Lights, (Point, Soft & Soft spot)
         */
        if (NormalMapMainLight == NULL)
        {
            /* Increase the marker ! */
            RWSRCGLOBAL(lightFrame)++;

            /* For all sectors that this atomic lies in, apply all lights within */
            cur = rwLinkListGetFirstLLLink(&atomic->llWorldSectorsInAtomic);
            end = rwLinkListGetTerminator(&atomic->llWorldSectorsInAtomic);
            while (cur != end)
            {
                const RpTie       *tpTie = rwLLLinkGetData(cur, RpTie, lWorldSectorInAtomic);
                const RwLLLink    *curLight, *endLight;

                /* Lights in the sector */
                curLight = rwLinkListGetFirstLLLink(&tpTie->worldSector->lightsInWorldSector);
                endLight = rwLinkListGetTerminator(&tpTie->worldSector->lightsInWorldSector);

                while (curLight != endLight)
                {
                    const RpLightTie  *lightTie;

                    lightTie = rwLLLinkGetData(curLight, RpLightTie, lightInWorldSector);

                    /* NB lightTie may actually be a dummyTie from a enclosing ForAll */

                    /* Check to see if the light has already been applied and is set to
                     * light atomics
                     */
                    if (lightTie->light &&
                        (lightTie->light->lightFrame != RWSRCGLOBAL(lightFrame)) &&
                        (rwObjectTestFlags(lightTie->light, rpLIGHTLIGHTATOMICS)))
                    {
                        const RwMatrix  *matrixLight;
                        const RwV3d     *pos;
                        const RwSphere  *sphere;
                        RwV3d           distanceVector;
                        RwReal          distanceSquare;
                        RwReal          distanceCollision;

                        /* don't light this atomic with the same light again! */
                        lightTie->light->lightFrame = RWSRCGLOBAL(lightFrame);

                        /* Does the light intersect the atomics bounding sphere */
                        matrixLight = RwFrameGetLTM(RpLightGetFrame(lightTie->light));

                        pos = &(matrixLight->pos);

                        sphere = RpAtomicGetWorldBoundingSphere(atomic);

                        RwV3dSub(&distanceVector, &(sphere->center), pos);

                        distanceSquare = RwV3dDotProduct(&distanceVector, &distanceVector);

                        distanceCollision = (sphere->radius + RpLightGetRadius(lightTie->light));

                        if (distanceSquare < (distanceCollision * distanceCollision))
                        {
                            /* We are just using one of them !!! */
                            NormalMapMainLight = lightTie->light;
                            break;
                        }
                    }

                    /* Next */
                    curLight = rwLLLinkGetNext(curLight);
                }

                /* Next one */
                cur = rwLLLinkGetNext(cur);
            }
        }
    }

    /* Ambient light, not used */
    shaderConstantPtr->x = NormalMapAmbientColor.red;
    shaderConstantPtr->y = NormalMapAmbientColor.green;
    shaderConstantPtr->z = NormalMapAmbientColor.blue;
    shaderConstantPtr->w = 1.0f;
    ++shaderConstantPtr;

    if (NormalMapMainLight != NULL)
    {
        RwFrame     *frame;
        RwMatrix    *ltm;
        const RwMatrix  *matrixLight;
        const RwRGBAReal *color;

        frame = RpAtomicGetFrame(atomic);
        ltm = RwFrameGetLTM(frame);

        matrixLight = RwFrameGetLTM(RpLightGetFrame(NormalMapMainLight));

        if (RpLightGetType(NormalMapMainLight) == rpLIGHTDIRECTIONAL)
        {
            const RwV3d     *dirLight;
            RwMatrix    matrixObjectNoScale;
            RwMatrix    matrixObjectNoScaleInverted;
            RwV3d       dirLightObjectSpace;
            RwReal      unused;

            if (rwMatrixTestFlags(ltm, rwMATRIXTYPEORTHONORMAL) != rwMATRIXTYPEORTHONORMAL)
            {
                RwMatrixOrthoNormalize(&matrixObjectNoScale, ltm);
                RwMatrixInvert(&matrixObjectNoScaleInverted, &matrixObjectNoScale);
            }
            else
            {
                RwMatrixInvert(&matrixObjectNoScaleInverted, ltm);
            }

            /* Get light dir */
            dirLight = &(matrixLight->at);

            RwV3dTransformVector(&dirLightObjectSpace, dirLight, &matrixObjectNoScaleInverted);

            shaderConstantPtr->x = -(dirLightObjectSpace.x);
            shaderConstantPtr->y = -(dirLightObjectSpace.y);
            shaderConstantPtr->z = -(dirLightObjectSpace.z);

            _rwV3dNormalizeMacro(unused,
                                 (RwV3d *)shaderConstantPtr,
                                 (const RwV3d *)shaderConstantPtr);

            shaderConstantPtr->w = 0.5f;

            ++shaderConstantPtr;

            /* Set the light color */
            color = RpLightGetColor(NormalMapMainLight);
            shaderConstantPtr->x = color->red;
            shaderConstantPtr->y = color->green;
            shaderConstantPtr->z = color->blue;
            shaderConstantPtr->w = 1.f;
            ++shaderConstantPtr;

            desc->numDirectionalLights = 1;
        }
        else if (RpLightGetType(NormalMapMainLight) == rpLIGHTPOINT)
        {
            RwMatrix    matrixObjectInverted;
            RwReal      radiusLight;
            const RwV3d *posLight;

            RwMatrixInvert(&matrixObjectInverted, ltm);

            /* Get light radius */
            if (rwMatrixTestFlags(ltm, rwMATRIXTYPEORTHONORMAL) != rwMATRIXTYPEORTHONORMAL)
            {
                RwV3d   radiousLocal;
                RwReal  lengthLocal;

                radiousLocal.x = 0.57735026918962576450914878050196f;
                radiousLocal.y = 0.57735026918962576450914878050196f;
                radiousLocal.z = 0.57735026918962576450914878050196f;

                RwV3dTransformVector(
                    &radiousLocal,
                    &radiousLocal,
                    &matrixObjectInverted );

                lengthLocal = RwV3dLength(&radiousLocal);

                radiusLight = RpLightGetRadius(NormalMapMainLight);
                shaderConstantPtr->x = -(lengthLocal / radiusLight);
            }
            else
            {
                radiusLight = RpLightGetRadius(NormalMapMainLight);
                shaderConstantPtr->x = -(1.f / radiusLight);
            }
            shaderConstantPtr->y = 1.0f;
            shaderConstantPtr->z = 0.5f;
            shaderConstantPtr->w = 0.0f;
            shaderConstantPtr++;

            /* Get light dir */
            posLight = &(matrixLight->pos);

            RwV3dTransformPoint((RwV3d *)shaderConstantPtr,
                                posLight,
                                &matrixObjectInverted);
            shaderConstantPtr->w = 0.0f;
            ++shaderConstantPtr;

            /* Set the light color */
            color = RpLightGetColor(NormalMapMainLight);
            shaderConstantPtr->x = color->red;
            shaderConstantPtr->y = color->green;
            shaderConstantPtr->z = color->blue;
            shaderConstantPtr->w = 1.f;
            ++shaderConstantPtr;

            desc->numPointLights = 1;
        }
        else if (RpLightGetType(NormalMapMainLight) == rpLIGHTSPOT ||
                 RpLightGetType(NormalMapMainLight) == rpLIGHTSPOTSOFT)
        {
            RwMatrix    matrixObjectInverted;
            RwReal      radiusLight;
            const RwV3d *posLight;

            RwMatrixInvert(&matrixObjectInverted, ltm);

            /* Get light radius */
            if (rwMatrixTestFlags(ltm, rwMATRIXTYPEORTHONORMAL) != rwMATRIXTYPEORTHONORMAL)
            {
                RwV3d   radiousLocal;
                RwReal  lengthLocal;

                radiousLocal.x = 0.57735026918962576450914878050196f;
                radiousLocal.y = 0.57735026918962576450914878050196f;
                radiousLocal.z = 0.57735026918962576450914878050196f;

                RwV3dTransformVector(
                    &radiousLocal,
                    &radiousLocal,
                    &matrixObjectInverted );

                lengthLocal = RwV3dLength(&radiousLocal);

                radiusLight = RpLightGetRadius(NormalMapMainLight);
                shaderConstantPtr->x = -(lengthLocal / radiusLight);
            }
            else
            {
                radiusLight = RpLightGetRadius(NormalMapMainLight);
                shaderConstantPtr->x = -(1.f / radiusLight);
            }
            shaderConstantPtr->y = 1.0f;
            shaderConstantPtr->z = 0.5f;
            shaderConstantPtr->w = 0.0f;
            ++shaderConstantPtr;

            /* Get light dir */
            posLight = &(matrixLight->pos);

            RwV3dTransformPoint((RwV3d *)shaderConstantPtr,
                                posLight,
                                &matrixObjectInverted);
            shaderConstantPtr->w = 0.0f;
            ++shaderConstantPtr;

            /* Set the light color */
            color = RpLightGetColor(NormalMapMainLight);
            shaderConstantPtr->x = color->red;
            shaderConstantPtr->y = color->green;
            shaderConstantPtr->z = color->blue;
            shaderConstantPtr->w = 1.f;
            ++shaderConstantPtr;

            desc->numSpotLights = 1;
        }
    }

    RWRETURN(shaderConstantPtr);
}

/****************************************************************************
 NormalMapSkinGetMaterialShaderCallBack
 */
static void *
NormalMapSkinGetMaterialShaderCallBack(const RpMaterial *material,
                                       _rpD3D9VertexShaderDescriptor *desc,
                                       _rpD3D9VertexShaderDispatchDescriptor *dispatch)
{
    RWFUNCTION(RWSTRING("NormalMapSkinGetMaterialShaderCallBack"));
    RWASSERT(material != NULL);
    RWASSERT(desc != NULL);
    RWASSERT(dispatch != NULL);

    desc->effect = 0;

    if ( desc->normals &&
         desc->numTexCoords > 0 &&
         (desc->numDirectionalLights ||
          desc->numPointLights ||
          desc->numSpotLights) )
    {
        const NormalMapExt  *normalmapext;

        normalmapext = CONSTMATERIALGETNORMALMAP(material);

        if (normalmapext->envMap != NULL)
        {
            desc->effect = rwD3D9VERTEXSHADEREFFECT_NORMALENVMAP;
        }
        else
        {
            desc->effect = rwD3D9VERTEXSHADEREFFECT_NORMALMAP;
        }
    }

    RWRETURN(_rpD3D9GetVertexShader(desc, dispatch));
}

/*
 ***************************************************************************
 */
static void
NormalMapSkinRenderCallBack(RxD3D9ResEntryHeader *resEntryHeader,
                            RxD3D9InstanceData *instancedMesh,
                            const _rpD3D9VertexShaderDescriptor  *desc,
                            const _rpD3D9VertexShaderDispatchDescriptor *dispatch)
{
    const NormalMapExt  *normalmapext;

    const RwSurfaceProperties *surfProp;
    const RwRGBA *materialcolor;

    RWFUNCTION(RWSTRING("NormalMapSkinRenderCallBack"));

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

    surfProp = &instancedMesh->material->surfaceProps;
    materialcolor = &instancedMesh->material->color;

    normalmapext = CONSTMATERIALGETNORMALMAP(instancedMesh->material);

    /* Set lights colors */
    if (NormalMapMainLight != NULL)
    {
        NormalMapPixelShaderConstants constantsPS;

        const RwRGBAReal *color;

        if ( normalmapext->envMap != NULL)
        {
            D3DMATRIX result;

            if (desc->prelit)
            {
                if (instancedMesh->material->texture != NULL)
                {
                    if (normalmapext->modulateEnvMap)
                    {
                        RwD3D9SetPixelShader(NormalMapEnvMapMetalPrelitPixelShader);
                    }
                    else
                    {
                        RwD3D9SetPixelShader(NormalMapEnvMapPrelitPixelShader);
                    }
                }
                else
                {
                    RwD3D9SetPixelShader(NormalMapEnvMapPrelitNoTexturePixelShader);
                }

                RwD3D9SetRenderState(D3DRS_SPECULARENABLE, TRUE);
            }
            else
            {
                if (instancedMesh->material->texture != NULL)
                {
                    if (normalmapext->modulateEnvMap)
                    {
                        RwD3D9SetPixelShader(NormalMapEnvMapMetalPixelShader);
                    }
                    else
                    {
                        RwD3D9SetPixelShader(NormalMapEnvMapPixelShader);
                    }
                }
                else
                {
                    RwD3D9SetPixelShader(NormalMapEnvMapNoTexturePixelShader);
                }
            }

            RwD3D9SetSamplerState(2, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
            RwD3D9SetSamplerState(2, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

            /* Set envmap matrix */
            if (normalmapext->envmapFrame != NULL)
            {
                RwMatrix    *envMtx;
                RwMatrix    invMtx;
                RwMatrix    tmpMtx;

                /* Transfrom the normals by the inverse of the env maps frame */
                envMtx = RwFrameGetLTM(normalmapext->envmapFrame);

                RwMatrixInvert(&invMtx, envMtx);

                RwMatrixMultiply(&tmpMtx, &invMtx, &NormalMapEnvMapTexMat);

                _rwD3D9VSGetWorldNormalizedMultiplyTransposeMatrix(&result, &tmpMtx);
            }
            else
            {
                _rwD3D9VSGetWorldNormalizedViewMultiplyTransposeMatrix(&result, &NormalMapEnvMapTexMat);
            }

            /* pack relevant 2 rows into constant registers */
            RwD3D9SetVertexShaderConstant(dispatch->offsetEffect, &result, 2);
        }
        else
        {
            if (desc->prelit)
            {
                if (instancedMesh->material->texture != NULL)
                {
                    RwD3D9SetPixelShader(NormalMapPrelitPixelShader);
                }
                else
                {
                    RwD3D9SetPixelShader(NormalMapPrelitNoTexturePixelShader);
                }
            }
            else
            {
                if (instancedMesh->material->texture != NULL)
                {
                    RwD3D9SetPixelShader(NormalMapPixelShader);
                }
                else
                {
                    RwD3D9SetPixelShader(NormalMapNoTexturePixelShader);
                }
            }
        }

        color = RpLightGetColor(NormalMapMainLight);

        if (desc->modulateMaterial &&
            *((const RwUInt32 *)(materialcolor)) != 0xffffffff)
        {
            const RwReal diffusseCoef = (surfProp->diffuse) / 255.f;
            const RwReal ambientCoef = (surfProp->ambient) / 255.f;

            constantsPS.colorDirLight.x = (color->red) * (RwReal)(materialcolor->red) * diffusseCoef;
            constantsPS.colorDirLight.y = (color->green) * (RwReal)(materialcolor->green) * diffusseCoef;
            constantsPS.colorDirLight.z = (color->blue) * (RwReal)(materialcolor->blue) * diffusseCoef;
            constantsPS.colorDirLight.w = (RwReal)(materialcolor->alpha) / 255.f;

            constantsPS.colorAmbientLight.x = (NormalMapAmbientColor.red) * (RwReal)(materialcolor->red) * ambientCoef;
            constantsPS.colorAmbientLight.y = (NormalMapAmbientColor.green) * (RwReal)(materialcolor->green) * ambientCoef;
            constantsPS.colorAmbientLight.z = (NormalMapAmbientColor.blue) * (RwReal)(materialcolor->blue) * ambientCoef;
            constantsPS.colorAmbientLight.w = normalmapext->shininess;
        }
        else
        {
            constantsPS.colorDirLight.x = (color->red) * (surfProp->diffuse);
            constantsPS.colorDirLight.y = (color->green) * (surfProp->diffuse);
            constantsPS.colorDirLight.z = (color->blue) * (surfProp->diffuse);
            constantsPS.colorDirLight.w = 1.0f;

            constantsPS.colorAmbientLight.x = (NormalMapAmbientColor.red) * (surfProp->ambient);
            constantsPS.colorAmbientLight.y = (NormalMapAmbientColor.green) * (surfProp->ambient);
            constantsPS.colorAmbientLight.z = (NormalMapAmbientColor.blue) * (surfProp->ambient);
            constantsPS.colorAmbientLight.w = normalmapext->shininess;
        }

        RwD3D9SetPixelShaderConstant(0, &constantsPS, sizeof(constantsPS) / sizeof(RwV4d));

        /* Set textures */
        RwD3D9SetTexture(normalmapext->normalMap, 0);

        RwD3D9SetTexture(instancedMesh->material->texture, 1);

        RwD3D9SetTexture(NULL, 2);

        if (normalmapext->envMap != NULL)
        {
            RwD3D9SetTexture(normalmapext->envMap, 3);
        }
        else
        {
            RwD3D9SetTexture(NULL, 3);
        }
    }
    else
    {
        RwUInt32 tFactor;

        if (desc->modulateMaterial &&
            *((const RwUInt32 *)(materialcolor)) != 0xffffffff)
        {
            tFactor  = RwFastRealToUInt32((NormalMapAmbientColor.red) * (RwReal)(materialcolor->red) * surfProp->ambient)     << 16;
            tFactor |= RwFastRealToUInt32((NormalMapAmbientColor.green) * (RwReal)(materialcolor->green) * surfProp->ambient) << 8;
            tFactor |= RwFastRealToUInt32((NormalMapAmbientColor.blue) * (RwReal)(materialcolor->blue) * surfProp->ambient)   << 0;
            tFactor |= (materialcolor->alpha) << 24;
        }
        else
        {
            const RwReal ambientCoef = (surfProp->ambient * 255.f);

            tFactor  = RwFastRealToUInt32((NormalMapAmbientColor.red) * ambientCoef)   << 16;
            tFactor |= RwFastRealToUInt32((NormalMapAmbientColor.green) * ambientCoef) << 8;
            tFactor |= RwFastRealToUInt32((NormalMapAmbientColor.blue) * ambientCoef)  << 0;
            tFactor |= (0xff) << 24;
        }

        RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, tFactor);

        if (instancedMesh->material->texture != NULL &&
            instancedMesh->material->texture->raster != NULL)
        {
            if (_rwD3D9TextureHasAlpha(instancedMesh->material->texture) || (tFactor & 0x00ffffff) != 0)
            {
                RwD3D9SetTexture(instancedMesh->material->texture, 0);

                RwD3D9SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
                RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
            }
            else
            {
                RwD3D9SetTexture(NULL, 0);

                RwD3D9SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
                RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
            }
        }
        else
        {
            RwD3D9SetTexture(NULL, 0);

            RwD3D9SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
        }
    }

    /*
    * Vertex shader
    */
    RwD3D9SetVertexShader(instancedMesh->vertexShader);

    /*
        * Render
        */
    if (resEntryHeader->indexBuffer != NULL)
    {
        RwD3D9DrawIndexedPrimitive((D3DPRIMITIVETYPE)resEntryHeader->primType,
                                    instancedMesh->baseIndex,
                                    0, instancedMesh->numVertices,
                                    instancedMesh->startIndex, instancedMesh->numPrimitives);
    }
    else
    {
        RwD3D9DrawPrimitive((D3DPRIMITIVETYPE)resEntryHeader->primType,
                            instancedMesh->baseIndex,
                            instancedMesh->numPrimitives);
    }

	//@{ 20050513 DDonSS : Threadsafe
	// ResEntry Unlock
	CS_RESENTRYHEADER_UNLOCK( resEntryHeader );
	//@} DDonSS

    /*
     * Set the Default Pixel shader
     */
    RwD3D9SetPixelShader(NULL);

    RwD3D9SetTexture(NULL, 1);

    RwD3D9SetTexture(NULL, 2);

    RwD3D9SetTexture(NULL, 3);

    RwD3D9SetRenderState(D3DRS_SPECULARENABLE, FALSE);

    RWRETURNVOID();
}

/*
 ***************************************************************************
 */
static RxPipeline *
NormalMapCreateAtomicSkinPipeline(void)
{
    RxPipeline  *pipe;

    RWFUNCTION(RWSTRING("NormalMapCreateAtomicSkinPipeline"));

    pipe = RxPipelineCreate();
    if (pipe)
    {
        RxLockedPipe    *lpipe;

        /* Identify the pipeline with it's plugin and id. */
        pipe->pluginId = rwPLUGIN_ID;
        pipe->pluginData = rpNORMMAPATOMICSKINNEDPIPELINE;

        lpipe = RxPipelineLock(pipe);
        if (NULL != lpipe)
        {
            RxNodeDefinition    *instanceNode;
            RxPipelineNode      *node;

            /*
             * Get the instance node definition
             */
            instanceNode = RxNodeDefinitionGetD3D9SkinAtomicAllInOne();

            /*
             * Add the node to the pipeline
             */
            lpipe = RxLockedPipeAddFragment(lpipe, NULL, instanceNode, NULL);

            /*
             * Unlock the pipeline
             */
            lpipe = RxLockedPipeUnlock(lpipe);

            /*
             * Set the pipeline specific data
             */
            if (NormalMapSupported)
            {
                node = RxPipelineFindNodeByName(pipe, instanceNode->name, NULL, NULL);

                /*
                * Set the lighting callback
                */
                _rxD3D9SkinVertexShaderSetLightingCallBack(node, NormalMapSkinEnumerateLights);

                /*
                * Set the GetMaterialShader callback
                */
                _rxD3D9SkinVertexShaderSetGetMaterialShaderCallBack(node, NormalMapSkinGetMaterialShaderCallBack);

                /*
                * Set the render callback
                */
                _rxD3D9SkinVertexShaderSetMeshRenderCallBack(node, NormalMapSkinRenderCallBack);
            }

            RWRETURN(pipe);
        }

        RxPipelineDestroy(pipe);
    }

    RWRETURN(NULL);
}

/*
 ***************************************************************************
 */
RwBool
NormalMapCreatePipelines(void)
{
    RWFUNCTION(RWSTRING("NormalMapCreatePipelines"));

    /*
     * Create a new atomic pipeline
     */
    NormalMapAtomicPipeline = NormalMapCreateAtomicPipeline();
    if (NormalMapAtomicPipeline == NULL)
    {
        RWRETURN(FALSE);
    }

    /*
     * Create a new skin atomic pipeline
     */
    NormalMapAtomicSkinPipeline = NormalMapCreateAtomicSkinPipeline();
    if (NormalMapAtomicSkinPipeline == NULL)
    {
        RWRETURN(FALSE);
    }

    /*
     * Create a new world sector pipeline
     */
    _rpNormMapWorldSectorPipeline = NormalMapCreateWorldSectorPipeline();
    if (_rpNormMapWorldSectorPipeline == NULL)
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}

/*
 ***************************************************************************
 */
void
NormalMapDestroyPipelines(void)
{
    RWFUNCTION(RWSTRING("NormalMapDestroyPipelines"));

    if (_rpNormMapWorldSectorPipeline != NULL)
    {
        RxPipelineDestroy(_rpNormMapWorldSectorPipeline);
        _rpNormMapWorldSectorPipeline = NULL;
    }

    if (NormalMapAtomicSkinPipeline != NULL)
    {
        RxPipelineDestroy(NormalMapAtomicSkinPipeline);
        NormalMapAtomicSkinPipeline = NULL;
    }

    if (NormalMapAtomicPipeline != NULL)
    {
        RxPipelineDestroy(NormalMapAtomicPipeline);
        NormalMapAtomicPipeline = NULL;
    }

    RWRETURNVOID();
}

/*
 ***************************************************************************
 */
RxPipeline *
RpNormMapGetAtomicPipeline(RpNormMapAtomicPipeline pipeline)
{
    RWAPIFUNCTION(RWSTRING("RpNormMapGetAtomicPipeline"));

    if (pipeline == rpNORMMAPATOMICSKINNEDPIPELINE)
    {
        RWRETURN(NormalMapAtomicSkinPipeline);
    }
    else
    {
        RWRETURN(NormalMapAtomicPipeline);
    }
}
