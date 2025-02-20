/*
 * Mipmap K Calc header.
 */

#ifndef MIPKCALC_H
#define MIPKCALC_H

#include "rwcore.h"
#include "rpworld.h"

typedef struct MipKInfo MipKInfo;
struct MipKInfo
{
    RpClump   *clump;
    RpWorld   *world;
    RwTexture *texture;
    RwReal     kSum;
    RwInt32    numCounts;
    RwReal     polyToPixelArea;
    RwBool    (*process)( const RpMaterial *material,
                          const RwTexture *texture );

};

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern RpAtomic *
MipKSumKValuesForAtomic(RpAtomic *atomic, void *pData);

extern RpWorldSector *
MipKSumKValuesForSector(RpWorldSector *sector, void *pData);

extern void
MipKAddTriangle( RwTexCoords *texCoords,
                 RwV3d *transformedCPs,
                 RwUInt32 indexA,
                 RwUInt32 indexB,
                 RwUInt32 indexC,
                 MipKInfo *kInfo );

extern void
MipKAddQuad( RwTexCoords *texCoords,
             RwV3d *transformedCPs,
             RwUInt32 indexA,
             RwUInt32 indexB,
             RwUInt32 indexC,
             RwUInt32 indexD,
             MipKInfo *kInfo );

extern RwBool
MipKProcessMaterial(const RpMaterial *material, const RwTexture *texture);

extern RwBool
MipKProcessMaterialFX(const RpMaterial *material, const RwTexture *texture);

extern const RpWorldSector *
MipKSumKValuesForSectorProcess(const RpWorldSector *sector, void *pData);

extern const RpAtomic *
MipKSumKValuesForAtomicProcess(const RpAtomic *atomic, void *pData);

extern const RpAtomic *
MipKSumKValuesForPatchProcess(const RpAtomic *atomic, void *pData);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* MIPKCALC_H */
