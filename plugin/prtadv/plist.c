/**********************************************************************
 *
 * File :     plist.c
 *
 * Abstract : Point List emitter
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

#include "plist.h"
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
 * \ingroup prtadvpointlist
 * \ref RpPrtAdvEmtPtListEmitCB is the emit callback for the point list
 * emitter property.
 *
 * This callback emits new particles using the current emitter. The particle
 * class must contain the point list particle properties.
 *
 * This callback should be added \e after the standard callback.
 *
 * All other properties of the particles should be handled by the previous emit
 * callback in the list.
 *
 * \param atomic        Pointer to the parent \ref RpAtomic.
 * \param emt           Pointer to the \ref RpPrtStdEmitter.
 * \param data          Void pointer to private data for the emit callback. This
 *                      callback expects the data to be a pointer to a
 *                      RwReal representing the delta time elapsed.
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
RpPrtAdvEmtPtListEmitCB(RpAtomic *atomic __RWUNUSEDRELEASE__,
                        RpPrtStdEmitter *emt,
                          void *data __RWUNUSEDRELEASE__)
{
    RwMatrix        *ltm;
    RwV3d           v;
#ifdef MATRIXSUPPORT
    RwV3d           v2, *vptr;
#endif /* MATRIXSUPPORT */

    RwReal          rr;
    RwInt32         ir, numPrt, i, prtStride;

    RpPrtStdEmitter                 *result;
    RpPrtStdParticleBatch           *prtBatch;

    RwChar                         *pTankPosOut = (RwChar *)NULL;

    RwInt32                         pTankPosStride = 0;

    RpPrtStdEmitterStandard         *emtStd;
    RpPrtStdEmitterPTank            *emtPTank;
    RpPrtAdvEmtPointList        *emtPTList;
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

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtPtListEmitCB"));

    RWASSERT(atomic);
    RWASSERT(emt);
    RWASSERT(data);

    /* Get the emitter's properties */
    _rpPrtAdvEmitterGetPropOffset(emt);

    result = NULL;

    emtPTank = NULL;
    emtPTList = NULL;

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

    if (rpPrtAdvGlobals.offsetEmtPtList >= 0)
    {
        emtPTList = ((RpPrtAdvEmtPointList *)
                    RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtPtList));
        RWASSERT(emtPTList->pointList);
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
                RwInt32 selectIdx;
                /* Select a Point */
                PRTSTD_SRAND(ir);
                rr = PRTSTD_2RSRAND2(ir)+0.5f;

                if( emtPTList->random )
                {
                    selectIdx = RwInt32FromRealMacro(rr
                                *(RwReal)emtPTList->numPoint);

                }
                else
                {
                    selectIdx = emtPTList->lastPoint;
                    emtPTList->lastPoint++;
                    if(emtPTList->lastPoint >= emtPTList->numPoint)
                    {
                        emtPTList->lastPoint = 0;
                    }

                }

                /* Initial pos. */
                PRTSTD_SRAND(ir);
                rr = PRTSTD_RSRAND(ir);
                v.x = emtPTList->pointList[selectIdx].x
                         + (rr * emtStd->emtSize.x);

                PRTSTD_SRAND(ir);
                rr = PRTSTD_RSRAND(ir);
                v.y = emtPTList->pointList[selectIdx].y
                         + (rr * emtStd->emtSize.y);

                PRTSTD_SRAND(ir);
                rr = PRTSTD_RSRAND(ir);
                v.z = emtPTList->pointList[selectIdx].z
                         + (rr * emtStd->emtSize.z);

                /* Transform to world space. */
                if (ltm)
                    RwV3dTransformPoint(&pos, &v, ltm);


                if (TRUE == emtPTList->useDirection)
                {
                    prtVel = (RwV3d  *)
                        RPPRTADVPROP(prt, rpPrtAdvGlobals.offsetPrtVel);

                    PRTSTD_SRAND(ir);
                    rr = PRTSTD_RSRAND(ir);
                    v.x = emtPTList->directionList[selectIdx].x
                                             + (rr * emtStd->prtInitDirBias.x);
                    PRTSTD_SRAND(ir);
                    rr = PRTSTD_RSRAND(ir);
                    v.y = emtPTList->directionList[selectIdx].y
                                             + (rr * emtStd->prtInitDirBias.y);
                    PRTSTD_SRAND(ir);
                    rr = PRTSTD_RSRAND(ir);
                    v.z = emtPTList->directionList[selectIdx].z
                                             + (rr * emtStd->prtInitDirBias.z);

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
 * \ingroup prtadvpointlist
 * \ref RpPrtAdvEmtPtListCloneCB is the clone callback for the point list
 * emitter property.
 *
 * This callback clones the point list property of the emitter.
 *
 * This callback should be added \e after the standard callback.
 *
 * All other properties of the particles should be handled by the previous emit
 * callback in the list.
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
RpPrtAdvEmtPtListCloneCB(RpAtomic *atomic __RWUNUSED__,
                                RpPrtStdEmitter *emt,
                                void *data)
{
    RwInt32                         srcOffset, dstOffset;

    RpPrtStdEmitter                 *dstEmt;
    RpPrtAdvEmtPointList            *emtPointList, *dstEmtPointList;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtPtListCloneCB"));

    RWASSERT(emt);
    RWASSERT(data);

    dstEmt = (RpPrtStdEmitter *) data;

    /* Get size of the multi color prop. */
    srcOffset = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
        rpPRTADVPROPERTYCODEEMITTERPOINTLIST);

    dstOffset = RpPrtStdPropTabGetPropOffset(dstEmt->emtClass->propTab,
        rpPRTADVPROPERTYCODEEMITTERPOINTLIST);

    if ((srcOffset >= 0) && (dstOffset >= 0))
    {
        emtPointList = ((RpPrtAdvEmtPointList *)
                    RPPRTADVPROP(emt, srcOffset));

        dstEmtPointList = ((RpPrtAdvEmtPointList *)
                    RPPRTADVPROP(dstEmt, dstOffset));

        dstEmtPointList->numPoint = emtPointList->numPoint;
        dstEmtPointList->useDirection = emtPointList->useDirection;
        dstEmtPointList->random = emtPointList->random;

        memcpy(dstEmtPointList->pointList, emtPointList->pointList,
            (dstEmtPointList->numPoint * sizeof(RwV3d)));

        if (TRUE == emtPointList->useDirection)
        {
            memcpy(dstEmtPointList->directionList, emtPointList->directionList,
                (dstEmtPointList->numPoint * sizeof(RwV3d)));
        }
    }

    RWRETURN(dstEmt);
}

/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup prtadvpointlist
 * \ref RpPrtAdvEmtPtListStreamGetSizeCB is the callback used to
 * determine the size, in bytes, of the binary representation of a point-list
 * emitter. This value is used in the binary chunk header to indicate the size
 * of the chunk.
 *
 * This callback should be added to an emitter class if the emitter contains
 * the point list emitter property.
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
 * \see RpPrtAdvEmtPtListStreamReadCB
 * \see RpPrtAdvEmtPtListStreamWriteCB
 */
RpPrtStdEmitter *
RpPrtAdvEmtPtListStreamGetSizeCB(RpAtomic * atomic __RWUNUSED__,
                                       RpPrtStdEmitter *emt,
                                       void * data)
{
    RwInt32                         offset,totalSize;

    RpPrtStdPropertyTable           *propTab;
    RpPrtStdEmitterClass            *eClass;
    RpPrtAdvEmtPointList           *emtPointList;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtPtListStreamGetSizeCB"));

    RWASSERT(emt);
    RWASSERT(data);

    eClass = emt->emtClass;
    propTab = eClass->propTab;

    /* Get size of the multi color prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERPOINTLIST);


    totalSize = 0;
    if (offset >= 0)
    {
        emtPointList = ((RpPrtAdvEmtPointList *)
                    RPPRTADVPROP(emt, offset));

        totalSize += sizeof(RwInt32);

        /* Not Streamed */
        /* emtPointList->currentPoint; */

        totalSize += sizeof(RwInt32);

        totalSize += sizeof(RwInt32);

        if( emtPointList->pointList )
        {
            totalSize += sizeof(RwInt32);

            totalSize += sizeof(RwV3d)*emtPointList->numPoint;
        }
        else
        {
            totalSize += sizeof(RwInt32);
        }

        if( emtPointList->directionList )
        {
            totalSize += sizeof(RwInt32);

            totalSize += sizeof(RwV3d)*emtPointList->numPoint;
        }
        else
        {
            totalSize += sizeof(RwInt32);
        }
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
 * \ingroup prtadvpointlist
 * \ref RpPrtAdvEmtPtListStreamReadCB is the stream in callback for
 * the point list particle property.
 *
 * This function streams in an emitter containing the point list property.
 *
 * This callback should be added to an emitter class if the emitter contains
 * the point list emitter property.
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
 * \see RpPrtAdvEmtPtListStreamGetSizeCB
 * \see RpPrtAdvEmtPtListStreamWriteCB
 */
RpPrtStdEmitter *
RpPrtAdvEmtPtListStreamReadCB(RpAtomic * atomic __RWUNUSED__,
                                       RpPrtStdEmitter *emt,
                                       void * data)
{
    RwInt32                         offset;
    RwStream                        *stream;

    RpPrtStdPropertyTable           *propTab;
    RpPrtStdEmitterClass            *eClass;
    RpPrtAdvEmtPointList           *emtPointList;
    RwInt32                         abool = 0;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtPtListStreamReadCB"));

    RWASSERT(emt);
    RWASSERT(data);

    stream = (RwStream *) data;

    eClass = emt->emtClass;
    propTab = eClass->propTab;

    /* Get size of the multi color prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERPOINTLIST);
    if (offset >= 0)
    {
        emtPointList = ((RpPrtAdvEmtPointList *)
                    RPPRTADVPROP(emt, offset));

        RwStreamReadInt32(stream,
                          (RwInt32*)&emtPointList->numPoint,
                          sizeof(RwInt32));

        /* Not Streamed */
        /* emtPointList->currentPoint; */

        RwStreamReadInt32(stream,&abool,sizeof(RwInt32));
        if( 1 == abool )
        {
            emtPointList->useDirection = TRUE;
        }
        else
        {
            emtPointList->useDirection = FALSE;
        }

        RwStreamReadInt32(stream,&abool,sizeof(RwInt32));
        if( 1 == abool )
        {
            emtPointList->random = TRUE;
        }
        else
        {
            emtPointList->random = FALSE;
        }


        RwStreamReadInt32(stream,&abool,sizeof(RwInt32));
        if( 1 == abool )
        {
            RwStreamReadReal(stream,
                                (RwReal*)emtPointList->pointList,
                                sizeof(RwV3d)*emtPointList->numPoint);
        }
        else
        {
            emtPointList->pointList = NULL;
        }

        RwStreamReadInt32(stream,&abool,sizeof(RwInt32));
        if( 1 == abool )
        {
            RwStreamReadReal(stream,
                                (RwReal*)emtPointList->directionList,
                                sizeof(RwV3d)*emtPointList->numPoint);
        }
        else
        {
            emtPointList->directionList = NULL;
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
 * \ingroup prtadvpointlist
 * \ref RpPrtAdvEmtPtListStreamWriteCB is the stream out callback for
 * the point list particle property.
 *
 * This function streams out an emitter containing the point list property.
 *
 * This callback should be added to an emitter class if the emitter contains
 * the point list emitter property.
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
 * \see RpPrtAdvEmtPtListStreamGetSizeCB
 * \see RpPrtAdvEmtPtListStreamReadCB
 */
RpPrtStdEmitter *
RpPrtAdvEmtPtListStreamWriteCB(RpAtomic * atomic __RWUNUSED__,
                                       RpPrtStdEmitter *emt,
                                       void * data)
{
    RwInt32                         offset;
    RwStream                        *stream;

    RpPrtStdPropertyTable           *propTab;
    RpPrtStdEmitterClass            *eClass;
    RpPrtAdvEmtPointList           *emtPointList;
    RwInt32                         abool = 0;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtPtListStreamWriteCB"));

    RWASSERT(emt);
    RWASSERT(data);

    stream = (RwStream *) data;

    eClass = emt->emtClass;
    propTab = eClass->propTab;

    /* Get size of the multi color prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERPOINTLIST);

    if (offset >= 0)
    {
        emtPointList = ((RpPrtAdvEmtPointList *)
                    RPPRTADVPROP(emt, offset));

        RwStreamWriteInt32(stream,
                           (RwInt32*)&emtPointList->numPoint,
                           sizeof(RwInt32));

        /* Not Streamed */
        /* emtPointList->currentPoint; */

        if( TRUE == emtPointList->useDirection )
        {
            abool = 1;
        }
        RwStreamWriteInt32(stream,&abool,sizeof(RwInt32));

        if( TRUE == emtPointList->random )
        {
            abool = 1;
        }
        else
        {
            abool = 0;
        }
        RwStreamWriteInt32(stream,&abool,sizeof(RwInt32));

        if( emtPointList->pointList )
        {
            abool = 1;
            RwStreamWriteInt32(stream,&abool,sizeof(RwInt32));

            RwStreamWriteReal(stream,
                                (RwReal*)emtPointList->pointList,
                                sizeof(RwV3d)*emtPointList->numPoint);
        }
        else
        {
            abool = 0;
            RwStreamWriteInt32(stream,&abool,sizeof(RwInt32));
        }

        if( emtPointList->directionList )
        {
            abool = 1;
            RwStreamWriteInt32(stream,&abool,sizeof(RwInt32));

            RwStreamWriteReal(stream,
                                (RwReal*)emtPointList->directionList,
                                sizeof(RwV3d)*emtPointList->numPoint);
        }
        else
        {
            abool = 0;
            RwStreamWriteInt32(stream,&abool,sizeof(RwInt32));
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
 * \ingroup prtadvpointlist
 * \ref RpPrtAdvEmtPtListCreateCB is the create callback for the
 * point list emitter property.
 *
 * This callback sets up the point list emitter properties of an emitter.
 *
 * This callback should be added to an emitter class if the emitter contains
 * the point list emitter property.
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
RpPrtAdvEmtPtListCreateCB(RpAtomic * atomic __RWUNUSED__,
                                RpPrtStdEmitter *emt,
                                void * data __RWUNUSED__)
{
    RwInt32 offset,size;
    RpPrtAdvEmtPointList        *emtPTList;
    RwInt32 *propSize;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtPtListCreateCB"));

    offset = RpPrtStdPropTabGetPropOffset(
    emt->emtClass->propTab, rpPRTADVPROPERTYCODEEMITTERPOINTLIST);

    emtPTList = (RpPrtAdvEmtPointList*) (((RwChar *)emt) + offset);

    offset = RpPrtStdPropTabGetPropIndex(
    emt->emtClass->propTab, rpPRTADVPROPERTYCODEEMITTERPOINTLIST);

    RpPrtStdPropTabGetProperties(emt->emtClass->propTab,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &propSize);

    size = propSize[offset];

    size -= sizeof(RpPrtAdvEmtPointList);

    emtPTList->numPoint = size / (2 * sizeof(RwV3d));
    emtPTList->pointList = (RwV3d *)(
                            (RwUInt32)emtPTList+
                            sizeof(RpPrtAdvEmtPointList));
    emtPTList->directionList =  (RwV3d *)(
                                (RwUInt32)emtPTList->pointList+
                                sizeof(RwV3d)*emtPTList->numPoint);

    emtPTList->useDirection = FALSE;
    emtPTList->lastPoint = 0;
    emtPTList->random = FALSE;

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


