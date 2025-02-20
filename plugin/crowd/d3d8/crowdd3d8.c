/*
 *  crowdd3d8.c
 *
 *  D3D8 implementation of RpCrowd
 *
 *  Uses the static billboard system (see crowdstatic.c|h).
 */

#include <d3d8.h>

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
struct _rpD3D8CrowdStaticBB
{
    RpAtomic           *atomic;
    RpCrowdTexture   ***splitTextures;  /* Array of Arrays of texture pointers. */
    RpMaterial        **materials;      /* Crowd Materials */
    RwUInt32            numMatPerSeq;
    RwBBox              boundingBox;
};
typedef struct _rpD3D8CrowdStaticBB rpD3D8CrowdStaticBB;

typedef struct _InstancedRow
{
    RxD3D8InstanceData *mesh;
    RwUInt32 row;

}   InstancedRow;

/******************************************************************************
 *  Global variables
 */

/* RpCrowd device extension data */
RwUInt32 _rpCrowdDeviceDataSize = sizeof(rpD3D8CrowdStaticBB);

/******************************************************************************
 *  Local variables
 */
static InstancedRow InstancedRowsToRender[NUMCROWDMATERIALS];
static RwUInt32     NumRows;

static rpD3D8CrowdStaticBB *CurrentCrowd = NULL;

/*
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *      Internal Functions
 *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/****************************************************************************
 D3D8SortTriangles

 Purpose:   Sort the triangles indices.

 On entry:  indices     - Pointer to an array of indices
            numIndices  - Number of indices
*/
static int
D3D8SortTriangles(const void *pA, const void *pB)
{
    const RxVertexIndex *pTA = (const RxVertexIndex *)pA;
    const RxVertexIndex *pTB = (const RxVertexIndex *)pB;

    RwInt32 sortedIndexA[3];
    RwInt32 sortedIndexB[3];
    RwInt32 temp;


    RWFUNCTION( RWSTRING( "D3D8SortTriangles" ) );

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
 *  D3D8CrowdFrustumTestBBox
 */
static RwFrustumTestResult
D3D8CrowdFrustumTestBBox(const RwCamera *camera,
                         const void *boundingBox)
{
    const RwV3d          *vect;
    const RwFrustumPlane *frustumPlane;
    RwInt32              numPlanes;
    RwFrustumTestResult  testResult;
    RwReal               distance;

    RWFUNCTION(RWSTRING("D3D8CrowdFrustumTestBBox"));

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
 *  D3D8InstancedDataMatSortCB
 */
static int
D3D8InstancedDataMatSortCB(const void *data1, const void *data2)
{
    const InstancedRow *elem1;
    const InstancedRow *elem2;
    int value;

    RWFUNCTION(RWSTRING("D3D8InstancedDataMatSortCB"));

    elem1 = (const InstancedRow *)data1;
    elem2 = (const InstancedRow *)data2;

    /* Sort by texture */
    value = ( (RwInt32)(elem1->row) -
              (RwInt32)(elem2->row) );

    if (!value)
    {
        /* Sort by vertex buffer */
        value = ( (RwInt32)(elem1->mesh->vertexBuffer) -
                  (RwInt32)(elem2->mesh->vertexBuffer) );

        if (!value)
        {
            /* Sort by vertex buffer position */
            value = ( (RwInt32)(elem1->mesh->baseIndex) -
                      (RwInt32)(elem2->mesh->baseIndex) );
        }
    }

    RWRETURN(value);
}

/****************************************************************************
 D3D8CrowdRenderCallback
 */
void
D3D8CrowdRenderCallback(RwResEntry *repEntry,
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
    RxD3D8ResEntryHeader    *resEntryHeader;
    RxD3D8InstanceData      *instancedData;
    RwInt32                 numMeshes;
    RwBool                  vertexAlphaBlend;
    RwUInt32                oldAlphaRef;
    void                    *lastVertexBuffer;
    RwUInt32                lastRow;
    RwReal                  invNumRows;
    RwInt32                 n;

    RWFUNCTION(RWSTRING("D3D8CrowdRenderCallback"));

    /* Enable clipping if needed */
    cam = RwCameraGetCurrentCamera();
    RWASSERT(cam);

    testResult = D3D8CrowdFrustumTestBBox(cam, &(CurrentCrowd->boundingBox));

    if (testResult == rwSPHEREINSIDE)
    {
        RwD3D8SetRenderState(D3DRS_CLIPPING, FALSE);
    }
    else if (testResult == rwSPHEREBOUNDARY)
    {
        RwD3D8SetRenderState(D3DRS_CLIPPING, TRUE);
    }
    else
    {
        RWRETURNVOID();
    }

    /* Get vertex alpha Blend state */
    RwD3D8GetRenderState(D3DRS_ALPHABLENDENABLE, &vertexAlphaBlend);

    /* let's pretend to be a PS2 */
    RwD3D8GetRenderState(D3DRS_ALPHAREF, &oldAlphaRef);
    RwD3D8SetRenderState(D3DRS_ALPHAREF, 128);

    /* Set Last to force the call */
    lastVertexBuffer = (void *)0xffffffff;
    lastRow = 0xffffffff;

    /* Get the instanced data */
    resEntryHeader = (RxD3D8ResEntryHeader *)(repEntry + 1);
    instancedData = (RxD3D8InstanceData *)(resEntryHeader + 1);

    /*
     * Data shared between meshes
     */

    /*
     * Set the Default Pixel shader
     */
    RwD3D8SetPixelShader(0);

    /*
     * Vertex shader
     */
    RwD3D8SetVertexShader(instancedData->vertexShader);

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
          D3D8InstancedDataMatSortCB);

    /*
     * Render
     */
    RwD3D8SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS,
                                  D3DTTFF_COUNT2);

    for(n = 0; n < numMeshes; n++)
    {
        instancedData = InstancedRowsToRender[n].mesh;

        if (n == 0)
        {
            /* Set the texture */
            RwD3D8SetTexture(instancedData->material->texture, 0);

            /* Disable blending, it's set because the texture has an alpha channel */
            RwD3D8SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        }

        /* Set vertex alpha blend */
        if (instancedData->vertexAlpha)
        {
            if (!vertexAlphaBlend)
            {
                vertexAlphaBlend = TRUE;

                /* We need to force it again */
                RwD3D8SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
                RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE);

                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)rwBLENDSRCALPHA);
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)rwBLENDINVSRCALPHA);
            }
        }
        else
        {
            if (vertexAlphaBlend)
            {
                vertexAlphaBlend = FALSE;

                /* Disable blending, it's set because the texture has an alpha channel */
                RwD3D8SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
                RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)FALSE);
            }
        }

        if (lastRow != InstancedRowsToRender[n].row)
        {
            texMat._22 = invNumRows;
            texMat._32 = ((RwReal)InstancedRowsToRender[n].row) * invNumRows;

            RwD3D8SetTransform(D3DTS_TEXTURE0, &texMat);

            lastRow = InstancedRowsToRender[n].row;
        }

        /* Set the stream source */
        if (lastVertexBuffer != instancedData->vertexBuffer)
        {
            RwD3D8SetStreamSource(0, instancedData->vertexBuffer, instancedData->stride);

            lastVertexBuffer = instancedData->vertexBuffer;
        }

        /* Set the Index buffer */
        RwD3D8SetIndices(instancedData->indexBuffer, instancedData->baseIndex);

        /* Draw the indexed primitive */
        RwD3D8DrawIndexedPrimitive((D3DPRIMITIVETYPE)instancedData->primType,
                                   0, instancedData->numVertices,
                                   0, instancedData->numIndices);
    }

    RwD3D8SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS,
                                  D3DTTFF_DISABLE);

    /* We need to force it again */
    RwD3D8SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE);

    RwD3D8SetRenderState(D3DRS_ALPHAREF, oldAlphaRef);

    RWRETURNVOID();
}

/****************************************************************************
 *  D3D8CrowdPipeCreate
 */
static RxPipeline *
D3D8CrowdPipeCreate(void)
{
    RxPipeline  *pipe;

    RWFUNCTION(RWSTRING("D3D8CrowdPipeCreate"));

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
            instanceNode = RxNodeDefinitionGetD3D8AtomicAllInOne();
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
            RxD3D8AllInOneSetRenderCallBack(node, D3D8CrowdRenderCallback);

            RWRETURN(pipe);
        }

        RxPipelineDestroy(pipe);
    }

    RWRETURN(NULL);
}

/******************************************************************************
 */
static RpCrowd *
D3D8StaticBBCreate(RpCrowd       *crowd,
                 rpCrowdStaticBB *staticBB,
                 RwUInt32       numMatPerSeq)
{
    RwUInt32    numMaterials;
    RpGeometry *crowdGeom;
    RwFrame    *frame, *baseFrame;
    RwUInt32    numMeshes, n;
    RpMesh     *mesh;
    RwV3d       boundingBoxPoints[8];

    RWFUNCTION(RWSTRING("D3D8StaticBBCreate"));
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

    /* Sort tri list */
    numMeshes = crowdGeom->mesh->numMeshes;
    mesh = (RpMesh *)(crowdGeom->mesh + 1);
    for (n = 0; n < numMeshes; n++, mesh++)
    {
        qsort(mesh->indices,
              (mesh->numIndices / 3), (3 * sizeof(RxVertexIndex)),
              D3D8SortTriangles);
    }

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
    CurrentCrowd = (rpD3D8CrowdStaticBB *)staticBB;

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
D3D8SetTextures(RpCrowd *crowd, rpCrowdStaticBB *staticBB)
{
    RwReal              mapping;
    RwUInt32            cycle, i;
    RpCrowdAnimation    *anim;

    RWFUNCTION(RWSTRING("D3D8SetTextures"));
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

    engineData->pipe = D3D8CrowdPipeCreate();
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

    if (!D3D8StaticBBCreate(crowd, staticBB, NUMMATPERSEQ))
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

    CurrentCrowd = (rpD3D8CrowdStaticBB *)staticBB;

    D3D8SetTextures(crowd, staticBB);

    /* Render the atomic */
    RpAtomicRender(staticBB->atomic);

    RWRETURN(crowd);
}
