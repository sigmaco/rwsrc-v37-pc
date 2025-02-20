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

#include "noded3d8patchatomicinstance.h"
#include "noded3d8patchskinatomicinstance.h"

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
 _rwD3D8PatchSkinAtomicAllInOneNodeNode

 _refineNode

 Inputs :
 Outputs :
 */
RwBool
_rwD3D8PatchSkinAtomicAllInOneNodeNode( RxPipelineNodeInstance *self,
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
    RwResEntry      **repEntryOwner;

    PatchMesh       *patchMesh;
    RwUInt32        numQuadPatches;
    RwUInt32        numTriPatches;
    RwUInt32         numVerts;

    void            *owner;

    RxD3D8ResEntryHeader    *resEntryHeader = (RxD3D8ResEntryHeader *) NULL;

    RwMatrix            *matrix;
    RwUInt32            lighting;

    RWFUNCTION(RWSTRING("_rwD3D8PatchSkinAtomicAllInOneNodeNode"));

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
        owner = (void *) atomic;
        repEntryOwner = &atomic->repEntry;
        repEntry = atomic->repEntry;
    }
    else
    {
        owner = (void *) geom;
        repEntryOwner = &geom->repEntry;
        repEntry = geom->repEntry;
    }

    if (NULL != repEntry)
    {
        /* If anything has changed, we should re-instance */
        resEntryHeader = (RxD3D8ResEntryHeader *) (repEntry + 1);

        if ( (resEntryHeader->serialNumber != meshHeader->serialNum) ||
             (numVerts != _rwD3D8PatchGetNumTotalVertices(repEntry)) )
        {
            /* Things have changed, destroy resources to force reinstance */
            RwResourcesFreeResEntry(repEntry);
            repEntry = NULL;
        }
    }

    /* Get skin info */
    skin = RpSkinGeometryGetSkin(geom);

    /* We need to re-generate the mesh due to some changes. */
    if (repEntry == NULL ||
        skin != NULL ||
        (geom->lockedSinceLastInst & (rpGEOMETRYLOCKVERTICES | rpGEOMETRYLOCKNORMALS)) != 0)
    {
        RwInt32                 numIndices;
        RwUInt32                size;
        RwUInt32                fvf;
        RwUInt32                stride;
        RpPatchMeshFlag         meshFlags;
        RpPatchInstanceAtomic   *instAtomic;
        RpPatchInstanceMesh     *instMesh;
        RxD3D8InstanceData      *instancedData;
        LPDIRECT3DVERTEXBUFFER8 vertexBuffer;
        RwUInt32                baseIndex;
        RwBool                  bReinstance;
        RpHAnimHierarchy        *hierarchy;

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
            numIndices += (numTriPatches);

        if (repEntry == NULL)
        {
            fvf = D3DFVF_XYZ;
            stride = sizeof(RwV3d);

            if (meshFlags & rpPATCHMESHNORMALS)
            {
                fvf |= D3DFVF_NORMAL;
                stride += sizeof(RwV3d);
            }

            if (meshFlags & rpPATCHMESHPRELIGHTS)
            {
                fvf |= D3DFVF_DIFFUSE;
                stride += sizeof(RwRGBA);
            }

            /* Texture coordinates */
            switch(_rpPatchMeshFlagGetNumTexCoords(meshFlags))
            {
                case 1:
                    stride += 1 * sizeof(RwTexCoords);
                    fvf |= D3DFVF_TEX1;
                    break;
                case 2:
                    stride += 2 * sizeof(RwTexCoords);
                    fvf |= D3DFVF_TEX2;
                    break;
                case 3:
                    stride += 3 * sizeof(RwTexCoords);
                    fvf |= D3DFVF_TEX3;
                    break;
                case 4:
                    stride += 4 * sizeof(RwTexCoords);
                    fvf |= D3DFVF_TEX4;
                    break;
                case 5:
                    stride += 5 * sizeof(RwTexCoords);
                    fvf |= D3DFVF_TEX5;
                    break;
                case 6:
                    stride += 6 * sizeof(RwTexCoords);
                    fvf |= D3DFVF_TEX6;
                    break;
                case 7:
                    stride += 7 * sizeof(RwTexCoords);
                    fvf |= D3DFVF_TEX7;
                    break;
                case 8:
                    stride += 8 * sizeof(RwTexCoords);
                    fvf |= D3DFVF_TEX8;
                    break;
            }

            /* Create only one vertex buffer stored in the first instance data */
            if (FALSE == _rxD3D8VertexBufferManagerCreate(fvf,
                                                          numVerts * stride,
                                                          (void **)&vertexBuffer,
                                                          &baseIndex))
            {
                RWRETURN(FALSE);
            }

            instancedData = NULL;

            /* Reserve extra space for added meshes */
            numMeshes *= 1 + (numIndices / 65535);

            /* Create temporary atomic info */
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

            instAtomic->meshFlags = meshFlags;
            instAtomic->stride = stride;

            bReinstance = FALSE;
        }
        else
        {
            /* Get the first RxD3D8InstanceData pointer */
            instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

            fvf = instancedData->vertexShader;
            stride = instancedData->stride;

            vertexBuffer = ((LPDIRECT3DVERTEXBUFFER8)
                            instancedData->vertexBuffer);
            baseIndex = (instancedData->baseIndex - instancedData->minVert);

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
            instAtomic->stride = stride;

            bReinstance = TRUE;
        }

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
            matrixArray = _rwD3D8SkinPrepareMatrix(atomic, skin, hierarchy);

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
#endif
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
#endif

            /* Nasty change to cheat the tesselation function */
            morphTarget->verts = (RwV3d *)vertices;
            morphTarget->normals = (RwV3d *)normals;

            /*
             *  Tesselate the patch directly over the vertex buffer
             */
            if (SUCCEEDED(IDirect3DVertexBuffer8_Lock( vertexBuffer,
                                                       baseIndex * stride,
                                                       numVerts * stride,
                                                       (RwUInt8 **)&(instAtomic->vertices),
                                                       D3DLOCK_NOSYSLOCK)))
            {
                if (bReinstance)
                {
                    _rpPatchReInstanceAtomic(instAtomic, geom, res);
                }
                else
                {
                    _rpD3D8PatchInstanceAtomic(instAtomic, geom, res);
                }

                IDirect3DVertexBuffer8_Unlock(vertexBuffer);
            }

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
            if (SUCCEEDED(IDirect3DVertexBuffer8_Lock( vertexBuffer,
                                                       baseIndex * stride,
                                                       numVerts * stride,
                                                       (RwUInt8 **)&(instAtomic->vertices),
                                                       D3DLOCK_NOSYSLOCK)))
            {
                if (bReinstance)
                {
                    if(geom->lockedSinceLastInst & (rpGEOMETRYLOCKVERTICES | rpGEOMETRYLOCKNORMALS))
                    {
                        _rpPatchReInstanceAtomic(instAtomic, geom, res);
                    }
                }
                else
                {
                    _rpD3D8PatchInstanceAtomic(instAtomic, geom, res);
                }

                IDirect3DVertexBuffer8_Unlock(vertexBuffer);
            }
        }

        /* Fill instance data and indices */
        if (!bReinstance)
        {
            /* update numMeshes, changed inside _rpD3D8PatchInstanceAtomic */
            numMeshes = instAtomic->numMeshes;

            /* RxD3D8ResEntryHeader, stores serialNumber & numMeshes */
            size = sizeof(RxD3D8ResEntryHeader);

            /* RxD3D8InstanceData structures, one for each mesh */
            size += sizeof(RxD3D8InstanceData) * numMeshes;

            repEntry = RwResourcesAllocateResEntry(owner, repEntryOwner,
                                                   size,
                                                   _rwD3D8PatchDestroyVertexBuffer);
            RWASSERT(NULL != repEntry);

            /* Blank the RpPatchInstanceAtomic & RxD3D8InstanceData's to '0' */
            memset((repEntry + 1), 0, size);

            /* Set the header info */
            resEntryHeader = (RxD3D8ResEntryHeader *) (repEntry + 1);
            resEntryHeader->serialNumber = meshHeader->serialNum;
            resEntryHeader->numMeshes = (RwUInt16)numMeshes;

            /* Get the first RxD3D8InstanceData pointer */
            instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

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

                /* Primitive type */
                instancedData->primType = D3DPT_TRIANGLESTRIP;

                /* The number of indices */
                instancedData->numIndices = instMesh->numIndices;

                /* Material */
                instancedData->material = instMesh->material;

                /* Vertex shader */
                instancedData->vertexShader = fvf;

                /* The vertex format stride */
                instancedData->stride = stride;

                /* Initialize the vertex buffers pointers */
                instancedData->vertexBuffer = vertexBuffer;

                /* Initialize vertex buffer managed to FALSE */
                instancedData->baseIndex = baseIndex + instancedData->minVert;

                instancedData->managed = TRUE;

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
                 * Set the index buffer
                 */

                /* Initialize the index buffers pointers */
                instancedData->indexBuffer = NULL;

                if (_rpPatchGlobals.platform.use32bitsIndexBuffers &&
                    (instancedData->baseIndex + instancedData->numVertices) > 65535)
                {
                    if (_rwD3D8IndexBuffer32bitsCreate( instancedData->numIndices,
                                                        &(instancedData->indexBuffer)))
                    {
                        RwUInt32   *indexBuffer;

                        if (D3D_OK == IDirect3DIndexBuffer8_Lock((LPDIRECT3DINDEXBUFFER8)instancedData->indexBuffer,
                                                                 0, 0, (RwUInt8 **)&indexBuffer, 0))
                        {
                            RwInt32     i;

                            for (i = 0; i < instancedData->numIndices; i++)
                            {
                                indexBuffer[i] = instMesh->indices[i];
                            }

                            IDirect3DIndexBuffer8_Unlock((LPDIRECT3DINDEXBUFFER8)instancedData->indexBuffer);
                        }
                    }
                    else
                    {
                        instancedData->indexBuffer = NULL;
                    }
                }

                if (instancedData->indexBuffer == NULL)
                {
                    if (RwD3D8IndexBufferCreate(instancedData->numIndices,
                                                &(instancedData->indexBuffer)))
                    {
                        RxVertexIndex   *indexBuffer;

                        if (D3D_OK == IDirect3DIndexBuffer8_Lock((LPDIRECT3DINDEXBUFFER8)instancedData->indexBuffer,
                                                                 0, 0, (RwUInt8 **)&indexBuffer, 0))
                        {
                            memcpy(indexBuffer, instMesh->indices, sizeof(RxVertexIndex) * instancedData->numIndices);

                            IDirect3DIndexBuffer8_Unlock((LPDIRECT3DINDEXBUFFER8)instancedData->indexBuffer);
                        }
                    }
                }


                instancedData++;
                instMesh++;
            }
        }
        else
        {
            RwResourcesUseResEntry(repEntry);
        }

        RwFree(instAtomic);

        geom->lockedSinceLastInst = 0;
    }
    else
    {
        RwResourcesUseResEntry(repEntry);
    }

    /*
     * Set up lights
     */
    _rwD3D8AtomicDefaultLightingCallback((void *)atomic);

    /*
     * Set the world transform
     */
    matrix = RwFrameGetLTM(RpAtomicGetFrame(atomic));

    RwD3D8SetTransformWorld(matrix);

    RwD3D8GetRenderState(D3DRS_LIGHTING, &lighting);

    if (lighting)
    {
        if (_rpPatchGlobals.platform.hardwareTL)
        {
            RwD3D8SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
        }
        else if (!rwMatrixTestFlags(matrix, (rwMATRIXTYPENORMAL | rwMATRIXINTERNALIDENTITY)))
        {
            const RwReal minlimit = 0.9f;
            const RwReal maxlimit = 1.1f;
            RwReal length;

            length = RwV3dDotProduct(&(matrix->right), &(matrix->right));

            if ( (FLOATASINT(length) > FLOATASINT(maxlimit)) || (FLOATASINT(length) < FLOATASINT(minlimit)) )
            {
                RwD3D8SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
            }
            else
            {
                length = RwV3dDotProduct(&(matrix->up), &(matrix->up));

                if ( (FLOATASINT(length) > FLOATASINT(maxlimit)) || (FLOATASINT(length) < FLOATASINT(minlimit)) )
                {
                    RwD3D8SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
                }
                else
                {
                    length = RwV3dDotProduct(&(matrix->at), &(matrix->at));

                    if ( (FLOATASINT(length) > FLOATASINT(maxlimit)) || (FLOATASINT(length) < FLOATASINT(minlimit)) )
                    {
                        RwD3D8SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
                    }
                    else
                    {
                        RwD3D8SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);
                    }
                }
            }
        }
        else
        {
            RwD3D8SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);
        }
    }
    else
    {
        RwD3D8SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);
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
