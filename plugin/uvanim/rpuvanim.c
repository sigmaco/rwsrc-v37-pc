
/*****************************************************************************
 *
 * File :     rpuvanim.c
 *
 * Abstract : UV Animation plugin
 *
 * See Also : rwsdk/plugin/anim
 *
 *****************************************************************************
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
 * Copyright (c) 2000 Criterion Software Ltd.
 * All Rights Reserved.
 *
 * RenderWare is a trademark of Canon Inc.
 *
 *****************************************************************************/

/**
 * \ingroup rpuvanim
 * \page rpuvanimoverview RpUVAnim Plugin Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rtanim.h, rtdict.h, rpuvanim.h
 * \li \b Libraries: rwcore, rpworld, rtanim, rtdict, rpuvanim
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpUVAnimPluginAttach
 *
 * \subsection uvanimoverview Overview
 * This plugin provides facilities for UV animation.
 *
 * Materials are extended with an optional link to a UV animation. These
 * animations may be stored in a dictionary.
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
#include <rpuvanim.h>
#include <rpmatfx.h>
#include <rtdict.h>


/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/
enum MaterialExtStreamFlags
{
    /* Note flag=pow(2, channel) is assumed by streaming fns */
    MaterialExtHasChannel0Anim = 0x01,
    MaterialExtHasChannel1Anim = 0x02,
    MaterialExtHasChannel2Anim = 0x04,
    MaterialExtHasChannel3Anim = 0x08,
    MaterialExtHasChannel4Anim = 0x10,
    MaterialExtHasChannel5Anim = 0x20,
    MaterialExtHasChannel6Anim = 0x40,
    MaterialExtHasChannel7Anim = 0x80,
    MaterialExtStreamFlagsSizeInt = RWFORCEENUMSIZEINT
};

typedef enum MaterialExtStreamFlags MaterialExtStreamFlags;

/*===========================================================================*
 *--- Public Global Variables -----------------------------------------------*
 *===========================================================================*/
RwModuleInfo rpUVAnimModule = {0, 0};
RpUVAnimMaterialGlobalVars RpUVAnimMaterialGlobals;

static RwInt32 _rpUVAnimCustomDataFreeListBlockSize = 128,
               _rpUVAnimCustomDataFreeListPreallocBlocks = 1;
static RwFreeList *_rpUVAnimCustomDataFreeList;

const RwV3d rpUVAnimPivot = { -0.5f, -0.5f, 0.0f };
const RwV3d rpUVAnimInvPivot = { 0.5f, 0.5f, 0.0f };

RtDictSchema _rpUVAnimDictSchema
   = {
        "UVAnim",
        rwID_UVANIMDICT,
        rwID_ANIMANIMATION,
        rwLIBRARYVERSION36000,
        NULL, /* dictionaries */
        NULL, /* current */
        (RtDictEntryAddRefCallBack *)&RpUVAnimAddRef,
        (RtDictEntryDestroyCallBack *)&RpUVAnimDestroy,
        (RtDictEntryGetNameCallBack *)&RpUVAnimGetName,
        (RtDictEntryStreamGetSizeCallBack *)&RtAnimAnimationStreamGetSize,
        (RtDictEntryStreamReadCallBack *)&RtAnimAnimationStreamRead,
        NULL, /* streamReadCompatibilityCB */
        (RtDictEntryStreamWriteCallBack *)RtAnimAnimationStreamWrite
   };

RtAnimInterpolatorInfo _rpUVAnimLinearInterpolatorInfo
    = {
        rwID_UVANIMLINEAR,             /* typeID */
        sizeof(RpUVAnimKeyFrame),      /* interpKeyFrameSize */
        sizeof(RpUVAnimKeyFrame),      /* animKeyFrameSize */
        RpUVAnimLinearKeyFrameApply,         /* keyFrameApplyCB */
        RpUVAnimLinearKeyFrameBlend,         /* keyFrameBlendCB */
        RpUVAnimLinearKeyFrameInterpolate,   /* keyFrameInterpolateCB */
        RpUVAnimLinearKeyFrameAdd,           /* keyFrameAddCB */
        RpUVAnimLinearKeyFrameMulRecip,      /* keyFrameMulRecipCB */
        RpUVAnimKeyFrameStreamRead,    /* keyFrameStreamReadCB */
        RpUVAnimKeyFrameStreamWrite,   /* keyFrameStreamWriteCB */
        RpUVAnimKeyFrameStreamGetSize, /* keyFrameStreamGetSizeCB */
        sizeof(_rpUVAnimCustomData),   /* customDataSize */
    };

RtAnimInterpolatorInfo _rpUVAnimParamInterpolatorInfo
    = {
        rwID_UVANIMPARAM,              /* typeID */
        sizeof(RpUVAnimKeyFrame),      /* interpKeyFrameSize */
        sizeof(RpUVAnimKeyFrame),      /* animKeyFrameSize */
        RpUVAnimParamKeyFrameApply,         /* keyFrameApplyCB */
        RpUVAnimParamKeyFrameBlend,         /* keyFrameBlendCB */
        RpUVAnimParamKeyFrameInterpolate,   /* keyFrameInterpolateCB */
        RpUVAnimParamKeyFrameAdd,           /* keyFrameAddCB */
        RpUVAnimParamKeyFrameMulRecip,      /* keyFrameMulRecipCB */
        RpUVAnimKeyFrameStreamRead,    /* keyFrameStreamReadCB */
        RpUVAnimKeyFrameStreamWrite,   /* keyFrameStreamWriteCB */
        RpUVAnimKeyFrameStreamGetSize, /* keyFrameStreamGetSizeCB */
        sizeof(_rpUVAnimCustomData),   /* customDataSize */
    };

/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/

#define RPUVANIMMATERIALGETDATA(material)                  \
    ( (void *)(((RwUInt8 *)material) +                     \
               RpUVAnimMaterialGlobals.engineOffset) )

#define RPUVANIMMATERIALGETCONSTDATA(material)                    \
    ( (const void *)(((const RwUInt8 *)material) +            \
                     RpUVAnimMaterialGlobals.engineOffset) )

/*===========================================================================*
 *--- External functions ----------------------------------------------------*
 *===========================================================================*/


/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

static void         *
UVAnimConstructor(void *object,
                 RwInt32 offset,
                 RwInt32 size);

static void        *
UVAnimDestructor(void *object,
                RwInt32 offset,
                RwInt32 size);

static void        *
UVAnimCopy(void *dstObject,
          const void * srcObject,
          RwInt32 offset,
          RwInt32 size);

static RwStream    *
UVAnimWrite(RwStream * stream,
           RwInt32 binaryLength,
           const void *object,
           RwInt32 offsetInObject,
           RwInt32 sizeInObject);

static RwStream    *
UVAnimRead(RwStream * stream,
          RwInt32 binaryLength,
          void *object,
          RwInt32 offsetInObject,
          RwInt32 sizeInObject);

static              RwInt32
UVAnimSize(const void *object,
          RwInt32 offsetInObject,
          RwInt32 sizeInObject);



/*--- UVAnimOpen -----------------------------------------------------------
 */
static void        *
UVAnimOpen(void *instance,
          RwInt32 offset __RWUNUSED__,
          RwInt32 size __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("UVAnimOpen"));
    RWASSERT(instance);

    _rpUVAnimCustomDataFreeList =
            RwFreeListCreateAndPreallocateSpace(
                sizeof(_rpUVAnimCustomData),
                _rpUVAnimCustomDataFreeListBlockSize, sizeof(RwInt32),
                _rpUVAnimCustomDataFreeListPreallocBlocks, _rpUVAnimCustomDataFreeList,
                rwID_UVANIMPLUGIN | rwMEMHINTDUR_GLOBAL);

    if (NULL == _rpUVAnimCustomDataFreeList)
    {
        instance = NULL;
    }

    /* setup the dictionary schema */
    RtDictSchemaInit(&_rpUVAnimDictSchema);

    RtAnimRegisterInterpolationScheme(&_rpUVAnimLinearInterpolatorInfo);
    RtAnimRegisterInterpolationScheme(&_rpUVAnimParamInterpolatorInfo);

    ++rpUVAnimModule.numInstances;

    RWRETURN(instance);
}

static void        *
UVAnimClose(void *instance,
           RwInt32 offset __RWUNUSED__,
           RwInt32 size __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("UVAnimClose"));
    RWASSERT(instance);

    --rpUVAnimModule.numInstances;

    RtDictSchemaDestruct(&_rpUVAnimDictSchema);

    if (NULL != _rpUVAnimCustomDataFreeList)
    {
        RwFreeListDestroy(_rpUVAnimCustomDataFreeList);
        _rpUVAnimCustomDataFreeList = (RwFreeList *)NULL;
    }

    RWRETURN(instance);
}


/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rpuvanim
 * \ref RpUVAnimPluginAttach is called by the application
 * to indicate that the UV animation plugin should be used.
 * The call to this function should be placed between
 * \ref RwEngineInit and \ref RwEngineOpen.
 *
 * \return True on success, false otherwise
 *
 * \see RpWorldPluginAttach
 */

RwBool
RpUVAnimPluginAttach(void)
{
    RwInt32             success = 0;
    RWAPIFUNCTION(RWSTRING("RpUVAnimPluginAttach"));
    RWASSERT(rpUVAnimModule.numInstances == 0);

    /* Register the plugIn */
    if (RwEngineRegisterPlugin(0,
                               rwID_UVANIMPLUGIN,
                               UVAnimOpen, UVAnimClose) < 0)
    {
        RWRETURN(FALSE);
    }

    /* Extend RpMaterial to hold uvanim ptrs */
    RpUVAnimMaterialGlobals.engineOffset =
        RpMaterialRegisterPlugin(sizeof(RpUVAnimMaterialExtension),
                              rwID_UVANIMPLUGIN,
                              UVAnimConstructor, UVAnimDestructor,
                              UVAnimCopy);

    /* Attach the stream handling functions */
    success =
        RpMaterialRegisterPluginStream(rwID_UVANIMPLUGIN,
                                    UVAnimRead, UVAnimWrite, UVAnimSize);

    RWRETURN((success >= 0) && (RpUVAnimMaterialGlobals.engineOffset >= 0));
}


/**
 * \ingroup uvanim
 * \ref RpUVAnimSetFreeListCreateParams allows the developer to specify
 * how many \ref RpUVAnim s to preallocate space for.
 * Call before \ref RwEngineInit.
 *
 * \param blockSize  number of entries per freelist block.
 * \param numBlocksToPrealloc  number of blocks to allocate on
 * \ref RwFreeListCreateAndPreallocateSpace.
 *
 * \see RwFreeList
 *
 */
void
RpUVAnimSetFreeListCreateParams(RwInt32 blockSize,
                                RwInt32 numBlocksToPrealloc)
{
#if 0
    /*
     * Can not use debugging macros since the debugger is not initialized before
     * RwEngineInit.
     */
    /* Do NOT comment out RWAPIFUNCTION as gnumake verify will not function */
    RWAPIFUNCTION(RWSTRING("RpUVAnimSetFreeListCreateParams"));
#endif
    _rpUVAnimCustomDataFreeListBlockSize = blockSize;
    _rpUVAnimCustomDataFreeListPreallocBlocks = numBlocksToPrealloc;
#if 0
    RWRETURNVOID();
#endif
}

/*----------------------------------------------------------------------*/

/*                            - MATERIAL -                              */

/*----------------------------------------------------------------------*/

static void        *
UVAnimConstructor(void *object,
                 RwInt32 offset __RWUNUSED__,
                 RwInt32 size __RWUNUSED__)
{
    RpUVAnimMaterialExtension *materialExt;
    RWFUNCTION(RWSTRING("UVAnimConstructor"));
    RWASSERT(object);

    materialExt = (RpUVAnimMaterialExtension *)RPUVANIMMATERIALGETDATA(object);

    memset(materialExt, 0, sizeof(RpUVAnimMaterialExtension));

    RWRETURN(object);
}

static void        *
UVAnimDestructor(void *object,
                RwInt32 offset __RWUNUSED__,
                RwInt32 size __RWUNUSED__)
{
    RpUVAnimMaterialExtension *materialExt;
    size_t i;

    RWFUNCTION(RWSTRING("UVAnimDestructor"));
    RWASSERT(object);

    materialExt = (RpUVAnimMaterialExtension *)RPUVANIMMATERIALGETDATA(object);

    for (i=0; i<RP_UVANIM_APPLIEDCHANNELS; ++i)
    {
        RwMatrix **matrix = &materialExt->uv[i];
        if (*matrix)
        {
            RwMatrixDestroy(*matrix);
        }
        *matrix = (RwMatrix *)NULL;
    }

    for (i=0; i<RP_UVANIM_MAXSLOTS; ++i)
    {
        RtAnimInterpolator *interp = materialExt->interp[i];
        if (interp)
        {
            RtAnimAnimation *anim = RtAnimInterpolatorGetCurrentAnim(interp);
            RpUVAnimDestroy(anim);

            RtAnimInterpolatorDestroy(interp);
        }
    }

    RWRETURN(object);
}

static void        *
UVAnimCopy(void *dstObject,
          const void * srcObject,
          RwInt32 offset __RWUNUSED__,
          RwInt32 size __RWUNUSED__)
{
    const RpUVAnimMaterialExtension *srcMaterialExt;
    RpUVAnimMaterialExtension *dstMaterialExt;

    RWFUNCTION(RWSTRING("UVAnimCopy"));
    RWASSERT(srcObject);
    RWASSERT(dstObject);

    srcMaterialExt = (const RpUVAnimMaterialExtension *)
        RPUVANIMMATERIALGETCONSTDATA(srcObject);
    dstMaterialExt = (RpUVAnimMaterialExtension *)
        RPUVANIMMATERIALGETDATA(dstObject);

    /* Allocate and copy into allocated interpolators,
     * incrementing anim references where necessary */
    {
        size_t allocateIndex;
        RwBool allocSuccess = TRUE;

        for (allocateIndex=0; allocateIndex<RP_UVANIM_MAXSLOTS; ++allocateIndex)
        {
            RtAnimInterpolator *interp = srcMaterialExt->interp[allocateIndex];
            RtAnimInterpolator **dest = &dstMaterialExt->interp[allocateIndex];

            if (interp)
            {
                RtAnimAnimation *anim = RtAnimInterpolatorGetCurrentAnim(interp);
                *dest = RtAnimInterpolatorCreate(
                            RtAnimAnimationGetNumNodes(anim),
                            anim->interpInfo->interpKeyFrameSize
                        );
                if (*dest)
                {
                    RtAnimInterpolatorCopy(*dest, interp);
                    RpUVAnimAddRef(anim);
                }
                else
                {
                    allocSuccess = FALSE;
                    break;
                }
            }
            else
            {
                *dest = (RtAnimInterpolator *)NULL;
            }
        }

        /* Cleanup on failure */
        if (!allocSuccess)
        {
            size_t cleanupIndex;
            for (cleanupIndex=0; cleanupIndex<allocateIndex; ++cleanupIndex)
            {
                RtAnimInterpolator **dest = &dstMaterialExt->interp[cleanupIndex];

                if (*dest)
                {
                    RtAnimAnimation *anim = RtAnimInterpolatorGetCurrentAnim(*dest);
                    RpUVAnimDestroy(anim);
                    RtAnimInterpolatorDestroy(*dest);
                }
            }
            RWRETURN((void *)NULL);
        }
    }

    RWRETURN(dstObject);
}

static RpUVAnimMaterialExtension *
_rpUVAnimCreateConsistantMatrices(RpUVAnimMaterialExtension *matExt)
{
    RwUInt32 channel;

    RWFUNCTION(RWSTRING("_rpUVAnimCreateConsistantMatrices"));
    RWASSERT(matExt);

    for(channel=0; channel<RP_UVANIM_APPLIEDCHANNELS; ++channel)
    {
        RwUInt32 slot=0;
        RwUInt32 node=0;
        RwMatrix **matrix=&matExt->uv[channel];
        RwBool needAMatrix=FALSE;

        /* Look for a slot that uses this channel */
        while (slot<RP_UVANIM_MAXSLOTS)
        {
            RpUVAnimInterpolator *interp = matExt->interp[slot];

            if (interp)
            {
                RpUVAnim* anim=RtAnimInterpolatorGetCurrentAnim(interp);

                if (channel==RpUVAnimGetUVNodeToChannelMap(anim)[node])
                {
                    needAMatrix=TRUE;
                    break;
                }
                ++node;

                if (node>=(RwUInt32)interp->numNodes)
                {
                    node=0;
                    ++slot;
                }
            }
            else
            {
                ++slot;
            }
        }

        /* Now make sure the actual presence of a matrix matches the required presence */
        if (needAMatrix && (!*matrix))
        {
            *matrix = RwMatrixCreate();
            if (!*matrix)
            {
                RWRETURN((RpUVAnimMaterialExtension *)NULL);
            }
        }
        else if (!needAMatrix && *matrix)
        {
            RwMatrixDestroy(*matrix);
            *matrix=NULL;
        }
    }
    RWRETURN(matExt);
}

/****************************************************************************
 UVAnimWrite

 Writes a material's uv animation to a stream

 Inputs :   RwStream *  Stream to write to
            RwInt32     Size of meshed when serialised (in bytes) (not used)
            void *      Object (material)
            RwInt32     Plugin data offset (not used)
            RwInt32     Plugin data size (not used)
 Outputs:   RwStream *  Stream pointer on success
 */

static RwStream    *
UVAnimWrite(RwStream * stream,
           RwInt32 binaryLength __RWUNUSED__,
           const void *object,
           RwInt32 offsetInObject __RWUNUSED__,
           RwInt32 sizeInObject __RWUNUSED__)
{
    const RpUVAnimMaterialExtension *materialExt
        = RPUVANIMMATERIALGETCONSTDATA(object);
    RwUInt32 size;
    MaterialExtStreamFlags flags=0;

    RWFUNCTION(RWSTRING("UVAnimWrite"));
    RWASSERT(stream);
    RWASSERT(object);

    size = sizeof(MaterialExtStreamFlags);

    /* Size of individual references */
    {
        size_t i;
        for (i=0; i<RP_UVANIM_MAXSLOTS; ++i)
        {
            RtAnimInterpolator *interp = materialExt->interp[i];
            if (interp)
            {
                flags |= (1 << i);
                size+=RP_UVANIM_MAXNAME;
            }
        }
    }

    if (!RwStreamWriteChunkHeader(stream, rwID_STRUCT, size))
    {
        RWRETURN((RwStream *)NULL);
    }

    /* Write out flags */
    if (!RwStreamWriteInt32
        (stream, (RwInt32 *) &flags, sizeof(MaterialExtStreamFlags)))
    {
        RWRETURN((RwStream *)NULL);
    }

    /* Write individual references */
    {
        size_t i;
        for (i=0; i<RP_UVANIM_MAXSLOTS; ++i)
        {
            RtAnimInterpolator *interp = materialExt->interp[i];
            if (interp)
            {
                RtAnimAnimation *anim=RtAnimInterpolatorGetCurrentAnim(interp);
                const RwChar *name=RpUVAnimGetName(anim);

                RwStreamWrite(stream, name, RP_UVANIM_MAXNAME);
            }
        }
    }

    RWRETURN(stream);
}

/* Make a name-placeholder animation */
static RpUVAnim *
CreateDummyAnim(const RwChar *name)
{
    RpUVAnim *anim;
    RwUInt32 dummyChannelMap[1] = { 0 };
    RpUVAnimKeyFrame *first, *last;
    RwMatrix identity;


    RWFUNCTION(RWSTRING("CreateDummyAnim"));

    anim = RpUVAnimCreate(name, 1, 2, 1.0f, dummyChannelMap,
                          rpUVANIMLINEARKEYFRAMES);
    if (!anim)
    {
        RWRETURN( (RpUVAnim *) NULL);
    }

    first = anim->pFrames;
    last =  (RpUVAnimKeyFrame *)    /* try to make it change-resistant */
                ( ((RwUInt8 *)first)
                   + anim->interpInfo->animKeyFrameSize
                );

    RwMatrixSetIdentity(&identity);

    RpUVAnimKeyFrameInit(anim, first, (RpUVAnimKeyFrame *) NULL,
                         0.0f, &identity);

    RpUVAnimKeyFrameInit(anim, last, first,
                         1.0f, &identity);

    RWRETURN(anim);
}

/****************************************************************************
 _rwStreamReadSingleUVAnim

 Reads a single uv animation from a stream, and attempts to locate it in
 the current dictionary and addref it

 Inputs :   RwStream *  Stream to read from
 Outputs:   RpUVAnim *  UV animation on success
 */

static RwStream *
_rwStreamReadSingleUVAnim(RwStream * stream, RpUVAnim **anim)
{
    RwChar name[RP_UVANIM_MAXNAME];

    RWFUNCTION(RWSTRING("_rwStreamReadSingleUVAnim"));
    RWASSERT(stream);

    if (!RwStreamRead(stream, name, RP_UVANIM_MAXNAME))
    {
        RWRETURN((RwStream *)NULL);
    }

    {
        RtDictSchema *schema = RpUVAnimGetDictSchema();
        RtDict *dict = RtDictSchemaGetCurrentDict(schema);

        if (dict)
        {
            *anim = RtDictFindNamedEntry(dict, name);

            if (*anim)
            {
                /* Add a ref for this material */
                RpUVAnimAddRef(*anim);
            }
            else   /* If no animation; need a fake one as a placeholder */
            {
                RWMESSAGE((RWSTRING("UV anim \'%s\' not found in current UV anim ")
                           RWSTRING("dictionary"), name));

                *anim = CreateDummyAnim(name);
            }
        }
        else
        {
            RWMESSAGE((RWSTRING("No current UV animation dictionary; could not ")
                       RWSTRING("locate anim \'%s\' to link to material"),name));

            *anim = CreateDummyAnim(name);
        }

        /* If no animation at this point, there's a problem */
        if (!*anim)
        {
            RWRETURN( (RwStream *) NULL);
        }
    }

    RWRETURN(stream);
}


/****************************************************************************
 UVAnimRead

 Reads a material's uv animation from a stream

 Inputs :   RwStream *  Stream to read from
            RwInt32     Size of animation uvanim data (in bytes)
            void *      Object (material)
            RwInt32     Plugin data offset (not used)
            RwInt32     Plugin data size (not used)
 Outputs:   RwStream *  Stream pointer on success
 */

static RwStream    *
UVAnimRead(RwStream * stream,
          RwInt32 binaryLength __RWUNUSED__,
          void *object,
          RwInt32 offsetInObject __RWUNUSED__,
          RwInt32 sizeInObject __RWUNUSED__)
{
    MaterialExtStreamFlags flags;
    RpUVAnimMaterialExtension *matExt = RPUVANIMMATERIALGETDATA(object);

    RWFUNCTION(RWSTRING("UVAnimRead"));
    RWASSERT(rpUVAnimModule.numInstances);
    RWASSERT(stream);
    RWASSERT(object);

    /* Read header and flags */
    if (   !RwStreamFindChunk(stream, rwID_STRUCT, NULL, NULL)
        || !RwStreamReadInt32(stream, (RwInt32 *)&flags, sizeof(flags)))
    {
        RWRETURN((RwStream *) NULL);
    }

    /* Read individual anim refs */
    {
        size_t i;
        for (i=0; i<RP_UVANIM_MAXSLOTS; ++i)
        {
            if (flags & (1 << i))
            {
                RpUVAnim *anim;
                if (!_rwStreamReadSingleUVAnim(stream, &anim))
                {
                    RWRETURN((RwStream *)NULL);
                }

                if (anim)
                {
                    RtAnimInterpolator **interp = &matExt->interp[i];

                    if (!(    (*interp = RtAnimInterpolatorCreate(
                                             RtAnimAnimationGetNumNodes(anim),
                                             anim->interpInfo->interpKeyFrameSize))
                           && (RtAnimInterpolatorSetCurrentAnim(*interp, anim))
                         )
                       )
                    {
                        /* Remove anim ref */
                        RpUVAnimDestroy(anim);

                        RWRETURN((RwStream *)NULL);
                    }
                }
            }
        }

        /* Ensure consistant matrices have been generated */
        if (!_rpUVAnimCreateConsistantMatrices(matExt))
        {
            RWRETURN((RwStream *)NULL);
        }
    }

    RWRETURN(stream);
}

/****************************************************************************
 UVAnimSize

 Gets the size of a materials uv animation (when serialized)

 Inputs :   void *      Object (material)
            RwInt32     Plugin data offset (not used)
            RwInt32     Plugin data size (not used)
 Outputs:   RwInt32     Size of mesh when serialised (in bytes)
 */

static              RwInt32
UVAnimSize(const void *object,
          RwInt32 offsetInObject __RWUNUSED__,
          RwInt32 sizeInObject __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("UVAnimSize"));

    /* Material needs extension data only if it has an interp / anim */
    if (RpMaterialUVAnimExists((const RpMaterial *)object))
    {
        const RpUVAnimMaterialExtension *matExt = RPUVANIMMATERIALGETCONSTDATA(object);
        RwInt32 size;

        size = rwCHUNKHEADERSIZE;        /* Struct wrapper */
        size += sizeof(MaterialExtStreamFlags);

        /* Size of individual references */
        {
            size_t i;
            for (i=0; i<RP_UVANIM_MAXSLOTS; ++i)
            {
                RtAnimInterpolator *interp = matExt->interp[i];
                if (interp)
                {
                    size+=RP_UVANIM_MAXNAME;
                }
            }
        }

        RWRETURN(size);
    }

    /* By default, do not need to stream extension data */
    RWRETURN(0);
}

_rpUVAnimCustomData *
_rpUVAnimCustomDataStreamRead(RwStream *stream)
{
    _rpUVAnimCustomData *customData;
    RWFUNCTION(RWSTRING("_rpUVAnimCustomDataStreamRead"));
    RWASSERT(stream);

    /* Allocate custom data */
    customData
        = (_rpUVAnimCustomData *)RwFreeListAlloc(
                                    _rpUVAnimCustomDataFreeList,
                                    rwID_UVANIMPLUGIN | rwMEMHINTDUR_EVENT);
    if (!customData)
    {
        RWRETURN((_rpUVAnimCustomData *)NULL);
    }

    if (!RwStreamRead(stream, customData->name, RP_UVANIM_MAXNAME))
    {
        RWRETURN((_rpUVAnimCustomData *)NULL);
    }

    if (!RwStreamReadInt32(stream, (RwInt32 *)customData->nodeToUVChannelMap,
                           sizeof(RwInt32) * RP_UVANIM_MAXSLOTS ))
    {
        RWRETURN((_rpUVAnimCustomData *)NULL);
    }

    /* Caller will own */
    customData->refCount = 1;

    RWRETURN(customData);
}

const _rpUVAnimCustomData *
_rpUVAnimCustomDataStreamWrite(const _rpUVAnimCustomData *customData,
                               RwStream *stream)
{
    RWFUNCTION(RWSTRING("_rpUVAnimCustomDataStreamWrite"));
    RWASSERT(customData);
    RWASSERT(stream);

    if (!RwStreamWrite(stream, &customData->name, RP_UVANIM_MAXNAME))
    {
        RWRETURN((_rpUVAnimCustomData *)NULL);
    }

    if (!RwStreamWriteInt32(stream, (const RwInt32 *)customData->nodeToUVChannelMap,
                           sizeof(RwInt32) * RP_UVANIM_MAXSLOTS ))
    {
        RWRETURN((_rpUVAnimCustomData *)NULL);
    }

    RWRETURN(customData);
}

RwUInt32
_rpUVAnimCustomDataStreamGetSize(const _rpUVAnimCustomData *customData __RWUNUSED__)
{
    RwUInt32 size;
    RWFUNCTION(RWSTRING("_rpUVAnimCustomDataStreamGetSize"));
    RWASSERT(customData);

    size = RP_UVANIM_MAXNAME
            + sizeof(RwInt32) * RP_UVANIM_MAXSLOTS;

    RWRETURN(size);
}

/*----------------------------------------------------------------------*/

/*                              - RpUVAnim -                            */

/*----------------------------------------------------------------------*/

static RpUVAnim *
_rpUVAnimInit(RpUVAnim *anim, const RwChar *name, RwUInt32 numNodes, RwUInt32 *nodeToUVChannelMap)
{
    RWFUNCTION(RWSTRING("_rpUVAnimInit"));
    RWASSERT(anim);
    RWASSERT(numNodes >= 1);
    RWASSERT(numNodes <= RP_UVANIM_MAXSLOTS);
    RWASSERT(nodeToUVChannelMap);

    {
        /* Allocate custom data */
        _rpUVAnimCustomData *customData
            = (_rpUVAnimCustomData *)RwFreeListAlloc(
                                       _rpUVAnimCustomDataFreeList,
                                       rwID_UVANIMPLUGIN | rwMEMHINTDUR_EVENT);
        if (!customData)
        {
            RWRETURN((RpUVAnim *)NULL);
        }

        /* Assign name */
        strncpy(customData->name, name, RP_UVANIM_MAXNAME);
        customData->name[RP_UVANIM_MAXNAME-1]='\0';

        /* Assign UV channels */
        memcpy(customData->nodeToUVChannelMap, nodeToUVChannelMap, sizeof(RwUInt32) * numNodes);

        /* Start refcount at 1, since the caller has a reference */
        customData->refCount = 1;

        /* Store customData in anim */
        anim->customData = customData;
    }

    RWRETURN(anim);
}

static RwBool
_rpUVAnimDestruct(RpUVAnim *anim)
{
    RWFUNCTION(RWSTRING("_rpUVAnimDestruct"));
    RWASSERT(anim);

    {
        _rpUVAnimCustomData *data = (_rpUVAnimCustomData *)anim->customData;

        --data->refCount;

        if (data->refCount)
        {
            /* Still have references, so didn't destruct */
            RWRETURN(FALSE);
        }
        else
        {
            /* No more references; really destruct */
            RwFreeListFree(_rpUVAnimCustomDataFreeList, anim->customData);
            RWRETURN(TRUE);
        }
    }
}

/**
 * \ingroup rpuvanim
 * \ref RpUVAnimCreate
 * creates and returns an \ref RpUVAnim. An \ref RpUVAnim is an \ref RtAnimAnimation
 * with some custom data. All \ref RtAnimAnimation functions can be used upon \ref RpUVAnim s.
 * Each node within an \ref RpUVAnim may be mapped to a nominal UV channel. Only channel
 * 0 and channel 1 mappings are used by the supplied \ref RpMaterialUVAnimApplyUpdate,
 * which is sufficient for single pass and dual pass UV animations.
 * It is also possible to get access to the individual interpolators and implement your own
 * update function, which may be useful for animating UV channels used by
 * multitexture effects.
 *
 * \param name Name of the animation; maximum length is \ref RP_UVANIM_MAXNAME
 * \param numNodes Number of nodes within the animation
 * \param numFrames Total number of keyframes over all nodes within the animation
 * \param duration Time of very last keyframe in the animation
 * \param nodeIndexToUVChannelMap Array by \ref RP_UVANIM_MAXSLOTS defining
 * which nominal UV channel gets modified by each node
 * \param numNodes representing the nominal UV channel that node controls
 * \param keyframeType Specify which interpolator scheme to use
 *
 * \return A pointer to the new anim on success, NULL if there is an error.
 *
 * \see RpUVAnimDestroy
 * \see RtAnimInterpolatorCreate
 * \see RtAnimInterpolatorDestroy
 */
RpUVAnim *
RpUVAnimCreate(const RwChar *name, RwUInt32 numNodes, RwUInt32 numFrames,
               RwReal duration, RwUInt32 *nodeIndexToUVChannelMap,
               RpUVAnimKeyFrameType keyframeType)
{
    RpUVAnim    *anim = NULL;

    RWAPIFUNCTION(RWSTRING("RpUVAnimCreate"));
    RWASSERT(rpUVAnimModule.numInstances);
    RWASSERT(numNodes >= 1);
    RWASSERT(numNodes <= RP_UVANIM_MAXSLOTS);
    RWASSERT(nodeIndexToUVChannelMap);

    switch (keyframeType)
    {
        case rpUVANIMLINEARKEYFRAMES:
            anim = RtAnimAnimationCreate(
                        _rpUVAnimLinearInterpolatorInfo.typeID,
                        numFrames,
                        0, /* flags */
                        duration);
            break;
        case rpUVANIMPARAMKEYFRAMES:
            anim = RtAnimAnimationCreate(
                        _rpUVAnimParamInterpolatorInfo.typeID,
                        numFrames,
                        0, /* flags */
                        duration);
            break;
        default:
            break;
    }

    if (anim)
    {
        RWRETURN(_rpUVAnimInit(anim, name, numNodes, nodeIndexToUVChannelMap));
    }

    RWRETURN((RpUVAnim *)NULL);
}


/**
 * \ingroup rpuvanim
 * \ref RpUVAnimDestroy
 * destroys a UV animation. UV animations are reference counted, so calling
 * this function will not necessarily free the animation unless the caller
 * holds the last reference to it.
 *
 * \param anim A pointer to the animation to be destroyed.
 *
 * \return TRUE
 *
 * \see RpUVAnimCreate
 * \see RtAnimInterpolatorCreate
 * \see RtAnimInterpolatorDestroy
 *
 */
RwBool
RpUVAnimDestroy(RpUVAnim *anim)
{
    RWAPIFUNCTION(RWSTRING("RpUVAnimDestroy"));
    RWASSERT(anim);

    /* Only destroy anim if destruct succeeds (which will happen when refCount
       says so) */
    if (_rpUVAnimDestruct(anim))
    {
        RtAnimAnimationDestroy(anim);
    }
    RWRETURN(TRUE);
}

/**
 * \ingroup rpuvanim
 * \ref RpUVAnimAddRef
 * registers that the caller has a reference to the anim.
 *
 * \param anim A pointer to the anim for which a reference is needed
 *
 * \return The anim
 *
 * \see RpUVAnimCreate
 * \see RpUVAnimDestroy
 *
 */
RpUVAnim *
RpUVAnimAddRef(RpUVAnim *anim)
{
    RWAPIFUNCTION(RWSTRING("RpUVAnimAddRef"));
    RWASSERT(anim);

    {
        _rpUVAnimCustomData *data=(_rpUVAnimCustomData *)anim->customData;
        ++data->refCount;
    }
    RWRETURN(anim);
}


/**
 * \ingroup rpuvanim
 * \ref RpMaterialUVAnimApplyUpdate
 * updates the UV transform matrices for a given material according to the current
 * state of the animations applied. This implementation looks through each
 * UV animation slot in the material, and accumulates update matrices for
 * the single and dual pass UV channels by preconcatenation. The accumulation
 * is done when a node of an animation is mapped to one of these channels.
 * This function must only be called on materials that have an
 * \ref rpMATFXEFFECTUVTRANSFORM or \ref rpMATFXEFFECTDUALUVTRANSFORM effect
 * set.
 * You are free to implement your own ApplyUpdate functions, as the
 * underlying RtAnimInterpolators are exposed through \ref RpMaterialUVAnimGetInterpolator
 * and \ref RpMaterialUVAnimGetInterpolator.
 *
 * \param material The material to apply the UV animation update to
 *
 * \return The material
 * \see RtMaterialUVAnimAddAnimTime
 * \see RtMaterialUVAnimSubAnimTime
 */
RpMaterial *
RpMaterialUVAnimApplyUpdate(RpMaterial *material)
{
    RpUVAnimMaterialExtension *matExt;
    RwUInt32 channel;

    RWAPIFUNCTION(RWSTRING("RpMaterialUVAnimApplyUpdate"));
    RWASSERT(rpUVAnimModule.numInstances);
    RWASSERT(material);

    matExt = (RpUVAnimMaterialExtension *)RPUVANIMMATERIALGETDATA(material);

    /* Clear matrices */
    for(channel=0; channel<RP_UVANIM_APPLIEDCHANNELS;++channel)
    {
        RwMatrix *matrix = matExt->uv[channel];
        if (matrix)
        {
            RwMatrixSetIdentity(matrix);
        }
    }

    /* Iterate over slots */
    {
        RwInt32 iSlot;

        for (iSlot=0; iSlot <  RP_UVANIM_MAXSLOTS; ++iSlot)
        {
            RtAnimInterpolator *interp = matExt->interp[iSlot];

            if (interp)
            {
                /* Cache anim info */
                RpUVAnim *anim = RtAnimInterpolatorGetCurrentAnim(interp);
                RwInt32 keyFrameSize = interp->currentInterpKeyFrameSize;
                RtAnimKeyFrameApplyCallBack keyFrameApplyCB
                    = interp->keyFrameApplyCB;
                RwUInt32 *nodeToUVChannelMap
                    = ((_rpUVAnimCustomData *)anim->customData)
                        ->nodeToUVChannelMap;

                /*  Iterate over each node for that RtAnimIterator */
                {
                    RwInt32 iNode;
                    void *nodeInterpFrame = rtANIMGETINTERPFRAME(interp, 0);;

                    for (iNode = 0; iNode < interp->numNodes; ++iNode)
                    {
                        channel = nodeToUVChannelMap[iNode];
                        if (channel < 2)
                        {
                            RwMatrix *matrix = matExt->uv[channel];
                            RwMatrix interpMatrix;

                            if (!matrix)
                            {
                                /* next node */
                                continue;
                            }

                            if (RpUVAnimLinearKeyFrameApply == keyFrameApplyCB)
                            {
                                RpUVAnimLinearKeyFrameToMatrixMacro(&interpMatrix,
                                                                    nodeInterpFrame);
                            }
                            else
                            if (RpUVAnimParamKeyFrameApply == keyFrameApplyCB)
                            {
                                RpUVAnimParamKeyFrameToMatrixMacro(&interpMatrix,
                                                                   nodeInterpFrame,
                                                                   &rpUVAnimPivot,
                                                                   &rpUVAnimInvPivot);
                            }
                            else
                            {
                                keyFrameApplyCB(&interpMatrix, nodeInterpFrame);
                            }

                            /* Concatenate with matrix from other anims */
                            RwMatrixTransform(matrix, &interpMatrix,
                                              rwCOMBINEPRECONCAT);
                            nodeInterpFrame
                              = (void *)((RwUInt8 *)nodeInterpFrame
                                                        + keyFrameSize);
                        }
                    }
                }
            }
        }

        RpMatFXMaterialSetUVTransformMatrices(
            material,
            matExt->uv[RP_UVANIM_FIRST_PASS_CHANNEL],
            matExt->uv[RP_UVANIM_DUAL_PASS_CHANNEL]);
    }

    RWRETURN(material);
}

/**
 * \ingroup rpuvanim
 * \ref RpUVAnimGetName
 *  returns the name of the UV animation
 *
 * \param anim A pointer to the anim for which the name is required
 *
 * \return The anim
 *
 * \see RpUVAnimCreate
 * \see RpUVAnimDestroy
 *
 */
const RwChar *
RpUVAnimGetName(const RpUVAnim *anim)
{
    RWAPIFUNCTION(RWSTRING("RpUVAnimGetName"));
    RWASSERT(anim);

    {
        _rpUVAnimCustomData *data=(_rpUVAnimCustomData *)anim->customData;
        RWRETURN(data->name);
    }
}

/**
 * \ingroup rpuvanim
 * \ref RpMaterialSetUVAnim
 * sets a UV animation on a material by creating an interpolator in the given
 * slot and setting its current animation to the supplied animation. It is an
 * error to set an animation on a slot that's already used.
 *
 * \param material The material on which to set the animation
 * \param anim The animation to set upon the material
 * \param slot The animation slot to use. Up to RP_UVANIM_MAXSLOTS are allowed.
 *
 * \return The material on success, NULL otherwise.
 *
 */
RpMaterial *
RpMaterialSetUVAnim(RpMaterial *material, RpUVAnim *anim, RwUInt32 slot)
{
    RpUVAnimMaterialExtension *materialExt;

    RWAPIFUNCTION(RWSTRING("RpMaterialSetUVAnim"));
    RWASSERT(rpUVAnimModule.numInstances);
    RWASSERT(material);
    RWASSERT(anim);
    RWASSERT(slot<RP_UVANIM_MAXSLOTS);

    materialExt = (RpUVAnimMaterialExtension *)RPUVANIMMATERIALGETDATA(material);

    {
        RtAnimInterpolator **interp = &materialExt->interp[slot];

        RWASSERTM(NULL==*interp, (RWSTRING("Animation already exists")));

        /* Create a new interpolator */
        *interp = RtAnimInterpolatorCreate(
                       RtAnimAnimationGetNumNodes(anim),
                       anim->interpInfo->interpKeyFrameSize);
        if (!*interp)
        {
            RWRETURN((RpMaterial *)NULL);
        }

        /* Set the new animation */
        RtAnimInterpolatorSetCurrentAnim(*interp, anim);

        /* Make sure we have somewhere to store 'total' UV animation matrices */
        if (!_rpUVAnimCreateConsistantMatrices(materialExt))
        {
            RtAnimInterpolatorDestroy(*interp);

            RWRETURN((RpMaterial *)NULL);
        }

        /* Now have an additional reference to the material */
        RpUVAnimAddRef(anim);
    }

    RWRETURN(material);
}


/**
 * \ingroup rpuvanim
 * \ref RpMaterialUVAnimGetInterpolator
 * gets the interpolator in a given slot on the material.
 *
 * \param material The material
 * \param slot The slot to get the interpolator from
 *
 * \return The interpolator in the specified slot
 *
 */
RpUVAnimInterpolator *
RpMaterialUVAnimGetInterpolator(RpMaterial *material, RwUInt32 slot)
{
    RpUVAnimMaterialExtension *materialExt;

    RWAPIFUNCTION(RWSTRING("RpMaterialUVAnimGetInterpolator"));
    RWASSERT(rpUVAnimModule.numInstances);
    RWASSERT(material);
    RWASSERT(slot<RP_UVANIM_MAXSLOTS);

    materialExt = (RpUVAnimMaterialExtension *)RPUVANIMMATERIALGETDATA(material);

    RWRETURN(materialExt->interp[slot]);
}

/**
 * \ingroup rpuvanim
 * \ref RpMaterialUVAnimSetInterpolator
 *  sets an interpolator in a given slot on the material. Does not check
 *  if there is already an interpolator there; you are responsible for
 *  destroying the old interpolator if one exists.
 *
 * \param material The material
 * \param interp The interpolator to set
 * \param slot The slot to place the interpolator in; maximum is \ref RP_UVANIM_MAXSLOTS
 *
 * \return The material
 *
 */
RpMaterial *
RpMaterialUVAnimSetInterpolator(RpMaterial *material,
                                RpUVAnimInterpolator *interp, RwUInt32 slot)
{
    RpUVAnimMaterialExtension *materialExt;

    RWAPIFUNCTION(RWSTRING("RpMaterialUVAnimSetInterpolator"));
    RWASSERT(rpUVAnimModule.numInstances);
    RWASSERT(material);
    RWASSERT(interp);

    materialExt = (RpUVAnimMaterialExtension *)RPUVANIMMATERIALGETDATA(material);
    materialExt->interp[slot] = interp;

    RWRETURN(material);
}

/**
 * \ingroup rpuvanim
 * \ref RpMaterialUVAnimSetCurrentTime
 * moves the animations on a material to a specific time
 *
 * \param material The material
 * \param time The time to move to
 *
 * \return The material
 *
 */
RpMaterial *
RpMaterialUVAnimSetCurrentTime(RpMaterial *material, RwReal time)
{
    RpUVAnimMaterialExtension *materialExt;

    RWAPIFUNCTION(RWSTRING("RpMaterialUVAnimSetCurrentTime"));
    RWASSERT(rpUVAnimModule.numInstances);
    RWASSERT(material);

    materialExt = (RpUVAnimMaterialExtension *)RPUVANIMMATERIALGETDATA(material);
    {
        RwUInt32 i;
        for (i=0; i<RP_UVANIM_MAXSLOTS; ++i)
        {
            RpUVAnimInterpolator *interp = materialExt->interp[i];
            if (interp)
            {
                RtAnimInterpolatorSetCurrentTime(interp, time);
            }
        }
    }


    RWRETURN(material);
}
/**
 * \ingroup rpuvanim
 * \ref RpMaterialUVAnimAddAnimTime
 * moves the animations on a material forwards in time
 *
 * \param material The material
 * \param deltaTime The time to move forwards (in seconds)
 *
 * \return The material
 *
 */
extern RpMaterial *
RpMaterialUVAnimAddAnimTime(RpMaterial *material, RwReal deltaTime)
{
    RpUVAnimMaterialExtension *materialExt;

    RWAPIFUNCTION(RWSTRING("RpMaterialUVAnimAddAnimTime"));
    RWASSERT(rpUVAnimModule.numInstances);
    RWASSERT(material);

    materialExt = (RpUVAnimMaterialExtension *)RPUVANIMMATERIALGETDATA(material);
    {
        RwUInt32 i;
        for (i=0; i<RP_UVANIM_MAXSLOTS; ++i)
        {
            RpUVAnimInterpolator *interp = materialExt->interp[i];
            if (interp)
            {
                RtAnimInterpolatorAddAnimTime(interp, deltaTime);
            }
        }
    }

    RWRETURN(material);
}

/**
 * \ingroup rpuvanim
 * \ref RpMaterialUVAnimSubAnimTime
 * moves the animations on a material backward in time.
 *
 * \param material The material
 * \param deltaTime The time to move backwards (in seconds)
 *
 * \return The material
 *
 */
RpMaterial *
RpMaterialUVAnimSubAnimTime(RpMaterial *material, RwReal deltaTime)
{
    RpUVAnimMaterialExtension *materialExt;

    RWAPIFUNCTION(RWSTRING("RpMaterialUVAnimSubAnimTime"));
    RWASSERT(rpUVAnimModule.numInstances);
    RWASSERT(material);

    materialExt = (RpUVAnimMaterialExtension *)RPUVANIMMATERIALGETDATA(material);
    {
        RwUInt32 i;
        for (i=0; i<RP_UVANIM_MAXSLOTS; ++i)
        {
            RpUVAnimInterpolator *interp = materialExt->interp[i];
            if (interp)
            {
                RtAnimInterpolatorSubAnimTime(interp, deltaTime);
            }
        }
    }

    RWRETURN(material);
}

/**
 * \ingroup rpuvanim
 * \ref RpMaterialUVAnimExists
 * determines if a material has a UV animation on it.
 *
 * \param material The material
 *
 * \return TRUE if there's an animation, FALSE otherwise
 *
 */
RwBool
RpMaterialUVAnimExists(const RpMaterial *material)
{
    const RpUVAnimMaterialExtension *matExt;

    RWAPIFUNCTION(RWSTRING("RpMaterialUVAnimExists"));
    RWASSERT(material);

    matExt = (const RpUVAnimMaterialExtension *)RPUVANIMMATERIALGETCONSTDATA(material);
    {
        RwUInt32 i=0;
        for (i=0; i<RP_UVANIM_MAXSLOTS; ++i)
        {
            if (matExt->interp[i])
            {
                RWRETURN(TRUE);
            }
        }
    }
    RWRETURN(FALSE);
}


#ifdef RWDEBUG
/* Debugging function */
const RpUVAnim *
_rpUVAnimDump(const RpUVAnim *anim)
{
    RWFUNCTION(RWSTRING("_rpUVAnimDump"));
    {
        RwUInt32 keyFrame;
        RpUVAnimKeyFrame *frame = (RpUVAnimKeyFrame *)anim->pFrames;
        RWMESSAGE((RpUVAnimGetName(anim)));
        if (anim->interpInfo->typeID == _rpUVAnimLinearInterpolatorInfo.typeID)
        {
            RpUVAnimLinearKeyFrameData *data = &((RpUVAnimKeyFrame *)anim->pFrames)->data.linear;
            for(keyFrame=0; keyFrame<(RwUInt32)anim->numFrames; ++keyFrame)
            {
                RWMESSAGE((RWSTRING("time:%-10.2f uv:%-3.2f %-3.2f %-3.2f ")
                           RWSTRING("%-3.2f %-3.2f %-3.2f     PrevFrame:%d\n"),
                           frame->time,
                           data->uv[0], data->uv[2], data->uv[3],
                           data->uv[3], data->uv[4], data->uv[5],
                           frame->prevFrame - (RpUVAnimKeyFrame *)anim->pFrames
                          ));
                ++frame;
            }
        }
        else
        {
            RpUVAnimParamKeyFrameData *data = &((RpUVAnimKeyFrame *)anim->pFrames)->data.param;
            for(keyFrame=0; keyFrame<(RwUInt32)anim->numFrames; ++keyFrame)
            {
                RWMESSAGE((RWSTRING("time:%-10.2f theta:%-4.3f s0:%-3.2f s1:%-3.2f ")
                           RWSTRING("skew:%-3.2f x:%-3.2f y:%-3.2f     PrevFrame:%d\n"),
                           frame->time,
                           data->theta, data->s0, data->s1,
                           data->skew, data->x, data->y,
                           frame->prevFrame - (RpUVAnimKeyFrame *)anim->pFrames
                          ));
                ++frame;
            }
        }
    }
    RWRETURN(anim);
}
#endif
