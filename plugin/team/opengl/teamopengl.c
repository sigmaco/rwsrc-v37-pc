/***********************************************************************
 *
 * Module:  teamopengl.c
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

#include "team.h"

#include "teamopengl.h"

/**
 * \defgroup rpteamopengl OpenGL
 * \ingroup rpteam
 *
 * OpenGL platform specific documentation for RpTeam.
 */

/**
 * \ingroup rpteamopengl
 * \page WarningOpenGLShadowsUseStencilBuffer OpenGL Shadows Use the Stencil Buffer
 *
 * The OpenGL implementation of RpTeam makes use of the stencil buffer to create
 * shadows. For it to work properly, you must clear the stencil buffer to 0
 * when you clear the main camera's raster. You can do this with this code:
 *
 * \verbatim
 #if (defined (OpenGL_DRVMODEL_H))
     RwOpenGLSetStencilClear( 0 );

     RwCameraClear(Camera, &BackgroundColor,
                  rwCAMERACLEARZ | rwCAMERACLEARIMAGE | rwCAMERACLEARSTENCIL );
 #else
     RwCameraClear(Camera, &BackgroundColor,
                  rwCAMERACLEARZ | rwCAMERACLEARIMAGE );
 #endif
 \endverbatim
 */

/**
 * \ingroup rpteamopengl
 * \page openglcompatibility OpenGL Compatibility Matrix
 *
 * Listed below are the supported OpenGL options for RpTeam.
 *
 * \verbatim
                           SUPPORTED
   LIGHTING
   Ambient                    Yes
   Directional                Yes
   Other                       No

   SKINNING
   Maximum Weights              4

   MATERIAL EFFECTS
   Dual                      Soft/Hard
   Environmental             Soft/Hard
   Other

   GEOMETRY PROPERTIES
   Pre-light color            Yes
   Pre-light color alpha       No
   Material color             Yes
   Material color alpha       Yes

   FOG                        Yes
   \endverbatim
 *
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
static RxPipeline   *openglTeamPipelines[TEAMOPENGLPIPEID_MAX] =
{
    (RxPipeline *)NULL,     /* TEAMOPENGLPIPEID_NAPIPE            */
    (RxPipeline *)NULL,     /* TEAMOPENGLPIPEID_STATIC            */
    (RxPipeline *)NULL,     /* TEAMOPENGLPIPEID_SKINNED           */
    (RxPipeline *)NULL,     /* TEAMOPENGLPIPEID_STATIC_MATFX      */
    (RxPipeline *)NULL,     /* TEAMOPENGLPIPEID_SKINNED_MATFX     */
    (RxPipeline *)NULL,     /* TEAMOPENGLPIPEID_STATIC_SHADOW     */
    (RxPipeline *)NULL,     /* TEAMOPENGLPIPEID_SKINNED_SHADOW    */
};


/* =====================================================================
 *  Global variables
 * ===================================================================== */


/* =====================================================================
 *  Static function prototypes
 * ===================================================================== */


/* =====================================================================
 *  Global function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: _rpTeamOpenGLSetRenderPipeline
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
void
_rpTeamOpenGLSetRenderPipeline( RxPipeline *pipe,
                                _rpTeamOpenGLPipeId pipeId )
{
    RWFUNCTION( RWSTRING( "_rpTeamOpenGLSetRenderPipeline" ) );

    RWASSERT( pipeId > TEAMOPENGLPIPEID_NAPIPE );
    RWASSERT( pipeId < TEAMOPENGLPIPEID_MAX );

    openglTeamPipelines[pipeId] = pipe;

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rpTeamGetRenderPipelines
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RxPipeline **
_rpTeamGetRenderPipelines( void )
{
    RWFUNCTION( RWSTRING( "_rpTeamGetRenderPipelines" ) );

    RWASSERT( NULL != openglTeamPipelines );

    RWRETURN( openglTeamPipelines );
}


/* ---------------------------------------------------------------------
 *  Function: _rpTeamInstancePipelinesCreate
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RxPipeline **
_rpTeamInstancePipelinesCreate( void )
{
    RWFUNCTION( RWSTRING( "_rpTeamInstancePipelinesCreate" ) );

    RWRETURN( (RxPipeline **)NULL );
}


/* ---------------------------------------------------------------------
 *  Function: _rpTeamInstancePipelinesDestroy
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
void
_rpTeamInstancePipelinesDestroy( void )
{
    RWFUNCTION( RWSTRING( "_rpTeamInstancePipelinesDestroy" ) );

    RWRETURNVOID();
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

