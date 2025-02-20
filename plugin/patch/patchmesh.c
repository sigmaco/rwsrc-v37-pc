/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   patchmesh.c                                                -*
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

#include "patchgeometry.h"
#include "patchmesh.h"
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

/*****************************************************************************
 _rpPatchMeshCreate

 Creates an empty patch mesh.

 Inputs :   definition - RpPatchMeshDefinition structure containing:
                numQuadPatches   - Number of quad patches in mesh.
                numTriPatches    - Number of tri patches in mesh.
                numControlPoints - Number of control points in mesh.
                numTexCoordSets  - Number of tex coord sets in mesh.
                flags            - Definition flags.

 Outputs:   PatchMesh - Returns the internal part of the PatchMesh.
 */
PatchMesh *
_rpPatchMeshCreate( RpPatchMeshDefinition *definition )
{
    RwUInt32 size;

    PatchMesh *patchMesh;
    RpPatchMesh *exposedMesh;
    RpMaterialList *matList;

    void *ptr;

    RWFUNCTION(RWSTRING("_rpPatchMeshCreate"));
    RWASSERT( (0 < definition->numQuadPatches) ||
              (0 < definition->numTriPatches) );
    RWASSERT(0 < definition->numControlPoints);
    RWASSERT(rpPATCHMESHPOSITIONS & definition->flag);

    /*
     * Calculate the size of memory necessary for the texCoords.
     * We only support 0, 1, 2, ... rwMAXTEXTURECOORDS for now.
     * Although we really need a single set.
     */
    RWASSERT(rwMAXTEXTURECOORDS > definition->numTexCoordSets);

    /* Control point definition. */
    size = (((0!=(definition->flag & rpPATCHMESHPOSITIONS)) * sizeof(RwV3d))
           +((0!=(definition->flag & rpPATCHMESHNORMALS))   * sizeof(RwV3d))
           +((0!=(definition->flag & rpPATCHMESHPRELIGHTS)) * sizeof(RwRGBA))
           +(definition->numTexCoordSets * sizeof(RwTexCoords)));
    size *= definition->numControlPoints;

    /* Patch mesh. */
    size += sizeof(PatchMesh);

    /* Quad Patches. */
    size += sizeof(QuadPatch) * definition->numQuadPatches;

    /* Tri Patches. */
    size += sizeof(TriPatch) * definition->numTriPatches;

    /* Malloc the memory. */
    patchMesh = (PatchMesh *)RwMalloc(size,
        rwID_PATCHMESH | rwMEMHINTDUR_EVENT);
    if(NULL == patchMesh)
    {
        /* Failed to allocate memory for the new patch mesh. */
        RWERROR((E_RW_NOMEM, (size)));
        RWRETURN((PatchMesh *)NULL);
    }

    /* Clean the memory. */
    memset(patchMesh, 0, size);

    /* Get the exposed mesh. */
    exposedMesh = &(patchMesh->userMesh);
    RWASSERT((RwUInt32)exposedMesh == (RwUInt32)patchMesh);

    /* Pointer fixup. */

    /* Quad Patches. */
    patchMesh->quadPatches = (QuadPatch *)(patchMesh + 1);
    ptr = (void *)( patchMesh->quadPatches +
                    definition->numQuadPatches );

    /* Tri Patches. */
    patchMesh->triPatches = (TriPatch *)ptr;
    ptr = (void *)( patchMesh->triPatches +
                    definition->numTriPatches );

    /* Positions. */
    if(definition->flag & rpPATCHMESHPOSITIONS)
    {
        exposedMesh->positions = (RwV3d *)ptr;
        ptr = (void *)( exposedMesh->positions +
                        definition->numControlPoints );
    }

    /* Normals. */
    if(definition->flag & rpPATCHMESHNORMALS)
    {
        exposedMesh->normals = (RwV3d *)ptr;
        ptr = (void *)( exposedMesh->normals +
                        definition->numControlPoints );
    }

    /* Pre-light colors. */
    if(definition->flag & rpPATCHMESHPRELIGHTS)
    {
        exposedMesh->preLightColors = (RwRGBA *)ptr;
        ptr = (void *)( exposedMesh->preLightColors +
                        definition->numControlPoints );
    }

    /* Texture coordinates. */
    if(0 < definition->numTexCoordSets)
    {
        RwUInt32 iTexCoords;

        for( iTexCoords = 0;
             iTexCoords < definition->numTexCoordSets;
             iTexCoords++ )
        {
            exposedMesh->texCoords[iTexCoords] = (RwTexCoords *)ptr;
            ptr = (void *)( &(exposedMesh->texCoords
                            [iTexCoords][definition->numControlPoints]) );
        }
    }

    /* Store the RpPatchMesh definition. */
    exposedMesh->definition.flag = definition->flag;
    exposedMesh->definition.numQuadPatches = definition->numQuadPatches;
    exposedMesh->definition.numTriPatches = definition->numTriPatches;
    exposedMesh->definition.numControlPoints = definition->numControlPoints;
    exposedMesh->definition.numTexCoordSets = definition->numTexCoordSets;
    patchMesh->refCount = 1;

    /* Need to initalise the material list. */
    matList = _rpMaterialListInitialize(&(patchMesh->matList));
    if(NULL == matList)
    {
        /* Failed to allocate memory for the new patch mesh. */
        RWERROR((E_RW_NOMEM, (size)));

        /* Free the patch mesh. */
        RwFree(patchMesh);

        /* And return - in shame. */
        RWRETURN((PatchMesh *)NULL);
    }

    /* Lock the entire mesh. */
    patchMesh->lockMode = rpPATCHMESHLOCKALL;

    /* At this point we do not create the internal geometry. */
    RWRETURN(patchMesh);
}

/*****************************************************************************
 _rpPatchMeshDestroy

 Creates an empty patch mesh.

 Inputs:    patchMesh - PatchMesh to destroy.
 Outputs:   RwBool - TRUE on success.
 */
RwBool
_rpPatchMeshDestroy( PatchMesh *patchMesh )
{
    RwBool success;

    RWFUNCTION(RWSTRING("_rpPatchMeshDestroy"));
    RWASSERT(NULL != patchMesh);

    /* Assume success until failure. */
    success = TRUE;

    (patchMesh->refCount)--;

    /*
     * If the patches reference count is equal to one, and the patch mesh
     * references a geometry then THE reference is the geometry extension's
     * reference.
     */
    if((1 == patchMesh->refCount) && (NULL != patchMesh->geometry))
    {
        RpGeometry *geometry = patchMesh->geometry;

        patchMesh->geometry = (RpGeometry *)NULL;

        success = RpGeometryDestroy(geometry);
        RWASSERT(TRUE == success);
    }
    /* Only delete if refCount is zero. */
    else if(0 == patchMesh->refCount)
    {
        RpMaterialList *matList;

        matList = _rpMaterialListDeinitialize(&(patchMesh->matList));
        RWASSERT(NULL != matList);

        RwFree(patchMesh);
    }

    RWRETURN(success);
}

/*****************************************************************************
 _rpPatchMeshGetGeometry

 Returns the internal patch meshes geometry.

 Inputs : patchMesh - PatchMesh to query.
 Outputs: RpGeometry - Geometry on success.
 */
RpGeometry *
_rpPatchMeshGetGeometry( PatchMesh *patchMesh )
{
    RpGeometry *geometry;

    RWFUNCTION(RWSTRING("_rpPatchMeshGetGeometry"));

    /* Get the patch meshes geometry. */
    if(NULL == patchMesh->geometry)
    {
        /* Doh - need to build it. */
        patchMesh->geometry = _rpPatchGeometryCreateFromPatchMesh(patchMesh);
        RWASSERT(NULL != patchMesh->geometry);
    }

    geometry = patchMesh->geometry;

    RWRETURN(geometry);
}

/*****************************************************************************
 _rpPatchMeshAddRef

 Increments the patch meshes reference count.

 Inputs : patchMesh - PatchMesh to increment.
 Outputs: PatchMesh - PatchMesh on success.
 */
PatchMesh *
_rpPatchMeshAddRef( PatchMesh *patchMesh )
{
    RWFUNCTION(RWSTRING("_rpPatchMeshAddRef"));
    RWASSERT(NULL != patchMesh);

    (patchMesh->refCount)++;

    RWRETURN(patchMesh);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rppatch
 * \ref RpPatchMeshCreate is used to created a new empty RpPatchMesh. The
 * number of \ref RpQuadPatch patches and \ref RpTriPatch patches must be
 * defined. The number of control points must also be specified.
 *
 * The patch mesh is created locked with \ref RpPatchMeshLockMode
 * rpPATCHMESHLOCKALL and should be unlocked with \ref RpPatchMeshUnlock
 * once the control points and patches have been defined.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param numQuadPatches    Number of \ref RpQuadPatch quadrilateral patches
 *                          the patch mesh will contain.
 * \param numTriPatches     Number of \ref RpTriPatch triangular patches the
 *                          patch mesh will contain.
 * \param numControlPoints  Total number of unique control points in the
 *                          patch mesh.
 * \param flag              An \ref RwUInt32 bit-field specifying the patch
 *                          mesh's properties.
 * \ref RpPatchMeshFlag:
 * \li rpPATCHMESHPOSITIONS - Patch mesh has control point positions.
 * \li rpPATCHMESHNORMALS   - Patch mesh has control point normals.
 * \li rpPATCHMESHPRELIGHTS - Patch mesh has control point pre-light color values.
 * \li rpPATCHMESHTEXTURED  - Patch mesh should be textured.
 * \li rpPATCHMESHLIGHT     - Patch mesh will be lit.
 * \li rpPATCHMESHMODULATEMATERIALCOLOR - Control point color will be
 *                                        modulated with the material color.
 * \li rpPATCHMESHTEXCOORDSETS(num) - Number of texture sets.
 *
 * The control point positions are mandatory and must be
 * defined when creating the patch mesh. The control point pre-light color
 * values and texture coordinates are optional. The patch mesh can hold up to
 * eight sets of texture coordinates for multipass and multitexture rendering.
 * The number of texture coordinate sets supported is platform dependent.
 *
 * \return A pointer to the \ref RpPatchMesh if successful, or NULL otherwise.
 *
 * \see RpPatchMeshDestroy
 * \see RpPatchMeshLock
 * \see RpPatchMeshUnlock
 */
RpPatchMesh *
RpPatchMeshCreate( RwUInt32 numQuadPatches,
                   RwUInt32 numTriPatches,
                   RwUInt32 numControlPoints,
                   RwUInt32 flag )
{
    PatchMesh *mesh;
    RpPatchMesh *patchMesh;

    RpPatchMeshDefinition definition;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshCreate"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(0 < numControlPoints);
    RWASSERT((0 < numQuadPatches) || (0 < numTriPatches));
    RWASSERT(rpPATCHMESHPOSITIONS & flag);

    /* Setup the definition. */
    definition.flag = flag;
    definition.numControlPoints = numControlPoints;
    definition.numTriPatches = numTriPatches;
    definition.numQuadPatches = numQuadPatches;
    definition.numTexCoordSets = _rpPatchMeshFlagGetNumTexCoords(flag);

    /* Create the patch mesh. */
    mesh = _rpPatchMeshCreate( &definition );
    RWASSERT(NULL != mesh);
    RWASSERT((0 == numQuadPatches) || (NULL != mesh->quadPatches));
    RWASSERT((0 == numTriPatches) || (NULL != mesh->triPatches));

    RWASSERT(definition.flag == _rpPatchMeshGetFlags(mesh));
    RWASSERT(definition.numControlPoints ==
             _rpPatchMeshGetNumControlPoints(mesh));
    RWASSERT(definition.numTriPatches ==
             _rpPatchMeshGetNumTriPatches(mesh));
    RWASSERT(definition.numQuadPatches ==
             _rpPatchMeshGetNumQuadPatches(mesh));
    RWASSERT(definition.numTexCoordSets ==
             _rpPatchMeshGetNumTexCoordSets(mesh));

    /* Get the external patch mesh. */
    patchMesh = _rpPatchMeshGetExternal(mesh);
    RWASSERT(NULL != patchMesh);

    RWRETURN(patchMesh);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshDestroy is used to destroy an RpPatchMesh. This destroys
 * the patch mesh together with any privately allocated data structure.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh to destroy.
 *
 * \return TRUE if successful, NULL otherwise.
 *
 * \see RpPatchMeshCreate.
 */
RwBool
RpPatchMeshDestroy( RpPatchMesh *patchMesh )
{
    PatchMesh *mesh;
    RwBool success;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshDestroy"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);

    /* Get the internal mesh. */
    mesh = _rpPatchMeshGetInternal(patchMesh);
    RWASSERT(NULL != mesh);

    /* Destroy the whole patch mesh. */
    success = _rpPatchMeshDestroy(mesh);

    RWRETURN(success);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshSetQuadPatch is used to add a set of quadrilateral (quad)
 * patch control point indices to the patch mesh. The quad patch
 * \ref RpQuadPatch object consists of \ref rpQUADPATCHNUMCONTROLINDICES
 * control points.
 *
 * The quad patch indices are copied internally and not referenced.
 * The patch mesh must be locked with \ref RpPatchMeshLockMode
 * rpPATCHMESHLOCKPATCHES before the patch indices are added.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 * \param quadIndex Index of the quad patch to define.
 * \param quadPatch Pointer to the \ref RpQuadPatch indices.
 *
 * \return Pointer to the patch mesh if successful, NULL otherwise.
 *
 * \see RpPatchMeshCreate
 * \see RpPatchMeshSetTriPatch
 * \see RpPatchMeshGetQuadPatch
 * \see RpPatchMeshLock
 */
RpPatchMesh *
RpPatchMeshSetQuadPatch( RpPatchMesh *patchMesh,
                         RwUInt32 quadIndex,
                         RpQuadPatch *quadPatch )
{
    PatchMesh *mesh;
    QuadPatch *quad;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshSetQuadPatch"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);
    RWASSERT(NULL != quadPatch);
    RWASSERT(quadIndex < patchMesh->definition.numQuadPatches);

    /* Grab the internal patch mesh. */
    mesh = _rpPatchMeshGetInternal(patchMesh);
    RWASSERT(NULL != mesh);
    RWASSERT(_rpPatchMeshCheckLocked(mesh, rpPATCHMESHLOCKPATCHES));

    /* Grab the quad patch for the index. */
    quad = &(mesh->quadPatches[quadIndex]);

    /* Copy the given quad patch into our index. */
    _rpQuadPatchAssignQuadIndices(quadPatch, quad);

    /* Return the external patch mesh. */
    RWRETURN(patchMesh);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshSetTriPatch is used to add a set of triangle (tri)
 * patch control point indices to the patch mesh. The tri patch
 * \ref RpTriPatch object consists of \ref rpTRIPATCHNUMCONTROLINDICES
 * control points.
 *
 * The tri patch indices are copied internally and not referenced.
 * The patch mesh must be locked with \ref RpPatchMeshLockMode
 * rpPATCHMESHLOCKPATCHES before the patch indices are added.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 * \param triIndex  Index of the tri patch to define.
 * \param triPatch  Pointer to the \ref RpTriPatch indices.
 *
 * \return Pointer to the patch mesh if successful, NULL otherwise.
 *
 * \see RpPatchMeshCreate
 * \see RpPatchMeshSetQaudPatch
 * \see RpPatchMeshGetTriPatch
 * \see RpPatchMeshLock
 */
RpPatchMesh *
RpPatchMeshSetTriPatch( RpPatchMesh *patchMesh,
                        RwUInt32 triIndex,
                        RpTriPatch *triPatch )
{
    PatchMesh *mesh;
    TriPatch *tri;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshSetTriPatch"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);
    RWASSERT(NULL != triPatch);
    RWASSERT(triIndex < patchMesh->definition.numTriPatches);

    /* Grab the internal patch mesh. */
    mesh = _rpPatchMeshGetInternal(patchMesh);
    RWASSERT(NULL != mesh);
    RWASSERT(_rpPatchMeshCheckLocked(mesh, rpPATCHMESHLOCKPATCHES));

    /* Grab the tri patch for the index. */
    tri = &(mesh->triPatches[triIndex]);

    /* Copy the given tri patch into our index. */
    _rpTriPatchAssignTriIndices(triPatch, tri);

    /* Return the external patch mesh. */
    RWRETURN(patchMesh);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshGetQuadPatch is used to get a set of quadrilateral (quad)
 * patch control point indices from the patch mesh.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 * \param quadIndex Index of the quad patch to define.
 *
 * \return Pointer to the \ref RpQuadPatch if successful, NULL otherwise.
 *
 * \see RpPatchMeshSetQuadPatch
 */
const RpQuadPatch *
RpPatchMeshGetQuadPatch( const RpPatchMesh *patchMesh,
                         RwUInt32 quadIndex )
{
    const PatchMesh *mesh;
    const QuadPatch *quad;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshGetQuadPatch"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);
    RWASSERT(quadIndex < patchMesh->definition.numQuadPatches);

    /* Grab the internal patch mesh. */
    mesh = _rpPatchMeshGetConstInternal(patchMesh);
    RWASSERT(NULL != mesh);

    /* Grab the quad patch for the index. */
    quad = &(mesh->quadPatches[quadIndex]);

    /* Return the quad patch. */
    RWRETURN((const RpQuadPatch *)quad);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshGetTriPatch is used to get a set of triangle (tri)
 * patch control point indices from the patch mesh.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 * \param triIndex  Index of the tri patch to define.
 *
 * \return Pointer to the RpTriPatch if successful, NULL otherwise.
 *
 * \see RpPatchMeshSetTriPatch
 */
const RpTriPatch *
RpPatchMeshGetTriPatch( const RpPatchMesh *patchMesh,
                        RwUInt32 triIndex )
{
    const PatchMesh *mesh;
    const TriPatch *tri;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshGetTriPatch"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);
    RWASSERT(triIndex < patchMesh->definition.numTriPatches);

    /* Grab the internal patch mesh. */
    mesh = _rpPatchMeshGetConstInternal(patchMesh);
    RWASSERT(NULL != mesh);

    /* Grab the tri patch for the index. */
    tri = &(mesh->triPatches[triIndex]);

    /* Return the tri patch. */
    RWRETURN((const RpTriPatch *)tri);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshLock should be called before any of the patch mesh's
 * control points or patches are changed. This function also prepares the
 * patch mesh for unlocking after its data has been modified so that
 * the platform specific instance can be rebuilt.
 *
 * A newly created patch mesh is already locked.
 *
 * \param patchMesh Pointer to the patch mesh to lock.
 * \param lockMode  A \ref RwUInt32 bit-field specifying the patch mesh
 *                  locking flag.
 *
 * The lock flags are (type \ref RpPatchMeshLockMode):
 * \li rpPATCHMESHLOCKPATCHES    - Lock the mesh patches.
 * \li rpPATCHMESHLOCKPOSITIONS  - Lock the control point positions.
 * \li rpPATCHMESHLOCKNORMALS    - Lock the control point normals.
 * \li rpPATCHMESHLOCKPRELIGHTS  - Lock the control point pre-light color values.
 * \li rpPATCHMESHLOCKTEXCOORDS  - Lock the control point first set of
 *                                 texture coordinates.
 * \li rpPATCHMESHLOCKTEXCOORDS2 - Lock the control point second set of
 *                                 texture coordinates.
 * \li . . .
 * \li rpPATCHMESHLOCKTEXCOORDS8 - Lock the control point eighth set of
 *                                 texture coordinates.
 * \li rpPATCHMESHLOCKALL        - Combination of all the above.
 *
 * The library rppatch and the header file rppatch.h are required.
 *
 * \return Pointer to the \ref RpPatchMesh if successful or NULL otherwise.
 *
 * \see RpPatchMeshCreate
 * \see RpPatchMeshUnlock
 */
RpPatchMesh *
RpPatchMeshLock( RpPatchMesh *patchMesh,
                 RwUInt32 lockMode )
{
    PatchMesh *mesh;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshLock"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);
    RWASSERT(!(lockMode & ~(rpPATCHMESHLOCKALL)));

    /* Get the internal patch mesh. */
    mesh = _rpPatchMeshGetInternal(patchMesh);
    RWASSERT(NULL != mesh);

    /* Set the lock flag. */
    mesh->lockMode |= lockMode;

    /* Return the external patch mesh. */
    RWRETURN(patchMesh);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshUnlock
 * is used to unlock the specified patch mesh. This function will release
 * the internal sections of the patch mesh which were locked with
 * \ref RpPatchMeshLock. Unlocking a patch mesh may force the patch mesh
 * to be reinstanced. When the patch mesh is instanced the individual
 * patches are grouped by material into patch material meshes which
 * substantially speeds up the rendering process. The reinstancing,
 * particularly the re-grouping process, is potentially slow, especially
 * if the patch material meshes are rebuilt. Hence the locking and unlocking
 * of patch meshes should be used carefully between rendering frames.
 *
 * Newly created patch meshes are always locked and should be unlocked before
 * they are used in any rendering.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 *
 * \return Pointer to the patch mesh if successful, NULL otherwise.
 *
 * \see RpPatchMeshLock
 */
RpPatchMesh *
RpPatchMeshUnlock( RpPatchMesh *patchMesh )
{
    PatchMesh *mesh;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshUnlock"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);

    mesh = _rpPatchMeshGetInternal(patchMesh);
    RWASSERT(NULL != mesh);

    if(_rpPatchMeshCheckLocked(mesh, rpPATCHMESHLOCKALL))
    {
        mesh = _rpPatchGeometryTransferFromPatchMesh(mesh);
        RWASSERT(NULL != mesh);
    }

    mesh->lockMode = rpNAPATCHMESHLOCKMODE;

    patchMesh = _rpPatchMeshGetExternal(mesh);
    RWRETURN(patchMesh);
}
