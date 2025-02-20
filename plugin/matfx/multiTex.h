/*
 *  multitex.h : Multitexturing (not all platforms)
 */

/* RWPUBLIC */

#ifndef RPMATFX_MULTITEX_H
#define RPMATFX_MULTITEX_H

/* RWPUBLICEND */

/******************************************************************************
 *  Includes
 */
#include "rwcore.h"
#include "rpworld.h"

#include "multiTexEffect.h"


/******************************************************************************
 *  Defines
 */
#define rpMAXMULTITEXTURES 8


/******************************************************************************
 *  Types
 */

typedef struct rpMultiTextureRegEntry rpMultiTextureRegEntry;
struct rpMultiTextureRegEntry
{
    RwPlatformID    platformID;
    RwUInt32        pluginID;
    RwUInt32        materialOffset;
    RwUInt32        extensionSize;
};

/*
 *  RpMultiTexture          (opaque)
 */
struct RpMultiTexture
{
    rpMultiTextureRegEntry *regEntry;

    RwUInt32                numTextures;
    RwTexture              *textures[rpMAXMULTITEXTURES];
    RwUInt8                 coordMap[rpMAXMULTITEXTURES];

    RpMTEffect             *effect;

    void                   *extension;
};

/* RWPUBLIC */
/**
 * \ingroup rpmultitex
 * \struct RpMultiTexture
 * RpMultiTexture is an opaque type.
 *
 * \see RpMaterialCreateMultiTexture
 * \see RpMaterialGetMultiTexture
 * \see RpMultiTextureSetTexture
 * \see RpMultiTextureGetTexture
 * \see RpMultiTextureSetCoords
 * \see RpMultiTextureGetCoords
 * \see RpMultiTextureSetEffect
 * \see RpMultiTextureGetEffect
 */
typedef struct RpMultiTexture RpMultiTexture;
/* RWPUBLICEND */

typedef struct _rpMultiTextureGlobals
{
    rpMTEffectGlobals   effect;

} rpMultiTextureGlobals;


/******************************************************************************
 *  Global variables
 */
extern RwModuleInfo  _rpMultiTextureModule;


/******************************************************************************
 *  Macros
 */
#define RPMULTITEXTUREGLOBAL(var)     \
    (RWPLUGINOFFSET(rpMultiTextureGlobals, RwEngineInstance, \
        _rpMultiTextureModule.globalsOffset)->var)

/******************************************************************************
 *  Functions
 */

/* RWPUBLIC */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*--- MultiTexture Functions ------------------------------------------------*/

extern RwUInt32
RpMultiTextureGetNumTextures(const RpMultiTexture *multiTexture);

extern RpMultiTexture *
RpMultiTextureSetTexture(RpMultiTexture *multiTexture,
                         RwUInt32        index,
                         RwTexture      *texture);

extern RwTexture *
RpMultiTextureGetTexture(const RpMultiTexture *multiTexture,
                         RwUInt32        index);

extern RpMultiTexture *
RpMultiTextureSetCoords(RpMultiTexture *multiTexture,
                        RwUInt32        index,
                        RwUInt32        texCoordIndex);

extern RwUInt32
RpMultiTextureGetCoords(const RpMultiTexture *multiTexture,
                        RwUInt32        index);

extern RpMultiTexture *
RpMultiTextureSetEffect(RpMultiTexture *multiTexture,
                        RpMTEffect    *effect);

extern RpMTEffect *
RpMultiTextureGetEffect(const RpMultiTexture *multiTexture);


/*--- Material MultiTexture Functions ---------------------------------------*/

extern RpMaterial *
RpMaterialCreateMultiTexture(RpMaterial *material,
                             RwPlatformID platformID,
                             RwUInt32 numTextures);

extern RpMaterial *
RpMaterialDestroyMultiTexture(RpMaterial *material,
                              RwPlatformID platformID);

extern RpMultiTexture *
RpMaterialGetMultiTexture(const RpMaterial *material,
                          RwPlatformID platformID);

extern RwBool
RpMaterialQueryMultiTexturePlatform(RwPlatformID platformID);

/* RWPUBLICEND */

extern RwBool   
_rpMultiTexturePluginAttach(void);

extern RwBool   
_rpMaterialRegisterMultiTexturePlugin(RwPlatformID platformID,
                                      RwUInt32 pluginID,
                                      RwUInt32 extensionSize);

extern RwBool   
_rpMultiTexturePlatformPluginsAttach(void);

/* RWPUBLIC */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RPMATFX_MULTITEX_H */

/* RWPUBLICEND */
