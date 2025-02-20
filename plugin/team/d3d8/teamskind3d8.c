/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/

#include <d3d8.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpteam.h"

#include "teampipes.h"
#include "teamskin.h"
#include "team.h"

#include "../../plugin/skin2/d3d8/skind3d8.h"
#include "../../plugin/skin2/skin.h"

#include "teamstaticd3d8.h"

/* Pre compiled vertex shader */
#include "skindefs.h"
#include "skinshdr.h"
#include "dualskinshdr.h"
#include "envmapskinshdr.h"
#include "skinshdr_unlit.h"
#include "skinshdr_unlit_nofog.h"

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
typedef struct _rpTeamD3D8MatrixTransposed3Rows _rpTeamD3D8MatrixTransposed3Rows;
struct _rpTeamD3D8MatrixTransposed3Rows
{
    RwReal right_x, up_x, at_x, pos_x;
    RwReal right_y, up_y, at_y, pos_y;
    RwReal right_z, up_z, at_z, pos_z;
};

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

RwUInt32  _rpTeamD3D8MaxBonesHardwareSupported = 0;

_rpTeamD3D8MatrixTransposed _rpTeamD3D8projViewMatrix;

RwBool  _rpTeamD3D8UsingVertexShader = FALSE;
RwBool  _rpTeamD3D8UsingLocalSpace = FALSE; /* TRUE if numNodes > _rpTeamD3D8MaxBonesHardwareSupported */
static RwBool  _rpTeamD3D8UsingTransposedMatrix = FALSE;

static _rpTeamD3D8MatrixTransposed3Rows *_rpTeamD3D8TransposedMatrix = NULL;
static RwUInt32 _rpTeamD3D8MaxTransposedMatrix = 0;

RwUInt32 _rpTeamD3D8UnlitSkinVertexShader = 0;
RwUInt32 _rpTeamD3D8UnlitNoFogSkinVertexShader = 0;

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
static const D3DPRIMITIVETYPE _RwD3D8PrimConvTable[NUMPRIMTYPES] =
{
    (D3DPRIMITIVETYPE)0,    /* rwPRIMTYPENAPRIMTYPE */
    D3DPT_LINELIST,         /* rwPRIMTYPELINELIST */
    D3DPT_LINESTRIP,        /* rwPRIMTYPEPOLYLINE */
    D3DPT_TRIANGLELIST,     /* rwPRIMTYPETRILIST */
    D3DPT_TRIANGLESTRIP,    /* rwPRIMTYPETRISTRIP */
    D3DPT_TRIANGLEFAN,      /* rwPRIMTYPETRIFAN */
    D3DPT_POINTLIST
};

static const RwUInt32 SkinVertexShaderDecl[] =
{
    D3DVSD_STREAM( 0 ),
    D3DVSD_REG( VSD_REG_POS,       D3DVSDT_FLOAT3 ),   /* Position */
    D3DVSD_REG( VSD_REG_WEIGHTS,   D3DVSDT_FLOAT2 ),   /* Weights */
    D3DVSD_REG( VSD_REG_INDICES,   D3DVSDT_SHORT2 ),   /* Indices */
    D3DVSD_REG( VSD_REG_NORMAL,    D3DVSDT_FLOAT3 ),   /* Normals */
    D3DVSD_REG( VSD_REG_TEXCOORDS, D3DVSDT_FLOAT2 ),   /* Texture coordinates */
    D3DVSD_END()
};

static const RwUInt32 DualSkinVertexShaderDecl[] =
{
    D3DVSD_STREAM( 0 ),
    D3DVSD_REG( VSD_REG_POS,       D3DVSDT_FLOAT3 ),   /* Position */
    D3DVSD_REG( VSD_REG_WEIGHTS,   D3DVSDT_FLOAT2 ),   /* Weights */
    D3DVSD_REG( VSD_REG_INDICES,   D3DVSDT_SHORT2 ),   /* Indices */
    D3DVSD_REG( VSD_REG_NORMAL,    D3DVSDT_FLOAT3 ),   /* Normals */
    D3DVSD_REG( VSD_REG_TEXCOORDS, D3DVSDT_FLOAT2 ),   /* Texture coordinates */
    D3DVSD_REG( VSD_REG_TEXCOORDS2,D3DVSDT_FLOAT2 ),   /* Texture coordinates */
    D3DVSD_END()
};

static RwUInt32 _rpTeamD3D8SkinVertexShader = 0;
static RwUInt32 _rpTeamD3D8DualSkinVertexShader = 0;
static RwUInt32 _rpTeamD3D8EnvMapSkinVertexShader = 0;

static D3DMATRIX _rpTeamD3D8ViewMatrix;

static RwRGBAReal   LightColor =
{
    0.0f, 0.0f, 0.0f, 0.0f
};

static RwV4d   LightDirection =
{
    0.0f, 0.0f, 0.0f, 0.0f
};

static RwUInt32 _rpTeamD3D8LastRenderFrame = 0xffffffff;

static TeamPipeIndex    TeamSkinedPipelineIndex = (TeamPipeIndex)TEAMPIPENULL;

static RwBool  TeamD3D8DirectionalLightFound = FALSE;
static RwBool  TeamD3D8AmbientLightFound = FALSE;

/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

/*****************************************************************************
 D3D8TeamSkinDestroyVertexBuffer
 Destroy all the buffer memory
 Inputs :
 Outputs :
 */
static void
D3D8TeamSkinDestroyVertexBuffer( RwResEntry *repEntry )
{
    RxD3D8ResEntryHeader    *resEntryHeader;
    RxD3D8InstanceData      *instancedData;
    LPDIRECT3DVERTEXBUFFER8 vertexBuffer;
    RwUInt32                numMeshes;
    RwUInt32                fvf;
    RwUInt32                stride;
    RwUInt32                numVertices;
    RwUInt32                baseIndex;

    RWFUNCTION(RWSTRING("D3D8TeamSkinDestroyVertexBuffer"));

    resEntryHeader = (RxD3D8ResEntryHeader *)(repEntry + 1);

    /* Get the instanced data structures */
    instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

    /* Save shared data */
    if (instancedData->managed)
    {
        vertexBuffer = (LPDIRECT3DVERTEXBUFFER8)instancedData->vertexBuffer;
        fvf = instancedData->vertexShader;
        stride = instancedData->stride;
        baseIndex = (instancedData->baseIndex - instancedData->minVert);
    }
    else
    {
        vertexBuffer = NULL;
        fvf = 0;
        stride = 0;
        baseIndex = 0;
    }

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
        /* Check if using a vertex shader */
        if (fvf & D3DFVF_RESERVED0)
        {
            _rxD3D8VertexBufferManagerDestroyNoFVF(stride, stride * numVertices,
                                                    vertexBuffer, baseIndex);
        }
        else
        {
            _rxD3D8VertexBufferManagerDestroy(fvf, stride * numVertices,
                                                vertexBuffer, baseIndex);
        }
    }

    RWRETURNVOID();
}

/****************************************************************************
 TeamD3D8SkinAtomicCreateVertexBuffer

 Purpose:   Just create the vertex buffer
 On entry:
 On exit :
*/
static RwBool
TeamD3D8SkinAtomicCreateVertexBuffer(const RpGeometry *geometry, RxD3D8ResEntryHeader *resEntryHeader)
{
    RpGeometryFlag          flags;
    RwUInt32                stride;
    RwUInt32                fvf;
    RwUInt32                vbSize;
    LPDIRECT3DVERTEXBUFFER8 vertexBuffer;
    RwUInt32                baseIndex;
    RxD3D8InstanceData      *instancedData;
    RwUInt32                numMeshes;
    RwUInt32                numTextureCoords;

    RWFUNCTION(RWSTRING("TeamD3D8SkinAtomicCreateVertexBuffer"));

    flags = (RpGeometryFlag)RpGeometryGetFlags(geometry);

    /*
     * Calculate the stride of the vertex
     */
    /* Positions */
    stride = sizeof(RwV3d);
    fvf = D3DFVF_XYZB3 | D3DFVF_LASTBETA_UBYTE4;

    /* Weights */
    stride += 2 * sizeof(RwReal);

    /* Indices */
    stride += sizeof(RwUInt32);

    /* Normals */
    RWASSERT(flags & rxGEOMETRY_NORMALS);
    stride += sizeof(RwV3d);
    fvf |= D3DFVF_NORMAL;

    /* Texture coordinates */
    numTextureCoords = RpGeometryGetNumTexCoordSets(geometry);
    RWASSERT(numTextureCoords>=1 && numTextureCoords<=2);
    if (numTextureCoords == 1)
    {
        stride += 1 * sizeof(RwTexCoords);
        fvf |= D3DFVF_TEX1;
    }
    else if (numTextureCoords == 2)
    {
        stride += 2 * sizeof(RwTexCoords);
        fvf |= D3DFVF_TEX2;
    }

    /*
     * Create the vertex buffer
     */
    vbSize = stride * geometry->numVertices;

    if (FALSE == _rxD3D8VertexBufferManagerCreateNoFVF(stride,
                                                vbSize,
                                                (void **)&vertexBuffer,
                                                &baseIndex))
    {
        RWRETURN(FALSE);
    }

    /* Get the first instanced data structures */
    instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

    /* Get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;

    do
    {
        instancedData->stride = stride;

        if (RpMatFXMaterialGetEffects(instancedData->material) == rpMATFXEFFECTENVMAP)
        {
            instancedData->vertexShader = _rpTeamD3D8EnvMapSkinVertexShader;
        }
        else if (numTextureCoords == 1)
        {
            instancedData->vertexShader = _rpTeamD3D8SkinVertexShader;
        }
        else if (numTextureCoords == 2)
        {
            instancedData->vertexShader = _rpTeamD3D8DualSkinVertexShader;
        }
        else
        {
            instancedData->vertexShader = fvf;
        }

        if (vertexBuffer == NULL)
        {
            instancedData->managed = FALSE;
        }
        else
        {
            instancedData->managed = TRUE;

            instancedData->vertexBuffer = vertexBuffer;
            instancedData->baseIndex = baseIndex + instancedData->minVert;
        }

        instancedData++;
    }
    while (--numMeshes);

    RWRETURN(TRUE);
}

/****************************************************************************
 TeamD3D8SkinAtomicInstance

 Purpose:   Fill the vertex buffer
 On entry:
 On exit :
*/
static RwBool
TeamD3D8SkinAtomicInstance(const RpAtomic *atomic,
                           RxD3D8ResEntryHeader *resEntryHeader,
                           RwUInt32 numNodes)
{
    const RpGeometry    *geometry;
    RxD3D8InstanceData  *instancedData;
    RwUInt32            stride;
    RwUInt8             *vertexData;

    RWFUNCTION(RWSTRING("TeamD3D8SkinAtomicInstance"));

    geometry = (const RpGeometry *)RpAtomicGetGeometry(atomic);

    /* Create vertex buffer */
    TeamD3D8SkinAtomicCreateVertexBuffer(geometry, resEntryHeader);

    /* Get the first instanced data structures */
    instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

    stride = instancedData->stride;

    /*
     * Fill static information of the vertex buffer
     */
    if (SUCCEEDED(IDirect3DVertexBuffer8_Lock((LPDIRECT3DVERTEXBUFFER8)instancedData->vertexBuffer,
                                             (instancedData->baseIndex - instancedData->minVert) * stride,
                                             geometry->numVertices * stride, &vertexData,
                                             D3DLOCK_NOSYSLOCK)))
    {
        const RpSkin *skin;
        RwUInt32    numTextureCoords;
        RwUInt32    numVertices;
        RwUInt8     *vertexBuffer;
        const RwV3d *pos;
        const RwV3d *normal;
        RwUInt32    offset = 0;

        /* Positions */
        pos = geometry->morphTarget[0].verts;

        vertexBuffer = vertexData;
        numVertices = geometry->numVertices;
        while (numVertices--)
        {
            *((RwV3d *)vertexBuffer) = *pos;
            vertexBuffer += stride;
            pos++;
        }
        offset = sizeof(RwV3d);

        skin = *RPSKINGEOMETRYGETCONSTDATA(geometry);
        RWASSERT(skin != NULL);

        /*
         * Weights
         */
        {
            const RwMatrixWeights *weights;

            weights = skin->vertexMaps.matrixWeights;

            vertexBuffer = vertexData + offset;
            numVertices = geometry->numVertices;
            while (numVertices--)
            {
                ((RwReal *)vertexBuffer)[0] = weights->w0;
                ((RwReal *)vertexBuffer)[1] = 1.f - weights->w0;
                vertexBuffer += stride;
                weights++;
            }

            offset += sizeof(RwReal) * 2;
        }

        /* Indices */
        if (numNodes <= _rpTeamD3D8MaxBonesHardwareSupported)
        {
            const RwUInt32    *indices;

            indices = skin->vertexMaps.matrixIndices;

            vertexBuffer = vertexData + offset;
            numVertices = geometry->numVertices;
            while (numVertices--)
            {
                const RwUInt32 index = *indices;

                ((RwUInt16 *)vertexBuffer)[0] = (RwUInt16)((index & 0xFF) * NUM_ROWS);
                ((RwUInt16 *)vertexBuffer)[1] = (RwUInt16)(((index >> 8) & 0xFF) * NUM_ROWS);
                vertexBuffer += stride;
                indices++;
            }

            offset += sizeof(RwUInt32);
        }
        else
        {
            static RwChar usedToIndex[256];

            const RwUInt32  numUsedBones = skin->boneData.numUsedBones;
            const RwUInt8   *usedBones = skin->boneData.usedBoneList;
            const RwUInt32  *indices;
            RwUInt32        n = 0;

            RWASSERT(numUsedBones <= _rpTeamD3D8MaxBonesHardwareSupported);

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

            vertexBuffer = vertexData + offset;
            numVertices = geometry->numVertices;
            while (numVertices--)
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

            offset += sizeof(RwUInt32);
        }

        /* Normals */
        normal = (const RwV3d *)((const RwUInt8 *)(geometry->morphTarget[0].normals));
        RWASSERT(normal != NULL);

        vertexBuffer = vertexData + offset;
        numVertices = geometry->numVertices;
        while (numVertices--)
        {
            *((RwV3d *)vertexBuffer) = *normal;
            vertexBuffer += stride;
            normal++;
        }
        offset += sizeof(RwV3d);

        /* Get number of texture coordinates */
        numTextureCoords = RpGeometryGetNumTexCoordSets(geometry);
        RWASSERT(numTextureCoords >= 1 && numTextureCoords <= 2);

        /* Texture coordinates */
        if (numTextureCoords)
        {
            RwUInt32    n;

            for (n = 0; n < numTextureCoords; n++)
            {
                const RwTexCoords *texCoord = (const RwTexCoords *)((const RwUInt8 *)(geometry->texCoords[n]));

                vertexBuffer = vertexData + offset;
                numVertices = geometry->numVertices;

    #if defined(NOASM)
                do
                {
                    *((RwTexCoords *)vertexBuffer) = *texCoord;
                    vertexBuffer += stride;
                    texCoord++;
                }
                while (--numVertices);
    #else
                _asm
                {
                    mov esi, texCoord
                    mov edi, vertexBuffer
                    mov edx, stride
                    mov ecx, numVertices
    beginloop:
                    mov eax, dword ptr[esi]
                    mov ebx, dword ptr[esi+4]
                    mov dword ptr[edi], eax
                    mov dword ptr[edi+4], ebx
                    add esi, 8
                    add edi, edx
                    dec ecx
                    jnz beginloop
                }
    #endif

                offset += sizeof(RwTexCoords);
            }
        }

        IDirect3DVertexBuffer8_Unlock((LPDIRECT3DVERTEXBUFFER8)instancedData->vertexBuffer);
    }

    RWRETURN(TRUE);
}

/****************************************************************************
 TeamD3D8SkinInstance

 Purpose:   To instance.

 On entry:

 On exit :
*/
static RwResEntry *
TeamD3D8SkinInstance(const RpAtomic *atomic,
                          void *owner,
                          RwResEntry **resEntryPointer,
                          RpMeshHeader *meshHeader,
                          RwUInt32 numNodes)
{
    RxD3D8ResEntryHeader    *resEntryHeader;
    RxD3D8InstanceData      *instancedData;
    RwResEntry              *resEntry;
    RpMesh                  *mesh;
    RwInt16                 numMeshes;
    RwUInt32                size;

    RWFUNCTION(RWSTRING("TeamD3D8SkinInstance"));

    /*
     * Calculate the amount of memory to allocate
     */

    /* RxD3D8ResEntryHeader, stores serialNumber & numMeshes */
    size = sizeof(RxD3D8ResEntryHeader);

    /* RxD3D8InstanceData structures, one for each mesh */
    size += sizeof(RxD3D8InstanceData) * meshHeader->numMeshes;

    /*
     * Allocate the resource entry
     */
    resEntry = RwResourcesAllocateResEntry(owner,
                                           resEntryPointer,
                                           size,
                                           D3D8TeamSkinDestroyVertexBuffer);
    RWASSERT(NULL != resEntry);

    /* Blank the RxD3D8ResEntryHeader & RxD3D8InstanceData's to '0' */
    memset((resEntry + 1), 0, size);

    /*
     * Initialize the RxD3D8ResEntryHeader
     */
    resEntryHeader = (RxD3D8ResEntryHeader *)(resEntry + 1);

    /* Set the serial number */
    resEntryHeader->serialNumber = meshHeader->serialNum;

    /* Set the number of meshes */
    resEntryHeader->numMeshes = meshHeader->numMeshes;

    /* Get the first RxD3D8InstanceData pointer */
    instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

    mesh = (RpMesh *)(meshHeader + 1);
    numMeshes = meshHeader->numMeshes;
    while (numMeshes--)
    {
        RwUInt32 numIndices = mesh->numIndices;

        /*
         * Number of vertices and Min vertex index,
         * (needed for instancing & reinstancing)
         */
        _rwD3D8MeshGetNumVerticesMinIndex(mesh->indices, numIndices,
                                &instancedData->numVertices,
                                &instancedData->minVert);

        /* Primitive type */
        instancedData->primType = _RwD3D8PrimConvTable[RpMeshHeaderGetPrimType(meshHeader)];

        /* The number of indices */
        instancedData->numIndices = numIndices;

        /* Material */
        instancedData->material = mesh->material;

        /* Vertex shader */
        instancedData->vertexShader = 0;

        /* The vertex format stride */
        instancedData->stride = 0;

        /* Initialize the vertex buffers pointers */
        instancedData->vertexBuffer = NULL;

        /* Initialize vertex buffer*/
        instancedData->baseIndex = 0;

        instancedData->managed = TRUE;

        /* Initialize vertex alpha to FALSE */
        instancedData->vertexAlpha = FALSE;

        /*
         * Set the index buffer
         */

        /* Initialize the index buffers pointers */
        if (RwD3D8IndexBufferCreate(numIndices, &(instancedData->indexBuffer)))
        {
            RxVertexIndex   *indexBuffer;

             if (D3D_OK == IDirect3DIndexBuffer8_Lock((LPDIRECT3DINDEXBUFFER8)instancedData->indexBuffer,
                                                    0, 0, (RwUInt8 **)&indexBuffer, 0))
             {
                if(instancedData->minVert)
                {
                    RxVertexIndex   *indexSrc;

                    indexSrc = mesh->indices;

                    while (numIndices--)
                    {
                        *indexBuffer = (RxVertexIndex)((*indexSrc) - (RxVertexIndex)instancedData->minVert);

                        indexBuffer++;
                        indexSrc++;
                    }
                }
                else
                {
                    memcpy(indexBuffer, mesh->indices, sizeof(RxVertexIndex) * numIndices);
                }

                 IDirect3DIndexBuffer8_Unlock((LPDIRECT3DINDEXBUFFER8)instancedData->indexBuffer);
             }
        }

        instancedData++;
        mesh++;
    }

    /*
     * Call the instance callback
     */
    TeamD3D8SkinAtomicInstance(atomic, resEntryHeader, numNodes);

    RWRETURN(resEntry);
}

/****************************************************************************
 TeamD3D8MatrixMultiplyTranspose3Rows

 On entry   : Dest matrix pointer, two source matrix pointers
 On exit    : Matrix pointer contains transposed result
 */
static void
TeamD3D8MatrixMultiplyTranspose3Rows(_rpTeamD3D8MatrixTransposed3Rows *dstMat,
               const RwMatrix *matA, const RwMatrix *matB)
{
    RWFUNCTION(RWSTRING("TeamD3D8MatrixMultiplyTranspose3Rows"));
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
 TeamD3D8MatrixMultiplyTranspose

 On entry   : Dest matrix pointer, two source matrix pointers
 On exit    : Matrix pointer contains transposed result
 */
static void
TeamD3D8MatrixMultiplyTranspose(_rpTeamD3D8MatrixTransposed *dstMat,
               const D3DMATRIX *matA, const D3DMATRIX *matB)
{
    RWFUNCTION(RWSTRING("TeamD3D8MatrixMultiplyTranspose"));
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
 _rwTeamD3D8SkinMatrixMultiplyTranspose

 On entry   : Dest matrix pointer, two source matrix pointers
 On exit    : Matrix pointer contains transposed result
 */
void
_rwTeamD3D8SkinMatrixMultiplyTranspose(_rpTeamD3D8MatrixTransposed *dstMat,
               const RwMatrix *matA, const _rpTeamD3D8MatrixTransposed *matB)
{
    RWFUNCTION(RWSTRING("_rwTeamD3D8SkinMatrixMultiplyTranspose"));
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
 TeamD3D8EnvMapMatrixMultiplyTranspose

 On entry   : Dest matrix pointer, two source matrix pointers
 On exit    : Matrix pointer contains transposed result
 */
static void
TeamD3D8EnvMapMatrixMultiplyTranspose(_rpTeamD3D8MatrixTransposed *dstMat,
               const D3DMATRIX *matA, const D3DMATRIX *matB)
{
    RWFUNCTION(RWSTRING("TeamD3D8EnvMapMatrixMultiplyTranspose"));
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
 _rpTeamD3D8SetVertexShaderMaterialColor
 */
void
_rpTeamD3D8SetVertexShaderMaterialColor(const RpMaterial *mat)
{
    const RwRGBA *color = &mat->color;
    const RwReal ambScale  = mat->surfaceProps.ambient;
    const RwReal diffScale = mat->surfaceProps.diffuse;

    RwRGBAReal matColor;
    RwRGBAReal modAmbientColor;
    RwRGBAReal modLightColor;

    RWFUNCTION(RWSTRING("_rpTeamD3D8SetVertexShaderMaterialColor"));

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
    RwD3D8SetVertexShaderConstant(VSCONST_REG_AMBIENT_OFFSET,
                                  (const void *)&modAmbientColor,
                                  VSCONST_REG_AMBIENT_SIZE);

    RwD3D8SetVertexShaderConstant(VSCONST_REG_DIR_LIGHT_OFFSET + 1,
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

        _rpTeamD3D8MatrixTransposed result;

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

            TeamD3D8EnvMapMatrixMultiplyTranspose(&result, (const D3DMATRIX *)&invMtx, &texMat);
        }
        else
        {
            TeamD3D8EnvMapMatrixMultiplyTranspose(&result, &_rpTeamD3D8ViewMatrix, &texMat);
        }

        /* pack relevant 2 rows into constant registers */
        RwD3D8SetVertexShaderConstant( VSCONST_REG_ENV_OFFSET,
                                       &result,
                                       VSCONST_REG_ENV_SIZE );
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpTeamD3D8UpdateLights

 Purpose:

 On entry:
 .
 On exit :
*/
static void
TeamD3D8SetDirectionalLight(RpLight *light)
{
    const RwV3d         *at;
    const RwRGBAReal    *color;

    RWFUNCTION(RWSTRING("TeamD3D8SetDirectionalLight"));

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
    _rwD3D8LightDirectionalEnable(light);

    RWRETURNVOID();
}

void
_rpTeamD3D8UpdateLights(const RpTeam *team)
{
    RWFUNCTION(RWSTRING("_rpTeamD3D8UpdateLights"));

    TeamD3D8DirectionalLightFound = FALSE;
    TeamD3D8AmbientLightFound = FALSE;

    AmbientSaturated.red   = 0.0f;
    AmbientSaturated.green = 0.0f;
    AmbientSaturated.blue  = 0.0f;
    AmbientSaturated.alpha = 1.0f;

    if (team->lights[rpTEAMDIRECTIONAL1LIGHT] != NULL)
    {
        TeamD3D8SetDirectionalLight(team->lights[rpTEAMDIRECTIONAL1LIGHT]);

        TeamD3D8DirectionalLightFound = TRUE;
    }

    if (team->lights[rpTEAMAMBIENTLIGHT] != NULL)
    {
        const RwRGBAReal    *color;

        color = RpLightGetColor(team->lights[rpTEAMAMBIENTLIGHT]);

        AmbientSaturated.red   += color->red;
        AmbientSaturated.green += color->green;
        AmbientSaturated.blue  += color->blue;

        TeamD3D8AmbientLightFound = TRUE;
    }

    if (TeamD3D8DirectionalLightFound &&
        TeamD3D8AmbientLightFound)
    {
        _rwD3D8LightsEnable(TRUE, rpATOMIC);

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
                    if (!TeamD3D8DirectionalLightFound)
                    {
                        TeamD3D8SetDirectionalLight(light);

                        TeamD3D8DirectionalLightFound = TRUE;
                    }
                }
                else if (RpLightGetType(light) == rpLIGHTAMBIENT)
                {
                    if (!TeamD3D8AmbientLightFound)
                    {
                        const RwRGBAReal    *color;

                        color = RpLightGetColor(light);

                        AmbientSaturated.red   += color->red;
                        AmbientSaturated.green += color->green;
                        AmbientSaturated.blue  += color->blue;

                        TeamD3D8AmbientLightFound = TRUE;
                    }
                }

                if (TeamD3D8DirectionalLightFound &&
                    TeamD3D8AmbientLightFound)
                {
                    _rwD3D8LightsEnable(TRUE, rpATOMIC);

                    RWRETURNVOID();
                }
            }

            /* Next */
            cur = rwLLLinkGetNext(cur);
        }
    }

    if (!TeamD3D8DirectionalLightFound)
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

    _rwD3D8LightsEnable(FALSE, rpATOMIC);

    RWRETURNVOID();
}

/****************************************************************************
 TeamD3D8ComputeProjViewWorld
 */
static void
TeamD3D8ComputeProjViewWorld(_rpTeamD3D8MatrixTransposed *projViewWorldMatrix)
{
    D3DMATRIX   projMatrix;

    RWFUNCTION(RWSTRING("TeamD3D8ComputeProjViewWorld"));
    RWASSERT( projViewWorldMatrix != NULL );

    /*
     * Projection matrix
     */
    RwD3D8GetTransform(D3DTS_PROJECTION, &projMatrix);

    /*
     * View matrix - (camera matrix)
     */
    RwD3D8GetTransform(D3DTS_VIEW, &_rpTeamD3D8ViewMatrix);

    TeamD3D8MatrixMultiplyTranspose(projViewWorldMatrix,
                                       &_rpTeamD3D8ViewMatrix,
                                       &projMatrix);

    RWRETURNVOID();
}

static void
TeamD3D8SetVertexShaderConstants(const RpAtomic *atomic,
                                 const RpSkin *skin,
                                 RwUInt32 numBones)
{
    RWFUNCTION(RWSTRING("TeamD3D8SetVertexShaderConstants"));
    RWASSERT( atomic != NULL );

    /* Calculate world constants */
    if (_rpTeamD3D8LastRenderFrame != RWSRCGLOBAL(renderFrame))
    {
        TeamD3D8ComputeProjViewWorld(&_rpTeamD3D8projViewMatrix);

        _rpTeamD3D8LastRenderFrame = RWSRCGLOBAL(renderFrame);
    }

    /* Set light direction */
    RwD3D8SetVertexShaderConstant(VSCONST_REG_DIR_LIGHT_OFFSET,
                                  (const void *)&LightDirection,
                                  1);

    if (!_rpTeamD3D8UsingLocalSpace)
    {
        const RwV4d *matrixArray;

        /*
         * Set the constant registers with the combined
         * camera & projection matrix
         */
        RwD3D8SetVertexShaderConstant(VSCONST_REG_TRANSFORM_OFFSET,
                                      (const void *)&_rpTeamD3D8projViewMatrix,
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

                RwD3D8SetVertexShaderConstant(VSCONST_REG_MATRIX_OFFSET + usedBones[baseIndex] * NUM_ROWS,
                                              (const void *)(matrixArray + usedBones[baseIndex] * NUM_ROWS),
                                              (n - baseIndex) * NUM_ROWS);
            }
            while(n < numUsedBones);
        }
        else
        {
            RwD3D8SetVertexShaderConstant(VSCONST_REG_MATRIX_OFFSET,
                                          (const void *)matrixArray,
                                          numBones * NUM_ROWS);
        }
    }
    else
    {
        RwFrame     *frame = RpAtomicGetFrame(atomic);
        const RwMatrix  *ltm = RwFrameGetLTM(frame);
        _rpTeamD3D8MatrixTransposed projViewWorldMatrix;

        _rwTeamD3D8SkinMatrixMultiplyTranspose(
                                        &projViewWorldMatrix,
                                        ltm,
                                        &_rpTeamD3D8projViewMatrix);

        /*
         * Set the constant registers with the combined
         * camera & projection matrix
         */
        RwD3D8SetVertexShaderConstant(VSCONST_REG_TRANSFORM_OFFSET,
                                      (const void *)&projViewWorldMatrix,
                                      VSCONST_REG_TRANSFORM_SIZE);

        if (!_rpTeamD3D8UsingTransposedMatrix)
        {
            const RwUInt32 numUsedBones = skin->boneData.numUsedBones;
            const RwUInt8 *usedBones = skin->boneData.usedBoneList;
            const RwMatrix  *sourceMatrix =
                        (const RwMatrix *)_rpTeamSkinGetCurrentMatrixCache();
            RwUInt32 n;

            if (_rpTeamD3D8MaxTransposedMatrix < numUsedBones)
            {
                _rpTeamD3D8MaxTransposedMatrix = numUsedBones;

                if (_rpTeamD3D8TransposedMatrix == NULL)
                {
                    _rpTeamD3D8TransposedMatrix =
                    (_rpTeamD3D8MatrixTransposed3Rows *)
                    RwMalloc(numUsedBones * sizeof(_rpTeamD3D8MatrixTransposed3Rows),
                             rwID_TEAMPLUGIN | rwMEMHINTDUR_EVENT |
                             rwMEMHINTFLAG_RESIZABLE);
                }
                else
                {
                    _rpTeamD3D8TransposedMatrix =
                    (_rpTeamD3D8MatrixTransposed3Rows *)
                    RwRealloc(_rpTeamD3D8TransposedMatrix,
                              numUsedBones * sizeof(_rpTeamD3D8MatrixTransposed3Rows),
                              rwID_TEAMPLUGIN | rwMEMHINTDUR_EVENT |
                              rwMEMHINTFLAG_RESIZABLE);
                }
            }

            for (n = 0; n < numUsedBones; n++)
            {
                const RwMatrix  *currentMatrix = &(sourceMatrix[usedBones[n]]);
                _rpTeamD3D8MatrixTransposed3Rows *transposedMatrix =
                                                &(_rpTeamD3D8TransposedMatrix[n]);

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

            RwD3D8SetVertexShaderConstant(VSCONST_REG_MATRIX_OFFSET,
                                          (const void *)_rpTeamD3D8TransposedMatrix,
                                          numUsedBones * NUM_ROWS);
        }
        else
        {
            const RwUInt32 numUsedBones = skin->boneData.numUsedBones;
            const RwUInt8 *usedBones = skin->boneData.usedBoneList;
            const _rpTeamD3D8MatrixTransposed   *sourceMatrix =
                    (const _rpTeamD3D8MatrixTransposed *)_rpTeamSkinGetCurrentMatrixCache();
            RwUInt32 n;

            for (n = 0; n < numUsedBones; n++)
            {
                RwD3D8SetVertexShaderConstant(VSCONST_REG_MATRIX_OFFSET + n * NUM_ROWS,
                                              (const void *)&(sourceMatrix[usedBones[n]]),
                                              NUM_ROWS);
            }
        }
    }

    RWRETURNVOID();
}

RwBool
_rxTeamD3D8SkinnedAtomicAllInOneNode(RxPipelineNodeInstance *self,
                        const RxPipelineNodeParam *params)
{
    RpAtomic            *atomic;
    RpHAnimHierarchy    *hierarchy;
    RpGeometry          *geometry;
    RwBool              retVal;

    RWFUNCTION(RWSTRING("_rxTeamD3D8SkinnedAtomicAllInOneNode"));
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

        _rpTeamD3D8UsingVertexShader = (skin != NULL &&
                                        skin->boneData.numUsedBones <=
                                        _rpTeamD3D8MaxBonesHardwareSupported);

        _rpTeamD3D8UsingLocalSpace = ((RwUInt32)hierarchy->numNodes >
                                      _rpTeamD3D8MaxBonesHardwareSupported);
    }
    else
    {
        _rpTeamD3D8UsingVertexShader = FALSE;
        _rpTeamD3D8UsingLocalSpace = TRUE;
    }

    if (_rpTeamD3D8UsingVertexShader)
    {
        if( _rpTeamShadowGetCurrentShadowData()->playerClip != 0 )
        {
            _rxD3D8SkinInstanceNodeData *privateData;
            RpMeshHeader    *meshHeader;
            RwUInt32        geomFlags;
            RwResEntry      *resEntry;

            /*
             * Use vertex shader
             */

            privateData = (_rxD3D8SkinInstanceNodeData *)self->privateData;

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
                RxD3D8ResEntryHeader    *resEntryHeader;

                resEntryHeader = (RxD3D8ResEntryHeader *)(resEntry + 1);
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
                resEntry = TeamD3D8SkinInstance(atomic, owner, resEntryPointer,
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
                RxD3D8ResEntryHeader    *resEntryHeader;
                RxD3D8InstanceData      *instancedData;
                RwInt32                 numMeshes;

                resEntryHeader = (RxD3D8ResEntryHeader *)(resEntry + 1);
                instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

                numMeshes = resEntryHeader->numMeshes;
                while (numMeshes--)
                {
                    if (RpMatFXMaterialGetEffects(instancedData->material) == rpMATFXEFFECTENVMAP)
                    {
                        instancedData->vertexShader = _rpTeamD3D8EnvMapSkinVertexShader;
                    }
                    else if (RpGeometryGetNumTexCoordSets(geometry) == 2)
                    {
                        instancedData->vertexShader = _rpTeamD3D8DualSkinVertexShader;
                    }
                    else
                    {
                        instancedData->vertexShader = _rpTeamD3D8SkinVertexShader;
                    }

                    instancedData++;
                }

                /* We have a resEntry so use it */
                RwResourcesUseResEntry(resEntry);
            }

            /*
             * Set vertex shader constants
             */
            TeamD3D8SetVertexShaderConstants(atomic,
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
        retVal = _rwSkinD3D8AtomicAllInOneNode(self, params);

        /* restore global matrix cache */
        _rpSkinGlobals.matrixCache.aligned = skinAlignedMatrixArray;
    }

    RWRETURN(retVal);
}

/****************************************************************************
 TeamD3D8SkinnedRenderCallback
 */
static void
TeamD3D8SkinnedRenderCallback(RwResEntry *repEntry,
                              void *object,
                              RwUInt8 type,
                              RwUInt32 flags)
{
    RWFUNCTION(RWSTRING("TeamD3D8SkinnedRenderCallback"));

    /* Check if player is visible */
    if( _rpTeamShadowGetCurrentShadowData()->playerClip != 0 )
    {
        RxD3D8ResEntryHeader    *resEntryHeader;
        RxD3D8InstanceData      *instancedData;
        RwBool                  lighting;
        RwBool                  forceBlack;
        RwBool                  vertexAlphaBlend;
        void                    *lastVertexBuffer;
        RwInt32                 numMeshes;
        RwTexture               *baseTexture;
        RwRGBA                  color;

        /* Enable clipping if needed */
        RwD3D8SetRenderState(D3DRS_CLIPPING, (_rpTeamShadowGetCurrentShadowData()->playerClip & 0x2) != 0);

        /* Get the instanced data */
        resEntryHeader = (RxD3D8ResEntryHeader *)(repEntry + 1);
        instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

        /* Get lighting state */
        RwD3D8GetRenderState(D3DRS_LIGHTING, &lighting);

        if (lighting ||
            (flags & rxGEOMETRY_PRELIT) != 0 ||
            (instancedData->vertexShader & D3DFVF_RESERVED0) != 0)
        {
            forceBlack = FALSE;

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
            forceBlack = TRUE;

            RwD3D8SetTexture(NULL, 0);

            RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, 0xff000000);

            RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
            RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
        }

        /* Get vertex alpha Blend state */
        vertexAlphaBlend = _rwD3D8RenderStateIsVertexAlphaEnable();

        /* Set Last vertex buffer to force the call */
        lastVertexBuffer = (void *)0xffffffff;

        /*
         * Set the default Pixel shader
         */
        RwD3D8SetPixelShader(0);

        /*
         * Vertex shader
         */
        RwD3D8SetVertexShader(instancedData->vertexShader);

        /* Get the number of meshes */
        numMeshes = resEntryHeader->numMeshes;
        while (numMeshes--)
        {
            color = *RpTeamMaterialGetPlayerColor(instancedData->material,
                                                  _rpTeamPlayerGetCurrentPlayer());

            if ( (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2)) )
            {
                baseTexture = RpTeamMaterialGetPlayerTexture(instancedData->material,
                                                   _rpTeamPlayerGetCurrentPlayer());

                RwD3D8SetTexture(baseTexture, 0);

                if (forceBlack)
                {
                    /* Only change the colorop, we need to use the texture alpha channel */
                    RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
                    RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
                }
            }

            if (instancedData->vertexAlpha ||
                (0xFF != color.alpha))
            {
                if (!vertexAlphaBlend)
                {
                    vertexAlphaBlend = TRUE;

                    _rwD3D8RenderStateVertexAlphaEnable(TRUE);
                }
            }
            else
            {
                if (vertexAlphaBlend)
                {
                    vertexAlphaBlend = FALSE;

                    _rwD3D8RenderStateVertexAlphaEnable(FALSE);
                }
            }

            /* Set the stream source */
            if (lastVertexBuffer != instancedData->vertexBuffer)
            {
                RwD3D8SetStreamSource(0, instancedData->vertexBuffer, instancedData->stride);

                lastVertexBuffer = instancedData->vertexBuffer;
            }

            /* Check if using a vertex shader */
            if (!_rpTeamD3D8UsingVertexShader)
            {
                if (lighting)
                {
                    if (instancedData->vertexAlpha)
                    {
                        RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
                    }
                    else
                    {
                        RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
                    }

                    RwD3D8SetSurfaceProperties(&color,
                                                &instancedData->material->surfaceProps,
                                                (flags & rxGEOMETRY_MODULATE));
                }
            }
            else
            {
                RwRGBA  oldColor;

                oldColor = instancedData->material->color;

                instancedData->material->color = color;

                _rpTeamD3D8SetVertexShaderMaterialColor(instancedData->material);

                instancedData->material->color = oldColor;
            }

            /*
             * Render
             */

            /* Set the Index buffer */
            RwD3D8SetIndices(instancedData->indexBuffer, instancedData->baseIndex);

            /* Draw the indexed primitive */
            RwD3D8DrawIndexedPrimitive((D3DPRIMITIVETYPE)instancedData->primType,
                                       0, instancedData->numVertices,
                                       0, instancedData->numIndices);

            /* Move onto the next instancedData */
            instancedData++;
        }

        if (forceBlack)
        {
            RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
            RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        }
    }

    /* Render the shadows */
    #if defined(TEAMSHADOWALLINONE)
    if (!_rpTeamD3D8UsingVertexShader)
    {
        _rwTeamD3D8SkinnedShadowsRenderCallback(repEntry, object, type, flags);
    }
    #endif

    RWRETURNVOID();
}

/****************************************************************************
 _rxTeamD3D8AtomicLightingCallback
 */
void
_rxTeamD3D8AtomicLightingCallback(void *object __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("_rxTeamD3D8AtomicLightingCallback"));

    if (TeamD3D8AmbientLightFound)
    {
        RwD3D8SetRenderState(D3DRS_AMBIENT, 0xffffffff);

        RwD3D8SetRenderState(D3DRS_LIGHTING, TRUE);
    }
    else
    {
        RwD3D8SetRenderState(D3DRS_AMBIENT, 0);

        if (TeamD3D8DirectionalLightFound)
        {
            RwD3D8SetRenderState(D3DRS_LIGHTING, TRUE);
        }
        else
        {
            RwD3D8SetRenderState(D3DRS_LIGHTING, FALSE);
        }
    }

    RWRETURNVOID();
}

/****************************************************************************
 TeamSkinD3D8AtomicAllInOnePipelineInit
 */
static RwBool
TeamSkinD3D8AtomicAllInOnePipelineInit(RxPipelineNode *node)
{
    _rxD3D8SkinInstanceNodeData *instanceData;

    RWFUNCTION(RWSTRING("TeamSkinD3D8AtomicAllInOnePipelineInit"));

    instanceData = (_rxD3D8SkinInstanceNodeData *)node->privateData;

    instanceData->renderCallback = TeamD3D8SkinnedRenderCallback;

    instanceData->lightingCallback = _rxTeamD3D8AtomicLightingCallback;

    RWRETURN(TRUE);
}

/*
 * NodeDefinitionGetD3D8TeamSkinAtomicAllInOne returns a
 * pointer to the \ref RxNodeDefinition associated with
 * the Skin Atomic version of PowerPipe.
 */
static RxNodeDefinition *
NodeDefinitionGetD3D8TeamSkinAtomicAllInOne(void)
{
    static RwChar _TeamSkinAtomicInstance_csl[] = "nodeTeamSkinAtomicAllInOne.csl";

    static RxNodeDefinition nodeD3D8SkinAtomicAllInOneCSL = { /* */
        _TeamSkinAtomicInstance_csl,                    /* Name */
        {                                           /* Nodemethods */
            _rxTeamD3D8SkinnedAtomicAllInOneNode,   /* +-- nodebody */
            NULL,                                   /* +-- nodeinit */
            NULL,                                   /* +-- nodeterm */
            TeamSkinD3D8AtomicAllInOnePipelineInit,  /* +-- pipelinenodeinit */
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
        (RwUInt32)sizeof(_rxD3D8SkinInstanceNodeData),/* PipelineNodePrivateDataSize */
        (RxNodeDefEditable)FALSE,           /* editable */
        0                                           /* inPipes */
    };

    RWFUNCTION(RWSTRING("NodeDefinitionGetD3D8TeamSkinAtomicAllInOne"));

    RWRETURN(&nodeD3D8SkinAtomicAllInOneCSL);
}

static RxPipeline *
TeamD3D8SkinnedPipelineCreate(void)
{
    RxPipeline  *pipe;

    RWFUNCTION(RWSTRING("TeamD3D8SkinnedPipelineCreate"));

    pipe = RxPipelineCreate();

    if (pipe)
    {
        RxLockedPipe    *lpipe;

        pipe->pluginId = rwID_TEAMPLUGIN;
        pipe->pluginData = TEAMD3D8PIPEID_SKINNED;

        if ((lpipe = RxPipelineLock(pipe)) != NULL)
        {
            RxNodeDefinition    *instanceNode;

            /*
             * Get the default instance node definition
             */
            instanceNode = NodeDefinitionGetD3D8TeamSkinAtomicAllInOne();
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
    const D3DCAPS8  *d3dCaps;
    RxPipeline      *pipe;

    RWFUNCTION(RWSTRING("_rpTeamSkinCustomPipelineCreate"));

    /*
     * Check hardware support
     */
    d3dCaps = (const D3DCAPS8 *)RwD3D8GetCaps();
    if ( (d3dCaps->VertexShaderVersion & 0xffff) >= 0x0101 &&
         d3dCaps->MaxVertexShaderConst >= 96 )
    {
        _rpTeamD3D8MaxBonesHardwareSupported = d3dCaps->MaxVertexShaderConst;

        /* substract view & projection matrix */
        /* substract ambient & directional light */
        /* substract material */
        _rpTeamD3D8MaxBonesHardwareSupported -= 4 + 3 + 2;

        _rpTeamD3D8MaxBonesHardwareSupported /= 3;

        /* Create vertex shaders */
        if (RwD3D8CreateVertexShader(SkinVertexShaderDecl,
                                     (const RwUInt32 *)dwSkinshdrVertexShader,
                                     &_rpTeamD3D8SkinVertexShader,
                                     0) == FALSE)
        {
            _rpTeamD3D8MaxBonesHardwareSupported = 0;
        }
        else if (RwD3D8CreateVertexShader(DualSkinVertexShaderDecl,
                                          (const RwUInt32 *)dwDualskinshdrVertexShader,
                                          &_rpTeamD3D8DualSkinVertexShader,
                                          0) == FALSE)
        {
            _rpTeamD3D8MaxBonesHardwareSupported = 0;
        }
        else if (RwD3D8CreateVertexShader(SkinVertexShaderDecl,
                                          (const RwUInt32 *)dwEnvmapskinshdrVertexShader,
                                          &_rpTeamD3D8EnvMapSkinVertexShader,
                                          0) == FALSE)
        {
            _rpTeamD3D8MaxBonesHardwareSupported = 0;
        }
        else if (RwD3D8CreateVertexShader(SkinVertexShaderDecl,
                                          (const RwUInt32 *)dwSkinshdr_unlitVertexShader,
                                          &_rpTeamD3D8UnlitSkinVertexShader,
                                          0) == FALSE)
        {
            _rpTeamD3D8MaxBonesHardwareSupported = 0;
        }
        else if (RwD3D8CreateVertexShader(SkinVertexShaderDecl,
                                          (const RwUInt32 *)dwSkinshdr_unlit_nofogVertexShader,
                                          &_rpTeamD3D8UnlitNoFogSkinVertexShader,
                                          0) == FALSE)
        {
            _rpTeamD3D8MaxBonesHardwareSupported = 0;
        }
    }
    else
    {
        _rpTeamD3D8MaxBonesHardwareSupported = 0;
    }

    /* Create pipeline */
    pipe = TeamD3D8SkinnedPipelineCreate();

    if (pipe)
    {
        TeamSkinedPipelineIndex = _rpTeamPipeAddPipeDefinition(pipe,
                                      (TeamPipeOpen)NULL,
                                      (TeamPipeClose)NULL);

        _rpTeamD3D8SetRenderPipeline(pipe, TEAMD3D8PIPEID_SKINNED);

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

    _rpTeamD3D8SetRenderPipeline(NULL, TEAMD3D8PIPEID_SKINNED);

    /* Destroy aux memory */
    if (_rpTeamD3D8TransposedMatrix != NULL)
    {
        RwFree(_rpTeamD3D8TransposedMatrix);
        _rpTeamD3D8TransposedMatrix = NULL;
    }
    _rpTeamD3D8MaxTransposedMatrix = 0;

    /* Destroy vertex shaders */
    if (_rpTeamD3D8MaxBonesHardwareSupported > 0)
    {
        RwD3D8DeleteVertexShader(_rpTeamD3D8UnlitNoFogSkinVertexShader);
        RwD3D8DeleteVertexShader(_rpTeamD3D8UnlitSkinVertexShader);

        RwD3D8DeleteVertexShader(_rpTeamD3D8SkinVertexShader);
        RwD3D8DeleteVertexShader(_rpTeamD3D8DualSkinVertexShader);
        RwD3D8DeleteVertexShader(_rpTeamD3D8EnvMapSkinVertexShader);

        _rpTeamD3D8MaxBonesHardwareSupported = 0;
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

        if ((RwUInt32)hierarchy->numNodes <= _rpTeamD3D8MaxBonesHardwareSupported)
        {
            _rpTeamD3D8MatrixTransposed3Rows *matrixArrayTransposed =
                               (_rpTeamD3D8MatrixTransposed3Rows *)matrixArray;

            /* Vertex shader is working on world space */
            if(hierarchy->flags & rpHANIMHIERARCHYNOMATRICES)
            {

                for( i = 0; i < hierarchy->numNodes; i++ )
                {
                    const RwMatrix *ltm =
                        RwFrameGetLTM(hierarchy->pNodeInfo[i].pFrame);

                    RWASSERT(NULL != ltm);

                    TeamD3D8MatrixMultiplyTranspose3Rows(
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

                    TeamD3D8MatrixMultiplyTranspose3Rows(
                                                    &matrixArrayTransposed[i],
                                                    &temmatrix,
                                                    localToWorld );
                }
            }
            else
            {
                for( i = 0; i < hierarchy->numNodes; i++)
                {
                    TeamD3D8MatrixMultiplyTranspose3Rows(
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
            _rwD3D8MatrixTransposed *matrixArrayTransposed =
                                        (_rwD3D8MatrixTransposed *)matrixArray;

            _rpTeamD3D8UsingTransposedMatrix = TRUE;

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

                    _rwD3D8MatrixMultiplyTranspose(&matrixArrayTransposed[i],
                                                   &temmatrix,
                                                   &inverseAtomicLTM );
                }
            }
            else if(hierarchy->flags & rpHANIMHIERARCHYLOCALSPACEMATRICES)
            {
                for( i = 0; i < hierarchy->numNodes; i++ )
                {
                    _rwD3D8MatrixMultiplyTranspose( &matrixArrayTransposed[i],
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

                    _rwD3D8MatrixMultiplyTranspose(&matrixArrayTransposed[i],
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
