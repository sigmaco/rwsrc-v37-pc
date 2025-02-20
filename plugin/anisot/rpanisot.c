/*****************************************************************************
 *
 * File :     rpanisot.c
 *
 * Abstract : Xbox, D3D8, D3D9 & OpenGL Anisotropic Texture Sampling Plugin for RenderWare.
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

#include <string.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include <rpdbgerr.h>
#include "rpanisot.h"
#include "anispriv.h"

/*--- Local Defines ---*/

#define TEXTUREGETANISOT(tex) \
    ((RwUInt8*)(((RwUInt8*)tex) + anisotTextureOffset))

#define CONSTTEXTUREGETANISOT(tex) \
    ((const RwUInt8*)(((const RwUInt8*)tex) + anisotTextureOffset))

/*--- Global Variables ---*/

#if (defined(RWDEBUG))
long                rpAnisotStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

static RwModuleInfo anisotModule;

static RwInt32      anisotTextureOffset = 0;
static RwInt32      anisotTextureStreamOffset = 0;
static const RwInt8 anisotDefault = 1;  /* 1 does just isotropic sampling */

/*--- Plugin Engine Functions ---*/

static void        *
anisotOpen(void *instance, RwInt32 __RWUNUSED__ offset,
             RwInt32 __RWUNUSED__ size)
{
    RWFUNCTION(RWSTRING("anisotOpen"));
    RWASSERT(instance);

    anisotModule.numInstances++;

    RWRETURN(instance);
}

static void        *
anisotClose(void *instance, RwInt32 __RWUNUSED__ offset,
              RwInt32 __RWUNUSED__ size)
{
    RWFUNCTION(RWSTRING("anisotClose"));
    RWASSERT(instance);

    anisotModule.numInstances--;

    RWRETURN(instance);
}

/*--- Plugin Texture Functions ---*/

static void        *
anisotConstruct(void *object, RwInt32 __RWUNUSED__ offset,
                  RwInt32 __RWUNUSED__ size)
{
    RwTexture          *tex;
    RwUInt8           *anisot;

    RWFUNCTION(RWSTRING("anisotConstruct"));
    RWASSERT(object);

    tex = (RwTexture *) object;
    anisot = TEXTUREGETANISOT(tex);
    *anisot = (RwUInt8)anisotDefault;

    RWRETURN(object);
}

static void        *
anisotDestruct(void *object, RwInt32 __RWUNUSED__ offset,
                 RwInt32 __RWUNUSED__ size)
{
    RWFUNCTION(RWSTRING("anisotDestruct"));
    RWASSERT(object);

    RWRETURN(object);
}

static void        *
anisotCopy(void *destinationObject, const void *sourceObject,
             RwInt32 __RWUNUSED__ offset, RwInt32 __RWUNUSED__ size)
{
    RwTexture          *destinationTexture;
    const RwTexture    *sourceTexture;
    RwUInt8           *destinationanisot;
    const RwUInt8     *sourceanisot;

    RWFUNCTION(RWSTRING("anisotCopy"));
    RWASSERT(destinationObject);
    RWASSERT(sourceObject);

    destinationTexture = (RwTexture *) destinationObject;
    sourceTexture = (const RwTexture *) sourceObject;
    destinationanisot = TEXTUREGETANISOT(destinationTexture);
    sourceanisot = CONSTTEXTUREGETANISOT(sourceTexture);

    *destinationanisot = *sourceanisot;

    RWRETURN(destinationObject);
}

/*--- Plugin Texture Stream Functions ---*/

static RwStream    *
anisotRead(RwStream * stream, RwInt32 __RWUNUSED__ binaryLength,
             void *object, RwInt32 __RWUNUSED__ offset,
             RwInt32 __RWUNUSED__ size)
{
    RwTexture          *tex = (RwTexture *) object;
    RwInt32             val;

    RWFUNCTION(RWSTRING("anisotRead"));

    if ((binaryLength == 4) && (tex))
    {
        if (RwStreamReadInt(stream, &val, sizeof(RwInt32)))
        {
            RpAnisotTextureSetMaxAnisotropy(tex, (RwUInt8)val);
            RWRETURN(stream);
        }
        RWRETURN((RwStream *)NULL);
    }
    RWRETURN((RwStream *)NULL);
}

static RwStream    *
anisotWrite(RwStream * stream, RwInt32 __RWUNUSED__ binaryLength,
              const void *object, RwInt32 __RWUNUSED__ offset,
              RwInt32 __RWUNUSED__ size)
{
    const RwTexture    *tex = (const RwTexture *) object;
    RwInt32             val;

    RWFUNCTION(RWSTRING("anisotWrite"));

    if ((tex) && (tex->raster))
    {
        val = (RwInt32) (*CONSTTEXTUREGETANISOT(tex));
        if (RwStreamWriteInt(stream, &val, sizeof(RwInt32)))
        {
            RWRETURN(stream);
        }
        RWRETURN((RwStream *)NULL);
    }
    RWRETURN((RwStream *)NULL);
}

static RwInt32
anisotGetSize(const void * object,
              RwInt32 __RWUNUSED__ offsetInObject,
              RwInt32 __RWUNUSED__ sizeInObject)
{
    const RwTexture    *tex = (const RwTexture *) object;

    RWFUNCTION(RWSTRING("anisotGetSize"));

    if ((tex) && (tex->raster))
    {
        /*
         * Only return non-zero if an anisotropy value has been
         * changed from default. Unnecessary anisotropy extension
         * data won't be written, and the default value is used
         * anyway if the anisotropy plugin is attached
         */
        RwInt32 val = (RwInt32) (*CONSTTEXTUREGETANISOT(tex));
        if (anisotDefault != val)
        {
            RWRETURN(sizeof(RwInt32));
        }
    }
    RWRETURN(0);
}

/*--- Plugin API Functions ---*/
/**
 * \ingroup rpanisot
 * \ref RpAnisotGetMaxSupportedMaxAnisotropy
 * is used to determine the maximum anisotropy available on the hardware.
 * \if xbox For Xbox, this is 4. \endif
 *
 * \return \ref RwInt8 containing the maximum anisotropy supported, or 0 if
 *         it is not supported.
 *
 * \see RpAnisotTextureSetMaxAnisotropy
 * \see RpAnisotTextureGetMaxAnisotropy
 */
RwInt8
RpAnisotGetMaxSupportedMaxAnisotropy(void)
{
    RWAPIFUNCTION(RWSTRING("RpAnisotGetMaxSupportedMaxAnisotropy"));

#if (defined(XBOX_DRVMODEL_H) || defined(NULLXBOX_DRVMODEL_H) || defined(NULL_DRVMODEL_H))
    RWRETURN(4);
#elif defined(D3D8_DRVMODEL_H)
    RWRETURN((RwInt8)(((const D3DCAPS8 *)RwD3D8GetCaps())->MaxAnisotropy));
#elif defined(D3D9_DRVMODEL_H)
    RWRETURN((RwInt8)(((const D3DCAPS9 *)RwD3D9GetCaps())->MaxAnisotropy));
#elif defined(OPENGL_DRVMODEL_H)
    RWRETURN( _rwOpenGLExt.MaxTextureAnisotropy );
#else
    RWRETURN(0);    /* Not supported */
#endif
}

/**
 * \ingroup rpanisot
 * \ref RpAnisotTextureSetMaxAnisotropy is used to set the maximum anisotropy
 * value that will be used when a particular texture is drawn. Higher numbers
 * will produce better quality but slower texture sampling when polygons are
 * viewed edge on, so it should be used in moderation.
 *
 * \if xbox
 * On Xbox this is done with the D3DTSS_MAXANISOTROPY texture stage state.
 * If the value is > 1, filter modes with magnification and minification filters
 * of type D3DTEXF_ANISOTROPIC are chosen with the same mip mapping filter
 * D3DTSS_MIPFILTER determined by the filter mode of the texture.
 * See also RwTextureFilterMode and RwTextureGetFilterMode.
 * \endif
 *
 * \if d3d8
 * In D3D8 this is done with the D3DTSS_MAXANISOTROPY texture stage state.
 * If the value is > 1, filter modes with magnification and minification filters
 * of type D3DTEXF_ANISOTROPIC are chosen with the same mip mapping filter
 * D3DTSS_MIPFILTER determined by the filter mode of the texture.
 * See also RwTextureFilterMode and RwTextureGetFilterMode.
 * \endif
 *
 * \if d3d9
 * In D3D9 this is done with the D3DTSS_MAXANISOTROPY texture stage state.
 * If the value is > 1, filter modes with magnification and minification filters
 * of type D3DTEXF_ANISOTROPIC are chosen with the same mip mapping filter
 * D3DTSS_MIPFILTER determined by the filter mode of the texture.
 * See also RwTextureFilterMode and RwTextureGetFilterMode.
 * \endif
 *
 * \if opengl
 * In OpenGL, this is achieved with the <em> GL_EXT_texture_filter_anisotropic </em>
 * extension and <em> glTexParameter </em> function.
 * \endif
 *
 * \param tex  Pointer to \ref RwTexture to update.
 * \param val  \ref RwInt8 containing the max anisotropy value that must be less than
 * the maximum reported by \ref RpAnisotGetMaxSupportedMaxAnisotropy.
 *
 * \return Pointer to an \ref RwTexture that is the texture passed in as an argument,
 * or NULL on failure.
 *
 * \see RpAnisotTextureGetMaxAnisotropy
 * \see RpAnisotGetMaxSupportedMaxAnisotropy
 */
RwTexture *
RpAnisotTextureSetMaxAnisotropy(RwTexture * tex, RwInt8 val)
{
    RWAPIFUNCTION(RWSTRING("RpAnisotTextureSetMaxAnisotropy"));

    RWASSERT( val > 0 );

    /* don't want this assert on OpenGL since a 0 maximum level
     * indicates that the extension is not supported
     */
#if !defined(OPENGL_DRVMODEL_H)
    /* or perhaps just clamp? */
    RWASSERT( val <= RpAnisotGetMaxSupportedMaxAnisotropy() );
#endif /* !defined(OPENGL_DRVMODEL_H) */

    if (tex)
    {
        *TEXTUREGETANISOT(tex) = val;

        RWRETURN(tex);
    }

    RWRETURN((RwTexture *)NULL);
}

/**
 * \ingroup rpanisot
 * \ref RpAnisotTextureGetMaxAnisotropy is used to get the maximum anisotropy
 * value that will be used when a particular texture is drawn.
 *
 * See \ref RpAnisotTextureSetMaxAnisotropy for further information.
 *
 * \param tex Pointer to \ref RwTexture being queried.
 *
 * \return \ref RwInt8 containing the texture's maximum anisotropy value.
 *
 * \see RpAnisotTextureSetMaxAnisotropy
 * \see RpAnisotGetMaxSupportedMaxAnisotropy
 */
RwInt8
RpAnisotTextureGetMaxAnisotropy(RwTexture * tex)
{
    RWAPIFUNCTION(RWSTRING("RpAnisotTextureGetMaxAnisotropy"));
    if (tex)
    {
        RWRETURN(*TEXTUREGETANISOT(tex));
    }

    RWRETURN(0);
}

/**
 * \ingroup rpanisot
 * \ref RpAnisotPluginAttach is used to attach the anisot plugin to
 * the RenderWare Graphics system to enable anisotropic texturing values to be stored
 * with textures.
 *
 * \if xbox
 * These texturing values are set as the D3DTSS_MAXANISOTROPY texture stage state.
 * \endif
 *
 * \if d3d8
 * These texturing values are set as the D3DTSS_MAXANISOTROPY texture stage state.
 * \endif
 *
 * \if d3d9
 * These texturing values are set as the D3DTSS_MAXANISOTROPY texture stage state.
 * \endif
 *
 * The plugin must be attached between initializing the system with
 * \ref RwEngineInit and opening it with \ref RwEngineOpen.
 *
 * Note that the include file rpanisot.h is required and must be included by an
 * application wishing to use this facility.  The anisot library is contained
 * in the file rpanisot.lib.
 *
 * \return \ref RwBool, TRUE if successful, or FALSE if there is an error
 *
 *
 *
 */
RwBool
RpAnisotPluginAttach(void)
{
    RwInt32             offset;

    RWAPIFUNCTION(RWSTRING("RpAnisotPluginAttach"));

    offset =
        RwEngineRegisterPlugin(0, rwPLUGIN_ID, anisotOpen,
                               anisotClose);
    if (offset < 0)
    {
        RWRETURN(FALSE);
    }

    anisotTextureOffset =
        RwTextureRegisterPlugin(sizeof(RwUInt8), rwPLUGIN_ID,
                                anisotConstruct, anisotDestruct,
                                anisotCopy);
    if (anisotTextureOffset < 0)
    {
        RWRETURN(FALSE);
    }

    anisotTextureStreamOffset =
        RwTextureRegisterPluginStream(rwPLUGIN_ID, anisotRead,
                                      anisotWrite, anisotGetSize);

    if (anisotTextureStreamOffset < 0)
    {
        RWRETURN(FALSE);
    }

#if defined(XBOX_DRVMODEL_H)
    _rwXboxSetTextureAnisotropyOffset(anisotTextureOffset);
#elif defined(D3D8_DRVMODEL_H)
    _rwD3D8SetTextureAnisotropyOffset(anisotTextureOffset);
#elif defined(D3D9_DRVMODEL_H)
    _rwD3D9SetTextureAnisotropyOffset(anisotTextureOffset);
#elif defined(OPENGL_DRVMODEL_H)
    _rwOpenGLSetTextureAnisotropyOffset(anisotTextureOffset);
#endif

    RWRETURN(TRUE);
}
