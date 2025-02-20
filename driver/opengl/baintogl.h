/***************************************************************************
 *                                                                         *
 * Module  : baintogl.h                                                    *
 *                                                                         *
 * Purpose : OpenGL device handling                                        *
 *                                                                         *
 **************************************************************************/

#if !defined(RWINTOGL_H)
#define RWINTOGL_H

/****************************************************************************
 Includes
 */
#include "badevice.h"
#include "barwtyp.h"

/****************************************************************************
 Defines
 */
/* Z buffer near and far values */
#define rwOGLNEARSCREENZ ((RwReal)(0.0))
#define rwOGLFARSCREENZ  ((RwReal)(-65535.0))


/****************************************************************************
 Global Types
 */

#if defined(__cplusplus)
extern "C"
{
#endif /* defined(__cplusplus) */

/****************************************************************************
 Globals
 */


/****************************************************************************
 Function prototypes
 */

/* Opening/Closing */
extern void
_rwOpenGLClose( void );

extern RwBool
_rwOpenGLOpen( void );

#if defined(__cplusplus)
}
#endif /* defined(__cplusplus) */

#endif /* !defined(RWINTOGL_H) */
