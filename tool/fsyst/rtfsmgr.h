#ifndef RTFSMGR_H
#define RTFSMGR_H

#include "rtfsyst.h"

/* RWPUBLIC */

#define LASTERROR                   (_rtGetFSManager()->lastError)
#define RTFSMAN_UNLIMITED_NUM_FS    -1

/****************************************************************************
 * Typedefs
 */

/**
 * \ingroup rtfsmdfatatypes
 * \ref RtFSManagerCallBackCode flags that specify the callback to set
 * A RtFSManagerCallBackCode is passed to the \ref RtFSManagerSetCallBack
 * function.
 *
 * \see RtFSManagerSetCallBack
 */
enum RtFSManagerCallBackCode
{
    RTFSM_CALLBACKCODE_ERROR = 0x01,        /**< This represents the Callback
                                                 that will trigger the error
                                                 callback. The file system
                                                 manager error callback is
                                                 triggered when a file system
                                                 manager error occurs. This
                                                 generally happen during
                                                 registration or when trying to
                                                 open a file.*/
    RTFSM_CALLBACKCODE_ERROR_TYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RtFSManagerCallBackCode RtFSManagerCallBackCode;

/**
 * \ingroup rtfsmdatatypes
 * \ref RtFSManagerError
 * File System Manager Error Codes
 */
enum RtFSManagerError
{
    RTFSM_ERROR_NOERROR = 0x01,       /**< No file system manager error.  */
    RTFSM_ERROR_INSTALL,              /**< Install to RenderWare error.   */
    RTFSM_ERROR_UNINSTALL,            /**< Uninstall from RenderWare error. */
    RTFSM_ERROR_INVALIDFS,            /**< Invalid file system specified. */
    RTFSM_ERROR_NOTREGISTRABLE,       /**< Can't register the file system. */
    RTFSM_ERROR_NOFS,                 /**< There is no file system associated
                                           with the file name passed. */
    RTFSM_ERROR_FSNAMEINUSE,          /**< The file system name is already in
                                           use. */
    RTFSM_ERROR_TOOMANYOPENFILES,     /**< The file system manager has reported
                                           that too many files were opened on
                                           the current file system. */
    /* Ensures sizeof(enum) == sizeof(RwInt32)) */
    RTFSM_ERROR_TYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RtFSManagerError RtFSManagerError;

/**
 * \ingroup rtfsmdatatypes
 * \ref RtFSManagerCallBack
 * represents the generic function prototype for a file system manager
 * callback.<br>
 *
 * \param err Error code specifying the error that occured.
 */
typedef void (*RtFSManagerCallBack)  ( RtFSManagerError err );

typedef struct FSManagerCallBacks FSManagerCallBacks;

#if !defined(DOXYGEN)
/*
 * RtFSManagerCallBacks
 * File system manager callbacks.
 */
struct FSManagerCallBacks
{
    RtFSManagerCallBack errorCallBack; /* A file system manager error
                                            occured. */
};

typedef struct RtFSManager RtFSManager;


/*
 * Generic File system manager object.
 * This file system allows for an arbitrary or set number of file systems.
 * This is deterrmined by the maxNbFS parameter passed to RtFSManagerOpen.
 */
struct RtFSManager
{
    RwSingleList         fs;            /**< Single link list containing the
                                             file systems this manager is
                                             managing */
    RwInt32              maxNbFS;       /**< Maximum number of file systems
                                             this manager can manage */
    RwInt32              curNbFS;       /**< Current number of file systems
                                             this manager is managing */
    RwBool               isOpened;      /**< Specify whether the file system
                                             manager is opened or not. */
    RtFSManagerError     lastError;     /**< Last error */
    FSManagerCallBacks   CallBacks;     /**< File system manager callbacks */
    
//@{ 20050513 DDonSS : Threadsafe
#if defined USE_THREADSAFE_FSMANAGER
#if defined WIN32
	CRITICAL_SECTION	criticalSection;
#else // defined WIN32
	void*				criticalSection;
#endif // defined WIN32
#endif defined USE_THREADSAFE_FSMANAGER
//@} DDonSS
	
};
#endif
/****************************************************************************
 * Function Prototypes
 */
#ifdef    __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern void *
RwFopen( const RwChar * name,
         const RwChar * access);

extern RwInt32
RwFclose(void *fptr);

extern size_t
RwFread( void *addr,
         size_t size,
         size_t count,
         void *fptr);

extern size_t
RwFwrite( const void *addr, size_t size, size_t count, void *fptr);

extern RwInt32
RwFseek( void    *fptr,
         long     offset,
         RwInt32  origin);

extern RwChar *
RwFgets( RwChar   *buffer, RwInt32   maxLen, void     *fptr);

extern RwInt32
RwFputs( const RwChar * buffer, void *fptr );

extern RwInt32
RwFeof( void *fptr );

extern RwBool
RwFexist( const RwChar * name );

extern RwInt32
RwFtell( void *fptr );

extern RwBool
RwFAsyncCancel( RtFile *fptr );

extern RwBool
RtFSManagerOpen( RwInt32 maxNbFS );

extern void
RtFSManagerClose( void );

extern RwBool
RtFSManagerSetCallBack( RtFSManagerCallBackCode cbCode,
                        RtFSManagerCallBack CallBack);

extern RwBool
RtFSManagerRegister( RtFileSystem *fs );

extern void
RtFSManagerUnregister( RtFileSystem *fs );

extern RtFileSystem *
RtFSManagerGetFileSystemFromName( RwChar *fsName );

extern RtFile *
RtFSManagerFOpen( const RwChar       *filename,
                  RtFileAccessFlag    access,
                  RtFSFileOpCallBack  CallBack,
                  void               *CallBackData );

extern RtFileSystem *
RtFSManagerGetFileSystemFromFileName( const RwChar *fileName );

extern void
RtFSManagerSetDefaultFileSystem( RtFileSystem *fs );

extern RtFileSystem *
RtFSManagerGetDefaultFileSystem( void );

extern void
RtFSManagerSyncAllFilesOnFileSystem( RtFileSystem *fs );

extern void
RtFSManagerSyncAllFiles( void );

extern RtFSManagerError
RtFSManagerGetLastError( void );

extern RwBool
_rtFSManagerIsNewFSRegistrable( void );

extern RwBool
_rtFSManagerIsFSNameInUse( RwChar *fsName );

extern RtFileSystem *
_rtGetFileSystemFromDeviceName( const RwChar *deviceName );

extern RtFile *
_rtFSManagerFOpenOnFS( RtFileSystem *fs,
                       const RwChar * name,
                       RtFileAccessFlag access,
                       RtFSFileOpCallBack  CallBack,
                       void               *CallBackData );

RtFSManager  *
_rtGetFSManager( void );

RwUInt32
TkFSManagerGetValue(RtInt64 value);

/**
 * \ingroup rtfsmdatatypes
 * \ref RtFSManagerGetLastError returns the last file system manager error.
 */
#define RtFSManagerGetLastError() LASTERROR


#ifdef __cplusplus
}
#endif  /* cplusplus */

/* RWPUBLICEND */

#endif /* FSYSMAN_H */

