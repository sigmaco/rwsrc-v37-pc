/***********************************************************************
 *
 * Module:  nodeOpenGLSubmitNoLight.c
 *
 * Purpose: OpenGL Immediate Mode Rendering
 *
 ***********************************************************************/

/* =====================================================================
 *  Includes
 * ===================================================================== */
#include <string.h> /* for memset() */

#include "batypes.h"

#include "bapipe.h"

#include "nodeOpenGLSubmitNoLight.h"

#if defined(WIN32)
#include <windows.h>
#endif /* defined(WIN32) */
#include <gl/gl.h>


/* =====================================================================
 *  Defines
 * ===================================================================== */


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */
typedef void (*rwOpenGLColorXFn)( const GLubyte *v );


/* =====================================================================
 *  Static variables
 * ===================================================================== */
static rwIm3DPool   *openGLImmPool = (rwIm3DPool *)NULL;

static GLenum openGLPrimConvTbl[ rwPRIMTYPELINELIST |
                                 rwPRIMTYPEPOLYLINE |
                                 rwPRIMTYPETRILIST  |
                                 rwPRIMTYPETRISTRIP |
                                 rwPRIMTYPETRIFAN ];


/* =====================================================================
 *  Global variables
 * ===================================================================== */


/* =====================================================================
 *  Static function prototypes
 * ===================================================================== */

static RwBool
openGLImmInstanceNode( RxPipelineNodeInstance *self,
                       const RxPipelineNodeParam *params );

static RwBool
openGLSubmitNoLightNodeInit( RxNodeDefinition *self );

static RwBool
openGLSubmitNoLightNode( RxPipelineNodeInstance *self,
                         const RxPipelineNodeParam *params );


/* =====================================================================
 *  Global function definitions
 * ===================================================================== */

/**
 * \ingroup coreopengl
 * \ref RxNodeDefinitionGetOpenGLImmInstance returns a pointer to a node.
 *
 * \return Pointer to an \ref RxNodeDefinition on success, NULL otherwise.
 *
 * \see RxNodeDefinitionGetOpenGLSubmitNoLight
 */
RxNodeDefinition *
RxNodeDefinitionGetOpenGLImmInstance( void )
{
    static RwChar           openglImmInstance_csl[] = RWSTRING( "OpenGLImmInstance.csl" );

    static RxNodeDefinition nodeOpenGLImmInstanceCSL =
    {
        openglImmInstance_csl,                          /* name */

        /* node methods */
        {
            (RxNodeBodyFn)openGLImmInstanceNode,        /* +-- node body */
            (RxNodeInitFn)NULL,                         /* +-- node init */
            (RxNodeTermFn)NULL,                         /* +-- node term */
            (RxPipelineNodeInitFn)NULL,                 /* +-- pipeline node init */
            (RxPipelineNodeTermFn)NULL,                 /* +-- pipeline node term */
            (RxPipelineNodeConfigFn)NULL,               /* +-- pipeline node config */
            (RxConfigMsgHandlerFn)NULL                  /* +-- config message handler */
        },

        /* IO */
        {
            0,                                          /* +-- num clusters of interest */
            (RxClusterRef *)NULL,                       /* +-- clusters of interest */
            (RxClusterValidityReq *)NULL,               /* +-- requirements of input clusters */
            0,                                          /* +-- num outputs */
            (RxOutputSpec *)NULL                        /* +-- output specifications */
        },

        0,                                              /* +-- size of private data */
        rxNODEDEFCONST,                                 /* +-- editable? */
        0                                               /* +-- unlocked pipe count */
    };

    RWAPIFUNCTION( RWSTRING( "RxNodeDefinitionGetOpenGLImmInstance" ) );

    RWRETURN( &nodeOpenGLImmInstanceCSL );
}


/**
 * \ingroup coreopengl
 * \ref RxNodeDefinitionGetOpenGLSubmitNoLight returns a pointer to a node
 * to dispatch polygons using OpenGL transform facilities.
 *
 * The node supports the submission of line lists and strips, and of triangle
 * lists, strips and fans.
 *
 * \return Pointer to an \ref RxNodeDefinition to submit triangles to the
 * rasterizer on success, NULL otherwise.
 *
 * \see RxNodeDefinitionGetOpenGLImmInstance
 */
RxNodeDefinition *
RxNodeDefinitionGetOpenGLSubmitNoLight( void )
{
    static RwChar           openglSubmitNoLight_csl[] = RWSTRING( "OpenGLSubmitNoLight.csl" );

    static RxNodeDefinition nodeOpenGLSubmitNoLight =
    {
        openglSubmitNoLight_csl,                        /* name */

        /* node methods */
        {
            (RxNodeBodyFn)openGLSubmitNoLightNode,      /* +-- node body */
            (RxNodeInitFn)openGLSubmitNoLightNodeInit,  /* +-- node init */
            (RxNodeTermFn)NULL,                         /* +-- node term */
            (RxPipelineNodeInitFn)NULL,                 /* +-- pipeline node init */
            (RxPipelineNodeTermFn)NULL,                 /* +-- pipeline node term */
            (RxPipelineNodeConfigFn)NULL,               /* +-- pipeline node config */
            (RxConfigMsgHandlerFn)NULL                  /* +-- config message handler */
        },

        /* IO */
        {
            0,                                          /* +-- num clusters of interest */
            (RxClusterRef *)NULL,                       /* +-- clusters of interest */
            (RxClusterValidityReq *)NULL,               /* +-- requirements of input clusters */
            0,                                          /* +-- num outputs */
            (RxOutputSpec *)NULL                        /* +-- output specifications */
        },

        0,                                              /* +-- size of private data */
        rxNODEDEFCONST,                                 /* +-- editable? */
        0                                               /* +-- unlocked pipe count */
    };

    RWAPIFUNCTION( RWSTRING( "RxNodeDefinitionGetOpenGLSubmitNoLight" ) );

    RWRETURN( &nodeOpenGLSubmitNoLight );
}



/* =====================================================================
 *  Static function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: openGLImmInstanceNode
 *
 *  Purpose :
 *
 *  On entry: self - Pointer to RxPipelineNodeInstance (not used)
 *
 *            params - Pointer to constant RxPipelineNodeParam.
 *
 *  On exit : RwBool, TRUE if successfully instanced, FALSE if not.
 * --------------------------------------------------------------------- */
static RwBool
openGLImmInstanceNode( RxPipelineNodeInstance *self __RWUNUSED__,
                       const RxPipelineNodeParam *params )
{
    RWFUNCTION( RWSTRING( "openGLImmInstanceNode" ) );

    openGLImmPool = (rwIm3DPool *)RxPipelineNodeParamGetData( params );

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: openGLSubmitNoLightNodeInit
 *
 *  Purpose :
 *
 *  On entry: self - Pointer to RxPipelineNodeInstance (not used)
 *
 *  On exit : RwBool, TRUE if successfully instanced, FALSE if not.
 * --------------------------------------------------------------------- */
static RwBool
openGLSubmitNoLightNodeInit( RxNodeDefinition *self __RWUNUSED__ )
{
    RWFUNCTION( RWSTRING( "openGLSubmitNoLightNodeInit" ) );

    memset( openGLPrimConvTbl, 0, sizeof(openGLPrimConvTbl) );

    openGLPrimConvTbl[rwPRIMTYPELINELIST] = GL_LINES;
    openGLPrimConvTbl[rwPRIMTYPEPOLYLINE] = GL_LINE_STRIP;
    openGLPrimConvTbl[rwPRIMTYPETRILIST]  = GL_TRIANGLES;
    openGLPrimConvTbl[rwPRIMTYPETRISTRIP] = GL_TRIANGLE_STRIP;
    openGLPrimConvTbl[rwPRIMTYPETRIFAN]   = GL_TRIANGLE_FAN;

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: openGLSubmitNoLightNode
 *
 *  Purpose :
 *
 *  On entry: self - Pointer to RxPipelineNodeInstance (not used).
 *
 *            params - Pointer to constant RxPipelineNodeParam (not used).
 *
 *  On exit : RwBool, TRUE if successfully rendered, FALSE if not.
 * --------------------------------------------------------------------- */
static RwBool
openGLSubmitNoLightNode( RxPipelineNodeInstance *self __RWUNUSED__,
                         const RxPipelineNodeParam *params __RWUNUSED__ )
{
    const _rwIm3DPoolStash  *stash;

    RwIm3DVertex            *vertices;

    RwBool                  popModelMatrix;

    rwOpenGLColorXFn        glColorXFn;

#if defined(RWMETRICS)
    RwUInt32                numPrimitives;
#endif /* defined(RWMETRICS) */


    RWFUNCTION( RWSTRING( "openGLSubmitNoLightNode" ) );

    stash = &(openGLImmPool->stash);
    RWASSERT( NULL != stash );

    vertices = openGLImmPool->elements;
    RWASSERT( NULL != vertices );

#if defined(RWDEBUG)
    {
        GLint   curOpenGLMatrixMode;


        glGetIntegerv( GL_MATRIX_MODE, &curOpenGLMatrixMode );
        RWASSERT( GL_MODELVIEW == curOpenGLMatrixMode );
    }
#endif /* defined(RWDEBUG) */

    /* push the model view matrix if required */
    popModelMatrix = FALSE;
    if ( NULL != stash->ltm )
    {
        popModelMatrix = !rwMatrixTestFlags( stash->ltm, rwMATRIXINTERNALIDENTITY );
        if ( FALSE != popModelMatrix )
        {
            glPushMatrix();
            _rwOpenGLApplyRwMatrix( stash->ltm );
        }
    }

    /* assume pushing 4 color components unless the rwIM3D_ALLOPAQUE flag is set */
    glColorXFn = (rwOpenGLColorXFn)glColor4ubv;
    if ( 0 != (rwIM3D_ALLOPAQUE & stash->flags) )
    {
        glColorXFn = (rwOpenGLColorXFn)glColor3ubv;
    }

    if ( NULL != stash->indices )
    {
        RwImVertexIndex *indices;


        /* indexed immediate mode data */
        indices = stash->indices;

#if defined(RWMETRICS)
        numPrimitives = 0;
        switch ( stash->primType )
        {
        case rwPRIMTYPETRISTRIP:
            {
                numPrimitives = stash->numIndices - 2;
            }
            break;

        case rwPRIMTYPETRIFAN:
            {
                numPrimitives = stash->numIndices - 2;
            }
            break;

        case rwPRIMTYPETRILIST:
            {
                numPrimitives = stash->numIndices / 3;
            }
            break;
        }
#endif /* defined(RWMETRICS) */

        glBegin( openGLPrimConvTbl[stash->primType] );

        switch (stash->primType)
        {
        case rwPRIMTYPEPOLYLINE:
        case rwPRIMTYPETRISTRIP:
        case rwPRIMTYPETRIFAN:
            {
                RwUInt16    numVertices;


                numVertices = (RwUInt16)(stash->numIndices);

                if ( 0 != (rwIM3D_VERTEXUV & stash->flags) )
                {
                    while ( numVertices-- )
                    {
                        RwIm3DVertex    *vert;


                        vert = vertices + *indices;

                        glColor4ubv( (GLubyte *)(&(vert->color)) );
                        glTexCoord2fv( (GLfloat *)(&(vert->texCoords)) );
                        glVertex3fv( (GLfloat *)(&(vert->position)) );
                        indices += 1;
                    }
                }
                else
                {
                    while ( numVertices-- )
                    {
                        RwIm3DVertex    *vert;


                        vert = vertices + *indices;

                        glColor4ubv( (GLubyte *)(&(vert->color)) );
                        glVertex3fv( (GLfloat *)(&(vert->position)) );
                        indices += 1;
                    }
                }
            }
            break;

        case rwPRIMTYPETRILIST:
            {
                RwUInt16    numTriangles;


                numTriangles = (RwUInt16)(stash->numIndices / 3);

                if ( 0 != (rwIM3D_VERTEXUV & stash->flags) )
                {
                    while ( numTriangles-- )
                    {
                        RwIm3DVertex    *vert;


                        vert = vertices + *indices;

                        glColor4ubv( (GLubyte *)(&(vert->color)) );
                        glTexCoord2fv( (GLfloat *)(&(vert->texCoords)) );
                        glVertex3fv( (GLfloat *)(&(vert->position)) );
                        indices += 1;

                        vert = vertices + *indices;

                        glColor4ubv( (GLubyte *)(&(vert->color)) );
                        glTexCoord2fv( (GLfloat *)(&(vert->texCoords)) );
                        glVertex3fv( (GLfloat *)(&(vert->position)) );
                        indices += 1;

                        vert = vertices + *indices;

                        glColor4ubv( (GLubyte *)(&(vert->color)) );
                        glTexCoord2fv( (GLfloat *)(&(vert->texCoords)) );
                        glVertex3fv( (GLfloat *)(&(vert->position)) );
                        indices += 1;
                    }
                }
                else
                {
                    while ( numTriangles-- )
                    {
                        RwIm3DVertex    *vert;


                        vert = vertices + *indices;

                        glColor4ubv( (GLubyte *)(&(vert->color)) );
                        glVertex3fv( (GLfloat *)(&(vert->position)) );
                        indices += 1;

                        vert = vertices + *indices;

                        glColor4ubv( (GLubyte *)(&(vert->color)) );
                        glVertex3fv( (GLfloat *)(&(vert->position)) );
                        indices += 1;

                        vert = vertices + *indices;

                        glColor4ubv( (GLubyte *)(&(vert->color)) );
                        glVertex3fv( (GLfloat *)(&(vert->position)) );
                        indices += 1;
                    }
                }
            }
            break;

        case rwPRIMTYPELINELIST:
            {
                RwUInt16    numLines;


                numLines = (RwUInt16)(stash->numIndices >> 1);

                if ( 0 != (rwIM3D_VERTEXUV & stash->flags) )
                {
                    while ( numLines-- )
                    {
                        RwIm3DVertex    *vert;


                        vert = vertices + *indices;

                        glColor4ubv( (GLubyte *)&(vert->color) );
                        glTexCoord2fv( (GLfloat *)&(vert->texCoords) );
                        glVertex3fv( (GLfloat *)&(vert->position) );
                        indices += 1;

                        vert = vertices + *indices;

                        glColor4ubv( (GLubyte *)&(vert->color) );
                        glTexCoord2fv( (GLfloat *)&(vert->texCoords) );
                        glVertex3fv( (GLfloat *)&(vert->position) );
                        indices += 1;
                    }
                }
                else
                {
                    while ( numLines-- )
                    {
                        RwIm3DVertex    *vert;


                        vert = vertices + *indices;

                        glColor4ubv( (GLubyte *)&(vert->color) );
                        glVertex3fv( (GLfloat *)&(vert->position) );
                        indices += 1;

                        vert = vertices + *indices;

                        glColor4ubv( (GLubyte *)&(vert->color) );
                        glVertex3fv( (GLfloat *)&(vert->position) );
                        indices += 1;
                    }
                }
            }
            break;

        default:
            {
                RWERROR( (E_RX_INVALIDPRIMTYPE) );
            }
        }

        glEnd();
    }
    else
    {
#if defined(RWMETRICS)
        numPrimitives = 0;
        switch ( stash->primType )
        {
        case rwPRIMTYPETRISTRIP:
            {
                numPrimitives = openGLImmPool->numElements - 2;
            }
            break;

        case rwPRIMTYPETRIFAN:
            {
                numPrimitives = openGLImmPool->numElements - 2;
            }
            break;

        case rwPRIMTYPETRILIST:
            {
                numPrimitives = openGLImmPool->numElements / 3;
            }
            break;
        }
#endif /* defined(RWMETRICS) */

        /* non-indexed immediate mode data */
        glBegin( openGLPrimConvTbl[stash->primType] );

        switch (stash->primType)
        {
        case rwPRIMTYPEPOLYLINE:
        case rwPRIMTYPETRISTRIP:
        case rwPRIMTYPETRIFAN:
            {
                RwUInt16    numVertices;


                numVertices = openGLImmPool->numElements;

                if ( 0 != (rwIM3D_VERTEXUV & stash->flags) )
                {
                    while ( numVertices-- )
                    {
                        glColor4ubv( (GLubyte *)(&(vertices->color)) );
                        glTexCoord2fv( (GLfloat *)(&(vertices->texCoords)) );
                        glVertex3fv( (GLfloat *)(&(vertices->position)) );
                        vertices += 1;
                    }
                }
                else
                {
                    while ( numVertices-- )
                    {
                        glColor4ubv( (GLubyte *)(&(vertices->color)) );
                        glVertex3fv( (GLfloat *)(&(vertices->position)) );
                        vertices += 1;
                    }
                }
            }
            break;

        case rwPRIMTYPETRILIST:
            {
                RwUInt16    numTriangles;


                numTriangles = (RwUInt16)(openGLImmPool->numElements / 3);

                if ( 0 != (rwIM3D_VERTEXUV & stash->flags) )
                {
                    while ( numTriangles-- )
                    {
                        glColor4ubv( (GLubyte *)(&(vertices->color)) );
                        glTexCoord2fv( (GLfloat *)(&(vertices->texCoords)) );
                        glVertex3fv( (GLfloat *)(&(vertices->position)) );
                        vertices += 1;

                        glColor4ubv( (GLubyte *)(&(vertices->color)) );
                        glTexCoord2fv( (GLfloat *)(&(vertices->texCoords)) );
                        glVertex3fv( (GLfloat *)(&(vertices->position)) );
                        vertices += 1;

                        glColor4ubv( (GLubyte *)(&(vertices->color)) );
                        glTexCoord2fv( (GLfloat *)(&(vertices->texCoords)) );
                        glVertex3fv( (GLfloat *)(&(vertices->position)) );
                        vertices += 1;
                    }
                }
                else
                {
                    while ( numTriangles-- )
                    {
                        glColor4ubv( (GLubyte *)(&(vertices->color)) );
                        glVertex3fv( (GLfloat *)(&(vertices->position)) );
                        vertices += 1;

                        glColor4ubv( (GLubyte *)(&(vertices->color)) );
                        glVertex3fv( (GLfloat *)(&(vertices->position)) );
                        vertices += 1;

                        glColor4ubv( (GLubyte *)(&(vertices->color)) );
                        glVertex3fv( (GLfloat *)(&(vertices->position)) );
                        vertices += 1;
                    }
                }
            }
            break;

        case rwPRIMTYPELINELIST:
            {
                RwUInt16    numLines;


                numLines = (RwUInt16)(openGLImmPool->numElements >> 1);

                if ( 0 != (rwIM3D_VERTEXUV & stash->flags) )
                {
                    while ( numLines-- )
                    {
                        glColor4ubv( (GLubyte *)&(vertices->color) );
                        glTexCoord2fv( (GLfloat *)&(vertices->texCoords) );
                        glVertex3fv( (GLfloat *)&(vertices->position) );
                        vertices += 1;

                        glColor4ubv( (GLubyte *)&(vertices->color) );
                        glTexCoord2fv( (GLfloat *)&(vertices->texCoords) );
                        glVertex3fv( (GLfloat *)&(vertices->position) );
                        vertices += 1;
                    }
                }
                else
                {
                    while ( numLines-- )
                    {
                        glColor4ubv( (GLubyte *)&(vertices->color) );
                        glVertex3fv( (GLfloat *)&(vertices->position) );
                        vertices += 1;

                        glColor4ubv( (GLubyte *)&(vertices->color) );
                        glVertex3fv( (GLfloat *)&(vertices->position) );
                        vertices += 1;
                    }
                }
            }
            break;

        default:
            {
                RWERROR( (E_RX_INVALIDPRIMTYPE) );
            }
        }

        glEnd();
    }

#if defined( RWMETRICS )
    switch ( stash->primType )
    {
        case rwPRIMTYPETRILIST:
        case rwPRIMTYPETRISTRIP:
        case rwPRIMTYPETRIFAN:
            {
                RWSRCGLOBAL(metrics)->numProcTriangles += numPrimitives;
            }
            break;
    }
#endif /* defined( RWMETRICS ) */

    /* pop the model view matrix if required */
    if ( FALSE != popModelMatrix )
    {
        glPopMatrix();
    }

    RWRETURN( TRUE );
}
