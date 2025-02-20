/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   rtnormmap.c                                                -*
 *-                                                                         -*
 *-  Purpose :   RtNormMap toolkit                                          -*
 *-                                                                         -*
 *===========================================================================*/

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/

#include <rwcore.h>
#include <rpdbgerr.h>

#include <rtnormmap.h>

#define COLORTOHEIGHT(_color, _bumpCoef) \
    (((_bumpCoef) * ((_color)->red + (_color)->green + (_color)->blue)) / 3)

/**
 * \ingroup rtnormmap
 * \page rtnormmapoverview RtNormMap Toolkit Overview
 *
 * \par Requirements for rtnormmap library
 * \li \b Headers: rwcore.h, rtnormmap.h
 * \li \b Libraries: rwcore, rtnormmap
 *
 * \subsection tnormmapoverview Overview
 * This toolkit provides facilities for creating normal maps. Normal maps are textures
 * that store a normal per pixel encoded as a color, converting the components
 * of the normals from the range [-1..1] to [0..255].
 *
 * Normal maps created using this toolkit could be used with the \ref rpnormmap
 * plugin to calculate per pixel lighting.
 *
 * \see \ref rpnormmap
 */

static void
V3dNormalize(RwV3d *vout, const RwV3d *vin)
{
    const RwReal length = RwV3dDotProductMacro(vin, vin);

    RWFUNCTION(RWSTRING("V3dNormalize"));

    if (length != 1.f)
    {
        RwReal recip;

        rwInvSqrtMacro(&recip, length);

        RwV3dScaleMacro(vout, vin, recip);
    }
    else
    {
        *vout = *vin;
    }

    RWRETURNVOID();
}

static void
V3dNormalizeInPlace(RwV3d *vin)
{
    const RwReal length = RwV3dDotProductMacro(vin, vin);

    RWFUNCTION(RWSTRING("V3dNormalizeInPlace"));

    if (length != 1.f)
    {
        RwReal recip;

        rwInvSqrtMacro(&recip, length);

        RwV3dScaleMacro(vin, vin, recip);
    }

    RWRETURNVOID();
}

static void
PackPixel(RwUInt32 *pixel, const RwV3d *vector)
{
    RwV3d normalizedVector;
    RwInt32 red, green, blue;

    RWFUNCTION(RWSTRING("PackPixel"));

    V3dNormalize(&normalizedVector, vector);

    blue = RwFastRealToUInt32(((normalizedVector.x + 1) * 255) / 2);
    if (blue > 255)
    {
        blue = 255;
    }
    else if(blue < 0)
    {
        blue = 0;
    }

    green = RwFastRealToUInt32(((normalizedVector.y + 1) * 255) / 2);
    if (green > 255)
    {
        green = 255;
    }
    else if(green < 0)
    {
        green = 0;
    }

    red = RwFastRealToUInt32(((normalizedVector.z + 1) * 255) / 2);
    if (red > 255)
    {
        red = 255;
    }
    else if(red < 0)
    {
        red = 0;
    }

    *pixel = (0xff << 24) | (((RwUInt32)red) << 16) | (((RwUInt32)green) << 8) | ((RwUInt32)blue);

    RWRETURNVOID();
}

/**
 * \ingroup rtnormmap
 * \ref RtNormMapCreateFromImage
 * creates a \ref RwImage containing a normal map generated
 * from a regular image using average pixel color diferences.
 *
 * \param image pointer to the source \ref RwImage
 * \param clamp TRUE if the normal map needs to be clamped at the edges
 * \param bumpiness level of bumpiness of the resulting normal map
 *
 * \return pointer to the resulting normal map image
 *
 * \see \ref rpnormmap
 */
RwImage *
RtNormMapCreateFromImage(RwImage *image,
                         RwBool clamp,
                         RwReal bumpiness)
{
    RwImage *normalMap = NULL;
    RwUInt32 width, height;

    RWAPIFUNCTION(RWSTRING("RtNormMapCreateFromImage"));
    RWASSERT(image != NULL);

    width = RwImageGetWidth(image);
    height = RwImageGetHeight(image);

    normalMap = RwImageCreate(width, height, 32);

    if (normalMap != NULL)
    {
        const RwReal bumpCoef = bumpiness * 20.f;

        const RwRGBA *src;
        RwUInt8 *dest;
        RwUInt32 stride, x, y;
        RwV3d vl, vr, vt, vb, vc;
        RwImage *image32;

        if (RwImageGetDepth(image) < 32 ||
            (RwUInt32)RwImageGetWidth(image) != width ||
            (RwUInt32)RwImageGetHeight(image) != height)
        {
            image32 = RwImageCreateResample(image, width, height);
        }
        else
        {
            image32 = image;
        }

        /* Create normal data */
        src = (const RwRGBA *)RwImageGetPixels(image32);

        RwImageAllocatePixels(normalMap);
        dest = (RwUInt8 *)RwImageGetPixels(normalMap);
        stride = 4 * width;

        vl.x = -255;
        vl.y = 0;
        vl.z = 0;

        vr.x = 255;
        vr.y = 0;
        vr.z = 0;

        vt.x = 0;
        vt.y = -255;
        vt.z = 0;

        vb.x = 0;
        vb.y = 255;
        vb.z = 0;

        vc.x = 0;
        vc.y = 0;
        vc.z = 0;

        for (y = 0; y < height; y++)
        {
            RwUInt32 *currentNormal = (RwUInt32 *)(dest + y * stride);
            for (x = 0; x < width; x++)
            {
                RwV3d normal = {0.0f, 0.0f, 0.0f};
                RwV3d position01;
                RwV3d position11;
                RwV3d normaltmp;
                const RwRGBA *color;

                /* Use average color  */
                if (x == 0)
                {
                    if (clamp)
                    {
                        color = src;
                    }
                    else
                    {
                        color = (src + (width - 1));
                    }
                }
                else
                {
                    color = (src - 1);
                }
                vl.z = COLORTOHEIGHT(color, bumpCoef);

                if (x == (width - 1))
                {
                    if (clamp)
                    {
                        color = src;
                    }
                    else
                    {
                        color = (src - (width - 1));
                    }
                }
                else
                {
                    color = (src + 1);
                }
                vr.z = COLORTOHEIGHT(color, bumpCoef);

                if (y == 0)
                {
                    if (clamp)
                    {
                        color = src;
                    }
                    else
                    {
                        color = (src + width * (height - 1));
                    }
                }
                else
                {
                    color = (src - width);
                }
                vt.z = COLORTOHEIGHT(color, bumpCoef);

                if (y == (height - 1))
                {
                    if (clamp)
                    {
                        color = src;
                    }
                    else
                    {
                        color = (src - width * (height - 1));
                    }
                }
                else
                {
                    color = (src + width);
                }
                vb.z = COLORTOHEIGHT(color, bumpCoef);

                vc.z = COLORTOHEIGHT(src, bumpCoef);

                /**/
                RwV3dSub(&position01, &vl, &vc);
                V3dNormalizeInPlace(&position01);

                RwV3dSub(&position11, &vt, &vc);
                V3dNormalizeInPlace(&position11);

                RwV3dCrossProduct(&normaltmp, &position01, &position11);
                V3dNormalizeInPlace(&normaltmp);

                RwV3dAdd(&normal, &normal, &normaltmp);

                /**/
                RwV3dSub(&position01, &vr, &vc);
                V3dNormalizeInPlace(&position01);

                RwV3dSub(&position11, &vt, &vc);
                V3dNormalizeInPlace(&position11);

                RwV3dCrossProduct(&normaltmp, &position11, &position01);
                V3dNormalizeInPlace(&normaltmp);

                RwV3dAdd(&normal, &normal, &normaltmp);

                /**/
                RwV3dSub(&position01, &vr, &vc);
                V3dNormalizeInPlace(&position01);

                RwV3dSub(&position11, &vb, &vc);
                V3dNormalizeInPlace(&position11);

                RwV3dCrossProduct(&normaltmp, &position01, &position11);
                V3dNormalizeInPlace(&normaltmp);

                RwV3dAdd(&normal, &normal, &normaltmp);

                /**/
                RwV3dSub(&position01, &vl, &vc);
                V3dNormalizeInPlace(&position01);

                RwV3dSub(&position11, &vb, &vc);
                V3dNormalizeInPlace(&position11);

                RwV3dCrossProduct(&normaltmp, &position11, &position01);
                V3dNormalizeInPlace(&normaltmp);

                RwV3dAdd(&normal, &normal, &normaltmp);

                if (normal.x != 0.0f ||
                    normal.y != 0.0f ||
                    normal.z != 0.0f)
                {
                    PackPixel(currentNormal, &normal);
                }
                else
                {
                    *currentNormal = 0xff8080ff;
                }

                src++;
                currentNormal++;
            }
        }

        if (image32 != image)
        {
            RwImageDestroy(image32);
        }
    }

    RWRETURN(normalMap);
}
