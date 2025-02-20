/***********************************************************************
 *
 * Module:  patchopenglgeneric.c
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
#include "patchatomic.h"

#include "patchopengl.h"


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
 *  Function: _rpPatchPipelinesCreate
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwBool
_rpPatchPipelinesCreate( RwUInt32 pipes )
{
    RxPipeline  *pipe;


    RWFUNCTION( RWSTRING( "_rpPatchPipelinesCreate" ) );

    if ( 0 != (pipes & rpPATCHPIPELINEGENERIC) )
    {
        pipe = _rpPatchOpenGLCreateGenericPipe();
        RWASSERT( NULL != pipe );
        if ( NULL == pipe )
        {
            RWRETURN( FALSE );
        }
         _rpPatchOpenGLPipeline(rpPATCHOPENGLPIPELINEGENERIC) = pipe;
        RWASSERT( NULL != _rpPatchOpenGLPipeline(rpPATCHOPENGLPIPELINEGENERIC) );
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rpPatchPipelinesAttach
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RpAtomic *
_rpPatchPipelinesAttach( RpAtomic *atomic,
                         RpPatchType type __RWUNUSED__ )
{
    RpPatchMesh             *patchMesh;

    RpPatchOpenGLPipeline   openglPatchPipe;

    PatchAtomicData         *atomicData;

    RpAtomicCallBackRender  curAtomicRenderCB;

    RxPipeline              *pipeline;


    RWFUNCTION( RWSTRING( "_rpPatchPipelinesAttach" ) );

    RWASSERT( NULL != atomic );

    /* get the patch mesh */
    patchMesh = RpPatchAtomicGetPatchMesh( atomic );
    RWASSERT( NULL != patchMesh );

    openglPatchPipe = rpPATCHOPENGLPIPELINEGENERIC;

    pipeline = RpPatchGetOpenGLPipeline( openglPatchPipe );
    RWASSERT( NULL != pipeline );

    atomic = RpAtomicSetPipeline( atomic, pipeline );
    RWASSERT( NULL != atomic );

    atomicData = PATCHATOMICGETDATA(atomic);

    curAtomicRenderCB = RpAtomicGetRenderCallBack( atomic );

    if ( curAtomicRenderCB != _rpPatchOpenGLAtomicRenderCB )
    {
        atomicData->openglLocals.renderCallback = curAtomicRenderCB;
        RpAtomicSetRenderCallBack( atomic, _rpPatchOpenGLAtomicRenderCB );
    }

    RWRETURN( atomic );
}


/* =====================================================================
 *  Static function definitions
 * ===================================================================== */

