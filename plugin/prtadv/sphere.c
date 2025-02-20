/**********************************************************************
 *
 * File :     sphere.c
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

#include "sphere.h"
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
 * \ingroup prtadvsphere
 * \ref RpPrtAdvEmtSphereEmitCB is the emit callback for the spherical
 * emitter property.
 *
 * This callback emits new particles using the current emitter. The particle
 * class must contain the spherical particle properties.
 *
 * This callback should be added \e after the standard callback.
 *
 * All other properties of the particles should be handled by the previous
 * emit callback in the list.
 *
 * \param atomic        Pointer to the parent RpAtomic.
 * \param emt           Pointer to the RpPrtStdEmitter.
 * \param data          Void pointer to private data for the emit callback. This
 *                      callback expects the data to be a pointer to a
 *                      RwReal representing the delta time elapsed.
 *
 * \return Pointer to the RpPrtStdEmitter if successful, NULL otherwise.
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
RpPrtAdvEmtSphereEmitCB(RpAtomic *atomic __RWUNUSEDRELEASE__,
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
    RpPrtAdvEmtSphere           *emtSphere = NULL;
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

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtSphereEmitCB"));

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

    if (rpPrtAdvGlobals.offsetEmtSphere >= 0)
    {
        emtSphere = ((RpPrtAdvEmtSphere *)
                    RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtSphere));
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
                RwReal Angle1;
#ifdef NO_TWEAKED_SIN
                RwReal Angle2;
#endif
                RwReal SinA2,CosA2;
                RwReal SinA1,CosA1;

                PRTSTD_SRAND(ir);
                Angle1 = PRTSTD_RSRAND(ir) * _RW_pi;

                SinA2 = (RwReal)RwSin(Angle1);
                CosA2 = (RwReal)RwCos(Angle1);

#ifdef NO_TWEAKED_SIN
                PRTSTD_SRAND(ir);
                Angle2 = PRTSTD_RSRAND(ir) * _RW_pi;

                SinA1 = (RwReal)RwSin(Angle2);
                CosA1 = (RwReal)RwCos(Angle2);
#else
                PRTSTD_SRAND(ir);
                SinA1 = PRTSTD_RSRAND(ir);
                {
                    RwReal result;

                    rwSqrt(&result,(1 - SinA1*SinA1));
                    CosA1 = result;
                }
#endif
                /* Initial pos. */
                PRTSTD_SRAND(ir);
                rr = PRTSTD_2RSRAND2(ir)+0.5f;

                v.x = (emtSphere->radius + rr*emtSphere->radiusGap)
                        *CosA1*CosA2;
                v.y = (emtSphere->radius + rr*emtSphere->radiusGap)
                        *CosA1*SinA2;
                v.z = (emtSphere->radius + rr*emtSphere->radiusGap)
                        *SinA1;

                /* Transform to world space. */
                if (ltm)
                    RwV3dTransformPoint(&pos, &v, ltm);

                if(TRUE == emtSphere->useSphereEmission)
                {
                    prtVel = (RwV3d  *)RPPRTADVPROP(prt,
                                                rpPrtAdvGlobals.offsetPrtVel);

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

/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup prtadvsphere
 * \ref RpPrtAdvEmtSphereCloneCB is the clone callback for the spherical
 * emitter property.
 *
 * This callback clones the spherical emitter property of the emitter.
 *
 * This callback should be added \e after the standard callback.
 *
 * All other properties of the particles should be handled by the previous
 * emit callback in the list.
 *
 * \param atomic        Pointer to the parent RpAtomic.
 * \param emt           Pointer to the source RpPrtStdEmitter.
 * \param data          Pointer to the destination RpPrtStdEmitter.
 *
 * \return Pointer to the destination RpPrtStdEmitter if successful, NULL otherwise.
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
RpPrtAdvEmtSphereCloneCB(RpAtomic *atomic __RWUNUSEDRELEASE__,
                        RpPrtStdEmitter *emt,
                          void *data )
{
    RwInt32                         srcOffset, dstOffset;

    RpPrtStdEmitter                 *dstEmt;
    RpPrtAdvEmtSphere           *emtSphere = NULL, *dstEmtSphere;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtSphereCloneCB"));

    RWASSERT(atomic);
    RWASSERT(emt);
    RWASSERT(data);

    dstEmt = (RpPrtStdEmitter *) data;

    srcOffset =
        RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERSPHERE);

    dstOffset =
        RpPrtStdPropTabGetPropOffset(dstEmt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERSPHERE);

    if ((srcOffset >= 0) && (dstOffset >= 0))
    {
        emtSphere = ((RpPrtAdvEmtSphere *)
                    RPPRTADVPROP(emt, srcOffset));

        dstEmtSphere = ((RpPrtAdvEmtSphere *)
                    RPPRTADVPROP(dstEmt, dstOffset));


        memcpy(dstEmtSphere, emtSphere, sizeof(RpPrtAdvEmtSphere));
    }

    RWRETURN(dstEmt);
}


/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup prtadvsphere
 * \ref RpPrtAdvEmtSphereStreamGetSizeCB is the callback used to determine
 * the size, in bytes, of the binary representation of a spherical emitter.
 * This value is used in the binary chunk header to indicate the size of the chunk.
 *
 * This callback should be added to a emitter class if the emitter contains
 * the spherical emitter property.
 *
 * \param atomic            A pointer to the emitter's parent RpAtomic.
 * \param emt               A pointer to the RpPrtStdEmitter.
 * \param data              Void pointer to a RwInt32 to return the stream
 *                          size.
 *
 * \return Pointer to the RpPrtStdEmitter if successful, NULL otherwise.
 *
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtStdEmitterStdStreamGetSizeCB
 * \see RpPrtStdEmitterStdStreamReadCB
 * \see RpPrtStdEmitterStdStreamWriteCB
 * \see RpPrtAdvEmtSphereStreamReadCB
 * \see RpPrtAdvEmtSphereStreamWriteCB
 */
RpPrtStdEmitter *
RpPrtAdvEmtSphereStreamGetSizeCB(RpAtomic * atomic __RWUNUSED__,
                                       RpPrtStdEmitter *emt,
                                       void * data)
{
    RwInt32                         offset,totalSize;

    RpPrtStdPropertyTable           *propTab;
    RpPrtStdEmitterClass            *eClass;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtSphereStreamGetSizeCB"));

    RWASSERT(emt);
    RWASSERT(data);

    eClass = emt->emtClass;
    propTab = eClass->propTab;

    /* Get size of the multi color prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERSPHERE);


    totalSize = 0;
    if (offset >= 0)
    {
        totalSize += sizeof(RwReal);
        totalSize += sizeof(RwReal);

        totalSize += sizeof(RwInt32);
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
 * \ingroup prtadvsphere
 * \ref RpPrtAdvEmtSphereStreamReadCB is the stream in callback for
 * the spherical particle property.
 *
 * This function streams in an emitter containing the spherical property.
 *
 * This callback should be added to a emitter class if the emitter contains
 * the spherical emitter property.
 *
 * \param atomic            A pointer to the emitter's parent RpAtomic.
 * \param emt               A pointer to the RpPrtStdEmitter to stream into.
 * \param data              Void pointer to the input stream.
 *
 * \return Pointer to the RpPrtStdEmitter if successful, NULL otherwise.
 *
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtStdEmitterStdStreamGetSizeCB
 * \see RpPrtStdEmitterStdStreamReadCB
 * \see RpPrtStdEmitterStdStreamWriteCB
 * \see RpPrtAdvEmtSphereStreamGetSizeCB
 * \see RpPrtAdvEmtSphereStreamWriteCB
 */
RpPrtStdEmitter *
RpPrtAdvEmtSphereStreamReadCB(RpAtomic * atomic __RWUNUSED__,
                                       RpPrtStdEmitter *emt,
                                       void * data)
{
    RwInt32                         offset;
    RwStream                        *stream;

    RpPrtStdPropertyTable           *propTab;
    RpPrtStdEmitterClass            *eClass;
    RpPrtAdvEmtSphere           *emtSphere;
    RwInt32                         abool = 0;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtSphereStreamReadCB"));

    RWASSERT(emt);
    RWASSERT(data);

    stream = (RwStream *) data;

    eClass = emt->emtClass;
    propTab = eClass->propTab;

    /* Get size of the multi color prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERSPHERE);
    if (offset >= 0)
    {
        emtSphere = ((RpPrtAdvEmtSphere *)
                    RPPRTADVPROP(emt, offset));


        RwStreamReadReal(stream,&emtSphere->radius,sizeof(RwReal));
        RwStreamReadReal(stream,&emtSphere->radiusGap,sizeof(RwReal));

        RwStreamReadInt32(stream,&abool,sizeof(RwInt32));
        if( 1 == abool )
        {
            emtSphere->useSphereEmission = TRUE;
        }
        else
        {
            emtSphere->useSphereEmission = FALSE;
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
 * \ingroup prtadvsphere
 * \ref RpPrtAdvEmtSphereStreamWriteCB is the stream out callback for
 * the spherical particle property.
 *
 * This function streams out an emitter containing the spherical property.
 *
 * This callback should be added to a emitter class if the emitter contains
 * the spherical emitter property.
 *
 * \param atomic            A pointer to the emitter's parent RpAtomic.
 * \param emt               A pointer to the RpPrtStdEmitter.
 * \param data              Void pointer to output stream.
 *
 * \return Pointer to the RpPrtStdEmitter if successful, NULL otherwise.
 *
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtStdEmitterStdStreamGetSizeCB
 * \see RpPrtStdEmitterStdStreamReadCB
 * \see RpPrtStdEmitterStdStreamWriteCB
 * \see RpPrtAdvEmtSphereStreamGetSizeCB
 * \see RpPrtAdvEmtSphereStreamReadCB
 */
RpPrtStdEmitter *
RpPrtAdvEmtSphereStreamWriteCB(RpAtomic * atomic __RWUNUSED__,
                                       RpPrtStdEmitter *emt,
                                       void * data)
{
    RwInt32                         offset;
    RwStream                        *stream;

    RpPrtStdPropertyTable           *propTab;
    RpPrtStdEmitterClass            *eClass;
    RpPrtAdvEmtSphere               *emtSphere;
    RwInt32                         abool = 0;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtSphereStreamWriteCB"));

    RWASSERT(emt);
    RWASSERT(data);

    stream = (RwStream *) data;

    eClass = emt->emtClass;
    propTab = eClass->propTab;

    /* Get size of the multi color prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERSPHERE);



    if (offset >= 0)
    {
        emtSphere = ((RpPrtAdvEmtSphere *)
                    RPPRTADVPROP(emt, offset));


        RwStreamWriteReal(stream,&emtSphere->radius,sizeof(RwReal));
        RwStreamWriteReal(stream,&emtSphere->radiusGap,sizeof(RwReal));

        if( TRUE == emtSphere->useSphereEmission )
        {
            abool = 1;
        }
        RwStreamWriteInt32(stream,&abool,sizeof(RwInt32));

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


