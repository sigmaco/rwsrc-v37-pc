/*
 * RpMatFX D3D9 platform specific header.
 */

#ifndef MATFXPLATFORM_H
#define MATFXPLATFORM_H

/* RWPUBLIC */

/**
 * \defgroup rpmatfxd3d9 D3D9
 * \ingroup rpmatfx
 *
 * D3D9 specific documentation.
 */

/* RWPUBLICEND */

/******************************************************************************
 *  Include files
 */
#include "rwcore.h"
#include "rpworld.h"

/* RWPUBLIC */

/******************************************************************************
 *  Enum types
 */

/**
 * \ingroup rpmatfxd3d9
 * \ref RpMatFXD3D9Pipeline
 */
enum RpMatFXD3D9Pipeline
{
    rpNAMATFXD3D9PIPELINE          = 0,
    rpMATFXD3D9ATOMICPIPELINE      = 1, /**<D3D9 atomic material effect rendering pipeline. */
    rpMATFXD3D9WORLDSECTORPIPELINE = 2, /**<D3D9 world sector material effect rendering pipeline. */
    rpMATFXD3D9PIPELINEMAX,
    rpMATFXD3D9PIPELINEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RpMatFXD3D9Pipeline RpMatFXD3D9Pipeline;

/******************************************************************************
 *  Global types
 */

/******************************************************************************
 *  Functions
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern RxPipeline *
RpMatFXGetD3D9Pipeline( RpMatFXD3D9Pipeline d3d9Pipeline );

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* RWPUBLICEND */

#endif /* MATFXPLATFORM_H */
