/*
 * 'Tile' renderer for grabbing screen shots
 */

/**
 * \ingroup rttilerender
 * \page rttilerenderoverview RtTile Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rttilerd.h
 * \li \b Libraries: rwcore, rpworld, rttilerd
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach
 *
 * \subsection tilerenderoverview Overview
 * This Toolkit provides a facility for rendering large, high-resolution
 * scenes on systems without the available video and system memory required
 * to handle such resolutions.
 *
 * By allowing a scene to be split into sections -- or ‘tiles’ -- and rendered
 * one row of tiles at a time, the memory overheads can be dramatically reduced
 * when rendering for high-resolution targets such as printed media. (Eg:
 * creating screenshots for magazines.)
 *
 * The developer can specify the number of tiles to split the rendered scene into,
 * as well as supply replacement callback functions for both rendering and
 * serialization.
 */

/***************************************************************************
 *                                                                         *
 * Module  : tktilerd.c                                                    *
 *                                                                         *
 * Purpose : 'Tile' renderer for grabbing screen shots                     *
 *                                                                         *
 **************************************************************************/

/****************************************************************************
 Includes
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "rwcore.h"
#include "rpworld.h"
#include "rpdbgerr.h"
#include "rt2d.h"

/* Include this file */
#include "rttilerd.h"

/****************************************************************************
 Local Types
 */

/* Sun raster header format */
typedef struct RwSunRasterHeader RwSunRasterHeader;
struct RwSunRasterHeader
{
    RwInt32             magic;
    RwInt32             width;
    RwInt32             height;
    RwInt32             bitsPerPixel;
    RwInt32             imageLength;
    RwInt32             type;
    RwInt32             mapType;
    RwInt32             mapLength;
};

#define rwSUNRASTERMAGIC    0x956aa659

/* Global vars */
typedef struct RtTileGlobalVars RtTileGlobalVars;
struct RtTileGlobalVars
{
    RwInt32             tileRenderType;
    RwChar             *imageFilename;
    RwStream           *imageStream;
    RwInt32             imageWidth;
    RwInt32             imageHeight;
    RwInt32             tileWidth;
    RwInt32             tileHeight;
    RwInt32             imageTileX;
    RwInt32             imageTileY;
    RwInt32             imageBufferSize;
    RwInt32             scaleOldWidth;
    RwInt32             scaleOldHeight;
    RwInt32            *imageBuffer;
};

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */
#define RWBYTESWAPLONG(longvar)                 \
    MACRO_START                                 \
    {                                           \
        RwUInt8     caTmp[4];                   \
        RwUInt8     cTmp;                       \
                                                \
        *(RwInt32 *) caTmp = (longvar);         \
        cTmp = caTmp[0];                        \
        caTmp[0] = caTmp[3];                    \
        caTmp[3] = cTmp;                        \
                                                \
        cTmp = caTmp[1];                        \
        caTmp[1] = caTmp[2];                    \
        caTmp[2] = cTmp;                        \
                                                \
        (longvar) = *(RwInt32 *)caTmp;          \
    }                                           \
    MACRO_STOP

#define RTTILEIMAGENONE     0
#define RTTILEIMAGECLAMP    1
#define RTTILEIMAGECROP     2

/* Copied from rppvsgen.c */

#if (defined(SKY))

static RwUInt8     *oldCamPixels;

static void
raslock(RwRaster * ras, void *pixels)
{
    RWFUNCTION(RWSTRING("raslock"));

    if (ras)
    {
        /* If we are on PS2, we must give the Camera a data area */
        /* Ordinarily, PS2 camera rasters have no in memory image */
        /* Under normal conditions it is impossible to lock the camera */
        /* raster on PS2 as its a real performance problem. However, */
        /* by setting the cpPixel pointer the driver will assume that */
        /* you know what you are doing and are willing to take the hit */

        /* We know that malloc aligns on qw */
        oldCamPixels = ras->cpPixels;
        ras->cpPixels = (RwUInt8 *) pixels;

        /* Lock the camera raster for read. This will do what ever is */
        /* required to get the data into memory */

        /* The PS2 driver doesn't do this for you */
        if (!RwRasterLock(ras, 0, rwRASTERLOCKREAD))
        {
            ras->cpPixels = oldCamPixels;
        }
    }

    RWRETURNVOID();
}

static void
rasunlock(RwRaster * ras)
{
    RWFUNCTION(RWSTRING("rasunlock"));

    RwRasterUnlock(ras);
    ras->cpPixels = (RwUInt8 *) oldCamPixels;
    RWRETURNVOID();
}

#define RASLOCK(_ras, _pixels)  raslock(_ras, _pixels)
#define RASUNLOCK(_ras)         rasunlock(_ras)

#endif /* (defined(SKY)) */

#if (!defined(RASLOCK))
#define RASLOCK(_ras, _pixels) /* No op */
#endif /* (!defined(RASLOCK)) */

#if (!defined(RASUNLOCK))
#define RASUNLOCK(_ras)        /* No op */
#endif /* (!defined(RASUNLOCK)) */

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                       Tile rendering function

 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/****************************************************************************
 */

static RtTileGlobalVars *
_tkImageTileSetUpGlobals(RtTileGlobalVars * tkGlobals,
                         RwInt32 iWidth, RwInt32 iHeight,
                         RwInt32 tWidth, RwInt32 tHeight, void *pData)
{
    RtTileGlobalVars   *results;
    RwSunRasterHeader   srHeader;
    RwInt32             i, stride;

    RWFUNCTION(RWSTRING("_tkImageTileSetUpGlobals"));

    results = tkGlobals;

    srHeader.bitsPerPixel = 24;
    srHeader.mapLength = 0;
    srHeader.width = iWidth;
    srHeader.height = iHeight;
    srHeader.type = 1;
    srHeader.mapType = 1;

    stride = (srHeader.width * srHeader.bitsPerPixel + 7) / 8;
    stride = (stride + 1) & -2;

    srHeader.imageLength = stride * iHeight;

    /* Put in correct format */
    {
        RwInt32            *npData = (RwInt32 *) & srHeader;

        for (i = sizeof(srHeader) / sizeof(RwInt32); i; i--)
        {
            RWBYTESWAPLONG((*npData));

            npData++;
        }
    }

    srHeader.magic = rwSUNRASTERMAGIC;

    tkGlobals->imageWidth = iWidth;
    tkGlobals->imageHeight = iHeight;
    tkGlobals->tileWidth = tWidth;
    tkGlobals->tileHeight = tHeight;
    tkGlobals->imageTileX = iWidth / tWidth;
    tkGlobals->imageTileY = iHeight / tHeight;

    tkGlobals->imageFilename = (RwChar *) pData;

    tkGlobals->imageStream =
        RwStreamOpen(rwSTREAMFILENAME, rwSTREAMWRITE,
                     tkGlobals->imageFilename);

    if (tkGlobals->imageStream != NULL)
    {
        RwStreamWrite(tkGlobals->imageStream, &srHeader,
                      sizeof(RwSunRasterHeader));

        tkGlobals->imageBufferSize =
            tkGlobals->imageTileX * tWidth * tHeight * 3 *
            sizeof(RwChar);

        tkGlobals->imageBuffer = (RwInt32 *)
            RwMalloc(tkGlobals->imageBufferSize,
                rwID_TILERENDPLUGIN | rwMEMHINTDUR_EVENT);

        memset((RwChar *) tkGlobals->imageBuffer, 0,
               tkGlobals->imageBufferSize);
    }
    else
    {
        tkGlobals->imageBuffer = (RwInt32 *)NULL;
    }


    RWRETURN(results);
}

/****************************************************************************
 _tkTileRender

 On entry   : Camera
            : Width in multiples of the cameras raster size
            : Height in multiples of the cameras raster size
            : Render function
            : Archive function
            : User data
            : Clamp or crop
 On exit    : The camera if rendered successfully.
            : NULL otherwise.
 */

static RwCamera    *
_tkTileRender(RwCamera * camera,
              RwInt32 iWidth, RwInt32 iHeight,
              RwInt32 tWidth, RwInt32 tHeight,
              RtTileRenderCallBack renderCallBack,
              RtTileArchiveCallBack archiveCallBack, void *pData,
              RwInt32 tileRenderType)
{
    RwCamera           *result, *tileCamera;
    RwImage            *imageTile;
    RwRaster           *camRaster, *camZRaster, *subRaster, *subZRaster;
    RwCameraProjection  rwProjection;
    RwFrame            *newFrame, *oldFrame;
    RwMatrix           *oldLTM, *newMatrix;
    RwV2d               newOffset, oldOffset, oldWindow, newWindow;
    RwV2d               tmpWindow;
    RwRect              tileRect;
    RwInt32             nX, nY, tileScaleX, tileScaleY;
    RwReal              imageScaleX, imageScaleY;
    RwReal              invTileScaleX, invTileScaleY;
    RwReal              imageXProgress, imageYProgress;
    RwReal              oldRatio, newRatio;
    const RwV2d        *oldView;
    RwInt32             rasWidth, rasHeight;

    RtTileGlobalVars    tkGlobals;
    void               *usePData;

    RWFUNCTION(RWSTRING("_tkTileRender"));

    /* Make sure the params are good */

    if ((iWidth < 1) || (iHeight < 1) || (tWidth < 1) || (tHeight < 1)
        || (iWidth < tWidth) || (iHeight < tHeight) || (camera == NULL))
        RWRETURN((RwCamera *)NULL);

    if (((iWidth % tWidth) > 0) || ((iHeight % tHeight) > 0))
        RWRETURN((RwCamera *)NULL);

    /* Init some local vars */
    result = camera;

    imageTile = (RwImage *)NULL;
    subRaster = (RwRaster *)NULL;
    subZRaster = (RwRaster *)NULL;
    newFrame = (RwFrame *)NULL;
    newMatrix = (RwMatrix *)NULL;

    tkGlobals.tileRenderType = tileRenderType;

    tkGlobals.imageStream = (RwStream *)NULL;
    tkGlobals.imageBuffer = (RwInt32 *)NULL;

    /* Get info on the camera. */
    tileCamera = RwCameraCreate();

    rwProjection = RwCameraGetProjection(camera);

    camRaster = RwCameraGetRaster(camera);
    camZRaster = RwCameraGetZRaster(camera);

    oldFrame = RwCameraGetFrame(camera);
    oldLTM = RwFrameGetLTM(oldFrame);

    oldView = RwCameraGetViewOffset(camera);
    oldOffset.x = oldView->x;
    oldOffset.y = oldView->y;

    oldView = RwCameraGetViewWindow(camera);
    oldWindow.x = oldView->x;
    oldWindow.y = oldView->y;

    /* Set up the raster */
    if (result != NULL)
    {
        /* Change the this camera's begin and end update. */
        tileCamera->beginUpdate = camera->beginUpdate;
        tileCamera->endUpdate = camera->endUpdate;

        RwCameraSetNearClipPlane(tileCamera,
                                 RwCameraGetNearClipPlane(camera));
        RwCameraSetFarClipPlane(tileCamera,
                                RwCameraGetFarClipPlane(camera));

        RwCameraSetProjection(tileCamera, rwProjection);

        rasWidth = RwRasterGetWidth(camRaster);
        rasHeight = RwRasterGetHeight(camRaster);

        imageScaleX = (RwReal) iWidth / (RwReal) rasWidth;
        imageScaleY = (RwReal) iHeight / (RwReal) rasHeight;

        /* Check if the raster is larger than the tile size */
        if ((rasWidth >= tWidth) && (rasHeight >= tHeight))
        {
            tileRect.x = 0;
            tileRect.y = 0;
            tileRect.w = tWidth;
            tileRect.h = tHeight;

            subRaster = RwRasterCreate(0, 0, 0, rwRASTERTYPECAMERA);

            if (subRaster != NULL)
            {
                RwRasterSubRaster(subRaster, camRaster, &tileRect);
                RwCameraSetRaster(tileCamera, subRaster);
            }
            else
                result = (RwCamera *)NULL;

            subZRaster = RwRasterCreate(0, 0, 0, rwRASTERTYPEZBUFFER);

            if (subRaster != NULL)
            {
                RwRasterSubRaster(subZRaster, camZRaster, &tileRect);
                RwCameraSetZRaster(tileCamera, subZRaster);
            }
            else
                result = (RwCamera *)NULL;
        }
        else
            result = (RwCamera *)NULL;
    }

    /* Set up the image tile */
    if (result != NULL)
    {
        imageTile = RwImageCreate(tWidth, tHeight, 32);

        if (imageTile == (RwImage *)NULL)
            result = (RwCamera *)NULL;
        else
        {
            if (!RwImageAllocatePixels(imageTile))
                result = (RwCamera *)NULL;
        }
    }

    /* Set up the new frame */
    if (result != NULL)
    {
        newFrame = RwFrameCreate();

        if (newFrame != NULL)
        {
            RwMatrixCopy(RwFrameGetMatrix(newFrame), oldLTM);
            newMatrix = RwFrameGetMatrix(newFrame);
            RwCameraSetFrame(tileCamera, newFrame);
        }
        else
            result = (RwCamera *)NULL;
    }

    /* Main rendering loop */
    if (result != NULL)
    {
        tileScaleX = iWidth / tWidth;
        tileScaleY = iHeight / tHeight;

        invTileScaleX = (RwReal) 1.0 / (RwReal) tileScaleX;
        invTileScaleY = (RwReal) 1.0 / (RwReal) tileScaleY;

        /* Clamp or crop the image. */
        if (tileRenderType == RTTILEIMAGECROP)
        {
            tmpWindow.x = (iWidth >= iHeight) ?
                (RwReal) 1.0 : (RwReal) iWidth / (RwReal) iHeight;
            tmpWindow.y = (iWidth <= iHeight) ?
                (RwReal) 1.0 : (RwReal) iHeight / (RwReal) iWidth;

            newWindow.x = tmpWindow.x * invTileScaleX;
            newWindow.y = tmpWindow.y * invTileScaleY;
        }
        else
        {
            if (iWidth > iHeight)
            {
                newWindow.y = oldWindow.y / tileScaleY;
                newWindow.x = newWindow.y *
                    (RwReal) tWidth / (RwReal) tHeight;
            }
            else
            {
                newWindow.x = oldWindow.x / tileScaleX;
                newWindow.y = newWindow.x *
                    (RwReal) tHeight / (RwReal) tWidth;
            }

        }

        oldRatio = (RwReal) oldWindow.y / (RwReal) oldWindow.x;
        newRatio = (RwReal) iHeight / (RwReal) iWidth;

        if (oldRatio > newRatio)
        {
            tkGlobals.scaleOldHeight = iHeight;
            tkGlobals.scaleOldWidth =
                RwInt32FromRealMacro(((RwReal) iWidth *
                                      (newRatio / oldRatio)));
        }
        else
        {
            tkGlobals.scaleOldWidth = iWidth;
            tkGlobals.scaleOldHeight =
                RwInt32FromRealMacro(((RwReal) iHeight *
                                      (oldRatio / newRatio)));
        }

        RwCameraSetViewWindow(tileCamera, &newWindow);

        /* Real world displacement */

        tmpWindow.x = newWindow.x;
        tmpWindow.y = newWindow.y;

        newWindow.x *= (RwReal) 2.0;
        newWindow.y *= (RwReal) 2.0;

        /* Check if we using the default call back */
        if ((archiveCallBack == RtTileDefaultArchive) && (pData))
        {
            _tkImageTileSetUpGlobals(&tkGlobals,
                                     iWidth, iHeight, tWidth, tHeight,
                                     pData);

            usePData = (void *) &tkGlobals;
        }
        else
        {
            usePData = pData;
        }

        imageYProgress = oldWindow.y;

        /* Initial offset value */
        newOffset.y = -(oldWindow.y - (newWindow.y * (RwReal) 0.5));

        for (nY = 0; nY < tileScaleY; nY++)
        {
            tmpWindow.x = newWindow.x * (RwReal) 0.5;
            imageXProgress = oldWindow.x;

            /* Initial offset value */
            newOffset.x = oldWindow.x - (newWindow.x * (RwReal) 0.5);

            for (nX = 0; nX < tileScaleX; nX++)
            {
                RwV2d               tmpV2d;
                RwV3d               tmpV3d;

                if ((tmpWindow.x > (RwReal) 0.0) &&
                    (tmpWindow.y > (RwReal) 0.0))
                {
                    /* Work out the cameras new position */

                    RwV3dScale(&tmpV3d, &oldLTM->right, newOffset.x);
                    RwV3dAdd(&newMatrix->pos, &tmpV3d, &oldLTM->pos);

                    RwV3dScale(&tmpV3d, &oldLTM->up, -newOffset.y);
                    RwV3dAdd(&newMatrix->pos, &tmpV3d, &newMatrix->pos);

                    /* RwMatrixUpdate(newMatrix); */
                    RwMatrixOptimize(newMatrix, (RwMatrixTolerance *) NULL);

                    /* Apply offset if perspective. */

                    if (rwProjection == rwPERSPECTIVE)
                    {
                        tmpV2d.x = newOffset.x + oldOffset.x;
                        tmpV2d.y = newOffset.y + oldOffset.y;

                        RwCameraSetViewOffset(tileCamera, &tmpV2d);
                    }

                    /* Update the frame */

                    RwFrameUpdateObjects(newFrame);

                    /* Setup 2d rendering */

                    /* Render the camera */
                    if (renderCallBack(tileCamera, nX, nY, pData) !=
                        tileCamera)
                        result = (RwCamera *)NULL;

                }
                else
                    nX = tileScaleX;

                if (result != NULL)
                {
                    /* Copy the raster to an image and pass it to the
                     * archive callback.
                     */
                    RASLOCK(subRaster, RwImageGetPixels(imageTile));

#ifndef SKY
                    RwImageSetFromRaster(imageTile, subRaster);
#endif

                    RASUNLOCK(subRaster);

                    if (archiveCallBack(imageTile, nX, nY, usePData)
                        == (RwImage *)NULL)
                        result = (RwCamera *) NULL;
                }

                /* Check for errors. */
                if (result == NULL)
                    break;

                /* Update progress. */
                imageXProgress -= (newWindow.x * (RwReal) 0.5);

                /* Clamp the image if we are clamping. */
                if (tileRenderType == RTTILEIMAGECLAMP)
                {
                    if (imageXProgress > (newWindow.x * (RwReal) 0.5))
                        tmpWindow.x = (newWindow.x * (RwReal) 0.5);
                    else
                        tmpWindow.x = imageXProgress;
                }

                /* Update the view offset for next pass */
                newOffset.x -= newWindow.x;
            }

            /* Check for errors. */
            if (result == NULL)
                break;

            /* Update progress. */
            imageYProgress -= (newWindow.y * (RwReal) 0.5);

            /* Clamp the image if we are clamping. */
            if (tileRenderType == RTTILEIMAGECLAMP)
            {
                if (imageYProgress > (newWindow.y * (RwReal) 0.5))
                    tmpWindow.y = (newWindow.y * (RwReal) 0.5);
                else
                    tmpWindow.y = imageYProgress;
            }

            /* Update the view offset for next pass */
            newOffset.y += newWindow.y;
        }
    }

    /* Restore state */

    /* Clean up. */
    if (newFrame != NULL)
        RwFrameDestroy(newFrame);
    if (imageTile != NULL)
        RwImageDestroy(imageTile);
    if (subRaster != NULL)
        RwRasterDestroy(subRaster);
    if (subZRaster != NULL)
        RwRasterDestroy(subZRaster);

    if (tileCamera != NULL)
        RwCameraDestroy(tileCamera);

    if (tkGlobals.imageStream != NULL)
        RwStreamClose(tkGlobals.imageStream, NULL);
    if (tkGlobals.imageBuffer != NULL)
        RwFree(tkGlobals.imageBuffer);

    /* All done */
    RWRETURN(result);
}

/**
 * \ingroup rttilerender
 * \ref RtTileRender
 * is used to perform tile rendering using the specified
 * camera. The resulting image's dimensions are arbitrary but must be an
 * exact muliple of the tile's size. Furthermore, the tile's size must
 * not exceed the camera raster's dimensions.
 *
 * This function requires the specification of two callback functions:
 * the render callback specifies the function that will do the rendering
 * for each tile; the archive callback specifies the function that will
 * save the results of each rendered tile. Both callbacks must be set.
 * There is a default archive callback provided, \ref RtTileDefaultArchive,
 * for saving images. A default render callback is not provided because it
 * is too application specific.
 *
 * The render callback should return a pointer to the camera to indicate
 * success; return NULL to indicate an error has occurred - this will
 * cause the tile rendering to be aborted. Similarly, the archive
 * callback should return a pointer to the tile image if successful or
 * NULL if there is an error which, again, will cause the tile rendering
 * to be prematurely terminated.
 *
 * An example of render callback would be
 *
 * \verbatim
    RwCamera *
    RtTileDefaultRender(RwCamera * camera,
                        RwInt32 x,
                        RwInt32  y,
                        void * pData)
    {
        RwRGBA bkcol;

        bkcol.red = 0;
        bkcol.green = 128;
        bkcol.blue = 255;
        bkcol.alpha = 255;

        RwCameraClear(camera, &bkcol,
                    (rwCAMERACLEARIMAGE | rwCAMERACLEARZ));

        if (RwCameraBeginUpdate(camera))
        {
            RpWorldRender(RwCameraGetWorld(camera));

            RwCameraEndUpdate(camera);
        }

        return camera;
    }
 \endverbatim
 *
 * The user-data pointer can be used to pass data to the callbacks. If
 * the default archive callback is used, a pointer to a string containing
 * the name of the output image filename (with .ras extension) should be
 * specified.
 *
 * The include file rttilerd.h and the library file rttilerd.lib are
 * required to use this function. The library file rpworld.lib is also
 * required.
 *
 * \verbatim
   The format of the callbacks is:
   RwCamera *(*RtTileRenderCallBack)(RwCamera *camera,
        RwInt32 x, RwInt32 y, void *data);
   RwImage *(*RtTileArchiveCallBack)(RwImage *image,
        RwInt32 x, RwInt32 y, void *data);

   where (x, y) are the horizontal and vertical indices of the current
   tile, ranging from 0 to imageWidth/tileWidth -1, and from 0 to
   imageHeight/tileHeight - 1, respectively, with (0,0) at top-left, and
   data is a user-data pointer to pass to the callbacks.
   \endverbatim
 *
 *
 * \param camera  Pointer to the camera.
 * \param iWidth  A RwInt32 value equal to the pixel-width of the output
 * image.
 * \param iHeight  A RwInt32 value equal to the pixel-height of the output
 * image.
 * \param tWidth  A RwInt32 value equal to the pixel-width of the tiles.
 * \param tHeight  A RwInt32 value equal to the pixel-height of the tiles.
 * \param renderCallBack  Pointer to the render callback function.
 * \param archiveCallBack  Pointer to the archive callback function.
 * \param pData  User-data pointer.
 * \return a pointer to the camera if successful or NULL if there is an
 * error.
 * \see RtTileDefaultArchive
 */
RwCamera           *
RtTileRender(RwCamera * camera,
             RwInt32 iWidth, RwInt32 iHeight,
             RwInt32 tWidth, RwInt32 tHeight,
             RtTileRenderCallBack renderCallBack,
             RtTileArchiveCallBack archiveCallBack, void *pData)
{
    RwCamera           *result;

    RWAPIFUNCTION(RWSTRING("RtTileRender"));

    RWASSERT(renderCallBack);
    RWASSERT(archiveCallBack);

    result = _tkTileRender(camera,
                           iWidth, iHeight, tWidth, tHeight,
                           renderCallBack, archiveCallBack, pData,
                           RTTILEIMAGECLAMP);

    RWRETURN(result);
}


/**
 * \ingroup rttilerender
 * \ref RtTileDefaultArchive
 * is the default archiving function when used in
 * conjunction with \ref RtTileRender.
 *
 * The default archiver accumulates the renderings for a complete line of
 * tiles (spanning the output image width) before writing the results to
 * the output file. This enables an application to produce very high
 * resolution images without requiring very high amounts of memory.
 *
 * The include file rttilerd.h and the library file rttilerd.lib are
 * required to use this function. The library file rpworld.lib is also
 * required.
 *
 * \param  image   Pointer to the image.
 * \param  x   A RwInt32 value equal to the horizontal index of the tile
 * (left-most tiles have index zero).
 * \param  y   A RwInt32 value equal to the vertical index of the tile
 * (top-most tiles have index zero).
 * \param  pData   User-data pointer to pass to archive callback.
 * \return a pointer to the tile's image if successful or NULL if there
 * is an error.
 * \see RtTileRender
 */
RwImage            *
RtTileDefaultArchive(RwImage * image,
                     RwInt32 x, RwInt32 __RWUNUSED__ y, void *pData)
{
    RWAPIFUNCTION(RWSTRING("RtTileDefaultArchive"));

    if (pData)
    {
        RwInt32             i, j, iX, iY, inStride, outStride;
        RwChar             *in, *out, *outPix;
        RwRGBA             *inPix;

        RtTileGlobalVars   *tkGlobals;

        tkGlobals = (RtTileGlobalVars *) pData;

        /* Check if we are within the tile ranges. */
        if (x < tkGlobals->imageTileX)
        {
            iY = y * tkGlobals->tileHeight;

            in = (RwChar *) RwImageGetPixels(image);
            inStride = RwImageGetStride(image);

            out = (RwChar *) tkGlobals->imageBuffer;
            out += x * tkGlobals->tileWidth * 3 * sizeof(RwChar);

            outStride = tkGlobals->imageWidth * 3 * sizeof(RwChar);

            for (j = 0; j < tkGlobals->tileHeight; j++, iY++)
            {
                iX = x * tkGlobals->tileWidth;

                inPix = (RwRGBA *) in;
                outPix = out;

                for (i = 0; i < tkGlobals->tileWidth; i++, iX++)
                {
                    *outPix++ = inPix->blue;
                    *outPix++ = inPix->green;
                    *outPix++ = inPix->red;

                    inPix++;

                    if (tkGlobals->tileRenderType == RTTILEIMAGECLAMP)
                        if (iX > tkGlobals->scaleOldWidth)
                            break;
                }

                out += outStride;
                in += inStride;

                if (tkGlobals->tileRenderType == RTTILEIMAGECLAMP)
                    if (iY > tkGlobals->scaleOldHeight)
                        break;
            }
        }

        /* Have we come to the last tile of the tileline ? */
        if (x >= tkGlobals->imageTileX - 1)
        {
            RwStreamWrite(tkGlobals->imageStream,
                          tkGlobals->imageBuffer,
                          tkGlobals->imageBufferSize);

            memset((RwChar *) tkGlobals->imageBuffer, 0,
                   tkGlobals->imageBufferSize);
        }
    }

    RWRETURN(image);
}
