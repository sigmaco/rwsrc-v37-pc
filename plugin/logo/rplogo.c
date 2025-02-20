/*
 * Logo plugin
 */

#define LOGOGENx
#define RVLOGOx

#ifdef LOGOGEN
#ifdef RVLOGO
#define LOGOFILE "rvdata.h"
#define LOGOSRC "rvlogo.png"
#else
#define LOGOFILE "rwdata.h"
#define LOGOSRC "rwlogo.png"
#endif
#endif
/**********************************************************************
 *
 * File :     logo.c
 *
 * Abstract : Add Criterion Logo
 *
 **********************************************************************
 *
 * This file is a product of Criterion Software Ltd.
 *
 * This file is provided as is with no warranties of any kind and is
 * provided without any obligation on Criterion Software Ltd. or
 * Canon Inc. to assist in its use or modification.
 *
 * Criterion Software Ltd. will not, under any
 * circumstances, be liable for any lost revenue or other damages arising
 * from the use of this file.
 *
 * Copyright (c) 1998 Criterion Software Ltd.
 * All Rights Reserved.
 *
 * RenderWare is a trademark of Canon Inc.
 *
 ************************************************************************/

/**
 * \ingroup rplogo
 * \page rplogooverview RpLogo Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rplogo.h
 * \li \b Libraries: rwcore, rplogo
 * \li \b Plugin \b attachments: \ref RpLogoPluginAttach
 *
 * \subsection logooverview Overview
 *
 * This plugin renders a RenderWare logo on the screen. The logo will appear
 * automatically when the plugin is attached. It is possible to turn the
 * appearance of the logo on/off after the plugin is attached. It is not
 * possible to change the logo's image.
 *
 * This plugin is intended for use in examples or demos. It should not be
 * used in the final product.
 */

/*--- Include files ---*/

#include <stdio.h>
#include <stdlib.h>

#include "rpplugin.h"
#include <rpdbgerr.h>

#include "rplogo.h"
#include "rtfsyst.h"

#ifdef LOGOGEN
#include "rtpng.h"
#endif

#ifdef LOGOGEN
static RwInt32      LogoRasterHeight;
#ifdef RVLOGO
#define LOGOSRC "rvlogo.png"
#else  /* RVLOGO */
#define LOGOSRC "rwlogo.png"
#endif /* RVLOGO */
#else  /* LOGOGEN */
#ifdef RVLOGO
#include  "rvdata.h"
#else  /* RVLOGO */
#include  "rwdata.h"
#endif /* RVLOGO */
#endif /* LOGOGEN */

/*--- Local Structures ---*/

/* Data structure that is used to extend the RenderWare global data for
 * use by the Logo plugin
 */

typedef struct LogoGlobals LogoGlobals;
struct LogoGlobals
{
    RwRaster           *raster;
    RpLogoPosition      pos;
    RwRect              rect;
};

typedef struct LogoCamera LogoCamera;
struct LogoCamera
{
    RwBool              state;
    RwCameraEndUpdateFunc cameraEndUpdate;
    RwCameraBeginUpdateFunc cameraBeginUpdate;
    RwUInt32            pad;
};

/*--- Local Definitions ---*/

/*--- Global Variables ---*/

/* These offsets define the offsets for the Logo private data into
 * the respective structures
 */

static RwInt32      GlobalOffset = -1; /* Offset into global data */
static RwInt32      CameraOffset = -1; /* Offset into camera data */

#if (defined(RWDEBUG))
long                rpLogoStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

#ifdef LOGOGEN
static RwReal       LogoImageRatio;
#endif

/************************************************************************
 *
 *      Function:       LogoCameraDestroy()
 *
 *      Description:    Called from RenderWare whenever a camera is destroyed.
 *                      This function is used to undo what was done in the
 *                      corresponding constructor function
 *                      LogoCameraCreate()
 *
 *      Parameters:     The parameters are as specified in the callback
 *                      function definition for RwCameraRegisterPlugin
 *
 *      Return Value:   The return value is specified in the callback
 *                      function definition for RwCameraRegisterPlugin
 *
 ************************************************************************/
static void        *
LogoCameraDestroy(void *object,
                  RwInt32 __RWUNUSED__ offset,
                  RwInt32 __RWUNUSED__ size)
{
    RwCamera           *camera = (RwCamera *) object;

    RWFUNCTION(RWSTRING("LogoCameraDestroy"));
    RWASSERT(0 <= GlobalOffset);
    RWASSERT(0 <= CameraOffset);

    RWASSERT(object);
    RWASSERT(0 <= offset);

    /* Restore the original EndCameraUpdate function */

    RWPLUGINOFFSET(LogoCamera, camera, CameraOffset)->state = FALSE;

    RWRETURN(object);
}

#define _STRINGIFY(X) #X
#define _STRINGIFY_EXP(X) _STRINGIFY(X)

#ifdef _XBOX
/*
These two optimizations used together seem to cause bad code to be generated for
the function call prolog & epilog in LogoCameraBeginUpdate
*/
#pragma optimize( "gy", off )
#pragma message ( __FILE__ "(" _STRINGIFY_EXP(__LINE__) "): MS optimization bug work around\n")
#endif


/************************************************************************
 *
 *      Function:       LogoBeginCameraUpdate()
 *
 *      Description:    Called from within RwBeginCameraUpdate(). Note -
 *                      default implementation is called afterwards.
 *
 ************************************************************************/
static RwCamera    *
LogoCameraBeginUpdate(RwCamera * camera)
{
    RwCameraBeginUpdateFunc defaultFunc;
    RwCamera           *result;
    LogoGlobals        *globals;
    RwRaster           *logoRaster;
    RpLogoPosition      pos;
    RwBool              state;

    RWFUNCTION(RWSTRING("LogoCameraBeginUpdate"));
    RWASSERT(0 <= GlobalOffset);
    RWASSERT(0 <= CameraOffset);

    /* NOTE: we calc the rectangle here so that:
     * (a) it is always valid in time for any rendering
     *    funcs that query it (e.g the demoskel menu)
     * (b) it is w.r.t the currently-updating camera
     *    (previously it was w.r.t the *previous* camera
     *    in multi-camera apps!) */

    globals = RWPLUGINOFFSET(LogoGlobals,
                             RwEngineInstance, GlobalOffset);
    logoRaster = globals->raster;
    pos = globals->pos;

    defaultFunc =
        RWPLUGINOFFSET(LogoCamera, camera,
                       CameraOffset)->cameraBeginUpdate;

    /* Render the Logo */
    state = RWPLUGINOFFSET(LogoCamera, camera, CameraOffset)->state;

    if (logoRaster)
    {
        RwRaster           *cameraRaster = RwCameraGetRaster(camera);
        RwRect              rect;
        const RwV2d        *vw;
        RwReal              ratio;
        RwInt32             rasterWidth, rasterHeight;

        rect.w = rasterWidth = RwRasterGetWidth(cameraRaster);
        rect.h = rasterHeight = RwRasterGetHeight(cameraRaster);

        /* use knowledge of pixel and aspect ratios to maintain logo aspect
         */

        vw = RwCameraGetViewWindow(camera);
        ratio = (rect.h * vw->x) / (LogoImageRatio * rect.w * vw->y);

        rect.w = RwInt32FromRealMacro((RwReal) rect.w * 0.1f);
        rect.h = RwInt32FromRealMacro((RwReal) rect.w * ratio);

        if (rect.h > rasterHeight - 48)
        {
            rect.h = rasterHeight - 48;
            rect.w = RwInt32FromRealMacro((RwReal) rect.h / ratio);
        }

        /* calculate X co-ordinate */
        switch (pos)
        {
            case rpLOGOLEFT:
            case rpLOGOTOPLEFT:
            case rpLOGOBOTTOMLEFT:
                rect.x = cameraRaster->nOffsetX + 16;
                break;

            case rpLOGORIGHT:
            case rpLOGOTOPRIGHT:
            case rpLOGOBOTTOMRIGHT:
                rect.x =
                    cameraRaster->nOffsetX + rasterWidth - rect.w - 16;
                break;

            case rpLOGOTOP:
            case rpLOGOCENTER:
            case rpLOGOBOTTOM:
                rect.x =
                    cameraRaster->nOffsetX +
                    ((rasterWidth - rect.w) / 2);
                break;

            case rpNALOGOPOSITION:
                RWASSERT(pos != rpNALOGOPOSITION);
                break;

            case rpLOGOPOSITIONFORCEENUMSIZEINT:
                break;
        }

        /* calculate Y co-ordinate */
        switch (pos)
        {
            case rpLOGOTOP:
            case rpLOGOTOPLEFT:
            case rpLOGOTOPRIGHT:
                rect.y = cameraRaster->nOffsetY + 24;
                break;

            case rpLOGOBOTTOM:
            case rpLOGOBOTTOMLEFT:
            case rpLOGOBOTTOMRIGHT:
                rect.y =
                    cameraRaster->nOffsetY + rasterHeight - rect.h - 24;
                break;

            case rpLOGOLEFT:
            case rpLOGOCENTER:
            case rpLOGORIGHT:
                rect.y =
                    cameraRaster->nOffsetY +
                    ((rasterHeight - rect.h) / 2);
                break;

            case rpNALOGOPOSITION:
                RWASSERT(pos != rpNALOGOPOSITION);
                break;

            case rpLOGOPOSITIONFORCEENUMSIZEINT:
                break;
        }

        /* Update the logo rendering rect... */
        RWPLUGINOFFSET(LogoGlobals, RwEngineInstance,
                       GlobalOffset)->rect = rect;
    }
    /* Then call the default function */

    result = defaultFunc(camera);

    RWRETURN(result);

}

/************************************************************************
 *
 *      Function:       LogoEndCameraUpdate()
 *
 *      Description:    Called from within RwEndCameraUpdate(). Note -
 *                      default implementation is called afterwards.
 *
 ************************************************************************/
static RwCamera    *
LogoCameraEndUpdate(RwCamera * camera)
{
    RwCamera           *result;
    RwCameraEndUpdateFunc defaultFunc;
    LogoGlobals        *globals;
    RwRaster           *logoRaster;
    RpLogoPosition      pos;
    RwBool              state;

    RWFUNCTION(RWSTRING("LogoCameraEndUpdate"));
    RWASSERT(0 <= GlobalOffset);
    RWASSERT(0 <= CameraOffset);

    globals = RWPLUGINOFFSET(LogoGlobals,
                             RwEngineInstance, GlobalOffset);
    logoRaster = globals->raster;
    pos = globals->pos;

    defaultFunc =
        RWPLUGINOFFSET(LogoCamera, camera,
                       CameraOffset)->cameraEndUpdate;

    /* Render the Logo */
    state = RWPLUGINOFFSET(LogoCamera, camera, CameraOffset)->state;

    if (logoRaster)
    {
        RwRect             *rect;

        /* Get the current logo rendering rect... */
        rect =
            &(RWPLUGINOFFSET
              (LogoGlobals, RwEngineInstance, GlobalOffset)->rect);

        /* only draw if it makes sense */
        if (state &&
            (rect->x > 0) && (rect->y > 0) &&
            (rect->w > 0) && (rect->h > 0))
        {
            RwBool              zTestEnable, zWriteEnable;
            RwBool              vAplhaEnable;
            RwInt32             srcBlend, dstBlend;
            RwRaster           *txtRaster;
            RwInt32             txtFilter;
            RwCullMode          cullMode;
            RwTextureAddressMode addrModeU;
            RwTextureAddressMode addrModeV;

            RwIm2DVertex        logoVerts[4];
            RwUInt32            i;

            RwReal              recipZ;
            RwReal              nearScreenZ;

            recipZ = 1.0f / RwCameraGetNearClipPlane(camera);
            nearScreenZ = RwIm2DGetNearScreenZ();

            RwRenderStateGet(rwRENDERSTATEZTESTENABLE,
                             (void *) &zTestEnable);
            RwRenderStateGet(rwRENDERSTATEZWRITEENABLE,
                             (void *) &zWriteEnable);

            RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE,
                             (void *) &vAplhaEnable);
            RwRenderStateGet(rwRENDERSTATESRCBLEND, (void *) &srcBlend);
            RwRenderStateGet(rwRENDERSTATEDESTBLEND,
                             (void *) &dstBlend);

            RwRenderStateGet(rwRENDERSTATETEXTURERASTER,
                             (void *) &txtRaster);
            RwRenderStateGet(rwRENDERSTATETEXTUREFILTER,
                             (void *) &txtFilter);

            RwRenderStateGet(rwRENDERSTATECULLMODE, (void *) &cullMode);

            RwRenderStateGet(rwRENDERSTATETEXTUREADDRESSU,
                             (void *) &addrModeU);
            RwRenderStateGet(rwRENDERSTATETEXTUREADDRESSV,
                             (void *) &addrModeV);

            RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void *) FALSE);
            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void *) FALSE);

            RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,
                             (void *) FALSE);
            RwRenderStateSet(rwRENDERSTATESRCBLEND,
                             (void *) rwBLENDSRCALPHA);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND,
                             (void *) rwBLENDINVSRCALPHA);

            RwRenderStateSet(rwRENDERSTATETEXTURERASTER,
                             (void *) logoRaster);
            RwRenderStateSet(rwRENDERSTATETEXTUREFILTER,
                             (void *) rwFILTERLINEAR);

            RwRenderStateSet(rwRENDERSTATECULLMODE,
                             (void *) rwCULLMODECULLNONE);

            RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS,
                             (void *) rwTEXTUREADDRESSWRAP);

            for (i = 0; i < 4; i++)
            {
                RwIm2DVertexSetScreenZ(&logoVerts[i], nearScreenZ);
                RwIm2DVertexSetIntRGBA(&logoVerts[i], 255, 255, 255,
                                       255);
            }

            RwIm2DVertexSetU(&logoVerts[0], (RwReal) (0.0),
                             (RwReal) (1.0));
            RwIm2DVertexSetV(&logoVerts[0], (RwReal) (0.0),
                             (RwReal) (1.0));

            RwIm2DVertexSetU(&logoVerts[1], (RwReal) (0.0),
                             (RwReal) (1.0));
            RwIm2DVertexSetV(&logoVerts[1], (RwReal) (1.0),
                             (RwReal) (1.0));

            RwIm2DVertexSetU(&logoVerts[2], (RwReal) (1.0),
                             (RwReal) (1.0));
            RwIm2DVertexSetV(&logoVerts[2], (RwReal) (1.0),
                             (RwReal) (1.0));

            RwIm2DVertexSetU(&logoVerts[3], (RwReal) (1.0),
                             (RwReal) (1.0));
            RwIm2DVertexSetV(&logoVerts[3], (RwReal) (0.0),
                             (RwReal) (1.0));

            RwIm2DVertexSetScreenX(&logoVerts[0], (RwReal) (rect->x));
            RwIm2DVertexSetScreenY(&logoVerts[0], (RwReal) (rect->y));
            RwIm2DVertexSetRecipCameraZ(&logoVerts[0], recipZ);

            RwIm2DVertexSetScreenX(&logoVerts[1], (RwReal) (rect->x));
            RwIm2DVertexSetScreenY(&logoVerts[1],
                                   (RwReal) (rect->y + rect->h));
            RwIm2DVertexSetRecipCameraZ(&logoVerts[1], recipZ);

            RwIm2DVertexSetScreenX(&logoVerts[2],
                                   (RwReal) (rect->x + rect->w));
            RwIm2DVertexSetScreenY(&logoVerts[2],
                                   (RwReal) (rect->y + rect->h));
            RwIm2DVertexSetRecipCameraZ(&logoVerts[2], recipZ);

            RwIm2DVertexSetScreenX(&logoVerts[3],
                                   (RwReal) (rect->x + rect->w));
            RwIm2DVertexSetScreenY(&logoVerts[3], (RwReal) (rect->y));
            RwIm2DVertexSetRecipCameraZ(&logoVerts[3], recipZ);

            /* Render it */
            RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, logoVerts, 4);

            RwRenderStateSet(rwRENDERSTATEZTESTENABLE,
                             (void *) zTestEnable);
            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,
                             (void *) zWriteEnable);

            RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,
                             (void *) vAplhaEnable);
            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *) srcBlend);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *) dstBlend);

            RwRenderStateSet(rwRENDERSTATETEXTURERASTER,
                             (void *) txtRaster);
            RwRenderStateSet(rwRENDERSTATETEXTUREFILTER,
                             (void *) txtFilter);

            RwRenderStateSet(rwRENDERSTATECULLMODE, (void *) cullMode);

            RwRenderStateSet(rwRENDERSTATETEXTUREADDRESSU,
                             (void *) addrModeU);
            RwRenderStateSet(rwRENDERSTATETEXTUREADDRESSV,
                             (void *) addrModeV);
        }
    }
    /* Then call the default function */

    result = defaultFunc(camera);

    RWRETURN(result);

}

/**
 * \ingroup rplogo
 * \ref RpLogoGetRenderingRect gets the rectangle on the camera raster
 *  the logo is rendered into.
 *
 * \return pointer to the rect
 * \see RpLogoGetPosition
 * \see RpLogoPluginAttach
 * \see RpLogoSetPosition
 */
RwRect             *
RpLogoGetRenderingRect(void)
{
    RwRect             *rect = (RwRect *) NULL;

    RWAPIFUNCTION(RWSTRING("RpLogoGetRenderingRect"));

    if ((0 <= GlobalOffset) && (0 <= CameraOffset))
    {
        rect = &(RWPLUGINOFFSET(LogoGlobals,
                                RwEngineInstance, GlobalOffset)->rect);
    }

    RWRETURN(rect);
}

/************************************************************************
 *
 *      Function:       LogoCameraCreate()
 *
 *      Description:    Called from RenderWare whenever a camera is created.
 *                      This function is used to initialise the data block
 *                      attached to every RwCamera
 *
 *      Parameters:     The parameters are as specified in the callback
 *                      function definition for RwCameraRegisterPlugin
 *
 *      Return Value:   The return value is specified in the callback
 *                      function definition for RwCameraRegisterPlugin
 *
 ************************************************************************/
static void        *
LogoCameraCreate(void *object, RwInt32 offset,
                 RwInt32 __RWUNUSED__ size)
{
    RwCamera           *camera = (RwCamera *) object;

    RWFUNCTION(RWSTRING("LogoCameraCreate"));
    RWASSERT(0 <= GlobalOffset);
    RWASSERT(0 <= CameraOffset);

    RWASSERT(object);
    RWASSERT(0 <= offset);

    /* Save the current Begin/EndCameraUpdate funcs */

    RWPLUGINOFFSET(LogoCamera, camera, offset)->cameraBeginUpdate =
        camera->beginUpdate;

    RWPLUGINOFFSET(LogoCamera, camera, offset)->cameraEndUpdate =
        camera->endUpdate;

    /* Now overload them */
    camera->beginUpdate = LogoCameraBeginUpdate;
    camera->endUpdate = LogoCameraEndUpdate;

    /*
     * Logo is off by default - turn the logo on yourself if you really want it,
     * Otherwise it interferes with PVS generation, camera subrasters, etc,
     * and becomes truly annoying.
     */
    RWPLUGINOFFSET(LogoCamera, camera, offset)->state = FALSE;

    RWRETURN(object);
}

/************************************************************************
 *
 *      Function:       LogoCameraCopy()
 *
 *      Description:    Called from RenderWare whenever a camera is copied.
 *                      Currently RenderWare does not support cloning of
 *                      cameras so this function is implemented as a dummy.
 *
 *      Parameters:     The parameters are as specified in the callback
 *                      function definition for RwCameraRegisterPlugin
 *
 *      Return Value:   The return value is specified in the callback
 *                      function definition for RwCameraRegisterPlugin
 *
 ************************************************************************/
static void        *
LogoCameraCopy(void *dst __RWUNUSED__,
               const void *src __RWUNUSED__,
               RwInt32 offset __RWUNUSEDRELEASE__,
               RwInt32 size __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("LogoCameraCopy"));
    RWASSERT(0 <= GlobalOffset);
    RWASSERT(0 <= CameraOffset);

    RWASSERT(0 <= offset);

    /* A failure status is returned. Since the functionality does not exist
     * we must do the 'right' thing if RenderWare is ever extended to support
     * camera cloning.
     */

    RWRETURN(NULL);
}

/**
 * \ingroup rplogo
 * \ref RpLogoGetPosition returns the current screen placement of
 *  the logo
 *
 *  \return Position on success, Null otherwise
 *
 * \see RpLogoGetState
 * \see RpLogoPluginAttach
 * \see RpLogoSetPosition
 * \see RpLogoSetState
 ************************************************************************/
RpLogoPosition
RpLogoGetPosition(void)
{
    RpLogoPosition      result = rpNALOGOPOSITION;
    LogoGlobals        *globals;

    RWAPIFUNCTION(RWSTRING("RpLogoGetPosition"));

    if ((0 <= GlobalOffset) && (0 <= CameraOffset))
    {

        globals = RWPLUGINOFFSET(LogoGlobals,
                                 RwEngineInstance, GlobalOffset);
        result = globals->pos;
    }

    RWRETURN(result);
}

/**
 * \ingroup rplogo
 * \ref RpLogoSetPosition determines where on the screen the logo
 * should be placed
 *
 * \param pos  screen location
 *
 * \return True on success, false otherwise
 *
 * \see RpLogoGetPosition
 * \see RpLogoGetState
 * \see RpLogoPluginAttach
 * \see RpLogoSetState
 ************************************************************************/
RwBool
RpLogoSetPosition(RpLogoPosition pos)
{
    RwBool              result;

    RWAPIFUNCTION(RWSTRING("RpLogoSetPosition"));

    result = ((0 <= GlobalOffset) && (0 <= CameraOffset));

    if (result)
    {
        switch (pos)
        {
            case rpLOGOTOP:
            case rpLOGOBOTTOM:
            case rpLOGOLEFT:
            case rpLOGORIGHT:
            case rpLOGOCENTER:
            case rpLOGOTOPLEFT:
            case rpLOGOTOPRIGHT:
            case rpLOGOBOTTOMLEFT:
            case rpLOGOBOTTOMRIGHT:
                {
                    LogoGlobals        *const globals =
                        RWPLUGINOFFSET(LogoGlobals,
                                       RwEngineInstance,
                                       GlobalOffset);

                    globals->pos = pos;
                }

                break;

            default:
                /* Bad parameter type */
                RWERROR((E_RW_BADPARAM,
                         RWSTRING("Invalid logo position specified")));
                result = FALSE;
                break;

        }
    }

    RWRETURN(result);
}

/************************************************************************
 * WST/Working Set Tuner build of ppforest on PC suggests ordering
 * RpLogoGetState
 * RpLogoSetState
 */

/**
 * \ingroup rplogo
 * \ref RpLogoGetState gets the current visibility state of the logo
 *
 * \return the old state
 * \see RpLogoGetPosition
 * \see RpLogoPluginAttach
 * \see RpLogoSetPosition
 * \see RpLogoSetState
 */
RwBool
RpLogoGetState(RwCamera * camera)
{
    RwBool              state;

    RWAPIFUNCTION(RWSTRING("RpLogoGetState"));
    RWASSERTISTYPE(camera, rwCAMERA);

    state = ((0 <= GlobalOffset) && (0 <= CameraOffset));
    if (state)
    {
        state = RWPLUGINOFFSET(LogoCamera, camera, CameraOffset)->state;
    }

    RWRETURN(state);
}

/**
 * \ingroup rplogo
 * \ref RpLogoSetState turns the logo on and off
 *
 * \param camera  determines whether logo in on or off
 * \param state flag TRUE logo on
 *
 * \return the old state
 *
 * \see RpLogoGetPosition
 * \see RpLogoGetState
 * \see RpLogoPluginAttach
 * \see RpLogoSetPosition
 ************************************************************************/
RwBool
RpLogoSetState(RwCamera * camera, RwBool state)
{
    RwBool              oldState;

    RWAPIFUNCTION(RWSTRING("RpLogoSetState"));
    RWASSERTISTYPE(camera, rwCAMERA);

    oldState = ((0 <= GlobalOffset) && (0 <= CameraOffset));

    if (oldState)
    {
        oldState = RpLogoGetState(camera);
        RWPLUGINOFFSET(LogoCamera, camera, CameraOffset)->state = state;
    }

    RWRETURN(oldState);
}

/************************************************************************
 *
 *      Function:       LogoClose()
 *
 *      Description:    Called from RenderWare when global data is terminated
 *
 *      Parameters:     The parameters are as specified in the callback
 *                      function definition for RwEngineRegisterPlugin
 *
 *      Return Value:   The return value is specified in the callback
 *                      function definition for RwEngineRegisterPlugin
 *
 ************************************************************************/
static void        *
LogoClose(void *instance, RwInt32 offset, RwInt32 __RWUNUSED__ size)
{

    LogoGlobals        *globals;
    RwRaster           *logoRaster;

    RWFUNCTION(RWSTRING("LogoClose"));
    RWASSERT(0 <= GlobalOffset);
    RWASSERT(0 <= CameraOffset);

    RWASSERT(instance);
    RWASSERT(0 <= offset);

    globals = RWPLUGINOFFSET(LogoGlobals, RwEngineInstance, offset);
    logoRaster = globals->raster;

    if (logoRaster)
    {
        RwRasterDestroy(logoRaster);
        logoRaster = (RwRaster *) NULL;
        globals->raster = logoRaster;
    }

    RWRETURN(instance);
}

/************************************************************************
 *
 *      Function:       LogoOpen()
 *
 *      Description:    Called from RenderWare when global data is intialised
 *
 *      Parameters:     The parameters are as specified in the callback
 *                      function definition for RwEngineRegisterPlugin
 *
 *      Return Value:   The return value is specified in the callback
 *                      function definition for RwEngineRegisterPlugin
 *
 ************************************************************************/
static void        *
LogoOpen(void *instance,
         RwInt32 offset __RWUNUSEDRELEASE__, RwInt32 size __RWUNUSED__)
{
#ifdef LOGOGEN
    RwImage            *imagePresampled;
#endif
    RwImage            *image;
    RwRaster           *logoRaster;
    RwInt32             rasterWidth, rasterHeight, rasterDepth,
        rasterFlags;
    LogoGlobals        *globals;

    RWFUNCTION(RWSTRING("LogoOpen"));
    RWASSERT(0 <= GlobalOffset);
    RWASSERT(0 <= CameraOffset);

    RWASSERT(instance);
    RWASSERT(0 <= offset);

    globals = RWPLUGINOFFSET(LogoGlobals,
                             RwEngineInstance, GlobalOffset);

#ifdef LOGOGEN

    RwImageRegisterImageFormat("png", RtPNGImageRead, NULL);

    /* use this code to create the header file
     * Note: it will be created in the application directory - not
     * the plugin one. You will need to copy it.
     */

    imagePresampled = RwImageRead(LOGOSRC);
    RwImageFindRasterFormat(imagePresampled, rwRASTERTYPETEXTURE,
                            &rasterWidth, &rasterHeight,
                            &rasterDepth, &rasterFlags);

    LogoImageRatio = (RwReal) RwImageGetWidth(imagePresampled) /
        (RwReal) RwImageGetHeight(imagePresampled);

    image =
        RwImageCreateResample(imagePresampled, rasterWidth,
                              rasterHeight);
    RwImageDestroy(imagePresampled);

    {
        int                 i, j;
        RwUInt8            *iptr = RwImageGetPixels(image);
        FILE               *fp = fopen(LOGOFILE, "w");
        int                 charcount = 0;
        RwChar              buffer[256];

        fp = RwFopen(LOGOFILE, "w");

        rwsprintf(
            buffer,
            "static const RwInt32 LogoRasterWidth = %d;\n",
            rasterWidth);
        RwFputs(buffer, fp);

        rwsprintf(
            buffer,
            "static const RwInt32 LogoRasterHeight = %d;\n",
            rasterHeight);
        RwFputs(buffer, fp);

        LogoRasterHeight = rasterHeight;

        rwsprintf(
            buffer,
            "static const RwReal  LogoImageRatio = %f;\n",
            LogoImageRatio);
        RwFputs(buffer, fp);

        rwsprintf(
            buffer,
            "static RwUInt8 LogoData[] = {\n");
        RwFputs(buffer, fp);

        for (j = 0; j < RwImageGetHeight(image); j++)
        {
            for (i = 0; i < RwImageGetWidth(image) * 4; i += 4)
            {
                rwsprintf(buffer, "0x%x, ", iptr[i + 0]);
                RwFputs(buffer, fp);

                rwsprintf(buffer, "0x%x, ", iptr[i + 1]);
                RwFputs(buffer, fp);

                rwsprintf(buffer, "0x%x, ", iptr[i + 2]);
                RwFputs(buffer, fp);

                rwsprintf(buffer, "0x%x, ", iptr[i + 3]);
                RwFputs(buffer, fp);

                charcount += 24;
                if (charcount > 70)
                {
                    rwsprintf(buffer, "\n");
                    RwFputs(buffer, fp);

                    charcount = 0;
                }
            }
            iptr += RwImageGetStride(image);
        }

        rwsprintf(buffer, "\n};");
        RwFputs(buffer, fp);

        RwFclose(fp);
    }
#else

    image = RwImageCreate(LogoRasterWidth, LogoRasterHeight, 32);
    RwImageSetStride(image, LogoRasterWidth * 4);
    RwImageSetPixels(image, LogoData);

#endif

    RwImageFindRasterFormat(image, rwRASTERTYPETEXTURE,
                            &rasterWidth, &rasterHeight,
                            &rasterDepth, &rasterFlags);

    if ((RwImageGetWidth(image) != rasterWidth) ||
        (RwImageGetHeight(image) != rasterHeight))
    {
        RwImage            *resampledImage;

        /* The image must be 32 bit */
        if (RwImageGetDepth(image) == 8)
        {
            RwImage            *newImage;

            newImage = RwImageCreate(RwImageGetWidth(image),
                                     RwImageGetHeight(image), 32);
            if (newImage)
            {
                if (RwImageAllocatePixels(newImage))
                {
                    RwImageCopy(newImage, image);
                    RwImageDestroy(image);

                    /* Use the 32 bit image */
                    image = newImage;
                }
                else
                {
                    RwImageDestroy(newImage);
                    newImage = (RwImage *) NULL;
                    RwImageDestroy(image);
                    image = (RwImage *) NULL;
                    instance = NULL;
                }
            }
            else
            {
                RwImageDestroy(image);
                image = (RwImage *) NULL;
                instance = NULL;
            }
        }

        if (instance)
        {
            /* The image must be resampled */
            resampledImage =
                RwImageCreate(rasterWidth, rasterHeight, 32);
            if (resampledImage)
            {
                if (RwImageAllocatePixels(resampledImage))
                {

                    RwImageResample(resampledImage, image);
                    RwImageDestroy(image);
                    image = resampledImage;
                }
                else
                {
                    RwImageDestroy(image);
                    image = (RwImage *) NULL;
                    instance = NULL;
                }

            }
            else
            {
                RwImageDestroy(image);
                image = (RwImage *) NULL;
                instance = NULL;
            }
        }
    }

    if (instance)
    {
        /* Gamma correct the image */
        RwImageGammaCorrect(image);

        /* Create a raster */
        logoRaster =
            RwRasterCreate(rasterWidth, rasterHeight, rasterDepth,
                           rasterFlags);
        if (logoRaster)
        {
            /* Convert the image into the raster */
            RwRasterSetFromImage(logoRaster, image);
            RwImageDestroy(image);

            globals->raster = logoRaster;
            globals->pos = rpLOGOBOTTOMLEFT;
        }
        else
        {
            RwImageDestroy(image);
            image = (RwImage *) NULL;
            instance = NULL;
        }
    }

    RWRETURN(instance);
}

/**
 * \ingroup rplogo
 * \ref RpLogoPluginAttach is called by the application to indicate
 * that the Logo plugin should be used. The call
 * to this function should be placed after \ref RwEngineInit
 * and \ref RpWorldPluginAttach
 *
 * \return True on success, false otherwise
 *
 * \see RpLogoGetPosition
 * \see RpLogoGetState
 * \see RpLogoSetPosition
 * \see RpLogoSetState
 ************************************************************************/
RwBool
RpLogoPluginAttach(void)
{
    RwBool              result = FALSE;

    RWAPIFUNCTION(RWSTRING("RpLogoPluginAttach"));

    /* Extend the global data block to include Logo plugin globals */

    GlobalOffset =
        RwEngineRegisterPlugin(sizeof(LogoGlobals),
                               rwID_LOGOPLUGIN, LogoOpen, LogoClose);

    result = (0 <= GlobalOffset);

    if (result)
    {

        /* Hook the RwCamera object
         * - we will be replacing the default EndUpdate
         * functionality.
         */

        CameraOffset =
            RwCameraRegisterPlugin(sizeof(LogoCamera),
                                   rwID_LOGOPLUGIN,
                                   LogoCameraCreate,
                                   LogoCameraDestroy, LogoCameraCopy);

        result = (0 <= CameraOffset);

    }

    RWRETURN(result);
}
