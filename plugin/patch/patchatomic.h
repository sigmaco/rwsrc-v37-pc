#ifndef PATCHATOMIC_H
#define PATCHATOMIC_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rppatch.h"

/* This must be included after rppatch.h */
#if ( defined(GCN_DRVMODEL_H)     ||    \
      defined(NULLGCN_DRVMODEL_H) ||    \
      defined(OPENGL_DRVMODEL_H) )
#include "patchplatform.h"
#endif /* ( defined(GCN_DRVMODEL_H) || defined(OPENGL_DRVMODEL_H) ) */

#include "patchlod.h"
#include "patchmesh.h"

/*
 * patchplatform.h may provide some defines to extend the plugin function:
 *
 * EXTEND_PatchAtomicData    A block of elements added to the plugin data
 *
 * All the additional code blocks execute after the main function's body.
 *
 * EXTEND_rpPatchAtomicConstructor  Code to extend the constructor.
 *                                  It should set object=NULL on error
 *
 * EXTEND_rpPatchAtomicDestructor   Code to extend the destructor.
 *                                  It should set object=NULL on error
 *
 * EXTEND_rpPatchAtomicCopy         Code to extend the copy constructor.
 *                                  It should set dstObject=NULL on error
 *
 * EXTEND_rpPatchAtomicGetSize      Expression returning an RwInt32 equal to
 *                                  the number of additional bytes required
 *
 * EXTEND_rpPatchAtomicRead         Code to extend the stream read.
 *                                  It should set stream=NULL on error
 *
 * EXTEND_rpPatchAtomicWrite        Code to extend the stream write.
 *                                  It should set stream=NULL on error
 *
 * EXTEND_rpPatchAtomicAlways       Code to extend the always callback.
 *                                  It should set result=FALSE on error
 *
 * EXTEND_rpPatchAtomicRights       Code to extend the rights callback.
 *                                  It should set result=FALSE on error
 */

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/
typedef struct PatchAtomicData PatchAtomicData;
struct PatchAtomicData
{
    PatchAtomicLod lod;
    PatchMesh *patchMesh;

#ifdef EXTEND_PatchAtomicData
    EXTEND_PatchAtomicData;
#endif

};

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern void *
_rpPatchAtomicConstructor( void *object,
                           RwInt32 offset,
                           RwInt32 size );
extern void *
_rpPatchAtomicDestructor( void *object,
                          RwInt32 offset,
                          RwInt32 size );
extern void *
_rpPatchAtomicCopy( void *dstObject,
                    const void *srcObject,
                    RwInt32 offset,
                    RwInt32 size );
extern RwInt32
_rpPatchAtomicGetSize( const void *atomic,
                       RwInt32 offset,
                       RwInt32 size );
extern RwStream *
_rpPatchAtomicRead( RwStream *stream,
                    RwInt32 length,
                    void *atomic,
                    RwInt32 offset,
                    RwInt32 size );

extern RwStream *
_rpPatchAtomicWrite( RwStream *stream,
                     RwInt32 length,
                     const void *atomic,
                     RwInt32 offset,
                     RwInt32 size );

extern RwBool
_rpPatchAtomicAlways( void *object,
                      RwInt32 offset,
                      RwInt32 size );

extern RwBool
_rpPatchAtomicRights( void *object,
                      RwInt32 offset,
                      RwInt32 size,
                      RwUInt32 extraData);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PATCHATOMIC_H */
