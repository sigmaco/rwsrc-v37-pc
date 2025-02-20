/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/

#include <d3d9.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpteam.h"

#include "teampipes.h"
#include "teamskin.h"
#include "team.h"

#include "../../plugin/skin2/d3d9/skind3d9.h"
#include "../../plugin/skin2/d3d9/skind3d9instance.h"
#include "../../plugin/skin2/d3d9/skind3d9generic.h"
#include "../../plugin/skin2/skin.h"

#include "teamstaticd3d9.h"

#include "skindefs.h"

/* Pre compiled vertex shader */
#include "skinshdr.h"
#include "dualskinshdr.h"
#include "envmapskinshdr.h"
#include "skinshdr_unlit.h"
#include "skinshdr_unlit_nofog.h"

/* Vertex shaders 2.0 */
#include "skinshdr2.h"
#include "dualskinshdr2.h"
#include "envmapskinshdr2.h"
#include "skinshdr_unlit2.h"
#include "skinshdr_unlit_nofog2.h"

#define NUMCLUSTERSOFINTEREST   0
#define NUMOUTPUTS              0

#define NUM_ROWS                3

#define COLORSCALAR 0.003921568627450980392156862745098f    /* 1.0f/ 255.0f */

 /*
 *

  CUSTOM FAST SKIN

 *
 */

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/
typedef struct _rpTeamD3D9MatrixTransposed3Rows _rpTeamD3D9MatrixTransposed3Rows;
struct _rpTeamD3D9MatrixTransposed3Rows
{
    RwReal right_x, up_x, at_x, pos_x;
    RwReal right_y, up_y, at_y, pos_y;
    RwReal right_z, up_z, at_z, pos_z;
};

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

RwUInt32  _rpTeamD3D9MaxBonesHardwareSupported = 0;

_rpTeamD3D9MatrixTransposed _rpTeamD3D9projViewMatrix;

RwBool  _rpTeamD3D9UsingVertexShader = FALSE;
RwBool  _rpTeamD3D9UsingLocalSpace = FALSE; /* TRUE if numNodes > _rpTeamD3D9MaxBonesHardwareSupported */
static RwBool  _rpTeamD3D9UsingTransposedMatrix = FALSE;

static _rpTeamD3D9MatrixTransposed3Rows *_rpTeamD3D9TransposedMatrix = NULL;
static RwUInt32 _rpTeamD3D9MaxTransposedMatrix = 0;

void *_rpTeamD3D9UnlitSkinVertexShader = NULL;
void *_rpTeamD3D9UnlitNoFogSkinVertexShader = NULL;

const D3DCAPS9  *D3DCaps = NULL;

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/

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

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
#define NUMPRIMTYPES    7
static const D3DPRIMITIVETYPE _RwD3D9PrimConvTable[NUMPRIMTYPES] =
{
    (D3DPRIMITIVETYPE)0,    /* rwPRIMTYPENAPRIMTYPE */
    D3DPT_LINELIST,         /* rwPRIMTYPELINELIST */
    D3DPT_LINESTRIP,        /* rwPRIMTYPEPOLYLINE */
    D3DPT_TRIANGLELIST,     /* rwPRIMTYPETRILIST */
    D3DPT_TRIANGLESTRIP,    /* rwPRIMTYPETRISTRIP */
    D3DPT_TRIANGLEFAN,      /* rwPRIMTYPETRIFAN */
    D3DPT_POINTLIST
};

static void *_rpTeamD3D9SkinVertexShader = NULL;
static void *_rpTeamD3D9DualSkinVertexShader = NULL;
static void *_rpTeamD3D9EnvMapSkinVertexShader = NULL;

static D3DMATRIX _rpTeamD3D9ViewMatrix;

static RwRGBAReal   LightColor =
{
    0.0f, 0.0f, 0.0f, 0.0f
};

static RwV4d   LightDirection =
{
    0.0f, 0.0f, 0.0f, 0.0f
};

static RwUInt32 _rpTeamD3D9LastRenderFrame = 0xffffffff;

static TeamPipeIndex    TeamSkinedPipelineIndex = (TeamPipeIndex)TEAMPIPENULL;

static RwBool  TeamD3D9DirectionalLightFound = FALSE;
static RwBool  TeamD3D9AmbientLightFound = FALSE;

/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

/****************************************************************************
 TeamD3D9SkinAtomicCreateVertexBuffer

 Purpose:   Just create the vertex buffer
 On entry:
 On exit :
*/
static RwBool
TeamD3D9SkinAtomicCreateVertexBuffer(const RpGeometry *geometry, RxD3D9ResEntryHeader *resEntryHeader)
{
    D3DVERTEXELEMENT9       declaration[18];
    RpGeometryFlag          flags;
    RwUInt32                numTextureCoords;
    RxD3D9InstanceData      *instancedData;
    RxD3D9VertexStream      *vertexStream;
    RwUInt32                vbSize;
    RwUInt32                declarationIndex, numMeshes, n;

    RWFUNCTION(RWSTRING("TeamD3D9SkinAtomicCreateVertexBuffer"));

    flags = (RpGeometryFlag)RpGeometryGetFlags(geometry);

    numTextureCoords = RpGeometryGetNumTexCoordSets(geometry);
    RWASSERT(numTextureCoords>=1 && numTextureCoords<=2);

    /*
     * Calculate the stride of the vertex
     */
    declarationIndex = 0;
    vertexStream = &(resEntryHeader->vertexStream[0]);
    vertexStream->stride = 0;

    /* Positions */
    declaration[declarationIndex].Stream = 0;
    declaration[declarationIndex].Offset = vertexStream->stride;
    declaration[declarationIndex].Type = D3DDECLTYPE_FLOAT3;
    declaration[declarationIndex].Method = D3DDECLMETHOD_DEFAULT;
    declaration[declarationIndex].Usage = D3DDECLUSAGE_POSITION;
    declaration[declarationIndex].UsageIndex = 0;
    declarationIndex++;
    vertexStream->stride = sizeof(RwV3d);
    vertexStream->geometryFlags = rpGEOMETRYLOCKVERTICES;

    /* Weights */
    declaration[declarationIndex].Stream = 0;
    declaration[declarationIndex].Offset = vertexStream->stride;

    if (D3DCaps->DeclTypes & D3DDTCAPS_USHORT2N)
    {
        declaration[declarationIndex].Type = D3DDECLTYPE_USHORT2N;
        vertexStream->stride += 2 * sizeof(RwUInt16);
    }
    else if (D3DCaps->DeclTypes & D3DDTCAPS_SHORT2N)
    {
        declaration[declarationIndex].Type = D3DDECLTYPE_SHORT2N;
        vertexStream->stride += 2 * sizeof(RwUInt16);
    }
    else if (D3DCaps->DeclTypes & D3DDTCAPS_DEC3N)
    {
        declaration[declarationIndex].Type = D3DDECLTYPE_DEC3N;
        vertexStream->stride += sizeof(RwUInt32);
    }
    else if (D3DCaps->DeclTypes & D3DDTCAPS_UBYTE4N)
    {
        declaration[declarationIndex].Type = D3DDECLTYPE_UBYTE4N;
        vertexStream->stride += 4 * sizeof(RwUInt8);
    }
    else
    {
        declaration[declarationIndex].Type = D3DDECLTYPE_D3DCOLOR;
        vertexStream->stride += sizeof(RwUInt32);
    }

    declaration[declarationIndex].Method = D3DDECLMETHOD_DEFAULT;
    declaration[declarationIndex].Usage = D3DDECLUSAGE_BLENDWEIGHT;
    declaration[declarationIndex].UsageIndex = 0;
    declarationIndex++;
    /*vertexStream->geometryFlags |= rpGEOMETRYLOCKNORMALS;*/

    /* Indices */
    declaration[declarationIndex].Stream = 0;
    declaration[declarationIndex].Offset = vertexStream->stride;
    declaration[declarationIndex].Type = D3DDECLTYPE_SHORT2;
    declaration[declarationIndex].Method = D3DDECLMETHOD_DEFAULT;
    declaration[declarationIndex].Usage = D3DDECLUSAGE_BLENDINDICES;
    declaration[declarationIndex].UsageIndex = 0;
    vertexStream->stride += sizeof(RwUInt32);
    declarationIndex++;
    /*vertexStream->geometryFlags |= rpGEOMETRYLOCKNORMALS;*/

    /* Normals */
    RWASSERT(flags & rxGEOMETRY_NORMALS);
    declaration[declarationIndex].Stream = 0;
    declaration[declarationIndex].Offset = vertexStream->stride;

    if (D3DCaps->DeclTypes & D3DDTCAPS_DEC3N)
    {
        declaration[declarationIndex].Type = D3DDECLTYPE_DEC3N;
        vertexStream->stride += sizeof(RwUInt32);
    }
    else
    {
        declaration[declarationIndex].Type = D3DDECLTYPE_FLOAT3;
        vertexStream->stride += sizeof(RwV3d);
    }

    declaration[declarationIndex].Method = D3DDECLMETHOD_DEFAULT;
    declaration[declarationIndex].Usage = D3DDECLUSAGE_NORMAL;
    declaration[declarationIndex].UsageIndex = 0;
    declarationIndex++;
    vertexStream->geometryFlags |= rpGEOMETRYLOCKNORMALS;

    /* Texture coordinates */
    if ( vertexStream->stride > (32 - numTextureCoords * sizeof(RwV2d)) )
    {
        for (n = 0; n < numTextureCoords; n++)
        {
            declaration[declarationIndex].Stream = 0;
            declaration[declarationIndex].Offset = vertexStream->stride;

            declaration[declarationIndex].Type =
                _rpD3D9FindFormatV2d((const RwV2d *)(((const RwUInt8 *)(&geometry->texCoords[n][0]))),
                                     geometry->numVertices);

            declaration[declarationIndex].Method = D3DDECLMETHOD_DEFAULT;
            declaration[declarationIndex].Usage = D3DDECLUSAGE_TEXCOORD;
            declaration[declarationIndex].UsageIndex = n;

            vertexStream->stride +=
                _rpD3D9VertexDeclarationGetSize(declaration[declarationIndex].Type);

            declarationIndex++;

            vertexStream->geometryFlags |= (rpGEOMETRYLOCKTEXCOORDS1 << n);
        }
    }
    else
    {
        for (n = 0; n < numTextureCoords; n++)
        {
            declaration[declarationIndex].Stream = 0;
            declaration[declarationIndex].Offset = vertexStream->stride;
            declaration[declarationIndex].Type = D3DDECLTYPE_FLOAT2;
            declaration[declarationIndex].Method = D3DDECLMETHOD_DEFAULT;
            declaration[declarationIndex].Usage = D3DDECLUSAGE_TEXCOORD;
            declaration[declarationIndex].UsageIndex = n;
            declarationIndex++;
            vertexStream->stride += sizeof(RwV2d);
            vertexStream->geometryFlags |= (rpGEOMETRYLOCKTEXCOORDS1 << n);
        }
    }

    declaration[declarationIndex].Stream = 0xFF;
    declaration[declarationIndex].Offset = 0;
    declaration[declarationIndex].Type = D3DDECLTYPE_UNUSED;
    declaration[declarationIndex].Method = 0;
    declaration[declarationIndex].Usage = 0;
    declaration[declarationIndex].UsageIndex = 0;

    RwD3D9CreateVertexDeclaration(declaration,
                                  &(resEntryHeader->vertexDeclaration));

    /*
     * Create the vertex buffer
     */
    vbSize = (vertexStream->stride) * (geometry->numVertices);

    vertexStream->managed = TRUE;

    if (FALSE == RwD3D9CreateVertexBuffer(vertexStream->stride, vbSize,
                                          &(vertexStream->vertexBuffer),
                                          &(vertexStream->offset)))
    {
        RWRETURN(FALSE);
    }

    /* Fix base index */
    resEntryHeader->useOffsets = FALSE;

    numMeshes = resEntryHeader->numMeshes;
    instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

    do
    {
        instancedData->baseIndex = instancedData->minVert +
                                   (vertexStream->offset / vertexStream->stride);

        instancedData++;
    }
    while (--numMeshes);

    /* Put vertex shaders */
    numMeshes = resEntryHeader->numMeshes;
    instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

    do
    {
        if (RpMatFXMaterialGetEffects(instancedData->material) == rpMATFXEFFECTENVMAP)
        {
            instancedData->vertexShader = _rpTeamD3D9EnvMapSkinVertexShader;
        }
        else if (numTextureCoords == 1)
        {
            instancedData->vertexShader = _rpTeamD3D9SkinVertexShader;
        }
        else if (numTextureCoords == 2)
        {
            instancedData->vertexShader = _rpTeamD3D9DualSkinVertexShader;
        }
        else
        {
            instancedData->vertexShader = NULL;
        }

        instancedData++;
    }
    while (--numMeshes);

    RWRETURN(TRUE);
}

/****************************************************************************
 TeamD3D9SkinAtomicInstance

 Purpose:   Fill the vertex buffer
 On entry:
 On exit :
*/
static RwBool
TeamD3D9SkinAtomicInstance(const RpAtomic *atomic,
                           RxD3D9ResEntryHeader *resEntryHeader,
                           RwUInt32 numNodes)
{
    D3DVERTEXELEMENT9   declaration[18];
    const RpGeometry    *geometry;
    RxD3D9VertexStream  *vertexStream;
    RwUInt32            stride, n;
    RwUInt8             *vertexData;

    RWFUNCTION(RWSTRING("TeamD3D9SkinAtomicInstance"));

    geometry = (const RpGeometry *)RpAtomicGetGeometry(atomic);

    resEntryHeader->totalNumVertex = geometry->numVertices;

    /* Create vertex buffer */
    TeamD3D9SkinAtomicCreateVertexBuffer(geometry, resEntryHeader);

    IDirect3DVertexDeclaration9_GetDeclaration((LPDIRECT3DVERTEXDECLARATION9)resEntryHeader->vertexDeclaration,
                                                declaration,
                                                &n);

    RWASSERT(n < 18);

    vertexStream = &(resEntryHeader->vertexStream[0]);

    stride = vertexStream->stride;

    /*
     * Fill static information of the vertex buffer
     */
    if (SUCCEEDED(IDirect3DVertexBuffer9_Lock((LPDIRECT3DVERTEXBUFFER9)vertexStream->vertexBuffer,
                                              vertexStream->offset,
                                              (geometry->numVertices) * stride,
                                              &vertexData,
                                              D3DLOCK_NOSYSLOCK)))
    {
        const RpSkin    *skin;
        RwUInt32        numTextureCoords;
        RwUInt32        declarationIndex;
        const RwV3d     *pos;
        const RwV3d     *normal;
        RwMatrixWeights *weights;

        /* Get number of texture coordinates */
        numTextureCoords = RpGeometryGetNumTexCoordSets(geometry);
        RWASSERT(numTextureCoords >= 1 && numTextureCoords <= 2);

        /* Positions */
        pos = geometry->morphTarget[0].verts;

        declarationIndex = 0;
        while (declaration[declarationIndex].Usage != D3DDECLUSAGE_POSITION ||
            declaration[declarationIndex].UsageIndex != 0)
        {
            declarationIndex++;
        }

        _rpD3D9VertexDeclarationInstV3d(declaration[declarationIndex].Type,
                                        vertexData +
                                        declaration[declarationIndex].Offset,
                                        pos,
                                        geometry->numVertices,
                                        stride);

        /* Get skin data */
        skin = *RPSKINGEOMETRYGETCONSTDATA(geometry);
        RWASSERT(skin != NULL);

        /*
         * Weights
         */
        weights = skin->vertexMaps.matrixWeights;

        /* Fix weights */
        n = geometry->numVertices;
        while (n--)
        {
            if (weights->w0 >= 1.0f)
            {
                weights->w0 = 1.0f;
                weights->w1 = 0.0f;
            }
            else
            {
                weights->w1 = 1.f - weights->w0;
            }

            weights->w2 = 0.0f;
            weights->w3 = 0.0f;

            weights++;
        }

        declarationIndex = 0;
        while (declaration[declarationIndex].Usage != D3DDECLUSAGE_BLENDWEIGHT ||
            declaration[declarationIndex].UsageIndex != 0)
        {
            declarationIndex++;
        }

        weights = skin->vertexMaps.matrixWeights;
        _rpD3D9VertexDeclarationInstWeights(declaration[declarationIndex].Type,
                                            vertexData +
                                            declaration[declarationIndex].Offset,
                                            (const RwV4d *)weights,
                                            geometry->numVertices,
                                            stride);

        /* Indices */
        declarationIndex = 0;
        while (declaration[declarationIndex].Usage != D3DDECLUSAGE_BLENDINDICES ||
            declaration[declarationIndex].UsageIndex != 0)
        {
            declarationIndex++;
        }

        if (numNodes <= _rpTeamD3D9MaxBonesHardwareSupported)
        {
            const RwUInt32    *indices;
            RwUInt8 *vertexBuffer;

            indices = skin->vertexMaps.matrixIndices;

            vertexBuffer = vertexData + declaration[declarationIndex].Offset;
            n = geometry->numVertices;
            while (n--)
            {
                const RwUInt32 index = *indices;

                ((RwUInt16 *)vertexBuffer)[0] = (RwUInt16)((index & 0xFF) * NUM_ROWS);
                ((RwUInt16 *)vertexBuffer)[1] = (RwUInt16)(((index >> 8) & 0xFF) * NUM_ROWS);
                vertexBuffer += stride;
                indices++;
            }
        }
        else
        {
            static RwChar usedToIndex[256];

            const RwUInt32  numUsedBones = skin->boneData.numUsedBones;
            const RwUInt8   *usedBones = skin->boneData.usedBoneList;
            const RwUInt32  *indices;
            RwUInt8 *vertexBuffer;

            RWASSERT(numUsedBones <= _rpTeamD3D9MaxBonesHardwareSupported);

            usedToIndex[0] = 0; /* Used for 1 weight vertex */
            for (n = 1; n < numNodes; n++)
            {
                usedToIndex[n] = (RwChar)0xff;
            }

            for (n = 0; n < numUsedBones; n++)
            {
                usedToIndex[usedBones[n]] = n;
            }

            indices = skin->vertexMaps.matrixIndices;

            vertexBuffer = vertexData + declaration[declarationIndex].Offset;
            n = geometry->numVertices;
            while (n--)
            {
                const RwUInt32 index = *indices;
                const RwUInt32 index0 = usedToIndex[index & 0xFF];
                const RwUInt32 index1 = usedToIndex[(index >> 8) & 0xFF];

                RWASSERT(index0 < numUsedBones);
                RWASSERT(index1 < numUsedBones);

                ((RwUInt16 *)vertexBuffer)[0] = (RwUInt16)(index0 * NUM_ROWS);
                ((RwUInt16 *)vertexBuffer)[1] = (RwUInt16)(index1 * NUM_ROWS);
                vertexBuffer += stride;
                indices++;
            }
        }

        /* Normals */
        normal = (const RwV3d *)((const RwUInt8 *)(geometry->morphTarget[0].normals));
        RWASSERT(normal != NULL);

        declarationIndex = 0;
        while (declaration[declarationIndex].Usage != D3DDECLUSAGE_NORMAL ||
            declaration[declarationIndex].UsageIndex != 0)
        {
            declarationIndex++;
        }

        _rpD3D9VertexDeclarationInstV3d(declaration[declarationIndex].Type,
                                        vertexData +
                                        declaration[declarationIndex].Offset,
                                        normal,
                                        geometry->numVertices,
                                        stride);

        /* Texture coordinates */
        if (numTextureCoords)
        {
            RwUInt32    n;

            for (n = 0; n < numTextureCoords; n++)
            {
                const RwTexCoords *texCoord = (const RwTexCoords *)((const RwUInt8 *)(geometry->texCoords[n]));

                declarationIndex = 0;
                while (declaration[declarationIndex].Usage != D3DDECLUSAGE_TEXCOORD ||
                    declaration[declarationIndex].UsageIndex != n)
                {
                    declarationIndex++;
                }

                _rpD3D9VertexDeclarationInstV2d(declaration[declarationIndex].Type,
                                                vertexData +
                                                declaration[declarationIndex].Offset,
                                                (const RwV2d *)texCoord,
                                                geometry->numVertices,
                                                stride);
            }
        }

        IDirect3DVertexBuffer9_Unlock((LPDIRECT3DVERTEXBUFFER9)vertexStream->vertexBuffer);
    }

    RWRETURN(TRUE);
}

/****************************************************************************
 TeamD3D9SkinInstance

 Purpose:   To instance.

 On entry:

 On exit :
*/
static RwResEntry *
TeamD3D9SkinInstance(const RpAtomic *atomic,
                     void *owner,
                     RwResEntry **resEntryPointer,
                     RpMeshHeader *meshHeader,
                     RwUInt32 numNodes)
{
    RxD3D9ResEntryHeader    *resEntryHeader;
    RxD3D9InstanceData      *instancedData;
    RwResEntry              *resEntry;
    RpMesh                  *mesh;
    RwInt32                 numMeshes;
    RwUInt32                size, n;
    RwUInt32                startIndex;
    RxVertexIndex           *indexBuffer;

    RWFUNCTION(RWSTRING("TeamD3D9SkinInstance"));

    /*
     * Calculate the amount of memory to allocate
     */

    /* RxD3D9ResEntryHeader, stores serialNumber & numMeshes */
    size = sizeof(RxD3D9ResEntryHeader);

    /* RxD3D9InstanceData structures, one for each mesh */
    size += sizeof(RxD3D9InstanceData) * meshHeader->numMeshes;

    /*
     * Allocate the resource entry
     */
    resEntry = RwResourcesAllocateResEntry(owner,
                                           resEntryPointer,
                                           size,
                                           _rwD3D9ResourceEntryInstanceDataDestroy);
    RWASSERT(NULL != resEntry);

    /* Blank the RxD3D9ResEntryHeader & RxD3D9InstanceData's to '0' */
    memset((resEntry + 1), 0, size);

    /*
     * Initialize the RxD3D9ResEntryHeader
     */
    resEntryHeader = (RxD3D9ResEntryHeader *)(resEntry + 1);

    /* Set the serial number */
    resEntryHeader->serialNumber = meshHeader->serialNum;

    /* Set the number of meshes */
    resEntryHeader->numMeshes = meshHeader->numMeshes;

    /*
     * Create the index buffer if needed
     */
    indexBuffer = NULL;

    resEntryHeader->indexBuffer = NULL;
    resEntryHeader->totalNumIndex = 0;

    if ((meshHeader->flags & rpMESHHEADERUNINDEXED) == 0)
    {
        /* Calculate total num indices */
        mesh = (RpMesh *)(meshHeader + 1);
        numMeshes = meshHeader->numMeshes;
        while (numMeshes--)
        {
            resEntryHeader->totalNumIndex += mesh->numIndices;

            mesh++;
        }

        /* Initialize the index buffers pointers */
        if (resEntryHeader->totalNumIndex)
        {
            if (RwD3D9IndexBufferCreate(resEntryHeader->totalNumIndex, &(resEntryHeader->indexBuffer)))
            {
                IDirect3DIndexBuffer9_Lock((LPDIRECT3DINDEXBUFFER9)resEntryHeader->indexBuffer,
                                           0, 0, (RwUInt8 **)&indexBuffer, 0);
            }
        }
    }

    /* Primitive type */
    resEntryHeader->primType = _RwD3D9PrimConvTable[RpMeshHeaderGetPrimType(meshHeader)];

    /* Initialize the vertex buffers pointers */
    for (n = 0; n < RWD3D9_MAX_VERTEX_STREAMS; n++)
    {
        resEntryHeader->vertexStream[n].vertexBuffer = NULL;
        resEntryHeader->vertexStream[n].offset = 0;
        resEntryHeader->vertexStream[n].stride = 0;
        resEntryHeader->vertexStream[n].managed = FALSE;
        resEntryHeader->vertexStream[n].geometryFlags = FALSE;
    }

    /* Vertex declaration */
    resEntryHeader->vertexDeclaration = NULL;

    /* Get the first RxD3D9InstanceData pointer */
    instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

    startIndex = 0;

    mesh = (RpMesh *)(meshHeader + 1);
    numMeshes = meshHeader->numMeshes;
    while (numMeshes--)
    {
        const RwUInt32 numIndices = mesh->numIndices;

        /*
         * Number of vertices and Min vertex index,
         * (needed for instancing & reinstancing)
         */
        _rwD3D9MeshGetNumVerticesMinIndex(mesh->indices, numIndices,
                                &instancedData->numVertices,
                                &instancedData->minVert);

        /* The number of primitives */
        switch (resEntryHeader->primType)
        {
            case D3DPT_LINELIST:
                {
                    RWASSERT(numIndices > 1);
                    instancedData->numPrimitives = numIndices / 2;
                    break;
                }

            case D3DPT_LINESTRIP:
                {
                    RWASSERT(numIndices > 1);
                    instancedData->numPrimitives = numIndices - 1;
                    break;
                }

            case D3DPT_TRIANGLELIST:
                {
                    RWASSERT(numIndices > 2);
                    instancedData->numPrimitives = numIndices / 3;
                    break;
                }

            case D3DPT_TRIANGLESTRIP:
                {
                    RWASSERT(numIndices > 2);
                    instancedData->numPrimitives = numIndices - 2;
                    break;
                }

            case D3DPT_TRIANGLEFAN:
                {
                    RWASSERT(numIndices > 2);
                    instancedData->numPrimitives = numIndices - 2;
                    break;
                }

            default:
                RWASSERT(FALSE && "Unknown primitive tipe.");
                instancedData->numPrimitives = 0;
                break;
        }

        /* Material */
        instancedData->material = mesh->material;

        /* Vertex shader */
        instancedData->vertexShader = NULL;

        /* Initialize vertex alpha to FALSE */
        instancedData->vertexAlpha = FALSE;

        /*
         * Set the index buffer
         */

        /* Initialize the index buffers pointers */
        if (indexBuffer != NULL)
        {
            instancedData->numIndex = numIndices;
            instancedData->startIndex = startIndex;

            if(instancedData->minVert)
            {
                RxVertexIndex   *indexSrc;
                RxVertexIndex   *indexDst;
                RwUInt32        n;

                indexSrc = mesh->indices;
                indexDst = indexBuffer;

                n = numIndices;
                while (n--)
                {
                    *indexDst = (RxVertexIndex)((*indexSrc) - (RxVertexIndex)instancedData->minVert);

                    indexSrc++;
                    indexDst++;
                }
            }
            else
            {
                memcpy(indexBuffer, mesh->indices, sizeof(RxVertexIndex) * numIndices);
            }

            if (resEntryHeader->primType == D3DPT_TRIANGLELIST)
            {
                _rwD3D9SortTriListIndices(indexBuffer, numIndices);
            }

            indexBuffer += numIndices;
            startIndex += numIndices;
        }
        else
        {
            instancedData->numIndex = 0;
            instancedData->startIndex = 0;
        }

        instancedData++;
        mesh++;
    }

    if (indexBuffer != NULL)
    {
        IDirect3DIndexBuffer9_Unlock((LPDIRECT3DINDEXBUFFER9)resEntryHeader->indexBuffer);
    }

    /*
     * Call the instance callback
     */
    TeamD3D9SkinAtomicInstance(atomic, resEntryHeader, numNodes);

    RWRETURN(resEntry);
}

/****************************************************************************
 TeamD3D9MatrixMultiplyTranspose3Rows

 On entry   : Dest matrix pointer, two source matrix pointers
 On exit    : Matrix pointer contains transposed result
 */
static void
TeamD3D9MatrixMultiplyTranspose3Rows(_rpTeamD3D9MatrixTransposed3Rows *dstMat,
               const RwMatrix *matA, const RwMatrix *matB)
{
    RWFUNCTION(RWSTRING("TeamD3D9MatrixMultiplyTranspose3Rows"));
    RWASSERT(dstMat);
    RWASSERT(matA);
    RWASSERT(matB);

    /* Multiply out right */
    dstMat->right_x =
        (matA->right.x * matB->right.x) +
        (matA->right.y * matB->up.x) +
        (matA->right.z * matB->at.x);
    dstMat->right_y =
        (matA->right.x * matB->right.y) +
        (matA->right.y * matB->up.y) +
        (matA->right.z * matB->at.y);
    dstMat->right_z =
        (matA->right.x * matB->right.z) +
        (matA->right.y * matB->up.z) +
        (matA->right.z * matB->at.z);

    /* Then up */
    dstMat->up_x =
        (matA->up.x * matB->right.x) +
        (matA->up.y * matB->up.x) +
        (matA->up.z * matB->at.x);
    dstMat->up_y =
        (matA->up.x * matB->right.y) +
        (matA->up.y * matB->up.y) +
        (matA->up.z * matB->at.y);
    dstMat->up_z =
        (matA->up.x * matB->right.z) +
        (matA->up.y * matB->up.z) +
        (matA->up.z * matB->at.z);

    /* Then at */
    dstMat->at_x =
        (matA->at.x * matB->right.x) +
        (matA->at.y * matB->up.x) +
        (matA->at.z * matB->at.x);
    dstMat->at_y =
        (matA->at.x * matB->right.y) +
        (matA->at.y * matB->up.y) +
        (matA->at.z * matB->at.y);
    dstMat->at_z =
        (matA->at.x * matB->right.z) +
        (matA->at.y * matB->up.z) +
        (matA->at.z * matB->at.z);

    /* Then pos - this is different because there is an extra add
     * (implicit 1 (one) in bottom right of matrix)
     */
    dstMat->pos_x =
        (matA->pos.x * matB->right.x) +
        (matA->pos.y * matB->up.x) +
        (matA->pos.z * matB->at.x) +
        ( /* (1*) */ matB->pos.x);
    dstMat->pos_y =
        (matA->pos.x * matB->right.y) +
        (matA->pos.y * matB->up.y) +
        (matA->pos.z * matB->at.y) +
        ( /* (1*) */ matB->pos.y);
    dstMat->pos_z =
        (matA->pos.x * matB->right.z) +
        (matA->pos.y * matB->up.z) +
        (matA->pos.z * matB->at.z) +
        ( /* (1*) */ matB->pos.z);

    /* And that's all folks */
    RWRETURNVOID();
}

/****************************************************************************
 TeamD3D9MatrixMultiplyTranspose

 On entry   : Dest matrix pointer, two source matrix pointers
 On exit    : Matrix pointer contains transposed result
 */
static void
TeamD3D9MatrixMultiplyTranspose(_rpTeamD3D9MatrixTransposed *dstMat,
               const D3DMATRIX *matA, const D3DMATRIX *matB)
{
    RWFUNCTION(RWSTRING("TeamD3D9MatrixMultiplyTranspose"));
    RWASSERT(dstMat);
    RWASSERT(matA);
    RWASSERT(matB);

    /* Multiply out right */
    dstMat->right_x =
        (matA->m[0][0] * matB->m[0][0]) +
        (matA->m[0][1] * matB->m[1][0]) +
        (matA->m[0][2] * matB->m[2][0]) +
        (matA->m[0][3] * matB->m[3][0]);
    dstMat->right_y =
        (matA->m[0][0] * matB->m[0][1]) +
        (matA->m[0][1] * matB->m[1][1]) +
        (matA->m[0][2] * matB->m[2][1]) +
        (matA->m[0][3] * matB->m[3][1]);
    dstMat->right_z =
        (matA->m[0][0] * matB->m[0][2]) +
        (matA->m[0][1] * matB->m[1][2]) +
        (matA->m[0][2] * matB->m[2][2]) +
        (matA->m[0][3] * matB->m[3][2]);
    dstMat->right_w =
        (matA->m[0][0] * matB->m[0][3]) +
        (matA->m[0][1] * matB->m[1][3]) +
        (matA->m[0][2] * matB->m[2][3]) +
        (matA->m[0][3] * matB->m[3][3]);

    /* Then up */
    dstMat->up_x =
        (matA->m[1][0] * matB->m[0][0]) +
        (matA->m[1][1] * matB->m[1][0]) +
        (matA->m[1][2] * matB->m[2][0]) +
        (matA->m[1][3] * matB->m[3][0]);
    dstMat->up_y =
        (matA->m[1][0] * matB->m[0][1]) +
        (matA->m[1][1] * matB->m[1][1]) +
        (matA->m[1][2] * matB->m[2][1]) +
        (matA->m[1][3] * matB->m[3][1]);
    dstMat->up_z =
        (matA->m[1][0] * matB->m[0][2]) +
        (matA->m[1][1] * matB->m[1][2]) +
        (matA->m[1][2] * matB->m[2][2]) +
        (matA->m[1][3] * matB->m[3][2]);
    dstMat->up_w =
        (matA->m[1][0] * matB->m[0][3]) +
        (matA->m[1][1] * matB->m[1][3]) +
        (matA->m[1][2] * matB->m[2][3]) +
        (matA->m[1][3] * matB->m[3][3]);

    /* Then at */
    dstMat->at_x =
        (matA->m[2][0] * matB->m[0][0]) +
        (matA->m[2][1] * matB->m[1][0]) +
        (matA->m[2][2] * matB->m[2][0]) +
        (matA->m[2][3] * matB->m[3][0]);
    dstMat->at_y =
        (matA->m[2][0] * matB->m[0][1]) +
        (matA->m[2][1] * matB->m[1][1]) +
        (matA->m[2][2] * matB->m[2][1]) +
        (matA->m[2][3] * matB->m[3][1]);
    dstMat->at_z =
        (matA->m[2][0] * matB->m[0][2]) +
        (matA->m[2][1] * matB->m[1][2]) +
        (matA->m[2][2] * matB->m[2][2]) +
        (matA->m[2][3] * matB->m[3][2]);
    dstMat->at_w =
        (matA->m[2][0] * matB->m[0][3]) +
        (matA->m[2][1] * matB->m[1][3]) +
        (matA->m[2][2] * matB->m[2][3]) +
        (matA->m[2][3] * matB->m[3][3]);

    /* Then pos */
    dstMat->pos_x =
        (matA->m[3][0] * matB->m[0][0]) +
        (matA->m[3][1] * matB->m[1][0]) +
        (matA->m[3][2] * matB->m[2][0]) +
        (matA->m[3][3] * matB->m[3][0]);
    dstMat->pos_y =
        (matA->m[3][0] * matB->m[0][1]) +
        (matA->m[3][1] * matB->m[1][1]) +
        (matA->m[3][2] * matB->m[2][1]) +
        (matA->m[3][3] * matB->m[3][1]);
    dstMat->pos_z =
        (matA->m[3][0] * matB->m[0][2]) +
        (matA->m[3][1] * matB->m[1][2]) +
        (matA->m[3][2] * matB->m[2][2]) +
        (matA->m[3][3] * matB->m[3][2]);
    dstMat->pos_w =
        (matA->m[3][0] * matB->m[0][3]) +
        (matA->m[3][1] * matB->m[1][3]) +
        (matA->m[3][2] * matB->m[2][3]) +
        (matA->m[3][3] * matB->m[3][3]);

    /* And that's all folks */
    RWRETURNVOID();
}

/****************************************************************************
 _rwTeamD3D9SkinMatrixMultiplyTranspose

 On entry   : Dest matrix pointer, two source matrix pointers
 On exit    : Matrix pointer contains transposed result
 */
void
_rwTeamD3D9SkinMatrixMultiplyTranspose(_rpTeamD3D9MatrixTransposed *dstMat,
               const RwMatrix *matA, const _rpTeamD3D9MatrixTransposed *matB)
{
    RWFUNCTION(RWSTRING("_rwTeamD3D9SkinMatrixMultiplyTranspose"));
    RWASSERT(dstMat);
    RWASSERT(matA);
    RWASSERT(matB);

    /* Multiply out right */
    dstMat->right_x =
        (matA->right.x * matB->right_x) +
        (matA->right.y * matB->up_x) +
        (matA->right.z * matB->at_x);
    dstMat->right_y =
        (matA->right.x * matB->right_y) +
        (matA->right.y * matB->up_y) +
        (matA->right.z * matB->at_y);
    dstMat->right_z =
        (matA->right.x * matB->right_z) +
        (matA->right.y * matB->up_z) +
        (matA->right.z * matB->at_z);
    dstMat->right_w =
        (matA->right.x * matB->right_w) +
        (matA->right.y * matB->up_w) +
        (matA->right.z * matB->at_w);

    /* Then up */
    dstMat->up_x =
        (matA->up.x * matB->right_x) +
        (matA->up.y * matB->up_x) +
        (matA->up.z * matB->at_x);
    dstMat->up_y =
        (matA->up.x * matB->right_y) +
        (matA->up.y * matB->up_y) +
        (matA->up.z * matB->at_y);
    dstMat->up_z =
        (matA->up.x * matB->right_z) +
        (matA->up.y * matB->up_z) +
        (matA->up.z * matB->at_z);
    dstMat->up_w =
        (matA->up.x * matB->right_w) +
        (matA->up.y * matB->up_w) +
        (matA->up.z * matB->at_w);

    /* Then at */
    dstMat->at_x =
        (matA->at.x * matB->right_x) +
        (matA->at.y * matB->up_x) +
        (matA->at.z * matB->at_x);
    dstMat->at_y =
        (matA->at.x * matB->right_y) +
        (matA->at.y * matB->up_y) +
        (matA->at.z * matB->at_y);
    dstMat->at_z =
        (matA->at.x * matB->right_z) +
        (matA->at.y * matB->up_z) +
        (matA->at.z * matB->at_z);
    dstMat->at_w =
        (matA->at.x * matB->right_w) +
        (matA->at.y * matB->up_w) +
        (matA->at.z * matB->at_w);

    /* Then pos - this is different because there is an extra add
     * (implicit 1 (one) in bottom right of matrix)
     */
    dstMat->pos_x =
        (matA->pos.x * matB->right_x) +
        (matA->pos.y * matB->up_x) +
        (matA->pos.z * matB->at_x) +
        ( /* (1*) */ matB->pos_x);
    dstMat->pos_y =
        (matA->pos.x * matB->right_y) +
        (matA->pos.y * matB->up_y) +
        (matA->pos.z * matB->at_y) +
        ( /* (1*) */ matB->pos_y);
    dstMat->pos_z =
        (matA->pos.x * matB->right_z) +
        (matA->pos.y * matB->up_z) +
        (matA->pos.z * matB->at_z) +
        ( /* (1*) */ matB->pos_z);
    dstMat->pos_w =
        (matA->pos.x * matB->right_w) +
        (matA->pos.y * matB->up_w) +
        (matA->pos.z * matB->at_w) +
        ( /* (1*) */ matB->pos_w);

    /* And that's all folks */
    RWRETURNVOID();
}

/****************************************************************************
 TeamD3D9EnvMapMatrixMultiplyTranspose

 On entry   : Dest matrix pointer, two source matrix pointers
 On exit    : Matrix pointer contains transposed result
 */
static void
TeamD3D9EnvMapMatrixMultiplyTranspose(_rpTeamD3D9MatrixTransposed *dstMat,
               const D3DMATRIX *matA, const D3DMATRIX *matB)
{
    RWFUNCTION(RWSTRING("TeamD3D9EnvMapMatrixMultiplyTranspose"));
    RWASSERT(dstMat);
    RWASSERT(matA);
    RWASSERT(matB);

    /* Multiply out right */
    dstMat->right_x =
        (matA->m[0][0] * matB->m[0][0]) +
        (matA->m[0][1] * matB->m[1][0]) +
        (matA->m[0][2] * matB->m[2][0]) +
        (matA->m[0][3] * matB->m[3][0]);
    dstMat->right_y =
        (matA->m[0][0] * matB->m[0][1]) +
        (matA->m[0][1] * matB->m[1][1]) +
        (matA->m[0][2] * matB->m[2][1]) +
        (matA->m[0][3] * matB->m[3][1]);

    /* Then up */
    dstMat->up_x =
        (matA->m[1][0] * matB->m[0][0]) +
        (matA->m[1][1] * matB->m[1][0]) +
        (matA->m[1][2] * matB->m[2][0]) +
        (matA->m[1][3] * matB->m[3][0]);
    dstMat->up_y =
        (matA->m[1][0] * matB->m[0][1]) +
        (matA->m[1][1] * matB->m[1][1]) +
        (matA->m[1][2] * matB->m[2][1]) +
        (matA->m[1][3] * matB->m[3][1]);

    /* Then at */
    dstMat->at_x =
        (matA->m[2][0] * matB->m[0][0]) +
        (matA->m[2][1] * matB->m[1][0]) +
        (matA->m[2][2] * matB->m[2][0]) +
        (matA->m[2][3] * matB->m[3][0]);
    dstMat->at_y =
        (matA->m[2][0] * matB->m[0][1]) +
        (matA->m[2][1] * matB->m[1][1]) +
        (matA->m[2][2] * matB->m[2][1]) +
        (matA->m[2][3] * matB->m[3][1]);

    /* Then pos */
    dstMat->pos_x = matB->m[3][0];
    dstMat->pos_y = matB->m[3][1];

    /* And that's all folks */
    RWRETURNVOID();
}

/****************************************************************************
 _rpTeamD3D9SetVertexShaderMaterialColor
 */
void
_rpTeamD3D9SetVertexShaderMaterialColor(const RpMaterial *mat)
{
    const RwRGBA *color = &mat->color;
    const RwReal ambScale  = mat->surfaceProps.ambient;
    const RwReal diffScale = mat->surfaceProps.diffuse;

    RwRGBAReal matColor;
    RwRGBAReal modAmbientColor;
    RwRGBAReal modLightColor;

    RWFUNCTION(RWSTRING("_rpTeamD3D9SetVertexShaderMaterialColor"));

    if (*((const RwUInt32 *)color) != 0xffffffff)
    {
        matColor.red   = color->red   * COLORSCALAR;
        matColor.green = color->green * COLORSCALAR;
        matColor.blue  = color->blue  * COLORSCALAR;
        matColor.alpha = color->alpha * COLORSCALAR;

        modAmbientColor.red   = matColor.red   * AmbientSaturated.red   * ambScale;
        modAmbientColor.green = matColor.green * AmbientSaturated.green * ambScale;
        modAmbientColor.blue  = matColor.blue  * AmbientSaturated.blue  * ambScale;
        modAmbientColor.alpha = 1.f;

        modLightColor.red   = matColor.red   * LightColor.red   * diffScale;
        modLightColor.green = matColor.green * LightColor.green * diffScale;
        modLightColor.blue  = matColor.blue  * LightColor.blue  * diffScale;
        modLightColor.alpha = 1.f;
    }
    else
    {
        matColor.red   = 1.f;
        matColor.green = 1.f;
        matColor.blue  = 1.f;
        matColor.alpha = 1.f;

        modAmbientColor.red   = AmbientSaturated.red   * ambScale;
        modAmbientColor.green = AmbientSaturated.green * ambScale;
        modAmbientColor.blue  = AmbientSaturated.blue  * ambScale;
        modAmbientColor.alpha = 1.f;

        modLightColor.red   = LightColor.red   * diffScale;
        modLightColor.green = LightColor.green * diffScale;
        modLightColor.blue  = LightColor.blue  * diffScale;
        modLightColor.alpha = 1.f;
    }

    /* Set material alpha */
    modAmbientColor.alpha = matColor.alpha;

    /* Set light colors */
    RwD3D9SetVertexShaderConstant(VSCONST_REG_AMBIENT_OFFSET,
                                  (const void *)&modAmbientColor,
                                  VSCONST_REG_AMBIENT_SIZE);

    RwD3D9SetVertexShaderConstant(VSCONST_REG_DIR_LIGHT_OFFSET + 1,
                                  (const void *)&modLightColor,
                                  1);

    /* Set envmap texture matrix */
    if (RpMatFXMaterialGetEffects(mat) == rpMATFXEFFECTENVMAP)
    {
        /*
         * Unlike the FFP matFX pipeline, all these matrices are transposed
         * to enable easier calcualtions in the vertex shader
         */
        static const D3DMATRIX texMat =
            D3DMatrixInitMacro(0.5f, 0.0f, 0.0f, 0.0f,
                               0.0f,-0.5f, 0.0f, 0.0f,
                               0.0f, 0.0f, 1.0f, 0.0f,
                               0.5f, 0.5f, 0.0f, 0.0f);

        _rpTeamD3D9MatrixTransposed result;

        RwFrame *frame;

        frame = RpMatFXMaterialGetEnvMapFrame(mat);
        if (frame)
        {
            const RwMatrix    *envMtx;
            RwMatrix    invMtx;

            /* Transfrom the normals by the inverse of the env maps frame */
            envMtx = RwFrameGetLTM(frame);

            RwMatrixInvert(&invMtx, envMtx);

            invMtx.right.x = -invMtx.right.x;
            invMtx.right.y = -invMtx.right.y;
            invMtx.right.z = -invMtx.right.z;

            invMtx.flags = 0;
            invMtx.pad1 = 0;
            invMtx.pad2 = 0;
            *((RwReal *)&(invMtx.pad3)) = 1.0f;

            TeamD3D9EnvMapMatrixMultiplyTranspose(&result, (const D3DMATRIX *)&invMtx, &texMat);
        }
        else
        {
            TeamD3D9EnvMapMatrixMultiplyTranspose(&result, &_rpTeamD3D9ViewMatrix, &texMat);
        }

        /* pack relevant 2 rows into constant registers */
        RwD3D9SetVertexShaderConstant( VSCONST_REG_ENV_OFFSET,
                                       &result,
                                       VSCONST_REG_ENV_SIZE );
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpTeamD3D9UpdateLights

 Purpose:

 On entry:
 .
 On exit :
*/
static void
TeamD3D9SetDirectionalLight(RpLight *light)
{
    const RwV3d         *at;
    const RwRGBAReal    *color;

    RWFUNCTION(RWSTRING("TeamD3D9SetDirectionalLight"));

    /*
     * Prepare light for the vertex shader pipeline
     */

    /* Set the lights direction */
    at = RwMatrixGetAt(RwFrameGetLTM(RpLightGetFrame(light)));

    LightDirection.x = at->x;
    LightDirection.y = at->y;
    LightDirection.z = at->z;
    LightDirection.w = 0.0f; /* Use this for clamping */

    /* Set the light color */
    color = RpLightGetColor(light);

    LightColor = *color;

    /*
     * Prepare light for the non vertex shader pipeline
     */
    _rwD3D9LightDirectionalEnable(light);

    RWRETURNVOID();
}

void
_rpTeamD3D9UpdateLights(const RpTeam *team)
{
    RWFUNCTION(RWSTRING("_rpTeamD3D9UpdateLights"));

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe Check
	THREADSAFE_CHECK_ISCALLEDMAIN();
	//@} DDonSS

    TeamD3D9DirectionalLightFound = FALSE;
    TeamD3D9AmbientLightFound = FALSE;

    AmbientSaturated.red   = 0.0f;
    AmbientSaturated.green = 0.0f;
    AmbientSaturated.blue  = 0.0f;
    AmbientSaturated.alpha = 1.0f;

    if (team->lights[rpTEAMDIRECTIONAL1LIGHT] != NULL)
    {
        TeamD3D9SetDirectionalLight(team->lights[rpTEAMDIRECTIONAL1LIGHT]);

        TeamD3D9DirectionalLightFound = TRUE;
    }

    if (team->lights[rpTEAMAMBIENTLIGHT] != NULL)
    {
        const RwRGBAReal    *color;

        color = RpLightGetColor(team->lights[rpTEAMAMBIENTLIGHT]);

        AmbientSaturated.red   += color->red;
        AmbientSaturated.green += color->green;
        AmbientSaturated.blue  += color->blue;

        TeamD3D9AmbientLightFound = TRUE;
    }

    if (TeamD3D9DirectionalLightFound &&
        TeamD3D9AmbientLightFound)
    {
        _rwD3D9LightsEnable(TRUE, rpATOMIC);

        RWRETURNVOID();
    }

    /* Find world lights */
    if (NULL != RWSRCGLOBAL(curWorld))
    {
        RpWorld   *world;
        RwLLLink  *cur;
        const RwLLLink  *end;

        world = (RpWorld *)RWSRCGLOBAL(curWorld);

        cur = rwLinkListGetFirstLLLink(&world->directionalLightList);
        end = rwLinkListGetTerminator(&world->directionalLightList);
        while (cur != end)
        {
            RpLight *const light =
                rwLLLinkGetData(cur, RpLight, inWorld);

            /* NB light may actually be a dummyTie from a enclosing ForAll */
            if (light && (rwObjectTestFlags(light, rpLIGHTLIGHTATOMICS)))
            {
                if (RpLightGetType(light) == rpLIGHTDIRECTIONAL)
                {
                    if (!TeamD3D9DirectionalLightFound)
                    {
                        TeamD3D9SetDirectionalLight(light);

                        TeamD3D9DirectionalLightFound = TRUE;
                    }
                }
                else if (RpLightGetType(light) == rpLIGHTAMBIENT)
                {
                    if (!TeamD3D9AmbientLightFound)
                    {
                        const RwRGBAReal    *color;

                        color = RpLightGetColor(light);

                        AmbientSaturated.red   += color->red;
                        AmbientSaturated.green += color->green;
                        AmbientSaturated.blue  += color->blue;

                        TeamD3D9AmbientLightFound = TRUE;
                    }
                }

                if (TeamD3D9DirectionalLightFound &&
                    TeamD3D9AmbientLightFound)
                {
                    _rwD3D9LightsEnable(TRUE, rpATOMIC);

                    RWRETURNVOID();
                }
            }

            /* Next */
            cur = rwLLLinkGetNext(cur);
        }
    }

    if (!TeamD3D9DirectionalLightFound)
    {
        LightColor.red   = 0.0f;
        LightColor.green = 0.0f;
        LightColor.blue  = 0.0f;
        LightColor.alpha = 1.0f;

        LightDirection.x = 0.0f;
        LightDirection.y = 0.0f;
        LightDirection.z = 0.0f;
        LightDirection.w = 0.0f;
    }

    _rwD3D9LightsEnable(FALSE, rpATOMIC);

    RWRETURNVOID();
}

/****************************************************************************
 TeamD3D9ComputeProjViewWorld
 */
static void
TeamD3D9ComputeProjViewWorld(_rpTeamD3D9MatrixTransposed *projViewWorldMatrix)
{
    D3DMATRIX   projMatrix;

    RWFUNCTION(RWSTRING("TeamD3D9ComputeProjViewWorld"));
    RWASSERT( projViewWorldMatrix != NULL );

    /*
     * Projection matrix
     */
    RwD3D9GetTransform(D3DTS_PROJECTION, &projMatrix);

    /*
     * View matrix - (camera matrix)
     */
    RwD3D9GetTransform(D3DTS_VIEW, &_rpTeamD3D9ViewMatrix);

    TeamD3D9MatrixMultiplyTranspose(projViewWorldMatrix,
                                       &_rpTeamD3D9ViewMatrix,
                                       &projMatrix);

    RWRETURNVOID();
}

static void
TeamD3D9SetVertexShaderConstants(const RpAtomic *atomic,
                                 const RpSkin *skin,
                                 RwUInt32 numBones)
{
    RWFUNCTION(RWSTRING("TeamD3D9SetVertexShaderConstants"));
    RWASSERT( atomic != NULL );

    /* Calculate world constants */
    if (_rpTeamD3D9LastRenderFrame != RWSRCGLOBAL(renderFrame))
    {
        TeamD3D9ComputeProjViewWorld(&_rpTeamD3D9projViewMatrix);

        _rpTeamD3D9LastRenderFrame = RWSRCGLOBAL(renderFrame);
    }

    /* Set light direction */
    RwD3D9SetVertexShaderConstant(VSCONST_REG_DIR_LIGHT_OFFSET,
                                  (const void *)&LightDirection,
                                  1);

    if (!_rpTeamD3D9UsingLocalSpace)
    {
        const RwV4d *matrixArray;

        /*
         * Set the constant registers with the combined
         * camera & projection matrix
         */
        RwD3D9SetVertexShaderConstant(VSCONST_REG_TRANSFORM_OFFSET,
                                      (const void *)&_rpTeamD3D9projViewMatrix,
                                      VSCONST_REG_TRANSFORM_SIZE);

        /* Set the bone matrixs */
        matrixArray = (const RwV4d *)_rpTeamSkinGetCurrentMatrixCache();

        if (skin->boneData.numUsedBones < numBones)
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
                    n++;
                }
                n++;

                RwD3D9SetVertexShaderConstant(VSCONST_REG_MATRIX_OFFSET + usedBones[baseIndex] * NUM_ROWS,
                                              (const void *)(matrixArray + usedBones[baseIndex] * NUM_ROWS),
                                              (n - baseIndex) * NUM_ROWS);
            }
            while(n < numUsedBones);
        }
        else
        {
            RwD3D9SetVertexShaderConstant(VSCONST_REG_MATRIX_OFFSET,
                                          (const void *)matrixArray,
                                          numBones * NUM_ROWS);
        }
    }
    else
    {
        RwFrame     *frame = RpAtomicGetFrame(atomic);
        const RwMatrix  *ltm = RwFrameGetLTM(frame);
        _rpTeamD3D9MatrixTransposed projViewWorldMatrix;

        _rwTeamD3D9SkinMatrixMultiplyTranspose(
                                        &projViewWorldMatrix,
                                        ltm,
                                        &_rpTeamD3D9projViewMatrix);

        /*
         * Set the constant registers with the combined
         * camera & projection matrix
         */
        RwD3D9SetVertexShaderConstant(VSCONST_REG_TRANSFORM_OFFSET,
                                      (const void *)&projViewWorldMatrix,
                                      VSCONST_REG_TRANSFORM_SIZE);

        if (!_rpTeamD3D9UsingTransposedMatrix)
        {
            const RwUInt32 numUsedBones = skin->boneData.numUsedBones;
            const RwUInt8 *usedBones = skin->boneData.usedBoneList;
            const RwMatrix  *sourceMatrix =
                        (const RwMatrix *)_rpTeamSkinGetCurrentMatrixCache();
            RwUInt32 n;

            if (_rpTeamD3D9MaxTransposedMatrix < numUsedBones)
            {
                _rpTeamD3D9MaxTransposedMatrix = numUsedBones;

                if (_rpTeamD3D9TransposedMatrix == NULL)
                {
                    _rpTeamD3D9TransposedMatrix =
                    (_rpTeamD3D9MatrixTransposed3Rows *)
                    RwMalloc(numUsedBones * sizeof(_rpTeamD3D9MatrixTransposed3Rows),
                             rwID_TEAMPLUGIN | rwMEMHINTDUR_EVENT |
                             rwMEMHINTFLAG_RESIZABLE);
                }
                else
                {
                    _rpTeamD3D9TransposedMatrix =
                    (_rpTeamD3D9MatrixTransposed3Rows *)
                    RwRealloc(_rpTeamD3D9TransposedMatrix,
                              numUsedBones * sizeof(_rpTeamD3D9MatrixTransposed3Rows),
                              rwID_TEAMPLUGIN | rwMEMHINTDUR_EVENT |
                              rwMEMHINTFLAG_RESIZABLE);
                }
            }

            for (n = 0; n < numUsedBones; n++)
            {
                const RwMatrix  *currentMatrix = &(sourceMatrix[usedBones[n]]);
                _rpTeamD3D9MatrixTransposed3Rows *transposedMatrix =
                                                &(_rpTeamD3D9TransposedMatrix[n]);

                transposedMatrix->right_x = currentMatrix->right.x;
                transposedMatrix->right_y = currentMatrix->right.y;
                transposedMatrix->right_z = currentMatrix->right.z;
                transposedMatrix->up_x = currentMatrix->up.x;
                transposedMatrix->up_y = currentMatrix->up.y;
                transposedMatrix->up_z = currentMatrix->up.z;
                transposedMatrix->at_x = currentMatrix->at.x;
                transposedMatrix->at_y = currentMatrix->at.y;
                transposedMatrix->at_z = currentMatrix->at.z;
                transposedMatrix->pos_x = currentMatrix->pos.x;
                transposedMatrix->pos_y = currentMatrix->pos.y;
                transposedMatrix->pos_z = currentMatrix->pos.z;
            }

            RwD3D9SetVertexShaderConstant(VSCONST_REG_MATRIX_OFFSET,
                                          (const void *)_rpTeamD3D9TransposedMatrix,
                                          numUsedBones * NUM_ROWS);
        }
        else
        {
            const RwUInt32 numUsedBones = skin->boneData.numUsedBones;
            const RwUInt8 *usedBones = skin->boneData.usedBoneList;
            const _rpTeamD3D9MatrixTransposed   *sourceMatrix =
                    (const _rpTeamD3D9MatrixTransposed *)_rpTeamSkinGetCurrentMatrixCache();
            RwUInt32 n;

            for (n = 0; n < numUsedBones; n++)
            {
                RwD3D9SetVertexShaderConstant(VSCONST_REG_MATRIX_OFFSET + n * NUM_ROWS,
                                              (const void *)&(sourceMatrix[usedBones[n]]),
                                              NUM_ROWS);
            }
        }
    }

    RWRETURNVOID();
}

RwBool
_rxTeamD3D9SkinnedAtomicAllInOneNode(RxPipelineNodeInstance *self,
                        const RxPipelineNodeParam *params)
{
    RpAtomic            *atomic;
    RpHAnimHierarchy    *hierarchy;
    RpGeometry          *geometry;
    RwBool              retVal;

    RWFUNCTION(RWSTRING("_rxTeamD3D9SkinnedAtomicAllInOneNode"));
    RWASSERT(NULL != params);

    atomic = (RpAtomic *)RxPipelineNodeParamGetData(params);
    RWASSERT(NULL != atomic);

    hierarchy = RpHAnimFrameGetHierarchy(_rpTeamPlayerGetCurrentPlayer()->frame);

    RPSKINATOMICGETDATA(atomic)->hierarchy = hierarchy;

    geometry = RpAtomicGetGeometry(atomic);
    RWASSERT(NULL != geometry);

    if (hierarchy != NULL)
    {
        const RpSkin        *skin;

        skin = *RPSKINGEOMETRYGETCONSTDATA(geometry);

        _rpTeamD3D9UsingVertexShader = (skin != NULL &&
                                        skin->boneData.numUsedBones <=
                                        _rpTeamD3D9MaxBonesHardwareSupported);

        _rpTeamD3D9UsingLocalSpace = ((RwUInt32)hierarchy->numNodes >
                                      _rpTeamD3D9MaxBonesHardwareSupported);
    }
    else
    {
        _rpTeamD3D9UsingVertexShader = FALSE;
        _rpTeamD3D9UsingLocalSpace = TRUE;
    }

    if (_rpTeamD3D9UsingVertexShader)
    {
        if( _rpTeamShadowGetCurrentShadowData()->playerClip != 0 )
        {
            _rxD3D9SkinInstanceNodeData *privateData;
            RpMeshHeader    *meshHeader;
            RwUInt32        geomFlags;
            RwResEntry      *resEntry;

            /*
             * Use vertex shader
             */

            privateData = (_rxD3D9SkinInstanceNodeData *)self->privateData;

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

            resEntry = geometry->repEntry;

            /* If the meshes have changed we should re-instance */
            if (resEntry)
            {
                RxD3D9ResEntryHeader    *resEntryHeader;

                resEntryHeader = (RxD3D9ResEntryHeader *)(resEntry + 1);
                if (resEntryHeader->serialNumber != meshHeader->serialNum)
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

                owner = (void *)geometry;
                resEntryPointer = &geometry->repEntry;

                /*
                 * Create vertex buffers and instance
                 */
                resEntry = TeamD3D9SkinInstance(atomic, owner, resEntryPointer,
                                                meshHeader,
                                                hierarchy->numNodes);
                if (!resEntry)
                {
                    RWRETURN(FALSE);
                }

                /* The geometry is up to date */
                geometry->lockedSinceLastInst = 0;
            }
            else
            {
                /* Update vertex shader pointers */
                RxD3D9ResEntryHeader    *resEntryHeader;
                RxD3D9InstanceData      *instancedData;
                RwInt32                 numMeshes;

                resEntryHeader = (RxD3D9ResEntryHeader *)(resEntry + 1);
                instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

                numMeshes = resEntryHeader->numMeshes;
                while (numMeshes--)
                {
                    if (RpMatFXMaterialGetEffects(instancedData->material) == rpMATFXEFFECTENVMAP)
                    {
                        instancedData->vertexShader = _rpTeamD3D9EnvMapSkinVertexShader;
                    }
                    else if (RpGeometryGetNumTexCoordSets(geometry) == 2)
                    {
                        instancedData->vertexShader = _rpTeamD3D9DualSkinVertexShader;
                    }
                    else
                    {
                        instancedData->vertexShader = _rpTeamD3D9SkinVertexShader;
                    }

                    instancedData++;
                }

                /* We have a resEntry so use it */
                RwResourcesUseResEntry(resEntry);
            }

            /*
             * Set vertex shader constants
             */
            TeamD3D9SetVertexShaderConstants(atomic,
                                             *RPSKINGEOMETRYGETCONSTDATA(geometry),
                                             hierarchy->numNodes);

            /*
             * Render
             */
            if (privateData->renderCallback)
            {
                privateData->renderCallback(resEntry, (void *)atomic, rpATOMIC, geomFlags);
            }

            #ifdef RWMETRICS
            /* Now update our metrics statistics */
            RWSRCGLOBAL(metrics)->numVertices  += RpGeometryGetNumVertices(geometry);
            RWSRCGLOBAL(metrics)->numTriangles += RpGeometryGetNumTriangles(geometry);
            #endif
        }

        retVal = TRUE;
    }
    else
    {
        RwMatrix            *skinAlignedMatrixArray;

        /* store global matrix cache */
        skinAlignedMatrixArray = _rpSkinGlobals.matrixCache.aligned;

        /* Trick to not calculate the matrix array again */
        _rpSkinGlobals.platform.lastHierarchyUsed = (const RpHAnimHierarchy *)hierarchy;
        _rpSkinGlobals.platform.lastRenderFrame = RWSRCGLOBAL(renderFrame);

        /* Force to use the already calculated matrix array */
        _rpSkinGlobals.matrixCache.aligned = _rpTeamSkinGetCurrentMatrixCache();

        /* normal skinning plugin function */
        retVal = _rwSkinD3D9AtomicAllInOneNode(self, params);

        /* restore global matrix cache */
        _rpSkinGlobals.matrixCache.aligned = skinAlignedMatrixArray;
    }

    RWRETURN(retVal);
}

/****************************************************************************
 TeamD3D9SkinnedRenderCallback
 */
static void
TeamD3D9SkinnedRenderCallback(RwResEntry *repEntry,
                              void *object,
                              RwUInt8 type,
                              RwUInt32 flags)
{
    RWFUNCTION(RWSTRING("TeamD3D9SkinnedRenderCallback"));

    /* Check if player is visible */
    if( _rpTeamShadowGetCurrentShadowData()->playerClip != 0 )
    {
        RxD3D9ResEntryHeader    *resEntryHeader;
        RxD3D9InstanceData      *instancedData;
        RwBool                  lighting;
        RwBool                  forceBlack;
        RwBool                  vertexAlphaBlend;
        RwInt32                 numMeshes;
        RwTexture               *baseTexture;
        RwRGBA                  oldColor;

        /* Enable clipping if needed */
        RwD3D9SetRenderState(D3DRS_CLIPPING, (_rpTeamShadowGetCurrentShadowData()->playerClip & 0x2) != 0);

        /* Get the instanced data */
        resEntryHeader = (RxD3D9ResEntryHeader *)(repEntry + 1);
        instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

        /* Get lighting state */
        RwD3D9GetRenderState(D3DRS_LIGHTING, &lighting);

        if (lighting ||
            (flags & rxGEOMETRY_PRELIT) != 0 ||
            instancedData->vertexShader != NULL)
        {
            forceBlack = FALSE;

            RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        }
        else
        {
            forceBlack = TRUE;

            RwD3D9SetTexture(NULL, 0);

            RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, 0xff000000);

            RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
        }

        /* Get vertex alpha Blend state */
        vertexAlphaBlend = _rwD3D9RenderStateIsVertexAlphaEnable();

        /*
         * Set the default Pixel shader
         */
        RwD3D9SetPixelShader(NULL);

        /* Set the Index buffer */
        RwD3D9SetIndices(resEntryHeader->indexBuffer);

        /* Set the stream source */
        _rwD3D9SetStreams(resEntryHeader->vertexStream,
                          resEntryHeader->useOffsets);

        /*
         * Vertex declaration
         */
        RwD3D9SetVertexDeclaration(resEntryHeader->vertexDeclaration);

        /*
         * Vertex shader
         */
        RwD3D9SetVertexShader(instancedData->vertexShader);

        /* Get the number of meshes */
        numMeshes = resEntryHeader->numMeshes;
        while (numMeshes--)
        {
            oldColor = instancedData->material->color;

            instancedData->material->color =
                *RpTeamMaterialGetPlayerColor(instancedData->material,
                                              _rpTeamPlayerGetCurrentPlayer());

            if ( (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2)) )
            {
                baseTexture = RpTeamMaterialGetPlayerTexture(instancedData->material,
                                                   _rpTeamPlayerGetCurrentPlayer());

                RwD3D9SetTexture(baseTexture, 0);
            }

            if (instancedData->vertexAlpha ||
                (0xFF != instancedData->material->color.alpha))
            {
                if (!vertexAlphaBlend)
                {
                    vertexAlphaBlend = TRUE;

                    _rwD3D9RenderStateVertexAlphaEnable(TRUE);
                }
            }
            else
            {
                if (vertexAlphaBlend)
                {
                    vertexAlphaBlend = FALSE;

                    _rwD3D9RenderStateVertexAlphaEnable(FALSE);
                }
            }

            /* Check if using a vertex shader */
            if (!_rpTeamD3D9UsingVertexShader)
            {
                if (lighting)
                {
                    RwD3D9SetSurfaceProperties(&instancedData->material->surfaceProps,
                                               &instancedData->material->color,
                                               flags);
                }
            }
            else
            {
                _rpTeamD3D9SetVertexShaderMaterialColor(instancedData->material);
            }

            /* Draw the indexed primitive */
            RwD3D9DrawIndexedPrimitive((D3DPRIMITIVETYPE)resEntryHeader->primType,
                                       instancedData->baseIndex,
                                       0, instancedData->numVertices,
                                       instancedData->startIndex,
                                       instancedData->numPrimitives);

            instancedData->material->color = oldColor;

            /* Move onto the next instancedData */
            instancedData++;
        }

        if (forceBlack)
        {
            RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
            RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
            RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        }
    }

    /* Render the shadows */
    #if defined(TEAMSHADOWALLINONE)
    if (!_rpTeamD3D9UsingVertexShader)
    {
        _rwTeamD3D9SkinnedShadowsRenderCallback(repEntry, object, type, flags);
    }
    #endif

    RWRETURNVOID();
}

/****************************************************************************
 _rxTeamD3D9AtomicLightingCallback
 */
void
_rxTeamD3D9AtomicLightingCallback(void *object __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("_rxTeamD3D9AtomicLightingCallback"));

    if (TeamD3D9AmbientLightFound)
    {
        RwD3D9SetRenderState(D3DRS_LIGHTING, TRUE);
    }
    else
    {
        RwD3D9SetRenderState(D3DRS_AMBIENT, 0);

        if (TeamD3D9DirectionalLightFound)
        {
            RwD3D9SetRenderState(D3DRS_LIGHTING, TRUE);
        }
        else
        {
            RwD3D9SetRenderState(D3DRS_LIGHTING, FALSE);
        }
    }

    RWRETURNVOID();
}

/****************************************************************************
 TeamSkinD3D9AtomicAllInOnePipelineInit
 */
static RwBool
TeamSkinD3D9AtomicAllInOnePipelineInit(RxPipelineNode *node)
{
    _rxD3D9SkinInstanceNodeData *instanceData;

    RWFUNCTION(RWSTRING("TeamSkinD3D9AtomicAllInOnePipelineInit"));

    instanceData = (_rxD3D9SkinInstanceNodeData *)node->privateData;

    instanceData->renderCallback = TeamD3D9SkinnedRenderCallback;

    instanceData->lightingCallback = _rxTeamD3D9AtomicLightingCallback;

    RWRETURN(TRUE);
}

/*
 * NodeDefinitionGetD3D9TeamSkinAtomicAllInOne returns a
 * pointer to the \ref RxNodeDefinition associated with
 * the Skin Atomic version of PowerPipe.
 */
static RxNodeDefinition *
NodeDefinitionGetD3D9TeamSkinAtomicAllInOne(void)
{
    static RwChar _TeamSkinAtomicInstance_csl[] = RWSTRING("nodeTeamSkinAtomicAllInOne.csl");

    static RxNodeDefinition nodeD3D9SkinAtomicAllInOneCSL = { /* */
        _TeamSkinAtomicInstance_csl,                    /* Name */
        {                                           /* Nodemethods */
            _rxTeamD3D9SkinnedAtomicAllInOneNode,   /* +-- nodebody */
            NULL,                                   /* +-- nodeinit */
            NULL,                                   /* +-- nodeterm */
            TeamSkinD3D9AtomicAllInOnePipelineInit,  /* +-- pipelinenodeinit */
            NULL,                                   /* +-- pipelineNodeTerm */
            NULL,                                   /* +-- pipelineNodeConfig */
            NULL,                                   /* +-- configMsgHandler */
        },
        {                                           /* Io */
            NUMCLUSTERSOFINTEREST,                  /* +-- NumClustersOfInterest */
            NULL,                                   /* +-- ClustersOfInterest */
            NULL,                                   /* +-- InputRequirements */
            NUMOUTPUTS,                             /* +-- NumOutputs */
            NULL                                    /* +-- Outputs */
        },
        (RwUInt32)sizeof(_rxD3D9SkinInstanceNodeData),/* PipelineNodePrivateDataSize */
        (RxNodeDefEditable)FALSE,           /* editable */
        0                                           /* inPipes */
    };

    RWFUNCTION(RWSTRING("NodeDefinitionGetD3D9TeamSkinAtomicAllInOne"));

    RWRETURN(&nodeD3D9SkinAtomicAllInOneCSL);
}

static RxPipeline *
TeamD3D9SkinnedPipelineCreate(void)
{
    RxPipeline  *pipe;

    RWFUNCTION(RWSTRING("TeamD3D9SkinnedPipelineCreate"));

    pipe = RxPipelineCreate();

    if (pipe)
    {
        RxLockedPipe    *lpipe;

        pipe->pluginId = rwID_TEAMPLUGIN;
        pipe->pluginData = TEAMD3D9PIPEID_SKINNED;

        if ((lpipe = RxPipelineLock(pipe)) != NULL)
        {
            RxNodeDefinition    *instanceNode;

            /*
             * Get the default instance node definition
             */
            instanceNode = NodeDefinitionGetD3D9TeamSkinAtomicAllInOne();
            RWASSERT(NULL != instanceNode);

            lpipe = RxLockedPipeAddFragment(lpipe, NULL,
                                           instanceNode,
                                           NULL);

            lpipe = RxLockedPipeUnlock(lpipe);

            RWASSERT(pipe == (RxPipeline *)lpipe);
            RWRETURN(pipe);
        }

        RxPipelineDestroy(pipe);

        pipe = NULL;
    }

    RWRETURN(pipe);
}

RwBool
_rpTeamSkinCustomPipelineCreate(void)
{
    RxPipeline      *pipe;

    RWFUNCTION(RWSTRING("_rpTeamSkinCustomPipelineCreate"));

    /*
     * Check hardware support
     */
    D3DCaps = (const D3DCAPS9 *)RwD3D9GetCaps();
    if ( (D3DCaps->VertexShaderVersion & 0xffff) >= 0x0101 &&
         D3DCaps->MaxVertexShaderConst >= 96 )
    {
        _rpTeamD3D9MaxBonesHardwareSupported = D3DCaps->MaxVertexShaderConst;

        /* substract view & projection matrix */
        /* substract ambient & directional light */
        /* substract material */
        _rpTeamD3D9MaxBonesHardwareSupported -= 4 + 3 + 2;

        _rpTeamD3D9MaxBonesHardwareSupported /= 3;

        /* Create vertex shaders */
        if ((D3DCaps->VertexShaderVersion & 0xffff) >= 0x0200)
        {
            if (RwD3D9CreateVertexShader((const RwUInt32 *)dwSkinshdr2VertexShader,
                                        &_rpTeamD3D9SkinVertexShader) == FALSE)
            {
                _rpTeamD3D9MaxBonesHardwareSupported = 0;
            }

            if (RwD3D9CreateVertexShader((const RwUInt32 *)dwSkinshdr_unlit2VertexShader,
                                            &_rpTeamD3D9UnlitSkinVertexShader) == FALSE)
            {
                _rpTeamD3D9MaxBonesHardwareSupported = 0;
            }

            if (RwD3D9CreateVertexShader((const RwUInt32 *)dwSkinshdr_unlit_nofog2VertexShader,
                                            &_rpTeamD3D9UnlitNoFogSkinVertexShader) == FALSE)
            {
                _rpTeamD3D9MaxBonesHardwareSupported = 0;
            }

            if (RwD3D9CreateVertexShader((const RwUInt32 *)dwDualskinshdr2VertexShader,
                                            &_rpTeamD3D9DualSkinVertexShader) == FALSE)
            {
                _rpTeamD3D9MaxBonesHardwareSupported = 0;
            }

            if (RwD3D9CreateVertexShader((const RwUInt32 *)dwEnvmapskinshdr2VertexShader,
                                            &_rpTeamD3D9EnvMapSkinVertexShader) == FALSE)
            {
                _rpTeamD3D9MaxBonesHardwareSupported = 0;
            }
        }
        else
        {
            if (RwD3D9CreateVertexShader((const RwUInt32 *)dwSkinshdrVertexShader,
                                        &_rpTeamD3D9SkinVertexShader) == FALSE)
            {
                _rpTeamD3D9MaxBonesHardwareSupported = 0;
            }

            if (RwD3D9CreateVertexShader((const RwUInt32 *)dwSkinshdr_unlitVertexShader,
                                            &_rpTeamD3D9UnlitSkinVertexShader) == FALSE)
            {
                _rpTeamD3D9MaxBonesHardwareSupported = 0;
            }

            if (RwD3D9CreateVertexShader((const RwUInt32 *)dwSkinshdr_unlit_nofogVertexShader,
                                            &_rpTeamD3D9UnlitNoFogSkinVertexShader) == FALSE)
            {
                _rpTeamD3D9MaxBonesHardwareSupported = 0;
            }

            if (RwD3D9CreateVertexShader((const RwUInt32 *)dwDualskinshdrVertexShader,
                                            &_rpTeamD3D9DualSkinVertexShader) == FALSE)
            {
                _rpTeamD3D9MaxBonesHardwareSupported = 0;
            }

            if (RwD3D9CreateVertexShader((const RwUInt32 *)dwEnvmapskinshdrVertexShader,
                                            &_rpTeamD3D9EnvMapSkinVertexShader) == FALSE)
            {
                _rpTeamD3D9MaxBonesHardwareSupported = 0;
            }
        }
    }
    else
    {
        _rpTeamD3D9MaxBonesHardwareSupported = 0;
    }

    /* Create pipeline */
    pipe = TeamD3D9SkinnedPipelineCreate();

    if (pipe)
    {
        TeamSkinedPipelineIndex = _rpTeamPipeAddPipeDefinition(pipe,
                                      (TeamPipeOpen)NULL,
                                      (TeamPipeClose)NULL);

        _rpTeamD3D9SetRenderPipeline(pipe, TEAMD3D9PIPEID_SKINNED);

        RWRETURN(TRUE);
    }

    RWRETURN(FALSE);
}

void
_rpTeamSkinCustomPipelineDestroy(void)
{
    RWFUNCTION(RWSTRING("_rpTeamSkinCustomPipelineDestroy"));

    RWASSERT(TEAMPIPENULL != TeamSkinedPipelineIndex);

    _rpTeamPipeRemovePipeDefinition(TeamSkinedPipelineIndex);
    TeamSkinedPipelineIndex = TEAMPIPENULL;

    _rpTeamD3D9SetRenderPipeline(NULL, TEAMD3D9PIPEID_SKINNED);

    /* Destroy aux memory */
    if (_rpTeamD3D9TransposedMatrix != NULL)
    {
        RwFree(_rpTeamD3D9TransposedMatrix);
        _rpTeamD3D9TransposedMatrix = NULL;
    }
    _rpTeamD3D9MaxTransposedMatrix = 0;

    /* Destroy vertex shaders */
    if (_rpTeamD3D9MaxBonesHardwareSupported > 0)
    {
        RwD3D9DeleteVertexShader(_rpTeamD3D9UnlitNoFogSkinVertexShader);
        RwD3D9DeleteVertexShader(_rpTeamD3D9UnlitSkinVertexShader);

        RwD3D9DeleteVertexShader(_rpTeamD3D9SkinVertexShader);
        RwD3D9DeleteVertexShader(_rpTeamD3D9DualSkinVertexShader);
        RwD3D9DeleteVertexShader(_rpTeamD3D9EnvMapSkinVertexShader);

        _rpTeamD3D9MaxBonesHardwareSupported = 0;
    }

    RWRETURNVOID();
}

RxPipeline *
_rpTeamSkinGetCustomPipeline(void)
{
    RWFUNCTION(RWSTRING("_rpTeamSkinGetCustomPipeline"));
    RWRETURN(_rpTeamPipeGetPipeline(TeamSkinedPipelineIndex));
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/
RwMatrix *
_rpTeamSkinMatBlendUpdating( RpSkin *skin,
                             RwFrame *frame,
                             RpHAnimHierarchy *hierarchy,
                             RwMatrix *matrixArray )
{
    RWFUNCTION(RWSTRING("_rpTeamSkinMatBlendUpdating"));

    if(NULL != hierarchy)
    {
        RwInt32         i;
        const RwMatrix  *skinToBone;

        RwMatrix    inverseAtomicLTM;
        RwMatrix    temmatrix;

        /* Get the bone information. */
        skinToBone = RpSkinGetSkinToBoneMatrices(skin);
        RWASSERT(NULL != skinToBone);

        if ((RwUInt32)hierarchy->numNodes <= _rpTeamD3D9MaxBonesHardwareSupported)
        {
            _rpTeamD3D9MatrixTransposed3Rows *matrixArrayTransposed =
                               (_rpTeamD3D9MatrixTransposed3Rows *)matrixArray;

            /* Vertex shader is working on world space */
            if(hierarchy->flags & rpHANIMHIERARCHYNOMATRICES)
            {

                for( i = 0; i < hierarchy->numNodes; i++ )
                {
                    const RwMatrix *ltm =
                        RwFrameGetLTM(hierarchy->pNodeInfo[i].pFrame);

                    RWASSERT(NULL != ltm);

                    TeamD3D9MatrixMultiplyTranspose3Rows(
                                                    &matrixArrayTransposed[i],
                                                    &skinToBone[i],
                                                    ltm );
                }
            }
            else if(hierarchy->flags & rpHANIMHIERARCHYLOCALSPACEMATRICES)
            {
                const RwMatrix *localToWorld =
                                RwFrameGetLTM(frame);

                RwMatrix temmatrix;

                for( i = 0; i < hierarchy->numNodes; i++ )
                {
                    RwMatrixMultiply( &temmatrix,
                                      &skinToBone[i],
                                      &(hierarchy->pMatrixArray[i]) );

                    TeamD3D9MatrixMultiplyTranspose3Rows(
                                                    &matrixArrayTransposed[i],
                                                    &temmatrix,
                                                    localToWorld );
                }
            }
            else
            {
                for( i = 0; i < hierarchy->numNodes; i++)
                {
                    TeamD3D9MatrixMultiplyTranspose3Rows(
                                                &matrixArrayTransposed[i],
                                                &skinToBone[i],
                                                &(hierarchy->pMatrixArray[i]) );
                }
            }
        }
        else
#if !defined(NOASM) && !defined(NOSSEASM)
        if (_rwIntelSSEsupported())
        {
            if(hierarchy->flags & rpHANIMHIERARCHYNOMATRICES)
            {
                const RwMatrix *ltm = RwFrameGetLTM(frame);
                RWASSERT(NULL != ltm);

                RwMatrixInvert(&inverseAtomicLTM, ltm);

                for( i = 0; i < hierarchy->numNodes; i++ )
                {
                    const RwMatrix *ltm =
                                    RwFrameGetLTM(hierarchy->pNodeInfo[i].pFrame);
                    RWASSERT(NULL != ltm);

                    RwMatrixMultiply( &temmatrix,
                                      &skinToBone[i],
                                      ltm );

                    RwMatrixMultiply( &matrixArray[i],
                                      &temmatrix,
                                      &inverseAtomicLTM );
                }
            }
            else if(hierarchy->flags & rpHANIMHIERARCHYLOCALSPACEMATRICES)
            {
                for( i = 0; i < hierarchy->numNodes; i++ )
                {
                    RwMatrixMultiply( &matrixArray[i],
                                      &skinToBone[i],
                                      &(hierarchy->pMatrixArray[i]) );
                }
            }
            else
            {
                const RwMatrix *ltm = RwFrameGetLTM(frame);
                RWASSERT(NULL != ltm);

                RwMatrixInvert(&inverseAtomicLTM, ltm);

                for( i = 0; i < hierarchy->numNodes; i++)
                {
                    RwMatrixMultiply( &temmatrix,
                                      &skinToBone[i],
                                      &(hierarchy->pMatrixArray[i]) );

                    RwMatrixMultiply( &matrixArray[i],
                                      &temmatrix,
                                      &inverseAtomicLTM );
                }
            }

            for( i = 0; i < hierarchy->numNodes; i++)
            {
                matrixArray[i].flags=0;
                matrixArray[i].pad1=0;
                matrixArray[i].pad2=0;
                matrixArray[i].pad3=0;
            }
        }
        else
#endif /* !defined(NOASM) && !defined(NOSSEASM) */
        {
            _rwD3D9MatrixTransposed *matrixArrayTransposed =
                                        (_rwD3D9MatrixTransposed *)matrixArray;

            _rpTeamD3D9UsingTransposedMatrix = TRUE;

            if(hierarchy->flags & rpHANIMHIERARCHYNOMATRICES)
            {
                const RwMatrix *ltm = RwFrameGetLTM(frame);
                RWASSERT(NULL != ltm);

                RwMatrixInvert(&inverseAtomicLTM, ltm);

                for( i = 0; i < hierarchy->numNodes; i++ )
                {
                    const RwMatrix *ltm =
                        RwFrameGetLTM(hierarchy->pNodeInfo[i].pFrame);
                    RWASSERT(NULL != ltm);

                    RwMatrixMultiply( &temmatrix,
                                      &skinToBone[i],
                                      ltm );

                    _rwD3D9MatrixMultiplyTranspose(&matrixArrayTransposed[i],
                                                   &temmatrix,
                                                   &inverseAtomicLTM );
                }
            }
            else if(hierarchy->flags & rpHANIMHIERARCHYLOCALSPACEMATRICES)
            {
                for( i = 0; i < hierarchy->numNodes; i++ )
                {
                    _rwD3D9MatrixMultiplyTranspose( &matrixArrayTransposed[i],
                                                    &skinToBone[i],
                                                    &(hierarchy->pMatrixArray[i]) );
                }
            }
            else
            {
                const RwMatrix *ltm = RwFrameGetLTM(frame);
                RWASSERT(NULL != ltm);

                RwMatrixInvert(&inverseAtomicLTM, ltm);

                for( i = 0; i < hierarchy->numNodes; i++)
                {
                    RwMatrixMultiply( &temmatrix,
                                      &skinToBone[i],
                                      &(hierarchy->pMatrixArray[i]) );

                    _rwD3D9MatrixMultiplyTranspose(&matrixArrayTransposed[i],
                                                   &temmatrix,
                                                   &inverseAtomicLTM );
                }
            }
        }
    }

    RWRETURN(matrixArray);
}

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/
