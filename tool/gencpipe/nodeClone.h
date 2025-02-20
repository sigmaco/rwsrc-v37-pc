/**
 * 
 * nodeClone
 * 
 * Data structures for cloning RxPacket's during RxPipeline execution 
 * 
 * Copyright (c) Criterion Software Limited
 */

/****************************************************************************
 *                                                                          *
 * module : nodeClone.h                                                     *
 *                                                                          *
 * purpose: see nodeClone.c                                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef NODECLONE_H
#define NODECLONE_H

/****************************************************************************
 Global types
 */


/****************************************************************************
 Global ptototypes
 */


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* Utility functions for cloning packets - into the core eventually */
extern RxPacketCache *RxPacketCacheCreate(RxPacket               *packet,
                                          RxPipelineNodeInstance *node,
                                          RxHeap                 *heap);
extern RxPacket      *RxPacketCacheClone( RxPacketCache          *cache,
                                          RxPipelineNodeInstance *node,
                                          RwBool                  lastClone);
extern void           RxPacketCacheDestroy(RxPacketCache         *cache,
                                           RxHeap                *heap);

#ifdef    __cplusplus
}
#endif /* __cplusplus */

#endif /* NODECLONE_H */


