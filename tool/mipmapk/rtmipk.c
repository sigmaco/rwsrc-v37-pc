/*
 * Mipmap K
 */

/**
 * \ingroup rtmipk
 * \page rtmipkoverview RtMipK Toolkit Overview
 *
 * \par Requirements
 * There are four versions of the RtMipK libraries in the RenderWare Graphics SDK.
 * They are fully featured versions on the RtMipK toolkit, and they contain
 * identical APIs. However, because the rendering pipelines are large we've taken
 * the step to compile different versions of the toolkit so that the user can select
 * precisely the pipelines they will be using.
 *
 * \par Requirements for rtmipk library
 * \li \b Headers: rwcore.h, rpworld.h, rpmipkl.h, rtmipk.h
 * \li \b Libraries: rwcore, rpworld, rpmipkl, rtmipk
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpMipmapKLPluginAttach,
 *
 * \par Requirements for rtmipkmatfx library
 * \li \b Headers: rwcore.h, rpworld.h, rpmatfx.h, rpmipkl.h, rtmipk.h
 * \li \b Libraries: rwcore, rpworld, rpmatfx, rpmipkl, rtmipkmatfx
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpMatFXPluginAttach,
 *     \ref RpMipmapKLPluginAttach,
 *
 * \par Requirements for rtmipkpatch library
 * \li \b Headers: rwcore.h, rpworld.h, rppatch.h, rpmipkl.h, rtmipk.h
 * \li \b Libraries: rwcore, rpworld, rppatch, rpmipkl, rtmipkpatch
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpPatchPluginAttach,
 *     \ref RpMipmapKLPluginAttach,
 *
 * \par Requirements for rpmipkpatchmatfx library
 * \li \b Headers: rwcore.h, rpworld.h, rppatch.h, rpmatfx.h, rpmipkl.h, rtmipk.h
 * \li \b Libraries: rwcore, rpworld, rppatch, rpmatfx, rpmipkl, rtmipkpatchmatfx
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpPatchPluginAttach,
 *     \ref RpMatFXPluginAttach, \ref RpMipmapKLPluginAttach
 *
 * \note Only one of the rtmipk libraries should be used in an application at once.
 *
 * \subsection mipkoverview Overview
 * The RtMipK Toolkit is used to try and calculate realistic "K"
 * values for textures in \ref RpClump's and \ref RpWorld's. The "K" values
 * are stored in the texture with either the \ref rpmipkl plugin or directly
 * into the driver texture specific data.
 *
 * The set "K" value is calculated as the average "K" value for all the
 * triangles using a \ref RwTexture.
 *
 * So we first calculate the screen texel ratio by:
 * \verbatim

   screen texel ratio :=        (video mode width * video mode height)
                         ----------------------------------------------------
                         ((view window width * 2) * (view window height * 2))
   \endverbatim
 *
 * Then the triangle texel area is calculated, by calculating
 * the screen pixel area per triangle:
 * \verbatim

    screen space pixel area := triangle area * screen texel ratio
   \endverbatim
 *
 * Then the pixel to texel ratio for the triangle:
 * \verbatim

   pixel to texel ratio := screen space pixel area * texel area
   \endverbatim
 *
 * The root of this the pixel to texel ratio is added to the sum for all
 * the triangles referencing the texture.
 *
 * Finally we calculate the average "K" value by:
 * \verbatim

   average "K" := - ( log( sum / count ) / log(2) )
   \endverbatim
 *
 * This MipMap "K" value is then stored in the texture.
 */

#include "rpplugin.h"
#include "rwcore.h"
#include "rpdbgerr.h"
#include "rpworld.h"
#include "rtmipk.h"

#include "mipkcalc.h"

#if (!defined(SKY2_DRVMODEL_H)) && (!defined(NULLSKY_DRVMODEL_H))
#include "rpmipkl.h"
#endif /* (!defined(SKY2_DRVMODEL_H)) && (!defined(NULLSKY_DRVMODEL_H)) */

static RwTexture *
MipKCalculateClumpKValue(RwTexture *texture, void *pData)
{
    MipKInfo *kInfo = (MipKInfo *)pData;

    RWFUNCTION(RWSTRING("MipKCalculateClumpKValue"));
    RWASSERT(NULL != kInfo);
    RWASSERT(NULL != texture);

    kInfo->texture = texture;
    kInfo->kSum = 0.0f;
    kInfo->numCounts = 0;

    RpClumpForAllAtomics(kInfo->clump, MipKSumKValuesForAtomic, kInfo);

    if(kInfo->numCounts > 0)
    {
        /* find k = log2(1/averageDist) */
        RwReal averageDist;
        RwReal k;

        averageDist = ((kInfo->kSum) / ((RwReal)kInfo->numCounts));
        k  = -((RwReal)((RwLog(averageDist)) / RwLog(2.0)));

        /* Set the mipmap k value. */
#if (defined(SKY2_DRVMODEL_H)) || (defined(NULLSKY_DRVMODEL_H))
        RpSkyTextureSetMipmapK(texture, k);
#else /* (defined(SKY2_DRVMODEL_H)) || (defined(NULLSKY_DRVMODEL_H)) */
        RpMipmapKLTextureSetK(texture, k);
#endif /* (defined(SKY2_DRVMODEL_H)) || (defined(NULLSKY_DRVMODEL_H)) */
    }

    RWRETURN(texture);
}

static RwTexture *
MipKCalculateWorldKValue(RwTexture *texture, void *pData)
{
    MipKInfo *kInfo = (MipKInfo *)pData;

    RWFUNCTION(RWSTRING("MipKCalculateWorldKValue"));
    RWASSERT(NULL != kInfo);
    RWASSERT(NULL != texture);

    kInfo->texture = texture;
    kInfo->kSum = 0.0f;
    kInfo->numCounts = 0;

    RpWorldForAllWorldSectors(kInfo->world, MipKSumKValuesForSector, kInfo);

    if(kInfo->numCounts > 0)
    {
        /* find k = log2(1/averageDist) */
        RwReal averageDist;
        RwReal k;

        averageDist = ((kInfo->kSum) / ((RwReal)kInfo->numCounts));
        k  = -((RwReal)((RwLog(averageDist)) / RwLog(2.0)));

        /* Set the mipmap k value. */
#if (defined(SKY2_DRVMODEL_H)) || (defined(NULLSKY_DRVMODEL_H))
        RpSkyTextureSetMipmapK(texture, k);
#else /* (defined(SKY2_DRVMODEL_H)) || (defined(NULLSKY_DRVMODEL_H)) */
        RpMipmapKLTextureSetK(texture, k);
#endif /* (defined(SKY2_DRVMODEL_H)) || (defined(NULLSKY_DRVMODEL_H)) */
    }

    RWRETURN(texture);
}

/**
 * \ingroup rtmipk
 * \ref RtMipKClumpCalculateKValues
 * computes the Sky specific "K" values for textures used in a clump's
 * atomics, based on the current video mode and the view window of
 * the specified camera. RtMipKClumpCalculateKValues uses the current 
 * texture dictionary to get the textures to process. So, set the correct 
 * texture dictionary to be the current texture dictionary before calling 
 * this function.
 *
 * \param clump  pointer to the clump
 * \param camera  pointer to the camera
 *
 * \see RtMipKWorldCalculateKValues
 */
void
RtMipKClumpCalculateKValues(RpClump *clump, RwCamera *camera)
{
    MipKInfo kInfo;

    RwInt32 vidMode;
    RwVideoMode modeInfo;

    RwReal viewWidth;
    RwReal viewHeight;

    RwTexDictionary *texDict = NULL;

    RWAPIFUNCTION(RWSTRING("RtMipKClumpCalculateKValues"));

    RWASSERT(NULL != clump);
    RWASSERT(NULL != camera);

    vidMode = RwEngineGetCurrentVideoMode();

    RwEngineGetVideoModeInfo(&modeInfo, vidMode);
    viewWidth = RwCameraGetViewWindow(camera)->x * 2.0f;
    viewHeight = RwCameraGetViewWindow(camera)->y * 2.0f;

    kInfo.polyToPixelArea = ( (modeInfo.width * modeInfo.height) /
                              (viewWidth * viewHeight) );
    kInfo.clump = clump;
    kInfo.world = (RpWorld *)NULL;

    texDict = RwTexDictionaryGetCurrent();
    if (texDict)
    {
        RwTexDictionaryForAllTextures( texDict,
                                       MipKCalculateClumpKValue, &kInfo );
    }

    RWRETURNVOID();
}

/**
 * \ingroup rtmipk
 * \ref RtMipKWorldCalculateKValues
 * computes the Sky specific "K" values for textures used in a world,
 * based on the current video mode and the view window of the specified
 * camera. RtMipKWorldCalculateKValues uses the current texture dictionary 
 * to get the textures to process. So, set the correct texture dictionary 
 * to be the current texture dictionary before calling this function.
 *
 * \param world  pointer to the world
 * \param camera  pointer to the camera
 *
 * \see RtMipKClumpCalculateKValues
 */
void
RtMipKWorldCalculateKValues(RpWorld *world, RwCamera *camera)
{
    MipKInfo kInfo;

    RwInt32 vidMode;
    RwVideoMode modeInfo;
    RwUInt32 flags;

    RwReal viewWidth;
    RwReal viewHeight;

    RwTexDictionary *texDict = NULL;

    RWAPIFUNCTION(RWSTRING("RtMipKWorldCalculateKValues"));

    RWASSERT(NULL != world);
    RWASSERT(NULL != camera);

    flags = RpWorldGetFlags(world);

    if( (flags & rpWORLDNATIVE) == rpWORLDNATIVE )
    {
        RWMESSAGE((RWSTRING("World is pre-instanced - unable to calculate k values.")));
    }
    else if(flags & (rpWORLDTEXTURED|rpWORLDTEXTURED2))
    {
        vidMode = RwEngineGetCurrentVideoMode();

        RwEngineGetVideoModeInfo(&modeInfo, vidMode);
        viewWidth = RwCameraGetViewWindow(camera)->x * 2.0f;
        viewHeight = RwCameraGetViewWindow(camera)->y * 2.0f;

        kInfo.polyToPixelArea = ( (modeInfo.width * modeInfo.height) /
                                  (viewWidth * viewHeight) );
        kInfo.clump = (RpClump *)NULL;
        kInfo.world = world;
        texDict = RwTexDictionaryGetCurrent();
        if (texDict)
        {
            RwTexDictionaryForAllTextures(RwTexDictionaryGetCurrent(),
                                          MipKCalculateWorldKValue, &kInfo);
        }
    }

    RWRETURNVOID();
}
