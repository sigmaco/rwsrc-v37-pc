/*
 *  dmphgeom.h - delta morph geometry data and morph targets
 */

#if (!defined(_DMPHGEOM_H))
#define _DMPHGEOM_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/

#include "rwcore.h"
#include "rpworld.h"

#if defined(PLATFORM_DATA)
#include "dmorphdata.h"
#endif /* defined(PLATFORM_DATA) */

/*===========================================================================*
 *--- Defines ---------------------------------------------------------------*
 *===========================================================================*/

#define rpDMORPHRLEMAXPACKETSIZE    127

/*===========================================================================*
 *--- Types -----------------------------------------------------------------*
 *===========================================================================*/

/* RWPUBLIC */
/*--- rpDMorphRLE --------------------------------------------------------
 *
 * Contains the run length encoding of the delta data.
 */
typedef struct rpDMorphRLE rpDMorphRLE;

#if (!defined(DOXYGEN))
struct rpDMorphRLE
{
    RwUInt8            *code;
    RwUInt32            numCodeElements;
    RwUInt32            numDataElements;
    void               *data;
};
#endif /* (!defined(DOXYGEN)) */

/*--- _RpDMorphTarget -------------------------------------------------------*/

/**
 * \ingroup rpdmorph
 * \struct RpDMorphTarget
 * Delta morph target object for defining a target for
 * a base geometry.
 * This should be considered an opaque type.
 * Use the RpDMorphGeometry and RpDMorphTarget API
 * functions to access.
 */
typedef struct RpDMorphTarget RpDMorphTarget;

#if (!defined(DOXYGEN))
struct RpDMorphTarget
{
    /*--- DMorphTarget ---*/
    RpGeometry            *geometry;
    RwChar                *name;
    RwUInt32              flags;
    RwUInt32              lockFlags;
    rpDMorphRLE           rle;
    /*--- DMorphGeometry ---*/
    RwV3d                 *vertices;
    RwV3d                 *normals;
    RwRGBA                *preLightColors;
    RwTexCoords           *texCoords;
    /*--- Bounding sphere ---*/
    RwSphere              boundingSphere;
};
#endif /* (!defined(DOXYGEN)) */

/*--- rpDMorphGeometryData -------------------------------------------------
 *
 * This is an extension to RpGeometry. This defines the number of
 * DMorphTargets.
 */
typedef struct rpDMorphGeometryData rpDMorphGeometryData;

#if (!defined(DOXYGEN))
struct rpDMorphGeometryData
{
    /*--- DMorphTargets ---*/
    RwUInt32            numDMorphTargets;
    RpDMorphTarget      *dMorphTargets;

    /*--- Current morph state of the geometry ---*/
    RwReal              *currValues;

#if defined(PLATFORM_DATA)
    /*--- Render callback chaining ---*/
    DMorphPlatformData  platformData;
#endif /* defined(PLATFORM_DATA) */
};
#endif /* (!defined(DOXYGEN)) */

/*===========================================================================*
 *--- Macros ----------------------------------------------------------------*
 *===========================================================================*/

/*--- Geometry data ---*/
#define RPDMORPHGEOMETRYGETDATA(geometry)                               \
    ((rpDMorphGeometryData **)                                          \
     (((RwUInt8 *)geometry)+                                            \
      (rpDMorphGeometryDataOffset)))

#define RPDMORPHGEOMETRYGETCONSTDATA(geometry)                          \
    ((const rpDMorphGeometryData * const *)                             \
     (((const RwUInt8 *)geometry)+                                      \
      (rpDMorphGeometryDataOffset)))
/* RWPUBLICEND */

/*--- Geometry Macros ---
 *
 * These are defined to try and minimize any pain when the Morphing is
 * fully removed from the RpWorld.
 */
#define RPDMORPHGEOMETRYGETVERTICES(geometry)                       \
    ((geometry)->morphTarget->verts)

#define RPDMORPHGEOMETRYGETNORMALS(geometry)                        \
    ((geometry)->morphTarget->normals)

#define RPDMORPHGEOMETRYGETPRELIGHTCOLORS(geometry)                 \
    ((geometry)->preLitLum)

#define RPDMORPHGEOMETRYGETTEXCOORDS(geometry)                      \
    ((geometry)->texCoords[0])

#define RPDMORPHGEOMETRYGETBOUNDINGSPHERE(geometry)                 \
    ((geometry)->morphTarget->boundingSphere)

#define RPDMORPHGEOMETRYGETNUMBEROFVERTICES(geometry)               \
    ((geometry)->numVertices)


/* RWPUBLIC */
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
/* RWPUBLICEND */

/* RWPUBLIC */
/*===========================================================================*
 *--- Global variables ------------------------------------------------------*
 *===========================================================================*/

extern RwInt32 rpDMorphGeometryDataOffset;
/* RWPUBLICEND */

/*===========================================================================*
 *--- Internal plugin functions ---------------------------------------------*
 *===========================================================================*/
extern RwBool _rpDMorphGeometryPluginAttach(void);

/*--- DMorphTarget functions ------------------------------------------------
 *
 * These functios work on the DMorphGeometry level.
 * Each DMorphGeometry has a list of DMorphTargets.
 */

/* RWPUBLIC */
/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/*--- DMorphGeometry functions ----------------------------------------------
 *
 * These functios work on the DMorphGeometry level.
 * Each DMorphGeometry has a list of DMorphTargets.
 */
extern RpGeometry *
RpDMorphGeometryCreateDMorphTargets( RpGeometry *geometry,
                                     RwUInt32 number );

extern RpGeometry *
RpDMorphGeometryDestroyDMorphTargets( RpGeometry *geometry );

extern RpGeometry *
RpDMorphGeometryAddDMorphTarget( RpGeometry  *geometry,
                                 RwUInt32    index,
                                 RwV3d       *vertices,
                                 RwV3d       *normals,
                                 RwRGBA      *preLightColors,
                                 RwTexCoords *texCoords,
                                 RwUInt32    flags );

extern RpGeometry *
RpDMorphGeometryRemoveDMorphTarget( RpGeometry *geometry,
                                    RwUInt32 index );

extern RpDMorphTarget *
RpDMorphGeometryGetDMorphTarget( const RpGeometry *geometry,
                                 RwUInt32 index );

extern RwUInt32
RpDMorphGeometryGetNumDMorphTargets( const RpGeometry *geometry );

extern RpGeometry *
RpDMorphGeometryTransformDMorphTargets( RpGeometry *geometry,
                                        const RwMatrix *matrix );

extern const RwSphere *
RpDMorphTargetGetBoundingSphere( const RpDMorphTarget *dMorphTarget );

extern RpDMorphTarget *
RpDMorphTargetSetName( RpDMorphTarget *dMorphTarget,
                       RwChar *name );

extern RwChar *
RpDMorphTargetGetName( RpDMorphTarget *dMorphTarget );

extern RpGeometryFlag
RpDMorphTargetGetFlags( RpDMorphTarget *dMorphTarget );

#ifdef __cplusplus
}
#endif /* __cplusplus */
/* RWPUBLICEND */

#endif /* _DMPHGEOM_H */
