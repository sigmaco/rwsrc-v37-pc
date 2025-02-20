/**
 * \ingroup rt2danim
 * \page rt2danimoverview Rt2dAnim Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rt2d.h, rt2danim.h,
 * \li \b Libraries: rwcore, rpworld, rt2d, rt2danim,
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach
 *
 * \subsection 2danimoverview Overview
 * \ref Rt2dAnim is a 2d animation library supporting several different levels
 * of animation on 2d objects.
 *
 * At a high level, \ref Rt2dMaestro allows the playback of user-interactive
 * animations incorporating buttons and hierarchical animations of 2d scenes.
 * A string label table lookup facility allows content to be accessed by
 * name. Sequencing is implemented via a message passing model. A custom
 * message handler may be chained before the default message handler. This
 * allows user code to be notified of events internal to the animation.
 * The user may also post external events into the animation via the message
 * passing interface.
 *
 * At the lowest level, \ref Rt2dAnim can be used to coordinate a simple
 * animation on a single level of 2d scene. Animations are constructed out
 * of a sequence of \ref Rt2dKeyFrameList objects on the scene. Individual
 * updates to the scene are scheduled through setting changes in
 * \ref Rt2dAnimObjectUpdate objects. Most animation operations are carried
 * out in conjunction with an \ref Rt2dAnimProps structure that enables the
 * keframe lists to be constructed against an existing scene.
 *
 * The APIs outside of those for \ref Rt2dMaestro itself are preliminary and
 * should be considered as subject to change.
 *
 * \see Rt2dMaestroCreate
 * \see Rt2dAnimCreate
 * \see Rt2dMessage
 * \see Rt2dStringLabel
 */


/**
 * \defgroup rt2danimobjectids Rt2dAnim Object IDs
 * \ingroup rt2danim
 *
 * Rt2dAnim list of fundamental object IDs used for streaming.
 *
 * \li rwID_2DANIM used by \ref Rt2dAnimStreamRead
 * \li rwID_2DKEYFRAME used by \ref Rt2dKeyFrameListStreamRead
 * \li rwID_2DMAESTRO used by \ref Rt2dMaestroStreamRead
 *
 * \see RwStreamFindChunk
 * \see RwStreamWriteChunkHeader
 */
/****************************************************************************
 Includes
 */

#include <rwcore.h>
#include <rpdbgerr.h>

#include "rt2danim.h"
#include "anim.h"



/****************************************************************************
 Local Types
 */

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */

/****************************************************************************
 Globals (across program)
 */
Rt2dAnimOnEndReachedCallBack _rt2dAnimOnEndReachedCallBack = NULL;

/****************************************************************************
 Local (static) Globals
 */

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimSetOnEndReachedCallBack sets a callback that is triggered on
 * the end of an animation being reached.
 *
 * \param callback  The new callback to be triggered
 *
 * \return Returns the callback that was set
 *
 * \see Rt2dAnimGetOnEndReachedCallBack
 * \see Rt2dAnimOnEndReachedCallBackLoop
 * \see Rt2dAnimOnEndReachedCallBackStop
 */
Rt2dAnimOnEndReachedCallBack
Rt2dAnimSetOnEndReachedCallBack(Rt2dAnimOnEndReachedCallBack callback)
{
    Rt2dAnimOnEndReachedCallBack previous;
    RWAPIFUNCTION(RWSTRING("Rt2dAnimSetOnEndReachedCallBack"))
    RWASSERT(callback);

    previous = _rt2dAnimOnEndReachedCallBack;

    _rt2dAnimOnEndReachedCallBack = callback;

    RWRETURN(previous);
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimGetOnEndReachedCallBack gets the callback that is
 * triggered on the end of an animation being reached.
 *
 * \return Returns the current callback
 *
 * \see Rt2dAnimSetOnEndReachedCallBack
 * \see Rt2dAnimOnEndReachedCallBackLoop
 * \see Rt2dAnimOnEndReachedCallBackStop
 */
Rt2dAnimOnEndReachedCallBack
Rt2dAnimGetOnEndReachedCallBack(void)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimGetOnEndReachedCallBack"))
    RWRETURN(_rt2dAnimOnEndReachedCallBack);
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimOnEndReachedCallBackLoop is a callback that will cause
 * an animation to be reset upon the end being reached. Can be used as
 * an onEndReached callback.
 *
 * \param anim  The animation ending
 * \param props  The props affected by the animation
 * \param remainingDeltaTime  The remaining time
 *
 * \return Returns the animation that was reset
 *
 * \see Rt2dAnimSetOnEndReachedCallBack
 * \see Rt2dAnimOnEndReachedCallBackStop
 */
Rt2dAnim *
Rt2dAnimOnEndReachedCallBackLoop(Rt2dAnim *anim, Rt2dAnimProps *props,
                          RwReal remainingDeltaTime)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimOnEndReachedCallBackLoop"));
    RWASSERT( anim );

    /* DUMMY - Only handles forward going case */
    if (remainingDeltaTime>0.0f)
    {
        Rt2dAnimReset(anim, props);

        /* No infinite loops thankyou */
        if (anim->last->time>0.0)
            Rt2dAnimAddDeltaTime(anim, props, remainingDeltaTime);
    }

    RWRETURN( anim );
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimOnEndReachedCallBackStop is a callback that will cause an
 * animation to be stopped upon the end being reached. Can be used as an
 * onEndReached callback.
 *
 * \param anim  The animation ending
 * \param props  The props affected by the animation
 * \param remainingDeltaTime  The remaining time
 *
 * \return Returns the animation that was stopped
 *
 * \see Rt2dAnimSetOnEndReachedCallBack
 * \see Rt2dAnimOnEndReachedCallBackLoop
 */
Rt2dAnim *
Rt2dAnimOnEndReachedCallBackStop(Rt2dAnim *anim,
                        Rt2dAnimProps *props __RWUNUSED__,
                        RwReal remainingDeltaTime __RWUNUSED__)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimOnEndReachedCallBackStop"));
    RWASSERT( anim );

    RWRETURN( anim );
}


