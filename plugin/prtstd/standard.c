/**********************************************************************
 *
 * File :     stdgen.c
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

#include "eclass.h"
#include "emitter.h"
#include "prop.h"
#include "pclass.h"
#include "prtstdvars.h"
#include "pptank.h"

#include "standard.h"

#if (defined(SKY2_DRVMODEL_H) && !defined(NOASM))

#include "stdsky2.h"

#endif /* (defined(SKY2_DRVMODEL_H) && !defined(NOASM)) */

/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdParticleStdUpdateCB is the update callback for the standard
 * particle property.
 *
 * This callback updates all the standard particle properties in a particle.
 *
 * This callback should be added to a particle class if the particle contains
 * any of the standard particle properties.
 *
 * \param emt           Pointer to the parent emitter.
 * \param prtBatch      Pointer to the particle batch to be updated.
 * \param data          Pointer to private data for the update callback. This
 * callback expects the data to be a pointer to a \ref RwReal representing the delta
 * time elapsed.
 *
 * \return pointer to the particle batch if sucessful, NULL otherwise.
 *
 * \see RpPrtStdAtomicUpdate
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtStdEmitterStdBeginUpdateCB
 * \see RpPrtStdEmitterStdCloneCB
 * \see RpPrtStdEmitterStdCreateCB
 * \see RpPrtStdEmitterStdDestroyCB
 * \see RpPrtStdEmitterStdEmitCB
 * \see RpPrtStdEmitterStdEndUpdateCB
 * \see RpPrtStdEmitterStdRenderCB
 * \see RpPrtStdPClassSetCallBack
 * \see RpPrtStdPClassStdCreate
 */
RpPrtStdParticleBatch *
RpPrtStdParticleStdUpdateCB(RpPrtStdEmitter *emt,
                                 RpPrtStdParticleBatch *prtBatch,
                                 void *data)
{

    RpPrtStdEmitterPTank              * emtPTank;

    RpPrtStdEmitterPrtColor           * emtPrtCol;
    RpPrtStdEmitterPrtTexCoords       * emtPrtTex;
    RpPrtStdEmitterStandard           * emtStd;
    RpPrtStdEmitterPrtMatrix          * emtPrtMtx;
    RpPrtStdEmitterPrtSize            * emtPrtSze;
    RpPrtStdEmitterPrt2DRotate        * emtPrt2DRot;

    RpPrtStdParticleColor             * prtColIn;
    RpPrtStdParticleTexCoords         * prtTexIn;
    RpPrtStdParticleSize              * prtSzeIn;
    RpPrtStdParticle2DRotate          * prt2DRotIn;

    RwChar                            * pTankColIn = (RwChar *)NULL ;
    RwChar                            * pTankColOut = (RwChar *)NULL ;
    RwChar                            * pTankPosIn = (RwChar *)NULL ;
    RwChar                            * pTankPosOut = (RwChar *)NULL ;
    RwChar                            * pTankTexIn = (RwChar *)NULL ;
    RwChar                            * pTankTexOut = (RwChar *)NULL ;
    RwChar                            * pTankSzeIn = (RwChar *)NULL ;
    RwChar                            * pTankSzeOut = (RwChar *)NULL ;
    RwChar                            * pTank2DRotIn = (RwChar *)NULL ;
    RwChar                            * pTank2DRotOut = (RwChar *)NULL ;
    RwChar                            * prtIn;
    RwChar                            * prtOut;

    RwInt32                             i;
    RwInt32                             numPrtIn;
    RwInt32                             numPrtOut;
    RwInt32                             pTankColStride = 0;
    RwInt32                             pTankPosStride = 0;
    RwInt32                             pTankTexStride = 0;
    RwInt32                             pTankSzeStride = 0;
    RwInt32                             pTank2DRotStride = 0;
    RwInt32                             prtStride;

    RwReal                              alphaT, deltaT, radi, dist;
    RwV3d                               emtPos, tmpPos;

    RwV3d                             * prtPosIn;
    RwV3d                             * prtVelIn;
    RwMatrix                          * prtMtxIn = (RwMatrix *) NULL;
    RwMatrix                          * prtMtxCns = (RwMatrix *) NULL;
    RwV3d                               mtxPos, prtScale;

    RWAPIFUNCTION(RWSTRING("RpPrtStdParticleStdUpdateCB"));

    RWASSERT(emt);
    RWASSERT(prtBatch);
    RWASSERT(data);

    deltaT = *(RwReal *)data;
    radi = (RwReal) 0.0;

    /* Get the emitter's properties */

    emtPTank = NULL;

    RWASSERT(rpPrtStdGlobals.offsetEmtStd >= 0);
    emtStd = ((RpPrtStdEmitterStandard *) RPPRTSTDPROP(emt, rpPrtStdGlobals.offsetEmtStd));

    if (rpPrtStdGlobals.offsetEmtPrtCol >= 0)
        emtPrtCol = ((RpPrtStdEmitterPrtColor *) RPPRTSTDPROP(emt, rpPrtStdGlobals.offsetEmtPrtCol));

    if (rpPrtStdGlobals.offsetEmtPrtTex >= 0)
        emtPrtTex = ((RpPrtStdEmitterPrtTexCoords *) RPPRTSTDPROP(emt, rpPrtStdGlobals.offsetEmtPrtTex));

    if (rpPrtStdGlobals.offsetEmtPrt2DRot >= 0)
        emtPrt2DRot = ((RpPrtStdEmitterPrt2DRotate *) RPPRTSTDPROP(emt, rpPrtStdGlobals.offsetEmtPrt2DRot));

    if (rpPrtStdGlobals.offsetEmtPrtMtx >= 0)
    {
        emtPrtMtx = ((RpPrtStdEmitterPrtMatrix *) RPPRTSTDPROP(emt, rpPrtStdGlobals.offsetEmtPrtMtx));

        /* Use the constant matrix */
        if (emtPrtMtx->flags)
        {
            prtMtxCns = &emtPrtMtx->prtCnsMtx;
        }
    }

    if (rpPrtStdGlobals.offsetEmtPrtSize >= 0)
        emtPrtSze = ((RpPrtStdEmitterPrtSize *) RPPRTSTDPROP(emt, rpPrtStdGlobals.offsetEmtPrtSize));

    if (rpPrtStdGlobals.offsetEmtPTank >= 0)
        emtPTank = ((RpPrtStdEmitterPTank *) RPPRTSTDPROP(emt, rpPrtStdGlobals.offsetEmtPTank));

    if (emtPTank)
    {
        /* Position. */
        i = 0;

        pTankPosIn = NULL;
        pTankPosOut = NULL;
        pTankPosStride = 0;
        if (emtPTank->dataFlags & rpPTANKDFLAGPOSITION)
        {
            pTankPosIn = (RwChar *) emtPTank->dataInPtrs[i];
            pTankPosOut = (RwChar *) emtPTank->dataOutPtrs[i];

            if (emtPTank->updateFlags & rpPTANKDFLAGPOSITION)
                pTankPosStride = emtPTank->dataStride[i];

            RWASSERT(pTankPosIn);
            RWASSERT(pTankPosOut);

            i++;
        }

        /* Color. */
        pTankColIn = NULL;
        pTankColOut = NULL;
        pTankColStride = 0;
        if (emtPTank->dataFlags & rpPTANKDFLAGCOLOR)
        {
            pTankColIn = (RwChar *) emtPTank->dataInPtrs[i];
            pTankColOut = (RwChar *) emtPTank->dataOutPtrs[i];

            if (emtPTank->updateFlags & rpPTANKDFLAGCOLOR)
                pTankColStride = emtPTank->dataStride[i];

            RWASSERT(pTankColIn);
            RWASSERT(pTankColOut);

            i++;
        }

        /* Size. */
        pTankSzeIn = NULL;
        pTankSzeOut = NULL;
        pTankSzeStride = 0;
        if (emtPTank->dataFlags & rpPTANKDFLAGSIZE)
        {
            pTankSzeIn = (RwChar *) emtPTank->dataInPtrs[i];
            pTankSzeOut = (RwChar *) emtPTank->dataOutPtrs[i];

            if (emtPTank->updateFlags & rpPTANKDFLAGSIZE)
                pTankSzeStride = emtPTank->dataStride[i];

            RWASSERT(pTankSzeIn);
            RWASSERT(pTankSzeOut);

            i++;
        }

        /* Matrix. */
        if (emtPTank->dataFlags & rpPTANKDFLAGMATRIX)
        {
            pTankPosIn = (RwChar *) emtPTank->dataInPtrs[i];
            pTankPosOut = (RwChar *) emtPTank->dataOutPtrs[i];

            if (emtPTank->updateFlags & rpPTANKDFLAGMATRIX)
                pTankPosStride = emtPTank->dataStride[i];

            i++;
        }

        /* 2D Rotate. */
        pTank2DRotIn = NULL;
        pTank2DRotOut = NULL;
        pTank2DRotStride = 0;
        if (emtPTank->dataFlags & rpPTANKDFLAG2DROTATE)
        {
            pTank2DRotIn = (RwChar *) emtPTank->dataInPtrs[i];
            pTank2DRotOut = (RwChar *) emtPTank->dataOutPtrs[i];

            if (emtPTank->updateFlags & rpPTANKDFLAG2DROTATE)
                pTank2DRotStride = emtPTank->dataStride[i];

            i++;
        }

        /* Texcoord. */
        pTankTexIn = NULL;
        pTankTexOut = NULL;
        pTankTexStride = 0;
        if (emtPTank->dataFlags & rpPTANKDFLAGVTX2TEXCOORDS)
        {
            pTankTexIn = (RwChar *) emtPTank->dataInPtrs[i];
            pTankTexOut = (RwChar *) emtPTank->dataOutPtrs[i];

            if (emtPTank->updateFlags & rpPTANKDFLAGVTX2TEXCOORDS)
                pTankTexStride = emtPTank->dataStride[i];

            RWASSERT(pTankTexIn);
            RWASSERT(pTankTexOut);

            i++;
        }
    }

    /* Particles update loop. */
    prtStride = emt->prtClass->objSize;
    prtIn = ((RwChar *)prtBatch) + prtBatch->offset;
    prtOut = prtIn;

    if (emtStd->ltm)
    {
        emtPos = emtStd->ltm->pos;
    }
    else
    {
        emtPos.x = (RwReal) 0.0;
        emtPos.y = (RwReal) 0.0;
        emtPos.z = (RwReal) 0.0;
    }

    numPrtIn = prtBatch->numPrt;
    numPrtOut = 0;

    while (numPrtIn-- > 0)
    {
        /* Is this particle dead ? */
        if (((RpPrtStdParticleStandard *)prtIn)->currTime < ((RpPrtStdParticleStandard *)prtIn)->endTime)
        {
            /* time */
            ((RpPrtStdParticleStandard *)prtIn)->currTime =
                ((RpPrtStdParticleStandard *)prtIn)->currTime + deltaT;
            alphaT = ((RpPrtStdParticleStandard *)prtIn)->currTime *
                     ((RpPrtStdParticleStandard *)prtIn)->invEndTime;

            alphaT = (alphaT > (RwReal) 1.0) ? (RwReal) 1.0 : (RwReal) alphaT;

            prtVelIn = (RwV3d *) RPPRTSTDPROP(prtIn, rpPrtStdGlobals.offsetPrtVel);
            prtColIn = ((RpPrtStdParticleColor *) RPPRTSTDPROP(prtIn, rpPrtStdGlobals.offsetPrtCol));
            prtTexIn = ((RpPrtStdParticleTexCoords *) RPPRTSTDPROP(prtIn, rpPrtStdGlobals.offsetPrtTex));
            prtMtxIn = (RwMatrix *) RPPRTSTDPROP(prtIn, rpPrtStdGlobals.offsetPrtMtx);
            prtSzeIn = (RpPrtStdParticleSize *) RPPRTSTDPROP(prtIn, rpPrtStdGlobals.offsetPrtSize);
            prt2DRotIn = (RpPrtStdParticle2DRotate *) RPPRTSTDPROP(prtIn, rpPrtStdGlobals.offsetPrt2DRot);

            if (rpPrtStdGlobals.offsetPrtVel >= 0)
            {
                /* velocity v = u + at */
                prtVelIn->x += (deltaT * emtStd->force.x);
                prtVelIn->y += (deltaT * emtStd->force.y);
                prtVelIn->z += (deltaT * emtStd->force.z);
            }

            /* Update the position */
            if (rpPrtStdGlobals.offsetPrtPos >= 0)
            {
                prtPosIn = (RwV3d *) RPPRTSTDPROP(prtIn, rpPrtStdGlobals.offsetPrtPos);

                /* position s = ut */
                prtPosIn->x += (deltaT * prtVelIn->x);
                prtPosIn->y += (deltaT * prtVelIn->y);
                prtPosIn->z += (deltaT * prtVelIn->z);
            }

            if (rpPrtStdGlobals.offsetPrtSize >= 0)
            {
                prtScale.x = prtSzeIn->currSize.x;
                prtScale.y = prtSzeIn->currSize.y;
                prtScale.z = (RwReal) 1.0;

                prtSzeIn->currSize.x = prtSzeIn->startSize.x + (alphaT * prtSzeIn->deltaSize.x);
                prtSzeIn->currSize.y = prtSzeIn->startSize.y + (alphaT * prtSzeIn->deltaSize.y);
            }

            /* Copy over some data */
            if (prtIn != prtOut)
            {
                memcpy(prtOut, prtIn, prtStride);
            }

            /* Update the pTank */
            if (emtPTank)
            {
                /* position. */
                if (emtPTank->dataFlags & emtPTank->updateFlags & rpPTANKDFLAGPOSITION)
                {
                    if (rpPrtStdGlobals.offsetPrtVel >= 0)
                    {
                        /* position s = ut */
                        ((RwV3d *) pTankPosOut)->x = ((RwV3d *) pTankPosIn)->x + (deltaT * prtVelIn->x);
                        ((RwV3d *) pTankPosOut)->y = ((RwV3d *) pTankPosIn)->y + (deltaT * prtVelIn->y);
                        ((RwV3d *) pTankPosOut)->z = ((RwV3d *) pTankPosIn)->z + (deltaT * prtVelIn->z);
#if (defined(SKY2_DRVMODEL_H) && defined(SKY_USE_STOP_FLAG) )
                        ((RwV4d *)pTankPosOut)->w = 0.0f;
#endif
                        RwV3dSub(&tmpPos, (RwV3d *) pTankPosOut, &emtPos);

                        dist = RwV3dDotProduct(&tmpPos, &tmpPos);

                        radi = (radi > dist) ? radi : dist;
                    }

                    pTankPosOut += pTankPosStride;
                }

                /* Matrix */
                if (emtPTank->dataFlags & emtPTank->updateFlags & rpPTANKDFLAGMATRIX)
                {
                    if (rpPrtStdGlobals.offsetPrtSize >= 0)
                    {
                        if (prtScale.x != (RwReal) 0.0)
                            prtScale.x = prtSzeIn->currSize.x / prtScale.x;
                        else
                            prtScale.x = (RwReal) 1.0;

                        if (prtScale.y != (RwReal) 0.0)
                            prtScale.y = prtSzeIn->currSize.y / prtScale.y;
                        else
                            prtScale.y = (RwReal) 1.0;

                        prtScale.z = (RwReal) 1.0;
                    }

                    if ((rpPrtStdGlobals.offsetPrtMtx >= 0) || (prtMtxCns != NULL))
                    {
                        /* Save the position */
                        mtxPos = ((RwMatrix *) pTankPosIn)->pos;

                        /* Reset the position to be the original before applying the
                         * transformation.
                         */

                        ((RwMatrix *) pTankPosIn)->pos.x = (RwReal) 0.0;
                        ((RwMatrix *) pTankPosIn)->pos.y = (RwReal) 0.0;
                        ((RwMatrix *) pTankPosIn)->pos.z = (RwReal) 0.0;

                        /* Scale the matrix */
                        if (rpPrtStdGlobals.offsetPrtSize >= 0)
                        {
                            RwV3dScale((RwMatrixGetRight((RwMatrix *) pTankPosIn)),
                                       (RwMatrixGetRight((RwMatrix *) pTankPosIn)),
                                       prtScale.x);

                            RwV3dScale((RwMatrixGetUp((RwMatrix *) pTankPosIn)),
                                       (RwMatrixGetUp((RwMatrix *) pTankPosIn)),
                                       prtScale.y);
                        }

                        /* Update the ptank mtx */
                        if (rpPrtStdGlobals.offsetPrtMtx >= 0)
                        {
                            RwMatrix result;

                            RwMatrixMultiply(&result,
                                (const RwMatrix *) pTankPosIn, (const RwMatrix *) prtMtxIn);

                            RwMatrixCopy((RwMatrix *) pTankPosOut,&result);
                        }
                        else
                        {
                            RwMatrix result;

                            RwMatrixMultiply(&result,
                                (const RwMatrix *) pTankPosIn, (const RwMatrix *) prtMtxCns);

                            RwMatrixCopy((RwMatrix *) pTankPosOut,&result);
                        }

                        /* Update the position */
                        ((RwMatrix *) pTankPosOut)->pos.x += mtxPos.x + (deltaT * prtVelIn->x);
                        ((RwMatrix *) pTankPosOut)->pos.y += mtxPos.y + (deltaT * prtVelIn->y);
                        ((RwMatrix *) pTankPosOut)->pos.z += mtxPos.z + (deltaT * prtVelIn->z);
                    }
                    else
                    {
                        /* Copy the matrix */
                        if (pTankPosOut != pTankPosIn)
                        {
                            ((RwMatrix *) pTankPosOut)->at = ((RwMatrix *) pTankPosIn)->at;
                            ((RwMatrix *) pTankPosOut)->up = ((RwMatrix *) pTankPosIn)->up;
                            ((RwMatrix *) pTankPosOut)->right = ((RwMatrix *) pTankPosIn)->right;
                            ((RwMatrix *) pTankPosOut)->flags = ((RwMatrix *) pTankPosIn)->flags;
                        }

                        /* Save the position */
                        mtxPos = ((RwMatrix *) pTankPosIn)->pos;

                        /* Scale the matrix */
                        if (rpPrtStdGlobals.offsetPrtSize >= 0)
                        {
                            RwV3dScale((RwMatrixGetRight((RwMatrix *) pTankPosIn)),
                                       (RwMatrixGetRight((RwMatrix *) pTankPosIn)),
                                       prtScale.x);

                            RwV3dScale((RwMatrixGetUp((RwMatrix *) pTankPosIn)),
                                       (RwMatrixGetUp((RwMatrix *) pTankPosIn)),
                                       prtScale.y);
                        }

                        /* Update the position */
                        ((RwMatrix *) pTankPosOut)->pos.x += mtxPos.x + (deltaT * prtVelIn->x);
                        ((RwMatrix *) pTankPosOut)->pos.y += mtxPos.y + (deltaT * prtVelIn->y);
                        ((RwMatrix *) pTankPosOut)->pos.z += mtxPos.z + (deltaT * prtVelIn->z);
                    }

                    RwV3dSub(&tmpPos, &((RwMatrix *) pTankPosOut)->pos, &emtPos);

                    dist = RwV3dDotProduct(&tmpPos, &tmpPos);

                    radi = (radi > dist) ? radi : dist;

                    pTankPosOut += pTankPosStride;
                }

                /* size */
                if (emtPTank->dataFlags & emtPTank->updateFlags & rpPTANKDFLAGSIZE)
                {
                    if (rpPrtStdGlobals.offsetPrtSize >= 0)
                    {
                        ((RwV2d *) pTankSzeOut)->x = prtSzeIn->startSize.x + (alphaT * prtSzeIn->deltaSize.x);
                        ((RwV2d *) pTankSzeOut)->y = prtSzeIn->startSize.y + (alphaT * prtSzeIn->deltaSize.y);
                    }
                    else if (pTankSzeOut != pTankSzeIn)
                    {
                        *((RwV2d *) pTankSzeOut) = *((RwV2d *) pTankSzeIn);
                    }

                    pTankSzeOut += pTankSzeStride;
                }

                /* color */
                if (emtPTank->dataFlags & emtPTank->updateFlags & rpPTANKDFLAGCOLOR)
                {
                    if (rpPrtStdGlobals.offsetPrtCol >= 0)
                    {
                        ((RwRGBA *) pTankColOut)->red =
                            RwInt32FromRealMacro(prtColIn->startCol.red + (alphaT * prtColIn->deltaCol.red));
                        ((RwRGBA *) pTankColOut)->green =
                            RwInt32FromRealMacro(prtColIn->startCol.green + (alphaT * prtColIn->deltaCol.green));
                        ((RwRGBA *) pTankColOut)->blue =
                            RwInt32FromRealMacro(prtColIn->startCol.blue + (alphaT * prtColIn->deltaCol.blue));
                        ((RwRGBA *) pTankColOut)->alpha =
                            RwInt32FromRealMacro(prtColIn->startCol.alpha + (alphaT * prtColIn->deltaCol.alpha));
                    }
                    else if (pTankColOut != pTankColIn)
                    {
                        *((RwRGBA *) pTankColOut) = *((RwRGBA *) pTankColIn);
                    }

                    pTankColOut += pTankColStride;
                }

                /* uv */
                if (emtPTank->dataFlags & emtPTank->updateFlags & rpPTANKDFLAGVTX2TEXCOORDS)
                {
                    if (rpPrtStdGlobals.offsetPrtTex >= 0)
                    {
                        ((RwTexCoords *) pTankTexOut)[0].u =
                             prtTexIn->startUV0.u + (alphaT * prtTexIn->deltaUV0.u);
                        ((RwTexCoords *) pTankTexOut)[0].v =
                            prtTexIn->startUV0.v + (alphaT * prtTexIn->deltaUV0.v);

                        ((RwTexCoords *) pTankTexOut)[1].u =
                            prtTexIn->startUV1.u + (alphaT * prtTexIn->deltaUV1.u);
                        ((RwTexCoords *) pTankTexOut)[1].v =
                            prtTexIn->startUV1.v + (alphaT * prtTexIn->deltaUV1.v);
                    }
                    else if (pTankTexOut != pTankTexIn)
                    {
                        ((RwTexCoords *) pTankTexOut)[0] = ((RwTexCoords *) pTankTexIn)[0];
                        ((RwTexCoords *) pTankTexOut)[1] = ((RwTexCoords *) pTankTexIn)[1];
                    }

                    pTankTexOut += pTankTexStride;
                }

                /* 2D Rotate */
                if (emtPTank->dataFlags & emtPTank->updateFlags & rpPTANKDFLAG2DROTATE)
                {
                    if (rpPrtStdGlobals.offsetPrt2DRot >= 0)
                    {
                        (*(RwReal *) pTank2DRotOut) =
                            prt2DRotIn->start2DRotate + (alphaT * prt2DRotIn->delta2DRotate);
                    }
                    else if (pTank2DRotOut != pTank2DRotIn)
                    {
                        *((RwReal *) pTank2DRotOut) = *((RwReal *) pTank2DRotIn);
                    }

                    pTank2DRotOut += pTank2DRotStride;
                }

            }

            prtOut += prtStride;

            numPrtOut++;
        }

        /* Next input prt */
        prtIn += prtStride;

        pTankPosIn += pTankPosStride;
        pTankColIn += pTankColStride;
        pTankTexIn += pTankTexStride;
        pTankSzeIn += pTankSzeStride;
        pTank2DRotIn += pTank2DRotStride;
    }

    /* Update the prt count in the batch */
    prtBatch->numPrt = numPrtOut;

    /* Position. */
    if (emtPTank)
    {
        emtPTank->numPrt += numPrtOut;

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

        /* Size. */
        if (emtPTank->dataFlags & rpPTANKDFLAGSIZE)
        {
            emtPTank->dataInPtrs[i] = pTankSzeIn;
            emtPTank->dataOutPtrs[i] = pTankSzeOut;
            i++;
        }

        /* Matrix. */
        if (emtPTank->dataFlags & rpPTANKDFLAGMATRIX)
        {
            emtPTank->dataInPtrs[i] = pTankPosIn;
            emtPTank->dataOutPtrs[i] = pTankPosOut;
            i++;
        }

        /* 2D Rotate. */
        if (emtPTank->dataFlags & rpPTANKDFLAG2DROTATE)
        {
            emtPTank->dataInPtrs[i] = pTank2DRotIn;
            emtPTank->dataOutPtrs[i] = pTank2DRotOut;
            i++;
        }

        /* Texcoord. */
        if (emtPTank->dataFlags & rpPTANKDFLAGVTX2TEXCOORDS)
        {
            emtPTank->dataInPtrs[i] = pTankTexIn;
            emtPTank->dataOutPtrs[i] = pTankTexOut;
            i++;
        }
    }

    /* Emitter's bounding sphere */
    if (radi > emt->boundingSphere)
        emt->boundingSphere = radi;

    RWRETURN(prtBatch);
}

/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterStdEmitCB is the emit callback for the standard
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
 * \return pointer to the emitter if sucessful, NULL otherwise.
 *
 * \see RpPrtStdAtomicUpdate
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtStdEmitterStdBeginUpdateCB
 * \see RpPrtStdEmitterStdCloneCB
 * \see RpPrtStdEmitterStdCreateCB
 * \see RpPrtStdEmitterStdDestroyCB
 * \see RpPrtStdEmitterStdEndUpdateCB
 * \see RpPrtStdEmitterStdRenderCB
 * \see RpPrtStdPClassSetCallBack
 * \see RpPrtStdPClassStdCreate
 */

RpPrtStdEmitter *
RpPrtStdEmitterStdEmitCB(RpAtomic * atomic __RWUNUSED__,
                              RpPrtStdEmitter *emt,
                              void * data __RWUNUSED__)
{
    RwMatrix        *ltm;
    RwV3d           v, v2, *vptr;
    RwReal          rr;
    RwInt32         ir, numPrt, i, prtStride;

    RpPrtStdEmitter                 *result;
    RpPrtStdParticleBatch           *prtBatch;

    RwChar                         *pTankPosOut = (RwChar *)NULL;
    RwChar                         *pTankColOut = (RwChar *)NULL;
    RwChar                         *pTankTexOut = (RwChar *)NULL;
    RwChar                         *pTankSzeOut = (RwChar *)NULL;
    RwChar                         *pTank2DRotOut = (RwChar *)NULL;

    RwInt32                         pTankPosStride = 0;
    RwInt32                         pTankColStride = 0;
    RwInt32                         pTankTexStride = 0;
    RwInt32                         pTankSzeStride = 0;
    RwInt32                         pTank2DRotStride = 0;

    RpPrtStdEmitterStandard         *emtStd;
    RpPrtStdEmitterPTank            *emtPTank;
    RpPrtStdEmitterPrtColor         *emtPrtCol = (RpPrtStdEmitterPrtColor *)NULL;
    RpPrtStdEmitterPrtTexCoords     *emtPrtTex = (RpPrtStdEmitterPrtTexCoords *)NULL;
    RpPrtStdEmitterPrtMatrix        *emtPrtMtx = (RpPrtStdEmitterPrtMatrix *)NULL;
    RpPrtStdEmitterPrtSize          *emtPrtSze = (RpPrtStdEmitterPrtSize *)NULL;
    RpPrtStdEmitterPrt2DRotate      *emtPrt2DRot = (RpPrtStdEmitterPrt2DRotate *)NULL;

    RwChar                          *prt;
    RpPrtStdParticleStandard        *prtStd;
    RpPrtStdParticleColor           *prtCol;
    RpPrtStdParticleTexCoords       *prtTex;
    RwMatrix                        *prtMtx;
    RpPrtStdParticleSize            *prtSze;
    RpPrtStdParticle2DRotate        *prt2DRot;

    RwV3d                           *prtPos, *prtVel, pos, size;
    RwTexCoords                     uv[2];
    RwRGBAReal                      color;
    RwReal                          rot;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterStdEmitCB"));

    RWASSERT(emt);

    result = NULL;

    emtPTank = NULL;

    RWASSERT(rpPrtStdGlobals.offsetEmtStd >= 0);
    emtStd = ((RpPrtStdEmitterStandard *)
              RPPRTSTDPROP(emt, rpPrtStdGlobals.offsetEmtStd));

    if (rpPrtStdGlobals.offsetEmtPrtCol >= 0)
        emtPrtCol = ((RpPrtStdEmitterPrtColor *)  RPPRTSTDPROP(emt, rpPrtStdGlobals.offsetEmtPrtCol));

    if (rpPrtStdGlobals.offsetEmtPrtTex >= 0)
        emtPrtTex = ((RpPrtStdEmitterPrtTexCoords *) RPPRTSTDPROP(emt, rpPrtStdGlobals.offsetEmtPrtTex));

    if (rpPrtStdGlobals.offsetEmtPTank >= 0)
        emtPTank = ((RpPrtStdEmitterPTank *) RPPRTSTDPROP(emt, rpPrtStdGlobals.offsetEmtPTank));

    if (rpPrtStdGlobals.offsetEmtPrtMtx >= 0)
        emtPrtMtx = ((RpPrtStdEmitterPrtMatrix *) RPPRTSTDPROP(emt, rpPrtStdGlobals.offsetEmtPrtMtx));

    if (rpPrtStdGlobals.offsetEmtPrtSize >= 0)
        emtPrtSze = ((RpPrtStdEmitterPrtSize *) RPPRTSTDPROP(emt, rpPrtStdGlobals.offsetEmtPrtSize));

    if (rpPrtStdGlobals.offsetEmtPrt2DRot >= 0)
        emtPrt2DRot = ((RpPrtStdEmitterPrt2DRotate *) RPPRTSTDPROP(emt, rpPrtStdGlobals.offsetEmtPrt2DRot));

    /* Check if we are to emit particles. */
    if (emtStd->currTime >= emtStd->emtEmitTime)
    {
        RwInt32 shift;

        /* Get the emitter props. */

        /* Number of particles to be emitted. */
        ir = emtStd->seed;
        PRTSTD_SRAND(ir);
        rr = PRTSTD_RSRAND(ir);

        shift = RwInt32FromRealMacro(rr * (RwReal)(emtStd->emtPrtEmitBias));
        numPrt = emtStd->emtPrtEmit + shift;
        prtBatch = NULL;

        if (emtPTank)
        {
            /* Clamp to maximum possible particles. */
            if ((emt->prtActive + numPrt) > (RwInt32)emtPTank->maxPrt)
                numPrt = (RwInt32)emtPTank->maxPrt - emt->prtActive;
        }

        if ((numPrt > 0) && ( NULL != emt->activeBatch ))
        {
            /* We got an active batch, let's try to fill it */
            if( emt->activeBatch->numPrt + numPrt <= emt->activeBatch->maxPrt )
            {
                /* We Got a winner here, so let's use it */
                prtBatch = emt->activeBatch;

            }
            /* Should do something clever in the other case but just
             * let be simple for now
             */
        }

        if ((numPrt > 0) && ( NULL == prtBatch ))
        {
            /* Grab a new particle for the new particles. */
            prtBatch = RpPrtStdEmitterNewParticleBatch(emt);

            /* First new prt is 0 */
            if (prtBatch != NULL)
            {
                prtBatch->newPrt = 0;
                prtBatch->numPrt = 0;

                /* Clamp to the max the batch can hold. */
                if (numPrt > prtBatch->maxPrt)
                    numPrt = prtBatch->maxPrt;
            }
        }

        if (numPrt <= 0)
            RWRETURN(emt);

        if (prtBatch != NULL)
        {
            if (emtPTank)
            {
                /* Position.  */
                i = 0;
                pTankPosOut = NULL;
                pTankPosStride = 0;
                if (emtPTank->dataFlags & rpPTANKDFLAGPOSITION)
                {
                    pTankPosOut = emtPTank->dataOutPtrs[i];
                    pTankPosStride = emtPTank->dataStride[i];

                    i++;
                }

                /* Color. */
                pTankColOut = NULL;
                pTankColStride = 0;
                if (emtPTank->dataFlags & rpPTANKDFLAGCOLOR)
                {
                    pTankColOut = emtPTank->dataOutPtrs[i];
                    pTankColStride = emtPTank->dataStride[i];

                    RWASSERT(pTankColOut);

                    i++;
                }

                /* Size. */
                pTankSzeOut = NULL;
                pTankSzeStride = 0;
                if (emtPTank->dataFlags & rpPTANKDFLAGSIZE)
                {
                    pTankSzeOut = emtPTank->dataOutPtrs[i];
                    pTankSzeStride = emtPTank->dataStride[i];

                    RWASSERT(pTankSzeOut);

                    i++;
                }

                /* Matrix. */
                if (emtPTank->dataFlags & rpPTANKDFLAGMATRIX)
                {
                    pTankPosOut = emtPTank->dataOutPtrs[i];
                    pTankPosStride = emtPTank->dataStride[i];

                    RWASSERT(pTankPosOut);

                    i++;
                }

                /* 2D Rotate. */
                pTank2DRotOut = NULL;
                pTank2DRotStride = 0;
                if (emtPTank->dataFlags & rpPTANKDFLAG2DROTATE)
                {
                    pTank2DRotOut = emtPTank->dataOutPtrs[i];
                    pTank2DRotStride = emtPTank->dataStride[i];

                    RWASSERT(pTank2DRotOut);

                    i++;
                }

                /* Texcoord. */
                pTankTexOut = NULL;
                pTankTexStride = 0;
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
            prtStride = emt->prtClass->objSize;
            prt = ((RwChar *) prtBatch) + prtBatch->offset + prtBatch->numPrt * prtStride;

            for (i = 0; i < numPrt; i++)
            {
                prtStd = (RpPrtStdParticleStandard *) prt;

                /* Life */
                prtStd->currTime = (RwReal) 0.0;
                PRTSTD_SRAND(ir);
                rr = PRTSTD_RSRAND(ir);
                prtStd->endTime = emtStd->prtLife + (rr * emtStd->prtLifeBias);
                prtStd->invEndTime = (RwReal) 1.0 / prtStd->endTime;

                /* Initial velocity & pos. */
                if (rpPrtStdGlobals.offsetPrtVel >= 0)
                {
                    prtVel = (RwV3d  *) RPPRTSTDPROP(prt, rpPrtStdGlobals.offsetPrtVel);

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
                }

                if (rpPrtStdGlobals.offsetPrtPos >= 0)
                {
                    prtPos = (RwV3d *) RPPRTSTDPROP(prt, rpPrtStdGlobals.offsetPrtPos);

                    prtPos->x = pos.x;
                    prtPos->y = pos.y;
                    prtPos->z = pos.z;
                }

                if (rpPrtStdGlobals.offsetPrtMtx >= 0)
                {
                    prtMtx = (RwMatrix *) RPPRTSTDPROP(prt, rpPrtStdGlobals.offsetPrtMtx);

                    RwMatrixSetIdentity(prtMtx);
                }

                /* Initial size */
                if (rpPrtStdGlobals.offsetEmtPrtSize >= 0)
                {
                    /* Start size. */
                    PRTSTD_SRAND(ir);
                    rr = PRTSTD_RSRAND(ir);
                    size.x = emtPrtSze->prtStartSize.x + (rr * emtPrtSze->prtStartSizeBias.x);

                    PRTSTD_SRAND(ir);
                    rr = PRTSTD_RSRAND(ir);
                    size.y = emtPrtSze->prtStartSize.y + (rr * emtPrtSze->prtStartSizeBias.y);

                    if (rpPrtStdGlobals.offsetPrtSize >= 0)
                    {
                        prtSze = (RpPrtStdParticleSize *) RPPRTSTDPROP(prt, rpPrtStdGlobals.offsetPrtSize);

                        /* Start size. */
                        prtSze->startSize.x = size.x;
                        prtSze->startSize.y = size.y;

                        /* Curr size */
                        prtSze->currSize.x = size.x;
                        prtSze->currSize.y = size.y;

                        /* Delta size. */
                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtSze->deltaSize.x = (emtPrtSze->prtEndSize.x +
                            (rr * emtPrtSze->prtEndSizeBias.x)) - prtSze->startSize.x;

                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtSze->deltaSize.y = (emtPrtSze->prtEndSize.y +
                            (rr * emtPrtSze->prtEndSizeBias.y)) - prtSze->startSize.y;
                    }
                }
                else
                {
                    size.x = emtStd->prtSize.x;
                    size.y = emtStd->prtSize.y;
                }

                /* Initial colors. */
                if (rpPrtStdGlobals.offsetEmtPrtCol >= 0)
                {
                    /* Start color. */
                    PRTSTD_SRAND(ir);
                    rr = PRTSTD_RSRAND(ir);
                    color.red = emtPrtCol->prtStartCol.red + (rr * emtPrtCol->prtStartColBias.red);

                    PRTSTD_SRAND(ir);
                    rr = PRTSTD_RSRAND(ir);
                    color.green = emtPrtCol->prtStartCol.green + (rr * emtPrtCol->prtStartColBias.green);

                    PRTSTD_SRAND(ir);
                    rr = PRTSTD_RSRAND(ir);
                    color.blue = emtPrtCol->prtStartCol.blue + (rr * emtPrtCol->prtStartColBias.blue);

                    PRTSTD_SRAND(ir);
                    rr = PRTSTD_RSRAND(ir);
                    color.alpha = emtPrtCol->prtStartCol.alpha + (rr * emtPrtCol->prtStartColBias.alpha);

                    if (rpPrtStdGlobals.offsetPrtCol >= 0)
                    {
                        prtCol = ((RpPrtStdParticleColor *)  RPPRTSTDPROP(prt, rpPrtStdGlobals.offsetPrtCol));

                        /* Start color. */
                        prtCol->startCol.red = color.red;
                        prtCol->startCol.green = color.green;
                        prtCol->startCol.blue =  color.blue;
                        prtCol->startCol.alpha = color.alpha;

                        /* Delta color. */
                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtCol->deltaCol.red = emtPrtCol->prtEndCol.red +
                            (rr * emtPrtCol->prtEndColBias.red) -  prtCol->startCol.red;

                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtCol->deltaCol.green =emtPrtCol->prtEndCol.green +
                            (rr * emtPrtCol->prtEndColBias.green) - prtCol->startCol.green;

                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtCol->deltaCol.blue = emtPrtCol->prtEndCol.blue +
                            (rr * emtPrtCol->prtEndColBias.blue) - prtCol->startCol.blue;

                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtCol->deltaCol.alpha = emtPrtCol->prtEndCol.alpha +
                            (rr * emtPrtCol->prtEndColBias.alpha) - prtCol->startCol.alpha;
                    }
                }

                /* Initial 2D Rotate */
                rot = 0.0f;
                if (rpPrtStdGlobals.offsetEmtPrt2DRot >= 0)
                {
                    /* Start 2D Rotate. */
                    PRTSTD_SRAND(ir);
                    rr = PRTSTD_RSRAND(ir);
                    rot = emtPrt2DRot->prtStart2DRotate + (rr * emtPrt2DRot->prtStart2DRotateBias);

                    /* Clamped to -ve +ve PI */
                    if ((rot > rwPI) || (rot < -rwPI))
                    {
                        /* Quick assumption, rot can never be > 2PI & < -2PI */
                        if (rot > rwPI)
                            rot -= ((RwReal) 2.0 * rwPI);

                        if (rot < -rwPI)
                            rot += ((RwReal) 2.0 * rwPI);
                    }

                    if (rpPrtStdGlobals.offsetPrt2DRot >= 0)
                    {
                        prt2DRot = (RpPrtStdParticle2DRotate *)
                             RPPRTSTDPROP(prt, rpPrtStdGlobals.offsetPrt2DRot);

                        /* Start 2D Rotate. */
                        prt2DRot->start2DRotate = rot;

                        /* Delta 2D Rotate. */
                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prt2DRot->delta2DRotate = (emtPrt2DRot->prtEnd2DRotate +
                            (rr * emtPrt2DRot->prtEnd2DRotateBias)) -  prt2DRot->start2DRotate;
                    }
                }

                /* Initial texcoord. */
                if (rpPrtStdGlobals.offsetEmtPrtTex >= 0)
                {
                    /* Start UV. */
                    PRTSTD_SRAND(ir);
                    rr = PRTSTD_RSRAND(ir);
                    uv[0].u = emtPrtTex->prtStartUV0.u + (rr * emtPrtTex->prtStartUV0Bias.u);
                    PRTSTD_SRAND(ir);
                    rr = PRTSTD_RSRAND(ir);
                    uv[0].v = emtPrtTex->prtStartUV0.v + (rr * emtPrtTex->prtStartUV0Bias.v);

                    PRTSTD_SRAND(ir);
                    rr = PRTSTD_RSRAND(ir);
                    uv[1].u = emtPrtTex->prtStartUV1.u + (rr * emtPrtTex->prtStartUV1Bias.u);
                    PRTSTD_SRAND(ir);
                    rr = PRTSTD_RSRAND(ir);
                    uv[1].v = emtPrtTex->prtStartUV1.v + (rr * emtPrtTex->prtStartUV1Bias.v);

                    /* Start UV. */
                    if (rpPrtStdGlobals.offsetPrtTex >= 0)
                    {
                        prtTex = ((RpPrtStdParticleTexCoords *)
                            RPPRTSTDPROP(prt, rpPrtStdGlobals.offsetPrtTex));

                        prtTex->startUV0.u = uv[0].u;
                        prtTex->startUV0.v = uv[0].v;
                        prtTex->startUV1.u = uv[1].u;
                        prtTex->startUV1.v = uv[1].v;

                        /* Delta UV. */
                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtTex->deltaUV0.u = emtPrtTex->prtEndUV0.u +
                            (rr * emtPrtTex->prtEndUV0Bias.u) - prtTex->startUV0.u;

                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtTex->deltaUV0.v = emtPrtTex->prtEndUV0.v +
                            (rr * emtPrtTex->prtEndUV0Bias.v) - prtTex->startUV0.v;

                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtTex->deltaUV1.u = emtPrtTex->prtEndUV1.u +
                            (rr * emtPrtTex->prtEndUV1Bias.u) - prtTex->startUV1.u;

                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        prtTex->deltaUV1.v = emtPrtTex->prtEndUV1.v +
                            (rr * emtPrtTex->prtEndUV1Bias.v) - prtTex->startUV1.v;
                    }
                }

                /* PTank */
                if (emtPTank)
                {
                    /* Initial pos. */
                    if (emtPTank->dataFlags & emtPTank->emitFlags & rpPTANKDFLAGPOSITION)
                    {
                        ((RwV3d *)pTankPosOut)->x = pos.x;
                        ((RwV3d *)pTankPosOut)->y = pos.y;
                        ((RwV3d *)pTankPosOut)->z = pos.z;
#if (defined(SKY2_DRVMODEL_H) && defined(SKY_USE_STOP_FLAG) )
                        ((RwV4d *)pTankPosOut)->w = 0.0f;
#endif
                        pTankPosOut += pTankPosStride;
                    }

                    if (emtPTank->dataFlags & emtPTank->emitFlags & rpPTANKDFLAGMATRIX)
                    {
                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        v.x = emtPrtMtx->prtPosMtxAt.x + (rr * emtPrtMtx->prtPosMtxAtBias.x);
                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        v.y = emtPrtMtx->prtPosMtxAt.y + (rr * emtPrtMtx->prtPosMtxAtBias.y);
                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        v.z = emtPrtMtx->prtPosMtxAt.z + (rr * emtPrtMtx->prtPosMtxAtBias.z);

                        RwV3dNormalize(&v, &v);

                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        v2.x = emtPrtMtx->prtPosMtxUp.x + (rr * emtPrtMtx->prtPosMtxUpBias.x);
                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        v2.y = emtPrtMtx->prtPosMtxUp.y + (rr * emtPrtMtx->prtPosMtxUpBias.y);
                        PRTSTD_SRAND(ir);
                        rr = PRTSTD_RSRAND(ir);
                        v2.z = emtPrtMtx->prtPosMtxUp.z + (rr * emtPrtMtx->prtPosMtxUpBias.z);

                        RwV3dNormalize(&v2, &v2);

                        vptr = RwMatrixGetAt((RwMatrix *) pTankPosOut);
                        *vptr = v;

                        vptr = RwMatrixGetRight((RwMatrix *) pTankPosOut);
                        RwV3dCrossProduct(vptr, &v, &v2);
                        v2 = *vptr;

                        vptr = RwMatrixGetUp((RwMatrix *) pTankPosOut);
                        RwV3dCrossProduct(vptr, &v2, &v);

                        /* Scale the matrix */
                        RwV3dScale((RwMatrixGetRight((RwMatrix *) pTankPosOut)),
                                   (RwMatrixGetRight((RwMatrix *) pTankPosOut)),
                                   size.x);

                        RwV3dScale((RwMatrixGetUp((RwMatrix *) pTankPosOut)),
                                   (RwMatrixGetUp((RwMatrix *) pTankPosOut)),
                                   size.y);

                        ((RwMatrix *)pTankPosOut)->pos.x = pos.x;
                        ((RwMatrix *)pTankPosOut)->pos.y = pos.y;
                        ((RwMatrix *)pTankPosOut)->pos.z = pos.z;

                        pTankPosOut += pTankPosStride;
                    }

                    /* Initial size. */
                    if (emtPTank->dataFlags & emtPTank->emitFlags & rpPTANKDFLAGSIZE)
                    {
                        ((RwV2d *) pTankSzeOut)->x = size.x;
                        ((RwV2d *) pTankSzeOut)->y = size.y;

                        pTankSzeOut += pTankSzeStride;
                    }

                    /* Initial colors. */
                    if (emtPTank->dataFlags & emtPTank->emitFlags & rpPTANKDFLAGCOLOR)
                    {
                        ((RwRGBA *) pTankColOut)->red = RwInt32FromRealMacro(color.red);
                        ((RwRGBA *) pTankColOut)->green = RwInt32FromRealMacro(color.green);
                        ((RwRGBA *) pTankColOut)->blue = RwInt32FromRealMacro(color.blue);
                        ((RwRGBA *) pTankColOut)->alpha = RwInt32FromRealMacro(color.alpha);

                        pTankColOut += pTankColStride;
                    }

                    /* Initial texcoord. */
                    if (emtPTank->dataFlags & emtPTank->emitFlags & rpPTANKDFLAGVTX2TEXCOORDS)
                    {
                        ((RwTexCoords *) pTankTexOut)[0].u = uv[0].u;
                        ((RwTexCoords *) pTankTexOut)[0].v = uv[0].v;

                        ((RwTexCoords *) pTankTexOut)[1].u = uv[1].u;
                        ((RwTexCoords *) pTankTexOut)[1].v = uv[1].v;

                        pTankTexOut += pTankTexStride;
                    }

                    /* Initial 2D rotate. */
                    if (emtPTank->dataFlags & emtPTank->emitFlags & rpPTANKDFLAG2DROTATE)
                    {
                        (*(RwReal *) pTank2DRotOut) = rot;

                        pTank2DRotOut += pTank2DRotStride;
                    }
                }

                prt += prtStride;
            }

            if (emtPTank)
            {
                emtPTank->numPrt += numPrt;

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

                /* Size. */
                if (emtPTank->dataFlags & rpPTANKDFLAGSIZE)
                {
                    emtPTank->dataOutPtrs[i] = pTankSzeOut;
                    i++;
                }

                /* Matrix. */
                if (emtPTank->dataFlags & rpPTANKDFLAGMATRIX)
                {
                    emtPTank->dataOutPtrs[i] = pTankPosOut;

                    i++;
                }

                /* 2D Rotate. */
                if (emtPTank->dataFlags & rpPTANKDFLAG2DROTATE)
                {
                    emtPTank->dataOutPtrs[i] = pTank2DRotOut;

                    i++;
                }

                /* Texcoord. */
                if (emtPTank->dataFlags & rpPTANKDFLAGVTX2TEXCOORDS)
                {
                    emtPTank->dataOutPtrs[i] = pTankTexOut;

                    i++;
                }
            }

            /* Set the number of active part count. */
            prtBatch->newPrt = prtBatch->numPrt;
            prtBatch->numPrt += numPrt;

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
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterStdCreateCB is the create callback for the standard
 * emitter property.
 *
 * This callback setups the standard emitter properties of an emitter.
 *
 * This callback should be added to an emitter class if the emitter contains
 * any of the standard emitter properties.
 *
 * \param atomic        Pointer to the parent atomic.
 * \param emt           Pointer to the emitter.
 * \param data          Pointer to private data for the emit callback. Not used.
 *
 * \return pointer to the emitter if sucessful, NULL otherwise.
 *
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtStdEmitterStdBeginUpdateCB
 * \see RpPrtStdEmitterStdDestroyCB
 * \see RpPrtStdEmitterStdCloneCB
 * \see RpPrtStdEmitterStdEmitCB
 * \see RpPrtStdEmitterStdEndUpdateCB
 * \see RpPrtStdEmitterStdRenderCB
 * \see RpPrtStdPClassSetCallBack
 * \see RpPrtStdPClassStdCreate
 * \see RpPrtStdParticleStdUpdateCB
 */
RpPrtStdEmitter *
RpPrtStdEmitterStdCreateCB(RpAtomic * atomic __RWUNUSED__,
                                RpPrtStdEmitter *emt,
                                void * data __RWUNUSED__)
{
    RwInt32                     offset;
    RpPrtStdEmitterStandard     *emtStd;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterStdCreateCB"));

    RWASSERT(emt);

    offset = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERSTANDARD);

    if (offset >= 0)
    {
        emtStd = ((RpPrtStdEmitterStandard *) RPPRTSTDPROP(emt, offset));

        emt->flag |= (rpPRTSTDEMITTERFLAGALIVE |
                    rpPRTSTDEMITTERFLAGACTIVE |
                    rpPRTSTDEMITTERFLAGPTANK |
                    rpPRTSTDEMITTERFLAGUPDATE |
                    rpPRTSTDEMITTERFLAGUPDATEPARTICLE |
                    rpPRTSTDEMITTERFLAGEMIT |
                    rpPRTSTDEMITTERFLAGRENDER);

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

        emtStd->prtDelta2DRotate = (RwReal) 0.0;

        emtStd->texture = NULL;
    }


    RWRETURN(emt);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterStdDestroyCB is the destroy callback for the standard
 * emitter property.
 *
 * This callback destroy the standard emitter properties of an emitter.
 *
 * This callback should be added to an emitter class if the emitter contains
 * any of the standard emitter properties.
 *
 * \param atomic        Pointer to the parent atomic.
 * \param emt           Pointer to the emitter.
 * \param data          Pointer to private data for the emit callback. Not used.
 *
 * \return pointer to the emitter if sucessful, NULL otherwise.
 *
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtStdEmitterStdCreateCB
 * \see RpPrtStdEmitterStdDestroyCB
 * \see RpPrtStdEmitterStdBeginUpdateCB
 * \see RpPrtStdEmitterStdCloneCB
 * \see RpPrtStdEmitterStdEmitCB
 * \see RpPrtStdEmitterStdEndUpdateCB
 * \see RpPrtStdEmitterStdRenderCB
 * \see RpPrtStdPClassSetCallBack
 * \see RpPrtStdPClassStdCreate
 * \see RpPrtStdParticleStdUpdateCB
 */
RpPrtStdEmitter *
RpPrtStdEmitterStdDestroyCB(RpAtomic * atomic __RWUNUSED__,
                                RpPrtStdEmitter *emt,
                                void * data __RWUNUSED__)
{
    RwInt32                     offset;
    RpPrtStdEmitterStandard     *emtStd;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterStdDestroyCB"));

    RWASSERT(emt);

    offset = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERSTANDARD);

    if (offset >= 0)
    {
        emtStd = ((RpPrtStdEmitterStandard *) RPPRTSTDPROP(emt, offset));

        if(  (0 != (emt->flag & rpPRTSTDEMITTERFLAGSTREAMREAD)) && (NULL!= emtStd->texture) )
        {
            RwTextureDestroy(emtStd->texture);
        }

    }


    RWRETURN(emt);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterStdCloneCB is the clone callback for the standard
 * emitter property.
 *
 * This callback clones the standard emitter properties of an emitter.
 *
 * This callback should be added to an emitter class if the emitter contains
 * any of the standard emitter properties.
 *
 * \param atomic        Pointer to the parent atomic.
 * \param emt           Pointer to the source emitter.
 * \param data          Pointer to the destination emitter.
 *
 * \return pointer to the destination emitter if sucessful, NULL otherwise.
 *
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtStdEmitterStdBeginUpdateCB
 * \see RpPrtStdEmitterStdEmitCB
 * \see RpPrtStdEmitterStdEndUpdateCB
 * \see RpPrtStdEmitterStdRenderCB
 * \see RpPrtStdPClassSetCallBack
 * \see RpPrtStdPClassStdCreate
 * \see RpPrtStdParticleStdUpdateCB
 */
RpPrtStdEmitter *
RpPrtStdEmitterStdCloneCB(RpAtomic * atomic __RWUNUSED__,
                                RpPrtStdEmitter *emt,
                                void * data)
{
    RwInt32                         srcOffset, dstOffset;
    RpPrtStdEmitter                 * newEmt;
    RpPrtStdEmitterStandard         * emtStd, * newEmtStd;
    RpPrtStdEmitterPrtColor         * emtPrtCol, * newEmtPrtCol;
    RpPrtStdEmitterPrtTexCoords     * emtPrtTex, * newEmtPrtTex;
    RpPrtStdEmitterPrtMatrix        * emtPrtMtx, * newEmtPrtMtx;
    RpPrtStdEmitterPrtSize          * emtPrtSze, * newEmtPrtSze;
    RpPrtStdEmitterPrt2DRotate      * emtPrt2DRot, * newEmtPrt2DRot;
    RpPrtStdEmitterPTank            * emtPTank, *newEmtPTank;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterStdCloneCB"));

    RWASSERT(emt);
    RWASSERT(data);

    newEmt = (RpPrtStdEmitter *) data;

    srcOffset = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,rpPRTSTDPROPERTYCODEEMITTERSTANDARD);
    dstOffset = RpPrtStdPropTabGetPropOffset(newEmt->emtClass->propTab,rpPRTSTDPROPERTYCODEEMITTERSTANDARD);

    if ((srcOffset >= 0) && (dstOffset))
    {
        emtStd = ((RpPrtStdEmitterStandard *) RPPRTSTDPROP(emt, srcOffset));

        newEmtStd = ((RpPrtStdEmitterStandard *) RPPRTSTDPROP(newEmt, dstOffset));

        memcpy(newEmtStd, emtStd, sizeof(RpPrtStdEmitterStandard));

        /* Up the ref count on the texture. */
        if (emtStd->texture != NULL)
            (void) RwTextureAddRef(emtStd->texture);
    }

    srcOffset = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERPRTCOLOR);
    dstOffset = RpPrtStdPropTabGetPropOffset(newEmt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERPRTCOLOR);

    if ((srcOffset >= 0) && (dstOffset >= 0))
    {
        emtPrtCol = ((RpPrtStdEmitterPrtColor *) RPPRTSTDPROP(emt, srcOffset));

        newEmtPrtCol = ((RpPrtStdEmitterPrtColor *) RPPRTSTDPROP(newEmt, dstOffset));

        memcpy(newEmtPrtCol, emtPrtCol, sizeof(RpPrtStdEmitterPrtColor));
    }

    srcOffset = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERPRTTEXCOORDS);
    dstOffset = RpPrtStdPropTabGetPropOffset(newEmt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERPRTTEXCOORDS);

    if ((srcOffset >= 0) && (dstOffset >= 0))
    {
        emtPrtTex = ((RpPrtStdEmitterPrtTexCoords *) RPPRTSTDPROP(emt, srcOffset));

        newEmtPrtTex = ((RpPrtStdEmitterPrtTexCoords *) RPPRTSTDPROP(newEmt, dstOffset));

        memcpy(newEmtPrtTex, emtPrtTex, sizeof(RpPrtStdEmitterPrtTexCoords));
    }

    srcOffset = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERPRTMATRIX);
    dstOffset = RpPrtStdPropTabGetPropOffset(newEmt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERPRTMATRIX);

    if ((srcOffset >= 0) && (dstOffset >= 0))
    {
        emtPrtMtx = ((RpPrtStdEmitterPrtMatrix *) RPPRTSTDPROP(emt, srcOffset));

        newEmtPrtMtx = ((RpPrtStdEmitterPrtMatrix *) RPPRTSTDPROP(newEmt, dstOffset));

        memcpy(newEmtPrtMtx, emtPrtMtx, sizeof(RpPrtStdEmitterPrtMatrix));
    }

    srcOffset = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERPRTSIZE);
    dstOffset = RpPrtStdPropTabGetPropOffset(newEmt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERPRTSIZE);

    if ((srcOffset >= 0) && (dstOffset >= 0))
    {
        emtPrtSze = ((RpPrtStdEmitterPrtSize *) RPPRTSTDPROP(emt, srcOffset));

        newEmtPrtSze = ((RpPrtStdEmitterPrtSize *) RPPRTSTDPROP(newEmt, dstOffset));

        memcpy(newEmtPrtSze, emtPrtSze, sizeof(RpPrtStdEmitterPrtSize));
    }

    srcOffset = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERPRT2DROTATE);
    dstOffset = RpPrtStdPropTabGetPropOffset(newEmt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERPRT2DROTATE);

    if ((srcOffset >= 0) && (dstOffset >= 0))
    {
        emtPrt2DRot = ((RpPrtStdEmitterPrt2DRotate *) RPPRTSTDPROP(emt, srcOffset));

        newEmtPrt2DRot = ((RpPrtStdEmitterPrt2DRotate *) RPPRTSTDPROP(newEmt, dstOffset));

        memcpy(newEmtPrt2DRot, emtPrt2DRot, sizeof(RpPrtStdEmitterPrt2DRotate));
    }

    srcOffset = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERPTANK);
    dstOffset = RpPrtStdPropTabGetPropOffset(newEmt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERPTANK);

    if ((srcOffset >= 0) && (dstOffset >= 0))
    {
        emtPTank = ((RpPrtStdEmitterPTank *) RPPRTSTDPROP(emt, srcOffset));

        newEmtPTank = ((RpPrtStdEmitterPTank *) RPPRTSTDPROP(newEmt, dstOffset));

        _rpPrtStdPTankClone(newEmtPTank, emtPTank);
    }

    RWRETURN(newEmt);
}

/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterStdBeginUpdateCB is the begin update callback for
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
 * \return pointer to the emitter if sucessful, NULL otherwise.
 *
 * \see RpPrtStdAtomicUpdate
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtStdEmitterStdCloneCB
 * \see RpPrtStdEmitterStdCreateCB
 * \see RpPrtStdEmitterStdDestroyCB
 * \see RpPrtStdEmitterStdEmitCB
 * \see RpPrtStdEmitterStdEndUpdateCB
 * \see RpPrtStdEmitterStdRenderCB
 * \see RpPrtStdPClassSetCallBack
 * \see RpPrtStdPClassStdCreate
 */
RpPrtStdEmitter *
RpPrtStdEmitterStdBeginUpdateCB(RpAtomic *atomic,
                                  RpPrtStdEmitter *emt, void *data)
{
    RwReal                      deltaT, rotate;
    RwFrame                     *frame;
    RpPrtStdEmitterStandard     *emtStd;
    RpPrtStdEmitterPTank        *emtPTank;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterStdBeginUpdateCB"));

    RWASSERT(atomic);
    RWASSERT(emt);
    RWASSERT(data);

    RWASSERT(rpPrtStdGlobals.offsetEmtStd >= 0);

    emtStd = ((RpPrtStdEmitterStandard *) RPPRTSTDPROP(emt, rpPrtStdGlobals.offsetEmtStd));

    frame = RpAtomicGetFrame(atomic);
    emtStd->ltm = RwFrameGetLTM(frame);

    deltaT = *(RwReal *)data;
    emtStd->currTime += deltaT;

    if (rpPrtStdGlobals.offsetEmtPTank >= 0)
    {
        emtPTank = ((RpPrtStdEmitterPTank *) RPPRTSTDPROP(emt, rpPrtStdGlobals.offsetEmtPTank));

        if ((emtPTank->updateFlags & rpPTANKDFLAGCNS2DROTATE) && (emtPTank->pTank != NULL))
        {
            rotate = RpPTankAtomicGetConstantRotate(emtPTank->pTank);

            rotate += deltaT * emtStd->prtDelta2DRotate;

            if (rotate > rwPI)
            {
                rotate -= ((RwReal) 2.0 * rwPI);
            }
            else if (rotate < -rwPI)
            {
                rotate += ((RwReal) 2.0 * rwPI);
            }

            RpPTankAtomicSetConstantRotate(emtPTank->pTank, rotate);
        }
    }

    RWRETURN(emt);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterStdEndUpdateCB is the end update callback for
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
 * \return pointer to the emitter if sucessful, NULL otherwise.
 *
 * \see RpPrtStdAtomicUpdate
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtStdEmitterStdBeginUpdateCB
 * \see RpPrtStdEmitterStdCloneCB
 * \see RpPrtStdEmitterStdCreateCB
 * \see RpPrtStdEmitterStdDestroyCB
 * \see RpPrtStdEmitterStdEmitCB
 * \see RpPrtStdEmitterStdRenderCB
 * \see RpPrtStdPClassSetCallBack
 * \see RpPrtStdPClassStdCreate
 */
RpPrtStdEmitter *
RpPrtStdEmitterStdEndUpdateCB(RpAtomic * atomic __RWUNUSED__,
                                   RpPrtStdEmitter *emt,
                                   void * data __RWUNUSED__)
{
    RpPrtStdEmitterStandard       *emtStd;
    RwReal          rr;
    RwInt32         ir;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterStdEndUpdateCB"));

    RWASSERT(emt);

    RWASSERT(rpPrtStdGlobals.offsetEmtStd >= 0);

    emtStd = ((RpPrtStdEmitterStandard *) RPPRTSTDPROP(emt, rpPrtStdGlobals.offsetEmtStd));

    /* Update the time for the next emission. */
    if (emtStd->currTime >= emtStd->emtEmitTime)
    {
        emtStd->emtPrevEmitTime = emtStd->currTime;

        ir = emtStd->seed;
        PRTSTD_SRAND(ir);
        rr = PRTSTD_RSRAND(ir);
        emtStd->emtEmitTime = emtStd->currTime + emtStd->emtEmitGap + (rr * emtStd->emtEmitGapBias);

        emtStd->seed = ir;
    }

    RWRETURN(emt);
}

/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterStdRenderCB is the render callback for
 * the standard emitter property.
 *
 * This callbacks renders the particles emitted by the emitter.
 *
 * This callback should be added to an emitter class if the emitter contains
 * any of the standard emitter properties.
 *
 * \param atomic        Pointer to the parent atomic.
 * \param emt           Pointer to the emitter.
 * \param data          Pointer to private data for the emit callback. Not used.
 *
 * \return pointer to the emitter if sucessful, NULL otherwise.
 *
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEmitterStdBeginUpdateCB
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtStdEmitterStdCloneCB
 * \see RpPrtStdEmitterStdCreateCB
 * \see RpPrtStdEmitterStdDestroyCB
 * \see RpPrtStdEmitterStdEmitCB
 * \see RpPrtStdEmitterStdEndUpdateCB
 * \see RpPrtStdPClassSetCallBack
 * \see RpPrtStdPClassStdCreate
 */
RpPrtStdEmitter *
RpPrtStdEmitterStdRenderCB(RpAtomic * atomic __RWUNUSED__,
                                RpPrtStdEmitter *emt,
                                void * data __RWUNUSED__)
{
    RpPrtStdEmitter             *result;
    RwInt32                     offset;
    RpPrtStdEmitterPTank        *emtPTank;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterStdRenderCB"));

    RWASSERT(emt);

    result = emt;
    if (emt->prtActive > 0)
    {
        offset = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERPTANK);

        if (offset >= 0)
        {
            emtPTank = ((RpPrtStdEmitterPTank *) RPPRTSTDPROP(emt, offset));

            RWASSERT(emtPTank->pTank != NULL);

            /* Render the particles stored in the pTank. */
            if ((emtPTank->numPrt > 0) && (RpAtomicRender(emtPTank->pTank) == NULL))
                result = NULL;
        }
        else
            result = NULL;
    }

    RWRETURN(result);
}

/************************************************************************
 *
 *
 *
 ************************************************************************/

RpPrtStdPropertyTable *
_rpPrtStdEmitterStdPropTabCreate( void )
{
    RpPrtStdPropertyTable       *propTab;
    RwInt32                     prop[2];

    RWFUNCTION(RWSTRING("_rpPrtStdEmitterStdPropTabCreate"));

    propTab = NULL;

    prop[0] = rpPRTSTDPROPERTYCODEEMITTERSTANDARD;
    prop[1] = sizeof(RpPrtStdEmitterStandard);

    propTab = RpPrtStdPropTabCreate(1, prop, &prop[1]);

    RWRETURN(propTab);
}

RpPrtStdPropertyTable *
_rpPrtStdParticleStdPropTabCreate( void )
{
    RpPrtStdPropertyTable       *propTab;
    RwInt32                     prop[2];

    RWFUNCTION(RWSTRING("_rpPrtStdParticleStdPropTabCreate"));

    propTab = NULL;

    prop[0] = rpPRTSTDPROPERTYCODEPARTICLESTANDARD;
    prop[1] = sizeof(RpPrtStdParticleStandard);

    propTab = RpPrtStdPropTabCreate(1, prop, &prop[1]);

    RWRETURN(propTab);
}

/************************************************************************
 *
 *
 *
 ************************************************************************/
/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPClassStdCreateID is a utility function to create
 * an particle class ID.
 *
 * The dataFlag is used to create a unique ID.
 *
 * Note that the ID range [0x00000000] to [0x00FFFFFF] is used internally, using
 * an idea in this range may result in crash and unpredictable behaviors.
 *
 * \verbatim
        rpPRTSTDPARTICLEDATAFLAGSTANDARD
        rpPRTSTDPARTICLEDATAFLAGCOLOR
        rpPRTSTDPARTICLEDATAFLAGTEXCOORDS
        rpPRTSTDPARTICLEDATAFLAGSIZE
        rpPRTSTDPARTICLEDATAFLAGVELOCITY
        rpPRTSTDPARTICLEDATAFLAGMATRIX
   \endverbatim
 *
 * \param dataFlag      A bitfield representing the standard properties to
 *                      be added to the particle class.
 *
 * \return The generated unique ID.
 *
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdPClassCreate
 */
RwInt32
RpPrtStdPClassStdCreateID( RwInt32 dataFlag )
{
    RWAPIFUNCTION(RWSTRING("RpPrtStdPClassStdCreateID"));

    RWRETURN(dataFlag);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPClassStdCreate is a utility function to create
 * a particle class containing a set of standard properties and callbacks.
 *
 * The dataFlag is used to set the properties in the particle class. Not all
 * properties are necessary and some are optional.
 *
 * This function creates a particle class containing standard properties only.
 * Particles with custom properties should use \ref RpPrtStdPClassCreate.
 *
 * A property table is created internally for this class. This property table
 * will be destroyed with the particle class when it is destroyed. The user
 * does not need to destroy the property table explicitly.
 *
 * The possible settings for the data flags are, where each setting represent
 * a property to be present.
 *
 * \verbatim
        rpPRTSTDPARTICLEDATAFLAGSTANDARD
        rpPRTSTDPARTICLEDATAFLAGCOLOR
        rpPRTSTDPARTICLEDATAFLAGTEXCOORDS
        rpPRTSTDPARTICLEDATAFLAGSIZE
        rpPRTSTDPARTICLEDATAFLAGVELOCITY
        rpPRTSTDPARTICLEDATAFLAGMATRIX
   \endverbatim
 *
 * \param dataFlag      A bitfield representing the standard properties to
 *                      be added to the emitter class.
 *
 * \return pointer to the particle class if sucessful, NULL otherwise.
 *
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdPClassCreate
 */
RpPrtStdParticleClass *
RpPrtStdPClassStdCreate( RwInt32 dataFlag )
{
    RpPrtStdParticleClass       *pClass;
    RpPrtStdPropertyTable       *propTab;
    RwInt32                     size, i, id,
                                prop[2 * rpPRTSTDPARTICLEPROPERTYCOUNT];

    RWAPIFUNCTION(RWSTRING("RpPrtStdPClassStdCreate"));

    id = RpPrtStdPClassStdCreateID(dataFlag);

    pClass = RpPrtStdPClassGet(id);
    if (NULL != pClass)
    {
        pClass->refCount++;

        RWRETURN(pClass);
    }

    pClass = RpPrtStdPClassCreate();

    if (pClass)
    {
        i = 0;
        size = 0;

        prop[i] = rpPRTSTDPROPERTYCODEPARTICLESTANDARD;
        prop[rpPRTSTDPARTICLEPROPERTYCOUNT + i] = sizeof(RpPrtStdParticleStandard);
        i++;
        size += sizeof(RpPrtStdParticleStandard);

        if (dataFlag & rpPRTSTDPARTICLEDATAFLAGVELOCITY)
        {
            prop[i] = rpPRTSTDPROPERTYCODEPARTICLEVELOCITY;
            prop[rpPRTSTDPARTICLEPROPERTYCOUNT + i] = sizeof(RwV3d);
            i++;
            size += sizeof(RwV3d);
        }

        if (dataFlag & rpPRTSTDPARTICLEDATAFLAGPOSITION)
        {
            prop[i] = rpPRTSTDPROPERTYCODEPARTICLEPOSITION;
            prop[rpPRTSTDPARTICLEPROPERTYCOUNT + i] = sizeof(RwV3d);
            i++;
            size += sizeof(RwV3d);
        }

        if (dataFlag & rpPRTSTDPARTICLEDATAFLAGCOLOR)
        {
            prop[i] = rpPRTSTDPROPERTYCODEPARTICLECOLOR;
            prop[rpPRTSTDPARTICLEPROPERTYCOUNT + i] = sizeof(RpPrtStdParticleColor);
            i++;
            size += sizeof(RpPrtStdParticleColor);
        }

        if (dataFlag & rpPRTSTDPARTICLEDATAFLAGTEXCOORDS)
        {
            prop[i] = rpPRTSTDPROPERTYCODEPARTICLETEXCOORDS;
            prop[rpPRTSTDPARTICLEPROPERTYCOUNT + i] = sizeof(RpPrtStdParticleTexCoords);
            i++;
            size += sizeof(RpPrtStdParticleTexCoords);
        }

        if (dataFlag &  rpPRTSTDPARTICLEDATAFLAGMATRIX)
        {
            prop[i] = rpPRTSTDPROPERTYCODEPARTICLEMATRIX;
            prop[rpPRTSTDPARTICLEPROPERTYCOUNT + i] = sizeof(RwMatrix);
            i++;
            size += sizeof(RwMatrix);
        }

        if (dataFlag &  rpPRTSTDPARTICLEDATAFLAGSIZE)
        {
            prop[i] = rpPRTSTDPROPERTYCODEPARTICLESIZE;
            prop[rpPRTSTDPARTICLEPROPERTYCOUNT + i] = sizeof(RpPrtStdParticleSize);
            i++;
            size += sizeof(RpPrtStdParticleSize);
        }

        if (dataFlag &  rpPRTSTDPARTICLEDATAFLAG2DROTATE)
        {
            prop[i] = rpPRTSTDPROPERTYCODEPARTICLE2DROTATE;
            prop[rpPRTSTDPARTICLEPROPERTYCOUNT + i] = sizeof(RpPrtStdParticle2DRotate);
            i++;
            size += sizeof(RpPrtStdParticle2DRotate);
        }

        RWASSERT(i > 0);
        RWASSERT(size > 0);

        propTab = RpPrtStdPropTabCreate(i, prop, &prop[rpPRTSTDPARTICLEPROPERTYCOUNT]);

        if (propTab == NULL)
            RWRETURN((RpPrtStdParticleClass *) NULL);

        RpPrtStdPClassSetPropTab(pClass, propTab);

        pClass->id = id;

        /* Destroy the prop tab. The particle class now owns
         * the property table and will destroy it when it is
         * destroyed.
         */
        RpPrtStdPropTabDestroy(propTab);

        /* Set up the standard callbacks. */
#if (defined(SKY2_DRVMODEL_H) && !defined(NOASM))
        RpPrtStdPClassStdSetupSkyCB(pClass);
#else
        RpPrtStdPClassStdSetupCB(pClass);
#endif /* (defined(SKY2_DRVMODEL_H) && !defined(NOASM)) */

    }

    RWRETURN(pClass);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEClassStdCreateID is a utility function to create
 * an emitter class ID.
 *
 * The dataFlag is used to create a unique ID.
 *
 * Note that the ID range [0x00000000] to [0x00FFFFFF] is used internally, using
 * an idea in this range may result in crash and unpredictable behaviors.
 *
 * \verbatim
        rpPRTSTDEMITTERDATAFLAGSTANDARD
        rpPRTSTDEMITTERDATAFLAGPRTCOLOR
        rpPRTSTDEMITTERDATAFLAGPRTTEXCOORDS
        rpPRTSTDEMITTERDATAFLAGPRTSIZE
        rpPRTSTDEMITTERDATAFLAGPTANK
        rpPRTSTDEMITTERDATAFLAGPRTMATRIX
   \endverbatim
 *
 * \param dataFlag      A bitfield representing the standard properties to
 *                      be added to the emitter class.
 *
 * \return The generated unique ID.
 *
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdPClassCreate
 */
RwInt32
RpPrtStdEClassStdCreateID( RwInt32 dataFlag )
{
    RWAPIFUNCTION(RWSTRING("RpPrtStdEClassStdCreateID"));

    RWRETURN(dataFlag);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEClassStdCreate is a utility function to create
 * an emitter class containing a set of standard properties and callbacks.
 *
 * The dataFlag is used to set the properties in the emitter class. Not all
 * properties are necessary and some are optional.
 *
 * This function creates an emitter class containing standard properties only.
 * Emitters with custom properties should use \ref RpPrtStdEClassCreate.
 *
 * A property table is created internally for this class. This property table
 * will be destroyed with the emitter class when it is destroyed. The user
 * does not need to destroy the property table explicitly.
 *
 * The possible settings for the data flags are, where each setting represent
 * a property to be present.
 *
 * \verbatim
        rpPRTSTDEMITTERDATAFLAGSTANDARD
        rpPRTSTDEMITTERDATAFLAGPRTCOLOR
        rpPRTSTDEMITTERDATAFLAGPRTTEXCOORDS
        rpPRTSTDEMITTERDATAFLAGPRTSIZE
        rpPRTSTDEMITTERDATAFLAGPTANK
        rpPRTSTDEMITTERDATAFLAGPRTMATRIX
   \endverbatim
 *
 * \param dataFlag      A bitfield representing the standard properties to
 *                      be added to the emitter class.
 *
 * \return pointer to the emitter class if sucessful, NULL otherwise.
 *
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdPClassCreate
 */
RpPrtStdEmitterClass *
RpPrtStdEClassStdCreate( RwInt32 dataFlag )
{
    RpPrtStdEmitterClass                *eClass;
    RpPrtStdPropertyTable               *propTab;
    RwInt32                             size, i, id,
                                        prop[2 * rpPRTSTDEMITTERPROPERTYCOUNT];
    RWAPIFUNCTION(RWSTRING("RpPrtStdEClassStdCreate"));

    id = RpPrtStdEClassStdCreateID(dataFlag);

    eClass = RpPrtStdEClassGet(id);
    if (NULL != eClass)
    {
        eClass->refCount++;

        RWRETURN(eClass);
    }

    eClass = RpPrtStdEClassCreate();

    if (eClass)
    {
        i = 0;
        size = 0;

        prop[0] = rpPRTSTDPROPERTYCODEEMITTER;
        prop[rpPRTSTDEMITTERPROPERTYCOUNT] = sizeof(RpPrtStdEmitter);
        i++;
        size += sizeof(RpPrtStdEmitter);

        if (dataFlag & rpPRTSTDEMITTERDATAFLAGSTANDARD)
        {
            prop[i] = rpPRTSTDPROPERTYCODEEMITTERSTANDARD;
            prop[rpPRTSTDEMITTERPROPERTYCOUNT + i] = sizeof(RpPrtStdEmitterStandard);
            i++;
            size += sizeof(RpPrtStdEmitterStandard);
        }

        if (dataFlag & rpPRTSTDEMITTERDATAFLAGPRTCOLOR)
        {
            prop[i] = rpPRTSTDPROPERTYCODEEMITTERPRTCOLOR;
            prop[rpPRTSTDEMITTERPROPERTYCOUNT + i] = sizeof(RpPrtStdEmitterPrtColor);
            i++;
            size += sizeof(RpPrtStdEmitterPrtColor);
        }

        if (dataFlag & rpPRTSTDEMITTERDATAFLAGPRTTEXCOORDS)
        {
            prop[i] = rpPRTSTDPROPERTYCODEEMITTERPRTTEXCOORDS;
            prop[rpPRTSTDEMITTERPROPERTYCOUNT + i] = sizeof(RpPrtStdEmitterPrtTexCoords);
            i++;
            size += sizeof(RpPrtStdEmitterPrtTexCoords);
        }

        if (dataFlag & rpPRTSTDEMITTERDATAFLAGPTANK)
        {
            prop[i] = rpPRTSTDPROPERTYCODEEMITTERPTANK;
            prop[rpPRTSTDEMITTERPROPERTYCOUNT + i] = sizeof(RpPrtStdEmitterPTank);
            i++;
            size += sizeof(RpPrtStdEmitterPTank);
        }

        if (dataFlag & rpPRTSTDEMITTERDATAFLAGPRTMATRIX)
        {
            prop[i] = rpPRTSTDPROPERTYCODEEMITTERPRTMATRIX;
            prop[rpPRTSTDEMITTERPROPERTYCOUNT + i] = sizeof(RpPrtStdEmitterPrtMatrix);
            i++;
            size += sizeof(RpPrtStdEmitterPrtMatrix);
        }

        if (dataFlag & rpPRTSTDEMITTERDATAFLAGPRTSIZE)
        {
            prop[i] = rpPRTSTDPROPERTYCODEEMITTERPRTSIZE;
            prop[rpPRTSTDEMITTERPROPERTYCOUNT + i] = sizeof(RpPrtStdEmitterPrtSize);
            i++;
            size += sizeof(RpPrtStdEmitterPrtSize);
        }

        if (dataFlag & rpPRTSTDEMITTERDATAFLAGPRT2DROTATE)
        {
            prop[i] = rpPRTSTDPROPERTYCODEEMITTERPRT2DROTATE;
            prop[rpPRTSTDEMITTERPROPERTYCOUNT + i] = sizeof(RpPrtStdEmitterPrt2DRotate);
            i++;
            size += sizeof(RpPrtStdEmitterPrt2DRotate);
        }

        RWASSERT(i > 0);
        RWASSERT(size > 0);

        propTab = RpPrtStdPropTabCreate(i, prop, &prop[rpPRTSTDEMITTERPROPERTYCOUNT]);

        if (propTab == NULL)
            RWRETURN((RpPrtStdEmitterClass *) NULL);

        RpPrtStdEClassSetPropTab(eClass, propTab);

        eClass->id = id;

        /* Destroy the prop tab. The particle class now owns
         * the property table and will destroy it when it is
         * destroyed.
         */
        RpPrtStdPropTabDestroy(propTab);

        /* Set up the standard callbacks. */
#if (defined(SKY2_DRVMODEL_H) && !defined(NOASM))
        RpPrtStdEClassStdSetupSkyCB(eClass);
#else
        RpPrtStdEClassStdSetupCB(eClass);
#endif /* (defined(SKY2_DRVMODEL_H) && !defined(NOASM)) */

    }

    RWRETURN(eClass);
}

/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterStdStreamReadCB is the stream in callback for
 * the standard particle property.
 *
 * This function streams in an emitter containing standard properties.
 *
 * This callback should be added to a emitter class if the emitter contains
 * any of the standard emitter properties.
 *
 * \param atomic            A pointer to the emitter's parent atomic.
 * \param emt               A pointer to the emitter to stream into.
 * \param data              A pointer to the input stream.
 *
 * \return pointer to the emitter if sucessful, NULL otherwise.
 *
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtStdEmitterStdStreamGetSizeCB
 * \see RpPrtStdEmitterStdStreamReadCB
 * \see RpPrtStdEmitterStdStreamWriteCB
 */
RpPrtStdEmitter *
RpPrtStdEmitterStdStreamReadCB(RpAtomic * atomic __RWUNUSED__,
                                    RpPrtStdEmitter *emt, void *data)
{
    RwInt32                         i, offset;
    RwStream                        *stream;

    RpPrtStdPropertyTable           *propTab;
    RpPrtStdEmitterClass            *eClass;

    RpPrtStdEmitterStandard         *emtStd;
    RpPrtStdEmitterPrtColor         *emtPrtCol;
    RpPrtStdEmitterPrtTexCoords     *emtPrtTex;
    RpPrtStdEmitterPrtMatrix        *emtPrtMtx;
    RpPrtStdEmitterPrtSize          *emtPrtSize;
    RpPrtStdEmitterPrt2DRotate      *emtPrt2DRot;
    RpPrtStdEmitterPTank            *emtPTank;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterStdStreamReadCB"));

    RWASSERT(emt);
    RWASSERT(data);

    stream = (RwStream *) data;

    eClass = emt->emtClass;
    propTab = eClass->propTab;

    /* Get the various properties */

    /* Read in the standard prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab, rpPRTSTDPROPERTYCODEEMITTERSTANDARD);

    if (offset >= 0)
    {
        emtStd = ((RpPrtStdEmitterStandard *) RPPRTSTDPROP(emt, offset));

        /* Random seed. */
        RwStreamReadInt32(stream, &emtStd->seed, sizeof(RwInt32));

        /* Max particles. */
        RwStreamReadInt32(stream, &emtStd->maxPrt, sizeof(RwInt32));

        /* External force */
        RwStreamReadReal(stream, (RwReal *) &emtStd->force, sizeof(RwV3d));

        /* Emitter's position and dimension */
        RwStreamReadReal(stream, (RwReal *) &emtStd->emtPos, sizeof(RwV3d));
        RwStreamReadReal(stream, (RwReal *) &emtStd->emtSize, sizeof(RwV3d));

        /* Emission gap */
        RwStreamReadReal(stream, &emtStd->emtEmitGap, sizeof(RwReal));
        RwStreamReadReal(stream, &emtStd->emtEmitGapBias, sizeof(RwReal));

        /* Particle mission rate */
        RwStreamReadInt32(stream, &emtStd->emtPrtEmit, sizeof(RwInt32));
        RwStreamReadInt32(stream, &emtStd->emtPrtEmitBias, sizeof(RwInt32));

        /* Particle initial speed */
        RwStreamReadReal(stream, &emtStd->prtInitVel, sizeof(RwReal));
        RwStreamReadReal(stream, &emtStd->prtInitVelBias, sizeof(RwReal));

        /* Particle life */
        RwStreamReadReal(stream, &emtStd->prtLife, sizeof(RwReal));
        RwStreamReadReal(stream, &emtStd->prtLifeBias, sizeof(RwReal));

        /* Particle emission direction */
        RwStreamReadReal(stream, (RwReal *) &emtStd->prtInitDir, sizeof(RwV3d));
        RwStreamReadReal(stream, (RwReal *) &emtStd->prtInitDirBias, sizeof(RwV3d));

        /* Particle constant size */
        RwStreamReadReal(stream, (RwReal *) &emtStd->prtSize, sizeof(RwV3d));

        /* Particle constant color */
        RwStreamReadInt32(stream, &i, sizeof(RwInt32));
        RPPRTSTDINT32T0RGBA(&emtStd->prtColor, &i);

        /* Particle constant tex coords */
        for (i = 0; i < 4; i++)
        {
            RwStreamReadReal(stream, (RwReal *) &emtStd->prtUV[i], sizeof(RwTexCoords));
        }

        /* Particle texture */
        RwStreamReadInt32(stream, &i, sizeof(RwInt32));
        if (i > 0)
        {
            if (RwStreamFindChunk(stream, rwID_TEXTURE, (RwUInt32 *)NULL, (RwUInt32 *)NULL))
            {
                emtStd->texture = RwTextureStreamRead(stream);
                RWASSERT(NULL != emtStd->texture);
            }
        }

        /* Particle global rotation */

        /*
         * Version 3 was when 2D rotation was introduced. So we only read
         * the 2D rotation setting if the file is version 3 or newer.
         */
        if (rpPrtStdGlobals.ver >= 3)
        {
            RwStreamReadReal(stream, (RwReal *) &emtStd->prtDelta2DRotate, sizeof(RwReal));
        }

    };

    /* Particle color */
    offset = RpPrtStdPropTabGetPropOffset(propTab, rpPRTSTDPROPERTYCODEEMITTERPRTCOLOR);

    if (offset >= 0)
    {
        emtPrtCol = ((RpPrtStdEmitterPrtColor *) RPPRTSTDPROP(emt, offset));

        /* Start and end color */
        RwStreamReadReal(stream, (RwReal *) &emtPrtCol->prtStartCol, sizeof(RwRGBAReal));
        RwStreamReadReal(stream, (RwReal *) &emtPrtCol->prtStartColBias, sizeof(RwRGBAReal));

        RwStreamReadReal(stream, (RwReal *) &emtPrtCol->prtEndCol, sizeof(RwRGBAReal));
        RwStreamReadReal(stream, (RwReal *) &emtPrtCol->prtEndColBias, sizeof(RwRGBAReal));
    }

    /* Particle texture coords */
    offset = RpPrtStdPropTabGetPropOffset(propTab, rpPRTSTDPROPERTYCODEEMITTERPRTTEXCOORDS);

    if (offset >= 0)
    {
        emtPrtTex = ((RpPrtStdEmitterPrtTexCoords *) RPPRTSTDPROP(emt, offset));

        /* Start and end tex coords */
        RwStreamReadReal(stream, (RwReal *) &emtPrtTex->prtStartUV0, sizeof(RwTexCoords));
        RwStreamReadReal(stream, (RwReal *) &emtPrtTex->prtStartUV0Bias, sizeof(RwTexCoords));

        RwStreamReadReal(stream, (RwReal *) &emtPrtTex->prtEndUV0, sizeof(RwTexCoords));
        RwStreamReadReal(stream, (RwReal *) &emtPrtTex->prtEndUV0Bias, sizeof(RwTexCoords));

        /* Start and end tex coords */
        RwStreamReadReal(stream, (RwReal *) &emtPrtTex->prtStartUV1, sizeof(RwTexCoords));
        RwStreamReadReal(stream, (RwReal *) &emtPrtTex->prtStartUV1Bias, sizeof(RwTexCoords));

        RwStreamReadReal(stream, (RwReal *) &emtPrtTex->prtEndUV1, sizeof(RwTexCoords));
        RwStreamReadReal(stream, (RwReal *) &emtPrtTex->prtEndUV1Bias, sizeof(RwTexCoords));
    }

    /* Particle matrix */
    offset = RpPrtStdPropTabGetPropOffset(propTab, rpPRTSTDPROPERTYCODEEMITTERPRTMATRIX);

    if (offset >= 0)
    {
        emtPrtMtx = ((RpPrtStdEmitterPrtMatrix *) RPPRTSTDPROP(emt, offset));

        RwStreamFindChunk(stream, rwID_MATRIX, (RwUInt32 *)NULL, (RwUInt32 *)NULL);

        /* Constant transform */
        RwMatrixStreamRead(stream, &emtPrtMtx->prtCnsMtx);

        RwStreamReadReal(stream, (RwReal *) &emtPrtMtx->prtPosMtxAt, sizeof(RwV3d));
        RwStreamReadReal(stream, (RwReal *) &emtPrtMtx->prtPosMtxAtBias, sizeof(RwV3d));
        RwStreamReadReal(stream, (RwReal *) &emtPrtMtx->prtPosMtxUp, sizeof(RwV3d));
        RwStreamReadReal(stream, (RwReal *) &emtPrtMtx->prtPosMtxUpBias, sizeof(RwV3d));

        RwStreamReadInt32(stream, (RwInt32 *) &emtPrtMtx->flags, sizeof(RwInt32));
    }

    /* Particle size */
    offset = RpPrtStdPropTabGetPropOffset(propTab, rpPRTSTDPROPERTYCODEEMITTERPRTSIZE);

    if (offset >= 0)
    {
        emtPrtSize = ((RpPrtStdEmitterPrtSize *) RPPRTSTDPROP(emt, offset));

        RwStreamReadReal(stream, (RwReal *) &emtPrtSize->prtStartSize, sizeof(RwV2d));
        RwStreamReadReal(stream, (RwReal *) &emtPrtSize->prtStartSizeBias, sizeof(RwV2d));
        RwStreamReadReal(stream, (RwReal *) &emtPrtSize->prtEndSize, sizeof(RwV2d));
        RwStreamReadReal(stream, (RwReal *) &emtPrtSize->prtEndSizeBias, sizeof(RwV2d));
    }

    /* Particle 2D Rotate */
    offset = RpPrtStdPropTabGetPropOffset(propTab, rpPRTSTDPROPERTYCODEEMITTERPRT2DROTATE);

    if (offset >= 0)
    {
        emtPrt2DRot = ((RpPrtStdEmitterPrt2DRotate *) RPPRTSTDPROP(emt, offset));

        RwStreamReadReal(stream, (RwReal *) &emtPrt2DRot->prtStart2DRotate, sizeof(RwReal));
        RwStreamReadReal(stream, (RwReal *) &emtPrt2DRot->prtStart2DRotateBias, sizeof(RwReal));
        RwStreamReadReal(stream, (RwReal *) &emtPrt2DRot->prtEnd2DRotate, sizeof(RwReal));
        RwStreamReadReal(stream, (RwReal *) &emtPrt2DRot->prtEnd2DRotateBias, sizeof(RwReal));
    }

    /* Emitter PTank */
    offset = RpPrtStdPropTabGetPropOffset(propTab, rpPRTSTDPROPERTYCODEEMITTERPTANK);

    if (offset >= 0)
    {
        emtPTank = ((RpPrtStdEmitterPTank *) RPPRTSTDPROP(emt, offset));

        _rpPrtStdPTankStreamRead(stream, emtPTank);
    }


    RWRETURN(emt);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterStdStreamWriteCB is the stream out callback for
 * the standard particle property.
 *
 * This function streams out an emitter containing standard properties.
 *
 * This callback should be added to a emitter class if the emitter contains
 * any of the standard emitter properties.
 *
 * \param atomic            A pointer to the emitter's parent atomic.
 * \param emt               A pointer to the emitter.
 * \param data              A pointer to output stream.
 *
 * \return pointer to the emitter if sucessful, NULL otherwise.
 *
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtStdEmitterStdStreamGetSizeCB
 * \see RpPrtStdEmitterStdStreamReadCB
 * \see RpPrtStdEmitterStdStreamWriteCB
 */
RpPrtStdEmitter *
RpPrtStdEmitterStdStreamWriteCB(RpAtomic * atomic __RWUNUSED__,
                                     RpPrtStdEmitter *emt, void *data)
{
    RwInt32                         i, offset;
    RwStream                        *stream;

    RpPrtStdPropertyTable           *propTab;
    RpPrtStdEmitterClass            *eClass;

    RpPrtStdEmitterStandard         *emtStd;
    RpPrtStdEmitterPrtColor         *emtPrtCol;
    RpPrtStdEmitterPrtTexCoords     *emtPrtTex;
    RpPrtStdEmitterPrtMatrix        *emtPrtMtx;
    RpPrtStdEmitterPrtSize          *emtPrtSize;
    RpPrtStdEmitterPrt2DRotate      *emtPrt2DRot;
    RpPrtStdEmitterPTank            *emtPTank;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterStdStreamWriteCB"));

    RWASSERT(emt);
    RWASSERT(data);

    stream = (RwStream *) data;

    eClass = emt->emtClass;
    propTab = eClass->propTab;

    /* Standard props */
    offset = RpPrtStdPropTabGetPropOffset(propTab, rpPRTSTDPROPERTYCODEEMITTERSTANDARD);

    if (offset >= 0)
    {
        emtStd = ((RpPrtStdEmitterStandard *) RPPRTSTDPROP(emt, offset));

        /* Random seed. */
        RwStreamWriteInt32(stream, &emtStd->seed, sizeof(RwInt32));

        /* Max particle. */
        RwStreamWriteInt32(stream, &emtStd->maxPrt, sizeof(RwInt32));

        /* External force */
        RwStreamWriteReal(stream, (RwReal *) &emtStd->force, sizeof(RwV3d));

        /* Emitter's position and dimension */
        RwStreamWriteReal(stream, (RwReal *) &emtStd->emtPos, sizeof(RwV3d));
        RwStreamWriteReal(stream, (RwReal *) &emtStd->emtSize, sizeof(RwV3d));

        /* Emission gap */
        RwStreamWriteReal(stream, &emtStd->emtEmitGap, sizeof(RwReal));
        RwStreamWriteReal(stream, &emtStd->emtEmitGapBias, sizeof(RwReal));

        /* Particle mission rate */
        RwStreamWriteInt32(stream, &emtStd->emtPrtEmit, sizeof(RwInt32));
        RwStreamWriteInt32(stream, &emtStd->emtPrtEmitBias, sizeof(RwInt32));

        /* Particle initial speed */
        RwStreamWriteReal(stream, (RwReal *) &emtStd->prtInitVel, sizeof(RwReal));
        RwStreamWriteReal(stream, (RwReal *) &emtStd->prtInitVelBias, sizeof(RwReal));

        /* Particle life */
        RwStreamWriteReal(stream, (RwReal *) &emtStd->prtLife, sizeof(RwReal));
        RwStreamWriteReal(stream, (RwReal *) &emtStd->prtLifeBias, sizeof(RwReal));

        /* Particle emission direction */
        RwStreamWriteReal(stream, (RwReal *) &emtStd->prtInitDir, sizeof(RwV3d));
        RwStreamWriteReal(stream, (RwReal *) &emtStd->prtInitDirBias, sizeof(RwV3d));

        /* Particle constant size */
        RwStreamWriteReal(stream, (RwReal *) &emtStd->prtSize, sizeof(RwV3d));

        /* Particle constant color */
        RPPRTSTDRGBATOINT32(&emtStd->prtColor, &i);
        RwStreamWriteInt32(stream, &i, sizeof(RwInt32));

        /* Particle constant tex coords */
        for (i = 0; i < 4; i++)
        {
            RwStreamWriteReal(stream, (RwReal *) &emtStd->prtUV[i], sizeof(RwTexCoords));
        }

        /* Particle texture */
        if (emtStd->texture)
        {
            i = 1;
            RwStreamWriteInt32(stream, &i, sizeof(RwInt32));
            RwTextureStreamWrite(emtStd->texture,stream);
        }
        else
        {
            i = 0;
            RwStreamWriteInt32(stream, &i, sizeof(RwInt32));
        }

        /* Particle constant 2D rotation */
        RwStreamWriteReal(stream, (RwReal *) &emtStd->prtDelta2DRotate, sizeof(RwReal));

    }

    /* Particle color */
    offset = RpPrtStdPropTabGetPropOffset(propTab, rpPRTSTDPROPERTYCODEEMITTERPRTCOLOR);

    if (offset >= 0)
    {
        emtPrtCol = ((RpPrtStdEmitterPrtColor *) RPPRTSTDPROP(emt, offset));

        /* Start and end color */
        RwStreamWriteReal(stream, (RwReal *) &emtPrtCol->prtStartCol, sizeof(RwRGBAReal));
        RwStreamWriteReal(stream, (RwReal *) &emtPrtCol->prtStartColBias, sizeof(RwRGBAReal));

        RwStreamWriteReal(stream, (RwReal *) &emtPrtCol->prtEndCol, sizeof(RwRGBAReal));
        RwStreamWriteReal(stream, (RwReal *) &emtPrtCol->prtEndColBias, sizeof(RwRGBAReal));
    }

    /* Particle texture coords */
    offset = RpPrtStdPropTabGetPropOffset(propTab, rpPRTSTDPROPERTYCODEEMITTERPRTTEXCOORDS);

    if (offset >= 0)
    {
        emtPrtTex = ((RpPrtStdEmitterPrtTexCoords *) RPPRTSTDPROP(emt, offset));

        /* Start and end tex coords */
        RwStreamWriteReal(stream, (RwReal *) &emtPrtTex->prtStartUV0, sizeof(RwTexCoords));
        RwStreamWriteReal(stream, (RwReal *) &emtPrtTex->prtStartUV0Bias, sizeof(RwTexCoords));

        RwStreamWriteReal(stream, (RwReal *) &emtPrtTex->prtEndUV0, sizeof(RwTexCoords));
        RwStreamWriteReal(stream, (RwReal *) &emtPrtTex->prtEndUV0Bias, sizeof(RwTexCoords));

        /* Start and end tex coords */
        RwStreamWriteReal(stream, (RwReal *) &emtPrtTex->prtStartUV1, sizeof(RwTexCoords));
        RwStreamWriteReal(stream, (RwReal *) &emtPrtTex->prtStartUV1Bias, sizeof(RwTexCoords));

        RwStreamWriteReal(stream, (RwReal *) &emtPrtTex->prtEndUV1, sizeof(RwTexCoords));
        RwStreamWriteReal(stream, (RwReal *) &emtPrtTex->prtEndUV1Bias, sizeof(RwTexCoords));
    }

    /* Particle matrix */
    offset = RpPrtStdPropTabGetPropOffset(propTab, rpPRTSTDPROPERTYCODEEMITTERPRTMATRIX);

    if (offset >= 0)
    {
        emtPrtMtx = ((RpPrtStdEmitterPrtMatrix *) RPPRTSTDPROP(emt, offset));

        /* Constant transform */
        RwMatrixStreamWrite(&emtPrtMtx->prtCnsMtx, stream);

        RwStreamWriteReal(stream, (RwReal *) &emtPrtMtx->prtPosMtxAt, sizeof(RwV3d));
        RwStreamWriteReal(stream, (RwReal *) &emtPrtMtx->prtPosMtxAtBias, sizeof(RwV3d));
        RwStreamWriteReal(stream, (RwReal *) &emtPrtMtx->prtPosMtxUp, sizeof(RwV3d));
        RwStreamWriteReal(stream, (RwReal *) &emtPrtMtx->prtPosMtxUpBias, sizeof(RwV3d));

        RwStreamWriteInt32(stream, (RwInt32 *) &emtPrtMtx->flags, sizeof(RwInt32));
    }

    /* Particle size */
    offset = RpPrtStdPropTabGetPropOffset(propTab, rpPRTSTDPROPERTYCODEEMITTERPRTSIZE);

    if (offset >= 0)
    {
        emtPrtSize = ((RpPrtStdEmitterPrtSize *) RPPRTSTDPROP(emt, offset));

        RwStreamWriteReal(stream, (RwReal *) &emtPrtSize->prtStartSize, sizeof(RwV2d));
        RwStreamWriteReal(stream, (RwReal *) &emtPrtSize->prtStartSizeBias, sizeof(RwV2d));
        RwStreamWriteReal(stream, (RwReal *) &emtPrtSize->prtEndSize, sizeof(RwV2d));
        RwStreamWriteReal(stream, (RwReal *) &emtPrtSize->prtEndSizeBias, sizeof(RwV2d));
    }

    /* Particle 2D Rotate */
    offset = RpPrtStdPropTabGetPropOffset(propTab, rpPRTSTDPROPERTYCODEEMITTERPRT2DROTATE);

    if (offset >= 0)
    {
        emtPrt2DRot = ((RpPrtStdEmitterPrt2DRotate *) RPPRTSTDPROP(emt, offset));

        RwStreamWriteReal(stream, (RwReal *) &emtPrt2DRot->prtStart2DRotate, sizeof(RwReal));
        RwStreamWriteReal(stream, (RwReal *) &emtPrt2DRot->prtStart2DRotateBias, sizeof(RwReal));
        RwStreamWriteReal(stream, (RwReal *) &emtPrt2DRot->prtEnd2DRotate, sizeof(RwReal));
        RwStreamWriteReal(stream, (RwReal *) &emtPrt2DRot->prtEnd2DRotateBias, sizeof(RwReal));
    }

    /* Emitter PTank */
    offset = RpPrtStdPropTabGetPropOffset(propTab, rpPRTSTDPROPERTYCODEEMITTERPTANK);

    if (offset >= 0)
    {
        emtPTank = ((RpPrtStdEmitterPTank *) RPPRTSTDPROP(emt, offset));

        _rpPrtStdPTankStreamWrite(stream, emtPTank);
    }

    RWRETURN(emt);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterStdStreamGetSizeCB is the callback used to determine
 * the size in bytes of the binary representation of an emitter. This value is
 * used in the binary chunk header to indicate the size of the chunk.
 *
 * This callback should be added to a emitter class if the emitter contains
 * any of the standard emitter properties.
 *
 * \param atomic            A pointer to the emitter's parent atomic.
 * \param emt               A pointer to the emitter.
 * \param data              A pointer to a \ref RwInt32 to return the stream size.
 *
 * \return pointer to the emitter if sucessful, NULL otherwise.
 *
 * \see RpPrtStdEClassSetCallBack
 * \see RpPrtStdEClassStdCreate
 * \see RpPrtStdEmitterStdStreamGetSizeCB
 * \see RpPrtStdEmitterStdStreamReadCB
 * \see RpPrtStdEmitterStdStreamWriteCB
 */
RpPrtStdEmitter *
RpPrtStdEmitterStdStreamGetSizeCB(RpAtomic * atomic __RWUNUSED__,
                                       RpPrtStdEmitter *emt,
                                       void * data)
{
    RwInt32                          totalSize;
    RwInt32                          offset;

    RpPrtStdPropertyTable           *propTab;
    RpPrtStdEmitterClass            *eClass;

    RpPrtStdEmitterStandard         *emtStd;
    RpPrtStdEmitterPrtMatrix        *emtPrtMtx;
    RpPrtStdEmitterPTank            *emtPTank;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterStdStreamGetSizeCB"));

    RWASSERT(emt);
    RWASSERT(data);

    totalSize = 0;

    eClass = emt->emtClass;
    propTab = eClass->propTab;

    /* Get size of the standard prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab, rpPRTSTDPROPERTYCODEEMITTERSTANDARD);

    if (offset >= 0)
    {
        emtStd = ((RpPrtStdEmitterStandard *) RPPRTSTDPROP(emt, offset));

        /* Random seed. */
        totalSize += sizeof(RwInt32);

        /* Max particle. */
        totalSize += sizeof(RwInt32);

        /* External force */
        totalSize += sizeof(RwV3d);

        /* Emitter's position and dimension */
        totalSize += sizeof(RwV3d);
        totalSize += sizeof(RwV3d);

        /* Emission gap */
        totalSize += sizeof(RwReal);
        totalSize += sizeof(RwReal);

        /* Particle mission rate */
        totalSize += sizeof(RwInt32);
        totalSize += sizeof(RwInt32);

        /* Particle initial speed */
        totalSize += sizeof(RwReal);
        totalSize += sizeof(RwReal);

        /* Particle life */
        totalSize += sizeof(RwReal);
        totalSize += sizeof(RwReal);

        /* Particle emission direction */
        totalSize += sizeof(RwV3d);
        totalSize += sizeof(RwV3d);

        /* Particle constant size */
        totalSize += sizeof(RwV3d);

        /* Particle constant color */
        totalSize += sizeof(RwInt32);

        /* Particle constant tex coords */
        totalSize += sizeof(RwTexCoords)*4 ;

        /* Particle texture */
        totalSize += sizeof(RwInt32);
        if (emtStd->texture)
        {
            totalSize += RwTextureStreamGetSize(emtStd->texture);
            totalSize += rwCHUNKHEADERSIZE;
        }

        /* Particle 2D rotation */
        totalSize += sizeof(RwReal);
    }

    /* Particle color */
    offset = RpPrtStdPropTabGetPropOffset(propTab, rpPRTSTDPROPERTYCODEEMITTERPRTCOLOR);

    if (offset >= 0)
        totalSize += 4 * sizeof(RwRGBAReal);

    /* Particle texture coords */
    offset = RpPrtStdPropTabGetPropOffset(propTab, rpPRTSTDPROPERTYCODEEMITTERPRTTEXCOORDS);

    if (offset >= 0)
        totalSize += 8 * sizeof(RwTexCoords);

    /* Particle matrix */
    offset = RpPrtStdPropTabGetPropOffset(propTab, rpPRTSTDPROPERTYCODEEMITTERPRTMATRIX);

    if (offset >= 0)
    {
        emtPrtMtx = (RpPrtStdEmitterPrtMatrix *) RPPRTSTDPROP(emt, offset);

        /* Transform matrix */
        totalSize += RwMatrixStreamGetSize(&emtPrtMtx->prtCnsMtx);
        totalSize += rwCHUNKHEADERSIZE;

        /* Particle matrix building vectors */
        totalSize += 4 * sizeof(RwV3d);

        /* Flag */
        totalSize += sizeof(RwInt32);
    }

    /* Particle size */
    offset = RpPrtStdPropTabGetPropOffset(propTab, rpPRTSTDPROPERTYCODEEMITTERPRTSIZE);

    if (offset >= 0)
        totalSize += 4 * sizeof(RwV2d);

    /* Particle 2D rotate */
    offset = RpPrtStdPropTabGetPropOffset(propTab, rpPRTSTDPROPERTYCODEEMITTERPRT2DROTATE);

    if (offset >= 0)
        totalSize += 4 * sizeof(RwReal);

    /* PTank */
    offset = RpPrtStdPropTabGetPropOffset(propTab, rpPRTSTDPROPERTYCODEEMITTERPTANK);

    if (offset >= 0)
    {
        emtPTank = (RpPrtStdEmitterPTank *) RPPRTSTDPROP(emt, offset);

        totalSize += _rpPrtStdPTankStreamGetSize(emtPTank);
    }

    *(RwInt32 *) data = totalSize;

    RWRETURN(emt);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEClassStdSetupCB is the callback function to
 * restore the emitter class callback functions for standard emitter
 * properties.
 *
 * This function is called after an emitter class is streamed in.
 *
 * This callback should be used if the emitter cle class contains the standard
 * emitter properties.
 *
 * \param eClass        A pointer to an emitter class for the restoring of the callback.
 *
 * \return pointer to the emitter class if sucessful, NULL otherwise.
 *
 * \see RpPrtStdGetEClassSetupCallBack
 * \see RpPrtStdSetEClassSetupCallBack
 */
RpPrtStdEmitterClass *
RpPrtStdEClassStdSetupCB(RpPrtStdEmitterClass *eClass)
{
    RwInt32                             i;
    RpPrtStdEmitterCallBackArray        emtCB[1];

    RWAPIFUNCTION(RWSTRING("RpPrtStdEClassStdSetupCB"));

    RWASSERT(eClass);

    for (i = 0; i < rpPRTSTDEMITTERCALLBACKMAX; i++)
        emtCB[0][i] = NULL;

    emtCB[0][rpPRTSTDEMITTERCALLBACKCREATE] = RpPrtStdEmitterStdCreateCB;
    emtCB[0][rpPRTSTDEMITTERCALLBACKDESTROY] = RpPrtStdEmitterStdDestroyCB;
    emtCB[0][rpPRTSTDEMITTERCALLBACKEMIT] = RpPrtStdEmitterStdEmitCB;
    emtCB[0][rpPRTSTDEMITTERCALLBACKBEGINUPDATE] = RpPrtStdEmitterStdBeginUpdateCB;
    emtCB[0][rpPRTSTDEMITTERCALLBACKENDUPDATE] = RpPrtStdEmitterStdEndUpdateCB;
    emtCB[0][rpPRTSTDEMITTERCALLBACKBEGINRENDER] = RpPrtStdEmitterStdRenderCB;
    emtCB[0][rpPRTSTDEMITTERCALLBACKSTREAMREAD] = RpPrtStdEmitterStdStreamReadCB;
    emtCB[0][rpPRTSTDEMITTERCALLBACKSTREAMWRITE] = RpPrtStdEmitterStdStreamWriteCB;
    emtCB[0][rpPRTSTDEMITTERCALLBACKSTREAMGETSIZE] = RpPrtStdEmitterStdStreamGetSizeCB;
    emtCB[0][rpPRTSTDEMITTERCALLBACKCLONE] = RpPrtStdEmitterStdCloneCB;

    RpPrtStdEClassSetCallBack(eClass, 1, emtCB);

    RWRETURN(eClass);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdPClassStdSetupCB is the callback function to
 * restore the particle class callback functions for standard particle
 * properties.
 *
 * This function is called after a particle class is streamed in.
 *
 * This callback should be used if the particle class contains the standard
 * particle properties.
 *
 * \param pClass        A pointer to a particle class for the restoring of the callback.
 *
 * \return pointer to the particle class if sucessful, NULL otherwise.
 *
 * \see RpPrtStdGetPClassSetupCallBack
 * \see RpPrtStdSetPClassSetupCallBack
 */
RpPrtStdParticleClass *
RpPrtStdPClassStdSetupCB(RpPrtStdParticleClass *pClass)
{
    RwInt32                             i;
    RpPrtStdParticleCallBackArray       prtCB[1];
    RWAPIFUNCTION(RWSTRING("RpPrtStdPClassStdSetupCB"));

    RWASSERT(pClass);

    for (i = 0; i < rpPRTSTDPARTICLECALLBACKMAX; i++)
        prtCB[0][i] = NULL;

    prtCB[0][rpPRTSTDPARTICLECALLBACKUPDATE] = RpPrtStdParticleStdUpdateCB;

    RpPrtStdPClassSetCallBack(pClass, 1, prtCB);

    RWRETURN(pClass);
}

