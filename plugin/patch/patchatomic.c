/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   patchatomic.c                                              -*
 *-                                                                         -*
 *-  Purpose :   General patch handling.                                    -*
 *-                                                                         -*
 *===========================================================================*/

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
#include "patchstream.h"
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

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/
static RpAtomic *
PatchAtomicSetup( RpAtomic *atomic,
                  RpPatchType type )
{
    PatchAtomicData *atomicData;
    RpGeometry *geometry;
    PatchGeometryData *geometryData;

    RWFUNCTION(RWSTRING("PatchAtomicSetup"));
    RWASSERT(NULL != atomic);
    atomicData = PATCHATOMICGETDATA(atomic);
    RWASSERT(NULL != atomicData);
    geometry = RpAtomicGetGeometry(atomic);
    RWASSERT(NULL != geometry);
    geometryData = PATCHGEOMETRYGETDATA(geometry);
    RWASSERT(NULL != geometryData);


    if (NULL != geometryData->mesh)
    {
        /* Get the patch mesh. */
        atomicData->patchMesh = geometryData->mesh;
        _rpPatchMeshAddRef(atomicData->patchMesh);

        atomic = RpPatchAtomicSetType(atomic, type);
        RWASSERT(NULL != atomic);
    }

    RWRETURN(atomic);
}

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*****************************************************************************
 _rpPatchAtomicConstructor

 Patch plugin atomic constructor extension.

 Inputs:  object - The atomic being constructed.
          offset - Offset of extension data in atomic.
          size   - Size of extension data.
 Outputs: void * - The RpAtomic if successful.
 */
void *
_rpPatchAtomicConstructor( void *object,
                           RwInt32 offset __RWUNUSED__,
                           RwInt32 size __RWUNUSED__ )
{
    RpAtomic *atomic;
    PatchAtomicData *atomicData;

    RWFUNCTION(RWSTRING("_rpPatchAtomicConstructor"));
    RWASSERT(NULL != object);

    atomic = (RpAtomic *)object;
    RWASSERT(NULL != atomic);
    atomicData = PATCHATOMICGETDATA(atomic);
    RWASSERT(NULL != atomicData);

    atomicData->patchMesh = (PatchMesh *)NULL;
    atomicData->lod.callback = _rpPatchLODAtomicDefaultSelectLOD;
    atomicData->lod.userData = (RpPatchLODUserData)NULL;

#ifdef EXTEND_rpPatchAtomicConstructor
    EXTEND_rpPatchAtomicConstructor;
#endif /* EXTEND_rpPatchAtomicConstructor */

    RWRETURN(object);
}

/*****************************************************************************
 _rpPatchAtomicDestructor

 Patch plugin atomic destructor extension.

 Inputs:  object - The atomic being destroyed.
          offset - Offset of the patch extension data in atomic.
          size   - Size of the patch extension data.
 Outputs: void * - The RpAtomic if successful.
 */
void *
_rpPatchAtomicDestructor( void *object,
                          RwInt32 offset __RWUNUSED__,
                          RwInt32 size __RWUNUSED__ )
{
    RpAtomic *atomic;
    PatchAtomicData *atomicData;

    RWFUNCTION(RWSTRING("_rpPatchAtomicDestructor"));
    RWASSERT(NULL != object);

    atomic = (RpAtomic *)object;
    RWASSERT(NULL != atomic);
    atomicData = PATCHATOMICGETDATA(atomic);
    RWASSERT(NULL != atomicData);

    if(NULL != atomicData->patchMesh)
    {
        PatchMesh *mesh;
        RwBool success;

        mesh = _rpPatchMeshGetInternal(atomicData->patchMesh);
        RWASSERT(NULL != mesh);

        success = _rpPatchMeshDestroy(mesh);
        RWASSERT(TRUE == success);
    }

    atomicData->patchMesh = (PatchMesh *)NULL;
    atomicData->lod.callback = (RpPatchLODCallBack)NULL;
    atomicData->lod.userData = (RpPatchLODUserData)NULL;

#ifdef EXTEND_rpPatchAtomicDestructor
    EXTEND_rpPatchAtomicDestructor;
#endif /* EXTEND_rpPatchAtomicDestructor */

    RWRETURN(object);
}

/*****************************************************************************
 _rpPatchAtomicCopy

 Patch plugin atomic copy extension.

 Inputs: dstObject - Destination atomic.
         srcObject - Source atomic.
         offset    - Offset of atomic extension.
         size      - Size of extension data.
 Outputs: void * - RpAtomic of destination atomic.
 */
void *
_rpPatchAtomicCopy( void *dstObject,
                    const void *srcObject,
                    RwInt32 offset __RWUNUSED__,
                    RwInt32 size __RWUNUSED__ )
{
    const RpAtomic *srcAtomic;
    RpAtomic *dstAtomic;
    const PatchAtomicData *srcAtomicData;
    PatchAtomicData *dstAtomicData;

    RWFUNCTION(RWSTRING("_rpPatchAtomicCopy"));
    RWASSERT(NULL != dstObject);
    RWASSERT(NULL != srcObject);

    srcAtomic = (const RpAtomic *)srcObject;
    RWASSERT(NULL != srcAtomic);
    dstAtomic = (RpAtomic *)dstObject;
    RWASSERT(NULL != dstAtomic);
    srcAtomicData = PATCHATOMICGETCONSTDATA(srcAtomic);
    RWASSERT(NULL != srcAtomicData);
    dstAtomicData = PATCHATOMICGETDATA(dstAtomic);
    RWASSERT(NULL != dstAtomicData);

    dstAtomicData->patchMesh = srcAtomicData->patchMesh;
    dstAtomicData->lod.callback = srcAtomicData->lod.callback;
    dstAtomicData->lod.userData = srcAtomicData->lod.userData;

    if(NULL != dstAtomicData->patchMesh)
    {
        PatchMesh *mesh;

        mesh = _rpPatchMeshGetInternal(dstAtomicData->patchMesh);
        RWASSERT(NULL != mesh);

        mesh = _rpPatchMeshAddRef(mesh);
        RWASSERT(NULL != mesh);
    }

#ifdef EXTEND_rpPatchAtomicCopy
    EXTEND_rpPatchAtomicCopy;
#endif /* EXTEND_rpPatchAtomicCopy */

    RWRETURN(dstObject);
}

/*****************************************************************************
 _rpPatchAtomicGetSize

 Patch atomic stream get size.

 Inputs: object - RpAtomic object.
         offset - Offset of patch extension data.
         size   - Size of patch extension data.
 Outputs: RwInt32 - Size of extension data in stream.
  */
RwInt32
_rpPatchAtomicGetSize( const void *object,
                       RwInt32 offset __RWUNUSED__,
                       RwInt32 size __RWUNUSED__ )
{
    const RpAtomic *atomic;
    const PatchAtomicData *atomicData;

    RwInt32 sizeTotal;

    RWFUNCTION(RWSTRING("_rpPatchAtomicGetSize"));
    RWASSERT(NULL != object);

    atomic = (const RpAtomic *)object;
    RWASSERT(NULL != atomic);
    atomicData = PATCHATOMICGETCONSTDATA(atomic);
    RWASSERT(NULL != atomicData);

    /* Nothing to do, at the moment.  */
    sizeTotal = 0;

#ifdef EXTEND_rpPatchAtomicGetSize
    sizeTotal += EXTEND_rpPatchAtomicGetSize;
#endif /* EXTEND_rpPatchAtomicGetSize */

    RWRETURN(sizeTotal);
}

/*****************************************************************************
 _rpPatchAtomicRead

 Patch atomic stream read.

 Inputs: stream - RwStream to read from.
         length - Length of patch data in stream.
         object - RpAtomic object we're streaming.
         offset - Offset of patch extension in atomic.
         size   - Size of patch externsion data.
 Outputs: RwStream * - The stream if successful.
 */
RwStream *
_rpPatchAtomicRead( RwStream *stream,
                    RwInt32 length __RWUNUSED__,
                    void *object,
                    RwInt32 offset __RWUNUSED__,
                    RwInt32 size __RWUNUSED__ )
{
    RpAtomic *atomic;
    PatchAtomicData *atomicData;

    RWFUNCTION(RWSTRING("_rpPatchAtomicRead"));
    RWASSERT(NULL != stream);
    RWASSERT(NULL != object);

    atomic = (RpAtomic *)object;
    RWASSERT(NULL != atomic);
    atomicData = PATCHATOMICGETDATA(atomic);
    RWASSERT(NULL != atomicData);

    /* Nothing to do, at the moment.  */

#ifdef EXTEND_rpPatchAtomicRead
    EXTEND_rpPatchAtomicRead;
#endif /* EXTEND_rpPatchAtomicRead */

    RWRETURN(stream);
}

/*****************************************************************************
 _rpPatchAtomicWrite

 Patch atomic stream write.

 Inputs: stream - RwStream to write to.
         length - Length of patch data in stream.
         object - RpAtomic object we're streaming.
         offset - Offset of patch extension in atomic.
         size   - Size of patch externsion data.
 Outputs: RwStream * - The stream if successful.
 */
RwStream *
_rpPatchAtomicWrite( RwStream *stream,
                     RwInt32 length __RWUNUSED__,
                     const void *object,
                     RwInt32 offset __RWUNUSED__,
                     RwInt32 size __RWUNUSED__ )
{
    const RpAtomic *atomic;
    const PatchAtomicData *atomicData;

    RWFUNCTION(RWSTRING("_rpPatchAtomicWrite"));
    RWASSERT(NULL != stream);
    RWASSERT(NULL != object);

    atomic = (const RpAtomic *)object;
    RWASSERT(NULL != atomic);
    atomicData = PATCHATOMICGETCONSTDATA(atomic);
    RWASSERT(NULL != atomicData);

    /* Nothing to do, at the moment.  */

#ifdef EXTEND_rpPatchAtomicWrite
    EXTEND_rpPatchAtomicWrite;
#endif /* EXTEND_rpPatchAtomicWrite */

    RWRETURN(stream);
}

/*****************************************************************************
 _rpPatchAtomicAlways

 Patch atomic always callback. Setup the patch atomic's pipeline.

 Inputs: object - RpAtomic object to setup.
         offset - Offset of patch extension in atomic.
         size   - Size of patch externsion data.
 Outputs: RwBool - TRUE if pipeline was attached successfully.
 */
RwBool
_rpPatchAtomicAlways( void *object,
                      RwInt32 offset __RWUNUSED__,
                      RwInt32 size   __RWUNUSED__ )
{
    RpAtomic *atomic;
    RwBool result = TRUE;

    RWFUNCTION(RWSTRING("_rpPatchAtomicAlways"));
    RWASSERT(NULL != object);

    atomic = (RpAtomic *)object;
    RWASSERT(NULL != atomic);

    atomic = PatchAtomicSetup(atomic, rpNAPATCHTYPE);
    RWASSERT(NULL != atomic);

#ifdef EXTEND_rpPatchAtomicAlways
    EXTEND_rpPatchAtomicAlways;
#endif /* EXTEND_rpPatchAtomicAlways */

    RWRETURN(result);
}

/*****************************************************************************
 _rpPatchAtomicRights

 Patch atomic rights callback. Setup the patch atomic's pipeline, called if
 the atomic was streamout with the correct right plugin id.

 Inputs: object - RpAtomic object to setup.
         offset - Offset of patch extension in atomic.
         size   - Size of patch externsion data.
         extraData - Hint at the correct pipeline to attach.
 Outputs: RwBool - TRUE if pipeline was attached successfully.
 */
RwBool
_rpPatchAtomicRights( void *object,
                      RwInt32 offset __RWUNUSED__,
                      RwInt32 size   __RWUNUSED__,
                      RwUInt32 extraData )
{
    RpAtomic *atomic;
    RpPatchType type;
    RwBool result = TRUE;

    RWFUNCTION(RWSTRING("_rpPatchAtomicRights"));
    RWASSERT(NULL != object);

    atomic = (RpAtomic *)object;
    RWASSERT(NULL != atomic);
    type = (RpPatchType)extraData;

    atomic = PatchAtomicSetup(atomic, type);
    RWASSERT(NULL != atomic);

#ifdef EXTEND_rpPatchAtomicRights
    EXTEND_rpPatchAtomicRights;
#endif /* EXTEND_rpPatchAtomicRights */

    RWRETURN(result);
}
/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/

/**
 * \ingroup rppatch
 * \ref RpPatchAtomicSetPatchMesh
 * is used to attach the specified patch mesh to the given atomic. Setting the
 * patch mesh also sets the atomic's bounding sphere equal to the bounding
 * sphere of the patch mesh.
 *
 * If a patch mesh is already attached to the given atomic then that patch
 * mesh is destroyed (unless it is still used by another atomic) before the
 * new one is added.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param atomic    Pointer to the atomic.
 * \param patchMesh Pointer to the patch mesh.
 *
 * \return A pointer to the \ref RpAtomic if successful, or NULL otherwise.
 *
 * \see RpPatchAtomicGetPatchMesh
 * \see RpPatchMeshCreate
 * \see RpPatchMeshLock
 * \see RpPatchMeshUnlock
 */
RpAtomic *
RpPatchAtomicSetPatchMesh( RpAtomic *atomic,
                           RpPatchMesh *patchMesh )
{
    PatchMesh *mesh;
    PatchAtomicData *atomicData;
    RpGeometry *geometry;

    RWAPIFUNCTION(RWSTRING("RpPatchAtomicSetPatchMesh"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != atomic);

    atomicData = PATCHATOMICGETDATA(atomic);
    RWASSERT(NULL != atomicData);

    mesh = _rpPatchMeshGetInternal(patchMesh);

    if(mesh != atomicData->patchMesh)
    {
        if(NULL != mesh)
        {
            /* Add a ref to the patch mesh. */
            mesh = _rpPatchMeshAddRef(mesh);
        }

        if(NULL != atomicData->patchMesh)
        {
            /* Destroy the old patch mesh. */
            _rpPatchMeshDestroy(atomicData->patchMesh);
        }

        /* Store the mesh. */
        atomicData->patchMesh = mesh;

        /* Check the geometry. */
        geometry = _rpPatchMeshGetGeometry(mesh);
        RWASSERT(NULL != geometry);

        atomic = RpAtomicSetGeometry(atomic, geometry, 0);
        RWASSERT(NULL != atomic);

        /* Setup the pipeline. */

        /*
         * No longer do this after introducing RpPatchAtomicSetType.
         *
         * atomic = _rpPatchPipelinesAttach(atomic);
         * RWASSERT(NULL != atomic);
         */
    }

    RWRETURN(atomic);
}

/**
 * \ingroup rppatch
 * \ref RpPatchAtomicGetPatchMesh
 * is used to retrieve the patch mesh referenced by the specified atomic.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param atomic Pointer to the atomic containing the patch mesh.
 *
 * \return A pointer to the atomic's \ref RpPatchMesh if successful, or NULL
 * if there is an error or if no patch mesh is defined.
 *
 * \see RpPatchAtomicSetPatchMesh
 */
RpPatchMesh *
RpPatchAtomicGetPatchMesh( const RpAtomic *atomic )
{
    RpPatchMesh *patchMesh;
    const PatchAtomicData *atomicData;

    RWAPIFUNCTION(RWSTRING("RpPatchAtomicGetPatchMesh"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != atomic);

    atomicData = PATCHATOMICGETCONSTDATA(atomic);
    RWASSERT(NULL != atomicData);

    patchMesh = _rpPatchMeshGetExternal(atomicData->patchMesh);

    RWRETURN(patchMesh);
}

/**
 * \ingroup rppatch
 * \ref RpPatchAtomicSetType
 *
 * Sets up the patch \ref RpAtomic with the correct rendering pipeline.
 * The \ref RpPatchMesh should be attached to the \ref RpAtomic with
 * \ref RpPatchAtomicSetPatchMesh before setting up the \ref RpAtomic's
 * rendering pipeline.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param atomic Pointer to the atomic.
 * \param type   Atomic rendering type.
 *
 * \return A pointer to the \ref RpAtomic if successful, or NULL otherwise.
 *
 * \see RpPatchAtomicGetType
 * \see RpPatchAtomicSetPatchMesh
 */
RpAtomic *
RpPatchAtomicSetType( RpAtomic *atomic,
                      RpPatchType type )
{
    RWAPIFUNCTION(RWSTRING("RpPatchAtomicSetType"));

    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != atomic);

    /* The following assert has been removed whilst we support the older
     * AtomicAlways callback. Hence it's possible to stream in an atomic
     * and not know what pipe was previously attached, hence attaching
     * a pipe with rpNAPATCHTYPE is legal - but only for now!
     */
    /* RWASSERT(type >= rpPATCHTYPEGENERIC); */
    RWASSERT(type <= rpPATCHTYPESKINMATFX);

    atomic = _rpPatchPipelinesAttach(atomic, type);

    RWASSERT(NULL != atomic);

    RWRETURN(atomic);
}

/**
 * \ingroup rppatch
 * \ref RpPatchAtomicGetType
 *
 * Returns the \ref RpPatchType of the rendering pipeline attached to the atomic.
 *
 * The patch plugin must be attached before using this function.
 *
 * \param atomic Pointer to the atomic.
 *
 * \return The \ref RpPatchType of the pipeline attached to the atomic,
 * or \ref rpNAPATCHTYPE otherwise.
 *
 * \see RpPatchAtomicSetType
 */
RpPatchType
RpPatchAtomicGetType( const RpAtomic *atomic )
{
    RxPipeline *pipeline;
    RpPatchType type;

    RWAPIFUNCTION(RWSTRING("RpPatchAtomicGetType"));
    RWASSERT(0 < _rpPatchGlobals.module.numInstances);
    RWASSERT(NULL != atomic);

    RpAtomicGetPipeline(atomic, &pipeline);
    RWASSERT(NULL != pipeline);

    type = (pipeline->pluginId == rwID_PATCHPLUGIN)
         ? (RpPatchType)(pipeline->pluginData)
         : rpNAPATCHTYPE;

    RWRETURN(type);
}
