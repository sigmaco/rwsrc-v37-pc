/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>
#include "rpdbgerr.h"

#include "rpmatfx.h"
#include "rpteam.h"

#include "teammatfx.h"
#include "team.h"

/*
 *

  CUSTOM FAST MATFX

 *
 */

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
static RpMaterial *
TeamMatFXEffectsEnabled( RpMaterial *material,
                         void *ptr )
{
    RpMatFXMaterialFlags flags;
    RwBool *effects;

    RWFUNCTION(RWSTRING("TeamMatFXEffectsEnabled"));
    RWASSERT(NULL != material);
    RWASSERT(NULL != ptr);

    effects = (RwBool *)ptr;

    flags = RpMatFXMaterialGetEffects(material);

    if(rpMATFXEFFECTNULL != flags)
    {
        /* The following effects have no support at the moment. */
        RWASSERT(flags != rpMATFXEFFECTBUMPMAP);
        RWASSERT(flags != rpMATFXEFFECTBUMPENVMAP);

        *effects = TRUE;
        RWRETURN((RpMaterial *)NULL);
    }

    RWRETURN(material);
}

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

/*---------------------- ATTACH PIPELINE -------------------------------*/
RpAtomic *
_rpTeamMatFXAttachPipeline(RpAtomic *atomic, RpTeamElementType type)
{
    RxPipeline *pipeline = (RxPipeline *)NULL;
    RpGeometry *geometry;
    RwUInt32 flags;

    RWFUNCTION(RWSTRING("_rpTeamMatFXAttachPipeline"));
    RWASSERT(NULL != atomic);

    geometry = RpAtomicGetGeometry(atomic);
    RWASSERT(NULL != geometry);

    flags = RpGeometryGetFlags(geometry);

    pipeline = _rpTeamMatFXGetPipeline(type, (flags & rpGEOMETRYTEXTURED2));
    RWASSERT(NULL != pipeline);

    /* Need to attach the custom pipeline. */
    atomic = RpAtomicSetPipeline(atomic, pipeline);
    RWASSERT(NULL != atomic);

    RWRETURN(atomic);
}
/*----------------------------------------------------------------------*/

/*---------------------- FIND EFFECTS ----------------------------------*/
RwBool
_rpTeamMatFXEffectsEnabled(RpAtomic *atomic)
{
    RwBool effects = FALSE;
    RpGeometry *geometry;

    RWFUNCTION(RWSTRING("_rpTeamMatFXEffectsEnabled"));
    RWASSERT(NULL != atomic);

    geometry = RpAtomicGetGeometry(atomic);
    RWASSERT(NULL != geometry);

    geometry = RpGeometryForAllMaterials( geometry,
                                          TeamMatFXEffectsEnabled,
                                          (void *)&effects );
    RWASSERT(NULL != geometry);

    RWRETURN(effects);
}
/*----------------------------------------------------------------------*/

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/
