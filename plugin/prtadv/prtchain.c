/**********************************************************************
 *
 * File :     prtchain.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rpplugin.h>
#include <rpdbgerr.h>

#include "rpprtstd.h"
#include "rpprtadv.h"

#include "prtadvvars.h"

#include "prtchain.h"


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


/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup rpprtadv
 * \ref RpPrtAdvParticleChainUpdateCB is the update callback for the advanced
 * chain particle property.
 *
 * This callback updates the standard particle properties in a particle.
 *
 * This callback should be added to a particle class if the particle contains
 * any of the standard particle properties.
 *
 * \param emt           Pointer to the parent emitter.
 * \param prtBatch      Pointer to the particle batch to be updated.
 * \param data          Pointer to private data for the update callback. This
 * callback expects the data to be a pointer to a \ref RwReal representing the
 * delta time elapsed.
 *
 * \return pointer to the particle batch if successful, NULL otherwise.
 *
 * \see RpPrtStdAtomicUpdate
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEmitterStdBeginUpdateCB
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtStdEmitterStdCreateCB
 * \see RpPrtStdEmitterStdDestroyCB
 * \see RpPrtStdEmitterStdEmitCB
 * \see RpPrtStdEmitterStdEndUpdateCB
 * \see RpPrtStdEmitterStdRenderCB
 * \see RpPrtStdPClassSetCallBack
 * \see RpPrtStdPClassStdCreate
 */
RpPrtStdParticleBatch *
RpPrtAdvParticleChainUpdateCB(RpPrtStdEmitter *emt,
                              RpPrtStdParticleBatch *prtBatch,
                              void *data)
{

    RpPrtStdEmitterPTank              * emtPTank;
    RpPrtStdEmitterPrtColor           * emtPrtCol;
    RpPrtStdEmitterPrtTexCoords       * emtPrtTex;
    RpPrtStdEmitterStandard           * emtStd;

    RpPrtStdParticleColor             * prtColIn;
    RpPrtStdParticleColor             * prtColOut;

    RpPrtStdParticleTexCoords         * prtTexIn;
    RpPrtStdParticleTexCoords         * prtTexOut;

    RpPrtAdvParticleChain             * prtChainIn;
    RpPrtAdvParticleChain             * prtChainOut;

    RwChar                            * pTankColIn = (RwChar *)NULL ;
    RwChar                            * pTankColOut = (RwChar *)NULL ;
    RwChar                            * pTankPosIn = (RwChar *)NULL ;
    RwChar                            * pTankPosOut = (RwChar *)NULL ;
    RwChar                            * pTankTexIn = (RwChar *)NULL ;
    RwChar                            * pTankTexOut = (RwChar *)NULL ;
    RwChar                            * prtIn;
    RwChar                            * prtOut;
    RwChar                            * ptrA;
    RwChar                            * ptrB;

    RwInt32                             i, j;
    RwInt32                             numPrtIn;
    RwInt32                             numPrtOut;
    RwInt32                             numPTankPrtOut;
    RwInt32                             numPrtChain;
    RwInt32                             offsetPrtAnim;
    RwInt32                             offsetPrtCol;
    RwInt32                             offsetPrtPos;
    RwInt32                             offsetPrtTex;
    RwInt32                             offsetPrtVel;
    RwInt32                             offsetPrtSize;
    RwInt32                             offsetPrtChain;
    RwInt32                             pTankColStride = 0;
    RwInt32                             pTankPosStride = 0;
    RwInt32                             pTankTexStride = 0;
    RwInt32                             prtSize;

    RwReal                              alphaT;
    RwReal                              deltaT;

    RwV3d                             * prtPosIn;
    RwV3d                             * prtPosOut;
    RwV3d                             * prtVelIn;
    RwV3d                             * prtVelOut;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvParticleChainUpdateCB"));

    RWASSERT(emt);
    RWASSERT(prtBatch);

    deltaT = *(RwReal *)data;

    /* Get the emitter's properties */
    _rpPrtAdvEmitterGetPropOffset(emt);

    emtPTank = NULL;

    RWASSERT(rpPrtAdvGlobals.offsetEmtStd >= 0);
    emtStd = ((RpPrtStdEmitterStandard *)
               RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtStd));

    if (rpPrtAdvGlobals.offsetEmtPrtCol >= 0)
        emtPrtCol = ((RpPrtStdEmitterPrtColor *)
                      RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtPrtCol));

    if (rpPrtAdvGlobals.offsetEmtPrtTex >= 0)
        emtPrtTex = ((RpPrtStdEmitterPrtTexCoords *)
                     RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtPrtTex));

    if (rpPrtAdvGlobals.offsetEmtPTank >= 0)
        emtPTank = ((RpPrtStdEmitterPTank *)
                    RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtPTank));

    if (emtPTank)
    {
        /* Position. */
        i = 0;
        if (emtPTank->dataFlags & rpPTANKDFLAGPOSITION)
        {
            pTankPosIn = (RwChar *) emtPTank->dataInPtrs[i];
            pTankPosOut = (RwChar *) emtPTank->dataOutPtrs[i];
            pTankPosStride = emtPTank->dataStride[i];

            RWASSERT(pTankPosIn);
            RWASSERT(pTankPosOut);

            i++;
        }
        else
        {
            pTankPosIn = NULL;
            pTankPosOut = NULL;
        }

        /* Color. */
        if (emtPTank->dataFlags & rpPTANKDFLAGCOLOR)
        {
            pTankColIn = (RwChar *) emtPTank->dataInPtrs[i];
            pTankColOut = (RwChar *) emtPTank->dataOutPtrs[i];
            pTankColStride = emtPTank->dataStride[i];

            RWASSERT(pTankColIn);
            RWASSERT(pTankColOut);

            i++;
        }
        else
        {
            pTankColIn = NULL;
            pTankColOut = NULL;
        }

        /* Texcoord. */
        if (emtPTank->dataFlags & rpPTANKDFLAGVTX2TEXCOORDS)
        {
            pTankTexIn = (RwChar *) emtPTank->dataInPtrs[i];
            pTankTexOut = (RwChar *) emtPTank->dataOutPtrs[i];
            pTankTexStride = emtPTank->dataStride[i];

            RWASSERT(pTankTexIn);
            RWASSERT(pTankTexOut);
        }
        else
        {
            pTankTexIn = NULL;
            pTankTexOut = NULL;
        }

    }

    /* Get the particle prop offsets. */
    offsetPrtPos = rpPrtAdvGlobals.offsetPrtPos;
    offsetPrtCol = rpPrtAdvGlobals.offsetPrtCol;
    offsetPrtTex = rpPrtAdvGlobals.offsetPrtTex;
    offsetPrtAnim = rpPrtAdvGlobals.offsetPrtAnim;
    offsetPrtVel = rpPrtAdvGlobals.offsetPrtVel;
    offsetPrtChain = rpPrtAdvGlobals.offsetPrtChain;

    /* Particles update loop. */
    prtSize = emt->prtClass->objSize;
    prtIn = ((RwChar *)prtBatch) + prtBatch->offset;
    prtOut = prtIn;

    numPrtIn = prtBatch->numPrt;
    numPrtOut = 0;
    numPTankPrtOut = 0;

    while (numPrtIn-- > 0)
    {
        prtChainIn = (RpPrtAdvParticleChain *)
            RPPRTADVPROP(prtIn, offsetPrtChain);

        numPrtChain = prtChainIn->numPrtChain;

        /* Is this particle dead ? */
        if (((RpPrtStdParticleStandard *)prtIn)->currTime <
            ((RpPrtStdParticleStandard *)prtIn)->endTime)
        {
            prtChainOut = (RpPrtAdvParticleChain *)
                RPPRTADVPROP(prtOut, offsetPrtChain);

            prtChainOut->numPrtChain = prtChainIn->numPrtChain;

            ((RpPrtStdParticleStandard *)prtOut)->flag =
                ((RpPrtStdParticleStandard *)prtIn)->flag;

            /* time */
            ((RpPrtStdParticleStandard *)prtOut)->currTime =
                ((RpPrtStdParticleStandard *)prtIn)->currTime + deltaT;
            ((RpPrtStdParticleStandard *)prtOut)->invEndTime =
                ((RpPrtStdParticleStandard *)prtIn)->invEndTime;
            ((RpPrtStdParticleStandard *)prtOut)->endTime =
                ((RpPrtStdParticleStandard *)prtIn)->endTime;

            alphaT =
                ((RpPrtStdParticleStandard *)prtOut)->currTime *
                ((RpPrtStdParticleStandard *)prtOut)->invEndTime;

            prtVelIn = (RwV3d *) RPPRTADVPROP(prtIn, offsetPrtVel);
            prtVelOut = (RwV3d *) RPPRTADVPROP(prtOut, offsetPrtVel);

            /* velocity v = u + at */
            prtVelOut->x = prtVelIn->x + (deltaT * emtStd->force.x);
            prtVelOut->y = prtVelIn->y + (deltaT * emtStd->force.y);
            prtVelOut->z = prtVelIn->z + (deltaT * emtStd->force.z);

            /* Update the position */
            if (offsetPrtPos >= 0)
            {
                prtPosIn = (RwV3d *) RPPRTADVPROP(prtIn, offsetPrtPos);
                prtPosOut = (RwV3d *) RPPRTADVPROP(prtOut, offsetPrtPos);

                /* position s = ut */
                prtPosOut->x = prtPosIn->x + (deltaT * prtVelOut->x);
                prtPosOut->y = prtPosIn->y + (deltaT * prtVelOut->y);
                prtPosOut->z = prtPosIn->z + (deltaT * prtVelOut->z);
            }

            /* Update the pTank */
            if (emtPTank)
            {
                /* position. */
                if (emtPTank->dataFlags & rpPTANKDFLAGPOSITION)
                {
                    /* Transfer the previous value down the chain */
                    ptrA = pTankPosOut + (numPrtChain - 1) * pTankPosStride;
                    ptrB = pTankPosIn + (numPrtChain - 2) * pTankPosStride;

                    for (i = 0; i < (numPrtChain - 1); i++)
                    {
                        ((RwV3d *) ptrA)->x = ((RwV3d *) ptrB)->x;
                        ((RwV3d *) ptrA)->y = ((RwV3d *) ptrB)->y;
                        ((RwV3d *) ptrA)->z = ((RwV3d *) ptrB)->z;

                        ptrA -= pTankPosStride;
                        ptrB -= pTankPosStride;
                    }

                    /* position s = ut */
                    ((RwV3d *) pTankPosOut)->x =
                        ((RwV3d *) pTankPosIn)->x + (deltaT * prtVelOut->x);
                    ((RwV3d *) pTankPosOut)->y =
                        ((RwV3d *) pTankPosIn)->y + (deltaT * prtVelOut->y);
                    ((RwV3d *) pTankPosOut)->z =
                        ((RwV3d *) pTankPosIn)->z + (deltaT * prtVelOut->z);
                }

                /* color */
                if (emtPTank->dataFlags & rpPTANKDFLAGCOLOR)
                {
                    RWASSERT(offsetPrtCol >= 0);

                    prtColIn = ( (RpPrtStdParticleColor *)
                                 RPPRTADVPROP(prtIn, offsetPrtCol) );

                    /* Transfer the previous value down the chain */
                    ptrA = pTankColOut + (numPrtChain - 1) * pTankColStride;
                    ptrB = pTankColIn + (numPrtChain - 2) * pTankColStride;

                    for (i = 0; i < (numPrtChain - 1); i++)
                    {
                        ((RwRGBA *) ptrA)->red   = ((RwRGBA *) ptrB)->red;
                        ((RwRGBA *) ptrA)->green = ((RwRGBA *) ptrB)->green;
                        ((RwRGBA *) ptrA)->blue  = ((RwRGBA *) ptrB)->blue;
                        ((RwRGBA *) ptrA)->alpha = ((RwRGBA *) ptrB)->alpha;

                        ptrA -= pTankColStride;
                        ptrB -= pTankColStride;
                    }

                    if (prtIn != prtOut)
                    {
                        prtColOut = ((RpPrtStdParticleColor *)
                                     RPPRTADVPROP(prtOut, offsetPrtCol));

                        prtColOut->startCol = prtColIn->startCol;
                        prtColOut->deltaCol = prtColIn->deltaCol;
                    }

                    ((RwRGBA *) pTankColOut)[0].red =
                        RwInt32FromRealMacro(prtColIn->startCol.red +
                                             (alphaT * prtColIn->deltaCol.red));
                    ((RwRGBA *) pTankColOut)[0].green =
                        RwInt32FromRealMacro(prtColIn->startCol.green +
                                             (alphaT * prtColIn->deltaCol.green));
                    ((RwRGBA *) pTankColOut)[0].blue =
                        RwInt32FromRealMacro(prtColIn->startCol.blue +
                                             (alphaT * prtColIn->deltaCol.blue));
                    ((RwRGBA *) pTankColOut)[0].alpha =
                        RwInt32FromRealMacro(prtColIn->startCol.alpha +
                                             (alphaT * prtColIn->deltaCol.alpha));

                    pTankColOut += numPrtChain * pTankColStride;
                }

                /* uv */
                if (emtPTank->dataFlags & rpPTANKDFLAGVTX2TEXCOORDS)
                {
                    RWASSERT(offsetPrtTex >= 0);

                    prtTexIn = ((RpPrtStdParticleTexCoords *)
                                RPPRTADVPROP(prtIn, offsetPrtTex));

                    /* Transfer the previous value down the chain */
                    ptrA = pTankTexOut + (numPrtChain - 1) * pTankTexStride;
                    ptrB = pTankTexIn + (numPrtChain - 2) * pTankTexStride;

                    for (i = (2 * (numPrtChain - 1)); i > 1; i--)
                    {
                        ((RwTexCoords *) ptrA)[0].u = ((RwTexCoords *) ptrB)[0].u;
                        ((RwTexCoords *) ptrA)[0].v = ((RwTexCoords *) ptrB)[0].v;

                        ((RwTexCoords *) ptrA)[1].u = ((RwTexCoords *) ptrB)[1].u;
                        ((RwTexCoords *) ptrA)[1].v = ((RwTexCoords *) ptrB)[1].v;

                        ptrA -= pTankTexStride;
                        ptrB -= pTankTexStride;
                    }

                    /* We need to copy over the particle tex coords */
                    if (prtIn != prtOut)
                    {
                        prtTexOut = ((RpPrtStdParticleTexCoords *)
                                     RPPRTADVPROP(prtOut, offsetPrtTex));

                        prtTexOut->startUV0 = prtTexIn->startUV0;
                        prtTexOut->deltaUV0 = prtTexIn->deltaUV0;

                        prtTexOut->startUV1 = prtTexIn->startUV1;
                        prtTexOut->deltaUV1 = prtTexIn->deltaUV1;
                    }

                    ((RwTexCoords *) pTankTexOut)[0].u =
                        prtTexIn->startUV0.u +
                        (alphaT * prtTexIn->deltaUV0.u);
                    ((RwTexCoords *) pTankTexOut)[0].v =
                        prtTexIn->startUV0.v +
                        (alphaT * prtTexIn->deltaUV0.v);

                    ((RwTexCoords *) pTankTexOut)[1].u =
                            prtTexIn->startUV1.u +
                            (alphaT * prtTexIn->deltaUV1.u);
                    ((RwTexCoords *) pTankTexOut)[1].v =
                            prtTexIn->startUV1.v +
                            (alphaT * prtTexIn->deltaUV1.v);

                    pTankTexOut += numPrtChain * pTankTexStride;
                }
            }

            prtOut += prtSize;

            pTankPosOut += numPrtChain * pTankPosStride;

            numPrtOut++;
            numPTankPrtOut += numPrtChain;
        }

        /* Next input prt */
        prtIn += prtSize;

        pTankPosIn += numPrtChain * pTankPosStride;
        pTankColIn += numPrtChain * pTankColStride;
        pTankTexIn += numPrtChain * pTankTexStride;
    }

    /* Update the prt count in the batch */
    prtBatch->numPrt = numPrtOut;

    /* Position. */
    if (emtPTank)
    {
        emtPTank->numPrt += numPTankPrtOut;

        i = 0;
        if (emtPTank->dataFlags & rpPTANKDFLAGPOSITION)
        {
            emtPTank->dataInPtrs[i] = pTankPosIn;
            emtPTank->dataOutPtrs[i] = pTankPosOut;
            i++;
        }

        /* Color. */
        if (emtPTank->dataFlags & rpPTANKDFLAGCOLOR)
        {
            emtPTank->dataInPtrs[i] = pTankColIn;
            emtPTank->dataOutPtrs[i] = pTankColOut;
            i++;
        }

        /* Texcoord. */
        if (emtPTank->dataFlags & rpPTANKDFLAGVTX2TEXCOORDS)
        {
            emtPTank->dataInPtrs[i] = pTankTexIn;
            emtPTank->dataOutPtrs[i] = pTankTexOut;
        }
    }

    RWRETURN(prtBatch);
}

/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup rpprtadv
 * RpPrtStdEmitterStdEmitCB is the emit callback for the standard
 * emitter property.
 *
 * This callback emits new particles using the current emitter. The particle
 * class must contain the standard particle properties.
 *
 * This callback should be added to an emitter class if the emitter contains
 * any of the standard emitter properties.
 *
 * \param atomic        Pointer to the parent atomic.
 * \param emt           Pointer to the emitter.
 * \param data          Pointer to private data for the emit callback. This
 * callback expects the data to be a pointer to a \ref RwReal representing the delta
 * time elapsed.
 *
 * \return pointer to the emitter if successful, NULL otherwise.
 *
 * \see RpPrtStdAtomicUpdate
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEmitterStdBeginUpdateCB
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtStdEmitterStdCreateCB
 * \see RpPrtStdEmitterStdDestroyCB
 * \see RpPrtStdEmitterStdEndUpdateCB
 * \see RpPrtStdEmitterStdRenderCB
 * \see RpPrtStdPClassSetCallBack
 * \see RpPrtStdPClassStdCreate
 */

RpPrtStdEmitter *
RpPrtAdvEmitterChainEmitCB(RpAtomic * __RWUNUSED__ atomic,
                           RpPrtStdEmitter *emt,
                           void *  __RWUNUSED__ data)
{
    RwMatrix        *ltm;
    RwV3d           v;
    RwReal          rr;
    RwInt32         ir, numPrt, i, j, prtSize, numPrtChain, numPTankOut;

    RpPrtStdEmitter                 *result;
    RpPrtStdParticleBatch           *prtBatch;

    RwChar                         *pTankPosOut = (RwChar *)NULL;
    RwChar                         *pTankColOut = (RwChar *)NULL;
    RwChar                         *pTankTexOut = (RwChar *)NULL;
    RwInt32                         pTankPosStride = 0;
    RwInt32                         pTankColStride = 0;
    RwInt32                         pTankTexStride = 0;

    RwInt32                         offsetPrtPos;
    RwInt32                         offsetPrtCol;
    RwInt32                         offsetPrtTex;
    RwInt32                         offsetPrtAnim;
    RwInt32                         offsetPrtVel;
    RwInt32                         offsetPrtChain;

    RpPrtStdEmitterStandard         *emtStd;
    RpPrtStdEmitterPTank            *emtPTank;
    RpPrtStdEmitterPrtColor         *emtPrtCol = (RpPrtStdEmitterPrtColor *)NULL;
    RpPrtStdEmitterPrtTexCoords     *emtPrtTex = (RpPrtStdEmitterPrtTexCoords *)NULL;
    RpPrtAdvEmitterPrtChain         *emtPrtChain;

    RwChar                          *prt;
    RpPrtStdParticleStandard        *prtStd;
    RpPrtStdParticleColor           *prtCol;
    RpPrtStdParticleTexCoords       *prtTex;
    RpPrtAdvParticleChain           *prtChain;

    RwChar                          *ptrA;

    RwV3d                           *prtPos, *prtVel, pos;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmitterChainEmitCB"));

    RWASSERT(atomic);
    RWASSERT(emt);
    RWASSERT(data);

    result = NULL;
    emtPTank = NULL;
    numPTankOut = 0;


    /* Get the emitter's properties */
    _rpPrtAdvEmitterGetPropOffset(emt);

    RWASSERT(rpPrtAdvGlobals.offsetEmtStd >= 0);
    emtStd = ((RpPrtStdEmitterStandard *)
              RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtStd));

    if (rpPrtAdvGlobals.offsetEmtPrtCol >= 0)
        emtPrtCol = ((RpPrtStdEmitterPrtColor *)
                     RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtPrtCol));

    if (rpPrtAdvGlobals.offsetEmtPrtTex >= 0)
        emtPrtTex = ((RpPrtStdEmitterPrtTexCoords *)
                     RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtPrtTex));

    if (rpPrtAdvGlobals.offsetEmtPTank >= 0)
        emtPTank = ((RpPrtStdEmitterPTank *)
                    RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtPTank));

    if (rpPrtAdvGlobals.offsetEmtPrtChain >= 0)
        emtPrtChain = ((RpPrtAdvEmitterPrtChain *)
                    RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtPrtChain));


    /* Check if we are to emit particles. */
    if (emtStd->currTime > emtStd->emtEmitTime)
    {
        /* Get the emitter props. */

        /* Grab a new particle for the new particles. */
        prtBatch = RpPrtStdEmitterNewParticleBatch(emt);

        if (prtBatch != NULL)
        {
            RwInt32 shift;
            /* Get the particle prop offsets. */

            offsetPrtVel= rpPrtAdvGlobals.offsetPrtVel;
            offsetPrtPos = rpPrtAdvGlobals.offsetPrtPos;
            offsetPrtCol = rpPrtAdvGlobals.offsetPrtCol;
            offsetPrtTex = rpPrtAdvGlobals.offsetPrtTex;
            offsetPrtAnim = rpPrtAdvGlobals.offsetPrtAnim;
            offsetPrtChain = rpPrtAdvGlobals.offsetPrtChain;

            /* Number of particles to be emitted. */
            ir = emtStd->seed;
            PRTSTD_SRAND(ir);
            rr = PRTSTD_RSRAND(ir);

            shift = RwInt32FromRealMacro(rr * (RwReal)(emtStd->emtPrtEmitBias));
            numPrt = emtStd->emtPrtEmit + shift;

            /* Clamp to maximum possible particles. */
            if (emtPTank)
            {
                if ((emt->prtActive + numPrt) > (RwInt32)emtPTank->maxPrt)
                    numPrt = (RwInt32)emtPTank->maxPrt - emt->prtActive;

                /* Position.  */
                i = 0;
                if (emtPTank->dataFlags & rpPTANKDFLAGPOSITION)
                {
                    pTankPosOut = emtPTank->dataOutPtrs[i];
                    pTankPosStride = emtPTank->dataStride[i];

                    i++;
                }

                /* Color. */
                if (emtPTank->dataFlags & rpPTANKDFLAGCOLOR)
                {
                    pTankColOut = emtPTank->dataOutPtrs[i];
                    pTankColStride = emtPTank->dataStride[i];

                    RWASSERT(pTankColOut);

                    i++;
                }

                /* Texcoord. */
                if (emtPTank->dataFlags & rpPTANKDFLAGVTX2TEXCOORDS)
                {
                    pTankTexOut = emtPTank->dataOutPtrs[i];
                    pTankTexStride = emtPTank->dataStride[i];

                    RWASSERT(pTankTexOut);

                    i++;
                }
            }

            ltm = emtStd->ltm;

            /* Main process loop. */
            prtSize = emt->prtClass->objSize;
            prt = ((RwChar *) prtBatch) + prtBatch->offset;

            for (i = 0; i < numPrt; i++)
            {
                prtStd = (RpPrtStdParticleStandard *) prt;

                prtVel = (RwV3d *) RPPRTADVPROP(prt, offsetPrtVel);

                prtChain = (RpPrtAdvParticleChain *)
                    RPPRTADVPROP(prt, offsetPrtChain);

                /* Chain length */
                PRTSTD_SRAND(ir);
                rr = PRTSTD_RSRAND(ir);


                numPrtChain = emtPrtChain->numPrtChain +
                    RwInt32FromRealMacro(rr * emtPrtChain->numPrtChainBias);

                prtChain->numPrtChain = numPrtChain;
                /* Life */

                prtStd->currTime = (RwReal) 0.0;
                PRTSTD_SRAND(ir);
                rr = PRTSTD_RSRAND(ir);
                prtStd->endTime =
                    emtStd->prtLife + (rr * emtStd->prtLifeBias);
                prtStd->invEndTime = (RwReal) 1.0 / prtStd->endTime;


                /* Initial velocity. */
                PRTSTD_SRAND(ir);
                rr = PRTSTD_RSRAND(ir);
                v.x = emtStd->prtInitDir.x + (rr * emtStd->prtInitDirBias.x);
                PRTSTD_SRAND(ir);
                rr = PRTSTD_RSRAND(ir);
                v.y = emtStd->prtInitDir.y + (rr * emtStd->prtInitDirBias.y);
                PRTSTD_SRAND(ir);
                rr = PRTSTD_RSRAND(ir);
                v.z = emtStd->prtInitDir.z + (rr * emtStd->prtInitDirBias.z);

                RwV3dNormalize(&v, &v);

                /* Transform the vec to world space. */
                if (ltm)
                    RwV3dTransformVector(&v, &v, ltm);

                /* Scale to initial velocity. */
                PRTSTD_SRAND(ir);
                rr = PRTSTD_RSRAND(ir);
                RwV3dScale((RwV3d *)prtVel, &v,
                    (emtStd->prtInitVel + (rr * emtStd->prtInitVelBias)));

                /* Initial pos. */
                PRTSTD_SRAND(ir);
                rr = PRTSTD_RSRAND(ir);
                v.x = emtStd->emtPos.x + (rr * emtStd->emtSize.x);
                PRTSTD_SRAND(ir);
                rr = PRTSTD_RSRAND(ir);
                v.y = emtStd->emtPos.y + (rr * emtStd->emtSize.y);
                PRTSTD_SRAND(ir);
                rr = PRTSTD_RSRAND(ir);
                v.z = emtStd->emtPos.z + (rr * emtStd->emtSize.z);

                /* Transform to world space. */
                if (ltm)
                    RwV3dTransformPoint(&pos, &v, ltm);

                if (offsetPrtPos >= 0)
                {
                    prtPos = (RwV3d *) RPPRTADVPROP(prt, offsetPrtPos);

                    prtPos->x = pos.x;
                    prtPos->y = pos.y;
                    prtPos->z = pos.z;
                }

                /* PTank */
                if (emtPTank)
                {
                    if (emtPTank->dataFlags & rpPTANKDFLAGPOSITION)
                    {
                        ptrA = pTankPosOut;

                        for (j = 0; j < numPrtChain; j++)
                        {
                            ((RwV3d *) ptrA)->x = pos.x;
                            ((RwV3d *) ptrA)->y = pos.y;
                            ((RwV3d *) ptrA)->z = pos.z;

                            ptrA += pTankPosStride;
                        }

                        pTankPosOut += numPrtChain * pTankPosStride;
                    }

                    /* Initial colors. */
                    if (emtPTank->dataFlags & rpPTANKDFLAGCOLOR)
                    {
                        prtCol = ((RpPrtStdParticleColor *)
                                   RPPRTADVPROP(prt, offsetPrtCol));

                        /* Start color. */
                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtCol->startCol.red =
                            emtPrtCol->prtStartCol.red + (rr * emtPrtCol->prtStartColBias.red);

                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtCol->startCol.green =
                            emtPrtCol->prtStartCol.green + (rr * emtPrtCol->prtStartColBias.green);

                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtCol->startCol.blue =
                            emtPrtCol->prtStartCol.blue + (rr * emtPrtCol->prtStartColBias.blue);

                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtCol->startCol.alpha =
                            emtPrtCol->prtStartCol.alpha + (rr * emtPrtCol->prtStartColBias.alpha);

                        ((RwRGBA *) pTankColOut)->red =
                            RwInt32FromRealMacro(prtCol->startCol.red);
                        ((RwRGBA *) pTankColOut)->green =
                            RwInt32FromRealMacro(prtCol->startCol.green);
                        ((RwRGBA *) pTankColOut)->blue =
                            RwInt32FromRealMacro(prtCol->startCol.blue);
                        ((RwRGBA *) pTankColOut)->alpha =
                            RwInt32FromRealMacro(prtCol->startCol.alpha);

                        /* Delta color. */
                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtCol->deltaCol.red =
                            emtPrtCol->prtEndCol.red + (rr * emtPrtCol->prtEndColBias.red) -
                            prtCol->startCol.red;

                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtCol->deltaCol.green =
                            emtPrtCol->prtEndCol.green + (rr * emtPrtCol->prtEndColBias.green) -
                            prtCol->startCol.green;

                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtCol->deltaCol.blue =
                            emtPrtCol->prtEndCol.blue + (rr * emtPrtCol->prtEndColBias.blue) -
                            prtCol->startCol.blue;

                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtCol->deltaCol.alpha =
                            emtPrtCol->prtEndCol.alpha + (rr * emtPrtCol->prtEndColBias.alpha) -
                            prtCol->startCol.alpha;

                        ptrA = pTankColOut + pTankColStride;

                        for (j = 0; j < (numPrtChain - 1); j++)
                        {
                            ((RwRGBA *) ptrA)->red =
                                ((RwRGBA *) pTankColOut)->red;
                            ((RwRGBA *) ptrA)->green =
                                ((RwRGBA *) pTankColOut)->green;
                            ((RwRGBA *) ptrA)->blue =
                                ((RwRGBA *) pTankColOut)->blue;
                            ((RwRGBA *) ptrA)->alpha =
                                ((RwRGBA *) pTankColOut)->alpha;

                            ptrA += pTankColStride;
                        }

                        pTankColOut += numPrtChain * pTankColStride;
                    }

                    /* Initial texcoord. */
                    if (emtPTank->dataFlags & rpPTANKDFLAGVTX2TEXCOORDS)
                    {
                        prtTex = ((RpPrtStdParticleTexCoords *)
                                   RPPRTADVPROP(prt, offsetPrtTex));

                        /* Start UV. */
                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtTex->startUV0.u = emtPrtTex->prtStartUV0.u +
                            (rr * emtPrtTex->prtStartUV0Bias.u);
                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtTex->startUV0.v = emtPrtTex->prtStartUV0.v +
                            (rr * emtPrtTex->prtStartUV0Bias.v);

                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtTex->startUV1.u = emtPrtTex->prtStartUV1.u +
                            (rr * emtPrtTex->prtStartUV1Bias.u);
                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtTex->startUV1.v = emtPrtTex->prtStartUV1.v +
                            (rr * emtPrtTex->prtStartUV1Bias.v);

                        ((RwTexCoords *) pTankTexOut)[0].u = prtTex->startUV0.u;
                        ((RwTexCoords *) pTankTexOut)[0].v = prtTex->startUV0.v;

                        ((RwTexCoords *) pTankTexOut)[1].u = prtTex->startUV1.u;
                        ((RwTexCoords *) pTankTexOut)[1].v = prtTex->startUV1.v;

                        /* Delta UV. */
                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtTex->deltaUV0.u =
                            emtPrtTex->prtEndUV0.u + (rr * emtPrtTex->prtEndUV0Bias.u) -
                            prtTex->startUV0.u;

                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtTex->deltaUV0.v =
                            emtPrtTex->prtEndUV0.v + (rr * emtPrtTex->prtEndUV0Bias.v) -
                            prtTex->startUV0.v;

                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtTex->deltaUV1.u =
                            emtPrtTex->prtEndUV1.u + (rr * emtPrtTex->prtEndUV1Bias.u) -
                            prtTex->startUV1.u;

                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtTex->deltaUV1.v =
                            emtPrtTex->prtEndUV1.v + (rr * emtPrtTex->prtEndUV1Bias.v) -
                            prtTex->startUV1.v;

                        ptrA = pTankTexOut + pTankTexStride;

                        for (j = 0; j < (numPrtChain - 1); j++)
                        {
                            ((RwTexCoords *) ptrA)[0].u =
                                ((RwTexCoords *) pTankTexOut)[0].u;
                            ((RwTexCoords *) ptrA)[0].v =
                                ((RwTexCoords *) pTankTexOut)[0].v;

                            ((RwTexCoords *) ptrA)[1].u =
                                ((RwTexCoords *) pTankTexOut)[1].u;
                            ((RwTexCoords *) ptrA)[1].v =
                                ((RwTexCoords *) pTankTexOut)[1].v;

                            ptrA += pTankTexStride;
                        }

                        pTankTexOut += numPrtChain * pTankTexStride;
                    }
                }

                numPTankOut += numPrtChain;

                prt += prtSize;
            }

            if (emtPTank)
            {
                emtPTank->numPrt += numPTankOut;

                /* Position. */
                i = 0;
                if (emtPTank->dataFlags & rpPTANKDFLAGPOSITION)
                {
                    emtPTank->dataOutPtrs[i] = pTankPosOut;
                    i++;
                }

                /* Color. */
                if (emtPTank->dataFlags & rpPTANKDFLAGCOLOR)
                {
                    emtPTank->dataOutPtrs[i] = pTankColOut;
                    i++;
                }

                /* Texcoord. */
                if (emtPTank->dataFlags & rpPTANKDFLAGVTX2TEXCOORDS)
                {
                    emtPTank->dataOutPtrs[i] = pTankTexOut;
                }
            }

            /* Set the number of active part count. */
            prtBatch->numPrt = numPrt;

            /* Update the emitter active count. */
            emt->prtActive += numPrt;

            /* Update the seed. */
            emtStd->seed = ir;

            result = emt;
        }
    }
    else
        result = emt;

    RWRETURN(result);
}

/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup rpprtadv
 * RpPrtStdEmitterStdCreateCB is the create callback for the standard
 * emitter property.
 *
 * This callback setups the standard emitter properties of an emitter.
 *
 * This callback should be added to an emitter class if the emitter contains
 * any of the standard emitter properties.
 *
 * \param atomic        Pointer to the parent atomic.
 * \param emt           Pointer to the emitter.
 * \param data          Pointer to private data for the emit callback.
 *
 * \return pointer to the emitter if successful, NULL otherwise.
 *
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEmitterStdBeginUpdateCB
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtStdEmitterStdEmitCB
 * \see RpPrtStdEmitterStdEndUpdateCB
 * \see RpPrtStdEmitterStdRenderCB
 * \see RpPrtStdPClassSetCallBack
 * \see RpPrtStdPClassStdCreate
 * \see RpPrtStdParticleStdUpdateCB
 */
RpPrtStdEmitter *
RpPrtAdvEmitterChainCreateCB(RpAtomic * __RWUNUSED__ atomic,
                                RpPrtStdEmitter *emt,
                                void * __RWUNUSED__ data)
{
    RwInt32                     offset;
    RpPrtStdEmitterStandard     *emtStd;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmitterChainCreateCB"));

    RWASSERT(emt);

    offset = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
                                          rpPRTSTDPROPERTYCODEEMITTERSTANDARD);
    RWASSERT(offset >= 0);

    emtStd = ((RpPrtStdEmitterStandard *)
               RPPRTADVPROP(emt, offset));

    emt->flag |= (rpPRTSTDEMITTERFLAGALIVE |
                  rpPRTSTDEMITTERFLAGACTIVE |
                  rpPRTSTDEMITTERFLAGPTANK);

    emtStd->seed = 1;

    emtStd->maxPrt = 0;

    emtStd->prtSize.x = (RwReal) 1.0;
    emtStd->prtSize.y = (RwReal) 1.0;

    emtStd->prtColor.red   = (RwInt8) 255;
    emtStd->prtColor.green = (RwInt8) 255;
    emtStd->prtColor.blue  = (RwInt8) 255;
    emtStd->prtColor.alpha = (RwInt8) 255;

    emtStd->prtUV[0].u = (RwReal) 0.0;
    emtStd->prtUV[0].v = (RwReal) 0.0;

    emtStd->prtUV[1].u = (RwReal) 1.0;
    emtStd->prtUV[1].v = (RwReal) 1.0;

    emtStd->prtUV[2].u = (RwReal) 0.0;
    emtStd->prtUV[2].v = (RwReal) 0.0;

    emtStd->prtUV[3].u = (RwReal) 0.0;
    emtStd->prtUV[3].v = (RwReal) 0.0;

    RWRETURN(emt);
}

/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup rpprtadv
 * RpPrtStdEmitterStdBeginUpdateCB is the begin update callback for
 * the standard emitter property.
 *
 * This callback updates the standard emitter properties of an emitter.
 *
 * This callback should be added to an emitter class if the emitter contains
 * any of the standard emitter properties.
 *
 * \param atomic        Pointer to the parent atomic.
 * \param emt           Pointer to the emitter.
 * \param data          Pointer to private data for the emit callback. Not used.
 *
 * \return pointer to the emitter if successful, NULL otherwise.
 *
 * \see RpPrtStdAtomicUpdate
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtStdEmitterStdCreateCB
 * \see RpPrtStdEmitterStdDestroyCB
 * \see RpPrtStdEmitterStdEmitCB
 * \see RpPrtStdEmitterStdEndUpdateCB
 * \see RpPrtStdEmitterStdRenderCB
 * \see RpPrtStdPClassSetCallBack
 * \see RpPrtStdPClassStdCreate
 */
RpPrtStdEmitter *
RpPrtAdvEmitterChainBeginUpdateCB(RpAtomic *atomic,
                                  RpPrtStdEmitter *emt, void *data)
{
    RwReal                      deltaT;
    RwFrame                     *frame;
    RpPrtStdEmitterStandard     *emtStd;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmitterChainBeginUpdateCB"));

    RWASSERT(atomic);
    RWASSERT(emt);

    RWASSERT(rpPrtAdvGlobals.offsetEmtStd >= 0);

    emtStd = ((RpPrtStdEmitterStandard *)
               RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtStd));

    frame = RpAtomicGetFrame(atomic);
    emtStd->ltm = RwFrameGetLTM(frame);

    deltaT = *(RwReal *)data;
    emtStd->currTime += deltaT;

    RWRETURN(emt);
}

/**
 * \ingroup rpprtadv
 * RpPrtStdEmitterStdEndUpdateCB is the end update callback for
 * the standard emitter property.
 *
 * This callback completes the updates for the standard emitter properties
 * of an emitter.
 *
 * This callback should be added to an emitter class if the emitter contains
 * any of the standard emitter properties.
 *
 * \param atomic        Pointer to the parent atomic.
 * \param emt           Pointer to the emitter.
 * \param data          Pointer to private data for the emit callback. Not used.
 *
 * \return pointer to the emitter if successful, NULL otherwise.
 *
 * \see RpPrtStdAtomicUpdate
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEmitterStdBeginUpdateCB
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtStdEmitterStdCreateCB
 * \see RpPrtStdEmitterStdDestroyCB
 * \see RpPrtStdEmitterStdEmitCB
 * \see RpPrtStdEmitterStdRenderCB
 * \see RpPrtStdPClassSetCallBack
 * \see RpPrtStdPClassStdCreate
 */
RpPrtStdEmitter *
RpPrtAdvEmitterChainEndUpdateCB(RpAtomic * __RWUNUSED__ atomic,
                                   RpPrtStdEmitter *emt,
                                   void *__RWUNUSED__ data)
{
    RpPrtStdEmitterStandard       *emtStd;
    RwReal          rr;
    RwInt32         ir;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmitterChainEndUpdateCB"));

    RWASSERT(atomic);
    RWASSERT(emt);

    RWASSERT(rpPrtAdvGlobals.offsetEmtStd >= 0);

    emtStd = ((RpPrtStdEmitterStandard *)
              RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtStd));

    /* Update the time for the next emission. */
    if (emtStd->currTime > emtStd->emtEmitTime)
    {
        emtStd->emtPrevEmitTime = emtStd->currTime;

        ir = emtStd->seed;
        PRTSTD_SRAND(ir);
        rr = PRTSTD_RSRAND(ir);
        emtStd->emtEmitTime = emtStd->currTime +
            emtStd->emtEmitGap + (rr * emtStd->emtEmitGapBias);

        emtStd->seed = ir;
    }

    RWRETURN(emt);
}


/************************************************************************
 *
 *
 *
 ************************************************************************/

 RpPrtStdPropertyTable *
_rpPrtAdvEmitterChainPropTabCreate( void )
{
    RpPrtStdPropertyTable       *propTab;
    RwInt32                     prop[2];

    RWFUNCTION(RWSTRING("_rpPrtAdvEmitterChainPropTabCreate"));

    propTab = NULL;

    prop[0] = rpPRTSTDPROPERTYCODEEMITTERSTANDARD;
    prop[1] = sizeof(RpPrtStdEmitterStandard);

    propTab = RpPrtStdPropTabCreate(1, prop, &prop[1]);

    RWRETURN(propTab);
}

 RpPrtStdPropertyTable *
_rpPrtAdvParticleChainPropTabCreate( void )
{
    RpPrtStdPropertyTable       *propTab;
    RwInt32                     prop[2];

    RWFUNCTION(RWSTRING("_rpPrtAdvParticleChainPropTabCreate"));

    propTab = NULL;

    prop[0] = rpPRTSTDPROPERTYCODEPARTICLESTANDARD;
    prop[1] = sizeof(RpPrtStdParticleStandard);

    propTab = RpPrtStdPropTabCreate(1, prop, &prop[1]);

    RWRETURN(propTab);
}


