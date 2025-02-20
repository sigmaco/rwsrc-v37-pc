/*
 * Mipmap K World Generic Extension.
 */

#include "rwcore.h"
#include "rpworld.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "mipkcalc.h"

RpWorldSector *
MipKSumKValuesForSector(RpWorldSector *sector, void *pData)
{
    MipKInfo *kInfo;

    RWFUNCTION(RWSTRING("MipKSumKValuesForSector"));
    RWASSERT(NULL != sector);

    kInfo = (MipKInfo *)pData;
    RWASSERT(NULL != kInfo);

    kInfo->process = MipKProcessMaterial;

    MipKSumKValuesForSectorProcess(sector, pData);

    RWRETURN(sector);
}

RwBool
MipKProcessMaterial(const RpMaterial *material, const RwTexture *texture)
{
    RwBool process;

    RWFUNCTION(RWSTRING("MipKProcessMaterial"));

    process = (RpMaterialGetTexture(material) == texture);

    RWRETURN(process);
}

