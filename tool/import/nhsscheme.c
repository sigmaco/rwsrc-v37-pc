#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <float.h>

#include "rwcore.h"
#include "rpworld.h"

#include "rpdbgerr.h"

#include "nhsstats.h"
#include "nhsutil.h"
#include "nhsworld.h"
#include "rtgcond.h"
//#include "nhswing.h"

#include "nhssplit.h"
#include "nhsscheme.h"
#include "rtimport.h"
#include "nullkd.h"


#if (defined(_WINDOWS))
#define RWCDECL __cdecl
#endif /* (defined(_WINDOWS)) */

#if (!defined(RWCDECL))
#define RWCDECL                /* No op */
#endif /* (!defined(RWCDECL)) */

#if (!defined(SPLIT_EPSILON))
#define SPLIT_EPSILON (((RwReal)1)/((RwReal)(1<<10)))
#endif /* (!defined(SPLIT_EPSILON)) */

#define PLANE_GUARD 0.001f

static RwInt32 childrenCount = 0;

/* In splitting the world we weight on how much [in units of vertices]
 * it costs to process higher level structures.
 * Tweak these to reflect approximate costs on platforms.
 * eg MESHCOST reflects how many vertices we could have processed in the
 * time it takes to begin a new Mesh.
 */

#define MESHCOST        100
#define SECTORCOST      400
#define EQ(a,b) (a - b < 0.0001 && a - b > -0.0001)

#define Int32Min2(a,b)    ((RwInt32)( ((a) <= (b)) ?  (a) : (b)))
#define Int32Max2(a,b)    ((RwInt32)( ((a) >= (b)) ?  (a) : (b)))

#define FuzzyWorldExtentMacro(_result, _extent, _maxExtent)     \
MACRO_START                                                     \
{                                                               \
    (_result) = (_extent) / (_maxExtent);                       \
    (_result) = ( ((_result) > 0.6f) ?                          \
                  (1.0f):                                       \
                  /* bias against cutting small extent */       \
                  ( 0.5f + (_result / 0.6f) * (1.0f-0.6f)) );   \
}                                                               \
MACRO_STOP

/********************************************************************************
 * CACHE FUNCTION
 */

/* Cache data */
static RtWorldImportHints *gMaterialBoundaryHints = (RtWorldImportHints*)NULL;
static _rtWorldImportGuideKDStack KDStack = {NULL, NULL, NULL};


/* destroy cache data */
void
_rtWorldImportBuildSchemeCacheCleanUp(void)
{
    RWFUNCTION(RWSTRING("_rtWorldImportBuildSchemeCacheCleanUp"));

    if(gMaterialBoundaryHints)
    {
        RtWorldImportHintsDestroy(gMaterialBoundaryHints);
        gMaterialBoundaryHints = (RtWorldImportHints*)NULL;
    }
    if (BuildCallBacks.partitionBuild == _rtWorldImportGuidedKDPartitionSelector)
    {
        /* clean up the contents of KD stack */
        _rtWorldImportGuideKDStackDestroy(&KDStack);
    }
    RWRETURNVOID();
}

/********************************************************************************
 * STATIC TOOLS AND UTILITIES
 */
static RwReal
AreaOfTriangle(RwV3d * v1, RwV3d * v2, RwV3d * v3)
{
    RwV3d av1, av2, av3;
    RwReal a,b,c,s;
    RwReal x;

    RWFUNCTION(RWSTRING("AreaOfTriangle"));

    RwV3dSub(&av1, v1, v2);
    RwV3dSub(&av2, v2, v3);
    RwV3dSub(&av3, v3, v1);
    a = RwV3dLength(&av1);
    b = RwV3dLength(&av2);
    c = RwV3dLength(&av3);
    s = (a + b + c) / ((RwReal)2);

    x = (s)*(s-a)*(s-b)*(s-c);
    if (x > 0)
    {
#if (defined(RW_USE_SPF))
        RWRETURN((RwReal)(sqrtf(x)));
#else /* (defined(RW_USE_SPF)) */
        RWRETURN((RwReal)(sqrt(x)));
#endif /* (defined(RW_USE_SPF)) */
    }
    else
    {
        RWRETURN((RwReal)0.0);
    }
}


typedef struct _rwSortVertex _rwSortVertex;
struct _rwSortVertex
{
    RtWorldImportVertex *vpVert;
    RwReal              dist;
};


static int          RWCDECL
ImportSortVertexCmp(const void *a, const void *b)
{
    const _rwSortVertex *sortVertA = (const _rwSortVertex *) a;
    const _rwSortVertex *sortVertB = (const _rwSortVertex *) b;

    RWFUNCTION(RWSTRING("ImportSortVertexCmp"));
    RWASSERT(sortVertA);
    RWASSERT(sortVertB);

    if (sortVertA->dist < sortVertB->dist)
    {
        RWRETURN(-1);
    }
    else if (sortVertA->dist > sortVertB->dist)
    {
        RWRETURN(1);
    }
    else
    {
        RWRETURN(0);
    }
}

/* increment value by a small amount that is will guarentee it to change */
static void
AddEpsilon(RwReal *value)
{
    RwSplitBits base;

    RWFUNCTION(RWSTRING("AddEpsilon"));

    if((*value > 1.0f) || (*value < -1.0f))
    {
        base.nReal = *value;
        base.nInt -= 0x0C000000;
        base.nInt &= 0x7F800000;
    }
    else
    {
        base.nReal = 0.0011f;
    }

   /* we have to make sure that at least 0.001f is added, because this value
        is hardcoded into the CLIP STATS
        This need fixing probably */

    if(base.nReal <= 0.001f)
    {
        base.nReal += 0.001f;
    }

    *value += base.nReal;

    RWRETURNVOID();
}

/* decrement value by a small amount that is will guarentee it to change */
static void
SubtractEpsilon(RwReal *value)
{
    RwSplitBits base;

    RWFUNCTION(RWSTRING("SubtractEpsilon"));

    if((*value > 1.0f) || (*value < -1.0f))
    {
        base.nReal = *value;
        base.nInt -= 0x0C000000;
        base.nInt &= 0x7F800000;
    }
    else
    {
        base.nReal = 0.0011f;
    }

    /* we have to make sure that at least 0.001f is subtracted, because this value
        is hardcoded into the CLIP STATS
        This need fixing probably */
    if(base.nReal <= 0.001f)
    {
        base.nReal += 0.001f;
    }

    *value -= base.nReal;

    RWRETURNVOID();
}

/********************************************************************************
 * PARTITION TERMINATORS...
 */

/**
 * \ingroup terminators
 * \ref RtWorldImportNeverPartitionTerminator never returns TRUE. This is
 * useful if the Partition Selector should decide when to terminate.
 * It is therefore essential that the Partition Selector returns with an invalid
 * partition at some point - to avoid the tree reaching its maximum size and
 * capacity.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param userData     NULL.
 *
 * \return Returns TRUE if successful, FALSE otherwise.
 */
RwBool
RtWorldImportNeverPartitionTerminator(RtWorldImportBuildSector * buildSector __RWUNUSED__,
                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                    void * userData __RWUNUSED__)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportNeverPartitionTerminator"));
    RWRETURN(FALSE);
}

/**
 * \ingroup terminators
 * \ref RtWorldImportMaximumLegalPartitionTerminator returns TRUE as soon
 * as a sector is legal, i.e. when the number of vertices imposed by the
 * \ref RwUInt16 index in polygons is met
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param userData     NULL.
 *
 * \return Returns TRUE if successful, FALSE otherwise
 */
RwBool
RtWorldImportMaximumLegalPartitionTerminator(RtWorldImportBuildSector * buildSector,
                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                    void * userData __RWUNUSED__)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportMaximumLegalPartitionTerminator"));

    /* 65536 vertices imposed by the RwUInt16 index in polygons */
    if (buildSector->numVertices > 65536)
    {
        RWRETURN(FALSE);
    }
    RWRETURN(TRUE);
}

/**
 * \ingroup terminators
 * \ref RtWorldImportDefaultPartitionTerminator is guaranteed to return FALSE
 * until all criteria are met. The criteria are
 * maximums, such as number of vertices, triangles and the size of the sector.
 * This terminator is ALWAYS called, even if a separate terminator is used
 * as well; this terminator overrides any TRUE result if the criteria have not
 * been met.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param userData     NULL.
 *
 * \return TRUE if all criteria have been met, FALSE otherwise.
 */
RwBool
RtWorldImportDefaultPartitionTerminator(RtWorldImportBuildSector * buildSector,
                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                    void * userData __RWUNUSED__)
{
    RtWorldImportParameters *conversionParams = RtWorldImportParametersGet();
    RwV3d               vSize;

    RWAPIFUNCTION(RWSTRING("RtWorldImportDefaultPartitionTerminator"));

    /* 65536 vertices imposed by the RwUInt16 index in polygons */
    if (buildSector->numVertices > 65536)
    {
        RWRETURN(FALSE);
    }

    /* In the ballpark for max triangles imposed
     * by the conversion parameters */
    if (buildSector->numPolygons >
        conversionParams->maxWorldSectorPolygons)
    {
        RWRETURN(FALSE);
    }

    /* Ok, so do an accurate test of number of triangles (after fanning) */
    if (_rtImportBuildSectorFindNumTriangles(buildSector) >
        conversionParams->maxWorldSectorPolygons)
    {
        RWRETURN(FALSE);
    }

    /* Worldspace size imposed by the conversion parameters */
    RwV3dSub(&vSize, &buildSector->boundingBox.sup,
             &buildSector->boundingBox.inf);
    if (vSize.x > conversionParams->worldSectorMaxSize)
    {
        RWRETURN(FALSE);
    }
    if (vSize.y > conversionParams->worldSectorMaxSize)
    {
        RWRETURN(FALSE);
    }
    if (vSize.z > conversionParams->worldSectorMaxSize)
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}

RwBool
_rtWorldImportGuidedKDStackPartitionTerminator(RtWorldImportBuildSector * buildSector __RWUNUSED__,
                                    RtWorldImportBuildStatus * buildStatus,
                                    void * userData)
{
    _rtWorldImportGuideKDStack *stack;
    RtWorldImportParameters *conversionParams = RtWorldImportParametersGet();

    RWFUNCTION(RWSTRING("_rtWorldImportGuidedKDStackPartitionTerminator"));

    stack = (_rtWorldImportGuideKDStack*)userData;

    if (buildStatus->depth == 1) childrenCount = 0;
    if(childrenCount > 0)
    {
        if(RtWorldImportDefaultPartitionTerminator(buildSector, NULL, NULL))
        {
            childrenCount--;

            if(childrenCount <= 0) /* the last child pop */
            {
                /* Pop */
                _rtWorldImportGuideKDPop(stack);
                childrenCount = 0;
                conversionParams->terminatorCheck = FALSE;
            }

            RWRETURN(TRUE);
        }
    }
    else if(buildStatus->depth >= rpWORLDMAXBSPDEPTH)
    {
        /* this could produce an invalid world if this sector
            does not conform to the global parameters */
        _rtWorldImportGuideKDPop(stack);
        RWRETURN(TRUE);
    }

    RWRETURN(FALSE);
}

/**
 * \ingroup terminators
 * \ref RtWorldImportDepthPartitionTerminator returns TRUE when the tree has
 * reached a certain depth.
 *
 * \note This terminator will be overidden if the default terminator does not permit
 * termination.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param userData     A pointer to an \ref RwInt32 that is the
 *                     maximum depth.
 *
 * \return TRUE if the depth of the buildSector is greater than or equal to
 * the depth in userData.
 *
 * \see RtWorldImportDefaultPartitionTerminator
 */
RwBool
RtWorldImportDepthPartitionTerminator(RtWorldImportBuildSector * buildSector __RWUNUSED__,
                    RtWorldImportBuildStatus * buildStatus,
                    void * userData)
{
    RwInt32 depth = *((RwInt32*)userData);

    RWAPIFUNCTION(RWSTRING("RtWorldImportDepthPartitionTerminator"));

    RWRETURN(buildStatus->depth >= depth);
}

/**
 * \ingroup terminators
 * \ref RtWorldImportSizePartitionTerminator returns TRUE when the dimensions
 * of the sector fall within the value specified in the user data.
 *
 * \note This terminator will be overidden if the default terminator does not permit
 * termination.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param userData     A pointer to an \ref RwReal that is the size.
 *
 * \return TRUE if the size of the buildSector is less than or equal to
 * the size in userData.
 *
 * \see RtWorldImportDefaultPartitionTerminator
 */
RwBool
RtWorldImportSizePartitionTerminator(RtWorldImportBuildSector * buildSector,
                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                    void * userData)
{
    RwReal size = *((RwReal*)userData);
    RwV3d               vSize;

    RWAPIFUNCTION(RWSTRING("RtWorldImportSizePartitionTerminator"));

    RwV3dSub(&vSize, &buildSector->boundingBox.sup,
             &buildSector->boundingBox.inf);
    if (vSize.x > size)
    {
        RWRETURN(FALSE);
    }
    if (vSize.y > size)
    {
        RWRETURN(FALSE);
    }
    if (vSize.z > size)
    {
        RWRETURN(FALSE);
    }
    RWRETURN(TRUE);
}

/**
 * \ingroup terminators
 * \ref RtWorldImportSectorAspectSizePartitionTerminator is the terminator that returns
 * TRUE when the size of the sector is less than or equal to the values provided. Each
 * dimension is treated separately.
 * \note The default terminator may override this if it returns TRUE and the sector is
 * still too large by the default parameters.
 *
 * \note This terminator will be overidden if the default terminator does not permit
 * termination.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param userData     A pointer to an \ref RwV3d pointer that is the
 *                     maximum size of the x, y and z axes of the sector
 *                     respectively.
 *
 * \return TRUE if the size of each axis is less than or equal to
 * the values in userData.
 *
 * \see RtWorldImportDefaultPartitionTerminator
 */
RwBool
RtWorldImportSectorAspectSizePartitionTerminator(RtWorldImportBuildSector * buildSector,
                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                    void * userData)
{
    RwReal sizex = ((RwV3d*)userData)->x;
    RwReal sizey = ((RwV3d*)userData)->y;
    RwReal sizez = ((RwV3d*)userData)->z;

    RWAPIFUNCTION(RWSTRING("RtWorldImportSectorAspectSizePartitionTerminator"));

    RWRETURN(((buildSector->boundingBox.sup.x - buildSector->boundingBox.inf.x) < sizex) &&
        ((buildSector->boundingBox.sup.y - buildSector->boundingBox.inf.y) < sizey) &&
        ((buildSector->boundingBox.sup.z - buildSector->boundingBox.inf.z) < sizez));
}

/**
 * \ingroup terminators
 * \ref RtWorldImportSectorHeightPartitionTerminator is a hybrid terminator;
 * its base criterion is minimum sector size, but this is a function of the height
 * of the sector - sectors nearer the top of the world are terminated earlier than those
 * near the bottom. The sectors also have a minimum aspect ratio of 2:1:2.
 * This terminator is useful in outdoor scenes.
 *
 * \note This terminator will be overidden if the default terminator does not permit
 * termination.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param userData     A pointer to an array of five values that are: the floor
 * height, ceiling height, world size, minimum sector size, and the power - affecting
 * the change of size of sectors towards the bottom of the scene.
 *
 * \return TRUE if the size of the sector is less than or equal to that dictated
 * by the values in userData.
 *
 * \see RtWorldImportDefaultPartitionTerminator
 */
RwBool
RtWorldImportSectorHeightPartitionTerminator(RtWorldImportBuildSector * buildSector,
                    RtWorldImportBuildStatus * buildStatus,
                    void * userData)
{
    RwReal high = buildSector->boundingBox.sup.y;
    RwReal low = buildSector->boundingBox.inf.y;
    RwReal* dat = ((RwReal*)userData);
    RwReal floor = dat[0];
    RwReal ceil = dat[1];
    RwReal worldSize = dat[2];
    RwReal lim = dat[3];
    RwReal power = dat[4];
    RwV3d minSizes;
    RwReal minSize;
    RwReal f;

    RWAPIFUNCTION(RWSTRING("RtWorldImportSectorHeightPartitionTerminator"));

    high -= floor;
    low -= floor;
    ceil -= floor;

    f = ((RwReal)1) - ((ceil-low) / ceil);
    f = (RwReal)(RwPow(f, power));

    if (f < lim) f = lim;

    minSize = f * worldSize;

    minSizes.y = minSize;
    minSizes.x = minSize * 2.0f;
    minSizes.z = minSize * 2.0f;

    RWRETURN(RtWorldImportSectorAspectSizePartitionTerminator(buildSector,
                    buildStatus,
                    (void*)&minSizes));
}


/********************************************************************************
 * PARTITION EVALUATORS...
 */
/**
 * \ingroup evaluators
 * \ref RtWorldImportFuzzyBalancedPartitionEvaluator is a hybrid evaluator;
 * it returns a value based on balance, splits, and materials
 *
 * \note The partition statistics must be set before calling this function
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition requiring evaluation.
 * \param userData     NULL.
 *
 * \return The fuzzy value.
 *
 * \see RtWorldImportSetPartitionStatistics
 */
RwReal
RtWorldImportFuzzyBalancedPartitionEvaluator(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                                    RtWorldImportPartition *partition,
                                    void * userData __RWUNUSED__)
{
    RtWorldImportParameters *conversionParams = RtWorldImportParametersGet();
    RwReal      extentScore;
    RwInt32     maxExtent;
    RwReal      score = RwRealMAXVAL;

    RWAPIFUNCTION(RWSTRING("RtWorldImportFuzzyBalancedPartitionEvaluator"));

    RWASSERT(conversionParams->maxWorldSectorPolygons > 0);

#if 0
      RwReal balanceWeight = ((RwReal)1);        /* for weighting the evaluation... */
      RwReal AreaOfWorld = 100.0f;
      RwReal oweights[3] = {1.0f, 1.2f, 1.0f}; /* Balance weights for plane orientation */
#endif

   /* Check what would be resultant polygons with this clip plane
    * Including a look at overlaps and alpha distances */

    /* does it actually divide the geometry? */
    if (partition->buildStats.numMaterialLeft && partition->buildStats.numMaterialRight)
    {
        RwReal      leftscore;
        RwReal      rightscore;
        RwReal      clipCost;
        RwReal      diff;
        RwInt32     leftsectors, rightsectors;
        RwInt32     tryPlane;
        RwV3d       vSize;

        /* Find the maximum extent for this sector */
        RwV3dSub(&vSize, &buildSector->boundingBox.sup,
             &buildSector->boundingBox.inf);

        maxExtent = 0;
        for (tryPlane = 0; tryPlane < 12; tryPlane += sizeof(RwReal))
        {
            if (GETCOORD(vSize, tryPlane) > GETCOORD(vSize, maxExtent))
            {
               maxExtent = tryPlane;
            }
        }

        if(GETCOORD(buildSector->boundingBox.inf, partition->type) >
            (partition->value - PLANE_GUARD))
        {
            RWRETURN(RwRealMAXVAL);
        }

        if(GETCOORD(buildSector->boundingBox.sup, partition->type) <
            (partition->value + PLANE_GUARD))
        {
            RWRETURN(RwRealMAXVAL);
        }

        /* we use extent just as a tiebreak only */
        FuzzyWorldExtentMacro(extentScore,
                            GETCOORD(vSize, partition->type),
                            GETCOORD(vSize, maxExtent));
        RWASSERT(0.0 != extentScore);
        extentScore = (1.0f / extentScore);

        /* Removed check for alpha - so this is just set to 256 for fuzzy reasonsif there's alpha we need to really clip */
        clipCost = 256.0f;

        leftsectors =
            (partition->buildStats.numPotentialLeft +
             conversionParams->maxWorldSectorPolygons -
             1) / conversionParams->maxWorldSectorPolygons;
        leftscore =
            (RwReal) (partition->buildStats.numPotentialLeft) +
            (RwReal) (partition->buildStats.numPotentialSplit) * clipCost +
            (RwReal) (partition->buildStats.numMaterialLeft * MESHCOST) +
            (RwReal) (leftsectors * SECTORCOST);

        /* bigger overlaps means proportionally more work */
        leftscore +=
            leftscore * (partition->buildStats.overlapLeft /
                         GETCOORD(vSize, partition->type));

        rightsectors =
            (partition->buildStats.numPotentialRight +
             conversionParams->maxWorldSectorPolygons -
             1) / conversionParams->maxWorldSectorPolygons;
        rightscore =
            (RwReal) (partition->buildStats.numPotentialRight) +
            (RwReal) (partition->buildStats.numPotentialSplit) * clipCost +
            (RwReal) (partition->buildStats.numMaterialRight * MESHCOST) +
            (RwReal) (rightsectors * SECTORCOST);

        /* bigger overlaps means proportionally more work */
        rightscore +=
            rightscore * (partition->buildStats.overlapRight /
                          GETCOORD(vSize, partition->type));

        /* score is total work if we use this split */
        score = leftscore + rightscore;

        /* we implicitly bias to a balanced tree because
         * we test from the median outward;
         * however we'll bias a bit more
         */
        diff = leftscore - rightscore;
        score += ((diff < 0) ? -diff : diff);

        /* both leaf node biasing */
        if ((leftsectors == 1) && (rightsectors == 1))
        {
            score *= 0.5f;
        }

        /* extent biasing */
        score *= extentScore;
    }

    RWRETURN(score);
}

/**
 * \ingroup evaluators
 * \ref RtWorldImportPotentialSplitPartitionEvaluator returns the number
 * of polygons that are split by the partition, not considering overlapping
 * sectors.
 *
 * \note The partition statistics must be set before calling this function
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition requiring evaluation.
 * \param userData     NULL.
 *
 * \return The number of polygons that are split by the partition.
 *
 * \see RtWorldImportSetPartitionStatistics
 */
RwReal
RtWorldImportPotentialSplitPartitionEvaluator(RtWorldImportBuildSector * buildSector __RWUNUSED__,
                                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                                    RtWorldImportPartition *partition,
                                    void * userData __RWUNUSED__)
{
    /* Returns the number of triangles that are split by the partition */
    RWAPIFUNCTION(RWSTRING("RtWorldImportPotentialSplitPartitionEvaluator"));

    RWRETURN((RwReal)(partition->buildStats.numPotentialSplit));
}

/**
 * \ingroup evaluators
 * \ref RtWorldImportVolumeBalancedPartitionEvaluator returns the balance of
 * the partition in terms of space on either side, in the inclusive range 0..1,
 * where 0 is perfectly balanced, and 1 is totally unbalanced.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition requiring evaluation.
 * \param userData     NULL.
 *
 * \return The balance of the partition. [0..1].
 */
RwReal
RtWorldImportVolumeBalancedPartitionEvaluator(RtWorldImportBuildSector * buildSector __RWUNUSED__,
                                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                                    RtWorldImportPartition *partition,
                                    void * userData __RWUNUSED__)
{
    RwReal sup = GETCOORD(buildSector->boundingBox.sup, partition->type);
    RwReal inf = GETCOORD(buildSector->boundingBox.inf, partition->type);
    RwReal sideA = partition->value - inf;
    RwReal sideB = sup - partition->value;
    RwReal high = RwRealMax2(sideA, sideB);
    RwReal low = RwRealMin2(sideA, sideB);
    RwReal balanceCost;

    RWAPIFUNCTION(RWSTRING("RtWorldImportVolumeBalancedPartitionEvaluator"));

    balanceCost = ((RwReal)high / ((RwReal)low + (RwReal)high));
    balanceCost = (balanceCost - 0.5f) * 2.0f;

    /* range 0..1, 0 best */
    RWRETURN(balanceCost);
}

/**
 * \ingroup evaluators
 * \ref RtWorldImportBalancedPartitionEvaluator returns the balance of
 * the partition in terms of polygons on either side, in the inclusive range 0..1,
 * where 0 is perfectly balanced, and 1 is totally unbalanced.
 *
 * \note The partition statistics must be set before calling this function
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition requiring evaluation.
 * \param userData     NULL.
 *
 * \return The balance of the partition [0..1].
 */
RwReal
RtWorldImportBalancedPartitionEvaluator(RtWorldImportBuildSector * buildSector __RWUNUSED__,
                                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                                    RtWorldImportPartition *partition,
                                    void * userData __RWUNUSED__)
{
    RwInt32 high, low;
    RwReal balanceCost;

    RWAPIFUNCTION(RWSTRING("RtWorldImportBalancedPartitionEvaluator"));

    high = Int32Max2(partition->buildStats.numPotentialRight,
                      partition->buildStats.numPotentialLeft);
    low = Int32Min2(partition->buildStats.numPotentialRight,
                     partition->buildStats.numPotentialLeft);

    balanceCost = ((RwReal)high / ((RwReal)low + (RwReal)high));
    balanceCost = (balanceCost - 0.5f) * 2.0f;

    /* range 0..1, 0 best */
    RWRETURN(balanceCost);
}

/**
 * \ingroup evaluators
 * \ref RtWorldImportAspectPartitionEvaluator evaluates the uniformity of the partition
 * with respect to the sector it is subdividing. It returns a measure of the aspect
 * over the size of the sector in the dimension of the partition; where a high
 * value is poor and a low value good. Thus a partition that would slice an already
 * thin sector along its shortest dimension would result in a high value.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition requiring evaluation.
 * \param userData     NULL.
 *
 * \return The uniformity of the partition in the sector.
 */
RwReal
RtWorldImportAspectPartitionEvaluator(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                                    RtWorldImportPartition *partition,
                                    void * userData __RWUNUSED__)
{
    RwV3d vSize;
    RwReal ext;
    RwReal len;

    RWAPIFUNCTION(RWSTRING("RtWorldImportAspectPartitionEvaluator"));

    RwV3dSub(&vSize, &buildSector->boundingBox.sup,
                 &buildSector->boundingBox.inf);

    len = RwV3dLength(&vSize);
    ext = GETCOORD(vSize, partition->type);

    RWRETURN(len / ext);

}

/**
 * \ingroup evaluators
 * \ref RtWorldImportExtentPartitionEvaluator returns the size of the (negative)
 * size of the sector in the dimension of the partition.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition requiring evaluation.
 * \param userData     NULL.
 *
 * \return The size of the sector in the dimension of the partition.
 */
RwReal
RtWorldImportExtentPartitionEvaluator(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                                    RtWorldImportPartition *partition,
                                    void * userData __RWUNUSED__)
{
    RwV3d vSize;

    RWAPIFUNCTION(RWSTRING("RtWorldImportExtentPartitionEvaluator"));

    RwV3dSub(&vSize, &buildSector->boundingBox.sup,
                 &buildSector->boundingBox.inf);

    RWRETURN(-GETCOORD(vSize, partition->type));
}

static RwBool
IsCoplanar(RtWorldImportVertex** vpVert, RtWorldImportPartition* partition)
{
    RwReal val[3];

    RWFUNCTION(RWSTRING("IsCoplanar"));

    val[0] = GETCOORD((vpVert[0])->OC, partition->type);
    val[1] = GETCOORD((vpVert[1])->OC, partition->type);
    val[2] = GETCOORD((vpVert[2])->OC, partition->type);

    RWRETURN(EQ(partition->value, val[0]) &&
        EQ(val[0],val[1]) &&
        EQ(val[1],val[2]));
}

/**
 * \ingroup evaluators
 * \ref RtWorldImportOccluderPartitionEvaluator returns the area covered (of
 * coplanar polygons.)
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition requiring evaluation.
 * \param userData     NULL.
 *
 * \return The area covered.
 */
RwReal
RtWorldImportOccluderPartitionEvaluator(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                                    RtWorldImportPartition *partition,
                                    void * userData __RWUNUSED__)
{
    RwInt32 allboundaries;
    RtWorldImportVertex *vpVert[3];
    RwReal eval = 0;

    RWAPIFUNCTION(RWSTRING("RtWorldImportOccluderPartitionEvaluator"));

    /* Evaluates the occluder against all the polygons in the scene,
     * and rewards it for those that is 'covers'
     * thus it identifies large occluders. The value returns is the area covered
     */
    allboundaries = 0;
    while (allboundaries<buildSector->numBoundaries)
    {
        vpVert[0] = buildSector->boundaries[allboundaries++].mode.vpVert;
        vpVert[1] = buildSector->boundaries[allboundaries++].mode.vpVert;
        vpVert[2] = buildSector->boundaries[allboundaries++].mode.vpVert;

        if (IsCoplanar(vpVert, partition))
        {
            eval -=(AreaOfTriangle(&((vpVert[0])->OC),&((vpVert[1])->OC),&((vpVert[2])->OC)));

            while (buildSector->boundaries[allboundaries++].mode.vpVert)
            {
                /* Has more than three vertices. Add on the extra area missed. */
                /*vpVert[0];*/
                vpVert[1] = vpVert[2];
                vpVert[2] = buildSector->boundaries[allboundaries-1].mode.vpVert;

                eval -=(AreaOfTriangle(&((vpVert[0])->OC),&((vpVert[1])->OC),&((vpVert[2])->OC)));
            }
        }
        else
        {
            while (buildSector->boundaries[allboundaries++].mode.vpVert)
                ; /* find end */
        }
    }
    RWRETURN(eval);
}

/**
 * \ingroup evaluators
 * \ref RtWorldImportWeightedOccluderPartitionEvaluator returns the weighted value
 *  of the partition as an occluder - this is mapped to the range [-1..0] where -1
 * represents an occluder that completely covers the geometry, and 0
 * where is covers nothing. This requires the area of the sector in the dimension
 * of the partitioner, and this is passed as userData. If the partition is a peripheral
 * one, rtWORLDIMPORTINVALIDPARTITION is returned.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition requiring evaluation.
 * \param userData     A pointer to an \ref RwReal that is the area of the sector
 * in the dimension of the partitioner.
 *
 * \return The value of the partition as an occluder in the range [-1..0].
 */
RwReal
RtWorldImportWeightedOccluderPartitionEvaluator(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus *buildStatus,
                                    RtWorldImportPartition *partition,
                                    void * userData)
{
    /* Combo */
    RwReal AreaOfSector = *((RwReal*)userData);
    RwReal balFilter = 0.9f;            /* first make sure balance is okay */
    RwReal areaWeight = ((RwReal)1);           /* make occluding most significant */
    RwReal occVal = (RwReal)0;
    RwReal balVal = (RwReal)0;
    RwReal eval = (RwReal)0;


    RWAPIFUNCTION(RWSTRING("RtWorldImportWeightedOccluderPartitionEvaluator"));

    /* First, make sure we're not picking something silly, like slicing
     * a sector so finely it should be refered to as a graze
     */
    balVal = RtWorldImportVolumeBalancedPartitionEvaluator(buildSector, buildStatus,
                partition,
                userData);
    if (balVal>balFilter) RWRETURN(rtWORLDIMPORTINVALIDPARTITION);


    /* This, is the area coverage of the partition
     * E.g. _________part________
     *      --p1---       --p2--- = 67% * Area of the world
     */
    occVal = RtWorldImportOccluderPartitionEvaluator(buildSector, buildStatus,
                partition,
                userData);

    eval = ((occVal*areaWeight)/AreaOfSector);


    RWRETURN(eval);
}

/**
 * \ingroup evaluators
 * \ref RtWorldImportWeightedDisjointOccluderPartitionEvaluator returns the weighted value
 * of the partition as a disjoint occluder - i.e. in a scene that is not highly occluded, such
 * as an outdoor scene, it evaluates a partition by a number of criteria, including
 * actual occlusion, number of splits, balance, and volume balance, which reflects
 * the better values for use with PVS.
 * This requires the area of the sector in the dimension
 * of the partitioner, and this is passed as userData. If the partition is a peripheral
 * one, or if it occludes next to nothing, rtWORLDIMPORTINVALIDPARTITION is returned.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition requiring evaluation.
 * \param userData     A pointer to an \ref RwReal that is the area of the sector in
 * the dimension of the partitioner.
 *
 * \return The value of the partition as an occluder.
 */
RwReal
RtWorldImportWeightedDisjointOccluderPartitionEvaluator(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus *buildStatus,
                                    RtWorldImportPartition *partition,
                                    void * userData)
{
    RwReal balFilter = 0.75f;            /* first make sure balance is okay */
    RwReal aspFilter = 10.0f;             /* make sure aspect is okay */
    RwReal areaWeight = 2.0f;           /* make occluding most significant */
    RwReal extWeight = ((RwReal)1);            /* make extent less significant */
    RwReal AreaOfSector = *((RwReal*)userData);
    RwReal LengthOfWorld = _rtImportBuildSectorFindBBoxDiagonal(&buildSector->boundingBox);
    RwReal aspVal = (RwReal)0;
    RwReal occVal = (RwReal)0;
    RwReal balVal = (RwReal)0;
    RwReal extVal = 0.0;
    RwReal splVal = (RwReal)0;
    RwReal eval = (RwReal)0;
#if (0)
    RwReal hidVal = (RwReal)0;
#endif /* (0) */

    RWAPIFUNCTION(RWSTRING("RtWorldImportWeightedDisjointOccluderPartitionEvaluator"));

    balVal = RtWorldImportVolumeBalancedPartitionEvaluator(buildSector, buildStatus,
                partition,
                userData);
    /* Balance poor */
    if (balVal > balFilter) RWRETURN(rtWORLDIMPORTINVALIDPARTITION);

    aspVal = RtWorldImportAspectPartitionEvaluator(buildSector, buildStatus,
                partition,
                userData);
    /* Aspect poor */
    if (aspVal > aspFilter) RWRETURN(rtWORLDIMPORTINVALIDPARTITION);

    /* First, make sure we're not picking something silly, like slicing
     * a sector so finely it should be refered to as a graze
     */
    /* This, is the area coverage of the partition
     * E.g. _________part________
     *      --p1---       --p2--- = 67% * Area of the world
     */


    /* if the occluder covers less than xxx of the non-empty sector, don't bother,
     * (use the default partition, or terminate)
     */
    occVal = RtWorldImportOccluderPartitionEvaluator(buildSector, buildStatus,
                partition,
                userData);
    /* Occluder poor */
    if (occVal/AreaOfSector > 0.0)
        RWRETURN(rtWORLDIMPORTINVALIDPARTITION);


    /* This weights maximum extents by magnitude */
    extVal = RtWorldImportExtentPartitionEvaluator(buildSector, buildStatus,
                partition,
                userData);

    /* An occluder is not THAT good, if there are not many polygons on one side.
     * \note This should be a pseudoglobal weight, but we'll use a local one
     * since parental locals should have considered that...
     */
    RtWorldImportSetPartitionStatistics(buildSector, partition);

    /*hidVal = RtWorldImportBalancedPartitionEvaluator(buildSector, buildStatus,
                partition,
                userData);*/

    splVal = RtWorldImportPotentialSplitPartitionEvaluator(buildSector, buildStatus,
                partition,
                userData);

    /* occluder poor and not even a maximum which is good for rolling hills */
    /*
    if ((partition->type==4) && (occVal/AreaOfSector > -0.01) && (splVal > 0))
        RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
    */

    eval = ((occVal*areaWeight)/AreaOfSector) + ((extVal*extWeight)/LengthOfWorld)
                /*+ (hidVal*hidWeight)*/ + (splVal/buildSector->numPolygons);



    RWRETURN(eval);
}

/**
 * \ingroup evaluators
 * \ref RtWorldImportMaterialCutsPartitionEvaluator returns the number of
 * materials split by the partition.
 *
 * \note The partition statistics must be set before calling this function
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition requiring evaluation.
 * \param userData     NULL.
 *
 * \return The number of materials split by the partition.
 *
 * \see RtWorldImportSetPartitionStatistics
 */
RwReal
RtWorldImportMaterialCutsPartitionEvaluator(RtWorldImportBuildSector * buildSector __RWUNUSED__,
                                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                                    RtWorldImportPartition *partition,
                                    void * userData __RWUNUSED__)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportMaterialCutsPartitionEvaluator"));

    RWRETURN((RwReal)partition->buildStats.numMaterialSplits);
}

/**
 * \ingroup evaluators
 * \ref RtWorldImportMaterialBalancedPartitionEvaluator returns the (positive)
 * difference between the number of materials left and those on the right of the partition.
 *
 * \note The partition statistics must be set before calling this function
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition requiring evaluation.
 * \param userData     NULL.
 *
 * \return The difference between the number of materials to the left and to the right.
 *
 * \see RtWorldImportSetPartitionStatistics
 */
RwReal
RtWorldImportMaterialBalancedPartitionEvaluator(RtWorldImportBuildSector * buildSector __RWUNUSED__,
                                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                                    RtWorldImportPartition *partition,
                                    void * userData __RWUNUSED__)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportMaterialBalancedPartitionEvaluator"));

    if(partition->buildStats.numMaterialLeft > partition->buildStats.numMaterialRight)
    {
        RWRETURN((RwReal)(partition->buildStats.numMaterialLeft - partition->buildStats.numMaterialRight));
    }
    else
    {
        RWRETURN((RwReal)(partition->buildStats.numMaterialRight - partition->buildStats.numMaterialLeft));
    }
}

/**
 * \ingroup evaluators
 * \ref RtWorldImportMaterialSeparatorPartitionEvaluator evaluates the balance
 * of materials on either side of the candidate partition.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition requiring evaluation.
 * \param userData     NULL.
 *
 * \return The material balance.
 */
RwReal
RtWorldImportMaterialSeparatorPartitionEvaluator(RtWorldImportBuildSector * buildSector __RWUNUSED__,
                                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                                    RtWorldImportPartition *partition,
                                    void * userData __RWUNUSED__)
{
    RwReal leftScore, rightScore, balanceScore, separatorCost;

    RWAPIFUNCTION(RWSTRING("RtWorldImportMaterialSeparatorPartitionEvaluator"));

    leftScore = (RwReal)(partition->buildStats.numMaterialSector -
                 partition->buildStats.numMaterialLeft);

    rightScore = (RwReal)(partition->buildStats.numMaterialSector -
                  partition->buildStats.numMaterialRight);

    balanceScore = RwRealAbs(leftScore - rightScore);

    separatorCost = balanceScore - (leftScore + rightScore);

    RWRETURN(separatorCost);
}

/**
 * \ingroup evaluators
 * \ref RtWorldImportHintBBoxPartitionEvaluator tests whether the partition cuts through
 * any shield bounding box hints, and returns a proportional value.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition requiring evaluation.
 * \param userData     A pointer to an \ref RwReal that is the cost per intersecting
 * Bounding Box. If it's NULL, rtWORLDIMPORTINVALIDPARTITION is returned after the first cut
 *
 * \return The weighted number of bounding box cuts.
 */
RwReal
RtWorldImportHintBBoxPartitionEvaluator(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                                    RtWorldImportPartition *partition,
                                    void *userData)
{
    RtWorldImportHints *hints = RtWorldImportHintsGetGroup(rtWORLDIMPORTSHIELDHINT);
    RwInt32 i;
    RwReal costPerCut = 0;
    RwReal costTotal = 0;
    RwBool costPerCutCalc = FALSE;

    RWAPIFUNCTION(RWSTRING("RtWorldImportHintBBoxPartitionEvaluator"));

    if(userData)
    {
        costPerCut = *((RwReal*)userData);
        costPerCutCalc = TRUE;
    }

    if(!hints)
    {
        /* there are no hints so the cost is 0 */
        RWRETURN(0);
    }

    /* test partition againt every bounding box hint */
    for(i=0;i<hints->numBoundingBoxes;i++)
    {
        /* Test for sector surrounding hint ( which will be ignored ) */
        if(!((GETCOORD(hints->boundingBoxes[i].bBox.sup, 0) >= GETCOORD(buildSector->boundingBox.sup, 0)) &&
             (GETCOORD(hints->boundingBoxes[i].bBox.inf, 0) <= GETCOORD(buildSector->boundingBox.inf, 0)) &&
             (GETCOORD(hints->boundingBoxes[i].bBox.sup, 4) >= GETCOORD(buildSector->boundingBox.sup, 4)) &&
             (GETCOORD(hints->boundingBoxes[i].bBox.inf, 4) <= GETCOORD(buildSector->boundingBox.inf, 4)) &&
             (GETCOORD(hints->boundingBoxes[i].bBox.sup, 8) >= GETCOORD(buildSector->boundingBox.sup, 8)) &&
             (GETCOORD(hints->boundingBoxes[i].bBox.inf, 8) <= GETCOORD(buildSector->boundingBox.inf, 8))) )
        {
            /* not surrounding. let's test for a partition intersection */
            if( (partition->value < GETCOORD(hints->boundingBoxes[i].bBox.sup, partition->type)) &&
                (partition->value > GETCOORD(hints->boundingBoxes[i].bBox.inf, partition->type)) )
            {
                /* test for sector hint intersection */
                if( (partition->type == 0) &&
                    ((GETCOORD(hints->boundingBoxes[i].bBox.inf, 4) <= GETCOORD(buildSector->boundingBox.sup, 4)) &&
                     (GETCOORD(hints->boundingBoxes[i].bBox.sup, 4) >= GETCOORD(buildSector->boundingBox.inf, 4))) &&
                    ((GETCOORD(hints->boundingBoxes[i].bBox.inf, 8) <= GETCOORD(buildSector->boundingBox.sup, 8)) &&
                     (GETCOORD(hints->boundingBoxes[i].bBox.sup, 8) >= GETCOORD(buildSector->boundingBox.inf, 8))) )
                {
                    /* partitioner cuts through a bounding box so costs */
                    if(costPerCutCalc == TRUE)
                    {
                        costTotal += costPerCut * hints->boundingBoxes[i].value;
                    }
                    else
                    {
                        RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
                    }
                }
                else if( (partition->type == 4) &&
                    ((GETCOORD(hints->boundingBoxes[i].bBox.inf, 0) <= GETCOORD(buildSector->boundingBox.sup, 0)) &&
                     (GETCOORD(hints->boundingBoxes[i].bBox.sup, 0) >= GETCOORD(buildSector->boundingBox.inf, 0))) &&
                    ((GETCOORD(hints->boundingBoxes[i].bBox.inf, 8) <= GETCOORD(buildSector->boundingBox.sup, 8)) &&
                     (GETCOORD(hints->boundingBoxes[i].bBox.sup, 8) >= GETCOORD(buildSector->boundingBox.inf, 8))) )
                {
                    /* partitioner cuts through a bounding box so costs*/
                    if(costPerCutCalc == TRUE)
                    {
                        costTotal += costPerCut * hints->boundingBoxes[i].value;
                    }
                    else
                    {
                        RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
                    }
                }
                else if( (partition->type == 8) &&
                    ((GETCOORD(hints->boundingBoxes[i].bBox.inf, 0) <= GETCOORD(buildSector->boundingBox.sup, 0)) &&
                     (GETCOORD(hints->boundingBoxes[i].bBox.sup, 0) >= GETCOORD(buildSector->boundingBox.inf, 0))) &&
                    ((GETCOORD(hints->boundingBoxes[i].bBox.inf, 4) <= GETCOORD(buildSector->boundingBox.sup, 4)) &&
                     (GETCOORD(hints->boundingBoxes[i].bBox.sup, 4) >= GETCOORD(buildSector->boundingBox.inf, 4))) )
                {
                    /* partitioner cuts through a bounding box so costs*/
                    if(costPerCutCalc == TRUE)
                    {
                        costTotal += costPerCut * hints->boundingBoxes[i].value;
                    }
                    else
                    {
                        RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
                    }
                }
            }
        }
    }

    RWRETURN(costTotal);
}



/********************************************************************************
 * PARTITION ITERATORS...
 */

static RwReal
FindHighestVertex(RtWorldImportBuildSector *buildSector, RtWorldImportBuildStatus *buildStatus)
{
    RwInt32 allboundaries = 0;
    RwReal dot;
    RwReal highest = -RwRealMAXVAL;
    RtWorldImportPartition xyCand, yzCand, xzCand;

    RWFUNCTION(RWSTRING("FindHighestVertex"));

    while (allboundaries<buildSector->numBoundaries)
    {
        if (buildSector->boundaries[allboundaries].mode.vpVert)
        {
            dot = GETCOORD(buildSector->boundaries[allboundaries].mode.vpVert->OC, 4);
            if (dot > highest)
            {
                xyCand.value = GETCOORD(buildSector->boundaries[allboundaries].mode.vpVert->OC, 8);
                xyCand.type = 8;
                xyCand.maxLeftValue = xyCand.value;
                xyCand.maxRightValue = xyCand.value;

                yzCand.value = GETCOORD(buildSector->boundaries[allboundaries].mode.vpVert->OC, 0);
                yzCand.type = 0;
                yzCand.maxLeftValue = yzCand.value;
                yzCand.maxRightValue = yzCand.value;

                xzCand.value = GETCOORD(buildSector->boundaries[allboundaries].mode.vpVert->OC, 4);
                xzCand.type = 4;
                xzCand.maxLeftValue = xzCand.value;
                xzCand.maxRightValue = xzCand.value;

                /* find highest vertex not stuck in a corner of the sector */
                if ((RtWorldImportVolumeBalancedPartitionEvaluator(buildSector, buildStatus,
                                &xyCand,
                                NULL) < 0.9) ||
                    (RtWorldImportVolumeBalancedPartitionEvaluator(buildSector, buildStatus,
                                &yzCand,
                                NULL) < 0.9) ||
                    (RtWorldImportVolumeBalancedPartitionEvaluator(buildSector, buildStatus,
                                &xzCand,
                                NULL) < 0.9))
                {
                    highest = dot;
                }
            }

        }
        allboundaries++;
    }
    RWRETURN(highest);
}


/**
 * \ingroup iterators
 * \ref RtWorldImportHintBBoxPartitionIterator iterates though all partitions
 * that are coincident with a face of a bounding box from the hints
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition that is found.
 * \param userData     NULL. (Can be passed an RwReal which takes on the strength value of
 *                     the partition hint.)
 * \param loopCounter  A pointer to the number of iterations thus far.
 *
 * \return TRUE if found.
 */
RwBool
RtWorldImportHintBBoxPartitionIterator(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                                    RtWorldImportPartition *partition,
                                    void * userData,
                                    RwInt32* loopCounter)
{
    RtWorldImportHints *hints = RtWorldImportHintsGetGroup(rtWORLDIMPORTPARTITIONHINT);
    static RwInt32 axis;
    static RwBool inf;
    static RwInt32 index, lindex;
    static RwBool xFlat = FALSE, yFlat = FALSE, zFlat = FALSE;
    RwReal e1, e2, w, v1, v2;
    RtWorldImportParameters *conversionParams = RtWorldImportParametersGet();
    RwReal overlap = conversionParams->maxOverlapPercent;

    RWAPIFUNCTION(RWSTRING("RtWorldImportHintBBoxPartitionIterator"));

    if (hints==NULL) RWRETURN(FALSE);

    if((*loopCounter) == 0)
    {
        axis = 0;
        inf = TRUE;
        index = 0;
        lindex = -1;
    }

    while(index < hints->numBoundingBoxes)
    {
        if (lindex!=index)
        {
            /* New box */
            lindex = index;

            xFlat = FALSE;
            yFlat = FALSE;
            zFlat = FALSE;
            if (GETCOORD(hints->boundingBoxes[index].bBox.inf, 0) == GETCOORD(hints->boundingBoxes[index].bBox.sup, 0))
            {
                xFlat = TRUE;
            }
            if (GETCOORD(hints->boundingBoxes[index].bBox.inf, 4) == GETCOORD(hints->boundingBoxes[index].bBox.sup, 4))
            {
                yFlat = TRUE;
            }
            if (GETCOORD(hints->boundingBoxes[index].bBox.inf, 8) == GETCOORD(hints->boundingBoxes[index].bBox.sup, 8))
            {
                zFlat = TRUE;
            }
        }
        if (((yFlat || zFlat) && (axis == 0)) ||
        ((xFlat || zFlat) && (axis == 4)) ||
        ((xFlat || yFlat) && (axis == 8)))
        {
            /* change axis */
            axis += 4;

            /* change bounding box */
            if(axis > 8)
            {
                axis = 0;
                index ++;
            }
            continue;
        }
        partition->type = axis;

        if(inf)
        {
            partition->value = GETCOORD(hints->boundingBoxes[index].bBox.inf, partition->type);
            inf = FALSE;
        }
        else
        {
            partition->value = GETCOORD(hints->boundingBoxes[index].bBox.sup, partition->type);
            inf = TRUE;

            /* change axis */
            axis += 4;

            /* change bounding box */
            if(axis > 8)
            {
                axis = 0;
                index ++;
            }
        }


        /*
        Ideally, we need to traverse tree so far with centre of this leaf, building a bbox of
        the real partitioner tessellation
        and use that in place of buildSector->boundBox
        */

        e1 = GETCOORD(buildSector->boundingBox.inf, partition->type);
        e2 = GETCOORD(buildSector->boundingBox.sup, partition->type);
        w = e2 - e1;
        v1 = w * overlap + e1;
        v2 = e2 - w * overlap;

        /* If the partition is in the sector...
         * ...and if the bbox of the bbox of the hint is partially inside the sector */
        if (((partition->value - PLANE_GUARD) > v1/*GETCOORD(buildSector->boundingBox.inf, partition->type)*/ &&
            (partition->value + PLANE_GUARD) < v2/*GETCOORD(buildSector->boundingBox.sup, partition->type)*/ ) &&
            (
                (GETCOORD(hints->boundingBoxes[lindex].bBox.sup, 0) > GETCOORD(buildSector->boundingBox.inf, 0)) &&
                (GETCOORD(hints->boundingBoxes[lindex].bBox.inf, 0) < GETCOORD(buildSector->boundingBox.sup, 0)) &&
                (GETCOORD(hints->boundingBoxes[lindex].bBox.sup, 4) > GETCOORD(buildSector->boundingBox.inf, 4)) &&
                (GETCOORD(hints->boundingBoxes[lindex].bBox.inf, 4) < GETCOORD(buildSector->boundingBox.sup, 4)) &&
                (GETCOORD(hints->boundingBoxes[lindex].bBox.sup, 8) > GETCOORD(buildSector->boundingBox.inf, 8)) &&
                (GETCOORD(hints->boundingBoxes[lindex].bBox.inf, 8) < GETCOORD(buildSector->boundingBox.sup, 8))
            ))
        {
            (*loopCounter)++;
            /*
            if(userData)
            {
                *((RwBool*)userData) = (inf) ? FALSE : TRUE;
            }
            */
            if (userData)
            {
                *((RwReal*)userData) = hints->boundingBoxes[lindex].value;
            }
            RWRETURN(TRUE);
        }
    }

    RWRETURN(FALSE);
}

typedef struct searchRange searchRange;
struct searchRange
{
    RwInt32 hold;
    RwReal high;
    RwReal low;
};


/**
 * \ingroup iterators
 * \ref RtWorldImportHighestVertexAccommodatingPlanePartitionIterator iterates through
 * all partitions that pass through the highest point in the sector.
 * \note The highest point returned may be located on a plane of the sector's bounding
 * box, but it cannot be on all three planes, i.e. a corner. This is considered out of
 * bounds so that at least one of the three candidate partitions (x, y, or z) is not peripheral.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition that is found.
 * \param userData     NULL.
 * \param loopCounter  A pointer to the number of iterations thus far.
 *
 * \return TRUE if found.
 */
RwBool
RtWorldImportHighestVertexAccommodatingPlanePartitionIterator(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                                    RtWorldImportPartition *partition,
                                    void * userData,
                                    RwInt32* loopCounter)
{
    /* Returns all (XY, ZY, XZ) planes passing through the highest point */
    RwInt32 hold = ((searchRange*)userData)->hold;
    static RwInt32 allboundaries;
    RwReal highest = ((searchRange*)userData)->high;
    RwReal lowest = ((searchRange*)userData)->low;
    RwReal vertH;

    RWAPIFUNCTION(RWSTRING("RtWorldImportHighestVertexAccommodatingPlanePartitionIterator"));

    if ((*loopCounter) == 0)
    {
        allboundaries = 0;
    }


    while (allboundaries<buildSector->numBoundaries)
    {
        if (buildSector->boundaries[allboundaries].mode.vpVert)
        {
            vertH = GETCOORD(buildSector->boundaries[allboundaries].mode.vpVert->OC, 4);
            if ((vertH >= lowest) && (vertH <= highest))
            {
                partition->value = GETCOORD(buildSector->boundaries[allboundaries].mode.vpVert->OC, hold);
                partition->type = hold;
                /*RtWorldImportSetPartitionStatistics(buildSector, partition);*/
                (*loopCounter)++;
                allboundaries++;

                RWRETURN(TRUE);
            }
        }
        allboundaries++;
    }

    RWRETURN(FALSE);
}

/**
 * \ingroup iterators
 * \ref RtWorldImportOrthogonalAutoPartitionIterator iterates through all partitions that
 * are defined by the plane of each triangle in the scene, if and only if it
 * is orthogonal.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition that is found.
 * \param userData     NULL.
 * \param loopCounter  A pointer to the number of iterations thus far.
 *
 * \return TRUE if found.
 */
RwBool
RtWorldImportOrthogonalAutoPartitionIterator(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                                    RtWorldImportPartition *partition,
                                    void *  userData __RWUNUSED__,
                                    RwInt32* loopCounter)
{
    /* Selects partitions that are coincident to (orthogonal) polygons in the scene */

    static RwInt32 hold;                       /* loop counters */
    static RwInt32 allboundaries;
    RwReal v1, v2, v3;

    RWAPIFUNCTION(RWSTRING("RtWorldImportOrthogonalAutoPartitionIterator"));

    if ((*loopCounter) == 0)
    {
        hold = 0;
        allboundaries = 0;
    }

    while (hold <= 8)
    {
        while (allboundaries<buildSector->numBoundaries)
        {
            v1 = GETCOORD(buildSector->boundaries[allboundaries++].mode.vpVert->OC, hold);
            v2 = GETCOORD(buildSector->boundaries[allboundaries++].mode.vpVert->OC, hold);
            v3 = GETCOORD(buildSector->boundaries[allboundaries++].mode.vpVert->OC, hold);

            /* more than 3 vertices, resulting from cut... */
            while (buildSector->boundaries[allboundaries++].mode.vpVert)
                ;

            if (EQ(v1, v2) && EQ(v2,v3)) /* if polygon is axis aligned */
            {
                partition->value = v1;
                partition->type = hold;
                /*RtWorldImportSetPartitionStatistics(buildSector, partition);*/
                (*loopCounter)++;

                RWRETURN(TRUE);
            }

        }
        allboundaries = 0;
        hold += 4;
    }
    RWRETURN(FALSE);
}


/**
 * \ingroup iterators
 * \ref RtWorldImportRegularIntervalPartitionIterator iterates through all partitions
 * that are equally spaced along each of the axes. The number is in the userData.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition that is found.
 * \param userData     A pointer to an \ref RwInt32 that is the number of partitions
 * per axis.
 * \param loopCounter  A pointer to the number of iterations thus far.
 *
 * \return TRUE if found.
 */
RwBool
RtWorldImportRegularIntervalPartitionIterator(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                                    RtWorldImportPartition *partition,
                                    void* userData,
                                    RwInt32* loopCounter)
{
    /* Selects a number IN_PROGRESS of polygons at regular interval along each axis */

    static RwInt32 hold;
    static RwReal nudge;
    RwReal inc;
    RwInt32 samps = *((RwInt32*)userData);

    RWAPIFUNCTION(RWSTRING("RtWorldImportRegularIntervalPartitionIterator"));

    if ((*loopCounter) == 0)
    {
        nudge = GETCOORD(buildSector->boundingBox.inf, hold);
        hold = 0;
    }

    while (hold <= 8)
    {
        inc = (((GETCOORD(buildSector->boundingBox.sup, hold)) -
                (GETCOORD(buildSector->boundingBox.inf, hold))) /
                ((RwReal)samps));

        if (inc <= 0.00001) continue;

        while (nudge <= GETCOORD(buildSector->boundingBox.sup, hold))
        {
            partition->value = nudge;
            partition->type = hold;
            /*RtWorldImportSetPartitionStatistics(buildSector, partition);*/
            nudge += inc;

            (*loopCounter)++;

            RWRETURN(TRUE);
        }
        nudge = GETCOORD(buildSector->boundingBox.inf, hold);
        hold += 4;
    }

    RWRETURN(FALSE);
}

/**
 * \ingroup iterators
 * \ref RtWorldImportMedianPercentagePartitionIterator iterates through a number
 * of partitions that are located towards the centre of the population of the sector.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition that is found.
 * \param userData     A pointer to an \ref RwInt32 that is the number of partitions
 * per axis.
 * \param loopCounter  A pointer to the number of iterations thus far.
 *
 * \return TRUE if successful, FALSE otherwise
 */
RwBool
RtWorldImportMedianPercentagePartitionIterator(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                                    RtWorldImportPartition *partition,
                                    void *userData,
                                    RwInt32 *loopCounter)
{
    RwInt32             nI;
    RwReal              quantThresh;
    RwReal              interIndexRange;
    RwInt32             maxClosestCheck;
    static RwInt32      partitionIndex;
    static RwInt32      tryPlane;
    static _rwSortVertex *sortVerts;
    static RwInt32      lowerIndex;
    static RwInt32      upperIndex;
    static RwInt32      lastQuantValue;


    RWAPIFUNCTION(RWSTRING("RtWorldImportMedianPercentagePartitionIterator"));

    RWASSERT(buildSector);
    RWASSERT(partition);

    maxClosestCheck = *((RwInt32*)userData);

    /* initalise partition plane iterator */
    if((*loopCounter) == 0)
    {
        /* Find the maximum extent for this sector */
#if 0
          RwV3dSub(&vSize, &buildSector->boundingBox.sup,
               &buildSector->boundingBox.inf);

          maxExtent = 0;
          for (tryPlane = 0; tryPlane < 12; tryPlane += sizeof(RwReal))
          {
              if (GETCOORD(vSize, tryPlane) > GETCOORD(vSize, maxExtent))
              {
                 maxExtent = tryPlane;
              }
          }
#endif
        /* Allocate an array for all of the vertices in this sector */
        sortVerts = (_rwSortVertex *)
            RwMalloc(sizeof(_rwSortVertex) * buildSector->numVertices,
                     rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
        for (nI = 0; nI < buildSector->numVertices; nI++)
        {
            sortVerts[nI].vpVert = &buildSector->vertices[nI];
        }

        /* set up values for the first partition */
        tryPlane = 0;

        /* find median of population */
        for (nI = 0; nI < buildSector->numVertices; nI++)
        {
            sortVerts[nI].dist =
                GETCOORD(sortVerts[nI].vpVert->OC, tryPlane);
        }

        qsort((void *) sortVerts,
              buildSector->numVertices,
              sizeof(_rwSortVertex), ImportSortVertexCmp);

        /* value that bisects population in tryPlane axis */
        /* median = sortVerts[nI >> 1].dist; */
        lowerIndex = (nI * 40) / 100; /* 40% */
        upperIndex = (nI * 60) / 100; /* 60% */

        /* granularity by which we move plane around +/-25% of median */
        interIndexRange =
            sortVerts[upperIndex].dist - sortVerts[lowerIndex].dist;
        quantThresh =
            ((RwReal) maxClosestCheck) /
            interIndexRange;

        partitionIndex = lowerIndex;

#if 0
          tryPlane = -sizeof(RwReal);
          partitionIndex = 1;
          upperIndex = 0;
#endif
          lastQuantValue = -1;
    }

    /* Go through the planes until we find one to return */
    while(tryPlane < 12)
    {
        RwReal              quantThresh = (RwReal)0;
        RwReal              interIndexRange;
        RwReal              /*score,*/ tryValue;
        RwInt32             quantValue;
#if 0
        RwReal              extentScore;
        RwReal              median;
#endif

        if(partitionIndex > upperIndex)
        {
            tryPlane += sizeof(RwReal);
            if(tryPlane >= 12 ) break;

            /* we use extent just as a tiebreak only */
#if 0
              FuzzyWorldExtentMacro(extentScore,
                                    GETCOORD(vSize, tryPlane),
                                    GETCOORD(vSize, maxExtent));
              RWASSERT(0.0 != extentScore);
              extentScore = (1.0f / extentScore);
#endif
            /* find median of population */
            for (nI = 0; nI < buildSector->numVertices; nI++)
            {
                sortVerts[nI].dist =
                    GETCOORD(sortVerts[nI].vpVert->OC, tryPlane);
            }

            qsort((void *) sortVerts,
                  buildSector->numVertices,
                  sizeof(_rwSortVertex), ImportSortVertexCmp);

            /* value that bisects population in tryPlane axis */
#if 0
            median = sortVerts[nI >> 1].dist;
#endif
            lowerIndex = (nI * 40) / 100; /* 40% */
            upperIndex = (nI * 60) / 100; /* 60% */

            /* granularity by which we move plane around +/-25% of median */
            interIndexRange =
                sortVerts[upperIndex].dist - sortVerts[lowerIndex].dist;
            quantThresh =
                ((RwReal) maxClosestCheck) /
                interIndexRange;

            partitionIndex = lowerIndex;
        }

        (*loopCounter)++;

        /* return next candidate planes */
        tryValue = GETCOORD(sortVerts[partitionIndex++].vpVert->OC, tryPlane);
        quantValue = RwInt32FromRealMacro(tryValue * quantThresh);

        if (quantValue != lastQuantValue)
        {
            /* set candidate plane */
            partition->value = tryValue;
            partition->type = tryPlane;
            /*RtWorldImportSetPartitionStatistics(buildSector, partition);*/

            lastQuantValue = quantValue;

            RWRETURN(TRUE);
        }
    }

    /* clean up */
    RwFree(sortVerts);

    RWRETURN(FALSE);
}

static RwBool
ImportCentralMaximumExtentPartitionIterator(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                                    RtWorldImportPartition *partition,
                                    void* userData,
                                    RwInt32* loopCounter)
{
    static RwInt32 hold;
    static RwReal leftRange, rightRange;
    static RwReal width;
#if (0)
    static RwReal nudge;
    RwReal inc;
#endif /* (0) */
    RwReal shift = (RwReal)RwPow((RwReal)2.0, -(*loopCounter)) * (RwReal)0.5;
    RwInt32 samps = 16;

    RWFUNCTION(RWSTRING("ImportCentralMaximumExtentPartitionIterator"));

    if ((*loopCounter) == samps)
    {
        RWRETURN(FALSE);
    }
    else if ((*loopCounter) == 0)
    {
        hold = *((RwInt32*)userData);
        leftRange = GETCOORD(buildSector->boundingBox.inf, hold);
        rightRange = GETCOORD(buildSector->boundingBox.sup, hold);
        width = rightRange - leftRange;

        partition->value = (leftRange + rightRange) / (RwReal)2;
        partition->type = hold;
        RtWorldImportSetPartitionStatistics(buildSector, partition);
    }
    else
    {
        leftRange = partition->value - (width * shift);
        rightRange = partition->value + (width * shift);

        if (partition->buildStats.numActualLeft >
            partition->buildStats.numActualRight)
        {
            /* set candidate plane */
            partition->value = leftRange;
            partition->type = hold;
            RtWorldImportSetPartitionStatistics(buildSector, partition);
        }
        else
        {
            /* set candidate plane */
            partition->value = rightRange;
            partition->type = hold;
            RtWorldImportSetPartitionStatistics(buildSector, partition);
        }
    }
    (*loopCounter)++;

    RWRETURN(TRUE);
}

/**
 * \ingroup iterators
 * \ref RtWorldImportMiddleAxisPartitionIterator iterates through all partitions
 * that are in the middle of the sector, (one for each axis).
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition that is found.
 * \param userData     NULL.
 * \param loopCounter  A pointer to the number of iterations thus far.
 *
 * \return TRUE if successful, FALSE if there is an error
 */
RwBool
RtWorldImportMiddleAxisPartitionIterator(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                                    RtWorldImportPartition *partition,
                                    void * userData __RWUNUSED__,
                                    RwInt32 *loopCounter)
{
    static RwInt32 tryPlane;
    RwV3d vSize;

    RWAPIFUNCTION(RWSTRING("RtWorldImportMiddleAxisPartitionIterator"));

    if ((*loopCounter)==0) tryPlane=0;

    RwV3dSub(&vSize, &buildSector->boundingBox.sup,
                     &buildSector->boundingBox.inf);

    while (tryPlane < (RwInt32)(3*sizeof(RwReal)))
    {
        RwV3dScale(&vSize, &vSize, 0.5f);
        RwV3dAdd(&vSize, &vSize, &buildSector->boundingBox.inf);

        partition->value = GETCOORD(vSize, tryPlane);
        partition->type = tryPlane;
        /*RtWorldImportSetPartitionStatistics(buildSector, partition);*/

        tryPlane += sizeof(RwReal);

        (*loopCounter)++;

        RWRETURN(TRUE);
    }

    RWRETURN(FALSE);
}

/* rtImport only stores the matIndex in the info node */
static RwUInt32
getMaterialIndex(RtWorldImportBuildVertex *boundaries, RwInt32 index)
{
    RWFUNCTION(RWSTRING("getMaterialIndex"));

    /* find the polyInfo */
    while(boundaries[index++].mode.vpVert)
        ;

    RWRETURN(boundaries[index-1].pinfo.matIndex);
}

/**
 * \ingroup iterators
 * \ref RtWorldImportMaterialBoundsPartitionIterator iterates through
 * all material bounds in a world sector. This is useful for grouping materials
 * into the same sectors.
 *
 * \note This implementation assumes that matIndex < buildSector->maxNumMaterials
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition that is found.
 * \param userData     NULL.
 * \param loopCounter  A pointer to the number of iterations thus far.
 *
 * \return TRUE if successful, FALSE otherwise
 */
RwBool
RtWorldImportMaterialBoundsPartitionIterator(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                                    RtWorldImportPartition *partition,
                                    void *userData,
                                    RwInt32 *loopCounter)
{
    static RwBBox **materialBBoxes;
    static RwInt32 matCount;
    static RwInt32 axis;
    static RwBool inf;
    static RwInt32 bboxIndex;
    RwBBox **materialBBoxesBin;
    RwUInt32 index;
    RwUInt32 binIndex;
    RwUInt32 matIndex;

    RWAPIFUNCTION(RWSTRING("RtWorldImportMaterialBoundsPartitionIterator"));

    /* Initialise Iterator - find all material boundaries */
    if(*loopCounter == 0)
    {
        /* a sparse array of bounding box pointers */
        materialBBoxesBin = (RwBBox**)RwMalloc(buildSector->maxNumMaterials*sizeof(RwBBox*),
                                               rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_FUNCTION);

        if(materialBBoxesBin)
        {
            matCount=0;
            axis=0;
            inf=TRUE;
            bboxIndex=0;

            /* initalise all pointer to NULL */
            for(index=0;index<buildSector->maxNumMaterials;index++)
            {
                materialBBoxesBin[index] = NULL;
            }

            /* go through all the boundaries for generating bounding box
                around each material */
            index=0;
            while(index < (RwUInt32)buildSector->numBoundaries)
            {
                matIndex = getMaterialIndex(buildSector->boundaries, index);

                /* set the new span values using the first vertex in the span */
                if(buildSector->boundaries[index].mode.vpVert)
                {
                    if(materialBBoxesBin[matIndex])
                    {
                        RwBBoxAddPoint(materialBBoxesBin[matIndex],
                                           &buildSector->boundaries[index].mode.vpVert->OC);
                    }
                    else
                    {
                        matCount++;

                        /* allocate new BBox for the material */
                        materialBBoxesBin[matIndex] =
                                            (RwBBox*)RwMalloc(sizeof(RwBBox),
                                            rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_FUNCTION);

                        /* add initial point to the bounding box */
                        RwBBoxInitialize(materialBBoxesBin[matIndex],
                                            &buildSector->boundaries[index].mode.vpVert->OC);
                    }
                }

                index++;
            }

            /* allocate a new array for the material bounding boxes */
            materialBBoxes = (RwBBox**)RwMalloc(matCount*sizeof(RwBBox*),
                rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_FUNCTION);

            /* copy the sparse array of material Bounding Boxes in to a linear array */
            index=0;
            binIndex=0;
            while(binIndex<buildSector->maxNumMaterials)
            {
                if(materialBBoxesBin[binIndex])
                {
                    /* This material is being used */
                    materialBBoxes[index++] = materialBBoxesBin[binIndex];
                }

                binIndex++;
            }

            /* free the sparse array */
            RwFree(materialBBoxesBin);
        }
        else
        {
            RWRETURN(FALSE);
        }
    }

    /* iterate all partitons */
    while(bboxIndex<matCount)
    {
        partition->type = axis;
        if(userData) *((RwBBox*)userData) = *materialBBoxes[bboxIndex];

        if(inf)
        {
            partition->value = GETCOORD(materialBBoxes[bboxIndex]->inf, partition->type);
            inf = FALSE;
        }
        else
        {
            partition->value = GETCOORD(materialBBoxes[bboxIndex]->sup, partition->type);
            inf = TRUE;

            /* change axis */
            axis += 4;

            /* change bounding box */
            if(axis > 8)
            {
                axis = 0;
                bboxIndex++;
            }
        }

        /* return the partitioner */
        /*RtWorldImportSetPartitionStatistics(buildSector, partition);*/
        (*loopCounter)++;
        RWRETURN(TRUE);
    }

    /* There are no more partitions */
    /* Clean up the memory */

    for(index=0;index<(RwUInt32)matCount;index++)
    {
        RwFree(materialBBoxes[index]);
    }

    RwFree(materialBBoxes);

    RWRETURN(FALSE);
}


/********************************************************************************
 * PARTITION SELECTORS...
 */

typedef struct PolySpan PolySpan;
struct PolySpan
{
    RwReal min;
    RwReal max;
    PolySpan *next;
};

static RwBool
GetBiggestSpanListGapPartitionSelectorSpecial(RtWorldImportBuildSector * buildSector, RwInt32 type,
                          PolySpan *list, RwReal *gapStart, RwReal *gapEnd,
                          RwReal *gapStartValue, RwReal *gapEndValue, RwReal *gapValue)
{
    PolySpan *current = list;
    RwReal bestGap = (RwReal)0;
    RwReal currentGap;
    RtWorldImportPartition tempStartPartition;
    RtWorldImportPartition tempEndPartition;
    RwReal tempStartValue, tempEndValue;

    RWFUNCTION(RWSTRING("GetBiggestSpanListGapPartitionSelectorSpecial"));

    while(current != NULL)
    {
        if(current->next != NULL)
        {
            currentGap = (current->next->min - current->max);
            if(currentGap > bestGap)
            {
                tempStartPartition.value = current->max;
                tempStartPartition.type = type;
                tempEndPartition.value = current->max + currentGap;
                tempEndPartition.type = type;

                /* check here for hints - as long as one of the extremes of the gap don't cut
                 * a hint, we're okay
                 */
                tempStartValue = RtWorldImportHintBBoxPartitionEvaluator(buildSector, NULL, &tempStartPartition, NULL);
                tempEndValue = RtWorldImportHintBBoxPartitionEvaluator(buildSector, NULL, &tempEndPartition, NULL);
                if((tempStartValue != rtWORLDIMPORTINVALIDPARTITION) ||
                    (tempEndValue != rtWORLDIMPORTINVALIDPARTITION))
                {
                    *gapValue = currentGap;
                    *gapStart = tempStartPartition.value;
                    *gapEndValue = tempEndValue;
                    *gapStartValue = tempStartValue;
                    *gapEnd = tempEndPartition.value;
                }
            }
        }

        current = current->next;
    }

    RWRETURN(bestGap > 0);
}





static void
CleanUpSpanList(PolySpan *list)
{
    PolySpan *current = list, *temp;

    RWFUNCTION(RWSTRING("CleanUpSpanList"));

    while(current != NULL)
    {
        temp = current;
        current = current->next;
        RwFree(temp);
    }

    RWRETURNVOID();
}

/* Add a new span to a list of sorted and merged 1D spans */
static void
AddPolySpan(PolySpan **list, RwReal min, RwReal max)
{
    PolySpan *previous = NULL, *current;

    RWFUNCTION(RWSTRING("AddPolySpan"));


    if((*list) == NULL)
    {
        current = (PolySpan*)RwMalloc(sizeof(PolySpan),
            rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
        current->min = min;
        current->max = max;
        current->next = NULL;
        *list = current;
    }
    else
    {
        current = (*list);
        while(current != NULL) /* for all spans */
        {
            /* overlap? */
            if( (min <= current->max) && (max >= current->min) )
            {
                /* merge spans */
                if(min < current->min)
                {
                    current->min = min;
                }
                if(max > current->max)
                {
                    current->max = max;
                }

                /* it is possible at this point that the merged span could
                    overlap the spans that follow in the list */

                previous = current;
                current = current->next;

                /* have we merged? */
                while((current != NULL) && (previous->max >= current->min) )
                {
                    /* merge spans */
                    /*if(previous->min < current->min)
                    {
                        previous->min = current->min;
                    }*/
                    if(previous->max < current->max)
                    {
                        previous->max = current->max;
                    }

                    /* remove current */
                    previous->next = current->next;
                    RwFree(current);
                    current = previous->next;
                }

                RWRETURNVOID();
            }
            /* less than */
            else if(max < current->min)
            {
                /* add here */
                if(previous == NULL) /* first in list */
                {
                    (*list) = (PolySpan*)RwMalloc(sizeof(PolySpan),
                       rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
                    (*list)->next = current;
                    (*list)->min = min;
                    (*list)->max = max;
                }
                else
                {
                    previous->next = (PolySpan*)RwMalloc(sizeof(PolySpan),
                              rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
                    previous->next->min = min;
                    previous->next->max = max;
                    previous->next->next = current;
                }
                RWRETURNVOID();
            }

            /* greater than */
            /* next */
            previous = current;
            current = current->next;
        }

        /* Not found place or merged, add to end */
        previous->next = (PolySpan*)RwMalloc(sizeof(PolySpan),
            rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
        previous->next->min = min;
        previous->next->max = max;
        previous->next->next = NULL;
    }

    RWRETURNVOID();
}


/* shrink wrap a bounding box around a material */
static RwBool
ShrinkWrapMaterialBBox(RtWorldImportBuildSector *buildSector,
                                          RwBBox *materialBBox, RwInt32 matIndex)
{   RwBBox bbOut;
    RwInt32 currentMatIndex;
    RwInt32 vertexCount = 0;
    RwBool firstVertex = TRUE;

    RWFUNCTION(RWSTRING("ShrinkWrapMaterialBBox"));
    RWASSERT(buildSector);
    RWASSERT(materialBBox);

    if (buildSector->vertices)
    {
        /* go through all the boundaries for this axis */
        while(vertexCount<buildSector->numBoundaries)
        {
            /* get the material index for this poly */
            currentMatIndex = getMaterialIndex(buildSector->boundaries, vertexCount);

            /* set the new span values using the first vertex in the span */
            if(currentMatIndex == matIndex)
            {
                /* add the rest of the vertices in this bounding box */
                while(buildSector->boundaries[vertexCount].mode.vpVert)
                {
                    if(RwBBoxContainsPoint(materialBBox, &buildSector->boundaries[vertexCount].mode.vpVert->OC))
                    {
                        /* initalise the bounding box with the first vertex */
                        if(firstVertex)
                        {
                            RwBBoxInitialize(&bbOut, &buildSector->boundaries[vertexCount].mode.vpVert->OC);
                            firstVertex = FALSE;
                        }
                        else
                        {
                            RwBBoxAddPointMacro(&bbOut, &buildSector->boundaries[vertexCount].mode.vpVert->OC);
                        }
                    }

                    vertexCount++;
                }

                /* skip the info boundaries vertex */
                vertexCount++;
            }
            else
            {
                /* move to the next poly */
                while(buildSector->boundaries[vertexCount++].mode.vpVert)
                    ;
            }
        }
    }

    if(firstVertex)
    {
        RWRETURN(FALSE);
    }

    *materialBBox = bbOut;
    RWRETURN(TRUE);
}

/* find a material partition */
RwBool
_rtWorldImportCullMiddleMaterialBBox(RtWorldImportBuildSector *buildSector,
                                    RwBBox *materialBBox,
                                    RwBBox *newBBox,
                                    RwInt32 matIndex)
{
    RwInt32 tryPlane;
    RwInt32 vertexCount = 0;
    PolySpan *allSpans;
    RwReal value, min, max;
    RwReal gapStart, gapEnd, gapSize;
    RwReal gapStartValue, gapEndValue;
    RwReal bestStart = 0, bestSize = 0;
    RwInt32 bestPlane = (RwInt32) 0;
    RwInt32 currentMatIndex;
    RwReal infEpsilon = (RwReal)0;
    RwReal supEpsilon = (RwReal)0;
    RwReal epsilon;
#if (0)
    RtWorldImportParameters *conversionParams = RtWorldImportParametersGet();
#endif /* (0) */

    RWFUNCTION(RWSTRING("_rtWorldImportCullMiddleMaterialBBox"));

    /* do nothing if it is spacefilling */
    if (!buildSector->vertices)
    {
        RWRETURN(FALSE);
    }



    epsilon = (((RwReal)1)/((RwReal)(1<<10)));
/*
        (RwReal)_rtImportBuildSectorFindBBoxDiagonal(&buildSector->boundingBox) /
                (RwReal)(1 << 16);
*/
    /* try X,Y and Z axis */
    for (tryPlane = 0; tryPlane < 12; tryPlane += sizeof(RwReal))
    {
        vertexCount = 0;
        allSpans = NULL;

        /* go through all the boundaries for this axis */
        while(vertexCount<buildSector->numBoundaries)
        {
            currentMatIndex = getMaterialIndex(buildSector->boundaries, vertexCount);

            /* set the new span values using the first vertex in the span */
            if(currentMatIndex == matIndex)
            {
                if(RwBBoxContainsPoint(materialBBox, &buildSector->boundaries[vertexCount].mode.vpVert->OC))
                {
                    min = max = GETCOORD(buildSector->boundaries[vertexCount++].mode.vpVert->OC, tryPlane);

                    /* test the rest of the vertices is this boundaries section */
                    while(buildSector->boundaries[vertexCount].mode.vpVert)
                    {
                        if(RwBBoxContainsPoint(materialBBox, &buildSector->boundaries[vertexCount].mode.vpVert->OC))
                        {
                            value = GETCOORD(buildSector->boundaries[vertexCount].mode.vpVert->OC, tryPlane);
                            if(value < min) min = value;
                            if(value > max) max = value;
                        }

                        vertexCount++;
                    }

                    /* skip the info boundaries vertex */
                    vertexCount++;

                    /* Add the new span to the list */
                    AddPolySpan(&allSpans, min, max);
                }
                else
                {
                    /* move to the next poly */
                    while(buildSector->boundaries[vertexCount++].mode.vpVert)
                        ;
                }
            }
            else
            {
                /* move to the next poly */
                while(buildSector->boundaries[vertexCount++].mode.vpVert)
                    ;
            }
        }

        /* find the largest gap */
        if(GetBiggestSpanListGapPartitionSelectorSpecial(buildSector, tryPlane, allSpans, &gapStart, &gapEnd,
            &gapStartValue, &gapEndValue, &gapSize))
        {
            /* test to make sure that tht start of the gap is not
                co-planar to the edge of the bounding box */

            if(gapSize > bestSize)
            {
                /*if(gapStart == GETCOORD(materialBBox->inf, tryPlane))
                {
                    bestSize = gapSize;
                    bestStart = gapStart;
                    bestPlane = tryPlane;
                    infEpsilon = 0;
                    supEpsilon = SPLIT_EPSILON;
                }
                else*/ if(gapStart < (GETCOORD(materialBBox->inf, tryPlane)+epsilon))
                {
                    bestSize = gapSize;
                    bestStart = gapStart;
                    bestPlane = tryPlane;
                    infEpsilon = 0;
                    supEpsilon = epsilon;
                }
                else /*if(gapStart > GETCOORD(materialBBox->inf, tryPlane))*/
                {
                    bestSize = gapSize;
                    bestStart = gapStart;
                    bestPlane = tryPlane;
#if 0
                    infEpsilon = -SPLIT_EPSILON;
                    supEpsilon = 0;
#endif
                    infEpsilon = 0;
                    supEpsilon = epsilon;
                }
            }
#if 0
            if( gapStart  >
                (GETCOORD(buildSector->boundingBox.inf, tryPlane) + SPLIT_EPSILON) )
            if( gapStart >=
                (GETCOORD(materialBBox->inf, tryPlane)) )
            {
                if(gapSize > bestSize)
                {
                    bestSize = gapSize;
                    bestStart = gapStart;
                    bestPlane = tryPlane;
                    epsilon = SPLIT_EPSILON;
                }
            }
            /* gap Start is no good so let's try the gap end */
            else if( (gapStart+gapSize)  <
                (GETCOORD(buildSector->boundingBox.sup, tryPlane) - SPLIT_EPSILON) )
            else if( gapStart >=
                (GETCOORD(materialBBox->sup, tryPlane)) )
            {
                if(gapSize > bestSize)
                {
                    bestSize = gapSize;
                    bestStart = gapStart + gapSize;
                    bestPlane = tryPlane;
                    epsilon = -SPLIT_EPSILON;
                }
            }
#endif
            /* else it's no good - forget it*/
        }

        CleanUpSpanList(allSpans);
    }

    if (bestSize > epsilon)
    {
        /* we've found a good partition so lets split the bounding box
            and shrink wrap the 2 new ones */

        if((bestStart > ((RwReal)1)) || (bestStart < -1.0f))
        {
            infEpsilon *= RwRealAbs(bestStart);
            supEpsilon *= RwRealAbs(bestStart);
        }

        /* split */
        *newBBox = *materialBBox;
        SETCOORD(materialBBox->sup, bestPlane, bestStart+infEpsilon);
        SETCOORD(newBBox->inf, bestPlane, bestStart+supEpsilon);

        /* shrink wrap */
       ShrinkWrapMaterialBBox(buildSector, materialBBox, matIndex);
       ShrinkWrapMaterialBBox(buildSector, newBBox, matIndex);

        RWRETURN(TRUE);
    }
    else
    {
        RWRETURN(FALSE);
    }
}

/**
 * \ingroup selectors
 * \ref RtWorldImportCullMiddleSpacePartitionSelector selects the partition
 * that removes the largest amount of empty space in a sector - this space is
 * from the middle of the sector only.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition that is found.
 * \param userData     A pointer to an \ref RwReal that is the minimum percentage of
 * space that may be removed. (Typically set between 0.05 and 0.25.)
 *
 * \return The cost value of the partitioner.
 */
RwReal
RtWorldImportCullMiddleSpacePartitionSelector(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                                    RtWorldImportPartition *partition,
                                    void *userData)
{
    RwInt32 tryPlane;
    RwInt32 vertexCount = 0;
    PolySpan *allSpans;
    RwReal value, min, max;
    RwReal gapStart, gapEnd, gapValue;
    RwReal gapStartValue, gapEndValue;
    RwReal bestStart = 0, bestSize = 0;
    RwInt32 bestPlane = (RwInt32) 0;
    RwReal *minGapSizePercentage = (RwReal *)userData;
    RwReal minGapSize;
    RwV3d vSize;

    RWAPIFUNCTION(RWSTRING("RtWorldImportCullMiddleSpacePartitionSelector"));


    /* Find the maximum extent for this sector */
    RwV3dSub(&vSize, &buildSector->boundingBox.sup,
             &buildSector->boundingBox.inf);

    /* do nothing if it is spacefilling */
    if (!buildSector->vertices)
    {
        RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
    }

    /* try X,Y and Z axis */
    for (tryPlane = 0; tryPlane < 12; tryPlane += sizeof(RwReal))
    {
        vertexCount = 0;
        allSpans = NULL;

        /* go through all the boundaries for this axis */
        while(vertexCount<buildSector->numBoundaries)
        {
            /* set the new span values using the first vertex in the span */
            min = max = GETCOORD(buildSector->boundaries[vertexCount++].mode.vpVert->OC, tryPlane);

            /* test the rest of the vertices is this boundaries section */
            while(buildSector->boundaries[vertexCount].mode.vpVert)
            {
                value = GETCOORD(buildSector->boundaries[vertexCount++].mode.vpVert->OC, tryPlane);
                if(value < min) min = value;
                if(value > max) max = value;
            }

            /* skip the info boundaries vertex */
            vertexCount++;

            /* Add the new span to the list */
            AddPolySpan(&allSpans, min, max);
        }

        /* find the largest gap in this axis */
        if(GetBiggestSpanListGapPartitionSelectorSpecial(buildSector, tryPlane, allSpans, &gapStart, &gapEnd,
            &gapStartValue, &gapEndValue, &gapValue))
        {
            /* test to make sure that the start of the gap is not
                co-planar to the edge of the bounding box */
            RwReal size = GETCOORD(vSize, tryPlane), offset;
            RwReal balance = gapValue;
            RtWorldImportPartition tempPartition;

            if (gapStart  >
                   (GETCOORD(buildSector->boundingBox.inf, tryPlane) + PLANE_GUARD) )
            {
                tempPartition.type = tryPlane;
                tempPartition.value = gapStart;
                if((gapValue > bestSize) && (gapStartValue != rtWORLDIMPORTINVALIDPARTITION))
                {
                    bestSize = gapValue;
                    bestStart = gapStart;
                    AddEpsilon(&bestStart);
                    bestPlane = tryPlane;
                    offset = (gapStart - GETCOORD(buildSector->boundingBox.inf, tryPlane));
                    balance = (offset - size) - (size - offset);
                    /* REPLACE WITH: balance = (RwReal)2 * (offset - size); */
                }
            }

            /* gap End might be better */

            offset = (gapEnd - GETCOORD(buildSector->boundingBox.inf, tryPlane));
            /* REPLACE WITH: offset = GETCOORD(buildSector->boundingBox.inf, tryPlane)) */

            if (balance > ((offset - size) - (size - offset)))
            {
                if (gapEnd  <
                       (GETCOORD(buildSector->boundingBox.sup, tryPlane) - PLANE_GUARD))
                {
                    tempPartition.type = tryPlane;
                    tempPartition.value = gapEnd;
                    if((gapValue > bestSize) && (gapEndValue != rtWORLDIMPORTINVALIDPARTITION))
                    {
                        bestSize = gapValue;
                        bestStart = gapEnd;
                        SubtractEpsilon(&bestStart);
                        bestPlane = tryPlane;
                    }

                }
            }
            /* else it's no good - forget it*/
        }

        CleanUpSpanList(allSpans);
    }

    minGapSize  = GETCOORD(vSize, bestPlane) * (*minGapSizePercentage);

    if ((bestSize > minGapSize) && (minGapSize > PLANE_GUARD))
    {
        partition->type = bestPlane;
        partition->value = bestStart;
        RtWorldImportSetPartitionStatistics(buildSector, partition);
        RWRETURN(-bestSize);
    }
    else
    {
        RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
    }
}

/**
 * \ingroup selectors
 * \ref RtWorldImportCullEndSpacePartitionSelector selects the partition
 * that removes the largest amount of empty space in a sector - this space is
 * at either end only.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition that is found.
 * \param userData     A pointer to an \ref RwReal that is the minimum percentage of
 * space that may be removed. (Typically set between 0.05 and 0.25.)
 *
 * \return The cost value of the partitioner.
 */
RwReal
RtWorldImportCullEndSpacePartitionSelector(RtWorldImportBuildSector * buildSector,
                                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                                    RtWorldImportPartition *partition,
                                    void *userData)
{
    RwBBox boundingBox;
    RwInt32 tryPlane;
    RwInt32 bestPlane = (RwInt32) 0;
    RwReal bestValue = (RwReal) 0;
    RwV3d vSize;
    RwReal bestScore = (RwReal) 0;
    RwReal minCullSize;
    RtWorldImportPartition tempPartition;
    RwReal *minCullSizePercentage = (RwReal *)userData;
    /* RwReal epsilon; */

    RWAPIFUNCTION(RWSTRING("RtWorldImportCullEndSpacePartitionSelector"));


    /* Find the maximum extent for this sector */
    RwV3dSub(&vSize, &buildSector->boundingBox.sup,
             &buildSector->boundingBox.inf);

    /* do nothing if it is spacefilling */
    if (!buildSector->vertices)
    {
        RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
    }

    /* determine vertex extent (may be different from inherited BBox) */
    _rtImportBuildSectorFindBBox(buildSector, &boundingBox);

    /* We add a guard band of 0.001 around the geometry.
         * In the case of (near)planar geometry this makes
         * propOfSectorRemoved comparatively large and so chooses
         * to repeatedly slice off the guard band
         *  (and then re-add the guard band).
         */
    if( (GETCOORD(vSize, 0) > (PLANE_GUARD + PLANE_GUARD)) &&
         (GETCOORD(vSize, 4) > (PLANE_GUARD + PLANE_GUARD)) &&
         (GETCOORD(vSize, 8) > (PLANE_GUARD + PLANE_GUARD)) )
    {
        /* choose an axis to split along */
        for (tryPlane = 0; tryPlane < 12; tryPlane += sizeof(RwReal))
        {
            RwReal propOfSectorRemoved, score;

             /* Calculate the proportion of the world that would be chopped off at the top end
             *                                  <---------->        Proportion of this bit
             *                  ****************                    (Vertex bounding box)
             *  ********************************************        (Sector bounding box)
             *
             *  NB bump bestValue toward spacefill to avoid including any vertices
             */
            propOfSectorRemoved =
                ( GETCOORD(buildSector->boundingBox.sup,tryPlane) -
                  GETCOORD(boundingBox.sup, tryPlane) );

            score = propOfSectorRemoved;
            if (score > bestScore && (propOfSectorRemoved > PLANE_GUARD))
            {
                tempPartition.type = tryPlane;
                tempPartition.value = GETCOORD(boundingBox.sup, tryPlane);
                if(RtWorldImportHintBBoxPartitionEvaluator(buildSector, NULL, &tempPartition, NULL) !=
                    rtWORLDIMPORTINVALIDPARTITION)
                {
                    bestScore = score;
                    bestPlane = tryPlane;
                    bestValue = GETCOORD(boundingBox.sup, tryPlane);
                    AddEpsilon(&bestValue);
                }
            }
            /* Calculate the proportion of the world that would be chopped off at the bottom end
             *  <-------------->                                    Proportion of this bit
             *                  ****************                    (Vertex bounding box)
             *  ********************************************        (Sector bounding box)
             *
             *  NB bump bestValue toward spacefill to avoid including any vertices
             */
            propOfSectorRemoved =
                ( GETCOORD(boundingBox.inf, tryPlane) -
                  GETCOORD(buildSector->boundingBox.inf, tryPlane) );

            score = propOfSectorRemoved;
            if (score > bestScore && (propOfSectorRemoved > PLANE_GUARD))
            {
                tempPartition.type = tryPlane;
                tempPartition.value = GETCOORD(boundingBox.inf, tryPlane);
                if(RtWorldImportHintBBoxPartitionEvaluator(buildSector, NULL, &tempPartition, NULL) !=
                    rtWORLDIMPORTINVALIDPARTITION)
                {
                    bestScore = score;
                    bestPlane = tryPlane;
                    bestValue = GETCOORD(boundingBox.inf, tryPlane);
                    SubtractEpsilon(&bestValue);
                }
            }
        }
    }

    minCullSize  = GETCOORD(vSize, bestPlane) * (*minCullSizePercentage);
    if((bestScore > minCullSize) && (minCullSize > PLANE_GUARD))
    {
        partition->type = bestPlane;
        partition->value = bestValue;
        RtWorldImportSetPartitionStatistics(buildSector, partition);
        RWRETURN(-bestScore);
    }
    else
    {
        RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
    }
}

/**
 * \ingroup selectors
 * \ref RtWorldImportCullSpacePartitionSelector selects the partition
 * that removes the largest amount of empty space in a sector - this space can
 * be at either end, or the middle of the sector.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition that is found.
 * \param userData     A pointer to an \ref RwReal array of size two that is the
 *                     minimum percentage of space that may be removed for
 *                     end and middle space culling, respectively.
 *                     (Typically set between 0.05 and 0.25.)
 *
 * \return The cost value of the partitioner.
 */
RwReal
RtWorldImportCullSpacePartitionSelector(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus *buildStatus,
                                    RtWorldImportPartition *partition,
                                    void *userData)
{
    RwReal endCost, middleCost;
    RwReal** cullParams = (RwReal**)userData;
    RtWorldImportPartition endPartition, middlePartition;

    RWAPIFUNCTION(RWSTRING("RtWorldImportCullSpacePartitionSelector"));

    /* Get best partition for culling off the end of a sector */
    endCost = RtWorldImportCullEndSpacePartitionSelector(buildSector,
            buildStatus, &endPartition, &(cullParams[0]));

    /* Get bext partition for culling from the middle of a partition */
    middleCost = RtWorldImportCullMiddleSpacePartitionSelector(buildSector,
            buildStatus, &middlePartition, &(cullParams[1]));

    if(endCost < middleCost) /* edge is best */
    {
        if(endCost < rtWORLDIMPORTINVALIDPARTITION)
        {
            *partition = endPartition;
            RtWorldImportSetPartitionStatistics(buildSector, partition);
            RWRETURN(endCost);
        }
    }
    else
    {
        if(middleCost < rtWORLDIMPORTINVALIDPARTITION)
        {
            *partition = middlePartition;
            RtWorldImportSetPartitionStatistics(buildSector, partition);
            RWRETURN(middleCost);
        }
    }

    RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
}

/**
 * \ingroup hints
 * \ref RtWorldImportMaterialGroupHintGenerator Generates bounding boxes
 * around all material groups in the buildSector
 *
 * \param buildSector           A pointer to the \ref RtWorldImportBuildSector
 * \param materialGroupHints    A pointer to the \ref RtWorldImportHints
 *
 * \see RtWorldImportHintsSetGroup
 * \see RtWorldImportHintsGetGroup
 * \see RtWorldImportHintsCreate
 * \see RtWorldImportHintsDestroy
 * \see RtWorldImportHintsAddBoundingBoxes
 */
void
RtWorldImportMaterialGroupHintGenerator(RtWorldImportBuildSector *buildSector,
                                        RtWorldImportHints *materialGroupHints)
{
    RwBBox *materialBBoxes;
    RwInt32 materialBBoxCount;
    RwBBox currentBBox;
    RwBool groupFurther = TRUE;
    RwUInt32 nI, nJ;
    RtWorldImportHints *hints = materialGroupHints;

    RWAPIFUNCTION(RWSTRING("RtWorldImportMaterialGroupHintGenerator"));

    /* use the world partition hints as a default */
    if(!hints)
    {
        hints = RtWorldImportHintsGetGroup(rtWORLDIMPORTPARTITIONHINT);
    }

    /* Generate the inital bounding box around each material */
    for(nI=0;nI<buildSector->maxNumMaterials;nI++)
    {
        currentBBox = buildSector->boundingBox;

        if(ShrinkWrapMaterialBBox(buildSector, &currentBBox, nI))
        {
            /* success */
            materialBBoxes = (RwBBox*)RwMalloc(sizeof(RwBBox),
                rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_FUNCTION | rwMEMHINTFLAG_RESIZABLE);
            materialBBoxCount = 1;
            *(materialBBoxes) = currentBBox;
        }
        else
        {
            materialBBoxes = NULL;
            materialBBoxCount = 0;
        }

        /* cull the middle space to form more material groups */
        groupFurther = TRUE;

        while(groupFurther)
        {
            groupFurther = FALSE;

            /* try to divide all the BBoxes for this material */
            for(nJ=0;nJ<(RwUInt32)materialBBoxCount;nJ++)
            {
                if(_rtWorldImportCullMiddleMaterialBBox(buildSector,
                                                   &materialBBoxes[nJ],
                                                   &currentBBox,
                                                   nI))
                {
                    RwBBox *tempBBox;

                    /* we have mangaged to divide the bounding box further so
                        we need to reallocate the array */

                    materialBBoxCount++;
                    tempBBox = (RwBBox*)RwRealloc(materialBBoxes,
                                                            sizeof(RwBBox)*materialBBoxCount,
                                                            rwID_NOHSWORLDPLUGIN  |
                                                            rwMEMHINTDUR_FUNCTION |
                                                            rwMEMHINTFLAG_RESIZABLE);
                    if(tempBBox)
                    {
                        groupFurther = TRUE;
                        materialBBoxes = tempBBox;
                        materialBBoxes[materialBBoxCount-1] = currentBBox;
                    }
                }
            }
        }

        /* add bounding boxes to the rtimport global hints */
        for(nJ=0;nJ<(RwUInt32)materialBBoxCount;nJ++)
        {
            hints = RtWorldImportHintsAddBoundingBoxes(hints, 1);
            hints->boundingBoxes[hints->numBoundingBoxes-1].bBox = materialBBoxes[nJ];
        }

        /* Free this material's bounding boxes */
        RwFree(materialBBoxes);
    }
    RWRETURNVOID();
}

typedef struct BestCost BestCost;
struct BestCost
{
    RwReal fuzzyCost;
    RwReal occluderCost;
    RwReal cutCost;
    RwReal separatorCost;
};

/**
 * \ingroup selectors
 * \ref RtWorldImportMaterialCountPartitionSelector partitions along good occluders whilst
 * principally trying to minimise material cuts. It partitions until the number of
 * materials per sector is below a given number.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition that is found.
 * \param userData     A pointer to an \ref RwInt32 that is the
 *                     maximum number of materials per sector
 *
 * \return             The cost value of the partitioner.
 */
RwReal
RtWorldImportMaterialCountPartitionSelector(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus *buildStatus,
                                    RtWorldImportPartition *partition,
                                    void *userData)
{
    RwReal bestCost = rtWORLDIMPORTINVALIDPARTITION;
    RtWorldImportPartition candidate;
    RtWorldImportPartition bestSeparatorPartition;
    RwInt32 count = 0;
    RwReal AreaOfSector;
    BestCost bestSeparatorCost;
    RwReal occluderCost;
    RwReal cutCost;
    RwReal cullCost;
    RwReal separatorCost;
    RwReal cullParams[2];
    RwReal totalCost;
    RwInt32 materialCull = *((RwInt32*)userData);
    RwInt32 maxClosestCheck = 20;
    RwReal eval;

    RWAPIFUNCTION(RWSTRING("RtWorldImportMaterialCountPartitionSelector"));

    /* costs */
    bestSeparatorCost.fuzzyCost = rtWORLDIMPORTINVALIDPARTITION;
    bestSeparatorCost.occluderCost = rtWORLDIMPORTINVALIDPARTITION;
    bestSeparatorCost.cutCost = rtWORLDIMPORTINVALIDPARTITION;
    bestSeparatorCost.separatorCost = rtWORLDIMPORTINVALIDPARTITION;
    bestSeparatorPartition.type = -1;

    cullParams[0] = 0.15f; /* 15% */
    cullParams[1] = 0.05f; /* 5% */

    if((!buildSector->numVertices))
    {
        RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
    }

    /* first try to cull space */
    cullCost = RtWorldImportCullSpacePartitionSelector(buildSector,
                                    buildStatus, &candidate, &cullParams);

    /* Now see if there are any partition hints
     */
    eval = RtWorldImportPartitionHintPartitionSelector(buildSector,
        buildStatus,  &candidate, NULL);
    if (eval!=rtWORLDIMPORTINVALIDPARTITION)
    {
        *partition = candidate;
        RtWorldImportSetPartitionStatistics(buildSector, partition);
        RWRETURN(eval);
    }

    /* if we can cull space and there is more than 1 material */
    if((cullCost != rtWORLDIMPORTINVALIDPARTITION) && (candidate.buildStats.numMaterialSector > 1))
    {
        *partition = candidate;
        bestCost = 0;
    }
    else
    {
        while(RtWorldImportMedianPercentagePartitionIterator(buildSector,
                buildStatus,  &candidate, (void*)&maxClosestCheck, &count))
        {
            /* Calculate states */
            RtWorldImportSetPartitionStatistics(buildSector, &candidate);

            /* if there are materials on both sides of the partition */
            if( (candidate.buildStats.numMaterialLeft > 0) &&
                (candidate.buildStats.numMaterialRight > 0))
            {
                AreaOfSector = _rtWorldImportBuildSectorAreaOfWall(&buildSector->boundingBox, candidate.type);

                occluderCost = RtWorldImportWeightedOccluderPartitionEvaluator(buildSector, buildStatus, &candidate, &AreaOfSector);

                cutCost = (RwReal)(candidate.buildStats.numMaterialSplits);

                separatorCost = RtWorldImportMaterialSeparatorPartitionEvaluator(
                                                buildSector, buildStatus, &candidate, NULL);

                totalCost = separatorCost;

                /* test the results */
                if(totalCost < bestSeparatorCost.separatorCost)
                {
                    /* test hints */
                    if(RtWorldImportHintBBoxPartitionEvaluator(buildSector, buildStatus, &candidate, NULL) !=
                        rtWORLDIMPORTINVALIDPARTITION)
                    {
                        bestSeparatorCost.separatorCost = totalCost;
                        bestSeparatorCost.cutCost = cutCost;
                        bestSeparatorCost.occluderCost = occluderCost;
                        bestSeparatorPartition = candidate;
                    }
                }
                else if(totalCost == bestSeparatorCost.separatorCost)
                {

                    if(occluderCost < bestSeparatorCost.occluderCost)
                    {
                        /* test hints */
                        if(RtWorldImportHintBBoxPartitionEvaluator(buildSector, buildStatus, &candidate, NULL) !=
                            rtWORLDIMPORTINVALIDPARTITION)
                        {
                            bestSeparatorCost.separatorCost = totalCost;
                            bestSeparatorCost.cutCost = cutCost;
                            bestSeparatorCost.occluderCost = occluderCost;
                            bestSeparatorPartition = candidate;
                        }
                    }
                }
            }
        }

        RtWorldImportSetPartitionStatistics(buildSector, &bestSeparatorPartition);

        if(bestSeparatorPartition.buildStats.numMaterialSector <= materialCull)
        {
            /* terminate when we have less that the given number of materials */
            RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
        }
        else
        {
            *partition = bestSeparatorPartition;
            bestCost = bestSeparatorCost.separatorCost;
        }
    }

    if(bestCost < rtWORLDIMPORTINVALIDPARTITION)
    {
        RtWorldImportSetPartitionStatistics(buildSector, partition);
        RWRETURN(bestCost);
    }

    RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
}

/**
 * \ingroup selectors
 * \ref RtWorldImportMaterialSeparatePartitionSelector Tries to separate the scene into
 * regions of similar materials whilst minimizing the cuts to other materials.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition that is found.
 * \param userData     NULL.
 *
 * \return             The cost value of the partitioner.
 */
RwReal
RtWorldImportMaterialSeparatePartitionSelector(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus *buildStatus,
                                    RtWorldImportPartition *partition,
                                    void * userData __RWUNUSED__)
{
    RwReal bestCost = rtWORLDIMPORTINVALIDPARTITION;
    RtWorldImportPartition candidate;
    RtWorldImportPartition bestSeparatorPartition;
    RwInt32 count = 0;
    RwReal AreaOfSector;
    RtWorldImportHints *oldHints;
    BestCost bestSeparatorCost;
    RwReal occluderCost;
    RwReal cutCost;
    RwReal cullCost;
    RwReal separatorCost;
    RwReal cullParams[2];
    RwReal totalCost;
    RwReal eval;

    RWAPIFUNCTION(RWSTRING("RtWorldImportMaterialSeparatePartitionSelector"));

    /* costs */
    bestSeparatorCost.fuzzyCost = rtWORLDIMPORTINVALIDPARTITION;
    bestSeparatorCost.occluderCost = rtWORLDIMPORTINVALIDPARTITION;
    bestSeparatorCost.cutCost = rtWORLDIMPORTINVALIDPARTITION;
    bestSeparatorCost.separatorCost = rtWORLDIMPORTINVALIDPARTITION;
    bestSeparatorPartition.type = -1;

    cullParams[0] = 0.15f; /* 15% */
    cullParams[1] = 0.05f; /* 05% */

    if(!buildSector->numVertices)
    {
        RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
    }

    /* Now see if there are any partition hints
     */
    eval = RtWorldImportPartitionHintPartitionSelector(buildSector,
        buildStatus,  &candidate, NULL);
    if (eval!=rtWORLDIMPORTINVALIDPARTITION)
    {
        *partition = candidate;
        RtWorldImportSetPartitionStatistics(buildSector, partition);
        RWRETURN(eval);
    }

    if(!gMaterialBoundaryHints)
    {
        /* create some hints to cache material bounding boxes */
        gMaterialBoundaryHints = RtWorldImportHintsCreate();

        /* generate bounding boxes these have to be destroyed by the caller */
        RtWorldImportMaterialGroupHintGenerator(buildSector, gMaterialBoundaryHints);
    }

    /* first try to cull space */
    cullCost = RtWorldImportCullSpacePartitionSelector(buildSector,
                                    buildStatus, &candidate, &cullParams);

    if( (cullCost != rtWORLDIMPORTINVALIDPARTITION) &&
        !RtWorldImportDefaultPartitionTerminator(buildSector, buildStatus, NULL))
    {
        *partition = candidate;
        bestCost = 0;
    }
    else
    {
        /* store the old hints */
        oldHints = RtWorldImportHintsGetGroup(rtWORLDIMPORTPARTITIONHINT);
        RtWorldImportHintsSetGroup(gMaterialBoundaryHints,rtWORLDIMPORTPARTITIONHINT);

        while(RtWorldImportHintBBoxPartitionIterator(buildSector, buildStatus, &candidate, NULL, &count))
        {
            /* restore the user's hints */
            RtWorldImportHintsSetGroup(oldHints, 1);

            /* Calculate states */
            RtWorldImportSetPartitionStatistics(buildSector, &candidate);

            if( (candidate.buildStats.numMaterialLeft > 0) &&
                (candidate.buildStats.numMaterialRight > 0) )
            {
                AreaOfSector = _rtWorldImportBuildSectorAreaOfWall(&buildSector->boundingBox, candidate.type);

                occluderCost = RtWorldImportWeightedOccluderPartitionEvaluator(buildSector, buildStatus, &candidate, &AreaOfSector);

                cutCost = (RwReal)(candidate.buildStats.numMaterialSplits);

                separatorCost = RtWorldImportMaterialSeparatorPartitionEvaluator(
                                                buildSector, buildStatus, &candidate, NULL);

                totalCost = (separatorCost + cutCost) * RwRealAbs(occluderCost);

                /* test results */
                if(totalCost < bestSeparatorCost.separatorCost)
                {
                    /* test hints */
                    if(RtWorldImportHintBBoxPartitionEvaluator(buildSector, buildStatus, &candidate, NULL) !=
                        rtWORLDIMPORTINVALIDPARTITION)
                    {
                        bestSeparatorCost.separatorCost = totalCost;
                        bestSeparatorCost.cutCost = cutCost;
                        bestSeparatorCost.occluderCost = occluderCost;
                        bestSeparatorPartition = candidate;
                    }
                }
                else if(totalCost == bestSeparatorCost.separatorCost)
                {
                    /* test hints */
                    if(RtWorldImportHintBBoxPartitionEvaluator(buildSector, buildStatus, &candidate, NULL) !=
                        rtWORLDIMPORTINVALIDPARTITION)
                    {
                        if(occluderCost < bestSeparatorCost.occluderCost)
                        {
                            bestSeparatorCost.separatorCost = totalCost;
                            bestSeparatorCost.cutCost = cutCost;
                            bestSeparatorCost.occluderCost = occluderCost;
                            bestSeparatorPartition = candidate;
                        }
                    }
                }
            }

            /* set the material boundary hints again */
            RtWorldImportHintsSetGroup(gMaterialBoundaryHints,rtWORLDIMPORTPARTITIONHINT);
        }

        RtWorldImportSetPartitionStatistics(buildSector, &bestSeparatorPartition);

        if(!RtWorldImportDefaultPartitionTerminator(buildSector, buildStatus, NULL))
        {
            if(bestSeparatorPartition.buildStats.numMaterialSector > 1)
            {
                *partition = bestSeparatorPartition;
                bestCost = bestSeparatorCost.separatorCost;
            }
        }
        else /* only perfect partition at this point */
        {
            if((bestSeparatorCost.separatorCost < 0) && (bestSeparatorCost.cutCost == 0))
            {
                *partition = bestSeparatorPartition;
                bestCost = bestSeparatorCost.separatorCost;
            }
            else if((bestSeparatorCost.occluderCost < 0) && (bestSeparatorCost.cutCost == 0))
            {
                *partition = bestSeparatorPartition;
                bestCost = bestSeparatorCost.occluderCost;
            }
        }

        /* restore the user's hints */
        RtWorldImportHintsSetGroup(oldHints, 1);
    }

    if(bestCost < rtWORLDIMPORTINVALIDPARTITION)
    {
        RtWorldImportSetPartitionStatistics(buildSector, partition);
        RWRETURN(bestCost);
    }

    RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
}

/**
 * \ingroup selectors
 * \ref RtWorldImportPartitionHintPartitionSelector is new for RenderWare 3.4.
 * It uses partition hints and turns them into partitions. It also honors shield
 * hints.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition that is found.
 * \param userData     NULL.
 *
 * \return             The cost value of the partitioner.
 */
RwReal
RtWorldImportPartitionHintPartitionSelector(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus *buildStatus,
                                    RtWorldImportPartition *partition,
                                    void * userData __RWUNUSED__)
{
    RwReal bestCost = rtWORLDIMPORTINVALIDPARTITION;
    RwReal cullParams[2];
    RtWorldImportPartition candidate;
    RwInt32 count = 0;
    RwReal cost;
    RwReal cpShield = 1.0; /* cost per shield hint cut per shield strength unit */
    RwReal cpPower; /* value/importance of the Partition hint */

    RWAPIFUNCTION(RWSTRING("RtWorldImportPartitionHintPartitionSelector"));

    cullParams[0] = 0.15f; /* 15% */
    cullParams[1] = 0.05f; /* 05% */
/*
    if(!buildSector->numVertices)
    {
        RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
    }
*/
    while(RtWorldImportHintBBoxPartitionIterator(buildSector, buildStatus,
        &candidate, (void*)&cpPower, &count))
    {
        cost = RtWorldImportHintBBoxPartitionEvaluator(buildSector, buildStatus,
            &candidate, (void*)&cpShield);
        /* cost is now equal to sum of the strength of each "shield hint"
         * e.g. [50]   --cand--   [30] [50] [100] = 230 */

        /* now subtract the value of the "partition hint"
         * e.g. [50]   -{50}-   [30] [50] [100] = 230 - 50 = 180 */
        cost -= cpPower;

        if(cost < bestCost)
        {
            *partition = candidate;
            bestCost = cost;
        }
    }

    if(bestCost < rtWORLDIMPORTINVALIDPARTITION)
    {
        RtWorldImportSetPartitionStatistics(buildSector, partition);
        RWRETURN(bestCost);
    }

    RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
}


/**
 * \ingroup selectors
 * \ref RtWorldImportHintBBoxPartitionSelector partitions the scene using the faces
 * of the bounding boxes in the hints. It attempts to minimise cuts to other bounding
 * boxes.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition that is found.
 * \param userData     NULL.
 *
 * \return             The cost value of the partitioner.
 */
RwReal
RtWorldImportHintBBoxPartitionSelector(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus *buildStatus,
                                    RtWorldImportPartition *partition,
                                    void * userData __RWUNUSED__)
{
    RwReal bestCost = rtWORLDIMPORTINVALIDPARTITION;
    RwReal cost;
    RwReal costPerCut = ((RwReal)1);
    RtWorldImportPartition candidate;
    RwInt32 count = 0;

    RWAPIFUNCTION(RWSTRING("RtWorldImportHintBBoxPartitionSelector"));

    while(RtWorldImportHintBBoxPartitionIterator(buildSector, buildStatus, &candidate, NULL, &count))
    {
        cost = RtWorldImportHintBBoxPartitionEvaluator(buildSector, buildStatus, &candidate, &costPerCut);

        if(cost < bestCost)
        {
            *partition = candidate;
            bestCost = cost;
        }
    }

    if(bestCost < rtWORLDIMPORTINVALIDPARTITION)
    {
        RtWorldImportSetPartitionStatistics(buildSector, partition);
        RWRETURN(bestCost);
    }

    RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
}

/* _rtWorldImportGuidedKDPartitionSelector
 *
 * Force splits to follow encoded structure in params - a stack based
 * traversal of the kd-tree. Data is copied from head of stack - this
 * should correspond to the correct node assuming an underlying inorder
 * traversal.
 */

RwReal
_rtWorldImportGuidedKDPartitionSelector(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus * buildStatus __RWUNUSED__,
                                    RtWorldImportPartition *partition,
                                    void *userData)
{
    RwReal partitionCost;
    _rtWorldImportGuideKDStack *stack;
    RtWorldImportParameters *conversionParams = RtWorldImportParametersGet();

    RWFUNCTION(RWSTRING("_rtWorldImportGuidedKDPartitionSelector"));

    stack = (_rtWorldImportGuideKDStack*)userData;

    if(stack->current)
    {
        /* keep track of the extra children node */
        if(childrenCount > 0)
        {
            childrenCount--;

            /* is the sector valid? */
            if(!RtWorldImportDefaultPartitionTerminator(buildSector, NULL, NULL))
            {
                /* NO! we need to partition to make sure the world is valid */
                partitionCost = RtWorldImportBalancedTreePartitionSelector(
                    buildSector, buildStatus, partition, NULL);
                childrenCount += 2;
                RWRETURN(partitionCost);
            }

            if(childrenCount <= 0) /* the last child pop */
            {
                /* Pop */
                 _rtWorldImportGuideKDPop(stack);
                childrenCount = 0;
                conversionParams->terminatorCheck = FALSE;
            }

            RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
        }

        /* a leaf node */
        if(stack->current->terminal)
        {
            /* is the sector valid? */
            if(!RtWorldImportDefaultPartitionTerminator(buildSector, NULL, NULL))
            {
                /* NO! we need to partition to make sure the world is valid */
                partitionCost = RtWorldImportBalancedTreePartitionSelector(
                    buildSector, buildStatus, partition, NULL);
                childrenCount = 2;
                conversionParams->terminatorCheck = TRUE;
                RWRETURN(partitionCost);
            }

            /* Pop */
             _rtWorldImportGuideKDPop(stack);
            RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
        }
        else
        {
            partition->type = stack->current->node->partition.type;
            partition->value = stack->current->node->partition.value;

            partition->maxLeftValue = stack->current->node->partition.maxLeftValue;
            partition->maxRightValue = stack->current->node->partition.maxRightValue;

            RtWorldImportSetPartitionStatistics(buildSector, partition);

            stack->current->node->partition.buildStats = partition->buildStats;

            /* Pop */
            _rtWorldImportGuideKDPop(stack);

            RWRETURN(1.0f);
        }
    }

    stack->current = stack->top; /* reset for next traversal */
    RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
}

RwReal
_rtWorldImportFuzzyBalancedPartitionSelector(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus *buildStatus,
                                    RtWorldImportPartition *partition,
                                    void *userData)
{
    RwReal bestCost = RwRealMAXVAL;
    RwReal cost;
    RwInt32 loopCount = 0;
    RtWorldImportPartition candidate;

    RWFUNCTION(RWSTRING("_rtWorldImportFuzzyBalancedPartitionSelector"));

    if((!buildSector->numVertices) || (buildStatus->depth >= 32))
    {
        RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
    }

    /* iterate all candidate planes */
    while(RtWorldImportMedianPercentagePartitionIterator(buildSector,
            buildStatus,  &candidate, userData, &loopCount))
    {
        RtWorldImportSetPartitionStatistics(buildSector, &candidate);
        cost = RtWorldImportFuzzyBalancedPartitionEvaluator(buildSector,
                    buildStatus,  &candidate, userData);

        /* is this the best partition so far? */
        if (cost < bestCost)
        {
            /* test hints */
            if(RtWorldImportHintBBoxPartitionEvaluator(buildSector, buildStatus, &candidate, NULL) !=
                rtWORLDIMPORTINVALIDPARTITION)
            {
                *partition = candidate;
                bestCost = cost;
            }
        }
    }

    /* Check if a suitable plane was found */
    if (bestCost < rtWORLDIMPORTINVALIDPARTITION)
        RtWorldImportSetPartitionStatistics(buildSector, partition);

    RWRETURN(bestCost);
}

/**
 * \ingroup selectors
 * \ref RtWorldImportBalancedCullPartitionSelector is a composite partition
 * selector that attempts to cull most space, and balance the BSP whilst
 * minimizing the polygonal and material splits in the scene.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition that is found.
 * \param userData     NULL.
 *
 * \return The cost value of the partitioner.
 */
RwReal
RtWorldImportBalancedCullPartitionSelector(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus *buildStatus,
                                    RtWorldImportPartition *partition,
                                    void *userData)
{
    RwReal cost;
    RwReal cullParams[2];
    RwReal eval;

    RWAPIFUNCTION(RWSTRING("RtWorldImportBalancedCullPartitionSelector"));

    if((!buildSector->numVertices) || (buildStatus->depth >= 32))
    {
        RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
    }

    cullParams[0] = 0.15f; /* 15% */
    cullParams[1] = 0.05f; /* 5% */

    /* Can we find a partition that will cull a slab of space from the sector? */
    if((cost = RtWorldImportCullSpacePartitionSelector(buildSector,
        buildStatus,  partition, &cullParams)) == rtWORLDIMPORTINVALIDPARTITION)
    {
        /* Can we stop now */
        if(!RtWorldImportDefaultPartitionTerminator(buildSector, buildStatus, NULL))
        {
            /* No... */
            /* Now see if there are any partition hints
             */
            eval = RtWorldImportPartitionHintPartitionSelector(buildSector,
                buildStatus,  partition, NULL);
            if (eval!=rtWORLDIMPORTINVALIDPARTITION)
            {
                /* *partition = candidate; */
                RtWorldImportSetPartitionStatistics(buildSector, partition);
                RWRETURN(eval);
            }

            /* Lets use the fuzzy balanced partition selector thingy */
            if((cost = _rtWorldImportFuzzyBalancedPartitionSelector(buildSector,
                    buildStatus,  partition, userData)) == rtWORLDIMPORTINVALIDPARTITION)
            {
                /* No? lets use that maximum middle axis partition selector */
                cost = RtWorldImportMaximumExtentPartitionSelector(buildSector, buildStatus,
                                                                 partition, NULL);
            }
        }
    }

    if (cost < rtWORLDIMPORTINVALIDPARTITION)
        RtWorldImportSetPartitionStatistics(buildSector, partition);

    RWRETURN(cost);
}

/**
 * \ingroup selectors
 * \ref RtWorldImportMaximumOccluderPartitionSelector selects the partition
 * that covers most polygons in the scene, thus likely to be maximum occluder.
 * This is good for (orthogonal) high-occlusion indoor scenes.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition that is found.
 * \param userData     NULL.
 *
 * \return The cost value of the partitioner.
 *
 * \see RtWorldImportGeneralOccluderPartitionSelector
 * \see RtWorldImportDisjointOccluderPartitionSelector
 */
RwReal
RtWorldImportMaximumOccluderPartitionSelector(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus *buildStatus,
                                    RtWorldImportPartition *partition,
                                    void * userData)
{
    RwReal bestvalue = rtWORLDIMPORTINVALIDPARTITION, eval;
    RtWorldImportPartition candidate;
    RwInt32 loopCount = 0;
    RwInt32 lastType = 0;
    RwReal lastValue = RwRealMAXVAL;
    RwReal AreaOfSector;
    RwBBox bbpOut;
    RwReal cullParams[2];

    RWAPIFUNCTION(RWSTRING("RtWorldImportMaximumOccluderPartitionSelector"));

    cullParams[0] = 0.50f;
    cullParams[1] = 0.16f;

    if((!buildSector->numVertices) || (buildStatus->depth >= 32))
    {
        RWRETURN(rtWORLDIMPORTINVALIDPARTITION);
    }

    /* Get rid of space, before finding occluders!...
     */
    if(RtWorldImportCullSpacePartitionSelector(buildSector,
        buildStatus,  &candidate, &cullParams)!=rtWORLDIMPORTINVALIDPARTITION)
    {
        *partition = candidate;
        RtWorldImportSetPartitionStatistics(buildSector, partition);
        RWRETURN(-2.0f);
    }

    /* Now see if there are any partition hints
     */
    eval = RtWorldImportPartitionHintPartitionSelector(buildSector,
        buildStatus,  &candidate, NULL);
    if (eval!=rtWORLDIMPORTINVALIDPARTITION)
    {
        *partition = candidate;
        RtWorldImportSetPartitionStatistics(buildSector, partition);
        RWRETURN(eval);
    }

    if(!RtWorldImportDefaultPartitionTerminator(buildSector, buildStatus, NULL))
    {


        bbpOut = buildSector->boundingBox;
        /*_rtImportBuildSectorFindBBox(buildSector, &bbpOut); */


        /* All space removed, try and find an occluder */
        while (RtWorldImportOrthogonalAutoPartitionIterator(buildSector,
                                            buildStatus,
                                            &candidate,
                                            userData,
                                            &loopCount))
        {
            if (!((candidate.type == lastType) && (candidate.value == lastValue)))
            {
                /* This candidate hasn't been tried before (recently) */
                lastType = candidate.type;
                lastValue = candidate.value;

                AreaOfSector = _rtWorldImportBuildSectorAreaOfWall(&bbpOut, candidate.type);

                eval = RtWorldImportWeightedOccluderPartitionEvaluator(buildSector, buildStatus,
                    &candidate,
                    (void*)&AreaOfSector);

                if (eval < bestvalue)
                {
                    /* test hints */
                    if(RtWorldImportHintBBoxPartitionEvaluator(buildSector, buildStatus, &candidate, NULL) !=
                        rtWORLDIMPORTINVALIDPARTITION)
                    {
                        *partition = candidate;
                        bestvalue = eval;
                    }
                }
            }
        }

    }

    if (bestvalue < rtWORLDIMPORTINVALIDPARTITION)
        RtWorldImportSetPartitionStatistics(buildSector, partition);

    RWRETURN(bestvalue);
}


/**
 * \ingroup selectors
 * \ref RtWorldImportDisjointOccluderPartitionSelector selects the partition
 * that is a good occluder, or is likely to put a cap or boundary along transitions
 * of high visibility change - such as the roof of a building.
 * This is good for (orthogonal) medium-occlusion outdoor scenes and city-scapes.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition that is found.
 * \param userData     NULL.
 *
 * \return The cost value of the partitioner.
 *
 * \see RtWorldImportGeneralOccluderPartitionSelector
 * \see RtWorldImportMaximumOccluderPartitionSelector
 */
RwReal
RtWorldImportDisjointOccluderPartitionSelector(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus *buildStatus,
                                    RtWorldImportPartition *partition,
                                    void * userData __RWUNUSED__)
{
    RwReal bestvalue = rtWORLDIMPORTINVALIDPARTITION, eval;
    RtWorldImportPartition candidate;
    RwInt32 loopCount = 0;
    RwInt32 xyz;
    RwReal minCullSpace = 0.125;
    searchRange ho;
    RwReal range = (RwReal)0;
    RwReal high;
    RwReal yaxisLength;
    RwReal AreaOfSector;
    RwReal segments = 10.0f;
    RwV3d vSize;
    RwReal cullParams[2];

    RWAPIFUNCTION(RWSTRING("RtWorldImportDisjointOccluderPartitionSelector"));

    cullParams[0] = cullParams[1] = minCullSpace;

    yaxisLength = buildSector->boundingBox.sup.y - buildSector->boundingBox.inf.y;

    /* Get rid of space, this will fix bad art work... */
    if(RtWorldImportCullSpacePartitionSelector(buildSector,
        buildStatus,  &candidate, &cullParams)!=rtWORLDIMPORTINVALIDPARTITION)
    {
        *partition = candidate;
        RtWorldImportSetPartitionStatistics(buildSector, partition);
        RWRETURN(1.0f);
    }

    /* Now see if there are any partition hints
     */
    eval = RtWorldImportPartitionHintPartitionSelector(buildSector,
        buildStatus,  &candidate, NULL);
    if (eval!=rtWORLDIMPORTINVALIDPARTITION)
    {
        *partition = candidate;
        RtWorldImportSetPartitionStatistics(buildSector, partition);
        RWRETURN(eval);
    }


    while ((bestvalue == rtWORLDIMPORTINVALIDPARTITION) && (range < yaxisLength))
    {
        xyz = 4; /* find a roof to use as a partition */


        while (xyz<=8)
        {
            RwV3dSub(&vSize, &buildSector->boundingBox.sup,
                         &buildSector->boundingBox.inf);

            /* Skip this aspect if it would result in a bad split */
            if (RwV3dLength(&vSize) / GETCOORD(vSize, xyz) < 10.0)
            {
                loopCount = 0;
                high = FindHighestVertex(buildSector, buildStatus);
                ho.hold = xyz;


                ho.high = (high - range) + (yaxisLength / segments);
                ho.low = (high - range);


                AreaOfSector = _rtWorldImportBuildSectorAreaOfWall(&buildSector->boundingBox, candidate.type);

                /* Try and find a horizontal top (4) or vertical face (0,8) in the high part
                 * of the scene */
                while (RtWorldImportHighestVertexAccommodatingPlanePartitionIterator(buildSector,
                                                    buildStatus,
                                                    &candidate,
                                                    (void*)&ho,
                                                    &loopCount))
                {
                    eval = RtWorldImportWeightedDisjointOccluderPartitionEvaluator(buildSector, buildStatus,
                        &candidate,
                        (void*)&AreaOfSector);


                    if (eval < bestvalue)
                    {
                        /* test hints */
                        if(RtWorldImportHintBBoxPartitionEvaluator(buildSector, buildStatus, &candidate, NULL) !=
                            rtWORLDIMPORTINVALIDPARTITION)
                        {
                            *partition = candidate;
                            bestvalue = eval;
                        }
                    }
                }
            }
            if ((xyz==4) && (bestvalue < rtWORLDIMPORTINVALIDPARTITION)) break; /* found a good top */

            /* (4),8,0 - find a wall as a partition */
            xyz+=4;
            if (xyz==12) xyz=0;
            else if (xyz==4) xyz=12;
        }
        range += yaxisLength / segments;
    }

    if (bestvalue < rtWORLDIMPORTINVALIDPARTITION)
        RtWorldImportSetPartitionStatistics(buildSector, partition);

    RWRETURN(bestvalue);
}

/**
 * \ingroup selectors
 * \ref RtWorldImportGeneralOccluderPartitionSelector selects either an indoor-type
 * or outdoor-type partition as appropriate from
 * \ref RtWorldImportMaximumOccluderPartitionSelector
 * and \ref RtWorldImportDisjointOccluderPartitionSelector respectively.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition that is found.
 * \param userData     A pointer to an \ref RwReal that is the 'outdoor size' -
 * the preferred minumum outdoor sector size - that dividing
 * the outdoor sector below this value returns rtWORLDIMPORTINVALIDPARTITION, and thus attempts
 * a termination subsequently.
 *
 * \return The cost value of the partitioner.
 *
 * \see RtWorldImportMaximumOccluderPartitionSelector
 * \see RtWorldImportDisjointOccluderPartitionSelector
 */
RwReal
RtWorldImportGeneralOccluderPartitionSelector(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus *buildStatus,
                                    RtWorldImportPartition *partition,
                                    void * userData)
{
    RtWorldImportPartition candidate;
    RwReal cost = rtWORLDIMPORTINVALIDPARTITION;

    RWAPIFUNCTION(RWSTRING("RtWorldImportGeneralOccluderPartitionSelector"));

    if (buildStatus->depth > 5)
    {
        /* try to find a good maximum occluder */
        cost = RtWorldImportMaximumOccluderPartitionSelector(buildSector,
            buildStatus, &candidate, userData);
    }

    if (cost > -0.1f)
    {
        /* Ok, so indoor failed - let's see if we can terminate very early */
        if ((buildStatus->depth > 5) &&
            (RtWorldImportSizePartitionTerminator(buildSector,
                                    buildStatus,
                                    userData)))
            RWRETURN(rtWORLDIMPORTINVALIDPARTITION);

        /* try to find good outdoor occluder */
        cost = RtWorldImportDisjointOccluderPartitionSelector(buildSector,
            buildStatus, &candidate, userData);
    }

    if (cost < rtWORLDIMPORTINVALIDPARTITION)
    {
        /* found good one */
        *partition = candidate;
        RtWorldImportSetPartitionStatistics(buildSector, partition);
    }

    RWRETURN(cost);
}

/**
 * \ingroup selectors
 * \ref RtWorldImportLeastCutAndBalancedPartitionSelector selects the partition
 * that is cut a relatively small number of times whilst being balanced in terms of
 * polygons on either side - ultimately, this is all to keep splits to a minimum.
 *
 * NB: This scheme does NOT honor "Partition hints"
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition that is found.
 * \param userData     NULL.
 *
 * \return The cost value of the partitioner.
 */
RwReal
RtWorldImportLeastCutAndBalancedPartitionSelector(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus *buildStatus,
                                    RtWorldImportPartition *partition,
                                    void * userData)
{
    RwReal bestvalue = RwRealMAXVAL, eval;
    RtWorldImportPartition candidate;
    RwInt32 loopCount = 0;
    RwInt32 loopCount2 = 0;

    RWAPIFUNCTION(RWSTRING("RtWorldImportLeastCutAndBalancedPartitionSelector"));

    while ((RtWorldImportRegularIntervalPartitionIterator(buildSector,
                                        buildStatus,
                                        &candidate,
                                        userData,
                                        &loopCount)) ||
            (RtWorldImportMiddleAxisPartitionIterator(buildSector,
                                        buildStatus,
                                        &candidate,
                                        userData,
                                        &loopCount2)))
    {
        RtWorldImportSetPartitionStatistics(buildSector, &candidate);

        eval = RtWorldImportPotentialSplitPartitionEvaluator(buildSector, buildStatus,
                &candidate,
                userData);

        eval += RtWorldImportBalancedPartitionEvaluator(buildSector, buildStatus,
                &candidate,
                userData);

        if (eval < bestvalue)
        {
            /* test hints */
            if(RtWorldImportHintBBoxPartitionEvaluator(buildSector, buildStatus, &candidate, NULL) !=
                rtWORLDIMPORTINVALIDPARTITION)
            {
                *partition = candidate;
                bestvalue = eval;
            }
        }
    }
    if (bestvalue < rtWORLDIMPORTINVALIDPARTITION)
        RtWorldImportSetPartitionStatistics(buildSector, partition);

    RWRETURN(bestvalue);
}

/**
 * \ingroup selectors
 * \ref RtWorldImportMaximumExtentPartitionSelector selects the partition
 * that divides the sector in half as to minimize the aspect difference.
 *
 * NB: This scheme does NOT honor "Partition hints"
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition that is found.
 * \param userData     TRUE if it should honor hints, FALSE otherwise.
 *
 * \return The cost value of the partitioner.
 */
RwReal
RtWorldImportMaximumExtentPartitionSelector(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus *buildStatus,
                                    RtWorldImportPartition *partition,
                                    void * userData)
{
    RwReal bestvalue = RwRealMAXVAL, eval;
    RtWorldImportPartition candidate;
    RwInt32 loopCount = 0;
    RwBool ignoreHints = FALSE;
#if (0)
    RtWorldImportParameters *conversionParams = RtWorldImportParametersGet();
#endif /* (0) */

    RWAPIFUNCTION(RWSTRING("RtWorldImportMaximumExtentPartitionSelector"));

    if(userData) ignoreHints = *((RwBool*)userData);

    while (RtWorldImportMiddleAxisPartitionIterator(buildSector,
                                        buildStatus,
                                        &candidate,
                                        userData,
                                        &loopCount))
    {
        eval = RtWorldImportExtentPartitionEvaluator(buildSector, buildStatus,
                &candidate,
                userData);

        if (eval < bestvalue)
        {
            /* test hints */
            if( ignoreHints ||
                ((RtWorldImportHintBBoxPartitionEvaluator(buildSector, buildStatus, &candidate, NULL) !=
                rtWORLDIMPORTINVALIDPARTITION)) )
            {
                *partition = candidate;
                bestvalue = eval;
            }
        }
    }

    if (bestvalue < rtWORLDIMPORTINVALIDPARTITION)
    {
        RtWorldImportSetPartitionStatistics(buildSector, partition);
    }

    RWRETURN(bestvalue);
}

/**
 * \ingroup selectors
 * \ref RtWorldImportBalancedTreePartitionSelector selects the partition
 * that divides the world roughly in half (with respect to the geometry)
 * and minimizes the aspect difference. I.e. producing a balanced scene and tree.
 *
 * \note This partition selector does NOT honor hints, but is guaranteed to always
 * find a partition.
 *
 * \param buildSector  A pointer to the Build Sector.
 * \param buildStatus  A pointer to the Build Status.
 * \param partition    A pointer to the partition that is found.
 * \param userData     NULL.
 *
 * \return The cost value of the partitioner.
 */
RwReal
RtWorldImportBalancedTreePartitionSelector(RtWorldImportBuildSector *buildSector,
                                    RtWorldImportBuildStatus * buildStatus,
                                    RtWorldImportPartition *partition,
                                    void * userData __RWUNUSED__)
{
    /* creates a balanced tree - does not honor hints!!! */
    RwReal bestValue = rtWORLDIMPORTINVALIDPARTITION;
    /*const*/RwReal eval = (RwReal)0;
    RtWorldImportPartition candidate;
    RwInt32 loopCount = 0;
    RwV3d vSize;
    RwInt32 maxExtent = 0;
    RwBool ignoreHints = TRUE;

    RWAPIFUNCTION(RWSTRING("RtWorldImportBalancedTreePartitionSelector"));

    RwV3dSub(&vSize, &buildSector->boundingBox.sup,
                 &buildSector->boundingBox.inf);

    if(buildSector->numVertices)
    {
        if (vSize.x > vSize.y)
        {
            if (vSize.x > vSize.z)
            {
                maxExtent = 0;
            }
            else
            {
                maxExtent = 8;
            }
        }
        else
        {
            if (vSize.y > vSize.z)
            {
                maxExtent = 4;
            }
            else
            {
                maxExtent = 8;
            }
        }
        while (ImportCentralMaximumExtentPartitionIterator(buildSector,
                                            buildStatus,
                                            &candidate,
                                            (void*)&maxExtent,
                                            &loopCount))
        {
            eval = RtWorldImportBalancedPartitionEvaluator(buildSector,
                                                buildStatus,
                                                &candidate,
                                                NULL);
            if (eval < bestValue)
            {
                *partition = candidate;
                bestValue = eval;
            }
        }
    }

    if(bestValue >= (RwReal)1)
    {
        /* we have to make sure a partition is selected */
        bestValue = RtWorldImportMaximumExtentPartitionSelector(
            buildSector, buildStatus, partition, &ignoreHints);
    }

    RtWorldImportSetPartitionStatistics(buildSector, partition);

    RWASSERT(bestValue != rtWORLDIMPORTINVALIDPARTITION);
    RWRETURN(bestValue);
}


/********************************************************************************
 * CONTROLLER...
 */
/**
 * \ingroup selectors
 * \ref RtWorldImportSetStandardBuildPartitionSelector is called to determine
 * whether partitioning will be achieved automatically, using the
 * default partition selected, or manually.
 *
 * \param partitionSelector     \see RtWorldImportBuildPartitionSelector
 * \param userData              For a manual build, a pointer to a
 *                              \ref RtWorldImportGuideKDTree; otherwise
 *                              a pointer to an \ref RwInt32 that is the number
 *                              candidate partitions that are tested
 *
 * \see RtWorldImportSetBuildCallBacks
 * \see RtWorldImportSetBuildCallBacksUserData
 */
void
RtWorldImportSetStandardBuildPartitionSelector(RtWorldImportBuildPartitionSelector partitionSelector,
                                    void* userData)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportSetStandardBuildPartitionSelector"));

    switch(partitionSelector)
    {
        case rwBUILDPARTITIONSELECTOR_GUIDED:
        {
            RtWorldImportGuideKDTree *myKD = (RtWorldImportGuideKDTree*)userData;

            if (KDStack.top!=NULL)
            {
                /* clean up the contents of KD stack
                 * (Should never happen, but could if user calls this function and
                 *  then never bothers to build the world afterall, before calling
                 *  this function again.)
                 */
                _rtWorldImportGuideKDStackDestroy(&KDStack);
            }
            KDStack.bottom = KDStack.top = NULL;
            _rtWorldImportGuideKDEncodeAsStack(myKD, &KDStack);
            KDStack.current = KDStack.top;

            RtWorldImportSetBuildCallBacks(
                _rtWorldImportGuidedKDPartitionSelector,
                _rtWorldImportGuidedKDStackPartitionTerminator);

            RtWorldImportSetBuildCallBacksUserData(
                (void *)&KDStack,
                (void *)&KDStack);
            break;
        }
        case rwBUILDPARTITIONSELECTOR_DEFAULT:
        default:
        {
            RtWorldImportSetBuildCallBacks(
                RtWorldImportBalancedCullPartitionSelector,
                RtWorldImportNeverPartitionTerminator);

            RtWorldImportSetBuildCallBacksUserData(
                (void *)userData,
                (void *)NULL);
        }
    }
    RWRETURNVOID();
}

