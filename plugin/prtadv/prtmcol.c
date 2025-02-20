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

#include "prtmcol.h"


/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup prtadvmultiprop
 * \ref RpPrtAdvPrtMultiPropUpdateCB is the update callback for the
 * multiple-property particle property.
 *
 * This callback updates the multiple-property properties in a particle.
 *
 * This callback should be added to a particle class if the particle contains
 * the particle's multiple-property properties.
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
RpPrtAdvPrtMultiPropUpdateCB(RpPrtStdEmitter *emt,
                             RpPrtStdParticleBatch *prtBatch,
                             void *data)
{
    RpPrtStdEmitterStandard           *emtStd;

    RpPrtAdvEmtPrtMultiColor      *emtPrtMultiCol=NULL;
    RpPrtAdvEmtPrtMultiTexCoords  *emtPrtMultiTex=NULL;
    RpPrtAdvEmtPrtMultiSize       *emtPrtMultiSize=NULL;

    RpPrtStdParticleColor             *prtColIn;
    RpPrtStdParticleTexCoords         *prtTexIn;
    RpPrtStdParticleSize              *prtSizeIn;

    RpPrtAdvPrtMultiColor        *prtMultiColIn;
    RpPrtAdvPrtMultiTexCoords    *prtMultiTexIn;
    RpPrtAdvPrtMultiSize         *prtMultiSizeIn;

    RwChar                            *prtIn;

    RwInt32                           numPrtIn;
    RwInt32                           prtStride;
    RwInt32                           prtMultiColSize = 0;
    RwInt32                           colTimeSize = 0;
    RwInt32                           colSize = 0;

    RwInt32                           prtMultiTexSize = 0;
    RwInt32                           texTimeSize = 0;
    RwInt32                           texSize = 0;

    RwInt32                           prtMultiSizeSize = 0;
    RwInt32                           sizeTimeSize = 0;
    RwInt32                           sizeSize = 0;

    RwReal                              currTime, deltaT, *multiTime;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvPrtMultiPropUpdateCB"));

    RWASSERT(emt);
    RWASSERT(prtBatch);

    deltaT = *(RwReal *)data;

    /* Get the emitter's properties */
    _rpPrtAdvEmitterGetPropOffset(emt);

    RWASSERT(rpPrtAdvGlobals.offsetEmtStd >= 0);

    emtStd = ((RpPrtStdEmitterStandard *)
               RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtStd));

    if (rpPrtAdvGlobals.offsetEmtPrtMultiCol >= 0)
    {
        emtPrtMultiCol = ((RpPrtAdvEmtPrtMultiColor *)
            RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtPrtMultiCol));

        colTimeSize = emtPrtMultiCol->numCol * sizeof(RwReal);
        colSize = emtPrtMultiCol->numCol * sizeof(RwRGBAReal);

        prtMultiColSize = sizeof(RpPrtAdvPrtMultiColor) +
            colTimeSize + (2 * colSize);
    }

    if (rpPrtAdvGlobals.offsetEmtPrtMultiTex >= 0)
    {
        emtPrtMultiTex = ((RpPrtAdvEmtPrtMultiTexCoords *)
             RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtPrtMultiTex));

        texTimeSize = emtPrtMultiTex->numTex * sizeof(RwReal);
        texSize = emtPrtMultiTex->numTex * sizeof(RwTexCoords);

        prtMultiTexSize = sizeof(RpPrtAdvPrtMultiTexCoords) +
            texTimeSize + (4 * texSize);
    }

    if (rpPrtAdvGlobals.offsetEmtPrtMultiSize >= 0)
    {
        emtPrtMultiSize = ((RpPrtAdvEmtPrtMultiSize *)
            RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtPrtMultiSize));

        sizeTimeSize = emtPrtMultiSize->numSize * sizeof(RwReal);
        sizeSize = emtPrtMultiSize->numSize * sizeof(RwV2d);

        prtMultiSizeSize = sizeof(RpPrtAdvPrtMultiSize) +
            sizeTimeSize + (2 * sizeSize);
    }

    /* Particles update loop. */
    prtStride = emt->prtClass->objSize;
    prtIn = ((RwChar *)prtBatch) + prtBatch->offset;

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

            /* check for multiple colors */
            if (rpPrtAdvGlobals.offsetPrtMultiCol >= 0)
            {
                prtMultiColIn = (RpPrtAdvPrtMultiColor *)
                    RPPRTADVPROP(prtIn, rpPrtAdvGlobals.offsetPrtMultiCol);

                if (prtMultiColIn->currIndex < emtPrtMultiCol->numCol)
                {
                    multiTime = (RwReal *)
                        (((RwChar *) prtMultiColIn) +
                            sizeof(RpPrtAdvPrtMultiColor));

                    /* Update the prt color interpolants */
                    if (currTime >=
                            multiTime[prtMultiColIn->currIndex])
                    {
                        if (rpPrtAdvGlobals.offsetPrtCol >= 0)
                        {
                            prtColIn = (RpPrtStdParticleColor *)
                                    RPPRTADVPROP(prtIn,
                                                rpPrtAdvGlobals.offsetPrtCol);

                            prtColIn->startCol = *(RwRGBAReal *)
                                (((RwChar *) prtMultiColIn) +
                                 emtPrtMultiCol->offsetStartCol +
                                 (prtMultiColIn->currIndex * sizeof(RwRGBAReal)));

                            prtColIn->deltaCol = *(RwRGBAReal *)
                                (((RwChar *) prtMultiColIn) +
                                 emtPrtMultiCol->offsetDeltaCol +
                                 (prtMultiColIn->currIndex * sizeof(RwRGBAReal)));

                            prtMultiColIn->currIndex++;
                        }
                    }
                }
            }


            /* check for multiple tex co-ords */
            if (rpPrtAdvGlobals.offsetPrtMultiTex >= 0)
            {
                prtMultiTexIn = (RpPrtAdvPrtMultiTexCoords *)
                    RPPRTADVPROP(prtIn, rpPrtAdvGlobals.offsetPrtMultiTex);

                if (prtMultiTexIn->currIndex < emtPrtMultiTex->numTex)
                {
                    multiTime = (RwReal *)
                        (((RwChar *) prtMultiTexIn) +
                            sizeof(RpPrtAdvPrtMultiTexCoords));

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

                            prtTexIn->deltaUV0 = *(RwTexCoords *)
                                (((RwChar *) prtMultiTexIn) +
                                 emtPrtMultiTex->offsetDeltaUV0 +
                                 (prtMultiTexIn->currIndex * sizeof(RwTexCoords)));

                            prtTexIn->startUV1 = *(RwTexCoords *)
                                (((RwChar *) prtMultiTexIn) +
                                 emtPrtMultiTex->offsetStartUV1 +
                                 (prtMultiTexIn->currIndex * sizeof(RwTexCoords)));

                            prtTexIn->deltaUV1 = *(RwTexCoords *)
                                (((RwChar *) prtMultiTexIn) +
                                 emtPrtMultiTex->offsetDeltaUV1 +
                                 (prtMultiTexIn->currIndex * sizeof(RwTexCoords)));

                            prtMultiTexIn->currIndex++;
                        }
                    }
                }
            }

            /* check for multiple size */
            if (rpPrtAdvGlobals.offsetPrtMultiSize >= 0)
            {
                prtMultiSizeIn = (RpPrtAdvPrtMultiSize *)
                    RPPRTADVPROP(prtIn, rpPrtAdvGlobals.offsetPrtMultiSize);

                if (prtMultiSizeIn->currIndex < emtPrtMultiSize->numSize)
                {
                    multiTime = (RwReal *)
                        (((RwChar *) prtMultiSizeIn) +
                            sizeof(RpPrtAdvPrtMultiSize));

                    /* Update the prt uv interpolants */
                    if (currTime >=
                        multiTime[prtMultiSizeIn->currIndex])
                    {
                        if (rpPrtAdvGlobals.offsetPrtSize >= 0)
                        {
                            prtSizeIn = (RpPrtStdParticleSize *)
                                    RPPRTADVPROP(prtIn,
                                                rpPrtAdvGlobals.offsetPrtSize);

                            prtSizeIn->startSize = *(RwV2d *)
                                (((RwChar *) prtMultiSizeIn) +
                                 emtPrtMultiSize->offsetStartSize +
                                 (prtMultiSizeIn->currIndex * (sizeof(RwV2d))));

                            prtSizeIn->deltaSize = *(RwV2d *)
                                (((RwChar *) prtMultiSizeIn) +
                                 emtPrtMultiSize->offsetDeltaSize +
                                 (prtMultiSizeIn->currIndex * (sizeof(RwV2d))));

                            prtMultiSizeIn->currIndex++;
                        }
                    }
                }
            }
        }

        /* Next input prt */
        prtIn += prtStride;
    }

    RWRETURN(prtBatch);
}

/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup prtadvmultiprop
 * \ref RpPrtAdvEmtMultiPropEmitCB is the emit callback for the multiple-property
 * emitter properties.
 *
 * This callback emits new particles using the current emitter. The particle
 * class must contain the multiple-property property.
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
RpPrtAdvEmtMultiPropEmitCB(RpAtomic * atomic __RWUNUSED__,
                              RpPrtStdEmitter *emt,
                              void * data __RWUNUSED__ )
{
    RwReal          rr;
    RwInt32         ir, numPrt, i, j, k, prtStride;

    RwInt32                           prtMultiColSize = 0;
    RwInt32                           colTimeSize = 0;
    RwInt32                           colSize = 0;

    RwInt32                           prtMultiTexSize = 0;
    RwInt32                           texTimeSize = 0;
    RwInt32                           texSize = 0;

    RwInt32                           prtMultiSizeSize = 0;
    RwInt32                           sizeTimeSize = 0;
    RwInt32                           sizeSize = 0;

    RpPrtStdEmitter                     *result;
    RpPrtStdParticleBatch               *prtBatch;

    RpPrtStdEmitterStandard             *emtStd;

    RpPrtAdvEmtPrtMultiColor        *emtPrtMultiCol = NULL;
    RpPrtAdvEmtPrtMultiTexCoords    *emtPrtMultiTex = NULL;
    RpPrtAdvEmtPrtMultiSize         *emtPrtMultiSize = NULL;

    RwChar                              *prt;
    RpPrtStdParticleStandard            *prtStd;
    RpPrtStdParticleColor               *prtCol;
    RpPrtStdParticleTexCoords           *prtTex;
    RpPrtStdParticleSize                *prtSize;

    RpPrtAdvPrtMultiColor          *prtMultiCol;
    RpPrtAdvPrtMultiTexCoords      *prtMultiTex;
    RpPrtAdvPrtMultiSize           *prtMultiSize;

    RwReal                              t0, t1, t2, dt, *time;
    RwRGBAReal                          color0, color1, color2, color3, dcolor, *startCol, *deltaCol;
    RwTexCoords                         tl_uv0, tl_uv1, tl_uv2, tl_uv3, tl_duv, *startUV0, *deltaUV0;
    RwTexCoords                         br_uv0, br_uv1, br_uv2, br_uv3, br_duv, *startUV1, *deltaUV1;
    RwV2d                               size0, size1, size2, size3, dsize, *startSize, *deltaSize;

    RpPrtAdvEmtPrtColorItem         *currentItem;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtMultiPropEmitCB"));

    RWASSERT(atomic);
    RWASSERT(emt);
    RWASSERT(data);

    result = NULL;

    _rpPrtAdvEmitterGetPropOffset(emt);

    RWASSERT(rpPrtAdvGlobals.offsetEmtStd >= 0);
    emtStd = ((RpPrtStdEmitterStandard *)
              RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtStd));

    if (rpPrtAdvGlobals.offsetEmtPrtMultiCol >= 0)
    {
        emtPrtMultiCol = ((RpPrtAdvEmtPrtMultiColor *)
            RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtPrtMultiCol));

        colTimeSize = emtPrtMultiCol->numCol * sizeof(RwReal);
        colSize = emtPrtMultiCol->numCol * sizeof(RwRGBAReal);

        prtMultiColSize = sizeof(RpPrtAdvPrtMultiColor) +
            colTimeSize + (2 * colSize);
    }

    if (rpPrtAdvGlobals.offsetEmtPrtMultiTex >= 0)
    {
        emtPrtMultiTex = ((RpPrtAdvEmtPrtMultiTexCoords *)
             RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtPrtMultiTex));

        texTimeSize = emtPrtMultiTex->numTex * sizeof(RwReal);
        texSize = emtPrtMultiTex->numTex * sizeof(RwTexCoords);

        prtMultiTexSize = sizeof(RpPrtAdvPrtMultiTexCoords) +
            texTimeSize + (4 * texSize);
    }

    if (rpPrtAdvGlobals.offsetEmtPrtMultiSize >= 0)
    {
        emtPrtMultiSize = ((RpPrtAdvEmtPrtMultiSize *)
            RPPRTADVPROP(emt, rpPrtAdvGlobals.offsetEmtPrtMultiSize));

        sizeTimeSize = emtPrtMultiSize->numSize * sizeof(RwReal);
        sizeSize = emtPrtMultiSize->numSize * sizeof(RwV2d);

        prtMultiSizeSize = sizeof(RpPrtAdvPrtMultiSize) +
            sizeTimeSize + (2 * sizeSize);
    }

    /* Check if we are to emit particles. */
    if (emtStd->currTime >= emtStd->emtEmitTime)
    {
        /* Grab the new particle for the new particles. */
        prtBatch = emt->activeBatch;

        if (prtBatch != NULL && -1 != prtBatch->newPrt)
        {
            if (prtBatch->newPrt >= 0)
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
                        /* Multiple colors */
                        if (rpPrtAdvGlobals.offsetPrtMultiCol >= 0)
                        {
                            RWASSERT(rpPrtAdvGlobals.offsetPrtCol >= 0);

                            prtCol = ((RpPrtStdParticleColor *)
                                RPPRTADVPROP(prt, rpPrtAdvGlobals.offsetPrtCol));

                            prtMultiCol = ((RpPrtAdvPrtMultiColor *)
                                RPPRTADVPROP(prt, rpPrtAdvGlobals.offsetPrtMultiCol));

                            /* Set up the pointers */
                            time = (RwReal *)
                                (((RwChar *) prtMultiCol) +
                                    sizeof(RpPrtAdvPrtMultiColor));
                            startCol = (RwRGBAReal *)
                                (((RwChar *) time) + colTimeSize);
                            deltaCol = (RwRGBAReal *)
                                (((RwChar *) startCol) + colSize);

                            /* Save last color. */
                            color3.red = prtCol->startCol.red + prtCol->deltaCol.red;
                            color3.green = prtCol->startCol.green + prtCol->deltaCol.green;
                            color3.blue = prtCol->startCol.blue + prtCol->deltaCol.blue;
                            color3.alpha = prtCol->startCol.alpha + prtCol->deltaCol.alpha;

                            currentItem = &emtPrtMultiCol->list[0];

                            /* Active time */
                            PRTSTD_SRAND(ir);
                            rr = PRTSTD_RSRAND(ir);
                            t1 = currentItem->time +
                                    (rr * currentItem->timeBias);

                            /* Next color. */
                            PRTSTD_SRAND(ir);
                            rr = PRTSTD_RSRAND(ir);
                            color1.red = currentItem->midCol.red +
                                    (rr * currentItem->midColBias.red);

                            PRTSTD_SRAND(ir);
                            rr = PRTSTD_RSRAND(ir);
                            color1.green = currentItem->midCol.green +
                                    (rr * currentItem->midColBias.green);

                            PRTSTD_SRAND(ir);
                            rr = PRTSTD_RSRAND(ir);
                            color1.blue = currentItem->midCol.blue +
                                    (rr * currentItem->midColBias.blue);

                            PRTSTD_SRAND(ir);
                            rr = PRTSTD_RSRAND(ir);
                            color1.alpha = currentItem->midCol.alpha +
                                    (rr * currentItem->midColBias.alpha);

                            color0 = color1;
                            t0 = t1;

                            RWASSERT(emtPrtMultiCol->numCol > 0);

                            for (j = 0; j < (RwInt32) (emtPrtMultiCol->numCol - 1); j++)
                            {
                                k = j + 1;
                                currentItem = &emtPrtMultiCol->list[k];

                                /* Active time */
                                PRTSTD_SRAND(ir);
                                rr = PRTSTD_RSRAND(ir);
                                t2 = currentItem->time +
                                        (rr * currentItem->timeBias);

                                time[j] = t1 * prtStd->endTime;

                                /* Duration */
                                dt = t2 - t1;
                                if (dt > 0.0)
                                    dt = (RwReal) 1.0 / (t2 - t1);
                                else
                                    dt = (RwReal) 0.0;

                                /* Next color. */
                                PRTSTD_SRAND(ir);
                                rr = PRTSTD_RSRAND(ir);
                                color2.red = currentItem->midCol.red +
                                    (rr * currentItem->midColBias.red);

                                PRTSTD_SRAND(ir);
                                rr = PRTSTD_RSRAND(ir);
                                color2.green = currentItem->midCol.green +
                                    (rr * currentItem->midColBias.green);

                                PRTSTD_SRAND(ir);
                                rr = PRTSTD_RSRAND(ir);
                                color2.blue = currentItem->midCol.blue +
                                    (rr * currentItem->midColBias.blue);

                                PRTSTD_SRAND(ir);
                                rr = PRTSTD_RSRAND(ir);
                                color2.alpha = currentItem->midCol.alpha +
                                    (rr * currentItem->midColBias.alpha);

                                dcolor.red = (color2.red - color1.red) * dt;
                                dcolor.green = (color2.green - color1.green) * dt;
                                dcolor.blue = (color2.blue - color1.blue) * dt;
                                dcolor.alpha = (color2.alpha - color1.alpha) * dt;

                                /* Start color */
                                startCol[j].red =
                                    color1.red - (dcolor.red * t1);
                                startCol[j].green =
                                    color1.green - (dcolor.green * t1);
                                startCol[j].blue =
                                    color1.blue - (dcolor.blue * t1);
                                startCol[j].alpha =
                                    color1.alpha - (dcolor.alpha * t1);

                                /* Delta color. */
                                deltaCol[j].red = dcolor.red;
                                deltaCol[j].green = dcolor.green;
                                deltaCol[j].blue = dcolor.blue;
                                deltaCol[j].alpha = dcolor.alpha;

                                /* Next col = next start col */
                                color1 = color2;
                                t1 = t2;
                            }

                            /* Last color */
                            {
                                /* Duration */
                                t2 = (RwReal) 1.0;

                                time[j] = t1 * prtStd->endTime;

                                dt = t2 - t1;
                                if (dt > 0.0)
                                    dt = (RwReal) 1.0 / (t2 - t1);
                                else
                                    dt = (RwReal) 0.0;

                                dcolor.red = (color3.red - color1.red) * dt;
                                dcolor.green = (color3.green - color1.green) * dt;
                                dcolor.blue = (color3.blue - color1.blue) * dt;
                                dcolor.alpha = (color3.alpha - color1.alpha) * dt;

                                /* Start color */
                                startCol[j].red = color1.red - (dcolor.red * t1);
                                startCol[j].green = color1.green - (dcolor.green * t1);
                                startCol[j].blue = color1.blue - (dcolor.blue * t1);
                                startCol[j].alpha = color1.alpha - (dcolor.alpha * t1);

                                /* Delta color. */
                                deltaCol[j].red = dcolor.red;
                                deltaCol[j].green = dcolor.green;
                                deltaCol[j].blue = dcolor.blue;
                                deltaCol[j].alpha = dcolor.alpha;
                            }

                            prtMultiCol->currIndex = 0;

                            /* Recalculate the first delta color */
                            {
                                dt = (RwReal) 1.0 / t0;

                                prtCol->deltaCol.red =
                                    (color0.red - prtCol->startCol.red) * dt;
                                prtCol->deltaCol.green =
                                    (color0.green - prtCol->startCol.green) * dt;
                                prtCol->deltaCol.blue =
                                    (color0.blue - prtCol->startCol.blue) * dt;
                                prtCol->deltaCol.alpha =
                                    (color0.alpha - prtCol->startCol.alpha) * dt;
                            }
                        }

                        /* Multiple uvs */
                        if (rpPrtAdvGlobals.offsetPrtMultiTex >= 0)
                        {
                            RWASSERT(rpPrtAdvGlobals.offsetPrtTex >= 0);

                            prtTex = ((RpPrtStdParticleTexCoords *)
                                RPPRTADVPROP(prt, rpPrtAdvGlobals.offsetPrtTex));

                            prtMultiTex = ((RpPrtAdvPrtMultiTexCoords *)
                                RPPRTADVPROP(prt, rpPrtAdvGlobals.offsetPrtMultiTex));

                            /* Set up the pointers */
                            time = (RwReal *)
                                (((RwChar *) prtMultiTex) +
                                    sizeof(RpPrtAdvPrtMultiTexCoords));

                            startUV0 = (RwTexCoords *)
                                (((RwChar *) time) + texTimeSize);
                            deltaUV0 = (RwTexCoords *)
                                (((RwChar *) startUV0) + texSize);
                            startUV1 = (RwTexCoords *)
                                (((RwChar *) deltaUV0) + texSize);
                            deltaUV1 = (RwTexCoords *)
                                (((RwChar *) startUV1) + texSize);

                            /* Save last uv. */
                            tl_uv3.u = prtTex->startUV0.u + prtTex->deltaUV0.u;
                            tl_uv3.v = prtTex->startUV0.v + prtTex->deltaUV0.v;
                            br_uv3.u = prtTex->startUV1.u + prtTex->deltaUV1.u;
                            br_uv3.v = prtTex->startUV1.v + prtTex->deltaUV1.v;

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

                            tl_uv0 = tl_uv1;
                            br_uv0 = br_uv1;
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

                                tl_duv.u = (tl_uv2.u - tl_uv1.u) * dt;
                                tl_duv.v = (tl_uv2.v - tl_uv1.v) * dt;
                                br_duv.u = (br_uv2.u - br_uv1.u) * dt;
                                br_duv.v = (br_uv2.v - br_uv1.v) * dt;

                                /* Start uv */
                                startUV0[j].u = tl_uv1.u - (tl_duv.u * t1);
                                startUV0[j].v = tl_uv1.v - (tl_duv.v * t1);
                                startUV1[j].u = br_uv1.u - (br_duv.u * t1);
                                startUV1[j].v = br_uv1.v - (br_duv.v * t1);

                                /* Delta uv. */
                                deltaUV0[j].u = tl_duv.u;
                                deltaUV0[j].v = tl_duv.v;
                                deltaUV1[j].u = br_duv.u;
                                deltaUV1[j].v = br_duv.v;

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

                                dt = t2 - t1;
                                if (dt > 0.0)
                                    dt = (RwReal) 1.0 / (t2 - t1);
                                else
                                    dt = (RwReal) 0.0;

                                tl_duv.u = (tl_uv3.u - tl_uv1.u) * dt;
                                tl_duv.v = (tl_uv3.v - tl_uv1.v) * dt;
                                br_duv.u = (br_uv3.u - br_uv1.u) * dt;
                                br_duv.v = (br_uv3.v - br_uv1.v) * dt;

                                /* Start uv */
                                startUV0[j].u = tl_uv1.u - (tl_duv.u * t1);
                                startUV0[j].v = tl_uv1.v - (tl_duv.v * t1);
                                startUV1[j].u = br_uv1.u - (br_duv.u * t1);
                                startUV1[j].v = br_uv1.v - (br_duv.v * t1);

                                /* Delta uv. */
                                deltaUV0[j].u = tl_duv.u;
                                deltaUV0[j].v = tl_duv.v;
                                deltaUV1[j].u = br_duv.u;
                                deltaUV1[j].v = br_duv.v;
                            }

                            prtMultiTex->currIndex = 0;

                            /* Recalculate the first delta uv */
                            {
                                dt = (RwReal) 1.0 / t0;

                                prtTex->deltaUV0.u =
                                    (tl_uv0.u - prtTex->startUV0.u) * dt;
                                prtTex->deltaUV0.v =
                                    (tl_uv0.v - prtTex->startUV0.v) * dt;
                                prtTex->deltaUV1.u =
                                    (br_uv0.u - prtTex->startUV1.u) * dt;
                                prtTex->deltaUV1.v =
                                    (br_uv0.v - prtTex->startUV1.v) * dt;
                            }
                        }

                        /* Multiple size */
                        if (rpPrtAdvGlobals.offsetPrtMultiSize >= 0)
                        {
                            prtSize = ((RpPrtStdParticleSize *)
                                RPPRTADVPROP(prt, rpPrtAdvGlobals.offsetPrtSize));

                            prtMultiSize = ((RpPrtAdvPrtMultiSize *)
                                RPPRTADVPROP(prt, rpPrtAdvGlobals.offsetPrtMultiSize));

                            /* Set up the pointers */
                            time = (RwReal *)
                                (((RwChar *) prtMultiSize) +
                                    sizeof(RpPrtAdvPrtMultiSize));

                            startSize = (RwV2d *)
                                (((RwChar *) time) + sizeTimeSize);
                            deltaSize = (RwV2d *)
                                (((RwChar *) startSize) + sizeSize);

                            /* Save last uv. */
                            size3.x = prtSize->startSize.x + prtSize->deltaSize.x;
                            size3.y = prtSize->startSize.y + prtSize->deltaSize.y;

                            /* Active time */
                            PRTSTD_SRAND(ir);
                            rr = PRTSTD_RSRAND(ir);
                            t1 = emtPrtMultiSize->list[0].time +
                                (rr * emtPrtMultiSize->list[0].timeBias);

                            /* Next uv. */
                            PRTSTD_SRAND(ir);
                            rr = PRTSTD_RSRAND(ir);
                            size1.x = emtPrtMultiSize->list[0].midSize.x +
                                (rr * emtPrtMultiSize->list[0].midSizeBias.x);

                            PRTSTD_SRAND(ir);
                            rr = PRTSTD_RSRAND(ir);
                            size1.y = emtPrtMultiSize->list[0].midSize.y +
                                (rr * emtPrtMultiSize->list[0].midSizeBias.y);

                            size0 = size1;
                            t0 = t1;

                            RWASSERT(emtPrtMultiSize->numSize > 0);

                            for (j = 0; j < (RwInt32) (emtPrtMultiSize->numSize - 1); j++)
                            {
                                k = j + 1;

                                /* Active time */
                                PRTSTD_SRAND(ir);
                                rr = PRTSTD_RSRAND(ir);
                                t2 = emtPrtMultiSize->list[k].time +
                                    (rr * emtPrtMultiSize->list[k].timeBias);

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
                                size2.x = emtPrtMultiSize->list[k].midSize.x +
                                    (rr * emtPrtMultiSize->list[k].midSizeBias.x);

                                PRTSTD_SRAND(ir);
                                rr = PRTSTD_RSRAND(ir);
                                size2.y = emtPrtMultiSize->list[k].midSize.y +
                                    (rr * emtPrtMultiSize->list[k].midSizeBias.y);


                                dsize.x = (size2.x - size1.x) * dt;
                                dsize.y = (size2.y - size1.y) * dt;

                                /* Start uv */
                                startSize[j].x = size1.x - (dsize.x * t1);
                                startSize[j].y = size1.y - (dsize.y * t1);

                                /* Delta uv. */
                                deltaSize[j].x = dsize.x;
                                deltaSize[j].y = dsize.y;

                                /* Next size = next start size */
                                size1 = size2;
                                t1 = t2;
                            }

                            /* Last size */
                            {
                                /* Duration */
                                t2 = (RwReal) 1.0;

                                time[j] = t1 * prtStd->endTime;

                                dt = t2 - t1;
                                if (dt > 0.0)
                                    dt = (RwReal) 1.0 / (t2 - t1);
                                else
                                    dt = (RwReal) 0.0;

                                dsize.x = (size3.x - size1.x) * dt;
                                dsize.y = (size3.y - size1.y) * dt;

                                /* Start uv */
                                startSize[j].x = size1.x - (dsize.x * t1);
                                startSize[j].y = size1.y - (dsize.y * t1);

                                /* Delta uv. */
                                deltaSize[j].x = dsize.x;
                                deltaSize[j].y = dsize.y;
                            }

                            prtMultiSize->currIndex = 0;

                            /* Recalculate the first delta size */
                            {
                                dt = (RwReal) 1.0 / t0;

                                prtSize->deltaSize.x =
                                    (size0.x - prtSize->startSize.x) * dt;
                                prtSize->deltaSize.y =
                                    (size0.y - prtSize->startSize.y) * dt;
                            }
                        }
                    }

                    prt += prtStride;
                }

                /* Update the seed. */
                emtStd->seed = ir;
            }
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
 * \ingroup prtadvmultiprop
 * \ref RpPrtAdvEmtMultiPropCloneCB is the clone callback for the multiple-property
 * emitter properties.
 *
 * This callback clones the multi-property of the emitter.
 *
 * This callback should be added \e after the standard callback.
 *
 * All other properties of the particles should be handled by the previous
 * clone callback in the list.
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
RpPrtAdvEmtMultiPropCloneCB(RpAtomic * atomic __RWUNUSED__,
                              RpPrtStdEmitter *emt,
                              void * data)
{
    RwInt32                             size, srcOffset, dstOffset;

    RpPrtStdEmitter                     *dstEmt;
    RpPrtAdvEmtPrtMultiColor        *emtPrtMultiCol = NULL, *dstEmtPrtMultiCol = NULL;
    RpPrtAdvEmtPrtMultiTexCoords    *emtPrtMultiTex = NULL, *dstEmtPrtMultiTex = NULL;
    RpPrtAdvEmtPrtMultiSize         *emtPrtMultiSize = NULL, *dstEmtPrtMultiSize = NULL;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtMultiPropCloneCB"));

    RWASSERT(atomic);
    RWASSERT(emt);
    RWASSERT(data);

    dstEmt = (RpPrtStdEmitter *) data;

    srcOffset =
        RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERPRTMULTICOLOR);

    dstOffset =
        RpPrtStdPropTabGetPropOffset(dstEmt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERPRTMULTICOLOR);

    if ((srcOffset >= 0) && (dstOffset >= 0))
    {
        emtPrtMultiCol = ((RpPrtAdvEmtPrtMultiColor *)
            RPPRTADVPROP(emt, srcOffset));

        dstEmtPrtMultiCol = ((RpPrtAdvEmtPrtMultiColor *)
            RPPRTADVPROP(dstEmt, dstOffset));

        size = sizeof(RpPrtAdvEmtPrtMultiColor) +
               emtPrtMultiCol->numCol * sizeof(RpPrtAdvEmtPrtColorItem);

        memcpy(dstEmtPrtMultiCol, emtPrtMultiCol, size);

        dstEmtPrtMultiCol->list = (RpPrtAdvEmtPrtColorItem *)
            (((RwChar *) dstEmtPrtMultiCol) + sizeof(RpPrtAdvEmtPrtMultiColor));
    }

    srcOffset =
        RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERPRTMULTITEXCOORDS);

    dstOffset =
        RpPrtStdPropTabGetPropOffset(dstEmt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERPRTMULTITEXCOORDS);

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
            (((RwChar *) dstEmtPrtMultiTex) + sizeof(RpPrtAdvEmtPrtMultiTexCoords));
    }

    srcOffset =
        RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERPRTMULTISIZE);

    dstOffset =
        RpPrtStdPropTabGetPropOffset(dstEmt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERPRTMULTISIZE);

    if ((srcOffset >= 0) && (dstOffset >= 0))
    {
        emtPrtMultiSize = ((RpPrtAdvEmtPrtMultiSize *)
            RPPRTADVPROP(emt, srcOffset));

        dstEmtPrtMultiSize = ((RpPrtAdvEmtPrtMultiSize *)
             RPPRTADVPROP(dstEmt, dstOffset));

        size = sizeof(RpPrtAdvEmtPrtMultiSize) +
               emtPrtMultiSize->numSize * sizeof(RpPrtAdvEmtPrtSizeItem);

        memcpy(dstEmtPrtMultiSize, emtPrtMultiSize, size);

        dstEmtPrtMultiSize->list = (RpPrtAdvEmtPrtSizeItem *)
            (((RwChar *) dstEmtPrtMultiSize) + sizeof(RpPrtAdvEmtPrtMultiSize));

    }

    RWRETURN(dstEmt);
}


/************************************************************************
 *
 *
 *
 ************************************************************************/

/**
 * \ingroup prtadvmultiprop
 * \ref RpPrtAdvEmtMultiPropStreamGetSizeCB is the callback used to
 * determine the size, in bytes, of the binary representation of a multiple-property
 * emitter. This value is used in the binary chunk header to indicate the size of the chunk.
 *
 * This callback should be added to a emitter class if the emitter contains
 * the multiple-property emitter property.
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
 * \see RpPrtAdvEmtMultiPropStreamReadCB
 * \see RpPrtAdvEmtMultiPropStreamWriteCB
 */
RpPrtStdEmitter *
RpPrtAdvEmtMultiPropStreamGetSizeCB(RpAtomic * atomic __RWUNUSED__,
                                       RpPrtStdEmitter *emt,
                                       void * data)
{
    RwInt32                          totalSize;
    RwInt32                          offset;

    RpPrtStdPropertyTable           *propTab;
    RpPrtStdEmitterClass            *eClass;

    RpPrtAdvEmtPrtMultiColor       *emtPrtMultiCol;
    RpPrtAdvEmtPrtMultiTexCoords   *emtPrtMultiTex;
    RpPrtAdvEmtPrtMultiSize        *emtPrtMultiSize;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtMultiPropStreamGetSizeCB"));

    RWASSERT(emt);
    RWASSERT(data);

    totalSize = 0;

    eClass = emt->emtClass;
    propTab = eClass->propTab;

    /* Get size of the multi color prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERPRTMULTICOLOR);

    if (offset >= 0)
    {
        emtPrtMultiCol = ((RpPrtAdvEmtPrtMultiColor *)
                  RPPRTADVPROP(emt, offset));

        /* Number of color variations */
        totalSize += sizeof(RwInt32);

        /* Item array */
        totalSize +=
                sizeof(RpPrtAdvEmtPrtColorItem)*emtPrtMultiCol->numCol;
    };

    /* Get size of the multi tex prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERPRTMULTITEXCOORDS);

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

    /* Get size of the multi size prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERPRTMULTISIZE);

    if (offset >= 0)
    {
        emtPrtMultiSize = ((RpPrtAdvEmtPrtMultiSize *)
                  RPPRTADVPROP(emt, offset));

        /* Number of uv variations */
        totalSize += sizeof(RwInt32);

        /* Item array */
        totalSize += sizeof(RpPrtAdvEmtPrtSizeItem)*
                            emtPrtMultiSize->numSize;
    };

    *(RwInt32 *) data = totalSize;

    RWRETURN(emt);
}


/**
 * \ingroup prtadvmultiprop
 * \ref RpPrtAdvEmtMultiPropStreamReadCB is the stream in callback for
 * the multiple-property property.
 *
 * This function streams in an emitter containing the multiple-property property.
 *
 * This callback should be added to a emitter class if the emitter contains
 * the multiple-property emitter property.
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
 * \see RpPrtAdvEmtMultiPropStreamGetSizeCB
 * \see RpPrtAdvEmtMultiPropStreamWriteCB
 */
RpPrtStdEmitter *
RpPrtAdvEmtMultiPropStreamReadCB(RpAtomic * atomic __RWUNUSED__,
                                    RpPrtStdEmitter *emt, void *data)
{
    RwInt32                         offset;
    RwStream                        *stream;

    RpPrtStdPropertyTable           *propTab;
    RpPrtStdEmitterClass            *eClass;

    RpPrtAdvEmtPrtMultiColor        *emtPrtMultiCol;
    RpPrtAdvEmtPrtMultiTexCoords    *emtPrtMultiTex;
    RpPrtAdvEmtPrtMultiSize         *emtPrtMultiSize;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtMultiPropStreamReadCB"));

    RWASSERT(emt);
    RWASSERT(data);

    stream = (RwStream *) data;

    eClass = emt->emtClass;
    propTab = eClass->propTab;

    /* Get size of the multi color prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERPRTMULTICOLOR);

    if (offset >= 0)
    {
        emtPrtMultiCol = ((RpPrtAdvEmtPrtMultiColor *)
                  RPPRTADVPROP(emt, offset));

        /* Number of variations */
        RwStreamReadInt32(stream,
                        (RwInt32*)&emtPrtMultiCol->numCol,
                        sizeof(RwInt32));

        if (emtPrtMultiCol->numCol > 0)
        {
            RwStreamReadReal(stream,
                                (RwReal*)emtPrtMultiCol->list,
                                sizeof(RpPrtAdvEmtPrtColorItem)*
                                emtPrtMultiCol->numCol);

            /* The data structures were changed in size, so we
             * need to re-adjust some offsets values.
             */
            if (_rpPrtStdStreamGetVersion() < rpPRTSTDVERSIONSTAMP)
            {
                emtPrtMultiCol->offsetTime =
                    sizeof(RpPrtAdvPrtMultiColor);
                emtPrtMultiCol->offsetStartCol =
                    emtPrtMultiCol->offsetTime +
                    (emtPrtMultiCol->numCol * sizeof(RwReal));
                emtPrtMultiCol->offsetDeltaCol =
                    emtPrtMultiCol->offsetStartCol +
                    (emtPrtMultiCol->numCol * sizeof(RwRGBAReal));
            }
        }
    }

    /* Get size of the multi tex prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERPRTMULTITEXCOORDS);

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
                /* Offsets into the particle's multitexcoords props.
                 * The offsets are from the start of the property, not from
                 * the particle
                 */
                emtPrtMultiTex->offsetTime =
                    sizeof(RpPrtAdvPrtMultiTexCoords);
                emtPrtMultiTex->offsetStartUV0 =
                    emtPrtMultiTex->offsetTime +
                    (emtPrtMultiTex->numTex * sizeof(RwReal));
                emtPrtMultiTex->offsetDeltaUV0 =
                    emtPrtMultiTex->offsetStartUV0 +
                    (emtPrtMultiTex->numTex * sizeof(RwTexCoords));
                emtPrtMultiTex->offsetStartUV1 =
                    emtPrtMultiTex->offsetDeltaUV0 +
                    (emtPrtMultiTex->numTex * sizeof(RwTexCoords));
                emtPrtMultiTex->offsetDeltaUV1 =
                    emtPrtMultiTex->offsetStartUV1 +
                    (emtPrtMultiTex->numTex * sizeof(RwTexCoords));
            }
        }
    }

    /* Get size of the multi size prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERPRTMULTISIZE);

    if (offset >= 0)
    {
        emtPrtMultiSize = ((RpPrtAdvEmtPrtMultiSize *)
                  RPPRTADVPROP(emt, offset));

        /* Number of variations */
        RwStreamReadInt32(stream,
                            (RwInt32*)&emtPrtMultiSize->numSize,
                            sizeof(RwInt32));

        if (emtPrtMultiSize->numSize > 0)
        {
            RwStreamReadReal(stream,
                                (RwReal*)emtPrtMultiSize->list,
                                sizeof(RpPrtAdvEmtPrtSizeItem)*
                                emtPrtMultiSize->numSize);

            /* The data structures were changed in size, so we
             * need to re-adjust some offsets values.
             */
            if (_rpPrtStdStreamGetVersion() < rpPRTSTDVERSIONSTAMP)
            {
                /* Offsets into the particle's multicolor props. The offsets
                 * are from the start of the property, not from the particle
                 */
                emtPrtMultiSize->offsetTime =
                    sizeof(RpPrtAdvPrtMultiSize);
                emtPrtMultiSize->offsetStartSize =
                    emtPrtMultiSize->offsetTime +
                    (emtPrtMultiSize->numSize * sizeof(RwReal));
                emtPrtMultiSize->offsetDeltaSize =
                    emtPrtMultiSize->offsetStartSize +
                    (emtPrtMultiSize->numSize * sizeof(RwV2d));
            }
        }
    }

    RWRETURN(emt);
}

/**
 * \ingroup prtadvmultiprop
 * \ref RpPrtAdvEmtMultiPropStreamWriteCB is the stream out callback for
 * the multiple-property particle property.
 *
 * This function streams out an emitter containing multiple-property properties.
 *
 * This callback should be added to a emitter class if the emitter contains
 * the multiple-property emitter property.
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
 * \see RpPrtAdvEmtMultiPropStreamGetSizeCB
 * \see RpPrtAdvEmtMultiPropStreamReadCB
 */
RpPrtStdEmitter *
RpPrtAdvEmtMultiPropStreamWriteCB(RpAtomic * atomic __RWUNUSED__,
                                     RpPrtStdEmitter *emt, void *data)
{
    RwInt32                         offset;
    RwStream                        *stream;

    RpPrtStdPropertyTable           *propTab;
    RpPrtStdEmitterClass            *eClass;

    RpPrtAdvEmtPrtMultiColor       *emtPrtMultiCol;
    RpPrtAdvEmtPrtMultiTexCoords   *emtPrtMultiTex;
    RpPrtAdvEmtPrtMultiSize        *emtPrtMultiSize;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtMultiPropStreamWriteCB"));

    RWASSERT(emt);
    RWASSERT(data);

    stream = (RwStream *) data;

    eClass = emt->emtClass;
    propTab = eClass->propTab;

    /* Get size of the multi color prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERPRTMULTICOLOR);

    if (offset >= 0)
    {
        emtPrtMultiCol = ((RpPrtAdvEmtPrtMultiColor *)
                  RPPRTADVPROP(emt, offset));

        /* Number of variations */
        RwStreamWriteInt32(stream,
                            (RwInt32*)&emtPrtMultiCol->numCol,
                            sizeof(RwInt32));

        if (emtPrtMultiCol->numCol > 0)
        {
            RwStreamWriteReal(stream,
                                (RwReal*)emtPrtMultiCol->list,
                                sizeof(RpPrtAdvEmtPrtColorItem)*
                                emtPrtMultiCol->numCol);
        }
    }

    /* Get size of the multi tex prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERPRTMULTITEXCOORDS);

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

    /* Get size of the multi size prop. */
    offset = RpPrtStdPropTabGetPropOffset(propTab,
        rpPRTADVPROPERTYCODEEMITTERPRTMULTISIZE);

    if (offset >= 0)
    {
        emtPrtMultiSize = ((RpPrtAdvEmtPrtMultiSize *)
                  RPPRTADVPROP(emt, offset));

        /* Number of variations */
        RwStreamWriteInt32(stream,
                            (RwInt32*)&emtPrtMultiSize->numSize,
                            sizeof(RwInt32));

        if (emtPrtMultiSize->numSize > 0)
        {
            RwStreamWriteReal(stream,
                                (RwReal*)emtPrtMultiSize->list,
                                sizeof(RpPrtAdvEmtPrtSizeItem)*
                                emtPrtMultiSize->numSize);
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
 * \ingroup prtadvmultiprop
 * \ref RpPrtAdvEmtMultiPropCreateCB is the create callback for the
 * multiple-property emitter property.
 *
 * This callback setups the multiple-property emitter properties of an emitter.
 *
 * This callback should be added to an emitter class if the emitter contains
 * the multiple-property emitter property.
 *
 * \param atomic        Pointer to the parent RpAtomic.
 * \param emt           Pointer to the RpPrtStdEmitter.
 * \param data          Void pointer to private data for the emit callback.
 *
 * \return Pointer to the RpPrtStdEmitter if successful, NULL otherwise.
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
RpPrtAdvEmtMultiPropCreateCB(RpAtomic * atomic __RWUNUSED__,
                                RpPrtStdEmitter *emt,
                                void * data __RWUNUSED__)
{
    RwInt32 offset,idx,size;

    RpPrtAdvEmtPrtMultiColor            * emtPrtMultiCol;
    RpPrtAdvEmtPrtMultiTexCoords        * emtPrtMultiTex;
    RpPrtAdvEmtPrtMultiSize             * emtPrtMultiSize;

    RwInt32 *propSize;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEmtMultiPropCreateCB"));

    offset = RpPrtStdPropTabGetPropOffset(
        emt->emtClass->propTab, rpPRTADVPROPERTYCODEPARTICLEMULTICOLOR);

    if( offset > 0 )
    {
        emtPrtMultiCol = (RpPrtAdvEmtPrtMultiColor *)
            (((RwChar *)emt) + offset);


        idx = RpPrtStdPropTabGetPropIndex(
            emt->emtClass->propTab, rpPRTADVPROPERTYCODEPARTICLEMULTICOLOR);

        RpPrtStdPropTabGetProperties(emt->emtClass->propTab,
                                            NULL,
                                            NULL,
                                            NULL,
                                            &propSize);

        size = propSize[idx];

        size -= sizeof(RpPrtAdvEmtPrtMultiColor);
        emtPrtMultiCol->numCol = size / sizeof(RpPrtAdvEmtPrtColorItem);

        emtPrtMultiCol->list = (RpPrtAdvEmtPrtColorItem*)(
                                (RwUInt32)(emtPrtMultiCol) +
                                sizeof(RpPrtAdvEmtPrtMultiColor));

        /* Offsets into the particle's multicolor props. The offsets are from the
         * start of the property, not from the particle
         */
        emtPrtMultiCol->offsetTime = sizeof(RpPrtAdvPrtMultiColor);
        emtPrtMultiCol->offsetStartCol = emtPrtMultiCol->offsetTime +
                (emtPrtMultiCol->numCol * sizeof(RwReal));
        emtPrtMultiCol->offsetDeltaCol = emtPrtMultiCol->offsetStartCol +
               (emtPrtMultiCol->numCol * sizeof(RwRGBAReal));
    }

    offset = RpPrtStdPropTabGetPropOffset(
    emt->emtClass->propTab, rpPRTADVPROPERTYCODEPARTICLEMULTITEXCOORDS);

    if( offset > 0 )
    {
        emtPrtMultiTex = (RpPrtAdvEmtPrtMultiTexCoords *)
            (((RwChar *)emt) + offset);


        idx = RpPrtStdPropTabGetPropIndex(
        emt->emtClass->propTab, rpPRTADVPROPERTYCODEPARTICLEMULTITEXCOORDS);

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
        emtPrtMultiTex->offsetTime = sizeof(RpPrtAdvPrtMultiTexCoords);
        emtPrtMultiTex->offsetStartUV0 = emtPrtMultiTex->offsetTime +
                (emtPrtMultiTex->numTex * sizeof(RwReal));
        emtPrtMultiTex->offsetDeltaUV0 = emtPrtMultiTex->offsetStartUV0 +
                (emtPrtMultiTex->numTex * sizeof(RwTexCoords));
        emtPrtMultiTex->offsetStartUV1 = emtPrtMultiTex->offsetDeltaUV0 +
                (emtPrtMultiTex->numTex * sizeof(RwTexCoords));
        emtPrtMultiTex->offsetDeltaUV1 = emtPrtMultiTex->offsetStartUV1 +
                (emtPrtMultiTex->numTex * sizeof(RwTexCoords));

    }

    offset = RpPrtStdPropTabGetPropOffset(
    emt->emtClass->propTab, rpPRTADVPROPERTYCODEPARTICLEMULTISIZE);

    if( offset > 0 )
    {
        emtPrtMultiSize = (RpPrtAdvEmtPrtMultiSize *)
            (((RwChar *)emt) + offset);


        idx = RpPrtStdPropTabGetPropIndex(
        emt->emtClass->propTab, rpPRTADVPROPERTYCODEPARTICLEMULTISIZE);

        RpPrtStdPropTabGetProperties(emt->emtClass->propTab,
                                            NULL,
                                            NULL,
                                            NULL,
                                            &propSize);

        size = propSize[idx];

        size -= sizeof(RpPrtAdvEmtPrtMultiSize);
        emtPrtMultiSize->numSize = size / sizeof(RpPrtAdvEmtPrtSizeItem);

        emtPrtMultiSize->list = (RpPrtAdvEmtPrtSizeItem*)(
                                (RwUInt32)(emtPrtMultiSize) +
                                sizeof(RpPrtAdvEmtPrtMultiSize));

        /* Offsets into the particle's multicolor props. The offsets are from the
         * start of the property, not from the particle
         */
        emtPrtMultiSize->offsetTime = sizeof(RpPrtAdvPrtMultiSize);
        emtPrtMultiSize->offsetStartSize = emtPrtMultiSize->offsetTime +
                (emtPrtMultiSize->numSize * sizeof(RwReal));
        emtPrtMultiSize->offsetDeltaSize = emtPrtMultiSize->offsetStartSize +
               (emtPrtMultiSize->numSize * sizeof(RwV2d));
    }

    RWRETURN(emt);
}

