/*
 * Mipmap K Patch Extension.
 */

#include "rwcore.h"
#include "rpworld.h"

#include "rppatch.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "mipkcalc.h"

RpAtomic *
MipKSumKValuesForAtomic(RpAtomic *atomic, void *pData)
{
    MipKInfo *kInfo;

    const RpPatchMesh *patchMesh;

    RWFUNCTION(RWSTRING("MipKSumKValuesForAtomic"));
    RWASSERT(NULL != atomic);

    kInfo = (MipKInfo *)pData;
    RWASSERT(NULL != kInfo);

    kInfo->process = MipKProcessMaterial;

    patchMesh = RpPatchAtomicGetPatchMesh(atomic);
    if(NULL != patchMesh)
    {
        MipKSumKValuesForPatchProcess(atomic, pData);
    }
    else
    {
        MipKSumKValuesForAtomicProcess(atomic, pData);
    }

    RWRETURN(atomic);
}
