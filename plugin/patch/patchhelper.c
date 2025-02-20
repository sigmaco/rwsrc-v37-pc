/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   patchhelper.c                                              -*
 *-                                                                         -*
 *-  Purpose :   General patch handling.                                    -*
 *-                                                                         -*
 *===========================================================================*/

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rppatch.h"

#include "patchmesh.h"
#include "patchhelper.h"
#include "patch.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rppatch
 * \ref RpPatchMeshTransform is used to apply the specified transformation
 * matrix to the given patch mesh. The transformation is applied equally to
 * all the control point positions and control point normals. The patch
 * mesh bounding sphere is recalculated after the transform.
 *
 * Note that the transformation modifies the patch mesh data and is
 * permanent. Note also that patch mesh locking and unlocking is performed,
 * as appropriate, before and after applying the transformation.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 * \param matrix    Pointer to the matrix transformation.
 *
 * \return Returns a pointer to the patch mesh if successsful, or NULL if
 * there is an error.
 *
 * \see RpPatchMeshLock
 * \see RpPatchMeshUnlock
 * \see RpPatchMeshCreate
 * \see RpPatchMeshGetPositions
 * \see RpPatchMeshGetNormals
 * \see RpPatchMeshGetNumControlPoints
 */
RpPatchMesh *
RpPatchMeshTransform( RpPatchMesh *patchMesh,
                      const RwMatrix *matrix )
{
    RwV3d *positions;
    RwV3d *normals;

    RwV3d *success;

    RwUInt32 numControlPoints;
    RwUInt32 iNormal;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshTransform"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);
    RWASSERT(NULL != matrix);
    RWASSERT(0 < _rpPatchMeshGetInternal(patchMesh)->refCount);
    RWASSERT(_rpPatchMeshCheckUserFlag(patchMesh, rpPATCHMESHPOSITIONS));
    RWASSERT(0 < RpPatchMeshGetNumControlPoints(patchMesh));

    /* We need to lock the patch meshes' positions and normals. */
    patchMesh = RpPatchMeshLock( patchMesh,
                                 rpPATCHMESHLOCKPOSITIONS |
                                 rpPATCHMESHLOCKNORMALS );
    RWASSERT(NULL != patchMesh);

    /* Get the number of control points in the patch mesh. */
    numControlPoints = RpPatchMeshGetNumControlPoints(patchMesh);

    /* Get the positions. */
    positions = RpPatchMeshGetPositions(patchMesh);
    RWASSERT(NULL != positions);

    /* Get the normals. */
    normals = RpPatchMeshGetNormals(patchMesh);
    RWASSERT(NULL != normals);

    /* Transform the positions by the matrix. */
    success = RwV3dTransformPoints( positions,
                                    positions,
                                    numControlPoints,
                                    matrix );
    RWASSERT(NULL != success);

    /* Now transform the normals. */
    if(NULL != normals)
    {
        RWASSERT(_rpPatchMeshCheckUserFlag(patchMesh, rpPATCHMESHNORMALS));

        success = RwV3dTransformVectors( normals,
                                         normals,
                                         numControlPoints,
                                         matrix );

        /* Should normalize them again. */
        for( iNormal = 0; iNormal < numControlPoints; iNormal++ )
        {
            RwV3dNormalize(normals, normals);
            normals++;
        }
    }

    /* Set the radius */
    /* Bounding sphere should get recalculated in the unlock. */

    /* Can put it back into a device dependent form */
    patchMesh = RpPatchMeshUnlock(patchMesh);
    RWASSERT(NULL != patchMesh);

    RWRETURN(patchMesh);
}

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))

/**
 * \ingroup rppatch
 * \ref RpPatchMeshGetFlags is used to retrieve the property flags associated
 * with the specified patch mesh.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for the final release version of an
 * application.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 *
 * \return Returns an \ref RwUInt32 bit-field whose value is the bit-wise OR of
 * \ref RpPatchMeshFlag values if successful, or zero if there is an error.
 *
 * \see RpPatchMeshLock
 * \see RpPatchMeshUnlock
 * \see RpPatchMeshSetFlags
 */
RwUInt32
RpPatchMeshGetFlags( const RpPatchMesh *patchMesh )
{
    RwUInt32 flags;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshGetFlags"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);

    flags = RpPatchMeshGetFlagsMacro(patchMesh);

    RWRETURN(flags);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshSetFlags is used to modify the property flags for the given
 * patch mesh.
 *
 * Note that the new flag settings will completely replace the existing ones.
 * Developers can logically OR (or add) flags together to combine them.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for the final release version of an
 * application.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 * \param flags     An \ref RwUInt32 bit-field whose value is the bit-wise OR of
 *                  \ref RpPatchMeshFlag values specifying the patch mesh's
 *                  properties.
 *
 * \see RpPatchMeshLock
 * \see RpPatchMeshUnlock
 * \see RpPatchMeshGetFlags
 */
RpPatchMesh *
RpPatchMeshSetFlags( RpPatchMesh *patchMesh,
                     RwUInt32 flags )
{
    RWAPIFUNCTION(RWSTRING("RpPatchMeshSetFlags"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);

    RpPatchMeshSetFlagsMacro(patchMesh, flags);

    RWRETURN(patchMesh);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshGetNumControlPoints
 * is used to determine the number of control points defining the specified
 * patch mesh.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for the final release version of an
 * application.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 *
 * \return Returns number of control points in the patch mesh.
 *
 * \see RpPatchMeshLock
 * \see RpPatchMeshUnlock
 * \see RpPatchMeshGetFlags
 * \see RpPatchMeshSetFlags
 * \see RpPatchMeshGetNumTexCoordSets
 * \see RpPatchMeshGetNumQuadPatches
 * \see RpPatchMeshGetNumTriPatches
 * \see RpPatchMeshGetPositions
 * \see RpPatchMeshGetNormals
 * \see RpPatchMeshGetPreLightColors
 * \see RpPatchMeshGetTexCoords
 */
RwUInt32
RpPatchMeshGetNumControlPoints( const RpPatchMesh *patchMesh )
{
    RwUInt32 numControlPoints;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshGetNumControlPoints"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);

    numControlPoints = RpPatchMeshGetNumControlPointsMacro(patchMesh);

    RWRETURN(numControlPoints);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshGetNumQuadPatches
 * is used to retrieve the number of quad patches that define the specified
 * patch mesh. The quad patches define the patch mesh's topology
 * and material properties.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for the final release version of an
 * application.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 *
 * \return Returns number of quad patches in the patch mesh.
 *
 * \see RpPatchMeshLock
 * \see RpPatchMeshUnlock
 * \see RpPatchMeshGetFlags
 * \see RpPatchMeshSetFlags
 * \see RpPatchMeshGetNumControlPoints
 * \see RpPatchMeshGetNumTexCoordSets
 * \see RpPatchMeshGetNumTriPatches
 * \see RpPatchMeshGetPositions
 * \see RpPatchMeshGetNormals
 * \see RpPatchMeshGetPreLightColors
 * \see RpPatchMeshGetTexCoords
 */
RwUInt32
RpPatchMeshGetNumQuadPatches( const RpPatchMesh *patchMesh )
{
    RwUInt32 numQuadPatches;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshGetNumQuadPatches"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);

    numQuadPatches = RpPatchMeshGetNumQuadPatchesMacro(patchMesh);

    RWRETURN(numQuadPatches);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshGetNumTriPatches
 * is used to retrieve the number of tri patches that define the specified
 * patch mesh. The tri patches define the patch mesh's topology
 * and material properties.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for the final release version of an
 * application.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 *
 * \return Returns number of texture coordinate sets.
 *
 * \see RpPatchMeshLock
 * \see RpPatchMeshUnlock
 * \see RpPatchMeshGetFlags
 * \see RpPatchMeshSetFlags
 * \see RpPatchMeshGetNumControlPoints
 * \see RpPatchMeshGetNumQuadPatches
 * \see RpPatchMeshGetNumTexCoordSets
 * \see RpPatchMeshGetPositions
 * \see RpPatchMeshGetNormals
 * \see RpPatchMeshGetPreLightColors
 * \see RpPatchMeshGetTexCoords
 */
RwUInt32
RpPatchMeshGetNumTriPatches( const RpPatchMesh *patchMesh )
{
    RwUInt32 numTriPatches;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshGetNumTriPatches"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);

    numTriPatches = RpPatchMeshGetNumTriPatchesMacro(patchMesh);

    RWRETURN(numTriPatches);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshGetNumTexCoordSets
 * is used to determine the number of sets of texture coordinates in the
 * specified patch mesh. Each set has one texture coordinate per control
 * point.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for the final release version of an
 * application.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 *
 * \return Returns number of texture coordinate sets.
 *
 * \see RpPatchMeshLock
 * \see RpPatchMeshUnlock
 * \see RpPatchMeshGetFlags
 * \see RpPatchMeshSetFlags
 * \see RpPatchMeshGetNumControlPoints
 * \see RpPatchMeshGetNumQuadPatches
 * \see RpPatchMeshGetNumTriPatches
 * \see RpPatchMeshGetPositions
 * \see RpPatchMeshGetNormals
 * \see RpPatchMeshGetPreLightColors
 * \see RpPatchMeshGetTexCoords
 */
RwUInt32
RpPatchMeshGetNumTexCoordSets( const RpPatchMesh *patchMesh )
{
    RwUInt32 numTexCoordSets;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshGetNumTexCoordSets"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);

    numTexCoordSets = RpPatchMeshGetNumTexCoordSetsMacro(patchMesh);

    RWRETURN(numTexCoordSets);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshGetPositions
 * is used to retrieve the array of control point positions from the
 * specified patch mesh. The array only exists if the patch mesh has been
 * created using the rpPATCHMESHPOSITIONS flag. Use this function to
 * initialize or redefine the patch mesh's control point position list.
 * There is a one-to-one correspondence between the control point position
 * array and the control point normal array.
 *
 * The patch mesh must be locked before the control point position data
 * can be modified.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for the final release version of an
 * application.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 *
 * \return Returns a pointer to the control point positions if successful, or
 * NULL otherwise.
 *
 * \see RpPatchMeshLock
 * \see RpPatchMeshUnlock
 * \see RpPatchMeshGetFlags
 * \see RpPatchMeshSetFlags
 * \see RpPatchMeshGetNumControlPoints
 * \see RpPatchMeshGetNumQuadPatches
 * \see RpPatchMeshGetNumTriPatches
 * \see RpPatchMeshGetNumTexCoordSets
 * \see RpPatchMeshGetNormals
 * \see RpPatchMeshGetPreLightColors
 * \see RpPatchMeshGetTexCoords
 */
RwV3d *
RpPatchMeshGetPositions( const RpPatchMesh *patchMesh )
{
    RwV3d *positions;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshGetPositions"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);

    positions = RpPatchMeshGetPositionsMacro(patchMesh);

    RWRETURN(positions);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshGetNormals
 * is used to retrieve the array of control point normals from the
 * specified patch mesh. The array only exists if the patch mesh has been
 * created using the rpPATCHMESHNORMALS flag. Use this function to initialize
 * or redefine the patch mesh's control point normal list. There is a
 * one-to-one correspondence between the control point position list array
 * and the control point normal list array.
 *
 * The patch mesh must be locked before the control point normal data can be
 * modified.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for the final release version of an
 * application.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 *
 * \return Returns a pointer to the control point normals if successful, or
 * NULL otherwise.
 *
 * \see RpPatchMeshLock
 * \see RpPatchMeshUnlock
 * \see RpPatchMeshGetFlags
 * \see RpPatchMeshSetFlags
 * \see RpPatchMeshGetNumControlPoints
 * \see RpPatchMeshGetNumQuadPatches
 * \see RpPatchMeshGetNumTriPatches
 * \see RpPatchMeshGetNumTexCoordSets
 * \see RpPatchMeshGetPositions
 * \see RpPatchMeshGetPreLightColors
 * \see RpPatchMeshGetTexCoords
 */
RwV3d *
RpPatchMeshGetNormals( const RpPatchMesh *patchMesh )
{
    RwV3d *normals;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshGetNormals"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);

    normals = RpPatchMeshGetNormalsMacro(patchMesh);

    RWRETURN(normals);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshGetPreLightColors
 * is used to retrieve the array of pre-lighting colors from the specified
 * patch mesh. The array only exists if the patch mesh has been created using
 * the rpPATCHMESHPRELIGHTS flag. The pre-lighting colors reside within the
 * patch mesh's control point topology, at one color per control point.
 * Use this function to initialize or redefine the pre-lighting colors.
 *
 * The patch mesh must be locked before the control point pre-lighting colors
 * data can be modified.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for the final release version of an
 * application.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 *
 * \return Returns a pointer to the control point pre-light colors if
 * successful, or NULL otherwise.
 *
 * \see RpPatchMeshLock
 * \see RpPatchMeshUnlock
 * \see RpPatchMeshGetFlags
 * \see RpPatchMeshSetFlags
 * \see RpPatchMeshGetNumControlPoints
 * \see RpPatchMeshGetNumQuadPatches
 * \see RpPatchMeshGetNumTriPatches
 * \see RpPatchMeshGetNumTexCoordSets
 * \see RpPatchMeshGetPositions
 * \see RpPatchMeshGetNormals
 * \see RpPatchMeshGetTexCoords
 */
RwRGBA *
RpPatchMeshGetPreLightColors( const RpPatchMesh *patchMesh )
{
    RwRGBA *preLightColors;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshGetPreLightColors"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);

    preLightColors = RpPatchMeshGetPreLightColorsMacro(patchMesh);

    RWRETURN(preLightColors);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshGetTexCoords
 * is used to retrieve a particular set of control point texture coordinates,
 * if they exist in the specified patch mesh. The number of sets available may
 * be determined using \ref RpPatchMeshGetNumTexCoordSets, and is fixed when the
 * patch mesh is created (see \ref RpPatchMeshCreate).
 *
 * Control point texture coordinates reside within the patch mesh's topology,
 * with each set having a (u,v) coordinate pair per control point. Use this
 * function to initialize or redefine the control point texture coordinates.
 *
 * There is a one-to-one correspondence between the control point texture
 * coordinates array and other control point list arrays.
 *
 * The patch mesh must be locked before modifying the control point texture
 * coordinates.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for the final release version of an
 * application.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 * \param index     The index of the desired texture coordinate set.
 *
 * \return Returns a pointer to an array of \ref RwTexCoords if successful, or
 * NULL if the specified texture coordinate set does not exist.
 *
 * \see RpPatchMeshLock
 * \see RpPatchMeshUnlock
 * \see RpPatchMeshGetFlags
 * \see RpPatchMeshSetFlags
 * \see RpPatchMeshGetNumControlPoints
 * \see RpPatchMeshGetNumQuadPatches
 * \see RpPatchMeshGetNumTriPatches
 * \see RpPatchMeshGetNumTexCoordSets
 * \see RpPatchMeshGetPositions
 * \see RpPatchMeshGetNormals
 * \see RpPatchMeshGetPreLightColors
 */
RwTexCoords *
RpPatchMeshGetTexCoords( const RpPatchMesh *patchMesh,
                         RwTextureCoordinateIndex index )
{
    RwTexCoords *texCoords;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshGetTexCoords"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);

    texCoords = RpPatchMeshGetTexCoordsMacro(patchMesh, index);

    RWRETURN(texCoords);
}

#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */
