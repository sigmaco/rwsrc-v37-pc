/**********************************************************************
 *
 * File :     circle.c
 *
 * Abstract : Circular emitter
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

#include "rpprtstd.h"
#include "rpprtadv.h"

#include "circle.h"
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

#define MATRIXSUPPORTx

/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup prtadvcircle
 * \ref RpPrtAdvEmtCircleEmitCB is the emit callback for the circular
 * emitter property.
 *
 * This callback emits new particles using the current emitter. The particle
 * class must contain the circular particle property.
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
RpPrtAdvEmtCircleEmitCB(RpAtomic *atomic __RWUNUSEDRELEASE__,
                        RpPrtStdEmitter *emt,
                          void *data __RWUNUSEDRELEASE__)
{
    RwMatrix        *ltm;
    RwV3d           v;
#ifdef MATRIXSUPPORT
    RwV3d           *vptr,v2;
#endif /* MATRIXSUPPORT */

    RwReal          rr;
    RwInt32         ir, numPrt, i, prtStride;

    RpPrtStdEmitter                 *result;
    RpPrtStdParticleBatch           *prtBatch;

    RwChar                         *pTankPosOut = (RwChar *)NULL;

    RwInt32                         pTankPosStride = 0;

    RpPrtStdEmitterStandard         *emtStd;
    RpPrtStdEmitterPTank            *emtPTank;
    RpPrtAdvEmtCircle           *emtCircle = NULL;
#ifdef MATRIXSUPPORT
    RpPrtStdEmitterPrtMatrix        *emtPrtMtx =
                                            (RpPrtStdEmitterPrtMatrix *)NULL;
#endif /* MATRIXSUPPORT */

    RwChar                          *prt;
    RpPrtStdParticleStandard        *prtStd;
#ifdef MATRIXSUPPORT
    RwMatrix                        *prtMtx;
#endif /* MATRIXSUPPORT */

    RwV3d                           *prtPos, *prtVel, pos;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtCircleEmitCB"));

    RWASSERT(atomic);
    RWASSERT(emt);
    RWASSERT(data);

    /* Get the emitter's properties */
    _rpPrtAdvEmitterGetPropOffset(emt);

    result = NULL;

    emtPTank = NULL;

    RWASSERT(rpPrtAdvGlobals.offsetEmtStd >= 0);
    emtStd = ((RpPrtStdEmitterStandard *)
              RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtStd));

    if (rpPrtAdvGlobals.offsetEmtPTank >= 0)
        emtPTank = ((RpPrtStdEmitterPTank *)
                    RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtPTank));

#ifdef MATRIXSUPPORT
    if (rpPrtAdvGlobals.offsetEmtPrtMtx >= 0)
        emtPrtMtx = ((RpPrtStdEmitterPrtMatrix *)
                    RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtPrtMtx));
#endif /* MATRIXSUPPORT */

    if (rpPrtAdvGlobals.offsetEmtCircle >= 0)
    {
        emtCircle = ((RpPrtAdvEmtCircle *)
                    RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtCircle));
    }

    /* Check if we are to emit particles. */
    prtBatch = emt->activeBatch;

    /* Get the emitter props. */
    /* Number of particles to be emitted. */

    if (prtBatch != NULL && -1 != prtBatch->newPrt)
    {
        numPrt = prtBatch->numPrt - prtBatch->newPrt;

        /* Clamp to maximum possible particles. */
        if (emtPTank)
        {
            /* Position.  */
            i = 0;
            pTankPosOut = NULL;
            pTankPosStride = 0;
            if (emtPTank->dataFlags & rpPTANKDFLAGPOSITION)
            {
                pTankPosStride = emtPTank->dataStride[i];
                pTankPosOut = emtPTank->dataOutPtrs[i] - numPrt*pTankPosStride;

                i++;
            }


            /* Matrix. */
            if (emtPTank->dataFlags & rpPTANKDFLAGMATRIX)
            {
                pTankPosStride = emtPTank->dataStride[i];
                pTankPosOut = emtPTank->dataOutPtrs[i] - numPrt*pTankPosStride;

                RWASSERT(pTankPosOut);

                i++;
            }

        }

        ltm = emtStd->ltm;
        ir = emtStd->seed;

        /* Main process loop. */
        prtStride = emt->prtClass->objSize;
        prt = ((RwChar *) prtBatch) + prtBatch->offset
                + prtBatch->newPrt * prtStride;

        for (i = 0; i < numPrt; i++)
        {
            prtStd = (RpPrtStdParticleStandard *) prt;

            /* Initial velocity & pos. */
            if (rpPrtAdvGlobals.offsetPrtVel >= 0)
            {
                RwReal Angle;
                RwReal SinA,CosA;
                RwReal SinB,CosB;

                PRTSTD_SRAND(ir);
                Angle = PRTSTD_RSRAND(ir) * _RW_pi;

                SinA = (RwReal)RwSin(Angle);
                CosA = (RwReal)RwCos(Angle);

                /* Initial pos. */
                PRTSTD_SRAND(ir);
                rr = PRTSTD_2RSRAND2(ir)+0.5f;
                v.x = SinA*(emtCircle->radius + rr*emtCircle->radiusGap);

                v.y = CosA*(emtCircle->radius + rr*emtCircle->radiusGap);

                PRTSTD_SRAND(ir);
                rr = PRTSTD_RSRAND(ir);
                v.z = emtCircle->height*rr;

                /* Transform to world space. */
                if (ltm)
                    RwV3dTransformPoint(&pos, &v, ltm);

                if(TRUE == emtCircle->useCircleEmission)
                {
                    prtVel = (RwV3d*)RPPRTADVPROP(prt,
                                                rpPrtAdvGlobals.offsetPrtVel);

                    SinB = (RwReal)RwSin(emtCircle->dirRotation);
                    CosB = (RwReal)RwCos(emtCircle->dirRotation);


                    PRTSTD_SRAND(ir);
                    rr = PRTSTD_RSRAND(ir);
                    v.x = SinB*SinA + rr*emtStd->prtInitDirBias.x;

                    PRTSTD_SRAND(ir);
                    rr = PRTSTD_RSRAND(ir);
                    v.y = SinB*CosA + rr*emtStd->prtInitDirBias.y;

                    PRTSTD_SRAND(ir);
                    rr = PRTSTD_RSRAND(ir);
                    v.z = CosB + rr*emtStd->prtInitDirBias.z;

                    RwV3dNormalize(&v, &v);

                    /* Transform the vec to world space. */
                    if (ltm)
                        RwV3dTransformVector(&v, &v, ltm);

                    /* Scale to initial velocity. */
                    PRTSTD_SRAND(ir);
                    rr = PRTSTD_RSRAND(ir);
                    RwV3dScale((RwV3d *)prtVel, &v,
                        (emtStd->prtInitVel + (rr * emtStd->prtInitVelBias)));
                }

            }

            if (rpPrtAdvGlobals.offsetPrtPos >= 0)
            {
                prtPos = (RwV3d *)
                    RPPRTADVPROP(prt, rpPrtAdvGlobals.offsetPrtPos);

                prtPos->x = pos.x;
                prtPos->y = pos.y;
                prtPos->z = pos.z;
            }

#ifdef MATRIXSUPPORT
            if (rpPrtAdvGlobals.offsetPrtMtx >= 0)
            {
                prtMtx = (RwMatrix *)
                    RPPRTADVPROP(prt, rpPrtAdvGlobals.offsetPrtMtx);

                RwMatrixSetIdentity(prtMtx);
            }
#endif /* MATRIXSUPPORT */



            /* PTank */
            if (emtPTank)
            {
                /* Initial pos. */
                if (emtPTank->dataFlags & emtPTank->updateFlags &
                    rpPTANKDFLAGPOSITION)
                {
                    ((RwV3d *)pTankPosOut)->x = pos.x;
                    ((RwV3d *)pTankPosOut)->y = pos.y;
                    ((RwV3d *)pTankPosOut)->z = pos.z;
#if (defined(SKY2_DRVMODEL_H) && defined(SKY_USE_STOP_FLAG))
                    ((RwV4d *)pTankPosOut)->w = 0.0f;
#endif

                    pTankPosOut += pTankPosStride;
                }
            }

            prt += prtStride;
        }

        /* Update the seed. */
        emtStd->seed = ir;

        result = emt;
    }



    RWRETURN(emt);
}

/**
 * \ingroup prtadvcircle
 * \ref RpPrtAdvEmtCircleCloneCB is the clone callback for the circular
 * emitter property.
 *
 * This callback clones the circular emitter property of the emitter.
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
 * \return Pointer to the destination \ref RpPrtStdEmitter if successful,
 * NULL otherwise.
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
RpPrtAdvEmtCircleCloneCB(RpAtomic *atomic __RWUNUSEDRELEASE__,
                        RpPrtStdEmitter *emt,
                          void *data )
{
    RwInt32                         srcOffset, dstOffset;

    RpPrtStdEmitter                 *dstEmt;
    RpPrtAdvEmtCircle           *emtCircle = NULL, *dstEmtCircle;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtCircleCloneCB"));

    RWASSERT(atomic);
    RWASSERT(emt);
    RWASSERT(data);

    dstEmt = (RpPrtStdEmitter *) data;

    srcOffset =
        RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERCIRCLE);

    dstOffset =
        RpPrtStdPropTabGetPropOffset(dstEmt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERCIRCLE);

    if ((srcOffset >= 0) && (dstOffset >= 0))
    {
        emtCircle = ((RpPrtAdvEmtCircle *)
                    RPPRTADVPROP(emt, srcOffset));

        dstEmtCircle = ((RpPrtAdvEmtCircle *)
                    RPPRTADVPROP(dstEmt, dstOffset));


        memcpy(dstEmtCircle, emtCircle, sizeof(RpPrtAdvEmtCircle));
    }

    RWRETURN(dstEmt);
}


/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup prtadvcircle
 * \ref RpPrtAdvEmtCircleStreamGetSizeCB is the callback used to determine
 * the size, in bytes, of the binary representation of a circular emitter.
 * This value is used in the binary chunk header to indicate the size of the chunk.
 *
 * This callback should be added to an emitter class if the emitter contains the
 * circular emitter property.
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
 * \see RpPrtAdvEmtCircleStreamReadCB
 * \see RpPrtAdvEmtCircleStreamWriteCB
 */
RpPrtStdEmitter *
RpPrtAdvEmtCircleStreamGetSizeCB(RpAtomic * atomic __RWUNUSED__,
                                       RpPrtStdEmitter *emt,
                                       void * data)
{
    RwInt32                         offset,totalSize;

    RpPrtStdPropertyTable           *propTab;
    RpPrtStdEmitterClass            *eClass;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtCircleStreamGetSizeCB"));

    RWASSERT(emt);
    RWASSERT(data);

    eClass = emt->emtClass;
    propTab = eClass->propTab;

    /* Get size of the multi color prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERCIRCLE);


    totalSize = 0;
    if (offset >= 0)
    {
        totalSize += sizeof(RwReal);
        totalSize += sizeof(RwReal);
        totalSize += sizeof(RwReal);

        totalSize += sizeof(RwInt32);

        totalSize += sizeof(RwReal);
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
 * \ingroup prtadvcircle
 * \ref RpPrtAdvEmtCircleStreamReadCB is the stream in callback for
 * the circular particle property.
 *
 * This function streams in an emitter containing the circular property.
 *
 * This callback should be added to an emitter class if the emitter contains
 * the circular emitter property.
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
 * \see RpPrtAdvEmtCircleStreamGetSizeCB
 * \see RpPrtAdvEmtCircleStreamWriteCB
 */
RpPrtStdEmitter *
RpPrtAdvEmtCircleStreamReadCB(RpAtomic * atomic __RWUNUSED__,
                                       RpPrtStdEmitter *emt,
                                       void * data)
{
    RwInt32                         offset;
    RwStream                        *stream;

    RpPrtStdPropertyTable           *propTab;
    RpPrtStdEmitterClass            *eClass;
    RpPrtAdvEmtCircle           *emtCircle;
    RwInt32                         abool = 0;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtCircleStreamReadCB"));

    RWASSERT(emt);
    RWASSERT(data);

    stream = (RwStream *) data;

    eClass = emt->emtClass;
    propTab = eClass->propTab;

    /* Get size of the multi color prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERCIRCLE);
    if (offset >= 0)
    {
        emtCircle = ((RpPrtAdvEmtCircle *)
                    RPPRTADVPROP(emt, offset));


        RwStreamReadReal(stream,&emtCircle->radius,sizeof(RwReal));
        RwStreamReadReal(stream,&emtCircle->radiusGap,sizeof(RwReal));
        RwStreamReadReal(stream,&emtCircle->height,sizeof(RwReal));

        RwStreamReadInt32(stream,&abool,sizeof(RwInt32));
        if( 1 == abool )
        {
            emtCircle->useCircleEmission = TRUE;
        }
        else
        {
            emtCircle->useCircleEmission = FALSE;
        }

        RwStreamReadReal(stream,&emtCircle->dirRotation,sizeof(RwReal));
    }

    RWRETURN(emt);
}

/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup prtadvcircle
 * \ref RpPrtAdvEmtCircleStreamWriteCB is the stream out callback for
 * the circular particle property.
 *
 * This function streams out an emitter containing the circular property.
 *
 * This callback should be added to an emitter class if the emitter contains
 * the circular emitter property.
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
 * \see RpPrtAdvEmtCircleStreamGetSizeCB
 * \see RpPrtAdvEmtCircleStreamReadCB
 */
RpPrtStdEmitter *
RpPrtAdvEmtCircleStreamWriteCB(RpAtomic * atomic __RWUNUSED__,
                                       RpPrtStdEmitter *emt,
                                       void * data)
{
    RwInt32                         offset;
    RwStream                        *stream;

    RpPrtStdPropertyTable           *propTab;
    RpPrtStdEmitterClass            *eClass;
    RpPrtAdvEmtCircle           *emtCircle;
    RwInt32                         abool = 0;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtCircleStreamWriteCB"));

    RWASSERT(emt);
    RWASSERT(data);

    stream = (RwStream *) data;

    eClass = emt->emtClass;
    propTab = eClass->propTab;

    /* Get size of the multi color prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERCIRCLE);



    if (offset >= 0)
    {
        emtCircle = ((RpPrtAdvEmtCircle *)
                    RPPRTADVPROP(emt, offset));


        RwStreamWriteReal(stream,&emtCircle->radius,sizeof(RwReal));
        RwStreamWriteReal(stream,&emtCircle->radiusGap,sizeof(RwReal));
        RwStreamWriteReal(stream,&emtCircle->height,sizeof(RwReal));

        if( TRUE == emtCircle->useCircleEmission )
        {
            abool = 1;
        }
        RwStreamWriteInt32(stream,&abool,sizeof(RwInt32));

        RwStreamWriteReal(stream,&emtCircle->dirRotation,sizeof(RwReal));
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


