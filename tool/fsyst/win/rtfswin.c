#include <stdio.h>
#include <windows.h>

#include <rwplcore.h>
#include <rpdbgerr.h>

#include "rtfswin.h"
#include "rtfsmgr.h"

extern const RwChar *RwGetFileNameFromDeviceName(const RwChar *fileName);

/* Critical section used for file handle delivery in multithreaded cases */
CRITICAL_SECTION section;


/****************************************************************************
 * Return the sector size.
 */
 RwUInt32
 TkWinFSGetSectorSize(RwChar *root)
 {
    RwUInt32 sectorSize;
    RwUInt32 secPerClust, nbFreeClust, totNbClust;
    RwBool   ret;
    
    RWFUNCTION(RWSTRING("TkWinFSGetSectorSize")); 
    
    ret = GetDiskFreeSpace(root, &secPerClust, &sectorSize, 
                           &nbFreeClust, &totNbClust);
    if (ret != FALSE)
    {
        RWRETURN(sectorSize);
    }
    else
    {
        RWRETURN(0);
    }
 }
 
/****************************************************************************
 * This function must be implemented in every file system
 */
RtFile *
TkWinFSGetFileObject( RtFileSystem *fs, RwUInt32 index )
{
    RWFUNCTION(RWSTRING("TkWinFSGetFileObject"));

    if (index < (RwUInt32)((RtFileSystem *)fs)->maxNbOpenFiles)
    {
        RWRETURN ((RtFile *)&((RtWinFileSystem *)fs)->files[index]);
    }
    else
    {
        RWRETURN (NULL);
    }
}

/****************************************************************************
 * Perfom any operation needed on closing this file system
 */
static void
TkWinFSClose( RtFileSystem *winFileSystem )
{
    RWFUNCTION(RWSTRING("TkWinFSClose"));

    RwFree(((RtWinFileSystem *)winFileSystem)->files);

    RWRETURNVOID();
}

/****************************************************************************
 * Here implement all rwfile functions...
 *
 * Open File
 */
RtFileSystemError
TkWinFSystemFOpen ( RtFileSystem  *fs,
                    RtFile        *file,
                    const RwChar  *filename,
                    RwUInt32       flags)
{
    RwChar    lsDeviceFilename[WIN_FS_MAX_PATH_LENGTH];
    RtWinFile *xFile = ((RtWinFile *)file);

    RwUInt32 access;
    RwUInt32 sharing;
    RwUInt32 creation;
    RwUInt32 attributes;

    RWFUNCTION(RWSTRING("TkWinFSystemFOpen"));

    if (NULL == _rtFSBuildFileName( lsDeviceFilename,
                                   WIN_FS_MAX_PATH_LENGTH,
                                   fs->deviceName,
                                   _rtGetFileNameFromDeviceName(filename),
                                   '\\'))
    {
        RWRETURN (RTFS_ERROR_FILENAMETOOLONG);
    }

    access = 0;
    sharing = 0;
    creation = 0;
    attributes = 0;

    if (flags & RTFILE_ACCESS_OPEN_ASYNC)
    {
        attributes = FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING;
        file->isAsync = TRUE;
        file->status   = RTFILE_STATUS_BUSY;
    }
    else
    {
        file->isAsync = FALSE;
    }

    if (flags & RTFILE_ACCESS_OPEN_WRITE)
    {
        access = GENERIC_WRITE;
        sharing = FILE_SHARE_WRITE;

        if (flags & RTFILE_ACCESS_OPEN_APPEND)
        {
            creation = OPEN_ALWAYS;
        }
        else
        {
            RWASSERT((flags & RTFILE_ACCESS_OPEN_CREATE));
            creation = CREATE_ALWAYS;
        }

        if (FALSE == file->isAsync)
        {
            attributes = FILE_ATTRIBUTE_NORMAL;
        }

    }
    else
    {
        creation = OPEN_EXISTING;
        attributes |= FILE_ATTRIBUTE_READONLY;
    }

    if (flags & RTFILE_ACCESS_OPEN_READ)
    {
        access |= GENERIC_READ;
        sharing |= FILE_SHARE_READ;
    }

    xFile->fp = CreateFile( lsDeviceFilename,
                            access,
                            sharing,
                            NULL,
                            creation,
                            attributes,
                            NULL);

    if (xFile->fp == INVALID_HANDLE_VALUE)
    {
        RWRETURN (RTFS_ERROR_FILENOTFOUND);
    }

    if (file->isAsync == FALSE)
    {
        file->status   = RTFILE_STATUS_READY;
    }

    file->error    = RTFILE_ERROR_NOERROR;

    file->position.noSupportValue.low  = 0;
    file->position.noSupportValue.high = 0;

    /* Get file size using windows fct */
    file->length.noSupportValue.low =
                                  GetFileSize(xFile->fp,
                                  (DWORD *)&(file->length.noSupportValue.high));

    /* If we are appending seek to the end */
    if (flags & RTFILE_ACCESS_OPEN_APPEND)
    {
        TkWinFSystemSetPosition(file, 0, RTFILE_POS_END);
    }

    LASTERROR      = GetLastError();

    RWRETURN (RTFS_ERROR_NOERROR);
}

/****************************************************************************
 * Close File
 */
void
TkWinFSystemFClose ( RtFile *file )
{
    RtWinFile *xFile = ((RtWinFile *)file);

    RWFUNCTION(RWSTRING("TkWinFSystemFClose"));

    file->status = RTFILE_STATUS_CLOSED;
    CloseHandle(xFile->fp);

    RWRETURNVOID();
}

/****************************************************************************
 * Read File
 */
RwUInt32
TkWinFSystemFRead ( RtFile     *file,
                    void       *pBuffer,
                    RwUInt32    nBytes )
{
    RwUInt32     nbBytesRead = 0;
    RtWinFile    *xFile  = (RtWinFile *)file;
    RtFileSystem *fs = file->fileSystem;
    RtFileSystemFileFunctionTable *fileFuncs = &fs->fsFileFunc;

    HANDLE fp     = xFile->fp;

    RWFUNCTION(RWSTRING("TkWinFSystemFRead"));

    if (file->isAsync)
    {
        file->outstandingFileOp = RTFS_FILEOP_READ;
    }
    
    /*
     * As sync is called to check the status of async operations
     * we must not read unless file status is READY
     */
    if (file->status != RTFILE_STATUS_READY)
    {
        RWASSERT (FALSE);
        RWRETURN (0);
    }

    /* Asynchronous read */
    if (file->isAsync)
    {
        RwBool  readResult        = FALSE;
        DWORD   nbBytesTransfered = 0;
                 
        /* Assert if read size is not a multiple of the sector size */
        RWASSERT((nBytes % ((RtWinFileSystem *)fs)->sectorSize) == 0);

        /*
         * Event handle for async readfile operation. Need this
         * later to find out when read has actually  finished.
         */
        xFile->eventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);

        /* Overlap structure used for asynchronous read */
        xFile->overlap.Offset     = (DWORD)file->position.noSupportValue.low;
        xFile->overlap.OffsetHigh = (DWORD)file->position.noSupportValue.high;
        xFile->overlap.hEvent     = xFile->eventHandle;

        /* Attempt an asynchronous read using overlapped IO */
        readResult = ReadFile(fp, pBuffer, nBytes, (LPDWORD)&nbBytesTransfered,
                              &xFile->overlap);
        LASTERROR  = GetLastError();

        /* If read finished before readfile function returned */
        if (readResult)
        {
            if (nbBytesTransfered != nBytes)
            {
                RWASSERT(FALSE);
                file->status = RTFILE_STATUS_ERROR;
            }

            /* Adjust file pointer to new position */
            fileFuncs->setPosition(file, nbBytesTransfered,
                                    RTFILE_POS_CURRENT);

            file->status = RTFILE_STATUS_READY;

            /* Trigger Callback */
            rtFSTriggerFileOutstandingOpCallback(file);
        }
        /*else if (LASTERROR == ERROR_HANDLE_EOF)
        {
            file->status = RTFILE_STATUS_EOF;
        }
        */
        else if ((LASTERROR == ERROR_IO_PENDING) ||
                 (LASTERROR == ERROR_SUCCESS))
        {
            file->status = RTFILE_STATUS_BUSY;
        }
        else
        {
            RWASSERT(FALSE);
            file->status = RTFILE_STATUS_ERROR;
        }
    }
    /* Synchronous read */
    else
    {
        file->status = RTFILE_STATUS_BUSY;
        if (ReadFile(fp, pBuffer, nBytes, (LPDWORD)&nbBytesRead, NULL))
        {
            RwInt32 low = file->position.noSupportValue.low;
            if (low + (RwInt32)nbBytesRead >= low)
            {
                file->position.noSupportValue.low += nbBytesRead;
            }
            else
            {
                RwInt32 nbToMaxInt = RwInt32MAXVAL - low;
                file->position.noSupportValue.low = nbBytesRead - nbToMaxInt;
                ++ file->position.noSupportValue.high;
            }
            
            file->status = RTFILE_STATUS_READY;
        }
        else
        {
            file->status = RTFILE_STATUS_ERROR;
        }

        LASTERROR = GetLastError();
    }

    RWRETURN (nbBytesRead);
}

/****************************************************************************
 * Write File
 */
RwUInt32
TkWinFSystemFWrite ( RtFile       *file,
                     const void   *buffer,
                     RwUInt32      nBytes )
{
    RwUInt32     nbBytesWriten;
    RtWinFile    *xFile  = (RtWinFile *)file;
    RtFileSystem *fs = file->fileSystem;
    RtFileSystemFileFunctionTable *fileFuncs = &fs->fsFileFunc;

    HANDLE fp     = xFile->fp;

    RWFUNCTION(RWSTRING("TkWinFSystemFWrite"));

    if (file->isAsync)
    {
        RwBool writeResult      = FALSE;
        DWORD nbBytesTransfered = 0;
        
        /* Assert if write size is not a multiple of the sector size */
        RWASSERT((nBytes % ((RtWinFileSystem *)fs)->sectorSize) == 0);

        /*
         * Event handle for async write operation. Need this
         * later to find out when write has actually finished.
         */
        xFile->eventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);

        /*
         * Overlap structure used for async write
         * This structure define the offset at which to start writing
         */
        xFile->overlap.Offset     = (DWORD)file->position.noSupportValue.low;
        xFile->overlap.OffsetHigh = (DWORD)file->position.noSupportValue.high;
        xFile->overlap.hEvent     = xFile->eventHandle;

        writeResult = WriteFile(fp, buffer, nBytes, &nbBytesTransfered,
                                &xFile->overlap);

        LASTERROR   = GetLastError();

        /* If write finished before write file function returned */
        if (writeResult)
        {
            if (nbBytesTransfered != nBytes)
            {
                RWASSERT(FALSE);
                file->status = RTFILE_STATUS_ERROR;
            }

            /* Update file's information to the new position*/
            fileFuncs->setPosition(file, nbBytesTransfered,
                                   RTFILE_POS_CURRENT);

            file->status = RTFILE_STATUS_READY;
        }
        else if ((LASTERROR == ERROR_IO_PENDING) ||
                 (LASTERROR == ERROR_SUCCESS))
        {
            file->status  = RTFILE_STATUS_BUSY;
        }
        else
        {
            RWASSERT(FALSE);
            file->status  = RTFILE_STATUS_ERROR;
        }
    }
    else
    {
        file->status = RTFILE_STATUS_BUSY;
        if (WriteFile(fp, buffer, nBytes, (LPDWORD)&nbBytesWriten, NULL))
        {
            file->status   = RTFILE_STATUS_READY;
            file->position.noSupportValue.low += nbBytesWriten;
        }
        else
        {
            LASTERROR = GetLastError();
            file->status = RTFILE_STATUS_ERROR;
        }
    }

    RWRETURN (nbBytesWriten);
}

/****************************************************************************
 * Set Position File
 */
RtInt64
TkWinFSystemSetPosition ( RtFile           *file,
                          RwInt32           offset,
                          RtFileSeekMethod  fPosition)
{
    DWORD     moveMethod;
    LONG      moveDistanceLow   = (RwInt32)offset;
    LONG      moveDistanceHigh  = 0;

    RtWinFile    *xFile = (RtWinFile *)file;

    RWFUNCTION(RWSTRING("TkWinFSystemSetPosition"));

    if (fPosition == RTFILE_POS_BEGIN)
    {
        moveMethod = FILE_BEGIN;        /* FILE_BEGIN -> windows */
    }
    else if (fPosition == RTFILE_POS_CURRENT)
    {
        moveMethod = FILE_CURRENT;
    }
    else
    {
        moveMethod = FILE_END;
    }

    moveDistanceLow = SetFilePointer(xFile->fp, moveDistanceLow, NULL,
                                     moveMethod);

    file->position.noSupportValue.low  = moveDistanceLow;
    file->position.noSupportValue.high = moveDistanceHigh;

    RWRETURN (file->position);
}

/****************************************************************************
 * Sync
 */
RtFileStatus
TkWinFSystemFSync ( RtFile *file, RwBool block )
{
    RtWinFile    *xFile = (RtWinFile *)file;
    RtFileSystemFileFunctionTable *fileFuncs = &file->fileSystem->fsFileFunc;

    RWFUNCTION(RWSTRING("TkWinFSystemFSync"));

    /* If this is an asynchronous file */
    if (file->isAsync)
    {
        /* If there is an IO operation pending */
        if (file->status == RTFILE_STATUS_BUSY)
        {
            DWORD nbBytesTransfered  = 0;
            RtFileStatus statusUpdate = RTFILE_STATUS_READY;

            /* default size callback parameter to file length */
            file->outstandingSize = TkFSManagerGetValue(file->length);

            if(file->outstandingFileOp == RTFS_FILEOP_READ)
            {
                if (!GetOverlappedResult(xFile->fp, &xFile->overlap,
                                         &nbBytesTransfered, block))
                {
                    LASTERROR = GetLastError();

                    if ((LASTERROR == ERROR_IO_PENDING) ||
                        (LASTERROR == ERROR_SUCCESS) ||
                        (LASTERROR == ERROR_IO_INCOMPLETE))
                    {
                        /* operation not complete */
                        RWRETURN (file->status);
                    }

                    statusUpdate = RTFILE_STATUS_ERROR;
                    file->outstandingSize = 0;
                }
                else
                {
                    /* Update file's information, everything is ok */
                    fileFuncs->setPosition(file, nbBytesTransfered,
                                           RTFILE_POS_CURRENT);

                    /* for a read, size callback paramter == bytes read */
                    if(file->outstandingFileOp == RTFS_FILEOP_READ)
                    {
                        file->outstandingSize = nbBytesTransfered;
                    }
                }
            }

            file->status = statusUpdate;

            /* Now trigger the callback */
            rtFSTriggerFileOutstandingOpCallback(file);
        }
    }
    RWRETURN (file->status);
}

/****************************************************************************
 * Abort
 */
RwBool
TkWinFSystemAbort ( RtFile *file )
{
    RWFUNCTION(RWSTRING("TkWinFSystemAbort"));
    file->status = RTFILE_STATUS_CLOSED;
    RWRETURN (TRUE);
}

/****************************************************************************
 * This function must be implemented in every file system
 * It returns the status of this file
 */
static RtFileStatus
TkWinFSFileGetStatus( RtFile *file )
{
    RWFUNCTION(RWSTRING("TkWinFSFileGetStatus"));

    if (file->status != RTFILE_STATUS_CLOSED)
    {
        TkWinFSystemFSync(file, FALSE);
    }

    RWRETURN (file->status);
}

/****************************************************************************
 * File Exists
 */
static RwBool
TkWinFSystemFileExists( RtFileSystem *fs,
                        const RwChar *fileName )
{
    RwChar      lsDeviceFilename[WIN_FS_MAX_PATH_LENGTH];

    RWFUNCTION(RWSTRING("TkWinFSystemFileExists"));

    if (NULL == _rtFSBuildFileName( lsDeviceFilename,
                                    WIN_FS_MAX_PATH_LENGTH,
                                   ((RtFileSystem *)fs)->deviceName,
                                    _rtGetFileNameFromDeviceName(fileName),
                                    '\\'))
    {
        RWRETURN (FALSE);
    }

    RWRETURN ( GetFileAttributes(lsDeviceFilename) != 0xffffffff );
}

/****************************************************************************
 * Initialise File System
 */

/**
 * \ingroup rtfswin
 * \ref RtWinFSystemInit is called to initialise the Windows file system.
 *
 * This function must be called after opening the file system manager. It is
 * responsible for creating the Windows file system, and initializing its file
 * function table. This initialization function requires a valid device name to
 * be passed. This allows greater flexibility, and is particularly useful
 * during development, i.e. its does not tie the application to reading or
 * writing from/to a specific device. Note that the device name of a specific
 * file system can be changed using the \ref RtFileSystemSetDeviceName function.
 *
 * The file system toolkit allows you to call this function several
 * times. For example you can register several windows file system (each with
 * a different name) that corresponds to each available drive or partition on
 * your local machine. This is a way to ensure that every possible files will
 * be opened using a particular file system. However this solution may not
 * be the most optimal as it will require more memory. Also note that should
 * you wish to create and register a single Windows file system, you will need
 * to change the device name (using the \ref RtFileSystemSetDeviceName
 * function) to the one on which you want to perform some I/O operations.
 *
 * \param maxNbOpenFiles    Maximum number of concurrently opened file this
 *                          file system can handle.
 * \param deviceName        Name of the device this file system represents. The
 *                          device name specified must be a valid partition
 *                          name.
 * \param fileSystemName    This is the unique name for this file system. Note
 *                          that the size of this name must not exceed
 *                          RTFS_MAX_NAME_LENGTH.
 *
 * \return                  A pointer to the newly created Windows file system,
 *                          or NULL if initialization failed.
 */
RtFileSystem *
RtWinFSystemInit( RwInt32 maxNbOpenFiles,
                  RwChar *deviceName,
                  RwChar *fileSystemName )
{
    RwInt32          i;
    RtWinFile       *files;
    RtWinFileSystem *winFileSystem;
    RtFileSystem    *fs;

    RWAPIFUNCTION(RWSTRING("RtWinFSystemInit"));
    RWASSERT (deviceName != NULL);

    /* First check that this file system hasn't already been registered */
    if (RtFSManagerGetFileSystemFromName(fileSystemName) != NULL)
    {
        RWRETURN(FALSE);
    }

    winFileSystem = (RtWinFileSystem *)RwMalloc(sizeof (RtWinFileSystem),
                            rwID_FILESYSTEMPLUGIN | rwMEMHINTDUR_GLOBAL);

    if (winFileSystem)
    {
        fs = (RtFileSystem *)winFileSystem;

        /* Initialise the get object function for this file system */
        fs->fsGetObject    = TkWinFSGetFileObject;
        fs->fsClose        = TkWinFSClose;

        fs->fsFileFunc.open             = TkWinFSystemFOpen;
        fs->fsFileFunc.close            = TkWinFSystemFClose;
        fs->fsFileFunc.read             = TkWinFSystemFRead;
        fs->fsFileFunc.write            = TkWinFSystemFWrite;
        fs->fsFileFunc.setPosition      = TkWinFSystemSetPosition;
        fs->fsFileFunc.sync             = TkWinFSystemFSync;
        fs->fsFileFunc.abort            = TkWinFSystemAbort;
        fs->fsFileFunc.isEOF            = _rtFSIsEOF;
        fs->fsFileFunc.getStatus        = TkWinFSFileGetStatus;
        fs->fsFileFunc.fExists          = TkWinFSystemFileExists;

        /* Now allocate memory for maximum number of open files */
        winFileSystem->files = (RtWinFile *)RwCalloc(maxNbOpenFiles,
                                sizeof (RtWinFile),
                                rwID_FILESYSTEMPLUGIN | rwMEMHINTDUR_GLOBAL);

        files = winFileSystem->files;

        /* ... do other file system specific initialisation... */
        for (i = 0; i < maxNbOpenFiles; ++i)
        {
            RtFile * f       = (RtFile *)&files[i];
            f->fileSystem = (RtFileSystem *)winFileSystem;
        }

        /* Init will also automatically try to register this file system */
        if (_rtFSInit((RtFileSystem *)winFileSystem, maxNbOpenFiles,
                       fileSystemName, deviceName) !=
                       RTFS_ERROR_NOERROR)
        {
            RwFree(winFileSystem->files);
            RwFree(winFileSystem);
            RWRETURN (NULL);
        }
        else
        {
            // Initialise the sector size for this file system.
            RwChar name[_MAX_PATH];
            
            rwstrcpy(name, deviceName);
            rwstrcat(name, "\\");
            winFileSystem->sectorSize = TkWinFSGetSectorSize(name);
        }
        
    }

    RWRETURN ((RtFileSystem *)winFileSystem);
}

