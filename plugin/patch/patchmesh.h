#ifndef PATCHMESH_H
#define PATCHMESH_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpdbgerr.h"
#include "rppatch.h"

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/
#define _rpPatchMeshGetInternal(patchMesh)                              \
    ((PatchMesh *)patchMesh)

#define _rpPatchMeshGetExternal(patchMesh)                              \
    ((RpPatchMesh *)patchMesh)

#define _rpPatchMeshGetConstInternal(patchMesh)                         \
    ((const PatchMesh *)patchMesh)

/*---------------------------------------------------------------------------*/

#define _rpPatchAssignIndices(src, tar, num)                            \
MACRO_START                                                             \
{                                                                       \
    RwUInt32 index;                                                     \
                                                                        \
    RWASSERT(NULL != src);                                              \
    RWASSERT(NULL != tar);                                              \
    RWASSERT(0 < num);                                                  \
                                                                        \
    for( index = 0; index < num; index++ )                              \
    {                                                                   \
        tar->cpIndices[index] = src->cpIndices[index];                  \
    }                                                                   \
}                                                                       \
MACRO_STOP

#define _rpQuadPatchAssignQuadIndices(quadPatch, patch)                 \
    _rpPatchAssignIndices(quadPatch, patch, rpQUADPATCHNUMCONTROLINDICES)

#define _rpTriPatchAssignTriIndices(triPatch, patch)                    \
    _rpPatchAssignIndices(triPatch, patch, rpTRIPATCHNUMCONTROLINDICES)

/*---------------------------------------------------------------------------*/

#define _rpPatchMeshCheckLocked(mesh, flag)                             \
    (mesh->lockMode & flag)

#define _rpPatchMeshCheckFlag(mesh, _flag)                              \
    (mesh->userMesh.definition.flag & _flag)

#define _rpPatchMeshCheckUserFlag(patchMesh, flag)                      \
    (RpPatchMeshGetFlags(patchMesh) & flag)

/*---------------------------------------------------------------------------*/

#define _rpPatchMeshFlagGetNumTexCoords(flags)                          \
    ((flags & 0xff0000) >> 16)

/*---------------------------------------------------------------------------*/

#define _rpPatchMeshGetNumQuadPatches(mesh)                             \
    (((PatchMesh *)mesh)->userMesh.definition.numQuadPatches)

#define _rpPatchMeshGetNumTriPatches(mesh)                              \
    (((PatchMesh *)mesh)->userMesh.definition.numTriPatches)

#define _rpPatchMeshGetNumControlPoints(mesh)                           \
    (((PatchMesh *)mesh)->userMesh.definition.numControlPoints)

#define _rpPatchMeshGetNumTexCoordSets(mesh)                            \
    (((PatchMesh *)mesh)->userMesh.definition.numTexCoordSets)

#define _rpPatchMeshGetFlags(mesh)                                      \
    (((PatchMesh *)mesh)->userMesh.definition.flag)

#define _rpPatchMeshGetPositions(mesh)                                  \
    (RpPatchMeshGetPositionsMacro(((RpPatchMesh *)mesh)))

#define _rpPatchMeshGetNormals(mesh)                                    \
    (RpPatchMeshGetNormalsMacro(((RpPatchMesh *)mesh)))

#define _rpPatchMeshGetPreLightColors(mesh)                             \
    (RpPatchMeshGetPreLightColorsMacro(((RpPatchMesh *)mesh)))

#define _rpPatchMeshGetTexCoords(mesh, index)                           \
    (RpPatchMeshGetTexCoordsMacro(((RpPatchMesh *)mesh), index + 1))

/*---------------------------------------------------------------------------*/

#define _rpConstPatchMeshGetNumQuadPatches(mesh)                        \
    (((const PatchMesh *)mesh)->userMesh.definition.numQuadPatches)

#define _rpConstPatchMeshGetNumTriPatches(mesh)                         \
    (((const PatchMesh *)mesh)->userMesh.definition.numTriPatches)

#define _rpConstPatchMeshGetNumControlPoints(mesh)                      \
    (((const PatchMesh *)mesh)->userMesh.definition.numControlPoints)

#define _rpConstPatchMeshGetNumTexCoordSets(mesh)                       \
    (((const PatchMesh *)mesh)->userMesh.definition.numTexCoordSets)

#define _rpConstPatchMeshGetFlags(mesh)                                 \
    (((const PatchMesh *)mesh)->userMesh.definition.flag)

#define _rpConstPatchMeshGetPositions(mesh)                             \
    (RpPatchMeshGetPositionsMacro(((const RpPatchMesh *)mesh)))

#define _rpConstPatchMeshGetNormals(mesh)                               \
    (RpPatchMeshGetNormalsMacro(((const RpPatchMesh *)mesh)))

#define _rpConstPatchMeshGetPreLightColors(mesh)                        \
    (RpPatchMeshGetPreLightColorsMacro(((const RpPatchMesh *)mesh)))

#define _rpConstPatchMeshGetTexCoords(mesh, index)                      \
    (RpPatchMeshGetTexCoordsMacro(((const RpPatchMesh *)mesh), index + 1))

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/
typedef struct TriPatch TriPatch;
struct TriPatch
{
    RwUInt32 cpIndices[rpTRIPATCHNUMCONTROLPOINTS];
    RwUInt32 matIndex;
};

typedef struct QuadPatch QuadPatch;
struct QuadPatch
{
    RwUInt32 cpIndices[rpQUADPATCHNUMCONTROLPOINTS];
    RwUInt32 matIndex;
};

typedef struct PatchMesh PatchMesh;
struct PatchMesh
{
    /* External version of our PatchMesh. */
    RpPatchMesh userMesh;

    RwUInt32 lockMode;
    RwUInt32 refCount;

    /* Material list. */
    RpMaterialList matList;

    /* Internal version of our PatchMesh. */
    RpGeometry *geometry;

    /* The patches. */
    QuadPatch *quadPatches;
    TriPatch *triPatches;
};

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern PatchMesh *
_rpPatchMeshCreate( RpPatchMeshDefinition *definition );

extern RwBool
_rpPatchMeshDestroy( PatchMesh *patchMesh );

extern RpGeometry *
_rpPatchMeshGetGeometry( PatchMesh *patchMesh );

extern PatchMesh *
_rpPatchMeshAddRef( PatchMesh *patchMesh );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PATCHMESH_H */
