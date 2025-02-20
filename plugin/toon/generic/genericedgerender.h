/*****************************************************************************

    File: genericedgerender.h

    Purpose: A short description of the file.

    Copyright (c) 2002 Criterion Software Ltd.

 */

#ifndef GENERICEDGERENDER_H
#define GENERICEDGERENDER_H

/*****************************************************************************
 Includes
 */
#include "toonink.h"
#include "toonpaint.h"
#include "toonmaterial.h"
#include "toongeo.h"

/*****************************************************************************
 Defines
 */
/* limits we're stuck with because of renderware geometries/world sectors */
#define RPTOON_MAX_VERTICES    65535
#define RPTOON_MAX_FACES       65535
#define RPTOON_MAX_EDGES       65535

/* threshold to check whether edge is too short to attempt to normalize */
#define RPTOON_REALLY_SMALL_EDGE_THRESHOLD 0.0001f

/*****************************************************************************
 Enums
 */

/*****************************************************************************
 Typedef Enums
 */

/*****************************************************************************
 Typedef Structs
 */
typedef struct ScreenSpaceProjectionTag ScreenSpaceProjection;

/*****************************************************************************
 Function Pointers
 */

/*****************************************************************************
 Structs
 */
/* project a 3D vertex down to 2D */
struct ScreenSpaceProjectionTag
{
    RwMatrix matrix;
};

#ifdef     __cplusplus
extern "C"
{
#endif  /* __cplusplus */

/*****************************************************************************
 Global Variables
 */
/* Generic 3D immediate mode wrapper for edge rendering */
extern RwIm3DVertex *_rpToonGenericVertexBuffer;

/*****************************************************************************
 Function prototypes
 */
extern void _rpToonScreenSpaceProjectionInit(ScreenSpaceProjection *projection,
                                             const RwMatrix *transform);

extern void _rpToonGenericEdgeRenderInit(void);
extern void _rpToonGenericEdgeRenderClean(void);
extern void _rpToonGenericEdgeRenderPushState(void);
extern void _rpToonGenericEdgeRenderSubmit(RwInt32 numVerts,
                                           RwPrimitiveType primType);
extern void _rpToonGenericEdgeRenderPopState(void);

/*****************************************************************************
 Functions
 */
extern void
ScreenSpaceProjectionProjectPoint(RwV2d *v0p,
                                  RwV3d *v0,
                                  const ScreenSpaceProjection *projection,
                                  const RwV3d *point);

extern RwReal
_rpToonComputePerspectiveScale(const RpToonInk *ink,
                               RwReal camSpaceZ);

extern void
BackProject(RwV3d *vertex);

extern RwIm3DVertex *
_rpToonGenericEdgeRenderAppendIm3dQuad(RwIm3DVertex *ptr,
                                       const RwV3d *v0p,
                                       const RwV3d *v1p,
                                       const RwV3d *v2p,
                                       const RwV3d *v3p,
                                       RwRGBA color);

#ifdef    __cplusplus
}
#endif /* __cplusplus */

#endif /* GENERICEDGERENDER_H */
