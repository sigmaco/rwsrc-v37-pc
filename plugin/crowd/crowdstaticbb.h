/*
 *  crowdstaticbb.h
 *
 *  Static billboard geometry system for RpCrowd. Used by various platforms.
 *  Other platforms (GCN and PS2) generate billboards from the base quads on
 *  the fly.
 */

#ifndef CROWDSTATICBB_H
#define CROWDSTATICBB_H

#include "rwcore.h"
#include "rpplugin.h"
#include "rpdbgerr.h"
#include "rpcrowd.h"

/******************************************************************************
 *  Types
 */
struct _rpCrowdStaticBB
{
    RpAtomic           *atomic;
    RpCrowdTexture   ***splitTextures;  /* Array of Arrays of texture pointers. */
    RpMaterial        **materials;      /* Crowd Materials */
    RwUInt32            numMatPerSeq;
};
typedef struct _rpCrowdStaticBB rpCrowdStaticBB;


/******************************************************************************
 *  Functions
 */
extern RpCrowd *_rpCrowdStaticBBCreate(RpCrowd *crowd,
                                     rpCrowdStaticBB *staticBB,
                                     RwUInt32 numMatPerSeq);

extern RwBool   _rpCrowdStaticBBDestroy(RpCrowd *crowd,
                                      rpCrowdStaticBB *staticBB);

extern RpCrowd *_rpCrowdStaticBBSetTextures(RpCrowd *crowd, 
                                          rpCrowdStaticBB *staticBB);

/**/
extern RpMaterial **
_rpCrowdStaticBBCreateCrowdMaterials(RwUInt32 numMaterials);

extern RpGeometry *
_rpCrowdStaticBBCreateCrowdGeometry(RpCrowd *crowd);

extern RwBool
_rpCrowdStaticBBAssignCrowdMaterials(RpMaterial   **materials, 
                                     RwUInt32       numMaterials,
                                     RpGeometry    *geometry);


#endif /* CROWDSTATICBB_H */

