/*
 * User Data plugin
 */

/**
 * \ingroup rpuserdata
 * \page rpuserdataoverview RpUserData Plugin Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rpusrdat.h
 * \li \b Libraries: rwcore, rpworld, rpusrdat
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpUserDataPluginAttach
 *
 * \subsection userdataoverview Overview
 * The RpUserData plugin provides functionality for storing user defined
 * data with geometry. The plugin extends \ref RpGeometry and \ref RpWorldSector
 * objects. Notionally, the plugin allows array of \ref RpUserDataArray structures
 * to be attached to these objects. Each \ref RpUserDataArray structure has an
 * identifier string, a data type (int, float, string), a number of elements
 * and a pointer to the data.
 *
 * Requires: Core Library and RpWorld Plugin.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rpplugin.h"
#include <rpdbgerr.h>
#include <rwcore.h>
#include <rpworld.h>

#include "rpusrdat.h"

#define OBJECTGETUSERDATALIST(object, offset) \
    ((RpUserDataList *)(((RwUInt8 *)object) + \
                         offset))

#define OBJECTGETCONSTUSERDATALIST(object, offset) \
    ((const RpUserDataList *)(((const RwUInt8 *)object) + \
                                offset))

#define RPGEOMETRYGETUSERDATALIST(geometry) \
    ((RpUserDataList *)(((RwUInt8 *)geometry) + \
                         userDataGeometryOffset))

#define RPGEOMETRYGETCONSTUSERDATALIST(geometry) \
    ((const RpUserDataList *)(((const RwUInt8 *)geometry) + \
                                userDataGeometryOffset))

#define RPWORLDSECTORGETUSERDATALIST(sector) \
    ((RpUserDataList *)(((RwUInt8 *)sector) + userDataWorldSectorOffset))

#define RPWORLDSECTORGETCONSTUSERDATALIST(sector) \
    ((const RpUserDataList *)(((const RwUInt8 *)sector) + userDataWorldSectorOffset))

#define RWFRAMEGETUSERDATALIST(frame) \
    ((RpUserDataList *)(((RwUInt8 *)frame) + \
                         userDataFrameOffset))

#define RWFRAMEGETCONSTUSERDATALIST(frame) \
    ((const RpUserDataList *)(((const RwUInt8 *)frame) + \
                                userDataFrameOffset))

#define RWCAMERAGETUSERDATALIST(camera) \
    ((RpUserDataList *)(((RwUInt8 *)camera) + \
                         userDataCameraOffset))

#define RWCAMERAGETCONSTUSERDATALIST(camera) \
    ((const RpUserDataList *)(((const RwUInt8 *)camera) + \
                                userDataCameraOffset))

#define RPLIGHTGETUSERDATALIST(light) \
    ((RpUserDataList *)(((RwUInt8 *)light) + \
                         userDataLightOffset))

#define RPLIGHTGETCONSTUSERDATALIST(light) \
    ((const RpUserDataList *)(((const RwUInt8 *)light) + \
                                userDataLightOffset))

#define RPMATERIALGETUSERDATALIST(material) \
    ((RpUserDataList *)(((RwUInt8 *)material) + \
                         userDataMaterialOffset))

#define RPMATERIALGETCONSTUSERDATALIST(material) \
    ((const RpUserDataList *)(((const RwUInt8 *)material) + \
                                userDataMaterialOffset))

#define RWTEXTUREGETUSERDATALIST(texture) \
    ((RpUserDataList *)(((RwUInt8 *)texture) + \
                         userDataTextureOffset))

#define RWTEXTUREGETCONSTUSERDATALIST(texture) \
    ((const RpUserDataList *)(((const RwUInt8 *)texture) + \
                                userDataTextureOffset))

#if (defined(RWDEBUG))
long                rpUserDataStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

RwModuleInfo        userDataModule;

static RwInt32      userDataGeometryOffset = 0;
static RwInt32      userDataGeometryStreamOffset = 0;

static RwInt32      userDataWorldSectorOffset = 0;
static RwInt32      userDataWorldSectorStreamOffset = 0;

static RwInt32      userDataFrameOffset = 0;
static RwInt32      userDataFrameStreamOffset = 0;

static RwInt32      userDataCameraOffset = 0;
static RwInt32      userDataCameraStreamOffset = 0;

static RwInt32      userDataLightOffset = 0;
static RwInt32      userDataLightStreamOffset = 0;

static RwInt32      userDataMaterialOffset = 0;
static RwInt32      userDataMaterialStreamOffset = 0;

static RwInt32      userDataTextureOffset = 0;
static RwInt32      userDataTextureStreamOffset = 0;

typedef struct RpUserDataList RpUserDataList;

struct RpUserDataList
{
    RwInt32             numElements;
    RpUserDataArray    *userData;
};

/* User Data functions */

static void        *
UserDataOpen(void *instance, RwInt32 __RWUNUSED__ offset,
             RwInt32 __RWUNUSED__ size)
{
    RWFUNCTION(RWSTRING("UserDataOpen"));
    RWASSERT(instance);

    ++userDataModule.numInstances;

    RWRETURN(instance);
}

static void        *
UserDataClose(void *instance, RwInt32 __RWUNUSED__ offset,
              RwInt32 __RWUNUSED__ size)
{
    RWFUNCTION(RWSTRING("UserDataClose"));
    RWASSERT(instance);

    --userDataModule.numInstances;

    RWRETURN(instance);
}

static void
UserDataDestruct(RpUserDataArray * userData)
{
    RWFUNCTION(RWSTRING("UserDataDestruct"));

    if (userData->name != NULL)
    {
        RwFree(userData->name);
        userData->name = (RwChar *)NULL;
    }

    if (userData->format == rpSTRINGUSERDATA)
    {
        RwInt32             i;
        RwChar            **charData = (RwChar **) userData->data;

        for (i = 0; i < userData->numElements; ++i)
        {
            if (charData[i] != NULL)
            {
                RwFree(charData[i]);
            }
        }
    }

    if (userData->data != NULL)
    {
        RwFree(userData->data);
        userData->data = (void *)NULL;
    }

    RWRETURNVOID();
}

static void
UserDataCopy(RpUserDataArray * dstUserData,
             RpUserDataArray * srcUserData)
{
    RwInt32             dataSize;

    RWFUNCTION(RWSTRING("UserDataCopy"));

    /* dstUserData must be empty */

    dstUserData->format = srcUserData->format;
    dstUserData->numElements = srcUserData->numElements;

    if (srcUserData->name != NULL)
    {
        rwstrdup(dstUserData->name, srcUserData->name);
    }

    if (srcUserData->data != NULL)
    {
        dataSize =
            dstUserData->numElements *
            RpUserDataGetFormatSize(dstUserData->format);

        dstUserData->data = RwMalloc(dataSize,
            rwID_USERDATAPLUGIN | rwMEMHINTDUR_EVENT);

        if (dstUserData->format == rpSTRINGUSERDATA)
        {
            RwInt32             i;
            RwChar            **srcCharData =
                (RwChar **) srcUserData->data;
            RwChar            **dstCharData =
                (RwChar **) dstUserData->data;

            for (i = 0; i < dstUserData->numElements; ++i)
            {
                if (srcCharData[i] == NULL)
                {
                    dstCharData[i] = (RwChar *) NULL;
                }
                else
                {
                    rwstrdup(dstCharData[i], srcCharData[i]);
                }
            }
        }
        else
        {
            memcpy(dstUserData->data, srcUserData->data, dataSize);
        }
    }

    RWRETURNVOID();
}

static RwStream    *
UserDataStreamRead(RpUserDataArray * userData, RwStream * stream)
{
    RwInt32             i, length;
    RwInt32            *intData;
    RwReal             *realData;
    RwChar            **charData;

    RWFUNCTION(RWSTRING("UserDataStreamRead"));

    if (!RwStreamReadInt(stream, &length, sizeof(RwInt32)))
    {
        RWRETURN((RwStream *) NULL);
    }

    if (length > 0)
    {
        userData->name = (RwChar *) RwMalloc(sizeof(RwChar) * length,
                             rwID_USERDATAPLUGIN | rwMEMHINTDUR_EVENT);

        if (userData->name == NULL)
        {
            RWRETURN((RwStream *) NULL);
        }

        if (!RwStreamRead
            (stream, userData->name, sizeof(RwChar) * length))
        {
            RWRETURN((RwStream *) NULL);
        }
    }

    if (!RwStreamReadInt
        (stream, (RwInt32 *) & userData->format, sizeof(RwInt32)))
    {
        RWRETURN((RwStream *) NULL);
    }

    if (!RwStreamReadInt
        (stream, &userData->numElements, sizeof(RwInt32)))
    {
        RWRETURN((RwStream *) NULL);
    }

    switch (userData->format)
    {
        case rpINTUSERDATA:
            userData->data =
                RwMalloc(sizeof(RwInt32) * userData->numElements,
                         rwID_USERDATAPLUGIN | rwMEMHINTDUR_EVENT);

            if (userData->data == NULL)
            {
                RWRETURN((RwStream *) NULL);
            }

            intData = (RwInt32 *) userData->data;

            if (!RwStreamReadInt(stream, intData, sizeof(RwInt32) * 
                                                  userData->numElements))
            {
                RWRETURN((RwStream *) NULL);
            }
            break;
        case rpREALUSERDATA:
            userData->data =
                RwMalloc(sizeof(RwReal) * userData->numElements,
                         rwID_USERDATAPLUGIN | rwMEMHINTDUR_EVENT);

            if (userData->data == NULL)
            {
                RWRETURN((RwStream *) NULL);
            }

            realData = (RwReal *) userData->data;

            if (!RwStreamReadReal(stream, realData, sizeof(RwReal) *
                                                userData->numElements))
            {
                RWRETURN((RwStream *) NULL);
            }
            break;
        case rpSTRINGUSERDATA:
            userData->data =
                RwMalloc(sizeof(RwChar *) * userData->numElements,
                         rwID_USERDATAPLUGIN | rwMEMHINTDUR_EVENT);

            if (userData->data == NULL)
            {
                RWRETURN((RwStream *) NULL);
            }

            charData = (RwChar **) userData->data;

            for (i = 0; i < userData->numElements; ++i)
            {
                if (!RwStreamReadInt(stream, &length, sizeof(RwInt32)))
                {
                    RWRETURN((RwStream *) NULL);
                }

                if (length > 0)
                {
                    charData[i] =
                        (RwChar *) RwMalloc(sizeof(RwChar) * length,
                                            rwID_USERDATAPLUGIN |
                                            rwMEMHINTDUR_EVENT);

                    if (charData[i] == NULL)
                    {
                        RWRETURN((RwStream *) NULL);
                    }

                    if (!RwStreamRead
                        (stream, charData[i], sizeof(RwChar) * length))
                    {
                        RWRETURN((RwStream *) NULL);
                    }
                }
                else
                {
                    charData[i] = (RwChar *) NULL;
                }
            }
            break;
        default:
            RWRETURN((RwStream *) NULL);
            break;
    }

    RWRETURN(stream);
}

static RwStream    *
UserDataStreamWrite(RpUserDataArray * userData, RwStream * stream)
{
    RwInt32             i, length;
    RwInt32            *intData;
    RwReal             *realData;
    RwChar            **charData;

    RWFUNCTION(RWSTRING("UserDataStreamWrite"));

    if (userData->name != NULL)
    {
        length = rwstrlen(userData->name) + 1;
    }
    else
    {
        length = 0;
    }

    if (!RwStreamWriteInt(stream, &length, sizeof(RwInt32)))
    {
        RWRETURN((RwStream *) NULL);
    }

    if (length > 0)
    {
        if (!RwStreamWrite
            (stream, userData->name, sizeof(RwChar) * length))
        {
            RWRETURN((RwStream *) NULL);
        }
    }

    if (!RwStreamWriteInt
        (stream, (RwInt32 *) & userData->format, sizeof(RwInt32)))
    {
        RWRETURN((RwStream *) NULL);
    }

    if (!RwStreamWriteInt
        (stream, &userData->numElements, sizeof(RwInt32)))
    {
        RWRETURN((RwStream *) NULL);
    }

    switch (userData->format)
    {
        case rpINTUSERDATA:
            intData = (RwInt32 *) userData->data;
            if (!RwStreamWriteInt(stream, intData, sizeof(RwInt32) *
                                                userData->numElements))
            {
                RWRETURN((RwStream *) NULL);
            }
            break;
        case rpREALUSERDATA:
            realData = (RwReal *) userData->data;
            if (!RwStreamWriteReal(stream, realData, sizeof(RwReal) *
                                                userData->numElements))
            {
                RWRETURN((RwStream *) NULL);
            }
            break;
        case rpSTRINGUSERDATA:
            charData = (RwChar **) userData->data;
            for (i = 0; i < userData->numElements; ++i)
            {
                if (charData[i] != NULL)
                {
                    length = rwstrlen(charData[i]) + 1;
                }
                else
                {
                    length = 0;
                }

                if (!RwStreamWriteInt(stream, &length, sizeof(RwInt32)))
                {
                    RWRETURN((RwStream *) NULL);
                }

                if (length > 0)
                {
                    if (!RwStreamWrite
                        (stream, charData[i], sizeof(RwChar) * length))
                    {
                        RWRETURN((RwStream *) NULL);
                    }
                }
            }
            break;
        default:
            RWRETURN((RwStream *) NULL);
            break;
    }

    RWRETURN(stream);
}

static              RwInt32
UserDataGetSize(RpUserDataArray * userData)
{
    RwInt32             i, length;
    RwInt32             size = 0;
    RwChar            **charData;

    RWFUNCTION(RWSTRING("UserDataGetSize"));

    if (userData != NULL)
    {
        size += sizeof(RwInt32);

        if (userData->name != NULL)
        {
            length = rwstrlen(userData->name) + 1;
            size += (sizeof(RwChar) * length);
        }

        size += (2 * sizeof(RwInt32));

        switch (userData->format)
        {
            case rpINTUSERDATA:
                size += (sizeof(RwInt32) * userData->numElements);
                break;
            case rpREALUSERDATA:
                size += (sizeof(RwReal) * userData->numElements);
                break;
            case rpSTRINGUSERDATA:
                charData = (RwChar **) userData->data;
                for (i = 0; i < userData->numElements; ++i)
                {
                    size += sizeof(RwInt32);

                    if (charData[i] != NULL)
                    {
                        length = rwstrlen(charData[i]) + 1;

                        size += (sizeof(RwChar) * length);
                    }
                }
                break;
            default:
                break;
        }
    }

    RWRETURN(size);
}

/* User Data list functions */

static void
UserDataListConstruct(RpUserDataList * userDataList)
{
    RWFUNCTION(RWSTRING("UserDataListConstruct"));

    userDataList->numElements = 0;
    userDataList->userData = (RpUserDataArray *) NULL;

    RWRETURNVOID();
}

static void
UserDataListDestruct(RpUserDataList * userDataList)
{
    RwInt32             i;

    RWFUNCTION(RWSTRING("UserDataListDestruct"));

    if (userDataList->userData != NULL)
    {
        for (i = 0; i < userDataList->numElements; ++i)
        {
            UserDataDestruct(&userDataList->userData[i]);
        }

        RwFree(userDataList->userData);
    }

    userDataList->userData    = (RpUserDataArray *) NULL;
    userDataList->numElements = 0;

    RWRETURNVOID();
}

static void
UserDataListCopy(RpUserDataList * dstList,
                 const RpUserDataList * srcList)
{
    RwInt32             i;

    RWFUNCTION(RWSTRING("UserDataListCopy"));

    UserDataListDestruct(dstList);

    dstList->numElements = srcList->numElements;

    if (dstList->numElements > 0)
    {
        dstList->userData = (RpUserDataArray *)
            RwMalloc(sizeof(RpUserDataArray) * dstList->numElements,
                     rwID_USERDATAPLUGIN | rwMEMHINTDUR_EVENT);

        for (i = 0; i < dstList->numElements; ++i)
        {
            UserDataCopy(&(dstList->userData[i]),
                         &(srcList->userData[i]));
        }
    }

    RWRETURNVOID();
}

static RwStream    *
UserDataListStreamRead(RpUserDataList * list, RwStream * stream)
{
    RwInt32             i, numElements;

    RWFUNCTION(RWSTRING("UserDataListStreamRead"));

    if (list != NULL)
    {
        if (!RwStreamReadInt(stream, &numElements, sizeof(RwInt32)))
        {
            RWRETURN((RwStream *) NULL);
        }

        list->numElements = numElements;

        list->userData = (RpUserDataArray *)
            RwMalloc(sizeof(RpUserDataArray) * list->numElements,
                     rwID_USERDATAPLUGIN | rwMEMHINTDUR_EVENT);

        for (i = 0; i < list->numElements; ++i)
        {
            stream = UserDataStreamRead(&list->userData[i], stream);
        }

        RWRETURN(stream);
    }
    RWRETURN((RwStream *) NULL);
}

static RwStream    *
UserDataListStreamWrite(const RpUserDataList * list, RwStream * stream)
{
    RwInt32             i;

    RWFUNCTION(RWSTRING("UserDataListStreamWrite"));

    if ((list != NULL) && (list->numElements > 0))
    {
        RwUInt32 numElementsToStream = 0;

        for (i = 0; i < list->numElements; ++i)
        {
            if (list->userData[i].format != rpNAUSERDATAFORMAT)
            {
                numElementsToStream++;
            }
        }

        if (numElementsToStream > 0)
        {
            if (!RwStreamWriteInt
                (stream, (RwInt32 *)&numElementsToStream, sizeof(RwInt32)))
            {
                RWRETURN((RwStream *) NULL);
            }

            for (i = 0; i < list->numElements; ++i)
            {
                if (list->userData[i].format != rpNAUSERDATAFORMAT)
                {
                    stream = UserDataStreamWrite(&list->userData[i], stream);
                }
            }
        }

        RWRETURN(stream);
    }
    RWRETURN((RwStream *) NULL);
}

static              RwInt32
UserDataListGetSize(const RpUserDataList * list)
{
    RwInt32             i;
    RwInt32             size = 0;

    RWFUNCTION(RWSTRING("UserDataListGetSize"));

    if ((list != NULL) && (list->numElements > 0))
    {
        for (i = 0; i < list->numElements; ++i)
        {
            if (list->userData[i].format != rpNAUSERDATAFORMAT)
            {
                size += UserDataGetSize(&list->userData[i]);
            }
        }

        if (size > 0)
        {
            size += sizeof(RwInt32);
        }
    }

    RWRETURN(size);
}

static              RwInt32
UserDataListGetNumElements(const RpUserDataList * list)
{
    RwInt32             numElements = 0;
    RwInt32             i;

    RWFUNCTION(RWSTRING("UserDataListGetNumElements"));

    RWASSERT(NULL != list);

    for (i = 0; i < list->numElements; ++i)
    {
        if (NULL != list->userData[i].data)
        {
            ++numElements;
            RWASSERT(NULL != list->userData[i].name);
            RWASSERT(rpNAUSERDATAFORMAT != list->userData[i].format);
            RWASSERT(0 != list->userData[i].numElements);
        }
    }

    RWRETURN(numElements);
}

static              RwInt32
UserDataListAddElement(RpUserDataList * list, RwChar * name,
                       RpUserDataFormat format, RwInt32 numElements)
{
    RpUserDataArray    *userData = (RpUserDataArray *)NULL;
    RwInt32             index = -1;
    RwInt32             i;

    RWFUNCTION(RWSTRING("UserDataListAddElement"));

    RWASSERT(0 < numElements);

    /* Search for an empty slot */
    for (i = 0; i < list->numElements; ++i)
    {
        if (NULL == list->userData[i].data)
        {
            userData = &list->userData[i];
            index = i;
            RWASSERT(NULL == list->userData[i].name);
            RWASSERT(rpNAUSERDATAFORMAT == list->userData[i].format);
            RWASSERT(0 == list->userData[i].numElements);

            break; /*Exit for loop so first empty slot is used.*/
        }
    }

    if (-1 == index)
    {
        /* We didn't find an empty slot, so grow the array */
        void *newUserData = NULL;
        if (list->userData != NULL)
        {
            newUserData = RwMalloc(sizeof(RpUserDataArray) *
                                   (list->numElements + 1),
                                   rwID_USERDATAPLUGIN |
                                   rwMEMHINTDUR_EVENT);

            if (newUserData == NULL)
            {
                RWRETURN(-1);
            }

            memcpy(newUserData,
                   list->userData,
                   sizeof(RpUserDataArray) * (list->numElements));

            RwFree(list->userData);
            list->userData = (RpUserDataArray *) newUserData;
        }
        else
        {
            list->userData = (RpUserDataArray *)
                RwMalloc(sizeof(RpUserDataArray) * (list->numElements + 1),
                         rwID_USERDATAPLUGIN | rwMEMHINTDUR_EVENT);

            if (list->userData == NULL)
            {
                RWRETURN(-1);
            }
        }
        index = list->numElements;
        ++list->numElements;
    }

    userData = &(list->userData[index]);

    userData->data = RwMalloc(RpUserDataGetFormatSize(format) *
                              numElements,
                              rwID_USERDATAPLUGIN | rwMEMHINTDUR_EVENT);

    if (userData->data == NULL)
    {
        RWRETURN(-1);
    }
    /* Initialize the data to 0's */
    memset(userData->data, 0, RpUserDataGetFormatSize(format) * numElements);

    rwstrdup(userData->name, name);
    userData->format = format;
    userData->numElements = numElements;

    RWRETURN(index);
}

static              void
UserDataListRemoveElement(RpUserDataList * list, RwInt32 index)
{
    RpUserDataArray *userData;
    
    RWFUNCTION(RWSTRING("UserDataListRemoveElement"));

    RWASSERT(NULL != list->userData);
    RWASSERT(index < list->numElements);

    userData = &(list->userData[index]);

    RWASSERT(NULL != userData->data);
    UserDataDestruct(userData);

    userData->format = rpNAUSERDATAFORMAT;
    userData->numElements = 0;

    /* Now pack down the array to ensure that all used entries are at
       the start */
    ++index;
    while (index < list->numElements)
    {
        list->userData[index-1].format = list->userData[index].format;
        list->userData[index-1].data = list->userData[index].data;
        list->userData[index-1].name = list->userData[index].name;
        list->userData[index-1].numElements = list->userData[index].numElements;
        ++index;
    }
    /* Ensure the last entry is blank */
    list->userData[list->numElements-1].format = rpNAUSERDATAFORMAT;
    list->userData[list->numElements-1].data = NULL;
    list->userData[list->numElements-1].name = NULL;
    list->userData[list->numElements-1].numElements = 0;

    RWRETURNVOID();
}



/* Generic callback functions */

static void        *
UserDataObjectConstruct(void *object, RwInt32 offset,
                          RwInt32 __RWUNUSED__ size)
{
    RpUserDataList  *userDataList;

    RWFUNCTION(RWSTRING("UserDataObjectConstruct"));
    RWASSERT(object);

    userDataList = OBJECTGETUSERDATALIST(object, offset);

    UserDataListConstruct(userDataList);

    RWRETURN(object);
}

static void        *
UserDataObjectDestruct(void *object, RwInt32 offset,
                         RwInt32 __RWUNUSED__ size)
{
    RpUserDataList  *userDataList;

    RWFUNCTION(RWSTRING("UserDataObjectDestruct"));
    RWASSERT(object);

    userDataList = OBJECTGETUSERDATALIST(object, offset);

    UserDataListDestruct(userDataList);

    RWRETURN(object);
}

static void        *
UserDataObjectCopy(void *destinationObject, const void *sourceObject,
                     RwInt32 offset,
                     RwInt32 __RWUNUSED__ size)
{
    RpUserDataList     *dstUserDataList;
    const RpUserDataList *srcUserDataList;

    RWFUNCTION(RWSTRING("UserDataObjectCopy"));
    RWASSERT(destinationObject);
    RWASSERT(sourceObject);

    dstUserDataList = OBJECTGETUSERDATALIST(destinationObject, offset);
    srcUserDataList = OBJECTGETCONSTUSERDATALIST(sourceObject, offset);

	//@{ 2007/01/20 burumal
	if ( !dstUserDataList || !srcUserDataList )
		return;
	//@}

    UserDataListCopy(dstUserDataList, srcUserDataList);	

    RWRETURN(destinationObject);
}

static RwStream    *
UserDataObjectStreamRead(RwStream * stream,
                           RwInt32 __RWUNUSED__ binaryLength,
                           void *object, RwInt32 offset,
                           RwInt32 __RWUNUSED__ size)
{
    RpUserDataList     *userDataList;

    RWFUNCTION(RWSTRING("UserDataObjectStreamRead"));
    RWASSERT(stream);
    RWASSERT(object);

    userDataList = OBJECTGETUSERDATALIST(object, offset);

    UserDataListStreamRead(userDataList, stream);

    RWRETURN(stream);
}

static RwStream    *
UserDataObjectStreamWrite(RwStream * stream,
                            RwInt32 __RWUNUSED__ binaryLength,
                            const void *object,
                            RwInt32 offset,
                            RwInt32 __RWUNUSED__ size)
{
    const RpUserDataList *userDataList;

    RWFUNCTION(RWSTRING("UserDataObjectStreamWrite"));
    RWASSERT(stream);
    RWASSERT(object);

    userDataList = OBJECTGETCONSTUSERDATALIST(object, offset);

    UserDataListStreamWrite(userDataList, stream);

    RWRETURN(stream);
}

static              RwInt32
UserDataObjectGetSize(const void *object,
                        RwInt32 offsetInObject,
                        RwInt32 __RWUNUSED__ sizeInObject)
{
    const RpUserDataList *userDataList;
    RwInt32             size = 0;

    RWFUNCTION(RWSTRING("UserDataObjectGetSize"));
    RWASSERT(object);

    userDataList = OBJECTGETCONSTUSERDATALIST(object, offsetInObject);

    size += UserDataListGetSize(userDataList);

    RWRETURN(size);
}


/* Geometry API */

/**
 * \ingroup rpuserdata
 * \ref RpGeometryAddUserDataArray is used to allocate and add a new
 * \ref RpUserDataArray to a \ref RpGeometry object.
 * The UserData plugin must be attached before using this function.
 *
 * \param geometry pointer to the geometry object
 * \param name an identifier string for the the user data
 * \param format the format of the user data array elements
 * \param numElements the number of elements on the user data array
 *
 * \return an index to the user data array on success, -1 on failure.
 *
 * \see RpGeometryGetUserDataArray
 * \see RpGeometryRemoveUserDataArray
 * \see RpUserDataPluginAttach
 */
RwInt32
RpGeometryAddUserDataArray(RpGeometry * geometry, RwChar * name,
                           RpUserDataFormat format, RwInt32 numElements)
{
    RwInt32             index;
    RpUserDataList     *userDataList;

    RWAPIFUNCTION(RWSTRING("RpGeometryAddUserDataArray"));
    RWASSERT(geometry);

    userDataList = RPGEOMETRYGETUSERDATALIST(geometry);

    index =
        UserDataListAddElement(userDataList, name, format, numElements);

    RWRETURN(index);
}

/**
 * \ingroup rpuserdata
 * \ref RpGeometryRemoveUserDataArray is used to remove and
 * deallocate a \ref RpUserDataArray added to a \ref RpGeometry object
 * by \ref RpGeometryAddUserDataArray.
 *
 * After calling this function any indices used to reference userdata
 * arrays on this object should be treated as invalid and recalculated.
 *
 * The UserData plugin must be attached before using this function.
 *
 * \param geometry  pointer to the geometry object
 * \param index     the index of the array to remove
 *
 * \return a pointer to the geometry on success, otherwise NULL
 *
 * \see RpGeometryGetUserDataArray
 * \see RpGeometryAddUserDataArray
 * \see RpUserDataPluginAttach
 */
RpGeometry *
RpGeometryRemoveUserDataArray(RpGeometry *geometry, RwInt32 index)
{
    RpUserDataList     *userDataList;

    RWAPIFUNCTION(RWSTRING("RpGeometryRemoveUserDataArray"));
    RWASSERT(geometry);

    userDataList = RPGEOMETRYGETUSERDATALIST(geometry);
    UserDataListRemoveElement(userDataList, index);

    RWRETURN(geometry);
}

/**
 * \ingroup rpuserdata
 * \ref RpGeometryGetUserDataArrayCount is used to retrieve the number of
 * \ref RpUserDataArray stored with a \ref RpGeometry.
 * The UserData plugin must be attached before using this function.
 *
 * \param geometry pointer to the geometry object
 *
 * \return the number of user data array stored with the geometry on success, otherwise -1
 *
 * \see RpGeometryGetUserDataArray
 * \see RpUserDataPluginAttach
 */
RwInt32
RpGeometryGetUserDataArrayCount(const RpGeometry * geometry)
{
    const RpUserDataList *userDataList;
    RwInt32 numElements;

    RWAPIFUNCTION(RWSTRING("RpGeometryGetUserDataArrayCount"));
    RWASSERT(geometry);

    userDataList = RPGEOMETRYGETCONSTUSERDATALIST(geometry);
    numElements = UserDataListGetNumElements(userDataList);

    RWRETURN(numElements);
}

/**
 * \ingroup rpuserdata
 * \ref RpGeometryGetUserDataArray is used to retrieve a \ref RpUserDataArray object
 * stored on a \ref RpGeometry.
 * The UserData plugin must be attached before using this function.
 *
 * \param geometry pointer to the geometry object
 * \param data the index of the user data array object to retrieve
 *
 * \return a pointer to the user data array object if it exists, NULL otherwise
 *
 * \see RpGeometryGetUserDataArrayCount
 * \see RpUserDataPluginAttach
 */
RpUserDataArray    *
RpGeometryGetUserDataArray(const RpGeometry * geometry, RwInt32 data)
{
    const RpUserDataList *userDataList;

    RWAPIFUNCTION(RWSTRING("RpGeometryGetUserDataArray"));
    RWASSERT(geometry);

    userDataList = RPGEOMETRYGETCONSTUSERDATALIST(geometry);

    if (data < userDataList->numElements)
    {
        RWRETURN(&(userDataList->userData[data]));
    }
    else
    {
        RWRETURN((RpUserDataArray *) NULL);
    }
}

/* World Sector API */

/**
 * \ingroup rpuserdata
 * \ref RpWorldSectorAddUserDataArray is used to allocate and add a new
 * \ref RpUserDataArray to a \ref RpWorldSector object.
 * The UserData plugin must be attached before using this function.
 *
 * \param sector pointer to the world sector object
 * \param name an identifier string for the the user data array
 * \param format the format of the user data array elements
 * \param numElements the number of elements on the user data array
 *
 * \return an index to the user data array on success, -1 on failure.
 *
 * \see RpWorldSectorGetUserDataArray
 * \see RpWorldSectorRemoveUserDataArray
 * \see RpUserDataPluginAttach
 */
RwInt32
RpWorldSectorAddUserDataArray(RpWorldSector * sector, RwChar * name,
                              RpUserDataFormat format,
                              RwInt32 numElements)
{
    RwInt32             index;
    RpUserDataList     *userDataList;

    RWAPIFUNCTION(RWSTRING("RpWorldSectorAddUserDataArray"));
    RWASSERT(sector);

    userDataList = RPWORLDSECTORGETUSERDATALIST(sector);

    index =
        UserDataListAddElement(userDataList, name, format, numElements);

    RWRETURN(index);
}

/**
 * \ingroup rpuserdata
 * \ref RpWorldSectorRemoveUserDataArray is used to remove and
 * deallocate a \ref RpUserDataArray added to a \ref RpWorldSector object
 * by \ref RpWorldSectorAddUserDataArray.
 *
 * After calling this function any indices used to reference userdata
 * arrays on this object should be treated as invalid and recalculated.
 *
 * The UserData plugin must be attached before using this function.
 *
 * \param sector  pointer to the world sector object
 * \param index   the index of the array to remove
 *
 * \return a pointer to the world sector on success, otherwise NULL
 *
 * \see RpWorldSectorGetUserDataArray
 * \see RpWorldSectorAddUserDataArray
 * \see RpUserDataPluginAttach
 */
RpWorldSector *
RpWorldSectorRemoveUserDataArray(RpWorldSector *sector, RwInt32 index)
{
    RpUserDataList     *userDataList;

    RWAPIFUNCTION(RWSTRING("RpWorldSectorRemoveUserDataArray"));
    RWASSERT(sector);

    userDataList = RPWORLDSECTORGETUSERDATALIST(sector);
    UserDataListRemoveElement(userDataList, index);

    RWRETURN(sector);
}

/**
 * \ingroup rpuserdata
 * \ref RpWorldSectorGetUserDataArrayCount is used to retrieve the number of
 * \ref RpUserDataArray stored with a \ref RpWorldSector.
 * The UserData plugin must be attached before using this function.
 *
 * \param sector pointer to the world sector object
 *
 * \return the number of user data arrays stored with the world sector on success, otherwise -1
 *
 * \see RpWorldSectorGetUserDataArray
 * \see RpUserDataPluginAttach
 */
RwInt32
RpWorldSectorGetUserDataArrayCount(const RpWorldSector * sector)
{
    const RpUserDataList *userDataList;
    RwInt32 numElements;

    RWAPIFUNCTION(RWSTRING("RpWorldSectorGetUserDataArrayCount"));
    RWASSERT(sector);

    userDataList = RPWORLDSECTORGETCONSTUSERDATALIST(sector);
    numElements = UserDataListGetNumElements(userDataList);

    RWRETURN(numElements);
}

/**
 * \ingroup rpuserdata
 * \ref RpWorldSectorGetUserDataArray is used to retrieve a
 * \ref RpUserDataArray object stored on a \ref RpWorldSector.
 * The UserData plugin must be attached before using this function.
 *
 * \param sector pointer to the world sector object
 * \param data the index of the user data array object to retrieve
 *
 * \return a pointer to the user data array object if it exists, NULL otherwise
 *
 * \see RpWorldSectorGetUserDataArrayCount
 * \see RpUserDataPluginAttach
 */
RpUserDataArray    *
RpWorldSectorGetUserDataArray(const RpWorldSector * sector,
                              RwInt32 data)
{
    const RpUserDataList *userDataList;

    RWAPIFUNCTION(RWSTRING("RpWorldSectorGetUserDataArray"));
    RWASSERT(sector);

    userDataList = RPWORLDSECTORGETCONSTUSERDATALIST(sector);

    if (data < userDataList->numElements)
    {
        RWRETURN(&(userDataList->userData[data]));
    }
    else
    {
        RWRETURN((RpUserDataArray *) NULL);
    }
}


/* Frame API */

/**
 * \ingroup rpuserdata
 * \ref RwFrameAddUserDataArray is used to allocate and add a new
 * \ref RpUserDataArray to a \ref RwFrame object.
 * The UserData plugin must be attached before using this function.
 *
 * \param frame pointer to the frame object
 * \param name an identifier string for the the user data array
 * \param format the format of the user data array elements
 * \param numElements the number of elements on the user data array
 *
 * \return an index to the user data array on success, -1 on failure.
 *
 * \see RwFrameGetUserDataArray
 * \see RwFrameRemoveUserDataArray
 * \see RpUserDataPluginAttach
 */
RwInt32
RwFrameAddUserDataArray(RwFrame * frame, RwChar * name,
                        RpUserDataFormat format,
                        RwInt32 numElements)
{
    RwInt32             index;
    RpUserDataList     *userDataList;

    RWAPIFUNCTION(RWSTRING("RwFrameAddUserDataArray"));
    RWASSERT(frame);

    userDataList = RWFRAMEGETUSERDATALIST(frame);

    index =
        UserDataListAddElement(userDataList, name, format, numElements);

    RWRETURN(index);
}

/**
 * \ingroup rpuserdata
 * \ref RwFrameRemoveUserDataArray is used to remove and
 * deallocate a \ref RpUserDataArray added to a \ref RwFrame object
 * by \ref RwFrameAddUserDataArray.
 *
 * After calling this function any indices used to reference userdata
 * arrays on this object should be treated as invalid and recalculated.
 *
 * The UserData plugin must be attached before using this function.
 *
 * \param frame  pointer to the frame object
 * \param index  the index of the array to remove
 *
 * \return a pointer to the frame on success, otherwise NULL
 *
 * \see RwFrameGetUserDataArray
 * \see RwFrameAddUserDataArray
 * \see RpUserDataPluginAttach
 */
RwFrame *
RwFrameRemoveUserDataArray(RwFrame *frame, RwInt32 index)
{
    RpUserDataList     *userDataList;

    RWAPIFUNCTION(RWSTRING("RwFrameRemoveUserDataArray"));
    RWASSERT(frame);

    userDataList = RWFRAMEGETUSERDATALIST(frame);
    UserDataListRemoveElement(userDataList, index);

    RWRETURN(frame);
}

/**
 * \ingroup rpuserdata
 * \ref RwFrameGetUserDataArrayCount is used to retrieve the number of
 * user data arrays stored with a \ref RwFrame.
 * The UserData plugin must be attached before using this function.
 *
 * \param frame pointer to the frame object
 *
 * \return the number of user data array stored with the frame on success, otherwise -1
 *
 * \see RwFrameGetUserDataArray
 * \see RpUserDataPluginAttach
 */
RwInt32
RwFrameGetUserDataArrayCount(const RwFrame * frame)
{
    const RpUserDataList *userDataList;
    RwInt32 numElements;

    RWAPIFUNCTION(RWSTRING("RwFrameGetUserDataArrayCount"));
    RWASSERT(frame);

    userDataList = RWFRAMEGETCONSTUSERDATALIST(frame);
    numElements = UserDataListGetNumElements(userDataList);

    RWRETURN(numElements);
}

/**
 * \ingroup rpuserdata
 * \ref RwFrameGetUserDataArray is used to retrieve a \ref RpUserDataArray
 * object stored on a \ref RwFrame.
 * The UserData plugin must be attached before using this function.
 *
 * \param frame pointer to the frame object
 * \param data the index of the user data array object to retrieve
 *
 * \return a pointer to the user data array object if it exists, NULL otherwise
 *
 * \see RwFrameGetUserDataArrayCount
 * \see RpUserDataPluginAttach
 */
RpUserDataArray    *
RwFrameGetUserDataArray(const RwFrame * frame,
                        RwInt32 data)
{
    const RpUserDataList *userDataList;

    RWAPIFUNCTION(RWSTRING("RwFrameGetUserDataArray"));
    RWASSERT(frame);

    userDataList = RWFRAMEGETCONSTUSERDATALIST(frame);

    if (data < userDataList->numElements)
    {
        RWRETURN(&(userDataList->userData[data]));
    }
    else
    {
        RWRETURN((RpUserDataArray *) NULL);
    }
}

/* Camera API */

/**
 * \ingroup rpuserdata
 * \ref RwCameraAddUserDataArray is used to allocate and add a new
 * \ref RpUserDataArray to a \ref RwCamera object.
 * The UserData plugin must be attached before using this function.
 *
 * \param camera pointer to the camera object
 * \param name an identifier string for the the user data array
 * \param format the format of the user data array elements
 * \param numElements the number of elements on the user data array
 *
 * \return an index to the user data array on success, -1 on failure.
 *
 * \see RwCameraGetUserDataArray
 * \see RwCameraRemoveUserDataArray
 * \see RpUserDataPluginAttach
 */
RwInt32
RwCameraAddUserDataArray(RwCamera * camera, RwChar * name,
                        RpUserDataFormat format,
                        RwInt32 numElements)
{
    RwInt32             index;
    RpUserDataList     *userDataList;

    RWAPIFUNCTION(RWSTRING("RwCameraAddUserDataArray"));
    RWASSERT(camera);

    userDataList = RWCAMERAGETUSERDATALIST(camera);

    index =
        UserDataListAddElement(userDataList, name, format, numElements);

    RWRETURN(index);
}

/**
 * \ingroup rpuserdata
 * \ref RwCameraRemoveUserDataArray is used to remove and
 * deallocate a \ref RpUserDataArray added to a \ref RwCamera object
 * by \ref RwCameraAddUserDataArray.
 *
 * After calling this function any indices used to reference userdata
 * arrays on this object should be treated as invalid and recalculated.
 *
 * The UserData plugin must be attached before using this function.
 *
 * \param camera  pointer to the camera object
 * \param index  the index of the array to remove
 *
 * \return a pointer to the camera on success, otherwise NULL
 *
 * \see RwCameraGetUserDataArray
 * \see RwCameraAddUserDataArray
 * \see RpUserDataPluginAttach
 */
RwCamera *
RwCameraRemoveUserDataArray(RwCamera *camera, RwInt32 index)
{
    RpUserDataList     *userDataList;

    RWAPIFUNCTION(RWSTRING("RwCameraRemoveUserDataArray"));
    RWASSERT(camera);

    userDataList = RWCAMERAGETUSERDATALIST(camera);
    UserDataListRemoveElement(userDataList, index);

    RWRETURN(camera);
}

/**
 * \ingroup rpuserdata
 * \ref RwCameraGetUserDataArrayCount is used to retrieve the number of
 * user data arrays stored with a \ref RwCamera.
 * The UserData plugin must be attached before using this function.
 *
 * \param camera pointer to the camera object
 *
 * \return the number of user data array stored with the camera on success, otherwise -1
 *
 * \see RwCameraGetUserDataArray
 * \see RpUserDataPluginAttach
 */
RwInt32
RwCameraGetUserDataArrayCount(const RwCamera * camera)
{
    const RpUserDataList *userDataList;
    RwInt32 numElements;

    RWAPIFUNCTION(RWSTRING("RwCameraGetUserDataArrayCount"));
    RWASSERT(camera);

    userDataList = RWCAMERAGETCONSTUSERDATALIST(camera);
    numElements = UserDataListGetNumElements(userDataList);

    RWRETURN(numElements);
}

/**
 * \ingroup rpuserdata
 * \ref RwCameraGetUserDataArray is used to retrieve a \ref RpUserDataArray
 * object stored on a \ref RwCamera.
 * The UserData plugin must be attached before using this function.
 *
 * \param camera pointer to the camera object
 * \param data the index of the user data array object to retrieve
 *
 * \return a pointer to the user data array object if it exists, NULL otherwise
 *
 * \see RwCameraGetUserDataArrayCount
 * \see RpUserDataPluginAttach
 */
RpUserDataArray    *
RwCameraGetUserDataArray(const RwCamera * camera,
                        RwInt32 data)
{
    const RpUserDataList *userDataList;

    RWAPIFUNCTION(RWSTRING("RwCameraGetUserDataArray"));
    RWASSERT(camera);

    userDataList = RWCAMERAGETCONSTUSERDATALIST(camera);

    if (data < userDataList->numElements)
    {
        RWRETURN(&(userDataList->userData[data]));
    }
    else
    {
        RWRETURN((RpUserDataArray *) NULL);
    }
}

/* Light API */

/**
 * \ingroup rpuserdata
 * \ref RpLightAddUserDataArray is used to allocate and add a new
 * \ref RpUserDataArray to a \ref RpLight object.
 * The UserData plugin must be attached before using this function.
 *
 * \param light pointer to the light object
 * \param name an identifier string for the the user data array
 * \param format the format of the user data array elements
 * \param numElements the number of elements on the user data array
 *
 * \return an index to the user data array on success, -1 on failure.
 *
 * \see RpLightGetUserDataArray
 * \see RpLightRemoveUserDataArray
 * \see RpUserDataPluginAttach
 */
RwInt32
RpLightAddUserDataArray(RpLight * light, RwChar * name,
                        RpUserDataFormat format,
                        RwInt32 numElements)
{
    RwInt32             index;
    RpUserDataList     *userDataList;

    RWAPIFUNCTION(RWSTRING("RpLightAddUserDataArray"));
    RWASSERT(light);

    userDataList = RPLIGHTGETUSERDATALIST(light);

    index =
        UserDataListAddElement(userDataList, name, format, numElements);

    RWRETURN(index);
}

/**
 * \ingroup rpuserdata
 * \ref RpLightRemoveUserDataArray is used to remove and
 * deallocate a \ref RpUserDataArray added to a \ref RpLight object
 * by \ref RpLightAddUserDataArray.
 *
 * After calling this function any indices used to reference userdata
 * arrays on this object should be treated as invalid and recalculated.
 *
 * The UserData plugin must be attached before using this function.
 *
 * \param light  pointer to the light object
 * \param index  the index of the array to remove
 *
 * \return a pointer to the camera on success, otherwise NULL
 *
 * \see RpLightGetUserDataArray
 * \see RpLightAddUserDataArray
 * \see RpUserDataPluginAttach
 */
RpLight *
RpLightRemoveUserDataArray(RpLight *light, RwInt32 index)
{
    RpUserDataList     *userDataList;

    RWAPIFUNCTION(RWSTRING("RpLightRemoveUserDataArray"));
    RWASSERT(light);

    userDataList = RPLIGHTGETUSERDATALIST(light);
    UserDataListRemoveElement(userDataList, index);

    RWRETURN(light);
}

/**
 * \ingroup rpuserdata
 * \ref RpLightGetUserDataArrayCount is used to retrieve the number of
 * user data arrays stored with a \ref RpLight.
 * The UserData plugin must be attached before using this function.
 *
 * \param light pointer to the light object
 *
 * \return the number of user data array stored with the light on success, otherwise -1
 *
 * \see RpLightGetUserDataArray
 * \see RpUserDataPluginAttach
 */
RwInt32
RpLightGetUserDataArrayCount(const RpLight * light)
{
    const RpUserDataList *userDataList;
    RwInt32 numElements;

    RWAPIFUNCTION(RWSTRING("RpLightGetUserDataArrayCount"));
    RWASSERT(light);

    userDataList = RPLIGHTGETCONSTUSERDATALIST(light);
    numElements = UserDataListGetNumElements(userDataList);

    RWRETURN(numElements);
}

/**
 * \ingroup rpuserdata
 * \ref RpLightGetUserDataArray is used to retrieve a \ref RpUserDataArray
 * object stored on a \ref RpLight.
 * The UserData plugin must be attached before using this function.
 *
 * \param light pointer to the light object
 * \param data the index of the user data array object to retrieve
 *
 * \return a pointer to the user data array object if it exists, NULL otherwise
 *
 * \see RpLightGetUserDataArrayCount
 * \see RpUserDataPluginAttach
 */
RpUserDataArray    *
RpLightGetUserDataArray(const RpLight * light,
                        RwInt32 data)
{
    const RpUserDataList *userDataList;

    RWAPIFUNCTION(RWSTRING("RpLightGetUserDataArray"));
    RWASSERT(light);

    userDataList = RPLIGHTGETCONSTUSERDATALIST(light);

    if (data < userDataList->numElements)
    {
        RWRETURN(&(userDataList->userData[data]));
    }
    else
    {
        RWRETURN((RpUserDataArray *) NULL);
    }
}

/* Material API */

/**
 * \ingroup rpuserdata
 * \ref RpMaterialAddUserDataArray is used to allocate and add a new
 * \ref RpUserDataArray to a \ref RpMaterial object.
 * The UserData plugin must be attached before using this function.
 *
 * \param material pointer to the material object
 * \param name an identifier string for the the user data array
 * \param format the format of the user data array elements
 * \param numElements the number of elements on the user data array
 *
 * \return an index to the user data array on success, -1 on failure.
 *
 * \see RpMaterialGetUserDataArray
 * \see RpMaterialRemoveUserDataArray
 * \see RpUserDataPluginAttach
 */
RwInt32
RpMaterialAddUserDataArray(RpMaterial * material, RwChar * name,
                        RpUserDataFormat format,
                        RwInt32 numElements)
{
    RwInt32             index;
    RpUserDataList     *userDataList;

    RWAPIFUNCTION(RWSTRING("RpMaterialAddUserDataArray"));
    RWASSERT(material);

    userDataList = RPMATERIALGETUSERDATALIST(material);

    index =
        UserDataListAddElement(userDataList, name, format, numElements);

    RWRETURN(index);
}

/**
 * \ingroup rpuserdata
 * \ref RpMaterialRemoveUserDataArray is used to remove and
 * deallocate a \ref RpUserDataArray added to a \ref RpMaterial object
 * by \ref RpMaterialAddUserDataArray.
 *
 * After calling this function any indices used to reference userdata
 * arrays on this object should be treated as invalid and recalculated.
 *
 * The UserData plugin must be attached before using this function.
 *
 * \param material  pointer to the material object
 * \param index  the index of the array to remove
 *
 * \return a pointer to the material on success, otherwise NULL
 *
 * \see RpMaterialGetUserDataArray
 * \see RpMaterialAddUserDataArray
 * \see RpUserDataPluginAttach
 */
RpMaterial *
RpMaterialRemoveUserDataArray(RpMaterial *material, RwInt32 index)
{
    RpUserDataList     *userDataList;

    RWAPIFUNCTION(RWSTRING("RpMaterialRemoveUserDataArray"));
    RWASSERT(material);

    userDataList = RPMATERIALGETUSERDATALIST(material);
    UserDataListRemoveElement(userDataList, index);

    RWRETURN(material);
}

/**
 * \ingroup rpuserdata
 * \ref RpMaterialGetUserDataArrayCount is used to retrieve the number of
 * user data arrays stored with a \ref RpMaterial.
 * The UserData plugin must be attached before using this function.
 *
 * \param material pointer to the material object
 *
 * \return the number of user data array stored with the material on success, otherwise -1
 *
 * \see RpMaterialGetUserDataArray
 * \see RpUserDataPluginAttach
 */
RwInt32
RpMaterialGetUserDataArrayCount(const RpMaterial * material)
{
    const RpUserDataList *userDataList;
    RwInt32 numElements;

    RWAPIFUNCTION(RWSTRING("RpMaterialGetUserDataArrayCount"));
    RWASSERT(material);

    userDataList = RPMATERIALGETCONSTUSERDATALIST(material);
    numElements = UserDataListGetNumElements(userDataList);

    RWRETURN(numElements);
}

/**
 * \ingroup rpuserdata
 * \ref RpMaterialGetUserDataArray is used to retrieve a \ref RpUserDataArray
 * object stored on a \ref RpMaterial.
 * The UserData plugin must be attached before using this function.
 *
 * \param material pointer to the material object
 * \param data the index of the user data array object to retrieve
 *
 * \return a pointer to the user data array object if it exists, NULL otherwise
 *
 * \see RpMaterialGetUserDataArrayCount
 * \see RpUserDataPluginAttach
 */
RpUserDataArray    *
RpMaterialGetUserDataArray(const RpMaterial * material,
                        RwInt32 data)
{
    const RpUserDataList *userDataList;

    RWAPIFUNCTION(RWSTRING("RpMaterialGetUserDataArray"));
    RWASSERT(material);

    userDataList = RPMATERIALGETCONSTUSERDATALIST(material);

    if (data < userDataList->numElements)
    {
        RWRETURN(&(userDataList->userData[data]));
    }
    else
    {
        RWRETURN((RpUserDataArray *) NULL);
    }
}

/* Texture API */

/**
 * \ingroup rpuserdata
 * \ref RwTextureAddUserDataArray is used to allocate and add a new
 * \ref RpUserDataArray to a \ref RwTexture object.
 * The UserData plugin must be attached before using this function.
 *
 * \param texture pointer to the texture object
 * \param name an identifier string for the the user data array
 * \param format the format of the user data array elements
 * \param numElements the number of elements on the user data array
 *
 * \return an index to the user data array on success, -1 on failure.
 *
 * \see RwTextureGetUserDataArray
 * \see RwTextureRemoveUserDataArray
 * \see RpUserDataPluginAttach
 */
RwInt32
RwTextureAddUserDataArray(RwTexture * texture, RwChar * name,
                        RpUserDataFormat format,
                        RwInt32 numElements)
{
    RwInt32             index;
    RpUserDataList     *userDataList;

    RWAPIFUNCTION(RWSTRING("RwTextureAddUserDataArray"));
    RWASSERT(texture);

    userDataList = RWTEXTUREGETUSERDATALIST(texture);

    index =
        UserDataListAddElement(userDataList, name, format, numElements);

    RWRETURN(index);
}

/**
 * \ingroup rpuserdata
 * \ref RwTextureRemoveUserDataArray is used to remove and
 * deallocate a \ref RpUserDataArray added to a \ref RwTexture object
 * by \ref RwTextureAddUserDataArray.
 *
 * After calling this function any indices used to reference userdata
 * arrays on this object should be treated as invalid and recalculated.
 *
 * The UserData plugin must be attached before using this function.
 *
 * \param texture  pointer to the texture object
 * \param index  the index of the array to remove
 *
 * \return a pointer to the texture on success, otherwise NULL
 *
 * \see RwTextureGetUserDataArray
 * \see RwTextureAddUserDataArray
 * \see RpUserDataPluginAttach
 */
RwTexture *
RwTextureRemoveUserDataArray(RwTexture *texture, RwInt32 index)
{
    RpUserDataList     *userDataList;

    RWAPIFUNCTION(RWSTRING("RwTextureRemoveUserDataArray"));
    RWASSERT(texture);

    userDataList = RWTEXTUREGETUSERDATALIST(texture);
    UserDataListRemoveElement(userDataList, index);

    RWRETURN(texture);
}

/**
 * \ingroup rpuserdata
 * \ref RwTextureGetUserDataArrayCount is used to retrieve the number of
 * user data arrays stored with a \ref RwTexture.
 * The UserData plugin must be attached before using this function.
 *
 * \param texture pointer to the texture object
 *
 * \return the number of user data array stored with the texture on success, otherwise -1
 *
 * \see RwTextureGetUserDataArray
 * \see RpUserDataPluginAttach
 */
RwInt32
RwTextureGetUserDataArrayCount(const RwTexture * texture)
{
    const RpUserDataList *userDataList;
    RwInt32 numElements;

    RWAPIFUNCTION(RWSTRING("RwTextureGetUserDataArrayCount"));
    RWASSERT(texture);

    userDataList = RWTEXTUREGETCONSTUSERDATALIST(texture);
    numElements = UserDataListGetNumElements(userDataList);

    RWRETURN(numElements);
}

/**
 * \ingroup rpuserdata
 * \ref RwTextureGetUserDataArray is used to retrieve a \ref RpUserDataArray
 * object stored on a \ref RwTexture.
 * The UserData plugin must be attached before using this function.
 *
 * \param texture pointer to the texture object
 * \param data the index of the user data array object to retrieve
 *
 * \return a pointer to the user data array object if it exists, NULL otherwise
 *
 * \see RwTextureGetUserDataArrayCount
 * \see RpUserDataPluginAttach
 */
RpUserDataArray    *
RwTextureGetUserDataArray(const RwTexture * texture,
                        RwInt32 data)
{
    const RpUserDataList *userDataList;

    RWAPIFUNCTION(RWSTRING("RwTextureGetUserDataArray"));
    RWASSERT(texture);

    userDataList = RWTEXTUREGETCONSTUSERDATALIST(texture);

    if (data < userDataList->numElements)
    {
        RWRETURN(&(userDataList->userData[data]));
    }
    else
    {
        RWRETURN((RpUserDataArray *) NULL);
    }
}


/* User Data Array API */

/**
 * \ingroup rpuserdata
 * \ref RpUserDataArrayGetName is used to retrieve the identifier string
 * attached to a a \ref RpUserDataArray object.
 * The UserData plugin must be attached before using this function.
 *
 * \param userData pointer to the user data array object
 *
 * \return the identifier string of the user data array object.
 *
 * \see RpGeometryGetUserDataArray
 * \see RpWorldSectorGetUserDataArray
 * \see RwFrameGetUserDataArray
 * \see RpUserDataPluginAttach
 */
RwChar             *
RpUserDataArrayGetName(RpUserDataArray * userData)
{
    RWAPIFUNCTION(RWSTRING("RpUserDataArrayGetName"));
    RWASSERT(userData);

    RWRETURN(userData->name);
}

/**
 * \ingroup rpuserdata
 * \ref RpUserDataArrayGetFormat is used to get format of the data elements
 * stored by a \ref RpUserDataArray object.
 * The UserData plugin must be attached before using this function.
 *
 * \param userData pointer to the user data array object
 *
 * \return the format of the elements stored by the user data array object.
 *
 * \see RpGeometryGetUserDataArray
 * \see RpWorldSectorGetUserDataArray
 * \see RwFrameGetUserDataArray
 * \see RpUserDataPluginAttach
 */
RpUserDataFormat
RpUserDataArrayGetFormat(RpUserDataArray * userData)
{
    RWAPIFUNCTION(RWSTRING("RpUserDataArrayGetFormat"));
    RWASSERT(userData);

    RWRETURN(userData->format);
}

/**
 * \ingroup rpuserdata
 * \ref RpUserDataArrayGetNumElements
 * is used to get the number of data elements
 * stored by a \ref RpUserDataArray object.
 * The UserData plugin must be attached before using this function.
 *
 * \param userData pointer to the user data array object
 *
 * \return the number of elements stored by the user data array object.
 *
 * \see RpGeometryGetUserDataArray
 * \see RpWorldSectorGetUserDataArray
 * \see RwFrameGetUserDataArray
 * \see RpUserDataPluginAttach
 */
RwInt32
RpUserDataArrayGetNumElements(RpUserDataArray * userData)
{
    RWAPIFUNCTION(RWSTRING("RpUserDataArrayGetNumElements"));
    RWASSERT(userData);

    RWRETURN(userData->numElements);
}

/**
 * \ingroup rpuserdata
 * \ref RpUserDataArrayGetInt is used to get the integer stored at entry
 * index in the \ref RpUserDataArray object.
 * The UserData plugin must be attached before using this function.
 *
 * \param userData poinnter to the user data array object
 * \param index the index in the data array to be retrieved
 *
 * \return the integer element
 *
 * \see RpGeometryGetUserDataArray
 * \see RpWorldSectorGetUserDataArray
 * \see RwFrameGetUserDataArray
 * \see RpUserDataArraySetInt
 * \see RpUserDataPluginAttach
 */
RwInt32
RpUserDataArrayGetInt(RpUserDataArray * userData, RwInt32 index)
{
    RWAPIFUNCTION(RWSTRING("RpUserDataArrayGetInt"));

    RWASSERT(userData);
    RWASSERT(index < userData->numElements);
    RWASSERT(userData->format == rpINTUSERDATA);

    RWRETURN(((RwInt32 *) userData->data)[index]);
}

/**
 * \ingroup rpuserdata
 * \ref RpUserDataArrayGetReal is used to get the real stored at entry
 * index in the \ref RpUserDataArray object.
 * The UserData plugin must be attached before using this function.
 *
 * \param userData pointer to the user data array object
 * \param index the index in the data array to be retrieved
 *
 * \return the real element
 *
 * \see RpGeometryGetUserDataArray
 * \see RpWorldSectorGetUserDataArray
 * \see RwFrameGetUserDataArray
 * \see RpUserDataArraySetReal
 * \see RpUserDataPluginAttach
 */
RwReal
RpUserDataArrayGetReal(RpUserDataArray * userData, RwInt32 index)
{
    RWAPIFUNCTION(RWSTRING("RpUserDataArrayGetReal"));

    RWASSERT(userData);
    RWASSERT(index < userData->numElements);
    RWASSERT(userData->format == rpREALUSERDATA);

    RWRETURN(((RwReal *) userData->data)[index]);
}

/**
 * \ingroup rpuserdata
 * \ref RpUserDataArrayGetString is used to get the string stored at entry
 * index in the \ref RpUserDataArray object.
 * The UserData plugin must be attached before using this function.
 *
 * \param userData pointer to the user data array object
 * \param index the index in the data array to be retrieved
 *
 * \return the string element
 *
 * \see RpGeometryGetUserDataArray
 * \see RpWorldSectorGetUserDataArray
 * \see RwFrameGetUserDataArray
 * \see RpUserDataArraySetString
 * \see RpUserDataPluginAttach
 */
RwChar             *
RpUserDataArrayGetString(RpUserDataArray * userData, RwInt32 index)
{
    RWAPIFUNCTION(RWSTRING("RpUserDataArrayGetString"));

    RWASSERT(userData);
    RWASSERT(index < userData->numElements);
    RWASSERT(userData->format == rpSTRINGUSERDATA);

    RWRETURN(((RwChar **) userData->data)[index]);
}

/**
 * \ingroup rpuserdata
 * \ref RpUserDataArraySetInt is used to set the integer stored at entry
 * index in the \ref RpUserDataArray object.
 * The UserData plugin must be attached before using this function.
 *
 * \param userData pointer to the user data array object
 * \param index the index in the data array to be stored
 * \param value the integer value to be stored
 *
 * \see RpGeometryGetUserDataArray
 * \see RpWorldSectorGetUserDataArray
 * \see RwFrameGetUserDataArray
 * \see RpUserDataArrayGetInt
 * \see RpUserDataPluginAttach
 */
void
RpUserDataArraySetInt(RpUserDataArray * userData, RwInt32 index,
                      RwInt32 value)
{
    RWAPIFUNCTION(RWSTRING("RpUserDataArraySetInt"));

    RWASSERT(userData);
    RWASSERT(index < userData->numElements);
    RWASSERT(userData->format == rpINTUSERDATA);

    ((RwInt32 *) userData->data)[index] = value;

    RWRETURNVOID();
}

/**
 * \ingroup rpuserdata
 * \ref RpUserDataArraySetReal is used to set the real stored at entry
 * index in the \ref RpUserDataArray object.
 * The UserData plugin must be attached before using this function.
 *
 * \param userData pointer to the user data array object
 * \param index the index in the data array to be stored
 * \param value the real value to be stored
 *
 * \see RpGeometryGetUserDataArray
 * \see RpWorldSectorGetUserDataArray
 * \see RwFrameGetUserDataArray
 * \see RpUserDataArrayGetReal
 * \see RpUserDataPluginAttach
 */
void
RpUserDataArraySetReal(RpUserDataArray * userData, RwInt32 index,
                       RwReal value)
{
    RWAPIFUNCTION(RWSTRING("RpUserDataArraySetReal"));

    RWASSERT(userData);
    RWASSERT(index < userData->numElements);
    RWASSERT(userData->format == rpREALUSERDATA);

    ((RwReal *) userData->data)[index] = value;

    RWRETURNVOID();
}

/**
 * \ingroup rpuserdata
 * \ref RpUserDataArraySetString is used to set the string stored at entry
 * index in the \ref RpUserDataArray object.
 * The UserData plugin must be attached before using this function.
 *
 * \param userData pointer to the user data array object
 * \param index the index in the data array to be stored
 * \param value the string value to be stored
 *
 * \see RpGeometryGetUserDataArray
 * \see RpWorldSectorGetUserDataArray
 * \see RwFrameGetUserDataArray
 * \see RpUserDataArrayGetReal
 * \see RpUserDataPluginAttach
 */
void
RpUserDataArraySetString(RpUserDataArray * userData,
                         RwInt32 index, RwChar * value)
{
    RWAPIFUNCTION(RWSTRING("RpUserDataArraySetString"));

    RWASSERT(userData);
    RWASSERT(index < userData->numElements);
    RWASSERT(userData->format == rpSTRINGUSERDATA);

    if (((RwChar **) userData->data)[index]!=(RwChar *)NULL)
    {
        RwFree(((RwChar **) userData->data)[index]);
    }

    if (value == NULL)
    {
        ((RwChar **) userData->data)[index] = (RwChar *)NULL;
    }
    else
    {
        rwstrdup(((RwChar **) userData->data)[index], value);
    }


    RWRETURNVOID();
}

/**
 * \ingroup rpuserdata
 * \ref RpUserDataGetFormatSize is used get the size (in bytes) of a given
 * UserData element format.
 * The UserData plugin must be attached before using this function.
 *
 * \param format the user data array format
 *
 * \return size of the user data array format in bytes.
 *
 * \see RpUserDataPluginAttach
 */
RwInt32
RpUserDataGetFormatSize(RpUserDataFormat format)
{
    RWAPIFUNCTION(RWSTRING("RpUserDataGetFormatSize"));

    switch (format)
    {
        case rpINTUSERDATA:
            RWRETURN(sizeof(RwInt32));
            break;
        case rpREALUSERDATA:
            RWRETURN(sizeof(RwReal));
            break;
        case rpSTRINGUSERDATA:
            RWRETURN(sizeof(RwChar *));
            break;
        default:
            RWRETURN(0);
            break;
    }
}

/* Plugin API */

/**
 * \ingroup rpuserdata
 * \ref RpUserDataPluginAttach is used to attach the UserData plugin to the
 * RenderWare system. The plugin must be attached between initializing the
 * system with \ref RwEngineInit and opening it with \ref RwEngineOpen.
 *
 * Note that the include file rpusrdat.h is required and must be included by
 * an application wishing to use this facility. The UserData library is
 * contained in the file rpusrdat.lib.
 *
 * \return Returns TRUE if successful, FALSE otherwise
 * an error.
 *
 * \see RpGeometryAddUserDataArray
 * \see RpWorldSectorAddUserDataArray
 * \see RwFrameAddUserDataArray
 * \see RpGeometryRemoveUserDataArray
 * \see RpWorldSectorRemoveUserDataArray
 * \see RwFrameRemoveUserDataArray
 * \see RpGeometryGetUserDataArray
 * \see RpWorldSectorGetUserDataArray
 * \see RwFrameGetUserDataArray
 *
 */
RwBool
RpUserDataPluginAttach(void)
{
    RwInt32             offset;

    RWAPIFUNCTION(RWSTRING("RpUserDataPluginAttach"));

    /* Register Engine plugin */
    offset =
        RwEngineRegisterPlugin(0, rwID_USERDATAPLUGIN, UserDataOpen,
                               UserDataClose);
    if (offset < 0)
    {
        RWRETURN(FALSE);
    }

    /* Register RpGeometry plugin */
    userDataGeometryOffset =
        RpGeometryRegisterPlugin(sizeof(RpUserDataList),
                                 rwID_USERDATAPLUGIN,
                                 UserDataObjectConstruct,
                                 UserDataObjectDestruct,
                                 UserDataObjectCopy);

    if (userDataGeometryOffset < 0)
    {
        RWRETURN(FALSE);
    }

    userDataGeometryStreamOffset =
        RpGeometryRegisterPluginStream(rwID_USERDATAPLUGIN,
                                       UserDataObjectStreamRead,
                                       UserDataObjectStreamWrite,
                                       UserDataObjectGetSize);

    if (userDataGeometryStreamOffset < 0)
    {
        RWRETURN(FALSE);
    }

    /* Register RpWorldSector plugin */
    userDataWorldSectorOffset =
        RpWorldSectorRegisterPlugin(sizeof(RpUserDataList),
                                    rwID_USERDATAPLUGIN,
                                    UserDataObjectConstruct,
                                    UserDataObjectDestruct,
                                    UserDataObjectCopy);

    if (userDataGeometryOffset < 0)
    {
        RWRETURN(FALSE);
    }

    userDataWorldSectorStreamOffset =
        RpWorldSectorRegisterPluginStream(rwID_USERDATAPLUGIN,
                                          UserDataObjectStreamRead,
                                          UserDataObjectStreamWrite,
                                          UserDataObjectGetSize);

    if (userDataGeometryStreamOffset < 0)
    {
        RWRETURN(FALSE);
    }

    /* Register RwFrame plugin */
    userDataFrameOffset =
        RwFrameRegisterPlugin(sizeof(RpUserDataList),
                                rwID_USERDATAPLUGIN,
                                UserDataObjectConstruct,
                                UserDataObjectDestruct,
                                UserDataObjectCopy);

    if (userDataFrameOffset < 0)
    {
        RWRETURN(FALSE);
    }

    userDataFrameStreamOffset =
        RwFrameRegisterPluginStream(rwID_USERDATAPLUGIN,
                                    UserDataObjectStreamRead,
                                    UserDataObjectStreamWrite,
                                    UserDataObjectGetSize);

    if (userDataFrameStreamOffset < 0)
    {
        RWRETURN(FALSE);
    }

    /* Register RwCamera plugin */
    userDataCameraOffset =
        RwCameraRegisterPlugin(sizeof(RpUserDataList),
                                rwID_USERDATAPLUGIN,
                                UserDataObjectConstruct,
                                UserDataObjectDestruct,
                                UserDataObjectCopy);

    if (userDataCameraOffset < 0)
    {
        RWRETURN(FALSE);
    }

    userDataCameraStreamOffset =
        RwCameraRegisterPluginStream(rwID_USERDATAPLUGIN,
                                    UserDataObjectStreamRead,
                                    UserDataObjectStreamWrite,
                                    UserDataObjectGetSize);

    if (userDataCameraStreamOffset < 0)
    {
        RWRETURN(FALSE);
    }

        /* Register RpLight plugin */
    userDataLightOffset =
        RpLightRegisterPlugin(sizeof(RpUserDataList),
                                rwID_USERDATAPLUGIN,
                                UserDataObjectConstruct,
                                UserDataObjectDestruct,
                                UserDataObjectCopy);

    if (userDataLightOffset < 0)
    {
        RWRETURN(FALSE);
    }

    userDataLightStreamOffset =
        RpLightRegisterPluginStream(rwID_USERDATAPLUGIN,
                                    UserDataObjectStreamRead,
                                    UserDataObjectStreamWrite,
                                    UserDataObjectGetSize);

    if (userDataLightStreamOffset < 0)
    {
        RWRETURN(FALSE);
    }

        /* Register RpMaterial plugin */
    userDataMaterialOffset =
       RpMaterialRegisterPlugin(sizeof(RpUserDataList),
                                rwID_USERDATAPLUGIN,
                                UserDataObjectConstruct,
                                UserDataObjectDestruct,
                                UserDataObjectCopy);

    if (userDataMaterialOffset < 0)
    {
        RWRETURN(FALSE);
    }

    userDataMaterialStreamOffset =
        RpMaterialRegisterPluginStream(rwID_USERDATAPLUGIN,
                                    UserDataObjectStreamRead,
                                    UserDataObjectStreamWrite,
                                    UserDataObjectGetSize);

    if (userDataMaterialStreamOffset < 0)
    {
        RWRETURN(FALSE);
    }

        /* Register RwTexture plugin */
    userDataTextureOffset =
        RwTextureRegisterPlugin(sizeof(RpUserDataList),
                                rwID_USERDATAPLUGIN,
                                UserDataObjectConstruct,
                                UserDataObjectDestruct,
                                UserDataObjectCopy);

    if (userDataTextureOffset < 0)
    {
        RWRETURN(FALSE);
    }

    userDataTextureStreamOffset =
        RwTextureRegisterPluginStream(rwID_USERDATAPLUGIN,
                                    UserDataObjectStreamRead,
                                    UserDataObjectStreamWrite,
                                    UserDataObjectGetSize);

    if (userDataTextureStreamOffset < 0)
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}
