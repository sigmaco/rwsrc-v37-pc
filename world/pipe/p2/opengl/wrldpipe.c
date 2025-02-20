/* The documentation in here is platform-specific versions of function-
   -descriptions for functions in bapipew.c */
/*
 * Manipulating world object custom pipelines (platform-specific)
 *
 * Copyright (c) Criterion Software Limited
 */
#include "rwcore.h"

#include "balight.h"
#include "baworld.h"

#include "bapipew.h"

#include "wrldpipe.h"

#include "nodeOpenGLAtomicAllInOne.h"
#include "nodeOpenGLWorldSectorAllInOne.h"
#include "opengllights.h"

/****************************************************************************
 _rxWorldDevicePluginAttach()
 */

RwBool
_rxWorldDevicePluginAttach(void)
{
    RwBool result = FALSE; /* fail, unless explicitly set TRUE */

    RWFUNCTION(RWSTRING("_rxWorldDevicePluginAttach"));

    result = _rpOpenGLLightPluginAttach();

    RWRETURN(result);
}

/**
 * \ingroup rpmaterialopengl
 * \page RpMaterialGetDefaultPipelineplatform RpMaterialGetDefaultPipeline (platform-specific)
 *
 * The default material pipeline for OpenGL are ignored.
 *
 * \see RpAtomicGetDefaultPipeline
 * \see \ref RpAtomicGetDefaultPipelineplatform
 * \see RpAtomicGetPipeline
 * \see RpAtomicSetDefaultPipeline
 * \see RpAtomicSetPipeline
 * \see RpMaterialGetDefaultPipeline
 * \see RpMaterialGetPipeline
 * \see RpMaterialSetDefaultPipeline
 * \see RpMaterialSetPipeline
 * \see RpWorldGetSectorPipeline
 * \see RpWorldSetSectorPipeline
 * \see RpWorldSectorGetPipeline
 * \see RpWorldSectorSetPipeline
 * \see RpWorldGetDefaultSectorPipeline
 * \see \ref RpWorldGetDefaultSectorPipelineplatform
 * \see RpWorldSetDefaultSectorPipeline
 *
 */
RwBool
_rpCreatePlatformMaterialPipelines(void)
{
    RWFUNCTION(RWSTRING("_rpCreatePlatformMaterialPipelines"));

    RWRETURN( TRUE );
}

void
_rpDestroyPlatformMaterialPipelines(void)
{
    RWFUNCTION(RWSTRING("_rpDestroyPlatformMaterialPipelines"));

    RWRETURNVOID();
}

/**
 * \ingroup rpworldsubopengl
 * \page RpWorldGetDefaultSectorPipelineplatform RpWorldGetDefaultSectorPipeline (platform-specific)
 *
 * The default world sector object pipeline for OpenGL is shown below. Note
 * that this is designed specifically to leverage hardware transformation and
 * lighting support. If you wish to use material pipelines which do
 * transformation and lighting (and anything else on top of that) themselves,
 * then you will need to replace the default object pipeline with the
 * generic world sector object pipeline.
 *
 * The default world sector object pipeline:
 * \verbatim
     OpenGLWorldSectorAllInOne.csl
   \endverbatim
 * \see RpAtomicGetDefaultPipeline
 * \see \ref RpAtomicGetDefaultPipelineplatform
 * \see RpAtomicGetPipeline
 * \see RpAtomicSetDefaultPipeline
 * \see RpAtomicSetPipeline
 * \see RpMaterialGetDefaultPipeline
 * \see \ref RpMaterialGetDefaultPipelineplatform
 * \see RpMaterialGetPipeline
 * \see RpMaterialSetDefaultPipeline
 * \see RpMaterialSetPipeline
 * \see RpWorldGetSectorPipeline
 * \see RpWorldSetSectorPipeline
 * \see RpWorldSectorGetPipeline
 * \see RpWorldSectorSetPipeline
 * \see RpWorldGetDefaultSectorPipeline
 * \see RpWorldSetDefaultSectorPipeline
 * \see \ref RpWorldGetDefaultSectorPipelineplatform
 */
RwBool
_rpCreatePlatformWorldSectorPipelines(void)
{
    RxPipeline        *pipe;


    RWFUNCTION(RWSTRING("_rpCreatePlatformWorldSectorPipelines"));

    pipe = RxPipelineCreate();

    if ( NULL != pipe )
    {
        RxLockedPipe    *lockedPipe;


        pipe->pluginId = rwID_WORLDPLUGIN;

        lockedPipe = RxPipelineLock(pipe);
        if ( NULL != lockedPipe )
        {
            lockedPipe = RxLockedPipeAddFragment( lockedPipe,
                                                  NULL,
                                                  RxNodeDefinitionGetOpenGLWorldSectorAllInOne(),
                                                  NULL );

            lockedPipe = RxLockedPipeUnlock( lockedPipe );
            RWASSERT( pipe == (RxPipeline *)lockedPipe );

            if ( NULL != lockedPipe )
            {
                RXPIPELINEGLOBAL(platformWorldSectorPipeline) = pipe;
                RpWorldSetDefaultSectorPipeline( pipe );

                RWRETURN( TRUE );
            }
        }

        RxPipelineDestroy(pipe);
    }

    RWRETURN(FALSE);
}

void
_rpDestroyPlatformWorldSectorPipelines(void)
{
    RWFUNCTION(RWSTRING("_rpDestroyPlatformWorldSectorPipelines"));

    RpWorldSetDefaultSectorPipeline( NULL );
    if ( NULL != RXPIPELINEGLOBAL(platformWorldSectorPipeline) )
    {
        RxPipelineDestroy( RXPIPELINEGLOBAL(platformWorldSectorPipeline) );
        RXPIPELINEGLOBAL(platformWorldSectorPipeline) = (RxPipeline *)NULL;
    }

    RWRETURNVOID();
}

/**
 * \ingroup rpatomicopengl
 * \page RpAtomicGetDefaultPipelineplatform RpAtomicGetDefaultPipeline (platform-specific)
 *
 * The default atomic object pipeline for OpenGL is shown below. Note
 * that this is designed specifically to leverage hardware transformation and
 * lighting support. If you wish to use material pipelines which do
 * transformation and lighting (and anything else on top of that) themselves,
 * then you will need to replace the default object pipeline with the
 * generic atomic object pipeline.
 *
 * The OpenGL default atomic object pipeline:
 * \verbatim
     OpenGLAtomicAllInOne.csl
   \endverbatim
 * \see RpAtomicGetDefaultPipeline
 * \see RpAtomicGetPipeline
 * \see RpAtomicSetDefaultPipeline
 * \see RpAtomicSetPipeline
 * \see RpMaterialGetDefaultPipeline
 * \see \ref RpMaterialGetDefaultPipelineplatform
 * \see RpMaterialGetPipeline
 * \see RpMaterialSetDefaultPipeline
 * \see RpMaterialSetPipeline
 * \see RpWorldGetDefaultSectorPipeline
 * \see RpWorldGetSectorPipeline
 * \see RpWorldSectorGetPipeline
 * \see RpWorldSectorSetPipeline
 * \see RpWorldSetDefaultSectorPipeline
 * \see RpWorldSetSectorPipeline
 * \see \ref RpWorldGetDefaultSectorPipelineplatform
 *
 */
RwBool
_rpCreatePlatformAtomicPipelines(void)
{
    RxPipeline        *pipe;


    RWFUNCTION(RWSTRING("_rpCreatePlatformAtomicPipelines"));

    pipe = RxPipelineCreate();

    if ( NULL != pipe )
    {
        RxLockedPipe    *lockedPipe;


        pipe->pluginId = rwID_WORLDPLUGIN;

        lockedPipe = RxPipelineLock(pipe);
        if ( NULL != lockedPipe )
        {
            lockedPipe = RxLockedPipeAddFragment( lockedPipe,
                                                  NULL,
                                                  RxNodeDefinitionGetOpenGLAtomicAllInOne(),
                                                  NULL );

            lockedPipe = RxLockedPipeUnlock( lockedPipe );
            RWASSERT( pipe == (RxPipeline *)lockedPipe );

            if ( NULL != lockedPipe )
            {
                RXPIPELINEGLOBAL(platformAtomicPipeline) = pipe;
                RpAtomicSetDefaultPipeline( pipe );

                if ( FALSE != _rwOpenGLLightsOpen() )
                {
                    RWRETURN( TRUE );
                }
            }
        }

        RxPipelineDestroy(pipe);
    }

    RWRETURN(FALSE);
}

void
_rpDestroyPlatformAtomicPipelines(void)
{
    RWFUNCTION(RWSTRING("_rpDestroyPlatformAtomicPipelines"));

    RpAtomicSetDefaultPipeline( NULL );
    if ( NULL != RXPIPELINEGLOBAL(platformAtomicPipeline) )
    {
        RxPipelineDestroy( RXPIPELINEGLOBAL(platformAtomicPipeline) );
        RXPIPELINEGLOBAL(platformAtomicPipeline) = (RxPipeline *)NULL;
    }

    _rwOpenGLLightsClose();

    RWRETURNVOID();
}
