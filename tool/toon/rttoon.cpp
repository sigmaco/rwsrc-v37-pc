/*****************************************************************************

    File: rttoon.c

    Purpose: A short description of the file.

    Copyright (c) 2002 Criterion Software Ltd.

 */

/**
 * \defgroup rttoon RtToon
 * \ingroup fxpack
 *
 * Toon Toolkit for RenderWare.
 */

/**
 * \defgroup rttoongeo RtToonGeo
 * \ingroup rttoon
 */

/**
 * \ingroup rttoon
 * \page rttoonoverview RtToon Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rptoon.h rttoon.h
 * \li \b Libraries: rwcore, rpworld, rptoon, rttoon
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach,
 *                               \ref RpToonPluginAttach
 *
 * \subsection toontoolkitoverview Overview
 *
 * RtToon provides functionality for creating and editing toon data. The
 * functionality is only normally used in the art path and as such can safely
 * ignore unless you need to customize the tool chain.
 *
 * \see \ref rptoon
 */

/*****************************************************************************
 Includes
 */
#if (defined(_MSC_VER))
#pragma warning( disable : 4786 4530 )
#endif

#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <set>
#include <string>

#include <rwcore.h>
#include <rpworld.h>
#include <rptoon.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rttoon.h"

/*****************************************************************************
 Defines
 */

/*****************************************************************************
 Enums
 */

/*****************************************************************************
 Typedef Enums
 */

/*****************************************************************************
 Typedef Structs
 */

/*****************************************************************************
 Function Pointers
 */

/*****************************************************************************
 Structs
 */

/*****************************************************************************
 Static Variables
 */

/*****************************************************************************
 Global Variables
 */

/*****************************************************************************
 Function prototypes
 */

/*****************************************************************************
 Functions
 */

/*****************************************************************************
 ToonEdgeCtor
 */
static RpToonEdge
ToonEdgeCtor(rpToonVertexIndex _v0,
             rpToonVertexIndex _v1,
             rpToonFaceIndex _face0,
             rpToonFaceIndex _face1)
{
    RpToonEdge temp;

    RWFUNCTION(RWSTRING("ToonEdgeCtor"));

    temp.v[0] = _v0;
    temp.v[1] = _v1;
    temp.face[0] = _face0;
    temp.face[1] = _face1;

    RWRETURN( temp );
}

typedef std::list<RwInt32> IncidentEdgeList;
typedef std::vector<IncidentEdgeList> VertexIncidentEdges;

/*****************************************************************************
 FindNextEdge
 */
static rpToonEdgeIndex
FindNextEdge(const VertexIncidentEdges &vertexIncidentEdges,
             rpToonVertexIndex v, rpToonFaceIndex f,
             rpToonEdgeIndex notThisEdge, RpToonEdge *edges )
{
    RWFUNCTION(RWSTRING("FindNextEdge"));

    if (f == RPTOONEDGE_NONEIGHBOURFACE)
    {
        RWRETURN(noNextEdge);
    }

    for (IncidentEdgeList::const_iterator i = vertexIncidentEdges[v].begin();
         i != vertexIncidentEdges[v].end(); ++i)
    {
        rpToonEdgeIndex incidentEdge = *i;

        if  (   (incidentEdge != notThisEdge)
                &&  (edges[incidentEdge].v[0] == v || edges[incidentEdge].v[1] == v)
                &&  (edges[incidentEdge].face[0] == f || edges[incidentEdge].face[1] == f)    )
        {
            RWRETURN(incidentEdge);
        }

        RWASSERT(edges[incidentEdge].face[0] != RPTOONEDGE_NONEIGHBOURFACE);
        /* RWASSERT(edges[incidentEdge].face[1] != RPTOONEDGE_NONEIGHBOURFACE); */
    }

    /* RWASSERT(0 && "impossible!"); */

    RWRETURN(noNextEdge);
}

/*****************************************************************************
 FindWingEdges
 */
static void
FindWingEdges(RpToonEdge *edges, RwInt32 numEdges, RwInt32 numVerts)
{

    /* for each vertex, make a list of all edges incident */
    VertexIncidentEdges vertexIncidentEdges(numVerts);
    rpToonEdgeIndex edge;
    RWFUNCTION(RWSTRING("FindWingEdges"));

    for (edge = 0; edge < numEdges; edge++)
    {
        vertexIncidentEdges[edges[edge].v[0]].push_back(edge);
        vertexIncidentEdges[edges[edge].v[1]].push_back(edge);
    }

    /* for each edge, add wing edges that share a vertex & face */
    for (edge = 0; edge < numEdges; edge++)
    {
        /* first vertex */

        /* first face */
        edges[edge].edgefv[0][0] = FindNextEdge(vertexIncidentEdges, edges[edge].v[0], edges[edge].face[0], edge, edges );

        /* 2nd face */
        edges[edge].edgefv[1][0] = FindNextEdge(vertexIncidentEdges, edges[edge].v[0], edges[edge].face[1], edge, edges );

        /* 2nd vertex */

        /* first face */
        edges[edge].edgefv[0][1] = FindNextEdge(vertexIncidentEdges, edges[edge].v[1], edges[edge].face[0], edge, edges );

        /* 2nd face */
        edges[edge].edgefv[1][1] = FindNextEdge(vertexIncidentEdges, edges[edge].v[1], edges[edge].face[1], edge, edges );
    }

    RWRETURNVOID();
}

/* does some edge have this vertex as a head already? */
typedef std::multimap<rpToonVertexIndex, rpToonEdgeIndex> HeadVertexToEdgeMap;

/*****************************************************************************
 ContainsEdge
 */
static bool
ContainsEdge( HeadVertexToEdgeMap &headVertexToEdgeMap, RpToonEdge *edges,
              rpToonVertexIndex v0, rpToonVertexIndex v1 )
{
    typedef std::pair<HeadVertexToEdgeMap::iterator, HeadVertexToEdgeMap::iterator> Range;

    Range heads = headVertexToEdgeMap.equal_range(v0);

    RWFUNCTION(RWSTRING("ContainsEdge"));

    for (HeadVertexToEdgeMap::iterator head = heads.first; head != heads.second; ++head)
    {
        rpToonEdgeIndex edge = head->second;

        if ((edges[ edge ].v[1] == v1)) /*  || (edges[ edge ].v0 == v1)) */
        {
            /* head and tail match, we've already got this edge */
            RWRETURN( true );
        }
    }

    RWRETURN( false );
}

typedef std::list<RwInt32> VertTriangles;
typedef std::vector<VertTriangles> VertTrianglesContainer;
typedef  VertTrianglesContainer DuplicateVertContainer;

/*****************************************************************************
 FindNeighbourTriangle
 */
static RwInt32
FindNeighbourTriangle( VertTrianglesContainer & vertTriangles,
                       RwInt32 v0, RwInt32 v1, RwInt32 tri,
                       rpToonTriangle *tris )
{
    RWFUNCTION(RWSTRING("FindNeighbourTriangle"));

    for (VertTriangles::iterator t = vertTriangles[v0].begin();
         t != vertTriangles[v0].end(); ++t)
    {
        RwInt32 neighbourTri = *t;
        if (neighbourTri != tri) /* it's not this tri  */
        {
            /* do any of the edges of this shared tri match this edge? */
            for (RwInt32 j=0; j<3; j++)
            {
                if (tris[neighbourTri].vertIndex[j] == v1)
                {
                    /* shares head and tail vertex, so it's a neighbour tri */
                    RWRETURN( neighbourTri );
                }
            }
        }
    }

    RWRETURN( RPTOONEDGE_NONEIGHBOURFACE );
}

typedef std::vector<RwInt32> IndexRemap;

/*****************************************************************************
 RemapTriangleIndices
 */
void
RemapTriangleIndices(RwInt32 numTris, rpToonTriangle * tris,
                     RwInt32 numVerts, const RwV3d *positions,
                     RwInt32 *indexRemap )
{
    /*
      Find vertices that have same position but different indices
      (possibly replicated because of different UVs, smoothing groups,
      materials, tristripper artifacts, dodgy meshing,

      Replace any such reference in the triangle list to just one of that set
      so adjacency info for edges doesn't end up thinking tri doesn't have neighbours
      when it really does.
    */
    RwInt32 duplicateVertices = 0;

    typedef std::multimap<RwReal, RwUInt32> VertexMap;

    VertexMap vertexMap;
    RWFUNCTION(RWSTRING("RemapTriangleIndices"));


    for (RwInt32 i=0; i<numVerts; i++)
    {
        RwReal key = positions[i].x + positions[i].y + positions[i].z;

        typedef std::pair<VertexMap::iterator, VertexMap::iterator> Range;

        Range verts = vertexMap.equal_range(key);

        bool isDuplicate = false;

        for (VertexMap::iterator v = verts.first; v != verts.second; ++v)
        {
            RwUInt32 uniqueIndex = v->second;

            if (    (positions[uniqueIndex].x == positions[i].x)
                    &&  (positions[uniqueIndex].y == positions[i].y)
                    &&  (positions[uniqueIndex].z == positions[i].z)
                    )
            {
                /* it's a duplicate, remap it to first one we found */
                isDuplicate = true;

                indexRemap[i] = uniqueIndex;

                duplicateVertices++;
                break;
            }
        }

        if (!isDuplicate)
        {
            /* it's a new vertex, add it to hash */
            vertexMap.insert(std::pair<RwReal, RwUInt32>(key, i));

            indexRemap[i] = i;
        }
    }

    /* point all the triangles at the first vertex in the case of duplicated positions */
    for (RwInt32 tri=0; tri<numTris; tri++)
    {
        for (RwInt32 v=0; v<3; v++)
        {
            tris[tri].vertIndex[v] = indexRemap[tris[tri].vertIndex[v]];
        }
    }

    RWRETURNVOID();
}

/*****************************************************************************
 ComputeVertexNormalsFromAveragedFaceNormals
 */
static void
ComputeVertexNormalsFromAveragedFaceNormals(const VertTrianglesContainer &vertTriangles,
                                            RwV3d *faceNormals,
                                            RwV3d *averageVertexNormals)
{
    /*
      Make a vertex normal as the average of all the normals of faces incident on that vertex
      to extrude *away* from the object - smoothing groups cause the "usual" vertex
      normals to look quite bad on some models from some views, e.g. cubes.
    */

    RWFUNCTION(RWSTRING("ComputeVertexNormalsFromAveragedFaceNormals"));


    for (RwUInt32 i=0; i<vertTriangles.size(); i++)
    {
        RwV3d normal = { 0.0f, 0.0f, 0.0f };

        for (VertTriangles::const_iterator j = vertTriangles[i].begin();
             j != vertTriangles[i].end(); ++j)
        {
            RwV3dAdd(&normal, &normal, &faceNormals[ *j ]);
        }

        if (!vertTriangles[i].empty()) /* might have removed this vertex asa duplicate */
        {
            RwV3dNormalize(&averageVertexNormals[i], &normal);
        }
    }

    RWRETURNVOID();
}

/*****************************************************************************
 ComputeExtrusionNormalsFromSharedVertexNormals
 */
static void
ComputeExtrusionNormalsFromSharedVertexNormals(RwInt32 numVerts,
                                               const RwV3d *vertexNormals,
                                               const IndexRemap & positionsRemap,
                                               RwV3d *extrusionNormals,
                                               RwInt32 *vertexNormalRemap)
{
    RwInt32 i;

    RWFUNCTION(RWSTRING("ComputeExtrusionNormalsFromSharedVertexNormals"));

    /* start extrusion normals as 0 */
    for (i=0; i<numVerts; i++)
    {
        extrusionNormals[i].x =
            extrusionNormals[i].y =
            extrusionNormals[i].z = 0.0f;
    }

    typedef std::list<RwInt32> IndexList;

    typedef std::vector< IndexList > NormalIndexList;

    NormalIndexList normalIndexList(numVerts);

    /* for each vertex */
    for (i=0; i<numVerts; i++)
    {
        /* add vertex normal to unique position vertex if we've not already added it */
        RwInt32 uniquePosition = positionsRemap[i];

        bool unique = true;

        for (IndexList::iterator n = normalIndexList[uniquePosition].begin();
             n != normalIndexList[uniquePosition].end(); ++n)
        {
            if (    (vertexNormals[i].x == vertexNormals[ *n ].x)
                    &&  (vertexNormals[i].y == vertexNormals[ *n ].y)
                    &&  (vertexNormals[i].z == vertexNormals[ *n ].z))
            {
                unique = false;

                vertexNormalRemap[i] = *n;

                break;
            }
        }

        if (unique)
        {
            normalIndexList[uniquePosition].push_back(i);

            extrusionNormals[uniquePosition].x += vertexNormals[i].x;
            extrusionNormals[uniquePosition].y += vertexNormals[i].y;
            extrusionNormals[uniquePosition].z += vertexNormals[i].z;

            vertexNormalRemap[i] = i;
        }
    }

    /* update count of vertex normals at each vertex to identify smoothing group boundaries */
    /*
      for (RwInt32 i=0; i<numVerts; i++)
      {
      vertexNormalCounts[i] = normalIndexList[i].size();
      }
    */

    /* renormalize averaged normals */
    for (i=0; i<numVerts; i++)
    {
        /* avoid asserts about 0 length vectors */
        if (    extrusionNormals[i].x != 0.0f
                &&   extrusionNormals[i].y != 0.0f
                &&   extrusionNormals[i].z != 0.0f  )
        {
            RwV3dNormalize( &extrusionNormals[i], &extrusionNormals[i] );
        }
        else
        {
            extrusionNormals[i] = vertexNormals[i];
        }
    }

    /* make sure all cloned positions have same extrusion normal */
    for (i=0; i<numVerts; i++)
    {
        RwInt32 uniquePositionIndex = positionsRemap[i];
        if (i != uniquePositionIndex)
        {
            extrusionNormals[i] = extrusionNormals[uniquePositionIndex];
        }
    }

    RWRETURNVOID();
}

/*****************************************************************************
 FindOriginalIndex
 */
inline RwInt32
FindOriginalIndex(const IndexRemap &indexRemap,
                  const rpToonTriangle *rawTriangle,
                  RwInt32 remappedVertexIndex)
{
    RWFUNCTION(RWSTRING("FindOriginalIndex"));

    for (RwInt32 i=0; i < 3; i++)
    {
        RwInt32 originalIndex = rawTriangle->vertIndex[i];

        if (indexRemap[originalIndex] == remappedVertexIndex)
        {
            RWRETURN( originalIndex );
        }
    }

    RWASSERT(0 && "index not found, impossible!!!");
    RWRETURN(-1);
}

/*****************************************************************************
 IsCreaseVertex
 */
inline bool
IsCreaseVertex(const IndexRemap &indexRemap,
               RwInt32 remappedVertexIndex,
               const IndexRemap &vertexNormalRemap,
               const rpToonTriangle *face0,
               const rpToonTriangle *face1)
{
    RwInt32 f0v = FindOriginalIndex(indexRemap, face0, remappedVertexIndex ),
        f1v = FindOriginalIndex(indexRemap, face1, remappedVertexIndex );
    RWFUNCTION(RWSTRING("IsCreaseVertex"));

    RWRETURN( vertexNormalRemap[f0v] != vertexNormalRemap[f1v] );
}

namespace
{
typedef struct IsCreaseEdge IsCreaseEdge;
struct IsCreaseEdge
{
        const rpToonTriangle *m_rawTris;
        const IndexRemap &m_indexRemap;
        const IndexRemap &m_vertexNormalRemap;

        inline IsCreaseEdge(const rpToonTriangle * rawTris,
                            const IndexRemap &indexRemap,
                            const IndexRemap &vertexNormalRemap);
        inline bool operator()(const RpToonEdge &edge);
};

inline
IsCreaseEdge::IsCreaseEdge(const rpToonTriangle * rawTris,
                           const IndexRemap &indexRemap,
                           const IndexRemap &vertexNormalRemap)
        : m_rawTris(rawTris),
          m_indexRemap(indexRemap),
          m_vertexNormalRemap(vertexNormalRemap)
{
    RWFUNCTION(RWSTRING("IsCreaseEdge::IsCreaseEdge"));

    RWRETURNVOID();
}

inline bool
IsCreaseEdge::operator()(const RpToonEdge &edge)
{
    RWFUNCTION(RWSTRING("IsCreaseEdge::operator()"));
    /* take object boundaries to be silhouettes instead */

    RWRETURN( (edge.face[1] != RPTOONEDGE_NONEIGHBOURFACE )
              && IsCreaseVertex( m_indexRemap,
                                 edge.v[0],
                                 m_vertexNormalRemap,
                                 &m_rawTris[ edge.face[0] ],
                                 &m_rawTris[ edge.face[1] ] )
              && IsCreaseVertex( m_indexRemap,
                                 edge.v[1],
                                 m_vertexNormalRemap,
                                 &m_rawTris[ edge.face[0] ],
                                 &m_rawTris[ edge.face[1] ] ) );
}

} /* namespace */

struct _rpToonEdgeReorder
{
        rpToonEdgeIndex edgeIndex;
        RwReal  angle;

        inline bool operator<(const _rpToonEdgeReorder & b) const
        {  return angle < b.angle; }
};

/*****************************************************************************
 _rpToonReorderEdgesByDihedralAngle
 */
RwBool
_rpToonReorderEdgesByDihedralAngle(RpToonGeo *toonGeo)
{
    rpToonEdgeIndex edgeIndex;
    RpToonEdge *newEdges;

    typedef std::vector<_rpToonEdgeReorder> EdgeReorderVec;

    EdgeReorderVec edgeReorderVec(toonGeo->numEdges);

    RWFUNCTION(RWSTRING("_rpToonReorderEdgesByDihedralAngle"));

    for (edgeIndex=0; edgeIndex<toonGeo->numEdges; edgeIndex++)
    {
        RwInt32 face0Index = toonGeo->edges[edgeIndex].face[0],
            face1Index = toonGeo->edges[edgeIndex].face[1];

        RWASSERT(face0Index != RPTOONEDGE_NONEIGHBOURFACE);
        RWASSERT(face0Index != face1Index);

        edgeReorderVec[edgeIndex].edgeIndex = edgeIndex;

        if (face1Index != RPTOONEDGE_NONEIGHBOURFACE)
        {
            edgeReorderVec[edgeIndex].angle = rwPI - RwV3dDotProduct(&toonGeo->faceNormals[face0Index],
                                                                     &toonGeo->faceNormals[face1Index]);
        }
        else
        {
            /* open edges are totally useless - put them at the end */
            edgeReorderVec[edgeIndex].angle = rwPI;
        }
    }

    std::sort(edgeReorderVec.begin(), edgeReorderVec.end());

    newEdges = (RpToonEdge *)RwMalloc(sizeof(RpToonEdge) * toonGeo->numEdges,
        rwID_TOONPLUGIN | rwMEMHINTDUR_EVENT);
    RWASSERT(newEdges&&"RwMalloc failed");
    if (!newEdges)
    {
        RWRETURN(FALSE);
    }

    for (edgeIndex=0; edgeIndex<toonGeo->numEdges; edgeIndex++)
    {
        rpToonEdgeIndex sourceEdgeIndex = edgeReorderVec[edgeIndex].edgeIndex;

        newEdges[edgeIndex] = toonGeo->edges[sourceEdgeIndex];

        /* to do - winged edge remapping */
    }

    /* swap old edges for reordered edges */
    RwFree(toonGeo->edges);
    toonGeo->edges = newEdges;

    RWRETURN(TRUE);
}

/*****************************************************************************
 ToonComputeFaceNormals
 */
static void
ToonComputeFaceNormals(RwInt32 numTris,
                       const rpToonTriangle * tris,
                       const RwV3d *vertices,
                       RwV3d *faceNormals)
{
    RwInt32 i;
    RwV3d   e0;
    RwV3d   e1;

    RWFUNCTION(RWSTRING("ToonComputeFaceNormals"));

    RWASSERT( tris != 0 );
    RWASSERT( vertices != 0 );
    RWASSERT( faceNormals != 0 );

    for (i = 0; i<numTris; i++)
    {
        RwV3dSub(&e0, &vertices[ tris[i].vertIndex[1] ], &vertices[ tris[i].vertIndex[0] ]);
        RwV3dSub(&e1, &vertices[ tris[i].vertIndex[2] ], &vertices[ tris[i].vertIndex[0] ]);
        RwV3dCrossProduct(&faceNormals[i], &e0, &e1);
    }

    RWRETURNVOID();
}

/*****************************************************************************
 _rpToonCreateEdgeInfo
 */
void
_rpToonCreateEdgeInfo(RpToonGeo *toonGeo,
                      const RwV3d *positions,
                      const RwV3d *vertexNormals,
                      RwBool generateCreaseEdgesFromSmoothingGroups)
{
    RWFUNCTION(RWSTRING("_rpToonCreateEdgeInfo"));

    RWASSERT(toonGeo != 0);

    /*
      Determine list of edges from geometry
      Could & should do this on export or preinstance
    */

    RwInt32 numTris = toonGeo->numTriangles,
        numVerts = toonGeo->numVerts;

    rpToonTriangle * rawTris = toonGeo->triangles;
    std::vector<rpToonTriangle> tempTris(numTris);
    rpToonTriangle *tris = &tempTris[0];
    memcpy(tris, rawTris, sizeof(rpToonTriangle) * numTris);

    /*
      Find vertices that have same position but different indices
      (possibly replicated because of different UVs, smoothing groups,
      materials, tristripper artifacts, dodgy meshing,

      Replace any such reference in the triangle list to just one of that set
      so adjacency info for edges doesn't end up thinking tri doesn't have neighbours
      when it really does.
    */
    IndexRemap indexRemap(numVerts);

    RemapTriangleIndices(numTris, tris, numVerts, positions, &indexRemap[0]);

#if (0)
    RwUInt16            vertIndex[3]; /**< vertex indices */
    RwInt16             matIndex; /**< Index into material list */
#endif /* (0) */

    /* for each vertex, build a list of all triangles that reference it */
    /* so we can find the polys an edge straddles quickly */

    VertTrianglesContainer vertTriangles(numVerts);

    /* foreach tri */
    RwInt32 tri;
    for (tri=0; tri<numTris; tri++)
    {
        /*
          RWASSERT(tris[tri].vertIndex[0] != tris[tri].vertIndex[1]
          && tris[tri].vertIndex[0] != tris[tri].vertIndex[2]
          && tris[tri].vertIndex[1] != tris[tri].vertIndex[2]
          && "pesky degenerate triangles" );
        */

        /* if triangle's NOT a degenerate (we don't need degenerate edges!) */
        if (tris[tri].vertIndex[0] != tris[tri].vertIndex[1]
            && tris[tri].vertIndex[0] != tris[tri].vertIndex[2]
            && tris[tri].vertIndex[1] != tris[tri].vertIndex[2])
        {
            /*   foreach trivert */
            for (RwInt32 i=0; i<3; i++)
            {
                /*       add tri index to vert's tri list */
                vertTriangles[tris[tri].vertIndex[i]].push_back(tri);
            }
        }
    }

    HeadVertexToEdgeMap headVertexToEdgeMap;

    typedef std::vector<RpToonEdge> EdgeContainer;

    EdgeContainer edges;

    /* for each tri */
    for (tri=0; tri<numTris; tri++)
    {
        /*   for each edge (vert (0,1), (1,2), (2,0)) */
        for (RwInt32 i=0; i<3; i++)
        {
            /* ensure that v0, v1 are traversed in same direction as they are in tri (face0) */
            /* (and hence the opposite direction to face1) so we can easily tell if we're */
            /* going clockwise or counter clockwise around a vertex later when extruding crease edges */
            RwInt32 v0 = tris[tri].vertIndex[i];
            RwInt32 v1 = tris[tri].vertIndex[(i + 1) % 3];

			RpToonEdge* pEdges =  ( edges.size() > 0 ) ? &edges[0] : NULL;

            if ((v0 != v1) /* no degenerate edges please */
                && !ContainsEdge( headVertexToEdgeMap, pEdges, v0, v1 ) /* edge not already in edges */
                && !ContainsEdge( headVertexToEdgeMap, pEdges, v1, v0 ))
            {
                /*           create new edge, reference neighbour triangles */
                RwInt32 face1 = FindNeighbourTriangle( vertTriangles, v0, v1, tri, tris );

                headVertexToEdgeMap.insert( std::pair<RwInt32, RwInt32>(v0, edges.size()) );

                edges.push_back( ToonEdgeCtor(v0, v1, tri, face1));
            }
        }
    }

    /*
      Sanity check: number edges for a closed mesh should be E = V + F - 2 (Euler's Thm).
    */
    toonGeo->numEdges = edges.size();
    toonGeo->edges = (RpToonEdge *) RwMalloc( toonGeo->numEdges * sizeof(RpToonEdge),
                                              rwID_TOONPLUGIN | rwMEMHINTDUR_EVENT );

    RWASSERT((toonGeo->edges != 0) && "RwMalloc failed");

    memcpy(toonGeo->edges, &edges[0], toonGeo->numEdges * sizeof(RpToonEdge));

    /*
      Get some face normals for silhoutte test
      need to be recalculated for skinning/morphing
    */
    /*   toonGeo->faceNormals = (RwV3d *) RwMalloc( numTris * sizeof(RwV3d),
                                     rwID_TOONPLUGIN | rwMEMHINTDUR_EVENT ); */
    /*   RWASSERT((toonGeo->faceNormals != 0) && "RwMalloc failed"); */

    if (positions) /* n.b. might be a space filling world sector */
    {
        RwInt32 i;

        ToonComputeFaceNormals( numTris, tris, positions, toonGeo->faceNormals );

        /*
          Doesn't need to be normalized for silhoutte edge detection,
          But helps for working out averaged face normals
        */
        for (i = 0; i<numTris; i++)
        {
            RwV3dNormalize( &toonGeo->faceNormals[i], &toonGeo->faceNormals[i] );
        }
    }

    /*
      Create a set of normals to extrude along (smoothing group normals aren't always very good)
    */
    if (vertexNormals)
    {
        /*  toonGeo->extrusionVertexNormals = (RwV3d *)RwMalloc(sizeof(RwV3d) * numVerts,
                                                    rwID_TOONPLUGIN | rwMEMHINTDUR_EVENT); */

        /* averaged face normals can give a bad weighting where several incident faces have same normal */
        /*     ComputeVertexNormalsFromAveragedFaceNormals( */
        /*         vertTriangles, toonGeo->faceNormals, toonGeo->extrusionVertexNormals ); */

        /* average of unique vertex normals on a vertex seems pretty good */
        IndexRemap vertexNormalRemap(numVerts);

        ComputeExtrusionNormalsFromSharedVertexNormals( numVerts, vertexNormals, indexRemap,
                                                        toonGeo->extrusionVertexNormals, &vertexNormalRemap[0] );

        /*     toonGeo->numCreaseEdges = FindCreaseEdges(toonGeo->numEdges, toonGeo->edges, rawTris, indexRemap, vertexNormalRemap ); */

        if (generateCreaseEdgesFromSmoothingGroups)
        {
            /*
              Put all the crease edges together at the front of the array
              so we can iterate over them easily when rendering them
            */
            RpToonEdge *endCreaseEdge = std::partition(
                                                       toonGeo->edges, toonGeo->edges + toonGeo->numEdges,
                                                       IsCreaseEdge(rawTris, indexRemap, vertexNormalRemap) );

            toonGeo->numCreaseEdges = std::distance(toonGeo->edges, endCreaseEdge );
        }
    }
    else
    {
        /* well this is going to bugger silhouettes on PS2, but could actually be an optimization */
        /* for the generic algorithm. prolly should do an exporter warning and fill in some */
        /* data that will not crash later */

        /* averaged face normals can give a bad weighting where several incident faces have same normal */
        ComputeVertexNormalsFromAveragedFaceNormals(
                                                    vertTriangles, toonGeo->faceNormals, toonGeo->extrusionVertexNormals );
    }

    /*
      Initialize ink edge IDs to defaults
    */
    toonGeo->edgeInkIDs = (rpToonEdgeInkID *) RwMalloc( toonGeo->numEdges * sizeof(rpToonEdgeInkID),
        rwID_TOONPLUGIN | rwMEMHINTDUR_EVENT );
    for (RwInt32 i=0; i<toonGeo->numEdges; i++)
    {
        toonGeo->edgeInkIDs[i].inkId[RPTOON_INKTYPE_SILHOUETTE] = RPTOON_INKTYPE_SILHOUETTE;

        /* if it's a crease edge, give it the crease ink */
        if (i < toonGeo->numCreaseEdges)
        {
            toonGeo->edgeInkIDs[i].inkId[RPTOON_INKTYPE_CREASE] = RPTOON_INKTYPE_CREASE;
        }
        /* otherwise give it a 0 to signal it shouldn't be drawn unless it's a silhoutte */
        else
        {
            toonGeo->edgeInkIDs[i].inkId[RPTOON_INKTYPE_CREASE] = RPTOON_INKTYPE_SILHOUETTE;
        }
    }

    /*     _rpToonReorderEdgesByDihedralAngle(toonGeo); */

    /*
      find connectivity between edges *after* rearranging them so indices match
    */
    FindWingEdges( toonGeo->edges,  toonGeo->numEdges, numVerts );

    RWRETURNVOID();
}

/**
 * \ingroup rttoongeo
 * \ref RtToonGeoGetNumEdges is used to retrieve the number of edges
 * RpToon built from the source geometry.
 *
 * \param toonGeo   A pointer to toon geometry.
 *
 * \return RwInt32 number of edges.
 */
extern "C" RwInt32
RtToonGeoGetNumEdges(const RpToonGeo *toonGeo)
{
    RWAPIFUNCTION(RWSTRING("RtToonGeoGetNumEdges"));
    RWASSERT(toonGeo);
    RWASSERT(0 < _rpToonGlobals.module.numInstances);

    RWRETURN(toonGeo->numEdges);
}

/**
 * \ingroup rttoongeo
 * \ref RtToonGeoGetEdges is used to retrieve a pointer of the edges
 * RpToon built from the source geometry
 *
 * \param toonGeo   A pointer to toon geometry.
 *
 * \return          A pointer to the edge list.
 */
extern "C" RpToonEdge *
RtToonGeoGetEdges(const RpToonGeo *toonGeo)
{
    RWAPIFUNCTION(RWSTRING("RtToonGeoGetEdges"));

    RWASSERT(toonGeo);
    RWASSERT(0 < _rpToonGlobals.module.numInstances);

    RWRETURN(toonGeo->edges);
}

/**
 * \ingroup rttoongeo
 * \ref RtToonGeoFindEdgeIndex returns the index of the toon edge which lies between
 * vertices with indices rwV0 and rwV1. This index
 * can then be used to specify ink properties for the edge via RtToonGeoSetEdgeInkID.
 *
 * It is an error to ask for an edge that doesn't exist.
 *
 * This function is very expensive, should be called at export time only and therefore
 * is only available on PC platforms
 *
 * \param toonGeo the \ref RpToonGeo
 * \param rwV0 the first vertex index, corresponding to a remapped vertex from RtToonGeoGetVertexRemap
 * \param rwV1 the 2nd vertex index, corresponding to a remapped vertex from RtToonGeoGetVertexRemap
 * \return the edge index.
 *
 * \see RtToonGeoGetNumEdges
 * \see RtToonGeoGetEdges
 * \see RtToonGeoGetVertexRemap
 * \see RpToonInkType
 * \see RtToonGeoGetInkIDFromName
 * \see RtToonGeoOptimizeEdgeInkIDs
 */
extern "C"  RwUInt16
RtToonGeoFindEdgeIndex(const RpToonGeo *toonGeo,
                       RwInt32 rwV0,
                       RwInt32 rwV1)
{
    RwUInt16 i;

    RWAPIFUNCTION(RWSTRING("RtToonGeoFindEdgeIndex"));
    RWASSERT(0 < _rpToonGlobals.module.numInstances);

    RWASSERT(toonGeo != 0);
    RWASSERT(rwV0 >= 0 && rwV0 < toonGeo->numVerts);
    RWASSERT(rwV1 >= 0 && rwV1 < toonGeo->numVerts);

    for (i=0; i<toonGeo->numEdges; i++)
    {
        if ((toonGeo->edges[i].v[0] == rwV0 && toonGeo->edges[i].v[1] == rwV1) ||
            (toonGeo->edges[i].v[1] == rwV0 && toonGeo->edges[i].v[0] == rwV1))

        {
            RWRETURN(i);
        }
    }

    RWASSERT(0&&"no matching edge found");

    RWRETURN(RPTOONEDGE_NONEIGHBOURFACE);
}

/**
 * \ingroup rttoongeo
 * \ref RtToonGeoSetEdgeInkID is used to tag a particular \ref RpToonEdge with
 * an \ref RpToonInkType and a \ref RpToonInk to use when rendering.
 *
 * You don't need to worry about this unless you're writing an exporter.
 *
 * Flagging edges arbitrarily is convienient for specifying edges in an art package,
 * but very tedious for rendering. So when you've finished using \ref RtToonGeoSetEdgeInkID
 * to change an \ref RpToonGeo's edge's inks, you MUST call \ref RtToonGeoOptimizeEdgeInkIDs
 * to pack the changes into a format convientient for rendering.  This will reorder the
 * edges, restrip all the meshes and possibly introduce new materials to accomodate
 * multiple silhouette ink styles.
 *
 * In the case of worlds you should use \ref RtToonGeoSetEdgeInkID on all the world sectors,
 * then call \ref RtToonWorldOptimize instead of \ref RtToonGeoOptimizeEdgeInkIDs when you've finished.
 *
 * \param toonGeo the \ref RpToonGeo
 * \param edgeIndex the edge to identify
 * \param type the \ref RpToonInkType to render this edge with
 * \param inkID the ink ID, gotten from \ref RtToonGeoGetInkIDFromName, to render this edge with.
 *
 * \see RtToonGeoGetNumEdges
 * \see RtToonGeoGetEdges
 * \see RpToonInkType
 * \see RtToonGeoGetInkIDFromName
 * \see RtToonGeoOptimizeEdgeInkIDs
 * \see RtToonWorldOptimize
 *
 * \return none.
 */
extern "C" void
RtToonGeoSetEdgeInkID(RpToonGeo *toonGeo,
                      RwUInt16 edgeIndex,
                      RpToonInkType type,
                      RwInt32 inkID)
{
    RWAPIFUNCTION(RWSTRING("RtToonGeoSetEdgeInkID"));
    RWASSERT(0 < _rpToonGlobals.module.numInstances);

    RWASSERT(toonGeo);
    RWASSERT(edgeIndex < toonGeo->numEdges);
    RWASSERT(inkID >= 0 && inkID < toonGeo->inkIDCount );

    toonGeo->edgeInkIDs[edgeIndex].inkId[type] = inkID;

    /* We'll need to re-optimize the edges before rendering now */
    toonGeo->edgeInkIDsOptimized = FALSE;

    RWRETURNVOID();
}

/**
 * \ingroup rttoongeo
 * \ref RtToonGeoGetVertexRemap attempts to find unique edges on a mesh to minimize the number of edges
 * that need to be tested and rendered.  Mesh rendering on the other hand dictates that
 * vertices must be replicated where normals, UVs, colors, etc, are not shared.
 * This function analyzes a set of RwV3d positions which may contain duplicates,
 * and for each vertex puts the index of the first unique position in the vertexRemap.
 * this gives a mapping of RenderWare geometry vertices to toon vertices, hence
 * a toon edge can be identified by 2 RenderWare geometry vertex indices "remapped" off
 * this array.
 *
 * This function is very expensive, should be called at export time only and therefore
 * is only available on PC platforms
 *
 * \param toonGeo the \ref RpToonGeo
 * \param positions a set of vertex positions to analyze
 * \param vertexRemap an array of RwInt32s (one for each vertex) to hold the result.
 *
 * \see RtToonGeoGetNumEdges
 * \see RtToonGeoGetEdges
 * \see RtToonGeoFindEdgeIndex
 * \see RpToonInkType
 * \see RtToonGeoGetInkIDFromName
 * \see RtToonGeoOptimizeEdgeInkIDs
 * \see RtToonGeoFindEdgeIndex
 */
extern "C" void
RtToonGeoGetVertexRemap(const RpToonGeo *toonGeo,
                        const RwV3d *positions,
                        RwInt32 *vertexRemap)
{
    RWAPIFUNCTION(RWSTRING("RtToonGeoGetVertexRemap"));
    RWASSERT(0 < _rpToonGlobals.module.numInstances);

    RwInt32 numTris = toonGeo->numTriangles,
        numVerts = toonGeo->numVerts;

    rpToonTriangle * rawTris = toonGeo->triangles;
    std::vector<rpToonTriangle> tempTris(numTris);
    rpToonTriangle *tris = &tempTris[0];
    memcpy(tris, rawTris, sizeof(rpToonTriangle) * numTris);

    /*
      Find vertices that have same position but different indices
      (possibly replicated because of different UVs, smoothing groups,
      materials, tristripper artifacts, dodgy meshing,

      Replace any such reference in the triangle list to just one of that set
      so adjacency info for edges doesn't end up thinking tri doesn't have neighbours
      when it really does.
    */

    RemapTriangleIndices(numTris, tris, numVerts, positions, vertexRemap);

    RWRETURNVOID();
}

/**
 * \ingroup rttoongeo
 * \ref RtToonGeoGetInkIDFromName is used to get from an ink name an RwInt32 identifying
 * that ink uniquely for this \ref RpToonGeo.  If the \ref RpToonGeo doesn't have an
 * ID for that name already, it will make a new one and note the ink name.  Otherwise it
 * will return the existing ID.  This ID can then be used to specify per edge which ink
 * style an edge is meant to use with \ref RtToonGeoSetEdgeInkID.
 *
 * It's done this way because each edge needs to reference an ink, but names are too long
 * to store efficiently, pointers don't stream well, and it's assumed that you need only
 * a few inks per object anyway.
 *
 * You don't need to worry about this unless you're writing an exporter.
 *
 * \param toonGeo the \ref RpToonGeo
 * \param inkName the name of the \ref RpToonInk to identify
 *
 * \return an RwInt32 indentifying this ink name with respect to this \ref RpToonGeo
 *
 * \see RtToonGeoSetEdgeInkID
 */
extern "C" RwInt32
RtToonGeoGetInkIDFromName(RpToonGeo *toonGeo,
                          const RwChar *inkName)
{
    RwInt32 inkId = -1, i;
    rpToonInkID *oldInkIDs;
    RpToonInk **oldInks;

    RWAPIFUNCTION(RWSTRING("RtToonGeoGetInkIDFromName"));
    RWASSERT(0 < _rpToonGlobals.module.numInstances);

    /* have we got it already ?
    Starting searching after default silhouette ink ID because we
    need something to uniquely identify crease edges as distinct from silhouette edges
    see RtToonGeoOptimizeEdgeInkIDs()
    */
    for (i=RPTOON_INKTYPE_CREASE; i<toonGeo->inkIDCount; i++)
    {
        if (strncmp(inkName, toonGeo->inkIDList[i].name, rwTEXTUREBASENAMELENGTH - 1) == 0)
        {
            RWRETURN(i);
        }
    }

    /* nope grow some space for it */
    oldInkIDs = toonGeo->inkIDList;
    toonGeo->inkIDList = (rpToonInkID *)RwMalloc( sizeof(rpToonInkID) * (toonGeo->inkIDCount + 1),
                                                  rwID_TOONPLUGIN | rwMEMHINTDUR_EVENT );
    RWASSERT(toonGeo->inkIDList != 0 && "RwMalloc failed");

    if (oldInkIDs)
    {
        memcpy(toonGeo->inkIDList, oldInkIDs, sizeof(rpToonInkID) * toonGeo->inkIDCount );
        RwFree(oldInkIDs);
    }

    /* also grow space for ink list */
    oldInks = toonGeo->inks;
    toonGeo->inks = (RpToonInk **)RwMalloc( sizeof(RpToonInk *) * (toonGeo->inkIDCount + 1),
                                            rwID_TOONPLUGIN | rwMEMHINTDUR_EVENT );
    RWASSERT(toonGeo->inks != 0 && "RwMalloc failed");

    if (oldInks)
    {
        memcpy(toonGeo->inks, oldInks, sizeof(RpToonInk *) * toonGeo->inkIDCount );
        RwFree(oldInks);
    }

    inkId = toonGeo->inkIDCount;
    toonGeo->inkIDCount++;

    RWASSERT((toonGeo->inkIDCount < 256) && "Thats a lot of inks, is it worth the extra memory?");

    /* copy ink ID into new slot */
    rwstrncpy(toonGeo->inkIDList[inkId].name, inkName, rwTEXTUREBASENAMELENGTH - 1);

    /* we don't have an ink instance yet, so make it null */
    toonGeo->inks[inkId] = 0;

    RWRETURN(inkId);
}

/**
 * \ingroup rttoon
 * \ref RtToonGeometryCreateToonGeo creates an RpToonGeo with edge and
 * connectivity information derived from the input RpGeometry, and stores it in
 * geometry.  Creating this information is an expensive memory intensive process
 * which is best done at export time and then streamed for use on consoles.  On non-PC platforms
 * the connectivity information will not be created and attempts to use it will fail.
 *
 * \param geometry  A pointer to the source geometry.
 * \param generateCreaseEdgesFromSmoothingGroups   Toon geometry will determine
 * "crease" edges which are always drawn from the smoothing group boundaries on
 * the source geometry.  This parameter is ignored if the source geometry has no
 * vertex normals defined.
 *
 * \return A pointer to toon geometry if sucessful, or NULL if a memory allocation
 * failed.
 *
 * \see RtToonWorldSectorCreateToonGeo
 *
 */
extern "C" RpToonGeo *
RtToonGeometryCreateToonGeo(RpGeometry *geometry,
                            RwBool generateCreaseEdgesFromSmoothingGroups)
{
    RwInt32 numVerts, numTriangles, i, j;
    RpTriangle *triangles;
    RpToonGeo *toonGeo;
    const RwV3d *vertexNormals, *positions;
    RpMorphTarget *morph;

    RWAPIFUNCTION(RWSTRING("RtToonGeometryCreateToonGeo"));
    RWASSERT(0 < _rpToonGlobals.module.numInstances);

    RWASSERT(geometry != 0);

    toonGeo = *RPTOONGEOMETRYGETDATA(geometry);
    RWASSERT((toonGeo == NULL) && "Geometry already has a toon geometry!");

    numVerts = RpGeometryGetNumVertices(geometry);
    numTriangles = RpGeometryGetNumTriangles(geometry);

    toonGeo = _rpToonGeoInit( numVerts, numTriangles );

    if (!toonGeo)
    {
        RWRETURN(0);
    }

    *RPTOONGEOMETRYGETDATA(geometry) = toonGeo;

    /* copy in geometry triangle data */
    triangles = RpGeometryGetTriangles(geometry);
    RWASSERT(triangles && "geometry has no triangles!");

    for (i=0; i<numTriangles; i++)
    {
        for (j=0; j<3; j++)
        {
            toonGeo->triangles[i].vertIndex[j] = triangles[i].vertIndex[j];
        }
    }

    /*
    Should be okay for skinning/morphing because we only use these for
    figuring out smoothing groups and extrusion normals
    */
    morph = RpGeometryGetMorphTarget(geometry,0);
    RWASSERT(morph != 0);

    positions = RpMorphTargetGetVertices( morph );
    RWASSERT(positions != 0);

    vertexNormals = RpMorphTargetGetVertexNormals( morph );

    _rpToonCreateEdgeInfo(
        toonGeo,
        positions,
        vertexNormals,
        generateCreaseEdgesFromSmoothingGroups);

    toonGeo->parentGeometry = geometry;

    RWRETURN(toonGeo);
}

/**
 * \ingroup rttoon
 * \ref RtToonWorldSectorCreateToonGeo creates an \ref RpToonGeo with edge and
 * connectivity information derived from the input \ref RpWorldSector, and stores it in
 * sector.  Creating this information is an expensive memory intensive process
 * which is best done at export time and then streamed for use on consoles.  On non-PC platforms
 * the connectivity information will not be created and attempts to use it will fail.
 *
 * \param sector  A pointer to the source sector.
 * \param generateCreaseEdgesFromSmoothingGroups   Toon geometry will determine
 * "crease" edges which are always drawn from the smoothing group boundaries on
 * the source sector.  This parameter is ignored if the source sector has no
 * vertex normals defined.
 *
 * \return A pointer to toon geometry if sucessful, or NULL if a memory allocation
 * failed.
 *
 * \see RtToonGeometryCreateToonGeo
 *
 */
extern "C" RpToonGeo *
RtToonWorldSectorCreateToonGeo(RpWorldSector *sector,
                               RwBool generateCreaseEdgesFromSmoothingGroups)
{
    RpToonGeo *toonGeo;
    RwInt32 numVerts, numTriangles, i, j;
    RwV3d *vertexNormals = 0;

    RWAPIFUNCTION(RWSTRING("RtToonWorldSectorCreateToonGeo"));
    RWASSERT(0 < _rpToonGlobals.module.numInstances);

    RWASSERT( sector != NULL );

    toonGeo = *RPTOONSECTORGETDATA(sector);
    RWASSERT((toonGeo == NULL) && "Sector already has a toon geometry!");

    numVerts = RpWorldSectorGetNumVertices(sector);
    numTriangles = RpWorldSectorGetNumTriangles(sector);

    toonGeo = _rpToonGeoInit( numVerts, numTriangles );

    if (!toonGeo)
    {
        RWRETURN(0);
    }

    *RPTOONSECTORGETDATA(sector) = toonGeo;

    /* copy in sector triangle data */
    if (numTriangles)
    {
        RWASSERT(sector->triangles && "world sector doesn't have polys... doh!");

        for (i=0; i<numTriangles; i++)
        {
            for (j=0; j<3; j++)
            {
                toonGeo->triangles[i].vertIndex[j] = sector->triangles[i].vertIndex[j];
            }
        }
    }

    /* unpack vertex normals to find smoothing groups */
    if (sector->normals && generateCreaseEdgesFromSmoothingGroups)
    {
        vertexNormals = (RwV3d *)RwMalloc( sizeof( RwV3d ) * numVerts,
                              rwID_TOONPLUGIN | rwMEMHINTDUR_FUNCTION);
        RWASSERT(vertexNormals && "RwMalloc failed" );

        for (i=0; i<numVerts; i++)
        {
            RPV3DFROMVERTEXNORMAL(vertexNormals[i], sector->normals[i]);
        }
    }

    _rpToonCreateEdgeInfo(
        toonGeo,
        sector->vertices,
        vertexNormals,
        generateCreaseEdgesFromSmoothingGroups);

    if (vertexNormals)
    {
        RwFree(vertexNormals);
    }

    toonGeo->parentWorldSector = sector;

    RWRETURN(toonGeo);
}

typedef struct _rpToonEdgeRemap
{
    rpToonEdgeIndex edgeIndex;
    RwInt32         sortKey;
} _rpToonEdgeRemap;

/*****************************************************************************
 EdgeRemapCompare
 */
static int
EdgeRemapCompare(const void *pA, const void *pB)
{
    const _rpToonEdgeRemap *erA = (const _rpToonEdgeRemap *)pA;

    const _rpToonEdgeRemap *erB = (const _rpToonEdgeRemap *)pB;
    RWFUNCTION(RWSTRING("EdgeRemapCompare"));


    RWRETURN( erA->sortKey - erB->sortKey );
}

typedef std::map<RpMaterial *, RpMaterial *> MaterialClonedFromMaterialMap;

typedef struct SortTriangle SortTriangle;
struct SortTriangle
{
        RwInt32 originalIndex;
        RpMaterial *material;
};

/*****************************************************************************
 SortPolygons
 */
static int
SortPolygons(const void *pA, const void *pB)
{
    /*
     * (copy pasted and modified ever so slightly from world/bameshop.c
     * - won't it be fun to keep these in synch if it ever changes.
     * See also tool/rtvcat.cpp)
     */

    const int transBIT = 16, rastBIT = 8, pipeBIT = 4, texBIT = 2, matBIT = 1;

    const SortTriangle *mtpA = *(const SortTriangle * const *) pA;
    const SortTriangle *mtpB = *(const SortTriangle * const *) pB;

    RpMaterial *materialA = mtpA->material;
    RpMaterial *materialB = mtpB->material;

    RwRaster   *rasterA   = (RwRaster *)NULL;
    RwRaster   *rasterB   = (RwRaster *)NULL;
    RxPipeline *pipelineA = (RxPipeline *)NULL;
    RxPipeline *pipelineB = (RxPipeline *)NULL;
    RwTexture  *textureA  = (RwTexture *)NULL;
    RwTexture  *textureB  = (RwTexture *)NULL;

    /* IMO use UInts for bitfields! Sign bits are nothing but trouble... */
    RwInt32     orderA = 0;
    RwInt32     orderB = 0;

    RWFUNCTION(RWSTRING("SortPolygons"));

    /* Easy case first */
    if (materialA == materialB)
    {
        RWRETURN(0);
    }

    /* We sort on:
     *   transparency > raster > pipeline > texture
     *
     * Transparency is required for correct alpha render ordering.
     * Raster upload is the greatest cost.
     * Pipeline swap might be a significant cost - vector code upload, CPU-side code cache miss
     * Texture state changes might also hurt even with the same raster. (?)
     */
#define RasterHasAlpha(_type)                           \
            ( ((_type) == rwRASTERFORMAT1555) ||        \
              ((_type) == rwRASTERFORMAT4444) ||        \
              ((_type) == rwRASTERFORMAT8888) )

    if (materialA)
    {
        /* Place transparent materials after non transparent ones */
        if (materialA->texture)
        {
            textureA = materialA->texture;
            rasterA = RwTextureGetRaster(textureA);

            if (RasterHasAlpha
                (RwRasterGetFormat(rasterA) & (RwInt32)
                 rwRASTERFORMATPIXELFORMATMASK))
            {
                orderA |= transBIT;
            }
        }

        if (materialA->color.alpha != 0xff)
        {
            orderA |= transBIT;
        }

        pipelineA = materialA->pipeline;
    }

    if (materialB)
    {
        /* Place transparent materials after non transparent ones */
        if (materialB->texture)
        {
            textureB = materialB->texture;
            rasterB = RwTextureGetRaster(textureB);

            if (RasterHasAlpha
                (RwRasterGetFormat(rasterB) & (RwInt32)
                 rwRASTERFORMATPIXELFORMATMASK))
            {
                orderB |= transBIT;
            }
        }

        if (materialB->color.alpha != 0xff)
        {
            orderB |= transBIT;
        }

        pipelineB = materialB->pipeline;
    }

    orderA |= ((RwUInt32) rasterA > (RwUInt32) rasterB) ? rastBIT : 0;
    orderB |= ((RwUInt32) rasterA < (RwUInt32) rasterB) ? rastBIT : 0;

    orderA |= ((RwUInt32) pipelineA > (RwUInt32) pipelineB) ? pipeBIT : 0;
    orderB |= ((RwUInt32) pipelineA < (RwUInt32) pipelineB) ? pipeBIT : 0;

    orderA |= ((RwUInt32) textureA > (RwUInt32) textureB) ? texBIT : 0;
    orderB |= ((RwUInt32) textureA < (RwUInt32) textureB) ? texBIT : 0;

    orderA |= ((RwUInt32) materialA > (RwUInt32) materialB) ? matBIT : 0;
    orderB |= ((RwUInt32) materialA < (RwUInt32) materialB) ? matBIT : 0;

    RWRETURN(orderA - orderB);
}

typedef std::vector<SortTriangle> SortTriangleVec;

/*****************************************************************************
 ToonGeoPostStripifyTriangleFixUp
 */
static void
ToonGeoPostStripifyTriangleFixUp(RpToonGeo *geo,
                                 SortTriangleVec & sortTriangleVec)
{
    /* triangles could have been reordered by adding new materials & re stripping */
    /* need to fix up dependent data without destroying edge info... arghghghghg */
    /* at least we don't have to write a version for MAX and a separate version for MAYA. */

    /* build mapping of old positions to new positions, better match what tristripper did */
    RwInt32 triangleIndex;
    typedef std::vector<SortTriangle *> SortTrianglePtrVec;
    SortTrianglePtrVec sortTrianglePtrVec(sortTriangleVec.size());

    RWFUNCTION(RWSTRING("ToonGeoPostStripifyTriangleFixUp"));


    for (triangleIndex = 0; triangleIndex < (RwInt32)sortTriangleVec.size(); triangleIndex++)
    {
        sortTrianglePtrVec[triangleIndex] = &sortTriangleVec[triangleIndex];
    }
    qsort(&sortTrianglePtrVec[0], sortTrianglePtrVec.size(), sizeof(SortTriangle *), SortPolygons);

    /* face normals, edge wings & triangle data needs fixing */

    /* fix the wings */
    typedef std::vector<rpToonFaceIndex> OldIndexToNewIndexVec;
    OldIndexToNewIndexVec oldIndexToNewIndexVec(geo->numTriangles);

    for (triangleIndex = 0; triangleIndex < geo->numTriangles; triangleIndex++)
    {
        RwInt32 oldIndex = sortTrianglePtrVec[triangleIndex]->originalIndex;

        oldIndexToNewIndexVec[oldIndex] = triangleIndex;
    }

    for (RwInt32 edgeIndex=0; edgeIndex < geo->numEdges; edgeIndex++)
    {
        for (RwInt32 face=0; face<2; face++)
        {
            rpToonFaceIndex oldFaceIndex = geo->edges[edgeIndex].face[face];

            if (oldFaceIndex != RPTOONEDGE_NONEIGHBOURFACE)
            {
                geo->edges[edgeIndex].face[face] = oldIndexToNewIndexVec[ oldFaceIndex ];
            }
        }
    }

    /* fix the triangle data & face normals */
    typedef std::vector<RwV3d> FaceNormalVec;
    FaceNormalVec faceNormalVec(geo->numTriangles);
    std::copy(&geo->faceNormals[0], &geo->faceNormals[geo->numTriangles], faceNormalVec.begin());

    typedef std::vector<rpToonTriangle> ToonTriangleVec;
    ToonTriangleVec toonTriangleVec(geo->numTriangles);
    std::copy(&geo->triangles[0], &geo->triangles[geo->numTriangles], toonTriangleVec.begin());

    for (triangleIndex = 0; triangleIndex < geo->numTriangles; triangleIndex++)
    {
        RwInt32 oldTriangleIndex = sortTrianglePtrVec[triangleIndex]->originalIndex;
        geo->faceNormals[triangleIndex] = faceNormalVec[oldTriangleIndex];
        geo->triangles[triangleIndex] = toonTriangleVec[oldTriangleIndex];
    }

    RWRETURNVOID();
}

#define UNASSIGNED_MATERIAL_SILHOUETTE_INK (255)

typedef std::set<RpMaterial *> MaterialSet;

/*****************************************************************************
 InitMaterialSilhouetteInkIDs
 */
static RpMaterial *
InitMaterialSilhouetteInkIDs(RpMaterial *material, void *data)
{
    RWFUNCTION(RWSTRING("InitMaterialSilhouetteInkIDs"));

    RpToonMaterial *toonMat = *RPTOONMATERIALGETDATA(material);

    /* if no toon material, create default one */
    if (!toonMat)
    {
        toonMat = RpToonMaterialCreate();
        RWASSERT(toonMat);
        *RPTOONMATERIALGETDATA(material) = toonMat;
    }

    toonMat->silhouetteInkID = UNASSIGNED_MATERIAL_SILHOUETTE_INK;

    MaterialSet *materialSet = (MaterialSet *)data;
    materialSet->insert(material);

    RWRETURN( material );
}

/*****************************************************************************
 DetermineDominantSilhouetteInkForTriangle
 */
static rpToonGeoInkIndex
DetermineDominantSilhouetteInkForTriangle(RpToonGeo *toonGeo,
                                          RwInt32 triangleIndex,
                                          RwInt32 *vertexIndexRemap)
{
    /* get the inks for each edge of triangle from toon info */
    rpToonGeoInkIndex sideInk[3];
    RWFUNCTION(RWSTRING("DetermineDominantSilhouetteInkForTriangle"));

    for (RwInt32 side=0; side<3; side++)
    {
        rpToonTriangle *toonTriangle = &toonGeo->triangles[triangleIndex];

        rpToonEdgeIndex edgeIndex =
            RtToonGeoFindEdgeIndex(toonGeo,
                                   vertexIndexRemap[toonTriangle->vertIndex[side]],
                                   vertexIndexRemap[toonTriangle->vertIndex[(side+1)%3]]);

        if (RPTOONEDGE_NONEIGHBOURFACE != edgeIndex)
        {
            sideInk[side] = toonGeo->edgeInkIDs[edgeIndex].inkId[RPTOON_INKTYPE_SILHOUETTE];
        }
        else
        {
            sideInk[side] = 0;
        }
    }

    /* 2 out of 3 ain't bad */
    if ((sideInk[0] == sideInk[1])
        || (sideInk[0] == sideInk[2]))
    {
        RWRETURN( sideInk[0] );
    }
    if (sideInk[1] == sideInk[2])
    {
        RWRETURN( sideInk[1] );
    }

    /* they're all different - pick arbitrarily */
    RWRETURN( sideInk[0] );
}

/*****************************************************************************
 _rpToonRepartitionMeshBySilhouetteInks
 */
extern "C" void
_rpToonRepartitionMeshBySilhouetteInks(RpGeometry *geometry)
{
    RWFUNCTION(RWSTRING("_rpToonRepartitionMeshBySilhouetteInks"));

    MaterialSet materialSet;
    RpGeometryForAllMaterials(geometry, InitMaterialSilhouetteInkIDs, (void *)&materialSet );
    MaterialClonedFromMaterialMap materialClonedFromMaterialMap;

    RpToonGeo *toonGeo = RpToonGeometryGetToonGeo(geometry);

    typedef std::vector<RwInt32> VertexRemapVec;
    VertexRemapVec vertexRemapVec(RpGeometryGetNumVertices(geometry));
    RtToonGeoGetVertexRemap(toonGeo, RpMorphTargetGetVertices(RpGeometryGetMorphTarget(geometry, 0)), &vertexRemapVec[0]);

    RpGeometryLock(geometry, rpGEOMETRYLOCKPOLYGONS);

    /* for each triangle */
    RwInt32 numTriangles = RpGeometryGetNumTriangles(geometry);
    RpTriangle *triangles = RpGeometryGetTriangles(geometry);

    RwInt32 triangleIndex;
    for (triangleIndex = 0; triangleIndex < numTriangles; triangleIndex++)
    {
        /* decide "dominant" silhouette ink on triangle */
        rpToonGeoInkIndex dominantInkIndex = DetermineDominantSilhouetteInkForTriangle(toonGeo, triangleIndex, &vertexRemapVec[0]);

        RpMaterial *trianglesOriginalMaterial = RpGeometryTriangleGetMaterial(geometry, &triangles[triangleIndex]);
        RpToonMaterial *toonMat = *RPTOONMATERIALGETDATA(trianglesOriginalMaterial);

        RWASSERT(toonMat != 0);

        /* if the triangle's material has no ink associated, associate dominant ink */
        if (toonMat->silhouetteInkID == UNASSIGNED_MATERIAL_SILHOUETTE_INK)
        {
            toonMat->silhouetteInkID = dominantInkIndex;
        }
        /* else if the triangle's material's ink doesn't match the dominant one, */
        else if (toonMat->silhouetteInkID != dominantInkIndex)
        {
            /* try and find another material with the matching ink to re-use, */
            RpMaterial *matchingMaterial = NULL;
            for (MaterialSet::iterator matIt = materialSet.begin(); matIt != materialSet.end(); ++matIt)
            {
                RpMaterial *candidateMatchingMaterial = *matIt;
                RpToonMaterial *candidateMatchingToonMat = *RPTOONMATERIALGETDATA(candidateMatchingMaterial);
                RWASSERT(candidateMatchingToonMat);
                if (candidateMatchingToonMat->silhouetteInkID == dominantInkIndex)
                {
                    /* and came from same original material!!! */
                    MaterialClonedFromMaterialMap::iterator cloneIt
                        = materialClonedFromMaterialMap.find(candidateMatchingMaterial);

                    if (    ((cloneIt == materialClonedFromMaterialMap.end()) && /* not a clone */
                             (candidateMatchingMaterial == trianglesOriginalMaterial)) ||
                            (cloneIt->second == trianglesOriginalMaterial))       /* or cloned from same mat */
                    {
                        matchingMaterial = candidateMatchingMaterial;
                        break;
                    }
                }
            }

            /* otherwise we'll have to make a new material. */
            if (!matchingMaterial)
            {
                matchingMaterial = RpMaterialClone( trianglesOriginalMaterial );
                RpToonMaterial *matchingMaterialToonMat = *RPTOONMATERIALGETDATA(matchingMaterial);
                matchingMaterialToonMat->silhouetteInkID = dominantInkIndex;
                materialSet.insert(matchingMaterial);
                materialClonedFromMaterialMap[matchingMaterial] = trianglesOriginalMaterial;
            }

            /* point the triangle at the nice material */
            RpGeometryTriangleSetMaterial(geometry, &triangles[triangleIndex], matchingMaterial );
        }
        /* (else triangle's got a material with matching ink already, nothing to do) */
    }

    /* save the original triangle order so we can resort it and fix dependent data */
    SortTriangleVec sortTriangleVec(toonGeo->numTriangles);
    for (triangleIndex = 0; triangleIndex < toonGeo->numTriangles; triangleIndex++)
    {
        sortTriangleVec[triangleIndex].originalIndex = triangleIndex;
        sortTriangleVec[triangleIndex].material = RpGeometryTriangleGetMaterial(geometry, &triangles[triangleIndex]);
    }

    /* resort the triangles by material & rebuild the tristips */
    RpGeometryUnlock(geometry);

    /* triangles might be reordered - we have to rebuild dependent data now. */
    ToonGeoPostStripifyTriangleFixUp(toonGeo, sortTriangleVec);

    /* save the association of materials + silhouette inks */

    RWRETURNVOID();
}

/**
 * \ingroup rttoongeo
 * \ref RtToonGeoOptimizeEdgeInkIDs is used to convert
 *  an \ref RpGeometry's \ref RpToonGeo to a rendering
 * - optimal format after \ref RtToonGeoSetEdgeInkID is
 * used to tag edges with inks.
 *
 * This function is very expensive, should be called at
 * export time only and therefore is only available on PC platforms
 *
 * \param toonGeo the \ref RpToonGeo
 * \return a pointer to the \ref RpToonGeo on success
 *         or NULL if an error occurred.
 * \see RtToonGeoGetNumEdges
 * \see RtToonGeoGetEdges
 * \see RpToonInkType
 * \see RtToonGeoGetInkIDFromName
 * \see RtToonWorldOptimize
 */
extern "C" RpToonGeo *
RtToonGeoOptimizeEdgeInkIDs(RpToonGeo *toonGeo)
{
    _rpToonEdgeRemap *remap, *invRemap;
    RpToonEdge *newEdges;
    rpToonEdgeInkID *newEdgeInkIDs;
    rpToonEdgeIndex i;
    RwInt32 f, v;

    RWAPIFUNCTION(RWSTRING("RtToonGeoOptimizeEdgeInkIDs"));
    RWASSERT(0 < _rpToonGlobals.module.numInstances);
    RWASSERT(toonGeo);

    /*
      Sort all the non-silhoutte edge ids into a contiguous block for reasonably quick rendering
      later (or at least until we build a separate geometry for them)
    */
    remap = (_rpToonEdgeRemap *) RwMalloc( sizeof(_rpToonEdgeRemap) * toonGeo->numEdges,
                                           rwID_TOONPLUGIN | rwMEMHINTDUR_FUNCTION);
    RWASSERT(remap&&"RwMalloc failed");
    if (!remap)
    {
        RWRETURN(0);
    }

    invRemap = (_rpToonEdgeRemap *) RwMalloc( sizeof(_rpToonEdgeRemap) * toonGeo->numEdges,
                                              rwID_TOONPLUGIN | rwMEMHINTDUR_FUNCTION );
    RWASSERT(invRemap&&"RwMalloc failed");
    if (!invRemap)
    {
        RWRETURN(0);
    }

    /* look up new index, get old index */
    toonGeo->numCreaseEdges = 0;

    for (i=0; i<toonGeo->numEdges; i++)
    {
        remap[i].edgeIndex = i;
        remap[i].sortKey = toonGeo->edgeInkIDs[i].inkId[RPTOON_INKTYPE_CREASE];

#define REALLY_HUGE_SORT_KEY 100000

        RWASSERT(remap[i].sortKey < REALLY_HUGE_SORT_KEY);

        /* Make sure edges that aren't supposed to be drawn go at the back */
        if (remap[i].sortKey == RPTOON_INKTYPE_SILHOUETTE)
        {
            remap[i].sortKey = REALLY_HUGE_SORT_KEY;
        }
        else
        {
            toonGeo->numCreaseEdges++;
        }
    }

    qsort(remap, toonGeo->numEdges, sizeof(_rpToonEdgeRemap), EdgeRemapCompare );

    for (i=0; i<toonGeo->numEdges; i++)
    {
        /* look up old index, get new index */
        invRemap[remap[i].edgeIndex].edgeIndex = i;
    }

    /*
      Now remapify all the edges
    */
    newEdges = (RpToonEdge *)RwMalloc( sizeof(RpToonEdge) * toonGeo->numEdges,
                                       rwID_TOONPLUGIN | rwMEMHINTDUR_EVENT );
    RWASSERT( newEdges && "RwMalloc failed");
    if (!newEdges)
    {
        RWRETURN(0);
    }

    newEdgeInkIDs = (rpToonEdgeInkID *)RwMalloc( sizeof(rpToonEdgeInkID) * toonGeo->numEdges,
                                                 rwID_TOONPLUGIN | rwMEMHINTDUR_EVENT );
    RWASSERT(newEdgeInkIDs && "RwMalloc failed" );
    if (!newEdgeInkIDs)
    {
        RWRETURN(0);
    }

    for (i=0; i<toonGeo->numEdges; i++)
    {
        /* remap the edge data */
        newEdges[i] = toonGeo->edges[remap[i].edgeIndex];

        /* remap the edge wings too */
        for (f=0; f<2; f++)
        {
            for (v=0; v<2; v++)
            {
                rpToonEdgeIndex edge = newEdges[i].edgefv[f][v];

                if (edge != noNextEdge)
                {
                    edge = invRemap[edge].edgeIndex;
                }

                newEdges[i].edgefv[f][v] = edge;
            }
        }

        /* and remap edge IDs */
        newEdgeInkIDs[i] = toonGeo->edgeInkIDs[remap[i].edgeIndex];
    }

    RwFree(toonGeo->edges);
    toonGeo->edges = newEdges;
    RwFree(toonGeo->edgeInkIDs);
    toonGeo->edgeInkIDs = newEdgeInkIDs;
    RwFree(invRemap);
    RwFree(remap);

    /*
      For PS2 we'll need to reorganize meshes into sub meshes where the silhoutte ink changes.
    */
    if (toonGeo->parentGeometry)
    {
        _rpToonRepartitionMeshBySilhouetteInks( toonGeo->parentGeometry );
    }
    /* else worlds have to be done with a special API call because whole world must be done at once. */

    /* edgeInkIDs are optimized until someone messes them up */
    toonGeo->edgeInkIDsOptimized = TRUE;

    RWRETURN(toonGeo);
}

/* gonna use a vector this time because I don't want to disturb the order of existing materials if possible */
typedef std::vector<RpMaterial *> MaterialVec;

#define UNASSIGNED_SECTOR (1<<20)
RwInt32 s_sectorIndex = 0;

/*****************************************************************************
 SectorAndInkID
 */
RwInt32
SectorAndInkID(RwInt32 sectorIndex, RwInt32 inkIndex)
{
    RWFUNCTION(RWSTRING("SectorAndInkID"));

    RWRETURN((sectorIndex << 8) | inkIndex);
}

#if 0
/*****************************************************************************
 GetInkIDFromPackedSectorAndInkID
 */
RwInt32
GetInkIDFromPackedSectorAndInkID(RwInt32 packed)
{
    RWFUNCTION(RWSTRING("GetInkIDFromPackedSectorAndInkID"));

    RWRETURN(packed & 0xff);
}
#endif /* 0 */

/*****************************************************************************
 RepartitionSectorBySilhouetteInks
 */
static RpWorldSector *
RepartitionSectorBySilhouetteInks(RpWorldSector *sector, void *data)
{
    RWFUNCTION(RWSTRING("RepartitionSectorBySilhouetteInks"));

    RWASSERT((sector->matListWindowBase == 0) && "TO DO: fix up triangle material indices to cope with this case, or kill matListWindowBase");

    MaterialClonedFromMaterialMap *materialClonedFromMaterialMap = (MaterialClonedFromMaterialMap *)data;

    RpWorld *world = RpWorldSectorGetWorld(sector);
    s_sectorIndex++;
    RWASSERT(s_sectorIndex < UNASSIGNED_SECTOR);

    /* if its a space filling sector, then nothing to do */
    RwInt32 numTriangles = sector->numTriangles;
    if (numTriangles == 0)
    {
        RWRETURN(sector);
    }

    RpToonGeo *toonGeo = RpToonWorldSectorGetToonGeo(sector);

    typedef std::vector<RwInt32> VertexRemapVec;
    VertexRemapVec vertexRemapVec(RpWorldSectorGetNumVertices(sector));
    RtToonGeoGetVertexRemap(toonGeo, sector->vertices, &vertexRemapVec[0]);

    /* for each triangle */
    RpTriangle *triangles = sector->triangles;
    for (RwInt32 triangleIndex = 0; triangleIndex < numTriangles; triangleIndex++)
    {
        /* decide "dominant" silhouette ink on triangle */
        rpToonGeoInkIndex dominantInkIndex = DetermineDominantSilhouetteInkForTriangle(toonGeo, triangleIndex, &vertexRemapVec[0]);

        /* fetch current material */
        RpMaterialList *matList = &world->matList;
        RwInt32 matIndex = triangles[triangleIndex].matIndex + sector->matListWindowBase;
        RpMaterial *trianglesOriginalMaterial = _rpMaterialListGetMaterial(matList, matIndex);
        RwInt32 thisSectorAndInkID = SectorAndInkID(s_sectorIndex, dominantInkIndex);
        RpToonMaterial *toonMat = *RPTOONMATERIALGETDATA(trianglesOriginalMaterial);
        RWASSERT(toonMat != 0);

#if (0)
        RwInt32 matInkIndex =
            GetInkIDFromPackedSectorAndInkID(toonMat->silhouetteInkID);
        RwInt32 matSectorIndex =
            GetInkIDFromPackedSectorAndInkID(toonMat->silhouetteInkID);
#endif /* (0) */


        /* if the triangle's material has no ink associated, associate dominant ink */
        if (toonMat->silhouetteInkID == SectorAndInkID(UNASSIGNED_SECTOR, UNASSIGNED_MATERIAL_SILHOUETTE_INK))
        {
            toonMat->silhouetteInkID = thisSectorAndInkID;
        }
        /* else if the triangle's material's ink doesn't match the dominant one, */
        else if (toonMat->silhouetteInkID != thisSectorAndInkID)
        {
            /* try and find another material with the matching ink to re-use, */
            RpMaterial *matchingMaterial = NULL;
            RwInt32 matchingMaterialMatListIndex = -1;
            for (RwInt32 matListIndex = 0; matListIndex<matList->numMaterials; matListIndex++)
            {
                RpMaterial *candidateMatchingMaterial = matList->materials[matListIndex];
                RpToonMaterial *candidateMatchingToonMat = *RPTOONMATERIALGETDATA(candidateMatchingMaterial);
                RWASSERT(candidateMatchingToonMat);
                if (candidateMatchingToonMat->silhouetteInkID == thisSectorAndInkID)
                {
                    /* and came from same original material!!! */
                    MaterialClonedFromMaterialMap::iterator cloneIt
                        = materialClonedFromMaterialMap->find(candidateMatchingMaterial);

                    if (    ((cloneIt == materialClonedFromMaterialMap->end()) && /* not a clone */
                             (candidateMatchingMaterial == trianglesOriginalMaterial)) ||
                            (cloneIt->second == trianglesOriginalMaterial))       /* or cloned from same mat */
                    {
                        matchingMaterial = candidateMatchingMaterial;
                        matchingMaterialMatListIndex = matListIndex;
                        break;
                    }
                }
            }

            /* otherwise we'll have to make a new material. */
            if (!matchingMaterial)
            {
                matchingMaterial = RpMaterialClone( trianglesOriginalMaterial );
                RpToonMaterial *matchingMaterialToonMat = *RPTOONMATERIALGETDATA(matchingMaterial);
                RWASSERT(matchingMaterialToonMat);
                matchingMaterialToonMat->silhouetteInkID = thisSectorAndInkID;
                matchingMaterialMatListIndex = _rpMaterialListAppendMaterial(matList, matchingMaterial);
                (*materialClonedFromMaterialMap)[matchingMaterial] = trianglesOriginalMaterial;
            }

            /* point the triangle at the nice material */
            triangles[triangleIndex].matIndex = matchingMaterialMatListIndex;
        }
        /* (else triangle's got a material with matching ink already, nothing to do) */
    }

    RWRETURN(sector);
}

typedef std::vector<SortTriangleVec *> SectorSortTriangleVec;

/*****************************************************************************
 NoteSectorTriangleOrder
 */
static RpWorldSector *
NoteSectorTriangleOrder(RpWorldSector *sector, void *data)
{
    RwInt32 numTriangles = sector->numTriangles;

    RWFUNCTION(RWSTRING("NoteSectorTriangleOrder"));

    if (numTriangles == 0)
    {
        RWRETURN( sector );
    }

    SectorSortTriangleVec *sectorSortTriangleVec = (SectorSortTriangleVec *)data;
    SortTriangleVec *sortTriangleVec = new SortTriangleVec(numTriangles);
    sectorSortTriangleVec->push_back(sortTriangleVec);

    RpWorld *world = RpWorldSectorGetWorld(sector);
    RpMaterialList *matList = &world->matList;

    RpTriangle *triangles = sector->triangles;
    for (RwInt32 triangleIndex = 0; triangleIndex < numTriangles; triangleIndex++)
    {
        (*sortTriangleVec)[triangleIndex].originalIndex = triangleIndex;

        RwInt32 matIndex = triangles[triangleIndex].matIndex + sector->matListWindowBase;
        RpMaterial *material = _rpMaterialListGetMaterial(matList, matIndex);
        (*sortTriangleVec)[triangleIndex].material = material;
    }

    RWRETURN( sector );
}

struct PostStripifySectorFixupData
{
    RwInt32 m_sectorIndex;

    SectorSortTriangleVec & m_sectorSectorSortTriangleData;

    PostStripifySectorFixupData(SectorSortTriangleVec & sectorSectorSortTriangleData)
    : m_sectorIndex(0), m_sectorSectorSortTriangleData(sectorSectorSortTriangleData) {}
};

/*****************************************************************************
 PostStripifySectorFixup
 */
static RpWorldSector *
PostStripifySectorFixup(RpWorldSector *sector, void *data)
{
    RwInt32 numTriangles = sector->numTriangles;

    RWFUNCTION(RWSTRING("PostStripifySectorFixup"));

    if (numTriangles == 0)
    {
        RWRETURN( sector );
    }

    PostStripifySectorFixupData *pssfd = (PostStripifySectorFixupData *)data;
    SortTriangleVec *sortTriangleVec =
        pssfd->m_sectorSectorSortTriangleData[pssfd->m_sectorIndex];

    RpToonGeo *toonGeo = RpToonWorldSectorGetToonGeo(sector);

    ToonGeoPostStripifyTriangleFixUp(toonGeo, *sortTriangleVec );

    delete sortTriangleVec;
    pssfd->m_sectorIndex++;

    RWRETURN( sector );
}

/**
 * \ingroup rttoon
 * \ref RtToonWorldOptimize is used to convert ALL of an \ref RpWorld's
 * \ref RpWorldSector's \ref RpToonGeo s to a rendering
 * - optimal format after \ref RtToonGeoSetEdgeInkID is used to tag edges with inks.
 *
 * This function is very expensive, should be called at export time only and therefore
 * is only available on PC platforms
 *
 * \param world the world to optimize
 *
 * \see RtToonGeoGetNumEdges
 * \see RtToonGeoGetEdges
 * \see RpToonInkType
 * \see RtToonGeoGetInkIDFromName
 * \see RtToonGeoOptimizeEdgeInkIDs
 */
extern "C" void
RtToonWorldOptimize(RpWorld *world)
{
    /* Repartition all meshes by silhouette inks */

    RWAPIFUNCTION(RWSTRING("RtToonWorldOptimize"));

    RpWorldLock(world);

    /* make all toon mats initially unused */
    RwInt32 materialIndex;
    for (materialIndex = 0; materialIndex < world->matList.numMaterials; materialIndex++)
    {
        RpMaterial *materialPtr = world->matList.materials[materialIndex];
        RpToonMaterial *toonMat = *RPTOONMATERIALGETDATA(materialPtr);

        /* if no toon material, create default one */
        if (!toonMat)
        {
            toonMat = RpToonMaterialCreate();
            RWASSERT(toonMat);
            *RPTOONMATERIALGETDATA(materialPtr) = toonMat;
        }

        toonMat->silhouetteInkID = SectorAndInkID(UNASSIGNED_SECTOR, UNASSIGNED_MATERIAL_SILHOUETTE_INK);
    }

    /* add new materials/rearrange triangle material pointers as necessary */
    s_sectorIndex = 0;
    MaterialClonedFromMaterialMap materialClonedFromMaterialMap;
    RpWorldForAllWorldSectors(world, RepartitionSectorBySilhouetteInks, (void *)&materialClonedFromMaterialMap);

    /* remove the sector IDs from the toon materials, ink index is relative to the toon geometry. */
    for (materialIndex = 0; materialIndex < world->matList.numMaterials; materialIndex++)
    {
        RpMaterial *materialPtr = world->matList.materials[materialIndex];
        RpToonMaterial *toonMat = *RPTOONMATERIALGETDATA(materialPtr);
        RWASSERT(toonMat != 0);
        toonMat->silhouetteInkID &= 0xff;
    }

    /* note original orders of triangles in sectors so we can rebuild dependent data */
    SectorSortTriangleVec sectorSortTriangleVec;
    RpWorldForAllWorldSectors(world, NoteSectorTriangleOrder, (void *)&sectorSortTriangleVec);

    /* restripify all the sectors with new materials */
    RpWorldUnlock(world);

    /* fix up all sectors toon geometries */
    PostStripifySectorFixupData pssfd(sectorSortTriangleVec);
    RpWorldForAllWorldSectors(world, PostStripifySectorFixup, (void *)&pssfd );

    RWRETURNVOID();
}
