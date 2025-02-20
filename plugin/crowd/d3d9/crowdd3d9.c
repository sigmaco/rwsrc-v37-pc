/*
 *  crowdd3d9.c
 *
 *  D3D9 implementation of RpCrowd
 *
 *  Uses the static billboard system (see crowdstatic.c|h).
 */

#include <d3d9.h>

#include "rwcore.h"
#include "rpworld.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpcrowd.h"
#include "crowdcommon.h"
#include "crowdstaticbb.h"

/******************************************************************************
 *  Defines
 */

/*
 *  NUMMATPERSEQ is the number of points in a sequence of an
 *  RpCrowdAnimation which are simultaneously referenced by materials
 *  of the crowd. This should be <= rpCROWDANIMATIONMAXSTEPS. Large
 *  numbers give a crowd more randomness, whereas smaller numbers might
 *  be more efficient.
 */
#define NUMMATPERSEQ  rpCROWDANIMATIONMAXSTEPS
#define NUMCROWDMATERIALS (rpCROWDANIMATIONNUMSEQ * NUMMATPERSEQ)

/******************************************************************************
 *  Types
 */
struct _rpD3D9CrowdStaticBB
{
    RpAtomic           *atomic;
    RpCrowdTexture   ***splitTextures;  /* Array of Arrays of texture pointers. */
    RpMaterial        **materials;      /* Crowd Materials */
    RwUInt32            numMatPerSeq;
    RwBBox              boundingBox;
};
typedef struct _rpD3D9CrowdStaticBB rpD3D9CrowdStaticBB;

typedef struct _InstancedRow
{
    RxD3D9InstanceData *mesh;
    RwUInt32 row;

} InstancedRow;

/******************************************************************************
 *  Global variables
 */

/* RpCrowd device extension data */
RwUInt32 _rpCrowdDeviceDataSize = sizeof(rpD3D9CrowdStaticBB);

/******************************************************************************
 *  Local variables
 */
static InstancedRow InstancedRowsToRender[NUMCROWDMATERIALS];
static RwInt32      NumRows;

static rpD3D9CrowdStaticBB *CurrentCrowd = NULL;

/*
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *      Internal Functions
 *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/****************************************************************************
 D3D9SortTriangles

 Purpose:   Sort the triangles indices.

 On entry:  indices     - Pointer to an array of indices
            numIndices  - Number of indices
*/
static int
D3D9SortTriangles(const void *pA, const void *pB)
{
    const RxVertexIndex *pTA = (const RxVertexIndex *)pA;
    const RxVertexIndex *pTB = (const RxVertexIndex *)pB;

    RwInt32 sortedIndexA[3];
    RwInt32 sortedIndexB[3];
    RwInt32 temp;


    RWFUNCTION( RWSTRING( "D3D9SortTriangles" ) );

    sortedIndexA[0] = pTA[0];
    sortedIndexA[1] = pTA[1];
    sortedIndexA[2] = pTA[2];

    if (sortedIndexA[0] > sortedIndexA[1])
    {
        temp = sortedIndexA[0];
        sortedIndexA[0] = sortedIndexA[1];
        sortedIndexA[1] = temp;
    }

    if (sortedIndexA[1] > sortedIndexA[2])
    {
        temp = sortedIndexA[1];
        sortedIndexA[1] = sortedIndexA[2];
        sortedIndexA[2] = temp;

        if (sortedIndexA[0] > sortedIndexA[1])
        {
            temp = sortedIndexA[0];
            sortedIndexA[0] = sortedIndexA[1];
            sortedIndexA[1] = temp;
        }
    }

    sortedIndexB[0] = pTB[0];
    sortedIndexB[1] = pTB[1];
    sortedIndexB[2] = pTB[2];

    if (sortedIndexB[0] > sortedIndexB[1])
    {
        temp = sortedIndexB[0];
        sortedIndexB[0] = sortedIndexB[1];
        sortedIndexB[1] = temp;
    }

    if (sortedIndexB[1] > sortedIndexB[2])
    {
        temp = sortedIndexB[1];
        sortedIndexB[1] = sortedIndexB[2];
        sortedIndexB[2] = temp;

        if (sortedIndexB[0] > sortedIndexB[1])
        {
            temp = sortedIndexB[0];
            sortedIndexB[0] = sortedIndexB[1];
            sortedIndexB[1] = temp;
        }
    }

    if (sortedIndexA[0] == sortedIndexB[0])
    {
        if (sortedIndexA[1] == sortedIndexB[1])
        {
            RWRETURN( sortedIndexA[2] - sortedIndexB[2] );
        }

        RWRETURN( sortedIndexA[1] - sortedIndexB[1] );
    }

    RWRETURN( sortedIndexA[0] - sortedIndexB[0] );
}

/****************************************************************************
 *  D3D9CrowdFrustumTestBBox
 */
static RwFrustumTestResult
D3D9CrowdFrustumTestBBox(const RwCamera *camera,
                         const void *boundingBox)
{
    const RwV3d          *vect;
    const RwFrustumPlane *frustumPlane;
    RwInt32              numPlanes;
    RwFrustumTestResult  testResult;
    RwReal               distance;

    RWFUNCTION(RWSTRING("D3D9CrowdFrustumTestBBox"));

    vect = ((const RwV3d *) boundingBox);
    frustumPlane = camera->frustumPlanes;
    numPlanes = 6;

    testResult = rwSPHEREINSIDE;

    do
    {
        /* Check against plane */
        distance = vect[1 - frustumPlane->closestX].x *
                    frustumPlane->plane.normal.x +
                   vect[1 - frustumPlane->closestY].y *
                    frustumPlane->plane.normal.y +
                   vect[1 - frustumPlane->closestZ].z *
                    frustumPlane->plane.normal.z;

        if (distance > frustumPlane->plane.distance)
        {
            distance = vect[frustumPlane->closestX].x *
                        frustumPlane->plane.normal.x +
                       vect[frustumPlane->closestY].y *
                        frustumPlane->plane.normal.y +
                       vect[frustumPlane->closestZ].z *
                        frustumPlane->plane.normal.z;

            if (distance < frustumPlane->plane.distance)
            {
                testResult = rwSPHEREBOUNDARY;
            }
            else
            {
                testResult = rwSPHEREOUTSIDE;

                break;
            }
        }

        /* Next plane */
        frustumPlane++;
    }
    while (--numPlanes);

    /* Inside all the planes */
    RWRETURN(testResult);
}

/****************************************************************************
 *  D3D9InstancedDataMatSortCB
 */
static int
D3D9InstancedDataMatSortCB(const void *data1, const void *data2)
{
    const InstancedRow *elem1;
    const InstancedRow *elem2;
    int value;

    RWFUNCTION(RWSTRING("D3D9InstancedDataMatSortCB"));

    elem1 = (const InstancedRow *)data1;
    elem2 = (const InstancedRow *)data2;

    /* Sort by texture */
    value = ( (RwInt32)(elem1->row) -
              (RwInt32)(elem2->row) );

    if (!value)
    {
        /* Sort by vertex buffer position */
        value = ( (RwInt32)(elem1->mesh->baseIndex) -
                    (RwInt32)(elem2->mesh->baseIndex) );
    }

    RWRETURN(value);
}

/****************************************************************************
 D3D9CrowdRenderCallback
 */
void
D3D9CrowdRenderCallback(RwResEntry *repEntry,
                              void *object,
                              RwUInt8 type,
                              RwUInt32 flags)
{
    static D3DMATRIX texMat =
    {
         1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f
    };

    RwCamera                *cam;
    RwFrustumTestResult     testResult;
    RxD3D9ResEntryHeader    *resEntryHeader;
    RxD3D9InstanceData      *instancedData;
    RwInt32                 numMeshes;
    RwUInt32                oldAlphaRef;
    RwUInt32                lastRow;
    RwReal                  invNumRows;
    RwInt32                 n;
    RwUInt32                alphaBlend, alphaTest;
    RwBool                  alphaBlendEnabled;

    RWFUNCTION(RWSTRING("D3D9CrowdRenderCallback"));

    /* Enable clipping if needed */
    cam = RwCameraGetCurrentCamera();
    RWASSERT(cam);

    testResult = D3D9CrowdFrustumTestBBox(cam, &(CurrentCrowd->boundingBox));

    if (testResult == rwSPHEREINSIDE)
    {
        RwD3D9SetRenderState(D3DRS_CLIPPING, FALSE);
    }
    else if (testResult == rwSPHEREBOUNDARY)
    {
        RwD3D9SetRenderState(D3DRS_CLIPPING, TRUE);
    }
    else
    {
        RWRETURNVOID();
    }

    /* Set Last to force the call */
    lastRow = 0xffffffff;

    /* Get the instanced data */
    resEntryHeader = (RxD3D9ResEntryHeader *)(repEntry + 1);
    instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);

    /*
     * Data shared between meshes
     */
    RwD3D9SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    RwD3D9SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS,
                                  D3DTTFF_COUNT2);

    /*
     * Set the Default Pixel shader
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

    /* Set the texture */
    RwD3D9SetTexture(instancedData->material->texture, 0);

    /*
     * Vertex shader
     */
    RwD3D9SetVertexShader(instancedData->vertexShader);

    /* Get vertex alpha state */
    RwD3D9GetRenderState(D3DRS_ALPHABLENDENABLE, &alphaBlend);
    RwD3D9GetRenderState(D3DRS_ALPHATESTENABLE, &alphaTest);
    RwD3D9GetRenderState(D3DRS_ALPHAREF, &oldAlphaRef);

    /* Set required render states */
    alphaBlendEnabled = alphaBlend;
    if (!alphaTest)
    {
        RwD3D9SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    }
    RwD3D9SetRenderState(D3DRS_ALPHAREF, 128);    /* let's pretend to be a PS2 */

    /* Get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;

    /*
     * Prepare render information
     */
    for(n = 0; n < numMeshes; n++)
    {
        InstancedRowsToRender[n].mesh = instancedData + n;
    }

    invNumRows = ((RwReal)1.f) / ((RwReal)NumRows);

    /*
     * Sort by texture
     */
    qsort(InstancedRowsToRender,
          numMeshes,
          sizeof(InstancedRow),
          D3D9InstancedDataMatSortCB);

    /*
     * Render
     */
    for(n = 0; n < numMeshes; n++)
    {
        instancedData = InstancedRowsToRender[n].mesh;

        /* Set vertex alpha blend */
        if (instancedData->vertexAlpha)
        {
            if (!alphaBlendEnabled)
            {
                alphaBlendEnabled = TRUE;

                /* We need to force it again */
                RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)rwBLENDSRCALPHA);
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)rwBLENDINVSRCALPHA);
            }
        }
        else
        {
            if (alphaBlendEnabled)
            {
                alphaBlendEnabled = FALSE;

                RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            }
        }

        if (lastRow != InstancedRowsToRender[n].row)
        {
            texMat._22 = invNumRows;
            texMat._32 = ((RwReal)InstancedRowsToRender[n].row) * invNumRows;

            RwD3D9SetTransform(D3DTS_TEXTURE0, &texMat);

            lastRow = InstancedRowsToRender[n].row;
        }

        /* Draw the indexed primitive */
        RwD3D9DrawIndexedPrimitive((D3DPRIMITIVETYPE)resEntryHeader->primType,
                                   instancedData->baseIndex,
                                   0, instancedData->numVertices,
                                   instancedData->startIndex,
                                   instancedData->numPrimitives);
    }

    RwD3D9SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS,
                                  D3DTTFF_DISABLE);

    /* Restore render states */
    RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, alphaBlend);
    RwD3D9SetRenderState(D3DRS_ALPHATESTENABLE, alphaTest);
    RwD3D9SetRenderState(D3DRS_ALPHAREF, oldAlphaRef);

    RWRETURNVOID();
}

/****************************************************************************
 *  D3D9CrowdPipeCreate
 */
static RxPipeline *
D3D9CrowdPipeCreate(void)
{
    RxPipeline  *pipe;

    RWFUNCTION(RWSTRING("D3D9CrowdPipeCreate"));

    pipe = RxPipelineCreate();
    if (pipe)
    {
        RxLockedPipe    *lpipe;

        lpipe = RxPipelineLock(pipe);
        if (NULL != lpipe)
        {
            RxNodeDefinition    *instanceNode;
            RxPipelineNode      *node;

            /*
             * Get the instance node definition
             */
            instanceNode = RxNodeDefinitionGetD3D9AtomicAllInOne();
            RWASSERT(NULL != instanceNode);

            /*
             * Add the node to the pipeline
             */
            lpipe = RxLockedPipeAddFragment(lpipe, NULL, instanceNode, NULL);

            /*
             * Unlock the pipeline
             */
            lpipe = RxLockedPipeUnlock(lpipe);

            /*
             * Set the pipeline specific data
             */
            node = RxPipelineFindNodeByName(pipe, instanceNode->name, NULL, NULL);
            RWASSERT(NULL != node);

            /*
             * Set the MatFX render callback
             */
            RxD3D9AllInOneSetRenderCallBack(node, D3D9CrowdRenderCallback);

            RWRETURN(pipe);
        }

        RxPipelineDestroy(pipe);
    }

    RWRETURN(NULL);
}

/******************************************************************************
 */
static void
D3D9ReorderGeometry(RpGeometry *geometry)
{
    RxVertexIndex *pOld2NewTable;
    RxVertexIndex *pRemapTable = NULL;
    RwInt32       maxVertex;
    RwUInt32    numMeshes, currentMesh, n;
    RpMesh     *mesh;

    RWFUNCTION(RWSTRING("D3D9ReorderGeometry"));
    RWASSERT(geometry);

    pOld2NewTable = (RxVertexIndex *)RwMalloc(geometry->numVertices * sizeof(RxVertexIndex),
                                              rwID_CROWDPPPLUGIN | rwMEMHINTDUR_FUNCTION);
    maxVertex = 0;

    memset(pOld2NewTable, 0xff, geometry->numVertices * sizeof(RxVertexIndex));

    numMeshes = geometry->mesh->numMeshes;
    mesh = (RpMesh *)(geometry->mesh + 1);
    for (currentMesh = 0; currentMesh < numMeshes; currentMesh++, mesh++)
    {
        RxVertexIndex *indices = mesh->indices;
        RwUInt32 numIndices = mesh->numIndices;

        for(n = 0; n < numIndices; n++)
        {
            const RxVertexIndex index = indices[n];

            if (pOld2NewTable[index] > maxVertex)
            {
                pOld2NewTable[index] = (RxVertexIndex)maxVertex;

                maxVertex++;
            }
        }
    }

    RWASSERT(maxVertex == geometry->numVertices);

    /* Create remap table */
    pRemapTable = (RxVertexIndex *)RwMalloc(geometry->numVertices * sizeof(RxVertexIndex),
                                            rwID_CROWDPPPLUGIN | rwMEMHINTDUR_FUNCTION);

    numMeshes = geometry->mesh->numMeshes;
    mesh = (RpMesh *)(geometry->mesh + 1);
    for (currentMesh = 0; currentMesh < numMeshes; currentMesh++, mesh++)
    {
        RxVertexIndex *indices = mesh->indices;
        RwUInt32 numIndices = mesh->numIndices;

        for(n = 0; n < numIndices; n++)
        {
            const RxVertexIndex index = indices[n];

            indices[n] = pOld2NewTable[index];

            pRemapTable[indices[n]] = index;
        }

        qsort(mesh->indices,
              (mesh->numIndices / 3), (3 * sizeof(RxVertexIndex)),
              D3D9SortTriangles);
    }

    RwFree(pOld2NewTable);

    /*
     * Remap
     */

    /* Positions */
    {
        RwV3d *oldPos;
        RwV3d *newPos;
        RwUInt32 numVertices;

        oldPos = geometry->morphTarget[0].verts;

        newPos = (RwV3d *)RwMalloc(geometry->numVertices * sizeof(RwV3d),
                            rwID_CROWDPPPLUGIN | rwMEMHINTDUR_FUNCTION);

        numVertices = geometry->numVertices;
        for (n = 0; n < numVertices; n++)
        {
            newPos[n] = oldPos[pRemapTable[n]];
        }

        for (n = 0; n < numVertices; n++)
        {
            oldPos[n] = newPos[n];
        }

        RwFree(newPos);
    }

    /* Pre-lighting */
    if (geometry->preLitLum != NULL)
    {
        RwRGBA *oldColor;
        RwRGBA *newColor;
        RwUInt32 numVertices;

        oldColor = geometry->preLitLum;

        newColor = (RwRGBA *)RwMalloc(geometry->numVertices * sizeof(RwRGBA),
                                  rwID_CROWDPPPLUGIN | rwMEMHINTDUR_FUNCTION);

        numVertices = geometry->numVertices;
        for (n = 0; n < numVertices; n++)
        {
            newColor[n] = oldColor[pRemapTable[n]];
        }

        for (n = 0; n < numVertices; n++)
        {
            oldColor[n] = newColor[n];
        }

        RwFree(newColor);
    }

    /* Texture coordinates */
    if (geometry->texCoords[0] != NULL)
    {
        RwTexCoords *oldCoord;
        RwTexCoords *newCoord;
        RwUInt32 numVertices;

        oldCoord = geometry->texCoords[0];

        newCoord = (RwTexCoords *)RwMalloc(geometry->numVertices * sizeof(RwTexCoords),
                                           rwID_CROWDPPPLUGIN | rwMEMHINTDUR_FUNCTION);

        numVertices = geometry->numVertices;
        for (n = 0; n < numVertices; n++)
        {
            newCoord[n] = oldCoord[pRemapTable[n]];
        }

        for (n = 0; n < numVertices; n++)
        {
            oldCoord[n] = newCoord[n];
        }

        RwFree(newCoord);
    }

    RwFree(pRemapTable);

    RWRETURNVOID();
}

/******************************************************************************
 */
static RpCrowd *
D3D9StaticBBCreate(RpCrowd       *crowd,
                 rpCrowdStaticBB *staticBB,
                 RwUInt32       numMatPerSeq)
{
    RwUInt32    numMaterials;
    RpGeometry *crowdGeom;
    RwFrame    *frame, *baseFrame;
    RwV3d       boundingBoxPoints[8];
    RwUInt32    n;

    RWFUNCTION(RWSTRING("D3D9StaticBBCreate"));
    RWASSERT(crowd);

    /* Initialize */
    staticBB->splitTextures = (RpCrowdTexture ***)NULL;
    staticBB->materials = (RpMaterial **)NULL;
    staticBB->atomic = (RpAtomic *)NULL;
    staticBB->numMatPerSeq = numMatPerSeq;

    numMaterials = numMatPerSeq * rpCROWDANIMATIONNUMSEQ;

    /* Create material array for animation cyling */
    staticBB->materials = _rpCrowdStaticBBCreateCrowdMaterials(numMaterials);
    if (!staticBB->materials)
    {
        _rpCrowdStaticBBDestroy(crowd, staticBB);
        RWRETURN((RpCrowd *)NULL);
    }

    /* Create the crowd atomic and geometry */
    staticBB->atomic = RpAtomicCreate();
    if (!staticBB->atomic)
    {
        _rpCrowdStaticBBDestroy(crowd, staticBB);
        RWRETURN((RpCrowd *)NULL);
    }

    crowdGeom = _rpCrowdStaticBBCreateCrowdGeometry(crowd);
    if (!crowdGeom)
    {
        _rpCrowdStaticBBDestroy(crowd, staticBB);
        RWRETURN((RpCrowd *)NULL);
    }

    /* Randomly assign materials to the billboards */
    _rpCrowdStaticBBAssignCrowdMaterials(staticBB->materials, numMaterials, crowdGeom);

    /* Now that materials have been assigned we can unlock */
    RpGeometryUnlock(crowdGeom);

    /* Sort vertices and materials */
    D3D9ReorderGeometry(crowdGeom);

    /* New billboard atomic needs its own frame */
    frame = RwFrameCreate();
    if (!frame)
    {
        RpGeometryDestroy(crowdGeom);
        _rpCrowdStaticBBDestroy(crowd, staticBB);
        RWRETURN((RpCrowd *)NULL);
    }

    /* Copy the LTM from the base geometry */
    RWASSERT(crowd->param.base);
    baseFrame = RpAtomicGetFrame(crowd->param.base);
    if (baseFrame)
    {
        *RwFrameGetMatrix(frame) = *RwFrameGetLTM(baseFrame);
    }
    else
    {
        RwMatrixSetIdentity(RwFrameGetMatrix(frame));
    }

    /* Setup atomic */
    RwFrameUpdateObjects(frame);
    RpAtomicSetFrame(staticBB->atomic, frame);
    RpAtomicSetGeometry(staticBB->atomic, crowdGeom, 0);
    RpGeometryDestroy(crowdGeom);

    /* Calculate bounding box */
    CurrentCrowd = (rpD3D9CrowdStaticBB *)staticBB;

    RwBBoxCalculate(&(CurrentCrowd->boundingBox),
                    crowdGeom->morphTarget[0].verts,
                    crowdGeom->numVertices);

    for (n = 0; n < 8; n++)
    {
        boundingBoxPoints[n].x = (n & 1 ? CurrentCrowd->boundingBox.sup.x :
                                          CurrentCrowd->boundingBox.inf.x);

        boundingBoxPoints[n].y = (n & 2 ? CurrentCrowd->boundingBox.sup.y :
                                          CurrentCrowd->boundingBox.inf.y);

        boundingBoxPoints[n].z = (n & 4 ? CurrentCrowd->boundingBox.sup.z :
                                          CurrentCrowd->boundingBox.inf.z);
    }

    RwV3dTransformPoints(boundingBoxPoints,
                         boundingBoxPoints,
                         8,
                         RwFrameGetLTM(RpAtomicGetFrame(CurrentCrowd->atomic)));

    RwBBoxCalculate(&(CurrentCrowd->boundingBox),
                    boundingBoxPoints,
                    8);

    RWRETURN(crowd);
}

/******************************************************************************
 */
static RpCrowd *
D3D9SetTextures(RpCrowd *crowd, rpCrowdStaticBB *staticBB)
{
    RwReal              mapping;
    RwUInt32            cycle, i;
    RpCrowdAnimation    *anim;

    RWFUNCTION(RWSTRING("D3D9SetTextures"));
    RWASSERT(crowd);
    RWASSERT(staticBB);

    /* Get current animation */
    RWASSERT(crowd->param.anims);
    RWASSERT(crowd->currAnim < crowd->param.numAnims);

    anim = crowd->param.anims[crowd->currAnim];
    RWASSERT(anim);

    /* Get current textures */
    RWASSERT(crowd->currTexture < crowd->param.numTextures);

    /* Find the cycle number within the animation sequence */
    RWASSERT(crowd->currAnimTime >= 0.0f);
    RWASSERT(crowd->currAnimTime <= anim->duration);

    cycle = RwInt32FromRealMacro((crowd->currAnimTime / anim->duration) *
                            anim->numSteps);

    /* Set current textures on all the materials */
    mapping = (RwReal)anim->numSteps / (RwReal)staticBB->numMatPerSeq;

    for (i=0; i < staticBB->numMatPerSeq; i++)
    {
        RwInt32 step, j;

        /* Map to frame index in sequences array */
        step = RwInt32FromRealMacro(i * mapping) + cycle;
        if (step >= anim->numSteps)
        {
            step -= anim->numSteps;
        }

        /* Set material textures for current step of each sequence */
        for (j=0; j < rpCROWDANIMATIONNUMSEQ; j++)
        {
            RwInt32     iMat, iRow;

            iMat = i * rpCROWDANIMATIONNUMSEQ + j;
            iRow = anim->sequences[step][j];

            InstancedRowsToRender[iMat].row = iRow;

            RpMaterialSetTexture(staticBB->materials[iMat],
                                 crowd->param.textures[crowd->currTexture]);
        }
    }

    /* Set current number of rows */
    NumRows = crowd->param.numTexRows;

    RWRETURN(crowd);
}

/*
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *      Standard interface functions
 *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/******************************************************************************
 */
RwBool
_rpCrowdDeviceOpen(void)
{
    CrowdEngineData *engineData = CROWDENGINEGETDATA(RwEngineInstance);

    RWFUNCTION(RWSTRING("_rpCrowdDeviceOpen"));

    engineData->pipe = D3D9CrowdPipeCreate();
    if (!engineData->pipe)
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}

/******************************************************************************
 */
RwBool
_rpCrowdDeviceClose(void)
{
    CrowdEngineData *engineData = CROWDENGINEGETDATA(RwEngineInstance);

    RWFUNCTION(RWSTRING("_rpCrowdDeviceClose"));

    RxPipelineDestroy(engineData->pipe);
    engineData->pipe = (RxPipeline *)NULL;

    RWRETURN(TRUE);
}

/******************************************************************************
 */
RpCrowd *
_rpCrowdDeviceInitCrowd(RpCrowd *crowd)
{
    rpCrowdStaticBB *staticBB = (rpCrowdStaticBB *)CROWDGETDEVICEDATA(crowd);

    RWFUNCTION(RWSTRING("_rpCrowdDeviceInitCrowd"));
    RWASSERT(crowd);

    if (!D3D9StaticBBCreate(crowd, staticBB, NUMMATPERSEQ))
    {
        RWRETURN((RpCrowd *)NULL);
    }

    staticBB->atomic->pipeline = CROWDENGINEGETDATA(RwEngineInstance)->pipe;
    RWASSERT(staticBB->atomic->pipeline);

    RWRETURN(crowd);
}

/******************************************************************************
 */
RwBool
_rpCrowdDeviceDeInitCrowd(RpCrowd *crowd)
{
    rpCrowdStaticBB *staticBB = (rpCrowdStaticBB *)CROWDGETDEVICEDATA(crowd);

    RWFUNCTION(RWSTRING("_rpCrowdDeviceDeInitCrowd"));
    RWASSERT(crowd);

    _rpCrowdStaticBBDestroy(crowd, staticBB);

    RWRETURN(TRUE);
}

/******************************************************************************
 */
RpCrowd *
_rpCrowdDeviceRenderCrowd(RpCrowd *crowd)
{
    rpCrowdStaticBB *staticBB = (rpCrowdStaticBB *)CROWDGETDEVICEDATA(crowd);

    RWFUNCTION(RWSTRING("_rpCrowdDeviceRenderCrowd"));
    RWASSERT(crowd);

    CurrentCrowd = (rpD3D9CrowdStaticBB *)staticBB;

    D3D9SetTextures(crowd, staticBB);

    /* Render the atomic */
    RpAtomicRender(staticBB->atomic);

    RWRETURN(crowd);
}
