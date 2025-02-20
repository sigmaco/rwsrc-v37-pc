/*****************************************************************************
 *
 * File :     rpmipkl.c
 *
 * Abstract : PS2 Mipmap K&L plugin for Renderware.
 *
 *****************************************************************************
 *
 * This file is a product of Criterion Software Ltd.
 *
 * This file is provided as is with no warranties of any kind and is
 * provided without any obligation on Criterion Software Ltd. or
 * Canon Inc. to assist in its use or modification.
 *
 * Criterion Software Ltd. will not, under any
 * circumstances, be liable for any lost revenue or other damages arising
 * from the use of this file.
 *
 * Copyright (c) 2000 Criterion Software Ltd.
 * All Rights Reserved.
 *
 * RenderWare is a trademark of Canon Inc.
 *
 *****************************************************************************/

/**
 * \ingroup rpmipkl
 * \page rpmipkloverview RpMipmapKL Plugin Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rpmipkl.h
 * \li \b Libraries: rwcore, rpworld, rpmipkl
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpMipmapKLPluginAttach
 *
 * \subsection mipkloverview Overview
 * The RpMipmapKL plugin extends the \ref RwTexture to contain "K" and "L"
 * values. The \ref rpmipkl plugin is provided on all platforms. This
 * allows the generation of art assets for the PlayStation 2.
 *
 * The "K" and "L" values control the way mipmap levels are selected by the
 * GS. Except in the unusual situation where all textures have been sized
 * to get an even texture density across the scene, it is better to
 * set each texture's "K" and "L" values directly using
 * \ref RpMipmapKLTextureSetK and \ref RpMipmapKLTextureSetL.
 *
 * \li "K" specifies the distance a polygon needs to be from the camera
 *     such that the pixel to texel ratio, when rendered, is 1:1 assuming
 *     the polygon is flat on the screen.
 *
 * \li "L" specifies the angle of inclination between the polygon and the
 *     camera viewwindow, it's a 2-bit value where 0 says the angle is 0
 *     degrees and 3 says it's 90 degrees.
 *
 * Essentially they allow you to work out the pixel:texel ratio when
 * rendering a single pixel, if you know it's distance from the camera.
 * For example, the GS can get the mipmap level purely from the value
 * in Z. Further information is available in the GS User's Manual.
 *
 * The toolkit \ref rtmipk is provided to help calculate suitable "K"
 * values.
 *
 * The following functions are provided to access the default "K" and
 * "L" values:
 * \li \ref RpMipmapKLTextureSetDefaultK
 * \li \ref RpMipmapKLTextureSetDefaultL
 * \li \ref RpMipmapKLTextureGetDefaultK
 * \li \ref RpMipmapKLTextureGetDefaultL
 *
 * The following functions are provided to directly alter the "K" and "L"
 * values for an individual texture:
 * \li \ref RpMipmapKLTextureSetK
 * \li \ref RpMipmapKLTextureSetL
 * \li \ref RpMipmapKLTextureGetK
 * \li \ref RpMipmapKLTextureGetL
 *
 * \note The \ref rtmipk toolkit is required to calculate the "K" and "L"
 * values. These calculations need to be performed as a pre-process.
 *
 * \if sky2
 * \note
 * When developing on the PlayStation 2, the driver functions
 * should be used instead to directly access the "K" and "L" values.
 * This plugin should not be attached or used with SKY or
 * NULLSKY libs.
 * \par
 * In order to alter the "K" and "L" values on the SKY and NULLSKY libs, call:
 * \li \ref RpSkyTextureSetDefaultMipmapK
 * \li \ref RpSkyTextureSetDefaultMipmapL
 * \li \ref RpSkyTextureSetMipmapK
 * \li \ref RpSkyTextureSetMipmapL
 * \endif
 */

#include <string.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include <rpdbgerr.h>
#include "rpmipkl.h"

/* Only build the actual lib on non Sky builds since for Sky builds we
   remap the functions to driver calls, however for doxygen builds allow the
   entire source to be processed */
#if (((!defined(SKY2_DRVMODEL_H)) && (!defined(NULLSKY_DRVMODEL_H))) || (defined(DOXYGEN)))

/*--- Local Defines ---*/

#define TEXTUREGETMIPMAPKL(tex) \
    ((RwUInt16*)(((RwUInt8*)tex) + mipmapKLTextureOffset))

#define CONSTTEXTUREGETMIPMAPKL(tex) \
    ((const RwUInt16*)(((const RwUInt8*)tex) + mipmapKLTextureOffset))

/*--- Global Variables ---*/

#if (defined(RWDEBUG))
long                rpMipmapKLStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

RwModuleInfo        mipmapKLModule;

static RwInt32      mipmapKLTextureOffset = 0;
static RwUInt32     mipmapKLDefault = 0xfc0;

static RwInt32      mipmapKLTextureStreamOffset = 0;

/*--- Plugin Engine Functions ---*/

static void *
MipmapKLOpen( void *instance,
              RwInt32 offset __RWUNUSED__,
              RwInt32 size __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("MipmapKLOpen"));
    RWASSERT(instance);

    mipmapKLModule.numInstances++;

    RWRETURN(instance);
}

static void *
MipmapKLClose( void *instance,
               RwInt32 offset __RWUNUSED__,
               RwInt32 size __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("MipmapKLClose"));
    RWASSERT(instance);

    mipmapKLModule.numInstances--;

    RWRETURN(instance);
}

/*--- Plugin Texture Functions ---*/

static void *
MipmapKLConstruct( void *object,
                   RwInt32 offset __RWUNUSED__,
                   RwInt32 size __RWUNUSED__ )
{
    RwTexture *tex;
    RwUInt16  *mipmapKL;

    RWFUNCTION(RWSTRING("MipmapKLConstruct"));
    RWASSERT(object);

    tex = (RwTexture *)object;
    mipmapKL = TEXTUREGETMIPMAPKL(tex);
    *mipmapKL = (RwUInt16)mipmapKLDefault;

    RWRETURN(object);
}

static void *
MipmapKLDestruct( void *object,
                  RwInt32 offset __RWUNUSED__,
                  RwInt32 size __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("MipmapKLDestruct"));
    RWASSERT(object);

    RWRETURN(object);
}

static void *
MipmapKLCopy( void *destinationObject,
              const void *sourceObject,
              RwInt32 offset __RWUNUSED__,
              RwInt32 size __RWUNUSED__ )
{
    RwTexture       *destinationTexture;
    const RwTexture *sourceTexture;
    RwUInt16        *destinationMipmapKL;
    const RwUInt16  *sourceMipmapKL;

    RWFUNCTION(RWSTRING("MipmapKLCopy"));
    RWASSERT(destinationObject);
    RWASSERT(sourceObject);

    destinationTexture = (RwTexture *)destinationObject;
    sourceTexture = (const RwTexture *)sourceObject;
    destinationMipmapKL = TEXTUREGETMIPMAPKL(destinationTexture);
    sourceMipmapKL = CONSTTEXTUREGETMIPMAPKL(sourceTexture);

    *destinationMipmapKL = *sourceMipmapKL;

    RWRETURN(destinationObject);
}

/*--- Plugin Texture Stream Functions ---*/

static RwStream *
MipmapKLRead( RwStream *stream,
              RwInt32 binaryLength __RWUNUSED__,
              void *object,
              RwInt32 offset __RWUNUSED__,
              RwInt32 size __RWUNUSED__ )
{
    RwTexture *tex = (RwTexture *)object;
    RwInt32   val;

    RWFUNCTION(RWSTRING("MipmapKLRead"));

    if ((binaryLength == 4) && (tex))
    {
        if (RwStreamReadInt(stream, &val, sizeof(RwInt32)))
        {
            *TEXTUREGETMIPMAPKL(tex) = (RwUInt16)val;
            RWRETURN(stream);
        }
        RWRETURN((RwStream *)NULL);
    }
    RWRETURN((RwStream *)NULL);
}

static RwStream *
MipmapKLWrite( RwStream *stream,
               RwInt32 binaryLength __RWUNUSED__,
               const void *object,
               RwInt32 offset __RWUNUSED__,
               RwInt32 size)
{
    const RwTexture *tex = (const RwTexture *)object;
    RwInt32         val;

    RWFUNCTION(RWSTRING("MipmapKLWrite"));

    if ((tex) && (tex->raster) && (size != 0))
    {
        val = (RwInt32)(*CONSTTEXTUREGETMIPMAPKL(tex));
        if (RwStreamWriteInt(stream, &val, sizeof(RwInt32)))
        {
            RWRETURN(stream);
        }
        RWRETURN((RwStream *)NULL);
    }
    RWRETURN((RwStream *)NULL);
}

static RwInt32
MipmapKLGetSize( const void *object __RWUNUSED__,
                 RwInt32 offsetInObject __RWUNUSED__,
                 RwInt32 sizeInObject __RWUNUSED__ )
{
    const RwTexture *tex = (const RwTexture *)object;
    RwInt32         val;

    RWFUNCTION(RWSTRING("MipmapKLGetSize"));

    val = ((tex) && (tex->raster)
           && ((RwInt32)(*CONSTTEXTUREGETMIPMAPKL(tex)) != mipmapKLDefault))
           ? 4 : 0;

    RWRETURN(val);
}


/*--- Plugin API Functions ---*/

/**
 * \ingroup rpmipkl
 * \ref RpMipmapKLTextureSetDefaultK
 * is used to set the default "K" value that will be used when textures
 * are created. It will almost always be better to set this, per texture,
 * using \ref RpMipmapKLTextureSetK.
 *
 * \if sky2
 * \note
 * When developing on the PlayStation 2, the driver functions
 * should be used instead to directly access the "K" and "L" values.
 * This plugin should not be attached or used with SKY or
 * NULLSKY libs.
 * \par
 * In order to alter the "K" and "L" values on the SKY and NULLSKY libs, call:
 * \li \ref RpSkyTextureSetDefaultMipmapK
 * \li \ref RpSkyTextureSetDefaultMipmapL
 * \li \ref RpSkyTextureSetMipmapK
 * \li \ref RpSkyTextureSetMipmapL
 * \endif
 *
 * \param val floating point "K" value.
 *
 * \return "K" value actually asigned (Limited precision is available)
 *
 * \see RpMipmapKLTextureSetDefaultL
 * \see RpMipmapKLTextureGetDefaultK
 * \see RpMipmapKLTextureGetDefaultL
 * \see RpMipmapKLTextureSetK
 */
RwReal
RpMipmapKLTextureSetDefaultK(RwReal val)
{
    RwInt32 i;
    RWAPIFUNCTION(RWSTRING("RpMipmapKLTextureSetDefaultK"));
    RWASSERT(0 < mipmapKLModule.numInstances);

    i = RwInt32FromRealMacro(val*16.0f);
    if (i < -2048)
    {
        i = -2048;
    }
    else if (i > 2047)
    {
        i = 2047;
    }
    mipmapKLDefault |= (RwUInt32)i & 0xfff;
    RWRETURN((RwReal)i * 0.0625f);
}


/**
 * \ingroup rpmipkl
 * \ref RpMipmapKLTextureSetDefaultL
 * is used to set the default "L" value that will be used when textures
 * are created. It will almost always be better to set this, per texture,
 * using \ref RpMipmapKLTextureSetL.
 *
 * \param val "L" value.
 *
 * \return "L" value actually asigned (Limited range is available)
 *
 * \if sky2
 * \note
 * When developing on the PlayStation 2, the driver functions
 * should be used instead to directly access the "K" and "L" values.
 * This plugin should not be attached or used with SKY or
 * NULLSKY libs.
 * \par
 * In order to alter the "K" and "L" values on the SKY and NULLSKY libs, call:
 * \li \ref RpSkyTextureSetDefaultMipmapK
 * \li \ref RpSkyTextureSetDefaultMipmapL
 * \li \ref RpSkyTextureSetMipmapK
 * \li \ref RpSkyTextureSetMipmapL
 * \endif
 *
 * \see RpMipmapKLTextureSetDefaultK
 * \see RpMipmapKLTextureGetDefaultK
 * \see RpMipmapKLTextureGetDefaultL
 * \see RpMipmapKLTextureSetL
 */
RwUInt32
RpMipmapKLTextureSetDefaultL(RwUInt32 val)
{
    RWAPIFUNCTION(RWSTRING("RpMipmapKLTextureSetDefaultL"));
    RWASSERT(0 < mipmapKLModule.numInstances);

    if (val > 3)
    {
        val = 3;
    }
    mipmapKLDefault |= (val<<12);
    RWRETURN(val);
}


/**
 * \ingroup rpmipkl
 * \ref RpMipmapKLTextureGetDefaultK
 * is used to get the current default "K" value used at texture creation.
 *
 * \return "K" value currently used by default
 *
 * \if sky2
 * \note
 * When developing on the PlayStation 2, the driver functions
 * should be used instead to directly access the "K" and "L" values.
 * This plugin should not be attached or used with SKY or
 * NULLSKY libs.
 * \par
 * In order to alter the "K" and "L" values on the SKY and NULLSKY libs, call:
 * \li \ref RpSkyTextureSetDefaultMipmapK
 * \li \ref RpSkyTextureSetDefaultMipmapL
 * \li \ref RpSkyTextureSetMipmapK
 * \li \ref RpSkyTextureSetMipmapL
 * \endif
 *
 * \see RpMipmapKLTextureSetDefaultK
 * \see RpMipmapKLTextureSetDefaultL
 * \see RpMipmapKLTextureGetDefaultL
 */
RwReal
RpMipmapKLTextureGetDefaultK(void)
{
    RwInt32 i;
    RWAPIFUNCTION(RWSTRING("RpMipmapKLTextureGetDefaultK"));
    RWASSERT(0 < mipmapKLModule.numInstances);

    i = mipmapKLDefault & 0xfff;
    if (i & 0x800)
    {
        i |= ~0xfff;
    }
    RWRETURN((RwReal)i * 0.0625f);
}


/**
 * \ingroup rpmipkl
 * \ref RpMipmapKLTextureGetDefaultL
 * is used to get the current default "K" value used at texture creation.
 *
 * \return "K" value currently used by default
 *
 * \if sky2
 * \note
 * When developing on the PlayStation 2, the driver functions
 * should be used instead to directly access the "K" and "L" values.
 * This plugin should not be attached or used with SKY or
 * NULLSKY libs.
 * \par
 * In order to alter the "K" and "L" values on the SKY and NULLSKY libs, call:
 * \li \ref RpSkyTextureSetDefaultMipmapK
 * \li \ref RpSkyTextureSetDefaultMipmapL
 * \li \ref RpSkyTextureSetMipmapK
 * \li \ref RpSkyTextureSetMipmapL
 * \endif
 *
 * \see RpMipmapKLTextureSetDefaultK
 * \see RpMipmapKLTextureSetDefaultL
 * \see RpMipmapKLTextureGetDefaultK
 */
RwUInt32
RpMipmapKLTextureGetDefaultL(void)
{
    RwUInt32 i;
    RWAPIFUNCTION(RWSTRING("RpMipmapKLTextureGetDefaultL"));
    RWASSERT(0 < mipmapKLModule.numInstances);

    i = (mipmapKLDefault & 0x3000) >> 12;
    RWRETURN(i);
}

/**
 * \ingroup rpmipkl
 * \ref RpMipmapKLTextureSetK
 * is used to set the "K" value that will be used when a particular
 * texture is drawn.
 *
 * \param tex  texture to update.
 * \param val  floating point "K" value.
 *
 * \return The texture, otherwise NULL on failure
 *
 * \if sky2
 * \note
 * When developing on the PlayStation 2, the driver functions
 * should be used instead to directly access the "K" and "L" values.
 * This plugin should not be attached or used with SKY or
 * NULLSKY libs.
 * \par
 * In order to alter the "K" and "L" values on the SKY and NULLSKY libs, call:
 * \li \ref RpSkyTextureSetDefaultMipmapK
 * \li \ref RpSkyTextureSetDefaultMipmapL
 * \li \ref RpSkyTextureSetMipmapK
 * \li \ref RpSkyTextureSetMipmapL
 * \endif
 *
 * \see RpMipmapKLTextureSetDefaultK
 * \see RpMipmapKLTextureGetDefaultK
 * \see RpMipmapKLTextureSetL
 * \see RpMipmapKLTextureGetK
 */
RwTexture *
RpMipmapKLTextureSetK(RwTexture *tex, RwReal val)
{
    RwInt32 i;

    RWAPIFUNCTION(RWSTRING("RpMipmapKLTextureSetK"));
    RWASSERT(0 < mipmapKLModule.numInstances);

    if (tex)
    {
        i = RwInt32FromRealMacro(val * 16.0f);
        if (i < -2048)
        {
            i = -2048;
        }
        else if (i > 2047)
        {
            i = 2047;
        }
        *TEXTUREGETMIPMAPKL(tex) &= ~0xfff;
        *TEXTUREGETMIPMAPKL(tex) |= (RwUInt16)i & 0xfff;
        RWRETURN(tex);
    }
    RWRETURN((RwTexture *)NULL);
}

/**
 * \ingroup rpmipkl
 * \ref RpMipmapKLTextureSetL is used to set the "L" value that will
 * be used when a particular texture is drawn.
 *
 * \param tex  texture to update.
 * \param val  "L" value.
 *
 * \return The texture, otherwise NULL on failure
 *
 * \if sky2
 * \note
 * When developing on the PlayStation 2, the driver functions
 * should be used instead to directly access the "K" and "L" values.
 * This plugin should not be attached or used with SKY or
 * NULLSKY libs.
 * \par
 * In order to alter the "K" and "L" values on the SKY and NULLSKY libs, call:
 * \li \ref RpSkyTextureSetDefaultMipmapK
 * \li \ref RpSkyTextureSetDefaultMipmapL
 * \li \ref RpSkyTextureSetMipmapK
 * \li \ref RpSkyTextureSetMipmapL
 * \endif
 *
 * \see RpMipmapKLTextureSetDefaultL
 * \see RpMipmapKLTextureGetDefaultL
 * \see RpMipmapKLTextureSetK
 * \see RpMipmapKLTextureGetL
 */
RwTexture *
RpMipmapKLTextureSetL(RwTexture *tex, RwUInt32 val)
{
    RWAPIFUNCTION(RWSTRING("RpMipmapKLTextureSetL"));
    RWASSERT(0 < mipmapKLModule.numInstances);

    if (tex)
    {
        if (val > 3)
        {
            val = 3;
        }
        *TEXTUREGETMIPMAPKL(tex) &= ~0x3000;
        *TEXTUREGETMIPMAPKL(tex) |= (RwUInt16)val << 12;
        RWRETURN(tex);
    }
    RWRETURN((RwTexture *)NULL);
}

/**
 * \ingroup rpmipkl
 * \ref RpMipmapKLTextureGetK is used to get the "K" value that will
 * be used when a particular texture is drawn.
 *
 * \param tex  texture being queried
 *
 * \return The texture's "K" value
 *
 * \if sky2
 * \note
 * When developing on the PlayStation 2, the driver functions
 * should be used instead to directly access the "K" and "L" values.
 * This plugin should not be attached or used with SKY or
 * NULLSKY libs.
 * \par
 * In order to alter the "K" and "L" values on the SKY and NULLSKY libs, call:
 * \li \ref RpSkyTextureSetDefaultMipmapK
 * \li \ref RpSkyTextureSetDefaultMipmapL
 * \li \ref RpSkyTextureSetMipmapK
 * \li \ref RpSkyTextureSetMipmapL
 * \endif
 *
 * \see RpMipmapKLTextureSetK
 * \see RpMipmapKLTextureGetL
 */
RwReal
RpMipmapKLTextureGetK(RwTexture *tex)
{
    RWAPIFUNCTION(RWSTRING("RpMipmapKLTextureGetK"));
    RWASSERT(0 < mipmapKLModule.numInstances);

    if (tex)
    {
        RwInt32 i;

        i = *TEXTUREGETMIPMAPKL(tex) & 0xfff;
        if (i & 0x800)
        {
            i |= ~0xfff;
        }
        RWRETURN((RwReal)i * 0.0625f);
    }
    RWRETURN((RwReal)0.0f);
}

/**
 * \ingroup rpmipkl
 * \ref RpMipmapKLTextureGetL is used to get the "L" value that will
 * be used when a particular texture is drawn.
 *
 * \param tex  texture being queried
 *
 * \return The texture's "L" value
 *
 * \if sky2
 * \note
 * When developing on the PlayStation 2, the driver functions
 * should be used instead to directly access the "K" and "L" values.
 * This plugin should not be attached or used with SKY or
 * NULLSKY libs.
 * \par
 * In order to alter the "K" and "L" values on the SKY and NULLSKY libs, call:
 * \li \ref RpSkyTextureSetDefaultMipmapK
 * \li \ref RpSkyTextureSetDefaultMipmapL
 * \li \ref RpSkyTextureSetMipmapK
 * \li \ref RpSkyTextureSetMipmapL
 * \endif
 *
 * \see RpMipmapKLTextureSetL
 * \see RpMipmapKLTextureGetK
 */
RwUInt32
RpMipmapKLTextureGetL(RwTexture *tex)
{
    RWAPIFUNCTION(RWSTRING("RpMipmapKLTextureGetL"));
    RWASSERT(0 < mipmapKLModule.numInstances);

    if (tex)
    {
        RwUInt32 i;

        i = (*TEXTUREGETMIPMAPKL(tex) & 0x3000) >> 12;
        RWRETURN(i);
    }
    RWRETURN(0);
}

/**
 * \ingroup rpmipkl
 * \ref RpMipmapKLPluginAttach is used to attach the MipmapKL plugin to
 * the RenderWare system to enable PlayStation 2 Mipmap "K" & "L" to be stored
 * with textures. The plugin must be attached between initializing the
 * system with \ref RwEngineInit and opening it with \ref RwEngineOpen.
 *
 * \note The include file rpmipkl.h is required and must be included by
 * an application wishing to use this facility.  The MipmapKL library is
 * contained in the file rpmipkl.lib.
 *
 * \return Returns TRUE if successful or FALSE if there is an error
 *
 * \if sky2
 * \note
 * When developing on the PlayStation 2, the driver functions
 * should be used instead to directly access the "K" and "L" values.
 * This plugin should not be attached or used with SKY or
 * NULLSKY libs.
 * \par
 * In order to alter the "K" and "L" values on the SKY and NULLSKY libs, call:
 * \li \ref RpSkyTextureSetDefaultMipmapK
 * \li \ref RpSkyTextureSetDefaultMipmapL
 * \li \ref RpSkyTextureSetMipmapK
 * \li \ref RpSkyTextureSetMipmapL
 * \endif
 *
 */
RwBool
RpMipmapKLPluginAttach(void)
{
    RwInt32 offset;

    RWAPIFUNCTION(RWSTRING("RpMipmapKLPluginAttach"));

    offset =
        RwEngineRegisterPlugin(0, rwID_SKYMIPMAPVAL, MipmapKLOpen,
                               MipmapKLClose);
    if (offset < 0)
    {
        RWRETURN(FALSE);
    }

    /*
     * Do not attach if building sky or nullsky as the driver will attache it
     * automatically
     */

    mipmapKLTextureOffset =
        RwTextureRegisterPlugin(sizeof(RwUInt16), rwID_SKYMIPMAPVAL,
                                MipmapKLConstruct, MipmapKLDestruct,
                                MipmapKLCopy);
    if (mipmapKLTextureOffset < 0)
    {
        RWRETURN(FALSE);
    }

    mipmapKLTextureStreamOffset =
        RwTextureRegisterPluginStream(rwID_SKYMIPMAPVAL, MipmapKLRead,
                                      MipmapKLWrite, MipmapKLGetSize);

    if (mipmapKLTextureStreamOffset < 0)
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}

#endif /* (((!defined(SKY2_DRVMODEL_H)) && (!defined(NULLSKY_DRVMODEL_H))) || (defined(DOXYGEN))) */
