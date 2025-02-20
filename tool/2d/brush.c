/*
 * Functionality for 2D rendering
 *
 * Copyright (c) 1998 Criterion Software Ltd.
 */

/****************************************************************************
 *                                                                          *
 *  Module  :   brush.c                                                     *
 *                                                                          *
 *  Purpose :   fill state                                                  *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
 */
#include <string.h>

#include <rwcore.h>
#include <rpdbgerr.h>
#include <rpworld.h>

#include "rt2d.h"
#include "font.h"
#include "path.h"
#include "tri.h"
#include "gstate.h"

#include "brush.h"

/****************************************************************************
 Local Types
 */

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */

#define FILE_CURRENT_VERSION 0x01
#define FILE_LAST_SUPPORTED_VERSION 0x01

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   Functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/****************************************************************************/
#define RW2Dx

#if (defined(RW2D) && defined(RWDEBUG))

static Rt2dBrush *
BrushPrint2d(Rt2dBrush *brush)
{
    RWFUNCTION(RWSTRING("BrushPrint2d"));

    if (brush)
    {
            RWMESSAGE(("top  [%1.5f %1.5f %1.5f %1.5f][%1.5f %1.5f]",
                        brush->top.col.red,
                        brush->top.col.green,
                        brush->top.col.blue,
                        brush->top.col.alpha,
                        brush->top.uv.x,
                        brush->top.uv.y));

            RWMESSAGE(("dtop [%1.5f %1.5f %1.5f %1.5f][%1.5f %1.5f]",
                        brush->dtop.col.red,
                        brush->dtop.col.green,
                        brush->dtop.col.blue,
                        brush->dtop.col.alpha,
                        brush->dtop.uv.x,
                        brush->dtop.uv.y));

            RWMESSAGE(("bottom  [%1.5f %1.5f %1.5f %1.5f][%1.5f %1.5f]",
                        brush->bottom.col.red,
                        brush->bottom.col.green,
                        brush->bottom.col.blue,
                        brush->bottom.col.alpha,
                        brush->bottom.uv.x,
                        brush->bottom.uv.y));

            RWMESSAGE(("dbottom [%1.5f %1.5f %1.5f %1.5f][%1.5f %1.5f]",
                        brush->dbottom.col.red,
                        brush->dbottom.col.green,
                        brush->dbottom.col.blue,
                        brush->dbottom.col.alpha,
                        brush->dbottom.uv.x,
                        brush->dbottom.uv.y));

            RWMESSAGE(("colorCaches [%3d %3d %3d]",
                        brush->colorCache.red,
                        brush->colorCache.green,
                        brush->colorCache.blue,
                        brush->colorCache.alpha));

            RWMESSAGE(("halfwidth [%3.3f]",
                        brush->halfwidth));

            RWMESSAGE(("refCount [%3d]",
                        brush->refCount));
    }

    RWRETURN(brush);
}

#define RT2DBRUSHPRINT(brush) BrushPrint2d(brush)

#endif /* ( defined(RW2D) && defined(RWDEBUG) ) */

#if (!defined(RT2DBRUSHPRINT))
#define RT2DBRUSHPRINT(brush) /* No op */
#endif /* (!defined(RT2DBRUSHPRINT)) */

/****************************************************************************/

void
_rt2dBrushClose(void)
{
    RWFUNCTION(RWSTRING("_rt2dBrushClose"));

    RwFreeListDestroy(Rt2dGlobals.brushFreeList);

    Rt2dGlobals.brushFreeList = (RwFreeList *)NULL;

    RwFree(Rt2dGlobals.brushVertexCache);

    Rt2dGlobals.brushVertexCache = (RwIm3DVertex *)NULL;

    RWRETURNVOID();
}

/****************************************************************************/

static RwInt32 _rt2dBrushFreeListBlockSize = 8,
               _rt2dBrushFreeListPreallocBlocks = 1;
static RwFreeList _rt2dBrushFreeList;

/**
 * \ingroup rt2d
 * \ref Rt2dBrushSetFreeListCreateParams allows the developer to specify
 * how many \ref Rt2dBrush s to preallocate space for.
 * Call before \ref RwEngineInit.
 *
 * \param blockSize  number of entries per freelist block.
 * \param numBlocksToPrealloc  number of blocks to allocate on
 * \ref RwFreeListCreateAndPreallocateSpace.
 *
 * \see RwFreeList
 *
 */
void
Rt2dBrushSetFreeListCreateParams( RwInt32 blockSize, RwInt32 numBlocksToPrealloc )
{
    /* cannot use debugging macros since the debugger is not initialized before RwEngineInit */
    /*
    RWAPIFUNCTION( RWSTRING( "Rt2dBrushSetFreeListCreateParams" ) );
    */

    _rt2dBrushFreeListBlockSize = blockSize;
    _rt2dBrushFreeListPreallocBlocks = numBlocksToPrealloc;

    /*
    RWRETURNVOID();
    */
}


RwBool
_rt2dBrushOpen(void)
{
    RWFUNCTION(RWSTRING("_rt2dBrushOpen"));

    Rt2dGlobals.brushVertexCache =
        (RwIm3DVertex *)(RwMalloc(sizeof(RwIm3DVertex) * VERTEXCACHESIZE,
        rwID_2DPLUGIN | rwMEMHINTDUR_GLOBAL));

    Rt2dGlobals.brushFreeList =
        RwFreeListCreateAndPreallocateSpace(sizeof(Rt2dBrush), _rt2dBrushFreeListBlockSize, 16,
        _rt2dBrushFreeListPreallocBlocks, &_rt2dBrushFreeList,
        rwID_2DBRUSH | rwMEMHINTDUR_GLOBAL);

    RWRETURN(TRUE);
}

/****************************************************************************/

/**
 * \ingroup rt2dbrush
 * \ref Rt2dBrushCreate
 * is used to create a new brush for painting and filling
 * paths. Note that the primitive resulting from a painted path is
 * topologically equivalent to a rectangle.
 *
 * A newly created brush has a number of default properties but these can
 * be defined by an application using the available API functions.
 *
 * Relevant properties are:
 *
 * \par Brush width
 * Used only when painting a path; width is measured perpendicular to
 * path and the path lies at the center of the painted primitive.
 * Default: 1.0.
 *
 * \par Brush colors
 * Four colors are required: For painting they define the colors at the
 * corners of the resulting primitive; for filling they define the colors
 * at the corners of the path's bounding-box. In both cases, the corner
 * colors are ordered anticlockwise and interior colors are determined by
 * bilinear interpolation.
 * Default: All colors are opaque white.
 *
 * \par Brush texture coordinates
 * Four (u, v) pairs are required: For painting they define the texture
 * coordinates at the corners of the resulting primitive; for filling
 * they define the texture coordinates at the corners of the path's
 * bounding-box. In both cases, corner texture coordinates are ordered
 * anticlockwise and interior coordinates are determined by bilinear
 * interpolation.
 * Default: All texture coordinates are zero.
 *
 * \par Brush texture image
 * Image used for texturing; texture coordinates must also be properly
 * defined.
 * Default:NULL.
 *
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \return a pointer to the new brush if successful or NULL if there is
 * an error.
 * \see Rt2dBrushDestroy
 * \see Rt2dBrushSetWidth
 * \see Rt2dBrushSetRGBA
 * \see Rt2dBrushSetUV
 * \see Rt2dBrushSetTexture
 */
Rt2dBrush          *
Rt2dBrushCreate(void)
{
    Rt2dBrush          *brush;

    RWAPIFUNCTION(RWSTRING("Rt2dBrushCreate"));

    brush = (Rt2dBrush *)RwFreeListAlloc(Rt2dGlobals.brushFreeList,
                                rwID_2DBRUSH | rwMEMHINTDUR_EVENT);

    if (brush)
    {
        RwRGBA              colour;
        RwV2d               uv;

        memset(brush, 0, sizeof(Rt2dBrush));

        colour.red = 255;
        colour.green = 255;
        colour.blue = 255;
        colour.alpha = 255;
        Rt2dBrushSetRGBA(brush, &colour, &colour, &colour, &colour);

        uv.x = (RwReal) (0.0);
        uv.y = (RwReal) (0.0);
        Rt2dBrushSetUV(brush, &uv, &uv, &uv, &uv);
        Rt2dBrushSetTexture(brush, (RwTexture *)NULL);

        Rt2dBrushSetWidth(brush, (RwReal) (1.0));

        brush->refCount = 1;

        RWRETURN(brush);
    }

    RWRETURN((Rt2dBrush *)NULL);
}

/****************************************************************************/

/**
 * \ingroup rt2dbrush
 * \ref Rt2dBrushDestroy
 * is used to destroy the specified brush. All brushes
 * created by an application must be destroyed before the application
 * closes down.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param brush  Pointer to the brush
 * \return TRUE if successful or FALSE if there is an error
 * \see Rt2dBrushCreate
 * \see Rt2dOpen
 * \see Rt2dClose
 */
RwBool
Rt2dBrushDestroy(Rt2dBrush * brush)
{
    RWAPIFUNCTION(RWSTRING("Rt2dBrushDestroy"));

    if (--brush->refCount <= 0)
    {
        /* Should not have a -v ref count */
        RWASSERT(brush->refCount == 0);

        if( brush->texture )
        {
            RwTextureDestroy(brush->texture);
        }
        RwFreeListFree(Rt2dGlobals.brushFreeList, brush);

        RWRETURN(TRUE);
    }

    RWRETURN(FALSE);
}

#ifndef SKY
static void
_rt2dBrushCheckFlatRGBA(Rt2dBrush * brush)
{
    RWFUNCTION(RWSTRING("_rt2dBrushCheckFlatRGBA"));

    /* setup vertex colours if possible; tests shuffled for efficiency */
    if ((brush->dtop.col.red    == 0.0f) &&
        (brush->dbottom.col.red == 0.0f) &&
        (brush->top.col.red     == brush->bottom.col.red) &&
        (brush->dtop.col.green    == 0.0f) &&
        (brush->dbottom.col.green == 0.0f) &&
        (brush->top.col.green     == brush->bottom.col.green) &&
        (brush->dtop.col.blue    == 0.0f) &&
        (brush->dbottom.col.blue == 0.0f) &&
        (brush->top.col.blue     == brush->bottom.col.blue) &&
        (brush->dtop.col.alpha    == 0.0f) &&
        (brush->dbottom.col.alpha == 0.0f) &&
        (brush->top.col.alpha     == brush->bottom.col.alpha)
       )
    {
        brush->flag &= ~FIELDRGBA;
    }
    else
    {
        brush->flag |= FIELDRGBA;
    }

    RWRETURNVOID();
}
#endif /* SKY */

/* _rt2dBrushTransformRGBARealDirect
 *   brush   - brush to be updated
 *   col0, col1, col2, col3
 *           - RwRGBAReals in the range 0.0f to 255.0f
 *   mult    - Scaling RwRGBAReal in the range 0.0f to 1.0f
 *   offset  - Offset RwRGBAReal in the range 0.0f to 255.0f
 */
Rt2dBrush *
_rt2dBrushTransformRGBARealDirect(Rt2dBrush *brush,
                                  RwRGBAReal *col0, RwRGBAReal *col1,
                                  RwRGBAReal *col2, RwRGBAReal *col3,
                                  RwRGBAReal *mult, RwRGBAReal *offset)
{
    RWFUNCTION(RWSTRING("_rt2dBrushTransformRGBARealDirect"));

    RWASSERT(brush);

    RwRGBARealTransformAssignAndClamp(&brush->top.col,    col3, mult, offset);
    RwRGBARealTransformAssignAndClamp(&brush->dtop.col,   col2, mult, offset);
    RwRGBARealTransformAssignAndClamp(&brush->bottom.col, col0, mult, offset);
    RwRGBARealTransformAssignAndClamp(&brush->dbottom.col,col1, mult, offset);

    #if defined(_XBOX)
    #define _STRINGIFY(X) #X
    #define _STRINGIFY_EXP(X) _STRINGIFY(X)
    #pragma message ( __FILE__ "(" _STRINGIFY_EXP(__LINE__) "): MS compiler bug work around\n")
    {
        /*
         * Silly fix for an "INTERNAL COMPILER ERROR" on the Xbox version, it
         * seems to get confused when trying to optimize the expanded macros.
         */

        RwRGBA *colorCache = &brush->colorCache;
        MyRGBAFromRGBAReal(colorCache, &brush->top.col);
    }
    #else /* defined(_XBOX) */

    MyRGBAFromRGBAReal(&brush->colorCache, &brush->top.col);

    #endif /* defined(_XBOX) */

    /* This operation does not affect flatness of brush, so don't touch the flag
     * via _rt2dBrushCheckFlatRGBA
     */

    RWRETURN(brush);
}

/**
 * \ingroup rt2dbrush
 * \ref Rt2dBrushSetRGBA
 * is used to define the color of the specified brush.
 * Four colors are required: For painting they define the colors at the
 * corners of the resulting primitive; for filling they define the colors
 * at the corners of the path's bounding-box. In both cases, corner
 * colors are ordered anticlockwise and interior colors are determined by
 * bilinear interpolation.
 * By default all colors are opaque white.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 *
 * \param brush  Pointer to the brush.
 * \param col0  Pointer to the first color.
 * \param col1  Pointer to the second color.
 * \param col2  Pointer to the third color.
 * \param col3  Pointer to the fourth color.
 *
 * \return pointer to the brush if successful or NULL if there is an
 * error.
 *
 * \see Rt2dBrushSetWidth
 * \see Rt2dBrushSetUV
 * \see Rt2dBrushSetTexture
 * \see Rt2dBrushCreate
 * \see Rt2dPathStroke
 * \see Rt2dPathFill
 */
Rt2dBrush          *
Rt2dBrushSetRGBA(Rt2dBrush * brush, RwRGBA * col0, RwRGBA * col1,
                 RwRGBA * col2, RwRGBA * col3)
{
    RWAPIFUNCTION(RWSTRING("Rt2dBrushSetRGBA"));

    RWASSERT(brush);

    MyRGBARealFromRGBA(&brush->top.col, col3);
    MyRGBARealFromRGBA(&brush->dtop.col, col2);
    RwRGBARealSub(&brush->dtop.col, &brush->dtop.col, &brush->top.col);

    MyRGBARealFromRGBA(&brush->bottom.col, col0);
    MyRGBARealFromRGBA(&brush->dbottom.col, col1);
    RwRGBARealSub(&brush->dbottom.col, &brush->dbottom.col,
                  &brush->bottom.col);

    memcpy(&brush->colorCache, col3, sizeof(RwRGBA));

#ifndef SKY

    _rt2dBrushCheckFlatRGBA(brush);

#endif /* SKY */

    RWRETURN(brush);
}

/****************************************************************************/

/**
 * \ingroup rt2dbrush
 * \ref Rt2dBrushSetUV
 * is used to define the texture coordinates for the
 * specified brush.
 * Four (u, v) pairs are required: For painting they define the texture
 * coordinates at the corners of the resulting primitive; for filling
 * they define the texture coordinates at the corners of the path's
 * bounding-box. In both cases, corner texture coordinates are ordered
 * anticlockwise and interior coordinates are determined by bilinear
 * interpolation.
 * By default all texture coordinates are zero.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param brush  Pointer to the brush.
 * \param uv0  Pointer to the first texture coordinates.
 * \param uv1  Pointer to the second texture coordinates.
 * \param uv2  Pointer to the third texture coordinates.
 * \param uv3  Pointer to the fourth texture coordinates.
 * \return pointer to the brush if successful or NULL if there is an
 * error.
 * \see Rt2dBrushSetTexture
 * \see Rt2dBrushSetRGBA
 * \see Rt2dBrushSetWidth
 * \see Rt2dBrushCreate
 */
Rt2dBrush          *
Rt2dBrushSetUV(Rt2dBrush * brush, RwV2d * uv0, RwV2d * uv1, RwV2d * uv2,
               RwV2d * uv3)
{
    RWAPIFUNCTION(RWSTRING("Rt2dBrushSetUV"));

    RWASSERT(brush);

    brush->top.uv = *uv3;
    RwV2dSub(&brush->dtop.uv, uv2, uv3);

    brush->bottom.uv = *uv0;
    RwV2dSub(&brush->dbottom.uv, uv1, uv0);

    /* setup vertex UV if possible */
    if ((brush->dtop.uv.x == 0.0f) &&
        (brush->dtop.uv.y == 0.0f) &&
        (brush->dbottom.uv.x == 0.0f) && (brush->dbottom.uv.y == 0.0f))
    {
        brush->flag &= ~FIELDUV;
    }
    else
    {
        brush->flag |= FIELDUV;
    }

    RWRETURN(brush);
}

/****************************************************************************/

/**
 * \ingroup rt2dbrush
 * \ref Rt2dBrushSetTexture
 * is used to define a texture for the specified
 * brush using the given image. For texturing to work properly,
 * appropriate texture coordinates must also be defined.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param brush  Pointer to the brush.
 * \param texture  Pointer to the texture.
 * \return pointer to the brush if successful or NULL if there is an
 * error.
 * \see Rt2dBrushSetUV
 * \see Rt2dBrushSetRGBA
 * \see Rt2dBrushSetWidth
 * \see Rt2dBrushCreate
 */
Rt2dBrush          *
Rt2dBrushSetTexture(Rt2dBrush * brush, RwTexture * texture)
{
    RWAPIFUNCTION(RWSTRING("Rt2dBrushSetTexture"));

    RWASSERT(brush);

    if (texture)
    {
        brush->flag |= FIELDUV;
        RwTextureAddRef(texture);
    }
    else
    {
        brush->flag &= ~FIELDUV;
    }

    /* Ref count goes down for the previous texture*/
    if( NULL != brush->texture)
    {
        RwTextureDestroy(brush->texture);
    }

    brush->texture = texture;

    RWRETURN(brush);
}

/**
 * \ingroup rt2dbrush
 * \ref Rt2dBrushStreamGetSize is used to  determine the size in bytes
 * of the binary representation of the given brush. This is used in
 * the binary chunk header to indicate the size of the chunk. The size does
 * include the size of the chunk header.
 *
 *
 * \param brush Pointer to the brush.
 *
 * \return Returns a RwUInt32 value equal to the chunk size (in bytes) of
 * the brush or zero if there is an error.
 *
 * \see Rt2dBrushStreamRead
 * \see Rt2dBrushStreamWrite
 *
 */
RwUInt32
Rt2dBrushStreamGetSize(Rt2dBrush *brush)
{
    RwUInt32 size = 0;
    RWAPIFUNCTION(RWSTRING("Rt2dBrushStreamGetSize"));
    RWASSERT(brush);


    size = sizeof(_rt2dStreamBrush);

    if(NULL != brush->texture)
    {
        size +=
            RwTextureStreamGetSize(brush->texture);
    }
    size += rwCHUNKHEADERSIZE;

    RWRETURN(size);
}

static void
rt2dShadeParametersCopy(rt2dShadeParameters *dst, rt2dShadeParameters *src)
{
    RWFUNCTION(RWSTRING("rt2dShadeParametersCopy"))
    RWASSERT(dst);
    RWASSERT(src);

    memcpy(&(dst->col), &(src->col), sizeof(RwRGBAReal));
    RwV2dAssign(&(dst->uv),&(src->uv));

    RWRETURNVOID();
}

static void
rt2dShadeParamersConvertToFloat32(rt2dShadeParameters *dst)
{
    RWFUNCTION(RWSTRING("rt2dShadeParamersConvertToFloat32"))
    RWASSERT(dst);

    (void)RwMemRealToFloat32(&dst->col.red, sizeof(dst->col.red));
    (void)RwMemRealToFloat32(&dst->col.green, sizeof(dst->col.green));
    (void)RwMemRealToFloat32(&dst->col.blue, sizeof(dst->col.blue));
    (void)RwMemRealToFloat32(&dst->col.alpha, sizeof(dst->col.alpha));

    (void)RwMemRealToFloat32(&dst->uv.x, sizeof(dst->uv.x));
    (void)RwMemRealToFloat32(&dst->uv.y, sizeof(dst->uv.y));

    RWRETURNVOID();

}

static void
rt2dShadeParamersConvertFromFloat32(rt2dShadeParameters *dst)
{
    RWFUNCTION(RWSTRING("rt2dShadeParamersConvertFromFloat32"))
    RWASSERT(dst);

    (void)RwMemFloat32ToReal(&dst->col.red, sizeof(dst->col.red));
    (void)RwMemFloat32ToReal(&dst->col.green, sizeof(dst->col.green));
    (void)RwMemFloat32ToReal(&dst->col.blue, sizeof(dst->col.blue));
    (void)RwMemFloat32ToReal(&dst->col.alpha, sizeof(dst->col.alpha));

    (void)RwMemFloat32ToReal(&dst->uv.x, sizeof(dst->uv.x));
    (void)RwMemFloat32ToReal(&dst->uv.y, sizeof(dst->uv.y));

    RWRETURNVOID();

}

/**
 * \ingroup rt2dbrush
 * \ref Rt2dBrushStreamWrite is used to write the specified brush to the
 * given binary stream.
 * Note that the stream will have been opened prior to this function call.
 *
 *
 * \param brush  Pointer to the brush.
 * \param stream  Pointer to the stream.
 *
 * \return Returns pointer to the brush if successful or NULL if
 * there is an error.
 *
 * \see Rt2dBrushStreamRead
 * \see Rt2dBrushStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 *
 */
Rt2dBrush *
Rt2dBrushStreamWrite(Rt2dBrush *brush, RwStream *stream)
{
    _rt2dStreamBrush sBrush;
    RwUInt32         size;

    RWAPIFUNCTION(RWSTRING("Rt2dBrushStreamWrite"));

    if (NULL == brush && NULL == stream)
    {
        RWRETURN((Rt2dBrush *)NULL);
    }

    size = Rt2dBrushStreamGetSize(brush);

    if (!RwStreamWriteChunkHeader(stream, rwID_2DBRUSH, size))
    {
        RWRETURN((Rt2dBrush *)NULL);
    }

    /* Set and Write Header */
#ifdef USE_STRUCT
    if (!RwStreamWriteChunkHeader
        (stream, rwID_STRUCT, sizeof(_rt2dStreamBrush)))
    {
        RWRETURN((Rt2dBrush *)NULL);
    }
#endif
    sBrush.version = FILE_CURRENT_VERSION;
    rt2dShadeParametersCopy(&(sBrush.top), &(brush->top));
    rt2dShadeParametersCopy(&sBrush.dtop, &brush->dtop);
    rt2dShadeParametersCopy(&sBrush.bottom, &brush->bottom);
    rt2dShadeParametersCopy(&sBrush.dbottom, &brush->dbottom);
    sBrush.flag = brush->flag;
    sBrush.halfwidth = brush->halfwidth;
    sBrush.gotTexture = (NULL != brush->texture);

    (void)RwMemRealToFloat32(&sBrush.halfwidth, sizeof(sBrush.halfwidth));

    rt2dShadeParamersConvertToFloat32(&sBrush.top);
    rt2dShadeParamersConvertToFloat32(&sBrush.dtop);
    rt2dShadeParamersConvertToFloat32(&sBrush.bottom);
    rt2dShadeParamersConvertToFloat32(&sBrush.dbottom);

    (void)RwMemLittleEndian32(&sBrush, sizeof(sBrush));

    if (!RwStreamWrite(stream, &sBrush, sizeof(_rt2dStreamBrush)))
    {
        RWRETURN((Rt2dBrush *)NULL);
    }

    if(NULL != brush->texture)
    {
        if (!RwTextureStreamWrite(brush->texture,stream))
        {
            RWRETURN((Rt2dBrush *)NULL);
        }
    }


    RWRETURN(brush);
}

static Rt2dBrush *
rt2dBrushStreamRead_V0x01( Rt2dBrush *brush, _rt2dStreamBrush *sBrush, RwStream *stream)
{
    RWFUNCTION(RWSTRING("rt2dBrushStreamRead_V0x01"));
    RWASSERT(stream);
    RWASSERT(sBrush);

    rt2dShadeParametersCopy(&brush->top, &sBrush->top);
    rt2dShadeParametersCopy(&brush->dtop, &sBrush->dtop);
    rt2dShadeParametersCopy(&brush->bottom, &sBrush->bottom);
    rt2dShadeParametersCopy(&brush->dbottom, &sBrush->dbottom);

    MyRGBAFromRGBAReal(&brush->colorCache, &brush->top.col);

    brush->flag = sBrush->flag;
#ifndef SKY
    _rt2dBrushCheckFlatRGBA(brush);
#endif
    brush->halfwidth = sBrush->halfwidth;

    if(sBrush->gotTexture)
    {
        RwTexture *texture;
        RwRaster *raster;

        if (!RwStreamFindChunk(stream,
                               rwID_TEXTURE,
                               (RwUInt32 *)NULL,
                               (RwUInt32 *)NULL))
        {
            RWRETURN((Rt2dBrush *)NULL);
        }

        texture = RwTextureStreamRead(stream);

        if (texture)
        {
            /* If we have a raster, need to make sure the texture is clamped if it's
             * non-power-two-size. Some platforms (eg GCN) don't support tiled textures
             * with non-power-of-two sizes, and assert
             * Need to check for presence of raster first, as can't make this determination
             * if the texture is not loaded
             */
            raster = RwTextureGetRaster(texture);
            if (raster)
            {
                RwBool textureSizeIsPowerOf2;
                RwUInt32 height, width;

                height = RwRasterGetHeight(raster);
                width = RwRasterGetWidth(raster);

                textureSizeIsPowerOf2
                   =    (0 == (height & (height-1)))
                     && (0 == (width & (width-1)));

                if (!textureSizeIsPowerOf2)
                {
                    /* Some platforms do not support non-power-of-two textures
                     * with tiled textures, so need to default addressing mode
                     * for these to clamped
                     */
                    RwTextureSetAddressing(texture, rwTEXTUREADDRESSCLAMP);
                }
            }
        }

        brush->texture = texture;
        brush->flag |= FIELDUV;

    }

    RWRETURN(brush);
}

Rt2dBrush *
_rt2dBrushStreamReadTo(Rt2dBrush *brush,RwStream *stream)
{
    _rt2dStreamBrush sBrush;

    RWFUNCTION(RWSTRING("_rt2dBrushStreamReadTo"));
#ifdef USE_STRUCT
    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version))
    {
        RWRETURN((Rt2dBrush *)NULL);
    }
#endif
    /* Read the Path header */
    if (RwStreamRead(stream, &sBrush, sizeof(sBrush)) != sizeof(sBrush))
    {
        RWRETURN((Rt2dBrush *)NULL);
    }

    /* Convert it */
    (void)RwMemNative32(&sBrush, sizeof(sBrush));

    rt2dShadeParamersConvertFromFloat32(&sBrush.top);
    rt2dShadeParamersConvertFromFloat32(&sBrush.dtop);
    rt2dShadeParamersConvertFromFloat32(&sBrush.bottom);
    rt2dShadeParamersConvertFromFloat32(&sBrush.dbottom);

    MyRGBAFromRGBAReal(&brush->colorCache, &brush->top.col);

    (void)RwMemFloat32ToReal(&sBrush.halfwidth, sizeof(sBrush.halfwidth));

    RWASSERT(sBrush.version <= FILE_LAST_SUPPORTED_VERSION && sBrush.version >= FILE_CURRENT_VERSION);

    switch(sBrush.version)
    {
        case  0x01:                     /* FILE_CURRENT_VERSION */
            RWRETURN(rt2dBrushStreamRead_V0x01(brush, &sBrush,stream));
            break;
        default:
            RWMESSAGE(((RWSTRING("Rt2dBrush Unsupported Version %d\n")), sBrush.version));
            break;
    }

    RWRETURN((Rt2dBrush*)NULL);
}

/**
 * \ingroup rt2dbrush
 * \ref Rt2dBrushStreamRead is used to read a brush object from the
 * specified binary stream. Note that prior to this function call a
 * brush chunk must be found in the stream using the
 * \ref RwStreamFindChunk API function.
 *
 * The sequence to locate and read a brush from a binary stream is
 * as follows:
 * \code
   RwStream  *stream;
   Rt2dBrush *newBrush;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_2DBRUSH, NULL, NULL) )
       {
           newBrush = Rt2dBrushStreamRead(stream);
       }

       RwStreamClose(stream, NULL);
   }
   \endcode
 *
 * \param stream  Pointer to the stream.
 *
 * \return Returns pointer to the brush if successful or NULL if there is an error.
 *
 * \see Rt2dBrushStreamWrite
 * \see Rt2dBrushStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 */
Rt2dBrush *
Rt2dBrushStreamRead(RwStream *stream)
{
    Rt2dBrush *brush = Rt2dBrushCreate();
    Rt2dBrush *result = (Rt2dBrush *) NULL;

    RWAPIFUNCTION(RWSTRING("Rt2dBrushStreamRead"));
    RWASSERT(stream);


    result = _rt2dBrushStreamReadTo(brush,stream);
    if( NULL == result )
    {
        Rt2dBrushDestroy(brush);
    }

    RWRETURN(brush);
}

/****************************************************************************/

 /* All of the Following functions are for Debug version only
  * They each have a macro counterpart which is used in the
  * Release builds.
  */

#if ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) )

/**
 * \ingroup rt2dbrush
 * \ref Rt2dBrushSetWidth
 * is used to define the width of the specified brush.
 * The width is used only when painting a path; it is measured
 * perpendicular to path and the path lies at the center of the painted
 * primitive.
 * A newly created brush has a unit width.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 *
 * \note This function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param brush  Pointer to the brush.
 * \param width  A RwReal value equal to the width of the brush.
 * \return pointer to the brush if successful or NULL if there is an
 * error.
 * \see Rt2dBrushGetWidth
 * \see Rt2dBrushSetRGBA
 * \see Rt2dBrushSetUV
 * \see Rt2dBrushSetTexture
 * \see Rt2dBrushCreate
 */
Rt2dBrush          *
Rt2dBrushSetWidth(Rt2dBrush * brush, RwReal width)
{
    RWAPIFUNCTION(RWSTRING("Rt2dBrushSetWidth"));

    RWASSERT(brush);

    Rt2dBrushSetWidthMacro(brush, width);

    RWRETURN(brush);
}

/****************************************************************************/

/**
 * \ingroup rt2dbrush
 * \ref Rt2dBrushGetWidth
 * is used to retrieve the width of the specified
 * brush.
 * The width is used only when painting a path; it is measured
 * perpendicular to path and the path lies at the center of the painted
 * primitive.
 * A newly created brush has a unit width.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param brush  Pointer to the brush.
 * \return a RwReal value equal to the width of the brush if successful
 * or zero if there is an error.
 * \see Rt2dBrushSetWidth
 * \see Rt2dBrushSetRGBA
 * \see Rt2dBrushSetUV
 * \see Rt2dBrushSetTexture
 * \see Rt2dBrushCreate
 */
RwReal
Rt2dBrushGetWidth(Rt2dBrush * brush)
{
    RWAPIFUNCTION(RWSTRING("Rt2dBrushGetWidth"));

    RWASSERT(brush);

    RWRETURN(Rt2dBrushGetWidthMacro(brush));
}

#endif /* ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) ) */
