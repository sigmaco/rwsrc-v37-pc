
/* *INDENT-OFF* */

/****************************************************************************
 *
 * File: rwg/rwsdk/plugin/adc/rpadc.c
 *
 * Copyright (C) 2002 Criterion Technologies.
 *
 * Purpose: RenderWare ADC flag generation plugin
 *
 ****************************************************************************/

/**
 * \ingroup rpadc
 * \page rpadcoverview RpADC Plugin Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rpadc.h
 * \li \b Libraries: rwcore, rpworld, rpapc.h
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach,
 *     \ref RpADCPluginAttach
 *
 * \subsection adcoverview Overview
 * This plugin provides tools for triangle-strips degenerated vertices
 * reduction on the PlayStation 2 platform.
 *
 * In order to build continuous triangle-strip structure suitable for
 * rendering on platforms such as the PlayStation 2, degenerated vertices
 * are inserted to create jumps from triangle to triangle through zero sized
 * triangles. Even zero size triangles have no effect on the final scene
 * rendering result, their vertices are as expensive to store and process
 * as useful triangles vertices.
 *
 * The ADC plugin uses a PlayStation 2 platform ability to flag triangles as
 * \e not to be drawn instead of degenerated vertices insertion, reducing the
 * amount of memory, DMA upload, and Vector Unit processing cost.
 *
 * Triangle-strips indices are purged from degenerated vertex indices, and an
 * extra array of information is used to store an ADC flag information for
 * each index. The ADC information is instanced in the fourth field of the
 * position cluster. To allow this instancing format, the CL_XYZW cluster
 * is used in place of the usual CL_XYZ position cluster. This extra 32-bit
 * word as a memory cost that is absorbed by the reduction of vertices to be
 * instanced after degenerated vertices elimination. An ADC converted
 * geometry or world sector can only be rendered through a rendering
 * pipeline crafted to support the ADC flag information.
 *
 * Before any of the plugin functions are used, the plugin
 * should be attached using \ref RpADCPluginAttach.
 *
 * \par Basic Usage
 *
 * The ADC plugin conversion tools can be used on any triangle stripped object.
 * Geometries and world sectors can be converted respectively using
 *
 * \li \ref RpADCGeometryConvertIgnoreWinding
 * \li \ref RpADCGeometryConvertPreserveWinding
 *
 * and
 *
 * \li \ref RpADCWorldSectorConvertIgnoreWinding
 * \li \ref RpADCWorldSectorConvertPreserveWinding
 *
 * The PreserveWinding functions are suitable for objects that require facing
 * information, for example, a world to be rendered with back face culling
 * turned on. A few degenerated vertices are still used in order to preserve
 * the strip structure from winding inversions.
 *
 * In order to query if a geometry or a world sector is formatted for ADC
 * flag support, the two following functions can be invoked
 *
 * \li \ref RpADCGeometryGetADCSupport
 * \li \ref RpADCWorldSectorGetADCSupport
 */

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <string.h>

#include "rwcore.h"
#include "rpworld.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpcriter.h"

#include "rpadc.h"

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/
typedef struct rpADCDegenerateRemovalStructureTag
{
    RwUInt32      numVertices;
    RwUInt32      numOrigVertices;
    RwBool        preserveWinding;
    RwInt8       *adcBits;
} rpADCRemovalStructure;

/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/
#define ADCDEBUGx

#define RPADCGEOMETRYGETEXTDATA(geometry) \
    ((RpADCData *)(((RwUInt8 *)geometry) + _rpADCGeometryOffset))

#define RPADCGEOMETRYGETCONSTEXTDATA(geometry) \
    ((const RpADCData *)(((const RwUInt8 *)geometry) + _rpADCGeometryOffset))

#define RPADCSECTORGETEXTDATA(sector) \
    ((RpADCData *)(((RwUInt8 *)sector) + _rpADCSectorOffset))

#define RPADCSECTORGETCONSTEXTDATA(sector) \
    ((const RpADCData *)(((const RwUInt8 *)sector) + _rpADCSectorOffset))

#define ROUNDUP4(x) (((RwUInt32)(x) + 4 - 1) & ~(4 - 1))

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
static RwInt32 _rpADCGeometryOffset = 0;
static RwInt32 _rpADCSectorOffset = 0;
static RpADCMeshCallback _rpADCMeshCallback = NULL;

#if (defined(ADCDEBUG)) /*-------------------**/
                                            /**/
static RwUInt32 _rpADCLocalIReminder = 0;   /**/
static RwUInt32 _rpADCLocalICounter  = 0;   /**/
static RwUInt32 _rpADCLocalTCounter  = 0;   /**/
static RwUInt32 _rpADCLocalVCounter  = 0;   /**/
static RwUInt32 _rpADCLocalFCounter  = 0;   /**/
                                            /**/
#endif /* (defined(ADCDEBUG)) ---------------**/

#if (defined(RWDEBUG))
long rpADCStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/

/*---------------------------------------------*/
/*--   ADC flag geometry Index Pre-Counter   --*/
/*---------------------------------------------*/
static RpMesh *
_rpADCCountVerticesAfterRemoval( RpMesh *pMesh,
                                 RpMeshHeader *pMeshHeader __RWUNUSED__,
                                 void *pData )
{
    rpADCRemovalStructure *adcData = (rpADCRemovalStructure*)pData;
    RwUInt32               i;
    RWFUNCTION(RWSTRING("_rpADCCountVerticesAfterRemoval"));
    RWASSERT(NULL != pMesh);

    /*-- Increment Count ----------------------------*/
    adcData->numOrigVertices += pMesh->numIndices;

    /*-- Check winding order flag -------------------*/
    if(adcData->preserveWinding)
    {
        /*-- Num verts without ADC keeping winding --*/
        for(i = 0; i < pMesh->numIndices; i++)
        {
            /*-- Join-strip degenerates -------------*/
            if((i > 1) && ((pMesh->indices[i]) == (pMesh->indices[i-1])))
            {
                /*-- Preserve winding order ---------*/
                if(    (pMesh->indices[i-1] == pMesh->indices[i-2])
                    || !(    (pMesh->indices[i-2] == pMesh->indices[i-3])
                          || (pMesh->indices[i+2] == pMesh->indices[i+1]) ) )
                {
                    /*-- Normal vertex --------------*/
                     adcData->numVertices++;
                }
                else
                {
                    /*-- ADC vertex -----------------*/
                }
            }
            else
            {
                /*-- Normal vertex ------------------*/
                adcData->numVertices++;
            }
        }
    }
    else
    {
        /*-- Num verts without ADC ------------------*/
        for(i = 0; i < pMesh->numIndices; i++)
        {
            /*-- Join-strip degenerates -------------*/
            if((i > 0) && ((pMesh->indices[i]) == (pMesh->indices[i-1])))
            {
                /*-- ADC vertex ---------------------*/
            }
            else
            {
                /*-- Normal vertex ------------------*/
                adcData->numVertices++;
            }
        }
    }
    RWRETURN(pMesh);
}

/*---------------------------------------------*/
/*-- Degenerates ADC replacement process     --*/
/*---------------------------------------------*/
static RpMesh *
_rpADCRemoveDegeneratesAndInsertADC( RpMesh *pMesh,
                                     RpMeshHeader *pMeshHeader,
                                     void *pData)
{
    rpADCRemovalStructure *adcData     = (rpADCRemovalStructure*)pData;
    RwImVertexIndex       *pSourceIndex = NULL;
    RwImVertexIndex       *pDestIndex   = pMesh->indices;
    RwImVertexIndex       *pDestWatcher = NULL;
    RwInt8                *pADCWatcher  = NULL;
    RwUInt32               numIndices   = 0;
    RwUInt32               i;
    RWFUNCTION(RWSTRING("_rpADCRemoveDegeneratesAndInsertADC"));

    /* If there's a callback, call it and only return true. */
    if((NULL != _rpADCMeshCallback) &&
       (FALSE == _rpADCMeshCallback(pMesh)))
    {
        /* Clear all ADC flags for this geometry and skip them */
        memset(adcData->adcBits, 0, sizeof(RwInt8)*pMesh->numIndices);
        adcData->adcBits += pMesh->numIndices;
        RWRETURN(pMesh);
    }

    /*-- Cache indices ------------------------------*/
    pSourceIndex = (RwImVertexIndex *)RwMalloc(
                       sizeof(RwImVertexIndex) * pMesh->numIndices,
                       rwID_ADCPLUGIN | rwMEMHINTDUR_FUNCTION);
    memcpy( pSourceIndex,
            pMesh->indices,
            sizeof(RwImVertexIndex) * pMesh->numIndices );

    /*-- Copy back and insert ADC bits --------------*/
    pDestIndex   = pMesh->indices;
    pDestWatcher = pDestIndex;
    pADCWatcher  = adcData->adcBits;

    /*-- Check winding order flag -------------------*/
    if(adcData->preserveWinding)
    {
        for(i = 0; i < pMesh->numIndices; i++)
        {
            /*-- Join-strip degenerates -------------*/
            if((i > 1) && (pSourceIndex[i] == pSourceIndex[i-1]))
            {
                /*-- Preserve winding order ---------*/
                if(      (pSourceIndex[i-1] == pSourceIndex[i-2])
                    || !(    (pSourceIndex[i-2] == pSourceIndex[i-3])
                          || (pSourceIndex[i+2] == pSourceIndex[i+1]) ) )
                {
                    /* Write the new index into the old and increment
                     * the vertex counter. */
                    (*pDestIndex) = pSourceIndex[i];
                    numIndices++;
                    /* Point to the next dest vertex. */
                    pDestIndex++;
                    /* Clear the ADC since we don't need it. */
                    *adcData->adcBits = 0;
                    /* Increment ADC buffer. */
                    adcData->adcBits++;
                }
                else
                {
                    /* Flag this vertex for ADC support. */
                    *adcData->adcBits = 1;
                }
            }
            else
            {
                /* Write the new index into the old and increment the
                 * vertex counter. */
                (*pDestIndex) = pSourceIndex[i];
                numIndices++;
                /* Point to the next dest vertex. */
                pDestIndex++;
                /* Increment ADC buffer. */
                adcData->adcBits++;
            }
        }
        for(i = 0; i < numIndices; i++)
        {
            /*-- ADC flag All zero size triangles ---*/
            if((i > 1) && (
                          ((pDestWatcher[i])   == (pDestWatcher[i-1])) ||
                          ((pDestWatcher[i])   == (pDestWatcher[i-2])) ||
                          ((pDestWatcher[i-1]) == (pDestWatcher[i-2]))
                          ))
            {
                pADCWatcher[i] = 1;
            }
        }

#if (defined(ADCDEBUG)) /*-- Getting ADC flag counter -*/
    {
        for(i = 0; i < numIndices; i++)
        {
            if(pADCWatcher[i] == 1)
            {
                _rpADCLocalFCounter++;
            }
        }
    }
#endif                  /*-- Getting ADC flag counter -*/

    }
    else
    {
        for(i = 0; i < pMesh->numIndices; i++)
        {
            /*-- Join-strip degenerates -------------*/
            if((i > 0) && ((pSourceIndex[i]) == (pSourceIndex[i-1])))
            {
                *adcData->adcBits = 1;
            }
            else
            {
                /* Write the new index into the old and increment
                 * the vertex counter. */
                (*pDestIndex) = pSourceIndex[i];
                numIndices++;
                /* Point to the next dest vertex. */
                pDestIndex++;
                /* Increment ADC buffer. */
                adcData->adcBits++;
            }
        }
        for(i = 0; i < numIndices; i++)
        {
            /*-- ADC flag All zero size triangles ---*/
            if((i > 1) && (
                          ((pDestWatcher[i])   == (pDestWatcher[i-1])) ||
                          ((pDestWatcher[i])   == (pDestWatcher[i-2])) ||
                          ((pDestWatcher[i-1]) == (pDestWatcher[i-2]))
                          ))
            {
                pADCWatcher[i] = 1;
            }
        }

#if (defined(ADCDEBUG)) /*-- Getting ADC flag counter -*/
    {
        pADCWatcher  = adcData->adcBits;
        for(i = 0; i < numIndices; i++)
        {
            if(pADCWatcher[i] == 1)
            {
                _rpADCLocalFCounter++;
            }
        }
    }
#endif                  /*-- Getting ADC flag counter -*/

    }

    /*-- Update Mesh Information --------------------*/
    /* Decrease the mesh header for the indices we've removed. */
    pMeshHeader->totalIndicesInMesh -= (pMesh->numIndices - numIndices);
    /* Update this meshes number of indices. */
    pMesh->numIndices = numIndices;
    RwFree(pSourceIndex);
    RWRETURN(pMesh);
}

static RwStream *
_rpADCStreamWrite( RwStream *stream,
                   const RpADCData *adcData,
                   RwBool streamADCbits,
                   RwInt32 binaryLength )
{
    RwInt32 streamSize;

    RWFUNCTION(RWSTRING("_rpADCStreamWrite"));
    RWASSERT(NULL != stream);
    RWASSERT(NULL != adcData);

    /* Set our total write size. */
    streamSize = binaryLength - rwCHUNKHEADERSIZE;

    /* First write our internal header to store version number (a current
     * oversight in the stream API is that we don't get the version
     * number passed into this readCB for its own chunk header!) */
    if (!RwStreamWriteChunkHeader( stream,
                                   rwID_ADCPLUGIN,
                                   streamSize ))
    {
        RwDebugSendMessage( rwDEBUGERROR,
                            "_rpADCGeometryStreamWrite",
                            "Could not write stream data header");
        RWRETURN((RwStream *)NULL);
    }

    /* We only stream the ADC bits if we're not preinstancing. */
    if(streamADCbits)
    {
        void *bytes;
        RwInt32 numBits;
        RwInt32 numInts;

        /* Collect the data to write. */
        bytes = (void *)adcData->adcBits;
        RWASSERT(NULL != bytes);
        RWASSERT(adcData->adcFormatted);
        numBits = adcData->numBits;

        /* We need to write complete ints. Hence increase number of
         * bits to nearest mod 4. */
        numInts = ROUNDUP4(numBits);

        /* Write the number of bits, then the adc bits. */
        stream = RwStreamWriteInt32(stream, &numBits, sizeof(RwInt32));
        RWASSERT(NULL != stream);
        stream = RwStreamWrite(stream, bytes, numInts);
        RWASSERT(NULL != stream);
    }
    else
    {
        /* Just write zero bits. */
        RwInt32 numBits = 0;

        /* Write the number of bits, then the adc bits. */
        stream = RwStreamWriteInt32(stream, &numBits, sizeof(RwInt32));
        RWASSERT(NULL != stream);
    }

    RWRETURN(stream);
}

static RwStream *
_rpADCStreamRead( RwStream *stream,
                  RpADCData *adcData,
                  RwInt32 binaryLength __RWUNUSEDRELEASE__ )
{
    RwUInt32 subChunkSize, version;

    RWFUNCTION(RWSTRING("_rpADCStreamRead"));
    RWASSERT(NULL != stream);
    RWASSERT(NULL != adcData);

    /* First read our internal header to get version number (a current
     * oversight in the stream API is that we don't get the version
     * number passed into this readCB for its own chunk header!) */
    if( !RwStreamFindChunk( stream,
                            rwID_ADCPLUGIN,
                            &subChunkSize,
                            &version ) )
    {
        RwDebugSendMessage( rwDEBUGERROR,
                            "_rpADCStreamRead",
                            "Could not read stream data header" );
        RWRETURN((RwStream *)NULL);
    }

    RWASSERT(binaryLength == (RwInt32)(subChunkSize + rwCHUNKHEADERSIZE));

    /* Once there are multiple plugin data file versions,
     * test 'version' here */
    {
        RwInt32 numBits;
        RwInt32 numInts;
        void *bytes;

        /* Read the number of adc bits. */
        stream = RwStreamReadInt32(stream, &numBits, sizeof(RwInt32));
        RWASSERT(NULL != stream);

        /* Test to see if we actually have ADC bits,
         * a preinstance geometry will have ADC plugin data but
         * no ADC bits. */
        if(0 != numBits)
        {
            /* We have bits, now read them in. */
            numInts = ROUNDUP4(numBits);
            /* Malloc space for the stream read. */
            bytes = (void *)RwMalloc(sizeof(RwInt8) * numInts,
                             rwID_ADCPLUGIN | rwMEMHINTDUR_EVENT);
            RWASSERT(NULL != bytes);
            /* Read the atomic plugin data */
            stream = RwStreamReadInt32(stream, bytes, numInts);
            RWASSERT(NULL != stream);
        }
        else
        {
            bytes = NULL;
        }

        adcData->adcFormatted = TRUE;
        adcData->adcBits = (RwInt8 *)bytes;
        /* Whilst we've loaded in numInts we store numBits. */
        adcData->numBits = numBits;
    }

    RWRETURN(stream);
}

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*---------------------------------------------*/
/*--       Engine Data Constructor           --*/
/*---------------------------------------------*/
static void *
_rpADCEngineDataConstructor( void *instance,
                             RwInt32 offset __RWUNUSED__,
                             RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("_rpADCEngineDataConstructor"));
    RWASSERT(NULL != instance);
    RWRETURN(instance);
}

/*---------------------------------------------*/
/*--       Global Data Destructor            --*/
/*---------------------------------------------*/
static void *
_rpADCEngineDataDestructor( void *instance,
                            RwInt32 offset __RWUNUSED__,
                            RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("_rpADCEngineDataDestructor"));
    RWASSERT(NULL != instance);
    _rpADCGeometryOffset = 0;
    _rpADCSectorOffset = 0;
    RWRETURN(instance);
}

/*---------------------------------------------*/
/*--       Local Data Constructor            --*/
/*---------------------------------------------*/
static void *
_rpADCGeometryDataConstructor( void *object,
                               RwInt32 offset __RWUNUSED__,
                               RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("_rpADCGeometryDataConstructor"));
    RWASSERT(NULL != object);
    RPADCGEOMETRYGETEXTDATA(object)->adcFormatted = FALSE;
    RPADCGEOMETRYGETEXTDATA(object)->adcBits      = NULL;
    RPADCGEOMETRYGETEXTDATA(object)->numBits      = 0;
    RWRETURN(object);
}

/*---------------------------------------------*/
/*--       Local Data Destructor             --*/
/*---------------------------------------------*/
static void *
_rpADCGeometryDataDestructor( void *object,
                              RwInt32 offset __RWUNUSED__,
                              RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("_rpADCGeometryDataDestructor"));
    RWASSERT(NULL != object);
    if( (RPADCGEOMETRYGETEXTDATA(object)->adcBits) &&
        (((RpGeometry *)object)->refCount == 1) )
    {
        RwFree(RPADCGEOMETRYGETEXTDATA(object)->adcBits);
        RPADCGEOMETRYGETEXTDATA(object)->adcBits      = NULL;
        RPADCGEOMETRYGETEXTDATA(object)->adcFormatted = FALSE;
        RPADCGEOMETRYGETEXTDATA(object)->numBits      = 0;
    }
    RWRETURN(object);
}

/*---------------------------------------------*/
/*--       Local Data Cloning                --*/
/*---------------------------------------------*/
static void *
_rpADCGeometryDataCopier( void *dstObject       __RWUNUSEDRELEASE__,
                          const void *srcObject __RWUNUSEDRELEASE__,
                          RwInt32 offset __RWUNUSED__,
                          RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("_rpADCGeometryDataCopier"));
    RWASSERT(NULL != srcObject);
    RWASSERT(NULL != dstObject);
    /* Illegal operation on a geometry. */
    RWRETURN(dstObject);
}

/*---------------------------------------------*/
/*--   Local Sector Data Constructor         --*/
/*---------------------------------------------*/
static void *
_rpADCSectorDataConstructor( void *object,
                             RwInt32 offset __RWUNUSED__,
                             RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("_rpADCSectorDataConstructor"));
    RWASSERT(NULL != object);
    RPADCSECTORGETEXTDATA(object)->adcFormatted = FALSE;
    RPADCSECTORGETEXTDATA(object)->adcBits      = NULL;
    RPADCSECTORGETEXTDATA(object)->numBits      = 0;
    RWRETURN(object);
}

/*---------------------------------------------*/
/*--   Local Sector Data Destructor          --*/
/*---------------------------------------------*/
static void *
_rpADCSectorDataDestructor( void *object,
                            RwInt32 offset __RWUNUSED__,
                            RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("_rpADCSectorDataDestructor"));
    RWASSERT(NULL != object);
    if(RPADCSECTORGETEXTDATA(object)->adcBits)
    {
        RwFree(RPADCSECTORGETEXTDATA(object)->adcBits);
        RPADCSECTORGETEXTDATA(object)->adcBits      = NULL;
        RPADCSECTORGETEXTDATA(object)->adcFormatted = FALSE;
        RPADCSECTORGETEXTDATA(object)->numBits      = 0;
    }
    RWRETURN(object);
}

/*---------------------------------------------*/
/*--   Local Sector Data Cloning             --*/
/*---------------------------------------------*/
static void *
_rpADCSectorDataCopier( void *dstObject       __RWUNUSEDRELEASE__,
                        const void *srcObject __RWUNUSEDRELEASE__,
                        RwInt32 offset __RWUNUSED__,
                        RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("_rpADCSectorDataCopier"));
    RWASSERT(NULL != srcObject);
    RWASSERT(NULL != dstObject);
    /* Illegal operation on a sector. */
    RWRETURN(dstObject);
}

/*---------------------------------------------*/
/*--   Geometry and Sector Streaming         --*/
/*---------------------------------------------*/
static RwStream *
_rpADCGeometryStreamWrite( RwStream *stream,
                           RwInt32 binaryLength,
                           const void *object,
                           RwInt32 offsetInObject __RWUNUSED__,
                           RwInt32 sizeInObject   __RWUNUSED__ )
{
    const RpGeometry *geometry;
    const RpADCData *adcData;
    RwBool streamADCBits;

    RWFUNCTION(RWSTRING("_rpADCGeometryStreamWrite"));
    RWASSERT(NULL != stream);
    geometry = (const RpGeometry *)object;
    RWASSERT(NULL != geometry);
    adcData = RPADCGEOMETRYGETCONSTEXTDATA(geometry);
    RWASSERT(NULL != adcData);

    /* Only steam ADC bits if we're *not* native. */
    streamADCBits = (0 == (RpGeometryGetFlags(geometry) & rpGEOMETRYNATIVE));

    RWRETURN(_rpADCStreamWrite(stream, adcData,
                               streamADCBits, binaryLength));
}

static RwStream *
_rpADCGeometryStreamRead( RwStream *stream,
                          RwInt32 binaryLength,
                          void *object,
                          RwInt32 offsetInObject __RWUNUSED__,
                          RwInt32 sizeInObject   __RWUNUSED__ )
{
    RpGeometry *geometry;
    RpADCData *adcData;

    RWFUNCTION(RWSTRING("_rpADCGeometryStreamRead"));
    RWASSERT(NULL != stream);
    geometry = (RpGeometry *)object;
    RWASSERT(NULL != geometry);
    adcData = RPADCGEOMETRYGETEXTDATA(geometry);
    RWASSERT(NULL != adcData);

    RWRETURN(_rpADCStreamRead(stream, adcData, binaryLength));
}

static RwInt32
_rpADCGeometryStreamGetSize( const void *object,
                             RwInt32 offsetInObject __RWUNUSED__,
                             RwInt32 sizeInObject   __RWUNUSED__ )
{
    const RpGeometry *geometry;
    const RpADCData *adcData;

    RwInt32 size;

    RWFUNCTION(RWSTRING("_rpADCGeometryStreamGetSize"));
    RWASSERT(NULL != object);
    geometry = (const RpGeometry *)object;
    RWASSERT(NULL != geometry);
    adcData = RPADCGEOMETRYGETCONSTEXTDATA(geometry);
    RWASSERT(NULL != adcData);

    /* Init size to 0 incase the geometry has no adc. */
    size = 0;

    if(adcData->adcFormatted)
    {
        /* At the moment the number of bits = the number of bytes. */
        size = rwCHUNKHEADERSIZE  /* Version chunk.  */
             + sizeof(RwInt32);   /* Number of bits. */

        /* If we're not preinstancing we store the adc bits. */
        if( 0 == (rpGEOMETRYNATIVE & RpGeometryGetFlags(geometry)) )
        {
            size += ROUNDUP4(adcData->numBits); /* ADC bits. */
        }
    }

    RWRETURN(size);
}

static RwStream *
_rpADCSectorStreamWrite( RwStream *stream,
                         RwInt32 binaryLength,
                         const void *object,
                         RwInt32 offsetInObject __RWUNUSED__,
                         RwInt32 sizeInObject   __RWUNUSED__ )
{
    const RpWorldSector *sector;
    const RpADCData *adcData;

    const RpWorld *world;
    RwBool streamADCBits;

    RWFUNCTION(RWSTRING("_rpADCSectorStreamWrite"));
    RWASSERT(NULL != stream);
    sector = (const RpWorldSector *)object;
    RWASSERT(NULL != sector);
    adcData = RPADCSECTORGETCONSTEXTDATA(sector);
    RWASSERT(NULL != adcData);

    world = RpWorldSectorGetWorld(sector);
    RWASSERT(NULL != world);

    /* Only steam ADC bits if we're *not* native. */
    streamADCBits = (0 == (RpWorldGetFlags(world) & rpWORLDNATIVE));

    RWRETURN(_rpADCStreamWrite(stream, adcData,
                               streamADCBits, binaryLength));
}

static RwStream *
_rpADCSectorStreamRead( RwStream *stream,
                        RwInt32 binaryLength,
                        void *object,
                        RwInt32 offsetInObject __RWUNUSED__,
                        RwInt32 sizeInObject   __RWUNUSED__ )
{
    RpWorldSector *sector;
    RpADCData *adcData;

    RWFUNCTION(RWSTRING("_rpADCSectorStreamRead"));
    RWASSERT(NULL != stream);
    sector = (RpWorldSector *)object;
    RWASSERT(NULL != sector);
    adcData = RPADCSECTORGETEXTDATA(sector);
    RWASSERT(NULL != adcData);

    RWRETURN(_rpADCStreamRead(stream, adcData, binaryLength));
}

static RwInt32
_rpADCSectorStreamGetSize( const void *object,
                           RwInt32 offsetInObject __RWUNUSED__,
                           RwInt32 sizeInObject   __RWUNUSED__ )
{
    const RpWorldSector *sector;
    const RpADCData *adcData;

    RwInt32 size;

    RWFUNCTION(RWSTRING("_rpADCSectorStreamGetSize"));
    RWASSERT(NULL != object);
    sector = (const RpWorldSector *)object;
    RWASSERT(NULL != sector);
    adcData = RPADCSECTORGETCONSTEXTDATA(sector);
    RWASSERT(NULL != adcData);

    /* Init size to 0 incase the geometry has no adc. */
    size = 0;

    if(adcData->adcFormatted)
    {
        RpWorld *world;

        /* At the moment the number of bits = the number of bytes. */
        size = rwCHUNKHEADERSIZE  /* Version chunk.  */
             + sizeof(RwInt32);   /* Number of bits. */

        /* Grab the sectors world so we can test the instance flags. */
        world = RpWorldSectorGetWorld(sector);
        RWASSERT(NULL != world);

        /* If we're not preinstancing we store the adc bits. */
        if( 0 == (rpWORLDNATIVE & RpWorldGetFlags(world)) )
        {
            size += ROUNDUP4(adcData->numBits); /* ADC bits. */
        }
    }

    RWRETURN(size);
}

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/*---------------------------------------------*/
/*--       Plugin Registration               --*/
/*---------------------------------------------*/

/**
 * \ingroup rpadc
 * \ref RpADCPluginAttach is used to attach the ADC plugin to the
 * RenderWare system. The plugin must be attached between initializing the
 * system with \ref RwEngineInit and opening it with \ref RwEngineOpen.
 *
 * Note that the include file rpadc.h is required and must be included by
 * an application wishing to use this facility. The ADC library is
 * contained in the file rpadc.lib.
 *
 * \return Returns TRUE if successful, FALSE otherwise
 * an error.
 *
 */
RwBool
RpADCPluginAttach(void)
{
    RwInt32 offset;
    RWAPIFUNCTION(RWSTRING("RpADCPluginAttach"));
    /*-- Pluging registration --------------------*/
    offset = RwEngineRegisterPlugin( 0, rwID_ADCPLUGIN,
                                     _rpADCEngineDataConstructor,
                                     _rpADCEngineDataDestructor );
    RWASSERT(0 <= offset);

    /*-- Geometry extension to ADC flag support --*/
    _rpADCGeometryOffset =
        RpGeometryRegisterPlugin( sizeof(RpADCData),
                                  rwID_ADCPLUGIN,
                                  _rpADCGeometryDataConstructor,
                                  _rpADCGeometryDataDestructor,
                                  _rpADCGeometryDataCopier );
    RWASSERT(0 < _rpADCGeometryOffset);


    /*-- WSector extension to ADC flag support  --*/
    _rpADCSectorOffset =
        RpWorldSectorRegisterPlugin( sizeof(RpADCData),
                                     rwID_ADCPLUGIN,
                                     _rpADCSectorDataConstructor,
                                     _rpADCSectorDataDestructor,
                                     _rpADCSectorDataCopier );
    RWASSERT(0 < _rpADCSectorOffset);

    /* Register the geometry and world sector apc streaming functions. */
    offset = RpGeometryRegisterPluginStream( rwID_ADCPLUGIN,
                                             _rpADCGeometryStreamRead,
                                             _rpADCGeometryStreamWrite,
                                             _rpADCGeometryStreamGetSize );
    RWASSERT(0 < offset);

    offset = RpWorldSectorRegisterPluginStream( rwID_ADCPLUGIN,
                                                _rpADCSectorStreamRead,
                                                _rpADCSectorStreamWrite,
                                                _rpADCSectorStreamGetSize );
    RWASSERT(0 < offset);

    /* Set the mesh callback to nothing */
    _rpADCMeshCallback = NULL;

    RWRETURN(TRUE);
}

/**
 * \ingroup rpadc
 * \ref RpADCGeometryConvertIgnoreWinding is used to convert a geometry
 * into an ADC support geometry while ignoring the triangle strip
 * winding order.
 *
 * \param geometry A pointer to the geometry
 *
 * \return TRUE on success, FALSE otherwise.
 *
 * \see RpADCGeometryConvertPreserveWinding
 * \see RpADCGeometryGetADCSupport
 */
RwBool
RpADCGeometryConvertIgnoreWinding(RpGeometry *geometry)
{
    RpADCData             *adcData = RPADCGEOMETRYGETEXTDATA(geometry);
    rpADCRemovalStructure  adcRemoval;
    RWAPIFUNCTION(RWSTRING("RpADCGeometryConvertIgnoreWinding"));
    RWASSERT(NULL != geometry);

    /*-- Test for Geometry convertion -----------------*/
    if(adcData->adcFormatted == TRUE)
    {
        RWRETURN(TRUE);
    }
    /*-- Clear struct and setup -----------------------*/
    adcRemoval.numVertices      = 0;
    adcRemoval.numOrigVertices  = 0;
    adcRemoval.preserveWinding = FALSE;

    /*-- Number of indices AFTER degenerates removal --*/
    RpGeometryForAllMeshes( geometry,
                            _rpADCCountVerticesAfterRemoval,
                            &adcRemoval );

    /*-- ADC flag memory allocation -------------------*/
    adcData->adcBits = (RwInt8 *)RwMalloc( sizeof(RwInt8) *
                                           adcRemoval.numVertices,
                                           rwID_ADCPLUGIN |
                                           rwMEMHINTDUR_EVENT );
    RWASSERT(NULL != adcData->adcBits);

    /*-- Clear ADC flag memory bloc to 0 --------------*/
    memset(adcData->adcBits, 0, sizeof(RwInt8) * adcRemoval.numVertices);

#if (defined(ADCDEBUG)) /*-- Saving Ratio Information -*/
    {
        RwUInt32 nvtx  = RpGeometryGetNumVertices(geometry);
        RwUInt32 ntrig = RpGeometryGetNumTriangles(geometry);

        _rpADCLocalIReminder += adcRemoval.numOrigVertices;
        _rpADCLocalICounter  += adcRemoval.numVertices;
        _rpADCLocalTCounter  += ntrig;
        _rpADCLocalVCounter  += nvtx;

        printf("Num vertices  %d\n", nvtx);
        printf("Num triangles %d\n", ntrig);
        printf("Old to new indices is %d to %d, or %.2f percent\n",
                    adcRemoval.numOrigVertices,
                    adcRemoval.numVertices,
                    ( 100.0f * ( (RwReal)adcRemoval.numOrigVertices
                               / (RwReal)adcRemoval.numVertices) )
                             - 100.0f );
        printf("Amount of duplication %.2f\n",
                    ((RwReal)adcRemoval.numVertices / (RwReal)nvtx) );
    }
#endif /* (defined(ADCDEBUG)) -------------------------*/

    /*-- Degenerates removal and AADC bit setup -------*/
    adcRemoval.adcBits = adcData->adcBits;
    RpGeometryForAllMeshes( geometry,
                            _rpADCRemoveDegeneratesAndInsertADC,
                            &adcRemoval );

    /*-- ADC flag process Terminated ------------------*/
    adcData->adcFormatted = TRUE;
    adcData->numBits = adcRemoval.numVertices;
    RWRETURN(TRUE);
}

/**
 * \ingroup rpadc
 * \ref RpADCGeometryConvertPreserveWinding is used to convert a geometry
 * into an ADC support geometry while preserving the triangle strip
 * winding order.
 *
 * \param geometry A pointer to the geometry
 *
 * \return TRUE on success, FALSE otherwise.
 *
 * \see RpADCGeometryConvertIgnoreWinding
 * \see RpADCGeometryGetADCSupport
 */
RwBool
RpADCGeometryConvertPreserveWinding(RpGeometry *geometry)
{
    RpADCData             *adcData = RPADCGEOMETRYGETEXTDATA(geometry);
    rpADCRemovalStructure  adcRemoval;
    RWAPIFUNCTION(RWSTRING("RpADCGeometryConvertPreserveWinding"));
    RWASSERT(NULL != geometry);

    /*-- Test for Geometry convertion -----------------*/
    if(adcData->adcFormatted == TRUE)
    {
        RWRETURN(TRUE);
    }
    /*-- Clear struct and setup -----------------------*/
    adcRemoval.numVertices      = 0;
    adcRemoval.numOrigVertices  = 0;
    adcRemoval.preserveWinding = TRUE;

    /*-- Number of indices AFTER degenerates removal --*/
    RpGeometryForAllMeshes( geometry,
                            _rpADCCountVerticesAfterRemoval,
                            &adcRemoval );

    /*-- ADC flag memory allocation -------------------*/
    adcData->adcBits = (RwInt8 *)RwMalloc( sizeof(RwInt8) *
                                         adcRemoval.numVertices,
                                         rwID_ADCPLUGIN |
                                         rwMEMHINTDUR_EVENT );
    RWASSERT(NULL != adcData->adcBits);

    /*-- Clear ADC flag memory bloc to 0 --------------*/
    memset(adcData->adcBits, 0, sizeof(RwInt8) *adcRemoval.numVertices);

#if (defined(ADCDEBUG)) /*-- Saving Ratio Information -*/
    {
        RwUInt32 nvtx  = RpGeometryGetNumVertices(geometry);
        RwUInt32 ntrig = RpGeometryGetNumTriangles(geometry);

        _rpADCLocalIReminder += adcRemoval.numOrigVertices;
        _rpADCLocalICounter  += adcRemoval.numVertices;
        _rpADCLocalTCounter  += ntrig;
        _rpADCLocalVCounter  += nvtx;

        printf("Num vertices  %d\n", nvtx);
        printf("Num triangles %d\n", ntrig);
        printf("Old to new indices is %d to %d, or %.2f percent\n",
                    adcRemoval.numOrigVertices,
                    adcRemoval.numVertices,
                    ( 100.0f * ( (RwReal)adcRemoval.numOrigVertices /
                                 (RwReal)adcRemoval.numVertices) ) - 100.0f );
        printf("Amount of duplication %.2f\n",
                    ((RwReal)adcRemoval.numVertices / (RwReal)nvtx) );
    }
#endif /* (defined(ADCDEBUG)) -------------------------*/

    /*-- Degenerates removal and AADC bit setup -------*/
    adcRemoval.adcBits = adcData->adcBits;
    RpGeometryForAllMeshes( geometry,
                            _rpADCRemoveDegeneratesAndInsertADC,
                            &adcRemoval );

    /*-- ADC flag process Terminated ------------------*/
    adcData->adcFormatted = TRUE;
    adcData->numBits = adcRemoval.numVertices;
    RWRETURN(TRUE);
}

/**
 * \ingroup rpadc
 * \ref RpADCWorldSectorConvertIgnoreWinding is used to convert a sector
 * into an ADC support sector while ignoring the triangle strips
 * winding order.
 *
 * \param sector A pointer to the sector
 *
 * \return TRUE on success, FALSE otherwise.
 *
 * \see RpADCWorldSectorConvertPreserveWinding
 * \see RpADCWorldSectorGetADCSupport
 */
RwBool
RpADCWorldSectorConvertIgnoreWinding(RpWorldSector *sector)
{
    RpADCData             *adcData = RPADCSECTORGETEXTDATA(sector);
    rpADCRemovalStructure  adcRemoval;
    RWAPIFUNCTION(RWSTRING("RpADCWorldSectorConvertIgnoreWinding"));
    RWASSERT(NULL != sector);

    /*-- Test for Geometry convertion -----------------*/
    if(adcData->adcFormatted == TRUE)
    {
        RWRETURN(TRUE);
    }
    /*-- Clear struct and setup -----------------------*/
    adcRemoval.numVertices      = 0;
    adcRemoval.numOrigVertices  = 0;
    adcRemoval.preserveWinding = FALSE;

    /*-- Number of indices AFTER degenerates removal --*/
    RpWorldSectorForAllMeshes( sector,
                               _rpADCCountVerticesAfterRemoval,
                               &adcRemoval );

    /*-- ADC flag memory allocation -------------------*/
    adcData->adcBits = (RwInt8 *)RwMalloc( sizeof(RwInt8) *
                                           adcRemoval.numVertices,
                                           rwID_ADCPLUGIN |
                                           rwMEMHINTDUR_EVENT );
    RWASSERT(NULL != adcData->adcBits);

    /*-- Clear ADC flag memory bloc to 0 --------------*/
    memset(adcData->adcBits, 0, sizeof(RwInt8) * adcRemoval.numVertices);

#if (defined(ADCDEBUG)) /*-- Saving Ratio Information -*/
    {
        RwUInt32 nvtx  = RpWorldSectorGetNumVertices(sector);
        RwUInt32 ntrig = RpWorldSectorGetNumTriangles(sector);

        _rpADCLocalIReminder += adcRemoval.numOrigVertices;
        _rpADCLocalICounter  += adcRemoval.numVertices;
        _rpADCLocalTCounter  += ntrig;
        _rpADCLocalVCounter  += nvtx;

        printf("Num vertices  %d\n", nvtx);
        printf("Num triangles %d\n", ntrig);
        printf("Old to new indices is %d to %d, or %.2f percent\n",
                    adcRemoval.numOrigVertices,
                    adcRemoval.numVertices,
                    ( 100.0f * ( (RwReal)adcRemoval.numOrigVertices /
                                 (RwReal)adcRemoval.numVertices) ) - 100.0f );
        printf("Amount of duplication %.2f\n",
                    ((RwReal)adcRemoval.numVertices / (RwReal)nvtx) );
    }
#endif /* (defined(ADCDEBUG)) -------------------------*/

    /*-- Degenerates removal and AADC bit setup -------*/
    adcRemoval.adcBits = adcData->adcBits;
    RpWorldSectorForAllMeshes( sector,
                               _rpADCRemoveDegeneratesAndInsertADC,
                               &adcRemoval );

    /*-- ADC flag process Terminated ------------------*/
    adcData->adcFormatted = TRUE;
    adcData->numBits = adcRemoval.numVertices;
    RWRETURN(TRUE);
}

/**
 * \ingroup rpadc
 * \ref RpADCWorldSectorConvertPreserveWinding is used to convert a sector
 * into an ADC support sector while preserving the triangle strips
 * winding order.
 *
 * \param sector A pointer to the sector
 *
 * \return TRUE on success, FALSE otherwise.
 *
 * \see RpADCWorldSectorConvertIgnoreWinding
 * \see RpADCWorldSectorGetADCSupport
 */
RwBool
RpADCWorldSectorConvertPreserveWinding(RpWorldSector *sector)
{
    RpADCData             *adcData = RPADCSECTORGETEXTDATA(sector);
    rpADCRemovalStructure  adcRemoval;
    RWAPIFUNCTION(RWSTRING("RpADCWorldSectorConvertPreserveWinding"));
    RWASSERT(NULL != sector);

    /*-- Test for Geometry convertion -----------------*/
    if(adcData->adcFormatted == TRUE)
    {
        RWRETURN(TRUE);
    }
    /*-- Clear struct and setup -----------------------*/
    adcRemoval.numVertices      = 0;
    adcRemoval.numOrigVertices  = 0;
    adcRemoval.preserveWinding = TRUE;

    /*-- Number of indices AFTER degenerates removal --*/
    RpWorldSectorForAllMeshes( sector,
                               _rpADCCountVerticesAfterRemoval,
                               &adcRemoval );

    /*-- ADC flag memory allocation -------------------*/
    adcData->adcBits = (RwInt8 *)RwMalloc( sizeof(RwInt8) *
                                           adcRemoval.numVertices,
                                           rwID_ADCPLUGIN |
                                           rwMEMHINTDUR_EVENT );
    RWASSERT(NULL != adcData->adcBits);

    /*-- Clear ADC flag memory bloc to 0 --------------*/
    memset(adcData->adcBits, 0, sizeof(RwInt8) * adcRemoval.numVertices);

#if (defined(ADCDEBUG)) /*-- Saving Ratio Information -*/
    {
        RwUInt32 nvtx  = RpWorldSectorGetNumVertices(sector);
        RwUInt32 ntrig = RpWorldSectorGetNumTriangles(sector);

        _rpADCLocalIReminder += adcRemoval.numOrigVertices;
        _rpADCLocalICounter  += adcRemoval.numVertices;
        _rpADCLocalTCounter  += ntrig;
        _rpADCLocalVCounter  += nvtx;

        printf("Num vertices  %d\n", nvtx);
        printf("Num triangles %d\n", ntrig);
        printf("Old to new indices is %d to %d, or %.2f percent\n",
                    adcRemoval.numOrigVertices,
                    adcRemoval.numVertices,
                    ( 100.0f * ( (RwReal)adcRemoval.numOrigVertices /
                                 (RwReal)adcRemoval.numVertices) ) - 100.0f );
        printf("Amount of duplication %.2f\n",
                    ((RwReal)adcRemoval.numVertices / (RwReal)nvtx) );
    }
#endif /* (defined(ADCDEBUG)) -------------------------*/

    /*-- Degenerates removal and AADC bit setup -------*/
    adcRemoval.adcBits = adcData->adcBits;
    RpWorldSectorForAllMeshes( sector,
                               _rpADCRemoveDegeneratesAndInsertADC,
                               &adcRemoval );

    /*-- ADC flag process Terminated ------------------*/
    adcData->adcFormatted = TRUE;
    adcData->numBits = adcRemoval.numVertices;
    RWRETURN(TRUE);
}

/**
 * \ingroup rpadc
 * \ref RpADCGeometryGetADCSupport is used to test if a geometry
 * is formated as an ADC flagged geometry.
 *
 * \param pGeom A pointer to the geometry
 *
 * \return A pointer to the ADC flag array if successfull. NULL otherwise
 *
 * \see RpADCGeometryConvertPreserveWinding
 * \see RpADCGeometryConvertIgnoreWinding
 */
RwInt8 *
RpADCGeometryGetADCSupport(RpGeometry *pGeom)
{
    RwBool bDone = (RPADCGEOMETRYGETEXTDATA(pGeom)->adcFormatted);
    RWAPIFUNCTION(RWSTRING("RpADCGeometryGetADCSupport"));
    RWASSERT(NULL != pGeom);
    if(bDone)
    {
        RWRETURN(RPADCGEOMETRYGETEXTDATA(pGeom)->adcBits);
    }
    else
    {
        RWRETURN(NULL);
    }
}

/**
 * \ingroup rpadc
 * \ref RpADCWorldSectorGetADCSupport is used to test if a sector
 * is formated as an ADC flagged sector.
 *
 * \param sector A pointer to the sector
 *
 * \return A pointer to the ADC flag array if successfull. NULL otherwise
 *
 * \see RpADCWorldSectorConvertPreserveWinding
 * \see RpADCWorldSectorConvertIgnoreWinding
 */
RwInt8 *
RpADCWorldSectorGetADCSupport(RpWorldSector *sector)
{
    RwBool bDone = (RPADCSECTORGETEXTDATA(sector)->adcFormatted);
    RWAPIFUNCTION(RWSTRING("RpADCWorldSectorGetADCSupport"));
    RWASSERT(NULL != sector);
    if(bDone)
    {
        RWRETURN(RPADCSECTORGETEXTDATA(sector)->adcBits);
    }
    else
    {
        RWRETURN(NULL);
    }
}

/**
 * \ingroup rpadc
 * \ref RpADCGetMeshCallback is used to get the callback the
 * plugin calls on each mesh during the ADC converison process.
 *
 * \return A pointer to the per-mesh callback if it exists, NULL otherwise
 *
 * \see RpADCSetMeshCallback
 * \see RpADCGeometryConvertPreserveWinding
 * \see RpADCGeometryConvertIgnoreWinding
 * \see RpADCWorldSectorConvertPreserveWinding
 * \see RpADCWorldSectorConvertIgnoreWinding
 */
RpADCMeshCallback
RpADCGetMeshCallback(void)
{
    RWAPIFUNCTION(RWSTRING("RpADCGetMeshCallback"));
    RWRETURN(_rpADCMeshCallback);
}

/**
 * \ingroup rpadc
 * \ref RpADCSetMeshCallback is used to set the callback the
 * plugin calls on each mesh during the ADC converison process.
 *
 * \param callback A pointer to the callback
 *
 * \see RpADCGetMeshCallback
 * \see RpADCGeometryConvertPreserveWinding
 * \see RpADCGeometryConvertIgnoreWinding
 * \see RpADCWorldSectorConvertPreserveWinding
 * \see RpADCWorldSectorConvertIgnoreWinding
 */
void
RpADCSetMeshCallback(RpADCMeshCallback callback)
{
    RWAPIFUNCTION(RWSTRING("RpADCSetMeshCallback"));
    _rpADCMeshCallback = callback;
    RWRETURNVOID();
}




/* The following functions can be used for mass reconditioning.
 * They will also generation some debugging output data about
 * the quality of reconditioning. */

#if(defined(ADCDEBUG))

/*---------------------------------------------*/
/*-- Clump for all Atomic Convertion CallBack -*/
/*---------------------------------------------*/
static RpAtomic *
_rpADCAtomicProcess(RpAtomic *pAtomic, void *pData)
{
    RwBool bWinding  = (RwBool)pData;
    RpGeometry *geom = NULL;

    RWFUNCTION(RWSTRING("_rpADCAtomicProcess"));
    RWASSERT(NULL != pAtomic);
    RWASSERT(NULL != pData);

    geom = RpAtomicGetGeometry(pAtomic);
    if(bWinding) /*-- Preserve winding order --*/
    {
        RpADCGeometryConvertPreserveWinding(geom);
    }
    else         /*-- Ignore winding order ----*/
    {
        RpADCGeometryConvertIgnoreWinding(geom);
    }
    RWRETURN(pAtomic);
}

/*---------------------------------------------*/
/*-- World for all Sector Convertion CallBack -*/
/*---------------------------------------------*/
static RpWorldSector *
_rpADCWorldSectorProcess(RpWorldSector *sector, void *pData)
{
    RwBool bWinding  = (RwBool)pData;

    RWFUNCTION(RWSTRING("_rpADCWorldSectorProcess"));
    RWASSERT(NULL != sector);
    RWASSERT(NULL != pData);
    if(RpWorldSectorGetNumTriangles(sector) != 0)
    {
        if(bWinding) /*-- Preserve winding order --*/
        {
            RpADCWorldSectorConvertPreserveWinding(sector);
        }
        else         /*-- Ignore winding order ----*/
        {
            RpADCWorldSectorConvertPreserveWinding(sector);
        }
    }
    RWRETURN(sector);
}

/*---------------------------------------------*/
/*--  Clump ADC flag without winding order   --*/
/*---------------------------------------------*/
RwBool
_rpADCClumpConvertIgnoreWinding(RpClump *pClump)
{
    RWFUNCTION(RWSTRING("_rpADCClumpConvertIgnoreWinding"));
    RWASSERT(NULL != pClump);

#if (defined(ADCDEBUG))
    _rpADCLocalIReminder = 0;
    _rpADCLocalICounter  = 0;
    _rpADCLocalTCounter  = 0;
    _rpADCLocalVCounter  = 0;
    _rpADCLocalFCounter  = 0;
#endif /* (defined(ADCDEBUG)) */

    RpClumpForAllAtomics(pClump, _rpADCAtomicProcess, (void*)FALSE);

#if (defined(ADCDEBUG))
    printf("-------------------------------\n");
    printf("-    Clump Global Counters    -\n");
    printf("-------------------------------\n");
    printf("Num triangles %d\n",   _rpADCLocalTCounter);
    printf("Num Vertices %d\n",    _rpADCLocalVCounter);
    printf("Old Num Indices %d\n", _rpADCLocalIReminder);
    printf("New Num Indices %d\n", _rpADCLocalICounter);
    printf("Num ADC Indices %d\n", _rpADCLocalFCounter);
    printf("Removal ratio %.2f percent\n",
                ( 100.0f * ( (RwReal)_rpADCLocalIReminder /
                             (RwReal)_rpADCLocalICounter) ) - 100.0f );
    printf("Amount of duplication %.2f\n", ( (RwReal)_rpADCLocalICounter /
                                             (RwReal)_rpADCLocalVCounter) );
#endif /* (defined(ADCDEBUG)) */

    RWRETURN(TRUE);
}

/*---------------------------------------------*/
/*--  Clump ADC flag with winding order      --*/
/*---------------------------------------------*/
RwBool
_rpADCClumpConvertPreserveWinding(RpClump *pClump)
{
    RWFUNCTION(RWSTRING("_rpADCClumpConvertPreserveWinding"));
    RWASSERT(NULL != pClump);

#if (defined(ADCDEBUG))
    _rpADCLocalIReminder = 0;
    _rpADCLocalICounter  = 0;
    _rpADCLocalTCounter  = 0;
    _rpADCLocalVCounter  = 0;
    _rpADCLocalFCounter  = 0;
#endif /* (defined(ADCDEBUG)) */

    RpClumpForAllAtomics(pClump, _rpADCAtomicProcess, (void*)TRUE);

#if (defined(ADCDEBUG))
    printf("-------------------------------\n");
    printf("-    Clump Global Counters    -\n");
    printf("-------------------------------\n");
    printf("Num triangles %d\n",   _rpADCLocalTCounter);
    printf("Num Vertices %d\n",    _rpADCLocalVCounter);
    printf("Old Num Indices %d\n", _rpADCLocalIReminder);
    printf("New Num Indices %d\n", _rpADCLocalICounter);
    printf("Num ADC Indices %d\n", _rpADCLocalFCounter);
    printf("Removal ratio %.2f percent\n",
                ( 100.0f * ( (RwReal)_rpADCLocalIReminder /
                             (RwReal)_rpADCLocalICounter) ) - 100.0f );
    printf("Amount of duplication %.2f\n", ( (RwReal)_rpADCLocalICounter /
                                             (RwReal)_rpADCLocalVCounter) );
#endif /* (defined(ADCDEBUG)) */

    RWRETURN(TRUE);
}

/*---------------------------------------------*/
/*--  World ADC flag without winding order   --*/
/*---------------------------------------------*/
RwBool
_rpADCWorldConvertIgnoreWinding(RpWorld *pWorld)
{
    RWFUNCTION(RWSTRING("_rpADCWorldConvertIgnoreWinding"));
    RWASSERT(NULL != pWorld);

#if (defined(ADCDEBUG))
    _rpADCLocalIReminder = 0;
    _rpADCLocalICounter  = 0;
    _rpADCLocalTCounter  = 0;
    _rpADCLocalVCounter  = 0;
    _rpADCLocalFCounter  = 0;
#endif /* (defined(ADCDEBUG)) */

    RpWorldForAllWorldSectors(pWorld, _rpADCWorldSectorProcess, (void*)FALSE);

#if (defined(ADCDEBUG))
    printf("-------------------------------\n");
    printf("-    World Global Counters    -\n");
    printf("-------------------------------\n");
    printf("Num triangles %d\n",   _rpADCLocalTCounter);
    printf("Num Vertices %d\n",    _rpADCLocalVCounter);
    printf("Old Num Indices %d\n", _rpADCLocalIReminder);
    printf("New Num Indices %d\n", _rpADCLocalICounter);
    printf("Num ADC Indices %d\n", _rpADCLocalFCounter);
    printf("Removal ratio %.2f percent\n",
                ( 100.0f * ( (RwReal)_rpADCLocalIReminder /
                             (RwReal)_rpADCLocalICounter) ) - 100.0f );
    printf("Amount of duplication %.2f\n", ( (RwReal)_rpADCLocalICounter /
                                             (RwReal)_rpADCLocalVCounter) );
#endif /* (defined(ADCDEBUG)) */

    RWRETURN(TRUE);
}

/*---------------------------------------------*/
/*--  World ADC flag with winding order      --*/
/*---------------------------------------------*/
RwBool
_rpADCWorldConvertPreserveWinding(RpWorld *pWorld)
{
    RWFUNCTION(RWSTRING("_rpADCWorldConvertPreserveWinding"));
    RWASSERT(NULL != pWorld);

#if (defined(ADCDEBUG))
    _rpADCLocalIReminder = 0;
    _rpADCLocalICounter  = 0;
    _rpADCLocalTCounter  = 0;
    _rpADCLocalVCounter  = 0;
    _rpADCLocalFCounter  = 0;
#endif /* (defined(ADCDEBUG)) */

    RpWorldForAllWorldSectors(pWorld, _rpADCWorldSectorProcess, (void*)TRUE);

#if (defined(ADCDEBUG))
    printf("-------------------------------\n");
    printf("-    World Global Counters    -\n");
    printf("-------------------------------\n");
    printf("Num triangles %d\n",   _rpADCLocalTCounter);
    printf("Num Vertices %d\n",    _rpADCLocalVCounter);
    printf("Old Num Indices %d\n", _rpADCLocalIReminder);
    printf("New Num Indices %d\n", _rpADCLocalICounter);
    printf("Num ADC Indices %d\n", _rpADCLocalFCounter);
    printf("Removal ratio %.2f percent\n",
                ( 100.0f * ( (RwReal)_rpADCLocalIReminder /
                             (RwReal)_rpADCLocalICounter) ) - 100.0f );
    printf("Amount of duplication %.2f\n", ( (RwReal)_rpADCLocalICounter /
                                             (RwReal)_rpADCLocalVCounter) );
#endif /* (defined(ADCDEBUG)) */

    RWRETURN(TRUE);
}

#endif /* (defined(ADCDEBUG)) */

/* *INDENT-ON* */
