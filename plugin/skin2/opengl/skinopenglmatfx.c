/***********************************************************************
 *
 * Module:  skinopenglmatfx.c
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

#include "rpmatfx.h"

#include "skinopenglmatfx.h"

#if defined( _WIN32 )
#if (!defined( NOASM ) && defined( RWDEBUG ))
#include "x86matbl.h"
#endif /* (!defined( NOASM ) && defined( RWDEBUG )) */
#endif /* defined( _WIN32 ) */


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

/* ---------------------------------------------------------------------
 *  Function: _rpSkinPipelinesCreate
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwBool
_rpSkinPipelinesCreate( RwUInt32 pipes __RWUNUSED__ )
{
    RpSkinOpenGLPipeline    pipeline;


    RWFUNCTION( RWSTRING( "_rpSkinPipelinesCreate" ) );

    for ( pipeline = rpSKINOPENGLPIPELINEGENERIC;
          pipeline < rpSKINOPENGLPIPELINEMAX;
          pipeline += (RpSkinOpenGLPipeline)(1) )
    {
        _rpSkinOpenGLPipeline( pipeline ) = (RxPipeline *)NULL;
    }

    _rpSkinOpenGLPipeline( rpSKINOPENGLPIPELINEGENERIC ) = _rpSkinOpenGLPipelineCreate( rpSKINTYPEGENERIC, NULL );
    RWASSERT( NULL != _rpSkinOpenGLPipeline( rpSKINOPENGLPIPELINEGENERIC ) );

    _rpSkinOpenGLPipeline( rpSKINTYPEMATFX ) = _rpSkinOpenGLPipelineCreate( rpSKINTYPEMATFX, _rpMatFXOpenGLAllInOneRenderCB );
    RWASSERT( NULL != _rpSkinOpenGLPipeline( rpSKINTYPEMATFX ) );

#if (!defined( NOASM ) && defined( RWDEBUG ))

    _rpSkinIntelx86ConfirmConstants();

#endif /* (!defined( NOASM ) && defined( RWDEBUG )) */

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rpSkinPipelinesAttach
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RpAtomic *
_rpSkinPipelinesAttach( RpAtomic *atomic,
                        RpSkinType type )
{
    RpSkinOpenGLPipeline    pipelineIdx;

    RxPipeline              *skinPipeline;


    RWFUNCTION( RWSTRING( "_rpSkinPipelinesAttach" ) );

    RWASSERT( NULL != atomic );

    switch ( type )
    {
    case rpSKINTYPEGENERIC:
        {
            pipelineIdx = rpSKINOPENGLPIPELINEGENERIC;
        }
        break;

    case rpSKINTYPEMATFX:
        {
            pipelineIdx = rpSKINOPENGLPIPELINEMATFX;
        }
        break;

    default:
        {
            if ( (RpAtomicGetPluginOffset( rwID_MATERIALEFFECTSPLUGIN ) > 0) &&
                 (FALSE != RpMatFXAtomicQueryEffects( atomic )) )
            {
                pipelineIdx = rpSKINOPENGLPIPELINEMATFX;
            }
            else
            {
                pipelineIdx = rpSKINOPENGLPIPELINEGENERIC;
            }
        }
    }

    skinPipeline = _rpSkinOpenGLPipeline( pipelineIdx );
    RWASSERT( NULL != skinPipeline );

    atomic = RpAtomicSetPipeline( atomic, skinPipeline );

    RWRETURN( atomic );
}


/* =====================================================================
 *  Static function definitions
 * ===================================================================== */
