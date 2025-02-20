/***************************************************************************
 *                                                                         *
 * Module  : drvfns.h                                                      *
 *                                                                         *
 * Purpose : OpenGL general driver functionality                           *
 *                                                                         *
 **************************************************************************/

#if !defined(DRVFNS_H)
#define DRVFNS_H

/****************************************************************************
 Includes
 */

/* ---------------------------------------------------------------------
 *  Typedef : _rwOpenGLEnumExtensionsCallback
 *
 *  Purpose : _rwOpenGLEnumExtensionsCallback should be used to define callbacks to
 *            apply actions while enumerating OpenGL extensions.
 *
 *  On entry: extString Constant pointer to RwChar referring to the current
 *                      OpenGL extension string.
 *
 *            lastExt Constant RwBool that is TRUE if \e extString is the last
 *                    extension to be enumerated, or FALSE if not.
 *
 *            inParam Constant void pointer for application-defined read only
 *                    data.
 *
 *            outParam Void pointer for application-defined read/write data.
 *
 *  On exit : RwBool, TRUE if enumeration is to continue, FALSE if enumeration
 *            is to terminate.
 * --------------------------------------------------------------------- */
typedef RwBool (*_rwOpenGLEnumExtensionsCallback)( const RwChar * const extString,
                                                   const RwBool lastExt,
                                                   const void * const inParam,
                                                   void *outParam );

/* RWPUBLIC */

/****************************************************************************
 Function prototypes
 */

#if defined(__cplusplus)
extern "C"
{
#endif /* defined(__cplusplus) */


/* General useful functions */
extern RwUInt32
_rwOpenGLNextPowerOf2( RwUInt32 in );

extern RwBool
RwOpenGLIsExtensionSupported( const RwChar * const extString );

extern void
_rwOpenGLApplyRwMatrix( RwMatrix *matrix );

extern void
_rwOpenGLMatrixToRwMatrix( RwMatrix *outMatrix,
                           void *inMatrix );

extern RwInt32
_rwOpenGLGetMaxSupportedTextureSize( void );

extern RwInt32
_rwOpenGLGetMaxSupportedHardwareLights( void );

/* Win32 multiple display functionality and SSE support */
#if defined( _WIN32 )
extern RwBool
_rwOpenGLCameraAttachWindow(void *camera, void *window);

extern RwBool
_rwIntelSSEsupported(void);

extern RwBool
RwOpenGLChangeVideoMode( RwInt32 modeIndex );
#endif /* defined( _WIN32 ) */

/* Memory manager functions for the Vertex Array Range extension */
#define _rwOpenGLVertexHeapAvailable()  \
    (NULL != _rwOpenGLExt.VertexArrayRangeNV)

extern void
_rwOpenGLVertexHeapSetSize( const RwUInt32 size );

extern void *
_rwOpenGLVertexHeapDynamicMalloc( const RwUInt32 size,
                                  const RwBool generateFence );

extern void *
_rwOpenGLVertexHeapStaticMalloc( const RwUInt32 size );

extern void
_rwOpenGLVertexHeapDynamicFree( void *videoMemory );

extern void
_rwOpenGLVertexHeapStaticFree( void *videoMemory );

extern void
_rwOpenGLVertexHeapSetNVFence( void *videoMemory );

extern void
_rwOpenGLVertexHeapFinishNVFence( void *videoMemory );

extern void
_rwOpenGLVertexHeapDynamicDiscard( void *videoMemory );

/* RWPUBLICEND */

extern void
_rwOpenGLVertexHeapTestAndPerformResize( void );

extern void
_rwOpenGLEnumExtensions( _rwOpenGLEnumExtensionsCallback callBack,
                         const void * const inParam,
                         void *outParam );

#if defined(RWDEBUG)
extern RwBool  
_rwOpenGLDebugOutputExtensionsCB( const RwChar * const extString,
                                  const RwBool lastExt,
                                  const void * const inParam,
                                  void *outParam );
#endif /* defined(RWDEBUG) */

extern void
_rwOpenGLDestroyExtensionsString( void );

/* These functions are called internally by RwEngineStart/Stop respectively */
extern RwBool
_rwOpenGLVertexHeapCreate(void);
extern void
_rwOpenGLVertexHeapDestroy(void);

/* Called from RwEngineInit to give the driver a chance to register plugins */
extern RwBool
_rwDeviceRegisterPlugin(void);

/* RWPUBLIC */

#if defined(__cplusplus)
}
#endif /* defined(__cplusplus) */

/* RWPUBLICEND */

#endif /* !defined(DRVFNS_H) */
