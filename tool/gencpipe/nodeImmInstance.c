/*
 * Initialization of a packet with vertices passed in to RwIm3DTransform
 *
 * Copyright (c) Criterion Software Limited
 */

/****************************************************************************
 *                                                                          *
 * module : nodeImmInstance.c                                               *
 *                                                                          *
 * purpose: cache a transformed set of vertices for use by                  *
 *          RwIm3DRender[Indexed]Primitive()                                *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 includes
 */

#include "rwcore.h"

#include <rpdbgerr.h>

#include "rtgncpip.h"

/****************************************************************************
 local defines
 */

#define MESSAGE(_string)                                           \
    RwDebugSendMessage(rwDEBUGMESSAGE, "ImmInstance.csl", _string)

/*****************************************************************************
 ImmInstanceNode

 Take a set of RxObjSpace3DVertexes and put them in a packet with a rxCLFLAGS_EXTERNAL
 RxObjSpace3DVertex cluster pointing at them. This will be
 used by a subsequent transform node (which will produce CamVerts and DevVerts)
 and the resulting packet(s) will be output to and cached by RwIm3DTranform()
 (it's what called this pipeline, to execute up to and including the transform
 node and then stop) for use (or multiple uses) by calls to
 RwIm3DRender[Indexed]Primitive().

 on entry: Self   - the pipelineNodeInstance referencing this code.
                    Used to get all info about cluster arrangements.
           Memory - the MemoryArena from which this Node should get any memory
                    it needs
           Data   - the ImmPool which holds the verts to be transformed
 on exit : TRUE on success, FALSE on failure
*/

static              RwBool
ImmInstanceNode(RxPipelineNodeInstance * self,
                const RxPipelineNodeParam * params)
{
    rwIm3DPool         *pool;
    RxObjSpace3DVertex *vertices;
    RxPacket           *packet;

    RwRGBA              OpaqueWhite = { 255, 255, 255, 255 };
    RwUInt32            NumVerts;

    RxCluster          *ObjVerts, *MeshState, *RenderState;
    RxMeshStateVector  *MeshData;
    RxRenderStateVector *rsvp;

    RWFUNCTION(RWSTRING("ImmInstanceNode"));

    pool = (rwIm3DPool *) RxPipelineNodeParamGetData(params);
    RWASSERT(NULL != pool);

    vertices = (RxObjSpace3DVertex *) pool->elements;
    RWASSERT(NULL != vertices);
    NumVerts = pool->numElements;

    packet = RxPacketCreate(self);

    RWASSERT(NULL != packet);

    ObjVerts = RxClusterLockWrite(packet, 0, self);
    MeshState = RxClusterLockWrite(packet, 1, self);
    RenderState = RxClusterLockWrite(packet, 2, self);

    /* Create space in clusters */
    MeshState = RxClusterInitializeData(MeshState, 1,
                                        sizeof(RxMeshStateVector));
    RWASSERT(NULL != MeshState);
    RenderState = RxClusterInitializeData(RenderState, 1,
                                          sizeof(RxRenderStateVector));
    RWASSERT(NULL != RenderState);

    /* Set up the ObjVerts cluster, which references external data */
    RxClusterSetExternalData(ObjVerts, pool->elements, pool->stride,
                             NumVerts);

    /* Set up MeshState data for this packet */
    MeshData = RxClusterGetCursorData(MeshState, RxMeshStateVector);

    MeshData->Flags = rxGEOMETRY_COLORED;
    /* TODO: Crop vertices (set stride here to offsetof(RxObjSpace3DVertex, u)) if no RxUVs? */
    MeshData->Flags |=
        (pool->stash.flags & rwIM3D_VERTEXUV) ? (rxGEOMETRY_TEXTURED) : (0);
    MeshData->SourceObject = (void *) pool;
    /* Set up the Local to Camera matrix for this Im3D vertex pool */
    if (pool->stash.ltm)
    {
        RwMatrixCopy(&MeshData->Obj2Cam, pool->stash.ltm);
        /* Following set up during RwCameraBeginUpdate */
        RwMatrixTransform(&MeshData->Obj2Cam,
                          &(((RwCamera *)
                             RWSRCGLOBAL
                             (curCamera))->viewMatrix), rwCOMBINEPOSTCONCAT);
        RwMatrixCopy(&MeshData->Obj2World, pool->stash.ltm);
    }
    else
    {
        RwMatrixCopy(&MeshData->Obj2Cam,
                     &(((RwCamera *) RWSRCGLOBAL(curCamera))->viewMatrix));
        RwMatrixSetIdentity(&MeshData->Obj2World);
    }
    MeshData->SurfaceProperties.ambient = ((RwReal)1);
    MeshData->SurfaceProperties.diffuse = ((RwReal)1);
    MeshData->SurfaceProperties.specular = ((RwReal)1);
    MeshData->Texture = (RwTexture *)NULL;
    MeshData->MatCol = OpaqueWhite;
    MeshData->Pipeline = (RxPipeline *)NULL; /* the im3DRenderPipeline is specified later */
    MeshData->PrimType = rwPRIMTYPENAPRIMTYPE; /* Not defined till the RenderPipeline */
    MeshData->NumElements = 0;
    MeshData->NumVertices = NumVerts;
    MeshData->ClipFlagsAnd = 0;
    MeshData->ClipFlagsOr = 0;

#ifdef RWMETRICS
    RWSRCGLOBAL(metrics)->numVertices += NumVerts;
#endif /* RWMETRICS */

    MeshState->numUsed++;

    /* Set up RenderState data for this packet */
    rsvp = RxClusterGetCursorData(RenderState, RxRenderStateVector);
    *rsvp = RXPIPELINEGLOBAL(defaultRenderState);
    rsvp->Flags |=
        (pool->stash.flags & rwIM3D_ALLOPAQUE) ? (0)
        : (rxRENDERSTATEFLAG_VERTEXALPHAENABLE);
    RenderState->numUsed++;

    /* Ready for next mesh */
    RxPacketDispatch(packet, 0, self);

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionGetImmInstance returns a pointer to a node
 * to initialize a packet with vertices passed in to RwIm3DTransform
 *
 * This node creates and initializes a packet such that its
 * RxObjSpace3DVertex cluster points to the vertices passed in to
 * RwIm3DTransform. It also sets up an appropriate mesh state cluster.
 *
 * The object-space vertices are obtained through the (void *)data pointer
 * of the node. RwIm3DTransform passes in a pointer to a rwIm3DPool struct
 * through this parameter. This struct contains the number of elements
 * (vertices), a pointer to the vertex array, the stride of said array,
 * the object-to-world transformation matrix of the vertices (if they have
 * one) and flags determining whether UV values need be present in the
 * vertices.
 *
 * The node has one output, through which initialized packets pass
 * The input requirements of this node:
 *
 * \verbatim
   RxClObjSpace3DVertices - don't want
   RxClMeshState          - don't want
   RxClRenderState        - don't want
   \endverbatim
 *
 * The characteristics of this node's first output:
 *
 * \verbatim
   RxClObjSpace3DVertices - valid
   RxClMeshState          - valid
   RxClRenderState        - valid
   \endverbatim
 *
 * \return A pointer to the node to initialize a packet with vertices passed in
 * to RwIm3DTransform on success, or NULL if there is an error
 *
 * \see RxNodeDefinitionGetClipLine
 * \see RxNodeDefinitionGetClipTriangle
 * \see RxNodeDefinitionGetCullTriangle
 * \see RxNodeDefinitionGetImmMangleLineIndices
 * \see RxNodeDefinitionGetImmMangleTriangleIndices
 * \see RxNodeDefinitionGetImmRenderSetup
 * \see RxNodeDefinitionGetScatter
 * \see RxNodeDefinitionGetSubmitLine
 * \see RxNodeDefinitionGetSubmitTriangle
 * \see RxNodeDefinitionGetTransform
 * \see RxNodeDefinitionGetUVInterp
 *
 */

RxNodeDefinition   *
RxNodeDefinitionGetImmInstance(void)
{
    static RxClusterRef N1clofinterest[] = { /* */
        {&RxClObjSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClRenderState, rxCLALLOWABSENT, rxCLRESERVED}
    };

#define NUMCLUSTERSOFINTEREST \
        ((sizeof(N1clofinterest))/(sizeof(N1clofinterest[0])))

    static RxClusterValidityReq N1inputreqs[NUMCLUSTERSOFINTEREST] = { /* */
        rxCLREQ_DONTWANT,
        rxCLREQ_DONTWANT,
        rxCLREQ_DONTWANT
    };

    static RxClusterValid N1outcl1[NUMCLUSTERSOFINTEREST] = { /* */
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_VALID
    };

    static RwChar       _ImmInstanceOut[] = RWSTRING("ImmInstanceOut");

    static RxOutputSpec N1outputs[] = { /* */
        {_ImmInstanceOut,
         N1outcl1,
         rxCLVALID_NOCHANGE}
    };

#define NUMOUTPUTS \
        ((sizeof(N1outputs))/(sizeof(N1outputs[0])))

    static RwChar       _ImmInstance_csl[] = RWSTRING("ImmInstance.csl");

    static RxNodeDefinition nodeImmInstanceCSL = {
        _ImmInstance_csl,
        {ImmInstanceNode,
         (RxNodeInitFn)NULL,
         (RxNodeTermFn)NULL,
         (RxPipelineNodeInitFn)NULL,
         (RxPipelineNodeTermFn)NULL,
         (RxPipelineNodeConfigFn)NULL,
         (RxConfigMsgHandlerFn)NULL},
        {NUMCLUSTERSOFINTEREST,
         N1clofinterest,
         N1inputreqs,
         NUMOUTPUTS,
         N1outputs},
        0,
        (RxNodeDefEditable) FALSE,
        0
    };

    RxNodeDefinition   *result = &nodeImmInstanceCSL;

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionGetImmInstance"));

    /*RWMESSAGE((RWSTRING("result %p"), result)); */

    RWRETURN(result);
}
