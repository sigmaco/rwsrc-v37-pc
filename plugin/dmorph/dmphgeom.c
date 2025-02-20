/*
 *  dmphgeom.c - delta morph geometry data and morph targets
 */

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rpplugin.h"
#include <rpdbgerr.h>
#include <rwcore.h>
#include <rpworld.h>

#include "dmorph.h"
#include "dmphgeom.h"

/*===========================================================================*
 *--- Types -----------------------------------------------------------------*
 *===========================================================================*/

/*--- DMorphTargetBinary -------------------------------------------------*/
typedef struct DMorphTargetBinary DMorphTargetBinary;
struct DMorphTargetBinary
{
    RwUInt32           flags;           /* Type RpGeometryFlag */
    RwUInt32           lockFlags;       /* Type RpGeometryLockMode */
    RwUInt32           numCodeElements;
    RwUInt32           numDataElements;
};

/*===========================================================================*
 *--- Global variables ------------------------------------------------------*
 *===========================================================================*/

RwInt32 rpDMorphGeometryDataOffset = 0;

/*===========================================================================*
 *--- Macros             ----------------------------------------------------*
 *===========================================================================*/

#define RGBASubMacro(_out, _a, _b)                  \
MACRO_START                                         \
{                                                   \
    (_out)->red   = (_a)->red   - (_b)->red;        \
    (_out)->green = (_a)->green - (_b)->green;      \
    (_out)->blue  = (_a)->blue  - (_b)->blue;       \
    (_out)->alpha = (_a)->alpha - (_b)->alpha;      \
}                                                   \
MACRO_STOP

#define TexCoordsSubMacro(_out, _a, _b)             \
MACRO_START                                         \
{                                                   \
    (_out)->u = (_a)->u - (_b)->u;                  \
    (_out)->v = (_a)->v - (_b)->v;                  \
}                                                   \
MACRO_STOP

/*
 *  Delta Test macros to determine whether a particular
 *  vertex delta is significant enough to be included in
 *  a delta morph target.
 */

#define POSITION_FTOL   1e-5f
#define NORMAL_TOL      1e-4f
#define TEXCOORD_TOL    1e-4f

#define PositionDeltaTestMacro(_del, _rad)                  \
    (RwV3dDotProduct(_del, _del)                            \
        > (_rad)*(_rad)*(POSITION_FTOL*POSITION_FTOL))

#define NormalDeltaTestMacro(_del)                          \
    (RwV3dDotProduct(_del, _del) > NORMAL_TOL*NORMAL_TOL)

#define RGBADeltaTestMacro(_del)                            \
   (((_del)->red != 0) || ((_del)->green != 0)              \
    || ((_del)->blue  != 0) || ((_del)->alpha != 0))

#define TexCoordDeltaTestMacro(_del)                        \
   ( ((_del)->u * (_del)->u + (_del)->v * (_del)->v)        \
     > TEXCOORD_TOL * TEXCOORD_TOL )

#define PACKET_IS_UNKNOWN   -1
#define PACKET_IS_SKIP       0
#define PACKET_IS_DELTA      1

/*===========================================================================*
 *--- Internal functions ----------------------------------------------------*
 *===========================================================================*/

/*--- DMorphGeometryCtor ---------------------------------------------------
 */
static void *
DMorphGeometryCtor( void *object,
                    RwInt32 offset __RWUNUSED__,
                    RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("DMorphGeometryCtor"));
    RWASSERT(object);

    *RPDMORPHGEOMETRYGETDATA(object) = (rpDMorphGeometryData *)NULL;

    RWRETURN(object);
}

/*--- DMorphGeometryDtor ---------------------------------------------------
 */
static void *
DMorphGeometryDtor( void *object,
                    RwInt32 offset __RWUNUSED__,
                    RwInt32 size   __RWUNUSED__ )
{
    rpDMorphGeometryData  *geometryData;

    RWFUNCTION(RWSTRING("DMorphGeometryDtor"));
    RWASSERT(object);

    geometryData = *RPDMORPHGEOMETRYGETDATA(object);
    if( geometryData )
    {
#if defined(PLATFORM_GEOM_DTOR)
        _rpDMorphGeometryDtor(geometryData);
#endif /* defined(PLATFORM_GEOM_DTOR) */

        if( geometryData->numDMorphTargets > 0 )
        {
            RpDMorphGeometryDestroyDMorphTargets((RpGeometry *)object);
        }
    }

    *RPDMORPHGEOMETRYGETDATA(object) = (rpDMorphGeometryData *)NULL;

    RWRETURN(object);
}

/*--- DMorphGeometryCopy ---------------------------------------------------
 */
static void *
DMorphGeometryCopy( void *dstObject,
                    const void *srcObject __RWUNUSEDRELEASE__,
                    RwInt32 offset __RWUNUSED__,
                    RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("DMorphGeometryCopy"));
    RWASSERT(dstObject);
    RWASSERT(srcObject);

    /* A geometry can not be copied so leave this empty and there ain't
     * nothing to do anyway */

    RWRETURN(dstObject);
}

/*--- DMorphGeometryStreamRead ---------------------------------------------
 */
static RwStream *
DMorphGeometryStreamRead( RwStream *stream,
                          RwInt32 binaryLength   __RWUNUSED__,
                          void *object,
                          RwInt32 offsetInObject __RWUNUSED__,
                          RwInt32 sizeInObject   __RWUNUSED__ )
{
    rpDMorphGeometryData   *geometryData;
    RpGeometry             *geometry;
    RwUInt32                numTargets;
    RwUInt32                i;

    RWFUNCTION(RWSTRING("DMorphGeometryStreamRead"));
    RWASSERT(stream);
    RWASSERT(object);

    /* Grab the geometry and extension data... */
    geometry = (RpGeometry *)object;

    /* Read the number of delta morph targets */
    if (!RwStreamReadInt32(stream, (RwInt32 *)&numTargets, sizeof(RwUInt32)))
        RWRETURN((RwStream *)NULL);

    /* Create the extension */
    RpDMorphGeometryCreateDMorphTargets(geometry, numTargets);
    geometryData = *RPDMORPHGEOMETRYGETDATA(geometry);
    RWASSERT(geometryData);

    /* Read in the morph targets */
    for( i = 0; i < geometryData->numDMorphTargets; i++ )
    {
        RpDMorphTarget     *target;
        DMorphTargetBinary  binary;
        RwUInt32            length;
        RwUInt32            size;
        RwUInt8            *memory;

        target = &(geometryData->dMorphTargets[i]);

        /* Read the name */
        if (!RwStreamReadInt32(stream, (RwInt32 *)&length, sizeof(RwInt32)))
            RWRETURN((RwStream *)NULL);

        if (length > 0)
        {
            target->name = (RwChar *)RwMalloc(length * sizeof(RwChar),
                              rwID_DMORPHPLUGIN | rwMEMHINTDUR_EVENT);

            if (!target->name)
            {
                RWERROR((E_RW_NOMEM, length * sizeof(RwChar)));
                RWRETURN((RwStream *)NULL);
            }

            if (!RwStreamRead( stream, (void *)target->name, length))
                RWRETURN((RwStream *)NULL);
        }
        else
        {
            target->name = (RwChar *)NULL;
        }

        /* Binary header */
        if (!RwStreamReadInt32(stream, (RwInt32 *)&binary, sizeof(binary)))
            RWRETURN((RwStream *)NULL);

        #if defined(D3D9_DRVMODEL_H)
        {
            RwUInt32 usageFlags;

            usageFlags = RpD3D9GeometryGetUsageFlags(geometry);

            usageFlags |= binary.lockFlags;

            RpD3D9GeometrySetUsageFlags(geometry, usageFlags);
        }
        #endif

        target->flags =               binary.flags;
        target->lockFlags =           binary.lockFlags;
        target->rle.numCodeElements = binary.numCodeElements;
        target->rle.numDataElements = binary.numDataElements;

        /* RLE code elements */
        size = target->rle.numCodeElements * sizeof(RwUInt8);
        target->rle.code = (RwUInt8 *)RwMalloc(size,
            rwID_DMORPHPLUGIN | rwMEMHINTDUR_EVENT);
        if (!target->rle.code)
        {
            RWERROR((E_RW_NOMEM, size));
            RWRETURN((RwStream *)NULL);
        }

        if (!RwStreamRead(stream, (void *)target->rle.code, size))
            RWRETURN((RwStream *)NULL);

        if(target->rle.numDataElements == 0)
        {
            target->rle.data = (void *)NULL;
        }
        else
        {
            /* RLE data elements */
            size = ((target->flags & rpGEOMETRYPOSITIONS) ? sizeof(RwV3d) : 0)
                 + ((target->flags & rpGEOMETRYNORMALS)   ? sizeof(RwV3d) : 0)
                 + ((target->flags & rpGEOMETRYPRELIT)    ? sizeof(RwRGBA) : 0)
                 + ((target->flags & rpGEOMETRYTEXTURED)  ? sizeof(RwTexCoords) : 0);
            size *= target->rle.numDataElements;

            target->rle.data = (void *)RwMalloc(size,
                rwID_DMORPHPLUGIN | rwMEMHINTDUR_EVENT);
            if (!target->rle.data)
            {
                RWERROR((E_RW_NOMEM, size));
                RWRETURN((RwStream *)NULL);
            }

            memory = (RwUInt8 *)target->rle.data;

            if (target->flags & rpGEOMETRYPOSITIONS)
            {
                size = target->rle.numDataElements * sizeof(RwV3d);
                if (!RwStreamReadReal(stream, (RwReal *)memory, size))
                    RWRETURN((RwStream *)NULL);
                target->vertices = (RwV3d *)memory;
                memory += size;
            }

            if (target->flags & rpGEOMETRYNORMALS)
            {
                size = target->rle.numDataElements * sizeof(RwV3d);
                if (!RwStreamReadReal(stream, (RwReal *)memory, size))
                    RWRETURN((RwStream *)NULL);
                target->normals = (RwV3d *)memory;
                memory += size;
            }

            if (target->flags & rpGEOMETRYPRELIT)
            {
                size = target->rle.numDataElements * sizeof(RwRGBA);
                if (!RwStreamRead(stream, memory, size))
                    RWRETURN((RwStream *)NULL);
                target->preLightColors = (RwRGBA *)memory;
                memory += size;
            }

            if (target->flags & rpGEOMETRYTEXTURED)
            {
                size = target->rle.numDataElements * sizeof(RwTexCoords);
                if (!RwStreamReadReal(stream, (RwReal *)memory, size))
                    RWRETURN((RwStream *)NULL);
                target->texCoords = (RwTexCoords *)memory;
            }
        }

        /* Bounding Sphere */
        if (!RwStreamReadReal(stream,
                (RwReal *)&target->boundingSphere, sizeof(RwSphere)))
        {
            RWRETURN((RwStream *)NULL);
        }

        /* Extra data */
        target->geometry = geometry;
    }

    /* OK, we're all done */
    RWRETURN(stream);
}

/*--- DMorphGeometryStreamWrite ---------------------------------------------
 */
static RwStream *
DMorphGeometryStreamWrite( RwStream *stream,
                           RwInt32 binaryLength   __RWUNUSED__,
                           const void *object,
                           RwInt32 offsetInObject __RWUNUSED__,
                           RwInt32 sizeInObject   __RWUNUSED__ )
{
    const rpDMorphGeometryData *geometryData;
    const RpGeometry *geometry;
    RwUInt32   i;

    RWFUNCTION(RWSTRING("DMorphGeometryStreamWrite"));
    RWASSERT(stream);
    RWASSERT(object);

    /* Grab the geometry and extension data... */
    geometry = (const RpGeometry *)object;
    geometryData = *RPDMORPHGEOMETRYGETCONSTDATA(geometry);
    RWASSERT(geometryData);

    /* Write number of morph targets */
    if (!RwStreamWriteInt32(stream,
                (const RwInt32 *)&(geometryData->numDMorphTargets),
                sizeof(RwInt32)))
    {
        RWRETURN((RwStream *)NULL);
    }

    /* Lets store the dmorphtarget data. */
    for( i = 0; i < geometryData->numDMorphTargets; i++ )
    {
        const RpDMorphTarget   *target;
        DMorphTargetBinary      binary;
        RwInt32                 nameLength;
        RwUInt32                size;
        const RwReal           *spherePtr;

        target = &(geometryData->dMorphTargets[i]);

        /* Target name */
        nameLength = (target->name) ? (rwstrlen(target->name) + 1) : 0;
        if (!RwStreamWriteInt32(stream, &nameLength, sizeof(RwInt32)))
            RWRETURN((RwStream *)NULL);

        if (target->name && !RwStreamWrite(stream, target->name, nameLength))
            RWRETURN((RwStream *)NULL);

        /* Binary header */
        binary.flags            = target->flags;
        binary.lockFlags        = target->lockFlags;
        binary.numCodeElements  = target->rle.numCodeElements;
        binary.numDataElements  = target->rle.numDataElements;

        if (!RwStreamWriteInt32(stream, (RwInt32 *)(&binary), sizeof(binary)))
            RWRETURN((RwStream *)NULL);

        /* RLE code elements */
        size = target->rle.numCodeElements * sizeof(RwUInt8);
        if (!RwStreamWrite(stream, (void *)(target->rle.code), size))
            RWRETURN((RwStream *)NULL);

        /* RLE data elements */
        if(target->rle.numDataElements != 0)
        {
            if (target->flags & rpGEOMETRYPOSITIONS)
            {
                size = target->rle.numDataElements * sizeof(RwV3d);
                if (!RwStreamWriteReal(stream, (RwReal *)target->vertices, size))
                    RWRETURN((RwStream *)NULL);
            }

            if (target->flags & rpGEOMETRYNORMALS)
            {
                size = target->rle.numDataElements * sizeof(RwV3d);
                if (!RwStreamWriteReal(stream, (RwReal *)target->normals, size))
                    RWRETURN((RwStream *)NULL);
            }

            if (target->flags & rpGEOMETRYPRELIT)
            {
                size = target->rle.numDataElements * sizeof(RwRGBA);
                if (!RwStreamWrite(stream, target->preLightColors, size))
                    RWRETURN((RwStream *)NULL);
            }

            if (target->flags & rpGEOMETRYTEXTURED)
            {
                size = target->rle.numDataElements * sizeof(RwTexCoords);
                if (!RwStreamWriteReal(stream, (RwReal *)target->texCoords, size))
                    RWRETURN((RwStream *)NULL);
            }
        }

        spherePtr = (const RwReal *)(&target->boundingSphere);

        /* Bounding Sphere */
        if (!RwStreamWriteReal( stream, spherePtr, sizeof(RwSphere)))
        {
            RWRETURN((RwStream *)NULL);
        }
    }

    /* OK, we're all done */
    RWRETURN(stream);
}

/*--- DMorphGeometryStreamSize ---------------------------------------------
 */
static RwInt32
DMorphGeometryStreamSize( const void *object,
                          RwInt32 offsetInObject __RWUNUSED__,
                          RwInt32 sizeInObject   __RWUNUSED__ )
{
    const rpDMorphGeometryData *geometryData;

    const RpGeometry *geometry;

    RwUInt32 size;
    RwUInt32 i;

    RWFUNCTION(RWSTRING("DMorphGeometryStreamSize"));

    /* Set the default size. */
    size = 0;

    /* Grab the geometry and extension data... */
    geometry = (const RpGeometry *)object;
    geometryData = *RPDMORPHGEOMETRYGETCONSTDATA(geometry);

    /* Does the geometry have any dmorphing data? */
    if( NULL != geometryData )
    {
        /* GeometryData: size */
        size += sizeof(RwUInt32);

        /* DMorphTarget: size */
        for( i = 0; i < geometryData->numDMorphTargets; i++ )
        {
            const RpDMorphTarget *dMorphTarget;

            /*RpGeometryFlag flags;*/
            RwUInt32 flags;

            dMorphTarget = &(geometryData->dMorphTargets[i]);
            flags = dMorphTarget->flags;

            /* DMorphTarget name length: size */
            size += sizeof(RwUInt32);

            if( NULL != dMorphTarget->name )
            {
                /* DMorphTarget name: size */
                size += rwstrlen(dMorphTarget->name) + 1;
            }

            /* DMorphTarget binary: size */
            size += sizeof(DMorphTargetBinary);

            /* DMorphTraget RLE codeElements: size */
            size += dMorphTarget->rle.numCodeElements;

            /* DMorphTarget RLE dataElemeents: size */
            size += (  ((flags & rpGEOMETRYPOSITIONS) ? sizeof(RwV3d) : 0)
                     + ((flags & rpGEOMETRYNORMALS)   ? sizeof(RwV3d) : 0)
                     + ((flags & rpGEOMETRYPRELIT)    ? sizeof(RwRGBA) : 0)
                     + ((flags & rpGEOMETRYTEXTURED)  ? sizeof(RwTexCoords) : 0))
                  * dMorphTarget->rle.numDataElements;

            /* DMorphTarget BoundingSphere: size */
            size += sizeof(RwSphere);
        }

#if defined(STREAM_STREAM)
        _rwDMorphGeometryNativeStreamSize(geometry);
#endif /* defined(STREAM_STREAM) */
    }

    /* OK, we're all done */
    RWRETURN(size);
}

/*--- DMorphTargetCalcBoundingSphere ---------------------------------------
 * Copied from old morph system - dodgy in my opinion. (JA)
 */
static RwSphere *
DMorphTargetCalcBoundingSphere(RwSphere    *boundingSphere,
                               const RwV3d *vertices,
                               RwUInt32     numVerts)
{
    RwUInt32    i;
    RwSphere    sphere;
    RwReal      sphere_radius = (RwReal)(0);
    RwBBox      boundBox;

    RWFUNCTION(RWSTRING("DMorphTargetCalcBoundingSphere"));
    RWASSERT(vertices);
    RWASSERT(boundingSphere);

    /* Find the median (sort of) by doing a bounding box,
     * then using the center of the box.
     */
    RwBBoxCalculate(&boundBox, vertices, numVerts);
    RwV3dAdd(&sphere.center, &boundBox.inf, &boundBox.sup);
    RwV3dScale(&sphere.center, &sphere.center, (RwReal) (0.5));

    /* Find the radius (we do this in square space). */
    for (i = 0; i < numVerts; i++)
    {
        RwReal              nDist;
        RwV3d               vTmp;

        RwV3dSub(&vTmp, &vertices[i], &sphere.center);
        nDist = RwV3dDotProduct(&vTmp, &vTmp);

        if (nDist > sphere_radius)
        {
            sphere_radius = nDist;
        }
    }

    /* Now do the root */
    if (sphere_radius > (RwReal) (0))
    {
        rwSqrt(&(sphere_radius), sphere_radius);
    }

    /* Add 0.1% for numerical inaccuracy */
    sphere.radius = sphere_radius * ((RwReal)(1.001));

    /* Save off result */
    (*boundingSphere) = sphere;

    /* All done */
    RWRETURN(boundingSphere);
}

/*--- DMorphRLEGenerate ----------------------------------------------------
 */
static RwBool
DMorphRLEGenerate( const RwV3d          *baseVertices,
                   const RwV3d          *vertices,
                   const RwV3d          *baseNormals,
                   const RwV3d          *normals,
                   const RwRGBA         *basePreLights,
                   const RwRGBA         *preLights,
                   const RwTexCoords    *baseTexCoords,
                   const RwTexCoords    *texCoords,
                         RwUInt32        numVerts,
                         RwUInt32        flags,
                         rpDMorphRLE    *rle,
                         RwReal          boundingRadius)
{
    RwUInt32    memSize;
    RwUInt8    *code;

    RwUInt32    numCodes, numDeltas;

    RwUInt32    iVert;

    RWFUNCTION(RWSTRING("DMorphRLEGenerate"));
    RWASSERT(!(flags & rpGEOMETRYPOSITIONS) || (baseVertices && vertices));
    RWASSERT(!(flags & rpGEOMETRYNORMALS)   || (baseNormals && normals));
    RWASSERT(!(flags & rpGEOMETRYPRELIT)    || (basePreLights && preLights));
    RWASSERT(!(flags & rpGEOMETRYTEXTURED)  || (baseTexCoords && texCoords));
    RWASSERT(rle);

    /* Malloc sufficent space for the worst case scenario */
    memSize = sizeof(RwUInt8) * numVerts;
    code = (RwUInt8 *)RwMalloc(memSize,
        rwID_DMORPHPLUGIN | rwMEMHINTDUR_FUNCTION);
    if (!code)
    {
        RWERROR((E_RW_NOMEM, memSize));
        RWRETURN(FALSE);
    }

    /* Search for blocks of delta vertices or non-delta vertices */
    numCodes = 0;
    numDeltas = 0;

    for (iVert=0; iVert < numVerts;)
    {
        RwBool      repeat;
        RwUInt8     packetSize = 0;
        RwBool      vertIsDelta;
        RwInt32     packetIsDelta;

        packetIsDelta = PACKET_IS_UNKNOWN;

        do
        {
            RwV3d       dV3d;
            RwRGBA      dRGBA;
            RwTexCoords dTexCoords;

            /* Find out if this vertex has a significant delta */
            vertIsDelta = FALSE;

            if (flags & rpGEOMETRYPOSITIONS)
            {
                RwV3dSub(&dV3d, &vertices[iVert], &baseVertices[iVert]);
                vertIsDelta = PositionDeltaTestMacro(&dV3d, boundingRadius);
            }

            if ((flags & rpGEOMETRYNORMALS) && !vertIsDelta)
            {
                RwV3dSub(&dV3d, &normals[iVert], &baseNormals[iVert]);
                vertIsDelta = NormalDeltaTestMacro(&dV3d);
            }

            if ((flags & rpGEOMETRYPRELIT) && !vertIsDelta)
            {
                RGBASubMacro(&dRGBA, &preLights[iVert], &basePreLights[iVert]);
                vertIsDelta = RGBADeltaTestMacro(&dRGBA);
            }

            if ((flags & rpGEOMETRYTEXTURED) && !vertIsDelta)
            {
                TexCoordsSubMacro(&dTexCoords, &texCoords[iVert], &baseTexCoords[iVert]);
                vertIsDelta = TexCoordDeltaTestMacro(&dTexCoords);
            }

            if (PACKET_IS_UNKNOWN == packetIsDelta)
            {
                if (vertIsDelta)
                {
                    packetIsDelta = PACKET_IS_DELTA;
                }
                else
                {
                    packetIsDelta = PACKET_IS_SKIP;
                }
            }

            /* Does the vertex match the current packet? */
            if (vertIsDelta == packetIsDelta)
            {
                if (vertIsDelta)
                {
                    /* Increase number of data elements. */
                    numDeltas++;
                }

                iVert++;
                packetSize++;
                repeat = (iVert < numVerts) &&
                    (packetSize < rpDMORPHRLEMAXPACKETSIZE);
            }
            else
            {
                /* We've reached the end of this run. Leave the current
                 * vertex for the next packet */
                repeat = FALSE;
            }
        }
        while (repeat);

        /* Close the current packet, and start the new one */
        code[numCodes++] = (((RwUInt8)packetIsDelta) << 7) | packetSize;
    }

    /* Copy the code to a new buffer of the reduced size, and free original */
    memSize = sizeof(RwUInt8) * numCodes;
    rle->code = (RwUInt8 *)RwMalloc(memSize,
        rwID_DMORPHPLUGIN | rwMEMHINTDUR_EVENT);
    if (!rle->code)
    {
        RwFree(code);
        RWERROR((E_RW_NOMEM, memSize));
        RWRETURN(FALSE);
    }

    rle->numCodeElements = numCodes;
    rle->numDataElements = numDeltas;
    memcpy(rle->code, code, numCodes);
    RwFree(code);

    RWRETURN(TRUE);
}

/*===========================================================================*
 *--- Plugin Internal Functions ---------------------------------------------*
 *===========================================================================*/

RwBool
_rpDMorphGeometryPluginAttach(void)
{
    RwInt32 offset;

    RWFUNCTION(RWSTRING("_rpDMorphGeometryPluginAttach"));

    /* Register the plugin with the geometry. */
    rpDMorphGeometryDataOffset =
        RpGeometryRegisterPlugin( sizeof(rpDMorphGeometryData *),
                                  rwID_DMORPHPLUGIN,
                                  DMorphGeometryCtor,
                                  DMorphGeometryDtor,
                                  DMorphGeometryCopy);

    if( 0 > rpDMorphGeometryDataOffset )
    {
        RWRETURN(FALSE);
    }

    /* Register the plugin with the geometry streams. */
    offset = RpGeometryRegisterPluginStream( rwID_DMORPHPLUGIN,
                                             DMorphGeometryStreamRead,
                                             DMorphGeometryStreamWrite,
                                             DMorphGeometryStreamSize );
    if ( 0 > offset )
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/*--- DMorphGeometry DMorphTarget functions ---------------------------------*/

/**
 * \ingroup rpdmorph
 * \ref RpDMorphGeometryCreateDMorphTargets is used to
 * create space for a number of delta morph targets.
 * Once the base geometry has been setup,
 * \ref RpDMorphGeometryAddDMorphTarget should be used to
 * add delta morph targets to the base.
 * When the delta morph targets are finished with they can
 * be removed with \ref RpDMorphGeometryRemoveDMorphTarget
 * and emptied with
 * \ref RpDMorphGeometryDestroyDMorphTargets.
 * This function cannot be called again on a geometry
 * without first calling
 * \ref RpDMorphGeometryDestroyDMorphTargets to remove
 * the old delta morph targets.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param geometry  Pointer to the base geometry.
 * \param number    The number of delta morph targets to
 *                  create.
 *
 * \return Returns NULL if there is an error, or otherwise
 *         a pointer to the base geometry.
 *
 * \see RpDMorphGeometryDestroyDMorphTargets
 * \see RpDMorphGeometryAddDMorphTarget
 * \see RpDMorphGeometryRemoveDMorphTarget
 */
RpGeometry *
RpDMorphGeometryCreateDMorphTargets( RpGeometry *geometry,
                                     RwUInt32 number )
{
    rpDMorphGeometryData **dataExtension;
    rpDMorphGeometryData *geometryData;
    RpDMorphTarget     *dMorphTargets;

    RwUInt32              size;

    RWAPIFUNCTION(RWSTRING("RpDMorphGeometryCreateDMorphTargets"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(geometry);
    RWASSERT(0 < number);

    /* Get our geometry extension data. */
    dataExtension = RPDMORPHGEOMETRYGETDATA(geometry);
    RWASSERT(NULL == (*dataExtension));

    /* Create rpDMorphGeometryData. */
    size = sizeof(rpDMorphGeometryData);
    geometryData = *dataExtension = (rpDMorphGeometryData *)RwMalloc(size,
                                     rwID_DMORPHPLUGIN | rwMEMHINTDUR_EVENT);
    if(NULL == geometryData)
    {
        RWRETURN((RpGeometry *)NULL);
    }

    /* Setup rpDMorphGeometryData. */
    memset(geometryData, 0, size);
    geometryData->numDMorphTargets = number;

    /* Create DMorphTargets and morph value array */
    size = (sizeof(RwReal) + sizeof(RpDMorphTarget)) * number;
    dMorphTargets = (RpDMorphTarget *)RwMalloc(size,
        rwID_DMORPHPLUGIN | rwMEMHINTDUR_EVENT);
    if( NULL == dMorphTargets )
    {
        RwFree(geometryData);
        geometryData = (rpDMorphGeometryData *)NULL;

        RWRETURN((RpGeometry *)NULL);
    }

    /* Setup the DMorphTargets and morph values */
    memset(dMorphTargets, 0, size);
    geometryData->dMorphTargets = dMorphTargets;
    geometryData->currValues = (RwReal *) (dMorphTargets + number);

    RWRETURN(geometry);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphGeometryDestroyDMorphTargets is used to
 * destroy the base geometry's delta morph targets. This
 * should only be used after a call to
 * \ref RpDMorphGeometryCreateDMorphTargets. Any delta
 * morph targets that have not been removed will be
 * automatically removed.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param geometry  Pointer to the base geometry.
 *
 * \return Returns NULL if there is an error, or otherwise
 *         a pointer to the base geometry.
 *
 * \see RpDMorphGeometryCreateDMorphTargets
 * \see RpDMorphGeometryAddDMorphTarget
 * \see RpDMorphGeometryRemoveDMorphTarget
 */
RpGeometry *
RpDMorphGeometryDestroyDMorphTargets( RpGeometry *geometry )
{
    /* This destroys all the geometry extension data */
    rpDMorphGeometryData **dataExtension;
    rpDMorphGeometryData *geometryData;

    RwUInt32 i;

    RWAPIFUNCTION(RWSTRING("RpDMorphGeometryDestroyDMorphTargets"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(geometry);

    /* Get our geometry extension data. */
    dataExtension = RPDMORPHGEOMETRYGETDATA(geometry);
    RWASSERT(dataExtension);

    geometryData = *dataExtension;
    RWASSERT(geometryData);
    RWASSERT(geometryData->dMorphTargets);

    /* Free up the DMorphTargets data. */
    for( i=0; i < geometryData->numDMorphTargets; i++ )
    {
        RpDMorphGeometryRemoveDMorphTarget( geometry, i );
    }

    /* Free up the DMorphGeometry data (also morph value array) */
    RwFree(geometryData->dMorphTargets);

    RwFree(geometryData);

    geometryData = (rpDMorphGeometryData *)NULL;
    *dataExtension = geometryData;

    RWRETURN(geometry);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphGeometryAddDMorphTarget is used to add
 * a delta morph target to the base geometry at the
 * specified index.
 *
 * The flags determine what geometry elements the delta
 * morph target will contain. The adding process
 * calculates the delta from the given data and the base.
 * The delta is stored in a compressed format as much of
 * the morph target will be identical to the base. Those
 * vertices with a delta of zero (within a small
 * tolerance) are stripped out.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param geometry        Pointer to the base geometry.
 * \param index           Index where the delta should be
 *                        created.
 * \param vertices        Pointer to the delta's vertices
 *                        or NULL if the delta is not
 *                        morphing the positions.
 * \param normals         Pointer to the delta's normals
 *                        or NULL if the delta is not
 *                        morphing the normals.
 * \param preLightColors  Pointer to the delta's prelight
 *                        colors or NULL if the delta is
 *                        not morphing the prelight colors.
 * \param texCoords       Pointer to the delta's texture
 *                        coordinates or NULL if the delta
 *                        is not morphing the texture
 *                        coordinates.
 * \param flags           One of
 *                        \li rpGEOMETRYPOSITIONS
 *                        \li rpGEOMETRYNORMALS
 *                        \li rpGEOMETRYPRELIT or
 *                        \li rpGEOMETRYTEXTURED
 *
 * \return Returns NULL if there is an error, or otherwise
 *         a pointer to the base geometry.
 *
 * \see RpDMorphGeometryRemoveDMorphTarget
 * \see RpDMorphGeometryDestroyDMorphTargets
 * \see RpDMorphGeometryCreateDMorphTargets
 */
RpGeometry *
RpDMorphGeometryAddDMorphTarget( RpGeometry  *geometry,
                                 RwUInt32    index,
                                 RwV3d       *vertices,
                                 RwV3d       *normals,
                                 RwRGBA      *preLightColors,
                                 RwTexCoords *texCoords,
                                 RwUInt32    flags )
{
    RwV3d                  *baseVertices;
    RwV3d                  *baseNormals;
    RwTexCoords            *baseTexCoords;
    RwRGBA                 *basePreLightColors;

    rpDMorphGeometryData   *geometryData;
    RpDMorphTarget         *dMorphTarget;
    rpDMorphRLE            *rle;

    RwUInt32                size;
    void                   *memory;

    RwUInt32                iCode, iVert, iDelta, nextVerts;

    RWAPIFUNCTION(RWSTRING("RpDMorphGeometryAddDMorphTarget"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(geometry);

    /* Lets check we only morph something the base has. */
    RWASSERT((flags & RpGeometryGetFlags(geometry)) == flags);

    /* Lets get the base morph information. */
    baseVertices = RPDMORPHGEOMETRYGETVERTICES(geometry);
    baseNormals = RPDMORPHGEOMETRYGETNORMALS(geometry);
    basePreLightColors = RPDMORPHGEOMETRYGETPRELIGHTCOLORS(geometry);
    baseTexCoords = RPDMORPHGEOMETRYGETTEXCOORDS(geometry);

    /* Get our geometry extension data. */
    geometryData = *RPDMORPHGEOMETRYGETDATA(geometry);
    RWASSERT(geometryData);
    RWASSERT(geometryData->numDMorphTargets > index);

    /* Initialize dmorph target */
    dMorphTarget = &geometryData->dMorphTargets[index];
    rle = &dMorphTarget->rle;
    dMorphTarget->flags = flags;
    dMorphTarget->geometry = geometry;

    if(flags & rpGEOMETRYPOSITIONS)
    {
        /* Calculate fully morphed bounding sphere. */
        DMorphTargetCalcBoundingSphere(&dMorphTarget->boundingSphere,
            vertices, geometry->numVertices);
    }
    else
    {
        /* Copy the geometry's bounding sphere. */
        RwSphereAssign(&dMorphTarget->boundingSphere,
                       &geometry->morphTarget->boundingSphere);
    }

    /* Lets generate the control block to remove the zero deltas.
       Start the generator with the maximum number of elements. */
    if (!DMorphRLEGenerate(baseVertices,       vertices,
                           baseNormals,        normals,
                           basePreLightColors, preLightColors,
                           baseTexCoords,      texCoords,
                           geometry->numVertices,
                           flags,
                           rle,
                           dMorphTarget->boundingSphere.radius))
    {
        RWRETURN((RpGeometry *)NULL);
    }

    /*
     *  Set up memory for the RLE deltas.
     *  Also set up lock flags.
     */

    size = ((flags & rpGEOMETRYPOSITIONS) ? sizeof(RwV3d) : 0)
         + ((flags & rpGEOMETRYNORMALS)   ? sizeof(RwV3d) : 0)
         + ((flags & rpGEOMETRYPRELIT)    ? sizeof(RwRGBA) : 0)
         + ((flags & rpGEOMETRYTEXTURED)  ? sizeof(RwTexCoords) : 0);

    size *= rle->numDataElements;

    rle->data = RwMalloc(size,
        rwID_DMORPHPLUGIN | rwMEMHINTDUR_EVENT);
    if (!rle->data)
    {
        RWERROR((E_RW_NOMEM, size));
        RWRETURN((RpGeometry *)NULL);
    }

    memory = rle->data;
    dMorphTarget->lockFlags = (RpGeometryLockMode)0;
    if(flags & rpGEOMETRYPOSITIONS)
    {
        dMorphTarget->vertices = (RwV3d *)memory;
        memory = (RwV3d *)memory + rle->numDataElements;
        dMorphTarget->lockFlags |= rpGEOMETRYLOCKVERTICES;
    }
    if (flags & rpGEOMETRYNORMALS)
    {
        dMorphTarget->normals = (RwV3d *)memory;
        memory = (RwV3d *)memory + rle->numDataElements;
        dMorphTarget->lockFlags |= rpGEOMETRYLOCKNORMALS;
    }
    if (flags & rpGEOMETRYPRELIT)
    {
        dMorphTarget->preLightColors = (RwRGBA *)memory;
        memory = (RwRGBA *)memory + rle->numDataElements;
        dMorphTarget->lockFlags |= rpGEOMETRYLOCKPRELIGHT;
    }
    if (flags & rpGEOMETRYTEXTURED)
    {
        dMorphTarget->texCoords = (RwTexCoords *) memory;
        dMorphTarget->lockFlags |= rpGEOMETRYLOCKTEXCOORDS;
    }

    #if defined(D3D9_DRVMODEL_H)
    {
        RwUInt32 usageFlags;

        usageFlags = RpD3D9GeometryGetUsageFlags(geometry);

        usageFlags |= dMorphTarget->lockFlags;

        RpD3D9GeometrySetUsageFlags(geometry, usageFlags);
    }
    #endif

    /*
     * Iterate through the RLE data, and calculate vertex deltas
     */

    iVert = 0;
    iDelta = 0;
    for (iCode=0; iCode < rle->numCodeElements; iCode++)
    {
        nextVerts = iVert + (rle->code[iCode] & 0x7f);

        /* Do we skip a block of vertices? */
        if (!(rle->code[iCode] & 0x80))
        {
            iVert = nextVerts;
        }
        else
        {
            /* Store deltas for the vertices */
            for (;iVert < nextVerts; iVert++, iDelta++)
            {
                if (flags & rpGEOMETRYPOSITIONS)
                {
                    RwV3dSub(&dMorphTarget->vertices[iDelta],
                        &vertices[iVert], &baseVertices[iVert]);
                }
                if (flags & rpGEOMETRYNORMALS)
                {
                    RwV3dSub(&dMorphTarget->normals[iDelta],
                        &normals[iVert], &baseNormals[iVert]);
                }
                if (flags & rpGEOMETRYPRELIT)
                {
                    RGBASubMacro(&dMorphTarget->preLightColors[iDelta],
                        &preLightColors[iVert], &basePreLightColors[iVert]);
                }
                if (flags & rpGEOMETRYTEXTURED)
                {
                    TexCoordsSubMacro(&dMorphTarget->texCoords[iDelta],
                        &texCoords[iVert], &baseTexCoords[iVert]);
                }
            }
        }
    }

    RWRETURN(geometry);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphGeometryRemoveDMorphTarget is used to
 * remove the index delta morph target from the base
 * geometry. Once removed the index is then available
 * for a new delta morph target.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param geometry  Pointer to the base geometry.
 * \param index     Index of the delta morph target.
 *
 * \return Returns NULL if there is an error, or otherwise
 *         a pointer to the base geometry.
 *
 * \see RpDMorphGeometryDestroyDMorphTargets
 * \see RpDMorphGeometryCreateDMorphTargets
 * \see RpDMorphGeometryAddDMorphTarget
 */
RpGeometry *
RpDMorphGeometryRemoveDMorphTarget( RpGeometry *geometry,
                                    RwUInt32 index )
{
    rpDMorphGeometryData *geometryData;
    RpDMorphTarget     *dMorphTarget;

    RWAPIFUNCTION(RWSTRING("RpDMorphGeometryRemoveDMorphTarget"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(geometry);

    /* Get our geometry extension data. */
    geometryData = *RPDMORPHGEOMETRYGETDATA(geometry);
    RWASSERT(geometryData);
    RWASSERT(index < geometryData->numDMorphTargets);

    dMorphTarget = &(geometryData->dMorphTargets[index]);

    /* Free up the DMorphTargets data. */
    if(dMorphTarget->name)
    {
        RwFree(dMorphTarget->name);
    }

    if(dMorphTarget->rle.code)
    {
        RwFree(dMorphTarget->rle.code);
    }

    if(dMorphTarget->rle.data)
    {
        /* This automatically frees the vertices, normals, etc. */
        RwFree(dMorphTarget->rle.data);
    }

    /* Clean the DMorphTarget. */
    memset(dMorphTarget, 0, sizeof(RpDMorphTarget));

    RWRETURN(geometry);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphGeometryGetDMorphTarget is used to
 * get the indexed delta morph target of the base geometry.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param geometry  Pointer to the base geometry.
 * \param index     Index of the delta morph target.
 *
 * \return Returns NULL if there is an error, or otherwise
 *         a pointer to the delta morph target.
 *
 * \see RpDMorphGeometryCreateDMorphTargets
 * \see RpDMorphGeometryGetNumDMorphTargets
 */
RpDMorphTarget *
RpDMorphGeometryGetDMorphTarget( const RpGeometry *geometry,
                                 RwUInt32 index )
{
    const rpDMorphGeometryData *geometryData;

    RWAPIFUNCTION(RWSTRING("RpDMorphGeometryGetDMorphTarget"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(geometry);

    /* Get our geometry extension data. */
    geometryData = *RPDMORPHGEOMETRYGETCONSTDATA(geometry);
    RWASSERT(geometryData);
    RWASSERT(index < geometryData->numDMorphTargets);

    /* Offer up the DMorphTarget at index. */
    RWRETURN(&(geometryData->dMorphTargets[index]));
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphGeometryGetNumDMorphTargets is used to
 * get the number of delta morph targets that were created
 * by a call to \ref RpDMorphGeometryCreateDMorphTargets.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param geometry  Pointer to the base geometry.
 *
 * \return Returns the number of delta morph targets.
 *
 * \see RpDMorphGeometryCreateDMorphTargets
 * \see RpDMorphGeometryGetDMorphTarget
 */
RwUInt32
RpDMorphGeometryGetNumDMorphTargets( const RpGeometry *geometry )
{
    const rpDMorphGeometryData *geometryData;

    RWAPIFUNCTION(RWSTRING("RpDMorphGeometryGetNumDMorphTargets"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(geometry);

    /* Get our geometry extension data. */
    geometryData = *RPDMORPHGEOMETRYGETCONSTDATA(geometry);

    if( NULL != geometryData )
    {
        /* Return the number of DMorphTargets. */
        RWRETURN(geometryData->numDMorphTargets);
    }
    else
    {
        /* Return zero. */
        RWRETURN(0);
    }
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphGeometryTransformDMorphTargets is used to
 * apply the specified transformation matrix to the given
 * geometry. The transformation is applied equally to all
 * delta morph targets defined on the base geometry and for
 * each delta morph target transforms both the vertex
 * position deltas and vertex normal deltas.
 *
 * Note that the transformation modifies the delta morph
 * target data and is permanent. It does not modify the
 * base geometry, this should be done with
 * \ref RpGeometryTransform.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param geometry Pointer to the base geometry.
 * \param matrix   Pointer to the matrix describing the
 *                 transformation.
 *
 * \return Returns pointer to the geometry if successfull
 *         or NULL if there is an error.
 */
RpGeometry *
RpDMorphGeometryTransformDMorphTargets( RpGeometry *geometry,
                                        const RwMatrix *matrix )
{
    rpDMorphGeometryData *geometryData;

    RwUInt32 iDMorph;

    RWAPIFUNCTION(RWSTRING("RpDMorphGeometryTransformDMorphTargets"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(NULL != geometry);
    RWASSERT(NULL != matrix);
    RWASSERTISTYPE(geometry, rpGEOMETRY);
    RWASSERT(0 < geometry->refCount);

    /* Get our geometry extension data. */
    geometryData = *RPDMORPHGEOMETRYGETDATA(geometry);

    for( iDMorph = 0; iDMorph < geometryData->numDMorphTargets; iDMorph++ )
    {
        RpDMorphTarget *dMorphTarget;

        dMorphTarget = &(geometryData->dMorphTargets[iDMorph]);

        if(0 != (dMorphTarget->flags & rpGEOMETRYPOSITIONS))
        {
            RwV3dTransformVectors( dMorphTarget->vertices,
                                   dMorphTarget->vertices,
                                   dMorphTarget->rle.numDataElements,
                                   matrix );
        }

        if(0 != (dMorphTarget->flags & rpGEOMETRYNORMALS))
        {
            RwV3dTransformVectors( dMorphTarget->normals,
                                   dMorphTarget->normals,
                                   dMorphTarget->rle.numDataElements,
                                   matrix );
        }
    }

    RWRETURN(geometry);
}

/*--- DMorphTarget functions -----------------------------------------------*/

/**
 * \ingroup rpdmorph
 * \ref RpDMorphTargetGetBoundingSphere is used to
 * get the bounding sphere of the delta morph target.
 * The bounding sphere returned is returned as if the
 * delta morph had been fully applied.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param dMorphTarget  Pointer to the delta morph target.
 *
 * \return Returns NULL if there is an error, or otherwise
 *         a pointer to the bounding sphere.
 *
 * \see RpDMorphGeometryAddDMorphTarget
 */
const RwSphere *
RpDMorphTargetGetBoundingSphere( const RpDMorphTarget *dMorphTarget )
{
    RWAPIFUNCTION(RWSTRING("RpDMorphTargetGetBoundingSphere"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(dMorphTarget);

    /* Offer up the DMorphTargets bounding sphere. */
    RWRETURN(&(dMorphTarget->boundingSphere));
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphTargetSetName is used to set the name
 * of the delta morph target.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param dMorphTarget  Pointer to the delta morph target.
 * \param name          Pointer to the given name of the delta morph target.
 *
 * \return Returns NULL if there is an error, or otherwise
 *         a pointer to the delta morph target.
 *
 * \see RpDMorphTargetGetName
 */
RpDMorphTarget *
RpDMorphTargetSetName( RpDMorphTarget *dMorphTarget,
                       RwChar *name )
{
    RWAPIFUNCTION(RWSTRING("RpDMorphTargetSetName"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(dMorphTarget);
    RWASSERT(name);

    /* Lets make a copy of the name for the DMorphTarget. */
    rwstrdup(dMorphTarget->name, name );
    RWASSERT(dMorphTarget->name);

    RWRETURN(dMorphTarget);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphTargetGetName is used to get the name
 * of the delta morph target.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param dMorphTarget  Pointer to the delta morph target.
 *
 * \return Returns NULL if there is an error, or otherwise
 *         a pointer to the name.
 *
 * \see RpDMorphTargetSetName
 */
RwChar *
RpDMorphTargetGetName( RpDMorphTarget *dMorphTarget )
{
    RWAPIFUNCTION(RWSTRING("RpDMorphTargetGetName"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(dMorphTarget);

    /* Offer up the DMorphTargets name. */
    RWRETURN(dMorphTarget->name);
}

/**
 * \ingroup rpdmorph
 * \ref RpDMorphTargetGetFlags is used to get elements
 * of geometry that the delta morph target contains.
 *
 * The world and delta morph plugins must be attached
 * before using this function. The header file rpdmorph.h
 * is required.
 *
 * \param dMorphTarget  Pointer to the delta morph target.
 *
 * \return Returns the geometry flags of the delta morph
 *         target.
 *
 * \see RpGeometryGetFlags
 * \see RpDMorphGeometryAddDMorphTarget
 */
RpGeometryFlag
RpDMorphTargetGetFlags( RpDMorphTarget *dMorphTarget )
{
    RWAPIFUNCTION(RWSTRING("RpDMorphTargetGetFlags"));
    RWASSERT(rpDMorphModule.numInstances);
    RWASSERT(dMorphTarget);

    /* Offer up the DMorphTargets flags. */
    RWRETURN((RpGeometryFlag)dMorphTarget->flags);
}
