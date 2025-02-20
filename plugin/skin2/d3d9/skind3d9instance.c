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

#include "skind3d9instance.h"

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
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

int g_index = -1;

void __ClearAddress()
{
	CONTEXT cxt;
	HANDLE thisThread;

	RWFUNCTION(RWSTRING("__ClearAddress"));

	if (g_index != -1)
	{
		thisThread = GetCurrentThread();

		// The only registers we care about are the debug registers
		cxt.ContextFlags = CONTEXT_DEBUG_REGISTERS;

		// Read the register values
		if (!GetThreadContext(thisThread, &cxt))
			RWASSERT(FALSE);

		// Zero out the debug register settings for this breakpoint
		RWASSERT(g_index >= 0 && g_index < 4); // m_index has bogus value
		__SetBits(&cxt.Dr7, g_index*2, 1, 0);

		// Write out the new debug registers
		if (!SetThreadContext(thisThread, &cxt))
			RWASSERT(FALSE);

		g_index = -1;
	}
}

void __SetBits(unsigned long* dw, int lowBit, int bits, int newValue)
{
	int mask;
	
	RWFUNCTION(RWSTRING("__SetBits"));

	mask = (1 << bits) - 1; // e.g. 1 becomes 0001, 2 becomes 0011, 3 becomes 0111

	*dw = (*dw & ~(mask << lowBit)) | (newValue << lowBit);
}

void __SetAddress(void* address, int len, int when)
{
	CONTEXT cxt;
	HANDLE thisThread;

	RWFUNCTION(RWSTRING("__SetAddress"));

	// make sure this breakpoint isn't already set
	RWASSERT(g_index == -1);

	thisThread = GetCurrentThread();
	
	switch (len)
	{
	case 1: len = 0; break;
	case 2: len = 1; break;
	case 4: len = 3; break;
	default: RWASSERT(FALSE); // invalid length
	}

	// The only registers we care about are the debug registers
	cxt.ContextFlags = CONTEXT_DEBUG_REGISTERS;

	// Read the register values
	if (!GetThreadContext(thisThread, &cxt))
		RWASSERT(FALSE);

	// Find an available hardware register
	for (g_index = 0; g_index < 4; ++g_index)
	{
		if ((cxt.Dr7 & (1 << (g_index*2))) == 0)
			break;
	}
	RWASSERT(g_index < 4); // All hardware breakpoint registers are already being used

	switch (g_index)
	{
	case 0: cxt.Dr0 = (DWORD) address; break;
	case 1: cxt.Dr1 = (DWORD) address; break;
	case 2: cxt.Dr2 = (DWORD) address; break;
	case 3: cxt.Dr3 = (DWORD) address; break;
	default: RWASSERT(FALSE); // m_index has bogus value
	}

	__SetBits(&cxt.Dr7, 16 + (g_index*4), 2, when);
	__SetBits(&cxt.Dr7, 18 + (g_index*4), 2, len);
	__SetBits(&cxt.Dr7, g_index*2,        1, 1);

	// Write out the new debug registers
	if (!SetThreadContext(thisThread, &cxt))
		RWASSERT(FALSE);
}

/*===========================================================================*
 *--- Functions -------------------------------------------------------*
 *===========================================================================*/

/****************************************************************************
 _rxD3D9SkinInstance

 Purpose:   To instance.

 On entry:

 On exit :
*/
RwResEntry *
_rxD3D9SkinInstance(const RpAtomic *atomic,
                    void *owner,
                    RwResEntry **resEntryPointer,
                    RpMeshHeader *meshHeader)
{
    RxD3D9ResEntryHeader    *resEntryHeader;
    RxD3D9InstanceData      *instancedData;
    RwResEntry              *resEntry;
    RpMesh                  *mesh;
    RwInt16                 numMeshes, n;
    RwUInt32                size;
    RwUInt32                startIndex;
    RxVertexIndex           *indexBuffer;
    RpGeometry              *geometry;
    RpSkin                  *skin;
    const RpHAnimHierarchy  *hierarchy;
    RwUInt32                geometryLockFlags;

    RWFUNCTION(RWSTRING("_rxD3D9SkinInstance"));

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

	//@{ 20050513 DDonSS : Threadsafe
	// Initialize ResEntry Lock
	CS_RESENTRYHEADER_INITLOCK( resEntryHeader );
	//@} DDonSS

	//. 2006. 7. 28. Nonstopdj
	CS_RESENTRYHEADER_LOCK(resEntryHeader);

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

    /* Calculate total num indices */    
    mesh = (RpMesh *)(meshHeader + 1);
    numMeshes = meshHeader->numMeshes;
    while (numMeshes--)
    {
        resEntryHeader->totalNumIndex += mesh->numIndices;

        ++mesh;
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

    /* Primitive type */
    resEntryHeader->primType = _RwD3D9PrimConvTable[RpMeshHeaderGetPrimType(meshHeader)];

    /* Initialize the vertex buffers pointers */
    for (n = 0; n < RWD3D9_MAX_VERTEX_STREAMS; ++n)
    {
        resEntryHeader->vertexStream[n].vertexBuffer = NULL;
        resEntryHeader->vertexStream[n].offset = 0;
        resEntryHeader->vertexStream[n].stride = 0;
        resEntryHeader->vertexStream[n].managed = FALSE;
        resEntryHeader->vertexStream[n].geometryFlags = 0;
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
        RwUInt32 numIndices = mesh->numIndices;

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

        /* Initialize vertex buffer managed to FALSE */
        instancedData->baseIndex = 0;

        /* Initialize vertex alpha to FALSE */
        instancedData->vertexAlpha = FALSE;

        /*
         * copy indices if needed
         */
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

                    ++indexSrc;
                    ++indexDst;
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

        ++instancedData;
        ++mesh;
    }

    if (indexBuffer != NULL)
    {
        IDirect3DIndexBuffer9_Unlock((LPDIRECT3DINDEXBUFFER9)resEntryHeader->indexBuffer);
    }

    /*
     * Instance
     */
    geometry = RpAtomicGetGeometry(atomic);
    RWASSERT(NULL != geometry);

    hierarchy = RPSKINATOMICGETCONSTDATA(atomic)->hierarchy;

    /* Check if we can use Vertex Shaders */
    skin = *RPSKINGEOMETRYGETDATA(geometry);

    if (skin != NULL)
    {
        if (skin->skinSplitData.meshRLECount != NULL)
        {
            skin->platformData.maxNumBones = skin->skinSplitData.boneLimit;
        }
        else
        {
            skin->platformData.maxNumBones = skin->boneData.numUsedBones;
        }

        if (hierarchy != NULL)
        {
            skin->platformData.useVertexShader =  FALSE; //_rwD3D9SkinUseVertexShader(atomic);
        }
        else
        {
            skin->platformData.useVertexShader = FALSE;
        }
    }

    /* Create vertex buffer */
    resEntryHeader->totalNumVertex = geometry->numVertices;

    _rpD3D9SkinAtomicCreateVertexBuffer(atomic, resEntryHeader, (hierarchy != NULL));

    if (skin != NULL && skin->platformData.useVertexShader == 0)
    {
        RwInt32         idx;


        /* always want to instance positions */
        geometryLockFlags = rpGEOMETRYLOCKVERTICES;

        if ( rpGEOMETRYNORMALS == (RpGeometryGetFlags(geometry) & rpGEOMETRYNORMALS) )
        {
            geometryLockFlags |= rpGEOMETRYLOCKNORMALS;
        }
        if ( rpGEOMETRYPRELIT == (RpGeometryGetFlags(geometry) & rpGEOMETRYPRELIT) )
        {
            geometryLockFlags |= rpGEOMETRYLOCKPRELIGHT;
        }
        for ( idx = 0; idx < RpGeometryGetNumTexCoordSets(geometry); idx += 1 )
        {
            geometryLockFlags |= (rpGEOMETRYLOCKTEXCOORDS1 << idx);
        }

        if (resEntryHeader->vertexStream[0].managed == FALSE)
        {
            geometryLockFlags &= ~(resEntryHeader->vertexStream[0].geometryFlags);
        }
    }
    else
    {
        geometryLockFlags = rpGEOMETRYLOCKALL;
    }

    _rpD3D9SkinGeometryReinstance(atomic, resEntryHeader, hierarchy, geometryLockFlags);

	// 2005.3.31 gemani
	resEntryHeader->isLive = 1;

	//. 2006. 7. 28. Nonstopdj
	//. UNLOCKÃß°¡.
	CS_RESENTRYHEADER_UNLOCK(resEntryHeader);

    RWRETURN(resEntry);
}

/****************************************************************************
 _rpD3D9SkinGeometryReinstance

 Purpose:   Fill the vertex buffer
 On entry:
 On exit :
*/
void
_rpD3D9SkinGeometryReinstance(const RpAtomic *atomic,
                              RxD3D9ResEntryHeader *resEntryHeader,
                              const RpHAnimHierarchy *hierarchy,
                              RwUInt32 lockedSinceLastInst)
{
    const RpGeometry *geometry;
    RpD3D9GeometryUsageFlag usageFlags;
    const RpSkin        *skin;
    RwUInt32            numTextureCoords;
    D3DVERTEXELEMENT9   declaration[18];
    RwUInt32            n, numStreams;
    RwUInt8             *lockedVertexBuffer[RWD3D9_MAX_VERTEX_STREAMS];
    RwUInt32            declarationIndex, stream;

    RWFUNCTION(RWSTRING("_rpD3D9SkinGeometryReinstance"));

    geometry = atomic->geometry;

    usageFlags = RpD3D9GeometryGetUsageFlags(geometry);

    skin = *RPSKINGEOMETRYGETCONSTDATA(geometry);

    /* Get number of texture coordinates */
    numTextureCoords = RpGeometryGetNumTexCoordSets(geometry);

    IDirect3DVertexDeclaration9_GetDeclaration((LPDIRECT3DVERTEXDECLARATION9)resEntryHeader->vertexDeclaration,
                                                declaration,
                                                &n);

    RWASSERT(n < 18);

    numStreams = 0;
    n = 0;
    while (declaration[n].Type != D3DDECLTYPE_UNUSED)
    {
        if (declaration[n].Stream >= numStreams)
        {
            numStreams = declaration[n].Stream + 1;
        }

        ++n;
    }

    RWASSERT(numStreams > 0 && numStreams <= RWD3D9_MAX_VERTEX_STREAMS);

    /*
     * Lock the vertex buffer
     */
    for (n = 0; n < RWD3D9_MAX_VERTEX_STREAMS; ++n)
    {
        lockedVertexBuffer[n] = NULL;
    }

    for (n = 0; n < numStreams; ++n)
    {
        RxD3D9VertexStream *vertexStream = &(resEntryHeader->vertexStream[n]);

        if (rpGEOMETRYLOCKALL == lockedSinceLastInst ||
            (vertexStream->geometryFlags & lockedSinceLastInst) != 0)
        {
            const RwUInt32 vbSize = (vertexStream->stride) * (resEntryHeader->totalNumVertex);

			if(vertexStream->managed ||
				vertexStream->offset != 0)
			{
				//@{ Jaewon 20050401
				if(vertexStream->vertexBuffer)
				//@} Jaewon
				{
				IDirect3DVertexBuffer9_Lock((LPDIRECT3DVERTEXBUFFER9)vertexStream->vertexBuffer,
									vertexStream->offset, vbSize, &(lockedVertexBuffer[n]),
									D3DLOCK_NOSYSLOCK);
				}
			}
			else
			{
				lockedSinceLastInst |= vertexStream->geometryFlags;

				//@{ Jaewon 20050401
				if(vertexStream->vertexBuffer)
				//@} Jaewon
				{
				IDirect3DVertexBuffer9_Lock((LPDIRECT3DVERTEXBUFFER9)vertexStream->vertexBuffer,
											0, 0, &(lockedVertexBuffer[n]),
											D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK);
				}
			}
        }
    }

    /* Positions */
    if (lockedSinceLastInst & rpGEOMETRYLOCKVERTICES)
    {
        const RwV3d *pos;

        declarationIndex = 0;
        while (declaration[declarationIndex].Usage != D3DDECLUSAGE_POSITION ||
            declaration[declarationIndex].UsageIndex != 0)
        {
            ++declarationIndex;
        }

        RWASSERT(declarationIndex < 18);

        stream = declaration[declarationIndex].Stream;

        pos = &geometry->morphTarget[0].verts[0];

        _rpD3D9VertexDeclarationInstV3d(declaration[declarationIndex].Type,
                                        lockedVertexBuffer[stream] +
                                        declaration[declarationIndex].Offset,
                                        pos,
                                        resEntryHeader->totalNumVertex,
                                        resEntryHeader->vertexStream[stream].stride);
    }

    if (skin != NULL &&
        skin->platformData.useVertexShader &&
        lockedSinceLastInst == rpGEOMETRYLOCKALL)
    {
        const RwUInt32 numUsedBones = skin->boneData.numUsedBones;
        const RwUInt8  *usedBones;
        RwUInt8 usedToIndex[0xff];

        /* Weights */
        if (skin->vertexMaps.maxWeights > 1)
        {
            RwMatrixWeights *weights;

            declarationIndex = 0;
            while (declaration[declarationIndex].Usage != D3DDECLUSAGE_BLENDWEIGHT ||
                declaration[declarationIndex].UsageIndex != 0)
            {
                ++declarationIndex;
            }

            stream = declaration[declarationIndex].Stream;

            weights = skin->vertexMaps.matrixWeights;

            /* Fix vertex weights */
            for (n = 0; n < resEntryHeader->totalNumVertex; ++n)
            {
                if (weights[n].w0 > 1.0f)
                {
                    weights[n].w0 = 1.0f;
                    weights[n].w1 = 0.0f;
                    weights[n].w2 = 0.0f;
                    weights[n].w3 = 0.0f;
                }
            }

            _rpD3D9VertexDeclarationInstWeights(declaration[declarationIndex].Type,
                                                lockedVertexBuffer[stream] +
                                                declaration[declarationIndex].Offset,
                                                (const RwV4d *)weights,
                                                resEntryHeader->totalNumVertex,
                                                resEntryHeader->vertexStream[stream].stride);
        }

        /* Indices */
        declarationIndex = 0;
        while (declaration[declarationIndex].Usage != D3DDECLUSAGE_BLENDINDICES ||
            declaration[declarationIndex].UsageIndex != 0)
        {
            ++declarationIndex;
        }

        stream = declaration[declarationIndex].Stream;

        memset(usedToIndex, 0, 0xff * sizeof(RwUInt8));

        if (skin->skinSplitData.meshRLECount != NULL)
        {
            if (skin->skinSplitData.matrixRemapIndices[0] >= hierarchy->numNodes)
            {
                skin->skinSplitData.matrixRemapIndices[0] = 0;
            }

            usedBones = skin->skinSplitData.matrixRemapIndices;

            for (n = 0; n < (RwUInt32)hierarchy->numNodes; n++)
            {
                if (usedBones[n] < 0xff)
                {
                    usedToIndex[n] = usedBones[n] * 3;
                }
            }
        }
        else
        {
            if ((RwUInt32)hierarchy->numNodes <= _rpSkinGlobals.platform.maxNumBones)
            {
                for (n = 0; n < (RwUInt32)hierarchy->numNodes; n++)
                {
                    usedToIndex[n] = n * 3;
                }
            }
            else
            {
                RWASSERT(numUsedBones <= _rpSkinGlobals.platform.maxNumBones);

                usedToIndex[0] = 0; /* Used for 1 weight vertex */

                usedBones = skin->boneData.usedBoneList;

                for (n = 0; n < numUsedBones; ++n)
                {
                    usedToIndex[usedBones[n]] = n * 3;
                }
            }
        }

        _rpD3D9VertexDeclarationInstIndicesRemap(declaration[declarationIndex].Type,
                                            lockedVertexBuffer[stream] +
                                            declaration[declarationIndex].Offset,
                                            skin->vertexMaps.matrixIndices,
                                            usedToIndex,
                                            resEntryHeader->totalNumVertex,
                                            resEntryHeader->vertexStream[stream].stride);
    }

    /* Normals */
    if ((rxGEOMETRY_NORMALS & RpGeometryGetFlags(geometry)) != 0)
    {
        if (lockedSinceLastInst & rpGEOMETRYLOCKNORMALS)
        {
            const RwV3d *normal;

            declarationIndex = 0;
            while (declaration[declarationIndex].Usage != D3DDECLUSAGE_NORMAL ||
                declaration[declarationIndex].UsageIndex != 0)
            {
                ++declarationIndex;
            }

            stream = declaration[declarationIndex].Stream;

            normal = &geometry->morphTarget[0].normals[0];

            _rpD3D9VertexDeclarationInstV3d(declaration[declarationIndex].Type,
                                            lockedVertexBuffer[stream] +
                                            declaration[declarationIndex].Offset,
                                            normal,
                                            resEntryHeader->totalNumVertex,
                                            resEntryHeader->vertexStream[stream].stride);
        }
    }

    if (usageFlags & rpD3D9GEOMETRYUSAGE_CREATETANGENTS)
    {
        if (lockedSinceLastInst & (rpGEOMETRYLOCKVERTICES | rpGEOMETRYLOCKTEXCOORDSALL))
        {
            RwUInt32 offset, stride;
            const RwV3d *pos;
            const RwV3d *normal;
            const RwTexCoords *texCoord;

            /* Find tangents */
            declarationIndex = 0;
            while (declaration[declarationIndex].Usage != D3DDECLUSAGE_TANGENT ||
                declaration[declarationIndex].UsageIndex != 0)
            {
                ++declarationIndex;
            }

            RWASSERT(declarationIndex < 18);

            stream = declaration[declarationIndex].Stream;
            offset = declaration[declarationIndex].Offset;
            stride = resEntryHeader->vertexStream[stream].stride;

            pos = geometry->morphTarget[0].verts;

            normal = geometry->morphTarget[0].normals;

            texCoord = (const RwTexCoords *)(geometry->texCoords[numTextureCoords - 1]);

            stream = declaration[declarationIndex].Stream;

			/* 2006. 5. 18. nonstopdj*/
			if(texCoord)
            _rpD3D9VertexDeclarationInstTangent(declaration[declarationIndex].Type,
                                                lockedVertexBuffer[stream] +
                                                offset,
                                                pos,
                                                normal,
                                                NULL,
                                                texCoord,
                                                resEntryHeader,
                                                stride
												//@{ Jaewon 20050330
												,
												geometry->mesh
												//@} Jaewon
												);
        }

        ++declarationIndex;
    }

    /* Texture coordinates */
    if (numTextureCoords &&
        (lockedSinceLastInst & rpGEOMETRYLOCKTEXCOORDSALL) != 0)
    {
        const RwV2d *texCoord;
        RwUInt32    i;

        for (i = 0; i < numTextureCoords; ++i)
        {
            if (lockedSinceLastInst & (rpGEOMETRYLOCKTEXCOORDS1 << i))
            {
                declarationIndex = 0;
                while (declaration[declarationIndex].Usage != D3DDECLUSAGE_TEXCOORD ||
                    declaration[declarationIndex].UsageIndex != i)
                {
                    ++declarationIndex;
                }

                RWASSERT(declarationIndex < 18);

                stream = declaration[declarationIndex].Stream;

                texCoord = (const RwV2d *)(((const RwUInt8 *)(&geometry->texCoords[i][0])));

                _rpD3D9VertexDeclarationInstV2d(declaration[declarationIndex].Type,
                                                lockedVertexBuffer[stream] +
                                                declaration[declarationIndex].Offset,
                                                texCoord,
                                                resEntryHeader->totalNumVertex,
                                                resEntryHeader->vertexStream[stream].stride);
            }
        }
    }

    /*
     * PreLight need to be done per mesh
     */
    if ((rpGEOMETRYPRELIT & RpGeometryGetFlags(geometry)) != 0 &&
        (lockedSinceLastInst & rpGEOMETRYLOCKPRELIGHT) != 0)
    {
        RwUInt32 numMeshes, offset, stride;
        RxD3D9InstanceData  *instancedData;

        /* Find prelit */
        declarationIndex = 0;
        while (declaration[declarationIndex].Usage != D3DDECLUSAGE_COLOR ||
               declaration[declarationIndex].UsageIndex != 0)
        {
            ++declarationIndex;
        }

        RWASSERT(declarationIndex < 18);

        stream = declaration[declarationIndex].Stream;
        offset = declaration[declarationIndex].Offset;
        stride = resEntryHeader->vertexStream[stream].stride;

        /* Get the number of meshes */
        numMeshes = resEntryHeader->numMeshes;

        /* Get the first instanced data structures */
        instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

        do
        {
            const RwRGBA    *color;

            color = (const RwRGBA *)(((const RwUInt8 *)(&geometry->preLitLum[instancedData->minVert])));

            instancedData->vertexAlpha =
            _rpD3D9VertexDeclarationInstColor(lockedVertexBuffer[stream] +
                                              offset +
                                              ((instancedData->minVert) * stride),
                                              color,
                                              instancedData->numVertices,
                                              stride);

            ++instancedData;
        }
        while (--numMeshes);
    }

    /*
     * Unlock the vertex buffer
     */
    for (n = 0; n < numStreams; ++n)
    {
        if (lockedVertexBuffer[n] != NULL)
        {
			//@{ Jaewon 20050330
			if(resEntryHeader->vertexStream[n].vertexBuffer)
			//@} Jaewon
			{
	            IDirect3DVertexBuffer9_Unlock((LPDIRECT3DVERTEXBUFFER9)(resEntryHeader->vertexStream[n].vertexBuffer));
			}
        }
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rpD3D9SkinAtomicCreateVertexBuffer

 Purpose:   Just create the vertex buffer
 On entry:
 On exit :
*/
RwBool
_rpD3D9SkinAtomicCreateVertexBuffer(const RpAtomic *atomic,
                                    RxD3D9ResEntryHeader *resEntryHeader,
                                    RwBool hasHierarchy)
{
    const D3DCAPS9      *d3dCaps;
    D3DVERTEXELEMENT9   declaration[18];
    const RpGeometry    *geometry;
    const RpSkin        *skin;
    RpGeometryFlag      flags;
    RpD3D9GeometryUsageFlag usageFlags;
    RwUInt32            numTextureCoords;
    RxD3D9VertexStream  *vertexStream;
    RwUInt32            numStreams, stream;
    RwUInt32            declarationIndex;
    RwUInt32            vbSize;
    RwUInt32            n, numMeshes;
    RxD3D9InstanceData      *instancedData;

    RWFUNCTION(RWSTRING("_rpD3D9SkinAtomicCreateVertexBuffer"));

    d3dCaps = (const D3DCAPS9 *)RwD3D9GetCaps();

    geometry = (const RpGeometry *)RpAtomicGetGeometry(atomic);

    skin = *RPSKINGEOMETRYGETCONSTDATA(geometry);

    flags = (RpGeometryFlag)RpGeometryGetFlags(geometry);

    usageFlags = RpD3D9GeometryGetUsageFlags(geometry);

    /* Get number of texture coordinates */
    numTextureCoords = RpGeometryGetNumTexCoordSets(geometry);

    /* Destroy previous vertex buffers */
    for (n = 0; n < RWD3D9_MAX_VERTEX_STREAMS; ++n)
    {
        vertexStream = &(resEntryHeader->vertexStream[n]);

        if(vertexStream->vertexBuffer != NULL)
        {
            if (resEntryHeader->vertexStream[n].managed)
            {
                RwD3D9DestroyVertexBuffer(vertexStream->stride,
                                          vertexStream->stride * (resEntryHeader->totalNumVertex),
                                          vertexStream->vertexBuffer,
                                          vertexStream->offset);
            }
            else
            {
                if (vertexStream->offset == 0 &&
                    vertexStream->dynamicLock == FALSE)
                {
                    RwD3D9DynamicVertexBufferDestroy(vertexStream->vertexBuffer);
                }
            }

            vertexStream->vertexBuffer = NULL;
        }

        vertexStream->offset = 0;
        vertexStream->stride = 0;
        vertexStream->geometryFlags = 0;
        vertexStream->managed = FALSE;
        vertexStream->dynamicLock = FALSE;
    }

    /*
     * Calculate the stride of the vertex
     */
    vertexStream = resEntryHeader->vertexStream;

    if ((usageFlags & rpD3D9GEOMETRYUSAGE_DYNAMICMASK) == 0 &&
        (skin == NULL ||
         skin->platformData.useVertexShader ||
         hasHierarchy == FALSE ||
         _rwD3D9SkinNeedsAManagedVertexBuffer(atomic, resEntryHeader)))
    {
        numStreams = 1;
       
        vertexStream[0].managed = TRUE;
    }
    else
    {
        numStreams = 2;

        vertexStream[0].managed = FALSE;
        vertexStream[1].managed = TRUE;

        _rpD3D9AddDynamicGeometry((RpGeometry *)geometry);
    }

    declarationIndex = 0;

    if (skin != NULL &&
        skin->platformData.useVertexShader)
    {
        /* Positions */
        if (usageFlags & rpD3D9GEOMETRYUSAGE_DYNAMICPOSITIONS)
        {
            stream = 0;
        }
        else
        {
            stream = (numStreams - 1);
        }

        declaration[declarationIndex].Stream = stream;
        declaration[declarationIndex].Offset = vertexStream[stream].stride;
        declaration[declarationIndex].Type = D3DDECLTYPE_FLOAT3;
        declaration[declarationIndex].Method = D3DDECLMETHOD_DEFAULT;
        declaration[declarationIndex].Usage = D3DDECLUSAGE_POSITION;
        declaration[declarationIndex].UsageIndex = 0;
        ++declarationIndex;
        vertexStream[stream].stride = sizeof(RwV3d);
        vertexStream[stream].geometryFlags = rpGEOMETRYLOCKVERTICES;

        /* Normals */
        if (flags & rxGEOMETRY_NORMALS)
        {
            if (usageFlags & rpD3D9GEOMETRYUSAGE_DYNAMICNORMALS)
            {
                stream = 0;
            }
            else
            {
                stream = (numStreams - 1);
            }

            declaration[declarationIndex].Stream = stream;
            declaration[declarationIndex].Offset = vertexStream[stream].stride;

            if (d3dCaps->DeclTypes & D3DDTCAPS_DEC3N)
            {
                declaration[declarationIndex].Type = D3DDECLTYPE_DEC3N;
                vertexStream[stream].stride += sizeof(RwUInt32);
            }
            else
            {
                declaration[declarationIndex].Type = D3DDECLTYPE_FLOAT3;
                vertexStream[stream].stride += sizeof(RwV3d);
            }

            declaration[declarationIndex].Method = D3DDECLMETHOD_DEFAULT;
            declaration[declarationIndex].Usage = D3DDECLUSAGE_NORMAL;
            declaration[declarationIndex].UsageIndex = 0;
            declarationIndex++;
            vertexStream[stream].geometryFlags |= rpGEOMETRYLOCKNORMALS;
        }

        /* Tangents */
        if (usageFlags & rpD3D9GEOMETRYUSAGE_CREATETANGENTS)
        {
            if (usageFlags & (rpD3D9GEOMETRYUSAGE_DYNAMICPOSITIONS | rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDSALL))
            {
                stream = 0;
            }
            else
            {
                stream = (numStreams - 1);
            }

            declaration[declarationIndex].Stream = stream;
            declaration[declarationIndex].Offset = vertexStream[stream].stride;

            if (d3dCaps->DeclTypes & D3DDTCAPS_DEC3N)
            {
                declaration[declarationIndex].Type = D3DDECLTYPE_DEC3N;
                vertexStream[stream].stride += sizeof(RwUInt32);
            }
            else
            {
                declaration[declarationIndex].Type = D3DDECLTYPE_FLOAT3;
                vertexStream[stream].stride += sizeof(RwV3d);
            }

            declaration[declarationIndex].Method = D3DDECLMETHOD_DEFAULT;
            declaration[declarationIndex].Usage = D3DDECLUSAGE_TANGENT;
            declaration[declarationIndex].UsageIndex = 0;
            ++declarationIndex;
        }

        /* Pre-lighting */
        if (flags & rxGEOMETRY_PRELIT)
        {
            if (usageFlags & rpD3D9GEOMETRYUSAGE_DYNAMICPRELIT)
            {
                stream = 0;
            }
            else
            {
                stream = (numStreams - 1);
            }

            declaration[declarationIndex].Stream = stream;
            declaration[declarationIndex].Offset = vertexStream[stream].stride;
            declaration[declarationIndex].Type = D3DDECLTYPE_D3DCOLOR;
            declaration[declarationIndex].Method = D3DDECLMETHOD_DEFAULT;
            declaration[declarationIndex].Usage = D3DDECLUSAGE_COLOR;
            declaration[declarationIndex].UsageIndex = 0;
            ++declarationIndex;
            vertexStream[stream].stride += sizeof(RwRGBA);
            vertexStream[stream].geometryFlags |= rpGEOMETRYLOCKPRELIGHT;
        }

        /* Texture coordinates */
        for (n = 0; n < numTextureCoords; ++n)
        {
            if (usageFlags & (rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDS1 << n))
            {
                stream = 0;
            }
            else
            {
                stream = (numStreams - 1);
            }

            declaration[declarationIndex].Stream = stream;
            declaration[declarationIndex].Offset = vertexStream[stream].stride;

            /* Compress texture coordinates if possible */
            declaration[declarationIndex].Type =
                _rpD3D9FindFormatV2d((const RwV2d *)(((const RwUInt8 *)(&geometry->texCoords[n][0]))),
                                     resEntryHeader->totalNumVertex);

            declaration[declarationIndex].Method = D3DDECLMETHOD_DEFAULT;
            declaration[declarationIndex].Usage = D3DDECLUSAGE_TEXCOORD;
            declaration[declarationIndex].UsageIndex = n;

            vertexStream[stream].stride +=
                _rpD3D9VertexDeclarationGetSize(declaration[declarationIndex].Type);

            vertexStream[stream].geometryFlags |= (rpGEOMETRYLOCKTEXCOORDS1 << n);

            ++declarationIndex;
        }

        /* Weights and indices are static info */
        stream = (numStreams - 1);

        /* Weights */
        if (skin->vertexMaps.maxWeights > 1)
        {
            declaration[declarationIndex].Stream = stream;
            declaration[declarationIndex].Offset = vertexStream[stream].stride;

            if (skin->vertexMaps.maxWeights >= 4)
            {
                if (d3dCaps->DeclTypes & D3DDTCAPS_USHORT4N)
                {
                    declaration[declarationIndex].Type = D3DDECLTYPE_USHORT4N;
                    vertexStream[stream].stride += 4 * sizeof(RwUInt16);
                }
                else if (d3dCaps->DeclTypes & D3DDTCAPS_SHORT4N)
                {
                    declaration[declarationIndex].Type = D3DDECLTYPE_SHORT4N;
                    vertexStream[stream].stride += 4 * sizeof(RwUInt16);
                }
                else if (d3dCaps->DeclTypes & D3DDTCAPS_UBYTE4N)
                {
                    declaration[declarationIndex].Type = D3DDECLTYPE_UBYTE4N;
                    vertexStream[stream].stride += 4 * sizeof(RwUInt8);
                }
                else
                {
                    declaration[declarationIndex].Type = D3DDECLTYPE_D3DCOLOR;
                    vertexStream[stream].stride += sizeof(RwUInt32);
                }
            }
            else if (skin->vertexMaps.maxWeights >= 3)
            {
                if (d3dCaps->DeclTypes & D3DDTCAPS_DEC3N)
                {
                    declaration[declarationIndex].Type = D3DDECLTYPE_DEC3N;
                    vertexStream[stream].stride += sizeof(RwUInt32);
                }
                else if (d3dCaps->DeclTypes & D3DDTCAPS_USHORT4N)
                {
                    declaration[declarationIndex].Type = D3DDECLTYPE_USHORT4N;
                    vertexStream[stream].stride += 4 * sizeof(RwUInt16);
                }
                else if (d3dCaps->DeclTypes & D3DDTCAPS_SHORT4N)
                {
                    declaration[declarationIndex].Type = D3DDECLTYPE_SHORT4N;
                    vertexStream[stream].stride += 4 * sizeof(RwUInt16);
                }
                else if (d3dCaps->DeclTypes & D3DDTCAPS_UBYTE4N)
                {
                    declaration[declarationIndex].Type = D3DDECLTYPE_UBYTE4N;
                    vertexStream[stream].stride += 4 * sizeof(RwUInt8);
                }
                else
                {
                    declaration[declarationIndex].Type = D3DDECLTYPE_D3DCOLOR;
                    vertexStream[stream].stride += sizeof(RwUInt32);
                }
            }
            else
            {
                if (d3dCaps->DeclTypes & D3DDTCAPS_USHORT2N)
                {
                    declaration[declarationIndex].Type = D3DDECLTYPE_USHORT2N;
                    vertexStream[stream].stride += 2 * sizeof(RwUInt16);
                }
                else if (d3dCaps->DeclTypes & D3DDTCAPS_SHORT2N)
                {
                    declaration[declarationIndex].Type = D3DDECLTYPE_SHORT2N;
                    vertexStream[stream].stride += 2 * sizeof(RwUInt16);
                }
                else if (d3dCaps->DeclTypes & D3DDTCAPS_DEC3N)
                {
                    declaration[declarationIndex].Type = D3DDECLTYPE_DEC3N;
                    vertexStream[stream].stride += sizeof(RwUInt32);
                }
                else if (d3dCaps->DeclTypes & D3DDTCAPS_UBYTE4N)
                {
                    declaration[declarationIndex].Type = D3DDECLTYPE_UBYTE4N;
                    vertexStream[stream].stride += 4 * sizeof(RwUInt8);
                }
                else
                {
                    declaration[declarationIndex].Type = D3DDECLTYPE_D3DCOLOR;
                    vertexStream[stream].stride += sizeof(RwUInt32);
                }
            }

            declaration[declarationIndex].Method = D3DDECLMETHOD_DEFAULT;
            declaration[declarationIndex].Usage = D3DDECLUSAGE_BLENDWEIGHT;
            declaration[declarationIndex].UsageIndex = 0;
            ++declarationIndex;
        }

        /* Indices */
        declaration[declarationIndex].Stream = stream;
        declaration[declarationIndex].Offset = vertexStream[stream].stride;

        if (skin->vertexMaps.maxWeights >= 3)
        {
            if (d3dCaps->DeclTypes & D3DDTCAPS_UBYTE4)
            {
                declaration[declarationIndex].Type = D3DDECLTYPE_UBYTE4;
                vertexStream[stream].stride += 4 * sizeof(RwUInt8);
            }
            else
            {
                declaration[declarationIndex].Type = D3DDECLTYPE_SHORT4;
                vertexStream[stream].stride += 4 * sizeof(RwUInt16);
            }
        }
        else
        {
            declaration[declarationIndex].Type = D3DDECLTYPE_SHORT2;
            vertexStream[stream].stride += 2 * sizeof(RwUInt16);
        }

        declaration[declarationIndex].Method = D3DDECLMETHOD_DEFAULT;
        declaration[declarationIndex].Usage = D3DDECLUSAGE_BLENDINDICES;
        declaration[declarationIndex].UsageIndex = 0;
        ++declarationIndex;
    }
    else
    {
        /* Positions */
        stream = 0; /* Always dinamic if possible */

        declaration[declarationIndex].Stream = stream;
        declaration[declarationIndex].Offset = vertexStream[stream].stride;
        declaration[declarationIndex].Type = D3DDECLTYPE_FLOAT3;
        declaration[declarationIndex].Method = D3DDECLMETHOD_DEFAULT;
        declaration[declarationIndex].Usage = D3DDECLUSAGE_POSITION;
        declaration[declarationIndex].UsageIndex = 0;
        ++declarationIndex;
        vertexStream[stream].stride = sizeof(RwV3d);
        vertexStream[stream].geometryFlags = rpGEOMETRYLOCKVERTICES;

        /* Normals */
        if (flags & rxGEOMETRY_NORMALS)
        {
            stream = 0; /* Always dinamic if possible */

            declaration[declarationIndex].Stream = stream;
            declaration[declarationIndex].Offset = vertexStream[stream].stride;
            declaration[declarationIndex].Type = D3DDECLTYPE_FLOAT3;
            declaration[declarationIndex].Method = D3DDECLMETHOD_DEFAULT;
            declaration[declarationIndex].Usage = D3DDECLUSAGE_NORMAL;
            declaration[declarationIndex].UsageIndex = 0;
            ++declarationIndex;
            vertexStream[stream].stride += sizeof(RwV3d);
            vertexStream[stream].geometryFlags |= rpGEOMETRYLOCKNORMALS;
        }

		//@{ Jaewon 20041007
        /* Tangents */
        if (usageFlags & rpD3D9GEOMETRYUSAGE_CREATETANGENTS)
        {
            if (usageFlags & (rpD3D9GEOMETRYUSAGE_DYNAMICPOSITIONS | rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDSALL))
            {
                stream = 0;
            }
            else
            {
                stream = (numStreams - 1);
            }

            declaration[declarationIndex].Stream = stream;
            declaration[declarationIndex].Offset = vertexStream[stream].stride;
            declaration[declarationIndex].Type = D3DDECLTYPE_FLOAT3;
            vertexStream[stream].stride += sizeof(RwV3d);
            declaration[declarationIndex].Method = D3DDECLMETHOD_DEFAULT;
            declaration[declarationIndex].Usage = D3DDECLUSAGE_TANGENT;
            declaration[declarationIndex].UsageIndex = 0;
            ++declarationIndex;
        }
		//@} Jaewon
		
        /* Pre-lighting */
        if (flags & rxGEOMETRY_PRELIT)
        {
            if (usageFlags & rpD3D9GEOMETRYUSAGE_DYNAMICPRELIT)
            {
                stream = 0;
            }
            else
            {
                stream = (numStreams - 1);
            }

            declaration[declarationIndex].Stream = stream;
            declaration[declarationIndex].Offset = vertexStream[stream].stride;
            declaration[declarationIndex].Type = D3DDECLTYPE_D3DCOLOR;
            declaration[declarationIndex].Method = D3DDECLMETHOD_DEFAULT;
            declaration[declarationIndex].Usage = D3DDECLUSAGE_COLOR;
            declaration[declarationIndex].UsageIndex = 0;
            ++declarationIndex;
            vertexStream[stream].stride += sizeof(RwRGBA);
            vertexStream[stream].geometryFlags |= rpGEOMETRYLOCKPRELIGHT;
        }

        /* Texture coordinates */
        for (n = 0; n < numTextureCoords; n++)
        {
            if (usageFlags & (rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDS1 << n))
            {
                stream = 0;
            }
            else
            {
                stream = (numStreams - 1);
            }

            declaration[declarationIndex].Stream = stream;
            declaration[declarationIndex].Offset = vertexStream[stream].stride;
            declaration[declarationIndex].Type = D3DDECLTYPE_FLOAT2;
            declaration[declarationIndex].Method = D3DDECLMETHOD_DEFAULT;
            declaration[declarationIndex].Usage = D3DDECLUSAGE_TEXCOORD;
            declaration[declarationIndex].UsageIndex = n;
            ++declarationIndex;
            vertexStream[stream].stride += sizeof(RwV2d);
            vertexStream[stream].geometryFlags |= (rpGEOMETRYLOCKTEXCOORDS1 << n);
        }
    }

    /* End declaration */
    declaration[declarationIndex].Stream = 0xFF;
    declaration[declarationIndex].Offset = 0;
    declaration[declarationIndex].Type = D3DDECLTYPE_UNUSED;
    declaration[declarationIndex].Method = 0;
    declaration[declarationIndex].Usage = 0;
    declaration[declarationIndex].UsageIndex = 0;

    RwD3D9CreateVertexDeclaration(declaration,
                                  &(resEntryHeader->vertexDeclaration));

    /* remove unused streams */
    while (numStreams > 1 &&
           vertexStream[numStreams - 1].stride == 0)
    {
        --numStreams;
    }

    RWASSERT(numStreams > 0 && numStreams <= RWD3D9_MAX_VERTEX_STREAMS);

    /*
     * Create the vertex buffer
     */
    if (numStreams == 1 &&
        vertexStream[0].managed)
    {
        vbSize = (vertexStream[0].stride) * (resEntryHeader->totalNumVertex);

        if (FALSE == RwD3D9CreateVertexBuffer(vertexStream[0].stride, vbSize,
                                              &(vertexStream[0].vertexBuffer),
                                              &(vertexStream[0].offset)))
        {
            RWRETURN(FALSE);
        }
    }
    else
    {
        for (n = 0; n < numStreams; ++n)
        {
            vertexStream = &(resEntryHeader->vertexStream[n]);

            vbSize = (vertexStream->stride) * (resEntryHeader->totalNumVertex);

            if (vertexStream->managed &&
                (d3dCaps->DevCaps2 & D3DDEVCAPS2_STREAMOFFSET) != 0)
            {
                if (FALSE == RwD3D9CreateVertexBuffer(vertexStream->stride, vbSize,
                                                      &(vertexStream->vertexBuffer),
                                                      &(vertexStream->offset)))
                {
                    RWRETURN(FALSE);
                }
            }
            else
            {
                if ( skin->platformData.useVertexShader == FALSE &&
                     ((d3dCaps->DevCaps2 & D3DDEVCAPS2_STREAMOFFSET) != 0 ||
                      numStreams == 1) )
                {
                    vertexStream->dynamicLock = TRUE;
                }
                else
                {
                    if (FALSE == RwD3D9DynamicVertexBufferCreate(vbSize,
                                                                &(vertexStream->vertexBuffer)))
                    {
                        RWRETURN(FALSE);
                    }
                }

                vertexStream->managed = FALSE;

                vertexStream->offset = 0;
            }
        }
    }

    /* Fix base index */
    if (numStreams > 1)
    {
        if (d3dCaps->DevCaps2 & D3DDEVCAPS2_STREAMOFFSET)
        {
            resEntryHeader->useOffsets = TRUE;
        }

        numMeshes = resEntryHeader->numMeshes;
        instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

        do
        {
            instancedData->baseIndex = instancedData->minVert;

            ++instancedData;
        }
        while (--numMeshes);
    }
    else
    {
        resEntryHeader->useOffsets = FALSE;

        numMeshes = resEntryHeader->numMeshes;
        instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

        do
        {
            instancedData->baseIndex = instancedData->minVert +
                                       (resEntryHeader->vertexStream[0].offset / resEntryHeader->vertexStream[0].stride);

            ++instancedData;
        }
        while (--numMeshes);
    }

    RWRETURN(TRUE);
}
