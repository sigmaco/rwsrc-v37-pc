/*
 * Generic type handling
 * Generic types enable limitted polymorphism capabilities in RenderWare
 *
 * Copyright (c) 1998 Criterion Software Ltd.
 */


/****************************************************************************
 Includes
 */

#include <stdlib.h>

#include "batypes.h"
#include "balibtyp.h"
#include "badebug.h"

#include "batypehf.h"

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

/****************************************************************************
 Local (static) Globals
 */

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   Generic types

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

#if (defined(DOXYGEN))

/**
 * \ingroup rwobject
 * \ref RwObjectGetType is used to retrieve the type ID of the specified
 * object. The ID can be used to identify the type of the object.
 *
 * \param object  Pointer to the object.
 *
 * \return Returns aa \ref RwUInt32 value equal to the type ID. Supported
 * types are:
 *
 *     \li rwCAMERA        Object has type \ref RwCamera
 *     \li rwFRAME         Object has type \ref RwFrame
 *     \li rwTEXDICTIONARY Object has type \ref RwTexDictionary
 *     \li rpATOMIC        Object has type \ref RpAtomic
 *     \li rpCLUMP         Object has type \ref RpClump
 *     \li rpGEOMETRY      Object has type \ref RpGeometry
 *     \li rpLIGHT         Object has type \ref RpLight
 *     \li rpWORLD         Object has type \ref RpWorld
 */
RwUInt32
RwObjectGetType(RwObject * object)
{
    RWAPIFUNCTION(RWSTRING("RwObjectGetType"));

    RWRETURN(0);
}

#endif /* (defined(DOXYGEN)) */

 /****************************************************************************
 RwHasFrameObjectSetFrame

 On entry   : Object
 On exit    :
 */

void
_rwObjectHasFrameSetFrame(void *object, RwFrame * frame)
{
    RwObjectHasFrame   *ohf = (RwObjectHasFrame *) object;

    RWFUNCTION(RWSTRING("_rwObjectHasFrameSetFrame"));

	//@{ 20050513 DDonSS : Threadsafe
	// Frame Lock
	CS_FRAME_LOCK();
	//@} DDonSS

    if (rwObjectGetParent(ohf))
    {
        rwLinkListRemoveLLLink(&ohf->lFrame);
    }

    /* Set the pointer */
    rwObjectSetParent(object, frame);

    /* Add it to the frames list of objects */
    if (frame)
    {
        rwLinkListAddLLLink(&frame->objectList, &ohf->lFrame);

        /* Force the objects using this frame to be updated */
        RwFrameUpdateObjects(frame);
    }

	//@{ 20050513 DDonSS : Threadsafe
	// Frame Unlock
	CS_FRAME_UNLOCK();
	//@} DDonSS

    /* All done */
    RWRETURNVOID();
}

/****************************************************************************
 _rwObjectHasFrameReleaseFrame

 On entry   : Object
 On exit    :
 */

void
_rwObjectHasFrameReleaseFrame(void *object)
{
    RwObjectHasFrame   *ohf = (RwObjectHasFrame *) object;

    RWFUNCTION(RWSTRING("_rwObjectHasFrameReleaseFrame"));

	//. validation check.
	if(!ohf)
		RWRETURNVOID();

	//@{ 20050513 DDonSS : Threadsafe
	// Frame Lock
	CS_FRAME_LOCK();
	//@} DDonSS

    if (rwObjectGetParent(ohf) && (&ohf->lFrame))
    {
		//rwLinkListRemoveLLLink(&ohf->lFrame);
		//
		//CS_FRAME_UNLOCK();
		//RWRETURNVOID();

		//. 2006. 1. 23. Nonstopdj
		//. if ohf->lFrame->next->prev & ohf->lFrame->prev->next has removed then force RWRETURNVOID.
		if(IsBadReadPtr(ohf->lFrame.prev, sizeof(RwLLLink)) &&
			IsBadReadPtr(ohf->lFrame.next, sizeof(RwLLLink)))
		{
			CS_FRAME_UNLOCK();
			RWRETURNVOID();
	
		}
		else
			rwLinkListRemoveLLLink(&ohf->lFrame);
    }

	//@{ 20050513 DDonSS : Threadsafe
	// Frame Unlock
	CS_FRAME_UNLOCK();
	//@} DDonSS

    RWRETURNVOID();
}


/**
 * \ingroup rwobject
 * \page rwobjectoverview RwObject Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h
 * \li \b Libraries: rwcore.lib
 *
 * \subsection objectoverview Overview
 *
 * A base object from which some RenderWare objects are derived.
 *
 * RwObject-derived objects are:
 * - \ref RwCamera
 * - \ref RwFrame
 * - \ref RwTexDictionary
 * - \ref RpAtomic
 * - \ref RpClump
 * - \ref RpGeometry
 * - \ref RpLight
 * - \ref RpWorld
 *
 * It is therefore possible to write functions that
 * simulate C++'s polymorphism features
 * by querying the RwObject ID of a passed RwObject
 * and acting on the result.
 */
