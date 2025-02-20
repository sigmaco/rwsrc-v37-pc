/***************************************************************************
 *                                                                         *
 * Module  :                                                               *
 *                                                                         *
 * Purpose :                                                               *
 *                                                                         *
 **************************************************************************/

/****************************************************************************
 Includes
 */
#include <stdlib.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "skin.h"
#include "bsplit.h"

/****************************************************************************
 Defines
 */

#define SkinSplitDataSize(_sk, _gd) \
        ((sizeof(RwUInt8) * (_sk)->boneData.numBones) + \
         (sizeof(RwUInt8) * (_gd)->numMeshes * 2) + \
         (sizeof(RwUInt8) * (_gd)->numRLE * 2))

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */

/****************************************************************************
 _rpSkinGetMeshBoneRemapIndices
 */
RwUInt8 *
_rpSkinGetMeshBoneRemapIndices(RpSkin *skin)
{
    RWFUNCTION(RWSTRING("_rpSkinGetMeshBoneRemapIndices"));

    RWASSERT(0 < _rpSkinGlobals.module.numInstances);
    RWASSERT(NULL != skin);

    RWRETURN(skin->skinSplitData.matrixRemapIndices);
}

/****************************************************************************
 _rpSkinGetMeshBoneRLECount
 */
RwUInt8 *
_rpSkinGetMeshBoneRLECount(RpSkin *skin)
{
    RWFUNCTION(RWSTRING("_rpSkinGetMeshBoneRLECount"));

    RWASSERT(0 < _rpSkinGlobals.module.numInstances);
    RWASSERT(NULL != skin);

    RWRETURN(skin->skinSplitData.meshRLECount);
}

/****************************************************************************
 _rpSkinGetMeshBoneRLE
 */
RwUInt8 *
_rpSkinGetMeshBoneRLE(RpSkin *skin)
{
    RWFUNCTION(RWSTRING("_rpSkinGetMeshBoneRLE"));

    RWASSERT(0 < _rpSkinGlobals.module.numInstances);
    RWASSERT(NULL != skin);

    RWRETURN(skin->skinSplitData.meshRLE);
}

/****************************************************************************
 _rpSkinSplitDataCreate
 */
RpSkin *
_rpSkinSplitDataCreate(RpSkin *skin,
                           RwUInt32 boneLimit,
                           RwUInt32 numMatrices,
                           RwUInt32 numMeshes,
                           RwUInt32 numRLE)
{
    RwUInt32            size;
    SkinSplitData       *skinSplitData;

    RWFUNCTION(RWSTRING("_rpSkinSplitDataCreate"));

    skinSplitData = &skin->skinSplitData;

    /* Destroy any existing bone split data */
    _rpSkinSplitDataDestroy(skin);

    size = (sizeof(RwUInt8) * numMatrices) +
           (sizeof(RwUInt8) * numMeshes * 2) +
           (sizeof(RwUInt8) * numRLE * 2);

    skinSplitData->matrixRemapIndices = (RwUInt8 *)
        RwMalloc(size, rwID_SKINPLUGIN | rwMEMHINTDUR_EVENT);

    if (skinSplitData->matrixRemapIndices == NULL)
        RWRETURN(NULL);

    memset(skinSplitData->matrixRemapIndices, 0, size);

    skinSplitData->boneLimit = boneLimit;
    skinSplitData->numMeshes = numMeshes;
    skinSplitData->numRLE = numRLE;

    /* Initialise the data */

    skinSplitData->meshRLECount = (RwUInt8 *)
        (skinSplitData->matrixRemapIndices + numMatrices);
    skinSplitData->meshRLE = (RwUInt8 *)
        (skinSplitData->meshRLECount + (numMeshes * 2));

    RWRETURN(skin);
}

/****************************************************************************
 _rpSkinSplitDataDestroy
 */
RwBool
_rpSkinSplitDataDestroy(RpSkin *skin)
{
    SkinSplitData   *skinSplitData;

    RWFUNCTION(RWSTRING("_rpSkinSplitDataDestroy"));

    skinSplitData = &skin->skinSplitData;

    if (skinSplitData->matrixRemapIndices != NULL)
    {
        RwFree(skinSplitData->matrixRemapIndices);
    }

    skinSplitData->boneLimit = 0;
    skinSplitData->numMeshes = 0;
    skinSplitData->numRLE = 0;

    skinSplitData->matrixRemapIndices = NULL;
    skinSplitData->meshRLECount = NULL;
    skinSplitData->meshRLE = NULL;

    RWRETURN (TRUE);
}

/****************************************************************************
 _rpSkinSplitDataStreamWrite
 */
RwStream *
_rpSkinSplitDataStreamWrite(RwStream *stream,
                                const RpSkin *skin)
{
    RwInt32                 streamSize;
    const SkinSplitData     *skinSplitData;

    RWFUNCTION(RWSTRING("_rpSkinSplitDataStreamWrite"));

    skinSplitData = &skin->skinSplitData;

    /* Write out the counters */
    if (!RwStreamWriteInt(stream,
            (const RwInt32 *)&skinSplitData->boneLimit, sizeof(RwInt32)))
    {
        RWRETURN ((RwStream *) NULL);
    }

    if (!RwStreamWriteInt(stream,
            (const RwInt32 *)&skinSplitData->numMeshes, sizeof(RwInt32)))
    {
        RWRETURN ((RwStream *) NULL);
    }

    if (!RwStreamWriteInt(stream,
            (const RwInt32 *)&skinSplitData->numRLE, sizeof(RwInt32)))
    {
        RWRETURN ((RwStream *) NULL);
    }

    if (0 < skin->skinSplitData.numMeshes)
    {
        streamSize = SkinSplitDataSize(skin, skinSplitData);

        /* Write out the index array */
        if (!RwStreamWrite(stream,
                skinSplitData->matrixRemapIndices, streamSize))
        {
            RWRETURN ((RwStream *) NULL);
        }
    }

    RWRETURN (stream);
}

/****************************************************************************
 _rpSkinSplitDataStreamRead
 */
RwStream *
_rpSkinSplitDataStreamRead(RwStream *stream,
                               RpSkin *skin)
{
    RwInt32             numMeshes, numRLE, boneLimit, streamSize;
    SkinSplitData       *skinSplitData;

    RWFUNCTION(RWSTRING("_rpSkinSplitDataStreamRead"));

    skinSplitData = &skin->skinSplitData;

    /* Read num meshes and num bones */
    if (!RwStreamReadInt(
            stream, (RwInt32 *)&boneLimit, sizeof(RwInt32)))
    {
        RWRETURN((RwStream *)NULL);
    }

    if (!RwStreamReadInt(
            stream, (RwInt32 *)&numMeshes, sizeof(RwInt32)))
    {
        RWRETURN((RwStream *)NULL);
    }

    if (!RwStreamReadInt(
            stream, (RwInt32 *)&numRLE, sizeof(RwInt32)))
    {
        RWRETURN((RwStream *)NULL);
    }

    if (numMeshes > 0)
    {
        if (!_rpSkinSplitDataCreate(skin, boneLimit,
                skin->boneData.numBones, numMeshes, numRLE))
        {
            RWRETURN(NULL);
        }

        streamSize = SkinSplitDataSize(skin, skinSplitData);

        /* Read in the index array */
        if (!RwStreamRead(stream,
                skinSplitData->matrixRemapIndices, streamSize))
        {
            RwFree(skinSplitData);

            RWRETURN((RwStream *) NULL);
        }
    }

    RWRETURN (stream);
}

/****************************************************************************
 _rpSkinSplitDataStreamGetSize
 */
RwInt32
_rpSkinSplitDataStreamGetSize(const RpSkin *skin)
{
    RwUInt32    streamSize;

    RWFUNCTION(RWSTRING("_rpSkinSplitDataStreamGetSize"));

    /* boneLimit, numMeshes, numRLE */
    streamSize = sizeof(RwUInt32) * 3;

    if (0 < skin->skinSplitData.numMeshes)
    {
        /* RLE data */
        streamSize += SkinSplitDataSize(skin, &skin->skinSplitData);
    }

    RWRETURN(streamSize);
}

#if (0)
/****************************************************************************
 _rpSkinSplitDataCopy
 */
void *
_rpSkinSplitDataCopy(void *dstObject,
                         const void *srcObject,
                         RwInt32 offset,
                         RwInt32 size)
{
    RwUInt32                dataSize;
    SkinSplitData       *skinSplitData, *skinSplitDataNew;

    RWFUNCTION(RWSTRING("_rpSkinSplitDataCopy"));

    if (skinSplitData != NULL)
    {
        skinSplitDataNew =
            _rpSkinSplitDataCreate(
                skinSplitData->numMatrices,
                skinSplitData->numMeshes,
                skinSplitData->numRLE);

        if (skinSplitDataNew == NULL)
            RWRETURN (NULL);

        dataSize = SkinSplitDataSize(skinSplitData);

        memcpy((skinSplitDataNew + 1), (skinSplitData + 1), dataSize);
    }

    RWRETURN (dstObject);
}

#endif /* (0) */
