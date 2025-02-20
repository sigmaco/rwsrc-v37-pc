
/****************************************************************************
 *                                                                          *
 * module : p2stdclsw.c                                                     *
 *                                                                          *
 * purpose: Initialise the values of the global world cluster definitions   *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 includes
 */

#include <rwcore.h>

#define _DEFAULTSTRIDE     ((RwUInt32)0)
#define _DEFAULTATTRIBUTES ((RwUInt32)0)
#define _ATTRIBUTESET      ((const char *)NULL)

/****************************************************************************
 Globals (across program)
 */

static RwChar _Lights_csl[] = RWSTRING("Lights.csl");

RxClusterDefinition RxClLights =
{ /* Uses the RxLight type */
    _Lights_csl,
    _DEFAULTSTRIDE,
    _DEFAULTATTRIBUTES,
    _ATTRIBUTESET
};

