
/*****************************************************************************
 *
 * File :     rpdmorph.c
 *
 * Abstract : DeltaMorph plugin for Renderware.
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
 * \ingroup rpdmorph
 * \page rpdmorphoverview RpDMorph Plugin Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rpdmorph.h
 * \li \b Libraries: rwcore, rpworld, rpdmorph
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpDMorphPluginAttach
 *
 * \subsection dmorphoverview Overview
 * This plugin provides facilities for delta morphing.
 * Delta morphing is a process where a base geometry
 * has a number of deltas applied to it. These deltas
 * are called delta morph targets. The targets can overlap
 * and morph any combination of the base geometry's
 * vertex components: positions, normals, pre light
 * colors and texture coordinates.
 *
 * The delta morph targets are stored as a plugin extension
 * of the base geometry data, and are compressed by excluding
 * sequences of vertices where the delta is zero.
 *
 * Although it is possible to directly manipulate the
 * amounts of each morph target applied to the base
 * geometry, a standard animation system is provided.
 *
 * Before any of the plugin functions are used, the plugin
 * should be attached using \ref RpDMorphPluginAttach.
 *
 * \par Basic Usage
 *
 * A delta morph atomic and an associated set of animations
 * can be created within the exporter. In an application, the
 * animation may be loaded and set on an atomic with
 *
 * \li \ref RpDMorphAnimationRead
 * \li \ref RpDMorphAtomicSetAnimation
 *
 * The animation of the atomic may then be advanced in time
 * using
 *
 * \li \ref RpDMorphAtomicAddTime
 *
 * The morphing takes place during rendering. Delta morph
 * geometry data can be detected by using
 *
 * \li \ref RpDMorphGeometryGetNumDMorphTargets
 *
 * which returns the number of delta morph targets or zero
 * if there none exist.
 *
 * \par Delta Morph Geometry
 *
 * These functions are available:
 *
 * \li \ref RpDMorphGeometryCreateDMorphTargets
 *          defines the geometry as a base for delta
 *          morphing. Prepares the geometry to accept
 *          morph targets.
 *
 * \li \ref RpDMorphGeometryDestroyDMorphTargets
 *          removes all the delta morph targets and returns
 *          the geometry to it's original state before
 *          \ref RpDMorphGeometryCreateDMorphTargets.
 *
 * \li \ref RpDMorphGeometryRemoveDMorphTarget
 *          removed a single delta morph targets from the
 *          base geometry.
 *
 * \li \ref RpDMorphGeometryAddDMorphTarget
 *          adds a new morph target to the base geometry.
 *
 * \li \ref RpDMorphGeometryGetDMorphTarget
 *          returns a pointer to the requested delta morph
 *          target.
 *
 * \li \ref RpDMorphGeometryGetNumDMorphTargets
 *          reports the number of delta morph targets
 *          created by
 *          \ref RpDMorphGeometryCreateDMorphTargets
 *
 * \par Delta Morph Targets
 *
 * These functions are available:
 *
 * \li \ref RpDMorphTargetSetName
 *          set the name of the delta morph target.
 *
 * \li \ref RpDMorphTargetGetName
 *          returns the name of the delta morph target.
 *
 * \li \ref RpDMorphTargetGetBoundingSphere
 *          returns a bounding sphere of the base geometry
 *          with the delta morph applied with value 1.
 *
 * \li \ref RpDMorphTargetGetFlags
 *          returns the \ref RpGeometryFlag flags which
 *          were set during when the delta morph target
 *          was added to the base with
 *          \ref RpDMorphGeometryAddDMorphTarget and
 *          defines what vertex data is stored in the
 *          delta morph target.
 *
 * \par Delta Morph Animations
 *
 * An animation consists of control sequences for each delta morph target
 * of an atomic. Each sequence consists of a chain of animation frames
 * which correspond to units of linear interpolation, having a start value,
 * an end value, and a duration.
 *
 * Animations and frame sequences may be created and destroyed with:
 *
 * \li \ref RpDMorphAnimationCreate
 * \li \ref RpDMorphAnimationDestroy
 * \li \ref RpDMorphAnimationCreateFrames
 * \li \ref RpDMorphAnimationDestroyFrames
 * \li \ref RpDMorphAnimationGetNumDMorphTargets
 * \li \ref RpDMorphAnimationGetNumFrames
 *
 * The frame sequence data may be edited and queried with:
 *
 * \li \ref RpDMorphAnimationFrameSet
 * \li \ref RpDMorphAnimationFrameSetNext
 * \li \ref RpDMorphAnimationFrameGetNext
 * \li \ref RpDMorphAnimationFrameSetStartValue
 * \li \ref RpDMorphAnimationFrameGetStartValue
 * \li \ref RpDMorphAnimationFrameSetEndValue
 * \li \ref RpDMorphAnimationFrameGetEndValue
 * \li \ref RpDMorphAnimationFrameSetDuration
 * \li \ref RpDMorphAnimationFrameGetDuration
 *
 * Animations may be written to and read from a binary stream using:
 *
 * \li \ref RpDMorphAnimationStreamRead
 * \li \ref RpDMorphAnimationStreamWrite
 * \li \ref RpDMorphAnimationStreamGetSize
 * \li \ref RpDMorphAnimationRead
 * \li \ref RpDMorphAnimationWrite
 *
 * \par Running Animations
 *
 * An animation may be set on a delta morph target and controlled
 * using:
 *
 * \li \ref RpDMorphAtomicSetAnimation
 * \li \ref RpDMorphAtomicGetAnimation
 * \li \ref RpDMorphAtomicAddTime
 * \li \ref RpDMorphAtomicGetAnimTime
 *
 * A loop callback may be defined to control what happens when an
 * animation reaches the end and loops back to the start:
 *
 * \li \ref RpDMorphAtomicSetAnimLoopCallBack
 * \li \ref RpDMorphAtomicGetAnimLoopCallBack
 *
 * The current positions and times within the frame sequences for the
 * delta morph targets may be queried or changed using the
 * following. These could be used for recording the current position
 * within an animation in order to jump directly back at a later time.
 *
 * \li \ref RpDMorphAtomicGetAnimFrame
 * \li \ref RpDMorphAtomicSetAnimFrame
 * \li \ref RpDMorphAtomicGetAnimFrameTime
 * \li \ref RpDMorphAtomicSetAnimFrameTime
 *
 * \par Direct Control of Morphing
 *
 * An alternative to using the animation system is to directly modify
 * the morph values. This can be done using:
 *
 * \li \ref RpDMorphAtomicGetDMorphValues
 *
 * This could be used for overriding a particular morph target on top
 * of a standard animation, for performing a full procedural animation,
 * or for implementing an alternative animation scheme to the standard
 * one.
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

#include "dmphgeom.h"
#include "dmphintp.h"
#include "dmorph.h"


/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Public Global Variables -----------------------------------------------*
 *===========================================================================*/
#if (defined(RWDEBUG))
long rpDMorphStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

RwModuleInfo rpDMorphModule = {0, 0};

/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- External functions ----------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*--- DMorphOpen -----------------------------------------------------------
 */
static void *
DMorphOpen( void *instance,
            RwInt32 offset __RWUNUSED__,
            RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("DMorphOpen"));
    RWASSERT(instance);

#if defined(PLATFORM_PIPELINES)
    if (0 == rpDMorphModule.numInstances)
    {
        RwBool  success;

        /* Set up the pipeline. */
        success = _rpDMorphPipelinesCreate(rpDMORPHTYPEGENERIC);
        RWASSERT(FALSE != success);
    }
#endif /* defined(PLATFORM_PIPELINES) */

    /* one more module instance */
    rpDMorphModule.numInstances++;

    RWRETURN(instance);
}

/*--- DMorphClose ----------------------------------------------------------
 */
static void *
DMorphClose( void *instance,
             RwInt32 offset __RWUNUSED__,
             RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("DMorphClose"));
    RWASSERT(instance);

    /* one less module instance */
    rpDMorphModule.numInstances--;

#if defined(PLATFORM_PIPELINES)
    if(0 == rpDMorphModule.numInstances)
    {
        RwBool success;

        /* Destroy the skinning pipelines. */
        success = _rpDMorphPipelinesDestroy();
        RWASSERT(FALSE != success);
    }
#endif /* defined(PLATFORM_PIPELINES) */

    RWRETURN(instance);
}


/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rpdmorph
 * \ref RpDMorphPluginAttach is called by the application
 * to indicate that the delta morph plugin should be used.
 * The call to this function should be placed between
 * \ref RwEngineInit and \ref RwEngineOpen and the world
 * plugin must already be attached.
 *
 * The library rpdmorph and the header file rpdmorph.h are
 * required.
 *
 * \return True on success, false otherwise
 *
 * \see RpDMorphGeometryCreateDMorphTargets
 * \see RpDMorphAnimationCreate
 * \see RpDMorphAnimationRead
 * \see RpDMorphAtomicSetAnimation
 * \see RpDMorphAtomicAddTime
 * \see RpWorldPluginAttach
 */
RwBool
RpDMorphPluginAttach( void )
{
    RwInt32 offset;

    RWAPIFUNCTION(RWSTRING("RpDMorphPluginAttach"));
    RWASSERT(rpDMorphModule.numInstances == 0);

    /* Register the plugin with the engine. */
    offset = RwEngineRegisterPlugin(
                    0, rwID_DMORPHPLUGIN, DMorphOpen, DMorphClose );

    if( offset < 0 )
    {
        RWRETURN(FALSE);
    }

    /* Register the geometry plugin data */
    if (!_rpDMorphGeometryPluginAttach())
    {
        RWRETURN(FALSE);
    }

    /* Register the atomic plugin data */
    if (!_rpDMorphAtomicPluginAttach())
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}


