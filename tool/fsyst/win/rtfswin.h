/*
 * Win File System Module
 *
 * Copyright (c) Criterion Software Limited
 */

#ifndef RTFSWIN_H
#define RTFSWIN_H

/* RWPUBLIC */

/**
 * \defgroup rtfswin Windows
 * \ingroup  rtfsyst
 * 
 * The Windows file system allows for synchronous and asynchronous file 
 * operations to be performed. This file system uses a driver internal 
 * read and write bufferring system.
 */

/**
 * \defgroup rtfswindatatypes Data Types
 * \ingroup  rtfswin
 * 
 * Windows File System Data Types.
 */
/* RWPUBLICEND */

#include "rtfsyst.h"

/* RWPUBLIC */
#include <windows.h>

#define WIN_FS_MAX_PATH_LENGTH 256
#define WIN_SECTOR_SIZE_MAX    2048

typedef struct RtWinFile RtWinFile;
/**
 * \ingroup rtfswindatatypes
 * \struct RtWinFile
 * File object used by the Windows file system. It describes the current state
 * of a specific file on the Windows file system.
 */
struct RtWinFile
{
    RtFile       file;          /**< Parent \ref RtFile object. */
    HANDLE       fp;            /**< Windows file handle. */
    OVERLAPPED   overlap;       /**< Structure for dealing with asynchronous
                                     file loading. */
    HANDLE       eventHandle;   /**< Event handle for dealing with asynchronous
                                     file loading. */
};

typedef struct RtWinFileSystem RtWinFileSystem;
/**
 * \ingroup rtfswindatatypes
 * \struct RtWinFileSystem
 * This represents the Windows file system. The Windows file system
 * contains a number of Windows specific files. This number is specified
 * during the initialization (see \ref RtWinFSystemInit).
 */
struct RtWinFileSystem
{
    RtFileSystem  wfs;          /**< Parent \ref RtFileSystem object. */
    RwUInt32      sectorSize;   /**< Sector size for this file system. */
    RtWinFile    *files;        /**< Windows file system files. */
};

#ifdef    __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* RWPUBLICEND */

RtFile            * 
TkWinFSGetFileObject( RtFileSystem *winFileSystem,
                      RwUInt32 index );

RtFileSystemError   
TkWinFSystemFOpen( RtFileSystem *fs,
                   RtFile *file, 
                   const RwChar *filename, 
                   RwUInt32 flags);

void                
TkWinFSystemFClose( RtFile *file);

RwUInt32            
TkWinFSystemFRead( RtFile *file, 
                   void *pBuffer, 
                   RwUInt32 nBytes );

RwUInt32            
TkWinFSystemFWrite( RtFile *file, 
                    const void *buffer, 
                    RwUInt32 nBytes );

RtInt64             
TkWinFSystemSetPosition( RtFile *file, 
                         RwInt32 offset, 
                         RtFileSeekMethod fPosition );

RtFileStatus        
TkWinFSystemFSync( RtFile *file, RwBool block );

RwBool              
TkWinFSystemAbort( RtFile *file );

RtFileStatus        
TkWinFSystemFileGetStatus( RtFile *file );

RtFileSystemStatus  
TkWinFSGetStatus( void );

/* RWPUBLIC */

RtFileSystem      * 
RtWinFSystemInit( RwInt32 maxNbOpenFiles, RwChar *deviceName, RwChar *fileSystemName );

#ifdef __cplusplus
}
#endif  /* cplusplus */

/* RWPUBLICEND */

#endif /* RTFSWIN_H */


