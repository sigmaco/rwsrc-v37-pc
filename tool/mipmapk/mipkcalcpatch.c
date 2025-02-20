/*
 * Mipmap K Patch Calculation function.
 */

#include "rwcore.h"
#include "rpworld.h"

#include "rppatch.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "mipkcalc.h"

const RpAtomic *
MipKSumKValuesForPatchProcess(const RpAtomic *atomic, void *pData)
{
    MipKInfo *kInfo;

    const RpPatchMesh *patchMesh;

    RWFUNCTION(RWSTRING("MipKSumKValuesForPatchProcess"));
    RWASSERT(NULL != atomic);

    kInfo = (MipKInfo *)pData;
    RWASSERT(NULL != kInfo);

    patchMesh = RpPatchAtomicGetPatchMesh(atomic);

    if( (NULL != patchMesh) &&
        (RpPatchMeshGetNumTexCoordSets(patchMesh) > 0) )
    {
        RwV3d *transformedCPs;
        RwTexCoords *texCoords;

        RwUInt32 numCPs;

        RwUInt32 size;
        RwUInt32 i;

        numCPs = RpPatchMeshGetNumControlPoints(patchMesh);
        size = sizeof(RwV3d) * numCPs;
        transformedCPs = (RwV3d *)RwMalloc(size,
            rwID_MIPMAPKPLUGIN | rwMEMHINTDUR_FUNCTION);
        RWASSERT(NULL != transformedCPs);

        RwV3dTransformPoints( transformedCPs,
                              RpPatchMeshGetPositions(patchMesh),
                              numCPs,
                              RwFrameGetLTM(RpAtomicGetFrame(atomic)) );

        texCoords = RpPatchMeshGetTexCoords( patchMesh,
                                             rwTEXTURECOORDINATEINDEX0 );
        RWASSERT(NULL != texCoords);

        for (i = 0; i < RpPatchMeshGetNumTriPatches(patchMesh); i++)
        {
            const RpTriPatch *triPatch;
            const RpMaterial *material;

            triPatch = RpPatchMeshGetTriPatch(patchMesh, i);
            material = RpPatchMeshGetTriPatchMaterial(patchMesh, i);

            if((kInfo->process)(material, kInfo->texture))
            {
                MipKAddTriangle( texCoords,
                                 transformedCPs,
                                 triPatch->cpIndices[0],
                                 triPatch->cpIndices[3],
                                 triPatch->cpIndices[9],
                                 kInfo );
            }
        }

        for (i = 0; i < RpPatchMeshGetNumQuadPatches(patchMesh); i++)
        {
            const RpQuadPatch *quadPatch;
            const RpMaterial *material;

            quadPatch = RpPatchMeshGetQuadPatch(patchMesh, i);
            material = RpPatchMeshGetQuadPatchMaterial(patchMesh, i);

            if((kInfo->process)(material, kInfo->texture))
            {
                MipKAddQuad( texCoords,
                             transformedCPs,
                             quadPatch->cpIndices[0],
                             quadPatch->cpIndices[3],
                             quadPatch->cpIndices[12],
                             quadPatch->cpIndices[15],
                             kInfo );
            }
        }

        RwFree(transformedCPs);
    }

    RWRETURN(atomic);
}
