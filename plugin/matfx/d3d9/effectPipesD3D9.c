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

/*==== D3D9 includes ====*/
#include <d3d9.h>

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

//@{ Jaewon 20041125
extern D3DTEXTUREOP TextureOpModulation;
//@} Jaewon

/**
 * \ingroup rpmatfxd3d9
 * \page rpmatfxd3d9overview D3D9 RpMatFX Overview
 *
 * The \ref rpmatfx plugin functions correctly under RWD3D9. In addition, RWD3D9 now takes
 * care of the type and position of lights when rendering bump maps.
 *
 * \ref rpmatfx will also detect if the video hardware supports pixel and vertex shaders.
 * If found, pixel shaders are used to increase efficiency of the rendering.
 *
 * When using the environment map effect with a non-opaque base texture, RpMatFX on D3D9 is forced
 * to use dual pass to achieve the desired result. To enable multitexturing but getting different
 * results, you need to enable the use of the frame buffer alpha through the functions
 * \ref RpMatFXMaterialSetEnvMapFrameBufferAlpha and \ref RpMatFXMaterialSetupEnvMap.
 *
 * RWD3D9 always tries to use multitexturing when possible but, due to a limitation
 * of the D3D9 API, only one palette is active when rendering. That
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

/* I'm including that here to not add the "d3d9.h" header to the effectPipesD3D9.h */
extern LPDIRECT3DDEVICE9    _RwD3DDevice;

/*----------------------------------------------------------------------*
 *-   Local Types                                                      -*
 *----------------------------------------------------------------------*/

typedef struct _rxD3D9BumpMapVertex RxD3D9BumpMapVertex;
struct _rxD3D9BumpMapVertex
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

static void *BumpMapPixelShader = NULL;
static void *EnvMapPixelShader = NULL;
static void *EnvMapNoBaseTexturePixelShader = NULL;
static void *BumpEnvMapPixelShader = NULL;
static void *BumpModulatedEnvMapPixelShader = NULL;
static void *BumpModulatedEnvMapNoAlphaPixelShader = NULL;

static RwBool       VideoCardSupportsMultitexture = FALSE;
static RwBool       VideoCardSupportsMultiplyAdd = FALSE;
static RwBool       VideoCardSupportsModulate2X = FALSE;
static RwBool       VideoCardSupportsDiffuseInStage0 = FALSE;
static RwUInt32     VideoCardMaxTextureBlendStages = 0;
static RwBool       VideoCardSupportsTangents = FALSE;

static RwBool       NeedToValidateModulate2X = TRUE;
static RwBool       NeedToValidateDiffuseInStage0 = TRUE;

static RxD3D9AllInOneInstanceCallBack D3D9DefaultInstanceCallbackAtomic = NULL;
static RxD3D9AllInOneInstanceCallBack D3D9DefaultInstanceCallbackWorldSector = NULL;

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

	//@{ 20050513 DDonSS : Threadsafe
	// Frame Lock
	CS_FRAME_LOCK();
	//@} DDonSS

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

        ++objcount;
        current = next;
    }

	//@{ 20050513 DDonSS : Threadsafe
	// Frame Unlock
	CS_FRAME_UNLOCK();
	//@} DDonSS

    RWRETURN((light));
}

/****************************************************************************
 D3D9AtomicMatFXInstanceCallback

 */
static RwBool
D3D9AtomicMatFXInstanceCallback(void *object,
                                RxD3D9ResEntryHeader *resEntryHeader,
                                RwBool reinstance)
{
    RWFUNCTION( RWSTRING( "D3D9AtomicMatFXInstanceCallback" ) );

    if (!reinstance)
    {
        const rpMatFXMaterialData *matFXData;
        RxD3D9InstanceData  *instancedData;
        RwUInt32            numMeshes;

        numMeshes = resEntryHeader->numMeshes;
        instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

        do
        {
            /* Now I can create my extra data depending on the effect */
            matFXData = *MATFXMATERIALGETCONSTDATA(instancedData->material);
            if( matFXData )
            {
                switch( matFXData->flags )
                {
                    case rpMATFXEFFECTENVMAP:
                        break;

                    case rpMATFXEFFECTBUMPMAP:
                    case rpMATFXEFFECTBUMPENVMAP:
                        /* Only use tangents if D3D8 level harware */
                        if (VideoCardSupportsTangents)
                        {
                            RpGeometry *geometry;
                            RpD3D9GeometryUsageFlag usageFlags;

                            geometry = (RpGeometry *)RpAtomicGetGeometry((RpAtomic *)object);

                            usageFlags = RpD3D9GeometryGetUsageFlags(geometry);

                            usageFlags |= rpD3D9GEOMETRYUSAGE_CREATETANGENTS;

                            RpD3D9GeometrySetUsageFlags(geometry, usageFlags);
                        }
                        break;

                    case rpMATFXEFFECTDUAL:
                        break;

                    case rpMATFXEFFECTUVTRANSFORM:
                        break;

                    case rpMATFXEFFECTDUALUVTRANSFORM:
                        break;

                    default:
                        break;
                }
            }

            ++instancedData;
        }
        while (--numMeshes);
    }

    RWRETURN(D3D9DefaultInstanceCallbackAtomic(object, resEntryHeader, reinstance));
}

/****************************************************************************
 D3D9WorldSectorMatFXInstanceCallback

 */
static RwBool
D3D9WorldSectorMatFXInstanceCallback(void *object,
                                     RxD3D9ResEntryHeader *resEntryHeader,
                                     RwBool reinstance)
{
    const rpMatFXMaterialData *matFXData;
    RxD3D9InstanceData  *instancedData;
    RwUInt32            numMeshes;

    RWFUNCTION( RWSTRING( "D3D9WorldSectorMatFXInstanceCallback" ) );

    /* Now I can create my extra data depending on the effect */

    numMeshes = resEntryHeader->numMeshes;
    instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

    do
    {
        matFXData = *MATFXMATERIALGETCONSTDATA(instancedData->material);
        if( matFXData )
        {
            switch( matFXData->flags )
            {
                case rpMATFXEFFECTENVMAP:
                    break;

                case rpMATFXEFFECTBUMPMAP:
                case rpMATFXEFFECTBUMPENVMAP:
                    /* Only use tangents if D3D8 level harware */
                    if (VideoCardSupportsTangents)
                    {
                        RpWorldSector *sector;
                        RpD3D9WorldSectorUsageFlag usageFlags;

                        sector = (RpWorldSector *)object;

                        usageFlags = RpD3D9WorldSectorGetUsageFlags(sector);

                        usageFlags |= rpD3D9WORLDSECTORUSAGE_CREATETANGENTS;

                        RpD3D9WorldSectorSetUsageFlags(sector, usageFlags);
                    }
                    break;

                case rpMATFXEFFECTDUAL:
                    break;

                case rpMATFXEFFECTUVTRANSFORM:
                    break;

                case rpMATFXEFFECTDUALUVTRANSFORM:
                    break;

                default:
                    break;
            }
        }

        ++instancedData;
    }
    while (--numMeshes);

    RWRETURN(D3D9DefaultInstanceCallbackWorldSector(object, resEntryHeader, reinstance));
}

/****************************************************************************
 CalculatePerturbedUVs

 */
static RwBool
CalculatePerturbedUVs(RxD3D9ResEntryHeader *resEntryHeader,
                      RxD3D9InstanceData *instancedData,
                      RxD3D9BumpMapVertex *bumpVertex)
{
    const MatFXBumpMapData  *bumpMapData = MATFXD3D9BUMPMAPGETDATA(instancedData->material);
    const RwReal            factor = bumpMapData->coef * bumpMapData->invBumpWidth;
    RwFrame                 *bumpFrame  = bumpMapData->frame;
    D3DVERTEXELEMENT9       declaration[18];
    RwUInt8                 *lockedVertexBuffer[RWD3D9_MAX_VERTEX_STREAMS];
    RxD3D9VertexStream      *vertexStream;
    RwMatrix                objToWorld;
    RwMatrix                worldToObj;
    const RpLight           *light;
    RwV3d                   lightPosObj;
    RwUInt32                declarationIndex, stream;
    RwUInt8                 *positions, *normals, *prelit, *texcoords, *tangent;
    RwUInt32                positionsType, normalsType, texcoordsType, tangentType;
    RwV3d                   vert1, vert2, vert3;
    RwV2d                   coord1, coord2, coord3;
    RwUInt32                stridePos, strideNormal, stridePrelit, strideTex, strideTangent;
    RwInt32                 i;

    RWFUNCTION(RWSTRING("CalculatePerturbedUVs"));

    if (bumpFrame == NULL)
    {
        bumpFrame = RwCameraGetFrame(RwCameraGetCurrentCamera());
        RWASSERT(bumpFrame);
    }

    /* Get light vector */
    RwD3D9GetTransform(D3DTS_WORLD, &objToWorld);

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
    IDirect3DVertexDeclaration9_GetDeclaration((LPDIRECT3DVERTEXDECLARATION9)resEntryHeader->vertexDeclaration,
                                                declaration,
                                                &i);

    RWASSERT(i < 18);

    /*
     * Lock the vertex buffer
     */
    for (i = 0; i < RWD3D9_MAX_VERTEX_STREAMS; ++i)
    {
        lockedVertexBuffer[i] = NULL;

        vertexStream = &(resEntryHeader->vertexStream[i]);

        if (vertexStream->vertexBuffer != NULL)
        {
            if (vertexStream->geometryFlags & (rpGEOMETRYLOCKVERTICES | rpGEOMETRYLOCKNORMALS | rpGEOMETRYLOCKTEXCOORDSALL))
            {
                IDirect3DVertexBuffer9_Lock((LPDIRECT3DVERTEXBUFFER9)vertexStream->vertexBuffer,
                                            vertexStream->offset +
                                            (vertexStream->stride) * (instancedData->minVert),
                                            (vertexStream->stride) * (instancedData->numVertices),
                                            &(lockedVertexBuffer[i]),
                                            D3DLOCK_NOSYSLOCK);
            }
        }
    }

    /* Find positions and offsets */
    declarationIndex = 0;
    while (declaration[declarationIndex].Usage != D3DDECLUSAGE_POSITION ||
           declaration[declarationIndex].UsageIndex != 0)
    {
        ++declarationIndex;
    }

    RWASSERT(declarationIndex < 18);

    stream = declaration[declarationIndex].Stream;
    positions = lockedVertexBuffer[stream] + declaration[declarationIndex].Offset;
    positionsType = declaration[declarationIndex].Type;
    stridePos = resEntryHeader->vertexStream[stream].stride;

    /* normals */
    declarationIndex = 0;
    while ((declaration[declarationIndex].Usage != D3DDECLUSAGE_NORMAL ||
            declaration[declarationIndex].UsageIndex != 0) &&
           declaration[declarationIndex].Type != D3DDECLTYPE_UNUSED)
    {
        ++declarationIndex;
    }

    RWASSERT(declarationIndex < 18);

    if (declaration[declarationIndex].Type != D3DDECLTYPE_UNUSED)
    {
        stream = declaration[declarationIndex].Stream;
        normals = lockedVertexBuffer[stream] + declaration[declarationIndex].Offset;
        normalsType = declaration[declarationIndex].Type;
        strideNormal = resEntryHeader->vertexStream[stream].stride;
    }
    else
    {
        normals = NULL;
    }

    /* prelit */
    declarationIndex = 0;
    while ((declaration[declarationIndex].Usage != D3DDECLUSAGE_COLOR ||
            declaration[declarationIndex].UsageIndex != 0) &&
           declaration[declarationIndex].Type != D3DDECLTYPE_UNUSED)
    {
        ++declarationIndex;
    }

    RWASSERT(declarationIndex < 18);

    if (declaration[declarationIndex].Type != D3DDECLTYPE_UNUSED)
    {
        stream = declaration[declarationIndex].Stream;
        prelit = lockedVertexBuffer[stream] + declaration[declarationIndex].Offset;
        stridePrelit = resEntryHeader->vertexStream[stream].stride;
    }
    else
    {
        prelit = NULL;
    }

    /* texture coords */
    declarationIndex = 0;
    while (declaration[declarationIndex].Usage != D3DDECLUSAGE_TEXCOORD ||
           declaration[declarationIndex].UsageIndex != 0)
    {
        ++declarationIndex;
    }

    RWASSERT(declarationIndex < 18);

    stream = declaration[declarationIndex].Stream;
    texcoords = lockedVertexBuffer[stream] + declaration[declarationIndex].Offset;
    texcoordsType = declaration[declarationIndex].Type;
    strideTex = resEntryHeader->vertexStream[stream].stride;

    /* tangent */
    declarationIndex = 0;
    while ((declaration[declarationIndex].Usage != D3DDECLUSAGE_TANGENT ||
            declaration[declarationIndex].UsageIndex != 0) &&
           declaration[declarationIndex].Type != D3DDECLTYPE_UNUSED)
    {
        ++declarationIndex;
    }

    RWASSERT(declarationIndex < 18);

    if (declaration[declarationIndex].Type != D3DDECLTYPE_UNUSED)
    {
        stream = declaration[declarationIndex].Stream;
        tangent = lockedVertexBuffer[stream] + declaration[declarationIndex].Offset;
        tangentType = declaration[declarationIndex].Type;
        strideTangent = resEntryHeader->vertexStream[stream].stride;
    }
    else
    {
        tangent = NULL;
    }

    /*
     * Process vertex
     */
    if (tangent != NULL)
    {
        RwInt32 numVertices;
        RwV3d   l, n, t, b;
        RwReal  unused;

        /* We have tangent information */
        numVertices = instancedData->numVertices;

        for (i = 0; i < numVertices; ++i)
        {
            if (D3DDECLTYPE_FLOAT3 == positionsType)
            {
                const RwV3d *inputposition1 = (const RwV3d *)(positions + (i * stridePos));

                vert1 = *inputposition1;
            }
            else
            {
                _rpD3D9VertexDeclarationUnInstV3d(positionsType, &vert1, positions + (i * stridePos));
            }

            if (D3DDECLTYPE_FLOAT2 == texcoordsType)
            {
                const RwV2d *inputcoord1 = (const RwV2d *)(texcoords + (i * strideTex));

                coord1 = *inputcoord1;
            }
            else
            {
                _rpD3D9VertexDeclarationUnInstV2d(texcoordsType, &coord1, texcoords + (i * strideTex));
            }

            if (normals != NULL)
            {
                if (D3DDECLTYPE_FLOAT3 == normalsType)
                {
                    const RwV3d *inputnormal = (const RwV3d *)(normals + (i * strideNormal));

                    n = *inputnormal;
                }
                else
                {
                    _rpD3D9VertexDeclarationUnInstV3d(normalsType, &n, normals + (i * strideNormal));
                }
            }
            else
            {
                n.x = 0;
                n.y = 0;
                n.z = 0;
            }

            if (D3DDECLTYPE_FLOAT3 == tangentType)
            {
                const RwV3d *inputtangent1 = (const RwV3d *)(tangent + (i * strideTangent));

                t = *inputtangent1;
            }
            else
            {
                _rpD3D9VertexDeclarationUnInstV3d(tangentType, &t, tangent + (i * strideTangent));
            }

            RwV3dSub(&l, &lightPosObj, (const RwV3d *)&vert1);
            _rwV3dNormalizeMacro(unused, &l, &l);

            /* Check to see whether the light is behind the triangle */
            if (1) /* RwV3dDotProduct(&l, &n) > 0.0f) */
            {
                RwV2d               shift;
                RxD3D9BumpMapVertex *currentBumpVertex;

                /* Get binormal */
                RwV3dCrossProduct(&b, &t, &n);

                /*
                 * So now that we have b, t and n, we have the tangent
                 * space coordinate system axes.
                 */
                shift.x = RwV3dDotProduct(&t, &l);
                shift.y = RwV3dDotProduct(&b, &l);

                currentBumpVertex = &(bumpVertex[i]);

                currentBumpVertex->position.x = vert1.x;
                currentBumpVertex->position.y = vert1.y;
                currentBumpVertex->position.z = vert1.z;

                if (normals != NULL)
                {
                    currentBumpVertex->normal.x = n.x;
                    currentBumpVertex->normal.y = n.y;
                    currentBumpVertex->normal.z = n.z;
                }
                else
                {
                    currentBumpVertex->normal.x = 0;
                    currentBumpVertex->normal.y = 0;
                    currentBumpVertex->normal.z = 0;
                }

                if (prelit != NULL)
                {
                    currentBumpVertex->color = *((const RwUInt32 *)(prelit + (i * stridePrelit)));
                }
                else
                {
                    currentBumpVertex->color = 0xffffffff;
                }

                currentBumpVertex->texcoords1.u = coord1.x;
                currentBumpVertex->texcoords1.v = coord1.y;

                currentBumpVertex->texcoords2.u = coord1.x + (shift.x * factor);
                currentBumpVertex->texcoords2.v = coord1.y + (shift.y * factor);
            }
        }
    }
    else
    {
        RwUInt32                i1, i2, i3;

        if (resEntryHeader->indexBuffer)
        {
            RwUInt8                 *processedFlags;
            RwInt32                 maxIndex;
            RxVertexIndex           *inds;

            processedFlags = (RwUInt8 *) RwMalloc(instancedData->numVertices,
                rwID_MATERIALEFFECTSPLUGIN | rwMEMHINTDUR_EVENT);
            memset(processedFlags, 0, instancedData->numVertices);

            maxIndex = instancedData->numIndex;

            /* Get pointer to the index information */
            IDirect3DIndexBuffer9_Lock((LPDIRECT3DINDEXBUFFER9)resEntryHeader->indexBuffer,
                                    (instancedData->startIndex) * sizeof(RwUInt16),
                                    (instancedData->numIndex) * sizeof(RwUInt16),
                                    (RwUInt8 **)&inds,
                                    D3DLOCK_NOSYSLOCK);

            for (i = 0; i < maxIndex; ++i)
            {
                if (processedFlags[inds[i]] == FALSE)
                {
                    RwV3d   b, t, n, temp1, temp2, l;
                    RwReal  unused;

                    if (resEntryHeader->primType == D3DPT_TRIANGLELIST)
                    {
                        i1 = inds[i];
                        i2 = inds[((i/3)*3) + (i+1)%3];
                        i3 = inds[((i/3)*3) + (i+2)%3];
                    }
                    else if (resEntryHeader->primType == D3DPT_TRIANGLESTRIP)
                    {
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

                        i1 = inds[i1];
                        i2 = inds[i2];
                        i3 = inds[i3];
                    }

                    if (D3DDECLTYPE_FLOAT3 == positionsType)
                    {
                        const RwV3d *inputposition1 = (const RwV3d *)(positions + (i1 * stridePos));
                        const RwV3d *inputposition2 = (const RwV3d *)(positions + (i2 * stridePos));
                        const RwV3d *inputposition3 = (const RwV3d *)(positions + (i3 * stridePos));

                        vert1 = *inputposition1;
                        vert2 = *inputposition2;
                        vert3 = *inputposition3;
                    }
                    else
                    {
                        _rpD3D9VertexDeclarationUnInstV3d(positionsType, &vert1, positions + (i1 * stridePos));
                        _rpD3D9VertexDeclarationUnInstV3d(positionsType, &vert2, positions + (i2 * stridePos));
                        _rpD3D9VertexDeclarationUnInstV3d(positionsType, &vert3, positions + (i3 * stridePos));
                    }

                    if (D3DDECLTYPE_FLOAT2 == texcoordsType)
                    {
                        const RwV2d *inputcoord1 = (const RwV2d *)(texcoords + (i1 * strideTex));
                        const RwV2d *inputcoord2 = (const RwV2d *)(texcoords + (i2 * strideTex));
                        const RwV2d *inputcoord3 = (const RwV2d *)(texcoords + (i3 * strideTex));

                        coord1 = *inputcoord1;
                        coord2 = *inputcoord2;
                        coord3 = *inputcoord3;
                    }
                    else
                    {
                        _rpD3D9VertexDeclarationUnInstV2d(texcoordsType, &coord1, texcoords + (i1 * strideTex));
                        _rpD3D9VertexDeclarationUnInstV2d(texcoordsType, &coord2, texcoords + (i2 * strideTex));
                        _rpD3D9VertexDeclarationUnInstV2d(texcoordsType, &coord3, texcoords + (i3 * strideTex));
                    }

                    if (normals != NULL)
                    {
                        if (D3DDECLTYPE_FLOAT3 == normalsType)
                        {
                            const RwV3d *inputnormal = (const RwV3d *)(normals + (i1 * strideNormal));

                            n = *inputnormal;
                        }
                        else
                        {
                            _rpD3D9VertexDeclarationUnInstV3d(normalsType, &n, normals + (i1 * strideNormal));
                        }
                    }
                    else
                    {
                        n.x = 0;
                        n.y = 0;
                        n.z = 0;
                    }

                    RwV3dSub(&l, &lightPosObj, (const RwV3d *)&vert1);
                    _rwV3dNormalizeMacro(unused, &l, &l);

                    /* Check to see whether the light is behind the triangle */
                    if (1) /* RwV3dDotProduct(&l, &n) > 0.0f) */
                    {
                        RwV2d               shift;
                        RxD3D9BumpMapVertex *currentBumpVertex;

                        /* A nice little algorithm to find the tangent vector */
                        RwV3dSub(&temp1, &vert2, &vert1);
                        RwV3dSub(&temp2, &vert3, &vert1);

                        RwV3dScale(&temp1, &temp1, coord3.y - coord1.y);
                        RwV3dScale(&temp2, &temp2, coord2.y - coord1.y);

                        RwV3dSub(&t, &temp1, &temp2);
                        _rwV3dNormalizeMacro(unused, &t, &t);
                        RwV3dCrossProduct(&b, &t, &n);

                        /*
                         * So now that we have b, t and n, we have the tangent
                         * space coordinate system axes.
                         */
                        shift.x = RwV3dDotProduct(&t, &l);
                        shift.y = RwV3dDotProduct(&b, &l);

                        currentBumpVertex = &(bumpVertex[i1]);

                        currentBumpVertex->position.x = vert1.x;
                        currentBumpVertex->position.y = vert1.y;
                        currentBumpVertex->position.z = vert1.z;

                        if (normals != NULL)
                        {
                            currentBumpVertex->normal.x = n.x;
                            currentBumpVertex->normal.y = n.y;
                            currentBumpVertex->normal.z = n.z;
                        }
                        else
                        {
                            currentBumpVertex->normal.x = 0;
                            currentBumpVertex->normal.y = 0;
                            currentBumpVertex->normal.z = 0;
                        }

                        if (prelit != NULL)
                        {
                            currentBumpVertex->color = *((const RwUInt32 *)(prelit + (i1 * stridePrelit)));
                        }
                        else
                        {
                            currentBumpVertex->color = 0xffffffff;
                        }

                        currentBumpVertex->texcoords1.u = coord1.x;
                        currentBumpVertex->texcoords1.v = coord1.y;

                        currentBumpVertex->texcoords2.u = coord1.x - (shift.x * factor);
                        currentBumpVertex->texcoords2.v = coord1.y - (shift.y * factor);
                    }

                    processedFlags[i1] = TRUE;
                }
            }

            IDirect3DIndexBuffer9_Unlock((LPDIRECT3DINDEXBUFFER9)resEntryHeader->indexBuffer);

            RwFree(processedFlags);
        }
        else
        {
            const RwInt32 numVerts = instancedData->numVertices;

            for (i = 0; i < numVerts; ++i)
            {
                RwV3d   b, t, n, temp1, temp2, l;
                RwReal  unused;

                if (resEntryHeader->primType == D3DPT_TRIANGLELIST)
                {
                    i1 = i;
                    i2 = (((i/3)*3) + (i+1)%3);
                    i3 = (((i/3)*3) + (i+2)%3);
                }
                else if (resEntryHeader->primType == D3DPT_TRIANGLESTRIP)
                {
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
                }

                if (D3DDECLTYPE_FLOAT3 == positionsType)
                {
                    const RwV3d *inputposition1 = (const RwV3d *)(positions + (i1 * stridePos));
                    const RwV3d *inputposition2 = (const RwV3d *)(positions + (i2 * stridePos));
                    const RwV3d *inputposition3 = (const RwV3d *)(positions + (i3 * stridePos));

                    vert1 = *inputposition1;
                    vert2 = *inputposition2;
                    vert3 = *inputposition3;
                }
                else
                {
                    _rpD3D9VertexDeclarationUnInstV3d(positionsType, &vert1, positions + (i1 * stridePos));
                    _rpD3D9VertexDeclarationUnInstV3d(positionsType, &vert2, positions + (i2 * stridePos));
                    _rpD3D9VertexDeclarationUnInstV3d(positionsType, &vert3, positions + (i3 * stridePos));
                }

                if (D3DDECLTYPE_FLOAT2 == texcoordsType)
                {
                    const RwV2d *inputcoord1 = (const RwV2d *)(texcoords + (i1 * strideTex));
                    const RwV2d *inputcoord2 = (const RwV2d *)(texcoords + (i2 * strideTex));
                    const RwV2d *inputcoord3 = (const RwV2d *)(texcoords + (i3 * strideTex));

                    coord1 = *inputcoord1;
                    coord2 = *inputcoord2;
                    coord3 = *inputcoord3;
                }
                else
                {
                    _rpD3D9VertexDeclarationUnInstV2d(texcoordsType, &coord1, texcoords + (i1 * strideTex));
                    _rpD3D9VertexDeclarationUnInstV2d(texcoordsType, &coord2, texcoords + (i2 * strideTex));
                    _rpD3D9VertexDeclarationUnInstV2d(texcoordsType, &coord3, texcoords + (i3 * strideTex));
                }

                if (normals != NULL)
                {
                    if (D3DDECLTYPE_FLOAT3 == normalsType)
                    {
                        const RwV3d *inputnormal = (const RwV3d *)(normals + (i1 * strideNormal));

                        n = *inputnormal;
                    }
                    else
                    {
                        _rpD3D9VertexDeclarationUnInstV3d(normalsType, &n, normals + (i1 * strideNormal));
                    }
                }
                else
                {
                    n.x = 0;
                    n.y = 0;
                    n.z = 0;
                }

                RwV3dSub(&l, &lightPosObj, (const RwV3d *)&vert1);
                _rwV3dNormalizeMacro(unused, &l, &l);

                /* Check to see whether the light is behind the triangle */
                if (1) /* RwV3dDotProduct(&l, &n) > 0.0f) */
                {
                    RwV2d               shift;
                    RxD3D9BumpMapVertex *currentBumpVertex;

                    /* A nice little algorithm to find the tangent vector */
                    RwV3dSub(&temp1, &vert2, &vert1);
                    RwV3dSub(&temp2, &vert3, &vert1);

                    RwV3dScale(&temp1, &temp1, coord3.y - coord1.y);
                    RwV3dScale(&temp2, &temp2, coord2.y - coord1.y);

                    RwV3dSub(&t, &temp1, &temp2);
                    _rwV3dNormalizeMacro(unused, &t, &t);
                    RwV3dCrossProduct(&b, &t, &n);

                    /*
                    * So now that we have b, t and n, we have the tangent
                    * space coordinate system axes.
                    */
                    shift.x = RwV3dDotProduct(&t, &l);
                    shift.y = RwV3dDotProduct(&b, &l);

                    currentBumpVertex = &(bumpVertex[i1]);

                    currentBumpVertex->position.x = vert1.x;
                    currentBumpVertex->position.y = vert1.y;
                    currentBumpVertex->position.z = vert1.z;

                    if (normals != NULL)
                    {
                        currentBumpVertex->normal.x = n.x;
                        currentBumpVertex->normal.y = n.y;
                        currentBumpVertex->normal.z = n.z;
                    }
                    else
                    {
                        currentBumpVertex->normal.x = 0;
                        currentBumpVertex->normal.y = 0;
                        currentBumpVertex->normal.z = 0;
                    }

                    if (prelit != NULL)
                    {
                        currentBumpVertex->color = *((const RwUInt32 *)(prelit + (i1 * stridePrelit)));
                    }
                    else
                    {
                        currentBumpVertex->color = 0xffffffff;
                    }

                    currentBumpVertex->texcoords1.u = coord1.x;
                    currentBumpVertex->texcoords1.v = coord1.y;

                    currentBumpVertex->texcoords2.u = coord1.x - (shift.x * factor);
                    currentBumpVertex->texcoords2.v = coord1.y - (shift.y * factor);
                }
            }
        }
    }

    /*
     * Unlock the vertex buffer
     */
    for (i = 0; i < RWD3D9_MAX_VERTEX_STREAMS; ++i)
    {
        if (lockedVertexBuffer[i] != NULL)
        {
            IDirect3DVertexBuffer9_Unlock((LPDIRECT3DVERTEXBUFFER9)(resEntryHeader->vertexStream[i].vertexBuffer));
        }
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

    RwD3D9SetTexture(texture, stage);

    /* Check cube textures */
    if (_rwD3D9RasterIsCubeRaster(RwTextureGetRaster(texture)))
    {
        const RwMatrix    *camMtx;
        RwMatrix          invMtx;

        RwD3D9SetTextureStageState(stage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3);
        RwD3D9SetTextureStageState(stage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);

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

        RwD3D9SetTransform(D3DTS_TEXTURE0 + stage, (const D3DMATRIX *)&invMtx);
    }
    else
    {
        /* Generate spheremap texture coords from the position */
        RwD3D9SetTextureStageState(stage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
        RwD3D9SetTextureStageState(stage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACENORMAL);

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

            RwMatrixMultiply(&tmpMtx, camMtx, &invMtx);

            tmpMtx.right.x = -tmpMtx.right.x;
            tmpMtx.right.y = -tmpMtx.right.y;
            tmpMtx.right.z = -tmpMtx.right.z;

            tmpMtx.flags = 0;

            tmpMtx.pos.x = 0.0f;
            tmpMtx.pos.y = 0.0f;
            tmpMtx.pos.z = 0.0f;

            RwMatrixMultiply(&result, &tmpMtx, &texMat);

            RwD3D9SetTransform(D3DTS_TEXTURE0 + stage, &result);
        }
        else
        {
            RwD3D9SetTransform(D3DTS_TEXTURE0 + stage, &texMat);
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

    RWFUNCTION(RWSTRING("ApplyAnimTextureMatrix"));

    if ( matrix != NULL &&
         FALSE == rwMatrixTestFlags(matrix, rwMATRIXINTERNALIDENTITY) )
    {
        RwD3D9SetTextureStageState(stage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);

        texMat.right.x = matrix->right.x;
        texMat.right.y = matrix->right.y;

        texMat.up.x = matrix->up.x;
        texMat.up.y = matrix->up.y;

        texMat.at.x = matrix->pos.x;
        texMat.at.y = matrix->pos.y;

        RwD3D9SetTransform(D3DTS_TEXTURE0 + stage, &texMat);
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpMatFXD3D9AtomicMatFXDefaultRender

 Purpose:

 On entry:

 On exit:
 */
void
_rpMatFXD3D9AtomicMatFXDefaultRender(RxD3D9ResEntryHeader *resEntryHeader,
                                     RxD3D9InstanceData *instancedData,
                                     RwUInt32 flags,
                                     RwTexture *baseTexture)
{
    const MatFXUVAnimData *uvAnim = NULL;

    RWFUNCTION(RWSTRING("_rpMatFXD3D9AtomicMatFXDefaultRender"));

    if (flags & (rxGEOMETRY_TEXTURED|rpGEOMETRYTEXTURED2))
    {
        const rpMatFXMaterialData   *matFXData;

        RwD3D9SetTexture(baseTexture, 0);

        matFXData = *MATFXMATERIALGETCONSTDATA(instancedData->material);

        if (matFXData != NULL &&
            matFXData->flags == rpMATFXEFFECTUVTRANSFORM)
        {
            uvAnim = MATFXD3D9UVANIMGETDATA(instancedData->material);

            ApplyAnimTextureMatrix(0, uvAnim->baseTransform);
        }

		//@{ Jaewon 20041125
        RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   TextureOpModulation);
		//@} Jaewon
        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    }
    else
    {
        RwD3D9SetTexture(NULL, 0);

        RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    }

    /*
     * Set the default Pixel shader
     */
    RwD3D9SetPixelShader(NULL);

    /*
     * Vertex shader
     */
    RwD3D9SetVertexShader(instancedData->vertexShader);

    /*
     * Draw the geometry
     */
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

    if (uvAnim != NULL)
    {
        RwD3D9SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpMatFXD3D9AtomicMatFXRenderBlack

 Purpose:

 On entry:

 On exit:
 */
void
_rpMatFXD3D9AtomicMatFXRenderBlack(RxD3D9ResEntryHeader *resEntryHeader,
                                   RxD3D9InstanceData *instancedData)
{
    RWFUNCTION(RWSTRING("_rpMatFXD3D9AtomicMatFXRenderBlack"));

    /*
     * Set the default Pixel shader
     */
    RwD3D9SetPixelShader(NULL);

    /*
     * Vertex shader
     */
    RwD3D9SetVertexShader(instancedData->vertexShader);

    /*
     * Draw the geometry
     */
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

    RWRETURNVOID();
}

/****************************************************************************
 _rpMatFXD3D9AtomicMatFXDualPassRender

 Purpose:

 On entry:

 On exit:
 */
void
_rpMatFXD3D9AtomicMatFXDualPassRender(RxD3D9ResEntryHeader *resEntryHeader,
                                      RxD3D9InstanceData *instancedData,
                                      RwUInt32 flags,
                                      RwTexture *baseTexture,
                                      RwTexture *dualPassTexture)
{
    RWFUNCTION(RWSTRING("_rpMatFXD3D9AtomicMatFXDualPassRender"));

    if ( dualPassTexture && dualPassTexture->raster)
    {
        const MatFXDualData   *dualData;
        const MatFXUVAnimData *uvAnim = NULL;

        RwBool  needSecondPass = TRUE;

        if ((*MATFXMATERIALGETCONSTDATA(instancedData->material))->flags !=
            rpMATFXEFFECTDUALUVTRANSFORM)
        {
            dualData = MATFXD3D9DUALGETDATA(instancedData->material);
        }
        else
        {
            dualData = MATFXD3D9DUALUVANIMGETDUALDATA(instancedData->material);

            uvAnim = MATFXD3D9UVANIMGETDATA(instancedData->material);
        }

        if (flags & (rxGEOMETRY_TEXTURED|rpGEOMETRYTEXTURED2))
        {
            RwD3D9SetTexture(baseTexture, 0);

            RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        }
        else
        {
            RwD3D9SetTexture(NULL, 0);

            RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        }

        if (uvAnim != NULL)
        {
            ApplyAnimTextureMatrix(0, uvAnim->baseTransform);
        }

        /* Check palettized textures */
        if (baseTexture &&
            (RwRasterGetFormat(RwTextureGetRaster(baseTexture)) & rwRASTERFORMATPAL8) != 0 &&
            (RwRasterGetFormat(RwTextureGetRaster(dualPassTexture)) & rwRASTERFORMATPAL8) != 0 &&
            baseTexture != dualPassTexture)
        {
            _rwD3D9RasterConvertToNonPalettized(dualPassTexture->raster);
        }

        /* Check for some blend modes optimizations */
        if (VideoCardSupportsMultitexture)
        {
            if ( (dualData->srcBlendMode == rwBLENDDESTCOLOR &&
                  dualData->dstBlendMode == rwBLENDZERO) ||
                 (dualData->srcBlendMode == rwBLENDZERO &&
                  dualData->dstBlendMode == rwBLENDSRCCOLOR))
            {
                if (baseTexture != NULL)
                {
                    RwD3D9SetTexture(dualPassTexture, 1);

                    /* Some old cards with 3 stages need the diffuse in the last one */
                    if (VideoCardMaxTextureBlendStages == 3)
                    {
                        RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
                        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

                        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
                        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

                        RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                        RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                        RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

						//@{ Jaewon 20041125
                        RwD3D9SetTextureStageState(2, D3DTSS_COLOROP,   TextureOpModulation);
						//@} Jaewon
                        RwD3D9SetTextureStageState(2, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
                        RwD3D9SetTextureStageState(2, D3DTSS_COLORARG2, D3DTA_CURRENT);

                        RwD3D9SetTextureStageState(2, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
                        RwD3D9SetTextureStageState(2, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
                        RwD3D9SetTextureStageState(2, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
                    }
                    else
                    {
                        /* This could fail in very old cards */
						//@{ Jaewon 20041125
                        RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   TextureOpModulation);
						//@} Jaewon
                        RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                        RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
                    }
                }
                else
                {
                    RwD3D9SetTexture(dualPassTexture, 0);

					//@{ Jaewon 20041125
                    RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   TextureOpModulation);
					//@} Jaewon
                    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
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
                        RwD3D9SetTexture(dualPassTexture, 1);

                        RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
                        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

                        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
                        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

                        RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_BLENDTEXTUREALPHA);
                        RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                        RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_BLENDTEXTUREALPHA);
                        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
                        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

						//@{ Jaewon 20041125
                        RwD3D9SetTextureStageState(2, D3DTSS_COLOROP,   TextureOpModulation);
						//@} Jaewon
                        RwD3D9SetTextureStageState(2, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
                        RwD3D9SetTextureStageState(2, D3DTSS_COLORARG2, D3DTA_CURRENT);

                        RwD3D9SetTextureStageState(2, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
                        RwD3D9SetTextureStageState(2, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
                        RwD3D9SetTextureStageState(2, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

                        needSecondPass = FALSE;
                    }
                }
                else
                {
                    RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, 0xffffffff);

                    RwD3D9SetTexture(dualPassTexture, 0);

                    RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_BLENDTEXTUREALPHA);
                    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);

					//@{ Jaewon 20041125
                    RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   TextureOpModulation);
					//@} Jaewon
                    RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
                    RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                    RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
                    RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
                    RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

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
                        RwD3D9SetTexture(dualPassTexture, 1);

                        RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE2X);
                        RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                        RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                        /* Some old cards with 3 stages need the diffuse in the last one */
                        if (VideoCardMaxTextureBlendStages == 3)
                        {
                            RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
                            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

                            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
                            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

                            RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                            RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

                            RwD3D9SetTextureStageState(2, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                            RwD3D9SetTextureStageState(2, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
                            RwD3D9SetTextureStageState(2, D3DTSS_COLORARG2, D3DTA_CURRENT);

                            RwD3D9SetTextureStageState(2, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
                            RwD3D9SetTextureStageState(2, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
                            RwD3D9SetTextureStageState(2, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
                        }
                        else
                        {
                            /* This could fail in very old cards */
                            RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                            RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
                        }
                    }
                    else
                    {
                        RwD3D9SetTexture(dualPassTexture, 0);

                        RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE2X);

                        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
                    }

                    if (NeedToValidateModulate2X)
                    {
                        RwUInt32 numPass = 0;

                        _rwD3D9RenderStateFlushCache();

                        if (FAILED(IDirect3DDevice9_ValidateDevice(_RwD3DDevice, (DWORD *)&numPass)) ||
                            numPass != 1)
                        {
                            VideoCardSupportsModulate2X = FALSE;

                            RwD3D9SetTexture(baseTexture, 0);

                            RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

                            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
                            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
                            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

                            RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
                            RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);

                            RwD3D9SetTextureStageState(2, D3DTSS_COLOROP,   D3DTOP_DISABLE);
                            RwD3D9SetTextureStageState(2, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);

                            RwD3D9SetTexture(NULL, 1);
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
                        RwD3D9SetTexture(dualPassTexture, 1);

                        RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MULTIPLYADD);
                        RwD3D9SetTextureStageState(1, D3DTSS_COLORARG0, D3DTA_CURRENT);
                        RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                        RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

                        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

                        needSecondPass = FALSE;
                    }
                }
                else
                {
                    RwD3D9SetTexture(dualPassTexture, 0);

                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

                    RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_ADD);
                    RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
                    RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                    RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                    RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

                    needSecondPass = FALSE;
                }
            }
            else if ( dualData->srcBlendMode == rwBLENDZERO &&
                      dualData->dstBlendMode == rwBLENDSRCALPHA )
            {
                if (baseTexture != NULL)
                {
                    RwD3D9SetTexture(dualPassTexture, 1);

                    /* Some old cards with 3 stages need the diffuse in the last one */
                    if (VideoCardMaxTextureBlendStages == 3)
                    {
                        RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
                        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

                        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
                        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

                        RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                        RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE | D3DTA_ALPHAREPLICATE);
                        RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

						//@{ Jaewon 20041125
                        RwD3D9SetTextureStageState(2, D3DTSS_COLOROP,   TextureOpModulation);
						//@} Jaewon
                        RwD3D9SetTextureStageState(2, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
                        RwD3D9SetTextureStageState(2, D3DTSS_COLORARG2, D3DTA_CURRENT);

                        RwD3D9SetTextureStageState(2, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
                        RwD3D9SetTextureStageState(2, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
                        RwD3D9SetTextureStageState(2, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
                    }
                    else
                    {
                        /* This could fail in very old cards*/
						//@{ Jaewon 20041125
                        RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   TextureOpModulation);
						//@} Jaewon
                        RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE | D3DTA_ALPHAREPLICATE);
                        RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
                    }
                }
                else
                {
                    RwD3D9SetTexture(dualPassTexture, 0);

					//@{ Jaewon 20041125
                    RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   TextureOpModulation);
					//@} Jaewon
                    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE | D3DTA_ALPHAREPLICATE);
                    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
                }

                needSecondPass = FALSE;
            }

            if (!needSecondPass)
            {
                if ((flags & rpGEOMETRYTEXTURED2) == 0)
                {
                    RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 0);
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
        RwD3D9SetPixelShader(NULL);

        /*
         * Vertex shader
         */
        RwD3D9SetVertexShader(instancedData->vertexShader);

        /*
         * Draw the geometry
         */
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
         * Draw second pass
         */
        if (needSecondPass)
        {
            RwBlendFunction srcBlend, destBlend;
            RwBool      zWriteEnable;
            RwBool      fogEnabled;
            RwUInt32    fogColor;

            RwD3D9SetTexture(dualPassTexture, 0);

            if (uvAnim != NULL)
            {
                ApplyAnimTextureMatrix(0, uvAnim->dualTransform);
            }

            if (!_rwD3D9RenderStateIsVertexAlphaEnable())
            {
                _rwD3D9RenderStateVertexAlphaEnable(TRUE);
            }

            /* Remove alpha test for some combos */
            if ( (dualData->srcBlendMode == rwBLENDDESTCOLOR && dualData->dstBlendMode == rwBLENDZERO) ||
                 (dualData->srcBlendMode == rwBLENDZERO && dualData->dstBlendMode == rwBLENDSRCCOLOR) ||
                 (dualData->srcBlendMode == rwBLENDDESTCOLOR && dualData->dstBlendMode == rwBLENDSRCCOLOR) )
            {
                RwD3D9SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
            }

            /*
             * Set appropiate blending mode
             */
            RwRenderStateGet(rwRENDERSTATESRCBLEND, (void *)&srcBlend);
            RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void *)&destBlend);

            _rwD3D9RenderStateSrcBlend(dualData->srcBlendMode);
            _rwD3D9RenderStateDestBlend(dualData->dstBlendMode);

            RwD3D9GetRenderState(D3DRS_ZWRITEENABLE, (void *)&zWriteEnable);
            RwD3D9SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

            RwD3D9GetRenderState(D3DRS_FOGENABLE, (void *)&fogEnabled);
            if (fogEnabled)
            {
                RwD3D9GetRenderState(D3DRS_FOGCOLOR, (void *)&fogColor);

                if (dualData->dstBlendMode == rwBLENDONE)
                {
                    RwD3D9SetRenderState(D3DRS_FOGCOLOR, 0);
                }
                else if ( dualData->srcBlendMode == rwBLENDDESTCOLOR ||
                          dualData->dstBlendMode == rwBLENDSRCCOLOR )
                {
                    RwD3D9SetRenderState(D3DRS_FOGCOLOR, 0xffffffff);
                }
            }

            if (flags & rpGEOMETRYTEXTURED2)
            {
                RwD3D9SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);
            }

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

            if (fogEnabled)
            {
                RwD3D9SetRenderState(D3DRS_FOGCOLOR, fogColor);
            }

            if (flags & rpGEOMETRYTEXTURED2)
            {
                RwD3D9SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
            }

            _rwD3D9RenderStateVertexAlphaEnable(FALSE);

            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)srcBlend);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)destBlend);
        }
        else
        {
            RwD3D9SetTexture(NULL, 1);

            RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

            RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
            RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);

            RwD3D9SetTextureStageState(2, D3DTSS_COLOROP,   D3DTOP_DISABLE);
            RwD3D9SetTextureStageState(2, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);

            if ((flags & rpGEOMETRYTEXTURED2) == 0)
            {
                RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
            }
        }

        if (uvAnim != NULL)
        {
            RwD3D9SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
            RwD3D9SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
        }
    }
    else
    {
        _rpMatFXD3D9AtomicMatFXDefaultRender(resEntryHeader, instancedData, flags, baseTexture);
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpMatFXD3D9AtomicMatFXEnvRender

 Purpose:

 On entry:

 On exit:
 */
void
_rpMatFXD3D9AtomicMatFXEnvRender(RxD3D9ResEntryHeader *resEntryHeader,
                                 RxD3D9InstanceData *instancedData,
                                 RwUInt32 flags,
                                 RwUInt32 pass,
                                 RwTexture *baseTexture,
                                 RwTexture *envMapTexture)
{
    const MatFXEnvMapData *envMapData;
    RwUInt32        shiney;
	//@{ Jaewon 20041220
	RwBool			alphaBlend;
	//@} Jaewon

    RWFUNCTION(RWSTRING("_rpMatFXD3D9AtomicMatFXEnvRender"));

    envMapData = MATFXD3D9ENVMAPGETDATA(instancedData->material, pass);

    shiney = (RwFastRealToUInt32(envMapData->coef * 255) & 0xFF);

    if (shiney && envMapTexture && envMapTexture->raster)
    {
        RwBool  useEnvMapPixelShader = FALSE;
        RwBool  useMultitexture = FALSE;
        RwBool  hasBaseTexture = FALSE;

        /*
         * Vertex shader
         */
        RwD3D9SetVertexShader(instancedData->vertexShader);

        if (pass == rpSECONDPASS)
        {
            /* Set the base texture */
            if (flags & (rxGEOMETRY_TEXTURED|rpGEOMETRYTEXTURED2) &&
                baseTexture)
            {
                RwD3D9SetTexture(baseTexture, 0);

                hasBaseTexture = TRUE;
            }
            else
            {
                RwD3D9SetTexture(NULL, 0);
            }

            /* Check palettized textures */
            if (hasBaseTexture &&
                (RwRasterGetFormat(RwTextureGetRaster(baseTexture)) & rwRASTERFORMATPAL8) != 0 &&
                (RwRasterGetFormat(RwTextureGetRaster(envMapTexture)) & rwRASTERFORMATPAL8) != 0 &&
                baseTexture != envMapTexture)
            {
                _rwD3D9RasterConvertToNonPalettized(envMapTexture->raster);
            }

            /* Choose code path */
			//@{ Jaewon 20041220
			// to do alpha-blending in an env-map fx
            //if ( envMapData->useFrameBufferAlpha ||
            //     !( instancedData->vertexAlpha ||
            //     0xFF != instancedData->material->color.alpha ||
            //     (hasBaseTexture && _rwD3D9TextureHasAlpha(baseTexture)) ) )
			//@} Jaewon
            {
                if (_rwD3D9TextureHasAlpha(envMapTexture))
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
                else if (shiney == 0xFF)
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

            /*
             * Set the default Pixel shader
             */
            if (!useEnvMapPixelShader)
            {
                RwD3D9SetPixelShader(NULL);

                if (hasBaseTexture)
                {
                    RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
                }
                else
                {
                    RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
                    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
                }
            }

            /*
             * Draw the geometry if dual pass
             */
            if (!useEnvMapPixelShader && !useMultitexture)
            {
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

            RwD3D9SetPixelShaderConstant(0, ShinyFloats, 1);

			//@{ Jaewon 20041220
			// to do alpha-blending in an env-map fx
			RwD3D9GetRenderState(D3DRS_ALPHABLENDENABLE, &alphaBlend);
			if ( instancedData->vertexAlpha ||
					0xFF != instancedData->material->color.alpha ||
					(hasBaseTexture && _rwD3D9TextureHasAlpha(baseTexture)) )
			{
				RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			}
			//@} Jaewon

            if (hasBaseTexture)
            {
                ApplyEnvMapTextureMatrix(envMapTexture, 1, envMapData->frame);

                /* Set pixel shader */
                RwD3D9SetPixelShader(EnvMapPixelShader);

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

                RwD3D9SetTexture(NULL, 1);

                RwD3D9SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
                RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
            }
            else
            {
                ApplyEnvMapTextureMatrix(envMapTexture, 0, envMapData->frame);

                /* Set pixel shader */
                RwD3D9SetPixelShader(EnvMapNoBaseTexturePixelShader);

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

                RwD3D9SetTexture(NULL, 0);

                RwD3D9SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
                RwD3D9SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
            }

			//@{ Jaewon 20041220
			// to do alpha-blending in an env-map fx
			RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, alphaBlend);
			//@} Jaewon
        }
        else if (useMultitexture)
        {
            if (shiney != 0xFF || hasBaseTexture)
            {
                ApplyEnvMapTextureMatrix(envMapTexture, 1, envMapData->frame);

                if (shiney != 0xFF)
                {
                    /* Set the shiney factor and the correct texture stages */
                    shiney = ((shiney << 24) |
                              (shiney << 16) |
                              (shiney << 8) |
                               shiney);

                    RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, shiney);

                    RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MULTIPLYADD);
                    RwD3D9SetTextureStageState(1, D3DTSS_COLORARG0, D3DTA_CURRENT);
                    RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                    RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_TFACTOR);
                }
                else
                {
                    RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_ADD);
                    RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                    RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                    if (NeedToValidateDiffuseInStage0)
                    {
                        RwUInt32 numPass = 0;

                        _rwD3D9RenderStateFlushCache();

                        if (FAILED(IDirect3DDevice9_ValidateDevice(_RwD3DDevice,
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

				//@{ Jaewon 20041220
				// to do alpha-blending in an env-map fx
				RwD3D9GetRenderState(D3DRS_ALPHABLENDENABLE, &alphaBlend);
				if ( instancedData->vertexAlpha ||
						0xFF != instancedData->material->color.alpha ||
						(hasBaseTexture && _rwD3D9TextureHasAlpha(baseTexture)) )
				{
					RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				}
				//@} Jaewon

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

				//@{ Jaewon 20041220
				// to do alpha-blending in an env-map fx
				RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, alphaBlend);
				//@} Jaewon

                RwD3D9SetTexture(NULL, 1);

                RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
                RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);

                RwD3D9SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
                RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
            }
            else
            {
                ApplyEnvMapTextureMatrix(envMapTexture, 0, envMapData->frame);

                RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_ADD);

				//@{ Jaewon 20041220
				// to do alpha-blending in an env-map fx
				RwD3D9GetRenderState(D3DRS_ALPHABLENDENABLE, &alphaBlend);
				if ( instancedData->vertexAlpha ||
						0xFF != instancedData->material->color.alpha ||
						(hasBaseTexture && _rwD3D9TextureHasAlpha(baseTexture)) )
				{
					RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				}
				//@} Jaewon

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

				//@{ Jaewon 20041220
				// to do alpha-blending in an env-map fx
				RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, alphaBlend);
				//@} Jaewon

                RwD3D9SetTexture(NULL, 0);

                RwD3D9SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
                RwD3D9SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
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

            if (!_rwD3D9RenderStateIsVertexAlphaEnable())
            {
                _rwD3D9RenderStateVertexAlphaEnable(TRUE);
            }

            /* Set needed blending modes for envmap */
            RwRenderStateGet(rwRENDERSTATESRCBLEND, (void *)&srcBlend);
            RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void *)&destBlend);

            _rwD3D9RenderStateSrcBlend(rwBLENDSRCALPHA);
            _rwD3D9RenderStateDestBlend(rwBLENDONE);

            /* Set the shinnyness */
            shiney = ((shiney << 24) |
                      (shiney << 16) |
                      (shiney << 8) |
                       shiney);

            if (shiney<0xFFFFFFFF)
            {
                RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, shiney);

                RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
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

            RwD3D9GetRenderState(D3DRS_LIGHTING, (void *)&lighting);
            RwD3D9GetRenderState(D3DRS_ZWRITEENABLE, (void *)&zWriteEnable);
            RwD3D9GetRenderState(D3DRS_FOGENABLE, (void *)&fogEnabled);

            RwD3D9SetRenderState(D3DRS_LIGHTING, FALSE);
            RwD3D9SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

            if (fogEnabled)
            {
                RwD3D9GetRenderState(D3DRS_FOGCOLOR, (void *)&fogColor);
                RwD3D9SetRenderState(D3DRS_FOGCOLOR, 0);
            }

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
            RwD3D9SetRenderState(D3DRS_LIGHTING, lighting);

            if (shiney<0xFFFFFFFF)
            {
                RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
            }
            else
            {
                RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
            }

            _rwD3D9RenderStateVertexAlphaEnable(FALSE);

            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)srcBlend);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)destBlend);

            RwD3D9SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
            RwD3D9SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_PASSTHRU);
        }
    }
    else
    {
        if (pass == rpSECONDPASS)
        {
            _rpMatFXD3D9AtomicMatFXDefaultRender(resEntryHeader, instancedData, flags, baseTexture);
        }
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpMatFXD3D9AtomicMatFXBumpMapRender

 Purpose:

 On entry:

 On exit:
 */
void
_rpMatFXD3D9AtomicMatFXBumpMapRender(RxD3D9ResEntryHeader *resEntryHeader,
                                     RxD3D9InstanceData *instancedData,
                                     RwUInt32 flags,
                                     RwTexture *baseTexture,
                                     RwTexture *bumpTexture,
                                     RwTexture *envMapTexture)
{
    RWFUNCTION(RWSTRING("_rpMatFXD3D9AtomicMatFXBumpMapRender"));

    if (bumpTexture == NULL)
    {
        if (baseTexture != NULL &&
            _rwD3D9TextureHasAlpha(baseTexture))
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
        LPDIRECT3DVERTEXBUFFER9 vertexBufferBumpMap;
        RxD3D9BumpMapVertex *bufferMem;

        /* Fill Vertex Buffer */
        if (RwD3D9DynamicVertexBufferLock(sizeof(RxD3D9BumpMapVertex),
                                                instancedData->numVertices,
                                                (void**)&vertexBufferBumpMap,
                                                (void**)&bufferMem,
                                                &vbBumpMapOffset))
        {
            RwBool  zWriteEnable;

            CalculatePerturbedUVs(resEntryHeader, instancedData, bufferMem);

            RwD3D9DynamicVertexBufferUnlock(vertexBufferBumpMap);

            /*
             * Set base textures
             */
            RwD3D9SetTexture(bumpTexture, 0);

            RwD3D9SetFVF(BUMPMAP_FVF);

            /*
             * Set the stream source
             */
            RwD3D9SetStreamSource(0, vertexBufferBumpMap, 0, sizeof(RxD3D9BumpMapVertex));

            /*
             * Vertex shader
             */
            RwD3D9SetVertexShader(instancedData->vertexShader);

            /*
             * Draw effect
             */
            if ( (envMapTexture && BumpEnvMapPixelShader) || BumpMapPixelShader )
            {
                RwBool alphaBlend;
                RwBool alphaTest;

                _rwD3D9RenderStateVertexAlphaEnable(FALSE);

                RwD3D9SetTexture(bumpTexture, 1);

                /* We need to force it due to alpha texture */
                RwD3D9GetRenderState(D3DRS_ALPHABLENDENABLE, &alphaBlend);
                RwD3D9GetRenderState(D3DRS_ALPHATESTENABLE, &alphaTest);

                RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
                RwD3D9SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

                /*
                 * Pixel shader
                 */
                if (envMapTexture && BumpEnvMapPixelShader)
                {
                    const MatFXEnvMapData *envMapData = MATFXD3D9ENVMAPGETDATA(instancedData->material, rpTHIRDPASS);

                    const RwUInt32        shiney = (RwFastRealToUInt32(envMapData->coef * 255) & 0xFF);

                    if (shiney)
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
                            if (_rwD3D9TextureHasAlpha(envMapTexture))
                            {
                                RwD3D9SetPixelShader(BumpModulatedEnvMapPixelShader);
                            }
                            else
                            {
                                RwD3D9SetPixelShader(BumpModulatedEnvMapNoAlphaPixelShader);
                            }
                        }
                        else
                        {
                            RwD3D9SetPixelShader(BumpEnvMapPixelShader);
                        }

                        RwD3D9SetPixelShaderConstant(0, ShinyFloats, 1);

                        if (resEntryHeader->indexBuffer != NULL)
                        {
                            RwD3D9DrawIndexedPrimitive((D3DPRIMITIVETYPE)resEntryHeader->primType,
                                                    vbBumpMapOffset,
                                                    0, instancedData->numVertices,
                                                    instancedData->startIndex, instancedData->numPrimitives);
                        }
                        else
                        {
                            RwD3D9DrawPrimitive((D3DPRIMITIVETYPE)resEntryHeader->primType,
                                                vbBumpMapOffset,
                                                instancedData->numPrimitives);
                        }

                        RwD3D9SetTexture(NULL, 2);

                        RwD3D9SetTextureStageState(2, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
                        RwD3D9SetTextureStageState(2, D3DTSS_TEXCOORDINDEX, 2);
                    }
                    else
                    {
                        RwD3D9SetPixelShader(BumpMapPixelShader);

                        if (resEntryHeader->indexBuffer != NULL)
                        {
                            RwD3D9DrawIndexedPrimitive((D3DPRIMITIVETYPE)resEntryHeader->primType,
                                                    vbBumpMapOffset,
                                                    0, instancedData->numVertices,
                                                    instancedData->startIndex, instancedData->numPrimitives);
                        }
                        else
                        {
                            RwD3D9DrawPrimitive((D3DPRIMITIVETYPE)resEntryHeader->primType,
                                                vbBumpMapOffset,
                                                instancedData->numPrimitives);
                        }
                    }
                }
                else
                {
                    RwD3D9SetPixelShader(BumpMapPixelShader);

                    if (resEntryHeader->indexBuffer != NULL)
                    {
                            RwD3D9DrawIndexedPrimitive((D3DPRIMITIVETYPE)resEntryHeader->primType,
                                                    vbBumpMapOffset,
                                                    0, instancedData->numVertices,
                                                    instancedData->startIndex, instancedData->numPrimitives);
                    }
                    else
                    {
                        RwD3D9DrawPrimitive((D3DPRIMITIVETYPE)resEntryHeader->primType,
                                            vbBumpMapOffset,
                                            instancedData->numPrimitives);
                    }
                }

                /*
                 * Restore default values
                 */
                RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, alphaBlend);
                RwD3D9SetRenderState(D3DRS_ALPHATESTENABLE, alphaTest);

                RwD3D9SetTexture(NULL, 1);
            }
            else
            {
                RwBlendFunction srcBlend, destBlend;

                RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
                RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

                /*
                 * Set the default Pixel shader
                 */
                RwD3D9SetPixelShader(NULL);

                /*
                 * First pass
                 */
                if (!_rwD3D9RenderStateIsVertexAlphaEnable())
                {
                    _rwD3D9RenderStateVertexAlphaEnable(TRUE);
                }

                RwD3D9SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

                RwRenderStateGet(rwRENDERSTATESRCBLEND, (void *)&srcBlend);
                RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void *)&destBlend);

                _rwD3D9RenderStateSrcBlend(rwBLENDINVSRCALPHA);
                _rwD3D9RenderStateDestBlend(rwBLENDZERO);

                if (resEntryHeader->indexBuffer != NULL)
                {
                    RwD3D9DrawIndexedPrimitive((D3DPRIMITIVETYPE)resEntryHeader->primType,
                                            vbBumpMapOffset,
                                            0, instancedData->numVertices,
                                            instancedData->startIndex, instancedData->numPrimitives);
                }
                else
                {
                    RwD3D9DrawPrimitive((D3DPRIMITIVETYPE)resEntryHeader->primType,
                                        vbBumpMapOffset,
                                        instancedData->numPrimitives);
                }

                RwD3D9SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);

                /*
                 * Second pass
                 */
                _rwD3D9RenderStateSrcBlend(rwBLENDSRCALPHA);
                _rwD3D9RenderStateDestBlend(rwBLENDONE);

                RwD3D9GetRenderState(D3DRS_ZWRITEENABLE, (void *)&zWriteEnable);
                RwD3D9SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

                RwD3D9SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);

                if (resEntryHeader->indexBuffer != NULL)
                {
                    RwD3D9DrawIndexedPrimitive((D3DPRIMITIVETYPE)resEntryHeader->primType,
                                            vbBumpMapOffset,
                                            0, instancedData->numVertices,
                                            instancedData->startIndex, instancedData->numPrimitives);
                }
                else
                {
                    RwD3D9DrawPrimitive((D3DPRIMITIVETYPE)resEntryHeader->primType,
                                        vbBumpMapOffset,
                                        instancedData->numPrimitives);
                }

                RwD3D9SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);

                RwD3D9SetRenderState(D3DRS_ZWRITEENABLE, zWriteEnable);

                /* Set standar blending mode */
                _rwD3D9RenderStateVertexAlphaEnable(FALSE);

                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)srcBlend);
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)destBlend);
            }

            /*
            * Set the stream source
            */
            _rwD3D9SetStreams(resEntryHeader->vertexStream,
                              resEntryHeader->useOffsets);

            /*
            * Vertex Declaration
            */
            RwD3D9SetVertexDeclaration(resEntryHeader->vertexDeclaration);

            if (envMapTexture && !BumpEnvMapPixelShader)
            {
                _rpMatFXD3D9AtomicMatFXEnvRender(resEntryHeader,
                                                 instancedData,
                                                 flags,
                                                 rpTHIRDPASS,
                                                 baseTexture,
                                                 envMapTexture);
            }
        }
    }
    else
    {
        _rpMatFXD3D9AtomicMatFXDefaultRender(resEntryHeader, instancedData, flags, baseTexture);
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rwD3D9AtomicMatFXRenderCallback

 Purpose:

 On entry:

 On exit:
 */
void
_rwD3D9AtomicMatFXRenderCallback(RwResEntry *repEntry,
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
    const MatFXDualData         *dualData;

    RWFUNCTION(RWSTRING("_rwD3D9AtomicMatFXRenderCallback"));

    /* Set clipping */
    _rwD3D9EnableClippingIfNeeded(object, type);

    /* Get lighting state */
    RwD3D9GetRenderState(D3DRS_LIGHTING, &lighting);

    if (lighting || (flags & rxGEOMETRY_PRELIT) != 0)
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

    /* Get the instanced data */
    resEntryHeader = (RxD3D9ResEntryHeader *)(repEntry + 1);
    instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

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

    /*
     * Set Indices
     */
    if (resEntryHeader->indexBuffer != NULL)
    {
        RwD3D9SetIndices(resEntryHeader->indexBuffer);
    }

    /*
     * Set the stream source
     */
    _rwD3D9SetStreams(resEntryHeader->vertexStream,
                      resEntryHeader->useOffsets);

    /*
    * Vertex Declaration
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
            if (lighting)
            {
                RwD3D9SetSurfaceProperties(&(instancedData->material->surfaceProps),
                                           &(instancedData->material->color),
                                           flags);
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
                    bumpmap = MATFXD3D9BUMPMAPGETDATA(instancedData->material);

                    _rpMatFXD3D9AtomicMatFXBumpMapRender(resEntryHeader,
                                                         instancedData,
                                                         flags,
                                                         instancedData->material->texture,
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
                                                     instancedData->material->texture,
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
                                                         instancedData->material->texture,
                                                         bumpmap->texture,
                                                         envMapData->texture);
                }
                break;

                case rpMATFXEFFECTDUAL:
                {
                    dualData = MATFXD3D9DUALGETDATA(instancedData->material);

                    _rpMatFXD3D9AtomicMatFXDualPassRender(resEntryHeader,
                                                          instancedData,
                                                          flags,
                                                          instancedData->material->texture,
                                                          dualData->texture);
                }
                break;

                case rpMATFXEFFECTDUALUVTRANSFORM:
                {
                    dualData = MATFXD3D9DUALUVANIMGETDUALDATA(instancedData->material);

                    _rpMatFXD3D9AtomicMatFXDualPassRender(resEntryHeader,
                                                          instancedData,
                                                          flags,
                                                          instancedData->material->texture,
                                                          dualData->texture);
                }
                break;

                case rpMATFXEFFECTUVTRANSFORM:
                default:
                    _rpMatFXD3D9AtomicMatFXDefaultRender(resEntryHeader,
                                                        instancedData,
                                                        flags,
                                                        instancedData->material->texture);
                    break;
            }
        }
        else
        {
            _rpMatFXD3D9AtomicMatFXRenderBlack(resEntryHeader, instancedData);
        }

        /* Move onto the next instancedData */
        ++instancedData;
    }

	//@{ 20050513 DDonSS : Threadsafe
	// ResEntry Unlock
	CS_RESENTRYHEADER_UNLOCK( resEntryHeader );
	//@} DDonSS

    RWRETURNVOID();
}

/****************************************************************************
 _rwD3D9MaterialMatFXHasBumpMap
 Purpose:
 On entry:
 On exit:   TRUE if the material has bumpmap info
 */
RwBool
_rwD3D9MaterialMatFXHasBumpMap(const RpMaterial *material)
{
    const rpMatFXMaterialData   *matFXData;

    RWFUNCTION(RWSTRING("_rwD3D9MaterialMatFXHasBumpMap"));

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
            instanceNode = RxNodeDefinitionGetD3D9AtomicAllInOne();

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
            instanceNode = RxNodeDefinitionGetD3D9WorldSectorAllInOne();

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
    const D3DCAPS9      *d3d9Caps;
    RxNodeDefinition    *instanceNode;
    RxPipelineNode      *node;

    RWFUNCTION(RWSTRING("_rpMatFXPipelinesCreate"));

    MatFXAtomicPipe = AtomicMatFxPipelineCreate();
    RWASSERT(NULL != MatFXAtomicPipe);

    /*
     * Get the instance node definition
     */
    instanceNode = RxNodeDefinitionGetD3D9AtomicAllInOne();
    RWASSERT(NULL != instanceNode);

    /*
     * Set the pipeline specific data
     */
    node = RxPipelineFindNodeByName(MatFXAtomicPipe, instanceNode->name, NULL, NULL);
    RWASSERT(NULL != node);

    D3D9DefaultInstanceCallbackAtomic = RxD3D9AllInOneGetInstanceCallBack(node);

    RxD3D9AllInOneSetInstanceCallBack(node, D3D9AtomicMatFXInstanceCallback);

    RxD3D9AllInOneSetRenderCallBack(node, _rwD3D9AtomicMatFXRenderCallback);

    /*
     * And likewise for world sectors:
     */
    MatFXWorldSectorPipe = WorldSectorMatFxPipelineCreate();
    RWASSERT(NULL != MatFXWorldSectorPipe);

    instanceNode = RxNodeDefinitionGetD3D9WorldSectorAllInOne();
    RWASSERT(NULL != instanceNode);

    node = RxPipelineFindNodeByName(MatFXWorldSectorPipe, instanceNode->name, NULL, NULL);
    RWASSERT(NULL != node);

    D3D9DefaultInstanceCallbackWorldSector = RxD3D9AllInOneGetInstanceCallBack(node);

    RxD3D9AllInOneSetInstanceCallBack(node, D3D9WorldSectorMatFXInstanceCallback);

    RxD3D9AllInOneSetRenderCallBack(node, _rwD3D9AtomicMatFXRenderCallback);

    /*
     * Get some video card capabilities
     */
    d3d9Caps = (const D3DCAPS9 *)RwD3D9GetCaps();

    VideoCardMaxTextureBlendStages = d3d9Caps->MaxTextureBlendStages;

    if (VideoCardMaxTextureBlendStages >= 2)
    {
        VideoCardSupportsMultitexture = (d3d9Caps->MaxSimultaneousTextures >= 2);
        VideoCardSupportsMultiplyAdd = (d3d9Caps->TextureOpCaps & D3DTEXOPCAPS_MULTIPLYADD);
        VideoCardSupportsModulate2X = (d3d9Caps->TextureOpCaps & D3DTEXOPCAPS_MODULATE2X);
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
    if ((d3d9Caps->PixelShaderVersion & 0xffff) >= 0x0101)
    {
        VideoCardSupportsTangents = TRUE;

        RwD3D9CreatePixelShader((const RwUInt32 *)dwBumpMapPixelShader,
                                &BumpMapPixelShader);

        RwD3D9CreatePixelShader((const RwUInt32 *)dwEnvMapPixelShader,
                                &EnvMapPixelShader);

        RwD3D9CreatePixelShader((const RwUInt32 *)dwEnvMapNoBaseTexturePixelShader,
                                &EnvMapNoBaseTexturePixelShader);

        RwD3D9CreatePixelShader((const RwUInt32 *)dwBumpEnvMapPixelShader,
                                &BumpEnvMapPixelShader);

        RwD3D9CreatePixelShader((const RwUInt32 *)dwBumpModulatedEnvMapPixelShader,
                                &BumpModulatedEnvMapPixelShader);

        RwD3D9CreatePixelShader((const RwUInt32 *)dwBumpModulatedEnvMapNoAlphaPixelShader,
                                &BumpModulatedEnvMapNoAlphaPixelShader);
    }
    else
    {
        VideoCardSupportsTangents = FALSE;

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
        RwD3D9DeletePixelShader(BumpMapPixelShader);
        BumpMapPixelShader = 0;
    }

    if (EnvMapPixelShader)
    {
        RwD3D9DeletePixelShader(EnvMapPixelShader);
        EnvMapPixelShader = 0;
    }

    if (EnvMapNoBaseTexturePixelShader)
    {
        RwD3D9DeletePixelShader(EnvMapNoBaseTexturePixelShader);
        EnvMapNoBaseTexturePixelShader = 0;
    }

    if (BumpEnvMapPixelShader)
    {
        RwD3D9DeletePixelShader(BumpEnvMapPixelShader);
        BumpEnvMapPixelShader = 0;
    }

    if (BumpModulatedEnvMapPixelShader)
    {
        RwD3D9DeletePixelShader(BumpModulatedEnvMapPixelShader);
        BumpModulatedEnvMapPixelShader = 0;
    }

    if (BumpModulatedEnvMapNoAlphaPixelShader)
    {
        RwD3D9DeletePixelShader(BumpModulatedEnvMapNoAlphaPixelShader);
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
            if (RwD3D9RasterIsCompressed(RwTextureGetRaster(baseTexture)))
            {
                usingCompressed = TRUE;
            }
            else
            {
                RWASSERT(RwTextureGetRaster(effectTexture) != NULL);
                if (RwD3D9RasterIsCompressed(RwTextureGetRaster(effectTexture)))
                {
                    usingCompressed = TRUE;
                }
            }
        }
        else
        {
            RWASSERT(RwTextureGetRaster(effectTexture) != NULL);
            if (RwD3D9RasterIsCompressed(RwTextureGetRaster(effectTexture)))
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
 * \ingroup rpmatfxd3d9
 * \ref RpMatFXGetD3D9Pipeline
 *
 * Returns one of the \ref rpmatfx internal D3D9 specific pipelines.
 *
 * The \ref rpmatfx plugin must be attached before using this function.
 *
 * \param d3d9Pipeline Type of the requested pipeline.
 *
 * \return The \ref RxPipeline requested from the plugin, or
 * NULL if the pipeline wasn't constructed.
 */
RxPipeline *
RpMatFXGetD3D9Pipeline( RpMatFXD3D9Pipeline d3d9Pipeline )
{
    RxPipeline *pipeline;

    RWAPIFUNCTION(RWSTRING("RpMatFXGetD3D9Pipeline"));
    RWASSERT(0 < MatFXInfo.Module.numInstances);
    RWASSERT(rpNAMATFXD3D9PIPELINE < d3d9Pipeline);
    RWASSERT(rpMATFXD3D9PIPELINEMAX > d3d9Pipeline);

    switch(d3d9Pipeline)
    {
        case rpMATFXD3D9ATOMICPIPELINE:
            pipeline = MatFXAtomicPipe;
            break;
        case rpMATFXD3D9WORLDSECTORPIPELINE:
            pipeline = MatFXWorldSectorPipe;
            break;
        default:
            pipeline = (RxPipeline *)NULL;
            break;
    }

    RWRETURN(pipeline);
}

/****************************************************************************
 _rpMatFXD3D9VertexShaderAtomicEnvRender
 */
void
_rpMatFXD3D9VertexShaderAtomicEnvRender(RxD3D9ResEntryHeader *resEntryHeader,
                                        RxD3D9InstanceData *instancedData,
                                        RwTexture *baseTexture,
                                        RwTexture *envMapTexture,
                                        const _rpD3D9VertexShaderDescriptor  *desc)
{
    const MatFXEnvMapData *envMapData;
    RwUInt32    shiney;
    RwBool      useEnvMapPixelShader = FALSE;
    RwBool      useMultitexture = FALSE;
    RwBool      hasBaseTexture = FALSE;
	//@{ Jaewon 20041220
	RwBool		alphaBlend;
	//@} Jaewon

    RWFUNCTION(RWSTRING("_rpMatFXD3D9VertexShaderAtomicEnvRender"));

    envMapData = MATFXD3D9ENVMAPGETDATA(instancedData->material, rpSECONDPASS);

    shiney = (RwFastRealToUInt32(envMapData->coef * 255) & 0xFF);

    /* Set the base texture */
    if (desc->numTexCoords &&
        baseTexture)
    {
        RwD3D9SetTexture(baseTexture, 0);

        hasBaseTexture = TRUE;
    }
    else
    {
        RwD3D9SetTexture(NULL, 0);
    }

    /* Check palettized textures */
    if (hasBaseTexture &&
        (RwRasterGetFormat(RwTextureGetRaster(baseTexture)) & rwRASTERFORMATPAL8) != 0 &&
        (RwRasterGetFormat(RwTextureGetRaster(envMapTexture)) & rwRASTERFORMATPAL8) != 0 &&
        baseTexture != envMapTexture)
    {
        _rwD3D9RasterConvertToNonPalettized(envMapTexture->raster);
    }

    /* Choose code path */
	//@{ Jaewon 20041220
	// to do alpha-blending in an env-map fx
    //if ( envMapData->useFrameBufferAlpha ||
    //        !( instancedData->vertexAlpha ||
    //        0xFF != instancedData->material->color.alpha ||
    //        (hasBaseTexture && _rwD3D9TextureHasAlpha(baseTexture)) ) )
	//@} Jaewon
    {
        if (_rwD3D9TextureHasAlpha(envMapTexture))
        {
            if (EnvMapPixelShader)
            {
                useEnvMapPixelShader = TRUE;
            }
        }
        else
        {
            useMultitexture = TRUE;
        }
    }

    /*
     * Set the default Pixel shader
     */
    if (useEnvMapPixelShader == FALSE)
    {
        RwD3D9SetPixelShader(NULL);

        if (hasBaseTexture)
        {
            RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        }
        else
        {
            RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        }
    }

    /*
     * Vertex shader
     */
    RwD3D9SetVertexShader(instancedData->vertexShader);

    /*
     * Draw the geometry if dual pass
     */
    if (!useEnvMapPixelShader && !useMultitexture)
    {
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

        RwD3D9SetTexture(envMapTexture, 1);

        /* Set pixel shader */
        RwD3D9SetPixelShader(EnvMapPixelShader);

        RwD3D9SetPixelShaderConstant(0, ShinyFloats, 1);

		//@{ Jaewon 20041220
		// to do alpha-blending in an env-map fx
		RwD3D9GetRenderState(D3DRS_ALPHABLENDENABLE, &alphaBlend);
		if ( instancedData->vertexAlpha ||
		        0xFF != instancedData->material->color.alpha ||
		        (hasBaseTexture && _rwD3D9TextureHasAlpha(baseTexture)) )
		{
		    RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		}
		//@} Jaewon

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

		//@{ Jaewon 20041220
		// to do alpha-blending in an env-map fx
		RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, alphaBlend);
		//@} Jaewon

        RwD3D9SetTexture(NULL, 1);
    }
    else if (useMultitexture)
    {
        if (desc->numTexCoords)
        {
            RwD3D9SetTexture(envMapTexture, 1);

            if (shiney != 0xFF)
            {
                /* Set the shiney factor and the correct texture stages */
                shiney = ((shiney << 24) |
                            (shiney << 16) |
                            (shiney << 8) |
                            shiney);

                RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, shiney);

                RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MULTIPLYADD);
                RwD3D9SetTextureStageState(1, D3DTSS_COLORARG0, D3DTA_CURRENT);
                RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_TFACTOR);
            }
            else
            {
                RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_ADD);
                RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
            }

            RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
            RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        }
        else
        {
            RwD3D9SetTexture(envMapTexture, 0);

            if (shiney != 0xFF)
            {
                /* Set the shiney factor and the correct texture stages */
                shiney = ((shiney << 24) |
                            (shiney << 16) |
                            (shiney << 8) |
                            shiney);

                RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, shiney);

                RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MULTIPLYADD);
                RwD3D9SetTextureStageState(0, D3DTSS_COLORARG0, D3DTA_DIFFUSE);
                RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
            }
            else
            {
                RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_ADD);
                RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            }
        }

		//@{ Jaewon 20041220
		// to do alpha-blending in an env-map fx
		RwD3D9GetRenderState(D3DRS_ALPHABLENDENABLE, &alphaBlend);
		if ( instancedData->vertexAlpha ||
		        0xFF != instancedData->material->color.alpha ||
		        (hasBaseTexture && _rwD3D9TextureHasAlpha(baseTexture)) )
		{
		    RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		}
		//@} Jaewon

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

		//@{ Jaewon 20041220
		// to do alpha-blending in an env-map fx
		RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, alphaBlend);
		//@} Jaewon

        if (desc->numTexCoords)
        {
            RwD3D9SetTexture(NULL, 1);

            RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
            RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
        }
    }
    else
    {
        RwBlendFunction srcBlend, destBlend;
        RwBool      lighting;
        RwBool      zWriteEnable;
        RwBool      fogEnabled;
        RwUInt32    fogColor;
        _rpD3D9VertexShaderDescriptor  desc2;
        _rpD3D9VertexShaderDispatchDescriptor dispatch;

        desc2 = *desc;
        desc2.numTexCoords = 0;
        desc2.effect = rwD3D9VERTEXSHADEREFFECT_ENVMAP;

        instancedData->vertexShader = _rpD3D9GetVertexShader(&desc2, &dispatch);

        RwD3D9SetVertexShader(instancedData->vertexShader);

        if (!_rwD3D9RenderStateIsVertexAlphaEnable())
        {
            _rwD3D9RenderStateVertexAlphaEnable(TRUE);
        }

        RwD3D9SetTexture(envMapTexture, 0);

        /* Set needed blending modes for envmap */
        RwRenderStateGet(rwRENDERSTATESRCBLEND, (void *)&srcBlend);
        RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void *)&destBlend);

        _rwD3D9RenderStateSrcBlend(rwBLENDSRCALPHA);
        _rwD3D9RenderStateDestBlend(rwBLENDONE);

        /* Set the shinnyness */
        shiney = ((shiney << 24) |
                    (shiney << 16) |
                    (shiney << 8) |
                    shiney);

        if (shiney < 0xFFFFFFFF)
        {
            RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, shiney);

            RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
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

        RwD3D9GetRenderState(D3DRS_LIGHTING, (void *)&lighting);
        RwD3D9GetRenderState(D3DRS_ZWRITEENABLE, (void *)&zWriteEnable);
        RwD3D9GetRenderState(D3DRS_FOGENABLE, (void *)&fogEnabled);

        RwD3D9SetRenderState(D3DRS_LIGHTING, FALSE);
        RwD3D9SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

        if (fogEnabled)
        {
            RwD3D9GetRenderState(D3DRS_FOGCOLOR, (void *)&fogColor);
            RwD3D9SetRenderState(D3DRS_FOGCOLOR, 0);
        }

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
        RwD3D9SetRenderState(D3DRS_LIGHTING, lighting);

        if (shiney < 0xFFFFFFFF)
        {
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        }
        else
        {
            RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
        }

        _rwD3D9RenderStateVertexAlphaEnable(FALSE);

        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)srcBlend);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)destBlend);

        RwD3D9SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
        RwD3D9SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_PASSTHRU);
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpMatFXD3D9VertexShaderAtomicDualRender

 Purpose:

 On entry:

 On exit:
 */
void
_rpMatFXD3D9VertexShaderAtomicDualRender(RxD3D9ResEntryHeader *resEntryHeader,
                                         RxD3D9InstanceData *instancedData,
                                         RwTexture *baseTexture,
                                         RwTexture *dualPassTexture,
                                         const _rpD3D9VertexShaderDescriptor  *desc,
                                         const _rpD3D9VertexShaderDispatchDescriptor *dispatch)
{
    const MatFXDualData *dualData;
    const MatFXUVAnimData *uvAnim = NULL;
    RwBlendFunction     srcBlend, destBlend;
    RwBool              zWriteEnable;
    RwUInt32            fogColor;
    _rpD3D9VertexShaderDescriptor  desc2;
    _rpD3D9VertexShaderDispatchDescriptor dispatch2;
    RwBool useDualPass = TRUE;

    RWFUNCTION(RWSTRING("_rpMatFXD3D9VertexShaderAtomicDualRender"));

    if ((*MATFXMATERIALGETCONSTDATA(instancedData->material))->flags !=
        rpMATFXEFFECTDUALUVTRANSFORM)
    {
        dualData = MATFXD3D9DUALGETDATA(instancedData->material);
    }
    else
    {
        dualData = MATFXD3D9DUALUVANIMGETDUALDATA(instancedData->material);

        uvAnim = MATFXD3D9UVANIMGETDATA(instancedData->material);
    }

    if (uvAnim != NULL)
    {
        _rpD3DVertexShaderSetUVAnimMatrix(uvAnim->baseTransform, dispatch);
    }

    if (desc->numTexCoords)
    {
        RwD3D9SetTexture(baseTexture, 0);

        RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    }
    else
    {
        RwD3D9SetTexture(NULL, 0);

        RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    }

    /* Try to use multitexturing */
    if (uvAnim == NULL)
    {
        if ( (dualData->srcBlendMode == rwBLENDDESTCOLOR &&
                dualData->dstBlendMode == rwBLENDZERO) ||
                (dualData->srcBlendMode == rwBLENDZERO &&
                dualData->dstBlendMode == rwBLENDSRCCOLOR))
        {
            if (baseTexture != NULL)
            {
                RwD3D9SetTexture(dualPassTexture, 1);

				//@{ Jaewon 20041125
                RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   TextureOpModulation);
				//@} Jaewon
                RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
            }
            else
            {
                RwD3D9SetTexture(dualPassTexture, 0);

				//@{ Jaewon 20041125
                RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   TextureOpModulation);
				//@} Jaewon
                RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
            }

            useDualPass = FALSE;
        }
        else if ( dualData->srcBlendMode == rwBLENDSRCALPHA &&
                    dualData->dstBlendMode == rwBLENDINVSRCALPHA )
        {
            if (baseTexture != NULL)
            {
                RwD3D9SetTexture(dualPassTexture, 1);

                RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
                RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

                RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_BLENDTEXTUREALPHA);
                RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_BLENDTEXTUREALPHA);
                RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
                RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

				//@{ Jaewon 20041125
                RwD3D9SetTextureStageState(2, D3DTSS_COLOROP,   TextureOpModulation);
				//@} Jaewon
                RwD3D9SetTextureStageState(2, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
                RwD3D9SetTextureStageState(2, D3DTSS_COLORARG2, D3DTA_CURRENT);

                RwD3D9SetTextureStageState(2, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
                RwD3D9SetTextureStageState(2, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
                RwD3D9SetTextureStageState(2, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
            }
            else
            {
                RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, 0xffffffff);

                RwD3D9SetTexture(dualPassTexture, 0);

                RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_BLENDTEXTUREALPHA);
                RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

                RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_BLENDTEXTUREALPHA);
                RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

				//@{ Jaewon 20041125
                RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   TextureOpModulation);
				//@} Jaewon
                RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
                RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
                RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
                RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
            }

            useDualPass = FALSE;
        }
        else if ( dualData->srcBlendMode == rwBLENDDESTCOLOR &&
                    dualData->dstBlendMode == rwBLENDSRCCOLOR)
        {
            if (baseTexture != NULL)
            {
                RwD3D9SetTexture(dualPassTexture, 1);

                RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE2X);
                RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
            }
            else
            {
                RwD3D9SetTexture(dualPassTexture, 0);

                RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE2X);

                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
            }

            useDualPass = FALSE;
        }
        else if ( dualData->srcBlendMode == rwBLENDONE &&
                    dualData->dstBlendMode == rwBLENDONE )
        {
            if (baseTexture != NULL)
            {
                RwD3D9SetTexture(dualPassTexture, 1);

                RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MULTIPLYADD);
                RwD3D9SetTextureStageState(1, D3DTSS_COLORARG0, D3DTA_CURRENT);
                RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

                RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
            }
            else
            {
                RwD3D9SetTexture(dualPassTexture, 0);

                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

                RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_ADD);
                RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
                RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
            }

            useDualPass = FALSE;
        }
        else if ( dualData->srcBlendMode == rwBLENDZERO &&
                    dualData->dstBlendMode == rwBLENDSRCALPHA )
        {
            if (baseTexture != NULL)
            {
                RwD3D9SetTexture(dualPassTexture, 1);

				//@{ Jaewon 20041125
                RwD3D9SetTextureStageState(1, D3DTSS_COLOROP,   TextureOpModulation);
				//@} Jaewon
                RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE | D3DTA_ALPHAREPLICATE);
                RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

                RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
            }
            else
            {
                RwD3D9SetTexture(dualPassTexture, 0);

				//@{ Jaewon 20041125
                RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   TextureOpModulation);
				//@} Jaewon
                RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE | D3DTA_ALPHAREPLICATE);
                RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
                RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
            }

            useDualPass = FALSE;
        }

        if (useDualPass == FALSE)
        {
            /* Check palettized textures */
            if (baseTexture &&
                (RwRasterGetFormat(RwTextureGetRaster(baseTexture)) & rwRASTERFORMATPAL8) != 0 &&
                (RwRasterGetFormat(RwTextureGetRaster(dualPassTexture)) & rwRASTERFORMATPAL8) != 0 &&
                baseTexture != dualPassTexture)
            {
                _rwD3D9RasterConvertToNonPalettized(dualPassTexture->raster);
            }

            /* Check shader to use */
            if (desc->numTexCoords == 1)
            {
                desc2 = *desc;

                desc2.effect = rwD3D9VERTEXSHADEREFFECT_DUALREPLICATE;

                instancedData->vertexShader = _rpD3D9GetVertexShader(&desc2, &dispatch2);
            }
        }
    }

    /*
     * Set the default Pixel shader
     */
    RwD3D9SetPixelShader(NULL);

    /*
     * Vertex shader
     */
    RwD3D9SetVertexShader(instancedData->vertexShader);

    /*
     * Draw the geometry
     */
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
     * SECOND PASS
     */
    if (useDualPass)
    {
        RwD3D9SetTexture(dualPassTexture, 0);

        desc2 = *desc;
        if (uvAnim != NULL)
        {
            if (desc->numTexCoords > 1)
            {
                desc2.effect = rwD3D9VERTEXSHADEREFFECT_DUALUVANIM;
            }
            else
            {
                desc2.effect = rwD3D9VERTEXSHADEREFFECT_UVANIM;
            }
        }
        else
        {
            if (desc->numTexCoords > 1)
            {
                desc2.effect = rwD3D9VERTEXSHADEREFFECT_DUAL;
            }
            else
            {
                desc2.effect = 0;
            }
        }

        instancedData->vertexShader = _rpD3D9GetVertexShader(&desc2, &dispatch2);

        /*
        * Vertex shader
        */
        RwD3D9SetVertexShader(instancedData->vertexShader);

        if (uvAnim != NULL)
        {
            _rpD3DVertexShaderSetUVAnimMatrix(uvAnim->dualTransform, &dispatch2);
        }

        if (!_rwD3D9RenderStateIsVertexAlphaEnable())
        {
            _rwD3D9RenderStateVertexAlphaEnable(TRUE);
        }

        /* Remove alpha test for some combos */
        if ( (dualData->srcBlendMode == rwBLENDDESTCOLOR && dualData->dstBlendMode == rwBLENDZERO) ||
                (dualData->srcBlendMode == rwBLENDZERO && dualData->dstBlendMode == rwBLENDSRCCOLOR) ||
                (dualData->srcBlendMode == rwBLENDDESTCOLOR && dualData->dstBlendMode == rwBLENDSRCCOLOR) )
        {
            RwD3D9SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        }

        /*
        * Set appropiate blending mode
        */
        RwRenderStateGet(rwRENDERSTATESRCBLEND, (void *)&srcBlend);
        RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void *)&destBlend);

        _rwD3D9RenderStateSrcBlend(dualData->srcBlendMode);
        _rwD3D9RenderStateDestBlend(dualData->dstBlendMode);

        RwD3D9GetRenderState(D3DRS_ZWRITEENABLE, (void *)&zWriteEnable);
        RwD3D9SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

        if (desc->fogMode)
        {
            RwD3D9GetRenderState(D3DRS_FOGCOLOR, (void *)&fogColor);

            if (dualData->dstBlendMode == rwBLENDONE)
            {
                RwD3D9SetRenderState(D3DRS_FOGCOLOR, 0);
            }
            else if ( dualData->srcBlendMode == rwBLENDDESTCOLOR ||
                        dualData->dstBlendMode == rwBLENDSRCCOLOR )
            {
                RwD3D9SetRenderState(D3DRS_FOGCOLOR, 0xffffffff);
            }
        }

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

        if (desc->fogMode)
        {
            RwD3D9SetRenderState(D3DRS_FOGCOLOR, fogColor);
        }

        _rwD3D9RenderStateVertexAlphaEnable(FALSE);

        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)srcBlend);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)destBlend);
    }
    else
    {
        RwD3D9SetTexture(NULL, 1);

        RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

        RwD3D9SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
        RwD3D9SetTextureStageState(2, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpMatFXD3D9VertexShaderAtomicBumpMapRender

 Purpose:

 On entry:

 On exit:
 */
void
_rpMatFXD3D9VertexShaderAtomicBumpMapRender(RxD3D9ResEntryHeader *resEntryHeader,
                                            RxD3D9InstanceData *instancedData,
                                            RwTexture *baseTexture,
                                            RwTexture *bumpTexture,
                                            RwTexture *envMapTexture,
                                            const _rpD3D9VertexShaderDescriptor  *desc,
                                            const _rpD3D9VertexShaderDispatchDescriptor *dispatch)
{
    RwBool alphaBlend;
    RwBool alphaTest;

    RWFUNCTION(RWSTRING("_rpMatFXD3D9VertexShaderAtomicBumpMapRender"));

    /*
     * Set base textures
     */
    RwD3D9SetTexture(bumpTexture, 0);
    RwD3D9SetTexture(bumpTexture, 1);

    /*
     * Vertex shader
     */
    RwD3D9SetVertexShader(instancedData->vertexShader);

    /*
     * Draw effect
     */
    RWASSERT((envMapTexture && BumpEnvMapPixelShader) || BumpMapPixelShader);

    _rwD3D9RenderStateVertexAlphaEnable(FALSE);

    /* We need to force it due to alpha texture */
    RwD3D9GetRenderState(D3DRS_ALPHABLENDENABLE, &alphaBlend);
    RwD3D9GetRenderState(D3DRS_ALPHATESTENABLE, &alphaTest);

    RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    RwD3D9SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

    /*
     * Pixel shader
     */
    if (envMapTexture && BumpEnvMapPixelShader)
    {
        const MatFXEnvMapData *envMapData = MATFXD3D9ENVMAPGETDATA(instancedData->material, rpTHIRDPASS);

        const RwUInt32  shiney = (RwFastRealToUInt32(envMapData->coef * 255) & 0xFF);

        if (shiney)
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

            RwD3D9SetTexture(envMapTexture, 2);

            /* Set pixel shader */
            if (envMapData->useFrameBufferAlpha)
            {
                if (_rwD3D9TextureHasAlpha(envMapTexture))
                {
                    RwD3D9SetPixelShader(BumpModulatedEnvMapPixelShader);
                }
                else
                {
                    RwD3D9SetPixelShader(BumpModulatedEnvMapNoAlphaPixelShader);
                }
            }
            else
            {
                RwD3D9SetPixelShader(BumpEnvMapPixelShader);
            }

            RwD3D9SetPixelShaderConstant(0, ShinyFloats, 1);
        }
        else
        {
            RwD3D9SetPixelShader(BumpMapPixelShader);
        }
    }
    else
    {
        RwD3D9SetPixelShader(BumpMapPixelShader);
    }

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

    /*
     * Restore default values
     */
    RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, alphaBlend);
    RwD3D9SetRenderState(D3DRS_ALPHATESTENABLE, alphaTest);

    RwD3D9SetTexture(NULL, 1);
    RwD3D9SetTexture(NULL, 2);

    RWRETURNVOID();
}
