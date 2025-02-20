#ifndef EFFECTPIPES_H
#define EFFECTPIPES_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include "rwcore.h"
#include "rpworld.h"

#include "matfx.h"

#define rpMATFXALIGNMENT  rwFRAMEALIGNMENT

#define RPMATFXALIGNMENT(_x) \
   (! (((rpMATFXALIGNMENT)-1) & ((RwUInt32)(_x))))

typedef struct rpMatFXMaterialData RWALIGN(rpMatFXMaterialData, rpMATFXALIGNMENT);

#if   defined (SKY2_DRVMODEL_H)
#include "sky2/effectPipesSky.h"
#elif defined (OPENGL_DRVMODEL_H)
#include "opengl/effectPipesOpenGL.h"
#elif defined (XBOX_DRVMODEL_H)
#include "xbox/effectPipesXbox.h"
#elif defined (GCN_DRVMODEL_H)
#include "gcn/effectPipesGcn.h"
#elif defined (NULLGCN_DRVMODEL_H)
#include "gcn/effectPipesGcn.h"
#elif defined (NULL_DRVMODEL_H)
#include "null/effectPipesNull.h"
#elif defined (NULLSKY_DRVMODEL_H)
#include "null/effectPipesNull.h"
#elif defined (NULLXBOX_DRVMODEL_H)
#include "null/effectPipesNull.h"
#elif defined (SOFTRAS_DRVMODEL_H)
#include "softras/effectPipesSoftras.h"
#elif defined (D3D8_DRVMODEL_H)
#include "d3d8/effectPipesD3D8.h"
#elif defined (D3D9_DRVMODEL_H)
#include "d3d9/effectPipesD3D9.h"
#else
#error "unrecognised driver"
#endif



/*===========================================================================*
 *--- Types -----------------------------------------------------------------*
 *===========================================================================*/
enum MatFXPass
{
    rpSECONDPASS = 0,
    rpTHIRDPASS  = 1,
    rpMAXPASS    = 2
};
typedef enum MatFXPass MatFXPass;

typedef struct MatFXBumpMapData RWALIGN(MatFXBumpMapData, rpMATFXALIGNMENT);

typedef struct MatFXEnvMapData RWALIGN(MatFXEnvMapData, rpMATFXALIGNMENT);
struct MatFXEnvMapData
{
    RwFrame   *frame;
    RwTexture *texture;
    RwReal    coef;
    RwBool    useFrameBufferAlpha;
};

typedef struct MatFXUVAnimData RWALIGN(MatFXUVAnimData, rpMATFXALIGNMENT);
struct MatFXUVAnimData
{
    RwMatrix *baseTransform;
    RwMatrix *dualTransform;
};

/*
 * MatFXDualData is device specific as it contains extra data
 * on the PS2 for the blend modes.
 */
typedef struct MatFXDualData RWALIGN(MatFXDualData, rpMATFXALIGNMENT);

typedef union MatFXEffectUnion RWALIGN(MatFXEffectUnion, rpMATFXALIGNMENT);
union MatFXEffectUnion
{
    MatFXBumpMapData  bumpMap;
    MatFXEnvMapData   envMap;
    MatFXDualData     dual;
    MatFXUVAnimData   uvAnim;
    
#if (defined (MULTITEXD3D8_H))
    MatFXD3D8Material d3d8Mat;
#endif
};

typedef struct MatFXEffectData RWALIGN(MatFXEffectData, rpMATFXALIGNMENT);
struct MatFXEffectData
{
    MatFXEffectUnion     data;
    RpMatFXMaterialFlags flag;
    
#if (defined (SKY2_DRVMODEL_H))
    MatFXSkyMaterial     skyMat;
#endif
};

struct rpMatFXMaterialData
{
    MatFXEffectData      data[rpMAXPASS];
    RpMatFXMaterialFlags flags;
};

/*===========================================================================*
 *--- Global Variables ------------------------------------------------------*
 *===========================================================================*/
extern RwInt32 MatFXMaterialDataOffset;

/*===========================================================================*
 *--- Defines ---------------------------------------------------------------*
 *===========================================================================*/
#define MATFXMATERIALGETDATA(material)          \
    ((rpMatFXMaterialData **)                   \
     (((RwUInt8 *)material)+                    \
      MatFXMaterialDataOffset))

#define MATFXMATERIALGETCONSTDATA(material)     \
    ((const rpMatFXMaterialData * const *)      \
     (((const RwUInt8 *)material)+              \
      MatFXMaterialDataOffset))

/*===========================================================================*
 *--- Functions -------------------------------------------------------------*
 *===========================================================================*/

/*--- Pipeline functions ----------------------------------------------------*/
extern RwBool
_rpMatFXPipelinesCreate(void);

extern RwBool
_rpMatFXPipelinesDestroy(void);

extern RpAtomic *
_rpMatFXPipelineAtomicSetup(RpAtomic *atomic);

extern RpWorldSector *
_rpMatFXPipelineWorldSectorSetup(RpWorldSector *worldSector);

/*--- Device data fucntions -------------------------------------------------*/
extern RwBool
_rpMatFXSetupDualRenderState(MatFXDualData *dualData, RwRenderState nState);

extern RwTexture *
_rpMatFXSetupBumpMapTexture(const RwTexture *baseTexture,
                            const RwTexture *effectTexture);

/*--- Texture fucntions -----------------------------------------------------*/
extern RwTexture *
_rpMatFXTextureMaskCreate( const RwTexture *baseTexture,
                           const RwTexture *effectTexture );

#endif /* EFFECTPIPES_H */
