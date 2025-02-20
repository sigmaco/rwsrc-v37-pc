/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "skin.h"

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

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rpskind3d8features
 * \page skinassembler RpSkin Matrix-blending Code
 *
 * \note This section only applies if the RenderWare SDK has been
 * built with available assembler code. This is the default case.
 *
 * The matrix-blending code in the skin plugin has been changed in RWD3D8 to utilize x86
 * assembler in order to increase performance in the skinning calculations.
 *
 * \note This assembler code does take advantage of any vector instructions available
 * on the host processor.
 *
 */

/**
 * \ingroup rpskind3d8
 * \ref RpSkinGetD3D8Pipeline
 *
 * Returns one of the \ref rpskin's internal platform specific pipelines.
 *
 * The \ref rpskin plugin must be attached before using this function.
 *
 * \param d3d8Pipeline Type of the requested pipeline.
 *
 * \return The \ref RxPipeline requested from the plugin, or
 * NULL if the pipeline wasn't constructed.
 *
 * \see RpSkinAtomicSetType
 * \see RpSkinAtomicGetType
 */
RxPipeline *
RpSkinGetD3D8Pipeline( RpSkinD3D8Pipeline d3d8Pipeline )
{
    RxPipeline *pipeline;

    RWAPIFUNCTION(RWSTRING("RpSkinGetD3D8Pipeline"));
    RWASSERT(0 < _rpSkinGlobals.module.numInstances);
    RWASSERT(rpNASKIND3D8PIPELINE < d3d8Pipeline);
    RWASSERT(rpSKIND3D8PIPELINEMAX > d3d8Pipeline);

    pipeline = _rpSkinPipeline(d3d8Pipeline);

    RWRETURN(pipeline);
}
