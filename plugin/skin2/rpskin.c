/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   rpskin.c                                                   -*
 *-                                                                         -*
 *-  Purpose :   API skin setup.                                            -*
 *-                                                                         -*
 *===========================================================================*/

/**
 * \ingroup rpskin
 * \page rpskinoverview RpSkin Plugin Overview
 *
 * \par Requirements
 * There are two versions of the RpSkin libraries in the RenderWare
 * Graphics SDK. They are both fully featured versions on the RpSkin
 * plugin, and they contain identical APIs. However, because the
 * rendering pipelines are large, we have taken the step to compile
 * different versions of the plugin so that you can precisely select 
 * the pipelines you want to use.
 *
 * \note You need to have a matrix array setup before you can do any
 * skinning. This can be done by calling \ref RpHAnimHierarchyUpdateMatrices
 * on an animation that has interpolation keyframes setup. Or, you can
 * setup a matrix array yourself.
 *
 * \par Requirements for rpskin library
 * The rpskin.lib library only contains the \ref rpSKINTYPEGENERIC pipeline.
 * \li \b Headers: rwcore.h, rpworld.h, rphanim.h, rtanim.h, rpskin.h
 * \li \b Libraries: rwcore, rpworld, rtquat, rphanim, rtanim, rpskin
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach,
 *                               \ref RpHAnimPluginAttach,
 *                               \ref RtAnimInitialize,
 *                               \ref RpSkinPluginAttach
 *
 * \par Requirements for rpskinmatfx library
 * The rpskinmatfx.lib library contains both the \ref rpSKINTYPEGENERIC and
 * \ref rpSKINTYPEMATFX pipelines.
 * \li \b Headers: rwcore.h, rpworld.h, rphanim.h, rtanim.h, rpmatfx.h, rpskin.h
 * \li \b Libraries: rwcore, rpworld, rtquat, rphanim, rpmatfx, rpskinmatfx
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach,
 *                               \ref RpHAnimPluginAttach,
 *                               \ref RtAnimInitialize,
 *                               \ref RpMatFXPluginAttach,
 *                               \ref RpSkinPluginAttach
 *
 * \par Restrictions
 * Each platform has a restriction on the number of bones that can be used with
 * a skin. Please refer to the platform specific section of the skin
 * documentation for information on this subject, \ref rpskinbonelimit.
 *
 * If a model requires more bones than this limit, it needs to be
 * 'split' into smaller groups such that each group requires less bones
 * than this limit. See the toolkit, \ref rtskinsplit, for more detail
 * on splitting a model to fit onto a target platform.
 *
 *
 * \note Only one of the skinning libraries
 *       should be used in an application at once.
 *
 * \subsection skinoverview Overview
 * The skin plugin provides a means to animate a model while
 * reducing the 'folding' and 'creasing' of polygons as the model
 * animates. A bone hierarchy is defined in a model and it is linked
 * to the model's mesh, so that animating the bone hierarchy
 * animates the mesh's vertices. The custom skinning rendering pipes
 * allow up to four bones to affect each vertex in the mesh.
 *
 * The skinning definition and rendering is defined in this plugin,
 * whereas the animation and hierarchy is defined in the
 * \ref rphanim plugin.
 *
 * Before any of the skinning functions can be used the plugin must
 * be registered with RenderWare with:
 *   \li \ref RpSkinPluginAttach
 *
 * The \ref RpSkin defines the relationship between the vertices
 * and bones. The following functions create and destroy a skin:
 *   \li \ref RpSkinCreate  Creates and initialized the skin for
 *                         a \ref RpGeometry or a \ref RpPatchMesh.
 *   \li \ref RpSkinDestroy Destroys the skin.
 *
 * Information about a skin can be queried with the following
 * functions:
 *   \li \ref RpSkinGetNumBones           Returns the number of
 *                                        bones in the skin.
 *   \li \ref RpSkinGetVertexBoneWeights  Returns the vertex bone
 *                                        weights.
 *   \li \ref RpSkinGetVertexBoneIndices  Returns the vertex bone
 *                                        indices.
 *   \li \ref RpSkinGetSkinToBoneMatrices Returns the skin-space to
 *                                        bone-space transformation
 *                                        matrices.
 *   \li \ref RpSkinIsSplit               Returns whether the skin has
 *                                        been split.
 *
 * Once the skin has been created and setup, it can be attached
 * to a \ref RpGeometry or \ref RpPatchMesh with:
 *   \li \ref RpSkinGeometrySetSkin Attaches a skin on to a geometry.
 *   \li \ref RpSkinGeometryGetSkin Queries a skin attached on a geometry.
 *   \li \ref RpPatchMeshSetSkin Attaches a skin on to a patch mesh.
 *   \li \ref RpPatchMeshGetSkin Queries a skin attached to a patch mesh.
 *
 * The \ref RpHAnimHierarchy used to animate the skin needs to be
 * attached to the \ref RpAtomic. This allows a number of atomics
 * to reference the same geometry, hence the same skin, and
 * animate it in different ways. The following functions control
 * the setting of the hierarchy:
 *   \li \ref RpSkinAtomicSetHAnimHierarchy  Attach a hierarchy to
 *                                           the atomic to use
 *                                           positioning the bones.
 *   \li \ref RpSkinAtomicGetHAnimHierarchy  Query the hierarchy
 *                                           attached to an atomic.
 *
 * The atomic still needs it's rendering pipeline overloaded with
 * a custom skinning rendering pipeline. These pipelines are
 * defined by \ref RpSkinType. The skinned atomic should have
 * their pipelines overloaded with:
 *   \li \ref RpSkinAtomicSetType Set the pipeline on to the atomic.
 *   \li \ref RpSkinAtomicGetType Query the type of the pipeline
 *                                attached to the atomic.
 *
 * If the atomic has a patch mesh attached instead of a geometry.
 * Then a patch mesh skinning rendering pipeline should be attached
 * instead. These are defined by \ref RpPatchType and the pipeline
 * is overloaded with:
 *   \li \ref RpPatchAtomicSetType Set the pipeline on to a patch
 *                                 atomic.
 *   \li \ref RpPatchAtomicGetType Query the type of a patch
 *                                 pipeline attached to the atomic.
 */

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include "rwcore.h"
#include "rpworld.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rphanim.h"
#include "rptoon.h"
#include "rpskin.h"

#include "skin.h"
#include "bsplit.h"

/*=========================================================================*
 *--- Private Types -------------------------------------------------------*
 *=========================================================================*/

/*=========================================================================*
 *--- Private Global Variables --------------------------------------------*
 *=========================================================================*/

/*=========================================================================*
 *--- Private Defines -----------------------------------------------------*
 *=========================================================================*/

/*=========================================================================*
 *--- Local Types ---------------------------------------------------------*
 *=========================================================================*/

/*=========================================================================*
 *--- Local Global Variables ----------------------------------------------*
 *=========================================================================*/
#if (defined(RWDEBUG))
long _rpSkinStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

/*=========================================================================*
 *--- Local Defines -------------------------------------------------------*
 *=========================================================================*/
#define rpSKINENTRIESPERBLOCK 20
#define rpSKINALIGNMENT sizeof(RwUInt32)

#define ROUNDUP16(x) (((RwUInt32)(x) + 16 - 1) & ~(16 - 1))

#define CHECKSTREAMANDRETURN(success) \
MACRO_START                           \
{                                     \
    if(NULL == (success))             \
    {                                 \
        RWRETURN((RwStream *)NULL);   \
    }                                 \
}                                     \
MACRO_STOP

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/
static RwBool
MatfxPluginIsAttached(void)
{
    RWFUNCTION(RWSTRING("MatfxPluginIsAttached"));

    RWRETURN(RwEngineGetPluginOffset(rwID_MATERIALEFFECTSPLUGIN) != -1);
}

static RwBool
ToonPluginIsAttached(void)
{
    RWFUNCTION(RWSTRING("ToonPluginIsAttached"));


    RWRETURN(RwEngineGetPluginOffset(rwID_TOONPLUGIN) != -1);
}

static RpAtomic *
SkinAtomicAttachBestPipeForAttachedPlugins(RpAtomic *atomic,
                                           RpSkinType type)
{
    RWFUNCTION(RWSTRING("SkinAtomicAttachBestPipeForAttachedPlugins"));

    /* Demote the atomic type to generic if we haven't got these plugins. */
    if (!MatfxPluginIsAttached() && (type == rpSKINTYPEMATFX))
    {
        RwDebugSendMessage(rwDEBUGMESSAGE, "SkinAtomicAttachBestPipeForAttachedPlugins",
            "Attempting to load a SKINMATFX object without the RpMatFX plugin attached. Object demoted to SKINGENERIC.");
        type = rpSKINTYPEGENERIC;
    }
    else if (!ToonPluginIsAttached() && (type == rpSKINTYPETOON))
    {
        RwDebugSendMessage(rwDEBUGMESSAGE, "SkinAtomicAttachBestPipeForAttachedPlugins",
            "Attempting to load a SKINTOON object without the RpToon plugin attached. Object demoted to SKINGENERIC.");
        type = rpSKINTYPEGENERIC;
    }

    /* _rpSkinPipelinesAttach may still have to demote further, depending what
     * combination library was linked. E.g. just because you attach RpMatFX doesn't
     * mean you've linked rpskinmatfx.lib. */
    RWRETURN(_rpSkinPipelinesAttach(atomic, type));
}

static RpAtomic *
SkinAtomicSetup( RpAtomic *atomic,
                 RpSkinType type )
{
    RpGeometry *geometry;

    RWFUNCTION(RWSTRING("SkinAtomicSetup"));
    RWASSERT(NULL != atomic);

    geometry = RpAtomicGetGeometry(atomic);

    if(NULL != geometry)
    {
        RpSkin *skin;

        skin = RpSkinGeometryGetSkin(geometry);

        if(NULL != skin)
        {
            RpAtomic *success;

            success = SkinAtomicAttachBestPipeForAttachedPlugins(atomic, type);
            RWASSERT(NULL != success);
        }
    }

    RWRETURN(atomic);
}

/*****************************************************************************
 SkinFindMaxWeights
 */
static void
SkinFindMaxWeights(RpSkin *skin,
                   RwMatrixWeights *matrixWeights,
                   RwUInt32 numVertices)
{
    RwUInt32    i;

    RWFUNCTION(RWSTRING("SkinFindMaxWeights"));
    RWASSERT(NULL != skin);

    /* Find the maximum number of weights to be used */
    skin->vertexMaps.maxWeights = 1;

    for (i = 0; i < numVertices; i++)
    {
        RwUInt32    j;

        RWASSERT(0 != *((RwUInt32 *)(&matrixWeights[i].w0)));

        for (j = skin->vertexMaps.maxWeights; j < 4; j++)
        {
            if (0 != *((RwUInt32 *)((&matrixWeights[i].w0) + j)))
            {
                skin->vertexMaps.maxWeights++;

                if (4 == skin->vertexMaps.maxWeights)
                {
                    RWRETURNVOID();
                }
            }
            else
            {
                break;
            }
        }
    }

    RWRETURNVOID();
}

/*****************************************************************************
 SkinFindNumUsedBones
 */
static void
SkinFindNumUsedBones(RpSkin *skin,
                     RwUInt32 *indices,
                     RwMatrixWeights *weights,
                     RwUInt8 *usedBoneList,
                     RwUInt32 *numUsedBones,
                     RwUInt32 numVertices)
{
    RwUInt32    i;

    RWFUNCTION(RWSTRING("SkinFindNumUsedBones"));
    RWASSERT(NULL != skin);

    *numUsedBones = 0;

    for (i = 0; i < numVertices; i++)
    {
        RwUInt32    j;

        for (j = 0; j < skin->vertexMaps.maxWeights; j++)
        {
            if (0 != *((RwUInt32 *)((&weights[i].w0) + j)))
            {
                RwUInt32    k;
                RwUInt32    index;
                RwBool      newBone = TRUE;

                index = (indices[i] >> (8 * j)) & 0xFF;

                for (k = 0; k < *numUsedBones; k++)
                {
                    if (index == usedBoneList[k])
                    {
                        newBone = FALSE;
                        break;
                    }
                }

                if (newBone)
                {
                    usedBoneList[*numUsedBones] = index;
                    (*numUsedBones)++;
                }
            }
        }
    }

    RWRETURNVOID();
}

/*****************************************************************************
 SkinCreateSkinData
 */
static RwBool
SkinCreateSkinData(RpSkin *skin,
                   RwUInt32 numBones,
                   RwUInt32 numUsedBones,
                   RwUInt32 numVertices,
                   RwUInt8 *usedBoneList,
                   RwMatrixWeights *vertexWeights,
                   RwUInt32 *vertexIndices,
                   RwMatrix *inverseMatrices)
{
    RwUInt32    size;

    RWFUNCTION(RWSTRING("SkinCreateSkinData"));
    RWASSERT(NULL != skin);
    RWASSERT(0 < numBones);
    RWASSERT(0 < numUsedBones);
    RWASSERT(0 < numVertices);

    /* Calc memory size */
    size = ((sizeof(RwUInt32) + sizeof(RwMatrixWeights)) * numVertices) +
           (sizeof(RwMatrix) * numBones) +
           (sizeof(RwUInt8) * numUsedBones) + 15;

    /* Allocate & clean the memory */
    skin->unaligned = (void **)RwMalloc(size,
        rwID_SKINPLUGIN | rwMEMHINTDUR_EVENT);
    if (!skin->unaligned)
    {
        RWRETURN(FALSE);
    }

    memset(skin->unaligned, 0, size);

    /* Store the number of bones */
    skin->boneData.numBones = numBones;

    /* Store the number of used bones */
    skin->boneData.numUsedBones = numUsedBones;

    /* Used bone list */
    skin->boneData.usedBoneList = (RwUInt8 *)skin->unaligned;
    RWASSERT(NULL != skin->boneData.usedBoneList);

    /* Inverse bone to skin matrices */
    skin->boneData.invBoneToSkinMat =
        (RwMatrix *)ROUNDUP16(skin->boneData.usedBoneList + numUsedBones);
    RWASSERT(NULL != skin->boneData.invBoneToSkinMat);

    /* Matrix indices indices */
    skin->vertexMaps.matrixIndices =
        (RwUInt32 *)(skin->boneData.invBoneToSkinMat + numBones);
    RWASSERT(NULL != skin->vertexMaps.matrixIndices);

    /* Matrix weights */
    skin->vertexMaps.matrixWeights =
        (RwMatrixWeights *)(skin->vertexMaps.matrixIndices + numVertices);
    RWASSERT(NULL != skin->vertexMaps.matrixWeights);

    /* Set up the used bone list */
    if((NULL != usedBoneList) && (0 != numUsedBones))
    {
        RwUInt32 size;

        size = sizeof(RwUInt8) * numUsedBones;

        memcpy(skin->boneData.usedBoneList, usedBoneList, size);
    }

    /* Set up initial boneToSkin matrices */
    if(NULL != inverseMatrices)
    {
        RwUInt32 iBone;

        iBone = skin->boneData.numBones;
        while (iBone--)
        {
            RwMatrixCopy(&(skin->boneData.invBoneToSkinMat[iBone]),
                         &(inverseMatrices[iBone]));
        }
    }

    /* Setup the skin's vertex maps. */
    if(NULL != vertexIndices)
    {
        RwUInt32 size;

        size = sizeof(RwUInt32) * numVertices;

        memcpy(skin->vertexMaps.matrixIndices, vertexIndices, size);
    }

    if(NULL != vertexWeights)
    {
        RwUInt32 size;

        size = sizeof(RwMatrixWeights) * numVertices;

        memcpy(skin->vertexMaps.matrixWeights, vertexWeights, size);
    }

    RWRETURN(TRUE);
}

static RpSkin *
SkinCreate(RwUInt32 numVertices,
           RwUInt32 numBones,
           RwUInt32 numBonesUsed,
           RwUInt32 maxWeights,
           RwMatrixWeights *vertexWeights,
           RwUInt32 *vertexIndices,
           RwMatrix *inverseMatrices)
{
    RwUInt8     usedBoneList[256];
    RpSkin      *skin;

    RWFUNCTION(RWSTRING("SkinCreate"));
    RWASSERT(0 < numVertices);
    RWASSERT(0 < numBones);

    /* Get a new skin */
    skin = (RpSkin *)RwFreeListAlloc(_rpSkinGlobals.freeList,
                         rwID_SKINPLUGIN | rwMEMHINTDUR_EVENT);
    RWASSERT(NULL != skin);

    /* Clean the skin */
    memset(skin, 0, sizeof(RpSkin));

    /* Find the maximum number of weights used */
    if (0 == maxWeights)
    {
        SkinFindMaxWeights(skin, vertexWeights, numVertices);
    }

    /* Find the number of used bones */
    if (0 == numBonesUsed)
    {
        SkinFindNumUsedBones(
            skin,
            vertexIndices,
            vertexWeights,
            usedBoneList,
            &numBonesUsed,
            numVertices);
    }

    /* Setup the skin's bones */
    if (!SkinCreateSkinData(
        skin,
        numBones,
        numBonesUsed,
        numVertices,
        usedBoneList,
        vertexWeights,
        vertexIndices,
        inverseMatrices))
    {
        RwFreeListFree(_rpSkinGlobals.freeList, skin);
        RWRETURN((RpSkin *)NULL);
    }

    RWRETURN(skin);
}

/*===========================================================================*
 *--- Private Functions -----------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/
static RwInt32 _rpSkinFreeListBlockSize = rpSKINENTRIESPERBLOCK,
               _rpSkinFreeListPreallocBlocks = 1;
static RwFreeList _rpSkinFreeList;

/**
 * \ingroup rpskin
 * \ref RpSkinSetFreeListCreateParams allows the developer to specify
 * how many \ref RpSkin s to preallocate space for.
 * Call before \ref RwEngineInit.
 *
 * \param blockSize  number of entries per freelist block.
 * \param numBlocksToPrealloc  number of blocks to allocate on
 * \ref RwFreeListCreateAndPreallocateSpace.
 *
 * \see RwFreeList
 *
 */
void
RpSkinSetFreeListCreateParams(RwInt32 blockSize,
                              RwInt32 numBlocksToPrealloc)
{
#if 0
    /*
     * Can not use debugging macros since the debugger is not initialized before
     * RwEngineInit.
     */
    /* Do NOT comment out RWAPIFUNCTION as gnumake verify will not function */
    RWAPIFUNCTION(RWSTRING("RpSkinSetFreeListCreateParams"));
#endif
    _rpSkinFreeListBlockSize = blockSize;
    _rpSkinFreeListPreallocBlocks = numBlocksToPrealloc;
#if 0
    RWRETURNVOID();
#endif
}


static void *
SkinOpen( void *instance,
          RwInt32 offset __RWUNUSED__,
          RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("SkinOpen"));
    RWASSERT(NULL != instance);

    /* Create the skinning pipelines, only once. */
    if(_rpSkinGlobals.module.numInstances == 0)
    {
        RwUInt32 pipes;
        RwBool success;
        RwUInt32 size;

        /* Assume we always want the generic pipe. */
        pipes = rpSKINPIPELINESKINGENERIC;

        /* Is the matfx plugin attached? */
        if(MatfxPluginIsAttached())
        {
            pipes |= rpSKINPIPELINESKINMATFX;
        }

        /* Is the toon plugin attached? */
        if(ToonPluginIsAttached())
        {
            pipes |= rpSKINPIPELINESKINTOON;
        }

        /* Set up the pipeline. */
        success = _rpSkinPipelinesCreate(pipes);
        RWASSERT(FALSE != success);

        /* Create a RpSkin freelist. */
        _rpSkinGlobals.freeList = RwFreeListCreateAndPreallocateSpace(
            sizeof(RpSkin),
            _rpSkinFreeListBlockSize,
            rpSKINALIGNMENT,
            _rpSkinFreeListPreallocBlocks,
            &_rpSkinFreeList,
            rwID_SKINPLUGIN | rwMEMHINTDUR_GLOBAL);
        RWASSERT(NULL != _rpSkinGlobals.freeList);

        /* Create the skinning matrix cache. */
        size = (sizeof(RwMatrix) * rpSKINMAXNUMBEROFMATRICES) + 15;

        _rpSkinGlobals.matrixCache.unaligned = RwMalloc(size,
                        rwID_SKINPLUGIN | rwMEMHINTDUR_GLOBAL);
        RWASSERT(NULL != _rpSkinGlobals.matrixCache.unaligned);

        /* Clean the memory. */
        memset(_rpSkinGlobals.matrixCache.unaligned, 0, size);

        _rpSkinGlobals.matrixCache.aligned = (RwMatrix *)
            ROUNDUP16(_rpSkinGlobals.matrixCache.unaligned);
    }

    /* Another instance. */
    _rpSkinGlobals.module.numInstances++;

    RWRETURN(instance);
}

static void *
SkinClose( void *instance,
           RwInt32 offset __RWUNUSED__,
           RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("SkinClose"));
    RWASSERT(NULL != instance);
    RWASSERT(0 < _rpSkinGlobals.module.numInstances);

    /* One less module instance. */
    _rpSkinGlobals.module.numInstances--;

    if(0 == _rpSkinGlobals.module.numInstances)
    {
        RwBool success;

        /* Destroy the skinning pipelines. */
        success = _rpSkinPipelinesDestroy();
        RWASSERT(FALSE != success);

        /* Destroy the skin free list. */
        RWASSERT(NULL != _rpSkinGlobals.freeList);
        RwFreeListDestroy(_rpSkinGlobals.freeList);
        _rpSkinGlobals.freeList = (RwFreeList *)NULL;

        /* Destroy the matrix cache. */
        RWASSERT(NULL != _rpSkinGlobals.matrixCache.unaligned);
        RwFree(_rpSkinGlobals.matrixCache.unaligned);
        _rpSkinGlobals.matrixCache.unaligned = NULL;
    }

    RWRETURN(instance);
}

static void *
SkinGeometryConstructor( void *object,
                         RwInt32 offset __RWUNUSED__,
                         RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("SkinGeometryConstructor"));
    RWASSERT(NULL != object);

    *RPSKINGEOMETRYGETDATA(object) = (RpSkin *)NULL;

    RWRETURN(object);
}

static void *
SkinGeometryDestructor( void *object,
                        RwInt32 offset __RWUNUSED__,
                        RwInt32 size   __RWUNUSED__ )
{
    RpGeometry *geometry;
    RpSkin *skin;

    RWFUNCTION(RWSTRING("SkinGeometryDestructor"));
    RWASSERT(NULL != object);

    /* Get the skin data from the geometry extension . */
    geometry = (RpGeometry *)object;
    skin = *RPSKINGEOMETRYGETDATA(geometry);

    if(NULL != skin)
    {
        /* Give the platforms a chance to destroy the skin. */
        _rpSkinDeinitialize(geometry);

        /* Destroy the skin, and remove the extension. */
        *RPSKINGEOMETRYGETDATA(geometry) = RpSkinDestroy(skin);
    }

    RWASSERT(NULL == *RPSKINGEOMETRYGETDATA(object));
    RWRETURN(object);
}

static void *
SkinGeometryCopy( void *dstObject,
                  const void *srcObject __RWUNUSEDRELEASE__,
                  RwInt32 offset __RWUNUSED__,
                  RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("SkinGeometryCopy"));
    RWASSERT(NULL != srcObject);
    RWASSERT(NULL != dstObject);

    /*
     * Should never be able to get here as there is no
     * RpGeometryCopy/Clone function
     */
    RWASSERT(0);

    RWRETURN(dstObject);
}

static void *
SkinAtomicConstructor( void *object,
                       RwInt32 offset __RWUNUSED__,
                       RwInt32 size   __RWUNUSED__ )
{
    RpAtomic *atomic;
    SkinAtomicData *atomicData;

    RWFUNCTION(RWSTRING("SkinAtomicConstructor"));
    RWASSERT(NULL != object);

    /* Get the atomic and it's extension data. */
    atomic = (RpAtomic *)object;
    atomicData = RPSKINATOMICGETDATA(atomic);

    /* Clean the memory */
    memset(atomicData, 0, sizeof(SkinAtomicData));

    RWRETURN(object);
}

static void *
SkinAtomicDestructor( void *object,
                      RwInt32 offset __RWUNUSED__,
                      RwInt32 size   __RWUNUSED__ )
{
    RpAtomic *atomic;
    SkinAtomicData *atomicData;

    RWFUNCTION(RWSTRING("SkinAtomicDestructor"));
    RWASSERT(NULL != object);

    /* Get the atomic and it's extension data. */
    atomic = (RpAtomic *)object;
    atomicData = RPSKINATOMICGETDATA(atomic);

    if(NULL != atomicData->hierarchy)
    {
        atomicData->hierarchy = (RpHAnimHierarchy *)NULL;
    }

    RWRETURN(object);
}

static void *
SkinAtomicCopy( void *dstObject,
                const void *srcObject,
                RwInt32 offset __RWUNUSED__,
                RwInt32 size   __RWUNUSED__ )
{
    const RpAtomic *srcAtomic;
    const SkinAtomicData *srcAtomicData;

    RpAtomic *dstAtomic;
    SkinAtomicData *dstAtomicData;

    RWFUNCTION(RWSTRING("SkinAtomicCopy"));
    RWASSERT(NULL != srcObject);
    RWASSERT(NULL != dstObject);

    /* Get the atomic from the objects. */
    srcAtomic = (const RpAtomic *)srcObject;
    RWASSERT(NULL != srcAtomic);
    dstAtomic = (RpAtomic *)dstObject;
    RWASSERT(NULL != dstAtomic);

    /* Get and setup the atomic extension. */
    srcAtomicData = RPSKINATOMICGETCONSTDATA(srcAtomic);
    RWASSERT(NULL != srcAtomicData);
    dstAtomicData = RPSKINATOMICGETDATA(dstAtomic);
    RWASSERT(NULL != dstAtomicData);


    /* Copy the present hierarchy. */
    dstAtomicData->hierarchy = srcAtomicData->hierarchy;

    /*
     * Note: no need to set the correct pipeline as this should be
     * done for us (as it's a property of the atomic not the plugin
     * extension.
     */

    RWRETURN(dstObject);
}

static RwBool
SkinAtomicAlways( void *object,
                  RwInt32 offset __RWUNUSED__,
                  RwInt32 size   __RWUNUSED__ )
{
    /* Pre-rights streaming matfxed skinned atomic legacy compatibility.
     * Try and determine the pipe to use to render with from atomic's data.
     * Assume it's generic unless we find some. */
    RpSkinType type = rpSKINTYPEGENERIC;
    RpAtomic *atomic;

    RWFUNCTION(RWSTRING("SkinAtomicAlways"));
    RWASSERT(NULL != object);

    atomic = (RpAtomic *)object;
    RWASSERT(NULL != atomic);

    /* Sneaky macro checks matfx plugin atomic enabled flag
     * without requiring matfx plugin to be attached. */
    #define SKINATOMICGETMATFXENABLED(_atomic)                              \
        ((RwBool)(*(((RwUInt8 *)_atomic) +                                  \
                  RpAtomicGetPluginOffset(rwID_MATERIALEFFECTSPLUGIN))))

    if (MatfxPluginIsAttached() && SKINATOMICGETMATFXENABLED(atomic))
    {
        type = rpSKINTYPEMATFX;
    }

    atomic = SkinAtomicSetup(atomic, type);
    RWASSERT(NULL != atomic);

    RWRETURN(TRUE);
}

static RwBool
SkinAtomicRights( void *object,
                  RwInt32 offset __RWUNUSED__,
                  RwInt32 size   __RWUNUSED__,
                  RwUInt32 extraData )
{
    RpAtomic *atomic;
    RpSkinType type;

    RWFUNCTION(RWSTRING("SkinAtomicRights"));
    RWASSERT(NULL != object);

    atomic = (RpAtomic *)object;
    RWASSERT(NULL != atomic);
    type = (RpSkinType)extraData;

    atomic = SkinAtomicSetup(atomic, type);
    RWASSERT(NULL != atomic);

    RWRETURN(TRUE);
}

static RwInt32
SkinGeometrySize(const void *object,
                 RwInt32 offset __RWUNUSED__,
                 RwInt32 bytes  __RWUNUSED__)
{
    const RpGeometry    *geometry;
    const RpSkin        *skin;
    RwInt32             size = 0;

    RWFUNCTION(RWSTRING("SkinGeometrySize"));
    RWASSERT(NULL != object);

    /* Get the geometry and the skin. */
    geometry = (const RpGeometry *)object;
    RWASSERT(NULL != geometry);
    RWASSERTISTYPE(geometry, rpGEOMETRY);

    skin = *RPSKINGEOMETRYGETCONSTDATA(geometry);

    if (NULL != skin)
    {
        if (!(rpGEOMETRYNATIVE & RpGeometryGetFlags(geometry)))
        {
            RwUInt32    numVertices;

            /* Num of vertices. */
            numVertices = RpGeometryGetNumVertices(geometry);

            /* Max weights & Num bones. */
            size = sizeof(RwUInt32);

            /* Used bone list */
            size += skin->boneData.numUsedBones * sizeof(RwUInt8);

            /* Matrix Indices. */
            size += numVertices * sizeof(RwUInt32);

            /* Matrix Weights. */
            size += numVertices * sizeof(RwMatrixWeights);

            /* Inverse Bone to Skin matrices */
            size += skin->boneData.numBones * sizeof(RwMatrix);

            /* Bone split info. */
            size += _rpSkinSplitDataStreamGetSize(skin);
        }
        else
        {
            size = _rpSkinGeometryNativeSize(geometry);
        }
    }

    RWRETURN(size);
}

static RwStream *
SkinGeometryWrite(RwStream *stream,
                  RwInt32 binaryLength __RWUNUSED__,
                  const void *object,
                  RwInt32 offsetInObject __RWUNUSED__,
                  RwInt32 sizeInObject __RWUNUSED__ )
{
    const RpGeometry    *geometry;
    const RpSkin        *skin;

    RWFUNCTION(RWSTRING("SkinGeometryWrite"));
    RWASSERT(NULL != stream);
    RWASSERT(NULL != object);

    /* Get the geometry and skin */
    geometry = (const RpGeometry *)object;
    RWASSERT(NULL != geometry);
    skin = *RPSKINGEOMETRYGETCONSTDATA(geometry);

    if (NULL != skin)
    {
        if (!(rpGEOMETRYNATIVE & RpGeometryGetFlags(geometry)))
        {
            RwUInt32 skinInfo;
            RwUInt32 numVertices;
            RwStream *success;

            /* Output skin paramters */

            /* Num of vertices */
            numVertices = RpGeometryGetNumVertices(geometry);

            /* Max weights, num used bone & num bones */
            skinInfo =
                ((skin->vertexMaps.maxWeights << 16) & 0x00FF0000) |
                ((skin->boneData.numUsedBones <<  8) & 0x0000FF00) |
                ((skin->boneData.numBones     <<  0) & 0x000000FF);

            success = RwStreamWriteInt32(stream,
                          (const RwInt32 *)&skinInfo,
                          sizeof(RwUInt32));
            CHECKSTREAMANDRETURN(success);

            /* Used bone list */
            success = RwStreamWrite(stream,
                          (void *)(skin->boneData.usedBoneList),
                          sizeof(RwUInt8) * skin->boneData.numUsedBones);
            CHECKSTREAMANDRETURN(success);

            /* Vertex indices */
            success = RwStreamWriteInt32(stream,
                          (const RwInt32 *)(skin->vertexMaps.matrixIndices),
                          sizeof(RwUInt32) * numVertices);
            CHECKSTREAMANDRETURN(success);

            /* Vertex weights */
            success = RwStreamWriteReal(stream,
                          (const RwReal *)(skin->vertexMaps.matrixWeights),
                          sizeof(RwMatrixWeights) * numVertices);
            CHECKSTREAMANDRETURN(success);

            /* Bone info */
            success = RwStreamWriteReal(stream,
                                        (RwReal *)skin->boneData.invBoneToSkinMat,
                                        sizeof(RwMatrix) * skin->boneData.numBones);
            CHECKSTREAMANDRETURN(success);

            /* Bone split info */
            success = _rpSkinSplitDataStreamWrite(stream, skin);
            CHECKSTREAMANDRETURN(success);
        }
        else
        {
            if (NULL == _rpSkinGeometryNativeWrite(stream, geometry))
            {
                RWRETURN(NULL);
            }
        }
    }

    RWRETURN(stream);
}

static RwStream *
SkinGeometryRead( RwStream *stream,
                  RwInt32 binaryLength   __RWUNUSED__,
                  void *object,
                  RwInt32 offsetInObject __RWUNUSED__,
                  RwInt32 sizeInObject   __RWUNUSED__ )
{
    RpGeometry  *geometry;

    RWFUNCTION(RWSTRING("SkinGeometryRead"));
    RWASSERT(NULL != stream);
    RWASSERT(NULL != object);

    /* Get the geometry. */
    geometry = (RpGeometry *)object;
    RWASSERT(NULL != geometry);

    if (!(rpGEOMETRYNATIVE & RpGeometryGetFlags(geometry)))
    {
        RwUInt32    skinInfo;
        RwUInt32    maxWeights;
        RwUInt32    numBones;
        RwUInt32    numUsedBones;
        RwUInt32    numVertices;
        RpSkin      *skin;
        RwStream    *success;
        RwUInt32    size;

        /* Read the max weights and num bones */
        success = RwStreamReadInt32(stream,
                                    (RwInt32 *)&skinInfo,
                                    sizeof(RwInt32));
        CHECKSTREAMANDRETURN(success);

        numBones     = (skinInfo >>  0) & 0xFF;
        numUsedBones = (skinInfo >>  8) & 0xFF;
        maxWeights   = (skinInfo >> 16) & 0xFF;

        /* Get the number of vertices */
        numVertices = RpGeometryGetNumVertices(geometry);

        skin = SkinCreate(numVertices,
                  numBones,
                  numUsedBones,
                  maxWeights,
                  (RwMatrixWeights *)NULL,
                  (RwUInt32 *)NULL,
                  (RwMatrix *)NULL);

        if (!skin)
        {
            RWASSERT(NULL != skin);
            RWRETURN((RwStream *)NULL);
        }

        size = sizeof(RwUInt8) * numUsedBones;

        /* Used bone list */
        if (RwStreamRead(stream, (void *)skin->boneData.usedBoneList, size)
            != size)
        {
            RWRETURN((RwStream *)NULL);
        }

        /* Vertex indices */
        success = RwStreamReadInt32( stream,
                                     (RwInt32 *)skin->vertexMaps.matrixIndices,
                                     sizeof(RwUInt32) * numVertices );
        CHECKSTREAMANDRETURN(success);

        /* Vertex weights */
        success = RwStreamReadReal( stream,
                                    (RwReal *)skin->vertexMaps.matrixWeights,
                                    sizeof(RwMatrixWeights) * numVertices );
        CHECKSTREAMANDRETURN(success);

        skin->vertexMaps.maxWeights = maxWeights;

        /* Inverse bone to skin matrices */
        success = RwStreamReadReal(
            stream,
            (RwReal *)skin->boneData.invBoneToSkinMat,
            sizeof(RwMatrix) * skin->boneData.numBones);
        CHECKSTREAMANDRETURN(success);

        /* Bone split info. */
        success = _rpSkinSplitDataStreamRead(stream, skin);
        CHECKSTREAMANDRETURN(success);

        geometry = RpSkinGeometrySetSkin(geometry, skin);
        RWASSERT(NULL != geometry);
    }
    else
    {
        if (NULL == _rpSkinGeometryNativeRead(stream, geometry))
        {
            RWRETURN(NULL);
        }
    }

    RWRETURN(stream);
}

static RwStream *
SkinAtomicRead( RwStream *stream,
                RwInt32 binaryLength   __RWUNUSED__,
                void *object,
                RwInt32 offsetInObject __RWUNUSED__,
                RwInt32 sizeInObject )
{
    RpAtomic *atomic;
    RpGeometry *geometry;
    RpSkin *skin;
    RwStream *success;

    RWFUNCTION(RWSTRING("SkinAtomicRead"));
    RWASSERT(NULL != stream);
    RWASSERT(NULL != object);

    /*
     * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     *  This function remains to allow backwards
     *  compatibility with old school skinned atomics.
     *  In time it should be removed.
     * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     */

    atomic = (RpAtomic *)object;
    RWASSERT(NULL != atomic);

    geometry = RpAtomicGetGeometry(atomic);
    RWASSERT(NULL != geometry);

    skin = RpSkinGeometryGetSkin(geometry);
    if(NULL == skin)
    {
        RwUInt32 numBones;
        RwUInt32 numVertices;
        RwUInt32 iBone;

        /* Need to load the skin. */

        success = RwStreamReadInt(stream,
                                  (RwInt32 *)&numBones,
                                  sizeof(RwInt32));
        CHECKSTREAMANDRETURN(success);

        numVertices = RpGeometryGetNumVertices(geometry);
        RWASSERT(0 < numVertices);

        skin = SkinCreate(numVertices,
                          numBones,
                          numBones,
                          4,
                          (RwMatrixWeights *)NULL,
                          (RwUInt32 *)NULL,
                          (RwMatrix *)NULL);
        RWASSERT(NULL != skin);

        success = RwStreamSkip(stream, sizeof(RwInt32));
        CHECKSTREAMANDRETURN(success);

        success = RwStreamReadInt(stream,
                                  (RwInt32 *)(skin->vertexMaps.matrixIndices),
                                  sizeof(RwUInt32) * numVertices);
        CHECKSTREAMANDRETURN(success);

        success = RwStreamReadReal(stream,
                                   (RwReal *)(skin->vertexMaps.matrixWeights),
                                   sizeof(RwMatrixWeights) * numVertices );
        CHECKSTREAMANDRETURN(success);

        /* Read the bone info */
        for (iBone = 0; iBone < skin->boneData.numBones; iBone++)
        {
            success = RwStreamSkip(stream, sizeof(RwInt32) * 3);
            CHECKSTREAMANDRETURN(success);

            success = RwStreamReadReal(stream,
                                       (RwReal *)&(skin->boneData.invBoneToSkinMat[iBone]),
                                       sizeof(RwMatrix));
            CHECKSTREAMANDRETURN(success);
        }

        /* Find the maximum number of weights to be used */
        SkinFindMaxWeights(
            skin,
            skin->vertexMaps.matrixWeights,
            numVertices);

        SkinFindNumUsedBones(
            skin,
            skin->vertexMaps.matrixIndices,
            skin->vertexMaps.matrixWeights,
            skin->boneData.usedBoneList,
            &skin->boneData.numUsedBones,
            numVertices);

        geometry = RpSkinGeometrySetSkin(geometry, skin);
        RWASSERT(NULL != geometry);
    }
    else
    {
        success = RwStreamSkip(stream, sizeInObject);
        CHECKSTREAMANDRETURN(success);
    }

    RWRETURN(stream);
}

static RwStream *
SkinAtomicWrite( RwStream *stream,
                 RwInt32 binaryLength   __RWUNUSED__,
                 const void *object     __RWUNUSED__,
                 RwInt32 offsetInObject __RWUNUSED__,
                 RwInt32 sizeInObject   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("SkinAtomicWrite"));
    RWRETURN(stream);
}

static RwInt32
SkinAtomicGetSize( const void *object     __RWUNUSED__,
                   RwInt32 offsetInObject __RWUNUSED__,
                   RwInt32 sizeInObject   __RWUNUSED__ )
{
    RwInt32 size = 0;

    RWFUNCTION(RWSTRING("SkinAtomicGetSize"));

#ifdef SKIN_ATOMIC_PLATFORM_STREAM_SIZE
    size += _rpSkinAtomicNativeSize((RpAtomic *)object);
#endif /* SKIN_ATOMIC_PLATFORM_STREAM_SIZE */

    RWRETURN(size);
}

/*===========================================================================*
 *--- Global Functions ------------------------------------------------------*
 *===========================================================================*/

RwMatrix *
_rpSkinGetAlignedMatrixCache(void)
{
    RWFUNCTION(RWSTRING("_rpSkinGetAlignedMatrixCache"));

    RWRETURN(_rpSkinGlobals.matrixCache.aligned);
}

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rpskin
 * \ref RpSkinPluginAttach is used to attach the skin plugin to the
 * RenderWare system to enable the manipulation of skinned geometries
 * and atomics. The plugin must be attached between initializing the
 * system with \ref RwEngineInit and opening it with \ref RwEngineOpen.
 *
 * \note The skin plugin requires the world plugin to be attached.
 * The include file rpskin.h is also required and must be included by
 * an application wishing to use this plugin.
 *
 * \return TRUE if successful, FALSE if an error occurs.
 */
RwBool
RpSkinPluginAttach(void)
{
    RwInt32 success;

    RWAPIFUNCTION(RWSTRING("RpSkinPluginAttach"));

    /* Register the plugIn */
    _rpSkinGlobals.engineOffset =
        RwEngineRegisterPlugin( 0,
                                rwID_SKINPLUGIN,
                                SkinOpen,
                                SkinClose );
    RWASSERT(0 < _rpSkinGlobals.engineOffset);

    /* Extend atomic to hold skin atomic data. */
    _rpSkinGlobals.atomicOffset =
        RpAtomicRegisterPlugin( sizeof(SkinAtomicData),
                                rwID_SKINPLUGIN,
                                SkinAtomicConstructor,
                                SkinAtomicDestructor,
                                SkinAtomicCopy );
    RWASSERT(0 < _rpSkinGlobals.atomicOffset);

    /* Attach the stream handling functions. */
    success = RpAtomicRegisterPluginStream( rwID_SKINPLUGIN,
                                            SkinAtomicRead,
                                            SkinAtomicWrite,
                                            SkinAtomicGetSize );
    RWASSERT(0 < success);

    /* Attach an always callback streaming function. */
    success = RpAtomicSetStreamAlwaysCallBack( rwID_SKINPLUGIN,
                                               SkinAtomicAlways );
    RWASSERT(0 < success);

    /* Attach a rights callback streaming function. */
    success = RpAtomicSetStreamRightsCallBack( rwID_SKINPLUGIN,
                                               SkinAtomicRights );
    RWASSERT(0 < success);

    /* Extend geometry to hold skin geometry data. */
    _rpSkinGlobals.geometryOffset =
        RpGeometryRegisterPlugin( sizeof(RpSkin *),
                                  rwID_SKINPLUGIN,
                                  SkinGeometryConstructor,
                                  SkinGeometryDestructor,
                                  SkinGeometryCopy );
    RWASSERT(0 < _rpSkinGlobals.geometryOffset);

    /* Attach the stream handling functions */
    success = RpGeometryRegisterPluginStream( rwID_SKINPLUGIN,
                                              SkinGeometryRead,
                                              SkinGeometryWrite,
                                              SkinGeometrySize );
    RWASSERT(0 < success);

    RWRETURN(TRUE);
}

/**
 * \ingroup rpskin
 * \ref RpSkinAtomicSetHAnimHierarchy attaches an \ref RpHAnimHierarchy
 * to the \ref RpAtomic. The hierarchy is used to define the positions
 * and orientation of the \ref RpSkin's bones. A hierarchy should be
 * initialized and attached to the atomic before rendering.
 *
 * The hierarchy information is attached to the atomic, this allows
 * atomics to animate a single skinned \ref RpGeometry.
 *
 * \param atomic    A pointer to the skinned atomic to attach the
 *                  hierarchy to.
 * \param hierarchy A pointer to the \ref RpHAnimHierarchy use to
 *                  animate the skinned geometry.
 *
 * \return A pointer to the atomic if successful, or NULL otherwise.
 *
 * \see RpSkinAtomicGetHAnimHierarchy
 */
RpAtomic *
RpSkinAtomicSetHAnimHierarchy( RpAtomic *atomic,
                               RpHAnimHierarchy *hierarchy )
{
    SkinAtomicData *atomicData;

    RWAPIFUNCTION(RWSTRING("RpSkinAtomicSetHAnimHierarchy"));
    RWASSERT(0 < _rpSkinGlobals.module.numInstances);
    RWASSERT(NULL != atomic);
    RWASSERT((!hierarchy) || (hierarchy->numNodes <= rpSKINMAXNUMBEROFMATRICES));

    atomicData = RPSKINATOMICGETDATA(atomic);
    RWASSERT(NULL != atomicData);

    atomicData->hierarchy = hierarchy;

    RWRETURN(atomic);
}

/**
 * \ingroup rpskin
 * \ref RpSkinAtomicGetHAnimHierarchy returns the
 * \ref RpHAnimHierarchy attached to the \ref RpAtomic.
 *
 * \param atomic A pointer to the skinned atomic to query the
 *               hierarchy.
 *
 * \return A pointer to the attached hierarchy, or NULL otherwise.
 *
 * \see RpSkinAtomicSetHAnimHierarchy
 */
RpHAnimHierarchy *
RpSkinAtomicGetHAnimHierarchy(const RpAtomic *atomic)
{
    const SkinAtomicData *atomicData;

    RWAPIFUNCTION(RWSTRING("RpSkinAtomicGetHAnimHierarchy"));
    RWASSERT(0 < _rpSkinGlobals.module.numInstances);
    RWASSERT(NULL != atomic);

    atomicData = RPSKINATOMICGETCONSTDATA(atomic);
    RWASSERT(NULL != atomicData);

    RWRETURN(atomicData->hierarchy);
}

/**
 * \ingroup rpskin
 * \ref RpSkinGeometryGetSkin returns the \ref RpSkin attached to
 * the \ref RpGeometry.
 *
 * \param geometry A pointer to the geometry to query the skin.
 *
 * \return A pointer to the attached skin, or NULL otherwise.
 *
 * \see RpSkinGeometrySetSkin
 */
RpSkin *
RpSkinGeometryGetSkin(RpGeometry *geometry)
{
    RpSkin *skin;

    RWAPIFUNCTION(RWSTRING("RpSkinGeometryGetSkin"));
    RWASSERT(0 < _rpSkinGlobals.module.numInstances);
    RWASSERT(NULL != geometry);

    /* Grab the skin. */
    skin = *RPSKINGEOMETRYGETDATA(geometry);

    RWRETURN(skin);
}

/**
 * \ingroup rpskin
 * \ref RpSkinGeometrySetSkin attaches the \ref RpSkin to the
 * \ref RpGeometry. This extends the geometry to contain the
 * skinning data.  Attaching the skin to the geometry isn't
 * sufficient to render the geometry as a skinned geometry.
 * The \ref RpAtomic that the geometry is attached to must also
 * have a \ref RpHAnimHierarchy attached to it with
 * \ref RpSkinAtomicSetHAnimHierarchy. Then the atomic should have
 * it's rendering pipeline overloaded with a custom skinning
 * pipeline. The custom skinning can be automatically attached
 * with \ref RpSkinAtomicSetType.
 *
 * \note The skin can also be attached to a \ref RpPatchMesh with
 *       \ref RpPatchMeshSetSkin. The patch mesh should be attached
 *       to an atomic and then have the rendering pipeline overload
 *       with \ref RpPatchAtomicSetType selecting a patch skinning
 *       rendering pipeline.
 *
 * \param geometry A pointer to the geometry to attach the skin.
 * \param skin     A pointer to the skin to attach.
 *
 * \return A pointer to the geometry, or NULL otherwise.
 *
 * \see RpSkinGeometryGetSkin
 */
RpGeometry *
RpSkinGeometrySetSkin( RpGeometry *geometry, RpSkin *skin )
{
    RpSkin *oldSkin;

    RWAPIFUNCTION(RWSTRING("RpSkinGeometrySetSkin"));
    RWASSERT(0 < _rpSkinGlobals.module.numInstances);
    RWASSERT(NULL != geometry);

    oldSkin = *RPSKINGEOMETRYGETDATA(geometry);
    if( skin != oldSkin )
    {
        /* Skins are different hence we need to change something. */
        if( NULL != oldSkin )
        {
            /* Give the platform a chance to tidy up. */
            _rpSkinDeinitialize(geometry);
        }

        /* Set the new skin. */
        *RPSKINGEOMETRYGETDATA(geometry) = skin;

        /* Setup the new skin. */
        if( NULL != skin )
        {
            /* Give the platforms a chance to create the skin. */
            if (!_rpSkinInitialize(geometry))
            {
                RWRETURN(NULL);
            }
        }
    }

    RWRETURN(geometry);
}

/**
 * \ingroup rpskin
 * \ref RpSkinCreate creates an \ref RpSkin which defines the
 * skinning data for a \ref RpGeometry. The skin can then be
 * attached to the geometry with \ref RpSkinGeometrySetSkin.
 *
 * A skin maps a geometry's vertices to a set of bones that can
 * be animated. The bones are defined by an \ref RpHAnimHierarchy,
 * and the animations are normally defined by an
 * \ref RtAnimAnimation.
 *
 * The skin mapping can assign up to four "weights" per vertex
 * to allow subtle blending between bones.
 *
 * Skins use the \ref RwMatrix structures of the
 * \ref RpHAnimHierarchy to define the initial bone transformations.
 *
 * \param numVertices      The number of vertices in the skin. This
 *                         be equal to the number of vertices in the
 *                         geometry.
 *
 * \param numBones         The number of bones in the skin (the
 *                         maximum number is platform specific).
 *                         This is used to define the array of
 *                         inverse bones matrices.
 *
 * \param vertexWeights    An array of weights, one per vertex. Each
 *                         bone weight should be in the range 0.0f to
 *                         1.0f. The total weight for a vertex should
 *                         always sum to 1.0f.
 *                         Process of the weights will stop when the
 *                         weight of a bone is 0.0f. The vertex indices
 *                         must reflect any reordering of the weights.
 *
 * \param vertexIndices    A pointer to an array of bone indices, one
 *                         per vertex. The 4 8-bit indices are packed
 *                         into each \ref RwUInt32 value, with the least
 *                         significant mapping to w0 in the vertex
 *                         weight array.
 *
 * \param inverseMatrices  A pointer to an array of \ref RwMatrix's,
 *                         one per bone. The matrices are often known
 *                         as inverse bone matrices, and define the
 *                         transformation from object-space (or
 *                         skin-space) to bone-space for that bone.
 *
 * The inverse bone matrices are used to transform the vertices from their
 * object-space into their bone-space. The key-frame based animation for
 * the bone in the hierarchy then transforms the vertex position back into
 * object-space in it's animated pose. The model is then rendered.
 *
 * \note A skin should be generated in exactly the same way for a
 *       \ref RpPatchMesh. The number of vertices is replaced by
 *       the number of control points. Instead of attaching the skin
 *       to a geometry the skin is attached to the patch mesh with
 *       \ref RpPatchMeshSetSkin.
 *
 * \return Returns a pointer to the skin that is generated if
 * successful, or NULL if there is an error.
 *
 * \see RpSkinDestroy
 */
RpSkin *
RpSkinCreate( RwUInt32 numVertices,
              RwUInt32 numBones,
              RwMatrixWeights *vertexWeights,
              RwUInt32 *vertexIndices,
              RwMatrix *inverseMatrices )
{
    RpSkin *skin;

    RWAPIFUNCTION(RWSTRING("RpSkinCreate"));
    RWASSERT(0 < _rpSkinGlobals.module.numInstances);
    RWASSERT(0 < numVertices);
    RWASSERT(NULL != vertexWeights);
    RWASSERT(NULL != vertexIndices);
    RWASSERT(NULL != inverseMatrices);
    RWASSERT(0 < numBones);

    /* Create a new skin. */
    skin = SkinCreate(numVertices,
                      numBones,
                      0,
                      0,
                      vertexWeights,
                      vertexIndices,
                      inverseMatrices);
    if (!skin)
    {
        RWRETURN(NULL);
    }

    RWRETURN(skin);
}

/**
 * \ingroup rpskin
 * \ref RpSkinDestroy destroys the \ref RpSkin releasing all it's
 * allocated memory. Destroying a \ref RpGeometry which has a
 * skin attached, will automatically destroy the skin. Any
 * \ref RpHAnimHierarchy used to animate the skin is unaffected.
 *
 * \param skin Pointer to the skin to destroy.
 *
 * \return NULL if the skin was successfully destroyed.
 *
 * \see RpSkinCreate
 */
RpSkin *
RpSkinDestroy(RpSkin *skin)
{
    RWAPIFUNCTION(RWSTRING("RpSkinDestroy"));
    RWASSERT(0 < _rpSkinGlobals.module.numInstances);
    RWASSERT(NULL != skin);

    /* Destroy the skin extension data. */
    if (skin->unaligned)
    {
        RwFree(skin->unaligned);
    }

    /* Destroy skin split data */
    _rpSkinSplitDataDestroy(skin);

    /* Free the skin. */
    RwFreeListFree(_rpSkinGlobals.freeList, skin);
    skin = (RpSkin *)NULL;

    RWRETURN(skin);
}

/**
 * \ingroup rpskin
 * \ref RpSkinGetNumBones returns the number of bones in the
 * \ref RpSkin.
 *
 * \param skin Pointer to skin to query.
 *
 * \return Number of bones.
 *
 * \see RpSkinGetVertexBoneWeights
 * \see RpSkinGetVertexBoneIndices
 * \see RpSkinGetSkinToBoneMatrices
 */
RwUInt32
RpSkinGetNumBones(RpSkin *skin)
{
    RWAPIFUNCTION(RWSTRING("RpSkinGetNumBones"));
    RWASSERT(0 < _rpSkinGlobals.module.numInstances);
    RWASSERT(NULL != skin);

    RWRETURN(skin->boneData.numBones);
}

/**
 * \ingroup rpskin
 * \ref RpSkinGetVertexBoneWeights returns read-only access to the
 * \ref RpSkin's per-vertex array of bone weights.
 *
 * \param skin Pointer to skin to query.
 *
 * \return Read-only per-vertex array of bone weights.
 *
 * \see RpSkinGetNumBones
 * \see RpSkinGetVertexBoneIndices
 * \see RpSkinGetSkinToBoneMatrices
 */
const RwMatrixWeights *
RpSkinGetVertexBoneWeights( RpSkin *skin )
{
    RWAPIFUNCTION(RWSTRING("RpSkinGetVertexBoneWeights"));
    RWASSERT(0 < _rpSkinGlobals.module.numInstances);
    RWASSERT(NULL != skin);

    RWRETURN(skin->vertexMaps.matrixWeights);
}

/**
 * \ingroup rpskin
 * \ref RpSkinGetVertexBoneIndices returns read-only access to the
 * \ref RpSkin's per-vertex array of bone indices.
 *
 * \param skin Pointer to skin to query.
 *
 * \return Read-only per-vertex array of bone indices.
 *
 * \see RpSkinGetNumBones
 * \see RpSkinGetVertexBoneWeights
 * \see RpSkinGetSkinToBoneMatrices
 */
const RwUInt32 *
RpSkinGetVertexBoneIndices( RpSkin *skin )
{
    RWAPIFUNCTION(RWSTRING("RpSkinGetVertexBoneIndices"));
    RWASSERT(0 < _rpSkinGlobals.module.numInstances);
    RWASSERT(NULL != skin);

    RWRETURN(skin->vertexMaps.matrixIndices);
}

/**
 * \ingroup rpskin
 * \ref RpSkinGetSkinToBoneMatrices returns read-only access to
 * the \ref RpSkin's per-bone array of skin to bone space
 * transform matrices.
 *
 * \param skin Pointer to skin to query.
 *
 * \return Read-only per-bone array of skin to bone space
 * transform matrices.
 *
 * \see RpSkinGetNumBones
 * \see RpSkinGetVertexBoneWeights
 * \see RpSkinGetVertexBoneIndices
 */
const RwMatrix *
RpSkinGetSkinToBoneMatrices( RpSkin *skin )
{
    RWAPIFUNCTION(RWSTRING("RpSkinGetSkinToBoneMatrices"));
    RWASSERT(0 < _rpSkinGlobals.module.numInstances);
    RWASSERT(NULL != skin);

    RWRETURN(skin->boneData.invBoneToSkinMat);
}

/**
 * \ingroup rpskin
 * \ref RpSkinIsSplit allows the application to query if the skin has been 'split'
 * to support large bone count models.
 *
 * \param skin Pointer to the skin to query.
 *
 * \return TRUE if the skin has been split, FALSE otherwise.
 *
 * \see RpSkinGetNumBones
 * \see RtSkinSplitAtomicSplitGeometry
 */
RwBool
RpSkinIsSplit( RpSkin *skin )
{
    RwBool      result;

    RWAPIFUNCTION(RWSTRING("RpSkinIsSplit"));
    RWASSERT(0 < _rpSkinGlobals.module.numInstances);
    RWASSERT(NULL != skin);

    result = (skin->skinSplitData.numMeshes > 0) ? TRUE : FALSE;

    RWRETURN(result);
}


/**
 * \ingroup rpskin
 * \ref RpSkinAtomicSetType
 * Set up the skinned \ref RpAtomic with the correct rendering
 * pipeline.
 *
 * The skin plugin must be attached before using this function.
 *
 * \param atomic Pointer to the atomic.
 * \param type   Atomic rendering type.
 *
 * \return A pointer to the \ref RpAtomic if successful, or NULL
 * otherwise.
 *
 * \see RpSkinAtomicGetType
 */
RpAtomic *
RpSkinAtomicSetType( RpAtomic *atomic,
                     RpSkinType type )
{
    RWAPIFUNCTION(RWSTRING("RpSkinAtomicSetType"));
    RWASSERT(0 < _rpSkinGlobals.module.numInstances);
    RWASSERT(NULL != atomic);

    atomic = SkinAtomicAttachBestPipeForAttachedPlugins(atomic, type);
    RWASSERT(NULL != atomic);

    RWRETURN(atomic);
}

/**
 * \ingroup rpskin
 * \ref RpSkinAtomicGetType
 * Returns the \ref RpSkinType of the rendering pipeline attached to
 * the atomic.
 *
 * The skin plugin must be attached before using this function.
 *
 * \param atomic Pointer to the atomic.
 *
 * \return The \ref RpSkinType of the pipeline attached to the atomic,
 * or \ref rpNASKINTYPE otherwise.
 *
 * \see RpSkinAtomicSetType
 */
RpSkinType
RpSkinAtomicGetType( RpAtomic *atomic )
{
    RxPipeline *pipeline;
    RpSkinType type;

    RWAPIFUNCTION(RWSTRING("RpSkinAtomicGetType"));
    RWASSERT(0 < _rpSkinGlobals.module.numInstances);
    RWASSERT(NULL != atomic);

    RpAtomicGetPipeline(atomic, &pipeline);
    RWASSERT(NULL != pipeline);

    type = (pipeline->pluginId == rwID_SKINPLUGIN)
         ? (RpSkinType)(pipeline->pluginData)
         : rpNASKINTYPE;

    RWRETURN(type);
}

