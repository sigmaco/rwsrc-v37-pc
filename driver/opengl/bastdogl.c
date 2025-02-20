/***************************************************************************
 *                                                                         *
 * Module  : bastdogl.c                                                    *
 *                                                                         *
 * Purpose : Standard function handling                                    *
 *                                                                         *
 **************************************************************************/

/****************************************************************************
 Includes
 */

#include "barwtyp.h"
#include "baimage.h"
#include "batextur.h"

/* This file's header */
#include "bastdogl.h"


/****************************************************************************
 Local (static) Globals
 */

/* Standard functions */
static RwStandard StandardFuncs[] =
{
    /* Camera ops */
    {rwSTANDARDCAMERABEGINUPDATE,     (RwStandardFunc)_rwOpenGLCameraBeginUpdate},
    {rwSTANDARDCAMERAENDUPDATE,       (RwStandardFunc)_rwOpenGLCameraEndUpdate},
    {rwSTANDARDCAMERACLEAR,           (RwStandardFunc)_rwOpenGLCameraClear},

    /* Raster/Pixel operations */
    {rwSTANDARDRASTERSHOWRASTER,      (RwStandardFunc)_rwOpenGLRasterShowRaster},
    {rwSTANDARDRGBTOPIXEL,            (RwStandardFunc)_rwOpenGLRGBToPixel},
    {rwSTANDARDPIXELTORGB,            (RwStandardFunc)_rwOpenGLPixelToRGB},
    {rwSTANDARDRASTERSETIMAGE,        (RwStandardFunc)_rwOpenGLRasterSetImage},
    {rwSTANDARDIMAGEGETRASTER,        (RwStandardFunc)_rwOpenGLImageSetRaster},

    /* Raster creation and destruction */
    {rwSTANDARDRASTERDESTROY,         (RwStandardFunc)_rwOpenGLRasterDestroy},
    {rwSTANDARDRASTERCREATE,          (RwStandardFunc)_rwOpenGLRasterCreate},

    /* Finding about a raster type */
    {rwSTANDARDIMAGEFINDRASTERFORMAT, (RwStandardFunc)_rwOpenGLImageFindRasterFormat},

    /* Texture operations */
    {rwSTANDARDTEXTURESETRASTER,      (RwStandardFunc)_rwOpenGLTextureSetRaster},

    /* Locking and releasing */
    {rwSTANDARDRASTERLOCK,            (RwStandardFunc)_rwOpenGLRasterLock},
    {rwSTANDARDRASTERUNLOCK,          (RwStandardFunc)_rwOpenGLRasterUnlock},
    {rwSTANDARDRASTERLOCKPALETTE,     (RwStandardFunc)_rwOpenGLRasterLockPalette},
    {rwSTANDARDRASTERUNLOCKPALETTE,   (RwStandardFunc)_rwOpenGLRasterUnlockPalette},

    /* Raster operations */
    {rwSTANDARDRASTERCLEAR,           (RwStandardFunc)_rwOpenGLRasterClear},
    {rwSTANDARDRASTERCLEARRECT,       (RwStandardFunc)_rwOpenGLRasterClearRect}, 
    {rwSTANDARDRASTERRENDER,          (RwStandardFunc)_rwOpenGLRasterRender},
    {rwSTANDARDRASTERRENDERSCALED,    (RwStandardFunc)_rwOpenGLRasterRenderScaled},
    {rwSTANDARDRASTERRENDERFAST,      (RwStandardFunc)_rwOpenGLRasterRenderFast},

    /* Setting the context */
    {rwSTANDARDSETRASTERCONTEXT,      (RwStandardFunc)_rwOpenGLSetRasterContext},

    /* Creating sub rasters */
    {rwSTANDARDRASTERSUBRASTER,       (RwStandardFunc)_rwOpenGLRasterSubRaster},

    /* Render order hint */
    {rwSTANDARDHINTRENDERF2B,         (RwStandardFunc)_rwOpenGLHintRenderFront2Back},

    /* Native texture serialisation */
    {rwSTANDARDNATIVETEXTUREGETSIZE,  (RwStandardFunc)_rwOpenGLNativeTextureGetSize },
    {rwSTANDARDNATIVETEXTUREWRITE,    (RwStandardFunc)_rwOpenGLNativeTextureWrite },
    {rwSTANDARDNATIVETEXTUREREAD,     (RwStandardFunc)_rwOpenGLNativeTextureRead },

    /* Raster mip levels */
    {rwSTANDARDRASTERGETMIPLEVELS,    (RwStandardFunc)_rwOpenGLRasterGetMipLevels}
};


/****************************************************************************
 OpenGLNullStandard

 On entry   : pOut
            : pInOut
            : nIn
 On exit    : FALSE
 */

static RwBool
OpenGLNullStandard(void *out,void *inOut,RwInt32 in)
{
    RWFUNCTION(RWSTRING("OpenGLNullStandard"));

    /* Stop warnings */
    out   = out;
    inOut = inOut;
    in    = in;

    RWRETURN(FALSE);
}


/****************************************************************************
 _rwOpenGLSetStandards

 On entry   : Standard functions
            : Space
            : Standards
            : Amount of standards
 On exit    :
 */
void
_rwOpenGLSetStandards(RwStandardFunc *funcs, RwInt32 numFuncs)
{
    RwInt32 i;
    RwStandard *standards = StandardFuncs;
    RwInt32 numStandards  = sizeof(StandardFuncs) / sizeof(RwStandard);

    RWFUNCTION(RWSTRING("_rwOpenGLSetStandards"));

    /* Clear out all of the standards initially */
    for (i = 0; i < numFuncs; i++)
    {
        funcs[i] = OpenGLNullStandard;
    }

    /* Fill in all of the standards */
    while (numStandards--)
    {
        if ((standards->nStandard < numFuncs) &&
            (standards->nStandard >= 0))
        {
            funcs[standards->nStandard] = standards->fpStandard;
        }

        standards++;
    }

    RWRETURNVOID();
}


