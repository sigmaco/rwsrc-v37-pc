/***********************************************************************
 *
 * Module:  skinplatform.c
 *
 * Purpose:
 *
 ***********************************************************************/

/* =====================================================================
 *  Includes
 * ===================================================================== */
#include "rwcore.h"
#include "rpplugin.h"
#include "rpdbgerr.h"

#include "skin.h"
#include "skinopengl.h"

/**
 * \ingroup rpskinopengl
 * \page rpskinopengloverview Overview
 *
 * RenderWare Graphics for OpenGL provides skinning functionality through
 * \ref rpskin. Currently, skinning matrix blending is performed entirely
 * on the CPU making use of optimized assembly for efficiency where
 * available. There is no code path for performing matrix blending on video
 * hardware in this release.
 *
 */

/**
 * \ingroup rpskinopenglfeatures
 * \page skinassembler RpSkin Matrix-blending Code
 *
 * \note This section only applies to Windows OpenGL.
 *
 * \note This section only applies if the RenderWare SDK has been
 * built with available assembler code. This is the default case.
 *
 * The matrix-blending code in the skin plugin has been changed
 * in RenderWare Graphics for OpenGL to utilize x86 assembler in order to
 * improve performance in the skinning calculations.
 *
 * \note This assembler code does take advantage of any
 * vector instructions available on the host processor.
 */


/* =====================================================================
 *  Defines
 * ===================================================================== */


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */


/* =====================================================================
 *  Static variables
 * ===================================================================== */

/* =====================================================================
 *  Global variables
 * ===================================================================== */


/* =====================================================================
 *  Static function prototypes
 * ===================================================================== */


/* =====================================================================
 *  Global function definitions
 * ===================================================================== */

/**
 * \ingroup rpskinopengl
 * \ref RpSkinGetOpenGLPipeline
 *
 * Returns one of the \ref rpskin's OpenGL pipelines.
 *
 * The \ref rpskin plugin must be attached before using this function.
 *
 * \param openglPipeline Type of the requested pipeline.
 *
 * \return The \ref RxPipeline requested from the plugin, or
 * NULL if the pipeline wasn't constructed.
 *
 * \see RpSkinAtomicSetType
 * \see RpSkinAtomicGetType
 */
RxPipeline *
RpSkinGetOpenGLPipeline( RpSkinOpenGLPipeline openglPipeline )
{
    RxPipeline *pipeline;


    RWAPIFUNCTION( RWSTRING( "RpSkinGetOpenGLPipeline" ) );

    RWASSERT(0 < _rpSkinGlobals.module.numInstances);

    RWASSERT( rpNASKINOPENGLPIPELINE < openglPipeline);
    RWASSERT( rpSKINOPENGLPIPELINEMAX > openglPipeline);

    pipeline = _rpSkinOpenGLPipeline(openglPipeline);

    RWRETURN( pipeline );
}


/* =====================================================================
 *  Static function definitions
 * ===================================================================== */
