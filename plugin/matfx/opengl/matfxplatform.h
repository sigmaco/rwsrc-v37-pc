/***********************************************************************
 *
 * Module:  matfxplatform.h
 *
 * Purpose: 
 *
 ***********************************************************************/

#if !defined( MATFXPLATFORM_H )
#define MATFXPLATFORM_H

/* RWPUBLIC */

/**
 * \defgroup rpmatfxopengl OpenGL
 * \ingroup rpmatfx
 *
 * OpenGL specific documentation.
 */

/* RWPUBLICEND */

/* =====================================================================
 *  Includes
 * ===================================================================== */


/* =====================================================================
 *  Defines
 * ===================================================================== */


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */

/* RWPUBLIC */

/**
 * \ingroup rpmatfxopengl
 * \ref RpMatFXOpenGLPipeline
 */
enum RpMatFXOpenGLPipeline
{
    rpNAMATFXOPENGLPIPELINE          = 0,
    rpMATFXOPENGLATOMICPIPELINE      = 1, /**<OpenGL atomic material effect rendering pipeline. */
    rpMATFXOPENGLWORLDSECTORPIPELINE = 2, /**<OpenGL world sector material effect rendering pipeline. */
    rpMATFXOPENGLPIPELINEMAX,
    rpMATFXOPENGLPIPELINEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RpMatFXOpenGLPipeline RpMatFXOpenGLPipeline;

/* RWPUBLICEND */

/* =====================================================================
 *  Extern variables
 * ===================================================================== */


/* =====================================================================
 *  Extern function prototypes
 * ===================================================================== */

/* RWPUBLIC */

#if defined( __cplusplus )
extern "C"
{
#endif /* defined( __cplusplus ) */

extern RxPipeline *
RpMatFXGetOpenGLPipeline( RpMatFXOpenGLPipeline openglPipeline );

#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

/* RWPUBLICEND */

#endif /* !defined( MATFXPLATFORM_H ) */
