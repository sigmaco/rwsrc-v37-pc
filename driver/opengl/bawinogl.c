/***************************************************************************
 *                                                                         *
 * Module  : bawinogl.c                                                    *
 *                                                                         *
 * Purpose : OpenGL device layer                                           *
 *                                                                         *
 **************************************************************************/

/****************************************************************************
 Includes
 */

#if defined(WIN32)
#include <windows.h>
#endif /* defined(WIN32) */
#include <GL/gl.h>

#include "barwtyp.h"
#include "baimage.h"
#include "batextur.h"

#include "cpuext.h"
#if !defined( NOASM )
#include "x86matml.h"
#include "x86matvc.h"
#include "baprocfp.h"
#endif /* !defined( NOASM ) */

#include "baogl.h"
#include "bastdogl.h"
#include "drvfns.h"

#include "barastgl.h"
#include "baim2dgl.h"
#include "baintogl.h"
#include "bavagl.h"
#include "barstate.h"


#if 0
#define OFFEREXTENSION(extension)                                   \
    (MessageBox(Window,                                             \
               TEXT(extension),                                     \
               TEXT("OpenGL Extensions"),                           \
               MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL) == IDYES)
#else
#define OFFEREXTENSION(extension) 1
#endif

#define IssueDebugString(_str) OutputDebugString((LPCSTR)(_str));

#if (!defined(ENUM_CURRENT_SETTINGS))
/* C.f
 * Program Files/Microsoft Visual Studio/VC98/Include/WINUSER.H
 */
#define ENUM_CURRENT_SETTINGS       (~0)
#endif /* (!defined(ENUM_CURRENT_SETTINGS)) */

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Globals (across program)
 */

RwOpenGLExtensions  _rwOpenGLExt;

PIXELFORMATDESCRIPTOR _rwOpenGLPixelFormatDesc =
{
    sizeof(PIXELFORMATDESCRIPTOR), /* size of this pfd */
    1,                             /* version number */
    PFD_DRAW_TO_WINDOW  |          /* support window */
     PFD_SUPPORT_OPENGL |          /* support OpenGL */
      PFD_DOUBLEBUFFER,            /* double buffered */
    PFD_TYPE_RGBA,                 /* RGBA type */
    16,                            /* 16-bit color depth */
    0, 0, 0, 0, 0, 0,              /* color bits ignored */
    0,                             /* no alpha buffer */
    0,                             /* shift bit ignored */
    0,                             /* no accumulation buffer */
    0, 0, 0, 0,                    /* accum bits ignored */
    16,                            /* 16-bit z-buffer */
    8,                             /* 8-bit stencil buffer */
    0,                             /* no auxiliary buffer */
    PFD_MAIN_PLANE,                /* main layer */
    0,                             /* reserved */
    0, 0, 0                        /* layer masks ignored */
};


/****************************************************************************
 Local (static) Globals
 */

static HGLRC        RenderContext;
static HWND         Window;
static HDC          DeviceContext;

typedef struct
{
    RwBool              fullScreen;
    DEVMODE             vidMode;
}
rwOglVideoMode;

static rwOglVideoMode *VideoModes = NULL;

static RwInt32      NumVideoModes = 0;
static RwInt32      CurrentVideoMode = 0;

static RwBool       SSEsupported = FALSE;

static DWORD        cachedWinStyle = 0;
static RECT         cachedWinRect = { 0, 0, 0, 0 };
static RwBool       engineStarted = FALSE;


/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                            System handler

 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */


static int __cdecl
OpenGLCompareVideoModes(const void *elem1, const void *elem2)
{
    const rwOglVideoMode *mode1 =
        (const rwOglVideoMode *)elem1;
    const rwOglVideoMode *mode2 =
        (const rwOglVideoMode *)elem2;

    RWFUNCTION(RWSTRING("OpenGLCompareVideoModes"));
    RWASSERT(elem1);
    RWASSERT(elem2);

    if ((mode1->vidMode.dmBitsPerPel == mode2->vidMode.dmBitsPerPel) &&
        (mode1->vidMode.dmPelsWidth  == mode2->vidMode.dmPelsWidth)  &&
        (mode1->vidMode.dmPelsHeight == mode2->vidMode.dmPelsHeight))
    {
        RWRETURN(0);
    }

    if ((mode1->vidMode.dmBitsPerPel == mode2->vidMode.dmBitsPerPel) &&
        (mode1->vidMode.dmPelsWidth  == mode2->vidMode.dmPelsWidth)  &&
        (mode1->vidMode.dmPelsHeight <  mode2->vidMode.dmPelsHeight))
    {
        RWRETURN(-1);
    }

    if ((mode1->vidMode.dmBitsPerPel == mode2->vidMode.dmBitsPerPel) &&
        (mode1->vidMode.dmPelsWidth  <  mode2->vidMode.dmPelsWidth))
    {
        RWRETURN(-1);
    }

    if (mode1->vidMode.dmBitsPerPel  <  mode2->vidMode.dmBitsPerPel)
    {
        RWRETURN(-1);
    }

    RWRETURN(1);
}


static RwBool
OpenGLEnumerateVideoModes(void)
{
    DEVMODE             devmode; /* dummy device mode settings */
    RwInt32             count = 0;

    RWFUNCTION(RWSTRING("OpenGLEnumerateVideoModes"));

    /*
     * Get the total number of modes so we can allocate memory for all
     * of them.
     */
    NumVideoModes = 0;
    while (EnumDisplaySettings(NULL, NumVideoModes, &devmode))
    {
        NumVideoModes++;
    }

    /*
     * Fill an array with all the devmodes so we don't have to keep
     * grabbing them.
     */
    VideoModes =
        (rwOglVideoMode *) RwDriverMalloc(sizeof(rwOglVideoMode) *
                                          NumVideoModes,
                                          rwID_DRIVERMODULE |
                                          rwMEMHINTDUR_GLOBAL);

    if (VideoModes == NULL)
    {
        RWRETURN(FALSE);
    }

    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &VideoModes[0].vidMode);
    VideoModes[0].fullScreen = FALSE;

    NumVideoModes = 1;
    while (EnumDisplaySettings(NULL, count++, &devmode))
    {
        RwInt32             currModeTotal = NumVideoModes - 1;
        RwBool              modeNotFound = TRUE;

        while (currModeTotal)
        {
            if (
                ((devmode.dmPelsWidth ==
                  VideoModes[currModeTotal].vidMode.dmPelsWidth)
                 && (devmode.dmPelsHeight ==
                     VideoModes[currModeTotal].vidMode.dmPelsHeight)
                 && (devmode.dmBitsPerPel ==
                     VideoModes[currModeTotal].vidMode.dmBitsPerPel)))
            {
                modeNotFound = FALSE;
            }

            currModeTotal--;
        }

        /* We've found a new mode!  And let's make sure it has a usable depth */
        if (modeNotFound && (devmode.dmBitsPerPel >= 16))
        {
            VideoModes[NumVideoModes].vidMode = devmode;
            VideoModes[NumVideoModes].fullScreen = TRUE;

            NumVideoModes++;
        }
    }

    /* Finally, sort our video mode list into some nice order */
    qsort(VideoModes + 1, NumVideoModes-1,
          sizeof(rwOglVideoMode),
          OpenGLCompareVideoModes);

    RWRETURN(TRUE);
}

static RwBool
OpenGLLoadExtensions(void)
{
    static RwChar _glActiveTextureARB[]                 = RWSTRING( "glActiveTextureARB" );
    static RwChar _glClientActiveTextureARB[]           = RWSTRING( "glClientActiveTextureARB" );
    static RwChar _glMultiTexCoord2fvARB[]              = RWSTRING( "glMultiTexCoord2fvARB" );
    static RwChar _glFlushVertexArrayRangeNV[]          = RWSTRING( "glFlushVertexArrayRangeNV" );
    static RwChar _glLockArraysEXT[]                    = RWSTRING( "glLockArraysEXT" );
    static RwChar _glLockArraysSGI[]                    = RWSTRING( "glLockArraysSGI" );
    static RwChar _glUnlockArraysEXT[]                  = RWSTRING( "glUnlockArraysEXT" );
    static RwChar _glUnlockArraysSGI[]                  = RWSTRING( "glUnlockArraysSGI" );
    static RwChar _glVertexArrayRangeNV[]               = RWSTRING( "glVertexArrayRangeNV" );
    static RwChar _wglAllocateMemoryNV[]                = RWSTRING( "wglAllocateMemoryNV" );
    static RwChar _wglFreeMemoryNV[]                    = RWSTRING( "wglFreeMemoryNV" );
    static RwChar _glGenFencesNV[]                      = RWSTRING( "glGenFencesNV" );
    static RwChar _glDeleteFencesNV[]                   = RWSTRING( "glDeleteFencesNV" );
    static RwChar _glSetFenceNV[]                       = RWSTRING( "glSetFenceNV" );
    static RwChar _glFinishFenceNV[]                    = RWSTRING( "glFinishFenceNV" );
    static RwChar _glIsFenceNV[]                        = RWSTRING( "glIsFenceNV" );
    static RwChar _glTestFenceNV[]                      = RWSTRING( "glTestFenceNV" );
    static RwChar _glNewObjectBufferATI[]               = RWSTRING( "glNewObjectBufferATI" );
    static RwChar _glIsObjectBufferATI[]                = RWSTRING( "glIsObjectBufferATI" );
    static RwChar _glFreeObjectBufferATI[]              = RWSTRING( "glFreeObjectBufferATI" );
    static RwChar _glUpdateObjectBufferATI[]            = RWSTRING( "glUpdateObjectBufferATI" );
    static RwChar _glArrayObjectATI[]                   = RWSTRING( "glArrayObjectATI" );
    static RwChar _glCompressedTexImage3DARB[]          = RWSTRING( "glCompressedTexImage3DARB" );
    static RwChar _glCompressedTexImage2DARB[]          = RWSTRING( "glCompressedTexImage2DARB" );
    static RwChar _glCompressedTexImage1DARB[]          = RWSTRING( "glCompressedTexImage1DARB" );
    static RwChar _glCompressedTexSubImage3DARB[]       = RWSTRING( "glCompressedTexSubImage3DARB" );
    static RwChar _glCompressedTexSubImage2DARB[]       = RWSTRING( "glCompressedTexSubImage2DARB" );
    static RwChar _glCompressedTexSubImage1DARB[]       = RWSTRING( "glCompressedTexSubImage1DARB" );
    static RwChar _glGetCompressedTexImageARB[]         = RWSTRING( "glGetCompressedTexImageARB" );

    static RwChar _GL_ARB_multitexture[]                = RWSTRING( "GL_ARB_multitexture" );
    static RwChar _GL_EXT_compiled_vertex_array[]       = RWSTRING( "GL_EXT_compiled_vertex_array" );
    static RwChar _GL_NV_vertex_array_range[]           = RWSTRING( "GL_NV_vertex_array_range" );
    static RwChar _GL_NV_fence[]                        = RWSTRING( "GL_NV_fence" );
    static RwChar _GL_SGI_compiled_vertex_array[]       = RWSTRING( "GL_SGI_compiled_vertex_array" );
    static RwChar _GL_ARB_texture_env_combine[]         = RWSTRING( "GL_ARB_texture_env_combine" );
    static RwChar _GL_ARB_texture_env_add[]             = RWSTRING( "GL_ARB_texture_env_add" );
    static RwChar _GL_ARB_texture_cube_map[]            = RWSTRING( "GL_ARB_texture_cube_map" );
    static RwChar _GL_NV_vertex_array_range2[]          = RWSTRING( "GL_NV_vertex_array_range2" );
    static RwChar _GL_EXT_texture_filter_anisotropic[]  = RWSTRING( "GL_EXT_texture_filter_anisotropic" );
    static RwChar _GL_ATI_vertex_array_object[]         = RWSTRING( "GL_ATI_vertex_array_object" );
    static RwChar _GL_ARB_texture_compression[]         = RWSTRING( "GL_ARB_texture_compression" );
    static RwChar _GL_EXT_texture_compression_s3tc[]    = RWSTRING( "GL_EXT_texture_compression_s3tc" );
    static RwChar _GL_ARB_texture_mirrored_repeat[]     = RWSTRING( "GL_ARB_texture_mirrored_repeat" );
    static RwChar _GL_SGIS_generate_mipmap[]            = RWSTRING( "GL_SGIS_generate_mipmap" );
    static RwChar _GL_EXT_texture_edge_clamp[]          = RWSTRING( "GL_EXT_texture_edge_clamp" );
    static RwChar _GL_NV_blend_square[]                 = RWSTRING( "GL_NV_blend_square" );


    RWFUNCTION( RWSTRING( "OpenGLLoadExtensions" ) );

    /* Set up extensions */
    if ( FALSE != RwOpenGLIsExtensionSupported( _GL_EXT_compiled_vertex_array ) )
    {
        if ( OFFEREXTENSION(_GL_EXT_compiled_vertex_array) )
        {
            _rwOpenGLExt.LockArrays =
                (PFNGLLOCKARRAYSEXTPROC)wglGetProcAddress(_glLockArraysEXT);
            RWASSERT( NULL != _rwOpenGLExt.LockArrays );

            _rwOpenGLExt.UnlockArrays =
                (PFNGLUNLOCKARRAYSEXTPROC)wglGetProcAddress(_glUnlockArraysEXT);
            RWASSERT( NULL != _rwOpenGLExt.UnlockArrays );
        }
    }
    else if ( FALSE != RwOpenGLIsExtensionSupported( _GL_SGI_compiled_vertex_array ) )
    {
        /* Maybe we're using SGI's implementation rather than M$'s */
        if ( OFFEREXTENSION(_GL_SGI_compiled_vertex_array) )
        {
            _rwOpenGLExt.LockArrays =
                (PFNGLLOCKARRAYSEXTPROC)wglGetProcAddress(_glLockArraysSGI);
            RWASSERT( NULL != _rwOpenGLExt.LockArrays );

            _rwOpenGLExt.UnlockArrays =
                (PFNGLUNLOCKARRAYSEXTPROC)wglGetProcAddress(_glUnlockArraysSGI);
            RWASSERT( NULL != _rwOpenGLExt.UnlockArrays );
        }
    }

    if ( FALSE != RwOpenGLIsExtensionSupported( _GL_NV_fence ) )
    {
        if ( OFFEREXTENSION(_GL_NV_fence) )
        {
            _rwOpenGLExt.GenFencesNV =
                (PFNGLGENFENCESNVPROC)wglGetProcAddress( _glGenFencesNV );
            RWASSERT( NULL != _rwOpenGLExt.GenFencesNV );

            _rwOpenGLExt.DeleteFencesNV =
                (PFNGLDELETEFENCESNVPROC)wglGetProcAddress( _glDeleteFencesNV );
            RWASSERT( NULL != _rwOpenGLExt.DeleteFencesNV );

            _rwOpenGLExt.SetFenceNV =
                (PFNGLSETFENCENVPROC)wglGetProcAddress( _glSetFenceNV );
            RWASSERT( NULL != _rwOpenGLExt.SetFenceNV );

            _rwOpenGLExt.FinishFenceNV =
                (PFNGLFINISHFENCENVPROC)wglGetProcAddress( _glFinishFenceNV );
            RWASSERT( NULL != _rwOpenGLExt.FinishFenceNV );

            _rwOpenGLExt.IsFenceNV =
                (PFNGLISFENCENVPROC)wglGetProcAddress( _glIsFenceNV );
            RWASSERT( NULL != _rwOpenGLExt.IsFenceNV );

            _rwOpenGLExt.TestFenceNV =
                (PFNGLTESTFENCENVPROC)wglGetProcAddress( _glTestFenceNV );
            RWASSERT( NULL != _rwOpenGLExt.TestFenceNV );
        }
    }

    if ( FALSE != RwOpenGLIsExtensionSupported( _GL_NV_vertex_array_range ) )
    {
        if ( OFFEREXTENSION(_GL_NV_vertex_array_range) )
        {
            _rwOpenGLExt.FlushVertexArrayRangeNV =
                (PFNGLFLUSHVERTEXARRAYRANGENVPROC)wglGetProcAddress( _glFlushVertexArrayRangeNV );
            RWASSERT( NULL != _rwOpenGLExt.FlushVertexArrayRangeNV );

            _rwOpenGLExt.VertexArrayRangeNV =
                (PFNGLVERTEXARRAYRANGENVPROC)wglGetProcAddress( _glVertexArrayRangeNV );
            RWASSERT( NULL != _rwOpenGLExt.VertexArrayRangeNV );

            _rwOpenGLExt.AllocateMemoryNV =
                (PFNWGLALLOCATEMEMORYNVPROC)wglGetProcAddress( _wglAllocateMemoryNV );
            RWASSERT( NULL != _rwOpenGLExt.AllocateMemoryNV );

            _rwOpenGLExt.FreeMemoryNV =
                (PFNWGLFREEMEMORYNVPROC)wglGetProcAddress( _wglFreeMemoryNV );
            RWASSERT( NULL != _rwOpenGLExt.FreeMemoryNV );

            /* assumes support for GL_NV_vertex_array_range */
            if ( FALSE != RwOpenGLIsExtensionSupported( _GL_NV_vertex_array_range2 ) )
            {
                if ( OFFEREXTENSION( _GL_NV_vertex_array_range2 ) )
                {
                    _rwOpenGLExt.VertexArrayRange2NV = TRUE;
                }
            }

            /* allocate video memory for vertex storage */
            if ( FALSE == _rwOpenGLVertexHeapCreate() )
            {
                _rwOpenGLExt.FlushVertexArrayRangeNV = NULL;
                _rwOpenGLExt.VertexArrayRangeNV      = NULL;
                _rwOpenGLExt.AllocateMemoryNV        = NULL;
                _rwOpenGLExt.FreeMemoryNV            = NULL;

                _rwOpenGLExt.GenFencesNV             = NULL;
                _rwOpenGLExt.DeleteFencesNV          = NULL;
                _rwOpenGLExt.SetFenceNV              = NULL;
                _rwOpenGLExt.FinishFenceNV           = NULL;
                _rwOpenGLExt.IsFenceNV               = NULL;
                _rwOpenGLExt.TestFenceNV             = NULL;

                _rwOpenGLExt.VertexArrayRange2NV     = FALSE;
            }
        }
    }

    if ( FALSE != RwOpenGLIsExtensionSupported( _GL_ARB_texture_env_add ) )
    {
        if ( OFFEREXTENSION( _GL_ARB_texture_env_add ) )
        {
            _rwOpenGLExt.TextureEnvAddARB = TRUE;
        }
    }

    if ( FALSE != RwOpenGLIsExtensionSupported( _GL_ARB_texture_cube_map ) )
    {
        if ( OFFEREXTENSION( _GL_ARB_texture_cube_map ) )
        {
            _rwOpenGLExt.TextureCubeMapARB = TRUE;
            glGetIntegerv( GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, (GLint *)&(_rwOpenGLExt.MaxCubeMapSize) );
        }
    }

    if ( FALSE != RwOpenGLIsExtensionSupported( _GL_EXT_texture_filter_anisotropic ) )
    {
        if ( OFFEREXTENSION( _GL_EXT_texture_filter_anisotropic ) )
        {
            glGetIntegerv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, (GLint *)&(_rwOpenGLExt.MaxTextureAnisotropy) );
        }
    }

    if ( FALSE != RwOpenGLIsExtensionSupported( _GL_ARB_texture_mirrored_repeat ) )
    {
        if ( OFFEREXTENSION( _GL_ARB_texture_mirrored_repeat ) )
        {
            _rwOpenGLExt.TextureMirroredRepeatARB = TRUE;
        }
    }

    if ( FALSE != RwOpenGLIsExtensionSupported( _GL_SGIS_generate_mipmap ) )
    {
        if ( OFFEREXTENSION( _GL_SGIS_generate_mipmap ) )
        {
            _rwOpenGLExt.GenerateMipmapSGIS = TRUE;
        }
    }

    if ( FALSE != RwOpenGLIsExtensionSupported( _GL_NV_blend_square ) )
    {
        if ( OFFEREXTENSION( _GL_NV_blend_square ) )
        {
            _rwOpenGLExt.BlendSquareNV = TRUE;
        }
    }

    /* OpenGL 1.1 and above required from hereon */
    if ( rwstrcmp( glGetString( GL_VERSION ), RWSTRING("1.1") ) < 0 )
    {
        RWRETURN( TRUE );
    }

    if ( FALSE != RwOpenGLIsExtensionSupported( _GL_ATI_vertex_array_object ) )
    {
        if ( OFFEREXTENSION( _GL_ATI_vertex_array_object ) )
        {
            _rwOpenGLExt.NewObjectBufferATI =
                (PFNGLNEWOBJECTBUFFERATIPROC)wglGetProcAddress(_glNewObjectBufferATI);
            RWASSERT( NULL != _rwOpenGLExt.NewObjectBufferATI );

            _rwOpenGLExt.IsObjectBufferATI =
                (PFNGLISOBJECTBUFFERATIPROC)wglGetProcAddress(_glIsObjectBufferATI);
            RWASSERT( NULL != _rwOpenGLExt.IsObjectBufferATI );

            _rwOpenGLExt.FreeObjectBufferATI =
                (PFNGLFREEOBJECTBUFFERATIPROC)wglGetProcAddress(_glFreeObjectBufferATI);
            RWASSERT( NULL != _rwOpenGLExt.FreeObjectBufferATI );

            _rwOpenGLExt.UpdateObjectBufferATI =
                (PFNGLUPDATEOBJECTBUFFERATIPROC)wglGetProcAddress(_glUpdateObjectBufferATI);
            RWASSERT( NULL != _rwOpenGLExt.UpdateObjectBufferATI );

            _rwOpenGLExt.ArrayObjectATI =
                (PFNGLARRAYOBJECTATIPROC)wglGetProcAddress(_glArrayObjectATI);
            RWASSERT( NULL != _rwOpenGLExt.ArrayObjectATI );
        }
    }

    if ( FALSE != _rwOpenGLExt.TextureCubeMapARB )
    {
        /* texture compression depends on cube map as cube maps may be stored in compressed form */
        if ( FALSE != RwOpenGLIsExtensionSupported( _GL_ARB_texture_compression ) )
        {
            if ( OFFEREXTENSION( _GL_ARB_texture_compression ) )
            {
                _rwOpenGLExt.CompressedTexImage3DARB        =
                    (PFNGLCOMPRESSEDTEXIMAGE3DARB)wglGetProcAddress(_glCompressedTexImage3DARB);
                RWASSERT( NULL != _rwOpenGLExt.CompressedTexImage3DARB );

                _rwOpenGLExt.CompressedTexImage2DARB        =
                    (PFNGLCOMPRESSEDTEXIMAGE2DARB)wglGetProcAddress(_glCompressedTexImage2DARB);
                RWASSERT( NULL != _rwOpenGLExt.CompressedTexImage2DARB );

                _rwOpenGLExt.CompressedTexImage1DARB        =
                    (PFNGLCOMPRESSEDTEXIMAGE1DARB)wglGetProcAddress(_glCompressedTexImage1DARB);
                RWASSERT( NULL != _rwOpenGLExt.CompressedTexImage1DARB );

                _rwOpenGLExt.CompressedTexSubImage3DARB     =
                    (PFNGLCOMPRESSEDTEXSUBIMAGE3DARB)wglGetProcAddress(_glCompressedTexSubImage3DARB);
                RWASSERT( NULL != _rwOpenGLExt.CompressedTexSubImage3DARB );

                _rwOpenGLExt.CompressedTexSubImage2DARB     =
                    (PFNGLCOMPRESSEDTEXSUBIMAGE2DARB)wglGetProcAddress(_glCompressedTexSubImage2DARB);
                RWASSERT( NULL != _rwOpenGLExt.CompressedTexSubImage2DARB );

                _rwOpenGLExt.CompressedTexSubImage1DARB     =
                    (PFNGLCOMPRESSEDTEXSUBIMAGE1DARB)wglGetProcAddress(_glCompressedTexSubImage1DARB);
                RWASSERT( NULL != _rwOpenGLExt.CompressedTexSubImage1DARB );

                _rwOpenGLExt.GetCompressedTexImageARB       =
                    (PFNGLGETCOMPRESSEDTEXIMAGEARB)wglGetProcAddress(_glGetCompressedTexImageARB);
                RWASSERT( NULL != _rwOpenGLExt.GetCompressedTexImageARB );
            }
        }
    }

    if ( NULL != _rwOpenGLExt.CompressedTexImage1DARB )
    {
        /* S3TC texture compression is built upon ARB texture compression */
        if ( FALSE != RwOpenGLIsExtensionSupported( _GL_EXT_texture_compression_s3tc ) )
        {
            if ( OFFEREXTENSION( _GL_EXT_texture_compression_s3tc ) )
            {
                _rwOpenGLExt.TextureCompressionS3TCEXT = TRUE;
            }
        }
    }

    /* OpenGL 1.2 and above required from hereon */
    if ( rwstrcmp( glGetString( GL_VERSION ), RWSTRING("1.2") ) < 0 )
    {
        RWRETURN( TRUE );
    }

    if ( FALSE != RwOpenGLIsExtensionSupported( _GL_ARB_multitexture ) )
    {
        if ( OFFEREXTENSION( _GL_ARB_multitexture ) )
        {
            _rwOpenGLExt.ActiveTextureARB =
                (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress(_glActiveTextureARB);
            RWASSERT( NULL != _rwOpenGLExt.ActiveTextureARB );

            _rwOpenGLExt.ClientActiveTextureARB =
                (PFNGLCLIENTACTIVETEXTUREARBPROC)wglGetProcAddress(_glClientActiveTextureARB);
            RWASSERT( NULL != _rwOpenGLExt.ClientActiveTextureARB );

            _rwOpenGLExt.MultiTexCoord2fvARB =
                (PFNGLMULTITEXCOORD2FVARBPROC)wglGetProcAddress(_glMultiTexCoord2fvARB);
            RWASSERT( NULL != _rwOpenGLExt.MultiTexCoord2fvARB );

            glGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB,
                           (GLint *)&(_rwOpenGLExt.MaxTextureUnits) );

            /* GL_ARB_texture_env_combine requires multitexturing */
            if ( FALSE != RwOpenGLIsExtensionSupported( _GL_ARB_texture_env_combine ) )
            {
                if ( OFFEREXTENSION( _GL_ARB_texture_env_combine ) )
                {
                    _rwOpenGLExt.TextureEnvCombineARB = TRUE;
                }
            }
        }
    }

    if ( FALSE != RwOpenGLIsExtensionSupported( _GL_EXT_texture_edge_clamp ) )
    {
        if ( OFFEREXTENSION( GL_EXT_texture_edge_clamp ) )
        {
            _rwOpenGLExt.TextureEdgeClampEXT = TRUE;
        }
    }

    RWRETURN( TRUE );
}

/****************************************************************************
 _rwOpenGLWinRasterContextCtor

 On entry   : Object, offset, size
 On exit    : Object pointer on success
 */

static void *
_rwOpenGLWinRasterContextCtor( void *object,
                               RwInt32 offsetInObject __RWUNUSED__,
                               RwInt32 sizeInObject __RWUNUSED__ )
{
    _rwOpenGLRasterExt  *rasterExt;

    RWFUNCTION( RWSTRING( "_rwOpenGLWinRasterContextCtor" ) );

    RWASSERT( NULL != object );

    rasterExt = RASTEREXTFROMRASTER(object);

    /* these are used to detect when we are using a camera for the first time */
    rasterExt->window  = NULL;
    rasterExt->context = NULL;

    RWRETURN( object );
}


/****************************************************************************
 _rwOpenGLWinRasterContextDtor

 On entry   : Object, offset, size
 On exit    : Object pointer on success
 */

static void *
_rwOpenGLWinRasterContextDtor( void *object,
                               RwInt32 offsetInObject __RWUNUSED__,
                               RwInt32 sizeInObject __RWUNUSED__ )
{
    _rwOpenGLRasterExt  *rasterExt;


    RWFUNCTION( RWSTRING( "_rwOpenGLWinRasterContextDtor" ) );

    RWASSERT( NULL != object );

    rasterExt = RASTEREXTFROMRASTER(object);

    if ( NULL != rasterExt->context )
    {
        ReleaseDC( rasterExt->window, rasterExt->context );
        rasterExt->context = NULL;
    }

    RWRETURN( object );
}


/****************************************************************************
 _rwOpenGLCameraAttachWindow

 On entry   : camera and window handle
 On exit    : TRUE on success
 */
RwBool
_rwOpenGLCameraAttachWindow(void *camera, void *window)
{
    RwBool                  rv = FALSE;
    _rwOpenGLRasterExt          *rasterExt;
    int                     pf;
    RECT                    rect;
    RwRaster                *raster;
    RwRaster                *zRaster;
    HDC                     deviceContext;
    RwCamera                *rwCamera;

    RWFUNCTION(RWSTRING("_rwOpenGLCameraAttachWindow"));
    RWASSERT(camera);
    RWASSERT(window);

    if ( FALSE == VideoModes[CurrentVideoMode].fullScreen )
    {
        if ( Window != window )
        {
            /*
             *  Set the correct Pixel format of the new window
             */
            deviceContext = GetDC(window);

            pf = ChoosePixelFormat(deviceContext, &_rwOpenGLPixelFormatDesc);
            if (pf == 0)
            {
                ReleaseDC(window, deviceContext);
                RWRETURN( rv );
            }

            if (SetPixelFormat(deviceContext, pf, &_rwOpenGLPixelFormatDesc) ==
                FALSE)
            {
                ReleaseDC(window, deviceContext);
                RWRETURN( rv );
            }

            /* Ah, but what did OpenGL actually give us? */
            DescribePixelFormat(deviceContext, pf,
                                sizeof(PIXELFORMATDESCRIPTOR),
                                &_rwOpenGLPixelFormatDesc);

            GetClientRect(window, &rect);


            rwCamera = (RwCamera *)camera;
            /*
             *  Create the rasters for the new window
             */
            raster = RwCameraGetRaster(rwCamera);
            if ( NULL != raster )
            {
                RwRasterDestroy(raster);
            }

            raster = RwRasterCreate(rect.right, rect.bottom, 0, rwRASTERTYPECAMERA);
            if ( NULL != raster )
            {
                RwCameraSetRaster(rwCamera, raster);
            }

            zRaster = RwCameraGetZRaster(rwCamera);
            if ( NULL != zRaster )
            {
                RwRasterDestroy(zRaster);
            }

            zRaster = RwRasterCreate(rect.right, rect.bottom, 0, rwRASTERTYPEZBUFFER);
            if ( NULL != zRaster )
            {
                RwCameraSetZRaster(rwCamera, zRaster);
            }

            rasterExt = RASTEREXTFROMRASTER(raster);
            if ( NULL != rasterExt )
            {
                rasterExt->window = window;
                rasterExt->context = deviceContext;

                rv = TRUE;
            }
        }
#if defined(RWDEBUG)
        else
        {
            RwDebugSendMessage( rwDEBUGMESSAGE, "_rwOpenGLCameraAttachWindow",
                                "This function should not be called for the main window!" );
        }
#endif
    }

    RWRETURN(rv);
}


/****************************************************************************
 _rwDeviceRegisterPlugin

 On entry   :
 On exit    : TRUE on success
 */
RwBool
_rwDeviceRegisterPlugin( void )
{
    RWFUNCTION( RWSTRING( "_rwDeviceRegisterPlugin" ) );

    /*
     * Gives the device a change to register it's plugins.
     */

    /* Reserve some memory inside RwRaster structures 
     * for storing data that the OpenGL library needs
     * to make a new texture current in the render state */
    _rwOpenGLRasterExtOffset =
                RwRasterRegisterPlugin( sizeof(_rwOpenGLRasterExt),
                                        rwID_DEVICEMODULE,
                                        _rwOpenGLWinRasterContextCtor,
                                        _rwOpenGLWinRasterContextDtor,
                                        NULL );

    if ( 0 > _rwOpenGLRasterExtOffset )
    {
        RWRETURN( FALSE );
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
_rwOpenGLSystem(RwInt32 request, void *pOut, void *pInOut, RwInt32 nIn)
{
    RWFUNCTION(RWSTRING("_rwOpenGLSystem"));

    switch (request)
    {

        /*********************** Video mode functions ***************************/

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
                RwInt32            *npOut = (RwInt32 *) pOut;

                (*npOut) = NumVideoModes;

                RWRETURN(TRUE);
            }

        case rwDEVICESYSTEMGETMODEINFO:
            {
                RwVideoMode        *mode = (RwVideoMode *) pOut;

                if ((nIn >= 0) && (nIn < NumVideoModes))
                {
                    mode->width  = VideoModes[nIn].vidMode.dmPelsWidth;
                    mode->height = VideoModes[nIn].vidMode.dmPelsHeight;
                    mode->depth  = VideoModes[nIn].vidMode.dmBitsPerPel;
                    mode->flags  = VideoModes[nIn].fullScreen ?
                                                rwVIDEOMODEEXCLUSIVE : 0;

                    RWRETURN(TRUE);
                }

                /* It's a bad time */
                RWRETURN(FALSE);
            }

        case rwDEVICESYSTEMGETMODE:
            {
                RwInt32 *npOut = (RwInt32 *) pOut;

                (*npOut) = CurrentVideoMode;

                RWRETURN(TRUE);
            }

        case rwDEVICESYSTEMFOCUS:
            {
                RWRETURN(TRUE);
            }

        /******************* Initialising **********************/

            /* Gives the device access to the libraries device block */

        case rwDEVICESYSTEMREGISTER:
            {
                RwDevice           *device = (RwDevice *) pOut;
                RwMemoryFunctions  *memFuncs = (RwMemoryFunctions *) pInOut;

                /* Set up the fast device access.. */
                (*device) = (*_rwDeviceGetHandle());

                _rwOpenGLIm2DOpen(device);

                dgGGlobals.memFuncs = memFuncs;

                RWRETURN(TRUE);
            }

        /******************* Opening/Closing **********************/

            /* The purpose of Open is to put the hardware in a state where
             * it can be queried (about its capabilities). This function also
             * tests if it is a candidate at all (like it can be started at
             * all). NOTE this includes setting up a minimal set of standard
             * handlers that can be used for querying the device. */

        case rwDEVICESYSTEMOPEN:
            {
                Window = (HWND) (((RwEngineOpenParams *) pInOut)->displayID);


                /* get the window rect */
                GetWindowRect( Window, &cachedWinRect );

                if (!OpenGLEnumerateVideoModes())
                {
                    RWRETURN(FALSE);
                }

                /* assume the first video mode is selected */
                CurrentVideoMode = 0;

                /* Open the OpenGL module */
                if (!_rwOpenGLOpen())
                {
                    RWRETURN(FALSE);
                }

#if defined(RWDEBUG)
                if (RWSRCGLOBAL(debugFunction))
                {
                    RwChar buffer[256];
                    RwInt32 len;
                    MEMORYSTATUS memstats;

                    /* OS info */
                    len = rwsprintf(buffer, "\nOS Info: ");
                    _rwCPUGetOSName(buffer + len);
                    RWSRCGLOBAL(debugFunction) (rwDEBUGMESSAGE, buffer);

                    RWSRCGLOBAL(debugFunction) (rwDEBUGMESSAGE,
                        "------------------------------------------------------------------------------");

                    GlobalMemoryStatus(&memstats);
                    len = (memstats.dwTotalPhys / (1024 * 1024));
                    len = ((len + 1) & 0xfffffffe); /* Round up to a multiple of 2 */
                    rwsprintf(buffer, "   RAM:    %d MB", len);
                    RWSRCGLOBAL(debugFunction) (rwDEBUGMESSAGE, buffer);

                    /* CPU info */
                    len = rwsprintf(buffer, "\nCPU Info: ");
                    _rwCPUGetCPUName(buffer + len);
                    RWSRCGLOBAL(debugFunction) (rwDEBUGMESSAGE, buffer);

                    RWSRCGLOBAL(debugFunction) (rwDEBUGMESSAGE,
                              "------------------------------------------------------------------------------");

                    rwsprintf(buffer, "   MMX:    %s", (_rwCPUHaveMMX() ? "Available" : "Not available"));
                    RWSRCGLOBAL(debugFunction) (rwDEBUGMESSAGE, buffer);

                    rwsprintf(buffer, "   SSE:    %s", (_rwCPUHaveSSE() ? "Available" : "Not available"));
                    RWSRCGLOBAL(debugFunction) (rwDEBUGMESSAGE, buffer);

                    rwsprintf(buffer, "   SSE2:   %s", (_rwCPUHaveSSE2() ? "Available" : "Not available"));
                    RWSRCGLOBAL(debugFunction) (rwDEBUGMESSAGE, buffer);

                    rwsprintf(buffer, "   3DNow!: %s\n", (_rwCPUHave3DNow() ? "Available" : "Not available"));
                    RWSRCGLOBAL(debugFunction) (rwDEBUGMESSAGE, buffer);
                }
#endif /* defined(RWDEBUG) */

                RWRETURN(TRUE);
            }

            /* The purpose of Close is to remove any set up that was performed
             * by Open */

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
             * rendering - so get it into a state ready for rendering */

        case rwDEVICESYSTEMSTART:
            {
                int                 pf;

                if (VideoModes[CurrentVideoMode].fullScreen)
                {
                    /* test first, then change, just in case. */
                    if (ChangeDisplaySettings
                        (&VideoModes[CurrentVideoMode].vidMode,
                         CDS_TEST) == DISP_CHANGE_SUCCESSFUL)
                    {
                        ChangeDisplaySettings(&VideoModes
                                              [CurrentVideoMode].vidMode,
                                              CDS_FULLSCREEN);

                        cachedWinStyle = GetWindowLong( Window, GWL_STYLE );

                        /* I'm not sure if this is dangerous or not but it's necessary to get rid of the window frame */
                        SetWindowLong(Window, GWL_STYLE,
                                      WS_POPUP | WS_MAXIMIZE | WS_CLIPSIBLINGS
                                      | WS_CLIPCHILDREN);

                        SetWindowPos(Window, HWND_TOPMOST,
                                     0, 0,
                                     VideoModes[CurrentVideoMode].
                                     vidMode.dmPelsWidth,
                                     VideoModes[CurrentVideoMode].
                                     vidMode.dmPelsHeight, 0);
                    }
                    else
                    {
                        /* Something went wrong, the test failed! */
                        RWRETURN(FALSE);
                    }
                }

                /* select the specified colour depth */
                _rwOpenGLPixelFormatDesc.cColorBits = (RwUInt8)VideoModes[CurrentVideoMode].vidMode.dmBitsPerPel;

                DeviceContext = GetDC(Window);

                pf = ChoosePixelFormat(DeviceContext, &_rwOpenGLPixelFormatDesc);
                if (pf == 0)
                {
                    ReleaseDC(Window, DeviceContext);
                    RWRETURN(FALSE);
                }

                if (SetPixelFormat(DeviceContext, pf, &_rwOpenGLPixelFormatDesc) ==
                    FALSE)
                {
                    ReleaseDC(Window, DeviceContext);
                    RWRETURN(FALSE);
                }

                /* Ah, but what did OpenGL actually give us? */
                DescribePixelFormat(DeviceContext, pf,
                                    sizeof(PIXELFORMATDESCRIPTOR),
                                    &_rwOpenGLPixelFormatDesc);

                RenderContext = wglCreateContext(DeviceContext);

                /* Set up the display */
                wglMakeCurrent(DeviceContext, RenderContext);

#if defined( RWDEBUG )
                if ( NULL != RWSRCGLOBAL( debugFunction ) )
                {
                    RwChar  buffer[256];

                    RwInt32 genericFormat = _rwOpenGLPixelFormatDesc.dwFlags & PFD_GENERIC_FORMAT;

                    RwInt32 genericAccelerated = _rwOpenGLPixelFormatDesc.dwFlags & PFD_GENERIC_ACCELERATED;


                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, "OpenGL driver information" );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, "-------------------------" );

                    if ( (PFD_GENERIC_FORMAT == genericFormat) && (0 == genericAccelerated) )
                    {
                        rwsprintf( buffer, "GL_VENDOR:\t\t\t\t%s (Software)", (LPCSTR)glGetString( GL_VENDOR ) );
                        RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );
                    }
                    else if ( (PFD_GENERIC_FORMAT == genericFormat) && (PFD_GENERIC_ACCELERATED == genericAccelerated) )
                    {
                        rwsprintf( buffer, "GL_VENDOR:\t\t\t\t%s (Hardware MCD)", (LPCSTR)glGetString( GL_VENDOR ) );
                        RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );
                        OutputDebugString( "Hardware MCD" );
                    }
                    else if ( (0 == genericFormat) && (0 == genericAccelerated) )
                    {
                        rwsprintf( buffer, "GL_VENDOR:\t\t\t\t%s (Hardware ICD)", (LPCSTR)glGetString( GL_VENDOR ) );
                        RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );
                    }

                    rwsprintf( buffer, "GL_RENDERER:\t\t\t%s", (LPCSTR)glGetString( GL_RENDERER ) );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                    rwsprintf( buffer, "GL_VERSION:\t\t\t\t%s", (LPCSTR)glGetString( GL_VERSION ) );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                    rwsprintf( buffer, "GL_EXTENSIONS:" );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );
                    _rwOpenGLEnumExtensions( _rwOpenGLDebugOutputExtensionsCB, NULL, NULL );

                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, "\n" );

                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, "Pixel format information" );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, "------------------------" );

                    /* pixel format index */
                    rwsprintf( buffer, "Index: %d", pf );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                    /* flags */
                    rwsprintf( buffer, "Flags:" );
                    if ( _rwOpenGLPixelFormatDesc.dwFlags & PFD_DRAW_TO_WINDOW )
                    {
                        rwstrcat(buffer," PFD_DRAW_TO_WINDOW");
                    }
                    if ( _rwOpenGLPixelFormatDesc.dwFlags & PFD_DRAW_TO_BITMAP )
                    {
                        rwstrcat(buffer," PFD_DRAW_TO_BITMAP");
                    }
                    if ( _rwOpenGLPixelFormatDesc.dwFlags & PFD_SWAP_COPY )
                    {
                        rwstrcat(buffer," PFD_SWAP_COPY");
                    }
                    if ( _rwOpenGLPixelFormatDesc.dwFlags & PFD_SWAP_EXCHANGE )
                    {
                        rwstrcat(buffer," PFD_SWAP_EXCHANGE");
                    }
                    if ( _rwOpenGLPixelFormatDesc.dwFlags & PFD_SUPPORT_OPENGL )
                    {
                        rwstrcat(buffer," PFD_SUPPORT_OPENGL");
                    }
                    if ( _rwOpenGLPixelFormatDesc.dwFlags & PFD_SUPPORT_GDI )
                    {
                        rwstrcat(buffer," PFD_SUPPORT_GDI");
                    }
                    if ( _rwOpenGLPixelFormatDesc.dwFlags & PFD_DOUBLEBUFFER )
                    {
                        rwstrcat(buffer," PFD_DOUBLEBUFFER");
                    }
                    if ( _rwOpenGLPixelFormatDesc.dwFlags & PFD_STEREO )
                    {
                        rwstrcat(buffer," PFD_STEREO");
                    }
                    if ( _rwOpenGLPixelFormatDesc.dwFlags & PFD_GENERIC_FORMAT )
                    {
                        rwstrcat(buffer," PFD_GENERIC_FORMAT");
                    }
                    if ( _rwOpenGLPixelFormatDesc.dwFlags & PFD_NEED_PALETTE )
                    {
                        rwstrcat(buffer," PFD_NEED_PALETTE");
                    }
                    if ( _rwOpenGLPixelFormatDesc.dwFlags & PFD_NEED_SYSTEM_PALETTE )
                    {
                        rwstrcat(buffer," PFD_NEED_SYSTEM_PALETTE");
                    }
                    if ( _rwOpenGLPixelFormatDesc.dwFlags & PFD_GENERIC_ACCELERATED )
                    {
                        rwstrcat(buffer," PFD_GENERIC_ACCELERATED");
                    }
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                    /* color buffer */
                    rwsprintf( buffer, "Color depth\t\t\t\t\t\t%d bits", _rwOpenGLPixelFormatDesc.cColorBits );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                    rwsprintf( buffer, "\t\t\t\t\t\t\t\tBit size\tShift offset" );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                    rwsprintf( buffer, "\tRed component\t:\t\t\t%d\t\t\t%d",
                                       _rwOpenGLPixelFormatDesc.cRedBits,
                                       _rwOpenGLPixelFormatDesc.cRedShift );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                    rwsprintf( buffer, "\tGreen component\t:\t\t\t%d\t\t\t%d",
                                       _rwOpenGLPixelFormatDesc.cGreenBits,
                                       _rwOpenGLPixelFormatDesc.cGreenShift );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                    rwsprintf( buffer, "\tBlue component\t:\t\t\t%d\t\t\t%d",
                                       _rwOpenGLPixelFormatDesc.cBlueBits,
                                       _rwOpenGLPixelFormatDesc.cBlueShift );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                    rwsprintf( buffer, "\tAlpha component\t:\t\t\t%d\t\t\t%d",
                                       _rwOpenGLPixelFormatDesc.cAlphaBits,
                                       _rwOpenGLPixelFormatDesc.cAlphaShift );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                    /* accumulation buffer */
                    rwsprintf( buffer, "Accumulation buffer depth\t\t%d bits", _rwOpenGLPixelFormatDesc.cAccumBits );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                    rwsprintf( buffer, "\t\t\t\t\t\t\t\tBit size" );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                    rwsprintf( buffer, "\tRed component\t:\t\t\t%d",
                                       _rwOpenGLPixelFormatDesc.cAccumRedBits );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                    rwsprintf( buffer, "\tGreen component\t:\t\t\t%d",
                                       _rwOpenGLPixelFormatDesc.cAccumGreenBits );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                    rwsprintf( buffer, "\tBlue component\t:\t\t\t%d",
                                       _rwOpenGLPixelFormatDesc.cAccumBlueBits );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                    rwsprintf( buffer, "\tAlpha component\t:\t\t\t%d",
                                       _rwOpenGLPixelFormatDesc.cAccumAlphaBits );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                    /* z-buffer */
                    rwsprintf( buffer, "Z-buffer depth\t\t\t\t\t%d bits", _rwOpenGLPixelFormatDesc.cDepthBits );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                    /* stencil buffer */
                    rwsprintf( buffer, "Stencil buffer depth\t\t\t%d bits", _rwOpenGLPixelFormatDesc.cStencilBits );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                    /* auxiliary buffers */
                    rwsprintf( buffer, "Number of auxiliary buffers\t\t%d", _rwOpenGLPixelFormatDesc.cAuxBuffers );
                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, buffer );

                    RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, "\n" );
                }
#endif /* defined( RWDEBUG ) */

                /* load OpenGL extensions */
                if ( FALSE == OpenGLLoadExtensions() )
                {
                    RWRETURN( FALSE );
                }

                /* open the raster module */
                if ( FALSE == _rwOpenGLRasterOpen() )
                {
                    RWRETURN( FALSE );
                }

                /* open the vertex array module */
                if ( FALSE == _rwOpenGLVAOpen() )
                {
                    RWRETURN( FALSE );
                }

                /* Set up the rendering state */
                _rwOpenGLRenderStateOpen();

                /* we've started */
                engineStarted = TRUE;

                /* All done */
                RWRETURN(TRUE);
            }

            /* Puts back the hardware into the state it was in immediately
             * after being opened */


        case rwDEVICESYSTEMSTOP:
            {
                RWASSERT( FALSE != engineStarted );

                /* close the renderstate module */
                _rwOpenGLRenderStateClose();

                /* close the vertex array module */
                _rwOpenGLVAClose();

                /* close the raster module */
                _rwOpenGLRasterClose();

#if !defined( NOASM )

                /* Restore the saved floating point control register */
                _rwProcessorRelease();

                /* Disable SSE flush-to-zero */
#if !defined( NOSSEASM )
#if (!defined(__GNUC__))
                if ( TRUE == SSEsupported )
                {
                    RwUInt32 temp;


                    __asm
                    {
                        stmxcsr     temp
                        mov         eax, temp
                        and         eax, not 0x8000
                        mov         temp, eax
                        ldmxcsr     temp
                    }
                }
#endif /* (!defined(__GNUC__)) */
#endif /*!defined( NOSSEASM )*/

#endif /* !defined( NOASM ) */

                _rwOpenGLVertexHeapDestroy();

                /* destroy the extensions string */
                _rwOpenGLDestroyExtensionsString();

                /* De-reference the current rendering context */
                wglMakeCurrent(NULL, NULL);

                /* Delete the rendering context associated with this engine instance */
                wglDeleteContext(RenderContext);

                /* Release the device context used with this rendering context */
                ReleaseDC(Window, DeviceContext);

                if (VideoModes[CurrentVideoMode].fullScreen)
                {
                    /* Change back to the default display settings as stored in the system registry */
                    ChangeDisplaySettings(NULL, 0);
                }

                engineStarted = FALSE;

                /* All done */
                RWRETURN(TRUE);
            }

        case rwDEVICESYSTEMFINALIZESTART:
            {
                SSEsupported = _rwCPUHaveSSE();

#if !defined( NOASM )

                /* save a copy of the FP control register and force to single precision */
                _rwProcessorInitialize();

#if !defined( NOSSEASM )
#if (!defined(__GNUC__))
                /* enable SSE flush-to-zero */
                if ( TRUE == SSEsupported )
                {
                    RwUInt32 temp;


                    __asm
                    {
                        stmxcsr     temp
                        mov         eax, temp
                        or          eax, 0x8000
                        mov         temp, eax
                        ldmxcsr     temp
                    }
                }
#endif /* (!defined(__GNUC__)) */
#endif /* !defined( NOSSEASM ) */

                /* set the matrix multiplication routine to the new assembler code
                 * this method allows future overloading */
                _rwMatrixSetMultFn( _rwIntelx86AsmMatrixMultiply );

                /* set the matrix-vector products to the new assembler code */
                _rwVectorSetMultFn(_rwIntelx86AsmV3dTransformPoint,
                                   _rwIntelx86AsmV3dTransformPoints,
                                   _rwIntelx86AsmV3dTransformVector,
                                   _rwIntelx86AsmV3dTransformVectors);

#endif /* !defined( NOASM ) */
            }
            break;


        case rwDEVICESYSTEMGETMAXTEXTURESIZE:
            {
                *(RwInt32*)pOut = _rwOpenGLGetMaxSupportedTextureSize();
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
_rwOpenGLRasterShowRaster(void *raster,
                          void *dev __RWUNUSED__,
                          RwInt32 in __RWUNUSED__)
{
    RwRaster        *topRaster;
    _rwOpenGLRasterExt  *topRasterExt;

    RwBool          success;

    RWFUNCTION(RWSTRING("_rwOpenGLRasterShowRaster"));

    /*
     * Got to get the top level raster as this is the only one with a
     * texture surface
     */
    topRaster = RwRasterGetParent((RwRaster *)raster);

    topRasterExt = RASTEREXTFROMRASTER(topRaster);

    /*
     *  Check if the current camera has its own device context
     */
    if (topRasterExt->context)
    {
        success = SwapBuffers(topRasterExt->context);
    }
    else
    {
        success = SwapBuffers(DeviceContext);
    }

    /* if using the NVIDIA vertex array range extension, check whether
     * a resize is required, and perform it when necessary */
    if ( FALSE != _rwOpenGLVertexHeapAvailable() )
    {
        _rwOpenGLVertexHeapTestAndPerformResize();
    }

    RWRETURN( success );
}

/****************************************************************************
 _rwOpenGLMakeCurrent

 On entry   : void
 On exit    : TRUE on success
 */

RwBool
_rwOpenGLMakeCurrent()
{
    HDC                     currDC = wglGetCurrentDC();
    HGLRC                   currRC = wglGetCurrentContext();
    _rwOpenGLRasterExt          *topRasterExt;

    RWFUNCTION(RWSTRING("_rwOpenGLMakeCurrent"));

    if(dgGGlobals.curCamera)
    {
        RwRaster *raster;
        RwRaster *topRaster;

        raster = RwCameraGetRaster(dgGGlobals.curCamera);

        /*
        * Got to get the top level raster as this is the only one with a
        * texture surface
        */
        topRaster = RwRasterGetParent(raster);

        topRasterExt = RASTEREXTFROMRASTER(topRaster);
    }
    else
    {
        topRasterExt = NULL;
    }

    /*
     *  Check if the current camera has its own device context
     */
    if (topRasterExt && topRasterExt->context)
    {
        if ((topRasterExt->context == currDC) && (RenderContext == currRC))
        {
            RWRETURN(TRUE);
        }

        RWRETURN(wglMakeCurrent(topRasterExt->context, RenderContext));
    }
    else
    {
        if ((DeviceContext == currDC) && (RenderContext == currRC))
        {
            RWRETURN(TRUE);
        }

        RWRETURN(wglMakeCurrent(DeviceContext, RenderContext));
    }
}

/****************************************************************************
 _rwOpenGLGetEngineWindowHeight

 On entry   : void
 On exit    : TRUE on success
 */

RwInt32
_rwOpenGLGetEngineWindowHeight()
{
    RECT                    rect;
    _rwOpenGLRasterExt          *rasterExt;

    RWFUNCTION(RWSTRING("_rwOpenGLGetEngineWindowHeight"));

    if(dgGGlobals.curCamera)
    {
        RwRaster    *parent;


        parent = RwRasterGetParent( RwCameraGetRaster(dgGGlobals.curCamera) );

        rasterExt = RASTEREXTFROMRASTER(parent);
    }
    else
    {
        rasterExt = NULL;
    }

    /*
     *  Check if the current camera has its own window
     */
    if (rasterExt && rasterExt->context)
    {
        GetClientRect(rasterExt->window, &rect);
    }
    else
    {
        GetClientRect(Window, &rect);
    }

    RWRETURN(rect.bottom);
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLGetEngineColorDepth
 *
 *  Purpose : Get the engine's current color depth.
 *
 *  On entry: Nothing.
 *
 *  On exit : RwInt32 containing the number of bits colour depth.
 * ------------------------------------------------------------------------- */
RwInt32
_rwOpenGLGetEngineColorDepth( void )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLGetEngineColorDepth" ) );

    RWRETURN( ((RwInt32)_rwOpenGLPixelFormatDesc.cColorBits) );
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

    RWRETURN(((RwInt32) _rwOpenGLPixelFormatDesc.cDepthBits));
}

/****************************************************************************
 _rwOpenGLSelectStencilBufferDepth

 On entry   : void
 On exit    : TRUE on success
 */

RwBool
_rwOpenGLSelectStencilBufferDepth(RwUInt8 depth)
{
    RWFUNCTION(RWSTRING("_rwOpenGLSelectStencilBufferDepth"));

    if (RWSRCGLOBAL(engineStatus) != rwENGINESTATUSSTARTED)
    {
        _rwOpenGLPixelFormatDesc.cStencilBits = depth;
        RWRETURN(TRUE);
    }
    RWRETURN(FALSE);
}

/****************************************************************************
 _rwOpenGLGetStencilBufferDepth

 On entry   : void
 On exit    : TRUE on success
 */

RwBool
_rwOpenGLGetStencilBufferDepth(RwUInt8 *depth)
{
    RWFUNCTION(RWSTRING("_rwOpenGLGetStencilBufferDepth"));
    RWASSERT(depth);

    if (RWSRCGLOBAL(engineStatus) == rwENGINESTATUSSTARTED)
    {
        *depth = _rwOpenGLPixelFormatDesc.cStencilBits;
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
    GLenum              errorFlag = glGetError();

    RWFUNCTION(RWSTRING("_rwOpenGLErrorCheck"));

    if (errorFlag != GL_NO_ERROR)
    {
        RwChar  buffer[256];


        IssueDebugString("Warning: OpenGL error flag set (");

        switch (errorFlag)
        {
            case GL_INVALID_ENUM:
                IssueDebugString("invalid enumerant");
                break;
            case GL_INVALID_VALUE:
                IssueDebugString("invalid value");
                break;
            case GL_INVALID_OPERATION:
                IssueDebugString("invalid operation");
                break;
            case GL_STACK_OVERFLOW:
                IssueDebugString("stack overflow");
                break;
            case GL_STACK_UNDERFLOW:
                IssueDebugString("stack underflow");
                break;
            case GL_OUT_OF_MEMORY:
                IssueDebugString("out of memory");
                break;
            default:
                IssueDebugString("unknown error");
                break;
        }

        rwsprintf( buffer, ") from file %s before line %d\n", filename, lineNumber );
        IssueDebugString( buffer );
    }

    RWRETURNVOID();
}

#endif

/**
 * _rwIntelSSEsupported
 *
 * return TRUE if the CPU supports SSE instructions
 */
RwBool
_rwIntelSSEsupported( void )
{
    RWFUNCTION( RWSTRING( "_rwIntelSSEsupported" ) );

    RWRETURN( SSEsupported );
}


/**
 * \ingroup rwenginevideomodeopengl
 * \ref RwOpenGLChangeVideoMode is used to force an immediate change of video
 * mode.
 *
 * This function should be called before any graphics are rendered or
 * immediately after \ref RwRasterShowRaster or \ref RwCameraShowRaster and
 * before the next frame is submitted.
 *
 * Supported mode changes are
 * \li fullscreen to fullscreen
 * \li fullscreen to windowed
 * \li windowed to fullscreen
 *
 * Changes to display resolution or color depth while remaining in windowed
 * mode should be performed through the desktop interface.
 *
 * \note This function can not be called before \ref RwEngineStart.
 *
 * \note This function is supported under Win32 OpenGL only.
 *
 * \param modeIndex An \ref RwInt32 value equal to the video mode's index,
 *                  as provided by RenderWare Graphic's video mode enumeration
 *                  functions.
 *
 * \return Returns TRUE if successful, FALSE otherwise.
 *
 * \see RwEngineGetNumVideoModes
 * \see RwEngineGetCurrentVideoMode
 * \see RwEngineGetVideoModeInfo
 */
RwBool
RwOpenGLChangeVideoMode( RwInt32 modeIndex )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLChangeVideoMode" ) );

    /* RwEngineStart must have been called already */
    RWASSERT( FALSE != engineStarted );
    if ( FALSE == engineStarted )
    {
        RWRETURN( FALSE );
    }

    /* has the video mode array been set up yet? */
    if ( NULL == VideoModes )
    {
        RWRETURN( FALSE );
    }

    /* is it a valid mode index? */
    if ( (modeIndex < 0) ||
         (modeIndex >= NumVideoModes) )
    {
        RWRETURN( FALSE );
    }

    /* already in this mode? */
    if ( modeIndex == CurrentVideoMode )
    {
        RWRETURN( TRUE );
    }

    if ( FALSE != VideoModes[CurrentVideoMode].fullScreen )
    {
        if ( FALSE != VideoModes[modeIndex].fullScreen )
        {
            /* Fullscreen to fullscreen */

            /* test first, then change, just in case. */
            if ( DISP_CHANGE_SUCCESSFUL ==
                    ChangeDisplaySettings( &VideoModes[modeIndex].vidMode,
                                           CDS_TEST ) )
            {
                CurrentVideoMode = modeIndex;

                ChangeDisplaySettings( &VideoModes[CurrentVideoMode].vidMode,
                                       CDS_FULLSCREEN);

                SetWindowPos( Window,
                              NULL,
                              0, 0,
                              VideoModes[CurrentVideoMode].vidMode.dmPelsWidth,
                              VideoModes[CurrentVideoMode].vidMode.dmPelsHeight,
                              SWP_NOMOVE |
                              SWP_NOOWNERZORDER |
                              SWP_NOZORDER |
                              SWP_SHOWWINDOW );

                wglMakeCurrent(DeviceContext, RenderContext);
                GL_ERROR_CHECK();

                RWRETURN( TRUE );
            }
        }
        else
        {
            /* Fullscreen to windowed */

            /* this is the only windowed mode - period */
            RWASSERT( 0 == modeIndex );

            /* test first, then change, just in case. */
            if ( DISP_CHANGE_SUCCESSFUL ==
                    ChangeDisplaySettings( NULL, CDS_TEST ) )
            {
                CurrentVideoMode = modeIndex;

                ChangeDisplaySettings( NULL, CDS_FULLSCREEN );

                SetWindowLong( Window,
                               GWL_STYLE,
                               cachedWinStyle | WS_OVERLAPPEDWINDOW );

                SetWindowPos( Window,
                              NULL,
                              cachedWinRect.left, cachedWinRect.top,
                              cachedWinRect.right - cachedWinRect.left,
                              cachedWinRect.bottom - cachedWinRect.top,
                              SWP_FRAMECHANGED |
                              SWP_NOOWNERZORDER |
                              SWP_NOZORDER |
                              SWP_SHOWWINDOW );

                wglMakeCurrent(DeviceContext, RenderContext);
                GL_ERROR_CHECK();

                RWRETURN( TRUE );
            }
        }
    }
    else
    {
        if ( FALSE == VideoModes[modeIndex].fullScreen )
        {
            /* Windowed to windowed - use Explorer */
            RWRETURN( FALSE );
        }
        else
        {
            /* Windowed to Fullscreen */

            /* test first, then change, just in case. */
            if ( DISP_CHANGE_SUCCESSFUL ==
                    ChangeDisplaySettings( &VideoModes[modeIndex].vidMode,
                                           CDS_TEST ) )
            {
                /* get the window rect */
                GetWindowRect( Window, &cachedWinRect );

                CurrentVideoMode = modeIndex;

                ChangeDisplaySettings( &VideoModes[CurrentVideoMode].vidMode,
                                       CDS_FULLSCREEN);

                SetWindowLong( Window,
                               GWL_STYLE,
                               WS_POPUP |
                               WS_MAXIMIZE |
                               WS_CLIPSIBLINGS |
                               WS_CLIPCHILDREN );

                SetWindowPos( Window,
                              HWND_TOPMOST,
                              0, 0,
                              VideoModes[CurrentVideoMode].vidMode.dmPelsWidth,
                              VideoModes[CurrentVideoMode].vidMode.dmPelsHeight,
                              SWP_FRAMECHANGED |
                              SWP_SHOWWINDOW );

                wglMakeCurrent(DeviceContext, RenderContext);
                GL_ERROR_CHECK();

                RWRETURN( TRUE );
            }
        }
    }

    RWRETURN( FALSE );
}

