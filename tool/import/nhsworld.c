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

#include "rpdbgerr.h"

#include "nhsstats.h"
#include "nhsutil.h"
#include "nhsworld.h"

/****************************************************************************
 Local Types
 */

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */

#if (defined(_WINDOWS))
#define RWCDECL __cdecl
#endif /* (defined(_WINDOWS)) */

#if (!defined(RWCDECL))
#define RWCDECL                /* No op */
#endif /* (!defined(RWCDECL)) */

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
          Converting the world from build sectors to world sectors
 ***************************************************************************
 ***************************************************************************/

/****************************************************************************
 ImportBuildSectorCompress

 Compresses the build sector into the world sector
 Has to find the appropriate texture map tiling

 On entry   : World sector
            : Build sector
            : World
            : Material List (from noHsWorld)
 On exit    : World sector on success
 */

static RpWorldSector *
ImportBuildSectorCompress(RpWorldSector * worldSector,
                          RtWorldImportBuildSector * buildSector,
                          RpWorld * world, RpMaterialList * noHsMatList,
                          RtWorldImportUserdataCallBacks *
                          UserDataCallBacks)
{
    RwInt32             nI, nJ;

    RWFUNCTION(RWSTRING("ImportBuildSectorCompress"));
    RWASSERT(worldSector);
    RWASSERT(buildSector);

    /* Compress the sector */
    if (buildSector->numPolygons)
    {
        RtWorldImportBuildVertex *boundaries;
        RpTriangle         *destTriangle;
        RwInt32             vertMapSize = (sizeof(RwInt32) *
                                           buildSector->numVertices);
        RwInt32            *vertMap = (RwInt32 *) RwMalloc(vertMapSize,
                               rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_FUNCTION);

        if (vertMap)
        {
            RwInt32             currentIndex = 0;

            /* clear the map */
            for (nJ = 0; nJ < buildSector->numVertices; nJ++)
            {
                vertMap[nJ] = -1;
            }

			/* NOTE:any redundant vertices in the world can 
			   cause vertMap to contain -1 values */

            /* first build a vertex map */
            for (nJ = 0;
                 nJ < rpMaterialListGetNumMaterials(noHsMatList); nJ++)
            {
                boundaries = buildSector->boundaries;
                for (nI = 0; nI < buildSector->numPolygons;
                     nI++, boundaries += 4)
                {
                    if (boundaries[3].pinfo.matIndex == nJ)
                    {
                        RtWorldImportBuildVertexMode *mode;
                        RwInt32             index;

                        /* the indices */
                        mode = &boundaries[0].mode;
                        index = mode->vpVert - buildSector->vertices;

                        if (-1 == vertMap[index])
                        {
                            vertMap[index] = currentIndex++;
                        }

                        mode = &boundaries[1].mode;
                        index = mode->vpVert - buildSector->vertices;

                        if (-1 == vertMap[index])
                        {
                            vertMap[index] = currentIndex++;
                        }

                        mode = &boundaries[2].mode;
                        index = mode->vpVert - buildSector->vertices;
                        if (-1 == vertMap[index])
                        {
                            vertMap[index] = currentIndex++;
                        }
                    }
                }
            }
        }
        else
        {
            RWERROR((E_RW_NOMEM, vertMapSize));
            RWRETURN((RpWorldSector *) NULL);
        }

        /* Copy over the coords */
        {
            RwV3d              *vertices = worldSector->vertices;

            for (nI = 0; nI < buildSector->numVertices; nI++)
            {
                RwV3d               pos;

                pos = buildSector->vertices[nI].OC;

				// vertMap[nI] could be -1
				if (vertMap[nI] >= 0)
				{
					vertices[vertMap[nI]] = pos;
				}
            }
        }

        /* Copy over the normal info if necessary */
        if (rpWORLDNORMALS & RpWorldGetFlags(world))
        {
            RpVertexNormal     *normals = worldSector->normals;
            RwV3d               anormal;
            RwReal              length2;
            RwReal              factor;

            for (nI = 0; nI < buildSector->numVertices; nI++)
            {
                anormal = buildSector->vertices[nI].normal;

                length2 = RwV3dDotProduct(&anormal, &anormal);

                if (0 < length2)
                {
#if (defined(RW_USE_SPF))
                    factor = ((RwReal)1) / ((RwReal)sqrtf(length2));
#else /* (defined(RW_USE_SPF)) */
                    factor = ((RwReal)1) / ((RwReal)sqrt(length2));
#endif /* (defined(RW_USE_SPF)) */
                }
                else
                {
                    factor = (RwReal)0.0;
                }

                RwV3dScale(&anormal, &anormal, factor);

				// vertMap[nI] could be -1
				if (vertMap[nI] >= 0)
				{
					RPVERTEXNORMALFROMRWV3D(normals[vertMap[nI]], anormal);
				}
            }
        }

        /* Copy over the prelighting info if necessary */
        if (rpWORLDPRELIT & RpWorldGetFlags(world))
        {
            RwRGBA             *preLitLum = worldSector->preLitLum;

            for (nI = 0; nI < buildSector->numVertices; nI++)
            {
				// vertMap[nI] could be -1
				if (vertMap[nI] >= 0)
				{
					preLitLum[vertMap[nI]] =
						buildSector->vertices[nI].preLitCol;
				}
            }
        }

        /* Copy over vertex texture coords if necessary */
        for (nI=0; nI<world->numTexCoordSets; nI++)
        {
            RwTexCoords  *texCoords = worldSector->texCoords[nI];

            for (nJ=0; nJ<buildSector->numVertices; nJ++)
            {
				// vertMap[nJ] could be -1
				if (vertMap[nJ] >= 0)
				{
					texCoords[vertMap[nJ]].u =
						buildSector->vertices[nJ].texCoords[nI].u;
					texCoords[vertMap[nJ]].v =
						buildSector->vertices[nJ].texCoords[nI].v;
				}
            }
        }

        /* Call vertex userdata callback on each vertex */
        if (UserDataCallBacks->sectorSetVertexUserdata)
        {
            for (nI = 0; nI < buildSector->numVertices; nI++)
            {
				// vertMap[nI] could be -1
				if (vertMap[nI] >= 0)
				{
					UserDataCallBacks->
						sectorSetVertexUserdata(&buildSector->vertices[nI].
												pUserdata, worldSector,
												vertMap[nI]);
				}
            }
        }

        /* Establish a base window address */
        worldSector->matListWindowBase = 0;

        /* compress the triangles */
        destTriangle = worldSector->triangles;

        boundaries = buildSector->boundaries;
        for (nI = 0; nI < buildSector->numPolygons;
             nI++, boundaries += 4)
        {
            RtWorldImportSectorSetPolygonUserdataCallBack
                sectorSetPolygonUserdata;
            RtWorldImportBuildVertexMode *mode;
            RwInt32             index;

            /* the indices */
            mode = &boundaries[0].mode;
            index = mode->vpVert - buildSector->vertices;
            destTriangle->vertIndex[0] = (RwUInt16) vertMap[index];

            mode = &boundaries[1].mode;
            index = mode->vpVert - buildSector->vertices;
            destTriangle->vertIndex[1] = (RwUInt16) vertMap[index];

            mode = &boundaries[2].mode;
            index = mode->vpVert - buildSector->vertices;
            destTriangle->vertIndex[2] = (RwUInt16) vertMap[index];

            /* we can safely copy over the material index since we
               no longer remap into material windows but take a copy
               of the source material list instead */
            destTriangle->matIndex = boundaries[3].pinfo.matIndex;

            RWASSERT(UserDataCallBacks);

            sectorSetPolygonUserdata =
                UserDataCallBacks->sectorSetPolygonUserdata;

            /* Call polygon userdata callback on each vertex */
            if (sectorSetPolygonUserdata)
            {
                void              **pUserdata =
                    (void **) &boundaries[3].pinfo.pUserdata;

                sectorSetPolygonUserdata(pUserdata, worldSector, nI);

            }

            /* Onto the next triangle */
            destTriangle++;
        }

        RwFree(vertMap);
        if ((RwUInt32) (destTriangle - worldSector->triangles) !=
            (RwUInt32) worldSector->numTriangles)
        {
            RWERROR((E_RW_SECTORINVNOPOLYGONS));
            RWRETURN((RpWorldSector *) NULL);
        }
    }
    else
    {
        worldSector->numVertices = 0;
        worldSector->numTriangles = 0;
    }

    RWRETURN(worldSector);
}

/****************************************************************************
 _rwImportWorldSectorCreate

 Compresses a Build sector into an world sector

 On entry       : RtWorldImportBuildSector
 On exit        : RpWorldSector
 */

static RpWorldSector *
ImportWorldSectorCreate(RtWorldImportBuildSector * buildSector,
                        RpWorld * world, RpMaterialList * noHsMatList,
                        RtWorldImportUserdataCallBacks *
                        UserDataCallBacks)
{
    RpWorldSector      *worldSector;
    RwInt32             numTriangles;
    RtWorldImportBuildSector *triangulatedBuildSector;
    RwBBox tightBBox;

    RWFUNCTION(RWSTRING("ImportWorldSectorCreate"));
    RWASSERT(buildSector);

    numTriangles = _rtImportBuildSectorFindNumTriangles(buildSector);

    worldSector = (RpWorldSector *) RwMalloc(sectorTKList.sizeOfStruct,
                         rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
    if (!worldSector)
    {
        RWERROR((E_RW_NOMEM, sectorTKList.sizeOfStruct));
        RWRETURN((RpWorldSector *) NULL);
    }


    tightBBox = buildSector->boundingBox;
    /* generate a tight bbox */
    if (buildSector->numVertices)
    {
        RwInt32             i;
        RwBBox              bbox;

        RwBBoxInitialize(&bbox, &buildSector->vertices[0].OC);
        for (i = 1; i < buildSector->numVertices; i++)
        {
            RwBBoxAddPointMacro(&bbox,
                                &buildSector->vertices[i].OC);
        }
        tightBBox = bbox;
    }
    else
    {
        /* Empty sector, so make tight bbox zero volume
         * and for tidyness, centre of the loose bbox
         */
        tightBBox.inf.x = (buildSector->boundingBox.inf.x + buildSector->boundingBox.sup.x) / 2.0f;
        tightBBox.inf.y = (buildSector->boundingBox.inf.y + buildSector->boundingBox.sup.y) / 2.0f;
        tightBBox.inf.z = (buildSector->boundingBox.inf.z + buildSector->boundingBox.sup.z) / 2.0f;
        tightBBox.sup.x = tightBBox.inf.x;
        tightBBox.sup.y = tightBBox.inf.y;
        tightBBox.sup.z = tightBBox.inf.z;
    }


    if (!_rtImportWorldSectorInitialize
        (worldSector, world, &tightBBox, numTriangles,
         buildSector->numVertices))
    {
        RwFree(worldSector);
        RWRETURN((RpWorldSector *) NULL);
    }

    /* Initialize memory allocated to toolkits */
    rwPluginRegistryInitObject(&sectorTKList, worldSector);

    if (numTriangles)
    {
        /* Set up the triangle array */
        triangulatedBuildSector =
            _rtImportBuildSectorTriangulize(buildSector, numTriangles,
                                            UserDataCallBacks);

        /* Copy over the information */
        ImportBuildSectorCompress(worldSector, triangulatedBuildSector,
                                  world, noHsMatList,
                                  UserDataCallBacks);

        /* Done with the triangulized one */
        _rtImportBuildSectorDestroy(triangulatedBuildSector,
                                    UserDataCallBacks);
    }

    /* Sanity check */
    _rtImportWorldSectorCheck(worldSector);

    /* Done */
    RWRETURN(worldSector);
}

/****************************************************************************
 _rtImportWorldSectorCompressTree

 On entry       : Sector tree
 On exit        : Pointer to root node
 */

RpSector           *
_rtImportWorldSectorCompressTree(RpSector * rootSector, RpWorld * world,
                                 RpMaterialList * noHsMatList,
                                 RtWorldImportUserdataCallBacks *
                                 UserDataCallBacks)
{
    RWFUNCTION(RWSTRING("_rtImportWorldSectorCompressTree"));
    RWASSERT(rootSector);

    /* Handle the cases */
    switch (rootSector->type)
    {
        case rwSECTORATOMIC:
            {
                /* This sector is already a world sector -> do nothing */
                break;
            }
        case rwSECTORBUILD:
            {
                RpSector           *spNew;
                RtWorldImportBuildSector *buildSector =
                    (RtWorldImportBuildSector *) rootSector;

                /* This is a build sector -> make it a world sector */
                spNew =
                    (RpSector *) ImportWorldSectorCreate(buildSector,
                                                         world,
                                                         noHsMatList,
                                                         UserDataCallBacks);

                /* Record that more polygons have been compressed */
                _rtWorldImportNumPolysInCompressedLeaves +=
                    buildSector->numPolygons;
                _rtImportWorldSendProgressMessage
                    (rtWORLDIMPORTPROGRESSBSPCOMPRESSUPDATE,
                     ((RwReal) _rtWorldImportNumPolysInCompressedLeaves
                      / (RwReal) _rtWorldImportTotalPolysInWorld) *
                     100);

                /* Destroy the old */
                _rtImportBuildSectorDestroy(buildSector,
                                            UserDataCallBacks);

                /* Return the new sector */
                RWRETURN(spNew);
            }
        default:
            {
                RpPlaneSector      *planeSector =
                    (RpPlaneSector *) rootSector;

                /* There is some junk after the structure,
                 * but we can live with that ;> */
                planeSector->leftSubTree =
                    _rtImportWorldSectorCompressTree(planeSector->
                                                     leftSubTree, world,
                                                     noHsMatList,
                                                     UserDataCallBacks);
                planeSector->rightSubTree =
                    _rtImportWorldSectorCompressTree(planeSector->
                                                     rightSubTree,
                                                     world, noHsMatList,
                                                     UserDataCallBacks);

                /* Return the sector */
                break;
            }
    }

    RWRETURN(rootSector);
}
