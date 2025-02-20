/**********************************************************************
 *
 * File :     stdgen.h
 *
 * Abstract : Basic Particle Engine in RenderWare
 *
 **********************************************************************
 *
 * This file is a product of Criterion Software Ltd.
 *
 * This file is provided as is with no warranties of any kind and is
 * provided without any obligation on Criterion Software Ltd. or
 * Canon Inc. to assist in its use or modification.
 *
 * Criterion Software Ltd. will not, under any
 * circumstances, be liable for any lost revenue or other damages arising
 * from the use of this file.
 *
 * Copyright (c) 1998 Criterion Software Ltd.
 * All Rights Reserved.
 *
 * RenderWare is a trademark of Canon Inc.
 *
 ************************************************************************/

#ifndef STDGEN_HDR
#define STDGEN_HDR

#include <rwcore.h>

#include "rpprtstd.h"

#include "emitter.h"


/************************************************************************
 *
 *
 *
 ************************************************************************/

/************************************************************************
 *
 *
 *
 ************************************************************************/

typedef struct RpPrtStdEmitterStandardOld RpPrtStdEmitterStandardOld;

struct RpPrtStdEmitterStandardOld
{
    RwInt32                     seed;                           /**< Seed for random number generator */

    RwInt32                     maxPrt;                         /**< Maximum number of active particles */

    RwReal                      currTime,                       /**< Current timestamp for emitter */
                                prevTime;                       /**< Previous timestamp for emitter */
    RwV3d                       force;                          /**< External force applied to particles */

    RwV3d                       emtPos,                         /**< Emitter position */
                                emtSize;                        /**< Emitter size. This is the volume where new particles
                                                                 * are emitted */
    RwReal                      emtEmitGap,                     /**< Time gap between emission */
                                emtEmitGapBias,                 /**< Time gap bias */
                                emtPrevEmitTime,                /**< Previous particle emission time */
                                emtEmitTime;                    /**< Next particle emission time */
    RwInt32                     emtPrtEmit,                     /**< Number of particle emit per emission */
                                emtPrtEmitBias;                 /**< Particle emission bias */

    RwReal                      prtInitVel,                     /**< Particle's initial speed */
                                prtInitVelBias;                 /**< Particle's initial speed bias */
    RwReal                      prtLife,                        /**< Particle's duration */
                                prtLifeBias;                    /**< Particle's duration bias */

    RwV3d                       prtInitDir,                     /**< Particle's initial direction */
                                prtInitDirBias;                 /**< particle's initial direction bias */

    RwV2d                       prtSize;                        /**< Particle's size */

    RwRGBA                      prtColor;                       /**< Particle's color */

    RwTexCoords                 prtUV[4];                       /**< Particle's texture coordindates */
    RwTexture                   *texture;                       /**< Particle's texture */

    RwMatrix                    *ltm;                           /**< LTM to transform particle coordinates from local space
                                                                 * to world space */
};

/************************************************************************
 *
 *
 *
 ************************************************************************/

#ifdef    __cplusplus
extern "C"
{
#endif         /* __cplusplus */

/************************************************************************/

#if (0)

extern RpPrtStdParticleBatch *
_rpPrtStdParticleStdCreateCB(RpPrtStdEmitter *emt,
                                  RpPrtStdParticleBatch *prtBatch, void *data);

extern RpPrtStdParticleBatch *
_rpPrtStdParticleStdDestroyCB(RpPrtStdEmitter *emt,
                                   RpPrtStdParticleBatch *prtBatch, void *data);

extern RpPrtStdParticleBatch *
_rpPrtStdParticleStdEmitCB(RpPrtStdEmitter *emt,
                                RpPrtStdParticleBatch *prtBatch, void *data);

extern RpPrtStdParticleBatch *
_rpPrtStdParticleStdUpdateCB(RpPrtStdEmitter *emt,
                                  RpPrtStdParticleBatch *prtBatch, void *data);

/************************************************************************/

extern RpPrtStdEmitter *
_rpPrtStdStandardEmtEmitCB(RpAtomic *atomic,
                           RpPrtStdEmitter *emt, void *data);

extern RpPrtStdEmitter *
_rpPrtStdEmitterStdCreateCB(RpAtomic *atomic,
                                 RpPrtStdEmitter *emt, void *data);

extern RpPrtStdEmitter *
_rpPrtStdEmitterStdDestroyCB(RpAtomic *atomic,
                                 RpPrtStdEmitter *emt, void *data);

extern RpPrtStdEmitter *
_rpPrtStdEmitterStdBeginUpdateCB(RpAtomic *atomic,
                                      RpPrtStdEmitter *emt, void *data);

extern RpPrtStdEmitter *
_rpPrtStdEmitterStdEndUpdateCB(RpAtomic *atomic,
                                    RpPrtStdEmitter *emt, void *data);

extern RpPrtStdEmitter *
_rpPrtStdEmitterStdRenderCB(RpAtomic *atomic,
                                 RpPrtStdEmitter *emt, void *data);

#endif /* (0) */

extern RpPrtStdPropertyTable *
_rpPrtStdEmitterStdPropTabCreate( void );

extern RpPrtStdPropertyTable *
_rpPrtStdParticleStdPropTabCreate( void );

#ifdef    __cplusplus
}
#endif         /* __cplusplus */


#endif /* STDGEN_HDR */


