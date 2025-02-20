/***************************************************************************
 *                                                                         *
 * Module  :  rtgncpip.c                                                   *
 *                                                                         *
 * Purpose :  Toolkit implementing the generic C based powerpipe nodes     *
 *            and pipelines                                                *
 *                                                                         *
 **************************************************************************/

/****************************************************************************
 Includes
 */
#include "rwcore.h"
#include "rpworld.h"

#include "rpdbgerr.h"
#include "rperror.h"

#include "rtgncpip.h"

/****************************************************************************
 Defines
 */

#define RTGENCPIPEGLOBAL(var) (gencpipeglobals.var)

/****************************************************************************
 Globals (across program)
 */

struct staticglobals
{
    RxPipeline           *genericIm3DTransformPipeline;
    rwIm3DRenderPipelines genericIm3DRenderPipelines;

};

struct staticglobals gencpipeglobals;

/****************************************************************************
 Local (static) Globals
 */

#if (defined(RWDEBUG))
long _rtGenCPipeStackDepth = 0;
#endif /* (defined(RWDEBUG)) */



/****************************************************************************

        Generic Core Pipes

 ***************************************************************************/

/**
 * \defgroup rtgencpipe RtGenCPipe
 * \ingroup altpipes
 *
 * Generic Pipes
 */

/**
 * \ingroup rtgencpipe
 * \page rtgencpipeoverview RtGenCPipe Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h
 * \li \b Libraries: rwcore
 * \li \b Plugin \b attachments: None
 *
 * \subsection gencpipeoverview Overview
 * This toolkit provides the generic pipelines which have been part of RwCore
 * in previous releases of RenderWare. These pipeline work on all platforms
 * but perform much less well than the platform specific pipelines.
 */

/**
 * \defgroup rtgencpiperwim3d RwIm3d
 * \ingroup rtgencpipe
 *
 * Immediate mode pipes
 */

/**
 * \defgroup rtgencpiperpatomic RpAtomic
 * \ingroup rtgencpipe
 *
 * Atomic pipes
 */

/**
 * \defgroup rtgencpiperpmaterial RpMaterial
 * \ingroup rtgencpipe
 *
 * Material pipes
 */

/**
 * \defgroup rtgencpiperpworldsub RpWorld
 * \ingroup rtgencpipe
 *
 * World pipes
 */


/**
 * \ingroup rtgencpiperwim3d
 * \ref RwIm3DGetGenericTransformPipeline returns the default
 * generic RwIm3D transform pipeline.
 *
 * Most generic RenderWare nodes only support triangle lists and line lists
 * and so in RwIm3D pipelines constructed from
 * generic nodes, triangle strips and fans and polylines are automatically
 * expanded (into triangle lists and line lists respectively) in
 * ImmMangleTriangleIndices.csl or ImmMangleLineIndices.csl.
 * Similarly, unindexed primitives are not supported by most nodes, so
 * RwIm3DRenderPrimitive() creates indices and renders indexed primitives.
 * Hence there is only one triangle and one line render pipeline for RwIm3D.
 * These are both shown below. Platform-specific nodes and pipelines are
 * available which can handle more cases and which usually operate more
 * efficiently, having been designed for a specific platform.
 *
 * The default RwIm3D transform pipeline is platform-specific, see
 * \ref RwIm3DGetTransformPipelineplatform for details.
 *
 * \return A pointer to the default generic RwIm3D transform pipeline on
 * success, otherwise NULL.
 *
 * \verbatim
   The default generic RwIm3D transform pipeline:

     ImmInstance.csl
      v
     Transform.csl
      v
     ImmStash.csl
     \endverbatim
 *
 * \see RwIm3DGetGenericRenderPipeline
 * \see RwIm3DEnd
 * \see RwIm3DEnd
 * \see RwIm3DRenderIndexedPrimitive
 * \see RwIm3DRenderLine
 * \see RwIm3DRenderPrimitive
 * \see RwIm3DRenderTriangle
 * \see RwIm3DTransform
 * \see RwIm3DGetRenderPipeline
 * \see \ref RwIm3DGetRenderPipelineplatform
 * \see RwIm3DSetRenderPipeline
 * \see \ref RwIm3DGetTransformPipelineplatform
 * \see RwIm3DSetTransformPipeline
 */
RxPipeline         *
RwIm3DGetGenericTransformPipeline(void)
{
    RWAPIFUNCTION(RWSTRING("RwIm3DGetGenericTransformPipeline"));
    RWRETURN(RTGENCPIPEGLOBAL(genericIm3DTransformPipeline));
}

/**
 * \ingroup rtgencpiperwim3d
 * \ref RwIm3DGetGenericRenderPipeline returns the
 * default generic RwIm3D render pipeline associated with a given
 * primitive type
 *
 * Most generic RenderWare nodes only support triangle lists and line lists
 * and so in RwIm3D pipelines constructed from
 * generic nodes, triangle strips and fans and polylines are automatically
 * expanded (into triangle lists and line lists respectively) in
 * ImmMangleTriangleIndices.csl or ImmMangleLineIndices.csl.
 * Similarly, unindexed primitives are not supported by most nodes, so
 * RwIm3DRenderPrimitive() creates indices and renders indexed primitives.
 * Hence there is only one RwIm3D triangle and one line render pipeline. These
 * are both shown below. Platform-specific nodes and pipelines are available
 * which can handle more cases.
 *
 * The default RwIm3D transform pipeline is platform-specific, see
 * \ref RwIm3DGetRenderPipelineplatform for details.
 *
 * \param type  An \ref RwPrimitiveType equal to the primitive type.
 *
 * \return A pointer to the default generic RwIm3D render pipeline for the
 * specified primitive type on success, otherwise NULL.
 *
 * \verbatim
   The default generic RwIm3D triangle render pipeline:

     ImmRenderSetup.csl
      v
     ImmMangleTriangleIndices.csl
      v
     CullTriangle.csl
      v
     ClipTriangle.csl
      v
     SubmitTriangle.csl

   The default generic RwIm3D line render pipeline:

     ImmRenderSetup.csl
      v
     ImmMangleLineIndices.csl
      v
     ClipLine.csl
      v
     SubmitLine.csl
     \endverbatim
 *
 * \see RwIm3DGetGenericTransformPipeline
 * \see RwIm3DEnd
 * \see RwIm3DRenderIndexedPrimitive
 * \see RwIm3DRenderLine
 * \see RwIm3DRenderPrimitive
 * \see RwIm3DRenderTriangle
 * \see RwIm3DTransform
 * \see \ref RwIm3DGetRenderPipelineplatform
 * \see RwIm3DSetRenderPipeline
 * \see RwIm3DGetTransformPipeline
 * \see \ref RwIm3DGetTransformPipelineplatform
 * \see RwIm3DSetTransformPipeline
 */
RxPipeline         *
RwIm3DGetGenericRenderPipeline(RwPrimitiveType type)
{
    RWAPIFUNCTION(RWSTRING("RwIm3DGetGenericRenderPipeline"));
    switch (type)
    {
        case rwPRIMTYPETRILIST:
            RWRETURN(RTGENCPIPEGLOBAL(genericIm3DRenderPipelines).triList);
        case rwPRIMTYPETRISTRIP:
            RWRETURN(RTGENCPIPEGLOBAL(genericIm3DRenderPipelines).triStrip);
        case rwPRIMTYPETRIFAN:
            RWRETURN(RTGENCPIPEGLOBAL(genericIm3DRenderPipelines).triFan);
        case rwPRIMTYPELINELIST:
            RWRETURN(RTGENCPIPEGLOBAL(genericIm3DRenderPipelines).lineList);
        case rwPRIMTYPEPOLYLINE:
            RWRETURN(RTGENCPIPEGLOBAL(genericIm3DRenderPipelines).polyLine);
        default:
            RWERROR((E_RX_INVALIDPRIMTYPE, type));
            RWRETURN((RxPipeline *)NULL);
    }
}


/**
 * \ingroup rtgencpiperwim3d
 * \ref RtGenCPipeCreateGenericIm3DTransformPipeline
 *
 * Internal Use
 */
RwBool
RtGenCPipeCreateGenericIm3DTransformPipeline(void)
{
    RxPipeline         *pipe;

    RWAPIFUNCTION(RWSTRING("RtGenCPipeCreateGenericIm3DTransformPipeline"));

    pipe = RxPipelineCreate();
    if (pipe != NULL)
    {
        RxPipeline         *lpipe;

        pipe->pluginId = rwID_COREPLUGIN;
        lpipe = RxPipelineLock(pipe);
        if (lpipe)
        {
            /* Lighting could be added later when wanted... */
            lpipe = RxLockedPipeAddFragment(lpipe,
                                            (RwUInt32 *)NULL,
                                            RxNodeDefinitionGetImmInstance(),
                                            RxNodeDefinitionGetTransform(),
                                            RxNodeDefinitionGetImmStash(),
                                            (RxNodeDefinition *)NULL);
            RWASSERT(lpipe != NULL);

            pipe = RxLockedPipeUnlock(lpipe);
            RWASSERT(pipe != NULL);
            RWASSERT(lpipe == pipe);

            if (pipe != NULL)
            {
                RTGENCPIPEGLOBAL(genericIm3DTransformPipeline) = pipe;
                RWRETURN(TRUE);
            }
        }
        /* Failed to lock/addfrag/unlock */
        _rxPipelineDestroy(pipe);
    }

    RWRETURN(FALSE);
}

/**
 * \ingroup rtgencpiperwim3d
 * \ref RtGenCPipeDestroyGenericIm3DTransformPipeline
 *
 * Internal Use
 */
void
RtGenCPipeDestroyGenericIm3DTransformPipeline(void)
{
    RWAPIFUNCTION(RWSTRING("RtGenCPipeDestroyGenericIm3DTransformPipeline"));

    if (RwIm3DGetTransformPipeline() == RTGENCPIPEGLOBAL(genericIm3DTransformPipeline))
    {
        RwIm3DSetTransformPipeline(NULL);
    }
    if (NULL != RTGENCPIPEGLOBAL(genericIm3DTransformPipeline))
    {
        _rxPipelineDestroy(RTGENCPIPEGLOBAL(genericIm3DTransformPipeline));
        RTGENCPIPEGLOBAL(genericIm3DTransformPipeline) = (RxPipeline *)NULL;
    }

    RWRETURNVOID();
}

/****************************************************************************
 Im3DCreateGenericTriRenderPipeline
 */
static RxPipeline  *
Im3DCreateGenericTriRenderPipeline(void)
{
    RxPipeline         *pipe;

    RWFUNCTION(RWSTRING("Im3DCreateGenericTriRenderPipeline"));

    pipe = RxPipelineCreate();
    if (pipe != NULL)
    {
        RxPipeline         *lpipe;

        pipe->pluginId = rwID_COREPLUGIN;
        lpipe = RxPipelineLock(pipe);
        if (lpipe != NULL)
        {
            RxPipelineNode     *setupNode, *mangleNode;

            lpipe =
                RxLockedPipeAddFragment(lpipe,
                                        (RwUInt32 *)NULL,
                                        RxNodeDefinitionGetImmRenderSetup(),
                                        RxNodeDefinitionGetImmMangleTriangleIndices(),
                                        RxNodeDefinitionGetCullTriangle(),
                                        RxNodeDefinitionGetClipTriangle(),
                                        RxNodeDefinitionGetSubmitTriangle(),
                                        (RxNodeDefinition *)NULL);
            RWASSERT(lpipe != NULL);

            /* Packets without indices go out through another output
             * to the same place (not that RwIm3D uses such packets atm,
             * that being the point of nodeImmMangle*Indices :) ) */
            setupNode =
                RxPipelineFindNodeByName(lpipe,
                                         "ImmRenderSetup.csl",
                                         (RxPipelineNode *)NULL,
                                         (RwInt32 *)NULL);
            RWASSERT(setupNode != NULL);
            mangleNode =
                RxPipelineFindNodeByName(lpipe,
                                         "ImmMangleTriangleIndices.csl",
                                         (RxPipelineNode *)NULL, (RwInt32 *) NULL);
            RWASSERT(mangleNode != NULL);
            lpipe = RxLockedPipeAddPath(lpipe,
                                        RxPipelineNodeFindOutputByName
                                        (setupNode,
                                         "ImmRenderSetupOutUnindexed"),
                                        RxPipelineNodeFindInput(mangleNode));
            RWASSERT(lpipe != NULL);

            pipe = RxLockedPipeUnlock(lpipe);
            RWASSERT(pipe != NULL);
            RWASSERT(pipe == lpipe);

            if (NULL != pipe)
            {
                RWRETURN(pipe);
            }
        }
        /* Failed to lock/addfrag/unlock */
        _rxPipelineDestroy(pipe);
    }

    RWRETURN((RxPipeline *)NULL);
}

/****************************************************************************
 Im3DCreateGenericLineRenderPipeline
 */
static RxPipeline  *
Im3DCreateGenericLineRenderPipeline(void)
{
    RxPipeline         *pipe;

    RWFUNCTION(RWSTRING("Im3DCreateGenericLineRenderPipeline"));

    pipe = RxPipelineCreate();
    if (pipe != NULL)
    {
        RxPipeline         *lpipe;

        pipe->pluginId = rwID_COREPLUGIN;
        lpipe = RxPipelineLock(pipe);
        if (lpipe != NULL)
        {
            RxPipelineNode     *setupNode, *mangleNode;

            lpipe =
                RxLockedPipeAddFragment(lpipe,
                                        (RwUInt32 *)NULL,
                                        RxNodeDefinitionGetImmRenderSetup(),
                                        RxNodeDefinitionGetImmMangleLineIndices(),
                                        RxNodeDefinitionGetClipLine(),
                                        RxNodeDefinitionGetSubmitLine(),
                                        (RxNodeDefinition *)NULL);
            RWASSERT(lpipe != NULL);

            /* Packets without indices go out through another output
             * to the same place (not that RwIm3D uses such packets atm,
             * that being the point of nodeImmMangle*Indices :) ) */
            setupNode =
                RxPipelineFindNodeByName(lpipe,
                                         "ImmRenderSetup.csl",
                                         (RxPipelineNode *)NULL, (RwInt32 *) NULL);
            RWASSERT(setupNode != NULL);
            mangleNode =
                RxPipelineFindNodeByName(lpipe,
                                         "ImmMangleLineIndices.csl",
                                         (RxPipelineNode *)NULL, (RwInt32 *) NULL);
            RWASSERT(mangleNode != NULL);
            lpipe = RxLockedPipeAddPath(lpipe,
                                        RxPipelineNodeFindOutputByName
                                        (setupNode,
                                         "ImmRenderSetupOutUnindexed"),
                                        RxPipelineNodeFindInput(mangleNode));
            RWASSERT(lpipe != NULL);

            pipe = RxLockedPipeUnlock(lpipe);
            RWASSERT(pipe != NULL);
            RWASSERT(pipe == lpipe);

            if (NULL != pipe)
            {
                RWRETURN(pipe);
            }
        }
        /* Failed to lock/addfrag/unlock */
        _rxPipelineDestroy(pipe);
    }

    RWRETURN((RxPipeline *)NULL);
}

/**
 * \ingroup rtgencpiperwim3d
 * \ref RtGenCPipeDestroyGenericIm3DRenderPipelines
 *
 * Internal Use
 */
void
RtGenCPipeDestroyGenericIm3DRenderPipelines(void)
{
    RWAPIFUNCTION(RWSTRING("RtGenCPipeDestroyGenericIm3DRenderPipelines"));

    /* Currently we cheat and expand TriFans/Strips to TriLists -
     * so there's only one triangles pipeline */
    if (RwIm3DGetRenderPipeline(rwPRIMTYPETRILIST) ==
            RTGENCPIPEGLOBAL(genericIm3DRenderPipelines).triList)
    {
        RwIm3DSetRenderPipeline((RxPipeline *)NULL, rwPRIMTYPETRILIST);
    }
    if (RwIm3DGetRenderPipeline(rwPRIMTYPETRIFAN) ==
            RTGENCPIPEGLOBAL(genericIm3DRenderPipelines).triList)
    {
        RwIm3DSetRenderPipeline((RxPipeline *)NULL, rwPRIMTYPETRIFAN);
    }
    if (RwIm3DGetRenderPipeline(rwPRIMTYPETRISTRIP) ==
            RTGENCPIPEGLOBAL(genericIm3DRenderPipelines).triList)
    {
        RwIm3DSetRenderPipeline((RxPipeline *)NULL, rwPRIMTYPETRISTRIP);
    }
    if (NULL != RTGENCPIPEGLOBAL(genericIm3DRenderPipelines).triList)
    {
        _rxPipelineDestroy(RTGENCPIPEGLOBAL(genericIm3DRenderPipelines).triList);
    }
    RTGENCPIPEGLOBAL(genericIm3DRenderPipelines).triList = (RxPipeline *)NULL;
    RTGENCPIPEGLOBAL(genericIm3DRenderPipelines).triFan = (RxPipeline *)NULL;
    RTGENCPIPEGLOBAL(genericIm3DRenderPipelines).triStrip = (RxPipeline *)NULL;

    /* Currently we cheat and expand polylines to linelists -
     * so there's only one lines pipeline */
    if (RwIm3DGetRenderPipeline(rwPRIMTYPELINELIST) ==
            RTGENCPIPEGLOBAL(genericIm3DRenderPipelines).lineList)
    {
        RwIm3DSetRenderPipeline((RxPipeline *)NULL, rwPRIMTYPELINELIST);
    }
    if (RwIm3DGetRenderPipeline(rwPRIMTYPEPOLYLINE) ==
            RTGENCPIPEGLOBAL(genericIm3DRenderPipelines).lineList)
    {
        RwIm3DSetRenderPipeline((RxPipeline *)NULL, rwPRIMTYPEPOLYLINE);
    }
    if (NULL != RTGENCPIPEGLOBAL(genericIm3DRenderPipelines).lineList)
    {
        _rxPipelineDestroy(RTGENCPIPEGLOBAL
                          (genericIm3DRenderPipelines).lineList);
    }
    RTGENCPIPEGLOBAL(genericIm3DRenderPipelines).lineList = (RxPipeline *)NULL;
    RTGENCPIPEGLOBAL(genericIm3DRenderPipelines).polyLine = (RxPipeline *)NULL;

    /* No generic pointlist pipe to destroy */

    RWRETURNVOID();
}

/**
 * \ingroup rtgencpiperwim3d
 * \ref RtGenCPipeCreateGenericIm3DRenderPipelines
 *
 * The render half of the RwIm3D pipeline is used by
 * RwIm3DRender[Indexed]Primitive() and uses the cached vertices from the
 * Im3DTransform pipeline, which is called from RwIm3DTransform (ok, so it
 * doesn't really work this way on H/W T&L cards). This creates generic
 * (that is C code nodes that'll run on all platforms) RwIm3D render pipelines
 * (there are several, to handle the various supported primitive types) and
 * stores pointers to them in RTGENCPIPEGLOBAL(genericIm3DRenderPipelines).
 */
RwBool
RtGenCPipeCreateGenericIm3DRenderPipelines(void)
{
    rwIm3DRenderPipelines *pipes;
    RxPipeline         *triPipe, *linePipe;

    RWAPIFUNCTION(RWSTRING("RtGenCPipeCreateGenericIm3DRenderPipelines"));

    /* Initialise to NULL pointers in case of failure */
    pipes = &RTGENCPIPEGLOBAL(genericIm3DRenderPipelines);
    pipes->triList = (RxPipeline *)NULL;
    pipes->triFan = (RxPipeline *)NULL;
    pipes->triStrip = (RxPipeline *)NULL;
    pipes->lineList = (RxPipeline *)NULL;
    pipes->polyLine = (RxPipeline *)NULL;
    pipes->pointList = (RxPipeline *)NULL;

    /* The generic pipelines cheat atm, one for triangles (fans
     * and strips munged to lists in ImmMangleIndices.csl) and
     * one for lines (polylines munged to linelists). */
    triPipe = Im3DCreateGenericTriRenderPipeline();
    linePipe = Im3DCreateGenericLineRenderPipeline();
    if ((triPipe != NULL) && (linePipe != NULL))
    {
        pipes->triList = triPipe;
        pipes->triFan = triPipe;
        pipes->triStrip = triPipe;
        pipes->lineList = linePipe;
        pipes->polyLine = linePipe;
        /* We do not provide a standard pipe to render points, this
         * is meant to be a data type free of interpretation by RW
         * to be processed by user plugins/pipelines (e.g Bèzier
         * control points) */

        RWRETURN(TRUE);
    }

    RtGenCPipeDestroyGenericIm3DRenderPipelines();

    RWRETURN(FALSE);
}


/****************************************************************************

        Generic World Pipes

 ***************************************************************************/

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))

/**
 * \ingroup rtgencpiperpmaterial
 * \ref RpMaterialGetGenericPipeline returns the default
 * generic material pipeline.
 *
 * This pipeline is constructed from generic nodes which will run on
 * all platforms. The default material pipeline is platform-specific
 * (see \ref RpMaterialGetDefaultPipeline).
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * The world plugin must be attached before using this function.
 *
 * \return A pointer to the default generic material pipeline
 * on success, otherwise NULL
 *
 *  The default generic material pipeline:
 * \verbatim

     Transform.csl
      v
     CullTriangle.csl
      v
     Light.csl
      v
     PostLight.csl
      v
     ClipTriangle.csl
      v
     SubmitTriangle.csl
   \endverbatim
 * \see RpAtomicGetPipeline
 * \see RpAtomicSetPipeline
 * \see RpAtomicGetGenericPipeline
 * \see RpAtomicGetDefaultPipeline
 * \see \ref RpAtomicGetDefaultPipelineplatform
 * \see RpAtomicSetDefaultPipeline
 * \see RpMaterialGetPipeline
 * \see RpMaterialSetPipeline
 * \see RpMaterialSetDefaultPipeline
 * \see RpMaterialGetDefaultPipeline
 * \see \ref RpMaterialGetDefaultPipelineplatform
 * \see RpWorldGetSectorPipeline
 * \see RpWorldSetSectorPipeline
 * \see RpWorldSectorGetPipeline
 * \see RpWorldSectorSetPipeline
 * \see RpWorldGetGenericSectorPipeline
 * \see RpWorldGetDefaultSectorPipeline
 * \see \ref RpWorldGetDefaultSectorPipelineplatform
 * \see RpWorldSetDefaultSectorPipeline
 */
RxPipeline *
RpMaterialGetGenericPipeline(void)
{
    RWAPIFUNCTION(RWSTRING("RpMaterialGetGenericPipeline"));

    RWRETURN(RpMaterialGetGenericPipelineMacro());
}

#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/****************************************************************************
 MaterialCreateGenericPipeline
 */
static RwBool
MaterialCreateGenericPipeline(void)
{
    RxPipeline *pipe;

    RWFUNCTION(RWSTRING("MaterialCreateGenericPipeline"));

    pipe = RxPipelineCreate();
    if (NULL != pipe)
    {
        RxLockedPipe *lpipe;

        pipe->pluginId = rwID_WORLDPLUGIN;
        lpipe = RxPipelineLock(pipe);
        if (NULL != lpipe)
        {
            lpipe = RxLockedPipeAddFragment(
                        lpipe,
                        (RwUInt32 *)NULL,
                        RxNodeDefinitionGetTransform(),
                        RxNodeDefinitionGetCullTriangle(),
                        /* Transform.csl now does the work of PreLight.csl */
                        /* RxNodeDefinitionGetPreLight(), */
                        RxNodeDefinitionGetLight(),
                        RxNodeDefinitionGetPostLight(),
                        RxNodeDefinitionGetClipTriangle(),
                        RxNodeDefinitionGetSubmitTriangle(),
                        (RxNodeDefinition *)NULL);
            RWASSERT (NULL != lpipe);

            pipe = RxLockedPipeUnlock(lpipe);

            RWASSERT(NULL != pipe);
            RWASSERT(pipe == lpipe);
            if (NULL != pipe)
            {
                RXPIPELINEGLOBAL(genericMaterialPipeline) = pipe;
                RXPIPELINEGLOBAL(currentMaterialPipeline) = pipe;

                RWRETURN(TRUE);
            }
        }
        /* Failed to lock/add/unlock the pipeline */
        _rxPipelineDestroy(pipe);
    }

    RWRETURN(FALSE);
}

/****************************************************************************
 * MaterialDestroyGenericPipeline
 */
static void
MaterialDestroyGenericPipeline(void)
{
    RWFUNCTION(RWSTRING("MaterialDestroyGenericPipeline"));

    RXPIPELINEGLOBAL(currentMaterialPipeline) = (RxPipeline *)NULL;
    if (NULL != RXPIPELINEGLOBAL(genericMaterialPipeline))
    {
        _rxPipelineDestroy(RXPIPELINEGLOBAL(genericMaterialPipeline));
        RXPIPELINEGLOBAL(genericMaterialPipeline) = (RxPipeline *)NULL;
    }

    RWRETURNVOID();
}

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))

/**
 * \ingroup rtgencpiperpatomic
 * \ref RpAtomicGetGenericPipeline returns the default
 * generic atomic object pipeline.
 *
 * This pipeline is constructed from generic nodes which will
 * run on all platforms. The default atomic object pipeline is
 * platform-specific (see \ref RpAtomicGetDefaultPipeline).
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * The world plugin must be attached before using this function.
 *
 * \return A pointer to the default generic atomic object pipeline
 * on success, otherwise NULL
 *
 * The default generic atomic object pipeline:
 * \verbatim
     AtomicInstance.csl
      v
     AtomicEnumerateLights.csl
      v
     MaterialScatter.csl
    \endverbatim
 * \see RpAtomicGetPipeline
 * \see RpAtomicSetPipeline
 * \see RpAtomicGetDefaultPipeline
 * \see \ref RpAtomicGetDefaultPipelineplatform
 * \see RpAtomicSetDefaultPipeline
 * \see RpMaterialGetPipeline
 * \see RpMaterialSetPipeline
 * \see RpMaterialSetDefaultPipeline
 * \see RpMaterialGetGenericPipeline
 * \see RpMaterialGetDefaultPipeline
 * \see \ref RpMaterialGetDefaultPipelineplatform
 * \see RpWorldGetSectorPipeline
 * \see RpWorldSetSectorPipeline
 * \see RpWorldSectorGetPipeline
 * \see RpWorldSectorSetPipeline
 * \see RpWorldGetGenericSectorPipeline
 * \see RpWorldGetDefaultSectorPipeline
 * \see \ref RpWorldGetDefaultSectorPipelineplatform
 * \see RpWorldSetDefaultSectorPipeline
 */
RxPipeline *
RpAtomicGetGenericPipeline(void)
{
    RWAPIFUNCTION(RWSTRING("RpAtomicGetGenericPipeline"));

    RWRETURN(RpAtomicGetGenericPipelineMacro());
}

#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/****************************************************************************
 AtomicCreateGenericPipeline
 */
static RwBool
AtomicCreateGenericPipeline(void)
{
    RxPipeline *pipe;

    RWFUNCTION(RWSTRING("AtomicCreateGenericPipeline"));

    pipe = RxPipelineCreate();
    if (NULL != pipe)
    {
        RxLockedPipe *lpipe;

        pipe->pluginId = rwID_WORLDPLUGIN;
        lpipe = RxPipelineLock(pipe);
        if (NULL != lpipe)
        {
            lpipe = RxLockedPipeAddFragment(
                        lpipe,
                        (RwUInt32 *)NULL,
                        RxNodeDefinitionGetAtomicInstance(),
                        RxNodeDefinitionGetAtomicEnumerateLights(),
                        RxNodeDefinitionGetMaterialScatter(),
                        (RxNodeDefinition *)NULL);
            RWASSERT (NULL != lpipe);

            pipe = RxLockedPipeUnlock(lpipe);

            RWASSERT(NULL != pipe);
            RWASSERT(pipe == lpipe);
            if (NULL != pipe)
            {
                RXPIPELINEGLOBAL(genericAtomicPipeline) = pipe;
                RXPIPELINEGLOBAL(currentAtomicPipeline) = pipe;

                RWRETURN(TRUE);
            }
        }
        /* Failed to lock/add/unlock the pipeline */
        _rxPipelineDestroy(pipe);
    }

    RWRETURN(FALSE);
}

/****************************************************************************
 AtomicDestroyGenericPipeline
 */
static void
AtomicDestroyGenericPipeline(void)
{
    RWFUNCTION(RWSTRING("AtomicDestroyGenericPipeline"));

    RXPIPELINEGLOBAL(currentAtomicPipeline) = (RxPipeline *)NULL;
    if (NULL != RXPIPELINEGLOBAL(genericAtomicPipeline))
    {
        _rxPipelineDestroy(RXPIPELINEGLOBAL(genericAtomicPipeline));
        RXPIPELINEGLOBAL(genericAtomicPipeline) = (RxPipeline *)NULL;
    }

    RWRETURNVOID();
}

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))

/**
 * \ingroup rtgencpiperpworldsub
 * \ref RpWorldGetGenericSectorPipeline returns the default
 * generic world-sector object pipeline.
 *
 * This pipeline is constructed from generic nodes which will run
 * on all platforms. The default world-sector object pipeline is
 * platform-specific (see \ref RpWorldGetDefaultSectorPipeline).
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * The world plugin must be attached before using this function.
 *
 * \return A pointer to the default generic world-sector object pipeline
 * on success, otherwise NULL
 *
 * The default generic world-sector object pipeline:
 * \verbatim

     WorldSectorInstance.csl
      v
     WorldSectorEnumerateLights.csl
      v
     MaterialScatter.csl
   \endverbatim
 * \see RpAtomicGetPipeline
 * \see RpAtomicSetPipeline
 * \see RpAtomicGetGenericPipeline
 * \see RpAtomicGetDefaultPipeline
 * \see \ref RpAtomicGetDefaultPipelineplatform
 * \see RpAtomicSetDefaultPipeline
 * \see RpMaterialGetPipeline
 * \see RpMaterialSetPipeline
 * \see RpMaterialSetDefaultPipeline
 * \see RpMaterialGetGenericPipeline
 * \see RpMaterialGetDefaultPipeline
 * \see \ref RpMaterialGetDefaultPipelineplatform
 * \see RpWorldGetSectorPipeline
 * \see RpWorldSetSectorPipeline
 * \see RpWorldSectorGetPipeline
 * \see RpWorldSectorSetPipeline
 * \see RpWorldGetDefaultSectorPipeline
 * \see \ref RpWorldGetDefaultSectorPipelineplatform
 * \see RpWorldSetDefaultSectorPipeline
 */
RxPipeline *
RpWorldGetGenericSectorPipeline(void)
{
    RWAPIFUNCTION(RWSTRING("RpWorldGetGenericSectorPipeline"));

    RWRETURN(RpWorldGetGenericSectorPipelineMacro());
}

#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/****************************************************************************
 WorldSectorCreateGenericPipeline
 */
static RwBool
WorldSectorCreateGenericPipeline(void)
{
    RxPipeline *pipe;

    RWFUNCTION(RWSTRING("WorldSectorCreateGenericPipeline"));

    pipe = RxPipelineCreate();
    if (NULL != pipe)
    {
        RxLockedPipe *lpipe;

        pipe->pluginId = rwID_WORLDPLUGIN;
        lpipe = RxPipelineLock(pipe);
        if (NULL != lpipe)
        {
            lpipe = RxLockedPipeAddFragment(
                        lpipe,
                        (RwUInt32 *)NULL,
                        RxNodeDefinitionGetWorldSectorInstance(),
                        RxNodeDefinitionGetWorldSectorEnumerateLights(),
                        RxNodeDefinitionGetMaterialScatter(),
                        (RxNodeDefinition *)NULL);
            RWASSERT (NULL != lpipe);

            pipe = RxLockedPipeUnlock(lpipe);

            RWASSERT(NULL != pipe);
            RWASSERT(pipe == lpipe);
            if (NULL != pipe)
            {
                RXPIPELINEGLOBAL(genericWorldSectorPipeline) = pipe;
                RXPIPELINEGLOBAL(currentWorldSectorPipeline) = pipe;

                RWRETURN(TRUE);
            }
        }
        /* Failed to lock/add/unlock the pipeline */
        _rxPipelineDestroy(pipe);
    }

    RWRETURN(FALSE);
}

/****************************************************************************
 * WorldSectorDestroyGenericPipeline
 */
static void
WorldSectorDestroyGenericPipeline(void)
{
    RWFUNCTION(RWSTRING("WorldSectorDestroyGenericPipeline"));

    RXPIPELINEGLOBAL(currentWorldSectorPipeline) = (RxPipeline *)NULL;
    if (NULL != RXPIPELINEGLOBAL(genericWorldSectorPipeline))
    {
        _rxPipelineDestroy(RXPIPELINEGLOBAL(genericWorldSectorPipeline));
        RXPIPELINEGLOBAL(genericWorldSectorPipeline) = (RxPipeline *)NULL;
    }

    RWRETURNVOID();
}

/**
 * \ingroup rtgencpiperpworldsub
 * \ref RtGenCPipeCreateGenericWorldPipelines
 *
 * Internal Use
 */
RwBool
RtGenCPipeCreateGenericWorldPipelines(void)
{
    RwBool result;

    RWAPIFUNCTION(RWSTRING("RtGenCPipeCreateGenericWorldPipelines"))

    result = MaterialCreateGenericPipeline();

    if (FALSE == result)
    {
        RWRETURN(result);
    }

    result = WorldSectorCreateGenericPipeline();

    if (FALSE == result)
    {
        MaterialDestroyGenericPipeline();
        RWRETURN(result);
    }

    result = AtomicCreateGenericPipeline();

    if (FALSE == result)
    {
        MaterialDestroyGenericPipeline();
        WorldSectorDestroyGenericPipeline();
        RWRETURN(result);
    }

    RWRETURN(result);
}

/**
 * \ingroup rtgencpiperpworldsub
 * \ref RtGenCPipeDestroyGenericWorldPipelines
 *
 * Internal Use
 */
void
RtGenCPipeDestroyGenericWorldPipelines(void)
{
    RWAPIFUNCTION(RWSTRING("RtGenCPipeDestroyGenericWorldPipelines"));

    MaterialDestroyGenericPipeline();
    WorldSectorDestroyGenericPipeline();
    AtomicDestroyGenericPipeline();

    RWRETURNVOID();
}
