
/*
 *  The world structure is an Axis aligned BSP tree - essentially
 *  a median K-D tree.
 *  It is constructed automatically from the TkWorldImport structure.
 *  It is also a 'compressed' format such that the world storage is
 *  approximately 10th of the normal size.
 *
 * See
 *     www.cs.sunysb.edu/~algorith/files/kd-trees.shtml
 * and
 *
 *    * Naive k-d -- the original kd-tree defined by Bentley,
 *      "Multidimensional Binary Search Trees Used for Associative Searching"
 *      ACM Sept. 1975 Vol. 18. No. 9
 *    * Median k-d -- A refined kd-tree, using median cuts and bucketing,
 *      discussed in J.H. Friedman, J.L. Bentley, R.A. Finkel "An Algorithm
 *      for Finding Best Matches in Logarithmic Expected Time" ACM
 *      Transactions of Mathematical Software Vol 3 No. 3 Sept. 1977 pp.
 *      209-226
 *    * Sproull k-d -- Sproull's variant of the kd-tree. The choice of
 *      partition plane is not orthogonal, rather, it is selected by the
 *      principal eigenvector of the covariance matrix of the points. R.F.
 *      Sproull "Refinements to Nearest-Neighbor Searching in k-Dimensional
 *      Trees" J. Algorithmica 1990. pp. 579-589
 *    * VP-tree - The vantage point tree is a data structure that chooses
 *      vantage points to perform a spherical decomposition of the search
 *      space. Yianilos claims this method is suited for non-Minkowski metrics
 *      (unlike kd-trees) and for lower dimensional objects embedded in a
 *      higher dimensional space. P.N. Yianilos "Data Structures and
 *      Algorithms for Nearest Neighbor Search in General Metric Spaces" SODA
 *      '93
 *
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

#include "nhsstats.h"
#include "nhsutil.h"
#include "nhsworld.h"
#include "rtgcond.h"
//#include "rtwing.h"

#include "nhssplit.h"
#include "nhsscheme.h"


/* Splitting up the world */
typedef struct _rwCut _rwCut;
struct _rwCut
{
    RwLLLink            lAll;
    RtWorldImportVertex *v1, *v2;
    RwReal              delta;
    RtWorldImportVertex vInterp;
};

typedef struct _rwCutVertex _rwCutVertex;
struct _rwCutVertex
{
    RtWorldImportVertex vpVert;
    RwInt32             type;
    RwReal              value;
};



#if (!defined(SPLIT_EPSILON))
#define SPLIT_EPSILON (((RwReal)1)/((RwReal)(1<<10)))
#endif /* (!defined(SPLIT_EPSILON)) */

#define EQ(a,b) (a - b < SPLIT_EPSILON && a - b > -SPLIT_EPSILON)

#define _rwSListGetNumEntriesMacro(_sList)     \
    ((_sList)->numElementsFilled)

#define CLIPV3D(out, in1, in2, t)               \
    MACRO_START                                 \
    {                                           \
        RwV3dSub((out), (in2), (in1));          \
        RwV3dScale((out), (out), (t));          \
        RwV3dAdd((out), (out), (in1));          \
    }                                           \
    MACRO_STOP

/* Clipping */
#define CLIPREAL(out, in1, in2, t)              \
    (out) = (((in2) - (in1)) * (t)) + (in1);

#define CLIPCOLOR(out, in1, in2, t)                     \
    MACRO_START                                         \
    {                                                   \
        RwRGBAReal       clippedCol, inpA;              \
                                                        \
        RwRGBARealFromRwRGBA(&inpA, (in1));             \
        RwRGBARealFromRwRGBA(&clippedCol, (in2));       \
        RwRGBARealSub(&clippedCol, &clippedCol, &inpA); \
        RwRGBARealScale(&clippedCol, &clippedCol, (t)); \
        RwRGBARealAdd(&clippedCol, &clippedCol, &inpA); \
        RwRGBAFromRwRGBAReal((out), &clippedCol);       \
    }                                                   \
    MACRO_STOP




/***********************************************************************/



static RtWorldImportBuildVertex *
ImportAddVertToBoundaries(RtWorldImportBuildSector * buildSector,
                          RtWorldImportVertex * vpVert, RwInt32 index)
{
    RwInt32             i;
    RtWorldImportBuildVertex *newBoundaries;
    RtWorldImportVertex *newVertices;
    RtWorldImportBuildVertexMode *mode;

    RWFUNCTION(RWSTRING("ImportAddVertToBoundaries"));

    newBoundaries = (RtWorldImportBuildVertex *)
        RwMalloc(sizeof(RtWorldImportBuildVertex) *
                 (buildSector->numBoundaries + 1),
                 rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
    newVertices = (RtWorldImportVertex *)
        RwMalloc(sizeof(RtWorldImportVertex) *
                 (buildSector->numVertices + 1),
                 rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);

    memcpy(newVertices, buildSector->vertices,
           sizeof(RtWorldImportVertex) * buildSector->numVertices);

    for (i = 0; i < buildSector->numVertices; i++)
    {
        buildSector->vertices[i].state.vpVert = &(newVertices[i]);
    }

    for (i = 0; i < buildSector->numBoundaries; i++)
    {
        RtWorldImportBuildVertex *boundary =
            &buildSector->boundaries[i];
        RtWorldImportBuildVertexMode *const mode = &boundary->mode;

        if (mode->vpVert)
        {
            mode->vpVert = mode->vpVert->state.vpVert;
        }
    }

    RwFree(buildSector->vertices);
    buildSector->vertices = newVertices;

    buildSector->vertices[buildSector->numVertices] = *vpVert;
    buildSector->numVertices++;

    for (i = 0; i < index; i++)
    {
        newBoundaries[i] = buildSector->boundaries[i];
    }

    mode = &newBoundaries[index].mode;
    mode->vpVert = &buildSector->vertices[buildSector->numVertices - 1];

    for (i = index; i < buildSector->numBoundaries; i++)
    {
        newBoundaries[i + 1] = buildSector->boundaries[i];
    }

    buildSector->numBoundaries++;
    RwFree(buildSector->boundaries);
    buildSector->boundaries = newBoundaries;
    RWRETURN(newBoundaries);
}

/* We use this version when the vertex is being added as the
   2nd vertex in the boundary since we fan on vertex 1 and the
   standard insertion would cause degenerate triangles */
static RtWorldImportBuildVertex *
ImportAddVertToBoundariesAfter1stVert(RtWorldImportBuildSector *
                                      buildSector,
                                      RtWorldImportVertex * vpVert,
                                      RwInt32 index)
{
    RwInt32             i;
    RwInt32             lastVert;
    RwInt32             numVertices;
    RtWorldImportVertex *vertices;
    RtWorldImportVertex *newVertices;
    RwInt32             numBoundaries;
    RtWorldImportBuildVertex *boundaries;
    RtWorldImportBuildVertex *newBoundaries;
    RtWorldImportBuildVertexMode *mode;

    RWFUNCTION(RWSTRING("ImportAddVertToBoundariesAfter1stVert"));

    numVertices = buildSector->numVertices;
    vertices = buildSector->vertices;
    newVertices = (RtWorldImportVertex *)
        RwMalloc(sizeof(RtWorldImportVertex) * (numVertices + 1),
                 rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);

    numBoundaries = buildSector->numBoundaries;
    boundaries = buildSector->boundaries;
    newBoundaries = (RtWorldImportBuildVertex *)
        RwMalloc(sizeof(RtWorldImportBuildVertex) *
                 (numBoundaries + 1), rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);

    memcpy(newVertices, vertices,
           sizeof(RtWorldImportVertex) * numVertices);

    for (i = 0; i < numVertices; i++)
    {
        vertices[i].state.vpVert = &(newVertices[i]);
    }

    for (i = 0; i < numBoundaries; i++)
    {
        RtWorldImportBuildVertex *const boundary = &boundaries[i];

        mode = &boundary->mode;

        if (mode->vpVert)
        {
            mode->vpVert = mode->vpVert->state.vpVert;
        }
    }
    RwFree(vertices);
    buildSector->vertices = newVertices;

    buildSector->vertices[buildSector->numVertices] = *vpVert;
    buildSector->numVertices++;

    for (i = 0; i < index; i++)
    {
        newBoundaries[i] = boundaries[i];
    }

    /* find last vertex in boundary */
    lastVert = index;

    for (mode = &boundaries[lastVert + 1].mode;
         mode->vpVert; mode = &boundaries[lastVert + 1].mode)
    {
        lastVert++;
    }

    newBoundaries[index] = boundaries[lastVert];
    newBoundaries[index + 1] = boundaries[index];

    mode = &newBoundaries[index + 2].mode;
    mode->vpVert = &buildSector->vertices[buildSector->numVertices - 1];

    for (i = index + 1; i < lastVert; i++)
    {
        newBoundaries[i + 2] = boundaries[i];
    }

    for (i = lastVert + 1; i < buildSector->numBoundaries; i++)
    {
        newBoundaries[i + 1] = boundaries[i];
    }

    buildSector->numBoundaries++;
    RwFree(boundaries);
    buildSector->boundaries = newBoundaries;

    RWRETURN(newBoundaries);
}

/* We use this version when the vertex is being added as the
   1st vertex in the boundary since we fan on vertex 1 and the
   standard insertion would cause degenerate triangles */
static RtWorldImportBuildVertex *
ImportAddVertToBoundariesBefore1stVert(RtWorldImportBuildSector *
                                       buildSector,
                                       RtWorldImportVertex * vpVert,
                                       RwInt32 index)
{
    RwInt32             i;
    RwInt32             lastVert;
    RtWorldImportBuildVertexMode *mode;
    RtWorldImportBuildVertex *newBoundaries;
    RtWorldImportVertex *newVertices;

    RWFUNCTION(RWSTRING("ImportAddVertToBoundariesBefore1stVert"));

    newBoundaries = (RtWorldImportBuildVertex *)
        RwMalloc(sizeof(RtWorldImportBuildVertex) *
                 (buildSector->numBoundaries + 1),
                 rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
    newVertices = (RtWorldImportVertex *)
        RwMalloc(sizeof(RtWorldImportVertex) *
                 (buildSector->numVertices + 1),
                 rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);

    memcpy(newVertices, buildSector->vertices,
           sizeof(RtWorldImportVertex) * buildSector->numVertices);
    for (i = 0; i < buildSector->numVertices; i++)
    {
        buildSector->vertices[i].state.vpVert = &(newVertices[i]);
    }
    for (i = 0; i < buildSector->numBoundaries; i++)
    {
        RtWorldImportBuildVertex *const boundary =
            &buildSector->boundaries[i];
        RtWorldImportBuildVertexMode *const mode = &boundary->mode;

        if (mode->vpVert)
        {
            mode->vpVert = mode->vpVert->state.vpVert;
        }
    }
    RwFree(buildSector->vertices);
    buildSector->vertices = newVertices;

    buildSector->vertices[buildSector->numVertices] = *vpVert;
    buildSector->numVertices++;

    for (i = 0; i < index; i++)
    {
        newBoundaries[i] = buildSector->boundaries[i];
    }

    /* find last vertex in boundary */
    lastVert = index;

    for (mode = &buildSector->boundaries[lastVert + 1].mode;
         mode->vpVert;
         mode = &buildSector->boundaries[lastVert + 1].mode)
    {
        lastVert++;
    }

    newBoundaries[index] = buildSector->boundaries[lastVert - 1];
    newBoundaries[index + 1] = buildSector->boundaries[lastVert];

    mode = &newBoundaries[index + 2].mode;
    mode->vpVert = &buildSector->vertices[buildSector->numVertices - 1];
    for (i = index; i < lastVert - 1; i++)
    {
        newBoundaries[i + 3] = buildSector->boundaries[i];
    }

    for (i = lastVert + 1; i < buildSector->numBoundaries; i++)
    {
        newBoundaries[i + 1] = buildSector->boundaries[i];
    }

    buildSector->numBoundaries++;
    RwFree(buildSector->boundaries);
    buildSector->boundaries = newBoundaries;
    RWRETURN(newBoundaries);
}

#ifdef RWDEBUG
static RwInt32             gAnalysis = 0;
#endif

static void
ImportCutVertFixupTJunctions(_rwCutVertex * cutVert,
                             RtWorldImportBuildSector * buildSector,
                             RtWorldImportUserdataCallBacks *
                             UserDataCallBacks)
{
    RwInt32             firstVertInBoundary = 0;
    RwInt32             nI;
    RtWorldImportParameters *conversionParams = RtWorldImportParametersGet();

    RWFUNCTION(RWSTRING("ImportCutVertFixupTJunctions"));

    for (nI = 0; nI < buildSector->numBoundaries; nI++)
    {
        RwInt32             vert2;
        RtWorldImportBuildVertexMode *modenI;
        RtWorldImportBuildVertexMode *modevert2;

        modenI = &buildSector->boundaries[nI].mode;
        if (!modenI->vpVert)
        {
            firstVertInBoundary = nI + 1;
            continue;
        }

        modevert2 = &buildSector->boundaries[nI + 1].mode;
        if (modevert2->vpVert)
        {
            vert2 = nI + 1;
        }
        else
        {
            vert2 = firstVertInBoundary;
        }
        modevert2 = &buildSector->boundaries[vert2].mode;

        if ((GETCOORD(modenI->vpVert->OC, cutVert->type) <
             cutVert->value &&
             GETCOORD(modevert2->vpVert->OC, cutVert->type) >
             cutVert->value) ||
            (GETCOORD(modenI->vpVert->OC, cutVert->type) >
             cutVert->value &&
             GETCOORD(modevert2->vpVert->OC, cutVert->type) <
             cutVert->value))
        {
            /* ok it's an edge crossing the plane which the split
             * vertex was split by. Are they along the same line?
             * First check for differing vertices so we actually
             * have 2 edges */
            if ((!EQ(modenI->vpVert->OC.x, modevert2->vpVert->OC.x) ||
                 !EQ(modenI->vpVert->OC.y, modevert2->vpVert->OC.y) ||
                 !EQ(modenI->vpVert->OC.z, modevert2->vpVert->OC.z)) &&
                (!EQ(modenI->vpVert->OC.x, cutVert->vpVert.OC.x) ||
                 !EQ(modenI->vpVert->OC.y, cutVert->vpVert.OC.y) ||
                 !EQ(modenI->vpVert->OC.z, cutVert->vpVert.OC.z)))
            {
                RwV3d               lineVector, lineVector2;
                RwV3d               normalizedLineVector;
                RwV3d               normalizedLineVector2;
                RwReal              val1, val2;

                RwV3dSub(&lineVector,
                         &modevert2->vpVert->OC, &modenI->vpVert->OC);
                val1 =
                    RwV3dNormalize(&normalizedLineVector, &lineVector);

                RwV3dSub(&lineVector2, &cutVert->vpVert.OC,
                         &modenI->vpVert->OC);
                val2 = RwV3dNormalize(&normalizedLineVector2,
                                      &lineVector2);

                if (val2 < val1 && val2 > 0.0f &&
                    EQ(normalizedLineVector.x,
                       normalizedLineVector2.x) &&
                    EQ(normalizedLineVector.y,
                       normalizedLineVector2.y) &&
                    EQ(normalizedLineVector.z, normalizedLineVector2.z))
                {
                    RwInt32     nJ;

                    /* along the line, splitting the verts,
                     * so this is a T-Junction */
                    RtWorldImportVertex *const vpVertnI =
                        modenI->vpVert;
                    RtWorldImportVertex *const vpVertvert2 =
                        modevert2->vpVert;
                    RwReal              delta = val2 / val1;
                    RtWorldImportVertex newVert;

                    RtWorldImportInterpVertexUserdataCallBack
                        interpVertexUserdata;

                    /* ensure it has the same (identical)
                     * position as the cut */
                    newVert.OC = cutVert->vpVert.OC;
                    CLIPV3D(&newVert.normal,
                            &vpVertnI->normal,
                            &vpVertvert2->normal, delta);
                    CLIPCOLOR(&newVert.preLitCol,
                              &vpVertnI->preLitCol,
                              &vpVertvert2->preLitCol, delta);

                    for (nJ=0; nJ<conversionParams->numTexCoordSets; nJ++)
                    {
                        CLIPREAL(newVert.texCoords[nJ].u,
                                 vpVertnI->texCoords[nJ].u,
                                 vpVertvert2->texCoords[nJ].u, delta);
                        CLIPREAL(newVert.texCoords[nJ].v,
                                 vpVertnI->texCoords[nJ].v,
                                 vpVertvert2->texCoords[nJ].v, delta);
                    }

                    newVert.pUserdata = NULL;

                    interpVertexUserdata =
                        UserDataCallBacks->interpVertexUserdata;
                    if (interpVertexUserdata)
                    {
                        interpVertexUserdata(&newVert.pUserdata,
                                             &vpVertnI->pUserdata,
                                             &vpVertvert2->pUserdata,
                                             delta);
                    }

                    if (nI == firstVertInBoundary)
                    {
                        buildSector->boundaries =
                            ImportAddVertToBoundariesAfter1stVert
                            (buildSector, &newVert,
                             firstVertInBoundary);
                        /* and jump back to retest whole boundary */
                        nI = firstVertInBoundary - 1;
                    }
                    else if (vert2 == firstVertInBoundary)
                    {
                        buildSector->boundaries =
                            ImportAddVertToBoundariesBefore1stVert
                            (buildSector, &newVert,
                             firstVertInBoundary);
                        /* and jump back to retest whole boundary */
                        nI = firstVertInBoundary - 1;
                    }
                    else
                    {
                        buildSector->boundaries =
                            ImportAddVertToBoundaries(buildSector,
                                                      &newVert, vert2);
                    }
                }
            }
        }
    }

    RWRETURNVOID();
}




static void
ImportAddCut(RwFreeList * flpCuts, RwLinkList * cuts,
             RtWorldImportBuildVertex * v1,
             RtWorldImportBuildVertex * v2, RwInt32 type, RwReal value,
             RtWorldImportBuildVertex * vpInterp,
             RtWorldImportUserdataCallBacks * UserDataCallBacks)
{
    RtWorldImportBuildVertexMode *mode;
    RwLLLink           *cur, *end;
    _rwCut             *cut;
    RtWorldImportParameters *conversionParams = RtWorldImportParametersGet();

    RWFUNCTION(RWSTRING("ImportAddCut"));

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe Check
	THREADSAFE_CHECK_ISCALLEDMAIN();
	//@} DDonSS

    /* condition endpoints */
    if (v1 > v2)
    {
        RtWorldImportBuildVertex *vTmp;

        vTmp = v1;
        v1 = v2;
        v2 = vTmp;
    }

    /* first see if we've done this edge already */
    cut = (_rwCut *) NULL;
    cur = rwLinkListGetFirstLLLink(cuts);
    end = rwLinkListGetTerminator(cuts);
    while (cur != end)
    {
        RtWorldImportBuildVertexMode const *modev1 = &v1->mode;
        RtWorldImportBuildVertexMode const *modev2 = &v2->mode;

        cut = rwLLLinkGetData(cur, _rwCut, lAll);
        if ((cut->v1 == modev1->vpVert) && (cut->v2 == modev2->vpVert))
        {
            break;
        }
        cut = (_rwCut *) NULL;

        /* Onto the next cut */
        cur = rwLLLinkGetNext(cur);
    }

    /* ok so create a new one */
    if (!cut)
    {
        RwInt32     nI;

        RtWorldImportBuildVertexMode const *modev1 = &v1->mode;
        RtWorldImportBuildVertexMode const *modev2 = &v2->mode;
        RtWorldImportInterpVertexUserdataCallBack interpVertexUserdata;

        cut = (_rwCut *) RwFreeListAlloc(flpCuts,
            rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
        rwLLLinkInitialize(&cut->lAll);
        rwLinkListAddLLLink(cuts, &cut->lAll);

        cut->v1 = modev1->vpVert;
        cut->v2 = modev2->vpVert;
        cut->delta = (value - GETCOORD(modev1->vpVert->OC, type)) /
            (GETCOORD(modev2->vpVert->OC, type) -
             GETCOORD(modev1->vpVert->OC, type));

        /* clip vertex data */
        CLIPV3D(&cut->vInterp.OC,
                &modev1->vpVert->OC, &modev2->vpVert->OC, cut->delta);
        CLIPV3D(&cut->vInterp.normal,
                &modev1->vpVert->normal, &modev2->vpVert->normal,
                cut->delta);
        CLIPCOLOR(&cut->vInterp.preLitCol,
                  &modev1->vpVert->preLitCol,
                  &modev2->vpVert->preLitCol, cut->delta);

        nI = conversionParams->numTexCoordSets;
        while(nI--)
        {
            CLIPREAL(cut->vInterp.texCoords[nI].u,
                     modev1->vpVert->texCoords[nI].u,
                     modev2->vpVert->texCoords[nI].u, cut->delta);
            CLIPREAL(cut->vInterp.texCoords[nI].v,
                     modev1->vpVert->texCoords[nI].v,
                     modev2->vpVert->texCoords[nI].v, cut->delta);
        }

        cut->vInterp.pUserdata = NULL;
        interpVertexUserdata = UserDataCallBacks->interpVertexUserdata;

        if (interpVertexUserdata)
        {
            interpVertexUserdata(&cut->vInterp.pUserdata,
                                 &modev1->vpVert->pUserdata,
                                 &modev2->vpVert->pUserdata,
                                 cut->delta);
        }
    }

    mode = &vpInterp->mode;
    mode->vpVert = &cut->vInterp;

    RWRETURNVOID();
}

static RtWorldImportVertex *
ImportRemapVertices(RwSList * boundaries, RwInt32 * total,
                    RtWorldImportUserdataCallBacks * UserDataCallBacks)
{
    RwInt32             nI, vcount;
    RtWorldImportBuildVertex *vpTmp;
    RtWorldImportVertex *vertices;
    RtWorldImportCloneVertexUserdataCallBack cloneVertexUserdata;

    RWFUNCTION(RWSTRING("ImportRemapVertices"));

    if (_rwSListGetNumEntriesMacro(boundaries) == 0)
    {
        *total = 0;
        RWRETURN((RtWorldImportVertex *) NULL);
    }

    /* clear all */
    vpTmp = (RtWorldImportBuildVertex *) rwSListGetEntry(boundaries, 0);
    for (nI = 0; nI < _rwSListGetNumEntriesMacro(boundaries);
         nI++, vpTmp++)
    {
        RtWorldImportBuildVertexMode *const mode = &vpTmp->mode;

        if (mode->vpVert)
        {
            mode->vpVert->state.forwardingAddress = -1;
        }
    }

    /* mark and count */
    vcount = 0;
    vpTmp = (RtWorldImportBuildVertex *) rwSListGetEntry(boundaries, 0);
    for (nI = 0; nI < _rwSListGetNumEntriesMacro(boundaries);
         nI++, vpTmp++)
    {
        RtWorldImportBuildVertexMode *const mode = &vpTmp->mode;

        if (mode->vpVert)
        {
            if (mode->vpVert->state.forwardingAddress < 0)
            {
                mode->vpVert->state.forwardingAddress = vcount++;
            }
        }
    }
    *total = vcount;

    vertices = (RtWorldImportVertex *)
        RwMalloc(sizeof(RtWorldImportVertex) * vcount,
            rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
    vpTmp = (RtWorldImportBuildVertex *) rwSListGetEntry(boundaries, 0);

    cloneVertexUserdata = UserDataCallBacks->cloneVertexUserdata;

    if (cloneVertexUserdata)
    {
        for (nI = 0; nI < _rwSListGetNumEntriesMacro(boundaries);
             nI++, vpTmp++)
        {
            RtWorldImportBuildVertexMode *const mode = &vpTmp->mode;

            if (mode->vpVert)
            {
                /* copy over vertex */
                vertices[mode->vpVert->state.forwardingAddress] =
                    *mode->vpVert;

                /* clone the userdata */
                cloneVertexUserdata(&vertices
                                    [mode->vpVert->state.
                                     forwardingAddress].pUserdata,
                                    &mode->vpVert->pUserdata);

                /* point to new vertex */
                mode->vpVert =
                    &vertices[mode->vpVert->state.forwardingAddress];
            }
        }
    }
    else
    {
        for (nI = 0; nI < _rwSListGetNumEntriesMacro(boundaries);
             nI++, vpTmp++)
        {
            RtWorldImportBuildVertexMode *const mode = &vpTmp->mode;

            if (mode->vpVert)
            {
                /* copy over vertex */
                vertices[mode->vpVert->state.forwardingAddress] =
                    *mode->vpVert;

                /* point to new vertex */
                mode->vpVert =
                    &vertices[mode->vpVert->state.forwardingAddress];
            }
        }
    }

    RWRETURN(vertices);
}

static void
ImportDestroyCuts(RwFreeList * flpCuts, RwLinkList * list,
                  RtWorldImportUserdataCallBacks * UserDataCallBacks)
{
    RwLLLink           *cur, *end;
    RtWorldImportDestroyVertexUserdataCallBack destroyVertexUserdata;

    RWFUNCTION(RWSTRING("ImportDestroyCuts"));
    RWASSERT(flpCuts);
    RWASSERT(list);

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe Check
	THREADSAFE_CHECK_ISCALLEDMAIN();
	//@} DDonSS

    cur = rwLinkListGetFirstLLLink(list);
    end = rwLinkListGetTerminator(list);
    destroyVertexUserdata = UserDataCallBacks->destroyVertexUserdata;

    if (destroyVertexUserdata)
    {
        while (cur != end)
        {
            _rwCut             *cut =
                rwLLLinkGetData(cur, _rwCut, lAll);

            destroyVertexUserdata(&cut->vInterp.pUserdata);

            cur = rwLLLinkGetNext(cur);
            RwFreeListFree(flpCuts, cut);
        }
    }
    else
    {
        while (cur != end)
        {
            _rwCut             *cut =
                rwLLLinkGetData(cur, _rwCut, lAll);

            cur = rwLLLinkGetNext(cur);
            RwFreeListFree(flpCuts, cut);
        }
    }

    RWRETURNVOID();
}


/****************************************************************************
 ImportBuildSectorSplit

 Partitions the polygon set amongst
 two new BuildSectors
 according to the previous setup clip Flags

*/
static              RwBool
ImportBuildSectorSplit(RtWorldImportBuildSector * buildSector,
                       RtWorldImportPartition * partition,
                       RtWorldImportBuildSector ** buildSectorOutLeft,
                       RtWorldImportBuildSector ** buildSectorOutRight,
                       RwSList * cutVertexList,
                       RtWorldImportUserdataCallBacks *
                       UserDataCallBacks)
{
    RwInt32             nI;
    RtWorldImportBuildSector *newleft;
    RtWorldImportBuildSector *newright;
    RtWorldImportBuildVertex *vpFirst;
    RtWorldImportBuildVertex *vpCurr;
    RtWorldImportBuildVertex *vpPrev;
    RtWorldImportBuildVertex *vpTri;
    RtWorldImportBuildVertex *vpTmp;
    RwSList            *leftboundaries, *rightboundaries;
    RwFreeList         *flpLeftCuts, *flpRightCuts;
    RwLinkList          leftcuts, rightcuts;
    RwBBox              bbox;
    RwInt32             clip, leftcount, rightcount;
    RwReal              distLeft, distRight;
    RwInt32             side;
    RwBool              mustClipAll = FALSE;
    RwReal              leftClipExtent, rightClipExtent;
    RtWorldImportParameters *conversionParams = RtWorldImportParametersGet();
    RwV3d               edge[3];
    RwV3d               triNormal;
    RtWorldImportVertex *v0, *v1, *v2;
    RwReal              pointing;

    RWFUNCTION(RWSTRING("ImportBuildSectorSplit"));

    /* Clip extents...This is the positive relative maximum overlap gap between
     * the central partition and the face of its overlap. E.g....
     *
     * (L-inf)   0  right=3  val=6     left=16          26   (R+sup)
     *           |        |      |          |           |
     *           |        |lce=3 |   rce=10 |           |
     *                    #-----------------------------#
     *           #==========================#
     *
     * NB right=3 and left=16 are on the correct sides, 'right' is ultimately
     * one of the faces that belongs to the right bbox #-#. Likewise for left and #=#.
     */
    leftClipExtent = partition->value - partition->maxRightValue;
    rightClipExtent = partition->maxLeftValue - partition->value;

    leftboundaries = rwSListCreate(sizeof(RtWorldImportBuildVertex),
                                   rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
    rightboundaries = rwSListCreate(sizeof(RtWorldImportBuildVertex),
                                    rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);

    flpLeftCuts = RwFreeListCreate(sizeof(_rwCut), 100, sizeof(RwUInt32),
                                   rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_FUNCTION);
    flpRightCuts = RwFreeListCreate(sizeof(_rwCut), 100, sizeof(RwUInt32),
                                    rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_FUNCTION);
    rwLinkListInitialize(&leftcuts);
    rwLinkListInitialize(&rightcuts);

    leftcount = rightcount = 0;
    distLeft = distRight = 0.0f;
    clip = rwCLIPVERTEXLEFT | rwCLIPVERTEXRIGHT;
    vpFirst = vpCurr = buildSector->boundaries;

    /* Cycle through all the vertices and polygons, putting them in the left
     * or right sectors. If split by the central partition, they are placed
     * on the majority side - if they span the entire overlap, they are split */
    for (nI = 0; nI < buildSector->numBoundaries; nI++, vpCurr++)
    {
        RtWorldImportBuildVertexMode *const modevpCurr = &vpCurr->mode;

        if (modevpCurr->vpVert)
        {
            RwReal              dist =
                GETCOORD(modevpCurr->vpVert->OC, partition->type) - partition->value;

            /* track total clip flags */
            clip &= modevpCurr->vpVert->state.clipFlags[0];

            /* track distances from plane */
            if (modevpCurr->vpVert->state.clipFlags[0] == rwCLIPVERTEXLEFT)
            {
                if (-dist > distLeft)
                {
                    distLeft = -dist;
                }
            }
            else if (modevpCurr->vpVert->state.clipFlags[0] ==
                     rwCLIPVERTEXRIGHT)
            {
                if (dist > distRight)
                {
                    distRight = dist;
                }
            }
            continue; /* NB: here we skip everything else, until we have all our verts! */
        }

        /* to do, add this to stats!!! ref:aj  */
        if (clip==(rwCLIPVERTEXLEFT | rwCLIPVERTEXRIGHT))
        {
            /* Here, we have a triangle that is coplanar with a partition
             * We place it in the sector that's on its front, since this is
             * ideal for PVS
             */
            RtWorldImportBuildVertex *tempCurr = vpCurr;

            v0 = (tempCurr-3)->mode.vpVert;
            v1 = (tempCurr-2)->mode.vpVert;
            v2 = (tempCurr-1)->mode.vpVert;

            RwV3dSub(&edge[0], &v1->OC, &v0->OC);
            RwV3dSub(&edge[1], &v2->OC, &v1->OC);
            RwV3dSub(&edge[2], &v0->OC, &v2->OC);

            /* Calculate the polygon normal */
            RwV3dCrossProduct(&triNormal, &edge[0], &edge[1]);

            /* must be tangent to an orthogonal plane, thus have only one non-zero
             * value in the normal
             */



            if (partition->type == 0) pointing = triNormal.x + v0->OC.x;
            else if (partition->type == 4) pointing = triNormal.y + v0->OC.y;
            else pointing = triNormal.z + v0->OC.z;


            if (pointing < partition->value) /* NB, this promotes polys to their facing side */
            {
                clip = rwCLIPVERTEXLEFT;
            }
            else
            {
                clip = rwCLIPVERTEXRIGHT;
            }
        }

        if (distLeft + distRight > conversionParams->worldSectorMaxSize &&
            clip == 0)
        {
            /* this polygon crosses the plane and will never fit in a finalised
               sector so force it to be split now */
            distLeft = leftClipExtent + 1.0f;
            distRight = rightClipExtent + 1.0f;
        }

        /* ok we've got to end of polygon's vertices */
        switch (clip)
        {
                /* wholly on the left */
            case rwCLIPVERTEXLEFT:

                vpTri = vpFirst;
                while (vpTri != vpCurr)
                {
                    vpTmp = (RtWorldImportBuildVertex *)
                        rwSListGetNewEntry(leftboundaries,
                                           rwID_NOHSWORLDPLUGIN |
                                           rwMEMHINTDUR_EVENT);
                    *vpTmp = *vpTri;
                    vpTri++;
                }
                vpTmp = (RtWorldImportBuildVertex *)
                    rwSListGetNewEntry(leftboundaries,
                                       rwID_NOHSWORLDPLUGIN |
                                       rwMEMHINTDUR_EVENT);
                *vpTmp = *vpCurr;
                leftcount++;
                break;

                /* wholly on the right */
            case rwCLIPVERTEXLEFT | rwCLIPVERTEXRIGHT:
            case rwCLIPVERTEXRIGHT:

                vpTri = vpFirst;
                while (vpTri != vpCurr)
                {
                    vpTmp = (RtWorldImportBuildVertex *)
                        rwSListGetNewEntry(rightboundaries,
                                           rwID_NOHSWORLDPLUGIN |
                                           rwMEMHINTDUR_EVENT);
                    *vpTmp = *vpTri;
                    vpTri++;
                }
                vpTmp = (RtWorldImportBuildVertex *)
                    rwSListGetNewEntry(rightboundaries,
                                       rwID_NOHSWORLDPLUGIN |
                                       rwMEMHINTDUR_EVENT);
                *vpTmp = *vpCurr;
                rightcount++;
                break;

                /* its a split */
            case 0:

                /* which side should we stick it on */
                if (mustClipAll ||
                    (distLeft > leftClipExtent &&
                     distRight > rightClipExtent))
                {
                    side = -1;
                }
                else
                {
                    if (distLeft > distRight)
                    {
                        if (distRight > rightClipExtent)
                        {
                            side = -1; /* split */
                        }
                        else
                        {
                            side = 0; /* left */
                        }
                    }
                    else
                    {
                        if (distLeft > leftClipExtent)
                        {
                            side = -1; /* split */
                        }
                        else
                        {
                            side = 1;/* right */
                        }
                    }
                }

                if (side == 0) /* put it on left */
                {
                    vpTri = vpFirst;
                    while (vpTri != vpCurr)
                    {
                        vpTmp = (RtWorldImportBuildVertex *)
                            rwSListGetNewEntry(leftboundaries,
                                               rwID_NOHSWORLDPLUGIN |
                                               rwMEMHINTDUR_EVENT);
                        *vpTmp = *vpTri;
                        vpTri++;
                    }
                    vpTmp = (RtWorldImportBuildVertex *)
                        rwSListGetNewEntry(leftboundaries,
                                           rwID_NOHSWORLDPLUGIN |
                                           rwMEMHINTDUR_EVENT);
                    *vpTmp = *vpCurr;
                    leftcount++;
                }
                else if (side == 1) /* put it on right */
                {
                    vpTri = vpFirst;
                    while (vpTri != vpCurr)
                    {
                        vpTmp = (RtWorldImportBuildVertex *)
                            rwSListGetNewEntry(rightboundaries,
                                               rwID_NOHSWORLDPLUGIN |
                                               rwMEMHINTDUR_EVENT);
                        *vpTmp = *vpTri;
                        vpTri++;
                    }
                    vpTmp = (RtWorldImportBuildVertex *)
                        rwSListGetNewEntry(rightboundaries,
                                           rwID_NOHSWORLDPLUGIN |
                                           rwMEMHINTDUR_EVENT);
                    *vpTmp = *vpCurr;
                    rightcount++;
                }
                else /* split the damn thing then! */
                {
                    RtWorldImportBuildVertexMode *const modevpFirst =
                        &vpFirst->mode;
                    RtWorldImportBuildVertexMode *modevpPrev;
                    RtWorldImportBuildVertexMode *modevpTri;

                    RtWorldImportSplitPolygonUserdataCallBack
                        splitPolygonUserdata;

                    clip = 0;

                    vpPrev = vpFirst;
                    vpTri = vpFirst + 1;
                    while (vpTri != vpCurr)
                    {
                        modevpPrev = &vpPrev->mode;
                        modevpTri = &vpTri->mode;

                        clip |= modevpPrev->vpVert->state.clipFlags[0];
                        if (modevpPrev->vpVert->state.clipFlags[0] &
                            rwCLIPVERTEXLEFT)
                        {
                            vpTmp = (RtWorldImportBuildVertex *)
                                rwSListGetNewEntry(leftboundaries,
                                                   rwID_NOHSWORLDPLUGIN |
                                                   rwMEMHINTDUR_EVENT);
                            *vpTmp = *vpPrev;
                        }
                        if (modevpPrev->vpVert->state.clipFlags[0] &
                            rwCLIPVERTEXRIGHT)
                        {
                            vpTmp = (RtWorldImportBuildVertex *)
                                rwSListGetNewEntry(rightboundaries,
                                                   rwID_NOHSWORLDPLUGIN |
                                                   rwMEMHINTDUR_EVENT);
                            *vpTmp = *vpPrev;
                        }
                        if (!(modevpPrev->vpVert->state.clipFlags[0] &
                              modevpTri->vpVert->state.clipFlags[0]))
                        {
                            vpTmp = (RtWorldImportBuildVertex *)
                                rwSListGetNewEntry(leftboundaries, rwID_NOHSWORLDPLUGIN |
                                rwMEMHINTDUR_EVENT);
                            ImportAddCut(flpLeftCuts, &leftcuts,
                                         vpPrev, vpTri, partition->type, partition->value,
                                         vpTmp,
                                         UserDataCallBacks);
                            vpTmp = (RtWorldImportBuildVertex *)
                                rwSListGetNewEntry(rightboundaries, rwID_NOHSWORLDPLUGIN |
                                rwMEMHINTDUR_EVENT);
                            ImportAddCut(flpRightCuts, &rightcuts,
                                         vpPrev, vpTri, partition->type, partition->value,
                                         vpTmp,
                                         UserDataCallBacks);

                            /* track all cuts */
                            {
                                _rwCutVertex       *temp =
                                    (_rwCutVertex *)
                                    rwSListGetNewEntry(cutVertexList,
                                        rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_FUNCTION);
                                RtWorldImportBuildVertexMode *const
                                    modevpTmp = &vpTmp->mode;

                                temp->type = partition->type;
                                temp->value = partition->value;
                                temp->vpVert = *(modevpTmp->vpVert);
                            }

                        }
                        vpPrev++;
                        vpTri++;
                    }

                    /* do closing edge too */
                    modevpPrev = &vpPrev->mode;

                    clip |= modevpPrev->vpVert->state.clipFlags[0];
                    if (modevpPrev->vpVert->state.
                        clipFlags[0] & rwCLIPVERTEXLEFT)
                    {
                        vpTmp = (RtWorldImportBuildVertex *)
                            rwSListGetNewEntry(leftboundaries,
                            rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
                        *vpTmp = *vpPrev;
                    }
                    if (modevpPrev->vpVert->state.
                        clipFlags[0] & rwCLIPVERTEXRIGHT)
                    {
                        vpTmp = (RtWorldImportBuildVertex *)
                            rwSListGetNewEntry(rightboundaries,
                            rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
                        *vpTmp = *vpPrev;
                    }
                    if (!(modevpPrev->vpVert->state.clipFlags[0] &
                          modevpFirst->vpVert->state.clipFlags[0]))
                    {
                        vpTmp = (RtWorldImportBuildVertex *)
                            rwSListGetNewEntry(leftboundaries,
                            rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
                        ImportAddCut(flpLeftCuts, &leftcuts, vpPrev,
                                     vpFirst, partition->type, partition->value, vpTmp,
                                     UserDataCallBacks);

                        vpTmp = (RtWorldImportBuildVertex *)
                            rwSListGetNewEntry(rightboundaries,
                            rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
                        ImportAddCut(flpRightCuts, &rightcuts, vpPrev,
                                     vpFirst, partition->type, partition->value, vpTmp,
                                     UserDataCallBacks);

                        /* track all cuts */
                        {
                            _rwCutVertex       *temp =
                                (_rwCutVertex *)
                                rwSListGetNewEntry(cutVertexList,
                                    rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_FUNCTION);
                            RtWorldImportBuildVertexMode *const
                                modevpTmp = &vpTmp->mode;

                            temp->type = partition->type;
                            temp->value = partition->value;
                            temp->vpVert = *(modevpTmp->vpVert);
                        }
                    }

                    splitPolygonUserdata =
                        UserDataCallBacks->splitPolygonUserdata;

                    /* mark end of polygons (if we created any) */
                    if (splitPolygonUserdata
                        && (clip & rwCLIPVERTEXLEFT)
                        && (clip & rwCLIPVERTEXRIGHT)
                        && vpCurr->pinfo.pUserdata)
                    {
                        /* if splitting the polygon and the callback is
                         * set split the polyinfo */
                        vpTmp = (RtWorldImportBuildVertex *)
                            rwSListGetNewEntry(leftboundaries,
                                rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
                        *vpTmp = *vpCurr;
                        vpTmp->pinfo.pUserdata = NULL;
                        splitPolygonUserdata(&vpTmp->pinfo.
                                             pUserdata,
                                             &vpCurr->pinfo.pUserdata);
                        leftcount++;

                        vpTmp = (RtWorldImportBuildVertex *)
                            rwSListGetNewEntry(rightboundaries,
                                rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
                        *vpTmp = *vpCurr;
                        vpTmp->pinfo.pUserdata = NULL;
                        splitPolygonUserdata(&vpTmp->pinfo.
                                             pUserdata,
                                             &vpCurr->pinfo.pUserdata);
                        rightcount++;

                    }
                    else
                    {
                        if (clip & rwCLIPVERTEXLEFT)
                        {
                            vpTmp = (RtWorldImportBuildVertex *)
                                rwSListGetNewEntry(leftboundaries,
                                    rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
                            *vpTmp = *vpCurr;
                            leftcount++;
                        }
                        if (clip & rwCLIPVERTEXRIGHT)
                        {
                            vpTmp = (RtWorldImportBuildVertex *)
                                rwSListGetNewEntry(rightboundaries,
                                    rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
                            *vpTmp = *vpCurr;
                            rightcount++;
                        }
                    }
                }
                break;

        }

        vpFirst = vpCurr + 1;
        clip = rwCLIPVERTEXLEFT | rwCLIPVERTEXRIGHT;
        distLeft = distRight = 0.0f;
    }



    /*
     * We have now classified all the polygons as left or right, and we can finish
     * by building the sectors per se...
     */

    /*
     * new left build sector...
     */
    newleft = _rtImportBuildSectorCreate();
    newleft->vertices =
        ImportRemapVertices(leftboundaries, &newleft->numVertices,
                            UserDataCallBacks);
    newleft->numBoundaries = _rwSListGetNumEntriesMacro(leftboundaries);
    newleft->boundaries =
        (RtWorldImportBuildVertex *) rwSListToArray(leftboundaries);
    newleft->numPolygons = leftcount;
    newleft->boundingBox = buildSector->boundingBox;
    SETCOORD(newleft->boundingBox.sup, partition->type, partition->value);

    /* Now we have classified the polygons, we can (hopefully) shrink the overlap
     * to tightly fit the extreme polygons
     */
    _rtImportBuildSectorFindBBox(newleft, &bbox);
    if (GETCOORD(bbox.sup, partition->type) >
        GETCOORD(newleft->boundingBox.sup, partition->type))
    {
        newleft->overlap = (GETCOORD(bbox.sup, partition->type) -
                            GETCOORD(newleft->boundingBox.sup, partition->type));
        SETCOORD(newleft->boundingBox.sup, partition->type,
                 GETCOORD(bbox.sup, partition->type));
    }
    else
    {
        newleft->overlap = 0.0f;
    }
    /* should be recalculated, for now copy */
    newleft->maxNumMaterials = buildSector->maxNumMaterials;
    *buildSectorOutLeft = newleft;




    /*
     * new right build sector...
     */
    newright = _rtImportBuildSectorCreate();
    newright->vertices =
        ImportRemapVertices(rightboundaries, &newright->numVertices,
                            UserDataCallBacks);
    newright->numBoundaries =
        _rwSListGetNumEntriesMacro(rightboundaries);
    newright->boundaries =
        (RtWorldImportBuildVertex *) rwSListToArray(rightboundaries);

    newright->numPolygons = rightcount;
    newright->boundingBox = buildSector->boundingBox;
    SETCOORD(newright->boundingBox.inf, partition->type, partition->value);

    /* Now we have classified the polygons, we can (hopefully) shrink the overlap
     * to tightly fit the extreme polygons
     */
    _rtImportBuildSectorFindBBox(newright, &bbox);
    if (GETCOORD(bbox.inf, partition->type) <
        GETCOORD(newright->boundingBox.inf, partition->type))
    {
        newright->overlap =
            ( GETCOORD(newright->boundingBox.inf, partition->type) -
              GETCOORD(bbox.inf, partition->type) );
        SETCOORD(newright->boundingBox.inf, partition->type,
                 GETCOORD(bbox.inf, partition->type));
    }
    else
    {
        newright->overlap = 0.0f;
    }
    /* should be recalculated, for now copy */
    newright->maxNumMaterials = buildSector->maxNumMaterials;
    *buildSectorOutRight = newright;

    /* Increase poly in world count */
    _rtWorldImportTotalPolysInWorld +=
        (leftcount + rightcount) - buildSector->numPolygons;




    /*
     * clean up...
     */
    ImportDestroyCuts(flpLeftCuts, &leftcuts, UserDataCallBacks);
    ImportDestroyCuts(flpRightCuts, &rightcuts, UserDataCallBacks);
    RwFreeListDestroy(flpLeftCuts);
    RwFreeListDestroy(flpRightCuts);



    /* Now we have used left and right values (maximal), copy in the new (optimal) values...
     * NB: This is an optional thing we're doing, since the part is now stored in the bsp
     * and the partition here is not needed anymore, but for the sake of sanity, it's a good
     * idea to update it, it help debuggging, and the values should be assumed to have been
     * updated
     */
    partition->maxLeftValue = GETCOORD(newright->boundingBox.inf, partition->type);
    partition->maxRightValue = GETCOORD(newleft->boundingBox.sup, partition->type);
    RWRETURN(TRUE);
}





static RtWorldImportBuildPlaneSector *
ImportBuildSectorSplitRecurse(RtWorldImportBuildSector * buildSector,
                              RwInt32 currentBSPDepth,
                              RwSList * cutVertexList,
                              RwBool * ExitRequested,
                              RtWorldImportUserdataCallBacks *
                              UserDataCallBacks)
{
    RtWorldImportPartition partition;
    RtWorldImportBuildPlaneSector *buildPlaneSector;
    RtWorldImportBuildSector *newLeftBuildSector;
    RtWorldImportBuildSector *newRightBuildSector;
    RtWorldImportBuildStatus buildStatus;
    RtWorldImportParameters *conversionParams = RtWorldImportParametersGet();
    RwBool terminate;
    RwBool terminateDefault = TRUE;
    RwBool ignoreHints = TRUE;

    RWFUNCTION(RWSTRING("ImportBuildSectorSplitRecurse"));
    RWASSERT(buildSector);


    if (*ExitRequested)
    {
        RWRETURN((RtWorldImportBuildPlaneSector *) NULL);
    }

    /* Keep track of the maximum BSP depth */
    if (currentBSPDepth > rpWORLDMAXBSPDEPTH)
    {
        RWERROR((E_RW_MAXBSPDEPTHEXCEEDED));
        RWRETURN((RtWorldImportBuildPlaneSector *) NULL);
    }
    buildStatus.depth = currentBSPDepth;


    /* CHECK FOR LEAF NODE... */
    /* Have we reached our acceptance criteria for the sectors yet? */
    /* We either have requested termination, or the selector failed and we
     * must force termination...
     */

    /* Have we reached the user termination criteria yet? */
    terminate = (BuildCallBacks.terminationBuild(buildSector, &buildStatus,
                                       (void*)BuildCallBacks.terminationUserData));

    /* Just make sure we are not going to exceed the maximum tree depth with our
       own terminator... */
    if (!terminate) terminate = (buildStatus.depth >= rpWORLDMAXBSPDEPTH);

    if(conversionParams->terminatorCheck)
    {
        /* is the default terminator is ok with terminating */
        terminateDefault = RtWorldImportDefaultPartitionTerminator(buildSector, &buildStatus,
                                       (void *)conversionParams);
    }


    /* If we should not terminate, or the (overriding) default says no, call the selector */
    if(((!terminate) || (!terminateDefault)) ||
        ((buildSector->numPolygons > 65535) || (buildSector->numVertices > 65535)))
    {
        terminate = (BuildCallBacks.partitionBuild(buildSector, &buildStatus,
                        &partition, BuildCallBacks.partitionUserData)==rtWORLDIMPORTINVALIDPARTITION);


        if(((terminate) && (!terminateDefault)) || /* No partition but we mustn't terminate */
            ((buildSector->numPolygons > 65535) || (buildSector->numVertices > 65535)))
        {
            /* Pick one that is guaranteed to success */
            terminate = (RtWorldImportMaximumExtentPartitionSelector(buildSector, &buildStatus,
                        &partition, (void*)&ignoreHints)==rtWORLDIMPORTINVALIDPARTITION);

            RWASSERT(!terminate);
        }
    }



    if (terminate)
    {

        /* Got a good sector */
        _rtWorldImportNumPolysInLeaves += buildSector->numPolygons;
        _rtImportWorldSendProgressMessage
            (rtWORLDIMPORTPROGRESSBSPBUILDUPDATE,
             ((RwReal) _rtWorldImportNumPolysInLeaves /
              (RwReal) _rtWorldImportTotalPolysInWorld) * 100);

        RWRETURN((RtWorldImportBuildPlaneSector *) buildSector);
    }


    /* PROCESS THE PARTITION (Split the build sector in two) */

    if (!ImportBuildSectorSplit(buildSector, &partition,
                                &newLeftBuildSector,
                                &newRightBuildSector,
                                cutVertexList,
                                UserDataCallBacks))
    {
        /* Ooops */
        RWRETURN((RtWorldImportBuildPlaneSector *) NULL);
    }

    /* Create a new plane sector */
    buildPlaneSector = _rtImportBuildPlaneSectorCreate(partition.type, partition.value);
    if (!buildPlaneSector)
    {
        /* Ooops */
        _rtImportBuildSectorDestroy(newLeftBuildSector,
                                    UserDataCallBacks);
        _rtImportBuildSectorDestroy(newRightBuildSector,
                                    UserDataCallBacks);
        RWRETURN((RtWorldImportBuildPlaneSector *) NULL);
    }
    buildPlaneSector->planeSector.leftValue =
        buildPlaneSector->planeSector.value +
        newLeftBuildSector->overlap;
    buildPlaneSector->planeSector.rightValue =
        buildPlaneSector->planeSector.value -
        newRightBuildSector->overlap;

    /* Recurse left */
    buildPlaneSector->planeSector.leftSubTree =
        (RpSector *) ImportBuildSectorSplitRecurse(newLeftBuildSector,
                                                   currentBSPDepth + 1,
                                                   cutVertexList,
                                                   ExitRequested,
                                                   UserDataCallBacks);
    if (buildPlaneSector->planeSector.leftSubTree == NULL)
    {
        /* An error has occurred -> destroy it */
        _rtImportBuildSectorDestroy(newLeftBuildSector,
                                    UserDataCallBacks);
        _rtImportBuildSectorDestroy(newRightBuildSector,
                                    UserDataCallBacks);
        _rtImportPlaneSectorDestroy(buildPlaneSector);

        RWRETURN((RtWorldImportBuildPlaneSector *) NULL);
    }

    /* Recurse right */
    buildPlaneSector->planeSector.rightSubTree =
        (RpSector *) ImportBuildSectorSplitRecurse(newRightBuildSector,
                                                   currentBSPDepth + 1,
                                                   cutVertexList,
                                                   ExitRequested,
                                                   UserDataCallBacks);
    if (buildPlaneSector->planeSector.rightSubTree == NULL)
    {
        /* An error has occurred -> destroy it */

        RtWorldImportBuildPlaneSector *leftSubTree =
            (RtWorldImportBuildPlaneSector *)
            buildPlaneSector->planeSector.leftSubTree;

        _rtImportPlaneSectorDestroyTree(leftSubTree, UserDataCallBacks);
        _rtImportBuildSectorDestroy(newRightBuildSector,
                                    UserDataCallBacks);
        _rtImportPlaneSectorDestroy(buildPlaneSector);

        RWRETURN((RtWorldImportBuildPlaneSector *) NULL);
    }

    /* only now destroy the original */
    _rtImportBuildSectorDestroy(buildSector, UserDataCallBacks);

    /* Return the sector */
    RWRETURN(buildPlaneSector);
}


static RpSector    *
ImportWorldFixupTJunctions(RpSector * rootSector,
                           RwSList * cutVertexList,
                           RwInt32 currentDepth,
                           RwBool * resplit,
                           RwBool * ExitRequested,
                           RtWorldImportUserdataCallBacks *
                           UserDataCallBacks)
{
    RtWorldImportBuildStatus buildStatus;
    RtWorldImportParameters *conversionParams = RtWorldImportParametersGet();
    RwBool terminate;
    RwBool terminateDefault;

    RWFUNCTION(RWSTRING("ImportWorldFixupTJunctions"));

    if (rootSector->type >= 0)
    {
        RpPlaneSector      *planeSector = (RpPlaneSector *) rootSector;

        planeSector->leftSubTree =
            ImportWorldFixupTJunctions(planeSector->leftSubTree,
                                       cutVertexList, currentDepth+1,
                                       resplit, ExitRequested,
                                       UserDataCallBacks);
        planeSector->rightSubTree =
            ImportWorldFixupTJunctions(planeSector->rightSubTree,
                                       cutVertexList, currentDepth+1,
                                       resplit, ExitRequested,
                                       UserDataCallBacks);
    }
    else
    {
        RwInt32             cut;
        RtWorldImportBuildSector *buildSector =
            (RtWorldImportBuildSector *) rootSector;

        for (cut = 0; cut < _rwSListGetNumEntriesMacro(cutVertexList);
             cut++)
        {

            _rwCutVertex       *cutVert = (_rwCutVertex *)
                rwSListGetEntry(cutVertexList, cut);

            /* Only call this is cutVert is not disjoint from buildSector... */
            if (
                (cutVert->vpVert.OC.x <= buildSector->boundingBox.sup.x + SPLIT_EPSILON) &&
                (cutVert->vpVert.OC.x >= buildSector->boundingBox.inf.x - SPLIT_EPSILON) &&
                (cutVert->vpVert.OC.z <= buildSector->boundingBox.sup.z + SPLIT_EPSILON) &&
                (cutVert->vpVert.OC.z >= buildSector->boundingBox.inf.z - SPLIT_EPSILON) &&
                (cutVert->vpVert.OC.y <= buildSector->boundingBox.sup.y + SPLIT_EPSILON) &&
                (cutVert->vpVert.OC.y >= buildSector->boundingBox.inf.y - SPLIT_EPSILON))
            {
                ImportCutVertFixupTJunctions(cutVert,
                                         buildSector,
                                         UserDataCallBacks);
            }
        }

/* ref:aj */
        buildStatus.depth = currentDepth;

        /* Have we reached the user termination criteria yet? */
        terminate = TRUE;
/*
  terminate = (BuildCallBacks.terminationBuild(buildSector, &buildStatus,
                                       (void*)BuildCallBacks.terminationUserData));
*/
        /* lets see if the default terminator is ok with terminating */
        if(conversionParams->terminatorCheck)
        {
            terminateDefault = RtWorldImportDefaultPartitionTerminator(buildSector, &buildStatus,
                                           (void *)conversionParams);
        }
        else
        {
            terminateDefault = TRUE; /* exit when the user terminator says so */
        }

        terminate = (terminate && terminateDefault) ? TRUE : FALSE;

        if (!terminate)
        {
            rootSector = (RpSector *)
                    ImportBuildSectorSplitRecurse(buildSector,
                                              currentDepth,
                                              cutVertexList,
                                              ExitRequested, UserDataCallBacks);
            *resplit = TRUE;
        }
    }

    RWRETURN(rootSector);
}

/****************************************************************************
 _rtImportBuildSectorCreateFromNoHSWorld

 On entry   : NoHSWorld
 On exit    : Build sector
 */

RtWorldImportBuildSector *
_rtImportBuildSectorCreateFromNoHSWorld(RtWorldImport * wpNoHS,
                                        RpMaterialList * matList,
                                        RtWorldImportUserdataCallBacks *
                                        UserDataCallBacks)
{
    RtWorldImportBuildSector *buildSector;
    RtWorldImportBuildVertex *boundaries;
    RwInt32             nI, nJ;
    RtWorldImportParameters *conversionParams = RtWorldImportParametersGet();

    RWFUNCTION(RWSTRING("_rtImportBuildSectorCreateFromNoHSWorld"));

    RWASSERT(wpNoHS);

    buildSector = _rtImportBuildSectorCreate();
    if (!buildSector)
    {
        RWRETURN((RtWorldImportBuildSector *) NULL);
    }

    /* copy over the vertices */
    buildSector->numVertices = wpNoHS->numVertices;
    if (wpNoHS->numVertices)
    {
        buildSector->vertices = (RtWorldImportVertex *)
            RwMalloc(sizeof(RtWorldImportVertex) *
                     buildSector->numVertices,
                     rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
        if (!buildSector->vertices)
        {
            RWERROR((E_RW_NOMEM,
                     sizeof(RtWorldImportVertex) *
                     buildSector->numVertices));
            _rtImportBuildSectorDestroy(buildSector, UserDataCallBacks);
            RWRETURN((RtWorldImportBuildSector *) NULL);
        }
        memcpy(buildSector->vertices, wpNoHS->vertices,
               sizeof(RtWorldImportVertex) * buildSector->numVertices);
        /* does not have preliting */

        /* Clear any texture coordinates that aren't being used */
        if (conversionParams->numTexCoordSets < rwMAXTEXTURECOORDS)
        {
            for (nI = 0; nI < buildSector->numVertices; nI++)
            {
                for (nJ = conversionParams->numTexCoordSets; nJ < rwMAXTEXTURECOORDS; nJ++)
                {
                    buildSector->vertices[nI].texCoords[nJ].u = 0.0f;
                    buildSector->vertices[nI].texCoords[nJ].v = 0.0f;
                }
            }
        }

        if ((conversionParams->flags & rpWORLDPRELIT) == 0)
        {
            for (nI = 0; nI < buildSector->numVertices; nI++)
            {
                buildSector->vertices[nI].preLitCol.red = 0;
                buildSector->vertices[nI].preLitCol.green = 0;
                buildSector->vertices[nI].preLitCol.blue = 0;
                buildSector->vertices[nI].preLitCol.alpha = 0;
            }
        }

        /* initialise material indices */
        for (nI = 0; nI < buildSector->numVertices; nI++)
        {
            buildSector->vertices[nI].matIndex = 0xFFFF;
        }
    }

    /* build polygon boundaries */
    buildSector->numPolygons = wpNoHS->numPolygons;
    buildSector->numBoundaries = wpNoHS->numPolygons * (3 + 1);
    if (wpNoHS->numPolygons)
    {
        buildSector->boundaries = (RtWorldImportBuildVertex *)
            RwMalloc(sizeof(RtWorldImportBuildVertex) *
                     buildSector->numBoundaries,
                     rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
        if (!buildSector->boundaries)
        {
            RWERROR((E_RW_NOMEM,
                     sizeof(RtWorldImportBuildVertex) *
                     buildSector->numBoundaries));
            _rtImportBuildSectorDestroy(buildSector, UserDataCallBacks);
            RWRETURN((RtWorldImportBuildSector *) NULL);
        }
        boundaries = buildSector->boundaries;

#if (0 && defined(RWVERBOSE))
        {
            RwInt32             matIndex =
                wpNoHS->matList.numMaterials - 1;
            for (nI = 0; nI < wpNoHS->numPolygons; nI++)
                wpNoHS->polygons[nI].matIndex = matIndex;
        }
#endif /* (0 && defined(RWVERBOSE)) */

        for (nI = 0; nI < wpNoHS->numPolygons; nI++)
        {
            /* From RtWorldImportTriangle */
            RwInt32             matIndex =
                wpNoHS->polygons[nI].matIndex;
            RpMaterial         *mat = rpMaterialListGetMaterial(matList,
                                                                matIndex);
            RwTexture          *tex = RpMaterialGetTexture(mat);
            RwChar             *tname = (tex ?
                                         RwTextureGetMaskName(tex) :
                                         (char *) NULL);

            for (nJ = 0; nJ < 3; nJ++)
            {
                RtWorldImportBuildVertexMode *const mode =
                    &boundaries->mode;
                const RwInt32       index =
                    wpNoHS->polygons[nI].vertIndex[nJ];

                mode->vpVert = &buildSector->vertices[index];
                boundaries++;
            }

            /* mark end of polygon boundary (and store polygon info) */
            boundaries->mode.vpVert = (RtWorldImportVertex *) NULL;
            boundaries->pinfo.matIndex =
                (RwInt16) wpNoHS->polygons[nI].matIndex;
            boundaries->pinfo.hasAlpha =
                (mat->color.alpha != 255) || (tex && tname && tname[0]);
            /* assign the userdata pointer to the boundaries and
             * remove it from the polygons */
            boundaries->pinfo.pUserdata =
                wpNoHS->polygons[nI].pUserdata;
            wpNoHS->polygons[nI].pUserdata = NULL;

            boundaries++;
        }
/*
        if (conversionParams->fixAndFilterGeometry ||
            conversionParams->decimateAndWeldGeometry ||
            conversionParams->specializeGeometry)
        {
            _rtImportBuildSectorConditionGeometry(buildSector,
                                                  matList);
        }
*/
        /* Calculate the bounding box */
        _rtImportBuildSectorFindBBox(buildSector,
                                     &buildSector->boundingBox);

        if (conversionParams->userSpecifiedBBox)
        {
            RwBBox             *const sectorBox =
                &buildSector->boundingBox;
            const RwBBox       *const userBox =
                &conversionParams->userBBox;

            if (sectorBox->sup.x < userBox->sup.x)
            {
                sectorBox->sup.x = userBox->sup.x;
            }
            if (sectorBox->sup.y < userBox->sup.y)
            {
                sectorBox->sup.y = userBox->sup.y;
            }
            if (sectorBox->sup.z < userBox->sup.z)
            {
                sectorBox->sup.z = userBox->sup.z;
            }
            if (sectorBox->inf.x > userBox->inf.x)
            {
                sectorBox->inf.x = userBox->inf.x;
            }
            if (sectorBox->inf.y > userBox->inf.y)
            {
                sectorBox->inf.y = userBox->inf.y;
            }
            if (sectorBox->inf.z > userBox->inf.z)
            {
                sectorBox->inf.z = userBox->inf.z;
            }
        }
    }
    else if (conversionParams->userSpecifiedBBox)
    {
        buildSector->boundingBox = conversionParams->userBBox;
    }

    /* setup the max number of materials */
    buildSector->maxNumMaterials = rpMaterialListGetNumMaterials(&wpNoHS->matList);

    /* All done */
    RWRETURN(buildSector);
}

RpWorld            *
_rtImportWorldCreateWorld(RtWorldImport * nohsworld,
                          RwBool * ExitRequested,
                          RtWorldImportUserdataCallBacks
                          * UserDataCallBacks)
{
    RtWorldImportBuildSector *buildSector;
    RpSector           *rootSector, *rootWorldSector;
    RpWorld            *world;
    RwInt32             nI;
    RwBool              newSplits = TRUE;
    RwSList            *cutVertexList = (RwSList *) NULL;
    RtWorldImportParameters *conversionParams = RtWorldImportParametersGet();

    RWFUNCTION(RWSTRING("_rtImportWorldCreateWorld"));
    RWASSERT(nohsworld);
    RWASSERT(conversionParams);

    /* reset ExitRequested to FALSE */
    *ExitRequested = FALSE;

    /* verify RtWorldImport */
    for (nI = 0; nI < nohsworld->numPolygons; nI++)
    {
        RwInt32             nK;
        const RwInt32       matIndex = nohsworld->polygons[nI].matIndex;
        RwInt32            *const vertIndex =
            nohsworld->polygons[nI].vertIndex;
        if (matIndex < 0)
        {
           RWRETURN((RpWorld *) NULL);
        }
        if (matIndex >=
            rpMaterialListGetNumMaterials(&nohsworld->matList))
        {
            RWRETURN((RpWorld *) NULL);
        }

        for (nK = 0; nK < 3; nK++)
        {
            if (vertIndex[nK] < 0)
            {
                RWRETURN((RpWorld *) NULL);
            }
            if (vertIndex[nK] >= nohsworld->numVertices)
            {
                RWRETURN((RpWorld *) NULL);
            }
        }
    }

    /* Create a sector ready for some partitioning */
    buildSector =
        _rtImportBuildSectorCreateFromNoHSWorld(nohsworld,
                                                &nohsworld->matList,
                                                UserDataCallBacks
                                                );

    if (!buildSector)
    {
        RWRETURN((RpWorld *) NULL);
    }

    /*
     * * create a world to store the tree
     * * (trash default single sector at root)
     */
    world = RpWorldCreate(&buildSector->boundingBox);

    /* Reset the progress counts */
    _rtWorldImportTotalPolysInWorld = 0;
    _rtWorldImportNumPolysInLeaves = 0;
    _rtWorldImportNumPolysInCompressedLeaves = 0;
    cutVertexList = rwSListCreate(sizeof(_rwCutVertex),
        rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_FUNCTION);

    /* Start splitting up the world */
    _rtWorldImportTotalPolysInWorld = buildSector->numPolygons;
    _rtImportWorldSendProgressMessage
        (rtWORLDIMPORTPROGRESSBSPBUILDSTART, 0.0f);

    rootSector = (RpSector *)
            ImportBuildSectorSplitRecurse(buildSector,
                                      1,
                                      cutVertexList,
                                      ExitRequested, UserDataCallBacks);

    /* clear up the tree building scheme cache */
    _rtWorldImportBuildSchemeCacheCleanUp();

    if (!rootSector)
    {

        /* Hasn't been able to split the world */
        RpWorldDestroy(world);
        _rtImportBuildSectorDestroy(buildSector, UserDataCallBacks);
        RWRETURN((RpWorld *) NULL);
    }

    /* check on T-Junctions created and fix 'em up */
    if (conversionParams->fixTJunctions)
    {

        while (newSplits)
        {
            newSplits = FALSE;
#ifdef RWDEBUG
            gAnalysis = 0;
#endif
            rootSector =
                ImportWorldFixupTJunctions(rootSector,
                                           cutVertexList,
                                           1,
                                           &newSplits,
                                           ExitRequested,
                                           UserDataCallBacks);
        }
    }

#ifdef RWDEBUG

    /* In case anyone is tracking SList entry allocations */
    while (_rwSListGetNumEntriesMacro(cutVertexList))
    {
        rwSListDestroyEntry(cutVertexList,
                            _rwSListGetNumEntriesMacro(cutVertexList) -
                            1);
    }
#endif
    /* RWDEBUG */
    rwSListDestroy(cutVertexList);
    _rtImportWorldSendProgressMessage
        (rtWORLDIMPORTPROGRESSBSPBUILDEND, 0.0f);

    /* Compress into the use format */
    _rtImportWorldSendProgressMessage
        (rtWORLDIMPORTPROGRESSBSPCOMPRESSSTART, 0.0f);
    RpWorldSetFlags(world, conversionParams->flags);
    world->numTexCoordSets = conversionParams->numTexCoordSets;

    /* we no longer perform any material duplication or use material
       windows etc, so simply copy over the materials */
    for (nI = 0; nI < rpMaterialListGetNumMaterials(&nohsworld->matList); nI++)
    {
        RpMaterial *mat = rpMaterialListGetMaterial(&nohsworld->matList, nI);
        rpMaterialListAppendMaterial(&world->matList, mat);
    }

    rootWorldSector =
        _rtImportWorldSectorCompressTree
        (rootSector, world, &nohsworld->matList, UserDataCallBacks);

    if (!rootWorldSector)
    {

        /* Hasn't been able to compress the world */
        RpWorldDestroy(world);
        _rtImportPlaneSectorDestroyTree
            ((RtWorldImportBuildPlaneSector *) rootSector,
             UserDataCallBacks);
        RWRETURN((RpWorld *) NULL);
    }
    _rtImportWorldSendProgressMessage
        (rtWORLDIMPORTPROGRESSBSPCOMPRESSEND, 0.0f);

    /* ok so kill default sector in world */
    rwPluginRegistryDeInitObject(&sectorTKList, world->rootSector);
    RwFree(world->rootSector);
    world->rootSector = rootWorldSector;

    /* Store the bounding box */
    rpWorldFindBBox(world, &world->boundingBox);
    /* Making sure the userBBox is respected */
    if (conversionParams->userSpecifiedBBox)
    {
        RwBBox             *const sectorBox =
            &world->boundingBox;
        const RwBBox       *const userBox =
            &conversionParams->userBBox;

        if (sectorBox->sup.x < userBox->sup.x)
        {
            sectorBox->sup.x = userBox->sup.x;
        }
        if (sectorBox->sup.y < userBox->sup.y)
        {
            sectorBox->sup.y = userBox->sup.y;
        }
        if (sectorBox->sup.z < userBox->sup.z)
        {
            sectorBox->sup.z = userBox->sup.z;
        }
        if (sectorBox->inf.x > userBox->inf.x)
        {
            sectorBox->inf.x = userBox->inf.x;
        }
        if (sectorBox->inf.y > userBox->inf.y)
        {
            sectorBox->inf.y = userBox->inf.y;
        }
        if (sectorBox->inf.z > userBox->inf.z)
        {
            sectorBox->inf.z = userBox->inf.z;
        }
    }




    /* Setup the 'loose' sector bounding boxes from the BSP planes */
    _rpWorldSetupSectorBoundingBoxes(world);

    /* Make it usable (create meshes, etc) */
    rpWorldUnlock(world);

    /* All done */
    RWRETURN(world);
}

