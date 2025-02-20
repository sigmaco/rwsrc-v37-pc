#include <d3d8.h>

#include "rwcore.h"
#include "rpworld.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "toonink.h"
#include "toonpaint.h"
#include "toonmaterial.h"
#include "toongeo.h"
#include "toon.h"
#include "edgerender.h"
#include "brutesilhouette.h"
#include "crease.h"

#include "d3d8toon.h"

static
#include "toonsilhouettevertexshader.h"

/* #define TRYBACKFACEEXTRUDE */
#define RENDERLINES


#if (defined(__GNUC__) && defined(__cplusplus))
#define D3DMatrixInitMacro(_XX, _XY, _XZ, _XW,  \
                           _YX, _YY, _YZ, _YW,  \
                           _ZX, _ZY, _ZZ, _ZW,  \
                           _WX, _WY, _WZ, _WW ) \
  {                                             \
    { { {   (_XX), (_XY), (_XZ), (_XW) },       \
        {   (_YX), (_YY), (_YZ), (_YW) },       \
        {   (_ZX), (_ZY), (_ZZ), (_ZW) },       \
        {   (_WX), (_WY), (_WZ), (_WW) }        \
    } }                                         \
  }
#endif /* (defined(__GNUC__) && defined(__cplusplus)) */


#if (!defined(D3DMatrixInitMacro))
#define D3DMatrixInitMacro(_XX, _XY, _XZ, _XW,  \
                           _YX, _YY, _YZ, _YW,  \
                           _ZX, _ZY, _ZZ, _ZW,  \
                           _WX, _WY, _WZ, _WW ) \
  {                                             \
     (_XX), (_XY), (_XZ), (_XW),                \
     (_YX), (_YY), (_YZ), (_YW),                \
     (_ZX), (_ZY), (_ZZ), (_ZW),                \
     (_WX), (_WY), (_WZ), (_WW)                 \
  }
#endif /* (!defined(D3DMatrixInitMacro)) */

static RxPipeline   *D3D8ToonAtomicPipeline = NULL,
                    *D3D8ToonSectorPipeline = NULL;
static RxD3D8AllInOneRenderCallBack AtomicRenderPipeline = NULL;

static D3DMATRIX    D3D8CameraSpaceLightmatrix =
         D3DMatrixInitMacro(0.0f, 0.0f, 0.0f, 0.0f,
                            0.0f, 1.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 1.0f, 0.0f,
                            0.0f, 0.0f, 0.0f, 1.0f);

static RwBool   VideoCardSupportsMultitexture = TRUE;

static RwUInt32 FastSilhouetteVertexShader = 0;

static const RwUInt32 FastSilhouetteVertexDecl[] =
{
    D3DVSD_STREAM( 0 ),
    D3DVSD_REG( 0, D3DVSDT_FLOAT3 ),
    D3DVSD_REG( 1, D3DVSDT_FLOAT3 ),
    D3DVSD_END()
};

/**
 * \ingroup rptoond3d8
 * \page rptoond3d8optimized Optimized Silhouette Rendering
 *
 * We provide two toon silhouette rendering solutions for this platform.
 * The generic version supports a large number of features.
 * Many of these features have been removed in the optimized version and,
 * as a result, a great deal of extra optimisations can be made.
 * The features not supported in the optimized version are:
 * \li Crease edges.
 * \li Per vertex thickness.
 * \li An ink per edge.
 *
 * This is the limitation for the supported features:
 * \li The model \e does need average normals.
 *
 */

/****************************************************************************
 FindFirstGlobalLight
 */
static RpLight *
FindFirstGlobalLight(void)
{
    RwLLLink    *cur, *end;
    RpWorld     *world;

    RWFUNCTION(RWSTRING("FindFirstGlobalLight"));

    world = (RpWorld *)RWSRCGLOBAL(curWorld);

    cur = rwLinkListGetFirstLLLink(&world->directionalLightList);
    end = rwLinkListGetTerminator(&world->directionalLightList);
    while (cur != end)
    {
        RpLight *light;

        light = rwLLLinkGetData(cur, RpLight, inWorld);

        /* NB light may actually be a dummyTie from a enclosing ForAll */
        if (light && (rwObjectTestFlags(light, rpLIGHTLIGHTATOMICS)))
        {
            switch (RpLightGetType(light))
            {
                case rpLIGHTAMBIENT:
                    /* nothing */
                    break;

                case rpLIGHTDIRECTIONAL:
                    RWRETURN(light);
                    break;

                default:
                    RWASSERT(0); /* unsupported light type */
            }
        }

        /* Next */
        cur = rwLLLinkGetNext(cur);
    } /* while */

    RWRETURN(0);
}

static void
UpdateCameraSpaceLightMatrix(void)
{
    RWFUNCTION(RWSTRING("UpdateCameraSpaceLightMatrix"));

    if (NULL != RWSRCGLOBAL(curWorld))
    {
        RpLight *dirLight = FindFirstGlobalLight();
        //RWASSERT((dirLight != 0) && "No directional light found");
        if (dirLight)
        {
            /* Set the lights direction, in D3D's "camera space" to match D3DTSS_TCI_CAMERASPACENORMAL */
            RwV3d               *at, transLight;
            RwMatrix            inverseCamLTM;
            RwCamera            *camera;
            RwFrame             *cameraFrame;

            camera = RwCameraGetCurrentCamera();
            RWASSERT(0 != camera);

            cameraFrame = RwCameraGetFrame(camera);
            RWASSERT(0 != cameraFrame);

            at = RwMatrixGetAt(RwFrameGetLTM(RpLightGetFrame(dirLight)));

            RwMatrixInvert(&inverseCamLTM, RwFrameGetLTM(cameraFrame));

            RwV3dTransformVector (
                &transLight,
                at,
                &inverseCamLTM );

            D3D8CameraSpaceLightmatrix._11 = transLight.x;
            D3D8CameraSpaceLightmatrix._21 = -transLight.y;
            D3D8CameraSpaceLightmatrix._31 = -transLight.z;
        }
    } /* if world */

    RWRETURNVOID();
}

static void
FlatRender( const RxD3D8InstanceData *mesh,
            RwUInt32 flags,
            const RpToonPaint *paint __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("FlatRender"));

    /* use the material color or not depending on the flags */
    if (flags & rxGEOMETRY_MODULATE)
    {
        /* modulate against material color */
        const RwRGBA  *color = &mesh->material->color;
        D3DCOLOR    matColor;

        matColor = (((RwUInt32)color->alpha) << 24) |
                   (((RwUInt32)color->red) << 16) |
                   (((RwUInt32)color->green) << 8) |
                   color->blue;

        RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, matColor );
    }
    else
    {
        /* just modulate against opaque white (does nothing) */
        RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, 0xffffffff );
    }

    if ( (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2)) != 0 &&
        mesh->material->texture != NULL )
    {
        RwD3D8SetTexture(mesh->material->texture, 0);

        /* use ordinary UV set for texture 0 */
        RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0 );

        /* blend material color with texture color */
        RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
        RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

        RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
        RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
    }
    else
    {
        RwD3D8SetTexture(NULL, 0);

        /* just pick up material color */
        RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
        RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

        RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
        RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
    }

    if (mesh->indexBuffer != NULL)
    {
        RwD3D8SetIndices(mesh->indexBuffer, mesh->baseIndex);

        /* Draw the indexed primitive */
        RwD3D8DrawIndexedPrimitive((D3DPRIMITIVETYPE)mesh->primType,
                                                0, mesh->numVertices,
                                                0, mesh->numIndices);
    }
    else
    {
        RwD3D8DrawPrimitive((D3DPRIMITIVETYPE)mesh->primType,
                                                mesh->baseIndex,
                                                mesh->numVertices);
    }

    RWRETURNVOID();
}

static void
FlatClose(void)
{
    RWFUNCTION(RWSTRING("FlatClose"));

    RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    RWRETURNVOID();
}

static void
ToonShadeRender( const RxD3D8InstanceData *mesh,
                 RwUInt32 flags,
                 const RpToonPaint *paint )
{
    RwTexture   *baseTexture;

    /*
    Load up the gradient texture w/ transform to do a dot product of light direction with normals,
    texture addressing mode will perform the clamping for us.
    In the case where there's no light set, this matrix will pick a texel with uv coordinate (0,0)
    which should be the darkest shade for that texture.
    */

    RWFUNCTION(RWSTRING("ToonShadeRender"));

    /* use the base texture or not depending on the flags */
    if ((flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2)) != 0)
    {
        baseTexture = mesh->material->texture;
    }
    else
    {
        baseTexture = NULL;
    }

    /* use the material color or not depending on the flags */
    if (flags & rxGEOMETRY_MODULATE)
    {
        /* modulate against material color */
        const RwRGBA  *color = &mesh->material->color;
        D3DCOLOR    matColor;

        matColor = (((RwUInt32)color->alpha) << 24) |
                   (((RwUInt32)color->red) << 16) |
                   (((RwUInt32)color->green) << 8) |
                   color->blue;

        RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, matColor );
    }
    else
    {
        /* just modulate against opaque white (does nothing) */
        RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, 0xffffffff );
    }

    RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACENORMAL );
    RwD3D8SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2 );
    RwD3D8SetTransform( D3DTS_TEXTURE0 + 0, &D3D8CameraSpaceLightmatrix );

    RwD3D8SetTexture(paint->gradient, 0);

    /* blend material color with shade color */
    RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
    RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);

    /* it also has a base texture */
    if (baseTexture)
    {
        if (VideoCardSupportsMultitexture)
        {
            RwD3D8SetTexture(baseTexture, 1);

            RwD3D8SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 0 );

            RwD3D8SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

            RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
            RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        }
    }
    else
    {
        RwD3D8SetTexture(NULL, 1);
    }

    if (mesh->indexBuffer != NULL)
    {
        RwD3D8SetIndices(mesh->indexBuffer, mesh->baseIndex);

        /* Draw the indexed primitive */
        RwD3D8DrawIndexedPrimitive((D3DPRIMITIVETYPE)mesh->primType,
                                                0, mesh->numVertices,
                                                0, mesh->numIndices);
    }
    else
    {
        RwD3D8DrawPrimitive((D3DPRIMITIVETYPE)mesh->primType,
                                                mesh->baseIndex,
                                                mesh->numVertices);
    }

    if (!VideoCardSupportsMultitexture && baseTexture)
    {
        RwBlendFunction srcBlend, destBlend;
        RwBool  zWriteEnable;

        RwRenderStateGet(rwRENDERSTATESRCBLEND, (void *)&srcBlend);
        RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void *)&destBlend);

        RwD3D8GetRenderState(D3DRS_ZWRITEENABLE, (void *)&zWriteEnable);
        RwD3D8SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

        RwD3D8SetTexture(baseTexture, 0);

        RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
        RwD3D8SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);

        RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
        RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

        RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
        RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);

        _rwD3D8RenderStateVertexAlphaEnable(TRUE);
        _rwD3D8RenderStateSrcBlend(rwBLENDZERO);
        _rwD3D8RenderStateDestBlend(rwBLENDSRCCOLOR);

        if (mesh->indexBuffer != NULL)
        {
            /* Draw the indexed primitive */
            RwD3D8DrawIndexedPrimitive((D3DPRIMITIVETYPE)mesh->primType,
                                                    0, mesh->numVertices,
                                                    0, mesh->numIndices);
        }
        else
        {
            RwD3D8DrawPrimitive((D3DPRIMITIVETYPE)mesh->primType,
                                                    mesh->baseIndex,
                                                    mesh->numVertices);
        }

        _rwD3D8RenderStateVertexAlphaEnable(FALSE);
        _rwD3D8RenderStateSrcBlend(srcBlend);
        _rwD3D8RenderStateDestBlend(destBlend);

        RwD3D8SetRenderState(D3DRS_ZWRITEENABLE, zWriteEnable);
    }

    RWRETURNVOID();
}

static void
ToonShadeClose(void)
{
    RWFUNCTION(RWSTRING("ToonShadeClose"));

    /* turn off uv generation */
    RwD3D8SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );

    RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);

    RwD3D8SetTexture(NULL, 1);

    RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

    RwD3D8SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);

    RWRETURNVOID();
}

typedef void (*PaintRender)(const RxD3D8InstanceData *mesh,
                            RwUInt32 flags,
                            const RpToonPaint *paint);

typedef void (*PaintClose)(void);

static const PaintRender _paintRender[RPTOON_PAINTTYPE_COUNT] =
{
    FlatRender,
    ToonShadeRender
};

static const PaintClose _paintClose[RPTOON_PAINTTYPE_COUNT] =
{
    FlatClose,
    ToonShadeClose
};

static RwV3d *_rpToonD3D8TempVertexPositions = 0;


static void
_rpToonD3D8UpdateVertexInfoFromVertexBuffer( RpToonGeo *toonGeo, RwResEntry *repEntry )
{
    /*
    The D3D8 Atomic pipe has just done some skinning/morphing into a vertex buffer.
    Lock it, and copy it into a format the line renders like.
    Yes, I know this is crap.
    Remove it at the first opportunity, e.g. with a vertex shader line renderer,
    or even by splitting the positions & normals into separate streams of RwV3ds
    (probably would make the behaviour of the skinner/morpher more write combining cache friendly as a bonus.)
    or meddle with the line render code to take a stream of positions & normals with a stride
    (will be a nuisance and possibly slow as well.)
    */
    RxD3D8ResEntryHeader    *resEntryHeader;
    RxD3D8InstanceData      *instancedData;
    RwInt32                 numMeshes;

    RWFUNCTION( RWSTRING( "_rpToonD3D8UpdateVertexInfoFromVertexBuffer" ) );

    resEntryHeader = (RxD3D8ResEntryHeader *)(repEntry + 1);
    numMeshes = resEntryHeader->numMeshes;
    instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);
    while (numMeshes--)
    {
        if (instancedData->numVertices)
        {
            RwInt32     stride;
            RwUInt8     *vertexData;
            RwV3d       *position;
            RwV3d       *normal;

            stride = instancedData->stride;

            position = &_rpToonD3D8TempVertexPositions[instancedData->minVert];

            normal = &toonGeo->extrusionVertexNormals[instancedData->minVert];

            if (SUCCEEDED(IDirect3DVertexBuffer8_Lock(
                            (LPDIRECT3DVERTEXBUFFER8)instancedData->vertexBuffer,
                            instancedData->baseIndex * instancedData->stride,
                            instancedData->numVertices * instancedData->stride,
                            &vertexData,
                            D3DLOCK_NOSYSLOCK)))
            {
                const RwUInt8   *data;
                RwUInt32        numVertices;

                data = vertexData;
                numVertices = instancedData->numVertices;

                do
                {
                    *position = *((const RwV3d *)data);
                    *normal = *(((const RwV3d *)data) + 1);
                    data += stride;
                    position++;
                    normal++;
                }
                while (--numVertices);

                /* Unlock the vertex buffer */
                IDirect3DVertexBuffer8_Unlock(
                        (LPDIRECT3DVERTEXBUFFER8)instancedData->vertexBuffer);
            }
        }

        instancedData++;
    }

    RWRETURNVOID();
}

/****************************************************************************
 D3D8MatrixTranspose
 */
static void
D3D8MatrixTranspose(D3DMATRIX *dstMat,
                    const D3DMATRIX *srcmat)
{
    RWFUNCTION(RWSTRING("D3D8MatrixTranspose"));
    RWASSERT(dstMat != NULL);
    RWASSERT(srcmat != NULL);

    /* Multiply out right */
    dstMat->m[0][0] = srcmat->m[0][0];
    dstMat->m[1][0] = srcmat->m[0][1];
    dstMat->m[2][0] = srcmat->m[0][2];
    dstMat->m[3][0] = srcmat->m[0][3];

    /* Then up */
    dstMat->m[0][1] = srcmat->m[1][0];
    dstMat->m[1][1] = srcmat->m[1][1];
    dstMat->m[2][1] = srcmat->m[1][2];
    dstMat->m[3][1] = srcmat->m[1][3];

    /* Then at */
    dstMat->m[0][2] = srcmat->m[2][0];
    dstMat->m[1][2] = srcmat->m[2][1];
    dstMat->m[2][2] = srcmat->m[2][2];
    dstMat->m[3][2] = srcmat->m[2][3];

    /* Then pos */
    dstMat->m[0][3] = srcmat->m[3][0];
    dstMat->m[1][3] = srcmat->m[3][1];
    dstMat->m[2][3] = srcmat->m[3][2];
    dstMat->m[3][3] = srcmat->m[3][3];

    /* And that's all folks */
    RWRETURNVOID();
}

/****************************************************************************
 GetProjectionTransposedMatrix

*/
static void
GetProjectionTransposedMatrix(void *projectionMatrix)
{
    D3DMATRIX projection;

    RWFUNCTION(RWSTRING("GetProjectionTransposedMatrix"));

    RwD3D8GetTransform(D3DTS_PROJECTION, &projection);

    D3D8MatrixTranspose(projectionMatrix, &projection);

    RWRETURNVOID();
}

/****************************************************************************
 GetWorldViewTransposedMatrix

*/
static void
GetWorldViewTransposedMatrix(void *worldViewMatrix,
                             const RwMatrix *transform)
{
    D3DMATRIX view;

    RWFUNCTION(RWSTRING("GetWorldViewTransposedMatrix"));

    RwD3D8GetTransform(D3DTS_VIEW, &view);

    if (transform != NULL)
    {
        D3DMATRIX worldView;

        RwMatrixMultiply((RwMatrix *)&worldView, transform, (const RwMatrix *)&view);

        worldView._14 = 0.0f;
        worldView._24 = 0.0f;
        worldView._34 = 0.0f;
        worldView._44 = 1.0f;

        D3D8MatrixTranspose(worldViewMatrix, &worldView);
    }
    else
    {
        D3D8MatrixTranspose(worldViewMatrix, &view);
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpToonD3D8RenderExtrudedSilhouette

*/
static void
_rpToonD3D8RenderExtrudedSilhouette(RpToonGeo *toonGeo,
                                    RwResEntry *repEntry,
                                    RwMatrix *ltmMatrix)
{
    const RxD3D8ResEntryHeader  *resEntryHeader;
    const RxD3D8InstanceData    *mesh;
    RwInt32                 numMeshes;
    RwV4d matrixConstants[8];
    const RpToonInk *lastInk = NULL;
    RwCullMode  oldCullMode;
    RwUInt32  oldZFunc;
    RwUInt32 inkColor;

    RWFUNCTION(RWSTRING("_rpToonD3D8RenderExtrudedSilhouette"));

    GetProjectionTransposedMatrix(matrixConstants + 0);
    GetWorldViewTransposedMatrix(matrixConstants + 4, ltmMatrix);

    RwD3D8SetVertexShaderConstant(0, matrixConstants, 7);

    RwD3D8GetRenderState(D3DRS_ZFUNC, &oldZFunc);
    RwD3D8SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);

    RwRenderStateGet(rwRENDERSTATECULLMODE, (void*)&oldCullMode);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLFRONT);

    /* Disable vertex alpha blending */
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)FALSE);

    /* Remove any texture */
    RwD3D8SetTexture(NULL, 0);

    /* Set color ops */
    RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
    RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
    RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);

    /* Set vertex shader */
    RwD3D8SetVertexShader(FastSilhouetteVertexShader);

    /* Disable any pixel shader */
    RwD3D8SetPixelShader(0);

    /* render each mesh */
    resEntryHeader = (const RxD3D8ResEntryHeader *)(repEntry + 1);
    numMeshes = resEntryHeader->numMeshes;
    mesh = (const RxD3D8InstanceData *)(resEntryHeader + 1);
    while (numMeshes--)
    {
        const RpToonMaterial *toonMat;
        const RpToonInk *currentInk = NULL;

        toonMat = *RPTOONMATERIALGETCONSTDATA(mesh->material);

        if (toonMat)
        {
            currentInk = toonGeo->inks[toonMat->silhouetteInkID];
        }
        else
        {
            currentInk = toonGeo->inks[toonGeo->edgeInkIDs[0].inkId[RPTOON_INKTYPE_SILHOUETTE]];
        }

        if (currentInk != NULL &&
            currentInk->farScale_factor != 0.0f)
        {
            if (lastInk != currentInk)
            {
                lastInk = currentInk;

                inkColor = (((RwUInt32)currentInk->color.alpha) << 24) |
                            (((RwUInt32)currentInk->color.red) << 16) |
                            (((RwUInt32)currentInk->color.green) << 8) |
                            currentInk->color.blue;

                RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, inkColor);

                matrixConstants[7].x = (currentInk->farScale_factor) *
                                    (currentInk->perspectiveScale_b);

                matrixConstants[7].y = (currentInk->farScale_factor) *
                                    ((currentInk->perspectiveScale_a) -
                                        (currentInk->perspectiveScale_b) * (currentInk->perspectiveScale_zMin));

                matrixConstants[7].z = 0.0f;
                matrixConstants[7].w = 1.0f;

                RwD3D8SetVertexShaderConstant(7, matrixConstants + 7, 1);
            }

            RwD3D8SetStreamSource(0, mesh->vertexBuffer, mesh->stride);

            if (mesh->indexBuffer != NULL)
            {
                RwD3D8SetIndices(mesh->indexBuffer, mesh->baseIndex);

                /* Draw the indexed primitive */
                RwD3D8DrawIndexedPrimitive((D3DPRIMITIVETYPE)mesh->primType,
                                                        0, mesh->numVertices,
                                                        0, mesh->numIndices);
            }
            else
            {
                RwD3D8DrawPrimitive((D3DPRIMITIVETYPE)mesh->primType,
                                                        mesh->baseIndex,
                                                        mesh->numVertices);
            }
        }

        mesh++;
    }

    RwRenderStateSet(rwRENDERSTATECULLMODE, (void *)oldCullMode);

    RwD3D8SetRenderState(D3DRS_ZFUNC, oldZFunc);

    RWRETURNVOID();
}

void
_rpToonD3D8RenderCallback(RwResEntry *repEntry,
                             void *object,
                             RwUInt8 type,
                             RwUInt32 flags)
{
    const RxD3D8ResEntryHeader  *resEntryHeader;
    const RxD3D8InstanceData    *mesh;
    RwInt32                 numMeshes;
    RpToonGeo               *toonGeo;
    RwCamera                *cam;
    RpToonPaintType         lastPaintType = RPTOON_PAINTTYPE_COUNT;

    RWFUNCTION(RWSTRING("_rpToonD3D8RenderCallback"));

    /* Get current camera */
    cam = RwCameraGetCurrentCamera();
    RWASSERT(cam);

    /* grab the toon geometry info because it stores paint info we need */
    if (type == rpATOMIC)
    {
        RpAtomic    *atomic = (RpAtomic *)object;
        RpGeometry *g;

        g = RpAtomicGetGeometry(atomic);
        RWASSERT((g != NULL) && "atomic can't have null geometry!");

        toonGeo = RpToonGeometryGetToonGeo(g);
        RWASSERT((toonGeo != NULL) && "atomic can't have null toon geometry!");

        /* Check clipping */
        if (RwD3D8CameraIsSphereFullyInsideFrustum(cam, RpAtomicGetWorldBoundingSphere(atomic)))
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
        RpWorldSector *sector = (RpWorldSector *)object;

        RWASSERT(sector != NULL);

        toonGeo = RpToonWorldSectorGetToonGeo(sector);
        RWASSERT((toonGeo != NULL) && "sector can't have null toon geometry!");

        /* Check clipping */
        if (RwD3D8CameraIsBBoxFullyInsideFrustum(cam, RpWorldSectorGetTightBBox(sector)))
        {
            RwD3D8SetRenderState(D3DRS_CLIPPING, FALSE);
        }
        else
        {
            RwD3D8SetRenderState(D3DRS_CLIPPING, TRUE);
        }
    }

    RWASSERT((toonGeo->paint != 0) && "must have a paint!");

    /* Update camera space light matrix if needed */
    if (flags & rxGEOMETRY_LIGHT)
    {
        UpdateCameraSpaceLightMatrix();
    }

    /* Default pixel shader */
    RwD3D8SetPixelShader(0);

    /* render each mesh */
    resEntryHeader = (const RxD3D8ResEntryHeader *)(repEntry + 1);
    numMeshes = resEntryHeader->numMeshes;
    mesh = (const RxD3D8InstanceData *)(resEntryHeader + 1);
    while (numMeshes--)
    {
        RpToonPaint     *thePaint;
        RpToonPaintType thePaintType;

        const RpToonMaterial *toonMat = *RPTOONMATERIALGETCONSTDATA( mesh->material );

        if (mesh->vertexAlpha ||
           (0xFF != mesh->material->color.alpha))
        {
           _rwD3D8RenderStateVertexAlphaEnable(TRUE);
        }
        else
        {
           _rwD3D8RenderStateVertexAlphaEnable(FALSE);
        }

        /* use the toon material paint override if there is one */

        if (toonMat && toonMat->overrideGeometryPaint && toonMat->paint)
        {
           thePaint = toonMat->paint;
        }
        /* otherwise use the geometry paint */
        else
        {
           thePaint = toonGeo->paint;
        }

        /*
        Probably just the vertex format unless the user set a programmable one,
        in which case there's a bunch of FFP material renderstate setting which is going to waste
        */
        RwD3D8SetVertexShader(mesh->vertexShader);

        RwD3D8SetStreamSource(0, mesh->vertexBuffer, mesh->stride);

        /* use the paint's paint type unless lightings turned off, in which case force flat */
        thePaintType = thePaint->type;

        if ( (flags & rxGEOMETRY_LIGHT) == 0 ||
             (thePaintType == RPTOON_PAINTTYPE_TOONSHADE &&
              thePaint->gradient == 0) )
        {
            thePaintType = RPTOON_PAINTTYPE_FLAT;
        }

        if (lastPaintType != thePaintType)
        {
            if (lastPaintType != RPTOON_PAINTTYPE_COUNT)
            {
                (_paintClose[lastPaintType])();
            }

            lastPaintType = thePaintType;
        }

        (_paintRender[thePaintType])(mesh, flags, thePaint);

        mesh++;
    }

    if (lastPaintType != RPTOON_PAINTTYPE_COUNT)
    {
        (_paintClose[lastPaintType])();
    }

#ifdef TRYBACKFACEEXTRUDE
    _rpToonD3D8BackfaceExtrudeRender(repEntry,object,type,flags,toonGeo);
#endif

    RWRETURNVOID();
}

static RxPipeline *
_rpToonAtomicPipelineCreate(void)
{
    RxPipeline  *pipe;

    RWFUNCTION(RWSTRING("_rpToonAtomicPipelineCreate"));

    pipe = RxPipelineCreate();
    if (pipe)
    {
        RxLockedPipe    *lpipe;

        pipe->pluginId = rwID_TOONPLUGIN;

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

static RxPipeline *
_rpToonSectorPipelineCreate(void)
{
    RxPipeline  *pipe;

    RWFUNCTION(RWSTRING("_rpToonSectorPipelineCreate"));

    pipe = RxPipelineCreate();
    if (pipe)
    {
        RxLockedPipe    *lpipe;

        pipe->pluginId = rwID_TOONPLUGIN;

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
_rpToonPipelinesCreate(void)
{
    RxNodeDefinition    *instanceNode;
    RxPipelineNode      *node;
    const D3DCAPS8      *d3dCaps;

    RWFUNCTION(RWSTRING("_rpToonPipelinesCreate"));

    /*
     * Create a new atomic pipeline
     */
    D3D8ToonAtomicPipeline = _rpToonAtomicPipelineCreate();
    if (!D3D8ToonAtomicPipeline)
    {
        RWRETURN(FALSE);
    }

    instanceNode = RxNodeDefinitionGetD3D8AtomicAllInOne();
    RWASSERT(NULL != instanceNode);

    node = RxPipelineFindNodeByName(D3D8ToonAtomicPipeline, instanceNode->name, NULL, NULL);
    RWASSERT(NULL != node);

    /*
     * Set the Tooning object pipeline
     */
 //   _rxD3D8AllInOneSetInstanceCallBack(node, _rpToonD3D8InstanceCallback);

    /*
     * Morph targets will completely destroy our Toonning instance data if we don't overload this!!!
     */
 //   _rxD3D8AllInOneSetReinstanceCallBack(node, _rpToonD3D8ReInstanceCallback);

    /* Set Lighting callback - just turn it off for now to prevent hw lighting being set up */
    RxD3D8AllInOneSetLightingCallBack(node, 0);

    RxD3D8AllInOneSetRenderCallBack(node, _rpToonD3D8RenderCallback);

    /*
     * Get the default object pipeline
     */
    AtomicRenderPipeline = RxD3D8AllInOneGetRenderCallBack(node);


    /*
     * Create a new world sector pipeline:
     */
    D3D8ToonSectorPipeline = _rpToonSectorPipelineCreate();
    RWASSERT(NULL != D3D8ToonSectorPipeline);

    instanceNode = RxNodeDefinitionGetD3D8WorldSectorAllInOne();
    RWASSERT(NULL != instanceNode);

    node = RxPipelineFindNodeByName(D3D8ToonSectorPipeline, instanceNode->name, NULL, NULL);
    RWASSERT(NULL != node);

    RxD3D8AllInOneSetRenderCallBack(node, _rpToonD3D8RenderCallback);

    RxD3D8AllInOneSetLightingCallBack(node, 0);


    /*
     * Initialize edge renderer
     */
    _rpToonEdgeRenderInit();
    _rpToonBruteSilhouetteInit();

    _rpToonD3D8TempVertexPositions = (RwV3d *)RwMalloc(sizeof(RwV3d) * RPTOON_MAX_VERTICES,
                                                   rwID_TOONPLUGIN | rwMEMHINTDUR_GLOBAL);
    if (!_rpToonD3D8TempVertexPositions)
    {
        RWRETURN(FALSE);
    }

#ifdef TRYBACKFACEEXTRUDE
    _rpToonD3D8BackfaceExtrudeInit();
#endif

    /* Get video card info */
    d3dCaps = (const D3DCAPS8 *)RwD3D8GetCaps();

    VideoCardSupportsMultitexture = (d3dCaps->MaxSimultaneousTextures > 1);

    if ((d3dCaps->VertexShaderVersion & 0xFFFF) >= 0x0101)
    {
        RwD3D8CreateVertexShader(FastSilhouetteVertexDecl, dwToonSilhouetteVertexShader, &FastSilhouetteVertexShader, 0);
    }

    RWRETURN(TRUE);
}

RwBool
_rpToonPipelinesDestroy(void)
{
    RWFUNCTION(RWSTRING("_rpToonPipelinesDestroy"));

    if (FastSilhouetteVertexShader != 0)
    {
        RwD3D8DeleteVertexShader(FastSilhouetteVertexShader);

        FastSilhouetteVertexShader = 0;
    }

    RxPipelineDestroy(D3D8ToonAtomicPipeline);
    D3D8ToonAtomicPipeline = NULL;

    RxPipelineDestroy(D3D8ToonSectorPipeline);
    D3D8ToonSectorPipeline = NULL;

    RwFree(_rpToonD3D8TempVertexPositions);

    _rpToonBruteSilhouetteClean();
    _rpToonEdgeRenderClean();

#ifdef TRYBACKFACEEXTRUDE
    _rpToonD3D8BackfaceExtrudeDestroy();
#endif

    RWRETURN(TRUE);
}

RxPipeline *
_rpToonD3D8GetPipeline( void )
{
    RWFUNCTION(RWSTRING("_rpToonD3D8GetPipeline"));
    RWRETURN(D3D8ToonAtomicPipeline);
}

static RwBool
IsMorphing( RpGeometry *g )
{
    RWFUNCTION( RWSTRING( "IsMorphing" ) );

    RWRETURN( RpGeometryGetNumMorphTargets(g) > 1 );
}

static RpAtomic *
ToonAtomicRender( RpAtomic *atomic )
{
    RpToonGeo *toonGeo;
    RwV3d *verts = 0, *normals = 0;
    RwBool faceNormalsInvalid = FALSE;
    RpGeometry *g;
    RwMatrix *lineTransform;

    RWFUNCTION(RWSTRING("ToonAtomicRender"));

    g = RpAtomicGetGeometry(atomic);

    toonGeo = RpToonGeometryGetToonGeo(g);

    /* cell shade set up paint render with custom pipeline */
    (*(toonGeo->defAtomicRenderCallback))(atomic);

    normals = toonGeo->extrusionVertexNormals;

    if (IsMorphing(g) || toonGeo->isSkinned)
    {
        RwResEntry *repEntry;

        /*
        If the mesh was dynamic, update some info for the line renderers.
        now this is gonna be ugly and inefficient, but we can be clever later....
        */
        RWASSERT( (rpGEOMETRYNORMALS & RpGeometryGetFlags(g)) && "skinned/morphed geometry must have normals for silhouette rendering");

        /* If the geometry has more than one morph target the resEntry in the
        * atomic is used else the resEntry in the geometry */
        if (g->numMorphTargets != 1)
        {
            repEntry = atomic->repEntry;
        }
        else
        {
            repEntry = g->repEntry;
        }

        /*
        If the mesh was dynamic, update some info for the line renderers.
        now this is gonna be ugly and inefficient, but we can be clever later....
        */
        RWASSERT( (rpGEOMETRYNORMALS & RpGeometryGetFlags(g)) && "skinned/morphed geometry must have normals for silhouette rendering");

        _rpToonD3D8UpdateVertexInfoFromVertexBuffer(toonGeo, repEntry);

        faceNormalsInvalid = TRUE;

        verts = _rpToonD3D8TempVertexPositions;
    }
    else
    {
        /* just static data? */
        RpMorphTarget *morph;
        morph = RpGeometryGetMorphTarget(g,0);
        RWASSERT(morph != 0);
        verts = RpMorphTargetGetVertices( morph );

        /* maybe dmorph or some other thing going on in the render callback chain */
        if (toonGeo->vertexPositionsDirty)
        {
            faceNormalsInvalid = TRUE;

            if (RpMorphTargetGetVertexNormals( morph ))
            {
                normals = RpMorphTargetGetVertexNormals( morph );
            }

            toonGeo->vertexPositionsDirty = FALSE;
        }
    }

    _rpToonUpdateAtomicPerspectiveScale(toonGeo, atomic );

    /* need to transform stuff into world space */
    lineTransform = RwFrameGetLTM(RpAtomicGetFrame(atomic));

#ifdef RENDERLINES
    /* edge detection & render */
    {
        ScreenSpaceProjection   ssp;

        GetScreenSpaceProjection(&ssp, lineTransform);

        _rpToonEdgeRenderPushState();
        _rpToonBruteSilhouetteRender( toonGeo, verts, normals, faceNormalsInvalid, lineTransform, &ssp );
        _rpToonCreaseRender( toonGeo, verts, &ssp );
        _rpToonEdgeRenderPopState();
    }
#endif

    RWRETURN(atomic);
}

/**
 * \ingroup rptoond3d8
 * \ref RpD3D8ToonFastSilhouetteAtomicRenderCallback is the render callback that needs
 * to be set in order to enable the optimized silhouette rendering on an atomic.
 *
 * \param atomic Pointer to the \ref RpAtomic to be rendered.
 *
 * \return Returns a pointer to the atomic if successful or NULL if there is an error.
 *
 * \see RpAtomicSetRenderCallBack
 * \see RpD3D8ToonFastSilhouetteWorldSectorRenderCallback
 */
RpAtomic *
RpD3D8ToonFastSilhouetteAtomicRenderCallback( RpAtomic *atomic )
{
    RpToonGeo *toonGeo;
    RpGeometry *g;

    RWAPIFUNCTION(RWSTRING("RpD3D8ToonFastSilhouetteAtomicRenderCallback"));

    g = RpAtomicGetGeometry(atomic);

    toonGeo = RpToonGeometryGetToonGeo(g);

    /* cell shade set up paint render with custom pipeline */
    AtomicDefaultRenderCallBack(atomic);

    #ifdef RENDERLINES
    if (FastSilhouetteVertexShader)
    {
        RwResEntry  *repEntry;
        RwMatrix    *ltmMatrix;

        /* If the geometry has more than one morph target the resEntry in the
        * atomic is used else the resEntry in the geometry */
        if (g->numMorphTargets != 1)
        {
            repEntry = atomic->repEntry;
        }
        else
        {
            repEntry = g->repEntry;
        }

        ltmMatrix = RwFrameGetLTM(RpAtomicGetFrame(atomic));

        RwD3D8SetRenderState(D3DRS_CLIPPING, TRUE);

        _rpToonUpdateAtomicPerspectiveScale(toonGeo, atomic);

        _rpToonD3D8RenderExtrudedSilhouette(toonGeo, repEntry, ltmMatrix);
    }
    else
    {
        RwV3d *verts = 0, *normals = 0;
        RwBool faceNormalsInvalid = FALSE;
        RwMatrix *lineTransform;

        normals = toonGeo->extrusionVertexNormals;

        if (IsMorphing(g) || toonGeo->isSkinned)
        {
            RwResEntry *repEntry;

            /*
            If the mesh was dynamic, update some info for the line renderers.
            now this is gonna be ugly and inefficient, but we can be clever later....
            */
            RWASSERT( (rpGEOMETRYNORMALS & RpGeometryGetFlags(g)) && "skinned/morphed geometry must have normals for silhouette rendering");

            /* If the geometry has more than one morph target the resEntry in the
            * atomic is used else the resEntry in the geometry */
            if (g->numMorphTargets != 1)
            {
                repEntry = atomic->repEntry;
            }
            else
            {
                repEntry = g->repEntry;
            }

            /*
            If the mesh was dynamic, update some info for the line renderers.
            now this is gonna be ugly and inefficient, but we can be clever later....
            */
            RWASSERT( (rpGEOMETRYNORMALS & RpGeometryGetFlags(g)) && "skinned/morphed geometry must have normals for silhouette rendering");

            _rpToonD3D8UpdateVertexInfoFromVertexBuffer(toonGeo, repEntry);

            faceNormalsInvalid = TRUE;

            verts = _rpToonD3D8TempVertexPositions;
        }
        else
        {
            /* just static data? */
            RpMorphTarget *morph;
            morph = RpGeometryGetMorphTarget(g,0);
            RWASSERT(morph != 0);
            verts = RpMorphTargetGetVertices( morph );

            /* maybe dmorph or some other thing going on in the render callback chain */
            if (toonGeo->vertexPositionsDirty)
            {
                faceNormalsInvalid = TRUE;

                if (RpMorphTargetGetVertexNormals( morph ))
                {
                    normals = RpMorphTargetGetVertexNormals( morph );
                }

                toonGeo->vertexPositionsDirty = FALSE;
            }
        }

        _rpToonUpdateAtomicPerspectiveScale(toonGeo, atomic );

        /* need to transform stuff into world space */
        lineTransform = RwFrameGetLTM(RpAtomicGetFrame(atomic));

        /* edge detection & render */
        {
            ScreenSpaceProjection   ssp;

            GetScreenSpaceProjection(&ssp, lineTransform);

            _rpToonEdgeRenderPushState();
            _rpToonBruteSilhouetteRender( toonGeo, verts, normals, faceNormalsInvalid, lineTransform, &ssp );
            _rpToonEdgeRenderPopState();
        }
    }
    #endif

    RWRETURN(atomic);
}

void
_rpToonAtomicChainAtomicRenderCallback(RpAtomic *atomic)
{
    RpToonGeo *toonGeo;

    RWFUNCTION(RWSTRING("_rpToonAtomicChainAtomicRenderCallback"));

    RWASSERT(atomic);

    /*
    Don't chain umpteem million times even if the stupid skinning streaming
    code calls us umpteem million times.
    */
    toonGeo = RpToonGeometryGetToonGeo(RpAtomicGetGeometry(atomic));

    RWASSERT(toonGeo);

    if (RpAtomicGetRenderCallBack(atomic) != ToonAtomicRender)
    {
        if (RpAtomicGetRenderCallBack(atomic) != RpD3D8ToonFastSilhouetteAtomicRenderCallback)
        {
            toonGeo->defAtomicRenderCallback = RpAtomicGetRenderCallBack(atomic);
        }
        else
        {
            toonGeo->defAtomicRenderCallback = AtomicDefaultRenderCallBack;
        }

        toonGeo->defSectorRenderCallback = NULL;

        RpAtomicSetRenderCallBack(atomic, ToonAtomicRender);
    }

    RWRETURNVOID();
}

void
_rpToonAtomicChainSkinnedAtomicRenderCallback(RpAtomic *atomic)
{
    RpToonGeo *toonGeo;

    RWFUNCTION(RWSTRING("_rpToonAtomicChainSkinnedAtomicRenderCallback"));

    toonGeo = RpToonGeometryGetToonGeo(RpAtomicGetGeometry(atomic));

    RWASSERT(toonGeo);

    toonGeo->isSkinned = TRUE;

    _rpToonAtomicChainAtomicRenderCallback(atomic);

    RWRETURNVOID();
}

RwBool
_rpToonAtomicPipelinesAttach(RpAtomic *atomic)
{
    RxPipeline *pipeline;
    RpAtomic *success;

    RWFUNCTION(RWSTRING("_rpToonAtomicPipelinesAttach"));

    pipeline = D3D8ToonAtomicPipeline;
    RWASSERT(NULL != pipeline);

    success = RpAtomicSetPipeline(atomic, pipeline);

    _rpToonAtomicChainAtomicRenderCallback(atomic);

    RWRETURN(success!=0);
}

static RpWorldSector *
ToonSectorRender( RpWorldSector *sector )
{
    RpToonGeo *toonGeo;

    RWFUNCTION(RWSTRING("ToonSectorRender"));

    toonGeo = RpToonWorldSectorGetToonGeo(sector);

    /* cell shade set up paint render with custom pipeline */
    (*(toonGeo->defSectorRenderCallback))(sector);

    /* sector might be a space filling one */
    if (toonGeo->numVerts)
    {
        /* edge detection & render */
        RWASSERT(sector->vertices && "sector has no vertices!");

        _rpToonUpdateWorldPerspectiveScale(toonGeo);

#ifdef RENDERLINES
        {
            ScreenSpaceProjection   ssp;

            GetScreenSpaceProjection(&ssp, NULL);

            _rpToonEdgeRenderPushState();
            _rpToonBruteSilhouetteRender( toonGeo, sector->vertices, toonGeo->extrusionVertexNormals, FALSE, NULL, &ssp );
            _rpToonCreaseRender( toonGeo, sector->vertices, &ssp );
            _rpToonEdgeRenderPopState();
        }
#endif
    }

    RWRETURN(sector);
}

/**
 * \ingroup rptoond3d8
 * \ref RpD3D8ToonFastSilhouetteWorldSectorRenderCallback is the render callback that needs
 * to be set in order to enable the optimized silhouette rendering on a world sector.
 *
 * \param sector Pointer to the \ref RpWorldSector to be rendered.
 *
 * \return Returns a pointer to the world sector if successful or NULL if there is an error.
 *
 * \see RpWorldSetSectorRenderCallBack
 * \see RpD3D8ToonFastSilhouetteAtomicRenderCallback
 */
RpWorldSector *
RpD3D8ToonFastSilhouetteWorldSectorRenderCallback( RpWorldSector *sector )
{
    RpToonGeo *toonGeo;

    RWAPIFUNCTION(RWSTRING("RpD3D8ToonFastSilhouetteWorldSectorRenderCallback"));

    /* cell shade set up paint render with custom pipeline */
    _rpSectorDefaultRenderCallBack(sector);

    toonGeo = RpToonWorldSectorGetToonGeo(sector);

    /* sector might be a space filling one */
#ifdef RENDERLINES
    if (toonGeo->numVerts)
    {
        /* edge detection & render */
        RWASSERT(sector->vertices && "sector has no vertices!");

        _rpToonUpdateWorldPerspectiveScale(toonGeo);

        if (FastSilhouetteVertexShader)
        {
            RwD3D8SetRenderState(D3DRS_CLIPPING, TRUE);

            _rpToonD3D8RenderExtrudedSilhouette(toonGeo, sector->repEntry, NULL);
        }
        else
        {
            ScreenSpaceProjection   ssp;

            GetScreenSpaceProjection(&ssp, NULL);

            _rpToonEdgeRenderPushState();
            _rpToonBruteSilhouetteRender( toonGeo, sector->vertices, toonGeo->extrusionVertexNormals, FALSE, NULL, &ssp );
            _rpToonEdgeRenderPopState();
        }
    }
#endif

    RWRETURN(sector);
}

RwBool
_rpToonSectorPipelinesAttach(RpWorldSector *sector)
{
    RxPipeline *pipeline;
    RpWorldSector *success;
    RpWorld *world;
    RpToonGeo *toonGeo;
    static RpWorld *lastWorld = 0;
    static RpWorldSectorCallBackRender defSectorRenderCallback = 0;

    RWFUNCTION(RWSTRING("_rpToonSectorPipelinesAttach"));

    pipeline = D3D8ToonSectorPipeline;
    RWASSERT(NULL != pipeline);

    RWASSERT(sector != NULL);

    success = RpWorldSectorSetPipeline(sector, pipeline);

    toonGeo = RpToonWorldSectorGetToonGeo(sector);

    world = RpWorldSectorGetWorld(sector);
    RWASSERT(world != NULL);

    if (!lastWorld)
    {
        lastWorld = world;
        defSectorRenderCallback = RpWorldGetSectorRenderCallBack(world);
    }

    RWASSERT((lastWorld == world) && "multiple worlds TODO");

/*    if (ToonSectorRender != RpWorldGetSectorRenderCallBack(world)) */
    {
        toonGeo->defSectorRenderCallback = defSectorRenderCallback; /* RpWorldGetSectorRenderCallBack(world); */

        RpWorldSetSectorRenderCallBack(world, ToonSectorRender);
    }

    toonGeo->defAtomicRenderCallback = NULL;

    RWRETURN(success!=0);
}
