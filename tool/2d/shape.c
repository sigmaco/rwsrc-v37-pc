/*
 *
 * Functionality for 2D rendering
 *
 * Copyright (c) 2001 Criterion Software Ltd.
 */

/****************************************************************************
 *                                                                          *
 *  Module  :   shape.c                                                     *
 *                                                                          *
 *  Purpose :   Hierarchical 2d shape representation                        *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
 */
#include <string.h>

#include "rwcore.h"
#include "rpdbgerr.h"
#include "rpworld.h"

#include "rt2d.h"
#include "object.h"
#include "font.h"
#include "path.h"
#include "tri.h"
#include "gstate.h"
#include "brush.h"
#include "shape.h"
#include "fill.h"
#include "stroke.h"

/*-----------------20/08/2001 10:20 AJH -------------
 * Is that really needed ???
 * --------------------------------------------------*/
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
static RwInt32
ShapeRepGetNodeCount(_rt2dShapeRep *rep)
{
    RWFUNCTION(RWSTRING("ShapeRepGetNodeCount"));
    RWASSERT(rep);

    RWRETURN(_rwSListGetNumEntries(rep->nodes));
}

_rt2dShapeRep *
_rt2dShapeRepCreate(RwInt32 nodeCount)
{
    _rt2dShapeRep *rep;
    RWFUNCTION(RWSTRING("_rt2dShapeRepCreate"));

    rep = (_rt2dShapeRep *)RwMalloc(sizeof(_rt2dShapeRep),
                        rwMEMHINTDUR_EVENT | rwID_2DPLUGIN);
    rep->refCount = 1;

    rep->nodes = rwSListCreate(sizeof(_rt2dShapeNode),
                        rwMEMHINTDUR_EVENT | rwID_2DSHAPE);
    rep->geometry = NULL;

    if( nodeCount )
    {
        rwSListGetNewEntries(rep->nodes, nodeCount,
            rwMEMHINTDUR_EVENT | rwID_2DSHAPE);
    }

    RWRETURN(rep);
}

RwBool
_rt2dShapeRepDestroy(_rt2dShapeRep *rep)
{
    RwInt32             nodeCount,i;
    _rt2dShapeNode      *nodeList;

    RWFUNCTION(RWSTRING("_rt2dShapeRepDestroy"));

    if (0 == (--(rep->refCount)))
    {
        nodeCount = ShapeRepGetNodeCount(rep);
        nodeList = (_rt2dShapeNode*)(rwSListGetArray(rep->nodes));

        for(i=0;i<nodeCount;i++)
        {
            if (nodeList->path)
            {
                Rt2dPathDestroy(nodeList->path);

                Rt2dBrushDestroy(nodeList->brush);
            }

            nodeList++;
        }

        /* Destruct node list */
        rwSListDestroy(rep->nodes);

        if (rep->geometry)
        {
            RpGeometryDestroy(rep->geometry);

            rep->geometry = NULL;
        }

        RwFree(rep);

        RWRETURN(TRUE);
    }

    RWRETURN(FALSE);
}

RwUInt32
_rt2dShapeRepAddRef(_rt2dShapeRep *rep)
{
    RWFUNCTION(RWSTRING("_rt2dShapeRepAddRef"));

    ++rep->refCount;

    RWRETURN(rep->refCount);
}


/* Function to create a geometry from meshs */
static RpGeometry *
CreateGeometryFromMeshes(_rt2dPathMeshHeader *pMeshHeader,
                         _rt2dPathMeshNode *pMeshNode,
                         RwUInt32 nodeCount,
                         RwUInt32 format)
{
    RpGeometry  *geom = (RpGeometry*)NULL;
    Rt2dPath    *subPath;
    RwUInt32    i;

    RWFUNCTION(RWSTRING("CreateGeometryFromMeshes"));

    /* Main build loop */
    if (pMeshHeader->node)
    {
        RwUInt32 vertIdx, triIdx;

        /* !!! Could this be done at fn level !!! - GJB */
        geom = RpGeometryCreate(pMeshHeader->numVert, pMeshHeader->numTri, format);

        /* Walk the path, creating the tris and transfering the verts over */
        vertIdx = 0;
        triIdx = 0;

        for (i = 0; i < nodeCount; i++)
        {
            if (pMeshNode->path != NULL)
            {
                subPath = pMeshNode->path->next;

                if (pMeshNode->flag & rt2dSHAPENODEFLAGSOLID)
                {
                    _rt2dPathFillCreateGeometry(subPath, pMeshNode->brush,
                        geom, &vertIdx, &triIdx);
                }
                else
                {
                    _rt2dPathStrokeCreateGeometry(subPath, pMeshNode->brush,
                        geom, &vertIdx, &triIdx);
                }
            }

            ++pMeshNode;
        }
    }

    /* Set up the bounding-sphere */
    if (geom != NULL)
    {
        RpMorphTarget       *morphTarget = RpGeometryGetMorphTarget(geom, 0);
        RwSphere            bsphere;

        RpMorphTargetCalcBoundingSphere(morphTarget, &bsphere);
        RpMorphTargetSetBoundingSphere(morphTarget, &bsphere);

        /* Geometry creation complete. Lock it */
        RpGeometryUnlock(geom);
    }

    RWRETURN(geom);
}


/*
 * Function to convert a shape to a geometry
 */
static RpGeometry *
ShapeRepCreateGeometry(_rt2dShapeRep *rep)
{
    Rt2dPath            *subPath, *tmpPath;
    RpGeometry          *geometry;
    RwUInt32            i, numVerts, count;
    RwUInt32            format;

    _rt2dShapeNode          *current, *finish;
    _rt2dPathMeshNode       *pMeshNode;
    _rt2dPathMeshHeader     pMeshHeader;

    RpTriStripMeshCallBack          cback;
    void                            *cbackData;

    RWFUNCTION(RWSTRING("ShapeRepCreateGeometry"));

    /* Save the current tristripper for restoring later */
    RpMeshGetTriStripMethod(&cback, &cbackData);

    /* Pre-process */
    format =
        (rpGEOMETRYTRISTRIP |
         rpGEOMETRYPOSITIONS |
         rpGEOMETRYTEXTURED |
         rpGEOMETRYMODULATEMATERIALCOLOR |
         rpGEOMETRYLIGHT |
         rpGEOMETRYPRELIT);

    count =_rwSListGetNumEntries(rep->nodes);
    current = (_rt2dShapeNode*)(rwSListGetArray(rep->nodes));
    finish = current + count;

    /* Build a tmp list of paths that can be instanced. Paths will be flattened and
     * non-closed path ignored.
     */
    pMeshNode = (_rt2dPathMeshNode *) RwMalloc(count * sizeof(_rt2dPathMeshNode),
                                           rwMEMHINTDUR_EVENT | rwID_2DPLUGIN);
    pMeshHeader.node = pMeshNode;
    pMeshHeader.numNode = count;
    pMeshHeader.numVert = 0;
    pMeshHeader.numIdx = 0;
    pMeshHeader.numTri = 0;

    i = 0;
    while (current != finish)
    {
        subPath = current->path;

        pMeshNode[i].path = NULL;
        pMeshNode[i].numVert = 0;
        pMeshNode[i].numTri = 0;
        pMeshNode[i].numIdx = 0;
        pMeshNode[i].brush = current->brush;
        pMeshNode[i].flag = current->flag;

        /* Check if the path is stroke or fill */
        if (current->flag & rt2dSHAPENODEFLAGSOLID)
        {
            /*
             * Fill path.
             */
            while (subPath != NULL)
            {
                /* Path must be closed */
                if (subPath->closed)
                {
                    /* we'll be using the flattened path */
                    tmpPath = Rt2dPathCreate();

                    if (!subPath->flat)
                    {
                        _rt2dSubPathFlatten(tmpPath, subPath);

                        numVerts = _rt2dPathGetNumSegment(tmpPath);
                    }
                    else
                    {
                        /* Copy over the segments, borrow the 'flat' flag to mark if this
                         * path's segments can be destroyed later.
                         */
                        _rwSListDestroy(tmpPath->segments);

                        tmpPath->segments = subPath->segments;
                        tmpPath->numSegment = subPath->numSegment;
                        tmpPath->closed = subPath->closed;
                        tmpPath->flat = FALSE;

                        numVerts = _rt2dPathGetNumSegment(subPath);
                    }

                    tmpPath->inset = subPath->inset;

                    pMeshNode[i].numVert += numVerts;
                    pMeshNode[i].numTri += (numVerts - 3);
                    pMeshNode[i].numIdx += (numVerts - 3) * 3;

                    /* Add the path the list */
                    if (pMeshNode[i].path == NULL)
                    {
                        tmpPath->next = tmpPath;
                    }
                    else
                    {
                        tmpPath->next = pMeshNode[i].path->next;
                        pMeshNode[i].path->next = tmpPath;
                    }
                    pMeshNode[i].path = tmpPath;
                }

                subPath = subPath->next;
            }
        }
        else
        {
            /*
             * Stroke path.
             */
            while (subPath != NULL)
            {
                /* we'll be using the flattened path */
                tmpPath = Rt2dPathCreate();

                if (!subPath->flat)
                {
                    _rt2dSubPathFlatten(tmpPath, subPath);

                    numVerts = _rt2dPathGetNumSegment(tmpPath);
                }
                else
                {
                    /* Copy over the segments, borrow the 'flat' flag to mark if this
                     * path's segments can be destroyed later.
                     */
                    _rwSListDestroy(tmpPath->segments);

                    tmpPath->segments = subPath->segments;
                    tmpPath->numSegment = subPath->numSegment;
                    tmpPath->closed = subPath->closed;
                    tmpPath->flat = FALSE;

                    numVerts = _rt2dPathGetNumSegment(subPath);
                }

                tmpPath->inset = subPath->inset;


                pMeshNode[i].numVert += (numVerts * 2);
                pMeshNode[i].numTri += (numVerts * 2) - 2;
                pMeshNode[i].numIdx += ((numVerts * 2) - 2) * 3;


                /* Add the path the list */
                if (pMeshNode[i].path == NULL)
                {
                    tmpPath->next = tmpPath;
                }
                else
                {
                    tmpPath->next = pMeshNode[i].path->next;
                    pMeshNode[i].path->next = tmpPath;
                }
                pMeshNode[i].path = tmpPath;

                subPath = subPath->next;
            }
        }

        pMeshHeader.numVert += pMeshNode[i].numVert;
        pMeshHeader.numTri += pMeshNode[i].numTri;
        pMeshHeader.numIdx += pMeshNode[i].numIdx;

        /* Next node */
        current++;
        i++;
    }

    /* create the geometry */
    geometry =
        CreateGeometryFromMeshes(&pMeshHeader, pMeshNode, count, format);

    /* Clean up */
    if (pMeshHeader.node)
    {
        for (i = 0; i < pMeshHeader.numNode; i++)
        {
            subPath = pMeshNode[i].path->next;

            do
            {
                /* Check if the segements needs to be destroyed */
                if (subPath->flat == FALSE)
                    subPath->segments = NULL;

                subPath = subPath->next;

            } while (subPath != pMeshNode[i].path->next);

            pMeshNode[i].path->next = NULL;

            Rt2dPathDestroy(subPath);
        }

        RwFree((RwChar *) pMeshHeader.node);
    }

    /* Restore the current tristripper for restoring later */
    /* RpMeshSetTriStripMethod(cback, cbackData); */

    /* Assign the geometry to the representation */
    rep->geometry = geometry;

    RWRETURN(geometry);
}

/* Cache the material starting colors */
static RwRGBA *
ShapeCreateColorCache(RpGeometry *geom)
{
    RwUInt32 count = RpGeometryGetNumMaterials(geom);
    RwUInt32 i;
    RwRGBA *colorCache;

    RWFUNCTION(RWSTRING("ShapeCreateColorCache"));

    if (!count)
    {
        RWRETURN((RwRGBA *)NULL);
    }

    colorCache = (RwRGBA *)RwMalloc(count * sizeof(RwRGBA),
                        rwID_2DPLUGIN | rwMEMHINTDUR_EVENT);

    for (i = 0; i < count; i++)
    {
        RpMaterial *material = RpGeometryGetMaterial(geom, i);

        colorCache[i] = *RpMaterialGetColor(material);
    }

    RWRETURN(colorCache);
}

RwUInt32
_rt2dShapeRepStreamGetSize(_rt2dShapeRep *rep)
{
    RwUInt32 size = 0;
    RwUInt32 nodeSize = 0;
    _rt2dShapeNode *nodeList;
    RwInt32 nodeCount,i;

    RWFUNCTION(RWSTRING("_rt2dShapeRepStreamGetSize"));

    nodeCount = _rwSListGetNumEntries(rep->nodes);
    nodeList = (_rt2dShapeNode *)
        _rwSListGetArray(rep->nodes);

    for(i=0;i<nodeCount;i++)
    {
        nodeSize = sizeof(_rt2dStreamShapeNode) +
                   sizeof(RwUInt32);

        if(nodeList->path)
        {
            nodeSize += Rt2dPathStreamGetSize(nodeList->path);
        }

        if(nodeList->brush)
        {
            nodeSize += Rt2dBrushStreamGetSize(nodeList->brush);
        }

        size += nodeSize;
        nodeList++;
    }

    RWRETURN(size);
}

_rt2dShapeRep *
_rt2dShapeRepStreamRead(RwStream *stream)
{
    _rt2dStreamShape sShape;
    _rt2dStreamShapeNode sShapeNode;
    RwUInt32 size, version;
    _rt2dShapeNode *nodeList;
    RwInt32 i;
    RwSList *sList;
    _rt2dShapeRep *rep;

    RWFUNCTION(RWSTRING("_rt2dShapeRepStreamRead"));

    if (RwStreamRead(stream, &sShape, sizeof(sShape)) != sizeof(sShape))
    {
        RWRETURN((_rt2dShapeRep *)NULL);
    }

    /* Convert it */
    (void)RwMemNative32(&sShape, sizeof(sShape));

    RWASSERT(sShape.version <= FILE_LAST_SUPPORTED_VERSION && sShape.version >= FILE_CURRENT_VERSION);

    rep = _rt2dShapeRepCreate(sShape.nodeCount);

    sList = rep->nodes;

    nodeList = (_rt2dShapeNode *)
        _rwSListGetArray(sList);

    /* Read Nodes */
    for(i=0;i<sShape.nodeCount;i++)
    {
        /* Read Header*/
        if (RwStreamRead(stream, &sShapeNode, sizeof(sShapeNode)) != sizeof(sShapeNode))
        {
            RWRETURN((_rt2dShapeRep *)NULL);
        }

        /* Convert it */
        (void)RwMemNative32(&sShapeNode, sizeof(sShapeNode));
        RWASSERT((sShapeNode.flags & rt2dSTREAMSHAPENODEGOTPATH));

        /* Read Stroke brush and path*/
        if( sShapeNode.flags & rt2dSTREAMSHAPENODEGOTPATH)
        {
            /* Read flag */
            RwStreamReadInt32(stream, (RwInt32 *) &nodeList->flag, sizeof(RwUInt32));

            if (!RwStreamFindChunk(stream, rwID_2DPATH, &size, &version))
            {
                RWRETURN((_rt2dShapeRep *)NULL);
            }

            nodeList->path = Rt2dPathCreate();
            Rt2dPathUnlock(nodeList->path);
            _rt2dPathStreamReadTo(nodeList->path, stream);

            if (!RwStreamFindChunk(stream, rwID_2DBRUSH, &size, &version))
            {
                RWRETURN((_rt2dShapeRep *)NULL);
            }
            nodeList->brush = Rt2dBrushCreate();
            _rt2dBrushStreamReadTo(nodeList->brush,stream);
        }
        else
        {
            nodeList->path  = (Rt2dPath *) NULL;
            nodeList->brush = (Rt2dBrush *) NULL;
        }

        nodeList++;
    }

    /* Create the geometry */
    rep->geometry = ShapeRepCreateGeometry(rep);

    RWRETURN(rep);
}

_rt2dShapeRep *
_rt2dShapeRepStreamWrite(_rt2dShapeRep *rep, RwStream *stream)
{
    _rt2dShapeNode *nodeList;
    _rt2dStreamShape sShape;
    _rt2dStreamShapeNode sShapeNode;
    RwInt32 nodeCount,i;
    RWFUNCTION(RWSTRING("_rt2dShapeRepStreamWrite"));

    nodeCount = _rwSListGetNumEntries(rep->nodes);
    nodeList = (_rt2dShapeNode *)
        _rwSListGetArray(rep->nodes);

    sShape.version = FILE_CURRENT_VERSION;
    sShape.nodeCount = nodeCount;

    (void)RwMemLittleEndian32(&sShape, sizeof(sShape));

    if (!RwStreamWrite(stream, &sShape, sizeof(sShape)))
    {
        RWRETURN((_rt2dShapeRep *)NULL);
    }

    /* Write Nodes */
    for(i=0;i<nodeCount;i++)
    {
        /* Write Nodes Header*/
        sShapeNode.flags = 0;

        if(nodeList->path)
        {
            sShapeNode.flags |= rt2dSTREAMSHAPENODEGOTPATH;
        }

        (void)RwMemLittleEndian32(&sShapeNode, sizeof(sShapeNode));

        if (!RwStreamWrite(stream, &sShapeNode, sizeof(sShapeNode)))
        {
            RWRETURN((_rt2dShapeRep *)NULL);
        }

        if (nodeList->path)
        {
            /* Write shape flag */
            RwStreamWriteInt32(stream, (RwInt32 *) &nodeList->flag, sizeof(RwUInt32));

            /* Write Stroke Path */
            if ( !Rt2dPathStreamWrite(nodeList->path,stream))
            {
                RWRETURN((_rt2dShapeRep *)NULL);
            }

            /* Write Stroke brush */
            if(nodeList->brush)
            {
                if ( !Rt2dBrushStreamWrite(nodeList->brush,stream))
                {
                    RWRETURN((_rt2dShapeRep *)NULL);
                }
            }
        }

        nodeList++;
    }

    RWRETURN(rep);
}


/****************************************************************************/

/**
 * \ingroup rt2dshape
 * \page rt2dshapeoverview Rt2dShape Overview
 *
 * Shapes are collections of brushes and paths which are added
 * together using nodes. Shapes can be saved and added to scenes.
 *
 * \par Creating Shapes
 * -# \ref Rt2dShapeCreate creates the shape.
 * -# \ref Rt2dShapeAddNode adds the shape, path, brushes together.
 *
 * \par Adding Shapes to Scenes
 * -# \ref Rt2dSceneLock locks the scene.
 * -# \ref Rt2dSceneAddChild adds the shape to the scene.
 * -# \ref Rt2dSceneUnlock unlocks the scene.
 *
 * \note The newly created geometry is in a locked state and must be unlocked
 * if it is to be used in any rendering. 
 */

/**
 * \ingroup rt2dshape
 * \ref Rt2dShapeCreate is used to create and initialize a new shape object.
 * To create a new shape:
 *
 * -# \ref Rt2dShapeCreate creates the shape.
 * -# \ref Rt2dShapeAddNode adds the shape, path, brushes together.
 *
 * \return Returns a pointer to the new shape object if successful, NULL
 * otherwise.
 *
 * \note The newly created geometry is in a locked state and must be unlocked
 * if it is to be used in any rendering. 
 *
 * \see Rt2dShapeAddNode
 * \see Rt2dShapeDestroy
 */
Rt2dObject  *
Rt2dShapeCreate(void)
{
    Rt2dObject           *object;

    RWAPIFUNCTION(RWSTRING("Rt2dShapeCreate"));

    object = _rt2dObjectCreate();
    RWASSERT(object);
    _rt2dObjectInit(object, rt2DOBJECTTYPESHAPE);

    _rt2dShapeInit(object, 0, NULL);

    RWRETURN(object);
}

RwBool
_rt2dShapeInit(Rt2dObject *shape, RwInt32 nodeCount, _rt2dShapeRep *rep)
{
    RwBool result = TRUE;
    RWFUNCTION(RWSTRING("_rt2dShapeInit"));
    RWASSERT(shape);

    if (rep)
    {
        shape->data.shape.rep = rep;
        _rt2dShapeRepAddRef(rep);
    }
    else
    {
        /* Create internal representation */
        shape->data.shape.rep = _rt2dShapeRepCreate(nodeCount);

        shape->data.shape.colorCache = NULL;
    }

    shape->flag |= Rt2dObjectIsLocked;

    RWRETURN(result);
}

/**
 * \ingroup rt2dshape
 * \ref Rt2dShapeDestroy is called to destroy the specified shape.
 *
 * \note If a shape has been added to a scene the scene takes ownership
 * of the shape and the shape can only be destroyed as part of the scene
 * using \ref Rt2dSceneDestroy.
 *
 * \param object Pointer to the shape object to destroy.
 *
 * \return Returns TRUE if successful, FALSE otherwise.
 *
 * \see Rt2dShapeCreate
 * \see Rt2dSceneCreate
 * \see Rt2dSceneDestroy
 */
RwBool
Rt2dShapeDestroy(Rt2dObject *object)
{
    RwBool              result;

    RWAPIFUNCTION(RWSTRING("Rt2dShapeDestroy"));

    /* NULL path is valid */
    result = (NULL != object);
    if (result)
    {
        /* Destruct contents */
        result = _rt2dShapeDestruct(object);

        /* Destroy base object */
        RwFreeListFree(Rt2dGlobals.objectFreeList, object);
    }

    RWRETURN(result);
}


RwBool
_rt2dShapeDestruct(Rt2dObject *shape)
{
    RwBool              result = TRUE;

    RWFUNCTION(RWSTRING("_rt2dShapeDestruct"));

    RWASSERT(shape);
    RWASSERT(((shape->type) == rt2DOBJECTTYPESHAPE));

    /* Destroy the shared representation */
    _rt2dShapeRepDestroy(shape->data.shape.rep);

    /* Destroy the atomic */
    if (shape->data.shape.atomic)
    {
        RwFrame         *frame;

        frame = RpAtomicGetFrame(shape->data.shape.atomic);

        if (frame)
        {
            RpAtomicSetFrame(shape->data.shape.atomic, NULL);


            RwFrameDestroy(frame);
        }

        RpAtomicDestroy(shape->data.shape.atomic);
    }

    /* Destroy the color cache */
    if (shape->data.shape.colorCache != NULL)
    {
        RwFree(shape->data.shape.colorCache);

        shape->data.shape.colorCache = NULL;
    }

    RWRETURN(result);
}

Rt2dObject *
_rt2dShapeCopy(Rt2dObject *dst, Rt2dObject *src)
{
    RWFUNCTION(RWSTRING("_rt2dShapeCopy"));
    RWASSERT(dst);
    RWASSERT(src);
    RWASSERT(rt2DOBJECTTYPESHAPE == src->type);

    memcpy(dst, src, sizeof(Rt2dObject));

    _rt2dShapeRepAddRef(src->data.shape.rep);

    if (src->data.shape.atomic)
    {
        dst->data.shape.atomic = RpAtomicClone(src->data.shape.atomic);
    }

    dst->data.shape.colorCache = NULL;

    RWRETURN(dst);
}

/**
 * \ingroup rt2dshape
 * \ref Rt2dShapeAddNode adds the shape, path and brush together. The property of the particular
 * path node, such as solid or outline, is defined by the flag parameter. See \ref Rt2dShapeNodeFlag.
 * Shapes with color-gradient brushes do not support dynamic update of colors
 *
 * \param shape  Pointer to the shape object.
 * \param flag   Shape's property flag.
 * \param path   Pointer to the path object.
 * \param brush  Pointer to the brush object.
 *
 * \return Returns pointer to a shape, NULL otherwise.
 *
 * \see Rt2dShapeCreate
 * \see Rt2dShapeGetNewPath
 * \see Rt2dShapeGetNodeCount
 */
Rt2dObject *
Rt2dShapeAddNode(Rt2dObject *shape, RwUInt32 flag, Rt2dPath *path, Rt2dBrush *brush )
{
    _rt2dShapeNode *node;

    RWAPIFUNCTION(RWSTRING("Rt2dShapeAddNode"));
    RWASSERT(shape);
    RWASSERT(rt2DOBJECTTYPESHAPE == shape->type);
    RWASSERT(path);
    RWASSERT(brush);

    node= (_rt2dShapeNode*)(_rwSListGetNewEntry(shape->data.shape.rep->nodes,
                                         rwID_2DSHAPE | rwMEMHINTDUR_EVENT));

    RWASSERT(node);

    /* The shape should be locked */
    RWASSERT((shape->flag & Rt2dObjectIsLocked));

    node->flag  = flag;
    node->path  = path;
    node->brush = brush;

    brush->refCount++;

    RWRETURN(shape);
}

/**
 * \ingroup rt2dshape
 * \ref Rt2dShapeGetNodeCount is used to determine the number of nodes
 * in a shape.
 *
 * \param shape Pointer to a shape.
 *
 * \return Returns the number of nodes in a shape.
 *
 * \see Rt2dShapeCreate
 * \see Rt2dShapeAddNode
 */
RwInt32
Rt2dShapeGetNodeCount(Rt2dObject* shape)
{
    RWAPIFUNCTION(RWSTRING("Rt2dShapeGetNodeCount"));
    RWASSERT(shape);
    RWASSERT(rt2DOBJECTTYPESHAPE == shape->type);

    RWRETURN(ShapeRepGetNodeCount(shape->data.shape.rep));
}

#define BrushSetup()                                                \
MACRO_START                                                         \
{                                                                   \
    memcpy(&brushColors[0], &brush->bottom.col, sizeof(RwRGBAReal));    \
    memcpy(&brushColors[1], &brush->dbottom.col, sizeof(RwRGBAReal));   \
    memcpy(&brushColors[2], &brush->dtop.col, sizeof(RwRGBAReal));      \
    memcpy(&brushColors[3], &brush->top.col, sizeof(RwRGBAReal));       \
    _rt2dBrushTransformRGBARealDirect(                                \
                                brush,                                \
                                &brushColors[0],                      \
                                &brushColors[1],                      \
                                &brushColors[2],                      \
                                &brushColors[3],                      \
                                &multColor,                           \
                                &offsColor );                         \
}                                                                     \
MACRO_STOP

#define BrushUnsetup()                                              \
MACRO_START                                                         \
{                                                                   \
    memcpy(&brush->bottom.col,  &brushColors[0], sizeof(RwRGBAReal));   \
    memcpy(&brush->dbottom.col, &brushColors[1], sizeof(RwRGBAReal));   \
    memcpy(&brush->dtop.col, &brushColors[2], sizeof(RwRGBAReal));      \
    memcpy(&brush->top.col, &brushColors[3], sizeof(RwRGBAReal));       \
}                                                                   \
MACRO_STOP

/**
 * \ingroup rt2dshape
 * \ref Rt2dShapeRender renders a 2d shape. Shapes can be rendered
 * individually. If a shape is part of a scene, the \ref Rt2dSceneRender
 * renders all visible objects.
 *
 * \param shape Pointer to the shape object to render
 *
 * \return Returns shape object if successful, NULL otherwise
 *
 * \see Rt2dShapeCreate
 * \see Rt2dSceneRender
 */
Rt2dObject *
Rt2dShapeRender(Rt2dObject *shape)
{
    RWAPIFUNCTION(RWSTRING("Rt2dShapeRender"));

    RWRETURN(_rt2dShapeRenderChild(shape, &_rt2dColorMultUnity, &_rt2dColorOffsZero));
}

Rt2dObject *
_rt2dShapeRenderChild(Rt2dObject *shape, const RwRGBAReal *colorMult, const RwRGBAReal *colorOffs)
{
    _rt2dShapeNode *current, *finish;
    Rt2dBrush *brush;
    RwRGBAReal multColor,offsColor;
    RwRGBAReal brushColors[4];
    RwRGBA      matColor;
    RpMaterial  *material;
    RpGeometry  *geometry;
    RwUInt32    i, numMaterial;

    RWFUNCTION(RWSTRING("_rt2dShapeRenderChild"));
    RWASSERT(shape);
    RWASSERT(shape->type == rt2DOBJECTTYPESHAPE);
    /* Object level stuff */
    if(shape->flag & Rt2dObjectVisible)
    {
        RwRGBARealTransformAssignMult(&multColor, &shape->colorMult, colorMult);
        RwRGBARealTransformAssignOffsetFinal(&offsColor, &shape->colorOffs, colorMult, colorOffs);

        /* Get ready to draw */
        if (shape->data.shape.atomic == NULL)
        {
            if((shape->flag & Rt2dObjectDirtyLTM) == Rt2dObjectDirtyLTM)
            {
                _rt2dCTMPush(&shape->MTM);
                RwMatrixCopy(&shape->LTM, _rt2dCTMGetDirect());

                shape->flag &= ~Rt2dObjectDirtyLTM;

            }
            else
            {
                _rt2dCTMSet(&shape->LTM);
            }

            current = (_rt2dShapeNode*)(rwSListGetArray(shape->data.shape.rep->nodes));
            finish = current + _rwSListGetNumEntries(shape->data.shape.rep->nodes);

            /* Draw all nodes of the current shape*/
            while (current!=finish)
            {

                brush = current->brush;

                /* save brush color */
                /*AJH:
                *  Is that really neaded, I don't know,
                * I guess object state should be persistent
                */
                BrushSetup();

                /* Do any necessary updates to the filled shape */
                if (current->flag & rt2dSHAPENODEFLAGSOLID)
                {
                    Rt2dPathFill(current->path, brush);
                }
                else
                {
                    Rt2dPathStroke(current->path, brush);
                }

                /* copy them back */
                /*AJH:
                *  Is that really neaded, I don't know,
                * I guess object state should be persistent
                */
                BrushUnsetup();

                /* Next node */
                ++current;
            }
        }
        else
        {
#if 0
/* ====================================================================================================
 * Must always do this block now that we're using possibly shared geometries. Needs a custom pipeline.
 */
             /* Save and modify the material color */
/*
 *          if (shape->flag & Rt2dObjectDirtyColor)
 *          {
 */
/* ====================================================================================================
 */
#endif
                geometry = shape->data.shape.rep->geometry;

#if 0
                /* Flag the prelights as dirty */
                RpGeometryLock(geometry, rpGEOMETRYLOCKPRELIGHT);
                RpGeometryUnlock(geometry);
#endif
                numMaterial = RpGeometryGetNumMaterials(geometry);

                for (i = 0; i < numMaterial; i++)
                {
                    RwRGBARealTransformAssignClampAndConvert(
                        &matColor, &(shape->data.shape.colorCache[i]), &multColor, &offsColor);

                    material = RpGeometryGetMaterial(geometry, i);
                    RpMaterialSetColor(material, &matColor);
                }

                shape->flag &= ~Rt2dObjectDirtyColor;
#if 0
/* ====================================================================================================
 *           }
 * ====================================================================================================
 */
#endif

            /* Replace the atomic's frame */
            if((shape->flag & Rt2dObjectDirtyLTM) == Rt2dObjectDirtyLTM)
            {
                _rt2dCTMPush(&shape->MTM);
                RwMatrixCopy(&shape->LTM, _rt2dCTMGetDirect());

                shape->flag &= ~Rt2dObjectDirtyLTM;
            }
            else
            {
                _rt2dCTMSet(&shape->LTM);
            }


            RwFrameTransform(
                RpAtomicGetFrame( shape->data.shape.atomic),
                _rt2dCTMGet(), rwCOMBINEREPLACE);

            /* Stroke the path */
            RpAtomicRender( shape->data.shape.atomic );
        }

        /* Restore CTM */
        Rt2dCTMPop();
    }

    RWRETURN(shape);
}

/* Inner level write; uses pool if given */
Rt2dObject *
_rt2dShapeStreamWrite(Rt2dObject *shape, RwStream *stream, _rt2dSceneResourcePool *pool)
{
    RWFUNCTION(RWSTRING("_rt2dShapeStreamWrite"));
    RWASSERT(shape);
    RWASSERT(stream);

    /* Write Rt2dObject base information*/
    if (!_rt2dObjectBaseStreamWrite(shape,stream))
    {
        RWRETURN((Rt2dObject *)NULL);
    }

    if (pool)
    {
        RwInt32 pos;
        /* write ref in pool*/
        if (!_rt2dSceneResourcePoolFindShapeRep(pool, shape->data.shape.rep, &pos))
        {
            RWRETURN((Rt2dObject *)NULL);
        }

        if (!RwStreamWriteInt32(stream, &pos, sizeof(RwUInt32)))
        {
            RWRETURN((Rt2dObject *)NULL);
        }
    }
    else
    {
        /* Write shape representation */
        if (!_rt2dShapeRepStreamWrite(shape->data.shape.rep, stream))
        {
            RWRETURN((Rt2dObject *)NULL);
        }
    }

    RWRETURN(shape);
}

/**
 * \ingroup rt2dshape
 * \ref Rt2dShapeStreamWrite is used to write the specified shape to the given
 * binary stream. Note that the stream will have been opened prior to this
 * function call.
 *
 * \param shape Pointer to the shape to stream.
 * \param stream Pointer to the stream.
 *
 * \return Returns a pointer to the shape if successful, NULL otherwise.
 *
 * \see Rt2dShapeStreamRead
 * \see Rt2dShapeStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 */
Rt2dObject *
Rt2dShapeStreamWrite(Rt2dObject *shape, RwStream *stream)
{
    RwUInt32 size;

    RWAPIFUNCTION(RWSTRING("Rt2dShapeStreamWrite"));
    RWASSERT(shape);
    RWASSERT(stream);

    size = Rt2dShapeStreamGetSize(shape);

    if (!RwStreamWriteChunkHeader(stream, rwID_2DSHAPE, size))
    {
        RWRETURN((Rt2dObject *)NULL);
    }

    if (!_rt2dShapeStreamWrite(shape, stream, NULL))
    {
        RWRETURN((Rt2dObject *)NULL);
    }

    RWRETURN(shape);
}

/* inner-level stream get size; takes into account pooled resources */
RwUInt32
_rt2dShapeStreamGetSize(Rt2dObject *shape, _rt2dSceneResourcePool *pool)
{
    RwUInt32 size;

    RWFUNCTION(RWSTRING("_rt2dShapeStreamGetSize"));
    RWASSERT(shape);
    RWASSERT(shape->type == rt2DOBJECTTYPESHAPE);

    size = _rt2dObjectBaseStreamGetSize(shape);

    if (pool)
    {
        size += sizeof(RwUInt32);
    }
    else
    {
        size += _rt2dShapeRepStreamGetSize(shape->data.shape.rep);
    }

    size += rwCHUNKHEADERSIZE;

    RWRETURN(size);
}


/**
 * \ingroup rt2dshape
 * \ref Rt2dShapeStreamGetSize is used to determine the size in bytes of
 * the binary representation of the given shape. This value is used in
 * the binary chunk header to indicate the size of the chunk. The size
 * does include the size of the chunk header.
 *
 * \param shape Pointer to the shape object.
 *
 * \return Returns a \ref RwUInt32 value equal to the chunk size (in
 * bytes) of the shape, NULL otherwise.
 *
 * \see Rt2dShapeStreamRead
 * \see Rt2dShapeStreamWrite
 */
RwUInt32
Rt2dShapeStreamGetSize(Rt2dObject *shape)
{
    RwUInt32 size;

    RWAPIFUNCTION(RWSTRING("Rt2dShapeStreamGetSize"));
    RWASSERT(shape);
    RWASSERT(shape->type == rt2DOBJECTTYPESHAPE);

    size = _rt2dShapeStreamGetSize(shape, NULL);

    RWRETURN(size);
}

Rt2dObject *
_rt2dShapeStreamReadTo(Rt2dObject *shape, RwStream *stream, _rt2dSceneResourcePool *pool)
{
    _rt2dShapeRep *rep;
    RpAtomic      *atomic;
    RwFrame       *frame;

    RWFUNCTION(RWSTRING("_rt2dShapeStreamReadTo"));
    RWASSERT(shape);
    RWASSERT(stream);
    RWASSERT(shape->type == rt2DOBJECTTYPESHAPE);

    if (pool)
    {
        RwUInt32 pos;

        if (RwStreamRead(stream, &pos, sizeof(pos)) != sizeof(pos))
        {
            RWRETURN((Rt2dObject *)NULL);
        }

        /* Convert it */
        (void)RwMemNative32(&pos, sizeof(pos));

        rep = pool->shapeReps[pos];
    }
    else
    {
        rep = _rt2dShapeRepStreamRead(stream);
    }

    /* We're already not locked */
    shape->flag &= ~Rt2dObjectIsLocked;

    /* Set up the color cache, since we've created the unlocked form explicitly */
    shape->data.shape.colorCache = ShapeCreateColorCache(rep->geometry);

    _rt2dShapeInit(shape, 0, rep);

    /* Set up the atomic to use the representation's geometry */

    /* Set up the frame */
    frame = RwFrameCreate();
    RwFrameTransform(frame, _rt2dCTMGet(),  rwCOMBINEREPLACE);

    /* Create the atomic */
    atomic = RpAtomicCreate();
    RpAtomicSetFrame(atomic, frame);
    RpAtomicSetGeometry(atomic, rep->geometry, 0);

    shape->data.shape.atomic = atomic;

    RWRETURN(shape);
}

/**
 * \ingroup rt2dshape
 * \ref Rt2dShapeStreamRead is used to read a shape from the specified
 * binary stream. Note that prior to this function call, a binary shape chunk
 * must be found in the stream using the \ref RwStreamFindChunk API function.
 *
 *The sequence to locate and read a shape from a binary stream is
 * as follows:
 * \code
   RwStream   *stream;
   Rt2dObject *newShape;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_2DSHAPE, NULL, NULL) )
       {
           newShape = Rt2dShapeStreamRead(stream);
       }

       RwStreamClose(stream, NULL);
   }
   \endcode
 *
 * \param stream Pointer to the stream
 *
 * \return Returns a pointer to the shape if successful, NULL otherwise.
 *
 * \see Rt2dShapeStreamWrite
 * \see Rt2dShapeStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RwStreamFindChunk
 */
Rt2dObject *
Rt2dShapeStreamRead(RwStream *stream)
{
    Rt2dObject *shape = _rt2dObjectCreate();
    Rt2dObject *result = (Rt2dObject *) NULL;

    RWAPIFUNCTION(RWSTRING("Rt2dShapeStreamRead"));
    RWASSERT(stream);
    RWASSERT(shape);

    _rt2dObjectInit(shape, rt2DOBJECTTYPESHAPE);

    /* Read base class information */
    if (!_rt2dObjectBaseStreamReadTo(shape, stream))
    {
        RWRETURN((Rt2dObject*)NULL);
    }

    result = _rt2dShapeStreamReadTo(shape, stream, NULL);

    if( NULL == result)
    {
        Rt2dShapeDestroy(shape);
    }

    RWRETURN(result);
}


/**
 * \ingroup rt2dshape
 * \ref Rt2dShapeLock
 * locks a shape for editing. A locked shape can not be streamed or rendered
 * and is provided only for editing purposes. When a shape is locked, \ref Rt2dPath
 * and \ref Rt2dBrush can be added or removed.
 *
 * \param shape Pointer to the shape
 *
 * \return Returns a pointer to the shape if successful, NULL otherwise.
 *
 * \see Rt2dShapeAddNode
 * \see Rt2dShapeCreate
 * \see Rt2dShapeUnLock
 */

Rt2dObject *
Rt2dShapeLock(Rt2dObject *shape)
{
    RwFrame *frame;

    RWAPIFUNCTION(RWSTRING("Rt2dShapeLock"));
    RWASSERT(shape);
    RWASSERT(shape->type == rt2DOBJECTTYPESHAPE);

    /* The shape should not be locked twice, so just assert here */
    RWASSERT((shape->flag & Rt2dObjectIsLocked) == 0);

    /* Get rid of the atomic */
    frame = RpAtomicGetFrame(shape->data.shape.atomic);

    RpAtomicSetFrame(shape->data.shape.atomic, NULL);

    RpAtomicDestroy(shape->data.shape.atomic);

    shape->data.shape.atomic = NULL;

    /* Free up the colorCache */
    if( shape->data.shape.colorCache )
    {        
        RwFree(shape->data.shape.colorCache);
        shape->data.shape.colorCache = NULL;
    }

    /* Lock the shape */
    shape->flag |= Rt2dObjectIsLocked;

    RWRETURN(shape);
}

/**
 * \ingroup rt2dshape
 * \ref Rt2dShapeUnlock unlocks a shape. An unlocked shape may be
 * streamed and rendered. The shape is triangulated at this point.
 * Any curves within the shape are flatten for triangulation using
 * the current flattening criteria.
 *
 * \param shape Pointer to the shape
 *
 * \return Returns a pointer to the shape if successful, NULL otherwise.
 *
 * \see Rt2dShapeAddNode
 * \see Rt2dShapeCreate
 * \see Rt2dShapeLock
 */


Rt2dObject *
Rt2dShapeUnlock(Rt2dObject *shape)
{
    Rt2dObject  *result = (Rt2dObject *) NULL;
    RpAtomic *atomic;
    RwFrame *frame;

    RWAPIFUNCTION(RWSTRING("Rt2dShapeUnlock"));
    RWASSERT(shape);
    RWASSERT(shape->type == rt2DOBJECTTYPESHAPE);
    /* The shape should be locked , so assert here */
    RWASSERT((shape->flag & Rt2dObjectIsLocked));

    /* Should not have a geometry or atomic yet */
    RWASSERT(!shape->data.shape.rep->geometry);
    RWASSERT(!shape->data.shape.atomic);

    /* Create the geometry */
    shape->data.shape.rep->geometry =
        ShapeRepCreateGeometry(shape->data.shape.rep);

    /* Set up the atomic to use the representation's geometry */
    /* Set up the frame */
    frame = RwFrameCreate();
    RwFrameTransform(frame, _rt2dCTMGet(),  rwCOMBINEREPLACE);

    /* Create the atomic */
    atomic = RpAtomicCreate();
    RpAtomicSetFrame(atomic, frame);
    RpAtomicSetGeometry(atomic, shape->data.shape.rep->geometry, 0);

    shape->data.shape.atomic = atomic;

    /* Initialise the color cache with material colors */
    shape->data.shape.colorCache =
        ShapeCreateColorCache(shape->data.shape.rep->geometry);

    /* Unlock the shape */
    shape->flag &= ~Rt2dObjectIsLocked;

    RWRETURN(result);
}

#if 0
/**
 * \ingroup rt2dshape
 * \ref Rt2dShapeMorph is used to apply a morphing animation to
 * a shape. The result of this operation will be stored in a result
 * shape. The result shape can be created using \ref Rt2dShapeCreate,
 * \ref Rt2dPathCreateMorphResultPath and \ref Rt2dPathCopy. An alpha value,
 * indicating the degree of morphing between the source and the destination
 * shape should be provided.
 *
 * \li if Alpha = 0, the result path will be equal to the source path
 * \li if Alpha = 1, the result path will be equal to the destination path
 *
 * \param result      Pointer to the result shape.
 * \param source      Pointer to the source shape.
 * \param destination Pointer to the destination shape.
 * \param alpha       A \ref RwReal value indicating the degree of morphing between
 *                    the source and the destination shape.
 *
 * \return Returns pointer to the result shape if successful, NULL otherwise.
 *
 * \see Rt2dPathCreateMorphResultPath
 * \see Rt2dPathCopy
 * \see Rt2dPathMorph
 */
Rt2dObject *
Rt2dShapeMorph(Rt2dObject *result, Rt2dObject *source, Rt2dObject *destination,
                RwReal alpha)
{
    _rt2dShapeNode *nResult, *nSource, *nDestination;
    RwInt32 nodeCount;
    RwInt32     i;

    RWAPIFUNCTION(RWSTRING("Rt2dShapeMorph"));
    RWASSERT(result);
    RWASSERT(source);
    RWASSERT(destination);
    RWASSERT(result->type == rt2DOBJECTTYPESHAPE);
    RWASSERT(source->type == rt2DOBJECTTYPESHAPE);
    RWASSERT(destination->type == rt2DOBJECTTYPESHAPE);

    nResult = (_rt2dShapeNode*)(rwSListGetArray(result->data.shape.rep->nodes));
    nSource = (_rt2dShapeNode*)(rwSListGetArray(source->data.shape.rep->nodes));
    nDestination =
            (_rt2dShapeNode*)(rwSListGetArray(destination->data.shape.rep->nodes));
    nodeCount = rwSListGetNumEntries(result->data.shape.rep->nodes);

    if(0.0f == alpha)
    {
        /* copy path source to result */
        for(i=0;i<nodeCount;i++)
        {
            Rt2dPathCopy(nResult->path, nSource->path);
            nResult++;
            nSource++;
        }
    }
    else if ( 1.0f == alpha)
    {
        /* copy path destination to result */
        for(i=0;i<nodeCount;i++)
        {
            Rt2dPathCopy(nResult->path, nDestination->path);
            nResult++;
            nDestination++;
        }
    }
    else
    {
        /* interpolate to result */
        for(i=0;i<nodeCount;i++)
        {
            Rt2dPathMorph(nResult->path,
                            nSource->path,
                            nDestination->path, alpha);
            nResult++;
            nSource++;
            nDestination++;
        }
    }

    RWRETURN(result);
}
#endif

/****************************************************************************/

#if defined (__MWERKS__)
#if (defined(RWVERBOSE))
#pragma message (__FILE__ "/" _SKY_EXPAND(__LINE__) ": __MWERKS__ == " _SKY_EXPAND(__MWERKS__))
#endif /* (defined (__MWERKS__)) */
#if (__option (global_optimizer))
#pragma always_inline on
#endif /* (__option (global_optimizer)) */
#endif /*  defined (__MWERKS__) */
