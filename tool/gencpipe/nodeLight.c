/*
 * nodeLight
 * Lighting world geometry
 *
 * Copyright (c) Criterion Software Limited
 */
/****************************************************************************
 *                                                                          *
 *  Module  :   nodeLight.c                                                 *
 *                                                                          *
 *  Purpose :   Yawn...                                                     *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
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

/****************************************************************************
 Local defines
 */

#define MESSAGE(_string)                                        \
    RwDebugSendMessage(rwDEBUGMESSAGE, "Light.csl", _string)

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   Functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/*****************************************************************************/

#define VERTSARG RxPipelineNodeInstance *self, RxPacket *packet

#define OBJCAMVERTDECL                                          \
    RxCluster *clobjvert, *clcamvert, *clvsteps;                \
    RwUInt32   objVertStride;                                   \
    RxObjSpace3DVertex *objVert;                                \
    RxCamSpace3DVertex *camVert;                                \
    RwUInt8   *vstep                                            \
/*    RwV3d      objNormal, objVertex;                     \*/

#define CAMVERTDECL                             \
    RxCluster *clcamvert, *clvsteps;            \
    RxCamSpace3DVertex *camVert;                \
    RwUInt8 *vstep

#define NUMVERTDECL RwInt32 numVert

#define OBJCAMVERTINIT()                                                \
  MACRO_START                                                           \
  {                                                                     \
      clobjvert = RxClusterLockRead (packet, 1);                        \
      clcamvert = RxClusterLockWrite(packet, 2, self);                  \
      clvsteps  = RxClusterLockRead (packet, 4);                        \
      objVertStride = clobjvert->stride;                                \
      objVert = RxClusterGetCursorData(clobjvert, RxObjSpace3DVertex);  \
      camVert = RxClusterGetCursorData(clcamvert, RxCamSpace3DVertex);  \
      if ( clvsteps != (RxCluster *)NULL &&                             \
           (clvsteps->flags & rxCLFLAGS_CLUSTERVALID) )                 \
      {                                                                 \
          vstep = (RwUInt8 *) clvsteps->data;                           \
          numVert = clvsteps->numUsed;                                  \
                                                                        \
          objVert =                                                     \
              (RxObjSpace3DVertex *) (((RwUInt8 *) objVert) +           \
                           *vstep * objVertStride);                     \
          camVert += *vstep;                                            \
                                                                        \
          vstep++;                                                      \
      }                                                                 \
      else                                                              \
      {                                                                 \
          vstep = (RwUInt8 *)NULL;                                      \
          numVert = clcamvert->numUsed;                                 \
      }                                                                 \
  }                                                                     \
  MACRO_STOP

#define CAMVERTINIT()                                                                           \
  MACRO_START                                                                                   \
  {                                                                                             \
      clcamvert = RxClusterLockWrite(packet, 2, self);                                          \
      clvsteps  = RxClusterLockRead (packet, 4);                                                \
      camVert = RxClusterGetCursorData(clcamvert, RxCamSpace3DVertex);                          \
      if ( clvsteps != (RxCluster *)NULL && (clvsteps->flags & rxCLFLAGS_CLUSTERVALID) )        \
      {                                                                                         \
          vstep = (RwUInt8 *) clvsteps->data;                                                   \
          numVert = clvsteps->numUsed;                                                          \
                                                                                                \
          camVert += *vstep;                                                                    \
                                                                                                \
          vstep++;                                                                              \
      }                                                                                         \
      else                                                                                      \
      {                                                                                         \
          vstep = (RwUInt8 *)NULL;                                                              \
          numVert = clcamvert->numUsed;                                                         \
      }                                                                                         \
  }                                                                                             \
  MACRO_STOP

#define NUMVERTINIT()

#define OBJCAMVERTINC()                                         \
  MACRO_START                                                   \
  {                                                             \
      RwUInt32 spin = 1;                                        \
                                                                \
      if ( vstep != (RwUInt8 *)NULL )                           \
      {                                                         \
          spin = *vstep++;                                      \
      }                                                         \
                                                                \
      do {                                                      \
          objVert = (RxObjSpace3DVertex *)                      \
                      (((RwUInt8 *) objVert) + objVertStride);  \
          camVert++;                                            \
      } while ( --spin );                                       \
  }                                                             \
  MACRO_STOP

#define CAMVERTINC()                            \
  MACRO_START                                   \
  {                                             \
      if ( vstep != (RwUInt8 *)NULL )           \
      {                                         \
          camVert += *vstep++;                  \
      }                                         \
      else                                      \
      {                                         \
          camVert++;                            \
      }                                         \
  }                                             \
  MACRO_STOP

/* Note the cunningness used below to get a pointer to ObjVert normal/pos
   whilst working within the constraints of the abstracted ObjVert macros: */
#define OBJVERTGETNORMAL(vector)                \
  RxObjSpace3DVertexGetNormal(objVert, vector)

#define OBJVERTGETPOS(vector)                   \
  RxObjSpace3DVertexGetPos(objVert, vector)

#define CAMVERTADDRGBA(r, g, b, a)              \
  MACRO_START                                   \
  {                                             \
      camVert->col.red   += (lum*r);            \
      camVert->col.green += (lum*g);            \
      camVert->col.blue  += (lum*b);            \
      camVert->col.alpha += (lum*a);            \
  }                                             \
  MACRO_STOP

/*
 * you're gonna need to add \rw3\rwsdk\world to
 * the project's include directories
 */

#include "baaplylt.c" /* this is *not* the RenderWare way! */

typedef void  (*ApplyLightFn) (VERTSARG, const void *voidLight, const RwMatrix * inverseMat, RwReal invScale, const RwSurfaceProperties * surfaceProps);

/*****************************************************************************
 _PacketApplyLight

 -

 on entry: -
 on exit : -
*/

static void
_PacketApplyLight(RxPipelineNodeInstance *self,
                  RxPacket * packet, RpLight * light,
                  const RwMatrix * inverse, RwReal invscale,
                  const RwSurfaceProperties * surfaceprops)
{
    RWFUNCTION(RWSTRING("_PacketApplyLight"));

    /* this because the Pipeline II/RenderWare integration is
     * proceeding incrementally: */

    switch (RpLightGetType(light))
    {
        case rpLIGHTAMBIENT:
            {
                _rwApplyAmbientLight(self, packet, (void *) light, inverse,
                                     invscale, surfaceprops);
                break;
            }
        case rpLIGHTDIRECTIONAL:
            {
                _rwApplyDirectionalLight(self, packet, (void *) light,
                                         inverse, invscale,
                                         surfaceprops);
                break;
            }
        case rpLIGHTPOINT:
            {
                _rwApplyPointLight(self, packet, (void *) light, inverse,
                                   invscale, surfaceprops);
                break;
            }
        case rpLIGHTSPOT:
            {
                _rwApplySpotLight(self, packet, (void *) light, inverse,
                                  invscale, surfaceprops);
                break;
            }
        case rpLIGHTSPOTSOFT:
            {
                _rwApplySpotSoftLight(self, packet, (void *) light, inverse,
                                      invscale, surfaceprops);
                break;
            }
        default:
            {
                break;
            }
    }

    RWRETURNVOID();
}

/*****************************************************************************
 _LightFn

 -

 on entry: -
 on exit : -
*/

static              RwBool
_LightFn( RxPipelineNodeInstance * self,
          const RxPipelineNodeParam * params __RWUNUSED__ )
{
    RxPacket          *packet;
    RxCluster         *clmeshstate;
    RxMeshStateVector *meshstate;

    RWFUNCTION(RWSTRING("_LightFn"));

    RWASSERT(NULL != self);

    packet = (RxPacket *)RxPacketFetch(self);
    RWASSERT(NULL != packet);

    clmeshstate = RxClusterLockRead(packet, 0);
    RWASSERT((clmeshstate != NULL) && (clmeshstate->numUsed > 0));

    meshstate = RxClusterGetCursorData(clmeshstate, RxMeshStateVector);

    if (meshstate->ClipFlagsAnd == 0)
    {
        RxCluster *cllights;
        RwUInt32   n;

        cllights = RxClusterLockRead(packet, 3);
        RWASSERT(NULL != cllights);

        n = cllights->numUsed;

        if (n > 0)
        {
             /* okay, so there *are* some lights */
            RwMatrix           *inverse;
            RwReal              invscale;

            if (meshstate->Obj2World.flags & rwMATRIXINTERNALIDENTITY)
            {
                inverse = (RwMatrix *)NULL;
                invscale = ((RwReal) 1);
            }
            else
            {
                inverse = RwMatrixCreate();
                RwMatrixInvert(inverse, &meshstate->Obj2World);
                invscale = RwV3dDotProduct(&inverse->at, &inverse->at);
                rwSqrt(&invscale, invscale);

            }

            do
            {      /* loop over lights */
                _PacketApplyLight(self, packet,
                                  *RxClusterGetCursorData(cllights, RxLight),
                                  inverse, invscale,
                                  &meshstate->SurfaceProperties);
            }
            while (RxClusterIncCursor(cllights), --n);

            if (inverse != NULL)
            {
                RwMatrixDestroy(inverse);
            }
        }
    }

    /* Dispatch to 1st [only] output */
    RxPacketDispatch(packet, 0, self);

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionGetLight returns a pointer to a node which lights
 * geometry with any relevant (i.e nearby) RpLights in the world,
 * accumulating lighting values in the camera-space vertices.
 *
 * For every light in the RxLights array (the RxLight struct merely holds a
 * pointer to an RpLight), the appropriate lighting function (ambient, point,
 * etc.) is applied for all vertices in the packet. If either the mesh
 * state's ClipFlagsAnd is non-zero or there are no lights in the RxLight
 * cluster, then the packet is passed on unmodified. Lights are transformed
 * into object-space so that lighting can be performed as efficiently as
 * possible (vertex normals need not be transformed).
 *
 * This node accepts an optional RxVStep cluster. This cluster may have
 * been generated by a prior node, such as a back-face culling node. If
 * present, this cluster is used to skip vertices which belong only to
 * back-face culled triangles (per-vertex operations like lighting and
 * transformation are wasted on such vertices). See \ref RxVStep for
 * details of the use of the RxVStep struct.
 *
 * The node has one output, through which the lit vertices pass.
 * The input requirements of this node:
 *      \li RxClMeshState          - required
 *      \li RxClObjSpace3DVertices - required
 *      \li RxClCamSpace3DVertices - required
 *      \li RxClLights             - optional
 *      \li RxClVSteps             - optional
 *
 * The characteristics of this node's first output:
 *      \li RxClMeshState          - no change
 *      \li RxClObjSpace3DVertices - no change
 *      \li RxClCamSpace3DVertices - valid
 *      \li RxClLights             - no change
 *      \li RxClVSteps             - no change
 *
 * \return pointer to a node to light geometry
 *
 * \see RxNodeDefinitionGetAtomicEnumerateLights
 * \see RxNodeDefinitionGetAtomicInstance
 * \see RxNodeDefinitionGetFastPathSplitter
 * \see RxNodeDefinitionGetMaterialScatter
 * \see RxNodeDefinitionGetPostLight
 * \see RxNodeDefinitionGetPreLight
 * \see RxNodeDefinitionGetWorldSectorEnumerateLights
 * \see RxNodeDefinitionGetWorldSectorInstance
 */
RxNodeDefinition *
RxNodeDefinitionGetLight(void)
{
    static RxClusterRef LightCLI[] = { /* */
      {&RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED},
      {&RxClObjSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED},
      {&RxClCamSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED},
      {&RxClLights, rxCLALLOWABSENT, rxCLRESERVED},
      {&RxClVSteps, rxCLALLOWABSENT, rxCLRESERVED} };

    #define NUMCLUSTERSOFINTEREST                       \
        ((sizeof(LightCLI))/(sizeof(LightCLI[0])))

    static RxClusterValidityReq LightInputVR[NUMCLUSTERSOFINTEREST] = { /* */
        rxCLREQ_REQUIRED,
        rxCLREQ_REQUIRED,
        rxCLREQ_REQUIRED,
        rxCLREQ_OPTIONAL,
        rxCLREQ_OPTIONAL };

    static RxClusterValid LightOutputV[NUMCLUSTERSOFINTEREST] = { /* */
        rxCLVALID_NOCHANGE,
        rxCLVALID_NOCHANGE,
        rxCLVALID_VALID,
        rxCLVALID_NOCHANGE,
        rxCLVALID_NOCHANGE };

    static RwChar _Output[] = RWSTRING("Output");

    static RxOutputSpec LightOutput[] = { /* */
      {_Output,
       LightOutputV,
       rxCLVALID_NOCHANGE} };

    #define NUMOUTPUTS                                          \
        ((sizeof(LightOutput))/(sizeof(LightOutput[0])))

    static RwChar _Light_csl[] = RWSTRING("Light.csl");

    static RxNodeDefinition nodeLightCSL = { /* */
      _Light_csl,
      {_LightFn,
       (RxNodeInitFn)NULL,
       (RxNodeTermFn)NULL,
       (RxPipelineNodeInitFn)NULL,
       (RxPipelineNodeTermFn)NULL,
       (RxPipelineNodeConfigFn)NULL,
       (RxConfigMsgHandlerFn)NULL },
      {NUMCLUSTERSOFINTEREST, LightCLI,
       LightInputVR,
       NUMOUTPUTS, LightOutput},
      0,
      (RxNodeDefEditable)FALSE,
      0
    };

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionGetLight"));

    /*RWMESSAGE((RWSTRING("Pipeline II node")));*/

    RWRETURN(&nodeLightCSL);
}

