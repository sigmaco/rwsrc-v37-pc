#ifndef TEAMSHADOWD3D8_H
#define TEAMSHADOWD3D8_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpteam.h"

extern void
_rwTeamD3D8SkinnedShadowsRenderCallback(RwResEntry *repEntry,
                                        void *object,
                                        RwUInt8 type,
                                        RwUInt32 flags);

extern void
_rwTeamD3D8StaticShadowsRenderCallback( RwResEntry *repEntry,
                                        void *object,
                                        RwUInt8 type,
                                        RwUInt32 flags);


#endif /* TEAMSHADOWD3D8_H */
