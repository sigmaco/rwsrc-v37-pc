/*
 * nodePostLight
 * Applying post-lighting calculations to geometry
 *
 * Copyright (c) Criterion Software Limited
 */
/****************************************************************************
 *                                                                          *
 * module : nodePostLight.c                                                 *
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
 _PostLightFn
*/

#define CLAMPCOLORCOMPONENT(c)                    \
  MACRO_START                                     \
  {                                               \
      if ( *(RwUInt32 *) &(c) >                   \
           *(const RwUInt32 *) &maxlum )          \
      {                                           \
          *(RwUInt32 *) &(c) =                    \
              ( *(const RwInt32 *) &(c) < 0 ) ?   \
              0U: *(const RwUInt32 *) &maxlum;    \
      }                                           \
  }                                               \
  MACRO_STOP

static              RwBool
_PostLightFn( RxPipelineNodeInstance * self,
              const RxPipelineNodeParam * params __RWUNUSED__)
{
    const RwReal       maxlum = (RwReal) 255;
    RxPacket          *packet;
    RxCluster         *clmeshstate;
    RxMeshStateVector *meshstate;

    RWFUNCTION(RWSTRING("_PostLightFn"));

    RWASSERT(NULL != self);

    packet = (RxPacket *)RxPacketFetch(self);
    RWASSERT(NULL != packet);

    clmeshstate = RxClusterLockRead(packet, 0);

    meshstate = RxClusterGetCursorData(clmeshstate, RxMeshStateVector);

    if (meshstate->ClipFlagsAnd == 0)
    {
        RxCluster          *clcamvert, *cldevvert, *clvsteps;
        RxCamSpace3DVertex *camVert;
        RxScrSpace2DVertex *devVert;
        RwUInt8            *vstep;
        RwUInt8             fakeVStep = 1;
        RwUInt32            vstepInc;
        RwRGBA              MatCol;
        RwInt32             n;
        RwUInt32            spin;
        RwRGBAReal         *col;

        MatCol = meshstate->MatCol;
        n = meshstate->NumVertices;

        clcamvert = RxClusterLockWrite(packet, 1, self);
        RWASSERT((NULL != clcamvert) && (clcamvert->numUsed > 0));
        cldevvert = RxClusterLockWrite(packet, 2, self);
        RWASSERT((NULL != cldevvert) && (cldevvert->numUsed > 0));
        clvsteps  = RxClusterLockRead( packet, 3);

        if ((clvsteps != NULL) && (clvsteps->numUsed > 0))
        {
            vstep = (RwUInt8 *) clvsteps->data;
            vstepInc = 1;
            n = clvsteps->numUsed;

            spin = *vstep;
            while (spin--)
            {
                RxClusterIncCursor(clcamvert);
                RxClusterIncCursor(cldevvert);
            }

            vstep++;
        }
        else
        {
            vstep = &fakeVStep;
            vstepInc = 0;
            n = meshstate->NumVertices;
        }

        if ((!(meshstate->Flags & rxGEOMETRY_MODULATE))   ||
            (*(RwUInt32 *)&MatCol == (RwUInt32)0xffffffff)  )
        {
            /* opaque white */
            while (n--)
            {
                camVert = RxClusterGetCursorData(clcamvert,
                                                 RxCamSpace3DVertex);
                devVert = RxClusterGetCursorData(cldevvert,
                                                 RxScrSpace2DVertex);
                col = &camVert->col;

                CLAMPCOLORCOMPONENT(col->red);
                CLAMPCOLORCOMPONENT(col->green);
                CLAMPCOLORCOMPONENT(col->blue);

                RwIm2DVertexSetRealRGBA(devVert,
                                        col->red, col->green,
                                        col->blue, col->alpha);

                spin = *vstep;
                vstep += vstepInc;

                while (spin--)
                {
                    RxClusterIncCursor(clcamvert);
                    RxClusterIncCursor(cldevvert);
                }
            }
        }
        else
        {
            /* not opaque white */
            RwRGBAReal          matcol;

            RwRGBARealFromRwRGBA(&matcol, &MatCol);

            while (n--)
            {
                camVert = RxClusterGetCursorData(clcamvert,
                                                 RxCamSpace3DVertex);
                devVert = RxClusterGetCursorData(cldevvert,
                                                 RxScrSpace2DVertex);
                col = &camVert->col;

                col->red   *= matcol.red;
                col->green *= matcol.green;
                col->blue  *= matcol.blue;
                col->alpha *= matcol.alpha;

                CLAMPCOLORCOMPONENT(col->red);
                CLAMPCOLORCOMPONENT(col->green);
                CLAMPCOLORCOMPONENT(col->blue);
                CLAMPCOLORCOMPONENT(col->alpha);

                RwIm2DVertexSetRealRGBA(devVert,
                                        col->red, col->green,
                                        col->blue, col->alpha);

                spin = *vstep;
                vstep += vstepInc;

                while (spin--)
                {
                    RxClusterIncCursor(clcamvert);
                    RxClusterIncCursor(cldevvert);
                }
            }
        }
    }

    /* Dispatch to 1st [only] output */
    RxPacketDispatch(packet, 0, self);

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionGetPostLight returns a pointer to a node
 * to apply post-lighting calculations to geometry
 *
 * This node first of all clamps color values in the camera-space vertices to the
 * range [0,255] and then copies these values into the screen-space vertices.
 * If the material color of the geometry is not {255, 255, 255, 255} then
 * the lighting value for each vertex is multiplied by the material color
 * (normalized by 1/255) before the clamping and copying is performed. Note
 * that the camera-space vertices contain a RwRGBAReal struct for the
 * accumulation of light values and that unlike elsewhere in RW, the values
 * in this struct are meant to be in the range [0,255] and not [0,1].
 *
 * This node accepts an optional RxVStep cluster. This cluster may have
 * been generated by a prior node, such as a back-face culling node. If
 * present, this cluster is used to skip vertices which belong only to
 * back-face culled triangles (per-vertex operations like lighting and
 * transformation are wasted on such vertices). See \ref RxVStep for
 * details of the use of the RxVStep struct.
 *
 * The node has one output, through which the post-lit vertices pass.
 * The input requirements of this node:
 *      \li RxClMeshState          - required
 *      \li RxClCamSpace3DVertices - required
 *      \li RxClScrSpace3DVertices - required
 *      \li RxClVSteps             - optional
 *
 * The characteristics of this node's first output:
 *      \li RxClMeshState          - no change
 *      \li RxClCamSpace3DVertices - valid
 *      \li RxClScrSpace3DVertices - valid
 *      \li RxClVSteps             - no change
 *
 * \return pointer to a node to apply post-lighting calculations to geometry
 *
 * \see RxNodeDefinitionGetAtomicEnumerateLights
 * \see RxNodeDefinitionGetAtomicInstance
 * \see RxNodeDefinitionGetFastPathSplitter
 * \see RxNodeDefinitionGetLight
 * \see RxNodeDefinitionGetMaterialScatter
 * \see RxNodeDefinitionGetPreLight
 * \see RxNodeDefinitionGetWorldSectorEnumerateLights
 * \see RxNodeDefinitionGetWorldSectorInstance
 */
RxNodeDefinition *
RxNodeDefinitionGetPostLight(void)
{

    static RxClusterRef PostLightCLI[] = { /* */
      {&RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED},
      {&RxClCamSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED},
      {&RxClScrSpace2DVertices, rxCLALLOWABSENT, rxCLRESERVED},
      {&RxClVSteps, rxCLALLOWABSENT, rxCLRESERVED} };

    #define NUMCLUSTERSOFINTEREST \
        ((sizeof(PostLightCLI))/(sizeof(PostLightCLI[0])))

    static RxClusterValidityReq PostLightInputVR[NUMCLUSTERSOFINTEREST] = { /* */
      rxCLREQ_REQUIRED,
      rxCLREQ_REQUIRED,
      rxCLREQ_REQUIRED,
      rxCLREQ_OPTIONAL };

    static RxClusterValid PostLightOutputV[NUMCLUSTERSOFINTEREST] = { /* */
     rxCLVALID_NOCHANGE,
     rxCLVALID_VALID,
     rxCLVALID_VALID,
     rxCLVALID_NOCHANGE };

    static RwChar _Output[] = RWSTRING("Output");

    static RxOutputSpec PostLightOutput[] = { /* */
      {_Output,
       PostLightOutputV,
       rxCLVALID_NOCHANGE} };

    #define NUMOUTPUTS \
        ((sizeof(PostLightOutput))/(sizeof(PostLightOutput[0])))

    static RwChar _PostLight_csl[] = RWSTRING("PostLight.csl");

    static RxNodeDefinition nodePostLightCSL = { /* */
        _PostLight_csl,
        {_PostLightFn,
         (RxNodeInitFn)NULL,
         (RxNodeTermFn)NULL,
         (RxPipelineNodeInitFn)NULL,
         (RxPipelineNodeTermFn)NULL,
         (RxPipelineNodeConfigFn)NULL,
         (RxConfigMsgHandlerFn)NULL },
        {NUMCLUSTERSOFINTEREST,
         PostLightCLI,
         PostLightInputVR,
         NUMOUTPUTS,
         PostLightOutput},
        0,
        (RxNodeDefEditable)FALSE,
        0 };

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionGetPostLight"));

    /*RWMESSAGE((RWSTRING("Pipeline II node")));*/

    RWRETURN(&nodePostLightCSL);
}

