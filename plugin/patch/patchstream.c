/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   patchstream.c                                              -*
 *-                                                                         -*
 *-  Purpose :   General patch handling.                                    -*
 *-                                                                         -*
 *===========================================================================*/

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rppatch.h"

#include "patchstream.h"
#include "patchmesh.h"
#include "patch.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/
#define PATCHFLAGTRUE(flag, bit)                                        \
    ((flag & bit) != 0)

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/
#define CHECKSTREAMANDRETURN(success) \
MACRO_START                           \
{                                     \
    if(NULL == (success))             \
    {                                 \
        RWRETURN((RwStream *)NULL);   \
    }                                 \
}                                     \
MACRO_STOP

#define PATCHWRITEREALS(stream, reals, size)                            \
    RwStreamWriteReal(stream, (const RwReal *)reals, size)

#define PATCHWRITEINTS(stream, ints, size)                              \
    RwStreamWriteInt(stream, (const RwInt32 *)ints, size)

#define CHECKSTREAMANDFAIL(success)   \
MACRO_START                           \
{                                     \
    if(NULL == (success))             \
    {                                 \
        RWRETURN((PatchMesh *)NULL);  \
    }                                 \
}                                     \
MACRO_STOP

#define PATCHREADREALS(stream, reals, size)                             \
    RwStreamReadReal(stream, (RwReal *)reals, size)

#define PATCHREADINTS(stream, ints, size)                               \
    RwStreamReadInt(stream, (RwInt32 *)ints, size)

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

/*****************************************************************************
 _rpPatchMeshStreamGetSize

 Returns the stream size of the PatchMesh.

 Inputs : mesh - Mesh to determine the size of.
 Outputs : RwUInt32 - Size in the stream.
 */
RwUInt32
_rpPatchMeshStreamGetSize( const PatchMesh *mesh )
{
    RwUInt32 numTexCoords;
    RwUInt32 sizeTotal;
    RwUInt32 cpSize;
    RwUInt32 flag;

    RWFUNCTION(RWSTRING("_rpPatchMeshStreamGetSize"));
    RWASSERT(NULL != mesh);

    sizeTotal = 0;

    /* Get the definition flags. */
    flag = _rpConstPatchMeshGetFlags(mesh);

    /* Calculate the size necessary for the texCoords. */
    numTexCoords = _rpPatchMeshFlagGetNumTexCoords(flag);
    RWASSERT(numTexCoords < rwMAXTEXTURECOORDS);

    /* Control point definition. */
    cpSize = (PATCHFLAGTRUE(flag, rpPATCHMESHPOSITIONS) * sizeof(RwV3d)) +
             (PATCHFLAGTRUE(flag, rpPATCHMESHNORMALS) * sizeof(RwV3d)) +
             (PATCHFLAGTRUE(flag, rpPATCHMESHPRELIGHTS) * sizeof(RwRGBA)) +
             (numTexCoords * sizeof(RwTexCoords));

    /* Rest of mesh. */
    sizeTotal += sizeof(RpPatchMeshDefinition);
    sizeTotal += cpSize * _rpConstPatchMeshGetNumControlPoints(mesh);
    sizeTotal += sizeof(QuadPatch) * _rpConstPatchMeshGetNumQuadPatches(mesh);
    sizeTotal += sizeof(TriPatch) * _rpConstPatchMeshGetNumTriPatches(mesh);
    sizeTotal += _rpMaterialListStreamGetSize(&(mesh->matList)) +
                 rwCHUNKHEADERSIZE;

    RWRETURN(sizeTotal);
}

/*****************************************************************************
 _rpPatchMeshStreamRead

 Creates a PatchMesh from the stream.

 Inputs : stream - RwStream to read from.
 Outputs : PatchMesh - Constructed patch mesh.
 */
PatchMesh *
_rpPatchMeshStreamRead( RwStream *stream )
{
    PatchMesh *mesh;

    RwUInt32 flags;
    RpPatchMeshDefinition definition;

    RwUInt32 numTexCoords;

    RWFUNCTION(RWSTRING("_rpPatchMeshStreamRead"));
    RWASSERT(NULL != stream);

    /*
     * Material list is going to get hosed (possibly) when the mesh is streamed
     * in as part of the geometry. Hmmm.
     */

    /* Read in the stream definition. */
    stream = PATCHREADINTS(stream, &definition, sizeof(RpPatchMeshDefinition));
    CHECKSTREAMANDFAIL(stream);

    /* Create the patch mesh. */
    mesh = _rpPatchMeshCreate( &definition );
    RWASSERT(NULL != mesh);

    /* Grab the flags. */
    flags = _rpPatchMeshGetFlags(mesh);
    RWASSERT(flags == definition.flag);

    /* Calculate the size necessary for the texCoords. */
    numTexCoords = _rpPatchMeshGetNumTexCoordSets(mesh);
    RWASSERT(numTexCoords < rwMAXTEXTURECOORDS);

    /* Positions. */
    if(PATCHFLAGTRUE(flags, rpPATCHMESHPOSITIONS))
    {
        stream = PATCHREADREALS( stream,
                                 _rpPatchMeshGetPositions(mesh),
                                 sizeof(RwV3d) * definition.numControlPoints );
        CHECKSTREAMANDFAIL(stream);
    }

    /* Normals. */
    if(PATCHFLAGTRUE(flags, rpPATCHMESHNORMALS))
    {
        stream = PATCHREADREALS( stream,
                                 _rpPatchMeshGetNormals(mesh),
                                 sizeof(RwV3d) * definition.numControlPoints );
        CHECKSTREAMANDFAIL(stream);
    }

    /* Pre light colours. */
    if(PATCHFLAGTRUE(flags, rpPATCHMESHPRELIGHTS))
    {
        stream = PATCHREADREALS(
                     stream,
                     _rpPatchMeshGetPreLightColors(mesh),
                     sizeof(RwRGBA) * definition.numControlPoints );
        CHECKSTREAMANDFAIL(stream);
    }

    /* Texture Coords. */
    if(0 < numTexCoords)
    {
        stream = PATCHREADREALS( stream,
                                 _rpPatchMeshGetTexCoords(mesh, 0),
                                 sizeof(RwTexCoords) * numTexCoords *
                                 definition.numControlPoints );
        CHECKSTREAMANDFAIL(stream);
    }

    /* Quad patches. */
    if(0 < definition.numQuadPatches)
    {
        stream = PATCHREADINTS( stream,
                                mesh->quadPatches,
                                sizeof(QuadPatch) *
                                definition.numQuadPatches );
        CHECKSTREAMANDFAIL(stream);
    }

    /* Tri patches. */
    if(0 < definition.numTriPatches)
    {
        stream = PATCHREADINTS( stream,
                                mesh->triPatches,
                                sizeof(TriPatch) * definition.numTriPatches );
        CHECKSTREAMANDFAIL(stream);
    }

    /* Find the stream matlist. */
    RwStreamFindChunk( stream,
                       (RwUInt32)rwID_MATLIST,
                       (RwUInt32 *)NULL,
                       (RwUInt32 *)NULL );
    CHECKSTREAMANDFAIL(stream);

    /* Material list. */
    _rpMaterialListStreamRead(stream, &(mesh->matList));

    RWRETURN(mesh);
}

/*****************************************************************************
 _rpPatchMeshStreamWrite

 Writes the patch mesh to the stream.

 Inputs : mesh   - The patch mesh to write.
          stream - The stream to write to.
 Outputs : RwStream - The stream if successful.
 */
RwStream *
_rpPatchMeshStreamWrite( const PatchMesh *mesh,
                         RwStream *stream )
{
    RwUInt32 cp;
    RwUInt32 numTexCoords;
    RwUInt32 numQuadPatches;
    RwUInt32 numTriPatches;

    RWFUNCTION(RWSTRING("_rpPatchMeshStreamWrite"));
    RWASSERT(NULL != mesh);
    RWASSERT(NULL != stream);

    /* Calculate the size necessary for the texCoords. */
    numTexCoords = _rpConstPatchMeshGetNumTexCoordSets(mesh);
    RWASSERT(numTexCoords < rwMAXTEXTURECOORDS);

    cp = _rpConstPatchMeshGetNumControlPoints(mesh);
    numQuadPatches = _rpConstPatchMeshGetNumQuadPatches(mesh);
    numTriPatches = _rpConstPatchMeshGetNumTriPatches(mesh);

    /* Write the stream definition. */
    stream = PATCHWRITEINTS( stream,
                             &(mesh->userMesh.definition),
                             sizeof(RpPatchMeshDefinition) );
    CHECKSTREAMANDRETURN(stream);

    /* Control point data. */

    /* Positions. */
    if(_rpPatchMeshCheckFlag(mesh, rpPATCHMESHPOSITIONS))
    {
        stream = PATCHWRITEREALS( stream,
                                  _rpConstPatchMeshGetPositions(mesh),
                                  sizeof(RwV3d) * cp );
        CHECKSTREAMANDRETURN(stream);
    }

    /* Normals. */
    if(_rpPatchMeshCheckFlag(mesh, rpPATCHMESHNORMALS))
    {
        stream = PATCHWRITEREALS( stream,
                                  _rpConstPatchMeshGetNormals(mesh),
                                  sizeof(RwV3d) * cp );
        CHECKSTREAMANDRETURN(stream);
    }

    /* Pre light colours. */
    if(_rpPatchMeshCheckFlag(mesh, rpPATCHMESHPRELIGHTS))
    {
        stream = PATCHWRITEREALS( stream,
                                  _rpConstPatchMeshGetPreLightColors(mesh),
                                  sizeof(RwRGBA) * cp );
        CHECKSTREAMANDRETURN(stream);
    }

    /* Texture Coords. */
    if(0 < numTexCoords)
    {
        stream = PATCHWRITEREALS( stream,
                                  _rpConstPatchMeshGetTexCoords(mesh, 0),
                                  sizeof(RwTexCoords) * numTexCoords * cp );
        CHECKSTREAMANDRETURN(stream);
    }

    /* Quad patches. */
    if(0 < numQuadPatches)
    {
        stream = PATCHWRITEINTS( stream,
                                 mesh->quadPatches,
                                 sizeof(QuadPatch) * numQuadPatches );
        CHECKSTREAMANDRETURN(stream);
    }

    /* Tri patches. */
    if(0 < numTriPatches)
    {
        stream = PATCHWRITEINTS( stream,
                                 mesh->triPatches,
                                 sizeof(TriPatch) * numTriPatches );
        CHECKSTREAMANDRETURN(stream);
    }

    /* Material list. */
    _rpMaterialListStreamWrite(&(mesh->matList), stream);

    RWRETURN(stream);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rppatch
 * \ref RpPatchMeshStreamGetSize
 * is used to determine the size in bytes of the binary representation of
 * the specified patch mesh. This is used in the binary chunk header to
 * indicate the size of the patch mesh chunk. The size does not include the
 * size of the chunk header.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh whose binary size is required.
 *
 * \return the chunk size of the patch mesh if successful, or zero otherwise.
 *
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RpPatchMeshStreamRead
 * \see RpPatchMeshStreamWrite
 */
RwUInt32
RpPatchMeshStreamGetSize( const RpPatchMesh *patchMesh )
{
    RwUInt32 sizeTotal;

    const PatchMesh *mesh;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshStreamGetSize"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);

    /* Get the interenal patch mesh. */
    mesh = _rpPatchMeshGetConstInternal(patchMesh);
    RWASSERT(NULL != mesh);

    /* Get the size of the patch mesh. */
    sizeTotal = _rpPatchMeshStreamGetSize(mesh) + rwCHUNKHEADERSIZE;

    RWRETURN(sizeTotal);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshStreamRead is used to read a patch mesh from the
 * specified binary stream. Prior to this function call, a binary patch
 * mesh chunk must have been found in the stream using the
 * \ref RwStreamFindChunk API function..
 *
 * The sequence to locate and read a patch mesh from a binary stream is
 * as follows:
 * \code
   RwStream *stream;
   RpPatchMesh *newPatchMesh;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_PATCHMESH, NULL, NULL) )
       {
           newPatchMesh = RpPatchMeshStreamRead(stream);
       }

       RwStreamClose(stream, NULL);
   }
  \endcode
 *
 * The patch plugin must be attached before using this function.
 *
 * \param stream Pointer to the binary stream.
 *
 * \return Returns a pointer to the patch mesh if successful, or NULL if there
 * is an error.
 *
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RpPatchMeshStreamGetSize
 * \see RpPatchMeshStreamRead
 * \see RpPatchMeshStreamWrite
 */
RpPatchMesh *
RpPatchMeshStreamRead( RwStream *stream )
{
    RpPatchMesh *patchMesh;
    PatchMesh *mesh;
    RwUInt32 size;
    RwUInt32 version;
    RwBool status;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshStreamRead"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != stream);

    status = RwStreamFindChunk(stream, rwID_STRUCT, &size, &version);
    if(!status)
    {
        RWERROR((E_RW_READ));
        RWRETURN((RpPatchMesh *)NULL);
    }

    /* Stream read the internal patch mesh. */
    mesh = _rpPatchMeshStreamRead(stream);
    RWASSERT(NULL != mesh);

    /* Get the external patch mesh. */
    patchMesh = _rpPatchMeshGetExternal(mesh);
    RWASSERT(NULL != patchMesh);

    RWRETURN(patchMesh);
}

RpPatchMesh *
_rpPatchMeshStreamReadNoChunk( RwStream *stream )
{
    RpPatchMesh *patchMesh;
    PatchMesh *mesh;

    RWFUNCTION(RWSTRING("_rpPatchMeshStreamReadNoChunk"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != stream);

    /* Stream read the internal patch mesh. */
    mesh = _rpPatchMeshStreamRead(stream);
    RWASSERT(NULL != mesh);

    /* Get the external patch mesh. */
    patchMesh = _rpPatchMeshGetExternal(mesh);
    RWASSERT(NULL != patchMesh);

    RWRETURN(patchMesh);
}

/**
 * \ingroup rppatch
 * \ref RpPatchMeshStreamWrite
 * is used to write the specified patch mesh to the given binary stream.
 * The stream must have been opened prior to this function call.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param patchMesh Pointer to the patch mesh.
 * \param stream    Pointer to the binary stream.
 *
 * \return Returns a pointer to the patch mesh if successful, or NULL if there
 * is an error.
 *
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RpPatchMeshStreamGetSize
 * \see RpPatchMeshStreamRead
 */
const RpPatchMesh *
RpPatchMeshStreamWrite( const RpPatchMesh *patchMesh,
                        RwStream *stream )
{
    const PatchMesh *mesh;
    RwUInt32 size;

    RWAPIFUNCTION(RWSTRING("RpPatchMeshStreamWrite"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != patchMesh);
    RWASSERT(NULL != stream);

    /* Get the internal patch mesh. */
    mesh = _rpPatchMeshGetConstInternal(patchMesh);
    RWASSERT(NULL != mesh);

    size = RpPatchMeshStreamGetSize(patchMesh);

    /* Write out a rwID_PATCHMESH chunk id, followed by a rwID_STRUCT id. */
    if(!RwStreamWriteChunkHeader(stream, rwID_PATCHMESH, size))
    {
        RWRETURN((const RpPatchMesh *)NULL);
    }

    if (!RwStreamWriteChunkHeader(stream, rwID_STRUCT, size - rwCHUNKHEADERSIZE))
    {
        RWRETURN((const RpPatchMesh *)NULL);
    }

    /* Write out the patch mesh. */
    stream = _rpPatchMeshStreamWrite(mesh, stream);
    RWASSERT(NULL != mesh);

    RWRETURN(patchMesh);
}
