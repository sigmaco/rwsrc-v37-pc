/****************************************************************************
 *
 * normmapd3d9plain.c
 *
 ****************************************************************************/

#include <d3d9.h>

#include "rwcore.h"
#include "rpworld.h"
#include "rpdbgerr.h"

#include "rpnormmap.h"
#include "normmap.h"
#include "normmapd3d9.h"

/*--- Local Variables ---*/

static RxPipeline   *NormalMapAtomicPipeline = NULL;

/*
 ***************************************************************************
 */
void
RpNormMapAtomicInitialize(RpAtomic *atomic, RpNormMapAtomicPipeline pipeline)
{
    RWAPIFUNCTION(RWSTRING("RpNormMapAtomicInitialize"));
    RWASSERT(atomic != NULL);
    RWASSERT(pipeline == rpNORMMAPATOMICSTATICPIPELINE);

    if (NormalMapSupported)
    {
        RpGeometry *geometry;
        RpD3D9GeometryUsageFlag flags;

        geometry = RpAtomicGetGeometry(atomic);

        flags = RpD3D9GeometryGetUsageFlags(geometry);

        flags |= rpD3D9GEOMETRYUSAGE_CREATETANGENTS;

        RpD3D9GeometrySetUsageFlags(geometry, flags);
    }

    RpAtomicSetPipeline(atomic, NormalMapAtomicPipeline);

    RWRETURNVOID();
}

/*
 ***************************************************************************
 */
RwBool
NormalMapCreatePipelines(void)
{
    RWFUNCTION(RWSTRING("NormalMapCreatePipelines"));

    /*
     * Create a new atomic pipeline
     */
    NormalMapAtomicPipeline = NormalMapCreateAtomicPipeline();
    if (NormalMapAtomicPipeline == NULL)
    {
        RWRETURN(FALSE);
    }

    /*
     * Create a new world sector pipeline
     */
    _rpNormMapWorldSectorPipeline = NormalMapCreateWorldSectorPipeline();
    if (_rpNormMapWorldSectorPipeline == NULL)
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}

/*
 ***************************************************************************
 */
void
NormalMapDestroyPipelines(void)
{
    RWFUNCTION(RWSTRING("NormalMapDestroyPipelines"));

    if (_rpNormMapWorldSectorPipeline != NULL)
    {
        RxPipelineDestroy(_rpNormMapWorldSectorPipeline);
        _rpNormMapWorldSectorPipeline = NULL;
    }

    if (NormalMapAtomicPipeline != NULL)
    {
        RxPipelineDestroy(NormalMapAtomicPipeline);
        NormalMapAtomicPipeline = NULL;
    }

    RWRETURNVOID();
}

/*
 ***************************************************************************
 */
RxPipeline *
RpNormMapGetAtomicPipeline(RpNormMapAtomicPipeline pipeline __RWUNUSED__)
{
    RWAPIFUNCTION(RWSTRING("RpNormMapGetAtomicPipeline"));

    RWRETURN(NormalMapAtomicPipeline);
}
