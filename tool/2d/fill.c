/*
 *
 * Functionality for 2D rendering
 *
 * Copyright (c) 1998 Criterion Software Ltd.
 */

/****************************************************************************
 *                                                                          *
 *  Module  :   fill.c                                                      *
 *                                                                          *
 *  Purpose :   fill paths                                                  *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
 */
#include <rwcore.h>
#include <rpworld.h>
#include <rpdbgerr.h>

#include <string.h>

#include "rt2d.h"
#include "path.h"
#include "font.h"
#include "tri.h"
#include "gstate.h"
#include "brush.h"

#if (defined(SKY2_DRVMODEL_H))
#include "rpworld.h"
#include "rppds.h"
#endif /* (defined(SKY2_DRVMODEL_H)) */

#include "fill.h"

#if defined (__MWERKS__)
#if (defined(RWVERBOSE))
#pragma message (__FILE__ "/" _SKY_EXPAND(__LINE__) ": __MWERKS__ == " _SKY_EXPAND(__MWERKS__))
#endif /* (defined (__MWERKS__)) */
#if (__option (global_optimizer))
#pragma always_inline off
#endif /* (__option (global_optimizer)) */
#endif /*  defined (__MWERKS__) */

/****************************************************************************
 Local Types
 */

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */
#define FILLMAXVERT 32

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   Functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

#ifdef AMB_SPECIFIC
#ifdef RWDEBUG
static void
WireRender2d(RwIm3DVertex * vertex, int vcount,
             RwImVertexIndex * topo, int icount)
{
    RwIm3DVertex        local[VERTEXCACHESIZE];
    int                 i;

    RWFUNCTION(RWSTRING("WireRender2d"));

    for (i = 0; i < vcount; i++)
    {
        local[i] = vertex[i];
        RwIm3DVertexSetRGBA(&(local[i]), 0, 0, 0, 255);
    }

    if (RwIm3DTransform(local, vcount, _rt2dCTMGet(), 0))
    {
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPEPOLYLINE, topo,
                                     icount - 1);
        RwIm3DEnd();
    }

    RWRETURNVOID();
}
#endif /* RWDEBUG */
#endif /* AMB_SPECIFIC */

/****************************************************************************/
static Rt2dPath *
DefaultPathClosedFill2d(Rt2dPath * path, Rt2dBrush * brush)
{
    Rt2dPath           *flat;
    rt2dPathNode       *pnode, *pleft, *pright;
    RwInt32             vcount, i, vindex;
    Rt2dBBox            bbox;
    RwV2d               posnormalize, avgnormal;
    RwIm3DVertex        *vdst;
    RwReal              layerDepth, inset;
    RwV2d               delta;
    RwV3d               pos;
    rt2dShadeParameters top, bottom, mid;
    RwMatrix           *ctm = _rt2dCTMGet();

    RWFUNCTION(RWSTRING("DefaultPathClosedFill2d"));

    RWASSERT(path);
    RWASSERT(path->closed);

    layerDepth = Rt2dGlobals.layerDepth;
    inset = path->inset;

    /* we'll be using the flattened path */
    flat = path;
    if (!path->flat)
    {
        flat = _rt2dScratchPath();
        _rt2dSubPathFlatten(flat, path);
    }

    pnode = _rt2dPathGetSegmentArray(flat);
    vcount = _rt2dPathGetNumSegment(flat);

    /* we need the bbox to generate vertex shading info */
    bbox.x = bbox.w = pnode[0].pos.x;
    bbox.y = bbox.h = pnode[0].pos.y;

    /* get left/right NOT pos/width */
    _rt2dFlatSubPathExtendBBox(flat, &bbox);
    posnormalize.x = ((RwReal) 1.0) / (bbox.w - bbox.x);
    posnormalize.y = ((RwReal) 1.0) / (bbox.h - bbox.y);

    if (brush->texture)
    {
        const RwTextureFilterMode filterMode =
            RwTextureGetFilterMode(brush->texture);
        const RwTextureAddressMode addressingMode =
            RwTextureGetAddressing(brush->texture);

        RwRenderStateSet(rwRENDERSTATETEXTURERASTER,
                         (void *) RwTextureGetRaster(brush->texture));
        RwRenderStateSet(rwRENDERSTATETEXTUREFILTER,
                         (void *) filterMode);
        RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS,
                         (void *) addressingMode);
    }
    else
    {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    }

    vdst = Rt2dGlobals.brushVertexCache;
    pright = pnode;
    pleft = pnode + vcount - 1;
    vindex = VERTEXCACHESIZE;              /* force new strip */
    for (i = 1; i < vcount; i++)
    {
        RwInt32             red;
        RwInt32             green;
        RwInt32             blue;
        RwInt32             alpha;

        /* need to flush? */
        if (vindex >= VERTEXCACHESIZE - 2)
        {
            /* render */
            if (vdst - Rt2dGlobals.brushVertexCache)
            {
                if (RwIm3DTransform(Rt2dGlobals.brushVertexCache,
                                    vdst - Rt2dGlobals.brushVertexCache,
                                    ctm,
                                    (brush->texture ? rwIM3D_VERTEXUV : 0) |
                                    Rt2dGlobals.TransformFlags))
                {
                    RwIm3DRenderIndexedPrimitive
                        (rwPRIMTYPETRISTRIP, Rt2dGlobals.topo, vindex);
                    RwIm3DEnd();

#ifdef AMB_SPECIFIC
#ifdef RWDEBUG
                    WireRender2d(Rt2dGlobals.brushVertexCache, vdst - Rt2dGlobals.brushVertexCache,
                                 Rt2dGlobals.topo, vindex);
#endif
#endif
                }
            }

            vdst = Rt2dGlobals.brushVertexCache;

            /* start new strip */

            /* corner normal */
            RwV2dScale(&avgnormal, &pleft->normal, inset);
            RwV2dSub((RwV2d *) & pos, &pleft->pos, &avgnormal);

            RwIm3DVertexSetPos(vdst, pos.x, pos.y, layerDepth);

            delta.x = pos.x - bbox.x;   /* DUMMY - Don't need to do this for flat fill & texture */
            delta.y = pos.y - bbox.y;
            delta.x *= posnormalize.x;
            delta.y *= posnormalize.y;

            /* RGBAs */
            if (brush->flag & FIELDRGBA) /* If it's not flat */
            {
               /* interpolate across bottom edge */
               RwRGBARealScale(&bottom.col, &brush->dbottom.col, delta.x);
               RwRGBARealAdd(&bottom.col, &brush->bottom.col, &bottom.col);

               /* interpolate across top edge */
               RwRGBARealScale(&top.col, &brush->dtop.col, delta.x);
               RwRGBARealAdd(&top.col, &brush->top.col, &top.col);

               /* interpolate top to bottom */
               RwRGBARealSub(&mid.col, &top.col, &bottom.col);
               RwRGBARealScale(&mid.col, &mid.col, delta.y);
               RwRGBARealAdd(&mid.col, &bottom.col, &mid.col);

               red   = RwInt32FromRealMacro(mid.col.red);
               green = RwInt32FromRealMacro(mid.col.green);
               blue  = RwInt32FromRealMacro(mid.col.blue);
               alpha = RwInt32FromRealMacro(mid.col.alpha);

               RwIm3DVertexSetRGBA(vdst,
                                (RwUInt8) red,
                                (RwUInt8) green,
                                (RwUInt8) blue, (RwUInt8) alpha);
            }
            else /* It's flat */
            {
                RwIm3DVertexSetRGBA(vdst,
                                    brush->colorCache.red,
                                    brush->colorCache.green,
                                    brush->colorCache.blue,
                                    brush->colorCache.alpha);
            }

            /* UVs */

            /* interpolate across bottom edge */
            RwV2dScale(&bottom.uv, &brush->dbottom.uv, delta.x);
            RwV2dAdd(&bottom.uv, &brush->bottom.uv, &bottom.uv);

            /* interpolate across top edge */
            RwV2dScale(&top.uv, &brush->dtop.uv, delta.x);
            RwV2dAdd(&top.uv, &brush->top.uv, &top.uv);

            /* interpolate top to bottom */
            RwV2dSub(&mid.uv, &top.uv, &bottom.uv);
            RwV2dScale(&mid.uv, &mid.uv, delta.y);
            RwV2dAdd(&mid.uv, &bottom.uv, &mid.uv);

            RwIm3DVertexSetU(vdst, mid.uv.x);
            RwIm3DVertexSetV(vdst, mid.uv.y);

            vdst++;
            pleft--;

            RwV2dScale(&avgnormal, &pright->normal, inset);
            RwV2dSub((RwV2d *) & pos, &pright->pos, &avgnormal);

            RwIm3DVertexSetPos(vdst, pos.x, pos.y, layerDepth);

            delta.x = pos.x - bbox.x;     /* DUMMY - Don't need to do this for flat fill & texture */
            delta.y = pos.y - bbox.y;
            delta.x *= posnormalize.x;
            delta.y *= posnormalize.y;

            /* RGBAs */
            if (brush->flag & FIELDRGBA) /* If it's not flat */
            {
               /* interpolate across bottom edge */
               RwRGBARealScale(&bottom.col, &brush->dbottom.col, delta.x);
               RwRGBARealAdd(&bottom.col, &brush->bottom.col, &bottom.col);

               /* interpolate across top edge */
               RwRGBARealScale(&top.col, &brush->dtop.col, delta.x);
               RwRGBARealAdd(&top.col, &brush->top.col, &top.col);

               /* interpolate top to bottom */
               RwRGBARealSub(&mid.col, &top.col, &bottom.col);
               RwRGBARealScale(&mid.col, &mid.col, delta.y);
               RwRGBARealAdd(&mid.col, &bottom.col, &mid.col);

               red   = RwInt32FromRealMacro(mid.col.red);
               green = RwInt32FromRealMacro(mid.col.green);
               blue  = RwInt32FromRealMacro(mid.col.blue);
               alpha = RwInt32FromRealMacro(mid.col.alpha);

               RwIm3DVertexSetRGBA(vdst,
                                (RwUInt8) red,
                                (RwUInt8) green,
                                (RwUInt8) blue, (RwUInt8) alpha);
            }
            else  /* It's flat */
            {
                RwIm3DVertexSetRGBA(vdst,
                                    brush->colorCache.red,
                                    brush->colorCache.green,
                                    brush->colorCache.blue,
                                    brush->colorCache.alpha);
            }

            /* UVs */
            /* interpolate across bottom edge */
            RwV2dScale(&bottom.uv, &brush->dbottom.uv, delta.x);
            RwV2dAdd(&bottom.uv, &brush->bottom.uv, &bottom.uv);

            /* interpolate across top edge */
            RwV2dScale(&top.uv, &brush->dtop.uv, delta.x);
            RwV2dAdd(&top.uv, &brush->top.uv, &top.uv);

            /* interpolate top to bottom */
            RwV2dSub(&mid.uv, &top.uv, &bottom.uv);
            RwV2dScale(&mid.uv, &mid.uv, delta.y);
            RwV2dAdd(&mid.uv, &bottom.uv, &mid.uv);

            RwIm3DVertexSetU(vdst, mid.uv.x);
            RwIm3DVertexSetV(vdst, mid.uv.y);
            vdst++;
            pright++;

            vindex = 2;
        }

        RwV2dScale(&avgnormal, &pleft->normal, inset);
        RwV2dSub((RwV2d *) & pos, &pleft->pos, &avgnormal);

        RwIm3DVertexSetPos(vdst, pos.x, pos.y, layerDepth);

        delta.x = pos.x - bbox.x;
        delta.y = pos.y - bbox.y;
        delta.x *= posnormalize.x;
        delta.y *= posnormalize.y;

        /* RGBAs */
        if (brush->flag & FIELDRGBA) /* If it's not flat */
        {
            /* interpolate across bottom edge */
            RwRGBARealScale(&bottom.col, &brush->dbottom.col, delta.x);
            RwRGBARealAdd(&bottom.col, &brush->bottom.col, &bottom.col);

            /* interpolate across top edge */
            RwRGBARealScale(&top.col, &brush->dtop.col, delta.x);
            RwRGBARealAdd(&top.col, &brush->top.col, &top.col);

            /* interpolate top to bottom */
            RwRGBARealSub(&mid.col, &top.col, &bottom.col);
            RwRGBARealScale(&mid.col, &mid.col, delta.y);
            RwRGBARealAdd(&mid.col, &bottom.col, &mid.col);

            red   = RwInt32FromRealMacro(mid.col.red);
            green = RwInt32FromRealMacro(mid.col.green);
            blue  = RwInt32FromRealMacro(mid.col.blue);
            alpha = RwInt32FromRealMacro(mid.col.alpha);

            RwIm3DVertexSetRGBA(vdst,
                                (RwUInt8) red,
                                (RwUInt8) green,
                                (RwUInt8) blue, (RwUInt8) alpha);
        }
        else  /* It's flat */
        {
            RwIm3DVertexSetRGBA(vdst,
                                brush->colorCache.red,
                                brush->colorCache.green,
                                brush->colorCache.blue,
                                brush->colorCache.alpha);
        }

        /* interpolate across bottom edge */
        RwV2dScale(&bottom.uv, &brush->dbottom.uv, delta.x);
        RwV2dAdd(&bottom.uv, &brush->bottom.uv, &bottom.uv);

        /* interpolate across top edge */
        RwV2dScale(&top.uv, &brush->dtop.uv, delta.x);
        RwV2dAdd(&top.uv, &brush->top.uv, &top.uv);

        /* interpolate top to bottom */
        RwV2dSub(&mid.uv, &top.uv, &bottom.uv);
        RwV2dScale(&mid.uv, &mid.uv, delta.y);
        RwV2dAdd(&mid.uv, &bottom.uv, &mid.uv);

        RwIm3DVertexSetU(vdst, mid.uv.x);
        RwIm3DVertexSetV(vdst, mid.uv.y);
        vdst++;
        pleft--;
        vindex++;

        if (pleft < pright)
        {
            break;
        }

        RwV2dScale(&avgnormal, &pright->normal, inset);
        RwV2dSub((RwV2d *) & pos, &pright->pos, &avgnormal);

        RwIm3DVertexSetPos(vdst, pos.x, pos.y, layerDepth);

        delta.x = pos.x - bbox.x;
        delta.y = pos.y - bbox.y;
        delta.x *= posnormalize.x;
        delta.y *= posnormalize.y;

        /* RGBAs */
        if (brush->flag & FIELDRGBA) /* If it's not flat */
        {
            /* interpolate across bottom edge */
            RwRGBARealScale(&bottom.col, &brush->dbottom.col, delta.x);
            RwRGBARealAdd(&bottom.col, &brush->bottom.col, &bottom.col);

            /* interpolate across top edge */
            RwRGBARealScale(&top.col, &brush->dtop.col, delta.x);
            RwRGBARealAdd(&top.col, &brush->top.col, &top.col);

            /* interpolate top to bottom */
            RwRGBARealSub(&mid.col, &top.col, &bottom.col);
            RwRGBARealScale(&mid.col, &mid.col, delta.y);
            RwRGBARealAdd(&mid.col, &bottom.col, &mid.col);

            red   = RwInt32FromRealMacro(mid.col.red);
            green = RwInt32FromRealMacro(mid.col.green);
            blue  = RwInt32FromRealMacro(mid.col.blue);
            alpha = RwInt32FromRealMacro(mid.col.alpha);

            RwIm3DVertexSetRGBA(vdst,
                                (RwUInt8) red,
                                (RwUInt8) green,
                                (RwUInt8) blue, (RwUInt8) alpha);
        }
        else /* It's flat */
        {
            RwIm3DVertexSetRGBA(vdst,
                                brush->colorCache.red,
                                brush->colorCache.green,
                                brush->colorCache.blue,
                                brush->colorCache.alpha);
        }

        /* uvs */
        /* interpolate across bottom edge */
        RwV2dScale(&bottom.uv, &brush->dbottom.uv, delta.x);
        RwV2dAdd(&bottom.uv, &brush->bottom.uv, &bottom.uv);

        /* interpolate across top edge */
        RwV2dScale(&top.uv, &brush->dtop.uv, delta.x);
        RwV2dAdd(&top.uv, &brush->top.uv, &top.uv);

        /* interpolate top to bottom */
        RwV2dSub(&mid.uv, &top.uv, &bottom.uv);
        RwV2dScale(&mid.uv, &mid.uv, delta.y);
        RwV2dAdd(&mid.uv, &bottom.uv, &mid.uv);

        RwIm3DVertexSetU(vdst, mid.uv.x);
        RwIm3DVertexSetV(vdst, mid.uv.y);
        vdst++;
        pright++;
        vindex++;

        if (pleft < pright)
        {
            break;
        }
    }

    /* render scrag end */
    if (vdst - Rt2dGlobals.brushVertexCache)
    {
        if (RwIm3DTransform(Rt2dGlobals.brushVertexCache,
                            vdst - Rt2dGlobals.brushVertexCache, ctm,
                            (brush->texture ? rwIM3D_VERTEXUV : 0) |
                            Rt2dGlobals.TransformFlags))
        {
            RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRISTRIP,
                                         Rt2dGlobals.topo, vindex);
            RwIm3DEnd();

#ifdef AMB_SPECIFIC
#ifdef RWDEBUG
            WireRender2d(vertex, vdst - vertex,
                         Rt2dGlobals.topo, vindex);
#endif
#endif
        }
    }

    RWRETURN(path);
}

/****************************************************************************/
static Rt2dPath    *
DefaultPathFill2d(Rt2dPath * path, Rt2dBrush * brush)
{
    RWFUNCTION(RWSTRING("DefaultPathFill2d"));

    if (path)
    {
        if (path->closed)
        {
            DefaultPathClosedFill2d(path, brush);
        }

        DefaultPathFill2d(path->next, brush);
    }

    RWRETURN(path);
}

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathFill
 * is used to fill the specified path using the colors and
 * texture coordinates of the given brush. The path must be closed for
 * this function to work properly.
 * The fill color for each point within the path is determined by
 * bilinear interpolation of the colors of the brush assuming they
 * represent the colors of the four corners of the path's
 * bounding-box. Hence, the fill color depends on the relative distance
 * of each interior point from the corner points of the path's
 * bounding-box.
 * If the brush also specifies texture coordinates and a texture image,
 * the path is filled with the image assuming that the bounding-box
 * corners have the texture coordinates of the brush.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param path  Pointer to the path.
 * \param brush  Pointer to the brush.
 * \return a pointer to the path if successful or NULL if there is an
 * error.
 * \see Rt2dBrushSetRGBA
 * \see Rt2dBrushSetUV
 * \see Rt2dBrushSetTexture
 * \see Rt2dPathStroke
 */

Rt2dPath           *
Rt2dPathFill(Rt2dPath * path, Rt2dBrush * brush)
{
    RWAPIFUNCTION(RWSTRING("Rt2dPathFill"));

#if (defined(SKY2_DRVMODEL_H))
    _rt2dPS2PathFill(path, brush);
#else /* (defined(SKY2_DRVMODEL_H)) */
    DefaultPathFill2d(path, brush);
#endif /* (defined(SKY2_DRVMODEL_H)) */

    RWRETURN(path);
}


/*
 * Function to convert a fill path to a geometry. The function relies on the
 * geometry already created. The geometry can hold multiple paths, each being a
 * single mesh.
 */
Rt2dPath *
_rt2dPathFillCreateGeometry(Rt2dPath * path, Rt2dBrush * brush,
                            RpGeometry *geom, RwUInt32 *geomVertIdx, RwUInt32 *geomTriIdx)
{
    Rt2dPath            *subPath;
    RpMorphTarget       *morphTarget;
    RpTriangle          *tri;
    RpMaterial          *material;
    RwUInt32            i, j, numTris, numVerts, totalVerts, totalTris, firstIdx, lastIdx, idx0, idx1, idx2;
    RwReal              layerDepth, inset;
    RwV3d               *pos;
    RwRGBA              *preLit;
    RwTexCoords         *texCoord;

    RwUInt32            *triIdx;
    rt2dPathNode        *pNode;
    Rt2dBBox            bbox;
    RwV2d               posnormalize, avgnormal;
    RwV2d               delta;
    rt2dShadeParameters top, bottom, mid;

    RWFUNCTION(RWSTRING("_rt2dPathFillCreateGeometry"));

    /* Create a material for the brush if required */
    if (brush->material == NULL)
    {
        material = RpMaterialCreate();

        if (brush->texture != NULL)
        {
            RpMaterialSetTexture(material, brush->texture);
        }

        if (brush->flag & FIELDRGBA) /* If it's not flat */
        {
            RwRGBA opaqueWhite = { 255, 255, 255, 255 };

            /* Will be coloring through prelights */
            RpMaterialSetColor(material, &opaqueWhite);
        }
        else    /* It's flat */
        {
            RpMaterialSetColor(material, &brush->colorCache);
        }

        brush->material = material;
    }
    else
    {
        material = brush->material;
        RpMaterialAddRef(material);
    }

    /* Assume the path can be filled as a tristrip, so first need to walk the path
     * to count the number of points.
     */

    totalVerts = 0;
    totalTris = 0;
    numTris = 0;
    triIdx = NULL;

    /* Build a tmp list of paths that can be instanced. Paths will be flattened and
     * non-closed path ignored.
     */
    subPath = path->next;

    do
    {
        numVerts = _rt2dPathGetNumSegment(subPath);

        totalVerts += numVerts;
        totalTris += (numVerts - 3);

        if (numVerts > numTris)
            numTris = (numVerts - 3);

        subPath = subPath->next;
    } while(subPath != path->next);

    /* Create the index array */
    if (numTris > 0)
    {
        if ((numTris * 3) > Rt2dGlobals.triIdxCacheSize)
        {
            if( Rt2dGlobals.triIdxCache )
            {        
                RwFree(Rt2dGlobals.triIdxCache);
            }

            Rt2dGlobals.triIdxCacheSize = 0;

            Rt2dGlobals.triIdxCache = (RwUInt32 *)
                RwMalloc(numTris * 3 * sizeof(RwUInt32),
                    rwMEMHINTDUR_EVENT | rwID_2DPLUGIN);

            if (Rt2dGlobals.triIdxCache == NULL)
                RWRETURN( (Rt2dPath *) NULL);

            Rt2dGlobals.triIdxCacheSize = numTris * 3;
        }

        triIdx = Rt2dGlobals.triIdxCache;

        memset(triIdx, 0, (numTris * 3 * sizeof(RwInt32)));
    }

    /* Main build loop */
    layerDepth = Rt2dGlobals.layerDepth;

    if ((totalTris > 0) && (totalVerts > 0))
    {
        morphTarget = RpGeometryGetMorphTarget(geom, 0);

        pos = RpMorphTargetGetVertices(morphTarget) + *geomVertIdx;

        tri = RpGeometryGetTriangles(geom) + *geomTriIdx;

        preLit = RpGeometryGetPreLightColors(geom) + *geomVertIdx;

        texCoord = RpGeometryGetVertexTexCoords(geom, rwTEXTURECOORDINATEINDEX0) + *geomVertIdx;

        /* Walk the path, creating the tris and transfering the verts over */
        subPath = path->next;
        firstIdx = *geomVertIdx;

        do
        {
            inset = subPath->inset;

            /*
             * Create the bridging tris
             */

            pNode = _rt2dPathGetSegmentArray(subPath);
            numVerts = _rt2dPathGetNumSegment(subPath);
            numTris = numVerts - 3;

            lastIdx = firstIdx + numVerts - 1;

            /* we need the bbox to generate vertex shading info */
            bbox.x = bbox.w = pNode[0].pos.x;
            bbox.y = bbox.h = pNode[0].pos.y;

            /* get left/right NOT pos/width */
            _rt2dFlatSubPathExtendBBox(subPath, &bbox);

            posnormalize.x = ((RwReal) 1.0) / (bbox.w - bbox.x);
            posnormalize.y = ((RwReal) 1.0) / (bbox.h - bbox.y);

            for (i = 0; i < numVerts; i++)
            {
                /* pos */
                RwV2dScale(&avgnormal, &pNode->normal, inset);
                RwV2dSub((RwV2d *) pos, &pNode->pos, &avgnormal);
                pos->z = layerDepth;

                delta.x = pos->x - bbox.x;
                delta.y = pos->y - bbox.y;
                delta.x *= posnormalize.x;
                delta.y *= posnormalize.y;

                /* RGBAs */
                if (brush->flag & FIELDRGBA) /* It's not flat */
                {
                    /* interpolate across bottom edge */
                    RwRGBARealScale(&bottom.col, &brush->dbottom.col, delta.x);
                    RwRGBARealAdd(&bottom.col, &brush->bottom.col, &bottom.col);

                    /* interpolate across top edge */
                    RwRGBARealScale(&top.col, &brush->dtop.col, delta.x);
                    RwRGBARealAdd(&top.col, &brush->top.col, &top.col);

                    /* interpolate top to bottom */
                    RwRGBARealSub(&mid.col, &top.col, &bottom.col);
                    RwRGBARealScale(&mid.col, &mid.col, delta.y);
                    RwRGBARealAdd(&mid.col, &bottom.col, &mid.col);

                    preLit->red   = RwInt32FromRealMacro(mid.col.red);
                    preLit->green = RwInt32FromRealMacro(mid.col.green);
                    preLit->blue  = RwInt32FromRealMacro(mid.col.blue);
                    preLit->alpha = RwInt32FromRealMacro(mid.col.alpha);

                    preLit++;
                }
                else /* It's flat */
                {
                    /* entirely using the material color, no preLit interference thanks */
                    preLit->red   = 255;
                    preLit->green = 255;
                    preLit->blue  = 255;
                    preLit->alpha = 255;

                    preLit++;
                }

                /* TexCoords */
                if (brush->texture)
                {
                    /* interpolate across bottom edge */
                    RwV2dScale(&bottom.uv, &brush->dbottom.uv, delta.x);
                    RwV2dAdd(&bottom.uv, &brush->bottom.uv, &bottom.uv);

                    /* interpolate across top edge */
                    RwV2dScale(&top.uv, &brush->dtop.uv, delta.x);
                    RwV2dAdd(&top.uv, &brush->top.uv, &top.uv);

                    /* interpolate top to bottom */
                    RwV2dSub(&mid.uv, &top.uv, &bottom.uv);
                    RwV2dScale(&mid.uv, &mid.uv, delta.y);
                    RwV2dAdd(&mid.uv, &bottom.uv, &mid.uv);

                    texCoord->u = mid.uv.x;
                    texCoord->v = mid.uv.y;

                    texCoord++;
                }

                pNode++;

                pos++;
            }

            /* Triangulate the shape */
            _rt2dPathTriangulate(subPath, (RwInt32 *) &numTris, (RwInt32 *) triIdx);

            /* Tri Index */
            idx0 = lastIdx - 1;
            idx1 = firstIdx;
            idx2 = lastIdx - 2;

            for (i = 0, j = 0; i < (numVerts - 3); i++, j += 3)
            {
                RpGeometryTriangleSetVertexIndices(geom, tri,
                    (RwUInt16)(firstIdx + triIdx[j]),
                    (RwUInt16)(firstIdx + triIdx[j + 1]),
                    (RwUInt16)(firstIdx + triIdx[j + 2]));

                RpGeometryTriangleSetMaterial(geom, tri, material);

                tri++;
            }

            firstIdx = lastIdx + 1;

            /* Next path */
            subPath = subPath->next;

        } while (subPath != path->next);
    }

    /* Destroy and material to decrement their ref counts */
    RpMaterialDestroy(material);

    /* Return the new index positions for the next path. */
    *geomVertIdx += totalVerts;
    *geomTriIdx += totalTris;

    RWRETURN(path);
}

#if defined (__MWERKS__)
#if (defined(RWVERBOSE))
#pragma message (__FILE__ "/" _SKY_EXPAND(__LINE__) ": __MWERKS__ == " _SKY_EXPAND(__MWERKS__))
#endif /* (defined (__MWERKS__)) */
#if (__option (global_optimizer))
#pragma always_inline on
#endif /* (__option (global_optimizer)) */
#endif /*  defined (__MWERKS__) */
