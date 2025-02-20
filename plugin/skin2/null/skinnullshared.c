/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpskin.h"

#include "skin.h"


/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

SkinGlobals _rpSkinGlobals =
{
    0,
    0,
    0,
    { (RwMatrix *)NULL, NULL },
    0,
    (RwFreeList *)NULL,
    { 0, 0 },
    {
        {                       /* SkinGlobalPlatform  platform  */
            (RxPipeline *)NULL, /*  rpSKINNULLPIPELINEGENERIC */
            (RxPipeline *)NULL, /*  rpSKINNULLPIPELINEMATFX   */
            (RxPipeline *)NULL, /*  rpSKINNULLPIPELINETOON    */
            (RxPipeline *)NULL  /*  rpSKINNULLPIPELINEMATFXTOON    */
        }
    },
    (SkinSplitData *) NULL
};

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

/****************************************************************************
 _rpSkinInitialize

 Initialise an atomic's matrix-blending skin data.

 Inputs :   RpGeometry * - Pointer to a skinned geometry.
 Outputs:   RpGeometry * - Pointer to the skinned geometry on success.
 */
RpGeometry *
_rpSkinInitialize(RpGeometry *geometry)
{
    RWFUNCTION(RWSTRING("_rpSkinInitialize"));
    RWASSERT(NULL != geometry);
    RWRETURN(geometry);
}


/****************************************************************************
 _rpSkinDeinitialize

 Platform specific deinitialize function for skinned geometry's.

 Inputs :  *geometry    - Pointer to the skinned geometry.
 Outputs:  RpGeometry * - The geometry which has been deinitialized.
 */
RpGeometry *
_rpSkinDeinitialize(RpGeometry *geometry)
{
    RWFUNCTION(RWSTRING("_rpSkinDeinitialize"));
    RWASSERT(NULL != geometry);
    RWRETURN(geometry);
}

/*===========================================================================*
 *--- Plugin Native Serialization Functions ---------------------------------*
 *===========================================================================*/

/****************************************************************************
 _rpSkinGeometryNativeSize
 */
RwInt32
_rpSkinGeometryNativeSize(const RpGeometry *geometry __RWUNUSEDRELEASE__)
{
    RWFUNCTION(RWSTRING("_rpSkinGeometryNativeSize"));
    RWASSERT(geometry);

    RWRETURN(0);
}

/****************************************************************************
 _rpSkinGeometryNativeWrite
 */
RwStream *
_rpSkinGeometryNativeWrite(RwStream *stream,
                           const RpGeometry *geometry __RWUNUSEDRELEASE__)
{
    RWFUNCTION(RWSTRING("_rpSkinGeometryNativeWrite"));
    RWASSERT(stream);
    RWASSERT(geometry);

    RWRETURN(stream);
}

/****************************************************************************
 _rpSkinGeometryNativeRead
 */
RwStream *
_rpSkinGeometryNativeRead(RwStream *stream,
                          RpGeometry *geometry __RWUNUSEDRELEASE__)
{
    RWFUNCTION(RWSTRING("_rpSkinGeometryNativeRead"));
    RWASSERT(stream);
    RWASSERT(geometry);

    RWRETURN(stream);
}
