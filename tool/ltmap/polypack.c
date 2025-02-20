/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   polypack.c                                                 -*
 *-                                                                         -*
 *-  Purpose :   RtLtMap toolkit                                            -*
 *-                                                                         -*
 *===========================================================================*/

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/

/* Needed for memset */
#include "string.h"

#include "rwcore.h"
#include "rpworld.h"
#include "rprandom.h"

#include "rpcollis.h"
#include "rtbary.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rppvs.h"
/* We need to use the non-INCGEN-d version of rpltmap.h */
#include "../../plugin/ltmap/rpltmap.h"
#include "rtltmap.h"

#include "sample.h"
#include "polypack.h"
#include "ltmapvar.h"

#ifdef SHOWLIGHTMAPUSAGE
#   include "image.h"
#endif


/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local defines ---------------------------------------------------------*
 *===========================================================================*/

#define rwXPLANE_LtMap 0
#define rwYPLANE_LtMap 1
#define rwZPLANE_LtMap 2


/* THIS MACROISED TO KEEP THE CODE CLEAR AND THE COMMENTS IN A SINGLE
 * LOCATION:
 */
#define TRISETUPSLIVERTESTMACRO(tri, sliver)                                     \
MACRO_START                                                                 \
{                                                                           \
    RWASSERT(NULL != uvObjectData->triangles);                              \
    newPoly.mat = rpMaterialListGetMaterial(                                \
        uvObjectData->matList,                                              \
        uvObjectData->triangles[tri].matIndex +                             \
        uvObjectData->matListWindowBase);                                   \
    newPoly.vertIndex[0] = uvObjectData->triangles[tri].vertIndex[0];       \
    newPoly.vertIndex[1] = uvObjectData->triangles[tri].vertIndex[1];       \
    newPoly.vertIndex[2] = uvObjectData->triangles[tri].vertIndex[2];       \
                                                                            \
    /* Find the area of the triangle */                                     \
    RwV3dSub(&(edge[0]),                                                    \
             &(uvObjectData->vertices[newPoly.vertIndex[2]]),               \
             &(uvObjectData->vertices[newPoly.vertIndex[1]]));              \
    RwV3dSub(&(edge[1]),                                                    \
             &(uvObjectData->vertices[newPoly.vertIndex[0]]),               \
             &(uvObjectData->vertices[newPoly.vertIndex[1]]));              \
                                                                            \
    RwV3dCrossProduct(&normal, &(edge[0]), &(edge[1]));                     \
    length  = RwV3dDotProduct(&normal, &normal);                            \
    length *= worldData->lightMapDensity *                                  \
            RtLtMapMaterialGetLightMapDensityModifier(newPoly.mat);         \
    length *= worldData->lightMapDensity *                                  \
            RtLtMapMaterialGetLightMapDensityModifier(newPoly.mat);         \
                                                                            \
/* TODO[3][ABK][ABI]: PUT IN A TEST SEARCHING FOR SLIVERS THAT END UP NOT ATTACHED TO OTHER POLYGONS...     */  \
/*        FREE-FLOATING SLIVERS ARE *BAD*, YOU SHOULD NEVER HAVE SLIVERS THAT AREN'T NEIGHBOURING           */  \
/*        NON-SLIVERS... THEIR PURPOSE IS MERELY TO PREVENT CRACKING, TO JOIN UP NON-SLIVERS                */  \
/*         SPECIFICALLY, FREE-FLOATING SLIVERS CAN SCREW UP LIGHTING... THEY WILL PROBABLY HAVE A DUFF      */  \
/*        NORMAL SO WHEN THEY'RE LIT THEY'LL GET A BAD COLOUR (RATHER THAN BEING FILLED IN BY DILATE)       */  \
/*        WHICH WILL PRODUCE JUST THE SORT OF UGLY ARTIFACT THAT CRACK-FILLING SLIVERS ARE MEANT TO AVOID!  */  \
                                                                                                                \
/* TODO[3][ABK][ABI]: WAIT, I THINK *THIS* IS WHAT GENERATES THOSE HORRIBLE SHIMMERING SLIVERS IN           */  \
/*        LEVELS. WE'RE NOT GENERATING SENSIBLE UVS FOR SLIVERS AND HENCE THEY HAVE RANDOM UVS THAT         */  \
/*        CROSS OVER WHATEVER IN THE LIGHTMAP! AS IT SAYS ABOVE, WE NEED TO JOIN SLIVERS TO                 */  \
/*        ADJACENT TRIANGLES AND SHARE UVS. IF THERE ARE NONE, ASSERT I GUESS... OR SPEND EXTRA             */  \
/*        EFFORT FINDING NEARBY NON-SLIVER POLYGONS WHOSE UVS YOU CAN STEAL (DO LINE TESTS FROM THE         */  \
/*        SLIVER VERTICES TO THE NEAREST NON-SLIVER POLYGONS (TRY LOTS OF DIRECTIONS, INCLUDING THE         */  \
/*        COORDINATE AXES) AND GRAB THE UVS FOUND AT THE CLOSEST COLLISION IN EACH CASE... OF COURSE        */  \
/*        THIS MIGHT GET UVS FROM DIFFERENT POLYS SO I GUESS IF SO THEN CHOOSE THE CLOSEST RESULT           */  \
/*        AND DUPLICATE THAT TO THE SLIVER'S OTHER VERTS                                                    */  \
/*         UPDATE: WE'RE NOW ASSIGNING UVS TO SLIVERS, JUST AS FOR OTHER TRIANGLES, ON THESE BASES:         */  \
/*         1. THE MODELLING PACKAGE CAN PROBABLY WORK OUT THE NORMALS OF SLIVERS SINCE IT CREATES THEM      */  \
/*         2. WE SKIP SLIVERS AND FILL THEM DURING DILATE ANYWAY                                            */  \
/*         3. FREE-FLOATING SLIVERS ARE BIZARRE AND SHOULD REALLY BE DEALT WITH BY ASSERTS                  */  \
/*         4. INITIALIZING SLIVER UVs WRONGLY CAN'T BE WORSE THAN LEAVING THEM UNINITIALIZED!               */  \
    /* Allow sliver-rejection (depending on where this is used) */                                              \
    sliver = (length < _rpLtMapGlobals.sliverAreaThreshold);                                                    \
}                                                                                                               \
MACRO_STOP


/*===========================================================================*
 *--- Local types -----------------------------------------------------------*
 *===========================================================================*/



/*===========================================================================*
 *--- Local variables -------------------------------------------------------*
 *===========================================================================*/


/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/


/****************************************************************************
 LtMapCompareVertex
 */
#if (defined(_WINDOWS))
static int __cdecl
#else
static int
#endif
LtMapCompareVertex(const void *a, const void *b)
{
    const RwV3d * const vert1 = *(const RwV3d * const *)a;
    const RwV3d * const vert2 = *(const RwV3d * const *)b;
    RwReal sortDist1, sortDist2;
    int result = 0;

    RWFUNCTION(RWSTRING("LtMapCompareVertex"));

    sortDist1 = RwV3dDotProduct(vert1, &rtLtMapGlobals.sortVector);
    sortDist2 = RwV3dDotProduct(vert2, &rtLtMapGlobals.sortVector);
    if (sortDist2 > sortDist1)
    {
        result =  1;
    }
    else if (sortDist1 > sortDist2)
    {
        result = -1;
    }

    RWRETURN(result);
}

/****************************************************************************
 TopologyDestroy
 */
static void
TopologyDestroy(LtMapTopology *topology)
{
    LtMapEdgeTriList *curEdgeTriList, *nextEdgeTriList;
    LtMapVertTriList *curVertTriList, *nextVertTriList;
    RwUInt32 i;

    RWFUNCTION(RWSTRING("TopologyDestroy"));

    /* Destroy the welded and unwelded lists of "triangles sharing a vertex" */
    for (i = 0;i < topology->numVerts;i++)
    {
        curVertTriList = topology->weldvertTriLists[i].next;

        /* Destroy any additional vertlist structures
         * (for extremely popular vertices :) )
         */
        while (curVertTriList != NULL)
        {
            nextVertTriList = curVertTriList->next;
            RwFree(curVertTriList);
            curVertTriList = nextVertTriList;
        }
    }
    RwFree(topology->weldvertTriLists);

    for (i = 0;i < topology->numVerts;i++)
    {
        curVertTriList = topology->vertTriLists[i].next;

        /* Destroy any additional vertlist structures
         * (for extremely popular vertices :) )
         */
        while (curVertTriList != NULL)
        {
            nextVertTriList = curVertTriList->next;
            RwFree(curVertTriList);
            curVertTriList = nextVertTriList;
        }
    }
    RwFree(topology->vertTriLists);

    /* Destroy the 'welded vertex' LUT */
    RwFree (topology->wVerts);

    /* Destroy the lists of "triangles sharing an edge" */
    for (i = 0;i < topology->numEdges;i++)
    {
        curEdgeTriList = topology->edgeTriLists[i].next;

        /* Destroy any additional edgesharelist structures (for extremely
         * popular edges :) )
         */
        while (curEdgeTriList != NULL)
        {
            nextEdgeTriList = curEdgeTriList->next;
            RwFree(curEdgeTriList);
            curEdgeTriList = nextEdgeTriList;
        }
    }
    RwFree(topology->edgeTriLists);

    /* Free the lists of "edges used by a triangle" */
    RwFree(topology->triEdgeLists);

    RwFree(topology);

    RWRETURNVOID();
}


/****************************************************************************
 VertsEqual
 */
static RwBool
VertsEqual(RwV3d *vert1, RwV3d *vert2)
{
    RwBool result = FALSE;
    RwV3d  delta;

    RWFUNCTION(RWSTRING("VertsEqual"));

    RwV3dSub(&delta, vert1, vert2);
    if (RwV3dDotProduct(&delta, &delta) < _rpLtMapGlobals.vertexWeldThreshold)
    {
        result = TRUE;
    }

    RWRETURN(result);
}


/****************************************************************************
 CreateVertexToTriangleLists
 */
static void
CreateVertexToTriangleLists (LtMapTopology *topology, RpTriangle *triangles)
{
    RwUInt32            i, j, k;
    RxVertexIndex       vertIndices[3], weldvertIndices [3];
    LtMapVertTriList   *curVertTriList, *newVertTriList;
    RwUInt32            weld;

    RWFUNCTION (RWSTRING ("CreateVertexToTriangleLists"));

    /* Build a list of pointers, for each vertex, to all triangles referencing
     * that vertex
     */
    for (i = 0;i < topology->numTris;i++)
    {
        /* We copy these explicitly because the RxVertexIndex size
         * is different from the vertIndex[] size for OpenGL! */
        for (j = 0;j < 3;j++)
        {
                vertIndices[j] =                   triangles[i].vertIndex[j];
            weldvertIndices[j] = topology->wVerts [triangles[i].vertIndex[j]];
        }

        /* NOTE: this allows a vertex to reference a triangle more
         *       than once (if the triangle is degenerate). */
        for (weld = 0; weld < 2; weld++)
        {
            for (j = 0;j < 3;j++)
            {
                /* Do unwelded tri list then welded tri list */
                if (weld == 0)
                {
                    curVertTriList = &(topology->    vertTriLists[    vertIndices[j]]);
                }
                else
                {
                    curVertTriList = &(topology->weldvertTriLists[weldvertIndices[j]]);
                }

                /* Skip to the end of the vert's chain of trilists */
                while (curVertTriList->next != NULL)
                {
                    curVertTriList = curVertTriList->next;
                }
                /* Find the first free slot in the trilist */
                for (k = 0; k < VERTTRILISTSIZE; k++)
                {
                    if (0xFFFFFFFF == curVertTriList->list[k])
                        break;
                }
                /* If all slots are full, add another trilist to the chain */
                if (VERTTRILISTSIZE == k)
                {
                    newVertTriList = (LtMapVertTriList *) RwMalloc(sizeof(LtMapVertTriList),
                                                       rwID_LTMAPPLUGIN | rwMEMHINTDUR_EVENT);
                    RWASSERT(NULL != newVertTriList);

                    memset(newVertTriList, 0xFFFFFFFF, sizeof(LtMapVertTriList));
                    newVertTriList->next = NULL;
                    curVertTriList->next = newVertTriList;
                    curVertTriList = newVertTriList;
                    k = 0;
                }
                /* Add the tri to the vert's list */
                curVertTriList->list[k] = i;
            }
        }
    }

    RWRETURNVOID ();
}

/****************************************************************************
 WeldTopologyVertices
 */
static void
WeldTopologyVertices (LtMapTopology *topology, RwV3d *vertices,
                      RwTexCoords *texCoords, RwBool posSharing)
{
    RwUInt32           i, j;
    RwBBox             objBox;
    RwV3d              **vertexPointers;

    RWFUNCTION (RWSTRING ("WeldTopologyVertices"));

    /* Now, depending on posSharing, take into account sharing on the basis of
     * vertex position as well as explicit links by triangle indices. We sort
     * the vertices on the basis of position, along the longest diagonal of the
     * object. We then run along this list, finding groups of vertices at the
     * 'same' position and we then combine their weldvertTriLists. */

    vertexPointers = (RwV3d **) RwMalloc(topology->numVerts * sizeof(RwV3d *),
                                rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION);
    RWASSERT(NULL != vertexPointers);

    RwBBoxInitialize(&objBox, &vertices[0]);
    for (i = 0;i < topology->numVerts;i++)
    {
        RwBBoxAddPoint(&objBox, &vertices[i]);
        vertexPointers[i] = &(vertices[i]);
    }

    /* Sort the vertices along the longest diagonal of the object */
    RwV3dSub(&rtLtMapGlobals.sortVector, &(objBox.sup), &(objBox.inf));
    qsort(vertexPointers, topology->numVerts, sizeof(RwV3d *), LtMapCompareVertex);

    topology->wVerts = (RwUInt32 *) RwMalloc (topology->numVerts * sizeof (RwUInt32),
                                rwID_LTMAPPLUGIN | rwMEMHINTDUR_FRAME);
    RWASSERT (topology->wVerts);

    for (i = 0;i < topology->numVerts;i++)
    {
        RwV3d   *testVertex = vertexPointers[i];
        RwUInt32 numShares  = 0;
        RwUInt32 vertIndex;

        do
        {
            numShares++;
            if ((i + numShares) >= topology->numVerts)
                break;
        }
        while (posSharing && FALSE != VertsEqual(vertexPointers[i + numShares], testVertex));

        /* Point every welded vertex on the shared list to the first
         * vertex on the unwelded list */
        for (j = 0;j < numShares;j++)
        {
            RwUInt32 weldvertIndex = vertexPointers [i + j] - vertices;
            RwUInt32 matchAxis;
            RwUInt8  curAxis, existingAxis;
            RwReal   u;

            /* Get the axis of this vertex to (perhaps) weld */
            u = texCoords [weldvertIndex].u;
            curAxis = RwFastRealToUInt32(10.0001f*u);
            RWASSERT(curAxis <= 5);

            /* Find first weldable vertex that has the same axis, as
             * vertices with different axes should not be welded */
            vertIndex = 0; /* Get rid of compiler warning */
            for (matchAxis = 0; matchAxis <= j; matchAxis++)
            {
                vertIndex = vertexPointers [i + matchAxis] - vertices;
                u = texCoords [vertIndex].u;
                existingAxis = RwFastRealToUInt32(10.0001f*u);
                RWASSERT(existingAxis <= 5);

                if (curAxis == existingAxis)
                    break; /* Found match with other vertex, or self */
            }
            RWASSERT (matchAxis != (j + 1)); /* Should have at least matched self */

            topology->wVerts [weldvertIndex] = vertIndex;
        }

        i += (numShares - 1);
    }
    RwFree(vertexPointers);

    RWRETURNVOID ();
}

/****************************************************************************
 CalculateEdgeLists
 */
static void
CalculateEdgeLists (LtMapTopology *topology, RpTriangle *triangles)
{
    RwUInt32            curTri, thisSide, k;
    RwUInt8             mod3[] = {0, 1, 2, 0, 1};
    RxVertexIndex       vertIndices[3];
    RwUInt32            numEdges;
    RwUInt32            curEntryA;
    RwUInt32            edgeNum, eachEdge;
    RwUInt32            *wVerts = topology->wVerts; /* Just to look nice */

    RWFUNCTION (RWSTRING ("CalculateEdgeLists"));

    /* For each triangle, create a list of (up to 3) pointers to edges
     * (creating the edges as you go) and pointers back from the edges to the
     * triangles */
    numEdges = 0;
    for (curTri = 0;curTri < topology->numTris;curTri++)
    {
        /* We copy these explicitly because the RxVertexIndex size
         * is different from the vertIndex[] size for OpenGL! */
        for (thisSide = 0;thisSide < 3;thisSide++)
        {
            vertIndices[thisSide] = wVerts [triangles[curTri].vertIndex[thisSide]];
        }

        /* Iterate over the edges of the triangle */
        for (thisSide = 0; thisSide < 3; thisSide++)
        {
            LtMapEdgeTriList *curEdgeTriList, *newEdgeTriList;
            RxVertexIndex vertIndexA, vertIndexB;

            /* Get an edge (don't care if it's degenerate) */
            vertIndexA = vertIndices [      thisSide + 0 ];
            vertIndexB = vertIndices [mod3 [thisSide + 1]];

            /* Test that edge has not already been created by another tri */
            edgeNum = 0xFFFFFFFF;
            for (eachEdge = 0; eachEdge < numEdges; eachEdge++)
            {
                RwUInt32 vertInd0, vertInd1;
                LtMapEdgeTriList *curEdgeTriList = &topology->edgeTriLists [eachEdge];

                curEntryA = 0;
                while (curEdgeTriList != NULL && curEdgeTriList->list [curEntryA] != 0xFFFFFFFF)
                {
                    /* Get the vertex indices of the appropriate side of the triangle */
                    RpTriangle *thisTri = &triangles [curEdgeTriList->list [curEntryA]];
                    vertInd0 = wVerts [thisTri->vertIndex [mod3 [curEdgeTriList->side [curEntryA] + 0]]];
                    vertInd1 = wVerts [thisTri->vertIndex [mod3 [curEdgeTriList->side [curEntryA] + 1]]];

                    /* Do these indices match the current edge being generated? */
                    if ((vertInd0 == vertIndexA && vertInd1 == vertIndexB) ||
                        (vertInd1 == vertIndexA && vertInd0 == vertIndexB))
                    {
                        edgeNum = eachEdge; /* Found an existing edge! */
                    }

                    /* Next. */
                    if (++ curEntryA == EDGETRILISTSIZE)
                    {
                        curEntryA = 0;
                        curEdgeTriList = curEdgeTriList->next;
                    }
                }
            }

            /* Create new edge? */
            if (edgeNum == 0xFFFFFFFF)
            {
                edgeNum = numEdges ++;
            }

            curEdgeTriList = &topology->edgeTriLists [edgeNum];
            while (curEdgeTriList->next != NULL)
                curEdgeTriList = curEdgeTriList->next;

            /* Find the first free slot in the trilist */
            for (k = 0; k < EDGETRILISTSIZE ; k++)
            {
                if (0xFFFFFFFF == curEdgeTriList->list[k])
                    break;
            }
            /* If all slots are full, add another trilist to the chain */
            if (k == EDGETRILISTSIZE)
            {
                newEdgeTriList = (LtMapEdgeTriList *) RwMalloc(sizeof(LtMapEdgeTriList),
                                                   rwID_LTMAPPLUGIN | rwMEMHINTDUR_EVENT);
                RWASSERT(NULL != newEdgeTriList);

                memset(newEdgeTriList, 0xFFFFFFFF, sizeof(LtMapEdgeTriList));
                newEdgeTriList->next = NULL;
                curEdgeTriList->next = newEdgeTriList;
                curEdgeTriList = newEdgeTriList;
                k = 0;
            }
            /* Add the tri to the edge's list */
            curEdgeTriList->list [k] = curTri;
            curEdgeTriList->side [k] = thisSide;

            topology->triEdgeLists [curTri].list [thisSide] = edgeNum;

        } /* For all edges */
    } /* For all triangles */

    /* We know how many edges there are now: */
    topology->numEdges = numEdges;

    RWRETURNVOID ();
}

/****************************************************************************
 TopologyCreate
 */
static LtMapTopology *
TopologyCreate (LtMapAllocUVObjectData *uvObjectData, RwBool posSharing)
{
    LtMapTopology      *topology;
    RwUInt32           i;
    
    RWFUNCTION(RWSTRING("TopologyCreate"));
    RWASSERT(NULL != uvObjectData->triangles);

    /* Create a new topology to fill */
    topology = (LtMapTopology *) RwMalloc(sizeof(LtMapTopology),
                          rwID_LTMAPPLUGIN | rwMEMHINTDUR_EVENT);
    RWASSERT(NULL != topology);
    topology->numTris  = uvObjectData->numTriangles;
    topology->numVerts = uvObjectData->numVertices;

    /* Work out the welded vertex positions */
    WeldTopologyVertices (topology, uvObjectData->vertices, uvObjectData->texCoords, posSharing);

    {
        /* Each vertex has a list of references to triangles */
        topology->vertTriLists = (LtMapVertTriList *)
            RwMalloc(topology->numVerts * sizeof(LtMapVertTriList),
            rwID_LTMAPPLUGIN | rwMEMHINTDUR_EVENT);
        RWASSERT(NULL != topology->vertTriLists);
        topology->weldvertTriLists = (LtMapVertTriList *)
            RwMalloc(topology->numVerts * sizeof(LtMapVertTriList),
            rwID_LTMAPPLUGIN | rwMEMHINTDUR_EVENT);
        RWASSERT(NULL != topology->weldvertTriLists);

        /* Clear the index list to "-1" and the next pointer to NULL */
        memset(topology->    vertTriLists, 0xFFFFFFFF, topology->numVerts * sizeof(LtMapVertTriList));
        memset(topology->weldvertTriLists, 0xFFFFFFFF, topology->numVerts * sizeof(LtMapVertTriList));
        for (i = 0; i < topology->numVerts; i++)
        {
            topology->    vertTriLists [i].next = NULL;
            topology->weldvertTriLists [i].next = NULL;
        }

        CreateVertexToTriangleLists (topology, uvObjectData->triangles);
    }

    {
        /* Each triangle has three references to edges */
        topology->triEdgeLists = (LtMapTriEdgeList *)
            RwMalloc(topology->numTris * sizeof(LtMapTriEdgeList),
                rwID_LTMAPPLUGIN | rwMEMHINTDUR_EVENT);
        RWASSERT(NULL != topology->triEdgeLists);

        /* Clear the tri->edge lists */
        memset(topology->triEdgeLists, 0xFFFFFFFF, topology->numTris * sizeof(LtMapTriEdgeList));

        /* And the edges refer back to the triangles */
        topology->edgeTriLists = (LtMapEdgeTriList *)
            RwMalloc(3*topology->numTris * sizeof(LtMapEdgeTriList),
            rwID_LTMAPPLUGIN | rwMEMHINTDUR_EVENT);
        RWASSERT(NULL != topology->edgeTriLists);
        memset(topology->edgeTriLists, 0xFFFFFFFF, 3 * topology->numTris * sizeof(LtMapEdgeTriList));
        for (i = 0; i < 3 * topology->numTris; i++)
        {
            topology->edgeTriLists [i].next = NULL;
        }
        CalculateEdgeLists (topology, uvObjectData->triangles);
    }

    RWRETURN(topology);
}

/****************************************************************************
 CullRect
 */

static void
CullRect(LtMapSlot *slot, RwUInt32 deadRect)
{
    RWFUNCTION( RWSTRING( "CullRect" ) );

#if (defined(SHOWLIGHTMAPUSAGE))
    {
        /* Clear wasted areas to a random bright red */
        RwRGBA ClearColour = {128, 0, 0, 255};
        ClearColour.red += 127 & RpRandom();
        ImageRectClear(rtLtMapGlobals.gLightMapUsageImage, &(slot->rects[deadRect]),
                       ClearColour);
    }
#endif /* (defined(SHOWLIGHTMAPUSAGE)) */
    slot->rects[deadRect] = slot->rects[slot->count - 1];
    slot->count--;

    RWRETURNVOID();
}

static RwUInt32
CullSmallestRect(LtMapSlot *slot, RwUInt32 exclude)
{
    RwUInt32 minArea;
    RwUInt32 smallest = 0, i = 0;

    RWFUNCTION( RWSTRING( "CullSmallestRect" ) );

    /* 'exclude' ensures we don't delete the current "bestRect"! */
    if (exclude == 0)
        smallest = 1;
    minArea = slot->rects[smallest].w * slot->rects[smallest].h;

    for (i = 0;i < slot->count;i++)
    {
        if ((RwUInt32)(slot->rects[i].w * slot->rects[i].h) < minArea)
        {
            if (i != exclude)
            {
                minArea = slot->rects[i].w * slot->rects[i].h;
                smallest = i;
            }
        }
    }
    /* "bestRect" may be moved by this deletion */
    if (exclude == (slot->count - 1))
        exclude = smallest;
    CullRect(slot, smallest);

    RWRETURN(exclude);
}

/****************************************************************************
 LtMapAllocRect
 */
static RwBool
LtMapAllocRect(LtMapPolySet *polySet, LtMapSlot *slot)
{
    RwUInt32 width, height, bestFit, i;
    RwInt32  bestRect;
    RwV2d *polySetSize = &polySet->size;
    RwInt32 polySetOrigin [2];
    RwRect  *oldRect, *newRect = (RwRect *)NULL;
    RwUInt32 bottomGap, rightGap;

    /* NOTE: in here, rectangles in the lightmap are integers and are such
     *       that if rect.x/y are zero and rect.w/h are 1 then the rectangle
     *       covers the top-left texel only. Ergo a rectangle covering the
     *       whole texture has w and h equal to texture width and height. */

    RWFUNCTION(RWSTRING("LtMapAllocRect"));
    RWASSERT(NULL != polySet);
    RWASSERT(NULL != slot);

    /* Caclulate the area needed to cover this polyset, taking into
     * account partial pixel coverage and an extra 1-texel boundary
     * (NOTE: we add 0.01f for the case where polySetSize->x/y is an
     *  exact integer - which happens a lot for Quake-engine levels.
     *  0.001f shouldn't lose precision for a 8192x8192 lightmap) */
    width  = 2 + RwFastRealToUInt32((RwReal)RwCeil(polySetSize->x + 0.001f));
    height = 2 + RwFastRealToUInt32((RwReal)RwCeil(polySetSize->y + 0.001f));

    /* Where does it fit best? */
    bestRect = -1;
    bestFit = slot->size;
    for (i = 0;i < slot->count;i++)
    {
        RwInt32 dw, dh;

        /* Too small? */
        dw = slot->rects[i].w - width;
        if (dw < 0)
            continue; 
        dh = slot->rects[i].h - height;
        if (dh < 0)
            continue;

        if ((RwUInt32)dw < bestFit)
        {
            bestFit = dw;
            bestRect = i;
        }
        if ((RwUInt32)dh < bestFit)
        {
            bestFit = dh;
            bestRect = i;
        }
    }

    /* did we find one? */
    if (bestRect < 0)
        RWRETURN (FALSE); /* Nope */
        
    oldRect = &(slot->rects[bestRect]);
    polySetOrigin[0] = oldRect->x;
    polySetOrigin[1] = oldRect->y;

    /* We have two choices for splitting - this simple decision
     * seems to work amazingly well (it tends to reject splits
     * that would create slivers and it results in a very square,
     * large free region to the bottom-right of the lightmap),
     * even better than something using aspect ratios... */
    rightGap    = oldRect->w - width;
    bottomGap   = oldRect->h - height;
    
    if (slot->count >= MAXRECTS)
    {
        /* Rect overflow - cull the smallest free rectangle (being careful
         * not to delete oldRect and to note if it's moved by the
         * deletion!) */
        /* TODO[6]: SHOULD USE AN RwSList, REALLY */
        bestRect = CullSmallestRect(slot, bestRect);
        oldRect = &(slot->rects[bestRect]);
    }
    newRect = &(slot->rects[slot->count]);
    *newRect = *oldRect;
    
    /* NOTE: The below can create zero-area triangles but they'll
     *       get deleted by CullSmallestRect() later if necessary.
     *       culling them in advance doesn't speed it up. */
    if (bottomGap < rightGap)
    {
        /* Make a vertical split */
        oldRect->x += width;
        oldRect->w -= width;
        newRect->y += height;
        newRect->h -= height;
        newRect->w  = width;
    }
    else
    {
        /* Make a horizontal split */
        oldRect->x += width;
        oldRect->w -= width;
        oldRect->h  = height;
        newRect->y += height;
        newRect->h -= height;
    }
    slot->count++;

#if (defined(SHOWLIGHTMAPUSAGE))
    /* Clear used rects to a random bright yellow */
    {
        RwRGBA usedColour = {128, 128, 0, 255};
        RwRect bob;
        bob.x = polySetOrigin[0];
        bob.y = polySetOrigin[1];
        bob.w = width;
        bob.h = height;
        usedColour.red += 127 & RpRandom();
        usedColour.green = usedColour.red;
        /* For debugging, it can be handy to show the boundaries between
         * polySets:
        usedColour.red   = RpRandom();
        usedColour.green = RpRandom();
        usedColour.blue  = RpRandom(); */
        ImageRectClear(rtLtMapGlobals.gLightMapUsageImage, &bob, usedColour);
    }
#endif /* (defined(SHOWLIGHTMAPUSAGE)) */

    /* Inset by 1 texel (packing pads the polySet with guard
     * texels) */
    polySet->uvOrigin.u = (RwReal) (polySetOrigin [0] + 1);
    polySet->uvOrigin.v = (RwReal) (polySetOrigin [1] + 1);

    RWRETURN (TRUE);
}

/****************************************************************************
 LtMapComparePolySet
 */
#if (defined(_WINDOWS))
static int __cdecl
#else
static int
#endif
LtMapComparePolySet(const void *a, const void *b)
{
    const LtMapPolySet * const set1 = *(const LtMapPolySet * const *)a;
    const LtMapPolySet * const set2 = *(const LtMapPolySet * const *)b;
    int result;

    RWFUNCTION(RWSTRING("LtMapComparePolySet"));

    /* It's amusing to reverse this test with SHOWLIGHTMAPUSAGE
     * turned on, and inspect the results - they're pretty bad! */
    result = RwInt32FromRealMacro(set2->size.y - set1->size.y);

    RWRETURN(result);
}

/* TODO[2][ABF]: NEED FUNCTION-GROUP DIVIDERS... UV ALLOC FUNCTIONS,
 *               LIGHT FUNCS, POINTLIGHT FUNCS, AREALIGHT FUNCS, ETC... */


/****************************************************************************
 LtMapCalculatePolySets
 */
static void
LtMapCalculatePolySets (LtMapAllocUVObjectData *uvObjectData,
                        LtMapWorldData *worldData,
                        LtMapPolySet **allSets, RwInt32 *numSetsOut)
{
    RwSList        *bounds[6], *edgeList;
    LtMapTopology  *topology;
    RwUInt8        *triAssigned;
    RwInt32         numProcessedTris, minTri;
    RwInt32         i, j, k;
    RwInt32         numSets;

    RWFUNCTION(RWSTRING("LtMapCalculatePolySets"));

    /* NOTE: you can't keep pointers to the members of RwSLists,
     *       they move around (realloc and rwSListDestroyEntry()) */

    /* You could group non-overlapping +/- polys to give only three axes
     * (fewer meshes, better lightmap packing), but it'd be a pain and would
     * introduce more wrongly-shared-texel artifacts */
    for (i = 0;i < 6;i++)
    {
        bounds[i] = rwSListCreate(sizeof(LtMapPolySet),
            rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION);
    }

    /* Generate topology info for the object, linking triangles, vertices and
     * edges. Note that we link on the basis of vertex position as well as
     * explicit triangle vertex index references (so vertex-splitting done at
     * export time, due to texture boundaries or creases, is ignored here). */
    topology = TopologyCreate (uvObjectData, TRUE);
    RWASSERT(NULL != topology);

    /* Now generate contiguous sets of (edge-sharing) triangles by
     * walking the mesh, keeping track of already-assigned triangles. */
    triAssigned = (RwUInt8 *) RwMalloc(uvObjectData->numTriangles * sizeof(RwUInt8),
                                       rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION);
    RWASSERT(NULL != triAssigned);

    edgeList = rwSListCreate(sizeof(RwUInt32),
        rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION);
    RWASSERT(NULL != edgeList);

    memset(triAssigned, 0, uvObjectData->numTriangles * sizeof(RwUInt8));
    numSets = numProcessedTris = minTri = 0;

    /* Keep creating polySets until all triangles have been processed: */
    while (numProcessedTris < uvObjectData->numTriangles)
    {
        RwV3d         edge[2], normal;
        LtMapPolySet *polySet;
        RpMaterial   *polySetMat;
        LtMapPoly     newPoly;
        RwReal        length, u;
        RwInt32       tri, triMatch, axis, polySetAxis;
        RwBool        sliver;

        /* Get an unassigned, non-sliver triangle to start a polySet */
        tri = minTri;
        while (triAssigned[tri])
        {
            tri++;
            /* We should only run out of triangles whilst adding them
             * to polySets, below. The only way they could run out here
             * is if there were some free-floating degenerates, not
             * attached to any non-degenerate triangles... which seems
             * pretty crazy and is a geometry bug, basically. */
            RWASSERT(tri < uvObjectData->numTriangles);
        }
        /* Whether it's a sliver or not, this triangle is now processed */
        triAssigned[tri] = TRUE;

        /* Also include any edges from triangles that physically share
         * the vertices of this triangle (not merely welded) as these
         * must be included even without a shared edge */
        for (j = 0;j < 3;j++)
        {
            RwUInt32 eachCorner;
            RwUInt32 eachTri, triTest;
            LtMapVertTriList *curVertTriList;

            eachCorner = uvObjectData->triangles [tri].vertIndex [j];
            curVertTriList = &topology->vertTriLists [eachCorner];
            eachTri = 0;
            while (curVertTriList != NULL && curVertTriList->list [eachTri] != 0xffffffff)
            {
                RwUInt32 eachEdge;
                triTest = curVertTriList->list [eachTri];

                /* Yes, this tri's edges must be added.  Some edges may be
                 * re-added later.  This slows the algorithm, but is harmless */
                for (eachEdge = 0; eachEdge < 3; eachEdge++)
                {
                    *(RwUInt32 *)rwSListGetNewEntry(edgeList,
                        rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION) =
                        topology->triEdgeLists [triTest].list [eachEdge];
                }

                if (++ eachTri == VERTTRILISTSIZE)
                {
                    eachTri = 0;
                    curVertTriList = curVertTriList->next;
                }
            }
        }

        numProcessedTris++;

        TRISETUPSLIVERTESTMACRO(tri, sliver);

        /* Which axis does this polygon belong to? */
        u = uvObjectData->texCoords[newPoly.vertIndex[0]].u;
        polySetAxis = RwFastRealToUInt32(10.0001f*u);
        RWASSERT(polySetAxis <= 5);

        /* Create a new polySet */
        polySet = (LtMapPolySet *)rwSListGetNewEntry(bounds[polySetAxis],
                                 rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION);
        polySet->members = rwSListCreate(sizeof(LtMapPoly),
            rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION);
        polySet->vertices = uvObjectData->vertices;
        polySet->texCoords = uvObjectData->texCoords;
        polySetMat = newPoly.mat;
        /* Grab density from this, the first polygon
         * (we only group polys of the same density) */
        polySet->density = worldData->lightMapDensity *
            RtLtMapMaterialGetLightMapDensityModifier(newPoly.mat);

        if (!(RtLtMapMaterialGetFlags(newPoly.mat) & rtLTMAPMATERIALLIGHTMAP))
        {
            /* Reduce density ridiculously for non-lightmapped geom,
             * so we don't waste lightmap space on it */
            polySet->density *= 0.0625f;
        }

        /* Add the first polygon to the polySet */
        *(LtMapPoly *)rwSListGetNewEntry(polySet->members,
                                         rwID_LTMAPPLUGIN |
                                         rwMEMHINTDUR_FUNCTION) = newPoly;

        /* Assign the first three edges to the polySet boundary */
        for (j = 0;j < 3;j++)
        {
           *(RwUInt32 *)rwSListGetNewEntry(edgeList, rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION) =
               topology->triEdgeLists[tri].list [j];
        }

        numSets++;

        /* Save some time next time round (actually avoids O(n^2)
         * looping, on "while (triAssigned[i])" above, which could
         * become significant for tricounts in the thousands)
         * NOTE: we start it at the *next* tri along. This ought
         *       to be safe because if we process all triangles,
         *       such that the next tri is outside array bounds,
         *       that's exactly when the while loop exits! :)_ _ _ */
        minTri = tri + 1;

        while (rwSListGetNumEntries (edgeList) > 0)
        {
            LtMapEdgeTriList *curEdgeTriList;
            RwUInt32 curEdge, curEntry;

            /* Work from the start of the edge-list so that we process
             * all of a edge's triangles before moving onto the next edge
             * (i.e. so we aren't distracted by edges being added to the
             * end of the list, thus having to process a edge's triangle
             * list more than once) */
            curEdge = *(RwUInt32 *)rwSListGetEntry(edgeList, 0);
            curEdgeTriList = &(topology->edgeTriLists[curEdge]);

            /* Run through the triangles sharing this edge */
            curEntry = 0;

            while (curEdgeTriList != NULL && curEdgeTriList->list [curEntry] != 0xFFFFFFFF)
            {
                do
                {
                    /* Done this tri already - if so skip onto next */
                    if (triAssigned [curEdgeTriList->list [curEntry]])
                        break;

                    /* Ok, we found a new triangle to add to the polySet
                     * (unless it's a sliver or facing in a different
                     * direction or has different material flags) */
                    triMatch = curEdgeTriList->list[curEntry];

                    TRISETUPSLIVERTESTMACRO(triMatch, sliver);

                    /* Which axis does this polygon belong to? */
                    u = uvObjectData->texCoords[newPoly.vertIndex[0]].u;
                    axis = RwFastRealToUInt32(10.0001f * u);
                    RWASSERT(axis <= 5);

                    /* We should not add this polygon to the polySet if it
                     * faces in a different direction to the polySet. */
                    if (axis != polySetAxis)
                        break;
                        
                    /* Nor should we add it if its material
                     * has a different sample density */
                    if (RtLtMapMaterialGetLightMapDensityModifier (newPoly.mat) !=
                        RtLtMapMaterialGetLightMapDensityModifier (polySetMat))
                        break;

                    /* Don't allow lightmapped and non-lightmapped
                     * geometry (usually corresponding to non-emitter and emitter)
                     * to be merged into a polySet, or bilerp will cause
                     * artifacts */
                    if ((RtLtMapMaterialGetFlags(newPoly.mat) & rtLTMAPMATERIALLIGHTMAP) !=
                        (RtLtMapMaterialGetFlags(polySetMat) & rtLTMAPMATERIALLIGHTMAP))
                        break;

                    /* Success - Assign the triangle to the polySet */
                    *(LtMapPoly *)rwSListGetNewEntry(polySet->members,
                             rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION) = newPoly;
                    triAssigned[triMatch] = TRUE;
                    numProcessedTris++;

                    /* Assign two new edges from the triangle to the polySet
                     * boundary (at least one will already have been in the
                     * boundary, it having been used to find this poly - others
                     * may be unnecessarily re-added, but it does no harm and
                     * preventing it would probably cost as much) */
                    for (j = 0;j < 3;j++)
                    {
                        if (curEdge != topology->triEdgeLists [triMatch].list [j])
                        {
                            *(RwUInt32 *)rwSListGetNewEntry(edgeList,
                                rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION) =
                                topology->triEdgeLists [triMatch].list [j];
                        }
                    }
              
                    /* Also include any edges from triangles that physically share
                     * the vertices of this triangle (not merely welded) as these
                     * must be included even without a shared edge */
                    for (j = 0;j < 3;j++)
                    {
                        RwUInt32 eachCorner;
                        RwUInt32 eachTri, triTest;
                        LtMapVertTriList *curVertTriList;

                        eachCorner = uvObjectData->triangles [triMatch].vertIndex [j];
                        curVertTriList = &topology->vertTriLists [eachCorner];
                        eachTri = 0;
                        while (curVertTriList != NULL && curVertTriList->list [eachTri] != 0xffffffff)
                        {
                            RwUInt32 eachEdge;
                            triTest = curVertTriList->list [eachTri];

                            /* Yes, this tri's edges must also be added.  Again,
                             * some edges may be re-added.  This slows the
                             * algorithm, but is harmless */
                            for (eachEdge = 0; eachEdge < 3; eachEdge++)
                            {
                                *(RwUInt32 *)rwSListGetNewEntry(edgeList,
                                    rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION) =
                                    topology->triEdgeLists [triTest].list [eachEdge];
                            }
                            if (++ eachTri == VERTTRILISTSIZE)
                            {
                                eachTri = 0;
                                curVertTriList = curVertTriList->next;
                            }
                        }
                    }


                } while (0); /* Handy for break, keeps indentation down, improves code clarity */

                if (++ curEntry == EDGETRILISTSIZE)
                {
                    /* This edge has more than one block of triangles,
                     * so step along to the next block: */
                    curEdgeTriList = curEdgeTriList->next;
                    curEntry = 0;
                }
            }

            /* We're done with this edge, delete it from the
             * SList (overwrite it with the last list entry) */
            *(RwUInt32 *)rwSListGetEntry(edgeList, 0) = *(RwUInt32 *)
                rwSListGetEntry(edgeList, (rwSListGetNumEntries(edgeList) - 1));
            rwSListDestroyEntry(edgeList, (rwSListGetNumEntries(edgeList) - 1));

        } /* While there are edges left to process in the polySet */
    } /* While there are triangles left to process */

    rwSListDestroy(edgeList);
    RwFree(triAssigned);
    TopologyDestroy(topology);

    /* Initialize the lightmap UVs. */
    for (i = 0;i < uvObjectData->numVertices;i++)
    {
        RwUInt32 temp, axis;

        /* We want the 'axis' information to remain intact. */
        axis = RwFastRealToUInt32(10.0001f * uvObjectData->texCoords[i].u);
        RWASSERT (axis < 6);
        temp = *(RwUInt32 *) &(uvObjectData->texCoords[i].u);
        temp = (temp & (~0x7)) | axis;
        uvObjectData->texCoords[i].u = *(RwReal *)&temp;
        temp = *(RwUInt32 *) &(uvObjectData->texCoords[i].v);
        temp = (temp & (~0x7)) | axis;
        uvObjectData->texCoords[i].v = *(RwReal *)&temp;
    }

    /* Find the texel-space bounding size of polySets */
    for (i = 0;i < 6;i++)
    {
        for (j = 0;j < rwSListGetNumEntries(bounds[i]);j++)
        {
            LtMapPolySet *polySet = (LtMapPolySet *) rwSListGetEntry(bounds[i], j);
            RwBBox  bbox;
            RwV3d   delta;

            for (k = 0;k < rwSListGetNumEntries(polySet->members);k++)
            {
                LtMapPoly *testPoly = (LtMapPoly *) rwSListGetEntry(polySet->members, k);

                if (0 == k)
                {
                    RwV3d start = polySet->vertices[testPoly->vertIndex[0]];
                    RwBBoxInitialize(&bbox, &start);
                }

                RwBBoxAddPoint(&bbox, &(polySet->vertices[testPoly->vertIndex[0]]));
                RwBBoxAddPoint(&bbox, &(polySet->vertices[testPoly->vertIndex[1]]));
                RwBBoxAddPoint(&bbox, &(polySet->vertices[testPoly->vertIndex[2]]));
            }

            /* We need to calculate how many whole texels the polySet covers,
             * so floor the inf vector and subtract it from the sup vector -
             * this gives us a value we can ceil() to give texel coverage, but
             * it's still a float, so we can scale it down later if the current
             * sector needs its polysets shrinking to fit into a lightmap */
            RwV3dScale(&bbox.inf, &bbox.inf, polySet->density);
            RwV3dScale(&bbox.sup, &bbox.sup, polySet->density);
            bbox.inf.x = (RwReal)RwFloor(bbox.inf.x);
            bbox.inf.y = (RwReal)RwFloor(bbox.inf.y);
            bbox.inf.z = (RwReal)RwFloor(bbox.inf.z);

            /* The 1-texel boundary needed around the rectangle
             * is taken into account below, during polyset packing */
            RwV3dSub(&delta, &bbox.sup, &bbox.inf);
            polySet->size.x = RwV3dDotProduct(&delta, &rtLtMapGlobals.rightVector[i]);
            polySet->size.y = RwV3dDotProduct(&delta, &rtLtMapGlobals.upVector[i]);

            /* Store the origin - NOTE: it being floor()d results in the
             * fractional UV coordinates for the 'lowest' vertex in the
             * polyset being retained. If you shifted the polyset down to the
             * next integer UV coord down, you'd see bilerp discontinuities
             * between adjacent coplanar polysets (their UV coordinate grids
             * would be offset) */
            RwV3dScale(&bbox.inf, &bbox.inf, 1.0f / polySet->density);
            polySet->worldOrigin = bbox.inf;

            /* NOTE: Okay, so we end up using integer texture coordinates while
             *       packing our polysets. What we do here, in terms of
             *       calculating origin and texel-size is important - thinking
             *       about a grid of
             *       texel-CENTRES (so bilerp averages values between the
             *       nearest four gridpoints), a polygon lying on the grid is
             *       affected (when bilerp is on) by the gridpoints of any
             *       4-gridpoint-square that it overlaps. Hence we floor the
             *       inf value and (later) ceil the sup value. Any points
             *       lying ON an integer texcoord need not be rounded up
             *       further, as bilerp will give zero contributions, to such
             *       points, of the next texel along.
             *       Greedy rasterisation during illumination ensures all
             *       texels that overlap a triangle are lit. Dilate() ensures
             *       that all 4-gridpoint-squares that affect a triangle are
             *       'lit'. */
        }
    }

    /* Sort the polysets, largest to smallest - the best order for packing */
    {
        LtMapPolySet **sortSet;
        LtMapPolySet **set;

        /* Build an array of pointers to every LtMapPolySet */
        sortSet = (LtMapPolySet **) RwMalloc(numSets * sizeof(LtMapPolySet *),
                                     rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION);
        set = sortSet;
        for (i = 0;i < 6;i++)
        {
            for (j = 0;j < rwSListGetNumEntries(bounds[i]);j++)
            {
                LtMapPolySet *polySet;

                polySet = (LtMapPolySet *) rwSListGetEntry(bounds[i], j);
                
                set[j] = polySet;
            }
            set += rwSListGetNumEntries(bounds[i]);
        }

        /* Sort the pointer array on Y size (seems to work well) */
        qsort(sortSet, numSets, sizeof(LtMapPolySet *), LtMapComparePolySet);

        /* Copy into the allSets array */
        *allSets = (LtMapPolySet *) RwMalloc(numSets * sizeof(LtMapPolySet), rwID_LTMAPPLUGIN | rwMEMHINTDUR_EVENT);
        for (i = 0; i < numSets; i++)
        {
            (*allSets) [i] = *(sortSet [i]);
            (*allSets) [i].uvOrigin.u = -1.0f; /* Uninitialised */
        }
        *numSetsOut = numSets;

        RwFree (sortSet);
    }

    /* Clean up */
    for (i = 0;i < 6;i++)
    {
        rwSListDestroy(bounds[i]);
    }

    RWRETURNVOID ();
}

/****************************************************************************
 LtMapFindFreeSlot
 */
static LtMapSlot *
LtMapFindFreeSlot(LtMapPolySet     *allSets,
                  RwUInt32         numSets,
                  LtMapAllocUVData *uvData,
                  RwBool           ownLightMap)
{
    LtMapSlot *useSlot = NULL;
    RwUInt32   i, index, numSlots;

    RWFUNCTION(RWSTRING("LtMapFindFreeSlot"));

    /* TODO[5][AAC]: THIS IS AN O(N^2) LOOP AND DOES TOO MUCH UNNECESSARY
     *         POLY-PACKING
     *         IT WOULD BE BETTER TO PRE-PROCESS THE LIST OF SECTORS TO SEE WHICH
     *         WILL LIKELY MAKE GOOD USE OF LIGHTMAPS IF PAIRED UP
     *          THEN YOU CAN PACK THE POLYSETS OF BOTH SECTORS TOGETHER, TO GET
     *         IMPROVED JOINT PACKING AND YOU'LL PROBABLY DO ONLY O(N) PACKING */

    numSlots = _rwSListGetNumEntries(rtLtMapGlobals.ltMapSlots);

    /* Find a Slot which can accomodate ALL these */
    for (index = 0;index < numSlots;index++)
    {
        LtMapSlot tmpSlot = uvData->slots[index];
        RwBool allFit;

#if 0        
        RwUInt32 sectorSize = 0, padSize = 0, freeSpace;

        /* Quick early-out optimisation - count up the summed size
         * of the sector's polySets and check it against the summed
         * size of free rectangles in the target lightmaps. This'll
         * save unnecessary packing */
        /* TODO[3]: THIS EARLY-OUT DOESN'T SEEM TO SAVE ANY SPEED EVEN THOUGH IT
         *          EARLIES-OUT A *LOT* OF THE TIME... SO WHERE *IS* THE TIME GOING
         *          THEN? */
        for (i = 0;i < numSets;i++)
        {
            RwUInt32 width, height;
            width  = RwFastRealToUInt32((RwReal)RwCeil(allSets[i].size.x + 0.001f));
            height = RwFastRealToUInt32((RwReal)RwCeil(allSets[i].size.y + 0.001f));
            sectorSize += width * height;
            padSize += 4 + 2 * (width + height);
        }

        freeSpace = 0;
        for (i = 0;i < tmpSlot.count;i++)
        {
            freeSpace += tmpSlot.rects[i].w * tmpSlot.rects[i].h;
        }

        if (freeSpace < (sectorSize + padSize))
            continue;

#endif        
        /* Multiply-instanced geometries must have their own light map */
        if (ownLightMap && tmpSlot.count != 1)
            continue;

#ifdef SHOWLIGHTMAPUSAGE
        /* We create a temporary image to modify during packing but if
         * packing fails we won't write it back to the raster. */
        rtLtMapGlobals.gLightMapUsageImage = RwImageCreate(
            RwRasterGetWidth( RwTextureGetRaster(uvData->slots[index].lightMap)),
            RwRasterGetHeight(RwTextureGetRaster(uvData->slots[index].lightMap)), 32);
        RWASSERT(NULL != rtLtMapGlobals.gLightMapUsageImage);

        RwImageAllocatePixels(rtLtMapGlobals.gLightMapUsageImage);

        RwImageSetFromRaster(rtLtMapGlobals.gLightMapUsageImage,
            RwTextureGetRaster(tmpSlot.lightMap));
#endif /* def SHOWLIGHTMAPUSAGE */

        allFit = TRUE;
        for (i = 0;i < numSets;i++)
        {
            if (!LtMapAllocRect(&allSets[i], &tmpSlot))
            {
                allFit = FALSE;
                break;
            }
        }

        /* Managed to fit all of them, so break out */
        if (allFit)
        {
            /* Update slot info with the newly-added polysets */
            uvData->slots[index] = tmpSlot;
            useSlot = &(uvData->slots[index]);

#ifdef SHOWLIGHTMAPUSAGE
            RwRasterSetFromImage(RwTextureGetRaster(tmpSlot.lightMap),
                rtLtMapGlobals.gLightMapUsageImage);
#endif /* def SHOWLIGHTMAPUSAGE */
       }

#ifdef SHOWLIGHTMAPUSAGE
        RwImageDestroy(rtLtMapGlobals.gLightMapUsageImage);
        rtLtMapGlobals.gLightMapUsageImage = (RwImage *)NULL;
#endif /* def SHOWLIGHTMAPUSAGE */
    
        if (allFit)
        {
            RWRETURN (useSlot);
        }
    }


    RWRETURN (NULL);
}

/****************************************************************************
 SelectOrCreateLightMap
 */
static LtMapSlot *
SelectOrCreateLightMap (LtMapPolySet *allSets, RwInt32 numSets,
                        LtMapAllocUVData       *uvData ,
                        LtMapAllocUVObjectData *uvObjectData)
{
    LtMapSlot      *useSlot, *newUseSlot = (LtMapSlot *)NULL;
    rpLtMapSampleMap *sampleMap = NULL;
    RwInt32 i;
    RwBool ownLightMap = FALSE;
    RwTexture *tex;
    RwUInt32   size;
    RwUInt32   format = RpLtMapGetRasterFormat();

    RWFUNCTION(RWSTRING("SelectOrCreateLightMap"));

    /* Geometries that have been multiple-instanced get their own lightmap
     * each time */
    if (uvObjectData->geomSource != NULL && uvObjectData->geomSource->refCount > 1)
    {
        ownLightMap = TRUE;
    }

    /* LtMapFindFreeSlot () checks if all this sector's polysets will fit in an
     * existing (partially filled) lightmap.  If not, we create a new one for
     * this sector below */
    do
    {
        useSlot = LtMapFindFreeSlot (allSets, numSets, uvData, ownLightMap);
        if (useSlot != NULL)
        {
            RWRETURN (useSlot);
        }

        /* Work out what size of light map to create */
        if (ownLightMap)
        {
            RwUInt32 numSlots;
            if (uvData->slots == NULL)
            {
                numSlots = 0;
            }
            else
            {
                numSlots = _rwSListGetNumEntries(rtLtMapGlobals.ltMapSlots);
            }

            /* Start with the smallest possible lightmap, then try
             * doubling the size until the geometry fits */
            if (numSlots != 0 && uvData->slots [numSlots - 1].count == 1)
            {
                size = uvData->slots [numSlots - 1].size * 2;

                _rpLtMapSampleMapDestroy(uvData->slots [numSlots - 1].sampleMap);
                RwTextureDestroy(uvData->slots [numSlots - 1].lightMap);
                _rwSListDestroyEndEntries(rtLtMapGlobals.ltMapSlots, 1);
            }
            else
            {
                size = rpLTMAPMINLIGHTMAPSIZE;
            }
        }
        else
        {
            /* Grab the lightmap size of the current object or use the global
             * default */
            if (0 == uvObjectData->objectData->lightMapSize)
            {
                uvObjectData->objectData->lightMapSize = _rpLtMapGlobals.lightMapSize;
            }
            size = uvObjectData->objectData->lightMapSize;
        }

#ifdef SHOWLIGHTMAPUSAGE
        /* Due to driver bugs (well, and for efficiency) we need to turn
         * auto-mipmapping off before modifying the subraster. We'll turn it
         * on again at the end of the packing process in
         * RtLtMapLightMapsCreate() */
        format &= ~rwRASTERFORMATAUTOMIPMAP;
#endif /* def SHOWLIGHTMAPUSAGE */

        /* NOTE: the texture ends up with a ref-count of one - this
         *       is what we want, as it is 'owned' by the world */
        tex = RwTextureCreate(RwRasterCreate(size, size, 0, format));

        /* Create a new vismap for this texture and add it to the SList. */
        sampleMap = _rpLtMapSampleMapCreate(size, size);

        /* TODO[3]: WHY WRAP? */
        RwTextureSetAddressing(tex, rwTEXTUREADDRESSWRAP);
        /* Set the sampling mode on the basis of the current renderStyle */
        if (_rpLtMapGlobals.renderStyle & rpLTMAPSTYLEPOINTSAMPLE)
        {
            RwTextureSetFilterMode(tex, rwFILTERNEAREST);
        }
        else
        {
            if (format & rwRASTERFORMATMIPMAP)
            {
                RwTextureSetFilterMode(tex, rwFILTERLINEARMIPLINEAR);
            }
            else
            {
                RwTextureSetFilterMode(tex, rwFILTERLINEAR);
            }
        }

        /* NOTE: the lightmap gets cleared in RtLtMapLightMapsCreate */
#ifdef SHOWLIGHTMAPUSAGE
        {
            /* Clear uninitialised space to bright blue */
            RwRGBA blue = {0, 0, 255, 255};
            RwRect rect;
            rect.x = rect.y = 0;
            rect.w = RwRasterGetWidth( RwTextureGetRaster(tex));
            rect.h = RwRasterGetHeight(RwTextureGetRaster(tex));
            RasterRectClear(RwTextureGetRaster(tex), &rect, blue);
        }
#endif /* def SHOWLIGHTMAPUSAGE */

        newUseSlot = _rwSListGetNewEntry(rtLtMapGlobals.ltMapSlots,
            rwID_LTMAPPLUGIN | rwMEMHINTDUR_EVENT);
        uvData->slots = (LtMapSlot *) _rwSListGetEntry(rtLtMapGlobals.ltMapSlots, 0);

        newUseSlot->count = 1;
        newUseSlot->rects[0].x = 0;
        newUseSlot->rects[0].y = 0;
        newUseSlot->rects[0].w = size;
        newUseSlot->rects[0].h = size;

        newUseSlot->lightMap = tex;
        newUseSlot->size = size;
        newUseSlot->sampleMap = sampleMap;

        /* Give new lightmap a name and add it to the texture dictionary. */
        _rtLtMapNameGen(RwTextureGetName(tex), FALSE);
        if( RwTexDictionaryGetCurrent() )
        {        
            RwTexDictionaryAddTexture(RwTexDictionaryGetCurrent(), tex);
        }

    } while (ownLightMap);

    useSlot = LtMapFindFreeSlot(allSets, numSets, uvData, ownLightMap);

    /* Keep reducing resolution until it fits */
    while (useSlot == NULL)
    {
        RwDebugSendMessage(rwDEBUGMESSAGE, RWSTRING("allocateUV"),
            RWSTRING("rescaling UVs (lightmap resolution too low or lightmap sample density too high)"));
        /* TODO[3][ACO]: WE COULD REDUCE SHRINKAGE SLIGHTLY BY CHOOSING A DIFFERENT
        *       DENSITY-MULTIPLIER BELOW. IF THAT GIVES YOU LIGHTMAP SHARING, YOU
        *       NEED A LOWER INITIAL DENSITY, LARGER LIGHTMAPS OR SMALLER OBJECTS. */
        for (i = 0;i < numSets;i++)
        {
            LtMapPolySet *polySet = &allSets [i];

            /*TODO[5][ACO]: IT MIGHT BE INTRACTABLE, BUT IT'D CLEARLY BE A QUALITY WIN TO SCALE NOT THE WHOLE
            *         SECTOR BUT ONLY THE LARGEST POLYSETS. THIS WILL AVOID DETAILED GEOMETRY ENDING UP
            *         WITH LOW-RES LIGHTMAPS JUST BECAUSE THEY SHARE A SECTOR WITH SOME LARGE POLYGONS.
            *          CURRENTLY, WE SCALE THE WHOLE SECTOR, ALL OR NOTHING. PER-MATERIAL DENSITY SETTING
            *         HELPS PEOPLE CORRECT THIS WHERE IT STANDS OUT, BUT IT'S NOT AUTOMATED. PERHAPS
            *         WE CAN COMPARE THE SIZES OF THE BIGGEST POLYSETS TO THE AVERAGE SIZE AND THE
            *         AMOUNT OF SPACE THAT MUST BE FREED UP TO ACCOMMODATE THE SECTOR... AND THEN
            *         SHRINK THE N BIGGEST POLYSETS... SOMETHIN'. ACTUALLY, GIVEN YOU KNOW HOW MUCH
            *         SPACE YOU NEED, IT SHOULD BE TRIVIAL TO WORK OUT HOW MANY OF THE N BIGGEST YOU
            *         NEED TO HALVE IN SIZE TO MEET THAT GOAL. FAIL AND YOU SHRINK SOME MORE. SHRINK
            *         'EM ALL AND YOU START FROM THE BIGGEST POLYSET AGAIN. SWEET! */

            /* NOTE: the 1-texel padding border is added to polySet->size inside
             * LtMapFindFreeSlot, so IT doesn't get shrunk by this scaling. */
            RwV2dScale(&(polySet->size), &(polySet->size), 0.5f);
            polySet->density *= 0.5f;
        }

        /* TODO[2][ACP]: WE NEED SOME CODE IN HERE THAT TERMINATES AND FAILS ONCE ALL A SECTOR'S POLYSETS
        *         ARE ONE PIXEL (9 PIXELS WITH BORDERS) AND *STILL* DON'T FIT INTO A LIGHTMAP. IN THIS
        *         CASE, VERTEX LIGHTING WOULD CLEARLY BE BETTER (THOUGH YOU MIGHT BE ABLE TO GET AWAY
        *         WITH SMALLER OBJECTS OR BIGGER LIGHTMAPS) - IT WOULD BE SIGNIFICANTLY BEFORE
        *         REACHING THIS EXTREME STAGE, IN FACT. ADD ASSERTS AND CRASH-PREVENTION. */

        useSlot = LtMapFindFreeSlot(allSets, numSets, uvData, ownLightMap);
    }

    /* If the new lightmap didn't get used (i.e, after shrinkage, the
     * polyset fit into a prior lightmap) then destroy it again */
    if (useSlot != newUseSlot)
    {
        _rpLtMapSampleMapDestroy(newUseSlot->sampleMap);
        RwTextureDestroy(newUseSlot->lightMap);
        _rwSListDestroyEndEntries(rtLtMapGlobals.ltMapSlots, 1);
    }

    RWRETURN (useSlot);
}



/****************************************************************************
 LtMapObjectAllocateUV
 */
static RwBool
CheckUVsSetUpForLightMapping (const LtMapAllocUVObjectData *uvObjectData)
{
    RwInt32         i, j;
    RwUInt32        axes[3];
    RwBool          outOfRange = FALSE, unEqual = FALSE;

    /* Check that the UVs are set up right (ignore Vs, we only ever use Us) */
    if (uvObjectData->geomSource == NULL ||
        RPLTMAPGEOMETRYGETDATA (uvObjectData->geomSource)->PolySetArray == NULL)
    {
        for (i = 0;i < uvObjectData->numTriangles;i++)
        {
            for (j = 0;j < 3;j++)
            {
                /* NOTE: the exporter isn't spitting out exact values for some
                 * reason, so we resort to testing integer-rounded values here */
                /*triCoords[j] = &(uvObjectData->texCoords[
                                       uvObjectData->triangles[i].vertIndex[j]]);
                  if ((0.0f != triCoords[j]->u) && (0.1f != triCoords[j]->u) &&
                      (0.2f != triCoords[j]->u) && (0.3f != triCoords[j]->u) &&
                      (0.4f != triCoords[j]->u) && (0.5f != triCoords[j]->u) )*/

                /* Make sure values are 0.0f, 0.1f ... 0.5f */
                axes[j] = RwFastRealToUInt32(10.0001f *
                    uvObjectData->texCoords[uvObjectData->triangles[i].vertIndex[j]].u);
                if (axes[j] > 5)
                {
                    outOfRange = TRUE;
                }
            }

            /* Make sure UVs are the same for all vertices in a triangle */
            if ((axes[0] != axes[1]) || (axes[1] != axes[2]) )
            {
                unEqual = TRUE;
            }
        }

        if (FALSE != outOfRange)
        {
            RwDebugSendMessage(rwDEBUGERROR, "LtMapAtomicAllocateUV",
                "Lightmap UV coordinates are not in a correctly-conditioned, post-export state (all UVs should be set to 0.0, 0.1, 0.2, 0.3, 0.4 or 0.5)");
        }
        if (FALSE != unEqual)
        {
            RwDebugSendMessage(rwDEBUGERROR, "LtMapAtomicAllocateUV",
                "Lightmap UV coordinates are not in a correctly-conditioned, post-export state (all three UVs should be equal for any given triangle)");
        }
        if ((FALSE != outOfRange) || (FALSE != unEqual))
        {
            RWRETURN (FALSE);
        }
    }

    RWRETURN (TRUE);
}

/****************************************************************************
 LtMapObjectAllocateUV
 */
static RwBool
LtMapObjectAllocateUV(RtLtMapLightingSession *session __RWUNUSED__,
                      LtMapAllocUVData       *uvData ,
                      LtMapAllocUVObjectData *uvObjectData)
{
    LtMapPolySet   *allSets;
    RwInt32         numSets;
    LtMapWorldData *worldData;
    RwUInt32        width, height;
    RwInt32         i, j, k, m;
    LtMapGeometryData *geometryData;
    RwV3d          *worldPos;
    LtMapSlot      *useSlot;

    RWFUNCTION(RWSTRING("LtMapObjectAllocateUV"));

    RWASSERT(NULL != uvData);
    RWASSERT(NULL != uvObjectData);
    RWASSERT(NULL != uvObjectData->matList);
    RWASSERT(NULL != uvObjectData->vertices);
    RWASSERT(NULL != uvObjectData->texCoords);
    RWASSERT(NULL != uvObjectData->triangles);

    /* Ensure the second set of UVs are properly set up */
    if (!CheckUVsSetUpForLightMapping (uvObjectData))
        RWRETURN (FALSE);

    /* Obtain poly sets if they've not already been calculated.  These
     * determine what sizes of UV rectangles are needed for this object */
    worldData = RPLTMAPWORLDGETDATA(uvData->world);
    if (uvObjectData->geomSource == NULL)
    {
        geometryData = NULL;
        LtMapCalculatePolySets (uvObjectData, worldData, &allSets, &numSets);
    }
    else
    {
        geometryData = RPLTMAPGEOMETRYGETDATA(uvObjectData->geomSource);
        if (geometryData->PolySetArray == NULL)
        {
            LtMapCalculatePolySets (uvObjectData, worldData, &allSets, &numSets);
        }
        else
        {
            allSets = geometryData->PolySetArray;
            numSets = geometryData->numSets;
        }
    }

    useSlot = SelectOrCreateLightMap (allSets, numSets, uvData, uvObjectData);

    /* Assign UVs if they have not already been assigned */
    if (geometryData == NULL || geometryData->PolySetArray == NULL)
    {
        for (i = 0; i < numSets; i++)
        {
            LtMapPolySet *polySet = &allSets [i];
            LtMapPoly *getAxisPoly = (LtMapPoly *) rwSListGetEntry(polySet->members, 0);
            RwUInt32    axis;
            RwUInt32    *get_axis;
            RwReal texelSize = 1.0f / useSlot->size;

            /* Fish axis out of bottom 3 bits */
            get_axis = (RwUInt32 *) &polySet->texCoords[getAxisPoly->vertIndex[0]].u;
            axis = (*get_axis) & 0x7;
            RWASSERT (axis < 6);

            for (k = 0; k < rwSListGetNumEntries(polySet->members); k++)
            {
                LtMapPoly *testPoly = (LtMapPoly *) rwSListGetEntry(polySet->members, k);

                for (m = 0;m < 3;m++)
                {
                    LtMapIntUV  intUVs;
                    RwV3d       pos;
                    RwV2d       proj;

                    /* We take the offset of the current vertex from this
                     * polySet's world-space origin, multiply it by texel
                     * density and then add the result onto the polySet's
                     * UVorigin. */
                    pos = polySet->vertices[testPoly->vertIndex[m]];
                    RwV3dSub(&pos, &pos, &(polySet->worldOrigin));
                    RwV3dScale(&pos, &pos, polySet->density);
                    proj.x = RwV3dDotProduct(&pos, &(rtLtMapGlobals.rightVector[axis]));
                    proj.y = RwV3dDotProduct(&pos, &(rtLtMapGlobals.upVector   [axis]));

                    /* Translate the UVs */
                    intUVs.uv.u = (polySet->uvOrigin.u + proj.x) * texelSize;
                    intUVs.uv.v = (polySet->uvOrigin.v + proj.y) * texelSize;

                    /* Preserve the axis information in the bottom 3 bits of each
                     * texture coordinate, (because it's a pain to
                     * recalculate it outside of the modelling package!):
                     * NOTE: we use a union rather than casting pointers because
                     *       some compilers (e.g GCC 2.96) will reorder operations
                     *       incorrectly, once casts change pointer types, when
                     *       building with optimizations on. */
                    intUVs.intUV[0] = (intUVs.intUV[0] & ~0x7) | axis;
                    intUVs.intUV[1] = (intUVs.intUV[1] & ~0x7) | axis;

                    polySet->texCoords[testPoly->vertIndex[m]] = intUVs.uv;
                }
            }
        }

        /* Note that UVs have been generated for this geometry */
        if (geometryData != NULL)
        {
            geometryData->PolySetArray = allSets;
            geometryData->numSets      = numSets;
        }
    }

    /* Allocate enough space for all the samples in
     * the biggest triangle that a lightmap can hold */
    width  = RwRasterGetWidth(RwTextureGetRaster(useSlot->lightMap));
    height = RwRasterGetHeight(RwTextureGetRaster(useSlot->lightMap));

    worldPos = (RwV3d *)RwMalloc((sizeof(RwV3d) * width * height) / 2,
                                 rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION);

    /* Accurately determine the number of lightmap samples for this object */
    uvObjectData->objectData->numSamples = 0;
    for (i = 0;i < uvObjectData->numTriangles;i++)
    {
        /* For each triangle, generate world-space positions of lightmap texels */
        LtMapPoly newPoly;
        RwV3d     triVerts[3], edge[2], normal;
        RwV2d     vScreen[3];
        RwReal    length;
        RwInt32   tri;
        RwBool    sliver;
        RwV3d   *pos;

        tri = i;
        TRISETUPSLIVERTESTMACRO(tri, sliver);

        /* Slivers get filled in by the 'dilate' process in RtLtMapIlluminate,
         * so we mustn't count their samples here or we'll get too big a value. */
        if (sliver)
            continue;

        /* Work out lightmap-space positions */
        for (j = 0;j < 3;j++)
        {
            vScreen[j].x = width * uvObjectData->texCoords[newPoly.vertIndex[j]].u;
            vScreen[j].y = height * uvObjectData->texCoords[newPoly.vertIndex[j]].v;
            triVerts[j] = uvObjectData->vertices[newPoly.vertIndex[j]];
        }
        /* Transform the vertices into world-space if necessary */
        if (NULL != uvObjectData->matrix)
        {
            RwV3dTransformPoints(&(triVerts[0]), &(triVerts[0]), 3, uvObjectData->matrix);
        }

        /* Flip back-facing triangles */
        if (((vScreen[0].x - vScreen[1].x) * (vScreen[2].y - vScreen[1].y)) <=
            ((vScreen[0].y - vScreen[1].y) * (vScreen[2].x - vScreen[1].x)))
        {
            RwV3d tmp3d;
            RwV2d tmp2d;

            tmp2d       = vScreen[2];
            vScreen[2]  = vScreen[1];
            vScreen[1]  = tmp2d;

            tmp3d       = triVerts[2];
            triVerts[2] = triVerts[1];
            triVerts[1] = tmp3d;
        }

        /* Generate world-space positions across triangle surface */
        pos = _rtLtMapLMWCRenderTriangle(worldPos, vScreen, triVerts);
        uvObjectData->objectData->numSamples += pos - worldPos;
    }
    RwFree(worldPos);

    /* Clean up */
    /* Only destroy the allSets for WorldSectors.  Atomics keep this array
     * (in a pointer in the referenced geometry) so that multiple
     * instances don't use different areas of the lightmaps */
    if (geometryData == NULL)
    {
        for (j = 0; j < numSets; j++)
        {
            rwSListDestroy(allSets [j].members);
        }

        RwFree(allSets);
    }

    /* Assign the lightmap */
    RWASSERT(NULL == uvObjectData->objectData->lightMap);
    uvObjectData->objectData->lightMap = useSlot->lightMap;
    uvObjectData->objectData->sampleMap = useSlot->sampleMap;
    (void) RwTextureAddRef(useSlot->lightMap);
    useSlot->sampleMap->refCount++;

    /* Update this since we may have ended up packing
     * this sector into a larger/smaller lightmap */
    uvObjectData->objectData->lightMapSize = useSlot->size;

    RWRETURN(TRUE);
}

/****************************************************************************
 LtMapAtomicAllocateUV
 */
RpAtomic *
_rtLtMapAtomicAllocateUV(RtLtMapLightingSession *session,
                      RpAtomic *atomic, LtMapAllocUVData *uvData)
{
    LtMapAllocUVObjectData uvObjectData = {NULL, NULL, 0, 0, NULL, NULL, 0, NULL, NULL, NULL};
    LtMapObjectData  *objectData;
    RpGeometry       *geom;

    RWFUNCTION(RWSTRING("_rtLtMapAtomicAllocateUV"));

    RWASSERT(NULL != atomic);
    RWASSERT(RpAtomicGetFlags(atomic) & rpATOMICRENDER);

    objectData = RPLTMAPATOMICGETDATA(atomic);

    RWASSERT(objectData->flags & rtLTMAPOBJECTLIGHTMAP);
    RWASSERT(NULL == objectData->lightMap);
    RWASSERT(NULL != uvData);

    geom = RpAtomicGetGeometry(atomic);
    RWASSERT(NULL != geom);
    
    /* This is needed so LtMapObjectAllocateUV () can tell if this geometry
     * has already been allocated UVs */
    uvObjectData.geomSource   = RpAtomicGetGeometry (atomic);
    uvObjectData.objectData   = objectData;
    uvObjectData.matList      = &(geom->matList);
    uvObjectData.numVertices  = RpGeometryGetNumVertices(geom);
    uvObjectData.vertices     = RpMorphTargetGetVertices(RpGeometryGetMorphTarget(geom, 0));

    /* TODO[2][AAK]: ALLOW USER TO SPECIFY *WHICH* TEXCOORD-SET IS USED FOR ATOMICS/SECTORS */
    uvObjectData.texCoords    = RpGeometryGetVertexTexCoords(geom, 2);

    if (NULL == uvObjectData.texCoords)
    {
        RwDebugSendMessage(rwDEBUGERROR, "LtMapAtomicAllocateUV",
            "Object must contain a second set of UV coordinates to support lightmaps");
        RWRETURN((RpAtomic *)NULL);
    }
    uvObjectData.numTriangles = RpGeometryGetNumTriangles(geom);
    uvObjectData.triangles    = RpGeometryGetTriangles(geom);

    /* Do it! */
    if (LtMapObjectAllocateUV(session, uvData, &uvObjectData) == FALSE)
        RWRETURN (NULL);
    RWASSERT(NULL != objectData->lightMap);

    /* Set up the appropriate render pipeline */
    RpAtomicSetPipeline(atomic, RpLtMapGetPlatformAtomicPipeline());

    RWRETURN(atomic);
}

/****************************************************************************
 LtMapWorldSectorAllocateUV
 */
RpWorldSector *
_rtLtMapWorldSectorAllocateUV(RtLtMapLightingSession *session,
                           RpWorldSector *sector, LtMapAllocUVData *uvData)
{
    LtMapAllocUVObjectData uvObjectData = {NULL, NULL, 0, 0, NULL, NULL, 0, NULL, NULL, NULL};
    LtMapObjectData        *objectData;

    RWFUNCTION(RWSTRING("_rtLtMapWorldSectorAllocateUV"));

    RWASSERT(NULL != sector);

    objectData = RPLTMAPWORLDSECTORGETDATA(sector);

    RWASSERT(NULL == objectData->lightMap);
    RWASSERT(sector->numTriangles > 0);
    RWASSERT(objectData->flags & rtLTMAPOBJECTLIGHTMAP);
    RWASSERT(NULL != uvData);

    uvObjectData.geomSource        = NULL;
    uvObjectData.objectData        = objectData;
    uvObjectData.matList           = &(uvData->world->matList);
    uvObjectData.numVertices       = sector->numVertices;
    uvObjectData.vertices          = sector->vertices;

    /* TODO[2][AAK]: ALLOW USER TO SPECIFY *WHICH* TEXCOORD-SET IS USED FOR ATOMICS/SECTORS */
    uvObjectData.texCoords         = sector->texCoords[1];

    if (NULL == uvObjectData.texCoords)
    {
        RwDebugSendMessage(rwDEBUGERROR, "LtMapWorldSectorAllocateUV",
            "Object must contain a second set of UV coordinates to support lightmaps");
        RWRETURN((RpWorldSector *)NULL);
    }
    uvObjectData.numTriangles      = sector->numTriangles;
    uvObjectData.triangles         = sector->triangles;
    uvObjectData.matListWindowBase = sector->matListWindowBase;

    /* Do it! */
    if (LtMapObjectAllocateUV(session, uvData, &uvObjectData) == FALSE)
        RWRETURN (NULL);
    RWASSERT(NULL != objectData->lightMap);

    /* Set up the appropriate render pipeline */
    RpWorldSectorSetPipeline(sector, RpLtMapGetPlatformWorldSectorPipeline());

    RWRETURN(sector);
}

/****************************************************************************
 LtMapGeometryResetUVs
 */
RpGeometry *
_rtLtMapGeometryResetUVs(RpGeometry *geom)
{
    RwTexCoords *texCoords;
    RwUInt32     i;

    RWFUNCTION(RWSTRING("_rtLtMapGeometryResetUVs"));

    RWASSERT(NULL != geom);
    RWASSERT(RpGeometryGetNumTexCoordSets(geom) >= 2);

    RpGeometryLock(geom, rpGEOMETRYLOCKTEXCOORDS);
    texCoords = RpGeometryGetVertexTexCoords(geom, 2);
    RWASSERT(NULL != texCoords);

    /* It's simply not practical (nor reasonable) to try and recalculate
     * all triangle normals here. Slivers will have been exported with
     * dodgy normals and normals calculations will vary from those that
     * the exporter uses, so for these reasons we won't get the same
     * facedness-calculation results that the exporter did.
     *
     * Hence, when we generate lightmap UVs we retain the old 'axis'
     * value (as an integer) in the lower 3 bits of each U and V coordinate.
     * This leaves a full 18+ bits of mantissa which is plenty of precision,
     * esp given that lightmaps are generally less than 8 bits in size!
     *
     * So, here, we grab the bottom 3 bits of the UVs and restore them */
    for (i = 0;i < (RwUInt32) RpGeometryGetNumVertices(geom);i++)
    {
        LtMapIntUV  intUVs;

        /* NOTE: we use a union as when calculating the UVs
         * (see comment there for why) */
        intUVs.uv = texCoords[i];
        RWASSERT((intUVs.intUV[0] & 7) == (intUVs.intUV[1] & 7));

        intUVs.uv.u = 0.1f * (intUVs.intUV[0] & 0x7);
        intUVs.uv.v = 0.1f * (intUVs.intUV[1] & 0x7);
        texCoords[i] = intUVs.uv;
    }

    RpGeometryUnlock(geom);

    RWRETURN(geom);
}


/****************************************************************************
 LtMapWorldSectorResetUVs
 */
RpWorldSector *
_rtLtMapWorldSectorResetUVs(RpWorldSector *sector)
{
    RwTexCoords *texCoords;
    RwUInt32     i;

    RWFUNCTION(RWSTRING("_rtLtMapWorldSectorResetUVs"));

    RWASSERT(RpWorldSectorGetWorld(sector)->numTexCoordSets >= 2);
    texCoords = sector->texCoords[1];
    RWASSERT(NULL != texCoords);

    /* It's simply not practical (nor reasonable) to try and recalculate
     * all triangle normals here. Slivers will have been exported with
     * dodgy normals and normals calculations will vary from those that
     * the exporter uses, so for these reasons we won't get the same
     * facedness-calculation results that the exporter did.
     *
     * Hence, when we generate lightmap UVs we retain the old 'axis'
     * value (as an integer) in the lower 3 bits of each U and V coordinate.
     * This leaves a full 18+ bits of mantissa which is plenty of precision,
     * esp given that lightmaps are generally less than 8 bits in size!
     *
     * So, here, we grab the bottom 3 bits of the UVs and restore them */
    for (i = 0;i < sector->numVertices;i++)
    {
        LtMapIntUV  intUVs;

        /* NOTE: we use a union as when calculating the UVs
         * (see comment there for why) */
        intUVs.uv = texCoords[i];
        RWASSERT((intUVs.intUV[0] & 7) == (intUVs.intUV[1] & 7));

        intUVs.uv.u = 0.1f * (intUVs.intUV[0] & 0x7);
        intUVs.uv.v = 0.1f * (intUVs.intUV[1] & 0x7);
        texCoords[i] = intUVs.uv;
    }

    RWRETURN(sector);
}

/****************************************************************************
 LtMapClassifyNormal
 */
RwUInt32
_rtLtMapClassifyNormal(RwV3d *normal)
{
    RwUInt32 ivec[3];
    RwUInt32 axis;

    RWFUNCTION(RWSTRING("_rtLtMapClassifyNormal"));

    /* Compare relative sizes */
    ivec[0] = RwFastRealToUInt32(normal->x * normal->x * 100);
    ivec[1] = RwFastRealToUInt32(normal->y * normal->y * 100);
    ivec[2] = RwFastRealToUInt32(normal->z * normal->z * 100);
    axis = rwXPLANE_LtMap;

    if (ivec[rwYPLANE_LtMap] > ivec[axis])
        axis = rwYPLANE_LtMap;

    if (ivec[rwZPLANE_LtMap] > ivec[axis])
        axis = rwZPLANE_LtMap;

    if (rtLTMAPGETCOORD(normal, axis) < 0.0f)
        axis += 3;

    RWRETURN(axis);
}
