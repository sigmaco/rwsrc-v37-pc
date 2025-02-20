/***********************************************************************
 *
 * Module:  im3dpipe.c
 *
 * Purpose: OpenGL immediate mode pipelines
 *
 ***********************************************************************/

/* =====================================================================
 *  Includes
 * ===================================================================== */
#include "bapipe.h"
#include "p2define.h"

#include "im3dpipe.h"
#include "nodeOpenGLSubmitNoLight.h"


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
 * \ingroup rwim3dopengl
 * \page RwIm3DGetTransformPipelineplatform RwIm3DGetTransformPipeline (platform-specific)
 *
 * The Im3D transform pipeline for OpenGL is shown below. Note that this
 * pipeline is designed specifically to leverage hardware transformation and
 * lighting support. If you wish to use an Im3D transform pipeline which does
 * transformation itself, then you will need to replace the default Im3D
 * transform pipeline with the generic Im3D transform pipeline.
 *
 * \verbatim
   The default Im3D transform pipeline:

     OpenGLImmInstance.csl
   \endverbatim
 *
 * \see RwIm3DEnd
 * \see RwIm3DRenderIndexedPrimitive
 * \see RwIm3DRenderLine
 * \see RwIm3DRenderPrimitive
 * \see RwIm3DRenderTriangle
 * \see RwIm3DTransform
 * \see RwIm3DGetRenderPipeline
 * \see \ref RwIm3DGetRenderPipelineplatform
 * \see RwIm3DGetTransformPipeline
 * \see RwIm3DSetRenderPipeline
 * \see RwIm3DSetTransformPipeline
 * \see RxNodeDefinitionGetOpenGLImmInstance
 * \see RxNodeDefinitionGetOpenGLSubmitNoLight
 */
RwBool
_rwIm3DCreatePlatformTransformPipeline( RxPipeline **globalPipe )
{
    RxPipeline  *pipe;


    RWFUNCTION( RWSTRING( "_rwIm3DCreatePlatformTransformPipeline" ) );

    RWASSERT( NULL != globalPipe );

    pipe = RxPipelineCreate();
    if ( NULL != pipe )
    {
        RxLockedPipe  *lockedPipe;


        pipe->pluginId = rwID_COREPLUGIN;

        lockedPipe = RxPipelineLock( pipe );
        if ( NULL != lockedPipe )
        {
            /* add the fragment */
            lockedPipe = RxLockedPipeAddFragment( lockedPipe,
                                                  NULL,
                                                  RxNodeDefinitionGetOpenGLImmInstance(),
                                                  NULL );
            RWASSERT( NULL != lockedPipe );

            pipe = RxLockedPipeUnlock( lockedPipe );

            RWASSERT( NULL != pipe );
            RWASSERT( pipe == lockedPipe );

            if ( NULL != pipe )
            {
                *globalPipe = pipe;
                RwIm3DSetTransformPipeline( pipe );

                RWRETURN( TRUE );
            }

        }

        /* pipeline couldn't be locked */
        RxPipelineDestroy( pipe );
    }

    RWRETURN( FALSE );
}


/**
 * \ingroup rwim3dopengl
 * \page RwIm3DGetRenderPipelineplatform RwIm3DGetRenderPipeline (platform-specific)
 *
 * The Im3D render pipelines for OpenGL are shown below. There is one for
 * rendering all primitive types. Note that these pipelines are designed
 * specifically to leverage hardware transformation and lighting support. If
 * you wish to use Im3D render pipelines which do transformation and lighting
 * (and anything else additionally) themselves, then you will need to replace
 * the relevant Im3D render pipeline one based on the generic Im3D render
 * pipelines.
 *
 * The OpenGL pipelines support triangle lists, strips and fans and line
 * lists and polylines.
 *
 * \verbatim
   The default Im3D render pipeline:

     OpenGLSubmitNoLight.csl
   \endverbatim
 *
 * \see RwIm3DEnd
 * \see RwIm3DRenderIndexedPrimitive
 * \see RwIm3DRenderLine
 * \see RwIm3DRenderPrimitive
 * \see RwIm3DRenderTriangle
 * \see RwIm3DTransform
 * \see RwIm3DGetRenderPipeline
 * \see RwIm3DGetTransformPipeline
 * \see \ref RwIm3DGetTransformPipelineplatform
 * \see RwIm3DSetRenderPipeline
 * \see RwIm3DSetTransformPipeline
 * \see RxNodeDefinitionGetOpenGLImmInstance
 * \see RxNodeDefinitionGetOpenGLSubmitNoLight
 *
 */
RwBool
_rwIm3DCreatePlatformRenderPipelines( rwIm3DRenderPipelines *globalPipes )
{
    RxPipeline  *pipe;


    RWFUNCTION( RWSTRING( "_rwIm3DCreatePlatformRenderPipelines" ) );

    RWASSERT( NULL != globalPipes );

    pipe = RxPipelineCreate();
    if ( NULL != pipe )
    {
        RxLockedPipe    *lockedPipe;


        pipe->pluginId = rwID_COREPLUGIN;

        lockedPipe = RxPipelineLock( pipe );
        if ( NULL != lockedPipe )
        {
            lockedPipe = RxLockedPipeAddFragment( lockedPipe,
                                                  NULL,
                                                  RxNodeDefinitionGetOpenGLSubmitNoLight(),
                                                  NULL );
            RWASSERT( NULL != lockedPipe );

            pipe = RxLockedPipeUnlock( lockedPipe );

            RWASSERT( NULL != pipe );
            RWASSERT( pipe == lockedPipe );

            if ( NULL != pipe )
            {
                globalPipes->triList    = pipe;
                globalPipes->triFan     = pipe;
                globalPipes->triStrip   = pipe;
                globalPipes->lineList   = pipe;
                globalPipes->polyLine   = pipe;

                /* set out pipe as the default for all primitives */
                RwIm3DSetRenderPipeline( pipe, rwPRIMTYPETRILIST );
                RwIm3DSetRenderPipeline( pipe, rwPRIMTYPETRIFAN );
                RwIm3DSetRenderPipeline( pipe, rwPRIMTYPETRISTRIP );
                RwIm3DSetRenderPipeline( pipe, rwPRIMTYPELINELIST );
                RwIm3DSetRenderPipeline( pipe, rwPRIMTYPEPOLYLINE );

                RWRETURN( TRUE );
            }
        }

        /* pipeline couldn't be locked */
        RxPipelineDestroy( pipe );
    }

    RWRETURN( FALSE );
}


/* ---------------------------------------------------------------------
 *  Function: _rwIm3DDestroyPlatformTransformPipeline
 *
 *  Purpose : Destroy the platform transform pipeline and reinstate the
 *            generic pipe.
 *
 *  On entry: globalPipe - Double pointer to an RxPipeline.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
void
_rwIm3DDestroyPlatformTransformPipeline( RxPipeline **globalPipe )
{
    RWFUNCTION( RWSTRING( "_rwIm3DDestroyPlatformTransformPipeline" ) );

    RWASSERT( NULL != globalPipe );

    /* reinstate the generic transform pipe */
    RwIm3DSetTransformPipeline( NULL );

    /* destroy our immediate mode pipeline */
    if ( NULL != *globalPipe )
    {
        RxPipelineDestroy( *globalPipe );
        *globalPipe = (RxPipeline *)NULL;
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rwIm3DDestroyPlatformRenderPipelines
 *
 *  Purpose : Destroy the platform render pipelines and reinstate the
 *            generic pipes.
 *
 *  On entry: globalPipes - Pointer to rwIm3DRenderPipelines.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
void
_rwIm3DDestroyPlatformRenderPipelines( rwIm3DRenderPipelines *globalPipes )
{
    RWFUNCTION( RWSTRING( "_rwIm3DDestroyPlatformRenderPipelines" ) );

    RWASSERT( NULL != globalPipes );

    /* reinstate the generic pipes for the primitive types */
    RwIm3DSetRenderPipeline( NULL, rwPRIMTYPETRILIST );
    RwIm3DSetRenderPipeline( NULL, rwPRIMTYPETRIFAN );
    RwIm3DSetRenderPipeline( NULL, rwPRIMTYPETRISTRIP );
    RwIm3DSetRenderPipeline( NULL, rwPRIMTYPELINELIST );
    RwIm3DSetRenderPipeline( NULL, rwPRIMTYPEPOLYLINE );

    if ( NULL != globalPipes->triList )
    {
        /* this pipeline is the same for all primitives
         * so only destroy it once */
        RxPipelineDestroy( globalPipes->triList );
        globalPipes->triList    = (RxPipeline *)NULL;
        globalPipes->triFan     = (RxPipeline *)NULL;
        globalPipes->triStrip   = (RxPipeline *)NULL;
        globalPipes->lineList   = (RxPipeline *)NULL;
        globalPipes->polyLine   = (RxPipeline *)NULL;
    }

    RWRETURNVOID();
}


/* =====================================================================
 *  Static function definitions
 * ===================================================================== */
