
/*
 *
 * Functionality for 2D rendering
 *
 * Copyright (c) 1998 Criterion Software Ltd.
 */

/****************************************************************************
 *                                                                          *
 *  Module  :   gstate.c                                                    *
 *                                                                          *
 *  Purpose :   graphics state                                              *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
 */
#include "rwcore.h"
#include "rpdbgerr.h"
#include "rpworld.h"

#include "rt2d.h"
#include "path.h"
#include "font.h"
#include "brush.h"
#include "object.h"
#include "tri.h"

#if (defined(SKY2_DRVMODEL_H))
#include "ps2pipes.h"
#endif /* (defined(SKY2_DRVMODEL_H)) */

#include "gstate.h"

/****************************************************************************
 Local Types
 */

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */

/****************************************************************************
 Globals (across program)
 */
Rt2dGlobalVars      Rt2dGlobals;

/****************************************************************************
 Local (static) Globals
 */

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   Functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

static RwCamera    *
PS2CameraEndUpdate2d(RwCamera * camera)
{
    RwCamera           *result;

    RWFUNCTION(RWSTRING("PS2CameraEndUpdate2d"));

    result = Rt2dGlobals.cameraEndUpdate(camera);

    RWRETURN(result);
}

/****************************************************************************/

/**
 * \ingroup rt2dctm
 * \ref Rt2dCTMSetIdentity
 * is used to set the current transformation matrix
 * (the matrix at the top of the matrix stack) equal to the identity
 * matrix. This procedure removes all previously applied transformations.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \return TRUE if successful or FALSE if there is an error
 * \see Rt2dCTMRotate
 * \see Rt2dCTMTranslate
 * \see Rt2dCTMScale
 * \see Rt2dCTMPush
 * \see Rt2dCTMPop
 */
RwBool
Rt2dCTMSetIdentity(void)
{
    RWAPIFUNCTION(RWSTRING("Rt2dCTMSetIdentity"));

    RwMatrixSetIdentity(Rt2dGlobals.ctm[Rt2dGlobals.mtos]);
    Rt2dGlobals.CTMValid = FALSE;
    Rt2dGlobals.sqToleranceValid = FALSE;

    RWRETURN(TRUE);
}

/****************************************************************************/

/**
 * \ingroup rt2dctm
 * \ref Rt2dCTMRotate
 * is used to apply a rotation to the current
 * transformation matrix (CTM) using the specified angle of rotation. A
 * rotation in an anticlockwise direction are achieved using a positive
 * angle.
 * Note that the rotation is preconcatenated with the CTM.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param theta  A RwReal value equal to the angle of rotation in degrees
 * \return TRUE if successful or FALSE if there is an error
 * \see Rt2dCTMTranslate
 * \see Rt2dCTMScale
 * \see Rt2dCTMPush
 * \see Rt2dCTMPop
 * \see Rt2dCTMSetIdentity
 */
RwBool
Rt2dCTMRotate(RwReal theta)
{
    RwV3d               axis;

    RWAPIFUNCTION(RWSTRING("Rt2dCTMRotate"));

    axis.x = 0.0f;
    axis.y = 0.0f;
    axis.z = 1.0f;
    RwMatrixRotate(Rt2dGlobals.ctm[Rt2dGlobals.mtos], &axis, theta,
                   rwCOMBINEPRECONCAT);

    Rt2dGlobals.CTMValid = FALSE;
    Rt2dGlobals.sqToleranceValid = FALSE;

    RWRETURN(TRUE);
}

/****************************************************************************/

/****************************************************************************/
RwMatrix           *
_rt2dCTMGetInverse(void)
{
    RWFUNCTION(RWSTRING("_rt2dCTMGetInverse"));

    /* force update */
    _rt2dCTMGet();

    RWRETURN(&Rt2dGlobals.iCTM2d);
}

/****************************************************************************/
RwBool
_rt2dCTMGetWinding(void)
{
    RwV3d               at;
    RwMatrix           *mat;

    RWFUNCTION(RWSTRING("_rt2dCTMGetWinding"));

    mat = _rt2dCTMGet();
    RwV3dCrossProduct(&at, &mat->right, &mat->up);
    RWRETURN(at.z < 0.0f ? TRUE : FALSE);
}

/****************************************************************************/

/**
 * \ingroup rt2ddevice
 * \ref Rt2dDeviceGetStep
 * is a helper function that is used to retrieve a
 * triplet of vectors in the view modelling space that specify (a) the
 * step to take to move exactly one pixel in the x-direction, (b) the
 * step to take to move exactly one pixel in the y-direction, and (c) the
 * position of the origin of the modelling space.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 *
 * \param xstep  Pointer to a 2D vector that will receive the x-pixel increment
 * \param ystep  Pointer to a 2D vector that will receive the y-pixel increment
 * \param origin Pointer to a 2D vector that will receive the position of the
 *               origin
 *
 * \return TRUE if successful or FALSE if there is an error
 * \see Rt2dDeviceGetMetric
 * \see Rt2dDeviceSetMetric
 */
RwBool
Rt2dDeviceGetStep(RwV2d * xstep, RwV2d * ystep, RwV2d * origin)
{
    RwMatrix           *mat;
    RwV3d               p;
    RwV2d               vw, vp;
    RwReal              layerDepth;

    RWAPIFUNCTION(RWSTRING("Rt2dDeviceGetStep"));

    if (RwCameraGetProjection(Rt2dGlobals.viewCamera) == rwPARALLEL)
        layerDepth = (RwReal) 1.0;
    else
        layerDepth = Rt2dGlobals.layerDepth;

    vw = Rt2dGlobals.vw;
    vp = Rt2dGlobals.vp;

    mat = _rt2dCTMGetInverse();
    p.x = (vw.x / vp.x) * -layerDepth;
    p.y = 0.0f;
    p.z = 0.0f;
    RwV3dTransformVector(&p, &p, mat);
    xstep->x = p.x;
    xstep->y = p.y;

    p.x = 0.0f;
    p.y = (vw.y / vp.y) * layerDepth;
    p.z = 0.0f;
    RwV3dTransformVector(&p, &p, mat);
    ystep->x = p.x;
    ystep->y = p.y;

    p.x = vw.x * 0.5f * layerDepth;
    p.y = -vw.y * 0.5f * layerDepth;
    p.z = 0.0f;
    RwV3dTransformPoint(&p, &p, mat);
    origin->x = p.x;
    origin->y = p.y;

    RWRETURN(TRUE);
}

/****************************************************************************/

/**
 * \ingroup rt2ddevice
 * \ref Rt2dDeviceSetMetric
 * is a helper function which can used to map the
 * specified rectangular space to the camera display. After this function
 * has been called, the lower- left corner of the camera view has
 * coordinates (xOrigin, yOrigin) and a width and height as specified.
 * For example, if the camera's frame buffer has dimensions 640x480,
 * calling Rt2dDeviceSetMetric(0.0f, 0.0f, 640.0f, 480.0f) would enable
 * an application to specified positions in terms of pixel locations on
 * the frame buffer.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param x  A RwReal value equal to the x-coordinate of the display's xOrigin
 * \param y  A RwReal value equal to the y-coordinate of the display's yOrigin
 * \param w  A RwReal value equal to the width of the display
 * \param h  A RwReal value equal to the height of the display
 * \return TRUE if successful or FALSE if there is an error
 * \see Rt2dDeviceGetMetric
 * \see Rt2dDeviceGetStep
 */
RwBool
Rt2dDeviceSetMetric(RwReal x, RwReal y, RwReal w, RwReal h)
{
    RwV2d               xstep, ystep, origin;
    RwReal              xlength, ylength;
    RwV3d               v;

    RWAPIFUNCTION(RWSTRING("Rt2dDeviceSetMetric"));

    /* Reset the metric matrix BEFORE Rt2dDeviceGetStep */
    RwMatrixSetIdentity(&Rt2dGlobals.metricMtx);

    Rt2dCTMPush();
    Rt2dCTMSetIdentity();

    Rt2dDeviceGetStep(&xstep, &ystep, &origin);

    RWASSERT(w > 0);
    xlength = Rt2dGlobals.vp.x / w;
    RWASSERT(h > 0);
    ylength = Rt2dGlobals.vp.y / h;

    RwV2dScale(&xstep, &xstep, xlength);
    RwV2dScale(&ystep, &ystep, ylength);

    RwV2dLengthMacro(xlength, &xstep);
    RwV2dLengthMacro(ylength, &ystep);

    /* Setup the transform, this will be preconcat with the CTM */
    v.x = origin.x;
    v.y = origin.y;
    v.z = 0.0f;
    RwMatrixTranslate(&Rt2dGlobals.metricMtx, &v, rwCOMBINEPRECONCAT);

    v.x = xlength;
    v.y = ylength;
    v.z = 1.0f;
    RwMatrixScale(&Rt2dGlobals.metricMtx, &v, rwCOMBINEPRECONCAT);

    v.x = -x;
    v.y = -y;
    v.z = 0.0f;
    RwMatrixTranslate(&Rt2dGlobals.metricMtx, &v, rwCOMBINEPRECONCAT);

    Rt2dGlobals.deviceX = x;
    Rt2dGlobals.deviceY = y;
    Rt2dGlobals.deviceW = w;
    Rt2dGlobals.deviceH = h;

    Rt2dCTMPop();

    Rt2dGlobals.CTMValid = FALSE;

    RWRETURN(TRUE);
}

/****************************************************************************/

/**
 * \ingroup rt2ddevice
 * \ref Rt2dDeviceGetMetric
 * is a helper function that is used to return the
 * specified rectangular space in the camera display. Where (x, y) is
 * origin at the bottom left corner and (w, h) is the width and height.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param x  A pointer to a RwReal for the x-coordinate of the display's xOrigin.
 * \param y  A pointer to a RwReal for the y-coordinate of the display's yOrigin.
 * \param w  A pointer to a RwReal for the width of the display.
 * \param h  A pointer to a RwReal for the height of the display.
 * \return TRUE if successful or FALSE if there is an error.
 * \see Rt2dDeviceSetMetric
 * \see Rt2dDeviceGetStep
 */
RwBool
Rt2dDeviceGetMetric(RwReal * x, RwReal * y, RwReal * w, RwReal * h)
{
    RWAPIFUNCTION(RWSTRING("Rt2dDeviceGetMetric"));

    *x = Rt2dGlobals.deviceX;
    *y = Rt2dGlobals.deviceY;
    *w = Rt2dGlobals.deviceW;
    *h = Rt2dGlobals.deviceH;

    RWRETURN(TRUE);
}

/****************************************************************************/

RwReal
_rt2dGetTolerance(void)
{
    RWFUNCTION(RWSTRING("_rt2dGetTolerance"));

    /* determine an object space tolerance */
    if (!Rt2dGlobals.sqToleranceValid)
    {
        RwMatrix           *mat;
        RwV3d               p;

        mat = _rt2dCTMGetInverse();
        p.x = (Rt2dGlobals.vw.x / Rt2dGlobals.vp.x) *
            Rt2dGlobals.layerDepth * Rt2dGlobals.tolerance;

        p.y = 0.0f;
        p.z = 0.0f;
        RwV3dTransformVector(&p, &p, mat);
        Rt2dGlobals.sqTolerance = RwV3dDotProduct(&p, &p);
        Rt2dGlobals.sqToleranceValid = TRUE;
    }

    RWRETURN(Rt2dGlobals.sqTolerance);
}

/****************************************************************************/

/**
 * \ingroup rt2ddevice
 * \ref Rt2dDeviceSetFlat
 * is used to define the maximum pixel error that is
 * tolerated when the curved parts of paths are tessellated, either
 * explicitly using Rt2dPathFlatten or during rendering. Tolerance must
 * be greater than zero.
 * The default tolerance value is 0.5.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param r  A RwReal value equal to the tolerance
 * \return TRUE if successful or FALSE if there is an error
 * \see Rt2dPathFlatten
 */
RwBool
Rt2dDeviceSetFlat(RwReal r)
{
    RWAPIFUNCTION(RWSTRING("Rt2dDeviceSetFlat"));

    if (r > 0.0f)
    {
        Rt2dGlobals.tolerance = r;
        Rt2dGlobals.sqToleranceValid = FALSE;

        RWRETURN(TRUE);
    }

    RWRETURN(FALSE);
}

/****************************************************************************/

/**
 * \ingroup rt2ddevice
 * \ref Rt2dDeviceGetClippath
 * is used to construct a 2D path that maps
 * directly to the camera raster's clipping boundary. The resulting path
 * thus encloses the whole camera view at the current layer depth.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param path  Pointer to the path
 * \return TRUE if successful or FALSE if there is an error
 * \see Rt2dDeviceSetLayerDepth
 */
RwBool
Rt2dDeviceGetClippath(Rt2dPath * path)
{
    RwMatrix           *mat;
    RwV3d               in[4], out[4];
    RwV2d               vw;
    RwReal              layerDepth;

    RWAPIFUNCTION(RWSTRING("Rt2dDeviceGetClippath"));

    if (RwCameraGetProjection(Rt2dGlobals.viewCamera) == rwPARALLEL)
        layerDepth = (RwReal) 1.0;
    else
        layerDepth = Rt2dGlobals.layerDepth;

    vw = Rt2dGlobals.vw;

    mat = _rt2dCTMGetInverse();

    in[0].x = -vw.x * layerDepth * 0.5f;
    in[0].y = -vw.y * layerDepth * 0.5f;
    in[0].z = 0.0f;
    in[1].x = -vw.x * layerDepth * 0.5f;
    in[1].y = vw.y * layerDepth * 0.5f;
    in[1].z = 0.0f;
    in[2].x = vw.x * layerDepth * 0.5f;
    in[2].y = vw.y * layerDepth * 0.5f;
    in[2].z = 0.0f;
    in[3].x = vw.x * layerDepth * 0.5f;
    in[3].y = -vw.y * layerDepth * 0.5f;
    in[3].z = 0.0f;

    RwV3dTransformPoints(out, in, 4, mat);

    Rt2dPathEmpty(path);
    Rt2dPathMoveto(path, out[0].x, out[0].y);
    Rt2dPathLineto(path, out[1].x, out[1].y);
    Rt2dPathLineto(path, out[2].x, out[2].y);
    Rt2dPathLineto(path, out[3].x, out[3].y);
    Rt2dPathClose(path);

    RWRETURN(TRUE);
}

/****************************************************************************/

/****************************************************************************/
#define rt2dCLIPMASK 0x0f
#define rt2dXLOCLIP 0x01
#define rt2dXHICLIP 0x02
#define rt2dYLOCLIP 0x04
#define rt2dYHICLIP 0x08

/**
 * \ingroup rt2ddevice
 * \ref Rt2dVisible
 * is used to determine if a box of the specified position
 * and dimensions is visible in the current camera view. Use this
 * function to determine whether a particular graphic can be seen and
 * needs to be rendered.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param x  A RwReal value equal to the x-coordinate of the lower-left
 *       of the box.
 * \param y  A RwReal value equal to the y-coordinate of the lower-left
 *       of the box.
 * \param w  A RwReal value equal to the width of the box.
 * \param h  A RwReal value equal to the height of the box.
 * \return TRUE if the box is visible or FALSE if not.
 * \see Rt2dDeviceGetClippath
 */
RwBool
Rt2dVisible(RwReal x, RwReal y, RwReal w, RwReal h)
{
    RwV3d               in[4], out[4];
    RwV2d               vw;
    RwInt32             i, clipAnd, clipOr;
    RwReal              layerDepth;

    RWAPIFUNCTION(RWSTRING("Rt2dVisible"));

    if (RwCameraGetProjection(Rt2dGlobals.viewCamera) == rwPARALLEL)
        layerDepth = (RwReal) 1.0;
    else
        layerDepth = Rt2dGlobals.layerDepth;

    vw = Rt2dGlobals.vw;

    in[0].x = x;
    in[0].y = y;
    in[0].z = 0.0f;
    in[1].x = x + w;
    in[1].y = y;
    in[1].z = 0.0f;
    in[2].x = x + w;
    in[2].y = y + h;
    in[2].z = 0.0f;
    in[3].x = x;
    in[3].y = y + h;
    in[3].z = 0.0f;

    RwV3dTransformPoints(out, in, 4, _rt2dCTMGet());

    clipAnd = rt2dCLIPMASK;

    for (i = 0; i < 4; i++)
    {
        clipOr = 0;

        if (out[i].x > vw.x * layerDepth * (RwReal) (0.5))
        {
            clipOr |= rt2dXHICLIP;
        }

        if (out[i].x < -vw.x * layerDepth * (RwReal) (0.5))
        {
            clipOr |= rt2dXLOCLIP;
        }

        if (out[i].y > vw.y * layerDepth * (RwReal) (0.5))
        {
            clipOr |= rt2dYHICLIP;
        }

        if (out[i].y < -vw.y * layerDepth * (RwReal) (0.5))
        {
            clipOr |= rt2dYLOCLIP;
        }

        clipAnd &= clipOr;
    }

    /* wholly on one side of a boundary */
    if (clipAnd)
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}

/****************************************************************************/

/**
 * \ingroup rt2ddevice
 * \ref Rt2dDeviceSetLayerDepth
 * is used to define the distance to a plane
 * parallel to the camera view-plane on which any 2D rendering will take
 * place. This function is useful when an application wishes to mix 3D
 * and 2D rendering in the same camera view. The depth must be greater
 * than zero.
 * The default layer depth is 1.0.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param depth  A RwReal value equal to the camera distance at which 2D
 *       is performed
 * \return TRUE if successful or FALSE if there is an error
 * \see Rt2dDeviceGetClippath
 * \see Rt2dDeviceSetMetric
 */
RwBool
Rt2dDeviceSetLayerDepth(RwReal depth)
{
    RWAPIFUNCTION(RWSTRING("Rt2dDeviceSetLayerDepth"));

    if (depth > 0.0f)
    {
        Rt2dGlobals.layerDepth = depth;
        Rt2dGlobals.CTMValid = FALSE;

        RWRETURN(TRUE);
    }

    RWRETURN(FALSE);
}

/****************************************************************************/

/**
 * \ingroup rt2ddevice
 * \ref Rt2dDeviceSetCamera
 * is used to define the specified camera to
 * be used for the output of any further 2D rendering. The 2D toolkit caches
 * some values of the camera and does not keep track of any changes made to the
 * specified camera. This functions must be called if the camera's view window and
 * raster dimensions are changed.
 * The metrics are reset to the default normalized values. If these were set
 * to map a different space, it must be reset after this function.
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param cam  Pointer to the camera
 * \return TRUE if successful or FALSE if there is an error
 * \see Rt2dOpen
 * \see Rt2dDeviceGetMetric
 * \see Rt2dDeviceSetMetric
 */
RwBool
Rt2dDeviceSetCamera(RwCamera * cam)
{
    RwRaster           *ras;

    RWAPIFUNCTION(RWSTRING("Rt2dDeviceSetCamera"));

    if (cam)
    {
        ras = RwCameraGetRaster(cam);
        if (ras)
        {
            RwInt32             rasterWidth = RwRasterGetWidth(ras);
            RwInt32             rasterHeight = RwRasterGetHeight(ras);

            Rt2dGlobals.vp.x = (RwReal) rasterWidth;
            Rt2dGlobals.vp.y = (RwReal) rasterHeight;
            Rt2dGlobals.vw.x = RwCameraGetViewWindow(cam)->x * 2.0f;
            Rt2dGlobals.vw.y = RwCameraGetViewWindow(cam)->y * 2.0f;


            /* Change the this camera's end update. */
            if ((cam->endUpdate != PS2CameraEndUpdate2d) &&
                (Rt2dGlobals.viewCamera != cam))
            {
                Rt2dGlobals.cameraEndUpdate = cam->endUpdate;

                cam->endUpdate = PS2CameraEndUpdate2d;
            }

            Rt2dGlobals.viewCamera = cam;
            Rt2dGlobals.CTMValid = FALSE;

            Rt2dGlobals.deviceX = 0.0;
            Rt2dGlobals.deviceY = 0.0;


            if (Rt2dGlobals.vw.x > Rt2dGlobals.vw.y)
            {
                Rt2dGlobals.deviceW = Rt2dGlobals.vw.x / Rt2dGlobals.vw.y;
                Rt2dGlobals.deviceH = 1.0;
            }
            else
            {
                Rt2dGlobals.deviceW = 1.0;
                Rt2dGlobals.deviceH = Rt2dGlobals.vw.y / Rt2dGlobals.vw.x;
            }

            RWRETURN(TRUE);
        }
    }
    else
    {
        Rt2dGlobals.viewCamera = cam;
    }

    RWRETURN(FALSE);
}

/**
 * \ingroup rt2ddevice
 * \ref Rt2dDeviceGetCamera
 * is used to get a pointer to the camera previously defined by
 * \ref Rt2dDeviceSetCamera
*
 * \return a pointer to the used camera
 * \see Rt2dDeviceSetCamera
 */
RwCamera *
Rt2dDeviceGetCamera(void)
{
    RWAPIFUNCTION(RWSTRING("Rt2dDeviceGetCamera"));

    RWRETURN(Rt2dGlobals.viewCamera);
}

/**
 * \ingroup rt2dsub
 * \ref Rt2dSetPipelineFlags
 * is used to add additional flags to to speed up the pipeline
 * processing. Any of the \ref RwIm3DTransformFlags enumerated values can be
 * used, but only rwIM3D_NOCLIP and rwIM3D_ALLOPAQUE produce useful results.
 *
 * The selected flag settings will be logically ORed into the Immediate Mode
 * flags variable, so existing flag settings will not be cleared.
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param flags  Flags to be passed to the pipeline
 * \return TRUE
 * \see RwIm3DTransformFlags
 */
RwBool
Rt2dSetPipelineFlags(RwUInt32 flags)
{

    RWAPIFUNCTION(RWSTRING("Rt2dSetPipelineFlags"));
    Rt2dGlobals.TransformFlags = flags;
    RWRETURN(TRUE);
}

/****************************************************************************/

/**
 * \ingroup rt2dsub
 * \ref Rt2dOpen
 * is used to perform various initializations that are required
 * before the 2D plugin API can be used. The specified camera's raster
 * will be used to display the output of any 2D rendering (the output
 * device can be redefined using \ref Rt2dDeviceSetCamera).
 * This function must be called before executing any other 2D drawing
 * functions. The function \ref Rt2dClose complements Rt2dOpen and should be
 * used before closing down an application.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param cam  Pointer to the camera.
 * \see Rt2dClose
 * \see Rt2dDeviceSetCamera
 */
void
Rt2dOpen(RwCamera * cam)
{
    RWAPIFUNCTION(RWSTRING("Rt2dOpen"));

    Rt2dSetPipelineFlags(0);

    _rt2dTriangulateOpen();
    _rt2dPathOpen();
    _rt2dBrushOpen();
    _rt2dFontOpen();
    _rt2dObjectOpen();
    _rt2dGstateOpen();

#if (defined(SKY2_DRVMODEL_H))
    _rt2dPS2PipeOpen();
#endif /* (defined(SKY2_DRVMODEL_H)) */

    Rt2dDeviceSetCamera(cam);

    RWRETURNVOID();
}

/****************************************************************************/

/**
 * \ingroup rt2dsub
 * \ref Rt2dClose
 * is used to perform various clean-up operations that are
 * required to close- down the 2D tools API. This function complements
 * \ref Rt2dOpen and should be used before closing down an application.
 *
 * Note that Rt2dClose does not destroy any CTMs, brushes, paths or fonts
 * that have been created by an application; these must be destroyed by
 * the application itself.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \return None
 *
 * \see Rt2dOpen
 */
void
Rt2dClose(void)
{
    RWAPIFUNCTION(RWSTRING("Rt2dClose"));

#if (defined(SKY2_DRVMODEL_H))
    _rt2dPS2PipeClose();
#endif /* (defined(SKY2_DRVMODEL_H)) */

    _rt2dGstateClose();
    _rt2dObjectClose();
    _rt2dFontClose();
    _rt2dBrushClose();
    _rt2dPathClose();
    _rt2dTriangulateClose();

    Rt2dDeviceSetCamera((RwCamera *)NULL);

    RWRETURNVOID();
}

/****************************************************************************/
void
_rt2dGstateClose(void)
{
    RwInt32 i;

    RWFUNCTION(RWSTRING("_rt2dGstateClose"));

    for( i=0 ; i<32 ; i++)
    {
        if( Rt2dGlobals.ctm[i] )
    {
            RwMatrixDestroy(Rt2dGlobals.ctm[i]);
            Rt2dGlobals.ctm[i] = (RwMatrix *)NULL;
        }
    }

    Rt2dGlobals.mtos = 0;

    if (Rt2dGlobals.triIdxCache != NULL)
    {
        RwFree(Rt2dGlobals.triIdxCache);

        Rt2dGlobals.triIdxCache = NULL;
        Rt2dGlobals.triIdxCacheSize = 0;
    }


    RWRETURNVOID();
}

/****************************************************************************/
RwBool
_rt2dGstateOpen(void)
{
    RwInt32             i;

    RWFUNCTION(RWSTRING("_rt2dGstateOpen"));

    /* establish stack */
    Rt2dGlobals.mtos = 0;
    Rt2dGlobals.ctm[0] = RwMatrixCreate();

    /* defaults */

    Rt2dCTMSetIdentity();
    Rt2dDeviceSetFlat((RwReal) (0.5));
    Rt2dDeviceSetLayerDepth(1.0f);

    Rt2dGlobals.flatDepth = 7;

    RwMatrixSetIdentity(&Rt2dGlobals.metricMtx);

    /* constant vertex indices */
    for (i = 0; i < VERTEXCACHESIZE; i++)
    {
        Rt2dGlobals.topo[i] = (RwImVertexIndex) i;
    }

    for( i=1 ; i<32 ; i++)
    {
        Rt2dGlobals.ctm[i] = (RwMatrix *)NULL;
    }

    Rt2dGlobals.viewCamera = (RwCamera *)NULL;

    Rt2dGlobals.deviceX = 0.0;
    Rt2dGlobals.deviceY = 0.0;
    Rt2dGlobals.deviceW = 1.0;
    Rt2dGlobals.deviceH = 1.0;

    Rt2dGlobals.triIdxCache = NULL;
    Rt2dGlobals.triIdxCacheSize = 0;

    RWRETURN(TRUE);
}

/****************************************************************************/

 /* All of the Following functions are for Debug version only
  * They each have a macro counterpart which is used in the
  * Release builds.
  */

#if ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) )

/**
 * \ingroup rt2dctm
 * \ref Rt2dCTMRead
 * is used to copy the current CTM matrix into the provided matrix.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for release versions of an
 * application.
 *
 * \param result  the matrix to store the CTM.
 *
 * \return The matrix if successful or NULL if there is an error.
 *
 */
RwMatrix           *
Rt2dCTMRead(RwMatrix * result)
{
    RWAPIFUNCTION(RWSTRING("Rt2dCTMRead"));

    result = Rt2dCTMReadMacro(result);

    RWRETURN(result);
}

#endif /* ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) ) */

/****************************************************************************/

/************************************************************************
 * WST/Working Set Tuner build of ppforest on PC suggests ordering
 * Rt2dCTMPush
 * Rt2dCTMPop
 * Rt2dCTMScale
 * Rt2dCTMTranslate
 * _rt2dCTMGet
 */

/**
 * \ingroup rt2dctm
 * \ref Rt2dCTMPush
 * is used to create a new transformation matrix and place it
 * at the top of the matrix stack, that is, make it the current
 * transformation matrix (CTM).
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 *
 *
 * \return TRUE if successful or FALSE if there is an error
 * \see Rt2dCTMPop
 * \see Rt2dCTMRotate
 * \see Rt2dCTMTranslate
 * \see Rt2dCTMScale
 * \see Rt2dCTMSetIdentity
 */
RwBool
Rt2dCTMPush(void)
{
    const RwBool        result = (Rt2dGlobals.mtos < 31);

    RWAPIFUNCTION(RWSTRING("Rt2dCTMPush"));

    if (result)
    {
        Rt2dGlobals.mtos++;
        if( NULL == Rt2dGlobals.ctm[Rt2dGlobals.mtos] )
        {
            Rt2dGlobals.ctm[Rt2dGlobals.mtos] = RwMatrixCreate();
        }
        RwMatrixCopy(Rt2dGlobals.ctm[Rt2dGlobals.mtos],
                     Rt2dGlobals.ctm[Rt2dGlobals.mtos - 1]);
    }

    RWRETURN(result);
}

/****************************************************************************/
RwBool
_rt2dCTMPush(RwMatrix *matrix)
{
    const RwBool        result = (Rt2dGlobals.mtos < 31);

    RWFUNCTION(RWSTRING("_rt2dCTMPush"));
    RWASSERT(matrix);

    if (result)
    {
        Rt2dGlobals.mtos++;
        if( NULL == Rt2dGlobals.ctm[Rt2dGlobals.mtos] )
        {
            Rt2dGlobals.ctm[Rt2dGlobals.mtos] = RwMatrixCreate();
        }
        if( NULL == matrix )
        {
            RwMatrixCopy(Rt2dGlobals.ctm[Rt2dGlobals.mtos],
                         Rt2dGlobals.ctm[Rt2dGlobals.mtos - 1]);
        }
        else
        {
            RwMatrixMultiply(Rt2dGlobals.ctm[Rt2dGlobals.mtos],
                             matrix,
                             Rt2dGlobals.ctm[Rt2dGlobals.mtos - 1]
                             );
        }
    }

    Rt2dGlobals.CTMValid = FALSE;
    Rt2dGlobals.sqToleranceValid = FALSE;

    RWRETURN(result);
}

/****************************************************************************/

RwBool
_rt2dCTMSet(RwMatrix *matrix)
{
    const RwBool        result = (Rt2dGlobals.mtos < 31);

    RWFUNCTION(RWSTRING("_rt2dCTMSet"));
    RWASSERT(matrix);

    if (result)
    {
        Rt2dGlobals.mtos++;
        if( NULL == Rt2dGlobals.ctm[Rt2dGlobals.mtos] )
        {
            Rt2dGlobals.ctm[Rt2dGlobals.mtos] = RwMatrixCreate();
        }

        RwMatrixCopy(Rt2dGlobals.ctm[Rt2dGlobals.mtos], matrix);
    }

    Rt2dGlobals.CTMValid = FALSE;
    Rt2dGlobals.sqToleranceValid = FALSE;

    RWRETURN(result);
}

/****************************************************************************/

/**
 * \ingroup rt2dctm
 * \ref Rt2dCTMPop
 * is used to remove the current transformation matrix (CTM),
 * that is, to destroy the matrix at the top of the matrix stack. The
 * matrix previously below the current one in the matrix stack, if any,
 * now becomes the current transformation matrix.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \return TRUE if successful or FALSE if there is an error
 * \see Rt2dCTMPush
 * \see Rt2dCTMRotate
 * \see Rt2dCTMTranslate
 * \see Rt2dCTMScale
 * \see Rt2dCTMSetIdentity
 */
RwBool
Rt2dCTMPop(void)
{
    RwBool              result = (Rt2dGlobals.mtos > 0);

    RWAPIFUNCTION(RWSTRING("Rt2dCTMPop"));

    if (result)
    {
        Rt2dGlobals.mtos--;

        Rt2dGlobals.CTMValid = FALSE;
        Rt2dGlobals.sqToleranceValid = FALSE;

    }

    RWRETURN(result);
}

/****************************************************************************/

/**
 * \ingroup rt2dctm
 * \ref Rt2dCTMScale
 * is used to apply a scale transformation to the current
 * transformation matrix (CTM) using the specified x- and y-scale
 * factors.
 * Note that the scale is preconcatenated with the CTM.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param x  A RwReal value equal to the scale factor in the x-direction.
 * \param y  A RwReal value equal to the scale factor in the y-direction.
 * \return TRUE if successful or FALSE if there is an error
 * \see Rt2dCTMRotate
 * \see Rt2dCTMTranslate
 * \see Rt2dCTMPush
 * \see Rt2dCTMPop
 * \see Rt2dCTMSetIdentity
 */
RwBool
Rt2dCTMScale(RwReal x, RwReal y)
{
    RwV3d               scale;

    RWAPIFUNCTION(RWSTRING("Rt2dCTMScale"));

    scale.x = x;
    scale.y = y;
    scale.z = 1.0f;
    RwMatrixScale(Rt2dGlobals.ctm[Rt2dGlobals.mtos], &scale,
                  rwCOMBINEPRECONCAT);

    Rt2dGlobals.CTMValid = FALSE;
    Rt2dGlobals.sqToleranceValid = FALSE;

    RWRETURN(TRUE);
}

/****************************************************************************/

/**
 * \ingroup rt2dctm
 * \ref Rt2dCTMTranslate
 * is used to apply a translation to the current
 * transformation matrix (CTM) using the specified x- and y-components.
 * Note that the translation is preconcatenated with the CTM.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param x  A RwReal value equal to the translation in the x-direction.
 * \param y  A RwReal value equal to the translation in the y-direction.
 * \return TRUE if successful or FALSE if there is an error
 * \see Rt2dCTMRotate
 * \see Rt2dCTMScale
 * \see Rt2dCTMPush
 * \see Rt2dCTMPop
 * \see Rt2dCTMSetIdentity
 */
RwBool
Rt2dCTMTranslate(RwReal x, RwReal y)
{
    RwV3d               translate;

    RWAPIFUNCTION(RWSTRING("Rt2dCTMTranslate"));

    translate.x = x;
    translate.y = y;
    translate.z = 0.0f;
    RwMatrixTranslate(Rt2dGlobals.ctm[Rt2dGlobals.mtos], &translate,
                      rwCOMBINEPRECONCAT);

    Rt2dGlobals.CTMValid = FALSE;
    Rt2dGlobals.sqToleranceValid = FALSE;

    RWRETURN(TRUE);
}

/****************************************************************************/

RwMatrix           *
_rt2dCTMGet(void)
{
    RwV3d               scale, translate;
    RwReal              mindim, layerDepth;
    RwMatrix            temp;
    RwV2d               vw;

    RWFUNCTION(RWSTRING("_rt2dCTMGet"));

    if ((!Rt2dGlobals.CTMValid))
    {
        if (RwCameraGetProjection(Rt2dGlobals.viewCamera) == rwPARALLEL)
            layerDepth = (RwReal) 1.0;
        else
            layerDepth = Rt2dGlobals.layerDepth;

        RwMatrixMultiply(&temp, Rt2dGlobals.ctm[Rt2dGlobals.mtos], &Rt2dGlobals.metricMtx);

        vw = Rt2dGlobals.vw;

        scale.x = layerDepth;
        scale.y = layerDepth;
        scale.z = 1.0f;
        RwMatrixScale(&temp, &scale, rwCOMBINEPOSTCONCAT);

        mindim = vw.x < vw.y ? vw.x : vw.y;
        scale.x = -mindim;
        scale.y = mindim;
        scale.z = 1.0f;
        RwMatrixScale(&temp, &scale, rwCOMBINEPOSTCONCAT);

        translate.x = (vw.x * 0.5f * layerDepth);
        translate.y = -(vw.y * 0.5f * layerDepth);
        translate.z = 0.0f;
        RwMatrixTranslate(&temp, &translate, rwCOMBINEPOSTCONCAT);

        /* Save the current 2d CTM, before the multi with the view mat */
        RwMatrixCopy(&Rt2dGlobals.CTM2d, &temp);

        /* iCTM2d not interested in camera position & orientation */
        RwMatrixInvert(&Rt2dGlobals.iCTM2d, &temp);

        RWMATRIXPRINT(&temp);
        RWMATRIXPRINT(&Rt2dGlobals.iCTM2d);

        Rt2dGlobals.CTMValid = TRUE;
    }

    /* because Im3D concat the viewMat-1 */

    RwMatrixMultiply(&Rt2dGlobals.CTM, &Rt2dGlobals.CTM2d,
                     RwFrameGetLTM(RwCameraGetFrame
                                   (Rt2dGlobals.viewCamera)));

    RWMATRIXPRINT(&Rt2dGlobals.CTM);

    RWRETURN(&Rt2dGlobals.CTM);
}

RwMatrix           *
_rt2dCTMGetDirect(void)
{
    RWFUNCTION(RWSTRING("_rt2dCTMGetDirect"));

    RWRETURN(Rt2dGlobals.ctm[Rt2dGlobals.mtos]);
}

