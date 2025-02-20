/*
 * World utility functions.
 * Helper functions to do extra, non core things with worlds
 *
 * Copyright (c) 1998 Criterion Software Ltd.
 */

/***************************************************************************
 *                                                                         *
 * Module  : baimputl.c                                                    *
 *                                                                         *
 * Purpose : Import utility functions                                      *
 *                                                                         *
 **************************************************************************/

/****************************************************************************
 Includes
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "rwcore.h"
#include "rpdbgerr.h"

#include "rtworld.h"

/* Assumes we have a statically bound library */

/****************************************************************************
 Local Types
 */
typedef struct TriNormal TriNormal;
struct TriNormal
{
    RpTriangle         *t;
    RwV3d               N;
    RwReal              angle[3];
};

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */

#ifdef _WINDOWS
#define __RWCDECL  __cdecl
#endif /* _WINDOWS */

#if (!defined(__RWCDECL))
#define __RWCDECL              /* No op */
#endif /* (!defined(__RWCDECL)) */

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */

static int          __RWCDECL
cmpTriMaterial(const void *a, const void *b)
{
    const RpTriangle   * const sortA = *(const RpTriangle * const *) a;
    const RpTriangle   * const sortB = *(const RpTriangle * const *) b;
    int                 result = sortA->matIndex - sortB->matIndex;

    RWFUNCTION(RWSTRING("cmpTriMaterial"));

    RWRETURN(result);
}

static void
addtonormal(TriNormal * sameTris,
            TriNormal * tri, RwInt32 vi, RwV3d * sn, RwV3d * vertices,
            RwV3d * normals /* , RwTexCoords *texcoords */ )
{

    RWFUNCTION(RWSTRING("addtonormal"));

    /* we need only examine triangles with the same material */
    while (sameTris->t->matIndex == tri->t->matIndex)
    {
        RwInt32             i;

        /* only blend across small crease angles */
        if (RwV3dDotProduct(&sameTris->N, &tri->N) > 0.707f)
        {
            for (i = 0; i < 3; i++)
            {
                RwV3d               delta;

                /* close in XYZ? */
                RwV3dSub(&delta,
                         &vertices[sameTris->t->vertIndex[i]],
                         &vertices[tri->t->vertIndex[vi]]);
                if (RwV3dDotProduct(&delta, &delta) < 0.001f)
                {
                    RwV3dAdd(&normals[sameTris->t->vertIndex[i]],
                             &normals[sameTris->t->vertIndex[i]], sn);
                }
            }
        }

        /* onto next in material run */
        sameTris++;
    }

    RWRETURNVOID();
}

static RpGeometry  *
GeometryCalculateVertexNormals(RpGeometry * geometry)
{
    TriNormal          *sortTris;
    RpTriangle         *triangles, dummyTri;
    RwTexCoords        *texcoords;
    RwInt32             frameNum;
    RwInt32             numFrames;
    RwInt32             numTriangles;
    RwInt32             numVertices;
    RwInt32             vertexNum;
    RwInt32             triangleNum;

    RWFUNCTION(RWSTRING("GeometryCalculateVertexNormals"));

    RWASSERT(NULL != geometry);

    numFrames = RpGeometryGetNumMorphTargets(geometry);
    numTriangles = RpGeometryGetNumTriangles(geometry);
    numVertices = RpGeometryGetNumVertices(geometry);
    triangles = RpGeometryGetTriangles(geometry);
    texcoords = RpGeometryGetVertexTexCoords(geometry, rwTEXTURECOORDINATEINDEX0);

    /* sort by material */
    sortTris =
        (TriNormal *) RwMalloc(sizeof(TriNormal) * (numTriangles + 1),
                               rwID_TLWORLDPLUGIN | rwMEMHINTDUR_FUNCTION);
    for (triangleNum = 0; triangleNum < numTriangles; triangleNum++)
    {
        sortTris[triangleNum].t = &triangles[triangleNum];
    }
    qsort(sortTris, triangleNum, sizeof(TriNormal), cmpTriMaterial);

    /* add a rogue with a different matIndex */
    dummyTri.matIndex = ~0;
    sortTris[numTriangles].t = &dummyTri;

    for (frameNum = 0; frameNum < numFrames; frameNum++)
    {
        TriNormal          *sameTris;
        RpMorphTarget      *morphTarget;
        RwV3d              *vertices;
        RwV3d              *normals;

        morphTarget = RpGeometryGetMorphTarget(geometry, frameNum);
        vertices = RpMorphTargetGetVertices(morphTarget);
        normals = RpMorphTargetGetVertexNormals(morphTarget);

        /* store a normal for each triangle */
        for (triangleNum = 0; triangleNum < numTriangles; triangleNum++)
        {
            RwV3d               v0, v1, v2, edge[3];
            RwReal              length;

            /* Calculate the normal */
            v0 = vertices[sortTris[triangleNum].t->vertIndex[0]];
            v1 = vertices[sortTris[triangleNum].t->vertIndex[1]];
            v2 = vertices[sortTris[triangleNum].t->vertIndex[2]];

            RwV3dSub(&edge[0], &v1, &v0);
            RwV3dSub(&edge[1], &v2, &v1);
            RwV3dSub(&edge[2], &v0, &v2);

            /* Calculate the polygon normal */
            RwV3dCrossProduct(&sortTris[triangleNum].N,
                              &edge[0], &edge[1]);
            length = ( RwV3dDotProduct(&sortTris[triangleNum].N,
                                       &sortTris[triangleNum].N) );
            if (length > ((RwReal)0))
            {
                RwReal  recip;

#if (defined(RW_USE_SPF))
                recip = ((RwReal)1) / (RwReal)sqrtf(length);
#else /* (defined(RW_USE_SPF)) */
                recip = ((RwReal)1) / (RwReal)sqrt((double)length);
#endif /* (defined(RW_USE_SPF)) */

                RwV3dScale(&sortTris[triangleNum].N,
                           &sortTris[triangleNum].N, recip);
            }

            /* and internal angles */
            _rwV3dNormalize(&edge[0], &edge[0]);
            _rwV3dNormalize(&edge[1], &edge[1]);
            _rwV3dNormalize(&edge[2], &edge[2]);

            /* find internal angles */
            length = -RwV3dDotProduct(&edge[0], &edge[2]);
            RwIEEEACosfMacro(sortTris[triangleNum].angle[0], length);
            length = -RwV3dDotProduct(&edge[1], &edge[0]);
            RwIEEEACosfMacro(sortTris[triangleNum].angle[1], length);
            sortTris[triangleNum].angle[2] =
                rwPI - (sortTris[triangleNum].angle[0] +
                        sortTris[triangleNum].angle[1]);

        }

        /* clear the vertex normals for this MorphTarget */
        memset(normals, 0, sizeof(RwV3d) * numVertices);

        /* distribute face normal to vertices near by */
        sameTris = sortTris;
        for (triangleNum = 0; triangleNum < numTriangles; triangleNum++)
        {
            RwV3d               scaledNormal;

            /* track beginning of runs of same material triangles */
            if (sortTris[triangleNum].t->matIndex !=
                sameTris->t->matIndex)
            {
                sameTris = &sortTris[triangleNum];
            }

            /* Add the normal to all nearby vertices */
            RwV3dScale(&scaledNormal, &sortTris[triangleNum].N,
                       sortTris[triangleNum].angle[0]);
            addtonormal(sameTris, &sortTris[triangleNum], 0,
                        &scaledNormal, vertices,
                        normals /* , texcoords */ );

            RwV3dScale(&scaledNormal, &sortTris[triangleNum].N,
                       sortTris[triangleNum].angle[1]);
            addtonormal(sameTris, &sortTris[triangleNum], 1,
                        &scaledNormal, vertices,
                        normals /* , texcoords */ );

            RwV3dScale(&scaledNormal, &sortTris[triangleNum].N,
                       sortTris[triangleNum].angle[2]);
            addtonormal(sameTris, &sortTris[triangleNum], 2,
                        &scaledNormal, vertices,
                        normals /* , texcoords */ );
        }

        /* now normalize the normals */
        for (vertexNum = 0; vertexNum < numVertices; vertexNum++)
        {
            RwV3d              *vertNormal;

            vertNormal = &normals[vertexNum];
            _rwV3dNormalize(vertNormal, vertNormal);
        }
    }

    /* clean up */
    RwFree(sortTris);

    RWRETURN(geometry);

}

/**
 * \ingroup rtworld
 * \ref RtGeometryCalculateVertexNormals
 * is used to calculate a normal vector for each vertex defining the
 * specified geometry. The geometry must have been created with the
 * rpGEOMETRYNORMALS flag so that the data array holding the vertex
 * normals is available.
 *
 * A vertex normal is calculated by averaging the face normals of all
 * connecting polygons that share the vertex, weighted by the angle of
 * each polygon at the vertex. If the vertex is not shared a normal equal
 * to the face normal is used. The resulting vertex normals are of unit
 * length.
 *
 * The geometry is unlocked after the vertex normals have been
 * calculated.
 *
 * The world plugin must be attached before using this function. The
 * include file rtworld.h and the library file rtworld.lib are also
 * required.
 *
 * \param geometry  Pointer to the geometry.
 *
 * \return pointer to the geometry if successful or NULL if there is an
 * error.
 *
 * \see RpGeometryCreate
 * \see RpWorldPluginAttach
 */
RpGeometry         *
RtGeometryCalculateVertexNormals(RpGeometry * geometry)
{
    RWAPIFUNCTION(RWSTRING("RtGeometryCalculateVertexNormals"));
    RWASSERT(geometry);

    /*
     * Treats polygons of same material as belonging to a shading group
     * Face normals are distributed to vertices at the same
     * position iff <45 crease angle
     */

    if (!(rpGEOMETRYNORMALS & RpGeometryGetFlags(geometry)))
    {
        RWRETURN((RpGeometry *)NULL);
    }

    if (geometry)
    {
        RWASSERTISTYPE(geometry, rpGEOMETRY);

        if (RpGeometryLock(geometry, rpGEOMETRYLOCKNORMALS))
        {
            geometry = GeometryCalculateVertexNormals(geometry);

            RpGeometryUnlock(geometry);
        }
    }

    RWRETURN(geometry);
}
