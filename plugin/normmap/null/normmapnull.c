/****************************************************************************
 *
 * normmapnull.c
 *
 ****************************************************************************/

#include "rwcore.h"
#include "rpworld.h"
#include "rpdbgerr.h"

#include "rpnormmap.h"
#include "normmap.h"
#include "normmapnull.h"

/*--- Global ---*/

/*--- Local Variables ---*/

static RwInt32 NormalMapNumInstances = 0;

/*
 ***************************************************************************
 */
RxPipeline *
NormalMapCreateAtomicPipeline(void)
{
    RxPipeline  *pipe;
    RxPipeline  *clone;

    RWFUNCTION(RWSTRING("NormalMapCreateAtomicPipeline"));

    /* We create the GENERIC pipe by cloning the default pipe. */
    pipe = RpAtomicGetDefaultPipeline();
    RWASSERT(NULL != pipe);

    clone = RxPipelineClone(pipe);
    RWASSERT(NULL != clone);

    /* Identify the pipeline with it's plugin and id. */
    clone->pluginId = rwPLUGIN_ID;
    clone->pluginData = rpNORMMAPATOMICSTATICPIPELINE;

    RWRETURN(clone);
}

/*
 ***************************************************************************
 */
RxPipeline *
NormalMapCreateWorldSectorPipeline(void)
{
    RxPipeline  *pipe;
    RxPipeline  *clone;

    RWFUNCTION(RWSTRING("NormalMapCreateWorldSectorPipeline"));

    /* We create the GENERIC pipe by cloning the default pipe. */
    pipe = RpWorldGetDefaultSectorPipeline();
    RWASSERT(NULL != pipe);

    clone = RxPipelineClone(pipe);
    RWASSERT(NULL != clone);

    /* Identify the pipeline with it's plugin and id. */
    clone->pluginId = rwPLUGIN_ID;
    clone->pluginData = 0;

    RWRETURN(clone);
}

/*
 ***************************************************************************
 */
void *
_rpNormMapOpen(void *instance,
              RwInt32 offset __RWUNUSED__,
              RwInt32 size __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("_rpNormMapOpen"));

    if (NormalMapNumInstances == 0)
    {
        if (NormalMapCreatePipelines() == FALSE)
        {
            RWRETURN(NULL);
        }
    }

    NormalMapNumInstances++;

    RWRETURN(instance);
}

/*
 ***************************************************************************
 */
void *
_rpNormMapClose(void *instance,
           RwInt32 offset __RWUNUSED__,
           RwInt32 size __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("_rpNormMapClose"));

    NormalMapNumInstances--;

    if (NormalMapNumInstances <= 0)
    {
        NormalMapDestroyPipelines();
    }
    
    RWRETURN(instance);
}

/*
 ***************************************************************************
 */
void
RpNormMapWorldEnable(RpWorld *world)
{
    ObjectNormalMapExt *worldData;

    RWAPIFUNCTION(RWSTRING("RpNormMapWorldEnable"));
    RWASSERT(world != NULL);

    worldData = WORLDGETNORMALMAP(world);

    if (worldData->enabled == FALSE)
    {
        worldData->enabled = TRUE;
    }

    RWRETURNVOID();
}
