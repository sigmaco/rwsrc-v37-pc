/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   patchskin.c                                                -*
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

#include "rpskin.h"
#include "rppatch.h"

#include "patchatomic.h"
#include "patchgeometry.h"
#include "patchskin.h"
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
 _rpPatchSkinGeometryGetSkin

 Returns the geometry's skin.

 Inputs : geometry - RpGeometry to get the skin from.
 Outputs : RpSkin - Skin attached to the geometry.
 */
RpSkin *
_rpPatchSkinGeometryGetSkin( RpGeometry *geometry )
{
    RpSkin *skin;

    RWFUNCTION(RWSTRING("_rpPatchSkinGeometryGetSkin"));
    RWASSERT(NULL != geometry);

    skin = RpSkinGeometryGetSkin(geometry);

    RWRETURN(skin);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rppatch
 * \ref RpPatchMeshGetSkin
 * is used to retrieve the \ref RpSkin attached to the patch mesh.
 *
 * The patch plugin must be attached before using this function.
 * The skin plugin must also be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh containing the skin.
 *
 * \return Returns a pointer to the \ref RpSkin if successful, or
 * NULL otherwise.
 *
 * \see RpPatchMeshSetSkin
 * \see RpPatchAtomicSetPatchMesh
 * \see RpPatchAtomicGetPatchMesh
 * \see RpSkinCreate
 */
RpSkin *
RpPatchMeshGetSkin( RpPatchMesh *patchMesh )
{
    PatchGeometryData *geometryData;
    PatchMesh *mesh;
    RpSkin *skin;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshGetSkin"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);

    mesh = _rpPatchMeshGetInternal(patchMesh);
    RWASSERT(NULL != mesh);

    geometryData = PATCHGEOMETRYGETDATA(mesh->geometry);
    RWASSERT(NULL != geometryData);

    skin = geometryData->skin;

    RWRETURN(skin);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshSetSkin
 * is used to attach the \ref RpSkin to the specified patch mesh. The skin
 * data is used during rendering to skin the patch mesh.
 *
 * The patch plugin and the skin plugin must be attached before using
 * this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 * \param skin      Pointer to the skin to attach.
 *
 * \return Returns a pointer to the patch mesh if successful, or NULL
 * otherwise.
 *
 * \see RpPatchMeshGetSkin
 * \see RpPatchAtomicSetPatchMesh
 * \see RpPatchAtomicGetPatchMesh
 * \see RpSkinCreate
 */
RpPatchMesh *
RpPatchMeshSetSkin( RpPatchMesh *patchMesh,
                    RpSkin *skin )
{
    PatchMesh *mesh;
    PatchGeometryData *geometryData;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshSetSkin"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);

    mesh = _rpPatchMeshGetInternal(patchMesh);
    RWASSERT(NULL != mesh);

    geometryData = PATCHGEOMETRYGETDATA(mesh->geometry);
    RWASSERT(NULL != geometryData);

    geometryData->skin = skin;

    mesh->geometry = RpSkinGeometrySetSkin(mesh->geometry, skin);
    RWASSERT(NULL != mesh->geometry);

    RWRETURN(patchMesh);
}
