/***********************************************************************
 *
 * Module:  openglpipe.h
 *
 * Purpose: OpenGL Pipe Structure
 *
 ***********************************************************************/

#if !defined( OPENGLPIPE_H )
#define OPENGLPIPE_H

/* =====================================================================
 *  Includes
 * ===================================================================== */
#include "bamesh.h"


/* =====================================================================
 *  Defines
 * ===================================================================== */

/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */

/* RWPUBLIC */

typedef struct RxOpenGLResEntryHeader RxOpenGLResEntryHeader;
/**
 * \ingroup worldextensionsopengl
 * \struct RxOpenGLResEntryHeader
 * This structure contains OpenGL specific resource entry data.
 */
struct RxOpenGLResEntryHeader
{
    RwUInt16    serialNumber;   /**< The mesh header serial number */
    RwUInt16    numMeshes;      /**< The number of meshes */
};


typedef struct RxOpenGLMeshInstanceData RxOpenGLMeshInstanceData;
/**
 * \ingroup worldextensionsopengl
 * \struct RxOpenGLMeshInstanceData
 * This structure contains the information necessary to describe an instanced
 * mesh using OpenGL.
 */
struct RxOpenGLMeshInstanceData
{
    RwUInt32        primType;           /**< OpenGL primitive type */

    RwUInt32        vertexDesc;         /**< Vertex description - equivalent to
                                          *  \ref RpGeometryFlag or \ref
                                          *  RpWorldFlag */

    RwUInt32        vertexStride;       /**< Stride of the vertex data
                                          *  in bytes */

    RwUInt32        numVertices;        /**< Number of vertices in the mesh */

    RwUInt32        vertexDataSize;     /**< Size of the vertex data in bytes */

    RwUInt8         *sysMemVertexData;  /**< Vertex data in system memory,
                                          *  NULL if not used */

    void            *vidMemVertexData;  /**< Vertex data in video memory (fast
                                          *  write-only memory), NULL if not
                                          *  used */

    RwBool          vidMemDataIsStatic; /**< If vertex data is in video memory,
                                          *  TRUE if a static block, FALSE if
                                          *  dynamic */

    RwUInt32        displayList;        /**< OpenGL display list name, 0 if
                                          *  there is no display list */

    RwUInt32        vaoName;            /**< Name of the ATI Vertex Array
                                          *  object, or 0 if there is no VAO */

    RwUInt32        minVertexIdx;       /**< Minimum vertex index */

    RwUInt32        numIndices;         /**< Number of indices in the mesh */

    RxVertexIndex   *indexData;         /**< Index data array */

    RwBool          remapVertexData;    /**< Vertex data was remapped to reduce
                                          *  vertex array sizes */

    RpMaterial      *material;          /**< Pointer to the mesh's material*/

    RwBool          vertexAlpha;        /**< TRUE if there are vertices in this
                                          *  mesh with alpha < 255, FALSE
                                          *  otherwise */

    RwUInt8         *position;          /**< \ref RwUInt8 pointer containing
                                          *  the address, or offset, of the
                                          *  start of the position data for 
                                          *  this mesh */

    RwUInt8         *normal;            /**< \ref RwUInt8 pointer containing
                                          *  the address, or offset, of the
                                          *  start of the normal data for this
                                          *  mesh */

    RwUInt8         *color;             /**< \ref RwUInt8 pointer containing
                                          *  the address, or offset, of the
                                          *  start of the color data for this
                                          *  mesh */

    RwUInt8         *texCoord[rwMAXTEXTURECOORDS];
                                        /**< Array of \ref RwUInt8 pointers
                                          *  containing the address, or offset,
                                          *  of each texture coordinate set
                                          *  for this mesh */
};


/**
 * \ingroup worldextensionsopengl
 * \ref RxOpenGLAllInOneInstanceCallBack callback function to instance a mesh.
 * 
 * \param object Void pointer to the original object (atomic or world sector).
 *
 * \param instanceData Pointer to \ref RxOpenGLMeshInstanceData containing 
 * instance data.
 *
 * \param instanceDLandVA Constant \ref RwBool, TRUE to (potentially) 
 * instance both display lists and vertex arrays, or FALSE to instance the 
 * most suitable type.
 *
 * \param reinstance Constant \ref RwBool indicating whether an instance or a reinstance
 *        should occur.
 *
 * \return RwBool, TRUE if successfully instanced the mesh, FALSE otherwise.
 */
typedef RwBool (*RxOpenGLAllInOneInstanceCallBack)( void *object,
                                                    RxOpenGLMeshInstanceData *instanceData,
                                                    const RwBool instanceDLandVA,
                                                    const RwBool reinstance );

/**
 * \ingroup worldextensionsopengl
 * \ref RxOpenGLAllInOneReinstanceCallBack callback function to reinstance a mesh.
 * 
 * \param object Void pointer to the original object (atomic or world sector).
 *
 * \param resEntry Pointer to the \ref RwResEntry of the last instance of \e object.
 *
 * \param meshHeader Pointer to constant \ref RpMeshHeader describing the meshes to reinstance.
 *
 * \param instanceDLandVA Constant \ref RwBool, TRUE to (potentially) instance both display lists
 *        and vertex arrays, or FALSE to instance the most suitable type.
 *
 * \param instanceCB Pointer to \ref RxOpenGLAllInOneInstanceCallBack used to execute
 *        the reinstancing.
 *
 * \return RwBool, TRUE if successfully reinstanced the mesh, FALSE otherwise.
 */
typedef RwBool (*RxOpenGLAllInOneReinstanceCallBack)( void *object,
                                                      RwResEntry *resEntry,
                                                      const RpMeshHeader *meshHeader,
                                                      const RwBool instanceDLandVA,
                                                      RxOpenGLAllInOneInstanceCallBack instanceCB );

/**
 * \ingroup worldextensionsopengl
 * \ref RxOpenGLAllInOneLightingCallBack callback function to apply lighting 
 * to an object.
 * 
 * \param object Void pointer to the original object (atomic or world sector).
 *
 */
typedef void (*RxOpenGLAllInOneLightingCallBack)( void *object );

/**
 * \ingroup worldextensionsopengl
 * \ref RxOpenGLAllInOneRenderCallBack callback function to render an object.
 * 
 * \param repEntry Pointer to the \ref RwResEntry of the last instance of \e object.
 *
 * \param object Void pointer to the original object (atomic or world sector).
 *
 * \param type Constant \ref RwUInt32 containing rpATOMIC or rwSECTORATOMIC,
 *        depending on what the \e object is.
 *
 * \param flags Constant \ref RwUInt32 containing \ref RpGeometryFlag's or \ref RpWorldFlag's,
 *        depending on what the \e object is.
 *
 */
typedef void (*RxOpenGLAllInOneRenderCallBack)( RwResEntry *repEntry,
                                                void *object,
                                                const RwUInt8 type,
                                                const RwUInt32 flags );

#if defined( __cplusplus )
extern "C"
{
#endif /* defined( __cplusplus ) */

/* =====================================================================
 *  Extern variables
 * ===================================================================== */

/* =====================================================================
 *  Extern function prototypes
 * ===================================================================== */

/* instance callbacks */
extern void
RxOpenGLAllInOneSetInstanceCallBack( RxPipelineNode *node,
                                     RxOpenGLAllInOneInstanceCallBack instanceCB );

extern RxOpenGLAllInOneInstanceCallBack
RxOpenGLAllInOneGetInstanceCallBack( RxPipelineNode *node );

/* reinstance callbacks */
extern void
RxOpenGLAllInOneSetReinstanceCallBack( RxPipelineNode *node,
                                       RxOpenGLAllInOneReinstanceCallBack reinstanceCB );

extern RxOpenGLAllInOneReinstanceCallBack
RxOpenGLAllInOneGetReinstanceCallBack( RxPipelineNode *node );

/* lighting callbacks */
extern void
RxOpenGLAllInOneSetLightingCallBack( RxPipelineNode *node,
                                     RxOpenGLAllInOneLightingCallBack lightingCB );

extern RxOpenGLAllInOneLightingCallBack
RxOpenGLAllInOneGetLightingCallBack( RxPipelineNode *node );

/* render callbacks */
extern void
RxOpenGLAllInOneSetRenderCallBack( RxPipelineNode *node,
                                   RxOpenGLAllInOneRenderCallBack renderCB );

extern RxOpenGLAllInOneRenderCallBack
RxOpenGLAllInOneGetRenderCallBack( RxPipelineNode *node );

/* instancing both display lists and vertex arrays */
extern void
RxOpenGLAllInOneSetInstanceDLandVA( RxPipelineNode *node,
                                    const RwBool instanceDLandVA );

extern RwBool
RxOpenGLAllInOneGetInstanceDLandVA( RxPipelineNode *node );

#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

/* RWPUBLICEND */

#endif /* !defined( OPENGLPIPE_H ) */
