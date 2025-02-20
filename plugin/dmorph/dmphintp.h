/*
 *  dmphintp.h - delta morph interpolation
 */

#if (!defined(_DMPHINTP_H))
#define _DMPHINTP_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/

#include "rwcore.h"
#include "rpworld.h"

#include "dmphanim.h"

/*===========================================================================*
 *--- Types -----------------------------------------------------------------*
 *===========================================================================*/

/* RWPUBLIC */
/*--- RpDMorphInterpolator --------------------------------------------------
 *
 *  Data holding current position in an animation.
 */
typedef struct rpDMorphInterpolator rpDMorphInterpolator;

#if (!defined(DOXYGEN))
struct rpDMorphInterpolator
{
    RpDMorphAnimation  *animation;
    RwReal              animTime;
    rpDMorphFrame     **currFrames;
    RwReal             *currTimes;
    RpAtomicCallBack    loopCallBack;
    void               *loopCallBackData;
};
#endif /* (!defined(DOXYGEN)) */

/*--- rpDMorphAtomicData ---------------------------------------------------
 *
 * This is an extension to RpAtomic.
 */
typedef struct rpDMorphAtomicData rpDMorphAtomicData;

#if (!defined(DOXYGEN))
struct rpDMorphAtomicData
{
    /*--- Morph state data ---*/
    RwReal                  *values;

    /*--- Animation interpolator ---*/
    rpDMorphInterpolator    *interpolator;

    /*--- Render callback chaining ---*/
    RpAtomicCallBackRender  defRenderCallBack;
};
#endif /* (!defined(DOXYGEN)) */

/**
 * \ingroup rpdmorph
 * \ref RpDMorphType defines the different ways a dmorph atomic can
 * be rendered. Once a dmorph \ref RpGeometry has been attached to
 * an \ref RpAtomic the atomic must be setup with the correct dmorph
 * rendering pipeline with \ref RpDMorphAtomicSetType.
 */
enum RpDMorphType
{
    rpNADMORPHTYPE        = 0, /**<Invalid DMorph pipeline.           */
    rpDMORPHTYPEGENERIC   = 1, /**<Generic DMorph rendering.          */
    rpDMORPHTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RpDMorphType RpDMorphType;

/*===========================================================================*
 *--- Macros ----------------------------------------------------------------*
 *===========================================================================*/

/*--- Atomic data ---*/
#define RPDMORPHATOMICGETDATA(atomic)                               \
    ((rpDMorphAtomicData *)(((RwUInt8 *)atomic)+                      \
                          (rpDMorphAtomicDataOffset)))

#define RPDMORPHATOMICGETCONSTDATA(atomic)                          \
    ((const rpDMorphAtomicData *)(((const RwUInt8 *)atomic)+          \
                                (rpDMorphAtomicDataOffset)))
/* RWPUBLICEND */

/*===========================================================================*
 *--- Internal plugin functions ---------------------------------------------*
 *===========================================================================*/

extern RwBool   _rpDMorphAtomicPluginAttach(void);

/* RWPUBLIC */
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*===========================================================================*
 *--- Global variables ------------------------------------------------------*
 *===========================================================================*/

extern RwInt32 rpDMorphAtomicDataOffset;

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/*--- DMorphAtomic functions ------------------------------------------------
 *
 * These functions work at the DMorphAtomic level.
 */
extern RpAtomic *
RpDMorphAtomicInitialize( RpAtomic *atomic );

extern RpAtomic *
RpDMorphAtomicSetType(RpAtomic *atomic,
                      RpDMorphType type);
extern RpDMorphType
RpDMorphAtomicGetType(RpAtomic *atomic);

extern RwReal *
RpDMorphAtomicGetDMorphValues( RpAtomic *atomic );

extern RpAtomic *
RpDMorphAtomicSetAnimation( RpAtomic *atomic,
                            RpDMorphAnimation *animation );

extern RpDMorphAnimation *
RpDMorphAtomicGetAnimation( const RpAtomic *atomic );

extern RpAtomic *
RpDMorphAtomicAddTime( RpAtomic *atomic,
                       RwReal time );

extern RwReal
RpDMorphAtomicGetAnimTime( const RpAtomic *atomic );

extern RpAtomic *
RpDMorphAtomicSetAnimLoopCallBack( RpAtomic *atomic,
                                   RpAtomicCallBack callBack,
                                   void *data );

extern RpAtomicCallBack
RpDMorphAtomicGetAnimLoopCallBack( const RpAtomic *atomic,
                                   void **callBackData );

extern RpAtomic *
RpDMorphAtomicSetAnimFrame( RpAtomic *atomic,
                            RwUInt32 dMorphTargetIndex,
                            RwUInt32 index );

extern RwUInt32
RpDMorphAtomicGetAnimFrame( const RpAtomic *atomic,
                            RwUInt32 dMorphTargetIndex );


extern RpAtomic *
RpDMorphAtomicSetAnimFrameTime( RpAtomic *atomic,
                                RwUInt32 dMorphTargetIndex,
                                RwReal time );

extern RwReal
RpDMorphAtomicGetAnimFrameTime( const RpAtomic *atomic,
                                RwUInt32 dMorphTargetIndex );

#ifdef __cplusplus
}
#endif /* __cplusplus */
/* RWPUBLICEND */

#endif /* _DMPHINTP_H */
