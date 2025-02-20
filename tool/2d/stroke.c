/*
 *
 * Functionality for 2D rendering
 *
 * Copyright (c) 1998 Criterion Software Ltd.
 */

/****************************************************************************
 *                                                                          *
 *  Module  :   stroke.c                                                    *
 *                                                                          *
 *  Purpose :   stroke paths                                                *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
 */
#include "rwcore.h"
#include "rpdbgerr.h"
#include "rpworld.h"

#include "rt2d.h"
#include "path.h"
#include "brush.h"
#include "font.h"
#include "tri.h"
#include "gstate.h"
#include "brush.h"
#include "fill.h"

#if (defined(SKY2_DRVMODEL_H))
#include "rppds.h"
#endif /* (defined(SKY2_DRVMODEL_H)) */

#include "stroke.h"

/****************************************************************************
 Local Types
 */

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */

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
        RwIm3DVertexSetRGBA(&(local[i]), 255, 255, 255, 255);
    }

    if (RwIm3DTransform(local, vcount, _rt2dCTMGet(), 0))
    {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPEPOLYLINE, topo, icount);
        RwIm3DEnd();
    }

    RWRETURNVOID();
}
#endif /* RWDEBUG */
#endif /* AMB_SPECIFIC */

/****************************************************************************/
static Rt2dPath *
DefaultPathStroke2d(Rt2dPath * path, Rt2dBrush * brush)
{
    Rt2dPath           *flat;
    rt2dPathNode       *pnode;
    rt2dPathNode       *firstnode;
    RwInt32             vcount, i, vindex;
    RwIm3DVertex       *vdst;
    RwReal              oobaseu, halfwidth, layerDepth, inset;

    RWFUNCTION(RWSTRING("DefaultPathStroke2d"));

    /* NULL path is valid */
    if (path)
    {
        RwV2d               pos;
        rt2dShadeParameters sp;
        RwV2d               avgnormal;
        RwMatrix           *ctm = _rt2dCTMGet();
        RwInt32             red;
        RwInt32             green;
        RwInt32             blue;
        RwInt32             alpha;

        layerDepth = Rt2dGlobals.layerDepth;
        halfwidth = brush->halfwidth;
        inset = path->inset;

        /* we'll be using the flattened path */
        flat = path;
        if (!path->flat)
        {
            flat = _rt2dScratchPath();
            _rt2dSubPathFlatten(flat, path);
        }

        firstnode = pnode = _rt2dPathGetSegmentArray(flat);

        vcount = _rt2dPathGetNumSegment(flat);

        if (brush->texture)
        {
            const RwTextureFilterMode filterMode =
                RwTextureGetFilterMode(brush->texture);
            const RwTextureAddressMode addressingMode =
                RwTextureGetAddressing(brush->texture);

            RwRenderStateSet(rwRENDERSTATETEXTURERASTER,
                             (void *)
                             RwTextureGetRaster(brush->texture));
            RwRenderStateSet(rwRENDERSTATETEXTUREFILTER,
                             (void *) filterMode);
            RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS,
                             (void *) addressingMode);
        }
        else
        {
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
        }

        oobaseu = 1.0f / pnode[vcount - 1].dist;
        vdst = Rt2dGlobals.brushVertexCache;
        vindex = 0;
        for (i = 0; i < vcount; i++, pnode++)
        {
            /* need to flush? (save space for last two vertices) */
            if (vindex >= VERTEXCACHESIZE- 2)
            {
                /* render */
                if (RwIm3DTransform
                    (Rt2dGlobals.brushVertexCache, vindex, ctm,
                    (brush->texture ? rwIM3D_VERTEXUV : 0) |
                    Rt2dGlobals.TransformFlags))
                {
                    RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRISTRIP,
                                                 Rt2dGlobals.topo,
                                                 vindex);
                    RwIm3DEnd();

#ifdef AMB_SPECIFIC
#ifdef RWDEBUG
                    WireRender2d(Rt2dGlobals.brushVertexCache, vindex,
                                 Rt2dGlobals.topo, vindex);
#endif
#endif
                }

                /* start new strip */
                vdst = Rt2dGlobals.brushVertexCache;
                vindex = 0;

                /* bump back to previous vertex */
                i--;
                pnode--;
            }

            /* corner normal */
            RwV2dScale(&avgnormal, &pnode->normal, (halfwidth + inset));

            RwV2dSub(&pos, &pnode->pos, &avgnormal);
            RwIm3DVertexSetPos(vdst, pos.x, pos.y, layerDepth);

            if (brush->flag & FIELDRGBA)   /* If it's not flat */
            {
                RwRGBARealScale(&sp.col, &brush->dtop.col,
                                pnode->dist * oobaseu);
                RwRGBARealAdd(&sp.col, &brush->top.col, &sp.col);

                red   = RwInt32FromRealMacro(sp.col.red);
                green = RwInt32FromRealMacro(sp.col.green);
                blue  = RwInt32FromRealMacro(sp.col.blue);
                alpha = RwInt32FromRealMacro(sp.col.alpha);

                RwIm3DVertexSetRGBA(vdst,
                                    (RwUInt8) red,
                                    (RwUInt8) green,
                                    (RwUInt8) blue, (RwUInt8) alpha);
            }
            else  /* Otherwise it's flat */
            {
                RwIm3DVertexSetRGBA(vdst,
                                    brush->colorCache.red,
                                    brush->colorCache.green,
                                    brush->colorCache.blue,
                                    brush->colorCache.alpha);
            }

            if (brush->flag & FIELDUV)
            {
                RwV2dScale(&sp.uv, &brush->dtop.uv,
                           pnode->dist * oobaseu);
                RwV2dAdd(&sp.uv, &brush->top.uv, &sp.uv);
                RwIm3DVertexSetU(vdst, sp.uv.x);
                RwIm3DVertexSetV(vdst, sp.uv.y);
            }

            vdst++;

            RwV2dScale(&avgnormal, &pnode->normal, (halfwidth - inset));

            RwV2dAdd(&pos, &pnode->pos, &avgnormal);
            RwIm3DVertexSetPos(vdst, pos.x, pos.y, layerDepth);

            if (brush->flag & FIELDRGBA)  /* It's not flat */
            {
                RwRGBARealScale(&sp.col, &brush->dbottom.col,
                                pnode->dist * oobaseu);
                RwRGBARealAdd(&sp.col, &brush->bottom.col, &sp.col);

                red   = RwInt32FromRealMacro(sp.col.red);
                green = RwInt32FromRealMacro(sp.col.green);
                blue  = RwInt32FromRealMacro(sp.col.blue);
                alpha = RwInt32FromRealMacro(sp.col.alpha);

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

            if (brush->flag & FIELDUV)
            {
                RwV2dScale(&sp.uv, &brush->dbottom.uv,
                           pnode->dist * oobaseu);
                RwV2dAdd(&sp.uv, &brush->bottom.uv, &sp.uv);
                RwIm3DVertexSetU(vdst, sp.uv.x);
                RwIm3DVertexSetV(vdst, sp.uv.y);
            }

            vdst++;

            vindex += 2;
        }

        /* render scrag end */
        if (vindex > 0)
        {
            /* render */
            if (RwIm3DTransform
                (Rt2dGlobals.brushVertexCache, vindex, ctm,
                 (brush->texture ? rwIM3D_VERTEXUV : 0) |
                 Rt2dGlobals.TransformFlags))
            {
                RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRISTRIP,
                                             Rt2dGlobals.topo, vindex);
                RwIm3DEnd();

#ifdef AMB_SPECIFIC
#ifdef RWDEBUG
                WireRender2d(Rt2dGlobals.brushVertexCache, vindex, Rt2dGlobals.topo,
                             vindex);
#endif
#endif
            }
        }

        DefaultPathStroke2d(path->next, brush);
    }

    RWRETURN(path);
}

/**
 * \ingroup rt2dpath
 * \ref Rt2dPathStroke
 * is used to paint the specified path using the given
 * brush.
 * The include file rt2d.h and the library file rt2d.lib are required to
 * use this function.
 * \param path  Pointer to the path.
 * \param brush  Pointer to the brush.
 * \return a pointer to the path if successful or NULL if there is an
 * error.
 * \see Rt2dBrushSetRGBA
 * \see Rt2dBrushSetUV
 * \see Rt2dBrushSetTexture
 * \see Rt2dBrushSetWidth
 */
Rt2dPath           *
Rt2dPathStroke(Rt2dPath * path, Rt2dBrush * brush)
{
    RWAPIFUNCTION(RWSTRING("Rt2dPathStroke"));

#if (defined(SKY2_DRVMODEL_H))
    RWRETURN(_rt2dPS2PathStroke(path, brush));
#else /* (defined(SKY2_DRVMODEL_H)) */
    RWRETURN(DefaultPathStroke2d(path, brush));
#endif /* (defined(SKY2_DRVMODEL_H)) */
}

/*
 * Function to convert a stroke path to a geometry. The function relies on the
 * geometry already created. The geometry can hold multiple paths, each being a
 * single mesh.
 */
Rt2dPath *
_rt2dPathStrokeCreateGeometry(Rt2dPath * path, Rt2dBrush * brush,
                              RpGeometry *geom, RwUInt32 *geomVertIdx, RwUInt32 *geomTriIdx)
{
    Rt2dPath            *subPath;
    RpMorphTarget       *morphTarget;
    RpTriangle          *tri;
    RpMaterial          *material;
    RwUInt32            i, numVerts, totalVerts, totalTris, firstIdx, lastIdx, idx0;
    RwReal              layerDepth, inset, oobaseu, halfwidth;
    RwV3d               *pos;

    RwRGBA              *preLit;

    RwTexCoords         *texCoord;

    rt2dPathNode       *pNode;
    RwV2d               avgnormal;
    rt2dShadeParameters sp;

    RWFUNCTION(RWSTRING("_rt2dPathStrokeCreateGeometry"));

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

    /* Count the total number of verts and tris */
    subPath = path->next;

    do
    {
        numVerts = _rt2dPathGetNumSegment(subPath);

        totalVerts += (numVerts * 2);
        totalTris += (numVerts * 2) - 2;

        subPath = subPath->next;
    } while (subPath != path->next);

    /* Main build loop */
    layerDepth = Rt2dGlobals.layerDepth;
    halfwidth = brush->halfwidth;

    subPath = path;

    if ((totalTris > 0) && (totalVerts > 0))
    {
        morphTarget = RpGeometryGetMorphTarget(geom, 0);

        /* Get the start of the data areas */
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

            pNode = _rt2dPathGetSegmentArray(subPath);
            numVerts = _rt2dPathGetNumSegment(subPath);

            lastIdx = firstIdx + (numVerts * 2) - 1;

            oobaseu = 1.0f / pNode[numVerts - 1].dist;

            for (i = 0; i < numVerts; i++, pNode++)
            {
                /* corner normal */
                RwV2dScale(&avgnormal, &pNode->normal, (halfwidth + inset));

                RwV2dSub((RwV2d *) pos, &pNode->pos, &avgnormal);
                pos->z = layerDepth;

                if (brush->flag & FIELDRGBA)  /* It's not flat */
                {
                    RwRGBARealScale(&sp.col, &brush->dtop.col,
                                    pNode->dist * oobaseu);
                    RwRGBARealAdd(&sp.col, &brush->top.col, &sp.col);

                    preLit->red   = RwInt32FromRealMacro(sp.col.red);
                    preLit->green = RwInt32FromRealMacro(sp.col.green);
                    preLit->blue  = RwInt32FromRealMacro(sp.col.blue);
                    preLit->alpha = RwInt32FromRealMacro(sp.col.alpha);

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

                if (brush->flag & FIELDUV)
                {
                    RwV2dScale(&sp.uv, &brush->dtop.uv,
                            pNode->dist * oobaseu);
                    RwV2dAdd(&sp.uv, &brush->top.uv, &sp.uv);

                    texCoord->u = sp.uv.x;
                    texCoord->v = sp.uv.y;

                    texCoord++;
                }

                pos++;

                RwV2dScale(&avgnormal, &pNode->normal, (halfwidth - inset));

                RwV2dAdd((RwV2d *) pos, &pNode->pos, &avgnormal);
                pos->z = layerDepth;

                if (brush->flag & FIELDRGBA) /* it's not flat */
                {
                    RwRGBARealScale(&sp.col, &brush->dbottom.col,
                                    pNode->dist * oobaseu);
                    RwRGBARealAdd(&sp.col, &brush->bottom.col, &sp.col);

                    preLit->red   = RwInt32FromRealMacro(sp.col.red);
                    preLit->green = RwInt32FromRealMacro(sp.col.green);
                    preLit->blue  = RwInt32FromRealMacro(sp.col.blue);
                    preLit->alpha = RwInt32FromRealMacro(sp.col.alpha);

                    preLit++;
                }
                else /* it's flat */
                {
                    /* entirely using the material color, no preLit interference thanks */
                    preLit->red   = 255;
                    preLit->green = 255;
                    preLit->blue  = 255;
                    preLit->alpha = 255;

                    preLit++;
                }

                if (brush->flag & FIELDUV)
                {
                    RwV2dScale(&sp.uv, &brush->dbottom.uv,
                            pNode->dist * oobaseu);
                    RwV2dAdd(&sp.uv, &brush->bottom.uv, &sp.uv);

                    texCoord->u = sp.uv.x;
                    texCoord->v = sp.uv.y;

                    texCoord++;
                }

                pos++;
            }

            /* Generate the tri indexes */
            idx0 = firstIdx;

            for (i = 0; i < (numVerts - 1); i++)
            {
                RpGeometryTriangleSetVertexIndices(geom, tri,
                    (RwUInt16)idx0,
                    (RwUInt16)(idx0 + 1),
                    (RwUInt16)(idx0 + 2));

                RpGeometryTriangleSetMaterial(geom, tri, material);

                tri++;

                RpGeometryTriangleSetVertexIndices(geom, tri,
                    (RwUInt16)(idx0 + 1),
                    (RwUInt16)(idx0 + 3),
                    (RwUInt16)(idx0 + 2));

                RpGeometryTriangleSetMaterial(geom, tri, material);

                tri++;

                idx0 += 2;
            }

            firstIdx = lastIdx + 1;

            /* Next path */
            subPath = subPath->next;

        } while (subPath != path->next);
    }

    /* Decrease the ref count */
    RpMaterialDestroy(material);

    /* Return the new index positions for the next path. */
    *geomVertIdx += totalVerts;
    *geomTriIdx += totalTris;

    RWRETURN(path);
}

