/***************************************************************************
 *                                                                         *
 * Module  : baqnxogl.c                                                    *
 *                                                                         *
 * Purpose : QNX Photon OpenGL device layer                                *
 *                                                                         *
 **************************************************************************/

/****************************************************************************
 Includes
 */

/* Photon libraries */
#include <Pt.h>
//#include <Ph.h>

/* OpenGL libraries */
#include <photon/PdGL.h>

/* RW type definitions */
#include "badevice.h"

/* Common driver includes */
#include "barwtyp.h"

/* Modules needed */
#include "baintogl.h"
#include "bastdogl.h"

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
static PdOpenGLContext_t *pd_context = NULL;

static RwInt32 NumVideoModes    = -1;
static RwInt32 CurrentVideoMode = 0;
static RwVideoMode *VideoModes  = NULL;
static PtWidget_t  *DisplayWindow = NULL;
static PgVideoModes_t modeList;
static PgDisplaySettings_t OriginalDisplaySettings;


#define QNXTRY(_op) if(-1 == (_op)) { fprintf(stderr, "QNX_FAIL:%s:%d: %s\n", __FILE__, (int)__LINE__, #_op); return FALSE; }

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
  int numModes, i;

  RWFUNCTION(RWSTRING("OpenGLEnumerateVideoModes"));

  /* Query for video modes */
  {
    PgVideoModeInfo_t *modeInfo = NULL;

    PgGetVideoModeList(&modeList);

    if(modeList.num_modes == 0)
    {
      /* no modes :( */
      RWRETURN(FALSE);
    }

    /* Allocate space for mode info and retrieve */
    numModes = modeList.num_modes + 1;
    modeInfo = (PgVideoModeInfo_t*)RwDriverMalloc(sizeof(PgVideoModeInfo_t) * numModes,
                                              rwID_DRIVERMODULE | rwMEMHINTDUR_GLOBAL);
    VideoModes = (RwVideoMode*)RwDriverMalloc(sizeof(RwVideoMode) * numModes,
                                              rwID_DRIVERMODULE | rwMEMHINTDUR_GLOBAL);

    /* Get first device and set as target
     * BROKEN. For some nice reason these aren't actually defined anywhere but the documentation
     *         This causes the PgGetVideoMode call to fail. bastards.  This is because QNX forgot
     *         to put them in QNX6 patch B libs!
     *         This means we cannot enumerate any devices apart from the main display
     */

    /*PdGetDevices(&device, 1);
      PdSetTargetDevice(NULL, &device);
    */

    /* Get video mode information
     * First is desktop windowed mode
     *
     * HACK.  Due to broken libs again we have to call this multiple times to (hopefully) get the
     *        correct result. You also have to call PgGetVideoModeInfo each time otherwise it still
     *        don't work right.
     */
    for( i = 0; i < 5; i++)
    {
     PgGetVideoMode(&OriginalDisplaySettings);
     PgGetVideoModeInfo(OriginalDisplaySettings.mode, &modeInfo[0]);
    }

    PgGetVideoModeInfo(OriginalDisplaySettings.mode, &modeInfo[0]);

    for(i = 1; i < numModes; i++)
    {
      QNXTRY(PgGetVideoModeInfo(modeList.modes[i-1], &modeInfo[i]));
    }

    /* Set the renderware versions */
    for(i = 0; i < numModes; i++)
    {
      VideoModes[i].width  = modeInfo[i].width;
      VideoModes[i].height = modeInfo[i].height;
      VideoModes[i].depth  = modeInfo[i].bits_per_pixel;
      VideoModes[i].flags  = (i == 0) ? 0 : rwVIDEOMODEEXCLUSIVE;
#if (0)
      fprintf(stderr, "Mode %d: %dx%d : %d bit\n", i, modeInfo[i].width, modeInfo[i].height, modeInfo[i].bits_per_pixel);
#endif /* (0) */
    }
  }

  /* Set global data now we're done */
  NumVideoModes = numModes;

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


            _rwOpenGLIm2DOpen(device);
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
         DisplayWindow = (PtWidget_t*)(((RwEngineOpenParams*)pInOut)->displayID);

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
        uint64_t  glAttribs[32], ngl  = 0;
        PtArg_t   winArgs[32];
        int       numWinArgs = 0;
        PhDim_t   dim;
        PhPoint_t windowPos = { 0, 0};

        /* Force hardware */
        //      glAttribs[ngl++] = PHOGL_ATTRIB_FORCE_SW;

        dim.w = VideoModes[CurrentVideoMode].width;
        dim.h = VideoModes[CurrentVideoMode].height;

        /* Setup attributes */
        if(VideoModes[CurrentVideoMode].flags & rwVIDEOMODEEXCLUSIVE)
        {
          glAttribs[ngl++] = PHOGL_ATTRIB_FULLSCREEN;
          glAttribs[ngl++] = PHOGL_ATTRIB_DIRECT;
          glAttribs[ngl++] = PHOGL_ATTRIB_FULLSCREEN_BEST;
          glAttribs[ngl++] = PHOGL_ATTRIB_FULLSCREEN_CENTER;

          /* Resize window */
          PtSetArg(&winArgs[numWinArgs++], Pt_ARG_DIM, &dim, 0);

          /* Change video mode */
          {
        PgDisplaySettings_t settings;

        settings.mode = modeList.modes[CurrentVideoMode];
        settings.refresh = 0;
        settings.flags = 0;
        QNXTRY(PgSetVideoMode(&settings));
          }
          /* Setup window for full screen */
          {
          PtSetArg(&winArgs[numWinArgs++], Pt_ARG_WINDOW_STATE, Pt_TRUE,
               Ph_WM_STATE_ISFRONT |
               Ph_WM_STATE_ISFOCUS);

          PtSetArg(&winArgs[numWinArgs++], Pt_ARG_WINDOW_MANAGED_FLAGS, Pt_TRUE,
               Ph_WM_FFRONT   |
               Ph_WM_CLOSE    |
               Ph_WM_TOFRONT  |
               Ph_WM_CONSWITCH);

          PtSetArg(&winArgs[numWinArgs++], Pt_ARG_POS, &windowPos, 0L);

          PtSetArg(&winArgs[numWinArgs++], Pt_ARG_WINDOW_RENDER_FLAGS, Pt_FALSE, ~0);
          }
        }

        if(VideoModes[CurrentVideoMode].depth > 0)
        {
          glAttribs[ngl++] = PHOGL_ATTRIB_DEPTH_BITS;
          glAttribs[ngl++] = 16;
        }

        /*  End of attributes */
        glAttribs[ngl] = PHOGL_ATTRIB_NONE;

        /* Update window resources */
        if(numWinArgs)
          PtSetResources(DisplayWindow, numWinArgs, winArgs);

            /* Create a rendering context */
            pd_context = PdCreateOpenGLContext(2,         /* Number of buffers */
                           &dim,      /* Screen dimensions */
                           0,         /* Screen bit depth  */
                           glAttribs);/* Create attributes */

        if(NULL == pd_context)
        {
                fprintf(stderr, "Unable to make create render context\n");

                RWRETURN(FALSE);
            }

            /* Bind the rendering context to the window and force update*/
        if(NULL == PhDCSetCurrent(pd_context))
        {
         fprintf(stderr, "QNX FAIL: Unable to set rendering context\n");

         RWRETURN(FALSE);
        }

        QNXTRY(PtReRealizeWidget(DisplayWindow));
        QNXTRY(PtWindowFocus(DisplayWindow));

            /* Set up the rendering state */
            _rwOpenGLSetInitialRenderState();

            /* All done */
            RWRETURN(TRUE);
        }

        /* Puts back the hardware into the state it was in immediately
           after being opened */

        case rwDEVICESYSTEMSTOP:
        {

            /* close the renderstate module */
            _rwOpenGLRenderStateClose();

            /* Shut down OpenGL */
        if(pd_context)
        {
          PhDCSetCurrent(NULL);
          PhDCRelease(pd_context);
          pd_context = NULL;
        }

        /* Reset video mode if it has changed */
        if(OriginalDisplaySettings.mode != modeList.modes[CurrentVideoMode])
        {
        QNXTRY(PgSetVideoMode(&OriginalDisplaySettings));
        }

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

    QNXTRY(PdOpenGLContextSwapBuffers(pd_context));

    RWRETURN(TRUE);
}


RwBool
_rwOpenGLMakeCurrent()
{
    RWFUNCTION(RWSTRING("_rwOpenGLMakeCurrent"));

    RWRETURN(TRUE);
}

/****************************************************************************
 _rwOpenGLGetEngineWindowHeight

 On entry   : void
 On exit    : TRUE on success
 */

RwInt32
_rwOpenGLGetEngineWindowHeight()
{
    RWFUNCTION(RWSTRING("_rwOpenGLGetEngineWindowHeight"));


    RWRETURN(VideoModes[CurrentVideoMode].height);
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




