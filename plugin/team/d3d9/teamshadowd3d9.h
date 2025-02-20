#ifndef TEAMSHADOWD3D9_H
#define TEAMSHADOWD3D9_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpteam.h"

extern void
_rwTeamD3D9SkinnedShadowsRenderCallback(RwResEntry *repEntry,
                                        void *object,
                                        RwUInt8 type,
                                        RwUInt32 flags);

extern void
_rwTeamD3D9StaticShadowsRenderCallback( RwResEntry *repEntry,
                                        void *object,
                                        RwUInt8 type,
                                        RwUInt32 flags);


#endif /* TEAMSHADOWD3D9_H */
