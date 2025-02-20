/***************************************************************************
 *                                                                         *
 * Module  : drvmodel.h (OpenGL)                                           *
 *                                                                         *
 * Purpose : Driver model description (device specific vertices/polys)     *
 *                                                                         *
 **************************************************************************/

/* RWPUBLIC */
#ifndef OPENGL_DRVMODEL_H
#define OPENGL_DRVMODEL_H

#if (defined(__ICL))
/* Avoid voluminous
 *   'warning #344: typedef name has already been declared (with same type)'
 * warnings from MS include files
 */
#pragma warning( disable : 344 )
#endif /* (defined(__ICL)) */

/* RWPUBLICEND */

/****************************************************************************
 Includes
 */

#include "batypes.h"
#include "bamatrix.h"

/* RWPUBLIC */
/****************************************************************************
 Defines
 */

/* Set true depth information (for fogging, eg) */
#define RwIm2DVertexSetCameraX(vert, camx)     /* Nothing */
#define RwIm2DVertexSetCameraY(vert, camy)     /* Nothing */
#define RwIm2DVertexSetCameraZ(vert, camz)     /* Nothing */
#define RwIm2DVertexSetRecipCameraZ(vert, recipz)    ((vert)->recpz = (recipz))

#define RwIm2DVertexGetCameraX(vert)           (cause an error)
#define RwIm2DVertexGetCameraY(vert)           (cause an error)
#define RwIm2DVertexGetCameraZ(vert)           (cause an error)
#define RwIm2DVertexGetRecipCameraZ(vert)            ((vert)->recpz)

/* Set screen space coordinates in a device vertex */
#define RwIm2DVertexSetScreenX(vert, scrnx)    ((vert)->x = (scrnx))
#define RwIm2DVertexSetScreenY(vert, scrny)    ((vert)->y = (scrny))
#define RwIm2DVertexSetScreenZ(vert, scrnz)    ((vert)->z = (scrnz))
#define RwIm2DVertexGetScreenX(vert)           ((vert)->x)
#define RwIm2DVertexGetScreenY(vert)           ((vert)->y)
#define RwIm2DVertexGetScreenZ(vert)           ((vert)->z)

/* Set texture coordinates in a device vertex */
#define RwIm2DVertexSetU(vert, u, recipz)       \
MACRO_START                                     \
{ ((vert)->oow = (recipz));                     \
  ((vert)->row = (RwReal)(0));                  \
  ((vert)->sow = ((u) * (recipz)));             \
}                                               \
MACRO_STOP
#define RwIm2DVertexSetV(vert, v, recipz)      ((vert)->tow = ((v) * (recipz)))
#define RwIm2DVertexGetU(vert)                 ((vert)->sow / ((vert)->oow))
#define RwIm2DVertexGetV(vert)                 ((vert)->tow / ((vert)->oow))

/* Modify the luminance stuff */
#define RwIm2DVertexSetRealRGBA(vert, red, green, blue, alpha)          \
MACRO_START                                                             \
{                                                                       \
    ((vert)->r = ((unsigned char)RwFastRealToUInt32(red)));             \
    ((vert)->g = ((unsigned char)RwFastRealToUInt32(green)));           \
    ((vert)->b = ((unsigned char)RwFastRealToUInt32(blue)));            \
    ((vert)->a = ((unsigned char)RwFastRealToUInt32(alpha)));           \
}                                                                       \
MACRO_STOP

#define RwIm2DVertexSetIntRGBA(vert, red, green, blue, alpha)           \
MACRO_START                                                             \
{                                                                       \
    ((vert)->r = ((unsigned char)(red)  ));                             \
    ((vert)->g = ((unsigned char)(green)));                             \
    ((vert)->b = ((unsigned char)(blue) ));                             \
    ((vert)->a = ((unsigned char)(alpha)));                             \
}                                                                       \
MACRO_STOP

#define RwIm2DVertexGetRed(vert)                ((vert)->r)
#define RwIm2DVertexGetGreen(vert)              ((vert)->g)
#define RwIm2DVertexGetBlue(vert)               ((vert)->b)
#define RwIm2DVertexGetAlpha(vert)              ((vert)->a)

#define RwIm2DVertexCopyRGBA(dst, src)                  \
MACRO_START                                             \
{                                                       \
    ((dst)->r = (src)->r);                              \
    ((dst)->g = (src)->g);                              \
    ((dst)->b = (src)->b);                              \
    ((dst)->a = (src)->a);                              \
}                                                       \
MACRO_STOP

/* Clipper stuff */
#define RwIm2DVertexClipRGBA(out, interp, near, far)    \
MACRO_START                                             \
{                                                       \
    (out)->r = (unsigned char)RwFastRealToUInt32(       \
        RWSHADCLIP((interp), (near)->r, (far)->r));     \
    (out)->g = (unsigned char)RwFastRealToUInt32(       \
        RWSHADCLIP((interp), (near)->g, (far)->g));     \
    (out)->b = (unsigned char)RwFastRealToUInt32(       \
        RWSHADCLIP((interp), (near)->b, (far)->b));     \
    (out)->a = (unsigned char)RwFastRealToUInt32(       \
        RWSHADCLIP((interp), (near)->a, (far)->a));     \
}                                                       \
MACRO_STOP

/****************************************************************************
 Global Types
 */

/* We use RwOpenGLVertex to drive the hardware */
/* The polygon index structure is 32 bit - see above */

typedef struct RwOpenGLVertex RwOpenGLVertex;
/**
 * \ingroup rwim3dopengl
 * \struct RwOpenGLVertex
 *
 * Structure describing a vertex in OpenGL
 */
struct RwOpenGLVertex
{
    float           x;      /**< X positional component */
    
    float           y;      /**< Y positional component */
    
    float           z;      /**< Z positional component */

    unsigned char   r;      /**< 8-bit red color component */
    
    unsigned char   g;      /**< 8-bit green color component */
    
    unsigned char   b;      /**< 8-bit blue color component */
    
    unsigned char   a;      /**< 8-bit alpha component */

    float           sow;    /**< Perspective-correct U texture coordinate */
    
    float           tow;    /**< Perspective-correct V texture coordinate */
    
    float           row;    /**< Internal use only */
    
    float           oow;    /**< Texture perspective correction */
    
    float           recpz;  /**< Reciprocal Z component */
};

/* Define types used */

/**
 * \ingroup rwim2dvertex
 * \ref RwIm2DVertex
 * Typedef for a RenderWare Graphics Immediate Mode 2D Vertex
 */

typedef RwOpenGLVertex      RwIm2DVertex;

/**
 * \ingroup rwcoregeneric
 * \ref RxVertexIndex
 * Typedef for a RenderWare Graphics PowerPipe 
 * Immediate Mode Vertex
 */
typedef RwUInt32        RxVertexIndex;

/**
 * \ingroup rwcoregeneric
 * \ref RwImVertexIndex
 * Typedef for a RenderWare Graphics Immediate Mode Vertex
 */

typedef RxVertexIndex   RwImVertexIndex;


/* OpenGL extension data and types - as defined in glext.h */

#ifdef WIN32
#ifndef APIENTRY
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <gl/gl.h>
#endif
#else
#define APIENTRY
#include "gl/gl.h"
#include "gl/glext.h"
#endif

#if !defined(GL_EXT_packed_pixels)
#define GL_UNSIGNED_BYTE_3_3_2_EXT          0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4_EXT       0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1_EXT       0x8034
#define GL_UNSIGNED_INT_8_8_8_8_EXT         0x8035
#define GL_UNSIGNED_INT_10_10_10_2_EXT      0x8036
#endif /* !defined(GL_EXT_packed_pixels) */

#if !defined(GL_EXT_compiled_vertex_array)
typedef void (APIENTRY * PFNGLLOCKARRAYSEXTPROC) (int first, int count);
typedef void (APIENTRY * PFNGLUNLOCKARRAYSEXTPROC) (void);
#endif /* !defined(GL_EXT_compiled_vertex_array) */

#if !defined(GL_NV_vertex_array_range)
#define GL_VERTEX_ARRAY_RANGE_NV            0x851D
#define GL_VERTEX_ARRAY_RANGE_LENGTH_NV     0x851E
#define GL_VERTEX_ARRAY_RANGE_VALID_NV      0x851F
#define GL_MAX_VERTEX_ARRAY_RANGE_ELEMENT_NV 0x8520
#define GL_VERTEX_ARRAY_RANGE_POINTER_NV    0x8521

typedef void (APIENTRY * PFNGLFLUSHVERTEXARRAYRANGENVPROC) (void);
typedef void (APIENTRY * PFNGLVERTEXARRAYRANGENVPROC) (int size,
                                                       const void *pointer);
typedef void* (APIENTRY * PFNWGLALLOCATEMEMORYNVPROC) (int size,
                                                       float readfreq,
                                                       float writefreq,
                                                       float priority);
typedef void (APIENTRY * PFNWGLFREEMEMORYNVPROC) (void *pointer);
#endif /* !defined(GL_NV_vertex_array_range) */

#if !defined(GL_NV_vertex_array_range2)
#define GL_VERTEX_ARRAY_RANGE_WITHOUT_FLUSH_NV 0x8533
#endif /* !defined(GL_NV_vertex_array_range2) */

#if !defined(GL_NV_fence)
#define GL_ALL_COMPLETED_NV               0x84F2

typedef void (APIENTRY * PFNGLDELETEFENCESNVPROC) (int n,
                                                   const void *fences);
typedef void (APIENTRY * PFNGLGENFENCESNVPROC) (int n,
                                                int *fences);
typedef void (APIENTRY * PFNGLSETFENCENVPROC) (int fence,
                                               int condition);
typedef void (APIENTRY * PFNGLFINISHFENCENVPROC) (int fence);
typedef int (APIENTRY * PFNGLISFENCENVPROC) (int fence);
typedef int (APIENTRY * PFNGLTESTFENCENVPROC) (int fence);
#endif /* !defined(GL_NV_fence) */

#if !defined(GL_EXT_paletted_texture)
#define GL_COLOR_INDEX1_EXT               0x80E2
#define GL_COLOR_INDEX2_EXT               0x80E3
#define GL_COLOR_INDEX4_EXT               0x80E4
#define GL_COLOR_INDEX8_EXT               0x80E5
#define GL_COLOR_INDEX12_EXT              0x80E6
#define GL_COLOR_INDEX16_EXT              0x80E7

typedef void 
(APIENTRY * PFNGLCOLORTABLEEXTPROC) (unsigned int target,
                                     unsigned int internalformat,
                                     int width,
                                     unsigned int format,
                                     unsigned int type,
                                     const void *table);
typedef void 
(APIENTRY * PFNGLGETCOLORTABLEEXTPROC) (unsigned int target, 
                                        unsigned int format, 
                                        unsigned int type, 
                                        void *data);
#endif /* !defined(GL_EXT_paletted_texture) */

#if !defined(GL_ARB_multitexture)
#define GL_TEXTURE0_ARB                   0x84C0
#define GL_TEXTURE1_ARB                   0x84C1
#define GL_TEXTURE2_ARB                   0x84C2
#define GL_TEXTURE3_ARB                   0x84C3
#define GL_TEXTURE4_ARB                   0x84C4
#define GL_TEXTURE5_ARB                   0x84C5
#define GL_TEXTURE6_ARB                   0x84C6
#define GL_TEXTURE7_ARB                   0x84C7
#define GL_TEXTURE8_ARB                   0x84C8
#define GL_TEXTURE9_ARB                   0x84C9
#define GL_TEXTURE10_ARB                  0x84CA
#define GL_TEXTURE11_ARB                  0x84CB
#define GL_TEXTURE12_ARB                  0x84CC
#define GL_TEXTURE13_ARB                  0x84CD
#define GL_TEXTURE14_ARB                  0x84CE
#define GL_TEXTURE15_ARB                  0x84CF
#define GL_TEXTURE16_ARB                  0x84D0
#define GL_TEXTURE17_ARB                  0x84D1
#define GL_TEXTURE18_ARB                  0x84D2
#define GL_TEXTURE19_ARB                  0x84D3
#define GL_TEXTURE20_ARB                  0x84D4
#define GL_TEXTURE21_ARB                  0x84D5
#define GL_TEXTURE22_ARB                  0x84D6
#define GL_TEXTURE23_ARB                  0x84D7
#define GL_TEXTURE24_ARB                  0x84D8
#define GL_TEXTURE25_ARB                  0x84D9
#define GL_TEXTURE26_ARB                  0x84DA
#define GL_TEXTURE27_ARB                  0x84DB
#define GL_TEXTURE28_ARB                  0x84DC
#define GL_TEXTURE29_ARB                  0x84DD
#define GL_TEXTURE30_ARB                  0x84DE
#define GL_TEXTURE31_ARB                  0x84DF
#define GL_ACTIVE_TEXTURE_ARB             0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB      0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB          0x84E2

typedef void (APIENTRY * PFNGLACTIVETEXTUREARBPROC) (unsigned int texture);
typedef void (APIENTRY * PFNGLCLIENTACTIVETEXTUREARBPROC) (unsigned int texture);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2FVARBPROC) (unsigned int target,
                                                        const float *v);
#endif /* !defined(GL_ARB_multitexture) */

#if !defined(GL_ARB_texture_env_combine)
#define GL_COMBINE_ARB                    0x8570
#define GL_COMBINE_RGB_ARB                0x8571
#define GL_COMBINE_ALPHA_ARB              0x8572
#define GL_RGB_SCALE_ARB                  0x8573
#define GL_ADD_SIGNED_ARB                 0x8574
#define GL_INTERPOLATE_ARB                0x8575
#define GL_CONSTANT_ARB                   0x8576
#define GL_PRIMARY_COLOR_ARB              0x8577
#define GL_PREVIOUS_ARB                   0x8578
#define GL_SOURCE0_RGB_ARB                0x8580
#define GL_SOURCE1_RGB_ARB                0x8581
#define GL_SOURCE2_RGB_ARB                0x8582
#define GL_SOURCE0_ALPHA_ARB              0x8588
#define GL_SOURCE1_ALPHA_ARB              0x8589
#define GL_SOURCE2_ALPHA_ARB              0x858A
#define GL_OPERAND0_RGB_ARB               0x8590
#define GL_OPERAND1_RGB_ARB               0x8591
#define GL_OPERAND2_RGB_ARB               0x8592
#define GL_OPERAND0_ALPHA_ARB             0x8598
#define GL_OPERAND1_ALPHA_ARB             0x8599
#define GL_OPERAND2_ALPHA_ARB             0x859A
#define GL_SUBTRACT_ARB                   0x84E7
#endif /* !defined(GL_ARB_texture_env_combine) */

#if !defined(GL_ARB_texture_cube_map)
#define GL_NORMAL_MAP_ARB                  0x8511
#define GL_REFLECTION_MAP_ARB              0x8512
#define GL_TEXTURE_CUBE_MAP_ARB            0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP_ARB    0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB 0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB 0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB 0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB 0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB 0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB 0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP_ARB      0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB   0x851C
#endif /* !defined(GL_ARB_texture_cube_map) */

#if !defined(GL_EXT_texture_filter_anisotropic)
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif /* !defined(GL_EXT_texture_filter_anisotropic) */

#if !defined(GL_ATI_vertex_array_object)
#define GL_STATIC_ATI                           0x8760
#define GL_DYNAMIC_ATI                          0x8761
#define GL_PRESERVE_ATI                         0x8762
#define GL_DISCARD_ATI                          0x8763

#define GL_OBJECT_BUFFER_SIZE_ATI               0x8764
#define GL_OBJECT_BUFFER_USAGE_ATI              0x8765
#define GL_ARRAY_OBJECT_BUFFER_ATI              0x8766
#define GL_ARRAY_OBJECT_OFFSET_ATI              0x8767

typedef unsigned int (APIENTRY * PFNGLNEWOBJECTBUFFERATIPROC)(int size,
                                                              const void *pointer,
                                                              unsigned int usage);
typedef unsigned char (APIENTRY * PFNGLISOBJECTBUFFERATIPROC)(unsigned int buffer);
typedef void (APIENTRY * PFNGLUPDATEOBJECTBUFFERATIPROC)(unsigned int buffer,
                                                         unsigned int offset,
                                                         int size,
                                                         const void *pointer,
                                                         unsigned int preserve);
typedef void (APIENTRY * PFNGLFREEOBJECTBUFFERATIPROC)(unsigned int buffer);
typedef void (APIENTRY * PFNGLARRAYOBJECTATIPROC)(unsigned int array,
                                                  int size,
                                                  unsigned int type,
                                                  int stride,
                                                  unsigned int buffer,
                                                  unsigned int offset);
#endif /* !defined(GL_ATI_vertex_array_object) */

#if !defined(GL_ARB_texture_compression)

#define GL_COMPRESSED_ALPHA_ARB                 0x84E9
#define GL_COMPRESSED_LUMINANCE_ARB             0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA_ARB       0x84EB
#define GL_COMPRESSED_INTENSITY_ARB             0x84EC
#define GL_COMPRESSED_RGB_ARB                   0x84ED
#define GL_COMPRESSED_RGBA_ARB                  0x84EE
#define GL_TEXTURE_COMPRESSION_HINT_ARB         0x84EF
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB    0x86A0
#define GL_TEXTURE_COMPRESSED_ARB               0x86A1
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB   0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS_ARB       0x86A3

typedef void (APIENTRY * PFNGLCOMPRESSEDTEXIMAGE3DARB)( unsigned int target,
                                                        int level,
                                                        unsigned int internalformat,
                                                        int width,
                                                        int height,
                                                        int depth,
                                                        int border,
                                                        int imageSize,
                                                        const void *data );

typedef void (APIENTRY * PFNGLCOMPRESSEDTEXIMAGE2DARB)( unsigned int target,
                                                        int level,
                                                        unsigned int internalformat,
                                                        int width,
                                                        int height,
                                                        int border, 
                                                        int imageSize,
                                                        const void *data );

typedef void (APIENTRY * PFNGLCOMPRESSEDTEXIMAGE1DARB)( unsigned int target,
                                                        int level,
                                                        unsigned int internalformat,
                                                        int width,
                                                        int border,
                                                        int imageSize,
                                                        const void *data );

typedef void (APIENTRY * PFNGLCOMPRESSEDTEXSUBIMAGE3DARB)( unsigned int target,
                                                           int level, 
                                                           int xoffset,
                                                           int yoffset,
                                                           int zoffset,
                                                           int width,
                                                           int height,
                                                           int depth,
                                                           unsigned int format,
                                                           int imageSize,
                                                           const void *data );

typedef void (APIENTRY * PFNGLCOMPRESSEDTEXSUBIMAGE2DARB)( unsigned int target,
                                                           int level,
                                                           int xoffset,
                                                           int yoffset,
                                                           int width,
                                                           int height,
                                                           unsigned int format,
                                                           int imageSize,
                                                           const void *data );

typedef void (APIENTRY * PFNGLCOMPRESSEDTEXSUBIMAGE1DARB)( unsigned int target,
                                                           int level, 
                                                           int xoffset,
                                                           int width,
                                                           unsigned int format,
                                                           int imageSize,
                                                           const void *data );

typedef void (APIENTRY * PFNGLGETCOMPRESSEDTEXIMAGEARB)( unsigned int target,
                                                         int lod,
                                                         void *img );

#endif /* !defined(GL_ARB_texture_compression) */

#if !defined(GL_EXT_texture_compression_s3tc)
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT     0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT    0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT    0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT    0x83F3
#endif /* !defined(GL_EXT_texture_compression_s3tc) */

#if !defined(GL_ARB_texture_mirrored_repeat)
#define GL_MIRRORED_REPEAT_ARB			    0x8370
#endif /* !defined(GL_ARB_texture_mirrored_repeat) */

#if !defined(GL_SGIS_generate_mipmap)
#define GL_GENERATE_MIPMAP_SGIS             0x8191
#define GL_GENERATE_MIPMAP_HINT_SGIS        0x8192
#endif /* !defined(GL_SGIS_generate_mipmap) */

#if !defined(GL_EXT_texture_edge_clamp)
#define GL_CLAMP_TO_EDGE_EXT                0x812F
#endif /* !defined(GL_EXT_texture_edge_clamp) */

typedef struct RwOpenGLExtensions RwOpenGLExtensions;
/** 
 * \ingroup rwengineopengl
 * \struct RwOpenGLExtensions
 *
 * This type contains the necessary information to query whether
 * an OpenGL extension (and the functions they provide) that RenderWare
 * Graphics requires is supported by the video card drivers on
 * the host computer, and any additional extension-centric data.
 *
 * Function pointers are NULL if the function is not supported.
 * \ref RwBool values are FALSE if the extension is not supported.
 * Other values are dependent on the extension that they refer to.
 */
struct RwOpenGLExtensions
{
    PFNGLLOCKARRAYSEXTPROC                  LockArrays;
        /**< Function pointer to glLockArraysEXT (or glLockArraysSGI).
          *  (Extension \e GL_EXT_compiled_vertex_array
          *  (or \e GL_SGI_compiled_vertex_array).) */

    PFNGLUNLOCKARRAYSEXTPROC                UnlockArrays;
        /**< Function pointer to glUnlockArraysEXT (or glUnlockArraysSGI).
          *  (Extension \e GL_EXT_compiled_vertex_array
          *  (or \e GL_SGI_compiled_vertex_array).) */

    PFNGLFLUSHVERTEXARRAYRANGENVPROC        FlushVertexArrayRangeNV;
        /**< Function pointer to glFlushVertexArrayRangeNV.
          *  (Extension \e GL_NV_vertex_array_range.) */

    PFNGLVERTEXARRAYRANGENVPROC             VertexArrayRangeNV;
        /**< Function pointer to glVertexArrayRangeNV.
          *  (Extension \e GL_NV_vertex_array_range.) */

    PFNWGLALLOCATEMEMORYNVPROC              AllocateMemoryNV;
        /**< Function pointer to wglAllocateMemoryNV.
          *  (Extension \e GL_NV_vertex_array_range.) */

    PFNWGLFREEMEMORYNVPROC                  FreeMemoryNV;
        /**< Function pointer to wglFreeMemoryNV.
          *  (Extension \e GL_NV_vertex_array_range.) */

    PFNGLGENFENCESNVPROC                    GenFencesNV;
        /**< Function pointer to glGenFencesNV.
          *  (Extension \e GL_NV_fence.) */

    PFNGLDELETEFENCESNVPROC                 DeleteFencesNV;
        /**< Function pointer to glDeleteFencesNV.
          *  (Extension \e GL_NV_fence.) */

    PFNGLSETFENCENVPROC                     SetFenceNV;
        /**< Function pointer to glSetFenceNV.
          *  (Extension \e GL_NV_fence.) */

    PFNGLFINISHFENCENVPROC                  FinishFenceNV;
        /**< Function pointer to glFinishFenceNV.
          *  (Extension \e GL_NV_fence.) */

    PFNGLISFENCENVPROC                      IsFenceNV;
        /**< Function pointer to glIsFenceNV.
          *  (Extension \e GL_NV_fence.) */

    PFNGLTESTFENCENVPROC                    TestFenceNV;
        /**< Function pointer to glTestFenceNV.
          *  (Extension \e GL_NV_fence.) */

    RwUInt32                                MaxTextureUnits;
        /**< \ref RwUInt32 containing the value of \c GL_MAX_TEXTURE_UNITS_ARB.
          *  (Extension \e GL_ARB_multitexture.) */

    PFNGLACTIVETEXTUREARBPROC               ActiveTextureARB;
        /**< Function pointer to glActiveTextureARB.
          *  (Extension \e GL_ARB_multitexture.) */

    PFNGLCLIENTACTIVETEXTUREARBPROC         ClientActiveTextureARB;
        /**< Function pointer to glClientActiveTextureARB.
          *  (Extension \e GL_ARB_multitexture.) */

    PFNGLMULTITEXCOORD2FVARBPROC            MultiTexCoord2fvARB;
        /**< Function pointer to glMultiTexCoord2fvARB.
          *  (Extension \e GL_ARB_multitexture.) */

    RwBool                                  TextureEnvCombineARB;
        /**< \ref RwBool, TRUE if the \e GL_ARB_texture_env_combine
          *  extension is supported, FALSE if not. */

    RwBool                                  TextureEnvAddARB;
        /**< \ref RwBool, TRUE if the \e GL_ARB_texture_env_add
          *  extension is supported, FALSE if not. */

    RwBool                                  TextureCubeMapARB;
        /**< \ref RwBool, TRUE if the \e GL_ARB_texture_cube_map
          *  extension is supported, FALSE if not. */

    RwUInt32                                MaxCubeMapSize;
        /**< \ref RwUInt32 containing the maximum cube map size allowed.
          *  The width and height of the cube map must be the same. */

    RwBool                                  VertexArrayRange2NV;
        /**< \ref RwBool, TRUE if the \e GL_NV_vertex_array_range2
          *  extension is supported, FALSE if not. */

    RwInt8                                  MaxTextureAnisotropy;
        /**< \ref RwInt8 containing the maximum texture anisotropy, or 0
          *  if the \e GL_EXT_texture_filter_anisotropic is not supported. */

    PFNGLNEWOBJECTBUFFERATIPROC             NewObjectBufferATI;
        /**< Function pointer to glNewObjectBufferATI.
          *  (Extension \e GL_ATI_vertex_array_object.) */

    PFNGLISOBJECTBUFFERATIPROC              IsObjectBufferATI;
        /**< Function pointer to glIsObjectBufferATI.
          *  (Extension \e GL_ATI_vertex_array_object.) */
    
    PFNGLUPDATEOBJECTBUFFERATIPROC          UpdateObjectBufferATI;
        /**< Function pointer to glUpdateObjectBufferATI.
          *  (Extension \e GL_ATI_vertex_array_object.) */

    
    PFNGLFREEOBJECTBUFFERATIPROC            FreeObjectBufferATI;
        /**< Function pointer to glFreeObjectBufferATI.
          *  (Extension \e GL_ATI_vertex_array_object.) */
    
    PFNGLARRAYOBJECTATIPROC                 ArrayObjectATI;
        /**< Function pointer to glArrayObjectATI.
          *  (Extension \e GL_ATI_vertex_array_object.) */

    PFNGLCOMPRESSEDTEXIMAGE3DARB            CompressedTexImage3DARB;
        /**< Function pointer to glCompressedTexImage3DARB.
          *  (Extension \e GL_ARB_texture_compression.) */

    PFNGLCOMPRESSEDTEXIMAGE2DARB            CompressedTexImage2DARB;
        /**< Function pointer to glCompressedTexImage2DARB.
          *  (Extension \e GL_ARB_texture_compression.) */

    PFNGLCOMPRESSEDTEXIMAGE1DARB            CompressedTexImage1DARB;
        /**< Function pointer to glCompressedTexImage1DARB.
          *  (Extension \e GL_ARB_texture_compression.) */

    PFNGLCOMPRESSEDTEXSUBIMAGE3DARB         CompressedTexSubImage3DARB;
        /**< Function pointer to glCompressedTexSubImage3DARB.
          *  (Extension \e GL_ARB_texture_compression.) */

    PFNGLCOMPRESSEDTEXSUBIMAGE2DARB         CompressedTexSubImage2DARB;
        /**< Function pointer to glCompressedTexSubImage2DARB.
          *  (Extension \e GL_ARB_texture_compression.) */

    PFNGLCOMPRESSEDTEXSUBIMAGE1DARB         CompressedTexSubImage1DARB;
        /**< Function pointer to glCompressedTexSubImage1DARB.
          *  (Extension \e GL_ARB_texture_compression.) */

    PFNGLGETCOMPRESSEDTEXIMAGEARB           GetCompressedTexImageARB;
        /**< Function pointer to glGetCompressedTexImageARB.
          *  (Extension \e GL_ARB_texture_compression.) */

    RwBool                                  TextureCompressionS3TCEXT;
        /**< \ref RwBool, TRUE if the \e GL_EXT_texture_compression_s3tc
          *  extension is supported, FALSE if not. */

    RwBool                                  TextureMirroredRepeatARB;
        /**< \ref RwBool, TRUE if the \e GL_ARB_texture_mirrored_repeat
          *  extension is supported, FALSE if not. */

    RwBool                                  GenerateMipmapSGIS;
        /**< \ref RwBool, TRUE if the \e GL_SGIS_generate_mipmap
          *  extension is supported, FALSE if not. */

    RwBool                                  TextureEdgeClampEXT;
        /**< \ref RwBool, TRUE if the \e GL_EXT_texture_edge_clamp
          *  extension is supported, FALSE if not. */

    RwBool                                  BlendSquareNV;
        /**< \ref RwBool, TRUE if the \e GL_NV_blend_square
          *  extension is supported, FALSE if not. */
};


typedef struct RwXWindowInfo RwXWindowInfo;
#if (!defined(DOXYGEN))
struct RwXWindowInfo
{
    void    *display;
    void    *window;
    void    *visualInfo;
};
#endif /* (!defined(DOXYGEN)) */

/* make the Win32 OpenGL pixel format descriptor accessible */
#if defined( _WIN32 )

extern PIXELFORMATDESCRIPTOR _rwOpenGLPixelFormatDesc;

#endif /* defined( _WIN32 ) */


#endif /* OPENGL_DRVMODEL_H */
/* RWPUBLICEND */
