/**
 * \defgroup rtvcat RtVCAT
 * \ingroup meshes
 *
 * Vertex Cache Aware Tristripping Toolkit for RenderWare.
 */

/**
 * \ingroup rtvcat
 * \page rtvcatoverview RtVCAT Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rtvcat.h
 * \li \b Libraries: rwcore, rpworld.h, rtvcat
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach
 *
 * \subsection vcatoverview Overview
 * The RtVCAT (Vertex Cache Aware Tristripper) toolkit provides a
 * custom tristripping callback for
 * generating tristrips that respect a vertex cache of a given size.
 * By generating tristrips that re-use vertices which are already
 * in the post transform and light cache as much as possible, large
 * rendering performance increases can be gained
 * (as much as 200% in some cases!) over models stripped with
 * other RenderWare tristrippers which were designed for platforms
 * such as PlayStation 2 which do not have a post transform and light cache.
 * However, this comes at the cost of additional degenerate triangles
 * so RtVCAT is not the best tristripper to choose for those platforms.
 *
 * For a detailed explanation see the XDK whitepaper "Xbox Vertex Performance.doc".
 *
 * Currently, RtVCAT is hardcoded to optimize for the Xbox and
 * GeForce 3 post transform and light cache size.
 *
 * RtVCAT is activated in the RenderWare exporters by selecting the
 * "Cache Aware (Xbox and GeForce 3)" tristrip method option.
 *
 * RtVCAT is implemented with xbStrip, Microsoft's
 * tristripping code available on the Xbox XDK.
 *
 * Copyright Criterion Software Limited
 */

/**********************************************************************
 *
 * File :     rtvcat.cpp
 *
 * Abstract : Generates vertex cache aware tristrips
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

/****************************************************************************
 Includes
 */

#include "rwcore.h"
#include "rpdbgerr.h"

#include "rpworld.h"

#ifndef SKY2_DRVMODEL_H

#include "xbstrip.h"

#include "rtvcat.h"

/****************************************************************************
 Defines
 */

#if (defined(_WINDOWS))
#define  __RWCDECL   __cdecl
#endif /* (defined(_WINDOWS)) */

#if (!defined(__RWCDECL))
#define  __RWCDECL             /* No op */
#endif /* (!defined(__RWCDECL)) */

#define RasterHasAlpha(_type)                           \
            ( ((_type) == rwRASTERFORMAT1555) ||        \
              ((_type) == rwRASTERFORMAT4444) ||        \
              ((_type) == rwRASTERFORMAT8888) )


/****************************************************************************
 Local Types
 */

typedef struct rpMesh16 rpMesh16;
struct rpMesh16
{
    RwUInt16           *indices;
    RwUInt32            numIndices;
    RpMaterial         *material;
};

typedef struct PrimitiveGroup PrimitiveGroup;
struct PrimitiveGroup
{
    RwUInt32  dwNumIndices;    // Number of output indices
    RwUInt16  *pStripIndices;   // Output indices
};


/****************************************************************************
 Local (static) Functions
 */

static int __RWCDECL
SortPolygons(const void *pA, const void *pB)
{
    const RwUInt32 transBIT = 16;
    const RwUInt32  rastBIT = 8;
    const RwUInt32  pipeBIT = 4;
    const RwUInt32  texBIT = 2;
    const RwUInt32  matBIT = 1;

    const RpBuildMeshTriangle * const * mtpA =
        (const RpBuildMeshTriangle * const *) pA;
    const RpBuildMeshTriangle * const * mtpB =
        (const RpBuildMeshTriangle * const *) pB;

    RpMaterial *materialA = (*mtpA)->material;
    RpMaterial *materialB = (*mtpB)->material;

    RwRaster   *rasterA   = (RwRaster *)NULL;
    RwRaster   *rasterB   = (RwRaster *)NULL;
    RwTexture  *textureA  = (RwTexture *)NULL;
    RwTexture  *textureB  = (RwTexture *)NULL;

    /* IMO use UInts for bitfields! Sign bits are nothing but trouble... */
    RwUInt32     orderA = 0;
    RwUInt32     orderB = 0;

    RWFUNCTION(RWSTRING("SortPolygons"));

    /* Easy case first */
    if (materialA == materialB)
    {
        RWRETURN(0);
    }

    /* We sort on:
     *   transparency > raster > pipeline > texture > material
     *
     * Transparency is required for correct alpha render ordering.
     * Raster upload is the greatest cost.
     * Pipeline swap might be a significant cost - vector code upload, CPU-side code cache miss.
     * Texture state changes might also hurt even with the same raster. (?)
     * Sorting things in memory order (i.e on RpMaterial pointer) is probably generally a good thing, pff...
     */

    if (materialA)
    {
        /* Place transparent materials after non transparent ones */
        if (materialA->texture)
        {
            textureA = materialA->texture;
            rasterA = RwTextureGetRaster(textureA);

            if (RasterHasAlpha
                (RwRasterGetFormat(rasterA) & (RwInt32)
                 rwRASTERFORMATPIXELFORMATMASK))
            {
                orderA |= transBIT;
            }
        }

        if (materialA->color.alpha != 0xff)
        {
            orderA |= transBIT;
        }
    }

    if (materialB)
    {
        /* Place transparent materials after non transparent ones */
        if (materialB->texture)
        {
            textureB = materialB->texture;
            rasterB = RwTextureGetRaster(textureB);

            if (RasterHasAlpha
                (RwRasterGetFormat(rasterB) & (RwInt32)
                 rwRASTERFORMATPIXELFORMATMASK))
            {
                orderB |= transBIT;
            }
        }

        if (materialB->color.alpha != 0xff)
        {
            orderB |= transBIT;
        }
    }

    orderA |= ((*mtpA)->rasterIndex > (*mtpB)->rasterIndex) ? rastBIT : 0;
    orderB |= ((*mtpA)->rasterIndex < (*mtpB)->rasterIndex) ? rastBIT : 0;

    orderA |= ((*mtpA)->pipelineIndex > (*mtpB)->pipelineIndex) ? pipeBIT : 0;
    orderB |= ((*mtpA)->pipelineIndex < (*mtpB)->pipelineIndex) ? pipeBIT : 0;

    orderA |= ((*mtpA)->textureIndex > (*mtpB)->textureIndex) ? texBIT : 0;
    orderB |= ((*mtpA)->textureIndex < (*mtpB)->textureIndex) ? texBIT : 0;

    orderA |= ((*mtpA)->matIndex > (*mtpB)->matIndex) ? matBIT : 0;
    orderB |= ((*mtpA)->matIndex < (*mtpB)->matIndex) ? matBIT : 0;

    RWRETURN(orderA - orderB);
}


/**
 * \ingroup rtvcat
 * \ref RpBuildMeshGenerateCacheAwareTriStrip generates a vertex cache aware
 * triangle strip.
 *
 * \param buildMesh pointer to the mesh which the triangle strip will be
 * generated from.
 * \param data  pointer to user-supplied data to pass to the callback
 * function.
 *
 * \return a pointer to the constructed mesh header.
 *
 * \see RpMeshGetTriStripMethod
 * \see RpMeshSetTriStripMethod
 * \see RpBuildMeshGenerateDefaultTriStrip
 * \see RpBuildMeshGeneratePreprocessTriStrip
 * \see RpBuildMeshGenerateExhaustiveTriStrip
 * \see RpBuildMeshGenerateTrivialTriStrip
 * \see RpBuildMeshGenerateDefaultIgnoreWindingTriStrip
 * \see RpBuildMeshGeneratePreprocessIgnoreWindingTriStrip
 * \see RpBuildMeshGenerateExhaustiveIgnoreWindingTriStrip
 *
 */
extern "C"
RpMeshHeader *
RpBuildMeshGenerateCacheAwareTriStrip(RpBuildMesh *buildMesh,
                                      void *data)
{
    RpMeshHeader         *result;
    RpBuildMeshTriangle **triPointers;
    RwUInt32              i;
    RwUInt16              numMeshes;
    RwUInt32              meshSize;
    RxVertexIndex        *stripMeshInds;

    RWAPIFUNCTION(RWSTRING("RpBuildMeshGenerateCacheAwareTriStrip"));
    RWASSERT(buildMesh);

    triPointers = (RpBuildMeshTriangle **)RwMalloc(buildMesh->numTriangles * sizeof(RpBuildMeshTriangle *),
                                                   rwID_VCATPLUGIN | rwMEMHINTDUR_FUNCTION);
    if (!triPointers)
    {
        RWRETURN((RpMeshHeader *)NULL);
    }

    /* Fill in pointers so that we can sort */
    for (i = 0; i < buildMesh->numTriangles; i++)
    {
        triPointers[i] = &(buildMesh->meshTriangles[i]);
    }

    /* Now sort them */
    qsort(triPointers, buildMesh->numTriangles, sizeof(RpBuildMeshTriangle *), SortPolygons);

    /* Figure out how many meshes there are */
    numMeshes = 1;
    if (buildMesh->numTriangles >= 2)
    {
        RpMaterial *lastMat = triPointers[0]->material;

        for (i = 1; i < buildMesh->numTriangles; i++)
        {
            if (triPointers[i]->material != lastMat)
            {
                /* We found another material */
                lastMat = triPointers[i]->material;
                numMeshes++;
            }
        }
    }


    /* Allocate an mesh array */
    RwUInt16 *inMeshIndices = (RwUInt16 *)RwMalloc(buildMesh->numTriangles * 3 * sizeof(RwUInt16),
                                                   rwID_VCATPLUGIN | rwMEMHINTDUR_FUNCTION);
    rpMesh16 *inMeshes = (rpMesh16 *)RwMalloc(numMeshes * sizeof(rpMesh16),
        rwID_VCATPLUGIN | rwMEMHINTDUR_FUNCTION);

    rpMesh16 *tempMesh = inMeshes;
    RwInt32 index = 0;

    RpMaterial *lastMat   = triPointers[0]->material;

    tempMesh->indices    = inMeshIndices;
    tempMesh->numIndices = 0;
    tempMesh->material   = triPointers[0]->material;

    for (i = 0; i < buildMesh->numTriangles; i++)
    {
        if (triPointers[i]->material != lastMat)
        {
            tempMesh++;

            /* We found another material so store off mesh information */
            tempMesh->indices    = inMeshIndices + index;
            tempMesh->numIndices = 0;
            tempMesh->material   = triPointers[i]->material;

            lastMat = triPointers[i]->material;
        }

        inMeshIndices[index++] = triPointers[i]->vertIndex[0];
        inMeshIndices[index++] = triPointers[i]->vertIndex[1];
        inMeshIndices[index++] = triPointers[i]->vertIndex[2];

        tempMesh->numIndices += 3;
    }

    /* Don't need these any more */
    RwFree(triPointers);
    triPointers = (RpBuildMeshTriangle **)NULL;

    PrimitiveGroup *primGroups = (PrimitiveGroup *)RwMalloc(numMeshes * sizeof(PrimitiveGroup),
                                                         rwID_VCATPLUGIN | rwMEMHINTDUR_FUNCTION);
    if (data == 0)
    {
        for (i = 0; i < numMeshes; i++)
        {
            Stripify(inMeshes[i].numIndices / 3,
                    inMeshes[i].indices,
                    &(primGroups[i].dwNumIndices),
                    &(primGroups[i].pStripIndices),
                    OPTIMIZE_FOR_CACHE | OUTPUT_TRISTRIP);
        }
    }
    else
    {
        for (i = 0; i < numMeshes; i++)
        {
            Stripify(inMeshes[i].numIndices / 3,
                    inMeshes[i].indices,
                    &(primGroups[i].dwNumIndices),
                    &(primGroups[i].pStripIndices),
                    OPTIMIZE_FOR_CACHE | OUTPUT_TRILIST);
        }
    }

    /* Work out how big the mesh header needs to be */
    RwInt32 numOutIndices = 0;
    for (i = 0; i < numMeshes; i++)
    {
        numOutIndices += primGroups[i].dwNumIndices;
    }

    /* Allocate the meshes */
    meshSize = sizeof(RpMeshHeader) +
               sizeof(RpMesh) * numMeshes +
               sizeof(RxVertexIndex) * numOutIndices;

    /* Should really use PMESHGLOBAL(nextSerialNum) */
    static RwInt16 nextSerialNum = 0;

    result = _rpMeshHeaderCreate(meshSize);
    if (data == 0)
    {
        result->flags = rpMESHHEADERTRISTRIP;
    }
    else
    {
        result->flags = 0;
    }

    result->numMeshes = numMeshes;
    result->serialNum = nextSerialNum;
    result->firstMeshOffset = 0;
    result->totalIndicesInMesh = numOutIndices;
    nextSerialNum++;

    /* Fill in destination meshes */
    RpMesh *outMesh;
    outMesh = (RpMesh *)(result + 1);
    stripMeshInds = (RxVertexIndex *) (outMesh + numMeshes);
    for (i = 0; i < numMeshes; i++)
    {
        RwUInt32 j;

        /* Add in the next mesh */
        outMesh->indices    = stripMeshInds;
        outMesh->numIndices = primGroups[i].dwNumIndices;
        outMesh->material   = inMeshes[i].material;

        /* And the indices */
        for (j = 0; j < primGroups[i].dwNumIndices; j++)
        {
            *stripMeshInds++ = primGroups[i].pStripIndices[j];
        }

        /* Skip to next */
        outMesh++;
    }

    /* Blow away our NvTriStrip resources */
    for (i = 0; i < numMeshes; i++)
    {
        delete[] primGroups[i].pStripIndices;
    }

    RwFree(primGroups);
    primGroups = NULL;

    RwFree(inMeshIndices);
    inMeshIndices = NULL;

    RwFree(inMeshes);
    inMeshes = NULL;

    RWRETURN(result);
}

#endif
