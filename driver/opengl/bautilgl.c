/***************************************************************************
 *                                                                         *
 * Module  : bautilgl.c                                                    *
 *                                                                         *
 * Purpose : General utility/helper functions for the OpenGL driver.       *
 *                                                                         *
 **************************************************************************/

/****************************************************************************
 Includes
 */

#include <string.h>

#if defined(WIN32)
#include <windows.h>
#endif /* defined(WIN32) */
#include <GL/gl.h>

#include "barwtyp.h"
#include "baresour.h"

#include "baogl.h"
#include "drvfns.h"


/* maximum width of the debug output for extensions */
#if defined(RWDEBUG)
#define RWOPENGL_OUTPUTEXTSMAXWIDTH    (80)
#endif /* defined(RWDEBUG) */

static RwUInt32 _rwOpenGLNumExtensions;                     /* number of extensions */

static RwChar   *_rwOpenGLExtensionString = (RwChar *)NULL; /* OpenGL platform extension strings */

static RwBool
_rwOpenGLIsExtensionSupportedCB( const RwChar * const extString,
                                 const RwBool lastExt,
                                 const void * const inParam,
                                 void *outParam );


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLRasterClose
 *
 *  Purpose : Find the next highest power of 2 of an unsigned integer.
 *
 *  On entry: in - RwUInt32 containing the integer to find the next highest
 *            power of 2 to.
 *
 *  On exit : RwUInt32 containing the power of 2.
 * ------------------------------------------------------------------------- */
RwUInt32
_rwOpenGLNextPowerOf2( RwUInt32 in )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLNextPowerOf2" ) );

     in -= 1;

     in |= in >> 16;
     in |= in >> 8;
     in |= in >> 4;
     in |= in >> 2;
     in |= in >> 1;

     RWRETURN( in + 1 );
}


/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLIsExtensionSupportedCB
 *
 *  Purpose : Determine if an extension is supported.
 *
 *  On entry: extString - Pointer to RwChar containing the extension string.
 *            lastExt   - RwBool, TRUE if this is the last extension string,
 *                        FALSE if not.
 *            inParam   - Void pointer for user input data.
 *            outParam  - Void pointer for user output data.
 *
 *  On exit : RwBool, TRUE if successful, FALSE otherwise.
 * --------------------------------------------------------------------- */
static RwBool
_rwOpenGLIsExtensionSupportedCB( const RwChar * const extString,
                                 const RwBool lastExt __RWUNUSED__,
                                 const void * const inParam,
                                 void *outParam )
{
    const RwChar  * const extStringToTest = (const RwChar * const)inParam;

    RwBool  *supported;


    RWFUNCTION( RWSTRING( "_rwOpenGLIsExtensionSupportedCB" ) );

    RWASSERT( NULL != extString );
    RWASSERT( NULL != inParam );
    RWASSERT( NULL != outParam );

    supported = (RwBool *)outParam;

    if ( 0 == rwstrcmp( extString, extStringToTest ) )
    {
        *supported = TRUE;
        RWRETURN( FALSE );
    }
    else
    {
        *supported = FALSE;
    }

    RWRETURN( TRUE );
}


/**
 * \ingroup rwengineopengl
 *
 * \ref RwOpenGLIsExtensionSupported can be used to determine whether
 * an OpenGL extension, identified by it's name, is supported by the
 * current implementation.
 *
 * \param extString Constant pointer to constant \ref RwChar string
 *                  containing the extension name.
 *
 * \return \ref RwBool, TRUE if the extension is supported, FALSE if not.
 */
RwBool
RwOpenGLIsExtensionSupported( const RwChar * const extString )
{
    RwBool  supported;


    RWAPIFUNCTION( "RwOpenGLIsExtensionSupported" );

    _rwOpenGLEnumExtensions( _rwOpenGLIsExtensionSupportedCB, extString, &supported );

    RWRETURN( supported );
}


/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLEnumExtensions
 *
 *  Purpose : serves two purposes:
 *
 *  1) generate an internal representation of the extensions supported by OpenGL - this is
 *     always created on the first call to this function. (Use _rwOpenGLDestroyExtensionsString
 *     to destroy the internal representation.)
 *
 *  2) execute a callback for each extension string supported.
 *
 *  On entry: callBack _rwOpenGLEnumExtensionsCallback callback function that is called with
 *                     every extension string. This may be NULL to not execute any callback.
 *
 *            inParam Constant void pointer as an input parameter to \e callBack.
 *
 *            outParam Void pointer as an output parameter to \e callBack.
 *
 *  On exit : Nothing
 * --------------------------------------------------------------------- */
void
_rwOpenGLEnumExtensions( _rwOpenGLEnumExtensionsCallback callBack,
                         const void * const inParam,
                         void *outParam )
{
    RWAPIFUNCTION( RWSTRING( "_rwOpenGLEnumExtensions" ) );

    /* generate the extensions string once */
    if ( NULL == _rwOpenGLExtensionString )
    {
        const GLubyte   *extString;

        RwUInt32        extStringLen;

        RwInt32         i;


        extString = glGetString( GL_EXTENSIONS );
        RWASSERT( NULL != extString );

        extStringLen = strlen( (const char *)extString ) + 1;

        /* allocate storage to hold the extensions string */
        _rwOpenGLExtensionString = (RwChar *)RwDriverMalloc( sizeof( RwChar ) *
                                                             extStringLen,
                                                             rwID_DRIVERMODULE |
                                                             rwMEMHINTDUR_GLOBAL );
        RWASSERT( NULL != _rwOpenGLExtensionString );
        if ( NULL == _rwOpenGLExtensionString )
        {
            RWERROR( (E_RW_NOMEM, (sizeof(RwChar) * extStringLen)) );
            RWRETURNVOID();
        }

        memcpy( (void *)_rwOpenGLExtensionString,
                (const void *)extString,
                sizeof( RwChar ) * extStringLen );

        i = 0;
        _rwOpenGLNumExtensions = 0;
        while ( _rwOpenGLExtensionString[i] != 0 )
        {
            /* replace spaces with null characters */
            if ( (GLubyte)' ' == _rwOpenGLExtensionString[i] )
            {
                _rwOpenGLExtensionString[i] = '\0';

                _rwOpenGLNumExtensions += 1;
            }

            i += 1;
        }
    }

    if ( NULL != callBack )
    {
        RwUInt32 i;

        RwUInt32 offset;


        RWASSERT( NULL != _rwOpenGLExtensionString );

        offset = 0;
        for ( i = 0 ; i < _rwOpenGLNumExtensions; i += 1 )
        {
            if ( FALSE == (callBack)( _rwOpenGLExtensionString + offset,
                                      (i == (_rwOpenGLNumExtensions - 1)),
                                      inParam,
                                      outParam ) )
            {
                /* are we exiting the iteration early? */
                break;
            }

            offset += rwstrlen( _rwOpenGLExtensionString + offset ) + 1;
        }
    }

    RWRETURNVOID();
}


#if defined( RWDEBUG )

/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLDebugOutputExtensionsCB
 *
 *  Purpose : Output all the supported extension strings in this
 *            OpenGL platform. Squeeze as many strings into 128 characters
 *            before outputting.
 *
 *  On entry: extString - Pointer to RwChar containing the extension string.
 *            lastExt   - RwBool, TRUE if this is the last extension string,
 *                        FALSE if not.
 *            inParam   - Void pointer for user input data.
 *            outParam  - Void pointer for user output data.
 *
 *  On exit : RwBool, TRUE if successful, FALSE otherwise.
 * --------------------------------------------------------------------- */
RwBool
_rwOpenGLDebugOutputExtensionsCB( const RwChar * const extString,
                                  const RwBool lastExt,
                                  const void * const inParam __RWUNUSED__,
                                  void *outParam __RWUNUSED__ )
{
    static RwChar   message[RWOPENGL_OUTPUTEXTSMAXWIDTH];

    static RwInt32  offset = 0;

    RwInt32         length;


    RWFUNCTION( RWSTRING( "_rwOpenGLDebugOutputExtensionsCB" ) );

    RWASSERT( NULL != extString );

    length = rwstrlen( extString );
    RWASSERT( length < RWOPENGL_OUTPUTEXTSMAXWIDTH - 1 );

    if ( offset + length >= RWOPENGL_OUTPUTEXTSMAXWIDTH - 1 )
    {
        message[offset] = '\0';

        if ( NULL != RWSRCGLOBAL( debugFunction ) )
        {
            RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, message );
        }

        offset = 0;
    }

    rwstrncpy( message + offset, extString, length );

    if ( (TRUE == lastExt) &&
         !(offset + length >= RWOPENGL_OUTPUTEXTSMAXWIDTH - 1) )
    {
        message[offset + length] = '\0';

        if ( NULL != RWSRCGLOBAL( debugFunction ) )
        {
            RWSRCGLOBAL( debugFunction )( rwDEBUGMESSAGE, message );
        }
    }
    else
    {
        message[offset + length] = ' ';
        offset += length + 1;
    }

    RWRETURN( TRUE );
}

#endif /* defined( RWDEBUG ) */


/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLDestroyExtensionsString
 *
 *  Purpose : Destroy the OpenGL platform extensions string.
 *
 *  On entry: Nothing
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
void
_rwOpenGLDestroyExtensionsString( void )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLDestroyExtensionsString" ) );

    if ( NULL != _rwOpenGLExtensionString )
    {
        RwDriverFree( _rwOpenGLExtensionString );
        _rwOpenGLExtensionString = (RwChar *)NULL;
    }

    RWRETURNVOID();
}


/****************************************************************************
 _rwOpenGLGetMaxSupportedTextureSize

 On entry   : Nothing
 On exit    : Returns the largest texture width/height supported by this
              OpenGL implementation.
 */
RwInt32
_rwOpenGLGetMaxSupportedTextureSize(void)
{
    static GLint maxTextureSize = 0;

    RWFUNCTION(RWSTRING("_rwOpenGLGetMaxSupportedTextureSize"));

    /* Get the driver to give us a rough estimate of the largest texture it can handle */
    if (!maxTextureSize)
    {
        _rwOpenGLMakeCurrent();
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    }

    RWRETURN(maxTextureSize);

#if 0 /* Out of action code until I can work out why it breaks the Max */

    /* But what if the driver is fibbing?  Let's check up on it! */
    do
    {
        /* On Apple's OGL implememtation, doing the proxy test invalidates the currently
         * bound texture so push and pop texture state
         */
        glPushAttrib(GL_TEXTURE_BIT);

            glTexImage2D(GL_PROXY_TEXTURE_2D,
                                    0,                 /* level */
                                    4,                 /* components/internal format */
                                    maxTextureSize,
                                    maxTextureSize,
                                    0,                 /* border */
                                    GL_RGBA,           /* data format */
                                    GL_UNSIGNED_BYTE,  /* data type */
                                    NULL);

            /* If a query of the proxied texture width is 0, we know that a texture this size
             * is too large
             */
            glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D,
                                    0,
                                    GL_TEXTURE_WIDTH,
                                    &textureWidth);

        glPopAttrib();

        /* The driver was telling the truth */
        if (textureWidth == maxTextureSize)
        {
            RWRETURN(maxTextureSize);
        }

        /* Try the next power of two down */
        maxTextureSize = maxTextureSize >> 1;
    }
    while (!textureWidth && maxTextureSize);

    /* I seriously doubt we'll ever get here.  A driver that doesn't support ANY
     * texture size?  What, are you CRAZY??  Oh have 2 and be happy.
     */
    RWRETURN(2);
#endif
}


/****************************************************************************
 _rwOpenGLGetMaxSupportedHardwareLights

 On entry   : Nothing
 On exit    : Number of simultaneously enabled lights that this OGL
              implementation supports.
 */
RwInt32
_rwOpenGLGetMaxSupportedHardwareLights(void)
{
    static GLint maxLights = -1;

    RWFUNCTION(RWSTRING("_rwOpenGLGetMaxSupportedHardwareLights"));

    if (maxLights == -1)
    {
        _rwOpenGLMakeCurrent();
        glGetIntegerv(GL_MAX_LIGHTS, &maxLights);
    }

    RWRETURN(maxLights);
}


/****************************************************************************
 _rwOpenGLApplyRwMatrix

  This function will take a RenderWare matrix and multiply it's OpenGL
  equivalent with what is on top of the current OpenGL matrix stack.

 On entry   : A RenderWare matrix
 On exit    : Nothing
 */
void
_rwOpenGLApplyRwMatrix(RwMatrix *matrix)
{
    GLfloat m[16];
    RwV3d *pos   = RwMatrixGetPos(matrix);
    RwV3d *at    = RwMatrixGetAt(matrix);
    RwV3d *right = RwMatrixGetRight(matrix);
    RwV3d *up    = RwMatrixGetUp(matrix);

    RWFUNCTION(RWSTRING("_rwOpenGLApplyRwMatrix"));

    m[0]  = right->x; m[1]  = right->y; m[2]  = right->z; m[3]  = 0;
    m[4]  = up->x;    m[5]  = up->y;    m[6]  = up->z;    m[7]  = 0;
    m[8]  = at->x;    m[9]  = at->y;    m[10] = at->z;    m[11] = 0;
    m[12] = pos->x;   m[13] = pos->y;   m[14] = pos->z;   m[15] = 1;

    glMultMatrixf(m);

    RWRETURNVOID();
}


/****************************************************************************
 _rwOpenGLMatrixToRwMatrix

 This function fills out an RwMatrix from an OpenGL matrix (4x4, column major).
 The caller is responsible for setting up the RwMatrix appropriately.

 On entry   : Pointer to RwMatrix to fill.
              Array of GLfloats containing the OpenGL matrix.

 On exit    : Nothing
 */
void
_rwOpenGLMatrixToRwMatrix( RwMatrix *outMatrix,
                           void *inMatrix )
{
    GLfloat *glMatrix;


    RWFUNCTION( RWSTRING( "_rwOpenGLMatrixToRwMatrix" ) );

    RWASSERT( NULL != outMatrix );
    RWASSERT( NULL != inMatrix );

    glMatrix = (GLfloat *)inMatrix;

    outMatrix->right.x = glMatrix[0];
    outMatrix->right.y = glMatrix[1];
    outMatrix->right.z = glMatrix[2];
    outMatrix->flags   = 0;

    outMatrix->up.x    = glMatrix[4];
    outMatrix->up.y    = glMatrix[5];
    outMatrix->up.z    = glMatrix[6];
    outMatrix->pad1    = 0;

    outMatrix->at.x    = glMatrix[8];
    outMatrix->at.y    = glMatrix[9];
    outMatrix->at.z    = glMatrix[10];
    outMatrix->pad2    = 0;

    outMatrix->pos.x   = glMatrix[12];
    outMatrix->pos.y   = glMatrix[13];
    outMatrix->pos.z   = glMatrix[14];
    outMatrix->pad3    = 0;

    RWRETURNVOID();
}


/************ VERTEX HEAP SUPPORT FOR VERTEX ARRAY RANGE **************/

#define OPENGLVERTEXHEAP_ALIGNTO32BYTES(_addr)  \
    ( (_addr) & 0xFFFFFFE0 )

#define OPENGLVERTEXHEAP_FENCENA   (0)

#define OPENGLVERTEXHEAPBLOCK_ISFENCEVALID(_block)                      \
    (OPENGLVERTEXHEAP_FENCENA != (_block)->fence)

#define OPENGLVERTEXHEAPBLOCK_FINISHFENCE(_block)                       \
MACRO_START                                                             \
    if ( GL_FALSE != _rwOpenGLExt.IsFenceNV( (_block)->fence ) )        \
    {                                                                   \
        if ( GL_FALSE == _rwOpenGLExt.TestFenceNV( (_block)->fence ) )  \
        {                                                               \
            _rwOpenGLExt.FinishFenceNV( (_block)->fence );              \
        }                                                               \
    }                                                                   \
MACRO_STOP

typedef struct openGLVertexHeapHeader openGLVertexHeapHeader;
struct openGLVertexHeapHeader
{
    void                    *videoMemory;   /* pointer to the video memory block */

    RwUInt32                blockSize;      /* size of the video memory block */

    RwInt32                 fence;          /* fence index as generated by OpenGL */

    openGLVertexHeapHeader  *prev;          /* pointer to the previous heap header in the list */

    openGLVertexHeapHeader  *next;          /* pointer to the next heap header in the list */

    RwBool                  reuse;          /* RwBool, TRUE if available for reuse, FALSE otherwise */
};

static void     *openGLVertexHeapMemory = (void *)NULL;

static RwUInt32 openGLVertexHeapSize = 4 * 1024 * 1024;

static openGLVertexHeapHeader   *openGLStaticUsedBlockStart = (openGLVertexHeapHeader *)NULL;
static openGLVertexHeapHeader   *openGLDynamicUsedBlockStart = (openGLVertexHeapHeader *)NULL;

static openGLVertexHeapHeader   *openGLFreeBlockStart = (openGLVertexHeapHeader *)NULL;
static RwFreeList               *openGLFreeBlockFreeList = (RwFreeList *)NULL;

static RwBool                   nvFenceAvailable = FALSE;

static RwBool                   nvResizeRequired = FALSE;


/* ---------------------------------------------------------------------
 *  Function: openglVertexHeapSplitFreeBlock
 *
 *  Purpose : Splits a free block into two.
 *
 *  On entry: block - Constant pointer to openGLVertexHeapHeader referring
 *                    to the larger block to split.
 *
 *            alignedSize - Constant RwUInt32 containing the final size
 *                          of block.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
static void
openglVertexHeapSplitFreeBlock( openGLVertexHeapHeader * const block,
                                const RwUInt32 alignedSize )
{
    openGLVertexHeapHeader  *freeBlock;


    RWFUNCTION( RWSTRING( "openglVertexHeapSplitFreeBlock" ) );

    /* create a new free block */
    freeBlock = RwFreeListAlloc( openGLFreeBlockFreeList,
        rwID_DRIVERMODULE | rwMEMHINTDUR_EVENT);
    RWASSERT( NULL != freeBlock );

    freeBlock->videoMemory = (RwUInt8 *)(block->videoMemory) + alignedSize;
    freeBlock->blockSize = block->blockSize - alignedSize;
    freeBlock->fence = OPENGLVERTEXHEAP_FENCENA;
    freeBlock->reuse = FALSE;

    /* add the new free block to the start of the free blocks list */
    freeBlock->prev = (openGLVertexHeapHeader *)NULL;
    freeBlock->next = openGLFreeBlockStart;
    openGLFreeBlockStart->prev = freeBlock;
    openGLFreeBlockStart = freeBlock;

    /* resize the original block */
    block->blockSize = alignedSize;

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLVertexHeapCreate
 *
 *  Purpose : Create the NV vertex array heap.
 *
 *  On entry: Nothing.
 *
 *  On exit : RwBool, TRUE if the vertex array was successfully created,
 *            FALSE if not.
 * --------------------------------------------------------------------- */
RwBool
_rwOpenGLVertexHeapCreate( void )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLVertexHeapCreate" ) );

    if ( _rwOpenGLVertexHeapAvailable() )
    {
        const RwReal    nvReadFrequency = 0.0f;     /* low read frequency */

        const RwReal    nvWriteFrequency = 1.0f;    /* high write frequency */

        const RwReal    nvPriority = 1.0f;          /* high priority, 1.0 = video, 0.5 = AGP */

        void            *endOfVideoMemory;


        openGLVertexHeapMemory = _rwOpenGLExt.AllocateMemoryNV( openGLVertexHeapSize,
                                                                nvReadFrequency,
                                                                nvWriteFrequency,
                                                                nvPriority );
        if ( NULL == openGLVertexHeapMemory )
        {
            RWMESSAGE( (RWSTRING("Error: NVIDIA vertex array range memory allocation failed for %d bytes"), openGLVertexHeapSize) );
            RWRETURN( FALSE );
        }
        else
        {
            RWMESSAGE( (RWSTRING("Info: NVIDIA vertex array range memory: %.1fMb"), openGLVertexHeapSize / 1024.0f / 1024.f) );
        }

        glEnableClientState( GL_VERTEX_ARRAY_RANGE_NV );
        if ( FALSE != _rwOpenGLExt.VertexArrayRange2NV )
        {
            glEnableClientState( GL_VERTEX_ARRAY_RANGE_WITHOUT_FLUSH_NV );
        }

        _rwOpenGLExt.VertexArrayRangeNV( openGLVertexHeapSize, openGLVertexHeapMemory );

        /* create a free block free list */
        RWASSERT( NULL == openGLFreeBlockFreeList );
        openGLFreeBlockFreeList = RwFreeListCreate( sizeof(openGLVertexHeapHeader), 1024, sizeof(RwUInt32),
            rwID_DRIVERMODULE | rwMEMHINTDUR_EVENT);
        RWASSERT( NULL != openGLFreeBlockFreeList );

        /* the whole memory block is the first free block */
        openGLFreeBlockStart = RwFreeListAlloc( openGLFreeBlockFreeList,
            rwID_DRIVERMODULE | rwMEMHINTDUR_EVENT);
        RWASSERT( NULL != openGLFreeBlockStart );
        openGLFreeBlockStart->videoMemory = openGLVertexHeapMemory;

        endOfVideoMemory = (RwUInt8 *)openGLVertexHeapMemory + openGLVertexHeapSize;
        endOfVideoMemory = (void *)(OPENGLVERTEXHEAP_ALIGNTO32BYTES( (RwUInt32)endOfVideoMemory ));
        openGLFreeBlockStart->blockSize = (RwUInt32)(endOfVideoMemory) - (RwUInt32)openGLVertexHeapMemory;
        openGLFreeBlockStart->fence = OPENGLVERTEXHEAP_FENCENA;
        openGLFreeBlockStart->prev = (openGLVertexHeapHeader *)NULL;
        openGLFreeBlockStart->next = (openGLVertexHeapHeader *)NULL;
        openGLFreeBlockStart->reuse = FALSE;

        if ( (NULL != _rwOpenGLExt.SetFenceNV) &&
             (NULL != _rwOpenGLExt.IsFenceNV) &&
             (NULL != _rwOpenGLExt.TestFenceNV) &&
             (NULL != _rwOpenGLExt.FinishFenceNV) &&
             (NULL != _rwOpenGLExt.GenFencesNV) &&
             (NULL != _rwOpenGLExt.DeleteFencesNV) )
        {
            nvFenceAvailable = TRUE;
        }

        nvResizeRequired = FALSE;
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLVertexHeapDestroy
 *
 *  Purpose : Destroys the previously created vertex array heap. This also
 *            ensures that any unrendered data is submitted to the GPU before
 *            destroying the data.
 *
 *  On entry: Nothing.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
void
_rwOpenGLVertexHeapDestroy( void )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLVertexHeapDestroy" ) );

    if ( _rwOpenGLVertexHeapAvailable() )
    {
        openGLVertexHeapHeader  *block;


        /* free the used blocks */
        block = openGLDynamicUsedBlockStart;
        while ( NULL != block )
        {
            openGLVertexHeapHeader  *nextBlock;


            nextBlock = block->next;

            if ( OPENGLVERTEXHEAPBLOCK_ISFENCEVALID(block) )
            {
                OPENGLVERTEXHEAPBLOCK_FINISHFENCE( block );
                _rwOpenGLExt.DeleteFencesNV( 1, &(block->fence) );
                block->fence = OPENGLVERTEXHEAP_FENCENA;
            }

            /* yes, this is meant to be openGLFreeBlockFreeList */
            RwFreeListFree( openGLFreeBlockFreeList, block );
            block = nextBlock;
        }
        openGLDynamicUsedBlockStart = (openGLVertexHeapHeader *)NULL;

        /* free the free blocks */
        block = openGLFreeBlockStart;
        while ( NULL != block )
        {
            openGLVertexHeapHeader  *nextBlock;


            nextBlock = block->next;
            RwFreeListFree( openGLFreeBlockFreeList, block );
            block = nextBlock;
        }
        RwFreeListDestroy( openGLFreeBlockFreeList );
        openGLFreeBlockFreeList = (RwFreeList *)NULL;
        openGLFreeBlockStart = (openGLVertexHeapHeader *)NULL;

        /* disable NV client states */
        if ( FALSE != _rwOpenGLExt.VertexArrayRange2NV )
        {
            glDisableClientState( GL_VERTEX_ARRAY_RANGE_WITHOUT_FLUSH_NV );
        }
        glDisableClientState( GL_VERTEX_ARRAY_RANGE_NV );

        if ( NULL != openGLVertexHeapMemory )
        {
            _rwOpenGLExt.FreeMemoryNV( openGLVertexHeapMemory );
            openGLVertexHeapMemory = (void *)NULL;
        }
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLVertexHeapSetSize
 *
 *  Purpose : Sets the initial size of the NV vertex array heap. This must
 *            be called before _rwOpenGLVertexHeapCreate to have any effect.
 *
 *  On entry: size - Constant RwUInt32 containing the new initial size of
 *                   the vertex array in bytes.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
void
_rwOpenGLVertexHeapSetSize( const RwUInt32 size )
{
    openGLVertexHeapSize = size;

    return;
}


/* ---------------------------------------------------------------------
 *  Function: openGLVertexHeapMergeFreeBlocks
 *
 *  Purpose : Merges all free blocks into the largest possible blocks.
 *
 *  On entry: Nothing.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
static void
openGLVertexHeapMergeFreeBlocks( void )
{
    openGLVertexHeapHeader  *pivotBlock;


    RWFUNCTION( RWSTRING( "openGLVertexHeapMergeFreeBlocks" ) );

    pivotBlock = openGLFreeBlockStart;
    while ( NULL != pivotBlock )
    {
        openGLVertexHeapHeader  *nextBlock;


        nextBlock = pivotBlock->next;
        while ( NULL != nextBlock )
        {
            /* does the pivot block video memory lie next to any free block video memory?
             * if so, merge the pivot into the other free block memory */
            if ( (RwUInt8 *)(pivotBlock->videoMemory) == (RwUInt8 *)(nextBlock->videoMemory) + (nextBlock->blockSize) )
            {
                nextBlock->blockSize += pivotBlock->blockSize;
                break;
            }
            else if ( (RwUInt8 *)(pivotBlock->videoMemory) + (pivotBlock->blockSize) == (RwUInt8 *)(nextBlock->videoMemory) )
            {
                nextBlock->videoMemory = pivotBlock->videoMemory;
                nextBlock->blockSize += pivotBlock->blockSize;
                break;
            }

            nextBlock = nextBlock->next;
        }

        /* if we've merged, lose the pivot block and restart
         * otherwise, move on */
        if ( NULL != nextBlock )
        {
            /* remove the pivot block from the free block list */
            if ( NULL != pivotBlock->prev )
            {
                pivotBlock->prev->next = pivotBlock->next;
            }
            if ( NULL != pivotBlock->next )
            {
                pivotBlock->next->prev = pivotBlock->prev;
            }
            if ( openGLFreeBlockStart == pivotBlock )
            {
                openGLFreeBlockStart = pivotBlock->next;
            }

            RwFreeListFree( openGLFreeBlockFreeList, pivotBlock );

            /* restart as we might be able to merge more blocks */
            pivotBlock = openGLFreeBlockStart;
        }
        else
        {
            pivotBlock = pivotBlock->next;
        }
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglVertexHeapBlockFree
 *
 *  Purpose : Free a used block and then attempt to merge it with all the
 *            other free blocks.
 *
 *  On entry: baseUsedHeader - Double pointer to openGLVertexHeapHeader referring
 *                             to the base used header, i.e. dynamic or static.
 *
 *            blockToFree - Pointer to openGLVertexHeapHeader referring to
 *                          the used block to free.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
static void
openglVertexHeapBlockFree( openGLVertexHeapHeader **baseUsedHeader,
                           openGLVertexHeapHeader *blockToFree )
{
    RWFUNCTION( RWSTRING( "openglVertexHeapBlockFree" ) );

    if ( OPENGLVERTEXHEAPBLOCK_ISFENCEVALID(blockToFree) )
    {
        OPENGLVERTEXHEAPBLOCK_FINISHFENCE( blockToFree );
        _rwOpenGLExt.DeleteFencesNV( 1, &(blockToFree->fence) );
        blockToFree->fence = OPENGLVERTEXHEAP_FENCENA;
    }

    /* remove this block from the used block list */
    if ( NULL != blockToFree->prev )
    {
        blockToFree->prev->next = blockToFree->next;
    }
    if ( NULL != blockToFree->next )
    {
        blockToFree->next->prev = blockToFree->prev;
    }
    if ( *baseUsedHeader == blockToFree )
    {
        *baseUsedHeader = blockToFree->next;
    }

    /* add the used block onto the free block list */
    blockToFree->prev = (openGLVertexHeapHeader *)NULL;
    blockToFree->next = openGLFreeBlockStart;
    if ( NULL != openGLFreeBlockStart )
    {
        openGLFreeBlockStart->prev = blockToFree;
    }
    openGLFreeBlockStart = blockToFree;

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openGLVertexHeapReuseDynamicBlocks
 *
 *  Purpose : Forces all dynamically used blocks marked for reuse to be freed.
 *
 *  On entry: Nothing.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
static void
openGLVertexHeapReuseDynamicBlocks( void )
{
    openGLVertexHeapHeader  *usedBlock;

    openGLVertexHeapHeader  *newDynamicUsedBlockStart;

    openGLVertexHeapHeader  *newDynamicUsedBlockEnd;

    RwBool                  needToMerge;


    RWFUNCTION( RWSTRING( "openGLVertexHeapReuseDynamicBlocks" ) );

    /* free all available dynamic used blocks */
    usedBlock = openGLDynamicUsedBlockStart;
    newDynamicUsedBlockStart = (openGLVertexHeapHeader *)NULL;
    newDynamicUsedBlockEnd = (openGLVertexHeapHeader *)NULL;
    needToMerge = FALSE;

    while ( NULL != usedBlock )
    {
        openGLVertexHeapHeader  *nextUsedBlock;


        nextUsedBlock = usedBlock->next;

        if ( FALSE == usedBlock->reuse )
        {
            if ( NULL == newDynamicUsedBlockStart )
            {
                newDynamicUsedBlockStart = usedBlock;
                usedBlock->next = (openGLVertexHeapHeader *)NULL;
                usedBlock->prev = (openGLVertexHeapHeader *)NULL;
                newDynamicUsedBlockEnd = usedBlock;
            }
            else
            {
                usedBlock->next = (openGLVertexHeapHeader *)NULL;
                usedBlock->prev = newDynamicUsedBlockEnd;
                newDynamicUsedBlockEnd->next = usedBlock;
                newDynamicUsedBlockEnd = usedBlock;
            }
        }
        else
        {
            openglVertexHeapBlockFree( &openGLDynamicUsedBlockStart, usedBlock );
            needToMerge = TRUE;
        }

        usedBlock = nextUsedBlock;
    }
    openGLDynamicUsedBlockStart = newDynamicUsedBlockStart;

    if ( FALSE != needToMerge )
    {
        /* merge all free blocks where possible */
        openGLVertexHeapMergeFreeBlocks();
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openGLVertexHeapForceDynamicBlockFree
 *
 *  Purpose : Forces all dynamically used blocks to be freed if possible.
 *            This MAY cause some rendering artifacts.
 *
 *  On entry: Nothing.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
static void
openGLVertexHeapForceDynamicBlockFree( void )
{
    openGLVertexHeapHeader  *usedBlock;


    RWFUNCTION( RWSTRING( "openGLVertexHeapForceDynamicBlockFree" ) );

    /* free all available dynamic used blocks */
    usedBlock = openGLDynamicUsedBlockStart;
    while ( NULL != usedBlock )
    {
        openGLVertexHeapHeader  *nextUsedBlock;


        nextUsedBlock = usedBlock->next;

        openglVertexHeapBlockFree( &openGLDynamicUsedBlockStart, usedBlock );

        usedBlock = nextUsedBlock;
    }
    openGLDynamicUsedBlockStart = (openGLVertexHeapHeader *)NULL;

    /* merge all free blocks where possible */
    openGLVertexHeapMergeFreeBlocks();

    RWRETURNVOID();
}



/* ---------------------------------------------------------------------
 *  Function: openGLVertexHeapGenericMalloc
 *
 *  Purpose : Allocate a block of memory from the vertex array
 *            of the required size, which can then be used by dynamic or
 *            static malloc functions as required.
 *
 *  On entry: size - Constant RwUInt32 containing the requested block size
 *                   in bytes.
 *
 *            generateFence - Constant RwBool containing TRUE if a fence is to
 *                            be generated, or FALSE if not.
 *
 *  On exit : Pointer to openGLVertexHeapHeader containing the free block
 *            that can be used.
 * --------------------------------------------------------------------- */
static openGLVertexHeapHeader *
openGLVertexHeapGenericMalloc( const RwUInt32 size,
                               const RwBool generateFence )
{
    RwUInt32                alignedSize;

    openGLVertexHeapHeader  *freeBlock;


    RWFUNCTION( RWSTRING( "openGLVertexHeapGenericMalloc" ) );

    /* round allocation size up to next 32-byte multiple */
    alignedSize = OPENGLVERTEXHEAP_ALIGNTO32BYTES( size + 31 );

    /* find a suitable free block */
    freeBlock = openGLFreeBlockStart;
    while ( NULL != freeBlock )
    {
        if ( freeBlock->blockSize >= alignedSize )
        {
            break;
        }

        freeBlock = freeBlock->next;
    }

    /* there are no suitable free blocks */
    if ( NULL == freeBlock )
    {
        /* try to free up some used dynamic blocks */
        openGLVertexHeapReuseDynamicBlocks();

        /* find a suitable free block */
        freeBlock = openGLFreeBlockStart;
        while ( NULL != freeBlock )
        {
            if ( freeBlock->blockSize >= alignedSize )
            {
                break;
            }

            freeBlock = freeBlock->next;
        }

        /* no video memory free at all */
        if ( NULL == freeBlock )
        {
            RWMESSAGE( (RWSTRING("Warning: NVIDIA vertex array range buffer close to exhaustion.\n"\
                                 "It is recommended to increase the default buffer size with"\
                                 "_rwOpenGLVertexHeapSetSize() to avoid this warning.")) );

            nvResizeRequired = TRUE;

            /* try harder to get some memory back */
            openGLVertexHeapForceDynamicBlockFree();

            /* find a suitable free block */
            freeBlock = openGLFreeBlockStart;
            while ( NULL != freeBlock )
            {
                if ( freeBlock->blockSize >= alignedSize )
                {
                    break;
                }

                freeBlock = freeBlock->next;
            }

            /* no video memory free at all */
            if ( NULL == freeBlock )
            {
                RWMESSAGE( (RWSTRING("Unable to allocate %d bytes in the NVIDIA vertex array range buffer.\n"\
                                     "Possibly due to large (or many) resident static vertex arrays.\n"\
                                     "Increase the default buffer size with _rwOpenGLVertexHeapSetSize() and try again."),
                            size) );

                RWRETURN( (void *)NULL );
            }
        }
    }

    /* split off any excess video memory into an extra free block */
    if ( freeBlock->blockSize > (RwUInt32)(alignedSize + 64) )
    {
        openglVertexHeapSplitFreeBlock( freeBlock, alignedSize );
    }

    /* create the NV fence for the new used block */
    if ( FALSE != nvFenceAvailable )
    {
        if ( FALSE != generateFence )
        {
            /* a zero valued fence is invalid */
            _rwOpenGLExt.GenFencesNV( 1, &(freeBlock->fence) );
        }
    }

    /* remove the free block from the free block list */
    if ( NULL != freeBlock->prev )
    {
        freeBlock->prev->next = freeBlock->next;
    }
    if ( NULL != freeBlock->next )
    {
        freeBlock->next->prev = freeBlock->prev;
    }
    if ( openGLFreeBlockStart == freeBlock )
    {
        openGLFreeBlockStart = freeBlock->next;
    }

    RWRETURN( freeBlock );
}


/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLVertexHeapDynamicMalloc
 *
 *  Purpose : Allocate a dynamic block of memory from the vertex array
 *            of the required size.
 *            Assumes support for NVIDIA vertex array range.
 *
 *  On entry: size - Constant RwUInt32 containing the requested block size
 *                   in bytes.
 *
 *            generateFence - Constant RwBool containing TRUE if a fence
 *                            is to be generated, FALSE if not.
 *
 *  On exit : Void pointer to the video memory allocated.
 * --------------------------------------------------------------------- */
void *
_rwOpenGLVertexHeapDynamicMalloc( const RwUInt32 size,
                                  const RwBool generateFence )
{
    openGLVertexHeapHeader  *freeBlock;


    RWFUNCTION( RWSTRING( "_rwOpenGLVertexHeapDynamicMalloc" ) );

    RWASSERT( _rwOpenGLVertexHeapAvailable() );

    /* get a free block, generate a fence */
    freeBlock = openGLVertexHeapGenericMalloc( size, generateFence );
    if ( NULL == freeBlock )
    {
        RWMESSAGE( (RWSTRING("Warning: Unable to malloc a dynamic block of size %d bytes"), size) );
        RWRETURN( (void *)NULL );
    }

    /* add the free block into the dynamic used block list */
    freeBlock->prev = (openGLVertexHeapHeader *)NULL;
    freeBlock->next = openGLDynamicUsedBlockStart;
    if ( NULL != openGLDynamicUsedBlockStart )
    {
        openGLDynamicUsedBlockStart->prev = freeBlock;
    }
    openGLDynamicUsedBlockStart = freeBlock;

    /* return a pointer to the video memory */
    RWRETURN( (void *)(freeBlock->videoMemory) );
}


/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLVertexHeapStaticMalloc
 *
 *  Purpose : Allocate a static block of memory from the vertex array
 *            of the required size.
 *            Assumes support for NVIDIA vertex array range.
 *
 *  On entry: size - Constant RwUInt32 containing the requested block size
 *                   in bytes.
 *
 *  On exit : Void pointer to the video memory allocated.
 * --------------------------------------------------------------------- */
void *
_rwOpenGLVertexHeapStaticMalloc( const RwUInt32 size )
{
    openGLVertexHeapHeader  *freeBlock;


    RWFUNCTION( RWSTRING( "_rwOpenGLVertexHeapStaticMalloc" ) );

    RWASSERT( _rwOpenGLVertexHeapAvailable() );

    /* get a free block, don't generate a fence */
    freeBlock = openGLVertexHeapGenericMalloc( size, FALSE );
    if ( NULL == freeBlock )
    {
        RWMESSAGE( (RWSTRING("Warning: Unable to malloc a static block of size %d bytes"), size) );
        RWRETURN( (void *)NULL );
    }

    /* add the free block into the static used block list */
    freeBlock->prev = (openGLVertexHeapHeader *)NULL;
    freeBlock->next = openGLStaticUsedBlockStart;
    if ( NULL != openGLStaticUsedBlockStart )
    {
        openGLStaticUsedBlockStart->prev = freeBlock;
    }
    openGLStaticUsedBlockStart = freeBlock;

    /* return a pointer to the video memory */
    RWRETURN( (void *)(freeBlock->videoMemory) );
}


/* ---------------------------------------------------------------------
 *  Function: openGLVertexHeapGenericFree
 *
 *  Purpose : Generic freeing of a NV vertex array heap block.
 *            Assumes support for NVIDIA vertex array range.
 *
 *  On entry: baseUsedHeader - Double pointer to openGLVertexHeapHeader referring
 *                             to the base used header, i.e. dynamic or static.
 *
 *            videoMemory - Void pointer to the video memory as returned
 *                          by _rwOpenGLVertexHeapDynamicMalloc.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
static void
openGLVertexHeapGenericFree( openGLVertexHeapHeader **baseUsedHeader,
                             void *videoMemory )
{
    openGLVertexHeapHeader  *usedBlock;


    RWFUNCTION( RWSTRING( "openGLVertexHeapGenericFree" ) );

    RWASSERT( _rwOpenGLVertexHeapAvailable() );

    if ( NULL == videoMemory )
    {
        RWMESSAGE( (RWSTRING("Error: Video memory pointer is invalid")) );
        RWRETURNVOID();
    }

    /* find the block containing this video memory */
    usedBlock = *baseUsedHeader;
    while ( NULL != usedBlock )
    {
        if ( videoMemory == usedBlock->videoMemory )
        {
            break;
        }

        usedBlock = usedBlock->next;
    }

    /* can't find the video memory */
    if ( NULL == usedBlock )
    {
#if defined(RWDEBUG)
        if ( openGLDynamicUsedBlockStart == *baseUsedHeader )
        {
            RWMESSAGE( (RWSTRING("Warning: Dynamic video memory at 0x%p appears to have been already freed"), videoMemory) );
        }
        else if ( openGLStaticUsedBlockStart == *baseUsedHeader )
        {
            RWMESSAGE( (RWSTRING("Warning: Static video memory at 0x%p appears be corrupted"), videoMemory) );
        }
#endif /* defined(RWDEBUG) */
        RWRETURNVOID();
    }

    openglVertexHeapBlockFree( baseUsedHeader, usedBlock );

    /* merge all free blocks where possible */
    openGLVertexHeapMergeFreeBlocks();

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLVertexHeapDynamicFree
 *
 *  Purpose : Frees a previously allocated dynamic vertex array block.
 *            Assumes support for NVIDIA vertex array range.
 *
 *  On entry: videoMemory - Void pointer to the video memory as returned
 *                          by _rwOpenGLVertexHeapDynamicMalloc.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
void
_rwOpenGLVertexHeapDynamicFree( void *videoMemory )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLVertexHeapDynamicFree" ) );

    openGLVertexHeapGenericFree( &openGLDynamicUsedBlockStart, videoMemory );

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLVertexHeapStaticFree
 *
 *  Purpose : Frees a previously allocated static vertex array block.
 *            Assumes support for NVIDIA vertex array range.
 *
 *  On entry: videoMemory - Void pointer to the video memory as returned
 *                          by _rwOpenGLVertexHeapDynamicMalloc.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
void
_rwOpenGLVertexHeapStaticFree( void *videoMemory )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLVertexHeapStaticFree" ) );

    openGLVertexHeapGenericFree( &openGLStaticUsedBlockStart, videoMemory );

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLVertexHeapSetNVFence
 *
 *  Purpose : Sets an NV fence on a previously allocated video memory.
 *            Assumes support for NVIDIA vertex array range.
 *
 *  On entry: videoMemory - Void pointer to the video memory as returned
 *                          by _rwOpenGLVertexHeapXXXMalloc.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
void
_rwOpenGLVertexHeapSetNVFence( void *videoMemory )
{
    openGLVertexHeapHeader  *usedBlock;


    RWFUNCTION( RWSTRING( "_rwOpenGLVertexHeapSetNVFence" ) );

    RWASSERT( _rwOpenGLVertexHeapAvailable() );

    if ( NULL == videoMemory )
    {
        RWMESSAGE( (RWSTRING("Error: Video memory pointer is invalid")) );
        RWRETURNVOID();
    }

    if ( FALSE != nvFenceAvailable )
    {
        /* find the dynamic block containing this video memory */
        usedBlock = openGLDynamicUsedBlockStart;
        while ( NULL != usedBlock )
        {
            if ( videoMemory == usedBlock->videoMemory )
            {
                break;
            }

            usedBlock = usedBlock->next;
        }

        /* if it's not dynamic, it might be static */
        if ( NULL == usedBlock )
        {
            /* find the static block containing this video memory */
            usedBlock = openGLStaticUsedBlockStart;
            while ( NULL != usedBlock )
            {
                if ( videoMemory == usedBlock->videoMemory )
                {
                    break;
                }

                usedBlock = usedBlock->next;
            }

            /* no luck, so early out, the block has probably been freed up
             * a dynamic memory purge, in which case it's already been
             * forced to finish */
            if ( NULL == usedBlock )
            {
                RWRETURNVOID();
            }
        }

        if ( OPENGLVERTEXHEAPBLOCK_ISFENCEVALID(usedBlock) )
        {
            _rwOpenGLExt.SetFenceNV( usedBlock->fence, GL_ALL_COMPLETED_NV );
        }
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLVertexHeapFinishNVFence
 *
 *  Purpose : Finish an NV fence on a previously allocated video memory.
 *            Assumes support for NVIDIA vertex array range.
 *
 *  On entry: videoMemory - Void pointer to the video memory as returned
 *                          by _rwOpenGLVertexHeapXXXMalloc.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
void
_rwOpenGLVertexHeapFinishNVFence( void *videoMemory )
{
    openGLVertexHeapHeader  *usedBlock;


    RWFUNCTION( RWSTRING( "_rwOpenGLVertexHeapFinishNVFence" ) );

    RWASSERT( _rwOpenGLVertexHeapAvailable() );

    if ( NULL == videoMemory )
    {
        RWMESSAGE( (RWSTRING("Error: Video memory pointer is invalid")) );
        RWRETURNVOID();
    }

    if ( FALSE != nvFenceAvailable )
    {
        /* find the dynamic block containing this video memory */
        usedBlock = openGLDynamicUsedBlockStart;
        while ( NULL != usedBlock )
        {
            if ( videoMemory == usedBlock->videoMemory )
            {
                break;
            }

            usedBlock = usedBlock->next;
        }

        /* if it's not dynamic, it might be static */
        if ( NULL == usedBlock )
        {
            /* find the static block containing this video memory */
            usedBlock = openGLStaticUsedBlockStart;
            while ( NULL != usedBlock )
            {
                if ( videoMemory == usedBlock->videoMemory )
                {
                    break;
                }

                usedBlock = usedBlock->next;
            }

            /* no luck, so early out, the block has probably been freed up
             * a dynamic memory purge, in which case it's already been
             * forced to finish */
            if ( NULL == usedBlock )
            {
                RWRETURNVOID();
            }
        }

        if ( OPENGLVERTEXHEAPBLOCK_ISFENCEVALID(usedBlock) )
        {
            OPENGLVERTEXHEAPBLOCK_FINISHFENCE( usedBlock );
        }
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLVertexHeapDynamicDiscard
 *
 *  Purpose : Discard a dynamic block ready for reuse.
 *            Assumes support for NVIDIA vertex array range.
 *
 *  On entry: videoMemory - Void pointer to the video memory as returned
 *                          by _rwOpenGLVertexHeapXXXMalloc.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
void
_rwOpenGLVertexHeapDynamicDiscard( void *videoMemory )
{
    openGLVertexHeapHeader  *usedBlock;


    RWFUNCTION( RWSTRING( "_rwOpenGLVertexHeapDynamicDiscard" ) );

    RWASSERT( _rwOpenGLVertexHeapAvailable() );

    if ( NULL == videoMemory )
    {
        RWMESSAGE( (RWSTRING("Error: Video memory pointer is invalid")) );
        RWRETURNVOID();
    }

    /* find the dynamic block containing this video memory */
    usedBlock = openGLDynamicUsedBlockStart;
    while ( NULL != usedBlock )
    {
        if ( videoMemory == usedBlock->videoMemory )
        {
            break;
        }

        usedBlock = usedBlock->next;
    }

    /* can't find the video memory */
    if ( NULL == usedBlock )
    {
        RWMESSAGE( (RWSTRING("Warning: Dynamic video memory at 0x%p appears to have been already freed"), videoMemory) );
        RWRETURNVOID();
    }

    usedBlock->reuse = TRUE;

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLVertexHeapTestAndPerformResize
 *
 *  Purpose : Tests whether a resize of the vertex array range buffer
 *            is required and performs the resize.
 *
 *  On entry: Nothing.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
void
_rwOpenGLVertexHeapTestAndPerformResize( void )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLVertexHeapTestAndPerformResize" ) );

    if ( FALSE != nvResizeRequired )
    {
        /* destroy all instanced data */
        RwResourcesEmptyArena();

        /* destroy the current vertex array buffer,
         * double it's size, and recreate it...
         * should go up in steps of 4, 8, 16, 32Mb, etc.
         * and if it ever fails just resort back to
         * system memory
         */
        _rwOpenGLVertexHeapDestroy();

        openGLVertexHeapSize *= 2;
        RWMESSAGE( (RWSTRING("Resizing NVIDIA vertex array range buffer "\
                             "to %d bytes"),
                   openGLVertexHeapSize) );

        if ( FALSE == _rwOpenGLVertexHeapCreate() )
        {
            RWMESSAGE( (RWSTRING("Unable to allocate NVIDIA vertex array range buffer."\
                                 "Falling back to system memory vertex arrays.")) );

            /* have to fall back to system memory */
            _rwOpenGLExt.FlushVertexArrayRangeNV = NULL;
            _rwOpenGLExt.VertexArrayRangeNV      = NULL;
            _rwOpenGLExt.AllocateMemoryNV        = NULL;
            _rwOpenGLExt.FreeMemoryNV            = NULL;

            _rwOpenGLExt.GenFencesNV             = NULL;
            _rwOpenGLExt.DeleteFencesNV          = NULL;
            _rwOpenGLExt.SetFenceNV              = NULL;
            _rwOpenGLExt.FinishFenceNV           = NULL;
            _rwOpenGLExt.IsFenceNV               = NULL;
            _rwOpenGLExt.TestFenceNV             = NULL;

            _rwOpenGLExt.VertexArrayRange2NV     = FALSE;
        }

        nvResizeRequired = FALSE;
    }

    RWRETURNVOID();
}
