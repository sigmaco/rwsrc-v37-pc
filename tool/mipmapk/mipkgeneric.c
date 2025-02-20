/*
 * Mipmap K Generic Extension.
 */

#include "rwcore.h"
#include "rpworld.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "mipkcalc.h"

RpAtomic *
MipKSumKValuesForAtomic(RpAtomic *atomic, void *pData)
{
    MipKInfo *kInfo;

    RWFUNCTION(RWSTRING("MipKSumKValuesForAtomic"));
    RWASSERT(NULL != atomic);

    kInfo = (MipKInfo *)pData;
    RWASSERT(NULL != kInfo);

    kInfo->process = MipKProcessMaterial;

    MipKSumKValuesForAtomicProcess(atomic, pData);

    RWRETURN(atomic);
}
