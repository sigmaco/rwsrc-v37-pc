/***********************************************************************
 *
 * Module:  nodeOpenGLAtomicAllInOne.c
 *
 * Purpose: OpenGL All-In-One world atomic pipelines
 *
 ***********************************************************************/

/* =====================================================================
 *  Includes
 * ===================================================================== */

#if defined(WIN32)
#include <windows.h>
#endif /* defined(WIN32) */
#include <gl/gl.h>

#include "baworld.h"
#include "openglpipe.h"
#include "openglpipepriv.h"
#include "opengllights.h"
#include "nodeOpenGLAtomicAllInOne.h"


/* =====================================================================
 *  Defines
 * ===================================================================== */
#define FLOATASINT(f) (*((const RwInt32 *)&(f)))


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
openglAtomicAllInOnePipelineInit( RxPipelineNode *node );

static RwBool
openglDefaultAtomicInstanceCB( void *object,
                               RxOpenGLMeshInstanceData *instanceData,
                               const RwBool instanceDLandVA,
                               const RwBool reinstance );

static RwBool
openglDefaultAtomicReinstanceCB( void *object,
                                 RwResEntry *resEntry,
                                 const RpMeshHeader *meshHeader,
                                 const RwBool instanceDLandVA,
                                 RxOpenGLAllInOneInstanceCallBack instanceCB );

static void
openglUpdateMorphInstance( RwResEntry *resEntry,
                           const RpMeshHeader *meshHeader,
                           RpAtomic *atomic,
                           RpGeometry *geometry,
                           const RpInterpolator *interp,
                           RxOpenGLMeshInstanceData *instanceData );


/* =====================================================================
 *  Global function definitions
 * ===================================================================== */

/**
 * \ingroup worldextensionsopengl
 * \ref RxNodeDefinitionGetOpenGLAtomicAllInOne returns an
 * \ref RxNodeDefinition that can be used as a fragment in a custom OpenGL
 * atomic pipeline.
 *
 * The \ref rpworldsub plugin must be attached before using this function.
 *
 * \return \ref RxNodeDefinition for the OpenGL atomic All-In-One node.
 *
 * \see RxPipelineCreate
 * \see RxPipelineDestroy
 * \see RxPipelineLock
 * \see RxLockedPipeUnlock
 * \see RxLockedPipeAddFragment
 * \see RxPipelineFindNodeByName
 */
RxNodeDefinition *
RxNodeDefinitionGetOpenGLAtomicAllInOne( void )
{
    static RwChar           openglAtomicAllInOne_csl[] = RWSTRING( "OpenGLAtomicAllInOne.csl" );

    static RxNodeDefinition nodeOpenGLAtomicAllInOneCSL =
    {
        openglAtomicAllInOne_csl,                                   /* name */

        /* node methods */
        {
            (RxNodeBodyFn)_rxOpenGLDefaultAtomicAllInOneNode,       /* +-- node body */
            (RxNodeInitFn)NULL,                                     /* +-- node init */
            (RxNodeTermFn)NULL,                                     /* +-- node term */
            (RxPipelineNodeInitFn)openglAtomicAllInOnePipelineInit, /* +-- pipeline node init */
            (RxPipelineNodeTermFn)_rxOpenGLAllInOnePipelineTerm,    /* +-- pipeline node term */
            (RxPipelineNodeConfigFn)NULL,                           /* +-- pipeline node config */
            (RxConfigMsgHandlerFn)NULL                              /* +-- config message handler */
        },

        /* IO */
        {
            0,                                                      /* +-- num clusters of interest */
            (RxClusterRef *)NULL,                                   /* +-- clusters of interest */
            (RxClusterValidityReq *)NULL,                           /* +-- requirements of input clusters */
            0,                                                      /* +-- num outputs */
            (RxOutputSpec *)NULL                                    /* +-- output specifications */
        },

        sizeof(_rxOpenGLAllInOnePrivateData),                       /* +-- size of private data */
        rxNODEDEFCONST,                                             /* +-- editable? */
        0                                                           /* +-- unlocked pipe count */
    };

    RWAPIFUNCTION( RWSTRING( "RxNodeDefinitionGetOpenGLAtomicAllInOne" ) );

    RWRETURN( &nodeOpenGLAtomicAllInOneCSL );
}


/* ---------------------------------------------------------------------
 *  Function: _rxOpenGLAllInOneAtomicInstanceVertexArray
 *
 *  Purpose : Instance atomic geometry into an OpenGL vertex array.
 *
 *  On entry:
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
void
_rxOpenGLAllInOneAtomicInstanceVertexArray( RxOpenGLMeshInstanceData *instanceData,
                                            RpAtomic *atomic,
                                            const RpGeometry *geometry,
                                            RpGeometryFlag geomFlags,
                                            RwInt32 numTexCoords,
                                            const RwBool reinstance,
                                            RxVertexIndex *remapTable,
                                            RwUInt8 *baseVertexMem )
{
    const RpInterpolator    *interp;

    RwInt32                 startMT;

    RwInt32                 endMT;

    RwUInt32                offset;


    RWFUNCTION( RWSTRING( "_rxOpenGLAllInOneAtomicInstanceVertexArray" ) );

    RWASSERT( NULL != instanceData );
    RWASSERT( NULL != atomic );
    RWASSERT( NULL != geometry );
    RWASSERT( NULL != baseVertexMem );

    interp = (const RpInterpolator *)RpAtomicGetInterpolator( atomic );
    RWASSERT( NULL != interp );

    startMT = RpInterpolatorGetStartMorphTarget( interp );
    endMT = RpInterpolatorGetEndMorphTarget( interp );

    /* instance positions and normals using morph targets where available */
    if ( startMT == endMT )
    {
        /* positions */
        if ( (0 != (geometry->lockedSinceLastInst & rpGEOMETRYLOCKVERTICES)) ||
             (FALSE == reinstance) )
        {
            const RwV3d *position;

            RwUInt8     *vertexData;

            RwUInt32    numVertices;


            position = &(geometry->morphTarget[startMT].verts[instanceData->minVertexIdx]);
            vertexData = baseVertexMem;
            numVertices = instanceData->numVertices;

            /* have we remapped vertex data to reduce vertex array usage? */
            if ( NULL == remapTable )
            {
                RWASSERT( FALSE == instanceData->remapVertexData );

                while ( numVertices-- )
                {
                    *(RwV3d *)(vertexData) = *position;

                    vertexData += instanceData->vertexStride;

                    position += 1;
                }
            }
            else
            {
                RwUInt32    idx;


                RWASSERT( FALSE != instanceData->remapVertexData );

                for ( idx = 0; idx < numVertices; idx += 1 )
                {
                    *( (RwV3d *)vertexData ) = position[ remapTable[idx] ];
                    vertexData += instanceData->vertexStride;
                }
            }
        }

        /* normals */
        if ( (0 != (geomFlags & rxGEOMETRY_NORMALS)) &&
             ((0 != (geometry->lockedSinceLastInst & rpGEOMETRYLOCKNORMALS)) ||
              (FALSE == reinstance)) )
        {
            const RwV3d *normal;

            RwUInt8     *vertexData;

            RwUInt32    numVertices;


            normal = &(geometry->morphTarget[startMT].normals[instanceData->minVertexIdx]);
            vertexData = baseVertexMem + sizeof(RwV3d);
            numVertices = instanceData->numVertices;

            /* have we remapped vertex data to reduce vertex array usage? */
            if ( NULL == remapTable )
            {
                RWASSERT( FALSE == instanceData->remapVertexData );

                while ( numVertices-- )
                {
                    *(RwV3d *)(vertexData) = *normal;

                    vertexData += instanceData->vertexStride;

                    normal += 1;
                }
            }
            else
            {
                RwUInt32    idx;


                RWASSERT( FALSE != instanceData->remapVertexData );

                for ( idx = 0; idx < numVertices; idx += 1 )
                {
                    *( (RwV3d *)vertexData ) = normal[ remapTable[idx] ];
                    vertexData += instanceData->vertexStride;
                }
            }
        }
    }
    else
    {
        RwReal      morphScale;

        const RwV3d *vertex1;

        const RwV3d *vertex2;

        RwUInt8     *vertexData;

        RwUInt32    numVertices;


        RWASSERT( NULL == remapTable );
        RWASSERT( FALSE == instanceData->remapVertexData );

        morphScale = interp->recipTime * interp->position;

        vertex1 = &(geometry->morphTarget[startMT].verts[instanceData->minVertexIdx]);
        RWASSERT( NULL != vertex1);

        vertex2 = &(geometry->morphTarget[endMT].verts[instanceData->minVertexIdx]);
        RWASSERT( NULL != vertex2 );

        vertexData = baseVertexMem;
        numVertices = instanceData->numVertices;

        if ( 0 != (geomFlags & rxGEOMETRY_NORMALS) )
        {
            const RwV3d *normal1;

            const RwV3d *normal2;


            normal1 = &(geometry->morphTarget[startMT].normals[instanceData->minVertexIdx]);
            RWASSERT( NULL != normal1);

            normal2 = &(geometry->morphTarget[endMT].normals[instanceData->minVertexIdx]);
            RWASSERT( NULL != normal2 );

            while ( numVertices-- )
            {
                RwV3d   *vertex;

                RwV3d   *normal;


                vertex = (RwV3d *)vertexData;

                /* positions */
                vertex->x = vertex1->x + (vertex2->x - vertex1->x) * morphScale;
                vertex->y = vertex1->y + (vertex2->y - vertex1->y) * morphScale;
                vertex->z = vertex1->z + (vertex2->z - vertex1->z) * morphScale;

                vertex1 += 1;
                vertex2 += 1;

                normal = vertex + 1;

                /* normals */
                normal->x = normal1->x + (normal2->x - normal1->x) * morphScale;
                normal->y = normal1->y + (normal2->y - normal1->y) * morphScale;
                normal->z = normal1->z + (normal2->z - normal1->z) * morphScale;

                normal1 += 1;
                normal2 += 1;

                vertexData += instanceData->vertexStride;
            };
        }
        else
        {
            while ( numVertices-- )
            {
                RwV3d   *vertex;


                vertex = (RwV3d *)vertexData;

                /* positions */
                vertex->x = vertex1->x + (vertex2->x - vertex1->x) * morphScale;
                vertex->y = vertex1->y + (vertex2->y - vertex1->y) * morphScale;
                vertex->z = vertex1->z + (vertex2->z - vertex1->z) * morphScale;

                vertex1 += 1;
                vertex2 += 1;

                vertexData += instanceData->vertexStride;
            };
        }
    }

    offset = sizeof(RwV3d);
    if ( 0 != (geomFlags & rxGEOMETRY_NORMALS) )
    {
        offset += sizeof(RwV3d);
    }

    /* instance prelight */
    if ( (0 != (geomFlags & rxGEOMETRY_PRELIT)) &&
         ((0 != (geometry->lockedSinceLastInst & rpGEOMETRYLOCKPRELIGHT)) ||
          (FALSE == reinstance)) )
    {
        const RwRGBA    *color;

        const RwRGBA    *materialColor;

        RwUInt8         *vertexData;

        RwUInt32        numVertices;

        RwUInt8         alpha;


        color = (const RwRGBA *)( (&(geometry->preLitLum[instanceData->minVertexIdx])) );
        RWASSERT( NULL != color );

        materialColor = RpMaterialGetColor( instanceData->material );
        RWASSERT( NULL != materialColor );

        vertexData = baseVertexMem + offset;
        numVertices = instanceData->numVertices;

        /* assume opaque */
        alpha = 0xFF;

        if ( (0 != (geomFlags & rxGEOMETRY_MODULATE)) &&
             (0xFFFFFFFF != *((const RwUInt32 *)materialColor)) )
        {
            /* have we remapped vertex data to reduce vertex array usage? */
            if ( NULL == remapTable )
            {
                RWASSERT( FALSE == instanceData->remapVertexData );

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
                RwUInt32    idx;


                RWASSERT( FALSE != instanceData->remapVertexData );

                for ( idx = 0; idx < numVertices; idx += 1 )
                {
                    const RwRGBA    remapColor = color[ remapTable[idx] ];


                    /* bits     info
                     * --------------
                     * 24-31    alpha
                     * 16-23    blue
                     * 8-15     green
                     * 0-7      red
                     */
                    *((RwUInt32 *)vertexData) =
                        ( ((remapColor.alpha * (materialColor->alpha + 1)) & 0xFF00) << 16) |
                        ( ((remapColor.blue  * (materialColor->blue  + 1)) & 0xFF00) << 8 ) |
                        ( ((remapColor.green * (materialColor->green + 1)) & 0xFF00)      ) |
                        ( ((remapColor.red   * (materialColor->red   + 1)) & 0xFF00) >> 8 );

                    alpha &= remapColor.alpha;

                    vertexData += instanceData->vertexStride;
                }
            }
        }
        else
        {
            if ( NULL == remapTable )
            {
                RWASSERT( FALSE == instanceData->remapVertexData );

                while ( numVertices-- )
                {
                    *((RwRGBA *)vertexData) = *color;

                    alpha &= color->alpha;

                    color += 1;

                    vertexData += instanceData->vertexStride;
                };
            }
            else
            {
                RwUInt32    idx;


                RWASSERT( FALSE != instanceData->remapVertexData );

                for ( idx = 0; idx < numVertices; idx += 1 )
                {
                    *((RwRGBA *)vertexData) = color[ remapTable[ idx] ];

                    alpha &= color->alpha;

                    vertexData += instanceData->vertexStride;
                }
            }
        }

        instanceData->vertexAlpha = (alpha != 0xFF);
    }
    else
    {
        instanceData->vertexAlpha = FALSE;
    }

    if ( 0 != (geomFlags & rxGEOMETRY_PRELIT) )
    {
        offset += sizeof(RwRGBA);
    }

    /* instance texture coordinates */
    if ( (0 != numTexCoords) &&
         ((0 != (geometry->lockedSinceLastInst & rpGEOMETRYLOCKTEXCOORDSALL)) ||
          (FALSE == reinstance)) )
    {
        RwInt32     idx;


        for ( idx = 0; idx < numTexCoords; idx += 1 )
        {
            if ( (0 != (geometry->lockedSinceLastInst & (rpGEOMETRYLOCKTEXCOORDS1 << idx))) ||
                 (FALSE == reinstance) )
            {
                const RwTexCoords   *texCoord;

                RwUInt8             *vertexData;

                RwUInt32            numVertices;


                texCoord = (const RwTexCoords *)( &(geometry->texCoords[idx][instanceData->minVertexIdx]) );

                vertexData = baseVertexMem + offset;
                numVertices = instanceData->numVertices;

                /* have we remapped vertex data to reduce vertex array usage? */
                if ( NULL == remapTable )
                {
                    RWASSERT( FALSE == instanceData->remapVertexData );

                    while ( numVertices-- )
                    {
                        *((RwTexCoords *)vertexData) = *texCoord;

                        texCoord += 1;

                        vertexData += instanceData->vertexStride;
                    }
                }
                else
                {
                    RwUInt32    idx;


                    RWASSERT( FALSE != instanceData->remapVertexData );

                    for ( idx = 0; idx < numVertices; idx += 1 )
                    {
                        *((RwTexCoords *)vertexData) = texCoord[ remapTable[idx] ];

                        vertexData += instanceData->vertexStride;
                    }
                }
            }

            offset += sizeof(RwTexCoords);
        }
    }

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rxOpenGLDefaultAllInOneAtomicLightingCB
 *
 *  Purpose : Default OpenGL Atomic Lighting Callback.
 *
 *  On entry: object - Void pointer to the object.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
void
_rxOpenGLDefaultAllInOneAtomicLightingCB( void *object )
{
    RpAtomic            *atomic;

    const RpGeometry    *geometry;

    RpGeometryFlag      geomFlags;


    RWFUNCTION( RWSTRING( "_rxOpenGLDefaultAllInOneAtomicLightingCB" ) );

    RWASSERT( NULL != object );

    atomic = (RpAtomic *)object;

    geometry = (const RpGeometry *)RpAtomicGetGeometry(atomic);
    RWASSERT( NULL != geometry );

    geomFlags = RpGeometryGetFlags(geometry);

    if ( (0 != (geomFlags & rxGEOMETRY_LIGHT)) &&
         (NULL != RWSRCGLOBAL(curWorld)) )
    {
        RwBool      lightingEnabled;

        RwLLLink    *curSector;

        RwLLLink    *endSector;


        /* assume lighting is disabled until we have enumerated
         * all possible light sources */
        lightingEnabled = FALSE;

        /* global lights (ambient & directional) */
        lightingEnabled |= _rwOpenGLLightsGlobalLightsEnable(rpLIGHTLIGHTATOMICS,TRUE);

        /* local lights (point, spot & soft spot) */

        /* increase the marker */
        RWSRCGLOBAL(lightFrame) += 1;

        /* for all sectors that this atomic lies in, apply all applicable lights */
        curSector = rwLinkListGetFirstLLLink( &(atomic->llWorldSectorsInAtomic) );
        endSector = rwLinkListGetTerminator( &(atomic->llWorldSectorsInAtomic) );
        while ( curSector != endSector )
        {
            RpTie       *tie;

            RwLLLink    *curLight;

            RwLLLink    *endLight;


            tie = rwLLLinkGetData( curSector, RpTie, lWorldSectorInAtomic );

            /* lights in the sector */
            curLight = rwLinkListGetFirstLLLink( &tie->worldSector->lightsInWorldSector );
            endLight = rwLinkListGetTerminator( &tie->worldSector->lightsInWorldSector );

            while ( curLight != endLight )
            {
                RpLightTie  *lightTie;


                lightTie = rwLLLinkGetData( curLight, RpLightTie, lightInWorldSector );

                /* lightTie may actually be a dummyTie from an enclosing ForAll */

                /* check to see if the light has already been applied and is set to
                 * light atomics */
                if ( (NULL != lightTie->light) &&
                     (lightTie->light->lightFrame != RWSRCGLOBAL(lightFrame)) &&
                     (0 != rwObjectTestFlags( lightTie->light, rpLIGHTLIGHTATOMICS )) )
                {
                    RwFrame         *lightFrame;

                    RwMatrix        *lightLTM;

                    const RwV3d     *lightPos;

                    const RwSphere  *atomicWorldBSphere;

                    RwV3d           lightToAtomicVector;

                    RwReal          lightToAtomicDistSq;

                    RwReal          atomicLightRadius;


                    /* don't light this atomic with the same light again! */
                    lightTie->light->lightFrame = RWSRCGLOBAL(lightFrame);

                    /* does the light intersect the atomic's bounding sphere in world space? */
                    lightFrame = (RwFrame *)RpLightGetFrame( lightTie->light );
                    RWASSERT( NULL != lightFrame );

                    lightLTM = (RwMatrix *)RwFrameGetLTM( lightFrame );
                    RWASSERT( NULL != lightLTM );

                    lightPos = &(lightLTM->pos);

                    atomicWorldBSphere = (const RwSphere *)RpAtomicGetWorldBoundingSphere( atomic );
                    RWASSERT( NULL != atomicWorldBSphere );

                    RwV3dSub( &lightToAtomicVector, &(atomicWorldBSphere->center), lightPos );
                    lightToAtomicDistSq = RwV3dDotProduct( &lightToAtomicVector, &lightToAtomicVector );

                    atomicLightRadius = atomicWorldBSphere->radius + RpLightGetRadius(lightTie->light);

                    /* only add the light if it is close enough to the atomic to affect it */
                    if ( lightToAtomicDistSq < (atomicLightRadius * atomicLightRadius) )
                    {
                        lightingEnabled |= _rwOpenGLLightsLocalLightEnable( lightTie->light );
                    }
                }

                /* next light */
                curLight = rwLLLinkGetNext( curLight );
            }

            /* next sector */
            curSector = rwLLLinkGetNext( curSector );
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
 *  Function: _rxOpenGLDefaultAtomicAllInOneNode
 *
 *  Purpose : OpenGL Atomic All-In-One node body.
 *
 *  On entry: self - Pointer to RxPipelineNode.
 *
 *            params - Pointer to constant RxPipelineNodeParam.
 *
 *  On exit : RwBool, TRUE if successful, FALSE if not.
 * --------------------------------------------------------------------- */
RwBool
_rxOpenGLDefaultAtomicAllInOneNode( RxPipelineNode * self,
                                    const RxPipelineNodeParam *params )
{
    RpAtomic                            *atomic;

    RpGeometry                          *geometry;

    RpMeshHeader                        *meshHeader;

    RwInt32                             numMorphTargets;

    RwResEntry                          *resEntry;

    _rxOpenGLAllInOnePrivateData          *privateData;

    RwFrame                             *atomicFrame;

    RwMatrix                            *atomicLTM;

    RwBool                              atomicLTMIsIdentity;


    RWFUNCTION( RWSTRING( "_rxOpenGLDefaultAtomicAllInOneNode" ) );

    RWASSERT( NULL != self );
    RWASSERT( NULL != params );

    atomic = (RpAtomic *)RxPipelineNodeParamGetData( params );
    RWASSERT( NULL != atomic );

    geometry = RpAtomicGetGeometry( atomic );
    RWASSERT( NULL != geometry );

    /* early out if there are no vertices */
    if ( geometry->numVertices <= 0 )
    {
        RWRETURN( TRUE );
    }

    meshHeader = geometry->mesh;

    /* early out if there are no meshes */
    if ( meshHeader->numMeshes <= 0 )
    {
        RWRETURN( TRUE );
    }

    numMorphTargets = RpGeometryGetNumMorphTargets(geometry);
    if ( 1 == numMorphTargets )
    {
        resEntry = geometry->repEntry;
    }
    else
    {
        resEntry = atomic->repEntry;
    }

    /* if the meshes have changed, we should re-instance */
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
        if ( NULL != privateData->reinstanceCB )
        {
            /* call the reinstance callback */
            if ( FALSE == privateData->reinstanceCB( (void *)atomic,
                                                     resEntry,
                                                     meshHeader,
                                                     privateData->instanceDLandVA,
                                                     privateData->instanceCB ) )
            {
                RwResourcesFreeResEntry( resEntry );
                RWRETURN( FALSE );
            }
        }

        /* we have a resource entry, so use it */
        RwResourcesUseResEntry( resEntry );
    }
    else
    {
        void        *owner;

        RwResEntry  **resEntryPtr;


        if ( 1 == numMorphTargets )
        {
            owner = (void *)geometry;
            resEntryPtr = (RwResEntry **)&(geometry->repEntry);
        }
        else
        {
            owner = (void *)atomic;
            resEntryPtr = (RwResEntry **)&(atomic->repEntry);
        }

        resEntry = _rxOpenGLInstance( (void *)atomic,
                                      owner,
                                      resEntryPtr,
                                      meshHeader,
                                      privateData->instanceDLandVA,
                                      privateData->instanceCB );
        if ( NULL == resEntry )
        {
            RWRETURN( FALSE );
        }

        /* the geometry is up to date */
        geometry->lockedSinceLastInst = 0;
    }

    /* set up lights */
    if ( NULL != privateData->lightingCB )
    {
        privateData->lightingCB( (void *)atomic );
    }

    /* model view transformation */
    atomicFrame = RpAtomicGetFrame( atomic );
    RWASSERT( NULL != atomicFrame );

    atomicLTM = RwFrameGetLTM( atomicFrame );
    RWASSERT( NULL != atomicLTM );

    atomicLTMIsIdentity = rwMatrixTestFlags(atomicLTM, rwMATRIXINTERNALIDENTITY);
    if ( FALSE == atomicLTMIsIdentity )
    {
#if defined(RWDEBUG)
        GLint   curOpenGLMatrixMode;


        glGetIntegerv( GL_MATRIX_MODE, &curOpenGLMatrixMode );
        RWASSERT( GL_MODELVIEW == curOpenGLMatrixMode );
#endif /* defined(RWDEBUG) */

        glPushMatrix();
        _rwOpenGLApplyRwMatrix( atomicLTM );

        /* do we need to normalize the normals */
        if ( (FALSE != RwOpenGLIsEnabled( rwGL_LIGHTING )) &&
             (FALSE == rwMatrixTestFlags(atomicLTM, rwMATRIXTYPENORMAL)) )
        {
            RwReal          length;

            const RwReal    minLimit = 0.9f;

            const RwReal    maxLimit = 1.1f;


            length = RwV3dDotProduct( &(atomicLTM->right), &(atomicLTM->right) );

            if ( (FLOATASINT(length) > FLOATASINT(maxLimit)) ||
                 (FLOATASINT(length) < FLOATASINT(minLimit)) )
            {
                RwOpenGLEnable( rwGL_NORMALIZE );
            }
            else
            {
                length = RwV3dDotProduct( &(atomicLTM->up), &(atomicLTM->up) );

                if ( (FLOATASINT(length) > FLOATASINT(maxLimit)) ||
                     (FLOATASINT(length) < FLOATASINT(minLimit)) )
                {
                    RwOpenGLEnable( rwGL_NORMALIZE );
                }
                else
                {
                    length = RwV3dDotProduct( &(atomicLTM->at), &(atomicLTM->at) );

                    if ( (FLOATASINT(length) > FLOATASINT(maxLimit)) ||
                         (FLOATASINT(length) < FLOATASINT(minLimit)) )
                    {
                        RwOpenGLEnable( rwGL_NORMALIZE );
                    }
                    else
                    {
                        RwOpenGLDisable( rwGL_NORMALIZE );
                    }
                }
            }
        }
        else
        {
            RwOpenGLDisable( rwGL_NORMALIZE );
        }
    }
    else
    {
        RwOpenGLDisable( rwGL_NORMALIZE );
    }

    /* render */
    if ( NULL != privateData->renderCB )
    {
        RwUInt32    geomFlags;


        geomFlags = RpGeometryGetFlags(geometry);
        privateData->renderCB( resEntry, (void *)atomic, rpATOMIC, geomFlags );
    }

    /* restore the model view matrix */
    if ( FALSE == atomicLTMIsIdentity )
    {
        glPopMatrix();
    }

    RwOpenGLDisable( rwGL_LIGHTING );

#if defined(RWMETRICS)
    RWSRCGLOBAL(metrics)->numVertices  += RpGeometryGetNumVertices(geometry);
    RWSRCGLOBAL(metrics)->numTriangles += RpGeometryGetNumTriangles(geometry);
#endif /* defined(RWMETRICS) */

    RWRETURN( TRUE );
}


/* =====================================================================
 *  Static function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: openglAtomicAllInOnePipelineInit
 *
 *  Purpose : Initialize the atomic all-in-one pipeline node.
 *
 *  On entry: node - Pointer to RxPipelineNode.
 *
 *  On exit : RwBool, TRUE if successfully initialized the pipeline node
 *            or FALSE if failed.
 * --------------------------------------------------------------------- */
static RwBool
openglAtomicAllInOnePipelineInit( RxPipelineNode *node )
{
    _rxOpenGLAllInOnePrivateData    *privateData;


    RWFUNCTION( RWSTRING( "openglAtomicAllInOnePipelineInit" ) );

    RWASSERT( NULL != node );

    /* common all-in-one pipeline initialization */
    if ( FALSE == _rxOpenGLAllInOnePipelineInit(node) )
    {
        RWRETURN( FALSE );
    }

    privateData = (_rxOpenGLAllInOnePrivateData *)(node->privateData);
    RWASSERT( NULL != privateData );

    /* set the default callbacks for the atomic pipeline */
    privateData->instanceDLandVA = FALSE;   /* don't instance both display lists and
                                               vertex buffers by default */
    privateData->instanceCB      = openglDefaultAtomicInstanceCB;
    privateData->reinstanceCB    = openglDefaultAtomicReinstanceCB;
    privateData->lightingCB      = _rxOpenGLDefaultAllInOneAtomicLightingCB;
    privateData->renderCB        = _rxOpenGLDefaultAllInOneRenderCB;

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: openglDefaultAtomicInstanceCB
 *
 *  Purpose : Default OpenGL Atomic Instance Callback.
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
openglDefaultAtomicInstanceCB( void *object,
                               RxOpenGLMeshInstanceData *instanceData,
                               const RwBool instanceDLandVA,
                               const RwBool reinstance )
{
    RpAtomic            *atomic;

    RpGeometry          *geometry;

    RwInt32             numTexCoords;

    RpGeometryFlag      geomFlags;

    RwInt32             numMorphTargets;

    RwBool              tryDisplayList;

    RxVertexIndex       *remapTable;

    RwInt32             startMT;

    RwInt32             endMT;

    RwReal              morphScale;


    RWFUNCTION( RWSTRING( "openglDefaultAtomicInstanceCB" ) );

    RWASSERT( NULL != object );
    RWASSERT( NULL != instanceData );

    /* get the atomic */
    atomic = (RpAtomic *)object;

    /* get the atomic's geometry */
    geometry = RpAtomicGetGeometry(atomic);
    RWASSERT( NULL != geometry );

    /* get the number of texture coordinates in this geometry */
    numTexCoords = RpGeometryGetNumTexCoordSets(geometry );

    /* geometry's flags */
    geomFlags = RpGeometryGetFlags(geometry);

    /* get the vertex descriptor */
    instanceData->vertexDesc = (RwUInt32)geomFlags;

    /* get the number of morph targets */
    numMorphTargets = RpGeometryGetNumMorphTargets(geometry);

    /* display lists are good for non-dynamic geometry */
    if ( 1 == numMorphTargets )
    {
        tryDisplayList = TRUE;

        /* when NVIDIA VAR or ATI VAO is available, it's always better to
         * use vertex arrays */
        if ( (_rwOpenGLVertexHeapAvailable()) ||
             (NULL != _rwOpenGLExt.NewObjectBufferATI) )
        {
            tryDisplayList = FALSE;
        }
    }
    else
    {
        tryDisplayList = FALSE;
    }

    remapTable = (RxVertexIndex *)NULL;
    if ( 1 != numMorphTargets )
    {
        const RpInterpolator    *interp;


        interp = (const RpInterpolator *)RpAtomicGetInterpolator( atomic );
        RWASSERT( NULL != interp );

        startMT = RpInterpolatorGetStartMorphTarget(interp);
        endMT = RpInterpolatorGetEndMorphTarget(interp);

        if ( (startMT >= numMorphTargets) ||
             (endMT >= numMorphTargets) )
        {
            /* clamp */
            startMT = endMT = 0;
        }

        morphScale = interp->recipTime * interp->position;
    }
    else
    {
        startMT = endMT = 0;
        morphScale = 0.0f;

        if ( (FALSE == reinstance) &&
             (FALSE == tryDisplayList) &&
             (instanceData->numIndices <= instanceData->numVertices) )
        {
            RxVertexIndex   *usedVertexIndices;

            RwUInt32        maxVertex;

            RwUInt32        idx;


            usedVertexIndices = (RxVertexIndex *)RwMalloc( instanceData->numVertices * sizeof(RxVertexIndex),
                                                           rwMEMHINTDUR_FUNCTION | rwID_WORLDPIPEMODULE );
            RWASSERT( NULL != usedVertexIndices );
            if ( NULL == usedVertexIndices )
            {
                RWERROR( (E_RW_NOMEM, (instanceData->numVertices * sizeof(RxVertexIndex))) );
                RWRETURN( FALSE );
            }

            maxVertex = 0;

            /* clear index data */
            memset( usedVertexIndices, 0xFF, instanceData->numVertices * sizeof(RxVertexIndex) );

            /* record a list of all vertex indices uses (and unrepeated) */
            for ( idx = 0; idx < instanceData->numIndices; idx += 1 )
            {
                const RxVertexIndex curIndex = instanceData->indexData[idx];


                if ( usedVertexIndices[curIndex] > maxVertex )
                {
                    usedVertexIndices[curIndex] = (RxVertexIndex)maxVertex;

                    maxVertex += 1;
                }
            }

            /* is it worth remapping? */
            if ( maxVertex < instanceData->numVertices )
            {
                remapTable = (RxVertexIndex *)RwMalloc( instanceData->numVertices * sizeof(RxVertexIndex),
                                                        rwMEMHINTDUR_FUNCTION | rwID_WORLDPIPEMODULE );
                RWASSERT( NULL != remapTable );
                if ( NULL == remapTable )
                {
                    RWERROR( (E_RW_NOMEM, instanceData->numVertices * sizeof(RxVertexIndex)) );
                    RWRETURN( FALSE );
                }

                RWMESSAGE( (RWSTRING( "Remapping from %d to %d vertices." ),
                            instanceData->numVertices, maxVertex) );

                instanceData->numVertices = maxVertex;

                instanceData->remapVertexData = TRUE;

                /* clear index data */
                memset( remapTable, 0xFF, instanceData->numVertices * sizeof(RxVertexIndex) );

                for ( idx = 0; idx < instanceData->numIndices; idx += 1 )
                {
                    const RxVertexIndex index = instanceData->indexData[idx];


                    instanceData->indexData[idx] = usedVertexIndices[index];
                    remapTable[ instanceData->indexData[idx] ] = index;
                }
            }

            RwFree( usedVertexIndices );
        }
    }

    /* if anything has changed, we have to instance our geometry */
    if ( (0 != (geometry->lockedSinceLastInst & rpGEOMETRYLOCKALL)) ||
         (FALSE == reinstance) )
    {
        /* if a display list is possible, try instancing to that */
        if ( FALSE != tryDisplayList )
        {
            /* there's no morph targets, so we're going to use a display list */
            if ( FALSE == reinstance )
            {
                RWASSERT( GL_FALSE == glIsList( instanceData->displayList ) );

                /* generate a display list */
                instanceData->displayList = glGenLists( 1 );
                RWASSERT( 0 != instanceData->displayList );
            }

            /* if we have to reinstance a display list, we have to instance EVERYTHING
             * so don't bother to check the geometry lock settings
             * this isn't very efficient so we don't want to be doing this often
             */
            _rxOpenGLInstanceDisplayList( instanceData,
                                          geomFlags,
                                          rpATOMIC,
                                          numTexCoords,
                                          (RwTexCoords **)((0 == numTexCoords) ? NULL : geometry->texCoords),
                                          ((0 == (geomFlags & rxGEOMETRY_PRELIT)) ? (RwRGBA *)NULL : geometry->preLitLum),
                                          ((0 == (geomFlags & rxGEOMETRY_NORMALS)) ? (void *)NULL : (void *)(geometry->morphTarget[0].normals)),
                                          geometry->morphTarget[0].verts );
        }

        /* if a display list wasn't possible, or the display list instancing failed
         * e.g. multiple texture coordinates are required but multitexturing isn't available,
         * or if we want both display lists and vertex arrays instanced for special effects,
         * instance into vertex arrays
         */
        if ( (FALSE == tryDisplayList) ||
             (FALSE != instanceDLandVA ) ||
             (GL_FALSE == glIsList( instanceData->displayList )) )
        {
            RwUInt8 *baseVertexMem;


            baseVertexMem = (RwUInt8 *)NULL;

            if ( FALSE == reinstance )
            {
                /* position */
                instanceData->vertexStride = sizeof(RwV3d);

                /* normals */
                if ( 0 != (geomFlags & rxGEOMETRY_NORMALS) )
                {
                    instanceData->vertexStride += sizeof(RwV3d);
                }

                /* prelight */
                if ( 0 != (geomFlags & rxGEOMETRY_PRELIT) )
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

                    if ( NULL != instanceData->vidMemVertexData )
                    {
                        _rwOpenGLVertexHeapDynamicDiscard( instanceData->vidMemVertexData );
                    }
                    /* generate a fence on this block */
                    instanceData->vidMemVertexData = (void *)_rwOpenGLVertexHeapDynamicMalloc( instanceData->vertexDataSize, TRUE );
                    RWASSERT( NULL != instanceData->vidMemVertexData );
                    if ( NULL == instanceData->vidMemVertexData )
                    {
                        RWERROR( (E_RW_NOMEM, instanceData->vertexDataSize) );
                        RWRETURN( FALSE );
                    }

                    instanceData->vidMemDataIsStatic = FALSE;
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
            }
            else
            {
                baseVertexMem = instanceData->vidMemVertexData;
                if ( NULL == baseVertexMem )
                {
                    baseVertexMem = instanceData->sysMemVertexData;
                }
                else
                {
                    if ( _rwOpenGLVertexHeapAvailable() )
                    {
                        /* if we're using nVIDIAs vertex array range extension
                         * then ensure that the previous render has finished */
                        _rwOpenGLVertexHeapFinishNVFence( baseVertexMem );
                    }
                }
            }
            RWASSERT( NULL != baseVertexMem );

            _rxOpenGLAllInOneAtomicInstanceVertexArray( instanceData,
                                                        atomic,
                                                        geometry,
                                                        geomFlags,
                                                        numTexCoords,
                                                        reinstance,
                                                        remapTable,
                                                        baseVertexMem );

            /* ATI Vertex Array Object */
            if ( NULL != _rwOpenGLExt.NewObjectBufferATI )
            {
                if ( FALSE == reinstance )
                {
                    RwUInt8     *offset;

                    RwInt32     idx;


                    instanceData->vaoName = _rwOpenGLExt.NewObjectBufferATI(
                                        instanceData->vertexDataSize,
                                        (const void *)instanceData->sysMemVertexData,
                                        GL_DYNAMIC_ATI );
                    RWASSERT( GL_FALSE !=
                        _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName ) );

                    offset = 0;

                    instanceData->position = offset;
                    offset += sizeof(RwV3d);

                    /* normals */
                    if ( 0 != (geomFlags & rxGEOMETRY_NORMALS) )
                    {
                        instanceData->normal = offset;
                        offset += sizeof(RwV3d);
                    }

                    /* prelight */
                    if ( 0 != (geomFlags & rxGEOMETRY_PRELIT) )
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
                    RWASSERT( GL_FALSE !=
                        _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName ) );
                    _rwOpenGLExt.UpdateObjectBufferATI( instanceData->vaoName,
                                                        0,
                                                        instanceData->vertexDataSize,
                                                        instanceData->sysMemVertexData,
                                                        GL_DISCARD_ATI );
                }
            }
            else
            {
                if ( FALSE == reinstance )
                {
                    RwUInt8 *offset;

                    RwInt32 idx;


                    offset = (RwUInt8 *)baseVertexMem;

                    instanceData->position = offset;
                    offset += sizeof(RwV3d);

                    /* normals */
                    if ( 0 != (geomFlags & rxGEOMETRY_NORMALS) )
                    {
                        instanceData->normal = offset;
                        offset += sizeof(RwV3d);
                    }

                    /* prelight */
                    if ( 0 != (geomFlags & rxGEOMETRY_PRELIT) )
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
        }
    }

    if ( NULL != remapTable )
    {
        RwFree( remapTable );
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: openglDefaultAtomicReinstanceCB
 *
 *  Purpose : Default OpenGL Atomic Reinstance Callback.
 *
 *  On entry: object - Void pointer to the object.
 *
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
static RwBool
openglDefaultAtomicReinstanceCB( void *object,
                                 RwResEntry *resEntry,
                                 const RpMeshHeader *meshHeader,
                                 const RwBool instanceDLandVA,
                                 RxOpenGLAllInOneInstanceCallBack instanceCB )
{
    RpAtomic                    *atomic;

    RpGeometry                  *geometry;

    RxOpenGLResEntryHeader      *resEntryHdr;

    RxOpenGLMeshInstanceData    *instanceData;


    RWFUNCTION( RWSTRING( "openglDefaultAtomicReinstanceCB" ) );

    RWASSERT( NULL != object );
    RWASSERT( NULL != resEntry );
    RWASSERT( NULL != meshHeader );

    atomic = (RpAtomic *)object;

    geometry = RpAtomicGetGeometry(atomic);
    RWASSERT( NULL != geometry );

    resEntryHdr = (RxOpenGLResEntryHeader *)(resEntry + 1);
    RWASSERT( NULL != resEntryHdr );

    instanceData = (RxOpenGLMeshInstanceData *)(resEntryHdr + 1);
    RWASSERT( NULL != instanceData );

    /* has anything changed in the last frame? */
    if ( 0 != geometry->lockedSinceLastInst )
    {
        if ( FALSE == _rxOpenGLReinstance( object,
                                           meshHeader,
                                           instanceData,
                                           instanceDLandVA,
                                           instanceCB ) )
        {
            RWRETURN( FALSE );
        }

        geometry->lockedSinceLastInst = 0;
    }

    /* if there are morph targets, morph if dirty */
    if ( 1 != RpGeometryGetNumMorphTargets(geometry) )
    {
        const RpInterpolator    *interp;


        interp = (const RpInterpolator *)&(atomic->interpolator);
        RWASSERT( NULL != interp );

        if ( 0 != (interp->flags & rpINTERPOLATORDIRTYINSTANCE) )
        {
            openglUpdateMorphInstance( resEntry, meshHeader, atomic, (RpGeometry *)geometry, interp, instanceData );

            /* interpolator is no longer dirty */
            atomic->interpolator.flags &= ~rpINTERPOLATORDIRTYINSTANCE;
        }
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: openglUpdateMorphInstance
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
static void
openglUpdateMorphInstance( RwResEntry *resEntry __RWUNUSEDRELEASE__,
                           const RpMeshHeader *meshHeader,
                           RpAtomic *atomic,
                           RpGeometry *geometry,
                           const RpInterpolator *interp,
                           RxOpenGLMeshInstanceData *instanceData )
{
    RwInt32         numMorphTargets;

    RwInt32         startMT;

    RwInt32         endMT;

    RwReal          morphScale;

    RpGeometryFlag  geomFlags;

    RwInt32         numTexCoords;

    RwUInt32        numMeshes;


    RWFUNCTION( RWSTRING( "openglUpdateMorphInstance" ) );

    RWASSERT( NULL != resEntry );
    RWASSERT( NULL != meshHeader );
    RWASSERT( NULL != geometry );
    RWASSERT( NULL != interp );
    RWASSERT( NULL != instanceData );

    numMorphTargets = RpGeometryGetNumMorphTargets(geometry);

    startMT = interp->startMorphTarget;
    endMT = interp->endMorphTarget;

    if ( (startMT >= numMorphTargets) ||
         (endMT >= numMorphTargets) )
    {
        /* clamp to range */
        startMT = endMT = 0;
    }

    morphScale = interp->recipTime * interp->position;

    geomFlags = RpGeometryGetFlags( geometry );

    numTexCoords = RpGeometryGetNumTexCoordSets( geometry );

    numMeshes = meshHeader->numMeshes;
    while ( numMeshes-- )
    {
        RwUInt8 *baseVertexMem;


        /* ensure we don't have a display list */
        RWASSERT( GL_FALSE == glIsList( instanceData->displayList ) );

        baseVertexMem = instanceData->vidMemVertexData;
        if ( NULL == baseVertexMem )
        {
            baseVertexMem = instanceData->sysMemVertexData;
        }
        else
        {
            if ( FALSE == instanceData->vidMemDataIsStatic )
            {
                if ( _rwOpenGLVertexHeapAvailable() )
                {
                    /* if we're using nVIDIAs vertex array range extension
                     * then ensure that the previous render has finished */
                    _rwOpenGLVertexHeapFinishNVFence( baseVertexMem );
                }
            }
        }
        RWASSERT( NULL != baseVertexMem );

        _rxOpenGLAllInOneAtomicInstanceVertexArray( instanceData,
                                                    atomic,
                                                    geometry,
                                                    geomFlags,
                                                    numTexCoords,
                                                    TRUE,
                                                    NULL,
                                                    baseVertexMem );

        /* ATI Vertex Array Object */
        if ( NULL != _rwOpenGLExt.UpdateObjectBufferATI )
        {
            RWASSERT( GL_FALSE !=
                _rwOpenGLExt.IsObjectBufferATI( instanceData->vaoName ) );
            _rwOpenGLExt.UpdateObjectBufferATI( instanceData->vaoName,
                                                0,
                                                instanceData->vertexDataSize,
                                                instanceData->sysMemVertexData,
                                                GL_DISCARD_ATI );
        }

        /* next RxOpenGLMeshInstanceData */
        instanceData += 1;
    }

    RWRETURNVOID();
}


