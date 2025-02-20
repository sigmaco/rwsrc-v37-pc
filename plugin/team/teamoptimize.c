/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>
#include "rpdbgerr.h"

#include "rpskin.h"
#include "rpteam.h"

#include "teamoptimize.h"
#include "team.h"

/*
 *

   .....   ......   ........  ......  .         .  ......   ........  .......
  =====.. ======.. ========. ======. =..       =. ======.  ========. =======.
 =======. =======. ========/ ======/ ==..     ==. ======/  ========. =======/
 ==.  ==. ==.  ==.    ==.      ==.   ===..   ===.   ==.         ==/  ==.
 ==.  ==. ==...==.    ==.      ==.   ====.. ====.   ==.        ==/   ==....
 ==.  ==. =======/    ==.      ==.   ==.==.==/==.   ==.       ==/    =====.
 ==.  ==. ======/     ==.      ==.   ==. ===/ ==.   ==.      ==/     =====/
 ==.  ==. ==.         ==.      ==.   ==.  =/  ==.   ==.     ==/      ==.
 ==...==. ==.         ==.     .==... ==.      ==.  .==...  ==/.....  ==......
 =======. ==.         ==.    ======. ==.      ==. ======. ========.  =======.
  =====/  ==/         ==/    ======/ ==/      ==/ ======/ ========/  =======/

 *
 */

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/
#define ROUNDUP16(x) (((RwUInt32)(x) + 16 - 1) & ~(16 - 1))

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/
static RpTeam *
TeamOptimizeStatic(RpTeam *team)
{
    RpTeamOptStatic *opData;
    RpTeamOptElement *data;
    RpTeamDictionaryElement *element;

    RwUInt32 numUniqueElements;
    RwUInt32 numRenderElements;

    RwUInt32 size;

    RpTeamRenderStatic *memory;

    RwUInt32 *map;

    RwUInt32 i, j;

    /* For now we're not going to optimize the optimizations as it were. */
    /* We also keep the optimized data completely out of the dictionary. */

    /* Ok this system isn't great when it comes to switching elements that
     * are active in the dictionary. For example is we want a single player
     * to switch his hands then we will have to reoptimize the entire static
     * structure. Hmm not great.
     *
     * Maybe change to totalNumber as apposed to number of selected
     * then we'll have sufficient space for some changes.
     */

    RWFUNCTION(RWSTRING("TeamOptimizeStatic"));
    RWASSERT(NULL != team);

    opData = &(team->opData.stat);

    numUniqueElements = 0;
    numRenderElements = 0;

    /* Count the static objects and their use. */
    for( i = 0; i < team->dictionary->numOfElements; i++ )
    {
        element = &(team->dictionary->elements[i]);

        if( rpTEAMELEMENTSTATIC == element->type )
        {
            RwUInt32 selected = team->selected[i];

            if( 0 < selected )
            {
                numUniqueElements++;
                numRenderElements += selected;
            }
        }
    }

    /* Do we need to optimize? */
    if(numRenderElements == 0)
    {
        opData->numElements = 0;

        /* Mark the static data as optimized. */
        _rpTeamSetElementTypeOptimized( team,
                                        rpTEAMELEMENTSTATIC,
                                        rpNATEAMLOCKMODE,
                                        TRUE );

        /* Return the empty static optimized team. */
        RWRETURN(team);
    }

    /* Ok we've:
     *  numUniqueElements different atomics.
     *  numRenderElements different actual renders!
     * We need ...
     */

    /* ... a map from dictionary to static arrays and ... */
    map = team->dictionary->map;
    RWASSERT(NULL != map);

    size = (sizeof(RpTeamOptElement) * numUniqueElements) +
           (sizeof(RpTeamRenderStatic) * numRenderElements);

    RWASSERT(NULL != opData->data);
    RWASSERT( numUniqueElements <=
                RpTeamDictionaryGetNumOfElements(team->dictionary) );
    RWASSERT( numRenderElements <=
                (RpTeamGetNumberOfPlayers(team) *
                 RpTeamGetNumberOfElements(team)) );

    /* Clean memory. */
    memset(opData->data, 0, size);

    opData->numElements = numUniqueElements;
    opData->optimized = FALSE;

    /* Need to fix up a few pointers. */
    memory = (RpTeamRenderStatic *)ROUNDUP16(opData->data + numUniqueElements);

    /* Need to position the matrix pointers. */
    for( i = 0, j = 0; j < opData->numElements; i++ )
    {
        RwUInt32 selected = team->selected[i];

        /* Make sure we don't over run the dictionary elements. */
        RWASSERT(i < team->dictionary->numOfElements);

        /* Create our map. */
        map[i] = j;

        /* Grab the element. */
        element = &(team->dictionary->elements[i]);

        if( (rpTEAMELEMENTSTATIC == element->type) &&
            (0 < selected) )
        {
            /* Grab the optimization data. */
            data = &(opData->data[j]);

            data->numClones = selected;
            data->staticRender = memory;
            data->lodAtomic = &(element->element);

            j++;

            memory += selected;
        }
    }

    /* Right, time to strip the players of their matrices. */
    for( i = 0; i < team->data.numOfPlayers; i++ )
    {
        RpTeamPlayer *player = &(team->players[i]);

        /* Should the player be rendered? and
         * Does it have any static elements/ */
        if( (player->flags & TEAMPLAYERSELECTED) &&
            (player->elementFlags & rpTEAMELEMENTSTATIC) )
        {
            /* Yes. */
            RpHAnimHierarchy *hierarchy;

            RwMatrix  *matrixArray;

            /* Get the frames hierarchy. */
            hierarchy = RpHAnimFrameGetHierarchy(player->frame);
            RWASSERT(NULL != hierarchy);

            matrixArray = RpHAnimHierarchyGetMatrixArray(hierarchy);
            RWASSERT(NULL != matrixArray);

            /* Need to look throught all the elements. */
            for( j = 0; j < team->data.numOfPlayerElements; j++ )
            {
                RwUInt32 index;

                index = player->elements[j];

                /* Is there an atomic to render? */
                if( rpTEAMELEMENTUNUSED != index )
                {
                    RpAtomic *atomic;

                    /* Grab the dictionary element. */
                    element = &(team->dictionary->elements[index]);
                    RWASSERT(NULL != element->element);

                    /* And it's atomic. */
                    atomic = element->element[rpTEAMELEMENTLODFIRST];
                    RWASSERT(NULL != atomic);

                    if( rpTEAMELEMENTSTATIC == element->type )
                    {
                        RwFrame  *frame;
                        RwInt32   id;
                        RwInt32   animIndex;
                        RwMatrix *matrix;

                        RwUInt32  opIndex;
                        RwUInt32  opStack;

                        /* Grab the frame. */
                        frame = RpAtomicGetFrame(atomic);
                        RWASSERT(NULL != frame);

                        /* Get the ID. */
                        id = RpHAnimFrameGetID(frame);

                        /* Get the index. */
                        animIndex = RpHAnimIDGetIndex(hierarchy, id);

                        /* Grab the matrix. */
                        matrix = &(matrixArray[animIndex]);

                        /* Get the optimized data index. */
                        opIndex = map[index];

                        /* Get the matrix stack. */
                        opStack = opData->data[opIndex].stack;

                        /* Set the matrix. */
                        opData->data[opIndex].staticRender[opStack].matrices =
                            matrix;

                        /* Set the player level. */
                        opData->data[opIndex].staticRender[opStack].player =
                            player;

                        /* Increase the optimized data elements stack. */
                        opData->data[opIndex].stack++;
                    }
                }
            }
        }
    }

#if (defined(RWDEBUG))
    /* Lets just check everything completed successfully. */
    for( i = 0; i < opData->numElements; i++ )
    {
        RWASSERT(opData->data[i].numClones == opData->data[i].stack);
        RWASSERT(NULL != opData->data[i].lodAtomic);

        for( j = 0; j < opData->data[i].numClones; j++ )
        {
            RWASSERT(NULL != opData->data[i].staticRender);
            RWASSERT(NULL != opData->data[i].staticRender[j].matrices);
            RWASSERT(NULL != opData->data[i].staticRender[j].player);
        }
    }
#endif

    /* Mark the static data as optimized. */
    _rpTeamSetElementTypeOptimized( team,
                                    rpTEAMELEMENTSTATIC,
                                    rpNATEAMLOCKMODE,
                                    TRUE );

    /* Check memory. */
    RWCRTCHECKMEMORY();

    RWRETURN(team);
}

static RpTeam *
TeamOptimizeSkinned(RpTeam *team)
{
    RpTeamOptSkin *opData;
    RpTeamOptElement *data;
    RpTeamDictionaryElement *element;

    RwUInt32 numUniqueElements;
    RwUInt32 numRenderElements;

    RwUInt32 size;

    RpTeamRenderSkin *memory;

    RwUInt32 *map;

    RwUInt32 i, j;

    /* THIS IS FAR FROM OPTIMAL - we're replicating information for each */
    /* that is constant for each atomic, ie frame is always the same.    */


    /* For now we're not going to optimize the optimizations as it were. */
    /* We also keep the optimized data completely out of the dictionary. */

    /* Ok this system isn't great when it comes to switching elements that
     * are active in the dictionary. For example is we want a single player
     * to switch his hands then we will have to reoptimize the entire skin
     * structure. Hmm not great.
     *
     * Maybe change to totalNumber as apposed to number of selected
     * then we'll have sufficient space for some changes.
     */

    RWFUNCTION(RWSTRING("TeamOptimizeSkinned"));
    RWASSERT(NULL != team);

    opData = &(team->opData.skin);

    numUniqueElements = 0;
    numRenderElements = 0;

    /* Count the skinned objects and their use. */
    for( i = 0; i < team->dictionary->numOfElements; i++ )
    {
        element = &(team->dictionary->elements[i]);

        if( rpTEAMELEMENTSKINNED == element->type )
        {
            RwUInt32 selected = team->selected[i];

            if( 0 < selected )
            {
                numUniqueElements++;
                numRenderElements += selected;
            }

            /* Find the first skin. */
            if( NULL == opData->firstSkin )
            {
                RpAtomic *atomic;

                atomic = element->element[rpTEAMELEMENTLODFIRST];
                RWASSERT(NULL != atomic);

                opData->firstSkin = RpSkinGeometryGetSkin(RpAtomicGetGeometry(atomic));
                RWASSERT(NULL != opData->firstSkin);
            }
        }
    }

    /* Do we need to optimize? */
    if(numRenderElements == 0)
    {
        opData->numElements = 0;

        /* Mark the skin data as optimized. */
        _rpTeamSetElementTypeOptimized( team,
                                        rpTEAMELEMENTSKINNED,
                                        rpNATEAMLOCKMODE,
                                        TRUE );

        /* Return the empty skin optimized team. */
        RWRETURN(team);
    }

    /* Ok we've:
     *  numUniqueElements different atomics.
     *  numRenderElements different actual renders!
     * We need ...
     */

    /* ... a map from dictionary to skin arrays and ... */
    map = team->dictionary->map;
    RWASSERT(NULL != map);

    size = (sizeof(RpTeamOptElement) * numUniqueElements) +
           (sizeof(RpTeamRenderSkin) * numRenderElements);

    RWASSERT(NULL != opData->data);
    RWASSERT( numUniqueElements <=
                RpTeamDictionaryGetNumOfElements(team->dictionary) );
    RWASSERT( numRenderElements <=
                (RpTeamGetNumberOfPlayers(team) *
                 RpTeamGetNumberOfElements(team)) );

    /* Clean memory. */
    memset(opData->data, 0, size);

    opData->numElements = numUniqueElements;
    opData->optimized = FALSE;

    /* Need to fix up a few pointers. */
    memory = (RpTeamRenderSkin *)(opData->data + numUniqueElements);

    /* Need to position the matrix pointers. */
    for( i = 0, j = 0; j < opData->numElements; i++ )
    {
        RwUInt32 selected = team->selected[i];

        /* Make sure we don't over run the dictionary elements. */
        RWASSERT(i < team->dictionary->numOfElements);

        /* Create our map. */
        map[i] = j;

        /* Grab the element. */
        element = &(team->dictionary->elements[i]);

        if( (rpTEAMELEMENTSKINNED == element->type) &&
            (0 < selected) )
        {
            /* Grab the optimization data. */
            data = &(opData->data[j]);

            data->numClones = selected;
            data->skinRender = memory;
            data->lodAtomic = &(element->element);

            j++;

            memory += selected;
        }
    }

    /* Right, time to strip the players of their matrices. */
    for( i = 0; i < team->data.numOfPlayers; i++ )
    {
        RpTeamPlayer *player = &(team->players[i]);

        /* Should the player be rendered? and
         * Does it have any skin elements/ */
        if( (player->flags & TEAMPLAYERSELECTED) &&
            (player->elementFlags & rpTEAMELEMENTSKINNED) )
        {
            /* Need to look throught all the elements. */
            for( j = 0; j < team->data.numOfPlayerElements; j++ )
            {
                RwUInt32 index;

                index = player->elements[j];

                /* Is there an atomic to render? */
                if( rpTEAMELEMENTUNUSED != index )
                {
                    /* Grab the dictionary element. */
                    element = &(team->dictionary->elements[index]);
                    RWASSERT(NULL != element->element);

                    if( rpTEAMELEMENTSKINNED == element->type )
                    {
                        RwUInt32  opIndex;
                        RwUInt32  opStack;

                        /* Get the optimized data index. */
                        opIndex = map[index];

                        /* Get the matrix stack. */
                        opStack = opData->data[opIndex].stack;

#if(!defined(TEAMMATRIXCACHECACHE))
                        /* Set the matrixCache. */
                        opData->data[opIndex].skinRender[opStack].matrixCache =
                            player->skinCache.matrixCache;
#endif

                        /* Set the player level. */
                        opData->data[opIndex].skinRender[opStack].player =
                            player;

                        /* Increase the optimized data elements stack. */
                        opData->data[opIndex].stack++;
                    }
                }
            }
        }
    }

#if (defined(RWDEBUG))
    /* Lets just check everything completed successfully. */
    for( i = 0; i < opData->numElements; i++ )
    {
        RWASSERT(opData->data[i].numClones == opData->data[i].stack);
        RWASSERT(NULL != opData->data[i].lodAtomic);

        for( j = 0; j < opData->data[i].numClones; j++ )
        {
            RWASSERT(NULL != opData->data[i].skinRender);
#if(!defined(TEAMMATRIXCACHECACHE))
            RWASSERT(NULL != opData->data[i].skinRender[j].matrixCache);
#endif
            RWASSERT(NULL != opData->data[i].skinRender[j].player);
        }
    }
#endif

    /* Mark the skinned data as optimized. */
    _rpTeamSetElementTypeOptimized( team,
                                    rpTEAMELEMENTSKINNED,
                                    rpNATEAMLOCKMODE,
                                    TRUE );

    /* Check memory. */
    RWCRTCHECKMEMORY();

    RWRETURN(team);
}

static RpTeam *
TeamOptimizePatch(RpTeam *team)
{
    RWFUNCTION(RWSTRING("TeamOptimizePatch"));
    RWASSERT(NULL != team);

    /* Mark the patch data as optimized. */
    _rpTeamSetElementTypeOptimized( team,
                                    rpTEAMELEMENTPATCH,
                                    rpNATEAMLOCKMODE,
                                    TRUE );

    RWRETURN(team);
}

static RpTeam *
TeamUnoptimizeStatic(RpTeam *team)
{
    RpTeamOptStatic *opData;

    RWFUNCTION(RWSTRING("TeamUnoptimizeStatic"));
    RWASSERT(NULL != team);

    opData = &(team->opData.stat);
    opData->optimized = FALSE;
    opData->numElements = 0;

    RWRETURN(team);
}

static RpTeam *
TeamUnoptimizeSkinned(RpTeam *team)
{
    RpTeamOptSkin *opData;

    RWFUNCTION(RWSTRING("TeamUnoptimizeSkinned"));
    RWASSERT(NULL != team);

    opData = &(team->opData.skin);
    opData->optimized = FALSE;
    opData->numElements = 0;
    opData->firstSkin = (RpSkin *)NULL;

    RWRETURN(team);
}

static RpTeam *
TeamUnoptimizePatch(RpTeam *team)
{
    RpTeamOptPatch *opData;

    RWFUNCTION(RWSTRING("TeamUnoptimizePatch"));
    RWASSERT(NULL != team);

    opData = &(team->opData.patch);
    opData->optimized = FALSE;

    RWRETURN(team);
}

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/
RpTeam *
_rpTeamOptimizeCreate( RpTeam *team,
                       RwUInt32 type,
                       RwUInt32 locked __RWUNUSED__ )
{
    RpTeam *stat          = team;
    RpTeam *skin          = team;
    RpTeam *patch         = team;

    RWFUNCTION(RWSTRING("_rpTeamOptimizeCreate"));
    RWASSERT(NULL != team);

    if( (rpTEAMELEMENTSTATIC & type) &&
        (FALSE == team->opData.stat.optimized) )
    {
        stat = TeamUnoptimizeStatic(team);
        RWASSERT(stat == team);
        stat = TeamOptimizeStatic(team);
        RWASSERT(stat == team);
    }

    if( (rpTEAMELEMENTSKINNED & type) &&
        (FALSE == team->opData.skin.optimized) )
    {
        skin = TeamUnoptimizeSkinned(team);
        RWASSERT(skin == team);
        skin = TeamOptimizeSkinned(team);
        RWASSERT(skin == team);
    }

    if( (rpTEAMELEMENTPATCH & type) &&
        (FALSE == team->opData.patch.optimized) )
    {
        patch = TeamUnoptimizePatch(team);
        RWASSERT(patch == team);
        patch = TeamOptimizePatch(team);
        RWASSERT(patch == team);
    }

    if( (stat  == team) &&
        (skin  == team) &&
        (patch == team) )
    {
        RWRETURN(team);
    }
    else
    {
        RWRETURN((RpTeam *)NULL);
    }
}

RpTeam *
_rpTeamOptimizeDestroy( RpTeam *team,
                        RwUInt32 type,
                        RwUInt32 locked )
{
    RpTeam *ret = (RpTeam *)NULL;

    RWFUNCTION(RWSTRING("_rpTeamOptimizeDestroy"));
    RWASSERT(NULL != team);

    /* Mark the data as no longer optimized, slighty ott. */
    _rpTeamSetElementTypeOptimized(team, type, locked, FALSE);

    /* And destroy the data. */
    if( rpTEAMELEMENTSTATIC & type )
    {
        ret = TeamUnoptimizeStatic(team);
        RWASSERT(team == ret);
    }

    if( rpTEAMELEMENTSKINNED & type )
    {
        ret = TeamUnoptimizeSkinned(team);
        RWASSERT(team == ret);
    }

    if( rpTEAMELEMENTPATCH & type )
    {
        ret = TeamUnoptimizePatch(team);
        RWASSERT(team == ret);
    }

    RWRETURN(ret);
}


RpTeam *
_rpTeamSetElementTypeOptimized( RpTeam *team,
                                RwUInt32 type,
                                RwUInt32 locked __RWUNUSED__,
                                RwBool optimized )
{
    RWFUNCTION(RWSTRING("_rpTeamSetElementTypeOptimized"));
    RWASSERT(NULL != team);

    if( rpTEAMELEMENTSTATIC & type )
    {
        team->opData.stat.optimized = optimized;
    }

    if( rpTEAMELEMENTSKINNED & type )
    {
        team->opData.skin.optimized = optimized;
    }

    if( rpTEAMELEMENTPATCH & type )
    {
        team->opData.patch.optimized = optimized;
    }

    /* Updata global optimized flag. */
    team->opData.optimized = team->opData.patch.optimized &&
                             team->opData.skin.optimized  &&
                             team->opData.patch.optimized;

    RWRETURN(team);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/
