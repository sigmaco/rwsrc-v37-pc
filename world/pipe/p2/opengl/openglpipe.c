/***********************************************************************
 *
 * Module:  openglpipe.c
 *
 * Purpose: OpenGL World Pipes
 *
 ***********************************************************************/

/* =====================================================================
 *  Includes
 * ===================================================================== */
#include <string.h> /* for memset() */

#if defined(WIN32)
#include <windows.h>
#endif /* defined(WIN32) */
#include <gl/gl.h>

#include "baworld.h"
#include "openglpipe.h"
#include "openglpipepriv.h"
#include "opengllights.h"

#if defined(macintosh)
#define min(_a,_b)  ( ((_a) <= (_b)) ? (_a) : (_b) )
#define max(_a,_b)  ( ((_a) > (_b)) ? (_a) : (_b) )
#endif /* defined(macintosh) */


/* =====================================================================
 *  Defines
 * ===================================================================== */
#if defined(RWMETRICS)
#define UPDATERENDERCBMETRICS(_instData)                    \
MACRO_START                                                 \
    switch ( (_instData)->primType )                        \
    {                                                       \
    case GL_TRIANGLES:                                      \
        {                                                   \
            if ( NULL != (_instData)->indexData )           \
            {                                               \
                RWSRCGLOBAL(metrics)->numProcTriangles +=   \
                    (_instData)->numIndices / 3;            \
            }                                               \
            else                                            \
            {                                               \
                RWSRCGLOBAL(metrics)->numProcTriangles +=   \
                    (_instData)->numVertices / 3;           \
            }                                               \
        }                                                   \
        break;                                              \
                                                            \
    case GL_TRIANGLE_STRIP:                                 \
    case GL_TRIANGLE_FAN:                                   \
        {                                                   \
            if ( NULL != (_instData)->indexData )           \
            {                                               \
                RWSRCGLOBAL(metrics)->numProcTriangles +=   \
                    (_instData)->numIndices - 2;            \
            }                                               \
            else                                            \
            {                                               \
                RWSRCGLOBAL(metrics)->numProcTriangles +=   \
                    (_instData)->numVertices - 2;           \
            }                                               \
        }                                                   \
        break;                                              \
    }                                                       \
MACRO_STOP
#else /* defined(RWMETRICS) */
#define UPDATERENDERCBMETRICS(_instData)
#endif /* defined(RWMETRICS) */


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */


/* =====================================================================
 *  Static variables
 * ===================================================================== */
/**
 * \ingroup worldextensionsrestrictionsopengl
 * \page openglgeneralrestrictions Restrictions of OpenGL RenderWare Graphics
 *
 * In this section we describe the known restrictions of OpenGL RenderWare Graphics
 * for retained mode use.
 *
 * \li Any non-opaque prelight alpha in vertex data is honoured when there is no
 * dynamic lighting in a world that holds the geometry. However, when dynamic lights
 * are used, all prelight alpha is rendered opaque. OpenGL only supports alpha
 * in the diffuse material component when dynamic lighting is enabled.
 *
 * \li \ref RpAtomicInstance and \ref RpWorldInstance are not supported.
 *
 */

static RwUInt32 openglAllInOneInitRefCount = 0;


/* =====================================================================
 *  Global variables
 * ===================================================================== */
RwUInt32 _rwOpenGLPrimConvTbl[ rwPRIMTYPELINELIST |
                               rwPRIMTYPEPOLYLINE |
                               rwPRIMTYPETRILIST  |
                               rwPRIMTYPETRISTRIP |
                               rwPRIMTYPETRIFAN ];


/* =====================================================================
 *  Static function prototypes
 * ===================================================================== */
static void
openglResEntryDestructor( RwResEntry *resEntry );

static void
openglInstanceDataDestructor( RxOpenGLMeshInstanceData *instanceData );


/* =====================================================================
 *  Global function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: _rxOpenGLInstance
 *
 *  Purpose : OpenGL instancing.
 *
 *  On entry: object - Void pointer to the object to instance.
 *
 *            owner - Void pointer to the owner of the object.
 *
 *            resEntryPtr - Double pointer to RwResEntry.
 *
 *            meshHeader - Pointer to RpMeshHeader.
 *
 *            instanceDLandVA - Constant RwBool.
 *
 *            instanceCB - RxOpenGLAllInOneInstanceCallBack function callback.
 *
 *  On exit : Pointer to RwResEntry or NULL if failed.
 * --------------------------------------------------------------------- */
RwResEntry *
_rxOpenGLInstance( void *object,
                   void *owner,
                   RwResEntry **resEntryPtr,
                   RpMeshHeader *meshHeader,
                   const RwBool instanceDLandVA,
                   RxOpenGLAllInOneInstanceCallBack instanceCB )
{
    RwUInt32                    size;

    RwResEntry                  *resEntry;

    RxOpenGLResEntryHeader      *resEntryHeader;

    RxOpenGLMeshInstanceData    *instanceData;

    RwUInt16                    numMeshes;

    RpMesh                      *currentMesh;

    RwUInt32                    unindexedFirstVertex;


    RWFUNCTION( RWSTRING( "_rxOpenGLInstance" ) );

    RWASSERT( NULL != meshHeader );

    /* calculate the size of the resource entry */
    size = sizeof(RxOpenGLResEntryHeader);
    size += sizeof(RxOpenGLMeshInstanceData) * meshHeader->numMeshes;

    /* allocate the resource entry */
    resEntry = RwResourcesAllocateResEntry( owner,
                                            resEntryPtr,
                                            size,
                                            openglResEntryDestructor );
    if ( NULL == resEntry )
    {
        RWRETURN( resEntry );
    }

    resEntryHeader = (RxOpenGLResEntryHeader *)(resEntry + 1);

    /* set up the resource entry header */
    resEntryHeader->serialNumber = meshHeader->serialNum;
    resEntryHeader->numMeshes = meshHeader->numMeshes;

    /* get the first RxOpenGLMeshInstanceData pointer */
    instanceData = (RxOpenGLMeshInstanceData *)(resEntryHeader + 1);

    numMeshes = meshHeader->numMeshes;
    currentMesh = (RpMesh *)(meshHeader + 1);

    unindexedFirstVertex = 0;

    while ( numMeshes-- )
    {
        RwInt32 idx;


        /* initialize the instance data to something sensible */
        instanceData->primType         = _rwOpenGLPrimConvTbl[RpMeshHeaderGetPrimType(meshHeader)];
        instanceData->vertexDesc       = 0;
        instanceData->vertexStride     = 0;
        instanceData->numVertices      = 0;
        instanceData->vertexDataSize   = 0;
        instanceData->sysMemVertexData = (RwUInt8 *)NULL;
        instanceData->vidMemVertexData = (void *)NULL;
        instanceData->displayList      = 0;
        instanceData->vaoName          = 0;
        instanceData->minVertexIdx     = 0;
        instanceData->numIndices       = 0;
        instanceData->indexData        = (RxVertexIndex *)NULL;
        instanceData->remapVertexData  = FALSE;
        instanceData->material         = currentMesh->material;
        instanceData->vertexAlpha      = FALSE;
        instanceData->position         = (RwUInt8 *)NULL;
        instanceData->normal           = (RwUInt8 *)NULL;
        instanceData->color            = (RwUInt8 *)NULL;
        for ( idx = 0; idx < rwMAXTEXTURECOORDS; idx += 1 )
        {
            instanceData->texCoord[idx] = (RwUInt8 *)NULL;
        }

        /* allocate index memory and fill */
        if ( 0 != (meshHeader->flags & rpMESHHEADERUNINDEXED) )
        {
            RWASSERT( 0 == currentMesh->numIndices );

            instanceData->numIndices   = 0;
            instanceData->indexData    = (RxVertexIndex *)NULL;

            instanceData->numVertices  = currentMesh->numIndices;
            instanceData->minVertexIdx = unindexedFirstVertex;

            unindexedFirstVertex += instanceData->numVertices;
        }
        else
        {
            instanceData->numIndices = currentMesh->numIndices;

            _rxOpenGLMeshGetNumVerticesAndMinIndex( currentMesh->indices,
                                                    currentMesh->numIndices,
                                                    &instanceData->numVertices,
                                                    &instanceData->minVertexIdx );

            instanceData->indexData = (RxVertexIndex *)RwMalloc( sizeof(RxVertexIndex) * instanceData->numIndices,
                                                                 rwMEMHINTDUR_EVENT | rwID_WORLDPIPEMODULE);
            RWASSERT( NULL != instanceData->indexData );
            if ( NULL == instanceData->indexData )
            {
                RWERROR( (E_RW_NOMEM, sizeof(RxVertexIndex) * instanceData->numIndices) );

                /* destroy the resource entry */
                RwResourcesFreeResEntry( resEntry );
                resEntry = (RwResEntry *)NULL;

                RWRETURN( resEntry );
            }

            if ( 0 == instanceData->minVertexIdx )
            {
                memcpy( instanceData->indexData,
                        currentMesh->indices,
                        sizeof(RxVertexIndex) * instanceData->numIndices );
            }
            else
            {
                RxVertexIndex   *srcIndex;

                RxVertexIndex   *dstIndex;

                RwUInt32        numIndices;


                srcIndex = currentMesh->indices;
                dstIndex = instanceData->indexData;

                numIndices = instanceData->numIndices;

                while ( numIndices-- )
                {
                    *dstIndex = (RxVertexIndex)(*srcIndex - instanceData->minVertexIdx);

                    srcIndex += 1;
                    dstIndex += 1;
                }
            }
        }

        /* call the instance callback */
        if ( NULL != instanceCB )
        {
            if ( FALSE == instanceCB( object, instanceData, instanceDLandVA, FALSE ) )
            {
                /* destroy the resource entry */
                RwResourcesFreeResEntry( resEntry );
                resEntry = (RwResEntry *)NULL;

                RWRETURN( resEntry );
            }
        }

        /* next RpMesh and RxOpenGLMeshInstanceData pointers */
        currentMesh += 1;
        instanceData += 1;
    }

    RWRETURN( resEntry );
}


/* ---------------------------------------------------------------------
 *  Function: _rxOpenGLReinstance
 *
 *  Purpose : OpenGL reinstancing.
 *
 *  On entry: object - Void pointer to the object to instance.
 *
 *            meshHeader - Pointer to constant RpMeshHeader.
 *
 *            instanceData - Pointer to RxOpenGLMeshInstanceData.
 *
 *            instanceDLandVA - Constant RwBool
 *
 *            instanceCB - RxOpenGLAllInOneInstanceCallBack function callback.
 *
 *  On exit : RwBool, TRUE if successfully reinstanced, FALSE if not.
 * --------------------------------------------------------------------- */
RwBool
_rxOpenGLReinstance( void *object,
                     const RpMeshHeader *meshHeader,
                     RxOpenGLMeshInstanceData *instanceData,
                     const RwBool instanceDLandVA,
                     RxOpenGLAllInOneInstanceCallBack instanceCB )
{
    RWFUNCTION( RWSTRING( "_rxOpenGLReinstance" ) );

    if ( NULL != instanceCB )
    {
        RwUInt16    numMeshes;


        numMeshes = meshHeader->numMeshes;

        while ( numMeshes-- )
        {
            if ( FALSE == instanceCB( object, instanceData, instanceDLandVA, TRUE ) )
            {
                RWRETURN( FALSE );
            }

            /* next RxOpenGLMeshInstanceData pointer */
            instanceData += 1;
        }
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rxOpenGLDefaultAllInOneRenderCB
 *
 *  Purpose : Default OpenGL render callback.
 *
 *  On entry: repEntry - Pointer to RwResEntry.
 *
 *            object - Void pointer to the object.
 *
 *            type - Constant RwUInt32 containing rpATOMIC or rwSECTORATOMIC.
 *
 *            flags - Constant RwUInt32 containing RpGeometryFlag's or RpWorldFlag's.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
void
_rxOpenGLDefaultAllInOneRenderCB( RwResEntry *repEntry,
                                  void *object __RWUNUSED__,
                                  const RwUInt8 type __RWUNUSED__,
                                  const RwUInt32 flags )
{
    RxOpenGLResEntryHeader      *resEntryHeader;

    RxOpenGLMeshInstanceData    *instanceData;

    RwUInt16                    numMeshes;


    RWFUNCTION( RWSTRING( "_rxOpenGLDefaultAllInOneRenderCB" ) );

    /* get the instanced data */
    resEntryHeader = (RxOpenGLResEntryHeader *)(repEntry + 1);
    RWASSERT( NULL != resEntryHeader );

    instanceData = (RxOpenGLMeshInstanceData *)(resEntryHeader + 1);
    RWASSERT( NULL != instanceData );

    numMeshes = resEntryHeader->numMeshes;
    while ( numMeshes-- )
    {
        const RwRGBA    *matColor;

        RwBool          vertexAlphaEnabled;


        RWASSERT( NULL != instanceData->material );

        matColor = RpMaterialGetColor( instanceData->material );
        RWASSERT( NULL != matColor );

        if ( (FALSE != instanceData->vertexAlpha) ||
             (0xFF != matColor->alpha) )
        {
            vertexAlphaEnabled = TRUE;
        }
        else
        {
            vertexAlphaEnabled = FALSE;
        }
        _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)vertexAlphaEnabled );

        if ( FALSE != RwOpenGLIsEnabled( rwGL_LIGHTING ) )
        {
            /* set up the material properties */
            RpOpenGLWorldSetMaterialProperties( instanceData->material,
                                                flags );
        }
        else
        {
            RwOpenGLDisable( rwGL_COLOR_MATERIAL );
            if ( 0 == (flags & rxGEOMETRY_PRELIT) )
            {
                /* black */
                glColor4f( 0.0f, 0.0f, 0.0f, 1.0f );
            }
        }

        /* bind or unbind textures */
        if ( 0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2)) )
        {
            RwOpenGLSetTexture( RpMaterialGetTexture( instanceData->material ) );
        }
        else
        {
            RwOpenGLSetTexture( NULL );
        }

        if ( 0 != instanceData->displayList )
        {
            RWASSERT( GL_FALSE != glIsList( instanceData->displayList ) );

            /* execute the display list */
            glCallList( instanceData->displayList );
        }
        else if ( (NULL != _rwOpenGLExt.ArrayObjectATI) &&
                  (GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName )) )
        {
            RWASSERT( GL_FALSE !=
                      _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName ) );

            /* set up client state pointers */
            RwOpenGLVASetPositionATI( 3,
                                      GL_FLOAT,
                                      instanceData->vertexStride,
                                      instanceData->vaoName,
                                      instanceData->position );

            RwOpenGLVASetNormalATI( (0 != (flags & rxGEOMETRY_NORMALS)),
                                    GL_FLOAT,
                                    instanceData->vertexStride,
                                    instanceData->vaoName,
                                    instanceData->normal );

            RwOpenGLVASetColorATI( (0 != (flags & rxGEOMETRY_PRELIT)),
                                   ((FALSE == vertexAlphaEnabled) ? 3 : 4),
                                   GL_UNSIGNED_BYTE,
                                   instanceData->vertexStride,
                                   instanceData->vaoName,
                                   instanceData->color );

            RwOpenGLVASetTexCoordATI( (0 != (flags & (rxGEOMETRY_TEXTURED |
                                                      rpGEOMETRYTEXTURED2))),
                                      2,
                                      GL_FLOAT,
                                      instanceData->vertexStride,
                                      instanceData->vaoName,
                                      instanceData->texCoord[0] );

            /* render */
            if ( NULL != instanceData->indexData )
            {
                glDrawElements( instanceData->primType,
                                instanceData->numIndices,
                                GL_UNSIGNED_INT,
                                (const GLvoid *)(instanceData->indexData) );
            }
            else
            {
                glDrawArrays( instanceData->primType,
                              instanceData->minVertexIdx,
                              instanceData->numVertices );
            }
        }
        else
        {
            /* set up client state pointers */
            RwOpenGLVASetPosition( 3,
                                   GL_FLOAT,
                                   instanceData->vertexStride,
                                   instanceData->position );

            RwOpenGLVASetNormal( (0 != (flags & rxGEOMETRY_NORMALS)),
                                 GL_FLOAT,
                                 instanceData->vertexStride,
                                 instanceData->normal );

            RwOpenGLVASetColor( (0 != (flags & rxGEOMETRY_PRELIT)),
                                ((FALSE == vertexAlphaEnabled) ? 3 : 4),
                                GL_UNSIGNED_BYTE,
                                instanceData->vertexStride,
                                instanceData->color );

            RwOpenGLVASetTexCoord( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))),
                                   2,
                                   GL_FLOAT,
                                   instanceData->vertexStride,
                                   instanceData->texCoord[0] );

            /* render */
            if ( NULL != instanceData->indexData )
            {
                glDrawElements( instanceData->primType,
                                instanceData->numIndices,
                                GL_UNSIGNED_INT,
                                (const GLvoid *)(instanceData->indexData) );
            }
            else
            {
                glDrawArrays( instanceData->primType,
                              instanceData->minVertexIdx,
                              instanceData->numVertices );
            }

            /* video memory specific additions */
            if ( NULL != instanceData->vidMemVertexData )
            {
                if ( _rwOpenGLVertexHeapAvailable() )
                {
                    _rwOpenGLVertexHeapSetNVFence( instanceData->vidMemVertexData );
                }
            }
        }

        UPDATERENDERCBMETRICS(instanceData);

        /* next RxOpenGLMeshInstanceData pointer */
        instanceData += 1;
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rxOpenGLMeshGetNumVerticesAndMinIndex
 *
 *  Purpose : Calculates the number of vertices and the minimum index
 *            from an RpMesh.
 *
 *  On entry: indices     - Pointer to RxVertexIndex array.
 *
 *            numIndices  - RwUInt32, containing the number of indices.
 *
 *            numVertices - Pointer to RwUInt32 referring to the return
 *                          value for the number of vertices.
 *
 *            minIndex    - Pointer to RwUInt32 referring to the return
 *                          value for the minimum index.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
void
_rxOpenGLMeshGetNumVerticesAndMinIndex( RxVertexIndex *indices,
                                        RwUInt32 numIndices,
                                        RwUInt32 *numVertices,
                                        RwUInt32 *minIndex )
{
    RWFUNCTION( RWSTRING( "_rxOpenGLMeshGetNumVerticesAndMinIndex" ) );

    RWASSERT( NULL != numVertices );
    RWASSERT( NULL != minIndex );

    *numVertices = 0;
    *minIndex = 0;

    if ( 0 != numIndices )
    {
        RwUInt32    minVertIdx;

        RwUInt32    maxVertIdx;


        RWASSERT( NULL != indices );

        /* set extreme values */
        minVertIdx = 0xFFFFFFFF;
        maxVertIdx = 0x00000000;

        while ( numIndices-- )
        {
            const RxVertexIndex index = *indices;

            minVertIdx = min(minVertIdx, index);
            maxVertIdx = max(maxVertIdx, index);

            indices += 1;
        };

        *numVertices = (maxVertIdx - minVertIdx) + 1;
        *minIndex = minVertIdx;
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rxOpenGLInstanceDisplayList
 *
 *  Purpose : Instance static geometry into an OpenGL display list.
 *
 *  On entry:
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
void
_rxOpenGLInstanceDisplayList( RxOpenGLMeshInstanceData *instanceData,
                              RpGeometryFlag geomFlags,
                              const RwUInt32 type,
                              RwInt32 numTexCoords,
                              RwTexCoords **srcTexCoord,
                              RwRGBA *srcColor,
                              void *srcNormal,
                              RwV3d *srcPosition )
{
    const RwRGBA    *materialColor;

    RwBool          modulateMaterial;

    RwUInt8         alpha;


    RWFUNCTION( RWSTRING( "_rxOpenGLInstanceDisplayList" ) );

    RWASSERT( NULL != instanceData );
    RWASSERT( NULL != srcPosition );

    /* if we want to use more texture coordinates than we have
     * texture units, we can't use display lists as we can dynamically
     * set texture coordinate sets to texture units
     */
    if ( (RwUInt32)numTexCoords > _rwOpenGLExt.MaxTextureUnits )
    {
        glDeleteLists( instanceData->displayList, 1 );
        instanceData->displayList = 0;

        RWRETURNVOID();
    }

#if defined(RWDEBUG)
    if ( numTexCoords > 1 )
    {
        RWASSERT( NULL != _rwOpenGLExt.MultiTexCoord2fvARB );
    }
#endif

    /* compile the display list */
    glNewList( instanceData->displayList, GL_COMPILE );

    glBegin( instanceData->primType );

    materialColor = RpMaterialGetColor( instanceData->material );
    RWASSERT( NULL != materialColor );

    modulateMaterial = (0 != (geomFlags & rxGEOMETRY_MODULATE)) &&
                       (0xFFFFFFFF != *((const RwUInt32 *)materialColor));

    alpha = 0xFF;

    /* is the geometry indexed or non-indexed? */
    if ( NULL == instanceData->indexData )
    {
        RwUInt32    vertexIdx;


        for ( vertexIdx = 0; vertexIdx < instanceData->numVertices; vertexIdx += 1 )
        {
            /* texture coordinates */
            if ( NULL != srcTexCoord )
            {
                if ( 1 == numTexCoords )
                {
                    glTexCoord2fv( (const GLfloat *)((RwTexCoords *)&(srcTexCoord[0][instanceData->minVertexIdx]) + vertexIdx) );
                }
                else
                {
                    RwInt32    idx;


                    for ( idx = 0; idx < numTexCoords; idx += 1 )
                    {
                        _rwOpenGLExt.MultiTexCoord2fvARB( GL_TEXTURE0_ARB + idx,
                                                          (const GLfloat *)((RwTexCoords *)&(srcTexCoord[idx][instanceData->minVertexIdx]) + vertexIdx) );
                    }
                }
            }

            /* colors */
            if ( NULL != srcColor )
            {
                RwRGBA  *color;


                color = (RwRGBA *)&(srcColor[instanceData->minVertexIdx]) + vertexIdx;
                RWASSERT( NULL != color );

                if ( FALSE != modulateMaterial )
                {
                    RwUInt32    modulatedColor;


                    /* bits     info
                     * --------------
                     * 24-31    alpha
                     * 16-23    blue
                     * 8-15     green
                     * 0-7      red
                     */
                    modulatedColor =
                        ( ((color->alpha * (materialColor->alpha + 1)) & 0xFF00) << 16) |
                        ( ((color->blue  * (materialColor->blue  + 1)) & 0xFF00) << 8 ) |
                        ( ((color->green * (materialColor->green + 1)) & 0xFF00)      ) |
                        ( ((color->red   * (materialColor->red   + 1)) & 0xFF00) >> 8 );

                    alpha &= color->alpha;

                    glColor4ubv( (const GLubyte *)&modulatedColor );
                }
                else
                {
                    glColor4ubv( (const GLubyte *)color );

                    alpha &= color->alpha;
                }
            }

            /* normals */
            if ( NULL != srcNormal )
            {
                if ( rpATOMIC == type )
                {
                    glNormal3fv( (const GLfloat *)((RwV3d *)&(((RwV3d *)srcNormal)[instanceData->minVertexIdx]) + vertexIdx) );
                }
                else
                {
                    RwV3d   sectorNormal;


                    RPV3DFROMVERTEXNORMAL( *((RwV3d *)&sectorNormal),
                                           *((RpVertexNormal *)&(((RpVertexNormal *)srcNormal)[instanceData->minVertexIdx]) + vertexIdx) );

                    glNormal3fv( (const GLfloat *)&sectorNormal );
                }
            }

            /* positions */
            glVertex3fv( (const GLfloat *)((RwV3d *)&(srcPosition[instanceData->minVertexIdx]) + vertexIdx) );
        }
    }
    else
    {
        RwUInt32        index;

        RxVertexIndex   resolvedIndex;


        for ( index = 0; index < instanceData->numIndices; index += 1 )
        {
            resolvedIndex = instanceData->indexData[index];

            /* texture coordinates */
            if ( NULL != srcTexCoord )
            {
                if ( 1 == numTexCoords )
                {
                    glTexCoord2fv( (const GLfloat *)((RwTexCoords *)&(srcTexCoord[0][instanceData->minVertexIdx]) + resolvedIndex) );
                }
                else
                {
                    RwInt32    idx;


                    for ( idx = 0; idx < numTexCoords; idx += 1 )
                    {
                        _rwOpenGLExt.MultiTexCoord2fvARB( GL_TEXTURE0_ARB + idx,
                                                          (const GLfloat *)((RwTexCoords *)&(srcTexCoord[idx][instanceData->minVertexIdx]) + resolvedIndex) );
                    }
                }
            }

            /* colors */
            if ( NULL != srcColor )
            {
                RwRGBA  *color;


                color = (RwRGBA *)&(srcColor[instanceData->minVertexIdx]) + resolvedIndex;
                RWASSERT( NULL != color );

                if ( FALSE != modulateMaterial )
                {
                    RwUInt32    modulatedColor;


                    /* bits     info
                     * --------------
                     * 24-31    alpha
                     * 16-23    blue
                     * 8-15     green
                     * 0-7      red
                     */
                    modulatedColor =
                        ( ((color->alpha * (materialColor->alpha + 1)) & 0xFF00) << 16) |
                        ( ((color->blue  * (materialColor->blue  + 1)) & 0xFF00) << 8 ) |
                        ( ((color->green * (materialColor->green + 1)) & 0xFF00)      ) |
                        ( ((color->red   * (materialColor->red   + 1)) & 0xFF00) >> 8 );

                    alpha &= color->alpha;

                    glColor4ubv( (const GLubyte *)&modulatedColor );
                }
                else
                {
                    glColor4ubv( (const GLubyte *)color );

                    alpha &= color->alpha;
                }
            }

            /* normals */
            if ( NULL != srcNormal )
            {
                if ( rpATOMIC == type )
                {
                    glNormal3fv( (const GLfloat *)(&(((RwV3d *)srcNormal)[instanceData->minVertexIdx]) + resolvedIndex) );
                }
                else
                {
                    RwV3d   sectorNormal;


                    RPV3DFROMVERTEXNORMAL( *((RwV3d *)&sectorNormal),
                                           *((RpVertexNormal *)&(((RpVertexNormal *)srcNormal)[instanceData->minVertexIdx]) + resolvedIndex) );

                    glNormal3fv( (const GLfloat *)&sectorNormal );
                }
            }

            /* positions */
            glVertex3fv( (const GLfloat *)(&(srcPosition[instanceData->minVertexIdx]) + resolvedIndex) );
        }
    }

    instanceData->vertexAlpha = (alpha != 0xFF);

    glEnd();

    /* end the display list */
    glEndList();

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rxOpenGLAllInOnePipelineInit
 *
 *  Purpose : Common OpenGL All-In-One pipeline initialization.
 *
 *  On entry: node - Pointer to RxPipelineNode.
 *
 *  On exit : RwBool, TRUE if successfully initialized the pipeline node
 *            or FALSE if failed.
 * --------------------------------------------------------------------- */
RwBool
_rxOpenGLAllInOnePipelineInit( RxPipelineNode *node __RWUNUSEDRELEASE__ )
{
    RWFUNCTION( RWSTRING( "_rxOpenGLAllInOnePipelineInit" ) );

    RWASSERT( NULL != node );

    if ( 0 == openglAllInOneInitRefCount )
    {
        /* initialize primitive conversion table */
        memset( _rwOpenGLPrimConvTbl, 0, sizeof(_rwOpenGLPrimConvTbl) );
        _rwOpenGLPrimConvTbl[rwPRIMTYPELINELIST] = GL_LINES;
        _rwOpenGLPrimConvTbl[rwPRIMTYPEPOLYLINE] = GL_LINE_STRIP;
        _rwOpenGLPrimConvTbl[rwPRIMTYPETRILIST]  = GL_TRIANGLES;
        _rwOpenGLPrimConvTbl[rwPRIMTYPETRISTRIP] = GL_TRIANGLE_STRIP;
        _rwOpenGLPrimConvTbl[rwPRIMTYPETRIFAN]   = GL_TRIANGLE_FAN;
    }

    openglAllInOneInitRefCount += 1;

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rxOpenGLAllInOnePipelineTerm
 *
 *  Purpose : Common OpenGL All-In-One pipeline termination.
 *
 *  On entry: node - Pointer to RxPipelineNode.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
void
_rxOpenGLAllInOnePipelineTerm( RxPipelineNode *node __RWUNUSEDRELEASE__ )
{
    RWFUNCTION( RWSTRING( "_rxOpenGLAllInOnePipelineTerm" ) );

    RWASSERT( NULL != node );

    openglAllInOneInitRefCount -= 1;

    if ( 0 == openglAllInOneInitRefCount )
    {
        RwOpenGLDisable( rwGL_COLOR_MATERIAL );

        RwOpenGLDisable( rwGL_LIGHTING );
    }

    RWRETURNVOID();
}


/**
 * \ingroup worldextensionsopengl
 * \ref RxOpenGLAllInOneSetInstanceCallBack allows the default instance
 * callback for an OpenGL All-In-One pipeline to be customized.
 *
 * \param node Pointer to \ref RxPipelineNode to configure the instance
 *        callback for.
 *
 * \param instanceCB The \ref RxOpenGLAllInOneInstanceCallBack instance callback
 *        to use in the pipeline.
 *
 * \see \ref RxOpenGLAllInOneInstanceCallBack
 * \see RxOpenGLAllInOneGetInstanceCallBack
 */
void
RxOpenGLAllInOneSetInstanceCallBack( RxPipelineNode *node,
                                     RxOpenGLAllInOneInstanceCallBack instanceCB )
{
    _rxOpenGLAllInOnePrivateData    *privateData;


    RWAPIFUNCTION( RWSTRING( "RxOpenGLAllInOneSetInstanceCallBack" ) );

    RWASSERT( NULL != node );

    privateData = (_rxOpenGLAllInOnePrivateData *)(node->privateData);
    RWASSERT( NULL != privateData );

    privateData->instanceCB = instanceCB;

    RWRETURNVOID();
}


/**
 * \ingroup worldextensionsopengl
 * \ref RxOpenGLAllInOneGetInstanceCallBack gets the currently configured
 * \ref RxOpenGLAllInOneInstanceCallBack instancing callback from an OpenGL
 * All-In-One pipeline.
 *
 * The \ref rpworldsub plugin must be attached before using this function.
 *
 * \param node Pointer to \ref RxPipelineNode to get the instance
 *        callback from.
 *
 * \return The currently used \ref RxOpenGLAllInOneInstanceCallBack
 *         in the pipeline.
 *
 * \see \ref RxOpenGLAllInOneInstanceCallBack
 * \see RxOpenGLAllInOneSetInstanceCallBack
 */
RxOpenGLAllInOneInstanceCallBack
RxOpenGLAllInOneGetInstanceCallBack( RxPipelineNode *node )
{
    _rxOpenGLAllInOnePrivateData    *privateData;


    RWAPIFUNCTION( RWSTRING( "RxOpenGLAllInOneGetInstanceCallBack" ) );

    RWASSERT( NULL != node );

    privateData = (_rxOpenGLAllInOnePrivateData *)(node->privateData);
    RWASSERT( NULL != privateData );

    RWRETURN( privateData->instanceCB );
}


/**
 * \ingroup worldextensionsopengl
 * \ref RxOpenGLAllInOneSetReinstanceCallBack allows the default reinstance
 * callback for an OpenGL All-In-One pipeline to be customized.
 *
 * The \ref rpworldsub plugin must be attached before using this function.
 *
 * \param node Pointer to \ref RxPipelineNode to configure the reinstance
 *        callback for.
 *
 * \param reinstanceCB The \ref RxOpenGLAllInOneReinstanceCallBack reinstance
 *        callback to use in the pipeline.
 *
 * \see \ref RxOpenGLAllInOneReinstanceCallBack
 * \see RxOpenGLAllInOneGetReinstanceCallBack
 */
void
RxOpenGLAllInOneSetReinstanceCallBack( RxPipelineNode *node,
                                       RxOpenGLAllInOneReinstanceCallBack reinstanceCB )
{
    _rxOpenGLAllInOnePrivateData    *privateData;


    RWAPIFUNCTION( RWSTRING( "RxOpenGLAllInOneSetReinstanceCallBack" ) );

    RWASSERT( NULL != node );

    privateData = (_rxOpenGLAllInOnePrivateData *)(node->privateData);
    RWASSERT( NULL != privateData );

    privateData->reinstanceCB = reinstanceCB;

    RWRETURNVOID();
}


/**
 * \ingroup worldextensionsopengl
 * \ref RxOpenGLAllInOneGetReinstanceCallBack gets the currently configured
 * \ref RxOpenGLAllInOneReinstanceCallBack reinstancing callback from an OpenGL
 * All-In-One pipeline.
 *
 * The \ref rpworldsub plugin must be attached before using this function.
 *
 * \param node Pointer to \ref RxPipelineNode to get the instance
 *        callback from.
 *
 * \return The currently used \ref RxOpenGLAllInOneReinstanceCallBack
 *         in the pipeline.
 *
 * \see \ref RxOpenGLAllInOneReinstanceCallBack
 * \see RxOpenGLAllInOneSetReinstanceCallBack
 */
RxOpenGLAllInOneReinstanceCallBack
RxOpenGLAllInOneGetReinstanceCallBack( RxPipelineNode *node )
{
    _rxOpenGLAllInOnePrivateData    *privateData;


    RWAPIFUNCTION( RWSTRING( "RxOpenGLAllInOneGetReinstanceCallBack" ) );

    RWASSERT( NULL != node );

    privateData = (_rxOpenGLAllInOnePrivateData *)(node->privateData);
    RWASSERT( NULL != privateData );

    RWRETURN( privateData->reinstanceCB );
}


/**
 * \ingroup worldextensionsopengl
 * \ref RxOpenGLAllInOneSetLightingCallBack allows the default lighting
 * callback for an OpenGL All-In-One pipeline to be customized.
 *
 * The \ref rpworldsub plugin must be attached before using this function.
 *
 * \param node Pointer to \ref RxPipelineNode to configure the lighting
 *        callback for.
 *
 * \param lightingCB The \ref RxOpenGLAllInOneLightingCallBack lighting callback
 *        to use in the pipeline.
 *
 * \see \ref RxOpenGLAllInOneLightingCallBack
 * \see RxOpenGLAllInOneGetLightingCallBack
 */
void
RxOpenGLAllInOneSetLightingCallBack( RxPipelineNode *node,
                                     RxOpenGLAllInOneLightingCallBack lightingCB )
{
    _rxOpenGLAllInOnePrivateData    *privateData;


    RWAPIFUNCTION( RWSTRING( "RxOpenGLAllInOneSetLightingCallBack" ) );

    RWASSERT( NULL != node );

    privateData = (_rxOpenGLAllInOnePrivateData *)(node->privateData);
    RWASSERT( NULL != privateData );

    privateData->lightingCB = lightingCB;

    RWRETURNVOID();
}


/**
 * \ingroup worldextensionsopengl
 * \ref RxOpenGLAllInOneGetLightingCallBack gets the currently configured
 * \ref RxOpenGLAllInOneLightingCallBack lighting callback from an OpenGL
 * All-In-One pipeline.
 *
 * The \ref rpworldsub plugin must be attached before using this function.
 *
 * \param node Pointer to \ref RxPipelineNode to get the lighting
 *        callback from.
 *
 * \return The currently used \ref RxOpenGLAllInOneLightingCallBack
 *         in the pipeline.
 *
 * \see \ref RxOpenGLAllInOneLightingCallBack
 * \see RxOpenGLAllInOneSetLightingCallBack
 */
RxOpenGLAllInOneLightingCallBack
RxOpenGLAllInOneGetLightingCallBack( RxPipelineNode *node )
{
    _rxOpenGLAllInOnePrivateData    *privateData;


    RWAPIFUNCTION( RWSTRING( "RxOpenGLAllInOneGetLightingCallBack" ) );

    RWASSERT( NULL != node );

    privateData = (_rxOpenGLAllInOnePrivateData *)(node->privateData);
    RWASSERT( NULL != privateData );

    RWRETURN( privateData->lightingCB );
}


/**
 * \ingroup worldextensionsopengl
 * \ref RxOpenGLAllInOneSetRenderCallBack allows the default render callback
 * for an OpenGL All-In-One pipeline to be customized.
 *
 * The \ref rpworldsub plugin must be attached before using this function.
 *
 * \param node Pointer to \ref RxPipelineNode to configure the render
 *        callback for.
 *
 * \param renderCB The \ref RxOpenGLAllInOneRenderCallBack render callback
 *        to use in the pipeline.
 *
 * \see \ref RxOpenGLAllInOneRenderCallBack
 * \see RxOpenGLAllInOneGetRenderCallBack
 */
void
RxOpenGLAllInOneSetRenderCallBack( RxPipelineNode *node,
                                   RxOpenGLAllInOneRenderCallBack renderCB )
{
    _rxOpenGLAllInOnePrivateData    *privateData;


    RWAPIFUNCTION( RWSTRING( "RxOpenGLAllInOneSetRenderCallBack" ) );

    RWASSERT( NULL != node );

    privateData = (_rxOpenGLAllInOnePrivateData *)(node->privateData);
    RWASSERT( NULL != privateData );

    privateData->renderCB = renderCB;

    RWRETURNVOID();
}


/**
 * \ingroup worldextensionsopengl
 * \ref RxOpenGLAllInOneGetRenderCallBack gets the currently configured
 * \ref RxOpenGLAllInOneRenderCallBack render callback from an OpenGL
 * All-In-One pipeline.
 *
 * The \ref rpworldsub plugin must be attached before using this function.
 *
 * \param node Pointer to \ref RxPipelineNode to get the render
 *        callback from.
 *
 * \return The currently used \ref RxOpenGLAllInOneRenderCallBack
 *         in the pipeline.
 *
 * \see \ref RxOpenGLAllInOneRenderCallBack
 * \see RxOpenGLAllInOneSetRenderCallBack
 */
RxOpenGLAllInOneRenderCallBack
RxOpenGLAllInOneGetRenderCallBack( RxPipelineNode *node )
{
    _rxOpenGLAllInOnePrivateData    *privateData;


    RWAPIFUNCTION( RWSTRING( "RxOpenGLAllInOneGetRenderCallBack" ) );

    RWASSERT( NULL != node );

    privateData = (_rxOpenGLAllInOnePrivateData *)(node->privateData);
    RWASSERT( NULL != privateData );

    RWRETURN( privateData->renderCB );
}


/**
 * \ingroup worldextensionsopengl
 * \ref RxOpenGLAllInOneSetInstanceDLandVA allows the default instancing method
 * for an OpenGL All-In-One pipeline to be customized. If the \e instanceDLandVA
 * parameter is TRUE then (potentially) both display lists and vertex arrays
 * are used to instance objects. If FALSE then the system determines which
 * instancing method to use based on the type of object and the system
 * capabilities.
 *
 * Note that it may be the case that display lists are not used to instance due to
 * the restraints that that method places on what the geometry is used for.
 * Also, if server side vertex array support is provided by the host system,
 * display lists are \e not used.
 *
 * The \ref rpworldsub plugin must be attached before using this function.
 *
 * \param node Pointer to \ref RxPipelineNode to configure the instancing method for.
 *
 * \param instanceDLandVA Constant \ref RwBool used to set the instancing method.
 *
 * \see RxOpenGLAllInOneGetInstanceDLandVA
 */
void
RxOpenGLAllInOneSetInstanceDLandVA( RxPipelineNode *node,
                                    const RwBool instanceDLandVA )
{
    _rxOpenGLAllInOnePrivateData    *privateData;


    RWAPIFUNCTION( RWSTRING( "RxOpenGLAllInOneSetInstanceDLandVA" ) );

    RWASSERT( NULL != node );

    privateData = (_rxOpenGLAllInOnePrivateData *)(node->privateData);
    RWASSERT( NULL != privateData );

    privateData->instanceDLandVA = instanceDLandVA;

    RWRETURNVOID();
}


/**
 * \ingroup worldextensionsopengl
 * \ref RxOpenGLAllInOneGetInstanceDLandVA gets the currently configured
 * instancing method from an OpenGL All-In-One pipeline.
 *
 * The \ref rpworldsub plugin must be attached before using this function.
 *
 * \param node Pointer to \ref RxPipelineNode to get the instancing
 *        method from.
 *
 * \return \ref RwBool, TRUE if both display lists and vertex arrays
 *         are (potentially) instanced, FALSE if a best choice is made.
 *
 * \see RxOpenGLAllInOneSetInstanceDLandVA
 */
RwBool
RxOpenGLAllInOneGetInstanceDLandVA( RxPipelineNode *node )
{
    _rxOpenGLAllInOnePrivateData    *privateData;


    RWAPIFUNCTION( RWSTRING( "RxOpenGLAllInOneGetInstanceDLandVA" ) );

    RWASSERT( NULL != node );

    privateData = (_rxOpenGLAllInOnePrivateData *)(node->privateData);
    RWASSERT( NULL != privateData );

    RWRETURN( privateData->instanceDLandVA );
}


/* =====================================================================
 *  Static function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: openglResEntryDestructor
 *
 *  Purpose : OpenGL RwResEntry destructor.
 *
 *  On entry: resEntry - Pointer to RwResEntry to destroy.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
static void
openglResEntryDestructor( RwResEntry *resEntry )
{
    RxOpenGLResEntryHeader  *resEntryHeader;

    RxOpenGLMeshInstanceData    *instanceData;

    RwUInt16                numMeshes;


    RWFUNCTION( RWSTRING( "openglResEntryDestructor" ) );

    RWASSERT( NULL != resEntry );

    resEntryHeader = (RxOpenGLResEntryHeader *)(resEntry + 1);
    RWASSERT( NULL != resEntryHeader );

    instanceData = (RxOpenGLMeshInstanceData *)(resEntryHeader + 1);
    RWASSERT( NULL != instanceData );

    /* get the number of meshes */
    numMeshes = resEntryHeader->numMeshes;

    while ( numMeshes-- )
    {
        openglInstanceDataDestructor( instanceData );

        /* next RxOpenGLMeshInstanceData pointer */
        instanceData += 1;
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglInstanceDataDestructor
 *
 *  Purpose : OpenGL instance data destructor.
 *
 *  On entry: instanceData - Pointer to RxOpenGLMeshInstanceData to destroy.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
static void
openglInstanceDataDestructor( RxOpenGLMeshInstanceData *instanceData )
{
    RWFUNCTION( RWSTRING( "openglInstanceDataDestructor" ) );

    RWASSERT( NULL != instanceData );

    if ( NULL != instanceData->sysMemVertexData )
    {
        RwFree( instanceData->sysMemVertexData );
        instanceData->sysMemVertexData = (RwUInt8 *)NULL;
    }

    if ( NULL != instanceData->vidMemVertexData )
    {
        if ( _rwOpenGLVertexHeapAvailable() )
        {
            if ( FALSE != instanceData->vidMemDataIsStatic )
            {
                _rwOpenGLVertexHeapStaticFree( instanceData->vidMemVertexData );
            }
            else
            {
                _rwOpenGLVertexHeapDynamicFree( instanceData->vidMemVertexData );
            }
        }

        instanceData->vidMemVertexData = (void *)NULL;
    }

    if ( NULL != instanceData->indexData )
    {
        RwFree( instanceData->indexData );
        instanceData->indexData = (RxVertexIndex *)NULL;
    }

    if ( GL_FALSE != glIsList( instanceData->displayList ) )
    {
        glDeleteLists( instanceData->displayList, 1 );
        instanceData->displayList = 0;
    }

    if ( NULL != _rwOpenGLExt.IsObjectBufferATI )
    {
        if ( GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName ) )
        {
            _rwOpenGLExt.FreeObjectBufferATI( instanceData->vaoName );
            instanceData->vaoName = 0;
        }
    }

    RWRETURNVOID();
}


