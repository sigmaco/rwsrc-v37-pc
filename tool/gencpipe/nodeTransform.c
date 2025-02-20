/*
 * Transforming vertices into camera space
 *
 * Copyright (c) Criterion Software Limited
 */
/****************************************************************************
 *                                                                          *
 * module : nodeTransform.c                                                 *
 *                                                                          *
 * purpose: yawn...                                                         *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 includes
 */

#include "rwcore.h"

#include "rpdbgerr.h"

#include "p2clpcom.h"

#include "rtgncpip.h"


/****************************************************************************
 local defines
 */
#define MESSAGE(_string)                                        \
    RwDebugSendMessage(rwDEBUGMESSAGE, "Transform.csl", _string)


#define m_X_X (Matrix->right.x)
#define m_Y_X (Matrix->up.x)
#define m_Z_X (Matrix->at.x)
#define m_W_X (Matrix->pos.x)

#define m_X_Y (Matrix->right.y)
#define m_Y_Y (Matrix->up.y)
#define m_Z_Y (Matrix->at.y)
#define m_W_Y (Matrix->pos.y)

#define m_X_Z (Matrix->right.z)
#define m_Y_Z (Matrix->up.z)
#define m_Z_Z (Matrix->at.z)
#define m_W_Z (Matrix->pos.z)


/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

static RwBool
TransformNodeParallel(RxPipelineNodeInstance * self, RwCamera * camera)
{

    RwBool              result = TRUE;
    RxPacket           *packet;
    RwRaster           *rpRas;
    RwInt32             camWidth;
    RwInt32             camHeight;

    RWFUNCTION(RWSTRING("TransformNodeParallel"));

    rpRas = RwCameraGetRaster(camera);

    /*
     * Set up oft-used clipping numeros
     */
    _rwClipInfoGlobal.camOffsetX = rpRas->nOffsetX;
    _rwClipInfoGlobal.camOffsetY = rpRas->nOffsetY;
    camWidth = RwRasterGetWidth(rpRas);
    _rwClipInfoGlobal.camWidth = (RwReal) camWidth;
    camHeight = RwRasterGetHeight(rpRas);
    _rwClipInfoGlobal.camHeight = (RwReal) camHeight;
    _rwClipInfoGlobal.farClip = camera->farPlane;
    _rwClipInfoGlobal.nearClip = camera->nearPlane;
    _rwClipInfoGlobal.zScale = camera->zScale;
    _rwClipInfoGlobal.zShift = camera->zShift;

    packet = (RxPacket *)RxPacketFetch(self);
    RWASSERT(NULL != packet);

    {
        RwUInt32            NumVerts;
        RwMatrix           *Matrix;
        RwV3d               objVertex;
        RwReal              inx, iny, inz, outx, outy, outz;
        RwInt32             ClipFlagsOr, ClipFlagsAnd;
        RxCluster          *ObjVerts, *CamVerts, *DevVerts, *MeshState;
        RwInt32             ObjVertsStride;
        RwInt32             CamVertsStride;
        RwInt32             DevVertsStride;
        RwRGBA              Col;
        RwBool              bVertColours = FALSE,
                            bVertPreLit  = FALSE,
                            bVertRxUVs = FALSE;
        RxMeshStateVector  *MeshData;

        ObjVerts = RxClusterLockRead(packet, 0);
        RWASSERT(NULL != ObjVerts);
        CamVerts = RxClusterLockWrite(packet, 1, self);
        RWASSERT(NULL != CamVerts);
        DevVerts = RxClusterLockWrite(packet, 2, self);
        RWASSERT(NULL != DevVerts);
        MeshState = RxClusterLockWrite(packet, 3, self);
        RWASSERT(NULL != MeshState);
        MeshData = RxClusterGetCursorData(MeshState, RxMeshStateVector);
        if (MeshData->NumVertices == 0)
        {
            /* Not actually an error... is it? */
            RWRETURN(TRUE);
        }
        /* Create new space for the Camera and Device vertices */
        CamVerts = RxClusterInitializeData(
                       CamVerts,
                       MeshData->NumVertices,
                       sizeof(RxCamSpace3DVertex));
        RWASSERT(NULL != CamVerts);

        DevVerts = RxClusterInitializeData(
                       DevVerts,
                       MeshData->NumVertices,
                       sizeof(RxScrSpace2DVertex));
        RWASSERT(NULL != DevVerts);

        Matrix = &(MeshData->Obj2Cam);
        NumVerts = MeshData->NumVertices;

        if (MeshData->Flags & rxGEOMETRY_COLORED)
        {
            bVertColours = TRUE;
        }
        else
        if (MeshData->Flags & rxGEOMETRY_PRELIT)
        {
            bVertPreLit = TRUE;
        }
        else
        {
            Col = MeshData->MatCol;
        }
        if (MeshData->Flags & rxGEOMETRY_TEXTURED)
        {
            bVertRxUVs = TRUE;
        }

        ClipFlagsOr = 0;
        ClipFlagsAnd = -1;

        ObjVertsStride = ObjVerts->stride;
        CamVertsStride = CamVerts->stride;
        DevVertsStride = DevVerts->stride;

#ifdef SKY2_DRVMODEL_H
        /* On the PlayStation 2, we should munge the camera matrix and
         * clipping info such that devverts are clipped to the large [0,4096]
         * overdraw frustum */
#endif

        /* Parallel projection */
        while (NumVerts-- > 0)
        {
            RxObjSpace3DVertexGetPos(RxClusterGetCursorData
                                       (ObjVerts, RxObjSpace3DVertex),
                                       &objVertex);
            inx = objVertex.x;
            iny = objVertex.y;
            inz = objVertex.z;

            outx = inx * m_X_X + iny * m_Y_X + inz * m_Z_X + m_W_X;
            outy = inx * m_X_Y + iny * m_Y_Y + inz * m_Z_Y + m_W_Y;
            outz = inx * m_X_Z + iny * m_Y_Z + inz * m_Z_Z + m_W_Z;

            /* Fill in coordinates */
            RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex)->cameraVertex.x =
                outx;
            RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex)->cameraVertex.y =
                outy;
            RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex)->cameraVertex.z =
                outz;

            /* Copy in vertex(Im3D)/prelighting or material
               colours now while Obj/Cam/DevVerts are in cache */
            if (bVertPreLit)
            {
                RxObjSpace3DVertexGetPreLitColor(RxClusterGetCursorData
                                                    (ObjVerts, RxObjSpace3DVertex),
                                                   &Col);
                RxCamSpace3DVertexSetRGBA(RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex),
                                            Col.red, Col.green, Col.blue, Col.alpha);
            }
            else
            if (bVertColours)
            {
                RxObjSpace3DVertexGetColor(RxClusterGetCursorData
                                              (ObjVerts, RxObjSpace3DVertex),
                                             &Col);
                /* TODO: the following should be unnecessary; camvert cols are ignored in Im3D */
                RxCamSpace3DVertexSetRGBA(RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex),
                                            Col.red, Col.green, Col.blue, Col.alpha);
            }
            else
            {
                /* Im3D verts *always* have vertex colours, so this must be a non-prelit
                   RxObjSpace3DVertex, which means we just initialise alpha to 255 */
                RxCamSpace3DVertexSetRGBA(RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex),
                                            0, 0, 0, 255);
            }
            RwIm2DVertexSetIntRGBA(RxClusterGetCursorData
                                   (DevVerts, RxScrSpace2DVertex),
                                   Col.red, Col.green, Col.blue,
                                   Col.alpha);

            /* Only do the projection for vertices inside the view volume
             * 3D clipped vertices will have to wait until later ... */

            RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex)->clipFlags =
                ((outz < _rwClipInfoGlobal.nearClip) ? (RwUInt8) rwZLOCLIP
                 : ((outz > _rwClipInfoGlobal.farClip) ? (RwUInt8) rwZHICLIP : 0)) |
                ((outy < 0) ? (RwUInt8) rwYLOCLIP
                 : ((outy > 1) ? (RwUInt8) rwYHICLIP : 0)) |
                ((outx < 0) ? (RwUInt8) rwXLOCLIP :
                 ((outx > 1) ? (RwUInt8) rwXHICLIP : 0));
            ClipFlagsOr |=
                RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex)->clipFlags;
            ClipFlagsAnd &=
                RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex)->clipFlags;

            if (RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex)->clipFlags == 0)
            {
                /* Set recipZ as 1.0 to prevent perspective correction */
                RwIm2DVertexSetCameraX(RxClusterGetCursorData
                                       (DevVerts, RxScrSpace2DVertex),
                                       outx);
                RwIm2DVertexSetCameraY(RxClusterGetCursorData
                                       (DevVerts, RxScrSpace2DVertex),
                                       outy);
                RwIm2DVertexSetCameraZ(RxClusterGetCursorData
                                       (DevVerts, RxScrSpace2DVertex),
                                       outz);
                RwIm2DVertexSetRecipCameraZ(RxClusterGetCursorData
                                            (DevVerts, RxScrSpace2DVertex),
                                            1.0f);

                /* Parallel projection */
                RwIm2DVertexSetScreenX(RxClusterGetCursorData
                                       (DevVerts, RxScrSpace2DVertex),
                                       outx * _rwClipInfoGlobal.camWidth +
                                       _rwClipInfoGlobal.camOffsetX);
                RwIm2DVertexSetScreenY(RxClusterGetCursorData
                                       (DevVerts, RxScrSpace2DVertex),
                                       outy * _rwClipInfoGlobal.camHeight +
                                       _rwClipInfoGlobal.camOffsetY);
                RwIm2DVertexSetScreenZ(RxClusterGetCursorData
                                       (DevVerts, RxScrSpace2DVertex),
                                       outz * _rwClipInfoGlobal.zScale +
                                       _rwClipInfoGlobal.zShift);

                /* If the geometry is textured,
                 * copy in RxUVs now while Obj/Cam/DevVerts are in cache */
                if (bVertRxUVs)
                {
                    RwReal              U, V;

                    U =
                        RxObjSpace3DVertexGetU(RxClusterGetCursorData
                                                 (ObjVerts,
                                                  RxObjSpace3DVertex));
                    V =
                        RxObjSpace3DVertexGetV(RxClusterGetCursorData
                                                 (ObjVerts,
                                                  RxObjSpace3DVertex));

                    RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex)->u = U;
                    RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex)->v = V;
                    RwIm2DVertexSetU(RxClusterGetCursorData
                                     (DevVerts, RxScrSpace2DVertex), U,
                                     (RwReal)(1.0));
                    RwIm2DVertexSetV(RxClusterGetCursorData
                                     (DevVerts, RxScrSpace2DVertex), V,
                                     (RwReal)(1.0));
                }
            }
            else
            {
                /* If the geometry is textured,
                 * copy in RxUVs now while Obj/Cam/DevVerts are in cache */
                if (bVertRxUVs)
                {
                    RwReal              U, V;

                    U =
                        RxObjSpace3DVertexGetU(RxClusterGetCursorData
                                                 (ObjVerts,
                                                  RxObjSpace3DVertex));
                    V =
                        RxObjSpace3DVertexGetV(RxClusterGetCursorData
                                                 (ObjVerts,
                                                  RxObjSpace3DVertex));

                    RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex)->u = U;
                    RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex)->v = V;
                }

                /* Set RecipZ to zero (an impossible value) to indicate clipped vertices
                   without reference to CamVert clipflags (useful e.g for particles)
                   [setting ScreenZ to zero also makes it *really* impossible, even if
                    1/z = 0 is possible on some hardware] */
                RwIm2DVertexSetRecipCameraZ(RxClusterGetCursorData
                                            (DevVerts, RxScrSpace2DVertex), 0);
                RwIm2DVertexSetScreenZ(RxClusterGetCursorData
                                            (DevVerts, RxScrSpace2DVertex), 0);
            }

            /* Onto the next vertex */
            RxClusterIncCursorByStride(ObjVerts, ObjVertsStride);
            RxClusterIncCursorByStride(CamVerts, CamVertsStride);
            RxClusterIncCursorByStride(DevVerts, DevVertsStride);
        }

        DevVerts->numUsed = MeshData->NumVertices;
        CamVerts->numUsed = MeshData->NumVertices;

        /* If clipFlagsOr   = 0, everything is on  the screen,
         * If clipFlagsAnd != 0, everything is off the screen.
         */
        MeshData->ClipFlagsOr = ClipFlagsOr;
        MeshData->ClipFlagsAnd = ClipFlagsAnd;

        /* Oi!!
         * If we're not overloading stuff, copy it across now from the instanced
         * vertices to the camera and potentially the device vertices
         * if (!StateData->ClipFlagsAnd)
         * {
         * _rwPipeState.currentContext->fpSetNonOverloadedFieldsInCamAndDevVert(repEntry);
         *}
         */

        /* Output the packet to the first output of this Node...
         * ...unless the vertices are entirely offscreen, in which case
         * send the packet to the second output (usually to be destroyed) */
        if (ClipFlagsAnd)
        {
            RxPacketDispatch(packet, 1, self);
        }
        else
        {
            RxPacketDispatch(packet, 0, self);
        }
    }

    RWRETURN(result);

}

static RwBool
TransformNodePerspective(RxPipelineNodeInstance * self, RwCamera * camera)
{
    RwBool              result = TRUE;
    RxPacket           *packet;
    RwRaster           *rpRas;
    RwInt32             camWidth;
    RwInt32             camHeight;

    RWFUNCTION(RWSTRING("TransformNodePerspective"));

    rpRas = RwCameraGetRaster(camera);

    /*
     * Set up oft-used clipping numeros
     */
    _rwClipInfoGlobal.camOffsetX = rpRas->nOffsetX;
    _rwClipInfoGlobal.camOffsetY = rpRas->nOffsetY;
    camWidth = RwRasterGetWidth(rpRas);
    _rwClipInfoGlobal.camWidth = (RwReal) camWidth;
    camHeight = RwRasterGetHeight(rpRas);
    _rwClipInfoGlobal.camHeight = (RwReal) camHeight;
    _rwClipInfoGlobal.farClip = camera->farPlane;
    _rwClipInfoGlobal.nearClip = camera->nearPlane;
    _rwClipInfoGlobal.zScale = camera->zScale;
    _rwClipInfoGlobal.zShift = camera->zShift;

    packet = (RxPacket *)RxPacketFetch(self);
    RWASSERT(NULL != packet);

    {
        RwUInt32            NumVerts;
        RwMatrix           *Matrix;
        RwInt32             ClipFlagsOr, ClipFlagsAnd;
        RwV3d               objVertex;
        RwReal              inx, iny, inz, outx, outy, outz;
        RxCluster          *ObjVerts, *CamVerts, *DevVerts, *MeshState;
        RwInt32             ObjVertsStride;
        RwInt32             CamVertsStride;
        RwInt32             DevVertsStride;
        RwRGBA              Col;
        RwBool              bVertColours = FALSE,
                            bVertPreLit  = FALSE,
                            bVertRxUVs = FALSE;
        RxMeshStateVector  *MeshData;

        ObjVerts = RxClusterLockRead(packet, 0);
        RWASSERT(NULL != ObjVerts);
        CamVerts = RxClusterLockWrite(packet, 1, self);
        RWASSERT(NULL != CamVerts);
        DevVerts = RxClusterLockWrite(packet, 2, self);
        RWASSERT(NULL != DevVerts);
        MeshState = RxClusterLockWrite(packet, 3, self);
        RWASSERT(NULL != MeshState);
        MeshData = RxClusterGetCursorData(MeshState, RxMeshStateVector);
        if (MeshData->NumVertices == 0)
        {
            /* Not actually an error... is it? */
            RWRETURN(TRUE);
        }

        /* Create new space for the Camera and Device vertices */
        CamVerts = RxClusterInitializeData(
                       CamVerts,
                       MeshData->NumVertices,
                       sizeof(RxCamSpace3DVertex));
        RWASSERT(NULL != CamVerts);

        DevVerts = RxClusterInitializeData(
                       DevVerts,
                       MeshData->NumVertices,
                       sizeof(RxScrSpace2DVertex));
        RWASSERT(NULL != DevVerts);

        Matrix = &(MeshData->Obj2Cam);
        NumVerts = MeshData->NumVertices;

        if (MeshData->Flags & rxGEOMETRY_COLORED)
        {
            bVertColours = TRUE;
        }
        else
        if (MeshData->Flags & rxGEOMETRY_PRELIT)
        {
            bVertPreLit = TRUE;
        }
        else
        {
            Col = MeshData->MatCol;
        }
        if (MeshData->Flags & rxGEOMETRY_TEXTURED)
        {
            bVertRxUVs = TRUE;
        }

        ClipFlagsOr = 0;
        ClipFlagsAnd = -1;

        ObjVertsStride = ObjVerts->stride;
        CamVertsStride = CamVerts->stride;
        DevVertsStride = DevVerts->stride;

#ifdef SKY2_DRVMODEL_H
        /* On the PlayStation 2, we should munge the camera matrix and
         * clipping info such that devverts are clipped to the large
         * [0,4096] overdraw frustum */
#endif

        /* Perspective Projection */
        while (NumVerts-- > 0)
        {
            RxObjSpace3DVertexGetPos(RxClusterGetCursorData
                                       (ObjVerts, RxObjSpace3DVertex),
                                       &objVertex);
            inx = objVertex.x;
            iny = objVertex.y;
            inz = objVertex.z;

            outx = inx * m_X_X + iny * m_Y_X + inz * m_Z_X + m_W_X;
            outy = inx * m_X_Y + iny * m_Y_Y + inz * m_Z_Y + m_W_Y;
            outz = inx * m_X_Z + iny * m_Y_Z + inz * m_Z_Z + m_W_Z;

            /* Fill in coordinates */
            RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex)->cameraVertex.x =
                outx;
            RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex)->cameraVertex.y =
                outy;
            RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex)->cameraVertex.z =
                outz;

            /* Copy in vertex(Im3D)/prelighting or material
               colours now while Obj/Cam/DevVerts are in cache */
            if (bVertPreLit)
            {
                RxObjSpace3DVertexGetPreLitColor(RxClusterGetCursorData
                                                    (ObjVerts, RxObjSpace3DVertex),
                                                   &Col);
                RxCamSpace3DVertexSetRGBA(RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex),
                                            Col.red, Col.green, Col.blue, Col.alpha);
            }
            else
            if (bVertColours)
            {
                RxObjSpace3DVertexGetColor(RxClusterGetCursorData
                                              (ObjVerts, RxObjSpace3DVertex),
                                             &Col);
                /* TODO: the following should be unnecessary; camvert cols are ignored in Im3D */
                RxCamSpace3DVertexSetRGBA(RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex),
                                            Col.red, Col.green, Col.blue, Col.alpha);
            }
            else
            {
                /* Im3D verts *always* have vertex colours, so this must be a non-prelit
                   RxObjSpace3DVertex, which means we just initialise alpha to 255 */
                RxCamSpace3DVertexSetRGBA(RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex),
                                            0, 0, 0, 255);
            }
            RwIm2DVertexSetIntRGBA(RxClusterGetCursorData
                                   (DevVerts, RxScrSpace2DVertex),
                                   Col.red, Col.green, Col.blue,
                                   Col.alpha);

            /* Only do the projection for vertices inside the view volume
             * 3D clipped vertices will have to wait until later...  */

            RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex)->clipFlags =
                ((outz < _rwClipInfoGlobal.nearClip) ? (RwUInt8) rwZLOCLIP
                 : ((outz > _rwClipInfoGlobal.farClip) ? (RwUInt8) rwZHICLIP : 0)) |
                ((outy < 0) ? (RwUInt8) rwYLOCLIP
                 : ((outy > outz) ? (RwUInt8) rwYHICLIP : 0)) |
                ((outx < 0) ? (RwUInt8) rwXLOCLIP
                 : ((outx > outz) ? (RwUInt8) rwXHICLIP : 0));
            ClipFlagsOr |=
                RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex)->clipFlags;
            ClipFlagsAnd &=
                RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex)->clipFlags;

            if (RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex)->clipFlags == 0)
            {
                RwReal              nRecipZ = 1 / outz;

                /* Set the recip */
                RwIm2DVertexSetCameraX(RxClusterGetCursorData
                                       (DevVerts, RxScrSpace2DVertex),
                                       outx);
                RwIm2DVertexSetCameraY(RxClusterGetCursorData
                                       (DevVerts, RxScrSpace2DVertex),
                                       outy);
                RwIm2DVertexSetCameraZ(RxClusterGetCursorData
                                       (DevVerts, RxScrSpace2DVertex),
                                       outz);
                RwIm2DVertexSetRecipCameraZ(RxClusterGetCursorData
                                            (DevVerts, RxScrSpace2DVertex),
                                            nRecipZ);

                /* Perspective projection */
                RwIm2DVertexSetScreenX(RxClusterGetCursorData
                                       (DevVerts, RxScrSpace2DVertex),
                                       outx * nRecipZ * _rwClipInfoGlobal.camWidth +
                                       _rwClipInfoGlobal.camOffsetX);
                RwIm2DVertexSetScreenY(RxClusterGetCursorData
                                       (DevVerts, RxScrSpace2DVertex),
                                       outy * nRecipZ *
                                       _rwClipInfoGlobal.camHeight +
                                       _rwClipInfoGlobal.camOffsetY);
                RwIm2DVertexSetScreenZ(RxClusterGetCursorData
                                       (DevVerts, RxScrSpace2DVertex),
                                       nRecipZ * _rwClipInfoGlobal.zScale +
                                       _rwClipInfoGlobal.zShift);

                /* If the geometry is textured,
                 * copy in RxUVs now while Obj/Cam/DevVerts are in cache */
                if (bVertRxUVs)
                {
                    RwReal              U, V;

                    U =
                        RxObjSpace3DVertexGetU(RxClusterGetCursorData
                                                 (ObjVerts,
                                                  RxObjSpace3DVertex));
                    V =
                        RxObjSpace3DVertexGetV(RxClusterGetCursorData
                                                 (ObjVerts,
                                                  RxObjSpace3DVertex));

                    RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex)->u = U;
                    RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex)->v = V;
                    RwIm2DVertexSetU(RxClusterGetCursorData
                                     (DevVerts, RxScrSpace2DVertex), U,
                                     nRecipZ);
                    RwIm2DVertexSetV(RxClusterGetCursorData
                                     (DevVerts, RxScrSpace2DVertex), V,
                                     nRecipZ);
                }
            }
            else
            {
                /* If the geometry is textured,
                 * copy in RxUVs now while Obj/Cam/DevVerts are in cache */
                if (bVertRxUVs)
                {
                    RwReal              U, V;

                    U =
                        RxObjSpace3DVertexGetU(RxClusterGetCursorData
                                                 (ObjVerts,
                                                  RxObjSpace3DVertex));
                    V =
                        RxObjSpace3DVertexGetV(RxClusterGetCursorData
                                                 (ObjVerts,
                                                  RxObjSpace3DVertex));

                    RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex)->u = U;
                    RxClusterGetCursorData(CamVerts, RxCamSpace3DVertex)->v = V;
                }

                /* Set RecipZ to zero (an impossible value) to indicate clipped vertices
                   without reference to CamVert clipflags (useful e.g for particles)
                   [setting ScreenZ to zero also makes it *really* impossible, even if
                    1/z = 0 is possible on some hardware] */
                RwIm2DVertexSetRecipCameraZ(RxClusterGetCursorData
                                            (DevVerts, RxScrSpace2DVertex), 0);
                RwIm2DVertexSetScreenZ(RxClusterGetCursorData
                                            (DevVerts, RxScrSpace2DVertex), 0);
            }

            /* Onto the next vertex */
            RxClusterIncCursorByStride(ObjVerts, ObjVertsStride);
            RxClusterIncCursorByStride(CamVerts, CamVertsStride);
            RxClusterIncCursorByStride(DevVerts, DevVertsStride);
        }

        DevVerts->numUsed = MeshData->NumVertices;
        CamVerts->numUsed = MeshData->NumVertices;

        /* If clipFlagsOr   = 0, everything is on  the screen,
         * If clipFlagsAnd != 0, everything is off the screen.
         */
        MeshData->ClipFlagsOr = ClipFlagsOr;
        MeshData->ClipFlagsAnd = ClipFlagsAnd;

        /* Oi!!
         * If we're not overloading stuff, copy it across now from the instanced
         * vertices to the camera and potentially the device vertices
         * if (!StateData->ClipFlagsAnd)
         * {
         * _rwPipeState.currentContext->fpSetNonOverloadedFieldsInCamAndDevVert(repEntry);
         *}
         */

        /* Output the packet to the first output of this Node...
         * ...unless the vertices are entirely offscreen, in which case
         * send the packet to the second output (usually to be destroyed) */
        if (ClipFlagsAnd)
        {
            RxPacketDispatch(packet, 1, self);
        }
        else
        {
            RxPacketDispatch(packet, 0, self);
        }
    }

    RWRETURN(result);

}

/*****************************************************************************
 TransformNode

 Generic (non-device-specific) transform Node.

 on entry: -
 on exit : -
*/

static RwBool
TransformNode( RxPipelineNodeInstance * self,
               const RxPipelineNodeParam * params __RWUNUSED__ )
{
    RwBool              result;
    RwCamera           *camera;

    RWFUNCTION(RWSTRING("TransformNode"));

    /* TODO: following bad for multithreading with multiple cameras */
    camera = (RwCamera *) RWSRCGLOBAL(curCamera);
    RWASSERT(NULL != camera);

    result =
        (rwPERSPECTIVE != camera->projectionType) ?
        TransformNodeParallel(self, camera) :
        TransformNodePerspective(self, camera);

    RWRETURN(result);
}

/**
 * \ingroup rtgencpipe
 * \ref RxNodeDefinitionGetTransform returns a pointer to a node
 * to transform vertices into camera space.
 *
 * This takes object-space vertices and transforms them into camera space,
 * generating camera-space and screen-space vertices. It performs clipping
 * and projection in accordance with the current camera's view frustum
 * (as defined by the camera's ViewWindow, ViewOffset and Projection
 * type - parallel or perspective).
 *
 * The object vertices are transformed into camera space by the Obj2Cam matrix
 * which should have been set up in the mesh state cluster by an instancing
 * node (this is done by the standard Im3D in the Im3D transform pipeline and
 * the atomic and world sector instancing nodes in the atomic and world sector
 * object pipelines).
 *
 * This node sets color values in the camera-space vertices to zero if the
 * incoming vertices are not prelit or to the prelighting color of each
 * vertex if they are. This means lighting can be perfomed after this node
 * without needing to use PreLight.csl (which is included because some
 * platform-specific transform nodes don't do this color initialization and
 * so that developers can use PreLight.csl as a independent unit). The
 * screen-space vertices also have their color initialized to the
 * prelighting color if present, the vertex color if present or, if neither
 * are present, the material color. This means that if no lighting is
 * performed (i.e Light.csl and PostLight.csl are omitted, for example with
 * Im3D vertices during Im3D rendering or with atomic/world-sector geometry
 * which is not lit) the screen-space vertices will still end up with valid
 * and sensible colors.
 *
 * After transformation, all generated camera vertices are tested to detect
 * if they lie outside of any or all of the current camera's clipping
 * (frustum) planes. The clip flags of the camera vertices are updated
 * accordingly. Camera-space vertices are only projected to generate valid
 * screen-space vertices if they are not clipped (screen-space vertices are
 * allocated space in their cluster and their color is filled in regardless).
 * The clip flags of all camera-space vertices are OR'd together to generate a
 * ClipFlagsOr value which is stored in the mesh state cluster. Similarly a
 * ClipFlagsAnd value is generated and stored by ANDing all the clipflags
 * together. These two values are useful in determining the bulk clipping
 * properties of the packet's vertices. For example, if ClipFlagsAnd is
 * non-zero, then all of the vertices have been clipped by the same one (or
 * more) frustum plane(s). If ClipFlagsOr is zero then all of the vertices
 * are onscreen.
 *
 * If ClipFlagsAnd is non-zero then all the vertices are clipped by one (or
 * more) frustum planes, so the geometry is entirely offscreen. In this case,
 * the packet is sent to the node's second output (which is usually left
 * disconnected, resulting in the packet's destruction. In some cases,
 * however, it may be desirable to connect this output to subsequent nodes
 * if the geometry's extent is known to be modified further down the pipeline
 * if, for example, vertex normals are added, to be rendered as lines),
 * but otherwise packets are sent to the node's first output. Note that all
 * triangles or lines represented by the geometry may still be offscreen even
 * if ClipFlagsAnd is zero (some vertices may be culled by one frustum plane
 * and some others by another).
 *
 * The node has two outputs.
 * The input requirements of this node:
 *
 * \verbatim
   RxClObjSpace3DVertices - required
   RxClCamSpace3DVertices - don't want
   RxClScrSpace2DVertices - don't want
   RxClMeshState          - required
   \endverbatim
 *
 * The characteristics of this node's first output:
 *
 * \verbatim
   RxClObjSpace3DVertices - no change
   RxClCamSpace3DVertices - valid
   RxClScrSpace2DVertices - valid
   RxClMeshState          - valid
   \endverbatim
 *
 * The characteristics of this node's second output:
 *
 * \verbatim
   RxClObjSpace3DVertices - no change
   RxClCamSpace3DVertices - valid
   RxClScrSpace2DVertices - valid
   RxClMeshState          - valid
   \endverbatim
 *
 * \return A pointer to a node to transform vertices into camera space
 *
 * \see RxNodeDefinitionGetClipLine
 * \see RxNodeDefinitionGetClipTriangle
 * \see RxNodeDefinitionGetCullTriangle
 * \see RxNodeDefinitionGetImmInstance
 * \see RxNodeDefinitionGetImmMangleLineIndices
 * \see RxNodeDefinitionGetImmMangleTriangleIndices
 * \see RxNodeDefinitionGetImmRenderSetup
 * \see RxNodeDefinitionGetScatter
 * \see RxNodeDefinitionGetSubmitLine
 * \see RxNodeDefinitionGetSubmitTriangle
 * \see RxNodeDefinitionGetUVInterp
 *
 */

RxNodeDefinition   *
RxNodeDefinitionGetTransform(void)
{
    static RxClusterRef N3clofinterest[] = { /* */
        {&RxClObjSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClCamSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClScrSpace2DVertices, rxCLALLOWABSENT, rxCLRESERVED},
        {&RxClMeshState,          rxCLALLOWABSENT, rxCLRESERVED}
    };

#define NUMCLUSTERSOFINTEREST \
        ((sizeof(N3clofinterest))/(sizeof(N3clofinterest[0])))


    static RxClusterValidityReq N3inputreqs[NUMCLUSTERSOFINTEREST] = { /* */
        rxCLREQ_REQUIRED,
        rxCLREQ_DONTWANT,
        rxCLREQ_DONTWANT,
        rxCLREQ_REQUIRED
    };

    static RxClusterValid N3outcl1[NUMCLUSTERSOFINTEREST] = { /* */
        rxCLVALID_NOCHANGE,
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_VALID
    };

    static RxClusterValid N3outcl2[NUMCLUSTERSOFINTEREST] = { /* */
        rxCLVALID_NOCHANGE,
        rxCLVALID_VALID,
        rxCLVALID_VALID,
        rxCLVALID_VALID
    };

    static RwChar _TransformOut[]  = RWSTRING("TransformOut");
    static RwChar _TransformOut2[] = RWSTRING("TransformOut2");

    static RxOutputSpec N3outputs[] = { /* */
        {_TransformOut,    N3outcl1,    rxCLVALID_NOCHANGE},
        {_TransformOut2,   N3outcl2,    rxCLVALID_NOCHANGE}
    };

#define NUMOUTPUTS \
        ((sizeof(N3outputs))/(sizeof(N3outputs[0])))

    static RwChar _Transform_csl[] = RWSTRING("Transform.csl");

    static RxNodeDefinition nodeTransformCSL = { /* */
        _Transform_csl,
        {TransformNode,
         (RxNodeInitFn)NULL,
         (RxNodeTermFn)NULL,
         (RxPipelineNodeInitFn)NULL,
         (RxPipelineNodeTermFn)NULL,
         (RxPipelineNodeConfigFn)NULL,
         (RxConfigMsgHandlerFn)NULL},
        {NUMCLUSTERSOFINTEREST,
         N3clofinterest,
         N3inputreqs,
         NUMOUTPUTS,
         N3outputs} ,
        0,
        (RxNodeDefEditable)FALSE,
        0
    };

    RxNodeDefinition *result = &nodeTransformCSL;

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionGetTransform"));

    /*RWMESSAGE((RWSTRING("result %p"), result));*/

    RWRETURN(result);
}

