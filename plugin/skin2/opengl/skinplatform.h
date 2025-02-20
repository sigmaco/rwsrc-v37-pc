/***********************************************************************
 *
 * Module:  skinplatform.h
 *
 * Purpose: 
 *
 ***********************************************************************/

#if !defined( SKINPLATFORM_H )
#define SKINPLATFORM_H

/* RWPUBLIC */

/**
 * \defgroup rpskinopengl OpenGL
 * \ingroup rpskin
 *
 * OpenGL skin pipeline extension.
 */

/**
 * \defgroup rpskinopenglfeatures Features
 * \ingroup rpskinopengl
 *
 * OpenGL skin pipeline features.
 */

/**
 * \defgroup rpskinopenglrestrictions Restrictions
 * \ingroup rpskinopengl
 *
 * OpenGL skin pipeline restrictions.
 */

/**
 * \defgroup rpskinbonelimit Bone limit
 * \ingroup rpskinopenglrestrictions
 *
 * \par Bone limit
 * The bone limit is 256 as skinning is performed on the CPU.
 *
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
 * \ingroup rpskinopengl
 * \ref RpSkinOpenGLPipeline rendering pipelines available within
 * the \ref RpSkin plugin. Use \ref RpSkinGetOpenGLPipeline to
 * retrieve the \ref RxPipeline's.
 */
enum RpSkinOpenGLPipeline
{
    rpNASKINOPENGLPIPELINE                  = 0,
    rpSKINOPENGLPIPELINEGENERIC             = 1, /**<OpenGL skin rendering pipeline. */
    rpSKINOPENGLPIPELINEMATFX               = 2, /**<OpenGL material effect skin rendering pipeline.*/
    rpSKINOPENGLPIPELINETOON                = 3, /**<OpenGL toon skin rendering pipeline.*/
    rpSKINOPENGLPIPELINEMATFXTOON           = 4, /**<OpenGL toon matfx skin rendering pipeline not supported.*/
    rpSKINOPENGLPIPELINEMAX,
    rpSKINOPENGLPIPELINEFORCEENUMSIZEINT    = RWFORCEENUMSIZEINT
};
typedef enum RpSkinOpenGLPipeline RpSkinOpenGLPipeline;

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
RpSkinGetOpenGLPipeline( RpSkinOpenGLPipeline openglPipeline );

#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

/* RWPUBLICEND */

#endif /* !defined( SKINPLATFORM_H ) */