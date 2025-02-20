/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <d3d9.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpskin.h"

#include "skin.h"

#include "x86matbl.h"

#if !defined(NOSSEASM)
#include "ssematbl.h"
#endif

#include "skind3d9generic.h"
#include "skind3d9instance.h"

extern RwBool _rwD3D9RenderStateVertexAlphaEnable(RwBool enable);

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/

#define FLOATASINT(f) (*((const RwInt32 *)&(f)))

#define RENDERFLAGS_HAS_TEXTURE  1
#define RENDERFLAGS_HAS_TFACTOR  2

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/

SkinGlobals _rpSkinGlobals =
{
    0,
    0,
    0,
    { (RwMatrix *)NULL, NULL },
    0,
    (RwFreeList *)NULL,
    { 0, 0 },
    {                           /* SkinGlobalPlatform  platform    */
        {
            (RxPipeline *)NULL, /*  rpSKIND3D9PIPELINEGENERIC */
            (RxPipeline *)NULL, /*  rpSKIND3D9PIPELINEMATFX   */
            (RxPipeline *)NULL, /*  rpSKIND3D9PIPELINETOON   */
            (RxPipeline *)NULL, /*  rpSKIND3D9PIPELINEMATFXTOON */
        },
        NULL,                   /*  Last hierarchy used */
        0,                      /*  Last render frame */
        NULL,                   /*  Last frame used */
        FALSE,                  /*  Hardware T&L */
        FALSE,                  /*  Hardware VS */
        0,                      /*  Hardware Max NumBones */
        0                       /*  Hardware Max Num Constants */
    },
    (SkinSplitData *) NULL
};

static const RwReal One = 1.f;
static const RwReal Zero = 0.f;

static RwV4d _rpD3D9SkinVertexShaderConstants[128];

/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/

/****************************************************************************
 - _rpD3D9SkinMatBlendAtomicRender

 - Matrix blending Atomic render function - performs weighted transform of
   an atomic's vertices according to the attached RpSkin data.

 - If NOASM=0 then an Intel x86 assembler function is used for matrix blending.
   Otherwise...

 - Inputs :   RpAtomic *    A pointer to the atomic.
 - Outputs:   RwBool        TRUE if the atomic has a skin, a hierarchy an
                            everything works right
 */
static void
_rpD3D9SkinMatBlendAtomicRender(RpAtomic *atomic,
                                RpGeometry *geometry,
                                RpSkin  *skin,
                                RpHAnimHierarchy *hierarchy)
{
    RwResEntry              *resEntry;
    RxD3D9ResEntryHeader    *resEntryHeader;
    const RwV3d             *originalVertices;
    const RwV3d             *originalNormals;
    RwUInt8                 *vertexData;
    RwUInt32                stride;
    LPDIRECT3DVERTEXBUFFER9 vertexBuffer;

    RWFUNCTION(RWSTRING("_rpD3D9SkinMatBlendAtomicRender"));
    RWASSERT(NULL != atomic);
    RWASSERT(NULL != skin);
    RWASSERT(NULL != hierarchy);

    /* Get instanced data */
    if (geometry->numMorphTargets != 1)
    {
        resEntry = atomic->repEntry;
    }
    else
    {
        resEntry = geometry->repEntry;
    }

    resEntryHeader = (RxD3D9ResEntryHeader *)(resEntry + 1);

    /* Get original vertex information */
    originalVertices = (const RwV3d *)
                        (geometry->morphTarget->verts);

    if (rpGEOMETRYNORMALS & RpGeometryGetFlags(geometry))
    {
        originalNormals = (const RwV3d *)
                            (geometry->morphTarget->normals);
    }
    else
    {
        originalNormals = NULL;
    }

    /* Check right vertexbuffer */
    vertexData = NULL;

	//@{ Jaewon 20050428
	// resEntry can be null here. I don't know why...==;
	if(resEntry)
	//@} Jaewon
	{
    if(_rwD3D9SkinNeedsAManagedVertexBuffer(atomic, resEntryHeader))
    {
        if (!resEntryHeader->vertexStream[0].managed)
        {
            _rpD3D9SkinAtomicCreateVertexBuffer(atomic, resEntryHeader, TRUE);

            geometry->lockedSinceLastInst = rpGEOMETRYLOCKALL;
        }

        vertexBuffer = (LPDIRECT3DVERTEXBUFFER9)resEntryHeader->vertexStream[0].vertexBuffer;

        stride = resEntryHeader->vertexStream[0].stride;

        IDirect3DVertexBuffer9_Lock( vertexBuffer,
                                     resEntryHeader->vertexStream[0].offset,
                                     (geometry->numVertices) * stride, &vertexData,
                                     D3DLOCK_NOSYSLOCK);
    }
    else
    {
        if (resEntryHeader->vertexStream[0].managed)
        {
            _rpD3D9SkinAtomicCreateVertexBuffer(atomic, resEntryHeader, TRUE);

            geometry->lockedSinceLastInst = rpGEOMETRYLOCKALL;
        }

        vertexBuffer = (LPDIRECT3DVERTEXBUFFER9)resEntryHeader->vertexStream[0].vertexBuffer;

        stride = resEntryHeader->vertexStream[0].stride;

        if (vertexBuffer != NULL &&
            resEntryHeader->vertexStream[0].offset == 0 &&
            resEntryHeader->vertexStream[0].dynamicLock == FALSE)
        {
            IDirect3DVertexBuffer9_Lock( vertexBuffer,
                                        0,
                                        0, &vertexData,
                                        D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK);
        }
        else
        {
            RwUInt32 newOffset;

            /* Use Dynamic Lock */
            RwD3D9DynamicVertexBufferLock(stride,
                                          geometry->numVertices,
                                          (void **)&vertexBuffer,
                                          (void **)&vertexData,
                                          &newOffset);

            resEntryHeader->vertexStream[0].vertexBuffer = vertexBuffer;

            newOffset = newOffset * stride;

            resEntryHeader->vertexStream[0].offset = newOffset;

            if (resEntryHeader->useOffsets == FALSE)
            {
                RwUInt32            numMeshes;
                RxD3D9InstanceData  *instancedData;

                numMeshes = resEntryHeader->numMeshes;
                instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

                do
                {
                    instancedData->baseIndex = instancedData->minVert + (newOffset / stride);

                    ++instancedData;
                }
                while (--numMeshes);
            }
        }

        geometry->lockedSinceLastInst |= resEntryHeader->vertexStream[0].geometryFlags;
    }
	}

    if (vertexData != NULL)
    {
        RwUInt8 *vertices;
        RwUInt8 *normals;

        /* Find Destination offset */
        vertices = vertexData;

        if (originalNormals != NULL)
        {
            normals = vertexData + sizeof(RwV3d);
        }
        else
        {
            normals = NULL;
        }

#if !defined(NOASM)

        #if !defined(NOSSEASM)
        if (_rwIntelSSEsupported())
        {
            RwMatrix *matrixArray;

            matrixArray = _rwD3D9SkinPrepareMatrix(atomic, skin, hierarchy);

            if (skin->vertexMaps.maxWeights > 2)
            {
                _rpSkinIntelSSEMatrixBlend(geometry->numVertices,
                                            skin->vertexMaps.matrixWeights,
                                            skin->vertexMaps.matrixIndices,
                                            matrixArray,
                                            vertices,
                                            originalVertices,
                                            normals,
                                            originalNormals,
                                            stride);
            }
            else if(skin->vertexMaps.maxWeights > 1)
            {
                _rpSkinIntelSSEMatrixBlend2Weights(geometry->numVertices,
                                                    skin->vertexMaps.matrixWeights,
                                                    skin->vertexMaps.matrixIndices,
                                                    matrixArray,
                                                    vertices,
                                                    originalVertices,
                                                    normals,
                                                    originalNormals,
                                                    stride);
            }
            else
            {
                if (skin->boneData.numUsedBones == 1)
                {
                    _rpSkinIntelSSEMatrixBlend1Matrix( geometry->numVertices,
                                                        &(matrixArray[skin->boneData.usedBoneList[0]]),
                                                        vertices,
                                                        originalVertices,
                                                        normals,
                                                        originalNormals,
                                                        stride);
                }
                else
                {
                    _rpSkinIntelSSEMatrixBlend1Weight( geometry->numVertices,
                                                        skin->vertexMaps.matrixIndices,
                                                        matrixArray,
                                                        vertices,
                                                        originalVertices,
                                                        normals,
                                                        originalNormals,
                                                        stride);
                }
            }
        }
        else
        #endif  /* !defined(NOSSEASM) */

#endif  /* !defined(NOASSM) */
        {
            _rwD3D9MatrixTransposed  *matrixArray;

            matrixArray = _rwD3D9SkinPrepareMatrixTransposed(atomic, skin, hierarchy);

            /* Vertex blending */
            if (skin->vertexMaps.maxWeights > 2)
            {
                _rpD3D9SkinGenericMatrixBlend(geometry->numVertices,
                                            skin->vertexMaps.matrixWeights,
                                            skin->vertexMaps.matrixIndices,
                                            matrixArray,
                                            vertices,
                                            originalVertices,
                                            normals,
                                            originalNormals,
                                            stride);
            }
            else if(skin->vertexMaps.maxWeights > 1)
            {
                _rpD3D9SkinGenericMatrixBlend2Weights(geometry->numVertices,
                                                    skin->vertexMaps.matrixWeights,
                                                    skin->vertexMaps.matrixIndices,
                                                    matrixArray,
                                                    vertices,
                                                    originalVertices,
                                                    normals,
                                                    originalNormals,
                                                    stride);
            }
            else
            {
                if (skin->boneData.numUsedBones == 1)
                {
                    _rpD3D9SkinGenericMatrixBlend1Matrix( geometry->numVertices,
                                                        &(matrixArray[skin->boneData.usedBoneList[0]]),
                                                        vertices,
                                                        originalVertices,
                                                        normals,
                                                        originalNormals,
                                                        stride);
                }
                else
                {
                    _rpD3D9SkinGenericMatrixBlend1Weight( geometry->numVertices,
                                                        skin->vertexMaps.matrixIndices,
                                                        matrixArray,
                                                        vertices,
                                                        originalVertices,
                                                        normals,
                                                        originalNormals,
                                                        stride);
                }
            }
        }

        IDirect3DVertexBuffer9_Unlock(vertexBuffer);
    }

    geometry->lockedSinceLastInst &= ~(rpGEOMETRYLOCKVERTICES | rpGEOMETRYLOCKNORMALS);

    RWRETURNVOID();
}

/****************************************************************************
 D3DMATRIXFromRwMatrix
 */
static __inline void
D3DMATRIXFromRwMatrix(D3DMATRIX *d, const RwMatrix *m)
{
    RwReal *val = (RwReal *)d;
    RWFUNCTION(RWSTRING("D3DMATRIXFromRwMatrix"));

    *val++ = m->right.x;
    *val++ = m->up.x;
    *val++ = m->at.x;
    *val++ = m->pos.x;

    *val++ = m->right.y;
    *val++ = m->up.y;
    *val++ = m->at.y;
    *val++ = m->pos.y;

    *val++ = m->right.z;
    *val++ = m->up.z;
    *val++ = m->at.z;
    *val++ = m->pos.z;

    RWRETURNVOID();
}

/****************************************************************************
 _rpD3D9SkinVertexShaderMatrixUpdate
 */
void
_rpD3D9SkinVertexShaderMatrixUpdate(RwV4d *shaderConstants,
                                    RpAtomic *atomic,
                                    RpSkin *skin)
{
    RpHAnimHierarchy *hierarchy;
    SkinAtomicData *atomicData;

    RWFUNCTION(RWSTRING("_rpD3D9SkinVertexShaderMatrixUpdate"));
    RWASSERT(NULL != atomic);
    RWASSERT(NULL != skin);

    /* Get the atomic's extension data. */
    atomicData = RPSKINATOMICGETDATA(atomic);
    RWASSERT(NULL != atomicData);

    /* Then it's hierarchy. */
    hierarchy = atomicData->hierarchy;

    if(NULL != hierarchy)
    {
        /* We need to distinguish between VS and FFP, so we negate the
           render frame */
        if (_rpSkinGlobals.platform.lastHierarchyUsed != hierarchy ||
            _rpSkinGlobals.platform.lastRenderFrame != ~((RwUInt32)RWSRCGLOBAL(renderFrame)) ||
            _rpSkinGlobals.platform.lastFrame != RpAtomicGetFrame(atomic))
        {
            RwMatrix result;
            const RwMatrix *skinToBone;

            _rpSkinGlobals.platform.lastHierarchyUsed = (const RpHAnimHierarchy *)hierarchy;
            _rpSkinGlobals.platform.lastRenderFrame = ~((RwUInt32)RWSRCGLOBAL(renderFrame));
            _rpSkinGlobals.platform.lastFrame = RpAtomicGetFrame(atomic);

            /* Get the bone information. */
            skinToBone = RpSkinGetSkinToBoneMatrices(skin);
            RWASSERT(NULL != skinToBone);

            if(hierarchy->flags & rpHANIMHIERARCHYNOMATRICES)
            {
                RwMatrix inverseLtm;
                RwMatrix prodMatrix;
                RwInt32 i;

                RpHAnimNodeInfo *frameInfo;

                /* Get the hierarchy's frame information. */
                frameInfo = hierarchy->pNodeInfo;
                RWASSERT(NULL != frameInfo);

                /* Setup the matrix flags. */
                prodMatrix.flags = 0;

                /* Invert the atomics ltm. */
                _rwD3D9VSGetInverseWorldMatrix(&inverseLtm);

                for (i = 0; i < hierarchy->numNodes; ++i)
                {
                    RwFrame *frame;
                    RwMatrix *ltm;

                    frame = frameInfo[i].pFrame;
                    RWASSERT(NULL != frame);
                    ltm = RwFrameGetLTM(frame);
                    RWASSERT(NULL != ltm);

                    RwMatrixMultiply(&prodMatrix,
                                     &skinToBone[i],
                                     ltm );

                    RwMatrixMultiply(&result,
                                     &prodMatrix,
                                     &inverseLtm);

                    D3DMATRIXFromRwMatrix((D3DMATRIX *)&shaderConstants[i * 3], &result);
                }
            }
            else
            {
                RwMatrix *matrixArray;

                matrixArray = hierarchy->pMatrixArray;
                RWASSERT(NULL != matrixArray);

                if( hierarchy->flags & rpHANIMHIERARCHYLOCALSPACEMATRICES )
                {
                    RwInt32 i;

                    for (i = 0; i < hierarchy->numNodes; ++i)
                    {
                        RwMatrixMultiply(&result, &skinToBone[i], &matrixArray[i]);

                        D3DMATRIXFromRwMatrix((D3DMATRIX *)&shaderConstants[i * 3], &result);
                    }
                }
                else
                {
                    RwMatrix inverseLtm;
                    RwMatrix tempMatrix;
                    RwInt32 i;

                    _rwD3D9VSGetInverseWorldMatrix(&inverseLtm);

                    for (i = 0; i < hierarchy->numNodes; ++i)
                    {
                        RwMatrixMultiply(&tempMatrix, &skinToBone[i], &matrixArray[i]);

                        RwMatrixMultiply(&result, &tempMatrix, &inverseLtm);

                        D3DMATRIXFromRwMatrix((D3DMATRIX *)&shaderConstants[i * 3], &result);
                    }
                }
            }
        }
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpD3D9SkinVertexShaderAtomicRender
 */
static void
_rpD3D9SkinVertexShaderAtomicRender(_rxD3D9SkinVertexShaderNodeData *vertexShaderNode,
                                    RpAtomic *atomic,
                                    RpGeometry *geometry,
                                    RpSkin  *skin,
                                    RpHAnimHierarchy *hierarchy)
{
    _rpD3D9VertexShaderDispatchDescriptor dispatch;
    _rpD3D9VertexShaderDescriptor   desc;
    RwMatrix                        *matrix;

    RwV4d                           *shaderConstantPtr;
    RwResEntry                      *resEntry;
    RxD3D9ResEntryHeader            *resEntryHeader;
    RxD3D9InstanceData              *instancedData;

    RwInt32                         numMeshes;
    const RwUInt8                   *meshBatches;
    const RwUInt8                   *boneBatches;
    RwUInt32                        shaderConstantCount;

    RwReal                          lastAmbientCoef = 1.f;
    RwReal                          lastDiffuseCoef = 1.f;
    RwUInt32                        color;
    RwUInt8                         offsetMaterialColor = 0xff;
    RwUInt8                         offsetBoneMatrices = 0xff;
    RwUInt8                         offsetFog = 0xff;

    RWFUNCTION(RWSTRING("_rpD3D9SkinVertexShaderAtomicRender"));

    RWASSERT(vertexShaderNode->beginCallback != NULL);
    vertexShaderNode->beginCallback(atomic, rpATOMIC, &desc);

    desc.numWeights = skin->vertexMaps.maxWeights;

    /* Set clipping */
    RwD3D9SetRenderState(D3DRS_CLIPPING, TRUE);

    /* Prepare matrix calculations */
    matrix = RwFrameGetLTM(RpAtomicGetFrame(atomic));

    _rwD3D9VSSetActiveWorldMatrix(matrix);

    shaderConstantPtr = _rpD3D9SkinVertexShaderConstants;

    _rwD3D9VSGetComposedTransformMatrix(shaderConstantPtr);
    shaderConstantPtr += 4;

    RWASSERT(vertexShaderNode->lightingCallback != NULL);
    shaderConstantPtr = vertexShaderNode->lightingCallback(atomic,
                                                           rpATOMIC,
                                                           shaderConstantPtr,
                                                           &desc);

    shaderConstantCount = ((RwUInt32)shaderConstantPtr - (RwUInt32)_rpD3D9SkinVertexShaderConstants) / sizeof(RwV4d);

    /* upload per atomic shader constants */
    RwD3D9SetVertexShaderConstant(0, _rpD3D9SkinVertexShaderConstants,
                                  shaderConstantCount);

    _rpD3D9SkinVertexShaderMatrixUpdate((RwV4d *)_rpSkinGlobals.matrixCache.aligned, atomic, skin);

    /* Get instanced data */
    if (geometry->numMorphTargets != 1)
    {
        resEntry = atomic->repEntry;
    }
    else
    {
        resEntry = geometry->repEntry;
    }

    /* Get header */
    resEntryHeader = (RxD3D9ResEntryHeader *)(resEntry + 1);

    /*
     * Data shared between meshes
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

    /* Get skin split data */
    meshBatches = skin->skinSplitData.meshRLECount;
    boneBatches = skin->skinSplitData.meshRLE;

    /* Get the instanced data */
    instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

    /* Init last color to only update color if needed */
    color = *((const RwUInt32 *)&(instancedData->material->color));

    /* Get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;
    while (numMeshes--)
    {
        const RpMaterial    *material;
        const RwRGBA        *matcolor;

        RWASSERT(instancedData->material != NULL);

        material = instancedData->material;

        matcolor = &(material->color);

        if ((0xFF != matcolor->alpha) ||
            instancedData->vertexAlpha)
        {
            _rwD3D9RenderStateVertexAlphaEnable(TRUE);
        }
        else
        {
            _rwD3D9RenderStateVertexAlphaEnable(FALSE);
        }

        /* Get right vertex shader */
        RWASSERT(vertexShaderNode->getmaterialshaderCallback != NULL);
        instancedData->vertexShader = vertexShaderNode->getmaterialshaderCallback(material,
                                                                                  &desc,
                                                                                  &dispatch);


        /* Do I need to remove lights in order to make it work? */
        shaderConstantCount = _rpD3D9GetNumConstantsUsed(&desc) +
                              skin->platformData.maxNumBones * 3;

        if (shaderConstantCount > _rpSkinGlobals.platform.maxNumConstants)
        {
            #ifdef RWDEBUG
            static RpSkin *lastskin = NULL;

            if (lastskin != skin)
            {
                RwChar buffer[512];

                lastskin = skin;

                rwsprintf(buffer,
                          "\n\nThere are too many bones for this combination of lights and effects:\n"
                          "   Required Vertex Shader constants: %d\n"
                          "   Max Vertex Shader constants: %d\n"
                          "   Bones: %d\n"
                          "   Directional lights: %d\n"
                          "   Point lights: %d\n"
                          "   Spot lights: %d\n"
                          "   Fog: %s\n"
                          "   Material color: %s\n"
                          "   Effect: %d\n"
                          "\n   Use the RtSkinSplit Toolkit to reduce the number of bones.\n",
                          shaderConstantCount,
                          _rpSkinGlobals.platform.maxNumConstants,
                          skin->platformData.maxNumBones,
                          (RwUInt32)desc.numDirectionalLights,
                          (RwUInt32)desc.numPointLights,
                          (RwUInt32)desc.numSpotLights,
                          (desc.fogMode ? "On" : "Off"),
                          (desc.modulateMaterial ? "On" : "Off"),
                          (RwUInt32)desc.effect);

                RwDebugSendMessage(rwDEBUGMESSAGE,
                                "_rpD3D9SkinVertexShaderAtomicRender",
                                buffer);
            }
            #endif

            do
            {
                if (desc.numSpotLights)
                {
                    --desc.numSpotLights;
                }
                else if (desc.numPointLights)
                {
                    --desc.numPointLights;
                }
                else if (desc.numDirectionalLights)
                {
                    --desc.numDirectionalLights;
                }
                else if (desc.fogMode)
                {
                    /* This is a bit too drastic */
                    desc.fogMode = 0;
                }
                else
                {
                    /* This should never happend!!!! */
                    break;
                }

                shaderConstantCount = _rpD3D9GetNumConstantsUsed(&desc) +
                                    skin->platformData.maxNumBones * 3;
            }
            while (shaderConstantCount > _rpSkinGlobals.platform.maxNumConstants);

            instancedData->vertexShader =
                                    vertexShaderNode->getmaterialshaderCallback(material,
                                                                                &desc,
                                                                                &dispatch);

            /* Force lights upload */
            lastAmbientCoef = 1.f - material->surfaceProps.ambient;
        }

        if (lastAmbientCoef != material->surfaceProps.ambient ||
            lastDiffuseCoef != material->surfaceProps.diffuse)
        {
            _rpD3D9VertexShaderUpdateLightsColors(_rpD3D9SkinVertexShaderConstants + RWD3D9VSCONST_AMBIENT_OFFSET,
                                                  &desc,
                                                  (material->surfaceProps.ambient) / lastAmbientCoef,
                                                  (material->surfaceProps.diffuse) / lastDiffuseCoef);

            lastAmbientCoef = material->surfaceProps.ambient;
            lastDiffuseCoef = material->surfaceProps.diffuse;
        }

        /* Update material color if needed */
        if (offsetMaterialColor != dispatch.offsetMaterialColor ||
            color != *((const RwUInt32 *)matcolor))
        {
            offsetMaterialColor = dispatch.offsetMaterialColor;
            color = *((const RwUInt32 *)matcolor);

            _rpD3D9VertexShaderUpdateMaterialColor(matcolor, &dispatch);
        }

        /* Update fog if needed */
        if (offsetFog != dispatch.offsetFogRange)
        {
            offsetFog = dispatch.offsetFogRange;

            _rpD3D9VertexShaderUpdateFogData(&desc, &dispatch);
        }

        /* Upload matrices */
        if (meshBatches != NULL)
        {
            const RwV4d *matrixArray = (const RwV4d *)_rpSkinGlobals.matrixCache.aligned;
            const RwUInt32 firstBatch = meshBatches[0];
            const RwUInt32 numBatches = meshBatches[1];
            RwUInt32 currentBatch;

            offsetBoneMatrices = dispatch.offsetBoneMatrices;

            meshBatches += 2;

            for (currentBatch = 0; currentBatch < numBatches; currentBatch++)
            {
                RwUInt32 firstBone = boneBatches[(firstBatch + currentBatch) * 2];
                RwUInt32 numBones = boneBatches[(firstBatch + currentBatch) * 2 + 1];

                do
                {
                    RwD3D9SetVertexShaderConstant(offsetBoneMatrices + (skin->skinSplitData.matrixRemapIndices[firstBone] * 3),
                                                  matrixArray + (firstBone * 3), 3);
                    ++firstBone;
                }
                while(--numBones);
            }
        }
        else
        {
            if (offsetBoneMatrices != dispatch.offsetBoneMatrices)
            {
                const RwV4d *matrixArray;

                offsetBoneMatrices = dispatch.offsetBoneMatrices;

                matrixArray = (const RwV4d *)_rpSkinGlobals.matrixCache.aligned;

                if ((RwUInt32)hierarchy->numNodes <= _rpSkinGlobals.platform.maxNumBones)
                {
                    if (skin->boneData.numUsedBones < (RwUInt32)hierarchy->numNodes)
                    {
                        const RwUInt32 numUsedBones = skin->boneData.numUsedBones;
                        const RwUInt8 *usedBones = skin->boneData.usedBoneList;
                        RwUInt32 n = 0;

                        do
                        {
                            const RwUInt32 baseIndex = n;

                            while(n < numUsedBones - 1 &&
                                usedBones[n] + 1 == usedBones[n + 1])
                            {
                                ++n;
                            }
                            ++n;

                            RwD3D9SetVertexShaderConstant(offsetBoneMatrices + usedBones[baseIndex] * 3,
                                                          (const void *)(matrixArray + usedBones[baseIndex] * 3),
                                                          (n - baseIndex) * 3);
                        }
                        while(n < numUsedBones);
                    }
                    else
                    {
                        RwD3D9SetVertexShaderConstant(offsetBoneMatrices,
                                                      (const void *)matrixArray,
                                                      (hierarchy->numNodes) * 3);
                    }
                }
                else
                {
                    const RwUInt32 numUsedBones = skin->boneData.numUsedBones;
                    const RwUInt8 *usedBones = skin->boneData.usedBoneList;
                    RwUInt32 n = 0;

                    do
                    {
                        const RwUInt32 baseIndex = n;

                        while(n < (numUsedBones - 1) &&
                              (usedBones[n] + 1) == usedBones[n + 1])
                        {
                            ++n;
                        }
                        ++n;

                        RwD3D9SetVertexShaderConstant(offsetBoneMatrices + baseIndex * 3,
                                                      (const void *)(matrixArray + usedBones[baseIndex] * 3),
                                                      (n - baseIndex) * 3);
                    }
                    while(n < numUsedBones);
                }
            }
        }

        RWASSERT(vertexShaderNode->meshRenderCallback != NULL);
        vertexShaderNode->meshRenderCallback(resEntryHeader, instancedData,
                                             &desc, &dispatch);

        /* Move onto the next instancedData */
        instancedData->vertexShader = NULL;

        ++instancedData;
    }

    RWASSERT(vertexShaderNode->endCallback != NULL);
    vertexShaderNode->endCallback(atomic, rpATOMIC, &desc);

    RWRETURNVOID();
}

/****************************************************************************
 _rpD3D9SkinFFPAtomicRender
 */
static void
_rpD3D9SkinFFPAtomicRender(_rxD3D9SkinInstanceNodeData *privateData,
                           RwResEntry *resEntry,
                           RpAtomic *atomic,
                           RwUInt32 geomFlags)
{
    RwMatrix    *matrix;
    RwUInt32    lighting;

    RWFUNCTION(RWSTRING("_rpD3D9SkinFFPAtomicRender"));

    /*
    * Set up lights
    */
    if (privateData->lightingCallback)
    {
        privateData->lightingCallback((void *)atomic);
    }

    /*
    * Set the world transform to identity because all the akin matrix were in world space
    */
    matrix = RwFrameGetLTM(RpAtomicGetFrame(atomic));

    RwD3D9SetTransformWorld(matrix);


    /* Check if we need to normalize */
    RwD3D9GetRenderState(D3DRS_LIGHTING, &lighting);

   if (lighting)
    {
        if (_rpSkinGlobals.platform.hardwareTL)
        {
            RwD3D9SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
        }
        else if (!rwMatrixTestFlags(matrix, (rwMATRIXTYPENORMAL | rwMATRIXINTERNALIDENTITY)))
        {
            const RwReal minlimit = 0.9f;
            const RwReal maxlimit = 1.1f;
            RwReal length;

            length = RwV3dDotProduct(&(matrix->right), &(matrix->right));

            if ( (FLOATASINT(length) > FLOATASINT(maxlimit)) || (FLOATASINT(length) < FLOATASINT(minlimit)) )
            {
                RwD3D9SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
            }
            else
            {
                length = RwV3dDotProduct(&(matrix->up), &(matrix->up));

                if ( (FLOATASINT(length) > FLOATASINT(maxlimit)) || (FLOATASINT(length) < FLOATASINT(minlimit)) )
                {
                    RwD3D9SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
                }
                else
                {
                    length = RwV3dDotProduct(&(matrix->at), &(matrix->at));

                    if ( (FLOATASINT(length) > FLOATASINT(maxlimit)) || (FLOATASINT(length) < FLOATASINT(minlimit)) )
                    {
                        RwD3D9SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
                    }
                    else
                    {
                        RwD3D9SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);
                    }
                }
            }
        }
        else
        {
            RwD3D9SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);
        }
    }
    else
    {
        RwD3D9SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);
    }

    /*
    * Render
    */
    if (privateData->renderCallback)
    {
       privateData->renderCallback(resEntry, (void *)atomic, rpATOMIC, geomFlags);
    }

    RWRETURNVOID();
}

RwBool
RpSkinD3DAtomicOnlyVertexUpdate(RpAtomic  *atomic)
{
	RpGeometry              *geometry;
    RwResEntry              *resEntry;
    RpMeshHeader            *meshHeader;
    RwUInt32                geomFlags;
    RpSkin                  *skin;
    RpHAnimHierarchy        *hierarchy;
	RxD3D9ResEntryHeader    *resEntryHeader;

	RWFUNCTION(RWSTRING("_rwSkinD3DAtomicAllInOneNodeOnlyVertexUpdate"));

	RWASSERT(NULL != atomic);

    geometry = RpAtomicGetGeometry(atomic);
    RWASSERT(NULL != geometry);

    /* If there ain't vertices, we cain't make packets... */
    if (geometry->numVertices <= 0)
    {
        /* Don't execute the rest of the pipeline */
        RWRETURN(TRUE);
    }

    meshHeader = geometry->mesh;

    /* Early out if no meshes */
    if (meshHeader->numMeshes <= 0)
    {
        /* If the app wants to use plugin data to make packets, it
         * should use its own instancing function. If we have verts
         * here, we need meshes too in order to build a packet. */
        RWRETURN(TRUE);
    }

    /* If the geometry has more than one morph target the resEntry in the
     * atomic is used else the resEntry in the geometry */
    if (geometry->numMorphTargets != 1)
    {
        resEntry = atomic->repEntry;
    }
    else
    {
        resEntry = geometry->repEntry;
    }

    /* If the meshes have changed we should re-instance */
    if (resEntry)
    {
        resEntryHeader = (RxD3D9ResEntryHeader *)(resEntry + 1);

        if (resEntryHeader->serialNumber != meshHeader->serialNum )
        {
            /* Destroy resources to force reinstance */
            RwResourcesFreeResEntry(resEntry);
            resEntry = NULL;
        }
    }

    geomFlags = RpGeometryGetFlags(geometry);

    /* Check to see if a resource entry already exists */
    if (!resEntry)
    {
		RwResEntry  **resEntryPointer;
        void        *owner;

        meshHeader = geometry->mesh;

        if (geometry->numMorphTargets != 1)
        {
            owner = (void *)atomic;
            resEntryPointer = &atomic->repEntry;
        }
        else
        {
            owner = (void *)geometry;
            resEntryPointer = &geometry->repEntry;
        }

        /*
         * Create vertex buffers and instance
         */
        resEntry = _rxD3D9SkinInstance(atomic, owner, resEntryPointer, meshHeader);
        if (!resEntry)
        {
            RWRETURN(FALSE);
        }

		// 2005.3.31 gemani
		resEntryHeader = (RxD3D9ResEntryHeader *)(resEntry + 1);

		// 2005.3.31 gemani - instanced
		resEntryHeader->isLive = 1;

        /* The geometry is up to date */
        geometry->lockedSinceLastInst = 0;
    }
    else
    {
        /* We have a resEntry so use it */
        RwResourcesUseResEntry(resEntry);
    }

	resEntryHeader = (RxD3D9ResEntryHeader *)(resEntry + 1);

	//@{ 2006/10/12 burumal
	if ( resEntryHeader->isLive )
	{
		if ( resEntryHeader->vertexStream[0].vertexBuffer == NULL )
		{
			if ( resEntryHeader->numMeshes > 1 )
			{
				int	nMeshNum;
				for ( nMeshNum = 1; nMeshNum < resEntryHeader->numMeshes; nMeshNum++ )
					if ( resEntryHeader->vertexStream[nMeshNum].vertexBuffer != NULL )
						break;

				if ( nMeshNum >= resEntryHeader->numMeshes )
					resEntryHeader->isLive = 0;
			}
		}
	}	
	//@}

	//. 2006. 7. 28. Nonstopdj
	//. 이미 지워진 resEntryHeader이므로.
	//if(!resEntryHeader->isLive || resEntryHeader->vertexStream[0].vertexBuffer == NULL)
	if(!resEntryHeader->isLive)
	{
		//.RwResourcesFreeResEntry(resEntry);
		if (resEntry->link.next)
		{
			//@{ 20050513 DDonSS : Threadsafe
			// Resource Arena Lock
			CS_RESARENA_LOCK();
			//@} DDonSS

			/* Remove the link list attachment */
			rwLinkListRemoveLLLink(&resEntry->link);

			/* Adjust the current size */
			RWRESOURCESGLOBAL(res.currentSize) -= resEntry->size;

			//@{ 20050513 DDonSS : Threadsafe
			// Resource Arena Unlock
			CS_RESARENA_UNLOCK();
			//@} DDonSS

#ifdef RWRESOURCESDYNAMIC

			/* Free from main memory */
			RwFree(resEntry);

#else /* RWRESOURCESDYNAMIC */

			/* Free from resources heap */
			_rwResHeapFree(resEntry);

#endif /* RWRESOURCESDYNAMIC */

		}
		else
		{
			/* This entry is not in a list and must be in main memory */
			RwFree(resEntry);
		}

		RWRETURN(TRUE);
	}

	//@{ 20050513 DDonSS : Threadsafe
	// ResEntry Lock
	CS_RESENTRYHEADER_LOCK( resEntryHeader );
	//@} DDonSS

    /*
     * Reinstance
     */
    skin = *RPSKINGEOMETRYGETDATA(geometry);
    hierarchy = RPSKINATOMICGETDATA(atomic)->hierarchy;

    if(skin != NULL ) 
		//&& skin->platformData.useVertexShader == FALSE)
    {
        /* Perform matrix blending */
        if (hierarchy != NULL)
        {
            /* Update skin vertices */
            _rpD3D9SkinMatBlendAtomicRender(atomic,
                                            geometry,
                                            skin,
                                            hierarchy);

        }
    }

    if (geometry->lockedSinceLastInst)
    {
        RxD3D9ResEntryHeader    *resEntryHeader;

        resEntryHeader = (RxD3D9ResEntryHeader *)(resEntry + 1);

        _rpD3D9SkinGeometryReinstance(atomic, resEntryHeader, hierarchy, geometry->lockedSinceLastInst);

		// 2005.3.31 gemani - instanced
		resEntryHeader->isLive = 1;

        geometry->lockedSinceLastInst = 0;
    }

    atomic->interpolator.flags &= ~rpINTERPOLATORDIRTYINSTANCE;

#ifdef RWMETRICS
    /* Now update our metrics statistics */
    RWSRCGLOBAL(metrics)->numVertices  += RpGeometryGetNumVertices(geometry);
    RWSRCGLOBAL(metrics)->numTriangles += RpGeometryGetNumTriangles(geometry);
#endif

	//@{ 20050513 DDonSS : Threadsafe
	// ResEntry Unlock
	CS_RESENTRYHEADER_UNLOCK( resEntryHeader );
	//@} DDonSS

    RWRETURN(TRUE);
}

/****************************************************************************
 _rwSkinD3D9AtomicAllInOneNode
 */
RwBool
_rwSkinD3D9AtomicAllInOneNode(RxPipelineNodeInstance *self,
                              const RxPipelineNodeParam *params)
{	
    _rxD3D9SkinInstanceNodeData *privateData;
    RpAtomic                *atomic;
    RpGeometry              *geometry;
    RwResEntry              *resEntry;
    RpMeshHeader            *meshHeader;
    RwUInt32                geomFlags;
    RpSkin                  *skin;
    RpHAnimHierarchy        *hierarchy;
	RxD3D9ResEntryHeader    *resEntryHeader;

    RWFUNCTION(RWSTRING("_rwSkinD3D9AtomicAllInOneNode"));
    RWASSERT(NULL != self);
    RWASSERT(NULL != params);

    privateData = (_rxD3D9SkinInstanceNodeData *)self->privateData;

    atomic = (RpAtomic *)RxPipelineNodeParamGetData(params);
    RWASSERT(NULL != atomic);

    geometry = RpAtomicGetGeometry(atomic);
    RWASSERT(NULL != geometry);

    /* If there ain't vertices, we cain't make packets... */
    if (geometry->numVertices <= 0)
    {
        /* Don't execute the rest of the pipeline */
        RWRETURN(TRUE);
    }

    meshHeader = geometry->mesh;

    /* Early out if no meshes */
    if (meshHeader->numMeshes <= 0)
    {
        /* If the app wants to use plugin data to make packets, it
         * should use its own instancing function. If we have verts
         * here, we need meshes too in order to build a packet. */
        RWRETURN(TRUE);
    }

    /* If the geometry has more than one morph target the resEntry in the
     * atomic is used else the resEntry in the geometry */
    if (geometry->numMorphTargets != 1)
    {
        resEntry = atomic->repEntry;
    }
    else
    {
        resEntry = geometry->repEntry;
    }

    /* If the meshes have changed we should re-instance */
    if (resEntry)
    {
        resEntryHeader = (RxD3D9ResEntryHeader *)(resEntry + 1);

        if (resEntryHeader->serialNumber != meshHeader->serialNum )
        {
            /* Destroy resources to force reinstance */
            RwResourcesFreeResEntry(resEntry);
            resEntry = NULL;
        }
    }

    geomFlags = RpGeometryGetFlags(geometry);

    /* Check to see if a resource entry already exists */
    if (!resEntry)
    {
		RwResEntry  **resEntryPointer;
        void        *owner;

        meshHeader = geometry->mesh;

        if (geometry->numMorphTargets != 1)
        {
            owner = (void *)atomic;
            resEntryPointer = &atomic->repEntry;
        }
        else
        {
            owner = (void *)geometry;
            resEntryPointer = &geometry->repEntry;
        }

        /*
         * Create vertex buffers and instance
         */
        resEntry = _rxD3D9SkinInstance(atomic, owner, resEntryPointer, meshHeader);
        if (!resEntry)
        {
            RWRETURN(FALSE);
        }

		// 2005.3.31 gemani
		resEntryHeader = (RxD3D9ResEntryHeader *)(resEntry + 1);

		// 2005.3.31 gemani - instanced
		resEntryHeader->isLive = 1;

        /* The geometry is up to date */
        geometry->lockedSinceLastInst = 0;
    }
    else
    {
        /* We have a resEntry so use it */
        RwResourcesUseResEntry(resEntry);
    }

	resEntryHeader = (RxD3D9ResEntryHeader *)(resEntry + 1);
	
	//@{ 2006/10/12 burumal
	if ( resEntryHeader->isLive )
	{
		if ( resEntryHeader->vertexStream[0].vertexBuffer == NULL )
		{			
			if ( resEntryHeader->numMeshes > 1 )
			{
				int	nMeshNum;
				for ( nMeshNum = 1; nMeshNum < resEntryHeader->numMeshes; nMeshNum++ )
					if ( resEntryHeader->vertexStream[nMeshNum].vertexBuffer != NULL )
						break;

				if ( nMeshNum >= resEntryHeader->numMeshes )
					resEntryHeader->isLive = 0;
			}
		}
	}	
	//@}

	//. 2006. 7. 28. Nonstopdj
	//. 이미 지워진 resEntryHeader이므로.
	//if(!resEntryHeader->isLive || resEntryHeader->vertexStream[0].vertexBuffer == NULL)
	if(!resEntryHeader->isLive)
	{
		//.RwResourcesFreeResEntry(resEntry);
		if (resEntry->link.next)
		{
			//@{ 20050513 DDonSS : Threadsafe
			// Resource Arena Lock
			CS_RESARENA_LOCK();
			//@} DDonSS

			/* Remove the link list attachment */
			rwLinkListRemoveLLLink(&resEntry->link);

			/* Adjust the current size */
			RWRESOURCESGLOBAL(res.currentSize) -= resEntry->size;

			//@{ 20050513 DDonSS : Threadsafe
			// Resource Arena Unlock
			CS_RESARENA_UNLOCK();
			//@} DDonSS

#ifdef RWRESOURCESDYNAMIC

			/* Free from main memory */
			RwFree(resEntry);

#else /* RWRESOURCESDYNAMIC */

			/* Free from resources heap */
			_rwResHeapFree(resEntry);

#endif /* RWRESOURCESDYNAMIC */

		}
		else
		{
			/* This entry is not in a list and must be in main memory */
			RwFree(resEntry);
		}

		RWRETURN(TRUE);
	}

	//@{ 20050513 DDonSS : Threadsafe
	// ResEntry Lock
	CS_RESENTRYHEADER_LOCK( resEntryHeader );
	//@} DDonSS

    /*
     * Reinstance
     */
    skin = *RPSKINGEOMETRYGETDATA(geometry);
    hierarchy = RPSKINATOMICGETDATA(atomic)->hierarchy;

    if(skin != NULL &&
       skin->platformData.useVertexShader == FALSE)
    {
        /* Perform matrix blending */
        if (hierarchy != NULL)
        {
            /* Update skin vertices */
            _rpD3D9SkinMatBlendAtomicRender(atomic,
                                            geometry,
                                            skin,
                                            hierarchy);

        }
    }

    if (geometry->lockedSinceLastInst)
    {
        RxD3D9ResEntryHeader    *resEntryHeader;

        resEntryHeader = (RxD3D9ResEntryHeader *)(resEntry + 1);

        _rpD3D9SkinGeometryReinstance(atomic, resEntryHeader, hierarchy, geometry->lockedSinceLastInst);

		// 2005.3.31 gemani - instanced
		resEntryHeader->isLive = 1;

        geometry->lockedSinceLastInst = 0;
    }

    atomic->interpolator.flags &= ~rpINTERPOLATORDIRTYINSTANCE;


	
    /*
     * Dispatch geometry
     */
	if(skin != NULL &&
	   skin->platformData.useVertexShader &&
	   //(privateData->vertexShaderNode).beginCallback &&
       hierarchy != NULL)
    {
		_rpD3D9SkinVertexShaderAtomicRender(&(privateData->vertexShaderNode),
                                            atomic,
                                            geometry,
                                            skin,
                                            hierarchy);
    }
	else
    {
		
        _rpD3D9SkinFFPAtomicRender(privateData, resEntry, atomic, geomFlags);
    }


#ifdef RWMETRICS
    /* Now update our metrics statistics */
    RWSRCGLOBAL(metrics)->numVertices  += RpGeometryGetNumVertices(geometry);
    RWSRCGLOBAL(metrics)->numTriangles += RpGeometryGetNumTriangles(geometry);
#endif

	//@{ 20050513 DDonSS : Threadsafe
	// ResEntry Unlock
	CS_RESENTRYHEADER_UNLOCK( resEntryHeader );
	//@} DDonSS

    RWRETURN(TRUE);
}

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

/****************************************************************************
 _rpSkinPipelinesDestroy
 Destroy the skinning pipelines.
 Inputs :   None
 Outputs:   RwBool - on success.
 */
RwBool
_rpSkinPipelinesDestroy(void)
{
    RwInt32 pipeIndex;

    RWFUNCTION(RWSTRING("_rpSkinPipelinesDestroy"));

    for (pipeIndex = rpSKIND3D9PIPELINEGENERIC; pipeIndex < rpSKIND3D9PIPELINEMAX; pipeIndex++)
    {
        if (_rpSkinPipeline(pipeIndex))
        {
            RxPipelineDestroy(_rpSkinPipeline(pipeIndex));

            _rpSkinPipeline(pipeIndex) = NULL;
        }
    }

    RWRETURN(TRUE);
}

/****************************************************************************
 BonesUsedSort
 */
int
BonesUsedSort(const void *data0, const void *data1)
{
    const RwUInt8 *index0 = (const RwUInt8 *)data0;
    const RwUInt8 *index1 = (const RwUInt8 *)data1;

    RWFUNCTION(RWSTRING("BonesUsedSort"));

    RWRETURN(*index0 - *index1);
}

/****************************************************************************
 _rpSkinInitialize

 Initialise an atomic's matrix-blending skin data.

 Inputs :   RpGeometry * - Pointer to a skinned geometry.
 Outputs:   RpGeometry * - Pointer to the skinned geometry on success.
 */
RpGeometry *
_rpSkinInitialize(RpGeometry *geometry)
{
    RpSkin *skin;

    RWFUNCTION(RWSTRING("_rpSkinInitialize"));
    RWASSERT(NULL != geometry);

    skin = *RPSKINGEOMETRYGETDATA(geometry);

    if(skin != NULL)
    {
        RwInt32 n;
        RwMatrixWeights *matrixWeights;
        RwUInt32 *matrixIndexMap;

        matrixWeights = skin->vertexMaps.matrixWeights;

        matrixIndexMap = skin->vertexMaps.matrixIndices;

        for (n = 0; n < geometry->numVertices; ++n)
        {
            RWASSERT(FLOATASINT(matrixWeights->w0) > FLOATASINT(Zero));

            if (FLOATASINT(matrixWeights->w0) < FLOATASINT(One))
            {
                RwBool bCheck;

                /* Sort indices and weights */
                do
                {
                    bCheck = FALSE;

                    if (FLOATASINT(matrixWeights->w0) < FLOATASINT(matrixWeights->w1))
                    {
                        RwUInt8 *indices = (RwUInt8 *)(matrixIndexMap + n);

                        const RwUInt8 temp = indices[0];
                        const RwReal tempf = matrixWeights->w0;

                        indices[0] = indices[1];
                        indices[1] = temp;

                        matrixWeights->w0 = matrixWeights->w1;
                        matrixWeights->w1 = tempf;

                        bCheck = TRUE;
                    }

                    if (FLOATASINT(matrixWeights->w1) < FLOATASINT(matrixWeights->w2))
                    {
                        RwUInt8 *indices = (RwUInt8 *)(matrixIndexMap + n);

                        const RwUInt8 temp = indices[1];
                        const RwReal tempf = matrixWeights->w1;

                        indices[1] = indices[2];
                        indices[2] = temp;

                        matrixWeights->w1 = matrixWeights->w2;
                        matrixWeights->w2 = tempf;

                        bCheck = TRUE;
                    }

                    if (FLOATASINT(matrixWeights->w2) < FLOATASINT(matrixWeights->w3))
                    {
                        RwUInt8 *indices = (RwUInt8 *)(matrixIndexMap + n);

                        const RwUInt8 temp = indices[2];
                        const RwReal tempf = matrixWeights->w2;

                        indices[2] = indices[3];
                        indices[3] = temp;

                        matrixWeights->w2 = matrixWeights->w3;
                        matrixWeights->w3 = tempf;

                        bCheck = TRUE;
                    }
                }
                while(bCheck);
            }

            ++matrixWeights;
        }
    }

    /* Sort the used bones list */
    if (skin->boneData.usedBoneList != NULL)
    {
        qsort(skin->boneData.usedBoneList,
              skin->boneData.numUsedBones,
              sizeof(RwUInt8),
              BonesUsedSort);
    }

    RWRETURN(geometry);
}


/****************************************************************************
 _rpSkinDeinitialize
 Platform specific deinitialize function for skinned geometry's.
 Inputs :  *geometry    - Pointer to the skinned geometry.
 Outputs:  RpGeometry * - The geometry which has been deinitialized.
 */
RpGeometry *
_rpSkinDeinitialize(RpGeometry *geometry)
{
    RpSkin *skin;

    RWFUNCTION(RWSTRING("_rpSkinDeinitialize"));
    RWASSERT(NULL != geometry);

    skin = *RPSKINGEOMETRYGETDATA(geometry);

    if(NULL != skin)
    {
    }

    RWRETURN(geometry);
}

/*===========================================================================*
 *--- Plugin Native Serialization Functions ---------------------------------*
 *===========================================================================*/

/****************************************************************************
 _rpSkinGeometryNativeSize
 */
RwInt32
_rpSkinGeometryNativeSize(const RpGeometry *geometry __RWUNUSED__)
{
#if 0
    RwInt32         size;
    const RpSkin    *skin;
#endif /* 0 */

    RWFUNCTION(RWSTRING("_rpSkinGeometryNativeSize"));

#if 0
    RWASSERT(geometry);

    /* Chunk header for version number + platform specific ID */
    size = rwCHUNKHEADERSIZE + sizeof(RwPlatformID);

    /* Num bones. */
    size += sizeof(RwUInt32);

    /* ... */

    RWRETURN(size);
#endif /* 0 */

    RWRETURN(0);
}

/****************************************************************************
 _rpSkinGeometryNativeWrite
 */
RwStream *
_rpSkinGeometryNativeWrite(RwStream *stream, const RpGeometry *geometry)
{
    RwPlatformID    id = rwID_PCD3D9;
    RwInt32         size;
    const RpSkin    *skin;

    RWFUNCTION(RWSTRING("_rpSkinGeometryNativeWrite"));
    RWASSERT(stream);
    RWASSERT(geometry);

    /* Get size of data to write */
    size = _rpSkinGeometryNativeSize(geometry) - rwCHUNKHEADERSIZE;

    /* Write a chunk header so we get a VERSION NUMBER */
    if (!RwStreamWriteChunkHeader(stream, rwID_STRUCT, size))
    {
        RWRETURN((RwStream *)NULL);
    }

    /* Write a platform unique identifier */
    if (!RwStreamWriteInt32(stream, (RwInt32 *)&id, sizeof(RwPlatformID)))
    {
        RWRETURN((RwStream *)NULL);
    }

    skin = *RPSKINGEOMETRYGETCONSTDATA(geometry);

    /*
     * Write the number of bones
     */
    if (!RwStreamWriteInt32(stream,
                            (const RwInt32 *)&(skin->boneData.numBones),
                            sizeof(RwUInt32)))
    {
        RWRETURN((RwStream *)NULL);
    }

    RWRETURN(stream);
}

/****************************************************************************
 _rpSkinGeometryNativeRead
 */
RwStream *
_rpSkinGeometryNativeRead(RwStream *stream, RpGeometry *geometry)
{
    RwUInt32        version, size;
    RwPlatformID    id;
    RpSkin          *skin;

    RWFUNCTION(RWSTRING("_rpSkinGeometryNativeRead"));
    RWASSERT(stream);
    RWASSERT(geometry);

    if (!RwStreamFindChunk(stream, rwID_STRUCT, (RwUInt32 *)&size, &version))
    {
        RWRETURN((RwStream *)NULL);
    }

    if ((version < rwLIBRARYBASEVERSION) ||
        (version > rwLIBRARYCURRENTVERSION))
    {
        RWERROR((E_RW_BADVERSION));
        RWRETURN((RwStream *)NULL);
    }

    /* Read the platform unique identifier */
    if (!RwStreamReadInt32(stream, (RwInt32 *)&id, sizeof(RwPlatformID)))
    {
        RWRETURN((RwStream *)NULL);
    }

    /* Check this data is funky for this platform */
    if (rwID_PCD3D9 != id)
    {
        RWASSERT(rwID_PCD3D9 == id);

        RWRETURN((RwStream *)NULL);
    }

    /* Create a new skin */
    skin = (RpSkin *)RwFreeListAlloc(_rpSkinGlobals.freeList,
                         rwID_SKINPLUGIN | rwMEMHINTDUR_EVENT);
    RWASSERT(NULL != skin);

    /* Clean the skin. */
    memset(skin, 0, sizeof(RpSkin));

    /* Read the number of bones */
    if (!RwStreamReadInt32(stream,
                           (RwInt32 *)&skin->boneData.numBones,
                           sizeof(RwInt32)))
    {
        RWRETURN((RwStream *)NULL);
    }

    RpSkinGeometrySetSkin(geometry, skin);

    RWRETURN(stream);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/
