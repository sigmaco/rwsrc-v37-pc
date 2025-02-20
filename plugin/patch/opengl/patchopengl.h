/***********************************************************************
 *
 * Module:  patchopengl.h
 *
 * Purpose: 
 *
 ***********************************************************************/

#if !defined( PATCHOPENGL_H )
#define PATCHOPENGL_H

/* =====================================================================
 *  Includes
 * ===================================================================== */


/* =====================================================================
 *  Defines
 * ===================================================================== */
#define _rpPatchOpenGLPipeline(_idx)   \
    (_rpPatchGlobals.platform.pipelines[(_idx) - 1])


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */
struct PatchPlatform
{
    RxPipeline  *pipelines[rpPATCHOPENGLPIPELINEMAX - 1];
};
typedef struct PatchPlatform PatchPlatform;


/* =====================================================================
 *  Extern variables
 * ===================================================================== */
extern RwUInt8 *openglPatchSkinInterleavedArrays;


/* =====================================================================
 *  Extern function prototypes
 * ===================================================================== */

#if defined( __cplusplus )
extern "C"
{
#endif /* defined( __cplusplus ) */

extern RpAtomic*
_rpPatchOpenGLAtomicRenderCB( RpAtomic *atomic );

extern RxPipeline *
_rpPatchOpenGLCreateGenericPipe( void );

extern RwUInt32
_rpPatchOpenGLGetNumVerticesFromResEntry( const RwResEntry *resEntry );

#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

#endif /* !defined( PATCHOPENGL_H ) */

