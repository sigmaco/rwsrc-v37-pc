#ifndef TEAMSKIND3D8_H
#define TEAMSKIND3D8_H

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
typedef struct _rpTeamD3D8MatrixTransposed _rpTeamD3D8MatrixTransposed;
struct _rpTeamD3D8MatrixTransposed
{
    RwReal right_x, up_x, at_x, pos_x;
    RwReal right_y, up_y, at_y, pos_y;
    RwReal right_z, up_z, at_z, pos_z;
    RwReal right_w, up_w, at_w, pos_w;
};

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

extern RwUInt32  _rpTeamD3D8MaxBonesHardwareSupported;

extern _rpTeamD3D8MatrixTransposed _rpTeamD3D8projViewMatrix;

extern RwBool  _rpTeamD3D8UsingVertexShader;
extern RwBool  _rpTeamD3D8UsingLocalSpace;

extern RwUInt32 _rpTeamD3D8UnlitSkinVertexShader;
extern RwUInt32 _rpTeamD3D8UnlitNoFogSkinVertexShader;

/*===========================================================================*
 *--- Private Functions -----------------------------------------------------*
 *===========================================================================*/

extern void
_rwTeamD3D8SkinMatrixMultiplyTranspose(_rpTeamD3D8MatrixTransposed *dstMat,
               const RwMatrix *matA, const _rpTeamD3D8MatrixTransposed *matB);

extern void
_rpTeamD3D8SetVertexShaderMaterialColor(const RpMaterial *mat);

extern RwBool
_rxTeamD3D8SkinnedAtomicAllInOneNode(RxPipelineNodeInstance *self,
                                     const RxPipelineNodeParam *params);

extern void
_rpTeamD3D8UpdateLights(const RpTeam *team);

extern void
_rxTeamD3D8AtomicLightingCallback(void *object);

extern RwMatrix *
_rpTeamSkinMatBlendUpdating( RpSkin *skin,
                             RwFrame *frame,
                             RpHAnimHierarchy *hierarchy,
                             RwMatrix *matrixArray );

#endif /* TEAMSKIND3D8_H */

