/*
 * Mipmap K World MatFX Extension.
 */

#include "rwcore.h"
#include "rpworld.h"
#include "rpmatfx.h"

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

    kInfo->process = MipKProcessMaterialFX;

    MipKSumKValuesForSectorProcess(sector, pData);

    RWRETURN(sector);
}

RwBool
MipKProcessMaterialFX(const RpMaterial *material, const RwTexture *texture)
{
    RpMatFXMaterialFlags effect;

    RwBool process;

    RWFUNCTION(RWSTRING("MipKProcessMaterialFX"));

    effect = RpMatFXMaterialGetEffects(material);

    process = (RpMaterialGetTexture(material) == texture);

    switch(effect)
    {
        case rpMATFXEFFECTBUMPMAP:
            process |=
                ( RpMatFXMaterialGetBumpMapBumpedTexture(material) ==
                  texture );
            break;
        case rpMATFXEFFECTENVMAP:
            process |=
                ( RpMatFXMaterialGetEnvMapTexture(material) == texture );
            break;
        case rpMATFXEFFECTBUMPENVMAP:
            process |=
                ( RpMatFXMaterialGetBumpMapBumpedTexture(material) ==
                  texture );
            process |=
                ( RpMatFXMaterialGetEnvMapTexture(material) == texture );
            break;
        case rpMATFXEFFECTDUAL:
            process |=
                ( RpMatFXMaterialGetDualTexture(material) == texture );
            break;
        case rpMATFXEFFECTNULL:
        default:
            break;
    }

    RWRETURN(process);
}
