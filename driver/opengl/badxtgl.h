/***************************************************************************
 *                                                                         *
 * Module  : badxtgl.h                                                     *
 *                                                                         *
 * Purpose : OpenGL DXT decompression support                              *
 *                                                                         *
 **************************************************************************/

#if !defined(RWDXTGL_H)
#define RWDXTGL_H

/****************************************************************************
 Includes
 */

/****************************************************************************
 Defines
 */
#define DDS_FOURCC                   0x00000004
#define DDS_RGB                      0x00000040
#define DDS_RGBA                     0x00000041
#define DDS_DEPTH                    0x00800000

#define DDSCAPS_COMPLEX              0x00000008
#define DDSCAPS_TEXTURE              0x00001000
#define DDSCAPS_MIPMAP               0x00400000

#define DDSCAPS2_CUBEMAP             0x00000200
#define DDSCAPS2_CUBEMAP_POSITIVEX   0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX   0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY   0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY   0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ   0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ   0x00008000
#define DDSCAPS2_VOLUME              0x00200000

#define FOURCC_DXT1 0x31545844 /* (MAKEFOURCC('D','X','T','1')) */
#define FOURCC_DXT3 0x33545844 /* (MAKEFOURCC('D','X','T','3')) */
#define FOURCC_DXT5 0x35545844 /* (MAKEFOURCC('D','X','T','5')) */


/****************************************************************************
 Global Types
 */

#if defined(__cplusplus)
extern "C"
{
#endif /* defined(__cplusplus) */

/****************************************************************************
 Globals
 */


/****************************************************************************
 Function prototypes
 */

extern RwBool
_rwOpenGLDecompressDXT1toARGB( const RwUInt32 width,
                               const RwUInt32 height,
                               RwUInt8 *destPixels,
                               RwUInt8 *srcPixels );

extern void
_rwOpenGLDecompressDXT3toARGB( const RwUInt32 width,
                               const RwUInt32 height,
                               RwUInt8 *destPixels,
                               RwUInt8 *srcPixels );

extern void
_rwOpenGLDecompressDXT5toARGB( const RwUInt32 width,
                               const RwUInt32 height,
                               RwUInt8 *destPixels,
                               RwUInt8 *srcPixels );

#if defined(__cplusplus)
}
#endif /* defined(__cplusplus) */

#endif /* !defined(RWDXTGL_H) */
