/***********************************************************************
 *
 * Module:  openglpipepriv.h
 *
 * Purpose: OpenGL Pipe Private Structure
 *
 ***********************************************************************/

#if !defined( OPENGLPIPEPRIV_H )
#define OPENGLPIPEPRIV_H

/* =====================================================================
 *  Includes
 * ===================================================================== */


/* =====================================================================
 *  Defines
 * ===================================================================== */


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */
typedef struct _rxOpenGLAllInOnePrivateData _rxOpenGLAllInOnePrivateData;
struct _rxOpenGLAllInOnePrivateData
{
    /* instance both display lists and vertex arrays, or make a sensible choice? */
    RwBool                              instanceDLandVA;    

    /* per mesh instance callback */
    RxOpenGLAllInOneInstanceCallBack    instanceCB;

    /* per mesh reinstance callback */
    RxOpenGLAllInOneReinstanceCallBack  reinstanceCB;

    /* per atomic/worldsector lighting callback */
    RxOpenGLAllInOneLightingCallBack    lightingCB;

    /* per atomic/worldsector render callback */
    RxOpenGLAllInOneRenderCallBack      renderCB;
};


#if defined( __cplusplus )
extern "C"
{
#endif /* defined( __cplusplus ) */

/* =====================================================================
 *  Extern variables
 * ===================================================================== */

extern RwUInt32 _rwOpenGLPrimConvTbl[ rwPRIMTYPELINELIST |
                                      rwPRIMTYPEPOLYLINE |
                                      rwPRIMTYPETRILIST  |
                                      rwPRIMTYPETRISTRIP |
                                      rwPRIMTYPETRIFAN ];

extern RwOpenGLExtensions _rwOpenGLExt;

/* =====================================================================
 *  Extern function prototypes
 * ===================================================================== */

extern RwResEntry *
_rxOpenGLInstance( void *object,
                   void *owner,
                   RwResEntry **resEntryPtr,
                   RpMeshHeader *meshHeader,
                   const RwBool instanceDLandVA,
                   RxOpenGLAllInOneInstanceCallBack instanceCB );

extern RwBool
_rxOpenGLReinstance( void *object,
                     const RpMeshHeader *meshHeader,
                     RxOpenGLMeshInstanceData *instanceData,
                     const RwBool instanceDLandVA,
                     RxOpenGLAllInOneInstanceCallBack instanceCB );

extern void
_rxOpenGLDefaultAllInOneRenderCB( RwResEntry *repEntry,
                                  void *object,
                                  const RwUInt8 type,
                                  const RwUInt32 flags );

extern void
_rxOpenGLMeshGetNumVerticesAndMinIndex( RxVertexIndex *indices,
                                        RwUInt32 numIndices,
                                        RwUInt32 *numVertices,
                                        RwUInt32 *minIndex );

extern void
_rxOpenGLInstanceDisplayList( RxOpenGLMeshInstanceData *instanceData,
                              RpGeometryFlag geomFlags,
                              const RwUInt32 type,
                              RwInt32 numTexCoords,
                              RwTexCoords **srcTexCoord,
                              RwRGBA *srcColor,
                              void *srcNormal,
                              RwV3d *srcPosition );

extern RwBool
_rxOpenGLAllInOnePipelineInit( RxPipelineNode *node );

extern void
_rxOpenGLAllInOnePipelineTerm( RxPipelineNode *node );

extern RwBool
_rwOpenGLSetRenderState( RwRenderState state, void *param );

#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

#endif /* !defined( OPENGLPIPEPRIV_H ) */
