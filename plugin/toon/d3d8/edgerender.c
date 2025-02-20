
#include <d3d8.h>

#include "rwcore.h"
#include "rpworld.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "toon.h"
#include "edgerender.h"

#include "toonvertexshader.h"
#include "toonvertexshaderflat.h"

#define RPTOON_MAX_VERTICES_BATCH    10000

typedef struct
{
    RwV3d pos;
    RwUInt32 color;
} EdgeVertex;

typedef struct
{
    RwV3d pos;
} EdgeVertexFlat;

typedef struct
{
    RwV3d pos;
    RwV3d normal;
    RwUInt32 color;
    RwV2d inkcoef;
} EdgeVertexShader;

typedef struct
{
    RwV3d pos;
    RwV3d normal;
    RwReal thickness;
} EdgeVertexShaderFlat;


static LPDIRECT3DINDEXBUFFER8 IndexBuffer = NULL;
static LPDIRECT3DVERTEXBUFFER8 VertexBuffer = NULL;
static RwUInt32 Offset = 0;
static RwUInt32 NumVertices = 0;
static RwUInt32 MaxVertices = 0;
static RwUInt32 MaxBatch = 0;

static EdgeVertex *VertexData = NULL;
static EdgeVertexFlat *VertexFlatData = NULL;
static EdgeVertexShader *VertexShaderData = NULL;
static EdgeVertexShaderFlat *VertexShaderFlatData = NULL;

static RwUInt32 VertexShader = 0;
static RwUInt32 VertexShaderFlat = 0;

static RwUInt32 OldShadeMode;
static RwUInt32 OldDither;

static const RwUInt32 VertexShaderDecl[] =
{
    D3DVSD_STREAM( 0 ),
    D3DVSD_REG( 0, D3DVSDT_FLOAT3 ),
    D3DVSD_REG( 1, D3DVSDT_FLOAT3 ),
    D3DVSD_REG( 2, D3DVSDT_D3DCOLOR ),
    D3DVSD_REG( 3, D3DVSDT_FLOAT2 ),
    D3DVSD_END()
};

static const RwUInt32 VertexShaderFlatDecl[] =
{
    D3DVSD_STREAM( 0 ),
    D3DVSD_REG( 0, D3DVSDT_FLOAT3 ),
    D3DVSD_REG( 1, D3DVSDT_FLOAT3 ),
    D3DVSD_REG( 2, D3DVSDT_FLOAT1 ),
    D3DVSD_END()
};

/****************************************************************************
 _rpToonEdgeRenderInit
 */
void
_rpToonEdgeRenderInit(void)
{
    const D3DCAPS8      *d3dCaps;
    RwUInt16 *indices;
    RwUInt32 i;

    RWFUNCTION(RWSTRING("_rpToonEdgeRenderInit"));

    /* Create index buffer */
    RwD3D8IndexBufferCreate(((RPTOON_MAX_VERTICES_BATCH / 4) * 6),
                            &IndexBuffer);
    RWASSERT(IndexBuffer != NULL);

    IDirect3DIndexBuffer8_Lock(IndexBuffer, 0, 0,
                               (RwUInt8 **)&indices,
                               D3DLOCK_NOSYSLOCK);


    for (i = 0; i < (RPTOON_MAX_VERTICES_BATCH / 4); i++)
    {
        indices[0] = i * 4 + 0;
        indices[1] = i * 4 + 1;
        indices[2] = i * 4 + 2;

        indices[3] = i * 4 + 2;
        indices[4] = i * 4 + 1;
        indices[5] = i * 4 + 3;

        indices += 6;
    }

    IDirect3DIndexBuffer8_Unlock(IndexBuffer);

    /* Vertex shaders */
    d3dCaps = (const D3DCAPS8 *)RwD3D8GetCaps();

    if ((d3dCaps->VertexShaderVersion & 0xFFFF) >= 0x0101 &&
        (d3dCaps->PixelShaderVersion & 0xFFFF) >= 0x0101)
    {
        RwD3D8CreateVertexShader(VertexShaderDecl, dwToonVertexShader, &VertexShader, 0);

        RwD3D8CreateVertexShader(VertexShaderFlatDecl, dwToonVertexShaderFlat, &VertexShaderFlat, 0);
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpToonEdgeRenderClean
 */
void
_rpToonEdgeRenderClean(void)
{
    RWFUNCTION(RWSTRING("_rpToonEdgeRenderClean"));

    if (IndexBuffer)
    {
        IDirect3DIndexBuffer8_Release(IndexBuffer);
        IndexBuffer = NULL;
    }

    if (VertexShaderFlat != 0)
    {
        RwD3D8DeleteVertexShader(VertexShaderFlat);

        VertexShaderFlat = 0;
    }

    if (VertexShader != 0)
    {
        RwD3D8DeleteVertexShader(VertexShader);

        VertexShader = 0;
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpToonEdgeRenderPushState
 */
void
_rpToonEdgeRenderPushState(void)
{
    RWFUNCTION(RWSTRING("_rpToonEdgeRenderPushState"));

    RwD3D8GetRenderState(D3DRS_SHADEMODE, &OldShadeMode);
    RwD3D8GetRenderState(D3DRS_DITHERENABLE, &OldDither);

    RwD3D8SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);
    RwD3D8SetRenderState(D3DRS_DITHERENABLE, FALSE);

    RwD3D8SetTexture(NULL, 0);

    RWRETURNVOID();
}

/****************************************************************************
 _rpToonEdgeRenderPopState
 */
void
_rpToonEdgeRenderPopState(void)
{
    RWFUNCTION(RWSTRING("_rpToonEdgeRenderPopState"));

    RwD3D8SetRenderState(D3DRS_SHADEMODE, OldShadeMode);
    RwD3D8SetRenderState(D3DRS_DITHERENABLE, OldDither);

    RWRETURNVOID();
}

/****************************************************************************
 GetScreenSpaceProjection
 */
void
GetScreenSpaceProjection(ScreenSpaceProjection *projection,
                         const RwMatrix *transform)
{
    RwCamera *camera;
    RwFrame *cameraFrame;

    RWFUNCTION(RWSTRING("GetScreenSpaceProjection"));

    camera = RwCameraGetCurrentCamera();
    RWASSERT(0 != camera);

    cameraFrame = RwCameraGetFrame(camera);
    RWASSERT(0 != cameraFrame);

    if (transform != NULL)
    {
        RwMatrix invCamLTM;

        RwMatrixInvert( &invCamLTM, RwFrameGetLTM(cameraFrame));

        RwMatrixMultiply( &projection->matrix, transform, &invCamLTM );
    }
    else
    {
        RwMatrixInvert( &projection->matrix, RwFrameGetLTM(cameraFrame));
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpToonEdgeRenderPrepareSubmit
 */
void
_rpToonEdgeRenderPrepareSubmit(RwUInt32 maxNumEdges)
{
    RwCamera *camera;
    RwFrame *cameraFrame;

    RWFUNCTION(RWSTRING("_rpToonEdgeRenderPrepareSubmit"));

    camera = RwCameraGetCurrentCamera();
    RWASSERT(0 != camera);

    cameraFrame = RwCameraGetFrame(camera);
    RWASSERT(0 != cameraFrame);

    /* World matrix */
    RwD3D8SetTransformWorld(RwFrameGetLTM(cameraFrame));

    /* Disable vertex alpha blending */
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)FALSE);

    /* Disable lighting */
    RwD3D8SetRenderState(D3DRS_LIGHTING, FALSE);

    RwD3D8SetRenderState(D3DRS_CLIPPING, TRUE);

    /* Set color ops */
    RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
    RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    /* FFP vertex shader */
    RwD3D8SetVertexShader(D3DFVF_XYZ | D3DFVF_DIFFUSE);

    /* Disable any pixel shader */
    RwD3D8SetPixelShader(0);

    NumVertices = 0;

    MaxVertices = maxNumEdges * 4;

    if (MaxVertices > RPTOON_MAX_VERTICES_BATCH)
    {
        MaxBatch = RPTOON_MAX_VERTICES_BATCH;
    }
    else if (MaxVertices > 1000)
    {
        MaxBatch = ((maxNumEdges / 4) * 4); /* We need a multiple of for */
    }

    if (MaxBatch)
    {
        RwD3D8DynamicVertexBufferLock(sizeof(EdgeVertex),
                                      MaxBatch,
                                      (void**)&VertexBuffer,
                                      (RwUInt8 **)&VertexData,
                                      &Offset);
    }

    RWRETURNVOID();
}

void
_rpToonEdgeRenderSubmit(void)
{
    RWFUNCTION(RWSTRING("_rpToonEdgeRenderSubmit"));

    if (MaxBatch)
    {
        RwD3D8DynamicVertexBufferUnlock(VertexBuffer);
    }

    if (NumVertices == 0)
    {
        /* early out to avoid crashing driver */
        RWRETURNVOID();
    }

    RWASSERT(NumVertices <= MaxBatch);

    RwD3D8SetIndices(IndexBuffer, Offset);

    RwD3D8SetStreamSource(0, VertexBuffer, sizeof(EdgeVertex));

    RwD3D8DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
                               0, NumVertices,
                               0, (NumVertices / 4) * 6);


    RWRETURNVOID();
}

void
_rpToonEdgeRenderAppendQuad(const RwV3d *vIn0,
                            const RwV3d *vIn1,
                            const RwV2d *vOut0,
                            const RwV2d *vOut1,
                            RwRGBA color)
{
    RwUInt32 d3dcolor;

    RWFUNCTION( RWSTRING( "_rpToonEdgeRenderAppendQuad" ) );

    RWASSERT(NumVertices <= MaxVertices);

    if (NumVertices >= MaxBatch)
    {
        _rpToonEdgeRenderSubmit();

        NumVertices = 0;

        MaxVertices -= MaxBatch;

        MaxBatch = min(MaxVertices, RPTOON_MAX_VERTICES_BATCH);
        RWASSERT(MaxBatch > 0);

        RwD3D8DynamicVertexBufferLock(sizeof(EdgeVertex),
                                    MaxBatch,
                                    (void**)&VertexBuffer,
                                    (RwUInt8 **)&VertexData,
                                    &Offset);
    }

    d3dcolor = (((RwUInt32)color.alpha) << 24) |
                (((RwUInt32)color.red) << 16) |
                (((RwUInt32)color.green) << 8) |
                color.blue;

    VertexData->pos.x = vIn0->x;
    VertexData->pos.y = vIn0->y;
    VertexData->pos.z = vIn0->z;
    VertexData->color = d3dcolor;
    VertexData++;

    VertexData->pos.x = vIn1->x;
    VertexData->pos.y = vIn1->y;
    VertexData->pos.z = vIn1->z;
    VertexData->color = d3dcolor;
    VertexData++;

    VertexData->pos.x = vOut0->x;
    VertexData->pos.y = vOut0->y;
    VertexData->pos.z = vIn0->z;
    VertexData->color = d3dcolor;
    VertexData++;

    VertexData->pos.x = vOut1->x;
    VertexData->pos.y = vOut1->y;
    VertexData->pos.z = vIn1->z;
    VertexData->color = d3dcolor;
    VertexData++;

    NumVertices += 4;

    RWRETURNVOID();
}

/****************************************************************************
 _rpToonEdgeRenderPrepareSubmit
 */
void
_rpToonEdgeRenderPrepareFlatSubmit(RwUInt32 maxNumEdges,
                                   RwRGBA color)
{
    RwCamera *camera;
    RwFrame *cameraFrame;
    RwUInt32 inkColor;

    RWFUNCTION(RWSTRING("_rpToonEdgeRenderPrepareSubmit"));

    camera = RwCameraGetCurrentCamera();
    RWASSERT(0 != camera);

    cameraFrame = RwCameraGetFrame(camera);
    RWASSERT(0 != cameraFrame);

    /* World matrix */
    RwD3D8SetTransformWorld(RwFrameGetLTM(cameraFrame));

    /* Disable vertex alpha blending */
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)FALSE);

    /* Disable lighting */
    RwD3D8SetRenderState(D3DRS_LIGHTING, FALSE);

    RwD3D8SetRenderState(D3DRS_CLIPPING, TRUE);

    inkColor = (((RwUInt32)color.alpha) << 24) |
                (((RwUInt32)color.red) << 16) |
                (((RwUInt32)color.green) << 8) |
                color.blue;

    RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, inkColor);

    /* Set color ops */
    RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
    RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);

    /* FFP vertex shader */
    RwD3D8SetVertexShader(D3DFVF_XYZ);

    /* Disable any pixel shader */
    RwD3D8SetPixelShader(0);

    NumVertices = 0;

    MaxVertices = maxNumEdges * 4;

    if (MaxVertices > RPTOON_MAX_VERTICES_BATCH)
    {
        MaxBatch = RPTOON_MAX_VERTICES_BATCH;
    }
    else if (MaxVertices > 1000)
    {
        MaxBatch = ((maxNumEdges / 4) * 4); /* We need a multiple of for */
    }

    if (MaxBatch)
    {
        RwD3D8DynamicVertexBufferLock(sizeof(EdgeVertexFlat),
                                      MaxBatch,
                                      (void**)&VertexBuffer,
                                      (RwUInt8 **)&VertexFlatData,
                                      &Offset);
    }

    RWRETURNVOID();
}

void
_rpToonEdgeRenderFlatSubmit(void)
{
    RWFUNCTION(RWSTRING("_rpToonEdgeRenderFlatSubmit"));

    if (MaxBatch)
    {
        RwD3D8DynamicVertexBufferUnlock(VertexBuffer);
    }

    if (NumVertices == 0)
    {
        /* early out to avoid crashing driver */
        RWRETURNVOID();
    }

    RWASSERT(NumVertices <= MaxBatch);

    RwD3D8SetIndices(IndexBuffer, Offset);

    RwD3D8SetStreamSource(0, VertexBuffer, sizeof(EdgeVertexFlat));

    RwD3D8DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
                               0, NumVertices,
                               0, (NumVertices / 4) * 6);


    RWRETURNVOID();
}

void
_rpToonEdgeRenderAppendFlatQuad(const RwV3d *vIn0,
                                const RwV3d *vIn1,
                                const RwV2d *vOut0,
                                const RwV2d *vOut1)
{
    RWFUNCTION( RWSTRING( "_rpToonEdgeRenderAppendFlatQuad" ) );

    RWASSERT(NumVertices <= MaxVertices);

    if (NumVertices >= MaxBatch)
    {
        _rpToonEdgeRenderFlatSubmit();

        NumVertices = 0;

        MaxVertices -= MaxBatch;

        MaxBatch = min(MaxVertices, RPTOON_MAX_VERTICES_BATCH);
        RWASSERT(MaxBatch > 0);

        RwD3D8DynamicVertexBufferLock(sizeof(EdgeVertexFlat),
                                    MaxBatch,
                                    (void**)&VertexBuffer,
                                    (RwUInt8 **)&VertexFlatData,
                                    &Offset);
    }

    VertexFlatData->pos.x = vIn0->x;
    VertexFlatData->pos.y = vIn0->y;
    VertexFlatData->pos.z = vIn0->z;
    VertexFlatData++;

    VertexFlatData->pos.x = vIn1->x;
    VertexFlatData->pos.y = vIn1->y;
    VertexFlatData->pos.z = vIn1->z;
    VertexFlatData++;

    VertexFlatData->pos.x = vOut0->x;
    VertexFlatData->pos.y = vOut0->y;
    VertexFlatData->pos.z = vIn0->z;
    VertexFlatData++;

    VertexFlatData->pos.x = vOut1->x;
    VertexFlatData->pos.y = vOut1->y;
    VertexFlatData->pos.z = vIn1->z;
    VertexFlatData++;

    NumVertices += 4;

    RWRETURNVOID();
}

/****************************************************************************
 _rpToonEdgeRenderPrepareVertexShaderSubmit
 */
void
_rpToonEdgeRenderPrepareVertexShaderSubmit(RwUInt32 maxNumEdges)
{
    RWFUNCTION(RWSTRING("_rpToonEdgeRenderPrepareVertexShaderSubmit"));

    /* Disable vertex alpha blending */
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)FALSE);

    /* Enable clipping */
    RwD3D8SetRenderState(D3DRS_CLIPPING, TRUE);

    /* Set color ops */
    RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
    RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    /* Set vertex shader */
    RwD3D8SetVertexShader(VertexShader);

    /* Disable any pixel shader */
    RwD3D8SetPixelShader(0);

    NumVertices = 0;

    MaxVertices = maxNumEdges * 4;

    if (MaxVertices > RPTOON_MAX_VERTICES_BATCH)
    {
        MaxBatch = RPTOON_MAX_VERTICES_BATCH;
    }
    else if (MaxVertices > 1000)
    {
        MaxBatch = ((maxNumEdges / 4) * 4); /* We need a multiple of for */
    }

    if (MaxBatch)
    {
        RwD3D8DynamicVertexBufferLock(sizeof(EdgeVertexShader),
                                      MaxBatch,
                                      (void**)&VertexBuffer,
                                      (RwUInt8 **)&VertexShaderData,
                                      &Offset);
    }

    RWRETURNVOID();
}

void
_rpToonEdgeRenderAppendVertexShaderQuad(const RwV3d *vPos0,
                                        const RwV3d *vPos1,
                                        const RwV3d *vNormal0,
                                        const RwV3d *vNormal1,
                                        RwReal vertexThicknesses0,
                                        RwReal vertexThicknesses1,
                                        const RpToonInk *ink)
{
    RwUInt32 d3dcolor;

    RWFUNCTION( RWSTRING( "_rpToonEdgeRenderAppendVertexShaderQuad" ) );

    RWASSERT(NumVertices <= MaxVertices);

    if (NumVertices >= MaxBatch)
    {
        _rpToonEdgeRenderVertexShaderSubmit();

        NumVertices = 0;

        MaxVertices -= MaxBatch;

        MaxBatch = min(MaxVertices, RPTOON_MAX_VERTICES_BATCH);
        RWASSERT(MaxBatch > 0);

        RwD3D8DynamicVertexBufferLock(sizeof(EdgeVertexShader),
                                    MaxBatch,
                                    (void**)&VertexBuffer,
                                    (RwUInt8 **)&VertexShaderData,
                                    &Offset);
    }

    d3dcolor = (((RwUInt32)ink->color.alpha) << 24) |
                (((RwUInt32)ink->color.red) << 16) |
                (((RwUInt32)ink->color.green) << 8) |
                ink->color.blue;

    VertexShaderData->pos.x = vPos0->x;
    VertexShaderData->pos.y = vPos0->y;
    VertexShaderData->pos.z = vPos0->z;
    VertexShaderData->normal.x = vNormal0->x;
    VertexShaderData->normal.y = vNormal0->y;
    VertexShaderData->normal.z = vNormal0->z;
    VertexShaderData->color = d3dcolor;
    VertexShaderData->inkcoef.x = 0.0f;
    VertexShaderData->inkcoef.y = 0.0f;
    VertexShaderData++;

    VertexShaderData->pos.x = vPos1->x;
    VertexShaderData->pos.y = vPos1->y;
    VertexShaderData->pos.z = vPos1->z;
    VertexShaderData->normal.x = vNormal1->x;
    VertexShaderData->normal.y = vNormal1->y;
    VertexShaderData->normal.z = vNormal1->z;
    VertexShaderData->color = d3dcolor;
    VertexShaderData->inkcoef.x = 0.0f;
    VertexShaderData->inkcoef.y = 0.0f;
    VertexShaderData++;

    if (ink->perspectiveScale_b != 0.0f)
    {
        RwReal auxCoef1, auxCoef2;

        auxCoef1 = (ink->farScale_factor) * (ink->perspectiveScale_b);
        auxCoef2 = (ink->farScale_factor) * ((ink->perspectiveScale_a) - (ink->perspectiveScale_b) * (ink->perspectiveScale_zMin));

        VertexShaderData->pos.x = vPos0->x;
        VertexShaderData->pos.y = vPos0->y;
        VertexShaderData->pos.z = vPos0->z;
        VertexShaderData->normal.x = vNormal0->x;
        VertexShaderData->normal.y = vNormal0->y;
        VertexShaderData->normal.z = vNormal0->z;
        VertexShaderData->color = d3dcolor;
        VertexShaderData->inkcoef.x = vertexThicknesses0 * auxCoef1;
        VertexShaderData->inkcoef.y =  vertexThicknesses0 * auxCoef2;
        VertexShaderData++;

        VertexShaderData->pos.x = vPos1->x;
        VertexShaderData->pos.y = vPos1->y;
        VertexShaderData->pos.z = vPos1->z;
        VertexShaderData->normal.x = vNormal1->x;
        VertexShaderData->normal.y = vNormal1->y;
        VertexShaderData->normal.z = vNormal1->z;
        VertexShaderData->color = d3dcolor;
        VertexShaderData->inkcoef.x = vertexThicknesses1 * auxCoef1;
        VertexShaderData->inkcoef.y =  vertexThicknesses1 * auxCoef2;
        VertexShaderData++;
    }
    else
    {
        RwReal auxCoef;

        auxCoef = (ink->farScale_factor) * (ink->perspectiveScale_a);

        VertexShaderData->pos.x = vPos0->x;
        VertexShaderData->pos.y = vPos0->y;
        VertexShaderData->pos.z = vPos0->z;
        VertexShaderData->normal.x = vNormal0->x;
        VertexShaderData->normal.y = vNormal0->y;
        VertexShaderData->normal.z = vNormal0->z;
        VertexShaderData->color = d3dcolor;
        VertexShaderData->inkcoef.x = 0.0f;
        VertexShaderData->inkcoef.y = vertexThicknesses0 * auxCoef;
        VertexShaderData++;

        VertexShaderData->pos.x = vPos1->x;
        VertexShaderData->pos.y = vPos1->y;
        VertexShaderData->pos.z = vPos1->z;
        VertexShaderData->normal.x = vNormal1->x;
        VertexShaderData->normal.y = vNormal1->y;
        VertexShaderData->normal.z = vNormal1->z;
        VertexShaderData->color = d3dcolor;
        VertexShaderData->inkcoef.x = 0.0f;
        VertexShaderData->inkcoef.y =  vertexThicknesses1 * auxCoef;
        VertexShaderData++;
    }

    NumVertices += 4;

    RWRETURNVOID();
}

void
_rpToonEdgeRenderVertexShaderSubmit(void)
{
    RWFUNCTION(RWSTRING("_rpToonEdgeRenderVertexShaderSubmit"));

    if (MaxBatch)
    {
        RwD3D8DynamicVertexBufferUnlock(VertexBuffer);
    }

    if (NumVertices == 0)
    {
        /* early out to avoid crashing driver */
        RWRETURNVOID();
    }

    RWASSERT(NumVertices <= MaxBatch);

    RwD3D8SetIndices(IndexBuffer, Offset);

    RwD3D8SetStreamSource(0, VertexBuffer, sizeof(EdgeVertexShader));

    RwD3D8DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
                               0, NumVertices,
                               0, (NumVertices / 4) * 6);


    RWRETURNVOID();
}

/****************************************************************************
 _rpToonEdgeRenderPrepareVertexShaderFlatSubmit
 */
void
_rpToonEdgeRenderPrepareVertexShaderFlatSubmit(RwUInt32 maxNumEdges,
                                               RwRGBA color)
{
    RwUInt32 inkColor;

    RWFUNCTION(RWSTRING("_rpToonEdgeRenderPrepareVertexShaderFlatSubmit"));

    /* Disable vertex alpha blending */
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)FALSE);

    /* Enable clipping */
    RwD3D8SetRenderState(D3DRS_CLIPPING, TRUE);

    inkColor = (((RwUInt32)color.alpha) << 24) |
                (((RwUInt32)color.red) << 16) |
                (((RwUInt32)color.green) << 8) |
                color.blue;

    RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, inkColor);

    /* Set color ops */
    RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
    RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);

    /* Set vertex shader */
    RwD3D8SetVertexShader(VertexShaderFlat);

    /* Disable any pixel shader */
    RwD3D8SetPixelShader(0);

    NumVertices = 0;

    MaxVertices = maxNumEdges * 4;

    if (MaxVertices > RPTOON_MAX_VERTICES_BATCH)
    {
        MaxBatch = RPTOON_MAX_VERTICES_BATCH;
    }
    else if (MaxVertices > 1000)
    {
        MaxBatch = ((maxNumEdges / 4) * 4); /* We need a multiple of for */
    }

    if (MaxBatch)
    {
        RwD3D8DynamicVertexBufferLock(sizeof(EdgeVertexShaderFlat),
                                      MaxBatch,
                                      (void**)&VertexBuffer,
                                      (RwUInt8 **)&VertexShaderFlatData,
                                      &Offset);
    }

    RWRETURNVOID();
}

void
_rpToonEdgeRenderAppendVertexShaderFlatQuad(const RwV3d *vPos0,
                                            const RwV3d *vPos1,
                                            const RwV3d *vNormal0,
                                            const RwV3d *vNormal1,
                                            RwReal vertexThicknesses0,
                                            RwReal vertexThicknesses1)
{
    RWFUNCTION( RWSTRING( "_rpToonEdgeRenderAppendVertexShaderFlatQuad" ) );

    RWASSERT(NumVertices <= MaxVertices);

    if (NumVertices >= MaxBatch)
    {
        _rpToonEdgeRenderVertexShaderFlatSubmit();

        NumVertices = 0;

        MaxVertices -= MaxBatch;

        MaxBatch = min(MaxVertices, RPTOON_MAX_VERTICES_BATCH);
        RWASSERT(MaxBatch > 0);

        RwD3D8DynamicVertexBufferLock(sizeof(EdgeVertexShaderFlat),
                                    MaxBatch,
                                    (void**)&VertexBuffer,
                                    (RwUInt8 **)&VertexShaderFlatData,
                                    &Offset);
    }

    VertexShaderFlatData->pos.x = vPos0->x;
    VertexShaderFlatData->pos.y = vPos0->y;
    VertexShaderFlatData->pos.z = vPos0->z;
    VertexShaderFlatData->normal.x = 0.0f;
    VertexShaderFlatData->normal.y = 0.0f;
    VertexShaderFlatData->normal.z = 1.0f;
    VertexShaderFlatData->thickness = 0.0f;
    VertexShaderFlatData++;

    VertexShaderFlatData->pos.x = vPos1->x;
    VertexShaderFlatData->pos.y = vPos1->y;
    VertexShaderFlatData->pos.z = vPos1->z;
    VertexShaderFlatData->normal.x = 0.0f;
    VertexShaderFlatData->normal.y = 0.0f;
    VertexShaderFlatData->normal.z = 1.0f;
    VertexShaderFlatData->thickness = 0.0f;
    VertexShaderFlatData++;

    VertexShaderFlatData->pos.x = vPos0->x;
    VertexShaderFlatData->pos.y = vPos0->y;
    VertexShaderFlatData->pos.z = vPos0->z;
    VertexShaderFlatData->normal.x = vNormal0->x;
    VertexShaderFlatData->normal.y = vNormal0->y;
    VertexShaderFlatData->normal.z = vNormal0->z;
    VertexShaderFlatData->thickness =  vertexThicknesses0;
    VertexShaderFlatData++;

    VertexShaderFlatData->pos.x = vPos1->x;
    VertexShaderFlatData->pos.y = vPos1->y;
    VertexShaderFlatData->pos.z = vPos1->z;
    VertexShaderFlatData->normal.x = vNormal1->x;
    VertexShaderFlatData->normal.y = vNormal1->y;
    VertexShaderFlatData->normal.z = vNormal1->z;
    VertexShaderFlatData->thickness =  vertexThicknesses1;
    VertexShaderFlatData++;

    NumVertices += 4;

    RWRETURNVOID();
}

void
_rpToonEdgeRenderVertexShaderFlatSubmit(void)
{
    RWFUNCTION(RWSTRING("_rpToonEdgeRenderVertexShaderFlatSubmit"));

    if (MaxBatch)
    {
        RwD3D8DynamicVertexBufferUnlock(VertexBuffer);
    }

    if (NumVertices == 0)
    {
        /* early out to avoid crashing driver */
        RWRETURNVOID();
    }

    RWASSERT(NumVertices <= MaxBatch);

    RwD3D8SetIndices(IndexBuffer, Offset);

    RwD3D8SetStreamSource(0, VertexBuffer, sizeof(EdgeVertexShaderFlat));

    RwD3D8DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
                               0, NumVertices,
                               0, (NumVertices / 4) * 6);


    RWRETURNVOID();
}
