/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   rpltmap.c                                                  -*
 *-                                                                         -*
 *-  Purpose :   RpLtMap plugin                                             -*
 *-                                                                         -*
 *===========================================================================*/

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/

/* Needed for memset */
#include "string.h"

#include "rwcore.h"
#include "rpworld.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "generic.h"
#include "rpltmap.h"


/**
 * \ingroup rpltmap
 * \page rpltmapoverview RpLtMap Plugin Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rpltmap.h
 * \li \b Libraries: rwcore, rpworld, rpltmap
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpLtMapPluginAttach
 *
 * \subsection ltmappluginoverview Overview
 * The lightmap plugin provides a means to render geometry using
 * detailed, full color, static lighting information from lightmap
 * textures.
 *
 * Before any of the lightmap functionality can be used the plugin
 * must be registered with RenderWare with:
 *   \li \ref RpLtMapPluginAttach
 *
 * Lightmap generation functionality is provided by the \ref rtltmap
 * toolkit. At run-time, each light-mapped \ref RpWorldSector or
 * \ref RpAtomic may reference one lightmap (several such objects
 * may share a single lightmap for efficiency). These lightmaps are
 * loaded (from individual texture files or the current texture
 * dictionary) automatically when the objects are read from disk.
 * At this point, platform-specific lightmap rendering pipelines
 * will also be attached to the objects. Note that these pipelines
 * are object pipelines, not material pipelines (i.e it is not
 * possible, currently, to vary rendering effects within lightmapped
 * objects on a mesh-by-mesh basis).
 *
 * The supplied lightmap rendering pipelines will take into account
 * dynamic vertex lighting correctly on all platforms (this will be
 * added to the light in the lightmaps). There are also several
 * rendering modes available for inspection of lightmaps (see
 * \ref RpLtMapSetRenderStyle).
 */

#if (defined(SKY2_DRVMODEL_H) || defined(NULLSKY_DRVMODEL_H))

/**
 * \defgroup rpltmapps2 PlayStation 2
 * \ingroup rpltmap
 *
 * Lightmap Plugin for RenderWare Graphics.
 *
 * PlayStation 2 Specific.
 */

/**
 * \ingroup rpltmapps2
 * \page rpltmapps2overview RpLtMap PlayStation 2 Overview
 *
 * On PlayStation 2, the lightmap rendering pipeline is implemented using a
 * unique two-pass 'darkmaps' approach. This requires that base textures be
 * pre-processed (see \ref rtltmapps2overview) and has some limitations:
 * \li base textures may not contain transparency information
 *     (though the material may)
 * \li only tri-stripped geometry is supported (this is much faster
 *     to render on PlayStation 2 than tri-lists and it is trivial to
 *     convert tri-lists to tri-strips should the need arise)
 */

#endif /* (defined(SKY2_DRVMODEL_H) || defined(NULLSKY_DRVMODEL_H)) */

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
#if (defined(RWDEBUG))
long _rpLtMapStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

/*===========================================================================*
 *--- Global Variables ------------------------------------------------------*
 *===========================================================================*/

LtMapGlobals _rpLtMapGlobals;

RwChar rpLtMapDefaultPrefixString[] = RWSTRING("ltmp");
RwChar rpLtMapImageExtension[]      = RWSTRING(".png");

/*===========================================================================*
 *--- Defines ---------------------------------------------------------------*
 *===========================================================================*/
#if (defined(SKY2_DRVMODEL_H) || defined(NULLSKY_DRVMODEL_H))
#define FORMAT_MSG "8bit non mipmaped Palettized texture"
#elif (defined(D3D8_DRVMODEL_H))
#define FORMAT_MSG "32bit non mipmaped"
#elif (defined(D3D9_DRVMODEL_H))
#define FORMAT_MSG "32bit non mipmaped"
#elif (defined(OPENGL_DRVMODEL_H))
#define FORMAT_MSG "32bit auto mipmaped"
#elif (defined(GCN_DRVMODEL_H) || defined(NULLGCN_DRVMODEL_H))
#define FORMAT_MSG "16bit auto mipmaped"
#elif (defined(XBOX_DRVMODEL_H) || defined(NULLXBOX_DRVMODEL_H))
#define FORMAT_MSG "32bit auto mipmaped"
#elif (defined(SOFTRAS_DRVMODEL_H))
#define FORMAT_MSG "32bit non mipmaped"
#elif (defined(NULL_DRVMODEL_H))
#define FORMAT_MSG "32bit auto mipmaped"
#endif

#define rpLTMAPDEFAULTLIGHTMAPDENSITY   0

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/

#if (defined(SKY2_DRVMODEL_H) || defined(NULLSKY_DRVMODEL_H))
static RwTexture *
LtMapInitializeWhiteTex(void)
{
    RwRGBA     opaqueWhite = {255, 255, 255, 255};
    RwTexture *texture;
    RwRaster  *raster;
    RwUInt32   stride, x, y;
    RwUInt32   pixelColor;
    RwUInt8   *pixels, *pixel;

    RWFUNCTION(RWSTRING("LtMapInitializeWhiteTex"));

    /* Set up the white texture used on PS2 to allow the "renderstyles" */
    raster = RwRasterCreate(32, 32, 32,
                 rwRASTERTYPETEXTURE|rwRASTERFORMAT8888);
    if (NULL == raster) RWRETURN(NULL);

    texture = RwTextureCreate(raster);
    if (NULL == texture)
    {
        RwRasterDestroy(raster);
        RWRETURN(NULL);
    }

    pixels = RwRasterLock(raster, 0, rwRASTERLOCKWRITE);
    if (NULL != pixels)
    {
        stride = RwRasterGetStride(raster);
        pixelColor = RwRGBAToPixel(&opaqueWhite, rwRASTERFORMAT8888);
        for (y = 0;y < 32;++y)
        {
            pixel = &(pixels[y*stride]);
            for (x = 0;x < 32;++x)
            {
               *(RwUInt32 *)pixel = pixelColor;
                pixel += 4;
            }
        }
        RWRETURN(texture);
    }

    RwTextureDestroy(texture);

    RWRETURN((RwTexture *)NULL);
}

#endif /* (defined(SKY2_DRVMODEL_H) || defined(NULLSKY_DRVMODEL_H)) */

RwTexture *
_rpLtMapClearLightMap(RwTexture *lightMap, RwRGBA *color)
{
    RwRGBA    colors[2] = {{255, 255, 255, 255}, {  0,  0,  0, 255}};
    RwRaster *raster;
    RwImage  *image;
    RwUInt32  width, height, stride;
    RwUInt8  *pixels;
    RwUInt32  x, y;

    RWFUNCTION(RWSTRING("_rpLtMapClearLightMap"));

    RWASSERT(NULL != lightMap);
    raster = RwTextureGetRaster(lightMap);
    RWASSERT(NULL != raster);

    width  = RwRasterGetWidth( raster);
    height = RwRasterGetHeight(raster);
    image = RwImageCreate(width, height, 32);
    RWASSERT(NULL != image);
    RwImageAllocatePixels(image);

    /* Clear all texels to an "unused" value */
/* TODO[6]: FILL WITH A CHECKERBOARD FOR DEBUG PURPOSES
 * - WE SHOULD CREATE AXIS-DEPENDENT CHECKERBOARDS, ALTERNATING BETWEEN "AXIS-COLOUR" AND BLACK.
 * - THIS WOULD REQUIRE FILLING IN TRIANGLES NOT ENTIRE RASTERS :O/
 * - IT'S OK, THERE'S CODE FOR IT IN RenderEngine (DEBUG-PREDICATED)
 * - REMEMBER THAT THE 'AXIS' VALUES ARE NOW ENCODED IN THE BOTTOM 3 BITS OF THE UVs */

    /* Users can override the colour (not alpha though) */
    if (NULL != color)
    {
        colors[0]       = *color;
        colors[0].alpha = 255;
        colors[1]       = *color;
        colors[1].alpha = 255;
    }

    stride = RwImageGetStride(image);
    pixels = RwImageGetPixels(image);
    RWASSERT(NULL != pixels);
    for (y = 0;y < height;++y)
    {
        RwRGBA *pixel = (RwRGBA *)&(pixels[y*stride]);
        for (x = (y&1);x < (width + (y&1));++x)
        {
           *pixel = colors[x&1];
            ++pixel;
        }
    }

    RwRasterSetFromImage(raster, image);
    RwImageDestroy(image);

    RWRETURN(lightMap);
}

rpLtMapSampleMap *
_rpLtMapSampleMapCreate( RwUInt32 w, RwUInt32 h )
{
    rpLtMapSampleMap      *sampleMap;
    RwUInt32            size;

    RWFUNCTION(RWSTRING("_rpLtMapSampleMapCreate"));

    RWASSERT(w > 0);
    RWASSERT(h > 0);

    size = sizeof(rpLtMapSampleMap);
    size += RPLTMAPSAMPLEMAPSIZE(w, h);

    sampleMap = (rpLtMapSampleMap *) RwMalloc(size,
        rwID_LTMAPPLUGIN | rwMEMHINTDUR_EVENT);
    if (NULL != sampleMap)
    {
        sampleMap->samplePixels = (RwUInt8 *)
            (((RwChar *) sampleMap) + sizeof(rpLtMapSampleMap));

        sampleMap->refCount = 1;

        memset(sampleMap->samplePixels, 0, (size - sizeof(rpLtMapSampleMap)));
    }

    RWRETURN(sampleMap);
}

extern RwBool
_rpLtMapSampleMapDestroy( rpLtMapSampleMap *sampleMap )
{
    RWFUNCTION(RWSTRING("_rpLtMapSampleMapDestroy"));

    RWASSERT(sampleMap);
    RWASSERT(sampleMap->refCount > 0);

    --sampleMap->refCount;
    if (sampleMap->refCount == 0)
    {
        RwFree(sampleMap);
    }

    RWRETURN(TRUE);
}

/*===========================================================================*
 *--- Plugin Functions ------------------------------------------------------*
 *===========================================================================*/

static void *
LtMapMaterialConstructor(void *object,
                         RwInt32 offset __RWUNUSED__,
                         RwInt32 size   __RWUNUSED__ )
{
    RwRGBA opaqueWhite = {255, 255, 255, 255};

    RWFUNCTION(RWSTRING("LtMapMaterialConstructor"));

    /* By default materials are lightmapped, don't emit light, do
     * cast shadows, aren't flat-shaded [and aren't translucent] */
    RPLTMAPMATERIALGETDATA(object)->flags   = 0;
    /* This multiplies the world's lightmap density */
    RPLTMAPMATERIALGETDATA(object)->lightMapDensity = 1.0f;
    /* Multiplies the default area light sample density
     * (twice the world's lightmap sample density or the
     *  value passed to RpLtMapWorldAreaLightsCreate) */
    RPLTMAPMATERIALGETDATA(object)->areaLightDensity = 1.0f;
    /* Makes light go further, multiplied by the value set by
     * RtLtMapSetAreaLightRadiusModifier. */
    RPLTMAPMATERIALGETDATA(object)->areaLightRadius = 1.0f;
    /* The colour can, effectively, be scaled by areaLightRadius */
    RPLTMAPMATERIALGETDATA(object)->areaLightColour = opaqueWhite;

    RWRETURN(object);
}

static void *
LtMapMaterialCopy(void *dstObject,
                  const void *srcObject __RWUNUSED__,
                  RwInt32 offset __RWUNUSED__,
                  RwInt32 size   __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("LtMapMaterialCopy"));

   *RPLTMAPMATERIALGETDATA(dstObject) = *RPLTMAPMATERIALGETCONSTDATA(srcObject);

    RWRETURN(dstObject);
}

/*
 * LtMapMaterialDestroy() is not needed.
 */


static void *
LtMapAtomicConstructor(void *object,
                       RwInt32 offset __RWUNUSED__,
                       RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("LtMapAtomicConstructor"));

    /* By default it may be lightmapped, will not be
     * vertex-lit and will cast shadows */
    RPLTMAPATOMICGETDATA(object)->flags = 0;
    RPLTMAPATOMICGETDATA(object)->numSamples = 0;
    /* This means it'll use the global default when a lightmap gets created for it */
    RPLTMAPATOMICGETDATA(object)->lightMapSize = 0;
    RPLTMAPATOMICGETDATA(object)->lightMap = (RwTexture *)NULL;
    RPLTMAPATOMICGETDATA(object)->sampleMap = NULL;
    RPLTMAPATOMICGETDATA(object)->clearCol = FALSE;

    RWRETURN(object);
}

static void *
LtMapAtomicDestructor(void *object,
                      RwInt32 offset __RWUNUSED__,
                      RwInt32 size   __RWUNUSED__ )
{
    LtMapObjectData *ltMapObject;

    RWFUNCTION(RWSTRING("LtMapAtomicDestructor"));

    ltMapObject =
        RPLTMAPATOMICGETDATA(object);

    if (NULL != ltMapObject->lightMap)
        RwTextureDestroy(ltMapObject->lightMap);

    if (NULL != ltMapObject->sampleMap)
        _rpLtMapSampleMapDestroy(ltMapObject->sampleMap);

    ltMapObject->lightMap = NULL;
    ltMapObject->sampleMap = NULL;

    RWRETURN(object);
}

static void *
LtMapAtomicCopy(void *dstObject,
                const void *srcObject __RWUNUSED__,
                RwInt32 offset __RWUNUSED__,
                RwInt32 size   __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("LtMapAtomicCopy"));

   *RPLTMAPATOMICGETDATA(dstObject) = *RPLTMAPATOMICGETCONSTDATA(srcObject);

    RWRETURN(dstObject);
}

/*
 * GEOMETRY EXTENSION: Constructor, Destructor and Copy functions
 */

static void *
LtMapGeometryConstructor(void *object,
                         RwInt32 offset __RWUNUSED__,
                         RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("LtMapGeometryConstructor"));
    
    /* By default it has not yet been light mapped */
    RPLTMAPGEOMETRYGETDATA(object)->PolySetArray = NULL;
    
    RWRETURN(object);
}

static void *
LtMapGeometryDestructor(void *object,
                        RwInt32 offset __RWUNUSED__,
                        RwInt32 size   __RWUNUSED__ )
{
    LtMapGeometryData *geometryData = RPLTMAPGEOMETRYGETDATA(object);

    RWFUNCTION(RWSTRING("LtMapGeometryDestructor"));

    if (geometryData->PolySetArray != NULL)
        RwFree (geometryData->PolySetArray);

    RWRETURN(object);
}

static void *
LtMapGeometryCopy(void *dstObject,
                  const void *srcObject __RWUNUSED__,
                  RwInt32 offset __RWUNUSED__,
                  RwInt32 size   __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("LtMapGeometryCopy"));
    
    *RPLTMAPGEOMETRYGETDATA(dstObject) = *RPLTMAPGEOMETRYGETCONSTDATA(srcObject);
    
    RWRETURN(dstObject);
}

static void *
LtMapWorldSectorConstructor(void *object,
                            RwInt32 offset __RWUNUSED__,
                            RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("LtMapWorldSectorConstructor"));

    /* By default it may be lightmapped, will not be
     * vertex-lit and will cast shadows */
    RPLTMAPWORLDSECTORGETDATA(object)->flags = 0;
    RPLTMAPWORLDSECTORGETDATA(object)->numSamples   = 0;
    /* This means it'll use the global default when a lightmap gets created for it */
    RPLTMAPWORLDSECTORGETDATA(object)->lightMapSize = 0;
    RPLTMAPWORLDSECTORGETDATA(object)->lightMap = (RwTexture *)NULL;
    RPLTMAPWORLDSECTORGETDATA(object)->sampleMap = NULL;
    RPLTMAPWORLDSECTORGETDATA(object)->clearCol = FALSE;

    RWRETURN(object);
}

static void *
LtMapWorldSectorDestructor(void *object,
                           RwInt32 offset __RWUNUSED__,
                           RwInt32 size   __RWUNUSED__ )
{
    LtMapObjectData *ltMapObject;

    RWFUNCTION(RWSTRING("LtMapWorldSectorDestructor"));

    ltMapObject =
        RPLTMAPWORLDSECTORGETDATA(object);

    if (NULL != ltMapObject->lightMap)
        RwTextureDestroy(ltMapObject->lightMap);

    if (NULL != ltMapObject->sampleMap)
        _rpLtMapSampleMapDestroy(ltMapObject->sampleMap);

    ltMapObject->lightMap = NULL;
    ltMapObject->sampleMap = NULL;

    RWRETURN(object);
}

static void *
LtMapWorldSectorCopy(void *dstObject,
                     const void *srcObject __RWUNUSED__,
                     RwInt32 offset __RWUNUSED__,
                     RwInt32 size   __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("LtMapWorldSectorCopy"));

    *RPLTMAPWORLDSECTORGETDATA(dstObject) =
       *RPLTMAPWORLDSECTORGETCONSTDATA(srcObject);

    RWRETURN(dstObject);
}

static void *
LtMapWorldConstructor(void *object,
                      RwInt32 offset __RWUNUSED__,
                      RwInt32 size   __RWUNUSED__ )
{
    LtMapWorldData *data;

    RWFUNCTION(RWSTRING("LtMapWorldConstructor"));

    data = (LtMapWorldData *)RPLTMAPWORLDGETDATA(object);

    /* This results in a value being automatically calculated,
     * from the world's bounding box, at lightmap-creation-time */
    data->lightMapDensity = rpLTMAPDEFAULTLIGHTMAPDENSITY;
    data->flag = 0;

    RWRETURN(object);
}

/* No need for a world destructor */

static void *
LtMapWorldCopy(void *dstObject __RWUNUSED__,
               const void *srcObject __RWUNUSED__,
               RwInt32 offset __RWUNUSED__,
               RwInt32 size __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("LtMapWorldCopy"));
    /* Nahh, not gonna support this */
    RWRETURN(NULL);
}

/*===========================================================================*
 *--- Plugin Stream Functions -----------------------------------------------*
 *===========================================================================*/

static LtMapObjectData *
LtMapObjectLightMapRead(LtMapObjectData *objectData, RwChar *name)
{
    RWFUNCTION(RWSTRING("LtMapObjectLightMapRead"));

    RWASSERT(NULL != objectData);

    if (rwstrcmp(name, ""))
    {
        RwUInt32   format = RpLtMapGetRasterFormat();
        RwRaster  *oldRaster;
        RwTexDictionary *current;
        RwTexture *texture = (RwTexture*)NULL;

        current = RwTexDictionaryGetCurrent();

        if( NULL != current )
        {
            texture = RwTexDictionaryFindNamedTexture(current, name);
        }

        if (NULL == texture)
        {
            texture = RwTextureRead(name, NULL);
            if (NULL == texture)
            {
                RWASSERT(0 < objectData->lightMapSize);
                if (0 >= objectData->lightMapSize)
                {
                    objectData->lightMapSize = _rpLtMapGlobals.lightMapSize;
                }

                /* Create a blank lightmap (it'll be created with a reference
                 * count of one, which is perfect as only this sector owns it) */
                texture = RwTextureCreate(RwRasterCreate(
                    objectData->lightMapSize,
                    objectData->lightMapSize, 0, format));
                RWASSERT(NULL != texture);
                RwTextureSetName(texture, name);
                if( RwTexDictionaryGetCurrent() )
                {        
                    RwTexDictionaryAddTexture(RwTexDictionaryGetCurrent(),
                                            texture);
                }
                /* Clear it to the checkerboard uninitialized' form */
                _rpLtMapClearLightMap(texture, (RwRGBA *)NULL);
            }
        }

        /* The sector "owns" the lightmap (along with any other objects
         * sharing it), so add an extra reference count (the initial
         * one can be for the texDictionary containing the lightmap) */
        RwTextureAddRef(texture);

        /* We convert the textures raster to the appropriate format.
         * If it came from a texture dictionary then it should be in
         * the right format already. If it came from an RwImage then
         * the format will be wrong, so we convert. */
        oldRaster = RwTextureGetRaster(texture);

#ifdef RWDEBUG
        if ((RwRasterGetFormat(oldRaster)|rwRASTERTYPETEXTURE)
            != (RwInt32)format)
        {
            RwDebugSendMessage(rwDEBUGMESSAGE, "LtMapObjectLightMapRead",
                "Warning: lightmap format is not "
                FORMAT_MSG
                ". Using non default texture format might cause visual"
                " glitches and/or performance problems");

            /*
             * convertion removed
             */
            /*
            RwUInt32  height = RwRasterGetHeight(oldRaster);
            RwUInt32  width = RwRasterGetWidth(oldRaster);
            RwRaster *newRaster;
            RwImage *image;

            newRaster = RwRasterCreate(width, height, 0, format);
            RWASSERT(NULL != newRaster);

            image = RwImageCreate(width, height, 32);
            RWASSERT(NULL != image);
            image = RwImageAllocatePixels(image);
            RWASSERT(NULL != image);

            RwImageSetFromRaster(image, oldRaster);
            RwRasterSetFromImage(newRaster, image);

            RwImageDestroy(image);

            RwTextureSetRaster(texture, newRaster);
            RwRasterDestroy(oldRaster);
             */
        }
#endif
        objectData->lightMap = texture;
    }
    else
    {
        RWASSERT(0 == objectData->lightMapSize);
        objectData->lightMap = (RwTexture *)NULL;
    }

    RWRETURN(objectData);
}


static RwStream *
LtMapMaterialRead(RwStream *stream,
                  RwInt32 binaryLength __RWUNUSEDRELEASE__,
                  void *object,
                  RwInt32 offsetInObject __RWUNUSED__,
                  RwInt32 sizeInObject __RWUNUSED__)
{
    LtMapMaterialStreamData streamData;
    LtMapMaterialData *materialData;
    RpMaterial *material;
    RwUInt32 subChunkSize, version;

    RWFUNCTION(RWSTRING("LtMapMaterialRead"));
    RWASSERT(NULL != stream);
    RWASSERT(NULL != object);

    material = (RpMaterial *)object;
    RWASSERT(NULL != material);

    materialData = RPLTMAPMATERIALGETDATA(material);

    /* First read our internal header to get version number (a current
     * oversight in the stream API is that we don't get the version
     * number passed into this readCB for its own chunk header!) */
    if (!RwStreamFindChunk(stream, rwID_LTMAPPLUGIN, &subChunkSize, &version))
    {
        RwDebugSendMessage(rwDEBUGERROR, "LtMapMaterialRead",
            "Could not read stream data header");
        RWRETURN((RwStream *)NULL);
    }

    /* Once there are multiple plugin data file versions, test 'version' here */
    {
        RWASSERT(binaryLength ==
            (rwCHUNKHEADERSIZE +
             sizeof(RwUInt32) + /* flags */
             sizeof(RwReal)   + /* lightMapDensity */
             sizeof(RwRGBA)   + /* areaLightColour */
             sizeof(RwReal)   + /* areaLightDensity */
             sizeof(RwReal)     /* areaLightRadius */ ));
        RWASSERT(subChunkSize == (binaryLength - rwCHUNKHEADERSIZE));

        /* Read the material plugin data */
        if (RwStreamRead(stream, &streamData, sizeof(streamData))
                != sizeof(streamData))
        {
            RwDebugSendMessage(rwDEBUGERROR, "LtMapMaterialRead", "Could not read stream data");
            RWRETURN((RwStream *)NULL);
        }

        /* Convert it */
        (void)RwMemNative32(&streamData.flags, sizeof(RwUInt32));
        (void)RwMemNative32(&streamData.lightMapDensity, sizeof(RwReal));
        (void)RwMemFloat32ToReal(&streamData.lightMapDensity, sizeof(RwReal));
        /* NOTE: the colour isn't endianness converted (it's byte data) */
        (void)RwMemNative32(&streamData.areaLightDensity, sizeof(RwReal));
        (void)RwMemFloat32ToReal(&streamData.areaLightDensity, sizeof(RwReal));
        (void)RwMemNative32(&streamData.areaLightRadius, sizeof(RwReal));
        (void)RwMemFloat32ToReal(&streamData.areaLightRadius, sizeof(RwReal));
        /* So we catch when this needs updating: */
        RWASSERT(20 == sizeof(LtMapMaterialStreamData));

        materialData->flags            = streamData.flags;
        materialData->lightMapDensity  = streamData.lightMapDensity;
        materialData->areaLightColour  = streamData.areaLightColour;
        materialData->areaLightDensity = streamData.areaLightDensity;
        materialData->areaLightRadius  = streamData.areaLightRadius;
    }
    /*else
    {
        RwDebugSendMessage(rwDEBUGERROR, "LtMapMaterialRead", "Invalid stream data version");
        RWRETURN((RwStream *)NULL);
    }*/

    RWRETURN(stream);
}

static RwStream *
LtMapMaterialWrite(RwStream *stream,
                   RwInt32 binaryLength   __RWUNUSED__,
                   const void *object     __RWUNUSED__,
                   RwInt32 offsetInObject __RWUNUSED__,
                   RwInt32 sizeInObject   __RWUNUSED__ )
{
    LtMapMaterialStreamData streamData;
    const LtMapMaterialData *materialData;
    const RpMaterial *material;

    RWFUNCTION(RWSTRING("LtMapMaterialWrite"));

    material = (const RpMaterial *)object;
    RWASSERT(NULL != material);
    materialData = RPLTMAPMATERIALGETCONSTDATA(material);

    /* First write our internal header to store version number (a current
     * oversight in the stream API is that we don't get the version
     * number passed into this readCB for its own chunk header!) */
    if (!RwStreamWriteChunkHeader(stream, rwID_LTMAPPLUGIN,
                LTMAPMATERIALSTREAMDATASIZE - rwCHUNKHEADERSIZE))
    {
        RwDebugSendMessage(rwDEBUGERROR, "LtMapMaterialWrite",
            "Could not write stream data header");
        RWRETURN((RwStream *)NULL);
    }

    /* Fill and write out the stream structure */
    streamData.flags            = materialData->flags;
    streamData.lightMapDensity  = materialData->lightMapDensity;
    streamData.areaLightColour  = materialData->areaLightColour;
    streamData.areaLightDensity = materialData->areaLightDensity;
    streamData.areaLightRadius  = materialData->areaLightRadius;

    (void)RwMemLittleEndian32(&streamData.flags, sizeof(RwUInt32));
    (void)RwMemRealToFloat32(&streamData.lightMapDensity, sizeof(RwReal));
    (void)RwMemLittleEndian32(&streamData.lightMapDensity, sizeof(RwReal));
    /* NOTE: the colour isn't endianness converted (it's byte data) */
    (void)RwMemRealToFloat32(&streamData.areaLightDensity, sizeof(RwReal));
    (void)RwMemLittleEndian32(&streamData.areaLightDensity, sizeof(RwReal));
    (void)RwMemRealToFloat32(&streamData.areaLightRadius, sizeof(RwReal));
    (void)RwMemLittleEndian32(&streamData.areaLightRadius, sizeof(RwReal));
    /* So we catch when this needs updating */
    RWASSERT(20 == sizeof(LtMapMaterialStreamData));

    if (!RwStreamWrite(stream, &streamData, sizeof(streamData)))
    {
        RwDebugSendMessage(rwDEBUGERROR, "LtMapMaterialWrite",
            "Could not write stream plugin data");
        RWRETURN((RwStream *)NULL);
    }

    RWRETURN(stream);
}

static RwInt32
LtMapMaterialGetSize(const void *object     __RWUNUSED__,
                   RwInt32 offsetInObject __RWUNUSED__,
                   RwInt32 sizeInObject   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("LtMapMaterialGetSize"));

    if (RPLTMAPMATERIALGETCONSTDATA((const RpMaterial *)object)->flags)
    {
        RWRETURN((RwInt32)LTMAPMATERIALSTREAMDATASIZE);
    }

    /* Don't bother streaming data if the object is in the default state */
    RWRETURN(0);
}


static RwStream *
LtMapAtomicRead(RwStream *stream,
                RwInt32 binaryLength __RWUNUSEDRELEASE__,
                void *object,
                RwInt32 offsetInObject __RWUNUSED__,
                RwInt32 sizeInObject __RWUNUSED__)
{
    LtMapObjectStreamData streamData;
    LtMapObjectData *atomicData;
    RpAtomic *atomic;
    RwUInt32  subChunkSize, version;

    RWFUNCTION(RWSTRING("LtMapAtomicRead"));
    RWASSERT(NULL != stream);
    RWASSERT(NULL != object);

    atomic = (RpAtomic *)object;
    RWASSERT(NULL != atomic);

    atomicData = RPLTMAPATOMICGETDATA(atomic);

    /* First read our internal header to get version number (a current
     * oversight in the stream API is that we don't get the version
     * number passed into this readCB for its own chunk header!) */
    if (!RwStreamFindChunk(stream, rwID_LTMAPPLUGIN, &subChunkSize, &version))
    {
        RwDebugSendMessage(rwDEBUGERROR, "LtMapAtomicRead",
            "Could not read stream data header");
        RWRETURN((RwStream *)NULL);
    }

    /* Once there are multiple plugin data file versions, test 'version' here */
    {
        RWASSERT(binaryLength ==
            (rwCHUNKHEADERSIZE +
             sizeof(RwUInt32) + /* flags */
             sizeof(RwUInt32) + /* numSamples */
             sizeof(RwUInt32) + /* lightMapSize */
             (sizeof(RwChar)*rwTEXTUREBASENAMELENGTH) /*lightMapName */));
        RWASSERT(subChunkSize == (binaryLength - rwCHUNKHEADERSIZE));

        /* Read the atomic plugin data */
        if (RwStreamRead(stream, &streamData, sizeof(streamData))
                != sizeof(streamData))
        {
            RwDebugSendMessage(rwDEBUGERROR, "LtMapAtomicRead", "Could not read stream data");
            RWRETURN((RwStream *)NULL);
        }

        /* Convert the stream structure */
        (void)RwMemNative32(&streamData.flags, sizeof(RwUInt32));
        (void)RwMemNative32(&streamData.numSamples, sizeof(RwUInt32));
        /* NOTE: the lightmap name string isn't endianness converted (it's byte data) */
        (void)RwMemNative32(&streamData.lightMapSize, sizeof(RwUInt32));
        /* So we catch when this needs updating: */
        RWASSERT(12 + rwTEXTUREBASENAMELENGTH*sizeof(RwChar) ==
                 sizeof(LtMapObjectStreamData));

        /* Store the streamed values in plugin data */
        atomicData->flags        = streamData.flags;
        atomicData->numSamples   = streamData.numSamples;
        atomicData->lightMapSize = streamData.lightMapSize;
        /* If the atomic has a lightmap, load it in */
        LtMapObjectLightMapRead(atomicData, streamData.lightMapName);
        if (NULL != atomicData->lightMap)
        {
            /* Set the appropriate rendering pipeline */
            RpAtomicSetPipeline(atomic,
                RpLtMapGetPlatformAtomicPipeline());
        }
    }
    /*else
    {
        RwDebugSendMessage(rwDEBUGERROR, "LtMapAtomicRead", "Invalid stream data version");
        RWRETURN((RwStream *)NULL);
    }*/

#if (defined(SKY2_DRVMODEL_H) || defined(NULLSKY_DRVMODEL_H))
/* TODO[6][ACL]: ADD A CHECK IN HERE TO VERIFY THE BASE TEXTURE, ASSERT IF INVALID
 *               CHECK IT HAS AN ALPHA CHANNEL AND CHECK RANDOM PIXELS FOR CORRECT
 *               LUMINANCE THEREIN (GIVEN THEY'RE RASTERS DONT FORGET TO USE RwRGBAFromPixel() ) */
#endif /* (defined(SKY2_DRVMODEL_H) || defined(NULLSKY_DRVMODEL_H)) */

    RWRETURN(stream);
}

static RwStream *
LtMapAtomicWrite(RwStream *stream,
                 RwInt32 binaryLength   __RWUNUSED__,
                 const void *object     __RWUNUSED__,
                 RwInt32 offsetInObject __RWUNUSED__,
                 RwInt32 sizeInObject   __RWUNUSED__ )
{
    LtMapObjectStreamData streamData;
    const LtMapObjectData *atomicData;
    const RpAtomic *atomic;

    RWFUNCTION(RWSTRING("LtMapAtomicWrite"));

    atomic = (const RpAtomic *)object;
    RWASSERT(NULL != atomic);

    atomicData = RPLTMAPATOMICGETCONSTDATA(atomic);

    /* First write our internal header to store version number (a current
     * oversight in the stream API is that we don't get the version
     * number passed into this readCB for its own chunk header!) */
    if (!RwStreamWriteChunkHeader(stream, rwID_LTMAPPLUGIN,
             LTMAPOBJECTSTREAMDATASIZE - rwCHUNKHEADERSIZE))
    {
        RwDebugSendMessage(rwDEBUGERROR, "LtMapAtomicWrite",
            "Could not write stream data header");
        RWRETURN((RwStream *)NULL);
    }

    /* Fill the stream structure */
    streamData.flags = atomicData->flags;
    /* NOTE: numSamples could be non-zero even if the atomic has
     *       no lightmap, given that we can do vertex lighting. */
    streamData.numSamples = atomicData->numSamples;
    if (NULL != atomicData->lightMap)
    {
        RwTexture *lightMap = atomicData->lightMap;

        rwsprintf(streamData.lightMapName, RwTextureGetName(lightMap));
        streamData.lightMapSize = atomicData->lightMapSize;
        RWASSERT((atomicData->lightMapSize == (RwUInt32)
                  RwRasterGetWidth( RwTextureGetRaster(lightMap))) &&
                 (atomicData->lightMapSize == (RwUInt32)
                  RwRasterGetHeight(RwTextureGetRaster(lightMap))) );
    }
    else
    {
        rwsprintf(streamData.lightMapName, "");
        streamData.lightMapSize = 0;
    }

    /* Convert and write out the stream structure */
    (void)RwMemLittleEndian32(&streamData.flags, sizeof(RwUInt32));
    (void)RwMemLittleEndian32(&streamData.numSamples, sizeof(RwUInt32));
    /* NOTE: the lightmap name string isn't endianness converted (it's byte data) */
    (void)RwMemLittleEndian32(&streamData.lightMapSize, sizeof(RwUInt32));
    /* So we catch when this needs updating: */
    RWASSERT(12 + rwTEXTUREBASENAMELENGTH*sizeof(RwChar) ==
             sizeof(LtMapObjectStreamData));
    if (!RwStreamWrite(stream, &streamData, sizeof(streamData)))
    {
        RwDebugSendMessage(rwDEBUGERROR, "LtMapAtomicWrite",
            "Could not write stream plugin data");
        RWRETURN((RwStream *)NULL);
    }

    RWRETURN(stream);
}

static RwInt32
LtMapAtomicGetSize(const void *object     __RWUNUSED__,
                   RwInt32 offsetInObject __RWUNUSED__,
                   RwInt32 sizeInObject   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("LtMapAtomicGetSize"));

    if (RPLTMAPATOMICGETCONSTDATA((const RpAtomic *)object)->flags)
    {
        RWRETURN((RwInt32)LTMAPOBJECTSTREAMDATASIZE);
    }

    /* Don't bother streaming data if the object is in the default state */
    RWRETURN(0);
}


static RwStream *
LtMapWorldSectorRead(RwStream *stream,
                     RwInt32 binaryLength __RWUNUSEDRELEASE__,
                     void *object,
                     RwInt32 offsetInObject __RWUNUSED__,
                     RwInt32 sizeInObject __RWUNUSED__)
{
    LtMapObjectStreamData streamData;
    LtMapObjectData *sectorData;
    RpWorldSector   *sector;
    RwUInt32         subChunkSize, version;

    RWFUNCTION(RWSTRING("LtMapWorldSectorRead"));
    RWASSERT(NULL != stream);
    RWASSERT(NULL != object);

    sector = (RpWorldSector *)object;
    RWASSERT(NULL != sector);

    sectorData = RPLTMAPWORLDSECTORGETDATA(sector);

    /* First read our internal header to get version number (a current
     * oversight in the stream API is that we don't get the version
     * number passed into this readCB for its own chunk header!) */
    if (!RwStreamFindChunk(stream, rwID_LTMAPPLUGIN, &subChunkSize, &version))
    {
        RwDebugSendMessage(rwDEBUGERROR, "LtMapWorldSectorRead",
            "Could not read stream data header");
        RWRETURN((RwStream *)NULL);
    }

    /* Once there are multiple plugin data file versions, test 'version' here */
    {
        RWASSERT(binaryLength ==
            (rwCHUNKHEADERSIZE +
             sizeof(RwUInt32) + /* flags */
             sizeof(RwUInt32) + /* numSamples */
             sizeof(RwUInt32) + /* lightMapSize */
             (sizeof(RwChar)*rwTEXTUREBASENAMELENGTH) /*lightMapName */));
        RWASSERT(subChunkSize == (binaryLength - rwCHUNKHEADERSIZE));

        /* Read the sector plugin data */
        if (RwStreamRead(stream, &streamData, sizeof(streamData))
                != sizeof(streamData))
        {
            RwDebugSendMessage(rwDEBUGERROR, "LtMapWorldSectorRead",
                "Could not read stream data");
            RWRETURN((RwStream *)NULL);
        }

        /* Convert the stream structure */
        (void)RwMemNative32(&streamData.flags, sizeof(RwUInt32));
        (void)RwMemNative32(&streamData.numSamples, sizeof(RwUInt32));
        /* NOTE: the lightmap name string isn't endianness converted (it's byte data) */
        (void)RwMemNative32(&streamData.lightMapSize, sizeof(RwUInt32));
        /* So we catch when this needs updating: */
        RWASSERT(12 + rwTEXTUREBASENAMELENGTH*sizeof(RwChar) ==
                 sizeof(LtMapObjectStreamData));

        /* Store the streamed values in plugin data */
        sectorData->flags        = streamData.flags;
        sectorData->numSamples   = streamData.numSamples;
        sectorData->lightMapSize = streamData.lightMapSize;
        /* If the sector has a lightmap, load it in */
        LtMapObjectLightMapRead(sectorData, streamData.lightMapName);
        if (NULL != sectorData->lightMap)
        {
            /* Set the appropriate rendering pipeline */
            RpWorldSectorSetPipeline(sector,
                RpLtMapGetPlatformWorldSectorPipeline());
        }
    }
    /*else
    {
        RwDebugSendMessage(rwDEBUGERROR, "LtMapWorldSectorRead", "Invalid stream data version");
        RWRETURN((RwStream *)NULL);
    }*/

#if (defined(SKY2_DRVMODEL_H) || defined(NULLSKY_DRVMODEL_H))
/* TODO[6][ACL]: ADD A CHECK IN HERE TO VERIFY THE BASE TEXTURE, ASSERT IF INVALID
 *               CHECK IT HAS AN ALPHA CHANNEL AND CHECK RANDOM PIXELS FOR CORRECT
 *               LUMINANCE THEREIN (GIVEN THEY'RE RASTERS DONT FORGET TO USE RwRGBAFromPixel() ) */
#endif /* (defined(SKY2_DRVMODEL_H) || defined(NULLSKY_DRVMODEL_H)) */

    RWRETURN(stream);
}

static RwStream *
LtMapWorldSectorWrite(RwStream *stream,
                      RwInt32 binaryLength   __RWUNUSED__,
                      const void *object     __RWUNUSED__,
                      RwInt32 offsetInObject __RWUNUSED__,
                      RwInt32 sizeInObject   __RWUNUSED__ )
{
    LtMapObjectStreamData streamData;
    const LtMapObjectData *sectorData;
    const RpWorldSector *sector;

    RWFUNCTION(RWSTRING("LtMapWorldSectorWrite"));

    sector = (const RpWorldSector *)object;

    sectorData = RPLTMAPWORLDSECTORGETCONSTDATA(sector);

    /* First write our internal header to store version number (a current
     * oversight in the stream API is that we don't get the version
     * number passed into this readCB for its own chunk header!) */
    if (!RwStreamWriteChunkHeader(stream, rwID_LTMAPPLUGIN,
                LTMAPOBJECTSTREAMDATASIZE - rwCHUNKHEADERSIZE))
    {
        RwDebugSendMessage(rwDEBUGERROR, "LtMapWorldSectorWrite",
            "Could not write stream data header");
        RWRETURN((RwStream *)NULL);
    }

    /* Fill the stream structure */
    streamData.flags = sectorData->flags;
    /* NOTE: numSamples could be non-zero even if the sector has
     *       no lightmap, given that we can do vertex lighting. */
    streamData.numSamples = sectorData->numSamples;
    if (NULL != sectorData->lightMap)
    {
        RwTexture *lightMap = sectorData->lightMap;

        rwsprintf(streamData.lightMapName, RwTextureGetName(lightMap));
        streamData.lightMapSize = sectorData->lightMapSize;
        RWASSERT((sectorData->lightMapSize == (RwUInt32)
                  RwRasterGetWidth( RwTextureGetRaster(lightMap))) &&
                 (sectorData->lightMapSize == (RwUInt32)
                  RwRasterGetHeight(RwTextureGetRaster(lightMap))) );
    }
    else
    {
        rwsprintf(streamData.lightMapName, "");
        streamData.lightMapSize = 0;
    }

    /* Convert and write out the stream structure */
    (void)RwMemLittleEndian32(&streamData.flags, sizeof(RwUInt32));
    (void)RwMemLittleEndian32(&streamData.numSamples, sizeof(RwUInt32));
    /* NOTE: the lightmap name string isn't endianness converted (it's byte data) */
    (void)RwMemLittleEndian32(&streamData.lightMapSize, sizeof(RwUInt32));
    /* So we catch when this needs updating: */
    RWASSERT(12 + rwTEXTUREBASENAMELENGTH*sizeof(RwChar) ==
             sizeof(LtMapObjectStreamData));
    if (!RwStreamWrite(stream, &streamData, sizeof(streamData)))
    {
        RwDebugSendMessage(rwDEBUGERROR, "LtMapWorldSectorWrite",
            "Could not write stream plugin data");
        RWRETURN((RwStream *)NULL);
    }

    RWRETURN(stream);
}

static RwInt32
LtMapWorldSectorGetSize(const void *object     __RWUNUSED__,
                        RwInt32 offsetInObject __RWUNUSED__,
                        RwInt32 sizeInObject   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("LtMapWorldSectorGetSize"));

    if (RPLTMAPWORLDSECTORGETCONSTDATA((const RpWorldSector *)object)->flags)
    {
        RWRETURN((RwInt32)LTMAPOBJECTSTREAMDATASIZE);
    }

    /* Don't bother streaming data if the object is in the default state */
    RWRETURN(0);
}


static RwStream *
LtMapWorldRead(RwStream *stream,
               RwInt32 binaryLength __RWUNUSEDRELEASE__,
               void *object,
               RwInt32 offsetInObject __RWUNUSED__,
               RwInt32 sizeInObject __RWUNUSED__)
{
    LtMapWorldStreamData streamData;
    LtMapWorldData *worldData;
    RpWorld *world;
    RwUInt32  subChunkSize, version;

    RWFUNCTION(RWSTRING("LtMapWorldRead"));
    RWASSERT(NULL != stream);
    RWASSERT(NULL != object);

    world = (RpWorld *)object;
    RWASSERT(NULL != world);

    worldData = RPLTMAPWORLDGETDATA(world);

    /* First read our internal header to get version number (a current
     * oversight in the stream API is that we don't get the version
     * number passed into this readCB for its own chunk header!) */
    if (!RwStreamFindChunk(stream, rwID_LTMAPPLUGIN, &subChunkSize, &version))
    {
        RwDebugSendMessage(rwDEBUGERROR, "LtMapWorldRead",
            "Could not read stream data header");
        RWRETURN((RwStream *)NULL);
    }

    /* Once there are multiple plugin data file versions, test 'version' here */
    {
        RWASSERT(binaryLength ==
            (rwCHUNKHEADERSIZE +
             sizeof(streamData)));
        RWASSERT(subChunkSize == (binaryLength - rwCHUNKHEADERSIZE));

        /* Read the world plugin data */
        if (RwStreamRead(stream, &streamData, sizeof(streamData))
                != sizeof(streamData))
        {
            RwDebugSendMessage(rwDEBUGERROR, "LtMapWorldRead",
                "Could not read stream data");
            RWRETURN((RwStream *)NULL);
        }

        /* Convert it */
        (void)RwMemNative32(&streamData.lightMapDensity, sizeof(RwReal));
        (void)RwMemFloat32ToReal(&streamData.lightMapDensity, sizeof(RwReal));

        /* Fill in the plugin data */
        worldData->lightMapDensity = streamData.lightMapDensity;
    }

    /*else
    {
        RwDebugSendMessage(rwDEBUGERROR, "LtMapWorldRead", "Invalid stream data version");
        RWRETURN((RwStream *)NULL);
    }*/

    /* Flag the world as lightmapped */
    worldData->flag |= rpLTMAPWORLDFLAGLIGHTMAP;

    RWRETURN(stream);
}

static RwStream *
LtMapWorldWrite(RwStream *stream,
                RwInt32 binaryLength   __RWUNUSEDRELEASE__,
                const void *object     __RWUNUSED__,
                RwInt32 offsetInObject __RWUNUSED__,
                RwInt32 sizeInObject   __RWUNUSED__ )
{
    LtMapWorldStreamData streamData;
    const LtMapWorldData *worldData;
    const RpWorld *world;

    RWFUNCTION(RWSTRING("LtMapWorldWrite"));

    RWASSERT(LTMAPWORLDSTREAMDATASIZE == binaryLength);
    RWASSERT(NULL != stream);
    RWASSERT(NULL != object);

    world = (const RpWorld *)object;
    RWASSERT(NULL != world);

    worldData = RPLTMAPWORLDGETCONSTDATA(world);

    /* First write our internal header to store version number (a current
     * oversight in the stream API is that we don't get the version
     * number passed into this readCB for its own chunk header!) */
    if (!RwStreamWriteChunkHeader(stream, rwID_LTMAPPLUGIN,
                LTMAPWORLDSTREAMDATASIZE - rwCHUNKHEADERSIZE))
    {
        RwDebugSendMessage(rwDEBUGERROR, "LtMapWorldWrite",
            "Could not write stream data header");
        RWRETURN((RwStream *)NULL);
    }

    /* Now write out the world plugin data */
    streamData.lightMapDensity = worldData->lightMapDensity;

    (void)RwMemRealToFloat32(&streamData.lightMapDensity, sizeof(RwReal));
    (void)RwMemLittleEndian32(&streamData.lightMapDensity, sizeof(RwReal));

    if (!RwStreamWrite(stream, &streamData, sizeof(streamData)))
    {
        RwDebugSendMessage(rwDEBUGERROR, "LtMapWorldWrite", "Could not write stream plugin data");
        RWRETURN((RwStream *)NULL);
    }

    RWRETURN(stream);
}

static RwInt32
LtMapWorldGetSize(const void *object     __RWUNUSED__,
                  RwInt32 offsetInObject __RWUNUSED__,
                  RwInt32 sizeInObject   __RWUNUSED__ )
{
    const LtMapWorldData *worldData;

    RWFUNCTION(RWSTRING("LtMapWorldGetSize"));

    worldData = RPLTMAPWORLDGETCONSTDATA((const RpWorld *)object);

    if (worldData->flag & rpLTMAPWORLDFLAGLIGHTMAP)
    {
        RWRETURN((RwInt32)LTMAPWORLDSTREAMDATASIZE);
    }

    /* Don't bother streaming data if the object is in the default state */
    RWRETURN(0);
}

/*===========================================================================*
 *--- Plugin Internal Functions ---------------------------------------------*
 *===========================================================================*/

/****************************************************************************
 AtomicLightMapListCB
 */
static RpAtomic *
AtomicLightMapListCB(RpAtomic *atomic, void *data)
{
    RwSList *lightMapList = (RwSList *)data;
    RwTexture *lightMap;
    RwUInt32 i;

    RWFUNCTION(RWSTRING("AtomicLightMapListCB"));

    lightMap = RpLtMapAtomicGetLightMap(atomic);
    if (NULL != lightMap)
    {
        for (i = 0;i < (RwUInt32)rwSListGetNumEntries(lightMapList);++i)
        {
            if (lightMap == *(RwTexture **)rwSListGetEntry(lightMapList, i))
            {
                lightMap = (RwTexture *)NULL;
                break;
            }
        }
        if (lightMap != NULL)
        {
            *(RwTexture **)rwSListGetNewEntry(lightMapList,
                rwID_LTMAPPLUGIN | rwMEMHINTDUR_EVENT) = lightMap;
        }
    }

    RWRETURN(atomic);
}

/****************************************************************************
 SectorLightMapListCB
 */
static RpWorldSector *
SectorLightMapListCB(RpWorldSector *sector, void *data)
{
    RwSList *lightMapList = (RwSList *)data;
    RwTexture *lightMap;
    RwUInt32 i;

    RWFUNCTION(RWSTRING("SectorLightMapListCB"));

    lightMap = RpLtMapWorldSectorGetLightMap(sector);
    if (NULL != lightMap)
    {
        for (i = 0;i < (RwUInt32)rwSListGetNumEntries(lightMapList);++i)
        {
            if (lightMap == *(RwTexture **)rwSListGetEntry(lightMapList, i))
            {
                lightMap = (RwTexture *)NULL;
                break;
            }
        }
        if (lightMap != NULL)
        {
            *(RwTexture **)rwSListGetNewEntry(lightMapList,
                rwID_LTMAPPLUGIN | rwMEMHINTDUR_EVENT) = lightMap;
        }
    }

    RpWorldSectorForAllAtomics(sector, AtomicLightMapListCB, data);

    RWRETURN(sector);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

static void *
LtMapClose(void *instance,
              RwInt32 offset __RWUNUSED__,
              RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("LtMapClose"));
    RWASSERT(NULL != instance);
    RWASSERT(0 < _rpLtMapGlobals.module.numInstances);

    /* One less module instance */
    _rpLtMapGlobals.module.numInstances--;

    if(0 == _rpLtMapGlobals.module.numInstances)
    {
#if (defined(SKY2_DRVMODEL_H) || defined(NULLSKY_DRVMODEL_H))
        /* Destroy the PS2-specific white texture */
        RwTextureDestroy(_rpLtMapGlobals.whiteTex);
        _rpLtMapGlobals.whiteTex = (RwTexture *)NULL;
#endif /* (defined(SKY2_DRVMODEL_H) || defined(NULLSKY_DRVMODEL_H)) */

        /* Destroy the lightmap pipelines */
        _rpLtMapPlatformPipelinesDestroy();
        _rpLtMapGenericPipelinesDestroy();
    }

    RWRETURN(instance);
}

static void *
LtMapOpen(void *instance,
             RwInt32 offset __RWUNUSED__,
             RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("LtMapOpen"));
    RWASSERT(NULL != instance);

    /* Create the lightmap pipelines, only once */
    if(_rpLtMapGlobals.module.numInstances == 0)
    {
        RwBool success;

        /* Set up the lightmap pipelines */
        success = _rpLtMapGenericPipelinesCreate();
        if (FALSE == success)
        {
            RwDebugSendMessage(rwDEBUGERROR, "LtMapOpen",
                "Could not initialize generic pipelines");
            RWRETURN(NULL);
        }

        success = _rpLtMapPlatformPipelinesCreate();
        RWASSERT(FALSE != success);
        if (FALSE == success)
        {
            RwDebugSendMessage(rwDEBUGERROR, "LtMapOpen",
                "Could not initialize platform-specific pipelines");
            _rpLtMapGenericPipelinesDestroy();
            RWRETURN(NULL);
        }
    }

#if (defined(SKY2_DRVMODEL_H) || defined(NULLSKY_DRVMODEL_H))
    /* Set up the white texture used on PS2 to allow the "renderstyles" */
    _rpLtMapGlobals.whiteTex = LtMapInitializeWhiteTex();
    if (NULL == _rpLtMapGlobals.whiteTex)
    {
        RwDebugSendMessage(rwDEBUGERROR, "LtMapOpen",
            "Could not initialize PS2-specific white texture");
        _rpLtMapGenericPipelinesDestroy();
        RWRETURN(NULL);
    }
#endif /* (defined(SKY2_DRVMODEL_H) || defined(NULLSKY_DRVMODEL_H)) */

    /* Set up default state */
    _rpLtMapGlobals.lightMapSize = rpLTMAPDEFAULTLIGHTMAPSIZE;
    _rpLtMapGlobals.renderStyle  = rpLTMAPSTYLERENDERBASE |
                                   rpLTMAPSTYLERENDERLIGHTMAP;
    rwsprintf(_rpLtMapGlobals.prefix, rpLtMapDefaultPrefixString);
    RWASSERT(rwstrlen(RWSTRING("")) <= rpLTMAPMAXPREFIXSTRINGLENGTH);
    _rpLtMapGlobals.lightMapCount           = 0;
    _rpLtMapGlobals.areaLightSampleLimit    = rpLTMAPDEFAULTMAXAREALIGHTSAMPLESPERMESH;
    _rpLtMapGlobals.areaLightDensity        = 1.0f;
    _rpLtMapGlobals.areaLightRadius         = 1.0f;
    _rpLtMapGlobals.recipAreaLightROICutoff = rpLTMAPDEFAULTAREALIGHTROICUTOFF;
    _rpLtMapGlobals.sliverAreaThreshold     = rpLTMAPDEFAULTSLIVERAREATHRESHOLD;
    _rpLtMapGlobals.vertexWeldThreshold     = rpLTMAPDEFAULTVERTEXWELDTHRESHOLD;

    /* Another instance */
    _rpLtMapGlobals.module.numInstances++;

    RWRETURN(instance);
}

/*===========================================================================*
 *--- Stealth API Functions -------------------------------------------------*
 *===========================================================================*/

/****************************************************************************
 LtMapWorldLightMapListCreate
 */
static RwSList *
LtMapWorldLightMapListCreate(RpWorld *world)
{
    RwSList *lightMapList;

    RWFUNCTION(RWSTRING("LtMapWorldLightMapListCreate"));
    RWASSERT(NULL != world);

    lightMapList = rwSListCreate(sizeof(RwTexture *),
        rwID_LTMAPPLUGIN | rwMEMHINTDUR_EVENT);
    RWASSERT(NULL != lightMapList);

    RpWorldForAllWorldSectors(world, SectorLightMapListCB, (void *)lightMapList);

    RWRETURN(lightMapList);
}

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rpltmap
 * \ref RpLtMapGetRasterFormat returns the raster format used for creating
 * new lightmap.
 *
 * The raster format used for creating lightmap textures varies between
 * different platforms for performance reasons. A default format is preset
 * during initialization that is most suitable to the host plaform.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \return The raster format.
 *
 * \see RtLtMapLightMapsCreate
 * \see RpLtMapSetRasterFormat
 * \see RwRasterGetFormat
 * \see RwRasterSetFormat
 */

RwUInt32
RpLtMapGetRasterFormat(void)
{
    RWAPIFUNCTION(RWSTRING("RpLtMapGetRasterFormat"));

    RWRETURN(_rpLtMapGlobals.rasFormat);
}

/**
 * \ingroup rpltmap
 * \ref RpLtMapSetRasterFormat set the raster format to be used for creating
 * new lightmap.
 *
 * The raster format used for creating lightmap textures varies between
 * different platforms for performance reasons. A default format is preset
 * during initialization that is most suitable to the host plaform.
 *
 * If the lightmap textures is intended for a different target platform, then
 * the texture format should be in the target's recommended format.
 *
 * The recommended formats are
 *
 * \li PlayStation 2, rwRASTERTYPETEXTURE |
 *     rwRASTERFORMAT8888 |
 *     rwRASTERFORMATPAL8.
 *
 * \li Direct X, rwRASTERTYPETEXTURE |
 *     rwRASTERFORMAT888 |
 *     rwRASTERFORMATMIPMAP |
 *     rwRASTERFORMATAUTOMIPMAP.
 *
 * \li OpenGL, rwRASTERTYPETEXTURE |
 *     rwRASTERFORMAT888 |
 *     rwRASTERFORMATMIPMAP |
 *     rwRASTERFORMATAUTOMIPMAP.
 *
 * \li GameCube, rwRASTERTYPETEXTURE |
 *     rwRASTERFORMAT565 |
 *     rwRASTERFORMATMIPMAP |
 *     rwRASTERFORMATAUTOMIPMAP.
 *
 * \li XBoX,  rwRASTERTYPETEXTURE |
 *     rwRASTERFORMAT888 |
 *     rwRASTERFORMATMIPMAP |
 *     rwRASTERFORMATAUTOMIPMAP.
 *
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \return TRUE.
 *
 * \see RtLtMapLightMapsCreate
 * \see RpLtMapGetRasterFormat
 * \see RwRasterGetFormat
 * \see RwRasterSetFormat
 */
RwBool
RpLtMapSetRasterFormat(RwUInt32 format)
{
    RWAPIFUNCTION(RWSTRING("RpLtMapSetRasterFormat"));

    /* Check the new format is a texture. */
    RWASSERT(format & rwRASTERTYPETEXTURE);

     _rpLtMapGlobals.rasFormat = format;

    RWRETURN(TRUE);
}

/**
 * \ingroup rpltmap
 * \ref RpLtMapAtomicGetLightMap returns the lightmap
 * in use by a given \ref RpAtomic, if any.
 *
 * The lightmaps used by \ref RpAtomic's are exposed so that they may be
 * analysed, processed or replaced by the user. If an \ref RpAtomic has
 * no lightmap (calls to \ref RtLtMapLightMapsCreate have not yet created
 * one, or the \ref RpAtomic is not flagged with \ref rtLTMAPOBJECTLIGHTMAP)
 * then NULL will be returned.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param atomic  A pointer to an \ref RpAtomic for which the lightmap should be returned
 *
 * \return A pointer to the \ref RpAtomic's lightmap on success, NULL if the \ref RpAtomic
 * has no lightmap or on error.
 *
 * \see RpLtMapAtomicSetLightMap
 * \see RpLtMapWorldSectorGetLightMap
 * \see RpLtMapWorldSectorSetLightMap
 */
RwTexture *
RpLtMapAtomicGetLightMap(RpAtomic *atomic)
{
    RwTexture *result;

    RWAPIFUNCTION(RWSTRING("RpLtMapAtomicGetLightMap"));
    RWASSERT(NULL != atomic);

    result = RPLTMAPATOMICGETDATA(atomic)->lightMap;

    RWRETURN(result);
}

/**
 * \ingroup rpltmap
 * \ref RpLtMapAtomicSetLightMap is used to set
 * the lightmap used by an \ref RpAtomic.
 *
 * Note that lightmaps must be square and of power-of-two side length.
 * A NULL lightmap cannot be passed (instead, clear the
 * \ref rtLTMAPOBJECTLIGHTMAP flag for this \ref RpAtomic, prior to
 * creating lightmaps). If the \ref RpAtomic contains an existing
 * lightmap, it will be destroyed.
 *
 * Simply resampling a lightmap to a different resolution will
 * most likely result in visual artifacts.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  atomic    A pointer to an \ref RpAtomic
 * \param  lightMap  A pointer to an \ref RwTexture
 *
 * \return A pointer to the \ref RpAtomic on success, NULL otherwise
 *
 * \see RpLtMapAtomicGetLightMap
 * \see RpLtMapWorldSectorGetLightMap
 * \see RpLtMapWorldSectorSetLightMap
 */
RpAtomic *
RpLtMapAtomicSetLightMap(RpAtomic *atomic, RwTexture *lightMap)
{
    RwTexture *oldLightMap;
    RwRaster  *raster;
    RwUInt32   width;

    RWAPIFUNCTION(RWSTRING("RpLtMapAtomicSetLightMap"));
    RWASSERT(NULL != atomic);
    RWASSERT(NULL != lightMap);

    raster = RwTextureGetRaster(lightMap);
    width  = RwRasterGetWidth(raster);
    if (width != (RwUInt32)RwRasterGetHeight(raster))
    {
        RwDebugSendMessage(rwDEBUGERROR, "RpLtMapAtomicSetLightMap",
            "Lightmaps must be square");
        RWRETURN((RpAtomic *)NULL);
    }
    if (width & (width - 1))
    {
        RwDebugSendMessage(rwDEBUGERROR, "RpLtMapAtomicSetLightMap",
            "Lightmaps must have power-of-two side length");
        RWRETURN((RpAtomic *)NULL);
    }
    /* Update to the new size */
    RPLTMAPATOMICGETDATA(atomic)->lightMapSize = width;

    oldLightMap = RPLTMAPATOMICGETDATA(atomic)->lightMap;
    RPLTMAPATOMICGETDATA(atomic)->lightMap = lightMap;
    if (NULL != oldLightMap)
    {
        /* Decrement the refCount of the old lightmap */
        RwTextureDestroy(oldLightMap);
    }

    RWRETURN(atomic);
}


/**
 * \ingroup rpltmap
 * \ref RpLtMapWorldSectorGetLightMap returns the lightmap
 * in use by a given \ref RpWorldSector, if any.
 *
 * The lightmaps used by \ref RpWorldSector's are exposed so that they may
 * be analysed, processed or replaced by the user. If an \ref RpWorldSector
 * has no lightmap (calls to \ref RtLtMapLightMapsCreate have not yet
 * created one, or the \ref RpWorldSector is not flagged with
 * \ref rtLTMAPOBJECTLIGHTMAP) then NULL will be returned.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param sector  A pointer to an \ref RpWorldSector for which the lightmap should be returned
 *
 * \return A pointer to the \ref RpWorldSector's lightmap on success,
 * NULL if the \ref RpWorldSector has no lightmap or on error.
 *
 * \see RpLtMapWorldSectorSetLightMap
 * \see RpLtMapAtomicGetLightMap
 * \see RpLtMapAtomicSetLightMap
 */
RwTexture *
RpLtMapWorldSectorGetLightMap(RpWorldSector *sector)
{
    RwTexture *result;

    RWAPIFUNCTION(RWSTRING("RpLtMapWorldSectorGetLightMap"));
    RWASSERT(NULL != sector);

    result = RPLTMAPWORLDSECTORGETDATA(sector)->lightMap;

    RWRETURN(result);
}

/**
 * \ingroup rpltmap
 * \ref RpLtMapWorldSectorSetLightMap is used to set
 * the lightmap used by an \ref RpWorldSector.
 *
 * Note that lightmaps must be square and of power-of-two side length.
 * A NULL lightmap cannot be passed (instead, clear the
 * \ref rtLTMAPOBJECTLIGHTMAP flag for this \ref RpWorldSector, prior
 * to creating lightmaps). If the \ref RpWorldSector contains an existing
 * lightmap, it will be destroyed.
 *
 * Simply resampling a lightmap to a different resolution will
 * most likely result in visual artifacts.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  sector    A pointer to an \ref RpWorldSector
 * \param  lightMap  A pointer to an \ref RwTexture
 *
 * \return A pointer to the \ref RpWorldSector on success, NULL otherwise
 *
 * \see RpLtMapWorldSectorGetLightMap
 * \see RpLtMapAtomicGetLightMap
 * \see RpLtMapAtomicSetLightMap
 */
RpWorldSector *
RpLtMapWorldSectorSetLightMap(RpWorldSector *sector, RwTexture *lightMap)
{
    RwTexture *oldLightMap;
    RwRaster  *raster;
    RwUInt32   width;

    RWAPIFUNCTION(RWSTRING("RpLtMapWorldSectorSetLightMap"));
    RWASSERT(NULL != sector);
    RWASSERT(NULL != lightMap);

    raster = RwTextureGetRaster(lightMap);
    width  = RwRasterGetWidth(raster);
    if (width != (RwUInt32)RwRasterGetHeight(raster))
    {
        RwDebugSendMessage(rwDEBUGERROR, "RpLtMapWorldSectorSetLightMap",
            "Lightmaps must be square");
        RWRETURN((RpWorldSector *)NULL);
    }
    if (width & (width - 1))
    {
        RwDebugSendMessage(rwDEBUGERROR, "RpLtMapWorldSectorSetLightMap",
            "Lightmaps must have power-of-two side length");
        RWRETURN((RpWorldSector *)NULL);
    }
    /* Update to the new size */
    RPLTMAPWORLDSECTORGETDATA(sector)->lightMapSize = width;

    oldLightMap = RPLTMAPWORLDSECTORGETDATA(sector)->lightMap;
    RPLTMAPWORLDSECTORGETDATA(sector)->lightMap = lightMap;
    if (NULL != oldLightMap)
    {
        /* Decrement the refCount of the old lightmap */
        RwTextureDestroy(oldLightMap);
    }

    RWRETURN(sector);
}


/**
 * \ingroup rpltmap
 * \ref RpLtMapSetRenderStyle sets the rendering style to be
 * used by the the rendering pipelines of the lightmap plugin.
 *
 * Rendering style is specified using flags of type \ref RpLtMapStyle:
 * \li rpLTMAPSTYLERENDERBASE      The base texture should be rendered
 * \li rpLTMAPSTYLERENDERLIGHTMAP  The lightmap should be rendered
 * \li rpLTMAPSTYLEPOINTSAMPLE     The lightmap should be point-sampled
 *
 * rpLTMAPSTYLEPOINTSAMPLE is useful for "debugging" lightmaps as it
 * makes lightmap texels more clearly visible. The world parameter
 * is used to find lightmaps whose filter mode should be changed if
 * this flag is changed. The lightmaps modified will be those used by
 * the \ref RpAtomic's and \ref RpWorldSector's inside the world.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param style  The \ref RpLtMapStyle rendering style to use
 * \param world  An optional pointer to an \ref RpWorld
 *
 * \return TRUE on success, otherwise FALSE
 *
 * \see RpLtMapGetRenderStyle
 */
RwBool
RpLtMapSetRenderStyle(RpLtMapStyle style, RpWorld *world)
{
    RWAPIFUNCTION(RWSTRING("RpLtMapSetRenderStyle"));
    RWASSERT(0 < _rpLtMapGlobals.module.numInstances);

    if (NULL != world)
    {
        RwTextureFilterMode filtering = rwFILTERNEAREST;
        RwUInt32 format = RpLtMapGetRasterFormat();
        RwSList *lightMapList;
        RwTexture *result;
        RwUInt32 i;

        if (!(style & 4))
        {
            filtering = rwFILTERLINEAR;
            if (format & rwRASTERFORMATMIPMAP)
            {
                filtering = rwFILTERLINEARMIPLINEAR;
            }
        }

        lightMapList = LtMapWorldLightMapListCreate(world);
        RWASSERT(NULL != lightMapList);

        /* LM filtering */
        for (i = 0;i < (RwUInt32)rwSListGetNumEntries(lightMapList);++i)
        {
            result = RwTextureSetFilterMode(*(RwTexture **)
                         rwSListGetEntry(lightMapList, i), filtering);
        }

        rwSListDestroy(lightMapList);
    }

    /* Render modes are {base,LM,base+LM} */
/* TODO[5][ACM]: all pipelines should #predicate renderStyle on
 * RWDEBUG {esp. PS2!} */
    _rpLtMapGlobals.renderStyle = style;

    RWRETURN(TRUE);
}

/**
 * \ingroup rpltmap
 * \ref RpLtMapGetRenderStyle gets the rendering style being
 * used by the rendering pipelines of the lightmap plugin.
 *
 * Rendering style is specified using flags of type \ref RpLtMapStyle:
 * \li rpLTMAPSTYLERENDERBASE      The base texture should be rendered
 * \li rpLTMAPSTYLERENDERLIGHTMAP  The lightmap should be rendered
 * \li rpLTMAPSTYLEPOINTSAMPLE     The lightmap should be point-sampled
 *
 * rpLTMAPSTYLEPOINTSAMPLE is useful for "debugging" lightmaps as it
 * makes lightmap texels more clearly visible.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \return The current rendering style of the lightmap plugin
 *
 * \see RpLtMapSetRenderStyle
 */
RpLtMapStyle
RpLtMapGetRenderStyle(void)
{
    const RpLtMapStyle result = (RpLtMapStyle)_rpLtMapGlobals.renderStyle;
    RWAPIFUNCTION(RWSTRING("RpLtMapGetRenderStyle"));

    RWASSERT(0 < _rpLtMapGlobals.module.numInstances);

    RWRETURN(result);
}

/* TODO[6][ACU]: GENERIC PIPELINES NOT WRITTEN YET */
#if (0 && !defined(DOXYGEN))
/*
 * \ingroup rpltmap
 * \ref RpLtMapGetGenericAtomicPipeline is used to
 * retrieve the platform-independent pipeline which is
 * used to render lightmapped \ref RpAtomic's.
 *

TODOCS:
##################

 * The lightmap plugin must be attached before using this function.
 *
 * \return The \ref RxPipeline requested from the plugin, or
 * NULL if the pipeline wasn't constructed.
 *
 * \see RpLtMapGetGenericWorldSectorPipeline
 * \see RpLtMapGetPlatformAtomicPipeline
 * \see RpLtMapGetPlatformWorldSectorPipeline
 */
RxPipeline *
RpLtMapGetGenericAtomicPipeline(void)
{
    RxPipeline *pipeline;

    RWAPIFUNCTION(RWSTRING("RpLtMapGetGenericAtomicPipeline"));
    RWASSERT(0 < _rpLtMapGlobals.module.numInstances);

    pipeline = _rpLtMapGetGenericAtomicPipeline();

    RWRETURN(pipeline);
}

/*
 * \ingroup rpltmap
 * \ref RpLtMapGetGenericWorldSectorPipeline is used to
 * retrieve the platform-independent pipeline which is
 * used to render lightmapped \ref RpWorldSector's.
 *

TODOCS:
##################

 * The lightmap plugin must be attached before using this function.
 *
 * \return The \ref RxPipeline requested from the plugin, or
 * NULL if the pipeline wasn't constructed.
 *
 * \see RpLtMapGetGenericAtomicPipeline
 * \see RpLtMapGetPlatformAtomicPipeline
 * \see RpLtMapGetPlatformWorldSectorPipeline
 */
RxPipeline *
RpLtMapGetGenericWorldSectorPipeline(void)
{
    RxPipeline *pipeline;

    RWAPIFUNCTION(RWSTRING("RpLtMapGetGenericWorldSectorPipeline"));
    RWASSERT(0 < _rpLtMapGlobals.module.numInstances);

    pipeline = _rpLtMapGetGenericWorldSectorPipeline();

    RWRETURN(pipeline);
}
#endif /* (0 && !defined(DOXYGEN)) */

/**
 * \ingroup rpltmap
 * \ref RpLtMapGetPlatformAtomicPipeline is used to
 * retrieve the platform-specific pipeline which is
 * used to render lightmapped \ref RpAtomic's.
 *
 * The operation of the lightmap rendering pipelines may
 * be altered using \ref RpLtMapSetRenderStyle.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \return The platform-specific \ref RxPipeline for rendering
 * \ref RpAtomic's on success, otherwise NULL
 *
 * \see RpLtMapGetPlatformWorldSectorPipeline
 * \see RpLtMapSetRenderStyle.
 * \see RpLtMapGetRenderStyle.
 */
RxPipeline *
RpLtMapGetPlatformAtomicPipeline(void)
{
    RxPipeline *pipeline;

    RWAPIFUNCTION(RWSTRING("RpLtMapGetPlatformAtomicPipeline"));
    RWASSERT(0 < _rpLtMapGlobals.module.numInstances);

    pipeline = _rpLtMapGetPlatformAtomicPipeline();

    RWRETURN(pipeline);
}

/**
 * \ingroup rpltmap
 * \ref RpLtMapGetPlatformWorldSectorPipeline is used to
 * retrieve the platform-specific pipeline which is used
 * to render lightmapped \ref RpWorldSector's.
 *
 * The operation of the lightmap rendering pipelines may
 * be altered using \ref RpLtMapSetRenderStyle.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \return The platform-specific \ref RxPipeline for rendering
 * \ref RpWorldSector's on success, otherwise NULL
 *
 * \see RpLtMapGetPlatformAtomicPipeline
 * \see RpLtMapSetRenderStyle.
 * \see RpLtMapGetRenderStyle.
 */
RxPipeline *
RpLtMapGetPlatformWorldSectorPipeline(void)
{
    RxPipeline *pipeline;

    RWAPIFUNCTION(RWSTRING("RpLtMapGetPlatformWorldSectorPipeline"));
    RWASSERT(0 < _rpLtMapGlobals.module.numInstances);

    pipeline = _rpLtMapGetPlatformWorldSectorPipeline();

    RWRETURN(pipeline);
}

/**
 * \ingroup rpltmap
 * \ref RpLtMapWorldLightMapsQuery returns the number of lightmaps used
 * by the \ref RpWorldSector's and \ref RpAtomic's in an \ref RpWorld.
 *
 * Note that several \ref RpWorldSector's and/or \ref RpAtomic's may share a
 * single lightmap.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param world  An \ref RpWorld to query for lightmaps
 *
 * \return The number of lightmaps used by this \ref RpWorld
 *
 * \see RpLtMapWorldSectorGetLightMap
 * \see RpLtMapAtomicGetLightMap
 * \see RtLtMapLightMapsCreate
 */
RwUInt32
RpLtMapWorldLightMapsQuery(RpWorld *world)
{
    RwSList *lightMapList;
    RwUInt32 numLightMaps;

    RWAPIFUNCTION(RWSTRING("RpLtMapWorldLightMapsQuery"));
    RWASSERT(NULL != world);

    lightMapList = LtMapWorldLightMapListCreate(world);
    RWASSERT(NULL != lightMapList);
    numLightMaps = rwSListGetNumEntries(lightMapList);
    rwSListDestroy(lightMapList);

    RWRETURN(numLightMaps);
}

/**
 * \ingroup rpltmap
 * \ref RpLtMapPluginAttach is used to attach the lightmap plugin
 * to the RenderWare system to enable the rendering of lightmapped
 * \ref RpWorldSector's and \ref RpAtomic's.
 *
 * The plugin must be attached between initializing the system with
 * \ref RwEngineInit and opening it with \ref RwEngineOpen.
 *
 * \note The lightmap plugin requires the world plugin to be attached.
 * The include file rpltmap.h is also required and must be included by
 * an application wishing to use this plugin.
 *
 * \return TRUE if successful, FALSE if an error occurs.
 *
 * \see RpLtMapGetPlatformAtomicPipeline
 * \see RpLtMapGetPlatformWorldSectorPipeline
 * \see RpLtMapGetRenderStyle
 * \see RpLtMapSetRenderStyle
 * \see RpLtMapWorldSectorGetLightMap
 * \see RpLtMapWorldSectorSetLightMap
 * \see RpLtMapAtomicGetLightMap
 * \see RpLtMapAtomicSetLightMap
 */
RwBool
RpLtMapPluginAttach(void)
{
    RwInt32 success;

    RWAPIFUNCTION(RWSTRING("RpLtMapPluginAttach"));

    /* Register the plugIn */
    _rpLtMapGlobals.engineOffset =
        RwEngineRegisterPlugin(0,
                               rwID_LTMAPPLUGIN,
                               LtMapOpen,
                               LtMapClose);
    RWASSERT(0 < _rpLtMapGlobals.engineOffset);

    /* Extend atomic to hold lightmap pointer. */
    _rpLtMapGlobals.atomicOffset =
        RpAtomicRegisterPlugin(sizeof(LtMapObjectData),
                               rwID_LTMAPPLUGIN,
                               LtMapAtomicConstructor,
                               LtMapAtomicDestructor,
                               LtMapAtomicCopy);
    RWASSERT(0 < _rpLtMapGlobals.atomicOffset);


    /* Extend geometry to hold 'lightmapped' flag */
    _rpLtMapGlobals.geometryOffset =
        RpGeometryRegisterPlugin(sizeof(LtMapGeometryData),
                               rwID_LTMAPPLUGIN,
                               LtMapGeometryConstructor,
                               LtMapGeometryDestructor,
                               LtMapGeometryCopy);
    RWASSERT(0 < _rpLtMapGlobals.geometryOffset);
   
    /* Attach the stream handling functions. */
    success = RpAtomicRegisterPluginStream(rwID_LTMAPPLUGIN,
                                           LtMapAtomicRead,
                                           LtMapAtomicWrite,
                                           LtMapAtomicGetSize);
    RWASSERT(0 < success);


    /* Extend sector to hold lightmap pointer. */
    _rpLtMapGlobals.sectorOffset =
        RpWorldSectorRegisterPlugin(sizeof(LtMapObjectData),
                                    rwID_LTMAPPLUGIN,
                                    LtMapWorldSectorConstructor,
                                    LtMapWorldSectorDestructor,
                                    LtMapWorldSectorCopy);
    RWASSERT(0 < _rpLtMapGlobals.sectorOffset);

    /* Attach the stream handling functions. */
    success = RpWorldSectorRegisterPluginStream(rwID_LTMAPPLUGIN,
                                                LtMapWorldSectorRead,
                                                LtMapWorldSectorWrite,
                                                LtMapWorldSectorGetSize);
    RWASSERT(0 < success);


    /* Extend world to hold lightmap list. */
    _rpLtMapGlobals.worldOffset =
        RpWorldRegisterPlugin(sizeof(LtMapWorldData),
                                    rwID_LTMAPPLUGIN,
                                    LtMapWorldConstructor,
                                    NULL,
                                    LtMapWorldCopy);

    RWASSERT(0 < _rpLtMapGlobals.worldOffset);

    /* Attach the stream handling functions. */
    success = RpWorldRegisterPluginStream(rwID_LTMAPPLUGIN,
                                          LtMapWorldRead,
                                          LtMapWorldWrite,
                                          LtMapWorldGetSize);
    RWASSERT(0 < success);

    /* Extend material to hold area-lighting info. */
    _rpLtMapGlobals.materialOffset =
        RpMaterialRegisterPlugin(sizeof(LtMapMaterialData),
                                 rwID_LTMAPPLUGIN,
                                 LtMapMaterialConstructor,
                                 NULL,
                                 LtMapMaterialCopy);
    RWASSERT(0 < _rpLtMapGlobals.materialOffset);

    /* Attach the stream handling functions. */
    success = RpMaterialRegisterPluginStream(rwID_LTMAPPLUGIN,
                                             LtMapMaterialRead,
                                             LtMapMaterialWrite,
                                             LtMapMaterialGetSize);
    RWASSERT(0 < success);

/* TODO[6][ABX]: DAVIDG SAYS DXT1 IS REASONABLE COMPRESSION
 *               -- 565 IS BETTER QUALITY
 *               IT'S AVAILABLE ON PC [OGL?] AS WELL AS GCN.
 *                IF DAVID GIVES ME THE CODE TO CONVERT TO DXT1, WE CAN DO
 *               THE CONVERSION IN HERE SOMEWHERE [HOW TO MAKE A SIMPLE API?]
 *                ALSO COMPRESSED TEXTURES ON GCN MIGHT BE WORTHWHILE */

    /* *!*!* N.B *!*!* Remember that we need at least 1 bit alpha! */

#if (defined(SKY2_DRVMODEL_H) || defined(NULLSKY_DRVMODEL_H))
/* TODO[5]: TRY 5551/565 SOMETIME, SEE IF THERE'S A VISIBLE DIFFERENCE AND SEE IF ANY EFFECT ON FPS */
     _rpLtMapGlobals.rasFormat = rwRASTERTYPETEXTURE|rwRASTERFORMAT8888|rwRASTERFORMATPAL8;
#elif (defined(D3D8_DRVMODEL_H))
    /* From DavidG:
     *  o Palettized textures are evil, they are slow when combined with
     *    bilinear filtering and cant be used properly in mulitexturing.
     *  o Non mipmapped textures used to be slower than mipmapped ones.
     *  o Textures should be created in the same format than the backbuffer,
     *    this helps to reduce errors with a lot of video cards, and sometimes
     *    this format is also the faster one.
     *     Given the textures are in a texDict and people can vary framebuffer
     *    format at run-time, perhaps its best to default to 565 */
/* TODO[5]: TRY 5551 SOMETIME, SEE IF THERE'S A VISIBLE DIFFERENCE AND SEE IF ANY EFFECT ON FPS */
     _rpLtMapGlobals.rasFormat = rwRASTERTYPETEXTURE|rwRASTERFORMAT888;/*|
             rwRASTERFORMATMIPMAP|rwRASTERFORMATAUTOMIPMAP;*/
#elif (defined(D3D9_DRVMODEL_H))
    /* From DavidG:
     *  o Palettized textures are evil, they are slow when combined with
     *    bilinear filtering and cant be used properly in mulitexturing.
     *  o Non mipmapped textures used to be slower than mipmapped ones.
     *  o Textures should be created in the same format than the backbuffer,
     *    this helps to reduce errors with a lot of video cards, and sometimes
     *    this format is also the faster one.
     *     Given the textures are in a texDict and people can vary framebuffer
     *    format at run-time, perhaps its best to default to 565 */
/* TODO[5]: TRY 5551 SOMETIME, SEE IF THERE'S A VISIBLE DIFFERENCE AND SEE IF ANY EFFECT ON FPS */
     _rpLtMapGlobals.rasFormat = rwRASTERTYPETEXTURE|rwRASTERFORMAT888|
             rwRASTERFORMATMIPMAP|rwRASTERFORMATAUTOMIPMAP;
#elif (defined(OPENGL_DRVMODEL_H))
/* TODO[5]: TRY 565 SOMETIME, SEE IF THERE'S A VISIBLE DIFFERENCE AND SEE IF ANY EFFECT ON FPS */
     _rpLtMapGlobals.rasFormat = rwRASTERTYPETEXTURE|rwRASTERFORMAT888|
             rwRASTERFORMATMIPMAP|rwRASTERFORMATAUTOMIPMAP;
#elif (defined(GCN_DRVMODEL_H) || defined(NULLGCN_DRVMODEL_H))
    /* GCN doesn't support 888[8] palettised and 888[8] trilinear is somewhat
     * slow/inconvenient. 565 palettised might look a bit crap, so skip that too. */
     _rpLtMapGlobals.rasFormat = rwRASTERTYPETEXTURE|rwRASTERFORMAT565|
             rwRASTERFORMATMIPMAP|rwRASTERFORMATAUTOMIPMAP;
#elif (defined(XBOX_DRVMODEL_H) || defined(NULLXBOX_DRVMODEL_H))
    /* Palettised are slow w/ bilinear on XBox */
     _rpLtMapGlobals.rasFormat = rwRASTERTYPETEXTURE|rwRASTERFORMAT888|
             rwRASTERFORMATMIPMAP|rwRASTERFORMATAUTOMIPMAP;
#elif (defined(SOFTRAS_DRVMODEL_H))
    /* Palettes and mipmaps are slow here and it uses only 8888 rasters */
     _rpLtMapGlobals.rasFormat = rwRASTERTYPETEXTURE|rwRASTERFORMAT8888;
#elif (defined(NULL_DRVMODEL_H))
    /* Go for the lowest common denominator in this case, we'll be
     * generating platform-independent texDicts or images in this build */
     _rpLtMapGlobals.rasFormat = rwRASTERTYPETEXTURE|rwRASTERFORMAT888|
             rwRASTERFORMATMIPMAP|rwRASTERFORMATAUTOMIPMAP;
#else
#error "Unsupported platform!"
#endif

    RWRETURN(TRUE);
}

