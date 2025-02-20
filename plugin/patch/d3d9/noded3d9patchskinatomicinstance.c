/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rppatch.h"
#include "rpskin.h"

#include "patch.h"
#include "patchatomic.h"
#include "patchmesh.h"

#include "noded3d9patchatomicinstance.h"
#include "noded3d9patchskinatomicinstance.h"

#define FLOATASINT(f) (*((const RwInt32 *)&(f)))

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/

#define PATCHPASSTHROUGH 1

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/

/*****************************************************************************
 _rpD3D9PatchAtomicInstance
 _refineNode
 Inputs :
 Outputs :
 */
static RwResEntry *
_rpD3D9PatchSkinAtomicInstance(PatchMesh *patchMesh,
                               RpAtomic *atomic,
                               RpGeometry *geom,
                               RwInt32 res)
{
    RwUInt32                numQuadPatches;
    RwUInt32                numTriPatches;
    RwUInt32                numVerts, numIndices, numMeshes;
    RpMeshHeader            *meshHeader;
    RpSkin                  *skin;
    RpHAnimHierarchy        *hierarchy;
    D3DVERTEXELEMENT9       declaration[18];
    RxD3D9InstanceData      *instancedData;
    RxD3D9VertexStream      vertexStream;
    RwUInt32                declarationIndex;
    RwUInt32                numTextureCoords;
    void                    *vertexDeclaration;
    RwUInt32                n, size;
    RpPatchInstanceAtomic   *instAtomic;
    RpPatchInstanceMesh     *instMesh;
    RwResEntry              *repEntry;
    RxD3D9ResEntryHeader    *resEntryHeader;
    RwUInt32                startIndex;
    RxVertexIndex           *indexBuffer;
    RwUInt32                *indexBuffer32;
    RwResEntry              **repEntryOwner;
    void                    *owner;

    RWFUNCTION(RWSTRING("_rpD3D9PatchAtomicInstance"));

    numQuadPatches = _rpPatchMeshGetNumQuadPatches(patchMesh);
    numTriPatches = _rpPatchMeshGetNumTriPatches(patchMesh);

    /* Total number of verts. */
    numVerts = (numQuadPatches * PATCHQUADNUMVERT(res + 1)) +
               (numTriPatches * PATCHTRINUMVERT(res + 1));

    /* Total number of indices and tris. */
    numIndices = (numQuadPatches * PATCHQUADNUMINDEX(res + 1)) +
                    (numTriPatches * PATCHTRINUMINDEX(res + 1));

    /* Add extra indices for connecting patches together. */
    numIndices += (numQuadPatches + numTriPatches - 1) * 2;

    /* Add extra for odd number of tri patches to preserve winding order. */
    if (res & 0x01)
    {
        numIndices += (numTriPatches);
    }

    /* Get skin info */
    skin = RpSkinGeometryGetSkin(geom);

    /* Get hierarchy */
    if (skin != NULL)
    {
        /* get hierarchy info */
        hierarchy = RpSkinAtomicGetHAnimHierarchy(atomic);
    }
    else
    {
        hierarchy = NULL;
    }

    /* Create temporary atomic info (creates extra space for extra meshes) */
    meshHeader = geom->mesh;
    numMeshes = meshHeader->numMeshes;
    numMeshes *= 1 + (numIndices / 65535);

    size = sizeof(RpPatchInstanceAtomic) +
        /*
        (numVerts * stride) +
        */
        (numMeshes * sizeof(RpPatchInstanceMesh)) +
        (numIndices * sizeof(RxVertexIndex));

    instAtomic = (RpPatchInstanceAtomic *)RwMalloc(size,
        rwID_PATCHPLUGIN | rwMEMHINTDUR_FUNCTION);

    instAtomic->indices = (RxVertexIndex *) & instAtomic->meshes[numMeshes];
    instAtomic->numMeshes = numMeshes;
    instAtomic->totalIndices = numIndices;
    instAtomic->totalVerts = numVerts;
    instAtomic->meshFlags = ((RpPatchMeshFlag)
                                _rpPatchMeshGetFlags(patchMesh));

    /*
        * Create vertex declaration
        */
    declarationIndex = 0;

    /* Positions */
    declaration[declarationIndex].Stream = 0;
    declaration[declarationIndex].Offset = 0;
    declaration[declarationIndex].Type = D3DDECLTYPE_FLOAT3;
    declaration[declarationIndex].Method = D3DDECLMETHOD_DEFAULT;
    declaration[declarationIndex].Usage = D3DDECLUSAGE_POSITION;
    declaration[declarationIndex].UsageIndex = 0;
    declarationIndex++;
    vertexStream.stride = sizeof(RwV3d);
    vertexStream.geometryFlags = rpGEOMETRYLOCKVERTICES;

    if (instAtomic->meshFlags & rpPATCHMESHNORMALS)
    {
        declaration[declarationIndex].Stream = 0;
        declaration[declarationIndex].Offset = vertexStream.stride;
        declaration[declarationIndex].Type = D3DDECLTYPE_FLOAT3;
        declaration[declarationIndex].Method = D3DDECLMETHOD_DEFAULT;
        declaration[declarationIndex].Usage = D3DDECLUSAGE_NORMAL;
        declaration[declarationIndex].UsageIndex = 0;
        declarationIndex++;
        vertexStream.stride += sizeof(RwV3d);
        vertexStream.geometryFlags |= rpGEOMETRYLOCKNORMALS;
    }

    if (instAtomic->meshFlags & rpPATCHMESHPRELIGHTS)
    {
        declaration[declarationIndex].Stream = 0;
        declaration[declarationIndex].Offset = vertexStream.stride;
        declaration[declarationIndex].Type = D3DDECLTYPE_D3DCOLOR;
        declaration[declarationIndex].Method = D3DDECLMETHOD_DEFAULT;
        declaration[declarationIndex].Usage = D3DDECLUSAGE_COLOR;
        declaration[declarationIndex].UsageIndex = 0;
        declarationIndex++;
        vertexStream.stride += sizeof(RwRGBA);
        vertexStream.geometryFlags |= rpGEOMETRYLOCKPRELIGHT;
    }

    /* Texture coordinates */
    numTextureCoords = _rpPatchMeshFlagGetNumTexCoords(instAtomic->meshFlags);
    for (n = 0; n < numTextureCoords; n++)
    {
        declaration[declarationIndex].Stream = 0;
        declaration[declarationIndex].Offset = vertexStream.stride;
        declaration[declarationIndex].Type = D3DDECLTYPE_FLOAT2;
        declaration[declarationIndex].Method = D3DDECLMETHOD_DEFAULT;
        declaration[declarationIndex].Usage = D3DDECLUSAGE_TEXCOORD;
        declaration[declarationIndex].UsageIndex = n;
        declarationIndex++;
        vertexStream.stride += sizeof(RwV2d);
        vertexStream.geometryFlags |= (rpGEOMETRYLOCKTEXCOORDS1 << n);
    }

    declaration[declarationIndex].Stream = 0xFF;
    declaration[declarationIndex].Offset = 0;
    declaration[declarationIndex].Type = D3DDECLTYPE_UNUSED;
    declaration[declarationIndex].Method = 0;
    declaration[declarationIndex].Usage = 0;
    declaration[declarationIndex].UsageIndex = 0;

    RwD3D9CreateVertexDeclaration(declaration, &vertexDeclaration);

    /* Create only one vertex buffer stored in the first instance data */
    vertexStream.managed = TRUE;
    vertexStream.dynamicLock = FALSE;

    if (FALSE == RwD3D9CreateVertexBuffer(vertexStream.stride,
                                            numVerts * (vertexStream.stride),
                                            &(vertexStream.vertexBuffer),
                                            &(vertexStream.offset)))
    {
        RwFree(instAtomic);

        RWRETURN(NULL);
    }

    instAtomic->stride = vertexStream.stride;

    /*
     *  Tesselate the patch directly over the vertex buffer
     */
    if (hierarchy != NULL)
    {
        RwUInt8             *vertices;
        RwUInt8             *normals;
        RpMorphTarget       * const morphTarget = &geom->morphTarget[0];
        RwV3d               * const originalVertices = morphTarget->verts;
        RwV3d               * const originalNormals = morphTarget->normals;

        const RwMatrix      *matrixArray;

        /* allocate space for skined vertices */
        vertices = (RwUInt8 *)RwMalloc(2 * (geom->numVertices) * sizeof(RwV3d),
                                       rwID_PATCHPLUGIN | rwMEMHINTDUR_FUNCTION);
        normals = vertices + (geom->numVertices) * sizeof(RwV3d);

        /* Vertex blending */
        /* Perform matrix blending */
        matrixArray = _rwD3D9SkinPrepareMatrix(atomic, skin, hierarchy);

#if defined(NOASM)
        _rpSkinGenericMatrixBlend(geom->numVertices,
                                    RpSkinGetVertexBoneWeights(skin),
                                    RpSkinGetVertexBoneIndices(skin),
                                    matrixArray,
                                    vertices,
                                    originalVertices,
                                    normals,
                                    originalNormals,
                                    sizeof(RwV3d));

#else
        #if !defined(NOSSEASM)
        if (_rwIntelSSEsupported())
        {
            _rpSkinIntelSSEMatrixBlend(geom->numVertices,
                                        RpSkinGetVertexBoneWeights(skin),
                                        RpSkinGetVertexBoneIndices(skin),
                                        matrixArray,
                                        vertices,
                                        originalVertices,
                                        normals,
                                        originalNormals,
                                        sizeof(RwV3d));
        }
        else
        #endif /* !defined(NOSSEASM) */

        {
            _rpSkinIntelx86MatrixBlend(geom->numVertices,
                                        RpSkinGetVertexBoneWeights(skin),
                                        RpSkinGetVertexBoneIndices(skin),
                                        matrixArray,
                                        (RwV3d *)vertices,
                                        originalVertices,
                                        (RwV3d *)normals,
                                        originalNormals,
                                        sizeof(RwV3d));
        }

#endif /* defined(NOASM) */

        /* Nasty change to cheat the tesselation function */
        morphTarget->verts = (RwV3d *)vertices;
        morphTarget->normals = (RwV3d *)normals;

        /*
         *  Tesselate the patch directly over the vertex buffer
         */
        if (SUCCEEDED(IDirect3DVertexBuffer9_Lock((LPDIRECT3DVERTEXBUFFER9)vertexStream.vertexBuffer,
                                                vertexStream.offset,
                                                numVerts * (vertexStream.stride),
                                                (RwUInt8 **)&(instAtomic->vertices),
                                                0)))
        {
            if (FALSE == _rpD3D9PatchInstanceAtomic(instAtomic, geom, res))
            {
                /* Free temp skined info */
                RwFree(vertices);

                RwD3D9DestroyVertexBuffer(vertexStream.stride,
                                            (vertexStream.stride) * numVerts,
                                            vertexStream.vertexBuffer,
                                            vertexStream.offset);

                RwFree(instAtomic);

                RWRETURN(NULL);
            }

            IDirect3DVertexBuffer9_Unlock((LPDIRECT3DVERTEXBUFFER9)vertexStream.vertexBuffer);
        }

        /* restore geom variables */
        morphTarget->verts = originalVertices;
        morphTarget->normals = originalNormals;

        /* Free temp skined info */
        RwFree(vertices);
    }
    else
    {
        if (SUCCEEDED(IDirect3DVertexBuffer9_Lock((LPDIRECT3DVERTEXBUFFER9)vertexStream.vertexBuffer,
                            vertexStream.offset,
                            numVerts * (vertexStream.stride),
                            (RwUInt8 **)&(instAtomic->vertices), 0)))
        {
            /* Interpolate */
            if (FALSE == _rpD3D9PatchInstanceAtomic(instAtomic, geom, res))
            {
                RwD3D9DestroyVertexBuffer(vertexStream.stride,
                                            (vertexStream.stride) * numVerts,
                                            vertexStream.vertexBuffer,
                                            vertexStream.offset);

                RwFree(instAtomic);

                RWRETURN(NULL);
            }

            IDirect3DVertexBuffer9_Unlock((LPDIRECT3DVERTEXBUFFER9)vertexStream.vertexBuffer);
        }
    }

    /* update numMeshes, changed inside _rpD3D9PatchInstanceAtomic */
    numMeshes = instAtomic->numMeshes;

    /* RxD3D9ResEntryHeader, stores serialNumber & numMeshes */
    size = sizeof(RxD3D9ResEntryHeader);

    /* RxD3D9InstanceData structures, one for each mesh */
    size += sizeof(RxD3D9InstanceData) * numMeshes;

    /* If the geometry has more than one morph target the resEntry in the
    * atomic is used else the resEntry in the geometry */
    if (RpGeometryGetNumMorphTargets(geom) != 1)
    {
        owner = (void *) atomic;
        repEntryOwner = &atomic->repEntry;
    }
    else
    {
        owner = (void *) geom;
        repEntryOwner = &geom->repEntry;
    }

    repEntry = RwResourcesAllocateResEntry(owner, repEntryOwner,
                                            size,
                                            _rwD3D9ResourceEntryInstanceDataDestroy);
    RWASSERT(NULL != repEntry);

    /* Blank the RpPatchInstanceAtomic & RxD3D9InstanceData's to '0' */
    memset((repEntry + 1), 0, size);

    /* Set the header info */
    resEntryHeader = (RxD3D9ResEntryHeader *) (repEntry + 1);
    resEntryHeader->serialNumber = meshHeader->serialNum;
    resEntryHeader->numMeshes = numMeshes;
    resEntryHeader->totalNumVertex = numVerts;

    /*
    * Create the index buffer if needed
    */
    indexBuffer = NULL;
    indexBuffer32 = NULL;

    resEntryHeader->indexBuffer = NULL;
    resEntryHeader->totalNumIndex = 0;

    if ((meshHeader->flags & rpMESHHEADERUNINDEXED) == 0)
    {
        /* Calculate total num indices */
        instMesh = instAtomic->meshes;
        while (numMeshes--)
        {
            resEntryHeader->totalNumIndex += instMesh->numIndices;

            instMesh++;
        }

        numMeshes = resEntryHeader->numMeshes;

        /* Initialize the index buffers pointers */
        if (resEntryHeader->totalNumIndex)
        {
            if (_rpPatchGlobals.platform.use32bitsIndexBuffers &&
                numVerts > 65535)
            {
                if (_rwD3D9IndexBuffer32bitsCreate( resEntryHeader->totalNumIndex,
                                                    &(resEntryHeader->indexBuffer)))
                {
                    IDirect3DIndexBuffer9_Lock((LPDIRECT3DINDEXBUFFER9)resEntryHeader->indexBuffer,
                                            0, 0, (RwUInt8 **)&indexBuffer32, 0);
                }
            }

            if (indexBuffer32 == NULL)
            {
                if (RwD3D9IndexBufferCreate(resEntryHeader->totalNumIndex, &(resEntryHeader->indexBuffer)))
                {
                    IDirect3DIndexBuffer9_Lock((LPDIRECT3DINDEXBUFFER9)resEntryHeader->indexBuffer,
                                            0, 0, (RwUInt8 **)&indexBuffer, 0);
                }
            }
        }
    }

    /* Primitive type */
    resEntryHeader->primType = D3DPT_TRIANGLESTRIP;

    /* Initialize the vertex buffers pointers */
    resEntryHeader->useOffsets = FALSE;

    resEntryHeader->vertexStream[0].vertexBuffer = vertexStream.vertexBuffer;
    resEntryHeader->vertexStream[0].offset = vertexStream.offset;
    resEntryHeader->vertexStream[0].stride = vertexStream.stride;
    resEntryHeader->vertexStream[0].geometryFlags = vertexStream.geometryFlags;
    resEntryHeader->vertexStream[0].managed = vertexStream.managed;
    resEntryHeader->vertexStream[0].dynamicLock = vertexStream.dynamicLock;

    for (n = 1; n < RWD3D9_MAX_VERTEX_STREAMS; n++)
    {
        resEntryHeader->vertexStream[n].vertexBuffer = NULL;
        resEntryHeader->vertexStream[n].offset = 0;
        resEntryHeader->vertexStream[n].stride = 0;
        resEntryHeader->vertexStream[n].geometryFlags = FALSE;
        resEntryHeader->vertexStream[n].managed = FALSE;
        resEntryHeader->vertexStream[n].dynamicLock = FALSE;
    }

    /* Vertex declaration */
    resEntryHeader->vertexDeclaration = vertexDeclaration;

    /* Get the first RxD3D9InstanceData pointer */
    instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

    /* Fill instance data and indices */
    startIndex = 0;

    instMesh = instAtomic->meshes;
    while (numMeshes--)
    {
        /*
            * Min vertex index,
            * (needed for instancing & reinstancing)
            */
        instancedData->minVert = instMesh->minVert;

        /* Number of vertices */
        instancedData->numVertices = instMesh->numVerts;

        /* The number of primitives */
        RWASSERT(instMesh->numIndices > 2);
        instancedData->numPrimitives = instMesh->numIndices - 2;

        /* Material */
        instancedData->material = instMesh->material;

        /* Vertex shader */
        instancedData->vertexShader = NULL;

        /* Initialize base index */
        instancedData->baseIndex = ((vertexStream.offset) / (vertexStream.stride)) + instancedData->minVert;

        /* Initialize vertex alpha */
        if ( instAtomic->meshFlags & rpPATCHMESHPRELIGHTS &&
                geom->preLitLum != NULL)
        {
            const RpMesh    *mesh;
            RwInt32         alpha;
            RxVertexIndex   *indices;
            RwInt32         numIndices;

            alpha = 0xff;

            mesh = instMesh->mesh;
            indices = mesh->indices;
            numIndices = mesh->numIndices;
            while (numIndices--)
            {
                alpha &= geom->preLitLum[*indices].alpha;

                indices++;
            }

            instancedData->vertexAlpha = (alpha != 0xff);
        }
        else
        {
            instancedData->vertexAlpha = FALSE;
        }

        /*
        * copy indices if needed
        */
        if (indexBuffer != NULL || indexBuffer32 != NULL)
        {
            instancedData->numIndex = instMesh->numIndices;
            instancedData->startIndex = startIndex;

            if(indexBuffer32 != NULL)
            {
                RwUInt32 i;

                for (i = 0; i < instMesh->numIndices; i++)
                {
                    *indexBuffer32++ = instMesh->indices[i];
                }
            }
            else
            {
                memcpy(indexBuffer, instMesh->indices, sizeof(RxVertexIndex) * (instMesh->numIndices));

                indexBuffer += instMesh->numIndices;
            }

            startIndex += instMesh->numIndices;
        }
        else
        {
            instancedData->numIndex = 0;
            instancedData->startIndex = 0;
        }

        instancedData++;
        instMesh++;
    }

    if (indexBuffer != NULL || indexBuffer32 != NULL)
    {
        IDirect3DIndexBuffer9_Unlock((LPDIRECT3DINDEXBUFFER9)resEntryHeader->indexBuffer);
    }

    RwFree(instAtomic);

    geom->lockedSinceLastInst = 0;

    RWRETURN(repEntry);
}

/*****************************************************************************
 _rwD3D9PatchSkinAtomicAllInOneNodeNode

 _refineNode

 Inputs :
 Outputs :
 */
RwBool
_rwD3D9PatchSkinAtomicAllInOneNodeNode( RxPipelineNodeInstance *self,
                            const RxPipelineNodeParam *params )
{
    RpNodePatchData *patchData;

    RpAtomic        *atomic;
    RpGeometry      *geom;
    RwUInt32        geomFlags;

    RpSkin          *skin;

    PatchAtomicData *atomicData;
    RpMeshHeader    *meshHeader;
    RwInt32         res;
    RwInt32         numMeshes;

    RwResEntry      *repEntry;

    PatchMesh       *patchMesh;
    RwUInt32        numQuadPatches;
    RwUInt32        numTriPatches;
    RwUInt32        numVerts;

    RwMatrix            *matrix;
    RwUInt32            lighting;

    RWFUNCTION(RWSTRING("_rwD3D9PatchSkinAtomicAllInOneNodeNode"));

    RWASSERT(NULL != self);
    RWASSERT(NULL != params);

    patchData = (RpNodePatchData *) self->privateData;
    RWASSERT(NULL != patchData);

    /* Cheap early out if this node's toggled off */
    if (0 && (patchData->patchOn == FALSE))
    {
        RxPacketDispatch(NULL, PATCHPASSTHROUGH, self);
        RWRETURN(TRUE);
    }

    atomic = (RpAtomic *) RxPipelineNodeParamGetData(params);
    RWASSERT(NULL != atomic);

    atomicData = PATCHATOMICGETDATA(atomic);
    RWASSERT(NULL != atomicData);

    geom = RpAtomicGetGeometry(atomic);
    RWASSERT(NULL != geom);

    geomFlags = RpGeometryGetFlags(geom);

    numVerts = geom->numVertices;
    /* If there ain't vertices, we cain't make packets... */
    if (numVerts <= 0)
    {
        /* Don't execute the rest of the pipeline */
        RWRETURN(TRUE);
    }

    meshHeader = geom->mesh;
    numMeshes = meshHeader->numMeshes;
    /* Early out if no meshes */
    if (numMeshes <= 0)
    {
        /* If the app wants to use plugin data to make packets, it
         * should use its own instancing function. If we have verts
         * here, we need meshes too in order to build a packet. */
        RWRETURN(TRUE);
    }

    /* Get the patch mesh. info */
    patchMesh = atomicData->patchMesh;
    RWASSERT(NULL != patchMesh);

    numQuadPatches = _rpPatchMeshGetNumQuadPatches(patchMesh);
    numTriPatches = _rpPatchMeshGetNumTriPatches(patchMesh);

    /* Query for the LOD. */
    res = (atomicData->lod.callback)(atomic, atomicData->lod.userData);

    /* Total number of verts. */
    numVerts = (numQuadPatches * PATCHQUADNUMVERT(res + 1)) +
               (numTriPatches * PATCHTRINUMVERT(res + 1));

    /* If the geometry has more than one morph target the resEntry in the
     * atomic is used else the resEntry in the geometry */
    if (RpGeometryGetNumMorphTargets(geom) != 1)
    {
        repEntry = atomic->repEntry;
    }
    else
    {
        repEntry = geom->repEntry;
    }

    if (NULL != repEntry)
    {
        RxD3D9ResEntryHeader    *resEntryHeader;

        /* If anything has changed, we should re-instance */
        resEntryHeader = (RxD3D9ResEntryHeader *) (repEntry + 1);

        if ( (resEntryHeader->serialNumber != meshHeader->serialNum) ||
             (numVerts != resEntryHeader->totalNumVertex) )
        {
            /* Things have changed, destroy resources to force reinstance */
            RwResourcesFreeResEntry(repEntry);
            repEntry = NULL;
        }
    }

    /* Get skin info */
    skin = RpSkinGeometryGetSkin(geom);

    /* We need to re-generate the mesh due to some changes. */
    if (repEntry == NULL)
    {
        repEntry = _rpD3D9PatchSkinAtomicInstance(patchMesh, atomic, geom, res);
        if (repEntry == NULL)
        {
            RWRETURN(FALSE);
        }
    }
    else
    {
        if (skin != NULL ||
            (geom->lockedSinceLastInst & (rpGEOMETRYLOCKVERTICES | rpGEOMETRYLOCKNORMALS)) != 0)
        {
            RwInt32                 numIndices;
            RwUInt32                size;
            RpPatchMeshFlag         meshFlags;
            RpPatchInstanceAtomic   *instAtomic;
            RxD3D9ResEntryHeader    *resEntryHeader;
            RxD3D9VertexStream      *vertexStream;
            RpHAnimHierarchy        *hierarchy;

            resEntryHeader = (RxD3D9ResEntryHeader *) (repEntry + 1);

            /* I assume positions and normals are on the first stream */
            vertexStream = &(resEntryHeader->vertexStream[0]);

            /* Calculate vertex stride and the fvf */
            meshFlags = ((RpPatchMeshFlag)
                        _rpPatchMeshGetFlags(patchMesh));

            /* Total number of indices and tris. */
            numIndices = (numQuadPatches * PATCHQUADNUMINDEX(res + 1)) +
                         (numTriPatches * PATCHTRINUMINDEX(res + 1));

            /* Add extra indices for connecting patches together. */
            numIndices += (numQuadPatches + numTriPatches - 1) * 2;

            /* Add extra for odd number of tri patches to preserve winding order. */
            if (res & 0x01)
            {
                numIndices += (numTriPatches);
            }

            /* Create temporary atomic info */
            size = sizeof(RpPatchInstanceAtomic) +
                   (numMeshes * sizeof(RpPatchInstanceMesh));

            instAtomic = (RpPatchInstanceAtomic *)RwMalloc(size,
                rwID_PATCHPLUGIN | rwMEMHINTDUR_FUNCTION);

            instAtomic->indices = NULL;
            instAtomic->numMeshes = numMeshes;
            instAtomic->totalIndices = numIndices;
            instAtomic->totalVerts = numVerts;
            instAtomic->meshFlags = meshFlags;
            instAtomic->stride = vertexStream->stride;

            /* Check skin */
            if (skin != NULL)
            {
                /* get hierarchy info */
                hierarchy = RpSkinAtomicGetHAnimHierarchy(atomic);
            }
            else
            {
                hierarchy = NULL;
            }

            if (hierarchy != NULL)
            {
                RwUInt8             *vertices;
                RwUInt8             *normals;
                RpMorphTarget       * const morphTarget = &geom->morphTarget[0];
                RwV3d               * const originalVertices = morphTarget->verts;
                RwV3d               * const originalNormals = morphTarget->normals;

                const RwMatrix      *matrixArray;

                /* allocate space for skined vertices */
                vertices = (RwUInt8 *)RwMalloc(2 * (geom->numVertices) * sizeof(RwV3d),
                                               rwID_PATCHPLUGIN | rwMEMHINTDUR_FUNCTION);
                normals = vertices + (geom->numVertices) * sizeof(RwV3d);


                /* Vertex blending */
                /* Perform matrix blending */
                matrixArray = _rwD3D9SkinPrepareMatrix(atomic, skin, hierarchy);

    #if defined(NOASM)
                _rpSkinGenericMatrixBlend(geom->numVertices,
                                        RpSkinGetVertexBoneWeights(skin),
                                        RpSkinGetVertexBoneIndices(skin),
                                        matrixArray,
                                        vertices,
                                        originalVertices,
                                        normals,
                                        originalNormals,
                                        sizeof(RwV3d));

    #else

                #if !defined(NOSSEASM)
                if (_rwIntelSSEsupported())
                {
                    _rpSkinIntelSSEMatrixBlend(geom->numVertices,
                                            RpSkinGetVertexBoneWeights(skin),
                                            RpSkinGetVertexBoneIndices(skin),
                                            matrixArray,
                                            vertices,
                                            originalVertices,
                                            normals,
                                            originalNormals,
                                            sizeof(RwV3d));
                }
                else
                #endif /* !defined(NOSSEASM) */

                {
                    _rpSkinIntelx86MatrixBlend(geom->numVertices,
                                            RpSkinGetVertexBoneWeights(skin),
                                            RpSkinGetVertexBoneIndices(skin),
                                            matrixArray,
                                            (RwV3d *)vertices,
                                            originalVertices,
                                            (RwV3d *)normals,
                                            originalNormals,
                                            sizeof(RwV3d));
                }

    #endif /* defined(NOASM) */

                /* Nasty change to cheat the tesselation function */
                morphTarget->verts = (RwV3d *)vertices;
                morphTarget->normals = (RwV3d *)normals;

                /*
                *  Tesselate the patch directly over the vertex buffer
                */
                if (vertexStream->managed)
                {
                    IDirect3DVertexBuffer9_Lock((LPDIRECT3DVERTEXBUFFER9)vertexStream->vertexBuffer,
                                                vertexStream->offset,
                                                numVerts * (vertexStream->stride),
                                                (RwUInt8 **)&(instAtomic->vertices),
                                                0);
                }
                else
                {
                    if (vertexStream->vertexBuffer != NULL &&
                        vertexStream->offset == 0 &&
                        vertexStream->dynamicLock == FALSE)
                    {
                        IDirect3DVertexBuffer9_Lock((LPDIRECT3DVERTEXBUFFER9)vertexStream->vertexBuffer,
                                                    0, 0,
                                                    (RwUInt8 **)&(instAtomic->vertices),
                                                    D3DLOCK_DISCARD);
                    }
                    else
                    {
                        RwUInt32 newOffset;

                        /* Use Dynamic Lock */
                        RwD3D9DynamicVertexBufferLock(vertexStream->stride,
                                                      numVerts,
                                                      (void **)&(vertexStream->vertexBuffer),
                                                      (void **)&(instAtomic->vertices),
                                                      &newOffset);

                        newOffset = newOffset * (vertexStream->stride);

                        vertexStream->offset = newOffset;

                        if (resEntryHeader->useOffsets == FALSE)
                        {
                            RwUInt32            numMeshes;
                            RxD3D9InstanceData  *instancedData;

                            numMeshes = resEntryHeader->numMeshes;
                            instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

                            do
                            {
                                instancedData->baseIndex = instancedData->minVert + (newOffset / (vertexStream->stride));

                                instancedData++;
                            }
                            while (--numMeshes);
                        }
                    }
                }

                _rpPatchReInstanceAtomic(instAtomic, geom, res);

                IDirect3DVertexBuffer9_Unlock((LPDIRECT3DVERTEXBUFFER9)vertexStream->vertexBuffer);

                /* restore geom variables */
                morphTarget->verts = originalVertices;
                morphTarget->normals = originalNormals;

                /* Free temp skined info */
                RwFree(vertices);
            }
            else
            {
                /*
                *  Tesselate the patch directly over the vertex buffer
                */
                if(geom->lockedSinceLastInst & (rpGEOMETRYLOCKVERTICES | rpGEOMETRYLOCKNORMALS))
                {
                    if (vertexStream->managed)
                    {
                        IDirect3DVertexBuffer9_Lock((LPDIRECT3DVERTEXBUFFER9)vertexStream->vertexBuffer,
                                                    vertexStream->offset,
                                                    numVerts * (vertexStream->stride),
                                                    (RwUInt8 **)&(instAtomic->vertices),
                                                    0);
                    }
                    else
                    {
                        IDirect3DVertexBuffer9_Lock((LPDIRECT3DVERTEXBUFFER9)vertexStream->vertexBuffer,
                                                    0, 0,
                                                    (RwUInt8 **)&(instAtomic->vertices),
                                                    D3DLOCK_DISCARD);
                    }

                    _rpPatchReInstanceAtomic(instAtomic, geom, res);

                    IDirect3DVertexBuffer9_Unlock((LPDIRECT3DVERTEXBUFFER9)vertexStream->vertexBuffer);
                }
            }

            RwFree(instAtomic);

            geom->lockedSinceLastInst = 0;
        }

        RwResourcesUseResEntry(repEntry);
    }

    /*
     * Set up lights
     */
    _rwD3D9AtomicDefaultLightingCallback((void *)atomic);

    /*
     * Set the world transform
     */
    matrix = RwFrameGetLTM(RpAtomicGetFrame(atomic));

    RwD3D9SetTransformWorld(matrix);

    RwD3D9GetRenderState(D3DRS_LIGHTING, &lighting);

    if (lighting)
    {
        if (_rpPatchGlobals.platform.hardwareTL)
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
    if (patchData->renderCallback != NULL)
    {
        patchData->renderCallback(repEntry, atomic, rpATOMIC, RpGeometryGetFlags(geom));
    }

#ifdef RWMETRICS
    /* Now update our metrics statistics */
    RWSRCGLOBAL(metrics)->numVertices  += numVerts;
    RWSRCGLOBAL(metrics)->numTriangles += (numQuadPatches * PATCHQUADNUMTRI(res + 1)) +
                                          (numTriPatches * PATCHTRINUMTRI(res + 1));
#endif

    RWRETURN(TRUE);
}

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/
