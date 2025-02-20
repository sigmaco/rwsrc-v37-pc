/*
 * Converting no hs worlds to real binary worlds (with bsps).
 * No HS worlds are used in the generation process of worlds
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

#include "nhsstats.h"
#include "nhsutil.h"

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
 Functions
 */

/***************************************************************************
 ***************************************************************************
                   Utilities to help with converting a world
 ***************************************************************************
 ***************************************************************************/

 /****************************************************************************
 _rtImportBuildSectorCreate

 On entry   :
 On exit    : New build sector
 */

RtWorldImportBuildSector *
_rtImportBuildSectorCreate(void)
{
    RtWorldImportBuildSector *buildSector;

    RWFUNCTION(RWSTRING("_rtImportBuildSectorCreate"));

    buildSector =
        (RtWorldImportBuildSector *)
        RwMalloc(sizeof(RtWorldImportBuildSector),
            rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
    if (!buildSector)
    {
        RWERROR((E_RW_NOMEM, sizeof(RtWorldImportBuildSector)));
        RWRETURN((RtWorldImportBuildSector *) NULL);
    }

    /* Set up the new sector */
    buildSector->type = rwSECTORBUILD;
    buildSector->vertices = (RtWorldImportVertex *) NULL;
    buildSector->numVertices = 0;
    buildSector->boundaries = (RtWorldImportBuildVertex *) NULL;
    buildSector->numBoundaries = 0;
    buildSector->numPolygons = 0;
    buildSector->overlap = 0.0f;

    RWRETURN(buildSector);
}

/****************************************************************************
 _rtImportBuildSectorDestroy

  On entry  : BuildSector
  On exit   : TRUE on success
  */

RwBool
_rtImportBuildSectorDestroy(RtWorldImportBuildSector * buildSector,
                            RtWorldImportUserdataCallBacks *
                            UserDataCallBacks)
{
    RtWorldImportBuildVertex *boundaries;
    RtWorldImportVertex *vertices;

    RWFUNCTION(RWSTRING("_rtImportBuildSectorDestroy"));
    RWASSERT(buildSector);

    vertices = buildSector->vertices;

    if (vertices)
    {
        const               RtWorldImportDestroyVertexUserdataCallBack
            destroyVertexUserdata =
            UserDataCallBacks->destroyVertexUserdata;

        if (destroyVertexUserdata)
        {
            const RwInt32       numVertices = buildSector->numVertices;
            RwInt32             i;

            for (i = 0; i < numVertices; i++)
            {
                if (vertices[i].pUserdata)
                {
                    destroyVertexUserdata(&vertices[i].pUserdata);
                }
            }
        }
        RwFree(vertices);
    }
    boundaries = buildSector->boundaries;

    if (boundaries)
    {
        const               RtWorldImportDestroyPolygonUserdataCallBack
            destroyPolygonUserdata =
            UserDataCallBacks->destroyPolygonUserdata;

        if (destroyPolygonUserdata)
        {
            const RwInt32       numBoundaries =
                buildSector->numBoundaries;
            RwInt32             i;

            for (i = 0; i < numBoundaries; i++)
            {
                RtWorldImportBuildVertexMode *const mode =
                    &boundaries[i].mode;

                if (!mode->vpVert && boundaries[i].pinfo.pUserdata)
                {
                    destroyPolygonUserdata(&boundaries[i].pinfo.
                                           pUserdata);
                }
            }
        }
        RwFree(boundaries);
    }

    RwFree(buildSector);

    RWRETURN(TRUE);
}

RwReal
_rtWorldImportBuildSectorAreaOfWall(RwBBox *boundingBox, RwInt32 type)
{
    /* area of selected face */
    RwV3d diag;

    RWFUNCTION(RWSTRING("_rtWorldImportBuildSectorAreaOfWall"));

    diag.x = (boundingBox->sup.x - boundingBox->inf.x);
    diag.y = (boundingBox->sup.y - boundingBox->inf.y);
    diag.z = (boundingBox->sup.z - boundingBox->inf.z);

    if (type == 8)
    {
        RWRETURN(diag.x * diag.y);
    }
    else if (type == 4)
    {
        RWRETURN(diag.x * diag.z);
    }
    else
    {
        RWRETURN(diag.y * diag.z);
    }
}


RwReal
_rtImportBuildSectorFindBBoxArea(RwBBox *boundingBox)
{
    /* area of smallest face */
    RwV3d diag;

    RWFUNCTION(RWSTRING("_rtImportBuildSectorFindBBoxArea"));

    diag.x = (boundingBox->sup.x - boundingBox->inf.x);
    diag.y = (boundingBox->sup.y - boundingBox->inf.y);
    diag.z = (boundingBox->sup.z - boundingBox->inf.z);

    if ((diag.x < diag.z) && (diag.y < diag.z))
    {
        RWRETURN(diag.x * diag.y);
    }
    else if ((diag.x < diag.y) && (diag.z < diag.y))
    {
        RWRETURN(diag.x * diag.z);
    }
    else
    {
        RWRETURN(diag.y * diag.z);
    }
}

RwReal
_rtImportBuildSectorFindBBoxDiagonal(RwBBox *boundingBox)
{
    RwV3d diag;
    RwReal size;

    RWFUNCTION(RWSTRING("_rtImportBuildSectorFindBBoxDiagonal"));

    diag.x = (boundingBox->sup.x - boundingBox->inf.x);
    diag.y = (boundingBox->sup.y - boundingBox->inf.y);
    diag.z = (boundingBox->sup.z - boundingBox->inf.z);
    size = RwV3dLength(&diag);

    RWRETURN(size);
}

RwReal
_rtImportBuildSectorFindBBoxVolume(RwBBox *boundingBox)
{
    RwV3d diag;

    RWFUNCTION(RWSTRING("_rtImportBuildSectorFindBBoxVolume"));

    diag.x = (boundingBox->sup.x - boundingBox->inf.x);
    diag.y = (boundingBox->sup.y - boundingBox->inf.y);
    diag.z = (boundingBox->sup.z - boundingBox->inf.z);
    RWRETURN(diag.x * diag.y * diag.z);
}

/****************************************************************************
 _rtImportBuildSectorFindBBox

 Only considers the extent dictated by the vertices used by polygons
 NB This is important because the CollSector creation alters the boundaries
 pointer.
 On entry   : BuildSector
            : BBox holding the extent (OUT)
 On exit    : TRUE on success
 */

RwBool
_rtImportBuildSectorFindBBox(RtWorldImportBuildSector * buildSector,
                             RwBBox * bbpOut)
{
    RWFUNCTION(RWSTRING("_rtImportBuildSectorFindBBox"));
    RWASSERT(buildSector);
    RWASSERT(bbpOut);

    if (buildSector->vertices)
    {
        RwInt32             nI;
        const RwInt32       numBoundaries = buildSector->numBoundaries;
        RtWorldImportBuildVertex *boundaries = buildSector->boundaries;
        const RtWorldImportBuildVertexMode *mode;

        mode = &boundaries->mode;
        RwBBoxInitialize(bbpOut, &mode->vpVert->OC);

        for (nI = 0; nI < numBoundaries; nI++, boundaries++)
        {
            mode = &boundaries->mode;
            if (mode->vpVert)
            {
                /* IF YOU CRASH HERE, THERE'S A PROBLEM WITH VERTICES AND POLYGONS
                 * PROBS SOME REDUNDANT VERTS */
                RwBBoxAddPointMacro(bbpOut, &mode->vpVert->OC);
            }
        }

    }
    else
    {
        /* just copy it */
        *bbpOut = buildSector->boundingBox;
    }
    RWRETURN(TRUE);
}

/****************************************************************************
 _rtImportBuildSectorFindNumTriangles

 On entry   : sector
 On exit    : number of triangles
 */

RwInt32
_rtImportBuildSectorFindNumTriangles(RtWorldImportBuildSector *
                                     buildSector)
{
    RtWorldImportBuildVertex *boundaries;
    RwInt32             sides, nJ, tricount;

    RWFUNCTION(RWSTRING("_rtImportBuildSectorFindNumTriangles"));
    RWASSERT(buildSector);

    tricount = sides = 0;
    boundaries = buildSector->boundaries;
    for (nJ = 0; nJ < buildSector->numBoundaries; nJ++, boundaries++)
    {
        RtWorldImportBuildVertexMode *const mode = &boundaries->mode;

        if (!mode->vpVert)
        {
            tricount += sides - 2;
            sides = 0;
        }
        else
        {
            sides++;
        }
    }

    RWRETURN(tricount);
}

/****************************************************************************
 _rtImportBuildSectorTriangulize

 On entry   : sector
 On exit    : sector of just triangles
 */

RtWorldImportBuildSector *
_rtImportBuildSectorTriangulize(RtWorldImportBuildSector * buildSector,
                                RwInt32 numTriangles,
                                RtWorldImportUserdataCallBacks *
                                UserDataCallBacks)
{
    RtWorldImportBuildSector *triangulatedBuildSector;
    RtWorldImportBuildVertex *vpFirst, *vpCurr, *boundaries;
    RwInt32             nI;

    RWFUNCTION(RWSTRING("_rtImportBuildSectorTriangulize"));
    RWASSERT(buildSector);

    triangulatedBuildSector = _rtImportBuildSectorCreate();
    if (!triangulatedBuildSector)
    {
        RWRETURN((RtWorldImportBuildSector *) NULL);
    }

    /* copy over the vertices */
    triangulatedBuildSector->numVertices = buildSector->numVertices;
    triangulatedBuildSector->vertices =
        (RtWorldImportVertex *) RwMalloc(sizeof(RtWorldImportVertex) *
                                         triangulatedBuildSector->
                                         numVertices,
                                         rwID_NOHSWORLDPLUGIN |
                                         rwMEMHINTDUR_EVENT);
    if (!triangulatedBuildSector->vertices)
    {
        RWERROR((E_RW_NOMEM,
                 sizeof(RtWorldImportVertex) *
                 buildSector->numVertices));
        _rtImportBuildSectorDestroy(triangulatedBuildSector,
                                    UserDataCallBacks);
        RWRETURN((RtWorldImportBuildSector *) NULL);
    }
    memcpy(triangulatedBuildSector->vertices, buildSector->vertices,
           sizeof(RtWorldImportVertex) *
           triangulatedBuildSector->numVertices);
    if (UserDataCallBacks->cloneVertexUserdata)
    {
        for (nI = 0; nI < buildSector->numVertices; nI++)
        {
            triangulatedBuildSector->vertices[nI].pUserdata = NULL;
            UserDataCallBacks->
                cloneVertexUserdata(&triangulatedBuildSector->
                                    vertices[nI].pUserdata,
                                    &buildSector->vertices[nI].
                                    pUserdata);
        }
    }

    /* Create the polygon refs */
    triangulatedBuildSector->numPolygons = numTriangles;
    triangulatedBuildSector->numBoundaries = numTriangles * (3 + 1);
    triangulatedBuildSector->boundaries =
        (RtWorldImportBuildVertex *)
        RwMalloc(sizeof(RtWorldImportBuildVertex) *
                 triangulatedBuildSector->numBoundaries,
                 rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
    if (!buildSector->boundaries)
    {
        RWERROR((E_RW_NOMEM,
                 sizeof(RtWorldImportBuildVertex) *
                 triangulatedBuildSector->numBoundaries));
        _rtImportBuildSectorDestroy(triangulatedBuildSector,
                                    UserDataCallBacks);
        RWRETURN((RtWorldImportBuildSector *) NULL);
    }

    vpFirst = buildSector->boundaries;
    vpCurr = vpFirst + 1;
    boundaries = triangulatedBuildSector->boundaries;
    if (UserDataCallBacks->splitPolygonUserdata)
    {
        for (nI = 1; nI < buildSector->numBoundaries; nI++, vpCurr++)
        {
            RtWorldImportBuildVertexMode *const mode = &vpCurr->mode;

            if (!mode->vpVert)
            {
                RtWorldImportBuildVertex *vpLast = vpFirst + 1;
                RwInt32             nJ;

                vpLast = vpCurr - 1;

                /* emit triangles in a strip-friendly way (not a fan!) */
                nJ = 0;
                while (vpLast - vpFirst > 1)
                {
                    if (nJ & 1)
                    {
                        *boundaries++ = *vpLast;
                        *boundaries++ = *vpFirst++;
                        *boundaries++ = *vpFirst;
                        /* terminator */
                        *boundaries = *vpCurr;
                        boundaries->pinfo.pUserdata = NULL;
                        if (vpCurr->pinfo.pUserdata)
                        {
                            UserDataCallBacks->
                                splitPolygonUserdata(&boundaries->pinfo.
                                                     pUserdata,
                                                     &vpCurr->pinfo.
                                                     pUserdata);

                        }
                        boundaries++;
                    }
                    else
                    {
                        *boundaries++ = *vpFirst;
                        *boundaries++ = *(vpLast - 1);
                        *boundaries++ = *vpLast--;
                        /* terminator */
                        *boundaries = *vpCurr;
                        boundaries->pinfo.pUserdata = NULL;
                        if (vpCurr->pinfo.pUserdata)
                        {
                            UserDataCallBacks->
                                splitPolygonUserdata(&boundaries->pinfo.
                                                     pUserdata,
                                                     &vpCurr->pinfo.
                                                     pUserdata);

                        }
                        boundaries++;
                    }

                    nJ++;
                }

                /* beginning of next polygon */
                vpFirst = vpCurr + 1;
            }
        }
    }
    else
    {
        for (nI = 1; nI < buildSector->numBoundaries; nI++, vpCurr++)
        {
            RtWorldImportBuildVertexMode *const mode = &vpCurr->mode;

            if (!mode->vpVert)
            {
                RtWorldImportBuildVertex *vpLast = vpFirst + 1;
                RwInt32             nJ;

                vpLast = vpCurr - 1;

                /* emit triangles in a strip-friendly way (not a fan!) */
                nJ = 0;
                while (vpLast - vpFirst > 1)
                {
                    if (nJ & 1)
                    {
                        *boundaries++ = *vpLast;
                        *boundaries++ = *vpFirst++;
                        *boundaries++ = *vpFirst;
                        /* terminator */
                        *boundaries++ = *vpCurr;
                    }
                    else
                    {
                        *boundaries++ = *vpFirst;
                        *boundaries++ = *(vpLast - 1);
                        *boundaries++ = *vpLast--;
                        /* terminator */
                        *boundaries++ = *vpCurr;
                    }

                    nJ++;
                }

                /* beginning of next polygon */
                vpFirst = vpCurr + 1;
            }
        }
    }

    /* lastly redirect the boundary vertex pointers */
    boundaries = triangulatedBuildSector->boundaries;
    for (nI = 0; nI < triangulatedBuildSector->numBoundaries;
         nI++, boundaries++)
    {
        RtWorldImportBuildVertexMode *const mode = &boundaries->mode;

        if (mode->vpVert)
        {
            RwInt32             index;

            index = mode->vpVert - buildSector->vertices;
            mode->vpVert = triangulatedBuildSector->vertices + index;
        }
    }

    triangulatedBuildSector->boundingBox = buildSector->boundingBox;
    RWRETURN(triangulatedBuildSector);
}

/****************************************************************************
 _rtImportBuildPlaneSectorCreate

 On entry   : Plane
            : Value
 On exit    :
 */

RtWorldImportBuildPlaneSector *
_rtImportBuildPlaneSectorCreate(RwInt32 type, RwReal value)
{
    RtWorldImportBuildPlaneSector *buildPlaneSector;

    RWFUNCTION(RWSTRING("_rtImportBuildPlaneSectorCreate"));

    buildPlaneSector =
        (RtWorldImportBuildPlaneSector *)
        RwMalloc(sizeof(RtWorldImportBuildPlaneSector),
            rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
    if (!buildPlaneSector)
    {
        RWERROR((E_RW_NOMEM, sizeof(RtWorldImportBuildPlaneSector)));
        RWRETURN((RtWorldImportBuildPlaneSector *) NULL);
    }

    /* Plane */
    buildPlaneSector->planeSector.type = type;
    buildPlaneSector->planeSector.value = value;
    buildPlaneSector->planeSector.leftSubTree = (RpSector *) NULL;
    buildPlaneSector->planeSector.rightSubTree = (RpSector *) NULL;
    buildPlaneSector->planeSector.leftValue = 0.0f;
    buildPlaneSector->planeSector.rightValue = 0.0f;

    RWRETURN(buildPlaneSector);
}

/****************************************************************************
 rwPlaneSectorDestroy

 On entry   : Plane sector
 On exit    : TRUE on success
 */

RwBool
_rtImportPlaneSectorDestroy(RtWorldImportBuildPlaneSector *
                            buildPlaneSector)
{
    RWFUNCTION(RWSTRING("_rtImportPlaneSectorDestroy"));
    RWASSERT(buildPlaneSector);

    RwFree(buildPlaneSector);

    RWRETURN(TRUE);
}

/****************************************************************************
 _rtImportPlaneSectorDestroyTree

  On entry  : Sector
  On exit   :
  */

void
_rtImportPlaneSectorDestroyTree(RtWorldImportBuildPlaneSector *
                                sector,
                                RtWorldImportUserdataCallBacks *
                                UserDataCallBacks)
{
    RWFUNCTION(RWSTRING("_rtImportPlaneSectorDestroyTree"));

    if (sector)
    {
        switch (sector->planeSector.type)
        {
            case rwSECTORBUILD:
                {
                    RtWorldImportBuildSector *const buildSector =
                        (RtWorldImportBuildSector *) sector;

                    _rtImportBuildSectorDestroy(buildSector,
                                                UserDataCallBacks);
                    break;
                }
            case rwSECTORATOMIC:
                {
                    break;
                }
            case rwXPLANE:
            case rwYPLANE:
            case rwZPLANE:
                {
                    RtWorldImportBuildPlaneSector *leftSubTree =
                        (RtWorldImportBuildPlaneSector *)
                        sector->planeSector.leftSubTree;

                    RtWorldImportBuildPlaneSector *rightSubTree =
                        (RtWorldImportBuildPlaneSector *)
                        sector->planeSector.rightSubTree;

                    _rtImportPlaneSectorDestroyTree(leftSubTree,
                                                    UserDataCallBacks);
                    _rtImportPlaneSectorDestroyTree(rightSubTree,
                                                    UserDataCallBacks);

                    _rtImportPlaneSectorDestroy(sector);
                    break;
                }
        }
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rtImportWorldSectorInitialize

 Allocates everything you need for a world sector

 On entry   : World sector
            : World
            : Bounding box of the sector
            : Amount of polygons
            : Amount of vertices
 On exit    : world sector pointer on success
 */

RpWorldSector      *
_rtImportWorldSectorInitialize(RpWorldSector * worldSector,
                               RpWorld * world, RwBBox * bbpBox,
                               RwInt32 numTriangles, RwInt32 numVertices)
{
    RwInt32             i;

    RWFUNCTION(RWSTRING("_rtImportWorldSectorInitialize"));
    RWASSERT(worldSector);
    RWASSERT(world);
    RWASSERT(bbpBox);

    /* Mark it as a world sector */
    worldSector->type = rwSECTORATOMIC;

    /* Copy over its size. The 'loose' bounding boxes should be
     * generated from the BSP planes once the import process has
     * finished.
     */
    worldSector->tightBoundingBox = (*bbpBox);

    /* Not instanced !!! */
    worldSector->repEntry = (RwResEntry *) NULL;
    worldSector->mesh = (RpMeshHeader *) NULL;

    /* Set so it contains nothing */
    rwLinkListInitialize(&worldSector->collAtomicsInWorldSector);
    rwLinkListInitialize(&worldSector->lightsInWorldSector);

    /* Set up the pointers */
    worldSector->numVertices = 0;
    worldSector->numTriangles = 0;
    worldSector->vertices = (RwV3d *) NULL;
    worldSector->triangles = (RpTriangle *) NULL;
    worldSector->normals = (RpVertexNormal *) NULL;
    worldSector->matListWindowBase = 0;

    for (i = 0; i < rwMAXTEXTURECOORDS; i++)
    {
        worldSector->texCoords[i] = (RwTexCoords *) NULL;
    }

    worldSector->preLitLum = (RwRGBA *) NULL;
    /* Use the default world sector object pipeline */
    worldSector->pipeline = (RxPipeline *) NULL;

    /* No point in doing anything unless we have both polygons and vertices.
     * Silly to have polygons without vertices.
     * Pointless to have vertices without polygons.
     */
    if (numTriangles && numVertices)
    {
        /* Initialize the memory blocks */
        worldSector->numVertices = (RwInt16) numVertices;
        worldSector->numTriangles = (RwInt16) numTriangles;

        /* Vertices themselves - always have these */
        worldSector->vertices =
            (RwV3d *) RwMalloc(sizeof(RwV3d) * numVertices,
                               rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
        if (!worldSector->vertices)
        {
            RWERROR((E_RW_NOMEM, (sizeof(RwV3d) * numVertices)));
            RWRETURN((RpWorldSector *) NULL);
        }

        /* Normals - dependant on whether we have these */
        if (rpWORLDNORMALS & RpWorldGetFlags(world))
        {
            worldSector->normals =
                (RpVertexNormal *) RwMalloc(sizeof(RpVertexNormal) *
                                            numVertices,
                                            rwID_NOHSWORLDPLUGIN |
                                            rwMEMHINTDUR_EVENT);
            if (!worldSector->normals)
            {
                RwFree(worldSector->vertices);
                worldSector->vertices = (RwV3d *) NULL;
                RWERROR((E_RW_NOMEM,
                         (sizeof(RpVertexNormal) * numVertices)));
                RWRETURN((RpWorldSector *) NULL);
            }
        }

        /* Pre lighting information  - dependant on whether we have these */
        if (rpWORLDPRELIT & RpWorldGetFlags(world))
        {
            worldSector->preLitLum =
                (RwRGBA *) RwMalloc(sizeof(RwRGBA) * numVertices,
                                   rwID_NOHSWORLDPLUGIN |
                                   rwMEMHINTDUR_EVENT);
            if (!worldSector->preLitLum)
            {
                if (worldSector->normals)
                {
                    RwFree(worldSector->normals);
                    worldSector->normals = (RpVertexNormal *) NULL;
                }
                RwFree(worldSector->vertices);
                worldSector->vertices = (RwV3d *) NULL;
                RWERROR((E_RW_NOMEM, (sizeof(RwRGBA) * numVertices)));
                RWRETURN((RpWorldSector *) NULL);
            }
        }

        /* Vertex tex coords - dependant on whether we have these */
        if (world->numTexCoordSets > 0)
        {
            /* Vertex tex coords */
            RwInt32             vertexTexCoordSize;
            RwInt32             i;

            vertexTexCoordSize = sizeof(RwTexCoords) * numVertices;

            for (i=0; i<world->numTexCoordSets; i++)
            {
                worldSector->texCoords[i] =
                    (RwTexCoords *) RwMalloc(vertexTexCoordSize,
                                             rwID_NOHSWORLDPLUGIN |
                                             rwMEMHINTDUR_EVENT);
                if (!worldSector->texCoords[i])
                {
                    RwInt32     j = i;

                    while (j--)
                    {
                        RwFree(worldSector->texCoords[j]);
                        worldSector->texCoords[j] = (RwTexCoords *) NULL;
                    }
                    if (worldSector->preLitLum)
                    {
                        RwFree(worldSector->preLitLum);
                        worldSector->preLitLum = (RwRGBA *) NULL;
                    }
                    if (worldSector->normals)
                    {
                        RwFree(worldSector->normals);
                        worldSector->normals = (RpVertexNormal *) NULL;
                    }
                    RwFree(worldSector->vertices);
                    worldSector->vertices = (RwV3d *) NULL;
                    RWERROR((E_RW_NOMEM, vertexTexCoordSize));
                    RWRETURN((RpWorldSector *) NULL);
                }
            }
        }

        /* Polygons themselves */
        worldSector->triangles =
            (RpTriangle *) RwMalloc(sizeof(RpTriangle) * numTriangles,
                                   rwID_NOHSWORLDPLUGIN |
                                   rwMEMHINTDUR_EVENT);
        if (!worldSector->triangles)
        {
            RwInt32     i;

            for (i=0; i<rwMAXTEXTURECOORDS; i++)
            {
                if (worldSector->texCoords[i])
                {
                    RwFree(worldSector->texCoords[i]);
                    worldSector->texCoords[i] = (RwTexCoords *) NULL;
                }
            }
            if (worldSector->preLitLum)
            {
                RwFree(worldSector->preLitLum);
                worldSector->preLitLum = (RwRGBA *) NULL;
            }
            if (worldSector->normals)
            {
                RwFree(worldSector->normals);
                worldSector->normals = (RpVertexNormal *) NULL;
            }
            RwFree(worldSector->vertices);
            worldSector->vertices = (RwV3d *) NULL;
            RWERROR((E_RW_NOMEM, (sizeof(RpTriangle) * numTriangles)));
            RWRETURN((RpWorldSector *) NULL);
        }
    }

    /* Done */
    RWRETURN(worldSector);
}

/****************************************************************************
 _rtImportWorldSectorCheck

  On entry      : World sector
  On exit       : TRUE if ok
  */

RwBool
_rtImportWorldSectorCheck(RpWorldSector * worldSector)
{
    RwInt32             nI, nJ;
    RpTriangle         *triangles;

    RWFUNCTION(RWSTRING("_rtImportWorldSectorCheck"));
    RWASSERT(worldSector);

    /* How about the polygons proper */
    triangles = worldSector->triangles;
    for (nI = 0; nI < worldSector->numTriangles; nI++)
    {
        for (nJ = 0; nJ < 3; nJ++)
        {
            if ((triangles[nI]).vertIndex[nJ] >= worldSector->numVertices)
            {
                RWERROR((E_RW_RANGE));
                RWRETURN(FALSE);
            }
        }
    }

    RWRETURN(TRUE);
}

/****************************************************************************
 _rtImportBuildSectorCheck

  On entry      : Build sector
  On exit       : TRUE if ok
  */

void
_rtImportBuildSectorCheck(RtWorldImportBuildSector * buildSector)
{
    RwInt32             nI, sides, pcount;
    RtWorldImportBuildVertex *boundaries;

    RWFUNCTION(RWSTRING("_rtImportBuildSectorCheck"));
    RWASSERT(buildSector);

    /* validate boundary vertex pointers */
    boundaries = buildSector->boundaries;
    sides = 0;
    pcount = 0;
    for (nI = 0; nI < buildSector->numBoundaries; nI++, boundaries++)
    {

        RtWorldImportBuildVertexMode *const mode = &boundaries->mode;

        if (mode->vpVert)
        {
            const RwInt32       index = (mode->vpVert -
                                         buildSector->vertices);

            if ((index < 0) || (index >= buildSector->numVertices))
            {
                /* AppTerminalPrintf("index\n"); */
            }

            sides++;
        }
        else
        {
            if ((sides < 3) || (sides > 128))
            {
                /* AppTerminalPrintf("sides\n"); */
            }

            sides = 0;
            pcount++;
        }
    }

    if (pcount != buildSector->numPolygons)
    {
        /* AppTerminalPrintf("numPolygons\n"); */
    }

    RWRETURNVOID();
}
