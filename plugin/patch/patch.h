#ifndef PATCH_H
#define PATCH_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rppatch.h"

#include "patchlod.h"

#if (defined(SKY2_DRVMODEL_H))
#include "sky2/patchsky.h"
#elif (defined(D3D8_DRVMODEL_H))
#include "d3d8/patchd3d8.h"
#elif (defined(D3D9_DRVMODEL_H))
#include "d3d9/patchd3d9.h"
#elif (defined(OPENGL_DRVMODEL_H))
#include "opengl/patchopengl.h"
#elif (defined(SOFTRAS_DRVMODEL_H))
#include "softras/patchsoftras.h"
#elif (defined(XBOX_DRVMODEL_H))
#include "xbox/patchxbox.h"
#elif (defined(GCN_DRVMODEL_H))
#include "gcn/patchgcn.h"
#elif (defined(NULLGCN_DRVMODEL_H))
#include "gcn/patchgcn.h"
#elif (defined(NULL_DRVMODEL_H)    || \
       defined(NULLSKY_DRVMODEL_H) || \
       defined(NULLXBOX_DRVMODEL_H))
#include "null/patchnull.h"
#endif

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/
typedef struct PatchGlobals PatchGlobals;
struct PatchGlobals
{
    RwInt32        engineOffset;
    RwInt32        atomicOffset;
    RwInt32        geometryOffset;
    RwModuleInfo   module;
    RwFreeList    *atomicFreeList;
    RwFreeList    *geometryFreeList;
    PatchPlatform  platform;
};

enum PatchPipeline
{
    rpNAPATCHPIPELINE      = 0,
    rpPATCHPIPELINEGENERIC = 0x01,
    rpPATCHPIPELINESKINNED = 0x02,
    rpPATCHPIPELINEMATFX   = 0x04,
    rpPATCHPIPELINEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum PatchPipeline PatchPipeline;

typedef struct PatchEngineData PatchEngineData;
struct PatchEngineData
{
    PatchLod lod;
};

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/
extern PatchGlobals _rpPatchGlobals;

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/
#define PATCHEXTENSIONGETDATA(type, object, offset)                     \
    ((type *)(((RwUInt8 *)object) + _rpPatchGlobals.offset))

#define PATCHEXTENSIONGETCONSTDATA(type, object, offset)                \
    ((const type *)(((const RwUInt8 *)object) + _rpPatchGlobals.offset))

#define PATCHENGINEGETDATA(engineObject)                                \
    PATCHEXTENSIONGETDATA( PatchEngineData,                             \
                           engineObject,                                \
                           engineOffset )

#define PATCHENGINEGETCONSTDATA(engineObject)                           \
    PATCHEXTENSIONGETCONSTDATA( PatchEngineData,                        \
                                engineObject,                           \
                                engineOffset )

#define PATCHATOMICGETDATA(atomicObject)                                \
    PATCHEXTENSIONGETDATA( PatchAtomicData,                             \
                           atomicObject,                                \
                           atomicOffset )

#define PATCHATOMICGETCONSTDATA(atomicObject)                           \
    PATCHEXTENSIONGETCONSTDATA( PatchAtomicData,                        \
                                atomicObject,                           \
                                atomicOffset )

#define PATCHGEOMETRYGETDATA(geometryObject)                            \
    PATCHEXTENSIONGETDATA( PatchGeometryData,                           \
                           geometryObject,                              \
                           geometryOffset )

#define PATCHGEOMETRYGETCONSTDATA(geometryObject)                       \
    PATCHEXTENSIONGETCONSTDATA( PatchGeometryData,                      \
                                geometryObject,                         \
                                geometryOffset )

/* _res is the number of verts along the edge. */
#define PATCHQUADNUMVERT(_res)                                          \
   ((_res) * (_res))

#define PATCHQUADNUMINDEX(_res)                                         \
    ((2 * (_res)) * ((_res) - 1))

#define PATCHQUADNUMTRI(_res)                                           \
    (2 * (_res - 1) * (_res - 1))

#define PATCHTRINUMVERT(_res)                                           \
    (((_res) * ((_res) + 1)) / 2)

#define PATCHTRINUMINDEX(_res)                                          \
    (((_res) - 1) * ((_res) + 1))

#define PATCHTRINUMTRI(_res)                                            \
    ((_res - 1) * (_res - 1))

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern RwBool
_rpPatchPipelinesCreate(RwUInt32 pipes);

extern RwBool
_rpPatchPipelinesDestroy(void);

extern RpAtomic *
_rpPatchPipelinesAttach( RpAtomic *atomic,
                         RpPatchType type );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PATCH_H */
