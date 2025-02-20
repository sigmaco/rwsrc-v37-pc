#ifndef TEAMPIPES_H
#define TEAMPIPES_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpteam.h"

#include "teamtypes.h"

#if (defined(TEAMPIPEEXTENSIONS))

/*=======================================================================*
 *--- Private defines ---------------------------------------------------*
 *=======================================================================*/
#define TEAMPIPE_MAX 10
#define TEAMPIPENULL ((RwUInt32)~0)
#define TEAMPIPEHEAPFLUSH TRUE

/*=======================================================================*
 *--- Private Types -----------------------------------------------------*
 *=======================================================================*/
typedef void (* TeamPipeOpen) (void);
typedef void (* TeamPipeClose) (void);
typedef RwUInt32 TeamPipeIndex;

typedef struct TeamPipeDefinition TeamPipeDefinition;
struct TeamPipeDefinition
{
    RxPipeline    *render;
    TeamPipeOpen   open;
    TeamPipeClose  close;
};

/*=======================================================================*
 *--- Private Variables -------------------------------------------------*
 *=======================================================================*/
extern TeamPipeDefinition PipeData[TEAMPIPE_MAX];
extern TeamPipeDefinition *lastDefinition;

/*=======================================================================*
 *--- Private functions -------------------------------------------------*
 *=======================================================================*/
#define _rpTeamPipeGetPipeDefinition(pipeIndex)                      \
    &(PipeData[pipeIndex])

#define _rpTeamPipeOpenPipe(definition)                              \
    (definition->open)()

#define _rpTeamPipeClosePipe(definition)                             \
    (definition->close)()

#define _rpTeamPipeOpenGlobalPipe(definition)                        \
MACRO_START                                                          \
{                                                                    \
    TeamPipeDefinition *thisDefinition;                              \
                                                                     \
    thisDefinition = (TeamPipeDefinition *)definition;               \
    RWASSERT(NULL != thisDefinition->render);                        \
                                                                     \
    if(lastDefinition == NULL)                                       \
    {                                                                \
        /* Open the new one. */                                      \
        if(NULL != thisDefinition->open)                             \
        {                                                            \
            _rpTeamPipeOpenPipe(thisDefinition);                     \
        }                                                            \
    }                                                                \
    else if(lastDefinition->render != thisDefinition->render)        \
    {                                                                \
        /* Close the old pipe. */                                    \
        if(NULL != lastDefinition->close)                            \
        {                                                            \
            _rpTeamPipeClosePipe(lastDefinition);                    \
        }                                                            \
                                                                     \
        /* And open the new one. */                                  \
        if(NULL != thisDefinition->open)                             \
        {                                                            \
            _rpTeamPipeOpenPipe(thisDefinition);                     \
        }                                                            \
    }                                                                \
                                                                     \
    /* And render it. */                                             \
    lastDefinition = thisDefinition;                                 \
}                                                                    \
MACRO_STOP

#define _rpTeamPipeCloseGlobalPipe()                                 \
MACRO_START                                                          \
{                                                                    \
    /* Close the old one. */                                         \
    if( (NULL != lastDefinition) &&                                  \
        (NULL != lastDefinition->close) )                            \
    {                                                                \
        (lastDefinition->close)();                                   \
    }                                                                \
                                                                     \
    lastDefinition = (TeamPipeDefinition *)NULL;                     \
}                                                                    \
MACRO_STOP

#define _rpTeamPipeGetPipeline(pipeIndex)                            \
    ((RxPipeline *)(_rpTeamPipeGetPipeDefinition(pipeIndex)))

#define _rpTeamPipeRenderAtomic(atomic)                              \
    (RxPipelineExecute(                                              \
        ((TeamPipeDefinition *)(atomic->pipeline))->render,          \
        (void *)atomic, TEAMPIPEHEAPFLUSH))

extern void
_rpTeamPipeRemovePipeDefinition(TeamPipeIndex pipeIndex);

extern TeamPipeIndex
_rpTeamPipeAddPipeDefinition( RxPipeline *pipeline,
                              TeamPipeOpen openPipe,
                              TeamPipeClose closePipe );

#else /* (defined(TEAMPIPEEXTENSIONS)) */

/*=======================================================================*
 *--- Private defines ---------------------------------------------------*
 *=======================================================================*/
#define TEAMPIPENULL (NULL)

/*
 * Since the PS2 version of RpTeam does not use memory allocations on the
 * global heap, the parameter to reset the heap to RxPipelineExecute() can
 * be FALSE on the PS2 and it will save the call to RxHeapReset.
 * This saves probably 60-100 or so cycles per atomic rendered by RpTeam.
 *
 * On all other platforms this is defaulted to TRUE.
 */
#define TEAMPIPEHEAPFLUSH FALSE

/*=======================================================================*
 *--- Private Types -----------------------------------------------------*
 *=======================================================================*/
typedef RxPipeline * TeamPipeIndex;

/*=======================================================================*
 *--- Private Variables -------------------------------------------------*
 *=======================================================================*/

/*=======================================================================*
 *--- Private functions -------------------------------------------------*
 *=======================================================================*/
#define _rpTeamPipeGetPipeDefinition(pipeIndex)
#define _rpTeamPipeOpenPipe(definition)
#define _rpTeamPipeClosePipe(definition)
#define _rpTeamPipeOpenGlobalPipe(definition)
#define _rpTeamPipeCloseGlobalPipe()

#define _rpTeamPipeGetPipeline(pipeIndex)                            \
    (pipeIndex)

#define _rpTeamPipeRenderAtomic(atomic)                              \
    (RxPipelineExecute(atomic->pipeline, (void *)atomic, TEAMPIPEHEAPFLUSH))

#define _rpTeamPipeRemovePipeDefinition(pipeIndex)                   \
    RxPipelineDestroy(pipeIndex)

#define _rpTeamPipeAddPipeDefinition(pipeline, open, close)          \
    (pipeline)

#endif /* (defined(TEAMPIPEEXTENSIONS)) */

/*=======================================================================*
 *--- Private defines ---------------------------------------------------*
 *=======================================================================*/

/*=======================================================================*
 *--- Private Types -----------------------------------------------------*
 *=======================================================================*/

/*=======================================================================*
 *--- Private Variables -------------------------------------------------*
 *=======================================================================*/

/*=======================================================================*
 *--- Private functions -------------------------------------------------*
 *=======================================================================*/

#endif /* TEAMPIPES_H */

