/***********************************************************************
 *
 * Module:  
 *
 * Purpose: 
 *
 ***********************************************************************/

#if !defined( NODEOPENGLSUBMITNOLIGHT_H )
#define NODEOPENGLSUBMITNOLIGHT_H

/* =====================================================================
 *  Includes
 * ===================================================================== */
#include "p2core.h"


/* =====================================================================
 *  Defines
 * ===================================================================== */


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */


/* =====================================================================
 *  Extern variables
 * ===================================================================== */


/* =====================================================================
 *  Extern function prototypes
 * ===================================================================== */

/* RWPUBLIC */

#if defined( __cplusplus )
extern "C"
{
#endif /* defined( __cplusplus ) */

/* RWPUBLICEND */
   
extern void
_rwOpenGLApplyRwMatrix( RwMatrix *matrix );

/* RWPUBLIC */

extern RxNodeDefinition *
RxNodeDefinitionGetOpenGLImmInstance( void );

extern RxNodeDefinition *
RxNodeDefinitionGetOpenGLSubmitNoLight( void );

#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

/* RWPUBLICEND */

#endif /* !defined( NODEOPENGLSUBMITNOLIGHT_H ) */
