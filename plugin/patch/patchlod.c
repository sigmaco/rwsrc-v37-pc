/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   patchlod.c                                                 -*
 *-                                                                         -*
 *-  Purpose :   General patch handling.                                    -*
 *-                                                                         -*
 *===========================================================================*/

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rppatch.h"

#include "patchlod.h"
#include "patchatomic.h"
#include "patch.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

/*****************************************************************************
 _rpPatchLODAtomicDefaultSelectLOD

 Default lod atomic function. Calculates the lod on a linear law between
 the minimum lod and maximum lod range.

 Inputs :   atomice  - Pointer to atomic.
            userData - Unused user data.
 Outputs:   RwUInt32 - LOD value, between rpPATCHLODMINVALUE and
                       rpPATCHLODMAXVALUE.
 */
RwUInt32
_rpPatchLODAtomicDefaultSelectLOD( RpAtomic *atomic,
                                   RpPatchLODUserData userData __RWUNUSED__ )
{
    PatchEngineData *engineData;

    RwFrame *frame;
    RwMatrix *ltm;
    RwCamera *camera;
    RwMatrix *viewMatrix;
    RwMatrix xForm;
    RwUInt32 result;
    RwReal z;

    RWFUNCTION(RWSTRING("_rpPatchLODAtomicDefaultSelectLOD"));
    RWASSERT(NULL != atomic);

    /* Get the engine data. */
    engineData = PATCHENGINEGETDATA(RwEngineInstance);

    /* Get the frame, ltm, camera, viewMatrix. */
    frame = (RwFrame *)rwObjectGetParent(atomic);
    RWASSERT(NULL != frame);
    ltm = RwFrameGetLTM(frame);
    RWASSERT(NULL != ltm);
    camera = (RwCamera *)RWSRCGLOBAL(curCamera);
    RWASSERT(NULL != camera);
    viewMatrix = &(camera->viewMatrix);
    RWASSERT(NULL != viewMatrix);

    RwMatrixMultiply(&xForm, ltm, viewMatrix);
    z = RwMatrixGetPos(&xForm)->z;

    if( z >= engineData->lod.range.maxRange )
    {
        result = engineData->lod.range.minLod;
    }
    else if( z <= engineData->lod.range.minRange )
    {
        result = engineData->lod.range.maxLod;
    }
    else
    {
        RwReal zDiff;
        RwReal realResult;

        zDiff = z - engineData->lod.range.minRange;

        realResult = zDiff * engineData->lod.cached.deltaLOD +
                     engineData->lod.range.maxLod;

        result = RwInt32FromRealMacro(realResult);
    }

    RWASSERT(rpPATCHLODMINVALUE <= result);
    RWASSERT(rpPATCHLODMAXVALUE >= result);
    RWRETURN(result);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rppatch
 * \ref RpPatchAtomicSetPatchLODCallBack is used to define a callback function
 * that determines which LOD should be used to facet the specified atomic.
 * This allows a custom LOD selection defined by the application.
 *
 * If a NULL callback is specified then a default callback is registered for
 * the atomic that calculates the LOD using linear interpolation between
 * a near and far range.
 *
 * Values beyond the range are clamped to use the max \ref rpPATCHLODMAXVALUE
 * and min \ref rpPATCHLODMINVALUE LOD respectively.
 *
 * The format of the callback function is \ref RpPatchLODCallBack:
 *
 * \ref RwUInt32 (* \ref RpPatchLODCallBack)
 *                   ( \ref RpAtomic *atomic,
 *                     \ref RpPatchLODUserData userData );
 *
 * The patch plugin must be attached before using this function.
 *
 * \param atomic   Pointer to the atomic containing the patch mesh.
 * \param callback The callback function to apply to the atomic.
 * \param userData Pointer to user-supplied data to pass to the callback
 *                 function.
 *
 * \return TRUE if successful: FALSE otherwise.
 *
 * \see RpPatchAtomicGetPatchLODCallBack
 * \see RpPatchSetDefaultLODCallBackRange
 * \see RpPatchGetDefaultLODCallBackRange
 */
RwBool
RpPatchAtomicSetPatchLODCallBack( RpAtomic *atomic,
                                  RpPatchLODCallBack callback,
                                  RpPatchLODUserData userData )
{
    PatchAtomicData *atomicData;

    RWAPIFUNCTION(RWSTRING("RpPatchAtomicSetPatchLODCallBack"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != atomic);

    /* Get the atomic data extension. */
    atomicData = PATCHATOMICGETDATA(atomic);
    RWASSERT(NULL != atomicData);

    /* If the call back is NULL we set the default call back. */
    if(NULL == callback)
    {
        RWASSERT(NULL != _rpPatchLODGetDefaultCallBack());
        atomicData->lod.callback = _rpPatchLODGetDefaultCallBack();
    }
    else
    {
        RWASSERT(NULL != callback);
        atomicData->lod.callback = callback;
    }

    /* Set the call back user data. */
    atomicData->lod.userData = userData;

    /* Make sure we have a call back - as we're just going to call it later. */
    RWASSERT(NULL != atomicData->lod.callback);

    /* Return. */
    RWRETURN(TRUE);
}

/**
 * \ingroup rppatch
 * \ref RpPatchAtomicGetPatchLODCallBack is used to retrieve the atomic's
 * LOD callback function.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param atomic   Pointer to the atomic containing the patch mesh.
 * \param callback Pointer to receive the LOD callback function.
 * \param userData Pointer to receive the user-supplied data.
 *
 * \return TRUE if successful: FALSE otherwise.
 *
 * \see RpPatchAtomicSetPatchLODCallBack
 * \see RpPatchSetDefaultLODCallBackRange
 * \see RpPatchGetDefaultLODCallBackRange
 */
RwBool
RpPatchAtomicGetPatchLODCallBack( const RpAtomic *atomic,
                                  RpPatchLODCallBack *callback,
                                  RpPatchLODUserData *userData )
{
    const PatchAtomicData *atomicData;

    RWAPIFUNCTION(RWSTRING("RpPatchAtomicGetPatchLODCallBack"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != atomic);

    /* Get the atomic data extension. */
    atomicData = PATCHATOMICGETCONSTDATA(atomic);
    RWASSERT(NULL != atomicData);

    /* Return the call back function. */
    if(NULL != callback)
    {
        *callback = atomicData->lod.callback;
    }

    /* Return the user data. */
    if(NULL != userData)
    {
        *userData = atomicData->lod.userData;
    }

    /* Return. */
    RWRETURN(TRUE);
}

/**
 * \ingroup rppatch
 * \ref RpPatchSetDefaultLODCallBackRange is used to setup the default LOD
 * callback. The values in \ref RpPatchLODRange are used by the default
 * patch atomic LOD callback.
 *
 * The minimum range defines the distance when the patch is at the maximum LOD.
 * Atomics closer than this distance are facetted at the maximum LOD.
 * The maximum range defines the distance when the patch is at the minimum LOD.
 * Atomics futher than this distance are facetted at the minimum LOD.
 * Atomics within the minimum and maximum range will have an LOD
 * in inverse proportion to the distance from the viewer.
 * The minimum and maximum values set the minimum and maximum LOD.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param lodRange Pointer to the definition structure \ref RpPatchLODRange:
 * \li \ref RwUInt32 minLod - Minimum LOD value.
 * \li \ref RwUInt32 maxLod - Maximum LOD value.
 * \li \ref RwReal minRange - Minimum LOD range.
 * \li \ref RwReal maxRange - Maximum LOD range.
 *
 * \return TRUE if successful: FALSE otherwise.
 *
 * \see RpPatchAtomicSetPatchLODCallBack
 * \see RpPatchAtomicGetPatchLODCallBack
 * \see RpPatchGetDefaultLODCallBackRange
 */
RwBool
RpPatchSetDefaultLODCallBackRange( RpPatchLODRange *lodRange )
{
    PatchEngineData *engineData;

    RWAPIFUNCTION(RWSTRING("RpPatchSetDefaultLODCallBackRange"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != lodRange);
    RWASSERT(0 < lodRange->minRange);
    RWASSERT(lodRange->minRange < lodRange->maxRange);
    RWASSERT(lodRange->minLod <= lodRange->maxLod);
    RWASSERT(rpPATCHLODMINVALUE <= lodRange->minLod);
    RWASSERT(rpPATCHLODMAXVALUE >= lodRange->maxLod);

    /* Get the engine data. */
    engineData = PATCHENGINEGETDATA(RwEngineInstance);

    /* Copy the range data. */
    _rpPatchLODRangeAssign(&(engineData->lod.range), lodRange);

    /* Calculate the cached values. */
    engineData->lod.cached.recipRange =
        ((RwReal)1.0f) / (lodRange->maxRange - lodRange->minRange);
    engineData->lod.cached.deltaLOD =
        ((RwReal)(lodRange->minLod) - (RwReal)(lodRange->maxLod));
    engineData->lod.cached.deltaLOD *= engineData->lod.cached.recipRange;

    /* Return. */
    RWRETURN(TRUE);
}

/**
 * \ingroup rppatch
 * \ref RpPatchGetDefaultLODCallBackRange is used to query the setup of the
 * default LOD callback. The values returned in \ref RpPatchLODRange are used
 * by the default patch atomic LOD callback.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param lodRange Pointer to the query structure \ref RpPatchLODRange:
 *
 * \return TRUE if successful: FALSE otherwise.
 *
 * \see RpPatchAtomicSetPatchLODCallBack
 * \see RpPatchAtomicGetPatchLODCallBack
 * \see RpPatchSetDefaultLODCallBackRange
 */
RwBool
RpPatchGetDefaultLODCallBackRange( RpPatchLODRange *lodRange )
{
    const PatchEngineData *engineData;

    RWAPIFUNCTION(RWSTRING("RpPatchGetDefaultLODCallBackRange"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != lodRange);

    /* Get the engine data. */
    engineData = PATCHENGINEGETCONSTDATA(RwEngineInstance);

    /* Copy out the range data. */
    _rpPatchLODRangeAssign(lodRange, &(engineData->lod.range));

    /* Return. */
    RWRETURN(TRUE);
}
