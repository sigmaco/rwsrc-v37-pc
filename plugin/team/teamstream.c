/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>
#include "rpdbgerr.h"

#include "rpmatfx.h"
#include "rpteam.h"

#include "teamstream.h"
#include "team.h"

/*
 *

   .......   .............   ........    ........     ......      .         .
  =======.. =============.  ========..  ========.    ======..    =..       =.
 =========. =============.  =========.  ========.   ========..   ==..     ==.
 ===.  ===/ ===/ ===. ===/  ===. ===.   ===. ===/  ===.   ===.   ===..   ===.
 ===.....        ===.       ===..===.   ===...     ===....===.   ====.. ====.
  ======..       ===.       ========/   =====.     ==========.   =====.=====.
   ======..      ===.       =======.    =====/     ==========.   ===========.
  ...  ===.      ===.       ===. ===.   ===.  ...  ===.   ===.   ===.===/===.
 ===...===.      ===..      ===.. ===.  ===..===.  ===..  ===..  ===..=/ ===..
 =========/     =====.     =====.  ===. ========. =====. =====. =====.  =====.
  =======/      =====/     =====/  ===/ ========/ =====/ =====/ =====/  =====/

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
typedef struct TeamDictionaryBinary TeamDictionaryBinary;
struct TeamDictionaryBinary
{
    RwUInt32 numOfElements;
    RwUInt32 numOfLods;
};

typedef struct TeamBinaryInt TeamBinaryInt;
struct TeamBinaryInt
{
    RwUInt32 numOfPlayerElements;
    RwUInt32 numOfPlayers;
};

typedef struct TeamBinaryReal TeamBinaryReal;
struct TeamBinaryReal
{
    RwReal farRange;
    RwReal invFarRange;
};

typedef struct TeamPlayerBinaryReal TeamPlayerBinaryReal;
struct TeamPlayerBinaryReal
{
    RwSphere boundingSphere;
};

typedef struct TeamPlayerBinaryInt TeamPlayerBinaryInt;
struct TeamPlayerBinaryInt
{
    RwUInt32 flags;
    RpTeamPlayerLOD lod;
};

enum TeamTextureType
{
    NATEAMTEXTURETYPE = 0,
    TEAMTEXTURETEAM,
    TEAMTEXTUREPLAYER,
    TEAMTEXTUREDUAL,
    TEAMTEXTURETYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum TeamTextureType TeamTextureType;

typedef struct TeamTextureBinary TeamTextureBinary;
struct TeamTextureBinary
{
    /* Header streamed. */
    TeamTextureType type;
    RwUInt32 dictionaryIndex;
    RwUInt32 lodIndex;
    RwUInt32 materialIndex;
    RwUInt32 playerIndex;
};

typedef struct TeamTextureStream TeamTextureStream;
struct TeamTextureStream
{
    TeamTextureBinary binary;
    TeamTextureStream * (* textureCallback)(TeamTextureStream *);
    const RpTeam *team;
    const RwTexture *texture;
    RwStream *stream;
    RwUInt32 total;
};

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/

/* Write macros. */
#define CHECKSTREAMWRITEANDFAIL(success)                                \
MACRO_START                                                             \
{                                                                       \
    if(NULL == (success))                                               \
    {                                                                   \
        RWRETURN((RwStream *)NULL);                                     \
    }                                                                   \
}                                                                       \
MACRO_STOP

#define TEAMWRITEREALS(stream, reals, size)                             \
    RwStreamWriteReal(stream, (const RwReal *)reals, size)

#define TEAMWRITEINTS(stream, ints, size)                               \
    RwStreamWriteInt(stream, (const RwInt32 *)ints, size)

/* Read macros. */
#define CHECKSTREAMREADANDFAIL(success)                                 \
MACRO_START                                                             \
{                                                                       \
    if(NULL == (success))                                               \
    {                                                                   \
        RWRETURN((RpTeam *)NULL);                                       \
    }                                                                   \
}                                                                       \
MACRO_STOP

#define TEAMREADREALS(stream, reals, size)                              \
    RwStreamReadReal(stream, (RwReal *)reals, size)

#define TEAMREADINTS(stream, ints, size)                                \
    RwStreamReadInt(stream, (RwInt32 *)ints, size)

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/
static TeamTextureStream *
TeamStreamSizeTexture(TeamTextureStream *streamData)
{
    RWFUNCTION(RWSTRING("TeamStreamSizeTexture"));
    RWASSERT(NULL != streamData);
    RWASSERT(NULL != streamData->texture);

    /* Add the size of the texture. */
    streamData->total += sizeof(TeamTextureBinary);
    streamData->total += rwCHUNKHEADERSIZE; /* For the texture. */
    streamData->total += RwTextureStreamGetSize(streamData->texture);

    RWRETURN(streamData);
}

static TeamTextureStream *
TeamStreamOutTexture(TeamTextureStream *streamData)
{
    RWFUNCTION(RWSTRING("TeamStreamOutTexture"));
    RWASSERT(NULL != streamData);
    RWASSERT(NULL != streamData->stream);
    RWASSERT(NULL != streamData->texture);

    /* Write the texture header. */
    streamData->stream = TEAMWRITEINTS( streamData->stream,
                                        &(streamData->binary),
                                        sizeof(TeamTextureBinary) );

    /* Then the texture. */
    RWASSERT(NULL != streamData->texture);

    streamData->texture = RwTextureStreamWrite( streamData->texture,
                                                streamData->stream );
    RWASSERT(NULL != streamData->texture);

    RWRETURN(streamData);
}

static RpMaterial *
TeamMaterialStreamTextures(RpMaterial *material, void *data)
{
    const RpTeam *team;

    TeamTextureStream *streamData;

    RwTexture *baseTexture;
    RwTexture *dualTexture = (RwTexture *)NULL;
    RwTexture *playerDualTexture;
    RwTexture *playerTexture;
    RwTexture *teamTexture;

    RwBool checkDual;

    RWFUNCTION(RWSTRING("TeamMaterialStreamTextures"));
    RWASSERT(NULL != material);
    RWASSERT(NULL != data);

    /* Grab the team data. */
    streamData = (TeamTextureStream *)data;
    RWASSERT(NULL != streamData);
    team = streamData->team;
    RWASSERT(NULL != team);
    RWASSERT(NULL != streamData->textureCallback);

    /* Set the player number to zero. */
    streamData->binary.playerIndex = 0;

    /* Is the material dual passed? */
    if(rpMATFXEFFECTDUAL == RpMatFXMaterialGetEffects(material))
    {
        checkDual = TRUE;
        dualTexture = RpMatFXMaterialGetDualTexture(material);
    }
    else
    {
        checkDual = FALSE;
    }

    /* Get the base texture. */
    baseTexture = RpMaterialGetTexture(material);

    /* Get the team texture. */
    teamTexture = RpTeamMaterialGetTeamTexture(material, team);

    if( (baseTexture != teamTexture) &&
        (NULL != teamTexture) )
    {
        streamData->texture = teamTexture;
        streamData->binary.type = TEAMTEXTURETEAM;

        streamData = (streamData->textureCallback)(streamData);
        RWASSERT(NULL != streamData);
    }

    /* For all players. */
    for( streamData->binary.playerIndex = 0;
         streamData->binary.playerIndex < RpTeamGetNumberOfPlayers(team);
         streamData->binary.playerIndex++ )
    {
        RpTeamPlayer *player = RpTeamGetPlayer( team,
                                                streamData->binary.playerIndex );
        RWASSERT(NULL != player);

        /* Player texture. */
        playerTexture = RpTeamMaterialGetPlayerTexture(material, player);

        if( (baseTexture != playerTexture) &&
            (teamTexture != playerTexture) &&
            (NULL != playerTexture) )
        {
            streamData->texture = playerTexture;
            streamData->binary.type = TEAMTEXTUREPLAYER;

            streamData = (streamData->textureCallback)(streamData);
            RWASSERT(NULL != streamData);
        }

        /* Dual texture. */
        if(checkDual)
        {
            playerDualTexture = RpTeamMaterialGetPlayerDualTexture( material,
                                                                    player );

            if( (dualTexture != playerDualTexture) &&
                (NULL != playerDualTexture) )
            {
                streamData->texture = dualTexture;
                streamData->binary.type = TEAMTEXTUREDUAL;

                streamData = (streamData->textureCallback)(streamData);
                RWASSERT(NULL != streamData);
            }
        }
    }

    /* Next material. */
    streamData->binary.materialIndex++;

    RWRETURN(material);
}

static RpAtomic *
TeamAtomicStreamTextures(RpAtomic *atomic, TeamTextureStream *streamData)
{
    RpGeometry *geometry;

    RWFUNCTION(RWSTRING("TeamAtomicStreamTextures"));
    RWASSERT(NULL != atomic);
    RWASSERT(NULL != streamData);

    geometry = RpAtomicGetGeometry(atomic);
    RWASSERT(NULL != geometry);

    /* Reset the material index for the new atomic. */
    streamData->binary.materialIndex = 0;

    geometry = RpGeometryForAllMaterials( geometry,
                                          TeamMaterialStreamTextures,
                                          (void *)streamData );
    RWASSERT(NULL != geometry);

    RWRETURN(atomic);
}

/*****************************************************************************
 TeamDictionaryForAllAtomics

 Recurse through the entire dictionary searching for materials with team
 texture extensions.

 Inputs : dictionary - RpTeamDictionary to search.
          streamData - TeamTextureStream data struct.
 Outputs : RpTeamDictionary * - Success?
 */
static RpTeamDictionary *
TeamDictionaryForAllAtomics( RpTeamDictionary *dictionary,
                             TeamTextureStream *streamData )
{
    RWFUNCTION(RWSTRING("TeamDictionaryForAllAtomics"));
    RWASSERT(NULL != dictionary);
    RWASSERT(NULL != streamData);

    for( streamData->binary.dictionaryIndex = 0;
         streamData->binary.dictionaryIndex <
            RpTeamDictionaryGetNumOfElements(dictionary);
         streamData->binary.dictionaryIndex++ )
    {
        RpTeamDictionaryElement *element;

        element = &(dictionary->elements[streamData->binary.dictionaryIndex]);
        RWASSERT(NULL != element);

        for( streamData->binary.lodIndex = 0;
             streamData->binary.lodIndex < rpTEAMELEMENTMAXLOD;
             streamData->binary.lodIndex++ )
        {
            RpAtomic *atomic = element->element[streamData->binary.lodIndex];

            if(NULL != atomic)
            {
                if(NULL == TeamAtomicStreamTextures(atomic, streamData))
                {
                    RWRETURN(dictionary);
                }
            }
        }
    }

    RWRETURN(dictionary);
}

/*****************************************************************************
 GetHierarchyFrameRecurse

 Get the hierarchy frame from the complete hierarchy.

 Inputs : frame - RwFrame to test.
          ptr   - data.
 Outputs : RwFrame - Did we find the frame?
 */
static RwFrame *
GetHierarchyFrameRecurse(RwFrame *frame, void *ptr)
{
    RwFrame **hierarchyFrame = (RwFrame **)ptr;

    RWFUNCTION(RWSTRING("GetHierarchyFrameRecurse"));
    RWASSERT(NULL != frame);

    if (RpHAnimFrameGetHierarchy(frame))
    {
        *hierarchyFrame = frame;
    }
    else
    {
        /* Look in child frames */
        RwFrameForAllChildren(frame, GetHierarchyFrameRecurse, hierarchyFrame);

        if (!(*hierarchyFrame))
        {
            RWRETURN(frame); /* Still haven't found it */
        }
    }

    RWRETURN((RwFrame *)NULL);
}

static RwFrame *
ClumpGetHierarchyFrame(RpClump *clump)
{
    RwFrame *frame = RpClumpGetFrame(clump);
    RwFrame *hierarchyFrame = (RwFrame *)NULL;

    RWFUNCTION(RWSTRING("ClumpGetHierarchyFrame"));
    RWASSERT(NULL != clump);

    if (RpHAnimFrameGetHierarchy(frame))
    {
        hierarchyFrame = frame;
    }
    else
    {
        RwFrameForAllChildren(frame,
                              GetHierarchyFrameRecurse,
                              &hierarchyFrame);
    }

    RWRETURN(hierarchyFrame);
}

/*****************************************************************************
 _rpTeamStreamGetSize

 Returns the stream size of the RpTeam.

 Inputs : team - RpTeam to determine the size of.
 Outputs : RwUInt32 - Size in the stream.
 */
RwUInt32
_rpTeamStreamGetSize( const RpTeam *team )
{
    RwUInt32 sizeTotal;
    RwUInt32 iPlayer;

    RpTeamDictionary *dictionary;

    TeamTextureStream texturesData;

    RWFUNCTION(RWSTRING("_rpTeamStreamGetSize"));
    RWASSERT(NULL != team);

    sizeTotal = 0;
    /* Team. */
    sizeTotal += sizeof(TeamBinaryInt);
    sizeTotal += sizeof(TeamBinaryReal);
    /* Players. */
    for( iPlayer = 0; iPlayer < RpTeamGetNumberOfPlayers(team); iPlayer++ )
    {
        RpTeamPlayer *player;
        RpClump *clump;
        RwFrame *root;

        player = RpTeamGetPlayer(team, iPlayer);
        RWASSERT(NULL != player);

        /* Player. */
        sizeTotal += sizeof(TeamPlayerBinaryReal);
        sizeTotal += sizeof(TeamPlayerBinaryInt);
        sizeTotal += sizeof(RwUInt32) * team->data.numOfPlayerElements;

        /* Frames. */

        /* How big is the hierarchy. */
        clump = RpClumpCreate();
        RWASSERT(NULL != clump);
        root = RwFrameGetParent(player->frame);
        RWASSERT(NULL != root);
        RpClumpSetFrame(clump, root);
        sizeTotal += RpClumpStreamGetSize(clump);
        RpClumpSetFrame(clump, (RwFrame *)NULL);
        RpClumpDestroy(clump);
    }

    /* Dictionary extension data. */
    texturesData.team = team;
    texturesData.textureCallback = TeamStreamSizeTexture;
    texturesData.stream = (RwStream *)NULL;
    texturesData.texture = (const RwTexture *)NULL;
    texturesData.total = 0;
    texturesData.binary.type = NATEAMTEXTURETYPE;
    texturesData.binary.dictionaryIndex = 0;
    texturesData.binary.lodIndex = 0;
    texturesData.binary.materialIndex = 0;
    texturesData.binary.playerIndex = 0;

    /* We pass through the dictionary counting the player and team
     * specific textures stream sizes. */
    dictionary = RpTeamGetDictionary(team);
    if(NULL != dictionary)
    {
        /* First we count the team, player and dual specific textures. */
        dictionary = TeamDictionaryForAllAtomics( dictionary,
                                                  &texturesData );
        RWASSERT(NULL != dictionary);

        /* Add the texture size. */
        sizeTotal += texturesData.total;
    }

    /* And always a footer. */
    sizeTotal += sizeof(TeamTextureBinary);

    RWRETURN(sizeTotal);
}

/*****************************************************************************
 _rpTeamStreamRead

 Creates a RpTeam from the stream.

 Inputs : stream     - RwStream to read from.
          dictionary - RpTeamDictionary to attach team extension to.
 Outputs : RpTeam - Constructed RpTeam.
 */
RpTeam *
_rpTeamStreamRead(RwStream *stream, RpTeamDictionary *dictionary)
{
    RpTeam *team;

    TeamBinaryInt dataInt;
    TeamBinaryReal dataReal;

    RwUInt32 sizeExpected;
    RwUInt32 version;
    RwUInt32 iPlayer;

    RwUInt32 *elements;

    RwBool success;
    RwBool moreTextures;

    RWFUNCTION(RWSTRING("_rpTeamStreamRead"));
    RWASSERT(NULL != stream);
    RWASSERT(NULL != dictionary);

    /* We should already be _at_ the rwID_TEAM, so find the rwID_STRUCT. */
    success = RwStreamFindChunk( stream,
                                 rwID_STRUCT,
                                 &sizeExpected,
                                 &version );
    RWASSERT(success);

    /* Load the binary data. */

    stream = TEAMREADINTS( stream,
                           &dataInt,
                           sizeof(TeamBinaryInt) );
    stream = TEAMREADREALS( stream,
                            &dataReal,
                            sizeof(TeamBinaryReal) );

    /* Create the team. */
    team = RpTeamCreate( dataInt.numOfPlayers,
                         dataInt.numOfPlayerElements );
    RWASSERT(NULL != team);

    /* Attach the dictionary to the team. */
    team = RpTeamSetDictionary(team, dictionary);
    RWASSERT(NULL != team);

    /* Setup the lod data. */
    team = RpTeamSetLODFarRange(team, dataReal.farRange);
    RWASSERT(NULL != team);

    /* Grab some data for the dictionary elements. */
    elements = (RwUInt32 *)RwMalloc(sizeof(RwUInt32) * RpTeamGetNumberOfElements(team),
                                    rwID_TEAMPLUGIN | rwMEMHINTDUR_FUNCTION);
    RWASSERT(NULL != elements);

    /* Now the players. */
    for( iPlayer = 0; iPlayer < RpTeamGetNumberOfPlayers(team); iPlayer++ )
    {
        RpTeamPlayer *player;

        TeamPlayerBinaryReal playerReals;
        TeamPlayerBinaryInt playerInts;

        RpClump *clump;
        RwFrame *frame;

        RwSphere *sphere;

        RwUInt32 iElements;

        /* Get the player. */
        player = RpTeamGetPlayer(team, iPlayer);
        RWASSERT(NULL != player);

        /* Player readls. */
        stream = TEAMREADREALS( stream,
                                &playerReals,
                                sizeof(TeamPlayerBinaryReal) );
        RWASSERT(NULL != stream);

        stream = TEAMREADINTS( stream,
                               &playerInts,
                               sizeof(TeamPlayerBinaryInt) );
        RWASSERT(NULL != stream);

        /* Setup bounding sphere. */
        sphere = RpTeamPlayerGetBoundingSphere(player);
        RWASSERT(NULL != sphere);

        sphere->radius = playerReals.boundingSphere.radius;
        sphere->center = playerReals.boundingSphere.center;

        /* Flags. */
        if(playerInts.flags & TEAMPLAYERSELECTED)
        {
            player = RpTeamPlayerSetActivity(player, TRUE);
            RWASSERT(NULL != player);
        }

        if(playerInts.flags & TEAMPLAYERSHADOW)
        {
            player = RpTeamPlayerSetShadows(player, TRUE);
            RWASSERT(NULL != player);
        }

        /* Lod. */
        player = RpTeamPlayerSetCurrentLOD(player, playerInts.lod.currentLOD);
        RWASSERT(NULL != player);

        /* Sync. */
        player = RpTeamPlayerSetSyncLODPerFrame(player, playerInts.lod.syncLODPerFrame);
        RWASSERT(NULL != player);

        /* Elements. */
        stream = TEAMREADINTS( stream, elements,
                               sizeof(RwUInt32) * RpTeamGetNumberOfElements(team) );
        RWASSERT(NULL != stream);

        for(iElements = 0; iElements < RpTeamGetNumberOfElements(team); iElements++ )
        {
            player = RpTeamPlayerSetElement( player,
                                             iElements,
                                             elements[iElements] );
            RWASSERT(NULL != player);
        }

        /* Frames. */

        /* Get frame from clump. */
        success = RwStreamFindChunk(stream, rwID_CLUMP, &sizeExpected, &version);
        RWASSERT(success);
        clump = RpClumpStreamRead(stream);
        RWASSERT(NULL != clump);
        frame = ClumpGetHierarchyFrame(clump);
        RWASSERT(NULL != frame);
        player = RpTeamPlayerSetHierarchyFrame(player, frame);
        RWASSERT(NULL != player);
        frame = RwFrameUpdateObjects(frame->root);
        RWASSERT(NULL != frame);
        clump = RpClumpSetFrame(clump, (RwFrame *)NULL);
        RWASSERT(NULL != clump);
        RpClumpDestroy(clump);
    }

    /* Free the elements. */
    RWASSERT(NULL != elements);
    RwFree(elements);

    /* Dictionary extension data. */

    for( moreTextures = TRUE; moreTextures; )
    {
        TeamTextureBinary binary;

        /* Read in a texture definition. */
        stream = TEAMREADINTS( stream,
                               &binary,
                               sizeof(TeamTextureBinary) );
        RWASSERT(NULL != stream);

        /* If type is non-null. */
        if(binary.type)
        {
            RwTexture *texture;

            /* Load the texture. */

            /* Find the chunk. */
            success = RwStreamFindChunk( stream,
                                         rwID_TEXTURE,
                                         &sizeExpected,
                                         &version );
            RWASSERT(success);

            texture = RwTextureStreamRead(stream);

            /* If the load was successful, attach it. */
            if(NULL != texture)
            {
                RpMaterial *material;
                RpAtomic *atomic;
                RpGeometry *geometry;
                RpTeamPlayer *player;

                /* Get the player. */
                player = RpTeamGetPlayer(team, binary.playerIndex);
                RWASSERT(NULL != player);

                /* Get the material. */
                atomic = RpTeamDictionaryGetLODElement( dictionary,
                                                        binary.lodIndex,
                                                        binary.dictionaryIndex );
                RWASSERT(NULL != atomic);
                geometry = RpAtomicGetGeometry(atomic);
                RWASSERT(NULL != geometry);
                material = RpGeometryGetMaterial(geometry, binary.materialIndex);
                RWASSERT(NULL != material);

                /* Attach it to the correct location!! */
                switch (binary.type)
                {
                    case TEAMTEXTURETEAM:
                    {
                        material = RpTeamMaterialSetTeamTexture(material, team, texture);
                        RWASSERT(NULL != material);

                        break;
                    }
                    case TEAMTEXTUREPLAYER:
                    {
                        material = RpTeamMaterialSetPlayerTexture(material, player, texture);
                        RWASSERT(NULL != material);

                        break;
                    }
                    case TEAMTEXTUREDUAL:
                    {
                        material = RpTeamMaterialSetPlayerDualTexture(material, player, texture);
                        RWASSERT(NULL != material);

                        break;
                    }
                    default:
                    {
                        RWASSERT( (binary.type == TEAMTEXTURETEAM) ||
                                  (binary.type == TEAMTEXTUREPLAYER) ||
                                  (binary.type == TEAMTEXTUREDUAL) );
                        break;
                    }
                }
            }
        }
        else
        {
            moreTextures = FALSE;
        }
    }

    RWRETURN(team);
}

/*****************************************************************************
 _rpTeamStreamWrite

 Writes the team to the stream.

 Inputs : team   - The RpTeam to write.
          stream - The stream to write to.
 Outputs : RwStream - The stream if successful.
 */
RwStream *
_rpTeamStreamWrite( const RpTeam *team,
                    RwStream *stream )
{
    TeamBinaryInt dataInt;
    TeamBinaryReal dataReal;

    RwUInt32 sizeExpected;
    RwUInt32 iPlayer;

    RpTeamDictionary *dictionary;

    TeamTextureStream texturesData;

    RWFUNCTION(RWSTRING("_rpTeamStreamWrite"));
    RWASSERT(NULL != team);
    RWASSERT(NULL != stream);

    /* Lets write the header. */
    sizeExpected = _rpTeamStreamGetSize(team);
    /* Size doesn't include the chunkheader. */
    stream = RwStreamWriteChunkHeader( stream,
                                       rwID_TEAM,
                                       sizeExpected + rwCHUNKHEADERSIZE);
    RWASSERT(NULL != stream);

    /* wrap structured data */
    stream = RwStreamWriteChunkHeader( stream,
                                       rwID_STRUCT,
                                       sizeExpected );
    RWASSERT(NULL != stream);

    /* Binary data. */
    dataInt.numOfPlayerElements = team->data.numOfPlayerElements;
    dataInt.numOfPlayers = team->data.numOfPlayers;
    dataReal.farRange = team->lod.farRange;
    dataReal.invFarRange = team->lod.invFarRange;

    stream = TEAMWRITEINTS( stream,
                            &dataInt,
                            sizeof(TeamBinaryInt) );
    stream = TEAMWRITEREALS( stream,
                             &dataReal,
                             sizeof(TeamBinaryReal) );

    /* Now the players. */
    for( iPlayer = 0; iPlayer < RpTeamGetNumberOfPlayers(team); iPlayer++ )
    {
        RpTeamPlayer *player;

        TeamPlayerBinaryReal playerReals;
        TeamPlayerBinaryInt playerInts;

        RpClump *clump;
        RwFrame *root;
        /* Get the player. */
        player = RpTeamGetPlayer(team, iPlayer);
        RWASSERT(NULL != player);

        /* Real data. */
        playerReals.boundingSphere.center = player->boundingSphere.center;
        playerReals.boundingSphere.radius = player->boundingSphere.radius;

        /* Int data. */
        playerInts.flags = player->flags;
        playerInts.lod.currentLOD = player->lod.currentLOD;
        playerInts.lod.syncLODPerFrame = player->lod.syncLODPerFrame;

        stream = TEAMWRITEREALS( stream,
                                 &playerReals,
                                 sizeof(TeamPlayerBinaryReal) );
        RWASSERT(NULL != stream);

        stream = TEAMWRITEINTS( stream,
                                &playerInts,
                                sizeof(TeamPlayerBinaryInt) );
        RWASSERT(NULL != stream);

        /* Elements. */
        stream = TEAMWRITEINTS( stream,
                                player->elements,
                                sizeof(RwUInt32) * team->data.numOfPlayerElements );
        RWASSERT(NULL != stream);

        /* Frames. */

        /* Write the frame and hierarchy. */
        clump = RpClumpCreate();
        RWASSERT(NULL != clump);
        root = RwFrameGetRoot(player->frame);
        RWASSERT(NULL != root);
        clump = RpClumpSetFrame(clump, root);
        RWASSERT(NULL != clump);
        clump = RpClumpStreamWrite(clump, stream);
        RWASSERT(NULL != stream);
        clump = RpClumpSetFrame(clump, (RwFrame *)NULL);
        RWASSERT(NULL != clump);
        RpClumpDestroy(clump);
    }

    /* Dictionary extension data. */
    texturesData.team = team;
    texturesData.textureCallback = TeamStreamOutTexture;
    texturesData.stream = stream;
    texturesData.texture = (const RwTexture *)NULL;
    texturesData.total = 0;
    texturesData.binary.type = NATEAMTEXTURETYPE;
    texturesData.binary.dictionaryIndex = 0;
    texturesData.binary.lodIndex = 0;
    texturesData.binary.materialIndex = 0;
    texturesData.binary.playerIndex = 0;

    /* We pass through the dictionary streaming out the player and team
     * specific textures. */
    dictionary = RpTeamGetDictionary(team);
    if(NULL != dictionary)
    {
        /* First we count the team, player and dual specific textures. */
        dictionary = TeamDictionaryForAllAtomics( dictionary,
                                                  &texturesData );
        RWASSERT(NULL != dictionary);
    }

    /* Stream a texture footer. */
    texturesData.binary.type = NATEAMTEXTURETYPE;
    texturesData.binary.dictionaryIndex = 0;
    texturesData.binary.lodIndex = 0;
    texturesData.binary.materialIndex = 0;
    texturesData.binary.playerIndex = 0;

    /* Write the texture header. */
    stream = TEAMWRITEINTS( stream,
                            &(texturesData.binary),
                            sizeof(TeamTextureBinary) );
    RWASSERT(NULL != stream);

    RWRETURN(stream);
}

/* DICTIONARY */

/*****************************************************************************
 _rpTeamDictionaryStreamGetSize

 Returns the stream size of the RpTeamDictionary.

 Inputs : dictionary - RpTeamDictionary to determine the size of.
 Outputs : RwUInt32 - Size in the stream.
 */
RwUInt32
_rpTeamDictionaryStreamGetSize( const RpTeamDictionary *dictionary )
{
    RwUInt32 sizeTotal;

    RwUInt32 iDictionaryElement;
    RwUInt32 iLodElement;

    RWFUNCTION(RWSTRING("_rpTeamDictionaryStreamGetSize"));
    RWASSERT(NULL != dictionary);

    sizeTotal = 0;
    sizeTotal += sizeof(TeamDictionaryBinary);
    sizeTotal += (dictionary->numOfElements * sizeof(RwUInt32));

    for( iDictionaryElement = 0;
         iDictionaryElement < dictionary->numOfElements;
         iDictionaryElement++ )
    {
        RpTeamDictionaryElement *dictionaryElement;

        dictionaryElement = &(dictionary->elements[iDictionaryElement]);
        RWASSERT(NULL != dictionaryElement);

        sizeTotal += sizeof(RpTeamElementType);

        for( iLodElement = 0;
             iLodElement < rpTEAMELEMENTLODSUP;
             iLodElement++ )
        {
            RpAtomic *atomic;

            atomic = dictionaryElement->element[iLodElement];

            if(NULL != atomic)
            {
                /* Atomic. */
                sizeTotal += RpAtomicStreamGetSize(atomic);

                /* Static frame id. */
                if(rpTEAMELEMENTSTATIC == dictionaryElement->type)
                {
                    sizeTotal += sizeof(RwInt32);
                }
            }
        }
    }

    RWRETURN(sizeTotal);
}

/*****************************************************************************
 _rpTeamDictionaryStreamRead

 Creates a RpTeamDictionary from the stream.

 Inputs : stream - RwStream to read from.
 Outputs : RpTeamDictionary - Constructed RpTeamDictionary.
 */
RpTeamDictionary *
_rpTeamDictionaryStreamRead( RwStream *stream )
{
    RpTeamDictionary *dictionary;

    TeamDictionaryBinary data;
    RwUInt32 *atomicPattern;

    RwUInt32 iPattern;
    RwUInt32 iLodElement;

    RwUInt32 sizeExpected;
    RwUInt32 version;

    RwBool success;

    RWFUNCTION(RWSTRING("_rpTeamDictionaryStreamRead"));
    RWASSERT(NULL != stream);

    /* We should already be _at_ the rwID_TEAMDICTIONARY, so find the rwID_STRUCT. */
    success = RwStreamFindChunk( stream,
                                 rwID_STRUCT,
                                 &sizeExpected,
                                 &version );
    RWASSERT(success);

    /* Read the binary data. */
    stream = TEAMREADINTS( stream,
                           &data,
                           sizeof(TeamDictionaryBinary) );
    RWASSERT(NULL != stream);

    /* Create our dictionary. */
    RWASSERT(rpTEAMELEMENTLODSUP == data.numOfLods);
    dictionary = RpTeamDictionaryCreate(data.numOfElements);
    RWASSERT(NULL != dictionary);
    RWASSERT(data.numOfElements == dictionary->numOfElements);

    /* Create space for the dictionary pattern. */
    atomicPattern = (RwUInt32 *)RwMalloc( dictionary->numOfElements *
                                          sizeof(RwUInt32),
                                          rwID_TEAMPLUGIN |
                                          rwMEMHINTDUR_FUNCTION );
    RWASSERT(NULL != atomicPattern);

    /* Load in the pattern. */
    stream = TEAMREADINTS( stream,
                           atomicPattern,
                           (dictionary->numOfElements * sizeof(RwUInt32)) );
    RWASSERT(NULL != stream);

    /* Blast throught the pattern to read in the dictionary elements. */
    for( iPattern = 0;
         iPattern < dictionary->numOfElements;
         iPattern++ )
    {
        RpTeamElementType type;

        /* Read it's type. */
        stream = TEAMREADINTS( stream,
                               &type,
                               sizeof(RpTeamElementType) );
        RWASSERT(NULL != stream);


        for( iLodElement = 0;
             iLodElement < rpTEAMELEMENTLODSUP;
             iLodElement++ )
        {
            if(atomicPattern[iPattern] & (1 << iLodElement))
            {
                RpAtomic *atomic;
                RwFrame *frame = (RwFrame *)NULL;

                success = RwStreamFindChunk(stream, rwID_ATOMIC, &sizeExpected, &version);
                RWASSERT(success);

                atomic = RpAtomicStreamRead(stream);
                RWASSERT(NULL != atomic);

                /* If static, add the id. */
                if(rpTEAMELEMENTSTATIC == type)
                {
                    RwInt32 id;

                    stream = TEAMREADINTS( stream,
                                           &id,
                                           sizeof(RwInt32) );
                    RWASSERT(NULL != stream);

                    frame = RwFrameCreate();
                    RWASSERT(NULL != frame);

                    RpHAnimFrameSetID(frame, id);

                    atomic = RpAtomicSetFrame(atomic, frame);
                    RWASSERT(NULL != atomic);
                }

                /* Add element to the dictionary. */
                dictionary = RpTeamDictionarySetLODElement( dictionary,
                                                            atomic,
                                                            type,
                                                            iLodElement,
                                                            iPattern );
                RWASSERT(NULL != dictionary);

                /* Destroy the atomics. */
                RpAtomicDestroy(atomic);

                /* and then it's frame. */
                if(NULL != frame) RwFrameDestroy(frame);
            }
        }
    }

    RWASSERT(NULL != atomicPattern);
    RwFree(atomicPattern);

    RWRETURN(dictionary);
}

/*****************************************************************************
 _rpTeamDictionaryStreamWrite

 Writes the dictionary to the stream.

 Inputs : dictionary - The RpTeamDictionary to write.
          stream     - The stream to write to.
 Outputs : RwStream - The stream if successful.
 */
RwStream *
_rpTeamDictionaryStreamWrite( const RpTeamDictionary *dictionary,
                              RwStream *stream )
{
    TeamDictionaryBinary data;

    RwUInt32 *atomicPattern;

    RwUInt32 iDictionaryElement;
    RwUInt32 iLodElement;
    RwUInt32 sizeExpected;

    RWFUNCTION(RWSTRING("_rpTeamDictionaryStreamWrite"));
    RWASSERT(NULL != dictionary);
    RWASSERT(NULL != stream);

    /* Lets write the header. */
    sizeExpected = _rpTeamDictionaryStreamGetSize(dictionary);
    /* Size doesn't include the chunkheader. */
    stream = RwStreamWriteChunkHeader( stream,
                                       rwID_TEAMDICTIONARY,
                                       sizeExpected + rwCHUNKHEADERSIZE);
    RWASSERT(NULL != stream);

    /* wrap structured data */
    stream = RwStreamWriteChunkHeader( stream,
                                       rwID_STRUCT,
                                       sizeExpected );
    RWASSERT(NULL != stream);

    data.numOfElements = dictionary->numOfElements;
    data.numOfLods = rpTEAMELEMENTLODSUP;

    /* Dictionary setup data. */
    stream = TEAMWRITEINTS( stream,
                            &data,
                            sizeof(TeamDictionaryBinary) );
    RWASSERT(NULL != stream);

    /* Next we work are way through the dictionary. */
    atomicPattern = (RwUInt32 *)RwMalloc( dictionary->numOfElements *
                                          sizeof(RwUInt32),
                                          rwID_TEAMPLUGIN |
                                          rwMEMHINTDUR_FUNCTION );
    RWASSERT(NULL != atomicPattern);

    for( iDictionaryElement = 0;
         iDictionaryElement < dictionary->numOfElements;
         iDictionaryElement++ )
    {
        RpTeamDictionaryElement *dictionaryElement;
        RwUInt32 elementPattern = 0;

        dictionaryElement = &(dictionary->elements[iDictionaryElement]);
        RWASSERT(NULL != dictionaryElement);

        for( iLodElement = 0;
             iLodElement < rpTEAMELEMENTLODSUP;
             iLodElement++ )
        {
            /* Is there an element? */
            if(NULL != dictionaryElement->element[iLodElement])
            {
                elementPattern |= (1 << iLodElement);
            }
        }

        atomicPattern[iDictionaryElement] = elementPattern;
    }

    /* Write out the pattern. */
    stream = TEAMWRITEINTS( stream,
                            atomicPattern,
                            (dictionary->numOfElements * sizeof(RwUInt32)) );
    RWASSERT(NULL != stream);

    /* Blast throught the dictionary to write the atomics. */
    for( iDictionaryElement = 0;
         iDictionaryElement < dictionary->numOfElements;
         iDictionaryElement++ )
    {
        RpTeamDictionaryElement *dictionaryElement;

        dictionaryElement = &(dictionary->elements[iDictionaryElement]);
        RWASSERT(NULL != dictionaryElement);

        if(0 != atomicPattern[iDictionaryElement])
        {
            /* Write it's type. */
            stream = TEAMWRITEINTS( stream,
                                    &(dictionaryElement->type),
                                    sizeof(RpTeamElementType) );
            RWASSERT(NULL != stream);
        }

        for( iLodElement = 0;
             iLodElement < rpTEAMELEMENTLODSUP;
             iLodElement++ )
        {
            if(NULL != dictionaryElement->element[iLodElement])
            {
                RpAtomic *atomic;

                RWASSERT(atomicPattern[iDictionaryElement] & (1 << iLodElement));

                atomic = dictionaryElement->element[iLodElement];
                RWASSERT(NULL != atomic);

                atomic = RpAtomicStreamWrite(atomic, stream);
                RWASSERT(NULL != atomic);

                /* Write it's id. */
                if(rpTEAMELEMENTSTATIC == dictionaryElement->type)
                {
                    RwFrame *frame;
                    RwInt32 id;

                    frame = RpAtomicGetFrame(atomic);
                    RWASSERT(NULL != frame);

                    id = RpHAnimFrameGetID(frame);

                    stream = TEAMWRITEINTS( stream,
                                            &id,
                                            sizeof(RwInt32) );
                    RWASSERT(NULL != stream);
                }
            }
            else
            {
                RWASSERT(~(atomicPattern[iDictionaryElement]) & (1 << iLodElement));
            }
        }
    }

    RWASSERT(NULL != atomicPattern);
    RwFree(atomicPattern);

    RWRETURN(stream);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rpteam
 * \ref RpTeamStreamGetSize
 * is used to determine the size in bytes of the binary representation of
 * the specified team. This is used in the binary chunk header to indicate
 * the size of the team chunk. The size does not include the size of the
 * chunk header.
 *
 * \param team Pointer to the team whose binary size is required.
 *
 * \return The chunk size of the patch mesh if successful, or zero otherwise.
 *
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RpTeamStreamRead
 * \see RpTeamStreamWrite
 */
RwUInt32
RpTeamStreamGetSize( const RpTeam *team )
{
    RwUInt32 sizeTotal;

    RWAPIFUNCTION(RWSTRING("RpTeamStreamGetSize"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != team);

    /* Get the size of the patch mesh. */
    sizeTotal = _rpTeamStreamGetSize(team) + rwCHUNKHEADERSIZE;

    RWRETURN(sizeTotal);
}

/**
 * \ingroup rpteam
 * \ref RpTeamStreamRead is used to read a team from the specified binary
 * stream. Prior to this function call, a binary team chunk must have been
 * found in the stream.
 *
 * The team's dictionary should be passed to the stream reading function.
 * This allows the team to reattach the team and player specific textures
 * setup for the dictionary element geometry materials.
 *
 * The sequence to locate and read a team from a binary stream where a
 * team dictionary already exists, is as follows:
 * \code
   RwStream *stream;
   RpTeam *newTeam;
   RpTeamDictionary *teamDictionary;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_TEAM, NULL, NULL) )
       {
           newTeam = RpTeamStreamRead(stream, teamDictionary);
       }

       RwStreamClose(stream, NULL);
   }
   \endcode
 *
 * \param stream     Pointer to the binary stream.
 * \param dictionary Pointer to the dictionary to attach the team specific
 *                   extensions.
 *
 * \return Returns a pointer to the team if successful, or NULL if there
 * is an error.
 *
 * \see RpTeamStreamWrite
 * \see RpTeamStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RwStreamFindChunk
 * \see RpTeamPluginAttach
 */
RpTeam *
RpTeamStreamRead( RwStream *stream, RpTeamDictionary *dictionary )
{
    RpTeam *team;

    RWAPIFUNCTION(RWSTRING("RpTeamStreamRead"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != stream);
    RWASSERT(NULL != dictionary);

    /* Stream read the team. */
    team = _rpTeamStreamRead(stream, dictionary);
    RWASSERT(NULL != team);

    RWRETURN(team);
}

/**
 * \ingroup rpteam
 * \ref RpTeamStreamWrite is used to write the specified team to the given
 * binary stream. The stream must have been opened prior to this function call.
 *
 * \param team   Pointer to the team.
 * \param stream Pointer to the binary stream.
 *
 * \return Returns a pointer to the team if successful, or NULL if there
 * is an error.
 *
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RpTeamStreamRead
 * \see RpTeamStreamGetSize
 */
const RpTeam *
RpTeamStreamWrite( const RpTeam *team,
                   RwStream *stream )
{
    RWAPIFUNCTION(RWSTRING("RpTeamStreamWrite"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != team);
    RWASSERT(NULL != stream);

    /* Write out the patch mesh. */
    stream = _rpTeamStreamWrite(team, stream);
    RWASSERT(NULL != stream);

    RWRETURN(team);
}

/*---Dictionary--------------------------------------------------------------*/

/**
 * \ingroup rpteam
 * \ref RpTeamDictionaryStreamGetSize is used to determine the size in bytes of
 * the binary representation of the specified dictionary. This is used in the
 * binary chunk header to indicate the size of the team chunk. The size
 * does not include the size of the chunk header.
 *
 * \param dictionary Pointer to the dictionary whose binary size is required.
 *
 * \return The chunk size of the patch mesh if successful, or zero otherwise.
 *
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RpTeamDictionaryStreamRead
 * \see RpTeamDictionaryStreamWrite
 */
RwUInt32
RpTeamDictionaryStreamGetSize( const RpTeamDictionary *dictionary )
{
    RwUInt32 sizeTotal;

    RWAPIFUNCTION(RWSTRING("RpTeamDictionaryStreamGetSize"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != dictionary);

    /* Get the size of the dictionary. */
    sizeTotal = _rpTeamDictionaryStreamGetSize(dictionary) + rwCHUNKHEADERSIZE;

    RWRETURN(sizeTotal);
}

/**
 * \ingroup rpteam
 * \ref RpTeamDictionaryStreamRead is used to read a dictionary from the
 * specified binary stream. Prior to this function call, a binary team
 * chunk must have been found in the stream.
 *
 * The sequence to locate and read a team dictionary from a binary stream is
 * as follows:
 * \code
   RwStream *stream;
   RpTeamDictionary *newTeamDictionary;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_TEAMDICTIONARY, NULL, NULL) )
       {
           newTeamDictionary = RpTeamDictionaryStreamRead(stream);
       }

       RwStreamClose(stream, NULL);
   }
   \endcode
 *
 * \param stream Pointer to the binary stream.
 *
 * \return Returns a pointer to the dictionary if successful, or NULL if there
 * is an error.
 *
 * \see RpTeamDictionaryStreamWrite
 * \see RpTeamDictionaryStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RwStreamFindChunk
 * \see RpTeamPluginAttach
 */
RpTeamDictionary *
RpTeamDictionaryStreamRead( RwStream *stream )
{
    RpTeamDictionary *dictionary;

    RWAPIFUNCTION(RWSTRING("RpTeamDictionaryStreamRead"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != stream);

    /* Stream read the dictionary. */
    dictionary = _rpTeamDictionaryStreamRead(stream);
    RWASSERT(NULL != dictionary);

    RWRETURN(dictionary);
}

/**
 * \ingroup rpteam
 * \ref RpTeamDictionaryStreamWrite is used to write the specified dictionary
 * to the given binary stream. The stream must have been opened prior to this
 * function call.
 *
 * \param dictionary   Pointer to the dictionary.
 * \param stream Pointer to the binary stream.
 *
 * \return Returns a pointer to the dictionary if successful, or NULL if there
 * is an error.
 *
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RpTeamStreamRead
 * \see RpTeamStreamGetSize
 */
const RpTeamDictionary *
RpTeamDictionaryStreamWrite( const RpTeamDictionary *dictionary,
                             RwStream *stream )
{
    RWAPIFUNCTION(RWSTRING("RpTeamDictionaryStreamWrite"));
    RWASSERT(0 < TeamModule.numInstances);
    RWASSERT(NULL != dictionary);
    RWASSERT(NULL != stream);

    /* Write out the patch mesh. */
    stream = _rpTeamDictionaryStreamWrite(dictionary, stream);
    RWASSERT(NULL != stream);

    RWRETURN(dictionary);
}
