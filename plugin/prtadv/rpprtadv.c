/**********************************************************************
 *
 * File :     rpprtadv.c
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
#include <rpprtstd.h>


#include "rpplugin.h"
#include "rpdbgerr.h"
#include "rpprtadv.h"

#include "prtadvvars.h"

#if (defined(RWDEBUG))
long                rpPrtAdvanceStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

/**
 * \ingroup rpprtadv
 * \page rpprtadvoverview RpPrtAdv Plugin Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rpptank.h, rpprtstd.h, rpprtadv.h
 * \li \b Libraries: rwcore, rpworld, rpptank, rpprtstd, rpprtadv
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach,
 *     \ref RpPTankPluginAttach, \ref RpPrtStdPluginAttach,
 *     \ref RpPrtAdvPluginAttach
 *
 * \subsection prtadvoverview Overview
 *
 * \ref rpprtadv is a collection of advanced properties and features for the
 * basic particle system \ref rpprtstd.
 *
 * The advanced particle tools and management is discussed in \ref rpprtadvmanage.
 * The remainder of this reference had been divided into sections, each dealing
 * with a specific particle extension:
 * \li \ref prtadvcircle
 * \li \ref prtadvsphere
 * \li \ref prtadvpointlist
 * \li \ref prtadvmultiprop
 * \li \ref prtadvtexstep
 * \li \ref prtadvprtemitter
 *
 * \par
 *
 * \ref rpprtadv adds three new types of emitter:
 * \li \ref prtadvcircle
 * \li \ref prtadvsphere
 * \li \ref prtadvpointlist
 *
 * \par
 * \par Circular and cylindrical shaped emitter
 * To create a circular or cylindrical shaped emitter class, add the flag
 * \b rpPRTADVEMITTERDATAFLAGCIRCLE when calling \ref RpPrtAdvEClassStdCreate.
 *
 * The emitting parameters are accessible using
 * the \b rpPRTADVPROPERTYCODEEMITTERCIRCLE property
 * with \ref RpPrtStdPropTabGetPropOffset.
 *
 * \par Spherical shaped emitter:
 * To create a spherical shaped emitter class, add the flag
 * \b rpPRTADVEMITTERDATAFLAGSPHERE when calling \ref RpPrtAdvEClassStdCreate.
 *
 * The emitting parameters are accessible using
 * the \b rpPRTADVPROPERTYCODEEMITTERSPHERE property
 * with \ref RpPrtStdPropTabGetPropOffset.
 *
 * \par Point list based emitter:
 * To create a point list based emitter class, add the flag
 * \b rpPRTADVEMITTERDATAFLAGPOINTLIST and pass a pointer to a
 * \ref RpPrtAdvParamBlock containing the number of points needed and the
 * optional use of a direction-per-point, when calling
 * \ref RpPrtAdvEClassStdCreate.
 *
 * The emitting parameters are accessible using
 * the \b rpPRTADVPROPERTYCODEEMITTERPOINTLIST property with
 * \ref RpPrtStdPropTabGetPropOffset, including the list of points and
 * directions.
 *
 * \note Only one type of emitter can be used in a single emitter class.
 *
 * \ref rpprtadv adds three new types of particle animation:
 * \li \ref prtadvmultiprop
 * \li \ref prtadvtexstep
 * \li \ref prtadvprtemitter
 *
 * \par
 *
 * \par Multi-property animations (multiple property and stepped texture coordinate animations):
 * Multi-property animations allow the creation of more complex animations for
 * color, size or texture coordinates. The basic particle animation system allows
 * simple color, size or texture coordinate animations, using a begin and
 * end value, whereas the multi-property animations can provide more complex
 * animations.
 *
 * For example:
 * \par Particle Standard Color Animation
 * Emitter Standard Start Color -> \e Interpolated \e between \e End \e and
 *  \e Start \e Color -> Emitter Standard End Color.
 *
 * \par Multi-property Color Animation
 * Emitter Standard Start Color -> \e Interpolated \e between \e First \e Frame
 * \e and \e Start \e Color -> First Frame Color -> \e Interpolated \e between
 * \e Second \e Frame \e Color \e and \e First \e Frame \e Color -> Second
 * Frame Color -> \e Interpolated \e between \e End \e Frame \e and \e Second
 * \e Frame \e Color -> Emitter Standard End Color.
 *
 * To create a multi-property emitter class, add one of the flags
 * \li \b rpPRTADVEMITTERDATAFLAGMULTICOLOR
 * \li \b rpPRTADVEMITTERDATAFLAGMULTITEXCOORDS
 * \li \b rpPRTADVEMITTERDATAFLAGMULTITEXCOORDSSTEP
 * \li \b rpPRTADVEMITTERDATAFLAGMULTISIZE
 *
 * and pass a pointer to a \ref RpPrtAdvParamBlock containing the number of
 * frame required when calling \ref RpPrtAdvEClassStdCreate.
 *
 * The emitting parameters are accessible using the
 * \li \b rpPRTADVPROPERTYCODEEMITTERPRTMULTICOLOR
 * \li \b rpPRTADVPROPERTYCODEEMITTERPRTMULTITEXCOORDS
 * \li \b rpPRTADVPROPERTYCODEEMITTERPRTMULTITEXCOORDSSTEP
 * \li \b rpPRTADVPROPERTYCODEEMITTERPRTMULTISIZE
 *
 * properties with \ref RpPrtStdPropTabGetPropOffset.
 *
 * To create a multi-property particle class, add one of the flags
 * \li \b rpPRTADVPARTICLEDATAFLAGMULTICOLOR
 * \li \b rpPRTADVPARTICLEDATAFLAGMULTITEXCOORDS
 * \li \b rpPRTADVPARTICLEDATAFLAGMULTITEXCOORDSSTEP
 * \li \b rpPRTADVPARTICLEDATAFLAGMULTISIZE
 *
 * and pass a pointer to a \ref RpPrtAdvParamBlock containing the number of
 * frame required when calling \ref RpPrtAdvPClassStdCreate.
 *
 * The emitting parameters are accessible using the
 * \li \b rpPRTADVPROPERTYCODEPARTICLEMULTICOLOR
 * \li \b rpPRTADVPROPERTYCODEPARTICLEMULTITEXCOORDS
 * \li \b rpPRTADVPROPERTYCODEPARTICLEMULTITEXCOORDSSTEP
 * \li \b rpPRTADVPROPERTYCODEPARTICLEMULTISIZE
 *
 * properties with \ref RpPrtStdPropTabGetPropOffset.
 *
 * \par Particle Emitters:
 * The use of particle emitters allow each particle to control another
 * emitter, the sub-emitter. A particle's position and velocity are used to
 * orient the sub-emitter.
 * The sub-emitter can carry any properties and any emitter type, even customized
 * user-data and callbacks.
 * A particle emitter system will always use the next emitter in the emitter
 * list attached to an RpAtomic.
 *
 * To create a particle emitter emitter class, add the flag
 * \b rpPRTADVEMITTERDATAFLAGPRTEMITTER
 * when calling \ref RpPrtAdvPClassStdCreate.
 *
 * The emitting parameters are accessible using the
 * \b rpPRTADVPROPERTYCODEEMITTERPRTCHAIN
 * property with \ref RpPrtStdPropTabGetPropOffset.
 *
 * To create a particle emitter particle class, add the flag
 * \b rpPRTADVPARTICLEDATAFLAGPRTEMITTER
 * and pass a pointer to a \ref RpPrtAdvParamBlock containing the number of
 * frame required when calling \ref RpPrtAdvPClassStdCreate.
 *
 * The emitting parameters are accessible using the
 * \b rpPRTADVPROPERTYCODEPARTICLEEMITTER
 * property with \ref RpPrtStdPropTabGetPropOffset.
 *
 *
 * \see \ref rpprtstdoverview
 * \see \ref rpptankoverview
 */

/************************************************************************
 *
 *
 ************************************************************************/

/*
 * Old data structures.
 */

typedef struct RpPrtAdvEmtPrtMultiColorOld
{
    RwUInt32                            numCol;
    RpPrtAdvEmitterPrtColorItem         *list;
}
RpPrtAdvEmtPrtMultiColorOld;

typedef struct RpPrtAdvEmtPrtMultiTexCoordsOld
{
    RwUInt32                            numTex;
    RpPrtAdvEmitterPrtTexCoordsItem     *list;
}
RpPrtAdvEmtPrtMultiTexCoordsOld;

typedef struct RpPrtAdvEmtPrtMultiSizeOld
{
    RwUInt32                            numSize;
    RpPrtAdvEmitterPrtSizeItem          *list;
}
RpPrtAdvEmtPrtMultiSizeOld;

typedef struct RpPrtAdvPrtMultiColorOld
{
    RwUInt32                            currIndex;
    RwReal                              *colTime;
    RwRGBAReal                          *startCol;
    RwRGBAReal                          *deltaCol;
}
RpPrtAdvPrtMultiColorOld;

typedef struct RpPrtAdvPrtMultiTexCoordsOld
{
    RwUInt32                            currIndex;
    RwReal                              *texTime;
    RwTexCoords                         *startUV0;
    RwTexCoords                         *deltaUV0;
    RwTexCoords                         *startUV1;
    RwTexCoords                         *deltaUV1;
}
RpPrtAdvPrtMultiTexCoordsOld;

typedef struct RpPrtAdvPrtMultiTexCoordsStepOld
{
    RwUInt32                            currIndex;
    RwReal                              *texTime;
    RwTexCoords                         *UV0;
    RwTexCoords                         *UV1;
}
RpPrtAdvPrtMultiTexCoordsStepOld;

typedef struct RpPrtAdvPrtMultiSizeOld
{
    RwUInt32                            currIndex;
    RwReal                              *sizeTime;
    RwV2d                               *startSize;
    RwV2d                               *deltaSize;
}
RpPrtAdvPrtMultiSizeOld;

/************************************************************************
 *
 *
 ************************************************************************/

RpPrtAdvGlobalVars rpPrtAdvGlobals;


/************************************************************************
 *
 *
 ************************************************************************/

/*
 * Post prop table read callback to re-adjust some property sizes.
 * Some property sizes were changed from previous versions and this can
 * cause memory problem.
 */
static RpPrtStdPropertyTable *
rpPrtAdvPropTabPostReadCBack( RpPrtStdPropertyTable * propTab )
{
    RwInt32                         numProp, size, sizeUA, oldSize, offset, i;

    RWFUNCTION(RWSTRING("rpPrtAdvPropTabPostReadCBack"));

    RWASSERT(propTab);

    /* Re-adjust some property sizes, only if it was from an older version
     * of the file.
     */
    if (_rpPrtStdStreamGetVersion() < rpPRTSTDVERSIONSTAMP)
    {
        numProp = propTab->numProp;
        offset = propTab->propSize[0];

        /* Naughty code alert.
         *
         * There isn't a way to distinguish if the property table is for
         * an emitter or a particle.
         *
         * So we look at the first property.
         * If the size matches that of the RpPrtStdParticleStandard, assume
         * it is a particle property table, otherwise it is an emitter
         * property table.
         */
        if (propTab->propSize[0] == sizeof(RpPrtStdParticleStandard))
        {
            /*
             * Assume this is particle property table.
             */
            for (i = 1; i < numProp; i++)
            {
                oldSize = propTab->propSizeUA[i];

                if (propTab->propID[i] == rpPRTADVPROPERTYCODEPARTICLEMULTICOLOR)
                {
                    sizeUA = oldSize -
                            sizeof(RpPrtAdvPrtMultiColorOld) +
                            sizeof(RpPrtAdvPrtMultiColor);

                    size = (sizeUA + (rwMATRIXALIGNMENT-1)) & ~(rwMATRIXALIGNMENT-1);
                }
                else if (propTab->propID[i] == rpPRTADVPROPERTYCODEPARTICLEMULTITEXCOORDS)
                {
                    sizeUA = oldSize -
                            sizeof(RpPrtAdvPrtMultiTexCoordsOld) +
                            sizeof(RpPrtAdvParticleMultiTexCoords);

                    size = (sizeUA + (rwMATRIXALIGNMENT-1)) & ~(rwMATRIXALIGNMENT-1);
                }
                else if (propTab->propID[i] == rpPRTADVPROPERTYCODEPARTICLEMULTITEXCOORDSSTEP)
                {
                    sizeUA = oldSize -
                            sizeof(RpPrtAdvPrtMultiTexCoordsStepOld) +
                            sizeof(RpPrtAdvPrtTexStep);

                    size = (sizeUA + (rwMATRIXALIGNMENT-1)) & ~(rwMATRIXALIGNMENT-1);
                }
                else if (propTab->propID[i] == rpPRTADVPROPERTYCODEPARTICLEMULTISIZE)
                {
                    sizeUA = oldSize -
                            sizeof(RpPrtAdvPrtMultiSizeOld) +
                            sizeof(RpPrtAdvPrtMultiSize);

                    size = (sizeUA + (rwMATRIXALIGNMENT-1)) & ~(rwMATRIXALIGNMENT-1);
                }
                else
                {
                    sizeUA = propTab->propSizeUA[i];
                    size = propTab->propSize[i];
                }

                propTab->propSizeUA[i] = sizeUA;
                propTab->propSize[i] = size;
                propTab->propOffset[i] = offset;

                offset += propTab->propSize[i];
            }
        }
        else
        {
            /*
             * Assume it is an emitter property table.
             */
            for (i = 1; i < numProp; i++)
            {
                oldSize = propTab->propSizeUA[i];

                if (propTab->propID[i] == rpPRTADVPROPERTYCODEEMITTERPRTMULTICOLOR)
                {
                    sizeUA = oldSize -
                            sizeof(RpPrtAdvEmtPrtMultiColorOld) +
                            sizeof(RpPrtAdvEmtPrtMultiColor);

                    size = (sizeUA + (rwMATRIXALIGNMENT-1)) & ~(rwMATRIXALIGNMENT-1);
                }
                else if ((propTab->propID[i] == rpPRTADVPROPERTYCODEEMITTERPRTMULTITEXCOORDS) ||
                         (propTab->propID[i] == rpPRTADVPROPERTYCODEEMITTERPRTMULTITEXCOORDSSTEP))
                {
                    sizeUA = oldSize -
                            sizeof(RpPrtAdvEmtPrtMultiTexCoordsOld) +
                            sizeof(RpPrtAdvEmtPrtMultiTexCoords);

                    size = (sizeUA + (rwMATRIXALIGNMENT-1)) & ~(rwMATRIXALIGNMENT-1);
                }
                else if (propTab->propID[i] == rpPRTADVPROPERTYCODEEMITTERPRTMULTISIZE)
                {
                    sizeUA = oldSize -
                            sizeof(RpPrtAdvEmtPrtMultiSizeOld) +
                            sizeof(RpPrtAdvEmtPrtMultiSize);

                    size = (sizeUA + (rwMATRIXALIGNMENT-1)) & ~(rwMATRIXALIGNMENT-1);
                }
                else
                {
                    sizeUA = propTab->propSizeUA[i];
                    size = propTab->propSize[i];
                }

                propTab->propSizeUA[i] = sizeUA;
                propTab->propSize[i] = size;
                propTab->propOffset[i] = offset;

                offset += propTab->propSize[i];
            }
        }

        /* Write the last offset */
        propTab->propOffset[i] = offset;
    }


    RWRETURN(propTab);
}

/*
 * Prop table destroy callback to reset the cached property table ptr.
 *
 * The property tab ptr is cached to reduce properties' offset query if the
 * property tab is the same as the previous property tab.
 *
 * If the current cached property tab is destroyed, then the ptr needs to be
 * reset.
 */
static RpPrtStdPropertyTable *
rpPrtAdvPropTabDtorCBack( RpPrtStdPropertyTable * propTab )
{
    RWFUNCTION(RWSTRING("rpPrtAdvPropTabDtorCBack"));

    RWASSERT(propTab);

    /* Need to reset the cache prop tab ptr if it is the one being destroyed. */
    if (rpPrtAdvGlobals.prtPropTabCurr == propTab)
        rpPrtAdvGlobals.prtPropTabCurr = NULL;
    else if (rpPrtAdvGlobals.emtPropTabCurr == propTab)
        rpPrtAdvGlobals.emtPropTabCurr = NULL;

    RWRETURN(propTab);
}


static void *
rpPrtAdvEngineOpen( void * ptr,
                     RwInt32 offset __RWUNUSED__,
                     RwInt32 size __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("rpPrtAdvEngineOpen"));

    _rpPrtStdSetPropTabPostReadCBack(rpPrtAdvPropTabPostReadCBack);

    _rpPrtStdSetPropTabDtorCBack(rpPrtAdvPropTabDtorCBack);

    RWRETURN(ptr);
}

static void *
rpPrtAdvEngineClose( void * ptr,
                     RwInt32 offset __RWUNUSED__,
                     RwInt32 size __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("rpPrtAdvEngineClose"));


    RWRETURN(ptr);
}

/************************************************************************/



/************************************************************************/

RpPrtStdEmitter    *
_rpPrtAdvEmitterGetPropOffset(RpPrtStdEmitter * emt)
{
    RWFUNCTION(RWSTRING("_rpPrtAdvEmitterGetPropOffset"));

    /* Get the known emitter props's offsets */
    if (emt->emtClass->propTab != rpPrtAdvGlobals.emtPropTabCurr)
    {
        rpPrtAdvGlobals.emtPropTabCurr = emt->emtClass->propTab;

        rpPrtAdvGlobals.offsetEmtStd = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
            rpPRTSTDPROPERTYCODEEMITTERSTANDARD);

        rpPrtAdvGlobals.offsetEmtPrtCol = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
            rpPRTSTDPROPERTYCODEEMITTERPRTCOLOR);

        rpPrtAdvGlobals.offsetEmtPrtTex = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
            rpPRTSTDPROPERTYCODEEMITTERPRTTEXCOORDS);

        rpPrtAdvGlobals.offsetEmtPrtSize = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
            rpPRTSTDPROPERTYCODEEMITTERPRTSIZE);

        rpPrtAdvGlobals.offsetEmtPTank = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
            rpPRTSTDPROPERTYCODEEMITTERPTANK);

        rpPrtAdvGlobals.offsetEmtPrtChain = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERPRTCHAIN);

        rpPrtAdvGlobals.offsetEmtPrtEmitter = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERPRTEMITTER);

        rpPrtAdvGlobals.offsetEmtPrtMultiCol = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERPRTMULTICOLOR);

        rpPrtAdvGlobals.offsetEmtPrtMultiTex = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERPRTMULTITEXCOORDS);

        rpPrtAdvGlobals.offsetEmtPrtMultiTexStep = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERPRTMULTITEXCOORDSSTEP);

        rpPrtAdvGlobals.offsetEmtPrtMultiSize = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERPRTMULTISIZE);

        rpPrtAdvGlobals.offsetEmtPtList = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERPOINTLIST);

        rpPrtAdvGlobals.offsetEmtCircle = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERCIRCLE);

        rpPrtAdvGlobals.offsetEmtSphere = RpPrtStdPropTabGetPropOffset(emt->emtClass->propTab,
            rpPRTADVPROPERTYCODEEMITTERSPHERE);

    }

    /* Get the known particle props's offsets */
    if (emt->prtClass->propTab != rpPrtAdvGlobals.prtPropTabCurr)
    {
        rpPrtAdvGlobals.prtPropTabCurr = emt->prtClass->propTab;

        rpPrtAdvGlobals.offsetPrtStd = RpPrtStdPropTabGetPropOffset(emt->prtClass->propTab,
            rpPRTSTDPROPERTYCODEPARTICLESTANDARD);

        rpPrtAdvGlobals.offsetPrtCol = RpPrtStdPropTabGetPropOffset(emt->prtClass->propTab,
            rpPRTSTDPROPERTYCODEPARTICLECOLOR);

        rpPrtAdvGlobals.offsetPrtTex = RpPrtStdPropTabGetPropOffset(emt->prtClass->propTab,
            rpPRTSTDPROPERTYCODEPARTICLETEXCOORDS);

        rpPrtAdvGlobals.offsetPrtSize = RpPrtStdPropTabGetPropOffset(emt->prtClass->propTab,
            rpPRTSTDPROPERTYCODEPARTICLESIZE);

        rpPrtAdvGlobals.offsetPrtPos = RpPrtStdPropTabGetPropOffset(emt->prtClass->propTab,
            rpPRTSTDPROPERTYCODEPARTICLEPOSITION);

        rpPrtAdvGlobals.offsetPrtVel = RpPrtStdPropTabGetPropOffset(emt->prtClass->propTab,
            rpPRTSTDPROPERTYCODEPARTICLEVELOCITY);

        rpPrtAdvGlobals.offsetPrtChain = RpPrtStdPropTabGetPropOffset(emt->prtClass->propTab,
            rpPRTADVPROPERTYCODEPARTICLECHAIN);

        rpPrtAdvGlobals.offsetPrtEmitter = RpPrtStdPropTabGetPropOffset(emt->prtClass->propTab,
            rpPRTADVPROPERTYCODEPARTICLEEMITTER);

        rpPrtAdvGlobals.offsetPrtMultiCol = RpPrtStdPropTabGetPropOffset(emt->prtClass->propTab,
            rpPRTADVPROPERTYCODEPARTICLEMULTICOLOR);

        rpPrtAdvGlobals.offsetPrtMultiTex = RpPrtStdPropTabGetPropOffset(emt->prtClass->propTab,
            rpPRTADVPROPERTYCODEPARTICLEMULTITEXCOORDS);

        rpPrtAdvGlobals.offsetPrtMultiTexStep = RpPrtStdPropTabGetPropOffset(emt->prtClass->propTab,
            rpPRTADVPROPERTYCODEPARTICLEMULTITEXCOORDSSTEP);

        rpPrtAdvGlobals.offsetPrtMultiSize = RpPrtStdPropTabGetPropOffset(emt->prtClass->propTab,
            rpPRTADVPROPERTYCODEPARTICLEMULTISIZE);

        /* Get the known particle props's index */

        rpPrtAdvGlobals.idxPrtStd = RpPrtStdPropTabGetPropIndex(emt->prtClass->propTab,
            rpPRTSTDPROPERTYCODEPARTICLESTANDARD);

        rpPrtAdvGlobals.idxPrtCol = RpPrtStdPropTabGetPropIndex(emt->prtClass->propTab,
            rpPRTSTDPROPERTYCODEPARTICLECOLOR);

        rpPrtAdvGlobals.idxPrtTex = RpPrtStdPropTabGetPropIndex(emt->prtClass->propTab,
            rpPRTSTDPROPERTYCODEPARTICLETEXCOORDS);

        rpPrtAdvGlobals.idxPrtSize = RpPrtStdPropTabGetPropIndex(emt->prtClass->propTab,
            rpPRTSTDPROPERTYCODEPARTICLESIZE);

        rpPrtAdvGlobals.idxPrtPos = RpPrtStdPropTabGetPropIndex(emt->prtClass->propTab,
            rpPRTSTDPROPERTYCODEPARTICLEPOSITION);

        rpPrtAdvGlobals.idxPrtVel = RpPrtStdPropTabGetPropIndex(emt->prtClass->propTab,
            rpPRTSTDPROPERTYCODEPARTICLEVELOCITY);

        rpPrtAdvGlobals.idxPrtChain =RpPrtStdPropTabGetPropIndex(emt->prtClass->propTab,
            rpPRTADVPROPERTYCODEPARTICLECHAIN);

        rpPrtAdvGlobals.idxPrtEmitter = RpPrtStdPropTabGetPropIndex(emt->prtClass->propTab,
            rpPRTADVPROPERTYCODEPARTICLEEMITTER);

        rpPrtAdvGlobals.idxPrtMultiCol = RpPrtStdPropTabGetPropIndex(emt->prtClass->propTab,
            rpPRTADVPROPERTYCODEPARTICLEMULTICOLOR);

        rpPrtAdvGlobals.idxPrtMultiTex = RpPrtStdPropTabGetPropIndex(emt->prtClass->propTab,
            rpPRTADVPROPERTYCODEPARTICLEMULTITEXCOORDS);

        rpPrtAdvGlobals.idxPrtMultiSize = RpPrtStdPropTabGetPropIndex(emt->prtClass->propTab,
            rpPRTADVPROPERTYCODEPARTICLEMULTISIZE);
    }

    RWRETURN(emt);
}


/************************************************************************
 *
 *
 ************************************************************************/

/**
 * \ingroup rpprtadvmanage
 * \ref RpPrtAdvPluginAttach is called by the application to indicate
 * that the advanced particle plugin should be used. The call
 * to this function should be placed between \ref RwEngineInit and
 * \ref RwEngineOpen and the world plugin must already be attached.
 *
 * The library rpprtadv and the header file rpprtadv.h are required.
 *
 * \return TRUE on success, FALSE otherwise.
 */
RwBool
RpPrtAdvPluginAttach(void)
{
    RWAPIFUNCTION(RWSTRING("RpPrtAdvPluginAttach"));

    rpPrtAdvGlobals.engineOffset = RwEngineRegisterPlugin(0,
        rwID_PRTADVPLUGIN,
        (RwPluginObjectConstructor) rpPrtAdvEngineOpen,
        (RwPluginObjectDestructor) rpPrtAdvEngineClose);

    RpPrtStdSetEClassSetupCallBack(RpPrtAdvEClassStdSetupCB);
    RpPrtStdSetPClassSetupCallBack(RpPrtAdvPClassStdSetupCB);

    RWRETURN((TRUE));
}



/************************************************************************
 *
 *
 ************************************************************************/
static RwInt32
_rpPrtAdvEClassStdFillPropTab( RwInt32 dataFlag,
                                RwInt32 *propID,
                                RwInt32 *propSize,
                                RpPrtAdvParamBlock *param )
{
    RwInt32 i;
    RwInt32 emitterType;

    RWFUNCTION(RWSTRING("_rpPrtAdvEClassStdFillPropTab"));

    RWASSERT(dataFlag);
    RWASSERT(propID);
    RWASSERT(propSize);

    i=0;
    propID[0] = rpPRTSTDPROPERTYCODEEMITTER;
    propSize[0] = sizeof(RpPrtStdEmitter);
    i++;

    if (dataFlag & rpPRTSTDEMITTERDATAFLAGSTANDARD)
    {
        propID[i] = rpPRTSTDPROPERTYCODEEMITTERSTANDARD;
        propSize[i] = sizeof(RpPrtStdEmitterStandard);
        i++;
    }

    if (dataFlag & rpPRTSTDEMITTERDATAFLAGPRTCOLOR)
    {
        propID[i] = rpPRTSTDPROPERTYCODEEMITTERPRTCOLOR;
        propSize[i] = sizeof(RpPrtStdEmitterPrtColor);
        i++;
    }

    if (dataFlag & rpPRTSTDEMITTERDATAFLAGPRTTEXCOORDS)
    {
        propID[i] = rpPRTSTDPROPERTYCODEEMITTERPRTTEXCOORDS;
        propSize[i] = sizeof(RpPrtStdEmitterPrtTexCoords);
        i++;
    }

    if (dataFlag & rpPRTSTDEMITTERDATAFLAGPTANK)
    {
        propID[i] = rpPRTSTDPROPERTYCODEEMITTERPTANK;
        propSize[i] = sizeof(RpPrtStdEmitterPTank);
        i++;
    }

    if (dataFlag & rpPRTSTDEMITTERDATAFLAGPRTMATRIX)
    {
        propID[i] = rpPRTSTDPROPERTYCODEEMITTERPRTMATRIX;
        propSize[i] = sizeof(RpPrtStdEmitterPrtMatrix);
        i++;
    }

    if (dataFlag & rpPRTSTDEMITTERDATAFLAGPRTSIZE)
    {
        propID[i] = rpPRTSTDPROPERTYCODEEMITTERPRTSIZE;
        propSize[i] = sizeof(RpPrtStdEmitterPrtSize);
        i++;
    }

    if (dataFlag & rpPRTSTDEMITTERDATAFLAGPRT2DROTATE)
    {
        propID[i] = rpPRTSTDPROPERTYCODEEMITTERPRT2DROTATE;
        propSize[i] = sizeof(RpPrtStdEmitterPrt2DRotate);
        i++;
    }

    if (dataFlag & rpPRTADVEMITTERDATAFLAGPRTEMITTER)
    {
        propID[i] = rpPRTADVPROPERTYCODEEMITTERPRTEMITTER;
        propSize[i] = sizeof(RpPrtAdvEmtPrtEmt);
        i++;
    }

    if (dataFlag & rpPRTADVEMITTERDATAFLAGMULTICOLOR)
    {
        RWASSERT((dataFlag & rpPRTSTDEMITTERDATAFLAGPRTCOLOR));
        RWASSERT(param);
        RWASSERT(param->colorFrames.numFrames > 0);

        propID[i] = rpPRTADVPROPERTYCODEEMITTERPRTMULTICOLOR;

        propSize[i] = sizeof(RpPrtAdvEmtPrtMultiColor) +
                      param->colorFrames.numFrames * sizeof(RpPrtAdvEmtPrtColorItem);

        i++;
    }

    if (dataFlag & rpPRTADVEMITTERDATAFLAGMULTITEXCOORDS)
    {
        RWASSERT((dataFlag & rpPRTSTDEMITTERDATAFLAGPRTTEXCOORDS));
        RWASSERT(0 == (dataFlag & rpPRTADVEMITTERDATAFLAGMULTITEXCOORDSSTEP));
        RWASSERT(param);
        RWASSERT(param->texFrames.numFrames > 0);

        propID[i] = rpPRTADVPROPERTYCODEEMITTERPRTMULTITEXCOORDS;

        propSize[i] = sizeof(RpPrtAdvEmtPrtMultiTexCoords) +
                      param->texFrames.numFrames * sizeof(RpPrtAdvEmtPrtTexCoordsItem);

        i++;
    }

    if (dataFlag & rpPRTADVEMITTERDATAFLAGMULTITEXCOORDSSTEP)
    {
        RWASSERT((dataFlag & rpPRTSTDEMITTERDATAFLAGPRTTEXCOORDS));
        RWASSERT(0 == (dataFlag & rpPRTADVEMITTERDATAFLAGMULTITEXCOORDS));
        RWASSERT(param);
        RWASSERT(param->texFrames.numFrames > 0);

        propID[i] = rpPRTADVPROPERTYCODEEMITTERPRTMULTITEXCOORDSSTEP;

        propSize[i] = sizeof(RpPrtAdvEmtPrtMultiTexCoords) +
                      param->texFrames.numFrames * sizeof(RpPrtAdvEmtPrtTexCoordsItem);

        i++;
    }

    if (dataFlag & rpPRTADVEMITTERDATAFLAGMULTISIZE)
    {
        RWASSERT((dataFlag & rpPRTSTDEMITTERDATAFLAGPRTSIZE));
        RWASSERT(param);
        RWASSERT(param->sizeFrames.numFrames > 0);

        propID[i] = rpPRTADVPROPERTYCODEEMITTERPRTMULTISIZE;

        propSize[i] = sizeof(RpPrtAdvEmtPrtMultiSize) +
                      param->sizeFrames.numFrames * sizeof(RpPrtAdvEmtPrtSizeItem);

        i++;
    }

    emitterType = (dataFlag & 0x00FF0000);

    switch( emitterType )
    {
        case 0: /* Default type (PrtStd Style) */
            break;
        case rpPRTADVEMITTERDATAFLAGPOINTLIST:
            {
                propID[i] = rpPRTADVPROPERTYCODEEMITTERPOINTLIST;
                propSize[i] =  sizeof(RpPrtAdvEmtPointList);

                RWASSERT(param);
                RWASSERT(param->pointList.numPoint > 0);

                propSize[i] += param->pointList.numPoint * sizeof(RwV3d);

                if( TRUE == param->pointList.haveDirection )
                {
                    propSize[i] += param->pointList.numPoint * sizeof(RwV3d);
                }

                i++;
            }
            break;
        case rpPRTADVEMITTERDATAFLAGCIRCLE:
            {
                propID[i] = rpPRTADVPROPERTYCODEEMITTERCIRCLE;
                propSize[i] = sizeof(RpPrtAdvEmtCircle);
                i++;
            }
            break;
        case rpPRTADVEMITTERDATAFLAGSPHERE:
            {
                propID[i] = rpPRTADVPROPERTYCODEEMITTERSPHERE;
                propSize[i] = sizeof(RpPrtAdvEmtSphere);
                i++;
            }
            break;
        default:
            RWMESSAGE((RWSTRING("Unsupported Emitter Type")));
            break;
    }


    RWRETURN(i);


}

/**
 * \ingroup rpprtadvmanage
 * \ref RpPrtAdvEClassStdCreate is a utility function to create
 * an emitter class containing a set of standard and advanced properties and
 * callbacks.
 *
 * This function support all the standard particles feature as well as the
 * advanced ones, and it is advised, when using both plugins, to always use
 * \ref RpPrtAdvEClassStdCreate.
 *
 * The \e dataFlag is used to set the properties of the emitter class. Not all
 * properties are necessary as some are optional.
 *
 * Emitters with user's custom properties should use
 * \ref RpPrtStdEClassCreate.
 *
 * The possible settings for the data flags are described below, where each
 * setting represents a property that can be present. Any one of these flags
 * can be used to create an emitter class:
 *
 * \par
 * Standard properties :
 * \verbatim
        rpPRTSTDEMITTERDATAFLAGSTANDARD
        rpPRTSTDEMITTERDATAFLAGPRTCOLOR
        rpPRTSTDEMITTERDATAFLAGPRTTEXCOORDS
        rpPRTSTDEMITTERDATAFLAGPRTSIZE
        rpPRTSTDEMITTERDATAFLAGPTANK\endverbatim
 *
 * Advanced properties :
 * \verbatim
        rpPRTADVEMITTERDATAFLAGPRTEMITTER
        rpPRTADVEMITTERDATAFLAGMULTICOLOR
        rpPRTADVEMITTERDATAFLAGMULTITEXCOORDS
        rpPRTADVEMITTERDATAFLAGMULTITEXCOORDSSTEP
        rpPRTADVEMITTERDATAFLAGMULTISIZE\endverbatim
 *
 * An emitter type can be chosen as well :
 * \verbatim
        rpPRTADVEMITTERDATAFLAGPOINTLIST
        rpPRTADVEMITTERDATAFLAGCIRCLE
        rpPRTADVEMITTERDATAFLAGSPHERE\endverbatim
 * \note Only \e one emitter type can be selected per emitter class.
 *
 * \warning When using the following flags :
 * \li rpPRTADVEMITTERDATAFLAGMULTICOLOR
 * \li rpPRTADVEMITTERDATAFLAGMULTITEXCOORDS
 * \li rpPRTADVEMITTERDATAFLAGMULTITEXCOORDSSTEP
 * \li rpPRTADVEMITTERDATAFLAGMULTISIZE
 *
 * the corresponding standard flag needs to be set as well. For example to use
 * multi-color particles, the flags rpPRTADVEMITTERDATAFLAGMULTICOLOR and
 * rpPRTSTDEMITTERDATAFLAGPRTCOLOR also need to be set.
 *
 * The \e paramBlock parameter (\ref RpPrtAdvParamBlock) needs to be filled
 * accordingly as well:
 * \li The \e pointList members (\ref RpPrtAdvParamPointList) needs to be
 * filled with the number of points required and the use of directions vectors
 * when using rpPRTADVEMITTERDATAFLAGPOINTLIST.
 * \li The \e colorFrames member (\ref RpPrtAdvMultiPropParams) needs to be
 * filled with the number of frames required when using
 * rpPRTADVEMITTERDATAFLAGMULTICOLOR.
 * \li The \e sizeFrames member (\ref RpPrtAdvMultiPropParams) need to be
 * filled with the number of frames required when using
 * rpPRTADVEMITTERDATAFLAGMULTISIZE.
 * \li The \e texFrames member (\ref RpPrtAdvMultiPropParams) need to be
 * filled with the number of frames required when using
 * rpPRTADVEMITTERDATAFLAGMULTITEXCOORDS and
 * rpPRTADVEMITTERDATAFLAGMULTITEXCOORDSSTEP.
 *
 * Please note that when using the multi-property emitter, the total number
 * of frames will be the number of frame passed by the paramblock parameter
 * \e plus \e 2, as the begin and end value from the standard properties are
 * used as well. For example, to have \e four frames of color animation, the
 * emitter will contain the begin and end values and \e two frames of color
 * animation.
 *
 * \param dataFlag      A bitfield representing the properties to
 *                      be added to the emitter class.
 * \param paramBlock    A pointer to a \ref RpPrtAdvParamBlock, needed only
 *                      when using multi-properties or the point list
 *                      emitter, as it contains the values used for the
 *                      calculation of the memory size needed for those
 *                      properties.
 *
 * \return Pointer to the RpPrtStdEmitterClass if successful, NULL otherwise.
 *
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdPClassCreate
 */
RpPrtStdEmitterClass *
RpPrtAdvEClassStdCreate( RwInt32 dataFlag, RpPrtAdvParamBlock *paramBlock)
{
    RpPrtStdEmitterClass                *eClass;
    RpPrtStdPropertyTable               *propTab;
    RwInt32                             i, id;
    RwInt32                             propID[rpPRTADVEMITTERPROPERTYCOUNT];
    RwInt32                             propSize[rpPRTADVEMITTERPROPERTYCOUNT];
    RwBool                              pTabNew;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEClassStdCreate"));

    id = RpPrtAdvEClassStdCreateID(dataFlag);

    eClass = NULL;

    i = _rpPrtAdvEClassStdFillPropTab(dataFlag, propID, propSize, paramBlock);

    RWASSERT(i > 0);

    pTabNew = FALSE;
    propTab = RpPrtStdPropTabGetByProperties(i, propID, propSize);

    if (propTab == NULL)
    {
        pTabNew = TRUE;
        propTab = RpPrtStdPropTabCreate(i, propID, propSize);
    }

    if (propTab)
    {
        eClass = RpPrtStdEClassGetByPropTab(propTab);

        if (eClass == NULL)
        {
            eClass = RpPrtStdEClassCreate();

            if (eClass)
            {
                RpPrtStdEClassSetPropTab(eClass, propTab);

                eClass->id = id;

                /* Set up the callbacks. */
                RpPrtAdvEClassStdSetupCB(eClass);

                /* Destroy the prop tab. The emitter class now owns
                 * the property table and will destroy it when it is
                 * destroyed.
                 */
                if (pTabNew)
                {
                    RpPrtStdPropTabDestroy(propTab);
                }
            }
        }
        else
        {
            eClass->refCount++;
        }
    }
    else
    {
        /* Memory failure. */
    }

    RWRETURN(eClass);
}

/**
 * \ingroup rpprtadvmanage
 * \ref RpPrtAdvEClassStdCreateID is a utility function to create
 * an emitter class ID.
 *
 * The \e dataFlag is used to create a unique ID.
 *
 * \note The ID range [0x00000000] to [0x00FFFFFF] is used internally.
 * Using an ID in this range may result in crash or unpredictable behavior.
 *
 * \param dataFlag      A bitfield representing the standard or advanced
 *                      properties to be added to the emitter class.
 *
 * \return The generated unique ID.
 *
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdPClassCreate
 */
RwInt32
RpPrtAdvEClassStdCreateID( RwInt32 dataFlag )
{
    RWAPIFUNCTION(RWSTRING("RpPrtAdvEClassStdCreateID"));

    RWRETURN(dataFlag);
}


static RwInt32
_rpPrtAdvPClassStdFillPropTab( RwInt32 dataFlag,
                                RwInt32 *propID,
                                RwInt32 *propSize,
                                RpPrtAdvParamBlock *param )
{
    RwInt32 i;
    RWFUNCTION(RWSTRING("_rpPrtAdvPClassStdFillPropTab"));

    RWASSERT(dataFlag);
    RWASSERT(propID);
    RWASSERT(propSize);

    i=0;

    propID[i] = rpPRTSTDPROPERTYCODEPARTICLESTANDARD;
    propSize[i] = sizeof(RpPrtStdParticleStandard);
    i++;

    if (dataFlag & rpPRTSTDPARTICLEDATAFLAGVELOCITY)
    {
        propID[i] = rpPRTSTDPROPERTYCODEPARTICLEVELOCITY;
        propSize[i] = sizeof(RwV3d);
        i++;
    }

    if (dataFlag & rpPRTSTDPARTICLEDATAFLAGPOSITION)
    {
        propID[i] = rpPRTSTDPROPERTYCODEPARTICLEPOSITION;
        propSize[i] = sizeof(RwV3d);
        i++;
    }

    if (dataFlag & rpPRTSTDPARTICLEDATAFLAGCOLOR)
    {
        propID[i] = rpPRTSTDPROPERTYCODEPARTICLECOLOR;
        propSize[i] = sizeof(RpPrtStdParticleColor);
        i++;
    }

    if (dataFlag & rpPRTSTDPARTICLEDATAFLAGTEXCOORDS)
    {
        propID[i] = rpPRTSTDPROPERTYCODEPARTICLETEXCOORDS;
        propSize[i] = sizeof(RpPrtStdParticleTexCoords);
        i++;
    }

    if (dataFlag &  rpPRTSTDPARTICLEDATAFLAGMATRIX)
    {
        propID[i] = rpPRTSTDPROPERTYCODEPARTICLEMATRIX;
        propSize[i] = sizeof(RwMatrix);
        i++;
    }

    if (dataFlag &  rpPRTSTDPARTICLEDATAFLAGSIZE)
    {
        propID[i] = rpPRTSTDPROPERTYCODEPARTICLESIZE;
        propSize[i] = sizeof(RpPrtStdParticleSize);
        i++;
    }

    if (dataFlag &  rpPRTSTDPARTICLEDATAFLAG2DROTATE)
    {
        propID[i] = rpPRTSTDPROPERTYCODEPARTICLE2DROTATE;
        propSize[i] = sizeof(RpPrtStdParticle2DRotate);
        i++;
    }

    if (dataFlag &  rpPRTADVPARTICLEDATAFLAGPRTEMITTER)
    {
        propID[i] = rpPRTADVPROPERTYCODEPARTICLEEMITTER;
        propSize[i] = sizeof(RpPrtAdvPrtEmitter);
        i++;
    }

    if (dataFlag & rpPRTADVPARTICLEDATAFLAGMULTICOLOR)
    {
        RWASSERT((dataFlag & rpPRTSTDPARTICLEDATAFLAGCOLOR));
        RWASSERT(param);
        RWASSERT(param->colorFrames.numFrames > 0);

        propID[i] = rpPRTADVPROPERTYCODEPARTICLEMULTICOLOR;
        propSize[i] = RPPRTADVPROPERTYSIZEPARTICLEMULTICOLOR(param->colorFrames.numFrames);

        i++;
    }

    if (dataFlag & rpPRTADVPARTICLEDATAFLAGMULTITEXCOORDS)
    {
        RWASSERT((dataFlag & rpPRTSTDPARTICLEDATAFLAGTEXCOORDS));
        RWASSERT(0 == (dataFlag & rpPRTADVPARTICLEDATAFLAGMULTITEXCOORDSSTEP));
        RWASSERT(param);
        RWASSERT(param->texFrames.numFrames > 0);

        propID[i] = rpPRTADVPROPERTYCODEPARTICLEMULTITEXCOORDS;
        propSize[i] = RPPRTADVPROPERTYSIZEPARTICLEMULTITEXCOORDS(param->texFrames.numFrames);

        i++;
    }

    if (dataFlag & rpPRTADVPARTICLEDATAFLAGMULTITEXCOORDSSTEP)
    {
        RWASSERT((dataFlag & rpPRTSTDPARTICLEDATAFLAGTEXCOORDS));
        RWASSERT(0 == (dataFlag & rpPRTADVPARTICLEDATAFLAGMULTITEXCOORDS));
        RWASSERT(param);
        RWASSERT(param->texFrames.numFrames > 0);

        propID[i] = rpPRTADVPROPERTYCODEPARTICLEMULTITEXCOORDSSTEP;
        propSize[i] = RPPRTADVPROPERTYSIZEPARTICLEMULTITEXSTEP(param->texFrames.numFrames);

        i++;
    }

    if (dataFlag & rpPRTADVPARTICLEDATAFLAGMULTISIZE)
    {
        RWASSERT((dataFlag & rpPRTSTDPARTICLEDATAFLAGSIZE));
        RWASSERT(param);
        RWASSERT(param->sizeFrames.numFrames > 0);

        propID[i] = rpPRTADVPROPERTYCODEPARTICLEMULTISIZE;
        propSize[i] = RPPRTADVPROPERTYSIZEPARTICLEMULTISIZE(param->sizeFrames.numFrames);

        i++;
    }

    RWRETURN(i);


}

/**
 * \ingroup rpprtadvmanage
 * \ref RpPrtAdvPClassStdCreate is a utility function to create
 * a particle class containing a set of standard and advanced properties and
 * callbacks.
 *
 * This function supports all the standard particle features as well as the
 * advanced ones, and it is advised, when using both plugins, to always use
 * \ref RpPrtAdvPClassStdCreate.
 *
 * The \e dataFlag is used to set the properties of the particle class. Not all
 * properties are necessary as some are optional.
 *
 * Particles with user's custom properties should use
 * \ref RpPrtStdPClassCreate.
 *
 * The possible settings for the data flags are described below, where each
 * setting represents a property that can be present. Any one of these flags
 * can be used to create a particle class:
 *
 * \par
 * Standard properties :
 * \verbatim
        rpPRTSTDPARTICLEDATAFLAGSTANDARD
        rpPRTSTDPARTICLEDATAFLAGCOLOR
        rpPRTSTDPARTICLEDATAFLAGTEXCOORDS
        rpPRTSTDPARTICLEDATAFLAGSIZE
        rpPRTSTDPARTICLEDATAFLAGVELOCITY \endverbatim
 *
 * Advanced properties :
 * \verbatim
        rpPRTADVPARTICLEDATAFLAGPRTEMITTER
        rpPRTADVPARTICLEDATAFLAGMULTICOLOR
        rpPRTADVPARTICLEDATAFLAGMULTITEXCOORDS
        rpPRTADVPARTICLEDATAFLAGMULTITEXCOORDSSTEP
        rpPRTADVPARTICLEDATAFLAGMULTISIZE \endverbatim
 *
 *
 * \warning When using the following flags :
 * \li rpPRTADVPARTICLEDATAFLAGMULTICOLOR
 * \li rpPRTADVPARTICLEDATAFLAGMULTITEXCOORDS
 * \li rpPRTADVPARTICLEDATAFLAGMULTITEXCOORDSSTEP
 * \li rpPRTADVPARTICLEDATAFLAGMULTISIZE
 *
 * the corresponding standard flags needs to be set as well. For example
 * to use multi-color particles, the flags rpPRTADVPARTICLEDATAFLAGMULTICOLOR
 * and rpPRTSTDPARTICLEDATAFLAGCOLOR also need to be set.
 *
 * The \e paramBlock parameter (\ref RpPrtAdvParamBlock) needs to be filled
 * accordingly as well:
 * \li The \e colorFrames member (\ref RpPrtAdvMultiPropParams) needs to be
 * filled with the number of frames required when using
 * rpPRTADVPARTICLEDATAFLAGMULTICOLOR.
 * \li The \e sizeFrames member (\ref RpPrtAdvMultiPropParams) needs to be
 * filled with the number of frames required when using
 * rpPRTADVPARTICLEDATAFLAGMULTISIZE.
 * \li The \e texFrames member (\ref RpPrtAdvMultiPropParams) needs to be
 * filled with the number of frames required when using
 * rpPRTADVPARTICLEDATAFLAGMULTITEXCOORDS and
 * rpPRTADVPARTICLEDATAFLAGMULTITEXCOORDSSTEP.
 *
 * Please note that when using multi-property particles, the total number of
 * frames will be the number of frames passed by the \e paramblock parameter
 * \e plus \e 2, as the begin and end value from the standard properties are
 * used as well. For example, to have \e four frames of color animation, the
 * particles will contain the begin and end values and \e two frames of color
 * animation.
 *
 * \param dataFlag      A bitfield representing the properties to
 *                      be added to the particle class.
 *
 * \param paramBlock    A pointer to a \ref RpPrtAdvParamBlock, needed only
 *                      when using multi-properties particles, as it contains the
 *                      values used for the calculation of the memory size
 *                      needed for those properties.
 *
 * \return Pointer to the RpPrtStdParticleClass if successful, NULL otherwise.
 *
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdPClassCreate
 */
RpPrtStdParticleClass *
RpPrtAdvPClassStdCreate( RwInt32 dataFlag, RpPrtAdvParamBlock *paramBlock )
{
    RpPrtStdParticleClass       *pClass;
    RpPrtStdPropertyTable       *propTab;
    RwInt32                     i, id;
    RwInt32                     propID[rpPRTADVPARTICLEPROPERTYCOUNT];
    RwInt32                     propSize[rpPRTADVPARTICLEPROPERTYCOUNT];
    RwBool                      pTabNew;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvPClassStdCreate"));

    id = RpPrtAdvPClassStdCreateID(dataFlag);

    pClass = NULL;

    i = _rpPrtAdvPClassStdFillPropTab(dataFlag, propID, propSize, paramBlock);

    RWASSERT(i > 0);

    pTabNew = FALSE;
    propTab = RpPrtStdPropTabGetByProperties(i, propID, propSize);

    if (propTab == NULL)
    {
        pTabNew = TRUE;
        propTab = RpPrtStdPropTabCreate(i, propID, propSize);
    }

    if (propTab)
    {
        pClass = RpPrtStdPClassGetByPropTab(propTab);

        if (pClass == NULL)
        {
            pClass = RpPrtStdPClassCreate();

            if (pClass)
            {
                RpPrtStdPClassSetPropTab(pClass, propTab);

                pClass->id = id;

                /* Set up the callbacks. */
                RpPrtAdvPClassStdSetupCB(pClass);

                /* Destroy the prop tab. The particle class now owns
                 * the property table and will destroy it when it is
                 * destroyed.
                 */
                if (pTabNew)
                {
                    RpPrtStdPropTabDestroy(propTab);
                }
            }
        }
        else
        {
            pClass->refCount++;
        }
    }
    else
    {
        /* Memory failure. */
    }

    RWRETURN(pClass);
}

/**
 * \ingroup rpprtadvmanage
 * \ref RpPrtAdvPClassStdCreateID is a utility function to create
 * a particle class ID.
 *
 * The \e dataFlag is used to create a unique ID.
 *
 * \note The ID range [0x00000000] to [0x00FFFFFF] is used internally.
 * Using an ID in this range may result in crash or unpredictable behavior.
 *
 * \param dataFlag      A bitfield representing the standard or advanced
 *                      properties to be added to the particle class.
 *
 * \return The generated unique ID.
 *
 * \see RpPrtStdEClassCreate
 * \see RpPrtStdPClassCreate
 */
RwInt32
RpPrtAdvPClassStdCreateID( RwInt32 dataFlag )
{
    RWAPIFUNCTION(RWSTRING("RpPrtAdvPClassStdCreateID"));

    RWRETURN(dataFlag);
}

/**
 * \ingroup rpprtadvmanage
 * \ref RpPrtAdvEClassStdSetupCB is the callback function to
 * restore the emitter class callback functions for advanced emitter
 * properties.
 *
 * This function is called after an emitter class is streamed in or after it
 * is created.
 *
 * This callback should be used if the emitter class contains any of the
 * standard or advanced emitter properties.
 *
 * \param eClass        A pointer to an RpPrtStdEmitterClass for restoring
 *                      the callbacks.
 *
 * \return Pointer to the RpPrtStdEmitterClass if successful, NULL otherwise.
 *
 * \see RpPrtStdGetEClassSetupCallBack
 * \see RpPrtStdSetEClassSetupCallBack
 */
RpPrtStdEmitterClass *
RpPrtAdvEClassStdSetupCB(RpPrtStdEmitterClass *eClass)
{
    RwInt32                             i,j,numSets;
    RwInt32                             emitterType;
    RpPrtStdEmitterCallBackArray        finalCB[8];
    RwInt32                             IndexCB[rpPRTSTDEMITTERCALLBACKMAX];

    RpPrtStdEmitterCallBackArray        beforeCB[4];
    RpPrtStdEmitterCallBackArray        afterCB[4];
    RpPrtStdEmitterCallBackArray        standardCB;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvEClassStdSetupCB"));

    RWASSERT( eClass->id );
    RWASSERT( 0 == (eClass->id & 0xFF000000) );

    memset(finalCB,0x0,sizeof(RpPrtStdEmitterCallBackArray)*8);
    memset(beforeCB,0x0,sizeof(RpPrtStdEmitterCallBackArray)*4);
    memset(afterCB,0x0,sizeof(RpPrtStdEmitterCallBackArray)*4);
    memset(standardCB,0x0,sizeof(RpPrtStdEmitterCallBackArray));

    numSets = 1;

    emitterType = (eClass->id & 0x00FF0000);

    switch( emitterType )
    {
        case 0: /* Default type (PrtStd Style) */
            break;
        case rpPRTADVEMITTERDATAFLAGPOINTLIST:
            {
                afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKEMIT] = RpPrtAdvEmtPtListEmitCB;
                afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKSTREAMREAD] = RpPrtAdvEmtPtListStreamReadCB;
                afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKSTREAMWRITE] = RpPrtAdvEmtPtListStreamWriteCB;
                afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKSTREAMGETSIZE] = RpPrtAdvEmtPtListStreamGetSizeCB;
                afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKCREATE] = RpPrtAdvEmtPtListCreateCB;
                afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKCLONE] = RpPrtAdvEmtPtListCloneCB;

                numSets++;
            }
            break;
        case rpPRTADVEMITTERDATAFLAGCIRCLE:
            {
                afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKEMIT] = RpPrtAdvEmtCircleEmitCB;
                afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKSTREAMREAD] = RpPrtAdvEmtCircleStreamReadCB;
                afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKSTREAMWRITE] = RpPrtAdvEmtCircleStreamWriteCB;
                afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKSTREAMGETSIZE] = RpPrtAdvEmtCircleStreamGetSizeCB;
                afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKCLONE] = RpPrtAdvEmtCircleCloneCB;
                numSets++;
            }
            break;
        case rpPRTADVEMITTERDATAFLAGSPHERE:
            {
                afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKEMIT] = RpPrtAdvEmtSphereEmitCB;
                afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKSTREAMREAD] = RpPrtAdvEmtSphereStreamReadCB;
                afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKSTREAMWRITE] = RpPrtAdvEmtSphereStreamWriteCB;
                afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKSTREAMGETSIZE] = RpPrtAdvEmtSphereStreamGetSizeCB;
                afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKCLONE] = RpPrtAdvEmtSphereCloneCB;

                numSets++;
            }
            break;
        default:
            RWMESSAGE((RWSTRING("Unsupported Emitter Type")));
            break;
    }

    standardCB[rpPRTSTDEMITTERCALLBACKCREATE] = RpPrtStdEmitterStdCreateCB;
    standardCB[rpPRTSTDEMITTERCALLBACKDESTROY] = RpPrtStdEmitterStdDestroyCB;

#if (defined(SKY2_DRVMODEL_H) )
    standardCB[rpPRTSTDEMITTERCALLBACKEMIT] = RpPrtStdEmitterStdEmitSkyCB;
#else
    standardCB[rpPRTSTDEMITTERCALLBACKEMIT] = RpPrtStdEmitterStdEmitCB;
#endif

    standardCB[rpPRTSTDEMITTERCALLBACKBEGINUPDATE] = RpPrtStdEmitterStdBeginUpdateCB;
    standardCB[rpPRTSTDEMITTERCALLBACKENDUPDATE] = RpPrtStdEmitterStdEndUpdateCB;
    standardCB[rpPRTSTDEMITTERCALLBACKBEGINRENDER] = RpPrtStdEmitterStdRenderCB;
    standardCB[rpPRTSTDEMITTERCALLBACKSTREAMREAD] = RpPrtStdEmitterStdStreamReadCB;
    standardCB[rpPRTSTDEMITTERCALLBACKSTREAMWRITE] = RpPrtStdEmitterStdStreamWriteCB;
    standardCB[rpPRTSTDEMITTERCALLBACKSTREAMGETSIZE] = RpPrtStdEmitterStdStreamGetSizeCB;
    standardCB[rpPRTSTDEMITTERCALLBACKCLONE] = RpPrtStdEmitterStdCloneCB;

    if (eClass->id & rpPRTADVEMITTERDATAFLAGPRTEMITTER)
    {
        afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKEMIT] = RpPrtAdvEmtPrtEmtEmitCB;
        afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKSTREAMREAD] = RpPrtAdvEmtPrtEmtStreamReadCB;
        afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKSTREAMWRITE] = RpPrtAdvEmtPrtEmtStreamWriteCB;
        afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKSTREAMGETSIZE] = RpPrtAdvEmtPrtEmtStreamGetSizeCB;
        afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKCLONE] = RpPrtAdvEmtPrtEmtCloneCB;

        numSets++;
    }

    if ((eClass->id & rpPRTADVEMITTERDATAFLAGMULTICOLOR) ||
        (eClass->id & rpPRTADVEMITTERDATAFLAGMULTITEXCOORDS) ||
        (eClass->id & rpPRTADVEMITTERDATAFLAGMULTISIZE) )
    {
        afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKEMIT] = RpPrtAdvEmtMultiPropEmitCB;
        afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKSTREAMREAD] = RpPrtAdvEmtMultiPropStreamReadCB;
        afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKSTREAMWRITE] = RpPrtAdvEmtMultiPropStreamWriteCB;
        afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKSTREAMGETSIZE] = RpPrtAdvEmtMultiPropStreamGetSizeCB;
        afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKCREATE] = RpPrtAdvEmtMultiPropCreateCB;
        afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKCLONE] = RpPrtAdvEmtMultiPropCloneCB;

        numSets++;
    }

    if ((eClass->id & rpPRTADVEMITTERDATAFLAGMULTITEXCOORDSSTEP))
    {
        afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKEMIT] = RpPrtAdvEmtTexStepEmitCB;
        afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKSTREAMREAD] = RpPrtAdvEmtTexStepStreamReadCB;
        afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKSTREAMWRITE] = RpPrtAdvEmtTexStepStreamWriteCB;
        afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKSTREAMGETSIZE] = RpPrtAdvEmtTexStepStreamGetSizeCB;
        afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKCREATE] = RpPrtAdvEmtTexStepCreateCB;
        afterCB[numSets-1][rpPRTSTDEMITTERCALLBACKCLONE] = RpPrtAdvEmtTexStepCloneCB;

        numSets++;
    }

    /* RpPrtAdvEClassStdSetupCB */

    /* set the before */
    memset(IndexCB,0,sizeof(RwInt32)*rpPRTSTDEMITTERCALLBACKMAX);

    for (i = 0; i < rpPRTSTDEMITTERCALLBACKMAX; i++)
    {
        for(j=0;j<4;j++)
        {
            if( NULL != beforeCB[j][i] )
            {
                finalCB[IndexCB[i]][i] = beforeCB[j][i];
                IndexCB[i]++;
            }
        }

        /* set the standard */
        finalCB[IndexCB[i]][i] = standardCB[i];
        IndexCB[i]++;

        /* set the afters */
        for(j=0;j<4;j++)
        {
            if( NULL != afterCB[j][i] )
            {
                finalCB[IndexCB[i]][i] = afterCB[j][i];
                IndexCB[i]++;
            }
        }
    }

    RpPrtStdEClassSetCallBack(eClass, numSets, finalCB);

    RWRETURN(eClass);
}

/**
 * \ingroup rpprtadvmanage
 * \ref RpPrtAdvPClassStdSetupCB is the callback function to
 * restore the particle class callback functions for advanced particle
 * properties.
 *
 * This function is called after a particle class is streamed in or after it
 * is created.
 *
 * This callback should be used if the particle class contains any of the
 * standard or advanced particles properties.
 *
 * \param pClass        A pointer to a RpPrtStdParticleClass for restoring
 *                      the callbacks.
 *
 * \return Pointer to the RpPrtStdParticleClass if successful, NULL otherwise.
 *
 * \see RpPrtStdGetPClassSetupCallBack
 * \see RpPrtStdSetPClassSetupCallBack
 */
RpPrtStdParticleClass *
RpPrtAdvPClassStdSetupCB(RpPrtStdParticleClass *pClass)
{
    RwInt32                             i,j,numSets;
    RwInt32                             particleType;
    RpPrtStdParticleCallBackArray       finalCB[8];
    RwInt32                             IndexCB[8];

    RpPrtStdParticleCallBackArray        beforeCB[4];
    RpPrtStdParticleCallBackArray        afterCB[4];
    RpPrtStdParticleCallBackArray        standardCB;

    RWAPIFUNCTION(RWSTRING("RpPrtAdvPClassStdSetupCB"));

    RWASSERT( pClass->id );
    RWASSERT( 0 == (pClass->id & 0xFF000000) );

    memset(finalCB, 0x0, sizeof(RpPrtStdParticleCallBackArray) * 8);
    memset(beforeCB, 0x0, sizeof(RpPrtStdParticleCallBackArray) * 4);
    memset(afterCB, 0x0, sizeof(RpPrtStdParticleCallBackArray) * 4);
    memset(standardCB, 0x0, sizeof(RpPrtStdParticleCallBackArray));

    numSets = 1;

    particleType = (pClass->id & 0x00FF0000);

    switch( particleType )
    {
        case 0: /* Default type (PrtStd Style) */
            break;
        default:
            RWMESSAGE((RWSTRING("Unsupported Particle Type")));
            break;
    }

#if (defined(SKY2_DRVMODEL_H) )
    standardCB[rpPRTSTDPARTICLECALLBACKUPDATE] = RpPrtStdParticleStdUpdateSkyCB;
#else
    standardCB[rpPRTSTDPARTICLECALLBACKUPDATE] = RpPrtStdParticleStdUpdateCB;
#endif

    /* set the before */
    if (pClass->id & rpPRTADVEMITTERDATAFLAGPRTEMITTER)
    {
        beforeCB[numSets-1][rpPRTSTDPARTICLECALLBACKUPDATE] = RpPrtAdvPrtEmtUpdateCB;
        numSets++;
    }

    if ((pClass->id & rpPRTADVPARTICLEDATAFLAGMULTICOLOR) ||
        (pClass->id & rpPRTADVPARTICLEDATAFLAGMULTITEXCOORDS) ||
        (pClass->id & rpPRTADVPARTICLEDATAFLAGMULTISIZE) )
    {
        beforeCB[numSets-1][rpPRTSTDPARTICLECALLBACKUPDATE] = RpPrtAdvPrtMultiPropUpdateCB;
        numSets++;
    }

    if ((pClass->id & rpPRTADVPARTICLEDATAFLAGMULTITEXCOORDSSTEP))
    {
        beforeCB[numSets-1][rpPRTSTDPARTICLECALLBACKUPDATE] = RpPrtAdvPrtTexStepUpdateCB;
        numSets++;
    }

    memset(IndexCB, 0, sizeof(RwInt32) * 8);

    for (i = 0; i < rpPRTSTDPARTICLECALLBACKMAX; i++)
    {
        for(j=0;j<4;j++)
        {
            if( NULL != beforeCB[j][i] )
            {
                finalCB[IndexCB[i]][i] = beforeCB[j][i];
                IndexCB[i]++;
            }
        }

        /* set the standard */
        finalCB[IndexCB[i]][i] = standardCB[i];
        IndexCB[i]++;

        /* set the afters */
        for(j=0;j<4;j++)
        {
            if( NULL != afterCB[j][i] )
            {
                finalCB[IndexCB[i]][i] = afterCB[j][i];
                IndexCB[i]++;
            }
        }
    }

    RpPrtStdPClassSetCallBack(pClass, numSets, finalCB);

    RWRETURN(pClass);
}

/************************************************************************/


/************************************************************************/

/************************************************************************/


/************************************************************************/


/************************************************************************/


