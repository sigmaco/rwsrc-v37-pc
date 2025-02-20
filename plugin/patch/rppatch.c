 /*===========================================================================*
 *-                                                                         -*
 *-  Module  :   rppatch.c                                                  -*
 *-                                                                         -*
 *-  Purpose :   General patch handling.                                    -*
 *-                                                                         -*
 *===========================================================================*/

/**
 * \ingroup rppatch
 * \page rppatchoverview RpPatch Plugin Overview
 *
 * \par Requirements
 *
 * There are presently four versions of the RpPatch libraries in the RenderWare
 * Graphics SDK. They are all fully featured versions of the RpPatch plugin
 * and they contain identical APIs. However, because the rendering pipelines
 * are large, different versions have been compiled so that the user can select
 * precisely the pipelines they will be using to link against.
 *
 * \par Requirements for rppatch library
 * The rppatch.lib library only contains the \ref rpPATCHTYPEGENERIC pipeline.
 * \li \b Headers: rwcore.h, rpworld.h, rtbezpat.h, rppatch.h
 * \li \b Libraries: rwcore, rpworld, rtbezpat, rppatch
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpPatchPluginAttach
 *
 * \par Requirements for rppatchmatfx library
 * The rppatchmatfx.lib library contains both the \ref rpPATCHTYPEGENERIC and
 * \ref rpPATCHTYPEMATFX pipelines.
 * \li \b Headers: rwcore.h, rpworld.h, rpmatfx.h, rtbezpat.h, rppatch.h
 * \li \b Libraries: rwcore, rpworld, rpmatfx, rtbezpat, rppatchmatfx
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpMatFXPluginAttach,
 *     \ref RpPatchPluginAttach
 *
 * \par Requirements for rppatchskin library
 * The rppatchskin.lib library contains both the \ref rpPATCHTYPEGENERIC and
 * \ref rpPATCHTYPESKIN pipelines.
 * \li \b Headers: rwcore.h, rpworld.h, rpskin.h, rtbezpat.h, rppatch.h,
 * \li \b Libraries: rwcore, rpworld, rpskin, rtbezpat, rppatchskin
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpSkinPluginAttach,
 *     \ref RpPatchPluginAttach
 *
 * \par Requirements for rppatchskinmatfx library
 * The rppatchskinmatfx.lib library contains all the pipelines,
 * \ref rpPATCHTYPEGENERIC, \ref rpPATCHTYPESKIN, \ref rpPATCHTYPEMATFX and
 * \ref rpPATCHTYPESKINMATFX.
 * \li \b Headers: rwcore.h, rpworld.h, rpmatfx.h, rpskin.h, rtbezpat.h, rppatch.h
 * \li \b Libraries: rwcore, rpworld, rpmatfx, rpskin, rtbezpat, rppatchskinmatfx
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpMatFXPluginAttach,
 *     \ref RpSkinPluginAttach, \ref RpPatchPluginAttach
 *
 * \note Only one of the patch libraries should be used in an application at once.
 *
 * \subsection patchoverview Overview
 * 3-D Bézier patches
 *
 * The RpPatch plugin extends the RenderWare Graphics API to
 * support patches. Quadrilateral and triangular bi-cubic patches
 * are supported using 16 and 10 control points respectively.
 *
 * A patch provides several benefits for representing curved
 * surfaces over a regular triangular mesh because of some of
 * its inherent properties.
 *
 * The advantages of using patches rather than triangular
 * meshes are:
 *
 * \li Data compactness:
 *     A single surface can be represented by 10 or 16
 *     control points. This gives a saving in memory usage.
 *
 * \li A patch has LOD (Level Of Detail):
 *     Because a patch can be refined to any LOD.
 *     The level of refinement can be coarse for distant patches
 *     and it can be made finer for close patches. This allows
 *     an improvement in performance and visual quality.
 *
 * The disadvantage of using patches is that a patch is not
 * directly renderable. A triangular mesh must be generated from
 * the patch control points.
 *
 * The process of refining the patch mesh's control points into
 * a triangle mesh varies on each platform. Depending on the
 * hardware of the host platform the patch mesh will either be
 * refined and retained or refined every frame:
 * \li Retain:
 *     To avoid regenerating the triangular mesh per frame, the
 *     current refined triangle mesh is cached into a \ref RwResEntry
 *     (a block of memory in the resources arena). This mesh is
 *     reused until the mesh needs to be refined. This typically
 *     occurs when the LOD has changed, forcing a regeneration.
 *
 * \li Regeneration:
 *     The patch mesh is refined every frame. This process has been
 *     extensively optimized to be quicker than retaining the
 *     triangle mesh. This process is faster at rendering the
 *     patch mesh and also saves a large amount of memory as the
 *     triangle mesh is not retained.
 *
 * \par Creating and initializing a Patch Mesh
 * \par
 *
 * Before using any of the RpPatch functions the plugin should be
 * attached with:
 * \li \ref RpPatchPluginAttach
 *
 * The patch plugin defines a new type, \ref RpPatchMesh . This
 * represents the collection of patches and has similar functions
 * to that of \ref RpGeometry.
 *
 * There are two different types of patches:
 * \li \ref RpQuadPatch quad patches, and
 * \li \ref RpTriPatch tri patches.
 *
 * A \ref RpPatchMesh should be initially setup with:
 * \li \ref RpPatchMeshCreate
 *          Creates an empty patch mesh, the number of
 *          \ref RpQuadPatch's, \ref RpTriPatch 's and control points
 *          must be specified to allocate the correct space. The
 *          \ref RpPatchMeshFlag's define which attributes the
 *          \ref RpPatchMesh contains.
 *
 * \li \ref RpPatchMeshDestroy
 *          Destroys an \ref RpPatchMesh. All data associated with it
 *          is freed.
 *
 * \li \ref RpPatchMeshLock
 *          Locks the data elements of an \ref RpPatchMesh allowing
 *          the users to edit them. The \ref RpPatchMeshLockMode
 *          bit flags define which elements within the
 *          \ref RpPatchMesh the developer wants to access.
 *
 * \li \ref RpPatchMeshUnlock
 *          Unlocks a previously locked \ref RpPatchMesh. This allows
 *          the patch plugin the opportunity to prepare the
 *          \ref RpPatchMesh for rendering.
 *
 * Information on a previously created \ref RpPatchMesh can be
 * retrieved with:
 * \li \ref RpPatchMeshGetNumControlPoints
 * \li \ref RpPatchMeshGetNumTriPatches
 * \li \ref RpPatchMeshGetNumQuadPatches
 * \li \ref RpPatchMeshGetNumTexCoordSets
 *
 * The \ref RpPatchMeshFlag's can be accessed with:
 * \li \ref RpPatchMeshGetFlags
 * \li \ref RpPatchMeshSetFlags \par
 *          However changing an \ref RpPatchMesh's flags will not
 *          automatically allocate memory, as it would if the flags had
 *          been defined at creation time.
 *
 * The control points can be setup by accessing them either directly
 * in the \ref RpPatchMesh and \ref RpPatchMeshDefinition structures
 * or using the following functions:
 * \li \ref RpPatchMeshGetPositions
 * \li \ref RpPatchMeshGetNormals
 * \li \ref RpPatchMeshGetPreLightColors
 * \li \ref RpPatchMeshGetTexCoords
 *
 * Once the control points have be defined, \ref RpQuadPatch 'es and
 * \ref RpTriPatch 'es can be added to the \ref RpPatchMesh. The
 * patches contain either \ref rpQUADPATCHNUMCONTROLINDICES or
 * \ref rpTRIPATCHNUMCONTROLINDICES indices into the control points.
 * This allows the patches to share control points. The patches
 * should be setup with the following functions:
 * \li \ref RpPatchMeshSetQuadPatch
 * \li \ref RpPatchMeshSetTriPatch
 * \li \ref RpPatchMeshGetQuadPatch
 * \li \ref RpPatchMeshGetTriPatch
 *
 * \par Materials
 * \par
 *
 * Each patch within the patch mesh should normally have an \ref RpMaterial
 * attached to it. The patches are grouped by material when rendered.
 * The patch materials should be accessed with the following functions:
 * \li \ref RpPatchMeshSetQuadPatchMaterial
 * \li \ref RpPatchMeshSetTriPatchMaterial
 * \li \ref RpPatchMeshGetQuadPatchMaterial
 * \li \ref RpPatchMeshGetTriPatchMaterial
 *
 * The \ref RpMaterial's attached to an \ref RpPatchMesh can be
 * accessed by the following functions:
 * \li \ref RpPatchMeshForAllMaterials
 * \li \ref RpPatchMeshGetNumMaterials
 * \li \ref RpPatchMeshGetMaterial
 *
 * \par Streaming
 * \par
 *
 * \ref RpPatchMesh 'es may be written to and read from a binary
 * stream using:
 * \li \ref RpPatchMeshStreamGetSize
 * \li \ref RpPatchMeshStreamRead
 * \li \ref RpPatchMeshStreamWrite
 *
 * \par Atomics
 * \par
 *
 * An \ref RpPatchMesh defines but does not render its mesh. It must be attached
 * to an RpAtomic to be rendered; a system very similar to that used with the
 * \ref RpGeometry.
 * \li \ref RpPatchAtomicSetPatchMesh
 *          Once an \ref RpPatchMesh has been attached to an
 *          \ref RpAtomic, any functions relating to
 *          \ref RpGeometry's are undefined. Hence it isn't possible
 *          to attach an \ref RpPatchMesh and an \ref RpGeometry to
 *          the same \ref RpAtomic.
 * \li \ref RpPatchAtomicGetPatchMesh
 *
 * \par Skinning
 * \par
 *
 * A patch mesh can be skinned in the same way as a geometry. An
 * \ref RpSkin should be created as usual for the control points
 * and then attached to the \ref RpPatchMesh with:
 * \li \ref RpPatchMeshGetSkin
 * \li \ref RpPatchMeshSetSkin
 *
 * \par Level Of Detail (LOD)
 * \par
 *
 * When the patches rendering the control points are refined
 * into triangles the level of tesselation is controlled by
 * LOD callback functions. All patch atomics by default
 * will use the default LOD function. The user may control this
 * by defining the LOD ranges:
 * \li \ref RpPatchSetDefaultLODCallBackRange
 * \li \ref RpPatchGetDefaultLODCallBackRange
 *
 * The following types are defined for use with the LOD callback
 * system:
 * \li \ref RpPatchLODRange
 * \li \ref RpPatchLODUserData
 * \li \ref RpPatchLODCallBack
 *
 * It is possible to overload the LOD callback function per patch
 * atomic, this allows the user to vary the callback function on
 * a per object basis:
 * \li \ref RpPatchAtomicSetPatchLODCallBack
 * \li \ref RpPatchAtomicGetPatchLODCallBack
 *
 * \par Pipelines
 * \par
 *
 * The patch plugin makes no assumptions about how to render the
 * patch atomics. Once an \ref RpPatchMesh has been attached to an
 * \ref RpAtomic it is necessary to attach a suitable patch
 * rendering pipeline. The patch plugin supports four different
 * rendering types, these are defined in the \ref RpPatchType
 * enumeration:
 * \li \ref rpPATCHTYPEGENERIC
 *          The patch \ref RpAtomic will be rendered with the
 *          default generic patch rendering pipeline.
 * \li \ref rpPATCHTYPESKIN
 *          The patch \ref RpAtomic will be rendered with a
 *          custom pipeline for rendering skinning patches. Make sure
 *          an \ref RpSkin has been attached to the \ref RpPatchMesh
 *          and an \ref RpHAnimHierarchy has been attached to the
 *          \ref RpAtomic.
 * \li \ref rpPATCHTYPEMATFX
 *          The patch \ref RpAtomic will be rendered with a
 *          custom pipeline for rendering the material effects
 *          of patches. The
 *          patch matfx pipeline supports all the material effects
 *          defined in the \ref rpmatfx plugin. The patches
 *          materials should be setup as usual.
 * \li \ref rpPATCHTYPESKINMATFX
 *          The patch \ref RpAtomic will be rendered with a
 *          custom skinned material effects patch rendering pipeline.
 *          The \ref RpPatchMesh, \ref RpAtomic and the patches'
 *          \ref RpMaterial's must be setup correctly.
 *
 * The \ref RpAtomic type is setup with the following functions:
 * \li \ref RpPatchAtomicSetType
 * \li \ref RpPatchAtomicGetType
 *
 */

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rppatch.h"


#include "patchatomic.h"
#include "patchgeometry.h"
#include "patch.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/
#define rpPATCHENTRIESPERBLOCK 20
#define rpPATCHALIGNMENT sizeof(RwUInt32)

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
#if (defined(RWDEBUG))
long _rpPatchStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

static RwInt32 _rpPatchAtomicFreeListBlockSize = rpPATCHENTRIESPERBLOCK;
static RwInt32 _rpPatchAtomicFreeListPreallocBlocks = 1;
static RwFreeList _rpPatchAtomicFreeList;

static RwInt32 _rpPatchGeometryFreeListBlockSize = rpPATCHENTRIESPERBLOCK;
static RwInt32 _rpPatchGeometryFreeListPreallocBlocks = 1;
static RwFreeList _rpPatchGeometryFreeList;

/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*****************************************************************************
 - PatchEngineOpen
 - Creates the patch pipelines and the global free list.
 - Inputs :   instance - Pointer to engine instance.
 - Outputs:   void *   - Pointer to engine instance - on success.
 */
static void *
PatchEngineOpen( void *instance,
                 RwInt32 offset __RWUNUSED__,
                 RwInt32 size   __RWUNUSED__ )
{
    RpPatchLODRange lodRange;

    RWFUNCTION(RWSTRING("PatchEngineOpen"));
    RWASSERT(NULL != instance);

    /* Create the patch pipelines, only once. */
    if(_rpPatchGlobals.module.numInstances == 0)
    {
        RwUInt32 pipes;
        RwInt32 offset;
        RwBool success;

        /* Assume we always want the generic pipe. */
        pipes = rpPATCHPIPELINEGENERIC;

        /* Is the skin plugin attached? */
        offset = RwEngineGetPluginOffset(rwID_SKINPLUGIN);
        if(-1 != offset)
        {
            pipes |= rpPATCHPIPELINESKINNED;
        }

        /* Is the matfx plugin attached? */
        offset = RwEngineGetPluginOffset(rwID_MATERIALEFFECTSPLUGIN);
        if(-1 != offset)
        {
            pipes |= rpPATCHPIPELINEMATFX;
        }

        /* Create the pipelines. */
        success = _rpPatchPipelinesCreate(pipes);
        RWASSERT(FALSE != success);

        /* Create a PatchAtomicData freelist. */
        _rpPatchGlobals.atomicFreeList =
            RwFreeListCreateAndPreallocateSpace( sizeof(PatchAtomicData),
                              _rpPatchAtomicFreeListBlockSize,
                              rpPATCHALIGNMENT,
                              _rpPatchAtomicFreeListPreallocBlocks,
                              &_rpPatchAtomicFreeList,
                              rwID_PATCHPLUGIN | rwMEMHINTDUR_GLOBAL);
        RWASSERT(NULL != _rpPatchGlobals.atomicFreeList);

        /* Create a PatchGeometryData freelist. */
        _rpPatchGlobals.geometryFreeList =
            RwFreeListCreateAndPreallocateSpace( sizeof(PatchGeometryData),
                              _rpPatchGeometryFreeListBlockSize,
                              rpPATCHALIGNMENT,
                              _rpPatchGeometryFreeListPreallocBlocks,
                              &_rpPatchGeometryFreeList,
                              rwID_PATCHPLUGIN | rwMEMHINTDUR_GLOBAL);
        RWASSERT(NULL != _rpPatchGlobals.geometryFreeList);
    }

    /* Another instance. */
    _rpPatchGlobals.module.numInstances++;

    /* Setup the default LOD. */
    lodRange.minLod = rpPATCHDEFAULTMINLOD;
    lodRange.maxLod = rpPATCHDEFAULTMAXLOD;
    lodRange.minRange = rpPATCHDEFAULTMINRANGE;
    lodRange.maxRange = rpPATCHDEFAULTMAXRANGE;

    RpPatchSetDefaultLODCallBackRange(&lodRange);

    RWRETURN(instance);
}

/*****************************************************************************
 - PatchEngineClose
 - Destroys the patch pipelines and the global free list.
 - Inputs :   instance - Pointer to engine instance.
 - Outputs:   void *   - Pointer to engine instance - on success.
 */
static void *
PatchEngineClose( void *instance,
                  RwInt32 offset __RWUNUSED__,
                  RwInt32 size   __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("PatchEngineClose"));
    RWASSERT(NULL != instance);
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);

    /* One less module instance. */
    _rpPatchGlobals.module.numInstances--;

    if(0 == _rpPatchGlobals.module.numInstances)
    {
        RwBool success;

        /* Destroy the patching pipelines. */
        success = _rpPatchPipelinesDestroy();
        RWASSERT(FALSE != success);

        /* Destroy the patch atomic free list. */
        RWASSERT(NULL != _rpPatchGlobals.atomicFreeList);
        RwFreeListDestroy(_rpPatchGlobals.atomicFreeList);
        _rpPatchGlobals.atomicFreeList = (RwFreeList *)NULL;

        /* Destroy the patch geometry free list. */
        RWASSERT(NULL != _rpPatchGlobals.geometryFreeList);
        RwFreeListDestroy(_rpPatchGlobals.geometryFreeList);
        _rpPatchGlobals.geometryFreeList = (RwFreeList *)NULL;
    }

    RWRETURN(instance);
}

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rppatch
 * \ref RpPatchAtomicSetFreeListCreateParams allows the developer to specify
 * how many PatchAtomicData s to preallocate space for.
 * Call before \ref RwEngineInit.
 *
 * \param blockSize  number of entries per freelist block.
 * \param numBlocksToPrealloc  number of blocks to allocate on
 * \ref RwFreeListCreateAndPreallocateSpace.
 *
 * \see RwFreeList
 */
void
RpPatchAtomicSetFreeListCreateParams(RwInt32 blockSize,
                                     RwInt32 numBlocksToPrealloc)
{
    /* Can not use debugging macros since the debugger is not initialized before
     * RwEngineInit. */
    /* Do NOT comment out RWAPIFUNCTION as gnumake verify will not function */
#if 0
    RWAPIFUNCTION(RWSTRING("RpPatchAtomicSetFreeListCreateParams"));
#endif
    _rpPatchAtomicFreeListBlockSize = blockSize;
    _rpPatchAtomicFreeListPreallocBlocks = numBlocksToPrealloc;
#if 0
    RWRETURNVOID();
#endif
}

/**
 * \ingroup rppatch
 * \ref RpPatchGeometrySetFreeListCreateParams allows the developer to specify
 * how many PatchGeometryData s to preallocate space for.
 * Call before \ref RwEngineInit.
 *
 * \param blockSize  number of entries per freelist block.
 * \param numBlocksToPrealloc  number of blocks to allocate on
 * \ref RwFreeListCreateAndPreallocateSpace.
 *
 * \see RwFreeList
 */
void
RpPatchGeometrySetFreeListCreateParams( RwInt32 blockSize, RwInt32 numBlocksToPrealloc )
{
    /* Can not use debugging macros since the debugger is not initialized before
     * RwEngineInit. */
    /* Do NOT comment out RWAPIFUNCTION as gnumake verify will not function */
#if 0
    RWAPIFUNCTION(RWSTRING("RpPatchGeometrySetFreeListCreateParams"));
#endif

    _rpPatchGeometryFreeListBlockSize = blockSize;
    _rpPatchGeometryFreeListPreallocBlocks = numBlocksToPrealloc;

#if 0
    RWRETURNVOID();
#endif
}

/**
 * \ingroup rppatch
 * \ref RpPatchPluginAttach is used to attach the patch plugin to the
 * RenderWare system to enable the use of patch modelling for
 * atomics. The patch plugin must be attached between initializing the
 * system with \ref RwEngineInit and opening it with \ref RwEngineOpen.
 *
 * Note that the include file rppatch.h is required and must be included
 * by an application wishing to use this plugin. The patch plugin library is
 * contained in the file rppatch.lib.
 *
 * \return TRUE on success: FALSE otherwise
 */
RwBool
RpPatchPluginAttach(void)
{
    RwInt32 success;

    RWAPIFUNCTION(RWSTRING("RpPatchPluginAttach"));

    /* Register the plugin. */
    _rpPatchGlobals.engineOffset =
        RwEngineRegisterPlugin( sizeof(PatchEngineData),
                                rwID_PATCHPLUGIN,
                                PatchEngineOpen,
                                PatchEngineClose );
    RWASSERT(0 < _rpPatchGlobals.engineOffset);

    /* Extend atomic to hold patch atomic data. */
    _rpPatchGlobals.atomicOffset =
        RpAtomicRegisterPlugin( sizeof(PatchAtomicData),
                                rwID_PATCHPLUGIN,
                                _rpPatchAtomicConstructor,
                                _rpPatchAtomicDestructor,
                                _rpPatchAtomicCopy );
    RWASSERT(0 < _rpPatchGlobals.atomicOffset);

    /* Attach the atomic stream handling functions. */
    success = RpAtomicRegisterPluginStream( rwID_PATCHPLUGIN,
                                            _rpPatchAtomicRead,
                                            _rpPatchAtomicWrite,
                                            _rpPatchAtomicGetSize );
    RWASSERT(0 < success);

    /* Attach an always callback streaming function. */
    success = RpAtomicSetStreamAlwaysCallBack( rwID_PATCHPLUGIN,
                                               _rpPatchAtomicAlways );
    RWASSERT(0 < success);

    /* Attach a rights callback streaming function. */
    success = RpAtomicSetStreamRightsCallBack( rwID_PATCHPLUGIN,
                                               _rpPatchAtomicRights );
    RWASSERT(0 < success);

    /* Extend geometry to hold patch geometry data. */
    _rpPatchGlobals.geometryOffset =
        RpGeometryRegisterPlugin( sizeof(PatchGeometryData),
                                  rwID_PATCHPLUGIN,
                                  _rpPatchGeometryConstructor,
                                  _rpPatchGeometryDestructor,
                                  _rpPatchGeometryCopy );
    RWASSERT(0 < _rpPatchGlobals.geometryOffset);

    /* Attach the stream handling functions */
    success = RpGeometryRegisterPluginStream( rwID_PATCHPLUGIN,
                                              _rpPatchGeometryRead,
                                              _rpPatchGeometryWrite,
                                              _rpPatchGeometryGetSize );
    RWASSERT(0 < success);

    /* Attach an always callback streaming function. */
    success = RpGeometrySetStreamAlwaysCallBack( rwID_PATCHPLUGIN,
                                                 _rpPatchGeometryAlways );
    RWASSERT(0 < success);

    RWRETURN(TRUE);
}
