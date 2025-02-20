/**********************************************************************
 *
 * File :     prtmcol.c
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

#include <rpprtstd.h>

#include "prtadvvars.h"

#include "prttexst.h"


/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup prtadvtexstep
 * \ref RpPrtAdvPrtTexStepUpdateCB is the update callback for the
 * stepped texture coordinate particle property.
 *
 * This callback updates the stepped texture coordinate property in a particle.
 *
 * This callback should be added to a particle class if the particle contains
 * the particle's stepped texture coordinate properties.
 *
 * The callback must be placed so that it is called before the standard update
 * callback.
 *
 * \param emt           Pointer to the parent \ref RpPrtStdEmitter.
 * \param prtBatch      Pointer to the \ref RpPrtStdParticleBatch to be updated.
 * \param data          Void pointer to private data for the update callback. This
 * callback expects the data to be a pointer to a \ref RwReal representing the
 * delta time elapsed.
 *
 * \return Pointer to the RpPrtStdParticleBatch if successful, NULL otherwise.
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
RpPrtAdvPrtTexStepUpdateCB(RpPrtStdEmitter *emt,
                           RpPrtStdParticleBatch *prtBatch,
                           void *data)
{
    RpPrtStdEmitterStandard           * emtStd;

    RpPrtAdvEmtPrtMultiTexCoords      * emtPrtMultiTex = NULL;

    RpPrtStdParticleTexCoords         * prtTexIn;

    RpPrtAdvPrtTexStep                * prtMultiTexIn;

    RwChar                            * prtIn;
    RwChar                            * prtOut;

    RwInt32                             numPrtIn;
    RwInt32                             prtStride;
    RwInt32                             prtMultiTexSize = 0;
    RwInt32                             texTimeSize = 0;
    RwInt32                             texSize = 0;

    RwReal                              currTime, deltaT, *multiTime;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvPrtTexStepUpdateCB"));

    RWASSERT(emt);
    RWASSERT(prtBatch);

    deltaT = *(RwReal *)data;

    /* Get the emitter's properties */
    _rpPrtAdvEmitterGetPropOffset(emt);

    RWASSERT(rpPrtAdvGlobals.offsetEmtStd >= 0);

    emtStd = ((RpPrtStdEmitterStandard *)
               RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtStd));

    if (rpPrtAdvGlobals.offsetEmtPrtMultiTexStep >= 0)
    {
        emtPrtMultiTex = ((RpPrtAdvEmtPrtMultiTexCoords *)
             RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtPrtMultiTexStep));

        texTimeSize = emtPrtMultiTex->numTex * sizeof(RwReal);
        texSize = emtPrtMultiTex->numTex * sizeof(RwTexCoords);

        prtMultiTexSize = sizeof(RpPrtAdvPrtTexStep) +
            texTimeSize + (2 * texSize);
    }

    /* Particles update loop. */
    prtStride = emt->prtClass->objSize;
    prtIn = ((RwChar *)prtBatch) + prtBatch->offset;
    prtOut = prtIn;

    numPrtIn = prtBatch->numPrt;

    while (numPrtIn-- > 0)
    {
        /* Is this particle dead ? */
        if (((RpPrtStdParticleStandard *)prtIn)->currTime <
            ((RpPrtStdParticleStandard *)prtIn)->endTime)
        {
            /* time */
            currTime =
                ((RpPrtStdParticleStandard *)prtIn)->currTime + deltaT;

            /* check for multiple tex co-ords */
            if (rpPrtAdvGlobals.offsetPrtMultiTexStep >= 0)
            {
                prtMultiTexIn = (RpPrtAdvPrtTexStep *)
                    RPPRTADVPROP(prtIn, rpPrtAdvGlobals.offsetPrtMultiTexStep);

                if (prtMultiTexIn->currIndex < emtPrtMultiTex->numTex)
                {
                    multiTime = (RwReal *)
                        (((RwChar *) prtMultiTexIn) +
                            sizeof(RpPrtAdvPrtTexStep));

                    /* Update the prt uv interpolants */
                    if (currTime >=
                            multiTime[prtMultiTexIn->currIndex])
                    {
                        if (rpPrtAdvGlobals.offsetPrtTex >= 0)
                        {
                            prtTexIn = (RpPrtStdParticleTexCoords *)
                                    RPPRTADVPROP(prtIn,
                                                rpPrtAdvGlobals.offsetPrtTex);

                            prtTexIn->startUV0 = *(RwTexCoords *)
                                (((RwChar *) prtMultiTexIn) +
                                 emtPrtMultiTex->offsetStartUV0 +
                                 (prtMultiTexIn->currIndex * sizeof(RwTexCoords)));

                            prtTexIn->startUV1 = *(RwTexCoords *)
                                (((RwChar *) prtMultiTexIn) +
                                 emtPrtMultiTex->offsetStartUV1 +
                                 (prtMultiTexIn->currIndex * sizeof(RwTexCoords)));

                            prtMultiTexIn->currIndex++;
                        }
                    }
                }
            }

            prtOut += prtStride;
        }

        /* Next input prt */
        prtIn += prtStride;
    }

    RWRETURN(prtBatch);
}


/**
 * \ingroup prtadvtexstep
 * \ref RpPrtAdvEmtTexStepEmitCB is the emit callback for the stepped texture
 * coordinate emitter properties.
 *
 * This callback emits new particles using the current emitter. The particle
 * class must contain the stepped texture coordinate properties.
 *
 * This callback should be added after the standard callback.
 *
 * All other properties of the particles should be handled by the previous
 * emit callback in the list.
 *
 * \param atomic        Pointer to the parent \ref RpAtomic.
 * \param emt           Pointer to the \ref RpPrtStdEmitter.
 * \param data          Void pointer to private data for the emit callback. This
 *                      callback expects the data to be a pointer to a
 *                      \ref RwReal representing the delta time elapsed.
 *
 * \return Pointer to the \ref RpPrtStdEmitter if successful, NULL otherwise.
 *
 * \see RpPrtStdAtomicUpdate
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEmitterStdBeginUpdateCB
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtAdvEClassStdCreate
 * \see RpPrtStdEmitterStdCreateCB
 * \see RpPrtStdEmitterStdDestroyCB
 * \see RpPrtStdEmitterStdEndUpdateCB
 * \see RpPrtStdEmitterStdRenderCB
 * \see RpPrtStdPClassSetCallBack
 * \see RpPrtStdPClassStdCreate
 * \see RpPrtAdvPClassStdCreate
 */
RpPrtStdEmitter *
RpPrtAdvEmtTexStepEmitCB(RpAtomic * atomic __RWUNUSED__,
                              RpPrtStdEmitter *emt,
                              void * data __RWUNUSED__)
{
    RwReal          rr;
    RwInt32         ir, numPrt, i, j, k, prtStride;

    RwInt32         prtMultiTexSize = 0;
    RwInt32         texTimeSize = 0;
    RwInt32         texSize = 0;

    RpPrtStdEmitter                     *result;
    RpPrtStdParticleBatch               *prtBatch;

    RpPrtStdEmitterStandard             *emtStd;

    RpPrtAdvEmtPrtMultiTexCoords        *emtPrtMultiTex = NULL;

    RwChar                              *prt;
    RpPrtStdParticleTexCoords           *prtTex;

    RpPrtAdvPrtTexStep                  *prtMultiTex;
    RpPrtStdParticleStandard            *prtStd;

    RwReal                              t0, t1, t2, dt, *time;
    RwTexCoords                         tl_uv1, tl_uv2, *startUV0;
    RwTexCoords                         br_uv1, br_uv2, *startUV1;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtTexStepEmitCB"));

    RWASSERT(atomic);
    RWASSERT(emt);
    RWASSERT(data);

    result = NULL;

    _rpPrtAdvEmitterGetPropOffset(emt);

    RWASSERT(rpPrtAdvGlobals.offsetEmtStd >= 0);
    emtStd = ((RpPrtStdEmitterStandard *)
              RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtStd));

    if (rpPrtAdvGlobals.offsetEmtPrtMultiTexStep >= 0)
    {
        emtPrtMultiTex = ((RpPrtAdvEmtPrtMultiTexCoords *)
             RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtPrtMultiTexStep));

        texTimeSize = emtPrtMultiTex->numTex * sizeof(RwReal);
        texSize = emtPrtMultiTex->numTex * sizeof(RwTexCoords);

        prtMultiTexSize = sizeof(RpPrtAdvPrtTexStep) +
            texTimeSize + (2 * texSize);
    }

    /* Check if we are to emit particles. */
    if (emtStd->currTime >= emtStd->emtEmitTime)
    {
        /* Grab the new particle for the new particles. */
        prtBatch = emt->activeBatch;

        if (prtBatch != NULL && -1 != prtBatch->newPrt)
        {
            /* Number of particles to be emitted. */
            ir = emtStd->seed;
            PRTSTD_SRAND(ir);
            rr = PRTSTD_RSRAND(ir);

            /* Main process loop. */
            numPrt = prtBatch->numPrt - prtBatch->newPrt;
            prtStride = emt->prtClass->objSize;
            prt = ((RwChar *) prtBatch) +
                prtBatch->offset + prtBatch->newPrt*prtStride;

            for (i = 0; i < numPrt; i++)
            {
                prtStd = (RpPrtStdParticleStandard *) prt;

                if( prtStd->currTime == 0 )
                {
                    /* Multiple uvs */
                    if (rpPrtAdvGlobals.offsetPrtMultiTexStep >= 0)
                    {
                        RWASSERT(rpPrtAdvGlobals.offsetPrtTex >= 0);

                        prtTex = ((RpPrtStdParticleTexCoords *)
                            RPPRTADVPROP(prt, rpPrtAdvGlobals.offsetPrtTex));

                        prtMultiTex = ((RpPrtAdvPrtTexStep *)
                            RPPRTADVPROP(prt,
                                    rpPrtAdvGlobals.offsetPrtMultiTexStep));

                        /* Set up the pointers */
                        time = (RwReal *)
                            (((RwChar *) prtMultiTex) +
                                sizeof(RpPrtAdvParticleMultiTexCoordsStep));

                        startUV0 = (RwTexCoords *)
                            (((RwChar *) time) + texTimeSize);
                        startUV1 = (RwTexCoords *)
                            (((RwChar *) startUV0) + texSize);



                        /* Active time */
                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        t1 = emtPrtMultiTex->list[0].time +
                                (rr * emtPrtMultiTex->list[0].timeBias);

                        /* Next uv. */
                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        tl_uv1.u = emtPrtMultiTex->list[0].midUV0.u +
                                (rr * emtPrtMultiTex->list[0].midUV0Bias.u);

                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        tl_uv1.v = emtPrtMultiTex->list[0].midUV0.v +
                                (rr * emtPrtMultiTex->list[0].midUV0Bias.v);

                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        br_uv1.u = emtPrtMultiTex->list[0].midUV1.u +
                                (rr * emtPrtMultiTex->list[0].midUV1Bias.u);

                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        br_uv1.v = emtPrtMultiTex->list[0].midUV1.v +
                                (rr * emtPrtMultiTex->list[0].midUV1Bias.v);

                        t0 = t1;

                        RWASSERT(emtPrtMultiTex->numTex > 0);

                        for (j = 0; j < (RwInt32) (emtPrtMultiTex->numTex - 1); j++)
                        {
                            k = j + 1;

                            /* Active time */
                            PRTSTD_SRAND(ir);
                            rr = PRTSTD_RSRAND(ir);
                            t2 = emtPrtMultiTex->list[k].time +
                                    (rr * emtPrtMultiTex->list[k].timeBias);

                            time[j] = t1 * prtStd->endTime;

                            /* Duration */
                            dt = t2 - t1;
                            if (dt > 0.0)
                                dt = (RwReal) 1.0 / (t2 - t1);
                            else
                                dt = (RwReal) 0.0;

                            /* Next uv. */
                            PRTSTD_SRAND(ir);
                            rr = PRTSTD_RSRAND(ir);
                            tl_uv2.u = emtPrtMultiTex->list[k].midUV0.u +
                                (rr * emtPrtMultiTex->list[k].midUV0Bias.u);

                            PRTSTD_SRAND(ir);
                            rr = PRTSTD_RSRAND(ir);
                            tl_uv2.v = emtPrtMultiTex->list[k].midUV0.v +
                                (rr * emtPrtMultiTex->list[k].midUV0Bias.v);

                            PRTSTD_SRAND(ir);
                            rr = PRTSTD_RSRAND(ir);
                            br_uv2.u = emtPrtMultiTex->list[k].midUV1.u +
                                (rr * emtPrtMultiTex->list[k].midUV1Bias.u);

                            PRTSTD_SRAND(ir);
                            rr = PRTSTD_RSRAND(ir);
                            br_uv2.v = emtPrtMultiTex->list[k].midUV1.v +
                                (rr * emtPrtMultiTex->list[k].midUV1Bias.v);

                            /* Start uv */
                            startUV0[j].u = tl_uv1.u;
                            startUV0[j].v = tl_uv1.v;
                            startUV1[j].u = br_uv1.u;
                            startUV1[j].v = br_uv1.v;

                            /* Next col = next start col */
                            tl_uv1 = tl_uv2;
                            br_uv1 = br_uv2;
                            t1 = t2;
                        }

                        /* Last uv */
                        {
                            /* Duration */
                            t2 = (RwReal) 1.0;

                            time[j] = t1 * prtStd->endTime;

                            /* Start uv */
                            startUV0[j].u = tl_uv1.u;
                            startUV0[j].v = tl_uv1.v;
                            startUV1[j].u = br_uv1.u;
                            startUV1[j].v = br_uv1.v;
                        }

                        prtMultiTex->currIndex = 0;

                        /* Recalculate the first delta uv */
                        {
                            dt = (RwReal) 1.0 / t0;

                            prtTex->deltaUV0.u = 0;
                            prtTex->deltaUV0.v = 0;
                            prtTex->deltaUV1.u = 0;
                            prtTex->deltaUV1.v = 0;
                        }
                    }

                }

                prt += prtStride;
            }

            /* Update the seed. */
            emtStd->seed = ir;
        }

        result = emt;
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
 * \ingroup prtadvtexstep
 * \ref RpPrtAdvEmtTexStepCloneCB is the clone callback for the stepped texture
 * coordinate emitter properties.
 *
 * This callback clones the stepped texture emitter coordinate property of the emitter.
 *
 * This callback should be added \e after the standard callback.
 *
 * All other properties of the particles should be handled by the previous
 * emit callback in the list.
 *
 * \param atomic        Pointer to the parent \ref RpAtomic.
 * \param emt           Pointer to the source \ref RpPrtStdEmitter.
 * \param data          Pointer to the destination \ref RpPrtStdEmitter.
 *
 * \return Pointer to the destination \ref RpPrtStdEmitter if successful, NULL otherwise.
 *
 * \see RpPrtStdAtomicUpdate
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEmitterStdBeginUpdateCB
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtAdvEClassStdCreate
 * \see RpPrtStdEmitterStdCreateCB
 * \see RpPrtStdEmitterStdDestroyCB
 * \see RpPrtStdEmitterStdEndUpdateCB
 * \see RpPrtStdEmitterStdRenderCB
 * \see RpPrtStdPClassSetCallBack
 * \see RpPrtStdPClassStdCreate
 * \see RpPrtAdvPClassStdCreate
 */
RpPrtStdEmitter *
RpPrtAdvEmtTexStepCloneCB(RpAtomic *atomic __RWUNUSEDRELEASE__,
                        RpPrtStdEmitter *emt,
                          void *data )
{
    RwInt32                         srcOffset, dstOffset, size;

    RpPrtStdEmitter                     *dstEmt;
    RpPrtAdvEmtPrtMultiTexCoords    *emtPrtMultiTex = NULL, *dstEmtPrtMultiTex = NULL;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtTexStepCloneCB"));

    RWASSERT(atomic);
    RWASSERT(emt);
    RWASSERT(data);

    dstEmt = (RpPrtStdEmitter *) data;

    srcOffset =
        RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERPRTMULTITEXCOORDSSTEP);
    
    dstOffset =
        RpPrtStdPropTabGetPropOffset(dstEmt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERPRTMULTITEXCOORDSSTEP);

    if ((srcOffset >= 0) && (dstOffset >= 0))
    {
        emtPrtMultiTex = ((RpPrtAdvEmtPrtMultiTexCoords *)
             RPPRTADVPROP(emt, srcOffset));

        dstEmtPrtMultiTex = ((RpPrtAdvEmtPrtMultiTexCoords *)
             RPPRTADVPROP(dstEmt, dstOffset));

        size = sizeof(RpPrtAdvEmtPrtMultiTexCoords) +
               emtPrtMultiTex->numTex * sizeof(RpPrtAdvEmtPrtTexCoordsItem);

        memcpy(dstEmtPrtMultiTex, emtPrtMultiTex, size);

        dstEmtPrtMultiTex->list = (RpPrtAdvEmtPrtTexCoordsItem *)
            ((RwUInt32)(dstEmtPrtMultiTex) + sizeof(RpPrtAdvEmtPrtMultiTexCoords));
    }

    RWRETURN(dstEmt);
}


/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup prtadvtexstep
 * \ref RpPrtAdvEmtTexStepStreamGetSizeCB is the callback used to determine
 * the size, in bytes, of the binary representation of a stepped texture coordinate
 * emitter. This value is used in the binary chunk header to indicate the size
 * of the chunk.
 *
 * This callback should be added to a emitter class if the emitter contains
 * the stepped texture coordinate emitter property.
 *
 * \param atomic            A pointer to the emitter's parent \ref RpAtomic.
 * \param emt               A pointer to the \ref RpPrtStdEmitter.
 * \param data              Void pointer to a \ref RwInt32 to return the stream
 *                          size.
 *
 * \return Pointer to the \ref RpPrtStdEmitter if successful, NULL otherwise.
 *
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtStdEmitterStdStreamGetSizeCB
 * \see RpPrtStdEmitterStdStreamReadCB
 * \see RpPrtStdEmitterStdStreamWriteCB
 * \see RpPrtAdvEmtTexStepStreamReadCB
 * \see RpPrtAdvEmtTexStepStreamWriteCB
 */
RpPrtStdEmitter *
RpPrtAdvEmtTexStepStreamGetSizeCB(RpAtomic * atomic __RWUNUSED__,
                                       RpPrtStdEmitter *emt,
                                       void * data)
{
    RwInt32                          totalSize;
    RwInt32                          offset;

    RpPrtStdPropertyTable           *propTab;
    RpPrtStdEmitterClass            *eClass;

    RpPrtAdvEmtPrtMultiTexCoords   *emtPrtMultiTex;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtTexStepStreamGetSizeCB"));

    RWASSERT(emt);
    RWASSERT(data);

    totalSize = 0;

    eClass = emt->emtClass;
    propTab = eClass->propTab;

    /* Get size of the multi tex prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERPRTMULTITEXCOORDSSTEP);

    if (offset >= 0)
    {
        emtPrtMultiTex = ((RpPrtAdvEmtPrtMultiTexCoords *)
                  RPPRTADVPROP(emt, offset));

        /* Number of uv variations */
        totalSize += sizeof(RwInt32);

        /* Item array */
        totalSize += sizeof(RpPrtAdvEmtPrtTexCoordsItem)*
                            emtPrtMultiTex->numTex;
    };

    *(RwInt32 *) data = totalSize;

    RWRETURN(emt);
}


/**
 * \ingroup prtadvtexstep
 * \ref RpPrtAdvEmtTexStepStreamReadCB is the stream in callback for
 * the stepped texture coordinate particle property.
 *
 * This function streams in an emitter containing the stepped texture
 * coordinate property.
 *
 * This callback should be added to a emitter class if the emitter contains
 * any of the stepped texture coordinate emitter properties.
 *
 * \param atomic            A pointer to the emitter's parent \ref RpAtomic.
 * \param emt               A pointer to the \ref RpPrtStdEmitter to stream into.
 * \param data              Void pointer to the input stream.
 *
 * \return Pointer to the \ref RpPrtStdEmitter if successful, NULL otherwise.
 *
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtStdEmitterStdStreamGetSizeCB
 * \see RpPrtStdEmitterStdStreamReadCB
 * \see RpPrtStdEmitterStdStreamWriteCB
 * \see RpPrtAdvEmtTexStepStreamGetSizeCB
 * \see RpPrtAdvEmtTexStepStreamWriteCB
 */
RpPrtStdEmitter *
RpPrtAdvEmtTexStepStreamReadCB(RpAtomic * atomic __RWUNUSED__,
                                    RpPrtStdEmitter *emt, void *data)
{
    RwInt32                         offset;
    RwStream                        *stream;

    RpPrtStdPropertyTable           *propTab;
    RpPrtStdEmitterClass            *eClass;

    RpPrtAdvEmtPrtMultiTexCoords    *emtPrtMultiTex;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtTexStepStreamReadCB"));

    RWASSERT(emt);
    RWASSERT(data);

    stream = (RwStream *) data;

    eClass = emt->emtClass;
    propTab = eClass->propTab;

    /* Get size of the multi color prop. */
    /* Get size of the multi tex prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERPRTMULTITEXCOORDSSTEP);

    if (offset >= 0)
    {
        emtPrtMultiTex = ((RpPrtAdvEmtPrtMultiTexCoords *)
                  RPPRTADVPROP(emt, offset));

        /* Number of variations */
        RwStreamReadInt32(stream,
                          (RwInt32*)&emtPrtMultiTex->numTex,
                          sizeof(RwInt32));

        if (emtPrtMultiTex->numTex > 0)
        {
            RwStreamReadReal(stream,
                                (RwReal*)emtPrtMultiTex->list,
                                sizeof(RpPrtAdvEmtPrtTexCoordsItem)*
                                emtPrtMultiTex->numTex);

            /* The data structures were changed in size, so we
             * need to re-adjust some offsets values.
             */
            if (_rpPrtStdStreamGetVersion() < rpPRTSTDVERSIONSTAMP)
            {
                emtPrtMultiTex->offsetTime = sizeof(RpPrtAdvPrtTexStep);
                emtPrtMultiTex->offsetStartUV0 = emtPrtMultiTex->offsetTime +
                        (emtPrtMultiTex->numTex * sizeof(RwReal));
                emtPrtMultiTex->offsetStartUV1 = emtPrtMultiTex->offsetStartUV0 +
                        (emtPrtMultiTex->numTex * sizeof(RwTexCoords));
            }
        }
    }

    RWRETURN(emt);
}

/**
 * \ingroup prtadvtexstep
 * \ref RpPrtAdvEmtTexStepStreamWriteCB is the stream out callback for
 * the stepped texture coordinate particle property.
 *
 * This function streams out an emitter containing the stepped texture
 * coordinate property.
 *
 * This callback should be added to a emitter class if the emitter contains
 * the stepped texture coordinate emitter property.
 *
 * \param atomic            A pointer to the emitter's parent \ref RpAtomic.
 * \param emt               A pointer to the \ref RpPrtStdEmitter.
 * \param data              Void pointer to output stream.
 *
 * \return Pointer to the \ref RpPrtStdEmitter if successful, NULL otherwise.
 *
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtStdEmitterStdStreamGetSizeCB
 * \see RpPrtStdEmitterStdStreamReadCB
 * \see RpPrtStdEmitterStdStreamWriteCB
 * \see RpPrtAdvEmtTexStepStreamGetSizeCB
 * \see RpPrtAdvEmtTexStepStreamReadCB
 */
RpPrtStdEmitter *
RpPrtAdvEmtTexStepStreamWriteCB(RpAtomic * atomic __RWUNUSED__,
                                     RpPrtStdEmitter *emt, void *data)
{
    RwInt32                         offset;
    RwStream                        *stream;

    RpPrtStdPropertyTable           *propTab;
    RpPrtStdEmitterClass            *eClass;

    RpPrtAdvEmtPrtMultiTexCoords   *emtPrtMultiTex;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtTexStepStreamWriteCB"));

    RWASSERT(emt);
    RWASSERT(data);

    stream = (RwStream *) data;

    eClass = emt->emtClass;
    propTab = eClass->propTab;

    /* Get size of the multi tex step prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERPRTMULTITEXCOORDSSTEP);

    if (offset >= 0)
    {
        emtPrtMultiTex = ((RpPrtAdvEmtPrtMultiTexCoords *)
                  RPPRTADVPROP(emt, offset));

        /* Number of variations */
        RwStreamWriteInt32(stream,
                           (RwInt32*)&emtPrtMultiTex->numTex,
                           sizeof(RwInt32));

        if (emtPrtMultiTex->numTex > 0)
        {
            RwStreamWriteReal(stream,
                                (RwReal*)emtPrtMultiTex->list,
                                sizeof(RpPrtAdvEmtPrtTexCoordsItem)*
                                emtPrtMultiTex->numTex);
        }
    }

    RWRETURN(emt);
}


/**
 * \ingroup prtadvtexstep
 * \ref RpPrtAdvEmtTexStepCreateCB is the create callback for the stepped
 * texture coordinate emitter property.
 *
 * This callback setups the stepped texture coordinate emitter properties of an emitter.
 *
 * This callback should be added to an emitter class if the emitter contains
 * the stepped texture coordinate emitter properties.
 *
 * \param atomic        Pointer to the parent \ref RpAtomic.
 * \param emt           Pointer to the \ref RpPrtStdEmitter.
 * \param data          Void pointer to private data for the emit callback.
 *
 * \return Pointer to the \ref RpPrtStdEmitter if successful, NULL otherwise.
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
RpPrtAdvEmtTexStepCreateCB(RpAtomic * atomic __RWUNUSED__,
                                RpPrtStdEmitter *emt,
                                void * data __RWUNUSED__)
{
    RwInt32 offset,idx,size;

    RpPrtAdvEmtPrtMultiTexCoords        * emtPrtMultiTex;

    RwInt32 *propSize;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtTexStepCreateCB"));

    offset = RpPrtStdPropTabGetPropOffset(
    emt->emtClass->propTab, rpPRTADVPROPERTYCODEPARTICLEMULTITEXCOORDSSTEP);

    if( offset > 0 )
    {
        emtPrtMultiTex = (RpPrtAdvEmtPrtMultiTexCoords *)
            (((RwChar *)emt) + offset);


        idx = RpPrtStdPropTabGetPropIndex(
                            emt->emtClass->propTab,
                            rpPRTADVPROPERTYCODEPARTICLEMULTITEXCOORDSSTEP);

        RpPrtStdPropTabGetProperties(emt->emtClass->propTab,
                                            NULL,
                                            NULL,
                                            NULL,
                                            &propSize);

        size = propSize[idx];

        size -= sizeof(RpPrtAdvEmtPrtMultiTexCoords);
        emtPrtMultiTex->numTex =
                                size / sizeof(RpPrtAdvEmtPrtTexCoordsItem);

        emtPrtMultiTex->list = (RpPrtAdvEmtPrtTexCoordsItem*)(
                                (RwUInt32)(emtPrtMultiTex) +
                                sizeof(RpPrtAdvEmtPrtMultiTexCoords));

        /* Offsets into the particle's multitexcoords props. The offsets are from the
         * start of the property, not from the particle
         */
        emtPrtMultiTex->offsetTime = sizeof(RpPrtAdvPrtTexStep);
        emtPrtMultiTex->offsetStartUV0 = emtPrtMultiTex->offsetTime +
                (emtPrtMultiTex->numTex * sizeof(RwReal));
        emtPrtMultiTex->offsetStartUV1 = emtPrtMultiTex->offsetStartUV0 +
                (emtPrtMultiTex->numTex * sizeof(RwTexCoords));

        /* Deltas does not exist for tex step. */
        emtPrtMultiTex->offsetDeltaUV0 = 0;
        emtPrtMultiTex->offsetDeltaUV1 = 0;
    }

    RWRETURN(emt);
}
