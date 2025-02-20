#ifndef PATCHGEOMETRY_H
#define PATCHGEOMETRY_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpskin.h"
#include "rppatch.h"

#include "patchmesh.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/
typedef struct PatchGeometryData PatchGeometryData;
struct PatchGeometryData
{
    PatchMesh *mesh;
    RpSkin *skin;
};

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/
#define rpQUADPATCHNUMVERTEXINDICES rpQUADPATCHNUMCONTROLINDICES
#define rpTRIPATCHNUMVERTEXINDICES rpQUADPATCHNUMCONTROLINDICES

#define rpPATCHTRIPATCHINDEX (5)
#define rpPATCHTRIPATCHFLAG (1e4)
#define rpPATCHQUADPATCHFLAG (0)
#define rpPATCHTRIPATCHUNUSEDINDEX (0)

#define _rpTriPatchAssignVertexIndices(triPatch, meshIndices)               \
MACRO_START                                                                 \
{                                                                           \
    RwUInt32 target = 0;                                                    \
    RwUInt32 source = 0;                                                    \
                                                                            \
    RWASSERT(NULL != triPatch);                                             \
    RWASSERT(NULL != meshIndices);                                          \
                                                                            \
    meshIndices[target++] = (RxVertexIndex)triPatch->cpIndices[source++];   \
    meshIndices[target++] = (RxVertexIndex)triPatch->cpIndices[source++];   \
    meshIndices[target++] = (RxVertexIndex)triPatch->cpIndices[source++];   \
    meshIndices[target++] = (RxVertexIndex)triPatch->cpIndices[source++];   \
    meshIndices[target++] = (RxVertexIndex)triPatch->cpIndices[source++];   \
    meshIndices[target++] = (RxVertexIndex)triPatch->cpIndices[source++];   \
    meshIndices[target++] = (RxVertexIndex)triPatch->cpIndices[source++];   \
    meshIndices[target++] = (RxVertexIndex)triPatch->cpIndices[source++];   \
    meshIndices[target++] = (RxVertexIndex)triPatch->cpIndices[source++];   \
    meshIndices[target++] = (RxVertexIndex)triPatch->cpIndices[source++];   \
    meshIndices[target++] = rpPATCHTRIPATCHUNUSEDINDEX;                     \
    meshIndices[target++] = rpPATCHTRIPATCHUNUSEDINDEX;                     \
    meshIndices[target++] = rpPATCHTRIPATCHUNUSEDINDEX;                     \
    meshIndices[target++] = rpPATCHTRIPATCHUNUSEDINDEX;                     \
    meshIndices[target++] = rpPATCHTRIPATCHUNUSEDINDEX;                     \
    meshIndices[target++] = rpPATCHTRIPATCHUNUSEDINDEX;                     \
                                                                            \
    RWASSERT(rpTRIPATCHNUMCONTROLINDICES == source);                        \
    RWASSERT(rpTRIPATCHNUMVERTEXINDICES == target);                         \
}                                                                           \
MACRO_STOP

#define _rpQuadPatchAssignVertexIndices(quadPatch, meshIndices)             \
MACRO_START                                                                 \
{                                                                           \
    RwUInt32 index;                                                         \
                                                                            \
    RWASSERT(NULL != quadPatch);                                            \
    RWASSERT(NULL != meshIndices);                                          \
                                                                            \
    /* Copy the control indices. */                                         \
    for( index = 0; index < rpQUADPATCHNUMCONTROLINDICES; index++ )         \
    {                                                                       \
        meshIndices[index] =                                                \
            (RxVertexIndex)quadPatch->cpIndices[index];                     \
    }                                                                       \
}                                                                           \
MACRO_STOP

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern RpGeometry *
_rpPatchGeometryCreateFromPatchMesh( PatchMesh *patchMesh );

extern PatchMesh *
_rpPatchGeometryTransferFromPatchMesh( PatchMesh *patchMesh );

extern void *
_rpPatchGeometryConstructor( void *object,
                             RwInt32 offset,
                             RwInt32 size );

extern void *
_rpPatchGeometryDestructor( void *object,
                            RwInt32 offset,
                            RwInt32 size );

extern void *
_rpPatchGeometryCopy( void *dstObject,
                      const void *srcObject,
                      RwInt32 offset,
                      RwInt32 size );

extern RwInt32
_rpPatchGeometryGetSize( const void *atomic,
                         RwInt32 offset,
                         RwInt32 size );

extern RwStream *
_rpPatchGeometryRead( RwStream *stream,
                      RwInt32 length,
                      void *atomic,
                      RwInt32 offset,
                      RwInt32 size );

extern RwStream *
_rpPatchGeometryWrite( RwStream *stream,
                       RwInt32 length,
                       const void *atomic,
                       RwInt32 offset,
                       RwInt32 size );

extern RwBool
_rpPatchGeometryAlways( void *object,
                        RwInt32 offset,
                        RwInt32 size );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PATCHGEOMETRY_H */
