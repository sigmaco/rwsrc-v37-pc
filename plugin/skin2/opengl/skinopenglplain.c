/***********************************************************************
 *
 * Module:  skinopenglplain.c
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

#include "skinopenglplain.h"

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
                        RpSkinType type __RWUNUSED__ )
{
    RxPipeline  *pipeline;


    RWFUNCTION( RWSTRING( "_rpSkinPipelinesAttach" ) );

    pipeline = _rpSkinOpenGLPipeline( rpSKINOPENGLPIPELINEGENERIC );
    RWASSERT( NULL != pipeline );

    atomic = RpAtomicSetPipeline( atomic, pipeline );

    RWRETURN( atomic );
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

