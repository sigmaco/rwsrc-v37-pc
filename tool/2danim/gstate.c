
/*
 *
 * Functionality for 2D rendering
 *
 * Copyright (c) 1998 Criterion Software Ltd.
 */

/****************************************************************************
 *                                                                          *
 *  Module  :   gstate.c                                                    *
 *                                                                          *
 *  Purpose :   graphics state                                              *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
 */

#include "rwcore.h"
#include <rpdbgerr.h>
#include <rpcriter.h>

#include "anim.h"
#include "button.h"
#include "maestro.h"
#include "strlabel.h"
#include "message.h"
#include "keyframe.h"
#include "props.h"
#include "cel.h"
#include "gstate.h"

#include "rt2d.h"


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
Rt2dAnimGlobalVars      Rt2dAnimGlobals;
RwBool                  _rt2dAnimOpenCalled = FALSE;

static RwInt32 _rt2dAnimFListBlockSize = 20;
static RwInt32 _rt2dAnimFListPreallocBlocks = 1;

/****************************************************************************
 Local (static) Globals
 */

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   Functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */


/****************************************************************************/

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimOpen
 * is used to perform various initializations that are required
 * before the 2D animation toolkit API can be used.
 * \see Rt2dAnimClose
 * \see Rt2dAnimSetFreeListCreateParams
 */
void
Rt2dAnimOpen(void)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimOpen"));

    RwFreeListCreateAndPreallocateSpace(sizeof(Rt2dAnim), _rt2dAnimFListBlockSize, sizeof(RwUInt32),
            _rt2dAnimFListPreallocBlocks, &Rt2dAnimGlobals.animFreeList, rwID_2DANIM | rwMEMHINTDUR_GLOBAL);

    RwFreeListCreateAndPreallocateSpace(sizeof(Rt2dButton), _rt2dAnimFListBlockSize, sizeof(RwUInt32),
            _rt2dAnimFListPreallocBlocks, &Rt2dAnimGlobals.buttonFreeList, rwID_2DANIM | rwMEMHINTDUR_GLOBAL);

    RwFreeListCreateAndPreallocateSpace(sizeof(Rt2dMaestro), _rt2dAnimFListBlockSize, sizeof(RwUInt32),
            _rt2dAnimFListPreallocBlocks, &Rt2dAnimGlobals.maestroFreeList, rwID_2DMAESTRO | rwMEMHINTDUR_GLOBAL);

    RwFreeListCreateAndPreallocateSpace(sizeof(Rt2dStringLabel), _rt2dAnimFListBlockSize, sizeof(RwUInt32),
            _rt2dAnimFListPreallocBlocks, &Rt2dAnimGlobals.stringLabelFreeList, rwID_2DANIM | rwMEMHINTDUR_GLOBAL);

    RwFreeListCreateAndPreallocateSpace(sizeof(Rt2dMessage), _rt2dAnimFListBlockSize, sizeof(RwUInt32),
            _rt2dAnimFListPreallocBlocks, &Rt2dAnimGlobals.messageFreeList, rwID_2DANIM | rwMEMHINTDUR_GLOBAL);

    RwFreeListCreateAndPreallocateSpace(sizeof(Rt2dMessageQueue), _rt2dAnimFListBlockSize, sizeof(RwUInt32),
            _rt2dAnimFListPreallocBlocks, &Rt2dAnimGlobals.messageQueueFreeList, rwID_2DANIM | rwMEMHINTDUR_GLOBAL);

    RwFreeListCreateAndPreallocateSpace(sizeof(Rt2dAnimObjectUpdate), _rt2dAnimFListBlockSize, sizeof(RwUInt32),
            _rt2dAnimFListPreallocBlocks, &Rt2dAnimGlobals.animObjectUpdateFreeList, rwID_2DANIM | rwMEMHINTDUR_GLOBAL);

    RwFreeListCreateAndPreallocateSpace(sizeof(Rt2dKeyFrameList), _rt2dAnimFListBlockSize, sizeof(RwUInt32),
            _rt2dAnimFListPreallocBlocks, &Rt2dAnimGlobals.keyFrameListFreeList, rwID_2DKEYFRAME | rwMEMHINTDUR_GLOBAL);

    RwFreeListCreateAndPreallocateSpace(sizeof(Rt2dAnimProps), _rt2dAnimFListBlockSize, sizeof(RwUInt32),
            _rt2dAnimFListPreallocBlocks, &Rt2dAnimGlobals.animPropsFreeList, rwID_2DANIM | rwMEMHINTDUR_GLOBAL);

    RwFreeListCreateAndPreallocateSpace(sizeof(Rt2dCelList), _rt2dAnimFListBlockSize, sizeof(RwUInt32),
            _rt2dAnimFListPreallocBlocks, &Rt2dAnimGlobals.celListFreeList, rwID_2DANIM | rwMEMHINTDUR_GLOBAL);

    RwFreeListCreateAndPreallocateSpace(sizeof(Rt2dCel), _rt2dAnimFListBlockSize, sizeof(RwUInt32),
            _rt2dAnimFListPreallocBlocks, &Rt2dAnimGlobals.celFreeList, rwID_2DANIM | rwMEMHINTDUR_GLOBAL);

    _rt2dAnimOpenCalled = TRUE;

    RWRETURNVOID();
}

/****************************************************************************/

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimClose
 * is used to perform various clean-up operations that are
 * required to close- down the 2D animation tools API. This function
 * complements \ref Rt2dAnimOpen and should be used before closing down an
 * application.
 * \return None
 *
 * \see Rt2dAnimOpen
 */
void
Rt2dAnimClose(void)
{
    RWAPIFUNCTION(RWSTRING("Rt2dAnimClose"));

    RwFreeListDestroy(&Rt2dAnimGlobals.animFreeList);
    RwFreeListDestroy(&Rt2dAnimGlobals.buttonFreeList);
    RwFreeListDestroy(&Rt2dAnimGlobals.maestroFreeList);
    RwFreeListDestroy(&Rt2dAnimGlobals.stringLabelFreeList);
    RwFreeListDestroy(&Rt2dAnimGlobals.messageFreeList);
    RwFreeListDestroy(&Rt2dAnimGlobals.messageQueueFreeList);
    RwFreeListDestroy(&Rt2dAnimGlobals.animObjectUpdateFreeList);
    RwFreeListDestroy(&Rt2dAnimGlobals.keyFrameListFreeList);
    RwFreeListDestroy(&Rt2dAnimGlobals.animPropsFreeList);
    RwFreeListDestroy(&Rt2dAnimGlobals.celListFreeList);
    RwFreeListDestroy(&Rt2dAnimGlobals.celFreeList);

    RWRETURNVOID();
}

/**
 * \ingroup rt2danimsub
 * \ref Rt2dAnimSetFreeListCreateParams allows the developer to specify
 * how many 2d animation objects to preallocate space for. This applies to all
 * the 2d animations objects.
 * Call before \ref RwEngineInit.
 *
 * \param blockSize  number of entries per freelist block.
 * \param numBlocksToPrealloc  number of blocks to allocate on
 * \ref RwFreeListCreateAndPreallocateSpace.
 *
 * \see RwFreeList
 */
void
Rt2dAnimSetFreeListCreateParams( RwInt32 blockSize, RwInt32 numBlocksToPrealloc )
{
    /*
     * Can not use debugging macros since the debugger is not initialized before
     * RwEngineInit.
     */
    /* Do NOT comment out RWAPIFUNCTION as gnumake verify will not function */
#if 0
    RWAPIFUNCTION(RWSTRING("Rt2dAnimSetFreeListCreateParams"));
#endif
    _rt2dAnimFListBlockSize = blockSize;
    _rt2dAnimFListPreallocBlocks = numBlocksToPrealloc;
#if 0
    RWRETURNVOID();
#endif
}

