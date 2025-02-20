/***************************************************************************
 *                                                                         *
 * Module  : baunxogl.c                                                    *
 *                                                                         *
 * Purpose : OpenGL device layer                                           *
 *                                                                         *
 **************************************************************************/

/****************************************************************************
 Includes
 */

/* OpenGL libraries */
#include <GL/glx.h>             /* this includes the necessary X headers */

#include <X11/Xatom.h>          /* for XA_RGB_DEFUALT_MAP atom */
#include <X11/Xmu/StdCmap.h>
#include <X11/extensions/xf86vmode.h>

#include <GL/gl.h>

/* RW type definitions */
#include "badevice.h"

/* Common driver includes */
#include "barwtyp.h"

/* Modules needed */
#include "baintogl.h"
#include "bastdogl.h"
#include "bautilgl.h"

/* This files header */
#include "baogl.h"


/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Globals (across program)
 */
RwOpenGLExtensions  _rwOpenGLExt;


/****************************************************************************
 Local (static) Globals
 */

static RwXWindowInfo Xinfo;
static GLXContext    Ctx;

static RwInt32 NumVideoModes    = -1;
static RwInt32 CurrentVideoMode = 0;
static RwVideoMode *VideoModes  = NULL;
static XF86VidModeModeInfo **XF86Modes = NULL;



/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                            System handler

 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */


RwBool
_rwOpenGLSelectStencilBufferDepth(RwUInt8 depth __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("_rwOpenGLSelectStencilBufferDepth"));

    RWRETURN(TRUE);
}

RwBool
_rwOpenGLGetStencilBufferDepth(RwUInt8 *depth)
{
    RWFUNCTION(RWSTRING("_rwOpenGLGetStencilBufferDepth"));
    RWASSERT(depth);

    if (RWSRCGLOBAL(engineStatus) == rwENGINESTATUSSTARTED)
    {
        *depth = 0;
        RWRETURN(TRUE);
    }
    RWRETURN(FALSE);
}


static RwBool
OpenGLEnumerateVideoModes(void)
{
    Display *display = (Display*)Xinfo.display;
    int majorVersion, minorVersion, numModes, i;

    RWFUNCTION(RWSTRING("OpenGLEnumerateVideoModes"));

    XF86VidModeQueryVersion(display, &majorVersion, &minorVersion);
    XF86VidModeGetAllModeLines(display, DefaultScreen(display), &numModes, &XF86Modes);

    NumVideoModes = numModes + 1;
    VideoModes = (RwVideoMode*)RwDriverMalloc(sizeof(RwVideoMode) * NumVideoModes,
                                          rwID_DRIVERMODULE | rwMEMHINTDUR_GLOBAL);

    VideoModes[0].height = XF86Modes[0]->vdisplay;
    VideoModes[0].width  = XF86Modes[0]->hdisplay;
    VideoModes[0].depth  = 16;
    VideoModes[0].flags  = 0;

    for (i = 1; i <= numModes; i++)
    {
        VideoModes[i].height = XF86Modes[i-1]->vdisplay;
        VideoModes[i].width  = XF86Modes[i-1]->hdisplay;
        VideoModes[i].depth  = 16;
        VideoModes[i].flags  = rwVIDEOMODEEXCLUSIVE;
    }

    RWRETURN(TRUE);
}


/****************************************************************************
 _rwDeviceRegisterPlugin

 On entry   :
 On exit    : TRUE on success
 */
RwBool
_rwDeviceRegisterPlugin(void)
{
    RWFUNCTION(RWSTRING("_rwDeviceRegisterPlugin"));

    /*
     * Gives the device a change to register it's plugins.
     */

    /* Reserve some memory inside raster structures
     * for storing data that the OpenGL library needs
     * to make a new texture current in the render state
     */
    RasterExtOffset = RwRasterRegisterPlugin(sizeof(_rwGlRasterExt),
                                              rwID_DEVICEMODULE,
                                              NULL, NULL, NULL);

    if (0 > RasterExtOffset)
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}


/****************************************************************************
 _rwOpenGLSystem

 On entry   : Request
            : Data out
            : Data in/out
            : Number in
 On exit    : TRUE on success
 */

RwBool
_rwOpenGLSystem(RwInt32 nRequest, void *pOut,void *pInOut,RwInt32 nIn)
{
    RWFUNCTION(RWSTRING("_rwOpenGLSystem"));

    switch (nRequest)
    {
        /*********************** Dos Controls ***************************/

        case rwDEVICESYSTEMUSEMODE:
        {
            if ((nIn >= 0) && (nIn < NumVideoModes))
            {
                CurrentVideoMode = nIn;
                RWRETURN(TRUE);
            }
            RWRETURN(FALSE);
        }

        case rwDEVICESYSTEMGETNUMMODES:
        {
            RwInt32 *npOut = (RwInt32 *)pOut;

            (*npOut) = NumVideoModes;

            RWRETURN(TRUE);
        }

        case rwDEVICESYSTEMGETMODEINFO:
        {
            RwVideoMode *mode = (RwVideoMode *)pOut;

            *mode  = VideoModes[nIn];

            RWRETURN(TRUE);
        }

        case rwDEVICESYSTEMGETMODE:
        {
            RwInt32     *npOut = (RwInt32 *)pOut;

            if (CurrentVideoMode >= 0)
            {
                (*npOut) = CurrentVideoMode;
                RWRETURN(TRUE);
            }

            /* It's a bad time */
            RWRETURN(FALSE);
        }

        case rwDEVICESYSTEMFOCUS:
        {
            RWRETURN(TRUE);
        }

        /******************* Opening/Closing **********************/

        /* Gives the device access to the libraries device block */

        case rwDEVICESYSTEMREGISTER:
        {
            RwDevice *device = (RwDevice *)pOut;
            RwMemoryFunctions *memFuncs = (RwMemoryFunctions *)pInOut;

            /* Set up the fast device access.. */
            (*device) = (*_rwDeviceGetHandle());

            _badpGOpenGLDevice  = device;
            dgGGlobals.memFuncs = memFuncs;

            RWRETURN(TRUE);
        }

        /******************* Opening/Closing **********************/

        /* The purpose of Open is to put the hardware in a state where
         it can be queried (about its capabilities). This function also
         tests if it is a candidate at all (like it can be started at
         all). NOTE this includes setting up a minimal set of standard
         handlers that can be used for querying the device. */

        case rwDEVICESYSTEMOPEN:
        {
            Xinfo = *(RwXWindowInfo*)
                        (((RwEngineOpenParams*)pInOut)->displayID);

            if (!OpenGLEnumerateVideoModes())
            {
                RWRETURN(FALSE);
            }

            /* Open the OpenGL module */
            if (!_rwOpenGLOpen())
            {
                RWRETURN(FALSE);
            }

            RWRETURN(TRUE);
        }

        /* The purpose of Close is to remove any set up that was performed
         by Open */

        case rwDEVICESYSTEMCLOSE:
        {
            /* Shut down OpenGL */
            _rwOpenGLClose();

            /* No mode selected */
            CurrentVideoMode = -1;

            RwDriverFree(VideoModes);

            RWRETURN(TRUE);
        }

        /************** Starting stopping ******************/

        /* Start means that this bit of hardware has been selected for
         rendering - so get it into a state ready for rendering */

        case rwDEVICESYSTEMSTART:
        {
            Display *display = (Display*)Xinfo.display;

            if (VideoModes[CurrentVideoMode].flags & rwVIDEOMODEEXCLUSIVE) /* full screen */
            {
                XSetWindowAttributes attr;
                XWMHints wmhints;

                XF86VidModeSwitchToMode(display,
                                        DefaultScreen(display),
                                        XF86Modes[CurrentVideoMode-1]);
                XF86VidModeSetViewPort(display,
                                       DefaultScreen(display),
                                       0, 0);

                /* Now we fiddle with the user supplied window to resize it to the screen */
                attr.override_redirect = True;
                XChangeWindowAttributes(display,
                                        (Window)Xinfo.window,
                                        CWOverrideRedirect,
                                        &attr);

                XWarpPointer(display, None, (Window)Xinfo.window, 0, 0, 0, 0, 0, 0);
                XMoveResizeWindow(display,
                                  (Window)Xinfo.window,
                                  0, 0,
                                  VideoModes[CurrentVideoMode].width,
                                  VideoModes[CurrentVideoMode].height);
            }

            /* Create a rendering context */
            Ctx = glXCreateContext(display,
                                   (XVisualInfo*)Xinfo.visualInfo,
                                   NULL,     /* No sharing of display lists  */
                                   GL_TRUE); /* direct rendering if possible */

            if (Ctx == NULL)
            {
                fprintf(stderr, "Unable to make create render context\n");
                RWRETURN(FALSE);
            }

            /* Bind the rendering context to the window */
            if (!glXMakeCurrent(display, (Window)Xinfo.window, Ctx))
            {
                fprintf(stderr, "Unable to make render context current\n");
                glXDestroyContext(display, Ctx);
                Ctx = NULL;
                RWRETURN(FALSE);
            }

            /* Set up the rendering state */
            _rwOpenGLSetStartRenderingState();

            /* All done */
            RWRETURN(TRUE);
        }

        /* Puts back the hardware into the state it was in immediately
           after being opened */

        case rwDEVICESYSTEMSTOP:
        {
            Display *display = (Display*)Xinfo.display;

            /* Shut down OpenGL */
            if (Ctx)
            {
                glXDestroyContext(display, Ctx);
                Ctx = NULL;
            }

            if (VideoModes[CurrentVideoMode].flags & rwVIDEOMODEEXCLUSIVE) /* full screen */
            {
                XF86VidModeSwitchToMode(display, DefaultScreen(display), XF86Modes[0]);
                XF86VidModeSetViewPort(display, DefaultScreen(display), 0, 0);
            }
            XFree(XF86Modes);
            XF86Modes = NULL;

            /* All done */
            RWRETURN(TRUE);
        }

        /************* pipeline initialisation **************************/
        case rwDEVICESYSTEMINITPIPELINE:
        {
            RWRETURN(TRUE);

        }

        /************* standard device functions ************************/

        case rwDEVICESYSTEMSTANDARDS:
        {
            _rwOpenGLSetStandards((RwStandardFunc *) pOut, nIn);

            RWRETURN(TRUE);
        }

        case rwDEVICESYSTEMGETID:
        {
            *(RwUInt16*)pOut = rwDEVICE_OPENGL;

            RWRETURN(TRUE);
        }

        default:
        {
            break;
        }
    }

    /* Ah ha, something that's not supported in the device */

    RWRETURN(FALSE);
}



/****************************************************************************
 _rwOpenGLRasterShowRaster

 On entry   : Camera
            : Device specific parameter (HWND)
 On exit    : TRUE on success
 */

RwBool
_rwOpenGLRasterShowRaster(void *pInOut __RWUNUSED__, void *pIn __RWUNUSED__, RwInt32 nIn __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("_rwOpenGLRasterShowRaster"));

    glXSwapBuffers((Display*)Xinfo.display, (Window)Xinfo.window);

    RWRETURN(TRUE);
}


RwBool
_rwOpenGLMakeCurrent()
{
    RWFUNCTION(RWSTRING("_rwOpenGLMakeCurrent"));
    RWRETURN(TRUE);
    /*
    glXMakeCurrent((Display*)Xinfo.display, (Window)Xinfo.window, Ctx));
    */
}

/****************************************************************************
 _rwOpenGLGetEngineWindowHeight

 On entry   : void
 On exit    : TRUE on success
 */

RwInt32
_rwOpenGLGetEngineWindowHeight()
{
    XWindowAttributes attribs;

    RWFUNCTION(RWSTRING("_rwOpenGLGetEngineWindowHeight"));

    XGetWindowAttributes((Display*)Xinfo.display, (Window)Xinfo.window, &attribs);

    RWRETURN(attribs.height);
}

/****************************************************************************
 _rwOpenGLGetEngineZBufferDepth

 On entry   : void
 On exit    : TRUE on success
 */

RwInt32
_rwOpenGLGetEngineZBufferDepth()
{
    RWFUNCTION(RWSTRING("_rwOpenGLGetEngineZBufferDepth"));

    RWRETURN(32);
}



#ifdef RWDEBUG

/* Do a check that our render state has not thrown up any errors */
void
_rwOpenGLErrorCheck( const RwChar filename[],
                     const RwUInt32 lineNumber )
{
    GLenum              errorFlag = glGetError();

    RWFUNCTION(RWSTRING("_rwOpenGLErrorCheck"));

    if (errorFlag != GL_NO_ERROR)
    {
        printf("Warning: OpenGL error flag set (");

        switch (errorFlag)
        {
            case GL_INVALID_ENUM:
                printf("invalid enumerant");
                break;
            case GL_INVALID_VALUE:
                printf("invalid value");
                break;
            case GL_INVALID_OPERATION:
                printf("invalid operation");
                break;
            case GL_STACK_OVERFLOW:
                printf("stack overflow");
                break;
            case GL_STACK_UNDERFLOW:
                printf("stack underflow");
                break;
            case GL_OUT_OF_MEMORY:
                printf("out of memory");
                break;
            default:
                printf("unknown error");
                break;
        }

        printf(") from file %s before line %d\n", filename, lineNumber );
    }

    RWRETURNVOID();
}

#endif


