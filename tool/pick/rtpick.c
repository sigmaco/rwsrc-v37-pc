/*
 * Functions for picking objects at specific camera raster coordinates.
 *
 * Copyright (c) Criterion Software Limited
 */

/**
 * \ingroup rtpick
 * \page rtpickoverview RtPick Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rpcollis.h, rtpick.h
 * \li \b Libraries: rwcore, rpworld.h, rpcollis, rtintsec, rtpick
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpCollisionPluginAttach
 *
 * \subsection pickoverview Overview
 * This toolkit provides some utilities for picking out objects in a scene
 * that lie under a particular pixel location of a camera's image raster.
 * This is useful for manipulating objects with a mouse or other controller.
 *
 * The following functions are available:
 *
 * \li \ref RwCameraCalcPixelRay calculates the parameters of a line through
 *          a particular pixel ray.
 * \li \ref RpWorldPickAtomicOnLine finds the nearest atomic along a line
 *          in the given world.
 * \li \ref RwCameraPickAtomicOnPixel finds the closest atomic lying
 *          under a particular pixel of the camera raster.
 */

/****************************************************************************
 Includes
 */

#include "rwcore.h"
#include "rpworld.h"
#include "rpcollis.h"
#include "rpdbgerr.h"

#include "rtpick.h"

/****************************************************************************
 Local Types
 */

typedef struct rtPickData rtPickData;
struct rtPickData
{
    RpAtomic           *closestAtomic;
    RwReal              closestDistance;
    RpWorldSector      *sector;
};

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */

/**
 * \ingroup rtpick
 * \ref RwCameraCalcPixelRay is used to determine the parameters of a
 * line which passes through the specified camera's frustum at the given
 * pixel location. The line starts and ends on the camera's near and far clip
 * planes, respectively, and is specified in world units.
 *
 * This function is useful if we wish to calculate
 * intersections of the line with atomics located within the frustum. This
 * enables us to identify the atomics corresponding to a user pick event
 * on the camera's image raster.
 *
 * The world plugin must be attached before using this function, and
 * the rtpick library is required (header file rtpick.h).
 *
 * \param camera  Pointer to the camera.
 * \param line  Pointer to a RwLine value which will receive the generated ray.
 * \param pixel  Pointer to a RwV2d value equal to the position of the picked pixel.
 *
 * \return Returns pointer to the camera if successful or NULL if
 * there is an error.
 *
 * \see RwCameraPickAtomicOnPixel
 * \see RpWorldPickAtomicOnLine
 * \see RpWorldForAllAtomicIntersections
 * \see RpWorldForAllWorldSectorIntersections
 * \see RpWorldPluginAttach
 *
 */
const RwCamera     *
RwCameraCalcPixelRay(const RwCamera * camera, RwLine * line,
                     const RwV2d * pixel)
{
    RwV2d               normalizedPos;
    RwV3d               topVert, botVert;
    RwInt32             rasWidth;
    RwInt32             rasHeight;
    const RwV3d        *frustVerts;

    RWAPIFUNCTION(RWSTRING("RwCameraCalcPixelRay"));
    RWASSERT(line);
    RWASSERT(camera);
    RWASSERTISTYPE(camera, rwCAMERA);
    RWASSERT(pixel);

    frustVerts = camera->frustumCorners;

    /* We're going to do this by interpolating between
     * frustum vertices on the front and back planes.
     */
    rasWidth = RwRasterGetWidth(RwCameraGetRaster(camera));
    normalizedPos.x = (pixel->x) / ((RwReal) rasWidth);
    rasHeight = RwRasterGetHeight(RwCameraGetRaster(camera));
    normalizedPos.y = (pixel->y) / ((RwReal) rasHeight);

    /* Figure out top and bottom on the front plane */
    RwV3dSub(&topVert, &frustVerts[2], &frustVerts[3]);
    RwV3dScale(&topVert, &topVert, normalizedPos.x);
    RwV3dAdd(&topVert, &topVert, &frustVerts[3]);
    RwV3dSub(&botVert, &frustVerts[1], &frustVerts[0]);
    RwV3dScale(&botVert, &botVert, normalizedPos.x);
    RwV3dAdd(&botVert, &botVert, &frustVerts[0]);

    /* Now interpolate between top and bottom */
    RwV3dSub(&line->start, &topVert, &botVert);
    RwV3dScale(&line->start, &line->start, normalizedPos.y);
    RwV3dAdd(&line->start, &line->start, &botVert);

    /* Figure out top and bottom on the back plane */
    RwV3dSub(&topVert, &frustVerts[6], &frustVerts[7]);
    RwV3dScale(&topVert, &topVert, normalizedPos.x);
    RwV3dAdd(&topVert, &topVert, &frustVerts[7]);
    RwV3dSub(&botVert, &frustVerts[5], &frustVerts[4]);
    RwV3dScale(&botVert, &botVert, normalizedPos.x);
    RwV3dAdd(&botVert, &botVert, &frustVerts[4]);

    /* Now interpolate between top and bottom */
    RwV3dSub(&line->end, &topVert, &botVert);
    RwV3dScale(&line->end, &line->end, normalizedPos.y);
    RwV3dAdd(&line->end, &line->end, &botVert);

    /* Done */
    RWRETURN(camera);
}

/****************************************************************************
 *
 *  RpWorldPickAtomicOnLine() - atomic callback
 */
static RpAtomic    *
TestAtomicForCloseness(RpIntersection * intersection __RWUNUSED__,
                       RpWorldSector * sector,
                       RpAtomic * atomic, RwReal distance, void *data)
{
    rtPickData         *pickData = (rtPickData *) data;

    RWFUNCTION(RWSTRING("TestAtomicForCloseness"));
    RWASSERT(atomic);
    RWASSERT(sector);
    RWASSERT(data);

    if (pickData->closestAtomic &&
        pickData->sector != sector)
    {
        /* We've got an atomic, we've gone to the next sector, we go
         * from front to back, we can safely early out
         */
        RWRETURN(NULL);
    }

    if ((!pickData->closestAtomic)
        || (distance < pickData->closestDistance))
    {
        /* This'll do nicely */
        pickData->closestAtomic = atomic;
        pickData->closestDistance = distance;
        pickData->sector = sector;
    }

    RWRETURN(atomic);
}

/**
 * \ingroup rtpick
 * \ref RpWorldPickAtomicOnLine is used to determine the atomic in the
 * specified world that intersects the given line closest to its start point.
 * The parameters of the line, its start and end positions, are specified
 * in world units.
 *
 * Note that this function only tests the atomic's bounding sphere to
 * determine if it has been picked. For a more accurate test based on the
 * atomic's geometry use \ref RpAtomicForAllIntersections.
 *
 * The world and collision plugins must be attached before using this function,
 * and the rtpick library is required (header file rtpick.h).
 *
 * \param world  Pointer to the world containing atomics.
 * \param line  Pointer to a RwLine value specifying the world line.
 *
 * \return Returns pointer to an atomic if successful or NULL if there is
 * an error or if there are no atomics on the pick line.
 *
 * \see RwCameraPickAtomicOnPixel
 * \see RwCameraCalcPixelRay
 * \see RpAtomicForAllIntersections
 * \see RpWorldForAllAtomicIntersections
 * \see RpWorldForAllWorldSectorIntersections
 * \see RpCollisionPluginAttach
 * \see RpWorldPluginAttach
 */
RpAtomic           *
RpWorldPickAtomicOnLine(RpWorld * world, const RwLine * line)
{
    RpAtomic           *result = NULL;
    RpIntersection      intersection;
    rtPickData          pickData;

    RWAPIFUNCTION(RWSTRING("RpWorldPickAtomicOnLine"));
    RWASSERT(world);
    RWASSERT(line);

    intersection.type = rpINTERSECTLINE;
    intersection.t.line = *line;

    pickData.closestAtomic = NULL;
    pickData.closestDistance = (RwReal) (0.0);
    pickData.sector = NULL;

    if (RpWorldForAllAtomicIntersections(world, &intersection,
                                         TestAtomicForCloseness,
                                         &pickData))
    {
        /* No error occured, return what was hit (could be nothing) */
        result = pickData.closestAtomic;
    }

    RWRETURN(result);
}

/**
 * \ingroup rtpick
 * \ref RwCameraPickAtomicOnPixel is used to determine the nearest
 * atomic, if any, which is selected through a user pick event on the
 * specified camera's image raster at the given pixel location.
 *
 * Note that this function only tests the atomic's bounding sphere to
 * determine if it has been picked. For a more accurate test based on the
 * atomic's geometry use \ref RpAtomicForAllIntersections.
 *
 * The world and collision plugins must be attached before using this function,
 * and the rtpick library is required (header file rtpick.h).
 *
 * \param camera  Pointer to the camera.
 * \param pixel  Pointer to the 2d coordinates of the picked camera pixel.
 *
 * \return Returns a pointer to an atomic if successful or NULL if there is
 * an error, or if there are no atomics on the pixel.
 *
 * \see RwCameraCalcPixelRay
 * \see RpWorldPickAtomicOnLine
 * \see RpWorldForAllAtomicIntersections
 * \see RpWorldForAllWorldSectorIntersections
 * \see RpCollisionPluginAttach
 * \see RpWorldPluginAttach
 */
RpAtomic           *
RwCameraPickAtomicOnPixel(const RwCamera * camera, const RwV2d * pixel)
{
    RpAtomic           *atomicRet = NULL;
    RpWorld            *world;

    RWAPIFUNCTION(RWSTRING("RwCameraPickAtomicOnPixel"));
    RWASSERT(camera);
    RWASSERTISTYPE(camera, rwCAMERA);
    RWASSERT(pixel);

    world = RwCameraGetWorld(camera);
    if (world)
    {
        RwLine              line;

        /* Find the line */
        RwCameraCalcPixelRay(camera, &line, pixel);

        atomicRet = RpWorldPickAtomicOnLine(world, &line);

    }

    RWRETURN(atomicRet);
}
