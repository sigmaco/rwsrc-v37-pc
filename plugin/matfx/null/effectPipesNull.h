/*
 *  effectPipesNull.h 
 *
 *  Note that this is included from nullgcn, nullsky, nullxbox.
 */

#ifndef EFFECTPIPESNULL_H
#define EFFECTPIPESNULL_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include "rwcore.h"
#include "rpworld.h"

/*===========================================================================*
 *--- Types -----------------------------------------------------------------*
 *===========================================================================*/
struct MatFXDualData
{
    RwTexture          *texture;
    RwBlendFunction     srcBlendMode;
    RwBlendFunction     dstBlendMode;
    /*--- device specific ---*/
};

struct MatFXBumpMapData
{
    RwFrame   *frame;
    RwTexture *texture;
    RwTexture *bumpTexture;
    RwReal    coef;
    RwReal    invBumpWidth;
};

#endif /* EFFECTPIPESNULL_H */
