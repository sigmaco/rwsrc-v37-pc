/*
 * nodePreLight
 * Applying pre-lighting calculations to geometry
 *
 * Copyright (c) Criterion Software Limited
 */
/****************************************************************************
 *                                                                          *
 * module : nodePreLight.c                                                  *
 *                                                                          *
 * purpose: yawn...                                                         *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 includes
 */

/* #include <assert.h> */
#include <float.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rwcore.h"
#include "rpworld.h"

#include "rpdbgerr.h"

#include "p2stdclsw.h"

#include "rtgncpip.h"


/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/*****************************************************************************
 _PreLightFn
*/

static              RwBool
_PreLightFn( RxPipelineNodeInstance * self,
             const RxPipelineNodeParam * params __RWUNUSED__ )
{
    RxPacket          *packet;
    RxCluster         *clmeshstate;
    RxMeshStateVector *meshstate;

    RWFUNCTION(RWSTRING("_PreLightFn"));

    RWASSERT(NULL != self);

    packet = (RxPacket *) RxPacketFetch(self);
    RWASSERT(NULL != packet);

    clmeshstate = RxClusterLockRead(packet, 0);

    meshstate = RxClusterGetCursorData(clmeshstate, RxMeshStateVector);

    if (meshstate->ClipFlagsAnd == 0)
    {
        RxCluster          *clobjvert, *clcamvert, *clvsteps;
        RwUInt32            objVertStride;
        RxObjSpace3DVertex *objVert;
        RxCamSpace3DVertex *camVert;
        RwInt32             n = meshstate->NumVertices;

        clobjvert = RxClusterLockRead(packet, 1);
        RWASSERT(NULL != clobjvert);
        clcamvert = RxClusterLockWrite(packet, 2, self);
        RWASSERT(NULL != clcamvert);
        clvsteps = RxClusterLockRead(packet, 3);

        objVertStride = clobjvert->stride;
        objVert = RxClusterGetCursorData(clobjvert, RxObjSpace3DVertex);
        RWASSERT(NULL != objVert);
        camVert = RxClusterGetCursorData(clcamvert, RxCamSpace3DVertex);
        RWASSERT(NULL != camVert);

        if ((clvsteps != NULL) && (clvsteps->numUsed > 0))
        {
            RwUInt8            *vstep = (RwUInt8 *) clvsteps->data;

            objVert =
                (RxObjSpace3DVertex *) (((RwUInt8 *) objVert) +
                             *vstep * objVertStride);
            camVert += *vstep;

            vstep++;

            n = clvsteps->numUsed;

            while (n--)
            {
                RwRGBA objPreLitColour;
                RxObjSpace3DVertexGetPreLitColor(objVert,
                                                   &objPreLitColour);

                RxCamSpace3DVertexSetRGBA(camVert,
                                      objPreLitColour.red,
                                      objPreLitColour.green,
                                      objPreLitColour.blue,
                                      objPreLitColour.alpha);

                {
                    RwUInt32            spin = *vstep++;

                    do
                    {
                        objVert =
                            (RxObjSpace3DVertex *) (((RwUInt8 *) objVert) +
                                         objVertStride);
                        camVert++;
                    }
                    while (--spin);
                }
            }
        }
        else
        {
            n = meshstate->NumVertices;

            while (n--)
            {
                RwRGBA objPreLitColour;
                RxObjSpace3DVertexGetPreLitColor(objVert,
                                                   &objPreLitColour);

                RxCamSpace3DVertexSetRGBA(camVert,
                                      objPreLitColour.red,
                                      objPreLitColour.green,
                                      objPreLitColour.blue,
                                      objPreLitColour.alpha);

                objVert =
                    (RxObjSpace3DVertex *) (((RwUInt8 *) objVert) +
                                 objVertStride);
                camVert++;
            }
        }
    }

    /* Dispatch to 1st [only] output */
    RxPacketDispatch(packet, 0, self);

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionGetPreLight returns a pointer to a node to apply
 * pre-lighting calculations to geometry. This node initializes the
 * light-accumulation value in camera-space vertices prior to lighting.
 *
 * The node copies the pre-light color values from the object-space vertices
 * to the camera-space vertices. If the geometry is not 'pre-lit' then it is
 * assumed that these values will have been set to {0, 0, 0, 255}, for all
 * object-space vertices, by the instancing node. Otherwise, pre-lighting
 * values should have been copied from the RpGeometry or RpWorldSector into
 * the object-space vertices by the instancing node.
 *
 * This node accepts an optional RxVStep cluster. This cluster may have
 * been generated by a prior node, such as a back-face culling node. If
 * present, this cluster is used to skip vertices which belong only to
 * back-face culled triangles (per-vertex operations like lighting and
 * transformation are wasted on such vertices). See \ref RxVStep for
 * details of the use of the RxVStep struct.
 *
 * The node has one output, through which the pre-lit vertices pass.
 * The input requirements of this node:
 *      \li RxClMeshState          - required
 *      \li RxClObjSpace3DVertices - required
 *      \li RxClCamSpace3DVertices - required
 *      \li RxClVSteps             - optional
 *
 * The characteristics of this node's first output:
 *      \li RxClMeshState          - valid
 *      \li RxClObjSpace3DVertices - valid
 *      \li RxClCamSpace3DVertices - valid
 *      \li RxClVSteps             - no change
 *
 * \return pointer to a node to apply pre-lighting calculations to geometry
 *
 * \see RxNodeDefinitionGetAtomicEnumerateLights
 * \see RxNodeDefinitionGetAtomicInstance
 * \see RxNodeDefinitionGetFastPathSplitter
 * \see RxNodeDefinitionGetLight
 * \see RxNodeDefinitionGetMaterialScatter
 * \see RxNodeDefinitionGetPostLight
 * \see RxNodeDefinitionGetWorldSectorEnumerateLights
 * \see RxNodeDefinitionGetWorldSectorInstance
 */
RxNodeDefinition *
RxNodeDefinitionGetPreLight(void)
{
    static RxClusterRef PreLightCLI[] = { /* */
        {&RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClObjSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClCamSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClVSteps, rxCLALLOWABSENT, rxCLRESERVED} };

#define NUMCLUSTERSOFINTEREST \
        ((sizeof(PreLightCLI))/(sizeof(PreLightCLI[0])))

    static RxClusterValidityReq PreLightInputVR[NUMCLUSTERSOFINTEREST] = { /* */
        rxCLREQ_REQUIRED,
        rxCLREQ_REQUIRED,
        rxCLREQ_REQUIRED,
        rxCLREQ_OPTIONAL };

    static RxClusterValid PreLightOutputV[NUMCLUSTERSOFINTEREST] = { /* */
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_NOCHANGE };

    static RwChar _Output[] = RWSTRING("Output");

    static RxOutputSpec PreLightOutput[] = { /* */
        {_Output,
         PreLightOutputV,
         rxCLVALID_NOCHANGE} };

#define NUMOUTPUTS \
        ((sizeof(PreLightOutput))/(sizeof(PreLightOutput[0])))

    static RwChar _PreLight_csl[] = RWSTRING("PreLight.csl");

    static RxNodeDefinition nodePreLightCSL = { /* */
        _PreLight_csl,
        {_PreLightFn,
         (RxNodeInitFn)NULL,
         (RxNodeTermFn)NULL,
         (RxPipelineNodeInitFn)NULL,
         (RxPipelineNodeTermFn)NULL,
         (RxPipelineNodeConfigFn)NULL,
         (RxConfigMsgHandlerFn)NULL},
        {NUMCLUSTERSOFINTEREST,
         PreLightCLI,
         PreLightInputVR,
         NUMOUTPUTS,
         PreLightOutput},
        0,
        (RxNodeDefEditable)FALSE,
        0
    };

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionGetPreLight"));

    /*RWMESSAGE((RWSTRING("Pipeline II node")));*/

    RWRETURN(&nodePreLightCSL);
}

