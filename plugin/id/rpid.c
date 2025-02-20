/*
 * Functionality for object IDs
 *
 * Copyright (c) Criterion Software Limited
 */

/**
 * \ingroup rpid
 * \page rpidoverview RpId Plugin Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rpid.h
 * \li \b Libraries: rwcore, rpworld, rpid
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpIdPluginAttach
 *
 * \subsection sidoverview Overview
 * The RpId plugin provides functionality to add identifiers to RenderWare
 * objects.
 */

/***************************************************************************
 *                                                                         *
 * Module  :    rpid.c                                                     *
 *                                                                         *
 * Purpose :    Id operations                                              *
 *                                                                         *
 **************************************************************************/

/****************************************************************************
 Includes
 */
#include <rwcore.h>
#include <rpworld.h>
#include <rpdbgerr.h>
#include <rpid.h>

/****************************************************************************
 Local (static) variables
 */
static RwInt32      idGeometryOffset;
static RwInt32      idGeometryStreamOffset;

static RwInt32      idWorldSectorOffset;
static RwInt32      idWorldSectorStreamOffset;

static RwInt32      idFrameOffset;
static RwInt32      idFrameStreamOffset;

static RwInt32      idCameraOffset;
static RwInt32      idCameraStreamOffset;

static RwInt32      idLightOffset;
static RwInt32      idLightStreamOffset;

static RwInt32      idMaterialOffset;
static RwInt32      idMaterialStreamOffset;

static RwInt32      idTextureOffset;
static RwInt32      idTextureStreamOffset;

/****************************************************************************
 Global variables
 */
const RpId rpIdNull = 0;

/****************************************************************************
 Local (static) functions
 */

/* Generic callback functions */

static void        *
idObjectConstruct(void *object, RwInt32 offset,
                          RwInt32 __RWUNUSED__ size)
{
    RpId  *id;

    RWFUNCTION(RWSTRING("idObjectConstruct"));
    RWASSERT(object);

    id = (RpId *)((RwUInt8 *)object + offset);

    *id = rpIdNull;

    RWRETURN(object);
}

static void        *
idObjectDestruct(void *object, RwInt32 __RWUNUSED__ offset,
                         RwInt32 __RWUNUSED__ size)
{
    RWFUNCTION(RWSTRING("idObjectDestruct"));
    RWASSERT(object);

    RWRETURN(object);
}

static void        *
idObjectCopy(void *destinationObject, const void *sourceObject,
                     RwInt32 offset,
                     RwInt32 __RWUNUSED__ size)
{
    RpId     *dst;
    const RpId *src;

    RWFUNCTION(RWSTRING("idObjectCopy"));
    RWASSERT(destinationObject);
    RWASSERT(sourceObject);

    dst = (RpId *)((RwUInt8 *)destinationObject + offset);
    src = (const RpId *)((const RwUInt8 *)sourceObject + offset);

    *dst = *src;

    RWRETURN(destinationObject);
}

static RwStream    *
idObjectStreamRead(RwStream * stream,
                           RwInt32 __RWUNUSED__ binaryLength,
                           void *object, RwInt32 offset,
                           RwInt32 __RWUNUSED__ size)
{
    RpId *id;

    RWFUNCTION(RWSTRING("idObjectStreamRead"));
    RWASSERT(stream);
    RWASSERT(object);

    id = (RpId *)((RwUInt8 *)object + offset);

    if (RwStreamRead(stream, (void *)id, sizeof(RpId)) == sizeof(RpId))
    {
        (void)RwMemNative32((void *)id, sizeof(RpId));

        RWRETURN(stream);
    }

    RWRETURN((RwStream *)NULL);
}

static RwStream    *
idObjectStreamWrite(RwStream * stream,
                            RwInt32 __RWUNUSED__ binaryLength,
                            const void *object,
                            RwInt32 offset,
                            RwInt32 __RWUNUSED__ size)
{
    RpId id;

    RWFUNCTION(RWSTRING("idObjectStreamWrite"));
    RWASSERT(stream);
    RWASSERT(object);

    id = *(const RpId *)((const RwUInt8 *)object + offset);

    (void)RwMemLittleEndian32(&id, sizeof(RpId));

    if (!RwStreamWrite(stream, &id, sizeof(RpId)))
    {
        RWRETURN((RwStream *)NULL);
    }

    RWRETURN(stream);
}

static              RwInt32
idObjectGetSize(const void *object,
                        RwInt32 offsetInObject,
                        RwInt32 __RWUNUSED__ sizeInObject)
{
    const RpId *id;

    RWFUNCTION(RWSTRING("idObjectGetSize"));
    RWASSERT(object);

    id = (const RpId *)((const RwUInt8 *)object + offsetInObject);

    if (*id != rpIdNull)
    {
        RWRETURN(sizeof(RpId));
    }

    RWRETURN(0);
}

/****************************************************************************
 API Functions
 */

/**
 * \ingroup rpid
 * \ref RpIdGenerateFromString is used to generate a reasonably unique id
 * from an arbitary length string. It uses CRC to generate the ID.
 *
 * The generated id is not guaranteed to be unique for all strings.
 *
 * Note that the include file rpid.h is required and must be included by
 * an application wishing to use this facility. The string id library is
 * contained in the file rpid.lib.
 *
 * The CRC algorithm used is:
 *
 * 1  A 32 bit value, the residual, is set 0xffffffff 
 * 2  The data stream is viewed as a series of bits starting with the lsb
 *    of the first byte and ending with the msb of the last byte. 
 * 3  Each bit is xor'ed with the lsb of the residual and if the result 
 *    is one, the generating (gf) value 0xdb710641 is xor'ed into the residual. 
 * 4  The residual is then rotated 1 bit to the right 
 * 5  Steps 3 and 4 are repeated for each bit in the stream 
 * 6  The ending crc is the bit inverted residual.
 *
 * For an overview of CRC, see:
 * http://www.gweep.net/~prefect/eng/reference/crc_v3.txt
 *
 *
 * \param  string   string to use to generate the id. If the string is
 *                  NULL the generating value will be returned.
 *
 * \return Returns id generated
 *
 */
RpId
RpIdGenerateFromString(const RwChar *string)
{
    const RwUInt32 gf = 0xdb710641;   /* generator function */
    RpId r = ~0;                      /* residual */
    RwInt32 length;

    RWAPIFUNCTION(RWSTRING("RpIdGenerateFromString"));

    if (NULL == string)
        RWRETURN(gf);

    length = rwstrlen(string);

    while (length--)
    {
        RwInt32 bitCount, byte = *string++;

        for (bitCount=0; bitCount < 8; bitCount++)
        {
            r ^= byte & 1;
            /* rotation combined with possible xor of "gf" */
            if (1 & r)
                r = ((r^gf) >> 1) | 0x80000000;
            else
                r >>= 1;

            byte >>= 1;
        }
    }

    RWRETURN(~r);
}

/* Plugin API */

/**
 * \ingroup rpid
 * \ref RpIdPluginAttach is used to attach the id plugin to the
 * RenderWare system. The plugin must be attached between initializing the
 * system with \ref RwEngineInit and opening it with \ref RwEngineOpen.
 *
 * Note that the include file rpid.h is required and must be included by
 * an application wishing to use this facility. The string id library is
 * contained in the file rpid.lib.
 *
 * \return Returns TRUE if successful, FALSE otherwise
 *
 */
RwBool
RpIdPluginAttach(void)
{
    RWAPIFUNCTION(RWSTRING("RpIdPluginAttach"));

    /* Register RpGeometry plugin */
    idGeometryOffset =
        RpGeometryRegisterPlugin(sizeof(RpId),
                                 rwID_IDPLUGIN,
                                 idObjectConstruct,
                                 idObjectDestruct,
                                 idObjectCopy);

    if (idGeometryOffset < 0)
    {
        RWRETURN(FALSE);
    }

    idGeometryStreamOffset =
        RpGeometryRegisterPluginStream(rwID_IDPLUGIN,
                                       idObjectStreamRead,
                                       idObjectStreamWrite,
                                       idObjectGetSize);

    if (idGeometryStreamOffset < 0)
    {
        RWRETURN(FALSE);
    }

    /* Register RpWorldSector plugin */
    idWorldSectorOffset =
        RpWorldSectorRegisterPlugin(sizeof(RpId),
                                    rwID_IDPLUGIN,
                                    idObjectConstruct,
                                    idObjectDestruct,
                                    idObjectCopy);

    if (idGeometryOffset < 0)
    {
        RWRETURN(FALSE);
    }

    idWorldSectorStreamOffset =
        RpWorldSectorRegisterPluginStream(rwID_IDPLUGIN,
                                          idObjectStreamRead,
                                          idObjectStreamWrite,
                                          idObjectGetSize);

    if (idGeometryStreamOffset < 0)
    {
        RWRETURN(FALSE);
    }

    /* Register RwFrame plugin */
    idFrameOffset =
        RwFrameRegisterPlugin(sizeof(RpId),
                                rwID_IDPLUGIN,
                                idObjectConstruct,
                                idObjectDestruct,
                                idObjectCopy);

    if (idFrameOffset < 0)
    {
        RWRETURN(FALSE);
    }

    idFrameStreamOffset =
        RwFrameRegisterPluginStream(rwID_IDPLUGIN,
                                    idObjectStreamRead,
                                    idObjectStreamWrite,
                                    idObjectGetSize);

    if (idFrameStreamOffset < 0)
    {
        RWRETURN(FALSE);
    }

    /* Register RwCamera plugin */
    idCameraOffset =
        RwCameraRegisterPlugin(sizeof(RpId),
                                rwID_IDPLUGIN,
                                idObjectConstruct,
                                idObjectDestruct,
                                idObjectCopy);

    if (idCameraOffset < 0)
    {
        RWRETURN(FALSE);
    }

    idCameraStreamOffset =
        RwCameraRegisterPluginStream(rwID_IDPLUGIN,
                                    idObjectStreamRead,
                                    idObjectStreamWrite,
                                    idObjectGetSize);

    if (idCameraStreamOffset < 0)
    {
        RWRETURN(FALSE);
    }

        /* Register RpLight plugin */
    idLightOffset =
        RpLightRegisterPlugin(sizeof(RpId),
                                rwID_IDPLUGIN,
                                idObjectConstruct,
                                idObjectDestruct,
                                idObjectCopy);

    if (idLightOffset < 0)
    {
        RWRETURN(FALSE);
    }

    idLightStreamOffset =
        RpLightRegisterPluginStream(rwID_IDPLUGIN,
                                    idObjectStreamRead,
                                    idObjectStreamWrite,
                                    idObjectGetSize);

    if (idLightStreamOffset < 0)
    {
        RWRETURN(FALSE);
    }

        /* Register RpMaterial plugin */
    idMaterialOffset =
       RpMaterialRegisterPlugin(sizeof(RpId),
                                rwID_IDPLUGIN,
                                idObjectConstruct,
                                idObjectDestruct,
                                idObjectCopy);

    if (idMaterialOffset < 0)
    {
        RWRETURN(FALSE);
    }

    idMaterialStreamOffset =
        RpMaterialRegisterPluginStream(rwID_IDPLUGIN,
                                    idObjectStreamRead,
                                    idObjectStreamWrite,
                                    idObjectGetSize);

    if (idMaterialStreamOffset < 0)
    {
        RWRETURN(FALSE);
    }

        /* Register RwTexture plugin */
    idTextureOffset =
        RwTextureRegisterPlugin(sizeof(RpId),
                                rwID_IDPLUGIN,
                                idObjectConstruct,
                                idObjectDestruct,
                                idObjectCopy);

    if (idTextureOffset < 0)
    {
        RWRETURN(FALSE);
    }

    idTextureStreamOffset =
        RwTextureRegisterPluginStream(rwID_IDPLUGIN,
                                    idObjectStreamRead,
                                    idObjectStreamWrite,
                                    idObjectGetSize);

    if (idTextureStreamOffset < 0)
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}

/*****************************************************************************
 * Set functions
 */

/**
 * \ingroup rpid
 * \ref RpGeometrySetId is used to set an id to a geometry
 *
 * The string id plugin must be attached before using this function.
 *
 * \param geometry  pointer to the geometry object
 * \param id        the id to set to the geometry
 *
 * \return a pointer to the geometry on success, otherwise NULL
 *
 * \see RpGeometryGetId
 * \see RpIdPluginAttach
 */
RpGeometry *
RpGeometrySetId(RpGeometry *geometry, RpId id)
{
    RWAPIFUNCTION(RWSTRING("RpGeometrySetId"));
    RWASSERT(geometry);

    *(RpId *)((RwUInt8 *)geometry + idGeometryOffset) = id;

    RWRETURN(geometry);
}

/**
 * \ingroup rpid
 * \ref RpWorldSectorSetId is used to set an id to a world sector
 *
 * The string id plugin must be attached before using this function.
 *
 * \param ws     pointer to the world sector object
 * \param id     the id to set to the world sector
 *
 * \return a pointer to the world sector on success, otherwise NULL
 *
 * \see RpWorldSectorGetId
 * \see RpIdPluginAttach
 */
RpWorldSector *
RpWorldSectorSetId(RpWorldSector *ws, RpId id)
{
    RWAPIFUNCTION(RWSTRING("RpWorldSectorSetId"));
    RWASSERT(ws);

    *(RpId *)((RwUInt8 *)ws + idWorldSectorOffset) = id;

    RWRETURN(ws);
}

/**
 * \ingroup rpid
 * \ref RwFrameSetId is used to set an id to a frame
 *
 * The string id plugin must be attached before using this function.
 *
 * \param frame     pointer to the geometry object
 * \param id        the id to set to the geometry
 *
 * \return a pointer to the frame on success, otherwise NULL
 *
 * \see RwFrameGetId
 * \see RpIdPluginAttach
 */
RwFrame *
RwFrameSetId(RwFrame *frame, RpId id)
{
    RWAPIFUNCTION(RWSTRING("RwFrameSetId"));
    RWASSERT(frame);

    *(RpId *)((RwUInt8 *)frame + idFrameOffset) = id;

    RWRETURN(frame);
}


/**
 * \ingroup rpid
 * \ref RwCameraSetId is used to set an id to a camera
 *
 * The string id plugin must be attached before using this function.
 *
 * \param camera    pointer to the camera object
 * \param id        the id to set to the camera
 *
 * \return a pointer to the camera on success, otherwise NULL
 *
 * \see RwCameraGetId
 * \see RpIdPluginAttach
 */
RwCamera *
RwCameraSetId(RwCamera *camera, RpId id)
{
    RWAPIFUNCTION(RWSTRING("RwCameraSetId"));
    RWASSERT(camera);

    *(RpId *)((RwUInt8 *)camera + idCameraOffset) = id;

    RWRETURN(camera);
}

/**
 * \ingroup rpid
 * \ref RpLightSetId is used to set an id to a light
 *
 * The string id plugin must be attached before using this function.
 *
 * \param light    pointer to the light object
 * \param id        the id to set to the light
 *
 * \return a pointer to the light on success, otherwise NULL
 *
 * \see RpLightGetId
 * \see RpIdPluginAttach
 */
RpLight *
RpLightSetId(RpLight *light, RpId id)
{
    RWAPIFUNCTION(RWSTRING("RpLightSetId"));
    RWASSERT(light);

    *(RpId *)((RwUInt8 *)light + idLightOffset) = id;

    RWRETURN(light);
}

/**
 * \ingroup rpid
 * \ref RpMaterialSetId is used to set an id to a material
 *
 * The string id plugin must be attached before using this function.
 *
 * \param material  pointer to the material object
 * \param id        the id to set to the material
 *
 * \return a pointer to the material on success, otherwise NULL
 *
 * \see RpMaterialGetId
 * \see RpIdPluginAttach
 */
RpMaterial *
RpMaterialSetId(RpMaterial *material, RpId id)
{
    RWAPIFUNCTION(RWSTRING("RpMaterialSetId"));
    RWASSERT(material);

    *(RpId *)((RwUInt8 *)material + idMaterialOffset) = id;

    RWRETURN(material);
}

/**
 * \ingroup rpid
 * \ref RwTextureSetId is used to set an id to a material
 *
 * The string id plugin must be attached before using this function.
 *
 * \param texture  pointer to the texture object
 * \param id        the id to set to the texture
 *
 * \return a pointer to the texture on success, otherwise NULL
 *
 * \see RwTextureGetId
 * \see RpIdPluginAttach
 */
RwTexture *
RwTextureSetId(RwTexture *texture, RpId id)
{
    RWAPIFUNCTION(RWSTRING("RwTextureSetId"));
    RWASSERT(texture);

    *(RpId *)((RwUInt8 *)texture + idTextureOffset) = id;

    RWRETURN(texture);
}


/*****************************************************************************
 * Get functions
 */


/**
 * \ingroup rpid
 * \ref RpGeometryGetId is used to get an id from a geometry
 *
 * The string id plugin must be attached before using this function.
 *
 * \param geometry  pointer to the geometry object
 *
 * \return a pointer to the geometry on success, otherwise NULL
 *
 * \see RpGeometrySetId
 * \see RpIdPluginAttach
 */
RpId
RpGeometryGetId(RpGeometry *geometry)
{
    RWAPIFUNCTION(RWSTRING("RpGeometryGetId"));
    RWASSERT(geometry);

    RWRETURN(*(RpId *)((RwUInt8 *)geometry + idGeometryOffset));
}

/**
 * \ingroup rpid
 * \ref RpWorldSectorGetId is used to get an id from a world sector
 *
 * The string id plugin must be attached before using this function.
 *
 * \param ws     pointer to the world sector object
 *
 * \return a pointer to the world sector on success, otherwise NULL
 *
 * \see RpWorldSectorSetId
 * \see RpIdPluginAttach
 */
RpId
RpWorldSectorGetId(RpWorldSector *ws)
{
    RWAPIFUNCTION(RWSTRING("RpWorldSectorGetId"));
    RWASSERT(ws);

    RWRETURN(*(RpId *)((RwUInt8 *)ws + idWorldSectorOffset));
}

/**
 * \ingroup rpid
 * \ref RwFrameGetId is used to get an id from a frame
 *
 * The string id plugin must be attached before using this function.
 *
 * \param frame     pointer to the geometry object
 *
 * \return a pointer to the frame on success, otherwise NULL
 *
 * \see RwFrameSetId
 * \see RpIdPluginAttach
 */
RpId
RwFrameGetId(RwFrame *frame)
{
    RWAPIFUNCTION(RWSTRING("RwFrameGetId"));
    RWASSERT(frame);

    RWRETURN(*(RpId *)((RwUInt8 *)frame + idFrameOffset));
}


/**
 * \ingroup rpid
 * \ref RwCameraGetId is used to get an id from a camera
 *
 * The string id plugin must be attached before using this function.
 *
 * \param camera    pointer to the camera object
 *
 * \return a pointer to the camera on success, otherwise NULL
 *
 * \see RwCameraSetId
 * \see RpIdPluginAttach
 */
RpId
RwCameraGetId(RwCamera *camera)
{
    RWAPIFUNCTION(RWSTRING("RwCameraGetId"));
    RWASSERT(camera);

    RWRETURN(*(RpId *)((RwUInt8 *)camera + idCameraOffset));
}

/**
 * \ingroup rpid
 * \ref RpLightGetId is used to get an id from a light
 *
 * The string id plugin must be attached before using this function.
 *
 * \param light    pointer to the light object
 *
 * \return a pointer to the light on success, otherwise NULL
 *
 * \see RpLightSetId
 * \see RpIdPluginAttach
 */
RpId
RpLightGetId(RpLight *light)
{
    RWAPIFUNCTION(RWSTRING("RpLightGetId"));
    RWASSERT(light);

    RWRETURN(*(RpId *)((RwUInt8 *)light + idLightOffset));
}

/**
 * \ingroup rpid
 * \ref RpMaterialGetId is used to get an id from a material
 *
 * The string id plugin must be attached before using this function.
 *
 * \param material  pointer to the material object
 *
 * \return a pointer to the material on success, otherwise NULL
 *
 * \see RpMaterialSetId
 * \see RpIdPluginAttach
 */
RpId
RpMaterialGetId(RpMaterial *material)
{
    RWAPIFUNCTION(RWSTRING("RpMaterialGetId"));
    RWASSERT(material);

    RWRETURN(*(RpId *)((RwUInt8 *)material + idMaterialOffset));
}

/**
 * \ingroup rpid
 * \ref RwTextureGetId is used to get an id from a material
 *
 * The string id plugin must be attached before using this function.
 *
 * \param texture  pointer to the texture object
 *
 * \return a pointer to the texture on success, otherwise NULL
 *
 * \see RwTextureSetId
 * \see RpIdPluginAttach
 */
RpId
RwTextureGetId(RwTexture *texture)
{
    RWAPIFUNCTION(RWSTRING("RwTextureGetId"));
    RWASSERT(texture);

    RWRETURN(*(RpId *)((RwUInt8 *)texture + idTextureOffset));
}
