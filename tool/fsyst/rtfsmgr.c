#include "rwplcore.h"
#include "rtfsmgr.h"
#include "rpdbgerr.h"

#if (defined (_XBOX))
#include "xbox/fsystplatform.h"
#elif (defined(SKY))
#include "sky2/fsystplatform.h"
struct  SemaParam fsParams;
RwInt32 fsSema;
#elif (defined(WIN32))
#include "win/fsystplatform.h"
#elif (defined(DOLPHIN))
#include "gcn/fsystplatform.h"
#elif (defined(MACOS))
#include "mac/fsystplatform.h"
#else
#error "Undefined target platform for file system"
#endif

#include <string.h>

/**
 * \defgroup rtfsmgr RtFSMgr
 * \ingroup rtfs
 *
 * File System Manager.
 */

/**
 * \defgroup rtfsmdatatypes Data Types
 * \ingroup  rtfsmgr
 *
 * RenderWare File System Manager Data Types.
 */

/**
 * \ingroup rtfsmgr
 * \page rtfsmgroverview RtFSMgr Overview
 *
 * The RenderWare file system manager provides a generic interface
 * through which all RenderWare I/O operations are undertaken. This
 * constitutes the first layer of the file system API. It supports
 * multiple file systems, each representing one device type. In other
 * words it is designed to provide a common file transfer interface
 * for all hardware devices.<br>
 *
 * The file system manager is responsible for managing the registration
 * of new file systems, and for providing a generic file interface.
 * This generic interface is an ANSI-C compliant interface that allows
 * any RenderWare file operations to be done via the file system
 * toolkit. As an example, a successful file open operation will involve
 * looking for a registered file system, and calling the open function of the
 * file system found.
 */

/****************************************************************************
 * File System Manager
 * RW has ONE global file system manager
 ***************************************************************************/
static RtFSManager   fsm;                 /** The file system manager */
static RtFileSystem *defaultFileSystem;   /** Default file system */

/**
 * \ingroup rtfsmgr
 * \ref RtFSManagerOpen opens and initializes the file system manager.
 *
 * This function performs all necessary operations to initialize the file
 * system manager. This includes creating the file system manager, and
 * installing a generic set of ANSI-C file functions to the standard
 * RenderWare file interface. This function must be called first in order
 * for any future file I/O operation to work through the file system toolkit.
 *
 *
 * \param maxNbFS   Maximum number of file systems the manager can handle,
 *                  or RTFSMAN_UNLIMITED_NUM_FS for infinite.
 * \return          TRUE if successful, FALSE otherwise.
 */
RwBool
RtFSManagerOpen( RwInt32 maxNbFS )
{
    RWAPIFUNCTION(RWSTRING("RtFSManagerOpen"));

    RWASSERTM((fsm.isOpened == FALSE), ("The file system manager is already "
                                        "opened"));
    fsm.curNbFS = 0;
    fsm.maxNbFS = maxNbFS;

    /* Initialise CallBacks */
    fsm.CallBacks.errorCallBack  = NULL;
    defaultFileSystem            = NULL;

    rwSingleListInitialize(&fsm.fs);

    RTFS_INITIALIZE_INTERRUPT_HANDLING;

    fsm.isOpened = TRUE;

	//@{ 20050513 DDonSS : Threadsafe
	// Initialize Critical Section
	CS_FSMANAGER_INITLOCK( &fsm );
	//@} DDonSS

    RtFileSystemVerboseMessage("RtFSManagerOpen", "File System Manager "
                                "Successfully Initialised");
    RWRETURN (TRUE);
}

/*
 * Display whether there are any open files on any registered file system
 */
static void
TkFSManagerDisplayInfo( void )
{
    RtFileSystem *fs = (RtFileSystem *)rwSingleListGetFirstSLLink(&fsm.fs);

    RWFUNCTION(RWSTRING("TkFSManagerDisplayInfo"));

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe 코드 추가하지 않음. Local Function이며, 호출하는 곳에서 처리
	// 호출하는 함수 : RtFSManagerClose
	//@} DDonSS

    RtFileSystemVerboseMessage("TkFSManagerDisplayInfo", " -------------------"
                                         "-----------------------------------");

    while (fs != (RtFileSystem *)rwSingleListGetTerminator(&fsm.fs))
    {
        RwInt32 i;
        RwInt32 nbOpenFiles = 0;

        for (i = 0; i < fs->maxNbOpenFiles; ++i)
        {
            RtFile *file = fs->fsGetObject(fs, i);
            if(file->status != RTFILE_STATUS_CLOSED)
            {
                ++nbOpenFiles;
            }
        }

        {
            RwChar msg[1024];
            rwsprintf(msg, "Number of opened files on %s (device name: %s):"
                                 " %d", fs->name, fs->deviceName, nbOpenFiles);
            RtFileSystemVerboseMessage("TkFSManagerDisplayInfo", msg);
        }

        fs = (RtFileSystem *)fs->next;
    }

    RtFileSystemVerboseMessage("TkFSManagerDisplayInfo", " -------------------"
                                         "-----------------------------------");
    RWRETURNVOID();
}

/*
 * Cast a RtInt64
 */
RwUInt32
TkFSManagerGetValue(RtInt64 value)
{
    RWFUNCTION(RWSTRING("TkFSManagerGetValue"));

#if (defined(WIN32) || defined(MACOS))
    RWRETURN((RwUInt32)value.noSupportValue.low);
#else /* (defined(WIN32) || defined(MACOS)) */
    RWRETURN((RwUInt32)value.supportValue);
#endif /* (defined(WIN32) || defined(MACOS)) */

}

/**
 * \ingroup rtfsmgr
 * \ref RtFSManagerClose closes the file system manager and frees all
 *      registered file systems.
 * This function must be called when the application is about to be closed.
 */
void
RtFSManagerClose( void )
{
    RWAPIFUNCTION(RWSTRING("RtFSManagerClose"));
    
    if (fsm.isOpened == TRUE)
    {
		//@{ 20050513 DDonSS : Threadsafe
		// Lock 처리를 위해 변수 선언부 이동
        RtFileSystem *head = NULL;
        RtFileSystem *next, *fs;
        
		// fsm Lock
		CS_FSMANAGER_LOCK( &fsm );
		//@} DDonSS
	
        head = (RtFileSystem *)rwSingleListGetFirstSLLink(&fsm.fs);

#if defined(RWDEBUG)
        TkFSManagerDisplayInfo();
#endif

        /* Now Remove all filesystems and free allocated memory */
        for (fs = head; fs != (RtFileSystem *)rwSingleListGetTerminator(&fsm.fs);
             fs = next)
        {
            next = (RtFileSystem *)fs->next;
            fs->fsClose(fs);
            RwFree(fs->deviceName);
            fs->deviceName = (RwChar *)NULL;

            _rtFileSystemTriggerCallBack(fs, RTFS_CALLBACKCODE_FSUNREGISTER);

            RwFree(fs);
        }

        RtFileSystemVerboseMessage("RtFSManagerClose", "File System Manager "
                                   "Successfully Closed");

        RTFS_SHUTDOWN_INTERRUPT_HANDLING;

        fsm.isOpened = FALSE;
        fsm.curNbFS  = 0;

		//@{ 20050513 DDonSS : Threadsafe
		// fsm Unlock
		CS_FSMANAGER_UNLOCK( &fsm );

		// Delete Lock
		CS_FSMANAGER_DELLOCK( &fsm );
		//@} DDonSS
    }
    else
    {
        RtFileSystemVerboseMessage("RtFSManagerClose", "File System Manager "
                                   "is Already Closed");
    }
    RWRETURNVOID();
}

/*
 * Trigger the file system manager error CallBack...
 * (is that really useful?)
 */
static RwBool
TkFSManagerTriggerErrorCallBack( RtFSManagerError err )
{
    RWFUNCTION(RWSTRING("TkFSManagerTriggerErrorCallBack"));

    if (fsm.CallBacks.errorCallBack != NULL)
    {
        fsm.CallBacks.errorCallBack(err);
        RWRETURN (TRUE);
    }

    RWRETURN (FALSE);
}

/**
 * \ingroup rtfsmgr
 * \ref RtFSManagerSetCallBack sets a specific callback for the file system
 *      manager. This specific callback is determined by the callback code
 *      passed.
 *
 * \param   cbCode      Callback code.
 * \param   CallBack    Callback function.
 * \return  TRUE if sucessful, FALSE otherwise.
 *
 * \see RtFSManagerCallBackCode
 */
RwBool
RtFSManagerSetCallBack( RtFSManagerCallBackCode cbCode,
                        RtFSManagerCallBack CallBack )
{
    RWAPIFUNCTION(RWSTRING("RtFSManagerSetCallBack"));

    switch (cbCode)
    {
        case RTFSM_CALLBACKCODE_ERROR:
            fsm.CallBacks.errorCallBack = (RtFSManagerCallBack)CallBack;
            break;

        default:
            RWASSERT(FALSE);
            RWRETURN (FALSE);
            break;
    }

    RWRETURN (TRUE);
}

/*
 * Can we register one more file system?
 */
RwBool
_rtFSManagerIsNewFSRegistrable( void )
{
    RWFUNCTION(RWSTRING("_rtFSManagerIsNewFSRegistrable"));

	//@{ 20050513 DDonSS : Threadsafe
	// Threadsafe 코드 추가하지 않음. Local Function이며, 호출하는 곳에서 처리
	// 호출하는 함수 : RtFSManagerRegister
	//@} DDonSS

    if (fsm.maxNbFS > 0)
    {
        if (fsm.curNbFS + 1 > fsm.maxNbFS)
        {
            TkFSManagerTriggerErrorCallBack((RtFSManagerError)
                                            RTFSM_ERROR_NOTREGISTRABLE);

            RtFileSystemVerboseMessage(
            "_rtFSManagerIsNewFSRegistrable", "Can't"
            " Register New File System: Maximum File System Manager Capacity "
            " Reached");
            RWRETURN (FALSE);
        }
    }

    RWRETURN (TRUE);
}

/**
 * \ingroup rtfsmgr
 * \ref RtFSManagerRegister registers a new file system with the file system
 * manager.
 *
 * This function will perform some initial steps before actually registering
 * the file system specified. It will first check whether the file system can
 * be registered. This is done by checking whether the file system manager has
 * reached its maximum capacity (which is specified during
 * \ref RtFSManagerOpen). If the maximum capacity has been reached, this
 * function will return FALSE. However in the case where the manager can handle
 * the new file system, this function will verify whether the unique name of
 * the file system passed is not already in use. If that verification is
 * successful, the function returns TRUE. In all other cases, it will return
 * FALSE.
 *
 * \param   fs          Pointer to the file system to register.
 * \return  TRUE if sucessful, FALSE otherwise.
 */
RwBool
RtFSManagerRegister( RtFileSystem *fs )
{
#ifdef RWDEBUG
    RwChar msg[100];
#endif

    RWAPIFUNCTION(RWSTRING("RtFSManagerRegister"));
    RWASSERT(fs != NULL);
    RWASSERT(rwstrlen(fs->name) < RTFS_MAX_NAME_LENGTH);
    
	//@{ 20050513 DDonSS : Threadsafe
	// fsm Lock
	CS_FSMANAGER_LOCK( &fsm );
	//@} DDonSS
	
    /* Normally if we reach this point the new file system can be registered
     * i.e. the FSManager can either support an unlimited number of FS,
     * or it still has some room for this new FS
     * However do not assume that and check again that we can register this
     * file system */
    if (_rtFSManagerIsNewFSRegistrable() != FALSE)
    {
        /* Check if a file system with this name is already registered */
        if (RtFSManagerGetFileSystemFromName (fs->name) == NULL)
        {
            _rtFileSystemTriggerCallBack(fs, RTFS_CALLBACKCODE_FSREGISTER);

            /* Add this file system to the list of file system managed by this
             * manager
             */
            rwSingleListAddSLLink(&fsm.fs, (RwSLLink *)fs);
            /* Increment the current number of file system */
            ++fsm.curNbFS;

			//@{ 20050513 DDonSS : Threadsafe
			// fsm Unlock
			CS_FSMANAGER_UNLOCK( &fsm );
			//@} DDonSS

#ifdef RWDEBUG
            rwsprintf(msg, "FileSystem %s Successfully Registered", fs->name);
            RtFileSystemVerboseMessage("RtFSManagerRegister", msg);
#endif
            RWRETURN(TRUE);
        }
        else
        {
			//@{ 20050513 DDonSS : Threadsafe
			// fsm Unlock
			CS_FSMANAGER_UNLOCK( &fsm );
			//@} DDonSS
        	
#ifdef RWDEBUG
            rwsprintf(msg, "FileSystem %s Failed to Register (a file system "
            "with the same name is already registered)", fs->name);
            RtFileSystemVerboseMessage("RtFSManagerRegister", msg);
#endif
            RWRETURN(FALSE);
        }
    }
    else
    {
		//@{ 20050513 DDonSS : Threadsafe
		// fsm Unlock
		CS_FSMANAGER_UNLOCK( &fsm );
		//@} DDonSS
		
        RWRETURN (FALSE);
    }
}

/**
 * \ingroup rtfsmgr
 * \ref RtFSManagerUnregister unregisters a specific file system from the
 * file system manager.
 *
 * This function checks that the file system passed is currently
 * registered and, if it is, removes it from the list of file systems being
 * managed by the file system manager. This will also involve freeing all
 * memory associated with that file system. Should the file system specified be
 * the default file system, this function will also reset the default file
 * system to NULL. <br><br>
 *
 * Note that if any files remain open when this function is called they will
 * not be closed. In other words this function should not be called if the file
 * system is still being used by any files. Should this be done it will result
 * in an unpreditable behaviour, which will probably cause your program to
 * crash.
 *
 * \param   fs   Pointer to the file system to unregister.
 */
void
RtFSManagerUnregister( RtFileSystem *fs )
{
	//@{ 20050513 DDonSS : Threadsafe
	// Lock 처리를 위해 변수 선언부 위로 이동..
	RtFileSystem *prev  = NULL;
	RtFileSystem *fsys  = NULL;
	
    RWAPIFUNCTION(RWSTRING("RtFSManagerUnregister"));

	// fsm Lock
	CS_FSMANAGER_LOCK( &fsm );
	//@} DDonSS

    prev  = (RtFileSystem *)rwSingleListGetFirstSLLink(&fsm.fs);
    fsys  = (RtFileSystem *)prev->next;

    if (defaultFileSystem == fs)
    {
        defaultFileSystem = NULL;
    }

    /* Just check if this file system has been registered before */
    if (RtFSManagerGetFileSystemFromName (fs->name) != NULL)
    {
        _rtFileSystemTriggerCallBack(fs, RTFS_CALLBACKCODE_FSUNREGISTER);

        /* fs is the first element of the list */
        if (prev == fs)
        {
            fsm.fs.link.next = prev->next;
            RwFree(fs->deviceName);
            fs->fsClose(fs);
            RwFree(fs);
            --fsm.curNbFS;
            
			//@{ 20050513 DDonSS : Threadsafe
			// fsm Unlock
			CS_FSMANAGER_UNLOCK( &fsm );
			//@} DDonSS
            
            RWRETURNVOID();
        }

        while (fsys != (RtFileSystem *)rwSingleListGetTerminator(&fsm.fs))
        {
            if (fsys == fs)
            {
                prev->next = fsys->next;
                /* Perform some file system specific closing operations */
                RwFree(fs->deviceName);
                fs->fsClose(fs);
                RwFree(fs);
                break;
            }
            fsys = (RtFileSystem *)fsys->next;
            prev = (RtFileSystem *)prev->next;
        }

        /* fs is the last element of the list and there is not further
         * elements
         */
        if (prev == fs)
        {
            RwFree(fs->deviceName);
            fs->fsClose(fs);
            RwFree(fs);
        }

        /* Decrement the current number of file system */
        --fsm.curNbFS;
    }
    
	//@{ 20050513 DDonSS : Threadsafe
	// fsm Unlock
	CS_FSMANAGER_UNLOCK( &fsm );
	//@} DDonSS

    RWRETURNVOID();
}

/**
 * \ingroup rtfsmgr
 * \ref RtFSManagerGetFileSystemFromName gets a file system from its unique
 *      name.
 *
 * This function will return a pointer to the registered file system which has
 * the unique name fsName. Note that fsName is user defined, and corresponds to
 * the fileSystemName passed during the OS specific file system initialization.
 * If no file system is found, the function will trigger the error callback
 * with an RTFSM_ERROR_NOFS error, should an error callback be specified.
 *
 * \param fsName    Name of the file system to retrieve.
 * \return          A pointer to the corresponding file system or NULL if no
 *                  file system with the specified name has been registered.
 */
RtFileSystem *
RtFSManagerGetFileSystemFromName( RwChar *fsName )
{
	//@{ 20050513 DDonSS : Threadsafe
	// Lock 처리를 위해 변수 선언부 위로 이동..
    RtFileSystem *fs = NULL;
    
    RWAPIFUNCTION(RWSTRING("RtFSManagerGetFileSystemFromName"));

	// fsm Lock    
	CS_FSMANAGER_LOCK( &fsm );
	//@} DDonSS
	
    fs = (RtFileSystem *)rwSingleListGetFirstSLLink(&fsm.fs);

    while (fs != (RtFileSystem *)rwSingleListGetTerminator(&fsm->fs))
    {
        if (rwstrcmp(fs->name, fsName) == 0)
        {
			//@{ 20050513 DDonSS : Threadsafe
			// fsm Unlock
			CS_FSMANAGER_UNLOCK( &fsm );
			//@} DDonSS
			
            RWRETURN (fs);
        }
        fs = (RtFileSystem *)fs->next;
    }

	//@{ 20050513 DDonSS : Threadsafe
	// fsm Unlock
	CS_FSMANAGER_UNLOCK( &fsm );
	//@} DDonSS

    TkFSManagerTriggerErrorCallBack((RtFSManagerError)RTFSM_ERROR_NOFS);

    RWRETURN (NULL);
}

/*
 * This function checks whether the file system name passed
 * is already in use by another registered file system.
 */
RwBool
_rtFSManagerIsFSNameInUse( RwChar *fsName )
{
    RtFileSystem *fs = RtFSManagerGetFileSystemFromName(fsName);

    RWFUNCTION(RWSTRING("_rtFSManagerIsFSNameInUse"));

    if (fs != NULL)
    {
        TkFSManagerTriggerErrorCallBack((RtFSManagerError)
                                        RTFSM_ERROR_FSNAMEINUSE);

        RWRETURN (TRUE);
    }
    else
    {
        RWRETURN (FALSE);
    }
}

/*
 * This function open a file on a specific file system.
 */
RtFile *
_rtFSManagerFOpenOnFS( RtFileSystem *fs,
                       const RwChar *fileName,
                       RtFileAccessFlag access,
                       RtFSFileOpCallBack  CallBack,
                       void               *CallBackData )
{
    RtFile       *file  = NULL;
    RwInt32       index = 0;

    RWFUNCTION(RWSTRING("_rtFSManagerFOpenOnFS"));

    if (fs != NULL)
    {
        /* Find a spare slot in this filesystem */
        /* Handle allocation is thread safe */
        RTFS_DISABLE_INTERRUPT;

        for (index = 0; index < fs->maxNbOpenFiles; ++index)
        {
            RtFile *nextFile = fs->fsGetObject(fs, index);

            if (nextFile->status == RTFILE_STATUS_CLOSED)
            {
                /* Found one return.
                 * Also mark the status as "opening" to prevent other threads
                 * from getting access to the same handle.
                 */
                nextFile->status = RTFILE_STATUS_OPENING;
                file = nextFile;
                break;
            }
        }

        /* Enable interrupts again */
        RTFS_ENABLE_INTERRUPT;

        if (file == NULL)
        {
            LASTERROR = RTFSM_ERROR_TOOMANYOPENFILES;
            TkFSManagerTriggerErrorCallBack((RtFSManagerError)
                                            RTFSM_ERROR_TOOMANYOPENFILES );

            RWRETURN (NULL);
        }
        else
        {
            RtFileSystemError fsErr;

            RtFileSystemVerboseMessage("_rtFSManagerFOpenOnFS", fileName);

            if (access & RTFILE_ACCESS_OPEN_ASYNC || file->isAsync != FALSE)
            {
                file->outstandingFileOp  = RTFS_FILEOP_OPEN;
                file->outstandingCB      = CallBack;
                file->outstandingCBData  = CallBackData;
            }

            /* Here set the ASYNC flag depending on user settings */
            if (fs->defaultToAsync != FALSE)
            {
                access |= RTFILE_ACCESS_OPEN_ASYNC;
                file->outstandingFileOp  = RTFS_FILEOP_OPEN;
                file->outstandingCB      = CallBack;
                file->outstandingCBData  = CallBackData;
            }

            /* File system specific open */
            fsErr = fs->fsFileFunc.open(fs, file, fileName, access);

            if (fsErr != RTFS_ERROR_NOERROR)
            {
                file->status = RTFILE_STATUS_CLOSED;
                file = NULL;
            }
        }

        if (file != NULL)
        {
            file->priority = RTFS_DEFAULTFILEPRIORITY;
        }

        RWRETURN (file);
    }
    else
    {
        TkFSManagerTriggerErrorCallBack((RtFSManagerError)
                                            RTFSM_ERROR_INVALIDFS);
        RWRETURN (NULL);
    }
}

/**
 * \ingroup rtfsmgr
 * \ref RtFSManagerFOpen opens a file in a file system that is determined by
 *      the device name attached to the file name. <br><br>
 *
 * This function will first try to get a file system device name from the file
 * name specified. In the case where a corresponding file system is found (or
 * a default file system has been registered), this function will look whether
 * there are any available files in this file system. Remember that the maximum
 * number of files a file system can concurrently handle is set during the file
 * system initialization. If the file system still has any available file slots
 * this function will then attempt to open the file.<br>
 *
 * This function can be used directly to open a file when a file
 * needs to be opened in asynchronous mode. In this case, you can use the
 * \ref RtFileAccessFlag enumeration to specify the correct file
 * access. You can mix these flags using the binary OR operator as shown below:
 * <br>
 *
 * RtFileAccessFlag flags = 0;<br>
 * flags |= RTFILE_ACCESS_OPEN_READ; <br>
 * flags |= RTFILE_ACCESS_OPEN_ASYNC;<br>
 * <br>
 *
 * To use this function, make sure that you have registered at least
 * one file system. The function will automatically attempt to look
 * for a file system from the file name so make sure that either your file
 * names include the device name, or that you have registered a default file
 * system (see \ref RtFSManagerSetDefaultFileSystem).
 *
 * Note that it is generally common place to open a separate thread and use the
 * file system in synchronous mode. This function should therefore be used only
 * in the case where a file needs to be opened directly in asynchronous mode,
 * and when the use of the internal asynchronous I/O mode is desired.
 *
 * \param filename          Name of the file to open.
 * \param access            Open access flags.
 * \param   CallBack        Asynchronous callback function. This
 *                          callback will only be triggered for on completion
 *                          of the current outstanding operation.
 * \param   CallBackData    Data passed to the callback function.

 * \return  NULL if failed, a pointer to a RtFile object if successful.
 */
RtFile *
RtFSManagerFOpen( const RwChar       *filename,
                  RtFileAccessFlag    access,
                  RtFSFileOpCallBack  CallBack,
                  void               *CallBackData )
{
    RtFileSystem *fs;

    RWAPIFUNCTION(RWSTRING("RtFSManagerFOpen"));
    RWASSERTM(fsm.isOpened == TRUE, ("The file system manager hasn't been"
                                     " initialised."));
    RWASSERTM(fsm.curNbFS > 0, ("There are no file system registered."));

    fs = RtFSManagerGetFileSystemFromFileName(filename);

    if (fs != NULL)
    {
        RWRETURN (_rtFSManagerFOpenOnFS(fs, filename, access, CallBack,
                                        CallBackData));
    }

    RWRETURN (NULL);
}

/*
 * Return the file system manager.
 */
RtFSManager *
_rtGetFSManager( void )
{
    RWFUNCTION(RWSTRING("_rtGetFSManager"));
    RWRETURN (&fsm);
}

/*
 * Gets a registered file system its device name.
 */
RtFileSystem *
_rtGetFileSystemFromDeviceName( const RwChar *deviceName )
{
	//@{ 20050513 DDonSS : Threadsafe
	// Lock 처리를 위해 변수 선언부 위로 이동..
    RtFileSystem *fs = NULL;

    RWFUNCTION(RWSTRING("_rtGetFileSystemFromDeviceName"));
    
	// fsm Lock
	CS_FSMANAGER_LOCK( &fsm );
	//@} DDonSS
    
    fs = (RtFileSystem *)rwSingleListGetFirstSLLink(&fsm.fs);

    while (fs != (RtFileSystem *)rwSingleListGetTerminator(&fsm.fs))
    {
        if (rwstricmp(deviceName, fs->deviceName) == 0)
        {
			//@{ 20050513 DDonSS : Threadsafe
			// fsm Unlock
			CS_FSMANAGER_UNLOCK( &fsm );
			//@} DDonSS
			
            RWRETURN (fs);
        }

        fs = (RtFileSystem *)fs->next;
    }

	//@{ 20050513 DDonSS : Threadsafe
	// fsm Unlock
	CS_FSMANAGER_UNLOCK( &fsm );
	//@} DDonSS

    RWRETURN (NULL);
}

/**
 * \ingroup rtfsmgr
 * \ref RtFSManagerGetFileSystemFromFileName Gets a registered file system from
 *      a complete file name, i.e. a file name including the device name.
 *
 * This function will look for the colon character (":") as a device separator.
 * Also note that this function will automatically return the default file
 * system (should one have been registered) in the case where it fails to
 * find the correct file system as specified by the device name appended
 * to the file name passed.
 *
 * \param fileName  Full name of the file from which to retrieve the file
 *                  system.
 * \return          A pointer to the corresponding file system or NULL if no
 *                  corresponding file system has been found.
 */
RtFileSystem *
RtFSManagerGetFileSystemFromFileName( const RwChar *fileName )
{
    RwUInt32        nIndex;
    RwChar          deviceName[100];
    RtFileSystem *fs;

    RWAPIFUNCTION(RWSTRING("RtFSManagerGetFileSystemFromFileName"));

	//@{ 20050513 DDonSS : Threadsafe
	// fsm Lock
	CS_FSMANAGER_LOCK( &fsm );
	//@} DDonSS
	
    if (fsm.curNbFS != 0)
    {
        RwUInt32 len = rwstrlen(fileName);
        
        /* Check for a colon */
        for (nIndex = 0; nIndex < len; ++nIndex)
        {
            if (fileName[nIndex] == ':')
            {
                strncpy(deviceName, fileName, nIndex + 1);
                deviceName[nIndex + 1] = '\0';

                if ((fs = _rtGetFileSystemFromDeviceName(deviceName)) != NULL)
                {
					//@{ 20050513 DDonSS : Threadsafe
					// fsm Unlock
					CS_FSMANAGER_UNLOCK( &fsm );
					//@} DDonSS
					
                    RWRETURN (fs);
                }
            }
        }
        /*
         * At this point we haven't found a file system that hooks up to the
         * device name attached to this file name. Let try to see if there is
         * a default file system
         */
        if (defaultFileSystem != NULL)
        {
			//@{ 20050513 DDonSS : Threadsafe
			// fsm Unlock
			CS_FSMANAGER_UNLOCK( &fsm );
			//@} DDonSS

            RWRETURN(defaultFileSystem);
        }
    }

	//@{ 20050513 DDonSS : Threadsafe
	// fsm Unlock
	CS_FSMANAGER_UNLOCK( &fsm );
	//@} DDonSS

    /* No file system found */
    TkFSManagerTriggerErrorCallBack((RtFSManagerError)RTFSM_ERROR_NOFS);
    RWRETURN (NULL);
}

/**
 * \ingroup rtfsmgr
 * \ref RtFSManagerSetDefaultFileSystem sets a default file system.
 *
 * The default file system is the file system that will be used as a last
 * resort when trying to open a file. This means that when a file open
 * operation is undertaken the manager will automatically try to use the
 * default file system if no other valid file system, i.e. a file system
 * corresponding to a possible device name specified in front of the file
 * name to open, has been found. <br><br>
 *
 * The file system manager allows you to set a default file system. This
 * is particularly useful if your path names are relative or if you want to
 * default any file I/O operations to a specific file system. Remember that
 * when a file is opened, through the file system toolkit, a corresponding file
 * system must be found. This means that if your paths are relative, they may
 * not include any device name, and therefore a default file system must be
 * registered in order for the open operation to be attempted.
 *
 * \param	fs	A pointer to the file system that will become the default file
 *              system
 */
void
RtFSManagerSetDefaultFileSystem( RtFileSystem *fs )
{
    RWAPIFUNCTION(RWSTRING("RtFSManagerSetDefaultFileSystem"));
    defaultFileSystem = fs;
    RWRETURNVOID();
}

/**
 * \ingroup rtfsmgr
 * \ref RtFSManagerGetDefaultFileSystem gets the current default file system
 *
 * \return The default file system or NULL if there is no default file system
 * set.
 */
RtFileSystem *
RtFSManagerGetDefaultFileSystem( void )
{
    RWAPIFUNCTION(RWSTRING("RtFSManagerGetDefaultFileSystem"));
    RWRETURN(defaultFileSystem);
}

/****************************************************************************
 * Generic RW File Function definitions
 */

/**
 * \ingroup rtfsmgr
 * \ref RwFopen opens a file. This is an ANSI-C compliant function allowing the
 *              use of standard file access flags, i.e. valid combinations of
 *              'r', 'w', 'b', and 'a'. This function calls
 *              \ref RtFSManagerFOpen.
 *
 * \param	name            Name of the file.
 * \param   access 	        File access flags.
 * \return  A pointer to the file opened or NULL if the file couldn't be
 *          opened.
 *
 * \ref RtFSManagerFOpen
 */
void *
RwFopen( const RwChar       *name,
         const RwChar       *access)
{
    RtFile  *file;
    RtFileAccessFlag flags = 0;
    RwBool plus = FALSE;
    RwBool bin  = FALSE;

    RWAPIFUNCTION(RWSTRING("RwFopen"));

    if (strstr(access, RWSTRING("+")))
    {
        plus = TRUE;
    }
    
    if (strstr(access, RWSTRING("b")))
    {
        bin = TRUE;
    }

    if (strstr(access, RWSTRING("r")))
    {
        flags |= RTFILE_ACCESS_OPEN_READ;

        if (plus)
        {
            flags |= RTFILE_ACCESS_OPEN_WRITE;
            flags |= RTFILE_ACCESS_OPEN_APPEND;
        }
    }

    if (strstr(access, RWSTRING("w")))
    {
        flags |= RTFILE_ACCESS_OPEN_WRITE;
        flags |= RTFILE_ACCESS_OPEN_CREATE;

        if (plus)
        {
            flags |= RTFILE_ACCESS_OPEN_READ;
        }

    }

    if (strstr(access, RWSTRING("a")))
    {
        flags |= RTFILE_ACCESS_OPEN_WRITE;
        flags |= RTFILE_ACCESS_OPEN_APPEND;

        if (plus)
        {
            flags |= RTFILE_ACCESS_OPEN_READ;
        }
        
        if (bin)
        {
            flags |= RTFILE_ACCESS_OPEN_CREATE;
        }
    }

    /*
     * The asynchronous flag is set just before the actual
     * opening of the file in RtFSManagerFOpen, because the
     * file system allows each individual file system implementation
     * to be defaulted (or not) to asynchronous
     */

    /* Try and open the file */
    file = RtFSManagerFOpen(name, flags, NULL, NULL);

    RWRETURN (file);
}

/**
 * \ingroup rtfsmgr
 * \ref RwFclose closes the file associated with the file pointer fptr
 *      (ANSI-C compliant).
 *
 * \param	fptr    File pointer corresponding to the file to close.
 * \return  This function will return 0.
 */
RwInt32
RwFclose(void *fptr)
{
    RtFile *file     = (RtFile *)fptr;
    RtFileSystemFileFunctionTable *fileFuncs = &file->fileSystem->fsFileFunc;

    RWAPIFUNCTION(RWSTRING("RwFclose"));

    RTFS_DISABLE_INTERRUPT;

    file->status = RTFILE_STATUS_CLOSED;
    fileFuncs->close(file);

    RTFS_ENABLE_INTERRUPT;
    RtFileSystemVerboseMessage("RwFclose", "Closing file");

    RWRETURN (0);
}

/**
 * \ingroup rtfsmgr
 * \ref RwFread reads "count" objects each of size "size" from the file
 *      associated with the file pointer "fptr", and stores the data read
 *      at the location given by "addr" (ANSI-C compliant).
 *
 * \param	addr    Buffer into which the data read will be stored.
 * \param	size    Number of bytes to read.
 * \param	count   Number of objects of size "size" to read.
 * \param	fptr    File pointer corresponding to the file to read from.
 *
 * \return  The number of object(s) read.
 */
size_t
RwFread( void *addr,
         size_t size,
         size_t count,
         void *fptr)
{
    RtFile       *file  = (RtFile *)fptr;
    RtFileSystemFileFunctionTable *fileFuncs = &file->fileSystem->fsFileFunc;
    RwUInt32    bytesRead;

    RWAPIFUNCTION(RWSTRING("RwFread"));

    /* 
     * The file outstanding operation is set in the fileFuncs read function
     * so that direct calls to the read function result in correct 
     * synchronisation of files opened asynchronously.
     */
    bytesRead = fileFuncs->read(file, addr, size * count);

    RWRETURN (bytesRead / size);
}

/**
 * \ingroup rtfsmgr
 * \ref RwFwrite writes "count" objects (obtained from the location given
 *      by the pointer "addr") each of size "size" from the file
 *      associated with the file pointer "fptr" (ANSI-C compliant).
 *
 * \param	addr    Buffer from which the data written is taken.
 * \param	size    Number of bytes to write.
 * \param	count   Number of objects of size "size" to write.
 * \param	fptr    File pointer corresponding to the file to write to.
 *
 * \return  The number of object(s) written.
 */
size_t
RwFwrite( const void *addr,
          size_t size,
          size_t count,
          void *fptr)
{
    RtFile       *file   = (RtFile *)fptr;
    RtFileSystemFileFunctionTable *fileFuncs = &file->fileSystem->fsFileFunc;
    RwUInt32      bytesWritten;

    RWAPIFUNCTION(RWSTRING("RwFwrite"));

    bytesWritten  = fileFuncs->write(file, addr, size * count);

    RWRETURN (bytesWritten / size);
}

/**
 * \ingroup rtfsmgr
 * \ref RwFseek sets the file position to the appropriate offset and from a
 *              specific origin (ANSI-C compliant). The new position (in bytes)
 *              is obtained by adding the offset "offset" to the position
 *              specified by "origin".
 *
 * \param	fptr    File pointer corresponding to the file to set the position
 *                  to.
 * \param	offset  Number of bytes corresponding to the new position within
 *                  the file.
 * \param	origin  Position form which the offset will be applied.
 *
 * \return  0 if successful, -1 otherwise.
 */
RwInt32
RwFseek( void    *fptr,
         long     offset,
         RwInt32  origin)
{
    RtFile *file     = (RtFile *)fptr;
    RtFileSystemFileFunctionTable *fileFuncs = &file->fileSystem->fsFileFunc;

    RWAPIFUNCTION(RWSTRING("RwFseek"));

    if(file->isAsync != FALSE)
    {
        file->outstandingFileOp = RTFS_FILEOP_SEEK;
        file->status = RTFILE_STATUS_BUSY;
    }

    switch (origin)
    {
        case SEEK_CUR:
        {
            fileFuncs->setPosition(file, offset, RTFILE_POS_CURRENT);
        }
        break;

        case SEEK_END:
        {
            fileFuncs->setPosition(file, offset, RTFILE_POS_END);
        }
        break;

        case SEEK_SET:
        {
            fileFuncs->setPosition(file, offset, RTFILE_POS_BEGIN);
        }
        break;

        default:
        {
            RWRETURN (-1);
        }
        break;
    }

    RWRETURN (0);
}

/**
 * \ingroup rtfsmgr
 * \ref RwFgets reads at most (maxLen - 1) characters from the file specified by
 *              the file pointer "fptr", and stores them in the string
 *              "buffer" (ANSI-C compliant).
 *
 * \param	buffer  String holding the characters read.
 * \param	maxLen  Maximum number of characters to read.
 * \param	fptr    File pointer corresponding to the file to read.
 *
 * \return  A pointer to the string read if successful, 0 otherwise.
 */
RwChar *
RwFgets( RwChar   *buffer,
         RwInt32   maxLen,
         void     *fptr)
{
    RtFile    *fp = (RtFile *) fptr;
    RwInt32   i;
    RwInt32   numBytesRead;

    RWAPIFUNCTION(RWSTRING("RwFgets"));
    RWASSERT(buffer);
    RWASSERT(fptr);

    i = 0;

    numBytesRead = RwFread(buffer, 1, maxLen - 1, fp);

    if (numBytesRead == 0)
    {
        RWRETURN(0);
    }

    while (i < numBytesRead)
    {
        if (buffer[i] == '\n')
        {
            ++i;
            buffer[i] = '\0';

            /*
             * The file pointer needs to be reset as RwFSFread
             * will have overshot the first new line
             */
            i -= numBytesRead;
            RwFseek(fp, i, SEEK_CUR);

            RWRETURN (buffer);
        }
        else if (buffer[i] == 0x0D)
        {
            if ((i < (numBytesRead - 1)) && (buffer[i + 1] == '\n'))
            {
                memcpy(&buffer[i], &buffer[i + 1],
                       (numBytesRead - i - 1));
                numBytesRead--;
            }
            else
            {
                ++i;
            }
        }
        else
        {
            ++i;
        }
    }

    /*
     * We didn't find a \n. Buffer must always end with NULL.
     */
    buffer[numBytesRead] = '\0';

    RWRETURN (buffer);
}

/**
 * \ingroup rtfsmgr
 * \ref RwFputs writes the string pointed by "buffer" to the file specified by
 *              the file pointer "fptr" (ANSI-C compliant).
 *
 * \param	buffer  String holding the characters to write.
 * \param	fptr    File pointer corresponding to the file to write to.
 *
 * \return  The total number of bytes written if successful, 0 otherwise.
 */
RwInt32
RwFputs( const RwChar * buffer, void *fptr )
{
    RtFile *fp = (RtFile *) fptr;
    RwInt32 len = 0;
    const RwChar *ptr = buffer;

    RWAPIFUNCTION(RWSTRING("RwFputs"));

    while (*ptr != '\0')
    {
        ++ptr;
        ++len;
    }

    if (!len)
    {
        RWRETURN(0);
    }

    RWRETURN(RwFwrite(buffer, 1, len, fp));
}

/**
 * \ingroup rtfsmgr
 * \ref RwFeof tests the end-of-file (EOF) indicator for the file specified by
 *             the file pointer "fptr" (ANSI-C compliant).
 *
 * \param	fptr    File pointer corresponding to the file to test for EOF.
 *
 * \return  1 if EOF, 0 otherwise.
 */
RwInt32
RwFeof( void *fptr )
{
    RtFile *file     = (RtFile *)fptr;

    RWAPIFUNCTION(RWSTRING("RwFeof"));

    RWRETURN (file->fileSystem->fsFileFunc.isEOF(file));
}

/**
 * \ingroup rtfsmgr
 * \ref RwFexist checks whether a file specified by its name exists.
 *
 * \param	name    Name of the file.
 *
 * \return  TRUE if the file exists, FALSE otherwise.
 */
RwBool
RwFexist( const RwChar * name )
{
    RtFileSystem *fs;

    RWAPIFUNCTION(RWSTRING("RwFexist"));

    RWASSERTM(fsm.isOpened == TRUE, ("The file system manager hasn't been"
                                     " initialised."));
    RWASSERTM(fsm.curNbFS > 0, ("There are no file system registered."));

    /* Try and get a device name glued to the filename */
    fs = RtFSManagerGetFileSystemFromFileName(name);

    if (fs == NULL)
    {
        RWRETURN (FALSE);
    }
    RWRETURN (fs->fsFileFunc.fExists(fs, name));

}

/**
 * \ingroup rtfsmgr
 * \ref RwFtell returns the current position of the file specified by file
 *              pointer "fptr".
 *
 * \param	fptr  File to get the position from.
 *
 * \return  Current position of the file.
 */
RwInt32
RwFtell( void *fptr )
{
    RtFile *file = (RtFile *)fptr;
    RwInt32 pos;

    RWAPIFUNCTION(RWSTRING("RwFtell"));

#if (defined(WIN32) || defined(MACOS))
    pos = (RwInt32)file->position.noSupportValue.low;
#else /* (defined(WIN32) || defined(MACOS)) */
    pos = (RwInt32)file->position.supportValue;
#endif /* (defined(WIN32) || defined(MACOS)) */

    RWRETURN (pos);
}

/**
 * \ingroup rtfsmgr
 * \ref RwFAsyncCancel cancels an asynchronous operation currently being
 *                      undertaken on file handle "fptr".
 *
 * \param	fptr  File on which to cancel the operation on.
 *
 * \return  TRUE if successful, FALSE otherwise.
 */
RwBool
RwFAsyncCancel( RtFile *fptr )
{
    RWAPIFUNCTION(RWSTRING("RwFAsyncCancel"));
    RWASSERT(fptr);
    RWRETURN(fptr->fileSystem->fsFileFunc.abort(fptr));
}

/**
 * \ingroup rtfsmgr
 * \ref RtFSManagerSyncAllFilesOnFileSystem will synchronise
 *      all the file on a particular file system, as well as
 *      trigger any callback for the current outstanding operations
 *      on the file within this file system.
 *
 * \param	fs  File system to synchronise.
 */
void
RtFSManagerSyncAllFilesOnFileSystem( RtFileSystem *fs )
{
    RwInt32 i;
    RWAPIFUNCTION(RWSTRING("RtFSManagerSyncAllFilesOnFileSystem"));

    /* Now look at all the files in this fs and call sync */
    for (i = 0; i < fs->maxNbOpenFiles; ++i)
    {
        RtFile *nextFile = fs->fsGetObject(fs, i);

        if(nextFile && nextFile->status != RTFILE_STATUS_CLOSED)
        {
            nextFile->fileSystem->fsFileFunc.sync(nextFile, FALSE);
        }
    }

    RWRETURNVOID();
}

/**
 * \ingroup rtfsmgr
 * \ref RtFSManagerSyncAllFiles will synchronise
 *      all the files on all the registered file systems.
 */
void
RtFSManagerSyncAllFiles( void )
{
	//@{ 20050513 DDonSS : Threadsafe
	// Lock 처리를 위해 변수 선언부 위로 이동..
    RtFileSystem *fs = NULL;
    
    RWAPIFUNCTION(RWSTRING("RtFSManagerSyncAllFiles"));
    
	// fsm Lock
	CS_FSMANAGER_LOCK( &fsm );
	//@} DDonSS
	
    fs = (RtFileSystem *)rwSingleListGetFirstSLLink(&fsm.fs);

    while (fs != (RtFileSystem *)rwSingleListGetTerminator(&fsm.fs))
    {
        RtFSManagerSyncAllFilesOnFileSystem(fs);
        fs = (RtFileSystem *)fs->next;
    }
    
	//@{ 20050513 DDonSS : Threadsafe
	// fsm Unlock
	CS_FSMANAGER_UNLOCK( &fsm );
	//@} DDonSS
	
    RWRETURNVOID();
}

