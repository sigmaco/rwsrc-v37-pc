/**
 * \ingroup rt2dmaestro
 * \page rt2dmaestrooverview Rt2dMaestro Overview
 *
 * \ref Rt2dMaestro is a high level coordinator of the playback of
 * user-interactive animations incorporating buttons and hierarchical
 * animations of 2d scenes.
 *
 * It makes use of several lower level features within the Rt2dAnim toolkit
 * such as \ref Rt2dAnim and \ref Rt2dKeyFrameList.
 *
 * Many of the other components of the \ref Rt2dAnim toolkit are intended
 * as subcomponents of \ref Rt2dMaestro. These include \ref Rt2dMessage,
 * \ref Rt2dStringLabel, \ref Rt2dCel and \ref Rt2dButton. Out of these subcomponents
 * \ref Rt2dMessage and \ref Rt2dStringLabel are the most useful for leveraging
 * \ref Rt2dMaestro with external code.
 *
 * An \ref Rt2dMaestro is usually created with an import tool of some description.
 * It is then read in with the usual RenderWare streaming functions and
 * the \ref Rt2dMaestroStreamRead function.
 *
 * A string label table lookup facility allows content to be accessed by
 * name. These are carried out via the \ref Rt2dStringLabel group of functions.
 * After streaming in an \ref Rt2dMaestro, strings of interest should be
 * searched for within the maestro's string label table. Once the string label
 * has been found, it may be referred to by index within the \ref Rt2dMaestro
 * it was found in.
 *
 * Sequencing is implemented via a message passing model. The different
 * messages are mapped within the \ref Rt2dMessage structure. A custom
 * message handler may be chained before the default message handler. This
 * allows user code to be notified of events internal to the animation.
 * The user may also post external events into the animation via the message
 * passing interface.
 *
 * User handles may also be registered within the string label table. In
 * combination with the ability to hook a custom message handler, user events
 * may be triggered by events internal to the director.
 *
 * An example could be a callback to be used when a particular animation is
 * 'stopped' by a button action. The callback could be registered within the
 * label table against the name of that animation. A custom message handler
 * would then watch for the rt2dMESSAGETYPESTOP message. If the message target
 * matches the desired animation, the stored callback could then be activated.
 *
 * \see Rt2dMaestroCreate
 * \see Rt2dMessage
 * \see Rt2dStringLabel
 */


/*
 *
 * Functionality for 2D rendering
 *
 * Copyright (c) 2001 Criterion Software Ltd.
 */

/****************************************************************************
 *                                                                          *
 *  Module  :   maestro.c                                                  *
 *                                                                          *
 *  Purpose :   simple 2d animation maestro functionality                  *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
 */


#include <rwcore.h>
#include <rpdbgerr.h>
#include <rpcriter.h>

#include "rt2danim.h"
#include "anim.h"
#include "props.h"
#include "maestro.h"
#include "bucket.h"
#include "button.h"
#include "strlabel.h"
#include "message.h"
#include "handler.h"
#include "cel.h"
#include "gstate.h"
#include "keyframe.h"

/*-----------------20/08/2001 10:20 AJH -------------
 * Is that really needed ???
 * --------------------------------------------------*/
#if defined (__MWERKS__)
#if (defined(RWVERBOSE))
#pragma message (__FILE__ "/" _SKY_EXPAND(__LINE__) ": __MWERKS__ == " _SKY_EXPAND(__MWERKS__))
#endif /* (defined (__MWERKS__)) */
#if (__option (global_optimizer))
#pragma always_inline off
#endif /* (__option (global_optimizer)) */
#endif /*  defined (__MWERKS__) */

/****************************************************************************
 Local Types
 */

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */

#define RWZEROTOL 0.0001f
#define GJB_DEBUGx

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */

/****************************************************************************
 Functions.
 */


/****************************************************************************
 *
 * Maestro's initialisation and destruction functions.
 *
 *
 ****************************************************************************/

Rt2dMaestro *
_rt2dMaestroInit(Rt2dMaestro *maestro)
{
    RWFUNCTION(RWSTRING("_rt2dMaestroInit"));

    RWASSERT(maestro);

    maestro->flag = 0;
    maestro->version = 0;
    maestro->animations = NULL;
    maestro->labels = NULL;
    maestro->byteBucket = NULL;
    maestro->scene = NULL;
    maestro->buttons = NULL;
    maestro->currAnimIndex = -1;

    /* Create the SLists. */
    _rt2dMaestroCreateAnimations(maestro);
    _rt2dMaestroCreateStringLabels(maestro);
    _rt2dMaestroCreateButtons(maestro);
    _rt2dMaestroCreateMessageList(maestro);
    _rt2dMaestroCreateMessageQueue(maestro);

    maestro->byteBucket = _rt2dByteBucketCreate();

    maestro->messageHandler = Rt2dMessageHandlerDefaultCallBack;


    maestro->prevMouse.position.x = (RwReal)0xFFFFFFFF;
    maestro->prevMouse.position.y = (RwReal)0xFFFFFFFF;
    maestro->prevMouse.button = 0;

    maestro->bbox.x = 0.0f;
    maestro->bbox.y = 0.0f;
    maestro->bbox.w = 0.0f;
    maestro->bbox.h = 0.0f;


    _rt2dStateTransTableInit(maestro->stateTransTable );

    RWRETURN(maestro);
}

RwBool
_rt2dMaestroDestruct(Rt2dMaestro *maestro)
{
    RwBool              result;

    RWFUNCTION(RWSTRING("_rt2dMaestroDestruct"));

    RWASSERT(maestro);

    result = TRUE;

    if (maestro->animations)
    {
        _rt2dMaestroDestroyAnimations(maestro);
    }

    if (maestro->labels)
    {
        _rt2dMaestroDestroyStringLabels(maestro);
    }

    /* Check and destroy the buttons. */
    if (maestro->buttons)
    {
        _rt2dMaestroDestroyButtons(maestro);
    }

    /* Check and destroy the messages */
    if (maestro->messageList)
    {
        _rt2dMaestroDestroyMessageList(maestro);
    }

    /* Check and destroy the scene objects. */
    if (maestro->scene)
    {
        Rt2dSceneDestroy(maestro->scene);
    }


    /* Check and destroy the message queue */
    if (maestro->messageQueue)
    {
        _rt2dMaestroDestroyMessageQueue(maestro);
    }

    if (maestro->byteBucket)
    {
        _rt2dByteBucketDestroy(maestro->byteBucket);

        maestro->byteBucket = NULL;
    }

    RWRETURN(result);
}

/****************************************************************************/

/**
 * \ingroup rt2dmaestro
 * \ref Rt2dMaestroCreate is used to create and initialize a new maestro.
 *
 * \return Pointer to a maestro.
 *
 * \see Rt2dMaestroDestroy
 */
Rt2dMaestro *
Rt2dMaestroCreate(void)
{
    Rt2dMaestro *maestro;

    RWAPIFUNCTION(RWSTRING("Rt2dMaestroCreate"));

    maestro = (Rt2dMaestro *)RwFreeListAlloc(&Rt2dAnimGlobals.maestroFreeList,
                                             rwID_2DMAESTRO | rwMEMHINTDUR_EVENT);

    if (maestro)
    {
        /* Initialise members */
        _rt2dMaestroInit(maestro);
    }

    RWRETURN(maestro);
}

/**
 * \ingroup rt2dmaestro
 * \ref Rt2dMaestroDestroy destroys a maestro.
 *
 * \param maestro Pointer to a maestro to destroy. Any data owned by the maestro is
 * also destroyed.
 *
 * \return Returns TRUE if successful, FALSE otherwise.
 *
 * \see Rt2dMaestroCreate
 */
RwBool
Rt2dMaestroDestroy(Rt2dMaestro *maestro)
{
    RWAPIFUNCTION(RWSTRING("Rt2dMaestroDestroy"));

    RWASSERT(maestro);

    /* Destroy members */
    _rt2dMaestroDestruct(maestro);

    /* Free maestro itself */
    RwFreeListFree(&Rt2dAnimGlobals.maestroFreeList, maestro);

    RWRETURN(TRUE);
}

/****************************************************************************
 *
 * Maestro's stream functions.
 *
 *
 ****************************************************************************/

/**
 * \ingroup rt2dmaestro
 * \ref Rt2dMaestroStreamGetSize is used to determine the size in bytes of
 * the binary representation of a maestro. This value is used in the
 * binary chunk header to indicate the size of the chunk. The size
 * includes the size of the chunk header.
 *
 * \param maestro Pointer to a maestro
 *
 * \return Returns a \ref RwUInt32 value equal to the chunk size (in bytes)
 * of the maestro.
 *
 * \see Rt2dMaestroStreamRead
 * \see Rt2dMaestroStreamWrite
 */
RwUInt32
Rt2dMaestroStreamGetSize(Rt2dMaestro *maestro)
{
    RwInt32                 size;

    RWAPIFUNCTION(RWSTRING("Rt2dMaestroStreamGetSize"));

    RWASSERT(maestro);

    /* Chunk header size */
    size = rwCHUNKHEADERSIZE;

    /* Int32 for version stamp. */
    size += sizeof(RwInt32);

    /* Size of bounding box. */
    size += sizeof(Rt2dBBox);

    /* Size of the animations. */
    size += _rt2dMaestroAnimationsStreamGetSize(maestro);

    /* Size of the labels. */
    size += _rt2dMaestroStringLabelsStreamGetSize(maestro);

    /* Size of the scene objects. */
    size += _rt2dMaestroSceneStreamGetSize(maestro);

    /* Size of the buttons. */
    size += _rt2dMaestroButtonsStreamGetSize(maestro);

    /* Size of the messages. */
    size += _rt2dMaestroMessageListStreamGetSize(maestro);
    size += _rt2dMaestroMessageStoreStreamGetSize(maestro);

    RWRETURN(size);
}

/**
 * \ingroup rt2dmaestro
 * \ref Rt2dMaestroStreamWrite is used to write the specified maestro
 * to the given binary stream. Note that the stream will have been
 * opened prior to this function call.
 *
 * \param maestro Pointer to the maestro to stream.
 * \param stream   Pointer to the stream to write to.
 *
 * \return Returns a pointer to the maestro if successful, NULL otherwise.
 *
 * \see Rt2dMaestroStreamRead
 * \see Rt2dMaestroStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 */
Rt2dMaestro *
Rt2dMaestroStreamWrite(Rt2dMaestro *maestro, RwStream *stream)
{
    RwStream             *result;
    RwUInt32              size;

    RWAPIFUNCTION(RWSTRING("Rt2dMaestroStreamWrite"));

    RWASSERT(maestro);
    RWASSERT(stream);

    result = NULL;

    size = Rt2dMaestroStreamGetSize(maestro);
    if (RwStreamWriteChunkHeader(stream, rwID_2DMAESTRO, size) == NULL)
        RWRETURN(NULL);

    /* Version stamp the file. */
    if (RwStreamWriteInt32(stream, &maestro->version, sizeof(RwInt32)) == NULL)
        RWRETURN(NULL);

    /* Write out the bounding box. */
    if (RwStreamWriteReal(stream, (RwReal *) &maestro->bbox, sizeof(Rt2dBBox)) == NULL)
        RWRETURN(NULL);

    /* Write out the labels. */
    if (_rt2dMaestroStringLabelsStreamWrite(maestro, stream) == NULL)
        RWRETURN(NULL);

    /* Write out the scene. */
    if (_rt2dMaestroSceneStreamWrite(maestro, stream) == NULL)
        RWRETURN(NULL);

    /* Write out the buttons. */
    if (_rt2dMaestroButtonsStreamWrite(maestro, stream) == NULL)
        RWRETURN(NULL);

    /* Write out the messages. */
    if (_rt2dMaestroMessageListStreamWrite(maestro, stream) == NULL)
        RWRETURN(NULL);
    if (_rt2dMaestroMessageStoreStreamWrite(maestro, stream) == NULL)
        RWRETURN(NULL);

    /* Write out the animations. */
    if (_rt2dMaestroAnimationsStreamWrite(maestro, stream) == NULL)
        RWRETURN(NULL);

    RWRETURN(maestro);
}

static Rt2dMaestro *
_rt2dMaestroAnimNodeResetCallBack(Rt2dMaestro *maestro,
                                      Rt2dAnimNode *animNode,
                                      void *pData __RWUNUSED__)
{
    Rt2dObject *scene;
    RwInt32 i;
    RwInt32 numKeyf;

    RWFUNCTION(RWSTRING("_rt2dMaestroAnimNodeResetCallBack"));

    RWASSERT(maestro);
    RWASSERT(animNode);

    if( (-1 == animNode->parent) && (-1 == animNode->posInParentScene))
    {
        scene = maestro->scene;
    }
    else
    {
        scene = Rt2dSceneGetChildByIndex(
                Rt2dMaestroGetAnimSceneByIndex(maestro, animNode->parent),
                animNode->posInParentScene);

        /* We depend upon the main animation to show / hide sub-animations as
           necessary */
        Rt2dObjectSetVisible(scene, FALSE);
    }

    numKeyf = Rt2dAnimGetNumberOfKeyFrames(&animNode->anim);

    for(i=0;i<numKeyf;i++)
    {
        Rt2dCel *cel;
        Rt2dButton *button;
        RwInt32 j, numButton;
        RwInt32 *buttonIndex;

        cel = _rt2dCelListGetCelByIndex(animNode->celList, i);

        if( cel )
        {
            numButton = _rwSListGetNumEntries(cel->buttonIndices);
            buttonIndex = (RwInt32 *)_rwSListGetArray(cel->buttonIndices);

            for (j = 0; j < numButton; j++)
            {
                button = _rt2dMaestroGetButtonByIndex(maestro, *buttonIndex);
                _rt2dButtonReset(button, scene);
                buttonIndex++;
            }
        }
    }

    Rt2dAnimReset(&animNode->anim, &animNode->props);
    Rt2dAnimAddDeltaFrameIndex(&animNode->anim, &animNode->props, 1);

    RWRETURN(maestro);
}

/**
 * \ingroup rt2dmaestro
 * \ref Rt2dMaestroStreamRead reads a maestro from a binary stream. If the
 * maestro argument is NULL then a new maestro is created, otherwise the
 * specified one is over-written. Note that prior to this function call a
 * binary maestro chunk must be found in the stream using the
 * \ref RwStreamFindChunk API function.
 *
 * The sequence to locate and read a maestro from a binary stream is
 * as follows:
 * \code
   RwStream *stream;
   Rt2dMaestro *newMaestro;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_2DMAESTRO, NULL, NULL) )
       {
           newMaestro = Rt2dMaestroStreamRead((Rt2dMaestro *)NULL, stream);
       }

       RwStreamClose(stream, NULL);
   }
   \endcode
 *
 * \param maestro Pointer to a maestro which will receive the data from the
 *                 stream. If one is not provided by the application (NULL is
 *                 passed), then one will be created.
 * \param stream   Pointer to the stream the maestro will be read from
 *
 * \return Returns a pointer to the maestro if successful, NULL otherwise.
 *
 * \see Rt2dMaestroStreamWrite
 * \see Rt2dMaestroStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RwStreamFindChunk
 *
 */
Rt2dMaestro *
Rt2dMaestroStreamRead(Rt2dMaestro *maestro, RwStream *stream)
{
    RwInt32                 version;

    RWAPIFUNCTION(RWSTRING("Rt2dMaestroStreamRead"));

    RWASSERT(stream);

#if defined( RWDEBUG )
    if (!_rt2dAnimOpenCalled)
    {
        RWMESSAGE((RWSTRING("Call Rt2dAnimOpen first")));
    }
    RWASSERT(_rt2dAnimOpenCalled);
#endif

    /* Check if we need to create a maestro. */
    if (maestro == NULL)
        maestro = Rt2dMaestroCreate();

    if (maestro)
    {
        /* Read the version stamp. */
        if (RwStreamReadInt32(stream, &version, sizeof(RwInt32)) == NULL)
            RWRETURN(NULL);

        /* Read the bounding box. */
        if (RwStreamReadReal(stream, (RwReal *) &maestro->bbox, sizeof(Rt2dBBox)) == NULL)
            RWRETURN(NULL);

        /* Read the labels. */
        if (_rt2dMaestroStringLabelsStreamRead(maestro, stream) == NULL)
            RWRETURN(NULL);

        /* Read the scene. */
        if (_rt2dMaestroSceneStreamRead(maestro, stream) == NULL)
            RWRETURN(NULL);

        /* Read the buttons. */
        if (_rt2dMaestroButtonsStreamRead(maestro, stream) == NULL)
            RWRETURN(NULL);

        /* Read the messages. */
        if (_rt2dMaestroMessageListStreamRead(maestro, stream) == NULL)
            RWRETURN(NULL);
        if (_rt2dMaestroMessageStoreStreamRead(maestro, stream) == NULL)
            RWRETURN(NULL);

       /* Read the animations. */
        if (_rt2dMaestroAnimationsStreamRead(maestro, stream) == NULL)
            RWRETURN(NULL);

        /*AJH: DUMMY
         * It looks like a hack, it smells like a hack, it fills like a hack
         * it's because IT'S A HACK
         */
        _rt2dMaestroForAllAnimNode(maestro,
            _rt2dMaestroAnimNodeResetCallBack, NULL);

    }

    RWRETURN(maestro);
}

/****************************************************************************
 *
 * Maestro's animation functions.
 *
 *
 ****************************************************************************/

Rt2dAnimNode *
_rt2dAnimNodeInit(Rt2dAnimNode * animNode)
{
    RWFUNCTION(RWSTRING("_rt2dAnimNodeInit"));

    RWASSERT(animNode);

    animNode->flag = RT2D_ANIMNODE_FLAG_DO_CEL_ACTIONS;

    animNode->parent = -1;
    animNode->posInParentScene = -1;

    if ((animNode->celList = Rt2dCelListCreate()) == NULL)
        RWRETURN(NULL);

    RWRETURN(animNode);
}

Rt2dAnimNode *
_rt2dAnimNodeDestruct(Rt2dAnimNode * animNode)
{
    RWFUNCTION(RWSTRING("_rt2dAnimNodeDestruct"));

    RWASSERT(animNode);

    animNode->flag = 0;

    animNode->parent = -1;
    animNode->posInParentScene = -1;

    if (animNode->celList)
        Rt2dCelListDestroy(animNode->celList);

    _rt2dAnimPropsDestruct(&animNode->props);

    animNode->celList = NULL;

    RWRETURN(animNode);
}

static Rt2dMaestro *
_rt2dMaestroAnimNodeDestructCallBack(Rt2dMaestro *maestro,
                                      Rt2dAnimNode *animNode,
                                      void *pData __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("_rt2dMaestroAnimNodeDestructCallBack"));

    RWASSERT(maestro);
    RWASSERT(animNode);

    _rt2dAnimDestruct(&animNode->anim, &animNode->props);
    _rt2dAnimNodeDestruct(animNode);

    RWRETURN(maestro);
}

static Rt2dMaestro *
_rt2dMaestroAnimNodeStreamGetSizeCallBack(Rt2dMaestro *maestro,
                                           Rt2dAnimNode *animNode,
                                           void *pData)
{
    RwInt32             *size;

    RWFUNCTION(RWSTRING("_rt2dMaestroAnimNodeStreamGetSizeCallBack"));

    RWASSERT(maestro);
    RWASSERT(animNode);

    size = (RwInt32 *) pData;

    *size += _rt2dCelListStreamGetSize(maestro, animNode->celList);
    *size += Rt2dAnimStreamGetSize(&animNode->anim, &animNode->props);

    RWRETURN(maestro);
}

static Rt2dMaestro *
_rt2dMaestroAnimNodeStreamWriteCallBack(Rt2dMaestro *maestro,
                                         Rt2dAnimNode *animNode,
                                         void *pData)
{
    RwStream            *stream;

    RWFUNCTION(RWSTRING("_rt2dMaestroAnimNodeStreamWriteCallBack"));

    RWASSERT(maestro);
    RWASSERT(animNode);

    stream = (RwStream *) pData;

    if (_rt2dCelListStreamWrite(maestro, animNode->celList, stream) == NULL)
        RWRETURN(NULL);

    if (RwStreamWriteInt32(stream, &animNode->parent, sizeof(RwInt32)) == NULL)
        RWRETURN(NULL);

    if (RwStreamWriteInt32(stream, &animNode->posInParentScene, sizeof(RwInt32)) == NULL)
        RWRETURN(NULL);

    if (Rt2dAnimStreamWrite(&animNode->anim, stream, &animNode->props) == NULL)
        RWRETURN(NULL);

    RWRETURN(maestro);
}

static Rt2dMaestro *
_rt2dMaestroAnimNodeUpdateCallBack(Rt2dMaestro *maestro,
                                    Rt2dAnimNode *node,
                                    void *pData __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("_rt2dMaestroAnimNodeUpdateCallBack"));

    RWASSERT(maestro);
    RWASSERT(node);

    if (NULL == Rt2dAnimTimeUpdate(&node->anim, &node->props))
    {
        RWRETURN(NULL);
    }

    RWRETURN(maestro);
}

Rt2dMaestro *
_rt2dMaestroCreateAnimations(Rt2dMaestro *maestro)
{
    RWFUNCTION(RWSTRING("_rt2dMaestroCreateAnimations"));

    RWASSERT(maestro);
    RWASSERT((maestro->animations == NULL));

    if ((maestro->animations = _rwSListCreate(sizeof(Rt2dAnimNode),
            rwID_2DANIM | rwMEMHINTDUR_EVENT))
        == NULL)
        RWRETURN(NULL);

    RWRETURN(maestro);
}

Rt2dMaestro *
_rt2dMaestroDestroyAnimations(Rt2dMaestro *maestro)
{
    RWFUNCTION(RWSTRING("_rt2dMaestroDestroyAnimations"));

    RWASSERT(maestro);

    /* Check and destroy the animations. */
    if (maestro->animations)
    {
        _rt2dMaestroForAllAnimNode(maestro,
            _rt2dMaestroAnimNodeDestructCallBack, NULL);

        _rwSListDestroy(maestro->animations);

        maestro->animations = NULL;
    }

    RWRETURN(maestro);
}

RwInt32
_rt2dMaestroAnimationsStreamGetSize(Rt2dMaestro *maestro)
{
    RwInt32             size;

    RWFUNCTION(RWSTRING("_rt2dMaestroAnimationsStreamGetSize"));

    RWASSERT(maestro);

    size = 0;

    /* An int for number of animations and props. */
    size += sizeof(RwInt32);

    /* Size for the animation and props array. */
    if (maestro->animations)
    {
        if ((_rt2dMaestroForAllAnimNode(maestro,
            _rt2dMaestroAnimNodeStreamGetSizeCallBack, (void *) &size)) == NULL)
            RWRETURN(-1);
    }

    RWRETURN(size);
}

Rt2dMaestro *
_rt2dMaestroAnimationsStreamRead(Rt2dMaestro *maestro, RwStream *stream)
{
    RwChunkHeaderInfo       chunkHeader;

    RwInt32                 i, num;
    Rt2dAnimNode            *animNode;
    Rt2dObject              *scene;

    RWFUNCTION(RWSTRING("_rt2dMaestroAnimationsStreamRead"));

    RWASSERT(maestro);
    RWASSERT(stream);

    /* Read in the num of buttons. */
    if (RwStreamReadInt32(stream, &num, sizeof(RwInt32)) == NULL)
    {
        RWRETURN((Rt2dMaestro *)NULL);
    }

    if (num > 0)
    {
        if (maestro->animations == NULL)
        {
            _rt2dMaestroCreateAnimations(maestro);
        }

        if (maestro->animations)
        {
            /* Initialise the number of entries in the animation and props array. */
            if ((animNode = (Rt2dAnimNode *)
                    _rwSListGetNewEntries(maestro->animations, num,
                        rwID_2DANIM | rwMEMHINTDUR_EVENT)) == NULL)
            {
                RWRETURN((Rt2dMaestro *)NULL);
            }

            for (i = 0; i < num; i++)
            {
                if (_rt2dAnimNodeInit(animNode) == NULL)
                {
                    RWRETURN((Rt2dMaestro *)NULL);
                }

                /* Read in the cel list. */
                if (_rt2dCelListStreamRead(maestro, animNode->celList, stream) == NULL)
                {
                    RWRETURN((Rt2dMaestro *)NULL);
                }

                /* Read in the parent index and pos in parent scene index. */
                if (RwStreamReadInt32(stream, &animNode->parent, sizeof(RwInt32)) == NULL)
                {
                    RWRETURN((Rt2dMaestro *)NULL);
                }

                if (RwStreamReadInt32(stream,
                    &animNode->posInParentScene, sizeof(RwInt32)) == NULL)
                {
                    RWRETURN((Rt2dMaestro *)NULL);
                }

                /* Read in the animations chunk header. */

                if (RwStreamReadChunkHeaderInfo(stream, &chunkHeader) == NULL)
                {
                    RWRETURN((Rt2dMaestro *)NULL);
                }

                /* Create the props. */

                scene = Rt2dMaestroGetAnimSceneByIndex(maestro, i);

                if (_rt2dAnimPropsInit(&animNode->props, scene) == NULL)
                {
                    RWRETURN((Rt2dMaestro *)NULL);
                }

                /* Read in the animations. */

                if (_rt2dAnimStreamRead(&animNode->anim, stream, &animNode->props) == NULL)
                {
                    RWRETURN((Rt2dMaestro *)NULL);
                }

                animNode++;
            }

            /* First animation is enabled */
            animNode = (Rt2dAnimNode *)_rwSListGetEntry(maestro->animations, 0);
        }
        else
        {
            /* Failed to create SList for animation and props. */
            RWRETURN(NULL);
        }
    }

    RWRETURN(maestro);
}

Rt2dMaestro *
_rt2dMaestroAnimationsStreamWrite(Rt2dMaestro *maestro, RwStream *stream)
{
    RwInt32                 num;

    RWFUNCTION(RWSTRING("_rt2dMaestroAnimationsStreamWrite"));

    RWASSERT(maestro);
    RWASSERT(stream);

    if (maestro->animations)
    {
        /* Write the number of animations. */
        num = _rwSListGetNumEntries(maestro->animations);

        if (RwStreamWriteInt32(stream, &num, sizeof(RwInt32)) == NULL)
            RWRETURN(NULL);

        /* Write out the animation data. */
        if (num > 0)
        {
            if ((_rt2dMaestroForAllAnimNode(maestro,
                _rt2dMaestroAnimNodeStreamWriteCallBack, (void *) stream)) == NULL)
                RWRETURN(NULL);
        }
    }
    else
    {
        /* No animations. */
        num = 0;
        if (RwStreamWriteInt32(stream, &num, sizeof(RwInt32)) == NULL)
            RWRETURN(NULL);
    }

    RWRETURN(maestro);
}

Rt2dAnimNode *
_rt2dMaestroGetAnimNodeByIndex(Rt2dMaestro *maestro, RwInt32 index)
{
    Rt2dAnimNode        *animNode;

    RWFUNCTION(RWSTRING("_rt2dMaestroGetAnimNodeByIndex"));

    RWASSERT(maestro);

    animNode = (Rt2dAnimNode *)_rwSListGetEntry(maestro->animations, index);

    RWRETURN(animNode);
}


/****************************************************************************/
Rt2dMaestro *
_rt2dMaestroForAllVisibleAnimNode(
                            Rt2dMaestro *maestro,
                            Rt2dMaestroAnimNodeCallBack callback,
                            void *data)
{
    Rt2dAnimNode        *animNode;
    RwInt32             num, i;

    RWFUNCTION(RWSTRING("_rt2dMaestroForAllVisibleAnimNode"));

    RWASSERT(maestro);
    RWASSERT(callback);

    /* Limits of loop */
    RWASSERT(maestro->animations);

    num = _rwSListGetNumEntries(maestro->animations);

    if (num > 0)
    {
        animNode = (Rt2dAnimNode *)_rwSListGetArray(maestro->animations);

        RWASSERT(animNode);

        for (i = 0; i < num; i++)
        {
            RwBool parentVisible =
                (    (animNode->parent == -1) /* root node */
                || ( ((Rt2dAnimNode*)_rwSListGetEntry(
                                          maestro->animations,
                                          animNode->parent))
                          ->flag | RT2D_ANIMNODE_FLAG_VISIBLE
                   )
                );

            maestro->currAnimIndex = i;

            if (   parentVisible
                && (Rt2dObjectIsVisible(animNode->props.scene))
               )
            {
               /* set visibility flag */
               animNode->flag |= RT2D_ANIMNODE_FLAG_VISIBLE;

               /* perform callback for animation */
               if ((callback)
                   (maestro, animNode, data) != maestro)
                   RWRETURN(NULL);
            }
            else
            {
               /* reset visibility flag */
               animNode->flag &= ~RT2D_ANIMNODE_FLAG_VISIBLE;
            }

            animNode++;
        }
    }

    maestro->currAnimIndex = -1;

    RWRETURN(maestro);
}

/****************************************************************************/

Rt2dMaestro *
_rt2dMaestroForAllAnimNode(Rt2dMaestro *maestro,
                            Rt2dMaestroAnimNodeCallBack callback,
                            void *data)
{
    Rt2dAnimNode        *animNode;
    RwInt32             num, i;

    RWFUNCTION(RWSTRING("_rt2dMaestroForAllAnimNode"));

    RWASSERT(maestro);
    RWASSERT(callback);

    /* Limits of loop */
    RWASSERT(maestro->animations);

    num = _rwSListGetNumEntries(maestro->animations);

    if (num > 0)
    {
        animNode = (Rt2dAnimNode *)_rwSListGetArray(maestro->animations);

        RWASSERT(animNode);

        for (i = 0; i < num; i++)
        {
            maestro->currAnimIndex = i;

            if ((callback)(maestro, animNode, data) != maestro)
            {
                RWRETURN((Rt2dMaestro *)NULL);
            }

            animNode++;
        }
    }

    maestro->currAnimIndex = -1;

    RWRETURN(maestro);
}

/**
 * \ingroup rt2dmaestro
 * \ref Rt2dMaestroForAllAnimations is a utility function to apply the
 * given callback to all the animations in the maestro. The callback function
 * is defined as \ref Rt2dMaestroAnimationsCallBack. The animation and its
 * props are passed to the function. Additional private data can be passed
 * via the data parameter.
 *
 * The callback function should return the \ref Rt2dAnim anim if successful
 * or NULL on error. Returning NULL from the callback will terminate the
 * loop immediately.
 *
 * \param maestro          Pointer to parent maestro.
 * \param callback          Pointer to the callback function.
 * \param data              Pointer to private data for the callback function.
 *
 * \return Returns a pointer to the maestro if successful, NULL otherwise
 *
 * \see Rt2dMaestroAddAnimations
 * \see Rt2dMaestroGetAnimationsByIndex
 * \see Rt2dMaestroUpdateAnimations
 *
 */
Rt2dMaestro *
Rt2dMaestroForAllAnimations(Rt2dMaestro *maestro,
                             Rt2dMaestroAnimationsCallBack callback,
                             void *data)
{
    Rt2dAnimNode        *animNode;
    RwInt32             num, i;

    RWAPIFUNCTION(RWSTRING("Rt2dMaestroForAllAnimations"));

    RWASSERT(maestro);
    RWASSERT(callback);

    /* Limits of loop */
    RWASSERT(maestro->animations);

    num = _rwSListGetNumEntries(maestro->animations);

    if (num > 0)
    {
        animNode = (Rt2dAnimNode *)_rwSListGetArray(maestro->animations);

        RWASSERT(animNode);

        for (i = 0; i < num; i++)
        {
            maestro->currAnimIndex = i;

            if ((callback)
               (maestro, &animNode->anim, &animNode->props, data) != maestro)
                RWRETURN(NULL);

            animNode++;
        }
    }

    maestro->currAnimIndex = -1;

    RWRETURN(maestro);
}

/* Adapter so _Rt2dMaestroForAllVisibleAnimNode can be used */
typedef struct _adaptAnimNodeCallBack _adaptAnimNodeCallBack;

struct _adaptAnimNodeCallBack
{
    Rt2dMaestroAnimationsCallBack callBack;
    void *data;
};

static Rt2dMaestro *
_rt2dMaestroAnimNodeCallBackAdaptor(Rt2dMaestro *maestro,
                                   Rt2dAnimNode *animNode,
                                   void *pData)
{
    _adaptAnimNodeCallBack *adapter = (_adaptAnimNodeCallBack *)pData;

    RWFUNCTION(RWSTRING("_rt2dMaestroAnimNodeCallBackAdaptor"));

    /*
    typedef Rt2dMaestro *(*Rt2dMaestroAnimationsCallBack)
        (Rt2dMaestro *maestro, Rt2dAnim *anim, Rt2dAnimProps *props, void * pData);
    */

    RWRETURN ((adapter->callBack)
                (maestro, &animNode->anim, &animNode->props, adapter->data));
}

/**
 * \ingroup rt2dmaestro
 * \ref Rt2dMaestroForAllVisibleAnimations is a utility function to apply the
 * given callback to all the visible animations in the maestro. The callback
 * function is defined as \ref Rt2dMaestroAnimationsCallBack. The animation
 * and its props are passed to the function. Additional private data can be
 * passed via the data parameter.
 *
 * The callback function should return the \ref Rt2dAnim anim if successful
 * or NULL on error. Returning NULL from the callback will terminate the
 * loop immediately.
 *
 * \param maestro          Pointer to parent maestro.
 * \param callBack         Pointer to the callback function.
 * \param data             Pointer to private data for the callback function.
 *
 * \return Returns a pointer to the maestro if successful, NULL otherwise
 *
 * \see Rt2dMaestroAddAnimations
 * \see Rt2dMaestroGetAnimationsByIndex
 * \see Rt2dMaestroUpdateAnimations
 *
 */
Rt2dMaestro *
Rt2dMaestroForAllVisibleAnimations(
                             Rt2dMaestro *maestro,
                             Rt2dMaestroAnimationsCallBack callBack,
                             void *data)
{
    _adaptAnimNodeCallBack adapter;

    RWAPIFUNCTION(RWSTRING("Rt2dMaestroForAllVisibleAnimations"));

    RWASSERT(maestro);
    RWASSERT(callBack);

    /* Set up adapter */
    adapter.callBack = callBack;
    adapter.data = data;

    /* Do loop */
    _rt2dMaestroForAllVisibleAnimNode(maestro,
                                      _rt2dMaestroAnimNodeCallBackAdaptor,
                                      &adapter);

    RWRETURN(maestro);
}

/**
 * \ingroup rt2dmaestro
 * \ref Rt2dMaestroAddAnimations is used to add a new animation to the
 * maestro. The associated props and cel list must also be given at the same time.
 * The maestro maintains a list of animations that are to be played.
 *
 * The new animation is copied into maestro's animation list, rather than creating
 * a reference. This allows the maestro to own the animation in the list and the
 * given animation can be destroyed. The other parameters, props and cel list, are
 * also copied.
 *
 * The parent index is the animations's parent's animation index in the maestro.
 * If it is the root animation, then the parent index can be set to -1.
 *
 * The posInParentScene is an index to the animation's scene in the parent's scene.
 *
 * The animation's index is returned in the index parameter if not NULL.
 *
 * \param maestro              Pointer to parent maestro.
 * \param parent                Index to parent animation in the maestro.
 * \param posInParentScene      Index to parent object in the maestro's scene.
 * \param anim                  Pointer to the animation to be added.
 * \param props                 Pointer to the associate props.
 * \param celList               Pointer to the associate cel list.
 * \param index                 Pointer to a RwInt32 to return the anim's index in the maestro.
 *
 * \return Returns a pointer to the maestro if successful, NULL otherwise.
 *
 * \see Rt2dMaestroGetAnimationsByIndex
 * \see Rt2dMaestroGetAnimPropsByIndex
 * \see Rt2dMaestroGetCelListByIndex
 */
Rt2dMaestro *
Rt2dMaestroAddAnimations(Rt2dMaestro *maestro,
                          RwInt32 parent, RwInt32 posInParentScene,
                          Rt2dAnim *anim, Rt2dAnimProps *props __RWUNUSED__,
                          Rt2dCelList *celList, RwInt32 *index)
{
    Rt2dObject              *scene;
    Rt2dAnimNode            *newAnimNode;
    RwInt32                 newIndex;

    RWAPIFUNCTION(RWSTRING("Rt2dMaestroAddAnimations"));

    RWASSERT(maestro);

    RWASSERT(maestro->animations);

    RWASSERT(maestro->flag & RT2D_MAESTRO_FLAG_LOCK);

    newIndex = _rwSListGetNumEntries(maestro->animations);

    if (index)
    {
        *index = newIndex;
    }

    /* Ask for new data area. */
    if ((newAnimNode =
          (Rt2dAnimNode *)_rwSListGetNewEntry(maestro->animations,
            rwID_2DANIM | rwMEMHINTDUR_EVENT)) == NULL)
    {
        RWRETURN((Rt2dMaestro *)NULL);
    }

    if (_rt2dAnimNodeInit(newAnimNode) == NULL)
    {
        RWRETURN((Rt2dMaestro *)NULL);
    }

    newAnimNode->parent = parent;
    newAnimNode->posInParentScene = posInParentScene;

    scene = Rt2dMaestroGetAnimSceneByIndex(maestro, newIndex);

    /* Create the props for this anim. */
    if ((_rt2dAnimPropsInit(&newAnimNode->props, scene)) == NULL)
    {
        RWRETURN((Rt2dMaestro *)NULL);
    }

    /* Copy the anim */
    if ((_rt2dAnimCopy(&newAnimNode->anim, anim, &newAnimNode->props)) == NULL)
    {
        RWRETURN((Rt2dMaestro *)NULL);
    }

    /* Copy the celList */
    if ((_rt2dCelListCopy(newAnimNode->celList, celList)) == NULL)
    {
        RWRETURN((Rt2dMaestro *)NULL);
    }

    RWRETURN(maestro);
}

/**
 * \ingroup rt2dmaestro
 * \ref Rt2dMaestroGetAnimationsByIndex is used to return an animation in the
 * maestro referenced by the index. This index should be the index returned
 * by \ref Rt2dMaestroAddAnimations.
 *
 * \param maestro              Pointer to the parent maestro.
 * \param index                 Animation's index in the maestro.
 *
 * \return Returns a pointer to an animation if successful, NULL otherwise.
 *
 * \see Rt2dMaestroAddAnimations
 * \see Rt2dMaestroGetAnimPropsByIndex
 * \see Rt2dMaestroGetCelListByIndex
 */
Rt2dAnim *
Rt2dMaestroGetAnimationsByIndex(Rt2dMaestro *maestro, RwInt32 index)
{
    Rt2dAnimNode        *animNode;

    RWAPIFUNCTION(RWSTRING("Rt2dMaestroGetAnimationsByIndex"));

    RWASSERT(maestro);

    animNode = _rt2dMaestroGetAnimNodeByIndex(maestro, index);

    RWRETURN(&animNode->anim);
}

/**
 * \ingroup rt2dmaestro
 * \ref Rt2dMaestroUpdateAnimations is used update all the animations in
 * the maestro. Updating the animation is normally required as a result
 * of a time increment.
 *
 * \param maestro              Pointer to parent maestro.
 *
 * \return Returns a pointer to the maestro if successful, NULL otherwise.
 *
 * \see Rt2dMaestroAddDeltaTime
 * \see Rt2dAnimTimeUpdate
 */

Rt2dMaestro *
Rt2dMaestroUpdateAnimations(Rt2dMaestro *maestro)
{
    RWAPIFUNCTION(RWSTRING("Rt2dMaestroUpdateAnimations"));

    RWASSERT(maestro);

    RWASSERT(!(maestro->flag & RT2D_MAESTRO_FLAG_LOCK));

    if (maestro->animations)
    {
        if (NULL == _rt2dMaestroForAllVisibleAnimNode(maestro,
                                     _rt2dMaestroAnimNodeUpdateCallBack, NULL) )
        {
            RWRETURN(NULL);
        }
    }

    RWRETURN(maestro);
}

/**
 * \ingroup rt2dmaestro
 * \ref Rt2dMaestroGetAnimPropsByIndex is used to return an animation's props
 * in the maestro referenced by the index. This index should be the index returned
 * by \ref Rt2dMaestroAddAnimations.
 *
 * \param maestro          Pointer to parent maestro.
 * \param index            Index to animation prop in the maestro.
 *
 * \return Returns a pointer to the anim props if successful, NULL otherwise.
 *
 * \see Rt2dMaestroAddAnimations
 * \see Rt2dMaestroGetAnimationsByIndex
 * \see Rt2dMaestroGetCelListByIndex
 */
Rt2dAnimProps *
Rt2dMaestroGetAnimPropsByIndex(Rt2dMaestro *maestro, RwInt32 index)
{
    Rt2dAnimNode        *animNode;

    RWAPIFUNCTION(RWSTRING("Rt2dMaestroGetAnimPropsByIndex"));

    RWASSERT(maestro);

    animNode = _rt2dMaestroGetAnimNodeByIndex(maestro, index);

    RWRETURN(&animNode->props);
}

/**************************************************************************
 *
 * Maestro's Lock and Unlock.
 *
 *
 **************************************************************************/

static Rt2dMaestro *
_rt2dMaestroAnimNodeLockCallBack(Rt2dMaestro *maestro,
                                  Rt2dAnimNode *animNode,
                                  void * pData __RWUNUSED__)
{
    Rt2dObject              *scene;
    RwInt32                 index;

    RWFUNCTION(RWSTRING("_rt2dMaestroAnimNodeLockCallBack"));

    RWASSERT(maestro);
    RWASSERT(animNode);

    if (_rt2dAnimPropsDestruct(&animNode->props) == NULL)
    {
        RWRETURN(NULL);
    }

    index = animNode - ((Rt2dAnimNode *)_rwSListGetArray(maestro->animations));

    scene = Rt2dMaestroGetAnimSceneByIndex(maestro, index);

    if (_rt2dAnimPropsInit(&animNode->props, scene) == NULL)
    {
        RWRETURN(NULL);
    }

    if (Rt2dAnimLock(&animNode->anim, &animNode->props) == NULL)
    {
        RWRETURN(NULL);
    }

    if (Rt2dCelListLock(animNode->celList) == NULL)
    {
        RWRETURN(NULL);
    }

    RWRETURN(maestro);
}

static Rt2dMaestro *
_rt2dMaestroAnimNodeUnlockCallBack(Rt2dMaestro *maestro,
                                    Rt2dAnimNode *animNode,
                                    void * pData __RWUNUSED__)
{
    Rt2dObject          *scene;
    RwInt32             index;

    RWFUNCTION(RWSTRING("_rt2dMaestroAnimNodeUnlockCallBack"));

    RWASSERT(maestro);
    RWASSERT(animNode);

    if (_rt2dAnimPropsDestruct(&animNode->props) == NULL)
        RWRETURN(NULL);

    index = animNode - ((Rt2dAnimNode *)_rwSListGetArray(maestro->animations));

    scene = Rt2dMaestroGetAnimSceneByIndex(maestro, index);

    if (_rt2dAnimPropsInit(&animNode->props, scene) == NULL)
        RWRETURN(NULL);

    if (Rt2dAnimUnlock(&animNode->anim, &animNode->props) == NULL)
        RWRETURN(NULL);

    if (Rt2dCelListUnlock(animNode->celList) == NULL)
        RWRETURN(NULL);

    RWRETURN(maestro);
}

/**
 * \ingroup rt2dmaestro
 * \ref Rt2dMaestroLock is used to lock a maestro for modification. The
 * maestro's animations, scene and cel lists are also locked.
 *
 * A maestro can only be locked once. Attempts to lock a locked maestro will do
 * nothing.
 *
 * \param maestro      Pointer to the maestro to lock.
 *
 * \return Returns a pointer to the maestro if successful, NULL otherwise.
 *
 * \see Rt2dMaestroUnlock
 * \see Rt2dMaestroCreate
 */
Rt2dMaestro *
Rt2dMaestroLock(Rt2dMaestro * maestro)
{
    RWAPIFUNCTION(RWSTRING("Rt2dMaestroLock"));

    RWASSERT(maestro);

    RWASSERT(!(maestro->flag & RT2D_MAESTRO_FLAG_LOCK));

    /* Limits of loop */
    RWASSERT(maestro->animations);

    if (Rt2dSceneLock(maestro->scene) == NULL)
        RWRETURN(maestro);

    if ((_rt2dMaestroForAllAnimNode(maestro,
            _rt2dMaestroAnimNodeLockCallBack, NULL)) == NULL)
        RWRETURN(NULL);

    maestro->flag |= RT2D_MAESTRO_FLAG_LOCK;

    RWRETURN(maestro);
}

/**
 * \ingroup rt2dmaestro
 * \ref Rt2dMaestroUnlock is used to unlock a maestro after modification. The
 * maestro's animations, scene and cel lists are also unlocked.
 *
 * Only a locked maestro can be unlocked. Attempts to unlock an unlocked maestro will do
 * nothing.
 *
 * \param maestro      Pointer to the maestro to unlock.
 *
 * \return Returns a pointer to the maestro if successful, NULL otherwise.
 *
 * \see Rt2dMaestroLock
 * \see Rt2dMaestroCreate
 */
Rt2dMaestro *
Rt2dMaestroUnlock(Rt2dMaestro *maestro)
{
    RWAPIFUNCTION(RWSTRING("Rt2dMaestroUnlock"));

    RWASSERT(maestro);

    RWASSERT(maestro->flag & RT2D_MAESTRO_FLAG_LOCK);

    /* Limits of loop */
    RWASSERT(maestro->animations);

    if (Rt2dSceneUnlock(maestro->scene) == NULL)
        RWRETURN(maestro);

    if ((_rt2dMaestroForAllAnimNode(maestro,
            _rt2dMaestroAnimNodeUnlockCallBack, NULL)) == NULL)
        RWRETURN(NULL);

    maestro->flag &= ~RT2D_MAESTRO_FLAG_LOCK;

    RWRETURN(maestro);
}

/****************************************************************************
 *
 * Maestro's scene functions.
 *
 *
 **************************************************************************/

RwInt32
_rt2dMaestroSceneStreamGetSize(Rt2dMaestro *maestro)
{
    RwInt32                 size;

    RWFUNCTION(RWSTRING("_rt2dMaestroSceneStreamGetSize"));

    RWASSERT(maestro);

    size = 0;

    /* Size of the label SList. */
    if (maestro->scene)
    {
        size += Rt2dSceneStreamGetSize(maestro->scene);
    }

    RWRETURN(size);
}

Rt2dMaestro *
_rt2dMaestroSceneStreamRead(Rt2dMaestro *maestro, RwStream *stream)
{
    RwInt32                 size;

    RWFUNCTION(RWSTRING("_rt2dMaestroSceneStreamRead"));

    RWASSERT(maestro);
    RWASSERT(stream);

    if (RwStreamReadInt32(stream, &size, sizeof(RwInt32)) == NULL)
        RWRETURN(NULL);

    if (size > 0)
    {
        RwUInt32                 size, version;
        /* Read in the stream chunk header. */
        if (!RwStreamFindChunk(stream, rwID_2DSCENE, &size, &version))
            RWRETURN(NULL);

        if ((maestro->scene = Rt2dSceneStreamRead(stream)) == NULL)
            RWRETURN(NULL);
    }

    RWRETURN(maestro);
}

Rt2dMaestro *
_rt2dMaestroSceneStreamWrite(Rt2dMaestro *maestro, RwStream *stream)
{
    RwInt32                 size;

    RWFUNCTION(RWSTRING("_rt2dMaestroSceneStreamWrite"));

    RWASSERT(maestro);
    RWASSERT(stream);

    /* Write out scene size. */
    size = _rt2dMaestroSceneStreamGetSize(maestro);
    if (RwStreamWriteInt32(stream, &size, sizeof(RwInt32)) == NULL)
        RWRETURN(NULL);

    if (size > 0)
    {
        RWASSERT(maestro->scene);

        /* Write out the scene. */
        Rt2dSceneStreamWrite(maestro->scene, stream);
    }

    RWRETURN(maestro);
}

/**
 * \ingroup rt2dmaestro
 * \ref Rt2dMaestroGetScene is used to retrieve the scene attached to the
 * maestro.
 *
 * \param maestro      Pointer to the parent maestro.
 *
 * \return Returns a pointer to a scene if successful, NULL otherwise.
 *
 * \see Rt2dMaestroSetScene
 * \see Rt2dSceneCreate
 */
Rt2dObject *
Rt2dMaestroGetScene(Rt2dMaestro *maestro)
{
    RWAPIFUNCTION(RWSTRING("Rt2dMaestroGetScene"));

    RWASSERT(maestro);

    RWRETURN(maestro->scene);
}

/**
 * \ingroup rt2dmaestro
 * \ref Rt2dMaestroSetScene sets the scene used by the maestro. The
 * maestro only creates a reference to the scene.
 *
 * \param maestro Pointer to the parent maestro.
 * \param scene    Pointer to the scene.
 *
 * \return Returns a pointer to the maestro if successful, NULL otherwise.
 *
 * \see Rt2dMaestroSetScene
 * \see Rt2dSceneCreate
 */
Rt2dMaestro *
Rt2dMaestroSetScene(Rt2dMaestro *maestro, Rt2dObject *scene)
{
    RWAPIFUNCTION(RWSTRING("Rt2dMaestroSetScene"));

    RWASSERT(maestro);
    RWASSERT(scene);

    maestro->scene = scene;

    RWRETURN(maestro);
}

/**************************************************************************/

static Rt2dMaestro *
_rt2dMaestroAnimNodeAddDeltaTimeCallBack(Rt2dMaestro *maestro,
                                          Rt2dAnimNode *animNode,
                                          void *pData)
{
    RwReal              deltaTime;
    RwInt32             currFrameIndex, nextFrameIndex, messageListIndex;
    Rt2dCel             *cel;

    RWFUNCTION(RWSTRING("_rt2dMaestroAnimNodeAddDeltaTimeCallBack"));

    RWASSERT(maestro);
    RWASSERT(animNode);
    RWASSERT(pData);

    deltaTime = *(RwReal *)pData;

    currFrameIndex = Rt2dAnimGetPrevFrameIndex(&animNode->anim);

    if (animNode->flag & RT2D_ANIMNODE_FLAG_DO_CEL_ACTIONS)
    {
        /* Okay, we're onto it! */
        animNode->flag &= (~RT2D_ANIMNODE_FLAG_DO_CEL_ACTIONS);

        /* Get the correct cel using the nextFrameIndex. */
        cel = (Rt2dCel *)_rwSListGetArray(animNode->celList->cel);
        cel += currFrameIndex;

        messageListIndex = cel->messageListIndex;

        if (messageListIndex >= 0)
            _rt2dMaestroMessageListPostByIndex(maestro, messageListIndex);

        Rt2dMaestroProcessMessages(maestro);

        _rt2dApplyMouseState(maestro, &maestro->prevMouse.position);
    }

    if (   Rt2dObjectIsVisible(animNode->props.scene)
         &&(!(animNode->flag & RT2D_ANIMNODE_FLAG_STOP))
       )
    {
#ifdef GJB_DEBUG
        RWMESSAGE((RWSTRING("Animation %d"), maestro->currAnimIndex));
#endif

        if ((Rt2dAnimAddDeltaTime(&animNode->anim, &animNode->props, deltaTime)) == NULL)
        {
            RWRETURN((Rt2dMaestro *)NULL);
        }

        /* Check the frame has moved on, is so then post any messages in the index. */
        nextFrameIndex = Rt2dAnimGetPrevFrameIndex(&animNode->anim);

        if (currFrameIndex != nextFrameIndex)
        {
            /* Get the correct cel using the nextFrameIndex. */
            cel = (Rt2dCel *)_rwSListGetArray(animNode->celList->cel);
            cel += nextFrameIndex;

            messageListIndex = cel->messageListIndex;

            if (messageListIndex >= 0)
            {
                _rt2dMaestroMessageListPostByIndex(maestro, messageListIndex);
            }
        }
    }


    RWRETURN(maestro);
}

static Rt2dMaestro *
EndLooping(Rt2dMaestro *maestro,
           Rt2dAnimNode *animNode,
           void *pData)
{
    RwBool forceChildReset = (RwBool)pData;
    RWFUNCTION(RWSTRING("EndLooping"));

    RWASSERT(maestro);
    RWASSERT(animNode);

    /* if we passed the end of the animation and we're not stopped, we need
     * to go back to the start, and so do all child animations
     */
    if (    (   !(animNode->flag & RT2D_ANIMNODE_FLAG_STOP)
              && (animNode->anim.currentTime >= Rt2dAnimGetFinalKeyFrameListTime(&animNode->anim))
            )
         || forceChildReset
       )
    {
        Rt2dAnimNode *end = (Rt2dAnimNode *)_rwSListGetEnd(maestro->animations);
        RwInt32 startIndex = animNode - (Rt2dAnimNode *)_rwSListGetArray(maestro->animations);

        Rt2dAnimGotoKeyFrameListByIndex(&animNode->anim, &animNode->props, 0);
        animNode->flag |= RT2D_ANIMNODE_FLAG_DO_CEL_ACTIONS;

        /* Do the same for all children, but force reset */
        ++animNode;
        while(animNode != end)
        {
            if (animNode->parent==startIndex)
            {
                Rt2dObject *scene
                     = Rt2dSceneGetChildByIndex(
                            Rt2dMaestroGetAnimSceneByIndex(maestro, animNode->parent),
                            animNode->posInParentScene);
                EndLooping(maestro, animNode, (void *)TRUE);

                /* Parent must hide child animation at end */
                Rt2dObjectSetVisible(scene, FALSE);
            }
            ++animNode;
        }

    }

    RWRETURN(maestro);
}

/**
 * \ingroup rt2dmaestro
 * \ref Rt2dMaestroAddDeltaTime increments the time in all the maestro's
 * animations by the specified amount. If the time increments takes the
 * animation to the next frame, then any messages at the start of the
 * next frame are posted and processed.
 *
 * \param maestro          Pointer to the parent maestro.
 * \param deltaTime         The time increment amount.
 *
 * \return Returns a pointer to the maestro if successful, NULL otherwise.
 *
 * \see Rt2dMaestroUpdateAnimations
 * \see Rt2dMaestroProcessMessages
 */
Rt2dMaestro *
Rt2dMaestroAddDeltaTime(Rt2dMaestro *maestro, RwReal deltaTime)
{
    RwReal              currTime=0.0f;
    RwReal              nextTime=0.0f;
    RwReal              frameStepTime=0.0f;
    RwBool              nextFrame;
    Rt2dAnimNode        *animNode, *endAnimNode;
    RWAPIFUNCTION(RWSTRING("Rt2dMaestroAddDeltaTime"));

    RWASSERT(maestro);

    RWASSERT(deltaTime >= 0.0f );

    RWASSERT(!(maestro->flag & RT2D_MAESTRO_FLAG_LOCK));

    /*
     * This section determines a nominal 'time of current frame' and a
     * suitable time step to move all the playing animations in the Rt2dMaestro 
     * forward.
     *
     * Slightly complex, because Flash
     *  -has a default framerate (Maestro doesn't)
     *  -automatically loops at the end
     *  -may have a 'stopped' animation with subanimations still playing
     *
     * We assume
     *  -all animations will have the same timestep, so just get a reasonable
     *   value from the currently playing animations.
     *  -the 'frameStepTime' will not change as time moves forward
     *
     */
    endAnimNode    = (Rt2dAnimNode *)_rwSListGetEnd(maestro->animations);
    for (animNode =  (Rt2dAnimNode *)_rwSListGetArray(maestro->animations);
         animNode != endAnimNode;
         ++animNode)
    {
        /* Animation must not be stopped to be used for time advancement calc */
        if (0==(animNode->flag & RT2D_ANIMNODE_FLAG_STOP))
        {
            RwReal prevTime;

            if (animNode->anim.next)
            {
                prevTime = Rt2dAnimGetPrevFrameTime(&animNode->anim);

                currTime = Rt2dAnimGetCurrentTime(&animNode->anim);

                nextTime = Rt2dAnimGetNextFrameTime(&animNode->anim);
                    
                frameStepTime = nextTime - prevTime;  

                break; /* have a current time and a step time */
            }
            else
            {
                RwInt32 index = Rt2dAnimGetPrevFrameIndex(&animNode->anim);

                /* This animation is playing, but is at end. May be possible
                 * to calculate a time step from previous frame time, if this
                 * frame exists. This is necessary to deal with the case that
                 * all animations/subanimations are at the end.
                 */
                if (index>0)
                {
                    Rt2dKeyFrameList *kflist = Rt2dAnimGetKeyFrameListByIndex(
                                                    &animNode->anim, index-1);
                    prevTime = kflist->time;

                    nextTime = Rt2dAnimGetPrevFrameTime(&animNode->anim);

                    frameStepTime = nextTime - prevTime;

                    /* fake a time in the previous frame */
                    currTime = Rt2dAnimGetCurrentTime(&animNode->anim) - frameStepTime;

                    break; /* have a current time and a step time */
                }
            }
        }
    }

    /* If we didn't find a running animation, everything's stopped */
    if (!frameStepTime)  /* This is zero or +ve, so don't need tolerance test */
    {
        RWRETURN(maestro);
    }

    /* Main loop. */
    while (deltaTime > RWZEROTOL)
    {
        RwReal timeTillEndOfFrame = nextTime - currTime;
        RwReal timeToAdvance;

        /* Advance the frame if necessary */
        if (deltaTime > timeTillEndOfFrame)
        {
            deltaTime -= timeTillEndOfFrame;
            currTime += timeTillEndOfFrame;
            nextTime += frameStepTime;
            timeToAdvance = timeTillEndOfFrame;
            nextFrame = TRUE;
        }
        else
        {
            timeToAdvance = deltaTime;
            deltaTime = 0.0f;
            nextFrame = FALSE;
        }

        /* Add the step time to all animations. */
        if ((_rt2dMaestroForAllVisibleAnimNode(
            maestro,
            _rt2dMaestroAnimNodeAddDeltaTimeCallBack,
            (void *) &timeToAdvance)
           ) == NULL)
        {
            RWRETURN(NULL);
        }

        if(nextFrame)
        {
            /* Process any messages if we reached the start of the next frame. */
            Rt2dMaestroProcessMessages(maestro);
            _rt2dApplyMouseState(maestro, &maestro->prevMouse.position);

            nextFrame = FALSE;
        }

        /* Do end-looping for each non-stopped animation */
        _rt2dMaestroForAllAnimNode(
            maestro,
            EndLooping,
            (void *)FALSE
        );
    }

    RWRETURN(maestro);
}

/**************************************************************************/

/**
 * \ingroup rt2dmaestro
 * \ref Rt2dMaestroGetAnimSceneByIndex is used returned the animation's scene
 * object in the maestro by the given index.
 *
 * This is the index returned by \ref Rt2dMaestroAddAnimations.
 *
 * \param maestro          Pointer to parent maestro.
 * \param index            Animation's index in the maestro.
 *
 * \return Pointer to the scene object if successful, NULL otherwise.
 *
 * \see Rt2dMaestroAddAnimations
 * \see Rt2dMaestroGetAnimationsByIndex
 * \see Rt2dMaestroGetAnimPropsByIndex
 * \see Rt2dMaestroGetCelListByIndex
 */
Rt2dObject *
Rt2dMaestroGetAnimSceneByIndex(Rt2dMaestro *maestro, RwInt32 index)
{
    Rt2dAnimNode            *animNode;
    Rt2dObject              *result;

    RWAPIFUNCTION(RWSTRING("Rt2dMaestroGetAnimSceneByIndex"));

    RWASSERT(maestro);

    if (index > 0)
    {
        animNode = (Rt2dAnimNode *)
                        _rwSListGetEntry(maestro->animations, index);

        result =
            Rt2dSceneGetChildByIndex(
                Rt2dMaestroGetAnimSceneByIndex(maestro, animNode->parent),
                animNode->posInParentScene);
    }
    else
    {
        result = maestro->scene;
    }

    RWRETURN(result);
}

/**************************************************************************/

/**
 * \ingroup rt2dmaestro
 * \ref Rt2dMaestroRender is used render the scene attached to the maestro.
 *
 * \param maestro Pointer to the maestro to render.
 *
 * \return Returns a pointer to the maestro if successful, NULL otherwise.
 *
 * \see Rt2dMaestroCreate
 */
Rt2dMaestro *
Rt2dMaestroRender(Rt2dMaestro *maestro)
{
    RWAPIFUNCTION(RWSTRING("Rt2dMaestroRender"));

    RWASSERT(maestro);

    if (maestro->scene)
    {
        if ((Rt2dSceneRender(maestro->scene)) == NULL)
        {
            RWRETURN((Rt2dMaestro *)NULL);
        }
    }

    RWRETURN(maestro);
}

/**************************************************************************
 *
 * Maestro's Bounding box.
 *
 *
 **************************************************************************/

/**
 * \ingroup rt2dmaestro
 * \ref Rt2dMaestroGetBBox is used return the bounding box that bounds all
 * the objects in the maestro's scene.
 *
 * \param maestro Pointer to the maestro.
 *
 * \return Returns a pointer to the maestro's bounding box.
 *
 * \see Rt2dMaestroCreate
 * \see Rt2dMaestroSetBBox
 */

Rt2dBBox *
Rt2dMaestroGetBBox(Rt2dMaestro *maestro)
{
    RWAPIFUNCTION(RWSTRING("Rt2dMaestroGetBBox"));

    RWASSERT(maestro);

    RWRETURN(&maestro->bbox);
}
/**
 *
 * \ingroup rt2dmaestro
 * \ref Rt2dMaestroSetBBox is used set the bounding box in the maestro.
 * The bounding box should bounds all the objects in the maestro's scene.
 *
 * \param maestro  Pointer to the maestro.
 * \param bbox      Pointer to the bounding box.
 *
 * \return Returns a pointer to the maestro's bounding box.
 *
 * \see Rt2dMaestroCreate
 * \see Rt2dMaestroSetBBox
 */

Rt2dMaestro *
Rt2dMaestroSetBBox(Rt2dMaestro *maestro, Rt2dBBox *bbox)
{
    RWAPIFUNCTION(RWSTRING("Rt2dMaestroSetBBox"));

    RWASSERT(maestro);
    RWASSERT(bbox);

    maestro->bbox = *bbox;

    RWRETURN(maestro);
}

#if defined (__MWERKS__)
#if (defined(RWVERBOSE))
#pragma message (__FILE__ "/" _SKY_EXPAND(__LINE__) ": __MWERKS__ == " _SKY_EXPAND(__MWERKS__))
#endif /* (defined (__MWERKS__)) */
#if (__option (global_optimizer))
#pragma always_inline on
#endif /* (__option (global_optimizer)) */
#endif /*  defined (__MWERKS__) */
