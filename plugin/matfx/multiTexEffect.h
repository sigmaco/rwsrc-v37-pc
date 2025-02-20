/*
 *  multitexEffect.h : Multitexture effects (not all platforms)
 */

/* RWPUBLIC */

#ifndef RPMATFX_MULTITEXEFFECT_H
#define RPMATFX_MULTITEXEFFECT_H

/* RWPUBLIC */

/******************************************************************************
 *  Includes
 */
#include "rwcore.h"
#include "rpworld.h"

/******************************************************************************
 *  Defines
 */
#define rpMTEFFECTNAMELENGTH      32

/******************************************************************************
 *  Types
 */

/* RWPUBLIC */

/**
 * \ingroup rpmultitex
 * \struct RpMTEffect
 * RpMTEffect is an opaque type. See the RpMTEffect API.
 */
typedef struct RpMTEffect RpMTEffect;

/**
 * \ingroup rpmultitex
 * \struct RpMTEffectDict
 * RpMTEffectDict is an opaque type.
 *
 * \see RpMTEffectDictCreate
 * \see RpMTEffectDictDestroy
 * \see RpMTEffectDictGetCurrent
 * \see RpMTEffectDictSetCurrent
 * \see RpMTEffectDictAddEffect
 * \see RpMTEffectDictFindNamedEffect
 * \see RpMTEffectDictStreamRead
 */
typedef struct RpMTEffectDict RpMTEffectDict;

/**
 * \ingroup rpmultitex
 *  RpMTEffectCallBack is the callback type used with
 * \ref RpMTEffectDictForAllEffects.
 *
 * \param effect    Pointer to the multi-texture effect.
 * \param data      Pointer to used data.
 *
 * \return Pointer to the effect, or NULL for early out.
 */
typedef RpMTEffect *(*RpMTEffectCallBack)(RpMTEffect *effect, void *data);

/**
 * \ingroup rpmultitex
 * \ref RpMTEffectDictCallBack
 * RpMTEffectDictCallBack is the callback type used with
 * \ref RpMTEffectDictForAllDictionaries.
 *
 * \param dict      Pointer to the multi-texture effect dictionary.
 * \param data      Pointer to used data.
 *
 * \return Pointer to the effect dictionary, or NULL for early out.
 */
typedef RpMTEffectDict *(*RpMTEffectDictCallBack)(
                            RpMTEffectDict *dict, void *data);

/* RWPUBLICEND */

struct RpMTEffectDict
{
    RwLinkList  effectList;   /* List of textures in dictionary */
    RwLLLink    dictListLink; /* Link list of all dicts in system */
};

typedef struct rpMTEffectGlobals rpMTEffectGlobals;
struct rpMTEffectGlobals
{
    RwLinkList          dictList;
    RpMTEffectDict     *currentDict;
    RwInt32             pathSize;
    RwChar             *path;
    RwChar             *scratchMem;
};

/* RWPUBLIC */

#if(!defined(DOXYGEN))
struct RpMTEffect
{
    RwPlatformID    platformID;
    RwUInt32        refCount;
    RwChar          name[rpMTEFFECTNAMELENGTH];
    RwLLLink        dictLink;
};
#endif /* (!defined(DOXYGEN)) */

typedef void        (*rpMTEffectDestroyCallBack)(RpMTEffect *effect);
typedef RwInt32     (*rpMTEffectStreamGetSizeCallBack)(const RpMTEffect *effect);
typedef RpMTEffect *(*rpMTEffectStreamReadCallBack)(RwStream *stream,
                                                    RwPlatformID platformID,
                                                    RwUInt32 version,
                                                    RwUInt32 length);
typedef const RpMTEffect *(*rpMTEffectStreamWriteCallBack)(
                        const RpMTEffect *effect, RwStream *stream);

/******************************************************************************
 *  Functions
 */


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*--- MultiTexture Effect Dictionary Functions ------------------------------*/

extern RpMTEffectDict *
RpMTEffectDictCreate(void);

extern void
RpMTEffectDictDestroy(RpMTEffectDict *dict);

extern RpMTEffectDict *
RpMTEffectDictAddEffect(RpMTEffectDict *dict, RpMTEffect *effect);

extern RpMTEffect *
RpMTEffectDictRemoveEffect(RpMTEffect *effect);

extern RpMTEffect *
RpMTEffectDictFindNamedEffect(const RpMTEffectDict *dict,
                              const RwChar *name);

extern RpMTEffectDict *
RpMTEffectDictSetCurrent(RpMTEffectDict *dict);

extern RpMTEffectDict *
RpMTEffectDictGetCurrent(void);

extern RwUInt32
RpMTEffectDictStreamGetSize(const RpMTEffectDict *dict);

extern const RpMTEffectDict *
RpMTEffectDictStreamWrite(const RpMTEffectDict *dict, RwStream *stream);

extern RpMTEffectDict *
RpMTEffectDictStreamRead(RwStream *stream);

extern RwBool
RpMTEffectDictForAllDictionaries(RpMTEffectDictCallBack callBack, void *data);

extern const RpMTEffectDict *
RpMTEffectDictForAllEffects(const RpMTEffectDict *dict,
                            RpMTEffectCallBack    callBack,
                            void                 *data);

/*--- Multi-Texture Effect Functions ----------------------------------------*/

extern const RwChar *
RpMTEffectSetPath(const RwChar *path);

extern RwChar *
RpMTEffectGetPath(void);

extern RpMTEffect *
RpMTEffectCreateDummy(void);

extern void
RpMTEffectDestroy(RpMTEffect *effect);

extern RwUInt32
RpMTEffectStreamGetSize(const RpMTEffect *effect);

extern const RpMTEffect *
RpMTEffectStreamWrite(const RpMTEffect *effect, RwStream *stream);

extern RpMTEffect *
RpMTEffectStreamRead(RwStream *stream);

extern RpMTEffect *
RpMTEffectFind(RwChar *name);

extern const RpMTEffect *
RpMTEffectWrite(const RpMTEffect *effect);

extern RpMTEffect *
RpMTEffectSetName(RpMTEffect *effect, RwChar *name);

extern RwChar *
RpMTEffectGetName(RpMTEffect *effect);

extern RpMTEffect *
RpMTEffectAddRef(RpMTEffect *effect);

extern RwBool 
_rpMTEffectSystemInit(void);

extern RwBool 
_rpMTEffectRegisterPlatform(RwPlatformID                     platformID,
                            rpMTEffectStreamReadCallBack     streamRead,
                            rpMTEffectStreamWriteCallBack    streamWrite,
                            rpMTEffectStreamGetSizeCallBack  streamGetSize,
                            rpMTEffectDestroyCallBack        destroy);

extern RwBool 
_rpMTEffectOpen(void);

extern RwBool
_rpMTEffectClose(void);

extern RpMTEffect *
_rpMTEffectInit(RpMTEffect   *effect, 
                RwPlatformID  platformID);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RPMATFX_MULTITEXEFFECT_H */

/* RWPUBLICEND */
