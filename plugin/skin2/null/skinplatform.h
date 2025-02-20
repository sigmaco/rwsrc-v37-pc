/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   skinplatform.h                                             -*
 *-                                                                         -*
 *-  Purpose :   Null Patch pipeline extension                           -*
 *-                                                                         -*
 *===========================================================================*/

#ifndef SKINPLATFORM_H
#define SKINPLATFORM_H

/* RWPUBLIC */

/**
 * \defgroup rpskinnull Null
 * \ingroup rpskin
 *
 * Null skin pipeline extension.
 */

/**
 * \defgroup rpskinnullrestrictions Restrictions
 * \ingroup rpskinnull
 *
 * NULL skin pipeline restrictions.
 */

/**
 * \defgroup rpskinbonelimit Bone limit
 * \ingroup rpskinnullrestrictions
 *
 * \par Bone limit
 * TODO!
 * BLAH \ref rtskinsplit Toolkit
 *
 */

/* RWPUBLICEND */

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include "rwcore.h"
#include "rpworld.h"

/* RWPUBLIC */

/*===========================================================================*
 *--- Null Defines -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Null Global Types --------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rpskinnull
 * \ref RpSkinNullPipeline rendering pipelines available within the
 * \ref RpSkin plugin. Use \ref RpSkinGetNullPipeline to retrieve
 * the \ref RxPipeline s.
 */
enum RpSkinNullPipeline
{
    rpNASKINNULLPIPELINE              = 0,
    rpSKINNULLPIPELINEGENERIC,
    /**<Null skin dummy pipeline.                                     */
    rpSKINNULLPIPELINEMATFX,
    /**<Null material effect skin dummy pipeline.                     */
    rpSKINNULLPIPELINETOON,
    /**<Null toon skin dummy pipeline.                                */
    rpSKINNULLPIPELINEMATFXTOON,
    /**<Null matfx toon skin dummy pipeline.                          */
    rpSKINNULLPIPELINEMAX,
    rpSKINNULLPIPELINEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RpSkinNullPipeline RpSkinNullPipeline;

/*===========================================================================*
 *--- PS2 Plugin API Functions ----------------------------------------------*
 *===========================================================================*/
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
/*---------------------------------------------------------------------------*/

extern RxPipeline *
RpSkinGetNullPipeline( RpSkinNullPipeline nullPipeline );

/*---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif /* __cplusplus */

/* RWPUBLICEND */

#endif /* SKINPLATFORM_H */
