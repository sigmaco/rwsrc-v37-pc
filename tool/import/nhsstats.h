/***************************************************************************
 *                                                                         *
 * Module  : nhsstats.h                                                      *
 *                                                                         *
 * Purpose : World handling functions.                                     *
 *                                                                         *
 **************************************************************************/

#ifndef RTNHSSTATS_H
#define RTNHSSTATS_H

/****************************************************************************
 Includes
 */

#include "rwcore.h"

#include "nhsworld.h"
#include "rtimport.h"

/****************************************************************************
 Defines
 */
/* Clip codes used when forming bsp sectors */
#define rwCLIPVERTEXLEFT    (1)
#define rwCLIPVERTEXRIGHT   (2)
#define BuildSectorSetWorldClipCodes(buildSector, type, value, maxLeftValue, maxRightValue)   \
MACRO_START                                                             \
{                                                                       \
    RwInt32             nI;                                             \
                                                                        \
    for (nI = 0; nI < buildSector->numVertices; nI++)                   \
    {                                                                   \
        RtWorldImportVertex *const vpVert = &buildSector->vertices[nI]; \
        const RwReal test = GETCOORD(vpVert->OC, type) - value;         \
        const RwReal leftRelativeValue = maxLeftValue - value;             \
        const RwReal rightRelativeValue = maxRightValue - value;           \
                                                                        \
        vpVert->state.clipFlags[0] = 0;                                    \
        vpVert->state.clipFlags[1] = 0;                                    \
                                                                        \
        /*                                                              \
         *  NOTE: if the vertex is within the plane it                  \
         *  will be classifed as being on both sides of the plane,      \
         *  this is intentional.                                        \
         */                                                             \
        if (test <= 0.001f) vpVert->state.clipFlags[0] |= rwCLIPVERTEXLEFT;                \
        if (test <= leftRelativeValue) vpVert->state.clipFlags[1] |= rwCLIPVERTEXLEFT;   \
                                                                                        \
        if (test >= -0.001f) vpVert->state.clipFlags[0] |= rwCLIPVERTEXRIGHT;              \
        if (test >= rightRelativeValue) vpVert->state.clipFlags[1] |= rwCLIPVERTEXRIGHT; \
    }                                                                                   \
}                                                                                       \
MACRO_STOP
/****************************************************************************
 Global types
 */

/****************************************************************************
 Function prototypes
 */

#ifdef    __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#endif /* RTNHSSTATS_H */
