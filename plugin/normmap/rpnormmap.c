/**
 * \ingroup rpnormmap
 * \page rpnormmapoverview RpNormMap Plugin Overview
 *
 * \par Requirements for rpnormmap library
 * \li \b Headers: rwcore.h, rpworld.h, rpnormmap.h
 * \li \b Libraries: rwcore, rpworld, rpnormmap
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpNormMapPluginAttach

 * \par Requirements for rpnormmapskin library
 * The rpnormmapskin.lib library contains both the \ref rpNORMMAPATOMICSTATICPIPELINE and
 * \ref rpNORMMAPATOMICSKINNEDPIPELINE pipelines.
 * \li \b Headers: rwcore.h, rpworld.h, rpskin.h, rpnormmap.h
 * \li \b Libraries: rwcore, rpworld, rpskin, rpnormmap
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpSkinPluginAttach,
 *     \ref RpNormMapPluginAttach
 *
 * \subsection normmapoverview Overview
 * This plugin provides facilities for using normal maps. Normal maps are textures
 * that store a normal per pixel encoded as a color, converting the components
 * of the normals from the range [-1..1] to [0..255]. These packed normals are
 * later unpacked in the pixel shader and are used to calculate the light contribution
 * per pixel.
 * \note Light distance attenuation for point lights or spotlights is still
 * calculated per vertex.
 *
 * The normal map plugin only supports one ambient light and one non-ambient light.
 * You can directly specify the lights to be used by the plugin by calling the function
 * \ref RpNormMapSetActiveLights.
 * \note In order to apply more than one non-ambient light to the same object,
 * you have to render additional additive passes for each light,
 * setting the ambient light color to black for all but the first light.
 * To force blending in additional passes for opaque objects,
 * you have to set their materials color alpha values to 254 during each pass.
 *
 * You can calculate a normal map from any \ref RwImage using the toolkit \ref rtnormmap.
 *
 * The normal map plugin supports static and skinned atomics, and world sectors.
 * Environment maps are also supported
 *
 * Before any of the plugin functions are used, the plugin
 * should be attached using \ref RpNormMapPluginAttach.
 *
 * \par Atomics
 *
 * The normal maps pipeline must be initialized with
 * \ref RpNormMapAtomicInitialize for each atomic that contains normal maps.
 *
 * \par World Sectors
 *
 * The normal maps pipeline must be enabled with \ref RpNormMapWorldEnable
 * and \ref RpNormMapWorldSectorInitialize for each world sector that contains normal maps.
 *
 * \par Normal Maps
 *
 * Normal Maps need to be setup for each material:
 *
 * \li \ref RpNormMapMaterialSetNormMapTexture
 * \li \ref RpNormMapMaterialGetNormMapTexture
 *
 * \par Environment Mapping
 *
 * These functions are available if environment mapping is used:
 *
 * \li \ref RpNormMapMaterialSetEnvMapTexture.
 * \li \ref RpNormMapMaterialSetEnvMapCoefficient.
 * \li \ref RpNormMapMaterialSetEnvMapFrame.
 * \li \ref RpNormMapMaterialModulateEnvMap.
 * \li \ref RpNormMapMaterialGetEnvMapTexture.
 * \li \ref RpNormMapMaterialGetEnvMapCoefficient.
 * \li \ref RpNormMapMaterialGetEnvMapFrame.
 * \li \ref RpNormMapMaterialIsEnvMapModulated.
 *
 * \par
 * \see \ref rtnormmap
 */

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include "rwcore.h"
#include "rpworld.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpnormmap.h"
#include "normmap.h"

#define NORMALMAP_ENABLED          1
#define NORMALMAP_ENVMAP_ENABLED   16
#define NORMALMAP_ENVMAP_MODULATE  32

/*--- Global ---*/
#if (defined(RWDEBUG))
long rpNormMapStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

/*--- Local Variables ---*/

RwInt32 _rpNormMapMaterialExOffset = 0;
RwInt32 _rpNormMapAtomicExOffset = 0;
RwInt32 _rpNormMapWorldExOffset = 0;

RxPipeline  *_rpNormMapWorldSectorPipeline = NULL;

RpLight  *_rpNormMapMainLightActive = NULL;
RpLight  *_rpNormMapAmbientLightActive = NULL;

/*
 ***************************************************************************
 */
static void *
NormalMapConstruct(void *object, RwInt32 offset, RwInt32 __RWUNUSED__ size)
{
    NormalMapExt  *normalmapext;

    RWFUNCTION(RWSTRING("NormalMapConstruct"));

    normalmapext = OBJECTGETNORMALMAP(object, offset);
    normalmapext->normalMap = NULL;
    normalmapext->envMap = NULL;
    normalmapext->shininess = 1.f;
    normalmapext->envmapFrame = NULL;
    normalmapext->modulateEnvMap = FALSE;

    RWRETURN(object);
}

/*
 ***************************************************************************
 */
static void        *
NormalMapDestruct(void *object, RwInt32 offset, RwInt32 __RWUNUSED__ size)
{
    NormalMapExt  *normalmapext;

    RWFUNCTION(RWSTRING("NormalMapDestruct"));

    normalmapext = OBJECTGETNORMALMAP(object, offset);

    if (normalmapext->normalMap != NULL)
    {
        RwTextureDestroy(normalmapext->normalMap);
        normalmapext->normalMap = NULL;
    }

    if (normalmapext->envMap != NULL)
    {
        RwTextureDestroy(normalmapext->envMap);
        normalmapext->envMap = NULL;
    }

    normalmapext->envmapFrame = NULL;
    normalmapext->modulateEnvMap = FALSE;

    RWRETURN(object);
}

/*
 ***************************************************************************
 */
static void        *
NormalMapCopy(void *destinationObject, const void *sourceObject,
             RwInt32 offset, RwInt32 __RWUNUSED__ size)
{
    NormalMapExt           *destination;
    const NormalMapExt     *source;

    RWFUNCTION(RWSTRING("NormalMapCopy"));

    destination = OBJECTGETNORMALMAP(destinationObject, offset);
    source = CONSTOBJECTGETNORMALMAP(sourceObject, offset);

    if (destination->normalMap != NULL)
    {
        RwTextureDestroy(destination->normalMap);
    }

    if (destination->envMap != NULL)
    {
        RwTextureDestroy(destination->envMap);
    }

    destination->normalMap = source->normalMap;
    destination->envMap = source->envMap;

    if (destination->normalMap != NULL)
    {
        (void)RwTextureAddRef(destination->normalMap);
    }

    if (destination->envMap != NULL)
    {
        (void)RwTextureAddRef(destination->envMap);
    }

    destination->envmapFrame = source->envmapFrame;

    destination->modulateEnvMap = source->modulateEnvMap;

    RWRETURN(destinationObject);
}

/*
 ***************************************************************************
 */
static RwStream *
NormalMapStreamWriteTexture(RwStream *stream, const RwTexture *texture)
{
    RWFUNCTION(RWSTRING("NormalMapStreamWriteTexture"));
    RWASSERT(stream);
    RWASSERT(texture);

    if (!RwTextureStreamWrite(texture, stream))
    {
        RWRETURN((RwStream *)NULL);
    }

    RWRETURN(stream);
}

/*
 ***************************************************************************
 */
static RwStream *
NormalMapStreamReadTexture(RwStream *stream, RwTexture **texture)
{
    RwError err;

    RWFUNCTION(RWSTRING("NormalMapStreamReadTexture"));
    RWASSERT(stream);
    RWASSERT(texture);

    if (!RwStreamFindChunk(
            stream, rwID_TEXTURE, (RwUInt32 *)NULL, (RwUInt32 *)NULL))
    {
        RWRETURN((RwStream *)NULL);
    }

    /* We rely on error mechanism to recover from benign non-existence of
     * texture. First clear any current error (presumably also benign).
     */
    RwErrorGet(&err);

    *texture = RwTextureStreamRead(stream);

    if (!(*texture))
    {
        /* Recover if just failed to read texture */
        RwErrorGet(&err);
        if ((err.errorCode != E_RW_NOERROR)
            && (err.errorCode != E_RW_READTEXMASK))
        {
            /* Something else went wrong, so lets put back the error and abort */
            RwErrorSet(&err);
            RWRETURN((RwStream *)NULL);
        }
    }

    RWRETURN(stream);
}

/*
 ***************************************************************************
 */
static RwUInt32
NormalMapStreamSizeTexture(const RwTexture *texture)
{
    RwUInt32 size;

    RWFUNCTION(RWSTRING("NormalMapStreamSizeTexture"));
    RWASSERT(texture);

    size = RwTextureStreamGetSize(texture) + rwCHUNKHEADERSIZE;

    RWRETURN(size);
}

/*
 ***************************************************************************
 */
static RwStream    *
NormalMapMaterialStreamWrite(RwStream      *stream,
                         RwInt32        binaryLength   __RWUNUSEDRELEASE__,
                         const void    *object,
                         RwInt32        offsetInObject __RWUNUSED__,
                         RwInt32        sizeInObject   __RWUNUSED__)
{
    const RpMaterial  *material = (const RpMaterial *) object;
    const NormalMapExt *normalMapExt;
    RwUInt32 flags;

    RWFUNCTION(RWSTRING("NormalMapMaterialStreamWrite"));
    RWASSERT(stream);
    RWASSERT(object);
    RWASSERT(binaryLength > 0);

    /* Lets grap the material and it's data. */
    normalMapExt = CONSTMATERIALGETNORMALMAP(material);

    /* If we are here, we should have a normal map */
    RWASSERT(normalMapExt->normalMap != NULL);

    /* Save flags */
    flags = 0;

    if (normalMapExt->normalMap != NULL)
    {
        flags |= NORMALMAP_ENABLED;

        if (normalMapExt->envMap != NULL)
        {
            flags |= NORMALMAP_ENVMAP_ENABLED;

            if (normalMapExt->modulateEnvMap)
            {
                flags |= NORMALMAP_ENVMAP_MODULATE;
            }
        }
    }

    if (!RwStreamWriteInt32(stream, (RwInt32 *)&flags, sizeof(RwUInt32)))
    {
        RWRETURN((RwStream *)NULL);
    }

    /* Save Normal Map */
    if (normalMapExt->normalMap != NULL)
    {
        if (!NormalMapStreamWriteTexture(stream, normalMapExt->normalMap))
        {
            RWRETURN((RwStream *)NULL);
        }

        /* Save Env Map */
        if (normalMapExt->envMap != NULL)
        {
            /* First we write the coefficient ... */
            if (!RwStreamWriteReal(
                    stream, &(normalMapExt->shininess), sizeof(RwReal)))
            {
                RWRETURN((RwStream *)NULL);
            }

            /* ... and the texture ... */
            if (!NormalMapStreamWriteTexture(stream, normalMapExt->envMap))
            {
                RWRETURN((RwStream *)NULL);
            }
        }
    }

    /* And we're spent. */
    RWRETURN(stream);
}

/*
 ***************************************************************************
 */
static RwStream    *
NormalMapMaterialStreamRead(RwStream   *stream,
                        RwInt32     binaryLength    __RWUNUSEDRELEASE__,
                        void       *object,
                        RwInt32     offsetInObject  __RWUNUSED__,
                        RwInt32     sizeInObject    __RWUNUSED__)
{
    RpMaterial          *material = (RpMaterial *) object;
    RwUInt32            flags;
    RwTexture           *texture = NULL;

    RWFUNCTION(RWSTRING("NormalMapMaterialStreamRead"));
    RWASSERT(stream);
    RWASSERT(object);
    RWASSERT(binaryLength > 0);

    /* Read flags */
    if (!RwStreamReadInt32(stream, (RwInt32 *)&flags, sizeof(RwUInt32)))
    {
        RWRETURN((RwStream *)NULL);
    }

    /* If we are here, we should have a normal map */
    RWASSERT(flags & NORMALMAP_ENABLED);

    /* Normal Map? */
    if (flags & NORMALMAP_ENABLED)
    {
        /* read normal map texture */
        if (!NormalMapStreamReadTexture(stream, &texture))
        {
            RWRETURN((RwStream *)NULL);
        }

        if (texture != NULL)
        {
            RpNormMapMaterialSetNormMapTexture(material, texture);

            /* This has incremented the texture reference count
             * so correct the reference count */
            RwTextureDestroy(texture);

            /* EnvMap? */
            if (flags & NORMALMAP_ENVMAP_ENABLED)
            {
                RwReal              coef;

                /* First we read the coefficient ... */
                if (!RwStreamReadReal(stream, &coef, sizeof(RwReal)))
                {
                    RWRETURN((RwStream *)NULL);
                }

                /* ... and the texture ... */
                if (!NormalMapStreamReadTexture(stream, &texture))
                {
                    RWRETURN((RwStream *)NULL);
                }

                /* Setup the effect. */
                if (texture != NULL)
                {
                    RpNormMapMaterialSetEnvMapTexture(material, texture);

                    /* This has incremented the texture reference count
                     * so correct the reference count */
                    RwTextureDestroy(texture);
                }

                RpNormMapMaterialSetEnvMapCoefficient(material, coef);

                if (flags & NORMALMAP_ENVMAP_MODULATE)
                {
                    RpNormMapMaterialModulateEnvMap(material, TRUE);
                }
                else
                {
                    RpNormMapMaterialModulateEnvMap(material, FALSE);
                }
            }
            else
            {
                RpNormMapMaterialSetEnvMapTexture(material, NULL);
                RpNormMapMaterialModulateEnvMap(material, FALSE);
            }
        }
        else
        {
            RpNormMapMaterialSetNormMapTexture(material, NULL);
            RpNormMapMaterialSetEnvMapTexture(material, NULL);
            RpNormMapMaterialModulateEnvMap(material, FALSE);
        }
    }
    else
    {
        RpNormMapMaterialSetNormMapTexture(material, NULL);
        RpNormMapMaterialSetEnvMapTexture(material, NULL);
        RpNormMapMaterialModulateEnvMap(material, FALSE);
    }

    /* And we're spent. */
    RWRETURN(stream);
}

/*
 ***************************************************************************
 */
static RwInt32
NormalMapMaterialStreamGetSize(const void  *object,
                           RwInt32      offsetInObject  __RWUNUSED__,
                           RwInt32      sizeInObject    __RWUNUSED__)
{
    const RpMaterial   *material = (const RpMaterial *) object;
    const NormalMapExt *normalMapExt;
    RwInt32             size;

    RWFUNCTION(RWSTRING("NormalMapMaterialStreamGetSize"));
    RWASSERT(object);

    /* Lets grap the material and it's data. */
    normalMapExt = CONSTMATERIALGETNORMALMAP(material);

    size = 0;
    if (normalMapExt->normalMap != NULL)
    {
        /* Start with flags */
        size = sizeof(RwUInt32);

        /* Normal map */
        size += NormalMapStreamSizeTexture(normalMapExt->normalMap);

        /* Env map */
        if (normalMapExt->envMap != NULL)
        {
            /* First we count the coefficient ... */
            size += sizeof(RwReal);

            /* ... and then the texture ... */
            size += NormalMapStreamSizeTexture(normalMapExt->envMap);
        }
    }

    /* And we're spent. */
    RWRETURN(size);
}

/*---------------------------------------------------------------------------*
 *    World: constructor, destructor, copy,                                  *
 *           stream read, stream write, stream size.                         *
 *---------------------------------------------------------------------------*/

static void        *
NormalMapWorldConstructor(void *object,
                            RwInt32 offset __RWUNUSED__,
                            RwInt32 size __RWUNUSED__)
{
    ObjectNormalMapExt *worldData;

    RWFUNCTION(RWSTRING("NormalMapWorldConstructor"));
    RWASSERT(object);

    /* Grab the world plugins data. */
    worldData = WORLDGETNORMALMAP((RpWorld *) object);

    worldData->enabled = FALSE;

    RWRETURN(object);
}

static void        *
NormalMapWorldDestructor(void *object,
                           RwInt32 offset __RWUNUSED__,
                           RwInt32 size __RWUNUSED__)
{
    ObjectNormalMapExt *worldData;

    RWFUNCTION(RWSTRING("NormalMapWorldDestructor"));
    RWASSERT(object);

    /* Grab the world plugins data. */
    worldData = WORLDGETNORMALMAP((RpWorld *) object);

    worldData->enabled = FALSE;

    RWRETURN(object);
}

static void        *
NormalMapWorldCopy(void *dstObject,
                     const void *srcObject,
                     RwInt32 offset __RWUNUSED__,
                     RwInt32 size __RWUNUSED__)
{
    const RpWorld *srcWorld;
    const ObjectNormalMapExt *srcWorldData;

    RpWorld      *dstWorld;
    ObjectNormalMapExt *dstWorldData;

    RWFUNCTION(RWSTRING("NormalMapWorldCopy"));
    RWASSERT(dstObject);
    RWASSERT(srcObject);

    srcWorld = (const RpWorld *) srcObject;
    srcWorldData = CONSTWORLDGETNORMALMAP(srcWorld);

    dstWorld = (RpWorld *) dstObject;
    dstWorldData = WORLDGETNORMALMAP(dstWorld);

    if (srcWorldData->enabled != FALSE &&
        dstWorldData->enabled == FALSE)
    {
        RpNormMapWorldEnable(dstWorld);
    }

    RWRETURN(dstObject);
}

static RwStream *
NormalMapWorldStreamWrite(RwStream   *stream,
                            RwInt32     binaryLength    __RWUNUSEDRELEASE__,
                            const void *object,
                            RwInt32     offsetInObject  __RWUNUSED__,
                            RwInt32     sizeInObject    __RWUNUSED__)
{
    RwStream    *streamOut;
    RwInt32     temp;
    const ObjectNormalMapExt *worldData = CONSTWORLDGETNORMALMAP(object);

    RWFUNCTION(RWSTRING("NormalMapWorldStreamWrite"));
    RWASSERT(stream);
    RWASSERT(object);
    RWASSERT(binaryLength > 0);

    /* Lets write out the extended data. */
    temp = worldData->enabled;
    streamOut = RwStreamWriteInt32(stream, &temp, sizeof(RwInt32));

    RWRETURN(streamOut);
}

static RwStream *
NormalMapWorldStreamRead(RwStream *stream,
                           RwInt32   binaryLength   __RWUNUSEDRELEASE__,
                           void     *object,
                           RwInt32   offsetInObject __RWUNUSED__,
                           RwInt32   sizeInObject   __RWUNUSED__)
{
    RpWorld  *world = (RpWorld *) object;
    RwBool   enabled;

    RWFUNCTION(RWSTRING("NormalMapWorldStreamRead"));
    RWASSERT(stream);
    RWASSERT(object);
    RWASSERT(binaryLength > 0);

    /* Read the worldSector data. */
    if (!RwStreamReadInt32(stream, (RwInt32 *)&enabled, sizeof(RwInt32)))
    {
        RWRETURN((RwStream *)NULL);
    }

    /* Do we need to enable the effects? */
    if (enabled)
    {
        RpNormMapWorldEnable(world);
    }

    RWRETURN(stream);
}

static RwInt32
NormalMapWorldStreamGetSize(const void *object,
                              RwInt32     offsetInObject __RWUNUSED__,
                              RwInt32     sizeInObject   __RWUNUSED__)
{
    const ObjectNormalMapExt *worldData = CONSTWORLDGETNORMALMAP(object);

    RWFUNCTION(RWSTRING("NormalMapWorldStreamGetSize"));
    RWASSERT(object);

    /* Is the world material effects enabled? */
    if (FALSE == worldData->enabled)
    {
        /* Nothing to write */
        RWRETURN(0);
    }

    /* Single enabled flag. */
    RWRETURN(sizeof(ObjectNormalMapExt));
}

/**
 * \ingroup rpnormmap
 * \ref RpNormMapMaterialSetNormMapTexture is used to setup the normal map
 * texture for a material.
 *
 * The world and normal maps plugins must be attached before using
 * this function. The header file rpnormmap.h is required.
 *
 * \param material Pointer to the \ref RpMaterial to setup the normal map texture
 *                 on.
 * \param normalmap Pointer to the \ref RwTexture to use as the normal map.
 *
 * \return The source \ref RpMaterial on success, NULL otherwise
 *
 * \see RpNormMapAtomicInitialize
 * \see RpNormMapWorldEnable
 * \see RpNormMapWorldSectorInitialize
 * \see RpNormMapMaterialSetEnvMapTexture.
 * \see RpNormMapMaterialSetEnvMapCoefficient.
 * \see RpNormMapMaterialGetNormMapTexture
 * \see RpNormMapMaterialSetEnvMapFrame.
 * \see RpNormMapMaterialGetEnvMapTexture.
 * \see RpNormMapMaterialGetEnvMapCoefficient.
 * \see RpNormMapMaterialGetEnvMapFrame.
 * \see RpNormMapMaterialModulateEnvMap.
 * \see RpNormMapMaterialIsEnvMapModulated.
 */
RpMaterial *
RpNormMapMaterialSetNormMapTexture(RpMaterial *material, RwTexture *normalmap)
{
    NormalMapExt  *normalmapext;

    RWAPIFUNCTION(RWSTRING("RpNormMapMaterialSetNormMapTexture"));
    RWASSERT(material != NULL);

    normalmapext = MATERIALGETNORMALMAP(material);

    if (normalmapext->normalMap != normalmap)
    {
        if (normalmapext->normalMap != NULL)
        {
            RwTextureDestroy(normalmapext->normalMap);
        }

        normalmapext->normalMap = normalmap;

        if (normalmapext->normalMap != NULL)
        {
            (void)RwTextureAddRef(normalmapext->normalMap);
        }
    }

    RWRETURN(material);
}

/**
 * \ingroup rpnormmap
 * \ref RpNormMapMaterialGetNormMapTexture is used to retrieve the normal map
 * texture from a material.
 *
 * The world and normal maps plugins must be attached before using
 * this function. The header file rpnormmap.h is required.
 *
 * \param material Pointer to the \ref RpMaterial to retrieve the normal map texture
 *                 from.
 *
 * \return The normal map texture used on success, NULL otherwise.
 *
 * \see RpNormMapAtomicInitialize
 * \see RpNormMapWorldEnable
 * \see RpNormMapWorldSectorInitialize
 * \see RpNormMapMaterialSetNormMapTexture
 * \see RpNormMapMaterialSetEnvMapTexture.
 * \see RpNormMapMaterialSetEnvMapCoefficient.
 * \see RpNormMapMaterialSetEnvMapFrame.
 * \see RpNormMapMaterialGetEnvMapTexture.
 * \see RpNormMapMaterialGetEnvMapCoefficient.
 * \see RpNormMapMaterialGetEnvMapFrame.
 * \see RpNormMapMaterialModulateEnvMap.
 * \see RpNormMapMaterialIsEnvMapModulated.
 */
RwTexture *
RpNormMapMaterialGetNormMapTexture(const RpMaterial *material)
{
    const NormalMapExt  *normalmapext;

    RWAPIFUNCTION(RWSTRING("RpNormMapMaterialGetNormMapTexture"));
    RWASSERT(material != NULL);

    normalmapext = CONSTMATERIALGETNORMALMAP(material);

    RWRETURN(normalmapext->normalMap);
}

/**
 * \ingroup rpnormmap
 * \ref RpNormMapMaterialSetEnvMapTexture sets the texture for a environment
 * mapping material effect using the normal map. The texture is rendered with
 * dynamically generated UVs using a spherical environment mapping.
 *
 * The world and normal maps plugins must be attached before using
 * this function. The header file rpnormmap.h is required.
 *
 * \param material Pointer to the \ref RpMaterial.
 * \param envmap   Pointer to the \ref RwTexture for the environment map.
 * Environment mapping is disabled if the pointer is NULL.
 *
 * \return The material used on success, NULL otherwise.
 *
 * \see RpNormMapAtomicInitialize
 * \see RpNormMapWorldEnable
 * \see RpNormMapWorldSectorInitialize
 * \see RpNormMapMaterialSetNormMapTexture
 * \see RpNormMapMaterialSetEnvMapCoefficient.
 * \see RpNormMapMaterialSetEnvMapFrame.
 * \see RpNormMapMaterialGetNormMapTexture
 * \see RpNormMapMaterialGetEnvMapTexture.
 * \see RpNormMapMaterialGetEnvMapCoefficient.
 * \see RpNormMapMaterialGetEnvMapFrame.
 * \see RpNormMapMaterialModulateEnvMap.
 * \see RpNormMapMaterialIsEnvMapModulated.
 */
RpMaterial *
RpNormMapMaterialSetEnvMapTexture(RpMaterial *material, RwTexture *envmap)
{
    NormalMapExt  *normalmapext;

    RWAPIFUNCTION(RWSTRING("RpNormMapMaterialSetEnvMapTexture"));
    RWASSERT(material != NULL);

    normalmapext = MATERIALGETNORMALMAP(material);

    if (normalmapext->envMap != envmap)
    {
        if (normalmapext->envMap != NULL)
        {
            RwTextureDestroy(normalmapext->envMap);
        }

        normalmapext->envMap = envmap;

        if (normalmapext->envMap != NULL)
        {
            (void)RwTextureAddRef(normalmapext->envMap);
        }
    }

    RWRETURN(material);
}

/**
 * \ingroup rpnormmap
 * \ref RpNormMapMaterialGetEnvMapTexture is used to retrieve the texture used
 * for the environment map effect.
 *
 * The world and normal maps plugins must be attached before using
 * this function. The header file rpnormmap.h is required.
 *
 * \param material A pointer to a \ref RpMaterial.
 *
 * \return Returns a pointer to the environment map's \ref RwTexture.
 *
 * \see RpNormMapAtomicInitialize
 * \see RpNormMapWorldEnable
 * \see RpNormMapWorldSectorInitialize
 * \see RpNormMapMaterialSetNormMapTexture
 * \see RpNormMapMaterialSetEnvMapTexture.
 * \see RpNormMapMaterialSetEnvMapCoefficient.
 * \see RpNormMapMaterialSetEnvMapFrame.
 * \see RpNormMapMaterialGetNormMapTexture
 * \see RpNormMapMaterialGetEnvMapCoefficient.
 * \see RpNormMapMaterialGetEnvMapFrame.
 * \see RpNormMapMaterialModulateEnvMap.
 * \see RpNormMapMaterialIsEnvMapModulated.
 */
RwTexture *
RpNormMapMaterialGetEnvMapTexture(const RpMaterial *material)
{
    const NormalMapExt  *normalmapext;

    RWAPIFUNCTION(RWSTRING("RpNormMapMaterialGetEnvMapTexture"));
    RWASSERT(material != NULL);

    normalmapext = CONSTMATERIALGETNORMALMAP(material);

    RWRETURN(normalmapext->envMap);
}

/**
 * \ingroup rpnormmap
 * \ref RpNormMapMaterialSetEnvMapCoefficient affects the reflectivity of the
 * material, i.e. the intensity of the environment map.
 *
 * The world and normal maps plugins must be attached before using
 * this function. The header file rpnormmap.h is required.
 *
 * \param material Pointer to the \ref RpMaterial.
 * \param coef     \ref RwReal representing the reflection coefficient of
 *                 the material
 *
 * \return The material used on success, NULL otherwise.
 *
 * \see RpNormMapAtomicInitialize
 * \see RpNormMapWorldEnable
 * \see RpNormMapWorldSectorInitialize
 * \see RpNormMapMaterialSetNormMapTexture
 * \see RpNormMapMaterialSetEnvMapTexture.
 * \see RpNormMapMaterialSetEnvMapFrame.
 * \see RpNormMapMaterialGetNormMapTexture
 * \see RpNormMapMaterialGetEnvMapTexture.
 * \see RpNormMapMaterialGetEnvMapCoefficient.
 * \see RpNormMapMaterialGetEnvMapFrame.
 * \see RpNormMapMaterialModulateEnvMap.
 * \see RpNormMapMaterialIsEnvMapModulated.
 */
RpMaterial *
RpNormMapMaterialSetEnvMapCoefficient(RpMaterial *material, RwReal coef)
{
    NormalMapExt  *normalmapext;

    RWAPIFUNCTION(RWSTRING("RpNormMapMaterialSetEnvMapCoefficient"));
    RWASSERT(material != NULL);

    normalmapext = MATERIALGETNORMALMAP(material);

    normalmapext->shininess = coef;

    RWRETURN(material);
}

/**
 * \ingroup rpnormmap
 * \ref RpNormMapMaterialGetEnvMapCoefficient is used to retrieve the
 * environment coefficient.
 *
 * The world and normal maps plugins must be attached before using
 * this function. The header file rpnormmap.h is required.
 *
 * \param material A const pointer to a \ref RpMaterial.
 *
 * \return Returns environment coefficient value.
 *
 * \see RpNormMapAtomicInitialize
 * \see RpNormMapWorldEnable
 * \see RpNormMapWorldSectorInitialize
 * \see RpNormMapMaterialSetNormMapTexture
 * \see RpNormMapMaterialSetEnvMapTexture.
 * \see RpNormMapMaterialSetEnvMapFrame.
 * \see RpNormMapMaterialSetEnvMapCoefficient.
 * \see RpNormMapMaterialGetNormMapTexture
 * \see RpNormMapMaterialGetEnvMapTexture.
 * \see RpNormMapMaterialGetEnvMapFrame.
 * \see RpNormMapMaterialModulateEnvMap.
 * \see RpNormMapMaterialIsEnvMapModulated.
 */
RwReal
RpNormMapMaterialGetEnvMapCoefficient(const RpMaterial *material)
{
    const NormalMapExt  *normalmapext;

    RWAPIFUNCTION(RWSTRING("RpNormMapMaterialGetEnvMapCoefficient"));
    RWASSERT(material != NULL);

    normalmapext = CONSTMATERIALGETNORMALMAP(material);

    RWRETURN(normalmapext->shininess);
}


/**
 * \ingroup rpnormmap
 * \ref RpNormMapMaterialSetEnvMapFrame sets the frame associated with the
 * environment mapping material effect. This will affect the position of
 * the environment map on the object.
 *
 * The world and normal maps plugins must be attached before using
 * this function. The header file rpnormmap.h is required.
 *
 * \param material Pointer to the \ref RpMaterial.
 * \param frame    Pointer to the \ref RwFrame affecting environment mapping.
 *
 * \return The material used on success, NULL otherwise.
 *
 * \see RpNormMapAtomicInitialize
 * \see RpNormMapWorldEnable
 * \see RpNormMapWorldSectorInitialize
 * \see RpNormMapMaterialSetNormMapTexture
 * \see RpNormMapMaterialSetEnvMapTexture.
 * \see RpNormMapMaterialSetEnvMapCoefficient.
 * \see RpNormMapMaterialGetNormMapTexture
 * \see RpNormMapMaterialGetEnvMapTexture.
 * \see RpNormMapMaterialGetEnvMapCoefficient.
 * \see RpNormMapMaterialGetEnvMapFrame.
 * \see RpNormMapMaterialModulateEnvMap.
 * \see RpNormMapMaterialIsEnvMapModulated.
 */
RpMaterial *
RpNormMapMaterialSetEnvMapFrame(RpMaterial *material, RwFrame *frame)
{
    NormalMapExt  *normalmapext;

    RWAPIFUNCTION(RWSTRING("RpNormMapMaterialSetEnvMapFrame"));
    RWASSERT(material != NULL);

    normalmapext = MATERIALGETNORMALMAP(material);

    normalmapext->envmapFrame = frame;

    RWRETURN(material);
}

/**
 * \ingroup rpnormmap
 * \ref RpNormMapMaterialGetEnvMapFrame is used to retrieve the \ref RwFrame
 * used for projecting the environment map.
 *
 * The world and normal maps plugins must be attached before using
 * this function. The header file rpnormmap.h is required.
 *
 * \param material Pointer to a \ref RpMaterial.
 *
 * \return Returns a pointer to the environment map's \ref RwFrame.
 *
 * \see RpNormMapAtomicInitialize
 * \see RpNormMapWorldEnable
 * \see RpNormMapWorldSectorInitialize
 * \see RpNormMapMaterialSetNormMapTexture
 * \see RpNormMapMaterialSetEnvMapTexture.
 * \see RpNormMapMaterialSetEnvMapCoefficient.
 * \see RpNormMapMaterialSetEnvMapFrame.
 * \see RpNormMapMaterialGetNormMapTexture
 * \see RpNormMapMaterialGetEnvMapTexture.
 * \see RpNormMapMaterialGetEnvMapCoefficient.
 * \see RpNormMapMaterialModulateEnvMap.
 * \see RpNormMapMaterialIsEnvMapModulated.
 */
RwFrame *
RpNormMapMaterialGetEnvMapFrame(RpMaterial *material)
{
    NormalMapExt  *normalmapext;

    RWAPIFUNCTION(RWSTRING("RpNormMapMaterialGetEnvMapFrame"));
    RWASSERT(material != NULL);

    normalmapext = MATERIALGETNORMALMAP(material);

    RWRETURN(normalmapext->envmapFrame);
}

/**
 * \ingroup rpnormmap
 * \ref RpNormMapMaterialModulateEnvMap forces the environment map to be modulated
 * by the base texture before applying it, when \e modulate is TRUE. By default the modulation
 * is disabled. Modulating the environment map by the base texture may be used,
 * for example, to simulate metallic materials.
 *
 * The world and normal maps plugins must be attached before using
 * this function. The header file rpnormmap.h is required.
 *
 * \param material Pointer to the \ref RpMaterial.
 * \param modulate \ref RwBool indicates whether or not the environment map
 * will be modulated with the base texture.
 *
 * \return The material used on success, NULL otherwise.
 *
 * \see RpNormMapAtomicInitialize
 * \see RpNormMapWorldEnable
 * \see RpNormMapWorldSectorInitialize
 * \see RpNormMapMaterialSetNormMapTexture
 * \see RpNormMapMaterialSetEnvMapTexture.
 * \see RpNormMapMaterialSetEnvMapFrame.
 * \see RpNormMapMaterialSetEnvMapCoefficient.
 * \see RpNormMapMaterialGetNormMapTexture
 * \see RpNormMapMaterialGetEnvMapTexture.
 * \see RpNormMapMaterialGetEnvMapCoefficient.
 * \see RpNormMapMaterialGetEnvMapFrame.
 * \see RpNormMapMaterialIsEnvMapModulated.
 */
RpMaterial *
RpNormMapMaterialModulateEnvMap(RpMaterial *material, RwBool modulate)
{
    NormalMapExt  *normalmapext;

    RWAPIFUNCTION(RWSTRING("RpNormMapMaterialModulateEnvMap"));
    RWASSERT(material != NULL);

    normalmapext = MATERIALGETNORMALMAP(material);

    normalmapext->modulateEnvMap = modulate;

    RWRETURN(material);
}

/**
 * \ingroup rpnormmap
 * \ref RpNormMapMaterialIsEnvMapModulated determines whether the
 * environment map is modulate by the base texture before applying it.
 *
 * The world and normal maps plugins must be attached before using
 * this function. The header file rpnormmap.h is required.
 *
 * \param material Const pointer to a \ref RpMaterial.
 *
 * \return TRUE if the modulation of the environment map is enabled
 * for this material, FALSE otherwise.
 *
 * \see RpNormMapAtomicInitialize
 * \see RpNormMapWorldEnable
 * \see RpNormMapWorldSectorInitialize
 * \see RpNormMapMaterialSetNormMapTexture
 * \see RpNormMapMaterialSetEnvMapTexture.
 * \see RpNormMapMaterialSetEnvMapFrame.
 * \see RpNormMapMaterialSetEnvMapCoefficient.
 * \see RpNormMapMaterialGetNormMapTexture
 * \see RpNormMapMaterialGetEnvMapTexture.
 * \see RpNormMapMaterialGetEnvMapCoefficient.
 * \see RpNormMapMaterialGetEnvMapFrame.
 * \see RpNormMapMaterialModulateEnvMap.
 */
RwBool
RpNormMapMaterialIsEnvMapModulated(const RpMaterial *material)
{
    const NormalMapExt  *normalmapext;

    RWAPIFUNCTION(RWSTRING("RpNormMapMaterialIsEnvMapModulated"));
    RWASSERT(material != NULL);

    normalmapext = CONSTMATERIALGETNORMALMAP(material);

    RWRETURN(normalmapext->modulateEnvMap);
}

/*
 ***************************************************************************
 */
static RwBool
NormalMapAtomicRights( void *object,
                  RwInt32 offset __RWUNUSED__,
                  RwInt32 size   __RWUNUSED__,
                  RwUInt32 extraData )
{
    RpAtomic *atomic;
    RpNormMapAtomicPipeline type;

    RWFUNCTION(RWSTRING("NormalMapAtomicRights"));
    RWASSERT(NULL != object);

    atomic = (RpAtomic *)object;
    RWASSERT(NULL != atomic);
    type = (RpNormMapAtomicPipeline)extraData;

    RpNormMapAtomicInitialize(atomic, type);

    RWRETURN(TRUE);
}

/*
 ***************************************************************************
 */
static RwBool
NormalMapWorldSectorRights( void *object,
                  RwInt32 offset __RWUNUSED__,
                  RwInt32 size   __RWUNUSED__,
                  RwUInt32 extraData __RWUNUSEDRELEASE__)
{
    RpWorldSector *worldsector;

    RWFUNCTION(RWSTRING("NormalMapWorldSectorRights"));
    RWASSERT(NULL != object);
    RWASSERT(0 == extraData);

    worldsector = (RpWorldSector *)object;

    RpWorldSectorSetPipeline(worldsector, _rpNormMapWorldSectorPipeline);

    RWRETURN(TRUE);
}

/**
 * \ingroup rpnormmap
 * \ref RpNormMapPluginAttach is called by the application to indicate that the
 * normal maps plugin should be used. The call to this function should be
 * placed between \ref RwEngineInit and \ref RwEngineOpen and the world plugin
 * must already be attached.
 *
 * \warning The include files rpnormmap.h and rpworld.h are also required and
 * must be included by an application wishing to use normal maps.
 *
 * \warning The normal maps plugin requires
 * \li the normal maps plugin rpnormmap.lib and
 * \li the world plugin rpworld.lib
 * to be linked into the application and attached at run time.
 *
 * \return True on success, false otherwise
 *
 * \see RpWorldPluginAttach
 * \see RpNormMapAtomicInitialize
 * \see RpNormMapWorldEnable
 * \see RpNormMapWorldSectorInitialize
 * \see RpNormMapMaterialSetNormMapTexture
 */
RwBool
RpNormMapPluginAttach(void)
{
    RwInt32 offset;

    RWAPIFUNCTION(RWSTRING("RpNormMapPluginAttach"));

    /* Register the plugIn */
    if (RwEngineRegisterPlugin(0, (RwUInt32)rwPLUGIN_ID,
                               _rpNormMapOpen, _rpNormMapClose) < 0)
    {
        RWRETURN(FALSE);
    }

    /*
     * Register the material plugin
     */
    _rpNormMapMaterialExOffset = RpMaterialRegisterPlugin(sizeof(NormalMapExt),
                                               (RwUInt32)rwPLUGIN_ID,
                                               NormalMapConstruct,
                                               NormalMapDestruct,
                                               NormalMapCopy);
    if (_rpNormMapMaterialExOffset == 0)
    {
        RWRETURN(FALSE);
    }

    offset =
        RpMaterialRegisterPluginStream((RwUInt32)rwPLUGIN_ID,
                                       NormalMapMaterialStreamRead,
                                       NormalMapMaterialStreamWrite,
                                       NormalMapMaterialStreamGetSize);
    if (offset < 0)
    {
        RWRETURN(FALSE);
    }

    /* Register atomic plugIn */
    _rpNormMapAtomicExOffset =
        RpAtomicRegisterPlugin(0,
                               (RwUInt32)rwPLUGIN_ID,
                               NULL,
                               NULL,
                               NULL);

    if (_rpNormMapAtomicExOffset < 0)
    {
        RWRETURN(FALSE);
    }

    /* Attach a rights callback streaming function. */
    offset = RpAtomicSetStreamRightsCallBack( rwPLUGIN_ID,
                                               NormalMapAtomicRights );
    RWASSERT(0 < offset);

    /* Register worldSector plugIn */
    _rpNormMapWorldExOffset =
        RpWorldRegisterPlugin(sizeof(ObjectNormalMapExt),
                                    (RwUInt32)rwPLUGIN_ID,
                                    NormalMapWorldConstructor,
                                    NormalMapWorldDestructor,
                                    NormalMapWorldCopy);

    if (_rpNormMapWorldExOffset < 0)
    {
        RWRETURN(FALSE);
    }

    offset =
        RpWorldRegisterPluginStream((RwUInt32)rwPLUGIN_ID,
                                          NormalMapWorldStreamRead,
                                          NormalMapWorldStreamWrite,
                                          NormalMapWorldStreamGetSize);
    if (offset < 0)
    {
        RWRETURN(FALSE);
    }

    /* Attach a rights callback streaming function. */
    offset = RpWorldSectorRegisterPlugin(0,
                                         (RwUInt32)rwPLUGIN_ID,
                                         NULL,
                                         NULL,
                                         NULL);
    if (offset < 0)
    {
        RWRETURN(FALSE);
    }

    offset = RpWorldSectorSetStreamRightsCallBack(rwPLUGIN_ID,
                                                  NormalMapWorldSectorRights);
    if (offset < 0)
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}

/**
 * \ingroup rpnormmap
 * \ref RpNormMapAtomicIsInitialized is used to check if the normal maps plugin
 * has been initialized on an atomic.
 *
 * The world and normal maps plugins must be attached before using
 * this function. The header file rpnormmap.h is required.
 *
 * This function must be used \e before the \ref RpAtomic is
 * rendered or pre-instanced.
 *
 * \param atomic Pointer to the \ref RpAtomic.
 *
 * \return True if the normal maps plugin has been initialized on the atomic,
 * false otherwise.
 *
 * \see RpNormMapPluginAttach
 * \see RpNormMapAtomicInitialize
 * \see RpNormMapWorldEnable
 * \see RpNormMapWorldSectorInitialize
 * \see RpNormMapWorldIsEnabled
 * \see RpNormMapMaterialSetNormMapTexture
 */
RwBool
RpNormMapAtomicIsInitialized(const RpAtomic *atomic)
{
    RWAPIFUNCTION(RWSTRING("RpNormMapAtomicIsInitialized"));
    RWASSERT(atomic);

    RWRETURN(atomic->pipeline != NULL &&
             atomic->pipeline->pluginId == rwPLUGIN_ID);
}

/**
 * \ingroup rpnormmap
 * \ref RpNormMapWorldSectorInitialize is used to initialize the normal maps
 * object pipeline on a world sector.
 *
 * The world and normal maps plugins must be attached before using
 * this function. The header file rpnormmap.h is required.
 *
 * \param worldsector Pointer to the world sector to initialize the normal maps pipeline on.
 *
 * \return True if the normal maps plugin has been initialized on the worldsector,
 * false otherwise.
 *
 * \see RpNormMapPluginAttach
 * \see RpNormMapAtomicInitialize
 * \see RpNormMapAtomicIsInitialized
 * \see RpNormMapWorldEnable
 * \see RpNormMapWorldIsEnabled
 * \see RpNormMapWorldSectorIsInitialized
 * \see RpNormMapMaterialSetNormMapTexture
 */
void
RpNormMapWorldSectorInitialize(RpWorldSector *worldsector)
{
    const RpWorld *world;
    const ObjectNormalMapExt *worldData;

    RWAPIFUNCTION(RWSTRING("RpNormMapWorldSectorInitialize"));
    RWASSERT(worldsector != NULL);

    world = RpWorldSectorGetWorld(worldsector);
    worldData = CONSTWORLDGETNORMALMAP(world);
    RWASSERT(worldData->enabled != FALSE);

    if (worldData->enabled)
    {
        RpWorldSectorSetPipeline(worldsector, _rpNormMapWorldSectorPipeline);
    }

    RWRETURNVOID();
}

/**
 * \ingroup rpnormmap
 * \ref RpNormMapWorldSectorIsInitialized is used to check if the normal maps plugin
 * has been initialized on a world sector.
 *
 * The world and normal maps plugins must be attached before using
 * this function. The header file rpnormmap.h is required.
 *
 * \param worldsector Const pointer to the \ref RpWorldSector.
 *
 * \return True if the normal maps plugin has been initialized on the world sector,
 * false otherwise.
 *
 * \see RpNormMapPluginAttach
 * \see RpNormMapAtomicInitialize
 * \see RpNormMapAtomicIsInitialized
 * \see RpNormMapWorldEnable
 * \see RpNormMapWorldIsEnabled
 * \see RpNormMapWorldSectorInitialize
 * \see RpNormMapMaterialSetNormMapTexture
 */
RwBool
RpNormMapWorldSectorIsInitialized(const RpWorldSector *worldsector)
{
    const RpWorld *world;
    const ObjectNormalMapExt *worldData;

    RWAPIFUNCTION(RWSTRING("RpNormMapWorldSectorIsInitialized"));
    RWASSERT(worldsector != NULL);

    world = RpWorldSectorGetWorld(worldsector);
    worldData = CONSTWORLDGETNORMALMAP(world);

    RWRETURN(worldData->enabled &&
             worldsector->pipeline == _rpNormMapWorldSectorPipeline);
}

/**
 * \ingroup rpnormmap
 * \ref RpNormMapWorldIsEnabled is used to check if the normal maps plugin
 * has been enabled on a world.
 *
 * The world and normal maps plugins must be attached before using
 * this function. The header file rpnormmap.h is required.
 *
 * \param world Pointer to the world.
 *
 * \see RpNormMapPluginAttach
 * \see RpNormMapAtomicInitialize
 * \see RpNormMapAtomicIsInitialized
 * \see RpNormMapWorldEnable
 * \see RpNormMapWorldSectorInitialize
 * \see RpNormMapMaterialSetNormMapTexture
 */
RwBool
RpNormMapWorldIsEnabled(const RpWorld *world)
{
    const ObjectNormalMapExt *worldData = CONSTWORLDGETNORMALMAP(world);

    RWAPIFUNCTION(RWSTRING("RpNormMapWorldIsEnabled"));
    RWASSERT(world);

    RWRETURN(worldData->enabled);
}

/**
 * \ingroup rpnormmap
 * \ref RpNormMapGetWorldSectorPipeline returns the \ref rpnormmap
 * internal world sector pipeline.
 *
 * The \ref rpnormmap plugin must be attached before using this function.
 *
 * \return The \ref RxPipeline requested from the plugin, or
 * NULL if the pipeline wasn't constructed.
 *
 * \see RpNormMapPluginAttach
 * \see RpNormMapAtomicInitialize
 * \see RpNormMapWorldIsEnabled
 * \see RpNormMapGetAtomicPipeline
 */
RxPipeline *
RpNormMapGetWorldSectorPipeline(void)
{
    RWAPIFUNCTION(RWSTRING("RpNormMapGetWorldSectorPipeline"));

    RWRETURN(_rpNormMapWorldSectorPipeline);
}

/**
 * \ingroup rpnormmap
 * \ref RpNormMapSetActiveLights is called to define the
 * ambient and the main light the normal maps plugin should use to light the
 * atomics or the world sectors. The normal maps plugin only supports a custom
 * light mode of a single main light and a single ambient light.
 *
 * If the parameters passed to this function are \e not NULL, then any
 * lights added to the world will be ignored whilst using the normal maps
 * pipelines.
 *
 * \param mainLight Pointer to the main light.
 * \param ambientLight Pointer to the ambient light.
 *
 * \see RpNormMapPluginAttach
 * \see RpNormMapAtomicInitialize
 * \see RpNormMapWorldIsEnabled
 * \see RpNormMapGetAtomicPipeline
 * \see RpNormMapGetWorldSectorPipeline
 */
void
RpNormMapSetActiveLights(RpLight *mainLight, RpLight *ambientLight)
{
    RWAPIFUNCTION(RWSTRING("RpNormMapSetActiveLights"));

    _rpNormMapMainLightActive = mainLight;

    _rpNormMapAmbientLightActive = ambientLight;

    RWRETURNVOID();
}
