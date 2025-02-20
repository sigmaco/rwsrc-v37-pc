/*
 *  multiTexNull.c
 *
 *  Support for xbox and GameCube multitexturing data in null builds.
 *  This just means that we can access a texture array and the name of
 *  an effect. We do not have access to the effect data itself.
 */

/******************************************************************************
 *  Includes
 */

#include "rpplugin.h"
#include <rpdbgerr.h>
#include "rwcore.h"

#include "multiTex.h"

/******************************************************************************
 */
typedef struct BlindEffect
{
    RwUInt32    size;
    RwUInt32    version;
    void       *data;

} BlindEffect;

/******************************************************************************
 */
static RwInt32
NullEffectGetSize(const RpMTEffect *effect)
{
    const BlindEffect *blind;

    RWFUNCTION(RWSTRING("NullEffectGetSize"));
    RWASSERT(effect);

    blind = (const BlindEffect *)(effect + 1);

    RWRETURN(blind->size);
}

/******************************************************************************
 */
static const RpMTEffect *
NullEffectWrite(const RpMTEffect *effect,
                RwStream   *stream)
{
    const BlindEffect *blind;

    RWFUNCTION(RWSTRING("NullEffectWrite"));
    RWASSERT(effect);
    RWASSERT(stream);

    blind = (const BlindEffect *)(effect + 1);

    /* Write the chunk header */
    if (!_rwStreamWriteVersionedChunkHeader(
        stream, rwID_EXTENSION, blind->size, blind->version, RWBUILDNUMBER))
    {
        RWRETURN((RpMTEffect *) NULL);
    }

    /* Write the data */
    if (!RwStreamWrite(stream, &blind->data, blind->size))
    {
        RWRETURN((RpMTEffect *) NULL);
    }

    RWRETURN(effect);
}


/******************************************************************************
 */
static RpMTEffect *
NullEffectRead(RwStream    *stream,
               RwPlatformID platformID,
               RwUInt32     version,
               RwUInt32     length)
{
    RpMTEffect     *effect;
    BlindEffect    *blind;
    RwUInt32        size;

    RWFUNCTION(RWSTRING("NullEffectRead"));
    RWASSERT(stream);

    /* Allocate memory for the effect */
    size = sizeof(RpMTEffect) + sizeof(BlindEffect) + length;
    effect = (RpMTEffect *) RwMalloc(size,
        rwID_MULTITEXPLUGIN | rwMEMHINTDUR_EVENT);
    if (!effect)
    {
        RWERROR((E_RW_NOMEM, size));
        RWRETURN((RpMTEffect *) NULL);
    }

    /* Initialize - we need to store the stream version for streaming out
     * later. This prevents interference with any necessary conversion to
     * be done on the target platform.
     */
    _rpMTEffectInit(effect, platformID);
    blind = (BlindEffect *)(effect + 1);
    blind->version = version;
    blind->size  = length;

    /* Read the blind data */
    if (length != RwStreamRead(stream, &blind->data, length))
    {
        RwFree(effect);
        RWRETURN((RpMTEffect *) NULL);
    }

    RWRETURN(effect);
}


/******************************************************************************
 */
RwBool
_rpMultiTexturePlatformPluginsAttach(void)
{
    RWFUNCTION(RWSTRING("_rpMultiTexturePlatformPluginsAttach"));

    /* Attach multitexture plugin system */
    if (!_rpMultiTexturePluginAttach())
    {
        RWRETURN(FALSE);
    }

    /* Register a multitexture plugin specific to GameCube */
    if (!_rpMaterialRegisterMultiTexturePlugin(
                rwID_GAMECUBE, rwID_GCNMATPLUGIN, 0))
    {
        RWRETURN(FALSE);
    }

    /* Register gamecube effects - just streams blind data */
    if (!_rpMTEffectRegisterPlatform(rwID_GAMECUBE,
        NullEffectRead, NullEffectWrite, NullEffectGetSize, NULL))
    {
        RWRETURN(FALSE);
    }

    /* Register a multitexture plugin specific to XBOX */
    if (!_rpMaterialRegisterMultiTexturePlugin(
                rwID_XBOX, rwID_XBOXMATPLUGIN, 0))
    {
        RWRETURN(FALSE);
    }

    /* Register xbox effects - just streams blind data */
    if (!_rpMTEffectRegisterPlatform(rwID_XBOX,
        NullEffectRead, NullEffectWrite, NullEffectGetSize, NULL))
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}

