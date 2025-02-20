/***********************************************************************
 *
 * Module:  patchplatform.c
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

#include "patch.h"


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
 * \ingroup rppatchopengl
 * \ref RpPatchGetOpenGLPipeline returns a pointer to the \ref RxPipeline that has
 * been created for the specific type of patch renderer. \e openglPipeline can
 * be one of \ref RpPatchOpenGLPipeline.
 *
 * The \ref rppatch plugin must be attached before using this function.
 *
 * \param openglPipeline \ref RpPatchOpenGLPipeline
 *
 * \return Pointer to the \ref RxPipeline associated with the specified patch renderer.
 *
 * \see RpPatchAtomicSetType
 * \see RpPatchAtomicGetType
 */
RxPipeline *
RpPatchGetOpenGLPipeline( RpPatchOpenGLPipeline openglPipeline )
{
    RxPipeline  *pipeline;


    RWAPIFUNCTION( RWSTRING( "RpPatchGetOpenGLPipeline" ) );

    RWASSERT( 0 < _rpPatchGlobals.module.numInstances );
    RWASSERT( rpNAPATCHOPENGLPIPELINE < openglPipeline );
    RWASSERT( rpPATCHOPENGLPIPELINEMAX > openglPipeline );

    pipeline = _rpPatchOpenGLPipeline( openglPipeline );

    RWRETURN( pipeline );
}


/* =====================================================================
 *  Static function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function:
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */

