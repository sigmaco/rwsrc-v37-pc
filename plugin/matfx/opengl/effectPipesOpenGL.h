/***********************************************************************
 *
 * Module:  effectPipesOpenGL.h
 *
 * Purpose: 
 *
 ***********************************************************************/

#if !defined( EFFECTPIPEOPENGL_H )
#define EFFECTPIPEOPENGL_H

/* =====================================================================
 *  Includes
 * ===================================================================== */


/* =====================================================================
 *  Defines
 * ===================================================================== */
#define MATFXOPENGLENVMAPGETDATA(material, pass)    \
  &((*((rpMatFXMaterialData **)                     \
       (((RwUInt8 *)material) +                     \
        MatFXMaterialDataOffset)))->data[pass].data.envMap)

#define MATFXOPENGLBUMPMAPGETDATA(material) \
  &((*((rpMatFXMaterialData **)             \
       (((RwUInt8 *)material) +             \
        MatFXMaterialDataOffset)))->data[rpSECONDPASS].data.bumpMap)

#define MATFXOPENGLDUALGETDATA(material)    \
  &((*((rpMatFXMaterialData **)             \
       (((RwUInt8 *)material) +             \
        MatFXMaterialDataOffset)))->data[rpSECONDPASS].data.dual)

#define MATFXOPENGLUVANIMGETDATA(material)  \
  &((*((rpMatFXMaterialData **)             \
       (((RwUInt8 *)material) +             \
        MatFXMaterialDataOffset)))->data[rpSECONDPASS].data.uvAnim)

#define MATFXOPENGLDUALUVANIMGETDUALDATA(material)  \
  &((*((rpMatFXMaterialData **)                     \
       (((RwUInt8 *)material) +                     \
        MatFXMaterialDataOffset)))->data[rpTHIRDPASS].data.dual)


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */
struct MatFXBumpMapData
{
    RwFrame     *frame;

    RwTexture   *texture;

    RwTexture   *bumpTexture;

    RwReal      coef;

    RwReal      invBumpWidth;
};

struct MatFXDualData
{
    RwTexture       *texture;

    RwBlendFunction srcBlendMode;

    RwBlendFunction dstBlendMode;
};

typedef void (*_rpMatFXOpenGLEnvMapRenderCB)( RwMatrix *objectLTM,
                                              RxOpenGLMeshInstanceData *instanceData,
                                              RwTexture *baseTexture,
                                              const RwUInt32 flags,
                                              const void *envMapDataVoid );

typedef void (*_rpMatFXOpenGLBumpMapRenderCB)( RwMatrix *objectLTM,
                                               RxOpenGLMeshInstanceData *instanceData,
                                               RwTexture *baseTexture,
                                               const RwUInt32 flags,
                                               const void *bumpMapDataVoid,
                                               const void *envMapDataVoid );

typedef void (*_rpMatFXOpenGLDualRenderCB)( RxOpenGLMeshInstanceData *instanceData,
                                            RwTexture *baseTexture,
                                            const RwUInt32 flags,
                                            const void *dualDataVoid,
                                            const void *uvanimDataVoid );

typedef void (*_rpMatFXOpenGLUVAnimRenderCB)( RxOpenGLMeshInstanceData *instanceData,
                                              RwTexture *baseTexture,
                                              const RwUInt32 flags,
                                              const void *uvanimDataVoid );


#if defined( __cplusplus )
extern "C"
{
#endif /* defined( __cplusplus ) */

/* =====================================================================
 *  Extern variables
 * ===================================================================== */
#if !defined(TEAMOPENGL_H)
extern RwOpenGLExtensions               _rwOpenGLExt;
#endif /* !defined(TEAMOPENGL_H) */

extern _rpMatFXOpenGLEnvMapRenderCB     _rpMatFXOpenGLEnvMapRender;

extern _rpMatFXOpenGLBumpMapRenderCB    _rpMatFXOpenGLBumpMapRender;

extern _rpMatFXOpenGLDualRenderCB       _rpMatFXOpenGLDualRender;

extern _rpMatFXOpenGLUVAnimRenderCB     _rpMatFXOpenGLUVAnimRender;


/* =====================================================================
 *  Extern function prototypes
 * ===================================================================== */

#if !defined(TEAMOPENGL_H)
extern RwBool
_rwOpenGLSetRenderState( RwRenderState state, void *param );
#endif /* !defined(TEAMOPENGL_H) */

extern RwBool
_rwOpenGLGetRenderState( RwRenderState state, void *param );

extern void
_rxOpenGLDefaultRenderCallback( RwResEntry *repEntry,
                                void *object,
                                const RwUInt8 type,
                                const RwUInt32 flags );

extern void
_rpMatFXOpenGLAllInOneRenderCB( RwResEntry *repEntry,
                                void *object,
                                const RwUInt8 type,
                                const RwUInt32 flags );

extern RxPipeline *
_rpMatFXOpenGLAtomicPipelineCreate( void );

extern void
_rpMatFXOpenGLDefaultRender( RxOpenGLMeshInstanceData *instanceData,
                             RwTexture *baseTexture,
                             const RwUInt32 flags );


#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

#endif /* !defined( EFFECTPIPEOPENGL_H ) */
