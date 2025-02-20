/***********************************************************************
 *
 * Module:  crowdopengl.c
 *
 * Purpose: RpCrowd for OpenGL
 *
 ***********************************************************************/

/* =====================================================================
 *  Includes
 * ===================================================================== */
#include "rwcore.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "crowdcommon.h"
#include "crowdstaticbb.h"

#include "crowdopengl.h"


/* =====================================================================
 *  Defines
 * ===================================================================== */
#if defined(RWMETRICS)
#define UPDATERENDERCBMETRICS(_instData)                    \
MACRO_START                                                 \
    switch ( (_instData)->primType )                        \
    {                                                       \
    case GL_TRIANGLES:                                      \
        {                                                   \
            if ( NULL != (_instData)->indexData )           \
            {                                               \
                RWSRCGLOBAL(metrics)->numProcTriangles +=   \
                    (_instData)->numIndices / 3;            \
            }                                               \
            else                                            \
            {                                               \
                RWSRCGLOBAL(metrics)->numProcTriangles +=   \
                    (_instData)->numVertices / 3;           \
            }                                               \
        }                                                   \
        break;                                              \
                                                            \
    case GL_TRIANGLE_STRIP:                                 \
    case GL_TRIANGLE_FAN:                                   \
        {                                                   \
            if ( NULL != (_instData)->indexData )           \
            {                                               \
                RWSRCGLOBAL(metrics)->numProcTriangles +=   \
                    (_instData)->numIndices - 2;            \
            }                                               \
            else                                            \
            {                                               \
                RWSRCGLOBAL(metrics)->numProcTriangles +=   \
                    (_instData)->numVertices - 2;           \
            }                                               \
        }                                                   \
        break;                                              \
    }                                                       \
MACRO_STOP
#else /* defined(RWMETRICS) */
#define UPDATERENDERCBMETRICS(_instData)
#endif /* defined(RWMETRICS) */

/*
 *  NUMMATPERSEQ is the number of points in a sequence of an
 *  RpCrowdAnimation which are simultaneously referenced by materials
 *  of the crowd. This should be <= rpCROWDANIMATIONMAXSTEPS. Large
 *  numbers give a crowd more randomness, whereas smaller numbers might
 *  be more efficient.
 */
#define NUMMATPERSEQ        rpCROWDANIMATIONMAXSTEPS

#define NUMCROWDMATERIALS   (rpCROWDANIMATIONNUMSEQ * NUMMATPERSEQ)

#define OPENGLCROWDSORTINDICES(_idx,_in)    \
MACRO_START                                 \
{                                           \
    RwInt32 temp;                           \
    (_idx)[0] = (_in)[0];                   \
    (_idx)[1] = (_in)[1];                   \
    (_idx)[2] = (_in)[2];                   \
    if ( (_idx)[0] > (_idx)[1] )            \
    {                                       \
        temp = (_idx)[0];                   \
        (_idx)[0] = (_idx)[1];              \
        (_idx)[1] = temp;                   \
    }                                       \
    if ( (_idx)[1] > (_idx)[2] )            \
    {                                       \
        temp = (_idx)[1];                   \
        (_idx)[1] = (_idx)[2];              \
        (_idx)[2] = temp;                   \
        if ( (_idx)[0] > (_idx)[1] )        \
        {                                   \
            temp = (_idx)[0];               \
            (_idx)[0] = (_idx)[1];          \
            (_idx)[1] = temp;               \
        }                                   \
    }                                       \
}                                           \
MACRO_STOP


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */
typedef struct _rpOpenGLCrowdInstanceRow _rpOpenGLCrowdInstanceRow;
struct _rpOpenGLCrowdInstanceRow
{
    RxOpenGLMeshInstanceData    *mesh;

    RwUInt32                    rowIdx;
};


/* =====================================================================
 *  Static variables
 * ===================================================================== */
static _rpOpenGLCrowdInstanceRow    openglCrowdInstanceRenderRow[NUMCROWDMATERIALS];

static RwUInt32                     openglCrowdNumRows;


/* =====================================================================
 *  Global variables
 * ===================================================================== */
RwUInt32    _rpCrowdDeviceDataSize = sizeof( rpCrowdStaticBB );



/* =====================================================================
 *  Static function prototypes
 * ===================================================================== */
static RxPipeline *
openglCrowdPipeCreate( void );

static void
openglCrowdRenderCB( RwResEntry *repEntry,
                     void *object,
                     const RwUInt8 type,
                     const RwUInt32 flags );

static RpCrowd *
_rpCrowdOpenGLStaticBBCreate( RpCrowd *crowd,
                              rpCrowdStaticBB *staticBB,
                              RwUInt32 numMatPerSeq );

static int
openglCrowdSortTriangles( const void *pA,
                          const void *pB );

static int
openglCrowdInstanceMeshMaterialSort( const void *pA,
                                     const void *pB );

static RpCrowd *
_rpOpenGLCrowdStaticBBSetTextures( RpCrowd *crowd,
                                   rpCrowdStaticBB *staticBB );


/* =====================================================================
 *  Global function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: _rpCrowdDeviceOpen
 *
 *  Purpose : Create a platform specific crowd pipeline.
 *
 *  On entry: Nothing.
 *
 *  On exit : RwBool, TRUE if successful, FALSE otherwise.
 * --------------------------------------------------------------------- */
RwBool
_rpCrowdDeviceOpen( void )
{
    CrowdEngineData *engineData;


    RWFUNCTION( RWSTRING( "_rpCrowdDeviceOpen" ) );

    engineData = CROWDENGINEGETDATA( RwEngineInstance );
    RWASSERT( NULL != engineData );

    engineData->pipe = openglCrowdPipeCreate();
    if ( NULL == engineData->pipe )
    {
        RWRETURN( FALSE );
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rpCrowdDeviceClose
 *
 *  Purpose : Destroy the platform specific crowd pipeline.
 *
 *  On entry: Nothing.
 *
 *  On exit : RwBool, TRUE if successful, FALSE otherwise.
 * --------------------------------------------------------------------- */
RwBool
_rpCrowdDeviceClose( void )
{
    CrowdEngineData *engineData;


    RWFUNCTION( RWSTRING( "_rpCrowdDeviceClose" ) );

    engineData = CROWDENGINEGETDATA( RwEngineInstance );
    RWASSERT( NULL != engineData );

    if ( NULL != engineData->pipe )
    {
        RxPipelineDestroy( engineData->pipe );
        engineData->pipe = (RxPipeline *)NULL;
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rpCrowdDeviceInitCrowd
 *
 *  Purpose : Create platform specific data when a crowd is constructed.
 *
 *  On entry: crowd - Pointer to RpCrowd.
 *
 *  On exit : Pointer to RpCrowd, or NULL if failed.
 * --------------------------------------------------------------------- */
RpCrowd *
_rpCrowdDeviceInitCrowd( RpCrowd *crowd )
{
    rpCrowdStaticBB *staticBB;


    RWFUNCTION( RWSTRING( "_rpCrowdDeviceInitCrowd" ) );

    RWASSERT( NULL != crowd );

    staticBB = (rpCrowdStaticBB *)CROWDGETDEVICEDATA( crowd );
    RWASSERT( NULL != staticBB );

    if ( NULL == _rpCrowdOpenGLStaticBBCreate( crowd, staticBB, NUMMATPERSEQ ) )
    {
        RWRETURN( (RpCrowd *)NULL );
    }

    staticBB->atomic->pipeline = CROWDENGINEGETDATA( RwEngineInstance )->pipe;
    RWASSERT( NULL != staticBB->atomic->pipeline );

    RWRETURN( crowd );
}


/* ---------------------------------------------------------------------
 *  Function: _rpCrowdDeviceDeInitCrowd
 *
 *  Purpose : Destroy any platform specific data for a crowd.
 *
 *  On entry: crowd - Pointer to RpCrowd.
 *
 *  On exit : RwBool, TRUE if successful, FALSE otherwise.
 * --------------------------------------------------------------------- */
RwBool
_rpCrowdDeviceDeInitCrowd( RpCrowd *crowd )
{
    rpCrowdStaticBB *staticBB;


    RWFUNCTION( RWSTRING( "_rpCrowdDeviceDeInitCrowd" ) );

    RWASSERT( NULL != crowd );

    staticBB = (rpCrowdStaticBB *)CROWDGETDEVICEDATA( crowd );
    RWASSERT( NULL != staticBB );

    _rpCrowdStaticBBDestroy( crowd, staticBB );

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rpCrowdDeviceRenderCrowd
 *
 *  Purpose : Initialize and render platform specific crowd.
 *
 *  On entry: crowd - Pointer to RpCrowd.
 *
 *  On exit : Pointer to RpCrowd, or NULL if failed.
 * --------------------------------------------------------------------- */
RpCrowd *
_rpCrowdDeviceRenderCrowd( RpCrowd *crowd )
{
    rpCrowdStaticBB *staticBB;


    RWFUNCTION( RWSTRING( "_rpCrowdDeviceRenderCrowd" ) );

    RWASSERT( NULL != crowd );

    staticBB = (rpCrowdStaticBB *)CROWDGETDEVICEDATA( crowd );
    RWASSERT( NULL != staticBB );

    _rpOpenGLCrowdStaticBBSetTextures( crowd, staticBB );

    /* render the atomic */
    RpAtomicRender( staticBB->atomic );

    RWRETURN( crowd );
}


/* =====================================================================
 *  Static function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: openglCrowdPipeCreate
 *
 *  Purpose : Create the OpenGL crowd pipeline, with a custom render callback.
 *
 *  On entry: Nothing.
 *
 *  On exit : Pointer to RxPipeline containing the new pipeline.
 * --------------------------------------------------------------------- */
static RxPipeline *
openglCrowdPipeCreate( void )
{
    RxPipeline  *pipe;


    RWFUNCTION( RWSTRING( "openglCrowdPipeCreate" ) );

    pipe = RxPipelineCreate();
    if ( NULL != pipe )
    {
        RxLockedPipe    *lockedPipe;


        lockedPipe = RxPipelineLock( pipe );
        if ( NULL != lockedPipe )
        {
            RxNodeDefinition    *allinoneNodeDef;


            allinoneNodeDef = RxNodeDefinitionGetOpenGLAtomicAllInOne();
            RWASSERT( NULL != allinoneNodeDef );

            lockedPipe = RxLockedPipeAddFragment( lockedPipe, NULL, allinoneNodeDef, NULL );
            RWASSERT( NULL != lockedPipe );

            lockedPipe = RxLockedPipeUnlock( lockedPipe );

            RWASSERT( (RxPipeline *)lockedPipe == pipe );

            if ( NULL != lockedPipe )
            {
                RxPipelineNode  *pipeNode;


                pipeNode = RxPipelineFindNodeByName( pipe,
                                                     allinoneNodeDef->name,
                                                     NULL,
                                                     NULL );
                RWASSERT( NULL != pipeNode );

                RxOpenGLAllInOneSetRenderCallBack( pipeNode, openglCrowdRenderCB );

                RWRETURN( pipe );
            }
        }

        RxPipelineDestroy( pipe );
        pipe = (RxPipeline *)NULL;
    }

    RWRETURN( pipe );
}


/* ---------------------------------------------------------------------
 *  Function: openglCrowdRenderCB
 *
 *  Purpose : Custom render callback for OpenGL crowds. Does not use the
 *            split textures as defined in the default. Instead makes use
 *            of the GL_TEXTURE matrix mode to move across a single texture.
 *
 *  On entry: repEntry - Pointer to RwResEntry containing the resource arena
 *                       entry to be rendered.
 *
 *            object - Void pointer to the object to be rendered.
 *
 *            type - Constant RwUInt8 containing rpATOMIC or rwSECTORATOMIC.
 *
 *            flags - Constant RwUInt32 containing geometry flags.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
static void
openglCrowdRenderCB( RwResEntry *repEntry,
                     void *object __RWUNUSED__,
                     const RwUInt8 type __RWUNUSED__,
                     const RwUInt32 flags )
{
    static RwReal   texMatrix[16] =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    RxOpenGLResEntryHeader      *resEntryHeader;

    RxOpenGLMeshInstanceData    *instanceData;

    RwAlphaTestFunction         oldAlphaTest;

    RwInt32                     oldAlphaRef;

    RwUInt16                    numMeshes;

    RwUInt16                    idx;

    RwReal                      invNumRows;

    RwUInt16                    lastRowRendered;


    RWFUNCTION( RWSTRING( "openglCrowdRenderCB" ) );

    /* get the instanced data */
    resEntryHeader = (RxOpenGLResEntryHeader *)(repEntry + 1);
    RWASSERT( NULL != resEntryHeader );

    instanceData = (RxOpenGLMeshInstanceData *)(resEntryHeader + 1);
    RWASSERT( NULL != instanceData );

    /* let's pretend to be a PS2 */
    _rwOpenGLGetRenderState( rwRENDERSTATEALPHATESTFUNCTION, (void *)&oldAlphaTest );
    _rwOpenGLGetRenderState( rwRENDERSTATEALPHATESTFUNCTIONREF, (void *)&oldAlphaRef );

    _rwOpenGLSetRenderState( rwRENDERSTATEALPHATESTFUNCTION, (void *)rwALPHATESTFUNCTIONGREATER );
    _rwOpenGLSetRenderState( rwRENDERSTATEALPHATESTFUNCTIONREF, (void *)128 );

    numMeshes = resEntryHeader->numMeshes;

    /* preprocess the meshes to avoid material changes */
    for ( idx = 0; idx < numMeshes; idx += 1 )
    {
        openglCrowdInstanceRenderRow[idx].mesh = instanceData + idx;
    }

    invNumRows = (RwReal)(1.0f) / (RwReal)openglCrowdNumRows;

    qsort( (void *)openglCrowdInstanceRenderRow,
           numMeshes,
           sizeof(_rpOpenGLCrowdInstanceRow),
           openglCrowdInstanceMeshMaterialSort );

    /* reset the instanceData pointer */
    instanceData = openglCrowdInstanceRenderRow[0].mesh;

    if ( 0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2)) )
    {
        RwOpenGLSetTexture( RpMaterialGetTexture( instanceData->material ) );
    }
    else
    {
        RwOpenGLSetTexture( NULL );
    }

    idx = 0;
    lastRowRendered = 0xFFFF;

#if defined(RWDEBUG)
    {
        GLint   curOpenGLMatrixMode;


        glGetIntegerv( GL_MATRIX_MODE, &curOpenGLMatrixMode );
        RWASSERT( GL_MODELVIEW == curOpenGLMatrixMode );
    }
#endif /* defined(RWDEBUG) */

    /* change to texture matrix stack */
    glMatrixMode( GL_TEXTURE );
    glPushMatrix();

    while ( numMeshes-- )
    {
        const RwRGBA    *matColor;

        RwBool          vertexAlphaEnabled;


        RWASSERT( NULL != instanceData->material );

        matColor = RpMaterialGetColor( instanceData->material );
        RWASSERT( NULL != matColor );

        if ( (FALSE != instanceData->vertexAlpha) ||
             (0xFF != matColor->alpha) )
        {
            vertexAlphaEnabled = TRUE;
        }
        else
        {
            vertexAlphaEnabled = FALSE;
        }
        _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)vertexAlphaEnabled );

        if ( FALSE != RwOpenGLIsEnabled( rwGL_LIGHTING ) )
        {
            /* set up the material properties */
            RpOpenGLWorldSetMaterialProperties( instanceData->material,
                                                flags );
        }
        else
        {
            RwOpenGLDisable( rwGL_COLOR_MATERIAL );
            if ( 0 == (flags & rxGEOMETRY_PRELIT) )
            {
                /* black */
                glColor4f( 0.0f, 0.0f, 0.0f, 1.0f );
            }
        }

        /* texture matrix */
        if ( lastRowRendered != openglCrowdInstanceRenderRow[idx].rowIdx )
        {
            /* note the difference here between OpenGL and D3D8
             * OpenGL's texture coordinates s, t, r, q are defined such that
             * q = 1 unless the developer explicitly specifies it's value
             * in D3D8, the homogeneous coordinate appears to be the first available
             * thus, in D3D8 the matrix required changes in elements (2,2) and (3,2)
             * whereas OpenGL required changes in elements (2,2) and (4,2) (not yet
             * taking into account the tranposition of the matrices).
             */
            texMatrix[5] = invNumRows;
            texMatrix[13] = (RwReal)(openglCrowdInstanceRenderRow[idx].rowIdx) * invNumRows;

            glLoadMatrixf( (const GLfloat *)texMatrix );

            lastRowRendered = openglCrowdInstanceRenderRow[idx].rowIdx;
        }

        if ( 0 != instanceData->displayList )
        {
            RWASSERT( GL_FALSE != glIsList( instanceData->displayList ) );

            /* execute the display list */
            glCallList( instanceData->displayList );
        }
        else if ( (NULL != _rwOpenGLExt.ArrayObjectATI) &&
                  (GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName )) )
        {
            RWASSERT( GL_FALSE !=
                      _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName ) );

            /* set up client state pointers */
            RwOpenGLVASetPositionATI( 3,
                                      GL_FLOAT,
                                      instanceData->vertexStride,
                                      instanceData->vaoName,
                                      instanceData->position );

            RwOpenGLVASetNormalATI( (0 != (flags & rxGEOMETRY_NORMALS)),
                                    GL_FLOAT,
                                    instanceData->vertexStride,
                                    instanceData->vaoName,
                                    instanceData->normal );

            RwOpenGLVASetColorATI( (0 != (flags & rxGEOMETRY_PRELIT)),
                                   ((FALSE == vertexAlphaEnabled) ? 3 : 4),
                                   GL_UNSIGNED_BYTE,
                                   instanceData->vertexStride,
                                   instanceData->vaoName,
                                   instanceData->color );

            RwOpenGLVASetTexCoordATI( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))),
                                      2,
                                      GL_FLOAT,
                                      instanceData->vertexStride,
                                      instanceData->vaoName,
                                      instanceData->texCoord[0] );

            /* render */
            if ( NULL != instanceData->indexData )
            {
                glDrawElements( instanceData->primType,
                                instanceData->numIndices,
                                GL_UNSIGNED_INT,
                                (const GLvoid *)(instanceData->indexData) );
            }
            else
            {
                glDrawArrays( instanceData->primType,
                              instanceData->minVertexIdx,
                              instanceData->numVertices );
            }
        }
        else
        {
            /* set up client state pointers */
            RwOpenGLVASetPosition( 3,
                                   GL_FLOAT,
                                   instanceData->vertexStride,
                                   instanceData->position );

            RwOpenGLVASetNormal( (0 != (flags & rxGEOMETRY_NORMALS)),
                                 GL_FLOAT,
                                 instanceData->vertexStride,
                                 instanceData->normal );

            RwOpenGLVASetColor( (0 != (flags & rxGEOMETRY_PRELIT)),
                                ((FALSE == vertexAlphaEnabled) ? 3 : 4),
                                GL_UNSIGNED_BYTE,
                                instanceData->vertexStride,
                                instanceData->color );

            RwOpenGLVASetTexCoord( (0 != (flags & (rxGEOMETRY_TEXTURED | rpGEOMETRYTEXTURED2))),
                                   2,
                                   GL_FLOAT,
                                   instanceData->vertexStride,
                                   instanceData->texCoord[0] );

            /* render */
            if ( NULL != instanceData->indexData )
            {
                glDrawElements( instanceData->primType,
                                instanceData->numIndices,
                                GL_UNSIGNED_INT,
                                (const GLvoid *)(instanceData->indexData) );
            }
            else
            {
                glDrawArrays( instanceData->primType,
                              instanceData->minVertexIdx,
                              instanceData->numVertices );
            }

            /* video memory specific additions */
            if ( NULL != instanceData->vidMemVertexData )
            {
                if ( _rwOpenGLVertexHeapAvailable() )
                {
                    _rwOpenGLVertexHeapSetNVFence( instanceData->vidMemVertexData );
                }
            }
        }

        UPDATERENDERCBMETRICS(instanceData);

        /* next RxOpenGLMeshInstanceData pointer */
        idx += 1;
        instanceData = openglCrowdInstanceRenderRow[idx].mesh;
    }

    glPopMatrix();

    /* go back to modelview matrix stack */
    glMatrixMode( GL_MODELVIEW );

    /* restore previous alpha test */
    _rwOpenGLSetRenderState( rwRENDERSTATEALPHATESTFUNCTION, (void *)oldAlphaTest );
    _rwOpenGLSetRenderState( rwRENDERSTATEALPHATESTFUNCTIONREF, (void *)oldAlphaRef );

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rpCrowdOpenGLStaticBBCreate
 *
 *  Purpose : Custom OpenGL instancing of crowds, based on _rpCrowdStaticBBCreate
 *            but not splitting the crowd textures, and including a tri-sort.
 *
 *  On entry: crowd - Pointer to RpCrowd.
 *
 *            staticBB - Pointer to rpCrowdStaticBB.
 *
 *            numMatPerSeq - RwUInt32 containing the  number of materials
 *                           per sequence.
 *
 *  On exit : Pointer to RpCrowd, or NULL if failed.
 * --------------------------------------------------------------------- */
static RpCrowd *
_rpCrowdOpenGLStaticBBCreate( RpCrowd *crowd,
                              rpCrowdStaticBB *staticBB,
                              RwUInt32 numMatPerSeq )
{
    RwUInt32    numMaterials;

    RpGeometry  *crowdGeom;

    RwFrame     *frame;

    RwFrame     *baseFrame;

    RwUInt32    numMeshes;

    RpMesh      *mesh;


    RWFUNCTION( RWSTRING( "_rpCrowdOpenGLStaticBBCreate" ) );

    RWASSERT( NULL != crowd );
    RWASSERT( NULL != staticBB );

    /* initialize */
    staticBB->splitTextures = (RpCrowdTexture ***)NULL;
    staticBB->materials = (RpMaterial **)NULL;
    staticBB->atomic = (RpAtomic *)NULL;
    staticBB->numMatPerSeq = numMatPerSeq;
    numMaterials = numMatPerSeq * rpCROWDANIMATIONNUMSEQ;

    /* *** difference from default: no split textures *** */

    /* create material array for animation cyling */
    staticBB->materials = _rpCrowdStaticBBCreateCrowdMaterials(numMaterials);
    if ( NULL == staticBB->materials )
    {
        _rpCrowdStaticBBDestroy( crowd, staticBB );
        RWRETURN( (RpCrowd *)NULL );
    }

    /* Create the crowd atomic and geometry */
    staticBB->atomic = RpAtomicCreate();
    if ( NULL == staticBB->atomic )
    {
        _rpCrowdStaticBBDestroy( crowd, staticBB );
        RWRETURN( (RpCrowd *)NULL );
    }

    crowdGeom = _rpCrowdStaticBBCreateCrowdGeometry(crowd);
    if ( NULL == crowdGeom )
    {
        _rpCrowdStaticBBDestroy( crowd, staticBB );
        RWRETURN( (RpCrowd *)NULL );
    }

    /* randomly assign materials to the billboards */
    _rpCrowdStaticBBAssignCrowdMaterials( staticBB->materials, numMaterials, crowdGeom );

    /* now that materials have been assigned we can unlock */
    RpGeometryUnlock(crowdGeom);

    /* *** difference from default: triangles sorted *** */
    numMeshes = crowdGeom->mesh->numMeshes;
    mesh = (RpMesh *)(crowdGeom->mesh + 1);
    while ( numMeshes-- )
    {
        qsort( mesh->indices,
               (mesh->numIndices / 3),
               (3 * sizeof(RxVertexIndex)),
               openglCrowdSortTriangles );
    }

    /* new billboard atomic needs its own frame */
    frame = RwFrameCreate();
    if ( NULL == frame )
    {
        RpGeometryDestroy(crowdGeom);
        _rpCrowdStaticBBDestroy(crowd, staticBB);
        RWRETURN( (RpCrowd *)NULL );
    }

    /* copy the LTM from the base geometry */
    RWASSERT( NULL != crowd->param.base );
    baseFrame = RpAtomicGetFrame(crowd->param.base);
    if ( NULL != baseFrame )
    {
        *RwFrameGetMatrix(frame) = *RwFrameGetLTM(baseFrame);
    }
    else
    {
        RwMatrixSetIdentity( RwFrameGetMatrix(frame) );
    }

    /* setup atomic */
    RwFrameUpdateObjects(frame);
    RpAtomicSetFrame(staticBB->atomic, frame);
    RpAtomicSetGeometry(staticBB->atomic, crowdGeom, 0);
    RpGeometryDestroy(crowdGeom);

    RWRETURN( crowd );
}


/* ---------------------------------------------------------------------
 *  Function: openglCrowdSortTriangles
 *
 *  Purpose : qsort callback to sort triangles.
 *
 *  On entry: pA - Constant void pointer to the first data to sort.
 *
 *            pB - Constant void pointer to the seconda data to sort.
 *
 *  On exit : Signed integer indicating the need to swap data.
 * --------------------------------------------------------------------- */
static int
openglCrowdSortTriangles( const void *pA,
                          const void *pB )
{
    RwInt32             sortedIndexA[3];

    RwInt32             sortedIndexB[3];


    RWFUNCTION( RWSTRING( "openglCrowdSortTriangles" ) );

    OPENGLCROWDSORTINDICES( sortedIndexA, (const RxVertexIndex *)pA );
    OPENGLCROWDSORTINDICES( sortedIndexB, (const RxVertexIndex *)pB );

    if ( sortedIndexA[0] == sortedIndexB[0] )
    {
        if ( sortedIndexA[1] == sortedIndexB[1] )
        {
            RWRETURN( sortedIndexA[2] - sortedIndexB[2] );
        }

        RWRETURN( sortedIndexA[1] - sortedIndexB[1] );
    }
    else
    {
        RWRETURN( sortedIndexA[0] - sortedIndexB[0] );
    }
}


/* ---------------------------------------------------------------------
 *  Function: openglCrowdInstanceMeshMaterialSort
 *
 *  Purpose : qsort callback to sort materials.
 *
 *  On entry: pA - Constant void pointer to the first data to sort.
 *
 *            pB - Constant void pointer to the seconda data to sort.
 *
 *  On exit : Signed integer indicating the need to swap data.
 * --------------------------------------------------------------------- */
static int
openglCrowdInstanceMeshMaterialSort( const void *pA,
                                     const void *pB )
{
    const _rpOpenGLCrowdInstanceRow *elem1;

    const _rpOpenGLCrowdInstanceRow *elem2;


    RWFUNCTION( RWSTRING( "openglCrowdInstanceMeshMaterialSort" ) );

    elem1 = (const _rpOpenGLCrowdInstanceRow *)pA;
    elem2 = (const _rpOpenGLCrowdInstanceRow *)pB;

    /* sort by texture */
    RWRETURN( (RwInt32)(elem1->rowIdx) - (RwInt32)(elem2->rowIdx) );
}


/* ---------------------------------------------------------------------
 *  Function: _rpOpenGLCrowdStaticBBSetTextures
 *
 *  Purpose : Custom OpenGL setting of crowd textured, based on the default
 *            _rpCrowdStaticBBSetTextures. Does not use the split textures
 *            and initializes other variables used in the render callback.
 *
 *  On entry: crowd - Pointer to RpCrowd.
 *
 *            staticBB - Pointer to rpCrowdStaticBB.
 *
 *  On exit : Pointer to RpCrowd, or NULL if failed.
 * --------------------------------------------------------------------- */
static RpCrowd *
_rpOpenGLCrowdStaticBBSetTextures( RpCrowd *crowd,
                                   rpCrowdStaticBB *staticBB )
{
    RpCrowdAnimation    *anim;

    RwUInt32            cycle;

    RwReal              mapping;

    RwUInt32            i;


    RWFUNCTION( RWSTRING( "_rpOpenGLCrowdStaticBBSetTextures" ) );

    RWASSERT( NULL != crowd );
    RWASSERT( NULL != staticBB );

    /* get current animation */
    RWASSERT( NULL != crowd->param.anims );
    RWASSERT( crowd->currAnim < crowd->param.numAnims );
    anim = crowd->param.anims[crowd->currAnim];
    RWASSERT( NULL != anim );

    /* get current textures */
    /* *** difference from default: no split textures *** */
    RWASSERT( crowd->currTexture < crowd->param.numTextures );

    /* find the cycle number within the animation sequence */
    RWASSERT( crowd->currAnimTime >= 0.0f );
    RWASSERT( crowd->currAnimTime <= anim->duration );
    cycle = RwInt32FromRealMacro( (crowd->currAnimTime / anim->duration) *
                                   anim->numSteps );

    /* set current textures on all the materials */
    mapping = (RwReal)anim->numSteps / (RwReal)staticBB->numMatPerSeq;

    for ( i = 0; i < staticBB->numMatPerSeq; i += 1 )
    {
        RwInt32 step;

        RwInt32 j;


        /* map to frame index in sequences array */
        step = RwInt32FromRealMacro(i * mapping) + cycle;
        if ( step >= anim->numSteps )
        {
            step -= anim->numSteps;
        }

        /* set material textures for current step of each sequence */
        for ( j = 0; j < rpCROWDANIMATIONNUMSEQ; j += 1)
        {
            RwInt32 iMat;

            RwInt32 iRow;


            iMat = i * rpCROWDANIMATIONNUMSEQ + j;

            iRow = anim->sequences[step][j];

            /* *** difference from default *** */
            openglCrowdInstanceRenderRow[iMat].rowIdx = iRow;

            RpMaterialSetTexture( staticBB->materials[iMat],
                                  crowd->param.textures[crowd->currTexture] );
        }
    }

    /* *** difference from default *** */
    /* current number of rows to render */
    openglCrowdNumRows = crowd->param.numTexRows;

    RWRETURN( crowd );
}
