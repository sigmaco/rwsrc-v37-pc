/*
 * Functionality for Splines
 *
 * Copyright (c) Criterion Software Limited
 */

/**
 * \ingroup rpspline
 * \page rpsplineoverview RpSpline Plugin Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rpspline.h
 * \li \b Libraries: rwcore, rpworld, rpspline
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpSplinePluginAttach
 *
 * \subsection splineoverview Overview
 * The RpSpline plugin provides a number of useful API extensions to support
 * the use and manipulation of interpolating cubic uniform B-Spline curves.
 *
 * Functions provided allow you to create splines and store them in RpSpline
 * datatypes.
 * These splines can then be used within your application.
 * Supporting functions include the ability to adjust the control points,
 * find a location along a spline and generate Frenet matrices.
 *
 * For more background on cubic uniform B-Splines, see the page 90 of the book:
 * "Advanced Animation and Rendering Techniques",
 * Watt and Watt,
 * Addison Wesley,
 * ISBN 0-201-54412-1.
 * http://www.awlonline.com/productpage/?ISBN=0201544121
 */

/***************************************************************************
 *                                                                         *
 * Module  :    rpspline.c                                                 *
 *                                                                         *
 * Purpose :    Spline operations                                          *
 *                                                                         *
 **************************************************************************/

/*
 *     Synopsis:             Spline Support For 3d library
 */

/****************************************************************************
 Includes
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Include this files header */

#include "rpplugin.h"
#include "rpdbgerr.h"
#include "rpspline.h"

/****************************************************************************
 Local Types
 */

/*
 * Binary Representation
 */

typedef struct _rpSpline _rpSpline;
struct _rpSpline
{
    RwChar              name[rpSPLINENAMELENGTH];
    RwInt32             numVertices;
    RwInt32             splineType;
};

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */

#ifndef min
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif /* min */

#define splSIGNIFICANTPOINTS        (10) /* Number of points considered */
#define splWEIGHTRES                (256) /* Resolution in the weightings */

#define splLINEARINTERPOLATE(start,weight,stop)             \
 ((((start)) + ( (((weight)) * ( (((stop)) - ( (start))))))))

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */

#if (defined(RWDEBUG))
long                rpSplineStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

static RwModuleInfo splineModule;

/* These are all lookup tables common to all instances of RenderWare */
static RwReal      *rpGBsplinePosition0 = (RwReal *)NULL;
static RwReal      *rpGBsplinePosition1 = (RwReal *)NULL;
static RwReal      *rpGBsplinePosition2 = (RwReal *)NULL;
static RwReal      *rpGBsplinePosition3 = (RwReal *)NULL;

static RwReal      *rpGBsplineVelocity0 = (RwReal *)NULL;
static RwReal      *rpGBsplineVelocity1 = (RwReal *)NULL;
static RwReal      *rpGBsplineVelocity2 = (RwReal *)NULL;
static RwReal      *rpGBsplineVelocity3 = (RwReal *)NULL;

static RwReal      *rpGBsplineAcceleration0 = (RwReal *)NULL;
static RwReal      *rpGBsplineAcceleration1 = (RwReal *)NULL;
static RwReal      *rpGBsplineAcceleration2 = (RwReal *)NULL;
static RwReal      *rpGBsplineAcceleration3 = (RwReal *)NULL;

static RwReal      *rapGTable[splSIGNIFICANTPOINTS + 1];

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                        Scaling vectors

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

#define addScaledVector(dest, srce, scale)      \
MACRO_START                                     \
{                                               \
    (dest)->x += (srce)->x * (scale);           \
    (dest)->y += (srce)->y * (scale);           \
    (dest)->z += (srce)->z * (scale);           \
}                                               \
MACRO_STOP

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                    Setting up the spline library

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/****************************************************************************
 _splSetBSpline

 Set up the basis for the B-splines.
 Done only once from RpSplineOpen(). Uses floating point for range.

 On entry : void
 On exit  : RwBool      TRUE if successful
****************************************************************************/
static              RwBool
_splSetBspline(void)
{
    RwReal              rT, rSample;
    RwInt32             nI;

    RWFUNCTION(RWSTRING("_splSetBspline"));

    rpGBsplinePosition0 = (RwReal *)
        RwMalloc(sizeof(RwReal) * 12 * (splWEIGHTRES + 1),
            rwID_SPLINEPLUGIN | rwMEMHINTDUR_GLOBAL);
    if (rpGBsplinePosition0)
    {
        rpGBsplinePosition1 = &rpGBsplinePosition0[splWEIGHTRES + 1];
        rpGBsplinePosition2 = &rpGBsplinePosition1[splWEIGHTRES + 1];
        rpGBsplinePosition3 = &rpGBsplinePosition2[splWEIGHTRES + 1];

        rpGBsplineVelocity0 = &rpGBsplinePosition3[splWEIGHTRES + 1];
        rpGBsplineVelocity1 = &rpGBsplineVelocity0[splWEIGHTRES + 1];
        rpGBsplineVelocity2 = &rpGBsplineVelocity1[splWEIGHTRES + 1];
        rpGBsplineVelocity3 = &rpGBsplineVelocity2[splWEIGHTRES + 1];

        rpGBsplineAcceleration0 =
            &rpGBsplineVelocity3[splWEIGHTRES + 1];
        rpGBsplineAcceleration1 =
            &rpGBsplineAcceleration0[splWEIGHTRES + 1];
        rpGBsplineAcceleration2 =
            &rpGBsplineAcceleration1[splWEIGHTRES + 1];
        rpGBsplineAcceleration3 =
            &rpGBsplineAcceleration2[splWEIGHTRES + 1];

        for (nI = 0; nI <= splWEIGHTRES; ++nI)
        {
            rT = (RwReal) ((RwReal) nI / (RwReal) splWEIGHTRES);

            /*
             * splWEIGHTRES samples from Uniform B-Spline basis cubics
             * evalulated with Horner's rule.
             *
             * Reference: "Advanced Animation and Rendering Techniques" p90
             *             Watt and Watt
             *             Adison Wesley
             *             ISBN 0-201-54412-1
             *
             * B-spline has local Matrix formulation:
             *               T
             * Q_i(u) = U M P
             * where
             *
             * U = [u^3, u^2, u, 1]
             *
             * M = [ [ -1, 3,  -3, 1 ]
             *       [ 3, -6,  3,  0 ]
             *       [ -3, 0,  3,  0 ]
             *       [ 1,  4,  1,  0 ] ] * 1/6
             *
             * P = [p_i, p_(i + 1), p_(i + 2), p_(i + 3)]
             *
             */

            rSample = (RwReal)
                ((((1.0f * rT + 0.0f) * rT + 0.0f) * rT + 0.0f) / 6.0f);
            rpGBsplinePosition0[nI] = rSample;

            rSample = (RwReal)
                ((((-3.0f * rT + 3.0f) * rT + 3.0f) * rT +
                  1.0f) / 6.0f);
            rpGBsplinePosition1[nI] = rSample;

            rSample = (RwReal)
                ((((3.0f * rT - 6.0f) * rT + 0.0f) * rT + 4.0f) / 6.0f);
            rpGBsplinePosition2[nI] = rSample;

            rSample = (RwReal)
                ((((-1.0f * rT + 3.0f) * rT - 3.0f) * rT +
                  1.0f) / 6.0f);
            rpGBsplinePosition3[nI] = rSample;

            /* N.b.
             *  Velocity Matrix is
             *  [ [ 0,  0,  0,  0 ]
             *    [ -1, 3,  -3, 1 ]
             *    [ 2,  -4, 2,  0 ]
             *    [ -1, 0,  1,  0 ] ] * 1/2
             */

            rSample =
                (RwReal) (((1.0f * rT + 0.0f) * rT + 0.0f) / 2.0f);
            rpGBsplineVelocity0[nI] = rSample;

            rSample =
                (RwReal) (((-3.0f * rT + 2.0f) * rT + 1.0f) / 2.0f);
            rpGBsplineVelocity1[nI] = rSample;

            rSample =
                (RwReal) (((3.0f * rT + -4.0f) * rT + 0.0f) / 2.0f);
            rpGBsplineVelocity2[nI] = rSample;

            rSample =
                (RwReal) (((-1.0f * rT + 2.0f) * rT - 1.0f) / 2.0f);
            rpGBsplineVelocity3[nI] = rSample;

            /* N.b.
             *  Acceleration Matrix is
             *  [ [  0,  0,  0,  0 ]
             *    [  0,  0,  0,  0 ]
             *    [ -1,  3, -3,  1 ]
             *    [  1, -2,  1,  0 ] ]
             */

            rSample = (RwReal) (1.0f * rT + 0.0f);
            rpGBsplineAcceleration0[nI] = rSample;

            rSample = (RwReal) (-3.0f * rT + 1.0f);
            rpGBsplineAcceleration1[nI] = rSample;

            rSample = (RwReal) (3.0f * rT - 2.0f);
            rpGBsplineAcceleration2[nI] = rSample;

            rSample = (RwReal) (-1.0f * rT + 1.0f);
            rpGBsplineAcceleration3[nI] = rSample;
        }
    }
    else
    {
        RWERROR((E_RW_NOMEM,
                 (sizeof(RwReal) * 12 * (splWEIGHTRES + 1))));
    }

    RWRETURN(rpGBsplinePosition0 != NULL);

}

/****************************************************************************
 _splBuildSeries

 Returns an array of specified length, containing a series implicitely
 defined by the rule:
    first, second, 4 * second - first, 4 * third - second, ...

 On entry : RwInt32     nFirst
            RwInt32     nSecond
            RwInt32     nLength      length of the required array

 On exit  : RwInt32*    the array containing the series
                        or NULL if failed
****************************************************************************/
static RwInt32     *
_splBuildSeries(RwInt32 nFirst, RwInt32 nSecond, RwInt32 nLength)
{
    RwInt32             nI, *npSeries;

    RWFUNCTION(RWSTRING("_splBuildSeries"));

    npSeries = (RwInt32 *) RwMalloc(sizeof(RwInt32) * nLength,
                        rwID_SPLINEPLUGIN | rwMEMHINTDUR_EVENT);

    if (!npSeries)
    {
        RWERROR((E_RW_NOMEM, (sizeof(RwInt32) * nLength)));
        RWRETURN((RwInt32 *)NULL);
    }

    npSeries[0] = nFirst;
    npSeries[1] = nSecond;

    for (nI = 2; nI < nLength; ++nI)
    {
        npSeries[nI] = (4 * npSeries[nI - 1]) - npSeries[nI - 2];
    }

    RWRETURN(npSeries);
}

/****************************************************************************
 _splBuildCoefficientMatrix

 Build the coefficient matrix for generating phantom spline points
 for an open loop.
 Called only once at startup from _splSetCoefficients.

 On entry : RwInt32     nDimension       dimension of the required matrix
 On exit  : RwReal*     the coefficient matrix
****************************************************************************/
static RwReal      *
_splBuildCoefficientMatrix(RwInt32 nDimension)
{
    RwInt32            *npSeries, nDivisor, nMultiplier, nI, nJ;
    RwReal             *rpCoefficient;

    RWFUNCTION(RWSTRING("_splBuildCoefficientMatrix"));

    rpCoefficient = (RwReal *)
        RwMalloc(sizeof(RwReal) * nDimension * nDimension,
                 rwID_SPLINEPLUGIN | rwMEMHINTDUR_EVENT);

    if (!rpCoefficient)
    {
        RWERROR((E_RW_NOMEM,
                 (sizeof(RwReal) * nDimension * nDimension)));
        RWRETURN((RwReal *)NULL);
    }

    /* Starting at co, set to zero, length third parameter */
    memset((void *) rpCoefficient, 0,
           sizeof(RwReal) * (unsigned int) (nDimension * nDimension));

    npSeries = _splBuildSeries(1, 4, nDimension - 1);

    if (!npSeries)
    {
        RwFree(rpCoefficient);
        rpCoefficient = (RwReal *)NULL;
        RWRETURN((RwReal *)NULL);
    }

    nDivisor = npSeries[nDimension - 2];
    RwFree(npSeries);
    npSeries = (RwInt32 *)NULL;

    npSeries = _splBuildSeries(1, 2, nDimension);
    if (!npSeries)
    {
        RwFree(rpCoefficient);
        rpCoefficient = (RwReal *)NULL;
        RWRETURN((RwReal *)NULL);
    }

    for (nI = nDimension - 2; nI >= 0; nI -= 2)
    {
        npSeries[nI] = -npSeries[nI];
    }

    if (nDimension & 1)
    {
        nMultiplier = 2L;
    }
    else
    {
        nMultiplier = -2L;
    }

    for (nI = 0; nI < nDimension; ++nI)
    {
        rpCoefficient[nDimension * nI] =
            (RwReal) ((RwReal) (npSeries[nDimension - 1 - nI]) /
                      (RwReal) nDivisor);

        for (nJ = 1; nJ <= nI; ++nJ)
        {
            rpCoefficient[nDimension * nI + nJ] = (RwReal)
                (((RwReal) (npSeries[nJ] *
                            npSeries[nDimension - 1 - nI] *
                            nMultiplier)) / (RwReal) nDivisor);
        }
    }
    rpCoefficient[nDimension * nDimension - 1] = rpCoefficient[0];

    for (nI = 0; nI < nDimension; ++nI)
    {
        for (nJ = nDimension - 1; nJ > nI; --nJ)
        {
            rpCoefficient[nI * nDimension + nJ]
                =
                rpCoefficient[nDimension * (nDimension - nI) - 1 - nJ];
        }
    }

    RwFree(npSeries);
    npSeries = (RwInt32 *)NULL;
    RWRETURN(rpCoefficient);
}

/****************************************************************************
 _splSetCoefficients

 Builds the table of coefficient matrices.
 Called once from RpSplineOpen.

 On entry : void
 On exit  : RwBool      TRUE if successful
****************************************************************************/
static              RwBool
_splSetCoefficients(void)
{
    RwInt32             nI, nJ;

    RWFUNCTION(RWSTRING("_splSetCoefficients"));

    for (nI = 4; nI <= splSIGNIFICANTPOINTS; ++nI)
    {
        /* Build the table of coefficient matrices */
        if ((rapGTable[nI] = _splBuildCoefficientMatrix(nI)) == NULL)
        {
            /* If problem, free those already allocated */
            for (nJ = --nI; nJ >= 4; --nJ)
            {
                RwFree(rapGTable[nJ]);
                rapGTable[nJ] = (RwReal *)NULL;
            }
            /* And return false */
            RWRETURN(FALSE);
        }
    }
    /* Success */
    RWRETURN(TRUE);
}

/****************************************************************************
 _splCreateSpline

 Create a spline from the specified points by generating a set of phantom
 points such that the spline generated from the phantom points passes through
 the original set of points.

 On entry : RpSpline    *splSpline          spline struc
            RwV3d       *points           control points
 On exit  : RpSpline*   spline struc, or NULL if unsuccessful
****************************************************************************/
static RpSpline    *
_splCreateSpline(RpSpline * splSpline, RwV3d * points)
{
    RwV3d              *vpTemp, /* temporary pointer */
                        vAccu;  /* accumulator */
    RwInt32             nSigPoints, /* significant points */
                        nNumPoints, /* original number of points */
                        nI, nJ, *npSeries, /* series array */
                        nDivisor; /* series divisor */
    RwReal             *rpCoefs; /* coefficent matrix */
    RwReal              recipDivisor;

    RWFUNCTION(RWSTRING("_splCreateSpline"));

    RWASSERT(splSpline);
    RWASSERT(points);

    nNumPoints = RpSplineGetNumControlPoints(splSpline);

    vpTemp = &splSpline->vPts[0];

    switch (splSpline->nSplineType)
    {
        case rpSPLINETYPEOPENLOOPBSPLINE: /* open ended spline */

            /* determine no of significant  points */
            nSigPoints = min(nNumPoints, splSIGNIFICANTPOINTS);
            rpCoefs = rapGTable[nSigPoints]; /* build coefficient matrix */

            if (!rpCoefs)
            {
                RWERROR((E_RW_NULLP));
                RWRETURN((RpSpline *)NULL);
            }

            for (nI = 0; nI < nNumPoints; ++nI)
            {
                vAccu.x = ((RwReal) 0);
                vAccu.y = ((RwReal) 0);
                vAccu.z = ((RwReal) 0);

                if (nI < nSigPoints / 2)
                {
                    /* use start of coefficient matrix for first points */
                    for (nJ = 0; nJ < nSigPoints; ++nJ)
                    {
                        addScaledVector(&vAccu, &points[nJ],
                                        rpCoefs[nI * nSigPoints + nJ]);
                    }
                }
                else if (nI < nNumPoints - nSigPoints / 2)
                {
                    /* use middle for mid points */
                    for (nJ = 0; nJ < nSigPoints; ++nJ)
                    {
                        addScaledVector(&vAccu,
                                        &points[nI - nSigPoints / 2 +
                                                nJ],
                                        rpCoefs[nSigPoints *
                                                (nSigPoints / 2) + nJ]);
                    }
                }
                else
                    /* and the end for last points */
                {
                    for (nJ = 0; nJ < nSigPoints; ++nJ)
                    {
                        addScaledVector(&vAccu,
                                        &points[nNumPoints -
                                                nSigPoints + nJ],
                                        rpCoefs[nSigPoints *
                                                (nSigPoints -
                                                 nNumPoints + nI) +
                                                nJ]);

                    }
                }
                vpTemp[nI + 1] = vAccu;
            }
            /* first & last points are duplicates of the 2nd & 2nd last. This
             * generates tidy ends for most cases
             */

            vpTemp[0] = vpTemp[2];
            vpTemp[nNumPoints + 1] = vpTemp[nNumPoints - 1];

            break;

        case rpSPLINETYPECLOSEDLOOPBSPLINE: /* closed loop */

            /* determine no of significant points */
            nSigPoints = min(nNumPoints, splSIGNIFICANTPOINTS);

            if (nSigPoints % 2)
            {
                /* odd no of points */
                npSeries = _splBuildSeries(1, 3, nSigPoints / 2 + 1);
                if (!npSeries)
                {
                    RWRETURN((RpSpline *)NULL);
                }
                nDivisor = npSeries[nSigPoints / 2];
                recipDivisor = ((((RwReal) 1)) / ((RwReal) (nDivisor)));
                RwFree(npSeries);
                npSeries = (RwInt32 *)NULL;
                npSeries = _splBuildSeries(1, 5, nSigPoints / 2 + 1);
                if (!npSeries)
                {
                    RWRETURN((RpSpline *)NULL);
                }
            }
            else
            {
                /* even no of points */
                npSeries = _splBuildSeries(1, 4, nSigPoints / 2);
                if (!npSeries)
                {
                    RWRETURN((RpSpline *)NULL);
                }
                nDivisor = npSeries[nSigPoints / 2 - 1];
                recipDivisor = ((((RwReal) 1)) / ((RwReal) (nDivisor)));
                RwFree(npSeries);
                npSeries = (RwInt32 *)NULL;
                npSeries = _splBuildSeries(1, 2, nSigPoints / 2 + 1);
                if (!npSeries)
                {
                    RWRETURN((RpSpline *)NULL);
                }
            }
            for (nI = nSigPoints / 2 - 1; nI >= 0; nI -= 2)
            {
                npSeries[nI] = -npSeries[nI];
            }

            for (nI = 0; nI < nNumPoints; ++nI)
            {
                RwV3dScale(&vAccu,
                           &points[nI % nNumPoints],
                           (RwReal) npSeries[nSigPoints / 2]);

                for (nJ = 1; nJ < (nSigPoints / 2 + 1); ++nJ)
                {
                    RwV3d              *srcePt =
                        &points[(nJ + nI) % nNumPoints];

                    addScaledVector(&vAccu,
                                    srcePt,
                                    (RwReal) npSeries[nSigPoints / 2 -
                                                      nJ]);
                }
                for (nJ = 1; nJ < (nSigPoints + 1) / 2; ++nJ)
                {
                    RwV3d              *srcePt =
                        &points[(nNumPoints + nI - nJ) % nNumPoints];

                    addScaledVector(&vAccu, srcePt, (RwReal)
                                    npSeries[nSigPoints / 2 - nJ]);
                }
                RwV3dScale(&vpTemp[nI + 1], &vAccu, recipDivisor);
            }

            vpTemp[0] = vpTemp[nNumPoints];
            vpTemp[nNumPoints + 1] = vpTemp[1];
            vpTemp[nNumPoints + 2] = vpTemp[2];
            RwFree(npSeries);
            npSeries = (RwInt32 *)NULL;
            break;

        default:
            RWERROR((E_RP_SPLINE_INVTYPE));
            RWRETURN((RpSpline *)NULL);
    }

    RWRETURN(splSpline);
}

/****************************************************************************
 SplineClose

 Callback for plugin termination.  On last time through, destroys the global tables.

 On entry : engine instance
          : offset of global data in instance
          : size of global data in instance
 On exit  : engine instance on success
****************************************************************************/
static void        *
SplineClose(void *instance,
            RwInt32 __RWUNUSED__ offset, RwInt32 __RWUNUSED__ size)
{
    RWFUNCTION(RWSTRING("SplineClose"));

    RWASSERT(instance);

    /* One less instance, and do we need the tables anymore */
    --splineModule.numInstances;
    if (splineModule.numInstances == 0)
    {
        RwInt32             nI;

        /* Free any resources that have been allocated by the spline stuff */
        for (nI = splSIGNIFICANTPOINTS; nI >= 4; --nI)
        {
            RwFree(rapGTable[nI]);
            rapGTable[nI] = (RwReal *)NULL;
        }

        RwFree(rpGBsplinePosition0);
        rpGBsplinePosition0 = (RwReal *)NULL;
    }

    RWRETURN(instance);
}

/****************************************************************************
 SplineOpen

 Callback for plugin initialisation.  On first time through, builds the global tables.

 On entry : engine instance
          : offset of global data in instance
          : size of global data in instance
 On exit  : engine instance on success
****************************************************************************/
static void        *
SplineOpen(void *instance,
           RwInt32 __RWUNUSED__ offset, RwInt32 __RWUNUSED__ size)
{
    RWFUNCTION(RWSTRING("SplineOpen"));

    RWASSERT(instance);

    if (splineModule.numInstances == 0)
    {
        if (!_splSetBspline())
        {
            RWERROR((E_RP_SPLINE_OPEN));
            RWRETURN(NULL);
        }
        if (!_splSetCoefficients())
        {
            RWERROR((E_RP_SPLINE_OPEN));
            RWRETURN(NULL);
        }
    }

    ++splineModule.numInstances;

    RWRETURN(instance);
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                        Loading & saving

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/**
 * \ingroup rpspline
 * \ref RpSplineStreamRead is used to read a spline from the specified
 * binary stream.  Note that prior to this function call a binary spline
 * chunk must be found in the stream using the \ref RwStreamFindChunk API
 * function.
 *
 * The spline plugin must be attached before using this function.
 *
 * The sequence to locate and read a spline from a binary stream
 * is as follows
 * \code
   RwStream *stream;
   RpSpline *newSpline;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_SPLINE, NULL, NULL) )
       {
           newSpline = RpSplineStreamRead(stream);
       }

       RwStreamClose(stream, NULL);
   }
   \endcode
 *
 * \param stream  Pointer to the stream.
 *
 * \return Returns a pointer to the new spline if successful or NULL if
 *        there is an error.
 *
 * \see RpSplineStreamWrite
 * \see RpSplineStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RwStreamFindChunk
 * \see RpSplinePluginAttach
 *
 */

RpSpline           *
RpSplineStreamRead(RwStream * stream)
{
    _rpSpline           s;
    RpSpline           *spline;
    RwV3d              *points;
    RwInt32             numVertices;

    RWAPIFUNCTION(RWSTRING("RpSplineStreamRead"));

    RWASSERT(stream);
    RWASSERT(splineModule.numInstances);

    if (RwStreamRead(stream, &s, sizeof(s)) != sizeof(s))
    {
        RWRETURN((RpSpline *)NULL);
    }
    (void)RwMemNative32(&s.numVertices, sizeof(s.numVertices));
    (void)RwMemNative32(&s.splineType, sizeof(s.splineType));

    numVertices = s.numVertices;

    /* We now know the name and number of vertices of the spline */
    /* Allocate memory for the control points */
    if (!(points = (RwV3d *) RwMalloc(sizeof(RwV3d) * numVertices,
                          rwID_SPLINEPLUGIN | rwMEMHINTDUR_FUNCTION)))
    {
        RWERROR((E_RW_NOMEM, (sizeof(RwV3d) * numVertices)));
        RWRETURN((RpSpline *)NULL);
    }

    /* Read in the points, all in one go */
    if (!RwStreamReadReal(stream, (RwReal *) points, sizeof
                          (RwV3d) * numVertices))
    {
        RWRETURN((RpSpline *)NULL);
    }

    /* Create a spline of the correct type */
    spline = RpSplineCreate(numVertices, s.splineType, points);

    /* Set its name */
    rwstrncpy(spline->caName, s.name, rpSPLINENAMELENGTH);

    /* set the object type */
    spline->tType.type = rwID_SPLINE;

    /* free up those control points */
    RwFree(points);
    points = (RwV3d *)NULL;

    RWRETURN(spline);
}

/**
 * \ingroup rpspline
 * \ref RpSplineStreamWrite is used to write the specified spline to
 * the given binary stream.  Note that the stream will have been opened
 * prior to this function call.
 *
 * The spline plugin must be attached before using this function.

 * \param spline  Pointer to the spline.
 * \param stream  Pointer to the stream.
 *
 * \return Returns a pointer to the spline if successful or NULL if there
 * is an error.
 *
 * \see RpSplineStreamRead
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RpSplinePluginAttach
 *
 */

const RpSpline     *
RpSplineStreamWrite(const RpSpline * spline, RwStream * stream)
{
    _rpSpline           s;

    RWAPIFUNCTION(RWSTRING("RpSplineStreamWrite"));

    RWASSERT(spline);
    RWASSERT(stream);
    RWASSERT(splineModule.numInstances);

    if (!RwStreamWriteChunkHeader(stream, rwID_SPLINE,
                                  RpSplineStreamGetSize(spline)))
    {
        RWRETURN((const RpSpline *)NULL);
    }

    /* Write header containing number of vertices, name and type */

    s.numVertices = RpSplineGetNumControlPoints(spline);
    if (!spline->caName)
    {
        RWERROR((E_RP_SPLINE_INVNAME));
        RWRETURN((const RpSpline *)NULL);
    }
    /* Copy name to header */
    rwstrncpy(s.name, spline->caName, rpSPLINENAMELENGTH);

    /* Copy type to header */
    s.splineType = spline->nSplineType;

    /* Write header */
    (void)RwMemLittleEndian32(&s.numVertices, sizeof(s.numVertices));
    (void)RwMemLittleEndian32(&s.splineType, sizeof(s.splineType));

    if (!RwStreamWrite(stream, &s, sizeof(s)))
    {
        RWRETURN((const RpSpline *)NULL);
    }

    /* Write the points , all in one go */
    if (!RwStreamWriteReal(stream, (RwReal *)spline->ctrlPoints,
                           sizeof(RwV3d)
                           * RpSplineGetNumControlPoints(spline)))
    {
        RWRETURN((const RpSpline *)NULL);
    }

    RWRETURN(spline);
}

/**
 * \ingroup rpspline
 * \ref RpSplineStreamGetSize is used to determine the size in bytes
 * of the binary representation of the specified spline object.  This is
 * used in the binary chunk header to indicate the size of the chunk. The
 * size does not include the size of the chunk header.
 *
 * The spline plugin must be attached before using this function.
 *
 * \param spline  Pointer to the spline.
 *
 * \return Returns an RwUInt32 value equal to the chunk size of the spline
 *        if successful or zero if there is an error.
 *
 * \see RpSplineStreamRead
 * \see RpSplineStreamWrite
 * \see RpSplinePluginAttach
 *
 */

RwUInt32
RpSplineStreamGetSize(const RpSpline * spline)
{
    RwUInt32            size;

    RWAPIFUNCTION(RWSTRING("RpSplineStreamGetSize"));

    RWASSERT(spline);
    RWASSERT(splineModule.numInstances);

    size = sizeof(_rpSpline);
    size += RpSplineGetNumControlPoints(spline) * sizeof(RwV3d);
    RWRETURN(size);
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                            Creating splines

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/**
 * \ingroup rpspline
 * \ref RpSplineGetNumControlPoints is used to retrieve the number of
 * control points defining the specified spline
 *
 * The spline plugin must be attached before using this function.
 *
 * \param spline  Pointer to the spline.
 *
 * \return Returns an RwInt32 value equal to the number of control points if
 *        successful or zero if there is an error.
 *
 * \see RpSplineSetControlPoint
 * \see RpSplineGetControlPoint
 * \see RpSplineFindPosition
 * \see RpSplineFindMatrix
 * \see RpSplinePluginAttach
 *
 */

RwInt32
RpSplineGetNumControlPoints(const RpSpline * spline)
{
    RWAPIFUNCTION(RWSTRING("RpSplineGetNumControlPoints"));

    RWASSERT(spline);
    RWASSERT(splineModule.numInstances);

    RWASSERT(spline->tType.type == rwID_SPLINE);

    switch (spline->nSplineType)
    {
        case rpSPLINETYPEOPENLOOPBSPLINE:
            {
                RWRETURN(spline->numCtrlPoints - 2);
                /* break; */
            }
        case rpSPLINETYPECLOSEDLOOPBSPLINE:
            {
                RWRETURN(spline->numCtrlPoints - 3);
                /* break; */
            }
        default:
            {
                RWERROR((E_RP_SPLINE_INVTYPE));
                RWRETURN(0);
                /* break; */
            }
    }
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                    Getting and setting control points

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/**
 * \ingroup rpspline
 * \ref RpSplineGetControlPoint is used to retrieve the position
 * of the given control point in the specified spline.
 *
 * The spline plugin must be attached before using this function.
 *
 * \param spline  Pointer to the spline.
 * \param control  An RwInt32 value equal to the index of the control point in the
 *       range of 0 to numPoints-1.
 * \param point  Pointer to an RwV3d value that will receive the position of the
 *       control point.
 *
 * \return Returns pointer to the control point position if successful or
 *        NULL is there is an error.
 *
 * \see RpSplineSetControlPoint
 * \see RpSplineGetNumControlPoints
 * \see RpSplineFindPosition
 * \see RpSplineFindMatrix
 * \see RpSplinePluginAttach
 *
 */

RwV3d              *
RpSplineGetControlPoint(RpSpline * spline, RwInt32 control,
                        RwV3d * point)
{
    RWAPIFUNCTION(RWSTRING("RpSplineGetControlPoint"));

    RWASSERT(spline);
    RWASSERT(point);
    RWASSERT(splineModule.numInstances);

    RWASSERT(spline->tType.type == rwID_SPLINE);

    /* If control point in range */
    if ((control >= 0)
        && (control < RpSplineGetNumControlPoints(spline)))
    {
        *point = spline->ctrlPoints[control];
        RWRETURN(point);
    }
    else
    {
        RWERROR((E_RW_BADPARAM,
                 RWSTRING("control point out of range")));
        RWRETURN((RwV3d *)NULL);
    }
}

/**
 * \ingroup rpspline
 * \ref RpSplineSetControlPoint is used to redefine the position of
 * the given contrl point in the specified spline and recalculate the
 * spline.
 *
 * The spline plugin must be attached before using this function.
 *
 * \param spline  Pointer to the spline.
 * \param control  An RwInt32 value equal to the index of the control point (index
 *       of the first control point is zero).
 * \param point  Pointer to an RwV3d value equal to the position of the control
 *       point.
 *
 * \return Returns a pointer to the spline if successful or NULL if there
 *        is an error.
 *
 * \see RpSplineGetControlPoint
 * \see RpSplineGetNumControlPoints
 * \see RpSplineFindPosition
 * \see RpSplineFindMatrix
 * \see RpSplinePluginAttach
 *
 */

RpSpline           *
RpSplineSetControlPoint(RpSpline * spline, RwInt32 control,
                        RwV3d * point)
{
    RWAPIFUNCTION(RWSTRING("RpSplineSetControlPoint"));

    RWASSERT(splineModule.numInstances);
    RWASSERT(spline);
    RWASSERT(point);

    /* If control point index in range */
    if ((control >= 0)
        && (control < RpSplineGetNumControlPoints(spline)))
    {
        spline->ctrlPoints[control] = *point;
        if (_splCreateSpline(spline, spline->ctrlPoints))
        {
            RWRETURN(spline);
        }

        RWRETURN((RpSpline *)NULL);
    }
    else
    {
        RWERROR((E_RW_BADPARAM,
                 RWSTRING("control point out of range")));
        RWRETURN((RpSpline *)NULL);
    }
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                    Getting a point along a spline,
                    transforming a frame along a spline.

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/**
 * \ingroup rpspline
 * \ref RpSplineFindPosition is used to calculate the position and the
 * unit tangent vector (pointing in the sense of increasing parameter
 * value) of a point on the specified spline with the given parameter
 * value.
 *
 * The spline plugin must be attached before using this function.
 *
 * \param spline  Pointer to the spline.
 * \param path  An RwInt32 value equal to the spline path type:
 *     \li rpSPLINEPATHSMOOTH - "Constant velocity" path.  Note that
 * this refers to constant velocity in control point space, and not in
 * cartesian space.  The plugin assumes that each control point is one
 * unit away from its neighbours.  If you require constant velocity in
 * cartesian (world) space then the control points must be distributed
 * evenly along the spline in world space.  At this time RenderWare does
 * not perform this resampling.
 *     \li rpSPLINEPATHNICEENDS - "Accelerate" into path, "deccelerate"
 *                                  out of path.
 * \param where  An RwReal value equal to the position along the spline: start
 *       position is defined as zero; end position as one.
 * \param pos  An RwV3d value that will receive the position of the required
 *       point.
 * \param tangent  An RwV3d value that will receive the position of the end
 *       point of the unit tangent vector. If the tangent is not required
 *       to be computed, its value can be set to NULL.
 *
 * \return Returns an RwV3d value equal to the position of the required
 *        point if successful or NULL if there is an error.
 *
 * \see RpSplineFindMatrix
 * \see RpSplineGetControlPoint
 * \see RpSplineGetNumControlPoints
 * \see RpSplinePluginAttach
 *
 */

RwV3d              *
RpSplineFindPosition(RpSpline * spline, RwInt32 path, RwReal where,
                     RwV3d * pos, RwV3d * tangent)
{
    RwInt32             nI, nJ, nNumPoints;
    RwReal              rFrac;
    RwReal              rParameter;
    RwReal              rSubSample;
    RwReal              rW0;
    RwReal              rW1;
    RwReal              rW2;
    RwReal              rW3;
    RwV3d              *splinePts;
    RwV3d              *vpV0, *vpV1, *vpV2, *vpV3;

    RWAPIFUNCTION(RWSTRING("RpSplineFindPosition"));

    RWASSERT(splineModule.numInstances);
    RWASSERT(spline);
    RWASSERT(pos);

    RWASSERT(spline->tType.type == rwID_SPLINE);

    /* Cache the array of points */
    splinePts = spline->vPts;

    /* Number of control points */
    nNumPoints = spline->numCtrlPoints;

    /* Make where positive */
    where = RwRealAbs(where);

    /* Fix where so it never goes past 1.0 */
    if ((spline->nSplineType == rpSPLINETYPECLOSEDLOOPBSPLINE) &&
        (where > ((RwReal) 1)))
    {
        where = ((where) - ((RwReal) (RwFastRealToUInt32(where))));
    }

    switch (path)
    {
        case rpSPLINEPATHSMOOTH:
            {
                /* No modification */
                break;
            }
        case rpSPLINEPATHNICEENDS:
            {
                /* Slow down towards ends */
                if (where <= ((RwReal) 0.5))
                {
                    where = (where + where) * where;
                }
                else
                {
                    where = (((RwReal) (-1.0f)) +
                             ((RwReal) (-2.0f) * where * where) +
                             ((RwReal) (4.0f) * where));
                }
                break;
            }
        default:
            {
                RWERROR((E_RP_SPLINE_INVTYPE));
                RWRETURN((RwV3d *)NULL);
            }
    }

    /* General case */
    if (where != ((RwReal) 1))
    {
        rParameter = (((RwReal) (nNumPoints - 3)) * (where));

        /* Integer part of rParameter */
        nJ = RwInt32FromRealMacro(rParameter);
        /* Fractional part of rParameter */
        rFrac = ((rParameter) - ((RwReal) (nJ)));

        /* Convert the fractional part into an index & a subsample */
        rParameter = ((rFrac) * ((RwReal) ((RwReal) splWEIGHTRES)));

        /* Integer part */
        nI = RwInt32FromRealMacro(rParameter);
        /* Subsample */
        rSubSample = ((rParameter) - ((RwReal) (nI)));
    }
    else
        /* Special case where==1.0 - frig it */
    {
        nJ = nNumPoints - 4;
        nI = splWEIGHTRES - 1;
        rSubSample = ((RwReal) 1);
    }

    vpV0 = splinePts + nJ;
    vpV1 = splinePts + nJ + 1;
    vpV2 = splinePts + nJ + 2;
    vpV3 = splinePts + nJ + 3;

    rW3 =
        (RwReal) splLINEARINTERPOLATE(rpGBsplinePosition3[nI],
                                      rSubSample,
                                      rpGBsplinePosition3[nI + 1]);
    rW2 =
        (RwReal) splLINEARINTERPOLATE(rpGBsplinePosition2[nI],
                                      rSubSample,
                                      rpGBsplinePosition2[nI + 1]);
    rW1 =
        (RwReal) splLINEARINTERPOLATE(rpGBsplinePosition1[nI],
                                      rSubSample,
                                      rpGBsplinePosition1[nI + 1]);
    rW0 =
        (RwReal) splLINEARINTERPOLATE(rpGBsplinePosition0[nI],
                                      rSubSample,
                                      rpGBsplinePosition0[nI + 1]);

    RwV3dScale(pos, vpV0, rW3);
    addScaledVector(pos, vpV1, rW2);
    addScaledVector(pos, vpV2, rW1);
    addScaledVector(pos, vpV3, rW0);

    if (tangent)
    {
        RwReal              recipLength;

        /* Now fill in the vector (which should be tangential (approx.)) */

        rW3 = (RwReal)
            splLINEARINTERPOLATE(rpGBsplineVelocity3[nI],
                                 rSubSample,
                                 rpGBsplineVelocity3[nI + 1]);
        rW2 =
            (RwReal) splLINEARINTERPOLATE(rpGBsplineVelocity2[nI],
                                          rSubSample,
                                          rpGBsplineVelocity2[nI + 1]);
        rW1 =
            (RwReal) splLINEARINTERPOLATE(rpGBsplineVelocity1[nI],
                                          rSubSample,
                                          rpGBsplineVelocity1[nI + 1]);
        rW0 =
            (RwReal) splLINEARINTERPOLATE(rpGBsplineVelocity0[nI],
                                          rSubSample,
                                          rpGBsplineVelocity0[nI + 1]);

        RwV3dScale(tangent, vpV0, rW3);
        addScaledVector(tangent, vpV1, rW2);
        addScaledVector(tangent, vpV2, rW1);
        addScaledVector(tangent, vpV3, rW0);

        /* Normalize vector. If original length 0= 0  */

        recipLength = _rwV3dNormalize(tangent, tangent);
        if (recipLength == ((RwReal) 0))
        {
            tangent->x = (RwReal) (0.);
            tangent->y = ((RwReal) 1);
            tangent->z = (RwReal) (0.);
        }

    }

    RWRETURN(pos);
}

/**
 * \ingroup rpspline
 * \ref RpSplineFindMatrix is used to calculate a matrix representing
 * the Frenet matrix at the given parameter position along the specified
 * spline.  This matrix can be used to transform an object's frame to the
 * given parameter position and orientation as follows: look-at vector
 * tangent to the path (pointing in the sense of increasing parameter
 * value), look-up vector as specified by up, and look-right vector
 * pointing either towards or away from the center of curvature.
 *
 * The spline plugin must be attached before using this function.
 *
 * \param spline  Pointer to the spline.
 * \param path  An RwInt32 value equal to the spline path type:
 *        \li rpSPLINEPATHSMOOTH - "Constant velocity" path.
 *        \li rpSPLINEPATHNICEENDS - "Acceleration" into path, "decelerate"
 *                                  out of path.
 * \param where  An RwReal value equal to the position along the spline; start
 *       position is defined as zero; end position as one.
 * \param up  An RwV3d value equal to the frame's look-up vector.  Specifying
 *       NULL calculates a matrix with look-up vector aligned with that
 *       of the Frenet frame and a look-right vector pointing towards the
 *       center of curvature.
 * \param matrix  An RwMatrix value that will receive the matrix.
 *
 * \return Returns an RwReal value equal to the spline's curvature at the
 *        point in question if successful or -1 if there is an error.
 *
 * \see RpSplineFindPosition
 * \see RpSplineGetControlPoint
 * \see RpSplinePluginAttach
 *
 */

RwReal
RpSplineFindMatrix(RpSpline * spline, RwInt32 path, RwReal where,
                   RwV3d * up, RwMatrix * matrix)
{
    RwInt32             nJ, nI, nNumPoints;
    RwReal              rCurvature;
    RwReal              rFrac;
    RwReal              rParameter;
    RwReal              rSubSample;
    RwReal              rW0;
    RwReal              rW1;
    RwReal              rW2;
    RwReal              rW3;
    RwReal              recipLength;
    RwV3d               vAcceleration;
    RwV3d               vBinormal;
    RwV3d               vVelocity;
    RwV3d              *position;
    RwV3d              *splinePts;
    RwV3d              *vpUnitAt;
    RwV3d              *vpUnitRight;
    RwV3d              *vpUnitUp;
    RwV3d              *vpV0;
    RwV3d              *vpV1;
    RwV3d              *vpV2;
    RwV3d              *vpV3;

    RWAPIFUNCTION(RWSTRING("RpSplineFindMatrix"));

    RWASSERT(splineModule.numInstances);
    RWASSERT(spline);
    RWASSERT(matrix);

    RWASSERT(spline->tType.type == rwID_SPLINE);

    /* Cache pointer to control points */
    splinePts = spline->vPts;

    vpUnitRight = &matrix->right;
    vpUnitUp = &matrix->up;
    vpUnitAt = &matrix->at;
    position = &matrix->pos;

    nNumPoints = spline->numCtrlPoints;

    /* Make where positive */
    where = RwRealAbs(where);

    /* Fix where so it goes past 1.0 */
    if ((spline->nSplineType == rpSPLINETYPECLOSEDLOOPBSPLINE) &&
        (where > ((RwReal) 1)))
    {
        where = ((where) - ((RwReal) (RwFastRealToUInt32(where))));
    }

    switch (path)
    {
        case rpSPLINEPATHSMOOTH:
            {
                /* No modification */
                break;
            }
        case rpSPLINEPATHNICEENDS:
            {
                /* Slow down towards ends */
                if (where <= ((RwReal) 0.5))
                {
                    where = (where + where) * where;
                }
                else
                {
                    where = (((RwReal) (-1.0f)) +
                             ((RwReal) (-2.0f) * where * where) +
                             ((RwReal) (4.0f) * where));
                }
                break;
            }
        default:
            {
                RWERROR((E_RP_SPLINE_INVTYPE));
                RWRETURN((RwReal) (-1.0));
            }
    }
    /* General case */
    if (where != ((RwReal) 1))
    {
        rParameter = (((RwReal) (nNumPoints - 3)) * (where));

        /* Integer part of rParameter */
        nJ = RwInt32FromRealMacro(rParameter);
        /* Fractional part of rParameter */
        rFrac = ((rParameter) - ((RwReal) (nJ)));

        /* Convert the fractional part into an index & a subsample */
        rParameter = ((rFrac) * ((RwReal) (splWEIGHTRES)));

        /* Integer part of p */
        nI = RwInt32FromRealMacro(rParameter);
        /* Subsample */
        rSubSample = ((rParameter) - ((RwReal) (nI)));
    }
    else
        /* Special case where == 1.0 - frig it */
    {
        nJ = nNumPoints - 4;
        nI = splWEIGHTRES - 1;
        rSubSample = ((RwReal) 1);
    }

    vpV0 = splinePts + nJ;
    vpV1 = splinePts + nJ + 1;
    vpV2 = splinePts + nJ + 2;
    vpV3 = splinePts + nJ + 3;

    /* RwMatrixUpdate(matrix); */
    RwMatrixOptimize(matrix, (RwMatrixTolerance *) NULL);

    /* Interpolate position */
    rW3 = (RwReal)
        splLINEARINTERPOLATE(rpGBsplinePosition3[nI],
                             rSubSample, rpGBsplinePosition3[nI + 1]);
    rW2 = (RwReal)
        splLINEARINTERPOLATE(rpGBsplinePosition2[nI],
                             rSubSample, rpGBsplinePosition2[nI + 1]);
    rW1 = (RwReal)
        splLINEARINTERPOLATE(rpGBsplinePosition1[nI],
                             rSubSample, rpGBsplinePosition1[nI + 1]);
    rW0 = (RwReal)
        splLINEARINTERPOLATE(rpGBsplinePosition0[nI],
                             rSubSample, rpGBsplinePosition0[nI + 1]);

    RwV3dScale(position, vpV0, rW3);
    addScaledVector(position, vpV1, rW2);
    addScaledVector(position, vpV2, rW1);
    addScaledVector(position, vpV3, rW0);

    /* Interpolate velocity */
    rW3 = (RwReal)
        splLINEARINTERPOLATE(rpGBsplineVelocity3[nI],
                             rSubSample, rpGBsplineVelocity3[nI + 1]);
    rW2 = (RwReal)
        splLINEARINTERPOLATE(rpGBsplineVelocity2[nI],
                             rSubSample, rpGBsplineVelocity2[nI + 1]);
    rW1 = (RwReal)
        splLINEARINTERPOLATE(rpGBsplineVelocity1[nI],
                             rSubSample, rpGBsplineVelocity1[nI + 1]);
    rW0 = (RwReal)
        splLINEARINTERPOLATE(rpGBsplineVelocity0[nI],
                             rSubSample, rpGBsplineVelocity0[nI + 1]);

    RwV3dScale(&vVelocity, vpV0, rW3);
    addScaledVector(&vVelocity, vpV1, rW2);
    addScaledVector(&vVelocity, vpV2, rW1);
    addScaledVector(&vVelocity, vpV3, rW0);

    /* Interpolate Acceleration */
    rW3 = (RwReal)
        splLINEARINTERPOLATE(rpGBsplineAcceleration3[nI],
                             rSubSample,
                             rpGBsplineAcceleration3[nI + 1]);
    rW2 =
        (RwReal) splLINEARINTERPOLATE(rpGBsplineAcceleration2[nI],
                                      rSubSample,
                                      rpGBsplineAcceleration2[nI + 1]);
    rW1 =
        (RwReal) splLINEARINTERPOLATE(rpGBsplineAcceleration1[nI],
                                      rSubSample,
                                      rpGBsplineAcceleration1[nI + 1]);
    rW0 =
        (RwReal) splLINEARINTERPOLATE(rpGBsplineAcceleration0[nI],
                                      rSubSample,
                                      rpGBsplineAcceleration0[nI + 1]);

    RwV3dScale(&vAcceleration, vpV0, rW3);
    addScaledVector(&vAcceleration, vpV1, rW2);
    addScaledVector(&vAcceleration, vpV2, rW1);
    addScaledVector(&vAcceleration, vpV3, rW0);

    /* Find unit at vector */
    *vpUnitAt = vVelocity;

    recipLength = _rwV3dNormalize(vpUnitAt, vpUnitAt);
    if (recipLength == ((RwReal) 0))
    {
        vpUnitAt->x = ((RwReal) 0);
        vpUnitAt->y = ((RwReal) 0);
        vpUnitAt->z = ((RwReal) 1);
    }

    /* Find unit up vector */
    RwV3dCrossProduct(&vBinormal, &vAcceleration, vpUnitAt);
    rCurvature = (RwReal) RwV3dNormalize(&vBinormal, &vBinormal);

    if (up)
    {
        /* UnitRight should be normal to both up and UnitAt */
        *vpUnitUp = *up;

        recipLength = _rwV3dNormalize(vpUnitUp, vpUnitUp);
        if (recipLength == ((RwReal) 0))
        {
            vpUnitUp->y = ((RwReal) 1);
        }
    }
    else
    {
        /* UnitRight should be normal to both binormal and UnitAt */
        *vpUnitUp = vBinormal;
        if (rCurvature == ((RwReal) 0))
        {
            vpUnitUp->y = ((RwReal) 1);
        }
    }

    /* Find unit right vector toward centre of curvature */
    RwV3dCrossProduct(vpUnitRight, vpUnitUp, vpUnitAt);

    recipLength = _rwV3dNormalize(vpUnitRight, vpUnitRight);
    if (recipLength == ((RwReal) 0))
    {
        vpUnitRight->x = ((RwReal) 1);
    }

    if (up)
    {
        /* Find actual vpUnitUp vector normal to both UnitAt and UnitRight */
        RwV3dCrossProduct(vpUnitUp, vpUnitAt, vpUnitRight);

        recipLength = _rwV3dNormalize(vpUnitUp, vpUnitUp);
        if (recipLength == ((RwReal) 0))
        {
            vpUnitUp->y = ((RwReal) 1);
        }
    }

    RWRETURN(rCurvature);
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                    Duplicating, destroying, shutting down

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/**
 * \ingroup rpspline
 * \ref RpSplineClone copies the specified spline.  This
 * function creates a new spline with exactly the same attributes as the
 * original.
 *
 * The spline plugin must be attached before using this function.
 *
 * \param spline  Pointer to the spline.
 *
 * \return Returns a pointer to the new spline if successful or NULL if
 * there is an error.
 *
 * \see RpSplineCreate
 * \see RpSplineRead
 * \see RpSplineStreamRead
 * \see RpSplineDestroy
 * \see RpSplinePluginAttach
 *
 */

RpSpline           *
RpSplineClone(RpSpline * spline)
{
    RpSpline           *slpNewSpline;
    RwInt32             nNumPoints;

    RWAPIFUNCTION(RWSTRING("RpSplineClone"));

    RWASSERT(spline);
    RWASSERT(splineModule.numInstances);

    RWASSERT(spline->tType.type == rwID_SPLINE);

    slpNewSpline = (RpSpline *)
        RwMalloc(sizeof(RpSpline) +
                 sizeof(RwV3d) * (unsigned int) (spline->numCtrlPoints -
                 1), rwID_SPLINE | rwMEMHINTDUR_EVENT);

    if (slpNewSpline)
    {
        slpNewSpline->tType.type = rwID_SPLINE;
        nNumPoints = RpSplineGetNumControlPoints(spline);
        slpNewSpline->numCtrlPoints = spline->numCtrlPoints;
        slpNewSpline->nSplineType = spline->nSplineType;
        slpNewSpline->pUser = spline->pUser;
        memcpy((void *) &slpNewSpline->vPts[0],
               (void *) &spline->vPts[0],
               sizeof(RwV3d) * (unsigned int) spline->numCtrlPoints);

        slpNewSpline->ctrlPoints = (RwV3d *)
            RwMalloc(sizeof(RwV3d) * nNumPoints,
                rwID_SPLINE | rwMEMHINTDUR_EVENT);
        if (slpNewSpline->ctrlPoints)
        {
            memcpy((void *) slpNewSpline->ctrlPoints,
                   (void *) spline->ctrlPoints,
                   sizeof(RwV3d) * (unsigned int) nNumPoints);

            RWRETURN(slpNewSpline);
        }
        else
        {
            RWERROR((E_RW_NOMEM, (sizeof(RwV3d) * nNumPoints)));
            RWRETURN((RpSpline *)NULL);
        }
    }
    else
    {
        RWERROR((E_RW_NOMEM,
                 (sizeof(RwChar) *
                  (sizeof(RpSpline) +
                   sizeof(RwV3d) * (unsigned int)
                   (spline->numCtrlPoints - 1)))));

        RWRETURN((RpSpline *)NULL);
    }
}

/**
 * \ingroup rpspline
 * \ref RpSplineRead is used to read a spline from the given disk
 * file.  This function uses the binary stream mechanism for reading the
 * spline data.
 *
 * The spline plugin must be attached before using this function.
 *
 * \param name  Pointer to a string containing the name of the input file.
 *
 * \return Returns a pointer to the new spline if successful or NULL if
 * there is an error.
 *
 * \see RpSplineStreamRead
 * \see RpSplineWrite
 * \see RpSplineStreamWrite
 * \see RpSplinePluginAttach
 *
 */

RpSpline *
RpSplineRead(const RwChar *name)
{
    RwStream           *stream;
    RpSpline           *spline = (RpSpline *)NULL;

    RWAPIFUNCTION(RWSTRING("RpSplineRead"));

    RWASSERT(name);
    RWASSERT(splineModule.numInstances);

    stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, name);
    if (stream)
    {
        RwUInt32            version;

        /* Look for the chunk marker */
        if (RwStreamFindChunk(stream, rwID_SPLINE,
                              (RwUInt32 *)NULL, &version))
        {
            RWASSERT(version >= rwLIBRARYBASEVERSION);
            RWASSERT(version <= rwLIBRARYCURRENTVERSION);

            if ((version >= rwLIBRARYBASEVERSION) &&
                (version <= rwLIBRARYCURRENTVERSION))
            {
                spline = RpSplineStreamRead(stream);
            }
            else
            {
                RWERROR((E_RW_BADVERSION));
            }
        }

        RwStreamClose(stream, NULL);
    }

    RWRETURN(spline);
}

/**
 * \ingroup rpspline
 * \ref RpSplineWrite is used to write the specified spline to the
 * given disk file.  This function uses the binary stream mechanism for
 * writing the spline data.
 *
 * The spline plugin must be attached before using this function.

 * \param spline  Pointer to the spline
 * \param name  Pointer to a string containing the name of the output file
 *
 * \return Returns TRUE if successful or FALSE if there is an error
 *
 * \see RpSplineStreamWrite
 * \see RpSplineRead
 * \see RpSplineStreamRead
 * \see RpSplinePluginAttach
 *
 */

RwBool
RpSplineWrite(RpSpline *spline, const RwChar *name)
{
    RwStream           *stream;

    RWAPIFUNCTION(RWSTRING("RpSplineWrite"));

    RWASSERT(spline);
    RWASSERT(name);
    RWASSERT(splineModule.numInstances);

    stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMWRITE, name);

    if (stream)
    {
        if (RpSplineStreamWrite(spline, stream))
        {
            RwStreamClose(stream, NULL);
            RWRETURN(TRUE);
        }

        RwStreamClose(stream, NULL);
    }

    /* Already spat error out when couldn't open file */
    RWRETURN(FALSE);
}

/**
 * \ingroup rpspline
 * \ref RpSplineDestroy is used to destroy the specified spline.
 *
 * Note this function does not free the memory holding the original
 * control point array if the spline was created with \ref RpSplineCreate.
 *
 * The spline plugin must be attached before using this function.
 *
 * \param spline  Pointer to the spline.
 *
 * \return Returns TRUE if successful or FALSE if there is an error
 *
 * \see RpSplineCreate
 * \see RpSplinePluginAttach
 *
 */

RwBool
RpSplineDestroy(RpSpline * spline)
{
    RWAPIFUNCTION(RWSTRING("RpSplineDestroy"));

    RWASSERT(spline);
    RWASSERT(splineModule.numInstances);

    RWASSERT(spline->tType.type == rwID_SPLINE);

    if (spline->ctrlPoints)
    {
        RwFree(spline->ctrlPoints);
        spline->ctrlPoints = (RwV3d *)NULL;
    }

    if (spline->pUser)
    {
        RwFree(spline->pUser);
        spline->pUser = NULL;
    }

    RwFree(spline);
    spline = (RpSpline *)NULL;
    RWRETURN(TRUE);
}

/**
 * \ingroup rpspline
 * \ref RpSplineCreate is used to create a new interpolating cubic spline
 * of the specified type with the given array of control point positions.  The
 * number of control points must be greater than or equal to four.
 *
 * \param numCtrlPoints  An RwInt32 value equal to the number of control points.
 * \param type  An RwInt32 value equal to the spline type:
 *
 *      \li rpSPLINETYPEOPENLOOPBSPLINE  Open loop - the curve starts at the
 * first control point and ends at the last control point.
 *
 * \li rpSPLINETYPECLOSEDLOOPBSPLINE Closed loop - the curve at the end control
 * point is joined to the curve at the start control point.
 *
 * \param ctrlPoints  Pointer to an array of RwV3d values containing the positions
 *       of the control points.
 *
 * \return Returns a pointer to the new spline if successful or NULL
 *        is there is an error.
 *
 * \see RpSplineDestroy
 * \see RpSplinePluginAttach
 *
 */

RpSpline           *
RpSplineCreate(RwInt32 numCtrlPoints, RwInt32 type, RwV3d * ctrlPoints)
{
    RpSpline           *spline; /* returned spline */

    RWAPIFUNCTION(RWSTRING("RpSplineCreate"));

    RWASSERT(ctrlPoints);
    RWASSERT(numCtrlPoints >= 4);
    RWASSERT(splineModule.numInstances);

    /* Determine the number of generated spline points and allocate space
     * accordingly. The closed loop case requires one extra point since the
     * interpolation is performed linearly and the beginning must therefore be
     * replicated at the end.
     */

    switch (type)
    {
        case rpSPLINETYPEOPENLOOPBSPLINE:
            {
                RwUInt32            memSize =
                    (sizeof(RpSpline) +
                     sizeof(RwV3d) * (numCtrlPoints + 1));

                spline = (RpSpline *) RwMalloc(memSize,
                    rwID_SPLINE | rwMEMHINTDUR_EVENT);
                if (!spline)
                {
                    RWERROR((E_RW_NOMEM, memSize));
                    RWRETURN((RpSpline *)NULL);
                }
                spline->numCtrlPoints = numCtrlPoints + 2;
                break;
            }

        case rpSPLINETYPECLOSEDLOOPBSPLINE:
            {
                RwUInt32            memSize =
                    (sizeof(RpSpline) +
                     sizeof(RwV3d) * (numCtrlPoints + 2));

                spline = (RpSpline *) RwMalloc(memSize,
                    rwID_SPLINE | rwMEMHINTDUR_EVENT);
                if (!spline)
                {
                    RWERROR((E_RW_NOMEM, memSize));
                    RWRETURN((RpSpline *)NULL);
                }
                spline->numCtrlPoints = numCtrlPoints + 3;
                break;
            }

        default:
            {
                RWERROR((E_RP_SPLINE_INVTYPE));
                RWRETURN((RpSpline *)NULL);
            }
    }

    /* Make sure data pointer is set to Null if unused */
    spline->pUser = NULL;

    /* Set name to Null too */
    spline->caName[0] = (RwChar) 0;

    /* Set the type in the struc */
    spline->nSplineType = type;

    /* Set the Dive type */
    spline->tType.type = rwID_SPLINE;

    /* Create the spline */
    if (!_splCreateSpline(spline, ctrlPoints))
    {
        RwFree(spline);
        spline = (RpSpline *)NULL;
        RWRETURN((RpSpline *)NULL);
    }

    spline->tType.type = rwID_SPLINE;

    /* Alloc mem for copy of control points */
    if (!
        (spline->ctrlPoints =
         (RwV3d *) RwMalloc(sizeof(RwV3d) * numCtrlPoints,
                            rwID_SPLINE | rwMEMHINTDUR_EVENT)))
    {
        RwFree(spline);
        spline = (RpSpline *)NULL;
        RWERROR((E_RW_NOMEM, (sizeof(RwV3d) * numCtrlPoints)));
        RWRETURN((RpSpline *)NULL);
    }

    /* Copy control points : dest,src,count */
    memcpy((void *) spline->ctrlPoints,
           (void *) ctrlPoints,
           sizeof(RwV3d) * (unsigned int) numCtrlPoints);

    RWRETURN(spline);
}

/**
 * \ingroup rpspline
 * \ref RpSplinePluginAttach is used to attach the spline plugin to
 * the RenderWare system to enable the generation of interpolating cubic
 * splines.  The spline plugin must be attached between initializing the
 * system with \ref RwEngineInit and opening it with \ref RwEngineOpen.
 *
 * \note The include file rpspline.h is required and must be included
 * by an application wishing the use interpolating cubic splines.
 *
 * \return Returns TRUE if successful or FALSE if there is an error
 *
 * \see RwEngineInit
 * \see RwEngineOpen
 * \see RwEngineStart
 *
 */

RwBool
RpSplinePluginAttach(void)
{
    RwInt32             offset;

    RWAPIFUNCTION(RWSTRING("RpSplinePluginAttach"));

    /* Add the splines 'plugin' */
    offset =
        RwEngineRegisterPlugin(0, rwID_SPLINEPLUGIN, SplineOpen,
                               SplineClose);

    if (offset < 0)
    {
        /* If any are negative, we've failed */
        RWRETURN(FALSE);
    }

    /* Hurrah */
    RWRETURN(TRUE);
}
