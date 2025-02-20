#ifndef PATCHLOD_H
#define PATCHLOD_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rppatch.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/
typedef struct PatchLODCachedValues PatchLODCachedValues;
struct PatchLODCachedValues
{
    RwReal deltaLOD;
    RwReal recipRange;
};

typedef struct PatchLod PatchLod;
struct PatchLod
{
    RpPatchLODRange range;
    PatchLODCachedValues cached;
};

typedef struct PatchAtomicLod PatchAtomicLod;
struct PatchAtomicLod
{
    RpPatchLODCallBack callback;
    RpPatchLODUserData userData;
};

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/
#define rpPATCHDEFAULTMAXLOD (12)
#define rpPATCHDEFAULTMINLOD (4)
#define rpPATCHDEFAULTMINRANGE ((RwReal)4.0f)
#define rpPATCHDEFAULTMAXRANGE ((RwReal)200.0f)

#define _rpPatchLODGetDefaultCallBack()                                 \
    _rpPatchLODAtomicDefaultSelectLOD

#define _rpPatchLODRangeAssign(target, source)                          \
    ( *(target) = *(source) )

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern RwUInt32
_rpPatchLODAtomicDefaultSelectLOD( RpAtomic *atomic,
                                   RpPatchLODUserData userData );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PATCHLOD_H */
