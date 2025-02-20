#ifndef TEAMSKIN_H
#define TEAMSKIN_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpteam.h"

#include "teamtypes.h"

/*===========================================================================*
 *--- Private Variables -----------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/
typedef struct SkinCyclicCache SkinCyclicCache;  /* 16 bytes total. */
struct SkinCyclicCache
{
    void     *unaligned;        /* 4 bytes. */
    RwMatrix *matrixCache;      /* 4 bytes. */
    RwUInt32  maxMatrices;      /* 4  bytes. */
    RwUInt32  nextMatrices;     /* 4  bytes. */
};

typedef struct SkinCache SkinCache; /* 12 bytes total. */
struct SkinCache
{
    void     *unaligned;            /* 4 bytes. */
    RwMatrix *matrixCache;          /* 4 bytes. */
    RwUInt32  number;               /* 4 bytes. */
};

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/
extern void
_rpTeamSkinDestroyMatrixCache(SkinCache *skinCache);

extern RwMatrix *
_rpTeamSkinCreateMatrixCache( SkinCache *skinCache,
                              RpHAnimHierarchy *hierarchy );

extern RpAtomic *
_rpTeamSkinAttachCustomPipeline(RpAtomic *atomic);

extern RwBool
_rpTeamSkinCustomPipelineCreate(void);

extern void
_rpTeamSkinCustomPipelineDestroy(void);

extern RxPipeline *
_rpTeamSkinGetCustomPipeline(void);

/*===========================================================================*
 *--- Pipeline access data --------------------------------------------------*
 *===========================================================================*/
#if (defined(TEAMGLOBALPOINTS))

extern RwMatrix *_rpTeamSkinMatrixCache;

#define _rpTeamSkinGetCurrentMatrixCache()                           \
    _rpTeamSkinMatrixCache

#define _rpTeamSkinSetCurrentMatrixCache(cache)                      \
    _rpTeamSkinMatrixCache = cache

#else /* (defined(TEAMGLOBALPOINTS)) */

#define _rpTeamSkinGetCurrentMatrixCache()
#define _rpTeamSkinSetCurrentMatrixCache(cache)

#endif /* (defined(TEAMGLOBALPOINTS)) */

#endif /* TEAMSKIN_H */

