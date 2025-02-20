#ifndef _PATCHEXPANDER_H
#define _PATCHEXPANDER_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rtbezpat.h"
#include "rppatch.h"

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/
#define rpPATCHNUMCONTROLINDICES rpQUADPATCHNUMCONTROLINDICES

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/
typedef struct RpPatchInstanceMesh RpPatchInstanceMesh;
struct RpPatchInstanceMesh
{
    RxVertexIndex *indices;
    RwUInt32       numIndices;
    RwUInt32       minVert;
    RwUInt32       numVerts;
    RpMaterial    *material;
    RpMesh        *mesh;
};

typedef struct RpPatchInstanceAtomic RpPatchInstanceAtomic;
struct RpPatchInstanceAtomic
{
    RwUInt32 serialNum; /* Serial # - combination of elements contributing to
                         * instanced data.  Enables us to detect when a
                         * re-instance is necessary.
                         */

    RxVertexIndex *indices; /* Vertex index buffer. */

    RpPatchMeshFlag meshFlags;  /* Format of the vertex */
    RwUInt32        stride;     /* Size of each vertex in bytes */

    void *vertices;       /* We also keep the vertices around for
                           * generic nodes and software lighting
                           * to play around with.
                           */

    RwInt32 numMeshes;    /* The number of meshes.        */
    RwInt32 totalIndices; /* The total number of indices. */
    RwInt32 totalVerts;   /* The total number of verts.   */
    RwInt32 res;          /* Resolution of the facetting. */
    RpPatchInstanceMesh meshes[1]; /* The meshes. */
};

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Functions -----------------------------------------------------*
 *===========================================================================*/
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern RwV3d*
_rpPatchRefineTriVector(RwV3d *vert, RwUInt32 stride,
                        RtBezierMatrix cptPos,
                        RwInt32 res);

extern RwRGBA*
_rpPatchRefineTriColor(RwRGBA *color, RwUInt32 stride,
                       RtBezierMatrix cptCol,
                       RwInt32 res);

extern RwTexCoords*
_rpPatchRefineTriTexCoords(RwTexCoords *texCoords, RwUInt32 stride,
                           const RwTexCoords *cptTexCoords,
                           RwInt32 res);

extern RwV3d *
_rpPatchRefineQuadVector(RwV3d *vert, RwUInt32 stride,
                         RtBezierMatrix cptVec,
                         RwInt32 res);

extern RwRGBA*
_rpPatchRefineQuadColor(RwRGBA *color, RwUInt32 stride,
                        RtBezierMatrix cptCol,
                        RwInt32 res);

extern RwTexCoords*
_rpPatchRefineQuadTexCoords(RwTexCoords *texCoords, RwUInt32 stride,
                            const RwTexCoords *cptTexCoords,
                            RwInt32 res);

extern RxVertexIndex *
_rpPatchGenerateTriIndex(RxVertexIndex *indices,
                         RwInt32 base,
                         RwInt32 res);

extern RxVertexIndex*
_rpPatchGenerateQuadIndex(RxVertexIndex *indices,
                          RwInt32 base,
                          RwInt32 res);

extern RwBool
_rpPatchInstanceAtomic( RpPatchInstanceAtomic *instAtomic,
                        RpGeometry *geom,
                        RwInt32 res );

extern RwBool
_rpPatchReInstanceAtomic( RpPatchInstanceAtomic *instAtomic,
                        const RpGeometry *geom,
                        RwInt32 res );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PATCHEXPANDER_H */
