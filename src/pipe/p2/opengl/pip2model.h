/***********************************************************************
 *
 * Module:  pip2model.h
 *
 * Purpose: Pipe structure definitions
 *
 ***********************************************************************/

#if !defined( PIP2MODEL_H )
#define PIP2MODEL_H

/* =====================================================================
 *  Includes
 * ===================================================================== */
#include "bacolor.h"
#include "bavector.h"

/* RWPUBLIC */

/* =====================================================================
 *  Defines
 * ===================================================================== */

#define RXHEAPPLATFORMDEFAULTSIZE  (1 << 12) /* 4k */

/* --- MISCELLANEOUS --- */
#define RwIm3DVertexGetNext(_vert)  ((_vert) + 1)

/* --- POSITION --- */
#define RxObjSpace3DVertexGetPos(_vert,_pos)    \
    RwV3dAssign( (RwV3d *)(_pos),               \
                 (const RwV3d *)(&((_vert)->position)) )

#define RxObjSpace3DLitVertexGetPos(_vert,_pos)     \
    RxObjSpace3DVertexGetPos(_vert,_pos)

#define RwIm3DVertexGetPos(_vert)   \
    (&((_vert)->position))

#define RxObjSpace3DVertexSetPos(_vert,_pos)        \
    RwV3dAssign( (RwV3d *)(&((_vert)->position)),   \
                 (const RwV3d *)(_pos) )

#define RxObjSpace3DLitVertexSetPos(_vert,_pos)     \
    RxObjSpace3DVertexSetPos(_vert,_pos)

#define RwIm3DVertexSetPos(_vert,_imx,_imy,_imz)    \
MACRO_START                                         \
{                                                   \
    RwV3d   tempV3d;                                \
    tempV3d.x = (_imx);                             \
    tempV3d.y = (_imy);                             \
    tempV3d.z = (_imz);                             \
    RxObjSpace3DLitVertexSetPos((_vert),&tempV3d);  \
}                                                   \
MACRO_STOP

/* --- PRELIGHT --- */
#define RxObjSpace3DVertexGetPreLitColor(_vert,_col)    \
    RwRGBAAssign( (RwRGBA *)(_col),                     \
                  (const RwRGBA *)(&((_vert)->color)) )

#define RxObjSpace3DVertexSetPreLitColor(_vert,_col)    \
    RwRGBAAssign( (RwRGBA *)(&((_vert)->color)),        \
                  (const RwRGBA *)(_col) )

/* --- COLOR --- */
#define RxObjSpace3DVertexGetColor  \
    RxObjSpace3DVertexGetPreLitColor

#define RxObjSpace3DLitVertexSetColor   \
    RxObjSpace3DVertexSetPreLitColor

#define RwIm3DVertexSetRGBA(_vert,_imr,_img,_imb,_ima)  \
MACRO_START                                             \
{                                                       \
    RwRGBA tempRGBA;                                    \
    tempRGBA.red   = (_imr);                            \
    tempRGBA.green = (_img);                            \
    tempRGBA.blue  = (_imb);                            \
    tempRGBA.alpha = (_ima);                            \
    RxObjSpace3DLitVertexSetColor(_vert, &tempRGBA);    \
}                                                       \
MACRO_STOP

/* --- NORMALS --- */
#define RxObjSpace3DVertexGetNormal(_vert,_normal)  \
    RwV3dAssign( (RwV3d *)(_normal),                \
                 (const RwV3d *)(&((_vert)->normal)) )

#define RwIm3DVertexGetNormal(_vert)   \
    (RwV3d *)( &((_vert)->normal) )

#define RxObjSpace3DVertexSetNormal(_vert,_normal)  \
    RwV3dAssign( (RwV3d *)(&((_vert)->normal)),     \
                 (const RwV3d *)(_normal) )

#define RwIm3DVertexSetNormal(_vert,_imnx,_imny,_imnz)  \
MACRO_START                                             \
{                                                       \
    RwV3d   tempV3d;                                    \
    tempV3d.x = (_imnx);                                \
    tempV3d.y = (_imny);                                \
    tempV3d.z = (_imnz);                                \
    RxObjSpace3DVertexSetNormal((_vert),&tempV3d);      \
}                                                       \
MACRO_STOP

/* --- TEXTURE COORDINATES --- */
#define RxObjSpace3DVertexGetU(_vert)   \
    ((_vert)->texCoords.u)

#define RxObjSpace3DVertexGetV(_vert)    \
    ((_vert)->texCoords.v)

#define RxObjSpace3DVertexSetU(_vert,_imu)   \
    ((_vert)->texCoords.u = (_imu))

#define RxObjSpace3DLitVertexSetU(_vert,_imu)   \
    RxObjSpace3DVertexSetU(_vert,_imu)

#define RwIm3DVertexSetU    \
    RxObjSpace3DLitVertexSetU

#define RxObjSpace3DVertexSetV(_vert,_imv)   \
    ((_vert)->texCoords.v = (_imv))

#define RxObjSpace3DLitVertexSetV(_vert,_imv)   \
    RxObjSpace3DVertexSetV(_vert,_imv)

#define RwIm3DVertexSetV    \
    RxObjSpace3DLitVertexSetV


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */

/**
 * \ingroup rwcoregeneric
 * \struct RxObjSpace3DVertex
 * Typedef for an RxObjSpace3DVertex.
 */
typedef struct RxObjSpace3DVertex RxObjSpace3DVertex;

/*
 * Structure representing object space vertex.
 */
#if (!defined(DOXYGEN))
struct RxObjSpace3DVertex
{
    RwV3d       position;   /* RwV3d containing the position of the vertex */

    RwV3d       normal;     /* RwV3d containing the vertex normal */

    RwRGBA      color;      /* RwRGBA containing the vertex color */

    RwTexCoords texCoords;  /* RwTexCoords containing one set of texture coordinates for the vertex */
};
#endif /* (!defined(DOXYGEN)) */

/* this vertex should not be truncated */
#define RxObjSpace3DVertexNoUVsNoNormalsSize    (sizeof(RxObjSpace3DVertex))
#define RxObjSpace3DVertexNoUVsSize             (sizeof(RxObjSpace3DVertex))
#define RxObjSpace3DVertexFullSize              (sizeof(RxObjSpace3DVertex))

/**
 * \ingroup rwcoregeneric
 * \ref RxObjSpace3DLitVertex
 * Typedef for an RxObjSpace3DLitVertex
 */
typedef RxObjSpace3DVertex RxObjSpace3DLitVertex;

/**
 * \ingroup rwim3dvertex
 * \ref RwIm3DVertex
 * Typedef for an RwIm3DVertex
 */
typedef RxObjSpace3DLitVertex RwIm3DVertex;

/**
 * \ingroup rwcoregeneric
 * \ref RxScrSpace2DVertex
 * Typedef for an RxScrSpace2DVertex structure
 */
typedef RwIm2DVertex RxScrSpace2DVertex;

/* RWPUBLICEND */

/* =====================================================================
 *  Extern variables
 * ===================================================================== */


/* =====================================================================
 *  Extern function prototypes
 * ===================================================================== */

#if defined( __cplusplus )
extern "C"
{
#endif /* defined( __cplusplus ) */

#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

#endif /* !defined( PIP2MODEL_H ) */
