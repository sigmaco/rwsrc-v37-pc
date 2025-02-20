/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>
#include "rpdbgerr.h"

#include "rpteam.h"

#if (defined(SKY2_DRVMODEL_H))
#include "rppds.h"
#endif /* (defined(SKY2_DRVMODEL_H)) */

#include "teamdictionary.h"
#include "team.h"

/*
 *

  .....   ......    ...  ......  ......    ..    ..  ..    ..    ....    ..  ..
 =====.. ======.   ===. ======. ======.   ==..  ==. ==.   ==..  ====..  ==. ==.
 ======. ======/  ====/ ======/ ======/  ====.. ==. ==.  ====.. =====.. ==. ==.
 ==. ==.   ==.   ==.      ==.     ==.   ==. ==. ===.==. ==. ==. ==. ==. ==. ==.
 ==. ==.   ==.   ==.      ==.     ==.   ==. ==. ===.==. ==..==. ==..==/ ==. ==.
 ==. ==.   ==.   ==.      ==.     ==.   ==. ==. ======. ======. =====/  ==..==/
 ==. ==.   ==.   ==.      ==.     ==.   ==. ==. ======. ======. ====..   ====/
 ==. ==.   ==.   ==.      ==.     ==.   ==. ==. ==.===. ==. ==. ==.==.    ==.
 ==..==.  .==... ==...    ==.    .==... ==..==/ ==.===. ==. ==. ==.==..   ==.
 ======/ ======. ====.    ==.   ======.  ====/  ==. ==. ==. ==. ==. ==.   ==.
 =====/  ======/  ===/    ==/   ======/   ==/   ==/ ==/ ==/ ==/ ==/ ==/   ==/

 *
 */

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

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

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/
static RpMaterial *
TeamDictionaryReplaceMaterialPipelines( RpMaterial *material,
                                        void *data )
{
    RxPipeline **newPipelines;
    RxPipeline *oldPipeline;
    RwUInt32 index;

    RWFUNCTION(RWSTRING("TeamDictionaryReplaceMaterialPipelines"));
    RWASSERT(NULL != material);
    RWASSERT(NULL != data);

    newPipelines = (RxPipeline **)data;
    RWASSERT(NULL != newPipelines);

    material = RpMaterialGetPipeline(material, &oldPipeline);
    RWASSERT(NULL != material);

    /* Only replace the pipeline is one exists. */
    if(NULL != oldPipeline)
    {
        RWASSERT(rwID_TEAMPLUGIN == oldPipeline->pluginId);
        index = oldPipeline->pluginData;

        RWASSERT(NULL != newPipelines[index]);
        material = RpMaterialSetPipeline(material, newPipelines[index]);
        RWASSERT(NULL != material);
    }

    RWRETURN(material);
}

static RpAtomic *
TeamDictionaryReplacePipelines( RpAtomic *atomic,
                                RxPipeline **newPipelines )
{
    RpGeometry *geometry;
    RxPipeline *oldPipeline;
    RwUInt32 index;

    RWFUNCTION(RWSTRING("TeamDictionaryReplacePipelines"));
    RWASSERT(NULL != atomic);
    RWASSERT(NULL != newPipelines);

    /* First replace the atomic pipeline. */
    RpAtomicGetPipeline(atomic, &oldPipeline);
    RWASSERT(NULL != oldPipeline);

#if (defined(TEAMPIPEEXTENSIONS))
    oldPipeline = ((TeamPipeDefinition *)oldPipeline)->render;
#endif /* (defined(TEAMPIPEEXTENSIONS)) */

    /* Get the pipelines index. */
    RWASSERT(rwID_TEAMPLUGIN == oldPipeline->pluginId);
    index = oldPipeline->pluginData;
    RWASSERT(NULL != newPipelines[index]);

#if (defined(TEAMPIPEEXTENSIONS))
    ((TeamPipeDefinition *)(atomic->pipeline))->render = newPipelines[index];
#else /* (defined(TEAMPIPEEXTENSIONS)) */
    atomic = RpAtomicSetPipeline(atomic, newPipelines[index]);
    RWASSERT(NULL != atomic);
#endif /* (defined(TEAMPIPEEXTENSIONS)) */

    geometry = RpAtomicGetGeometry(atomic);
    RWASSERT(NULL != geometry);

    geometry = RpGeometryForAllMaterials(
                   geometry,
                   TeamDictionaryReplaceMaterialPipelines,
                   (void *)newPipelines );
    RWASSERT(NULL != geometry);

    RWRETURN(atomic);
}

static RpAtomic *
TeamDictionaryRenderCallBack(RpAtomic *atomic)
{
    RWFUNCTION(RWSTRING("TeamDictionaryRenderCallBack"));
    RWASSERT(NULL != atomic);

    if(_rpTeamPipeRenderAtomic(atomic))
    {
        RWRETURN(atomic);
    }

    RWRETURN((RpAtomic *)NULL);
}

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/
RpTeamDictionaryElement *
_rpTeamDictionaryDestroyElement( RpTeamDictionaryElement *element )
{
    RwUInt32 i;

    RWFUNCTION(RWSTRING("_rpTeamDictionaryDestroyElement"));
    RWASSERT(NULL != element);

    for( i = 0; i < rpTEAMELEMENTLODSUP; i++ )
    {
        _rpTeamDictionaryDestroyLODElement(element, i);
    }

    element->type = rpNATEAMELEMENTTYPE;

    RWRETURN(element);
}

RpTeamDictionaryElement *
_rpTeamDictionaryDestroyLODElement( RpTeamDictionaryElement *element,
                                    RwUInt32 lodIndex )
{
    RWFUNCTION(RWSTRING("_rpTeamDictionaryDestroyLODElement"));
    RWASSERT(NULL != element);
    RWASSERT(rpTEAMELEMENTLODSUP > lodIndex);

    if( NULL != element->element[lodIndex] )
    {
        RpAtomic *atomic = element->element[lodIndex];

        /* Special case destruction. */
        switch(element->type)
        {
            case rpTEAMELEMENTSTATIC:
            {
                RwFrame *frame;

                /* Grab the atomics frame. */
                frame = RpAtomicGetFrame(atomic);
                RWASSERT(NULL != frame);

                /* Forget it. */
                RpAtomicSetFrame(atomic, (RwFrame *)NULL);

                /* Destroy it. */
                RwFrameDestroy(frame);

                break;
            }
            case rpTEAMELEMENTSKINNED:
            case rpTEAMELEMENTPATCH:
            {
                break;
            }
            default:
            {
                /* Shouldn't get here. */
                RWASSERT(FALSE);
                break;
            }
        }

        /* Destroy the atomic. */
        RpAtomicDestroy(atomic);

        element->element[lodIndex] = (RpAtomic *)NULL;
    }

    RWRETURN(element);
}

RpTeamPlayer *
_rpTeamDictionarySetElement( RpTeamPlayer *player,
                             RwUInt32 playerIndex,
                             RwUInt32 dictionaryIndex )
{
    RpTeamDictionary *dictionary;
    RpTeamDictionaryElement *element;

    RwUInt32 present;

    RWFUNCTION(RWSTRING("_rpTeamDictionarySetElement"));
    RWASSERT(NULL != player);
    RWASSERT(player->team->data.numOfPlayerElements > playerIndex);

    /* Get the present element; */
    present = player->elements[playerIndex];

    /* Get the team's dictionary. */
    dictionary = player->team->dictionary;
    RWASSERT(NULL != dictionary);

    /* Is the slot already in use? */
    if( rpTEAMELEMENTUNUSED != present )
    {
        /* Make things a little easier. */
        element = &(dictionary->elements[present]);

        /* Yes - then lets get rid of it. */
        RWASSERT(0 < element->refCount);

        element->refCount --;

        player->elements[playerIndex] = rpTEAMELEMENTUNUSED;

        /* Is the player selected. */
        if(player->flags & TEAMPLAYERSELECTED)
        {
            /* Then also reduce the selection count. */
            player->team->selected[present] --;

            /* Need to flag that type of element as no longer optimized. */
            _rpTeamSetElementTypeOptimized( player->team,
                                            element->type,
                                            rpNATEAMLOCKMODE,
                                            FALSE );

            RWASSERT(    element->refCount
                      >= player->team->selected[present] );
        }
    }

    /* Now lets get the new element from the dictionary. */

    /* Are we setting an element. */
    if( rpTEAMELEMENTUNUSED != dictionaryIndex )
    {
        /* Yes - then lets use it. */
        element = &(dictionary->elements[dictionaryIndex]);

        RWASSERT(_rpTeamLODNumOf(element->element) > 0);

        /* Increase reference count. */
        element->refCount ++;

        /* Is the player selected. */
        if(player->flags & TEAMPLAYERSELECTED)
        {
            /* Then also reduce the selection count. */
            player->team->selected[dictionaryIndex] ++;

            /* Need to flag that type of element as no longer optimized. */
            _rpTeamSetElementTypeOptimized( player->team,
                                            element->type,
                                            rpNATEAMLOCKMODE,
                                            FALSE );

            RWASSERT(    element->refCount
                      >= player->team->selected[dictionaryIndex] );
        }
    }

    player->elements[playerIndex] = dictionaryIndex;

    RWRETURN(player);
}

RpTeamDictionary *
_rpTeamDictionaryDestroy( RpTeamDictionary *dictionary )
{
    RwUInt32 i;

    RWFUNCTION(RWSTRING("_rpTeamDictionaryDestroy"));
    RWASSERT(NULL != dictionary);
    RWASSERT(NULL != dictionary->elements);

    /* Now lets delete the dictionary elements. */
    for( i = 0; i < dictionary->numOfElements; i++ )
    {
        RpTeamDictionaryElement *element = &(dictionary->elements[i]);

        _rpTeamDictionaryDestroyElement(element);

#if (defined(RWDEBUG))
        {
            RwUInt32 j;

            for( j = 0; j < rpTEAMELEMENTLODSUP; j++ )
            {
                RWASSERT(NULL == element->element[j]);
            }

            RWASSERT(rpNATEAMELEMENTTYPE == element->type);
        }
#endif /* (defined(RWDEBUG)) */
    }

    RWRETURN(dictionary);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*****************************************************************************
 _rpTeamDictionaryAtomicRights

 Team atomic rights callback. Setup the team dictionary atomic's pipeline,
 called if the atomic was streamout with the correct right plugin id.
 ie If the atomic was streamed out as part of a team dictionary.

 Inputs: object - RpAtomic object to setup.
         offset - Offset of team extension in atomic.
         size   - Size of team externsion data.
         extraData - Hint at the correct pipeline to attach.
 Outputs: RwBool - TRUE if pipeline was attached successfully.
 */
RwBool
_rpTeamDictionaryAtomicRights( void *object,
                               RwInt32 offset __RWUNUSED__,
                               RwInt32 size   __RWUNUSED__,
                               RwUInt32 extraData )
{
    RpAtomic *atomic;
    RxPipeline *pipeline;

    RWFUNCTION(RWSTRING("_rpTeamDictionaryAtomicRights"));
    RWASSERT(NULL != object);
    atomic = (RpAtomic *)object;
    RWASSERT(NULL != atomic);

#if (defined(SKY2_DRVMODEL_H))
    /* Get the pipeline from the pds system... */
    pipeline = RpPDSGetPipe(extraData);
#else /* (defined(SKY2_DRVMODEL_H)) */
    {
        RxPipeline **pipelines;

        /* Get the team pipelines. */
        pipelines = _rpTeamGetRenderPipelines();
        RWASSERT(NULL != pipelines);
        RWASSERT(NULL != pipelines[extraData]);

        pipeline = pipelines[extraData];
    }
#endif /* (defined(SKY2_DRVMODEL_H)) */
    RWASSERT(NULL != pipeline);
    atomic = RpAtomicSetPipeline(atomic, pipeline);
    RWASSERT(NULL != atomic);

    RWRETURN(TRUE);
}

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rpteam
 * \ref RpTeamDictionaryGetNumOfElements returns the number of elements the
 * team's dictionary can contain.
 *
 * \param dictionary Pointer to the dictionary to query.
 *
 * \return Returns total number elements the dictionary can contain.
 *
 * \see RpTeamCreate
 * \see RpTeamGetDictionary
 * \see RpTeamDictionarySetElement
 * \see RpTeamDictionaryGetElement
 * \see RpTeamDictionarySetLODElement
 * \see RpTeamDictionaryGetLODElement
 */
RwUInt32
RpTeamDictionaryGetNumOfElements( const RpTeamDictionary *dictionary )
{
    RWAPIFUNCTION(RWSTRING("RpTeamDictionaryGetNumOfElements"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != dictionary);

    RWRETURN(dictionary->numOfElements);
}

/**
 * \ingroup rpteam
 * \ref RpTeamDictionarySetElement is used to set an atomic element into the
 * dictionary. As no level of detail is specified it's assumed to be
 * using \ref RpTeamElementLOD default rpTEAMELEMENTLODFIRST.
 *
 * To permanently remove an element from the dictionary, set a NULL element
 * into the dictionary element.
 *
 * \param dictionary Pointer to the dictionary to added the element to.
 * \param atomic     Pointer to the atomic to add.
 * \param type       Element type.
 * \param index      Location in the dictionary to add the element.
 *
 * \return Returns a pointer to the dictionary if successful or NULL if there
 * is an error.
 *
 * \see RpTeamGetDictionary
 * \see RpTeamLock
 * \see RpTeamDictionaryGetNumOfElements
 * \see RpTeamDictionaryGetElement
 * \see RpTeamDictionarySetLODElement
 * \see RpTeamDictionaryGetLODElement
 * \see RpTeamPlayerSetElement
 */
RpTeamDictionary *
RpTeamDictionarySetElement( RpTeamDictionary *dictionary,
                            RpAtomic *atomic,
                            RpTeamElementType type,
                            RwUInt32 index )
{
    RpTeamDictionary *retDictionary;

    RWAPIFUNCTION(RWSTRING("RpTeamDictionarySetElement"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != dictionary);
    RWASSERT(dictionary->numOfElements > index);

    retDictionary =
        RpTeamDictionarySetLODElement( dictionary,
                                       atomic,
                                       type,
                                       (RwUInt32)rpTEAMELEMENTLODFIRST,
                                       index );

    RWRETURN(dictionary);
}

/**
 * \ingroup rpteam
 * \ref RpTeamDictionaryGetElement is used to return an atomic element from the
 * team dictionary. The element will remain in the dictionary.
 *
 * \param dictionary Pointer to the dictionary to query.
 * \param index      Location in the dictionary to get the element.
 *
 * \return Returns a pointer to the atomic if successful or NULL if there
 * is an error.
 *
 * \see RpTeamGetDictionary
 * \see RpTeamDictionaryGetNumOfElements
 * \see RpTeamDictionarySetElement
 * \see RpTeamDictionarySetLODElement
 * \see RpTeamDictionaryGetLODElement
 * \see RpTeamPlayerGetElement
 */
RpAtomic *
RpTeamDictionaryGetElement( const RpTeamDictionary *dictionary,
                            RwUInt32 index )
{
    RpAtomic *element;

    RWAPIFUNCTION(RWSTRING("RpTeamDictionaryGetElement"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != dictionary);
    RWASSERT(dictionary->numOfElements > index);

    element = RpTeamDictionaryGetLODElement( dictionary,
                                             (RwUInt32)rpTEAMELEMENTLODFIRST,
                                             index );

    RWRETURN(element);
}

/**
 * \ingroup rpteam
 * \ref RpTeamDictionarySetLODElement is used to set an atomic element into the
 * dictionary. The dictionary elements are broken up into a number of levels of
 * detail, defined by \ref rpTEAMELEMENTMAXLOD. The atomic element will be stored
 * in the correct level of detail index.
 *
 * We define two extra special levels of detail called \ref rpTEAMELEMENTLODSHADOW1
 * and \ref rpTEAMELEMENTLODSHADOW4, which should be used for shadow
 * rendering geometry. This geometry should be low detail, none textured, with no
 * pre-light colors. The geometry should also be generated from a single mesh.
 *
 * To permanently remove an element from the dictionary, set a NULL element
 * into the dictionary element with the correct level of detail.
 *
 * \param dictionary Pointer to the dictionary to add the element to.
 * \param atomic     Pointer to the atomic to add.
 * \param type       Element type.
 * \param lodIndex   Level of detail index.
 * \param index      Location in the dictionary to add the element.
 *
 * \return Returns a pointer to the dictionary if successful or NULL if there
 * is an error.
 *
 * \see RpTeamGetDictionary
 * \see RpTeamLock
 * \see RpTeamDictionaryGetNumOfElements
 * \see RpTeamDictionaryGetLODElement
 * \see RpTeamPlayerSetElement
 */
RpTeamDictionary *
RpTeamDictionarySetLODElement( RpTeamDictionary *dictionary,
                               RpAtomic *atomic,
                               RpTeamElementType type,
                               RwUInt32 lodIndex,
                               RwUInt32 index )
{
    RpTeamDictionaryElement *element;

    RWAPIFUNCTION(RWSTRING("RpTeamDictionarySetLODElement"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != dictionary);
    RWASSERT(dictionary->numOfElements > index);
    RWASSERT((RwUInt32)rpTEAMELEMENTLODSUP > lodIndex);

    /* Grab the element in question. */
    element = &(dictionary->elements[index]);

    /* Check reference. */
    RWASSERT(0 == element->refCount);

    /* Lets destroy any previous element. */
    _rpTeamDictionaryDestroyLODElement(element, lodIndex);
    RWASSERT(NULL == element->element[lodIndex]);
    RWASSERT( (rpNATEAMELEMENTTYPE == element->type) ||
              (element->type == type) );

    /* Are we given a new atomic? */
    if( NULL != atomic )
    {
        /* Yes - then clone the atomic. */
        RpAtomic *newAtomic;

#if(defined(RWDEBUG))
        {
            RpGeometry *geometry;
            RwUInt32 geomFlags;

            geometry = RpAtomicGetGeometry(atomic);
            RWASSERT(NULL != geometry);
            geomFlags = RpGeometryGetFlags(geometry);
            RWASSERT((geomFlags & rpGEOMETRYTRISTRIP) == rpGEOMETRYTRISTRIP);
        }
#endif /*(defined(RWDEBUG))*/

        /* Clone the given atomic. */
        newAtomic = RpAtomicClone(atomic);
        RWASSERT(NULL != newAtomic);

        /* We do not support user render callbacks */
        newAtomic->renderCallBack = NULL;

        /* Store the newAtomic and it's type. */
        element->element[lodIndex] = newAtomic;
        element->type = type;

         /* Special case construction. */
        switch(type)
        {
            case rpTEAMELEMENTSTATIC:
            {
                RwFrame  *frame;
                RwInt32  id;

                /* Grab the atomics frame ... */
                frame = RpAtomicGetFrame(atomic);
                RWASSERT(NULL != frame);

                /* and it's id. */
                id = RpHAnimFrameGetID(frame);
                frame = (RwFrame *)NULL;

                /* Create the new atomic it's own frame. */
                frame = RwFrameCreate();
                RWASSERT(NULL != frame);

                /* Set the frames id. */
                RpHAnimFrameSetID(frame, id);

                /* Set the newAtomics frame. */
                RpAtomicSetFrame(newAtomic, frame);

                /* Attach static pipeline. */
                newAtomic = _rpTeamStaticAttachCustomPipeline(newAtomic);
                break;
            }
            case rpTEAMELEMENTSKINNED:
            {
                /* Setup the atomic with some custom skinning. */
                newAtomic = _rpTeamSkinAttachCustomPipeline(newAtomic);
                break;
            }
            case rpTEAMELEMENTPATCH:
            {
                break;
            }
            default:
            {
                /* Shouldn't get here. */
                RWASSERT(FALSE);
                break;
            }
        }

        RWASSERT(NULL != newAtomic);

#if (defined (SKY2_DRVMODEL_H))

        newAtomic = _rpTeamSkyAttachMaterialPipeline(newAtomic, type);
        RWASSERT(NULL != newAtomic);

#else /* (defined (SKY2_DRVMODEL_H)) */

        /* Do we need to overload the pipeline? */
        if( (rpTEAMELEMENTLODSHADOW1 == lodIndex) ||
            (rpTEAMELEMENTLODSHADOW4 == lodIndex) )
        {
            newAtomic = _rpTeamShadowAttachPipeline(newAtomic, type, FALSE);
        }
        else if( _rpTeamMatFXEffectsEnabled(newAtomic) )
        {
            newAtomic = _rpTeamMatFXAttachPipeline(newAtomic, type);
        }

        RWASSERT(NULL != newAtomic);
        RWASSERT(NULL != dictionary->elements[index].element[lodIndex]);
        RWASSERT(dictionary->elements[index].type == type);

#endif /* (defined (SKY2_DRVMODEL_H)) */

    }

    RWRETURN(dictionary);
}

/**
 * \ingroup rpteam
 * \ref RpTeamDictionaryGetLODElement is used to return an element's level of
 * detail atomic from the team dictionary. The element will remain in the
 * dictionary.
 *
 * \param dictionary      Pointer to the dictionary to query.
 * \param lodIndex        Level of detail index.
 * \param dictionaryIndex Location in the dictionary to get the element.
 *
 * \return Returns a pointer to the atomic if successful or NULL if there
 * is an error.
 *
 * \see RpTeamGetDictionary
 * \see RpTeamDictionaryGetNumOfElements
 * \see RpTeamDictionarySetLODElement
 * \see RpTeamPlayerGetElement
 */
RpAtomic *
RpTeamDictionaryGetLODElement( const RpTeamDictionary *dictionary,
                               RwUInt32 lodIndex,
                               RwUInt32 dictionaryIndex )
{
    RpAtomic *element;

    RWAPIFUNCTION(RWSTRING("RpTeamDictionaryGetLODElement"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != dictionary);
    RWASSERT(dictionary->numOfElements > dictionaryIndex);
    RWASSERT((RwUInt32)rpTEAMELEMENTLODSUP > lodIndex);

    element = dictionary->elements[dictionaryIndex].element[lodIndex];

    RWRETURN(element);
}

/**
 * \ingroup rpteam
 * \ref RpTeamDictionaryCreate is called to create an \ref RpTeamDictionary
 * object. The team dictionary object contains the player elements used to
 * construct a team's players.
 *
 * \param numOfDictionaryElements Number of elements the dictionary can hold.
 *
 * \return Returns a pointer to the team dictionary if successful or NULL if
 * there is an error.
 *
 * \see RpTeamDictionaryDestroy
 * \see RpTeamDestroy
 * \see RpTeamCreate
 * \see RpTeamSetDictionary
 * \see RpTeamGetDictionary
 */
RpTeamDictionary *
RpTeamDictionaryCreate( RwUInt32 numOfDictionaryElements )
{
    RpTeamDictionary *dictionary = (RpTeamDictionary *)NULL;

    RwUInt32 sizeDictionary = 0;

    RWAPIFUNCTION(RWSTRING("RpTeamDictionaryCreate"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(0 < numOfDictionaryElements);

    /* We first calculate how much memory is needed. */
    sizeDictionary = sizeof(RpTeamDictionary) +
                     /*--- TeamDictionary ---*/
                     (sizeof(RpTeamDictionaryElement) * numOfDictionaryElements) +
                     /*--- TeamOptimise map --*/
                     (sizeof(RwUInt32) * numOfDictionaryElements);

    /* Malloc some space. */
    /* Determine size with slack for 16 byte alignment. */
    dictionary = (RpTeamDictionary *)RwMalloc(sizeDictionary,
        rwID_TEAMDICTIONARY | rwMEMHINTDUR_EVENT);

    /* Did we get some? */
    if(NULL != dictionary)
    {
        void *memory;

        /* Blank the memory. */
        memset(dictionary, 0, sizeDictionary);

        /* Pointer fixup ... */
        memory = (void *)(dictionary + 1);

        /* ... the dictionary elements. */
        dictionary->elements = (RpTeamDictionaryElement *)memory;

        /* ... the dictionary map. */
        memory = (void *)&(dictionary->elements[numOfDictionaryElements]);
        dictionary->map = (RwUInt32 *)memory;

        /* Setup the requested data. */
        dictionary->numOfElements = numOfDictionaryElements;
    }
    else
    {
        RWERROR((E_RW_NOMEM, sizeDictionary));
    }

    /* Return the dictionary. */
    RWRETURN(dictionary);
}

/**
 * \ingroup rpteam
 * \ref RpTeamDictionaryDestroy is called to destroy a team dictionary
 * object. All the elements and all the levels of detail in the team
 * dictionary will be destroyed.
 *
 * \param dictionary Pointer to the dictionary to destroy.
 *
 * \return Returns TRUE if successful or FALSE if there is an error.
 *
 * \see RpTeamDictionaryCreate
 * \see RpTeamGetDictionary
 * \see RpTeamDestroy
 */
RwBool
RpTeamDictionaryDestroy( RpTeamDictionary *dictionary )
{
    RWAPIFUNCTION(RWSTRING("RpTeamDictionaryDestroy"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != dictionary);
    RWASSERT(NULL != dictionary->elements);

    /* Clean up and destroy the dictionary. */
    _rpTeamDictionaryDestroy(dictionary);

    /* Free the memory. */
    RwFree(dictionary);

    RWRETURN(TRUE);
}

/**
 * \ingroup rpteam
 * \ref RpTeamDictionaryInstance is called to instance the dictionary
 * elements. All elements in the dictionary and all levels of detail
 * will be instanced. \ref RpAtomicInstance contains more details
 * about the instancing process.
 *
 * This function must be called between \ref RwCameraBeginUpdate and
 * \ref RwCameraEndUpdate as this function executes the rendering
 * pipelines to guarantee that all the needed data is generated.
 *
 * NOTE: See the platform specific documentation to see if this feature is
 * not supported on your target platform.
 *
 * \param dictionary Pointer to the dictionary to instance.
 *
 * \return Returns the dictionary if successful or NULL if there is an error.
 *
 * \see RpAtomicInstance
 * \see RpTeamGetDictionary
 */
RpTeamDictionary *
RpTeamDictionaryInstance( RpTeamDictionary *dictionary )
{
    RwUInt32 iElement;
    RxPipeline **instancePipelines;
    RxPipeline **renderPipelines;

    RwFrame *dummyFrame;

    RWAPIFUNCTION(RWSTRING("RpTeamDictionaryInstance"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != dictionary);
    RWASSERT(NULL != dictionary->elements);

    /* Lets create a dummy frame. */
    dummyFrame = RwFrameCreate();
    RWASSERT(NULL != dummyFrame);

    instancePipelines = _rpTeamInstancePipelinesCreate();
    if(NULL != instancePipelines)
    {
        renderPipelines = _rpTeamGetRenderPipelines();
        RWASSERT(NULL != renderPipelines);

        /* We go throught the dictionary, instancing all the elements. */
        for( iElement = 0;
             iElement < RpTeamDictionaryGetNumOfElements(dictionary);
             iElement++ )
        {
            RpTeamDictionaryElement *element;

            RwUInt32 iLod;

            /* Grab the dictionary element. */
            element = &(dictionary->elements[iElement]);
            RWASSERT(NULL != element);

            for( iLod = 0;
                 iLod < rpTEAMELEMENTLODSUP;
                 iLod++ )
            {
                RpAtomic *atomic;

                /* Grab the atomic. */
                atomic = element->element[iLod];

                if(NULL != atomic)
                {
                    RwFrame *cacheFrame;
                    RwBool instanceSuccess;

                    /* Cache the atomics frame. */
                    cacheFrame = RpAtomicGetFrame(atomic);
                    /* Attach the dummy frame. */
                    atomic = RpAtomicSetFrame(atomic, dummyFrame);
                    RWASSERT(NULL != atomic);

                    /* Need an atomic render callback. */
                    atomic->renderCallBack = TeamDictionaryRenderCallBack;

                    atomic = TeamDictionaryReplacePipelines(atomic, instancePipelines);
                    RWASSERT(NULL != atomic);

                    instanceSuccess = RpAtomicInstance(atomic);
                    RWASSERT(instanceSuccess);
                    RWASSERT((atomic->geometry->flags & rpGEOMETRYNATIVE) == rpGEOMETRYNATIVE);

                    atomic = TeamDictionaryReplacePipelines(atomic, renderPipelines);
                    RWASSERT(NULL != atomic);

                    /* Disable the callback. */
                    atomic->renderCallBack = NULL;

                    /* Reattach the old frame. */
                    atomic = RpAtomicSetFrame(atomic, cacheFrame);
                    RWASSERT(NULL != atomic);
                }
            }
        }

        /* Destroy the instancing pipelines. */
        _rpTeamInstancePipelinesDestroy();
    }
    else
    {
#if (defined (SKY2_DRVMODEL_H))
        /* We go throught the dictionary, instancing all the elements. */
        for( iElement = 0;
             iElement < RpTeamDictionaryGetNumOfElements(dictionary);
             iElement++ )
        {
            RpTeamDictionaryElement *element;

            RwUInt32 iLod;

            /* Grab the dictionary element. */
            element = &(dictionary->elements[iElement]);
            RWASSERT(NULL != element);

            for( iLod = 0;
                 iLod < rpTEAMELEMENTLODSUP;
                 iLod++ )
            {
                RpAtomic *atomic;

                /* Grab the atomic. */
                atomic = element->element[iLod];

                if(NULL != atomic)
                {
                    RwFrame *cacheFrame;
                    RwBool instanceSuccess;

                    /* Cache the atomics frame. */
                    cacheFrame = RpAtomicGetFrame(atomic);
                    /* Attach the dummy frame. */
                    atomic = RpAtomicSetFrame(atomic, dummyFrame);
                    RWASSERT(NULL != atomic);

                    /* Need an atomic render callback. */
                    atomic->renderCallBack = TeamDictionaryRenderCallBack;

                    instanceSuccess = RpAtomicInstance(atomic);
                    RWASSERT(instanceSuccess);
                    RWASSERT((atomic->geometry->flags & rpGEOMETRYNATIVE) == rpGEOMETRYNATIVE);

                    /* Disable the callback. */
                    atomic->renderCallBack = NULL;

                    /* Reattach the old frame. */
                    atomic = RpAtomicSetFrame(atomic, cacheFrame);
                    RWASSERT(NULL != atomic);
                }
            }
        }
#else /* (defined (SKY2_DRVMODEL_H)) */
        RwDebugSendMessage(rwDEBUGMESSAGE, "Team plugin", "Platform does not support pre-instancing.");
#endif /* (defined (SKY2_DRVMODEL_H)) */
    }

    /* Lets destroy the dummy frame. */
    RwFrameDestroy(dummyFrame);

    RWRETURN(dictionary);
}
