
/***************************************************************************
 *                                                                         *
 * Module  : basync.c                                                      *
 *                                                                         *
 * Purpose : Syncing up the world                                          *
 *                                                                         *
 **************************************************************************/

/****************************************************************************
 Includes
 */

#include <stdlib.h>

#include "batypes.h"
#include "balibtyp.h"
#include "badebug.h"
#include "batypehf.h"

#include "bamatrix.h"
#include "baframe.h"
#include "bavector.h"

/* */

#include "basync.h"

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

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */


/****************************************************************************
 _rwFrameSyncHierarchyRecurse

 Syncs a frames hierarchy and objects attached (recursively)

 On entry   : Root frame of the hierarchy to sync, dirty
 On exit    :
 */

static void
FrameSyncHierarchyRecurse(RwFrame *frame, RwInt32 flags)
{
    RWFUNCTION(RWSTRING("FrameSyncHierarchyRecurse"));

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe 코드 추가하지 않음. Local Function이며, 호출하는 곳에서 처리
	// 호출하는 함수 : FrameSyncHierarchy
	//@} DDonSS

    /* NULL is a valid; termination condition */
    while (frame)
    {
        RwInt32 accumflags = flags | rwObjectGetPrivateFlags(frame);

        RWASSERTISTYPE(frame, rwFRAME);

        if (accumflags & rwFRAMEPRIVATESUBTREESYNCLTM)
        {
            /* Work out the new local transformation matrix */
            RwMatrixMultiply(&frame->ltm,
                             &frame->modelling,
                             &((RwFrameGetParent(frame))->ltm));
        }

        /* If we got here, we always need to sync the objects */
        if (!rwLinkListEmpty(&frame->objectList))
        {
            RwLLLink    *current, *end;

            current = rwLinkListGetFirstLLLink(&frame->objectList);
            end = rwLinkListGetTerminator(&frame->objectList);
            while (current != end)
            {
                RwObjectHasFrame    *object;

                object = (RwObjectHasFrame *)
                    rwLLLinkGetData(current, RwObjectHasFrame, lFrame);
                rwObjectHasFrameSync(object);
                current = rwLLLinkGetNext(current);
            }
        }

        /* clear flag */
        rwObjectSetPrivateFlags(frame, rwObjectGetPrivateFlags(frame) &
                                        ~(rwFRAMEPRIVATESUBTREESYNCLTM |
                                          rwFRAMEPRIVATESUBTREESYNCOBJ));

        /* Depth first...
         * Child has dirty status including this frame,
         * sibling has dirty status of parent (parameter in)
         */
        FrameSyncHierarchyRecurse(frame->child, accumflags);

        /* tail recursion */
        frame = frame->next;
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rwFrameSyncHierarchyRecurse

 Syncs a frames hierarchy and objects attached (recursively)

 On entry   : Root frame of the hierarchy to sync, dirty
 On exit    :
 */

static void
FrameSyncHierarchyRecurseNoLTM(RwFrame *frame)
{
    RWFUNCTION(RWSTRING("FrameSyncHierarchyRecurseNoLTM"));

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe 코드 추가하지 않음. Local Function이며, 호출하는 곳에서 처리
	// 호출하는 함수 : FrameSyncHierarchy
	//@} DDonSS
	
    /* NULL is a valid; termination condition */
    while (frame)
    {
        RWASSERTISTYPE(frame, rwFRAME);
        RWASSERT(!(rwObjectGetPrivateFlags(frame) & rwFRAMEPRIVATESUBTREESYNCLTM));

        /* If we got here, we always need to sync the objects */
        if (!rwLinkListEmpty(&frame->objectList))
        {
            RwLLLink    *current, *end;

            current = rwLinkListGetFirstLLLink(&frame->objectList);
            end = rwLinkListGetTerminator(&frame->objectList);
            while (current != end)
            {
                RwObjectHasFrame    *object;

                object = (RwObjectHasFrame *)
                    rwLLLinkGetData(current, RwObjectHasFrame, lFrame);
                rwObjectHasFrameSync(object);
                current = rwLLLinkGetNext(current);
            }
        }

        /* clear flag */
        rwObjectSetPrivateFlags(frame, rwObjectGetPrivateFlags(frame) &
                                        ~(rwFRAMEPRIVATESUBTREESYNCOBJ));

        /* Depth first...
         * Child has dirty status including this frame,
         * sibling has dirty status of parent (parameter in)
         */
        FrameSyncHierarchyRecurseNoLTM(frame->child);

        /* tail recursion */
        frame = frame->next;
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rwFrameSyncHierachy

 Syncs a frames hierarchy and objects attached

 On entry   : Root frame of the hierarchy to sync
 On exit    :
 */

static void
FrameSyncHierarchy(RwFrame *frame)
{
    RwInt32 oldFlags;

    RWFUNCTION(RWSTRING("FrameSyncHierarchy"));
    RWASSERT(frame);
    RWASSERTISTYPE(frame, rwFRAME);
    RWASSERT(frame->root == frame);

    oldFlags = rwObjectGetPrivateFlags(frame);

    if (oldFlags & rwFRAMEPRIVATEHIERARCHYSYNCLTM)
    {
        if (oldFlags & rwFRAMEPRIVATESUBTREESYNCLTM)
        {
            /* Root of hierarchy has no parent matrix - different from rest of hierarchy */
            RwMatrixCopy(&frame->ltm, &frame->modelling);
        }

        /* Always handle objects in the world */
        if (!rwLinkListEmpty(&frame->objectList))
        {
            RwLLLink    *current, *end;

            current = rwLinkListGetFirstLLLink(&frame->objectList);
            end = rwLinkListGetTerminator(&frame->objectList);
            while (current != end)
            {
                RwObjectHasFrame    *object;

                object = (RwObjectHasFrame *)
					rwLLLinkGetData(current, RwObjectHasFrame, lFrame);
				rwObjectHasFrameSync(object);
				current = rwLLLinkGetNext(current);
            }
        }

        /* Do the children */
        FrameSyncHierarchyRecurse(frame->child, oldFlags & rwFRAMEPRIVATESUBTREESYNCLTM);
    }
    else
    {
        /* Always handle objects in the world */
        if (!rwLinkListEmpty(&frame->objectList))
        {
            RwLLLink    *current, *end;

            current = rwLinkListGetFirstLLLink(&frame->objectList);
            end = rwLinkListGetTerminator(&frame->objectList);
            while (current != end)
            {
                RwObjectHasFrame    *object;

                object = (RwObjectHasFrame *)
                    rwLLLinkGetData(current, RwObjectHasFrame, lFrame);
                rwObjectHasFrameSync(object);
                current = rwLLLinkGetNext(current);
            }
        }

        /* Do the children */
        FrameSyncHierarchyRecurseNoLTM(frame->child);
    }

    /* clear flag */
    rwObjectSetPrivateFlags(frame, oldFlags &
                                    ~(rwFRAMEPRIVATEHIERARCHYSYNCLTM |
                                      rwFRAMEPRIVATEHIERARCHYSYNCOBJ |
                                      rwFRAMEPRIVATESUBTREESYNCLTM |
                                      rwFRAMEPRIVATESUBTREESYNCOBJ));

    /* All done */
    RWRETURNVOID();
}

/****************************************************************************
 _rwFrameSyncDirty

 Syncs all dirty frame hierarchies in the system.

 On entry   : None
 On exit    : TRUE on success
 */

RwBool
_rwFrameSyncDirty(void)
{
    RwLLLink           *cur, *end, *begin, *prev;

    RWFUNCTION(RWSTRING("_rwFrameSyncDirty"));

	//@{ 20050513 DDonSS : Threadsafe
	// Frame Lock
	CS_FRAME_LOCK();
	//@} DDonSS

    cur = rwLinkListGetFirstLLLink(&RWSRCGLOBAL(dirtyFrameList));
    end = rwLinkListGetTerminator(&RWSRCGLOBAL(dirtyFrameList));
	
	//. 2006. 1. 17. Nonstopdj
	//. block infinite loop
	begin = cur;

    while (cur != end)
    {
        RwFrame *rootFrame =
            rwLLLinkGetData(cur, RwFrame, inDirtyListLink);

        /* Sync up the hierarchy */
        FrameSyncHierarchy(rootFrame);

		/* prev frame pointer save */
		prev = cur;

        /* Onto the next */
        cur = rwLLLinkGetNext(cur);

		/* force loop break when pointer same first pointer or next pointer */
		if(cur == begin || cur == prev)
			break;
    }

    /* We have dealt with them all -> everybody's happy ! */
    rwLinkListInitialize(&RWSRCGLOBAL(dirtyFrameList));

	//@{ 20050513 DDonSS : Threadsafe
	// Frame Unlock
	CS_FRAME_UNLOCK();
	//@} DDonSS

    /* All done */
    RWRETURN(TRUE);
}

/****************************************************************************
 FrameSyncHierarchyLTMRecurse

 Syncs a frames local transformation matrix hierarchy but not objects
 positions (recursively)

 On entry   : Root frame of the hierarchy to sync, dirty
 On exit    :
 */

static void
FrameSyncHierarchyLTMRecurse(RwFrame * frame, RwInt32 flags)
{
    RWFUNCTION(RWSTRING("FrameSyncHierarchyLTMRecurse"));

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe 코드 추가하지 않음. Local Function이며, 호출하는 곳에서 처리
	// 호출하는 함수 : _rwFrameSyncHierarchyLTM
	//@} DDonSS
	
    /* NULL is a valid; termination condition */
    while (frame)
    {
        RwInt32 accumflags = flags | rwObjectGetPrivateFlags(frame);

        RWASSERTISTYPE(frame, rwFRAME);

        if (accumflags & rwFRAMEPRIVATESUBTREESYNCLTM)
        {
            /* Work out the new local transformation matrix */
			/* 2006. 3. 10. Nonstopdj */
			/* validation check*/
			if(&frame->ltm == NULL				||
			   &frame->modelling == NULL		||
			   RwFrameGetParent(frame) == NULL  ||
			   &((RwFrameGetParent(frame))->ltm) == NULL)
			   RWRETURNVOID();

            RwMatrixMultiply(&frame->ltm,
                             &frame->modelling,
                             &((RwFrameGetParent(frame))->ltm));
            /* clear flag */
            rwObjectSetPrivateFlags(frame, rwObjectGetPrivateFlags(frame) &
                                            ~(rwFRAMEPRIVATESUBTREESYNCLTM));
        }

        /* Depth first */
        /* Child has dirty status including this frame,
         * sibling has dirty status of parent (parameter in)
         */
        FrameSyncHierarchyLTMRecurse(frame->child, accumflags);

        /* tail recursion */
        frame = frame->next;
    }

    RWRETURNVOID();
}

/****************************************************************************
 _rwFrameSyncHierachyLTM

 Syncs a frames hierarchy - but does not sync the objects attached to
 the frame (this will be performed later with the FrameSyncHierarchy
 function, presumably when the begin camera update is performed).

 On entry   : Root frame
 On exit    :
 */

void
_rwFrameSyncHierarchyLTM(RwFrame * frame)
{
    RwInt32 oldFlags;

    RWFUNCTION(RWSTRING("_rwFrameSyncHierarchyLTM"));
    RWASSERT(frame);
    RWASSERTISTYPE(frame, rwFRAME);
    RWASSERT(frame->root == frame);

	/* 2006. 3. 6. Nonstopdj */
	/* change lock positon.	 */

    oldFlags = rwObjectGetPrivateFlags(frame);

    if (oldFlags & rwFRAMEPRIVATESUBTREESYNCLTM)
    {
        /* Root of hierarchy has no parent matrix - different from rest of hierarchy */
        RwMatrixCopy(&frame->ltm, &frame->modelling);
    }
	
    /* Do the children */
    FrameSyncHierarchyLTMRecurse(frame->child, oldFlags);

    /* clear flag */
    rwObjectSetPrivateFlags(frame, oldFlags &
                                    ~(rwFRAMEPRIVATEHIERARCHYSYNCLTM |
                                      rwFRAMEPRIVATESUBTREESYNCLTM));

    /* All done */
    RWRETURNVOID();
}

#if defined (__MWERKS__)
#if (defined(RWVERBOSE))
#pragma message (__FILE__ "/" _SKY_EXPAND(__LINE__) ": __MWERKS__ == " _SKY_EXPAND(__MWERKS__))
#endif /* (defined (__MWERKS__)) */
#if (__option (global_optimizer))
#pragma always_inline on
#endif /* (__option (global_optimizer)) */
#endif /*  defined (__MWERKS__) */
