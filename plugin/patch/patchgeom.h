#ifndef PATCHGEOM_H
#define PATCHGEOM_H

#include <rpworld.h>

/* A vertex/normals modification function */
/* atomic is the atomic passed to _rpPatchToGeometry()
   verts points to the output array of vertices
   norms points to the output array of normals
   
   pass a null array if there are no normals. */

typedef RpAtomic*
(*modificationFn)(RpAtomic *atomic, void *vertices, void *normals);

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* Null should be passed if no modificationFn is required */

extern RpGeometry*
_rpPatchToGeometry(RpAtomic *atomic, modificationFn func);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PATCHGEOM_H */
