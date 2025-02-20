/*
 * RpMatFX D3D8 platform specific header.
 */

#ifndef MATFXPLATFORM_H
#define MATFXPLATFORM_H

/* RWPUBLIC */

/**
 * \defgroup rpmatfxd3d8 D3D8
 * \ingroup rpmatfx
 *
 * D3D8 specific documentation.
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
 * \ingroup rpmatfxd3d8
 * \ref RpMatFXD3D8Pipeline
 */
enum RpMatFXD3D8Pipeline
{
    rpNAMATFXD3D8PIPELINE          = 0,
    rpMATFXD3D8ATOMICPIPELINE      = 1, /**<D3D8 atomic material effect rendering pipeline. */
    rpMATFXD3D8WORLDSECTORPIPELINE = 2, /**<D3D8 world sector material effect rendering pipeline. */
    rpMATFXD3D8PIPELINEMAX,
    rpMATFXD3D8PIPELINEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RpMatFXD3D8Pipeline RpMatFXD3D8Pipeline;

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
RpMatFXGetD3D8Pipeline( RpMatFXD3D8Pipeline d3d8Pipeline );

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* RWPUBLICEND */

#endif /* MATFXPLATFORM_H */
