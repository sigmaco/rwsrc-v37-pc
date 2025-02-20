
#ifndef BSPLIT_H
#define BSPLIT_H

/****************************************************************************
 Includes
 */
#include <rwcore.h>
#include <rpworld.h>


#include "skin.h"

/****************************************************************************
 Defines
 */

/****************************************************************************
 Global Types
 */

/****************************************************************************
 Global variables (across program)
 */

/****************************************************************************
 Function prototypes
 */
#ifdef __cplusplus
extern "C"
{
#endif /* __cpluscplus */

extern RwStream *
_rpSkinSplitDataStreamWrite( RwStream * stream, const RpSkin *skin );

extern RwStream *
_rpSkinSplitDataStreamRead( RwStream * stream, RpSkin *skin );

extern RwInt32
_rpSkinSplitDataStreamGetSize( const RpSkin *skin );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BSPLIT_H */

