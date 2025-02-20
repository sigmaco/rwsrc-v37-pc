/**********************************************************************
 *
 * File :     emitter.c
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

#include <rwcore.h>
#include <rpworld.h>
#include <rpptank.h>

#include <rpplugin.h>
#include <rpdbgerr.h>

#include "rpprtstd.h"
#include "prop.h"
#include "pclass.h"
#include "eclass.h"

#include "emitter.h"
#include "pptank.h"

#include "prtstdvars.h"

/* #define SKY2_EMITUPDATECOUNTx */



#ifdef SKY2_EMITUPDATECOUNT

#include "libpc.h"
#include "libdma.h"

extern              sky_counter[];
extern              sky_c;

#endif /* SKY2_EMITUPDATECOUNT */

/************************************************************************
 *
 *
 ************************************************************************/

RpPrtStdParticleBatch *
_rpPrtStdParticleBatchCtor(RpPrtStdParticleBatch * prtBatch)
{

    RwChar             *ptr;
    RwInt32             i;
    RpPrtStdParticleClass *pClass;
    RpPrtStdParticleCallBack callback;

    RWFUNCTION(RWSTRING("_rpPrtStdParticleBatchCtor"));

    RWASSERT(prtBatch);
    RWASSERT(prtBatch->prtClass);

    prtBatch->numPrt = 0;
    prtBatch->newPrt = 0;
    prtBatch->next = prtBatch;

    ptr = ((RwChar *) prtBatch) + sizeof(RpPrtStdParticleBatch);

    /* Call the callbacks. */
    pClass = prtBatch->prtClass;
    for (i = 0; i < pClass->numCallback; i++)
    {
        if ((callback = (pClass->callback[i][rpPRTSTDPARTICLECALLBACKCREATE])) != NULL)
        {
            if ((callback) (NULL, prtBatch, NULL) != prtBatch)
                RWRETURN((RpPrtStdParticleBatch *) NULL);
        }
    }


    RWRETURN(prtBatch);
}

RpPrtStdParticleBatch *
_rpPrtStdParticleBatchDtor(RpPrtStdParticleBatch * prtBatch)
{
    RwChar             *ptr;
    RwInt32             i;
    RpPrtStdParticleClass *pClass;
    RpPrtStdParticleCallBack callback;

    RWFUNCTION(RWSTRING("_rpPrtStdParticleBatchDtor"));

    RWASSERT(prtBatch);
    RWASSERT(prtBatch->prtClass);

    /* Call the callbacks. */
    pClass = prtBatch->prtClass;
    for (i = 0; i < pClass->numCallback; i++)
    {
        if ((callback = (pClass->callback[i][rpPRTSTDPARTICLECALLBACKDESTROY])) != NULL)
        {
            if ((callback) (NULL, prtBatch, NULL) != prtBatch)
                RWRETURN((RpPrtStdParticleBatch *) NULL);
        }
    }

    ptr = ((RwChar *) prtBatch) + sizeof(RpPrtStdParticleBatch);

    prtBatch->numPrt = 0;
    prtBatch->newPrt = 0;
    prtBatch->next = NULL;

    RWRETURN(prtBatch);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdParticleBatchCreate is the function to create a new particle
 * batch.
 *
 * \param pClass        A pointer to the particle class.
 * \param maxPrt        Max number of particles in the batch.
 *
 * \return a pointer to a particle batch on success, NULL otherwise
 *
 * \see RpPrtStdParticleBatchAddBatch
 * \see RpPrtStdParticleBatchDestroy
 */
RpPrtStdParticleBatch *
RpPrtStdParticleBatchCreate(RpPrtStdParticleClass * pClass,
                            RwInt32 maxPrt)
{
    RpPrtStdParticleBatch *prtBatch;
    RwInt32             size;

    RWAPIFUNCTION(RWSTRING("RpPrtStdParticleBatchCreate"));

    RWASSERT(pClass);

    size = ((sizeof(RpPrtStdParticleBatch) + 0x0F) & ~0x0F) + (pClass->objSize * maxPrt);

    prtBatch = (RpPrtStdParticleBatch *) RwMalloc(size,
        rwID_PRTSTDPLUGIN | rwMEMHINTDUR_EVENT);

    if (prtBatch)
    {
        prtBatch->maxPrt = maxPrt;
        prtBatch->prtClass = pClass;
        prtBatch->offset = ((sizeof(RpPrtStdParticleBatch) + 0x0F) & ~0x0F);

        _rpPrtStdParticleBatchCtor(prtBatch);
    }

    RWRETURN(prtBatch);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdParticleBatchDestroy is the function to destroy a particle
 * batch.
 *
 * \param prtBatch      Pointer to the particle batch.
 *
 * \return TRUE on success, FALSE otherwise
 *
 * \see RpPrtStdParticleBatchAddBatch
 * \see RpPrtStdParticleBatchCreate
 */
RwBool
RpPrtStdParticleBatchDestroy(RpPrtStdParticleBatch * prtBatch)
{
    RwBool              result;

    RWAPIFUNCTION(RWSTRING("RpPrtStdParticleBatchDestroy"));

    RWASSERT(prtBatch);

    result = FALSE;

    if (prtBatch)
    {
        _rpPrtStdParticleBatchDtor(prtBatch);

        RwFree(prtBatch);

        result = TRUE;
    }

    RWRETURN(result);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdParticleBatchAddBatch is the function to add a particle
 * batch to circular linked particle batch list.
 *
 * If the list is empty, then the particle batch forms a new list. If
 * the list is not empty, then the particle batch is added to the end of
 * the list. The particle class of the list and the batch must match.
 *
 * \param prtBatchHead      Pointer to the particle batch list.
 * \param prtBatch          Pointer to the particle batch.
 *
 * \return a pointer to a particle batch on success, NULL otherwise
 *
 * \see RpPrtStdParticleBatchCreate
 * \see RpPrtStdParticleBatchDestroy
 */
RpPrtStdParticleBatch *
RpPrtStdParticleBatchAddBatch(RpPrtStdParticleBatch * prtBatchHead,
                              RpPrtStdParticleBatch * prtBatch)
{
    RWAPIFUNCTION(RWSTRING("RpPrtStdParticleBatchAddBatch"));

    RWASSERT(prtBatch);

    if (prtBatchHead == NULL)
    {
        prtBatch->next = prtBatch;
    }
    else
    {
        /* The particle class must match. */
        RWASSERT(prtBatch->prtClass == prtBatchHead->prtClass);

        prtBatch->next = prtBatchHead->next;

        prtBatchHead->next = prtBatch;
    }

    RWRETURN(prtBatch);
}

/************************************************************************
 *
 *
 ************************************************************************/

RpPrtStdEmitter    *
_rpPrtStdEmitterCtor(RpPrtStdEmitter * emt, void * data)
{
    RwInt32             i, offset;
    RpPrtStdEmitterPTank *emtPTank;
    RwChar             *ptr;

    RpPrtStdEmitterClass *eClass;
    RpPrtStdEmitterCallBack callback;

    RWFUNCTION(RWSTRING("_rpPrtStdEmitterCtor"));

    RWASSERT(emt);

    emt->next = emt;
    emt->flag = 0;
    emt->atomic = NULL;
    emt->prtActive = 0;
    emt->prtBatchMaxPrt = 0;
    emt->prtClass = NULL;

    emt->inactiveBatch = NULL;
    emt->activeBatch = NULL;

    ptr = ((RwChar *) emt) + sizeof(RpPrtStdEmitter);

    memset(ptr, 0, (emt->emtClass->objSize - sizeof(RpPrtStdEmitter)));

    /* Set up the pTank if required. */
    offset = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERPTANK);

    if (offset >= 0)
    {
        emtPTank = (RpPrtStdEmitterPTank *) RPPRTSTDPROP(emt, offset);

        _rpPrtStdPTankCtor(emtPTank);
    }

    /* Call the callbacks. */
    eClass = emt->emtClass;
    for (i = 0; i < eClass->numCallback; i++)
    {
        if ((callback = (eClass->callback[i][rpPRTSTDEMITTERCALLBACKCREATE])) != NULL)
        {
            if ((callback) (NULL, emt, data) != emt)
                RWRETURN((RpPrtStdEmitter *) NULL);
        }
    }

    RWRETURN(emt);
}

RpPrtStdEmitter    *
_rpPrtStdEmitterDtor(RpPrtStdEmitter * emt)
{
    RwInt32             i, offset;
    RpPrtStdEmitterPTank *emtPTank;
    RwChar             *ptr;

    RpPrtStdEmitterClass *eClass;
    RpPrtStdEmitterCallBack callback;

    RWFUNCTION(RWSTRING("_rpPrtStdEmitterDtor"));

    RWASSERT(emt);
    RWASSERT(emt->emtClass);

    /* Destroy any emitter batch. */
    RpPrtStdEmitterDestroyParticleBatch(emt);

    /* Call the callbacks. */
    eClass = emt->emtClass;
    for (i = 0; i < eClass->numCallback; i++)
    {
        if ((callback = (eClass->callback[i][rpPRTSTDEMITTERCALLBACKDESTROY])) != NULL)
        {
            if ((callback) (NULL, emt, NULL) != emt)
                RWRETURN((RpPrtStdEmitter *) NULL);
        }
    }

    ptr = ((RwChar *) emt) + sizeof(RpPrtStdEmitter);

    emt->next = NULL;

    emt->flag = 0;

    emt->prtActive = 0;
    emt->prtBatchMaxPrt = 0;

    emt->inactiveBatch = NULL;
    emt->activeBatch = NULL;

    emt->boundingSphere = (RwReal) 0.0;

    /* Set up the pTank if required. */
    offset = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERPTANK);

    if (offset >= 0)
    {
        emtPTank = (RpPrtStdEmitterPTank *) RPPRTSTDPROP(emt, offset);

        _rpPrtStdPTankDtor(emtPTank);
    }

    RpPrtStdEClassDestroy(emt->emtClass);
    RpPrtStdPClassDestroy(emt->prtClass);

    RWRETURN(emt);
}

RpPrtStdEmitter *
_rpPrtStdEmitterCopy(RpPrtStdEmitter *dstEmt, RpPrtStdEmitter *emt)
{
    RwInt32                     i;
    RpPrtStdEmitterClass        *eClass;
    RpPrtStdEmitterCallBack     callback;

    RWFUNCTION(RWSTRING("_rpPrtStdEmitterCopy"));

    RWASSERT(dstEmt);
    RWASSERT(emt);

    /* Call the callbacks. */
    eClass = emt->emtClass;

    for (i = 0; i < eClass->numCallback; i++)
    {
        if ((callback = (eClass->callback[i][rpPRTSTDEMITTERCALLBACKCLONE])) != NULL)
        {
            if ((callback) (emt->atomic, emt, (void *) dstEmt) != dstEmt)
                RWRETURN((RpPrtStdEmitter *) NULL);
        }
    }

    RWRETURN(dstEmt);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterCreate is the function to create a new emitter.
 *
 * \param eClass        Pointer to the emitter class.
 * \param data          Private data passed to the create callbacks.
 *
 * \return a pointer to an emitter on success, NULL otherwise
 *
 * \see RpPrtStdEmitterAddEmitter
 * \see RpPrtStdEmitterClone
 * \see RpPrtStdEmitterDestroy
 */
RpPrtStdEmitter    *
RpPrtStdEmitterCreate(RpPrtStdEmitterClass * eClass, void * data)
{
    RpPrtStdEmitter    *emt;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterCreate"));

    RWASSERT(eClass);

    emt = (RpPrtStdEmitter *) RwMalloc(sizeof(RpPrtStdEmitter) + eClass->objSize,
                                       rwID_PRTSTDPLUGIN | rwMEMHINTDUR_EVENT);

    if (emt)
    {
        eClass->refCount++;

        emt->emtClass = eClass;

        emt = _rpPrtStdEmitterCtor(emt, data);
    }

    /* Call the creates callbacks. */

    RWRETURN(emt);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterDestroy is the function to destroy an emitter.
 *
 * \param emt       Pointer to the emitter.
 *
 * \return TRUE on success, FALSE otherwise
 *
 * \see RpPrtStdEmitterAddEmitter
 * \see RpPrtStdEmitterClone
 * \see RpPrtStdEmitterCreate
 */
RwBool
RpPrtStdEmitterDestroy(RpPrtStdEmitter * emt)
{
    RwBool              result;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterDestroy"));

    RWASSERT(emt);

    /* Call the destroy callbacks. */

    result = FALSE;
    if (_rpPrtStdEmitterDtor(emt) == emt)
    {
        RwFree(emt);

        result = TRUE;
    }

    RWRETURN(result);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterClone is the function to clone an emitter.
 *
 * \param emt       Pointer to the emitter.
 *
 * \return A pointer to the new emitter on success, null otherwise.
 *
 * \see RpPrtStdEmitterCreate
 * \see RpPrtStdEmitterDestroy
 */
RpPrtStdEmitter *
RpPrtStdEmitterClone(RpPrtStdEmitter * emt)
{
    RpPrtStdEmitter             *newEmt;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterClone"));

    RWASSERT(emt);

    newEmt = RpPrtStdEmitterCreate(emt->emtClass, NULL);

    if (newEmt)
    {
        newEmt->flag = emt->flag;

        RpPrtStdEmitterSetPClass(newEmt, emt->prtClass, emt->prtBatchMaxPrt);

        if (_rpPrtStdEmitterCopy(newEmt, emt) != newEmt)
        {
            RpPrtStdEmitterDestroy(newEmt);

            newEmt = NULL;
        }
    }

    RWRETURN(newEmt);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterAddEmitter is the function to add an emitter
 * to a circular linked emitter list.
 *
 * If the list is empty, then the emitter forms a new list. If
 * the list is not empty, then the emitter is added to the end of
 * the list. The emitter class of the list and the emitter must match.
 *
 * \param emtHead        Pointer to the emitter list.
 * \param emt           Pointer to the emitter.
 *
 * \return a pointer to an emitter on success, NULL otherwise
 *
 * \see RpPrtStdEmitterCreate
 * \see RpPrtStdEmitterDestroy
 */
RpPrtStdEmitter    *
RpPrtStdEmitterAddEmitter(RpPrtStdEmitter * emtHead,
                          RpPrtStdEmitter * emt)
{
    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterAddEmitter"));

    RWASSERT(emt);

    if (emtHead == NULL)
    {
        emt->next = emt;
    }
    else
    {
        emt->next = emtHead->next;

        emtHead->next = emt;
    }

    RWRETURN(emt);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterForAllParticleBatch is a utility function to apply
 * a callback function to all particle batches attached to the emitter.
 *
 * If the emiter does not contain any particle batches, then the function
 * does nothing.
 *
 * The library rpprtstd and the header file rpprtstd.h are required.
 *
 * \param emt           Pointer to an emitter.
 * \param callback      Callback function to be called for the particle batches.
 * \param data          Pointer to private data to be passed to the callback.
 *
 * \return a pointer to an emitter on success, NULL otherwise
 *
 * \see RpPrtStdEmitterNewParticleBatch
 * \see RpPrtStdEmitterDestroyParticleBatch
 */
RpPrtStdEmitter    *
RpPrtStdEmitterForAllParticleBatch(RpPrtStdEmitter * emt,
                                   RpPrtStdParticleCallBack callback,
                                   void *data)
{
    RpPrtStdParticleBatch *prtBatch, *prtBatchHead, *prtBatchNext;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterForAllParticleBatch"));

    RWASSERT(emt);
    RWASSERT(callback);

    if (emt->activeBatch != NULL)
    {
        prtBatchHead = emt->activeBatch->next;
        prtBatch = emt->activeBatch->next;

        do
        {
            RWASSERT(prtBatch);

            prtBatchNext = prtBatch->next;

            if ((callback) (emt, prtBatch, data) != prtBatch)
            {
                RWRETURN((RpPrtStdEmitter *) NULL);
            }

            prtBatch = prtBatchNext;
        }
        while (prtBatch != prtBatchHead);
    }

    RWRETURN(emt);
}

/************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterDestroyParticleBatch is a function to destroy all
 * particle batches attached to the emitter.
 *
 * If the emiter does not contain any particle batches, then the function
 * does nothing.
 *
 * The library rpprtstd and the header file rpprtstd.h are required.
 *
 * \param emt           Pointer to an emitter.
 *
 * \return a pointer to an emitter on success, NULL otherwise
 *
 * \see RpPrtStdParticleBatchCreate
 * \see RpPrtStdParticleBatchDestroy
 * \see RpPrtStdEmitterNewParticleBatch
 */
RpPrtStdEmitter    *
RpPrtStdEmitterDestroyParticleBatch(RpPrtStdEmitter * emt)
{
    RwInt32             i;
    RpPrtStdParticleClass *pClass;
    RpPrtStdParticleBatch *prtBatchArray[2], *prtBatch,
        *prtBatchHead, *prtBatchNext;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterDestroyParticleBatch"));

    RWASSERT(emt);

    prtBatchArray[0] = emt->activeBatch;
    prtBatchArray[1] = emt->inactiveBatch;

    for (i = 0; i < 2; i++)
    {
        if (prtBatchArray[i] != NULL)
        {

            prtBatchHead = prtBatchArray[i]->next;
            prtBatch = prtBatchArray[i]->next;

            pClass = prtBatch->prtClass;

            do
            {
                RWASSERT(prtBatch);

                prtBatchNext = prtBatch->next;

                /*
                 * If we are destroying an active batch, we need to use
                 * RpPrtStdParticleBatchDestroy so any internal data
                 * are cleaned up correctly.
                 *
                 * If we are destroying an inactive batch, then it should
                 * have already been cleaned up so we just free it to
                 * avoid calling the clean up code twice. If the user needs
                 * to dealloc some memory, then it may get done twice which
                 * will cause problems.
                 */

                if (i == 0)
                {
                    if (RpPrtStdParticleBatchDestroy(prtBatch) == FALSE)
                    {
                        RWRETURN((RpPrtStdEmitter *) NULL);
                    }
                }
                else
                {
                    RwFree(prtBatch);
                }

                prtBatch = prtBatchNext;
            }
            while (prtBatch != prtBatchHead);
        }
    }

    /* Reset the ptrs. */
    emt->activeBatch = NULL;
    emt->inactiveBatch = NULL;
    emt->prtActive = 0;

    RWRETURN(emt);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterNewParticleBatch is the function to return an
 * empty particle batch for use with the emitter.
 *
 * The new particle batch will be added to the list of particle batches
 * attached to the emitter.
 *
 * \param emt           Pointer to the emitter.
 *
 * \return a pointer to a particle batch on success, NULL otherwise
 *
 * \see RpPrtStdParticleBatchCreate
 * \see RpPrtStdParticleBatchDestroy
 * \see RpPrtStdEmitterDestroyParticleBatch
 */
RpPrtStdParticleBatch *
RpPrtStdEmitterNewParticleBatch(RpPrtStdEmitter * emt)
{
    RpPrtStdParticleBatch *prtBatch;
    RpPrtStdParticleClass *pClass;

    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterNewParticleBatch"));

    RWASSERT(emt);

    pClass = emt->prtClass;

    RWASSERT(pClass);

    if (emt->inactiveBatch == NULL)
    {
        /* Create a new batch. */
        prtBatch = RpPrtStdParticleBatchCreate(pClass, emt->prtBatchMaxPrt);
    }
    else
    {
        /* Remove the batch from the inactive list. */
        prtBatch = emt->inactiveBatch->next;

        if (prtBatch == emt->inactiveBatch)
        {
            emt->inactiveBatch = NULL;
        }
        else
        {
            emt->inactiveBatch->next = prtBatch->next;
        }

        prtBatch->next = prtBatch;

        _rpPrtStdParticleBatchCtor(prtBatch);

    }

    /* Add the batch to the active list. */
    if (prtBatch)
    {
        emt->activeBatch = RpPrtStdParticleBatchAddBatch(emt->activeBatch, prtBatch);
    }

    RWRETURN(prtBatch);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterAddParticleBatch is the function to add a particle
 * batch to an emitter.
 *
 * If the list is empty, then the particle batch forms a new list. If
 * the list is not empty, then the particle batch is added to the end of
 * the list. The particle class of the emitter and the particle batch
 * must match.
 *
 * \param emt               Pointer to the emitter.
 * \param prtBatch          Pointer to the particle batch.
 *
 * \return a pointer to an emitter on success, NULL otherwise
 *
 * \see
 */
RpPrtStdEmitter    *
RpPrtStdEmitterAddParticleBatch(RpPrtStdEmitter * emt,
                                RpPrtStdParticleBatch * prtBatch)
{
    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterAddParticleBatch"));

    RWASSERT(emt);
    RWASSERT(prtBatch);

    /* Add the batch to the active list. */
    emt->activeBatch = RpPrtStdParticleBatchAddBatch(emt->activeBatch, prtBatch);

    RWRETURN(emt);
}

/************************************************************************
 *
 *
 ************************************************************************/

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterSetPClass is the function to set the particle
 * class to an emitter.
 *
 * Particles emitted from the emitter will be created using this particle class.
 * Any existing particle batches attached to the emitter will be destroyed as
 * they will be unusable with the new particle class.
 *
 * \param emt               A pointer to the emitter.
 * \param pClass            A pointer to the particle class.
 * \param maxPrt            A \ref RwInt32 for the particle batch size.
 *
 * \return a pointer to an emitter on success, NULL otherwise
 *
 * \see RpPrtStdPClassCreate
 * \see RpPrtStdPClassDestroy
 * \see RpPrtStdEmitterGetPClass
 */
RpPrtStdEmitter    *
RpPrtStdEmitterSetPClass(RpPrtStdEmitter * emt,
                                RpPrtStdParticleClass * pClass,
                                RwInt32 maxPrt)
{
    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterSetPClass"));

    RWASSERT(emt);
    RWASSERT(pClass);

    /* Remove the existing class's particle batches */
    if (emt->prtClass != NULL)
    {
        RpPrtStdEmitterDestroyParticleBatch(emt);

        RpPrtStdPClassDestroy(emt->prtClass);
    }

    emt->prtClass = pClass;
    emt->prtBatchMaxPrt = maxPrt;

    pClass->refCount++;

    RWRETURN(emt);
}

/**
 * \ingroup rpprtstd
 * \ref RpPrtStdEmitterGetPClass is the function to return the particle
 * class attached to an emitter.
 *
 * Particles from the emitter will be created using this particle class.
 *
 * \param emt         A pointer to the emitter.
 * \param pClass      A pointer to the particle class.
 * \param maxPrt      A pointer to a \ref RwInt32 for the particle batch size.
 *
 * \return a pointer to an emitter on success, NULL otherwise
 *
 * \see RpPrtStdPClassCreate
 * \see RpPrtStdPClassDestroy
 * \see RpPrtStdEmitterSetPClass
 */
RpPrtStdEmitter    *
RpPrtStdEmitterGetPClass(RpPrtStdEmitter * emt,
                                RpPrtStdParticleClass ** pClass,
                                RwInt32 * maxPrt)
{
    RWAPIFUNCTION(RWSTRING("RpPrtStdEmitterGetPClass"));

    RWASSERT(emt);

    *pClass = emt->prtClass;
    *maxPrt = emt->prtBatchMaxPrt;

    RWRETURN(emt);
}

/************************************************************************
 *
 *
 ************************************************************************/

RpPrtStdEmitter    *
_rpPrtStdEmitterUpdate(RpAtomic * atomic, RpPrtStdEmitter * emt,
                       void *data)
{
    RpPrtStdEmitterClass *eClass;
    RpPrtStdParticleClass *pClass;

    RpPrtStdEmitter    *result;
    RpPrtStdParticleBatch *prtBatch;
    RpPrtStdParticleBatch *prtBatchHead;
    RpPrtStdParticleBatch *prtBatchNext;
    RpPrtStdParticleBatch *prtBatchHeadNew;
    RwInt32             i, prtActive;
    RwSphere            *sphere;

    RwUInt32            pTankLock;
    RpPrtStdEmitterPTank *emtPTank;
    RpPrtStdEmitterCallBack emtCallback;
    RpPrtStdParticleCallBack prtCallback;

#if (defined(SKY2_EMITUPDATECOUNT))
    RwInt32             control;
#endif /* (defined(SKY2_EMITUPDATECOUNT)) */

#if (0)
    RwInt32             offset;
    RpPrtStdParticleBatch *prtBatchPrev;
#endif /* (0) */

    RWFUNCTION(RWSTRING("_rpPrtStdEmitterUpdate"));

    /* Useful counters */

#ifdef SKY2_EMITUPDATECOUNT

    sky_c = 0;

    /* set which counters you want */
    control =
        SCE_PC0_CPU_CYCLE | (SCE_PC_U0 | SCE_PC_S0 | SCE_PC_K0 |
                             SCE_PC_EXL0);
    /*    control  = SCE_PC0_ICACHE_MISS | (SCE_PC_U0); */
    control |= SCE_PC1_DCACHE_MISS | (SCE_PC_U1);
    control |= SCE_PC_CTE;

    /* Disable interrupts to get an accurate profile */
    DI();

    /* start */
    scePcStop();
    scePcStart(control, 0, 0);

#endif /* (SKY2_EMITUPDATECOUNT) */

    result = emt;

    RWASSERT(atomic);
    RWASSERT(emt);

    eClass = emt->emtClass;
    pClass = emt->prtClass;

    RWASSERT(eClass);
    RWASSERT(pClass);

    /* Is the emitter active ? */
    if (emt->flag & rpPRTSTDEMITTERFLAGACTIVE)
    {
        _rpPrtStdEmitterGetPropOffset(emt);

        /* If the emitter does not have a PTank, create one now. */
        pTankLock = 0;
        if (emt->flag & rpPRTSTDEMITTERFLAGPTANK)
        {
            RWASSERT(rpPrtStdGlobals.offsetEmtPTank >= 0);

            emtPTank = (RpPrtStdEmitterPTank *) RPPRTSTDPROP(emt, rpPrtStdGlobals.offsetEmtPTank);

            if (emtPTank->pTank == NULL)
                RpPrtStdEmitterCreatePTank(emt);

            if (emtPTank->pTank == NULL)
                RWRETURN((RpPrtStdEmitter *) NULL);
        }
        else
            emtPTank = NULL;

        /* Begin update for the emitter. */
        if (emt->flag & rpPRTSTDEMITTERFLAGUPDATE)
        {
            pTankLock |= 0x02;

#if (defined(SKY2_DRVMODEL_H) &&  defined(SKY_USE_MULTIBUFERING))
            if( !(emt->flag & rpPRTSTDEMITTERFLAGNOBUFFERSWAP) )
            {
                RwBool result = RpPTankAtomicSkySwapFrames(emtPTank->pTank);

                if( FALSE == result )
                {
                    RWMESSAGE((RWSTRING("The current PTank buffer is still in "
                                       "use, particle update can't be"
                                       " applied.\n"
                                       "Please Ensure that the call to "
                                       "RpPrtStdAtomicUpdate is correctly "
                                       "synchronised.\n"
                                       "Place a call to _rwDMAWaitQueue before,"
                                       " or, if the application allow it, "
                                       "between RwCameraBeginUpdate "
                                       "and RwCameraEndUpdate")));
                }
            }
#endif /* (defined(SKY2_DRVMODE_H) &&  defined(SKY_USE_MULTIBUFERING)) */

            for (i = 0; i < eClass->numCallback; i++)
            {
                if ((emtCallback = (eClass->callback[i][rpPRTSTDEMITTERCALLBACKBEGINUPDATE])) != NULL)
                {
                    if ((emtCallback) (atomic, emt, data) != emt)
                        RWRETURN((RpPrtStdEmitter *) NULL);
                }
            }
        }

        sphere = RpAtomicGetBoundingSphere(atomic);

        emt->boundingSphere = (RwReal) 0.0;

        /* Update the particles from this emitter. */
        prtActive = 0;

        if ((emt->activeBatch != NULL) && (emt->flag & rpPRTSTDEMITTERFLAGUPDATEPARTICLE))
        {
            /* Lock the pTank if required. */
            if (emtPTank)
            {
                _rpPrtStdPTankGetDataPtr(emtPTank);

                emtPTank->numPrt = 0;

                pTankLock |= 0x01;
            }

            /* Main update loop. */
            prtBatchHeadNew = NULL;
            prtBatchHead = emt->activeBatch->next;
            prtBatch = prtBatchHead;

            /* Reset the active batch list to be empty */
            emt->activeBatch = NULL;

            do
            {
                RWASSERT(prtBatch);

                /* Update the new prt position */
                prtBatch->newPrt = -1;

                prtBatchNext = prtBatch->next;

                /* Check if we are to update or remove this batch. */
                if (prtBatch->numPrt <= 0)
                {
                    /* Call the destroy callback for the batch. */
                    for (i = 0; i < pClass->numCallback; i++)
                    {
                        if ((prtCallback = (pClass->callback[i][rpPRTSTDPARTICLECALLBACKDESTROY])) != NULL)
                        {
                            if ((prtCallback)(emt, prtBatch, data) != prtBatch)
                                RWRETURN((RpPrtStdEmitter *) NULL);
                        }
                    }

                    /* Add the batch to the inactive batch list. */
                    emt->inactiveBatch =
                        RpPrtStdParticleBatchAddBatch(emt->inactiveBatch, prtBatch);
                }
                else
                {
                    /* Call the update callback for the batch. */

                    for (i = 0; i < pClass->numCallback; i++)
                    {
                        if ((prtCallback = (pClass->callback[i][rpPRTSTDPARTICLECALLBACKUPDATE])) != NULL)
                        {
                            if ((prtCallback)(emt, prtBatch, data) != prtBatch)
                                RWRETURN((RpPrtStdEmitter *) NULL);
                        }
                    }

                    /* Add the batch to the active batch list. */
                    emt->activeBatch = RpPrtStdParticleBatchAddBatch(emt->activeBatch, prtBatch);

                    prtActive += prtBatch->numPrt;
                }

                prtBatch = prtBatchNext;
            }
            while (prtBatch != prtBatchHead);
        }
        else
        {
            prtActive = emt->prtActive;
        }

        /* Update the active count. */
        emt->prtActive = prtActive;

        /* Lock the pTank if required. */
        if (emtPTank && !(pTankLock & 0x01))
        {
            const RpPTankFormatDescriptor   *pTankFormat;
            RwInt32                         i;

            _rpPrtStdPTankGetDataPtr(emtPTank);

            /* If we're here that means that the
             * rpPRTSTDEMITTERFLAGUPDATEPARTICLE is off :
             * so let's guard band the active particles
             */
            pTankFormat = RpPTankAtomicGetDataFormat(emtPTank->pTank);


            for(i=0;i < pTankFormat->numClusters;i++)
            {
                emtPTank->dataInPtrs[i] += emtPTank->numPrt*emtPTank->dataStride[i];
                emtPTank->dataOutPtrs[i] += emtPTank->numPrt*emtPTank->dataStride[i];
            }

            /* emtPTank->numPrt = 0; */

            pTankLock |= 0x01;
        }

        /* Emit new particles. */
        if (emt->flag & rpPRTSTDEMITTERFLAGEMIT)
        {
#if (defined(SKY2_DRVMODEL_H) &&  defined(SKY_USE_MULTIBUFERING))
            if(!(emt->flag & rpPRTSTDEMITTERFLAGNOBUFFERSWAP) && !(pTankLock & 0x02))
            {
                RwBool result = RpPTankAtomicSkySwapFrames(emtPTank->pTank);

                if( FALSE == result )
                {
                    RWMESSAGE((RWSTRING("The current PTank buffer is still in "
                                       "use, particle update can't be"
                                       " applied.\n"
                                       "Please Ensure that the call to "
                                       "RpPrtStdAtomicUpdate is correctly "
                                       "synchronised.\n"
                                       "Place a call to _rwDMAWaitQueue before,"
                                       " or, if the application allows it, "
                                       "between RwCameraBeginUpdate "
                                       "and RwCameraEndUpdate")));
                }
            }
#endif /* (defined(SKY2_DRVMODE_H) &&  defined(SKY_USE_MULTIBUFERING)) */

            pTankLock |= 0x02;

            for (i = 0; i < emt->emtClass->numCallback; i++)
            {
                if ((emtCallback = (eClass->callback[i][rpPRTSTDEMITTERCALLBACKEMIT])) != NULL)
                {
                    if ((emtCallback) (atomic, emt, data) != emt)
                        RWRETURN((RpPrtStdEmitter *) NULL);
                }
            }
        }

        if (emtPTank && (pTankLock & 0x01))
        {
            RpPTankAtomicSetActiveParticlesCount(emtPTank->pTank, emtPTank->numPrt);

            RpPTankAtomicUnlock(emtPTank->pTank);

            pTankLock &= ~0x01;
        }

        /* End update for the emitter. */
        if (emt->flag & rpPRTSTDEMITTERFLAGUPDATE)
        {
            for (i = 0; i < eClass->numCallback; i++)
            {
                if ((emtCallback = (emt->emtClass->callback[i][rpPRTSTDEMITTERCALLBACKENDUPDATE])) != NULL)
                {
                    if ((emtCallback) (atomic, emt, data) != emt)
                        RWRETURN((RpPrtStdEmitter *) NULL);
                }
            }
        }

        /* Update the bounding sphere */
        if (emt->boundingSphere > 0)
            rwSqrt(&emt->boundingSphere, emt->boundingSphere);

        if (emt->boundingSphere > sphere->radius)
            sphere->radius = emt->boundingSphere;
    }

#ifdef SKY2_EMITUPDATECOUNT

    /* Grab the counters */
    sky_counter[0] = scePcGetCounter0();
    sky_counter[1] = scePcGetCounter1();
    sky_c = 2;

    scePcStop();

    /* Re-enable interrupts */
    EI();

#endif /* (SKY2_EMITUPDATECOUNT) */

    RWRETURN(result);
}

/************************************************************************/

RpPrtStdEmitter    *
_rpPrtStdEmitterRender(RpAtomic * atomic,
                       RpPrtStdEmitter * emt, void *data)
{
    RpPrtStdEmitterClass *eClass;
    RpPrtStdParticleClass *pClass;

    RwInt32             i;
    RpPrtStdParticleBatch *prtBatch, *prtBatchHead;

    RpPrtStdEmitterCallBack emtCallback;
    RpPrtStdParticleCallBack prtCallback;

#if (0)
    RpPrtStdEmitter    *result;
#endif /* (0) */

    RWFUNCTION(RWSTRING("_rpPrtStdEmitterRender"));

    RWASSERT(atomic);
    RWASSERT(emt);

    eClass = emt->emtClass;
    pClass = emt->prtClass;

    RWASSERT(eClass);
    RWASSERT(pClass);

    if (emt->flag & rpPRTSTDEMITTERFLAGACTIVE)
    {

        /* Get the props offsets */
        _rpPrtStdEmitterGetPropOffset(emt);

        /* Begin for the emitter. */
        if (emt->flag & rpPRTSTDEMITTERFLAGRENDER)
        {
            for (i = 0; i < eClass->numCallback; i++)
            {
                if ((emtCallback = (eClass->callback[i][rpPRTSTDEMITTERCALLBACKBEGINRENDER])) != NULL)
                {
                    if ((emtCallback) (atomic, emt, data) != emt)
                        RWRETURN((RpPrtStdEmitter *) NULL);
                }
            }
        }

        /* Render the particles from this emitter. */
        if ((emt->activeBatch != NULL) && (emt->flag & rpPRTSTDEMITTERFLAGRENDERPARTICLE))
        {
            prtBatchHead = emt->activeBatch->next;
            prtBatch = prtBatchHead;

            do
            {
                RWASSERT(prtBatch);

                for (i = 0; i < eClass->numCallback; i++)
                {
                    if ((prtCallback =(pClass->callback[i][rpPRTSTDPARTICLECALLBACKRENDER])) != NULL)
                    {
                        if ((prtCallback) (emt, prtBatch, data) != prtBatch)
                            RWRETURN((RpPrtStdEmitter *) NULL);
                    }
                }

                prtBatch = prtBatch->next;
            }
            while (prtBatch != prtBatchHead);
        }

        /* End render for the emitter. */
        if (emt->flag & rpPRTSTDEMITTERFLAGRENDER)
        {
            for (i = 0; i < eClass->numCallback; i++)
            {
                if ((emtCallback = (eClass->callback[i][rpPRTSTDEMITTERCALLBACKENDRENDER])) != NULL)
                {
                    if ((emtCallback) (atomic, emt, data) != emt)
                        RWRETURN((RpPrtStdEmitter *) NULL);
                }
            }
        }
    }

    RWRETURN(emt);
}

/************************************************************************
 *
 *
 ************************************************************************/

RpPrtStdEmitter    *
_rpPrtStdEmitterStreamRead(RpAtomic * atomic, RwStream * stream)
{
    RwInt32             i, eClassID, pClassID, prtBatchSize, flag, embedded;

    RpPrtStdPropertyTable   *ePropTab, *pPropTab;
    RpPrtStdEmitterClass    *eClass, *eClassTmp;
    RpPrtStdParticleClass   *pClass, *pClassTmp;
    RpPrtStdEmitterCallBack emtCallback;

    RpPrtStdEmitter    *emt;

    RWFUNCTION(RWSTRING("_rpPrtStdEmitterStreamRead"));

    RWASSERT(stream);

    /* Stream in the emitter class id and flag */
    RwStreamReadInt32(stream, &eClassID, sizeof(RwInt32));
    RwStreamReadInt32(stream, &flag, sizeof(RwInt32));

    /* Stream in the particle class id and prt batch size */
    RwStreamReadInt32(stream, &pClassID, sizeof(RwInt32));
    RwStreamReadInt32(stream, &prtBatchSize, sizeof(RwInt32));

    /* Emitter got class and props ? */
    RwStreamReadInt32(stream, &embedded, sizeof(RwInt32));

    if( embedded != 0 )
    {
        /* pClass proptab */
        if( !RwStreamFindChunk(stream, rwID_PRTSTDGLOBALDATA, (RwUInt32 *)NULL, (RwUInt32 *)NULL))
        {
            RWRETURN((RpPrtStdEmitter *) NULL);
        }

        pPropTab = RpPrtStdPropTabStreamRead(stream);

        /* eClass proptab */
        if( !RwStreamFindChunk(stream, rwID_PRTSTDGLOBALDATA, (RwUInt32 *)NULL, (RwUInt32 *)NULL))
        {
            RWRETURN((RpPrtStdEmitter *) NULL);
        }

        ePropTab = RpPrtStdPropTabStreamRead(stream);

        /* pClass */
        if( !RwStreamFindChunk(stream, rwID_PRTSTDGLOBALDATA, (RwUInt32 *)NULL, (RwUInt32 *)NULL))
        {
            RWRETURN((RpPrtStdEmitter *) NULL);
        }

        pClass = RpPrtStdPClassStreamRead(stream);

        /* eClass */
        if( !RwStreamFindChunk(stream, rwID_PRTSTDGLOBALDATA, (RwUInt32 *)NULL, (RwUInt32 *)NULL))
        {
            RWRETURN((RpPrtStdEmitter *) NULL);
        }

        eClass = RpPrtStdEClassStreamRead(stream);
    }
    else
    {
        ePropTab = NULL;
        pPropTab = NULL;

        /* Search for the eClass and pClass) */
        eClass = RpPrtStdEClassGet(eClassID);
        pClass = RpPrtStdPClassGet(pClassID);
    }

    RWASSERT(eClass);
    RWASSERT(pClass);

    /* Need to reset the property table. It is possible for the wrong table
     * to be attached. This happens if there are more than one property table
     * with the same set of properties but the properties are different size.
     *
     * The correct one is the one returned from streaming, where as the one
     * queried from the ID may be wrong. So reset the property table here.
     *
     * This is get around the problem of using the property IDs to generate
     * IDs.
     */
    if (ePropTab)
    {
        RWASSERT(pPropTab);

        /*
         * Check if there are classes that uses this prop tab
         */
        eClassTmp = RpPrtStdEClassGetByPropTab(ePropTab);

        /*
         * The class does not match, meaning the class returned from the
         * stream read is wrong. So create a new one with the prop tab.
         */
        if (eClassTmp != eClass)
        {
            if (eClassTmp == NULL)
            {
                eClassTmp = RpPrtStdEClassCreate();

                RpPrtStdEClassSetPropTab(eClassTmp, ePropTab);

                eClassTmp->id = eClass->id;
                eClassTmp->numCallback = 0;
                eClassTmp->callback = NULL;

                /* Call the setup callback. This needs to restores the render callbacks. */
                (rpPrtStdGlobals.emtClassSetupCallback)(eClassTmp);
            }
            else
                eClassTmp->refCount++;

            RpPrtStdEClassDestroy(eClass);
            eClass = eClassTmp;
        }

        /*
         * Check if there are classes that uses this prop tab
         */
        pClassTmp = RpPrtStdPClassGetByPropTab(pPropTab);

        /*
         * The class does not match, meaning the class returned from the
         * stream read is wrong. So create a new one with the prop tab.
         */
        if (pClassTmp != pClass)
        {
            if (pClassTmp == NULL)
            {
                pClassTmp = RpPrtStdPClassCreate();

                RpPrtStdPClassSetPropTab(pClassTmp, pPropTab);

                pClassTmp->id = pClass->id;
                pClassTmp->numCallback = 0;
                pClassTmp->callback = NULL;

                /* Call the setup callback. This needs to restores the render callbacks. */
                (rpPrtStdGlobals.prtClassSetupCallback)(pClassTmp);
            }
            else
                pClassTmp->refCount++;

            RpPrtStdPClassDestroy(pClass);
            pClass = pClassTmp;
        }
    }

    /* Create the emitter. */
    if ((emt = RpPrtStdEmitterCreate(eClass, NULL)) == NULL)
        RWRETURN((RpPrtStdEmitter *) NULL);

    emt->flag = flag|rpPRTSTDEMITTERFLAGSTREAMREAD;

    /* Set up the particle class and particle batch size */
    RpPrtStdEmitterSetPClass(emt, pClass, prtBatchSize);

    /* Stream in extension data. */
    for (i = 0; i < eClass->numCallback; i++)
    {
        if ((emtCallback = (eClass->callback[i][rpPRTSTDEMITTERCALLBACKSTREAMREAD])) != NULL)
        {
            if ((emtCallback) (atomic, emt, (void *) stream) != emt)
                RWRETURN((RpPrtStdEmitter *) NULL);
        }
    }

    /* Add the emitter to the atomic. */
    RpPrtStdAtomicAddEmitter(atomic, emt);

    /* If embedded, we need to decrement the property tables and classes ref counts */
    if (embedded)
    {
        RpPrtStdPropTabDestroy(eClass->propTab);
        RpPrtStdPropTabDestroy(pClass->propTab);

        RpPrtStdEClassDestroy(eClass);
        RpPrtStdPClassDestroy(pClass);
    }


    RWRETURN(emt);
}

RpPrtStdEmitter    *
_rpPrtStdEmitterStreamWrite(RpAtomic * atomic,
                            RpPrtStdEmitter * emt, void *data)
{
    RwInt32             i;
    RwStream           *stream;

    RpPrtStdEmitterClass *eClass;
    RpPrtStdParticleClass *pClass;
    RpPrtStdEmitterCallBack emtCallback;

    RWFUNCTION(RWSTRING("_rpPrtStdEmitterStreamWrite"));

    RWASSERT(emt);
    RWASSERT(data);

    stream = (RwStream *) data;

    eClass = emt->emtClass;
    pClass = emt->prtClass;

    RWASSERT(eClass);
    RWASSERT(pClass);

    /* Stream out the emitter class id and flag */
    RwStreamWriteInt32(stream, (RwInt32 *) & eClass->id, sizeof(RwInt32));
    RwStreamWriteInt32(stream, (RwInt32 *) & emt->flag, sizeof(RwInt32));

    /* Stream out the particle class id and prt batch size */
    RwStreamWriteInt32(stream, (RwInt32 *) & pClass->id, sizeof(RwInt32));
    RwStreamWriteInt32(stream, (RwInt32 *) & emt->prtBatchMaxPrt, sizeof(RwInt32));

    if( TRUE == rpPrtStdGlobals.rpPrtStdStreamEmbedded )
    {
        RwInt32 value = 1;

        /* Emitter got class and props ? */
        RwStreamWriteInt32(stream, &value, sizeof(RwInt32));

        /* pClass proptab */
        RpPrtStdPropTabStreamWrite(pClass->propTab,stream);

        /* eClass proptab */
        RpPrtStdPropTabStreamWrite(eClass->propTab,stream);

        /* pClass */
        RpPrtStdPClassStreamWrite(pClass,stream);

        /* eClass */
        RpPrtStdEClassStreamWrite(eClass,stream);
    }
    else
    {
        RwInt32 value = 0;
        /* Emitter got class and props ? */
        RwStreamWriteInt32(stream, &value, sizeof(RwInt32));
    }

    /* Stream out extension data. */
    for (i = 0; i < eClass->numCallback; i++)
    {
        if ((emtCallback = (eClass->callback[i][rpPRTSTDEMITTERCALLBACKSTREAMWRITE])) != NULL)
        {
            if ((emtCallback) (atomic, emt, data) != emt)
                RWRETURN((RpPrtStdEmitter *) NULL);
        }
    }

    RWRETURN(emt);
}

RpPrtStdEmitter    *
_rpPrtStdEmitterStreamGetSize(RpAtomic * atomic,
                              RpPrtStdEmitter * emt, void *data)
{
    RwInt32             i, size, totalSize;
    RpPrtStdEmitterClass *eClass;
    RpPrtStdParticleClass *pClass;
    RpPrtStdEmitterCallBack emtCallback;

    RWFUNCTION(RWSTRING("_rpPrtStdEmitterStreamGetSize"));

    RWASSERT(emt);

    eClass = emt->emtClass;
    pClass = emt->prtClass;

    RWASSERT(eClass);
    RWASSERT(pClass);

    /* Get the chunk size of emitter. */
    totalSize = 0;

    /* Size of  the emitter class id and flag */
    totalSize += 2 * sizeof(RwInt32);

    /* Size of  the particle class id and prt batch size */
    totalSize += 2 * sizeof(RwInt32);

    /* Emitter got class and props ? */
    totalSize += sizeof(RwInt32);

    if( TRUE == rpPrtStdGlobals.rpPrtStdStreamEmbedded )
    {
        /* pClass proptab */
        totalSize += RpPrtStdPropTabStreamGetSize(pClass->propTab);
        totalSize += rwCHUNKHEADERSIZE;

        /* eClass proptab */
        totalSize += RpPrtStdPropTabStreamGetSize(eClass->propTab);
        totalSize += rwCHUNKHEADERSIZE;

        /* pClass */
        totalSize += RpPrtStdPClassStreamGetSize(pClass);
        totalSize += rwCHUNKHEADERSIZE;

        /* eClass */
        totalSize += RpPrtStdEClassStreamGetSize(eClass);
        totalSize += rwCHUNKHEADERSIZE;

    }

    for (i = 0; i < eClass->numCallback; i++)
    {
        if ((emtCallback = (eClass->callback[i][rpPRTSTDEMITTERCALLBACKSTREAMGETSIZE])) != NULL)
        {
            if ((emtCallback) (atomic, emt, (void *) &size) != emt)
                RWRETURN((RpPrtStdEmitter *) NULL);

            totalSize += size;
        }
    }

    *(RwInt32 *) data += totalSize;

    RWRETURN(emt);
}

/************************************************************************
 *
 *
 ************************************************************************/

RpPrtStdEmitter    *
_rpPrtStdEmitterGetPropOffset(RpPrtStdEmitter * emt)
{
    RWFUNCTION(RWSTRING("_rpPrtStdEmitterGetPropOffset"));

    /* Get the known emitter props's offsets */
    if (emt->emtClass->propTab != rpPrtStdGlobals.emtPropTabCurr)
    {
        rpPrtStdGlobals.emtPropTabCurr = emt->emtClass->propTab;

        rpPrtStdGlobals.offsetEmtStd =
            RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERSTANDARD);

        rpPrtStdGlobals.offsetEmtPrtCol =
            RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERPRTCOLOR);

        rpPrtStdGlobals.offsetEmtPrtTex =
            RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERPRTTEXCOORDS);

        rpPrtStdGlobals.offsetEmtPrtSize =
            RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERPRTSIZE);

        rpPrtStdGlobals.offsetEmtPTank =
            RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERPTANK);

        rpPrtStdGlobals.offsetEmtPrtMtx =
            RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERPRTMATRIX);

        rpPrtStdGlobals.offsetEmtPrt2DRot =
            RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab, rpPRTSTDPROPERTYCODEEMITTERPRT2DROTATE);
    }

    /* Get the known particle props's offsets */
    if (emt->prtClass->propTab != rpPrtStdGlobals.prtPropTabCurr)
    {
        rpPrtStdGlobals.prtPropTabCurr = emt->prtClass->propTab;

        rpPrtStdGlobals.offsetPrtStd =
            RpPrtStdPropTabGetPropOffset(emt->prtClass->propTab, rpPRTSTDPROPERTYCODEPARTICLESTANDARD);

        rpPrtStdGlobals.offsetPrtCol =
            RpPrtStdPropTabGetPropOffset(emt->prtClass->propTab, rpPRTSTDPROPERTYCODEPARTICLECOLOR);

        rpPrtStdGlobals.offsetPrtTex =
            RpPrtStdPropTabGetPropOffset(emt->prtClass->propTab, rpPRTSTDPROPERTYCODEPARTICLETEXCOORDS);

        rpPrtStdGlobals.offsetPrtSize =
            RpPrtStdPropTabGetPropOffset(emt->prtClass->propTab, rpPRTSTDPROPERTYCODEPARTICLESIZE);

        rpPrtStdGlobals.offsetPrtPos =
            RpPrtStdPropTabGetPropOffset(emt->prtClass->propTab, rpPRTSTDPROPERTYCODEPARTICLEPOSITION);

        rpPrtStdGlobals.offsetPrtVel =
            RpPrtStdPropTabGetPropOffset(emt->prtClass->propTab, rpPRTSTDPROPERTYCODEPARTICLEVELOCITY);

        rpPrtStdGlobals.offsetPrtMtx =
            RpPrtStdPropTabGetPropOffset(emt->prtClass->propTab, rpPRTSTDPROPERTYCODEPARTICLEMATRIX);

        rpPrtStdGlobals.offsetPrt2DRot =
            RpPrtStdPropTabGetPropOffset(emt->prtClass->propTab, rpPRTSTDPROPERTYCODEPARTICLE2DROTATE);

        /* Get the known particle props's index */

        rpPrtStdGlobals.idxPrtStd =
            RpPrtStdPropTabGetPropIndex(emt->prtClass->propTab, rpPRTSTDPROPERTYCODEPARTICLESTANDARD);

        rpPrtStdGlobals.idxPrtCol =
            RpPrtStdPropTabGetPropIndex(emt->prtClass->propTab, rpPRTSTDPROPERTYCODEPARTICLECOLOR);

        rpPrtStdGlobals.idxPrtTex =
            RpPrtStdPropTabGetPropIndex(emt->prtClass->propTab, rpPRTSTDPROPERTYCODEPARTICLETEXCOORDS);

        rpPrtStdGlobals.idxPrtSize =
            RpPrtStdPropTabGetPropIndex(emt->prtClass->propTab, rpPRTSTDPROPERTYCODEPARTICLESIZE);

        rpPrtStdGlobals.idxPrtPos =
            RpPrtStdPropTabGetPropIndex(emt->prtClass->propTab, rpPRTSTDPROPERTYCODEPARTICLEPOSITION);

        rpPrtStdGlobals.idxPrtVel =
            RpPrtStdPropTabGetPropIndex(emt->prtClass->propTab, rpPRTSTDPROPERTYCODEPARTICLEVELOCITY);

        rpPrtStdGlobals.idxPrtMtx =
            RpPrtStdPropTabGetPropIndex(emt->prtClass->propTab, rpPRTSTDPROPERTYCODEPARTICLEMATRIX);

        rpPrtStdGlobals.idxPrt2DRot =
            RpPrtStdPropTabGetPropIndex(emt->prtClass->propTab, rpPRTSTDPROPERTYCODEPARTICLE2DROTATE);

    }

    RWRETURN(emt);
}
