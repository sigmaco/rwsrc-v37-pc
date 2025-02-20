/***********************************************************************
 *
 * Module:  ptankopengl.c
 *
 * Purpose: RpPTank for OpenGL
 *
 ***********************************************************************/

/* =====================================================================
 *  Includes
 * ===================================================================== */
#include "rwcore.h"
#include "rpworld.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "ptank.h"
#include "ptankprv.h"
#include "ptankg.h"

#include "ptankopengl.h"


/* =====================================================================
 *  Defines
 * ===================================================================== */
#define OPENGLRGBATOCOLOR(rgba)    (*(RwUInt32*)rgba)


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */


/* =====================================================================
 *  Static variables
 * ===================================================================== */
static RwUInt8  *openglPTankVertexData = (RwUInt8 *)NULL;

/**
 * \ingroup rpptankopengl
 *
 * \page rpptankopengloverview OpenGL RpPTank Overview
 *
 * To improve performance, the \c GL_QUAD primitive type is used to render
 * particles. Note that this primitive type is not supported generally by
 * RenderWare Graphics and is purely used for performance reasons internally
 * to \ref rpptank for OpenGL.
 */


/* =====================================================================
 *  Global variables
 * ===================================================================== */


/* =====================================================================
 *  Static function prototypes
 * ===================================================================== */

static RwBool
openglPTankCreateCB( RpAtomic *atomic,
                     RpPTankData *ptankGlobal,
                     RwInt32 maxPCount,
                     RwUInt32 dataFlags,
                     RwUInt32 platFlags );

static RwBool
openglPTankRenderCB( RpAtomic *atomic,
                     RpPTankData *ptankGlobal,
                     RwInt32 actPCount );

RpPTankCallBacks defaultCB =
{
    (RpPTankAllocCallBack)NULL,
    openglPTankCreateCB,
    _rpPTankGENInstance,
    openglPTankRenderCB
};

static void
openglPTankInsSetup( RpPTANKInstanceSetupData *data,
                     RpAtomic *atomic,
                     RpPTankData *ptankGlobal,
                     RwInt32 actPCount,
                     RwUInt32 instFlags );

static __inline void
SinCos( float angle, float *sine, float *cosine );


/* =====================================================================
 *  Global function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: PTankOpen
 *
 *  Purpose : Open the OpenGL specific RpPTank plugin.
 *
 *  On entry: object - Void pointer to the object initialized with RpPTank.
 *
 *            offset - Unused RwInt32.
 *
 *            size - Unused RwInt32.
 *
 *  On exit : Void pointer, or NULL if failed.
 * --------------------------------------------------------------------- */
void *
PTankOpen( void *object,
           RwInt32 __RWUNUSED__ offset,
           RwInt32 __RWUNUSED__ size )
{
    RWFUNCTION( RWSTRING( "PTankOpen" ) );

    RWASSERT( NULL != object );

    RWRETURN( object );
}


/* ---------------------------------------------------------------------
 *  Function: PTankClose
 *
 *  Purpose : Close the OpenGL specific RpPTank plugin.
 *
 *  On entry: object - Void pointer to the object de-initialized from RpPTank.
 *
 *            offset - Unused RwInt32.
 *
 *            size - Unused RwInt32.
 *
 *  On exit : Void pointer, or NULL if failed.
 * --------------------------------------------------------------------- */
void *
PTankClose( void *object,
            RwInt32 __RWUNUSED__ offset,
            RwInt32 __RWUNUSED__ size )
{
    RWFUNCTION( RWSTRING( "PTankClose" ) );

    RWASSERT( NULL != object );

    /* only need to free up a system memory vertex array */
    if ( !_rwOpenGLVertexHeapAvailable() )
    {
        if ( NULL != openglPTankVertexData )
        {
            RwFree( openglPTankVertexData );
            openglPTankVertexData = (RwUInt8 *)NULL;
        }
    }

    RWRETURN( object );
}


/* =====================================================================
 *  Static function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: openglPTankCreateCB
 *
 *  Purpose : OpenGL specific PTank creation. Sets up a dummy geometry
 *            and moves the setup callback to an OpenGL specific function
 *            in order to create vertex arrays.
 *
 *  On entry: atomic - Pointer to an RpAtomic.
 *
 *            ptankGlobal - Pointer to RpPTankData.
 *
 *            maxPCount - RwInt32 containing the maximum particle count.
 *                        (not used.)
 *
 *            dataFlags - RwUInt32 containing particle data flags.
 *
 *            platFlags - RwUInt32 containing platform specific particle flags.
 *                        (not used.)
 *
 *  On exit : RwBool, TRUE if successfully created the PTank data, or FALSE
 *            if failed.
 * --------------------------------------------------------------------- */
static RwBool
openglPTankCreateCB( RpAtomic *atomic,
                     RpPTankData *ptankGlobal,
                     RwInt32 __RWUNUSED__ maxPCount,
                     RwUInt32 dataFlags,
                     RwUInt32 __RWUNUSED__ platFlags )
{
    RpGeometryFlag      geomFlag;

    RpGeometry          *geometry;

    RpMaterial          *material;

    RwSurfaceProperties surfaceProps;

    RpTriangle          dummyTri;

    RpPTankAtomicExtPrv *ptankPrv;


    RWFUNCTION( RWSTRING( "openglPTankCreateCB" ) );

    RWASSERT( NULL != atomic );
    RWASSERT( NULL != ptankGlobal );
    RWASSERT( FALSE != RpAtomicIsPTank(atomic) );

    /* construct the geometry flags */
    geomFlag = rpGEOMETRYPOSITIONS |
               rpGEOMETRYTEXTURED;

    if ( 0 != (dataFlags & rpPTANKDFLAGNORMAL) )
    {
        geomFlag |= rpGEOMETRYLIGHT |
                    rpGEOMETRYNORMALS |
                    rpGEOMETRYMODULATEMATERIALCOLOR;
    }

    if ( 0 != (dataFlags & (rpPTANKDFLAGCOLOR | rpPTANKDFLAGVTXCOLOR | rpPTANKDFLAGCNSVTXCOLOR)) )
    {
        geomFlag |= rpGEOMETRYPRELIT;
    }

    /* create an empty geometry of radius 1.0f */
    geometry = RpGeometryCreateSpace( 1.0f );
    if ( NULL == geometry )
    {
        RWRETURN( FALSE );
    }

    /* set the geometry flags */
    RpGeometrySetFlags( geometry, geomFlag );

    /* create the material */
    material = RpMaterialCreate();
    if ( NULL == material )
    {
        RpGeometryDestroy( geometry );
        RWRETURN( FALSE );
    }

    /* set the surface reflection coefficients */
    surfaceProps.ambient  = 0.3f;
    surfaceProps.diffuse  = 1.0f;
    surfaceProps.specular = 1.0f;
    RpMaterialSetSurfaceProperties( material, &surfaceProps );

    /* add the material to a dummy triangle */
    RpGeometryTriangleSetMaterial( geometry, &dummyTri, material );

    /* associate the geometry with the atomic
     * this will increment the geometry's reference count by 1
     */
    RpAtomicSetGeometry( atomic, geometry, 0 );

    /* remove this ownership of the geometry and materials */
    RpGeometryDestroy( geometry );
    geometry = (RpGeometry *)NULL;

    RpMaterialDestroy( material );
    material = (RpMaterial *)NULL;

    /* initialize constant color to opaque black */
    ptankGlobal->cColor.red   = 0;
    ptankGlobal->cColor.green = 0;
    ptankGlobal->cColor.blue  = 0;
    ptankGlobal->cColor.alpha = 0xFF;

    _rpPTankGENInstanceSelectCB( atomic );

    /* instancing callback paths */
    ptankPrv = RPATOMICPTANKPLUGINDATA( atomic );
    ptankPrv->insSetupCB  = (RpPTankGENInstanceSetupCallback)openglPTankInsSetup;
    ptankPrv->insEndingCB = (RpPTankGENInstanceEndingCallback)NULL;

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: openglPTankRenderCB
 *
 *  Purpose : OpenGL custom render callback for particles. Uses GL_QUADS
 *            primitive.
 *
 *  On entry: atomic - Pointer to RpAtomic.
 *
 *            ptankGlobal - Pointer to RpPTankData.
 *
 *            actPCount - RwInt32 containing the actual particle count.
 *
 *  On exit : RwBool, TRUE if successful, FALSE otherwise.
 * --------------------------------------------------------------------- */
static RwBool
openglPTankRenderCB( RpAtomic *atomic,
                     RpPTankData *ptankGlobal,
                     RwInt32 actPCount )
{
    RpGeometry      *geometry;

    RpGeometryFlag  geomFlags;

    RwUInt32        stride;

    RpMaterial      *material;

    const RwRGBA    *matColor;

    RwBlendFunction srcBlend;

    RwBlendFunction dstBlend;


    RWFUNCTION( RWSTRING( "openglPTankRenderCB" ) );

    RWASSERT( NULL != atomic );
    RWASSERT( NULL != ptankGlobal );

    geometry = RpAtomicGetGeometry( atomic );
    RWASSERT( NULL != geometry);

    geomFlags = RpGeometryGetFlags( geometry );

    /* calculate the vertex stride */
    stride = sizeof(RwV3d);
    if ( 0 != (geomFlags & rxGEOMETRY_NORMALS) )
    {
        stride += sizeof(RwV3d);
    }
    if ( 0 != (geomFlags & rxGEOMETRY_PRELIT) )
    {
        stride += sizeof(RwRGBA);
    }
    if ( 0 != (geomFlags & rxGEOMETRY_TEXTURED) )
    {
        stride += sizeof(RwTexCoords);
    }

    material = RpGeometryGetMaterial( geometry, 0 );
    RWASSERT( NULL != material );

    matColor = RpMaterialGetColor( material );
    RWASSERT( NULL != matColor );

    _rxOpenGLDefaultAllInOneAtomicLightingCB( (void *)atomic );

    if ( FALSE != RwOpenGLIsEnabled( rwGL_LIGHTING ) )
    {
        /* set up the material properties */
        RpOpenGLWorldSetMaterialProperties( material,
                                            geomFlags );
    }
    else
    {
        RwOpenGLDisable( rwGL_COLOR_MATERIAL );
        if ( 0 == (geomFlags & rxGEOMETRY_PRELIT) )
        {
            RwUInt32    color;


            /* unlit but prelit geometry */
            color = OPENGLRGBATOCOLOR( &(ptankGlobal->cColor) );
            glColor4ubv( (const GLubyte *)&color );
        }
    }

    if ( 0 != (geomFlags & rxGEOMETRY_TEXTURED) )
    {
        RwOpenGLSetTexture( RpMaterialGetTexture( material ) );
    }
    else
    {
        RwOpenGLSetTexture( NULL );
    }

    if ( FALSE != ptankGlobal->vertexAlphaBlend )
    {
        _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE );

        _rwOpenGLGetRenderState( rwRENDERSTATESRCBLEND, (void *)&srcBlend );
        _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND, (void *)(ptankGlobal->srcBlend) );

        _rwOpenGLGetRenderState( rwRENDERSTATEDESTBLEND, (void *)&dstBlend );
        _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND, (void *)(ptankGlobal->dstBlend) );
    }
    else
    {
        _rwOpenGLSetRenderState( rwRENDERSTATEVERTEXALPHAENABLE, (void *)FALSE );
    }

    /* render */
    {
        const RwInt32   MaxVerticesPacket = (0xffff & (~4));

        RwInt32         numVertices;

        RwUInt32        firstVertex;

        const RwUInt8   *vertexMem;


        numVertices = actPCount * 4;
        firstVertex = 0;
        vertexMem = openglPTankVertexData;

        /* set up client state pointers */
        RwOpenGLVASetPosition( 3,
                               GL_FLOAT,
                               stride,
                               vertexMem );
        vertexMem += sizeof(RwV3d);

        if ( 0 != (geomFlags & rxGEOMETRY_NORMALS) )
        {
            RwOpenGLVASetNormal( TRUE,
                                 GL_FLOAT,
                                 stride,
                                 vertexMem );
            vertexMem += sizeof(RwV3d);
        }
        else
        {
            RwOpenGLVADisableNormal();
        }

        if ( 0 != (geomFlags & rxGEOMETRY_PRELIT) )
        {
            RwOpenGLVASetColor( TRUE,
                                ((FALSE == ptankGlobal->vertexAlphaBlend) ? 3 : 4),
                                GL_UNSIGNED_BYTE,
                                stride,
                                vertexMem );
            vertexMem += sizeof(RwRGBA);
        }
        else
        {
            RwOpenGLVADisableColor();
        }

        if ( 0 != (geomFlags & rxGEOMETRY_TEXTURED) )
        {
            RwOpenGLVASetTexCoord( TRUE,
                                   2,
                                   GL_FLOAT,
                                   stride,
                                   vertexMem );
            vertexMem += sizeof(RwTexCoords);
        }
        else
        {
            RwOpenGLVADisableTexCoord( 0 );
        }

        while ( 1 )
        {
            /* draw the quads */
            if ( numVertices <= MaxVerticesPacket )
            {
                glDrawArrays( GL_QUADS,
                              firstVertex,
                              numVertices );

#if defined(RWMETRICS)
                RWSRCGLOBAL(metrics)->numProcTriangles += numVertices / 2;
#endif /* defined(RWMETRICS) */

                break;
            }
            else
            {
                glDrawArrays( GL_QUADS,
                              firstVertex,
                              MaxVerticesPacket );

                numVertices -= MaxVerticesPacket;
                firstVertex += MaxVerticesPacket;

#if defined(RWMETRICS)
                RWSRCGLOBAL(metrics)->numProcTriangles += MaxVerticesPacket / 2;
#endif /* defined(RWMETRICS) */
            }
        }
    }

    if ( FALSE != ptankGlobal->vertexAlphaBlend )
    {
        _rwOpenGLSetRenderState( rwRENDERSTATESRCBLEND, (void *)srcBlend );
        _rwOpenGLSetRenderState( rwRENDERSTATEDESTBLEND, (void *)dstBlend );
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: openglPTankInsSetup
 *
 *  Purpose : OpenGL custom instance setup function. Used to create/prepare
 *            vertex arrays ready for general instancing.
 *
 *  On entry: data - Pointer to RpPTANKInstanceSetupData.
 *
 *            atomic - Pointer to RpAtomic.
 *
 *            ptankGlobal - Pointer to RpPTankData.
 *
 *            actPCount - RwInt32 containing the actual particle count.
 *
 *            instFlags - RwUInt32 containing instance flags.
 *                        (not used.)
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
static void
openglPTankInsSetup( RpPTANKInstanceSetupData *data,
                     RpAtomic *atomic,
                     RpPTankData *ptankGlobal,
                     RwInt32 actPCount,
                     RwUInt32 __RWUNUSED__ instFlags )
{
    RwInt32         numVertices;

    RpGeometry      *geometry;

    RpGeometryFlag  geomFlags;

    RpMaterial      *material;

    RwCamera        *camera;

    RwFrame         *cameraFrame;

    RwMatrix        *cameraLTM;

    RwUInt32        stride;

    RwV3d           *right;

    RwV3d           *up;

    RwUInt32        vertexArraySize;

    static RwUInt32 lastVertexArraySize = 0xFFFFFFFF;

    RwUInt32        offset;


    RWFUNCTION( RWSTRING( "openglPTankInsSetup" ) );

    RWASSERT( NULL != data );
    RWASSERT( NULL != atomic );
    RWASSERT( NULL != ptankGlobal );
    RWASSERT( FALSE != RpAtomicIsPTank(atomic) );

    numVertices = actPCount * 4;

    geometry = RpAtomicGetGeometry(atomic);
    RWASSERT( NULL != geometry );

    geomFlags = RpGeometryGetFlags(geometry);

    material = RpGeometryGetMaterial( geometry, 0 );
    RWASSERT( NULL != material );

    if ( NULL != material->texture )
    {
        if ( 0 == (geomFlags & rpGEOMETRYTEXTURED) )
        {
            geomFlags |= rpGEOMETRYTEXTURED;
            RpGeometrySetFlags( geometry, geomFlags );
        }
    }
    else
    {
        if ( 0 != (geomFlags & rpGEOMETRYTEXTURED) )
        {
            geomFlags &= ~rpGEOMETRYTEXTURED;
            RpGeometrySetFlags( geometry, geomFlags );
        }
    }

    /* current camera LTM */
    camera = (RwCamera *)RWSRCGLOBAL( curCamera );
    RWASSERT( NULL != camera );

    cameraFrame = RwCameraGetFrame( camera );
    RWASSERT( NULL != cameraFrame );

    cameraLTM = RwFrameGetLTM( cameraFrame );
    RWASSERT( NULL != cameraLTM );

    /* calculate the vertex stride */

    /* positions */
    stride = sizeof(RwV3d);
    data->instancePositions = TRUE;

    /* normals */
    if ( 0 != (geomFlags & rpGEOMETRYNORMALS) )
    {
        stride += sizeof(RwV3d);
        data->instanceNormals = TRUE;
    }

    /* prelight */
    if ( 0 != (geomFlags & rpGEOMETRYPRELIT) )
    {
        stride += sizeof(RwRGBA);
        data->instanceColors = TRUE;
    }

    /* texture coordinates */
    if ( 0 != (geomFlags & rpGEOMETRYTEXTURED) )
    {
        stride += sizeof(RwTexCoords);
        data->instanceUVs = TRUE;
    }

    /* set up the right and up vectors */
    right = &data->right;
    up = &data->up;

    if ( 0 != (ptankGlobal->format.dataFlags & rpPTANKDFLAGCNSMATRIX) )
    {
        RwV3dAssign( right, RwMatrixGetRight( &ptankGlobal->cMatrix ) );
        RwV3dAssign( up, RwMatrixGetUp( &ptankGlobal->cMatrix ) );
    }
    else if ( 0 != (ptankGlobal->format.dataFlags & rpPTANKDFLAGCNS2DROTATE) )
    {
        RwV3d   rightCamera;
        RwV3d   upCamera;

        RwReal  sinA;
        RwReal  cosA;

        rightCamera.x = -cameraLTM->right.x;
        rightCamera.y = -cameraLTM->right.y;
        rightCamera.z = -cameraLTM->right.z;

        upCamera.x = cameraLTM->up.x;
        upCamera.y = cameraLTM->up.y;
        upCamera.z = cameraLTM->up.z;

        SinCos( ptankGlobal->cRotate, &sinA, &cosA );

        /* calculate world space size vectors */
        right->x = (+ cosA) * rightCamera.x + (- sinA) * upCamera.x;
        right->y = (+ cosA) * rightCamera.y + (- sinA) * upCamera.y;
        right->z = (+ cosA) * rightCamera.z + (- sinA) * upCamera.z;

        up->x = (+ sinA) * rightCamera.x + (+ cosA) * upCamera.x;
        up->y = (+ sinA) * rightCamera.y + (+ cosA) * upCamera.y;
        up->z = (+ sinA) * rightCamera.z + (+ cosA) * upCamera.z;
    }
    else
    {
        right->x = -cameraLTM->right.x;
        right->y = -cameraLTM->right.y;
        right->z = -cameraLTM->right.z;

        up->x = cameraLTM->up.x;
        up->y = cameraLTM->up.y;
        up->z = cameraLTM->up.z;
    }

    vertexArraySize = stride * numVertices;

    /* allocate a vertex array */
    if ( _rwOpenGLVertexHeapAvailable() )
    {
        if ( NULL != openglPTankVertexData )
        {
            _rwOpenGLVertexHeapDynamicDiscard( openglPTankVertexData );
        }

        /* don't generate a fence on this block */
        openglPTankVertexData = (void *)_rwOpenGLVertexHeapDynamicMalloc( vertexArraySize, FALSE );
    }
    /* TODO: ATI support */
    else
    {
        if ( lastVertexArraySize != vertexArraySize )
        {
            if ( NULL != openglPTankVertexData )
            {
                RwFree( openglPTankVertexData );
            }
            openglPTankVertexData = (void *)RwMalloc( vertexArraySize,
                rwID_PTANKPLUGIN | rwMEMHINTDUR_EVENT );
            lastVertexArraySize = vertexArraySize;
        }
    }

    /* set up the vertex output pointers */
    if ( FALSE != data->instancePositions )
    {
        data->positionOut.data = (RwUInt8 *)openglPTankVertexData;
        data->positionOut.stride = stride;
    }
    offset = sizeof(RwV3d);

    if ( FALSE != data->instanceNormals )
    {
        data->normalsOut.data = (RwUInt8 *)((RwUInt8 *)(openglPTankVertexData) + offset);
        data->normalsOut.stride = stride;
    }
    if ( 0 != (geomFlags & rpGEOMETRYNORMALS) )
    {
        offset += sizeof(RwV3d);
    }

    if ( FALSE != data->instanceColors )
    {
        data->colorsOut.data = (RwUInt8 *)((RwUInt8 *)(openglPTankVertexData) + offset);
        data->colorsOut.stride = stride;
    }
    if ( 0 != (geomFlags & rpGEOMETRYPRELIT) )
    {
        offset += sizeof(RwRGBA);
    }

    if ( FALSE != data->instanceUVs )
    {
        data->UVOut.data = (RwUInt8 *)((RwUInt8 *)(openglPTankVertexData) + offset);
        data->UVOut.stride = stride;
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: SinCos
 *
 *  Purpose : (Fast) sin/cos calculations. Uses assembler if that is available.
 *
 *  On entry: angle - Float containing the angle to take the sin/cos of.
 *
 *            sine - Pointer to float to return sin(angle).
 *
 *            cosine - Pointer to float to return cos(angle).
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
static __inline void
SinCos( float angle __RWUNUSED__, float *sine __RWUNUSED__, float *cosine __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("SinCos"));

#if defined(NOASM)

    *sine = (RwReal)RwSin(angle);
    *cosine = (RwReal)RwCos(angle);

#else /* defined(NOASM) */

#if !defined(__GNUC__)
    __asm
    {
        fld     dword ptr angle
        fsincos
        mov     eax, sine
        mov     edx, cosine
        fstp    dword ptr [edx]
        fstp    dword ptr [eax]
    }
#else
#error "Cannot handle inline assembly with a GCC compiler"
#endif /* !defined(__GNUC__) */

#endif /* defined(NOASM) */

    RWRETURNVOID();
}
