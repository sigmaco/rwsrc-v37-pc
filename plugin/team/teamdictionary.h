#ifndef TEAMDICTIONARY_H
#define TEAMDICTIONARY_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpteam.h"

#include "teamlod.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/
typedef struct RpTeamDictionaryElement RpTeamDictionaryElement;
struct RpTeamDictionaryElement
{
    TeamLODAtomic      element;
    RpTeamElementType  type;
    RwUInt32           refCount;
};

/*===========================================================================*
 *--- Public Types ----------------------------------------------------------*
 *===========================================================================*/
struct RpTeamDictionary
{
    RwUInt32                 numOfElements;
    RpTeamDictionaryElement *elements;
    RwUInt32                *map;
};

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/
extern RwBool
_rpTeamDictionaryAtomicRights( void *object,
                               RwInt32 offset,
                               RwInt32 size,
                               RwUInt32 extraData );

extern RpTeamDictionaryElement *
_rpTeamDictionaryDestroyElement( RpTeamDictionaryElement *element );

extern RpTeamDictionaryElement *
_rpTeamDictionaryDestroyLODElement( RpTeamDictionaryElement *element,
                                    RwUInt32 lodIndex );

extern RpTeamPlayer *
_rpTeamDictionarySetElement( RpTeamPlayer *player,
                             RwUInt32 playerIndex,
                             RwUInt32 dictionaryIndex );

extern RpTeamDictionary *
_rpTeamDictionaryDestroy( RpTeamDictionary *dictionary );

#endif /* TEAMDICTIONARY_H */

