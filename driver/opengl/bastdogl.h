/***************************************************************************
 *                                                                         *
 * Module  : bastdogl.h                                                    *
 *                                                                         *
 * Purpose : Standard function handling                                    *
 *                                                                         *
 **************************************************************************/

#ifndef RWSTDOGL_H
#define RWSTDOGL_H

/****************************************************************************
 Includes
 */
#include "badevice.h" /* For RwStandardFunc */


/****************************************************************************
 Function prototypes
 */

#ifdef    __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* INSTALL STANDARD FUNCTIONS */
extern void   
_rwOpenGLSetStandards(RwStandardFunc *funcs, RwInt32 numFuncs);

/* CAMERA OPERATIONS */
extern RwBool 
_rwOpenGLCameraClear(void *inOut, void *in, RwInt32 colour);
extern RwBool 
_rwOpenGLCameraBeginUpdate(void *out,void *camera, RwInt32 in);
extern RwBool 
_rwOpenGLCameraEndUpdate(void *out,void *camera, RwInt32 in);

/* RASTER <-> IMAGE CONVERSIONS */
extern RwBool 
_rwOpenGLImageSetRaster( RwImage *image,
                         RwRaster *raster,
                         RwInt32 flags );

extern RwBool 
_rwOpenGLRasterSetImage( RwRaster *raster,
                         RwImage *image,
                         RwInt32 flags );

extern RwBool 
_rwOpenGLRGBToPixel( RwUInt32 *pixelColor,
                     RwRGBA *RGBAcolor,
                     RwInt32 rasterFormat );

extern RwBool 
_rwOpenGLPixelToRGB( RwRGBA *RGBAcolor,
                     RwUInt32 *pixelColor,
                     RwInt32 rasterFormat );

extern RwBool 
_rwOpenGLImageFindRasterFormat( RwRaster *raster,
                                RwImage *image,
                                RwInt32 flags );

/* RASTER BLITTING OPERATIONS */
extern RwBool 
_rwOpenGLSetRasterContext( void *inOut,
                           RwRaster *raster,
                           RwInt32 flags );

extern RwBool
_rwOpenGLRasterClear( void *inOut,
                      void *in,
                      RwInt32 pixelColor );

extern RwBool 
_rwOpenGLRasterClearRect( void *inOut,
                          RwRect *rect,
                          RwInt32 pixelColor );

extern RwBool 
_rwOpenGLRasterRender( RwRaster *raster,
                       RwRect *rect,
                       RwInt32 flags );

extern RwBool 
_rwOpenGLRasterRenderFast( RwRaster *raster,
                           RwRect *rect, 
                           RwInt32 flags );

extern RwBool 
_rwOpenGLRasterRenderScaled( RwRaster *raster,
                             RwRect *rect,
                             RwInt32 flags );

/* MATIVE TEXTURE SERIALISAITON */
extern RwBool 
_rwOpenGLNativeTextureGetSize( RwUInt32 *size,
                               RwTexture *texture,
                               RwInt32 flags );

extern RwBool 
_rwOpenGLNativeTextureRead( RwStream *stream,
                            RwTexture **texture,
                            RwUInt32 size );

extern RwBool 
_rwOpenGLNativeTextureWrite( RwStream *stream,
                             RwTexture *texture,
                             RwUInt32 length );


/* GENERAL RASTER OPERATIONS */
extern RwBool 
_rwOpenGLRasterCreate( void *inOut,
                       RwRaster *raster,
                       RwInt32 flags );

extern RwBool 
_rwOpenGLRasterDestroy( void *inOut,
                        RwRaster *raster,
                        RwInt32 flags );

extern RwBool 
_rwOpenGLRasterLock( RwUInt8 **pixels,
                     RwRaster *raster,
                     RwInt32 access );

extern RwBool 
_rwOpenGLRasterUnlock( void *inOut,
                       RwRaster *raster,
                       RwInt32 flags );

extern RwBool 
_rwOpenGLRasterLockPalette( RwUInt8 **palette,
                            RwRaster *raster,
                            RwInt32 accessMode );

extern RwBool 
_rwOpenGLRasterUnlockPalette( void *inOut,
                              RwRaster *raster,
                              RwInt32 flags );

extern RwBool 
_rwOpenGLRasterGetMipLevels( RwInt32 *numMipLevels,
                             RwRaster *raster,
                             RwInt32 flags );

extern RwBool  
_rwOpenGLRasterShowRaster(void *inOut, void *inA, RwInt32 inB);

extern RwBool 
_rwOpenGLRasterSubRaster( RwRaster *subRaster,
                          RwRaster *raster,
                          RwInt32 flags );

/* GENERAL STANDARD FUNCTIONS */
extern RwBool 
_rwOpenGLHintRenderFront2Back(void *pInOut, void *pIn, RwInt32 flags);

extern RwBool 
_rwOpenGLTextureSetRaster( RwTexture *texture,
                           RwRaster *raster,
                           RwInt32 flags );

#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#endif /* RWSTDOGL_H */



