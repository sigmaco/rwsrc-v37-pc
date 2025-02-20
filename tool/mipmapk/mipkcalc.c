/*
 * Mipmap K Calculation functions.
 */

#include "rwcore.h"
#include "rpworld.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "mipkcalc.h"

#define TEXELAREA(a, b, c)                                              \
    ( ((RwReal)0.5) * ( ( (a).u * (b).v ) - ( (b).u * (a).v ) +         \
                        ( (b).u * (c).v ) - ( (c).u * (b).v ) +         \
                        ( (c).u * (a).v ) - ( (a).u * (c).v ) ) )

void
MipKAddTriangle( RwTexCoords *texCoords,
                 RwV3d *transformedCPs,
                 RwUInt32 indexA,
                 RwUInt32 indexB,
                 RwUInt32 indexC,
                 MipKInfo *kInfo )
{
    /* add this one in */
    RwReal pixelArea, texelArea;
    RwReal polyArea, pixelToTexel;

    RwV3d  vTmp1, vTmp2, area;

    RwRaster *raster;

    RWFUNCTION(RWSTRING("MipKAddTriangle"));

    raster = RwTextureGetRaster(kInfo->texture);

    texelArea = TEXELAREA( texCoords[indexA],
                           texCoords[indexB],
                           texCoords[indexC] );

    texelArea = (RwReal)RwFabs(texelArea);
    texelArea *= RwRasterGetWidth(raster) *
                 RwRasterGetHeight(raster);

    /* calculate a 3d poly area */
    RwV3dSub( &vTmp1,
              &transformedCPs[indexB],
              &transformedCPs[indexA] );
    RwV3dSub( &vTmp2,
              &transformedCPs[indexC],
              &transformedCPs[indexA] );
    RwV3dCrossProduct(&area, &vTmp1, &vTmp2);

    polyArea = 0.5f * RwV3dLength(&area);

    if (polyArea > 0.0f && texelArea > 1.0f)
    {
        RwReal temp;

        /* calculate a screenspace (pixel) area */
        pixelArea = polyArea * kInfo->polyToPixelArea;

        /* calculate the pixel to texel ratio */
        pixelToTexel = pixelArea / texelArea;

        /* pixel to texel ratio scales linearly, i.e twice
         * as far away has 1/4 pixel to texel ratio */
        rwSqrt(&temp, pixelToTexel);
        kInfo->kSum += temp;

        kInfo->numCounts++;
    }

    RWRETURNVOID();
}

void
MipKAddQuad( RwTexCoords *texCoords,
             RwV3d *transformedCPs,
             RwUInt32 indexA,
             RwUInt32 indexB,
             RwUInt32 indexC,
             RwUInt32 indexD,
             MipKInfo *kInfo )
{
    /* add this one in */
    RwReal pixelArea, texelArea;
    RwReal polyArea, pixelToTexel;

    RwV3d  vTmp1, vTmp2, area;

    RwRaster *raster;

    RWFUNCTION(RWSTRING("MipKAddQuad"));

    raster = RwTextureGetRaster(kInfo->texture);

    texelArea = TEXELAREA( texCoords[indexA],
                           texCoords[indexB],
                           texCoords[indexD] ) +
                TEXELAREA( texCoords[indexA],
                           texCoords[indexD],
                           texCoords[indexC] );

    texelArea = (RwReal)RwFabs(texelArea);
    texelArea *= RwRasterGetWidth(raster) *
                 RwRasterGetHeight(raster);

    /* calculate a 3d poly area */
    RwV3dSub( &vTmp1,
              &transformedCPs[indexB],
              &transformedCPs[indexA] );
    RwV3dSub( &vTmp2,
              &transformedCPs[indexD],
              &transformedCPs[indexA] );
    RwV3dCrossProduct(&area, &vTmp1, &vTmp2);

    polyArea = 0.5f * RwV3dLength(&area);

    RwV3dSub( &vTmp1,
              &transformedCPs[indexD],
              &transformedCPs[indexA] );
    RwV3dSub( &vTmp2,
              &transformedCPs[indexC],
              &transformedCPs[indexA] );
    RwV3dCrossProduct(&area, &vTmp1, &vTmp2);

    polyArea += 0.5f * RwV3dLength(&area);

    if (polyArea > 0.0f && texelArea > 1.0f)
    {
        RwReal temp;
        /* calculate a screenspace (pixel) area */
        pixelArea = polyArea * kInfo->polyToPixelArea;

        /* calculate the pixel to texel ratio */
        pixelToTexel = pixelArea / texelArea;

        /* pixel to texel ratio scales linearly, i.e twice
         * as far away has 1/4 pixel to texel ratio */
        rwSqrt(&temp, pixelToTexel);
        kInfo->kSum += temp;

        kInfo->numCounts++;
    }

    RWRETURNVOID();
}

const RpWorldSector *
MipKSumKValuesForSectorProcess(const RpWorldSector *sector, void *pData)
{
    MipKInfo *kInfo;

    RwUInt32 i;

    RWFUNCTION(RWSTRING("MipKSumKValuesForSectorProcess"));
    RWASSERT(NULL != sector);

    kInfo = (MipKInfo *)pData;
    RWASSERT(NULL != kInfo);

    for( i = 0; i < sector->numTriangles; i++ )
    {
        const RpTriangle *tri;
        const RpMaterial *material;

        tri = &sector->triangles[i];
        RWASSERT(NULL != tri);

        material = RpWorldGetMaterial(kInfo->world, tri->matIndex);
        RWASSERT(NULL != material);

        if((kInfo->process)(material, kInfo->texture))
        {
            MipKAddTriangle( sector->texCoords[0],
                             sector->vertices,
                             tri->vertIndex[0],
                             tri->vertIndex[1],
                             tri->vertIndex[2],
                             kInfo );
        }
    }

    RWRETURN(sector);
}

const RpAtomic *
MipKSumKValuesForAtomicProcess(const RpAtomic *atomic, void *pData)
{
    MipKInfo *kInfo;

    RpGeometry *geometry;

    RWFUNCTION(RWSTRING("MipKSumKValuesForAtomicProcess"));
    RWASSERT(NULL != atomic);

    kInfo = (MipKInfo *)pData;
    RWASSERT(NULL != kInfo);

    geometry = RpAtomicGetGeometry(atomic);

    if( (RpGeometryGetFlags(geometry) & rpGEOMETRYNATIVE) == rpGEOMETRYNATIVE )
    {
        RWMESSAGE((RWSTRING("Geometry is pre-instanced - unable to calculate k values.")));
        RWRETURN(atomic);
    }

    if( (NULL != geometry) &&
        (RpGeometryGetNumTexCoordSets(geometry) > 0) )
    {
        RpMorphTarget *morphTarget;
        RwV3d *transformedVerts;
        RwTexCoords *texCoords;

        RwUInt32 numVertices;

        RwUInt32 size;
        RwUInt32 i;

        numVertices = RpGeometryGetNumVertices(geometry);
        size = sizeof(RwV3d) * numVertices;
        transformedVerts = (RwV3d *)RwMalloc(size,
            rwID_MIPMAPKPLUGIN | rwMEMHINTDUR_FUNCTION);
        RWASSERT(NULL != transformedVerts);

        morphTarget = RpGeometryGetMorphTarget(geometry, 0);
        RWASSERT(NULL != morphTarget);

        RwV3dTransformPoints( transformedVerts,
                              RpMorphTargetGetVertices(morphTarget),
                              numVertices,
                              RwFrameGetLTM(RpAtomicGetFrame(atomic)) );

        texCoords = RpGeometryGetVertexTexCoords( geometry,
                                                  rwTEXTURECOORDINATEINDEX0 );
        RWASSERT(NULL != texCoords);

        for( i = 0; i < (RwUInt32)RpGeometryGetNumTriangles(geometry); i++)
        {
            const RpTriangle *tri;
            const RpMaterial *material;

            tri = &(RpGeometryGetTriangles(geometry)[i]);
            material = RpGeometryGetMaterial(geometry, tri->matIndex);

            RWASSERT(NULL != kInfo->process);
            if((kInfo->process)(material, kInfo->texture))
            {
                MipKAddTriangle( texCoords,
                                 RpMorphTargetGetVertices(morphTarget),
                                 tri->vertIndex[0],
                                 tri->vertIndex[1],
                                 tri->vertIndex[2],
                                 kInfo );
            }
        }

        RwFree(transformedVerts);
    }

    RWRETURN(atomic);
}
