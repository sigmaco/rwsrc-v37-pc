/**
 * \ingroup rtgcond
 * \page rtgcondoverview RtGCond Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rtgcond.h
 * \li \b Libraries: rwcore, rpworld, rtgcond, rtwing
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach,
 * \subsection gcondoverview Overview
 *
 * The functions in this toolkit allow geometric (vertex and polygon)
 * and attribute (uvs, prelights, etc.) operations in a simple and
 * independent manner. Some of the functions make heavy use of the
 * winged/half-edge edge data structure (\ref rtwingoverview)
 * for the sake of efficiency.
 */


/****************************************************************************
 Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "rwcore.h"
#include "rpworld.h"
#include "rpdbgerr.h"

#include "rtgcond.h"
#include "rtwing.h"


/****************************************************************************
 Defines
 */
#define RWMIN(a,b)  (((a) < (b)) ? (a) : (b))
#define RWMAX(a,b)  (((a) > (b)) ? (a) : (b))

/****************************************************************************
 Locals
 */
RtGCondGeometryConditioningPipeline conditioningPipeline = NULL;
RtGCondParameters* GCParams;
RtGCondUserdataCallBacks GCUserDataCallBacks = {
    (RtGCondCloneVertexUserdataCallBack) NULL,
    (RtGCondInterpVertexUserdataCallBack) NULL,
    (RtGCondSplitPolygonUserdataCallBack) NULL,
    (RtGCondDestroyVertexUserdataCallBack) NULL,
    (RtGCondDestroyPolygonUserdataCallBack) NULL
};

RwInt32 GCondMaterialDataOffset = 0;

/****************************************************************************
 Static data structures
 */
typedef struct TreePartition TreePartition;
struct TreePartition
{
    /* For vertex welding */
    RwInt32 type;
    RwReal  value;
};

typedef struct VertexTree VertexTree;
struct VertexTree
{
    /* Leaf stored BSP for 'quick' sorting */
    RtGCondVertex** vertexPointerList;
    RwInt32 vplNum;

    VertexTree  *left;
    VertexTree  *right;
};



static void GCondRenormalizeNormals(RtGCondVertex *vertexList, RwInt32 numVertices);

/****************************************************************************
 * Geometry Conditioning Level 0: Tools & Utils
 ***************************************************************************/
/*
 * RtGCondDestroyVertexPolygonReference Internal use only. Destroys the braux fields
 * in the RtGCondPolygon. Should be called after braux fields have been put
 * to use, otherwise it is likely to become quickly invalid.
 */
void
_rtGCondDestroyVertexPolygonReference(RtGCondGeometryList* geometryList)
{
    RwInt32 i;

    RWFUNCTION(RWSTRING("_rtGCondDestroyVertexPolygonReference"));

    /* free temp data structure */
    for (i = 0; i < geometryList->numVertices; i++)
    {
        if (geometryList->vertices[i].numBrauxIndices != 0)
        {
            RWASSERT(geometryList->vertices[i].brauxIndices);

            RwFree(geometryList->vertices[i].brauxIndices);
            geometryList->vertices[i].brauxIndices = NULL;
            geometryList->vertices[i].numBrauxIndices = 0;
        }
    }

    RWRETURNVOID();
}

/*
   RtGCondCreateVertexPolygonReference Internal use only. Calculates the braux fields
   in the RtGCondPolygon. remapper is pointer to optional remapper, NULL otherwise
 */
void
_rtGCondCreateVertexPolygonReference(RtGCondGeometryList* geometryList,
                                    RwInt32* remapper)
{
    RwInt32 i, j, k;
    RtGCondPolygon* poly;
    RwInt32 index;


    RWFUNCTION(RWSTRING("_rtGCondCreateVertexPolygonReference"))

    /* Alloc */
    /* geometryList->vertices->brauxIndices =
    (RwInt32*)RwMalloc(sizeof(RwInt32) * geometryList->numVertices,
                       rwID_GEOMCONDPLUGIN | rwMEMHINTDUR_EVENT);
    */

    /* Init */
    for (j = 0; j < geometryList->numVertices; j++)
    {
        geometryList->vertices[j].numBrauxIndices = 0;
        geometryList->vertices[j].brauxIndices = NULL;
    }

    /* Calculate space required */
    for (i = 0; i < geometryList->numPolygons; i++)
    {
        poly = &geometryList->polygons[i];
        for (j = 0; j < poly->numIndices; j++)
        {
            if (!remapper)
            {
                geometryList->vertices[poly->indices[j]].numBrauxIndices++;
            }
            else
            {
                geometryList->vertices[remapper[poly->indices[j]]].numBrauxIndices++;
            }
        }
    }

    /* Create space */
    for (j = 0; j < geometryList->numVertices; j++)
    {
        if (geometryList->vertices[j].numBrauxIndices > 0)
        {
            geometryList->vertices[j].brauxIndices =
                (RwInt32*)RwMalloc(sizeof(RwInt32) * geometryList->vertices[j].numBrauxIndices,
                                   rwID_GEOMCONDPLUGIN | rwMEMHINTDUR_EVENT);
            for (k = 0; k < geometryList->vertices[j].numBrauxIndices; k++)
            {
                /* initialize */
                geometryList->vertices[j].brauxIndices[k] = -1;
            }
        }
    }

    /* Reinit */
    for (j = 0; j < geometryList->numVertices; j++)
    {
        geometryList->vertices[j].numBrauxIndices = 0;
    }

    /* Assign */
    for (i = 0; i < geometryList->numPolygons; i++)
    {
        poly = &geometryList->polygons[i];
        for (j = 0; j < poly->numIndices; j++)
        {
            if (!remapper)
            {
                index = geometryList->vertices[poly->indices[j]].numBrauxIndices;
                geometryList->vertices[poly->indices[j]].brauxIndices[index] = i;
                geometryList->vertices[poly->indices[j]].numBrauxIndices++;
            }
            else
            {
                index = geometryList->vertices[remapper[poly->indices[j]]].numBrauxIndices;
                geometryList->vertices[remapper[poly->indices[j]]].brauxIndices[index] = i;
                geometryList->vertices[remapper[poly->indices[j]]].numBrauxIndices++;
            }
        }
    }
    RWRETURNVOID();
}

static RwReal
GCondACos(RwReal cs)
{
    RwReal ac;

    RWFUNCTION(RWSTRING("GCondACos"));

    if (cs >= 0.999995f)
    {
        ac = 0.0f;
    }
    else if (cs <= -0.999995f)
    {
        ac = 180.0f;
    }
    else
    {
        ac = (RwReal)RwACos(cs);
        ac *= 180.0f/3.14159265535f;
    }

    RWRETURN(ac);
}

/**
 * \ingroup rtgcond
 * \ref RtGCondNormalize normalizes a given vector to high precision
 * (using the sqrt function). It also has useful asserts.
 *
 * \param vec    A pointer to a vector requiring normalizing
 *
 * \return If the vector is close to being zero-length vector it doesn't
 *  do anything and returns FALSE, TRUE otherwise.
 *
 * \see RtGCondColinearVertices
 */
RwBool
RtGCondNormalize(RwV3d* vec)
{
    RwReal scalar;
    RwReal lt;
    RwReal length = RwV3dDotProductMacro(vec, vec);

    RWAPIFUNCTION(RWSTRING("RtGCondNormalize"));

#if (defined(RW_USE_SPF))
    length = (RwReal)sqrtf(length);
#else  /* (defined(RW_USE_SPF)) */
    length = (RwReal)sqrt(length);
#endif /* (defined(RW_USE_SPF)) */

    if (length >= GCONDR1TOL)
    {
        scalar = 1.0f / length;
        RwV3dScale(vec, (const RwV3d*)vec, scalar);

        lt = vec->x * vec->x + vec->y * vec->y + vec->z * vec->z;
        RWASSERT(lt > 0.99f && lt < 1.01f);
    }
    else
    {
        /* Cannot normalize a zero-length vector */
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}

static RwBool
GCondEqualZero(RwReal val, RwReal tol)
{
    RWFUNCTION(RWSTRING("GCondEqualZero"));

    RWRETURN((val <= tol) && (val >= -tol));
}

static RwBool
GCondPositionEqual(RwV3d* a, RwV3d* b)
{
    RWFUNCTION(RWSTRING("GCondPositionEqual"));

    RWRETURN(GCondEqualZero(a->x - b->x, GCONDZERO) && GCondEqualZero(a->y - b->y, GCONDZERO) && GCondEqualZero(a->z - b->z, GCONDZERO));
}

/**
 * \ingroup rtgcond
 * \ref RtGCondLength returns the length of given vector to high precision
 * (using the sqrt function), succeeds even if the vector is zero length.
 *
 * \param vec    A pointer to a vector
 *
 * \return The length of the vector
 */
RwReal
RtGCondLength(RwV3d* vec)
{
    RwReal length;

    RWAPIFUNCTION(RWSTRING("RtGCondLength"));

    length = RwV3dDotProductMacro(vec, vec);

#if (defined(RW_USE_SPF))
    length = (RwReal)(sqrtf(length));
#else /* (defined(RW_USE_SPF)) */
    length = (RwReal)(sqrt(length));
#endif /* (defined(RW_USE_SPF)) */

    if (length < GCONDR1TOL)
    {
        RWRETURN(0.0f);
    }

    RWRETURN(length);
}

/**
 * \ingroup rtgcond
 * \ref RtGCondVectorsEqual compares two vectors to see if they match orthogonally within
 * the tolerance.
 *
 * \param v1 pointer to first vector
 * \param v2 pointer to second vector
 * \param tol tolerance
 *
 * \return Returns TRUE if matching, FALSE otherwise.
 */
RwBool
RtGCondVectorsEqual(RwV3d* v1, RwV3d* v2, RwReal tol)
{
    RWAPIFUNCTION(RWSTRING("RtGCondVectorsEqual"));

    if (GCondEqualZero(v1->x - v2->x, tol) &&
        GCondEqualZero(v1->y - v2->y, tol) &&
        GCondEqualZero(v1->z - v2->z, tol))
    {
        RWRETURN(TRUE);
    }
    RWRETURN(FALSE);
}

/**
 * \ingroup rtgcond
 * \ref RtGCondColinearVertices calculates if the given three vertices are colinear.
 *
 * \param v1 pointer to first vertex
 * \param v2 pointer to second vertex
 * \param v3 pointer to third vertex
 * \param strictOrder TRUE if a---b---c order assumed, FALSE otherwise (e.g. a---c---b)
 * \param tol positional tolerance
 *
 * \return Returns TRUE if colinear, FALSE otherwise.
 * 
 * \see RtGCondColinearVerticesTol
 */
RwBool
RtGCondColinearVertices(RwV3d* v1, RwV3d* v2, RwV3d* v3, RwBool strictOrder, RwReal tol)
{
    RwV3d e1, e2, n;

    RWAPIFUNCTION(RWSTRING("RtGCondColinearVertices"));

    if (RtGCondVectorsEqual(v1, v2, GCONDR1TOL) || RtGCondVectorsEqual(v2, v3, GCONDR1TOL) || RtGCondVectorsEqual(v1, v3, GCONDR1TOL))
    {
        RWRETURN(FALSE);
    }

    RwV3dSub(&e1, v2, v1);
    RwV3dSub(&e2, v3, v2);
    RtGCondNormalize(&e1);
    RtGCondNormalize(&e2);
    RwV3dCrossProduct(&n, &e1, &e2);

    if (tol < GCONDZERO)
    {
        tol = GCONDZERO;
    }    
    
    if (RtGCondLength(&n) <= tol)
    {
        if (RwV3dDotProduct(&e1, &e2) > 0)
        {
            RWRETURN(TRUE); /* a----b----c */
        }
        else
        {
            RWRETURN(!strictOrder); /* a----c----b */
        }
    }
    RWRETURN(FALSE);
}

static RwReal
GCondSnap(RwReal x, RwReal weldThreshold, RwBool round)
{
    RWFUNCTION(RWSTRING("GCondSnap"));

    if (weldThreshold == 0.0f) RWRETURN(x);
    if (round == FALSE)
    {
        RWRETURN(x - (RwReal)RwFmod(x, weldThreshold));
    }
    else
    {
        RWRETURN(x - (RwReal)RwFmod(x, weldThreshold) + weldThreshold / 2.0f);
    }
}

/****************************************************************************
 * Geometry Conditioning Level 1: Fixers
 ***************************************************************************/
/**
 * \ingroup rtgcond
 * \ref RtGCondSnapPipelineNode snaps the given vertices to a grid.
 *
 * \note This should only be used if the side effects are
 * appreciated - such as a voxelized geometry appearance if the tolerance
 * too high.
 *
 * \ref RtGCondWeldVerticesPipelineNode should be used for welding near-by neighbors
 *
 * \param geometryList a pointer to the list of geometry to be snapped
 * \param weldThreshold the effective grid resolution
 *
 * \see RtGCondWeldVerticesPipelineNode
 * \see RtGCondSnapUVsPipelineNode
 */
void
RtGCondSnapPipelineNode(RtGCondGeometryList *geometryList, RwReal weldThreshold)
{
    RwInt32 i;
    RtGCondVertex* tempVertices = geometryList->vertices;

    RWAPIFUNCTION(RWSTRING("RtGCondSnapPipelineNode"));

    i = 0;
    while (i < geometryList->numVertices)
    {
        tempVertices[i].position.x -= (RwReal)RwFmod(tempVertices[i].position.x, weldThreshold);
        tempVertices[i].position.y -= (RwReal)RwFmod(tempVertices[i].position.y, weldThreshold);
        tempVertices[i].position.z -= (RwReal)RwFmod(tempVertices[i].position.z, weldThreshold);
        i++;
    }

    RWRETURNVOID();
}

/**
 * \ingroup rtgcond
 * \ref RtGCondSnapUVsPipelineNode snaps the given vertex uv's to a grid.
 * \ref RtGCondWeldVerticesPipelineNode should be used for welding near-by
 * vertices with similar uvs.
 *
 * \param geometryList a pointer to the list of geometry to be snapped
 * \param weldThreshold the effective grid resolution
 *
 * \see RtGCondWeldVerticesPipelineNode
 * \see RtGCondSnapPipelineNode
 */
void
RtGCondSnapUVsPipelineNode(RtGCondGeometryList *geometryList, RwReal weldThreshold)
{
    RwInt32 i, j;
    RtGCondVertex* tempVertices = geometryList->vertices;

    RWAPIFUNCTION(RWSTRING("RtGCondSnapUVsPipelineNode"));

    i = 0;
    while (i < geometryList->numVertices)
    {
        for (j = 0; j < rwMAXTEXTURECOORDS; j++)
        {
            tempVertices[i].texCoords[j].u -= (RwReal)RwFmod(tempVertices[i].texCoords[j].u, weldThreshold);
            tempVertices[i].texCoords[j].v -= (RwReal)RwFmod(tempVertices[i].texCoords[j].v, weldThreshold);
        }
        i++;
    }

    RWRETURNVOID();
}

static void
GCondRenormalizeNormals(RtGCondVertex *vertexList, RwInt32 numVertices)
{
    /* O(): Strictly linear on vertex number */
    RwInt32 i;
    RtGCondVertex* temp;

    RWFUNCTION(RWSTRING("GCondRenormalizeNormals"));

    temp = vertexList;
    for (i = 0; i < numVertices; i++)
    {
        RtGCondNormalize(&temp->normal);

        temp++;
    }

    RWRETURNVOID();
}

static RwReal
GCondAreaOfTriangle(const RwV3d * const c0, const RwV3d * const c1,
                      const RwV3d * const c2)
{
    RwReal             area2;
    RwV3d              e0, e1;
    RwV3d              n;

    RWFUNCTION(RWSTRING("GCondAreaOfTriangle"));

    RwV3dSubMacro(&e0, c1, c2);
    RwV3dSubMacro(&e1, c2, c0);

    RwV3dCrossProductMacro(&n, &e0, &e1);
    area2 = RtGCondLength(&n) / 2.0f;

    RWRETURN(area2);
}

#if(defined(GCONDCHECK))
static RwBool
GCondZeroAreaTriangle(const RwV3d * const c0, const RwV3d * const c1,
                      const RwV3d * const c2, RwReal areaThreshold)
{
    RWFUNCTION(RWSTRING("GCondZeroAreaTriangle"));

    RWRETURN(GCondAreaOfTriangle(c0, c1, c2) <= areaThreshold);
}
#endif /*(defined(GCONDCHECK))*/

/**
 * \ingroup rtgcond
 * \ref RtGCondAreaOfPolygon finds the area of a polygon
 *
 * \param geometryList a pointer to the list of geometry containing the polygons
 * \param polygon a pointer to the polygon
 *
 * \return area of the polygon
 *
 * \see RtGCondCullZeroAreaPolygonsPipelineNode
 */
RwReal
RtGCondAreaOfPolygon(RtGCondGeometryList* geometryList, RtGCondPolygon* polygon)
{
    RwReal             area2 = 0.0f;
    RwInt32 j;
    RwV3d              *v0;
    RwV3d              *v1;
    RwV3d              *v2;

    RWAPIFUNCTION(RWSTRING("RtGCondAreaOfPolygon"));

    for (j = 2; j < polygon->numIndices; j++)
    {
        v0 = &geometryList->vertices[polygon->indices[0]].position;
        v1 = &geometryList->vertices[polygon->indices[j-1]].position;
        v2 = &geometryList->vertices[polygon->indices[j]].position;

        area2 += GCondAreaOfTriangle(v0, v1, v2);
    }

    RWRETURN(area2);
}

static RwBool
GCondZeroAreaPolygon(RtGCondGeometryList* geometryList, RtGCondPolygon* tempPolygons,
                      RwReal areaThreshold)
{
    RWFUNCTION(RWSTRING("GCondZeroAreaPolygon"));

    RWRETURN(RtGCondAreaOfPolygon(geometryList, tempPolygons) <= areaThreshold);
}

/**
 * \ingroup rtgcond
 * \ref RtGCondCullZeroAreaPolygonsPipelineNode removes all polygons whose
 * area is less than the tolerance (then it remaps).
 *
 * \note This should be treated
 * with respect, since applying the filter to a scene stand-alone
 * could lead to holes appearing in meshes where slivers once stood
 * and if too high, polygons around the tops of fine
 * spheres and cones, for example, can go missing. In the former case
 * weld vertices first, in the latter, make sure the threshold is
 * significantly small, preferably zero.
 *
 * \param geometryList a pointer to the list of geometry containing the polygons
 * \param areaThreshold the area below which polygons are culled
 *  
 * \see RtGCondWeldVerticesPipelineNode
 * \see RtGCondRemapVerticesPipelineNode
 * \see RtGCondAreaOfPolygon
 * \see RtGCondRemoveSliversPipelineNode
 */
void
RtGCondCullZeroAreaPolygonsPipelineNode(RtGCondGeometryList *geometryList, RwReal areaThreshold)
{
    /* Status: Rewritten... */

    /* Efficiency: avg: O(n); best: O(n); worst: O(n^2);
     * - where n is number of polygons
     * - worst case assumes memcpy of n elements is O(n)
     */
    RwInt32 bytes;
    RwInt32 i;
    RwBool nonZeroAreaTriangle;

    RWAPIFUNCTION(RWSTRING("RtGCondCullZeroAreaPolygonsPipelineNode"));
    RWASSERT(NULL != geometryList);

    /* Scan polygons looking for zero areas... */
    i = 0;
    while (i < geometryList->numPolygons)
    {
        nonZeroAreaTriangle = !(GCondZeroAreaPolygon(geometryList, &geometryList->polygons[i], areaThreshold));

        if (!nonZeroAreaTriangle)
        {
            if (GCUserDataCallBacks.destroyPolygonUserdata)
            {
                GCUserDataCallBacks.destroyPolygonUserdata(&(geometryList->polygons[i].pUserData));
            }

            /* Free the polygon indices before anything else... */
            RtGCondFreeIndices(&(geometryList->polygons[i]));

            /* Shift all elements after zero area 'left' one... */
            bytes = (geometryList->numPolygons - i - 1) * sizeof(RtGCondPolygon);
            memcpy(&geometryList->polygons[i], &geometryList->polygons[i + 1], bytes);

            geometryList->numPolygons--;
        }
        else
        {
            i++;
        }
    }

    /* Reallocate memory to allow for potentially reduced list size... */
    geometryList->polygons = (RtGCondPolygon *)RwRealloc(geometryList->polygons,
        (geometryList->numPolygons) * sizeof(RtGCondPolygon),
        rwID_GEOMCONDPLUGIN | rwMEMHINTDUR_EVENT | rwMEMHINTFLAG_RESIZABLE);

    /* Cull unused vertices - invalid geometry if there are unused ones left */
    RtGCondRemapVerticesPipelineNode(geometryList);

    RWRETURNVOID();
}

static void
GCondCullTaggedPolygons(RtGCondGeometryList *geometryList)
{
    RwInt32 bytes;
    RwInt32 i;

    RWFUNCTION(RWSTRING("GCondCullTaggedPolygons"));
    RWASSERT(NULL != geometryList);

    /* Scan polygons... */
    i = 0;
    while (i < geometryList->numPolygons)
    {


        if (geometryList->polygons[i].id == -1)
        {
            if (GCUserDataCallBacks.destroyPolygonUserdata)
            {
                GCUserDataCallBacks.destroyPolygonUserdata(&(geometryList->polygons[i].pUserData));
            }

            /* Free the polygon indices before anything else... */
            RtGCondFreeIndices(&(geometryList->polygons[i]));

            /* Shift all elements after zero area 'left' one... */
            bytes = (geometryList->numPolygons - i - 1) * sizeof(RtGCondPolygon);
            memcpy(&geometryList->polygons[i], &geometryList->polygons[i + 1], bytes);

            geometryList->numPolygons--;
        }
        else
        {
            i++;
        }
    }

    /* Reallocate memory to allow for potentially reduced list size... */
    geometryList->polygons = (RtGCondPolygon *)RwRealloc(geometryList->polygons,
        (geometryList->numPolygons) * sizeof(RtGCondPolygon),
        rwID_GEOMCONDPLUGIN | rwMEMHINTDUR_EVENT | rwMEMHINTFLAG_RESIZABLE);

    /* Cull unused vertices - invalid geometry if there are unused ones left */
    RtGCondRemapVerticesPipelineNode(geometryList);

    RWRETURNVOID();
}

/****************************************************************************/
/**
 * \ingroup rtgcond
 * \ref RtGCondLimitUVsPipelineNode shifts uvs values into the interval
 * between the given minimum and maximum values. It tries to preserve
 * continuity if possible. There are two ways UV limiting can be done. The
 * first is using the standard UV limits which are texture independent, and
 * the second is to specify the limits in texels, in which case UV limits
 * are calculated by: limitUMin = minUvTexelLimit / textureWidth etc.
 * Depending on the texelLimits parameter we'll use the limitUVMin and
 * limitUVMax limits passed by the parameters or use \ref
 * RtGCondMaterialGetUVLimits function to get already pre-calculated per
 * material per UV set texel based UV limits.
 *
 * \note If the uv range of any polygon is greater than the
 * interval, the filter will obviously fail.
 *
 * \param geometryList a pointer to the list of geometry
 * \param limitUVMin minimum allowable uv value
 * \param limitUVMax maximum allowable uv value
 * \param texelLimits maximum allowable uv value
 *
 * \see RtGCondMaterialSetUVLimits
 * \see RtGCondLimitUVsPipelineNode
 * \see RtGCondFixAndFilterGeometryPipeline
 */
void
RtGCondLimitUVsPipelineNode(RtGCondGeometryList * geometryList,
                            RwReal limitUVMin, RwReal limitUVMax, RwBool texelLimits)
{
    /* Efficiency: O(n); where n is the number of polygons
     */

    RwInt32 polyIndex;
    RwInt32 rediculousMax = 2 << 15;
    RwInt32 rediculousMin = -rediculousMax;

    RWAPIFUNCTION(RWSTRING("RtGCondLimitUVsPipelineNode"));

    if (limitUVMax - limitUVMin < 1.0f)
    {
        RWASSERT(limitUVMax - limitUVMin >= 1.0f);
        /* No UV limiting done because the given values cannot be honoured */
    }
    else
    {
        /* For all polygons */
        for (polyIndex = 0; polyIndex < geometryList->numPolygons; polyIndex++)
        {
            RwInt32     i;
            RwInt32     v;
            RwReal      limitUVOffset[2];
            RwReal      uMin[rwMAXTEXTURECOORDS];
            RwReal      uMax[rwMAXTEXTURECOORDS];
            RwReal      vMin[rwMAXTEXTURECOORDS];
            RwReal      vMax[rwMAXTEXTURECOORDS];
            RwInt32     numUVs = 0;
            RpMaterial  *mat;

            /* Get the material for this polygon */
            mat = rpMaterialListGetMaterial(&geometryList->matList,
                geometryList->polygons[polyIndex].matIndex);

            for (i = 0; i < rwMAXTEXTURECOORDS; i++)
            {
                uMin[i] = RwRealMAXVAL;
                uMax[i] = RwRealMINVAL;
                vMin[i] = RwRealMAXVAL;
                vMax[i] = RwRealMINVAL;
            }

            /* Calculate min & max u & v... For all verts */
            for (v = 0; v < geometryList->polygons[polyIndex].numIndices; v++)
            {
                RtGCondVertex* vertex = &geometryList->vertices[geometryList->polygons[polyIndex].indices[v]];
                RwInt32 j;

                numUVs = geometryList->numUVs;
                for (j = 0; j < numUVs; j++)
                {
                    RwTexCoords texCoords = vertex->texCoords[j];
                    if (uMin[j] > texCoords.u)
                    {
                        uMin[j] = texCoords.u;
                    }
                    if (uMax[j] < texCoords.u)
                    {
                        uMax[j] = texCoords.u;
                    }
                    if (vMin[j] > texCoords.v)
                    {
                        vMin[j] = texCoords.v;
                    }
                    if (vMax[j] < texCoords.v)
                    {
                        vMax[j] = texCoords.v;
                    }
                }
            }

            /* Iterate on all uvs, applying the limit filter */
            for (i = 0; i < numUVs; i++)
            {
                RwInt32 uv, uvRange[2];
                RwReal  min[2], max[2];
                RwReal  minLimits[2], maxLimits[2];

                /* Get the limits. If we are doing texel limiting, get the limits
                 * from the materials, if just doing the UV limiting then obey the
                 * limits passed by parameters. */
                if (texelLimits)
                {
                    RtGCondMaterialGetUVLimits(mat, i, &minLimits[0], &maxLimits[0],
                                                       &minLimits[1], &maxLimits[1]);
                }
                else
                {
                    minLimits[0] = minLimits[1] = limitUVMin;
                    maxLimits[0] = maxLimits[1] = limitUVMax;
                }

                min[0]              = uMin[i];
                min[1]              = vMin[i];
                max[0]              = uMax[i];
                max[1]              = vMax[i];
                limitUVOffset[0]    = 0.0f;
                limitUVOffset[1]    = 0.0f;
                uvRange[0]          = RwInt32FromRealMacro(maxLimits[0] - minLimits[0]);
                uvRange[1]          = RwInt32FromRealMacro(maxLimits[1] - minLimits[1]);

                for (uv = 0; uv < 2; uv++)
                {
                    /* first shift triangles outside the range by multiples of the range */
                    if (min[uv] > maxLimits[uv] && min[uv] < rediculousMax)
                    {
                        do
                        {
                            min[uv] -= uvRange[uv];
                            max[uv] -= uvRange[uv];
                            limitUVOffset[uv] -= uvRange[uv];
                        }while (min[uv] > maxLimits[uv]);
                    }
                    if (max[uv] < minLimits[uv] && max[uv] > rediculousMin)
                    {
                        do
                        {
                            min[uv] += uvRange[uv];
                            max[uv] += uvRange[uv];
                            limitUVOffset[uv] += uvRange[uv];
                        }while (max[uv] < minLimits[uv]);
                    }
                    /* now shift any triangles still stradling the borders by 1 unit at a time */
                    if (max[uv] > maxLimits[uv] && max[uv] < rediculousMax)
                    {
                        do
                        {
                            min[uv] -= 1.0f;
                            max[uv] -= 1.0f;
                            limitUVOffset[uv] -= 1.0f;
                        }while (max[uv] > maxLimits[uv]);
                    }
                    if (min[uv] < minLimits[uv] && min[uv] > rediculousMin)
                    {
                        do
                        {
                            min[uv] += 1.0f;
                            max[uv] += 1.0f;
                            limitUVOffset[uv] += 1.0f;
                        }while (min[uv] < minLimits[uv]);
                    }
                }
                for (v = 0; v < geometryList->polygons[polyIndex].numIndices; v++)
                {
                    RtGCondVertex* vertex = &geometryList->vertices[geometryList->polygons[polyIndex].indices[v]];

                    vertex->texCoords[i].u += limitUVOffset[0];
                    vertex->texCoords[i].v += limitUVOffset[1];
                }
            }
        }
    }
    RWRETURNVOID();
}

/**
 * \ingroup rtgcond
 * \ref RtGCondRemapVerticesPipelineNode cross-references polygon indices with vertices,
 * if there are any vertices which are not references they are removed, and
 * the entire lists is shifted before the polygon indices are remapped.
 *
 * \param geometryList a pointer to the list of geometry
 *
 * \see RtGCondCullZeroAreaPolygonsPipelineNode
 * \see RtGCondRemoveSliversPipelineNode
 */
void
RtGCondRemapVerticesPipelineNode(RtGCondGeometryList * geometryList)
{
    /* The 'remapper' */
    /* Efficiency: avg: O(n); best: O(n); worst: O(n);
     * - where n is number of polygons
     */
    RwInt32* remapper;
    RwInt32 checkSum = 0, newIndex, i, j;
    RtGCondPolygon* p;

    RWAPIFUNCTION(RWSTRING("RtGCondRemapVerticesPipelineNode"));
    RWASSERT(NULL != geometryList);

    /* initialize remapper */
    remapper = (RwInt32 *) RwMalloc(sizeof(RwInt32) * geometryList->numVertices,
             rwID_GEOMCONDPLUGIN | rwMEMHINTDUR_FUNCTION);
    memset(remapper, 0, sizeof(RwInt32) * geometryList->numVertices);

    /* Maintain array, 1 if index used, 0 else */
    /* When switching a 0 to a 1 increment counter */
    p = geometryList->polygons;
    for (i = 0; i < geometryList->numPolygons; i++)
    {
        for (j = 0; j < p->numIndices; j++)
        {
            if (remapper[p->indices[j]] == 0)
            {
                remapper[p->indices[j]] = 1;
                checkSum++;
            }
        }
        p++;
    }

    /* Do we have unused vertices? */
    if (checkSum != geometryList->numVertices)
    {
        RtGCondVertex *newVertices, *tempNewVertices;
        RtGCondVertex *tempOldVertices;

        /* Here, we need to squish vertex array and calculate remapping information! */
        newVertices = (RtGCondVertex *) RwMalloc(sizeof(RtGCondVertex) * checkSum,
                 rwID_GEOMCONDPLUGIN | rwMEMHINTDUR_EVENT | rwMEMHINTFLAG_RESIZABLE);

        tempNewVertices = newVertices;
        tempOldVertices = geometryList->vertices;
        newIndex = 0;
        for (i = 0; i < geometryList->numVertices; i++)
        {
            if (remapper[i] == 1)
            {
                /* Copy all vertex attributes from old position to new */
                *tempNewVertices = *tempOldVertices;

                /* Vertex remapping */
                tempNewVertices->index = newIndex;
                remapper[i] = newIndex;

                newIndex++;
                tempNewVertices++;
            }
            else if (GCUserDataCallBacks.destroyVertexUserdata)
            {
                GCUserDataCallBacks.destroyVertexUserdata(&tempOldVertices->pUserData);
            }
            tempOldVertices++;
        }

        RWASSERT(newIndex == checkSum);

        /* Polygon index remapping */
        p = geometryList->polygons;
        for (i = 0; i < geometryList->numPolygons; i++)
        {
            for (j = 0; j < p->numIndices; j++)
            {
                p->indices[j] = remapper[p->indices[j]];
            }
            p++;
        }


        RwFree(geometryList->vertices);
        geometryList->vertices = NULL;

        geometryList->vertices = newVertices;
        geometryList->numVertices = checkSum;
    }
    RwFree(remapper);
    remapper = NULL;

    RWRETURNVOID();
}

/**
 * \ingroup rtgcond
 * \ref RtGCondSortVerticesOnMaterialPipelineNode makes sure the vertex list is ordered by
 * material.
 *
 * \param geometryList a pointer to the list of geometry
 */
void
RtGCondSortVerticesOnMaterialPipelineNode(RtGCondGeometryList * geometryList)
{
    /* Efficiency: avg: O(n); best: O(n); worst: O(n);
     * - where n is number of polygons
     */
    RwInt32* remapper;
    RwInt32 newIndex, i, j;
    RtGCondPolygon* p;
    RwInt32 numMaterials = 0;

    RWAPIFUNCTION(RWSTRING("RtGCondSortVerticesOnMaterialPipelineNode"));
    RWASSERT(NULL != geometryList);

    for (j = 0; j < geometryList->numVertices; j++)
    {
        if (geometryList->vertices[j].matIndex > numMaterials - 1)
        {
            numMaterials = geometryList->vertices[j].matIndex + 1;
        }
    }

    /* initialize remapper */
    remapper = (RwInt32 *) RwMalloc(sizeof(RwInt32) * geometryList->numVertices,
            rwID_GEOMCONDPLUGIN | rwMEMHINTDUR_FUNCTION);
    memset(remapper, 0, sizeof(RwInt32) * geometryList->numVertices);

    {
        RtGCondVertex *newVertices, *tempNewVertices;
        RtGCondVertex *tempOldVertices;
        RwInt32 vertIndex = 0;

        /* Here, we need to squish vertex array and calculate remapping information! */
        newVertices = (RtGCondVertex *) RwMalloc(sizeof(RtGCondVertex) * geometryList->numVertices,
                               rwID_GEOMCONDPLUGIN | rwMEMHINTDUR_EVENT | rwMEMHINTFLAG_RESIZABLE);

        tempNewVertices = newVertices;
        tempOldVertices = geometryList->vertices;
        newIndex = 0;


        for (i = 0; i < numMaterials; i++)
        {
            for (j = 0; j < geometryList->numVertices; j++)
            {
                if (geometryList->vertices[j].matIndex == i)
                {
                    newVertices[vertIndex] = geometryList->vertices[j];
                    remapper[j] = vertIndex;
                    vertIndex++;
                }
            }
        }

        /* Polygon index remapping */
        p = geometryList->polygons;
        for (i = 0; i < geometryList->numPolygons; i++)
        {
            for (j = 0; j < p->numIndices; j++)
            {
                p->indices[j] = remapper[p->indices[j]];
            }
            p++;
        }


        RwFree(geometryList->vertices);
        geometryList->vertices = newVertices;
    }
    RwFree(remapper);
    remapper = NULL;

    RWRETURNVOID();
}

/*
 * RtGCondCopyVertexInfo copies the data from one vertex to another
 * and assigns it a given index.
 */
void
_rtGCondCopyVertexInfo(RtGCondVertex* from, RtGCondVertex* to, RwInt32 index)
{
    RwInt32 tx;

    RWFUNCTION(RWSTRING("_rtGCondCopyVertexInfo"));

    if (GCUserDataCallBacks.cloneVertexUserdata)
    {
        GCUserDataCallBacks.cloneVertexUserdata(&to->pUserData, &from->pUserData);
    }
    to->position = from->position;
    to->normal = from->normal;
    to->preLitCol = from->preLitCol;
    to->index = index;

    for (tx = 0; tx < rwMAXTEXTURECOORDS; tx++)
    {
        to->texCoords[tx] = from->texCoords[tx];
    }

    to->matIndex = from->matIndex;
    to->pUserData = from->pUserData;

    RWRETURNVOID();
}

/**
 * \ingroup rtgcond
 * \ref RtGCondUnshareVerticesOnMaterialBoundariesPipelineNode ensures that vertices
 * are not shared between two polygons which have different materials
 *
 * \param geometryList a pointer to the list of geometry
 *
 * \see RtGCondUnshareVerticesPipelineNode
 */
void
RtGCondUnshareVerticesOnMaterialBoundariesPipelineNode(RtGCondGeometryList
                                          * geometryList)
{
    /* Ensure that vertices aren't shared between two polygons
     * which have different materials
     */
    /* Efficiency: avg: O(n); best: O(n); worst: O(n^2);
     * - where n is number of polygons
     */
    RwInt32             PreNumVertices;
    RwInt32             newVertexCount = 0, newIndex = 0, currentIndex = 0;
    RwInt32             i,j,k,l;
    RtGCondPolygon      *p, *pS;
    RtGCondVertex       *v;

    RWAPIFUNCTION(RWSTRING("RtGCondUnshareVerticesOnMaterialBoundariesPipelineNode"));

    PreNumVertices = geometryList->numVertices;

    /* Get potential new vertex count, and set indices if unset */
    p = geometryList->polygons;
    for (i = 0; i < geometryList->numPolygons; i++)
    {
        for (j = 0; j < p->numIndices; j++)
        {
            if (geometryList->vertices[p->indices[j]].matIndex == -1)
            {
                geometryList->vertices[p->indices[j]].matIndex = p->matIndex;
            }
            else if (geometryList->vertices[p->indices[j]].matIndex != p->matIndex)
            {
                newVertexCount++;
                /* introduce new vertex */
            }
        }
        p++;
    }

    geometryList->vertices = (RtGCondVertex*)RwRealloc
                                (geometryList->vertices,
                                sizeof(RtGCondVertex) * (newVertexCount + PreNumVertices),
                                rwID_GEOMCONDPLUGIN | rwMEMHINTDUR_EVENT |
                                rwMEMHINTFLAG_RESIZABLE);
    newVertexCount = 0;
    p = geometryList->polygons;
    v = &geometryList->vertices[PreNumVertices];
    newIndex = 0;
    for (i = 0; i < geometryList->numPolygons; i++)
    {
        for (j = 0; j < p->numIndices; j++)
        {
            currentIndex = p->indices[j];
            if (geometryList->vertices[currentIndex].matIndex != p->matIndex)
            {
                RwInt32 tx;
                /* The vertex does not point to the right material,
                 * introduce new vertex
                 */
                if (GCUserDataCallBacks.cloneVertexUserdata)
                {
                    GCUserDataCallBacks.cloneVertexUserdata(&v->pUserData, &geometryList->vertices[currentIndex].pUserData);
                }
                v->position = geometryList->vertices[currentIndex].position;
                v->normal = geometryList->vertices[currentIndex].normal;
                v->preLitCol = geometryList->vertices[currentIndex].preLitCol;
                v->index = geometryList->numVertices + newIndex;

                for (tx = 0; tx < geometryList->numUVs; tx++)
                {
                    v->texCoords[tx] = geometryList->vertices[currentIndex].texCoords[tx];
                }

                v->matIndex = geometryList->vertices[currentIndex].matIndex;
                v->pUserData = geometryList->vertices[currentIndex].pUserData;

                p->indices[j] = geometryList->numVertices + newIndex++;
                v++;
                newVertexCount++;

                /* Need to scan for others that exhibit this quality! */
                pS = p;
                /* vS = geometryList->vertices[PreNumVertices]; */
                for (k = i; k < geometryList->numPolygons; k++)
                {
                    for (l = 0; l < pS->numIndices; l++)
                    {
                        /* If another vertex, h, shares the old index, o,
                         * and h's matindex differs from its polygon
                         * see if the new vertex, n, is okay and reassign...
                         */
                        if ((pS->indices[l] == currentIndex) &&
                           (geometryList->vertices[pS->indices[l]].matIndex != pS->matIndex) &&
                           (geometryList->vertices[pS->indices[l]].matIndex == p->matIndex))
                        {
                            pS->indices[l] = p->indices[j];
                            /* introduce new vertex */
                        }
                    }
                    pS++;
                }

            }
        }
        p++;
    }

    /* Need final realloc */
    geometryList->numVertices = newVertexCount + PreNumVertices;
    geometryList->vertices = (RtGCondVertex*)RwRealloc
                                (geometryList->vertices,
                                sizeof(RtGCondVertex) * geometryList->numVertices,
                                rwID_GEOMCONDPLUGIN | rwMEMHINTDUR_EVENT |
                                rwMEMHINTFLAG_RESIZABLE);

    RWRETURNVOID();
}

/**
 * \ingroup rtgcond
 * \ref RtGCondUnshareVerticesPipelineNode ensures that no vertices
 * are not shared between multiple polygons
 *
 * \param geometryList a pointer to the list of geometry
 * 
 * \return TRUE if successful, FALSE otherwise
 *
 * \see RtGCondUnshareVerticesOnMaterialBoundariesPipelineNode
 */
RwBool
RtGCondUnshareVerticesPipelineNode(RtGCondGeometryList *geometryList)
{
    RwInt32             newVertexCount = 0, newIndex = 0, currentIndex = 0;
    RwInt32             i,j;
    RtGCondPolygon      *p;
    RtGCondVertex       *v, *unsharedVertices;

    RWAPIFUNCTION(RWSTRING("RtGCondUnshareVerticesPipelineNode"));

    p = geometryList->polygons;
    for (i = 0; i < geometryList->numPolygons; i++)
    {
        newVertexCount += p->numIndices;
        p++;
    }

    unsharedVertices = (RtGCondVertex*)RwMalloc(
                                sizeof(RtGCondVertex) * newVertexCount,
                                rwID_GEOMCONDPLUGIN | rwMEMHINTDUR_EVENT |
                                rwMEMHINTFLAG_RESIZABLE);
    if (unsharedVertices == NULL)
    {
        RWRETURN(FALSE);
    }

    p = geometryList->polygons;
    v = unsharedVertices;
    newIndex = 0;
    for (i = 0; i < geometryList->numPolygons; i++)
    {
        for (j = 0; j < p->numIndices; j++)
        {
            RwInt32 tx;

            currentIndex = p->indices[j];
            /* Introduce new vertex
             */
            if (GCUserDataCallBacks.cloneVertexUserdata)
            {
                GCUserDataCallBacks.cloneVertexUserdata(&v->pUserData, &geometryList->vertices[currentIndex].pUserData);
            }
            v->position  = geometryList->vertices[currentIndex].position;
            v->normal    = geometryList->vertices[currentIndex].normal;
            v->preLitCol = geometryList->vertices[currentIndex].preLitCol;
            v->index     = newIndex;

            for (tx = 0; tx < geometryList->numUVs; tx++)
            {
                v->texCoords[tx].u = geometryList->vertices[currentIndex].texCoords[tx].u;
                v->texCoords[tx].v = geometryList->vertices[currentIndex].texCoords[tx].v;
            }

            /* Set unused UVs to 0 as they are used in comparison (that needs sorting out). This a small part-time fix.
            */
            for (tx = geometryList->numUVs; tx < rwMAXTEXTURECOORDS; tx++)
            {
                v->texCoords[tx].u = 0.0f;
                v->texCoords[tx].v = 0.0f;
            }

            v->matIndex  = geometryList->vertices[currentIndex].matIndex;
            v->pUserData = geometryList->vertices[currentIndex].pUserData;

            p->indices[j] = newIndex++;
            v++;
        }
        p++;
    }

    RwFree(geometryList->vertices);
    geometryList->numVertices = newVertexCount;
    geometryList->vertices = unsharedVertices;

    RWRETURN(TRUE);
}

static void
GCondFindBalancedPartitioner(TreePartition *partition, RtGCondVertex** vertexPointerList, RwInt32 vplNum,
                             RwReal weldThreshold)
{
    /* scan vertices for average and find best oriented
     * plane, best balance (ideally would be median of verts, but O(n) required)...
     */
    RwReal x, y, z, xa, xb, xc, ya, yb, yc, za, zb, zc, vx, vy, vz;
    RtGCondVertex** temp = NULL;
    RwInt32 i = 0;
    RwBool offset = TRUE;

    RWFUNCTION(RWSTRING("GCondFindBalancedPartitioner"));

    x = 0.0f; y = 0.0f; z = 0.0f;
    temp = vertexPointerList;
    for (i = 0; i < vplNum; i++)
    {
        x += (*temp)->position.x;
        y += (*temp)->position.y;
        z += (*temp)->position.z;
        temp++;
    }
    x /= (RwReal)i;
    y /= (RwReal)i;
    z /= (RwReal)i;

    /* round position of cands to the midway of the grid-lines. This is
     * to prevent two planes of the different leafs being close to each
     * other, and also have the planes aligned to a grid. */
    x = GCondSnap(x, weldThreshold, offset);
    y = GCondSnap(y, weldThreshold, offset);
    z = GCondSnap(z, weldThreshold, offset);

    /* scoring */
    xa = 0.0f; xb = 0.0f; xc = 0.0f;
    ya = 0.0f; yb = 0.0f; yc = 0.0f;
    za = 0.0f; zb = 0.0f; zc = 0.0f;
    temp = vertexPointerList;
    for (i = 0; i < vplNum; i++)
    {
        /* check number of verts either side of partition cands */
        if ((*temp)->position.x < x) xa++;
        else if ((*temp)->position.x > x) xb++;
        else xc++;

        if ((*temp)->position.y < y) ya++;
        else if ((*temp)->position.y > y) yb++;
        else yc++;

        if ((*temp)->position.z < z) za++;
        else if ((*temp)->position.z > z) zb++;
        else zc++;

        temp++;
    }

    /* qualifying */
    xa > xb ? (vx = ((xb + 1.0f) / (xa + 1.0f)) - (xc / (RwReal)i)) : (vx = ((xa + 1.0f) / (xb + 1.0f)) - (xc / (RwReal)i));
    ya > yb ? (vy = ((yb + 1.0f) / (ya + 1.0f)) - (yc / (RwReal)i)) : (vy = ((ya + 1.0f) / (yb + 1.0f)) - (yc / (RwReal)i));
    za > zb ? (vz = ((zb + 1.0f) / (za + 1.0f)) - (zc / (RwReal)i)) : (vz = ((za + 1.0f) / (zb + 1.0f)) - (zc / (RwReal)i));

    /* find and assign winner */
    if (vx > vy)
    {
        if (vx > vz)
        {
            partition->type = 0;
            partition->value = x;
        }
        else
        {
            partition->type = 8;
            partition->value = z;
        }
    }
    else
    {
        if (vy > vz)
        {
            partition->type = 4;
            partition->value = y;
        }
        else
        {
            partition->type = 8;
            partition->value = z;
        }
    }

    RWRETURNVOID();
}

static void
GCondDestroyVertexTree(VertexTree* tree)
{
    RWFUNCTION(RWSTRING("GCondDestroyVertexTree"));

    // Check for terminal case.
    if (tree == NULL)
    {
        RWRETURNVOID();
    }
    
    // Recur children.
    GCondDestroyVertexTree(tree->left);
    GCondDestroyVertexTree(tree->right);
    
    // Delete the current node.
    if (tree->vplNum != 0)
    {
        RwFree(tree->vertexPointerList);
    }
    RwFree(tree);

    RWRETURNVOID();
}

static RwBool
GCondPreLitColMatch(RwRGBA* a, RwRGBA* b, RwReal preLitThreshold)
{
    RWFUNCTION(RWSTRING("GCondPreLitColMatch"));

    RWRETURN((RwRealAbs(a->red - b->red) / 255.0f <= preLitThreshold) &&
                (RwRealAbs(a->green - b->green) / 255.0f <= preLitThreshold) &&
                (RwRealAbs(a->blue - b->blue) / 255.0f <= preLitThreshold) &&
                (RwRealAbs(a->alpha - b->alpha) / 255.0f <= preLitThreshold));
}

static VertexTree*
GCondCreateVertexTree(RtGCondVertex **vertexPointerList, RwInt32 vplNum,
                 RwBool positionalTerminal,
                 RwReal weldThreshold, RwReal normalWeldThreshold,
                 RwReal uvThreshold, RwReal preLitThreshold,
                 RwBool ignoreMaterials,
                 RwInt32 depth)
{
    /*
     * Build a KD-tree grid aligned and balanced
     * Best/Average/Worst: O(n log n)
     */
    RtGCondVertex** temp = NULL;
    RtGCondVertex** leftVertexList = NULL;
    RtGCondVertex** rightVertexList = NULL;
    RwInt32 mismatchCode = -1;
    RwInt32 i = 0, leftVertNum = 0, rightVertNum = 0;
    VertexTree* tree = NULL;
    RwBool leaf;
    TreePartition partition;

    RWFUNCTION(RWSTRING("GCondCreateVertexTree"));

    /* Create the node */
    tree = (VertexTree*)RwMalloc(sizeof(VertexTree), rwID_GEOMCONDPLUGIN | rwMEMHINTDUR_EVENT);

    tree->vertexPointerList = vertexPointerList;
    tree->vplNum = vplNum;

    /********* COMPARISONS **********/

    leaf = TRUE; /* Until proved otherwise */
    if ((FALSE != leaf) && (vplNum > 1) && (FALSE == positionalTerminal))
    {
        /* Positional test */
        mismatchCode = 1;

        /* Divide by balanced plane */
        GCondFindBalancedPartitioner(&partition, vertexPointerList, vplNum,
            weldThreshold);

        leaf = FALSE;
    }
    if ((FALSE != leaf) && (vplNum > 1) && (!ignoreMaterials))
    {
        /* Check all verts for material match */
        mismatchCode = 0;

        temp = vertexPointerList;
        for (i = 0; i < vplNum; i++)
        {
            if ((*temp)->matIndex != (*vertexPointerList)->matIndex)
            {
                leaf = FALSE;
                break;
            }

            temp++;
        }
    }

    if ((FALSE != leaf) && (vplNum > 1) && (normalWeldThreshold >= 0.0f))
    {
        /* Check all verts for normal theshold */
        mismatchCode = 2;

        temp = vertexPointerList;
        for (i = 0; i < vplNum; i++)
        {
            RwBool baseIsZero = ((RtGCondLength(&(*vertexPointerList)->normal)) < 0.01f);
            RwBool candIsZero = ((RtGCondLength(&(*temp)->normal)) < 0.01f);

            if ((!baseIsZero && !candIsZero) &&
                (GCondACos(RwV3dDotProduct(&((*vertexPointerList)->normal),
                    &((*temp)->normal))) > normalWeldThreshold))
            {
                leaf = FALSE;
                break;
            }
            temp++;
        }
    }
    if ((FALSE != leaf) && (vplNum > 1) && (preLitThreshold >= 0.0f))
    {
        /* Check all verts for prelit theshold */
        mismatchCode = 3;

        temp = vertexPointerList;
        for (i = 0; i < vplNum; i++)
        {
            if (!GCondPreLitColMatch(&(*temp)->preLitCol,
                    &(*vertexPointerList)->preLitCol, preLitThreshold))
            {
                leaf = FALSE;
                break;
            }

            temp++;
        }
    }
    if ((FALSE != leaf) && (vplNum > 1) && (uvThreshold >= 0.0f))
    {
        /* Check all verts for uv theshold */
        mismatchCode = 4;

        temp = vertexPointerList;
        for (i = 0; i < vplNum; i++)
        {
            RwInt32 j;
            for (j = 0; j < rwMAXTEXTURECOORDS; j++)
            {
                if ((RwRealAbs((*temp)->texCoords[j].u - (*vertexPointerList)->texCoords[j].u) > uvThreshold) ||
                    (RwRealAbs((*temp)->texCoords[j].v - (*vertexPointerList)->texCoords[j].v) > uvThreshold))
                {
                    leaf = FALSE;
                    break;
                }
            }
            if (leaf == FALSE) break;
            temp++;
        }
    }

    /********* CONSTRUCTION **********/

    tree->left = NULL;
    tree->right = NULL;

    /* non-leaf: filter in vertices into leftVertexList, rightVertexList */
    if (TRUE != leaf)
    {
        temp = vertexPointerList;
        for (i = 0; i < vplNum; i++)
        {
            RwBool pass = FALSE;

            switch (mismatchCode)
            {
                case 0:
                {
                    pass = (*temp)->matIndex == (*vertexPointerList)->matIndex;
                    break;
                }
                case 1:
                {
                    pass = GETCOORD((*temp)->position, partition.type) < partition.value;
                    break;
                }
                case 2:
                {
                    RwBool baseIsZero = ((RtGCondLength(&(*vertexPointerList)->normal)) < 0.01f);
                    RwBool candIsZero = ((RtGCondLength(&(*temp)->normal)) < 0.01f);

                    pass =
                        ((baseIsZero || candIsZero) ||
                        GCondACos(RwV3dDotProduct(&((*vertexPointerList)->normal),
                        &((*temp)->normal))) <= normalWeldThreshold);
                    break;
                }
                case 3:
                {
                    pass = (GCondPreLitColMatch(&(*temp)->preLitCol,
                        &(*vertexPointerList)->preLitCol, preLitThreshold));
                    break;
                }
                case 4:
                {
                    RwInt32 j;

                    pass = TRUE;
                    for (j = 0; j < rwMAXTEXTURECOORDS; j++)
                    {
                        if ((RwRealAbs((*temp)->texCoords[j].u - (*vertexPointerList)->texCoords[j].u) > uvThreshold) ||
                            (RwRealAbs((*temp)->texCoords[j].v - (*vertexPointerList)->texCoords[j].v) > uvThreshold))
                        {
                            pass = FALSE;
                        }
                    }
                    break;
                }
                default:
                    RWASSERT(FALSE);
            }

            if (pass)
            {
                leftVertexList = (RtGCondVertex**)RwRealloc(leftVertexList, sizeof(RtGCondVertex*) * (leftVertNum + 1),
                                                     rwID_GEOMCONDPLUGIN | rwMEMHINTDUR_EVENT | rwMEMHINTFLAG_RESIZABLE);
                leftVertexList[leftVertNum++] = *temp;
            }
            else
            {
                rightVertexList = (RtGCondVertex**)RwRealloc(rightVertexList, sizeof(RtGCondVertex*) * (rightVertNum + 1),
                                                     rwID_GEOMCONDPLUGIN | rwMEMHINTDUR_EVENT | rwMEMHINTFLAG_RESIZABLE);
                rightVertexList[rightVertNum++] = *temp;
            }
            temp++;
        }

        /* for each child repeat */
        if (leftVertNum)
        {
            tree->left = GCondCreateVertexTree(leftVertexList, leftVertNum, rightVertNum == 0, 
                weldThreshold, normalWeldThreshold, uvThreshold, preLitThreshold, ignoreMaterials, depth+1);
        }

        if (rightVertNum)
        {
            tree->right = GCondCreateVertexTree(rightVertexList, rightVertNum, leftVertNum == 0, 
                weldThreshold, normalWeldThreshold, uvThreshold, preLitThreshold, ignoreMaterials, depth+1);
        }
    }

    RWRETURN(tree);
}

typedef struct PrelitColTot PrelitColTot;
struct PrelitColTot
{
    RwInt32 red;
    RwInt32 green;
    RwInt32 blue;
    RwInt32 alpha;
};

static void
GCondTraverseVertexTree(VertexTree* tree, RwInt32* remapper, RwReal tol,
                        RwBool remapInfoOnly, RwBool averageAttributes)
{
    RwInt32 i, j;
    RtGCondVertex** tempVerts;
    RwV3d avg = {0.0f, 0.0f, 0.0f};
    RwV3d normAvg = {0.0f, 0.0f, 0.0f};
    PrelitColTot prelitAvg = {0, 0, 0, 0};
    RwTexCoords uvAvg[rwMAXTEXTURECOORDS];

    RWFUNCTION(RWSTRING("GCondTraverseVertexTree"));

    if (!tree)
    {
        RWRETURNVOID();
    }

    if (!remapInfoOnly && averageAttributes)
    {
        for (j = 0; j < rwMAXTEXTURECOORDS; j++)
        {
            uvAvg[j].u = 0.0f;
            uvAvg[j].v = 0.0f;
        }
    }
    if ((tree->left == NULL) && (tree->right == NULL))
    {
        /* leaf */
        if (tree->vplNum <= 1)
        {
            if (tree->vplNum == 1)
            {
                remapper[(*tree->vertexPointerList)->index] = (*tree->vertexPointerList)->index;
            }
            RWRETURNVOID();
        }
        if (!remapInfoOnly)
        {
            /* All the vertices in this cell are in proximity and should be merged if
             * conditions are okay
             */
            
            /* Calculate the average values */
            tempVerts = tree->vertexPointerList;
            for (i = 0; i < tree->vplNum; i++)
            {
                avg.x += (*tempVerts)->position.x;
                avg.y += (*tempVerts)->position.y;
                avg.z += (*tempVerts)->position.z;

                if(averageAttributes)
                {
                    normAvg.x += (*tempVerts)->normal.x;
                    normAvg.y += (*tempVerts)->normal.y;
                    normAvg.z += (*tempVerts)->normal.z;

                    prelitAvg.red += (*tempVerts)->preLitCol.red;
                    prelitAvg.green += (*tempVerts)->preLitCol.green;
                    prelitAvg.blue += (*tempVerts)->preLitCol.blue;
                    prelitAvg.alpha += (*tempVerts)->preLitCol.alpha;

                    for (j = 0; j < rwMAXTEXTURECOORDS; j++)
                    {
                        uvAvg[j].u += (*tempVerts)->texCoords[j].u;
                        uvAvg[j].v += (*tempVerts)->texCoords[j].v;
                    }
                }
                tempVerts++;
            }

            avg.x /= (RwReal)tree->vplNum;
            avg.y /= (RwReal)tree->vplNum;
            avg.z /= (RwReal)tree->vplNum;

            if(averageAttributes)
            {
                normAvg.x /= (RwReal)tree->vplNum;
                normAvg.y /= (RwReal)tree->vplNum;
                normAvg.z /= (RwReal)tree->vplNum;
                RtGCondNormalize(&normAvg);

                prelitAvg.red /= tree->vplNum;
                prelitAvg.green /= tree->vplNum;
                prelitAvg.blue /= tree->vplNum;
                prelitAvg.alpha /= tree->vplNum;

                for (j = 0; j < rwMAXTEXTURECOORDS; j++)
                {
                    uvAvg[j].u /= (RwReal)tree->vplNum;
                    uvAvg[j].v /= (RwReal)tree->vplNum;
                }
            }
        }
        
        /* We have the average values now assign them to the vertices. */
        tempVerts = tree->vertexPointerList;
        for (i = 0; i < tree->vplNum; i++)
        {
            if (!remapInfoOnly)
            {
                (*tempVerts)->position.x = avg.x;
                (*tempVerts)->position.y = avg.y;
                (*tempVerts)->position.z = avg.z;

                if(averageAttributes)
                {
                    (*tempVerts)->normal.x = normAvg.x;
                    (*tempVerts)->normal.y = normAvg.y;
                    (*tempVerts)->normal.z = normAvg.z;

                    (*tempVerts)->preLitCol.red = prelitAvg.red;
                    (*tempVerts)->preLitCol.green = prelitAvg.green;
                    (*tempVerts)->preLitCol.blue = prelitAvg.blue;
                    (*tempVerts)->preLitCol.alpha = prelitAvg.alpha;

                    for (j = 0; j < rwMAXTEXTURECOORDS; j++)
                    {
                        uvAvg[j].u /= uvAvg[j].u;
                        uvAvg[j].v /= uvAvg[j].v;
                    }
                }
            }
            remapper[(*tempVerts)->index] = (*tree->vertexPointerList)->index;

            tempVerts++;
        }
        RWRETURNVOID();
    }
    else
    {
        /* for each child repeat */
        GCondTraverseVertexTree(tree->left, remapper, tol, remapInfoOnly, averageAttributes);
        GCondTraverseVertexTree(tree->right, remapper, tol, remapInfoOnly, averageAttributes);
    }

    RWRETURNVOID();
}

/**
 * \ingroup rtgcond
 * \ref RtGCondWeldVerticesPipelineNode clusters all nearby vertices that
 * share near identical normal, uv and preLights, and welds them
 * into a single vertex - then remaps the polygons.
 *
 * \note This function does exactly what it says - this is NOT a
 * mesh decimator and should not be used as one! The weldThreshold
 * must be smaller than the smallest triangle (in its smallest
 * dimension), otherwise the algorithm cannot be guaranteed to be
 * correct, and it might lead to holes, tucks and other topological
 * oddities that will affect such things as PVS not to mention the
 * appearance of the objects in the scene!
 *
 * \param geometryList        a pointer to the list of geometry
 * \param weldThreshold       the positional threshold in 3-space
 * \param normalWeldThreshold the normal threshold in degrees
 * \param uvThreshold         the UV threshold in uv-units
 * \param preLitThreshold     the prelight threshold in range 0..1, r, g
 *                            b and a are all checked against this single value
 * \param ignoreMaterials     FALSE by default. Set it to true for non-aesthetic welding, such
 *                            as collision worlds.
 * \param remap               TRUE if remapping required, FALSE otherwise. TRUE by default.
 * \param implicit            TRUE if the function should only calculate remap information
 *                            only without any action, FALSE otherwise. FALSE by default.
 * \param averageAttributes   TRUE if the attributes (normals, uvs, prelights) are to be averaged.
 *                            \note postitions are averaged by default if implicit parameter is FALSE.
 *
 * \return remap information, (an index to index map). This is of use if implict is TRUE.
 *
 * \see RtGCondRemapVerticesPipelineNode
 */
RwInt32*
RtGCondWeldVerticesPipelineNode(RtGCondGeometryList *geometryList, RwReal weldThreshold,
                    RwReal normalWeldThreshold, RwReal uvThreshold, RwReal preLitThreshold,
                    RwBool ignoreMaterials, RwBool remap, RwBool implicit, RwBool averageAttributes)
{
    /* Need to pass conditions, e.g. weld iff same normal, or same color, etc. */
    VertexTree* tree;
    RtGCondVertex** vpl = (RtGCondVertex**)RwMalloc(sizeof(RtGCondVertex*) * geometryList->numVertices,
                            rwID_GEOMCONDPLUGIN | rwMEMHINTDUR_EVENT);
    RtGCondVertex** tmp;
    RtGCondVertex* temp;
    RwInt32 i;
    RwInt32* remapper;
    RtGCondPolygon* tempPolygons;

    RWAPIFUNCTION(RWSTRING("RtGCondWeldVerticesPipelineNode"));


    /* Create vertex pointer list - possibly move to data structure? */
    tmp = vpl;
    temp = geometryList->vertices;
    for (i = 0; i < geometryList->numVertices; i++)
    {
        *tmp++ = temp++;
    }

    /* Merge into buckets on offset boundary */
    tree = GCondCreateVertexTree(vpl, geometryList->numVertices,
        FALSE, weldThreshold, normalWeldThreshold,
        uvThreshold, preLitThreshold, ignoreMaterials, 0);

    /* Weld and create remap info */
    remapper = (RwInt32*)RwMalloc(sizeof(RwInt32) * geometryList->numVertices,
                                  rwID_GEOMCONDPLUGIN | rwMEMHINTDUR_FUNCTION);


    GCondTraverseVertexTree(tree, remapper, weldThreshold, implicit, averageAttributes);

    if (remap && !implicit)
    {
        /* remapper holds array of indices pointing to representative unique indices,
         * use if to remap polygons, i.e. if vertex 5, 6 and 7 are the same, let all refs
         * to 6 and 7 become 5.
         */
        RwInt32 j;
        i = 0;
        tempPolygons = geometryList->polygons;
        while (i < geometryList->numPolygons)
        {
            j = 0;
            while (j < tempPolygons->numIndices)
            {
                tempPolygons->indices[j] = remapper[tempPolygons->indices[j]];
                j++;
            }
            tempPolygons++;
            i++;
        }

        RtGCondRemapVerticesPipelineNode(geometryList);
    }
    /* RwFree(remapper); */

    GCondDestroyVertexTree(tree);
    tree = NULL;

    vpl = NULL;

    if (implicit)
    {
        RWRETURN(remapper);
    }
    else
    {
        RwFree(remapper);
        remapper = NULL;
        RWRETURN(NULL);
    }
}

/**
 * \ingroup rtgcond
 * \ref RtGCondBuildNormalsPipelineNode rebuilds the normals for
 *  the given geometryList. A resulting  vertex normal will be an
 *  average of all polygon normals, for polygons that reference
 *  that vertex. Each polygon normal will be used proportional to
 *  the angle of that polygon marked by that vertex.
 *
 * \param geometryList A pointer to the list of geometry
 */
void
RtGCondBuildNormalsPipelineNode(RtGCondGeometryList * geometryList)
{
    RwInt32             i, j, vertIndices[3];
    RwReal              EFsinTheta, EFcosTheta, angle;
    RwV3d               edge[2], triNormal, *normals, *currNormal;
    RtGCondVertex       *v0, *v1, *v2;
    RtGCondPolygon      *currPoly;

    RWAPIFUNCTION(RWSTRING("RtGCondBuildNormalsPipelineNode"));

    /* We need to store an array of last normals assigned, in case,
     * two or more normals from neighbouring polygons all add up to
     * zero length vector, i.e. when a vert is shared between two 
     * double-sided polygons. */
    normals = RwMalloc(sizeof(RwV3d) * geometryList->numVertices,
                       rwMEMHINTDUR_FUNCTION | rwID_GEOMCONDPLUGIN);

    /* Wipe vertex normals and initialise the normals array. */
    for (i = 0; i < geometryList->numVertices; i++)
    {
        currNormal = &geometryList->vertices[i].normal;
        currNormal->x = currNormal->y = currNormal->z = 0.0f;
        normals[i].x = normals[i].y = normals[i].z = 0.0f;
    }

    /* Accumulate normal contributions from polygons. */
    for (i = 0; i < geometryList->numPolygons; i++)
    {
        currPoly = &geometryList->polygons[i];
        for (j = 0; j < currPoly->numIndices; j++)
        {    
            /* Calculate the indices. */
            vertIndices[0] = j;
            vertIndices[1] = vertIndices[0] + 1;
            if (vertIndices[1] >= currPoly->numIndices)
            {
                vertIndices[1] = 0;
            }
            vertIndices[2] = vertIndices[1] + 1;
            if (vertIndices[2] >= currPoly->numIndices)
            {
                vertIndices[2] = 0;
            }

            /* Get the vertices */
            v0 = &geometryList->vertices[currPoly->indices[vertIndices[0]]];
            v1 = &geometryList->vertices[currPoly->indices[vertIndices[1]]];
            v2 = &geometryList->vertices[currPoly->indices[vertIndices[2]]];

            /* Calculate the edges */
            RwV3dSub(&edge[0], &v1->position, &v0->position);
            RwV3dSub(&edge[1], &v2->position, &v1->position);

            /* Calculate the triangle normal */
            RwV3dCrossProduct(&triNormal, &edge[0], &edge[1]);
        
            /* Get the normal length and normalize it. */
            EFsinTheta = RtGCondLength(&triNormal);
            if (EFsinTheta == 0)
            {
                continue;
            }
            RtGCondNormalize(&triNormal);

            /* Save the last assigned normal for this vertex.
             * Do this before it is scaled by the angle. */
            normals[currPoly->indices[vertIndices[1]]] = triNormal;

            /* Calculate internal angles. */
            EFcosTheta = -RwV3dDotProduct(&edge[1], &edge[0]);
            angle = (RwReal) RwATan2(EFsinTheta, EFcosTheta);

            /* Add the normal to the triangle's vertex. */
            RwV3dScale(&triNormal, &triNormal, angle);
            RwV3dAdd(&v1->normal, &v1->normal, &triNormal);
        }
    }

    /* Make vertex normals of unit length */
    for (i = 0; i < geometryList->numVertices; i++)
    {
        currNormal = &geometryList->vertices[i].normal;

        if (!RtGCondNormalize(currNormal))
        {
            /* Normalization fails because we have a zero-length vector.
             * Check if we had anything assigned to this vertex, if not
             * it is a vertex which is not used by any polygon and it'll
             * have a zero-length vector for the normal. */
            if ((normals[i].x != 0.0f) || (normals[i].y != 0.0f) ||
                (normals[i].z != 0.0f))
            {
                *currNormal = normals[i];
            }
        }
    }

    /* Free the temp array of normals. */
    RwFree(normals);

    RWRETURNVOID();
}

/**
 * \ingroup rtgcond
 * \ref RtGCondRemoveIdenticalPolygonsPipelineNode removes all but one polygon that
 * occupies the exactly same space and has the same material as another -
 * this leads to bad welding and z-fighting, and ideally should not be
 * in the geometry in the first place.
 *
 * \param geometryList a pointer to the list of geometry
 *
 * \see RtGCondCullZeroAreaPolygonsPipelineNode
 * \see RtGCondUnshareVerticesOnMaterialBoundariesPipelineNode
 * \see RtGCondLimitUVsPipelineNode
 * \see RtGCondWeldVerticesPipelineNode
 */
void
RtGCondRemoveIdenticalPolygonsPipelineNode(RtGCondGeometryList* geometryList)
{
    RwInt32 i, jpv, j, ipv, c;
    RtGCondPolygon* ip;
    RtGCondPolygon* jp;
    RwBool startMatch, endMatch;
    RwInt32* vRemapper;

    RWAPIFUNCTION(RWSTRING("RtGCondRemoveIdenticalPolygonsPipelineNode"));

    /* General Z-code for geometrical identical duplicates.
     */

    vRemapper = RtGCondWeldVerticesPipelineNode(geometryList, GCParams->weldThreshold,
                -1, -1, -1, /* positional only */
                FALSE, TRUE, TRUE, FALSE);
    _rtGCondCreateVertexPolygonReference(geometryList, vRemapper);


    /* for ALL polygons */
    for (i = 0; i < geometryList->numPolygons; i++)
    {
        ip = &geometryList->polygons[i];

        if (ip->id == -1) continue;

        /* For all vertices of the polygon */
        /* For each polygon attached to vertex, test the rest of the indices */
        for (c = 0; c < ip->numIndices; c++)
        {
            for (j = 0; j < geometryList->vertices[ip->indices[c]].numBrauxIndices; j++)
            {
                jp = &geometryList->polygons[geometryList->vertices[ip->indices[c]].brauxIndices[j]];
                if (ip == jp || ip->matIndex != jp->matIndex)
                {
                    /* The very same polygon is fine, of course, and diff materials
                     * mean these won't get welded anyhow
                     */
                    continue;
                }
                startMatch = FALSE;
                for (jpv = 0; jpv < jp->numIndices; jpv++)
                {
                    if (vRemapper[jp->indices[jpv]] == vRemapper[ip->indices[0]])
                    {
                        startMatch = TRUE;
                        break;
                    }
                }
                if (startMatch)
                {
                    endMatch = TRUE;
                    for (ipv = 0; ipv < ip->numIndices; ipv++)
                    {
                        if (vRemapper[jp->indices[jpv]] != vRemapper[ip->indices[ipv]])
                        {
                            endMatch = FALSE;
                            break;
                        }
                        jpv++;
                        if (jpv >= jp->numIndices) jpv = 0;
                    }
                }
                if (startMatch && endMatch)
                {
                    jp->id = -1;
                }
            }
        }
    }
    GCondCullTaggedPolygons(geometryList);
    _rtGCondDestroyVertexPolygonReference(geometryList);
    RwFree(vRemapper);
    vRemapper = NULL;

    RWRETURNVOID();
}

/**
 * \ingroup rtgcond
 * \ref RtGCondRemoveSliversPipelineNode removes slivers and faces with fewer
 * than three vertices. Subsequently, it
 * remaps removing any unused vertices.
 *
 * \param geometryList a pointer to the list of geometry
 *
 * \see RtGCondCullZeroAreaPolygonsPipelineNode
 * \see RtGCondUnshareVerticesOnMaterialBoundariesPipelineNode
 * \see RtGCondLimitUVsPipelineNode
 * \see RtGCondWeldVerticesPipelineNode
 */
void
RtGCondRemoveSliversPipelineNode(RtGCondGeometryList* geometryList)
{
    RwInt32 i, ct, cpo;
    RtGCondPolygon* ip;
    RwV3d a, b, c, e, f;
    RwInt32 pCheckSum = 0, newIndex;
    RtGCondPolygon *newPolygons, *tempNewPolygons;
    RtGCondPolygon *tempOldPolygons, *p;

    RWAPIFUNCTION(RWSTRING("RtGCondRemoveSliversPipelineNode"));

    /* General Z-code for really bad scenes (after bad geometry perturbation):
     * Removes polygons with less than 3 vertices, slivers of the
     * form  *------------+ and of the form  +-----+------+
     */


    /* Remove collapsed faces and unused vertices... */
    p = geometryList->polygons;
    for (i = 0; i < geometryList->numPolygons; i++)
    {
        if (geometryList->polygons[i].numIndices >= 3)
        {
            pCheckSum++;
        }
        p++;
    }


    /* Here, we need to squish polygon array... */
    newPolygons = (RtGCondPolygon*) RwMalloc(sizeof(RtGCondPolygon) * pCheckSum,
             rwMEMHINTDUR_EVENT | rwMEMHINTFLAG_RESIZABLE | rwID_GEOMCONDPLUGIN);

    tempNewPolygons = newPolygons;
    tempOldPolygons = geometryList->polygons;
    newIndex = 0;
    for (i = 0; i < geometryList->numPolygons; i++)
    {
        if (tempOldPolygons->numIndices >= 3)
        {
            /* Copy all vertex attributes from old position to new */
            *tempNewPolygons = *tempOldPolygons;

            newIndex++;
            tempNewPolygons++;
        }
        else if (tempOldPolygons->numIndices > 0)/* Z-code */
        {
            RtGCondFreeIndices(tempOldPolygons);
            if (GCUserDataCallBacks.destroyPolygonUserdata)
            {
                GCUserDataCallBacks.destroyPolygonUserdata(&tempOldPolygons->pUserData);
            }
        }
        tempOldPolygons++;
    }
    RWASSERT(newIndex == pCheckSum);

    RwFree(geometryList->polygons);
    geometryList->polygons = newPolygons;
    geometryList->numPolygons = pCheckSum;

    RtGCondRemapVerticesPipelineNode(geometryList);
    RtGCondCullZeroAreaPolygonsPipelineNode(geometryList, 0.0f);



    /* Remove slivers... */

    /* for ALL polygons */
    for (i = 0; i < geometryList->numPolygons; i++)
    {
        ip = &geometryList->polygons[i];

        if (ip->id == -1) continue;

        /* For all vertices of the polygon */
        /* For each polygon attached to vertex, test the rest of the indices */
        if (ip->numIndices == 3)
        {
            for (ct = 0; ct < ip->numIndices; ct++)
            {
                cpo = ct + 1;
                if (cpo == ip->numIndices) cpo = 0;

                if (GCondPositionEqual(&geometryList->vertices[ip->indices[ct]].position,
                    &geometryList->vertices[ip->indices[cpo]].position))
                {
                    ip->id = -1;
                }
            }
        }
    }
    GCondCullTaggedPolygons(geometryList);



    /* Remove awkward slivers... */

    /* for ALL polygons */
    for (i = 0; i < geometryList->numPolygons; i++)
    {
        ip = &geometryList->polygons[i];

        if (ip->id == -1) continue;

        if (ip->numIndices == 3)
        {
            /* For all vertices of the polygon */
            /* For each polygon attached to vertex, test the rest of the indices */
            a = geometryList->vertices[ip->indices[0]].position;
            b = geometryList->vertices[ip->indices[1]].position;
            c = geometryList->vertices[ip->indices[2]].position;
            RwV3dSub(&e, &b, &a);
            RwV3dSub(&f, &c, &a);
            RtGCondNormalize(&e);
            RtGCondNormalize(&f);

            if ((GCondEqualZero(e.x - f.x, GCONDZERO) && GCondEqualZero(e.y - f.y, GCONDZERO) && GCondEqualZero(e.z - f.z, GCONDZERO)) ||
                (GCondEqualZero(e.x + f.x, GCONDZERO) && GCondEqualZero(e.y + f.y, GCONDZERO) && GCondEqualZero(e.z + f.z, GCONDZERO)))
            {
                ip->id = -1;
            }
        }
    }
    GCondCullTaggedPolygons(geometryList);

    RWRETURNVOID();
}



/****************************************************************************
 * Geometry Conditioning Controllers
 ***************************************************************************/

/**
 * \ingroup rtgcond
 * \ref RtGCondFixAndFilterGeometryPipeline a provided, custom pipeline used
 * by RenderWare Graphics' \ref rtworldimport under geometry conditioning.
 * Comprises a number of pipelined tools (filters and fixers). Prepares
 * geometry for polygon welding and platform specific conditioning.
 *
 * \param geometryList a pointer to the list of geometry
 *
 * \return TRUE if successful, FALSE otherwise
 *
 * \see RtGCondCullZeroAreaPolygonsPipelineNode
 * \see RtGCondUnshareVerticesOnMaterialBoundariesPipelineNode
 * \see RtGCondLimitUVsPipelineNode
 * \see RtGCondWeldVerticesPipelineNode
 */
RwBool
RtGCondFixAndFilterGeometryPipeline(RtGCondGeometryList *geometryList)
{
    RWAPIFUNCTION(RWSTRING("RtGCondFixAndFilterGeometryPipeline"));

    /* Tidy up the normals - we want them at high precision */
    GCondRenormalizeNormals(geometryList->vertices, geometryList->numVertices);

    /* Weld vertices - two-passes: the first moves all vertices in unison (which
     * is not possible if there's any constraints) to
     * keep the world sealed, the second then honors the other parameters.
     * this is not a mesh welder as such, it just reduces near identicals,
     * and helps with slivers).
     */
    RtGCondWeldVerticesPipelineNode(geometryList,
                            GCParams->weldThreshold,
                            -1.0f, -1.0f, -1.0f,
                            TRUE, FALSE, FALSE, FALSE);
    RtGCondWeldVerticesPipelineNode(geometryList,
                            0.0f,
                            GCParams->flags & rtGCONDNORMALS ? GCParams->angularThreshold : -1.0f,
                            GCParams->flags & rtGCONDTEXTURES ? GCParams->uvThreshold : -1.0f,
                            GCParams->flags & rtGCONDPRELIT ? GCParams->preLitThreshold : -1.0f,
                            FALSE, TRUE, FALSE, TRUE);

    RtGCondRemoveSliversPipelineNode(geometryList);
    RtGCondRemoveIdenticalPolygonsPipelineNode(geometryList);

    if (GCParams->areaThreshold >= 0.0f)
    {
        /* Cull zero area polygons - no point rendering a load of invisible geometry */
        RtGCondCullZeroAreaPolygonsPipelineNode(geometryList, GCParams->areaThreshold);
    }

    /* Unshare the vertices. Could fail due to not enough memory. */
    if (!RtGCondUnshareVerticesPipelineNode(geometryList))
    {
        RWRETURN(FALSE);
    }

    RtGCondLimitUVsPipelineNode(geometryList, GCParams->uvLimitMin,
        GCParams->uvLimitMax, GCParams->texelLimits);

    /* Finally, make sure shared vertices is allowed, this was undone by
     * a call to RtGCondUnshareVerticesPipelineNode
     */
    RtGCondWeldVerticesPipelineNode(    geometryList,
                            0.0f,
                            0.0f,
                            0.0f,
                            0.0f,
                            FALSE, TRUE, FALSE, FALSE);

    /* Duplicate vertices on material boundaries and sort by material */
    RtGCondUnshareVerticesOnMaterialBoundariesPipelineNode(geometryList);
    RtGCondSortVerticesOnMaterialPipelineNode(geometryList);

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgcond
 * \ref RtGCondGroundUVs shifts uvs values so that the smallest value in u and v
 * is in the range [0..1)
 *
 * \param geometryList a pointer to the list of geometry
 *
 * \see RtGCondLimitUVsPipelineNode
 */
void
RtGCondGroundUVs(RtGCondGeometryList * geometryList)
{
    /* Efficiency: O(n); where n is the number of polygons
     */

    RwInt32 polyIndex;

    RWAPIFUNCTION(RWSTRING("RtGCondGroundUVs"));

    /* For all polygons */
    for (polyIndex = 0; polyIndex < geometryList->numPolygons; polyIndex++)
    {
        RwInt32 i;
        RwInt32 v;
        RwReal limitUVOffset[2];
        RwReal uMin[rwMAXTEXTURECOORDS];
        RwReal uMax[rwMAXTEXTURECOORDS];
        RwReal vMin[rwMAXTEXTURECOORDS];
        RwReal vMax[rwMAXTEXTURECOORDS];
        RwInt32 numUVs = 0;

        for (i = 0; i < rwMAXTEXTURECOORDS; i++)
        {
            uMin[i] = RwRealMAXVAL;
            uMax[i] = RwRealMINVAL;
            vMin[i] = RwRealMAXVAL;
            vMax[i] = RwRealMINVAL;
        }


        /* Calculate min & max u & v... */
        /* For all verts */
        for (v = 0; v < geometryList->polygons[polyIndex].numIndices; v++)
        {
            RtGCondVertex* vertex = &geometryList->vertices[geometryList->polygons[polyIndex].indices[v]];
            RwInt32 j;

            numUVs = geometryList->numUVs;
            for (j = 0; j < numUVs; j++)
            {
                if (rwTEXTUREADDRESSWRAP == GCParams->textureMode[j])
                {
                    RwTexCoords texCoords = vertex->texCoords[j];
                    if (uMin[j] > texCoords.u)
                    {
                        uMin[j] = texCoords.u;
                    }
                    if (uMax[j] < texCoords.u)
                    {
                        uMax[j] = texCoords.u;
                    }
                    if (vMin[j] > texCoords.v)
                    {
                        vMin[j] = texCoords.v;
                    }
                    if (vMax[j] < texCoords.v)
                    {
                        vMax[j] = texCoords.v;
                    }
                }
            }
        }

        /* Iterate on all uvs, applying the limit filter */
        for (i = 0; i < numUVs; i++)
        {
            if (rwTEXTUREADDRESSWRAP == GCParams->textureMode[i])
            {
                RwInt32 uv;
                RwReal min[2];
                RwReal max[2];

                min[0] = uMin[i];
                min[1] = vMin[i];
                max[0] = uMax[i];
                max[1] = vMax[i];
                limitUVOffset[0] = 0.0f;
                limitUVOffset[1] = 0.0f;

                for (uv = 0; uv < 2; uv++)
                {
                    max[uv] -= (RwReal)(RwFloor(min[uv]));
                    limitUVOffset[uv] -= (RwReal)(RwFloor(min[uv]));
                    min[uv] -= (RwReal)(RwFloor(min[uv]));
                }
                for (v = 0; v < geometryList->polygons[polyIndex].numIndices; v++)
                {
                    RtGCondVertex* vertex = &geometryList->vertices[geometryList->polygons[polyIndex].indices[v]];

                    vertex->texCoords[i].u += limitUVOffset[0];
                    vertex->texCoords[i].v += limitUVOffset[1];
                }
            }
        }
    }

    RWRETURNVOID();
}

static RwInt32
ImportSortPolygonCmp(const void *a, const void *b)
{
    const RtGCondPolygon *paPolygon = (const RtGCondPolygon *)a;
    const RtGCondPolygon *pbPolygon = (const RtGCondPolygon *)b;
    const RwV3d* paCentroid = ((const RwV3d*)paPolygon->pInternalData);
    const RwV3d* pbCentroid = ((const RwV3d*)pbPolygon->pInternalData);
    RwReal dec = 100.0f; /* 10^#Decimal_places */

    RWFUNCTION(RWSTRING("ImportSortPolygonCmp"));

    if ((RwInt32FromRealMacro(paCentroid->x * dec - pbCentroid->x * dec)) != 0)
    {
        RWRETURN(RwInt32FromRealMacro(paCentroid->x * dec - pbCentroid->x * dec));
    }
    if ((RwInt32FromRealMacro(paCentroid->z * dec - pbCentroid->z * dec)) != 0)
    {
        RWRETURN(RwInt32FromRealMacro(paCentroid->z * dec - pbCentroid->z * dec));
    }
    RWRETURN(RwInt32FromRealMacro(paCentroid->y * dec - pbCentroid->y * dec));
}


/**
 * \ingroup rtgcond
 * \ref RtGCondDecimateAndWeldGeometryPipeline a provided, custom pipeline used
 * by RenderWare Graphics' \ref rtworldimport under geometry conditioning.
 * Comprises a number of pipelined tools (welders). Prepares geometry for
 * platform specific conditioning.
 *
 * \param geometryList a pointer to the list of geometry
 *
 * \return TRUE if successful, FALSE otherwise
 *
 * \see RtWingCreate
 * \see RtWingEdgeDecimation
 */
RwBool
RtGCondDecimateAndWeldGeometryPipeline(RtGCondGeometryList *geometryList)
{
    RwInt32 i;
    RtWings wings;
    RwV3d   *centroids = (RwV3d*)RwMalloc(sizeof(RwV3d) * geometryList->numPolygons,
                                     rwMEMHINTDUR_FUNCTION | rwID_GEOMCONDPLUGIN);

    RWAPIFUNCTION(RWSTRING("RtGCondDecimateAndWeldGeometryPipeline"));

    GCondRenormalizeNormals(geometryList->vertices, geometryList->numVertices);

    /* Unshare the vertices. Could fail due to not enough memory. */
    if (!RtGCondUnshareVerticesPipelineNode(geometryList))
    {
        RWRETURN(FALSE);
    }

    /* Prerequiste for super-efficient UV-translation */
    RtGCondGroundUVs(geometryList);
    /* Make sure we haven't breech any limits by doing this */
    RtGCondLimitUVsPipelineNode(geometryList, GCParams->uvLimitMin,
        GCParams->uvLimitMax, GCParams->texelLimits);

    /* Another prerequiste for super-efficient UV-translation */
    for (i = 0; i < geometryList->numPolygons; i++)
    {
        centroids[i].x = (geometryList->vertices[geometryList->polygons[i].indices[0]].position.x +
            geometryList->vertices[geometryList->polygons[i].indices[1]].position.x +
            geometryList->vertices[geometryList->polygons[i].indices[2]].position.x) / 3.0f;
        centroids[i].y = (geometryList->vertices[geometryList->polygons[i].indices[0]].position.y +
            geometryList->vertices[geometryList->polygons[i].indices[1]].position.y +
            geometryList->vertices[geometryList->polygons[i].indices[2]].position.y) / 3.0f;
        centroids[i].z = (geometryList->vertices[geometryList->polygons[i].indices[0]].position.z +
            geometryList->vertices[geometryList->polygons[i].indices[1]].position.z +
            geometryList->vertices[geometryList->polygons[i].indices[2]].position.z) / 3.0f;
        geometryList->polygons[i].pInternalData = (void*)(&centroids[i]);
    }

    qsort(geometryList->polygons, geometryList->numPolygons,
      sizeof(RtGCondPolygon), ImportSortPolygonCmp);

    RwFree(centroids);
    centroids = NULL;

    /* Exit if we failed to create the structure. Probably not enough memory. */
    if (!RtWingCreate(&wings, geometryList))
    {
        RWRETURN(FALSE);
    }
    
    RtWingEdgeDecimation(&wings, geometryList);
    RtWingConvexPartitioning(&wings, geometryList, GCParams->convexPartitioningMode);
    RtWingDestroy(&wings);

    RtGCondRemoveSliversPipelineNode(geometryList);
    RtGCondCullZeroAreaPolygonsPipelineNode(geometryList, 0.0f);
    RtGCondRemapVerticesPipelineNode(geometryList);

    /* Finally, make sure shared vertices is allowed, this was undone by
     * a call to RtGCondUnshareVerticesPipelineNode */
    RtGCondWeldVerticesPipelineNode(    geometryList,
                            0.0f,
                            0.0f,
                            0.0f,
                            0.0f,
                            FALSE, TRUE, FALSE, FALSE);

    /* Duplicate vertices on material boundaries and sort by material */
    RtGCondUnshareVerticesOnMaterialBoundariesPipelineNode(geometryList);
    RtGCondSortVerticesOnMaterialPipelineNode(geometryList);

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgcond
 * \ref RtGCondSetGeometryConditioningPipeline sets the geometry conditioning
 * pipeline that \ref RtGCondApplyGeometryConditioningPipeline will call.
 *
 * \param pipeline a pointer to a predefined RtGCondGeometryConditioningPipeline
 *
 * \see RtGCondGetGeometryConditioningPipeline
 * \see RtGCondApplyGeometryConditioningPipeline
 * \see RtGCondFixAndFilterGeometryPipeline
 * \see RtGCondDecimateAndWeldGeometryPipeline
 */
void
RtGCondSetGeometryConditioningPipeline(RtGCondGeometryConditioningPipeline pipeline)
{
    RWAPIFUNCTION(RWSTRING("RtGCondSetGeometryConditioningPipeline"));

    conditioningPipeline = pipeline;

    RWRETURNVOID();
}

/**
 * \ingroup rtgcond
 * \ref RtGCondGetGeometryConditioningPipeline gets the geometry conditioning
 * pipeline that \ref RtGCondApplyGeometryConditioningPipeline will call.
 *
 * \returns a pointer to the RtGCondGeometryConditioningPipeline
 *
 * \see RtGCondSetGeometryConditioningPipeline
 * \see RtGCondApplyGeometryConditioningPipeline
 * \see RtGCondFixAndFilterGeometryPipeline
 * \see RtGCondDecimateAndWeldGeometryPipeline
 */
RtGCondGeometryConditioningPipeline
RtGCondGetGeometryConditioningPipeline( void )
{
    RWAPIFUNCTION(RWSTRING("RtGCondGetGeometryConditioningPipeline"));

    RWRETURN(conditioningPipeline);
}

/**
 * \ingroup rtgcond
 * \ref RtGCondApplyGeometryConditioningPipeline actions the geometry conditioning
 * pipeline set by \ref RtGCondSetGeometryConditioningPipeline.
 *
 * \param geometryList a pointer to the list of geometry
 *
 * \see RtGCondSetGeometryConditioningPipeline
 * \see RtGCondGetGeometryConditioningPipeline
 * \see RtGCondFixAndFilterGeometryPipeline
 * \see RtGCondDecimateAndWeldGeometryPipeline
 */
void
RtGCondApplyGeometryConditioningPipeline(RtGCondGeometryList *geometryList)
{
    RWAPIFUNCTION(RWSTRING("RtGCondApplyGeometryConditioningPipeline"));

    if (conditioningPipeline)
    {
        conditioningPipeline(geometryList);
    }

    RWRETURNVOID();
}

/**
 * \ingroup rtgcond
 * \ref RtGCondFreeVertices destroys the vertices in the geometry list.
 *
 * \param geometryList a pointer to the list of geometry
 *
 * \see RtGCondAllocateVertices
 */
void
RtGCondFreeVertices(RtGCondGeometryList* geometryList)
{
    RWAPIFUNCTION(RWSTRING("RtGCondFreeVertices"));

    RwFree(geometryList->vertices);
    geometryList->vertices = NULL;
    geometryList->numVertices = 0;

    RWRETURNVOID();
}

/**
 * \ingroup rtgcond
 * \ref RtGCondAllocateVertices allocates space for a number of vertices
 *
 * \param geometryList a pointer to the list of geometry
 * \param num the number of vertices to allocate
 *
 * \return TRUE if successful, FALSE otherwise
 *
 * \see RtGCondAllocatePolygons
 * \see RtGCondFreeVertices
 */
RwBool
RtGCondAllocateVertices(RtGCondGeometryList* geometryList, RwInt32 num)
{
    RWAPIFUNCTION(RWSTRING("RtGCondAllocateVertices"));

    RWASSERT (num > 0);
    geometryList->vertices = (RtGCondVertex*)RwMalloc(sizeof(RtGCondVertex) * num,
           rwMEMHINTDUR_FUNCTION | rwID_GEOMCONDPLUGIN | rwMEMHINTFLAG_RESIZABLE);
    if (geometryList->vertices == NULL)
    {
        geometryList->numVertices = 0;
        RWRETURN(FALSE);
    }

    geometryList->numVertices = num;

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgcond
 * \ref RtGCondFreePolygons destroys the polygons in the geometry list.
 *
 * \param geometryList a pointer to the list of geometry
 *
 * \see RtGCondAllocatePolygons
 */
void
RtGCondFreePolygons(RtGCondGeometryList* geometryList)
{
    RWAPIFUNCTION(RWSTRING("RtGCondFreePolygons"));

    RwFree(geometryList->polygons);
    geometryList->polygons = NULL;
    geometryList->numPolygons = 0;

    RWRETURNVOID();
}

/**
 * \ingroup rtgcond
 * \ref RtGCondAllocatePolygons allocates space for a number of polygons
 *
 * \param geometryList a pointer to the list of geometry
 * \param num the number of polygons to allocate
 *
 * \return TRUE if successful, FALSE otherwise
 *
 * \see RtGCondAllocateVertices
 * \see RtGCondFreePolygons
 */
RwBool
RtGCondAllocatePolygons(RtGCondGeometryList* geometryList, RwInt32 num)
{
    RWAPIFUNCTION(RWSTRING("RtGCondAllocatePolygons"));

    RWASSERT (num > 0);
    geometryList->polygons = (RtGCondPolygon*)RwMalloc(sizeof(RtGCondPolygon) * num,
                 rwID_GEOMCONDPLUGIN | rwMEMHINTDUR_EVENT | rwMEMHINTFLAG_RESIZABLE);
    if (geometryList->polygons == NULL)
    {
        geometryList->numPolygons = 0;
        RWRETURN(FALSE);
    }
    
    geometryList->numPolygons = num;

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgcond
 * \ref RtGCondFreeIndices destroys the indices of the given polygon.
 *
 * \param polygon a pointer to the polygon
 *
 * \see RtGCondAllocateIndices
 */
void
RtGCondFreeIndices(RtGCondPolygon* polygon)
{
    RWAPIFUNCTION(RWSTRING("RtGCondFreeIndices"));

    RWASSERT(polygon);
    RWASSERT(polygon->indices);
    RWASSERT(polygon->numIndices);

    RwFree(polygon->indices);
    polygon->indices = NULL;
    polygon->numIndices = 0;

    RWRETURNVOID();
}

/**
 * \ingroup rtgcond
 * \ref RtGCondAllocateIndices allocates space for a number of indices of a
 * polygon.
 *
 * \param polygon a pointer to the polygon
 * \param num the number of indices to allocate
 *
 * \return TRUE if successful, FALSE otherwise
 *
 * \see RtGCondReallocateIndices
 * \see RtGCondFreeIndices
 */
RwBool
RtGCondAllocateIndices(RtGCondPolygon* polygon, RwInt32 num)
{
    RWAPIFUNCTION(RWSTRING("RtGCondAllocateIndices"));

    RWASSERT (num > 0);
    polygon->indices = (RwInt32*)RwMalloc(sizeof(RwInt32) * num,
                        rwMEMHINTDUR_EVENT | rwID_GEOMCONDPLUGIN);
    if (polygon->indices == NULL)
    {
        polygon->numIndices = 0;
        RWRETURN(FALSE);
    }

    polygon->numIndices = num;

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgcond
 * \ref RtGCondReallocateIndices reallocates space for a number of indices of a
 * polygon.
 *
 * \param polygon a pointer to the polygon
 * \param num the number of indices to allocate
 *
 * \return TRUE if successful, FALSE otherwise
 *
 * \see RtGCondAllocateIndices
 */
RwBool
RtGCondReallocateIndices(RtGCondPolygon* polygon, RwInt32 num)
{
    RwInt32* newIndices;
    RwInt32 i;

    RWAPIFUNCTION(RWSTRING("RtGCondReallocateIndices"));

    if (num > 0)
    {
        newIndices = (RwInt32*)RwMalloc(sizeof(RwInt32) * num,
                     rwMEMHINTDUR_EVENT | rwID_GEOMCONDPLUGIN);
        if (newIndices == NULL)
        {
            polygon->numIndices = 0;
            RWRETURN(FALSE);
        }

        for (i = 0; i < RWMIN(polygon->numIndices, num); i++)
        {
            newIndices[i] = polygon->indices[i];
        }
        if (polygon->indices != NULL)
        {
            RwFree(polygon->indices);
        }
        polygon->indices = newIndices;
        polygon->numIndices = num;
    }
    else
    {
        if (polygon->indices != NULL)
        {
            RwFree(polygon->indices);
            polygon->indices = NULL;
            polygon->numIndices = 0;
        }
    }

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgcond
 * \ref RtGCondReallocateVertices reallocates space for a number of vertices.
 *
 * \param geometryList a pointer to the list of geometry
 * \param num the number of vertices to reallocate
 *
 * \return TRUE if successful, FALSE otherwise
 *
 * \see RtGCondReallocateIndices
 * \see RtGCondReallocatePolygons
 * \see RtGCondAllocateVertices
 */
RwBool
RtGCondReallocateVertices(RtGCondGeometryList* geometryList, RwInt32 num)
{
    RWAPIFUNCTION(RWSTRING("RtGCondReallocateVertices"));

    RWASSERT(num > 0);

    geometryList->numVertices = num;
    geometryList->vertices = (RtGCondVertex*)RwRealloc(geometryList->vertices,
        sizeof(RtGCondVertex) * num, rwMEMHINTDUR_EVENT | rwID_GEOMCONDPLUGIN |
        rwMEMHINTFLAG_RESIZABLE);
    if (geometryList->vertices == NULL)
    {
        geometryList->numVertices = 0;
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}

/**
 * \ingroup rtgcond
 * \ref RtGCondReallocatePolygons reallocates space for a number of polygons.
 *
 * \param geometryList a pointer to the list of geometry
 * \param num the number of polygons to reallocate
 *
 * \return TRUE if successful, FALSE otherwise
 *
 * \see RtGCondReallocateIndices
 * \see RtGCondReallocateVertices
 * \see RtGCondAllocatePolygons
 */
RwBool
RtGCondReallocatePolygons(RtGCondGeometryList* geometryList, RwInt32 num)
{
    RtGCondPolygon* newPolygons;
    RwInt32 i, j;

    RWAPIFUNCTION(RWSTRING("RtGCondReallocatePolygons"));

    RWASSERT(num > 0);

    newPolygons = (RtGCondPolygon*)RwMalloc(sizeof(RtGCondPolygon) * num,
      rwMEMHINTDUR_EVENT | rwID_GEOMCONDPLUGIN | rwMEMHINTFLAG_RESIZABLE);
    if (newPolygons == NULL)
    {
        geometryList->numPolygons = 0;
        RWRETURN(FALSE);
    }

    for (i = 0; i < RWMIN(geometryList->numPolygons, num); i++)
    {
        newPolygons[i] = geometryList->polygons[i];
        for (j = 0; j < geometryList->polygons[i].numIndices; j++)
        {
            newPolygons[i].indices[j] = geometryList->polygons[i].indices[j];
        }
    }
    if (geometryList->polygons != NULL)
    {
        RtGCondFreePolygons(geometryList);
    }
    geometryList->polygons = newPolygons;
    geometryList->numPolygons = num;

    RWRETURN(TRUE);
}
/**
 * \ingroup rtgcond
 * \ref RtGCondParametersSet is used to set up a list of geometry conditioning
 * parameters used by the provided geometry conditioning pipelines.
 *
 * \param gcParams a pointer to \ref RtGCondParameters
 *
 * \see RtGCondParametersGet
 * \see RtGCondParametersInit
 * \see RtGCondFixAndFilterGeometryPipeline
 */
void
RtGCondParametersSet(RtGCondParameters* gcParams)
{
    RWAPIFUNCTION(RWSTRING("RtGCondParametersSet"));

    GCParams = gcParams;

    RWRETURNVOID();
}

/**
 * \ingroup rtgcond
 * \ref RtGCondParametersGet is used to get a list of geometry conditioning
 * parameters used by the provided geometry conditioning pipelines.
 *
 * \return a pointer to \ref RtGCondParameters
 *
 * \see RtGCondParametersSet
 * \see RtGCondParametersInit
 * \see RtGCondFixAndFilterGeometryPipeline
 */
RtGCondParameters*
RtGCondParametersGet(void)
{
    RWAPIFUNCTION(RWSTRING("RtGCondParametersGet"));

    RWRETURN(GCParams);
}

/**
 * \ingroup rtgcond
 * \ref RtGCondParametersInit is used to initialize a list of geometry conditioning
 * parameters used by the provided geometry conditioning pipelines.
 *
 * \param gcParams a pointer to \ref RtGCondParameters
 *
 * \see RtGCondParametersSet
 * \see RtGCondParametersGet
 * \see RtGCondFixAndFilterGeometryPipeline
 */
void
RtGCondParametersInit(RtGCondParameters* gcParams)
{
    RwInt32 i;

    RWAPIFUNCTION(RWSTRING("RtGCondParametersInit"));

    gcParams->flags                 = rtGCONDNORMALS | rtGCONDTEXTURES |
                                      rtGCONDPRELIT;
    gcParams->weldThreshold         = (RwReal)0.001;
    gcParams->angularThreshold      = (RwReal)1.0;
    gcParams->uvThreshold           = (RwReal)(0.5/128.0);
    gcParams->preLitThreshold       = (RwReal)(2.0/256.0);
    gcParams->areaThreshold         = (RwReal)0.0;
    gcParams->uvLimitMin            = (RwReal)-16.0;
    gcParams->uvLimitMax            = (RwReal)16.0;
    gcParams->texelLimits           = FALSE;
    for (i = 0; i < rwMAXTEXTURECOORDS; i++)
    {
        gcParams->textureMode[i]    = rwTEXTUREADDRESSWRAP;
    }
    gcParams->polyNormalsThreshold  = (RwReal)0.01;
    gcParams->polyUVsThreshold      = (RwReal)0.01;
    gcParams->polyPreLitsThreshold  = (RwReal)0.01;
    gcParams->decimationMode        = 0;
    gcParams->decimationPasses      = 5;
    gcParams->convexPartitioningMode = rtWINGEDGEPARTITIONTACK;

    RWRETURNVOID();
}

/**
 * \ingroup rtgcond
 * \ref RtGCondSetUserdataCallBacks sets user callbacks which are called when
 * cloning, interpolation, splitting and destroying operation are processed.
 *
 * \param cloneVertexUserdataCB the callback for cloning
 * \param interpVertexUserdataCB the callback for interpolating
 * \param splitPolygonUserdataCB the callback for splitting
 * \param destroyVertexUserdataCB the callback for destroying vertices
 * \param destroyPolygonUserdataCB the callback for destroying polygons
 */
void
RtGCondSetUserdataCallBacks(RtGCondCloneVertexUserdataCallBack cloneVertexUserdataCB,
                            RtGCondInterpVertexUserdataCallBack interpVertexUserdataCB,
                            RtGCondSplitPolygonUserdataCallBack splitPolygonUserdataCB,
                            RtGCondDestroyVertexUserdataCallBack destroyVertexUserdataCB,
                            RtGCondDestroyPolygonUserdataCallBack destroyPolygonUserdataCB)
{
    RWAPIFUNCTION(RWSTRING("RtGCondSetUserdataCallBacks"));

    GCUserDataCallBacks.destroyVertexUserdata =
        destroyVertexUserdataCB;
    GCUserDataCallBacks.cloneVertexUserdata =
        cloneVertexUserdataCB;
    GCUserDataCallBacks.interpVertexUserdata =
        interpVertexUserdataCB;
    GCUserDataCallBacks.destroyPolygonUserdata =
        destroyPolygonUserdataCB;
    GCUserDataCallBacks.splitPolygonUserdata =
        splitPolygonUserdataCB;

    RWRETURNVOID();
}

/**
 * \ingroup rtgcond
 * \ref RtGCondMaterialGetUVLimits is used to get already pre-calculated per
 * material per UV set texel based UV limits. There are two ways UV limiting
 * can be done. The first is using the standard UV limits which are texture
 * independent, and the second is to specify the limits in texels, in which
 * case UV limits are calculated by: limitUMin = minUvTexelLimit / textureWidth
 * etc. Depending on the texelLimits parameter \ref RtGCondLimitUVsPipelineNode
 * function will use the limits passed to it by the parameters or use
 * this function to get the limits stored in the materials.
 *
 * \param mat from which to get the texel based UV limits
 * \param uvSet for which to query the values
 * \param uMin the pointer to the value to receive the min limit in U direction
 * \param uMax the pointer to the value to receive the max limit in U direction
 * \param vMin the pointer to the value to receive the min limit in V direction
 * \param vMax the pointer to the value to receive the max limit in V direction
 *
 * \see RtGCondMaterialSetUVLimits, RtGCondLimitUVsPipelineNode
 */
void
RtGCondMaterialGetUVLimits(RpMaterial *mat, RwInt32 uvSet,
                           RwReal *uMin, RwReal *uMax,
                           RwReal *vMin, RwReal *vMax)
{
    RWAPIFUNCTION(RWSTRING("RtGCondMaterialGetUVLimits"));

    *uMin = GCONDGETCONSTMATERIALDATA(mat)->uvLimits[uvSet].u.min;
    *uMax = GCONDGETCONSTMATERIALDATA(mat)->uvLimits[uvSet].u.max;
    *vMin = GCONDGETCONSTMATERIALDATA(mat)->uvLimits[uvSet].v.min;
    *vMax = GCONDGETCONSTMATERIALDATA(mat)->uvLimits[uvSet].v.max;

    RWRETURNVOID();
}

/**
 * \ingroup rtgcond
 * \ref RtGCondMaterialSetUVLimits is used to set already pre-calculated per
 * material per UV set texel based UV limits. There are two ways UV limiting
 * can be done. The first is using the standard UV limits which are texture
 * independent, and the second is to specify the limits in texels, in which
 * case UV limits are calculated by: limitUMin = minUvTexelLimit / textureWidth
 * etc. Depending on the texelLimits parameter \ref RtGCondLimitUVsPipelineNode
 * function will use the limits passed to it by the parameters or get the
 * limits stored in the materials.
 *
 * \param mat to which to store the texel based UV limits
 * \param uvSet for which to store the values
 * \param uMin value of the min limit in U direction to store
 * \param uMax value of the max limit in U direction to store
 * \param vMin value of the min limit in V direction to store
 * \param vMax value of the max limit in V direction to store
 *
 * \see RtGCondMaterialGetUVLimits, RtGCondLimitUVsPipelineNode
 */
void
RtGCondMaterialSetUVLimits(RpMaterial *mat, RwInt32 uvSet,
                           RwReal uMin, RwReal uMax,
                           RwReal vMin, RwReal vMax)
{
    RWAPIFUNCTION(RWSTRING("RtGCondMaterialSetUVLimits"));

    GCONDGETMATERIALDATA(mat)->uvLimits[uvSet].u.min = uMin;
    GCONDGETMATERIALDATA(mat)->uvLimits[uvSet].u.max = uMax;
    GCONDGETMATERIALDATA(mat)->uvLimits[uvSet].v.min = vMin;
    GCONDGETMATERIALDATA(mat)->uvLimits[uvSet].v.max = vMax;

    RWRETURNVOID();
}

static void *
GCondMaterialConstructor(void *object,
                         RwInt32 offset __RWUNUSED__,
                         RwInt32 size   __RWUNUSED__ )
{
    RwInt32             i;
    GCondMaterialData   *matData = GCONDGETMATERIALDATA(object);

    RWFUNCTION(RWSTRING("GCondMaterialConstructor"));

    for (i = 0; i < rwMAXTEXTURECOORDS; i++)
    {
        matData->uvLimits[i].u.min = -RwRealMAXVAL;
        matData->uvLimits[i].u.max = RwRealMAXVAL;
        matData->uvLimits[i].v.min = -RwRealMAXVAL;
        matData->uvLimits[i].v.max = RwRealMAXVAL;
    }

    RWRETURN(object);
}

static void *
GCondMaterialCopy(void *dstObject,
                  const void *srcObject,
                  RwInt32 offset __RWUNUSED__,
                  RwInt32 size   __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("GCondMaterialCopy"));

   *GCONDGETMATERIALDATA(dstObject) = *GCONDGETCONSTMATERIALDATA(srcObject);

    RWRETURN(dstObject);
}

/**
 * \ingroup rtgcond
 * \ref RtGCondInitialize is used to initialize the geometry conditioning toolkit
 * with the RenderWare Graphics system. The toolkit must be initialized after
 * initializing the system with \ref RwEngineInit and calling RpWorldPluginAttach,
 * and before opening the system with \ref RwEngineOpen.
 *
 * \return \ref RwBool, TRUE if successful, FALSE if an error occurs.
 *
 * \see RwEngineInit
 * \see RwEngineOpen
 */
RwBool
RtGCondInitialize(void)
{
    RWAPIFUNCTION(RWSTRING("RtGCondInitialize"));

    /* Extend material with uv limit information. */
    GCondMaterialDataOffset =
        RpMaterialRegisterPlugin(sizeof(GCondMaterialData),
                                 rwID_GEOMCONDPLUGIN,
                                 GCondMaterialConstructor,
                                 NULL,
                                 GCondMaterialCopy);
    RWASSERT(0 < GCondMaterialDataOffset);
    if (GCondMaterialDataOffset <= 0)
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}
