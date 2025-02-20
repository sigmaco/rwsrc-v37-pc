/**********************************************************************
 *
 * File :     ptank.c
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

#include <rpplugin.h>
#include <rpdbgerr.h>

#include "rpprtstd.h"

#include "prtstdvars.h"

#include "pptank.h"

/************************************************************************
 *
 *
 ************************************************************************/

RpPrtStdEmitterPTank *
_rpPrtStdPTankCtor(RpPrtStdEmitterPTank *emtPTank)
{
    RWFUNCTION(RWSTRING("_rpPrtStdPTankCtor"));

    RWASSERT(emtPTank);

    emtPTank->dataFlags = 0;
    emtPTank->platFlags = 0;
    emtPTank->updateFlags = ~0x0;
    emtPTank->emitFlags = ~0x0;
    emtPTank->maxPrt = 0;

    emtPTank->pTank = (RpAtomic *)NULL;
    emtPTank->dataInPtrs = (RwChar **)NULL;
    emtPTank->dataOutPtrs = (RwChar **)NULL;

    emtPTank->strSrcBlend = (RwInt32)rwBLENDONE;
    emtPTank->strDstBlend = (RwInt32)rwBLENDONE;
    emtPTank->strVtxABlend = TRUE;

    RWRETURN(emtPTank);
}

RpPrtStdEmitterPTank *
_rpPrtStdPTankDtor(RpPrtStdEmitterPTank *emtPTank)
{
    RwFrame             *frame;

    RWFUNCTION(RWSTRING("_rpPrtStdPTankDtor"));

    RWASSERT(emtPTank);

    if (emtPTank->pTank != NULL)
    {
        frame = RpAtomicGetFrame(emtPTank->pTank);

        RpAtomicSetFrame(emtPTank->pTank, NULL);

        if (frame != NULL)
            RwFrameDestroy(frame);

        RpAtomicDestroy(emtPTank->pTank);
    }

    emtPTank->pTank = NULL;

    if (emtPTank->dataInPtrs != NULL)
        RwFree(emtPTank->dataInPtrs);

    emtPTank->dataInPtrs = NULL;
    emtPTank->dataOutPtrs = NULL;

    emtPTank->maxPrt = 0;

    RWRETURN(emtPTank);
}

RpPrtStdEmitterPTank *
_rpPrtStdPTankClone(RpPrtStdEmitterPTank *dstEmtPTank, RpPrtStdEmitterPTank *srcEmtPTank)
{
    RWFUNCTION(RWSTRING("_rpPrtStdPTankClone"));

    RWASSERT(srcEmtPTank);
    RWASSERT(dstEmtPTank);

    dstEmtPTank->dataFlags = srcEmtPTank->dataFlags;
    dstEmtPTank->platFlags = srcEmtPTank->platFlags;
    dstEmtPTank->maxPrt = srcEmtPTank->maxPrt;
    dstEmtPTank->updateFlags = srcEmtPTank->updateFlags;
    dstEmtPTank->emitFlags = srcEmtPTank->emitFlags;
    dstEmtPTank->strSrcBlend = srcEmtPTank->strSrcBlend;
    dstEmtPTank->strDstBlend = srcEmtPTank->strDstBlend;
    dstEmtPTank->strVtxABlend = srcEmtPTank->strVtxABlend;

    RWRETURN(dstEmtPTank);
}

/************************************************************************/

RpPrtStdEmitterPTank *
_rpPrtStdPTankGetDataPtr(RpPrtStdEmitterPTank *emtPTank)
{

    const RpPTankFormatDescriptor   *pTankFormat;
    RpPTankLockStruct               pTankLock;
    RwInt32                         i, j;

    RWFUNCTION(RWSTRING("_rpPrtStdPTankGetDataPtr"));

    RWASSERT(emtPTank);
    RWASSERT(emtPTank->pTank);

    pTankFormat = RpPTankAtomicGetDataFormat(emtPTank->pTank);
    i = 0;
    j = 1;
    emtPTank->dataFlags = 0;

    while (i < pTankFormat->numClusters)
    {
        /* Check if this property exist by attempting to lock it. */
        if (RpPTankAtomicLock(emtPTank->pTank,
                &pTankLock, j, rpPTANKLOCKWRITE) != FALSE)
        {

#if (defined(SKY2_DRVMODEL_H) && defined(SKY_USE_MULTIBUFERING))



            emtPTank->dataInPtrs[i] = (RwChar *)
                RpPTankAtomicSkyGetPreviousBuffer(emtPTank->pTank, j);

#else /* (SKY2_DRVMODEL_H) */

            emtPTank->dataInPtrs[i] = (RwChar *) pTankLock.data;

#endif /* (SKY2_DRVMODEL_H) */

            emtPTank->dataOutPtrs[i] = (RwChar *) pTankLock.data;
            emtPTank->dataStride[i] = pTankLock.stride;
            i++;

            emtPTank->dataFlags |= j;
        }
        j <<= 1;
    }

    RWRETURN(emtPTank);
}

/************************************************************************/

RpPrtStdEmitterPTank *
_rpPrtStdPTankCreateDataPtr(RpPrtStdEmitterPTank *emtPTank)
{
    const RpPTankFormatDescriptor       *pTankFormat;

    RWFUNCTION(RWSTRING("_rpPrtStdPTankCreateDataPtr"));

    RWASSERT(emtPTank);

    /* Destroy the old data ptrs */
    if (emtPTank->dataInPtrs != NULL)
        RwFree(emtPTank->dataInPtrs);

    emtPTank->dataInPtrs = NULL;
    emtPTank->dataOutPtrs = NULL;
    emtPTank->dataStride = NULL;

    if (emtPTank->pTank)
    {
        /* Set up data ptrs for updating. */
        pTankFormat = RpPTankAtomicGetDataFormat(emtPTank->pTank);
        emtPTank->dataInPtrs = (RwChar **)
            RwMalloc(pTankFormat->numClusters * 3 * sizeof(void *),
                     rwID_PRTSTDPLUGIN | rwMEMHINTDUR_EVENT);

        if (emtPTank->dataInPtrs == (RwChar **)NULL)
            RWRETURN((RpPrtStdEmitterPTank *) NULL);

        emtPTank->dataOutPtrs =
            &emtPTank->dataInPtrs[pTankFormat->numClusters];
        emtPTank->dataStride = (RwInt32 *)
            &emtPTank->dataOutPtrs[pTankFormat->numClusters];
    }

    RWRETURN(emtPTank);
}

/************************************************************************/

RpPrtStdEmitterPTank *
_rpPrtStdPTankStreamRead(RwStream *stream, RpPrtStdEmitterPTank *emtPTank)
{
    RWFUNCTION(RWSTRING("_rpPrtStdPTankStreamRead"));

    RWASSERT(stream);
    RWASSERT(emtPTank);

    /* pTank update flag */
    RwStreamRead(stream, (RwInt32 *) &emtPTank->updateFlags, sizeof(RwInt32));

    /* pTank emit flag */
    RwStreamRead(stream, (RwInt32 *) &emtPTank->emitFlags, sizeof(RwInt32));

    /* pTank blending modes */
    RwStreamReadInt32(stream,(RwInt32 *) &emtPTank->strSrcBlend,
                        sizeof(RwInt32));
    RwStreamReadInt32(stream,(RwInt32 *) &emtPTank->strDstBlend,
                        sizeof(RwInt32));
    RwStreamReadInt32(stream,(RwInt32 *) &emtPTank->strVtxABlend,
                        sizeof(RwInt32));

    RWRETURN(emtPTank);
}

RpPrtStdEmitterPTank *
_rpPrtStdPTankStreamWrite(RwStream *stream, RpPrtStdEmitterPTank *emtPTank)
{
    RWFUNCTION(RWSTRING("_rpPrtStdPTankStreamWrite"));

    RWASSERT(stream);
    RWASSERT(emtPTank);

    /* pTank update flag */
    RwStreamWrite(stream, (RwInt32 *) &emtPTank->updateFlags, sizeof(RwInt32));

    /* pTank emit flag */
    RwStreamWrite(stream, (RwInt32 *) &emtPTank->emitFlags, sizeof(RwInt32));

    /* pTank blending modes */
    RwStreamWriteInt32(stream,(RwInt32 *) &emtPTank->strSrcBlend,
                        sizeof(RwInt32));
    RwStreamWriteInt32(stream,(RwInt32 *) &emtPTank->strDstBlend,
                        sizeof(RwInt32));
    RwStreamWriteInt32(stream,(RwInt32 *) &emtPTank->strVtxABlend,
                        sizeof(RwInt32));


    RWRETURN(emtPTank);
}

RwInt32
_rpPrtStdPTankStreamGetSize(RpPrtStdEmitterPTank *emtPTank __RWUNUSEDRELEASE__)
{
    RwInt32             size;

    RWFUNCTION(RWSTRING("_rpPrtStdPTankStreamGetSize"));

    RWASSERT(emtPTank);

    /* pTank update & emit flag */
    size = 2 * sizeof(RwUInt32);

    /* pTank blending mode */
    size += 3*sizeof(RwUInt32);

    RWRETURN(size);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterCreatePTank allows the user to force the creation of
 * the PTank. In normal operation, the PTank is only created when needed,
 * this occurs in the first update of the emitter.
 *
 * RpPrtStdEmitterCreatePTank allows the PTank to be created at the user's
 * request before this. Allowing additional setup on the PTank, such as
 * changing the render states, to be made.
 *
 * If the PTank already exist, then the function does nothing.
 *
 * \param emt           Pointer to the parent emitter.
 *
 * \return Pointer to the particle emitter if sucessful, NULL otherwise.
 *
 * \see RpPrtStdAtomicUpdate
 * \see RpPTankAtomicCreate
 * \see RpPrtStdEmitterLinkPTank
 */
RpPrtStdEmitter *
RpPrtStdEmitterCreatePTank(RpPrtStdEmitter *emt)
{
    RwV2d                           v2d;
    RwTexCoords                     uv[4];
    RwInt32                         offset, dataFlags;
    RwFrame                         *frame;
    RpPrtStdEmitterStandard         *emtStd;
    RpPrtStdEmitterPTank            *emtPTank;
    RwUInt32                        tmpPlatFlags;
    RpAtomic                        *pTank;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterCreatePTank"));

    RWASSERT(emt);

    /* Set up the pTank if required. */
    offset = RpPrtStdPropTabGetPropOffset(
                 emt->emtClass->propTab,
                 rpPRTSTDPROPERTYCODEEMITTERPTANK);

    RWASSERT(offset >= 0);

    emtPTank = (RpPrtStdEmitterPTank *)
                    RPPRTSTDPROP(emt, offset);

    /* We should not create a second pTank. */
    if (emtPTank->pTank != NULL)
    {
        if (emtPTank->dataInPtrs != NULL)
            _rpPrtStdPTankCreateDataPtr(emtPTank);

        RWRETURN(emt);
    }

    /* Get the standard emitter */
    emtStd = NULL;
    if ((offset = RpPrtStdPropTabGetPropOffset(
                 emt->emtClass->propTab,
                 rpPRTSTDPROPERTYCODEEMITTERSTANDARD)) >= 0)
        emtStd = (RpPrtStdEmitterStandard *)
                        RPPRTSTDPROP(emt, offset);

    /*
     * Generate the dataflags if not set depending on the properties
     * in the emitter.
     *
     * If the data flag is not 0, we assume this is setup already.
     */
    dataFlags = 0;

    /* Position or matrix ? */
    if ((offset = RpPrtStdPropTabGetPropOffset(
                emt->emtClass->propTab,
                rpPRTSTDPROPERTYCODEEMITTERPRTMATRIX)) >= 0)
    {
        dataFlags |= rpPTANKDFLAGMATRIX;
    }
    else
    {
        dataFlags |= rpPTANKDFLAGPOSITION;
    }

    /* Constant color ? */
    if ((offset = RpPrtStdPropTabGetPropOffset(
                emt->emtClass->propTab,
                rpPRTSTDPROPERTYCODEEMITTERPRTCOLOR)) >= 0)
    {
        dataFlags |= rpPTANKDFLAGCOLOR;
    }

    /* Constant tex coords ? */
    if (emtStd->texture)
    {
        if ((offset = RpPrtStdPropTabGetPropOffset(
                    emt->emtClass->propTab,
                    rpPRTSTDPROPERTYCODEEMITTERPRTTEXCOORDS)) >= 0)
        {
            dataFlags |= rpPTANKDFLAGVTX2TEXCOORDS;
        }
        else
        {
            dataFlags |= rpPTANKDFLAGCNSVTX2TEXCOORDS;
        }
    }

    /* Constant size ? */
    if ((offset = RpPrtStdPropTabGetPropOffset(
                emt->emtClass->propTab,
                rpPRTSTDPROPERTYCODEEMITTERPRTSIZE)) >= 0)
    {
        /* Only have the size field if not using matrix particles */
        if (dataFlags & rpPTANKDFLAGPOSITION)
            dataFlags |= rpPTANKDFLAGSIZE;
    }

    /* Turn off constant rotate update */
    emtPTank->updateFlags &= ~rpPTANKDFLAGCNS2DROTATE;

    /* Only have the 2D rotate field if not using matrix particles */
    if (dataFlags & rpPTANKDFLAGPOSITION)
    {
        /* Per particle ? */
        if ((offset = RpPrtStdPropTabGetPropOffset(
             emt->emtClass->propTab,
             rpPRTSTDPROPERTYCODEEMITTERPRT2DROTATE)) >= 0)
        {
            dataFlags |= rpPTANKDFLAG2DROTATE;
        }
        else
        {
            /* Global rotation ? */
            if ((emtStd->prtDelta2DRotate > (RwReal)  ZERO_TOL) ||
                (emtStd->prtDelta2DRotate < (RwReal) -ZERO_TOL))
            {
                dataFlags |= rpPTANKDFLAGCNS2DROTATE;
                emtPTank->updateFlags |= rpPTANKDFLAGCNS2DROTATE;
            }
        }
    }


#ifdef SKY

    dataFlags |= rpPTANKDFLAGARRAY;

#endif /* SKY */


    pTank = RpPTankAtomicCreate(emtStd->maxPrt,
        dataFlags,
#ifdef SKY
/*        rpPTANKSKYFLAGINSTANCEBUFFER |*/
/*          rpPTANKSKYFLAGUSEGENERIC |*/
#endif

#if (defined(SKY2_DRVMODEL_H) && defined(SKY_USE_STOP_FLAG))
            rpPTANKSKYFLAGUSESTOPFLAG |
#endif

#if (defined(SKY2_DRVMODEL_H) && defined(SKY_USE_MULTIBUFERING))
            rpPTANKSKYFLAGINSTANCEBUFFER |
#endif
        emtPTank->platFlags);

    if (pTank == NULL)
        RWRETURN((RpPrtStdEmitter *) NULL);

    /* Set up a dummy frame */
    frame = RwFrameCreate();
    RwMatrixSetIdentity(RwFrameGetMatrix(frame));
    RpAtomicSetFrame(pTank, frame);

    /* Set up the constant props in the PTank if needed */
    v2d.x = (RwReal) -0.0;
    v2d.y = (RwReal) -0.0;
    RpPTankAtomicSetConstantCenter(pTank, &v2d);

    /* Particle color */
    if (!(dataFlags & rpPTANKDFLAGCOLOR))
    {
        RpPTankAtomicSetConstantColor(pTank, &emtStd->prtColor);
    }

    /* Particle tex coords */
    if (dataFlags & rpPTANKDFLAGCNSVTX2TEXCOORDS)
    {
        uv[0].u = emtStd->prtUV[0].u;
        uv[0].v = emtStd->prtUV[0].v;

        uv[1].u = emtStd->prtUV[1].u;
        uv[1].v = emtStd->prtUV[1].v;

        RpPTankAtomicSetConstantVtx2TexCoords(pTank, uv);
    }

    /* Particle tex coords */
    if (dataFlags & rpPTANKDFLAGCNSVTX4TEXCOORDS)
    {
        /* Can we set anything ? */
        /* RpPTankAtomicSetConstantVtx2TexCoords(emtPTank->pTank, ???); */
    }

    /* Particle size */
    if (!(dataFlags & rpPTANKDFLAGSIZE))
    {
        v2d.x = emtStd->prtSize.x;
        v2d.y = emtStd->prtSize.y;

        RpPTankAtomicSetConstantSize(pTank, &v2d);
    }

    /* Particle texture */
    if (emtStd->texture)
        RpPTankAtomicSetTexture(pTank, emtStd->texture);

    
    /*
     * Store the platforme flag as it will get reset to 0 by 
     * RpPrtStdEmitterLinkPTank, as the link function got no knowledge 
     * (and doesn't need it).
     */
    tmpPlatFlags = emtPTank->platFlags;
    
    if ( NULL == RpPrtStdEmitterLinkPTank(emt,pTank))
        RWRETURN((RpPrtStdEmitter *) NULL);

    /*
     * Reset the platforme flag so that streaming of it can be done.
     */
    emtPTank->platFlags = tmpPlatFlags;

    /* Setting the blending mode if needed */
    RpPTankAtomicSetBlendModes(emtPTank->pTank,
                                emtPTank->strSrcBlend,
                                emtPTank->strDstBlend);

    RpPTankAtomicSetVertexAlpha(emtPTank->pTank, emtPTank->strVtxABlend);

    RWRETURN(emt);
}

/************************************************************************/

RpPrtStdEmitter *
_rpPrtStdEmitterDestroyPTank(RpPrtStdEmitter *emt)
{
    RwInt32                         offset;
    RpPrtStdEmitterPTank            *emtPTank;

    RWFUNCTION(RWSTRING("_rpPrtStdEmitterDestroyPTank"));

    RWASSERT(emt);

    /* Set up the pTank if required. */
    offset = RpPrtStdPropTabGetPropOffset(
                 emt->emtClass->propTab,
                 rpPRTSTDPROPERTYCODEEMITTERPTANK);

    RWASSERT(offset >= 0);

    emtPTank = (RpPrtStdEmitterPTank *)
                    RPPRTSTDPROP(emt, offset);

    _rpPrtStdPTankDtor(emtPTank);

    RWRETURN(emt);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterLinkPTank allow one to link a custom PTank to an emitter.
 * This function should be called before any call to \ref RpPrtStdAtomicUpdate
 * and should never be called after.
 * Once linked, the emitter got ownership of the ptank and will destroy it
 * as if it had created it.
 * The PTank Atomic need to be linked to a \ref RwFrame
 *
 * \param emt           Pointer to the parent emitter.
 * \param ptank         Pointer to the ptank object to link.
 *
 * \return pointer to the particle emitter if sucessful, NULL otherwise.
 *
 * \see RpPrtStdAtomicUpdate
 * \see RpPTankAtomicCreate
 * \see RpPrtStdEmitterCreatePTank
 */
RpPrtStdEmitter *
RpPrtStdEmitterLinkPTank(RpPrtStdEmitter *emt,RpAtomic *ptank)
{
    RwInt32                         offset;
    RpPrtStdEmitterStandard         *emtStd;
    RpPrtStdEmitterPTank            *emtPTank;
    const RpPTankFormatDescriptor         *ptankFormat;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterLinkPTank"));

    RWASSERT(emt);
    RWASSERT(ptank);

    /* Set up the pTank if required. */
    offset = RpPrtStdPropTabGetPropOffset(
                 emt->emtClass->propTab,
                 rpPRTSTDPROPERTYCODEEMITTERPTANK);

    RWASSERT(offset >= 0);

    emtPTank = (RpPrtStdEmitterPTank *)
                    RPPRTSTDPROP(emt, offset);

    /* We should not create a second pTank. */
    if (emtPTank->pTank != NULL)
    {
        RWRETURN((RpPrtStdEmitter *)NULL);
    }

    /* Get the standard emitter */
    emtStd = NULL;
    if ((offset = RpPrtStdPropTabGetPropOffset(
                 emt->emtClass->propTab,
                 rpPRTSTDPROPERTYCODEEMITTERSTANDARD)) >= 0)
        emtStd = (RpPrtStdEmitterStandard *)
                        RPPRTSTDPROP(emt, offset);

    /*
     * Generate the dataflags if not set depending on the properties
     * in the emitter.
     *
     * If the data flag is not 0, we assume this is setup already.
     */
    ptankFormat = RpPTankAtomicGetDataFormat(ptank);

    emtPTank->maxPrt = RpPTankAtomicGetMaximumParticlesCount(ptank);

    emtPTank->pTank = ptank;

    emtPTank->dataFlags = ptankFormat->dataFlags;
    emtPTank->platFlags = 0;
    emtPTank->numPrt = 0;

    RpPTankAtomicSetActiveParticlesCount(ptank,0);

    if (_rpPrtStdPTankCreateDataPtr(emtPTank) != emtPTank)
        RWRETURN((RpPrtStdEmitter *) NULL);

    RWRETURN(emt);
}

