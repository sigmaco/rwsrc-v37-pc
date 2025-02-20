#ifndef TEAMSTREAM_H
#define TEAMSTREAM_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpteam.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern RwUInt32
_rpTeamStreamGetSize(const RpTeam *team);

extern RpTeam *
_rpTeamStreamRead(RwStream *stream, RpTeamDictionary *dictionary);

extern RwStream *
_rpTeamStreamWrite(const RpTeam *team, RwStream *stream);

extern RwUInt32
_rpTeamDictionaryStreamGetSize(const RpTeamDictionary *dictionary);

extern RpTeamDictionary *
_rpTeamDictionaryStreamRead(RwStream *stream);

extern RwStream *
_rpTeamDictionaryStreamWrite(const RpTeamDictionary *dictionary, RwStream *stream);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* TEAMSTREAM_H */

