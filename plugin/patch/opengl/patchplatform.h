/***********************************************************************
 *
 * Module:  patchplatform.h
 *
 * Purpose: 
 *
 ***********************************************************************/

#if !defined( PATCHPLATFORM_H )
#define PATCHPLATFORM_H

#ifndef RPPATCH_H

/* RWPUBLIC */

/**
 * \defgroup rppatchopengl OpenGL
 * \ingroup rppatch
 *
 * OpenGL patch pipeline extension.
 */

/* RWPUBLICEND */

#endif /* !RPPATCH_H */

/* =====================================================================
 *  Includes
 * ===================================================================== */


/* =====================================================================
 *  Defines
 * ===================================================================== */
/* We need to add to PatchAtomicData */

struct openglTag
{
    RpAtomicCallBackRender renderCallback;
};

#define EXTEND_PatchAtomicData struct openglTag openglLocals

/* We also need the constructor, etc */

#define EXTEND_rpPatchAtomicConstructor \
    atomicData->openglLocals.renderCallback = (RpAtomicCallBackRender)NULL

#define EXTEND_rpPatchAtomicCopy        \
    dstAtomicData->openglLocals.renderCallback                             \
                               = srcAtomicData->openglLocals.renderCallback


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */

#if !defined( RPPATCH_H )

/* RWPUBLIC */

/**
 * \ingroup rppatchopengl
 * \ref RpPatchOpenGLPipeline rendering pipelines available within the
 * \ref rppatch plugin. Use \ref RpPatchGetOpenGLPipeline to retrieve
 * the \ref RxPipeline s.
 */
enum RpPatchOpenGLPipeline
{
    rpNAPATCHOPENGLPIPELINE         = 0,

    rpPATCHOPENGLPIPELINEGENERIC    = 1, /**<OpenGL generic patch rendering pipeline. */
    rpPATCHOPENGLPIPELINEMATFX      = 2, /**<OpenGL material effect patch rendering pipeline. */
    rpPATCHOPENGLPIPELINESKINNED    = 3, /**<OpenGL skinned patch rendering pipeline. */
    rpPATCHOPENGLPIPELINESKINMATFX  = 4, /**<OpenGL skinned material effect patch rendering pipeline. */

    rpPATCHOPENGLPIPELINEMAX,

    rpPATCHOPENGLPIPELINEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RpPatchOpenGLPipeline RpPatchOpenGLPipeline;

/* RWPUBLICEND */

#endif /* !defined( RPPATCH_H ) */

/* =====================================================================
 *  Extern variables
 * ===================================================================== */


/* =====================================================================
 *  Extern function prototypes
 * ===================================================================== */

#if !defined( RPPATCH_H )

/* RWPUBLIC */

#if defined( __cplusplus )
extern "C"
{
#endif /* defined( __cplusplus ) */

extern RxPipeline *
RpPatchGetOpenGLPipeline( RpPatchOpenGLPipeline openglPipeline );

#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

/* RWPUBLICEND */

#endif /* !defined( RPPATCH_H ) */

#endif /* !defined( PATCHPLATFORM_H ) */
