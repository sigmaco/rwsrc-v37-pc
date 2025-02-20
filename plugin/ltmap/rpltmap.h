/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   rpltmap.h                                                  -*
 *-                                                                         -*
 *-  Purpose :   Lightmap plugin public/internal API                        -*
 *-                                                                         -*
 *===========================================================================*/

/* RWPUBLIC */

/**
 * \defgroup rpltmap RpLtMap
 * \ingroup lighting
 *
 * Lightmap Plugin for RenderWare Graphics.
 */

#ifndef RPLTMAP_H
#define RPLTMAP_H

/*===========================================================================*
 *--- Includes --------------------------------------------------------------*
 *===========================================================================*/

#include "rwcore.h"
#include "rpworld.h"

/* RWPUBLICEND */

#include "rpcriter.h"
#include "rpltmap.rpe"

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/

#define LTMAPMAXPIPES 3

#define RPLTMAPMATERIALGETDATA(_mtrl)                             \
     ((LtMapMaterialData *)(((RwUInt8 *)(_mtrl)) +                \
                  _rpLtMapGlobals.materialOffset))

#define RPLTMAPMATERIALGETCONSTDATA(_mtrl)                        \
     ((const LtMapMaterialData *)(((const RwUInt8 *)(_mtrl)) +    \
                  _rpLtMapGlobals.materialOffset))

#define RPLTMAPATOMICGETDATA(_atmc)                               \
     ((LtMapObjectData *)(((RwUInt8 *)(_atmc)) +                  \
                  _rpLtMapGlobals.atomicOffset))

#define RPLTMAPATOMICGETCONSTDATA(_atmc)                          \
     ((const LtMapObjectData *)(((const RwUInt8 *)(_atmc)) +      \
                  _rpLtMapGlobals.atomicOffset))

#define RPLTMAPGEOMETRYGETDATA(_gmtr)                             \
     ((LtMapGeometryData *)(((RwUInt8 *)(_gmtr)) +                \
                  _rpLtMapGlobals.geometryOffset))

#define RPLTMAPGEOMETRYGETCONSTDATA(_gmtr)                        \
     ((const LtMapGeometryData *)(((const RwUInt8 *)(_gmtr)) +    \
                  _rpLtMapGlobals.geometryOffset))

#define RPLTMAPWORLDSECTORGETDATA(_sctr)                          \
     ((LtMapObjectData *)(((RwUInt8 *)(_sctr)) +                  \
                  _rpLtMapGlobals.sectorOffset))

#define RPLTMAPWORLDSECTORGETCONSTDATA(_sctr)                     \
     ((const LtMapObjectData *)(((const RwUInt8 *)(_sctr)) +      \
                  _rpLtMapGlobals.sectorOffset))

#define RPLTMAPWORLDGETDATA(_wrld)                                \
     ((LtMapWorldData *)(((RwUInt8 *)(_wrld)) +                   \
                  _rpLtMapGlobals.worldOffset))

#define RPLTMAPWORLDGETCONSTDATA(_wrld)                           \
     ((const LtMapWorldData *)(((const RwUInt8 *)(_wrld)) +       \
                  _rpLtMapGlobals.worldOffset))

#define RPLTMAPTEXTUREGETDATA(_txtr)                              \
     ((LtMapTextureData *)(((RwUInt8 *)(_txtr)) +                 \
                  _rpLtMapGlobals.textureOffset))

#define RPLTMAPTEXTUREGETCONSTDATA(_txtr)                         \
     ((const LtMapTextureData *)(((const RwUInt8 *)(_txtr)) +     \
                  _rpLtMapGlobals.textureOffset))

#define RPLTMAPIMAGEFROMTEXTURE(_text)                            \
    ( *(RPLTMAPTEXTUREGETDATA(_text)) )

#define RPLTMAPIMAGEFROMCONSTTEXTURE(_text)                       \
    ( *(RPLTMAPTEXTUREGETCONSTDATA(_text)) )



#define RPLTMAPSAMPLEMAPSET(_map, _x, _y, _s)        \
    (_map)[((_y) * (_s) + (_x)) >> 0x03] |= (1 << (((_y) * (_s) + (_x)) & 0x07))

#define RPLTMAPSAMPLEMAPUNSET(_map, _x, _y, _s)        \
    (_map)[((_y) * (_s) + (_x)) >> 0x03] &= ~(1 << (((_y) * (_s) + (_x)) & 0x07))

#define RPLTMAPSAMPLEMAPGET(_map, _x, _y, _s)        \
    ((_map)[((_y) * (_s) + (_x)) >> 0x03] & (1 << (((_y) * (_s) + (_x)) & 0x07)))

#define RPLTMAPSAMPLEMAPSIZE(_x, _y) \
    ((((_x) * (_y)) + 0x07) >> 0x03)


/* The extra 4 bytes are for version number */
#define LTMAPMATERIALSTREAMDATASIZE    \
    (rwCHUNKHEADERSIZE + sizeof(LtMapMaterialStreamData))
#define LTMAPOBJECTSTREAMDATASIZE      \
    (rwCHUNKHEADERSIZE + sizeof(LtMapObjectStreamData))
#define LTMAPWORLDSTREAMDATASIZE       \
    (rwCHUNKHEADERSIZE + sizeof(LtMapWorldStreamData))
#define LTMAPGEOMETRYSTREAMDATASIZE    \
    (rwCHUNKHEADERSIZE + sizeof(LtMapGeometryStreamData))

#define rpLTMAPUNUSEDALPHA       ((RwUInt8)0)
#define rpLTMAPGREEDILYUSEDALPHA ((RwUInt8)1)
#define rpLTMAPUSEDALPHA         ((RwUInt8)2)

/* RWPUBLIC */

/* Used during lightmap illumination (sliver triangles are skipped
 * (their texels should be filled by dilate()), because their normals
 * can't be accurately calculated) */
#define rpLTMAPDEFAULTSLIVERAREATHRESHOLD (0.001f)

/* Used during lightmap UV calculation (polySets may be
 * joined on the basis of vertices with equal positions) */
#define rpLTMAPDEFAULTVERTEXWELDTHRESHOLD (0.1f)

#define rpLTMAPDEFAULTLIGHTMAPSIZE 128
#define rpLTMAPMINLIGHTMAPSIZE     16
#define rpLTMAPMAXLIGHTMAPSIZE     512/*?? any better way of determining this ??*/

#define rpLTMAPMAXPREFIXSTRINGLENGTH 4

#define rpLTMAPDEFAULTMAXAREALIGHTSAMPLESPERMESH 256
/* The default tolerance for errors induced by area light ROIs is 1
 * (being the smallest difference in lightmap colour values) */
#define rpLTMAPDEFAULTAREALIGHTROICUTOFF (1.0f)


/* Misc flags for used in the world plugin data. */
#define rpLTMAPWORLDFLAGLIGHTMAP        0x01


/* RWPUBLICEND */

/*===========================================================================*
 *--- Global Types ----------------------------------------------------------*
 *===========================================================================*/

/* RWPUBLIC */

/**
 * \ingroup rpltmap
 * \ref RpLtMapStyle
 * Flags specifying the rendering style of the lightmap plugin.
 *
 * \see RpLtMapGetRenderStyle
 * \see RpLtMapSetRenderStyle
 */
enum RpLtMapStyle
{
    rpLTMAPSTYLENASTYLE          = 0x0,

    rpLTMAPSTYLERENDERBASE       = 0x1, /**< The base texture should be rendered */
    rpLTMAPSTYLERENDERLIGHTMAP   = 0x2, /**< The lightmap should be rendered */
    rpLTMAPSTYLEPOINTSAMPLE      = 0x4, /**< The lightmap should be point-sampled */

    rpLTMAPSTYLEFORCEENUMSIZEINT = 0x7FFFFFFF
};
typedef enum RpLtMapStyle RpLtMapStyle;

#if (!defined(DOXYGEN))
typedef struct rpLtMapSampleMap rpLtMapSampleMap;
struct rpLtMapSampleMap
{
    RwUInt32    refCount;
    RwUInt8     *samplePixels;
};
#endif /* (!defined(DOXYGEN)) */

/* RWPUBLICEND */

typedef struct LtMapPlatformData   LtMapPlatformData;
typedef struct LtMapGlobalPlatform LtMapGlobalPlatform;

typedef struct LtMapGlobals LtMapGlobals;
struct LtMapGlobals
{
    /* Offsets */
    RwInt32      engineOffset;
    RwInt32      atomicOffset;
    RwInt32      geometryOffset;
    RwInt32      sectorOffset;
    RwInt32      worldOffset;
    RwInt32      materialOffset;
    RwInt32      textureOffset;

    /* Plugin data */
    RwModuleInfo module;
    RwUInt32     lightMapSize;            /* The current default on-creation lightmap resolution.
                                           * Overridden by per-object values */
    RwUInt32     renderStyle;             /* The current RpLtMapStyle */
    RwChar       prefix[rpLTMAPMAXPREFIXSTRINGLENGTH + 1]; /* Default prefix string for lightmaps */
    RwUInt32     lightMapCount;
    RwUInt32     areaLightSampleLimit;    /* If an area light mesh has more samples than
                                           * this, its density gets reduced */
    RwReal       areaLightDensity;        /* This is a convenient way of modifying area
                                           * light sample density for *all* materials */
    RwReal       areaLightRadius;         /* This is a convenient way of modifying area
                                           * light falloff for *all* materials */
    RwReal       recipAreaLightROICutoff; /* The R.O.I. of area lights (used for culling during
                                           * lighting) is defined as the distance at which light
                                           * intensity is reduced to this value (so dimmer lights
                                           * have smaller R.O.I.s) */
    RwReal       sliverAreaThreshold;     /* Used during lightmap illumination (sliver triangles are
                                           * skipped (their texels should be filled by dilate()),
                                           * because their normals can't be accurately calculated) */
    RwReal       vertexWeldThreshold;     /* Used during lightmap UV calculation (polySets may be
                                           * joined on the basis of vertices with equal positions) */
#if (defined(SKY2_DRVMODEL_H) || defined(NULLSKY_DRVMODEL_H))
    RwTexture   *whiteTex;                /* Used to replace the base texture for the
                                           * "lightmap only" renderStyle on PS2 */
#endif /* (defined(SKY2_DRVMODEL_H) || defined(NULLSKY_DRVMODEL_H)) */

    RwUInt32     rasFormat;               /* Format used for creating lightmap rasters. */

    /* Generic pipes */
    RxPipeline  *genericPipes[2];

    /* Platform pipes */
    RxPipeline  *platformPipes[LTMAPMAXPIPES];
};

typedef struct LtMapObjectData LtMapObjectData;
struct LtMapObjectData
{
    RwUInt32   flags;      /* See \ref RtLtMapObjectFlags */
    RwUInt32   numSamples; /* Number of lightmap texels used to light this object (does
                            * NOT include vertex prelights) - used in progressCBs  */
    RwUInt32   lightMapSize; /* Width of lightmap texture to create for this object.
                              * object - used in progressCBs. Zero means use
                              * the global default value on creation. */
    RwUInt8    clearCol;
    RwRGBA     color;
    RwTexture *lightMap;

    rpLtMapSampleMap    *sampleMap;
};

typedef struct LtMapObjectStreamData LtMapObjectStreamData;
struct LtMapObjectStreamData
{
    RwUInt32 flags;        /* See \ref RtLtMapObjectFlags */
    RwUInt32 numSamples;   /* Number of lightmap texels used to light this object (does
                            * NOT include vertex prelights) - used in progressCBs */
    RwUInt32 lightMapSize; /* Saved per-object in case lightmaps are missing on load and
                            * so blank lightmaps of the appropriate size need creating */
    RwChar   lightMapName[rwTEXTUREBASENAMELENGTH]; /* The name of this atomic's lightmap */
};

typedef struct LtMapGeometryData
{
    struct LtMapPolySet *PolySetArray;     /* Set by LtMapObjectAllocateUV ().  Stops
                                     * shared geometry being re-lightmapped. */
    RwInt32      numSets;           /* Only valid if PolySetArray != NULL */
} LtMapGeometryData;
typedef LtMapGeometryData LtMapGeometryStreamData; /* File/memory are identical */

typedef struct LtMapWorldData LtMapWorldData;
struct LtMapWorldData
{
    RwUInt32    flag;            /* General purpose flag. */
    RwReal      lightMapDensity; /* World-space lightmap sample density */
};

typedef struct LtMapWorldStreamData LtMapWorldStreamData;
struct LtMapWorldStreamData
{
    RwReal   lightMapDensity; /* World-space lightmap sample density, multiplied by a per-material value */
};

typedef struct LtMapMaterialData LtMapMaterialData;
struct LtMapMaterialData
{
    RwUInt32 flags;            /* See RtLtMapMaterialFlags */
    RwReal   lightMapDensity;  /* Multiplies the world's lightmap density, useful
                                * to get large floor areas sampled at a lower rate */
    RwRGBA   areaLightColour;  /* The color of this area light emitter (it can effectively
                                * be scaled by areaLightRadius) */
    RwReal   areaLightDensity; /* Multiplies the default world area light density, which is either
                                * set in the call to \ref RtLtMapAreaLightGroupCreate() or
                                * defaults to twice the world's lightmap sample density. */
    RwReal   areaLightRadius;  /* This multiplies the distance at which an area light gives
                                * a certain light contribution (it is combined with the value
                                * set by \ref RtLtMapSetAreaLightRadiusModifier) */
};

/* Memory/file structures are identical as of yet */
typedef struct LtMapMaterialData LtMapMaterialStreamData;

/* This keeps the RwImage used during lighting tightly bound to lightmap textures */
typedef RwImage * LtMapTextureData;

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

extern LtMapGlobals _rpLtMapGlobals;

extern RwChar rpLtMapDefaultPrefixString[];
extern RwChar rpLtMapImageExtension[];

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/* RWPUBLIC */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* RWPUBLICEND */

extern RwBool _rpLtMapPipelinesCreate(RwUInt32 pipes);
extern RwBool _rpLtMapPipelinesDestroy(void);

extern RwInt32
_rpLtMapGeometryNativeSize(const RpGeometry *geometry);
extern RwStream *
_rpLtMapGeometryNativeWrite(RwStream *stream, const RpGeometry *geometry);
extern RwStream *
_rpLtMapGeometryNativeRead(RwStream *stream, RpGeometry *geometry);

extern RwInt32
_rpLtMapWorldSectorNativeSize(const RpWorldSector *sector);
extern RwStream *
_rpLtMapWorldSectorNativeWrite(RwStream *stream, const RpWorldSector *sector);
extern RwStream *
_rpLtMapWorldSectorNativeRead(RwStream *stream, RpWorldSector *sector);

extern RxPipeline *
_rpLtMapGetPlatformAtomicPipeline(void);
extern RxPipeline *
_rpLtMapGetPlatformWorldSectorPipeline(void);

extern RwBool
_rpLtMapPlatformPipelinesCreate(void);
extern void
_rpLtMapPlatformPipelinesDestroy(void);

extern RwTexture *_rpLtMapClearLightMap(RwTexture *lightMap, RwRGBA *color);

extern RpWorld *_rpLtMapWorldDestroyAreaLights(RpWorld *world);

extern rpLtMapSampleMap *
_rpLtMapSampleMapCreate( RwUInt32 w, RwUInt32 h );

extern RwBool
_rpLtMapSampleMapDestroy( rpLtMapSampleMap *sMap );

/* RWPUBLIC */

extern RwBool
RpLtMapPluginAttach(void);

extern RxPipeline *
RpLtMapGetPlatformAtomicPipeline(void);
extern RxPipeline *
RpLtMapGetPlatformWorldSectorPipeline(void);

extern RwBool
RpLtMapSetRenderStyle(RpLtMapStyle style, RpWorld *world);
extern RpLtMapStyle
RpLtMapGetRenderStyle(void);

extern RwUInt32
RpLtMapWorldLightMapsQuery(RpWorld *world);

extern RwTexture *
RpLtMapWorldSectorGetLightMap(RpWorldSector *sector);
extern RpWorldSector *
RpLtMapWorldSectorSetLightMap(RpWorldSector *sector, RwTexture *lightMap);
extern RwTexture *
RpLtMapAtomicGetLightMap(RpAtomic *atomic);
extern RpAtomic *
RpLtMapAtomicSetLightMap(RpAtomic *atomic, RwTexture *lightMap);

extern RwUInt32 RpLtMapGetRasterFormat(void);
extern RwBool RpLtMapSetRasterFormat(RwUInt32 format);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RPLTMAP_H */

/* RWPUBLICEND */
