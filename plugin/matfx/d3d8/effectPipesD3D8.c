/*----------------------------------------------------------------------*
 *                                                                      *
 * Module  :                                                            *
 *                                                                      *
 * Purpose :                                                            *
 *                                                                      *
 * FX      :                                                            *
 *----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*
 *-   Includes                                                         -*
 *----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*==== D3D8 includes ====*/
#include <d3d8.h>

/*==== RW libs includes ====*/
#include "rpplugin.h"
#include <rpdbgerr.h>
#include <rwcore.h>

#include "matfx.h"
#include "matfxplatform.h"
#include "effectpipes.h"

#include "BumpMapPShader.h"
#include "EnvMapPShader.h"
#include "EnvMapNoBaseTexturePShader.h"
#include "BumpEnvMapPShader.h"
#include "BumpModulatedEnvMapPShader.h"
#include "BumpModulatedEnvMapNoAlphaPShader.h"

/**
 * \ingroup rpmatfxd3d8
 * \page rpmatfxd3d8overview D3D8 RpMatFX Overview
 *
 * The \ref rpmatfx plugin functions correctly under RWD3D8. In addition, RWD3D8 now takes
 * care of the type and position of lights when rendering bump maps.
 *
 * \ref rpmatfx will also detect if the video hardware supports pixel and vertex shaders.
 * If found, pixel shaders are used to increase efficiency of the rendering.
 *
 * When using the environment map effect with a non-opaque base texture, RpMatFX on D3D8 is forced
 * to use dual pass to achieve the desired result. To enable multitexturing but getting different
 * results, you need to enable the use of the frame buffer alpha through the functions
 * \ref RpMatFXMaterialSetEnvMapFrameBufferAlpha and \ref RpMatFXMaterialSetupEnvMap.
 *
 * RWD3D8 always tries to use multitexturing when possible but, due to a limitation
 * of the D3D8 API, only one palette is active when rendering. That
 * means we need to render the effect in two passes when two or more palettized textures
 * are used in that effect.
 * We highly recommend you do not use palettized textures at all,
 * use \ref compressedtextures instead.
 *
 * If you want to use \ref compressedtextures with the bump mapping effect, you
 * need to encode the bumpmap into the alpha channel of the base texture, and to
 * pass NULL as the bump texture parameter to the functions
 * \ref RpMatFXMaterialSetupBumpMap or \ref RpMatFXMaterialSetBumpMapTexture.
 *
 */

#define BUMPMAP_FVF                             \
    (D3DFVF_XYZ |                               \
     D3DFVF_NORMAL |                            \
     D3DFVF_DIFFUSE |                           \
     D3DFVF_TEX2 |                              \
     D3DFVF_TEXCOORDSIZE2(0) |                  \
     D3DFVF_TEXCOORDSIZE2(1))

/*===========================================================================*
 *--- Public Global Variables -----------------------------------------------*
 *===========================================================================*/

/* I'm including that here to not add the "d3d8.h" header to the effectPipesD3D8.h */
extern LPDIRECT3DDEVICE8    _RwD3DDevice;

/*----------------------------------------------------------------------*
 *-   Local Types                                                      -*
 *----------------------------------------------------------------------*/

typedef struct _rxD3D8BumpMapVertex RxD3D8BumpMapVertex;
struct _rxD3D8BumpMapVertex
{
    RwV3d       position;
    RwV3d       normal;
    RwUInt32    color;
    RwTexCoords texcoords1;
    RwTexCoords texcoords2;
};

/*----------------------------------------------------------------------*
 *-   Local/static Globals                                             -*
 *----------------------------------------------------------------------*/

static RxPipeline   *MatFXAtomicPipe = NULL,
                    *MatFXWorldSectorPipe = NULL;

static RwUInt32     BumpMapPixelShader = 0;

static RwUInt32     EnvMapPixelShader = 0;
static RwUInt32     EnvMapNoBaseTexturePixelShader = 0;

static RwUInt32     BumpEnvMapPixelShader = 0;

static RwUInt32     BumpModulatedEnvMapPixelShader = 0;
static RwUInt32     BumpModulatedEnvMapNoAlphaPixelShader = 0;

static RwBool       VideoCardSupportsMultitexture = FALSE;
static RwBool       VideoCardSupportsMultiplyAdd = FALSE;
static RwBool       VideoCardSupportsModulate2X = FALSE;
static RwBool       VideoCardSupportsDiffuseInStage0 = FALSE;
static RwUInt32     VideoCardMaxTextureBlendStages = 0;

static RwBool       NeedToValidateModulate2X = TRUE;
static RwBool       NeedToValidateDiffuseInStage0 = TRUE;

/*----------------------------------------------------------------------*
 *-   Functions                                                        -*
 *----------------------------------------------------------------------*/
/********************************************************/

/****************************************************************************
 FrameGetFirstLight

 Purpose: Find and return the first light that owns the frame

 */
static const RpLight *
FrameGetFirstLight(const RwFrame *frame)
{
    const RpLight         *light = NULL;
    const RwLLLink        *current, *next, *end;
    int objcount = 0;

    RWFUNCTION(RWSTRING("FrameGetFirstLight"));

    current = rwLinkListGetFirstLLLink(&frame->objectList);
    end = rwLinkListGetTerminator(&frame->objectList);

    while (current != end)
    {
        const RwObject *object;

        next = rwLLLinkGetNext(current);
        object = (const RwObject *)
            rwLLLinkGetConstData(current,RwObjectHasFrame, lFrame);

        if (RwObjectGetType(object) == rpLIGHT)
        {
            light = (const RpLight *)object;

            break;
        }

        objcount++;
        current = next;
    }

    RWRETURN((light));
}

/****************************************************************************
 CalculatePerturbedUVs

 */
static RwBool
CalculatePerturbedUVs(RxD3D8InstanceData *instancedData,
                    RxD3D8BumpMapVertex *bumpVertex)
{
    const MatFXBumpMapData  *bumpMapData = MATFXD3D8BUMPMAPGETDATA(instancedData->material);
    const RwReal            factor = bumpMapData->coef * bumpMapData->invBumpWidth;
    RwFrame                 *bumpFrame  = bumpMapData->frame;
    RwInt32                 i;
    RwMatrix                objToWorld;
    RwMatrix                worldToObj;
    const RpLight           *light;
    RwV3d                   lightPosObj;
    RwReal                  *lockedVB;
    RwInt32                 texCoordsOffset;

    RWFUNCTION(RWSTRING("CalculatePerturbedUVs"));

    if (bumpFrame == NULL)
    {
        bumpFrame = RwCameraGetFrame(RwCameraGetCurrentCamera());
        RWASSERT(bumpFrame);
    }

    /* Get light vector */
    RwD3D8GetTransform(D3DTS_WORLD, &objToWorld);

    RwMatrixInvert(&worldToObj, &objToWorld);

    light = FrameGetFirstLight(bumpFrame);

    if ( light != NULL &&
        (RpLightGetType(light) == rpLIGHTDIRECTIONAL ||
         RpLightGetType(light) == rpLIGHTSPOT ||
         RpLightGetType(light) == rpLIGHTSPOTSOFT) )
    {
        const RwV3d *lightAt;
        RwV3d       lightAtInv;

        lightAt = RwMatrixGetAt(RwFrameGetLTM(bumpFrame));

        lightAtInv.x = - (lightAt->x);
        lightAtInv.y = - (lightAt->y);
        lightAtInv.z = - (lightAt->z);

        RwV3dTransformVector(&lightPosObj, &lightAtInv, &worldToObj);
    }
    else
    {
        const RwV3d *lightPos;

        lightPos = RwMatrixGetPos(RwFrameGetLTM(bumpFrame));

        RwV3dTransformPoint(&lightPosObj, lightPos, &worldToObj);
    }

    /* Calculates the texture coords offset */
    texCoordsOffset = 3;

    if (instancedData->vertexShader & D3DFVF_NORMAL)
    {
        texCoordsOffset += 3;
    }

    if (instancedData->vertexShader & D3DFVF_DIFFUSE)
    {
        texCoordsOffset += 1;
    }

    /* Get pointer to the verter information */
    if (SUCCEEDED(IDirect3DVertexBuffer8_Lock((LPDIRECT3DVERTEXBUFFER8)instancedData->vertexBuffer,
                        instancedData->baseIndex * instancedData->stride,
                        instancedData->numVertices * instancedData->stride,
                        (RwUInt8 **)&lockedVB,
                        D3DLOCK_NOSYSLOCK)))
    {
        /* Process vertex */
        if (instancedData->indexBuffer)
        {
            RwUInt8                 *processedFlags;
            RwInt32                 maxIndex;
            RxVertexIndex           *inds;

            processedFlags = (RwUInt8 *) RwMalloc(instancedData->numVertices,
                rwID_MATERIALEFFECTSPLUGIN | rwMEMHINTDUR_FUNCTION);
            memset(processedFlags, 0, instancedData->numVertices);

            maxIndex = instancedData->numIndices;

            /* Get pointer to the index information */
            IDirect3DIndexBuffer8_Lock((LPDIRECT3DINDEXBUFFER8)instancedData->indexBuffer,
                                0,
                                instancedData->numIndices * sizeof(RwUInt16),
                                (RwUInt8 **)&inds,
                                D3DLOCK_NOSYSLOCK);

            for (i = 0; i < maxIndex; i++)
            {
                if (processedFlags[inds[i]] == FALSE)
                {
                    const RwReal *vert1 = NULL;
                    const RwReal *vert2 = NULL;
                    const RwReal *vert3 = NULL;
                    RwV3d   b, t, n, temp1, temp2, l;
                    RwReal  unused;

                    if (instancedData->primType == D3DPT_TRIANGLELIST)
                    {
                        vert1 = lockedVB + ((inds[i] * instancedData->stride) / sizeof(RwReal));
                        vert2 = lockedVB + ((inds[((i/3)*3) + (i+1)%3] * instancedData->stride) / sizeof(RwReal));
                        vert3 = lockedVB + ((inds[((i/3)*3) + (i+2)%3] * instancedData->stride) / sizeof(RwReal));
                    }
                    else if (instancedData->primType == D3DPT_TRIANGLESTRIP)
                    {
                        RwInt32 i1, i2, i3;
                        i1 = i;
                        if (i < 2)
                        {
                            if (i%2)
                            {
                                i2 = i + 2;
                                i3 = i + 1;
                            }
                            else
                            {
                                i2 = i + 1;
                                i3 = i + 2;
                            }
                        }
                        else
                        {
                            if (i%2)
                            {
                                i2 = i - 1;
                                i3 = i - 2;
                            }
                            else
                            {
                                i2 = i - 2;
                                i3 = i - 1;
                            }
                        }
                        vert1 = lockedVB + ((inds[i1] * instancedData->stride) / sizeof(RwReal));
                        vert2 = lockedVB + ((inds[i2] * instancedData->stride) / sizeof(RwReal));
                        vert3 = lockedVB + ((inds[i3] * instancedData->stride) / sizeof(RwReal));
                    }

                    if (instancedData->vertexShader & D3DFVF_NORMAL)
                    {
                        n.x = vert1[3];
                        n.y = vert1[4];
                        n.z = vert1[5];
                    }
                    else
                    {
                        n.x = 0;
                        n.y = 0;
                        n.z = 0;
                    }

                    RwV3dSub(&l, &lightPosObj, (const RwV3d *)vert1);
                    _rwV3dNormalizeMacro(unused, &l, &l);

                    /* Check to see whether the light is behind the triangle */
                    if (1) /* RwV3dDotProduct(&l, &n) > 0.0f) */
                    {
                        RwV2d               shift;
                        RxD3D8BumpMapVertex *currentBumpVertex;

                        /* A nice little algorithm to find the tangent vector */
                        RwV3dSub(&temp1, (const RwV3d *)vert2, (const RwV3d *)vert1);
                        RwV3dSub(&temp2, (const RwV3d *)vert3, (const RwV3d *)vert1);

                        RwV3dScale(&temp1, &temp1, vert3[texCoordsOffset+1] - vert1[texCoordsOffset+1]);
                        RwV3dScale(&temp2, &temp2, vert2[texCoordsOffset+1] - vert1[texCoordsOffset+1]);

                        RwV3dSub(&t, &temp1, &temp2);
                        _rwV3dNormalizeMacro(unused, &t, &t);
                        RwV3dCrossProduct(&b, &t, &n);

                        /*
                         * So now that we have b, t and n, we have the tangent
                         * space coordinate system axes.
                         */
                        shift.x = RwV3dDotProduct(&t, &l);
                        shift.y = RwV3dDotProduct(&b, &l);

                        currentBumpVertex = &(bumpVertex[inds[i]]);

                        currentBumpVertex->position.x = vert1[0];
                        currentBumpVertex->position.y = vert1[1];
                        currentBumpVertex->position.z = vert1[2];

                        if (instancedData->vertexShader & D3DFVF_NORMAL)
                        {
                            currentBumpVertex->normal.x = vert1[3];
                            currentBumpVertex->normal.y = vert1[4];
                            currentBumpVertex->normal.z = vert1[5];

                            if (instancedData->vertexShader & D3DFVF_DIFFUSE)
                            {
                                currentBumpVertex->color = *((const RwUInt32 *)vert1+6);
                            }
                            else
                            {
                                currentBumpVertex->color = 0xffffffff;
                            }
                        }
                        else
                        {
                            currentBumpVertex->normal.x = 0;
                            currentBumpVertex->normal.y = 0;
                            currentBumpVertex->normal.z = 0;

                            if (instancedData->vertexShader & D3DFVF_DIFFUSE)
                            {
                                currentBumpVertex->color = *((const RwUInt32 *)vert1+3);
                            }
                            else
                            {
                                currentBumpVertex->color = 0xffffffff;
                            }
                        }

                        currentBumpVertex->texcoords1.u = vert1[texCoordsOffset];
                        currentBumpVertex->texcoords1.v = vert1[texCoordsOffset+1];

                        currentBumpVertex->texcoords2.u = vert1[texCoordsOffset] - (shift.x * factor);
                        currentBumpVertex->texcoords2.v = vert1[texCoordsOffset+1] - (shift.y * factor);
                    }

                    processedFlags[inds[i]] = TRUE;
                }
            }

            IDirect3DIndexBuffer8_Unlock((LPDIRECT3DINDEXBUFFER8)instancedData->indexBuffer);

            RwFree(processedFlags);
        }
        else
        {
            const RwInt32 numVerts = instancedData->numVertices;

            for (i = 0; i < numVerts; i++)
            {
                const RwReal *vert1 = NULL;
                const RwReal *vert2 = NULL;
                const RwReal *vert3 = NULL;
                RwV3d   b, t, n, temp1, temp2, l;
                RwReal  unused;

                if (instancedData->primType == D3DPT_TRIANGLELIST)
                {
                    vert1 = lockedVB + ((i * instancedData->stride) / sizeof(RwReal));
                    vert2 = lockedVB + (((((i/3)*3) + (i+1)%3) * instancedData->stride) / sizeof(RwReal));
                    vert3 = lockedVB + (((((i/3)*3) + (i+2)%3) * instancedData->stride) / sizeof(RwReal));
                }
                else if (instancedData->primType == D3DPT_TRIANGLESTRIP)
                {
                    RwInt32 i1, i2, i3;
                    i1 = i;
                    if (i < 2)
                    {
                        if (i%2)
                        {
                            i2 = i + 2;
                            i3 = i + 1;
                        }
                        else
                        {
                            i2 = i + 1;
                            i3 = i + 2;
                        }
                    }
                    else
                    {
                        if (i%2)
                        {
                            i2 = i - 1;
                            i3 = i - 2;
                        }
                        else
                        {
                            i2 = i - 2;
                            i3 = i - 1;
                        }
                    }
                    vert1 = lockedVB + ((i1 * instancedData->stride) / sizeof(RwReal));
                    vert2 = lockedVB + ((i2 * instancedData->stride) / sizeof(RwReal));
                    vert3 = lockedVB + ((i3 * instancedData->stride) / sizeof(RwReal));
                }

                if (instancedData->vertexShader & D3DFVF_NORMAL)
                {
                    n.x = vert1[3];
                    n.y = vert1[4];
                    n.z = vert1[5];
                }
                else
                {
                    n.x = 0;
                    n.y = 0;
                    n.z = 0;
                }

                RwV3dSub(&l, &lightPosObj, (const RwV3d *)vert1);
                _rwV3dNormalizeMacro(unused, &l, &l);

                /* Check to see whether the light is behind the triangle */
                if (1) /* RwV3dDotProduct(&l, &n) > 0.0f) */
                {
                    RwV2d               shift;
                    RxD3D8BumpMapVertex *currentBumpVertex;

                    /* A nice little algorithm to find the tangent vector */
                    RwV3dSub(&temp1, (const RwV3d *)vert2, (const RwV3d *)vert1);
                    RwV3dSub(&temp2, (const RwV3d *)vert3, (const RwV3d *)vert1);

                    RwV3dScale(&temp1, &temp1, vert3[texCoordsOffset+1] - vert1[texCoordsOffset+1]);
                    RwV3dScale(&temp2, &temp2, vert2[texCoordsOffset+1] - vert1[texCoordsOffset+1]);

                    RwV3dSub(&t, &temp1, &temp2);
                    _rwV3dNormalizeMacro(unused, &t, &t);
                    RwV3dCrossProduct(&b, &t, &n);

                    /*
                     * So now that we have b, t and n, we have the tangent
                     * space coordinate system axes.
                     */
                    shift.x = RwV3dDotProduct(&t, &l);
                    shift.y = RwV3dDotProduct(&b, &l);

                    currentBumpVertex = &(bumpVertex[i]);

                    currentBumpVertex->position.x = vert1[0];
                    currentBumpVertex->position.y = vert1[1];
                    currentBumpVertex->position.z = vert1[2];

                    if (instancedData->vertexShader & D3DFVF_NORMAL)
                    {
                        currentBumpVertex->normal.x = vert1[3];
                        currentBumpVertex->normal.y = vert1[4];
                        currentBumpVertex->normal.z = vert1[5];

                        if (instancedData->vertexShader & D3DFVF_DIFFUSE)
                        {
                            currentBumpVertex->color = *((const RwUInt32 *)vert1+6);
                        }
                        else
                        {
                            currentBumpVertex->color = 0xffffffff;
                        }
                    }
                    else
                    {
                        currentBumpVertex->normal.x = 0;
                        currentBumpVertex->normal.y = 0;
                        currentBumpVertex->normal.z = 0;

                        if (instancedData->vertexShader & D3DFVF_DIFFUSE)
                        {
                            currentBumpVertex->color = *((const RwUInt32 *)vert1+3);
                        }
                        else
                        {
                            currentBumpVertex->color = 0xffffffff;
                        }
                    }

                    currentBumpVertex->texcoords1.u = vert1[texCoordsOffset];
                    currentBumpVertex->texcoords1.v = vert1[texCoordsOffset+1];

                    currentBumpVertex->texcoords2.u = vert1[texCoordsOffset] - (shift.x * factor);
                    currentBumpVertex->texcoords2.v = vert1[texCoordsOffset+1] - (shift.y * factor);
                }
            }
        }

        IDirect3DVertexBuffer8_Unlock((LPDIRECT3DVERTEXBUFFER8)instancedData->vertexBuffer);
    }

    RWRETURN(TRUE);
}


/****************************************************************************
 ApplyEnvMapTextureMatrix

 */
static RwBool
ApplyEnvMapTextureMatrix(RwTexture *texture, RwUInt32 stage, RwFrame *frame)
{
    static const RwMatrix texMat =
    {
        {((RwReal)0.5), ((RwReal)0.0), ((RwReal)0.0)}, 0,
        {((RwReal)0.0),-((RwReal)0.5), ((RwReal)0.0)}, 0,
        {((RwReal)0.0), ((RwReal)0.0), ((RwReal)1.0)}, 0,
        {((RwReal)0.5), ((RwReal)0.5), ((RwReal)0.0)}, 0
    };

    RWFUNCTION(RWSTRING("ApplyEnvMapTextureMatrix"));
    RWASSERT(texture);

    RwD3D8SetTexture(texture, stage);

    /* Check cube textures */
    if (_rwD3D8RasterIsCubeRaster(RwTextureGetRaster(texture)))
    {
        const RwMatrix    *camMtx;
        RwMatrix          invMtx;

        RwD3D8SetTextureStageState(stage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3);
        RwD3D8SetTextureStageState(stage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);

        /* Transform the refelctions back into world space */
        camMtx = RwFrameGetLTM(RwCameraGetFrame(RwCameraGetCurrentCamera()));

        invMtx.right.x = -camMtx->right.x;
        invMtx.right.y = -camMtx->right.y;
        invMtx.right.z = -camMtx->right.z;

        invMtx.up.x = camMtx->up.x;
        invMtx.up.y = camMtx->up.y;
        invMtx.up.z = camMtx->up.z;

        invMtx.at.x = camMtx->at.x;
        invMtx.at.y = camMtx->at.y;
        invMtx.at.z = camMtx->at.z;

        invMtx.pos.x = 0.0f;
        invMtx.pos.y = 0.0f;
        invMtx.pos.z = 0.0f;

        invMtx.flags = 0;
        invMtx.pad1 = 0;
        invMtx.pad2 = 0;
        *((RwReal *)&(invMtx.pad3)) = 1.0f;

        RwD3D8SetTransform(D3DTS_TEXTURE0 + stage, (const D3DMATRIX *)&invMtx);
    }
    else
    {
        /* Generate spheremap texture coords from the position */
        RwD3D8SetTextureStageState(stage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
        RwD3D8SetTextureStageState(stage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACENORMAL);

        if (frame)
        {
            const RwMatrix    *camMtx;
            const RwMatrix    *envMtx;
            RwMatrix    invMtx;
            RwMatrix    tmpMtx;
            RwMatrix    result;

            /* Transform the normals back into world space */
            camMtx = RwFrameGetLTM(RwCameraGetFrame(RwCameraGetCurrentCamera()));

            /* Transfrom the normals by the inverse of the env maps frame */
            envMtx = RwFrameGetLTM(frame);

            RwMatrixInvert(&invMtx, envMtx);

            RwMatrixMultiply(&tmpMtx, &invMtx, camMtx);

            tmpMtx.right.x = -tmpMtx.right.x;
            tmpMtx.right.y = -tmpMtx.right.y;
            tmpMtx.right.z = -tmpMtx.right.z;

            tmpMtx.flags = 0;

            tmpMtx.pos.x = 0.0f;
            tmpMtx.pos.y = 0.0f;
            tmpMtx.pos.z = 0.0f;

            RwMatrixMultiply(&result, &tmpMtx, &texMat);

            RwD3D8SetTransform(D3DTS_TEXTURE0 + stage, &result);
        }
        else
        {
            RwD3D8SetTransform(D3DTS_TEXTURE0 + stage, &texMat);
        }
    }

    RWRETURN(TRUE);
}

/****************************************************************************
 ApplyAnimTextureMatrix

 */
static void
ApplyAnimTextureMatrix(RwUInt32 stage, RwMatrix *matrix)
{
    static RwMatrix texMat =
    {
        {0.0f, 0.0f, 0.0f}, 0,
        {0.0f, 0.0f, 0.0f}, 0,
        {0.0f, 0.0f, 1.0f}, 0,
        {0.0f, 0.0f, 0.0f}, 0
    };

    RWFUNCTION(RWSTRING("ApplyEnvMapTextureMatrix"));

    if ( matrix != NULL &&
         FALSE == rwMatrixTestFlags(matrix, rwMATRIXINTERNALIDENTITY) )
    {
        RwD3D8SetTextureStageState(stage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);

        texMat.right.x = matrix->right.x;
        texMat.right.y = matrix->right.y;

        texMat.up.x = matrix->up.x;
        texMat.up.y = matrix->up.y;

        texMat.at.x = matrix->pos.x;
        texMat.at.y = matrix->pos.y;

        RwD3D8SetTransform(D3DTS_TEXTURE0 + stage, &texMat);
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpMatFXD3D8AtomicMatFXDefaultRender

 Purpose:

 On entry:

 On exit:
 */
void
_rpMatFXD3D8AtomicMatFXDefaultRender(RxD3D8InstanceData *instancedData,
                                    RwUInt32 flags,
                                    RwTexture *baseTexture)
{
    const MatFXUVAnimData *uvAnim = NULL;

    RWFUNCTION(RWSTRING("_rpMatFXD3D8AtomicMatFXDefaultRender"));

    if (flags & (rxGEOMETRY_TEXTURED|rpGEOMETRYTEXTURED2))
    {
        const rpMatFXMaterialData   *matFXData;

        RwD3D8SetTexture(baseTexture, 0);

        matFXData = *MATFXMATERIALGETCONSTDATA(instancedData->material);

        if (matFXData != NULL &&
            matFXData->flags == rpMATFXEFFECTUVTRANSFORM)
        {
            uvAnim = MATFXD3D8UVANIMGETDATA(instancedData->material);

            ApplyAnimTextureMatrix(0, uvAnim->baseTransform);
        }
    }
    else
    {
        RwD3D8SetTexture(NULL, 0);
    }

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

    if (instancedData->vertexAlpha)
    {
        RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
    }
    else
    {
        RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
    }

    /*
     * Set the default Pixel shader
     */
    RwD3D8SetPixelShader(0);

    /*
     * Vertex shader
     */
    RwD3D8SetVertexShader(instancedData->vertexShader);

    /*
     * Set the stream source
     */
    RwD3D8SetStreamSource(0, instancedData->vertexBuffer,
                              instancedData->stride);

    /*
     * Draw the geometry
     */
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

    if (uvAnim != NULL)
    {
        RwD3D8SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpMatFXD3D8AtomicMatFXRenderBlack

 Purpose:

 On entry:

 On exit:
 */
void
_rpMatFXD3D8AtomicMatFXRenderBlack(RxD3D8InstanceData *instancedData)
{
    RWFUNCTION(RWSTRING("_rpMatFXD3D8AtomicMatFXRenderBlack"));

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

    if (instancedData->vertexAlpha)
    {
        RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
    }
    else
    {
        RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
    }

    /*
     * Set the default Pixel shader
     */
    RwD3D8SetPixelShader(0);

    /*
     * Vertex shader
     */
    RwD3D8SetVertexShader(instancedData->vertexShader);

    /*
     * Set the stream source
     */
    RwD3D8SetStreamSource(0, instancedData->vertexBuffer,
                              instancedData->stride);

    /*
     * Draw the geometry
     */
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

    RWRETURNVOID();
}

/****************************************************************************
 _rpMatFXD3D8AtomicMatFXDualPassRender

 Purpose:

 On entry:

 On exit:
 */
void
_rpMatFXD3D8AtomicMatFXDualPassRender(RxD3D8InstanceData *instancedData,
                                      RwUInt32 flags,
                                      RwTexture *baseTexture,
                                      RwTexture *dualPassTexture)
{
    RWFUNCTION(RWSTRING("_rpMatFXD3D8AtomicMatFXDualPassRender"));

    if ( dualPassTexture )
    {
        const MatFXDualData   *dualData;
        const MatFXUVAnimData *uvAnim = NULL;

        RwBool  needSecondPass = TRUE;

        if ((*MATFXMATERIALGETCONSTDATA(instancedData->material))->flags !=
            rpMATFXEFFECTDUALUVTRANSFORM)
        {
            dualData = MATFXD3D8DUALGETDATA(instancedData->material);
        }
        else
        {
            dualData = MATFXD3D8DUALUVANIMGETDUALDATA(instancedData->material);

            uvAnim = MATFXD3D8UVANIMGETDATA(instancedData->material);
        }

        if (flags & (rxGEOMETRY_TEXTURED|rpGEOMETRYTEXTURED2))
        {
            RwD3D8SetTexture(baseTexture, 0);
        }
        else
        {
            RwD3D8SetTexture(NULL, 0);
        }

        if (uvAnim != NULL)
        {
            ApplyAnimTextureMatrix(0, uvAnim->baseTransform);
        }

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

        if (instancedData->vertexAlpha)
        {
            RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
        }
        else
        {
            RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
        }

        /* Check for some blend modes optimizations */
        if (baseTexture &&
            (RwRasterGetFormat(RwTextureGetRaster(baseTexture)) & rwRASTERFORMATPAL8) != 0 &&
            (RwRasterGetFormat(RwTextureGetRaster(dualPassTexture)) & rwRASTERFORMATPAL8) != 0 &&
            baseTexture != dualPassTexture)
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
                            dualPassTexture->name);

                RwDebugSendMessage(rwDEBUGMESSAGE, "MatFX plugin (Dual)", buff);

                done = TRUE;
            }
            #endif
        }
        else if (VideoCardSupportsMultitexture)
        {
            if ( (dualData->srcBlendMode == rwBLENDDESTCOLOR &&
                  dualData->dstBlendMode == rwBLENDZERO) ||
                 (dualData->srcBlendMode == rwBLENDZERO &&
                  dualData->dstBlendMode == rwBLENDSRCCOLOR))
            {
                if (baseTexture != NULL)
                {
                    RwD3D8SetTexture(dualPassTexture, 1);

                    /* Some old cards with 3 stages need the diffuse in the last one */
                    if (VideoCardMaxTextureBlendStages == 3)
                    {
                        RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
                        RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

                        RwD3D8SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                        RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                        RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                        RwD3D8SetTextureStageState(2, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                        RwD3D8SetTextureStageState(2, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
                        RwD3D8SetTextureStageState(2, D3DTSS_COLORARG2, D3DTA_CURRENT);

                        if(_rwD3D8TextureHasAlpha(dualPassTexture))
                        {
                            if (_rwD3D8TextureHasAlpha(baseTexture))
                            {
                                RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                                RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

                                RwD3D8SetTextureStageState(2, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                                RwD3D8SetTextureStageState(2, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
                            }
                        }
                    }
                    else
                    {
                        /* This could fail in very old cards */
                        if(_rwD3D8TextureHasAlpha(dualPassTexture))
                        {
                            if (_rwD3D8TextureHasAlpha(baseTexture))
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
                    RwD3D8SetTexture(dualPassTexture, 0);

                    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);
                }

                needSecondPass = FALSE;
            }
            else if ( dualData->srcBlendMode == rwBLENDSRCALPHA &&
                      dualData->dstBlendMode == rwBLENDINVSRCALPHA )
            {
                if (baseTexture != NULL)
                {
                    if (VideoCardMaxTextureBlendStages >= 3)
                    {
                        RwD3D8SetTexture(dualPassTexture, 1);

                        RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
                        RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

                        RwD3D8SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_BLENDTEXTUREALPHA);
                        RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                        RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                        RwD3D8SetTextureStageState(2, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                        RwD3D8SetTextureStageState(2, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
                        RwD3D8SetTextureStageState(2, D3DTSS_COLORARG2, D3DTA_CURRENT);

                        if(_rwD3D8TextureHasAlpha(baseTexture))
                        {
                            RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
                            RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

                            RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_BLENDTEXTUREALPHA);
                            RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
                            RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

                            RwD3D8SetTextureStageState(2, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
                            RwD3D8SetTextureStageState(2, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
                            RwD3D8SetTextureStageState(2, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
                        }

                        needSecondPass = FALSE;
                    }
                }
                else
                {
                    RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, 0xffffffff);

                    RwD3D8SetTexture(dualPassTexture, 0);

                    RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_BLENDTEXTUREALPHA);
                    RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                    RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
                    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);

                    RwD3D8SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                    RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
                    RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                    needSecondPass = FALSE;
                }
            }
            else if ( dualData->srcBlendMode == rwBLENDDESTCOLOR &&
                      dualData->dstBlendMode == rwBLENDSRCCOLOR)
            {
                if (VideoCardSupportsModulate2X)
                {
                    if (baseTexture != NULL)
                    {
                        RwD3D8SetTexture(dualPassTexture, 1);

                        RwD3D8SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE2X);
                        RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                        RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                        /* Some old cards with 3 stages need the diffuse in the last one */
                        if (VideoCardMaxTextureBlendStages == 3)
                        {
                            RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
                            RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

                            RwD3D8SetTextureStageState(2, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                            RwD3D8SetTextureStageState(2, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
                            RwD3D8SetTextureStageState(2, D3DTSS_COLORARG2, D3DTA_CURRENT);

                            if(_rwD3D8TextureHasAlpha(dualPassTexture))
                            {
                                if (_rwD3D8TextureHasAlpha(baseTexture))
                                {
                                    RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                                    RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

                                    RwD3D8SetTextureStageState(2, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                                    RwD3D8SetTextureStageState(2, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
                                }
                            }
                        }
                        else
                        {
                            /* This could fail in very old cards */
                            if(_rwD3D8TextureHasAlpha(dualPassTexture))
                            {
                                if (_rwD3D8TextureHasAlpha(baseTexture))
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
                        RwD3D8SetTexture(dualPassTexture, 0);

                        RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE2X);

                        RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);
                    }

                    if (NeedToValidateModulate2X)
                    {
                        RwUInt32 numPass = 0;

                        _rwD3D8RenderStateFlushCache();

                        if (FAILED(IDirect3DDevice8_ValidateDevice(_RwD3DDevice, (DWORD *)&numPass)) ||
                            numPass != 1)
                        {
                            VideoCardSupportsModulate2X = FALSE;

                            RwD3D8SetTexture(baseTexture, 0);

                            RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                            RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                            RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

                            RwD3D8SetTexture(NULL, 1);

                            RwD3D8SetTextureStageState(2, D3DTSS_COLOROP,   D3DTOP_DISABLE);
                            RwD3D8SetTextureStageState(2, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);
                        }
                        else
                        {
                            needSecondPass = FALSE;
                        }

                        NeedToValidateModulate2X = FALSE;
                    }
                    else
                    {
                        needSecondPass = FALSE;
                    }
                }
            }
            else if ( dualData->srcBlendMode == rwBLENDONE &&
                      dualData->dstBlendMode == rwBLENDONE )
            {
                if (baseTexture != NULL)
                {
                    if (VideoCardSupportsMultiplyAdd)
                    {
                        RwD3D8SetTexture(dualPassTexture, 1);

                        RwD3D8SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MULTIPLYADD);
                        RwD3D8SetTextureStageState(1, D3DTSS_COLORARG0, D3DTA_CURRENT);
                        RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                        RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

                        if(_rwD3D8TextureHasAlpha(dualPassTexture))
                        {
                            if (_rwD3D8TextureHasAlpha(baseTexture))
                            {
                                RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                                RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
                            }
                            else
                            {
                                RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);
                            }
                        }

                        needSecondPass = FALSE;
                    }
                }
                else
                {
                    RwD3D8SetTexture(dualPassTexture, 0);

                    RwD3D8SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_ADD);
                    RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
                    RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);

                    needSecondPass = FALSE;
                }
            }
            else if ( dualData->srcBlendMode == rwBLENDZERO &&
                      dualData->dstBlendMode == rwBLENDSRCALPHA )
            {
                if (baseTexture != NULL)
                {
                    RwD3D8SetTexture(dualPassTexture, 1);

                    /* Some old cards with 3 stages need the diffuse in the last one */
                    if (VideoCardMaxTextureBlendStages == 3)
                    {
                        RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
                        RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

                        RwD3D8SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                        RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE | D3DTA_ALPHAREPLICATE);
                        RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                        RwD3D8SetTextureStageState(2, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                        RwD3D8SetTextureStageState(2, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
                        RwD3D8SetTextureStageState(2, D3DTSS_COLORARG2, D3DTA_CURRENT);

                        if (_rwD3D8TextureHasAlpha(baseTexture))
                        {
                            RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                            RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

                            RwD3D8SetTextureStageState(2, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                            RwD3D8SetTextureStageState(2, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
                        }
                    }
                    else
                    {
                        /* This could fail in very old cards*/
                        RwD3D8SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                        RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE | D3DTA_ALPHAREPLICATE);
                        RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                        if (_rwD3D8TextureHasAlpha(baseTexture))
                        {
                            RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                            RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
                        }
                    }
                }
                else
                {
                    RwD3D8SetTexture(dualPassTexture, 0);

                    RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                    RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE | D3DTA_ALPHAREPLICATE);
                    RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
                    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);
                }

                needSecondPass = FALSE;
            }

            if (!needSecondPass)
            {
                if ((flags & rpGEOMETRYTEXTURED2) == 0)
                {
                    RwD3D8SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 0);
                }

                if (uvAnim != NULL)
                {
                    ApplyAnimTextureMatrix(1, uvAnim->dualTransform);
                }
            }
        }

        /*
         * Set the default Pixel shader
         */
        RwD3D8SetPixelShader(0);

        /*
         * Vertex shader
         */
        RwD3D8SetVertexShader(instancedData->vertexShader);

        /*
         * Set the stream source
         */
        RwD3D8SetStreamSource(0, instancedData->vertexBuffer,
                                  instancedData->stride);

        /*
         * Draw the geometry
         */
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

        /*
         * Draw second pass
         */
        if (needSecondPass)
        {
            RwBlendFunction srcBlend, destBlend;
            RwBool      zWriteEnable;
            RwBool      fogEnabled;
            RwUInt32    fogColor;

            RwD3D8SetTexture(dualPassTexture, 0);

            if (uvAnim != NULL)
            {
                ApplyAnimTextureMatrix(0, uvAnim->dualTransform);
            }

            if (!_rwD3D8RenderStateIsVertexAlphaEnable())
            {
                _rwD3D8RenderStateVertexAlphaEnable(TRUE);
            }

            /* Remove alpha test for some combos */
            if ( (dualData->srcBlendMode == rwBLENDDESTCOLOR && dualData->dstBlendMode == rwBLENDZERO) ||
                 (dualData->srcBlendMode == rwBLENDZERO && dualData->dstBlendMode == rwBLENDSRCCOLOR) ||
                 (dualData->srcBlendMode == rwBLENDDESTCOLOR && dualData->dstBlendMode == rwBLENDSRCCOLOR) )
            {
                RwD3D8SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
            }

            /*
             * Set appropiate blending mode
             */
            RwRenderStateGet(rwRENDERSTATESRCBLEND, (void *)&srcBlend);
            RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void *)&destBlend);

            _rwD3D8RenderStateSrcBlend(dualData->srcBlendMode);
            _rwD3D8RenderStateDestBlend(dualData->dstBlendMode);

            RwD3D8GetRenderState(D3DRS_ZWRITEENABLE, (void *)&zWriteEnable);
            RwD3D8SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

            RwD3D8GetRenderState(D3DRS_FOGENABLE, (void *)&fogEnabled);
            if (fogEnabled)
            {
                RwD3D8GetRenderState(D3DRS_FOGCOLOR, (void *)&fogColor);

                if (dualData->dstBlendMode == rwBLENDONE)
                {
                    RwD3D8SetRenderState(D3DRS_FOGCOLOR, 0);
                }
                else if ( dualData->srcBlendMode == rwBLENDDESTCOLOR ||
                          dualData->dstBlendMode == rwBLENDSRCCOLOR )
                {
                    RwD3D8SetRenderState(D3DRS_FOGCOLOR, 0xffffffff);
                }
            }

            if (flags & rpGEOMETRYTEXTURED2)
            {
                RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);
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

            RwD3D8SetRenderState(D3DRS_ZWRITEENABLE, zWriteEnable);

            if (fogEnabled)
            {
                RwD3D8SetRenderState(D3DRS_FOGCOLOR, fogColor);
            }

            if (flags & rpGEOMETRYTEXTURED2)
            {
                RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
            }

            _rwD3D8RenderStateVertexAlphaEnable(FALSE);

            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)srcBlend);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)destBlend);
        }
        else
        {
            RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

            RwD3D8SetTexture(NULL, 1);

            RwD3D8SetTextureStageState(2, D3DTSS_COLOROP,   D3DTOP_DISABLE);
            RwD3D8SetTextureStageState(2, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);

            if ((flags & rpGEOMETRYTEXTURED2) == 0)
            {
                RwD3D8SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
            }
        }

        if (uvAnim != NULL)
        {
            RwD3D8SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
            RwD3D8SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
        }
    }
    else
    {
        _rpMatFXD3D8AtomicMatFXDefaultRender(instancedData, flags, baseTexture);
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpMatFXD3D8AtomicMatFXEnvRender

 Purpose:

 On entry:

 On exit:
 */
void
_rpMatFXD3D8AtomicMatFXEnvRender(RxD3D8InstanceData *instancedData,
                                 RwUInt32 flags,
                                 RwUInt32 pass,
                                 RwTexture *baseTexture,
                                 RwTexture *envMapTexture)
{
    const MatFXEnvMapData *envMapData;
    RwUInt32        shinny;

    RWFUNCTION(RWSTRING("_rpMatFXD3D8AtomicMatFXEnvRender"));

    envMapData = MATFXD3D8ENVMAPGETDATA(instancedData->material, pass);

    shinny = (RwFastRealToUInt32(envMapData->coef * 255) & 0xFF);

    if (shinny && envMapTexture)
    {
        RwBool  useEnvMapPixelShader = FALSE;
        RwBool  useMultitexture = FALSE;
        RwBool  hasBaseTexture = FALSE;

        /* Vertex alpha */
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

        if (instancedData->vertexAlpha)
        {
            RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
        }
        else
        {
            RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
        }

        if (pass == rpSECONDPASS)
        {
            /* Set the base texture */
            if (flags & (rxGEOMETRY_TEXTURED|rpGEOMETRYTEXTURED2) &&
                baseTexture)
            {
                RwD3D8SetTexture(baseTexture, 0);

                hasBaseTexture = TRUE;
            }
            else
            {
                RwD3D8SetTexture(NULL, 0);
            }

            /* Choose code path */
            if (!hasBaseTexture ||
                (RwRasterGetFormat(RwTextureGetRaster(baseTexture)) & rwRASTERFORMATPAL8) == 0 ||
                (RwRasterGetFormat(RwTextureGetRaster(envMapTexture)) & rwRASTERFORMATPAL8) == 0 ||
                baseTexture == envMapTexture)
            {
                if ( envMapData->useFrameBufferAlpha ||
                     !( instancedData->vertexAlpha ||
                        0xFF != instancedData->material->color.alpha ||
                        (hasBaseTexture && _rwD3D8TextureHasAlpha(baseTexture)) ) )
                {
                    if (_rwD3D8TextureHasAlpha(envMapTexture))
                    {
                        if (EnvMapPixelShader)
                        {
                            useEnvMapPixelShader = TRUE;
                        }
                    }
                    else if (VideoCardSupportsMultiplyAdd)
                    {
                        useMultitexture = TRUE;
                    }
                    else if (shinny == 0xFF)
                    {
                        /*
                         * Some video cards with 3 stages
                         * need the diffuse in the last one
                         */
                        if (!hasBaseTexture ||
                            VideoCardSupportsDiffuseInStage0 ||
                            NeedToValidateDiffuseInStage0)
                        {
                            useMultitexture = TRUE;
                        }
                    }
                }
            }
            else
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
                                envMapTexture->name);

                    RwDebugSendMessage(rwDEBUGMESSAGE, "MatFX plugin (EnvMap)", buff);

                    done = TRUE;
                }
                #endif
            }

            /*
             * Set the default Pixel shader
             */
            if (!useEnvMapPixelShader)
            {
                RwD3D8SetPixelShader(0);
            }

            /*
             * Vertex shader
             */
            RwD3D8SetVertexShader(instancedData->vertexShader);

            /*
             * Set the stream source
             */
            RwD3D8SetStreamSource(0, instancedData->vertexBuffer,
                                      instancedData->stride);

            /*
             * Draw the geometry if dual pass
             */
            if (instancedData->indexBuffer != NULL)
            {
                /* Set the index buffer */
                RwD3D8SetIndices(instancedData->indexBuffer, instancedData->baseIndex);

                /* Draw the indexed primitive */
                if (!useEnvMapPixelShader && !useMultitexture)
                {
                    RwD3D8DrawIndexedPrimitive((D3DPRIMITIVETYPE)instancedData->primType,
                                               0, instancedData->numVertices,
                                               0, instancedData->numIndices);
                }
            }
            else
            {
                if (!useEnvMapPixelShader && !useMultitexture)
                {
                    RwD3D8DrawPrimitive((D3DPRIMITIVETYPE)instancedData->primType,
                                        instancedData->baseIndex,
                                        instancedData->numVertices);
                }
            }
        }

        /*
         * Add envmap scaled by coef
         */
        if (useEnvMapPixelShader)
        {
            const RwReal coef = envMapData->coef;
#if (defined(__VECTORC__))
            RwReal ShinyFloats[4];
            ShinyFloats[0] = coef;
            ShinyFloats[1] = coef;
            ShinyFloats[2] = coef;
            ShinyFloats[3] = coef;
#else /* (defined(__VECTORC__)) */
            const RwReal ShinyFloats[4]={coef, coef, coef, coef};
#endif /* (defined(__VECTORC__)) */

            RwD3D8SetPixelShaderConstant(0, ShinyFloats, 1);

            if (hasBaseTexture)
            {
                ApplyEnvMapTextureMatrix(envMapTexture, 1, envMapData->frame);

                /* Set pixel shader */
                RwD3D8SetPixelShader(EnvMapPixelShader);

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

                RwD3D8SetTexture(NULL, 1);

                RwD3D8SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
                RwD3D8SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
            }
            else
            {
                ApplyEnvMapTextureMatrix(envMapTexture, 0, envMapData->frame);

                /* Set pixel shader */
                RwD3D8SetPixelShader(EnvMapNoBaseTexturePixelShader);

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

                RwD3D8SetTexture(NULL, 0);

                RwD3D8SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
                RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
            }
        }
        else if (useMultitexture)
        {
            if (shinny != 0xFF || hasBaseTexture)
            {
                ApplyEnvMapTextureMatrix(envMapTexture, 1, envMapData->frame);

                if (shinny != 0xFF)
                {
                    /* Set the shinny factor and the correct texture stages */
                    shinny = ((shinny << 24) |
                              (shinny << 16) |
                              (shinny << 8) |
                               shinny);

                    RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, shinny);

                    RwD3D8SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MULTIPLYADD);
                    RwD3D8SetTextureStageState(1, D3DTSS_COLORARG0, D3DTA_CURRENT);
                    RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                    RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_TFACTOR);
                }
                else
                {
                    RwD3D8SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_ADD);
                    RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                    RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                    if (NeedToValidateDiffuseInStage0)
                    {
                        RwUInt32 numPass = 0;

                        _rwD3D8RenderStateFlushCache();

                        if (FAILED(IDirect3DDevice8_ValidateDevice(_RwD3DDevice,
                                                                   (DWORD *)&numPass))
                            || numPass != 1)
                        {
                            VideoCardSupportsDiffuseInStage0 = FALSE;
                        }
                        else
                        {
                            VideoCardSupportsDiffuseInStage0 = TRUE;
                        }

                        NeedToValidateDiffuseInStage0 = FALSE;
                    }
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

                RwD3D8SetTexture(NULL, 1);

                RwD3D8SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
                RwD3D8SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
            }
            else
            {
                ApplyEnvMapTextureMatrix(envMapTexture, 0, envMapData->frame);

                RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_ADD);

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

                RwD3D8SetTexture(NULL, 0);

                RwD3D8SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
                RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
            }
        }
        else
        {
            RwBlendFunction srcBlend, destBlend;
            RwBool      lighting;
            RwBool      zWriteEnable;
            RwBool      fogEnabled;
            RwUInt32    fogColor;

            ApplyEnvMapTextureMatrix(envMapTexture, 0, envMapData->frame);

            if (!_rwD3D8RenderStateIsVertexAlphaEnable())
            {
                _rwD3D8RenderStateVertexAlphaEnable(TRUE);
            }

            /* Set needed blending modes for envmap */
            RwRenderStateGet(rwRENDERSTATESRCBLEND, (void *)&srcBlend);
            RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void *)&destBlend);

            _rwD3D8RenderStateSrcBlend(rwBLENDSRCALPHA);
            _rwD3D8RenderStateDestBlend(rwBLENDONE);

            /* Set the shinnyness */
            shinny = ((shinny << 24) |
                      (shinny << 16) |
                      (shinny << 8) |
                       shinny);

            if (shinny<0xFFFFFFFF)
            {
                RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, shinny);

                RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

                RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
                RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
                RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
            }
            else
            {
                RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
                RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

                RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
                RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            }

            RwD3D8GetRenderState(D3DRS_LIGHTING, (void *)&lighting);
            RwD3D8GetRenderState(D3DRS_ZWRITEENABLE, (void *)&zWriteEnable);
            RwD3D8GetRenderState(D3DRS_FOGENABLE, (void *)&fogEnabled);

            RwD3D8SetRenderState(D3DRS_LIGHTING, FALSE);
            RwD3D8SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

            if (fogEnabled)
            {
                RwD3D8GetRenderState(D3DRS_FOGCOLOR, (void *)&fogColor);
                RwD3D8SetRenderState(D3DRS_FOGCOLOR, 0);
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
            RwD3D8SetRenderState(D3DRS_LIGHTING, lighting);

            if (shinny<0xFFFFFFFF)
            {
                RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
                RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
            }
            else
            {
                RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
            }

            _rwD3D8RenderStateVertexAlphaEnable(FALSE);

            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)srcBlend);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)destBlend);

            RwD3D8SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
            RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_PASSTHRU);
        }
    }
    else
    {
        if (pass == rpSECONDPASS)
        {
            _rpMatFXD3D8AtomicMatFXDefaultRender(instancedData, flags, baseTexture);
        }
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpMatFXD3D8AtomicMatFXBumpMapRender

 Purpose:

 On entry:

 On exit:
 */
void
_rpMatFXD3D8AtomicMatFXBumpMapRender(RxD3D8InstanceData *instancedData,
                                     RwUInt32 flags,
                                     RwTexture *baseTexture,
                                     RwTexture *bumpTexture,
                                     RwTexture *envMapTexture)
{
    RWFUNCTION(RWSTRING("_rpMatFXD3D8AtomicMatFXBumpMapRender"));

    if (bumpTexture == NULL)
    {
        if (baseTexture != NULL &&
            _rwD3D8TextureHasAlpha(baseTexture))
        {
            bumpTexture = baseTexture;
        }
    }

    /*
     * Draw Bump Map
     */
    if (bumpTexture)
    {
        RwUInt32      vbBumpMapOffset;
        LPDIRECT3DVERTEXBUFFER8 vertexBufferBumpMap;
        RxD3D8BumpMapVertex *bufferMem;

        /* Fill Vertex Buffer */
        if (RwD3D8DynamicVertexBufferLock(sizeof(RxD3D8BumpMapVertex),
                                                instancedData->numVertices,
                                                (void**)&vertexBufferBumpMap,
                                                (void**)&bufferMem,
                                                &vbBumpMapOffset))
        {
            RwBool  zWriteEnable;

            CalculatePerturbedUVs(instancedData, bufferMem);

            RwD3D8DynamicVertexBufferUnlock(vertexBufferBumpMap);

            /*
             * Set base textures
             */
            RwD3D8SetTexture(bumpTexture, 0);

            /*
             * Vertex shader
             */
            RwD3D8SetVertexShader(BUMPMAP_FVF);

            /*
             * Set the stream source
             */
            RwD3D8SetStreamSource(0, vertexBufferBumpMap, sizeof(RxD3D8BumpMapVertex));

            /*
             * Set Indices
             */
            if (instancedData->indexBuffer != NULL)
            {
                RwD3D8SetIndices(instancedData->indexBuffer, vbBumpMapOffset);
            }

            /*
             * Draw effect
             */
            if ( (envMapTexture && BumpEnvMapPixelShader) || BumpMapPixelShader )
            {
                RwBool alphaBlend;
                RwBool alphaTest;

                _rwD3D8RenderStateVertexAlphaEnable(FALSE);

                RwD3D8SetTexture(bumpTexture, 1);

                /* We need to force it due to alpha texture */
                RwD3D8GetRenderState(D3DRS_ALPHABLENDENABLE, &alphaBlend);
                RwD3D8GetRenderState(D3DRS_ALPHATESTENABLE, &alphaTest);

                RwD3D8SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
                RwD3D8SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

                /*
                 * Pixel shader
                 */
                if (envMapTexture && BumpEnvMapPixelShader)
                {
                    const MatFXEnvMapData *envMapData = MATFXD3D8ENVMAPGETDATA(instancedData->material, rpTHIRDPASS);

                    const RwUInt32        shinny = (RwFastRealToUInt32(envMapData->coef * 255) & 0xFF);

                    if (shinny)
                    {
                        const RwReal coef = envMapData->coef;
#if (defined(__VECTORC__))
                        RwReal ShinyFloats[4];
                        ShinyFloats[0] = coef;
                        ShinyFloats[1] = coef;
                        ShinyFloats[2] = coef;
                        ShinyFloats[3] = coef;
#else /* (defined(__VECTORC__)) */
                        const RwReal ShinyFloats[4]={coef, coef, coef, coef};
#endif /* (defined(__VECTORC__)) */

                        ApplyEnvMapTextureMatrix(envMapTexture, 2, envMapData->frame);

                       /* Set pixel shader */
                        if (envMapData->useFrameBufferAlpha)
                        {
                            if (_rwD3D8TextureHasAlpha(envMapTexture))
                            {
                                RwD3D8SetPixelShader(BumpModulatedEnvMapPixelShader);
                            }
                            else
                            {
                                RwD3D8SetPixelShader(BumpModulatedEnvMapNoAlphaPixelShader);
                            }
                        }
                        else
                        {
                            RwD3D8SetPixelShader(BumpEnvMapPixelShader);
                        }

                        RwD3D8SetPixelShaderConstant(0, ShinyFloats, 1);

                        if (instancedData->indexBuffer != NULL)
                        {
                            RwD3D8DrawIndexedPrimitive((D3DPRIMITIVETYPE)instancedData->primType,
                                                       0, instancedData->numVertices,
                                                       0, instancedData->numIndices);
                        }
                        else
                        {
                            RwD3D8DrawPrimitive((D3DPRIMITIVETYPE)instancedData->primType,
                                                vbBumpMapOffset,
                                                instancedData->numVertices);
                        }

                        RwD3D8SetTexture(NULL, 2);

                        RwD3D8SetTextureStageState(2, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
                        RwD3D8SetTextureStageState(2, D3DTSS_TEXCOORDINDEX, 2);
                    }
                    else
                    {
                        RwD3D8SetPixelShader(BumpMapPixelShader);

                        if (instancedData->indexBuffer != NULL)
                        {
                            RwD3D8DrawIndexedPrimitive((D3DPRIMITIVETYPE)instancedData->primType,
                                                       0, instancedData->numVertices,
                                                       0, instancedData->numIndices);
                        }
                        else
                        {
                            RwD3D8DrawPrimitive((D3DPRIMITIVETYPE)instancedData->primType,
                                                vbBumpMapOffset,
                                                instancedData->numVertices);
                        }
                    }
                }
                else
                {
                    RwD3D8SetPixelShader(BumpMapPixelShader);

                    if (instancedData->indexBuffer != NULL)
                    {
                        RwD3D8DrawIndexedPrimitive((D3DPRIMITIVETYPE)instancedData->primType,
                                                   0, instancedData->numVertices,
                                                   0, instancedData->numIndices);
                    }
                    else
                    {
                        RwD3D8DrawPrimitive((D3DPRIMITIVETYPE)instancedData->primType,
                                            vbBumpMapOffset,
                                            instancedData->numVertices);
                    }
                }

                /*
                 * Restore default values
                 */
                RwD3D8SetRenderState(D3DRS_ALPHABLENDENABLE, alphaBlend);
                RwD3D8SetRenderState(D3DRS_ALPHATESTENABLE, alphaTest);

                RwD3D8SetTexture(NULL, 1);
            }
            else
            {
                RwBlendFunction srcBlend, destBlend;

                /*
                 * Set the default Pixel shader
                 */
                RwD3D8SetPixelShader(0);

                /*
                 * First pass
                 */
                if (!_rwD3D8RenderStateIsVertexAlphaEnable())
                {
                    _rwD3D8RenderStateVertexAlphaEnable(TRUE);
                }

                RwD3D8SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

                RwRenderStateGet(rwRENDERSTATESRCBLEND, (void *)&srcBlend);
                RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void *)&destBlend);

                _rwD3D8RenderStateSrcBlend(rwBLENDINVSRCALPHA);
                _rwD3D8RenderStateDestBlend(rwBLENDZERO);

                if (instancedData->indexBuffer != NULL)
                {
                    RwD3D8DrawIndexedPrimitive((D3DPRIMITIVETYPE)instancedData->primType,
                                               0, instancedData->numVertices,
                                               0, instancedData->numIndices);
                }
                else
                {
                    RwD3D8DrawPrimitive((D3DPRIMITIVETYPE)instancedData->primType,
                                        vbBumpMapOffset,
                                        instancedData->numVertices);
                }

                RwD3D8SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);

                /*
                 * Second pass
                 */
                _rwD3D8RenderStateSrcBlend(rwBLENDSRCALPHA);
                _rwD3D8RenderStateDestBlend(rwBLENDONE);

                RwD3D8GetRenderState(D3DRS_ZWRITEENABLE, (void *)&zWriteEnable);
                RwD3D8SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

                RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);

                if (instancedData->indexBuffer != NULL)
                {
                    RwD3D8DrawIndexedPrimitive((D3DPRIMITIVETYPE)instancedData->primType,
                                               0, instancedData->numVertices,
                                               0, instancedData->numIndices);
                }
                else
                {
                    RwD3D8DrawPrimitive((D3DPRIMITIVETYPE)instancedData->primType,
                                        vbBumpMapOffset,
                                        instancedData->numVertices);
                }

                RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);

                RwD3D8SetRenderState(D3DRS_ZWRITEENABLE, zWriteEnable);

                /* Set standar blending mode */
                _rwD3D8RenderStateVertexAlphaEnable(FALSE);

                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)srcBlend);
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)destBlend);
            }

            if (envMapTexture && !BumpEnvMapPixelShader)
            {
                _rpMatFXD3D8AtomicMatFXEnvRender(instancedData,
                                                 flags,
                                                 rpTHIRDPASS,
                                                 baseTexture,
                                                 envMapTexture);
            }
        }
    }
    else
    {
        _rpMatFXD3D8AtomicMatFXDefaultRender(instancedData, flags, baseTexture);
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rwD3D8AtomicMatFXRenderCallback

 Purpose:

 On entry:

 On exit:
 */
void
_rwD3D8AtomicMatFXRenderCallback(RwResEntry *repEntry,
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
    const MatFXDualData         *dualData;

    RWFUNCTION(RWSTRING("_rwD3D8AtomicMatFXRenderCallback"));

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
    if (type == rpATOMIC)
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
    else
    {
        RpWorldSector   *worldSector;
        RwCamera        *cam;

        worldSector = (RpWorldSector *)object;

        cam = RwCameraGetCurrentCamera();
        RWASSERT(cam);

        if (RwD3D8CameraIsBBoxFullyInsideFrustum(cam, RpWorldSectorGetTightBBox(worldSector)))
        {
            RwD3D8SetRenderState(D3DRS_CLIPPING, FALSE);
        }
        else
        {
            RwD3D8SetRenderState(D3DRS_CLIPPING, TRUE);
        }
    }

    /* Get lighting state */
    RwD3D8GetRenderState(D3DRS_LIGHTING, &lighting);

    if (lighting || (flags & rxGEOMETRY_PRELIT) != 0)
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

    /* Get the instanced data */
    resEntryHeader = (RxD3D8ResEntryHeader *)(repEntry + 1);
    instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

    /* Get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;
    while (numMeshes--)
    {
        if (!forceBlack)
        {
            if (lighting)
            {
                RwD3D8SetSurfaceProperties(&instancedData->material->color,
                                        &instancedData->material->surfaceProps,
                                        (flags & rxGEOMETRY_MODULATE));
            }

            /*
             * Render
             */
            matFXData = *MATFXMATERIALGETCONSTDATA(instancedData->material);
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
                    bumpmap = MATFXD3D8BUMPMAPGETDATA(instancedData->material);

                    _rpMatFXD3D8AtomicMatFXBumpMapRender(instancedData,
                                                         flags,
                                                         instancedData->material->texture,
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
                                                     instancedData->material->texture,
                                                     envMapData->texture);
                }
                break;

            case rpMATFXEFFECTBUMPENVMAP:
                {
                    bumpmap = MATFXD3D8BUMPMAPGETDATA(instancedData->material);
                    envMapData = MATFXD3D8ENVMAPGETDATA(instancedData->material, rpTHIRDPASS);

                    _rpMatFXD3D8AtomicMatFXBumpMapRender(instancedData,
                                                         flags,
                                                         instancedData->material->texture,
                                                         bumpmap->texture,
                                                         envMapData->texture);
                }
                break;

            case rpMATFXEFFECTDUAL:
                {
                    dualData = MATFXD3D8DUALGETDATA(instancedData->material);

                    _rpMatFXD3D8AtomicMatFXDualPassRender(instancedData,
                                                          flags,
                                                          instancedData->material->texture,
                                                          dualData->texture);
                }
                break;

            case rpMATFXEFFECTDUALUVTRANSFORM:
                {
                    dualData = MATFXD3D8DUALUVANIMGETDUALDATA(instancedData->material);

                    _rpMatFXD3D8AtomicMatFXDualPassRender(instancedData,
                                                          flags,
                                                          instancedData->material->texture,
                                                          dualData->texture);
                }
                break;

            case rpMATFXEFFECTUVTRANSFORM:
            default:
                _rpMatFXD3D8AtomicMatFXDefaultRender(instancedData,
                                                     flags,
                                                     instancedData->material->texture);
                break;
            }
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
 _rwD3D8MaterialMatFXHasBumpMap
 Purpose:
 On entry:
 On exit:   TRUE if the material has bumpmap info
 */
RwBool
_rwD3D8MaterialMatFXHasBumpMap(const RpMaterial *material)
{
    const rpMatFXMaterialData   *matFXData;

    RWFUNCTION(RWSTRING("_rwD3D8MaterialMatFXHasBumpMap"));

    matFXData = *MATFXMATERIALGETCONSTDATA(material);
    if (NULL != matFXData)
    {
        const RpMatFXMaterialFlags effectType = matFXData->flags;

        RWRETURN(effectType == rpMATFXEFFECTBUMPMAP || effectType == rpMATFXEFFECTBUMPENVMAP);
    }

    RWRETURN(FALSE);
}

/*--- Create and destroy pipelines ------------------------------------------*/

/****************************************************************************
 AtomicMatFxPipelineCreate

 Purpose:

 On entry:

 On exit:
 */
static RxPipeline *
AtomicMatFxPipelineCreate(void)
{
    RxPipeline  *pipe;

    RWFUNCTION(RWSTRING("AtomicMatFxPipelineCreate"));

    pipe = RxPipelineCreate();
    if (pipe)
    {
        RxLockedPipe    *lpipe;

        pipe->pluginId = rwID_MATERIALEFFECTSPLUGIN;
        lpipe = RxPipelineLock(pipe);
        if (NULL != lpipe)
        {
            RxNodeDefinition    *instanceNode;

            /*
             * Get the instance node definition
             */
            instanceNode = RxNodeDefinitionGetD3D8AtomicAllInOne();

            /*
             * Add the node to the pipeline
             */
            lpipe = RxLockedPipeAddFragment(lpipe, NULL, instanceNode, NULL);

            /*
             * Unlock the pipeline
             */
            lpipe = RxLockedPipeUnlock(lpipe);

            RWRETURN(pipe);
        }

        RxPipelineDestroy(pipe);
    }

    RWRETURN(NULL);
}

/****************************************************************************
 rpWorldSectorMatFxPipelineCreate

 Purpose:

 On entry:

 On exit:
 */
static RxPipeline *
WorldSectorMatFxPipelineCreate(void)
{
    RxPipeline  *pipe;

    RWFUNCTION(RWSTRING("WorldSectorMatFxPipelineCreate"));

    pipe = RxPipelineCreate();
    if (pipe)
    {
        RxLockedPipe    *lpipe;

        pipe->pluginId = rwID_MATERIALEFFECTSPLUGIN;

        lpipe = RxPipelineLock(pipe);
        if (NULL != lpipe)
        {
            RxNodeDefinition    *instanceNode;

            /*
             * Get the instance node definition
             */
            instanceNode = RxNodeDefinitionGetD3D8WorldSectorAllInOne();

            /*
             * Add the node to the pipeline
             */
            lpipe = RxLockedPipeAddFragment(lpipe, NULL, instanceNode, NULL);

            /*
             * Unlock the pipeline
             */
            lpipe = RxLockedPipeUnlock(lpipe);

            RWRETURN(pipe);
        }

        RxPipelineDestroy(pipe);
    }

    RWRETURN(NULL);
}

RwBool
_rpMatFXPipelinesCreate(void)
{
    RxNodeDefinition    *instanceNode;
    RxPipelineNode      *node;

    RWFUNCTION(RWSTRING("_rpMatFXPipelinesCreate"));

    MatFXAtomicPipe = AtomicMatFxPipelineCreate();
    RWASSERT(NULL != MatFXAtomicPipe);

    /*
     * Get the instance node definition
     */
    instanceNode = RxNodeDefinitionGetD3D8AtomicAllInOne();
    RWASSERT(NULL != instanceNode);

    /*
     * Set the pipeline specific data
     */
    node = RxPipelineFindNodeByName(MatFXAtomicPipe, instanceNode->name, NULL, NULL);
    RWASSERT(NULL != node);

    /*
     * Set the MatFX render callback
     */
    RxD3D8AllInOneSetRenderCallBack(node, _rwD3D8AtomicMatFXRenderCallback);

    /*
     * And likewise for world sectors:
     */
    MatFXWorldSectorPipe = WorldSectorMatFxPipelineCreate();
    RWASSERT(NULL != MatFXWorldSectorPipe);

    instanceNode = RxNodeDefinitionGetD3D8WorldSectorAllInOne();
    RWASSERT(NULL != instanceNode);

    node = RxPipelineFindNodeByName(MatFXWorldSectorPipe, instanceNode->name, NULL, NULL);
    RWASSERT(NULL != node);

    RxD3D8AllInOneSetRenderCallBack(node, _rwD3D8AtomicMatFXRenderCallback);

    /*
     * Get some video card capabilities
     */
    VideoCardMaxTextureBlendStages = ((const D3DCAPS8 *)RwD3D8GetCaps())->MaxTextureBlendStages;

    if (VideoCardMaxTextureBlendStages >= 2)
    {
        VideoCardSupportsMultitexture = (((const D3DCAPS8 *)RwD3D8GetCaps())->MaxSimultaneousTextures >= 2);
        VideoCardSupportsMultiplyAdd = (((const D3DCAPS8 *)RwD3D8GetCaps())->TextureOpCaps & D3DTEXOPCAPS_MULTIPLYADD);
        VideoCardSupportsModulate2X = (((const D3DCAPS8 *)RwD3D8GetCaps())->TextureOpCaps & D3DTEXOPCAPS_MODULATE2X);
    }
    else
    {
        VideoCardSupportsMultitexture = FALSE;
        VideoCardSupportsMultiplyAdd = FALSE;
        VideoCardSupportsModulate2X = FALSE;
        VideoCardSupportsDiffuseInStage0 = FALSE;

        NeedToValidateModulate2X = FALSE;
        NeedToValidateDiffuseInStage0 = FALSE;
    }

    /*
     * Try to create the pixel shaders
     */
    if ( (((const D3DCAPS8 *)RwD3D8GetCaps())->PixelShaderVersion & 0xffff) >= 0x0100)
    {
        RwD3D8CreatePixelShader((const RwUInt32 *)dwBumpMapPShaderPixelShader,
                                &BumpMapPixelShader);

        RwD3D8CreatePixelShader((const RwUInt32 *)dwEnvMapPShaderPixelShader,
                                &EnvMapPixelShader);

        RwD3D8CreatePixelShader((const RwUInt32 *)dwEnvMapNoBaseTexturePShaderPixelShader,
                                &EnvMapNoBaseTexturePixelShader);

        RwD3D8CreatePixelShader((const RwUInt32 *)dwBumpEnvMapPShaderPixelShader,
                                &BumpEnvMapPixelShader);

        RwD3D8CreatePixelShader((const RwUInt32 *)dwBumpModulatedEnvMapPShaderPixelShader,
                                &BumpModulatedEnvMapPixelShader);

        RwD3D8CreatePixelShader((const RwUInt32 *)dwBumpModulatedEnvMapNoAlphaPShaderPixelShader,
                                &BumpModulatedEnvMapNoAlphaPixelShader);
    }
    else
    {
        BumpMapPixelShader = 0;

        EnvMapPixelShader = 0;
        EnvMapNoBaseTexturePixelShader = 0;

        BumpEnvMapPixelShader = 0;

        BumpModulatedEnvMapPixelShader = 0;
        BumpModulatedEnvMapNoAlphaPixelShader = 0;
    }

#if defined(RWDEBUG)
    if (BumpMapPixelShader &&
        EnvMapPixelShader &&
        EnvMapNoBaseTexturePixelShader &&
        BumpEnvMapPixelShader &&
        BumpModulatedEnvMapPixelShader &&
        BumpModulatedEnvMapNoAlphaPixelShader)
    {
        RwDebugSendMessage(rwDEBUGMESSAGE, "MatFX plugin", "Device supports pixel shaders and using them.");
    }
    else
    {
        RwDebugSendMessage(rwDEBUGMESSAGE, "MatFX plugin", "Device doesn't support pixel shaders, using fixed function pipeline.");
    }
#endif

    RWRETURN(TRUE);
}

RwBool
_rpMatFXPipelinesDestroy(void)
{
    RWFUNCTION(RWSTRING("_rpMatFXPipelinesDestroy"));

    if (MatFXAtomicPipe)
    {
        RxPipelineDestroy(MatFXAtomicPipe);
        MatFXAtomicPipe = NULL;
    }

    if (MatFXWorldSectorPipe)
    {
        RxPipelineDestroy(MatFXWorldSectorPipe);
        MatFXWorldSectorPipe = NULL;
    }

    if (BumpMapPixelShader)
    {
        RwD3D8DeletePixelShader(BumpMapPixelShader);
        BumpMapPixelShader = 0;
    }

    if (EnvMapPixelShader)
    {
        RwD3D8DeletePixelShader(EnvMapPixelShader);
        EnvMapPixelShader = 0;
    }

    if (EnvMapNoBaseTexturePixelShader)
    {
        RwD3D8DeletePixelShader(EnvMapNoBaseTexturePixelShader);
        EnvMapNoBaseTexturePixelShader = 0;
    }

    if (BumpEnvMapPixelShader)
    {
        RwD3D8DeletePixelShader(BumpEnvMapPixelShader);
        BumpEnvMapPixelShader = 0;
    }

    if (BumpModulatedEnvMapPixelShader)
    {
        RwD3D8DeletePixelShader(BumpModulatedEnvMapPixelShader);
        BumpModulatedEnvMapPixelShader = 0;
    }

    if (BumpModulatedEnvMapNoAlphaPixelShader)
    {
        RwD3D8DeletePixelShader(BumpModulatedEnvMapNoAlphaPixelShader);
        BumpModulatedEnvMapNoAlphaPixelShader = 0;
    }

    RWRETURN(TRUE);
}

/*--- Attach pipelines ------------------------------------------------------*/
RpAtomic *
_rpMatFXPipelineAtomicSetup(RpAtomic *atomic)
{
    RWFUNCTION(RWSTRING("_rpMatFXPipelineAtomicSetup"));
    RWASSERT(atomic);

    RpAtomicSetPipeline(atomic, MatFXAtomicPipe);

    RWRETURN(atomic);
}

RpWorldSector *
_rpMatFXPipelineWorldSectorSetup(RpWorldSector *worldSector)
{
    RWFUNCTION(RWSTRING("_rpMatFXPipelineWorldSectorSetup"));
    RWASSERT(worldSector);

    RpWorldSectorSetPipeline(worldSector, MatFXWorldSectorPipe);

    RWRETURN(worldSector);
}

/*--- Upload texture --------------------------------------------------------*/

/*--- Device data fucntions -------------------------------------------------*/
RwBool
_rpMatFXSetupDualRenderState(MatFXDualData *dualData __RWUNUSEDRELEASE__,
                             RwRenderState nState __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("_rpMatFXSetupDualRenderState"));
    RWASSERT(dualData);
    RWRETURN(TRUE);
}

RwTexture *
_rpMatFXSetupBumpMapTexture(const RwTexture *baseTexture,
                            const RwTexture *effectTexture)
{
    RwTexture *texture = NULL;

    RWFUNCTION(RWSTRING("_rpMatFXSetupBumpMapTexture"));

    if (effectTexture != NULL)
    {
        RwBool usingCompressed = FALSE;

        /* We can NOT manipulate compressed textures */
        if (baseTexture != NULL)
        {
            RWASSERT(RwTextureGetRaster(baseTexture) != NULL);
            if (RwD3D8RasterIsCompressed(RwTextureGetRaster(baseTexture)))
            {
                usingCompressed = TRUE;
            }
            else
            {
                RWASSERT(RwTextureGetRaster(effectTexture) != NULL);
                if (RwD3D8RasterIsCompressed(RwTextureGetRaster(effectTexture)))
                {
                    usingCompressed = TRUE;
                }
            }
        }
        else
        {
            RWASSERT(RwTextureGetRaster(effectTexture) != NULL);
            if (RwD3D8RasterIsCompressed(RwTextureGetRaster(effectTexture)))
            {
                usingCompressed = TRUE;
            }
        }

        if (usingCompressed)
        {
            #if defined(RWDEBUG)
            RwChar buff[256];

            if (baseTexture != NULL)
            {
                rwsprintf(buff,
                            "\n"
                            "\tThis effect does not support compressed textures.\n"
                            "\tCheck textures '%s' and '%s'.",
                            baseTexture->name,
                            effectTexture->name);
            }
            else
            {
                rwsprintf(buff,
                            "\n"
                            "\tThis effect does not support compressed textures.\n"
                            "\tCheck texture '%s'.",
                            effectTexture->name);
            }

            RwDebugSendMessage(rwDEBUGERROR, "MatFX plugin (BumpMap)", buff);
            #endif
        }
        else
        {
            texture = _rpMatFXTextureMaskCreate(baseTexture, effectTexture);
        }
    }
    else
    {
        texture = (RwTexture *)baseTexture;
    }

    RWRETURN(texture);
}

/**
 * \ingroup rpmatfxd3d8
 * \ref RpMatFXGetD3D8Pipeline
 *
 * Returns one of the \ref rpmatfx internal D3D8 specific pipelines.
 *
 * The \ref rpmatfx plugin must be attached before using this function.
 *
 * \param d3d8Pipeline Type of the requested pipeline.
 *
 * \return The \ref RxPipeline requested from the plugin, or
 * NULL if the pipeline wasn't constructed.
 */
RxPipeline *
RpMatFXGetD3D8Pipeline( RpMatFXD3D8Pipeline d3d8Pipeline )
{
    RxPipeline *pipeline;

    RWAPIFUNCTION(RWSTRING("RpMatFXGetD3D8Pipeline"));
    RWASSERT(0 < MatFXInfo.Module.numInstances);
    RWASSERT(rpNAMATFXD3D8PIPELINE < d3d8Pipeline);
    RWASSERT(rpMATFXD3D8PIPELINEMAX > d3d8Pipeline);

    switch(d3d8Pipeline)
    {
        case rpMATFXD3D8ATOMICPIPELINE:
            pipeline = MatFXAtomicPipe;
            break;
        case rpMATFXD3D8WORLDSECTORPIPELINE:
            pipeline = MatFXWorldSectorPipe;
            break;
        default:
            pipeline = (RxPipeline *)NULL;
            break;
    }

    RWRETURN(pipeline);
}
