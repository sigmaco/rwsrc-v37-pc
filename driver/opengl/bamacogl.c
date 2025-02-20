/**************************************************************************
*                                                                         *
* Module  : bamacogl.c                                                    *
*                                                                         *
* Purpose : Apple Mac OpenGL device layer                                 *
*                                                                         *
**************************************************************************/

/**************************************************************************
 Includes
 */
#include <string.h>

/* OpenGL libraries */
#include "DrawSprocket.h"
#include <agl.h>

/* RW type definitions */
#include "badevice.h"

/* Common driver includes */
#include "baprocfp.h"
#include "barwtyp.h"
#include "drvfns.h"
#include "drvmodel.h"

/* Modules needed */
#include "baintogl.h"
#include "bastdogl.h"
#include "baim2dgl.h"
#include "bavagl.h"

/* This files header */
#include "baogl.h"

#define ReportError printf

/****************************************************************************
 Globals (across program)
 */
RwOpenGLExtensions _rwOpenGLExt;

/****************************************************************************
 Local (static) Globals
 */

WindowPtr  Win;
AGLContext Ctx;

static CGrafPtr Screen = NULL;

static RwVideoMode VideoModes[] =
    {
        { 640, 480, 16, 0 },
        { 640, 480, 16, rwVIDEOMODEEXCLUSIVE },
        { 640, 480, 32, rwVIDEOMODEEXCLUSIVE },
        { 800, 600, 16, rwVIDEOMODEEXCLUSIVE },
        { 800, 600, 32, rwVIDEOMODEEXCLUSIVE },
        { 1024, 768, 16, rwVIDEOMODEEXCLUSIVE },
        { 1024, 768, 32, rwVIDEOMODEEXCLUSIVE }
    };
static const RwInt32 NumVideoModes = 7;
static RwInt32 CurrentVideoMode = 0;

static DSpContextReference  gTheContext;


/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                            System handler

 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

static RwBool
OpenGLEnumerateVideoModes(void)
{
    GDHandle displayHandle;
    OSStatus error;

    RWFUNCTION(RWSTRING("OpenGLEnumerateVideoModes"));

    printf("Enumerating vid modes\n");

    error = DSpStartup();
    if (error)
    {
        RWRETURN(FALSE);
    }

    displayHandle = GetMainDevice();

    if (displayHandle)
    {
        OSStatus            err;
        DSpContextReference context;
        DisplayIDType       displayID;

        err = DMGetDisplayIDByGDevice(displayHandle, &displayID, true);

        if (DSpGetFirstContext(displayID, &context) == noErr)
        {
            do
            {
                DSpContextAttributes contextAttribs;

                DSpContext_GetAttributes(context, &contextAttribs);

                printf("CONTEXT WIDTH: %d HEIGHT %d\n",
                            contextAttribs.displayWidth, contextAttribs.displayHeight);
            }
            while (DSpGetNextContext(context, &context));

            RWRETURN(TRUE);
        }
    }

    RWRETURN(FALSE);
}


static CGrafPtr
OpenGLSetupFullScreen(void)
{
    OSStatus err;

    CGrafPtr theFrontBuffer;
    DSpContextAttributes contextAttribs;

    RWFUNCTION(RWSTRING("OpenGLSetupFullScreen"));

    err = DSpStartup();
    if (err)
    {
        RwDebugSendMessage(rwDEBUGMESSAGE, "OpenGLSetupFullScreen", "Unable to startup DrawSprocket");
        RWRETURN(NULL);
    }

#ifdef RWDEBUG
    DSpSetDebugMode(true);
#endif

    /* First of all, clear the structure or DrawSprocket can report an error */
    memset(&contextAttribs, 0, sizeof(DSpContextAttributes));
    contextAttribs.displayWidth     = (RwUInt32)VideoModes[CurrentVideoMode].width;
    contextAttribs.displayHeight    = (RwUInt32)VideoModes[CurrentVideoMode].height;
    contextAttribs.colorNeeds       = kDSpColorNeeds_Require;
    contextAttribs.displayDepthMask = ((VideoModes[CurrentVideoMode].depth == 16) ? kDSpDepthMask_16 : kDSpDepthMask_32);
    contextAttribs.displayBestDepth = (RwUInt32)VideoModes[CurrentVideoMode].depth;
    contextAttribs.pageCount        = 1;

    err = DSpFindBestContext( &contextAttribs, &gTheContext );
    if( err ) /* Unable to find a suitable device */
    {
        RwDebugSendMessage(rwDEBUGMESSAGE, "OpenGLSetupFullScreen", "Unable to find a suitable device");
        RWRETURN(NULL);
    }

    err = DSpContext_Reserve( gTheContext, &contextAttribs );
    if( err ) /* Unable to create the display */
    {
        RwDebugSendMessage(rwDEBUGMESSAGE, "OpenGLSetupFullScreen", "Unable to create the display");
        RWRETURN(NULL);
    }

    err = DSpContext_FadeGammaOut( NULL, NULL );
    if( err ) /* Unable to fade the display */
    {
        RwDebugSendMessage(rwDEBUGMESSAGE, "OpenGLSetupFullScreen", "Unable to fade out the desktop");
        RWRETURN(NULL);
    }

    err = DSpContext_SetState( gTheContext, kDSpContextState_Active );
    if( err ) /* Unable to set the display */
    {
        RwDebugSendMessage(rwDEBUGMESSAGE, "OpenGLSetupFullScreen", "Unable to set the display");
        RWRETURN(NULL);
    }

    err = DSpContext_FadeGammaIn( NULL, NULL );
    if( err ) /* Unable to fade the display */
    {
        RwDebugSendMessage(rwDEBUGMESSAGE, "OpenGLSetupFullScreen", "Unable to fade in RenderWare's display");
        RWRETURN(NULL);
    }

    {
        /* create a window to draw into */
        Rect r;
        AuxWinHandle awh;
        r.top = r.left = 0;
        DSpContext_LocalToGlobal(gTheContext, (Point *)&r);
        r.right = r.left + VideoModes[CurrentVideoMode].width;
        r.bottom = r.top + VideoModes[CurrentVideoMode].height;

        theFrontBuffer = (CGrafPtr)NewCWindow (NULL, &r, "\p", 0, plainDBox, (WindowPtr)-1, 0, 0);

        /* set the content color of the window to black to avoid a white flash when the window appears. */
        if (GetAuxWin ((WindowPtr)theFrontBuffer, &awh))
        {
            CTabHandle theColorTable;
            OSErr osErr;

            /*****/

            theColorTable = (**awh).awCTable;
            osErr = HandToHand((Handle*)&theColorTable);
            if (osErr) /* Out of memory */
            {
                RWRETURN(NULL);
            }

            (**theColorTable).ctTable[wContentColor].rgb.red = 0;
            (**theColorTable).ctTable[wContentColor].rgb.green = 0;
            (**theColorTable).ctTable[wContentColor].rgb.blue = 0;

            CTabChanged(theColorTable);

            /* the color table will be disposed by the window manager when the window is disposed */
            SetWinColor((WindowPtr)theFrontBuffer, (WCTabHandle)theColorTable);
        }

        ShowWindow((GrafPtr)theFrontBuffer);
        SetPort((GrafPtr)theFrontBuffer);
    }

    RWRETURN(theFrontBuffer);
}


static void
OpenGLShutDownFullScreen(CGrafPtr theFrontBuffer)
{
    RWFUNCTION(RWSTRING("OpenGLShutDownFullScreen"));

    DSpContext_FadeGammaOut( NULL, NULL );
    DisposeWindow((WindowPtr)theFrontBuffer);
    DSpContext_SetState( gTheContext, kDSpContextState_Inactive );
    DSpContext_FadeGammaIn( NULL, NULL );
    DSpContext_Release( gTheContext );
    DSpShutdown();

    RWRETURNVOID();
}


static RwBool
OpenGLLoadPlatformSpecificExtensions(void)
{
    RWFUNCTION( RWSTRING( "OpenGLLoadPlatformSpecificExtensions" ) );

    if ( FALSE != RwOpenGLIsExtensionSupported( "GL_APPLE_transform_hint" ) )
    {
        glHint( GL_TRANSFORM_HINT_APPLE, GL_FASTEST );
    }

    if ( FALSE != RwOpenGLIsExtensionSupported( "GL_EXT_compiled_vertex_array" ) )
    {
        _rwOpenGLExt.LockArrays   =
            (PFNGLLOCKARRAYSEXTPROC)glLockArraysEXT;

        _rwOpenGLExt.UnlockArrays =
           (PFNGLUNLOCKARRAYSEXTPROC)glUnlockArraysEXT;
    }

    if ( FALSE != RwOpenGLIsExtensionSupported( "GL_ARB_texture_env_add" ) )
    {
        _rwOpenGLExt.TextureEnvAddARB = TRUE;
    }

    if ( FALSE != RwOpenGLIsExtensionSupported( "GL_ARB_texture_cube_map" ) )
    {
        _rwOpenGLExt.TextureCubeMapARB = TRUE;
    }

    if ( FALSE != RwOpenGLIsExtensionSupported( "GL_EXT_texture_filter_anisotropic" ) )
    {
        glGetIntegerv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,
                       (GLint *)&(_rwOpenGLExt.MaxTextureAnisotropy) );
    }

    /* OpenGL 1.2 and above required from hereon */
    if ( rwstrcmp( (const RwChar *)glGetString( GL_VERSION ), RWSTRING("1.2") ) < 0 )
    {
        RWRETURN( TRUE );
    }

    if ( FALSE != RwOpenGLIsExtensionSupported( "GL_ARB_multitexture" ) )
    {
        _rwOpenGLExt.ActiveTextureARB =
           (PFNGLACTIVETEXTUREARBPROC)glActiveTextureARB;

        _rwOpenGLExt.ClientActiveTextureARB =
           (PFNGLCLIENTACTIVETEXTUREARBPROC)glClientActiveTextureARB;

        _rwOpenGLExt.MultiTexCoord2fvARB =
           (PFNGLMULTITEXCOORD2FVARBPROC)glMultiTexCoord2fvARB;

        glGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB,
                       (GLint *)&(_rwOpenGLExt.MaxTextureUnits) );

        /* GL_ARB_texture_env_combine is dependent on multitexturing */
        if ( FALSE != RwOpenGLIsExtensionSupported( "GL_ARB_texture_env_combine" ) )
        {
            _rwOpenGLExt.TextureEnvCombineARB = TRUE;
        }
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

            /* Mode out of range or mode switch not possible */
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

            if ((nIn >= 0) && (nIn < NumVideoModes))
            {
                *mode = VideoModes[nIn];

                RWRETURN(TRUE);
            }

            /* It's a bad time */
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

            _rwOpenGLIm2DOpen( device );

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
            /* Cache the window handle that the user has kindly passed in */
            Win = (WindowPtr)(((RwEngineOpenParams *)pInOut)->displayID);

            /* Open the OpenGL module */
            if (!_rwOpenGLOpen())
            {
                RWRETURN(FALSE);
            }

            OpenGLEnumerateVideoModes();

            /* assume the first video mode is selected */
            CurrentVideoMode = 0;

            RWRETURN(TRUE);
        }

        /* The purpose of Close is to remove any set up that was performed
         by Open */

        case rwDEVICESYSTEMCLOSE:
        {
            /* No mode selected */
            CurrentVideoMode = -1;

            /* Shut down OpenGL */
            _rwOpenGLClose();

            RWRETURN(TRUE);
        }

        /************** Starting stopping ******************/

        /* Start means that this bit of hardware has been selected for
         rendering - so get it into a state ready for rendering */

        case rwDEVICESYSTEMSTART:
        {
            AGLPixelFormat fmt = NULL;

            if ( rwVIDEOMODEEXCLUSIVE == VideoModes[CurrentVideoMode].flags )
            {
#if 0
                GLint FullScreenAttribs[] = { AGL_RGBA,
                                              AGL_DEPTH_SIZE, 16,
/*                                            AGL_STENCIL_SIZE, 16, */
                                              AGL_DOUBLEBUFFER,
                                              AGL_FULLSCREEN,
                                              AGL_NONE };
#else
                GLint FullScreenAttribs[] = { AGL_RGBA,
                                              AGL_DEPTH_SIZE, 16,
/*                                              AGL_STENCIL_SIZE, 16, */
                                              AGL_DOUBLEBUFFER,
                                              AGL_NONE };

                Screen = OpenGLSetupFullScreen();
                if ( NULL == Screen )
                {
                    RWRETURN(FALSE);
                }
#endif

                fmt = aglChoosePixelFormat( NULL, 0, FullScreenAttribs );
            }
            else
            {
                GLint WindowAttribs[]     = { AGL_RGBA,
                                              AGL_DEPTH_SIZE, 16,
/*                                              AGL_STENCIL_SIZE, 16, */
                                              AGL_DOUBLEBUFFER,
                                              AGL_ACCELERATED,
                                              AGL_RED_SIZE, 8,
                                              AGL_GREEN_SIZE, 8,
                                              AGL_BLUE_SIZE, 8,
                                              AGL_ALL_RENDERERS,
                                              AGL_NONE };

                fmt = aglChoosePixelFormat(NULL, 0, WindowAttribs);
            }

            /* Check that we have the requested pixel format */
            if (!fmt)
            {
                if (VideoModes[CurrentVideoMode].flags == rwVIDEOMODEEXCLUSIVE)
                {
                    OpenGLShutDownFullScreen(Screen);
                }
                RWRETURN(FALSE);
            }

            /* Let's create our context! */
            Ctx = aglCreateContext(fmt, NULL);

            /* Check that we have a valid context */
            if (!Ctx)
            {
                if (VideoModes[CurrentVideoMode].flags == rwVIDEOMODEEXCLUSIVE)
                {
                    OpenGLShutDownFullScreen(Screen);
                }
                RWRETURN(FALSE);
            }

            if (VideoModes[CurrentVideoMode].flags == rwVIDEOMODEEXCLUSIVE)
            {
#if 0
                if (!aglSetFullScreen(Ctx, 640, 480, 60, 0))
#else
                if (!aglSetDrawable(Ctx, (AGLDrawable)Screen))
#endif
                {
                    OpenGLShutDownFullScreen(Screen);
                    RWRETURN(FALSE);
                }
            }
            else /* we are windowed */
            {
                if (!aglSetDrawable(Ctx, (AGLDrawable)Win))
                {
                    RWRETURN(FALSE);
                }
            }

            /* Set up the display */
            if (!aglSetCurrentContext(Ctx))
            {
                if (VideoModes[CurrentVideoMode].flags == rwVIDEOMODEEXCLUSIVE)
                {
                    OpenGLShutDownFullScreen(Screen);
                }
                RWRETURN(FALSE);
            }

#if defined( RWDEBUG )

            if ( NULL != RWSRCGLOBAL( debugFunction ) )
            {
                RwChar  buffer[80];

                GLint   aglPixFormatValue;


                RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, "OpenGL driver information" );
                RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, "-------------------------" );

                rwsprintf( buffer, "GL_VENDOR:\t\t\t\t%s", glGetString( GL_VENDOR ) );
                RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                rwsprintf( buffer, "GL_RENDERER:\t\t\t%s", glGetString( GL_RENDERER ) );
                RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                rwsprintf( buffer, "GL_VERSION:\t\t\t\t%s", glGetString( GL_VERSION ) );
                RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                rwsprintf( buffer, "GL_EXTENSIONS:" );
                RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );
                _rwOpenGLEnumExtensions( _rwOpenGLDebugOutputExtensionsCB, NULL, NULL );

                RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, "\n" );

                RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, "Pixel format information" );
                RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, "------------------------" );

                /* color buffer */
                if ( GL_FALSE != aglDescribePixelFormat( fmt, AGL_PIXEL_SIZE, &aglPixFormatValue ) )
                {
                    rwsprintf( buffer, "Color depth\t\t\t\t\t\t%d bits", aglPixFormatValue );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );
                }

                rwsprintf( buffer, "\t\t\t\t\t\t\t\tBit size" );
                RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                if ( GL_FALSE != aglDescribePixelFormat( fmt, AGL_RED_SIZE, &aglPixFormatValue ) )
                {
                    rwsprintf( buffer, "\tRed component\t:\t\t\t%d", aglPixFormatValue );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );
                }

                if ( GL_FALSE != aglDescribePixelFormat( fmt, AGL_GREEN_SIZE, &aglPixFormatValue ) )
                {
                    rwsprintf( buffer, "\tGreen component\t:\t\t\t%d", aglPixFormatValue );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );
                }

                if ( GL_FALSE != aglDescribePixelFormat( fmt, AGL_BLUE_SIZE, &aglPixFormatValue ) )
                {
                    rwsprintf( buffer, "\tBlue component\t:\t\t\t%d", aglPixFormatValue );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );
                }

                if ( GL_FALSE != aglDescribePixelFormat( fmt, AGL_ALPHA_SIZE, &aglPixFormatValue ) )
                {
                    rwsprintf( buffer, "\tAlpha component\t:\t\t\t%d", aglPixFormatValue );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );
                }

                /* accumulation buffer */
                rwsprintf( buffer, "Accumulation buffers" );
                RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                rwsprintf( buffer, "\t\t\t\t\t\t\t\tBit size" );
                RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                if ( GL_FALSE != aglDescribePixelFormat( fmt, AGL_ACCUM_RED_SIZE, &aglPixFormatValue ) )
                {
                    rwsprintf( buffer, "\tRed component\t:\t\t\t%d", aglPixFormatValue );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );
                }

                if ( GL_FALSE != aglDescribePixelFormat( fmt, AGL_ACCUM_GREEN_SIZE, &aglPixFormatValue ) )
                {
                    rwsprintf( buffer, "\tGreen component\t:\t\t\t%d", aglPixFormatValue );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );
                }

                if ( GL_FALSE != aglDescribePixelFormat( fmt, AGL_ACCUM_BLUE_SIZE, &aglPixFormatValue ) )
                {
                    rwsprintf( buffer, "\tBlue component\t:\t\t\t%d", aglPixFormatValue );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );
                }

                if ( GL_FALSE != aglDescribePixelFormat( fmt, AGL_ACCUM_ALPHA_SIZE, &aglPixFormatValue ) )
                {
                    rwsprintf( buffer, "\tAlpha component\t:\t\t\t%d", aglPixFormatValue );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );
                }

                /* z-buffer */
                if ( GL_FALSE != aglDescribePixelFormat( fmt, AGL_DEPTH_SIZE, &aglPixFormatValue ) )
                {
                    rwsprintf( buffer, "Z-buffer depth\t\t\t\t\t%d bits", aglPixFormatValue );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );
                }

                /* stencil buffer */
                if ( GL_FALSE != aglDescribePixelFormat( fmt, AGL_STENCIL_SIZE, &aglPixFormatValue ) )
                {
                    rwsprintf( buffer, "Stencil buffer depth\t\t\t%d bits", aglPixFormatValue );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );
                }

                /* auxiliary buffers */
                if ( GL_FALSE != aglDescribePixelFormat( fmt, AGL_AUX_BUFFERS, &aglPixFormatValue ) )
                {
                    rwsprintf( buffer, "Number of auxiliary buffers\t\t%d", aglPixFormatValue );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );
                }

                RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, "\n" );
            }

#endif /* defined( RWDEBUG ) */

            aglDestroyPixelFormat(fmt);

            OpenGLLoadPlatformSpecificExtensions();

            /* open the vertex array module */
            if ( FALSE == _rwOpenGLVAOpen() )
            {
                RWRETURN( FALSE );
            }

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

            /* close the vertex array module */
            _rwOpenGLVAClose();

            /* destroy the extensions string */
            _rwOpenGLDestroyExtensionsString();

            /* Deselect our current rendering context */
            aglSetCurrentContext(NULL);

            /* First disassociate our rendering context from its window */
            aglSetDrawable(Ctx, NULL);

            /* And then destroy the context itself */
            aglDestroyContext(Ctx);

            if (Screen)
            {
                OpenGLShutDownFullScreen(Screen);
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
            _rwOpenGLSetStandards((RwStandardFunc *)pOut, nIn);

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
_rwOpenGLRasterShowRaster(void *pInOut, void *pIn, RwInt32 nIn)
{
    RwRaster    *camRas = (RwRaster *)pInOut;

    RWFUNCTION(RWSTRING("_rwOpenGLRasterShowRaster"));
    /* pInOut is raster */
    /* pIn is a WindowPtr */

    aglSwapBuffers(Ctx);

    if (aglGetError() == AGL_BAD_CONTEXT)
    {
        RWRETURN(FALSE);
    }

    /* if using the NVIDIA vertex array range extension, check whether
     * a resize is required, and perform it when necessary */
    if ( FALSE != _rwOpenGLVertexHeapAvailable() )
    {
        _rwOpenGLVertexHeapTestAndPerformResize();
    }

    RWRETURN(TRUE);
}


/****************************************************************************
 _rwOpenGLMakeCurrent

 On entry   : void
 On exit    : TRUE on success
 */

RwBool
_rwOpenGLMakeCurrent()
{
    RWFUNCTION(RWSTRING("_rwOpenGLMakeCurrent"));

    if (aglGetCurrentContext() != Ctx)
    {
        if (aglSetCurrentContext(Ctx) == GL_FALSE)
        {
            RWRETURN(FALSE);
        }

        if (aglGetError() == AGL_BAD_CONTEXT)
        {
            RWRETURN(FALSE);
        }
    }

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

    if (VideoModes[CurrentVideoMode].flags == rwVIDEOMODEEXCLUSIVE)
    {
        RWRETURN(VideoModes[CurrentVideoMode].height);
    }

    RWRETURN(480);
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
    RWRETURN(16);
}


RwBool
_rwOpenGLSelectStencilBufferDepth(RwUInt8 depth)
{
    RWFUNCTION(RWSTRING("_rwOpenGLSelectStencilBufferDepth"));

    if (RWSRCGLOBAL(engineStatus) != rwENGINESTATUSSTARTED)
    {
        RWRETURN(TRUE);
    }
    RWRETURN(FALSE);
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



#ifdef RWDEBUG

/* Do a check that our render state has not thrown up any errors */
void
_rwOpenGLErrorCheck( const RwChar filename[],
                     const RwUInt32 lineNumber )
{
    GLenum errorFlag = glGetError();

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

