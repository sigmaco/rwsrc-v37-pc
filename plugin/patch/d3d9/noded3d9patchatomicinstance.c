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

#include "noded3d9patchatomicinstance.h"

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
 _rpD3D9PatchInstanceAtomic

 Generate the atomic refinememt.

 Inputs:
 Outputs:
 */
RwBool
_rpD3D9PatchInstanceAtomic( RpPatchInstanceAtomic *instAtomic,
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

    RWFUNCTION(RWSTRING("_rpD3D9PatchInstanceAtomic"));

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
 _rpD3D9PatchAtomicInstance
 _refineNode
 Inputs :
 Outputs :
 */
static RwResEntry *
_rpD3D9PatchAtomicInstance(PatchMesh *patchMesh,
                           RpAtomic *atomic,
                           RpGeometry *geom,
                           RwInt32 res)
{
    RwUInt32                numQuadPatches;
    RwUInt32                numTriPatches;
    RwUInt32                numVerts, numIndices, numMeshes;
    RpMeshHeader            *meshHeader;
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
        instancedData->baseIndex = instMesh->minVert +
                                   (vertexStream.offset/ vertexStream.stride);

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
 _rwD3D9PatchAtomicAllInOneNodeNode

 _refineNode

 Inputs :
 Outputs :
 */
RwBool
_rwD3D9PatchAtomicAllInOneNodeNode( RxPipelineNodeInstance *self,
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

    PatchMesh       *patchMesh;
    RwUInt32        numQuadPatches;
    RwUInt32        numTriPatches;
    RwUInt32        numVerts;

    RwMatrix        *matrix;
    RwUInt32        lighting;

    RWFUNCTION(RWSTRING("_rwD3D9PatchAtomicAllInOneNodeNode"));
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

    /* We need to re-generate the mesh due to some changes. */
    if (repEntry == NULL)
    {
        repEntry = _rpD3D9PatchAtomicInstance(patchMesh, atomic, geom, res);
        if (repEntry == NULL)
        {
            RWRETURN(FALSE);
        }
    }
    else
    {
        if (geom->lockedSinceLastInst & (rpGEOMETRYLOCKVERTICES | rpGEOMETRYLOCKNORMALS))
        {
            RwInt32                 numIndices;
            RwUInt32                size;
            RpPatchInstanceAtomic   *instAtomic;
            RxD3D9ResEntryHeader    *resEntryHeader;
            RxD3D9VertexStream      *vertexStream;

            /* Set the header info */
            resEntryHeader = (RxD3D9ResEntryHeader *) (repEntry + 1);

            /* I assume positions and normals are on the first stream */
            vertexStream = &(resEntryHeader->vertexStream[0]);

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
            instAtomic->meshFlags = ((RpPatchMeshFlag)
                                     _rpPatchMeshGetFlags(patchMesh));
            instAtomic->stride = vertexStream->stride;

            /*
             *  Tesselate the patch directly over the vertex buffer
             */
            if (SUCCEEDED(IDirect3DVertexBuffer9_Lock((LPDIRECT3DVERTEXBUFFER9)vertexStream->vertexBuffer,
                             vertexStream->offset,
                             numVerts * (vertexStream->stride),
                             (RwUInt8 **)&(instAtomic->vertices), 0)))
            {
                /* Interpolate */
                if (FALSE == _rpPatchReInstanceAtomic(instAtomic, geom, res))
                {
                    IDirect3DVertexBuffer9_Unlock((LPDIRECT3DVERTEXBUFFER9)vertexStream->vertexBuffer);

                    RwFree(instAtomic);

                    RWRETURN(FALSE);
                }

                IDirect3DVertexBuffer9_Unlock((LPDIRECT3DVERTEXBUFFER9)vertexStream->vertexBuffer);
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
        if ((((const D3DCAPS9 *)RwD3D9GetCaps())->DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0)
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
