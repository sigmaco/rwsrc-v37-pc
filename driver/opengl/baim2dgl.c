/***************************************************************************
 *                                                                         *
 * Module  : baim2dgl.c                                                    *
 *                                                                         *
 * Purpose : Handling of 2D immediate mode rendering                       *
 *                                                                         *
 **************************************************************************/

/****************************************************************************
 Includes
 */

/* RW type definitions */
#include "batypes.h"
#include "bamemory.h"
#include "badevice.h"
#include "batextur.h"
#include "baimmedi.h"
#include "bavector.h"

/* OpenGL driver includes */
#include "drvmodel.h"

/* Common driver includes */
#include "barwtyp.h"

/* OpenGL driver includes */
#include "drvfns.h"
#include "baogl.h"     /* For GL_ERROR_CHECK() */
#include "baintogl.h"  /* For raster extension stuff */

/* Header for this file */
#include "baim2dgl.h"

/* OpenGL library includes */
#ifdef WIN32
#include <windows.h>
#else
#define WINGDIAPI
#define APIENTRY
#include <string.h>
#endif
#include <GL/gl.h>




/* Primitive type lookup tables: RW->OGL */
static GLenum PrimLookup[rwPRIMTYPELINELIST|
                         rwPRIMTYPEPOLYLINE|
                         rwPRIMTYPETRILIST |
                         rwPRIMTYPETRISTRIP|
                         rwPRIMTYPETRIFAN];

static RwDevice *GlobalDevice = NULL;


typedef WINGDIAPI void (APIENTRY *OGLVertexFunc)(const GLfloat *v);
typedef WINGDIAPI void (APIENTRY *OGLColorFunc)(const GLubyte *c);

OGLVertexFunc VertexFunc = NULL;     /* Specify the glVertex function on Z buffer state */
OGLColorFunc  ColorFunc  = NULL;     /* Specify the glColor function on alpha blend state */

/*
 * Below are shorthand representations for OpenGL vertex specifications.
 * Notice that the array parameter versions of the OGL functions are
 * being used.  This gives a performance boost to the driver.
 */
#define GL_VERTEX_V(vert)         VertexFunc(&((vert)->x))
#define GL_VERTEX_T_V(vert)       glTexCoord4fv(&((vert)->sow));    \
                                  VertexFunc(&((vert)->x))
#define GL_VERTEX_C_V(vert)       ColorFunc(&((vert)->r));          \
                                  VertexFunc(&((vert)->x))
#define GL_VERTEX_C_T_V(vert)     ColorFunc(&((vert)->r));          \
                                  glTexCoord4fv(&((vert)->sow));    \
                                  VertexFunc(&((vert)->x))


/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                    Immediate polygon renderer

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */



static RwBool
OpenGLIm2DRenderLine(RwIm2DVertex *verts, RwInt32 numVerts,
                        RwInt32 vert1, RwInt32 vert2)
{
    RwImVertexIndex indices[2];

    RWFUNCTION(RWSTRING("OpenGLIm2DRenderLine"));
    RWASSERT(verts);

    indices[0] = (RwImVertexIndex)vert1;
    indices[1] = (RwImVertexIndex)vert2;

    GlobalDevice->fpIm2DRenderIndexedPrimitive(rwPRIMTYPELINELIST, verts, numVerts, indices, 2);

    RWRETURN(TRUE);
}

static RwBool
OpenGLIm2DRenderTriangle(RwIm2DVertex *verts, RwInt32 numVerts,
                            RwInt32 vert1, RwInt32 vert2, RwInt32 vert3)
{
    RwImVertexIndex indices[3];

    RWFUNCTION(RWSTRING("OpenGLIm2DRenderTriangle"));
    RWASSERT(verts);

    indices[0] = (RwImVertexIndex)vert1;
    indices[1] = (RwImVertexIndex)vert2;
    indices[2] = (RwImVertexIndex)vert3;

    GlobalDevice->fpIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, verts, numVerts, indices, 3);

    RWRETURN(TRUE);
}


#define SET_UP_SCREEN_SPACE_PROJECTION                          \
    {                                                           \
        glMatrixMode(GL_PROJECTION);                            \
        glPushMatrix();                                         \
        glLoadIdentity();                                       \
                                                                \
        if (dgGGlobals.curCamera)                               \
        {                                                       \
            RwRaster *frameBuffer  = RwCameraGetRaster(dgGGlobals.curCamera); \
            RwRaster *rasterParent = NULL;                      \
            rasterParent = frameBuffer;                         \
                                                                \
            while (rasterParent != rasterParent->parent)        \
            {                                                   \
                rasterParent = rasterParent->parent;            \
            }                                                   \
                                                                \
            if (frameBuffer->cType == rwRASTERTYPECAMERATEXTURE)    \
            {                                                   \
                glScalef(1, -1, 1);                             \
            }                                                   \
                                                                \
            glScaled(2.0/(GLdouble)frameBuffer->width, -2.0/(GLdouble)frameBuffer->height, 2.0/(GLdouble)rwOGLFARSCREENZ);    \
            glTranslated(-(GLdouble)frameBuffer->width / 2.0, -(GLdouble)frameBuffer->height / 2.0, -((GLdouble)rwOGLFARSCREENZ/2.0));           \
        }                                                       \
                                                                \
        glMatrixMode(GL_MODELVIEW);                             \
        glPushMatrix();                                         \
        glLoadIdentity();                                       \
    }

#define CLEAN_UP_SCREEN_SPACE_PROJECTION        \
    {                                           \
        glMatrixMode(GL_PROJECTION);            \
        glPopMatrix();                          \
        glMatrixMode(GL_MODELVIEW);             \
        glPopMatrix();                          \
    }

#define INDEXED_PRIMITIVE_UPDATE_METRICS                                                \
    {                                                                                   \
        RwInt32 numPrims;                                                               \
                                                                                        \
        RWSRCGLOBAL(metrics)->numVertices += numVertices;                               \
                                                                                        \
        if (primType == rwPRIMTYPETRILIST)                                              \
        {                                                                               \
            numPrims = numIndices / 3;                                                  \
            RWSRCGLOBAL(metrics)->numTriangles     += numPrims;                         \
            RWSRCGLOBAL(metrics)->numProcTriangles += numPrims;                         \
        }                                                                               \
        else if ((primType == rwPRIMTYPETRISTRIP) || (primType == rwPRIMTYPETRIFAN))    \
        {                                                                               \
            numPrims = numIndices - 2;                                                  \
            RWSRCGLOBAL(metrics)->numTriangles     += numPrims;                         \
            RWSRCGLOBAL(metrics)->numProcTriangles += numPrims;                         \
        }                                                                               \
    }

#define PRIMITIVE_UPDATE_METRICS                                                        \
    {                                                                                   \
        RwInt32 numPrims;                                                               \
                                                                                        \
        RWSRCGLOBAL(metrics)->numVertices += numVertices;                               \
                                                                                        \
        if (primType == rwPRIMTYPETRILIST)                                              \
        {                                                                               \
            numPrims = numVertices / 3;                                                 \
            RWSRCGLOBAL(metrics)->numTriangles     += numPrims;                         \
            RWSRCGLOBAL(metrics)->numProcTriangles += numPrims;                         \
        }                                                                               \
        else if ((primType == rwPRIMTYPETRISTRIP) || (primType == rwPRIMTYPETRIFAN))    \
        {                                                                               \
            numPrims = numVertices - 2;                                                 \
            RWSRCGLOBAL(metrics)->numTriangles     += numPrims;                         \
            RWSRCGLOBAL(metrics)->numProcTriangles += numPrims;                         \
        }                                                                               \
    }


/****************************************************************************
 OpenGLIm2DRenderPrimitive_NoGouraud_NoTexture

 On entry   : primitive type, vertices, numVertices
 On exit    :
 */

static RwBool
OpenGLIm2DRenderPrimitive_NoGouraud_NoTexture(RwPrimitiveType primType,
                                              RwIm2DVertex *vertices,
                                              RwInt32 numVertices)
{
    RwInt32 index      = 0;
    RwOpenGLVertex *vert = (RwOpenGLVertex*)vertices;

    RWFUNCTION(RWSTRING("OpenGLIm2DRenderPrimitive_NoGouraud_NoTexture"));

#ifdef USE_OGL_PIPE
    SET_UP_SCREEN_SPACE_PROJECTION
#endif

    glBegin(PrimLookup[primType]);
        switch (primType)
        {
            case rwPRIMTYPELINELIST:
            {
                while (index < numVertices)
                {
                    GL_VERTEX_V(vert);   vert++;
                    GL_VERTEX_C_V(vert); vert++;  /* Flat shaded so don't bother with specifying a vertex colour */

                    index += 2;
                }
                break;
            }
            case rwPRIMTYPETRILIST:
            {
                while (index < numVertices)
                {
                    GL_VERTEX_V(vert);   vert++;
                    GL_VERTEX_V(vert);   vert++;
                    GL_VERTEX_C_V(vert); vert++;

                    index += 3;
                }
                break;
            }
            case rwPRIMTYPEPOLYLINE:
            case rwPRIMTYPETRISTRIP:
            case rwPRIMTYPETRIFAN:
            {
                GL_VERTEX_V(vert); vert++;

                index = 1;
                while (index < numVertices)
                {
                    GL_VERTEX_C_V(vert); vert++;

                    index++;
                }
                break;
            }
            default:
            {
                glEnd();
                RWRETURN(FALSE);
                break;
            }
        }
    glEnd();

#ifdef USE_OGL_PIPE
    CLEAN_UP_SCREEN_SPACE_PROJECTION
#endif

#ifdef RWMETRICS
    PRIMITIVE_UPDATE_METRICS
#endif /* RWMETRICS */

    RWRETURN(TRUE);
}


/****************************************************************************
 OpenGLIm2DRenderPrimitive_NoGouraud_Texture

 On entry   : primitive type, vertices, numVertices
 On exit    :
 */

static RwBool
OpenGLIm2DRenderPrimitive_NoGouraud_Texture(RwPrimitiveType primType,
                                            RwIm2DVertex *vertices,
                                            RwInt32 numVertices)
{
    RwInt32 index    = 0;
    RwOpenGLVertex *vert = (RwOpenGLVertex*)vertices;

    RWFUNCTION(RWSTRING("OpenGLIm2DRenderPrimitive_NoGouraud_Texture"));

#ifdef USE_OGL_PIPE
    SET_UP_SCREEN_SPACE_PROJECTION
#endif

    glBegin(PrimLookup[primType]);
        switch (primType)
        {
            case rwPRIMTYPELINELIST:
            {
                while (index < numVertices)
                {
                    GL_VERTEX_T_V(vert);    vert++;
                    GL_VERTEX_C_T_V(vert);  vert++; /* Flat shaded so don't bother with specifying a vertex colour */

                    index += 2;
                }
                break;
            }
            case rwPRIMTYPEPOLYLINE:
            {
                GL_VERTEX_T_V(vert); vert++;

                index = 1;
                while (index < numVertices)
                {
                    GL_VERTEX_C_T_V(vert); vert++;

                    index++;
                }
                break;
            }
            case rwPRIMTYPETRILIST:
            {
                while (index < numVertices)
                {
                    GL_VERTEX_T_V(vert);   vert++;
                    GL_VERTEX_T_V(vert);   vert++;
                    GL_VERTEX_C_T_V(vert); vert++;

                    index += 3;
                }
                break;
            }
            case rwPRIMTYPETRISTRIP:
            case rwPRIMTYPETRIFAN:
            {
                GL_VERTEX_T_V(vert); vert++;
                GL_VERTEX_T_V(vert); vert++;

                index = 2;
                while (index < numVertices)
                {
                    GL_VERTEX_C_T_V(vert); vert++;

                    index++;
                }
                break;
            }
            default:
            {
                glEnd();
                RWRETURN(FALSE);
                break;
            }
        }
    glEnd();

#ifdef USE_OGL_PIPE
    CLEAN_UP_SCREEN_SPACE_PROJECTION
#endif

#ifdef RWMETRICS
    PRIMITIVE_UPDATE_METRICS
#endif /* RWMETRICS */

    RWRETURN(TRUE);
}


/****************************************************************************
 OpenGLIm2DRenderPrimitive_Gouraud_NoTexture

 On entry   : primitive type, vertices, numVertices
 On exit    :
 */

static RwBool
OpenGLIm2DRenderPrimitive_Gouraud_NoTexture(RwPrimitiveType primType,
                                            RwIm2DVertex *vertices,
                                            RwInt32 numVertices)
{
    RwInt32 index    = 0;
    RwOpenGLVertex *vert = (RwOpenGLVertex*)vertices;

    RWFUNCTION(RWSTRING("OpenGLIm2DRenderPrimitive_Gouraud_NoTexture"));

#ifdef USE_OGL_PIPE
    SET_UP_SCREEN_SPACE_PROJECTION
#endif

    glBegin(PrimLookup[primType]);
        switch (primType)
        {
            case rwPRIMTYPELINELIST:
            {
                while (index < numVertices)
                {
                    GL_VERTEX_C_V(vert); vert++;
                    GL_VERTEX_C_V(vert); vert++;

                    index += 2;
                }
                break;
            }
            case rwPRIMTYPETRILIST:
            {
                while (index < numVertices)
                {
                    GL_VERTEX_C_V(vert); vert++;
                    GL_VERTEX_C_V(vert); vert++;
                    GL_VERTEX_C_V(vert); vert++;

                    index += 3;
                }
                break;
            }
            case rwPRIMTYPEPOLYLINE:
            case rwPRIMTYPETRISTRIP:
            case rwPRIMTYPETRIFAN:
            {
                while (index < numVertices)
                {
                    GL_VERTEX_C_V(vert); vert++;

                    index++;
                }
                break;
            }
            default:
            {
                glEnd();
                RWRETURN(FALSE);
                break;
            }
        }
    glEnd();

#ifdef USE_OGL_PIPE
    CLEAN_UP_SCREEN_SPACE_PROJECTION
#endif

#ifdef RWMETRICS
    PRIMITIVE_UPDATE_METRICS
#endif /* RWMETRICS */

    RWRETURN(TRUE);
}


/****************************************************************************
 OpenGLIm2DRenderPrimitive_Gouraud_Texture

 On entry   : primitive type, vertices, numVertices
 On exit    :
 */

static RwBool
OpenGLIm2DRenderPrimitive_Gouraud_Texture(RwPrimitiveType primType,
                                          RwIm2DVertex *vertices,
                                          RwInt32 numVertices)
{
    RwInt32 index    = 0;
    RwOpenGLVertex *vert = (RwOpenGLVertex*)vertices;

    RWFUNCTION(RWSTRING("OpenGLIm2DRenderPrimitive_Gouraud_Texture"));

#ifdef USE_OGL_PIPE
    SET_UP_SCREEN_SPACE_PROJECTION
#endif

    glBegin(PrimLookup[primType]);
        switch (primType)
        {
            case rwPRIMTYPELINELIST:
            {
                while (index < numVertices)
                {
                    GL_VERTEX_C_T_V(vert); vert++;
                    GL_VERTEX_C_T_V(vert); vert++;

                    index += 2;
                }
                break;
            }
            case rwPRIMTYPETRILIST:
            {
                while (index < numVertices)
                {
                    GL_VERTEX_C_T_V(vert); vert++;
                    GL_VERTEX_C_T_V(vert); vert++;
                    GL_VERTEX_C_T_V(vert); vert++;

                    index += 3;
                }
                break;
            }
            case rwPRIMTYPEPOLYLINE:
            case rwPRIMTYPETRISTRIP:
            case rwPRIMTYPETRIFAN:
            {
                while (index < numVertices)
                {
                    GL_VERTEX_C_T_V(vert); vert++;

                    index++;
                }
                break;
            }
            default:
            {
                glEnd();
                RWRETURN(FALSE);
                break;
            }
        }
    glEnd();

#ifdef USE_OGL_PIPE
    CLEAN_UP_SCREEN_SPACE_PROJECTION
#endif

#ifdef RWMETRICS
    PRIMITIVE_UPDATE_METRICS
#endif /* RWMETRICS */

    RWRETURN(TRUE);
}







/****************************************************************************
 OpenGLIm2DRenderIndexedPrimitive_NoGouraud_NoTexture

 On entry   : primitive type, vertices, numVertices
 On exit    :
 */

static RwBool
OpenGLIm2DRenderIndexedPrimitive_NoGouraud_NoTexture(RwPrimitiveType primType,
                                                     RwIm2DVertex *vertices,
                                                     RwInt32 numVertices __RWUNUSED__,
                                                     RwImVertexIndex *indices,
                                                     RwInt32 numIndices)
{
    RwInt32 index = 0;
    RwOpenGLVertex *vert1, *vert2, *vert3;

    RwOpenGLVertex *verts       = (RwOpenGLVertex*)vertices;
    RwImVertexIndex *inds   = (RwImVertexIndex *)indices;

    RWFUNCTION(RWSTRING("OpenGLIm2DRenderIndexedPrimitive_NoGouraud_NoTexture"));

#ifdef USE_OGL_PIPE
    SET_UP_SCREEN_SPACE_PROJECTION
#endif

    glBegin(PrimLookup[primType]);
        switch (primType)
        {
            case rwPRIMTYPELINELIST:
            {
                while (index < numIndices)
                {
                    vert1 = verts + inds[index];
                    vert2 = verts + inds[index + 1];

                    GL_VERTEX_V(vert1);
                    GL_VERTEX_C_V(vert2); /* Flat shaded so don't bother with specifying a vertex colour */

                    index += 2;
                }
                break;
            }
            case rwPRIMTYPEPOLYLINE:
            {
                vert1 = verts + inds[0];
                GL_VERTEX_V(vert1);

                index = 1;
                while (index < numIndices)
                {
                    vert1 = verts + inds[index];
                    GL_VERTEX_C_V(vert1);

                    index++;
                }
                break;
            }
            case rwPRIMTYPETRILIST:
            {
                while (index < numIndices)
                {
                    vert1 = verts + inds[index];
                    vert2 = verts + inds[index + 1];
                    vert3 = verts + inds[index + 2];

                    GL_VERTEX_V(vert1);
                    GL_VERTEX_V(vert2);
                    GL_VERTEX_C_V(vert3);

                    index += 3;
                }
                break;
            }
            case rwPRIMTYPETRISTRIP:
            case rwPRIMTYPETRIFAN:
            {
                vert1 = verts + inds[0];
                vert2 = verts + inds[1];

                GL_VERTEX_V(vert1);
                GL_VERTEX_V(vert2);

                index = 2;
                while (index < numIndices)
                {
                    vert1 = verts + inds[index];
                    GL_VERTEX_C_V(vert1);

                    index++;
                }
                break;
            }
            default:
            {
                glEnd();
                RWRETURN(FALSE);
                break;
            }
        }
    glEnd();


#ifdef USE_OGL_PIPE
    CLEAN_UP_SCREEN_SPACE_PROJECTION
#endif

#ifdef RWMETRICS
    INDEXED_PRIMITIVE_UPDATE_METRICS
#endif /* RWMETRICS */

    RWRETURN(TRUE);
}


/****************************************************************************
 OpenGLIm2DRenderIndexedPrimitive_NoGouraud_Texture

 On entry   : primitive type, vertices, numVertices
 On exit    :
 */

static RwBool
OpenGLIm2DRenderIndexedPrimitive_NoGouraud_Texture(RwPrimitiveType primType,
                                                   RwIm2DVertex *vertices,
                                                   RwInt32 numVertices __RWUNUSED__,
                                                   RwImVertexIndex *indices,
                                                   RwInt32 numIndices)
{
    RwInt32 index = 0;
    RwOpenGLVertex *vert1, *vert2, *vert3;

    RwOpenGLVertex *verts       = (RwOpenGLVertex*)vertices;
    RwImVertexIndex *inds   = (RwImVertexIndex *)indices;

    RWFUNCTION(RWSTRING("OpenGLIm2DRenderIndexedPrimitive_NoGouraud_Texture"));

#ifdef USE_OGL_PIPE
    SET_UP_SCREEN_SPACE_PROJECTION
#endif

    glBegin(PrimLookup[primType]);
        switch (primType)
        {
            case rwPRIMTYPELINELIST:
            {
                while (index < numIndices)
                {
                    vert1 = verts + inds[index];
                    vert2 = verts + inds[index + 1];

                    GL_VERTEX_T_V(vert1);
                    GL_VERTEX_C_T_V(vert2); /* Flat shaded so don't bother with specifying a vertex colour */

                    index += 2;
                }
                break;
            }
            case rwPRIMTYPEPOLYLINE:
            {
                vert1 = verts + inds[0];
                GL_VERTEX_T_V(vert1);

                index = 1;
                while (index < numIndices)
                {
                    vert1 = verts + inds[index];

                    GL_VERTEX_C_T_V(vert1);

                    index++;
                }
                break;
            }
            case rwPRIMTYPETRILIST:
            {
                while (index < numIndices)
                {
                    vert1 = verts + inds[index];
                    vert2 = verts + inds[index + 1];
                    vert3 = verts + inds[index + 2];

                    GL_VERTEX_T_V(vert1);
                    GL_VERTEX_T_V(vert2);
                    GL_VERTEX_C_T_V(vert3);

                    index += 3;
                }
                break;
            }
            case rwPRIMTYPETRISTRIP:
            case rwPRIMTYPETRIFAN:
            {
                vert1 = verts + inds[0];
                vert2 = verts + inds[1];
                GL_VERTEX_T_V(vert1);
                GL_VERTEX_T_V(vert2);

                index = 2;
                while (index < numIndices)
                {
                    vert1 = verts + inds[index];
                    GL_VERTEX_C_T_V(vert1);

                    index++;
                }
                break;
            }
            default:
            {
                glEnd();
                RWRETURN(FALSE);
                break;
            }
        }
    glEnd();

#ifdef USE_OGL_PIPE
    CLEAN_UP_SCREEN_SPACE_PROJECTION
#endif

#ifdef RWMETRICS
    INDEXED_PRIMITIVE_UPDATE_METRICS
#endif /* RWMETRICS */

    RWRETURN(TRUE);
}



/****************************************************************************
 OpenGLIm2DRenderIndexedPrimitive_Gouraud_NoTexture

 On entry   : primitive type, vertices, numVertices
 On exit    :
 */

static RwBool
OpenGLIm2DRenderIndexedPrimitive_Gouraud_NoTexture(RwPrimitiveType primType,
                                                   RwIm2DVertex *vertices,
                                                   RwInt32 numVertices __RWUNUSED__,
                                                   RwImVertexIndex *indices,
                                                   RwInt32 numIndices)
{
    RwInt32 index = 0;
    RwOpenGLVertex *vert1, *vert2, *vert3;

    RwOpenGLVertex *verts       = (RwOpenGLVertex*)vertices;
    RwImVertexIndex *inds   = (RwImVertexIndex *)indices;

    RWFUNCTION(RWSTRING("OpenGLIm2DRenderIndexedPrimitive_Gouraud_NoTexture"));

#ifdef USE_OGL_PIPE
    SET_UP_SCREEN_SPACE_PROJECTION
#endif

    glBegin(PrimLookup[primType]);
        switch (primType)
        {
            case rwPRIMTYPELINELIST:
            {
                while (index < numIndices)
                {
                    vert1 = verts + inds[index];
                    vert2 = verts + inds[index + 1];

                    GL_VERTEX_C_V(vert1);
                    GL_VERTEX_C_V(vert2);

                    index += 2;
                }
                break;
            }
            case rwPRIMTYPETRILIST:
            {
                while (index < numIndices)
                {
                    vert1 = verts + inds[index];
                    vert2 = verts + inds[index + 1];
                    vert3 = verts + inds[index + 2];

                    GL_VERTEX_C_V(vert1);
                    GL_VERTEX_C_V(vert2);
                    GL_VERTEX_C_V(vert3);

                    index += 3;
                }
                break;
            }
            case rwPRIMTYPEPOLYLINE:
            case rwPRIMTYPETRISTRIP:
            case rwPRIMTYPETRIFAN:
            {
                while (index < numIndices)
                {
                    vert1 = verts + inds[index];
                    GL_VERTEX_C_V(vert1);

                    index++;
                }
                break;
            }
            default:
            {
                glEnd();
                RWRETURN(FALSE);
                break;
            }
        }
    glEnd();

#ifdef USE_OGL_PIPE
    CLEAN_UP_SCREEN_SPACE_PROJECTION
#endif

#ifdef RWMETRICS
    INDEXED_PRIMITIVE_UPDATE_METRICS
#endif /* RWMETRICS */

    RWRETURN(TRUE);
}


/****************************************************************************
 OpenGLIm2DRenderIndexedPrimitive_Gouraud_Texture

 On entry   : primitive type, vertices, numVertices
 On exit    :
 */

static RwBool
OpenGLIm2DRenderIndexedPrimitive_Gouraud_Texture(RwPrimitiveType primType,
                                                 RwIm2DVertex *vertices,
                                                 RwInt32 numVertices __RWUNUSED__,
                                                 RwImVertexIndex *indices,
                                                 RwInt32 numIndices)
{
    RwInt32 index = 0;
    RwOpenGLVertex *vert1, *vert2, *vert3;

    RwOpenGLVertex *verts       = (RwOpenGLVertex*)vertices;
    RwImVertexIndex *inds   = (RwImVertexIndex *)indices;

    RWFUNCTION(RWSTRING("OpenGLIm2DRenderIndexedPrimitive_Gouraud_Texture"));

#ifdef USE_OGL_PIPE
    SET_UP_SCREEN_SPACE_PROJECTION
#endif

    glBegin(PrimLookup[primType]);
        switch (primType)
        {
            case rwPRIMTYPELINELIST:
            {
                while (index < numIndices)
                {
                    vert1 = verts + inds[index];
                    vert2 = verts + inds[index + 1];

                    GL_VERTEX_C_T_V(vert1);
                    GL_VERTEX_C_T_V(vert2);

                    index += 2;
                }
                break;
            }
            case rwPRIMTYPETRILIST:
            {
                while (index < numIndices)
                {
                    vert1 = verts + inds[index];
                    vert2 = verts + inds[index + 1];
                    vert3 = verts + inds[index + 2];

                    GL_VERTEX_C_T_V(vert1);
                    GL_VERTEX_C_T_V(vert2);
                    GL_VERTEX_C_T_V(vert3);

                    index += 3;
                }
                break;
            }
            case rwPRIMTYPEPOLYLINE:
            case rwPRIMTYPETRISTRIP:
            case rwPRIMTYPETRIFAN:
            {
                while (index < numIndices)
                {
                    vert1 = verts + inds[index];
                    GL_VERTEX_C_T_V(vert1);

                    index++;
                }
                break;
            }
            default:
            {
                glEnd();
                RWRETURN(FALSE);
                break;
            }
        }
    glEnd();

#ifdef USE_OGL_PIPE
    CLEAN_UP_SCREEN_SPACE_PROJECTION
#endif

#ifdef RWMETRICS
    INDEXED_PRIMITIVE_UPDATE_METRICS
#endif /* RWMETRICS */

    RWRETURN(TRUE);
}

RwIm2DRenderPrimitiveFunction
    _rwOpenGLRenderPrimitiveFuncs[4] =
    {
        OpenGLIm2DRenderPrimitive_NoGouraud_NoTexture,
        OpenGLIm2DRenderPrimitive_NoGouraud_Texture,
        OpenGLIm2DRenderPrimitive_Gouraud_NoTexture,
        OpenGLIm2DRenderPrimitive_Gouraud_Texture
    };

RwIm2DRenderIndexedPrimitiveFunction
   _rwOpenGLRenderIndexedPrimitiveFuncs[4] =
    {
        OpenGLIm2DRenderIndexedPrimitive_NoGouraud_NoTexture,
        OpenGLIm2DRenderIndexedPrimitive_NoGouraud_Texture,
        OpenGLIm2DRenderIndexedPrimitive_Gouraud_NoTexture,
        OpenGLIm2DRenderIndexedPrimitive_Gouraud_Texture
    };

/****************************************************************************
 _rwOpenGLIm2DSetSubmitFuncs

 Set the submission callbacks for 2D in response to changes in render state

 On entry   : None
 On exit    : None
 */

void
_rwOpenGLIm2DSetSubmitFuncs(RwBool gouraudOn, RwBool textureOn)
{
    RwUInt32 mode;

    RWFUNCTION(RWSTRING("_rwOpenGLIm2DSetSubmitFuncs"));

    mode = ((gouraudOn) ? 2 : 0) | ((textureOn) ? 1 : 0);
    GlobalDevice->fpIm2DRenderPrimitive        = _rwOpenGLRenderPrimitiveFuncs[mode];
    GlobalDevice->fpIm2DRenderIndexedPrimitive = _rwOpenGLRenderIndexedPrimitiveFuncs[mode];

    RWRETURNVOID();
}


/****************************************************************************
 _rwOpenGLIm2DSetVertexFuncs

 Set the vertex specification functions for 2D in response to changes in render state

 On entry   : zTestOn - Z-tests are enabled if TRUE
              zWriteOn - Z-writing is enabled if TRUE
              alphaVertexOn - Vertex color contains alpha if TRUE
              fogOn - Fogging is enabled if TRUE

 On exit    : None
 */

void
_rwOpenGLIm2DSetVertexFuncs( RwBool zTestOn,
                             RwBool zWriteOn,
                             RwBool alphaVertexOn,
                             RwBool fogOn )
{
    RWFUNCTION(RWSTRING("_rwOpenGLIm2DSetVertexFuncs"));

    if (alphaVertexOn)
    {
        ColorFunc = glColor4ubv;
    }
    else
    {
        ColorFunc = glColor3ubv;
    }

    if ( zTestOn || zWriteOn || fogOn )
    {
        VertexFunc = glVertex3fv;
    }
    else
    {
        VertexFunc = glVertex2fv;
    }

    RWRETURNVOID();
}


/****************************************************************************
 _rwOpenGLIm2DOpen

 Initialise stuff local to this module

 On entry   : None
 On exit    : None
 */

void
_rwOpenGLIm2DOpen(RwDevice *device)
{
    RWFUNCTION(RWSTRING("_rwOpenGLIm2DOpen"));
    RWASSERT(device);

    GlobalDevice = device;

    /* Set up the primitive conversion table */
    memset(PrimLookup, 0, sizeof(PrimLookup));
    PrimLookup[rwPRIMTYPELINELIST] = GL_LINES;
    PrimLookup[rwPRIMTYPEPOLYLINE] = GL_LINE_STRIP;
    PrimLookup[rwPRIMTYPETRILIST]  = GL_TRIANGLES;
    PrimLookup[rwPRIMTYPETRISTRIP] = GL_TRIANGLE_STRIP;
    PrimLookup[rwPRIMTYPETRIFAN]   = GL_TRIANGLE_FAN;

    /* Set up the 2D Immediate mode render functions */
    GlobalDevice->fpIm2DRenderLine     = OpenGLIm2DRenderLine;
    GlobalDevice->fpIm2DRenderTriangle = OpenGLIm2DRenderTriangle;

    _rwOpenGLIm2DSetSubmitFuncs(TRUE, FALSE);
    _rwOpenGLIm2DSetVertexFuncs(TRUE, TRUE, FALSE, FALSE);

    RWRETURNVOID();
}

/****************************************************************************
 _rwOpenGLIm2DClose

 De-initialise stuff local to this module

 On entry   : None
 On exit    : None
 */

void
_rwOpenGLIm2DClose(void)
{
    RWFUNCTION(RWSTRING("_rwOpenGLIm2DClose"));
    RWRETURNVOID();
}
