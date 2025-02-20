/*
 * Converting no hs worlds to real binary worlds (with bsps).
 * No HS worlds are used in the generation process of worlds
 *
 * Copyright (c) 1998 Criterion Software Ltd.
 */

/****************************************************************************
 Includes
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "rwcore.h"
#include "rpworld.h"

#include "rpdbgerr.h"

#include "nhsworld.h"
#include "nhsstats.h"
#include "nhsutil.h"

typedef struct RtBoundaryState RtBoundaryState;
struct RtBoundaryState
{
    RtWorldImportBuildVertex *boundaries;

    /* vertex stats for current polygon */
    RwInt32             clip;
    RwInt32             overlap;
    RwReal              dist, distLeft, distRight;

    /* count usage of each material */
    RwUInt32           *matl, *matr, *matb;

    /* running polygon total */
    RtWorldImportBuildClipStatistics stats;
};

#define InitBuildSectorGetClipStatistics(_state)                            \
MACRO_START                                                                 \
{                                                                           \
    (_state)->clip =  rwCLIPVERTEXLEFT | rwCLIPVERTEXRIGHT;                 \
    (_state)->overlap =  rwCLIPVERTEXLEFT | rwCLIPVERTEXRIGHT;              \
    (_state)->distLeft = (_state)->distRight = 0.0f;                        \
}                                                                           \
MACRO_STOP

#define VertexBuildSectorGetClipStatistics(_state, _vpVert, _plane, _value) \
MACRO_START                                                                 \
{                                                                           \
    (_state)->dist =  GETCOORD((_vpVert)->OC,  (_plane)) - (_value);        \
    (_state)->clip &= (_vpVert)->state.clipFlags[0];                        \
    (_state)->overlap &= (_vpVert)->state.clipFlags[1];                     \
    if ((_state)->dist < 0.0f)                                              \
    {                                                                       \
        if (-(_state)->dist > (_state)->distLeft)                           \
        {                                                                   \
            (_state)->distLeft = -(_state)->dist;                           \
        }                                                                   \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        if ((_state)->dist > (_state)->distRight)                           \
        {                                                                   \
            (_state)->distRight = (_state)->dist;                           \
        }                                                                   \
    }                                                                       \
}                                                                           \
MACRO_STOP

#define TerminatorBuildSectorGetClipStatistics(_state)                      \
MACRO_START                                                                 \
{                                                                           \
    /* figure stats for this boundary not considering the maximum overlap*/ \
    switch ((_state)->clip & (rwCLIPVERTEXLEFT | rwCLIPVERTEXRIGHT))        \
    {                                                                       \
        case rwCLIPVERTEXLEFT:                                              \
            {                                                               \
                (_state)->stats.numPotentialLeft++;                           \
                break;                                                      \
            }                                                               \
        case rwCLIPVERTEXLEFT | rwCLIPVERTEXRIGHT:                          \
        case rwCLIPVERTEXRIGHT:                                             \
            {                                                               \
                (_state)->stats.numPotentialRight++;                          \
                break;                                                      \
            }                                                               \
        case 0:                                                             \
            {                                                               \
                (_state)->stats.numPotentialSplit++;                          \
                                                                            \
                if ((_state)->distRight > (_state)->distLeft)               \
                {                                                           \
                    (_state)->stats.numPotentialRight++;                      \
                    if ((_state)->distLeft > (_state)->stats.overlapRight)  \
                    {                                                       \
                        (_state)->stats.overlapRight = (_state)->distLeft;  \
                    }                                                       \
                }                                                           \
                else                                                        \
                {                                                           \
                    (_state)->stats.numPotentialLeft++;                       \
                    if ((_state)->distRight > (_state)->stats.overlapLeft)  \
                    {                                                       \
                        (_state)->stats.overlapLeft = (_state)->distRight;  \
                    }                                                       \
                }                                                           \
                                                                            \
                break;                                                      \
            }                                                               \
    }                                                                       \
    (_state)->matb[(_state)->boundaries->pinfo.matIndex]++;                 \
    /* figure stats for this boundary considering the maximum overlap*/     \
    switch ((_state)->overlap & (rwCLIPVERTEXLEFT | rwCLIPVERTEXRIGHT))     \
    {                                                                       \
        case rwCLIPVERTEXLEFT:                                              \
            {                                                               \
                /* Definately left */                                       \
                (_state)->stats.numActualLeft++;                              \
                (_state)->matl[(_state)->boundaries->pinfo.matIndex]++;     \
                break;                                                      \
            }                                                               \
        case rwCLIPVERTEXRIGHT:                                             \
            {                                                               \
                /* Definately right */                                      \
                (_state)->stats.numActualRight++;                             \
                (_state)->matr[(_state)->boundaries->pinfo.matIndex]++;     \
                break;                                                      \
            }                                                               \
        case rwCLIPVERTEXLEFT | rwCLIPVERTEXRIGHT:                          \
            {                                                               \
                /* straddles partition, but inside overlap */               \
                if ((_state)->distRight >= (_state)->distLeft)              \
                {                                                           \
                    (_state)->stats.numActualRight++;                         \
                    (_state)->matr[(_state)->boundaries->pinfo.matIndex]++; \
                }                                                           \
                else                                                        \
                {                                                           \
                    (_state)->stats.numActualLeft++;                          \
                    (_state)->matl[(_state)->boundaries->pinfo.matIndex]++; \
                }                                                           \
                break;                                                      \
            }                                                               \
        case 0:                                                             \
            {                                                               \
                /* Definately split, it overlaps the overlap */             \
                (_state)->stats.numActualSplit++;                             \
                (_state)->matr[(_state)->boundaries->pinfo.matIndex]++;     \
                (_state)->matl[(_state)->boundaries->pinfo.matIndex]++;     \
                break;                                                      \
            }                                                               \
    }                                                                       \
}                                                                           \
MACRO_STOP

/****************************************************************************
 _rtBuildSectorGetDistStatistics

 On entry   : Build sector
            : Overlap(OUT)
            : Alphadist(OUT)
 */
/**
 * \ingroup rtworldimport
 * \ref RtWorldImportSetPartitionStatistics is used to set the stats for the
 * given partition. This must be called before using any of the statistics of
 * a partitioner.
 *
 * \param buildSector  The current build sector.
 * \param partition  Candidate or chosen partition.
 */
void
RtWorldImportSetPartitionStatistics(RtWorldImportBuildSector * buildSector,
                                      RtWorldImportPartition * partition)
{
    RwInt32             numBoundaries;
    RwInt32             nJ;
    RtBoundaryState     state;
    RtWorldImportParameters *conversionParams = RtWorldImportParametersGet();

    RWAPIFUNCTION(RWSTRING("RtWorldImportSetPartitionStatistics"));
    RWASSERT(buildSector);
    RWASSERT(partition);

    /* Set the clip codes */
    BuildSectorSetOverlapsMacro(buildSector->boundingBox, *partition, *conversionParams);
    BuildSectorSetWorldClipCodes(buildSector, partition->type, partition->value,
        partition->maxLeftValue, partition->maxRightValue);


    state.stats.overlapLeft = 0.0f;
    state.stats.overlapRight = 0.0f;
    state.stats.numPotentialLeft = 0;
    state.stats.numPotentialRight = 0;
    state.stats.numPotentialSplit = 0;
    state.stats.numActualSplit = 0;
    state.stats.numActualLeft = 0;
    state.stats.numActualRight = 0;
    state.stats.numMaterialLeft = 0;
    state.stats.numMaterialRight = 0;
    state.stats.numMaterialSplits = 0;
    state.stats.numMaterialSector = 0;

    state.matl = (RwUInt32 *) RwCalloc(sizeof(RwUInt32), buildSector->maxNumMaterials,
                                       rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_FUNCTION);
    state.matr = (RwUInt32 *) RwCalloc(sizeof(RwUInt32), buildSector->maxNumMaterials,
                                       rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_FUNCTION);
    state.matb = (RwUInt32 *) RwCalloc(sizeof(RwUInt32), buildSector->maxNumMaterials,
                                       rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_FUNCTION);

    numBoundaries = buildSector->numBoundaries;
    state.boundaries = buildSector->boundaries;

    InitBuildSectorGetClipStatistics(&state);

    for (nJ = 0; nJ < numBoundaries; nJ++)
    {

        RtWorldImportBuildVertexMode *const mode =
            &state.boundaries->mode;
        RtWorldImportVertex *const vpVert = mode->vpVert;

        if (!vpVert)
        {
            TerminatorBuildSectorGetClipStatistics(&state);
            InitBuildSectorGetClipStatistics(&state);
        }
        else
        {
            VertexBuildSectorGetClipStatistics(&state, vpVert,
                                               partition->type, partition->value);
        }
        state.boundaries++;
    }

    /* how many materials used? */

/* NOTE don't think we need to initalise these */
    partition->buildStats.numMaterialSplits = partition->buildStats.numMaterialSector = 0;
    partition->buildStats.numMaterialLeft = partition->buildStats.numMaterialRight = 0;
    for (nJ = 0; nJ < (RwInt32)buildSector->maxNumMaterials; nJ++)
    {
        if (state.matl[nJ])
            state.stats.numMaterialLeft++;
        if (state.matr[nJ])
            state.stats.numMaterialRight++;
        if ((state.matl[nJ]) && (state.matr[nJ]))
            state.stats.numMaterialSplits++;
        if (state.matb[nJ])
            state.stats.numMaterialSector++;
    }
    RwFree(state.matr);
    RwFree(state.matl);
    RwFree(state.matb);

    /* Copy the stats over */
    partition->buildStats.overlapLeft = state.stats.overlapLeft;
    partition->buildStats.overlapRight = state.stats.overlapRight;
    partition->buildStats.numPotentialLeft = state.stats.numPotentialLeft;
    partition->buildStats.numPotentialRight = state.stats.numPotentialRight;
    partition->buildStats.numPotentialSplit = state.stats.numPotentialSplit;
    partition->buildStats.numActualSplit = state.stats.numActualSplit;
    partition->buildStats.numActualLeft = state.stats.numActualLeft;
    partition->buildStats.numActualRight = state.stats.numActualRight;
    partition->buildStats.numMaterialLeft = state.stats.numMaterialLeft;
    partition->buildStats.numMaterialRight = state.stats.numMaterialRight;
    partition->buildStats.numMaterialSplits = state.stats.numMaterialSplits;
    partition->buildStats.numMaterialSector = state.stats.numMaterialSector;


    /* All done */

    RWRETURNVOID();
}
