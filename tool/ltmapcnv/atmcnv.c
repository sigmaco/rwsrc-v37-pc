/*
 * Atomic Lightmap conversion toolkit
 *
 * Copyright (c) Criterion Software Limited
 */

/***************************************************************************
 *                                                                         *
 * Module  : atmcnv.h                                                    *
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
 Atomic Sector conversion Data
 */

static RwInt32
_rtLtMapCnvAtomicGetTexNameArrayIdx(RpAtomic *atm)
{
    RwInt32 userDataCount = 0;
    RpGeometry *geom = NULL;

    RWFUNCTION(RWSTRING("_rtLtMapCnvAtomicGetTexNameArrayIdx"));
    RWASSERT(NULL != atm);

    geom = RpAtomicGetGeometry(atm);
    userDataCount = RpGeometryGetUserDataArrayCount(geom);

    if((NULL != geom) && (userDataCount > 0))
    {
        RwInt32 i;
        for(i=0;i<userDataCount;i++)
        {
            RpUserDataArray *userData = RpGeometryGetUserDataArray(geom,i);
            if( userData )
            {
                RwInt32 numElement = RpUserDataArrayGetNumElements(userData);
                RpUserDataFormat format = RpUserDataArrayGetFormat(userData);
                RwInt32 numPoly = RpGeometryGetNumTriangles(geom);
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
 * \ref RtLtMapCnvAtomicGetTexNameArray is used to retrieve the
 * array of lightmap name references attached to an \ref RpAtomic.
 *
 * The size of the array is equal to the number of triangles in the
 * \ref RpAtomic.
 *
 * The array contains the lightmap texture reference for each triangle in the
 * \ref RpAtomic.
 *
 * \param atm A pointer to the \ref RpAtomic.
 * \return A pointer to an \ref RpUserDataArray on success. NULL otherwise.
 *
 * \see RtLtMapCnvAtomicGetUArray
 * \see RtLtMapCnvAtomicGetVArray
 *
 */
RpUserDataArray *
RtLtMapCnvAtomicGetTexNameArray(RpAtomic *atm)
{
    RwInt32 arrayIdx;
    RpUserDataArray *userData = NULL;
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvAtomicGetTexNameArray"));
    RWASSERT(NULL != atm );

    arrayIdx = _rtLtMapCnvAtomicGetTexNameArrayIdx(atm);

    if( arrayIdx > -1 )
    {
        userData = RpGeometryGetUserDataArray(RpAtomicGetGeometry(atm),arrayIdx);
    }

    RWRETURN(userData);
}


static RwInt32
_rtLtMapCnvAtomicGetUArrayIdx(RpAtomic *atm)
{
    RwInt32 userDataCount = 0;
    RpGeometry *geom = NULL;

    RWFUNCTION(RWSTRING("_rtLtMapCnvAtomicGetUArrayIdx"));
    RWASSERT(NULL != atm);

    geom = RpAtomicGetGeometry(atm);
    userDataCount = RpGeometryGetUserDataArrayCount(geom);

    if((NULL != geom) && (userDataCount > 0))
    {
        RwInt32 i;
        for(i=0;i<userDataCount;i++)
        {
            RpUserDataArray *userData = RpGeometryGetUserDataArray(geom,i);
            if( userData )
            {
                RwInt32 numElement = RpUserDataArrayGetNumElements(userData);
                RpUserDataFormat format = RpUserDataArrayGetFormat(userData);
                RwInt32 numVtx = RpGeometryGetNumVertices(geom);
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
 * \ref RtLtMapCnvAtomicGetUArray is used to retrieve the
 * array of U lightmap co-ordinates attached to an \ref RpAtomic.
 *
 * The size of the array is equal to the number of vertices in the
 * \ref RpAtomic.
 *
 * The array contains the imported U lightmap co-ordinate for each vertex in
 * the \ref RpAtomic.
 *
 * \param atm A pointer to the \ref RpAtomic.
 * \return A pointer to an \ref RpUserDataArray on success. NULL otherwise.
 *
 * \see RtLtMapCnvAtomicGetVArray
 * \see RtLtMapCnvAtomicGetTexNameArray
 *
 */
RpUserDataArray *
RtLtMapCnvAtomicGetUArray(RpAtomic *atm)
{
    RwInt32 arrayIdx;
    RpUserDataArray *userData = NULL;

    RWAPIFUNCTION(RWSTRING("RtLtMapCnvAtomicGetUArray"));
    RWASSERT(NULL != atm);

    arrayIdx = _rtLtMapCnvAtomicGetUArrayIdx(atm);

    if( arrayIdx > -1 )
    {
        userData = RpGeometryGetUserDataArray(RpAtomicGetGeometry(atm),arrayIdx);
    }

    RWRETURN(userData);
}


static RwInt32
_rtLtMapCnvAtomicGetVArrayIdx(RpAtomic *atm)
{
    RwInt32 userDataCount = 0;
    RpGeometry *geom = NULL;

    RWFUNCTION(RWSTRING("_rtLtMapCnvAtomicGetVArrayIdx"));
    RWASSERT(NULL != atm);

    geom = RpAtomicGetGeometry(atm);
    userDataCount = RpGeometryGetUserDataArrayCount(geom);

    if((NULL != geom) && (userDataCount > 0))
    {
        RwInt32 i;
        for(i=0;i<userDataCount;i++)
        {
            RpUserDataArray *userData = RpGeometryGetUserDataArray(geom,i);
            if( userData )
            {
                RwInt32 numElement = RpUserDataArrayGetNumElements(userData);
                RpUserDataFormat format = RpUserDataArrayGetFormat(userData);
                RwInt32 numVtx = RpGeometryGetNumVertices(geom);
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
 * \ref RtLtMapCnvAtomicGetVArray is used to retrieve the
 * array of V lightmap co-ordinates attached to an \ref RpAtomic.
 *
 * The size of the array is equal to the number of vertices in the
 * \ref RpAtomic.
 *
 * The array contains the imported V lightmap co-ordinate for each vertex in
 * the \ref RpAtomic.
 *
 * \param atm A pointer to the \ref RpAtomic.
 * \return A pointer to an \ref RpUserDataArray on success. NULL otherwise.
 *
 * \see RtLtMapCnvAtomicGetUArray
 * \see RtLtMapCnvAtomicGetTexNameArray
 *
 */
RpUserDataArray *
RtLtMapCnvAtomicGetVArray(RpAtomic *atm)
{
    RwInt32 arrayIdx;
    RpUserDataArray *userData = NULL;

    RWAPIFUNCTION(RWSTRING("RtLtMapCnvAtomicGetVArray"));
    RWASSERT(NULL != atm);

    arrayIdx = _rtLtMapCnvAtomicGetVArrayIdx(atm);

    if( arrayIdx > -1 )
    {
        userData = RpGeometryGetUserDataArray(RpAtomicGetGeometry(atm),arrayIdx);
    }

    RWRETURN(userData);
}


/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvAtomicCnvDataCreate is used to create
 * the conversion data for an \ref RpAtomic. This creates a set of
 * arrays for the \ref RpAtomic to store the UV lightmap co-ordinates and
 * the lightmap texture name.
 *
 * \param atm A pointer to the \ref RpAtomic.
 *
 * \return A pointer to an \ref RpAtomic on success. NULL otherwise.
 *
 * \see RtLtMapCnvAtomicCnvDataDestroy
 * \see RtLtMapCnvAtomicGetUArray
 * \see RtLtMapCnvAtomicGetVArray
 * \see RtLtMapCnvAtomicGetTexNameArray
 */
RpAtomic *
RtLtMapCnvAtomicCnvDataCreate(RpAtomic *atm)
{
    RwInt32 numVtx;
    RwInt32 numPoly;
    RpGeometry *geom;

    RWAPIFUNCTION(RWSTRING("RtLtMapCnvAtomicCnvDataCreate"));
    RWASSERT(NULL != atm);

    geom = RpAtomicGetGeometry(atm);
    RWASSERT(geom);

    numVtx = RpGeometryGetNumVertices(geom);
    numPoly = RpGeometryGetNumTriangles(geom);

    if( (numVtx > 0) && (numPoly > 0) )
    {

        RwInt32 i;
        RpUserDataArray *userData;

        RwInt32 u_vtxUserDataIdx;

        RwInt32 v_vtxUserDataIdx;

        RwInt32 polyUserDataIdx;

        /* Create U coordinate array */
        u_vtxUserDataIdx  =
            RpGeometryAddUserDataArray(geom,
            rtLtMapCnvUVtxUserDataName,
            rpREALUSERDATA,
            numVtx);

        if( -1 == u_vtxUserDataIdx )
        {
            RWRETURN((RpAtomic*)NULL);
        }

        /* Create V coordinate array */
        v_vtxUserDataIdx  =
            RpGeometryAddUserDataArray(geom,
            rtLtMapCnvVVtxUserDataName,
            rpREALUSERDATA,
            numVtx);

        if( -1 == v_vtxUserDataIdx )
        {
            RpGeometryRemoveUserDataArray(geom,
                            u_vtxUserDataIdx);
            RWRETURN((RpAtomic*)NULL);
        }

        /* Create texture name coordinate array */
        polyUserDataIdx  =
            RpGeometryAddUserDataArray(geom,
            rtLtMapCnvPolyUserDataName,
            rpSTRINGUSERDATA,
            numPoly);

        if( -1 == polyUserDataIdx )
        {
            RpGeometryRemoveUserDataArray(geom,
                            u_vtxUserDataIdx);
            RpGeometryRemoveUserDataArray(geom,
                            v_vtxUserDataIdx);
            RWRETURN((RpAtomic*)NULL);
        }

        /* initialise Arrays */
        userData = RpGeometryGetUserDataArray(geom,polyUserDataIdx);
        for(i=0;i<numPoly;i++)
        {
            RpUserDataArraySetString(userData,i,NULL);
        }

        userData = RpGeometryGetUserDataArray(geom,u_vtxUserDataIdx);
        for(i=0;i<numVtx;i++)
        {
            RpUserDataArraySetReal(userData,i,1974.0f);
        }

        userData = RpGeometryGetUserDataArray(geom,v_vtxUserDataIdx);
        for(i=0;i<numVtx;i++)
        {
            RpUserDataArraySetReal(userData,i,1980.0f);
        }

    }

    RWRETURN(atm);
}


/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvAtomicCnvDataDestroy is used to destroy
 * the conversion data attached to an \ref RpAtomic. This destroys the
 * set of arrays for storing the UV lightmap co-ordinates and the lightmap
 * texture name.
 *
 * \param atm A pointer to the \ref RpAtomic.
 *
 * \return A pointer to an \ref RpAtomic on success. NULL otherwise.
 *
 * \see RtLtMapCnvAtomicCnvDataCreate
 * \see RtLtMapCnvAtomicGetUArray
 * \see RtLtMapCnvAtomicGetVArray
 * \see RtLtMapCnvAtomicGetTexNameArray
 */
RpAtomic *
RtLtMapCnvAtomicCnvDataDestroy(RpAtomic *atm)
{
    RwInt32 idx;
    RpGeometry *geom;
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvAtomicCnvDataDestroy"));
    RWASSERT(NULL != atm);

    geom = RpAtomicGetGeometry(atm);
    RWASSERT(NULL != geom);

    idx = _rtLtMapCnvAtomicGetTexNameArrayIdx(atm);
    if( idx > -1 )
    {
        RpGeometryRemoveUserDataArray(geom,idx);
    }

    idx = _rtLtMapCnvAtomicGetUArrayIdx(atm);
    if( idx > -1 )
    {
        RpGeometryRemoveUserDataArray(geom,idx);
    }

    idx = _rtLtMapCnvAtomicGetVArrayIdx(atm);
    if( idx > -1 )
    {
        RpGeometryRemoveUserDataArray(geom,idx);
    }

    RWRETURN(atm);
}


/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvAtomicSessionCreate is used to create a
 * \ref RtLtMapCnvAtomicSession.
 *
 * The \ref RtLtMapCnvAtomicSession will be initialized with default values.
 * The pointer to an \ref RpAtomic is cached inside the session, since a session
 * is not usable with a NULL Atomic pointer.
 *
 * \param atomic A pointer to the \ref RpAtomic
 *
 * \return A pointer to an \ref RtLtMapCnvAtomicSession on success. NULL otherwise.
 *
 * \see RtLtMapCnvAtomicSessionDestroy
 * \see RtLtMapCnvAtomicConvert
 * \see RtLtMapCnvAtomicIsConvertible
 */
RtLtMapCnvAtomicSession *
RtLtMapCnvAtomicSessionCreate(RpAtomic *atomic)
{
    RtLtMapCnvAtomicSession *session = NULL;
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvAtomicSessionCreate"));
    RWASSERT(atomic);

    session = (RtLtMapCnvAtomicSession *)
                                 RwMalloc(sizeof(RtLtMapCnvAtomicSession),
                                   rwID_LTMAPCNVTOOLKIT | rwMEMHINTDUR_EVENT);
    RWASSERT(NULL!=session);

    if( NULL != session )
    {
        RpGeometry *geom;

        session->atomic = atomic;

        session->headILTM = NULL;
        session->ILTMCount = 0;

        session->headRWLTM = NULL;
        session->RWLTMCount = 0;

        session->samplingFactor = 2;
        RtLtMapCnvAtomicSessionSetImportedLtMapResample(session, FALSE);
        RtLtMapCnvAtomicSessionSetGreedyRasterization(session,FALSE);
        RtLtMapCnvAtomicSessionSetRejectionFactor(session,0.0f);

        geom = RpAtomicGetGeometry(atomic);

        session->texName =
                    RtLtMapCnvAtomicGetTexNameArray(atomic);
        session->vtxU = RtLtMapCnvAtomicGetUArray(atomic);
        session->vtxV = RtLtMapCnvAtomicGetVArray(atomic);

    }

    RWRETURN(session);
}


/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvAtomicSessionDestroy is used to destroy a
 * \ref RtLtMapCnvAtomicSession. Any temporary memory is also destroyed.
 *
 * \param session A pointer to the \ref RtLtMapCnvAtomicSession
 *
 * \see RtLtMapCnvAtomicSessionCreate
  * \see RtLtMapCnvAtomicConvert
 * \see RtLtMapCnvAtomicIsConvertible
 */
void
RtLtMapCnvAtomicSessionDestroy(RtLtMapCnvAtomicSession *session)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvAtomicSessionDestroy"));
    RWASSERT(NULL != session);

    if( NULL != session )
    {
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
 * \ref RtLtMapCnvAtomicConvert is used to convert imported
 * lightmaps for use within RenderWare.
 *
 * This function takes the imported lightmaps and converts it into a form useable
 * by RenderWare's LightMap plugin, \ref rpltmap.
 *
 * RenderWare lightmap textures are generated from a source sample image. For better
 * results, this source sample image can be larger than the final lightmap texture
 * and sampled down  to a smaller size.
 *
 * The sample factor specifies the scale of this source sample image relative to
 * the destination lightmap texture.
 *
 * The imported UV lightmap co-ordinates are re-mapped to new values in the
 * destination lightmaps.
 *
 * \param session A pointer to the \ref RtLtMapCnvAtomicSession
 * \param factor An \ref RwUInt32 representing the sample factor
 *
 * \return TRUE on sucess, FALSE otherwise.
 *
 * \see RtLtMapCnvAtomicSessionCreate
 * \see RtLtMapCnvAtomicSessionDestroy
 * \see RtLtMapCnvAtomicIsConvertible
 */
RwBool
RtLtMapCnvAtomicConvert(RtLtMapCnvAtomicSession *session, RwUInt32 factor)
{
    RwBool result = TRUE;
    RwBool importedLightmapFound = FALSE;
    RpAtomic *atomic;
    RpGeometry *geom;
    RpMorphTarget *morphTarget;

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

    RWAPIFUNCTION(RWSTRING("RtLtMapCnvAtomicConvert"));
    RWASSERT(NULL != session);
    RWASSERT(NULL != session->atomic);
    RWASSERT(NULL != session->geometry);
    RWASSERT(0 < factor);

    width = 1;
    height = 1;

    atomic = session->atomic;

    session->errorFlag = rtLTMAPERROR_NONE;
    session->samplingFactor = factor;

    geom = RpAtomicGetGeometry(session->atomic);
    morphTarget = RpGeometryGetMorphTarget(geom, 0);

    numVtx = RpGeometryGetNumVertices(geom);
    numTriangles = RpGeometryGetNumTriangles(geom);

    if( numVtx > 0 &&  numTriangles > 0)
    {
        objectData = RPLTMAPATOMICGETDATA(session->atomic);

        triangles = RpGeometryGetTriangles(geom);
        vtx = RpMorphTargetGetVertices(morphTarget);

        texCoords = RpGeometryGetVertexTexCoords(geom, rwTEXTURECOORDINATEINDEX1);

        ltmap = RpLtMapAtomicGetLightMap(atomic);
        if( NULL == ltmap )
        {
            RWMESSAGE(("No LightMap attached"));

            session->errorFlag |= rtLTMAPERROR_GENERATEDLIGHTMAPNOTFOUND |
                                  rtLTMAPERROR_NOGENERATEDLIGHTMAPS;
            RWRETURN(FALSE);
        }

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
                
                matid = triangles[j].matIndex;
                material = rpMaterialListGetMaterial(&geom->matList,matid);
                
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
                    iltmapName = RtLtMapCnvAtomicGetLightMapName(session,j);
                    if( NULL == iltmapName )
                    {
                        RWMESSAGE(("LightMap name unkown triangle %d",j));
                        session->errorFlag |=
                                            rtLTMAPERROR_IMPORTEDLIGHTMAPNOTFOUND;
                        result = FALSE;
                        continue;
                    }

                    if( 0 == iltmapName[0] )
                    {
                        RWMESSAGE(("No Imported LightMap (triangle %d",j));
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
                    RtLtMapCnvAtomicGetVtxUV(session,idx0,(&srcUV[0]));

                    dstUV[1] = texCoords[idx1];
                    RtLtMapCnvAtomicGetVtxUV(session,idx1,(&srcUV[1]));

                    dstUV[2] = texCoords[idx2];
                    RtLtMapCnvAtomicGetVtxUV(session,idx2,(&srcUV[2]));


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

    if( FALSE == importedLightmapFound )
    {
        session->errorFlag |= rtLTMAPERROR_NOIMPORTEDLIGHTMAPS;
    }

    RWRETURN(result);
}


/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvAtomicIsConvertible is used to determine if an
 * \ref RpAtomic can be converted for use by RenderWare's LightMap plugin,
 * \ref rpltmap.
 *
 * The function checks if imported lightmap data are attached to an \ref RpAtomic.
 * If so, then the \ref RpAtomic is determined to be convertible.
 *
 * \param atomic A pointer to the \ref RpAtomic
 *
 * \return TRUE if the \ref RpAtomic is convertible, FALSE otherwise.
 *
 * \see RtLtMapCnvAtomicSessionCreate
 * \see RtLtMapCnvAtomicSessionDestroy
 * \see RtLtMapCnvAtomicConvert
 */
RwBool
RtLtMapCnvAtomicIsConvertible(RpAtomic *atomic)
{
    RwBool result = FALSE;
    RwInt32 numVtx;
    RwInt32 numTriangle;
    RpGeometry *geom;

    RWAPIFUNCTION(RWSTRING("RtLtMapCnvAtomicIsConvertible"));
    RWASSERT(NULL != atomic);

    geom = RpAtomicGetGeometry(atomic);
    RWASSERT(NULL != geom);

    numVtx = RpGeometryGetNumVertices(geom);
    numTriangle = RpGeometryGetNumTriangles(geom);

    if( numVtx > 0 && numTriangle > 0 )
    {
        RwInt32 idx;
        idx = _rtLtMapCnvAtomicGetTexNameArrayIdx(atomic);

        result = (idx > -1);

        idx = _rtLtMapCnvAtomicGetVArrayIdx(atomic);
        result &= (idx > -1);

        idx = _rtLtMapCnvAtomicGetUArrayIdx(atomic);
        result &= (idx > -1);

    }

    RWRETURN(result);
}

#if ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) )

/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvAtomicSetVtxUV is used to assign a UV
 * lightmap co-ordinate to a vertex in an \ref RpAtomic.
 *
 * \note Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param session A pointer to the \ref RtLtMapCnvAtomicSession
 * \param idx An \ref RwInt32 representing the vertex index
 * \param tex A pointer to the \ref RwTexCoords to assign
 *
 * \see RtLtMapCnvAtomicSetVtxUV
 */
void
RtLtMapCnvAtomicSetVtxUV( RtLtMapCnvAtomicSession *session,
                            RwInt32 idx,
                            RwTexCoords *tex)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvAtomicSetVtxUV"));
    RWASSERT(NULL != session);
    RWASSERT(NULL != session->vtxU);
    RWASSERT(RpUserDataArrayGetNumElements(session->vtxU) > idx);
    RWASSERT(RpUserDataArrayGetNumElements(session->vtxV) > idx);
    RWASSERT(NULL != session->vtxV);
    RWASSERT(NULL != tex);

    RpUserDataArraySetReal(session->vtxU,idx,tex->u);
    RpUserDataArraySetReal(session->vtxV,idx,tex->v);
    RWRETURNVOID();
}

/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvAtomicGetVtxUV is used to retrieve the UV
 * lightmap co-ordinate of a vertex in an \ref RpAtomic.
 *
 * \note Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param session A pointer to the \ref RtLtMapCnvAtomicSession
 * \param idx An \ref RwInt32 representing the vertex index
 * \param tex A pointer to the \ref RwTexCoords for returning the UV co-ordinate
 *
 * \see RtLtMapCnvAtomicSetVtxUV
 */
void
RtLtMapCnvAtomicGetVtxUV(RtLtMapCnvAtomicSession *session,
                            RwInt32 idx,
                            RwTexCoords *tex)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvAtomicGetVtxUV"));
    RWASSERT(NULL != session);
    RWASSERT(NULL != session->vtxU);
    RWASSERT(RpUserDataArrayGetNumElements(session->vtxU) > idx);
    RWASSERT(NULL != session->vtxV);
    RWASSERT(RpUserDataArrayGetNumElements(session->vtxV) > idx);

    RWASSERT(NULL != tex);

    tex->u = RpUserDataArrayGetReal(session->vtxU,idx);
    tex->v = RpUserDataArrayGetReal(session->vtxV,idx);

    RWRETURNVOID();
}

/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvAtomicSetLightMapName is used to set the
 * lighmap texture used by the triangle in an \ref RpAtomic.
 *
 * Imported lightmap data may consist of more than one lightmap per \ref RpAtomic.
 * Each triangle references its lightmap texture by name.
 *
 * \note Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param session A pointer to the \ref RtLtMapCnvAtomicSession
 * \param idx An \ref RwInt32 representing the triangle index
 * \param name A pointer to an \ref RwChar for the name of the lightmap to assign
 *
 * \see RtLtMapCnvAtomicGetLightMapName
 */
void
RtLtMapCnvAtomicSetLightMapName(RtLtMapCnvAtomicSession *session,
                                    RwInt32 idx,
                                    RwChar *name)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvAtomicSetLightMapName"));
    RWASSERT(NULL != session);
    RWASSERT(NULL != session->texName);
    RWASSERT(RpUserDataArrayGetNumElements(session->texName) > idx);
    RWASSERT(NULL != name);

    RpUserDataArraySetString(session->texName,idx,name);

    RWRETURNVOID();
}

/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvAtomicGetLightMapName is used to retrieve the
 * lightmap texture used by the triangle in an \ref RpAtomic.
 *
 * Imported lightmap data may consist of more than one lightmap per \ref RpAtomic.
 * Each triangle references its lightmap texture by name.
 *
 * \note Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param session A pointer to the \ref RtLtMapCnvAtomicSession
 * \param idx An \ref RwInt32 representing the triangle index
 *
 * \return A pointer to an \ref RwChar that is the name of the lightmap, NULL otherwise
 *
 * \see RtLtMapCnvAtomicSetLightMapName
 */
RwChar *
RtLtMapCnvAtomicGetLightMapName(RtLtMapCnvAtomicSession *session, RwInt32 idx)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvAtomicGetLightMapName"));
    RWASSERT(NULL != session);
    RWASSERT(NULL != session->texName);
    RWASSERT(RpUserDataArrayGetNumElements(session->texName) > idx);

    RWRETURN(RpUserDataArrayGetString(session->texName,idx));
}

/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvAtomicSessionSetImportedLtMapResample is used to
 * select resampling of the imported lightmaps.
 *
 * RenderWare lightmap textures are generated from a source sample image. This
 * source sample image is generated from the imported lightmaps by re-rasterizing
 * the triangles into the source sample image.
 *
 * The source sample image size is based on the size of the destination lightmap
 * texture, which can be different to the imported lightmaps. This differences can
 * potentially cause samples in the imported lightmaps to be missed.  To eliminate
 * this error, imported lightmaps can be re-sampled to the same size as the source
 * sample image.
 *
 * \note Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param session A pointer to the \ref RtLtMapCnvAtomicSession
 * \param resample An \ref RwBool to select reaample
 *
 * \see RtLtMapCnvAtomicSessionGetImportedLtMapResample
 * \see RtLtMapCnvAtomicConvert
 */
void
RtLtMapCnvAtomicSessionSetImportedLtMapResample(
                                          RtLtMapCnvAtomicSession *session,
                                          RwBool resample)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvAtomicSessionSetImportedLtMapResample"));
    RWASSERT(session);

    session->reSampleImportedLightmap = resample;

    RWRETURNVOID();
}


/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvAtomicSessionSetGreedyRasterization is used to
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
 * \param session A pointer to the \ref RtLtMapCnvAtomicSession.
 * \param greedy An \ref RwBool to enable greedy rasterization.
 *
 * \see RtLtMapCnvAtomicSessionGetGreedyRasterization
 * \see RtLtMapCnvAtomicConvert
 */
void
RtLtMapCnvAtomicSessionSetGreedyRasterization(
                                          RtLtMapCnvAtomicSession *session,
                                          RwBool greedy)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvAtomicSessionSetGreedyRasterization"));
    RWASSERT(session);

    session->useGreedyRasterization = greedy;

    RWRETURNVOID();
}


/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvAtomicSessionSetRejectionFactor is used to
 * set the rejection factor. Triangles with an area smaller than this
 * threshold are rejected and are not lightmapped.
 *
 * \note Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param session A pointer to the \ref RtLtMapCnvAtomicSession
 * \param factor An \ref RwReal to set the rejection factor
 *
 * \see RtLtMapCnvAtomicSessionGetRejectionFactor
 * \see RtLtMapCnvAtomicConvert
 */
void
RtLtMapCnvAtomicSessionSetRejectionFactor(
                                          RtLtMapCnvAtomicSession *session,
                                          RwReal factor)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvAtomicSessionSetRejectionFactor"));
    RWASSERT(session);

    session->smallSizeRejectionFactor = factor;

    RWRETURNVOID();
}

/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvAtomicSessionGetImportedLtMapResample is used to
 * query if resampling of the imported lightmaps is set.
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
 * \param session A pointer to the \ref RtLtMapCnvAtomicSession.
 *
 * \return TRUE if resampling is enabled, FALSE otherwise.
 *
 * \see RtLtMapCnvAtomicSessionSetImportedLtMapResample
 * \see RtLtMapCnvAtomicConvert
 */
RwBool
RtLtMapCnvAtomicSessionGetImportedLtMapResample(
                                          RtLtMapCnvAtomicSession *session)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvAtomicSessionGetImportedLtMapResample"));
    RWASSERT(session);
    RWRETURN(session->reSampleImportedLightmap);
}


/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvAtomicSessionGetGreedyRasterization is used to
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
 * \param session A pointer to the \ref RtLtMapCnvAtomicSession
 *
 * \return TRUE if greedy rasterization is enabled. FALSE otherwise.
 *
 * \see RtLtMapCnvAtomicSessionSetGreedyRasterization
 * \see RtLtMapCnvAtomicConvert
 */
RwBool
RtLtMapCnvAtomicSessionGetGreedyRasterization(
                                          RtLtMapCnvAtomicSession *session)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvAtomicSessionGetGreedyRasterization"));
    RWASSERT(session);
    RWRETURN(session->useGreedyRasterization);

}


/**
 * \ingroup rtltmapcnv
 * \ref RtLtMapCnvAtomicSessionGetRejectionFactor is used to
 * retrieve the rejection factor. Triangles with an area smaller than this
 * threshold are rejected and are not lightmapped.
 *
 * \note Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param session A pointer to the \ref RtLtMapCnvAtomicSession
 *
 * \return An \ref RwReal representing the rejection factor
 *
 * \see RtLtMapCnvAtomicSessionSetRejectionFactor
 * \see RtLtMapCnvAtomicConvert
 */
RwReal
RtLtMapCnvAtomicSessionGetRejectionFactor(
                                          RtLtMapCnvAtomicSession *session)
{
    RWAPIFUNCTION(RWSTRING("RtLtMapCnvAtomicSessionGetRejectionFactor"));
    RWASSERT(session);
    RWRETURN(session->smallSizeRejectionFactor);
}

#endif /* ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) ) */

