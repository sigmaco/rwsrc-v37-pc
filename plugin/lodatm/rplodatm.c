/*
 *
 * LOD Management
 */

/**
 * \ingroup rplodatm
 * \page rplodatmoverview RpLODAtomic Plugin Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rplodatm.h
 * \li \b Libraries: rwcore, rpworld, rplodatm
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpLODAtomicPluginAttach
 *
 * \subsection lodatmoverview Overview
 * This plugin provides high-level support for LOD management in your
 * models and extends RenderWare Graphics by providing dynamic level-of-detail
 * support for the RpAtomic datatype exposed by RpWorld.
 *
 * LOD management is linked to individual RpAtomic datatypes. The process
 * is usually as follows:
 *
 * \li Up to ten RpGeometry objects containing the different levels of detail
 * are associated with the RpAtomic using the plugin뭩 functions.
 *
 * \li The maximum and minimum LOD thresholds are set. The plugin uses linear
 * interpolation to determine which of the RpGeometry objects is to be rendered
 * at any particular time.
 *
 * \li The developer hooks the plugin뭩 renderer to the RpAtomic(s) for which
 * LOD data has been created. (The developer can optionally use their own callback
 * function instead of the default routine.)
 *
 * The RpLODAtomic plugin will then take care of switching between the different
 * RpGeometry objects for the atomic.
 */

/**********************************************************************
 *
 * File :     rplodatm.c
 *
 * Abstract : Handle level of detail (LOD) geometry in RenderWare
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

#include "rpplugin.h"
#include "rwcore.h"
#include "rpworld.h"
#include "rpdbgerr.h"
#include "rplodatm.h"

/****************************************************************************
 * Local Globals.
 */

#if (defined(RWDEBUG))
long                rpLodatmStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

/****************************************************************************
 Global Types
 */

typedef struct _rpLODAtomicGlobalVars RpLODAtomicGlobalVars;
struct _rpLODAtomicGlobalVars
{
    RwInt32             engineOffset; /* Offset into global data */
    RwInt32             atmCacheOffset; /* Offset into global data */
    RwCamera           *atmCamera;
    RwFreeList         *atmCacheFreeList;
};

typedef struct _rpLODAtomicCache RpLODAtomicCache;
struct _rpLODAtomicCache
{
    RwBool              hook;
    RpAtomicCallBackRender atomicRender;
    RpLODAtomicLODCallBack atomicSelectLOD;
    RwReal              farRange;
    RwReal              invFarRange;
    RwInt32             curLOD;
    RpGeometry         *lodGeometry[RPLODATOMICMAXLOD];
};

#define RPLODFROMATOMIC(_atomic) \
    ((RpLODAtomicCache *)(((char *)(_atomic)) + RpLODAtomicOffset))

#define RPLODOFFSET(_ptr, _offset) \
    (*(RpLODAtomicCache **)(((char *)(_ptr)) + _offset))

RpLODAtomicGlobalVars RpLODAtomicGlobals;

/************************************************************************
 */
static RpAtomic    *
LODAtomicDefaultLODZero(RpAtomic * atomic,
                        RpLODAtomicCache * rpLODAtomicCache)
{
    RpAtomic           *result;
    RpGeometry         *geom;

    RWFUNCTION(RWSTRING("LODAtomicDefaultLODZero"));

    result = atomic;

    RWASSERT(atomic != NULL);
    RWASSERT(rpLODAtomicCache != NULL);

    geom = RpAtomicGetGeometry(atomic);

    if (geom != NULL)
        RpGeometryAddRef(geom);

    rpLODAtomicCache->lodGeometry[0] = geom;

    RWRETURN(result);
}

/************************************************************************
 */
static              RwInt32
LODAtomicCallBackSelectLOD(RpAtomic * atomic)
{
    RpLODAtomicCache   *rpLODAtomicCache;

    RwInt32             result;
    RwInt32             offset;
    RwV3d               vAt, atomicPos, cameraPos, cameraAt;
    RwReal              dot;
    RwCamera           *camera;

    RWFUNCTION(RWSTRING("LODAtomicCallBackSelectLOD"));

    result = -1;
    offset = RpLODAtomicGlobals.atmCacheOffset;
    camera = RwCameraGetCurrentCamera();

    RWASSERT(offset >= 0);
    RWASSERT(atomic != NULL);
    RWASSERT(camera != NULL);

    rpLODAtomicCache = RPLODOFFSET(atomic, offset);

    if (rpLODAtomicCache != NULL)
    {
        /* Calculate how far the atomic is away from the viewer. */
        atomicPos = RwFrameGetLTM(RpAtomicGetFrame(atomic))->pos;
        cameraPos = RwFrameGetLTM(RwCameraGetFrame(camera))->pos;
        cameraAt = RwFrameGetLTM(RwCameraGetFrame(camera))->at;

        RwV3dSub(&vAt, &atomicPos, &cameraPos);

        dot = RwV3dDotProduct(&vAt, &cameraAt);

        result = RwInt32FromRealMacro(dot * rpLODAtomicCache->invFarRange);

        result = (result < 0) ? 0 :
            (result >= RPLODATOMICMAXLOD) ? RPLODATOMICMAXLOD - 1 :
            result;
    }

    RWRETURN(result);
}

/************************************************************************
 */
static RpAtomic    *
LODAtomicSelectLOD(RpAtomic * atomic,
                   RpLODAtomicCache * rpLODAtomicCache)
{
    RpAtomic           *result;
    RwInt32             lodIdx;
    RpGeometry         *lodGeom, *geom;

    RWFUNCTION(RWSTRING("LODAtomicSelectLOD"));

    result = NULL;

    RWASSERT(atomic != NULL);
    RWASSERT(rpLODAtomicCache != NULL);

    lodIdx = (rpLODAtomicCache->atomicSelectLOD) (atomic);

    lodIdx = (lodIdx < 0) ? rpLODAtomicCache->curLOD :
        (lodIdx >= RPLODATOMICMAXLOD) ? RPLODATOMICMAXLOD - 1 : lodIdx;

    lodGeom = rpLODAtomicCache->lodGeometry[lodIdx];

    /* Reset the geometry if changed. */
    geom = RpAtomicGetGeometry(atomic);

    if (lodGeom != geom)
        RpAtomicSetGeometry(atomic, lodGeom, 0);

    rpLODAtomicCache->curLOD = lodIdx;

    /* If we switching in a NULL geometry, then we should stop the
     * rendering atomic.
     */
    if (lodGeom != NULL)
        result = atomic;

    RWRETURN(result);
}

/************************************************************************
 */
//@{ Jaewon 20050602
// Removed 'static'
RpAtomic    *
//@} Jaewon
LODAtomicCallBackRender(RpAtomic * atomic)
{
    RpLODAtomicCache   *rpLODAtomicCache;

    RpAtomic           *result;
    RwInt32             offset;

    RWFUNCTION(RWSTRING("LODAtomicCallBackRender"));

    result = NULL;
    offset = RpLODAtomicGlobals.atmCacheOffset;

    RWASSERT(offset >= 0);
    RWASSERT(atomic != NULL);

    rpLODAtomicCache = RPLODOFFSET(atomic, offset);

    if (rpLODAtomicCache != NULL)
    {
        RWASSERT(rpLODAtomicCache->atomicRender !=
                 LODAtomicCallBackRender);

        result = LODAtomicSelectLOD(atomic, rpLODAtomicCache);

        if (result != NULL)
		{
			//@{ Jaewon 20050423
			// rpLODAtomicCache->atomicRender can be null... why?==;
			if(rpLODAtomicCache->atomicRender)
				result = (rpLODAtomicCache->atomicRender(atomic));
			//@} Jaewon
		}
    }
    else
    {
        /* Problem. We should not get here. Calling the atomic render
         * may end up again here so do nothing and return an error.
         */
        RWASSERT(FALSE);

        result = NULL;
    }

    RWRETURN(result);
}

/************************************************************************
 */
static RpAtomic    *
LODAtomicHookRender(RpAtomic * atomic,
                    RpLODAtomicCache * rpLODAtomicCache)
{
    RWFUNCTION(RWSTRING("LODAtomicHookRender"));

    RWASSERT(rpLODAtomicCache != NULL);

    if (rpLODAtomicCache->hook == FALSE)
    {
        rpLODAtomicCache->atomicRender =
            RpAtomicGetRenderCallBack(atomic);

        RpAtomicSetRenderCallBack(atomic, LODAtomicCallBackRender);

        rpLODAtomicCache->hook = TRUE;
    }

    RWRETURN(atomic);
}

/************************************************************************
 */
static RpAtomic    *
LODAtomicUnHookRender(RpAtomic * atomic,
                      RpLODAtomicCache * rpLODAtomicCache)
{
    RWFUNCTION(RWSTRING("LODAtomicUnHookRender"));

    RWASSERT(rpLODAtomicCache != NULL);

    if (rpLODAtomicCache->hook)
    {
        if (rpLODAtomicCache->atomicRender != NULL)
            RpAtomicSetRenderCallBack(atomic,
                                      rpLODAtomicCache->atomicRender);

        rpLODAtomicCache->atomicRender = NULL;
        rpLODAtomicCache->hook = FALSE;
    }

    RWRETURN(atomic);
}

/************************************************************************
 */
static RpLODAtomicCache *
LODAtomicInitCache(RpLODAtomicCache * rpLODAtomicCache)
{
    RwInt32             i;

    RWFUNCTION(RWSTRING("LODAtomicInitCache"));

    RWASSERT(rpLODAtomicCache != NULL);

    rpLODAtomicCache->hook = FALSE;
    rpLODAtomicCache->atomicRender = NULL;
    rpLODAtomicCache->atomicSelectLOD = LODAtomicCallBackSelectLOD;

    rpLODAtomicCache->farRange = -1;
    rpLODAtomicCache->invFarRange = -1;

    rpLODAtomicCache->curLOD = -1;

    for (i = 0; i < RPLODATOMICMAXLOD; ++i)
        rpLODAtomicCache->lodGeometry[i] = NULL;

    RWRETURN(rpLODAtomicCache);
}

/************************************************************************
 */
static RpLODAtomicCache *
LODAtomicNewCache(void)
{
    RpLODAtomicCache   *rpLODAtomicCache;

    RWFUNCTION(RWSTRING("LODAtomicNewCache"));

    rpLODAtomicCache = (RpLODAtomicCache *)
        RwFreeListAlloc(RpLODAtomicGlobals.atmCacheFreeList,
                        rwID_LODATMPLUGIN | rwMEMHINTDUR_EVENT);

    if (rpLODAtomicCache != NULL)
        LODAtomicInitCache(rpLODAtomicCache);

    RWRETURN(rpLODAtomicCache);
}

/************************************************************************
 */
static RpLODAtomicCache *
LODAtomicCopyCache(RpLODAtomicCache * rpDstLODAtomicCache,
                   RpLODAtomicCache * rpSrcLODAtomicCache)
{
    RwInt32             i;

    RWFUNCTION(RWSTRING("LODAtomicCopyCache"));

    RWASSERT(rpDstLODAtomicCache != NULL);
    RWASSERT(rpSrcLODAtomicCache != NULL);

    rpDstLODAtomicCache->hook = rpSrcLODAtomicCache->hook;
    rpDstLODAtomicCache->atomicRender =
        rpSrcLODAtomicCache->atomicRender;

    rpDstLODAtomicCache->atomicSelectLOD =
        rpSrcLODAtomicCache->atomicSelectLOD;

    rpDstLODAtomicCache->farRange = rpSrcLODAtomicCache->farRange;

    rpDstLODAtomicCache->invFarRange = rpSrcLODAtomicCache->invFarRange;

    rpDstLODAtomicCache->curLOD = rpSrcLODAtomicCache->curLOD;

    for (i = 0; i < RPLODATOMICMAXLOD; ++i)
    {
        rpDstLODAtomicCache->lodGeometry[i] =
            rpSrcLODAtomicCache->lodGeometry[i];

        if (rpDstLODAtomicCache->lodGeometry[i])
            RpGeometryAddRef(rpDstLODAtomicCache->lodGeometry[i]);
    }

    RWRETURN(rpSrcLODAtomicCache);
}

/************************************************************************
 */
static void
LODAtomicDestroyCache(RpLODAtomicCache * rpLODAtomicCache)
{
    RwInt32             i;

    RWFUNCTION(RWSTRING("LODAtomicDestroyCache"));

    RWASSERT(rpLODAtomicCache != NULL);

    for (i = 0; i < RPLODATOMICMAXLOD; ++i)
    {
        if (rpLODAtomicCache->lodGeometry[i])
            RpGeometryDestroy(rpLODAtomicCache->lodGeometry[i]);
    }

    LODAtomicInitCache(rpLODAtomicCache);

    RwFreeListFree(RpLODAtomicGlobals.atmCacheFreeList,
                   rpLODAtomicCache);

    RWRETURNVOID();
}

/************************************************************************
 */
static RwStream    *
LODAtomicCallBackReadChunk(RwStream * stream,
                           RwInt32 __RWUNUSED__ len,
                           RpAtomic * atomic,
                           RwInt32 offset, RwInt32 __RWUNUSED__ size)
{
    RwStream           *result;
    RwUInt32            structSize, version;

    RWFUNCTION(RWSTRING("LODAtomicCallBackReadChunk"));

    result = NULL;

    if (RwStreamFindChunk(stream, rwID_STRUCT, &structSize, &version))
    {
        RwInt32             i;
        RpGeometry         *lodGeometry;
        RpLODAtomicCache   *rpLODAtomicCache;

        RWASSERT(version >= rwLIBRARYBASEVERSION);
        RWASSERT(version <= rwLIBRARYCURRENTVERSION);

        result = stream;

        rpLODAtomicCache = LODAtomicNewCache();

        /* Read the atomic cache from the stream. */
        RWASSERT(structSize <= sizeof(RpLODAtomicCache));

        RwStreamRead(stream, rpLODAtomicCache, structSize);

        (void)RwMemNative32(rpLODAtomicCache, sizeof(RpLODAtomicCache));

        /* Check if we need to read in any additional lod geometry. */
        for (i = 0; i < RPLODATOMICMAXLOD; ++i)
        {
            if ((rpLODAtomicCache->lodGeometry[i] != NULL) &&
                (i != rpLODAtomicCache->curLOD))
            {
                if (RwStreamFindChunk(stream, rwID_GEOMETRY, NULL,
                                      &version))
                {
                    RWASSERT(version >= rwLIBRARYBASEVERSION);
                    RWASSERT(version <= rwLIBRARYCURRENTVERSION);

                    lodGeometry = RpGeometryStreamRead(stream);
					//@{ Jaewon 20050412
					// Sometimes 'lodGeometry' is null... why?--;
					if(lodGeometry)
					//@} Jaewon
						RpGeometryAddRef(lodGeometry);

                    rpLODAtomicCache->lodGeometry[i] = lodGeometry;
                }
            }
        }

        /* Set the missing curLOD geometry.
         */
        if (rpLODAtomicCache->curLOD >= 0)
        {
            lodGeometry = RpAtomicGetGeometry(atomic);
            RpGeometryAddRef(lodGeometry);

            rpLODAtomicCache->lodGeometry[rpLODAtomicCache->curLOD] =
                lodGeometry;
        }

        /* Reset the select LOD to the default.
         */
        rpLODAtomicCache->atomicSelectLOD = LODAtomicCallBackSelectLOD;

        /* Hook into the atomic render.
         */
        rpLODAtomicCache->hook = FALSE;
        LODAtomicHookRender(atomic, rpLODAtomicCache);

        RPLODOFFSET(atomic, offset) = rpLODAtomicCache;
    }
    else
    {
        RPLODOFFSET(atomic, offset) = NULL;
    }

    RWRETURN(result);
}

/************************************************************************
 */
static RwStream    *
LODAtomicCallBackWriteChunk(RwStream * stream,
                            RwInt32 __RWUNUSED__ len,
                            RpAtomic * atomic,
                            RwInt32 offset, RwInt32 __RWUNUSED__ size)
{
    RwInt32             i;
    RpLODAtomicCache   *rpLODAtomicCache;

    RWFUNCTION(RWSTRING("LODAtomicCallBackWriteChunk"));

    rpLODAtomicCache = RPLODOFFSET(atomic, offset);

    if (rpLODAtomicCache != NULL)
    {
        RpLODAtomicCache    tmpLODAtomicCache;

        RwStreamWriteChunkHeader(stream, rwID_STRUCT,
                                 sizeof(RpLODAtomicCache));

        /* Write out the cache. */
        tmpLODAtomicCache = *rpLODAtomicCache;

		//>@ 2005.3.30 gemani
		tmpLODAtomicCache.atomicRender		= NULL;
		tmpLODAtomicCache.atomicSelectLOD	= NULL;
		for (i = 0; i < RPLODATOMICMAXLOD; ++i)
		{
			if(tmpLODAtomicCache.lodGeometry[i])
				//@{ 20050513 DDonSS : Warning을 피하기 위해 형변환 처리함
				tmpLODAtomicCache.lodGeometry[i]	= (RpGeometry*)1;
				//@} DDonSS
		}

        (void)RwMemLittleEndian32(&tmpLODAtomicCache,
                          sizeof(tmpLODAtomicCache));

        //RwStreamWrite(stream, rpLODAtomicCache,
        //              sizeof(RpLODAtomicCache));

		RwStreamWrite(stream, &tmpLODAtomicCache,
                      sizeof(RpLODAtomicCache));
		// <@

        /* Write out any lod geometry. We skip the cur lod to avoid
         * duplication.
         */
        for (i = 0; i < RPLODATOMICMAXLOD; ++i)
        {
            if ((rpLODAtomicCache->lodGeometry[i] != NULL) &&
                (i != rpLODAtomicCache->curLOD))
                RpGeometryStreamWrite(rpLODAtomicCache->lodGeometry[i],
                                      stream);
        }
    }

    RWRETURN(stream);
}

/************************************************************************
 */
static              RwInt32
LODAtomicCallBackGetChunkSize(RpAtomic * atomic,
                              RwInt32 offset, RwInt32 size)
{
    RwInt32             i, totalsize, geomsize;
    RpLODAtomicCache   *rpLODAtomicCache;

    RWFUNCTION(RWSTRING("LODAtomicCallBackGetChunkSize"));

    size = 0;
    totalsize = 0;

    rpLODAtomicCache = RPLODOFFSET(atomic, offset);

    if (rpLODAtomicCache != NULL)
    {
        totalsize = sizeof(RpLODAtomicCache) + rwCHUNKHEADERSIZE;

        for (i = 0; i < RPLODATOMICMAXLOD; ++i)
        {
            if ((rpLODAtomicCache->lodGeometry[i] != NULL) &&
                (i != rpLODAtomicCache->curLOD))
            {
                geomsize =
                    RpGeometryStreamGetSize
                    (rpLODAtomicCache->lodGeometry[i]) +
                    rwCHUNKHEADERSIZE;

                totalsize += geomsize;
            }
        }
    }

    RWRETURN(totalsize);
}

/************************************************************************
 */
static void        *
LODAtomicCallBackConstructor(RpAtomic * atomic,
                             RwInt32 offset, RwInt32 __RWUNUSED__ size)
{
    RWFUNCTION(RWSTRING("LODAtomicCallBackConstructor"));

    RPLODOFFSET(atomic, offset) = NULL;

    RWRETURN(atomic);
}

/************************************************************************
 */
static void        *
LODAtomicCallBackCopy(RpAtomic * dstAtomic,
                      RpAtomic * srcAtomic,
                      RwInt32 offset, RwInt32 __RWUNUSED__ size)
{
    RpLODAtomicCache   *rpSrcLODAtomicCache, *rpDstLODAtomicCache;

    RWFUNCTION(RWSTRING("LODAtomicCallBackCopy"));

    RWASSERT(dstAtomic);
    RWASSERT(srcAtomic);

    rpSrcLODAtomicCache = RPLODOFFSET(srcAtomic, offset);

    /* Check if the src is empty. */
    if (rpSrcLODAtomicCache != NULL)
    {
        /* Check if we need to create a new cache for the destination. */
        rpDstLODAtomicCache = RPLODOFFSET(dstAtomic, offset);

        if (rpDstLODAtomicCache == NULL)
            rpDstLODAtomicCache = LODAtomicNewCache();

        if (rpDstLODAtomicCache != NULL)
            LODAtomicCopyCache(rpDstLODAtomicCache,
                               rpSrcLODAtomicCache);

        RPLODOFFSET(dstAtomic, offset) = rpDstLODAtomicCache;
    }

    RWRETURN(srcAtomic);
}

/************************************************************************
 */
static void        *
LODAtomicCallBackDestructor(RpAtomic * atomic,
                            RwInt32 offset, RwInt32 __RWUNUSED__ size)
{
    RpLODAtomicCache   *rpLODAtomicCache;

    RWFUNCTION(RWSTRING("LODAtomicCallBackDestructor"));

    rpLODAtomicCache = RPLODOFFSET(atomic, offset);

    if (rpLODAtomicCache != NULL)
    {
        LODAtomicUnHookRender(atomic, rpLODAtomicCache);

        LODAtomicDestroyCache(rpLODAtomicCache);

        RPLODOFFSET(atomic, offset) = NULL;
    }

    RWRETURN(atomic);
}



static RwInt32 _rpLODAtomicCacheFreeListBlockSize = 4096 / sizeof(RpLODAtomicCache),
               _rpLODAtomicCacheFreeListPreallocBlocks = 1;
static RwFreeList _rpLODAtomicCacheFreeList;

/**
 * \ingroup rplodatm
 * \ref RpLODAtomicCacheSetFreeListCreateParams allows the developer to specify
 * how many RpLODAtomicCache s to preallocate space for.
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
RpLODAtomicCacheSetFreeListCreateParams( RwInt32 blockSize, RwInt32 numBlocksToPrealloc )
{
#if 0
    /*
     * Can not use debugging macros since the debugger is not initialized before
     * RwEngineInit.
     */
    /* Do NOT comment out RWAPIFUNCTION as gnumake verify will not function */
    RWAPIFUNCTION(RWSTRING("RpLODAtomicCacheSetFreeListCreateParams"));
#endif
    _rpLODAtomicCacheFreeListBlockSize = blockSize;
    _rpLODAtomicCacheFreeListPreallocBlocks = numBlocksToPrealloc;
#if 0
    RWRETURNVOID();
#endif
}




/************************************************************************
 */
static void        *
LODAtomicCallBackOpen(void *ptr,
                      RwInt32 __RWUNUSED__ offset,
                      RwInt32 __RWUNUSED__ size)
{
    void               *result;

    RWFUNCTION(RWSTRING("LODAtomicCallBackOpen"));

    result = ptr;

    RpLODAtomicGlobals.atmCacheFreeList =
        RwFreeListCreateAndPreallocateSpace(sizeof(RpLODAtomicCache), _rpLODAtomicCacheFreeListBlockSize,
        sizeof(RwUInt32), _rpLODAtomicCacheFreeListPreallocBlocks, &_rpLODAtomicCacheFreeList,
        rwID_LODATMPLUGIN | rwMEMHINTDUR_EVENT);

    if (RpLODAtomicGlobals.atmCacheFreeList == NULL)
        result = NULL;

    RWRETURN(result);
}

/************************************************************************
 */
static void        *
LODAtomicCallBackClose(void *ptr,
                       RwInt32 __RWUNUSED__ offset,
                       RwInt32 __RWUNUSED__ size)
{
    RWFUNCTION(RWSTRING("LODAtomicCallBackClose"));

    if (RpLODAtomicGlobals.atmCacheFreeList != NULL)
        RwFreeListDestroy(RpLODAtomicGlobals.atmCacheFreeList);

    RpLODAtomicGlobals.atmCacheFreeList = NULL;

    RWRETURN(ptr);
}

/************************************************************************
 *
 *      Function:       RpLODPluginAttach()
 *
 *      Description:    This function is called by the application to indicate
 *                      that the LOD plugin should be used. The call
 *                      to this function should be placed after RwEngineInit
 *
 *      Parameters:     None.
 *
 *      Return Value:   True on success, false otherwise
 *
 ************************************************************************/

/**
 * \ingroup rplodatm
 * \ref RpLODAtomicPluginAttach is used to attach the LOD plugin to the
 * RenderWare system to enable the use of level-of-detail modelling for
 * atomics. The LOD plugin must be attached between initializing the
 * system with RwEngineInit and opening it with RwEngineOpen.
 *
 * Note that the include file rplodatm.h is required and must be included
 * by an application wishing to use this plugin.  The LOD plugin library is
 * contained in the file rplodatm.lib.
 *
 * \return True on success, false otherwise

 * \see RpLODAtomicForAllLODGeometries
 * \see RpLODAtomicGetCurrentLOD
 * \see RpLODAtomicGetGeometry
 * \see RpLODAtomicGetRange
 * \see RpLODAtomicHookRender
 * \see RpLODAtomicSelectLOD
 * \see RpLODAtomicSetCamera
 * \see RpLODAtomicSetCurrentLOD
 * \see RpLODAtomicSetGeometry
 * \see RpLODAtomicSetLODCallBack
 * \see RpLODAtomicSetRange
 * \see RpLODAtomicUnHookRender
 */
RwBool
RpLODAtomicPluginAttach(void)
{
    RwInt32             offset;
    RwBool              result = FALSE;

    RWAPIFUNCTION(RWSTRING("RpLODAtomicPluginAttach"));

    /* Extend the global data block to include LOD globals */

    /* Need to initialise the global here before engine open to pick up the
     * offsets */
    RpLODAtomicGlobals.engineOffset =
        RwEngineRegisterPlugin(0,
                               rwID_LODATMPLUGIN,
                               (RwPluginObjectConstructor)
                               LODAtomicCallBackOpen,
                               (RwPluginObjectDestructor)
                               LODAtomicCallBackClose);

    RpLODAtomicGlobals.atmCacheOffset =
        RpAtomicRegisterPlugin(sizeof(RpLODAtomicCache *),
                               rwID_LODATMPLUGIN,
                               (RwPluginObjectConstructor)
                               LODAtomicCallBackConstructor,
                               (RwPluginObjectDestructor)
                               LODAtomicCallBackDestructor,
                               (RwPluginObjectCopy)
                               LODAtomicCallBackCopy);

    offset =
        RpAtomicRegisterPluginStream(rwID_LODATMPLUGIN,
                                     (RwPluginDataChunkReadCallBack)
                                     LODAtomicCallBackReadChunk,
                                     (RwPluginDataChunkWriteCallBack)
                                     LODAtomicCallBackWriteChunk,
                                     (RwPluginDataChunkGetSizeCallBack)
                                     LODAtomicCallBackGetChunkSize);

    RpLODAtomicGlobals.atmCamera = NULL;
    RpLODAtomicGlobals.atmCacheFreeList = NULL;

    result = (RpLODAtomicGlobals.atmCacheOffset >= 0);

    RWRETURN(result);
}

/************************************************************************
 *
 *      Function:       RpLODAtomicSetGeometry()
 *
 *      Description:    This function is called to set geometry for
 *                      each LOD.
 *
 *      Parameters:     atomic : The LOD's atomic.
 *                      lodIdx : LOD level.
 *                      geom   : Geometry for this LOD.
 *
 *      Return Value:   The atomic if successful, NULL otherwise.
 *
 ************************************************************************/

/**
 * \ingroup rplodatm
 * \ref RpLODAtomicSetGeometry is used to define the LOD geometry
 * for the specified atomic.
 *
 * The index of the highest LOD geometry is zero (this is the base geometry).
 * The maximum number of possible levels is 10 (indices 0 to 9). If the LOD
 * geometry for the given index already exists, it is first destroyed before
 * the new one is registered. For each LOD level, the default geometry is
 * NULL, which is an allowed value; for any levels with NULL geometry the
 * atomic is not rendered.
 *
 * The LOD plugin must be attached before using this function.
 *
 * \param atomic  The LOD's atomic.
 * \param lodIdx  LOD level.
 * \param lodGeom   Geometry for this LOD.
 *
 * \return The atomic if successful, NULL otherwise.

 * \see RpLODAtomicForAllLODGeometries
 * \see RpLODAtomicGetCurrentLOD
 * \see RpLODAtomicGetGeometry
 * \see RpLODAtomicGetRange
 * \see RpLODAtomicHookRender
 * \see RpLODAtomicPluginAttach
 * \see RpLODAtomicSelectLOD
 * \see RpLODAtomicSetCamera
 * \see RpLODAtomicSetCurrentLOD
 * \see RpLODAtomicSetLODCallBack
 * \see RpLODAtomicSetRange
 * \see RpLODAtomicUnHookRender
 */
RpAtomic           *
RpLODAtomicSetGeometry(RpAtomic * atomic,
                       RwInt32 lodIdx, RpGeometry * lodGeom)
{
    RpLODAtomicCache   *rpLODAtomicCache;

    RpAtomic           *result;
    RwInt32             offset;

    RWAPIFUNCTION(RWSTRING("RpLODAtomicSetGeometry"));

    result = NULL;
    offset = RpLODAtomicGlobals.atmCacheOffset;

    RWASSERT(offset >= 0);
    RWASSERT(atomic != NULL);

    RWASSERT(lodIdx < RPLODATOMICMAXLOD);
    RWASSERT(lodIdx >= 0);

    rpLODAtomicCache = RPLODOFFSET(atomic, offset);

    if (rpLODAtomicCache == NULL)
    {
        rpLODAtomicCache = LODAtomicNewCache();

        RPLODOFFSET(atomic, offset) = rpLODAtomicCache;

        if (rpLODAtomicCache != NULL)
            LODAtomicDefaultLODZero(atomic, rpLODAtomicCache);
    }

    if (rpLODAtomicCache != NULL)
    {
        if (lodGeom != NULL)
            RpGeometryAddRef(lodGeom);

        if (rpLODAtomicCache->lodGeometry[lodIdx] != NULL)
            RpGeometryDestroy(rpLODAtomicCache->lodGeometry[lodIdx]);

        rpLODAtomicCache->lodGeometry[lodIdx] = lodGeom;

        result = LODAtomicHookRender(atomic, rpLODAtomicCache);
    }

    RWRETURN(result);
}

/************************************************************************
 *
 *      Function:       RpLODAtomicGetGeometry()
 *
 *      Description:    This function is called to get the geometry for
 *                      the LOD.
 *
 *      Parameters:     atomic : The LOD's atomic.
 *                      lodIdx : LOD level.
 *
 *      Return Value:   The geometry if successful, NULL otherwise.
 *
 ************************************************************************/

/**
 * \ingroup rplodatm
 * \ref RpLODAtomicGetGeometry is used to retrieve the LOD geometry
 * in the specified atomic that is indexed with the given value.
 *
 * The index of the highest LOD geometry is zero (this is the base
 * geometry).  The maximum number of possible levels is 10 (indices 0 to 9)
 *
 * The LOD plugin must be attached before using this function.
 *
 * \param atomic  The LOD's atomic.
 * \param lodIdx  LOD level.
 *
 * \return The geometry if successful, NULL otherwise.

 * \see RpLODAtomicForAllLODGeometries
 * \see RpLODAtomicGetCurrentLOD
 * \see RpLODAtomicGetRange
 * \see RpLODAtomicHookRender
 * \see RpLODAtomicPluginAttach
 * \see RpLODAtomicSelectLOD
 * \see RpLODAtomicSetCamera
 * \see RpLODAtomicSetCurrentLOD
 * \see RpLODAtomicSetGeometry
 * \see RpLODAtomicSetLODCallBack
 * \see RpLODAtomicSetRange
 * \see RpLODAtomicUnHookRender
 */
RpGeometry         *
RpLODAtomicGetGeometry(RpAtomic * atomic, RwInt32 lodIdx)
{
    RpLODAtomicCache   *rpLODAtomicCache;

    RpGeometry         *result;
    RwInt32             offset;

    RWAPIFUNCTION(RWSTRING("RpLODAtomicGetGeometry"));

    result = NULL;
    offset = RpLODAtomicGlobals.atmCacheOffset;

    RWASSERT(offset >= 0);
    RWASSERT(atomic != NULL);

    RWASSERT(lodIdx < RPLODATOMICMAXLOD);
    RWASSERT(lodIdx >= 0);

    rpLODAtomicCache = RPLODOFFSET(atomic, offset);

    if (rpLODAtomicCache != NULL)
        result = rpLODAtomicCache->lodGeometry[lodIdx];

    RWRETURN(result);
}

/************************************************************************
 *
 *      Function:       RpLODAtomicSetCurrentLOD()
 *
 *      Description:    This function is called to set LOD for the atomic.
 *
 *      Parameters:     atomic : The LOD's atomic.
 *                      lodIdx : LOD level.
 *
 *      Return Value:   The atomic if successful, NULL otherwise.
 *
 ************************************************************************/

/**
 * \ingroup rplodatm
 * \ref RpLODAtomicSetCurrentLOD sets the LOD for the atomic. This disables 
 * the automatic switching of the LOD during rendering. Instead, only the set 
 * LOD geometry is used by the atomic during rendering.
 *
 * \param atomic  The LOD's atomic.
 * \param lodIdx  LOD level.
 *
 * \return The atomic if successful, NULL otherwise.

 * \see RpLODAtomicForAllLODGeometries
 * \see RpLODAtomicGetCurrentLOD
 * \see RpLODAtomicGetGeometry
 * \see RpLODAtomicGetRange
 * \see RpLODAtomicHookRender
 * \see RpLODAtomicPluginAttach
 * \see RpLODAtomicSelectLOD
 * \see RpLODAtomicSetCamera
 * \see RpLODAtomicSetGeometry
 * \see RpLODAtomicSetLODCallBack
 * \see RpLODAtomicSetRange
 * \see RpLODAtomicUnHookRender
 */
RpAtomic           *
RpLODAtomicSetCurrentLOD(RpAtomic * atomic, RwInt32 lodIdx)
{
    RpGeometry         *lodGeom;
    RpLODAtomicCache   *rpLODAtomicCache;

    RpAtomic           *result;
    RwInt32             offset;

    RWAPIFUNCTION(RWSTRING("RpLODAtomicSetCurrentLOD"));

    result = NULL;
    offset = RpLODAtomicGlobals.atmCacheOffset;

    RWASSERT(offset >= 0);
    RWASSERT(atomic != NULL);

    RWASSERT(lodIdx < RPLODATOMICMAXLOD);
    RWASSERT(lodIdx >= 0);

    rpLODAtomicCache = RPLODOFFSET(atomic, offset);

    if (rpLODAtomicCache != NULL)
    {
        lodGeom = rpLODAtomicCache->lodGeometry[lodIdx];

        if (rpLODAtomicCache->curLOD != lodIdx)
            RpAtomicSetGeometry(atomic, lodGeom, 0);

        rpLODAtomicCache->curLOD = lodIdx;

        result = LODAtomicUnHookRender(atomic, rpLODAtomicCache);
    }

    RWRETURN(result);
}

/************************************************************************
 *
 *      Function:       RpLODAtomicGetCurrentLOD()
 *
 *      Description:    This function is called to get the current LOD.
 *
 *      Parameters:     atomic : The LOD's atomic.
 *
 *      Return Value:   The LOD if successful, -1 otherwise.
 *
 ************************************************************************/

/**
 * \ingroup rplodatm
 * \ref RpLODAtomicGetCurrentLOD gets the current LOD.
 *
 * \param atomic  The LOD's atomic.
 *
 * \return The LOD if successful, -1 otherwise.

 * \see RpLODAtomicForAllLODGeometries
 * \see RpLODAtomicGetGeometry
 * \see RpLODAtomicGetRange
 * \see RpLODAtomicHookRender
 * \see RpLODAtomicPluginAttach
 * \see RpLODAtomicSelectLOD
 * \see RpLODAtomicSetCamera
 * \see RpLODAtomicSetCurrentLOD
 * \see RpLODAtomicSetGeometry
 * \see RpLODAtomicSetLODCallBack
 * \see RpLODAtomicSetRange
 * \see RpLODAtomicUnHookRender
 */
RwInt32
RpLODAtomicGetCurrentLOD(RpAtomic * atomic)
{
    RpLODAtomicCache   *rpLODAtomicCache;

    RwInt32             result;
    RwInt32             offset;

    RWAPIFUNCTION(RWSTRING("RpLODAtomicGetCurrentLOD"));

    result = -1;
    offset = RpLODAtomicGlobals.atmCacheOffset;

    RWASSERT(offset >= 0);
    RWASSERT(atomic != NULL);

    rpLODAtomicCache = RPLODOFFSET(atomic, offset);

    if (rpLODAtomicCache != NULL)
        result = rpLODAtomicCache->curLOD;

    RWRETURN(result);
}

/************************************************************************
 *
 *      Function:       RpLODAtomicSetRange()
 *
 *      Description:    This function is called to set the maximum range for
 *                      the LOD.
 *
 *      Parameters:     atomic : The LOD's atomic.
 *                      farFrange : Maximum Range of the LODs.
 *
 *      Return Value:   The atomic if successful, NULL otherwise.
 *
 ************************************************************************/

/**
 * \ingroup rplodatm
 * \ref RpLODAtomicSetRange RpLODAtomicSetRange is used to define
 * the range from the camera over which LOD switching for the specified
 * atomic takes place.
 *
 * The range is used by the atomic's default LOD callback to determine
 * which LOD geometry should be used to render the atomic, given the
 * distance of the atomic from the camera.  The default LOD callback
 * calculates the LOD level using linear interpolation between zero
 * and the far range.  For distances beyond the far range only the lowest
 * LOD geometry is used for rendering the atomic.

 * The LOD plugin must be attached before using this function.
 *
 * \param atomic  The LOD's atomic.
 * \param farRange  Maximum Range of the LODs.
 *
 * \return The atomic if successful, NULL otherwise.

 * \see RpLODAtomicForAllLODGeometries
 * \see RpLODAtomicGetCurrentLOD
 * \see RpLODAtomicGetGeometry
 * \see RpLODAtomicGetRange
 * \see RpLODAtomicHookRender
 * \see RpLODAtomicPluginAttach
 * \see RpLODAtomicSelectLOD
 * \see RpLODAtomicSetCamera
 * \see RpLODAtomicSetCurrentLOD
 * \see RpLODAtomicSetGeometry
 * \see RpLODAtomicSetLODCallBack
 * \see RpLODAtomicUnHookRender
 */
RpAtomic           *
RpLODAtomicSetRange(RpAtomic * atomic, RwReal farRange)
{
    RpLODAtomicCache   *rpLODAtomicCache;

    RpAtomic           *result;
    RwInt32             offset;

    RWAPIFUNCTION(RWSTRING("RpLODAtomicSetRange"));

    result = NULL;
    offset = RpLODAtomicGlobals.atmCacheOffset;

    RWASSERT(offset >= 0);
    RWASSERT(atomic != NULL);
    RWASSERT(farRange > (RwReal) 0.0);

    rpLODAtomicCache = RPLODOFFSET(atomic, offset);

    if (rpLODAtomicCache == NULL)
    {
        rpLODAtomicCache = LODAtomicNewCache();

        RPLODOFFSET(atomic, offset) = rpLODAtomicCache;

        if (rpLODAtomicCache != NULL)
            LODAtomicDefaultLODZero(atomic, rpLODAtomicCache);
    }

    if (rpLODAtomicCache != NULL)
    {
        rpLODAtomicCache->farRange = farRange;
        rpLODAtomicCache->invFarRange =
            (RwReal) RPLODATOMICMAXLOD / farRange;

        result = LODAtomicHookRender(atomic, rpLODAtomicCache);
    }

    RWRETURN(result);
}

/************************************************************************
 *
 *      Function:       RpLODAtomicGetRange()
 *
 *      Description:    This function is called to get the far set.
 *
 *      Parameters:     atomic : The LOD's atomic.
 *
 *      Return Value:   The far range if successful, -1 otherwise.
 *
 ************************************************************************/

/**
 * \ingroup rplodatm
 * \ref RpLODAtomicGetRange gets the far range
 *
 * \param atomic  The LOD's atomic.
 *
 * \return The far range if successful, -1 otherwise.

 *
 * \see RpLODAtomicForAllLODGeometries
 * \see RpLODAtomicGetCurrentLOD
 * \see RpLODAtomicGetGeometry
 * \see RpLODAtomicHookRender
 * \see RpLODAtomicPluginAttach
 * \see RpLODAtomicSelectLOD
 * \see RpLODAtomicSetCamera
 * \see RpLODAtomicSetCurrentLOD
 * \see RpLODAtomicSetGeometry
 * \see RpLODAtomicSetLODCallBack
 * \see RpLODAtomicSetRange
 * \see RpLODAtomicUnHookRender
 */
RwReal
RpLODAtomicGetRange(RpAtomic * atomic)
{
    RpLODAtomicCache   *rpLODAtomicCache;

    RwReal              result;
    RwInt32             offset;

    RWAPIFUNCTION(RWSTRING("RpLODAtomicGetRange"));

    result = (RwReal) - 1.0;
    offset = RpLODAtomicGlobals.atmCacheOffset;

    RWASSERT(offset >= 0);
    RWASSERT(atomic != NULL);

    rpLODAtomicCache = RPLODOFFSET(atomic, offset);

    if (rpLODAtomicCache != NULL)
        result = (RwReal) rpLODAtomicCache->farRange;

    RWRETURN(result);
}

/************************************************************************
 *
 *      Function:       RpLODAtomicSetCamera()
 *
 *      Description:    This function is called to set the current camera
 *                      used for rendering. This camera is used for selecting
 *                      LOD based on the distance of object from the viewer.
 *
 *      Parameters:     The LOD's camera.
 *
 *      Return Value:   None.
 *
 ************************************************************************/

/**
 * \ingroup rplodatm
 * \ref RpLODAtomicSetCamera is used to register the specified camera
 * as the one for determining the range of an atomic and, therefore,
 * the LOD geometry that is used for rendering.  Usually, this camera is
 * the same one that is used for rendering LOD atomics. This camera is
 * used for selecting LOD based on the distance of object from the viewer.
 *
 * The LOD plugin must be attached before using this function.
 *
 * \param camera  The LOD's camera.
 * \see RpLODAtomicForAllLODGeometries
 * \see RpLODAtomicGetCurrentLOD
 * \see RpLODAtomicGetGeometry
 * \see RpLODAtomicGetRange
 * \see RpLODAtomicHookRender
 * \see RpLODAtomicPluginAttach
 * \see RpLODAtomicSelectLOD
 * \see RpLODAtomicSetCurrentLOD
 * \see RpLODAtomicSetGeometry
 * \see RpLODAtomicSetLODCallBack
 * \see RpLODAtomicSetRange
 * \see RpLODAtomicUnHookRender
 */
void
RpLODAtomicSetCamera(RwCamera * camera)
{
    RWAPIFUNCTION(RWSTRING("RpLODAtomicSetCamera"));

    RpLODAtomicGlobals.atmCamera = camera;

    RWRETURNVOID();
}

/************************************************************************
 *
 *      Function:       RpLODAtomicSetLODCallBack()
 *
 *      Description:    This function is called to set call bacl for
 *                      selecting the LOD.
 *
 *      Parameters:     atomic : The LOD atomic
 *                       callback : The callback function.
 *
 *      Return Value:   The atomic if successful. NULL otherwise.
 *
 ************************************************************************/

/**
 * \ingroup rplodatm
 * \ref RpLODAtomicSetLODCallBack is used to define a callback function
 * that determines which LOD geometry should be used to render the specified
 * atomic.
 *
 * A default callback is defined for all atomics that calculates the LOD
 * level using linear interpolation between zero and the LOD far range.
 * For distances beyond the far range the lowest LOD geometry is used for
 * rendering the atomic.
 *
 * The format of the callback function is:
 *
 *    RwInt32 (*RpLODAtomicLODCallBack) (RpAtoimc *atomic);
 *
 * where the return value is the index of the LOD geometry.
 *
 * The LOD plugin must be attached before using this function.
 *
 * \param atomic  The LOD atomic
 * \param callback  The callback function.
 *
 * \return The atomic if successful. NULL otherwise.

 * \see RpLODAtomicForAllLODGeometries
 * \see RpLODAtomicGetCurrentLOD
 * \see RpLODAtomicGetGeometry
 * \see RpLODAtomicGetRange
 * \see RpLODAtomicHookRender
 * \see RpLODAtomicPluginAttach
 * \see RpLODAtomicSelectLOD
 * \see RpLODAtomicSetCamera
 * \see RpLODAtomicSetCurrentLOD
 * \see RpLODAtomicSetGeometry
 * \see RpLODAtomicSetRange
 * \see RpLODAtomicUnHookRender
 */
RpAtomic           *
RpLODAtomicSetLODCallBack(RpAtomic * atomic,
                          RpLODAtomicLODCallBack callback)
{
    RpLODAtomicCache   *rpLODAtomicCache;

    RpAtomic           *result;
    RwInt32             offset;

    RWAPIFUNCTION(RWSTRING("RpLODAtomicSetLODCallBack"));

    result = NULL;
    offset = RpLODAtomicGlobals.atmCacheOffset;

    RWASSERT(offset >= 0);
    RWASSERT(atomic != NULL);
    RWASSERT(callback != NULL);

    rpLODAtomicCache = RPLODOFFSET(atomic, offset);

    if (rpLODAtomicCache == NULL)
    {
        rpLODAtomicCache = LODAtomicNewCache();

        RPLODOFFSET(atomic, offset) = rpLODAtomicCache;

        if (rpLODAtomicCache != NULL)
            LODAtomicDefaultLODZero(atomic, rpLODAtomicCache);
    }

    if (rpLODAtomicCache != NULL)
    {
        rpLODAtomicCache->atomicSelectLOD = callback;

        result = LODAtomicHookRender(atomic, rpLODAtomicCache);
    }

    RWRETURN(result);
}

/************************************************************************
 *
 *      Function:       RpLODAtomicSelectLOD()
 *
 *      Description:    This function is called to apply the select LOD
 *                      callback to set the current geometry without
 *                      rendering.
 *
 *      Parameters:     atomic : The LOD atomic
 *
 *      Return Value:   The atomic if successful. NULL otherwise.
 *
 ************************************************************************/

/**
 * \ingroup rplodatm
 * \ref RpLODAtomicSelectLOD applies the select LOD
 * callback to set the current geometry without rendering.
 *
 * \param atomic  The LOD atomic
 *
 * \return The atomic if successful. NULL otherwise.

 *
 * \see RpLODAtomicForAllLODGeometries
 * \see RpLODAtomicGetCurrentLOD
 * \see RpLODAtomicGetGeometry
 * \see RpLODAtomicGetRange
 * \see RpLODAtomicHookRender
 * \see RpLODAtomicPluginAttach
 * \see RpLODAtomicSetCamera
 * \see RpLODAtomicSetCurrentLOD
 * \see RpLODAtomicSetGeometry
 * \see RpLODAtomicSetLODCallBack
 * \see RpLODAtomicSetRange
 * \see RpLODAtomicUnHookRender
 */
RpAtomic           *
RpLODAtomicSelectLOD(RpAtomic * atomic)
{
    RpLODAtomicCache   *rpLODAtomicCache;

    RpAtomic           *result;
    RwInt32             offset;

    RWAPIFUNCTION(RWSTRING("RpLODAtomicSelectLOD"));

    result = NULL;
    offset = RpLODAtomicGlobals.atmCacheOffset;

    RWASSERT(offset >= 0);
    RWASSERT(atomic != NULL);

    rpLODAtomicCache = RPLODOFFSET(atomic, offset);

    if (rpLODAtomicCache != NULL)
        result = LODAtomicSelectLOD(atomic, rpLODAtomicCache);

    RWRETURN(result);
}

/************************************************************************
 *
 *      Function:       RpLODAtomicForAllLOD()
 *
 *      Description:    This function is called to apply the callback to
 *                      all LOD in the atomic.
 *
 *      Parameters:     atomic : The LOD atomic
 *                      callback : The callback function.
 *                      pData : Application private data.
 *
 *      Return Value:   The atomic if successful. NULL otherwise.
 *
 ************************************************************************/

/**
 * \ingroup rplodatm
 * \ref RpLODAtomicForAllLODGeometries applies the callback to
 * all LOD in the atomic.
 *
 * \param atomic  The LOD atomic
 * \param callback  The callback function.
 * \param pData  Application private data.
 *
 * \return The atomic if successful. NULL otherwise.

 * \see RpLODAtomicGetCurrentLOD
 * \see RpLODAtomicGetGeometry
 * \see RpLODAtomicGetRange
 * \see RpLODAtomicHookRender
 * \see RpLODAtomicPluginAttach
 * \see RpLODAtomicSelectLOD
 * \see RpLODAtomicSetCamera
 * \see RpLODAtomicSetCurrentLOD
 * \see RpLODAtomicSetGeometry
 * \see RpLODAtomicSetLODCallBack
 * \see RpLODAtomicSetRange
 * \see RpLODAtomicUnHookRender
 */
RpAtomic           *
RpLODAtomicForAllLODGeometries(RpAtomic * atomic,
                               RpGeometryCallBack callback, void *pData)
{
    RwInt32             i;
    RpGeometry         *lodGeom = NULL;
    RpLODAtomicCache   *rpLODAtomicCache;

    RpAtomic           *result;
    RwInt32             offset;

    RWAPIFUNCTION(RWSTRING("RpLODAtomicForAllLODGeometries"));

    result = NULL;
    offset = RpLODAtomicGlobals.atmCacheOffset;

    RWASSERT(offset >= 0);
    RWASSERT(atomic != NULL);
    RWASSERT(callback != NULL);

    rpLODAtomicCache = RPLODOFFSET(atomic, offset);

    if (rpLODAtomicCache != NULL)
    {
        result = atomic;

        for (i = 0; i < RPLODATOMICMAXLOD; ++i)
        {
            if (rpLODAtomicCache->lodGeometry[i] != NULL)
                lodGeom = (callback)
                    (rpLODAtomicCache->lodGeometry[i], pData);

            if (lodGeom == NULL)
                break;
        }
    }

    RWRETURN(result);
}

/************************************************************************
 *
 *      Function:       RpLODAtomicHookRender()
 *
 *      Description:    This function is called to hook into the atomic's
 *                      callback. This effectively enable the LOD.
 *
 *      Parameters:     atomic : The LOD atomic
 *
 *      Return Value:   The atomic if successful. NULL otherwise.
 *
 ************************************************************************/

/**
 * \ingroup rplodatm
 * \ref RpLODAtomicHookRender hooks into the atomic's
 * callback. This effectively enables the LOD.
 *
 * \param atomic  The LOD atomic
 *
 * \return The atomic if successful. NULL otherwise.

 * \see RpLODAtomicForAllLODGeometries
 * \see RpLODAtomicGetCurrentLOD
 * \see RpLODAtomicGetGeometry
 * \see RpLODAtomicGetRange
 * \see RpLODAtomicPluginAttach
 * \see RpLODAtomicSelectLOD
 * \see RpLODAtomicSetCamera
 * \see RpLODAtomicSetCurrentLOD
 * \see RpLODAtomicSetGeometry
 * \see RpLODAtomicSetLODCallBack
 * \see RpLODAtomicSetRange
 * \see RpLODAtomicUnHookRender
 */
RpAtomic           *
RpLODAtomicHookRender(RpAtomic * atomic)
{
    RpLODAtomicCache   *rpLODAtomicCache;
    RpAtomic           *result;
    RwInt32             offset;

    RWAPIFUNCTION(RWSTRING("RpLODAtomicHookRender"));

    result = NULL;
    offset = RpLODAtomicGlobals.atmCacheOffset;

    RWASSERT(offset >= 0);
    RWASSERT(atomic != NULL);

    rpLODAtomicCache = RPLODOFFSET(atomic, offset);

    if (rpLODAtomicCache != NULL)
        result = LODAtomicHookRender(atomic, rpLODAtomicCache);

    RWRETURN(result);
}

/************************************************************************
 *
 *      Function:       RpLODAtomicUnHookRender()
 *
 *      Description:    This function is called to unhook into the atomic's
 *                      callback. This effectively disable the LOD.
 *
 *      Parameters:     atomic : The LOD atomic
 *
 *      Return Value:   The atomic if successful. NULL otherwise.
 *
 ************************************************************************/

/**
 * \ingroup rplodatm
 * \ref RpLODAtomicUnHookRender unhooks into the atomic's
 * callback. This effectively disable the LOD.
 *
 * \param atomic  The LOD atomic
 *
 * \return The atomic if successful. NULL otherwise.
 * \see RpLODAtomicForAllLODGeometries
 * \see RpLODAtomicGetCurrentLOD
 * \see RpLODAtomicGetGeometry
 * \see RpLODAtomicGetRange
 * \see RpLODAtomicHookRender
 * \see RpLODAtomicPluginAttach
 * \see RpLODAtomicSelectLOD
 * \see RpLODAtomicSetCamera
 * \see RpLODAtomicSetCurrentLOD
 * \see RpLODAtomicSetGeometry
 * \see RpLODAtomicSetLODCallBack
 * \see RpLODAtomicSetRange
 */
RpAtomic           *
RpLODAtomicUnHookRender(RpAtomic * atomic)
{
    RpLODAtomicCache   *rpLODAtomicCache;
    RpAtomic           *result;
    RwInt32             offset;

    RWAPIFUNCTION(RWSTRING("RpLODAtomicUnHookRender"));

    result = NULL;
    offset = RpLODAtomicGlobals.atmCacheOffset;

    RWASSERT(offset >= 0);
    RWASSERT(atomic != NULL);

    rpLODAtomicCache = RPLODOFFSET(atomic, offset);

    if (rpLODAtomicCache != NULL)
        result = LODAtomicUnHookRender(atomic, rpLODAtomicCache);

    RWRETURN(result);
}
