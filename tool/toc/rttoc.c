/******************************************************************************
 *                                                                            *
 * Module  : rtoc.c                                                           *
 *                                                                            *
 * Purpose : Table Of Contents (TOC)                                          *
 *                                                                            *
 ******************************************************************************
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
 ******************************************************************************/

/**
 * \ingroup rttoc
 * \page rttocoverview RtTOC Toolkit overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rttoc.h
 * \li \b Libraries: rwcore, rttoc
 * \li \b Plugin \b attachments: None
 *
 * \subsection tocoverview Overview
 * The RtTOC toolkit provides functionality to generate a Table Of Contents
 * (TOC) for a stream. The TOC can be serialized at the head of a stream
 * providing an easy listing of the contents of the stream. A TOC will be
 * found at the head of the \c .rws files (\ref glossaryrwsformat) exported
 * using a RenderWare Graphics \ref glossaryexporter.
 *
 * The TOC can also be used to skip through a stream to read specific chunks,
 * rather than reading the stream sequentially. The \e offset element of the
 * \ref RtTOCEntry structure contains the byte offset of its entry's \e chunk
 * \e header from the \e beginning of the file. Hence the sequence of events
 * to read a TOC entry's chunk is:
 *
 * -# Open the \ref RwStream for reading using \ref RwStreamOpen and the
 *    \ref rwSTREAMREAD flag.
 * -# Find the TOC chunk using \ref RwStreamFindChunk with the ID
 *    \ref rwID_TOC.
 * -# Read the TOC using \ref RtTOCStreamRead.
 * -# Identify the \ref RtTOCEntry required by looping over all TOC entries
 *    from index 0 to \ref RtTOCGetNumEntries - 1, inclusive.
 * -# Close the \ref RwStream using \ref RwStreamClose, and re-open it for
 *    reading. This resets the stream position.
 * -# Skip through the stream to the TOC entry's chunk header using the
 *    \e offset element of the \ref RtTOCEntry as the argument to
 *    \ref RwStreamSkip.
 * -# Read the chunk header using \ref RwStreamReadChunkHeaderInfo.
 *    Alternatively use \ref RwStreamFindChunk with the chunk ID in the
 *    \ref RtTOCEntry.
 * -# Read the chunk with the appropriate API stream read function, e.g.
 *    \ref RpWorldStreamRead for \ref rwID_WORLD chunks.
 *
 * It should be observed that after applying the appropriate skip from the
 * \ref RtTOCEntry, chunks should be read in the usual way as if the stream
 * had been read sequentially.
 *
 * \subsection tocrwsuse Use of a TOC in an RWS file
 * RWS files, as output from the RenderWare Graphics exporters, contain a TOC
 * at their head. As the first chunk of the file, this is easy to find, and
 * the TOC interface allows the remaining chunks in the RWS file to be located
 * with ease.
 *
 * The code fragment below shows one possible use of the TOC. The TOC is
 * obtained from an \c .rws file, and all TOC entries are queried whether they
 * need to be loaded with an application defined function \c IsTOCEntryWanted.
 *
 * \code

   RwStream *stream;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "myartwork.rws");
   if( stream )
   {
       RtTOC *toc = (RtTOC *)NULL;


       // get the TOC
       if( RwStreamFindChunk(stream, rwID_TOC, NULL, NULL) )
       {
           toc = RtTOCStreamRead(stream);
       }

       RwStreamClose(stream, NULL);

       if ( NULL != toc )
       {
           RwInt32  numTOCEntries;

           RwInt32  i;


           // determine the number of entries in the TOC
           numTOCEntries = RtTOCGetNumEntries( toc );

           for ( i = 0; i < numTOCEntries; i += 1 )
           {
                RtTOCEntry  *tocEntry;


                // get the next TOC entry
                tocEntry = RtTOCGetEntry( toc, i );

                // determine if the TOC entry is wanted through an application
                // specific function
                if ( FALSE != IsTOCEntryWanted( tocEntry ) )
                {
                    // re-open the stream to reset the position
                    stream = RwStreamOpen( rwSTREAMFILENAME, rwSTREAMREAD,
                                           "myartwork.rws" );
                    if ( NULL != stream )
                    {
                        RwChunkHeaderInfo   chunkHdrInfo;


                        // skip to the chunk header of the TOC entry
                        RwStreamSkip( stream, tocEntry->offset );

                        // read the chunk header
                        RwStreamReadChunkHeaderInfo( stream, &chunkHdrInfo );

                        // stream read clunk with appropriate function
                        // e.g. world = RpWorldStreamRead( stream );

                        RwStreamClose( stream, NULL );
                    }
                }
           }

           // destroy the TOC when finished
           RtTOCDestroy( toc );
       }
   }
  \endcode
 *
 */

/******************************************************************************
 Includes
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "rwcore.h"
#include "rpdbgerr.h"

#include "rttoc.h"



/******************************************************************************
 Local Types
 */

/******************************************************************************
 Local (Static) Prototypes
 */

/******************************************************************************
 Local Defines
 */
#define RWTOCDEFAULTSIZE    (20)

#define TOCDEBUGx

/******************************************************************************
 Globals (across program)
 */

/******************************************************************************
 Local (static) Globals
 */

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                                 TOC function

 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/******************************************************************************
 */

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*
 *                                                                            *
 *                          - Create/Destroy TOCs -                           *
 *                                                                            *
 *!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

/*****************************************************************************
 TOCCreate
 */
static RtTOC *
TOCCreate(RwInt32 numEntries)
{
    RtTOC       *toc;
    RwUInt32    size;

    RWFUNCTION(RWSTRING("TOCCreate"));
    RWASSERT(numEntries > 0);

    size = sizeof(RtTOC) + (sizeof(RtTOCEntry) * (numEntries - 1));
    toc = (RtTOC *)RwMalloc(size, rwID_TOC | rwMEMHINTDUR_EVENT |
        rwMEMHINTFLAG_RESIZABLE);
    if (!toc)
    {
        RWERROR((E_RW_NOMEM, size));
        RWRETURN((RtTOC *)NULL);
    }

    toc->numEntries = numEntries;

    RWRETURN(toc);
}


/*****************************************************************************
 TOCAddEntry
 */
static RtTOC *
TOCAddEntry(RtTOC *toc, RwInt32 id, RwUInt32 offset)
{
    RWFUNCTION(RWSTRING("TOCAddEntry"));
    RWASSERT(NULL != toc);

    /* Check for empty entries in the TOC */
    if (toc->entry[toc->numEntries - 1].id ==
        (RwCorePluginID)(toc->numEntries - 1))
    {
        RtTOC       *newToc;
        RwUInt32    size;

        /* Realloc space for more TOC entries */
        size =
            RtTOCStreamGetSize(toc) + (sizeof(RtTOCEntry) * RWTOCDEFAULTSIZE);
        newToc = RwRealloc(toc, size, rwID_TOC | rwMEMHINTDUR_EVENT |
                    rwMEMHINTFLAG_RESIZABLE);
        if (!newToc)
        {
            RWERROR((E_RW_NOMEM, size));
            RWRETURN((RtTOC *)NULL);
        }

        /* Move the number of active entries to the end of the new TOC */
        newToc->entry[newToc->numEntries + RWTOCDEFAULTSIZE - 1].id =
            newToc->entry[newToc->numEntries - 1].id;

        /* Update the number of entries in the TOC */
        newToc->numEntries = newToc->numEntries + RWTOCDEFAULTSIZE;

        toc = newToc;
    }

    /* Add entry to the TOC */
    toc->entry[toc->entry[toc->numEntries - 1].id].id = id;
    toc->entry[toc->entry[toc->numEntries - 1].id].offset = offset;

    /* Move on to the next empty TOC entry */
    toc->entry[toc->numEntries - 1].id++;

    RWRETURN(toc);
}


/*****************************************************************************
 TOCShrinkWrap
 */
static RtTOC *
TOCShrinkWrap(RtTOC *toc)
{
    RWFUNCTION(RWSTRING("TOCShrinkWrap"));

    if (toc->entry[toc->numEntries - 1].id !=
        (RwCorePluginID)(toc->numEntries))
    {
        RwUInt32    size;
        RtTOC       *newToc;

        toc->numEntries = toc->entry[toc->numEntries - 1].id;

        size = RtTOCStreamGetSize((const RtTOC *)toc);
        newToc = RwRealloc(toc, size, rwID_TOC | rwMEMHINTDUR_EVENT |
                    rwMEMHINTFLAG_RESIZABLE);
        if (!newToc)
        {
            RWERROR((E_RW_NOMEM, size));
            RWRETURN((RtTOC *)NULL);
        }

        toc = newToc;
    }

    RWRETURN(toc);
}


/*****************************************************************************
 TOCIsChunkPublic
 */
static RwBool
TOCIsChunkPublic(RwUInt32 type)
{
    RWFUNCTION(RWSTRING("TOCIsChunkPublic"));

    switch (type)
    {
    case rwID_CAMERA:
    case rwID_TEXTURE:
    case rwID_MATERIAL:
    case rwID_WORLD:
    case rwID_SPLINE:
    case rwID_MATRIX:
    case rwID_GEOMETRY:
    case rwID_CLUMP:
    case rwID_LIGHT:
    case rwID_ATOMIC:
    case rwID_TEXDICTIONARY:
    case rwID_IMAGE:
    case rwID_HANIMANIMATION:
    case rwID_TEAM:
    case rwID_DMORPHANIMATION:
    case rwID_MTEFFECTNATIVE:
    case rwID_MTEFFECTDICT:
    case rwID_TEAMDICTIONARY:
    case rwID_PITEXDICTIONARY:
    case rwID_TOC:
    case rwID_UVANIMDICT:
        {
            RWRETURN(TRUE);
        }
        break;

    default:
        break;
    }

    RWRETURN(FALSE);
}


/*****************************************************************************
 TOCProcessChunk
 */
static void
TOCProcessChunk(RwStream *stream,
                   RwChunkHeaderInfo *info,
                   RwUInt32 *pos,
                   RtTOC **toc)
{
    RWFUNCTION(RWSTRING("TOCProcessChunk"));
    RWASSERT(NULL != stream);
    RWASSERT(NULL != info);
    RWASSERT(NULL != pos);

#if (defined(RWDEBUG) && defined(TOCDEBUG))
    if (TOCIsChunkPublic(info->type))
    {
        RwChar  buffer[256];

        rwsprintf(buffer, "T: %8d L: %8d V: %8d B: %8d C: %8d",
            info->type,
            info->length,
            info->version,
            info->buildNum,
            info->isComplex);

        RwDebugSendMessage(rwDEBUGMESSAGE, RWSTRING("TOCProcessChunk"), buffer);
    }
#endif /* (defined(RWDEBUG) && defined(TOCDEBUG)) */

    /*
     * Only add public chunks to the TOC, (ones the have API serialization
     * function).
     */
    if (TOCIsChunkPublic(info->type))
    {
        /* Add Chunk to TOC */
        *toc = TOCAddEntry(*toc, info->type, *pos - rwCHUNKHEADERSIZE);
    }

#if 0 /* Do not recurse into complex chunks */
    if (info->isComplex)
    {
        RwUInt32    end;

        end = *pos + info->length;
        while (*pos < end)
        {
            RwChunkHeaderInfo   subInfo;

            RwStreamReadChunkHeaderInfo(stream, &subInfo);
            *pos += rwCHUNKHEADERSIZE;

            TOCProcessChunk(stream, &subInfo, pos, toc);
        }

        RWASSERT(*pos == end);
    }
    else
#endif
    {
        RwStreamSkip(stream, info->length);
        *pos += info->length;
    }

    RWRETURNVOID();
}


/**
 * \ingroup rttoc
 * \ref RtTOCCreate creates a Table Of Contents (TOC) for a stream. This
 * function parses the input stream and generates a TOC entry for chunks
 * listed in \ref RwCorePluginID.
 *
 * \note The stream must have been opened for reading prior to calling this
 * function.
 *
 * \note This function parses the stream. On return from this function the
 * stream position will need to be reset. This can be done by closing and then
 * re-opening the stream.
 *
 * \param stream Pointer to \ref RwStream to create a TOC for.
 *
 * \return Pointer to an \ref RtTOC on success, else NULL.
 *
 * \see RtTOCDestroy
 * \see RtTOCGetNumEntries
 * \see RtTOCGetEntry
 * \see RtTOCStreamGetSize
 * \see RtTOCStreamWrite
 * \see RtTOCStreamRead
 */
RtTOC *
RtTOCCreate(RwStream *stream)
{
    RwUInt32            size;
    RwUInt32            pos = 0;
    RwInt32             i;
    RwChunkHeaderInfo   info;
    RtTOC               *toc;
    RtTOC               *newToc;

    RWAPIFUNCTION(RWSTRING("RtTOCCreate"));
    RWASSERT(NULL != stream);

    /* Create a temporary TOC */
    toc = TOCCreate(RWTOCDEFAULTSIZE);
    toc->entry[RWTOCDEFAULTSIZE - 1].id = 0;

    /* Process all chunks */
    while (RwStreamReadChunkHeaderInfo(stream, &info))
    {
        RwUInt32    curPos;

        curPos = pos;

        pos += rwCHUNKHEADERSIZE;

        /* Find the number of entries in the TOC */
        TOCProcessChunk(stream, &info, &pos, &toc);
        RWASSERT(((pos - rwCHUNKHEADERSIZE) - curPos) == info.length);
    }

    /* Must have at least 1 entry in a TOC */
    RWASSERT(toc->entry[toc->numEntries - 1].id > 0);

    /* Shrink wrap temporary TOC to fit */
    newToc = TOCShrinkWrap(toc);
    if (!newToc)
    {
        RtTOCDestroy(toc);
        RWRETURN((RtTOC *)NULL);
    }

    /* Apply TOC size to TOC offsets */
    size = rwCHUNKHEADERSIZE + RtTOCStreamGetSize(newToc);
    for (i = 0; i < newToc->numEntries; i++)
    {
        newToc->entry[i].offset += size;
    }

    RWRETURN(newToc);
}


/**
 * \ingroup rttoc
 * \ref RtTOCDestroy destroys a TOC that was created by calling
 * \ref RtTOCCreate.
 *
 * \param toc Pointer to the \ref RtTOC to destroy.
 *
 * \see RtTOCCreate
 * \see RtTOCGetNumEntries
 * \see RtTOCGetEntry
 * \see RtTOCStreamGetSize
 * \see RtTOCStreamWrite
 * \see RtTOCStreamRead
 */
void
RtTOCDestroy(RtTOC *toc)
{
    RWAPIFUNCTION(RWSTRING("RtTOCDestroy"));
    RWASSERT(NULL != toc);

    RwFree(toc);

    RWRETURNVOID();
}


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*
 *                                                                            *
 *                            - Accessing TOCs -                              *
 *                                                                            *
 *!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

/**
 * \ingroup rttoc
 * \ref RtTOCGetNumEntries gets the number of entries in the TOC.
 *
 * \param toc Pointer to the \ref RtTOC to get the number of entries from.
 *
 * \return \ref RwInt32 containing the number of entries in the TOC.
 *
 * \see RtTOCCreate
 * \see RtTOCDestroy
 * \see RtTOCGetEntry
 * \see RtTOCStreamGetSize
 * \see RtTOCStreamWrite
 * \see RtTOCStreamRead
 */
RwInt32
RtTOCGetNumEntries(const RtTOC *toc)
{
    RWAPIFUNCTION(RWSTRING("RtTOCGetNumEntries"));
    RWASSERT(NULL != toc);

    RWRETURN(toc->numEntries);
}


/**
 * \ingroup rttoc
 * \ref RtTOCGetEntry gets the specified TOC entry from the TOC.
 *
 * \param toc   Pointer to the \ref RtTOC to get the TOC entry from.
 *
 * \param entry \ref RwInt32 containing the zero-based index of the TOC entry
 *              to obtain.
 *
 * \return Pointer to the \ref RtTOCEntry containing the required TOC entry
 *         data.
 *
 * \see RtTOCCreate
 * \see RtTOCDestroy
 * \see RtTOCGetNumEntries
 * \see RtTOCStreamGetSize
 * \see RtTOCStreamWrite
 * \see RtTOCStreamRead
 */
RtTOCEntry *
RtTOCGetEntry(RtTOC *toc, RwInt32 entry)
{
    RWAPIFUNCTION(RWSTRING("RtTOCGetEntry"));
    RWASSERT(NULL != toc);
    RWASSERT(entry >= 0);
    RWASSERT(entry < toc->numEntries);

    RWRETURN(&(toc->entry[entry]));
}


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*
 *                                                                            *
 *                           - TOC Serialization -                            *
 *                                                                            *
 *!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

/**
 * \ingroup rttoc
 * \ref RtTOCStreamGetSize is used to determine the size in bytes of the
 * binary representation of the TOC. This is used in the binary chunk header
 * to indicate the size of the texture chunk. The size does not include the
 * size of the chunk header.
 *
 * \param toc Pointer to the \ref RtTOC whose binary size is required.
 *
 * \return \ref RwUInt32 value equal to the chunk size, in bytes, of the
 * TOC.
 *
 * \see RtTOCCreate
 * \see RtTOCDestroy
 * \see RtTOCGetNumEntries
 * \see RtTOCGetEntry
 * \see RtTOCStreamWrite
 * \see RtTOCStreamRead
 */
RwUInt32
RtTOCStreamGetSize(const RtTOC *toc)
{
    RwUInt32    size = 0;

    RWAPIFUNCTION(RWSTRING("RtTOCStreamGetSize"));
    RWASSERT(NULL != toc);

    size = sizeof(RtTOC) + (sizeof(RtTOCEntry) * (toc->numEntries - 1));

    RWRETURN(size);
}


/**
 * \ingroup rttoc
 * \ref RtTOCStreamWrite is used to write the specified TOC to the given binary
 * stream.
 *
 * Note that the stream will have been opened prior to this function call.

 * \param toc    Pointer to \ref RtTOC to write.
 * \param stream Pointer to \ref RwStream to write the TOC to.
 *
 * \return Pointer to the \ref RtTOC argument if successful or NULL if there
 *         is an error.
 *
 * \see RtTOCCreate
 * \see RtTOCDestroy
 * \see RtTOCGetNumEntries
 * \see RtTOCGetEntry
 * \see RtTOCStreamGetSize
 * \see RtTOCStreamRead
 */
const RtTOC *
RtTOCStreamWrite(RtTOC *toc, RwStream *stream)
{
    RwUInt32    size;
    RwInt32     i;

    RWAPIFUNCTION(RWSTRING("RtTOCStreamWrite"));
    RWASSERT(NULL != toc);
    RWASSERT(NULL != stream);

    size = RtTOCStreamGetSize(toc);

    /* TOC Chunk header */
    if (!RwStreamWriteChunkHeader(stream, rwID_TOC, size))
    {
        RWERROR((E_RW_WRITE));
        RWRETURN((RtTOC *)NULL);
    }

    /* Make sure the TOC is in little endian format before writing */

    for (i = 0; i < toc->numEntries; i++)
    {
        RtTOCEntry  *tocEntry;

        tocEntry = &toc->entry[i];

        (void)RwMemLittleEndian32(&tocEntry->id, sizeof(RwCorePluginID));
        (void)RwMemLittleEndian32(&tocEntry->offset, sizeof(RwUInt32));

        (void)RwMemLittleEndian32(&tocEntry->guid.data1, sizeof(RwUInt32));
        (void)RwMemLittleEndian16(&tocEntry->guid.data2, sizeof(RwUInt16));
        (void)RwMemLittleEndian16(&tocEntry->guid.data3, sizeof(RwUInt16));
    }

    (void)RwMemLittleEndian32(&toc->numEntries, sizeof(RwInt32));

    /* Write the TOC */
    if (!RwStreamWrite(stream, (void *)toc, size))
    {
        RWERROR((E_RW_WRITE));
        toc = (RtTOC *)NULL;
        RWRETURN((RtTOC *)NULL);
    }

    /* Convert back to the correct platform endianess */
    (void)RwMemNative32(&toc->numEntries, sizeof(RwInt32));

    for (i = 0; i < toc->numEntries; i++)
    {
        RtTOCEntry  *tocEntry;

        tocEntry = &toc->entry[i];

        (void)RwMemNative32(&tocEntry->id, sizeof(RwCorePluginID));
        (void)RwMemNative32(&tocEntry->offset, sizeof(RwUInt32));

        (void)RwMemNative32(&tocEntry->guid.data1, sizeof(RwUInt32));
        (void)RwMemNative16(&tocEntry->guid.data2, sizeof(RwUInt16));
        (void)RwMemNative16(&tocEntry->guid.data3, sizeof(RwUInt16));
    }

    RWRETURN(toc);
}


/**
 * \ingroup rttoc
 * \ref RtTOCStreamRead is used to read a TOC from the specified binary stream.
 *
 * Note that prior to this function call a binary TOC chunk must be found in
 * the stream using the \ref RwStreamFindChunk API function.
 *
 * \param stream Pointer to \ref RwStream to read the TOC from.
 *
 * \return Pointer to an \ref RtTOC on success, else NULL.
 *
 * The sequence to locate and read a TOC from a binary stream is as follows:
 * \code

   RwStream *stream;
   RtTOC *toc;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_TOC, NULL, NULL) )
       {
           toc = RtTOCStreamRead(stream);
       }

       RwStreamClose(stream, NULL);
   }
  \endcode
 *
 * \see RtTOCCreate
 * \see RtTOCDestroy
 * \see RtTOCGetNumEntries
 * \see RtTOCGetEntry
 * \see RtTOCStreamGetSize
 * \see RtTOCStreamWrite
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RwStreamFindChunk
 */
RtTOC *
RtTOCStreamRead(RwStream *stream)
{
    RwInt32     numEntries;
    RwUInt32    size;
    RtTOC       *toc;

    RWAPIFUNCTION(RWSTRING("RtTOCStreamRead"));
    RWASSERT(NULL != stream);

    /* Read in the number of TOC entries */
    if (!RwStreamReadInt32(stream, &numEntries, sizeof(RwInt32)))
    {
        RWERROR((E_RW_READ));
        RWRETURN((RtTOC *)NULL);
    }

    /* Create the TOC */
    RWASSERT(numEntries > 0);
    toc = TOCCreate(numEntries);
    if (!toc)
    {
        RWASSERT(NULL != toc);
        RWRETURN((RtTOC *)NULL);
    }

    /* Read in the TOC */
    size = RtTOCStreamGetSize(toc) - sizeof(RwInt32);
    if (!RwStreamRead(stream, ((RwInt32 *)toc) + 1, size))
    {
        RWERROR((E_RW_READ));
        RWRETURN((RtTOC *)NULL);
    }

    /* Convert to the correct platform endianess */
    while (numEntries--)
    {
        RtTOCEntry  *tocEntry;

        tocEntry = &toc->entry[numEntries];

        (void)RwMemNative32(&tocEntry->id, sizeof(RwCorePluginID));
        (void)RwMemNative32(&tocEntry->offset, sizeof(RwUInt32));

        (void)RwMemNative32(&tocEntry->guid.data1, sizeof(RwUInt32));
        (void)RwMemNative16(&tocEntry->guid.data2, sizeof(RwUInt16));
        (void)RwMemNative16(&tocEntry->guid.data3, sizeof(RwUInt16));
    }

    RWRETURN(toc);
}
