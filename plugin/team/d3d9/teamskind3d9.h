#ifndef TEAMSKIND3D9_H
#define TEAMSKIND3D9_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpteam.h"

/*===========================================================================*
 *--- Global Variables ------------------------------------------------------*
 *===========================================================================*/

extern RwRGBAReal   AmbientSaturated;

/*===========================================================================*
 *--- Private Global Types --------------------------------------------------*
 *===========================================================================*/
typedef struct _rpTeamD3D9MatrixTransposed _rpTeamD3D9MatrixTransposed;
struct _rpTeamD3D9MatrixTransposed
{
    RwReal right_x, up_x, at_x, pos_x;
    RwReal right_y, up_y, at_y, pos_y;
    RwReal right_z, up_z, at_z, pos_z;
    RwReal right_w, up_w, at_w, pos_w;
};

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

extern RwUInt32  _rpTeamD3D9MaxBonesHardwareSupported;

extern _rpTeamD3D9MatrixTransposed _rpTeamD3D9projViewMatrix;

extern RwBool  _rpTeamD3D9UsingVertexShader;
extern RwBool  _rpTeamD3D9UsingLocalSpace;

extern void *_rpTeamD3D9UnlitSkinVertexShader;
extern void *_rpTeamD3D9UnlitNoFogSkinVertexShader;

/*===========================================================================*
 *--- Private Functions -----------------------------------------------------*
 *===========================================================================*/

extern void
_rwTeamD3D9SkinMatrixMultiplyTranspose(_rpTeamD3D9MatrixTransposed *dstMat,
               const RwMatrix *matA, const _rpTeamD3D9MatrixTransposed *matB);

extern void
_rpTeamD3D9SetVertexShaderMaterialColor(const RpMaterial *mat);

extern RwBool
_rxTeamD3D9SkinnedAtomicAllInOneNode(RxPipelineNodeInstance *self,
                                     const RxPipelineNodeParam *params);

extern void
_rpTeamD3D9UpdateLights(const RpTeam *team);

extern void
_rxTeamD3D9AtomicLightingCallback(void *object);

extern RwMatrix *
_rpTeamSkinMatBlendUpdating( RpSkin *skin,
                             RwFrame *frame,
                             RpHAnimHierarchy *hierarchy,
                             RwMatrix *matrixArray );

#endif /* TEAMSKIND3D9_H */

