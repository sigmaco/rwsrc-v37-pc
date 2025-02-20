/***********************************************************************
 *
 * Module:  skinopengltoon.c
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

#include "rptoon.h"

#include "skinopengltoon.h"

#include "../toon/pipes.h"

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
 *  Function:
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */

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

    RxPipelineNode          *pipeNode;


    RWFUNCTION( RWSTRING( "_rpSkinPipelinesCreate" ) );

    for ( pipeline = rpSKINOPENGLPIPELINEGENERIC;
          pipeline < rpSKINOPENGLPIPELINEMAX;
          pipeline += (RpSkinOpenGLPipeline)(1) )
    {
        _rpSkinOpenGLPipeline( pipeline ) = (RxPipeline *)NULL;
    }

    _rpSkinOpenGLPipeline( rpSKINOPENGLPIPELINEGENERIC ) = _rpSkinOpenGLPipelineCreate( rpSKINTYPEGENERIC, NULL );
    RWASSERT( NULL != _rpSkinOpenGLPipeline( rpSKINOPENGLPIPELINEGENERIC ) );

    _rpSkinOpenGLPipeline( rpSKINTYPETOON ) = _rpSkinOpenGLPipelineCreate( rpSKINTYPETOON, _rpToonOpenGLAllInOneRenderCB );
    RWASSERT( NULL != _rpSkinOpenGLPipeline( rpSKINTYPETOON ) );

    /* disable the lighting callback for the skintoon pipeline
     * all-in-one pipelines only have one node */
    pipeNode = RxPipelineFindNodeByIndex( _rpSkinOpenGLPipeline( rpSKINTYPETOON ), 0 );
    RWASSERT( NULL != pipeNode );

    RxOpenGLAllInOneSetLightingCallBack( pipeNode, (RxOpenGLAllInOneLightingCallBack)NULL );

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

    case rpSKINTYPETOON:
        {
            pipelineIdx = rpSKINOPENGLPIPELINETOON;
            _rpToonAtomicChainSkinnedAtomicRenderCallback( atomic );
        }
        break;

    default:
        {
            pipelineIdx = rpSKINOPENGLPIPELINEGENERIC;
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
