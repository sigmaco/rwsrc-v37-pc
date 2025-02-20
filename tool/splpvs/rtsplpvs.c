/*
 * Spline PVS Toolkit.
 */

/**********************************************************************
 *
 * File :     rtsplpvs.c
 *
 * Abstract : Handle PVS generation from a spline.
 *
 **********************************************************************
 *
 * This file is a product of Criterion Software Ltd.
 *
 * This file is provided as is with no warranties of any kind and is
 * provided without any obligation on Criterion Software Ltd. or
 * Canon Inc. to assist in its use or modification.
 *
 * Criterion Software Ltd. will not, under any
 * circumstances, be liable for any lost revenue or other damages arising
 * from the use of this file.
 *
 * Copyright (c) 1998 Criterion Software Ltd.
 * All Rights Reserved.
 *
 * RenderWare is a trademark of Canon Inc.
 *
 ************************************************************************/

/**
 * \ingroup rtsplinepvs
 * \page rtsplinepvsoverview RtSplinePVS Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpcollis.h, rpworld.h, rppvs.h, rpspline.h, rtsplpvs.h
 * \li \b Libraries: rwcore, rpworld, rpcollis, rppvs, rpspline, rtintsec, rtsplpvs
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpPVSPluginAttach
 *     \ref RpCollisionPluginAttach \ref RpSplinePluginAttach,
 *
 * \subsection splinepvsoverview Overview
 * This toolkit provides facilities for generating PVS data using a spline for a world.
 *
 * The \ref rppvs plugin generates PVS data for a world by taking samples from within the
 * boundaries of the world sector's bounding box. For some worlds, this is not necessary.
 *
 * An example is a racetrack where movement is limited to the track and the world is
 * modelled around the track and is not fully complete. Such a world would contain
 * geometry that would be visible from the track only with the rest of the world not
 * modelled.
 *
 * In such a scenario, PVS samples should be also taken from along track.
 *
 * The RtSplPVS toolkit allows samples to be taken from just the along the spline. This
 * reduces the number of samples taken and also generate PVS data more suited to such
 * worlds.
 */

#include "rwcore.h"
#include "rpdbgerr.h"
#include "rpworld.h"
#include "rpspline.h"
#include "rppvs.h"
#include "rtsplpvs.h"
#include "rpcollis.h"

static RwBool
SplinePVSProgress(RwInt32 msg __RWUNUSED__,
                  RwReal  value __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("SplinePVSProgress"));

    RWRETURN(TRUE);

}

static RpWorldSector *
SplinePVSGeneric(RpWorldSector * sector,
                 const RwBBox * bbox __RWUNUSED__,
                 void * data __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("SplinePVSGeneric"));

    RWRETURN(sector);
}

/**
 * \ingroup rtsplinepvs
 * \ref RtSplinePVSConstruct is used to create PVS from sampling on a
 * spline.
 *
 * \param world  Pointer to the world where PVS is to be created.
 * \param spline  Pointer sampling spline.
 * \param samples  Number of samples to be generated from the spline.
 *
 * \return Returns the input world if PVS creation was successful.
 *        NULL otherwise.
 *
 * \see RpPVSCreate
 * \see RpPVSSamplePOV
 */
RpWorld            *
RtSplinePVSConstruct(RpWorld * world,
                     RpSpline * spline, RwInt32 samples)
{
    RpWorld               *result;
    RpPVSProgressCallBack pvsProgressCB;
    RwBool collData = FALSE;

    RWAPIFUNCTION(RWSTRING("RtSplinePVSConstruct"));

    RWASSERT(world);
    RWASSERT(spline);

    /* Sanity check */
    if ((samples < 1) || (world == NULL) || (spline == NULL))
    {
        result = NULL;
    }
    else
    {
#if (0)
        RwInt32             numControlPoints =
            RpSplineGetNumControlPoints(spline);
#endif /* (0) */

        /* Swap the progress call back. */
        pvsProgressCB = RpPVSGetProgressCallBack(world);

        RpPVSSetProgressCallBack(world, SplinePVSProgress);

        /* Begin */
        if (pvsProgressCB)
            pvsProgressCB(rpPVSPROGRESSSTART, 0.0f);

        /* Build collision data for PVS generation */
        collData = RpCollisionWorldQueryData(world);
        if (collData == FALSE)
        {
            RpCollisionWorldBuildData(world,
                                  (RpCollisionBuildParam *) NULL);
        }

        /* First create a blank PVS data */
        result =
            RpPVSConstruct(world, SplinePVSGeneric, NULL);

        if (result)
        {
            RwInt32             i;
            RwReal              where, delta;
            RwMatrix            mat;

            where = (RwReal) 0.0;
            delta = (RwReal) 1.0 / (RwReal) samples;

            for (i = 0; i < samples; i++)
            {
                RpSplineFindMatrix(spline, rpSPLINEPATHSMOOTH, where,
                                   NULL, &mat);

                RpPVSSamplePOV(&(mat.pos), TRUE);

                if (pvsProgressCB)
                    pvsProgressCB(rpPVSPROGRESSUPDATE, where * 100.0f);

                where += delta;
            }
        }
        /* Now destroy collision data */
        if (!collData) RpCollisionWorldDestroyData(world);

        /* End */
        if (pvsProgressCB)
            pvsProgressCB(rpPVSPROGRESSEND, 100.0f);

        /* Replace the call back. */
        RpPVSSetProgressCallBack(world, pvsProgressCB);
    }

    RWRETURN(result);
}
