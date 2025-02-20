#include "rwplcore.h"
#include "rtfsyst.h"
#include "rtfsmgr.h"
#include "rpdbgerr.h"
#include <string.h>

/**
 * \defgroup rtfsyst RtFSyst
 * \ingroup rtfs
 *
 * File System
 */

/**
 * \ingroup rtfsyst
 * \page rtfsystoverview RtFSyst Overview
 *
 * \par Requirements
 * \li \b Headers: rwplcore.h
 * \li \b Libraries: rwplcore
 *
 * The RenderWare file system toolkit allows the definition of custom file
 * systems. Each file system must be registered with the file system manager,
 * \ref rtfsmgr, as it is the responsibility of the file system manager to
 * perform all generic file operations. A custom file system contains all
 * the functions necessary to undertake synchronous and asynchronous file
 * operations.<br>
 *
 * The file system defaults all streaming to synchronous. This default mode
 * allows you to use all the RwStream API functions normally. The default
 * file system mode can be changed to asynchronous using the
 * \ref RtFileSystemDefaultToAsync function. <br>
 * <br>
 *
 * Also note that the file system toolkit allows you to initialize several
 * custom file systems of the same type. This means that you can register
 * many file systems on the same platform, providing that each of them has a
 * different device name and a different file system name. This means that on
 * certain platforms you can register a file system per existing device.
 *
 * Significant changes have been made to the file system toolkit for RenderWare
 * Graphics 3.7. For details, see the \ref whatsnew37 section.
 *
 */

/*
 * Initialize the file system
 * Must be called by all subclasses, i.e. all custom file systems
 */
RtFileSystemError
_rtFSInit( RtFileSystem *fs,
           RwInt32       maxNbOpenFiles,
           RwChar        fsName[RTFS_MAX_NAME_LENGTH],
           RwChar       *deviceName )
{
#ifdef RWDEBUG
    RwChar msg[41];
#endif

    RWFUNCTION(RWSTRING("_rtFSInit"));
    RWASSERT (deviceName != NULL);
    RWASSERT (rwstrlen(fsName) < RTFS_MAX_NAME_LENGTH);

    /*
     * First try to see if this new file system can be
     * registered against the file system manager
     */
    if (_rtFSManagerIsNewFSRegistrable() != FALSE)
    {
        RwInt32 i;

        /* Check that this fs name is not already in use */
        if (_rtFSManagerIsFSNameInUse(fsName))
        {
            RWRETURN (RTFS_ERROR_FILESYSTEMNAMEINUSE);
        }
        else
        {
            rwstrcpy(fs->name, fsName);
        }

        fs->maxNbOpenFiles = maxNbOpenFiles;
        fs->defaultToAsync = FALSE;

        /* Fill out the Files */
        for (i = 0; i < maxNbOpenFiles; ++i)
        {
            RtFile *file  = fs->fsGetObject(fs, i);
            file->status  = RTFILE_STATUS_CLOSED;
            file->isAsync = fs->defaultToAsync;
        }
    }
    else
    {
#ifdef RWDEBUG
        rwsprintf(msg, "FileSystem %s Failed to Initialise (file system is not"
                       " registrable)", fs->name);
        RtFileSystemVerboseMessage("_rtFSInit", msg);
#endif

        RWRETURN ((RtFileSystemError)RTFS_ERROR_TOOMANYFILESYSTEMS);
    }

    fs->deviceName = NULL;

    {
        RtFileSystemError ret;
        if ((ret = RtFileSystemSetDeviceName(fs, deviceName)) !=
                    RTFS_ERROR_NOERROR)
        {
            RWRETURN (ret);
        }
    }

    fs->status = RTFS_STATUS_READY;
    fs->CallBacks.registerCallBack   = NULL;
    fs->CallBacks.unregisterCallBack = NULL;
    
#ifdef RWDEBUG
    rwsprintf(msg, "FileSystem %s Successfully Initialised", fs->name);
    RtFileSystemVerboseMessage("_rtFSInit", msg);
#endif

    RWRETURN ((RtFileSystemError)RTFS_ERROR_NOERROR);
}

/*
 * This function looks for the ':' character in the file name specified and
 * return the filename without the device name attached to it.
 */
const RwChar *
_rtGetFileNameFromDeviceName( const RwChar *fileName )
{
    RwInt32 nIndex;
    const RwChar *fPath = fileName;

    RWFUNCTION(RWSTRING("_rtGetFileNameFromDeviceName"));

    for (nIndex = 0; nIndex < (RwInt32)rwstrlen(fileName); ++nIndex)
    {
        if (fileName[nIndex] == ':')
        {
            fPath = &fileName[nIndex + 1];
            break;
        }
    }
    RWRETURN (fPath);
}


/*
 * Combines the drive name and filename, and sets all seperators to that
 * specified to create a valid path.
 *
 * Returns the valid file name.
 */
RwChar *
_rtFSBuildFileName( RwChar       *buffer,
                    RwInt32       bufferLength,
                    const RwChar *driveName,
                    const RwChar *filePath,
                    RwChar        cSeparator)
{
    RwInt32 driveLength = strlen(driveName);
    RwInt32 pathLength  = strlen(filePath);
    RwInt32 index;

    RWFUNCTION(RWSTRING("_rtFSBuildFileName"));

    /* Check for space including null terminator */
    if (driveLength + pathLength > (bufferLength - 1))
    {
        RWRETURN (NULL);
    }

    /* Copy Drive Name */
    rwstrcpy(buffer, driveName);

    /* Copy Path Name, set all slashes to specified seperator */
    for (index = 0; index <= pathLength; ++index)
    {
        if (filePath[index] == '/' ||  filePath[index] == '\\')
        {
            buffer[index + driveLength] = cSeparator;
        }
        else
        {
            buffer[index + driveLength] = filePath[index];
        }
    }

    RWRETURN (buffer);
}

/*
 * File Exist
 */
RwBool
_rtFSFileExists( RtFileSystem *fs, const RwChar *fileName )
{
    RtFile   *file;
    RwUInt32  flags = 0;

    RWFUNCTION(RWSTRING("_rtFSFileExists"));

    flags |= RTFILE_ACCESS_OPEN_READ;

    file = (RtFile *)_rtFSManagerFOpenOnFS ((RtFileSystem *)fs,
                                            fileName,
                                            flags, NULL, NULL);

    if (file != NULL)
    {
        fs->fsFileFunc.close(file);
        RWRETURN (TRUE);
    }

    RWRETURN (FALSE);
}

/*
 * Test for EOF
 */
RwBool
_rtFSIsEOF( RtFile *file )
{
    RWFUNCTION(RWSTRING("_rtFSIsEOF"));

#if (defined(WIN32) || defined(MACOS))
    {
        RwUInt32 posLow, lenLow;
        RwInt32  posHigh, lenHigh;

        posLow  = (RwUInt32)file->position.noSupportValue.low;
        lenLow  = (RwUInt32)file->length.noSupportValue.low;
        posHigh = (RwInt32)file->position.noSupportValue.high;
        lenHigh = (RwInt32)file->length.noSupportValue.high;

        if ((posHigh == 0xffffffff) && (posLow == 0xffffffff))
        {
            RWRETURN (TRUE);
        }
        else
        {
            if ((posHigh > lenHigh) ||
               ((posHigh == lenHigh) && (posLow >= lenLow)))
            {
                RWRETURN (TRUE);
            }
            else
            {
                RWRETURN (FALSE);
            }
        }
    }

#else /* (defined(WIN32) || defined(MACOS)) */
    {
        RwInt64 position = file->position.supportValue;

        if (position == -1)
        {
            RWRETURN (TRUE);
        }
        else
        {
            RWRETURN (position >= file->length.supportValue);
        }
    }
#endif /* (defined(WIN32) || defined(MACOS)) */
}

/**
 * \ingroup rtfsyst
 * \ref RtFileSystemSetCallBack sets a specific file system callback, for a
 *      specific file system. This is determined by the callback code specified
 *      when calling this function.
 *
 * \param   fs          The file system to set the callcack to.
 * \param   cbCode      Callcack code.
 * \param   CallBack    The callback function.
 * \return  TRUE is successful, FALSE otherwise.
 *
 * \see RtFileSystemCallBackCode
 */
RwBool
RtFileSystemSetCallBack( RtFileSystem *fs,
                         RtFileSystemCallBackCode cbCode,
                         RtFileSystemCallBack CallBack)
{
    RWAPIFUNCTION(RWSTRING("RtFileSystemSetCallBack"));

    switch (cbCode)
    {
        case RTFS_CALLBACKCODE_FSREGISTER:
            fs->CallBacks.registerCallBack   = CallBack;
            break;

        case RTFS_CALLBACKCODE_FSUNREGISTER:
            fs->CallBacks.unregisterCallBack = CallBack;
            break;

        default:
            RWASSERT(FALSE);
            RWRETURN (FALSE);
            break;
    }

    RWRETURN (TRUE);
}

/**
 * \ingroup rtfsyst
 * \ref RtFileSystemGetCallBack gets a specific file system callback.
 *
 * Please refer to the \ref RtFileSystemCallBackCode documentation for more
 * information on when specific callbacks are triggered.
 *
 * \param   fs          The file system to get the callback from.
 * \param   cbCode      Callback code.
 * \return  The specific callback, or NULL if no callback is associated.
 */
RtFileSystemCallBack
RtFileSystemGetCallBack( RtFileSystem *fs, RtFileSystemCallBackCode cbCode )
{
    RWAPIFUNCTION(RWSTRING("RtFileSystemGetCallBack"));

    switch (cbCode)
    {
        case RTFS_CALLBACKCODE_FSREGISTER:
            if (fs->CallBacks.registerCallBack != NULL)
            {
                RWRETURN (fs->CallBacks.registerCallBack);
            }
            break;

        case RTFS_CALLBACKCODE_FSUNREGISTER:
            if (fs->CallBacks.unregisterCallBack != NULL)
            {
                RWRETURN (fs->CallBacks.unregisterCallBack);
            }
            break;

        default:
            RWASSERT(FALSE);
            RWRETURN (NULL);
            break;
    }

    RWRETURN (NULL);
}

/*
 * Trigger a CallBack
 */
void
_rtFileSystemTriggerCallBack( RtFileSystem *fs,
                              RtFileSystemCallBackCode cbCode )
{
    RWFUNCTION(RWSTRING("_rtFileSystemTriggerCallBack"));

    switch (cbCode)
    {
        case RTFS_CALLBACKCODE_FSREGISTER:
            if (fs->CallBacks.registerCallBack != NULL)
            {
                fs->CallBacks.registerCallBack(fs->name);
            }
            break;

        case RTFS_CALLBACKCODE_FSUNREGISTER:
            if (fs->CallBacks.unregisterCallBack != NULL)
            {
                fs->CallBacks.unregisterCallBack(fs->name);
            }
            break;

        default:
            RWASSERT(FALSE);
            break;
    }

    RWRETURNVOID ();
}

/**
 * \ingroup rtfsyst
 * \ref RtFileSystemSetDeviceName sets the device name for a specific file
 * system. Note that the file system device name is case sensitive.
 *
 * The device name is used by the file system toolkit to open files on a
 * specific device. This device name can be user-defined or in some cases
 * needs to be a valid existing name (for more information see the platform
 * specific documentation).
 *
 * \param   fs          The file system to set the device name to.
 * \param   deviceName  The device name.
 * \return  An error code or RTFS_ERROR_NOERROR if no error occured.
 */
RtFileSystemError
RtFileSystemSetDeviceName( RtFileSystem *fs, RwChar *deviceName )
{
    RWAPIFUNCTION(RWSTRING("RtFileSystemSetDeviceName"));
    RWASSERT (deviceName != NULL);

    RtFileSystemVerboseMessage("RtFileSystemSetDeviceName",
    "This function does not check the validity of the device name format");

#ifdef SKY
    RtFileSystemVerboseMessage("RtFileSystemSetDeviceName",
            "If you are using the PlayStation2 AT Mon file system you can use"
            " the RtSkyATMonSetHostIPAddress function, which will check the"
            " validity of the device name.");
#endif

    if (_rtGetFileSystemFromDeviceName(deviceName) == NULL)
    {
        if (fs->deviceName != NULL)
        {
            RwFree(fs->deviceName);
        }

        fs->deviceName = (RwChar *)RwMalloc(
                                  sizeof(RwChar) * (1 + rwstrlen(deviceName)),
                                  rwID_FILESYSTEMPLUGIN | rwMEMHINTDUR_GLOBAL);
        rwstrcpy(fs->deviceName, deviceName);
    }
    else
    {
        RWRETURN ( RTFS_ERROR_FILESYSTEMDEVICENAMEREGISTERED );
    }

    RWRETURN((RtFileSystemError)RTFS_ERROR_NOERROR);
}

/**
 * \ingroup rtfsyst
 * \ref RtFileSystemGetDeviceName gets the current device name for a specific
 *      file system.
 *
 * \param   fs          The file system to get the device name from.
 * \return  The file system device name.
 */
RwChar *
RtFileSystemGetDeviceName( RtFileSystem *fs )
{
    RWAPIFUNCTION(RWSTRING("RtFileSystemGetDeviceName"));

    RWRETURN (fs->deviceName);
}

/**
 * \ingroup rtfsyst
 * \ref RtFileSystemGetStatus gets the status of the specified file system.
 *      This function is useful if at any point you want to check the current
 *      state of the file system. This can give you information on whether the
 *      file system is ready and working properly or is having problem
 *      completing a specific task.
 * \param   fs          The file system to get the status from.
 * \return The file system status.
 *
 * \see RtFileSystemStatus
 */
RtFileSystemStatus
RtFileSystemGetStatus( RtFileSystem *fs )
{
    RWAPIFUNCTION(RWSTRING("RtFileSystemGetStatus"));

    RWRETURN (fs->status);
}

/**
 * \ingroup rtfsyst
 * \ref RtFileSystemDefaultToAsync defaults the file system to asynchronous or
 * synchronous mode.
 *
 * The file system determines whether a file should be opened in synchronous or
 * asynchronous mode, depending on the isAsync value passed to this function.
 * Initially, all operations are performed synchonously. When a call to
 * RtFileSystemDefaultToAsync is made, the default mode will be set for all
 * operations following the call. This means that all operations performed on
 * files opened before this call will still be undertaken according to the
 * previous settings, i.e. the settings valid when the particular file was
 * opened.
 *
 * \param fs        The file system to default.
 * \param isAsync   If TRUE subsequent operations on this file system will
 *                  default to asynchronous mode, if FALSE operations are
 *                  synchronous.
 */
void
RtFileSystemDefaultToAsync( RtFileSystem *fs, RwBool isAsync )
{
    RWAPIFUNCTION(RWSTRING("RtFileSystemDefaultToAsync"));
    fs->defaultToAsync = isAsync;
    RWRETURNVOID();
}

/**
 * \ingroup rtfsyst
 * \ref RtFileSystemIsDefaultedToAsync determines the mode to which this file
 * system is currently defaulted.
 *
 *
 * \param   fs  The file system.
 * \return  TRUE if fs is defaulted to asynchronous, FALSE otherwise.
 */
RwBool
RtFileSystemIsDefaultedToAsync( RtFileSystem *fs )
{
    RWAPIFUNCTION(RWSTRING("RtFileSystemIsDefaultedToAsync"));
    RWRETURN (fs->defaultToAsync);
}

/**
 * \ingroup rtfsyst
 * \ref RtFileSystemSetOutstandingOpCallback set the callback for the current
 *      outstanding asynchronous operation. Note that the operation that
 *      follows a call to RtFileSystemSetOutstandingOpCallback will set the
 *      appropriate file operation for the callback.
 *
 * \param   file            The file to set the callback on.
 * \param   CallBack        Asynchronous callback function. This
 *                          callback will only be triggered for on completion
 *                          of the current outstanding operation.
 * \param   CallBackData    Data passed to the callback function.
 */
void
RtFileSystemSetOutstandingOpCallback( RtFile *file,
                                     RtFSFileOpCallBack  CallBack,
                                     void               *CallBackData)
{
    RWAPIFUNCTION(RWSTRING("RtFileSystemSetOutstandingOpCallback"));
    RWASSERT(file);

    file->outstandingCB     = CallBack;
    file->outstandingCBData = CallBackData;

    RWRETURNVOID();

}

/****************************************************************************
 * Trigger Callback on operation completion for this file.
 */
void
rtFSTriggerFileOutstandingOpCallback( RtFile *file )
{
    RWFUNCTION(RWSTRING("rtFSTriggerFileOutstandingOpCallback"));

    if (file != NULL && file->isAsync != FALSE)
    {
        if (file->outstandingFileOp != RTFS_FILEOP_NONE &&
            file->outstandingCB     != NULL)
        {
            RwInt32 data;
            RtFSFileOperation fileOp = file->outstandingFileOp;

            /* If it is a seek pass the position as the size parameter
             * in the callback, otherwise pass the size.
             */
            if (file->outstandingFileOp == RTFS_FILEOP_SEEK)
            {
                data = RtFileSystemGetFilePosition(file);
            }
            else
            {
                data = file->outstandingSize;
            }

            file->outstandingFileOp = RTFS_FILEOP_NONE;


            /* Trigger the callback */
            file->outstandingCB( file,
                                 data,
                                 (RwUInt32)file->status,
                                 fileOp,
                                 file->outstandingCBData);
        }
        else
        {
            /*
             * No callback have been set so still set the outstanding
             * file operation to NONE
             */
            file->outstandingFileOp = RTFS_FILEOP_NONE;
        }
    }

    RWRETURNVOID();
}
