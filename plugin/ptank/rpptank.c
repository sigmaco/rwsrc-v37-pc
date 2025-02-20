/*
 * Particle Tank
 */

/**
 * \ingroup rpptank
 * \page rpptankoverview RpPTank Plugin Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rpptank.h
 * \li \b Libraries: rwcore, rpworld, rpptank
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpPTankPluginAttach
 *
 * \subsection ptankoverview Overview
 * PTank is a driver level particle manager and particle container.
 * It helps the user define, animate and render particles through a platform
 * independent API and provides platform specific access to the data.
 * A particle is a 2D sprite, based on a set of information for example;
 * position, size, 2D rotation and color.
 *
 *  The user is able to access the particles data through a lock/unlock
 * mechanism. In addition to that system, RpPTank also provides a
 * lock/unlock mechanism for the Platform specific data. This allows the
 * application to directly manipulate data offering the best performance.
 * The user is able, via RpPTank, to define specific particles meeting its
 * exact needs and to animate them using a platform independent abstraction
 * layer.
 *
 * When creating a PTank object, a series of flags defining the particles and
 * the maximum number of particles to display are passed to
 * \ref RpPTankAtomicCreate. These two values are not modifiable after the
 * PTank object has been created.
 *
 * \section dataorganization Data Organization
 *
 * RpPTank supports two internal organizations for its data,
 * "structure of arrays" (Array Organization) or "array of structures"
 * (Structure Organization), depending on the user needs and the targeted
 * platform.
 *
 * \subsection arraysorgnaization Arrays Organization
 *
 * "Arrays organization" means that data is stored in different arrays,
 * using one array per data piece : one array of positions, one array of
 * colors etc.
 *
 * \par Positions Array
 *
 * xyz xyz xyz xyz xyz xyz xyz xyz xyz xyz xyz xyz xyz xyz
 *
 * \par Colors Array
 *
 * rgba rgba rgba rgba rgba rgba rgba rgba rgba rgba rgba rgba rgba rgba
 *
 * The number of arrays depends on the particle definition passed to
 * \ref RpPTankAtomicCreate.
 *
 * \subsection structureorganization Structure Organization
 *
 * "Structures organizations" means that data is stored in one array of
 * structure, per particle. The size of the structures depends on the
 * particle definition passed to \ref RpPTankAtomicCreate.
 *
 * \par Example
 *
 * xyz rgba xyz rgba xyz rgba xyz rgba xyz rgba xyz rgba xyz rgba
 *
 * \subsection specifyingdataorg Specifying the Data Organization
 *
 * The format used can be specified by passing a flag to
 * \ref RpPTankAtomicCreate
 *
 * \li \ref rpPTANKDFLAGARRAY        : Data is organized in a structure of
 * arrays
 * \li \ref rpPTANKDFLAGSTRUCTURE    : Data is organized in an array of
 * structures
 *
 * \subsection accessingdata Accessing the Data
 *
 * To modify the data set, the PTank object will give access to this data
 * via \ref RpPTankAtomicLock, with the required parameter.
 * The PTank object will then return a pointer to the desired array
 * along with the stride needed to access the next structure/data.
 *
 * The number of actually visible/rendered particles varies between 0 and
 * the maximum number specified when creating a PTank object. To change the
 * number of active particles call \ref RpPTankAtomicSetActiveParticlesCount.
 *
 *
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rwcore.h"
#include "rpworld.h"
#include "rpplugin.h"
#include "rpdbgerr.h"
#include "ptank.h"
#include "ptankprv.h"

#include "ptankgen.h"

#include "ptankplatform.h"

#define ALIGNPAD 16


#if (defined(RWDEBUG))
long                rpPTankStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

/*****************************************************************************
 *  Global Variables
 */

RwInt32 _rpPTankAtomicDataOffset = 0; /* Offset in RpAtomic */
RwInt32 _rpPTankGlobalsOffset = 0;     /* Offset in Rwengine */

const RwInt32 datasize[] =
{
    sizeof(RwV3d),/*rpPTANKDFLAGPOSITION*/
    sizeof(RwMatrix),/*rpPTANKDFLAGMATRIX*/
    sizeof(RwV3d),/*rpPTANKDFLAGNORMAL*/
    sizeof(RwV2d),/*rpPTANKDFLAGSIZE*/
    sizeof(RwRGBA),/*rpPTANKDFLAGCOLOR*/
    4*sizeof(RwRGBA),/*rpPTANKDFLAGVTXCOLOR*/
    sizeof(RwReal),/*rpPTANKDFLAG2DROTATE*/
    2*sizeof(RwTexCoords),/*rpPTANKDFLAGVTX2TEXCOORDS*/
    4*sizeof(RwTexCoords)/*rpPTANKDFLAGVTX4TEXCOORDS*/
};




#if (\
       defined(GCN_DRVMODEL_H)\
       )

    #define FAVORITEORGANISATIONMODE  (rpPTANKDFLAGSTRUCTURE)

#elif (\
       defined(D3D8_DRVMODEL_H)    || \
       defined(D3D9_DRVMODEL_H)    || \
       defined(XBOX_DRVMODEL_H)    || \
       defined(NULL_DRVMODEL_H)    || \
       defined(NULLGCN_DRVMODEL_H) || \
       defined(NULLSKY_DRVMODEL_H) || \
       defined(SKY2_DRVMODEL_H)    || \
       defined(OPENGL_DRVMODEL_H)  || \
       defined(SOFTRAS_DRVMODEL_H) || \
       defined(NULLXBOX_DRVMODEL_H) )

    #define FAVORITEORGANISATIONMODE  (rpPTANKDFLAGARRAY)

#endif

/*
 *****************************************************************************
 */
static void        *
PTankAtomicInit(void *object,
                    RwInt32 __RWUNUSED__ offset,
                    RwInt32 __RWUNUSED__ size)
{
    RWFUNCTION(RWSTRING("PTankAtomicInit"));

    /* Initialize reference count */
    RPATOMICPTANKPLUGINDATA(object) = NULL;

    RWRETURN(object);
}


/*
 *****************************************************************************
 */
static void        *
PTankAtomicDestruct(void *object,
                    RwInt32 __RWUNUSED__ offset,
                    RwInt32 __RWUNUSED__ size)
{
    RWFUNCTION(RWSTRING("PTankAtomicDestruct"));

    /* Initialize reference count */
    if( NULL != RPATOMICPTANKPLUGINDATA(object) )
    {
        RpPTankAtomicSetTexture((RpAtomic*)object,(RwTexture*)NULL);

        if( NULL != RPATOMICPTANKPLUGINDATA(object)->rawdata )
        {
#if (defined(SKY2_DRVMODEL_H))
            /*
             * Waiting to be sure the rendering buffer are done with so we can 
             * free up nicely.
             */
            _rpPTankSkyWaitForDispatch((RpAtomic *)object);
            /*
             * Freeing resource arena. (usually only used by matrix based ptank.
             */
            _rpPTankSkyDestroy((RpAtomic *)object);
#endif
            RwFree(RPATOMICPTANKPLUGINDATA(object)->rawdata);
        }

        RwFree(RPATOMICPTANKPLUGINDATA(object));
        RPATOMICPTANKPLUGINDATA(object) = NULL;
    }

    RWRETURN(object);
}


/**
 * \ingroup rpptank
 * \ref RpPTankPluginAttach is used to attach the PTank plugin
 * to the RenderWare system. The PTank plugin must be attached
 * between initializing the system with \ref RwEngineInit and opening
 * it with \ref RwEngineOpen.
 *
 * \return TRUE on success, FALSE otherwise.
 *
 * \see RwEngineInit
 * \see RwEngineOpen
 * \see RwEngineStart
 */
RwBool
RpPTankPluginAttach(void)
{
    RWAPIFUNCTION(RWSTRING("RpPTankPluginAttach"));

    /* Atomic plugin */
    _rpPTankGlobalsOffset =
        RwEngineRegisterPlugin(sizeof(RwInt32),
                               rwID_PTANKPLUGIN,
                               PTankOpen,
                               PTankClose);

    if( _rpPTankGlobalsOffset >= 0 )
    {
        _rpPTankAtomicDataOffset =
            RpAtomicRegisterPlugin(sizeof(RwInt32),
                                   rwID_PTANKPLUGIN,
                                   PTankAtomicInit,
                                   PTankAtomicDestruct,
                                   (RwPluginObjectCopy)NULL);

        RWRETURN( _rpPTankAtomicDataOffset >= 0 );
    }


    RWRETURN(FALSE);
}


/*
 *****************************************************************************
 */
static RwBool
rpPTankAStructAlloc(RpPTankAtomicExtPrv *ptankGlobal,
                    RwUInt32 dataFlags,RwUInt32 platFlags __RWUNUSED__)
{
    RwBool result = FALSE;

    RwBool gotPosition;
    RwBool gotColor;
    RwBool gotSize;
    RwBool gotMatrix;
    RwBool gotNormal;
    RwBool got2DRotate;
    RwBool gotVtxColor;
    RwBool gotVtx2TexCoords;
    RwBool gotVtx4TexCoords;

    RwUInt32 size;
    RwUInt32 tmpSize;
    RwUInt8 *data;
    RwUInt8 *alignedData;

    RWFUNCTION(RWSTRING("rpPTankAStructAlloc"));
    RWASSERT(NULL != ptankGlobal);
    RWASSERT(0 != dataFlags);

    /* One structure, several arrays */

    gotPosition = (dataFlags & rpPTANKDFLAGPOSITION ) == rpPTANKDFLAGPOSITION;
    gotColor = (dataFlags & rpPTANKDFLAGCOLOR ) == rpPTANKDFLAGCOLOR;
    gotSize = (dataFlags & rpPTANKDFLAGSIZE ) == rpPTANKDFLAGSIZE;
    gotMatrix = (dataFlags & rpPTANKDFLAGMATRIX ) == rpPTANKDFLAGMATRIX;
    gotNormal = (dataFlags & rpPTANKDFLAGNORMAL ) == rpPTANKDFLAGNORMAL;
    got2DRotate = (dataFlags & rpPTANKDFLAG2DROTATE ) == rpPTANKDFLAG2DROTATE;
    gotVtxColor = (dataFlags & rpPTANKDFLAGVTXCOLOR ) == rpPTANKDFLAGVTXCOLOR;
    gotVtx2TexCoords = (dataFlags & rpPTANKDFLAGVTX2TEXCOORDS ) ==
                                                     rpPTANKDFLAGVTX2TEXCOORDS;
    gotVtx4TexCoords = (dataFlags & rpPTANKDFLAGVTX4TEXCOORDS ) ==
                                                     rpPTANKDFLAGVTX4TEXCOORDS;

    tmpSize = 0;
    ptankGlobal->publicData.format.dataFlags &= 0xFFFFFE00;
    ptankGlobal->publicData.format.numClusters = 0;
    ptankGlobal->lockFlags = 0;
    /* counting data pieces, setting up PURE dataFlags,
     * calculating global size
     */

    if( TRUE == gotPosition )
    {
        ptankGlobal->publicData.format.numClusters++;
        ptankGlobal->publicData.format.dataFlags |= rpPTANKDFLAGPOSITION;
        tmpSize += datasize[RPPTANKSIZEPOSITION]*ptankGlobal->maxPCount;
    }

    if( TRUE == gotMatrix )
    {
        ptankGlobal->publicData.format.numClusters++;
        ptankGlobal->publicData.format.dataFlags |= rpPTANKDFLAGMATRIX;
        tmpSize += datasize[RPPTANKSIZEMATRIX]*ptankGlobal->maxPCount;
    }

    if( TRUE == gotNormal )
    {
        ptankGlobal->publicData.format.numClusters++;
        ptankGlobal->publicData.format.dataFlags |= rpPTANKDFLAGNORMAL;
        tmpSize += datasize[RPPTANKSIZENORMAL]*ptankGlobal->maxPCount;
    }

    if( TRUE ==  gotSize )
    {
        ptankGlobal->publicData.format.numClusters++;
        ptankGlobal->publicData.format.dataFlags |= rpPTANKDFLAGSIZE;
        tmpSize += datasize[RPPTANKSIZESIZE]*ptankGlobal->maxPCount;
    }

    if( TRUE == gotColor )
    {
        ptankGlobal->publicData.format.numClusters++;
        ptankGlobal->publicData.format.dataFlags |= rpPTANKDFLAGCOLOR;
        tmpSize += datasize[RPPTANKSIZECOLOR]*ptankGlobal->maxPCount;
    }

    if( TRUE == gotVtxColor )
    {
        ptankGlobal->publicData.format.numClusters++;
        ptankGlobal->publicData.format.dataFlags |= rpPTANKDFLAGVTXCOLOR;
        tmpSize += datasize[RPPTANKSIZEVTXCOLOR]*ptankGlobal->maxPCount;
    }

    if( TRUE == got2DRotate )
    {
        ptankGlobal->publicData.format.numClusters++;
        ptankGlobal->publicData.format.dataFlags |= rpPTANKDFLAG2DROTATE;
        tmpSize += datasize[RPPTANKSIZE2DROTATE]*ptankGlobal->maxPCount;
    }

    if( TRUE == gotVtx2TexCoords )
    {
        ptankGlobal->publicData.format.numClusters++;
        ptankGlobal->publicData.format.dataFlags |= rpPTANKDFLAGVTX2TEXCOORDS;
        tmpSize += datasize[RPPTANKSIZEVTX2TEXCOORDS]*ptankGlobal->maxPCount;
    }

    if( TRUE == gotVtx4TexCoords )
    {
        ptankGlobal->publicData.format.numClusters++;
        ptankGlobal->publicData.format.dataFlags |= rpPTANKDFLAGVTX4TEXCOORDS;
        tmpSize += datasize[RPPTANKSIZEVTX4TEXCOORDS]*ptankGlobal->maxPCount;
    }

    size = tmpSize + ALIGNPAD;

    data = (RwUInt8 *)RwMalloc(size,
        rwID_PTANKPLUGIN | rwMEMHINTDUR_EVENT);

    if( NULL != data )
    {
        /* Setting up ptr list */

        alignedData = (RwUInt8*)(((RwUInt32)data
                        + (ALIGNPAD - 1)) & ~(ALIGNPAD - 1));
        tmpSize = 0;

        if( TRUE == gotPosition )
        {
            ptankGlobal->publicData.clusters[RPPTANKSIZEPOSITION].data
                    = alignedData + tmpSize;

            ptankGlobal->publicData.clusters[RPPTANKSIZEPOSITION].stride
                    = sizeof(RwV3d);

            tmpSize += datasize[RPPTANKSIZEPOSITION]*ptankGlobal->maxPCount;
        }

        if( TRUE == gotMatrix )
        {
            ptankGlobal->publicData.clusters[RPPTANKSIZEMATRIX].data
                    = alignedData + tmpSize;
            ptankGlobal->publicData.clusters[RPPTANKSIZEMATRIX].stride
                    = sizeof(RwMatrix);

            tmpSize += datasize[RPPTANKSIZEMATRIX]*ptankGlobal->maxPCount;
        }

        if( TRUE == gotNormal )
        {
            ptankGlobal->publicData.clusters[RPPTANKSIZENORMAL].data
                    = alignedData + tmpSize;

            ptankGlobal->publicData.clusters[RPPTANKSIZENORMAL].stride
                    = sizeof(RwV3d);

            tmpSize += datasize[RPPTANKSIZENORMAL]*ptankGlobal->maxPCount;
        }

        if( TRUE ==  gotSize )
        {
            ptankGlobal->publicData.clusters[RPPTANKSIZESIZE].data
                    = alignedData + tmpSize;

            ptankGlobal->publicData.clusters[RPPTANKSIZESIZE].stride
                    = sizeof(RwV2d);

            tmpSize += datasize[RPPTANKSIZESIZE]*ptankGlobal->maxPCount;
        }

        if( TRUE == gotColor )
        {
            ptankGlobal->publicData.clusters[RPPTANKSIZECOLOR].data
                    = alignedData + tmpSize;

            ptankGlobal->publicData.clusters[RPPTANKSIZECOLOR].stride
                    = sizeof(RwRGBA);

            tmpSize += datasize[RPPTANKSIZECOLOR]*ptankGlobal->maxPCount;
        }

        if( TRUE == gotVtxColor )
        {
            ptankGlobal->publicData.clusters[RPPTANKSIZEVTXCOLOR].data
                    = alignedData + tmpSize;

            ptankGlobal->publicData.clusters[RPPTANKSIZEVTXCOLOR].stride
                    = 4*sizeof(RwRGBA);

            tmpSize += datasize[RPPTANKSIZEVTXCOLOR]*ptankGlobal->maxPCount;
        }

        if( TRUE == got2DRotate )
        {
            ptankGlobal->publicData.clusters[RPPTANKSIZE2DROTATE].data
                    = alignedData + tmpSize;

            ptankGlobal->publicData.clusters[RPPTANKSIZE2DROTATE].stride
                    = sizeof(RwReal);

            tmpSize += datasize[RPPTANKSIZE2DROTATE]*ptankGlobal->maxPCount;
        }

        if( TRUE == gotVtx2TexCoords )
        {
            ptankGlobal->publicData.clusters[RPPTANKSIZEVTX2TEXCOORDS].data
                    = alignedData + tmpSize;

            ptankGlobal->publicData.clusters[RPPTANKSIZEVTX2TEXCOORDS].stride
                    = 2*sizeof(RwTexCoords);

            tmpSize+=datasize[RPPTANKSIZEVTX2TEXCOORDS]*ptankGlobal->maxPCount;
        }

        if( TRUE == gotVtx4TexCoords )
        {
            ptankGlobal->publicData.clusters[RPPTANKSIZEVTX4TEXCOORDS].data
                    = alignedData + tmpSize;

            ptankGlobal->publicData.clusters[RPPTANKSIZEVTX4TEXCOORDS].stride
                    = 4*sizeof(RwTexCoords);

            tmpSize+=datasize[RPPTANKSIZEVTX4TEXCOORDS]*ptankGlobal->maxPCount;
        }

        /*AJH:
         * Dodgy pointer size assertion :((
         */

        ptankGlobal->publicData.format.stride = 0;
        ptankGlobal->rawdata = data;

        result = TRUE;
    }

    RWRETURN(result);
}


/*
 *****************************************************************************
 */
static RwBool
rpPTankSStructAlloc(RpPTankAtomicExtPrv *ptankGlobal,
                    RwUInt32 dataFlags,RwUInt32 platFlags __RWUNUSED__)
{
    RwBool result = FALSE;

    RwBool gotPosition;
    RwBool gotColor;
    RwBool gotSize;
    RwBool gotMatrix;
    RwBool gotNormal;
    RwBool got2DRotate;
    RwBool gotVtxColor;
    RwBool gotVtx2TexCoords;
    RwBool gotVtx4TexCoords;

    RwUInt32 size;
    RwUInt32 tmpSize;
    RwUInt8 *data;
    RwUInt8 *alignedData;

    RWFUNCTION(RWSTRING("rpPTankSStructAlloc"));
    RWASSERT(NULL != ptankGlobal);
    RWASSERT(0 != dataFlags);

    /* One array, several structures */


    gotPosition = (dataFlags & rpPTANKDFLAGPOSITION ) == rpPTANKDFLAGPOSITION;
    gotColor = (dataFlags & rpPTANKDFLAGCOLOR ) == rpPTANKDFLAGCOLOR;
    gotSize = (dataFlags & rpPTANKDFLAGSIZE ) == rpPTANKDFLAGSIZE;
    gotMatrix = (dataFlags & rpPTANKDFLAGMATRIX ) == rpPTANKDFLAGMATRIX;
    gotNormal = (dataFlags & rpPTANKDFLAGNORMAL ) == rpPTANKDFLAGNORMAL;
    got2DRotate = (dataFlags & rpPTANKDFLAG2DROTATE ) == rpPTANKDFLAG2DROTATE;
    gotVtxColor = (dataFlags & rpPTANKDFLAGVTXCOLOR ) == rpPTANKDFLAGVTXCOLOR;
    gotVtx2TexCoords = (dataFlags & rpPTANKDFLAGVTX2TEXCOORDS ) ==
                                                     rpPTANKDFLAGVTX2TEXCOORDS;
    gotVtx4TexCoords = (dataFlags & rpPTANKDFLAGVTX4TEXCOORDS ) ==
                                                     rpPTANKDFLAGVTX4TEXCOORDS;

    tmpSize = 0;
    ptankGlobal->publicData.format.dataFlags &= 0xFFFFFE00;
    ptankGlobal->publicData.format.numClusters = 0;

    /* counting data pieces, setting up PURE dataFlags,
     * calculating global size
     */

    if( TRUE == gotPosition )
    {
        ptankGlobal->publicData.format.numClusters++;
        ptankGlobal->publicData.format.dataFlags |= rpPTANKDFLAGPOSITION;
        tmpSize += datasize[RPPTANKSIZEPOSITION];
    }

    if( TRUE == gotMatrix )
    {
        ptankGlobal->publicData.format.numClusters++;
        ptankGlobal->publicData.format.dataFlags |= rpPTANKDFLAGMATRIX;
        tmpSize += datasize[RPPTANKSIZEMATRIX];
    }

    if( TRUE == gotNormal )
    {
        ptankGlobal->publicData.format.numClusters++;
        ptankGlobal->publicData.format.dataFlags |= rpPTANKDFLAGNORMAL;
        tmpSize += datasize[RPPTANKSIZENORMAL];
    }

    if( TRUE ==  gotSize )
    {
        ptankGlobal->publicData.format.numClusters++;
        ptankGlobal->publicData.format.dataFlags |= rpPTANKDFLAGSIZE;
        tmpSize += datasize[RPPTANKSIZESIZE];
    }

    if( TRUE == gotColor )
    {
        ptankGlobal->publicData.format.numClusters++;
        ptankGlobal->publicData.format.dataFlags |= rpPTANKDFLAGCOLOR;
        tmpSize += datasize[RPPTANKSIZECOLOR];
    }

    if( TRUE == gotVtxColor )
    {
        ptankGlobal->publicData.format.numClusters++;
        ptankGlobal->publicData.format.dataFlags |= rpPTANKDFLAGVTXCOLOR;
        tmpSize += datasize[RPPTANKSIZEVTXCOLOR];
    }

    if( TRUE == got2DRotate )
    {
        ptankGlobal->publicData.format.numClusters++;
        ptankGlobal->publicData.format.dataFlags |= rpPTANKDFLAG2DROTATE;
        tmpSize += datasize[RPPTANKSIZE2DROTATE];
    }

    if( TRUE == gotVtx2TexCoords )
    {
        ptankGlobal->publicData.format.numClusters++;
        ptankGlobal->publicData.format.dataFlags |= rpPTANKDFLAGVTX2TEXCOORDS;
        tmpSize += datasize[RPPTANKSIZEVTX2TEXCOORDS];
    }

    if( TRUE == gotVtx4TexCoords )
    {
        ptankGlobal->publicData.format.numClusters++;
        ptankGlobal->publicData.format.dataFlags |= rpPTANKDFLAGVTX4TEXCOORDS;
        tmpSize += datasize[RPPTANKSIZEVTX4TEXCOORDS];
    }

    /* if got matrix, each structure need to be matrix aligned */
    if ( gotMatrix )
    {
        tmpSize = (tmpSize + (rwMATRIXALIGNMENT - 1))
                        & ~(rwMATRIXALIGNMENT - 1);
    }

    ptankGlobal->publicData.format.stride = tmpSize;

    size = (tmpSize*ptankGlobal->maxPCount) + ALIGNPAD;

    data = (RwUInt8 *)RwMalloc(size, rwMEMHINTDUR_EVENT);

    if( NULL != data )
    {
        /* Setting up ptr list */
        alignedData = (RwUInt8*)(((RwUInt32)data
                        + (ALIGNPAD - 1)) & ~(ALIGNPAD - 1));
        tmpSize = 0;

        if( TRUE == gotPosition )
        {
            ptankGlobal->publicData.clusters[RPPTANKSIZEPOSITION].data
                    = alignedData + tmpSize;

            ptankGlobal->publicData.clusters[RPPTANKSIZEPOSITION].stride
                    = ptankGlobal->publicData.format.stride;

            tmpSize += datasize[RPPTANKSIZEPOSITION];
        }

        if( TRUE == gotMatrix )
        {
            ptankGlobal->publicData.clusters[RPPTANKSIZEMATRIX].data
                    = alignedData + tmpSize;
            ptankGlobal->publicData.clusters[RPPTANKSIZEMATRIX].stride
                    = ptankGlobal->publicData.format.stride;

            tmpSize += datasize[RPPTANKSIZEMATRIX];
        }

        if( TRUE == gotNormal )
        {
            ptankGlobal->publicData.clusters[RPPTANKSIZENORMAL].data
                    = alignedData + tmpSize;

            ptankGlobal->publicData.clusters[RPPTANKSIZENORMAL].stride
                    = ptankGlobal->publicData.format.stride;

            tmpSize += datasize[RPPTANKSIZENORMAL];
        }

        if( TRUE ==  gotSize )
        {
            ptankGlobal->publicData.clusters[RPPTANKSIZESIZE].data
                    = alignedData + tmpSize;

            ptankGlobal->publicData.clusters[RPPTANKSIZESIZE].stride
                    = ptankGlobal->publicData.format.stride;

            tmpSize += datasize[RPPTANKSIZESIZE];
        }

        if( TRUE == gotColor )
        {
            ptankGlobal->publicData.clusters[RPPTANKSIZECOLOR].data
                    = alignedData + tmpSize;

            ptankGlobal->publicData.clusters[RPPTANKSIZECOLOR].stride
                    = ptankGlobal->publicData.format.stride;

            tmpSize += datasize[RPPTANKSIZECOLOR];
        }

        if( TRUE == gotVtxColor )
        {
            ptankGlobal->publicData.clusters[RPPTANKSIZEVTXCOLOR].data
                    = alignedData + tmpSize;

            ptankGlobal->publicData.clusters[RPPTANKSIZEVTXCOLOR].stride
                    = ptankGlobal->publicData.format.stride;

            tmpSize += datasize[RPPTANKSIZEVTXCOLOR];
        }

        if( TRUE == got2DRotate )
        {
            ptankGlobal->publicData.clusters[RPPTANKSIZE2DROTATE].data
                    = alignedData + tmpSize;

            ptankGlobal->publicData.clusters[RPPTANKSIZE2DROTATE].stride
                    = ptankGlobal->publicData.format.stride;

            tmpSize += datasize[RPPTANKSIZE2DROTATE];
        }

        if( TRUE == gotVtx2TexCoords )
        {
            ptankGlobal->publicData.clusters[RPPTANKSIZEVTX2TEXCOORDS].data
                    = alignedData + tmpSize;

            ptankGlobal->publicData.clusters[RPPTANKSIZEVTX2TEXCOORDS].stride
                    = ptankGlobal->publicData.format.stride;

            tmpSize+=datasize[RPPTANKSIZEVTX2TEXCOORDS];
        }

        if( TRUE == gotVtx4TexCoords )
        {
            ptankGlobal->publicData.clusters[RPPTANKSIZEVTX4TEXCOORDS].data
                    = alignedData + tmpSize;

            ptankGlobal->publicData.clusters[RPPTANKSIZEVTX4TEXCOORDS].stride
                    = ptankGlobal->publicData.format.stride;

            tmpSize+=datasize[RPPTANKSIZEVTX4TEXCOORDS];
        }

        ptankGlobal->rawdata = data;

        result = TRUE;
    }

    RWRETURN(result);
}

/*
 *****************************************************************************
 */
static RwUInt32
rpPTankValidateFlag(RwUInt32 dataFlags)
{
    RwUInt32 result = 0;
    RwBool gotPosition;
    RwBool gotColor;
    RwBool gotSize;
    RwBool gotMatrix;
    RwBool gotNormal;
    RwBool got2DRotate;
    RwBool gotVtxColor;
    RwBool gotVtx2TexCoords;
    RwBool gotVtx4TexCoords;

    RwBool gotCMatrix;
    RwBool gotC2DRotate;
    RwBool gotCVtxColor;
    RwBool gotCVtx2TexCoords;
    RwBool gotCVtx4TexCoords;

    RWFUNCTION(RWSTRING("rpPTankValidateFlag"));

    gotPosition = (dataFlags & rpPTANKDFLAGPOSITION ) == rpPTANKDFLAGPOSITION;
    gotColor = (dataFlags & rpPTANKDFLAGCOLOR ) == rpPTANKDFLAGCOLOR;
    gotSize = (dataFlags & rpPTANKDFLAGSIZE ) == rpPTANKDFLAGSIZE;
    gotMatrix = (dataFlags & rpPTANKDFLAGMATRIX ) == rpPTANKDFLAGMATRIX;
    gotNormal = (dataFlags & rpPTANKDFLAGNORMAL ) == rpPTANKDFLAGNORMAL;
    got2DRotate = (dataFlags & rpPTANKDFLAG2DROTATE ) == rpPTANKDFLAG2DROTATE;
    gotVtxColor = (dataFlags & rpPTANKDFLAGVTXCOLOR ) == rpPTANKDFLAGVTXCOLOR;

    gotVtx2TexCoords =
        (dataFlags & rpPTANKDFLAGVTX2TEXCOORDS ) == rpPTANKDFLAGVTX2TEXCOORDS;

    gotVtx4TexCoords =
        (dataFlags & rpPTANKDFLAGVTX4TEXCOORDS ) == rpPTANKDFLAGVTX4TEXCOORDS;

    gotCMatrix =
            (dataFlags & rpPTANKDFLAGCNSMATRIX ) == rpPTANKDFLAGCNSMATRIX;

    gotC2DRotate =
            (dataFlags & rpPTANKDFLAGCNS2DROTATE ) == rpPTANKDFLAGCNS2DROTATE;

    gotCVtxColor =
            (dataFlags & rpPTANKDFLAGCNSVTXCOLOR ) == rpPTANKDFLAGCNSVTXCOLOR;

    gotCVtx2TexCoords = (dataFlags & rpPTANKDFLAGCNSVTX2TEXCOORDS ) ==
                                                  rpPTANKDFLAGCNSVTX2TEXCOORDS;

    gotCVtx4TexCoords = (dataFlags & rpPTANKDFLAGCNSVTX4TEXCOORDS ) ==
                                                  rpPTANKDFLAGCNSVTX4TEXCOORDS;

    if(gotMatrix == TRUE && TRUE == gotCMatrix)
    {
        RWMESSAGE(MATCONSCONFLICT);
        dataFlags &= rpPTANKDFLAGCNSMATRIX;
    }

    if(got2DRotate == TRUE && TRUE == gotC2DRotate)
    {
        RWMESSAGE(ROTCONSCONFLICT);
        dataFlags &= rpPTANKDFLAGCNS2DROTATE;
    }

    if(gotVtxColor == TRUE && TRUE == gotCVtxColor)
    {
        RWMESSAGE(VCOCONSCONFLICT);
        dataFlags &= rpPTANKDFLAGCNSVTXCOLOR;

    }

    if(gotVtx2TexCoords == TRUE && TRUE == gotCVtx2TexCoords)
    {
        RWMESSAGE(V2XCONSCONFLICT);
        dataFlags &= rpPTANKDFLAGCNSVTX2TEXCOORDS;

    }

    if(gotVtx4TexCoords == TRUE && TRUE == gotCVtx4TexCoords)
    {
        RWMESSAGE(V4XCONSCONFLICT);
        dataFlags &= rpPTANKDFLAGCNSVTX4TEXCOORDS;
    }

    /* Checking position flag :
        * if a system is created without position or matrix then we
        * need to create/one, same for size and 2d rotation
        */
    if( TRUE == gotMatrix && TRUE == gotPosition )
    {
        RWMESSAGE(POSANDMATMSG);
        dataFlags &= ~rpPTANKDFLAGPOSITION;

    }

    if( TRUE == gotMatrix && TRUE == gotSize )
    {
        RWMESSAGE(SIZANDMATMSG);
        dataFlags &= ~rpPTANKDFLAGSIZE;

    }

    if( TRUE == gotMatrix && TRUE == got2DRotate )
    {
        RWMESSAGE(ROTANDMATMSG);
        dataFlags &= ~rpPTANKDFLAG2DROTATE;

    }

    /* Checking constant flag :
        * if a system is created without position or matrix then we
        * need to create/one, same for size and 2d rotation
        */
    if( FALSE == gotMatrix && FALSE == gotPosition )
    {
        RWMESSAGE(NOPOSMSG);
        dataFlags |= rpPTANKDFLAGPOSITION;
    }

    if ( 0 == (dataFlags & ( rpPTANKDFLAGSTRUCTURE | rpPTANKDFLAGARRAY)) )
    {
        dataFlags |= FAVORITEORGANISATIONMODE;
    }

    result = dataFlags;

    RWRETURN(result);
}


/*
 *****************************************************************************
 */


static RpAtomic *
rpPTankAtomicRenderCB(RpAtomic *atomic)
{
    RwBool callDefault = TRUE;
    RpPTankAtomicExtPrv *ptankGlobal;

    RWFUNCTION(RWSTRING("rpPTankAtomicRenderCB"));
    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

    if( 0 < ptankGlobal->actPCount )
    {
        if( ptankGlobal->ptankCallBacks.instance )
        {
            ptankGlobal->ptankCallBacks.instance(atomic, &ptankGlobal->publicData,
                            ptankGlobal->actPCount,
                            ptankGlobal->instFlags);


            /*AJH:
            * Might be moved to the CB ?
            */
            ptankGlobal->instFlags = 0;
        }


        if(ptankGlobal->ptankCallBacks.render)
        {
            callDefault    = ptankGlobal->ptankCallBacks.render(
                                            atomic, &ptankGlobal->publicData,
                                            ptankGlobal->actPCount);
        }

        if( TRUE == callDefault )
        {
            ptankGlobal->defaultRenderCB(atomic);
        }

    }

    RWRETURN(atomic);
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicCreate
 * is used to create a PTank object. A PTank object is an extended atomic.
 * To destroy a PTank object call \ref RpPTankAtomicDestroy.
 *
 * \param  maxParticleNum   Maximum number of objects, or particles
 * held by the PTank object.
 *
 * \param  dataFlags  A set of flags describing the particles held by the
 * PTank object. The \ref RpPTankDataFlags are the values accepted by this
 * function.
 *
 * \param  platFlags  A set of flags used to enable platform specific functions
 *  and properties. See the platform specific section of this API Reference for
 *  more information.
 *
 * \return A pointer to the created PTank object ( in fact a pointer to
 * a \ref RpAtomic ) on succes,  NULL otherwise.
 *
 * \see RpAtomicIsPTank
 * \see RpPTankAtomicDestroy
 */
RpAtomic*
RpPTankAtomicCreate(RwInt32 maxParticleNum,
                RwUInt32 dataFlags, RwUInt32 platFlags)
{
    RpAtomic *atomic;
    RWAPIFUNCTION(RWSTRING("RpPTankAtomicCreate"));
    RWASSERT(maxParticleNum>0);

    atomic = _rpPTankAtomicCreateCustom(maxParticleNum,
                rpPTankValidateFlag(dataFlags), platFlags,
                &defaultCB);

    RWRETURN(atomic);
}

#if (!defined(DOXYGEN))

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicCreateCustom
 * is used to create a Custom PTank object.
 *
 * \param  maxParticleNum   Maximum number of objects, or particles
 * held by the PTank object.
 *
 * \param  dataFlags  A set of flags describing the particles held by the
 * PTank object. The \ref RpPTankDataFlags are the values accepted by this
 * function.
 *
 * \param  platFlags  A set of flags used to enable platform specific functions
 *  and properties. See the platform specific section of this API Reference for
 *  more information.
 *
 * \param callbacks  A structure containing the custom callbacks
 *
 * \return A pointer to the created PTank object ( in fact a pointer to
 * a \ref RpAtomic ) on succes,  NULL otherwise.
 *
 * \see RpAtomicIsPTank
 * \see RpPTankAtomicDestroy
 * \see RpPTankAtomicCreate
 */
RpAtomic*
_rpPTankAtomicCreateCustom(RwInt32 maxParticleNum,
                RwUInt32 dataFlags, RwUInt32 platFlags,
                RpPTankCallBacks *callbacks)
{
    RwBool aResult = FALSE;
    RpAtomic *atomic;
    RWFUNCTION(RWSTRING("_rpPTankAtomicCreateCustom"));
    RWASSERT(maxParticleNum>0);
    RWASSERT(callbacks);

    atomic = RpAtomicCreate();

    if(atomic)
    {
        RpPTankAtomicExtPrv *ptankGlobal = (RpPTankAtomicExtPrv *)
                                         RwMalloc(sizeof(RpPTankAtomicExtPrv),
                                           rwID_PTANKPLUGIN | rwMEMHINTDUR_EVENT);

        if( ptankGlobal )
        {

            ptankGlobal->defaultRenderCB = RpAtomicGetRenderCallBack(atomic);
            RpAtomicSetRenderCallBack(atomic, rpPTankAtomicRenderCB);
            ptankGlobal->maxPCount = maxParticleNum;
            ptankGlobal->actPCount = 0;

            ptankGlobal->publicData.format.dataFlags = dataFlags;

            ptankGlobal->platFlags = platFlags;
            ptankGlobal->instFlags = rpPTANKIFLAGALL;
            ptankGlobal->lockFlags = 0;

            ptankGlobal->rawdata = NULL;

            RPATOMICPTANKPLUGINDATA(atomic) = ptankGlobal;

            memcpy(&ptankGlobal->ptankCallBacks,
                                callbacks,
                                sizeof(RpPTankCallBacks));

            if( ptankGlobal->ptankCallBacks.alloc )
            {
                ptankGlobal->rawdata =
                    ptankGlobal->ptankCallBacks.alloc(&ptankGlobal->publicData,
                                                      ptankGlobal->maxPCount,
                                                      dataFlags, platFlags);
            }
            else
            {
                if( rpPTANKDFLAGSTRUCTURE ==
                                          (dataFlags & rpPTANKDFLAGSTRUCTURE) )
                {
                    aResult = rpPTankSStructAlloc(ptankGlobal,
                                                  dataFlags,platFlags);
                }
                else
                {
                    aResult = rpPTankAStructAlloc(ptankGlobal,
                                                  dataFlags,platFlags);
                }
            }

            if( (aResult == TRUE) || (NULL != ptankGlobal->rawdata) )
            {
                if( TRUE ==
                            ptankGlobal->ptankCallBacks.create(
                                        atomic, &ptankGlobal->publicData,
                                        ptankGlobal->maxPCount, dataFlags,
                                        platFlags))

                {
                    RwV2d center = { 0.0f, 0.0f };
                    RwV2d size = { 1.0f, 1.0f };
                    RwRGBA color = {0, 0, 0, 255};
                    RwTexCoords uv[4] = {{0.0f,0.0f},
                                    {0.0f,1.0f},
                                    {1.0f,1.0f},
                                    {1.0f,0.0f}};

                    RpPTankAtomicSetTexture(atomic, (RwTexture*)NULL);

                    RwV2dAssign(&(ptankGlobal->publicData.cSize), &size);

                    ptankGlobal->publicData.cRotate = 0.0f;

                    RwV2dAssign(&ptankGlobal->publicData.cCenter,&center);

                    RwMatrixSetIdentity(&ptankGlobal->publicData.cMatrix);

                    ptankGlobal->publicData.cColor = color;
                    ptankGlobal->publicData.cVtxColor[0] = color;
                    ptankGlobal->publicData.cVtxColor[1] = color;
                    ptankGlobal->publicData.cVtxColor[2] = color;
                    ptankGlobal->publicData.cVtxColor[3] = color;

                    ptankGlobal->publicData.cUV[0] = uv[0];
                    ptankGlobal->publicData.cUV[1] = uv[1];
                    ptankGlobal->publicData.cUV[2] = uv[2];
                    ptankGlobal->publicData.cUV[3] = uv[3];

                    RwRenderStateGet(rwRENDERSTATESRCBLEND,
                            (void *)&ptankGlobal->publicData.srcBlend);

                    RwRenderStateGet(rwRENDERSTATEDESTBLEND,
                            (void *)&ptankGlobal->publicData.dstBlend);

                    RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE,
                            (void *)&ptankGlobal->publicData.vertexAlphaBlend);

                    RWRETURN(atomic);
                }
            }
        }
    }

    RpAtomicDestroy(atomic);

    RWRETURN(NULL);
}

#endif

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicDestroy
 * is used to destroy a PTank object previously created by calling
 * \ref RpPTankAtomicCreate.
 *
 * \param  ptank   a pointer to the ptank object to be destroyed
 *
 * \see RpAtomicIsPTank
 * \see RpPTankAtomicCreate
 */
void
RpPTankAtomicDestroy(RpAtomic *ptank)
{
    RWAPIFUNCTION(RWSTRING("RpPTankAtomicDestroy"));
    RWASSERT(ptank);
    RWASSERT(RpAtomicIsPTank(ptank));

    RpAtomicDestroy(ptank);

    RWRETURNVOID();
}

/**
 * \ingroup rpptank
 * \ref RpAtomicIsPTank
 * is called to check if an atomic is a PTank object.
 *
 * \param atomic  A pointer to the atomic to be tested.
 *
 * \return TRUE if the atomic is a PTank object, FALSE otherwise.
 *
 * \see RpPTankAtomicCreate
 */
RwBool
RpAtomicIsPTank(RpAtomic *atomic)
{
    RwBool result = FALSE;
    RWAPIFUNCTION(RWSTRING("RpAtomicIsPTank"));

    result = ((RpAtomic *) NULL != atomic);

    if (result)
    {
        result = (NULL != RPATOMICPTANKPLUGINDATA(atomic));
    }

    RWRETURN(result);
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicGetDataFormat
 * is called to get the format descriptor of a PTank object.
 *
 * \param atomic  A pointer to the PTank object to access.
 *
 * \return A pointer to the \ref RpPTankFormatDescriptor structure describing
 * the PTank data organization.
 *
 * \see RpPTankAtomicLock
 * \see RpPTankAtomicUnlock
 * \see RpPTankAtomicCreate
 */
const RpPTankFormatDescriptor *
RpPTankAtomicGetDataFormat(RpAtomic *atomic)
{
    RpPTankAtomicExtPrv *ptankGlobal = NULL;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicGetDataFormat"));
    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

    RWRETURN(&ptankGlobal->publicData.format);
}

#if ((defined(RWDEBUG)) || (defined(RWSUPPRESSINLINE)))

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicGetActiveParticlesCount
 * is called to get the number of active particles in the PTank object.
 * These particles are rendered in most instances.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 *
 * \return The number of active particles.
 *
 * \see RpPTankAtomicSetActiveParticlesCount
 * \see RpPTankAtomicGetMaximumParticlesCount
 */
RwInt32
RpPTankAtomicGetActiveParticlesCount(RpAtomic *atomic)
{
    RWAPIFUNCTION(RWSTRING("RpPTankAtomicGetActiveParticlesCount"));
    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    RWRETURN( RPATOMICPTANKPLUGINDATA(atomic)->actPCount );
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicGetMaximumParticlesCount
 * is called to get the maximum number of particles held by the PTank object.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 *
 * \return The maximum number of particles held by the PTank object.
 *
 * \see RpPTankAtomicGetActiveParticlesCount
 * \see RpPTankAtomicSetActiveParticlesCount
 */
RwInt32
RpPTankAtomicGetMaximumParticlesCount(RpAtomic *atomic)
{
    RWAPIFUNCTION(RWSTRING("RpPTankAtomicGetMaximumParticlesCount"));
    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    RWRETURN( RPATOMICPTANKPLUGINDATA(atomic)->maxPCount );
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicSetActiveParticlesCount
 * is called to set the number of active particles in a PTank object.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 * \param count  The new number of active particles to set.
 * \return None.
 *
 * \see RpPTankAtomicGetActiveParticlesCount
 * \see RpPTankAtomicGetMaximumParticlesCount
 */
void
RpPTankAtomicSetActiveParticlesCount(RpAtomic *atomic, RwInt32 count)
{
    RpPTankAtomicExtPrv *ptankGlobal = NULL;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicSetActiveParticlesCount"));
    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);
    RWASSERT(count <= ptankGlobal->maxPCount );

    ptankGlobal->instFlags |= rpPTANKIFLAGACTNUMCHG;
    ptankGlobal->actPCount = count;

    RWRETURNVOID();
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicSetTexture
 * is called to set the texture used by a PTank object
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 * \param texture A pointer to the \ref RwTexture to use.
 *
 * \return None.
 */
void
RpPTankAtomicSetTexture(RpAtomic *atomic, RwTexture *texture)
{
    RpGeometry *geom;
    RpMaterial *material;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicSetTexture"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    /* Ensure we have a set of texcoords for a non-null texture */
    if (texture != NULL)
    {
        RWASSERT((RPATOMICPTANKPLUGINDATA(atomic)->publicData.format.dataFlags
                & (rpPTANKDFLAGVTX2TEXCOORDS | rpPTANKDFLAGVTX4TEXCOORDS |
                    rpPTANKDFLAGCNSVTX2TEXCOORDS | rpPTANKDFLAGCNSVTX4TEXCOORDS))
                    != 0 );
    }

    geom = RpAtomicGetGeometry(atomic);
    material = RpGeometryGetMaterial(geom,0);

    RpMaterialSetTexture(material, texture);

    RWRETURNVOID();
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicGetTexture
 * is called to get the texture used by a PTank object
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 *
 * \return A Pointer to the current texture, NULL if none is selected.
 */
RwTexture *
RpPTankAtomicGetTexture(RpAtomic *atomic)
{
    RpGeometry *geom;
    RpMaterial *material;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicGetTexture"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    geom = RpAtomicGetGeometry(atomic);
    material = RpGeometryGetMaterial(geom,0);

    RWRETURN(RpMaterialGetTexture(material));
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicGetMaterial
 * is called to get the Material used by a PTank object
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 *
 * \return A Pointer to the current Material on success, NULL otherwise.
 */
RpMaterial *
RpPTankAtomicGetMaterial(RpAtomic *atomic)
{
    RpGeometry *geom;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicGetMaterial"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    geom = RpAtomicGetGeometry(atomic);

    RWRETURN(RpGeometryGetMaterial(geom,0));
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicSetBlendModes
 * is called to set the blending functions used by a PTank object
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 * \param srcBlendMode A \ref RwBlendFunction used to set the
 *                     source blend mode.
 * \param dstBlendMode A  \ref RwBlendFunction used to set the
 *                     destination blend mode.
 *
 * \return None.
 */
void
RpPTankAtomicSetBlendModes(RpAtomic *atomic,
                                 RwBlendFunction srcBlendMode,
                                 RwBlendFunction dstBlendMode )
{
    RpPTankAtomicExtPrv *ptankGlobal = NULL;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicSetBlendModes"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

    ptankGlobal->publicData.srcBlend = srcBlendMode;
    ptankGlobal->publicData.dstBlend = dstBlendMode;

    ptankGlobal->instFlags |= rpPTANKIFLAGALPHABLENDING;

    RWRETURNVOID();
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicGetBlendModes
 * is called to get the blending functions used by a PTank object
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 * \param srcBlendMode A pointer to a \ref RwBlendFunction used to set the
 *                     source blend mode.
 * \param dstBlendMode A pointer to a \ref RwBlendFunction used to set the
 *                     destination blend mode.
 *
 * \return None.
 */
void
RpPTankAtomicGetBlendModes(RpAtomic *atomic,
                                 RwBlendFunction *srcBlendMode,
                                 RwBlendFunction *dstBlendMode )
{
    RpPTankAtomicExtPrv *ptankGlobal;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicGetBlendModes"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));
    RWASSERT(NULL != srcBlendMode);
    RWASSERT(NULL != dstBlendMode);

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

    *srcBlendMode = (RwBlendFunction)(ptankGlobal->publicData.srcBlend);
    *dstBlendMode = (RwBlendFunction)(ptankGlobal->publicData.dstBlend);

    RWRETURNVOID();
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicSetVertexAlpha
 * is called to set the state of the vertex alpha transparency for this object
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 * \param vtxAlphaState A \ref RwBool used to set the
 *                     the vertex alpha transparency.
 *
 * \return None.
 */
void
RpPTankAtomicSetVertexAlpha(RpAtomic *atomic, RwBool vtxAlphaState)
{
    RpPTankAtomicExtPrv *ptankGlobal = NULL;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicSetVertexAlpha"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

    ptankGlobal->publicData.vertexAlphaBlend = vtxAlphaState;

    RWRETURNVOID();
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicGetVertexAlpha
 * is called to get the state of the vertex alpha transparency for this object
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
  *
 * \return A \ref RwBool used to set the the vertex alpha transparency.
 */
RwBool
RpPTankAtomicGetVertexAlpha(RpAtomic *atomic)
{
    RpPTankAtomicExtPrv *ptankGlobal;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicGetVertexAlpha"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

    RWRETURN(ptankGlobal->publicData.vertexAlphaBlend);
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicSetConstantCenter
 * is used to set the center of the particles. if the center is (0.0f, 0.0f),
 * then the particle center will match it's position.
 * By default the center is (0.0f, 0.0f).
 *
 * To use this feature the \ref rpPTANKDFLAGUSECENTER flag should be passed
 * to \ref RpPTankAtomicCreate.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 * \param center A pointer to the new value.
 *
 * \return None.
 *
 * \see RpPTankAtomicCreate
 * \see RpPTankAtomicGetConstantCenter
 */
void
RpPTankAtomicSetConstantCenter(RpAtomic *atomic, RwV2d *center)
{
    RpPTankAtomicExtPrv *ptankGlobal = NULL;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicSetConstantCenter"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

    ptankGlobal->publicData.cCenter = *center;
    ptankGlobal->instFlags |= rpPTANKIFLAGCENTER;

    RWRETURNVOID();
}


/**
 * \ingroup rpptank
 * \ref RpPTankAtomicGetConstantCenter
 * is used to get the center of the particles.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 *
 * \return A pointer to the particles center.
 *
 * \see RpPTankAtomicSetConstantCenter
 */
const RwV2d *
RpPTankAtomicGetConstantCenter(RpAtomic *atomic)
{
    RpPTankAtomicExtPrv *ptankGlobal = NULL;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicGetConstantCenter"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

    RWRETURN(&ptankGlobal->publicData.cCenter);
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicSetConstantSize
 * is used to set the size of the particles. This size is the size for all particles
 * when no size or matrix is specified when calling \ref RpPTankAtomicCreate.
 * The default value is (1.0f,1.0f).
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 * \param size A pointer to a 2d vector holding the new size. size->x
 * represents the width and size->y the height of the particles.
 *
 * \return None.
 *
 * \see RpPTankAtomicGetConstantSize
 * \see RpPTankAtomicCreate
 */
void
RpPTankAtomicSetConstantSize(RpAtomic *atomic, RwV2d *size)
{
    RpPTankAtomicExtPrv *ptankGlobal = NULL;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicSetConstantSize"));

    RWASSERT(atomic);
    RWASSERT(size);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

    ptankGlobal->publicData.cSize = *size;

    ptankGlobal->instFlags |= rpPTANKIFLAGCNSSIZE;

    RWRETURNVOID();
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicGetConstantSize
 * is used to get the size of the particles.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 *
 * \return A pointer to the particles size.
 *
 * \see RpPTankAtomicSetConstantSize
 *
 */
const RwV2d *
RpPTankAtomicGetConstantSize(RpAtomic *atomic)
{
    RpPTankAtomicExtPrv *ptankGlobal = NULL;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicGetConstantSize"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

    RWRETURN(&ptankGlobal->publicData.cSize);
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicSetConstantRotate
 * is used to set the angle of rotation of the particles. If the rotation's
 * angle is 0.0f, the particles will not be rotated. The rotation is made in the
 * screen plane, around the Z axis, through the constant center.
 *
 * To use this feature the \ref rpPTANKDFLAGCNS2DROTATE flag should be
 * passed to \ref RpPTankAtomicCreate.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 * \param rotate The new angle of rotation.
 *
 * \return None.
 *
 * \see RpPTankAtomicGetConstantRotate
 * \see RpPTankAtomicSetConstantCenter
 * \see RpPTankAtomicGetConstantCenter
 * \see RpPTankAtomicCreate
 */
void
RpPTankAtomicSetConstantRotate(RpAtomic *atomic, RwReal rotate)
{
    RpPTankAtomicExtPrv *ptankGlobal = NULL;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicSetConstantRotate"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

    ptankGlobal->publicData.cRotate = rotate;

    ptankGlobal->instFlags |= rpPTANKIFLAGCNS2DROTATE;

    RWRETURNVOID();
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicGetConstantRotate
 * is used to get the angle of rotation of the particles.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 *
 * \return The angle of rotation.
 *
 * \see RpPTankAtomicSetConstantRotate
 *
 */
RwReal
RpPTankAtomicGetConstantRotate(RpAtomic *atomic)
{
    RpPTankAtomicExtPrv *ptankGlobal = NULL;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicGetConstantRotate"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

    RWRETURN(ptankGlobal->publicData.cRotate);
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicSetConstantMatrix
 * is used to set the matrix of the particles. The matrix will be used to
 * create 3D oriented particles lying in the (right,up) plane. The pos
 * member of the \ref RwMatrix is ignored.
 *
 * To use this feature the \ref rpPTANKDFLAGCNSMATRIX flag should be
 * passed to \ref RpPTankAtomicCreate.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 * \param matrix  A pointer to the new matrix.
 *
 * \return None.
 *
 * \see RpPTankAtomicGetConstantMatrix
 * \see RpPTankAtomicCreate
 */
void
RpPTankAtomicSetConstantMatrix(RpAtomic *atomic, RwMatrix *matrix)
{
    RpPTankAtomicExtPrv *ptankGlobal = NULL;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicSetConstantMatrix"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

    RwMatrixCopy(&(ptankGlobal->publicData.cMatrix),matrix);

    ptankGlobal->instFlags |= rpPTANKIFLAGCNSMATRIX;

    RWRETURNVOID();
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicGetConstantMatrix
 * is used to get the particles matrix.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 *
 * \return A pointer to the matrix.
 *
 * \see RpPTankAtomicSetConstantMatrix
 *
 */
const RwMatrix *
RpPTankAtomicGetConstantMatrix(RpAtomic *atomic)
{
    RpPTankAtomicExtPrv *ptankGlobal = NULL;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicGetConstantMatrix"));
    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

    RWRETURN(&(ptankGlobal->publicData.cMatrix));
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicSetConstantColor
 * is used to set the color of the particles. All the particles share this
 * color when no color information is specified when calling \ref RpPTankAtomicCreate.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 * \param color A pointer to the new color
 *
 * \return None.
 *
 * \see RpPTankAtomicGetConstantColor
 * \see RpPTankAtomicSetConstantVtxColor
 * \see RpPTankAtomicGetConstantVtxColor
 * \see RpPTankAtomicCreate
 */
void
RpPTankAtomicSetConstantColor(RpAtomic *atomic, RwRGBA *color)
{
    RpPTankAtomicExtPrv *ptankGlobal = NULL;
    RpGeometry *geom;
    RpMaterial *material;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicSetConstantColor"));

    RWASSERT(atomic);
    RWASSERT(color);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

    geom = RpAtomicGetGeometry(atomic);
    material = RpGeometryGetMaterial(geom,0);

    ptankGlobal->publicData.cColor = *color;

    if( material )
    {
        RpMaterialSetColor(material,&ptankGlobal->publicData.cColor);
    }

    ptankGlobal->instFlags |= rpPTANKIFLAGCNSCOLOR;

    RWRETURNVOID();
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicGetConstantColor
 * is used to get the particles constant color.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 *
 * \return A pointer to the constant color.
 *
 * \see RpPTankAtomicSetConstantColor
 * \see RpPTankAtomicSetConstantVtxColor
 * \see RpPTankAtomicGetConstantVtxColor
 *
 */
const RwRGBA *
RpPTankAtomicGetConstantColor(RpAtomic *atomic)
{
    RpPTankAtomicExtPrv *ptankGlobal = NULL;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicGetConstantColor"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

    RWRETURN(&(ptankGlobal->publicData.cColor));
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicSetConstantVtxColor
 * is used to set the vertex colors of the particles.
 *
 * To use this feature the \ref rpPTANKDFLAGCNSVTXCOLOR flag should be
 * passed to \ref RpPTankAtomicCreate.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 * \param color  A pointer to an array of 4 \ref RwRGBA values,
 * one per particle vertex
 *
 * \return None.
 *
 * \see RpPTankAtomicGetConstantVtxColor
 * \see RpPTankAtomicSetConstantColor
 * \see RpPTankAtomicGetConstantColor
 * \see RpPTankAtomicCreate
 */
void
RpPTankAtomicSetConstantVtxColor(RpAtomic *atomic, RwRGBA *color)
{
    RpPTankAtomicExtPrv *ptankGlobal = NULL;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicSetConstantVtxColor"));

    RWASSERT(atomic);
    RWASSERT(color);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

    memcpy(ptankGlobal->publicData.cVtxColor,color,sizeof(RwRGBA)*4);

    ptankGlobal->instFlags |= rpPTANKIFLAGCNSVTXCOLOR;

    RWRETURNVOID();
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicGetConstantVtxColor
 * is used to get the particles vertex color.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 *
 * \return A pointer to the array of vertex colors.
 *
 * \see RpPTankAtomicSetConstantVtxColor
 * \see RpPTankAtomicSetConstantColor
 * \see RpPTankAtomicGetConstantColor
 *
 */
const RwRGBA *
RpPTankAtomicGetConstantVtxColor(RpAtomic *atomic)
{
    RpPTankAtomicExtPrv *ptankGlobal = NULL;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicGetConstantVtxColor"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

    RWRETURN(ptankGlobal->publicData.cVtxColor);
}


/**
 * \ingroup rpptank
 * \ref RpPTankAtomicSetConstantVtx2TexCoords
 * is used to set the two texture coordinates used per particle. Only the
 * top-left and bottom-right texture coordinates are submitted, the two others
 * are calculated using the following rules :
 *
 *             (U1,V1)--(U2,V1)
 *
 *             (U1,V2)--(U2,V2)
 *
 * To use this feature the \ref rpPTANKDFLAGCNSVTX2TEXCOORDS flag should be
 * passed to \ref RpPTankAtomicCreate.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 * \param UVs     A pointer to an array of 2 \ref RwTexCoords values.
 *
 * \return None.
 *
 * \see RpPTankAtomicSetConstantVtx2TexCoords
 * \see RpPTankAtomicSetConstantVtx4TexCoords
 * \see RpPTankAtomicGetConstantVtx4TexCoords
 * \see RpPTankAtomicCreate
 */
void
RpPTankAtomicSetConstantVtx2TexCoords(RpAtomic *atomic, RwTexCoords *UVs)
{
    RpPTankAtomicExtPrv *ptankGlobal = NULL;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicSetConstantVtx2TexCoords"));

    RWASSERT(atomic);
    RWASSERT(UVs);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

    memcpy(ptankGlobal->publicData.cUV,UVs,sizeof(RwTexCoords)*2);

    ptankGlobal->instFlags |= rpPTANKIFLAGCNSVTX2TEXCOORDS;

    RWRETURNVOID();
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicGetConstantVtx2TexCoords
 * is used to get the two texture coordinates used per particle.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 *
 * \return A pointer to the array of texture coordinates.
 *
 * \see RpPTankAtomicSetConstantVtx2TexCoords
 * \see RpPTankAtomicSetConstantVtx4TexCoords
 * \see RpPTankAtomicGetConstantVtx4TexCoords
 *
 */
const RwTexCoords *
RpPTankAtomicGetConstantVtx2TexCoords(RpAtomic *atomic)
{
    RpPTankAtomicExtPrv *ptankGlobal = NULL;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicGetConstantVtx2TexCoords"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

    RWRETURN(ptankGlobal->publicData.cUV);
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicSetConstantVtx4TexCoords
 * is used to set the four texture coordinates used per particles.
 *
 * To use this feature the \ref rpPTANKDFLAGCNSVTX4TEXCOORDS flag should be
 * passed to \ref RpPTankAtomicCreate.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 * \param UVs     A pointer to an array of four \ref RwTexCoords values.
 *
 * \return None.
 *
 * \see RpPTankAtomicSetConstantVtx4TexCoords
 * \see RpPTankAtomicSetConstantVtx2TexCoords
 * \see RpPTankAtomicGetConstantVtx2TexCoords
 * \see RpPTankAtomicCreate
 */
void
RpPTankAtomicSetConstantVtx4TexCoords(RpAtomic *atomic, RwTexCoords *UVs)
{
    RpPTankAtomicExtPrv *ptankGlobal = NULL;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicSetConstantVtx4TexCoords"));

    RWASSERT(atomic);
    RWASSERT(UVs);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

    memcpy(ptankGlobal->publicData.cUV,UVs,sizeof(RwTexCoords)*4);

    ptankGlobal->instFlags |= rpPTANKIFLAGCNSVTX4TEXCOORDS;

    RWRETURNVOID();
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicGetConstantVtx4TexCoords
 * is used to get the four texture coordinates used per particles.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param atomic  A pointer to the PTank object to access.
 *
 * \return A pointer to the array of texture coordinates.
 *
 * \see RpPTankAtomicSetConstantVtx4TexCoords
 * \see RpPTankAtomicSetConstantVtx2TexCoords
 * \see RpPTankAtomicGetConstantVtx2TexCoords
 *
 */
const RwTexCoords *
RpPTankAtomicGetConstantVtx4TexCoords(RpAtomic *atomic)
{
    RpPTankAtomicExtPrv *ptankGlobal = NULL;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicGetConstantVtx4TexCoords"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

    RWRETURN(ptankGlobal->publicData.cUV);
}

#endif

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicLock
 * is used to lock and access the data held by the PTank object.
 *
 * The function fills a \ref RpPTankLockStruct with a pointer to the data
 * accessed and the stride of that data, whether in a structure or an array
 * organization.
 *
 * When using a structure organization, locking \ref rpPTANKLFLAGPOSITION
 * returns the base pointer of the array of structures.
 *
 * \param atomic  A pointer to the PTank object to access.
 * \param dst A pointer to the \ref RpPTankLockStruct to be filled.
 * \param dataFlags Only one flag needed (see \ref RpPTankDataLockFlags ).
 * \param lockFlag  Specifies the type of access (see \ref RpPTankLockFlags ).
 *
 * \return TRUE on success, FALSE otherwise.
 *
 * \see RpPTankAtomicUnlock
 * \see RpPTankAtomicGetDataFormat
 */
RwBool
RpPTankAtomicLock(RpAtomic *atomic, RpPTankLockStruct *dst,
            RwUInt32 dataFlags, RpPTankLockFlags lockFlag)
{
    RpPTankData *ptankGlobal = NULL;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicLock"));
    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));
    RWASSERT(0 != dataFlags);
    RWASSERT(0 != lockFlag);
    RWASSERT(dst);

    ptankGlobal = &(RPATOMICPTANKPLUGINDATA(atomic)->publicData);


    if(( ptankGlobal->format.dataFlags & dataFlags) == 0)
    {
        dst->data = NULL;
        dst->stride = 0;
        RWRETURN(FALSE);
    }

    switch(dataFlags)
    {
        case rpPTANKLFLAGPOSITION:
            *dst = ptankGlobal->clusters[RPPTANKSIZEPOSITION];
            break;
        case rpPTANKLFLAGCOLOR:
            *dst = ptankGlobal->clusters[RPPTANKSIZECOLOR];
            break;
        case rpPTANKLFLAGSIZE:
            *dst = ptankGlobal->clusters[RPPTANKSIZESIZE];
            break;
        case rpPTANKLFLAGMATRIX:
            *dst = ptankGlobal->clusters[RPPTANKSIZEMATRIX];
            break;
        case rpPTANKLFLAGNORMAL:
            *dst = ptankGlobal->clusters[RPPTANKSIZENORMAL];
            break;
        case rpPTANKLFLAG2DROTATE:
            *dst = ptankGlobal->clusters[RPPTANKSIZE2DROTATE];
            break;
        case rpPTANKLFLAGVTXCOLOR:
            *dst = ptankGlobal->clusters[RPPTANKSIZEVTXCOLOR];
            break;
        case rpPTANKLFLAGVTX2TEXCOORDS:
            *dst = ptankGlobal->clusters[RPPTANKSIZEVTX2TEXCOORDS];
            break;
        case rpPTANKLFLAGVTX4TEXCOORDS:
            *dst = ptankGlobal->clusters[RPPTANKSIZEVTX4TEXCOORDS];
            break;
        default:
            RWMESSAGE(LOCKABORTMSG);
        break;
    }

    if( 0 != ptankGlobal->format.stride )
    {
        dst->stride = ptankGlobal->format.stride;
    }

    RWASSERT(dst->data);

    if( dst->data && ((lockFlag & rpPTANKLOCKWRITE ) == rpPTANKLOCKWRITE))
    {
        RPATOMICPTANKPLUGINDATA(atomic)->lockFlags |= lockFlag | dataFlags;
    }

    RWRETURN(TRUE);
}

/**
 * \ingroup rpptank
 * \ref RpPTankAtomicUnlock
 *
 * is used to unlock some previously locked data
 *
 * \param atomic  A pointer to the PTank object to access.
 *
 * \return A Pointer to the PTank object on success, NULL otherwise.
 *
 * \see RpPTankAtomicLock
 * \see RpPTankAtomicGetDataFormat
 *
 */
RpAtomic *
RpPTankAtomicUnlock(RpAtomic *atomic)
{
    RpPTankAtomicExtPrv *ptankGlobal = NULL;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicUnlock"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    ptankGlobal = RPATOMICPTANKPLUGINDATA(atomic);

/*
    Just do nothing for now as the realy work is carried by instancing
  */
    ptankGlobal->instFlags |= ptankGlobal->lockFlags;
    ptankGlobal->lockFlags = 0;

    RWRETURN(atomic);
}


/**
 * \ingroup rpptank
 * \ref RpPTankAtomicSetMaterial
 * is called to set the material used by a PTank object
 *
 * \param atomic   A pointer to the PTank object to access.
 * \param material A pointer to the \ref RpMaterial to use.
 *
 * \return A Pointer to the PTank object on success, NULL otherwise.
 */
RpAtomic *
RpPTankAtomicSetMaterial(RpAtomic *atomic, RpMaterial *material)
{
    RpAtomic *result = NULL;
    RpGeometry *geom;
    RpMaterial *matPrev;
    RxPipeline *matPipe;
    RwInt32 numTriangles;

    RWAPIFUNCTION(RWSTRING("RpPTankAtomicSetMaterial"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    geom = RpAtomicGetGeometry(atomic);
    matPrev = RpGeometryGetMaterial(geom,0);

    RpMaterialGetPipeline(matPrev,&matPipe);
    RpMaterialSetPipeline(material, matPipe);

    /* To avoid multiple material in the material list, as all the pipes
     * use RpGeometryGetMaterial(geom,0), we'll just patch the matlist...
     */
     if( geom->matList.numMaterials > 0 )
     {
        geom->matList.materials[0] = material;
     }

    numTriangles = RpGeometryGetNumTriangles(geom);



    if( numTriangles )
    {
        /* We got a real geometry here so do it the proper way : */
        if (RpGeometryLock(geom, rpGEOMETRYLOCKPOLYGONS))
        {
            RpTriangle         *triangles;
            RwInt32             triNum;

            triangles = RpGeometryGetTriangles(geom);
            for (triNum = 0; triNum < numTriangles; triNum++)
            {
                RpGeometryTriangleSetMaterial(geom, triangles,
                                                material);
                triangles++;
            }

            RpGeometryUnlock(geom);
        }

        result = atomic;
    }
    else
    {
        RpMeshHeader *meshHeader;
        RpMesh *mesh;

        meshHeader = geom->mesh;

        if( (NULL != meshHeader) && (0 < meshHeader->numMeshes) )
        {
            mesh = (RpMesh *)(meshHeader + 1);

            if( NULL != mesh->material )
            {
                RpMaterialDestroy(mesh->material);
            }

            mesh->material = material;

            if( NULL != material )
            {
                RpMaterialAddRef(material);
            }
        }
    }

    RWRETURN(result);

}
