/*
 * RpMatFX Null platform specific header.
 */

#ifndef MATFXPLATFORM_H
#define MATFXPLATFORM_H

/* RWPUBLIC */

/**
 * \defgroup rpmatfxnull Null
 * \ingroup rpmatfx
 *
 * Null specific documentation.
 */

#include "multiTexEffect.h"
#include "multiTex.h"

/**
 * \ingroup rpmatfxnull
 * \ref RpMatFXNullPipeline
 */
enum RpMatFXNullPipeline
{
    rpNAMATFXNULLPIPELINE    = 0,
    rpMATFXNULLPIPELINEMATFX = 1, /**<Null material effect rendering pipeline. */
    rpMATFXNULLPIPELINEMAX,
    rpMATFXNULLPIPELINEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RpMatFXNullPipeline RpMatFXNullPipeline;

/*===========================================================================*
 *--- Functions -------------------------------------------------------------*
 *===========================================================================*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern RxPipeline *
RpMatFXGetNullPipeline( RpMatFXNullPipeline nullPipeline );

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* RWPUBLICEND */

#endif /* MATFXPLATFORM_H */
