/*****************************************************************************

    File: dmorph.h

    Purpose: A short description of the file.

    Copyright (c) 2002 Criterion Software Ltd.

 */

#ifndef DMORPH_H
#define DMORPH_H

/* RWPUBLIC */
/**
 * \defgroup rpdmorph RpDMorph
 * \ingroup deltamorphing
 *
 * Delta Morphing Plugin for RenderWare Graphics.
 */
/* RWPUBLICEND */

/*****************************************************************************
 Includes
 */
#include <rwcore.h>
#include <rpworld.h>

#include "dmphgeom.h"
#include "dmphintp.h"

/*****************************************************************************
 Defines
 */

/*****************************************************************************
 Enums
 */

/*****************************************************************************
 Typedef Enums
 */

/*****************************************************************************
 Typedef Structs
 */

/*****************************************************************************
 Function Pointers
 */

/*****************************************************************************
 Structs
 */
/* RWPUBLIC */
#ifdef     __cplusplus
extern "C"
{
#endif  /* __cplusplus */
/* RWPUBLICEND */

/*****************************************************************************
 Global Variables
 */
extern RwModuleInfo rpDMorphModule;

/*****************************************************************************
 Function prototypes
 */
#if defined(PLATFORM_PIPELINES)
extern RwBool _rpDMorphPipelinesCreate(RwUInt32 type);
extern RwBool _rpDMorphPipelinesDestroy(void);
extern RpAtomic *_rpDMorphPipelinesAttach(RpAtomic *atomic,
                                          RpDMorphType type);
#endif /* defined(PLATFORM_PIPELINES) */

#if defined(PLATFORM_STREAM)
extern RwInt32 _rwDMorphGeometryNativeStreamSize(const RpGeometry *geometry);
#endif /* defined(PLATFORM_STREAM) */

#if defined(PLATFORM_GEOM_DTOR)
extern void
_rpDMorphGeometryDtor(rpDMorphGeometryData  *geometryData);
#endif /* defined(PLATFORM_GEOM_DTOR) */

/* RWPUBLIC */
extern RwBool RpDMorphPluginAttach( void );

#ifdef    __cplusplus
}
#endif /* __cplusplus */
/* RWPUBLICEND */

#endif /* DMORPH_H */
