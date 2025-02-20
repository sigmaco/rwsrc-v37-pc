/*
 * nodeWorldSectorInstance
 * Instancing a world sector |
 *
 * Copyright (c) Criterion Software Limited
 */

/****************************************************************************
 *                                                                          *
 * module : nodeWorldSectorInstance.c                                       *
 *                                                                          *
 * purpose: yawn...                                                         *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 includes
 */

#include <rwcore.h>
#include <rpdbgerr.h>
#include "rpworld.h"
#include "p2stdclsw.h"

#include <math.h>

/* Pick up prototype for memset */
#include <string.h>
#if (defined(_MSC_VER))
#if (_MSC_VER>=1000)
#include <memory.h>
#endif /* (_MSC_VER>=1000) */
#endif /* (defined(_MSC_VER)) */

#include "rtgncpip.h"

/****************************************************************************
 local types
 */

typedef struct InstancedMesh InstancedMesh;
struct InstancedMesh
{
    RxVertexIndex      *indices;
    RwUInt32            numIndices;
    RxObjSpace3DVertex *vertices;
    RwUInt32            numVerts;
    RwUInt32            sizeOfVert;
    RpMaterial         *material;
    RpMesh             *mesh;
};

typedef struct InstancedSector InstancedSector;
struct InstancedSector
{
    /* Serial # - combination of elements contributing to
     * instanced data.  Enables us to detect when a re-instance is
     * necessary. */
    RwUInt32            serialNum;
    InstancedMesh       meshes[1]; /* The meshes :-) */
};

/****************************************************************************
 local defines
 */

#define MESSAGE(_string)                                                  \
    RwDebugSendMessage(rwDEBUGMESSAGE, "WorldSectorInstance.csl", _string)

/****************************************************************************
 Functions
 */

static void
instanceVerts(RxObjSpace3DVertex * instVerts,
              RpWorld * world __RWUNUSEDRELEASE__,
              RpWorldSector * sector, RwUInt32 vertSize)
{
    RxObjSpace3DVertex *vert;
    RwUInt32            numVerts;
    RwUInt32            worldFlags;

    RWFUNCTION(RWSTRING("instanceVerts"));
    RWASSERT(instVerts);
    RWASSERT(world);
    RWASSERT(sector);

    worldFlags = RpWorldGetFlags((RpWorld *)RWSRCGLOBAL(curWorld));

    /* Do a full instance of the world */
    if (worldFlags & (rpWORLDTEXTURED | rpWORLDTEXTURED2))
    {
        const RwTexCoords  *texCoords = sector->texCoords[0];

        numVerts = sector->numVertices;
        vert = instVerts;
        while (numVerts--)
        {
            RxObjSpace3DVertexSetU(vert, texCoords->u);
            RxObjSpace3DVertexSetV(vert, texCoords->v);
            vert = (RxObjSpace3DVertex *) ((RwUInt8 *) vert + vertSize);
            texCoords++;
        }
    }

    if (worldFlags & rpWORLDPRELIT)
    {
        const RwRGBA       *preLitLum = sector->preLitLum;

        numVerts = sector->numVertices;
        vert = instVerts;
        while (numVerts--)
        {
            RxObjSpace3DVertexSetPreLitColor(vert, preLitLum);
            vert = (RxObjSpace3DVertex *) ((RwUInt8 *) vert + vertSize);
            preLitLum++;
        }
    }
    else
    {
        static const RwRGBA opaqueBlack = { 0, 0, 0, 255 };

        numVerts = sector->numVertices;
        vert = instVerts;
        while (numVerts--)
        {
            RxObjSpace3DVertexSetPreLitColor(vert, &opaqueBlack);
            vert = (RxObjSpace3DVertex *) ((RwUInt8 *) vert + vertSize);
        }
    }

    /* Position and normals */
    if (worldFlags & rpWORLDNORMALS)
    {
        const RpVertexNormal *normal = sector->normals;

        numVerts = sector->numVertices;
        vert = instVerts;
        while (numVerts--)
        {
            RwV3d               tempV;

            RPV3DFROMVERTEXNORMAL(tempV, *normal);
            RxObjSpace3DVertexSetNormal(vert, &tempV);

            vert = (RxObjSpace3DVertex *) ((RwUInt8 *) vert + vertSize);
            normal++;
        }
    }

    {
        const RwV3d        *pos = sector->vertices;

        numVerts = sector->numVertices;
        vert = instVerts;
        while (numVerts--)
        {
            RxObjSpace3DVertexSetPos(vert, pos);
            vert = (RxObjSpace3DVertex *) ((RwUInt8 *) vert + vertSize);
            pos++;
        }
    }

    RWRETURNVOID();
}

static void
instanceMeshes(InstancedMesh * meshes, RpMeshHeader * meshHeader,
               RxVertexIndex * dstIndices,
               RwUInt8 * dstVerts, RwUInt32 vertSize)
{
    RwUInt32            numMeshes;
    RpMesh            * mesh;
    RwBool              triStrip;
    RwUInt32            minVert, maxVert;

    RWFUNCTION(RWSTRING("instanceMeshes"));
    RWASSERT(meshes);
    RWASSERT(meshHeader);

    numMeshes = meshHeader->numMeshes;
    mesh = (RpMesh *) (meshHeader + 1);
    triStrip = (meshHeader->flags & rpMESHHEADERTRISTRIP) ? TRUE : FALSE;

    minVert = 0;
    while (numMeshes--)
    {
        const RxVertexIndex *meshInds = mesh->indices;
        RwUInt32            numInds;

        meshes->indices = dstIndices;
        meshes->sizeOfVert = vertSize;
        meshes->material = mesh->material;
        meshes->mesh = (RpMesh *) mesh;

        /* Build the triangles */
        numInds = mesh->numIndices;
        if (meshHeader->flags & rpMESHHEADERUNINDEXED)
        {
            /* Unindexed primitives */
            meshes->vertices =
                (RxObjSpace3DVertex *) & dstVerts[minVert * vertSize];
            meshes->numVerts = mesh->numIndices;

            if (triStrip)
            {
                /* Decompress into a trilist */
                RxVertexIndex i;
                RWASSERT(meshInds == NULL);
                meshes->numIndices = (numInds - 2) * 3;
                for (i = 0;i < (numInds - 2);i++)
                {
                   *dstIndices = i + (i&1);
                    dstIndices++;
                   *dstIndices = i + ((i&1)^1);
                    dstIndices++;
                   *dstIndices = i + 2;
                    dstIndices++;
                }
            }
            else
            {
                meshes->numIndices = numInds;
                if (meshHeader->flags & rpMESHHEADERUNINDEXED)
                {
                    RxVertexIndex i;
                    RWASSERT(meshInds == NULL);
                    for (i = 0;i < numInds;i++)
                    {
                       *dstIndices = i;
                        dstIndices++;
                    }
                }
            }

            minVert += numInds;
        }
        else
        {
            minVert = 0xFFFF;
            maxVert = 0x0000;

            while (numInds--)
            {
                const RxVertexIndex ind = meshInds[numInds];

                /* RWASSERT(0<=ind); */

                minVert = (minVert < (RwUInt32) ind) ? minVert : (RwUInt32) ind;
                maxVert = (maxVert > (RwUInt32) ind) ? maxVert : (RwUInt32) ind;
            }
            numInds = mesh->numIndices;

            if (triStrip)
            {
                /* Decompress into a trilist */
                RwUInt32 flip = 0;

                meshes->numIndices = (numInds - 2) * 3;
                while (numInds > 2)
                {
                    dstIndices[0] =
                        (RxVertexIndex) (meshInds[0 ^ flip] - minVert);
                    dstIndices[1] =
                        (RxVertexIndex) (meshInds[1 ^ flip] - minVert);
                    dstIndices[2] = (RxVertexIndex) (meshInds[2] - minVert);

                    /* Check for bridging (degenerate) triangles that join
                     * several tristrips together into a single tristrips */
                    if ((dstIndices[0] == dstIndices[1]) ||
                        (dstIndices[0] == dstIndices[2]) ||
                        (dstIndices[1] == dstIndices[2]))
                    {
                        meshes->numIndices -= 3;
                    }
                    else
                    {
                        dstIndices += 3;
                    }

                    meshInds++;
                    numInds--;
                    flip ^= 1;
                }
            }
            else
            {
                meshes->numIndices = numInds;
                while (numInds > 2)
                {
                    dstIndices[0] = (RxVertexIndex) (meshInds[0] - minVert);
                    dstIndices[1] = (RxVertexIndex) (meshInds[1] - minVert);
                    dstIndices[2] = (RxVertexIndex) (meshInds[2] - minVert);

                    dstIndices += 3;
                    meshInds += 3;
                    numInds -= 3;
                }
            }

            meshes->vertices =
                (RxObjSpace3DVertex *) & dstVerts[minVert * vertSize];
            meshes->numVerts = (maxVert - minVert) + 1;
        }

        meshes++;
        mesh++;
    }

    RWRETURNVOID();
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/*****************************************************************************
 WorldSectorInstanceNodeFn
*/

static              RwBool
WorldSectorInstanceNodeFn(RxPipelineNodeInstance * self,
                          const RxPipelineNodeParam * params)
{
    RpWorldSector      *sector;
    RpWorld            *world;
    RwResEntry         *repEntry;
    InstancedSector    *instancedSector;
    RxVertexIndex      *dstIndices;
    RxObjSpace3DVertex *dstVerts;
    RpMeshHeader       *meshHeader;
    RwUInt32            numMeshes;
    RwUInt32            numVerts;
    InstancedMesh      *instancedMesh;
    RwUInt32            totalNumTris;
    RwUInt32            worldFlags;
    RwUInt32            vertSize;
    const RwSurfaceProperties *source;
    RWFUNCTION(RWSTRING("WorldSectorInstanceNodeFn"));

    RWASSERT(NULL != self);

    sector = (RpWorldSector *) RxPipelineNodeParamGetData(params);
    RWASSERT(NULL != sector);

    /* TODO: Bad for multithread/multiworld rendering.
     *       SectorGetWorld() would be better. */
    world = (RpWorld *) RWSRCGLOBAL(curWorld);
    RWASSERT(world);

    numVerts = sector->numVertices;
    /* If there ain't vertices, we cain't make packets... */
    if (numVerts <= 0)
    {
        /* Don't execute the rest of the pipeline */
        RWRETURN(TRUE);
    }

    meshHeader = sector->mesh;
    numMeshes = meshHeader->numMeshes;
    /* Early out if no meshes */
    if(numMeshes <= 0)
    {
        /* If the app wants to use plugin data to make packets, it
         * should use its own instancing function. If we have verts
         * here, we need meshes too in order to build a packet. */
        RWRETURN(TRUE);
    }

    worldFlags = RpWorldGetFlags(world);
    repEntry = sector->repEntry;

    /* Figure out the best vertex size */
    vertSize = RxObjSpace3DVertexNoUVsNoNormalsSize;
    if (worldFlags & rpWORLDNORMALS)
    {
        vertSize = RxObjSpace3DVertexNoUVsSize;
    }
    if (worldFlags & (rpWORLDTEXTURED | rpWORLDTEXTURED2))
    {
        vertSize = RxObjSpace3DVertexFullSize;
    }

    /* And how many triangles in total */
    if (meshHeader->flags & rpMESHHEADERTRISTRIP)
    {
        totalNumTris = meshHeader->totalIndicesInMesh - (2 * numMeshes);
    }
    else
    {
        totalNumTris = meshHeader->totalIndicesInMesh / 3;
    }

    if (NULL != repEntry)
    {
        /* If anything has changed, we should re-instance */
        instancedSector = (InstancedSector *) (repEntry + 1);
        dstVerts = ( (RxObjSpace3DVertex *)
                     &instancedSector->meshes[numMeshes] );
        dstIndices = (RxVertexIndex *)
            ( ((RwUInt8 *)dstVerts) + numVerts*vertSize );
        if (instancedSector->serialNum != meshHeader->serialNum)
        {
            /* Things have changed, destroy resources to force reinstance */
            RwResourcesFreeResEntry(repEntry);
            repEntry = (RwResEntry *)NULL;
        }
    }

    if (NULL == repEntry)
    {
        RwUInt32            size;

        size = ( sizeof(InstancedSector) +
                 ((numMeshes - 1) * sizeof(InstancedMesh)) +
                 (totalNumTris * 3 * sizeof(RxVertexIndex)) +
                 (numVerts * vertSize) );
        repEntry =
            RwResourcesAllocateResEntry(sector,
                                        &sector->repEntry,
                                        size,
                                        (RwResEntryDestroyNotify)NULL);
        RWASSERT(NULL != repEntry);

        /* Extra header info */
        instancedSector = (InstancedSector *) (repEntry + 1);
        instancedSector->serialNum = meshHeader->serialNum;

        dstVerts =
            (RxObjSpace3DVertex *) & instancedSector->meshes[numMeshes];
        dstIndices = (RxVertexIndex *)
            ( ((RwUInt8 *)dstVerts) + numVerts*vertSize );

        /* Instance the verts and meshes */
        instanceVerts(dstVerts, world, sector, vertSize);
        instanceMeshes(instancedSector->meshes, meshHeader, dstIndices,
                       (RwUInt8 *)dstVerts, vertSize);

        /* totalNumTris will have changed because instanceMeshes now strips
         * out degenerate triangles, so we update it (and yes, this means
         * the resource entry may be a little too big but we assume it's
         * just a little - if not, target your exporter output to this
         * platform, guys...) */
        totalNumTris = instancedSector->meshes->numIndices / 3;
    }
    else
    {
        RwResourcesUseResEntry(repEntry);
    }

    /* Now build reference clusters into the RepEntry */
    instancedMesh = instancedSector->meshes;
    while (numMeshes--)
    {
        RxPacket           *packet;
        RxCluster          *objVerts, *indices, *meshState;
        RxCluster          *renderState;
        RxMeshStateVector  *meshData;
        RxRenderStateVector *rsvp;
        RpMaterial         *mat;
        const RwRGBA       *MatCol;

        if ((instancedMesh->numVerts   > 0) &&
            (instancedMesh->numIndices > 0)   )
        {
            packet = RxPacketCreate(self);
            RWASSERT(NULL != packet);

            objVerts = RxClusterLockWrite(packet, 0, self);
            RWASSERT(NULL != objVerts);
            indices = RxClusterLockWrite(packet, 1, self);
            RWASSERT(NULL != indices);
            meshState = RxClusterLockWrite(packet, 2, self);
            RWASSERT(NULL != meshState);
            renderState = RxClusterLockWrite(packet, 3, self);
            RWASSERT(NULL != renderState);

            meshState =
                RxClusterInitializeData(meshState, 1,
                                        sizeof(RxMeshStateVector));
            RWASSERT(NULL != meshState);
            renderState =
                RxClusterInitializeData(renderState, 1,
                                        sizeof(RxRenderStateVector));
            RWASSERT(NULL != renderState);

            objVerts = RxClusterSetExternalData(objVerts,
                                                instancedMesh->vertices,
                                                instancedMesh->sizeOfVert,
                                                instancedMesh->numVerts);
            RWASSERT(NULL != objVerts);

            indices = RxClusterSetExternalData(indices,
                                               instancedMesh->indices,
                                               sizeof(RxVertexIndex),
                                               instancedMesh->numIndices);
            RWASSERT(NULL != indices);

            mat = instancedMesh->material;

            /* Set up MeshState data for this packet */
            meshData = RxClusterGetCursorData(meshState, RxMeshStateVector);
            RWASSERT(NULL != meshData);
            meshData->SourceObject = (void *) mat;
            meshData->DataObject = (void *) RxPipelineNodeParamGetData(params);

            /* Currently all we support, but should predicate on mesh contents */
            meshData->PrimType = rwPRIMTYPETRILIST;
            meshData->NumVertices = instancedMesh->numVerts;
            meshData->NumElements = instancedMesh->numIndices / 3;
            /* Set up the Local to Camera matrix for this Sector */
            meshData->Obj2Cam =
                (((RwCamera *) RWSRCGLOBAL(curCamera))->viewMatrix);
            RwMatrixSetIdentity(&meshData->Obj2World);
            source = RpMaterialGetSurfaceProperties(mat);
            RwSurfacePropertiesAssign(&meshData->SurfaceProperties, source);
            meshData->Flags = worldFlags;
            meshData->Texture = RpMaterialGetTexture(mat);

            MatCol = RpMaterialGetColor(mat);
            RwRGBAAssign(&meshData->MatCol, MatCol);
            meshData->Pipeline = mat->pipeline;
            meshData->ClipFlagsAnd = 0;
            meshData->ClipFlagsOr = 0;
            meshData->SourceMesh = (void *) instancedMesh->mesh;

            meshState->numUsed++;

            /* Set up RenderState data for this packet */
            rsvp = RxClusterGetCursorData(renderState, RxRenderStateVector);
            RWASSERT(NULL != rsvp);
            *rsvp = RXPIPELINEGLOBAL(defaultRenderState);
            if (worldFlags & (rpWORLDTEXTURED|rpWORLDTEXTURED2))
            {
                RwTexture          *texture = meshData->Texture;

                if (texture)
                {
                    rsvp->TextureRaster = RwTextureGetRaster(texture);
                    rsvp->AddressModeU = RwTextureGetAddressingU(texture);
                    rsvp->AddressModeV = RwTextureGetAddressingV(texture);
                    rsvp->FilterMode = RwTextureGetFilterMode(texture);
                }
            }

            renderState->numUsed++;

            /* we like transparent objects too */
            if (meshData->MatCol.alpha != 255)
            {
                rsvp->Flags |= rxRENDERSTATEFLAG_VERTEXALPHAENABLE;
            }

            /* Ready for next mesh */
            RxPacketDispatch(packet, 0, self);
        }

        instancedMesh++;
    }

#ifdef RWMETRICS
    /* Now update our metrics statistics */
    RWSRCGLOBAL(metrics)->numVertices  += RpWorldSectorGetNumVertices(sector);
    RWSRCGLOBAL(metrics)->numTriangles += RpWorldSectorGetNumTriangles(sector);
#endif

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionGetWorldSectorInstance returns a pointer to a node
 * to instance a world sector
 *
 * RpWorldSectors are split into RpMeshes when created by an exporter. There
 * should be one mesh per material used in the world sector. It is a
 * requirement of the new flexible pipeline scheme that packets of geometry
 * be entirely self-contained and this has the following ramifications:
 * vertices at material boundaries within an RpWorldSector must be duplicated
 * and vertices in the world sector's vertex array must be sorted on material,
 * so that the runs of vertices for each material are contiguous and mutually
 * disjoint. This arrangement of world sectors is currently guaranteed by all
 * RW exporters.
 *
 * The node will instance the world sector into a RwResEntry (a block of
 * memory in the resources arena) so that hopefully it can be cached in this
 * ready-to-render format and reused for several frames. This RwResEntry will
 * be big enough to hold all the object-space vertices and triangles for all
 * the meshes in the world-sector. When parts of the world-sector's data
 * change, the entire world sector is reinstanced.
 *
 * Depending on the flags of the RpWorld which is the parent of the world
 * sector, an appropriate size of object-space vertex is chosen. If the world
 * sector is not textured, a smaller vertex can be used (we basically truncate
 * the vertex since we have put UVs at the end). If it also has no normals (is
 * not lit) then an even smaller vertex can be used. The size of the
 * appropriate vertex is used as the stride of the object-space vertex cluster.
 * If the world-sector is pre-lit, we copy pre-lighting values into the
 * object-space vertices, otherwise we initialize them to {0, 0, 0, 255}. If
 * the geometry is textured, we copy in RxUVs. If the geometry has normals
 * (is lit), we copy in normals.
 *
 * Indices are currently created as triangle lists even if the world sector
 * was originally formed of triangle strips. When we have a comprehensive set
 * of nodes that can handle triangle-strip-indexed geometry, this will change.
 *
 * The object-space vertex cluster and triangles cluster of each packet
 * references the vertex/triangle data in the RwResEntry and hence these
 * clusters are marked as rxCLFLAGS_EXTERNAL.
 *
 * The mesh state cluster is created on the RxHeap for each packet, and
 * its initialized with a pointer to the RpMaterial of the packet's associated
 * mesh in the SourceObject entry, the material's color in MatCol, its
 * texture (if any) in Texture and a pointer to its material pipeline in
 * Pipeline. The primitive type is currently set to triangle list as mentioned
 * above. The Obj2Cam matrix is initialized from the current camera's frame
 * and the Obj2World matrix is set to the identity. The surface properties are
 * set from those of the owning RpWorld and the flags of the world are also
 * copied in to the mesh state. The NumVertices and NumElements (number of
 * triangles) values are set to the appropriate value for this mesh and the
 * CliFlagsAnd and ClipFlagsOr values are both initialized to zero.
 *
 * The render state cluster is initialized to the default render state and
 * then the texture settings are overridden if the mesh's material has a
 * texture. If the material's alpha value is not 255 then vertex alpha is
 * enabled in the render state's flags.
 *
 * The node has one output, through which the instanced geometry passes.
 * The input requirements of this node:
 *      \li RxClObjSpace3DVertices - don't want
 *      \li RxClIndices            - don't want
 *      \li RxClMeshState          - don't want
 *      \li RxClRenderState        - don't want
 *
 * The characteristics of this node's first output:
 *      \li RxClObjSpace3DVertices - valid
 *      \li RxClIndices            - valid
 *      \li RxClMeshState          - valid
 *      \li RxClRenderState        - valid
 *
 * \return pointer to a node to instance a world sector
 *
 * \see RxNodeDefinitionGetAtomicEnumerateLights
 * \see RxNodeDefinitionGetAtomicInstance
 * \see RxNodeDefinitionGetFastPathSplitter
 * \see RxNodeDefinitionGetLight
 * \see RxNodeDefinitionGetMaterialScatter
 * \see RxNodeDefinitionGetPostLight
 * \see RxNodeDefinitionGetPreLight
 * \see RxNodeDefinitionGetWorldSectorEnumerateLights
 */

RxNodeDefinition   *
RxNodeDefinitionGetWorldSectorInstance(void)
{
    /* note to the uninitiated: when initializing structures in C,
     * it is not necessary to specify values for all the fields;
     * fields not explicitly initialized are set to zero.
     * node specifications typically make free with this facility in a manner
     * similar to the optional arguments feature of C++ */

    static RxClusterRef gNodeClusters[] = { /* */
        {&RxClObjSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClIndices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClRenderState, rxCLALLOWABSENT, rxCLRESERVED}
    };

#define NUMCLUSTERSOFINTEREST \
        ((sizeof(gNodeClusters))/(sizeof(gNodeClusters[0])))

    static RxClusterValidityReq gNodeReqs[NUMCLUSTERSOFINTEREST] = { /* */
        rxCLREQ_DONTWANT,
        rxCLREQ_DONTWANT,
        rxCLREQ_DONTWANT,
        rxCLREQ_DONTWANT
    };

    static RwChar       _Output[] = RWSTRING("Output");
    static RxClusterValid gNodeOut1[NUMCLUSTERSOFINTEREST] = { /* */
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_VALID
    };

    static RxOutputSpec gNodeOuts[] = { /* */
        {
         _Output,              /* Name */
         gNodeOut1,            /* OutputClusters */
         rxCLVALID_INVALID     /* AllOtherClusters */
         }
    };

#define NUMOUTPUTS \
        ((sizeof(gNodeOuts))/(sizeof(gNodeOuts[0])))

    static RwChar       _WorldSectorInstance_csl[] =
        RWSTRING("WorldSectorInstance.csl");

    static RxNodeDefinition nodeWorldSectorInstanceCSL = { /* */
        _WorldSectorInstance_csl, /* Name */
        {                      /* nodemethods */
         WorldSectorInstanceNodeFn, /* +-- nodebody */
            (RxNodeInitFn)NULL,
            (RxNodeTermFn)NULL,
            (RxPipelineNodeInitFn)NULL,
            (RxPipelineNodeTermFn)NULL,
            (RxPipelineNodeConfigFn)NULL,
            (RxConfigMsgHandlerFn)NULL
        },
        {                      /* Io */
         NUMCLUSTERSOFINTEREST, /* +-- NumClustersOfInterest */
         gNodeClusters,        /* +-- ClustersOfInterest */
         gNodeReqs,            /* +-- InputRequirements */
         NUMOUTPUTS,           /* +-- NumOutputs */
         gNodeOuts             /* +-- Outputs */
         },
        0,
        (RxNodeDefEditable)FALSE,
        0
    };

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionGetWorldSectorInstance"));

    /*RWMESSAGE((RWSTRING("Pipeline II node"))); */

    RWRETURN(&nodeWorldSectorInstanceCSL);
}

