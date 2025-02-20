/****************************************************************************
 *
 * This file is a product of Criterion Software Ltd.
 *
 * This file is provided as is with no warranties of any kind and is
 * provided without any obligation on Criterion Software Ltd.
 * or Canon Inc. to assist in its use or modification.
 *
 * Criterion Software Ltd. and Canon Inc. will not, under any
 * circumstances, be liable for any lost revenue or other damages
 * arising from the use of this file.
 *
 * Copyright (c) 2001 Criterion Software Ltd.
 * All Rights Reserved.
 *
 */
/****************************************************************************
 *
 * ptankd3d8.c
 *
 * Copyright (C) 2001 Criterion Technologies.
 *
 *
 * Purpose: D3D8 Billboard implementation.
 *
 ****************************************************************************/

/**
 * \ingroup rpptankd3d8
 *
 * \page rpptankd3d8overview D3D8 RpPTank Overview
 *
 * The D3D8 optimized verion of RpPTank uses point sprites when the hardware supports them.
 * Please note that point sprites severely limits the capabilities of the billboards.
 * Due to hardware limitations, only the following flags are respected:
 *
 * \li rpPTANKDFLAGPOSITION
 * \li rpPTANKDFLAGNORMAL
 * \li rpPTANKDFLAGSIZE (if the hardware supports the D3DFVF_PSIZE vertex format flag)
 * \li rpPTANKDFLAGCOLOR
 * \li rpPTANKDFLAGUSECENTER
 * \li rpPTANKDFLAGARRAY
 * \li rpPTANKDFLAGSTRUCTURE
 *
 * All the other flags are ignored. You can also specify a constant color or a constant
 * size for all the billboards.
 *
 * \note As the D3D8 point sprites are always square, only the x size of the constant size
 * will be taken into account.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <d3d8.h>

#include "rwcore.h"
#include "rpworld.h"
#include "rpplugin.h"
#include "rpdbgerr.h"
#include "ptank.h"
#include "ptankprv.h"
#include "ptankg.h"

#include "ptankd3d8.h"
#include "ptankplatform.h"

#define ALIGNPAD 16

#define FLOATASINT(f) (*((const RwInt32 *)&(f)))

#define VALIDPOINTSPRITESFLAGS  (rpPTANKDFLAGPOSITION |         \
                                 rpPTANKDFLAGNORMAL |           \
                                 rpPTANKDFLAGCOLOR |            \
                                 rpPTANKDFLAGUSECENTER |        \
                                 rpPTANKDFLAGCNSVTX2TEXCOORDS | \
                                 rpPTANKDFLAGARRAY |            \
                                 rpPTANKDFLAGSTRUCTURE)

#define VALIDPOINTSPRITESSIZEFLAGS  (rpPTANKDFLAGPOSITION |         \
                                     rpPTANKDFLAGNORMAL |           \
                                     rpPTANKDFLAGCOLOR |            \
                                     rpPTANKDFLAGSIZE |             \
                                     rpPTANKDFLAGUSECENTER |        \
                                     rpPTANKDFLAGCNSVTX2TEXCOORDS | \
                                     rpPTANKDFLAGARRAY |            \
                                     rpPTANKDFLAGSTRUCTURE)

#define RGBATOD3D8COLOR(col)    ( (((RwUInt32)((col)->alpha)) << 24) |  \
                                 (((RwUInt32)((col)->red)) << 16) |     \
                                 (((RwUInt32)((col)->green)) << 8) |    \
                                 ((RwUInt32)((col)->blue)) )


#if (0)

static RwBool
rpPTankD3D8Instance(RpAtomic *atomic,
                    RpPTankData *ptankGlobal,
                    RwInt32 actPCount,
                    RwUInt32 instFlags);

static void
rpPTankD3D8InsPosNCCSPR(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);

static void
rpPTankD3D8InsPosNCPSPR(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);

static void
rpPTankD3D8InsPosCCCSPR(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);

#endif /* (0) */

static RwBool
rpPTankD3D8Create(RpAtomic *atomic,
                  RpPTankData *ptankGlobal,
                  RwInt32 maxPCount,
                  RwUInt32 dataFlags,
                  RwUInt32 platFlags);


static RwBool
rpPTankD3D8RenderCallBack(RpAtomic *atomic,
                          RpPTankData *ptankGlobal,
                          RwInt32 actPCount);
static void
rpPTankD3D8InsPosNCCSNR_spr(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);
static void
rpPTankD3D8InsPosCCCSNR_spr(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);

static void
rpPTankD3D8InsPosNCPSNR_spr(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);
static void
rpPTankD3D8InsPosCCPSNR_spr(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);

static void
rpPTankD3D8InsNormals_spr(RpPTankLockStruct *dstCluster,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);

static void
rpPTankD3D8InsColor_spr(RpPTankLockStruct *dstCluster,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);


RpPTankCallBacks defaultCB =
{
    NULL,                       /* Alloc, using default one */
    rpPTankD3D8Create,
    _rpPTankGENInstance,
    rpPTankD3D8RenderCallBack
};

/*
 * Global static variables
 */
static LPDIRECT3DINDEXBUFFER8 rpTankD3D8IndexBuffer = NULL;
static RwInt32 rpTankD3D8MaxIndex = 0;

static LPDIRECT3DVERTEXBUFFER8 rpTankD3D8DynamicVertexBuffer = NULL;
static RwUInt32 rpTankD3D8BaseIndex = 0;

static RwReal rpTankD3D8VideoCardMaxPointSize = 1.f;
static RwBool rpTankD3D8VideoCardSizePerParticle = FALSE;

RwBool rpTankD3D8UsePointSprites = FALSE;

/*
 *****************************************************************************
 */
static __inline void
SinCos(float angle, float *sine, float *cosine)
{
    RWFUNCTION(RWSTRING("SinCos"));

#if defined(NOASM)
    *sine = (RwReal)RwSin(angle);
    *cosine = (RwReal)RwCos(angle);
#else
    __asm
    {
        fld        dword ptr angle
        fsincos
        mov        eax, sine
        mov        edx, cosine
        fstp    dword ptr [edx]
        fstp    dword ptr [eax]
    }
#endif

    RWRETURNVOID();
}

void
rpPTankD3D8InsEnding(RpPTANKInstanceSetupData *data,
                RpAtomic *atomic,
                RpPTankData *ptankGlobal,
                RwInt32 actPCount,
                RwUInt32 instFlags)
{
    RWFUNCTION( RWSTRING( "rpPTankD3D8InsEnding" ) );

    RwD3D8DynamicVertexBufferUnlock(rpTankD3D8DynamicVertexBuffer);

    RWRETURNVOID();
}

void
rpPTankD3D8InsSetup(RpPTANKInstanceSetupData *data,
                RpAtomic *atomic,
                RpPTankData *ptankGlobal,
                RwInt32 actPCount,
                RwUInt32 instFlags)
{
    RwInt32         numVertices;
    RpGeometry      *geom;
    RpGeometryFlag  flags;
    RpMaterial      *material;
    RwV3d           *right = &data->right;
    RwV3d           *up = &data->up;
    RwUInt32        strideVertex;
    const RwMatrix  *camLTM;
    RwUInt8         *vertexData = NULL;
    RwUInt32        fvf;


    RWFUNCTION(RWSTRING("rpPTankD3D8InsSetup"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    numVertices = actPCount * 4;

    geom = RpAtomicGetGeometry(atomic);

    flags = (RpGeometryFlag)RpGeometryGetFlags(geom);

    material = RpGeometryGetMaterial(geom, 0);

    if (material->texture)
    {
    if ( (flags & rxGEOMETRY_TEXTURED) == 0)
    {
        RpGeometrySetFlags(geom, flags | rxGEOMETRY_TEXTURED);
    }
    }
    else
    {
        if ( (flags & rxGEOMETRY_TEXTURED) != 0)
        {
            RpGeometrySetFlags(geom, flags & ~rxGEOMETRY_TEXTURED);
        }
    }

    /* Get current camera matrix */
    camLTM = RwFrameGetLTM(RwCameraGetFrame((RwCamera *)RWSRCGLOBAL(curCamera)));

    /*
     * Calculate FVF and Stride
     */
    flags = (RpGeometryFlag)RpGeometryGetFlags(geom);

    /* What do we instance ?? */
    /* Well every thing as it's a DYNAMIC buffer */

    /* Positions */
    strideVertex = sizeof(RwV3d);
    fvf = D3DFVF_XYZ;
    data->instancePositions = TRUE;

    /* Normals */
    if (flags & rxGEOMETRY_NORMALS)
    {
        strideVertex += sizeof(RwV3d);
        fvf |= D3DFVF_NORMAL;

        data->instanceNormals = TRUE;
    }

    /* Pre-lighting */
    if (flags & rxGEOMETRY_PRELIT)
    {
        strideVertex += sizeof(RwRGBA);
        fvf |= D3DFVF_DIFFUSE;
        data->instanceColors = TRUE;
    }

    /* Texture coordinates */
    if (flags & rxGEOMETRY_TEXTURED)
    {
        strideVertex += sizeof(RwTexCoords);
        fvf |= D3DFVF_TEX1;
        data->instanceUVs = TRUE;
    }

    /* Request and lock the needed vertex buffer */
    /*Setting up right and up vectors */
    if((ptankGlobal->format.dataFlags & rpPTANKDFLAGCNSMATRIX )
                                            == rpPTANKDFLAGCNSMATRIX)
    {
        RwV3dAssign(right, RwMatrixGetRight(&ptankGlobal->cMatrix));
        RwV3dAssign(up, RwMatrixGetUp(&ptankGlobal->cMatrix));
    }
    else if((ptankGlobal->format.dataFlags & rpPTANKDFLAGCNS2DROTATE)
                                            == rpPTANKDFLAGCNS2DROTATE)
    {
        RwV3d   rightCamera;
        RwV3d   upCamera;
        RwReal  sinA, cosA;

        rightCamera.x = -camLTM->right.x;
        rightCamera.y = -camLTM->right.y;
        rightCamera.z = -camLTM->right.z;

        upCamera.x = camLTM->up.x;
        upCamera.y = camLTM->up.y;
        upCamera.z = camLTM->up.z;

        SinCos(ptankGlobal->cRotate, &sinA, &cosA);

        /* Calculate world space size vectors */
        right->x = (+ cosA) * rightCamera.x + (- sinA) * upCamera.x;
        right->y = (+ cosA) * rightCamera.y + (- sinA) * upCamera.y;
        right->z = (+ cosA) * rightCamera.z + (- sinA) * upCamera.z;

        up->x = (+ sinA) * rightCamera.x + (+ cosA) * upCamera.x;
        up->y = (+ sinA) * rightCamera.y + (+ cosA) * upCamera.y;
        up->z = (+ sinA) * rightCamera.z + (+ cosA) * upCamera.z;
    }
    else
    {
        right->x = -camLTM->right.x;
        right->y = -camLTM->right.y;
        right->z = -camLTM->right.z;

        up->x = camLTM->up.x;
        up->y = camLTM->up.y;
        up->z = camLTM->up.z;
    }

    if(RwD3D8DynamicVertexBufferLock(strideVertex,
                                      numVertices,
                                      (void **)&rpTankD3D8DynamicVertexBuffer,
                                      (void **)&vertexData,
                                      &rpTankD3D8BaseIndex))
    {
        RwUInt32 offset = 0;

        if( data->instancePositions )
        {
            data->positionOut.data = (RwUInt8 *)vertexData;
            data->positionOut.stride = strideVertex;
        }

        offset = sizeof(RwV3d);

        if( data->instanceNormals )
        {
            data->normalsOut.data = (RwUInt8 *)(vertexData + sizeof(RwV3d));
            data->normalsOut.stride =  strideVertex;
        }

        if(flags & rxGEOMETRY_NORMALS)
        {
            offset += sizeof(RwV3d);
        }

        if( data->instanceColors )
        {
            data->colorsOut.data = (RwUInt8 *)(vertexData + offset);
            data->colorsOut.stride = strideVertex;
        }

        if( flags & rxGEOMETRY_PRELIT)
        {
            offset += sizeof(RwRGBA);
        }

        if( data->instanceUVs )
        {
                data->UVOut.data = (RwUInt8 *)(vertexData + offset);
                data->UVOut.stride = strideVertex;
        }
    }

    rpTankD3D8UsePointSprites = FALSE;

    RWRETURNVOID();
}

void
rpPTankD3D8InsSetup_spr(RpPTANKInstanceSetupData *data,
                RpAtomic *atomic,
                RpPTankData *ptankGlobal,
                RwInt32 actPCount,
                RwUInt32 instFlags)
{
    RpGeometryFlag  flags;
    RwUInt32        strideVertex;
    RwInt32         numVertices;
    RwUInt8         *vertexData = NULL;
    RwUInt32        fvf;
    RpGeometry      *geom;
#if (0)
    RwV3d           *right = &data->right;
    RwV3d           *up = &data->up;
#endif /* (0) */
    RWFUNCTION(RWSTRING("rpPTankD3D8InsSetup_spr"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    geom = RpAtomicGetGeometry(atomic);
    numVertices = actPCount;

    flags = (RpGeometryFlag)RpGeometryGetFlags(geom);

    if ( (flags & rxGEOMETRY_TEXTURED) != 0)
    {
        RpGeometrySetFlags(geom, flags & ~rxGEOMETRY_TEXTURED);
    }

    /*
     * Calculate FVF and Stride
     */
    flags = (RpGeometryFlag)RpGeometryGetFlags(geom);

    /* Positions */
    strideVertex = sizeof(RwV3d);
    fvf = D3DFVF_XYZ;

    data->instancePositions = TRUE;

    /* Normals */
    if (flags & rxGEOMETRY_NORMALS)
    {
        strideVertex += sizeof(RwV3d);
        fvf |= D3DFVF_NORMAL;

        data->instanceNormals = TRUE;
    }

    /* Pre-lighting */
    if (flags & rxGEOMETRY_PRELIT)
    {
        strideVertex += sizeof(RwRGBA);
        fvf |= D3DFVF_DIFFUSE;

        data->instanceColors = TRUE;
    }

    if ((ptankGlobal->format.dataFlags & rpPTANKDFLAGSIZE) != 0)
    {
        strideVertex += sizeof(RwReal);
        fvf |= D3DFVF_PSIZE;
    }

    /* Request and lock the needed vertex buffer */
    if(RwD3D8DynamicVertexBufferLock(strideVertex,
                                      numVertices,
                                      (void **)&rpTankD3D8DynamicVertexBuffer,
                                      (void **)&vertexData,
                                      &rpTankD3D8BaseIndex))
    {
        RwUInt32 offset = 0;

        if( data->instancePositions )
        {
            data->positionOut.data = (RwUInt8 *)vertexData;
            data->positionOut.stride = strideVertex;
        }

        offset = sizeof(RwV3d);

        if( data->instanceNormals )
        {
            data->normalsOut.data = (RwUInt8 *)(vertexData + sizeof(RwV3d));
            data->normalsOut.stride =  strideVertex;
        }

        if(flags & rxGEOMETRY_NORMALS)
        {
            offset += sizeof(RwV3d);
        }

        if ((ptankGlobal->format.dataFlags & rpPTANKDFLAGSIZE) != 0)
        {
            offset += sizeof(RwReal);
        }

        if( data->instanceColors )
        {
            data->colorsOut.data = (RwUInt8 *)(vertexData + offset);
            data->colorsOut.stride = strideVertex;
        }

    }

    rpTankD3D8UsePointSprites = TRUE;

    RWRETURNVOID();
}

/*
 ***************************************************************************
 */
static RwBool
rpPTankD3D8RenderCallBack(RpAtomic *atomic,
                          RpPTankData *ptankGlobal,
                          RwInt32 actPCount)
{
    RwBool          lighting;
    RpGeometry      *geom;
    RpMaterial      *material;
    RpGeometryFlag  flags;
    RwUInt32        fvf;
    RwUInt32        stride;
    RwUInt32        shadeMode;
    RwBool          useConstantColor = FALSE;
    RwBlendFunction srcBlend, dstBlend;
    RwInt32         numVertices;
    RwInt32         firstVertex;

    RWFUNCTION(RWSTRING("rpPTankD3D8RenderCallBack"));

    geom = RpAtomicGetGeometry(atomic);
    material = RpGeometryGetMaterial(geom,0);
    flags = (RpGeometryFlag)RpGeometryGetFlags(geom);

    /*
     * Calculate FVF and Stride
     */

    /* Positions */
    stride = sizeof(RwV3d);
    fvf = D3DFVF_XYZ;

    /* Normals */
    if (flags & rxGEOMETRY_NORMALS)
    {
        stride += sizeof(RwV3d);
        fvf |= D3DFVF_NORMAL;
    }

    /* Pre-lighting */
    if (flags & rxGEOMETRY_PRELIT)
    {
        stride += sizeof(RwRGBA);
        fvf |= D3DFVF_DIFFUSE;
    }

    /* Texture coordinates */
    if (flags & rxGEOMETRY_TEXTURED)
    {
        stride += sizeof(RwTexCoords);
        fvf |= D3DFVF_TEX1;
    }

    if (rpTankD3D8UsePointSprites &&
        (ptankGlobal->format.dataFlags & rpPTANKDFLAGSIZE) != 0)
    {
        stride += sizeof(RwReal);
        fvf |= D3DFVF_PSIZE;
    }

    /* Set lights */
    _rwD3D8AtomicDefaultLightingCallback((void *)atomic);

    /*
     * Set the world transform to identity
     */
    RwD3D8SetTransformWorld(NULL);


    /*
     * Set needed render states
     */
    RwD3D8GetRenderState(D3DRS_LIGHTING, &lighting);
    RwD3D8GetRenderState(D3DRS_SHADEMODE, &shadeMode);

    RwD3D8SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);

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
            RwUInt32 color;

            useConstantColor = TRUE;

            color = RGBATOD3D8COLOR(&(ptankGlobal->cColor));

            RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, color);

            RwD3D8SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);
        }
        else if( (ptankGlobal->format.dataFlags &
                 (rpPTANKDFLAGVTXCOLOR | rpPTANKDFLAGCNSVTXCOLOR)) == 0)
        {
            RwD3D8SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);
        }
    }

    /* As I don't relly in the bounding sphere I'll force clipping */
    RwD3D8SetRenderState(D3DRS_CLIPPING, TRUE);

    /*
     * Set the Default Pixel shader
     */
    RwD3D8SetPixelShader(0);

    /*
     * Vertex shader
     */
    RwD3D8SetVertexShader(fvf);

    /*
     * Texture
     */
    RwD3D8SetTexture(material->texture, 0);

    if (useConstantColor)
    {
        if (material->texture)
        {
            RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
        }
        else
        {
            RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
        }

            RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
        }

    /*
     * Enable vertex alpha blend if needed
     */
    if (ptankGlobal->vertexAlphaBlend)
    {
        RwRenderStateGet(rwRENDERSTATESRCBLEND, (void *)&srcBlend);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)(ptankGlobal->srcBlend));

        RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void *)&dstBlend);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)(ptankGlobal->dstBlend));

        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE);

        if (useConstantColor)
        {
            if (material->texture)
            {
                RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
            }
            else
            {
                RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
            }

            RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
        }
    }
    else
    {
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)FALSE);
    }

    if (lighting)
    {
        RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);

        RwD3D8SetSurfaceProperties(&(material->color),
                                    &(material->surfaceProps),
                                    TRUE);
    }

    /* Set the stream source */
    RwD3D8SetStreamSource(0, rpTankD3D8DynamicVertexBuffer, stride);

    /* render particles */
    if (rpTankD3D8UsePointSprites)
    {
        RwReal temp;

        RwD3D8SetRenderState(D3DRS_POINTSPRITEENABLE, TRUE);
        RwD3D8SetRenderState(D3DRS_POINTSCALEENABLE,  TRUE);

        temp = ptankGlobal->cSize.x;
        RwD3D8SetRenderState(D3DRS_POINTSIZE,     *((RwUInt32 *)(&temp)));

        temp = 0.00f;
        RwD3D8SetRenderState(D3DRS_POINTSIZE_MIN, *((RwUInt32 *)(&temp)));

        temp = rpTankD3D8VideoCardMaxPointSize;
        RwD3D8SetRenderState(D3DRS_POINTSIZE_MAX, *((RwUInt32 *)(&temp)));

        temp = 0.00f;
        RwD3D8SetRenderState(D3DRS_POINTSCALE_A,  *((RwUInt32 *)(&temp)));

        temp = 0.00f;
        RwD3D8SetRenderState(D3DRS_POINTSCALE_B,  *((RwUInt32 *)(&temp)));

        temp = 0.50f;
        RwD3D8SetRenderState(D3DRS_POINTSCALE_C,  *((RwUInt32 *)(&temp)));

        /* Draw the points sprites */
        numVertices = actPCount;
        firstVertex = rpTankD3D8BaseIndex;

        while (1)
        {
            if (numVertices <= 0xffff)
            {
                RwD3D8DrawPrimitive(D3DPT_POINTLIST, firstVertex, numVertices);

                break;
            }
            else
            {
                RwD3D8DrawPrimitive(D3DPT_POINTLIST, firstVertex, 0xffff);

                firstVertex += 0xffff;
                numVertices -= 0xffff;
            }
        }

        RwD3D8SetRenderState(D3DRS_POINTSPRITEENABLE, FALSE);
        RwD3D8SetRenderState(D3DRS_POINTSCALEENABLE,  FALSE);
    }
    else
    {
        const RwInt32 MaxVerticesPacket = (0xffff & (~3));

        numVertices = actPCount * 4;
        firstVertex = rpTankD3D8BaseIndex;

        RWASSERT(rpTankD3D8IndexBuffer != NULL);

        /* Draw the quads */
        while (1)
        {
            /* Set the Index buffer */
            RwD3D8SetIndices(rpTankD3D8IndexBuffer, firstVertex);

            /* Draw the indexed primitives */
            if (numVertices <= MaxVerticesPacket) /* Check num of triangles */
            {
                RwD3D8DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
                                           0, numVertices,
                                           0, (numVertices / 2) * 3);

                break;
            }
            else
            {

                RwD3D8DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
                                           0, MaxVerticesPacket,
                                           0, (MaxVerticesPacket / 2) * 3);

                numVertices -= MaxVerticesPacket;
                firstVertex += MaxVerticesPacket;
           }
        }
    }

    /* Restore render states */
    if (useConstantColor)
    {
        RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

        if (ptankGlobal->vertexAlphaBlend)
        {
            RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        }
    }

    RwD3D8SetRenderState(D3DRS_SHADEMODE, shadeMode);

    if (ptankGlobal->vertexAlphaBlend)
    {
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)srcBlend);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)dstBlend);
    }

    RWRETURN(FALSE);
}

/*
 ***************************************************************************
 */
static RwBool
rpPTankD3D8Create(RpAtomic *atomic, RpPTankData *ptankGlobal,
            RwInt32 maxPCount, RwUInt32 dataFlags,
            RwUInt32 platFlags)
{
    RwBool              result = TRUE;
    RpGeometry          *geom = NULL;
    RwSurfaceProperties surfProp;
    RpMaterial          *material;
    RwUInt32            geomFlag;
    RpTriangle          triangle;

    RWFUNCTION(RWSTRING("rpPTankD3D8Create"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    geomFlag = rpGEOMETRYPOSITIONS;

    if( (dataFlags & rpPTANKDFLAGNORMAL) == rpPTANKDFLAGNORMAL )
    {
        geomFlag |= rpGEOMETRYLIGHT
                    | rpGEOMETRYNORMALS
                    | rpGEOMETRYMODULATEMATERIALCOLOR;
    }

    if( (dataFlags &
        (rpPTANKDFLAGCOLOR | rpPTANKDFLAGVTXCOLOR | rpPTANKDFLAGCNSVTXCOLOR)) != 0 )
    {
        geomFlag |= rpGEOMETRYPRELIT;
    }

    /* Check point sprites */
    if((platFlags & rpPTANKD3D8FLAGSUSEPOINTSPRITES)
                                        == rpPTANKD3D8FLAGSUSEPOINTSPRITES)
    {
        if (rpTankD3D8VideoCardMaxPointSize <= 1.f)
        {
            RWMESSAGE(("Point Sprites not supported by current 3D Hardware"));

            platFlags &= ~rpPTANKD3D8FLAGSUSEPOINTSPRITES;
        }
        else if ((ptankGlobal->format.dataFlags & (~VALIDPOINTSPRITESFLAGS))
                                                != 0)
        {
            if ( (ptankGlobal->format.dataFlags & (~VALIDPOINTSPRITESSIZEFLAGS)) != 0 ||
                 !rpTankD3D8VideoCardSizePerParticle )
            {
                RWMESSAGE(("dataFlag not valid for Point Sprites"));

                platFlags &= ~rpPTANKD3D8FLAGSUSEPOINTSPRITES;
            }
        }
    }

    if((platFlags & rpPTANKD3D8FLAGSUSEPOINTSPRITES)
                                            != rpPTANKD3D8FLAGSUSEPOINTSPRITES)
    {
        geomFlag |= rpGEOMETRYTEXTURED;
    }

    geom = RpGeometryCreateSpace(1.f);

    RpGeometrySetFlags(geom, geomFlag);

    /*
     * Create the material
     */
    material = RpMaterialCreate();

    /*
     * Set the surface reflection coefficients...
     */
    surfProp.ambient = 0.3f;
    surfProp.diffuse = 1.0f;
    surfProp.specular = 1.0f;
    RpMaterialSetSurfaceProperties(material, &surfProp);

    /* Add the material to a dummy triangle */
    RpGeometryTriangleSetMaterial(geom, &triangle, material);

    /*
     * Associate the geom with the atomic. This will increment the
     * geom's reference count by 1...
     */
    RpAtomicSetGeometry(atomic, geom, 0);

    /*
     * As a convenience, we can remove the application's ownership of the
     * geom, materials and textures it created by calling the corresponding
     * destroy functions. This will decrement their reference counts
     * without actually deleting the resources because they now have other
     * owners (the atomic owns the geom, the geom and its triangles
     * own the materials, each material owns a texture). Now we can simply use
     * RpClumpDestroy later when the application has finished with it...
     */
    RpGeometryDestroy(geom);

    RpMaterialDestroy(material);

    if (result)
    {
        /*
         * Check index buffer size if needed
         */
        if((platFlags & rpPTANKD3D8FLAGSUSEPOINTSPRITES)
                        != rpPTANKD3D8FLAGSUSEPOINTSPRITES)
        {
            if (rpTankD3D8IndexBuffer == NULL ||
                rpTankD3D8MaxIndex < (maxPCount * 2 * 3))
            {
                if (rpTankD3D8IndexBuffer != NULL)
                {
                    IDirect3DIndexBuffer8_Release(rpTankD3D8IndexBuffer);
                    rpTankD3D8IndexBuffer = NULL;
                }

                rpTankD3D8MaxIndex = (maxPCount * 2 * 3);

                if (RwD3D8IndexBufferCreate(rpTankD3D8MaxIndex, (void **)&rpTankD3D8IndexBuffer))
                {
                    RxVertexIndex   *indexData;

                    if (SUCCEEDED(IDirect3DIndexBuffer8_Lock(rpTankD3D8IndexBuffer,
                                                            0, 0,
                                                            (RwUInt8 **)&indexData,
                                                            0)))
                    {
                        RwInt32 i;

                        for (i=0; i < maxPCount; i++)
                        {
                            indexData[0] = i * 4 + 0;
                            indexData[1] = i * 4 + 1;
                            indexData[2] = i * 4 + 2;

                            indexData[3] = i * 4 + 0;
                            indexData[4] = i * 4 + 2;
                            indexData[5] = i * 4 + 3;

                            indexData += 2 * 3;
                        }

                        IDirect3DIndexBuffer8_Unlock(rpTankD3D8IndexBuffer);
                    }
                }
            }
        }
    }

    /* Initialize constant color to black opaque */
    ptankGlobal->cColor.red = 0;
    ptankGlobal->cColor.green = 0;
    ptankGlobal->cColor.blue = 0;
    ptankGlobal->cColor.alpha = 0xff;

    _rpPTankGENInstanceSelectCB(atomic);


    /* Instancing Callback Patch */
    {
        RpPTankAtomicExtPrv *PTankPrv;

        PTankPrv = RPATOMICPTANKPLUGINDATA(atomic);

        if((platFlags & rpPTANKD3D8FLAGSUSEPOINTSPRITES)
                                            == rpPTANKD3D8FLAGSUSEPOINTSPRITES)
        {
            PTankPrv->insSetupCB =
                    (RpPTankGENInstanceSetupCallback)rpPTankD3D8InsSetup_spr;

            if ((ptankGlobal->format.dataFlags & rpPTANKDFLAGSIZE) != 0)
            {
                if( PTankPrv->insPosCB == (RpPTankGENInstancePosCallback)
                                                rpPTankGENInsPosNCPSNR )
                {
                    PTankPrv->insPosCB = (RpPTankGENInstancePosCallback)
                                                rpPTankD3D8InsPosNCPSNR_spr;
                }
                else if( PTankPrv->insPosCB == (RpPTankGENInstancePosCallback)
                                                rpPTankGENInsPosCCPSNR )
                {
                    PTankPrv->insPosCB = (RpPTankGENInstancePosCallback)
                                                rpPTankD3D8InsPosCCPSNR_spr;
                }
                else
                {
                    RWASSERT(FALSE && "Not supported point sprites instance funtion.");
                }
            }
            else
            {
                if( PTankPrv->insPosCB == (RpPTankGENInstancePosCallback)
                                                rpPTankGENInsPosNCCSNR )
                {
                    PTankPrv->insPosCB = (RpPTankGENInstancePosCallback)
                                                rpPTankD3D8InsPosNCCSNR_spr;
                }
                else if( PTankPrv->insPosCB == (RpPTankGENInstancePosCallback)
                                                rpPTankGENInsPosCCCSNR )
                {
                    PTankPrv->insPosCB = (RpPTankGENInstancePosCallback)
                                                rpPTankD3D8InsPosCCCSNR_spr;
                }
                else
                {
                    RWASSERT(FALSE && "Not supported point sprites instance funtion.");
                }
            }

            if( PTankPrv->insNormalsCB == (RpPTankGENInstanceCallback)
                                            rpPTankGENInsNormals)
            {
                PTankPrv->insNormalsCB = (RpPTankGENInstanceCallback)
                                            rpPTankD3D8InsNormals_spr;
            }

            if( PTankPrv->insColorsCB == (RpPTankGENInstanceCallback)
                                            rpPTankGENInsColorPP)
            {
                PTankPrv->insColorsCB = (RpPTankGENInstanceCallback)
                                            rpPTankD3D8InsColor_spr;
            }
        }
        else
        {
            PTankPrv->insSetupCB =
                    (RpPTankGENInstanceSetupCallback)rpPTankD3D8InsSetup;
        }

        PTankPrv->insEndingCB =
                        (RpPTankGENInstanceSetupCallback)rpPTankD3D8InsEnding;
    }


    RWRETURN(result);
}


/*
 *****************************************************************************
 */
static void
rpPTankD3D8InsPosNCCSNR_spr(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                        /* no center, constant size, no rotation */
{
    RwInt32 i;
    RwV3d *src;
    RwV3d *dst;
    RwUInt8 *uSrc;
    RwUInt32 uStride;
    RwUInt8 *uDst;
    RwUInt32 uDStride;

    RWFUNCTION(RWSTRING("rpPTankD3D8InsPosNCCSNR_spr"));
    RWASSERT(TRUE == rpTankD3D8UsePointSprites);

    uSrc = ptankGlobal->clusters[RPPTANKSIZEPOSITION].data;
    uStride = ptankGlobal->clusters[RPPTANKSIZEPOSITION].stride;

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    if (uStride == uDStride &&
        uStride == sizeof(RwV3d))
    {
        memcpy(uDst, uSrc, pCount * sizeof(RwV3d));
    }
    else
    {
        for (i = 0 ; i < pCount ; i++)
        {
            src = (RwV3d*)uSrc;

            /* vtx 0 */
            dst = (RwV3d*)uDst;
            dst->x = src->x;
            dst->y = src->y;
            dst->z = src->z;
            uDst += uDStride;

            uSrc += uStride;

        }
    }

    RWRETURNVOID();
}

static void
rpPTankD3D8InsPosCCCSNR_spr(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                        /* no center, constant size, no rotation */
{
    RwInt32 i;
    RwV3d *src;
    RwV3d *dst;
    RwUInt8 *uSrc;
    RwUInt32 uStride;
    RwUInt8 *uDst;
    RwUInt32 uDStride;

    RWFUNCTION(RWSTRING("rpPTankD3D8InsPosCCCSNR_spr"));
    RWASSERT(TRUE == rpTankD3D8UsePointSprites);

    uSrc = ptankGlobal->clusters[RPPTANKSIZEPOSITION].data;
    uStride = ptankGlobal->clusters[RPPTANKSIZEPOSITION].stride;

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    for (i = 0 ; i < pCount ; i++)
    {
        src = (RwV3d*)uSrc;

        /* vtx 0 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + ptankGlobal->cCenter.x;
        dst->y = src->y + ptankGlobal->cCenter.y;
        dst->z = src->z ;
        uDst += uDStride;

        uSrc += uStride;

    }

    RWRETURNVOID();
}
static void
rpPTankD3D8InsPosNCPSNR_spr(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                        /* no center, constant size, no rotation */
{
    RwInt32 i;
    RwV3d *src;
    RwV2d *srcSize;
    RwV3d *dst;
    RwReal *dstSize;
    RwUInt8 *uSrc;
    RwUInt32 uStride;
    RwUInt8 *uSize;
    RwUInt32 uSStride;
    RwUInt8 *uDst;
    RwUInt32 uDStride;
    RwUInt8 *uDstSize;

    RWFUNCTION(RWSTRING("rpPTankD3D8InsPosNCPSNR_spr"));
    RWASSERT(TRUE == rpTankD3D8UsePointSprites);

    uSrc = ptankGlobal->clusters[RPPTANKSIZEPOSITION].data;
    uStride = ptankGlobal->clusters[RPPTANKSIZEPOSITION].stride;

    uSize = ptankGlobal->clusters[RPPTANKSIZESIZE].data;
    uSStride = ptankGlobal->clusters[RPPTANKSIZESIZE].stride;

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    uDstSize = uDst + sizeof(RwV3d);
    if (ptankGlobal->format.dataFlags & rpPTANKDFLAGNORMAL)
    {
        uDstSize += sizeof(RwV3d);
    }

    for (i = 0 ; i < pCount ; i++)
    {
        src = (RwV3d*)uSrc;
        srcSize = (RwV2d*)uSize;

        /* vtx 0 */
        dst = (RwV3d*)uDst;
        dst->x = src->x;
        dst->y = src->y;
        dst->z = src->z;
        uDst += uDStride;

        dstSize = (RwReal*)uDstSize;
        *dstSize = srcSize->x;
        uDstSize += uDStride;

        uSrc += uStride;
        uSize += uSStride;
    }

    RWRETURNVOID();
}

static void
rpPTankD3D8InsPosCCPSNR_spr(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                        /* no center, constant size, no rotation */
{
    RwInt32 i;
    RwV3d *src;
    RwV2d *srcSize;
    RwV3d *dst;
    RwReal *dstSize;
    RwUInt8 *uSrc;
    RwUInt32 uStride;
    RwUInt8 *uSize;
    RwUInt32 uSStride;
    RwUInt8 *uDst;
    RwUInt32 uDStride;
    RwUInt8 *uDstSize;

    RWFUNCTION(RWSTRING("rpPTankD3D8InsPosCCPSNR_spr"));
    RWASSERT(TRUE == rpTankD3D8UsePointSprites);

    uSrc = ptankGlobal->clusters[RPPTANKSIZEPOSITION].data;
    uStride = ptankGlobal->clusters[RPPTANKSIZEPOSITION].stride;

    uSize = ptankGlobal->clusters[RPPTANKSIZESIZE].data;
    uSStride = ptankGlobal->clusters[RPPTANKSIZESIZE].stride;

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    uDstSize = uDst + sizeof(RwV3d);
    if (ptankGlobal->format.dataFlags & rpPTANKDFLAGNORMAL)
    {
        uDstSize += sizeof(RwV3d);
    }

    for (i = 0 ; i < pCount ; i++)
    {
        src = (RwV3d*)uSrc;
        srcSize = (RwV2d*)uSize;

        /* vtx 0 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + ptankGlobal->cCenter.x;
        dst->y = src->y + ptankGlobal->cCenter.y;
        dst->z = src->z ;
        uDst += uDStride;

        dstSize = (RwReal*)uDstSize;
        *dstSize = srcSize->x;
        uDstSize += uDStride;

        uSrc += uStride;
        uSize += uSStride;
    }

    RWRETURNVOID();
}

static void
rpPTankD3D8InsNormals_spr(RpPTankLockStruct *dstCluster,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
{
    RwInt32 i;
    RwUInt8 *uDst;
    RwUInt32 uDStride;
    RwUInt8 *uSrc;
    RwUInt32 uSStride;
    RwV3d *src,*dst;

    RWFUNCTION(RWSTRING("rpPTankD3D8InsNormals_spr"));
    RWASSERT(TRUE == rpTankD3D8UsePointSprites);

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    uSrc = ptankGlobal->clusters[RPPTANKSIZENORMAL].data;
    uSStride = ptankGlobal->clusters[RPPTANKSIZENORMAL].stride;

    for(i=0;i<pCount;i++)
    {
        src = (RwV3d *)uSrc;

        dst = (RwV3d *)uDst;
        dst->x = src->x;
        dst->y = src->y;
        dst->z = src->z;
        uDst += uDStride;

        uSrc += uSStride;
    }

    RWRETURNVOID();
}

static void
rpPTankD3D8InsColor_spr(RpPTankLockStruct *dstCluster,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                         /* 1 Color per particle*/
{
    RwInt32 i;
    RwRGBA *src;
    RwUInt32 *dst;
    RwUInt8 *uDst;
    RwUInt32 uDStride;
    RwUInt8 *uSrc;
    RwUInt32 uSStride;

    RWFUNCTION(RWSTRING("rpPTankD3D8InsColor_spr"));
    RWASSERT(TRUE == rpTankD3D8UsePointSprites);

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    uSrc = ptankGlobal->clusters[RPPTANKSIZECOLOR].data;
    uSStride = ptankGlobal->clusters[RPPTANKSIZECOLOR].stride;

    for (i = 0 ; i < pCount ; i++)
    {
        src = (RwRGBA *)uSrc;

        /* vtx 0 */
        dst = (RwUInt32 *)uDst;
        *dst = RGBATOD3D8COLOR(src);
        uDst += uDStride;

        uSrc += uSStride;

    }

    RWRETURNVOID();
}


/*
 *****************************************************************************
 */
void *
PTankOpen(void *object,
                    RwInt32 __RWUNUSED__ offset,
                    RwInt32 __RWUNUSED__ size)
{
    const D3DCAPS8 *d3d8caps;

    RWFUNCTION(RWSTRING("PTankOpen"));

    d3d8caps = (const D3DCAPS8 *)RwD3D8GetCaps();

    rpTankD3D8VideoCardMaxPointSize =
                        (d3d8caps->MaxPointSize);

    rpTankD3D8VideoCardSizePerParticle =
                        ((d3d8caps->FVFCaps & D3DFVFCAPS_PSIZE) != 0);

    RWRETURN(object);
}


/*
 *****************************************************************************
 */
void        *
PTankClose(void *object,
                    RwInt32 __RWUNUSED__ offset,
                    RwInt32 __RWUNUSED__ size)
{
    RWFUNCTION(RWSTRING("PTankClose"));

    if (rpTankD3D8IndexBuffer != NULL)
    {
        IDirect3DIndexBuffer8_Release(rpTankD3D8IndexBuffer);
        rpTankD3D8IndexBuffer = NULL;
    }

    rpTankD3D8MaxIndex = 0;

    RWRETURN(object);
}
