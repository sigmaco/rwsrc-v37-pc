/*
 * World utility functions.
 * Helper functions to do extra, non core things with worlds
 *
 * Copyright (c) 1998 Criterion Software Ltd.
 *
*/

/**
 * \ingroup rtworld
 * \page rtworldoverview RtWorld Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rtworld.h
 * \li \b Libraries: rwcore, rpworld, rtworld
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach
 *
 * \subsection worldtoolkitoverview Overview
 * Provides the following functionality in addition to that already contained
 * within the RpWorld plugin:
 *
 * \li A method for obtaining material indices when given an \ref RpMaterial object;
 * \li A method for finding materials by texture name;
 * \li An iterator for all polygons within a given \ref RpWorld object;
 * \li Methods for obtaining the number of polygons, vertices or \ref RpWorldSector
 * in a given \ref RpWorld object;
 * \li Methods for optimising \ref RpAtomic and \ref RpClump objects;
 * \li Method for calculating vertex normals.
 */

/****************************************************************************
 Includes
 */

#include <stdlib.h>
#include <math.h>

#include "rwcore.h"

#include "rpdbgerr.h"

/* This files header */
#include "rtworld.h"

/****************************************************************************
 Local Types
 */

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

/****************************************************************************
 WorldGetNumWorldSectors

 On entry   : Atomic sector
            : Pointer to RwInt32 counter
 On exit    :
 */

static RpWorldSector *
WorldGetNumWorldSectors(RpWorldSector * worldSector, void *pData)
{
    RWFUNCTION(RWSTRING("WorldGetNumWorldSectors"));
    RWASSERT(worldSector);
    RWASSERT(pData);

    if (worldSector && pData)
    {
        /* Increase the counter */
        (*(RwInt32 *) pData)++;

        RWRETURN(worldSector);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN((RpWorldSector *)NULL);
}

/**
 * \ingroup rtworld
 * \ref RtWorldGetNumWorldSectors is used to retrieve the number of
 * world sectors constituting all static geometry in the specified world.
 *
 * The world plugin must be attached before using this function. The include
 * file rtworld.h and the library file rtworld.lib are also required to use
 * this function.
 *
 * \param world  Pointer to the world.
 *
 * \return Returns a RwInt32 value equal to the number of world sectors
 * if successful or -1 if there is an error.
 *
 * \see RtWorldFindMaterialNum
 * \see RtWorldFindMaterialWithTextureName
 * \see RtWorldGetNumPolygons
 * \see RtWorldGetNumVertices
 * \see RpWorldPluginAttach
 *
 */
RwInt32
RtWorldGetNumWorldSectors(RpWorld * world)
{
    RWAPIFUNCTION(RWSTRING("RtWorldGetNumWorldSectors"));
    RWASSERT(world);

    if (world)
    {
        RwInt32             nCount = 0;

        RpWorldForAllWorldSectors(world, WorldGetNumWorldSectors,
                                  &nCount);

        RWRETURN(nCount);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN(-1);
}

/****************************************************************************
 WorldGetNumVertices

 On entry   : Atomic sector
            : Pointer to RwInt32 counter
 On exit    :
 */

static RpWorldSector *
WorldGetNumVertices(RpWorldSector * worldSector, void *pData)
{
    RWFUNCTION(RWSTRING("WorldGetNumVertices"));
    RWASSERT(worldSector);
    RWASSERT(pData);

    if (worldSector && pData)
    {
        /* Increase the counter */
        (*(RwInt32 *) pData) +=
            RpWorldSectorGetNumVertices(worldSector);

        RWRETURN(worldSector);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN((RpWorldSector *)NULL);
}

/**
 * \ingroup rtworld
 * \ref RtWorldGetNumVertices is used to retrieve the number of vertices
 * constituting all static geometry in the specified world. Vertices which
 * are not used by static geometry are not counted.
 *
 * The world plugin must be attached before using this function. The include
 * file rtworld.h and the library file rtworld.lib are also required to use
 * this function.
 *
 * \param world  Pointer to the world.
 *
 * \return Returns a RwInt32 value equal to the number of vertices if
 * successful or -1 if there is an error.
 *
 * \see RtWorldFindMaterialNum
 * \see RtWorldFindMaterialWithTextureName
 * \see RtWorldGetNumPolygons
 * \see RtWorldGetNumWorldSectors
 * \see RpWorldPluginAttach
 *
 */
RwInt32
RtWorldGetNumVertices(RpWorld * world)
{
    RWAPIFUNCTION(RWSTRING("RtWorldGetNumVertices"));
    RWASSERT(world);

    if (world)
    {
        RwInt32             nCount = 0;

        RpWorldForAllWorldSectors(world, WorldGetNumVertices, &nCount);

        RWRETURN(nCount);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN(-1);
}

/****************************************************************************
 WorldGetNumPolygons

 On entry   : Atomic sector
            : Pointer to RwInt32 counter
 On exit    :
 */

static RpWorldSector *
WorldGetNumPolygons(RpWorldSector * worldSector, void *pData)
{
    RWFUNCTION(RWSTRING("WorldGetNumPolygons"));
    RWASSERT(worldSector);
    RWASSERT(pData);

    if (worldSector && pData)
    {
        /* Increase the counter */
        (*(RwInt32 *) pData) +=
            RpWorldSectorGetNumTriangles(worldSector);

        RWRETURN(worldSector);
    }
    RWERROR((E_RW_NULLP));
    RWRETURN((RpWorldSector *)NULL);
}

/**
 * \ingroup rtworld
 * \ref RtWorldGetNumPolygons is used to retrieve the number of polygons
 * constituting all static geometry in the specified world. Polygons which
 * are not used by static geometry are not counted.
 *
 * The world plugin must be attached before using this function. The include
 * file rtworld.h and the library file rtworld.lib are also required to use
 * this function.
 *
 * \param world  Pointer to the world.
 *
 * \return Returns a RwInt32 value equal to the number of polygons if
 * successful or -1 if there is an error.
 *
 * \see RtWorldFindMaterialNum
 * \see RtWorldFindMaterialWithTextureName
 * \see RtWorldGetNumVertices
 * \see RtWorldGetNumWorldSectors
 * \see RpWorldPluginAttach
 *
 */
RwInt32
RtWorldGetNumPolygons(RpWorld * world)
{
    RWAPIFUNCTION(RWSTRING("RtWorldGetNumPolygons"));
    RWASSERT(world);

    if (world)
    {
        RwInt32             nCount = 0;

        RpWorldForAllWorldSectors(world, WorldGetNumPolygons, &nCount);

        RWRETURN(nCount);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN(-1);
}

/**
 * \ingroup rtworld
 * \ref RtWorldFindMaterialWithTextureName is used to find a material
 * in the specified world’s material list (static geometry only) that
 * references a texture with the given name.
 *
 * The world plugin must be attached before using this function. The include
 * file rtworld.h and the library file rtworld.lib are also required to use
 * this function.
 *
 * \param world  Pointer to the world.
 * \param name  Pointer to a string containing the name of the texture.
 *
 * \return Returns a pointer to the material if successful or
 * NULL if there is an error.
 *
 * \see RtWorldFindMaterialNum
 * \see RtWorldGetNumPolygons
 * \see RtWorldGetNumVertices
 * \see RtWorldGetNumWorldSectors
 * \see RpWorldPluginAttach
 *
 */
RpMaterial         *
RtWorldFindMaterialWithTextureName(RpWorld * world, RwChar * name)
{
    RWAPIFUNCTION(RWSTRING("RtWorldFindMaterialWithTextureName"));
    RWASSERT(world);
    RWASSERT(name);

    if (world && name)
    {
        RwInt32             nI;
        RwTexture          *tex;

        if (!RwTexDictionaryGetCurrent())
        {
            RWRETURN((RpMaterial *)NULL);
        }

        tex =
            RwTexDictionaryFindNamedTexture(RwTexDictionaryGetCurrent(),
                                            name);
        if (!tex)
        {
            RWRETURN((RpMaterial *)NULL);
        }

        for (nI = 0; nI < RpWorldGetNumMaterials(world); nI++)
        {
            RpMaterial         *mat;

            mat = RpWorldGetMaterial(world, nI);

            if (tex == RpMaterialGetTexture(mat))
            {
                RWRETURN(mat);
            }
        }
    }
    else
    {
        RWERROR((E_RW_NULLP));
    }

    RWRETURN((RpMaterial *)NULL);
}

/**
 * \ingroup rtworld
 * \ref RtWorldFindMaterialNum is used to retrieve the index of the
 * specified material (used by static geometry) in the given world. The index
 * is into the world’s material list.
 *
 * The world plugin must be attached before using this function. The include
 * file rtworld.h and the library file rtworld.lib are also required to use
 * this function.
 *
 * \param world  Pointer to the world.
 * \param material  Pointer to the material.
 *
 * \return Returns a RwInt32 value equal to the index of the material
 * if successful or -1 if there is an error.
 *
 * \see RtWorldFindMaterialWithTextureName
 * \see RtWorldGetNumPolygons
 * \see RtWorldGetNumVertices
 * \see RtWorldGetNumWorldSectors
 * \see RpWorldPluginAttach
 *
 */
RwInt32
RtWorldFindMaterialNum(RpWorld * world, RpMaterial * material)
{
    RWAPIFUNCTION(RWSTRING("RtWorldFindMaterialNum"));
    RWASSERT(world);
    RWASSERT(material);

    if (world && material)
    {
        RwInt32             nI;

        for (nI = 0; nI < RpWorldGetNumMaterials(world); nI++)
        {
            if (RpWorldGetMaterial(world, nI) == material)
            {
                RWRETURN(nI);
            }
        }
    }
    else
    {
        RWERROR((E_RW_NULLP));
    }

    RWRETURN(-1);
}
