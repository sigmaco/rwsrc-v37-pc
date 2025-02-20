/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   patchmaterial.c                                            -*
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

#include "patchmaterial.h"
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

/*****************************************************************************
 PatchMeshAddMaterial

 Adds a material to the patch mesh's material list.

 Inputs :   mesh     - Pointer to patch mesh.
            material - Material to add.
 Outputs:   RwUInt32 - Material index, in the material list.
 */
static RwInt32
PatchMeshAddMaterial( PatchMesh *mesh,
                      RpMaterial *material)
{
    RwInt32 listIndex;

    RWFUNCTION(RWSTRING("PatchMeshAddMaterial"));
    RWASSERT(NULL != mesh);
    RWASSERT(NULL != material);

    /* Need to check that the material isn't already referenced. */
    listIndex = _rpMaterialListFindMaterialIndex(&(mesh->matList), material);

    /* Didn't find it. */
    if(-1 == listIndex)
    {
        /* Add the element to the list. */
        listIndex = _rpMaterialListAppendMaterial(&(mesh->matList), material);
        RWASSERT(-1 != listIndex);
    }

    /* Return the list index. */
    RWRETURN(listIndex);
}

/*****************************************************************************
 PatchMeshGetMaterial

 Gets a material from a patch mesh's material list.

 Inputs :   mesh     - Pointer to patch mesh.
            matIndex - Index in to the material list.
 Outputs:   RpMaterial * - Material from the material list.
 */
static RpMaterial *
PatchMeshGetMaterial( const PatchMesh *mesh,
                      RwUInt32 matIndex )
{
    RpMaterial *material;

    RWFUNCTION(RWSTRING("PatchMeshGetMaterial"));
    RWASSERT(NULL != mesh);
    RWASSERT(matIndex < (RwUInt32)(mesh->matList.numMaterials));

    material = (mesh->matList.materials)[matIndex];

    RWRETURN(material);
}

/*****************************************************************************
 PatchMeshRemoveMaterial

 Tries to remove a material from the patch mesh's material list, only if the
 material is no longer referenced.

 Inputs :   mesh     - Pointer to patch mesh.
            matIndex - Index of material to remove.
 Outputs:   void - None.
 */
static void
PatchMeshRemoveMaterial( PatchMesh *mesh,
                         RwUInt32 matIndex )
{
    RwUInt32 matRef;
    RwUInt32 iPatch;

    RWFUNCTION(RWSTRING("PatchMeshRemoveMaterial"));
    RWASSERT(NULL != mesh);
    RWASSERT(matIndex < (RwUInt32)mesh->matList.numMaterials);

    /* Count the number of patches using material. */
    for( matRef = 0, iPatch = 0;
         iPatch < mesh->userMesh.definition.numQuadPatches;
         iPatch++ )
    {
        if((RwUInt32)(mesh->quadPatches[iPatch].matIndex) == matIndex)
        {
            matRef++;
        }
    }

    for( iPatch = 0;
         iPatch < mesh->userMesh.definition.numTriPatches;
         iPatch++ )
    {
        if((RwUInt32)(mesh->triPatches[iPatch].matIndex) == matIndex)
        {
            matRef++;
        }
    }

    if(0 == matRef)
    {
        /* Nowt to do - as matLists can't remove materials. */
        /*_rpMaterialListRemoveMaterial(&(mesh->matList), matIndex);*/
    }

    RWRETURNVOID();
}

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
 * \ref RpPatchMeshSetQuadPatchMaterial
 * is used to associate the specified material with the given quad patch
 * in the given patch mesh. The patch mesh's material list is modified
 * accordingly. The material's reference count will be incremented if it's
 * the first patch in the patch mesh to reference the material.
 *
 * Before updating the patch mesh's materials, the patch mesh should be
 * locked with \ref rpPATCHMESHLOCKPATCHES. When the patch mesh is unlocked,
 * the patches will be collected into material groups. This decreases the
 * number of renderstate changes when rendering the patch mesh.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 * \param quadIndex Index into the patch mesh's array of quad patches.
 * \param material  Pointer to the material to set.
 *
 * \return Returns a pointer to the patch mesh if successful, or NULL
 * otherwise.
 *
 * \see RpPatchMeshSetTriPatchMaterial
 * \see RpPatchMeshGetQuadPatchMaterial
 * \see RpPatchMeshGetTriPatchMaterial
 * \see RpPatchMeshForAllMaterials
 * \see RpPatchMeshGetNumMaterials
 * \see RpPatchMeshGetNumQuadPatches
 * \see RpPatchMeshGetNumTriPatches
 * \see RpPatchMeshGetMaterial
 */
RpPatchMesh *
RpPatchMeshSetQuadPatchMaterial( RpPatchMesh *patchMesh,
                                 RwUInt32 quadIndex,
                                 RpMaterial *material )
{
    PatchMesh *mesh;
    QuadPatch *quad;
    RwUInt32 matIndex;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshSetQuadPatchMaterial"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);
    RWASSERT(quadIndex < patchMesh->definition.numQuadPatches);

    /* Grab the internal patch mesh. */
    mesh = _rpPatchMeshGetInternal(patchMesh);
    RWASSERT(NULL != mesh);
    RWASSERT(_rpPatchMeshCheckLocked(mesh, rpPATCHMESHLOCKPATCHES));

    /* Grab the quad patch for the index. */
    quad = &(mesh->quadPatches[quadIndex]);

    /* Store the old index. */
    matIndex = quad->matIndex;

    /* Copy the given quad material. */
    quad->matIndex = PatchMeshAddMaterial(mesh, material);

    /* Remove old material. */
    PatchMeshRemoveMaterial(mesh, matIndex);

    /* Return the external patch mesh. */
    RWRETURN(patchMesh);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshSetTriPatchMaterial
 * is used to associate the specified material with the given tri patch
 * in the given patch mesh. The patch mesh's material list is modified
 * accordingly. The material's reference count will be incremented if it's
 * the first patch in the patch mesh to reference the material.
 *
 * Before updating the patch mesh's materials, the patch mesh should be
 * locked with \ref rpPATCHMESHLOCKPATCHES. When the patch mesh is unlocked,
 * the patches will be collected into material groups. This decreases the
 * number of renderstate changes when rendering the patch mesh.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 * \param triIndex  Index of the tri patch in the patch mesh.
 * \param material  Pointer to the material to set.
 *
 * \return Returns a pointer to the patch mesh if successful, or NULL
 * otherwise.
 *
 * \see RpPatchMeshSetQuadPatchMaterial
 * \see RpPatchMeshGetQuadPatchMaterial
 * \see RpPatchMeshGetTriPatchMaterial
 * \see RpPatchMeshForAllMaterials
 * \see RpPatchMeshGetNumMaterials
 * \see RpPatchMeshGetNumQuadPatches
 * \see RpPatchMeshGetNumTriPatches
 * \see RpPatchMeshGetMaterial
 */
RpPatchMesh *
RpPatchMeshSetTriPatchMaterial( RpPatchMesh *patchMesh,
                                RwUInt32 triIndex,
                                RpMaterial *material )
{
    PatchMesh *mesh;
    TriPatch *tri;
    RwUInt32 matIndex;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshSetTriPatchMaterial"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);
    RWASSERT(triIndex < patchMesh->definition.numTriPatches);

    /* Grab the internal patch mesh. */
    mesh = _rpPatchMeshGetInternal(patchMesh);
    RWASSERT(NULL != mesh);
    RWASSERT(_rpPatchMeshCheckLocked(mesh, rpPATCHMESHLOCKPATCHES));

    /* Grab the tri patch for the index. */
    tri = &(mesh->triPatches[triIndex]);

    /* Store the old index. */
    matIndex = tri->matIndex;

    /* Copy the given tri patch into our index. */
    tri->matIndex = PatchMeshAddMaterial(mesh, material);

    /* Remove old material. */
    PatchMeshRemoveMaterial(mesh, matIndex);

    /* Return the external patch mesh. */
    RWRETURN(patchMesh);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshGetQuadPatchMaterial
 * is used to retrieve the material associated with the specified quad patch
 * in the given patch mesh.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 * \param quadIndex Index into the patch mesh's quad patches array.
 *
 * \return Returns a pointer to the quad patch's material if successful, or
 * NULL otherwise.
 *
 * \see RpPatchMeshSetQuadPatchMaterial
 * \see RpPatchMeshSetTriPatchMaterial
 * \see RpPatchMeshGetTriPatchMaterial
 * \see RpPatchMeshForAllMaterials
 * \see RpPatchMeshGetNumMaterials
 * \see RpPatchMeshGetNumQuadPatches
 * \see RpPatchMeshGetNumTriPatches
 * \see RpPatchMeshGetMaterial
 */
RpMaterial *
RpPatchMeshGetQuadPatchMaterial( const RpPatchMesh *patchMesh,
                                 RwUInt32 quadIndex )
{
    const PatchMesh *mesh;
    RpMaterial *material;
    RwUInt32 materialIndex;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshGetQuadPatchMaterial"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);
    RWASSERT(quadIndex < patchMesh->definition.numQuadPatches);

    /* Grab the internal patch mesh. */
    mesh = _rpPatchMeshGetConstInternal(patchMesh);
    RWASSERT(NULL != mesh);

    /* Get the material index. */
    materialIndex = mesh->quadPatches[quadIndex].matIndex;

    /* Get the material. */
    material = PatchMeshGetMaterial(mesh, materialIndex);

    /* Return the material. */
    RWRETURN(material);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshGetTriPatchMaterial
 * is used to retrieve the material associated with the specified tri patch
 * in the given patch mesh.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 * \param triIndex  Index into the tri patches array.
 *
 * \return Returns a pointer to the tri patch's material if successful, or NULL
 * otherwise.
 *
 * \see RpPatchMeshSetQuadPatchMaterial
 * \see RpPatchMeshSetTriPatchMaterial
 * \see RpPatchMeshGetQuadPatchMaterial
 * \see RpPatchMeshForAllMaterials
 * \see RpPatchMeshGetNumMaterials
 * \see RpPatchMeshGetNumQuadPatches
 * \see RpPatchMeshGetNumTriPatches
 * \see RpPatchMeshGetMaterial
 */
RpMaterial *
RpPatchMeshGetTriPatchMaterial( const RpPatchMesh *patchMesh,
                                RwUInt32 triIndex )
{
    const PatchMesh *mesh;
    RpMaterial *material;
    RwUInt32 materialIndex;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshGetTriPatchMaterial"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);
    RWASSERT(triIndex < patchMesh->definition.numTriPatches);

    /* Grab the internal patch mesh. */
    mesh = _rpPatchMeshGetConstInternal(patchMesh);
    RWASSERT(NULL != mesh);

    /* Get the material index. */
    materialIndex = mesh->triPatches[triIndex].matIndex;

    /* Get the material. */
    material = PatchMeshGetMaterial(mesh, materialIndex);

    /* Return the material. */
    RWRETURN(material);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshGetNumMaterials
 * is used to retrieve the number of different materials in use by all
 * the patches in the specified patch mesh.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 *
 * \return Returns the number of materials referenced by the patch mesh.
 *
 * \see RpPatchMeshSetQuadPatchMaterial
 * \see RpPatchMeshSetTriPatchMaterial
 * \see RpPatchMeshGetQuadPatchMaterial
 * \see RpPatchMeshGetTriPatchMaterial
 * \see RpPatchMeshForAllMaterials
 * \see RpPatchMeshGetNumQuadPatches
 * \see RpPatchMeshGetNumTriPatches
 * \see RpPatchMeshGetMaterial
 */
RwUInt32
RpPatchMeshGetNumMaterials( const RpPatchMesh *patchMesh )
{
    const PatchMesh *mesh;
    RwUInt32 numMaterials;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshGetNumMaterials"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);

    /* Grab the internal patch mesh. */
    mesh = _rpPatchMeshGetConstInternal(patchMesh);
    RWASSERT(NULL != mesh);

    numMaterials = rpMaterialListGetNumMaterials(&(mesh->matList));

    /* Return the number of materials. */
    RWRETURN(numMaterials);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshGetMaterial
 * is used to retrieve the material with the given index from the specified
 * patch mesh's material list.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh     Pointer to the patch mesh.
 * \param materialIndex Index of the material in the patch mesh.
 *
 * \return Returns a pointer to the material if successful, or NULL otherwise.
 *
 * \see RpPatchMeshSetQuadPatchMaterial
 * \see RpPatchMeshSetTriPatchMaterial
 * \see RpPatchMeshGetQuadPatchMaterial
 * \see RpPatchMeshGetTriPatchMaterial
 * \see RpPatchMeshForAllMaterials
 * \see RpPatchMeshGetNumMaterials
 * \see RpPatchMeshGetNumQuadPatches
 * \see RpPatchMeshGetNumTriPatches
 */
RpMaterial *
RpPatchMeshGetMaterial( const RpPatchMesh *patchMesh,
                        RwUInt32 materialIndex )
{
    const PatchMesh *mesh;
    RpMaterial *material;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshGetMaterial"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);

    /* Grab the internal patch mesh. */
    mesh = _rpPatchMeshGetConstInternal(patchMesh);
    RWASSERT(NULL != mesh);
    RWASSERT(materialIndex < RpPatchMeshGetNumMaterials(patchMesh));

    /* Get the material. */
    material = PatchMeshGetMaterial(mesh, materialIndex);

    /* Return the material. */
    RWRETURN(material);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshForAllMaterials
 * is used to apply the given callback function to all materials referenced
 * by patches in the specified patch mesh.
 *
 * The format of the callback function is:
 *
 * \verbatim
   RpMaterial * (* RpMaterialCallBack)(RpMaterial *material, void *userData)
   \endverbatim
 *
 * where data is a user-supplied data pointer to pass to the callback function.
 *
 * Note that if any invocation of the callback function returns a failure
 * status the iteration is terminated. However,
 * \ref RpPatchMeshForAllMaterials will still return successfully.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh containing the materials.
 * \param callBack  Pointer to the callback function to apply to each material.
 * \param userData  Pointer to user-supplied data to pass to the callback function.
 *
 * \return Returns a pointer to the patch mesh if successful or NULL if there
 * is an error.
 *
 * \see RpPatchMeshGetNumMaterials
 * \see RpPatchMeshGetMaterial
 */
const RpPatchMesh *
RpPatchMeshForAllMaterials( const RpPatchMesh *patchMesh,
                            RpMaterialCallBack callBack,
                            void *userData )
{
    RwUInt32 numMaterials;
    RwUInt32 iMaterial;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshForAllMaterials"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);
    RWASSERT(NULL != callBack);

    numMaterials = RpPatchMeshGetNumMaterials(patchMesh);

    for( iMaterial = 0; iMaterial < numMaterials; iMaterial++ )
    {
        RpMaterial *material;

        material = RpPatchMeshGetMaterial(patchMesh, iMaterial);

        if(NULL == callBack(material, userData))
        {
            /* Early out */
            RWRETURN(patchMesh);
        }
    }

    /* All ok */
    RWRETURN(patchMesh);
}
