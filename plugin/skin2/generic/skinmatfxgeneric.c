/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpskin.h"
#include "rpmatfx.h"

#include "skin.h"
#include "skincommon.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/
SkinGlobals _rpSkinGlobals =
{
    0,
    0,
    0,
    { (RwMatrix *)NULL, NULL },
    0,
    (RwFreeList *)NULL,
    { 0, 0 },
    {
        {                       /* SkinGlobalPlatform  platform  */
            (RxPipeline *)NULL, /*  rpSKINGENERICPIPELINEGENERIC */
            (RxPipeline *)NULL  /*  rpSKINGENERICPIPELINEMATFX   */
        }
    },
    (SkinSplitData *) NULL
};

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

/****************************************************************************
 SkinMeshSearchForEffect

 Works through the skin meshes materials searching for the first material
 effect.

 Inputs : atomic - RpAtomic * we should search.
 Outputs: RpMatFXMaterialFlags - First effect found, or rpMATFXEFFECTNULL
                                 if none.
 */
static RpMatFXMaterialFlags
SkinAtomicSearchForEffect(RpAtomic *atomic)
{
    RpGeometry *geometry;
    RwUInt32 iMaterial;
    RwUInt32 numMaterials;

    RWFUNCTION(RWSTRING("SkinAtomicSearchForEffect"));
    RWASSERT(NULL != atomic);
    geometry = RpAtomicGetGeometry(atomic);
    RWASSERT(NULL != geometry);
    numMaterials = (RwUInt32)RpGeometryGetNumMaterials(geometry);

    /* Search the materials for effects. */
    for( iMaterial = 0; iMaterial < numMaterials; ++iMaterial )
    {
        RpMatFXMaterialFlags effect;
        RpMaterial *material;

        material = RpGeometryGetMaterial(geometry, iMaterial);
        RWASSERT(NULL != material);

        effect = RpMatFXMaterialGetEffects(material);

        if(effect != rpMATFXEFFECTNULL)
        {
            RWRETURN(effect);
        }
    }

    RWRETURN(rpMATFXEFFECTNULL);
}

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

/****************************************************************************
 _rpSkinPipelinesCreate

 Create the skinning pipelines.
 We create the generic pipelines by cloning the generic rendering pipelines
 and attaching the pluginId and pluginData fields to the clone.

 Inputs :   None
 Outputs:   RwBool - on success.
 */
RwBool
_rpSkinPipelinesCreate(RwUInt32 pipes __RWUNUSED__)
{
    RxPipeline *pipe;
    RxPipeline *clone;

    RWFUNCTION(RWSTRING("_rpSkinPipelinesCreate"));

    if(pipes & rpSKINPIPELINESKINGENERIC)
    {
        /* We create the GENERIC pipe by cloning the default pipe. */
        pipe = RpAtomicGetDefaultPipeline();
        RWASSERT(NULL != pipe);

        clone = RxPipelineClone(pipe);
        RWASSERT(NULL != clone);

        clone->pluginId = rwID_SKINPLUGIN;
        clone->pluginData = rpSKINTYPEGENERIC;

        _rpSkinPipeline(rpSKINGENERICPIPELINEGENERIC) = clone;
        RWASSERT(NULL != _rpSkinPipeline(rpSKINGENERICPIPELINEGENERIC));
    }

    if(pipes & rpSKINPIPELINESKINMATFX)
    {
        /* We also create the MATFX pipe by cloning the default matfx pipe. */
        pipe = RpAtomicGetDefaultPipeline();
        RWASSERT(NULL != pipe);

        clone = RxPipelineClone(pipe);
        RWASSERT(NULL != clone);

        clone->pluginId = rwID_SKINPLUGIN;
        clone->pluginData = rpSKINTYPEMATFX;

        _rpSkinPipeline(rpSKINGENERICPIPELINEMATFX) = clone;
        RWASSERT(NULL != _rpSkinPipeline(rpSKINGENERICPIPELINEMATFX));
    }

    RWRETURN(TRUE);
}


/****************************************************************************
 _rpSkinPipelinesDestroy
 Destroy the skinning pipelines.
 Inputs :   None
 Outputs:   RwBool - on success.
 */
RwBool
_rpSkinPipelinesDestroy(void)
{
    RWFUNCTION(RWSTRING("_rpSkinPipelinesDestroy"));

    if(NULL != _rpSkinPipeline(rpSKINGENERICPIPELINEGENERIC))
    {
        RxPipelineDestroy(_rpSkinPipeline(rpSKINGENERICPIPELINEGENERIC));
        _rpSkinPipeline(rpSKINGENERICPIPELINEGENERIC) = (RxPipeline *)NULL;
    }

    if(NULL != _rpSkinPipeline(rpSKINGENERICPIPELINEMATFX))
    {
        RxPipelineDestroy(_rpSkinPipeline(rpSKINGENERICPIPELINEMATFX));
        _rpSkinPipeline(rpSKINGENERICPIPELINEMATFX) = (RxPipeline *)NULL;
    }

    RWRETURN(TRUE);
}


/****************************************************************************
 _rpSkinPipelinesAttach

 Attach the generic skinning pipeline to an atomic.

 Inputs :   atomic *   - Pointer to the atomic.
            type       - RpSkinType requested.
 Outputs:   RpAtomic * - Pointer to the atomic on success.
 */
RpAtomic *
_rpSkinPipelinesAttach( RpAtomic *atomic,
                        RpSkinType type )
{
    RwBool matfx;

    RpSkinGenericPipeline iPipeline;
    RxPipeline *pipeline;

    RWFUNCTION(RWSTRING("_rpSkinPipelinesAttach"));
    RWASSERT(NULL != atomic);

    /* Is the patch mesh matfxed. */
    matfx = (SkinAtomicSearchForEffect(atomic) != rpMATFXEFFECTNULL);

    /* Do we have a hint? */
    if(type == rpNASKINTYPE)
    {
        iPipeline = (matfx == TRUE) ? rpSKINGENERICPIPELINEMATFX
                                    : rpSKINGENERICPIPELINEGENERIC;
    }
    else
    {
        /* Ok we've been given a hint. */
        switch(type)
        {
            case rpSKINTYPEGENERIC:
                iPipeline = rpSKINGENERICPIPELINEGENERIC;
                break;
            case rpSKINTYPEMATFX:
                iPipeline = rpSKINGENERICPIPELINEMATFX;
                break;
            default:
                iPipeline = rpNASKINGENERICPIPELINE;
                break;
        }
    }

    RWASSERT(rpNASKINGENERICPIPELINE < iPipeline);
    RWASSERT(rpSKINGENERICPIPELINEMAX > iPipeline);

    pipeline = RpSkinGetGenericPipeline(iPipeline);
    RWASSERT(NULL != pipeline);

    atomic = RpAtomicSetPipeline(atomic, pipeline);
    RWASSERT(NULL != atomic);

    /*
     * Obviously these don't chain, as we should probably store each atomic's
     * atomic render callback.
     */
    if(defRenderCallback == NULL)
    {
        defRenderCallback = RpAtomicGetRenderCallBack(atomic);
    }

    RpAtomicSetRenderCallBack(atomic, SkinMatBlendAtomicRender);

    RWRETURN(atomic);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

