/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>
#include "rpdbgerr.h"

#include "rpteam.h"

#include "teamskin.h"
#include "team.h"

/*
 *

  CUSTOM FAST SKIN

 *
 */

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/
#if (defined(TEAMGLOBALPOINTS))
RwMatrix *_rpTeamSkinMatrixCache;
#endif /* (defined(TEAMGLOBALPOINTS)) */

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/
#define ROUNDUP16(x) (((RwUInt32)(x) + 16 - 1) & ~(16 - 1))

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/
void
_rpTeamSkinDestroyMatrixCache(SkinCache *skinCache)
{
    RWFUNCTION(RWSTRING("_rpTeamSkinDestroyMatrixCache"));
    RWASSERT(NULL != skinCache);

    if(skinCache->unaligned)
    {
        RwFree(skinCache->unaligned);
    }

    skinCache->unaligned = (void *)NULL;
    skinCache->matrixCache = (RwMatrix *)NULL;
    skinCache->number = 0;

    RWRETURNVOID();
}

RwMatrix *
_rpTeamSkinCreateMatrixCache( SkinCache *skinCache,
                              RpHAnimHierarchy *hierarchy )
{
    RwUInt32 size;
    RwUInt32 num;

    RWFUNCTION(RWSTRING("_rpTeamSkinCreateMatrixCache"));
    RWASSERT(NULL != skinCache);
    RWASSERT(NULL != hierarchy);
    RWASSERT(65 > hierarchy->numNodes);
    RWASSERT(33 > hierarchy->numNodes);

    num = hierarchy->numNodes;

    /* Determine size with slack for 16 byte alignment */
    size = (sizeof(RwMatrix) * num) + 15;
    skinCache->unaligned = RwMalloc(size,
        rwID_TEAMPLUGIN | rwMEMHINTDUR_EVENT);
    if( NULL == skinCache->unaligned )
    {
        RWERROR((E_RW_NOMEM, size));
        RWRETURN((RwMatrix *)NULL);
    }

    /* Clean memory. */
    memset(skinCache->unaligned, 0, size);

    skinCache->matrixCache = (RwMatrix *)ROUNDUP16(skinCache->unaligned);
    skinCache->number = num;

    RWRETURN(skinCache->matrixCache);
}

RpAtomic *
_rpTeamSkinAttachCustomPipeline(RpAtomic *atomic)
{
    RxPipeline *pipeline;

    RWFUNCTION(RWSTRING("_rpTeamSkinAttachCustomPipeline"));
    RWASSERT(NULL != atomic);

    pipeline = _rpTeamSkinGetCustomPipeline();
    RWASSERT(NULL != pipeline);

    /* Need to attach the custom pipeline. */
    atomic = RpAtomicSetPipeline(atomic, pipeline);
    RWASSERT(NULL != atomic);

    RWRETURN(atomic);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/
