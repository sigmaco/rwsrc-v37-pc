/***************************************************************************
 *                                                                         *
 * Module  : barastgl.h                                                    *
 *                                                                         *
 * Purpose : OpenGL raster functionality                                   *
 *                                                                         *
 **************************************************************************/

#if !defined(RWRASTGL_H)
#define RWRASTGL_H

/****************************************************************************
 Includes
 */

/****************************************************************************
 Defines
 */
#define RWPIXELFORMAT2GL(_rasFmt)  \
    ( ((_rasFmt) & rwRASTERFORMATPIXELFORMATMASK) >> 8 )

#define RASTERCFORMAT2GL(_rasFmt)  \
    RWPIXELFORMAT2GL( (_rasFmt) << 8 )

/* handles up to 4096 * 4096 */
#define MAX_MIPLEVELS   (13)


/****************************************************************************
 Global Types
 */

typedef struct _rwOpenGLRasterExt _rwOpenGLRasterExt;
struct _rwOpenGLRasterExt
{
    /* OpenGL texture object */
    RwUInt32                textureID;

    /* texture's current address U mode */
    RwTextureAddressMode    addressModeU;
    
    /* texture's current address V mode */
    RwTextureAddressMode    addressModeV;
    
    /* texture's current filter mode */
    RwTextureFilterMode     filterMode;

    /* mipmap level currently locked */
    RwUInt8                 lockedMipLevel;

    /* for supporting multiple display windows */
    void                    *window;
    void                    *context;

    /* can we call glGetTexImage? */
    RwBool                  canFetchServerData[MAX_MIPLEVELS];

    /* internal texture format */
    RwUInt32                serverPixelFormat;

    /* 0 = not compressed, 1 = DXT1, 3 = DXT3, 5 = DXT5 */
    RwUInt32                dxtCompressedType;

    /* are we using hardware mip generation? */
    RwBool                  hardwareMipGen;

    /* is this a cubemap? */
    RwBool                  cubeMap;
};

#define RASTEREXTFROMRASTER(_raster)    \
    ((_rwOpenGLRasterExt *)(((RwUInt8 *)(_raster)) + _rwOpenGLRasterExtOffset))

typedef struct _rwOpenGLRasterFmt _rwOpenGLRasterFmt;
struct _rwOpenGLRasterFmt
{
    RwUInt32    colorDepth;

    RwUInt32    clientBytesPerPixel;

    RwUInt32    clientPixelFormat;
};

/* RWPUBLIC */

#if defined(__cplusplus)
extern "C"
{
#endif /* defined(__cplusplus) */

/* RWPUBLICEND */

/****************************************************************************
 Globals
 */

/* Raster extensions offset */
extern RwInt32              _rwOpenGLRasterExtOffset;

extern _rwOpenGLRasterFmt   _rwOpenGLRasterFormatInfo[];


/****************************************************************************
 Function prototypes
 */

extern RwBool
RwFexist( const RwChar * name );

extern RwBool
_rwOpenGLRasterOpen( void );

extern void
_rwOpenGLRasterClose( void );

extern RwBool
_rwOpenGLRasterSetFormat( RwRaster *raster,
                          RwInt32 flags );

extern RwInt32
_rwOpenGLDetermineMaxMipLevel( RwInt32 width,
                               RwInt32 height );

extern RwBool
_rwOpenGLDescribeMipLevel( RwInt32 mipLevel,
                           RwInt32 width,
                           RwInt32 height,
                           RwInt32 bytesPerPixel,
                           RwInt32 *mipWidth,
                           RwInt32 *mipHeight,
                           RwInt32 *stride );

extern RwUInt32
_rwOpenGLRasterGetDXTRasterSize( const RwUInt32 width,
                                 const RwUInt32 height,
                                 const RwUInt32 dxtCompressedType );

/* RWPUBLIC */

extern void *
RwOpenGLRasterStreamReadDDS( RwStream *stream );

extern void *
RwOpenGLDDSTextureRead( const RwChar *name,
                        const RwChar *maskname );

extern void
RwOpenGLRasterSetGenericTextureCompression( const RwBool enable );

extern RwBool
RwOpenGLRasterGetGenericTextureCompression( void );

extern RwBool
RwOpenGLRasterIsCubeMap( void *rasterVoid );

#if defined(__cplusplus)
}
#endif /* defined(__cplusplus) */

/* RWPUBLICEND */

#endif /* !defined(RWRASTGL_H) */
