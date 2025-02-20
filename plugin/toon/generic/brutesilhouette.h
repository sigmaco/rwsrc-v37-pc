/*****************************************************************************

    File: brutesilhouette.h

    Purpose: A short description of the file.

    Copyright (c) 2002 Criterion Software Ltd.

 */

#ifndef BRUTESILHOUETTE_H
#define BRUTESILHOUETTE_H

/*****************************************************************************
 Includes
 */

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

#ifdef     __cplusplus
extern "C"
{
#endif  /* __cplusplus */

/*****************************************************************************
 Global Variables
 */

/*****************************************************************************
 Function prototypes
 */
extern void _rpToonBruteSilhouetteInit(void);
extern void _rpToonBruteSilhouetteClean(void);
extern void _rpToonBruteSilhouetteRender(RpToonGeo *toonGeo,
                                         const RwV3d *verts,
                                         const RwV3d *normals,
                                         RwBool faceNormalsInvalid,
                                         const RwMatrix *transform);

#ifdef    __cplusplus
}
#endif /* __cplusplus */

#endif /* BRUTESILHOUETTE_H */
