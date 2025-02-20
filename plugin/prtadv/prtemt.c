/**********************************************************************
 *
 * File :     prtemt.c
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

#include <rwcore.h>
#include <rpworld.h>
#include <rpptank.h>
#include <rpprtstd.h>

#include <rpplugin.h>
#include <rpdbgerr.h>

#include "rpprtadv.h"

#include "prtemt.h"
#include "prtadvvars.h"

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
 * \ingroup prtadvprtemitter
 * \ref RpPrtAdvPrtEmtUpdateCB is the update callback for the sub-emitter
 * particle property.
 *
 * This callback updates the sub-emitter properties in a particle.
 *
 * This callback should be added to a particle class if the particle contains
 * the particle's sub-emitter property.
 *
 * The callback must be placed so that it is called before the standard update
 * callback.
 *
 * \param emt           Pointer to the parent \ref RpPrtStdEmitter.
 * \param prtBatch      Pointer to the \ref RpPrtStdParticleBatch to be updated.
 * \param data          Void pointer to private data for the update callback. This
 * callback expects the data to be a pointer to a \ref RwReal representing
 * the delta time elapsed.
 *
 * \return Pointer to the \ref RpPrtStdParticleBatch if successful, NULL otherwise.
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
RpPrtAdvPrtEmtUpdateCB(RpPrtStdEmitter *emt,
                                 RpPrtStdParticleBatch *prtBatch,
                                 void *data)
{
    RwReal                          currTime, emitTime;

    RpAtomic                        *atomic;
    RwV3d                           v1, v2;
    RwInt32                         i;
    RwInt32                         numPrtIn;
    RwInt32                         numPrtOut;

    RwFrame                         *frame;
    RwMatrix                        mtx, atmMtx, *ltm;

    RpPrtStdEmitter                 *subEmt=NULL;
    RpPrtStdEmitterStandard         *emtStd;

    RwInt32                         mainEmtVelOffset;
    RwInt32                         mainEmtPrtEmtOffset;
    RwInt32                         mainEmtPrtStdOffset;

    RpPrtStdEmitterStandard         *emtStd2;
    RpPrtStdEmitterPTank            *emtPTank;
    RpPrtAdvEmtPrtEmt               *emtPrtEmt=NULL;

    RwChar                          *prtIn,
                                    *prtOut;
    RpPrtStdParticleStandard        *prtStd;
    RwV3d                           *prtVel;
    RpPrtAdvPrtEmitter              *prtAdvIn;

    RwChar                          *pTankPosIn;

    RwInt32                         prtSize;
    RwInt32                         pTankPosStride = 0;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvPrtEmtUpdateCB"));

    RWASSERT(emt);
    RWASSERT(prtBatch);

    atomic = emt->atomic;

    /* Get the emitter's properties */
    _rpPrtAdvEmitterGetPropOffset(emt);

    emtPTank = NULL;

    RWASSERT(rpPrtAdvGlobals.offsetEmtStd >= 0);

    emtStd = ((RpPrtStdEmitterStandard *)
               RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtStd));

    if (rpPrtAdvGlobals.offsetEmtPTank >= 0)
        emtPTank = ((RpPrtStdEmitterPTank *)
                    RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtPTank));

    if (rpPrtAdvGlobals.offsetEmtPrtEmitter >= 0)
    {
        emtPrtEmt = ((RpPrtAdvEmtPrtEmt *)
                    RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtPrtEmitter));

        if(emtPrtEmt->emitter == NULL)
        {
            emtPrtEmt->emitter = emt->next;
        }

        subEmt = emtPrtEmt->emitter;

        RWASSERT(subEmt);
    }

    if (subEmt->emtClass->propTab != rpPrtAdvGlobals.emtPropTab2)
    {
        rpPrtAdvGlobals.offsetEmtStd2 =
            RpPrtStdPropTabGetPropOffset(subEmt->emtClass->
                propTab,
                rpPRTSTDPROPERTYCODEEMITTERSTANDARD);

        rpPrtAdvGlobals.emtPropTab2 = subEmt->emtClass->propTab;
    }

    emtStd2 = ((RpPrtStdEmitterStandard *)
           RPPRTADVPROP(subEmt, rpPrtAdvGlobals.offsetEmtStd2));

    mainEmtVelOffset = rpPrtAdvGlobals.offsetPrtVel;
    mainEmtPrtEmtOffset = rpPrtAdvGlobals.offsetPrtEmitter;
    mainEmtPrtStdOffset = rpPrtAdvGlobals.offsetPrtStd;

    if (emtPTank)
    {
        /* Position. */
        i = 0;
        if (emtPTank->dataFlags & rpPTANKDFLAGPOSITION)
        {
            pTankPosIn = (RwChar *) emtPTank->dataInPtrs[i];
            pTankPosStride = emtPTank->dataStride[i];

            RWASSERT(pTankPosIn);

            i++;
        }
        else
        {
            pTankPosIn = NULL;
        }

        /* Particles update loop. */
        prtSize = emt->prtClass->objSize;
        prtIn = ((RwChar *)prtBatch) + prtBatch->offset;
        prtOut = prtIn;

        /* Main update loop. */
        numPrtIn = prtBatch->numPrt;
        numPrtOut = 0;

        /* Replace the atomic's frame. */
        frame = RpAtomicGetFrame(atomic);
        atmMtx = *RwFrameGetMatrix(frame);

        while (numPrtIn-- > 0)
        {
            if (((RpPrtStdParticleStandard *)prtIn)->currTime <
                ((RpPrtStdParticleStandard *)prtIn)->endTime)
            {
                prtStd = (RpPrtStdParticleStandard *)
                    RPPRTADVPROP(prtIn, mainEmtPrtStdOffset);

                prtAdvIn = (RpPrtAdvPrtEmitter *)
                    RPPRTADVPROP(prtIn, mainEmtPrtEmtOffset);

                /* Do we emit this emitter ? */
                if (prtStd->currTime > prtAdvIn->emtEmitTime)
                {
                    subEmt = emtPrtEmt->emitter;
                    RWASSERT(subEmt);

                    prtVel = (RwV3d *)
                        RPPRTADVPROP(prtIn, mainEmtVelOffset);

                    RwV3dNormalize(&mtx.at, prtVel);

                    v1.x = (RwReal) 0.0;
                    v1.y = (RwReal) 0.0;
                    v1.z = (RwReal) 0.0;

                    v2.x = (RwReal) RwFabs(mtx.at.x);
                    v2.y = (RwReal) RwFabs(mtx.at.y);
                    v2.z = (RwReal) RwFabs(mtx.at.z);

                    if (v2.x <= v2.y)
                    {
                        if (v2.x <= v2.z)
                            v1.x = (RwReal) 1.0;
                        else
                            v1.z = (RwReal) 1.0;
                    }
                    else
                    {
                        if (v2.y <= v2.z)
                            v1.y = (RwReal) 1.0;
                        else
                            v1.z = (RwReal) 1.0;
                    }

                    RwV3dCrossProduct(&mtx.right, &v1, &mtx.at);
                    RwV3dCrossProduct(&mtx.up, &mtx.at, &mtx.right);

                    mtx.pos = *(RwV3d *) pTankPosIn;

                    RwFrameTransform(frame, &mtx, rwCOMBINEREPLACE);

                    /* Emit from the emitter. */

                    emtStd2->ltm = RwFrameGetLTM(frame);

                    currTime = emtStd2->currTime;
                    emitTime = emtStd2->emtEmitTime;

                    emtStd2->currTime = prtStd->currTime;
                    emtStd2->emtEmitTime = prtStd->currTime;

                    /* Set to emit */
                    subEmt->flag |= rpPRTSTDEMITTERFLAGEMIT;
                    subEmt->flag |= rpPRTSTDEMITTERFLAGNOBUFFERSWAP;
                    subEmt->flag &= ~rpPRTSTDEMITTERFLAGUPDATEPARTICLE;

                    _rpPrtStdEmitterUpdate(atomic, subEmt,data);

                    /* And Clear it */
                    subEmt->flag &= ~rpPRTSTDEMITTERFLAGEMIT;
                    subEmt->flag &= ~rpPRTSTDEMITTERFLAGNOBUFFERSWAP;
                    subEmt->flag |= rpPRTSTDEMITTERFLAGUPDATEPARTICLE;

                    emtStd2->currTime = currTime;
                    emtStd2->emtEmitTime = emitTime;

                    /* Update the next emission time */
                    prtAdvIn->emtEmitTime += prtAdvIn->emtEmitTimeGap;
                }

                prtOut += prtSize;
            }

            prtIn += prtSize;
            pTankPosIn += pTankPosStride;
        }

        /* Restore the atomic's frame */
        RwFrameTransform(frame, &atmMtx, rwCOMBINEREPLACE);

        RwFrameUpdateObjects(frame);

        ltm = RwFrameGetLTM(frame);

        _rpPrtStdEmitterGetPropOffset(emt);
    }



    RWRETURN(prtBatch);
}


/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup prtadvprtemitter
 * \ref RpPrtAdvEmtPrtEmtEmitCB is the emit callback for the sub-emitter
 * emitter properties.
 *
 * This callback emits new particles using the current emitter. The particle
 * class must contain the sub-emitter property.
 *
 * This callback should be added \e after the standard callback.
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
RpPrtAdvEmtPrtEmtEmitCB(RpAtomic *atomic __RWUNUSED__,
                        RpPrtStdEmitter *emt,
                          void *data __RWUNUSED__)
{
    RwReal          rr;
    RwInt32         ir, numPrtOut;

    RpPrtStdEmitterStandard         *emtStd;
    RpPrtAdvEmtPrtEmt       *emtPrtEmt = NULL;

    RwChar                          *prtIn;

    RpPrtStdParticleStandard        *prtStd;
    RpPrtAdvPrtEmitter              *prtAdv;

    RpPrtStdParticleBatch           *prtBatch;

    RwInt32                         prtSize;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtPrtEmtEmitCB"));

    RWASSERT(emt);

    /* Get the emitter's properties */
    _rpPrtAdvEmitterGetPropOffset(emt);

    RWASSERT(rpPrtAdvGlobals.offsetEmtStd >= 0);

    emtStd = ((RpPrtStdEmitterStandard *)
        RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtStd));

    if (rpPrtAdvGlobals.offsetEmtPrtEmitter >= 0)
        emtPrtEmt = ((RpPrtAdvEmtPrtEmt *)
            RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtPrtEmitter));

    /* Random seed */
    ir = emtStd->seed;

    /* Particles update loop. Assume the last batch is the most recent so
     * would contain the newly emitted prts */
    prtBatch = emt->activeBatch;

    if (prtBatch != NULL && -1 != prtBatch->newPrt)
    {

        prtSize = emt->prtClass->objSize;

        /* Set up the particle emission data for new emitted particles */
        if (prtBatch->newPrt >= 0)
        {
            numPrtOut = prtBatch->numPrt - prtBatch->newPrt;

            /* Start of prts in the batch */
            prtIn = ((RwChar *)prtBatch) + prtBatch->offset;

            /* Start of newly emitted prts */
            prtIn += (prtBatch->newPrt * prtSize);

            while (numPrtOut-- > 0)
            {
                /* Set up the data for this particle emitter */
                prtStd = (RpPrtStdParticleStandard *)
                    RPPRTADVPROP(prtIn, rpPrtAdvGlobals.offsetPrtStd);

                prtAdv = (RpPrtAdvPrtEmitter *)
                    RPPRTADVPROP(prtIn, rpPrtAdvGlobals.offsetPrtEmitter);
                RWASSERT(prtAdv);

                PRTSTD_SRAND(ir);
                rr = PRTSTD_RSRAND(ir);
                prtAdv->emtEmitTime = prtStd->currTime +
                    emtPrtEmt->time + (rr * emtPrtEmt->timeBias);

               PRTSTD_SRAND(ir);
                rr = PRTSTD_RSRAND(ir);
                prtAdv->emtEmitTimeGap =
                    emtPrtEmt->timeGap + (rr * emtPrtEmt->timeGapBias);

                prtIn += prtSize;
            }


            /* Update the seed. */
            emtStd->seed = ir;
        }
    }

    RWRETURN(emt);
}

/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup prtadvprtemitter
 * \ref RpPrtAdvEmtPrtEmtCloneCB is the clone callback for the sub-emitter
 * emitter properties.
 *
 * This callback clones the sub emitter property of the emitter. The sub emitter is
 * not copied and is set NULL for the destination emitter. This is because it maybe
 * unsafe to use the sub-emitter in the source emitter.
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
RpPrtAdvEmtPrtEmtCloneCB(RpAtomic *atomic __RWUNUSED__,
                         RpPrtStdEmitter *emt,
                         void *data )
{
    RwInt32                         srcOffset, dstOffset;

    RpPrtStdEmitter                 *dstEmt;
    RpPrtAdvEmtPrtEmt               *emtPrtEmt = NULL, *dstEmtPrtEmt;


    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtPrtEmtCloneCB"));

    RWASSERT(emt);

    dstEmt = (RpPrtStdEmitter *) data;

    srcOffset =
        RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERPRTEMITTER);

    dstOffset =
        RpPrtStdPropTabGetPropOffset(dstEmt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERPRTEMITTER);

    if ((srcOffset >= 0) && (dstOffset >= 0))
    {
        emtPrtEmt = ((RpPrtAdvEmtPrtEmt *)
            RPPRTADVPROP(emt, srcOffset));

        dstEmtPrtEmt = ((RpPrtAdvEmtPrtEmt *)
            RPPRTADVPROP(dstEmt, dstOffset));

        memcpy(dstEmtPrtEmt, emtPrtEmt, sizeof(RpPrtAdvEmtPrtEmt));

        /* Need to set the sub emitter to NULL because it is
         * unsafe to use for the clone emitter */
        dstEmtPrtEmt->emitter = NULL;
    }

    RWRETURN(dstEmt);
}

/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup prtadvprtemitter
 * \ref RpPrtAdvEmtPrtEmtStreamGetSizeCB is the callback used to
 * determine the size, in bytes, of the binary representation of a sub-emitter
 * emitter. This value is used in the binary chunk header to indicate the size of the chunk.
 *
 * This callback should be added to a emitter class if the emitter contains
 * the sub-emitter property.
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
 * \see RpPrtAdvEmtPrtEmtStreamReadCB
 * \see RpPrtAdvEmtPrtEmtStreamWriteCB
 */
RpPrtStdEmitter *
RpPrtAdvEmtPrtEmtStreamGetSizeCB(RpAtomic * atomic __RWUNUSED__,
                                       RpPrtStdEmitter *emt,
                                       void * data)
{
    RwInt32                         offset,totalSize;

    RpPrtStdPropertyTable           *propTab;
    RpPrtStdEmitterClass            *eClass;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtPrtEmtStreamGetSizeCB"));

    RWASSERT(emt);
    RWASSERT(data);

    eClass = emt->emtClass;
    propTab = eClass->propTab;

    /* Get size of the multi color prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERPRTEMITTER);


    totalSize = 0;
    if( offset > 0 )
    {
        totalSize += 4*sizeof(RwReal);
    }

    *(RwInt32 *) data = totalSize;

    RWRETURN(emt);
}

/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup prtadvprtemitter
 * \ref RpPrtAdvEmtPrtEmtStreamReadCB is the stream in callback for
 * the sub-emitter particle property.
 *
 * This function streams in an emitter containing the sub-emitter property.
 *
 * This callback should be added to an emitter class if the emitter contains
 * the sub-emitter emitter property.
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
 * \see RpPrtAdvEmtPrtEmtStreamGetSizeCB
 * \see RpPrtAdvEmtPrtEmtStreamWriteCB
 */
RpPrtStdEmitter *
RpPrtAdvEmtPrtEmtStreamReadCB(RpAtomic * atomic __RWUNUSED__,
                                       RpPrtStdEmitter *emt,
                                       void * data)
{
    RwInt32                         offset;
    RwStream                        *stream;

    RpPrtStdPropertyTable           *propTab;
    RpPrtStdEmitterClass            *eClass;
    RpPrtAdvEmtPrtEmt       *emtPrtEmt;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtPrtEmtStreamReadCB"));

    RWASSERT(emt);
    RWASSERT(data);

    stream = (RwStream *) data;

    eClass = emt->emtClass;
    propTab = eClass->propTab;

    /* Get size of the multi color prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERPRTEMITTER);

    if (offset >= 0)
    {
        emtPrtEmt = ((RpPrtAdvEmtPrtEmt *)
                    RPPRTADVPROP(emt, offset));

        RwStreamReadReal(stream,&emtPrtEmt->time,sizeof(RwReal));
        RwStreamReadReal(stream,&emtPrtEmt->timeBias,sizeof(RwReal));
        RwStreamReadReal(stream,&emtPrtEmt->timeGap,sizeof(RwReal));
        RwStreamReadReal(stream,&emtPrtEmt->timeGapBias,sizeof(RwReal));
    }

    RWRETURN(emt);
}


/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup prtadvprtemitter
 * \ref RpPrtAdvEmtPrtEmtStreamWriteCB is the stream out callback for
 * the sub-emitter particle property.
 *
 * This function streams out an emitter containing the sub-emitter property.
 *
 * This callback should be added to an emitter class if the emitter contains
 * the sub-emitter emitter property.
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
 * \see RpPrtAdvEmtPrtEmtStreamGetSizeCB
 * \see RpPrtAdvEmtPrtEmtStreamReadCB
 */
RpPrtStdEmitter *
RpPrtAdvEmtPrtEmtStreamWriteCB(RpAtomic * atomic __RWUNUSED__,
                                       RpPrtStdEmitter *emt,
                                       void * data)
{
    RwInt32                         offset;
    RwStream                        *stream;

    RpPrtStdPropertyTable           *propTab;
    RpPrtStdEmitterClass            *eClass;
    RpPrtAdvEmtPrtEmt       *emtPrtEmt;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtPrtEmtStreamWriteCB"));

    RWASSERT(emt);
    RWASSERT(data);

    stream = (RwStream *) data;

    eClass = emt->emtClass;
    propTab = eClass->propTab;

    /* Get size of the multi color prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERPRTEMITTER);

    if (offset >= 0)
    {
        emtPrtEmt = ((RpPrtAdvEmtPrtEmt *)
                    RPPRTADVPROP(emt, offset));

        RwStreamWriteReal(stream,&emtPrtEmt->time,sizeof(RwReal));
        RwStreamWriteReal(stream,&emtPrtEmt->timeBias,sizeof(RwReal));
        RwStreamWriteReal(stream,&emtPrtEmt->timeGap,sizeof(RwReal));
        RwStreamWriteReal(stream,&emtPrtEmt->timeGapBias,sizeof(RwReal));
    }

    RWRETURN(emt);
}

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


