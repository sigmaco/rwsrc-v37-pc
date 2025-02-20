/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   area.c                                                     -*
 *-                                                                         -*
 *-  Purpose :   Area-light-related code for the RtLtMap toolkit            -*
 *-                                                                         -*
 *===========================================================================*/

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include "rwcore.h"
#include "rpworld.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

/* We need to use the non-INCGEN-d version of rpltmap.h */
#include "../../plugin/ltmap/rpltmap.h"
#include "rtltmap.h"


#include "polypack.h"
#include "sample.h"
#include "ltmapvar.h"

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
/*===========================================================================*
 *--- Local defines ---------------------------------------------------------*
 *===========================================================================*/


/*===========================================================================*
 *--- Global variables ------------------------------------------------------*
 *===========================================================================*/


/*===========================================================================*
 *--- Local variables -------------------------------------------------------*
 *===========================================================================*/


/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/


/****************************************************************************
 LtMapAreaLightSampleTriangle
 */
static RwUInt32
LtMapAreaLightSampleTriangle(RwSList *triangles, RwReal *meshArea, RwV3d   *triVerts, RwReal   density)
{
    RwV3d    edge[2], normal;
    RwSList *lightList;
    RwReal   length;
    RwUInt32 numSamples = 0, i;

    RWFUNCTION(RWSTRING("LtMapAreaLightSampleTriangle"));

    /* Allocate workspace for the rasterizer */
    lightList = rwSListCreate(sizeof(RwV3d), rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION);
    RWASSERT(NULL != lightList);

    /* Generate a normal (before we flip facedness) */
    RwV3dSub(&edge[0], &triVerts[2], &triVerts[1]);
    RwV3dSub(&edge[1], &triVerts[0], &triVerts[1]);
    RwV3dCrossProduct(&normal, &edge[0], &edge[1]);
    length = RwV3dDotProduct(&normal, &normal);

    /* Uses the same sliver area variable as in LtMapObjectForAllSamples.
     * Basically, we can't trust the calculated normal of sliver triangles: */
    if (length > _rpLtMapGlobals.sliverAreaThreshold)
    {
        LtMapAreaLight *triangle;
        RwReal          triangleArea;
        RwV2d           vScreen[3];
        RwV3d           bias;
        RwUInt32        axis;

        /* Create a new LtMapAreaLight */
        triangle = (LtMapAreaLight *)rwSListGetNewEntry(triangles,
                            rwID_LTMAPPLUGIN | rwMEMHINTDUR_EVENT);
        RWASSERT(NULL != triangle);
        triangle->lights = (RwV3d *)NULL;

        /* Normalize the triangle normal and calculate the triangle's area */
        rwSqrtInvSqrtMacro(&triangleArea, &length, length);
        RwV3dScale(&normal, &normal, length);
        triangleArea *= 0.5f;
        *meshArea += triangleArea;

        /* Move away from emitter surface - to avoid collisions with
         * the emitter surface blocking visibility to the light samples */
/* TODO[3][ABE]: THIS MAY BREAK - WHEN YOU'VE RESOLVED THIS FOR THE LIGHTMAP SAMPLES,
 *               MAKE THE SAME DECISION HERE (THE PROBLEM IS THIS PUSHING AREA LIGHT
 *               SAMPLES *THROUGH* ADJACENT POLYGONS AND HENCE BREAKING VISIBILITY) */
        RwV3dScale(&bias, &normal, 0.005f);
        RwV3dAdd(&triVerts[0], &triVerts[0], &bias);
        RwV3dAdd(&triVerts[1], &triVerts[1], &bias);
        RwV3dAdd(&triVerts[2], &triVerts[2], &bias);

        /* Generate a 'screen-space' triangle for the rasterizer */
/* TODO[2][ABI]: YOU SHOULD USE TRIANGLE LIGHTMAP UVs! LtMapClassifyNormal IS NOT RELIABLE! */
        axis = _rtLtMapClassifyNormal(&normal);
/* TODO[6][ACI]: YOU SHOULD ROTATE THE RASTERIZATION PLANE, AXIS-ALIGNED SAMPLES SUCK!
 *               COULD ALSO JITTER AREA LIGHT SAMPLES WITHIN THEIR UNIT SQUARES, USING
 *               SOME COORDINATE-SEEDED RANDOM NUMBER GENERATOR (SO IT'S REPEATABLE) */
        for (i = 0;i < 3;i++)
        {
            vScreen[i].x = density * RwV3dDotProduct(&triVerts[i], &rtLtMapGlobals.rightVector[axis]);
            vScreen[i].y = density * RwV3dDotProduct(&triVerts[i], &rtLtMapGlobals.upVector[axis]);
        }

        /* flip backfacing */
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

        /* Generate world-space positions across the triangle surface */
        if ((density * density) * triangleArea > _rpLtMapGlobals.areaLightSampleLimit)
        {
            /* Early-out, too many sample points! (probably)
             * Don't worry, the new (uesless) triangle will
             * get destroyed in LtMapAreaLightSampleMeshes. */
            rwSListDestroy(lightList);
            numSamples = RwInt32FromRealMacro((density * density) * triangleArea);
            RWRETURN(numSamples);
        }

        _rtLtMapALWCRenderTriangle(lightList, vScreen, triVerts);
        numSamples = rwSListGetNumEntries(lightList);

        if (numSamples == 0)
        {
/* TODO[6][ACI] THIS AND INDEED THE SAMPLING ABOVE MAY GENERATE UNEVEN SAMPLE DISTRIBUTION
 *              IN WORLD-SPACE, WHICH COULD CREATE ARTIFACTS (E.G A ROW OF NEAR-SLIVERS WHICH
 *              ARE VERY TALL AND WHICH COVER A SQUARE WILL BE SAMPLED ON A LINE ACROSS THEIR
 *              MIDDLE!)
 *               PERHAPS WE NEED LIGHTMAP-STYLE GREEDY SAMPLE-GENERATION AGAIN? DO IT IN
 *              PROJECTED AXIS-SPACE, WITH A TEMPORARY RASTER FROM WHICH WE GENERATE RWV3DS AS
 *              A POST-PROCESS */
            /* Put one sample at the centroid */
            RwV3d *light;

            light = (RwV3d *)rwSListGetNewEntry(lightList,
                rwID_LTMAPPLUGIN | rwMEMHINTDUR_FUNCTION);
            RWASSERT(NULL != light);
            RwV3dAdd(light, &triVerts[0], &triVerts[1]);
            RwV3dAdd(light, light, &triVerts[2]);
            RwV3dScale(light, light, 1.0f / 3.0f);
            numSamples = 1;
        }

        /* Copy from the RwSList into a permanent array */
        triangle->lights = (RwV3d *)RwMalloc(numSamples * sizeof(RwV3d),
                             rwID_LTMAPPLUGIN | rwMEMHINTDUR_EVENT);
        RWASSERT(NULL != triangle->lights);
        for (i = 0;i < numSamples;i++)
        {
            triangle->lights[i] = *(RwV3d *)rwSListGetEntry(lightList, i);
        }

        /* We store (triangleArea/numSamples) in the triangle, which is used
         * during lighting to ensure that each sample represents the right area
         * of the emitter and that in sum they represent *all* of the emitter. */
        triangle->areaPerSample = triangleArea / numSamples;
        triangle->plane.normal = normal;
        triangle->plane.distance = RwV3dDotProduct(&normal, &triVerts[0]);
        triangle->numSamples = numSamples;
        triangle->flags = 0;
        _rtLtMapTriangleMakeSphere(&triangle->sphere, triVerts);
    }

    rwSListDestroy(lightList);

    RWRETURN(numSamples);
}

static void
LtMapAreaLightCalcMeshROI(LtMapAreaLightMesh *lightMesh,
                          RwReal area,
                          LtMapMaterialData *matData,
                          RwBBox *box)
{
    RwV3d radius;
    RwReal totalOutput,roiRadius;

    RWFUNCTION(RWSTRING("LtMapAreaLightCalcMeshROI"));

    /* build a bounding sphere */
    RwV3dAdd(&lightMesh->sphere.center, &box->inf, &box->sup);
    RwV3dScale(&lightMesh->sphere.center, &lightMesh->sphere.center, 0.5f);
    RwV3dSub(&radius, &box->sup, &box->inf);
    lightMesh->sphere.radius = RwV3dLength(&radius) * 0.5f;

    /* Calculate a (very) conservative ROI for the mesh centred on the above
     * sphere (it takes the distance at which falloff of a point light of the
     * same total emission as this mesh reaches _rpLtMapGlobals.recipAreaLightROICutoff,
     * and then adds this value to the mesh's radius so that in the worst case
     * (i.e all the mesh's light is concentrated at one corner) you're still
     * conservative): */

    /* TODO[5][AAH]: reduce the ROI by calculating the maximum output in any given *direction* */
    totalOutput = matData->areaLightColour.red;
    if (matData->areaLightColour.green > totalOutput)
    {
        totalOutput = matData->areaLightColour.green;
    }
    if (matData->areaLightColour.blue  > totalOutput)
    {
        totalOutput = matData->areaLightColour.blue;
    }
    totalOutput *= area;

    /* We're solving "(output / ROI^2) = error" hence "ROI = sqrt(output / error)"
     * NOTE: _rpLtMapGlobals.recipAreaLightROICutoff is inverted when set, so the
     * multiplication below really performs a division */
    rwSqrt(&roiRadius, (totalOutput * _rpLtMapGlobals.recipAreaLightROICutoff));

    /* Take into account the global and per-mesh radius multipliers */
    roiRadius *= _rpLtMapGlobals.areaLightRadius * matData->areaLightRadius;
    lightMesh->ROI = lightMesh->sphere.radius + roiRadius;

    RWRETURNVOID();
}

/****************************************************************************
 LtMapAreaLightSampleMeshes
 */
static RpMeshHeader *
LtMapAreaLightSampleMeshes(RpMeshHeader *meshHeader,
                           RwV3d        *vertices,
                           RtLtMapAreaLightGroup *lights,
                           RwMatrix     *matrix)
{
    LtMapMaterialData *matData;
    RpMesh            *mesh;
    RwUInt32           i, j;
    RwInt32            k;

    RWFUNCTION(RWSTRING("LtMapAreaLightSampleMeshes"));
    RWASSERT(NULL != meshHeader);
    RWASSERT(NULL != matrix);

    mesh = (RpMesh *)(((RwUInt8 *)(meshHeader + 1)) + meshHeader->firstMeshOffset);
    for (i = 0; i < meshHeader->numMeshes; i++, mesh++)
    {
        matData = RPLTMAPMATERIALGETDATA(mesh->material);

        /* Is this mesh an emitter? */
        if (rtLTMAPMATERIALAREALIGHT & matData->flags)
        {
            LtMapAreaLightMesh *lightMesh = NULL;
            RwBBox box;
            RwV3d triVerts[3];
            RwUInt32 donePolygons = 0, totalSamples = 0;
            RwReal meshArea;
            RwUInt32 meshStartIndex = 0;
            RwUInt32 numPolygons = mesh->numIndices - 2;
            RwReal density;

            /* Calculate density as the product of the material density modifier,
             * the global density modifier and the current default density. */
            density = rtLtMapGlobals.areaLightDensity *
                matData->areaLightDensity * _rpLtMapGlobals.areaLightDensity;

            if (!(meshHeader->flags & rpMESHHEADERPRIMMASK))
            {
                numPolygons = mesh->numIndices / 3;
            }

            /* We have to iterate over meshes (since they'll usually form
             * spatially-localised 'area lights'), so it's a bit messy */
            j = 0;
            while (j < mesh->numIndices-2)
            {
                RxVertexIndex indices[3];
                RwUInt32 startIndex = j;
                RwUInt32 degen,anydegen;

                /* skip over degenerates */
                anydegen = 0;
                do
                {
                    if (meshHeader->flags & rpMESHHEADERTRISTRIP)
                    {
                        indices[0] = j;
                        indices[1] = j + 1 + (j & 1);
                        indices[2] = j + 1 + ((~j) & 1);
                        j++;
                    }
                    else if (meshHeader->flags & rpMESHHEADERTRIFAN)
                    {
                        indices[0] = 0;
                        indices[1] = j + 1;
                        indices[2] = j + 2;
                        j++;
                    }
                    else
                    {
                        /* TriList */
                        RWASSERT(!(meshHeader->flags & rpMESHHEADERPRIMMASK));
                        indices[0] = j++;
                        indices[1] = j++;
                        indices[2] = j++;
                    }

                    degen = 0;
                    degen |= mesh->indices[indices[0]] == mesh->indices[indices[1]];
                    degen |= mesh->indices[indices[1]] == mesh->indices[indices[2]];
                    degen |= mesh->indices[indices[2]] == mesh->indices[indices[0]];
                    anydegen |= degen;
                }
                while (degen);

                for (k = 0;k < 3;k++)
                {
                    triVerts[k] = vertices[mesh->indices[indices[k]]];
                }

                /* Put the triangle in world-space */
                RwV3dTransformPoints(triVerts, triVerts, 3, matrix);

                /* do we need to create a new lightMesh? */
                if ((lightMesh == NULL) || anydegen)
                {
                    /* finalize current one */
                    if (lightMesh)
                    {
                        /* Calculate a ROI bounding sphere for the lightMesh */
                        LtMapAreaLightCalcMeshROI(lightMesh, meshArea, matData, &box);
                    }

                    /* Allocate a new LtMapAreaLightMesh & triangles */
                    lightMesh = (LtMapAreaLightMesh *)rwSListGetNewEntry(lights->meshes,
                                                 rwID_LTMAPPLUGIN | rwMEMHINTDUR_EVENT);
                    RWASSERT(NULL != lightMesh);
                    lightMesh->flags = 0;
                    lightMesh->material = mesh->material;
                    lightMesh->triangles = rwSListCreate(sizeof(LtMapAreaLight),
                                         rwID_LTMAPPLUGIN | rwMEMHINTDUR_EVENT);
                    RWASSERT(NULL != lightMesh->triangles);

                    /* reset stats */
                    meshStartIndex = startIndex;
                    meshArea = 0.0f;
                    totalSamples = 0;
                    donePolygons = 0;

                    /* Calculate a world-space bounding box for the mesh */
                    RwBBoxInitialize(&box, triVerts);
                }

                /* Calculate a world-space bounding box for the mesh */
                for (k = 0;k < 3;k++)
                {
                    RwBBoxAddPoint(&box, &triVerts[k]);
                }

                /* Generate world space positions */
                totalSamples += LtMapAreaLightSampleTriangle(lightMesh->triangles, &meshArea, triVerts, density);
                donePolygons++;

                if (totalSamples > _rpLtMapGlobals.areaLightSampleLimit)
                {
                    /* Early-out unless samples per poly is low (in which case
                     * shrinking density will have little effect, so you might
                     * as well leave it with one or two samples per polygon - to
                     * reduce numbers further you'd have to actually miss out
                     * triangles which'd make parts of the emitter black) */
/* TODO[2][ACI]: YOU COULD DO BETTER TO REDUCE DENSITY WHILST RETAINING A UNIFORM
 *         SAMPLING OF THE SURFACE, POSSIBLY USING GREEDY RASTERISATION AS MENTIONED
 *         IN LtMapAreaLightSampleTriangle ABOVE. EVEN SKIPPING TRIANGLES TO SAMPLE
 *         MORE SPARSELY MIGHT BE WORKABLE (SKIP BY *AREA* TO DEAL W/ TRIANGLES OF
 *         VARYING SIZE). EVEN THE CURRENT METHOD CAN BE IMPROVED IF YOU CALCULATE
 *         MAX OR VARIANCE OF TRIANGLE SIZE. */
                    if (totalSamples >= 2*donePolygons)
                    {
                        /* We use donePolygons to estimate the size of the whole mesh
                         * (given we've earlied out to save potentially a lot of time) */
                        RwReal fracDone = (RwReal)donePolygons / (RwReal)numPolygons;
                        RwReal temp;

                        /* A scaling of density will reduce numSamples by the
                         * square of that scaling */
                        rwSqrt(&temp, _rpLtMapGlobals.areaLightSampleLimit * fracDone / totalSamples);
                        density *= temp;

                        /* Debugging - we may come back to improving this rescaling */
                        /*{
                            RwChar string[256];
                            RwUInt32 percent = RwFastRealToUInt32(100*(1.0f - temp));
                            rwsprintf(string, "shrinking area light density of %s by %d%%",
                                      mesh->material->texture->name, percent);
                            RwDebugSendMessage(rwDEBUGMESSAGE, "", string);
                        }*/

                        /* Destroy the samples we just created and try again */
                        for (k = 0;k < rwSListGetNumEntries(lightMesh->triangles);k++)
                        {
                            LtMapAreaLight *triangle = (LtMapAreaLight *)rwSListGetEntry(lightMesh->triangles, k);

                            RwFree(triangle->lights);
                            triangle->lights = (RwV3d *)NULL;
                        }
                        rwSListEmpty(lightMesh->triangles);

                        /* start this again */
                        j = meshStartIndex;

                        /* reset stats */
                        meshArea = 0.0f;
                        totalSamples = 0;
                        donePolygons = 0;
                    }
                }
            }

            /* Calculate a ROI bounding sphere for the mesh */
            LtMapAreaLightCalcMeshROI(lightMesh, meshArea, matData, &box);
        } /* Is this mesh an emitter? */
    } /* For all meshes */

    RWRETURN(meshHeader);
}


/*===========================================================================*
 *--- Toolkit API Functions -------------------------------------------------*
 *===========================================================================*/

/* TODO[ABF][2][5]: create macro versions of all trivial set/get functions! */

/**
 * \ingroup rtltmap
 * \ref RtLtMapSetMaxAreaLightSamplesPerMesh is used to
 * limit the number of area light samples generated, per source
 * \ref RpMesh, by \ref RtLtMapAreaLightGroupCreate.
 *
 * This limit is useful to semi-automatically reduce the area light
 * sample density for large polygons, whilst leaving smaller polygons
 * with a high sample density. For a given light-emitting object,
 * increasing the number of area light samples soon becomes a case
 * of 'diminishing returns' with respect to the quality of the resultant
 * lighting solution. Since a world-space sample density is used to
 * generate area light samples, large polygons can end up with huge
 * numbers of samples, most of which contribute little to improving
 * lighting quality - this per-mesh limit should help avoid this
 * occurring.
 *
 * When area light samples have been created for a mesh and it is found
 * that there are too many samples, the sample density value is decreased
 * until an acceptable number of samples is generated. The distribution
 * of samples within the mesh remains even.
 *
 * The default limit value is given by rpLTMAPDEFAULTMAXAREALIGHTSAMPLESPERMESH.
 *
 * Note that to have any effect, this function must be used before a call
 * to \ref RtLtMapAreaLightGroupCreate.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  maxSamples  A \ref RwUInt32 specifying the maximum allowed number
 *                     of area light samples per light-emitting mesh
 *
 * \return TRUE on success, otherwise FALSE
 *
 * \see RtLtMapGetMaxAreaLightSamplesPerMesh
 * \see RtLtMapGetAreaLightDensityModifier
 * \see RtLtMapSetAreaLightDensityModifier
 * \see RtLtMapMaterialGetAreaLightDensityModifier
 * \see RtLtMapMaterialSetAreaLightDensityModifier
 * \see RtLtMapAreaLightGroupCreate
 * \see RtLtMapAreaLightGroupDestroy
 * \see RtLtMapIlluminate
 */
RwBool
RtLtMapSetMaxAreaLightSamplesPerMesh(RwUInt32 maxSamples)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapSetMaxAreaLightSamplesPerMesh"));

    _rpLtMapGlobals.areaLightSampleLimit = maxSamples;

    RWRETURN(TRUE);
}


/**
 * \ingroup rtltmap
 * \ref RtLtMapGetMaxAreaLightSamplesPerMesh is used to
 * retrieve the maximum number of area light samples which will
 * be  generated, per source \ref RpMesh, by \ref RtLtMapAreaLightGroupCreate.
 *
 * See \ref RtLtMapSetMaxAreaLightSamplesPerMesh for further details.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \return The current maximum number of samples per light-emitting mesh
 *
 * \see RtLtMapSetMaxAreaLightSamplesPerMesh
 * \see RtLtMapGetAreaLightDensityModifier
 * \see RtLtMapSetAreaLightDensityModifier
 * \see RtLtMapMaterialGetAreaLightDensityModifier
 * \see RtLtMapMaterialSetAreaLightDensityModifier
 * \see RtLtMapAreaLightGroupCreate
 * \see RtLtMapAreaLightGroupDestroy
 * \see RtLtMapIlluminate
 */
RwUInt32
RtLtMapGetMaxAreaLightSamplesPerMesh(void)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapGetMaxAreaLightSamplesPerMesh"));

    RWRETURN(_rpLtMapGlobals.areaLightSampleLimit);
}


/**
 * \ingroup rtltmap
 * \ref RtLtMapSetAreaLightDensityModifier is used to
 * modify the spacing of area light samples generated by
 * \ref RtLtMapAreaLightGroupCreate.
 *
 * The area light sample density modifier is used as a
 * multiplicative modifier to the default value. Hence, setting
 * a value of 2 will double the (linear) density of area light
 * samples, halving the distance between adjacent samples and
 * resulting in approximately four times as many samples being
 * generated. Overall brightness should not be affected by
 * the density modifier, just the quality (accuracy/softness)
 * of the resultant lighting solution.
 *
 * The default modifier is one (identity).
 *
 * See also \ref RtLtMapMaterialSetAreaLightDensityModifier. The
 * per-material modifier multiplies this global modifier.
 *
 * Note that to have any effect, this function must be used before a call
 * to \ref RtLtMapAreaLightGroupCreate.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param   modifier  A \ref RwReal area light density modifier
 *
 * \return TRUE on success, otherwise FALSE
 *
 * \see RtLtMapGetAreaLightDensityModifier
 * \see RtLtMapMaterialGetAreaLightDensityModifier
 * \see RtLtMapMaterialSetAreaLightDensityModifier
 * \see RtLtMapAreaLightGroupCreate
 * \see RtLtMapAreaLightGroupDestroy
 * \see RtLtMapIlluminate
 */
RwBool
RtLtMapSetAreaLightDensityModifier(RwReal modifier)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapSetAreaLightDensityModifier"));
    RWASSERT(modifier > 0);

    _rpLtMapGlobals.areaLightDensity = modifier;

    RWRETURN(TRUE);
}


/**
 * \ingroup rtltmap
 * \ref RtLtMapGetAreaLightDensityModifier is used to retrieve
 * the current modifier of the spacing between the area light
 * samples generated by \ref RtLtMapAreaLightGroupCreate.
 *
 * See \ref RtLtMapSetAreaLightDensityModifier for further details.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \return the current area light density modifier
 *
 * \see RtLtMapSetAreaLightDensityModifier
 * \see RtLtMapMaterialGetAreaLightDensityModifier
 * \see RtLtMapMaterialSetAreaLightDensityModifier
 * \see RtLtMapAreaLightGroupCreate
 * \see RtLtMapAreaLightGroupDestroy
 * \see RtLtMapIlluminate
 */
RwReal
RtLtMapGetAreaLightDensityModifier(void)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapGetAreaLightDensityModifier"));

    RWRETURN(_rpLtMapGlobals.areaLightDensity);
}


/**
 * \ingroup rtltmap
 * \ref RtLtMapSetAreaLightRadiusModifier is used to modify
 * the falloff of all area light sources.
 *
 * The area light source radius modifier can be used to effectively
 * reduce or increase the intensity of light emitted from area light
 * sources. As an example, if the modifier is changed from one to two
 * then the same intensity of light will now be received at twice the
 * distance from a light source.
 *
 * The default modifier is one (identity).
 *
 * See also \ref RtLtMapMaterialSetAreaLightRadiusModifier. The per-material
 * modifier multiplies this global modifier.
 *
 * Note that to have any effect, this function must be used before a call
 * to \ref RtLtMapAreaLightGroupCreate.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  modifier  A \ref RwReal value specifying the radius modifier
 *                   for area light sources
 *
 * \return TRUE on success, otherwise FALSE
 *
 * \see RtLtMapGetAreaLightRadiusModifier
 * \see RtLtMapMaterialGetAreaLightRadiusModifier
 * \see RtLtMapMaterialSetAreaLightRadiusModifier
 * \see RtLtMapAreaLightGroupCreate
 * \see RtLtMapAreaLightGroupDestroy
 * \see RtLtMapIlluminate
 */
RwBool
RtLtMapSetAreaLightRadiusModifier(RwReal modifier)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapSetAreaLightRadiusModifier"));

    /* Hmm, might be able to do interesting things with a negative modifier! :) */
    RWASSERT(modifier > 0);

    _rpLtMapGlobals.areaLightRadius = modifier;

    RWRETURN(TRUE);
}


/**
 * \ingroup rtltmap
 * \ref RtLtMapGetAreaLightRadiusModifier is used to retrieve
 * the current global area light source intensity falloff modifier.
 *
 * See \ref RtLtMapSetAreaLightRadiusModifier for further details.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \return the current area light source intensity falloff modifier
 *
 * \see RtLtMapSetAreaLightRadiusModifier
 * \see RtLtMapMaterialGetAreaLightRadiusModifier
 * \see RtLtMapMaterialSetAreaLightRadiusModifier
 * \see RtLtMapAreaLightGroupCreate
 * \see RtLtMapAreaLightGroupDestroy
 * \see RtLtMapIlluminate
 */
RwReal
RtLtMapGetAreaLightRadiusModifier(void)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapGetAreaLightRadiusModifier"));

    RWRETURN(_rpLtMapGlobals.areaLightRadius);
}


/**
 * \ingroup rtltmap
 * \ref RtLtMapSetAreaLightErrorCutoff is used to determine
 * the error tolerance in enforcing a finite Region Of Influence
 * (R.O.I.) on area light sources.
 *
 * Area light samples are treated, during lighting calculations,
 * as having an inverse square law for light intensity falloff.
 * For efficiency, area light samples (and also hierarchical
 * groups of samples) are given a finite R.O.I, so that visibility
 * does not have to be tested between all area lights and all sample
 * points in the scene.
 *
 * The error tolerance value specifies the intensity of an area light
 * source at the limit of its R.O.I. In other words, light intensity
 * will drop with increasing distance from the light source, until
 * it reaches the limit of the light's R.O.I. - at which point the
 * intensity will immediately drop to zero, introducing a discontinuity.
 * The magnitude of this discontinuity if the error tolerance value.
 *
 * Hence, a larger error tolerance value will reduce the R.O.I.s of
 * area light sources (which will accelerate lighting calculations)
 * but may result in visible discontinuities in lightmaps.
 *
 * The error tolerance value is given in terms of perceived brightness
 * (which is related logarithmically to light intensity, as described
 * in \ref RtLtMapIlluminate) and should be in the range zero to 255.
 * This corresponds to RwRGBA values in the resultant lightmaps.
 *
 * The default error tolerance value is given by rpLTMAPDEFAULTrecipAreaLightROICutoff.
 *
 * Note that to have any effect, this function must be used before a call
 * to \ref RtLtMapAreaLightGroupCreate.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  tolerance  A \ref RwReal specifying the intensity below
 *                    which area light sources are to be ignored.
 *
 * \return TRUE on success, otherwise FALSE
 *
 * \see RtLtMapGetAreaLightErrorCutoff
 * \see RtLtMapAreaLightGroupCreate
 * \see RtLtMapAreaLightGroupDestroy
 * \see RtLtMapIlluminate
 */
RwBool
RtLtMapSetAreaLightErrorCutoff(RwReal tolerance)
{
    RwReal intensity;

    RWAPIFUNCTION(RWSTRING("RtLtMapSetAreaLightErrorCutoff"));

    /* There's no point in going below a certain intensity, really...
     * The default ROI is as conservative as necessary for a single
     * light source... if the contributions from many samples sum up
     * (and normally they would be in the same mesh and hence have a
     * single, larger ROI) then you might get slightly more accurate
     * results in the lowest bit if you reduce the ROI intensity further. */
    RWASSERT(tolerance >= 0);
    RWASSERT(tolerance <= 255);

#if 0
    intensity = tolerance;
#else

    /* Just using the tolerance gives a much better result.  We need to investigate
       why and work through the brightness vs intensity model. 
     */
    intensity = tolerance;
#endif

    /* Store the reciprocal for efficiency of usage */
    _rpLtMapGlobals.recipAreaLightROICutoff = 1.0f / intensity;

    RWRETURN(TRUE);
}


/**
 * \ingroup rtltmap
 * \ref RtLtMapGetAreaLightErrorCutoff is used to retrieve the
 * current error tolerance used in enforcing a finite Region Of
 * Influence (R.O.I.) on area light sources.
 *
 * See \ref RtLtMapSetAreaLightErrorCutoff for further details.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \return the current area lighting error cutoff value
 *
 * \see RtLtMapSetAreaLightErrorCutoff
 * \see RtLtMapAreaLightGroupCreate
 * \see RtLtMapAreaLightGroupDestroy
 * \see RtLtMapIlluminate
 */
RwReal
RtLtMapGetAreaLightErrorCutoff(void)
{
#if 0        
    RwReal intensity, tolerance;
#else
    RwReal intensity;
#endif

    RWAPIFUNCTION(RWSTRING("RtLtMapGetAreaLightErrorCutoff"));

    /* We store the reciprocal for efficiency of usage */
    intensity = 1.0f / _rpLtMapGlobals.recipAreaLightROICutoff;

#if 0    
    tolerance = intensity;

    RWRETURN(tolerance);
#else
    RWRETURN(intensity);
#endif
}


/**
 * \ingroup rtltmap
 * \ref RtLtMapMaterialSetAreaLightRadiusModifier is used to modify
 * the falloff of light emitted from triangles with this material.
 *
 * The radius modifier of a light-emitting material can be used to effectively
 * reduce or increase the intensity of emitted light. As an example, if the
 * modifier is changed from one to two then the same intensity of light will
 * now be received at twice the distance from the light source.
 *
 * The default modifier for a material is one (identity).
 *
 * Note that to have any effect, this function must be used before a call
 * to \ref RtLtMapAreaLightGroupCreate.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  material  A pointer to an \ref RpMaterial
 * \param  modifier  A \ref RwReal value specifying the area light
 *                   radius modifier for this material
 *
 * \return A pointer to the material on success, otherwise NULL
 *
 * \see RtLtMapMaterialGetAreaLightRadiusModifier
 * \see RtLtMapMaterialGetAreaLightDensityModifier
 * \see RtLtMapMaterialSetAreaLightDensityModifier
 * \see RtLtMapMaterialGetAreaLightColor
 * \see RtLtMapMaterialSetAreaLightColor
 * \see RtLtMapMaterialGetFlags
 * \see RtLtMapMaterialSetFlags
 * \see RtLtMapAreaLightGroupCreate
 * \see RtLtMapAreaLightGroupDestroy
 * \see RtLtMapIlluminate
 */
RpMaterial *
RtLtMapMaterialSetAreaLightRadiusModifier(RpMaterial *material, RwReal modifier)
{
    LtMapMaterialData *matData;

    RWAPIFUNCTION(RWSTRING("RtLtMapMaterialSetAreaLightRadiusModifier"));

    RWASSERT(NULL != material);
    matData = RPLTMAPMATERIALGETDATA(material);

    RWASSERT(modifier > 0);
    matData->areaLightRadius = modifier;

    RWRETURN(material);
}


/**
 * \ingroup rtltmap
 * \ref RtLtMapMaterialGetAreaLightRadiusModifier is used to retrieve
 * the area light radius modifier for a material.
 *
 * The radius modifier of a light-emitting material effectively reduces
 * or increases the intensity of emitted light. As an example, if the
 * modifier is changed from one to two then the same intensity of light
 * will now be received at twice the distance from the light source.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  material  A pointer to an \ref RpMaterial
 *
 * \return The area light radius modified of the specified material
 *
 * \see RtLtMapMaterialSetAreaLightRadiusModifier
 * \see RtLtMapMaterialGetAreaLightDensityModifier
 * \see RtLtMapMaterialSetAreaLightDensityModifier
 * \see RtLtMapMaterialGetAreaLightColor
 * \see RtLtMapMaterialSetAreaLightColor
 * \see RtLtMapMaterialGetFlags
 * \see RtLtMapMaterialSetFlags
 * \see RtLtMapAreaLightGroupCreate
 * \see RtLtMapAreaLightGroupDestroy
 * \see RtLtMapIlluminate
 */
RwReal
RtLtMapMaterialGetAreaLightRadiusModifier(RpMaterial *material)
{
    LtMapMaterialData *matData;

    RWAPIFUNCTION(RWSTRING("RtLtMapMaterialGetAreaLightRadiusModifier"));

    RWASSERT(NULL != material);
    matData = RPLTMAPMATERIALGETDATA(material);

    RWRETURN(matData->areaLightRadius);
}


/**
 * \ingroup rtltmap
 * \ref RtLtMapMaterialSetAreaLightDensityModifier is used
 * to modify the spacing of area light samples generated by
 * \ref RtLtMapAreaLightGroupCreate from triangles with this
 * material (and flagged as being area light emitters with
 * \ref rtLTMAPMATERIALAREALIGHT).
 *
 * The area light sample density modifier of a material is used as
 * a multiplicative modifier to the default value. Hence, setting
 * a value of 2 will double the (linear) density of area light
 * samples, halving the distance between adjacent samples and
 * resulting in approximately four times as many samples being
 * generated (from triangles using this material). Overall
 * brightness should not be affected by the density modifier,
 * just the quality (accuracy/softness) of the resultant lighting
 * solution.
 *
 * Note that to have any effect, this function must be used before a call
 * to \ref RtLtMapAreaLightGroupCreate.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  material  A pointer to an \ref RpMaterial for which to set
 *                   the area-lighting sample density
 * \param  modifier  A \ref RwReal value specifying the area light
 *                   density modifier for this material
 *
 * \return A pointer to the material on success, otherwise NULL
 *
 * \see RtLtMapMaterialGetAreaLightDensityModifier
 * \see RtLtMapMaterialSetFlags
 * \see RtLtMapMaterialGetFlags
 * \see RtLtMapMaterialSetAreaLightColor
 * \see RtLtMapMaterialGetAreaLightColor
 * \see RtLtMapMaterialGetAreaLightRadiusModifier
 * \see RtLtMapMaterialSetAreaLightRadiusModifier
 * \see RtLtMapAreaLightGroupCreate
 * \see RtLtMapAreaLightGroupDestroy
 * \see RtLtMapIlluminate
 */
RpMaterial *
RtLtMapMaterialSetAreaLightDensityModifier(RpMaterial *material, RwReal modifier)
{
    LtMapMaterialData *matData;

    RWAPIFUNCTION(RWSTRING("RtLtMapMaterialSetAreaLightDensityModifier"));

    RWASSERT(NULL != material);
    matData = RPLTMAPMATERIALGETDATA(material);

    RWASSERT(modifier > 0);
    matData->areaLightDensity = modifier;

    RWRETURN(material);
}


/**
 * \ingroup rtltmap
 * \ref RtLtMapMaterialGetAreaLightDensityModifier is used
 * to retrieve the area light sample density modifier for
 * a material.
 *
 * The area light sample density modifier of a material is used as
 * a multiplicative modifier to the default value. Hence, setting
 * a value of 2 will double the (linear) density of area light
 * samples, halving the distance between adjacent samples and
 * resulting in approximately four times as many samples being
 * generated (from triangles using this material).
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  material  A pointer to an \ref RpMaterial
 *
 * \return The area light sample density modifier for this material
 *
 * \see RtLtMapMaterialSetAreaLightDensityModifier
 * \see RtLtMapMaterialSetFlags
 * \see RtLtMapMaterialGetFlags
 * \see RtLtMapMaterialSetAreaLightColor
 * \see RtLtMapMaterialGetAreaLightColor
 * \see RtLtMapMaterialGetAreaLightRadiusModifier
 * \see RtLtMapMaterialSetAreaLightRadiusModifier
 * \see RtLtMapAreaLightGroupCreate
 * \see RtLtMapAreaLightGroupDestroy
 * \see RtLtMapIlluminate
 */
RwReal
RtLtMapMaterialGetAreaLightDensityModifier(RpMaterial *material)
{
    LtMapMaterialData *matData;

    RWAPIFUNCTION(RWSTRING("RtLtMapMaterialGetAreaLightDensityModifier"));

    RWASSERT(NULL != material);
    matData = RPLTMAPMATERIALGETDATA(material);

    RWRETURN(matData->areaLightDensity);
}


/**
 * \ingroup rtltmap
 * \ref RtLtMapMaterialSetAreaLightColor is used to set the color of
 * area light samples generated by \ref RtLtMapAreaLightGroupCreate
 * from triangles with this material (and flagged as being area
 * light emitters with \ref rtLTMAPMATERIALAREALIGHT).
 *
 * Whilst the color of an area light emitter is only specified using
 * a limited-range \ref RwRGBA value, this may be effectively reduced
 * or increased in intensity through
 * \ref RtLtMapMaterialSetAreaLightRadiusModifier, which takes a
 * high-dynamic-range \ref RwReal value.
 *
 * The default color of an area light emitter is {255, 255, 255}. The
 * alpha value is currently unused.
 *
 * Note that to have any effect, this function must be used before a call
 * to \ref RtLtMapAreaLightGroupCreate.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param   material  A pointer to an \ref RpMaterial
 * \param   color     An \ref RwRGBA value specifying the color of light
 *                    emitted from triangles using this material
 *
 * \return A pointer to the material on success, otherwise NULL
 *
 * \see RtLtMapMaterialGetAreaLightColor
 * \see RtLtMapMaterialSetFlags
 * \see RtLtMapMaterialGetFlags
 * \see RtLtMapMaterialSetAreaLightDensityModifier
 * \see RtLtMapMaterialGetAreaLightDensityModifier
 * \see RtLtMapMaterialGetAreaLightRadiusModifier
 * \see RtLtMapMaterialSetAreaLightRadiusModifier
 * \see RtLtMapAreaLightGroupCreate
 * \see RtLtMapAreaLightGroupDestroy
 * \see RtLtMapIlluminate
 */
RpMaterial *
RtLtMapMaterialSetAreaLightColor(RpMaterial *material, RwRGBA color)
{
    LtMapMaterialData *matData;

    RWAPIFUNCTION(RWSTRING("RtLtMapMaterialSetAreaLightColor"));

    RWASSERT(NULL != material);
    matData = RPLTMAPMATERIALGETDATA(material);

    matData->areaLightColour = color;

    RWRETURN(material);
}

/**
 * \ingroup rtltmap
 * \ref RtLtMapMaterialGetAreaLightColor is used to retrieve the color
 * of area light samples generated by \ref RtLtMapAreaLightGroupCreate
 * from triangles with this material (and flagged as being area
 * light emitters with \ref rtLTMAPMATERIALAREALIGHT).
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param   material  A pointer to an \ref RpMaterial
 *
 * \return The color of this light-emitting material
 *
 * \see RtLtMapMaterialSetAreaLightColor
 * \see RtLtMapMaterialSetFlags
 * \see RtLtMapMaterialGetFlags
 * \see RtLtMapMaterialSetAreaLightDensityModifier
 * \see RtLtMapMaterialGetAreaLightDensityModifier
 * \see RtLtMapMaterialGetAreaLightRadiusModifier
 * \see RtLtMapMaterialSetAreaLightRadiusModifier
 * \see RtLtMapAreaLightGroupCreate
 * \see RtLtMapAreaLightGroupDestroy
 * \see RtLtMapIlluminate
 */
RwRGBA
RtLtMapMaterialGetAreaLightColor(RpMaterial *material)
{
    LtMapMaterialData *matData;

    RWAPIFUNCTION(RWSTRING("RtLtMapMaterialGetAreaLightColor"));

    RWASSERT(NULL != material);
    matData = RPLTMAPMATERIALGETDATA(material);

    RWRETURN(matData->areaLightColour);
}


/**
 * \ingroup rtltmap
 * \ref RtLtMapAreaLightGroupCreate is used to generate a set of area light samples
 * from the geometry in an \ref RtLtMapLightingSession.
 *
 * Area lighting is performed, within RtLtMap, by generating a set of point light
 * 'samples' across the surface of geometry which is flagged as being a light
 * emitter. This set of point lights approximates, to a higher or lower degree,
 * the 'true' emission of light from a surface. The more samples are generated,
 * the more accurate the result will be, but the longer it will take to compute.
 *
 * Each light sample is treated as a point light (similar to an \ref RpLight
 * of type \ref rpLIGHTPOINT) with an inverse square law intensity falloff.
 * It has an associated area of surface which it represents, such that for
 * a sample of unit area and unit brightness, light intensity will be also
 * be unit at unit distance along the sample's normal (which is the normal
 * of the triangle from which it was sourced). This means that, if a scene
 * is uniformly scaled by any factor, the resulting lighting solution should
 * look the same. See \ref RtLtMapIlluminate for further details on how light
 * contributions are summed and post-processed.
 *
 * This function traverses the triangles of the objects specified by the received
 * \ref RtLtMapLightingSession structure. Only atomics flagged as rpATOMICRENDER
 * will be used. Note that the camera member of this structure is ignored,
 * because lights outside of the camera frustum can light geometry within
 * the camera frustum. Area light samples are generated for triangles whose
 * material is flagged with \ref rtLTMAPMATERIALAREALIGHT (see
 * \ref RtLtMapMaterialFlags). Various functions (listed below) can
 * be used to modify the area lighting properties of a given material.
 *
 * This function returns an \ref RtLtMapAreaLightGroup, containing the area light
 * samples, which may later be passed to \ref RtLtMapIlluminate. These containers
 * hierarchically group area light samples, such that they can be efficiently
 * culled during lighting calculations (naively testing visibility from each
 * lightmap sample to every single area light sample in the scene - of which
 * there may be many thousand - can be extremely, and unnecessarily, expensive).
 * Several \ref RtLtMapAreaLightGroup structures may be chained together, such
 * that area light samples sourced from multiple \ref RtLtMapLightingSession's
 * may be passed to \ref RtLtMapIlluminate simultaneously. Each container may
 * have a pointer to an RwFrame, such that, for example, area light samples
 * from multiple worlds, which are connected by portals, or which are composed
 * of atomics and not world sectors, may be used within a single call to
 * \ref RtLtMapIlluminate).
 *
 * The density parameter of this function specifies the number of area light
 * samples to be generated per unit world-space. The higher this value is, the
 * more samples will be generated. If this value is set to zero, a default value
 * will be used, which is twice the lightmap sample density for the world of
 * the current \ref RtLtMapLightingSession. If this value is zero (i.e the
 * world has not yet been used in a call to \ref RtLtMapLightMapsCreate) then
 * a sample density is chosen such that 400 area light samples would fit, in a
 * straight line, into the diagonal 'length' of the world's bounding box. The
 * density value may be modified by material plugin data (see
 * \ref RtLtMapMaterialSetAreaLightDensityModifier) or a global modifier
 * (see \ref RtLtMapSetAreaLightDensityModifier).
 *
 * For triangles within atomics which have multiple morph targets, the positions
 * of the vertices specified in the 'start' morph target of the atomic's
 * interpolator will be used.
 *
 * Other functions available to tweak area lighting calculations are:
 * \li \ref RtLtMapSetMaxAreaLightSamplesPerMesh
 * \li \ref RtLtMapSetAreaLightRadiusModifier
 * \li \ref RtLtMapSetAreaLightErrorCutoff
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param  session  A pointer to an \ref RtLtMapLightingSession structure specifying
 *                  the set of objects from which to create area light samples
 * \param  density  A \ref RwReal specifying the density of area light samples
 *                  in world-space
 *
 * \return A pointer to the new area light group on success, otherwise NULL
 *
 * \see RtLtMapAreaLightGroupDestroy
 * \see RtLtMapIlluminate
 * \see RtLtMapSetMaxAreaLightSamplesPerMesh
 * \see RtLtMapSetAreaLightRadiusModifier
 * \see RtLtMapSetAreaLightErrorCutoff
 * \see RtLtMapMaterialGetFlags
 * \see RtLtMapMaterialSetFlags
 * \see RtLtMapMaterialGetAreaLightColor
 * \see RtLtMapMaterialSetAreaLightColor
 * \see RtLtMapMaterialGetAreaLightDensityModifier
 * \see RtLtMapMaterialSetAreaLightDensityModifier
 * \see RtLtMapMaterialSetAreaLightRadiusModifier
 * \see RtLtMapMaterialGetAreaLightRadiusModifier
 */
RtLtMapAreaLightGroup *
RtLtMapAreaLightGroupCreate(RtLtMapLightingSession *session, RwReal density)
{
    RtLtMapAreaLightGroup *group;
    LtMapSessionInfo sessionInfo;
    RwUInt32 i;

    RWAPIFUNCTION(RWSTRING("RtLtMapAreaLightGroupCreate"));

    RWASSERT(NULL != session);
    _rtLtMapLightingSessionInfoCreate(&sessionInfo, session, FALSE);

    group = (RtLtMapAreaLightGroup *)RwMalloc(sizeof(RtLtMapAreaLightGroup),
                                     rwID_LTMAPPLUGIN | rwMEMHINTDUR_EVENT);
    RWASSERT(NULL != group);
    group->frame = (RwFrame *)NULL;
    group->next  = (RtLtMapAreaLightGroup *)NULL;

    /* Create a blank list for new lights */
    group->meshes = rwSListCreate(sizeof(LtMapAreaLightMesh),
                      rwID_LTMAPPLUGIN | rwMEMHINTDUR_EVENT);
    RWASSERT(NULL != group->meshes);

    if (0 == density)
    {
        if (NULL != session->world)
        {
            LtMapWorldData *worldData;

            /* Default to twice this world's lightmap density */
            worldData = RPLTMAPWORLDGETDATA(session->world);
            density = 2 * worldData->lightMapDensity;
            if (0 == density)
            {
                /* Default to the world's 'length' over 400 */
                const RwBBox *box;
                RwV3d length;

                RwDebugSendMessage(rwDEBUGMESSAGE, "RtLtMapAreaLightGroupCreate",
                    "RtLtMapWorldCreateLightMaps() has not been called for this world yet, hence lightmap samples density is undefined. Area light sample density will be automatically calculated but its value may not be a good one");

                box = RpWorldGetBBox(session->world);
                RwV3dSub(&length, &(box->sup), &(box->inf));
                density = 400.0f / RwV3dLength(&length);
            }
        }
        else
        {
            density = 1;
        }
    }
    rtLtMapGlobals.areaLightDensity = density;

    for (i = 0;i < (RwUInt32)sessionInfo.numSectors;i++)
    {
        RpWorldSector *sector = *(RpWorldSector **)rwSListGetEntry(sessionInfo.localSectors, i);
        RwMatrix identity;

        RwMatrixSetIdentity(&identity);
        if (sector->numTriangles > 0)
        {
            RWASSERT(NULL != sector->vertices);
            LtMapAreaLightSampleMeshes(sector->mesh, sector->vertices, group, &identity);
        }
    }

    for (i = 0; i < (RwUInt32)sessionInfo.numAtomics; i++)
    {
        RpAtomic *atomic = *(RpAtomic **)rwSListGetEntry(sessionInfo.localAtomics, i);
        RpGeometry *geom = RpAtomicGetGeometry(atomic);

        if ((RpAtomicGetFlags(atomic) & rpATOMICRENDER) && (RpGeometryGetNumTriangles(geom) > 0))
        {
            RpGeometry    *geom;
            RwUInt32       morphNum;
            RpMorphTarget *morphTarget;

            geom = RpAtomicGetGeometry(atomic);
            RWASSERT(NULL != geom);

            /* If it's morph animated, the lightmaps may well look silly, but it'll still work */
            morphNum    = RpInterpolatorGetStartMorphTarget(RpAtomicGetInterpolator(atomic));
            morphTarget = RpGeometryGetMorphTarget(geom, morphNum);
            RWASSERT(NULL != morphTarget);
            RWASSERT(NULL != morphTarget->verts);

            /* Create area lights from the meshes in this atomic */
            LtMapAreaLightSampleMeshes(geom->mesh, morphTarget->verts, group, RwFrameGetLTM(RpAtomicGetFrame(atomic)));
        }
    }

    _rtLtMapLightingSessionInfoDestroy(&sessionInfo);

    RWRETURN(group);
}

/**
 * \ingroup rtltmap
 * \ref RtLtMapAreaLightGroupDestroy is used to destroy an area
 * light samples container created by \ref RtLtMapAreaLightGroupCreate.
 *
 * The lightmap plugin must be attached before using this function.
 *
 * \param   group  A pointer to an area light samples container to destroy
 *
 * \return TRUE on success, FALSE otherwise
 *
 * \see RtLtMapAreaLightGroupCreate
 * \see RtLtMapIlluminate
 */
RwBool
RtLtMapAreaLightGroupDestroy(RtLtMapAreaLightGroup *group)
{
    RwInt32 i, j;

    RWAPIFUNCTION(RWSTRING("RtLtMapAreaLightGroupDestroy"));
    RWASSERT(NULL != group);

    for (i = 0;i < rwSListGetNumEntries(group->meshes);i++)
    {
        LtMapAreaLightMesh *mesh = (LtMapAreaLightMesh *)rwSListGetEntry(group->meshes, i);

        for (j = 0;j < rwSListGetNumEntries(mesh->triangles);j++)
        {
            LtMapAreaLight *triangle = (LtMapAreaLight *)rwSListGetEntry(mesh->triangles, j);

            RwFree(triangle->lights);
        }

        rwSListDestroy(mesh->triangles);
    }

    rwSListDestroy(group->meshes);
    RwFree(group);

    RWRETURN(TRUE);
}
