/*----------------------------------------------------------------------*
 *                                                                      *
 * Module  :                                                            *
 *                                                                      *
 * Purpose :                                                            *
 *                                                                      *
 * FX      :                                                            *
 *----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*
 *-   Includes                                                         -*
 *----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*==== RW libs includes =====*/
#include "rpplugin.h"
#include <rpdbgerr.h>
#include <rwcore.h>

#include "matfx.h"
#include "effectpipes.h"

/*----------------------------------------------------------------------*
 *-   Local Types                                                      -*
 *----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*
 *-   Local/static Globals                                             -*
 *----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*
 *-   Globals across program                                           -*
 *----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*
 *-   Defines                                                          -*
 *----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*
 *-   Functions                                                        -*
 *----------------------------------------------------------------------*/

/*--- Create and destroy pipelines ------------------------------------------*/
RwBool
_rpMatFXPipelinesCreate(void)
{
    RWFUNCTION(RWSTRING("_rpMatFXPipelinesCreate"));
    RWRETURN(TRUE);
}

RwBool
_rpMatFXPipelinesDestroy(void)
{
    RWFUNCTION(RWSTRING("_rpMatFXPipelinesDestroy"));
    RWRETURN(TRUE);
}

/*--- Attach pipelines ------------------------------------------------------*/
RpAtomic           *
_rpMatFXPipelineAtomicSetup(RpAtomic * atomic)
{
    RWFUNCTION(RWSTRING("_rpMatFXPipelineAtomicSetup"));
    RWASSERT(atomic);
    RWRETURN(atomic);
}

RpWorldSector      *
_rpMatFXPipelineWorldSectorSetup(RpWorldSector * worldSector)
{
    RWFUNCTION(RWSTRING("_rpMatFXPipelineWorldSectorSetup"));
    RWASSERT(worldSector);
    RWRETURN(worldSector);
}

/*--- Upload texture --------------------------------------------------------*/

/*--- Device data fucntions -------------------------------------------------*/
RwBool
_rpMatFXSetupDualRenderState(MatFXDualData * __RWUNUSEDRELEASE__ dualData,
                             RwRenderState __RWUNUSED__ nState)
{
    RWFUNCTION(RWSTRING("_rpMatFXSetupDualRenderState"));
    RWASSERT(dualData);
    RWRETURN(TRUE);
}

RwTexture *
_rpMatFXSetupBumpMapTexture(const RwTexture *baseTexture,
                            const RwTexture *effectTexture)
{
    RwTexture *texture;
    RWFUNCTION(RWSTRING("_rpMatFXSetupBumpMapTexture"));

    texture = _rpMatFXTextureMaskCreate(baseTexture, effectTexture);

    RWRETURN(texture);
}

