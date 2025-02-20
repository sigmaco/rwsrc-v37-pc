/*
 * Optimization of clumps and geometries for rendering.
 * Maximize the mesh dispatch size by combining materials, eg.
 *
 * Copyright (c) 1998 Criterion Software Ltd.
 */

/****************************************************************************
 Includes
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "rwcore.h"
#include "rpworld.h"
#include "rpdbgerr.h"

#include "rtworld.h"

/****************************************************************************
 Local Types
 */
typedef struct rwVertexNorm rwVertexNorm;
struct rwVertexNorm
{
    RwV3d              *vertex;
    RwV3d              *normal;
};

typedef struct rwVertexInfo rwVertexInfo;
struct rwVertexInfo
{
    RwInt32             index;
    rwVertexNorm       *vertNormList;
    RwRGBA             *prelight;
    RwTexCoords        *texCoord;
    rwVertexInfo       *sameAs;
};

typedef struct rwUniqueVertices rwUniqueVertices;
struct rwUniqueVertices
{
    RwInt32             numVertices;
    RwInt32             numFrames;
    rwVertexNorm       *vertNormList;
    RwRGBA             *prelights;
    RwTexCoords        *texCoords;
};

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */

#if (!defined(__RWCDECL))
#if (defined(_WINDOWS))
#define __RWCDECL __cdecl
#else /* (defined(_WINDOWS)) */
#define __RWCDECL              /* No op */
#endif /* (defined(_WINDOWS)) */
#endif /* (!defined(__RWCDECL)) */

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */
static RwPlaneType  GPlane;

/****************************************************************************
 Functions
 */

/****************************************************************************
 OptimizeGeometryAreMaterialsSame

 On entry   : Material A, material B
 On exit    : TRUE if materials are same and can be combined
 */

static              RwBool
OptimizeGeometryAreMaterialsSame(RpMaterial * matA, RpMaterial * matB,
                                 RwUInt32 flags)
{
    RWFUNCTION(RWSTRING("OptimizeGeometryAreMaterialsSame"));
    RWASSERT(matA);
    RWASSERT(matB);

    /* If textures don't match, reject it */
    if (matA->texture != matB->texture)
    {
        RWRETURN(FALSE);
    }

#if (0)
    if (!matA->texture && !matB->texture)
    {
        /* Color must match if not textured */

        if ((matA->color.red != matB->color.red) ||
            (matA->color.green != matB->color.green) ||
            (matA->color.blue != matB->color.blue))
        {
            RWRETURN(FALSE);
        }
    }
#endif /* (0) */

    if (flags & rpGEOMETRYMODULATEMATERIALCOLOR)
    {
        /* color must match if modulated by it */
        if ((matA->color.red != matB->color.red) ||
            (matA->color.green != matB->color.green) ||
            (matA->color.blue != matB->color.blue) ||
            (matA->color.alpha != matB->color.alpha))
        {
            RWRETURN(FALSE);
        }
    }

#if (0)
    /* If opacity doesn't match, throw it out */

    if (matA->color.alpha != matB->color.alpha)
    {
        RWRETURN(FALSE);
    }
#endif /* (0) */

    if ((matA->surfaceProps.ambient != matB->surfaceProps.ambient) ||
        (matA->surfaceProps.diffuse != matB->surfaceProps.diffuse) ||
        (matA->surfaceProps.specular != matB->surfaceProps.specular))
    {
        RWRETURN(FALSE);
    }

    /* Oh well, it must be true then */
    RWRETURN(TRUE);
}

/****************************************************************************
 OptimizeGeometryReplaceMaterial

 On entry   : geometry, material to replace, material to replace with
 On exit    : Geometry pointer on success
 */
static RpGeometry  *
OptimizeGeometryReplaceMaterial(RpGeometry * geometry,
                                RwInt32 replaceThis, RwInt32 withThis)
{
    RwInt32             numTriangles;
    RpTriangle         *triangle;
    RpMaterial         *materialOld, *materialNew;

    RWFUNCTION(RWSTRING("OptimizeGeometryReplaceMaterial"));
    RWASSERT(geometry);
    RWASSERTISTYPE(geometry, rpGEOMETRY);

    numTriangles = RpGeometryGetNumTriangles(geometry);
    triangle = RpGeometryGetTriangles(geometry);
    materialOld = rpMaterialListGetMaterial(&geometry->matList, replaceThis);
    materialNew = rpMaterialListGetMaterial(&geometry->matList, withThis);

    while (numTriangles--)
    {
        if (triangle->matIndex == replaceThis)
        {
            RpMaterialDestroy(materialOld);
            RpMaterialAddRef(materialNew);
            triangle->matIndex = (RwInt16) withThis;
        }

        triangle++;
    }

    /* Once this is done, there will be materials in the material list with a
     * reference count of one - they will live on for the duration of the
     * geometry's life
     */
    RWRETURN(geometry);
}

/**
 * \ingroup rtworld
 * \ref RtAtomicOptimize is used to optimize the specified atomic for
 * rendering by eliminating duplicated materials in the atomic’s geometry. It
 * also searches the atomic’s geometry for vertices that are within the given
 * distance of each other and replaces them with a single vertex. Their normals
 * must also be within the given tolerance for the replacement to occur. This
 * function may be used to eliminate vertices from the geometry that are
 * considered coincident within the specified tolerance.
 *
 * Note that only vertices that have the same pre-light color and texture
 * coordinates can be replaced with a single vertex. Also, if there is more
 * than one morph target within the geometry, the position and normal of a
 * specific vertex must be within the specified tolerance over all morph
 * targets for this optimization to be performed.
 *
 * The world plugin must be attached before using this function. The include
 * file rtworld.h and the library file rtworld.lib are also required.
 *
 * \param atomic  Pointer to the atomic.
 * \param dist  A RwReal value equal to the tolerance.
 *
 * \return Returns a pointer to the atomic if successful or
 * NULL if there is an error.
 *
 * \see RtClumpOptimize
 * \see RpWorldPluginAttach
 *
 */
RpAtomic           *
RtAtomicOptimize(RpAtomic * atomic, RwReal __RWUNUSED__ dist)
{
    RWAPIFUNCTION(RWSTRING("RtAtomicOptimize"));
    RWASSERT(atomic);
    RWASSERTISTYPE(atomic, rpATOMIC);

    if (atomic)
    {
        RpGeometry         *geometry;

        geometry = RpAtomicGetGeometry(atomic);
        if (geometry)
        {
            RWASSERTISTYPE(geometry, rpGEOMETRY);
            (void)RtGeometryOptimize(geometry, dist);
        }

        RWRETURN(atomic);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN((RpAtomic *) NULL);
}

/****************************************************************************
 OptimizeOptimizeAtomic

 On entry   : Atomic
            : User data pointer (not used)
 On exit    : Atomic on success
 */

static RpAtomic    *
OptimizeOptimizeAtomic(RpAtomic * atomic, void *pData)
{
    RwReal              dist;

    RWFUNCTION(RWSTRING("OptimizeOptimizeAtomic"));
    RWASSERT(atomic);
    RWASSERT(pData);
    RWASSERTISTYPE(atomic, rpATOMIC);

    dist = *(RwReal *) pData;
    RWRETURN(RtAtomicOptimize(atomic, dist));
}

/**
 * \ingroup rtworld
 * \ref RtClumpOptimize is used to optimize the specified clump for
 * rendering by eliminating duplicated materials in geometry referenced by
 * each atomic constituting the clump. It also searches the geometry for
 * vertices that are within the given distance of each other and replaces
 * them with a single vertex. Their normals must also be within the given
 * tolerance for the replacement to occur. This function may be used to
 * eliminate vertices from the geometry that are considered coincident within
 * the specified tolerance.
 *
 * Note that only vertices that have the same pre-light color and texture
 * coordinates can be replaced with a single vertex. Also, if there is more
 * than one morph target within the geometry, the position and normal of a
 * specific vertex must be within the specified tolerance over all morph
 * targets for this optimization to be performed.
 *
 * The world plugin must be attached before using this function. The include
 * file rtworld.h and the library file rtworld.lib are also required.
 *
 * \param clump  Pointer to the clump.
 * \param dist  A RwReal value equal to the tolerance.
 *
 * \return Returns pointer to the clump if successful or
 * NULL if there is an error.
 *
 * \see RtAtomicOptimize
 * \see RpWorldPluginAttach
 *
 */
RpClump            *
RtClumpOptimize(RpClump * clump, RwReal dist)
{
    RWAPIFUNCTION(RWSTRING("RtClumpOptimize"));
    RWASSERT(clump);
    RWASSERTISTYPE(clump, rpCLUMP);

    if (clump)
    {
        RpClump            *clumpRet;

        clumpRet = RpClumpForAllAtomics(clump, OptimizeOptimizeAtomic, &dist);
        RWRETURN(clumpRet);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN((RpClump *) NULL);
}

/****************************************************************************
 OptimizeVertexInfoCreate

 On entry   : geometry
 On exit    : rwVertexInfo
 */

static rwVertexInfo *
OptimizeVertexInfoCreate(RpGeometry * geometry)
{
    RWFUNCTION(RWSTRING("OptimizeVertexInfoCreate"));

    if (geometry)
    {
        rwVertexInfo       *vi, *vertInfo;
        rwVertexNorm       *vertNormList;
        RwInt32             numMorphTargets;
        RwInt32             frameNum;
        RwInt32             vertNum, numVertices;
        RwRGBA             *prelights;
        RwTexCoords        *texCoords;

        numVertices = RpGeometryGetNumVertices(geometry);
        vi = (rwVertexInfo *)RwMalloc(sizeof(rwVertexInfo) * numVertices,
            rwID_TLWORLDPLUGIN | rwMEMHINTDUR_EVENT);
        if (!vi)
        {
            RWRETURN((rwVertexInfo *) NULL);
        }

        numMorphTargets = RpGeometryGetNumMorphTargets(geometry);
        vertNormList = (rwVertexNorm *) RwMalloc(sizeof(rwVertexNorm) *  numVertices * numMorphTargets,
                                                 rwID_TLWORLDPLUGIN | rwMEMHINTDUR_EVENT);
        if (!vertNormList)
        {
            RwFree(vi);
            RWRETURN((rwVertexInfo *) NULL);
        }

        /* set up the vertNorm list pointers */
        vertInfo = vi;
        for (vertNum = 0; vertNum < numVertices; vertNum++)
        {
            vertInfo[vertNum].vertNormList = vertNormList;
            vertNormList += numMorphTargets;
        }

        /* get the vertex and normals from each key frame */
        for (frameNum = 0; frameNum < numMorphTargets; frameNum++)
        {
            RwInt32             vertNum;
            RpMorphTarget      *morphTarget;
            RwV3d              *vertices;
            RwV3d              *normals;

            morphTarget = RpGeometryGetMorphTarget(geometry, frameNum);
            vertices = RpMorphTargetGetVertices(morphTarget);
            normals = RpMorphTargetGetVertexNormals(morphTarget);

            for (vertNum = 0; vertNum < numVertices; vertNum++)
            {
                vertInfo[vertNum].vertNormList[frameNum].vertex = &vertices[vertNum];
                if (normals)
                {
                    vertInfo[vertNum].vertNormList[frameNum].normal = &normals[vertNum];
                }
                else
                {
                    vertInfo[vertNum].vertNormList[frameNum].normal = (RwV3d *) NULL;
                }
            }
        }

        /* get the colour, prelight, & texture information */
        prelights = RpGeometryGetPreLightColors(geometry);
        texCoords = RpGeometryGetVertexTexCoords(geometry, rwTEXTURECOORDINATEINDEX0);
        vertInfo = vi;
        for (vertNum = 0; vertNum < numVertices; vertNum++)
        {
            vertInfo->sameAs = (rwVertexInfo *) NULL;

            vertInfo->index = -1;

            /* prelights */
            if (prelights)
            {
                vertInfo->prelight = &prelights[vertNum];
            }
            else
            {
                vertInfo->prelight = (RwRGBA *) NULL;
            }

            /* texCoords */
            if (texCoords)
            {
                vertInfo->texCoord = &texCoords[vertNum];
            }
            else
            {
                vertInfo->texCoord = (RwTexCoords *) NULL;
            }

            vertInfo++;
        }

        RWRETURN(vi);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN((rwVertexInfo *) NULL);
}

/****************************************************************************
 OptimizeVertexInfoDestroy

 On entry   : rwVertexInfo
 On exit    : TRUE on success
 */

static              RwBool
OptimizeVertexInfoDestroy(rwVertexInfo * vi)
{
    RWFUNCTION(RWSTRING("OptimizeVertexInfoDestroy"));
    RWASSERT(vi);

    if (vi)
    {
        if (vi->vertNormList)
        {
            RwFree(vi->vertNormList);
            vi->vertNormList = (rwVertexNorm *) NULL;
        }

        RwFree(vi);
        RWRETURN(TRUE);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN(FALSE);
}

/****************************************************************************
 _rtVertexInfoSort

 On entry   : rwVertexInfo A, rwVertexInfo B
 On exit    : -1 less,0 same +1 larger

 */

static int          __RWCDECL
_rtVertexInfoSort(const void *pA, const void *pB)
{
    RwV3d              *vertA, *vertB;
    RwReal              a, b;
    const rwVertexInfo *vpA = (*(const rwVertexInfo * const *) pA);
    const rwVertexInfo *vpB = (*(const rwVertexInfo * const *) pB);

    RWFUNCTION(RWSTRING("_rtVertexInfoSort"));
    RWASSERT(vpA);
    RWASSERT(vpB);

    vertA = vpA->vertNormList->vertex;
    vertB = vpB->vertNormList->vertex;

    a = GETCOORD(*vertA, GPlane);
    b = GETCOORD(*vertB, GPlane);

    if (a == b)
    {
        RWRETURN(0);
    }
    else if (a > b)
    {
        RWRETURN(1);
    }
    else
    {
        RWRETURN(-1);
    }
}

/****************************************************************************
 OptimizeVertexInfoCompare

 On entry   : rwVertexInfo A, rwVertexInfo B, num key frames,
              distance in x, y, & z between vertices and normals to merge
 On exit    : TRUE if equal
 */

static              RwBool
OptimizeVertexInfoCompare(rwVertexInfo * viA, rwVertexInfo * viB,
                          RwInt32 numMorphTargets, RwReal dist)
{
    RwInt32             frameNum;

    RWFUNCTION(RWSTRING("OptimizeVertexInfoCompare"));
    RWASSERT(viA);
    RWASSERT(viB);

    /* compare the vertices in each of the remaining morph targets */
    if (numMorphTargets > 1)
    {
        RwV3d              *vertA, *vertB;

        for (frameNum = 1; frameNum < numMorphTargets; frameNum++)
        {
            vertA = viA->vertNormList[frameNum].vertex;
            vertB = viB->vertNormList[frameNum].vertex;

            if ((RwRealAbs((vertA->x) - (vertB->x)) > dist) ||
                (RwRealAbs((vertA->y) - (vertB->y)) > dist) ||
                (RwRealAbs((vertA->z) - (vertB->z)) > dist))
            {
                RWRETURN(FALSE);
            }
        }
    }

    /* compare normals */
    for (frameNum = 0; frameNum < numMorphTargets; frameNum++)
    {
        RwV3d              *normA, *normB;

        normA = viA->vertNormList[frameNum].normal;
        normB = viB->vertNormList[frameNum].normal;

        if (normA && normB)
        {
            if ((RwRealAbs((normA->x) - (normB->x)) > dist) ||
                (RwRealAbs((normA->y) - (normB->y)) > dist) ||
                (RwRealAbs((normA->z) - (normB->z)) > dist))
            {
                RWRETURN(FALSE);
            }
        }
    }

    /* compare prelight */
    if ((viA->prelight) && (viB->prelight))
    {
        if ((viA->prelight->red != viB->prelight->red) ||
            (viA->prelight->green != viB->prelight->green) ||
            (viA->prelight->blue != viB->prelight->blue) ||
            (viA->prelight->alpha != viB->prelight->alpha))
        {
            RWRETURN(FALSE);
        }
    }

    /* compare texture coordinates */
    if ((viA->texCoord) && (viB->texCoord))
    {
        if ((viA->texCoord->u != viB->texCoord->u) ||
            (viA->texCoord->v != viB->texCoord->v))
        {
            RWRETURN(FALSE);
        }
    }

    RWRETURN(TRUE);
}

/****************************************************************************
 OptimizeVertexInfoChain

 On entry   : rwVertexInfo A, rwVertexInfo B
 On exit    :
 */

static void
OptimizeVertexInfoChain(rwVertexInfo * viA, rwVertexInfo * viB)
{
    RWFUNCTION(RWSTRING("OptimizeVertexInfoChain"));
    RWASSERT(viA);
    RWASSERT(viB);

    while (viA->sameAs)
    {
        viA = viA->sameAs;
    }

    while (viB->sameAs)
    {
        viB = viB->sameAs;
    }

    if (viA != viB)
    {
        if (viA < viB)
        {
            viB->sameAs = viA;
        }
        else
        {
            viA->sameAs = viB;
        }
    }

    RWRETURNVOID();
}

/****************************************************************************
 OptimizeVertexInfoSortArray

 On entry   : rwVertexInfo A, number of vertices, number of faces,
              distance in x, y, & z between vertices and normals to merge
 On exit    : TRUE on success
 */

static              RwBool
OptimizeVertexInfoSortArray(rwVertexInfo * vi, RwInt32 numVertices,
                            RwInt32 numFrames, RwReal dist)
{
    RWFUNCTION(RWSTRING("OptimizeVertexInfoSortArray"));
    RWASSERT(vi);

    if (vi)
    {
        rwVertexInfo      **vertInfoSort;
        RwInt32             i;
        RwInt32             xCur, xPrev;

        /* used to sort the vertInfo array */
        vertInfoSort = (rwVertexInfo **)RwMalloc(sizeof(rwVertexInfo *) * numVertices,
                                            rwID_TLWORLDPLUGIN | rwMEMHINTDUR_FUNCTION);
        if (!vertInfoSort)
        {
            RWRETURN(FALSE);
        }

        /* setup the initail values */
        for (i = 0; i < numVertices; i++)
        {
            vertInfoSort[i] = &vi[i];
        }

        /* we now have all of the vertices -> lets sort them */
        GPlane = rwXPLANE;
        qsort((void *) vertInfoSort, numVertices,
              sizeof(rwVertexInfo *), _rtVertexInfoSort);

        xCur = xPrev = 0;
        do
        {
            RwInt32             yCur, yPrev;

            while ((xCur < numVertices) &&
                   (vertInfoSort[xCur]->vertNormList->vertex->x <=
                    vertInfoSort[xPrev]->vertNormList->vertex->x +
                    dist))
            {
                xCur++;
            }

            GPlane = rwYPLANE;
            qsort(&vertInfoSort[xPrev], xCur - xPrev,
                  sizeof(rwVertexInfo *), _rtVertexInfoSort);

            yCur = xPrev;
            yPrev = xPrev;

            do
            {
                RwInt32             zCur;
                RwInt32             zPrev;

                while ((yCur < xCur) &&
                       (vertInfoSort[yCur]->vertNormList->vertex->y <=
                        vertInfoSort[yPrev]->vertNormList->vertex->y +
                        dist))
                {
                    yCur++;
                }

                GPlane = rwZPLANE;
                qsort(&vertInfoSort[yPrev], yCur - yPrev,
                      sizeof(rwVertexInfo *), _rtVertexInfoSort);

                zCur = yPrev;
                zPrev = yPrev;

                do
                {
                    RwInt32             indA, indB;
                    rwVertexInfo       *viA, *viB;

                    while ((zCur < yCur) &&
                           (vertInfoSort[zCur]->vertNormList->vertex->
                            z <=
                            vertInfoSort[zPrev]->vertNormList->vertex->
                            z + dist))
                    {
                        zCur++;
                    }

                    /* Between nZPrev and nZCur-1 we have all of the vertices
                     * at the same position
                     * Compare each vertex in this interval with every other vertex
                     * The interval will normally be small, so this
                     * isn't as ineffiecient as it at fisrt seems
                     */
                    for (indA = zPrev; indA < zCur - 1; indA++)
                    {
                        viA = vertInfoSort[indA];

                        for (indB = indA; indB < zCur; indB++)
                        {
                            viB = vertInfoSort[indB];

                            if (OptimizeVertexInfoCompare
                                (viA, viB, numFrames, dist))
                            {
                                OptimizeVertexInfoChain(viA, viB);
                            }
                        }
                    }

                    /* Onto the next */
                    zPrev = zCur;
                }
                while (zCur < yCur);

                yPrev = yCur;
            }
            while (yCur < xCur);

            xPrev = xCur;
        }
        while (xCur < numVertices);

        /* No longer need the mapping array */
        RwFree(vertInfoSort);

        RWRETURN(TRUE);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN(FALSE);
}

/****************************************************************************
 OptimizeVertexInfoCalcUniqueVertices

 On entry   : rwVertexInfo A, number of vertices
 On exit    : number of unique vertices
 */

static              RwInt32
OptimizeVertexInfoCalcUniqueVertices(rwVertexInfo * vi,
                                     RwInt32 numVertices)
{
    RWFUNCTION(RWSTRING("OptimizeVertexInfoCalcUniqueVertices"));
    RWASSERT(vi);

    if (vi)
    {
        RwInt32             i, numUniqueVertices;

        /* find out how many new vertices we have */
        numUniqueVertices = 0;
        for (i = 0; i < numVertices; i++)
        {
            if (vi->sameAs == NULL)
            {
                vi->index = numUniqueVertices;
                numUniqueVertices++;
            }

            vi++;
        }

        RWRETURN(numUniqueVertices);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN(FALSE);
}

/****************************************************************************
 OptimizeUniqueVerticesDestroy

 On entry   : rwVertexInfo A
 On exit    : TRUE on success
 */

static              RwBool
OptimizeUniqueVerticesDestroy(rwUniqueVertices * uv)
{
    RWFUNCTION(RWSTRING("OptimizeUniqueVerticesDestroy"));
    RWASSERT(uv);

    if (uv)
    {
        RwInt32             frameNum;

        if (uv->vertNormList)
        {
            for (frameNum = 0; frameNum < uv->numFrames; frameNum++)
            {
                if (uv->vertNormList->vertex)
                {
                    RwFree(uv->vertNormList->vertex);
                    uv->vertNormList->vertex = (RwV3d *) NULL;
                }
            }

            RwFree(uv->vertNormList);
            uv->vertNormList = (rwVertexNorm *) NULL;
        }

        if (uv->prelights)
        {
            RwFree(uv->prelights);
            uv->prelights = (RwRGBA *) NULL;
        }

        if (uv->texCoords)
        {
            RwFree(uv->texCoords);
            uv->texCoords = (RwTexCoords *) NULL;
        }

        RwFree(uv);

        RWRETURN(TRUE);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN(FALSE);
}

/****************************************************************************
 OptimizeUniqueVerticesCreate

 On entry   : rwVertexInfo A, number of vertices, number of morph targets
 On exit    : rwUniqueVertices
 */

static rwUniqueVertices *
OptimizeUniqueVerticesCreate(rwVertexInfo * vi, RwInt32 numVertices,
                             RwInt32 numFrames)
{
    RWFUNCTION(RWSTRING("OptimizeUniqueVerticesCreate"));
    RWASSERT(vi);

    if (vi)
    {
        RwInt32             frameNum;
        RwInt32             numUniqueVertices;
        rwUniqueVertices   *uv;

        /* rwUniqueVertices structure */
        uv = (rwUniqueVertices *)RwMalloc(sizeof(rwUniqueVertices),
                       rwID_TLWORLDPLUGIN | rwMEMHINTDUR_EVENT);
        if (!uv)
        {
            RWRETURN((rwUniqueVertices *) NULL);
        }

        /* verNormList */
        uv->vertNormList = (rwVertexNorm *) RwMalloc(sizeof(rwVertexNorm) * numFrames,
                   rwID_TLWORLDPLUGIN | rwMEMHINTDUR_EVENT);
        if (!uv->vertNormList)
        {
            OptimizeUniqueVerticesDestroy(uv);

            RWRETURN((rwUniqueVertices *) NULL);
        }

        numUniqueVertices = OptimizeVertexInfoCalcUniqueVertices(vi, numVertices);

        /* vertices & normals for each morph target */
        for (frameNum = 0; frameNum < numFrames; frameNum++)
        {
            RwV3d              *vertices;

            /* add the vertices */
            vertices = (RwV3d *)RwMalloc(sizeof(RwV3d) * numUniqueVertices,
                rwID_TLWORLDPLUGIN | rwMEMHINTDUR_EVENT | rwMEMHINTFLAG_RESIZABLE);
            if (!vertices)
            {
                OptimizeUniqueVerticesDestroy(uv);

                RWRETURN((rwUniqueVertices *) NULL);
            }

            /* add optional normals */
            if (vi->vertNormList[frameNum].normal)
            {
                RwV3d              *normals;

                vertices = (RwV3d *) RwRealloc(vertices, sizeof(RwV3d) * (numUniqueVertices << 1),
                               rwID_TLWORLDPLUGIN | rwMEMHINTDUR_EVENT | rwMEMHINTFLAG_RESIZABLE);
                if (!vertices)
                {
                    OptimizeUniqueVerticesDestroy(uv);

                    RWRETURN((rwUniqueVertices *) NULL);
                }
                normals = vertices + numUniqueVertices;
                uv->vertNormList[frameNum].normal = normals;
            }

            uv->vertNormList[frameNum].vertex = vertices;
        }

        uv->numVertices = numUniqueVertices;
        uv->numFrames = numFrames;

        /* prelights */
        if (vi->prelight)
        {
            uv->prelights = ((RwRGBA *)RwMalloc(sizeof(RwRGBA) * numUniqueVertices,
                                        rwID_TLWORLDPLUGIN | rwMEMHINTDUR_EVENT));
            if (!uv->prelights)
            {
                OptimizeUniqueVerticesDestroy(uv);

                RWRETURN((rwUniqueVertices *) NULL);
            }
        }
        else
        {
            uv->prelights = (RwRGBA *) NULL;
        }

        /* texture coords */
        if (vi->texCoord)
        {
            uv->texCoords = (RwTexCoords *)RwMalloc(sizeof(RwTexCoords) * numUniqueVertices,
                                                    rwID_TLWORLDPLUGIN | rwMEMHINTDUR_EVENT);
            if (!uv->texCoords)
            {
                OptimizeUniqueVerticesDestroy(uv);
                RWRETURN((rwUniqueVertices *) NULL);
            }
        }
        else
        {
            uv->texCoords = (RwTexCoords *) NULL;
        }

        /* now initialize the values */
        {
            RwInt32             i, currentVert;
            rwVertexInfo       *vertInfo;

            /* set up the vertices & normals */
            vertInfo = vi;
            for (frameNum = 0; frameNum < numFrames; frameNum++)
            {
                rwVertexNorm       *uniqueVertNorms;

                vertInfo = vi;
                currentVert = 0;
                uniqueVertNorms = &uv->vertNormList[frameNum];
                for (i = 0; i < numVertices; i++)
                {
                    /* unique so create a new vertex */
                    if (vertInfo->sameAs == NULL)
                    {
                        if (vertInfo->vertNormList[frameNum].vertex)
                        {
                            uniqueVertNorms->vertex[currentVert] =
                                *vertInfo->vertNormList[frameNum].vertex;
                        }

                        if (vertInfo->vertNormList[frameNum].normal)
                        {
                            uniqueVertNorms->normal[currentVert] =
                                *vertInfo->vertNormList[frameNum].normal;
                        }

                        currentVert++;
                    }

                    vertInfo++;
                }
            }

            /* setup any prelights, and texture coordinates */
            vertInfo = vi;
            currentVert = 0;
            for (i = 0; i < numVertices; i++)
            {
                /* unique so create a new vertex */
                if (vertInfo->sameAs == NULL)
                {
                    if (uv->prelights)
                    {
                        uv->prelights[currentVert] = *vertInfo->prelight;
                    }

                    if (uv->texCoords)
                    {
                        uv->texCoords[currentVert] = *vertInfo->texCoord;
                    }

                    currentVert++;
                }

                vertInfo++;
            }
        }

        RWRETURN(uv);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN((rwUniqueVertices *) NULL);
}

/****************************************************************************
 OptimizeGeometrySetUniqueVertices

 On entry   : geometry, rwUniqueVertices, numUniqueVertices
 On exit    : TRUE on success
 */

static              RwBool
OptimizeGeometrySetUniqueVertices(RpGeometry * geometry,
                                  rwUniqueVertices * uv,
                                  RwInt32 numUniqueVertices)
{
    RWFUNCTION(RWSTRING("OptimizeGeometrySetUniqueVertices"));
    RWASSERT(geometry);
    RWASSERT(uv);
    if (geometry && uv)
    {
        if (RpGeometryLock
            (geometry,
             rpGEOMETRYLOCKVERTICES |
             rpGEOMETRYLOCKNORMALS | rpGEOMETRYLOCKTEXCOORDS))
        {
            RwInt32             numFrames, frameNum;

            geometry->numVertices = numUniqueVertices;
            /* set the vertices & normals for each frame */
            numFrames = RpGeometryGetNumMorphTargets(geometry);
            for (frameNum = 0; frameNum < numFrames; frameNum++)
            {
                RpMorphTarget      *morphTarget;

                morphTarget = RpGeometryGetMorphTarget(geometry, frameNum);

                memcpy(morphTarget->verts, uv->vertNormList[frameNum].vertex,
                    sizeof(RwV3d)*numUniqueVertices);
                if (morphTarget->normals)
                {
                    memcpy(morphTarget->normals, uv->vertNormList[frameNum].normal,
                        sizeof(RwV3d)*numUniqueVertices);
                }
            }

            /* set any prelights, texture coordinates */
            if (uv->prelights)
            {
                memcpy(geometry->preLitLum, uv->prelights, sizeof(RwRGBA)*numUniqueVertices);
            }

            if (uv->texCoords)
            {
                memcpy(geometry->texCoords[0], uv->texCoords, sizeof(RwTexCoords)*numUniqueVertices);
            }

            RpGeometryUnlock(geometry);
            RWRETURN(TRUE);
        }

        /* couldn't lock the geometry */
        RWRETURN(FALSE);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN(FALSE);
}

/****************************************************************************
 OptimizeGeometryReMappFaces

 On entry   : geometry, rwUniqueVertices
 On exit    : RpGeometry on success
 */

static RpGeometry  *
OptimizeGeometryReMapFaces(RpGeometry * geometry, rwVertexInfo * vi)
{
    RWFUNCTION(RWSTRING("OptimizeGeometryReMapFaces"));
    RWASSERT(geometry);
    RWASSERT(vi);
    if (geometry && vi)
    {
        if (RpGeometryLock(geometry, rpGEOMETRYLOCKPOLYGONS))
        {
            RwInt32             i, numTriangles;
            RwInt32             numNewTriangles = 0;
            RpTriangle         *triangles, *src, *dst;

            numTriangles = RpGeometryGetNumTriangles(geometry);
            triangles = RpGeometryGetTriangles(geometry);
            src = dst = triangles;
            for (i = 0; i < numTriangles; i++)
            {
                RwInt32             j;
                RwUInt16            vert[3];

                RpGeometryTriangleGetVertexIndices(geometry, src, &vert[0], &vert[1], &vert[2]);
                for (j = 0; j < 3; j++)
                {
                    rwVertexInfo       *vertInfo;

                    vertInfo = &vi[vert[j]];
                    while (vertInfo->sameAs)
                    {
                        vertInfo = vertInfo->sameAs;
                    }

                    vert[j] = (RwUInt16) vertInfo->index;
                }

                if ((vert[0] != vert[1]) &&
                    (vert[1] != vert[2]) && (vert[0] != vert[2]))
                {
                    RpGeometryTriangleSetVertexIndices(geometry, dst, vert[0], vert[1], vert[2]);
                    numNewTriangles++;
                    dst++;
                }
                /* tis the triangle of Satan, renounce it now */
                src++;
            }

            /* reset the number of triangles */
            geometry->numTriangles = numNewTriangles;

            RpGeometryUnlock(geometry);
            RWRETURN(geometry);
        }

        /* couldn't lock the geometry */
        RWRETURN(FALSE);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN((RpGeometry *) NULL);
}

/****************************************************************************
 OptimizeGeometryOptimize

 On entry   : geometry, distance in x, y, & z between vertices and normals to merge
 On exit    : RpGeometry on success
 */

static RpGeometry  *
OptimizeGeometryOptimize(RpGeometry * geometry, RwReal dist)
{
    RWFUNCTION(RWSTRING("OptimizeGeometryOptimize"));
    if (geometry)
    {
        RwInt32             numVertices, numUniqueVertices;
        RwInt32             numFrames;
        rwVertexInfo       *vertInfo;
        rwUniqueVertices   *uv;

        vertInfo = OptimizeVertexInfoCreate(geometry);
        if (!vertInfo)
        {
            RWRETURN((RpGeometry *) NULL);
        }

        numVertices = RpGeometryGetNumVertices(geometry);
        numFrames = RpGeometryGetNumMorphTargets(geometry);
        if (!OptimizeVertexInfoSortArray(vertInfo, numVertices, numFrames, dist))
        {
            OptimizeVertexInfoDestroy(vertInfo);
            RWRETURN((RpGeometry *) NULL);
        }

        numUniqueVertices = OptimizeVertexInfoCalcUniqueVertices(vertInfo, numVertices);
        uv = OptimizeUniqueVerticesCreate(vertInfo, numVertices, numFrames);
        if (!uv)
        {
            OptimizeVertexInfoDestroy(vertInfo);
            RWRETURN((RpGeometry *) NULL);
        }

        OptimizeGeometrySetUniqueVertices(geometry, uv, numUniqueVertices);
        OptimizeGeometryReMapFaces(geometry, vertInfo);
        OptimizeUniqueVerticesDestroy(uv);
        OptimizeVertexInfoDestroy(vertInfo);
        RWRETURN(geometry);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN((RpGeometry *) NULL);
}

/*
 * \ref _rtGeometryOptimize is used to optimize the specified geometry
 * for rendering by eliminating duplicated materials. It also searches the
 * specified geometry for vertices that are within the given distance of
 * each other and replaces them with a single vertex. Their normals must
 * also be within the given tolerance for the replacement to occur. This
 * function may be used to eliminate vertices from the geometry that are
 * considered coincident within the specified tolerance.
 *
 * Note that only vertices that have the same pre-light color and texture
 * coordinates can be replaced with a single vertex. Also, if there is more
 * than one morph target within the geometry, the position and normal of a
 * specific vertex must be within the specified tolerance over all morph
 * targets for this optimization to be performed.
 *
 * The world plugin must be attached before using this function. The include
 * file rtworld.h and the library file rtworld.lib are also required.
 *
 * \param geometry  Pointer to the geometry.
 * \param dist  A RwReal value equal to the tolerance.
 *
 * \return Returns pointer to the geometry if successful or
 * NULL if there is an error.
 *
 * \see RtClumpOptimize
 * \see RtAtomicOptimize
 * \see RpWorldPluginAttach
 */
RpGeometry         *
_rtGeometryOptimize(RpGeometry * geometry, RwReal dist)
{
    RWFUNCTION(RWSTRING("_rtGeometryOptimize"));
    RWASSERT(geometry);
    if (geometry)
    {
        RpMeshHeader       *meshHeader;

        RWASSERTISTYPE(geometry, rpGEOMETRY);
        /* first remove duplicate vertices and degenerate triangles */
        if (!OptimizeGeometryOptimize(geometry, dist))
        {
            RWRETURN((RpGeometry *) NULL);
        }

        /* Find materials which are the same, and combine them */

        /* First build a list of materials that exist in the geometry */
        meshHeader = geometry->mesh;
        if (meshHeader)
        {
            RwInt32             numMaterials = meshHeader->numMeshes;
            RwUInt32            geometryFlags;

            if (numMaterials < 2)
            {
                /* Can't optimize with less than 2 materials */
                RWRETURN(geometry);
            }

            geometryFlags = RpGeometryGetFlags(geometry);
            /* Now we can start finding duplicate materials
             * and removing them */
            if (RpGeometryLock(geometry, rpGEOMETRYLOCKPOLYGONS))
            {
                /* Do material compares up the list */
                RwInt32             matA = 0;

                while (matA < (numMaterials - 1))
                {
                    RwInt32             matB = matA + 1;

                    while (matB < numMaterials)
                    {
                        if (OptimizeGeometryAreMaterialsSame
                            (rpMaterialListGetMaterial(&geometry->matList, matA),
                             rpMaterialListGetMaterial(&geometry->matList, matB),
                             geometryFlags))
                        {
                            /* Materials are the same, so replace B with A and remove B */
                            OptimizeGeometryReplaceMaterial(geometry,  matB, matA);
                        }

                        matB++;
                    }
                    matA++;
                }

                /* Unlock and return */
                RpGeometryUnlock(geometry);
                RWRETURN(geometry);
            }

            /* Can't optimize because we have a lock failure */
            RWRETURN((RpGeometry *) NULL);
        }

        /* Can't optimize because it seems to be locked */
        RWRETURN((RpGeometry *) NULL);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN((RpGeometry *) NULL);
}
