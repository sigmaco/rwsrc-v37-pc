/***********************************************************************
 *
 * Module:  teamskinopengl.h
 *
 * Purpose:
 *
 ***********************************************************************/

#if !defined( TEAMSKINOPENGL_H )
#define TEAMSKINOPENGL_H

/* =====================================================================
 *  Includes
 * ===================================================================== */


/* =====================================================================
 *  Defines
 * ===================================================================== */


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */


#if defined( __cplusplus )
extern "C"
{
#endif /* defined( __cplusplus ) */

/* =====================================================================
 *  Extern variables
 * ===================================================================== */
extern RxOpenGLAllInOneReinstanceCallBack   _rpTeamOpenGLSkinDefaultReinstanceCB;


/* =====================================================================
 *  Extern function prototypes
 * ===================================================================== */

extern RwMatrix *
_rpTeamSkinMatBlendUpdating( RpSkin *skin,
                             RwFrame *frame,
                             RpHAnimHierarchy *hierarchy,
                             RwMatrix *matrixArray );

extern RxPipeline *
_rpSkinOpenGLPipelineCreate( const RpSkinType type,
                             RxOpenGLAllInOneRenderCallBack renderCB );

extern RwBool
_rpTeamOpenGLSkinAllInOneAtomicReinstanceCB( void *object,
                                             RwResEntry *resEntry,
                                             const RpMeshHeader *meshHeader,
                                             const RwBool instanceDLandVA,
                                             RxOpenGLAllInOneInstanceCallBack instanceCB );

extern void
_rpTeamOpenGLSkinAllInOneRenderCB( RwResEntry *repEntry,
                                   void *object,
                                   const RwUInt8 type,
                                   const RwUInt32 flags );

#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

#endif /* !defined( TEAMSKINOPENGL_H ) */
