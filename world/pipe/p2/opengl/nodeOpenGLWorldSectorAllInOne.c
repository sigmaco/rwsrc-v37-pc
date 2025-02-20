/***********************************************************************
 *
 * Module:  nodeOpenGLWorldSectorAllInOne.c
 *
 * Purpose: OpenGL All-In-One world sector pipelines
 *
 ***********************************************************************/

/* =====================================================================
 *  Includes
 * ===================================================================== */
#include "baworld.h"

#include "openglpipe.h"
#include "openglpipepriv.h"
#include "opengllights.h"
#include "nodeOpenGLWorldSectorAllInOne.h"


/* =====================================================================
 *  Defines
 * ===================================================================== */


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */


/* =====================================================================
 *  Static variables
 * ===================================================================== */

/* =====================================================================
 *  Global variables
 * ===================================================================== */


/* =====================================================================
 *  Static function prototypes
 * ===================================================================== */
static RwBool
openglWorldSectorAllInOnePipelineInit( RxPipelineNode *node );

static RwBool
openglWorldSectorAllInOneNode( RxPipelineNode * self,
                               const RxPipelineNodeParam *params );

static RwBool
openglDefaultWorldSectorInstanceCB( void *object,
                                    RxOpenGLMeshInstanceData *instanceData,
                                    const RwBool instanceDLandVA,
                                    const RwBool reinstance );

static void
openglDefaultWorldSectorLightingCB( void *object );

static void
openglAllInOneWorldSectorInstanceVertexArray( RxOpenGLMeshInstanceData *instanceData,
                                              const RpWorldSector *sector,
                                              RpWorldFlag worldFlags,
                                              RwInt32 numTexCoords,
                                              RwUInt8 *baseVertexMem );


/* =====================================================================
 *  Global function definitions
 * ===================================================================== */

/**
 * \ingroup worldextensionsopengl
 * \ref RxNodeDefinitionGetOpenGLWorldSectorAllInOne returns an
 * \ref RxNodeDefinition that can be used as a fragment in a custom OpenGL
 * world sector pipeline.
 *
 * The \ref rpworldsub plugin must be attached before using this function.
 *
 * \return \ref RxNodeDefinition for the OpenGL world sector All-In-One node.
 *
 * \see RxPipelineCreate
 * \see RxPipelineDestroy
 * \see RxPipelineLock
 * \see RxLockedPipeUnlock
 * \see RxLockedPipeAddFragment
 * \see RxPipelineFindNodeByName
 */
RxNodeDefinition *
RxNodeDefinitionGetOpenGLWorldSectorAllInOne( void )
{
    static RwChar           openglWorldSectorAllInOne_csl[] =
        RWSTRING( "OpenGLWorldSectorAllInOne.csl" );

    static RxNodeDefinition nodeOpenGLWorldSectorAllInOneCSL =
    {
        openglWorldSectorAllInOne_csl,                                      /* name */

        /* node methods */
        {
            (RxNodeBodyFn)openglWorldSectorAllInOneNode,                    /* +-- node body */
            (RxNodeInitFn)NULL,                                             /* +-- node init */
            (RxNodeTermFn)NULL,                                             /* +-- node term */
            (RxPipelineNodeInitFn)openglWorldSectorAllInOnePipelineInit,    /* +-- pipeline node init */
            (RxPipelineNodeTermFn)_rxOpenGLAllInOnePipelineTerm,            /* +-- pipeline node term */
            (RxPipelineNodeConfigFn)NULL,                                   /* +-- pipeline node config */
            (RxConfigMsgHandlerFn)NULL                                      /* +-- config message handler */
        },

        /* IO */
        {
            0,                                                              /* +-- num clusters of interest */
            (RxClusterRef *)NULL,                                           /* +-- clusters of interest */
            (RxClusterValidityReq *)NULL,                                   /* +-- requirements of input clusters */
            0,                                                              /* +-- num outputs */
            (RxOutputSpec *)NULL                                            /* +-- output specifications */
        },

        sizeof(_rxOpenGLAllInOnePrivateData),                               /* +-- size of private data */
        rxNODEDEFCONST,                                                     /* +-- editable? */
        0                                                                   /* +-- unlocked pipe count */
    };

    RWAPIFUNCTION( RWSTRING( "RxNodeDefinitionGetOpenGLWorldSectorAllInOne" ) );

    RWRETURN( &nodeOpenGLWorldSectorAllInOneCSL );
}



/* =====================================================================
 *  Static function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: openglWorldSectorAllInOnePipelineInit
 *
 *  Purpose : Initialize the world sector all-in-one pipeline node.
 *
 *  On entry: node - Pointer to RxPipelineNode.
 *
 *  On exit : RwBool, TRUE if successfully initialized the pipeline node
 *            or FALSE if failed.
 * --------------------------------------------------------------------- */
static RwBool
openglWorldSectorAllInOnePipelineInit( RxPipelineNode *node )
{
    _rxOpenGLAllInOnePrivateData    *privateData;


    RWFUNCTION( RWSTRING( "openglWorldSectorAllInOnePipelineInit" ) );

    RWASSERT( NULL != node );

    /* common all-in-one pipeline initialization */
    if ( FALSE == _rxOpenGLAllInOnePipelineInit(node) )
    {
        RWRETURN( FALSE );
    }

    privateData = (_rxOpenGLAllInOnePrivateData *)(node->privateData);
    RWASSERT( NULL != privateData );

    /* set the default callbacks for the world sector pipeline */
    privateData->instanceDLandVA = TRUE;   /* don't instance both display lists
                                              and vertex buffers by default */
    privateData->instanceCB      = openglDefaultWorldSectorInstanceCB;
    privateData->reinstanceCB    = NULL; /* worlds do not get reinstanced */
    privateData->lightingCB      = openglDefaultWorldSectorLightingCB;
    privateData->renderCB        = _rxOpenGLDefaultAllInOneRenderCB;

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: openglWorldSectorAllInOneNode
 *
 *  Purpose : OpenGL world sector All-In-One node body.
 *
 *  On entry: self - Pointer to RxPipelineNode.
 *
 *            params - Pointer to constant RxPipelineNodeParam.
 *
 *  On exit : RwBool, TRUE if successful, FALSE if not.
 * --------------------------------------------------------------------- */
static RwBool
openglWorldSectorAllInOneNode( RxPipelineNode * self,
                               const RxPipelineNodeParam *params )
{
    RpWorldSector                       *sector;

    RpMeshHeader                        *meshHeader;

    RwResEntry                          *resEntry;

    _rxOpenGLAllInOnePrivateData          *privateData;


    RWFUNCTION( RWSTRING( "openglWorldSectorAllInOneNode" ) );

    RWASSERT( NULL != self );
    RWASSERT( NULL != params );

    sector = (RpWorldSector *)RxPipelineNodeParamGetData( params );
    RWASSERT( NULL != sector );

    /* early out if there are no vertices */
    if ( sector->numVertices <= 0 )
    {
        RWRETURN( TRUE );
    }

    meshHeader = sector->mesh;

    /* early out if there are no meshes */
    if ( meshHeader->numMeshes <= 0 )
    {
        RWRETURN( TRUE );
    }

    /* if the meshes have changed, we should re-instance */
    resEntry = sector->repEntry;
    if ( NULL != resEntry )
    {
        RxOpenGLResEntryHeader  *resEntryHeader;


        resEntryHeader = (RxOpenGLResEntryHeader *)(resEntry + 1);

        if ( resEntryHeader->serialNumber != meshHeader->serialNum )
        {
            RwResourcesFreeResEntry( resEntry );
            resEntry = (RwResEntry *)NULL;
        }
    }

    privateData = (_rxOpenGLAllInOnePrivateData *)self->privateData;

    /* does the resource entry already exist? */
    if ( NULL != resEntry )
    {
        /* we have a resource entry, so use it */
        RwResourcesUseResEntry( resEntry );
    }
    else
    {
        resEntry = _rxOpenGLInstance( (void *)sector,
                                      (void *)sector,
                                      &(sector->repEntry),
                                      meshHeader,
                                      privateData->instanceDLandVA,
                                      privateData->instanceCB );

        if ( NULL == resEntry )
        {
            RWRETURN( FALSE );
        }
    }

    /* set up lights */
    if ( NULL != privateData->lightingCB )
    {
        privateData->lightingCB( (void *)sector );
    }

    /* disable normal normalization */
    RwOpenGLDisable( rwGL_NORMALIZE );

    /* render */
    if ( NULL != privateData->renderCB )
    {
        RwUInt32    worldFlags;


        worldFlags = RpWorldGetFlags( (RpWorld *)RWSRCGLOBAL(curWorld) );

        privateData->renderCB( resEntry, (void *)sector, rwSECTORATOMIC, worldFlags );
    }

    RwOpenGLDisable( rwGL_LIGHTING );

#if defined(RWMETRICS)
    RWSRCGLOBAL(metrics)->numVertices  += RpWorldSectorGetNumVertices(sector);
    RWSRCGLOBAL(metrics)->numTriangles += RpWorldSectorGetNumTriangles(sector);
#endif /* defined(RWMETRICS) */

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: openglDefaultWorldSectorInstanceCB
 *
 *  Purpose : Default OpenGL world sector Instance Callback.
 *
 *  On entry: object - Void pointer to the object.
 *
 *            instanceData - Pointer to RxOpenGLMeshInstanceData.
 *
 *            instanceDLandVA - Constant RwBool, TRUE if to instance both display
 *                              lists and vertex arrays, FALSE to choose the best
 *                              sort.
 *
 *            reinstance - Constant RwBool, TRUE to reinstance, FALSE to instance.
 *
 *  On exit : RwBool, TRUE if successfully instanced, FALSE if not.
 * --------------------------------------------------------------------- */
static RwBool
openglDefaultWorldSectorInstanceCB( void *object,
                                    RxOpenGLMeshInstanceData *instanceData,
                                    const RwBool instanceDLandVA,
                                    const RwBool reinstance __RWUNUSEDRELEASE__ )
{
    RpWorldSector   *sector;

    RwUInt32        worldFlags;

    RwInt32         numTexCoords;


    RWFUNCTION( RWSTRING( "openglDefaultWorldSectorInstanceCB" ) );

    RWASSERT( NULL != object );
    RWASSERT( NULL != instanceData );
    RWASSERT( FALSE == reinstance );    /* we don't reinstance world sectors */

    sector = (RpWorldSector *)object;

    worldFlags = RpWorldGetFlags( (RpWorld *)RWSRCGLOBAL(curWorld) );

    /* get the vertex descriptor */
    instanceData->vertexDesc = (RwUInt32)worldFlags;

    RWASSERT( GL_FALSE == glIsList( instanceData->displayList ) );

    numTexCoords = 0;
    if ( 0 != (worldFlags & rpWORLDTEXTURED) )
    {
        numTexCoords = 1;
    }
    if ( 0 != (worldFlags & rpWORLDTEXTURED2) )
    {
        numTexCoords = 2;
    }

    /* when NVIDIA VAR or ATI VAO is available, it's always better to
     * use vertex arrays */
    if ( !_rwOpenGLVertexHeapAvailable() &&
         (NULL == _rwOpenGLExt.NewObjectBufferATI) )
    {
        /* generate a display list */
        instanceData->displayList = glGenLists( 1 );
        RWASSERT( 0 != instanceData->displayList );

        /* compile the display list */
        _rxOpenGLInstanceDisplayList( instanceData,
                                      worldFlags,
                                      rwSECTORATOMIC,
                                      numTexCoords,
                                      (RwTexCoords **)((0 == numTexCoords) ? NULL : sector->texCoords),
                                      ((0 == (worldFlags & rpWORLDPRELIT)) ? (RwRGBA *)NULL : sector->preLitLum),
                                      ((0 == (worldFlags & rpWORLDNORMALS)) ? (void *)NULL : (void *)sector->normals),
                                      sector->vertices );
    }

    /* check if the display list instancing completed successfully
     * or if this pipeline requires both a display list and vertex array
     * instancing for special effects */
    if ( (GL_FALSE == glIsList( instanceData->displayList )) ||
         (FALSE != instanceDLandVA) )
    {
        RwUInt8 *baseVertexMem;


        /* position */
        instanceData->vertexStride = sizeof(RwV3d);

        /* normals */
        if ( 0 != (worldFlags & rxGEOMETRY_NORMALS) )
        {
            instanceData->vertexStride += sizeof(RwV3d);
        }

        /* prelight */
        if ( 0 != (worldFlags & rxGEOMETRY_PRELIT) )
        {
            instanceData->vertexStride += sizeof(RwRGBA);
        }

        /* texture coordinates */
        instanceData->vertexStride += numTexCoords * sizeof(RwTexCoords);

        instanceData->vertexDataSize = instanceData->numVertices * instanceData->vertexStride;

        /* allocate the vertex memory, either in system or video memory with the latter preferred */
        if ( _rwOpenGLVertexHeapAvailable() )
        {
            RWASSERT( NULL == instanceData->sysMemVertexData );

            instanceData->vidMemVertexData = (void *)_rwOpenGLVertexHeapStaticMalloc( instanceData->vertexDataSize );
            RWASSERT( NULL != instanceData->vidMemVertexData );
            if ( NULL == instanceData->vidMemVertexData )
            {
                RWERROR( (E_RW_NOMEM, instanceData->vertexDataSize) );
                RWRETURN( FALSE );
            }

            instanceData->vidMemDataIsStatic = TRUE;
            baseVertexMem = instanceData->vidMemVertexData;
        }
        else
        {
            RWASSERT( NULL == instanceData->vidMemVertexData );

            instanceData->sysMemVertexData = (RwUInt8 *)RwMalloc( instanceData->vertexDataSize,
                rwMEMHINTDUR_EVENT | rwID_WORLDPIPEMODULE);
            RWASSERT( NULL != instanceData->sysMemVertexData );
            if ( NULL == instanceData->sysMemVertexData )
            {
                RWERROR( (E_RW_NOMEM, instanceData->vertexDataSize) );
                RWRETURN( FALSE );
            }

            baseVertexMem = instanceData->sysMemVertexData;
        }

        openglAllInOneWorldSectorInstanceVertexArray( instanceData,
                                                      sector,
                                                      worldFlags,
                                                      numTexCoords,
                                                      baseVertexMem );

        /* ATI Vertex Array Object */
        if ( NULL != _rwOpenGLExt.NewObjectBufferATI )
        {
            RwUInt8 *offset;

            RwInt32 idx;


            instanceData->vaoName = _rwOpenGLExt.NewObjectBufferATI(
                                instanceData->vertexDataSize,
                                (const void *)instanceData->sysMemVertexData,
                                GL_STATIC_ATI );
            RWASSERT( GL_FALSE !=
                _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName ) );

            offset = 0;

            instanceData->position = offset;
            offset += sizeof(RwV3d);

            /* normals */
            if ( 0 != (worldFlags & rxGEOMETRY_NORMALS) )
            {
                instanceData->normal = offset;
                offset += sizeof(RwV3d);
            }

            /* prelight */
            if ( 0 != (worldFlags & rxGEOMETRY_PRELIT) )
            {
                instanceData->color = offset;
                offset += sizeof(RwRGBA);
            }

            /* texture coordinates */
            for ( idx = 0; idx < numTexCoords; idx += 1 )
            {
                instanceData->texCoord[idx] = offset;
                offset += sizeof(RwTexCoords);
            }
        }
        else
        {
            RwUInt8 *offset;

            RwInt32 idx;


            offset = (RwUInt8 *)baseVertexMem;

            instanceData->position = offset;
            offset += sizeof(RwV3d);

            /* normals */
            if ( 0 != (worldFlags & rxGEOMETRY_NORMALS) )
            {
                instanceData->normal = offset;
                offset += sizeof(RwV3d);
            }

            /* prelight */
            if ( 0 != (worldFlags & rxGEOMETRY_PRELIT) )
            {
                instanceData->color = offset;
                offset += sizeof(RwRGBA);
            }

            /* texture coordinates */
            for ( idx = 0; idx < numTexCoords; idx += 1 )
            {
                instanceData->texCoord[idx] = offset;
                offset += sizeof(RwTexCoords);
            }
        }
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: openglDefaultWorldSectorLightingCB
 *
 *  Purpose : Default OpenGL world sector Lighting Callback.
 *
 *  On entry: object - Void pointer to the object.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
static void
openglDefaultWorldSectorLightingCB( void *object )
{
    RwUInt32    worldFlags;


    RWFUNCTION( RWSTRING( "openglDefaultWorldSectorLightingCB" ) );

    RWASSERT( NULL != object );

    worldFlags = RpWorldGetFlags( (const RpWorld *)RWSRCGLOBAL(curWorld) );

    if ( 0 != (worldFlags & rxGEOMETRY_LIGHT) )
    {
        RpWorldSector   *sector;

        RwBool          lightingEnabled;

        RwLLLink        *curLight;

        RwLLLink        *endLight;


        sector = (RpWorldSector *)object;

        /* assume lighting is disabled until we have enumerated
         * all possible light sources */
        lightingEnabled = FALSE;

        /* global lights (ambient & directional) */
        lightingEnabled |= _rwOpenGLLightsGlobalLightsEnable(rpLIGHTLIGHTWORLD,TRUE);

        /* local lights (point, spot & soft spot) */

        /* increase the marker */
        RWSRCGLOBAL(lightFrame) += 1;

        /* lights in the sector */
        curLight = rwLinkListGetFirstLLLink( &sector->lightsInWorldSector );
        endLight = rwLinkListGetTerminator( &sector->lightsInWorldSector );

        while ( curLight != endLight )
        {
            RpLightTie  *lightTie;


            lightTie = rwLLLinkGetData( curLight, RpLightTie, lightInWorldSector );

            /* lightTie may actually be a dummyTie from an enclosing ForAll */

            /* check to see if the light has already been applied and is set to
             * light atomics */
            if ( (NULL != lightTie->light) &&
                 (0 != rwObjectTestFlags( lightTie->light, rpLIGHTLIGHTWORLD )) )
            {
                lightingEnabled |= _rwOpenGLLightsLocalLightEnable( lightTie->light );
            }

            /* next light */
            curLight = rwLLLinkGetNext( curLight );
        }

        /* now enable the lights required */
        _rwOpenGLLightsEnable( lightingEnabled );
    }
    else
    {
        /* disable all lights */
        _rwOpenGLLightsEnable( FALSE );
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglAllInOneWorldSectorInstanceVertexArray
 *
 *  Purpose : Instance world sector geometry into an OpenGL vertex array.
 *
 *  On entry:
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
static void
openglAllInOneWorldSectorInstanceVertexArray( RxOpenGLMeshInstanceData *instanceData,
                                              const RpWorldSector *sector,
                                              RpWorldFlag worldFlags,
                                              RwInt32 numTexCoords,
                                              RwUInt8 *baseVertexMem )
{
    RwUInt32        offset;


    RWFUNCTION( RWSTRING( "openglAllInOneWorldSectorInstanceVertexArray" ) );

    RWASSERT( NULL != instanceData );
    RWASSERT( NULL != sector );
    RWASSERT( NULL != baseVertexMem );

    /* positions */
    {
        const RwV3d *position;

        RwUInt8     *vertexData;

        RwUInt32    numVertices;


        position = &(sector->vertices[instanceData->minVertexIdx]);
        vertexData = baseVertexMem;
        numVertices = instanceData->numVertices;

        while ( numVertices-- )
        {
            *(RwV3d *)(vertexData) = *position;

            vertexData += instanceData->vertexStride;

            position += 1;
        }
    }

    offset = sizeof(RwV3d);

    /* normals */
    if ( 0 != (worldFlags & rpWORLDNORMALS) )
    {
        const RpVertexNormal    *normal;

        RwUInt8                 *vertexData;

        RwUInt32                numVertices;


        normal = &(sector->normals[instanceData->minVertexIdx]);
        vertexData = baseVertexMem + offset;
        numVertices = instanceData->numVertices;

        while ( numVertices-- )
        {
            RPV3DFROMVERTEXNORMAL( *((RwV3d *)vertexData), *normal );

            vertexData += instanceData->vertexStride;

            normal += 1;
        }

        offset += sizeof(RwV3d);
    }

    /* instance prelight */
    if ( 0 != (worldFlags & rpWORLDPRELIT) )
    {
        const RwRGBA    *color;

        const RwRGBA    *materialColor;

        RwUInt8         *vertexData;

        RwUInt32        numVertices;

        RwUInt8         alpha;


        color = (const RwRGBA *)( (&(sector->preLitLum[instanceData->minVertexIdx])) );
        RWASSERT( NULL != color );

        materialColor = RpMaterialGetColor( instanceData->material );
        RWASSERT( NULL != materialColor );

        vertexData = baseVertexMem + offset;
        numVertices = instanceData->numVertices;

        /* assume opaque */
        alpha = 0xFF;

        if ( (0 != (worldFlags & rpWORLDMODULATEMATERIALCOLOR)) &&
             (0xFFFFFFFF != *((const RwUInt32 *)materialColor)) )
        {
            while ( numVertices-- )
            {
                /* bits     info
                 * --------------
                 * 24-31    alpha
                 * 16-23    blue
                 * 8-15     green
                 * 0-7      red
                 */
                *((RwUInt32 *)vertexData) =
                    ( ((color->alpha * (materialColor->alpha + 1)) & 0xFF00) << 16) |
                    ( ((color->blue  * (materialColor->blue  + 1)) & 0xFF00) << 8 ) |
                    ( ((color->green * (materialColor->green + 1)) & 0xFF00)      ) |
                    ( ((color->red   * (materialColor->red   + 1)) & 0xFF00) >> 8 );

                alpha &= color->alpha;

                color += 1;

                vertexData += instanceData->vertexStride;
           };
        }
        else
        {
            while ( numVertices-- )
            {
                *((RwRGBA *)vertexData) = *color;

                alpha &= color->alpha;

                color += 1;

                vertexData += instanceData->vertexStride;
           };
        }

        instanceData->vertexAlpha = (alpha != 0xFF);

        offset += sizeof(RwRGBA);
    }
    else
    {
        instanceData->vertexAlpha = FALSE;
    }

    /* instance texture coordinates */
    if ( 0 != numTexCoords )
    {
        RwInt32     idx;


        RWASSERT( 0 != ((worldFlags & rpWORLDTEXTURED) || (worldFlags & rpWORLDTEXTURED2)) );

        for ( idx = 0; idx < numTexCoords; idx += 1 )
        {
            const RwTexCoords   *texCoord;

            RwUInt8             *vertexData;

            RwUInt32            numVertices;


            texCoord = (const RwTexCoords *)( &(sector->texCoords[idx][instanceData->minVertexIdx]) );

            vertexData = baseVertexMem + offset;
            numVertices = instanceData->numVertices;

            while ( numVertices-- )
            {
                *((RwTexCoords *)vertexData) = *texCoord;

                texCoord += 1;

                vertexData += instanceData->vertexStride;
            }

            offset += sizeof(RwTexCoords);
        }
    }

    RWRETURNVOID();
}
