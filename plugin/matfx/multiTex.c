/*
 *  multiTex.c
 *
 *  Multi-texturing extension to materials. Specific platforms only.
 */

/**
 * \defgroup rpmultitex Multi-Texturing
 * \ingroup materials
 *
 * Multi-texturing extension to materials
 */

/**
 *
 * \ingroup rpmultitex
 * \page rpmatfxexoverview Multi-Texturing Extension Overview
 *
 * \par What is the RpMatFX Multi-Texturing Extension?
 *
 * The RpMatFX Multi-Texturing Extension allows access to hardware specific
 * features helping the use of multi-texturing and texture coordinates generation
 * on platforms that support it (currently, Xbox and GameCube).
 *
 * This section deals with multi-texturing functions common to both GameCube and
 * Xbox, such as the setup of texture arrays on materials (see \ref
 * RpMaterialCreateMultiTexture). The blending of the textures is described
 * by a multi-texture effect which may be assigned to a material.
 * The low-level details of these effects are platform
 * specific and are described in the GameCube or Xbox section, but the
 * high-level management of the effects is handled by a common system.
 * This allows effects to be named, added to dictionaries, and streamed in
 * and out.
 *
 * The high-level management is available in null builds so that
 * a texture array may be set up from an exporter tool, and an effect
 * associated using its name only.
 *
 * Full low-level streaming and access to the effect settings is only
 * available in the platform specific libraries (eg gcn and nullgcn).
 */


/******************************************************************************
 *  Includes
 */
#include <string.h>

#include "rpplugin.h"
#include "rpdbgerr.h"
#include "rwcore.h"
#include "rpworld.h"

#include "matfx.h"
#include "multiTex.h"
#include "multiTexEffect.h"


/******************************************************************************
 *  Defines
 */
#define MAXPLATFORMID  (rwID_PCD3D9)


/******************************************************************************
 *  Local types
 */

typedef struct _MultiTextureExt
{
    RpMultiTexture  *multiTexture;

} MultiTextureExt;

typedef enum _MultiTextureStreamFlag
{
    MULTITEXTURESTREAMEFFECT = 0x01

} MultiTextureStreamFlag;

typedef struct _MultiTextureStreamHdr
{
    RwUInt8     platformID;
    RwUInt8     numTextures;
    RwUInt8     flags;
    RwUInt8     pad;

} MultiTextureStreamHdr;


/******************************************************************************
 *  Local variables
 */

/*
 *  We can manage multiple platform multi-texture plugins, and store the
 *  registration details here. The platformID is used as the array index.
 */
rpMultiTextureRegEntry  RegEntries[MAXPLATFORMID+1];

RwModuleInfo  _rpMultiTextureModule = {0, 0};

/******************************************************************************
 *  Macros
 */
#define GetMultiTextureExtMacro(material, offset)   \
    ((MultiTextureExt *)((RwUInt8 *)material + offset))

#define GetConstMultiTextureExtMacro(material, offset)   \
    ((const MultiTextureExt *)((const RwUInt8 *)material + offset))


/*
 *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *      Local functions
 *
 *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */


static void *
MultiTextureOpen(void    *object        __RWUNUSED__,
                 RwInt32 offsetInObject __RWUNUSED__,
                 RwInt32 sizeInObject   __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("MultiTextureOpen"));

    ++_rpMultiTextureModule.numInstances;

    _rpMTEffectOpen();

    RWRETURN(object);
}


static void *
MultiTextureClose(void    *object        __RWUNUSED__,
                  RwInt32 offsetInObject __RWUNUSED__,
                  RwInt32 sizeInObject   __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("MultiTextureClose"));

    _rpMTEffectClose();

    --_rpMultiTextureModule.numInstances;

    RWRETURN(object);
}


/******************************************************************************
 */
static RpMultiTexture *
MultiTextureCreate(rpMultiTextureRegEntry *regEntry, RwUInt32 numTextures)
{
    RwUInt32        size;
    RpMultiTexture *multiTexture;

    RWFUNCTION(RWSTRING("MultiTextureCreate"));
    RWASSERT(regEntry);

    /* Allocate memory */
    size = sizeof(RpMultiTexture) + regEntry->extensionSize;
    multiTexture = (RpMultiTexture *) RwMalloc(size,
        rwID_MULTITEXPLUGIN | rwMEMHINTDUR_EVENT | rwMEMHINTFLAG_RESIZABLE);
    if (!multiTexture)
    {
        RWERROR((E_RW_NOMEM, size));
        RWRETURN((RpMultiTexture *) NULL);
    }

    /* Initialize */
    memset(multiTexture, 0, size);
    multiTexture->regEntry = regEntry;
    multiTexture->numTextures = numTextures;
    if (regEntry->extensionSize > 0)
    {
        multiTexture->extension = (void *)(multiTexture + 1);
    }

    RWRETURN(multiTexture);
}


/******************************************************************************
 */
static void
MultiTextureDestroy(RpMultiTexture *multiTexture)
{
    RwUInt32    i;

    RWFUNCTION(RWSTRING("MultiTextureDestroy"));

    for (i = 0; i < (RwUInt32)multiTexture->numTextures; ++i)
    {
        if (multiTexture->textures[i])
        {
            RwTextureDestroy(multiTexture->textures[i]);
            multiTexture->textures[i] = (RwTexture *) NULL;
        }
    }

    if (multiTexture->effect)
    {
        RpMTEffectDestroy(multiTexture->effect);
        multiTexture->effect = (RpMTEffect *) NULL;
    }

    RwFree(multiTexture);

    RWRETURNVOID();
}


/******************************************************************************
 */
static void *
MultiTextureConstructor(void    *object,
                        RwInt32  offset,
                        RwInt32  size   __RWUNUSED__)
{
    MultiTextureExt *ext = GetMultiTextureExtMacro(object, offset);

    RWFUNCTION(RWSTRING("MultiTextureConstructor"));
    RWASSERT(object);

    ext->multiTexture = (RpMultiTexture *) NULL;

    RWRETURN(object);
}


/******************************************************************************
 */
static void *
MultiTextureDestructor(void    *object,
                       RwInt32  offset,
                       RwInt32  size __RWUNUSED__)
{
    MultiTextureExt *ext = GetMultiTextureExtMacro(object, offset);

    RWFUNCTION(RWSTRING("MultiTextureDestructor"));
    RWASSERT(object);

    if (ext->multiTexture)
    {
        MultiTextureDestroy(ext->multiTexture);
        ext->multiTexture = (RpMultiTexture *) NULL;
    }

    RWRETURN(object);
}


/******************************************************************************
 */
static void *
MultiTextureCopy(void       *dstObject,
                 const void *srcObject,
                 RwInt32     offset,
                 RwInt32     size   __RWUNUSED__)
{
    RpMultiTexture const   *srcMT;
    RpMultiTexture         *dstMT;
    RwUInt32                i;

    RWFUNCTION(RWSTRING("MultiTextureCopy"));
    RWASSERT(dstObject);
    RWASSERT(srcObject);

    /* Quit if nothing to copy */
    srcMT = GetConstMultiTextureExtMacro(srcObject, offset)->multiTexture;
    if (!srcMT)
    {
        RWRETURN(dstObject);
    }

    /* Create destination multi texture */
    dstMT = MultiTextureCreate(srcMT->regEntry, srcMT->numTextures);
    if (!dstMT)
    {
        RWRETURN(NULL);
    }

    GetMultiTextureExtMacro(dstObject, offset)->multiTexture = dstMT;

    /* Copy textures */
    for (i = 0; i < srcMT->numTextures; i++)
    {
        RpMultiTextureSetTexture(dstMT, i, RpMultiTextureGetTexture(srcMT, i));
        RpMultiTextureSetCoords(dstMT, i, RpMultiTextureGetCoords(srcMT, i));
    }

    /* Copy effect */
    RpMultiTextureSetEffect(dstMT, RpMultiTextureGetEffect(srcMT));

    RWRETURN(dstObject);
}


/******************************************************************************
 */
static RwInt32
MultiTextureStreamGetSize(const void *object,
                          RwInt32     offset,
                          RwInt32     sizeInObj __RWUNUSED__)
{
    RpMultiTexture *multiTexture;
    RwUInt32        size = 0;

    RWFUNCTION(RWSTRING("MultiTextureStreamGetSize"));
    RWASSERT(object);

    multiTexture = GetConstMultiTextureExtMacro(object, offset)->multiTexture;
    if (multiTexture)
    {
        RwUInt32    i;

        /* Version + header */
        size += sizeof(RwInt32) + sizeof(MultiTextureStreamHdr);

        /* Tex Coord map */
        size += multiTexture->numTextures;

        /* Textures */
        for (i = 0; i < multiTexture->numTextures; ++i)
        {
            /* This automatically deals with null textures */
            size += _rpMatFXStreamSizeTexture(multiTexture->textures[i]);
        }

        /* Effect */
        if (multiTexture->effect)
        {
            size += rwCHUNKHEADERSIZE
                 + _rwStringStreamGetSize(multiTexture->effect->name);
        }
    }

    RWRETURN(size);
}


/******************************************************************************
 */
static RwStream *
MultiTextureStreamWrite(RwStream   *stream,
                        RwInt32     length    __RWUNUSED__,
                        const void *object,
                        RwInt32     offset,
                        RwInt32     sizeInObj __RWUNUSED__)
{
    RpMultiTexture  *multiTexture;

    RWFUNCTION(RWSTRING("MultiTextureStreamWrite"));
    RWASSERT(stream);
    RWASSERT(object);

    multiTexture = GetConstMultiTextureExtMacro(object, offset)->multiTexture;
    if (multiTexture)
    {
        RwUInt32                version;
        MultiTextureStreamHdr   header;
        RwUInt32                i;

        /* Write version */
        version = RwEngineGetVersion();
        if (!RwStreamWriteInt32(stream, (RwInt32 *)&version, sizeof(RwInt32)))
        {
            RWRETURN((RwStream *) NULL);
        }

        /* Write header */
        header.platformID = (RwUInt8) multiTexture->regEntry->platformID;
        header.numTextures = (RwUInt8) multiTexture->numTextures;
        header.flags = multiTexture->effect ? MULTITEXTURESTREAMEFFECT : 0;
        header.pad = 0;
        if (!RwStreamWrite(stream, &header, sizeof(header)))
        {
            RWRETURN((RwStream *) NULL);
        }

        /* Write textures */
        if (multiTexture->numTextures)
        {
            if (!RwStreamWrite(stream,
                    &multiTexture->coordMap, multiTexture->numTextures))
            {
                RWRETURN((RwStream *) NULL);
            }

            for (i = 0; i < multiTexture->numTextures; ++i)
            {
                /* This automatically deals with null textures */
                _rpMatFXStreamWriteTexture(stream, multiTexture->textures[i]);
            }
        }

        /* Write the name of the effect */
        if (multiTexture->effect &&
            !_rwStringStreamWrite(multiTexture->effect->name, stream))
        {
            RWRETURN((RwStream *) NULL);
        }
    }

    RWRETURN(stream);
}


/******************************************************************************
 */
static RwStream *
MultiTextureStreamRead(RwStream *stream,
                       RwInt32   length    __RWUNUSED__,
                       void     *object,
                       RwInt32   offset,
                       RwInt32   sizeInObj __RWUNUSED__)
{
    RwUInt32                version;
    MultiTextureStreamHdr   header;
    rpMultiTextureRegEntry *regEntry;
    RpMultiTexture         *multiTexture;

    RWFUNCTION(RWSTRING("MultiTextureStreamRead"));
    RWASSERT(stream);
    RWASSERT(object);

    /* Read version */
    if (!RwStreamReadInt32(stream, (RwInt32 *)&version, sizeof(RwInt32)))
    {
        RWRETURN((RwStream *) NULL);
    }
    RWASSERT(version <= RwEngineGetVersion());

    /* Read header */
    if (!RwStreamRead(stream, &header, sizeof(header)))
    {
        RWRETURN((RwStream *) NULL);
    }

    /* Get the registry entry which must exist if we ever got here */
    RWASSERT(header.platformID <= MAXPLATFORMID);
    regEntry = &RegEntries[header.platformID];
    RWASSERT(regEntry->pluginID);

    /* Create the multitexture */
    multiTexture = MultiTextureCreate(regEntry, (RwUInt32)header.numTextures);
    if (!multiTexture)
    {
        RWRETURN((RwStream *) NULL);
    }

    /* Read textures */
    if (header.numTextures)
    {
        RwUInt32 i;

        if (!RwStreamRead(stream,
                &multiTexture->coordMap, multiTexture->numTextures))
        {
            MultiTextureDestroy(multiTexture);
            RWRETURN((RwStream *) NULL);
        }

        for (i = 0; i < multiTexture->numTextures; ++i)
        {
            /* This automatically deals with null textures */
            if (!_rpMatFXStreamReadTexture(stream, &multiTexture->textures[i]))
            {
                MultiTextureDestroy(multiTexture);
                RWRETURN((RwStream *) NULL);
            }
        }
    }

    /* Read the effect */
    if (MULTITEXTURESTREAMEFFECT & header.flags)
    {
        RwChar      name[rpMTEFFECTNAMELENGTH];
        RpMTEffect *effect;

        if (!_rwStringStreamFindAndRead(name, stream))
        {
            MultiTextureDestroy(multiTexture);
            RWRETURN((RwStream *) NULL);
        }

        effect = RpMTEffectFind(name);
        if (!effect)
        {
            effect = RpMTEffectCreateDummy();
            if (!effect)
            {
                MultiTextureDestroy(multiTexture);
                RWRETURN((RwStream *) NULL);
            }

            RpMTEffectSetName(effect, name);
        }

        RpMultiTextureSetEffect(multiTexture, effect);

        /* Get rid of the superfluous reference count */
        RpMTEffectDestroy(effect);
    }

    /* Set the extension */
    GetMultiTextureExtMacro(object, offset)->multiTexture = multiTexture;

    RWRETURN(stream);
}


/*
 *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *      SPI functions.
 *
 *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */


/******************************************************************************
 */
RwBool
_rpMultiTexturePluginAttach()
{
    RWFUNCTION(RWSTRING("_rpMultiTexturePluginAttach"));

    /* Init effects */
    if (!_rpMTEffectSystemInit())
    {
        RWRETURN(FALSE);
    }

    /* Initialize registry memory */
    memset(RegEntries, 0, sizeof(RegEntries));

    _rpMultiTextureModule.globalsOffset =
        RwEngineRegisterPlugin(sizeof(rpMultiTextureGlobals),
                               rwID_MULTITEXPLUGIN,
                               MultiTextureOpen,
                               MultiTextureClose);

    if (_rpMultiTextureModule.globalsOffset < 0)
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}


/******************************************************************************
 */
RwBool
_rpMaterialRegisterMultiTexturePlugin(RwPlatformID    platformID,
                            RwUInt32        pluginID,
                            RwUInt32        extensionSize)
{
    /*
     *  This function may be used to create a multitexture plugin
     *  for any platform with unique platform ID and plugin ID given in the
     *  reg entry.
     */
    RwInt32                 offset;
    rpMultiTextureRegEntry  *regEntry;

    RWFUNCTION(RWSTRING("_rpMaterialRegisterMultiTexturePlugin"));
    RWASSERT(platformID);
    RWASSERT(pluginID);

    /* Register material plugIn */
    offset = RpMaterialRegisterPlugin(sizeof(MultiTextureExt),
                                      pluginID,
                                      MultiTextureConstructor,
                                      MultiTextureDestructor,
                                      MultiTextureCopy);
    if (offset < 0)
    {
        RWRETURN(FALSE);
    }

    regEntry = &RegEntries[platformID];
    regEntry->materialOffset = offset;

    offset = RpMaterialRegisterPluginStream(pluginID,
                                            MultiTextureStreamRead,
                                            MultiTextureStreamWrite,
                                            MultiTextureStreamGetSize);
    if (offset < 0)
    {
        RWRETURN(FALSE);
    }

    /* Log the remaining registry info */
    regEntry->platformID = platformID;
    regEntry->pluginID = pluginID;
    regEntry->extensionSize = extensionSize;

    RWRETURN(TRUE);
}


/*
 *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *      API functions
 *
 *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */


/******************************************************************************
 *  Multi-texture functions
 */


/**
 * \ingroup rpmultitex
 * \ref RpMultiTextureGetNumTextures
 * returns the number of textures used in the provided RpMultiTexture.
 * \param multiTexture a pointer to the RpMultiTexture.
 *
 * \return A \ref RwUInt32 describing the number of textures used.
 * \see RpMultiTextureSetTexture
 * \see RpMultiTextureGetTexture
 * \see RpMaterialCreateMultiTexture
 */
RwUInt32
RpMultiTextureGetNumTextures(const RpMultiTexture *multiTexture)
{
    RWAPIFUNCTION(RWSTRING("RpMultiTextureGetNumTextures"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(multiTexture);

    RWRETURN(multiTexture->numTextures);
}


/**
 * \ingroup rpmultitex
 * \ref RpMultiTextureSetEffect
 * sets the multi-texture effect to be used within this multi-texture.
 * \param multiTexture a pointer to the RpMultiTexture.
 * \param effect a pointer to the RpMTEffect to be used.
 * \return Returns a valid pointer to the changed \ref RpMultiTexture, or NULL
 * otherwise
 * \see RpMultiTextureGetEffect
 * \see RpMTEffectCreateDummy
 */
RpMultiTexture *
RpMultiTextureSetEffect(RpMultiTexture *multiTexture,
                        RpMTEffect     *effect)
{
    RWAPIFUNCTION(RWSTRING("RpMultiTextureSetEffect"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(multiTexture);
    RWASSERT((effect->platformID == multiTexture->regEntry->platformID) || (effect->platformID == 0));

    if (multiTexture->effect)
    {
        RpMTEffectDestroy(multiTexture->effect);
    }

    multiTexture->effect = effect;
    if (effect)
    {
        RpMTEffectAddRef(multiTexture->effect);
    }

    RWRETURN(multiTexture);
}


/**
 * \ingroup rpmultitex
 * \ref RpMultiTextureGetEffect
 * returns the RpMTEffect currently set for the RpMultiTexture.
 * \param multiTexture the RpMultiTexture the information is needed from.
 * \return A valid pointer to a RpMTEffect if an effect has been assigned
 * or NULL otherwise.
 * \see RpMultiTextureSetEffect
 */
RpMTEffect *
RpMultiTextureGetEffect(const RpMultiTexture *multiTexture)
{
    RWAPIFUNCTION(RWSTRING("RpMultiTextureGetEffect"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(multiTexture);

    RWRETURN(multiTexture->effect);
}


/**
 * \ingroup rpmultitex
 * \ref RpMultiTextureSetTexture
 * sets a texture in the multiTexture effect for a given texture index.
 * \param multiTexture A pointer to the RpMultiTexture.
 * \param index the index in the textures array this texture is to be set.
 * \param texture the texture to be set.
 * \return A valid pointer to a RpMultiTexture on success, NULL otherwise.
 * \see RpMultiTextureGetNumTextures
 * \see RpMultiTextureGetTexture
 */
RpMultiTexture *
RpMultiTextureSetTexture(RpMultiTexture *multiTexture,
                         RwUInt32        index,
                         RwTexture      *texture)
{
    RWAPIFUNCTION(RWSTRING("RpMultiTextureSetTexture"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(multiTexture);
    RWASSERT(index < multiTexture->numTextures);

    /* Clear out any existing texture */
    if (multiTexture->textures[index])
    {
        RwTextureDestroy(multiTexture->textures[index]);
    }

    /* Set new texture */
    multiTexture->textures[index] = texture;
    if (texture)
    {
        RwTextureAddRef(texture);
    }

    RWRETURN(multiTexture);
}


/**
 * \ingroup rpmultitex
 * \ref RpMultiTextureGetTexture
 * returns the RwTexture used for this index in the textures array.
 * \param multiTexture the RpMultiTexture we need the information from.
 * \param index the index in the texture array.
 * \return A valid pointer to a \ref RwTexture if successful, NULL otherwise.
 * \see RpMultiTextureSetTexture
 * \see RpMultiTextureGetNumTextures
 */
RwTexture *
RpMultiTextureGetTexture(const RpMultiTexture   *multiTexture,
                         RwUInt32          index)
{
    RWAPIFUNCTION(RWSTRING("RpMultiTextureGetTexture"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(multiTexture);
    RWASSERT(index < multiTexture->numTextures);

    RWRETURN(multiTexture->textures[index]);
}


/**
 * \ingroup rpmultitex
 * \ref RpMultiTextureSetCoords
 * sets the index to a set of texture coordinates for a stage in the multiTexture.
 * \param multiTexture A pointer to the RpMultiTexture.
 * \param index the stage to be set.
 * \param texCoordIndex the texture coordinate index to be set
 * \return A valid pointer to a RpMultiTexture if successful, NULL otherwise.
 * \see RpMultiTextureGetCoords
 */
RpMultiTexture *
RpMultiTextureSetCoords(RpMultiTexture    *multiTexture,
                        RwUInt32           index,
                        RwUInt32           texCoordIndex)
{
    RWAPIFUNCTION(RWSTRING("RpMultiTextureSetCoords"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(multiTexture);
    RWASSERT(index < multiTexture->numTextures);
    RWASSERT(texCoordIndex < rwMAXTEXTURECOORDS);

    multiTexture->coordMap[index] = (RwUInt8) texCoordIndex;

    RWRETURN(multiTexture);
}


/**
 * \ingroup rpmultitex
 * \ref RpMultiTextureGetCoords
 * returns the index to a set of texture coordinates for a specific stage, as defined
 * in the RpMultiTexture.
 * \param multiTexture the RpMultiTexture we need the information from.
 * \param index the stage where the desired texture coordinates set is used.
 * \return A \ref RwInt32, the index of the desired texture coordinates set.
 * \see RpMultiTextureSetCoords
 */
RwUInt32
RpMultiTextureGetCoords(const RpMultiTexture *multiTexture,
                        RwUInt32        index)
{
    RWAPIFUNCTION(RWSTRING("RpMultiTextureGetCoords"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(multiTexture);
    RWASSERT(index < multiTexture->numTextures);

    RWRETURN(multiTexture->coordMap[index]);
}


/******************************************************************************
 *  Material functions
 */


/**
 * \ingroup rpmultitex
 * \ref RpMaterialCreateMultiTexture
 * creates a RpMultiTexture material extension, attaches it to the material and
 * initializes the number of texture coordinates sets and textures.
 * \param material The material to be affected.
 * \param platformID The platform the multi-texture is to be used on. Currently,
 * only rwID_XBOX and rwID_GAMECUBE are valid platforms.
 * \param numTextures The number of texture in the texture array and number of
 * texture coordinates sets indices to be created.
 * \return A valid pointer to a \ref RpMaterial if successful, NULL otherwise.
 * \see RpMaterialDestroyMultiTexture
 * \see RpMultiTextureSetCoords
 * \see RpMultiTextureSetTexture
 * \see RpMultiTextureSetEffect
 */
RpMaterial *
RpMaterialCreateMultiTexture(RpMaterial      *material,
                             RwPlatformID     platformID,
                             RwUInt32         numTextures)
{
    rpMultiTextureRegEntry  *regEntry;
    MultiTextureExt         *ext;

    RWAPIFUNCTION(RWSTRING("RpMaterialCreateMultiTexture"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(material);
    RWASSERT(platformID <= MAXPLATFORMID);
    RWASSERT(RegEntries[platformID].pluginID);
    RWASSERT(numTextures <= rpMAXMULTITEXTURES);
    /* TODO: do we assert on num textures per platform? */

    regEntry = &RegEntries[platformID];
    ext = GetMultiTextureExtMacro(material, regEntry->materialOffset);

    /* Destroy any existing multiTexture */
    if (ext->multiTexture)
    {
        MultiTextureDestroy(ext->multiTexture);
    }

    /* Create the new multitexture */
    ext->multiTexture = MultiTextureCreate(regEntry, numTextures);
    if (!ext->multiTexture)
    {
        RWRETURN((RpMaterial *) NULL);
    }

    RWRETURN(material);
}


/**
 * \ingroup rpmultitex
 * \ref RpMaterialDestroyMultiTexture
 * destroys the multi-texture material extension and the memory associated to it.
 * Only the platform specific effect will be destroyed if the multi-texture is defined
 * for multiple platforms.
 * \param material The material to be destroyed.
 * \param platformID The platform specific effect to be destroyed. Currently,
 * only rwID_XBOX and rwID_GAMECUBE are valid platforms.
 * \return A valid pointer to a \ref RpMaterial.
 * \see RpMaterialCreateMultiTexture
 */
RpMaterial *
RpMaterialDestroyMultiTexture(RpMaterial   *material,
                              RwPlatformID  platformID)
{
    rpMultiTextureRegEntry  *regEntry;
    MultiTextureExt         *ext;

    RWAPIFUNCTION(RWSTRING("RpMaterialDestroyMultiTexture"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(material);
    RWASSERT(platformID <= MAXPLATFORMID);
    RWASSERT(RegEntries[platformID].pluginID);

    regEntry = &RegEntries[platformID];
    ext = GetMultiTextureExtMacro(material, regEntry->materialOffset);

    /* Destroy the multiTexture */
    if (ext->multiTexture)
    {
        MultiTextureDestroy(ext->multiTexture);
        ext->multiTexture = (RpMultiTexture *) NULL;
    }

    RWRETURN(material);
}


/**
 * \ingroup rpmultitex
 * \ref RpMaterialGetMultiTexture
 * returns a pointer to the RpMultiTexture associated with the material.
 * \param material The material we need the information from.
 * \param platformID The platform we want the multi-texture for.Currently,
 * only rwID_XBOX and rwID_GAMECUBE are valid platforms.
 * \return A valid pointer to the RpMultiTexture requested on success,
 * Null otherwise.
 * \see RpMaterialCreateMultiTexture.
 */
RpMultiTexture *
RpMaterialGetMultiTexture(const RpMaterial    *material,
                          RwPlatformID   platformID)
{
    rpMultiTextureRegEntry  *regEntry;

    RWAPIFUNCTION(RWSTRING("RpMaterialGetMultiTexture"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(material);
    RWASSERT(platformID <= MAXPLATFORMID);

    regEntry = &RegEntries[platformID];
    if (regEntry->pluginID)
    {
        const MultiTextureExt *ext;
        ext = GetConstMultiTextureExtMacro(material, regEntry->materialOffset);
        RWRETURN(ext->multiTexture);
    }

    RWRETURN((RpMultiTexture *) NULL);
}


/**
 * \ingroup rpmultitex
 * \ref RpMaterialQueryMultiTexturePlatform
 * specifies if the multi-texture extension is available for the
 * requested platform.
 * \param platformID The platform we need the information for. Currently,
 * only rwID_XBOX and rwID_GAMECUBE are valid platforms.
 * \return A \ref RwBool, TRUE if the multi-texture extension exists
 * on the platform, FALSE otherwise.
 * \see RpMaterialCreateMultiTexture
 */
RwBool
RpMaterialQueryMultiTexturePlatform(RwPlatformID    platformID)
{
    RWAPIFUNCTION(RWSTRING("RpMaterialQueryMultiTexturePlatform"));
    RWASSERT(_rpMultiTextureModule.numInstances);
    RWASSERT(platformID <= MAXPLATFORMID);

    RWRETURN(RegEntries[platformID].pluginID != 0);
}
