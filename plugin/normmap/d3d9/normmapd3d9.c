/****************************************************************************
 *
 * normmapd3d9.c
 *
 ****************************************************************************/

#include <d3d9.h>

#include "rwcore.h"
#include "rpworld.h"
#include "rpdbgerr.h"

#include "rpnormmap.h"
#include "normmap.h"
#include "normmapd3d9.h"

#include "dot3.h"
#include "dot3prelit.h"
#include "dot3notexture.h"
#include "dot3prelitnotexture.h"

#include "dot3envmap.h"
#include "dot3envmapprelit.h"
#include "dot3envmapnotexture.h"
#include "dot3envmapprelitnotexture.h"

#include "dot3envmap14.h"
#include "dot3envmapprelit14.h"
#include "dot3envmapnotexture14.h"
#include "dot3envmapprelitnotexture14.h"

#include "dot3envmapmetal.h"
#include "dot3envmapmetalprelit.h"
#include "dot3envmapmetal14.h"
#include "dot3envmapmetalprelit14.h"

#define RWD3D9_ALIGN16 __declspec(align(16))

struct NormalMapVertexShaderConstants
{
    NormalMapMatrixTransposed worldViewProjectionTransposed;
    RwV4d ambient;  /* Ignored */
    RwV4d lightData0;
    RwV4d lightData1;
    RwV4d lightSpotDir;
    RwV4d lightAngleCoefs;
};
typedef struct NormalMapVertexShaderConstants NormalMapVertexShaderConstants;

/*--- Global ---*/
void *NormalMapPixelShader = NULL;
void *NormalMapPrelitPixelShader = NULL;
void *NormalMapEnvMapPixelShader = NULL;
void *NormalMapEnvMapPrelitPixelShader = NULL;

void *NormalMapNoTexturePixelShader = NULL;
void *NormalMapPrelitNoTexturePixelShader = NULL;
void *NormalMapEnvMapNoTexturePixelShader = NULL;
void *NormalMapEnvMapPrelitNoTexturePixelShader = NULL;

void *NormalMapEnvMapMetalPixelShader = NULL;
void *NormalMapEnvMapMetalPrelitPixelShader = NULL;

const RpLight    *NormalMapMainLight = NULL;
RwRGBAReal       NormalMapAmbientColor;

const RwMatrix NormalMapEnvMapTexMat =
{
    1.0f, 0.0f, 0.0f, 0,
    0.0f,-1.0f, 0.0f, 0,
    0.0f, 0.0f, 1.0f, 0,
    0.0f, 0.0f, 0.0f, 0x3f800000
};

RwBool NormalMapSupported = FALSE;

/*--- Local Variables ---*/

static RwInt32 NormalMapNumInstances = 0;

/****************************************************************************
*/
static void
NormalMapAtomicLightingCallBack(void *object)
{
    RpAtomic        *atomic;
    RpGeometryFlag  flags;

    RWFUNCTION(RWSTRING("NormalMapAtomicLightingCallBack"));

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe Check
	THREADSAFE_CHECK_ISCALLEDMAIN();
	//@} DDonSS

    atomic = (RpAtomic *)object;
    flags = (RpGeometryFlag)
        RpGeometryGetFlags(RpAtomicGetGeometry(atomic));

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

    if ( (flags & rxGEOMETRY_LIGHT) != 0 &&
         (NULL != RWSRCGLOBAL(curWorld)) &&
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

    RwD3D9SetRenderState(D3DRS_LIGHTING, FALSE);

    RWRETURNVOID();
}

/****************************************************************************
*/
static void
NormalMapWorldSectorLightingCallBack(void *object)
{
    RpGeometryFlag  flags;

    RWFUNCTION(RWSTRING("NormalMapWorldSectorLightingCallBack"));

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe Check
	THREADSAFE_CHECK_ISCALLEDMAIN();
	//@} DDonSS

    flags = RpWorldGetFlags((const RpWorld *)RWSRCGLOBAL(curWorld));

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

    if ( (flags & rxGEOMETRY_LIGHT) != 0 &&
         (_rpNormMapMainLightActive == NULL ||
          _rpNormMapAmbientLightActive == NULL) )
    {
        const RpWorld *world = (const RpWorld *)RWSRCGLOBAL(curWorld);
        const RpWorldSector *sector;
        const RwLLLink      *cur, *end;

        sector = (const RpWorldSector *)object;

        /*
         * Global lights, (Directional & Ambient)
         */

        cur = rwLinkListGetFirstLLLink(&world->directionalLightList);
        end = rwLinkListGetTerminator(&world->directionalLightList);
        while (cur != end)
        {
            const RpLight *light;

            light = rwLLLinkGetData(cur, RpLight, inWorld);

            /* NB light may actually be a dummyTie from a enclosing ForAll */
            if (light && (rwObjectTestFlags(light, rpLIGHTLIGHTWORLD)))
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
            cur = rwLinkListGetFirstLLLink(&sector->lightsInWorldSector);
            end = rwLinkListGetTerminator(&sector->lightsInWorldSector);
            while (cur != end)
            {
                const RpLightTie *lightTie;

                lightTie = rwLLLinkGetData(cur, RpLightTie, lightInWorldSector);

                /* NB lightTie may actually be a dummyTie from a enclosing ForAll */

                /* Check to see if the light is set to light worlds */
                if (lightTie->light &&
                    (rwObjectTestFlags(lightTie->light, rpLIGHTLIGHTWORLD)))
                {
                    /* We are just using one of them !!! */
                    NormalMapMainLight = lightTie->light;
                    break;
                }

                /* Next */
                cur = rwLLLinkGetNext(cur);
           }
        }
    }

    RwD3D9SetRenderState(D3DRS_LIGHTING, FALSE);

    RWRETURNVOID();
}

/*
 ***************************************************************************
 */
static void
NormalMapRenderCallBack(RwResEntry *repEntry,
                        void *object,
                        RwUInt8 type,
                        RwUInt32 flags)
{
    const RxD3D9ResEntryHeader  *resEntryHeader;
    const RxD3D9InstanceData    *instancedData;

    RWFUNCTION(RWSTRING("NormalMapRenderCallBack"));
    RWASSERT(NormalMapSupported != FALSE);

    resEntryHeader = (const RxD3D9ResEntryHeader *)(repEntry + 1);

	//@{ 20050513 DDonSS : Threadsafe
	// ResEntry Lock
	CS_RESENTRYHEADER_LOCK( (RxD3D9ResEntryHeader*)resEntryHeader );
	//@} DDonSS
	
	//>@ 2005.3.31 gemani
	if(!resEntryHeader->isLive)					//validation check
	{
		//@{ 20050513 DDonSS : Threadsafe
		// ResEntry Unlock
		CS_RESENTRYHEADER_UNLOCK( ( (RxD3D9ResEntryHeader*)resEntryHeader ) );
		//@} DDonSS

		RWRETURNVOID();	
	}
	//<@

    /*
     * Set Indices
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
    
    /* Shared states */
    RwD3D9SetTexture(NULL, 2);
    RwD3D9SetSamplerState(2, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    RwD3D9SetSamplerState(2, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

    /* Check light */
    if (NormalMapMainLight != NULL)
    {
        _rpD3D9VertexShaderDescriptor   desc;
        _rpD3D9VertexShaderDispatchDescriptor dispatch;
        void *vertexShader;

        const NormalMapExt  *normalmapext;
        const RwMatrix  *matrixLight;
        RwUInt32 normalmapTexCoors;

        NormalMapPixelShaderConstants constantsPS;
        NormalMapVertexShaderConstants constantsVS;
        NormalMapMatrixTransposed envMapMatrix;

        const RwBool isPrelit = ((flags & rxGEOMETRY_PRELIT) != 0);

        RwReal  LastDifuse = FLT_MAX;
        RwReal  LastAmbient = FLT_MAX;
        RwUInt32 LastColor = 0;
        RwTexture *LastNormalMap = NULL;
        RwTexture *LastEnvMap = (RwTexture *)0xffffffff; /* to force update */
        RwFrame *LastEnvMapFrame = (RwFrame *)0xffffffff; /* to force update */
        void *lastPixelShader = NULL;
        void *currentPixelShader = NULL;
        RwInt32     numMeshes;

        /* Initialize descriptor */
        _rxD3D9VertexShaderDefaultBeginCallBack(object, type, &desc);

        /* Get the tex coordinates for the normal map (the last one) */
        if (type == rpATOMIC)
        {
            const RpGeometry *geom = RpAtomicGetGeometry((RpAtomic *)object);
            RwFrame *frame;
            const RwMatrix  *ltm;

            normalmapTexCoors = RpGeometryGetNumTexCoordSets(geom) - 1;

            frame = RpAtomicGetFrame((RpAtomic *)object);
            ltm = RwFrameGetLTM(frame);

            _rwD3D9VSSetActiveWorldMatrix(ltm);
        }
        else
        {
            const RpWorld *world = RpWorldSectorGetWorld((RpWorldSector *)object);

            normalmapTexCoors = world->numTexCoordSets - 1;

            _rwD3D9VSSetActiveWorldMatrix(NULL);
        }

        _rwD3D9VSGetComposedTransformMatrix(&constantsVS.worldViewProjectionTransposed);

        /* Get light data */
        matrixLight = RwFrameGetLTM(RpLightGetFrame(NormalMapMainLight));

        if (RpLightGetType(NormalMapMainLight) == rpLIGHTDIRECTIONAL)
        {
            const RwV3d     *dirLight;

            dirLight = &(matrixLight->at);

            if (type == rpATOMIC)
            {
                RwV3d   dirLightObjectSpace;
                RwReal  unused;

                _rwD3D9VSGetNormalInLocalSpace(dirLight, &dirLightObjectSpace);

                constantsVS.lightData0.x = -(dirLightObjectSpace.x);
                constantsVS.lightData0.y = -(dirLightObjectSpace.y);
                constantsVS.lightData0.z = -(dirLightObjectSpace.z);

                _rwV3dNormalizeMacro(unused,
                                     (RwV3d *)&constantsVS.lightData0,
                                     (const RwV3d *)&constantsVS.lightData0);
            }
            else
            {
                constantsVS.lightData0.x = -(dirLight->x);
                constantsVS.lightData0.y = -(dirLight->y);
                constantsVS.lightData0.z = -(dirLight->z);
            }

            constantsVS.lightData0.w = 0.5f;

            RwD3D9SetVertexShaderConstant(0, &constantsVS, 6);

            desc.numDirectionalLights = 1;
        }
        else
        {
            RwV4d           *currentConstant;
            const RwV3d     *posLight;
            RwReal radiusLight;

            currentConstant = &(constantsVS.lightData0);
            posLight = &(matrixLight->pos);

            /* Get radious */
            radiusLight = RpLightGetRadius(NormalMapMainLight);

            _rwD3D9VSGetRadiusInLocalSpace(radiusLight, &radiusLight);

            currentConstant->x = -(1.f / radiusLight);
            currentConstant->y = 1.0f;
            currentConstant->z = 0.5f;
            currentConstant->w = 0.0f;
            ++currentConstant;

            /* Get pos */
            if (type == rpATOMIC)
            {
                _rwD3D9VSGetPointInLocalSpace(posLight, (RwV3d *)currentConstant);
            }
            else
            {
                currentConstant->x = (posLight->x);
                currentConstant->y = (posLight->y);
                currentConstant->z = (posLight->z);
            }
            currentConstant->w = 0.5f;
            ++currentConstant;

            if (RpLightGetType(NormalMapMainLight) == rpLIGHTSPOT ||
                RpLightGetType(NormalMapMainLight) == rpLIGHTSPOTSOFT)
            {
                const RwV3d     *dirLight;
                RwReal angle;
                RwReal cosangle;

                dirLight = &(matrixLight->at);

                if (type == rpATOMIC)
                {
                    RwV3d   dirLightObjectSpace;
                    RwReal  unused;

                    _rwD3D9VSGetNormalInLocalSpace(dirLight, &dirLightObjectSpace);

                    constantsVS.lightSpotDir.x = -(dirLightObjectSpace.x);
                    constantsVS.lightSpotDir.y = -(dirLightObjectSpace.y);
                    constantsVS.lightSpotDir.z = -(dirLightObjectSpace.z);

                    _rwV3dNormalizeMacro(unused,
                                         (RwV3d *)&constantsVS.lightSpotDir,
                                         (const RwV3d *)&constantsVS.lightSpotDir);
                }
                else
                {
                    constantsVS.lightSpotDir.x = -(dirLight->x);
                    constantsVS.lightSpotDir.y = -(dirLight->y);
                    constantsVS.lightSpotDir.z = -(dirLight->z);
                }

                angle = RpLightGetConeAngle(NormalMapMainLight);

                cosangle = (RwReal)RwCos(angle);

                if (RpLightGetType(NormalMapMainLight) == rpLIGHTSPOT ||
                    cosangle >= 0.999f)
                {
                    constantsVS.lightAngleCoefs.x = (1.f / 0.001f);
                    constantsVS.lightAngleCoefs.y = (-cosangle) *
                                                    constantsVS.lightAngleCoefs.x;
                    constantsVS.lightAngleCoefs.z = 0.5f;
                    constantsVS.lightAngleCoefs.w = 0.0f;
                }
                else
                {
                    constantsVS.lightAngleCoefs.x = (1.f / (1.f - cosangle));
                    constantsVS.lightAngleCoefs.y = (-cosangle) *
                                                    constantsVS.lightAngleCoefs.x;
                    constantsVS.lightAngleCoefs.z = 0.5f;
                    constantsVS.lightAngleCoefs.w = 0.0f;
                }

                RwD3D9SetVertexShaderConstant(0, &constantsVS, 9);

                desc.numSpotLights = 1;
            }
            else
            {
                RwD3D9SetVertexShaderConstant(0, &constantsVS, 7);

                desc.numPointLights = 1;
            }
        }

        /* Get the instanced data */
        instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

        numMeshes = resEntryHeader->numMeshes;
        while (numMeshes--)
        {
            const RwSurfaceProperties *surfProp;
            const RwRGBA *materialcolor;

            surfProp = &instancedData->material->surfaceProps;
            materialcolor = &instancedData->material->color;

            if (instancedData->vertexAlpha ||
                (0xFF != materialcolor->alpha))
            {
                RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE);
            }
            else
            {
                RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)FALSE);
            }

            normalmapext = CONSTMATERIALGETNORMALMAP(instancedData->material);

            /* Check pixel shader */
            if ( normalmapext->envMap != NULL)
            {
                if (instancedData->material->texture != NULL)
                {
                    if (isPrelit)
                    {
                        if (normalmapext->modulateEnvMap)
                        {
                            currentPixelShader = NormalMapEnvMapMetalPrelitPixelShader;
                        }
                        else
                        {
                            currentPixelShader = NormalMapEnvMapPrelitPixelShader;
                        }

                        RwD3D9SetRenderState(D3DRS_SPECULARENABLE, TRUE);
                    }
                    else
                    {
                        if (normalmapext->modulateEnvMap)
                        {
                            currentPixelShader = NormalMapEnvMapMetalPixelShader;
                        }
                        else
                        {
                            currentPixelShader = NormalMapEnvMapPixelShader;
                        }

                        RwD3D9SetRenderState(D3DRS_SPECULARENABLE, FALSE);
                    }
                }
                else
                {
                    if (isPrelit)
                    {
                        currentPixelShader = NormalMapEnvMapPrelitNoTexturePixelShader;

                        RwD3D9SetRenderState(D3DRS_SPECULARENABLE, TRUE);
                    }
                    else
                    {
                        currentPixelShader = NormalMapEnvMapNoTexturePixelShader;

                        RwD3D9SetRenderState(D3DRS_SPECULARENABLE, FALSE);
                    }
                }
            }
            else
            {
                if (instancedData->material->texture != NULL)
                {
                    if (isPrelit)
                    {
                        currentPixelShader = NormalMapPrelitPixelShader;
                    }
                    else
                    {
                        currentPixelShader = NormalMapPixelShader;
                    }
                }
                else
                {
                    if (isPrelit)
                    {
                        currentPixelShader = NormalMapPrelitNoTexturePixelShader;
                    }
                    else
                    {
                        currentPixelShader = NormalMapNoTexturePixelShader;
                    }
                }

                RwD3D9SetRenderState(D3DRS_SPECULARENABLE, FALSE);
            }

            if (lastPixelShader != currentPixelShader)
            {
                lastPixelShader = currentPixelShader;

                RwD3D9SetPixelShader(currentPixelShader);

                LastDifuse = FLT_MAX;
            }

            /* Check vertex shader */
            if (LastEnvMap != normalmapext->envMap)
            {
                if ( normalmapext->envMap != NULL)
                {
                    desc.effect = rwD3D9VERTEXSHADEREFFECT_NORMALENVMAP;
                }
                else
                {
                    desc.effect = rwD3D9VERTEXSHADEREFFECT_NORMALMAP;
                }

                vertexShader = _rpD3D9GetVertexShader(&desc, &dispatch);

                RwD3D9SetVertexShader(vertexShader);
            }

            /* Set envmap matrix */
            if (normalmapext->envMap != NULL &&
                LastEnvMapFrame != normalmapext->envmapFrame)
            {
                LastEnvMapFrame = normalmapext->envmapFrame;

                if (normalmapext->envmapFrame != NULL)
                {
                    RwMatrix    *envMtx;
                    RwMatrix    invMtx;
                    RwMatrix    tmpMtx;

                    /* Transfrom the normals by the inverse of the env maps frame */
                    envMtx = RwFrameGetLTM(normalmapext->envmapFrame);

                    if (!rwMatrixTestFlags(envMtx, rwMATRIXTYPEORTHONORMAL))
                    {
                        RwMatrix    normMtx;

                        RwMatrixOrthoNormalize(&normMtx, envMtx);

                        RwMatrixInvert(&invMtx, &normMtx);
                    }
                    else
                    {
                        RwMatrixInvert(&invMtx, envMtx);
                    }

                    RwMatrixMultiply(&tmpMtx, &invMtx, &NormalMapEnvMapTexMat);

                    _rwD3D9VSGetWorldNormalizedMultiplyTransposeMatrix(&envMapMatrix,
                                                                       &tmpMtx);
                }
                else
                {
                    _rwD3D9VSGetWorldNormalizedViewMultiplyTransposeMatrix(&envMapMatrix,
                                                                           &NormalMapEnvMapTexMat);
                }

                RwD3D9SetVertexShaderConstant(dispatch.offsetEffect, &envMapMatrix, 2);
            }

            /* Set lights colors */
            if (LastDifuse != surfProp->diffuse ||
                LastAmbient != surfProp->ambient ||
                LastColor != *((const RwUInt32 *)(materialcolor)))
            {
                const RwRGBAReal *color = RpLightGetColor(NormalMapMainLight);

                LastDifuse = surfProp->diffuse;
                LastAmbient = surfProp->ambient;
                LastColor = *((const RwUInt32 *)(materialcolor));

                if ((flags & rxGEOMETRY_MODULATE) != 0 &&
                    LastColor != 0xffffffff)
                {
                    const RwReal diffusseCoef = (LastDifuse) / 255.f;
                    const RwReal ambientCoef = (LastAmbient) / 255.f;

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
                    constantsPS.colorDirLight.x = (color->red) * LastDifuse;
                    constantsPS.colorDirLight.y = (color->green) * LastDifuse;
                    constantsPS.colorDirLight.z = (color->blue) * LastDifuse;
                    constantsPS.colorDirLight.w = 1.0f;

                    constantsPS.colorAmbientLight.x = (NormalMapAmbientColor.red) * LastAmbient;
                    constantsPS.colorAmbientLight.y = (NormalMapAmbientColor.green) * LastAmbient;
                    constantsPS.colorAmbientLight.z = (NormalMapAmbientColor.blue) * LastAmbient;
                    constantsPS.colorAmbientLight.w = normalmapext->shininess;
                }

                RwD3D9SetPixelShaderConstant(0, &constantsPS, sizeof(constantsPS) / sizeof(RwV4d));
            }

            /* Set textures */
            if (LastNormalMap != normalmapext->normalMap)
            {
                LastNormalMap = normalmapext->normalMap;

                RwD3D9SetTexture(normalmapext->normalMap, 0);
            }

            RwD3D9SetTexture(instancedData->material->texture, 1);

            if (LastEnvMap != normalmapext->envMap)
            {
                LastEnvMap = normalmapext->envMap;

                if (normalmapext->envMap != NULL)
                {
                    RwD3D9SetTexture(normalmapext->envMap, 3);
                }
                else
                {
                    RwD3D9SetTexture(NULL, 3);
                }
            }

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
            ++instancedData;
        }

        /*
         * Set the Default Pixel shader
         */
        RwD3D9SetPixelShader(0);

        RwD3D9SetTexture(NULL, 1);

        RwD3D9SetTexture(NULL, 2);

        RwD3D9SetTexture(NULL, 3);

        RwD3D9SetRenderState(D3DRS_SPECULARENABLE, FALSE);
    }
    else
    {
        RwReal      LastAmbient = FLT_MAX;
        RwUInt32    LastColor = 0;
        RwUInt32    tFactor = 0;
        RwInt32     numMeshes;

        /* Get the instanced data */
        instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

        numMeshes = resEntryHeader->numMeshes;
        while (numMeshes--)
        {
            const RwSurfaceProperties *surfProp;
            const RwRGBA *materialcolor;

            if (instancedData->vertexAlpha ||
                (0xFF != instancedData->material->color.alpha))
            {
                RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE);
            }
            else
            {
                RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)FALSE);
            }

            surfProp = &instancedData->material->surfaceProps;
            materialcolor = &instancedData->material->color;

            if (LastAmbient != surfProp->ambient ||
                LastColor != *((const RwUInt32 *)(materialcolor)))
            {
                LastAmbient = surfProp->ambient;
                LastColor = *((const RwUInt32 *)(materialcolor));

                if ((flags & rxGEOMETRY_MODULATE) != 0 &&
                    LastColor != 0xffffffff)
                {
                    tFactor  = RwFastRealToUInt32((NormalMapAmbientColor.red) * (RwReal)(materialcolor->red) * LastAmbient)     << 16;
                    tFactor |= RwFastRealToUInt32((NormalMapAmbientColor.green) * (RwReal)(materialcolor->green) * LastAmbient) << 8;
                    tFactor |= RwFastRealToUInt32((NormalMapAmbientColor.blue) * (RwReal)(materialcolor->blue) * LastAmbient)   << 0;
                    tFactor |= (materialcolor->alpha) << 24;
                }
                else
                {
                    const RwReal ambientCoef = (LastAmbient * 255.f);

                    tFactor  = RwFastRealToUInt32((NormalMapAmbientColor.red) * ambientCoef)   << 16;
                    tFactor |= RwFastRealToUInt32((NormalMapAmbientColor.green) * ambientCoef) << 8;
                    tFactor |= RwFastRealToUInt32((NormalMapAmbientColor.blue) * ambientCoef)  << 0;
                    tFactor |= (0xff) << 24;
                }

                RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, tFactor);
            }

            if (instancedData->material->texture != NULL &&
                instancedData->material->texture->raster != NULL)
            {
                if (_rwD3D9TextureHasAlpha(instancedData->material->texture) ||
                    (tFactor & 0x00ffffff) != 0)
                {
                    RwD3D9SetTexture(instancedData->material->texture, 0);

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
            ++instancedData;
        }
    }

	//@{ 20050513 DDonSS : Threadsafe
	// ResEntry Unlock
	CS_RESENTRYHEADER_UNLOCK( ( (RxD3D9ResEntryHeader*)resEntryHeader ) );
	//@} DDonSS

    RWRETURNVOID();
}

/*
 ***************************************************************************
 */
RxPipeline *
NormalMapCreateAtomicPipeline(void)
{
    RxPipeline  *pipe;

    RWFUNCTION(RWSTRING("NormalMapCreateAtomicPipeline"));

    pipe = RxPipelineCreate();
    if (pipe)
    {
        RxLockedPipe    *lpipe;

        /* Identify the pipeline with it's plugin and id. */
        pipe->pluginId = rwPLUGIN_ID;
        pipe->pluginData = rpNORMMAPATOMICSTATICPIPELINE;

        lpipe = RxPipelineLock(pipe);
        if (NULL != lpipe)
        {
            RxNodeDefinition    *instanceNode;
            RxPipelineNode      *node;

            /*
             * Get the instance node definition
             */
            instanceNode = RxNodeDefinitionGetD3D9AtomicAllInOne();

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
                * Set the render callback
                */
                RxD3D9AllInOneSetRenderCallBack(node, NormalMapRenderCallBack);

                /*
                * Set the lightinh callback
                */
                RxD3D9AllInOneSetLightingCallBack(node, NormalMapAtomicLightingCallBack);
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
RxPipeline *
NormalMapCreateWorldSectorPipeline(void)
{
    RxPipeline  *pipe;

    RWFUNCTION(RWSTRING("NormalMapCreateWorldSectorPipeline"));

    pipe = RxPipelineCreate();
    if (pipe)
    {
        RxLockedPipe    *lpipe;

        /* Identify the pipeline with it's plugin and id. */
        pipe->pluginId = rwPLUGIN_ID;
        pipe->pluginData = 0;

        lpipe = RxPipelineLock(pipe);
        if (NULL != lpipe)
        {
            RxNodeDefinition    *instanceNode;
            RxPipelineNode      *node;

            /*
             * Get the instance node definition
             */
            instanceNode = RxNodeDefinitionGetD3D9WorldSectorAllInOne();

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
                * Set the render callback
                */
                RxD3D9AllInOneSetRenderCallBack(node, NormalMapRenderCallBack);

                /*
                * Set the lightinh callback
                */
                RxD3D9AllInOneSetLightingCallBack(node, NormalMapWorldSectorLightingCallBack);
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
void *
_rpNormMapOpen(void *instance,
               RwInt32 offset __RWUNUSED__,
               RwInt32 size __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("_rpNormMapOpen"));

    if (NormalMapNumInstances == 0)
    {
        const D3DCAPS9 *d3dCaps;

        d3dCaps = (const D3DCAPS9 *)RwD3D9GetCaps();

        if ((d3dCaps->DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0 &&
            (d3dCaps->VertexShaderVersion & 0xFFFF) >= 0x0101 &&
            (d3dCaps->PixelShaderVersion & 0xFFFF) >= 0x0101) /* We require pixel shaders to remove problems with crappy video cards */
        {
            NormalMapSupported = TRUE;
        }
        else
        {
            RwDebugSendMessage(rwDEBUGMESSAGE, "Normal maps plugin", "Device does not support vertex and pixel shaders. Using default render callback.");

            NormalMapSupported = FALSE;
        }

        if (NormalMapCreatePipelines() == FALSE)
        {
            RWRETURN(NULL);
        }

        /*
         * Create the pixel shaders
         */
        if (NormalMapSupported)
        {
            RwD3D9CreatePixelShader(dwDot3PixelShader,
                                    &NormalMapPixelShader);

            RwD3D9CreatePixelShader(dwDot3prelitPixelShader,
                                    &NormalMapPrelitPixelShader);

            RwD3D9CreatePixelShader(dwDot3notexturePixelShader,
                                    &NormalMapNoTexturePixelShader);

            RwD3D9CreatePixelShader(dwDot3prelitnotexturePixelShader,
                                    &NormalMapPrelitNoTexturePixelShader);

            if ((d3dCaps->PixelShaderVersion & 0xFFFF) >= 0x0104)
            {
                RwD3D9CreatePixelShader(dwDot3envmap14PixelShader,
                                        &NormalMapEnvMapPixelShader);

                RwD3D9CreatePixelShader(dwDot3envmapprelit14PixelShader,
                                        &NormalMapEnvMapPrelitPixelShader);

                RwD3D9CreatePixelShader(dwDot3envmapnotexture14PixelShader,
                                        &NormalMapEnvMapNoTexturePixelShader);

                RwD3D9CreatePixelShader(dwDot3envmapprelitnotexture14PixelShader,
                                        &NormalMapEnvMapPrelitNoTexturePixelShader);

                RwD3D9CreatePixelShader(dwDot3envmapmetal14PixelShader,
                                        &NormalMapEnvMapMetalPixelShader);

                RwD3D9CreatePixelShader(dwDot3envmapmetalprelit14PixelShader,
                                        &NormalMapEnvMapMetalPrelitPixelShader);
            }
            else
            {
                RwD3D9CreatePixelShader(dwDot3envmapPixelShader,
                                        &NormalMapEnvMapPixelShader);

                RwD3D9CreatePixelShader(dwDot3envmapprelitPixelShader,
                                        &NormalMapEnvMapPrelitPixelShader);

                RwD3D9CreatePixelShader(dwDot3envmapnotexturePixelShader,
                                        &NormalMapEnvMapNoTexturePixelShader);

                RwD3D9CreatePixelShader(dwDot3envmapprelitnotexturePixelShader,
                                        &NormalMapEnvMapPrelitNoTexturePixelShader);

                RwD3D9CreatePixelShader(dwDot3envmapmetalPixelShader,
                                        &NormalMapEnvMapMetalPixelShader);

                RwD3D9CreatePixelShader(dwDot3envmapmetalprelitPixelShader,
                                        &NormalMapEnvMapMetalPrelitPixelShader);
            }
        }
    }

    ++NormalMapNumInstances;

    RWRETURN(instance);
}

/*
 ***************************************************************************
 */
void *
_rpNormMapClose(void *instance,
                RwInt32 offset __RWUNUSED__,
                RwInt32 size __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("_rpNormMapClose"));

    --NormalMapNumInstances;

    if (NormalMapNumInstances <= 0)
    {
        /* Destroy pixel shaders */
        if (NormalMapEnvMapMetalPixelShader)
        {
            RwD3D9DeletePixelShader(NormalMapEnvMapMetalPixelShader);
            NormalMapEnvMapMetalPixelShader = NULL;
        }

        if (NormalMapEnvMapMetalPrelitPixelShader)
        {
            RwD3D9DeletePixelShader(NormalMapEnvMapMetalPrelitPixelShader);
            NormalMapEnvMapMetalPrelitPixelShader = NULL;
        }

        if (NormalMapEnvMapPrelitNoTexturePixelShader)
        {
            RwD3D9DeletePixelShader(NormalMapEnvMapPrelitNoTexturePixelShader);
            NormalMapEnvMapPrelitNoTexturePixelShader = NULL;
        }

        if (NormalMapEnvMapNoTexturePixelShader)
        {
            RwD3D9DeletePixelShader(NormalMapEnvMapNoTexturePixelShader);
            NormalMapEnvMapNoTexturePixelShader = NULL;
        }

        if (NormalMapPrelitNoTexturePixelShader)
        {
            RwD3D9DeletePixelShader(NormalMapPrelitNoTexturePixelShader);
            NormalMapPrelitNoTexturePixelShader = NULL;
        }

        if (NormalMapNoTexturePixelShader)
        {
            RwD3D9DeletePixelShader(NormalMapNoTexturePixelShader);
            NormalMapNoTexturePixelShader = NULL;
        }

        if (NormalMapEnvMapPrelitPixelShader)
        {
            RwD3D9DeletePixelShader(NormalMapEnvMapPrelitPixelShader);
            NormalMapEnvMapPrelitPixelShader = NULL;
        }

        if (NormalMapEnvMapPixelShader)
        {
            RwD3D9DeletePixelShader(NormalMapEnvMapPixelShader);
            NormalMapEnvMapPixelShader = NULL;
        }

        if (NormalMapPrelitPixelShader)
        {
            RwD3D9DeletePixelShader(NormalMapPrelitPixelShader);
            NormalMapPrelitPixelShader = NULL;
        }

        if (NormalMapPixelShader)
        {
            RwD3D9DeletePixelShader(NormalMapPixelShader);
            NormalMapPixelShader = NULL;
        }

        NormalMapDestroyPipelines();
    }

    RWRETURN(instance);
}

/*
 ***************************************************************************
 */
static RpWorldSector *
NormmapEnableWorldSectorCallBack(RpWorldSector *worldSector,
                           void *data)
{
    RpD3D9WorldSectorUsageFlag flags;

    RWFUNCTION(RWSTRING("NormmapEnableWorldSectorCallBack"));
    RWASSERT(worldSector != NULL);

    flags = RpD3D9WorldSectorGetUsageFlags(worldSector);

    flags |= rpD3D9WORLDSECTORUSAGE_CREATETANGENTS;

    RpD3D9WorldSectorSetUsageFlags(worldSector, flags);

    RWRETURN(worldSector);
}

/*
 ***************************************************************************
 */
void
RpNormMapWorldEnable(RpWorld *world)
{
    ObjectNormalMapExt *worldData;
    RWAPIFUNCTION(RWSTRING("RpNormMapWorldEnable"));
    RWASSERT(world != NULL);

    worldData = WORLDGETNORMALMAP(world);

    if (worldData->enabled == FALSE)
    {
        if (NormalMapSupported)
        {
            RpWorldForAllWorldSectors(world,
                                    NormmapEnableWorldSectorCallBack,
                                    NULL);
        }

        worldData->enabled = TRUE;
    }

    RWRETURNVOID();
}
