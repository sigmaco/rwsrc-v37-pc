/*
 * nodeAtomicInstance
 * Instancing a world atomic
 *
 * Copyright (c) Criterion Software Limited
 */
/****************************************************************************
 *                                                                          *
 * module : nodeAtomicInstance.c                                            *
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
    RxVertexIndex          *indices;
    RwUInt32                numIndices;
    RxObjSpace3DVertex    *vertices;
    RwUInt32                numVerts;
    RwUInt32                sizeOfVert;
    RpMaterial              *material;
    RpMesh                  *mesh;
};

typedef struct InstancedAtomic InstancedAtomic;
struct InstancedAtomic
{
    RwUInt32                serialNum;      /* Serial # - combination of
                                             * elements contributing to
                                             * instanced data.  Enables us to
                                             * detect when a re-instance is
                                             * necessary.
                                             */

    InstancedMesh           meshes[1];      /* The meshes :-) */
};

/****************************************************************************
 local defines
 */

#define MESSAGE(_string)                                             \
    RwDebugSendMessage(rwDEBUGMESSAGE, "AtomicInstance.csl", _string)

/****************************************************************************
 Functions
 */

static void
instanceVertPos(RxObjSpace3DVertex *instVerts, RpAtomic *atomic,
                RpGeometry *geom, RwUInt32 vertSize)
{
    const RpInterpolator    *interp;
    RwInt32                 startMT, endMT;
    RwInt32                 numVerts;
    static const RwV3d      zeroNormal = {(RwReal)(0.0f),
                                          (RwReal)(0.0f),
                                          (RwReal)(0.0f)};

    RWFUNCTION(RWSTRING("instanceVertPos"));
    RWASSERT(instVerts);
    RWASSERT(atomic);
    RWASSERT(geom);

    interp = &atomic->interpolator;
    startMT = interp->startMorphTarget;
    endMT = interp->endMorphTarget;
    numVerts = geom->numVertices;

    if ((startMT >= geom->numMorphTargets) ||
        (endMT >= geom->numMorphTargets))
    {
        /* Clamp to in range */
        startMT = endMT = 0;
    }

    if (startMT == endMT)
    {
        const RpMorphTarget *morphTarget = &geom->morphTarget[startMT];
        const RwV3d         *pos = morphTarget->verts;

        if (rpGEOMETRYNORMALS & RpGeometryGetFlags(geom))
        {
            const RwV3d     *normal = morphTarget->normals;

            while (numVerts--)
            {
                RxObjSpace3DVertexSetPos(instVerts, pos);
                RxObjSpace3DVertexSetNormal(instVerts, normal);
                pos++;
                normal++;
                instVerts =
                    (RxObjSpace3DVertex *)((RwUInt8 *)instVerts + vertSize);
            }
        }
        else
        {
            while (numVerts--)
            {
                RxObjSpace3DVertexSetPos(instVerts, pos);
                RxObjSpace3DVertexSetNormal(instVerts, &zeroNormal);
                pos++;
                instVerts = (RxObjSpace3DVertex *)((RwUInt8 *)instVerts + vertSize);
            }
        }
    }
    else
    {
        /* Interpolate */
        const RpMorphTarget *morphTarget1 = &geom->morphTarget[startMT];
        const RpMorphTarget *morphTarget2 = &geom->morphTarget[endMT];
        const RwV3d         *pos1 = morphTarget1->verts;
        const RwV3d         *pos2 = morphTarget2->verts;
        RwReal              scale = interp->recipTime * interp->position;

        if (rpGEOMETRYNORMALS & RpGeometryGetFlags(geom))
        {
            const RwV3d     *normal1 = morphTarget1->normals;
            const RwV3d     *normal2 = morphTarget2->normals;

            while (numVerts--)
            {
                RwV3d   tempV;

                RwV3dSub(&tempV, pos2, pos1);
                RwV3dScale(&tempV, &tempV, scale);
                RwV3dAdd(&tempV, &tempV, pos1);
                RxObjSpace3DVertexSetPos(instVerts, &tempV);

                RwV3dSub(&tempV, normal2, normal1);
                RwV3dScale(&tempV, &tempV, scale);
                RwV3dAdd(&tempV, &tempV, normal1);
                RxObjSpace3DVertexSetNormal(instVerts, &tempV);

                pos1++;
                pos2++;
                normal1++;
                normal2++;
                instVerts = (RxObjSpace3DVertex *)((RwUInt8 *)instVerts + vertSize);
            }
        }
        else
        {
            while (numVerts--)
            {
                RwV3d   tempV;

                RwV3dSub(&tempV, pos2, pos1);
                RwV3dScale(&tempV, &tempV, scale);
                RwV3dAdd(&tempV, &tempV, pos1);
                RxObjSpace3DVertexSetPos(instVerts, &tempV);
                RxObjSpace3DVertexSetNormal(instVerts, &zeroNormal);

                pos1++;
                pos2++;
                instVerts = (RxObjSpace3DVertex *)((RwUInt8 *)instVerts + vertSize);
            }
        }
    }

    RWRETURNVOID();
}

static void
instanceVerts(RxObjSpace3DVertex *instVerts, RpAtomic *atomic,
              RpGeometry *geom, RwUInt32 vertSize)
{
    RxObjSpace3DVertex    *vert;
    RwUInt32                numVerts;
    RwUInt32                geomFlags;

    RWFUNCTION(RWSTRING("instanceVerts"));
    RWASSERT(instVerts);
    RWASSERT(atomic);
    RWASSERT(geom);

    geomFlags = RpGeometryGetFlags(geom);

    /* Do a full instance of the geometry */
    if (geomFlags & rpGEOMETRYTEXTURED)
    {
        const RwTexCoords       *texCoords =
            RpGeometryGetVertexTexCoords(geom, rwTEXTURECOORDINATEINDEX0);

        numVerts = geom->numVertices;
        vert = instVerts;
        while (numVerts--)
        {
            RxObjSpace3DVertexSetU(vert, texCoords->u);
            RxObjSpace3DVertexSetV(vert, texCoords->v);
            vert = (RxObjSpace3DVertex *)((RwUInt8 *)vert + vertSize);
            texCoords++;
        }
    }

    if (geomFlags & rpGEOMETRYPRELIT)
    {
        const RwRGBA            *preLitLum = geom->preLitLum;

        numVerts = geom->numVertices;
        vert = instVerts;
        while (numVerts--)
        {
            RxObjSpace3DVertexSetPreLitColor(vert, preLitLum);
            vert = (RxObjSpace3DVertex *)((RwUInt8 *)vert + vertSize);
            preLitLum++;
        }
    }
    else
    {
        static const RwRGBA opaqueBlack = {0, 0, 0, 255};

        numVerts = geom->numVertices;
        vert = instVerts;
        while (numVerts--)
        {
            RxObjSpace3DVertexSetPreLitColor(vert, &opaqueBlack);
            vert = (RxObjSpace3DVertex *)((RwUInt8 *)vert + vertSize);
        }
    }

    /* Position and normal */
    instanceVertPos(instVerts, atomic, geom, vertSize);

    RWRETURNVOID();
}

static void
instanceMeshes(InstancedMesh *meshes, RpMeshHeader * meshHeader,
               RxVertexIndex *dstIndices, RwUInt8 *dstVerts, RwUInt32 vertSize)
{
    RwUInt32                numMeshes;
    RpMesh                 *mesh;
    RwBool                  triStrip;
    RwUInt32                minVert, maxVert;

    RWFUNCTION(RWSTRING("instanceMeshes"));
    RWASSERT(meshes);
    RWASSERT(meshHeader);

    numMeshes = meshHeader->numMeshes;
    mesh = (RpMesh *)(meshHeader + 1);
    triStrip = (meshHeader->flags & rpMESHHEADERTRISTRIP) ? TRUE : FALSE;

    minVert = 0;
    while (numMeshes--)
    {
        const RxVertexIndex   *meshInds = mesh->indices;
        RwUInt32                numInds;

        meshes->indices = dstIndices;
        meshes->sizeOfVert = vertSize;
        meshes->material = mesh->material;
        meshes->mesh = (RpMesh *)mesh;

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

            meshes->vertices =
                (RxObjSpace3DVertex *) & dstVerts[minVert * vertSize];
            meshes->numVerts = (maxVert - minVert) + 1;

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
 AtomicInstanceNode
*/

static RwBool
AtomicInstanceNode(RxPipelineNodeInstance *self,
                   const RxPipelineNodeParam * params)
{
    RpAtomic           *atomic;
    RpGeometry         *geom;
    RwResEntry         *repEntry;
    RwResEntry         **repEntryOwner;
    void               *owner;
    InstancedAtomic    *instancedAtomic;
    RxVertexIndex      *dstIndices;
    RxObjSpace3DVertex *dstVerts;
    RpMeshHeader       *meshHeader;
    RwUInt32            numMeshes;
    RwUInt32            numVerts;
    RpInterpolator     *interpolator;
    InstancedMesh      *instancedMesh;
    RwUInt32            totalNumTris;
    RwUInt32            geomFlags;
    RwUInt32            vertSize;
    const RwSurfaceProperties *source;
    RWFUNCTION(RWSTRING("AtomicInstanceNode"));

    RWASSERT(NULL != self);

    atomic = (RpAtomic *)RxPipelineNodeParamGetData(params);
    RWASSERT(NULL != atomic);

    geom = RpAtomicGetGeometry(atomic);
    RWASSERT(NULL != geom);

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
    if(numMeshes <= 0)
    {
        /* If the app wants to use plugin data to make packets, it
         * should use its own instancing function. If we have verts
         * here, we need meshes too in order to build a packet. */
        RWRETURN(TRUE);
    }

    geomFlags = RpGeometryGetFlags(geom);
    interpolator = &atomic->interpolator;

    /* If the geometry has more than one morph target the resEntry in the
     * atomic is used else the resEntry in the geometry */
    if (RpGeometryGetNumMorphTargets(geom) != 1)
    {
        owner = (void *)atomic;
        repEntryOwner = &atomic->repEntry;
        repEntry = atomic->repEntry;
    }
    else
    {
        owner = (void *)geom;
        repEntryOwner = &geom->repEntry;
        repEntry = geom->repEntry;
    }

    /* Figure out the best vertex size */
    vertSize = RxObjSpace3DVertexNoUVsNoNormalsSize;
    if (geomFlags & rpGEOMETRYNORMALS)
    {
        vertSize = RxObjSpace3DVertexNoUVsSize;
    }
    if (geomFlags & rpGEOMETRYTEXTURED)
    {
        vertSize = RxObjSpace3DVertexFullSize;
    }

    /* And how many triangles in total */
    if (meshHeader->flags & rpMESHHEADERTRISTRIP)
    {
        totalNumTris = meshHeader->totalIndicesInMesh - (2*numMeshes);
    }
    else
    {
        totalNumTris = meshHeader->totalIndicesInMesh / 3;
    }

    if (NULL != repEntry)
    {
        /* If anything has changed, we should re-instance */
        instancedAtomic = (InstancedAtomic *)(repEntry + 1);
        dstVerts =
            (RxObjSpace3DVertex *)&instancedAtomic->meshes[numMeshes];
        dstIndices =
            (RxVertexIndex *)(((RwUInt8 *)dstVerts) + (numVerts * vertSize));
        if (instancedAtomic->serialNum != meshHeader->serialNum)
        {
            /* Things have changed, destroy resources to force reinstance */
            RwResourcesFreeResEntry(repEntry);
            repEntry = (RwResEntry *)NULL;
        }
    }

    if (!repEntry)
    {
        RwUInt32        size;

        size = ( sizeof(InstancedAtomic) +
                 ((numMeshes-1) * sizeof(InstancedMesh)) +
                 (numVerts * vertSize) +
                 (totalNumTris * 3 * sizeof(RxVertexIndex)));

        repEntry =
            RwResourcesAllocateResEntry(owner,
                                        repEntryOwner,
                                        size,
                                        (RwResEntryDestroyNotify)NULL);
        RWASSERT(NULL != repEntry);

        /* Extra header info */
        instancedAtomic = (InstancedAtomic *)(repEntry + 1);
        instancedAtomic->serialNum = meshHeader->serialNum;

        dstVerts =
            (RxObjSpace3DVertex *)&instancedAtomic->meshes[numMeshes];
        dstIndices =
            (RxVertexIndex *)(((RwUInt8 *)dstVerts) + (numVerts * vertSize));

        /* Instance the verts and meshes */
        instanceVerts(dstVerts, atomic, geom, vertSize);
        instanceMeshes(instancedAtomic->meshes, meshHeader,
                       dstIndices, (RwUInt8 *)dstVerts, vertSize);

        /* All clean now */
        geom->lockedSinceLastInst = 0;
    }
    else
    {
        RwUInt32 dirtyFlags = geom->lockedSinceLastInst;

        RwResourcesUseResEntry(repEntry);

        /* If verts/normals are only thing to change, reinstance just those */
        if (interpolator->flags & rpINTERPOLATORDIRTYINSTANCE)
        {
            dirtyFlags |= rpGEOMETRYLOCKVERTICES|rpGEOMETRYLOCKNORMALS;
            interpolator->flags &= ~rpINTERPOLATORDIRTYINSTANCE;
        }

        switch (dirtyFlags)
        {
            case (rpGEOMETRYLOCKVERTICES):
            case (rpGEOMETRYLOCKNORMALS):
            case (rpGEOMETRYLOCKVERTICES|rpGEOMETRYLOCKNORMALS):
            {
                instanceVertPos(dstVerts, atomic, geom, vertSize);
                break;
            }
            case (0):
            {
                break;
            }
            default:
            {
                instanceVerts(dstVerts, atomic, geom, vertSize);
                instanceMeshes(instancedAtomic->meshes, meshHeader,
                               dstIndices, (RwUInt8 *)dstVerts, vertSize);
                break;
            }
        }

        geom->lockedSinceLastInst = 0;
    }

    /* Now build reference clusters into the RepEntry */
    instancedMesh = instancedAtomic->meshes;
    while (numMeshes--)
    {
        RxPacket            *packet;
        RxCluster           *objVerts, *indices, *meshState, *renderState;
        RxMeshStateVector   *meshData;
        RxRenderStateVector *rsvp;
        RxRenderStateVector *defaultRsvp;
        RpMaterial          *mat;
        const RwRGBA        *MatCol;

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

            meshState = RxClusterInitializeData(
                            meshState, 1, sizeof(RxMeshStateVector));
            RWASSERT(NULL != meshState);
            renderState = RxClusterInitializeData(
                            renderState, 1, sizeof(RxRenderStateVector));
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
            meshData->SourceObject = (void *)mat;
            meshData->DataObject = RxPipelineNodeParamGetData(params);
            /* Currently all we support, but
             * should predicate on mesh contents */
            meshData->PrimType = rwPRIMTYPETRILIST;
            meshData->NumVertices = instancedMesh->numVerts;
            meshData->NumElements = instancedMesh->numIndices / 3;
            /* Set up the Local to Camera matrix for this Atomic */
            meshData->Obj2Cam = *RwFrameGetLTM(RpAtomicGetFrame(atomic));
            /* TODO: global camera bad for multithreading with multiple cameras */
            RwMatrixTransform(&meshData->Obj2Cam,
                              &(((RwCamera *)RWSRCGLOBAL(curCamera))->viewMatrix),
                              rwCOMBINEPOSTCONCAT);
            meshData->Obj2World = *RwFrameGetLTM(RpAtomicGetFrame(atomic));
            source = RpMaterialGetSurfaceProperties(mat);
            RwSurfacePropertiesAssign(&meshData->SurfaceProperties, source);

            meshData->Flags = geomFlags;
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

            defaultRsvp = &RXPIPELINEGLOBAL(defaultRenderState);
            RxRenderStateVectorAssign(rsvp, defaultRsvp);

            if (geomFlags & rpGEOMETRYTEXTURED)
            {
                RwTexture *texture = meshData->Texture;

                if (NULL != texture)
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
    RWSRCGLOBAL(metrics)->numVertices  += RpGeometryGetNumVertices(geom);
    RWSRCGLOBAL(metrics)->numTriangles += RpGeometryGetNumTriangles(geom);
#endif

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionGetAtomicInstance returns a pointer to a node
 * to instance an atomic
 *
 * RpGeometrys are split into RpMeshes when RpGeometryUnlock is called. There
 * should be one mesh per material used in the geometry. It is a requirement
 * of the new flexible pipeline scheme that packets of geometry be entirely
 * self-contained and this has the following ramifications: vertices at
 * material boundaries within an RpGeometry must be duplicated and vertices
 * in the geometry's vertex array must be sorted on material, so that the
 * runs of vertices for each material are contiguous and mutually disjoint.
 * This arrangement of geometries is currently guaranteed by all RW
 * exporters as well as RpGeometryStreamWrite. If you procedurally generate
 * geometry in your application and have more than one material assigned to
 * that geometry, be careful that you comply with the above requirements. If
 * possible, generate the geometry once, save it to disk and load it up at
 * the beginning of the app rather than re-generating it every time the app
 * is run.
 *
 * The node will instance the geometry into a RwResEntry (a block of memory
 * in the resources arena) so that hopefully it can be cached in this
 * ready-to-render format and reused for several frames. This RwResEntry will
 * be big enough to hold all the object-space vertices and triangles for all
 * the meshes in the geometry. When parts of the geometry's data change, these
 * will need to be reinstanced. Sometimes this involves modifying only vertex
 * data (say vertex colors or texture coords) but sometimes it involves
 * reinstancing triangles or the entire RwResEntry. If the geometry has more
 * than one RpMorphTarget then vertex positions and normals will need to be
 * reinstanced every frame the animation progresses - interpolation between
 * RpMorphTargets is performed here.
 *
 * Depending on the flags of the RpGeometry, an appropriate size of
 * object-space vertex is chosen. If the geometry is not textured, a smaller
 * vertex can be used (we basically truncate the vertex since we have put UVs
 * at the end). If it also has no normals (is not lit) then an even smaller
 * vertex can be used. The size of the appropriate vertex is used as the
 * stride of the object-space vertex cluster. If the geometry is pre-lit, we
 * copy pre-lighting values into the object-space vertices, otherwise we
 * initialize them to {0, 0, 0, 255}. If the geometry is textured, we copy in
 * RxUVs. If the geometry has normals (is lit), we copy in normals.
 *
 * Indices are currently created as triangle lists even if the geometry was
 * originally formed of triangle strips. When we have a comprehensive set of
 * nodes that can handle triangle-strip-indexed geometry, this will change.
 *
 * The object-space vertex cluster and indices cluster of each packet
 * references the vertex/index data in the RwResEntry and hence these
 * clusters are marked as rxCLFLAGS_EXTERNAL.
 *
 * The mesh state cluster is created on the RxHeap for each packet, and
 * its initialized with a pointer to the RpMaterial of the packet's associated
 * mesh in the SourceObject entry, the material's color in MatCol, its
 * texture (if any) in Texture and a pointer to its material pipeline in
 * Pipeline. The primitive type is currently set to triangle list as mentioned
 * above. The Obj2Cam and Obj2World matrices are initialized from the atomic's
 * frame and the current camera's frame. The surface properties are set from
 * those of the geometry and the flags of the geometry are also copied in to
 * the mesh state. The NumVertices and NumElements (number of triangles)
 * values are set to the appropriate value for this mesh and the CliFlagsAnd
 * and ClipFlagsOr values are both initialized to zero.
 *
 * The render state cluster is initialized to the default render state and
 * then the texture settings are overridden if the mesh's material has a
 * texture. If the material's alpha value is not 255 then vertex alpha is
 * enabled in the render state's flags.
 *
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
 * \return pointer to a node to instance an atomic
 *
 * \see RxNodeDefinitionGetAtomicEnumerateLights
 * \see RxNodeDefinitionGetFastPathSplitter
 * \see RxNodeDefinitionGetLight
 * \see RxNodeDefinitionGetMaterialScatter
 * \see RxNodeDefinitionGetPostLight
 * \see RxNodeDefinitionGetPreLight
 * \see RxNodeDefinitionGetWorldSectorEnumerateLights
 * \see RxNodeDefinitionGetWorldSectorInstance
 */

RxNodeDefinition *
RxNodeDefinitionGetAtomicInstance(void)
{
    static RxClusterRef N1clofinterest[] = { /* */
        {&RxClObjSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClIndices,            rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClMeshState,          rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClRenderState,        rxCLALLOWABSENT, rxCLRESERVED} };

    #define NUMCLUSTERSOFINTEREST \
        ((sizeof(N1clofinterest))/(sizeof(N1clofinterest[0])))

    static RxClusterValidityReq N1inputreqs[NUMCLUSTERSOFINTEREST] = { /* */
        rxCLREQ_DONTWANT,
        rxCLREQ_DONTWANT,
        rxCLREQ_DONTWANT,
        rxCLREQ_DONTWANT };

    static RwChar _AtomicInstanceOut[] = RWSTRING("AtomicInstanceOut");
    static RxClusterValid N1outcl1[NUMCLUSTERSOFINTEREST] = { /* */
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_VALID };

    static RxOutputSpec N1outputs[] = { /* */
        {_AtomicInstanceOut,
         N1outcl1,
         rxCLVALID_NOCHANGE} };

    #define NUMOUTPUTS \
        ((sizeof(N1outputs))/(sizeof(N1outputs[0])))

    static RwChar _AtomicInstance_csl[] = RWSTRING("AtomicInstance.csl");

    static RxNodeDefinition nodeAtomicInstanceCSL = { /* */
        _AtomicInstance_csl,
        {
            AtomicInstanceNode,
            (RxNodeInitFn)NULL,
            (RxNodeTermFn)NULL,
            (RxPipelineNodeInitFn)NULL,
            (RxPipelineNodeTermFn)NULL,
            (RxPipelineNodeConfigFn)NULL,
            (RxConfigMsgHandlerFn)NULL
        },
        {
            NUMCLUSTERSOFINTEREST,
            N1clofinterest,
            N1inputreqs,
            NUMOUTPUTS,
            N1outputs
                },
        0,
        (RxNodeDefEditable) FALSE,
        0
    };

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionGetAtomicInstance"));

    /*RWMESSAGE((RWSTRING("Pipeline II node")));*/

    RWRETURN(&nodeAtomicInstanceCSL);
}

