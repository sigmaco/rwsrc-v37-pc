/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rppatch.h"

#include "patch.h"
#include "patchatomic.h"
#include "patchmesh.h"

#include "noded3d8patchatomicinstance.h"

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

 /****************************************************************************
 _rwD3D8PatchGetNumTotalVertices

 Purpose:

 On entry:

 On exit :
*/
RwUInt32
_rwD3D8PatchGetNumTotalVertices(const RwResEntry *repEntry)
{
    const RxD3D8ResEntryHeader  *resEntryHeader;
    const RxD3D8InstanceData    *instancedData;
    RwInt32                     numMeshes;
    RwUInt32                    numVerts;

    RWFUNCTION(RWSTRING("_rwD3D8PatchGetNumTotalVertices"));

    /* Get the instanced data */
    resEntryHeader = (const RxD3D8ResEntryHeader *)(repEntry + 1);
    instancedData = (const RxD3D8InstanceData *)(resEntryHeader + 1);

    /* Get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;

    numVerts = 0;
    while (numMeshes--)
    {
        numVerts += instancedData->numVertices;

        instancedData++;
    }

    RWRETURN(numVerts);
}

/*****************************************************************************
 _rwD3D8PatchDestroyVertexBuffer

 Destroy all the buffer memory

 Inputs :
 Outputs :
 */
void
_rwD3D8PatchDestroyVertexBuffer( RwResEntry *repEntry )
{
    RxD3D8ResEntryHeader    *resEntryHeader;
    RxD3D8InstanceData      *instancedData;
    LPDIRECT3DVERTEXBUFFER8 vertexBuffer;
    RwUInt32                numMeshes;
    RwUInt32                fvf;
    RwUInt32                stride;
    RwUInt32                numVertices;
    RwUInt32                baseIndex;

    RWFUNCTION(RWSTRING("_rwD3D8PatchDestroyVertexBuffer"));

    resEntryHeader = (RxD3D8ResEntryHeader *)(repEntry + 1);

    /* Get the instanced data structures */
    instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

    /* Save shared data */
    vertexBuffer = (LPDIRECT3DVERTEXBUFFER8)instancedData->vertexBuffer;
    fvf = instancedData->vertexShader;
    stride = instancedData->stride;
    baseIndex = (instancedData->baseIndex - instancedData->minVert);

    /* Get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;
    numVertices = 0;

    while (numMeshes--)
    {
        /* Destroy the index buffer */
        if (instancedData->indexBuffer)
        {
            IDirect3DIndexBuffer8_Release((LPDIRECT3DINDEXBUFFER8)instancedData->indexBuffer);
            instancedData->indexBuffer = NULL;
        }

        instancedData->vertexBuffer = NULL;

        numVertices += instancedData->numVertices;

        /* On to the next RxD3D8InstanceData */
        instancedData++;
    }

    /* Destroy the only one vertex buffer that we have */
    if (vertexBuffer)
    {
        _rxD3D8VertexBufferManagerDestroy(fvf, stride * numVertices,
                                            vertexBuffer, baseIndex);
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpD3D8PatchInstanceAtomic

 Generate the atomic refinememt.

 Inputs:
 Outputs:
 */
RwBool
_rpD3D8PatchInstanceAtomic( RpPatchInstanceAtomic *instAtomic,
                            RpGeometry *geom,
                            RwInt32 res )
{
    RpMeshHeader        *meshHeader;
    RpMesh              *mesh;

    RpPatchInstanceMesh *instMesh;

    RtBezierMatrix cptMatrix;

    RwInt32 base;
    RwInt32 firstpatch;
    RwInt32 i;
    RwInt32 j;
    RwInt32 k;
    RwInt32 l;
    RwInt32 minVert;
    RwInt32 numCtrlIndices;
    RwInt32 numMesh;
    RwInt32 numQuadIndices;
    RwInt32 numQuadVerts;
    RwInt32 numTriIndices;
    RwInt32 numTriVerts;
    RwInt32 numTextureCoords;
    RwInt32 numTextureCoordsPatch;

    RwTexCoords     cptTexCoords[4];
    const RwV3d     *vert;
    const RwV3d     *nrm;
    const RwRGBA    *preLit;

    RpPatchMeshFlag meshFlags;
    RwUInt32        stride;
    RwUInt8         *pt;
    RxVertexIndex   *cptIdx;
    RxVertexIndex   *idx;

    RwInt32 numMeshResult;

    RWFUNCTION(RWSTRING("_rpD3D8PatchInstanceAtomic"));

    vert = geom->morphTarget[0].verts;
    nrm = geom->morphTarget[0].normals;
    preLit = geom->preLitLum;

    meshHeader = geom->mesh;
    mesh = (RpMesh *)((RwUInt8 *)(meshHeader + 1) +
                      meshHeader->firstMeshOffset);
    numMesh = meshHeader->numMeshes;

    meshFlags = instAtomic->meshFlags;
    stride = instAtomic->stride;
    pt = (RwUInt8 *)instAtomic->vertices;
    idx = instAtomic->indices;
    instMesh = instAtomic->meshes;
    numTextureCoordsPatch = _rpPatchMeshFlagGetNumTexCoords(meshFlags);

    numMeshResult = meshHeader->numMeshes;

    /* Calculate patch result info */
    numQuadIndices = PATCHQUADNUMINDEX((res + 1));
    numQuadVerts = PATCHQUADNUMVERT((res + 1));
    numTriIndices = PATCHTRINUMINDEX((res + 1));
    numTriVerts = PATCHTRINUMVERT((res + 1));
    minVert = 0;

    numTextureCoords = RpGeometryGetNumTexCoordSets(geom);

    /*
     * We write an index for tri patches for odd number of patches.
     * This is to preserve the winding order.
     */
    if(res & 0x01)
        numTriIndices++;

    while(numMesh)
    {
        firstpatch = 1;
        base = 0;
        instMesh->indices = idx;
        instMesh->material = mesh->material;
        instMesh->mesh = mesh;
        instMesh->numIndices = 0;
        instMesh->minVert = minVert;
        instMesh->numVerts = 0;

        /* Extract the quad control points from the mesh index. */
        numCtrlIndices = mesh->numIndices;
        if (numCtrlIndices)
        {
            cptIdx = mesh->indices;
            while (numCtrlIndices > 0)
            {
                RwBool  isTriPatch = FALSE;
                RwBool  addMesh = FALSE;
                RwUInt8 *vertex = pt;

                /* We use the fifth texCoords to flag if its a quad or tri
                 * patch
                 */
                if( numTextureCoords &&
                    geom->texCoords[0][cptIdx[5]].u != (RwReal)0.0f )
                {
                    isTriPatch = TRUE;

                    /* Check triangles limit */
                    if ( (instMesh->numIndices + numTriIndices) > 65535)
                    {
                        addMesh = TRUE;
                    }
                }
                else
                {
                    /* Check triangles limit */
                    if ( (instMesh->numIndices + numQuadIndices) > 65535)
                    {
                        addMesh = TRUE;
                    }
                }

                if (addMesh &&
                    numMeshResult < instAtomic->numMeshes)
                {
                    instMesh++;

                    firstpatch = 1;
                    base = 0;
                    instMesh->indices = idx;
                    instMesh->material = mesh->material;
                    instMesh->mesh = mesh;
                    instMesh->numIndices = 0;
                    instMesh->minVert = minVert;
                    instMesh->numVerts = 0;

                    numMeshResult++;
                }

                /* Duplicate the last index of the previous and the first
                 * index of the next patch. This connects the two together
                 * into a single tristrip.
                 */
                if(!firstpatch)
                {
                    idx[0] = idx[-1];
                    idx[1] = (RxVertexIndex) base;
                    idx += 2;
                    instMesh->numIndices += 2;
                }

                if( isTriPatch )
                {
                    /* Tri patch. */

                    /* Set up the control point array. */
                    l = 0;
                    for( j = 0; j < 4; j++ )
                    {
                        for( i = 0; i < (4 - j); i++ )
                        {
                            k = cptIdx[l];
                            cptMatrix[i][j].x = vert[k].x;
                            cptMatrix[i][j].y = vert[k].y;
                            cptMatrix[i][j].z = vert[k].z;
                            l++;
                        }

                        for( ; i < 4; i++ )
                        {
                            cptMatrix[i][j].x = (RwReal) 0.0;
                            cptMatrix[i][j].y = (RwReal) 0.0;
                            cptMatrix[i][j].z = (RwReal) 0.0;
                        }
                    }

                    _rpPatchRefineTriVector( (RwV3d *)vertex,
                                             stride,
                                             cptMatrix,
                                             res);

                    vertex += sizeof(RwV3d);

                    /* Set up the normals */
                    if( meshFlags & rpPATCHMESHNORMALS )
                    {
                        if( nrm != NULL )
                        {
                            l = 0;
                            for( j = 0; j < 4; j++ )
                            {
                                for( i = 0; i < (4 - j); i++ )
                                {
                                    k = cptIdx[l];
                                    cptMatrix[i][j].x = nrm[k].x;
                                    cptMatrix[i][j].y = nrm[k].y;
                                    cptMatrix[i][j].z = nrm[k].z;
                                    l++;
                                }

                                for( ; i < 4; i++ )
                                {
                                    cptMatrix[i][j].x = (RwReal) 0.0;
                                    cptMatrix[i][j].y = (RwReal) 0.0;
                                    cptMatrix[i][j].z = (RwReal) 0.0;
                                }
                            }

                            _rpPatchRefineTriVector( (RwV3d *)vertex,
                                                     stride,
                                                     cptMatrix,
                                                     res );
                        }

                        vertex += sizeof(RwV3d);
                    }

                    /* Set up the prelit. */
                    if(meshFlags & rpPATCHMESHPRELIGHTS)
                    {
                        if(preLit != NULL)
                        {
                            l = 0;
                            for( j = 0; j < 4; j++ )
                            {
                                for( i = 0; i < (4 - j); i++ )
                                {
                                    k = cptIdx[l];
                                    cptMatrix[i][j].z = preLit[k].red;
                                    cptMatrix[i][j].y = preLit[k].green;
                                    cptMatrix[i][j].x = preLit[k].blue;
                                    cptMatrix[i][j].w = preLit[k].alpha;
                                    l++;
                                }

                                for( ; i < 4; i++ )
                                {
                                    cptMatrix[i][j].x = (RwReal) 0.0;
                                    cptMatrix[i][j].y = (RwReal) 0.0;
                                    cptMatrix[i][j].z = (RwReal) 0.0;
                                    cptMatrix[i][j].w = (RwReal) 0.0;
                                }
                            }

                            _rpPatchRefineTriColor( (RwRGBA *)vertex,
                                                    stride,
                                                    cptMatrix,
                                                    res );
                        }

                        vertex += sizeof(RwRGBA);
                    }

                    /* Set up the texccords. */
                    for( i = 0;
                         i < numTextureCoords && i < numTextureCoordsPatch;
                         i++ )
                    {
                        const RwTexCoords *texCoords = geom->texCoords[i];

                        cptTexCoords[0] = texCoords[cptIdx[0]];
                        cptTexCoords[1] = texCoords[cptIdx[9]];
                        cptTexCoords[2] = texCoords[cptIdx[3]];

                        _rpPatchRefineTriTexCoords( (RwTexCoords *)vertex,
                                                    stride,
                                                    cptTexCoords,
                                                    res );

                        vertex += sizeof(RwTexCoords);
                    }

                    _rpPatchGenerateTriIndex(idx, base, res);

                    base += numTriVerts;
                    pt += numTriVerts * stride;
                    idx += numTriIndices;
                    instMesh->numVerts += numTriVerts;
                    instMesh->numIndices += numTriIndices;
                    minVert += numTriVerts;
                    firstpatch = 0;
                }
                else
                {
                    /* Quad patch. */

                    /* Set up the control point array. */
                    for( i = 0; i < 4; i++ )
                    {
                        for( j = 0; j < 4; j++ )
                        {
                            k = cptIdx[(j << 2) + i];
                            cptMatrix[i][j].x = vert[k].x;
                            cptMatrix[i][j].y = vert[k].y;
                            cptMatrix[i][j].z = vert[k].z;
                        }
                    }

                    _rpPatchRefineQuadVector( (RwV3d *)vertex,
                                              stride,
                                              cptMatrix,
                                              res );

                    vertex += sizeof(RwV3d);

                    /* Set up normals */
                    if(meshFlags & rpPATCHMESHNORMALS)
                    {
                        if(nrm != NULL)
                        {
                            for( i = 0; i < 4; i++ )
                            {
                                for( j = 0; j < 4; j++ )
                                {
                                    k = cptIdx[(j << 2) + i];
                                    cptMatrix[i][j].x = nrm[k].x;
                                    cptMatrix[i][j].y = nrm[k].y;
                                    cptMatrix[i][j].z = nrm[k].z;
                                }
                            }

                            _rpPatchRefineQuadVector( (RwV3d *)vertex,
                                                      stride,
                                                      cptMatrix,
                                                      res );
                        }

                        vertex += sizeof(RwV3d);
                    }

                    /* Set up the prelit. */
                    if(meshFlags & rpPATCHMESHPRELIGHTS)
                    {
                        if(preLit != NULL)
                        {
                            for( i = 0; i < 4; i++ )
                            {
                                for( j = 0; j < 4; j++ )
                                {
                                    k = cptIdx[(j << 2) + i];
                                    cptMatrix[i][j].z = preLit[k].red;
                                    cptMatrix[i][j].y = preLit[k].green;
                                    cptMatrix[i][j].x = preLit[k].blue;
                                    cptMatrix[i][j].w = preLit[k].alpha;
                                }
                            }

                            _rpPatchRefineQuadColor( (RwRGBA *)vertex,
                                                     stride,
                                                     cptMatrix,
                                                     res );
                        }

                        vertex += sizeof(RwRGBA);
                    }

                    /* Set up the texccords. */
                    for( i = 0;
                         i < numTextureCoords && i < numTextureCoordsPatch;
                         i++ )
                    {
                        const RwTexCoords *texCoords = geom->texCoords[i];

                        cptTexCoords[0] = texCoords[cptIdx[0]];
                        cptTexCoords[1] = texCoords[cptIdx[3]];
                        cptTexCoords[2] = texCoords[cptIdx[12]];
                        cptTexCoords[3] = texCoords[cptIdx[15]];

                        _rpPatchRefineQuadTexCoords( (RwTexCoords *)vertex,
                                                     stride,
                                                     cptTexCoords,
                                                     res );

                        vertex += sizeof(RwTexCoords);
                    }

                    _rpPatchGenerateQuadIndex(idx, base, res);

                    base += numQuadVerts;
                    pt += numQuadVerts * stride;
                    idx += numQuadIndices;
                    instMesh->numVerts += numQuadVerts;
                    instMesh->numIndices += numQuadIndices;
                    minVert += numQuadVerts;
                    firstpatch = 0;
                }

                numCtrlIndices -= rpPATCHNUMCONTROLINDICES;
                cptIdx += rpPATCHNUMCONTROLINDICES;
            }

            instMesh++;
        }
        else
        {
            numMeshResult--;
        }

        mesh++;
        numMesh--;
    }

    instAtomic->numMeshes = numMeshResult;

    RWRETURN(TRUE);
}

/*****************************************************************************
 _rwD3D8PatchAtomicAllInOneNodeNode

 _refineNode

 Inputs :
 Outputs :
 */
RwBool
_rwD3D8PatchAtomicAllInOneNodeNode( RxPipelineNodeInstance *self,
                            const RxPipelineNodeParam *params )
{
    RpNodePatchData *patchData;

    RpAtomic        *atomic;
    RpGeometry      *geom;
    RwUInt32        geomFlags;

    PatchAtomicData *atomicData;
    RpMeshHeader    *meshHeader;
    RwInt32         res;
    RwInt32         numMeshes;

    RwResEntry      *repEntry;
    RwResEntry      **repEntryOwner;

    PatchMesh       *patchMesh;
    RwUInt32        numQuadPatches;
    RwUInt32        numTriPatches;
    RwUInt32        numVerts;

    void            *owner;

    RxD3D8ResEntryHeader    *resEntryHeader;

    RwMatrix            *matrix;
    RwUInt32            lighting;

    RWFUNCTION(RWSTRING("_rwD3D8PatchAtomicAllInOneNodeNode"));

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

    /* We need to re-generate the mesh due to some changes. */
    if (repEntry == NULL)
    {
        RwInt32                 numIndices;
        RwUInt32                size;
        RwUInt32                fvf;
        RwUInt32                stride;
        RpPatchInstanceAtomic   *instAtomic;
        RpPatchInstanceMesh     *instMesh;
        RxD3D8InstanceData      *instancedData;
        LPDIRECT3DVERTEXBUFFER8 vertexBuffer;
        RwUInt32                baseIndex;

        /* Total number of indices and tris. */
        numIndices = (numQuadPatches * PATCHQUADNUMINDEX(res + 1)) +
                     (numTriPatches * PATCHTRINUMINDEX(res + 1));

        /* Add extra indices for connecting patches together. */
        numIndices += (numQuadPatches + numTriPatches - 1) * 2;

        /* Add extra for odd number of tri patches to preserve winding order. */
        if (res & 0x01)
            numIndices += (numTriPatches);

        /* Create temporary atomic info (creates extra space for extra meshes) */
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

        /* Calculate vertex stride and the fvf */
        fvf = D3DFVF_XYZ;

        stride = sizeof(RwV3d);

        if (instAtomic->meshFlags & rpPATCHMESHNORMALS)
        {
            fvf |= D3DFVF_NORMAL;

            stride += sizeof(RwV3d);
        }

        if (instAtomic->meshFlags & rpPATCHMESHPRELIGHTS)
        {
            fvf |= D3DFVF_DIFFUSE;

            stride += sizeof(RwRGBA);
        }

        /* Texture coordinates */
        switch(_rpPatchMeshFlagGetNumTexCoords(instAtomic->meshFlags))
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

        instAtomic->stride = stride;

        /* Create only one vertex buffer stored in the first instance data */
        if (FALSE == _rxD3D8VertexBufferManagerCreate(fvf,
                                                    numVerts * stride,
                                                    (void **)&vertexBuffer,
                                                    &baseIndex))
        {
            RwFree(instAtomic);

            RWRETURN(FALSE);
        }

        /*
         *  Tesselate the patch directly over the vertex buffer
         */
        if (SUCCEEDED(IDirect3DVertexBuffer8_Lock(vertexBuffer,
                         baseIndex * stride,
                         numVerts * stride,
                         (RwUInt8 **)&(instAtomic->vertices), 0)))
        {
            /* Interpolate */
            if (FALSE == _rpD3D8PatchInstanceAtomic(instAtomic, geom, res))
            {
                _rxD3D8VertexBufferManagerDestroy(fvf,
                                                numVerts * stride,
                                                (void *)vertexBuffer, baseIndex);

                RwFree(instAtomic);

                RWRETURN(FALSE);
            }

            IDirect3DVertexBuffer8_Unlock(vertexBuffer);
        }

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

        /* Fill instance data and indices */
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
                if (RwD3D8IndexBufferCreate(instancedData->numIndices, &(instancedData->indexBuffer)))
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

        RwFree(instAtomic);

        geom->lockedSinceLastInst = 0;
    }
    else
    {
        if (geom->lockedSinceLastInst & (rpGEOMETRYLOCKVERTICES | rpGEOMETRYLOCKNORMALS))
        {
            RwInt32                 numIndices;
            RwUInt32                size;
            RpPatchInstanceAtomic   *instAtomic;
            RxD3D8InstanceData      *instancedData;

            /* Set the header info */
            resEntryHeader = (RxD3D8ResEntryHeader *) (repEntry + 1);

            /* Get the first RxD3D8InstanceData pointer */
            instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

            /* Total number of indices and tris. */
            numIndices = (numQuadPatches * PATCHQUADNUMINDEX(res + 1)) +
                         (numTriPatches * PATCHTRINUMINDEX(res + 1));

            /* Add extra indices for connecting patches together. */
            numIndices += (numQuadPatches + numTriPatches - 1) * 2;

            /* Add extra for odd number of tri patches to preserve winding order. */
            if (res & 0x01)
                numIndices += (numTriPatches);

            /* Create temporary atomic info */
            size = sizeof(RpPatchInstanceAtomic) +
                    (numMeshes * sizeof(RpPatchInstanceMesh));

            instAtomic = (RpPatchInstanceAtomic *)RwMalloc(size,
                rwID_PATCHPLUGIN | rwMEMHINTDUR_FUNCTION);

            instAtomic->indices = NULL;
            instAtomic->numMeshes = numMeshes;
            instAtomic->totalIndices = numIndices;
            instAtomic->totalVerts = numVerts;
            instAtomic->meshFlags = ((RpPatchMeshFlag)
                                     _rpPatchMeshGetFlags(patchMesh));
            instAtomic->stride = instancedData->stride;

            /*
             *  Tesselate the patch directly over the vertex buffer
             */
            if (SUCCEEDED(IDirect3DVertexBuffer8_Lock((LPDIRECT3DVERTEXBUFFER8)instancedData->vertexBuffer,
                             (instancedData->baseIndex - instancedData->minVert) * instancedData->stride,
                             numVerts * instancedData->stride,
                             (RwUInt8 **)&(instAtomic->vertices), 0)))
            {
                /* Interpolate */
                if (FALSE == _rpPatchReInstanceAtomic(instAtomic, geom, res))
                {
                    IDirect3DVertexBuffer8_Unlock((LPDIRECT3DVERTEXBUFFER8)instancedData->vertexBuffer);

                    RwFree(instAtomic);

                    RWRETURN(FALSE);
                }

                IDirect3DVertexBuffer8_Unlock((LPDIRECT3DVERTEXBUFFER8)instancedData->vertexBuffer);
            }

            RwFree(instAtomic);

            geom->lockedSinceLastInst = 0;
        }

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
        if ((((const D3DCAPS8 *)RwD3D8GetCaps())->DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0)
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
        patchData->renderCallback(repEntry, atomic, rpATOMIC, geomFlags);
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
