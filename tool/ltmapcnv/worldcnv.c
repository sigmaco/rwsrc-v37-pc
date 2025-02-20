/*
 * World Lightmap conversion toolkit
 *
 * Copyright (c) Criterion Software Limited
 */

/***************************************************************************
 *                                                                         *
 * Module  : worldcnv.h                                                    *
 *                                                                         *
 * Purpose :                                                               *
 *                                                                         *
 **************************************************************************/

/****************************************************************************
 Includes
 */

#include <stdlib.h>
#include <string.h>

#include "rpplugin.h"
#include <rpdbgerr.h>
#include <rwcore.h>
#include <rtltmapcnv.h>
#include "../../plugin/ltmap/rpltmap.h"
#include "../ltmap/image.h"
#include "rasterng.h"
#include "rasterg.h"
#include "fillerng.h"
#include "fillerg.h"


/****************************************************************************
 Local Defines
 */

/****************************************************************************
 Public Globals
 */

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Structures
 */

/****************************************************************************
 World Sector conversion Data
 */

static RwInt32
_rtLtMapCnvWorldSectorGetTexNameArrayIdx(RpWorldSector *ws)
{
    RwInt32 userDataCount;

    RWFUNCTION(RWSTRING("_rtLtMapCnvWorldSectorGetTexNameArrayIdx"));
    RWASSERT(NULL != ws);

    userDataCount = RpWorldSectorGetUserDataArrayCount(ws);

    if (userDataCount > 0)
    {
        RwInt32 i;
        for(i=0;i<userDataCount;i++)
        {
            RpUserDataArray *userData = RpWorldSectorGetUserDataArray(ws,i);
            if( userData )
            {
                RwInt32 numElement = RpUserDataArrayGetNumElements(userData);
                RpUserDataFormat format = RpUserDataArrayGetFormat(userData);
                RwInt32 numPoly = RpWorldSectorGetNumTriangles(ws);
                RwChar *name = RpUserDataArrayGetName(userData);

                if( (rpSTRINGUSERDATA == format) &&
                    (numElement == numPoly) &&
                    (0 == rwstrcmp(name,rtLtMapCnvPolyUserDataName))
                    )
                {
                    RWRETURN(i);
                }
            }
        }
    }

    RWRETURN(-1);
}

/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvWorldSectorGetTexNameArray is used to retrieve the
 * array of lightmap name references attached to the \ref RpWorldSector.
 *
 * The size of the array is equal to the number of triangles in the
 * \ref RpWorldSector.
 *
 * The array contains the lightmap texture reference for each triangle in the
 * \ref RpWorldSector.
 *
 * \param ws A pointer to the \ref RpWorldSector.
 * \return A pointer to an \ref RpUserDataArray on success. NULL otherwise.
 *
 * \see RtLtMapCnvWorldSectorGetUArray
 * \see RtLtMapCnvWorldSectorGetVArray
 *
 */
RpUserDataArray *
RtLtMapCnvWorldSectorGetTexNameArray(RpWorldSector *ws)
{
    RwInt32 arrayIdx = _rtLtMapCnvWorldSectorGetTexNameArrayIdx(ws);
    RpUserDataArray *userData = NULL;

    RWAPIFUNCTION(RWSTRING("RtLtMapCnvWorldSectorGetTexNameArray"));

    if( arrayIdx > -1 )
    {
        userData = RpWorldSectorGetUserDataArray(ws,arrayIdx);
    }

    RWRETURN(userData);
}


static RwInt32
_rtLtMapCnvWorldSectorGetUArrayIdx(RpWorldSector *ws)
{
    RwInt32 userDataCount;

    RWFUNCTION(RWSTRING("_rtLtMapCnvWorldSectorGetUArrayIdx"));
    RWASSERT(NULL != ws);

    userDataCount = RpWorldSectorGetUserDataArrayCount(ws);

    if (userDataCount > 0)
    {
        RwInt32 i;
        for(i=0;i<userDataCount;i++)
        {
            RpUserDataArray *userData = RpWorldSectorGetUserDataArray(ws,i);
            if( userData )
            {
                RwInt32 numElement = RpUserDataArrayGetNumElements(userData);
                RpUserDataFormat format = RpUserDataArrayGetFormat(userData);
                RwInt32 numVtx = RpWorldSectorGetNumVertices(ws);
                RwChar *name = RpUserDataArrayGetName(userData);

                if( (rpREALUSERDATA == format) &&
                    (numElement == numVtx) &&
                    (0 == rwstrcmp(name,rtLtMapCnvUVtxUserDataName))
                    )
                {
                    RWRETURN(i);
                }
            }
        }
    }

    RWRETURN(-1);
}

/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvWorldSectorGetUArray is used to retrieve the
 * array of U lightmap co-ordinates attached to the \ref RpWorldSector.
 *
 * The size of the array is equal to the number of vertices in the
 * \ref RpWorldSector.
 *
 * The array contains the imported U lightmap co-ordinate for each vertex in
 * the \ref RpWorldSector.
 *
 * \param ws A pointer to the \ref RpWorldSector.
 * \return A pointer to an \ref RpUserDataArray on success. NULL otherwise.
 *
 * \see RtLtMapCnvWorldSectorGetVArray
 * \see RtLtMapCnvWorldSectorGetTexNameArray
 *
 */
RpUserDataArray *
RtLtMapCnvWorldSectorGetUArray(RpWorldSector *ws)
{
    RwInt32 arrayIdx;
    RpUserDataArray *userData = NULL;

    RWAPIFUNCTION(RWSTRING("RtLtMapCnvWorldSectorGetUArray"));
    RWASSERT(ws != NULL);

    arrayIdx = _rtLtMapCnvWorldSectorGetUArrayIdx(ws);

    if( arrayIdx > -1 )
    {
        userData = RpWorldSectorGetUserDataArray(ws,arrayIdx);
    }

    RWRETURN(userData);
}


static RwInt32
_rtLtMapCnvWorldSectorGetVArrayIdx(RpWorldSector *ws)
{
    RwInt32 userDataCount;

    RWFUNCTION(RWSTRING("_rtLtMapCnvWorldSectorGetVArrayIdx"));
    RWASSERT(NULL != ws);

    userDataCount = RpWorldSectorGetUserDataArrayCount(ws);

    if (userDataCount > 0)
    {
        RwInt32 i;
        for(i=0;i<userDataCount;i++)
        {
            RpUserDataArray *userData = RpWorldSectorGetUserDataArray(ws,i);
            if( userData )
            {
                RwInt32 numElement = RpUserDataArrayGetNumElements(userData);
                RpUserDataFormat format = RpUserDataArrayGetFormat(userData);
                RwInt32 numVtx = RpWorldSectorGetNumVertices(ws);
                RwChar *name = RpUserDataArrayGetName(userData);

                if( (rpREALUSERDATA == format) &&
                    (numElement == numVtx) &&
                    (0 == rwstrcmp(name,rtLtMapCnvVVtxUserDataName))
                    )
                {
                    RWRETURN(i);
                }
            }
        }
    }

    RWRETURN(-1);
}

/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvWorldSectorGetVArray is used to retrieve the
 * array of V lightmap co-ordinates attached to the \ref RpWorldSector.
 *
 * The size of the array is equal to the number of vertices in the
 * \ref RpWorldSector.
 *
 * The array contains the imported V lightmap co-ordinate for each vertex in
 * the \ref RpWorldSector.
 *
 * \param ws A pointer to the \ref RpWorldSector.
 * \return A pointer to an \ref RpUserDataArray on success. NULL otherwise.
 *
 * \see RtLtMapCnvWorldSectorGetUArray
 * \see RtLtMapCnvWorldSectorGetTexNameArray
 *
 */
RpUserDataArray *
RtLtMapCnvWorldSectorGetVArray(RpWorldSector *ws)
{
    RwInt32 arrayIdx;
    RpUserDataArray *userData = NULL;

    RWAPIFUNCTION(RWSTRING("RtLtMapCnvWorldSectorGetVArray"));
    RWASSERT(ws != NULL);

    arrayIdx = _rtLtMapCnvWorldSectorGetVArrayIdx(ws);

    if( arrayIdx > -1 )
    {
        userData = RpWorldSectorGetUserDataArray(ws,arrayIdx);
    }

    RWRETURN(userData);
}


/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvWorldSectorCnvDataCreate is used to create
 * the conversion data for the \ref RpWorldSector. This creates a set of
 * arrays for the \ref RpWorldSector to store the UV lightmap co-ordinates and
 * the lightmap texture name.
 *
 * \param ws A pointer to the \ref RpWorldSector.
 *
 * \return A pointer to an \ref RpWorldSector on success. NULL otherwise.
 *
 * \see RtLtMapCnvWorldSectorCnvDataDestroy
 * \see RtLtMapCnvWorldSectorGetUArray
 * \see RtLtMapCnvWorldSectorGetVArray
 * \see RtLtMapCnvWorldSectorGetTexNameArray
 */
RpWorldSector *
RtLtMapCnvWorldSectorCnvDataCreate(RpWorldSector *ws)
{
    RwInt32 numVtx;
    RwInt32 numPoly;

    RWAPIFUNCTION(RWSTRING("RtLtMapCnvWorldSectorCnvDataCreate"));
    RWASSERT(NULL != ws);

    numVtx = RpWorldSectorGetNumVertices(ws);
    numPoly = RpWorldSectorGetNumTriangles(ws);

    if( (numVtx > 0) && (numPoly > 0) )
    {

        RwInt32 i;
        RpUserDataArray *userData;

        RwInt32 u_vtxUserDataIdx;

        RwInt32 v_vtxUserDataIdx;

        RwInt32 polyUserDataIdx;

        /* Create U coordinate array */
        u_vtxUserDataIdx  =
            RpWorldSectorAddUserDataArray(ws,
            rtLtMapCnvUVtxUserDataName,
            rpREALUSERDATA,
            numVtx);

        if( -1 == u_vtxUserDataIdx )
        {
            RWRETURN((RpWorldSector*)NULL);
        }

        /* Create V coordinate array */
        v_vtxUserDataIdx  =
            RpWorldSectorAddUserDataArray(ws,
            rtLtMapCnvVVtxUserDataName,
            rpREALUSERDATA,
            numVtx);

        if( -1 == v_vtxUserDataIdx )
        {
            RpWorldSectorRemoveUserDataArray(ws,
                            u_vtxUserDataIdx);
            RWRETURN((RpWorldSector*)NULL);
        }

        /* Create texture name coordinate array */
        polyUserDataIdx  =
            RpWorldSectorAddUserDataArray(ws,
            rtLtMapCnvPolyUserDataName,
            rpSTRINGUSERDATA,
            numPoly);

        if( -1 == polyUserDataIdx )
        {
            RpWorldSectorRemoveUserDataArray(ws,
                            u_vtxUserDataIdx);
            RpWorldSectorRemoveUserDataArray(ws,
                            v_vtxUserDataIdx);
            RWRETURN((RpWorldSector*)NULL);
        }

        /* initialise Arrays */
        userData = RpWorldSectorGetUserDataArray(ws,polyUserDataIdx);
        for(i=0;i<numPoly;i++)
        {
            RpUserDataArraySetString(userData,i,NULL);
        }

        userData = RpWorldSectorGetUserDataArray(ws,u_vtxUserDataIdx);
        for(i=0;i<numVtx;i++)
        {
            RpUserDataArraySetReal(userData,i,1974.0f);
        }

        userData = RpWorldSectorGetUserDataArray(ws,v_vtxUserDataIdx);
        for(i=0;i<numVtx;i++)
        {
            RpUserDataArraySetReal(userData,i,1980.0f);
        }

    }

    RWRETURN(ws);
}


/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvWorldSectorCnvDataDestroy is used to destroy
 * the conversion data attached to the \ref RpWorldSector. This destroys the
 * set of arrays for storing the UV lightmap co-ordinates and the lightmap
 * texture name.
 *
 * \param ws A pointer to the \ref RpWorldSector.
 *
 * \return A pointer to an \ref RpWorldSector on success. NULL otherwise.
 *
 * \see RtLtMapCnvWorldSectorCnvDataCreate
 * \see RtLtMapCnvWorldSectorGetUArray
 * \see RtLtMapCnvWorldSectorGetVArray
 * \see RtLtMapCnvWorldSectorGetTexNameArray
 */
RpWorldSector *
RtLtMapCnvWorldSectorCnvDataDestroy(RpWorldSector *ws)
{
    RwInt32 idx;

    RWAPIFUNCTION(RWSTRING("RtLtMapCnvWorldSectorCnvDataDestroy"));
    RWASSERT(NULL != ws);

    idx = _rtLtMapCnvWorldSectorGetTexNameArrayIdx(ws);
    if( idx > -1 )
    {
        RpWorldSectorRemoveUserDataArray(ws,idx);
    }

    idx = _rtLtMapCnvWorldSectorGetUArrayIdx(ws);
    if( idx > -1 )
    {
        RpWorldSectorRemoveUserDataArray(ws,idx);
    }

    idx = _rtLtMapCnvWorldSectorGetVArrayIdx(ws);
    if( idx > -1 )
    {
        RpWorldSectorRemoveUserDataArray(ws,idx);
    }

    RWRETURN(ws);
}


static RpWorldSector *
_rtLtMapCnvWorldcountWS(RpWorldSector *ws, void *data)
{
    RwInt32 *count = (RwInt32 *)data;

    RWFUNCTION(RWSTRING("_rtLtMapCnvWorldcountWS"));
    RWASSERT(NULL != ws);

    (*count)++;

    RWRETURN(ws);
}

static RpWorldSector *
_rtLtMapCnvWSDataSetupCB(RpWorldSector *ws, void *data)
{
    RtLtMapCnvWorldSectorData **WSData = (RtLtMapCnvWorldSectorData **)data;

    RWFUNCTION(RWSTRING("_rtLtMapCnvWSDataSetupCB"));
    RWASSERT(NULL != ws);

    (*WSData)->worldSector = ws;

    (*WSData)->texName = RtLtMapCnvWorldSectorGetTexNameArray(ws);
    (*WSData)->vtxU = RtLtMapCnvWorldSectorGetUArray(ws);
    (*WSData)->vtxV = RtLtMapCnvWorldSectorGetVArray(ws);

    (*WSData)++;

    RWRETURN(ws);
}



/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvWorldSessionCreate is used to create a
 * \ref RtLtMapCnvWorldSession.
 *
 * The \ref RtLtMapCnvWorldSession is initialized with default values.
 * The pointer to the \ref RpWorld is cached inside the session, since a session
 * is not usable with a NULL world pointer.
 *
 * \param world A pointer to the \ref RpWorld
 *
 * \return A pointer to an \ref RtLtMapCnvWorldSession on success. NULL otherwise.
 *
 * \see RtLtMapCnvWorldSessionDestroy
 * \see RtLtMapCnvWorldConvert
 * \see RtLtMapCnvWorldIsConvertible
 */
RtLtMapCnvWorldSession *
RtLtMapCnvWorldSessionCreate(RpWorld *world)
{
    RtLtMapCnvWorldSession *session = NULL;
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvWorldSessionCreate"));
    RWASSERT(world);

    session = (RtLtMapCnvWorldSession *)
                                 RwMalloc(sizeof(RtLtMapCnvWorldSession),
                                   rwID_LTMAPCNVTOOLKIT | rwMEMHINTDUR_EVENT);
    RWASSERT(NULL!=session);

    if( NULL != session )
    {
        RwInt32 numSector = 0;
        RtLtMapCnvWorldSectorData *sectorList;

        session->world = world;

        session->headILTM = NULL;
        session->ILTMCount = 0;

        session->headRWLTM = NULL;
        session->RWLTMCount = 0;

        session->samplingFactor = 2;
        RtLtMapCnvWorldSessionSetImportedLtMapResample(session, FALSE);
        RtLtMapCnvWorldSessionSetGreedyRasterization(session,FALSE);
        RtLtMapCnvWorldSessionSetRejectionFactor(session,0.0f);

        RpWorldForAllWorldSectors(world,
                                _rtLtMapCnvWorldcountWS,
                                (void*)&numSector);

        session->numSector = numSector;

        sectorList = (RtLtMapCnvWorldSectorData*)
                          RwMalloc(sizeof(RtLtMapCnvWorldSectorData) * numSector,
                                   rwID_LTMAPCNVTOOLKIT | rwMEMHINTDUR_EVENT);

        if( NULL == sectorList )
        {
            RwFree(session);
            session = NULL;
        }
        else
        {
            session->sectorList = sectorList;

            RpWorldForAllWorldSectors(world,
                                    _rtLtMapCnvWSDataSetupCB,
                                    (void*)&sectorList);
        }
    }

    RWRETURN(session);
}


/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvWorldSessionDestroy is used to destroy a
 * \ref RtLtMapCnvWorldSession. Any temporary memory is also destroyed.
 *
 * \param session A pointer to the \ref RtLtMapCnvWorldSession
 *
 * \see RtLtMapCnvWorldSessionCreate
 * \see RtLtMapCnvWorldConvert
 * \see RtLtMapCnvWorldIsConvertible
 */
void
RtLtMapCnvWorldSessionDestroy(RtLtMapCnvWorldSession *session)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvWorldSessionDestroy"));
    RWASSERT(NULL != session);

    if( NULL != session )
    {
        if( NULL != session->sectorList )
        {
            RwFree(session->sectorList);
        }

        if( NULL != session->headILTM)
        {
            _rtLtMapCnvImageCacheDestroy(session->headILTM);
        }

        if( NULL != session->headRWLTM )
        {
            _rtLtMapCnvImageCacheDestroy(session->headRWLTM);
        }

        RwFree(session);
    }

    RWRETURNVOID();
}


/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvWorldConvert is used to convert imported
 * lightmaps for use within RenderWare.
 *
 * This function takes the imported lightmaps and converts them into a form useable
 * by RenderWare's LightMap plugin, \ref rpltmap.
 *
 * RenderWare lightmap textures are generated from a source sample image.  For
 * for better results, this source sample image can be larger than the final lightmap
 * texture and sampled down to a smaller size.
 *
 * The sample factor specifies the scale of this source sample image relative to
 * the destination lightmap texture.
 *
 * The imported UV lightmap co-ordinates are re-mapped to new values in the
 * destination lightmaps.
 *
 * \param session A pointer to the \ref RtLtMapCnvWorldSession
 * \param factor An \ref RwUInt32 representing the sample factor
 *
 * \return TRUE on sucess, FALSE otherwise.
 *
 * \see RtLtMapCnvWorldSessionCreate
 * \see RtLtMapCnvWorldSessionDestroy
 * \see RtLtMapCnvWorldIsConvertible
 */
RwBool
RtLtMapCnvWorldConvert(RtLtMapCnvWorldSession *session, RwUInt32 factor)
{
    RwBool result = TRUE;
    RwBool importedLightmapFound = FALSE;
    RwBool generatedLightmapFound = FALSE;
    RwInt32 i;
    RpWorld *world;
    RwInt32 numSector;
    RtLtMapCnvWorldSectorData *sector;

    RWAPIFUNCTION(RWSTRING("RtLtMapCnvWorldConvert"));
    RWASSERT(NULL != session);
    RWASSERT(NULL != session->world);
    RWASSERT(NULL != session->sectorList);
    RWASSERT(0 < session->numSector);
    RWASSERT(0 < factor);

    world = session->world;
    numSector = session->numSector;

    session->samplingFactor = factor;
    session->errorFlag = rtLTMAPERROR_NONE;

    for(i=0;i<numSector;i++)
    {
        RpWorldSector *ws;

        LtMapObjectData     *objectData;

        RwV3d *vtx;
        RwInt32 numVtx;

        RpTriangle *triangles;
        RwInt32 numTriangles;

        RwTexCoords *texCoords;

        rtLtMapCnvImageList *iltmap;
        RwChar *iltmapName;
        RwTexture *ltmap;
        RwInt32 width;
        RwInt32 height;

        rtLtMapCnvImageList *rwltmap;

        RwInt32 idx0,idx1,idx2;

        RwTexCoords dstUV[3];
        RwTexCoords srcUV[3];

        RwInt32 j;

        width = 1;
        height = 1;

        sector = &session->sectorList[i];

        ws = sector->worldSector;

        numVtx = RpWorldSectorGetNumVertices(ws);
        numTriangles = RpWorldSectorGetNumTriangles(ws);
        if( numVtx > 0 &&  numTriangles > 0)
        {
            objectData = RPLTMAPWORLDSECTORGETDATA(ws);

            triangles = ws->triangles;
            vtx = ws->vertices;

            texCoords = ws->texCoords[1];

            ltmap = RpLtMapWorldSectorGetLightMap(ws);
            if( NULL == ltmap )
            {
                RWMESSAGE(("No LightMap attached (Sector %d)",i));
                session->errorFlag |= rtLTMAPERROR_GENERATEDLIGHTMAPNOTFOUND;

                result = FALSE;

                continue;
            }

            generatedLightmapFound = TRUE;

            rwltmap = _rtLtMapCnvImageCacheGet(session->headRWLTM,
                                                    RwTextureGetName(ltmap));

            if( NULL == rwltmap && NULL != ltmap )
            {
                RwRaster *raster = RwTextureGetRaster(ltmap);

                width = RwRasterGetWidth(raster);
                height = RwRasterGetHeight(raster);

                session->headRWLTM =
                        _rtLtMapCnvImageCacheAdd(session->headRWLTM,
                                                        RwTextureGetName(ltmap));
                rwltmap = session->headRWLTM;


                rwltmap->texture = ltmap;
                (void) RwTextureAddRef(rwltmap->texture);

                rwltmap->image = RwImageCreate(width * session->samplingFactor,
                                               height * session->samplingFactor,
                                               32);
                rwltmap->image = RwImageAllocatePixels(rwltmap->image);

                _rtLtMapLightMapImageSetFromTexture(rwltmap->texture,
                                         rwltmap->image, objectData->sampleMap);
            }

            if( (NULL != rwltmap) && (NULL != rwltmap->image) )
            {
                for (j=0;j<numTriangles;j++)
                {
                    RpMaterial *material;
                    RwInt32 matid;
                    
                    matid = ws->matListWindowBase + triangles[j].matIndex;
                    material = rpMaterialListGetMaterial(&world->matList,matid);

                    if( 0 == (RtLtMapMaterialGetFlags(material) & 
                            rtLTMAPMATERIALLIGHTMAP) )
                    {
                        RwRGBA black = {0,0,0,rpLTMAPUSEDALPHA};
                        
                        // No lightmaps Just fill up with black
                        idx0 = triangles[j].vertIndex[0];
                        idx1 = triangles[j].vertIndex[1];
                        idx2 = triangles[j].vertIndex[2];

                        dstUV[0] = texCoords[idx0];
                        dstUV[1] = texCoords[idx1];
                        dstUV[2] = texCoords[idx2];

                        if (((dstUV[0].u - dstUV[1].u) * (dstUV[2].v - dstUV[1].v)) <=
                            ((dstUV[0].v - dstUV[1].v) * (dstUV[2].u - dstUV[1].u)))
                        {
                            RwTexCoords tmpTexCoords;

                            tmpTexCoords  = dstUV[2];
                            dstUV[2]      = dstUV[1];
                            dstUV[1]      = tmpTexCoords;
                        }

                        if( TRUE == session->useGreedyRasterization )
                        {
                            _rtltmapFillerGreedy(rwltmap->image, dstUV,
                                                        &black,
                                            session->smallSizeRejectionFactor);
                        
                        }
                        else
                        {
                            _rtltmapFillerNonGreedy(rwltmap->image, dstUV,
                                                        &black,
                                            session->smallSizeRejectionFactor);
                        }
                    }
                    else
                    {
                        // convert imported to native lightmaps

                        iltmapName = RtLtMapCnvWorldSectorGetLightMapName(sector,j);
                        if( NULL == iltmapName )
                        {
                            RWMESSAGE(("LightMap name unkown (Sector %d - triangle %d",
                                        i,j));
                            session->errorFlag |=
                                            rtLTMAPERROR_IMPORTEDLIGHTMAPNOTFOUND;
                            result = FALSE;
                            continue;
                        }

                        if( 0 == iltmapName[0] )
                        {
                            RWMESSAGE(("No Imported LightMap (Sector %d - triangle %d",
                                        i,j));
                            session->errorFlag |=
                                            rtLTMAPERROR_IMPORTEDLIGHTMAPNOTFOUND;
                            result = FALSE;
                            continue;
                        }

                        iltmap =
                        _rtLtMapCnvImageCacheGet(session->headILTM,iltmapName);

                        if( NULL == iltmap )
                        {
                            RwImage *image;
                            RwRaster *raster;
                            RwTexture *texture;

                            texture = RwTextureRead(iltmapName,NULL);

                            if( NULL == texture )
                            {
                                RWMESSAGE(("Texture Read Fail %s",iltmapName));
                                session->errorFlag |=
                                            rtLTMAPERROR_IMPORTEDLIGHTMAPNOTFOUND;
                                result = FALSE;
                                continue;
                            }

                            importedLightmapFound = TRUE;

                            session->headILTM =
                                    _rtLtMapCnvImageCacheAdd(session->headILTM,
                                                                iltmapName);
                            iltmap = session->headILTM;

                            iltmap->texture = texture;

                            raster = RwTextureGetRaster(iltmap->texture);
                            image = RwImageCreate(RwRasterGetWidth(raster),
                                                RwRasterGetHeight(raster),
                                                32);
                            image = RwImageAllocatePixels(image);
                            image = RwImageSetFromRaster(image, raster);

                            if( image )
                            {
                                if( (TRUE == session->reSampleImportedLightmap ) &&
                                    (RwImageGetWidth(image) !=
                                                width * (RwInt32) session->samplingFactor) &&
                                    (RwImageGetHeight(image) !=
                                                height * (RwInt32) session->samplingFactor) )
                                {
                                    iltmap->image = RwImageCreateResample(image,
                                                width * (RwInt32) session->samplingFactor,
                                                height * (RwInt32) session->samplingFactor);

                                    RwImageDestroy(image);
                                }
                                else
                                {
                                    iltmap->image = image;
                                }
                            }
                        }

                        idx0 = triangles[j].vertIndex[0];
                        idx1 = triangles[j].vertIndex[1];
                        idx2 = triangles[j].vertIndex[2];

                        dstUV[0] = texCoords[idx0];
                        RtLtMapCnvWorldSectorGetVtxUV(sector,idx0,(&srcUV[0]));

                        dstUV[1] = texCoords[idx1];
                        RtLtMapCnvWorldSectorGetVtxUV(sector,idx1,(&srcUV[1]));

                        dstUV[2] = texCoords[idx2];
                        RtLtMapCnvWorldSectorGetVtxUV(sector,idx2,(&srcUV[2]));

                        if( (srcUV[0].u > 1.0f) ||
                            (srcUV[0].v > 1.0f) ||
                            (srcUV[1].u > 1.0f) ||
                            (srcUV[1].v > 1.0f) ||
                            (srcUV[2].u > 1.0f) ||
                            (srcUV[2].v > 1.0f) )
                        {
                            RWMESSAGE(
                            ("Imported UVs are bigger than 1.0f - Triangle %d",j));

                            session->errorFlag |=
                                        rtLTMAPERROR_IMPORTEDUVSOVERONEDOTZERO;
                            result = FALSE;
                            continue;
                        }

                        if( (srcUV[0].u < 0.0f) ||
                            (srcUV[0].v < 0.0f) ||
                            (srcUV[1].u < 0.0f) ||
                            (srcUV[1].v < 0.0f) ||
                            (srcUV[2].u < 0.0f) ||
                            (srcUV[2].v < 0.0f) )
                        {
                            RWMESSAGE(
                                ("Imported UVs are less than 0.0f - Triangle %d",j));

                            session->errorFlag |=
                                        rtLTMAPERROR_IMPORTEDUVSOVERONEDOTZERO;
                            result = FALSE;
                            continue;
                        }

                        if (((dstUV[0].u - dstUV[1].u) * (dstUV[2].v - dstUV[1].v)) <=
                            ((dstUV[0].v - dstUV[1].v) * (dstUV[2].u - dstUV[1].u)))
                        {
                            RwTexCoords tmpTexCoords;

                            tmpTexCoords  = dstUV[2];
                            dstUV[2]      = dstUV[1];
                            dstUV[1]      = tmpTexCoords;

                            tmpTexCoords  = srcUV[2];
                            srcUV[2]      = srcUV[1];
                            srcUV[1]      = tmpTexCoords;
                        }

                        if( TRUE == session->useGreedyRasterization )
                        {
                            _rtltmapUVRemaperGreedy(rwltmap->image, dstUV,
                                                iltmap->image, srcUV,
                                                session->smallSizeRejectionFactor);
                        }
                        else
                        {
                            _rtltmapUVRemaperNonGreedy(rwltmap->image, dstUV,
                                                iltmap->image, srcUV,
                                                session->smallSizeRejectionFactor);
                        }
                    }       
                }
            }

            _rtLtMapTextureSetFromLightMapImage(rwltmap->texture,
                                                rwltmap->image,
                                                objectData->sampleMap);

            _rtLtMapDilate(rwltmap->texture, objectData->sampleMap);
        }


    }

    if( FALSE == importedLightmapFound )
    {
        session->errorFlag |= rtLTMAPERROR_NOIMPORTEDLIGHTMAPS;
    }

    if( FALSE == generatedLightmapFound )
    {
        session->errorFlag |= rtLTMAPERROR_NOGENERATEDLIGHTMAPS;
    }

    RWRETURN(result);
}

static RpWorldSector *
_rtLtMapCnvIsWSConvertible(RpWorldSector *ws, void *data)
{
    RwBool *result = (RwBool*)data;
    RwInt32 numVtx = RpWorldSectorGetNumVertices(ws);
    RwInt32 numPoly = RpWorldSectorGetNumTriangles(ws);

    RWFUNCTION(RWSTRING("_rtLtMapCnvIsWSConvertible"));

    if( numVtx > 0 && numPoly > 0 )
    {
        RwInt32 idx;
        RwBool valid;
        idx = _rtLtMapCnvWorldSectorGetTexNameArrayIdx(ws);

        valid = (idx > -1);

        idx = _rtLtMapCnvWorldSectorGetVArrayIdx(ws);
        valid &= (idx > -1);

        idx = _rtLtMapCnvWorldSectorGetUArrayIdx(ws);
        valid &= (idx > -1);

        *result |= valid;
    }


    RWRETURN(ws);
}

/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvWorldIsConvertible is used to determine if a
 * \ref RpWorld can be converted for use by RenderWare's LightMap plugin,
 * \ref rpltmap.
 *
 * The function checks if imported lightmap data are attached to the \ref RpWorld.
 * If so, the \ref RpWorld is determined to be convertible.
 *
 * Note that all non empty \ref RpWorldSector in the \ref RpWorld must contain
 * the imported lightmap data for the \ref RpWorld to be convertible.
 *
 * \param world A pointer to the \ref RpWorld
 *
 * \return TRUE if the \ref RpWorld is convertible, FALSE otherwise.
 *
 * \see RtLtMapCnvWorldSessionCreate
 * \see RtLtMapCnvWorldSessionDestroy
 * \see RtLtMapCnvWorldConvert
 */
RwBool
RtLtMapCnvWorldIsConvertible(RpWorld *world)
{
    RwBool result = FALSE;
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvWorldIsConvertible"));
    RWASSERT(NULL != world);

    RpWorldForAllWorldSectors(world,
                            _rtLtMapCnvIsWSConvertible,
                            (void*)&result);

    RWRETURN(result);
}

#if ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) )

/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvWorldSectorSetVtxUV is used to assign a UV
 * lightmap co-ordinate to a vertex in an \ref RpWorldSector.
 *
 * \note Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param data A pointer to the \ref RtLtMapCnvWorldSectorData
 * \param idx An \ref RwInt32 representing the vertex index
 * \param tex A pointer to the \ref RwTexCoords to assign
 *
 * \see RtLtMapCnvWorldSectorSetVtxUV
 */
void
RtLtMapCnvWorldSectorSetVtxUV( RtLtMapCnvWorldSectorData *data,
                            RwInt32 idx,
                            RwTexCoords *tex)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvWorldSectorSetVtxUV"));
    RWASSERT(NULL != data);
    RWASSERT(NULL != data->vtxU);
    RWASSERT(RpUserDataArrayGetNumElements(data->vtxU) > idx);
    RWASSERT(RpUserDataArrayGetNumElements(data->vtxV) > idx);
    RWASSERT(NULL != data->vtxV);
    RWASSERT(NULL != tex);

    RpUserDataArraySetReal(data->vtxU,idx,tex->u);
    RpUserDataArraySetReal(data->vtxV,idx,tex->v);
    RWRETURNVOID();
}

/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvWorldSectorGetVtxUV is used to retrieve the UV
 * lightmap co-ordinate of a vertex in an \ref RpWorldSector.
 *
 * \note Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param data A pointer to the \ref RtLtMapCnvWorldSectorData
 * \param idx An \ref RwInt32 representing the vertex index
 * \param tex A pointer to the \ref RwTexCoords for returning the UV co-ordinate
 *
 * \see RtLtMapCnvWorldSectorSetVtxUV
 */
void
RtLtMapCnvWorldSectorGetVtxUV(RtLtMapCnvWorldSectorData *data,
                            RwInt32 idx,
                            RwTexCoords *tex)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvWorldSectorGetVtxUV"));
    RWASSERT(NULL != data);
    RWASSERT(NULL != data->vtxU);
    RWASSERT(RpUserDataArrayGetNumElements(data->vtxU) > idx);
    RWASSERT(NULL != data->vtxV);
    RWASSERT(RpUserDataArrayGetNumElements(data->vtxV) > idx);

    RWASSERT(NULL != tex);

    tex->u = RpUserDataArrayGetReal(data->vtxU,idx);
    tex->v = RpUserDataArrayGetReal(data->vtxV,idx);

    RWRETURNVOID();
}

/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvWorldSectorSetLightMapName is used to set the
 * lighmap texture used by the triangle in an \ref RpWorldSector.
 *
 * Imported lightmap data may consist of more than one lightmap per \ref RpWorldSector.
 * Each triangle reference its lightmap texture by its name.
 *
 * \note Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param data A pointer to the \ref RtLtMapCnvWorldSectorData
 * \param idx An \ref RwInt32 representing the triangle index
 * \param name A pointer to an \ref RwChar for the name of the lightmap to assign
 *
 * \see RtLtMapCnvWorldSectorGetLightMapName
 */
void
RtLtMapCnvWorldSectorSetLightMapName(RtLtMapCnvWorldSectorData *data,
                                    RwInt32 idx,
                                    RwChar *name)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvWorldSectorSetLightMapName"));
    RWASSERT(NULL != data);
    RWASSERT(NULL != data->texName);
    RWASSERT(RpUserDataArrayGetNumElements(data->texName) > idx);
    RWASSERT(NULL != name);

    RpUserDataArraySetString(data->texName,idx,name);

    RWRETURNVOID();
}

/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvWorldSectorGetLightMapName is used to retrieve the
 * lighmap texture used by the triangle in an \ref RpWorldSector.
 *
 * Imported lightmap data may consist of more than one lightmap per \ref RpWorldSector.
 * Each triangle references its lightmap texture by name.
 *
 * \note Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param data A pointer to the \ref RtLtMapCnvWorldSectorData
 * \param idx An \ref RwInt32 representing the triangle index
 *
 * \return A pointer to an \ref RwChar that is the name of the lightmap, NULL otherwise
 *
 * \see RtLtMapCnvWorldSectorSetLightMapName
 */
RwChar *
RtLtMapCnvWorldSectorGetLightMapName(RtLtMapCnvWorldSectorData *data, RwInt32 idx)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvWorldSectorGetLightMapName"));
    RWASSERT(NULL != data);
    RWASSERT(NULL != data->texName);
    RWASSERT(RpUserDataArrayGetNumElements(data->texName) > idx);

    RWRETURN(RpUserDataArrayGetString(data->texName,idx));
}

/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvWorldSessionSetImportedLtMapResample is used to
 * select resampling of the imported lightmaps.
 *
 * RenderWare lightmap textures are generated from a source sample image. This
 * source sample image is generated from the imported lightmaps by re-rasterizing
 * the triangles into the source sample image.
 *
 * The source sample image size is based on the size of the destination lightmap
 * texture, which can be different to the imported lightmaps. This difference can
 * potentially cause samples in the imported lightmaps to be missed. To eliminate
 * this error, the imported lightmaps can be re-sampled to the same size as the
 * source sample image.
 *
 * \note Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param session A pointer to the \ref RtLtMapCnvWorldSession
 * \param resample An \ref RwBool to select reaample
 *
 * \see RtLtMapCnvWorldSessionGetImportedLtMapResample
 * \see RtLtMapCnvWorldConvert
 */
void
RtLtMapCnvWorldSessionSetImportedLtMapResample(
                                          RtLtMapCnvWorldSession *session,
                                          RwBool resample)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvWorldSessionSetImportedLtMapResample"));
    RWASSERT(session);

    session->reSampleImportedLightmap = resample;

    RWRETURNVOID();
}


/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvWorldSessionSetGreedyRasterization is used to
 * select the rasterization method. Triangles are rasterized during conversion
 * to remap their lightmaps from the imported form to RenderWare's form.
 *
 * During standard rasterization, a pixel is only considered to form part of
 * a triangle's lightmap if the triangle covers the centre of the pixel.
 *
 * During greedy rasterization, a pixel is considered to form part of a
 * triangle's lightmap if the triangle covers any part of the pixel.
 *
 * \note Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param session A pointer to the \ref RtLtMapCnvWorldSession
 * \param greedy An \ref RwBool to enable greedy rasterization.
 *
 * \see RtLtMapCnvWorldSessionGetGreedyRasterization
 * \see RtLtMapCnvWorldConvert
 */
void
RtLtMapCnvWorldSessionSetGreedyRasterization(
                                          RtLtMapCnvWorldSession *session,
                                          RwBool greedy)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvWorldSessionSetGreedyRasterization"));
    RWASSERT(session);

    session->useGreedyRasterization = greedy;

    RWRETURNVOID();
}


/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvWorldSessionSetRejectionFactor is used to
 * set the rejection factor. Triangles with an area smaller than this
 * threshold are rejected and are not lightmapped.
 *
 * \note Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param session A pointer to the \ref RtLtMapCnvWorldSession
 * \param factor An \ref RwReal to set the rejection factor
 *
 * \see RtLtMapCnvWorldSessionGetRejectionFactor
 * \see RtLtMapCnvWorldConvert
 */
void
RtLtMapCnvWorldSessionSetRejectionFactor(
                                          RtLtMapCnvWorldSession *session,
                                          RwReal factor)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvWorldSessionSetRejectionFactor"));
    RWASSERT(session);

    session->smallSizeRejectionFactor = factor;

    RWRETURNVOID();
}

/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvWorldSessionGetImportedLtMapResample is used to
 * query if resampling of the imported lightmaps is set.
 *
 * RenderWare lightmap textures are generated from a source sample image. This
 * source sample image is generated from the imported lightmaps by re-rasterizing
 * the triangles into the source sample image.
 *
 * The source sample image size is based on the size of the destination lightmap
 * texture, which can be different to the imported lightmaps. This difference can
 * potentially cause samples in the imported lightmaps to be missed. To eliminate
 * this error, the imported lightmaps can be re-sampled to the same size as
 * the source sample image.
 *
 * \note Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param session A pointer to the \ref RtLtMapCnvWorldSession.
 *
 * \return TRUE if resampling is enabled, FALSE otherwise.
 *
 * \see RtLtMapCnvWorldSessionSetImportedLtMapResample
 * \see RtLtMapCnvWorldConvert
 */
RwBool
RtLtMapCnvWorldSessionGetImportedLtMapResample(
                                          RtLtMapCnvWorldSession *session)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvWorldSessionGetImportedLtMapResample"));
    RWASSERT(session);
    RWRETURN(session->reSampleImportedLightmap);
}


/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvWorldSessionGetGreedyRasterization is used to
 * retrieve the rasterization method. Triangles are rasterized during conversion
 * to remap their lightmaps from the imported form to RenderWare's form.
 *
 * During standard rasterization, a pixel is only considered to form part of
 * a triangle's lightmap if the triangle covers the centre of the pixel.
 *
 * During greedy rasterization, a pixel is considered to form part of a
 * triangle's lightmap if the triangle covers any part of the pixel.
 *
 * \note Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param session A pointer to the \ref RtLtMapCnvWorldSession
 *
 * \return TRUE if greedy rasterization if enabled. FALSE otherwise.
 *
 * \see RtLtMapCnvWorldSessionSetGreedyRasterization
 * \see RtLtMapCnvWorldConvert
 */
RwBool
RtLtMapCnvWorldSessionGetGreedyRasterization(
                                          RtLtMapCnvWorldSession *session)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvWorldSessionGetGreedyRasterization"));
    RWASSERT(session);
    RWRETURN(session->useGreedyRasterization);
}


/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvWorldSessionGetRejectionFactor is used to
 * retrieve the rejection factor. Triangles with an area smaller than this
 * threshold are rejected and are not lightmapped.
 *
 * \note Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param session A pointer to the \ref RtLtMapCnvWorldSession
 *
 * \return An \ref RwReal representing the rejection factor
 *
 * \see RtLtMapCnvWorldSessionSetRejectionFactor
 * \see RtLtMapCnvWorldConvert
 */
RwReal
RtLtMapCnvWorldSessionGetRejectionFactor(
                                          RtLtMapCnvWorldSession *session)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvWorldSessionGetRejectionFactor"));
    RWASSERT(session);
    RWRETURN(session->smallSizeRejectionFactor);
}

#endif /* ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) ) */

