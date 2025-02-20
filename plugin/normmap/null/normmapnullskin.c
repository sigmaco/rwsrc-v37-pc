/****************************************************************************
 *
 * normmapnullskin.c
 *
 ****************************************************************************/

#include "rwcore.h"
#include "rpworld.h"
#include "rpskin.h"
#include "rpdbgerr.h"

#include "rpnormmap.h"
#include "normmap.h"
#include "normmapnull.h"

/*--- Global ---*/

/*--- Local Variables ---*/

static RxPipeline   *NormalMapAtomicPipeline = NULL;
static RxPipeline   *NormalMapAtomicSkinPipeline = NULL;

/*
 ***************************************************************************
 */
void
RpNormMapAtomicInitialize(RpAtomic *atomic, RpNormMapAtomicPipeline pipeline)
{

    RWAPIFUNCTION(RWSTRING("RpNormMapAtomicInitialize"));
    RWASSERT(atomic != NULL);

    if (pipeline == rpNORMMAPATOMICSKINNEDPIPELINE)
    {
        RpAtomicSetPipeline(atomic, NormalMapAtomicSkinPipeline);
    }
    else
    {
        RWASSERT(pipeline == rpNORMMAPATOMICSTATICPIPELINE);

        RpAtomicSetPipeline(atomic, NormalMapAtomicPipeline);
    }

    RWRETURNVOID();
}

/*
 ***************************************************************************
 */
static RxPipeline *
NormalMapCreateAtomicSkinPipeline(void)
{
    RxPipeline  *pipe;
    RxPipeline  *clone;

    RWFUNCTION(RWSTRING("NormalMapCreateAtomicSkinPipeline"));

    /* We create the GENERIC pipe by cloning the default pipe. */
    pipe = RpAtomicGetDefaultPipeline();
    RWASSERT(NULL != pipe);

    clone = RxPipelineClone(pipe);
    RWASSERT(NULL != clone);

    /* Identify the pipeline with it's plugin and id. */
    clone->pluginId = rwPLUGIN_ID;
    clone->pluginData = rpNORMMAPATOMICSKINNEDPIPELINE;

    RWRETURN(clone);
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
     * Create a new skin atomic pipeline
     */
    NormalMapAtomicSkinPipeline = NormalMapCreateAtomicSkinPipeline();
    if (NormalMapAtomicSkinPipeline == NULL)
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

    if (NormalMapAtomicSkinPipeline != NULL)
    {
        RxPipelineDestroy(NormalMapAtomicSkinPipeline);
        NormalMapAtomicSkinPipeline = NULL;
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
RpNormMapGetAtomicPipeline(RpNormMapAtomicPipeline pipeline)
{
    RWAPIFUNCTION(RWSTRING("RpNormMapGetAtomicPipeline"));

    if (pipeline == rpNORMMAPATOMICSKINNEDPIPELINE)
    {
        RWRETURN(NormalMapAtomicSkinPipeline);
    }
    else
    {
        RWRETURN(NormalMapAtomicPipeline);
    }
}
