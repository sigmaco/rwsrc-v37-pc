/***********************************************************************
 *
 * Module:  patchopenglskin.c
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

#include "rpskin.h"

#include "patch.h"
#include "patchatomic.h"
#include "patchgeom.h"
#include "patchskin.h"

#include "patchopengl.h"
#include "patchopenglskin.h"
#include "patchopenglskinrender.h"


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

    if ( 0 != (pipes & rpPATCHPIPELINESKINNED) )
    {
        pipe = _rpSkinOpenGLPipelineCreate( rpSKINTYPEGENERIC, NULL );
        RWASSERT( NULL != pipe );
        if ( NULL == pipe )
        {
            RWRETURN( FALSE );
        }

        pipe->pluginId = rwID_PATCHPLUGIN;
        pipe->pluginData = rpPATCHTYPESKIN;

         _rpPatchOpenGLPipeline(rpPATCHOPENGLPIPELINESKINNED) = pipe;
        RWASSERT( NULL != _rpPatchOpenGLPipeline(rpPATCHOPENGLPIPELINESKINNED) );
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
                         RpPatchType type )
{
    RpPatchMesh             *patchMesh;

    RwBool                  skinEnabled;

    RpPatchOpenGLPipeline   openglPatchPipe;

    PatchAtomicData         *atomicData;

    RpAtomicCallBackRender  curAtomicRenderCB;

    RxPipeline              *pipeline;


    RWFUNCTION( RWSTRING( "_rpPatchPipelinesAttach" ) );

    RWASSERT( NULL != atomic );

    /* get the patch mesh */
    patchMesh = RpPatchAtomicGetPatchMesh( atomic );
    RWASSERT( NULL != patchMesh );

    /* has the patch mesh got skin data? */
    skinEnabled = (NULL != RpPatchMeshGetSkin(patchMesh));

    /* do we have a hint? */
    if ( rpNAPATCHTYPE == type )
    {
        if ( FALSE == skinEnabled )
        {
            openglPatchPipe = rpPATCHOPENGLPIPELINEGENERIC;
        }
        else
        {
            openglPatchPipe = rpPATCHOPENGLPIPELINESKINNED;
        }
    }
    else
    {
        switch( type )
        {
        case rpPATCHTYPEGENERIC:
        case rpPATCHTYPEMATFX:
            {
                openglPatchPipe = rpPATCHOPENGLPIPELINEGENERIC;
            }
            break;

        case rpPATCHTYPESKIN:
        case rpPATCHTYPESKINMATFX:
            {
                openglPatchPipe = rpPATCHOPENGLPIPELINESKINNED;
            }
            break;

        default:
            {
                openglPatchPipe = rpNAPATCHOPENGLPIPELINE;
            }
        }
    }

    RWASSERT( rpNAPATCHOPENGLPIPELINE < openglPatchPipe );
    RWASSERT( rpPATCHOPENGLPIPELINEMAX > openglPatchPipe );

    pipeline = RpPatchGetOpenGLPipeline( openglPatchPipe );
    RWASSERT( NULL != pipeline );

    atomic = RpAtomicSetPipeline( atomic, pipeline );
    RWASSERT( NULL != atomic );

    atomicData = PATCHATOMICGETDATA(atomic);

    curAtomicRenderCB = RpAtomicGetRenderCallBack( atomic );

    if ( FALSE == skinEnabled )
    {
        if ( curAtomicRenderCB != _rpPatchOpenGLAtomicRenderCB )
        {
            atomicData->openglLocals.renderCallback = curAtomicRenderCB;
            RpAtomicSetRenderCallBack( atomic, _rpPatchOpenGLAtomicRenderCB );
        }
    }
    else
    {
        if ( curAtomicRenderCB != _rpPatchOpenGLSkinAtomicRenderCB )
        {
            atomicData->openglLocals.renderCallback = curAtomicRenderCB;
            RpAtomicSetRenderCallBack( atomic, _rpPatchOpenGLSkinAtomicRenderCB );
        }
    }

    RWRETURN( atomic );
}


/* =====================================================================
 *  Static function definitions
 * ===================================================================== */

