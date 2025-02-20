/***************************************************************************
 *                                                                         *
 * Module  : baintogl.c                                                    *
 *                                                                         *
 * Purpose : OpenGL device handling                                        *
 *                                                                         *
 **************************************************************************/

/****************************************************************************
 Includes
 */

#if defined(WIN32)
#include <windows.h>
#endif /* defined(WIN32) */
#include <GL/GL.h>

#include "barwtyp.h"
#include "baimage.h"
#include "batextur.h"
#include "bavector.h"

#include "baogl.h"
#include "bastdogl.h"

#include "baintogl.h"
#include "barstate.h"
#include "barastgl.h"

#if ( !defined(NOASM) && defined(WIN32) )
#include "baprocfp.h"
#endif /* ( !defined(NOASM) && defined(WIN32) ) */


/**
 * \ingroup rwengineopengl
 * \page rwengineopengloverview Overview
 *
 * This section of the API reference contains OpenGL specific documentation
 * regarding how the RenderWare Graphics RwEngine behaves and is extended
 * under OpenGL.
 *
 * \li \ref rwenginevideomodeopengl
 * \li \ref rwenginevertexarraysopengl
 * \li \ref rwenginemultitextureopengl
 *
 */

/**
 * \ingroup rwrasteropengl
 * \page rasterdimensions Raster Dimensions
 *
 * This section describes specific information for the constraints of raster dimensions
 * using RenderWare Graphics OpenGL.
 *
 * Rasters of the type \ref rwRASTERTYPENORMAL, \ref rwRASTERTYPETEXTURE and \ref
 * rwRASTERTYPECAMERATEXTURE map to the concept of an OpenGL texture object. Hence
 * the dimensions of such rasters are constrained by the limits set by OpenGL; \e power \e of
 * \e two \e sides.
 *
 * Using \ref RwImageFindRasterFormat for any loaded image will determine whether
 * a raster can be created directly from the image, or if the image needs to be
 * resampled, according to platform specific raster requirements.
 *
 * \see \ref RwImageFindRasterFormat
 *
 */

/**
 * \ingroup rwrasteropengl
 * \page offscreenrendering Offscreen rendering
 *
 * This section describes specific information for offscreen rendering using
 * RenderWare Graphics OpenGL.
 *
 * Cameras used for offscreen rendering must have a raster of type \ref rwRASTERTYPECAMERATEXTURE
 * attached to them. On some platforms, these rasters exist separately from the main rendering
 * surface, but under OpenGL they share the same video memory. Hence, under OpenGL, calling
 * \ref RwCameraShowRaster on a camera with an \ref rwRASTERTYPECAMERATEXTURE raster \e may
 * display the offscreen camera's raster (depending on whether any rendering has occurred
 * subsequent to the offscreen camera).
 *
 */

/**
 * \ingroup rwrasteropengl
 * \page rasterformats Raster Formats
 *
 * This section describes specific information for specifying raster formats
 * using RenderWare Graphics OpenGL.
 *
 * All pixel formats, as specified in \ref RwRasterFormat, are supported, with the
 * exception of \ref rwRASTERFORMATLUM8 and both palettized formats. Specifying one of
 * the unsupported types to \ref RwRasterCreate will return a failure code.
 *
 * Specifying a raster format of 0 (zero) to \ref RwRasterCreate will allow
 * RenderWare Graphics OpenGL to select the most appropriate raster format for the
 * current rendering mode. For example, a 16bpp color buffer will only result in
 * 16bpp rasters (to reduce bandwidth and texture memory usage).
 *
 * \ref RwImageFindRasterFormat will also allow RenderWare Graphics OpenGL to select
 * the most appropriate raster format, given both the alpha coverage in the \ref RwImage
 * and the current rendering mode.
 *
 * It is recommended to allow RenderWare Graphics OpenGL to select raster formats for
 * best performance.
 *
 * \see \ref RwRasterCreate
 * \see \ref RwImageFindRasterFormat
 *
 */

/**
 * \ingroup rwrasteropengl
 * \page compressedrasters Compressed Rasters
 *
 * This section describes specific information for using compressed rasters 
 * in RenderWare Graphics OpenGL.
 *
 * Compressed textures are particularly useful in today's graphics applications as
 * they both reduce the amount of texture storage used and the bandwidth of uploading
 * textures to the video card. There are two ways of using compressed rasters in
 * RenderWare Graphics OpenGL; generic and \c DXTn compression. Both require the
 * \c GL_ARB_texture_compression extension to be supported, while \c DXTn formats
 * also require the \c GL_EXT_texture_compression_S3TC extension.
 *
 * Generic texture compressed formats are available on video cards supporting
 * the \c GL_ARB_texture_compression extension. These compressed formats are
 * implemented transparently by the video card driver. As the details of the compression
 * used are indeterminable, rasters compressed with a generic format will \e never
 * be streamed in compressed form.
 *
 * By default, generic texture compression formats are not selected. The API function
 * \ref RwOpenGLRasterSetGenericTextureCompression may be used to enable generic
 * texture compression, for all \ref rwRASTERTYPETEXTURE rasters created in the future,
 * until generic texture compression usage is disabled.
 *
 * \c DXTn compressed formats, where \c n can be 1, 3 or 5, are available when both
 * the \c GL_ARB_texture_compression and \c GL_EXT_texture_compression_S3TC extensions
 * are supported. \c .dds files, that usually contain \c DXTn textures, can be loaded
 * into an \ref RwRaster using the RenderWare Graphics OpenGL API function 
 * \ref RwOpenGLRasterStreamReadDDS from a pre-opened stream. Alternatively, 
 * \ref RwOpenGLDDSTextureRead may be called to generate an \ref RwTexture given a \c .dds
 * filename that is searched for on the current image path. Note that calling either of
 * these functions on a system that does not support texture compression will decompress
 * the \c DXTn image in software into an uncompressed raster or texture.
 *
 * Note that any compressed raster, whether the compression format be generic or \c DXTn,
 * \e cannot be locked, and may \e not be used as sources to \ref RwImage's through 
 * \ref RwImageSetFromRaster.
 *
 * However, \c DXTn rasters can be streamed into RenderWare Graphics OpenGL texture
 * dictionaries, which can considerably reduce the size of such dictionaries, and hence
 * their loading times.
 *
 * Should an OpenGL texture dictionary containing \c DXTn rasters be read on a system
 * that does not support texture compression, the texture dictionary will fail to
 * load. This is because texture dictionaries are optimized for per-system use. A new
 * texture dictionary specific to the current host should be generated for optimum
 * texture loading performance.
 *
 * \see \ref RwOpenGLRasterSetGenericTextureCompression
 * \see \ref RwOpenGLRasterGetGenericTextureCompression
 * \see \ref RwOpenGLRasterStreamReadDDS
 * \see \ref RwOpenGLDDSTextureRead
 * \see \ref RwImageSetPath
 * \see \ref RwImageSetFromRaster
 * \see \ref RwRasterLock
 * \see \ref cubemaps
 *
 */

/**
 * \ingroup rwrasteropengl
 * \page hwmipgen Hardware Mip Generation
 *
 * This section describes specific information for hardware mip generation
 * using RenderWare Graphics OpenGL.
 *
 * If the \c GL_SGIS_generate_mipmap extension is supported, mipmapped rasters created with the
 * \ref rwRASTERFORMATAUTOMIPMAP flag will have their non-toplevel mips generated
 * by the video hardware.
 *
 * RenderWare Graphics for OpenGL specifies \c GL_NICEST as the hint to the quality of
 * the generated mip levels.
 *
 * Note that locking a hardware generated mip raster at a mip level other than zero will
 * fail as those mip levels do not exist.
 *
 * Texture dictionaries also benefit from hardware generated mip rasters as only the top
 * level mip is stored, rather than all mip levels. Note that reading a texture dictionary
 * containing hardware generated mip levels on a system that does not support hardware
 * mip generation will cause the texture dictionary to fail to load. This is because
 * texture dictionaries are optimized for per-system use. A new texture dictionary specific
 * to the current host should be generated for optimum texture loading performance.
 *
 */

/**
 * \ingroup rwrasteropengl
 * \page cubemaps Cube Map Rasters
 *
 * This section describes specific information for cube map support in RenderWare Graphics
 * OpenGL.
 *
 * If the \c GL_ARB_texture_cube_map extension is supported, compressed cube map rasters may be
 * loaded via \c .dds files using \ref RwOpenGLRasterStreamReadDDS on a pre-opened stream, or
 * \ref RwOpenGLDDSTextureRead specifying a filename that is searched for on the current
 * image path. If any conditions for supporting a cubemap or loading from \c .dds files are not
 * met, then the cube map load will fail.
 *
 * A cube map raster can be identified using the function \ref RwOpenGLRasterIsCubeMap.
 *
 * If a texture containing a cube map raster is set as the environment map in \ref rpmatfx then
 * the cubemap will be bound and \c GL_REFLECTION_MAP_ARB texture coordinate generation
 * will be used for the environment map.
 *
 * Cube maps also support mipmapping, and may be streamed into a texture dictionary in
 * compressed form. This requires the host system that reads the texture dictionary to support
 * \c GL_ARB_texture_compression, \c GL_ARB_texture_cube_map and \c GL_EXT_texture_compression_s3tc
 * extensions. Should these conditions not be met, the texture dictionary will fail to load.
 *
 * Note that there are system specific limitations regarding the maximum size of cube maps.
 * RenderWare Graphics will fail to load a cubemap if it's size is greater than this maximum.
 * See \ref RwOpenGLExtensions.
 *
 * Please read the section \ref compressedrasters for more information on the use of compressed
 * rasters in RenderWare Graphics OpenGL.
 *
 * \see \ref compressedrasters
 * \see \ref RwOpenGLRasterStreamReadDDS
 * \see \ref RwOpenGLDDSTextureRead
 * \see \ref RwOpenGLRasterIsCubeMap
 * \see \ref RwImageSetPath
 * \see \ref RpMatFXMaterialSetEnvMapTexture
 * \see \ref RwOpenGLExtensions
 */

/**
 * \ingroup rwrasteropengl
 * \page raster2dfunctions Raster 2D Functions
 *
 * This section describes OpenGL specific information for using the RenderWare Graphics
 * 2D raster functions.
 *
 * If the destination raster is of type \ref rwRASTERTYPECAMERA then supported
 * source types for blitting from are:
 *
 * \li \ref rwRASTERTYPECAMERA - with the requirement that no scaling is required
 * \li \ref rwRASTERTYPENORMAL
 * \li \ref rwRASTERTYPETEXTURE
 * \li \ref rwRASTERTYPECAMERATEXTURE
 *
 * If the destination raster is of type \ref rwRASTERTYPENORMAL,
 * \ref rwRASTERTYPETEXTURE, or \ref rwRASTERTYPECAMERATEXTURE then blitting can
 * only occur if no scaling is required.
 *
 * Note that alpha blending the source raster into the destination is \e not
 * supported.
 *
 * \see \ref RwRasterRender
 * \see \ref RwRasterRenderScaled
 * \see \ref RwRasterRenderFast
 *
 */

/**
 * \defgroup fpprecisionopengl Floating-point Precision Mode
 * \ingroup realtypesopengl
 *
 * \note This section only applies to Windows OpenGL.
 *
 * \note This section only applies if the RenderWare SDK has been
 * built with available assembler code. This is the default case.
 *
 * Upon calling \ref RwEngineStart and \ref RwCameraBeginUpdate (i.e. every
 * frame), RenderWare Graphics forces the FPU into \e single \e precision mode.
 * This increases the performance of x86 assembler mathematical functions that
 * are used in RenderWare Graphics.
 *
 * \see \ref matrixmultopengl
 * \see \ref matvecprodopengl
 * \see \ref skinassembler
 *
 */

/**
 * \ingroup rwmatrixopengl
 * \page matrixmultopengl RwMatrixMultiply
 *
 * \note This section only applies to Windows OpenGL.
 *
 * \note This section only applies if the RenderWare SDK has been
 * built with available assembler code. This is the default case.
 *
 * The \ref RwMatrixMultiply function has been changed in OpenGL RenderWare
 * Graphics to utilize x86 floating-point assembler code to calculate the
 * product of two \ref RwMatrix's. This assembler is written to take advantage
 * of the structure of an \ref RwMatrix and examples requiring a large number
 * of matrix products will see a performance improvement. For example, in
 * applications that use \ref rpskin.
 *
 * \note This assembler code does \e not take advantage of any vector
 * instructions available on the host processor. It is, however, written to use
 * instruction pairing/overlapping as available on Pentium (and equivalent)
 * processors.
 *
 */

/**
 * \ingroup rwv3dopengl
 * \page matvecprodopengl Matrix-vector Products
 *
 * \note This section only applies to Windows OpenGL.
 *
 * \note This section only applies if the RenderWare SDK has been
 * built with available assembler code. This is the default case.
 *
 * The \ref RwV3dTransformPoints and \ref RwV3dTransformVectors functions have
 * been changed in OpenGL RenderWare Graphics to utilize x86 floating-point
 * assembler code to calculate matrix-vector products. This assembler is
 * written to take advantage of the structure of \ref RwMatrix's and
 * \ref RwV3d's. Performance improvements will be apparent for applications
 * that make heavy use of these functions.
 *
 * \note This assembler code does \e not take advantage of any vector
 * instructions available on the host processor.
 *
 */

/**
 * \ingroup rwenginevideomodeopengl
 * \page videomodeoverviewopengl Overview
 *
 * Fullscreen and windowed modes are supported by RenderWare Graphics for
 * OpenGL. The default is windowed mode. This can be changed between calls to
 * \ref RwEngineOpen and \ref RwEngineStart using the \ref RwEngineSetVideoMode
 * function.
 *
 * RenderWare Graphics for OpenGL also provides additional functionality to
 * change the video mode \e after \ref RwEngineStart has been called. This is
 * provided by \ref RwOpenGLChangeVideoMode.
 *
 * \note In order to perform frame-rate comparisons, ensure that your video
 * card driver settings for the "Vertical sync" are set to "off". Otherwise
 * the OpenGL driver will clamp the frame rate. Please consult your driver
 * documentation instructions for how to do this.
 *
 */


/****************************************************************************
 Global prototypes
 */

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

/* The device globals */
RwRwDeviceGlobals      dgGGlobals;

/****************************************************************************
 Local (static) Globals
 */
static RwUInt32 cachedCameraClearColor = 0;

static RwBool cameraRestoreZTest = FALSE;
static RwBool cameraRestoreZWrite = FALSE;


/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                         Miscellaneous

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/****************************************************************************
 _rwDeviceGetHandle

 On entry   :
 On exit    : Device block handle
 */

RwDevice *
_rwDeviceGetHandle(void)
{
    static RwDevice OpenGLDevice =
    {
        (RwReal)(1),                         /* Default gamma correction */

        _rwOpenGLSystem,

        rwOGLNEARSCREENZ, rwOGLFARSCREENZ,   /* Z buffer near and far values */

        _rwOpenGLSetRenderState,             /* Setting the rendering state */
        _rwOpenGLGetRenderState,             /* Getting the rendering state */

        /* These get set up when the immediate mode module is set up */
        NULL, NULL, NULL, NULL,

        /* These get set up when the immediate mode module is set up */
        NULL, NULL, NULL, NULL
    };    /* OpenGL system */
    RWFUNCTION(RWSTRING("_rwDeviceGetHandle"));

    RWRETURN(&OpenGLDevice);
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                       'Standard' Functions

 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

RwBool
_rwOpenGLHintRenderFront2Back(void * pInOut __RWUNUSED__,
                              void * pIn __RWUNUSED__,
                              RwInt32 renderFront2Back __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("_rwOpenGLHintRenderFront2Back"));

    RWRETURN(TRUE);
}


/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                           Camera Begin/End

 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/****************************************************************************
 _rwOpenGLCameraBeginUpdate

 Start update to camera

 On entry   : NULL
            : pCamera - camera to start update to
            : 0
 On exit    : TRUE on success
 */

RwBool
_rwOpenGLCameraBeginUpdate(void *out __RWUNUSED__,
                           void *cam,
                           RwInt32 in __RWUNUSED__)
{
    RwCamera *camera       = (RwCamera *)cam;
    RwRaster *frameBuffer  = RwCameraGetRaster(camera);
    RwRaster *rasterParent = NULL;
    const RwV2d *vw        = RwCameraGetViewWindow(camera);
    const RwV2d *voffset   = RwCameraGetViewOffset(camera);
    RwFrame *frame         = RwCameraGetFrame(camera);
    RwMatrix *matrix       = RwFrameGetLTM(frame);
    RwV3d *right           = RwMatrixGetRight(matrix);
    RwV3d *pos             = RwMatrixGetPos(matrix);
    RwV3d *up              = RwMatrixGetUp(matrix);
    RwReal nearClip        = RwCameraGetNearClipPlane(camera),
           farClip         = RwCameraGetFarClipPlane(camera),
           fogDistance     = RwCameraGetFogDistance(camera);
    GLfloat m[4][4]        = { { 1, 0, 0, 0 } ,
                               { 0, 1, 0, 0 } ,
                               { 0, 0, 1, 0 },
                               { 0, 0, 0, 1 } };
    RwMatrix    invMat;
    RwV3d       cop;

    static RwReal cachedFogStart = 0,
                  cachedFogEnd   = 0;

    RWFUNCTION(RWSTRING("_rwOpenGLCameraBeginUpdate"));

#if ( !defined( NOASM ) && defined( WIN32 ) )

    /* force the FPU to single precision */
    _rwProcessorForceSinglePrecision();

#endif /* ( !defined( NOASM ) && defined( WIN32 ) ) */

    /* And flag current camera and world */
    dgGGlobals.curCamera = camera;

    rasterParent = frameBuffer;
    while (rasterParent != rasterParent->parent)
    {
        rasterParent = rasterParent->parent;
    }

    _rwOpenGLMakeCurrent();
    GL_ERROR_CHECK();

    /* Set up the projection information */
    glViewport(frameBuffer->nOffsetX,
               _rwOpenGLGetEngineWindowHeight() - (frameBuffer->nOffsetY + frameBuffer->height),
               frameBuffer->width,
               frameBuffer->height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    /* We have to flip our image for our camera texture to be grabbed the right way up! */
    if ( rwRASTERTYPECAMERATEXTURE == frameBuffer->cType )
    {
        glScalef(1, -1, 1);
        glFrontFace(GL_CW);
    }
    else
    {
        glFrontFace(GL_CCW);
    }

    if ( rwPARALLEL == RwCameraGetProjection(camera) )
    {
        /* Calculate the projection values
         * A     = 2 / (right - left)
         * B     = 2 / (top - bottom)
         * C     = - 2 / (far - near)
         * ta    = - (right + left) / (right - left) = 0
         * tb    = - (top + bottom) / (top - bottom) = 0
         * tc    = - (far + near) / (far - near)
         * alpha = - offsetx
         * beta  = - offsety
         */
        GLfloat A = 1 / vw->x;
        GLfloat B = 1 / vw->y;
        GLfloat C = - 2 / (farClip - nearClip);
        GLfloat tc = - (farClip + nearClip) / (farClip - nearClip);
        GLfloat alpha = - voffset->x;
        GLfloat beta = - voffset->y;

        static GLfloat projMat[16] = {
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 1
        };

        projMat[0]  = A;
        projMat[8]  = A * alpha;

        projMat[5]  = B;
        projMat[9]  = B * beta;

        projMat[10] = C;
        projMat[14] = tc;

        /* Setting the projection matrix */
        glMultMatrixf(projMat);
    }
    else
    {
        RwReal x, y;
        RwReal xoffset, yoffset;

        xoffset = -voffset->x * nearClip;
        yoffset = -voffset->y * nearClip;

        x = vw->x * nearClip;
        y = vw->y * nearClip;

        glFrustum(-x + xoffset, x + xoffset, -y + yoffset, y + yoffset, nearClip, farClip);
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /*
     * The following bit of code is equivalent to but faster than:
     *
     * gluLookAt(pos->x, pos->y, pos->z,                            Where's the camera
     *           pos->x + at->x, pos->y + at->y, pos->z + at->z,    What are we looking at
     *           up->x, up->y, up->z);                              What's our orientation
     */

    /*
     * We used the RwMatrixInvert function to take care of any scaling factor.
     * The previous code just transposed the matrix,
     * wich fails in some situations.
     */
    RwMatrixInvert(&invMat, matrix);

    /*
     * As the matrix is now inverted we just need to change the signs
     * for the 1st and 3rd columns.
     */
    m[0][0] = -invMat.right.x;
    m[0][1] = invMat.right.y;
    m[0][2] = -invMat.right.z;

    m[1][0] = -invMat.up.x;
    m[1][1] = invMat.up.y;
    m[1][2] = -invMat.up.z;

    m[2][0] = -invMat.at.x;
    m[2][1] = invMat.at.y;
    m[2][2] = -invMat.at.z;

    glMultMatrixf(&m[0][0]);

    /* Calculate the world space view offset vector */
    RwV3dScale(&cop, right, -camera->viewOffset.x);
    RwV3dIncrementScaled(&cop, up, camera->viewOffset.y);

    glTranslatef(-pos->x - cop.x, -pos->y - cop.y, -pos->z - cop.z);

    /* Last of all, set up some fogging info */
    if (cachedFogStart != fogDistance)
    {
        glFogf(GL_FOG_START, fogDistance);
        cachedFogStart = fogDistance;
    }

    if (cachedFogEnd != farClip)
    {
        glFogf(GL_FOG_END, farClip);
        cachedFogEnd = farClip;
    }

    /* a camera without a Z-buffer doesn't require Z-tests or Z-writes */
    if ( NULL == RwCameraGetZRaster(camera) )
    {
        _rwOpenGLGetRenderState( rwRENDERSTATEZTESTENABLE, (void *)&cameraRestoreZTest );
        if ( FALSE != cameraRestoreZTest )
        {
            _rwOpenGLSetRenderState( rwRENDERSTATEZTESTENABLE, (void *)FALSE );
        }

        _rwOpenGLGetRenderState( rwRENDERSTATEZWRITEENABLE, (void *)&cameraRestoreZWrite );
        if ( FALSE != cameraRestoreZWrite )
        {
            _rwOpenGLSetRenderState( rwRENDERSTATEZWRITEENABLE, (void *)FALSE );
        }
    }

    /* All done */
    GL_ERROR_CHECK();
    RWRETURN(TRUE);
}


/****************************************************************************
 _rwOpenGLCameraEndUpdate

 End update to camera, and tidy up

 On entry   : NULL
            : pCamera - camera to end update to
            : 0
 On exit    : TRUE on success
 */

RwBool
_rwOpenGLCameraEndUpdate(void *out, void *cameraIn ,RwInt32 in)
{
    RwCamera *camera = (RwCamera*)cameraIn;
    RwRaster *camRaster;

    RWFUNCTION(RWSTRING("_rwOpenGLCameraEndUpdate"));

    /* Stop warnings */
    out = out;
    in  = in;

    camRaster = RwCameraGetRaster(camera);

    /* Do we need to generate a nice camera texture now? */
    if ( rwRASTERTYPECAMERATEXTURE == camRaster->cType )
    {
        RwRaster            *parentRas;

        _rwOpenGLRasterExt  *rasExt;

        RwInt32             width;
        
        RwInt32             height;

        RwInt32             boundTexId;


        parentRas = camRaster;
        while ( parentRas->parent != parentRas )
        {
            parentRas = parentRas->parent;
        }

        rasExt = RASTEREXTFROMRASTER(parentRas);

        width  = RwRasterGetWidth(camRaster);
        height = RwRasterGetHeight(camRaster);

        _rwOpenGLMakeCurrent();
        GL_ERROR_CHECK();

        /* Grab the texture from the back buffer */
        glReadBuffer(GL_BACK);

        /* save the currently bound texture as we are invalidating the state cache */
        glGetIntegerv( GL_TEXTURE_BINDING_2D, &boundTexId );

        glBindTexture(GL_TEXTURE_2D, rasExt->textureID);

        glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
        glPixelStorei( GL_UNPACK_ROW_LENGTH, width );

        glCopyTexSubImage2D( GL_TEXTURE_2D,
                             0,                         /* miplevel */
                             camRaster->nOffsetX,
                             camRaster->nOffsetY,       /* Offset x, y */
                             0,                         /* window coords of lower left */
                             _rwOpenGLGetEngineWindowHeight() - height,
                             width, height );           /* subimage dimensions */

        GL_ERROR_CHECK();

        /* restore the bound texture to re-validate the state cache */
        glBindTexture( GL_TEXTURE_2D, boundTexId );
    }

    /* a camera without a Z-buffer may require Z-tests and Z-writes re-enabling */
    if ( FALSE != cameraRestoreZTest )
    {
        _rwOpenGLSetRenderState( rwRENDERSTATEZTESTENABLE, (void *)TRUE );
        cameraRestoreZTest = FALSE;
    }

    if ( FALSE != cameraRestoreZWrite )
    {
        _rwOpenGLSetRenderState( rwRENDERSTATEZWRITEENABLE, (void *)TRUE );
        cameraRestoreZWrite = FALSE;
    }

    /* No current camera now */
    dgGGlobals.curCamera = NULL;

    RWRETURN(TRUE);
}


/****************************************************************************
 _rwOpenGLCameraClear

 On entry   : Camera
            : RwRGBA colour to clear to
            : Buffer mask to clear
 On exit    : TRUE on success
 */

RwBool
_rwOpenGLCameraClear(void *inOut, void *in, RwInt32 nIn)
{
    /* pInOut is camera */
    /* pIn is pointer to RwRGBA giving colour to clear frame buffer to */
    /* nIn is buffer clear mask */
    RwCamera *camera    = (RwCamera*)inOut;
    RwRaster *camRaster = RwCameraGetRaster(camera);
    RwRaster *rasParent;
    RwBool    clearSubRect = FALSE;
    GLbitfield clearType   = 0;
    RwBool    stencilOn, depthWriteOn;
    RwCamera *oldCamera;


    RWFUNCTION(RWSTRING("_rwOpenGLCameraClear"));

    oldCamera = dgGGlobals.curCamera;
    dgGGlobals.curCamera = camera;

    _rwOpenGLMakeCurrent();
    GL_ERROR_CHECK();

    if (nIn & rwCAMERACLEARIMAGE)
    {
        /* Going to clear the framebuffer, so set up the colour */
        if (cachedCameraClearColor != *(RwUInt32 *)in)
        {
            RwRGBA *col = (RwRGBA *)in;

            glClearColor((GLfloat)col->red   * (1.0f/255.0f),
                         (GLfloat)col->green * (1.0f/255.0f),
                         (GLfloat)col->blue  * (1.0f/255.0f),
                         (GLfloat)col->alpha * (1.0f/255.0f));
            cachedCameraClearColor = *(RwUInt32 *)in;
        }
    }

    if (nIn & rwCAMERACLEARIMAGE)
    {
        /* Clear the frame buffer */
        clearType |= GL_COLOR_BUFFER_BIT;
    }

    if (nIn & rwCAMERACLEARZ)
    {
        /* Clear the Z buffer */
        clearType |= GL_DEPTH_BUFFER_BIT;
    }

    if (nIn & rwCAMERACLEARSTENCIL)
    {
        /* Clear the Stencil buffer - always with the default 0x0 value */
        clearType |= GL_STENCIL_BUFFER_BIT;
    }

    /* find the parent raster for the frame buffer */
    /* and assume its dimensions are the same as Z */
    rasParent = camRaster;
    while (rasParent != rasParent->parent)
    {
        rasParent = rasParent->parent;
    }

    if ((rasParent->width    != camRaster->width   ) ||
        (rasParent->height   != camRaster->height  ) ||
        (rasParent->nOffsetX != camRaster->nOffsetX) ||
        (rasParent->nOffsetY != camRaster->nOffsetY))
    {
        clearSubRect = TRUE;

        /*
         * Restrict camera raster clearing to only the region of the client window that the
         * raster occupies
         */
        RwOpenGLEnable( rwGL_SCISSOR_TEST );

        glScissor( camRaster->nOffsetX,
                   _rwOpenGLGetEngineWindowHeight() -
                        (camRaster->nOffsetY + camRaster->height),
                   camRaster->width,
                   camRaster->height );
    }

    stencilOn = RwOpenGLIsEnabled( rwGL_STENCIL_TEST );
    _rwOpenGLGetRenderState(rwRENDERSTATEZWRITEENABLE, &depthWriteOn);

    if (stencilOn && (nIn & rwCAMERACLEARSTENCIL))
    {
        RwOpenGLDisable( rwGL_STENCIL_TEST );
    }

    if (!depthWriteOn && (nIn & rwCAMERACLEARZ))
    {
        glDepthMask(GL_TRUE);
        glClear(clearType);
        glDepthMask(GL_FALSE);
    }
    else
    {
        glClear(clearType);
    }

    if (stencilOn && (nIn & rwCAMERACLEARSTENCIL))
    {
        RwOpenGLEnable( rwGL_STENCIL_TEST );
    }

    if (clearSubRect)
    {
        RwOpenGLDisable( rwGL_SCISSOR_TEST );
    }

    dgGGlobals.curCamera = oldCamera;
    GL_ERROR_CHECK();

    RWRETURN(TRUE);
}


/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                            Opening/Closing

 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/****************************************************************************
 _rwOpenGLOpen

 On entry   :
 On exit    : TRUE on success
 */

RwBool
_rwOpenGLOpen(void)
{
    RWFUNCTION(RWSTRING("_rwOpenGLOpen"));

    /* Initialise all extensions to something sensible */
    _rwOpenGLExt.LockArrays                     = NULL;
    _rwOpenGLExt.UnlockArrays                   = NULL;

    _rwOpenGLExt.FlushVertexArrayRangeNV        = NULL;
    _rwOpenGLExt.VertexArrayRangeNV             = NULL;
    _rwOpenGLExt.AllocateMemoryNV               = NULL;
    _rwOpenGLExt.FreeMemoryNV                   = NULL;

    _rwOpenGLExt.GenFencesNV                    = NULL;
    _rwOpenGLExt.DeleteFencesNV                 = NULL;
    _rwOpenGLExt.SetFenceNV                     = NULL;
    _rwOpenGLExt.FinishFenceNV                  = NULL;
    _rwOpenGLExt.IsFenceNV                      = NULL;
    _rwOpenGLExt.TestFenceNV                    = NULL;

    _rwOpenGLExt.MaxTextureUnits                = 1;
    _rwOpenGLExt.ActiveTextureARB               = NULL;
    _rwOpenGLExt.ClientActiveTextureARB         = NULL;
    _rwOpenGLExt.MultiTexCoord2fvARB            = NULL;

    _rwOpenGLExt.TextureEnvCombineARB           = FALSE;

    _rwOpenGLExt.TextureEnvAddARB               = FALSE;
    _rwOpenGLExt.TextureCubeMapARB              = FALSE;
    _rwOpenGLExt.MaxCubeMapSize                 = 0;
    _rwOpenGLExt.VertexArrayRange2NV            = FALSE;

    _rwOpenGLExt.MaxTextureAnisotropy           = 0;

    _rwOpenGLExt.NewObjectBufferATI             = NULL;
    _rwOpenGLExt.IsObjectBufferATI              = NULL;
    _rwOpenGLExt.FreeObjectBufferATI            = NULL;
    _rwOpenGLExt.UpdateObjectBufferATI          = NULL;
    _rwOpenGLExt.ArrayObjectATI                 = NULL;

    _rwOpenGLExt.CompressedTexImage3DARB        = NULL;
    _rwOpenGLExt.CompressedTexImage2DARB        = NULL;
    _rwOpenGLExt.CompressedTexImage1DARB        = NULL;
    _rwOpenGLExt.CompressedTexSubImage3DARB     = NULL;
    _rwOpenGLExt.CompressedTexSubImage2DARB     = NULL;
    _rwOpenGLExt.CompressedTexSubImage1DARB     = NULL;
    _rwOpenGLExt.GetCompressedTexImageARB       = NULL;

    _rwOpenGLExt.TextureCompressionS3TCEXT      = FALSE;

    _rwOpenGLExt.TextureMirroredRepeatARB       = FALSE;

    _rwOpenGLExt.GenerateMipmapSGIS             = FALSE;

    _rwOpenGLExt.TextureEdgeClampEXT            = FALSE;

    _rwOpenGLExt.BlendSquareNV                  = FALSE;

    /* invalidate the cached camera clear color */
    cachedCameraClearColor = 0;

    RWRETURN(TRUE);
}

/****************************************************************************
 _rwOpenGLClose

 On entry   :
 On exit    :
 */

void
_rwOpenGLClose(void)
{
    RWFUNCTION(RWSTRING("_rwOpenGLClose"));
    RWRETURNVOID();
}

