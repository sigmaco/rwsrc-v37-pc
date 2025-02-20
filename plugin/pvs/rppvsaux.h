/*
 * Potentially Visible Set plug-in
 */

/**********************************************************************
 *
 * file :     rpPVSaux.h
 *
 * abstract : handle culling of worldsectors in RenderWare
 *
 **********************************************************************
 *
 * This file is a product of Criterion Software Ltd.
 *
 * This file is provided as is with no warranties of any kind and is
 * provided without any obligation on Criterion Software Ltd. or
 * Canon Inc. to assist in its use or modification.
 *
 * Criterion Software Ltd. will not, under any
 * circumstances, be liable for any lost revenue or other damages arising
 * from the use of this file.
 *
 * Copyright (c) 2001 Criterion Software Ltd.
 * All Rights Reserved.
 *
 * RenderWare is a trademark of Canon Inc.
 *
 ************************************************************************/

#include "rwcore.h"
#include "rpworld.h"

extern void                _rpPVSDestroyPoly(_rpPVSPolyPtr * P);
extern void                _rpPVSdecapitate(_rpPVSPolyListPtr * list);
extern RwInt32             _rpPVSLinkCardinality(_rpPVSPolyListPtr list);
extern RwInt32             _rpPVSLinkCardinality2(_rpPVSPolyPtr list);

extern void                _rpPVSaddendII(_rpPVSPolyListPtr * end, _rpPVSPolyRecord * t);
extern _rpPVSPolyListPtr         _rpPVScreateNode(_rpPVSPolyRecord * t);
extern void                _rpPVSaddendIIp(_rpPVSPolyPtr * end, RwV3d * t);
extern void                _rpPVSDestroyPolyList(_rpPVSPolyListPtr * P);
extern RwInt32             _rpPVSWorldGetNumWorldSectors(RpWorld * world);

#define rpPVSSECTORCULL TRUE        /* Tests whole sectors, and avoids testing many individual polygons */
#define rpPVSTRANSLUCENCY TRUE      /* Treats translucent polygons as a special case: can be occluded, but not occluder */
#define rpPVSDETESS TRUE        /* Detesselates triangles to improve the preprocessing speed */
#define rpPVSDECOLINEAR TRUE        /* Removes slivers and colinear vertices to improve speed and accuracy */
#define rpPVSJUMP TRUE              /* Skips sectors already determined as visible or in a trivial sector: see rpPVSSECTORCULL */
#define rpPVSORTHOGONALDETESSONLY FALSE

#define rpPVSABS(x) ((x)<0.0f ? -(x) : (x)) /* Return absolute value */
#define rpPVSTHOU 0.001f
#define rpPVSSMALL 0.000001f           /* A small value to represent geometric zero */
#define rpPVSTINY 0.000001f         /* A very small value to represent computational zero */
#define rpPVSPNZ(x) ((x)<-rpPVSSMALL ? -1 : ((x)>rpPVSSMALL ? 1 : 0)) /* Checks the sign of the value,
                                                             returns Pos (1), Neg (-1), Zero (0) */
#define rpPVSMIN(x,y) ((x)<(y) ? (x) : (y))
#define rpPVSMAX(x,y) ((x)>(y) ? (x) : (y))

#define rpPVSSECTORCULLTOL 8

#define PVSaddendSupremeMacro(_head, _tail, _cell)      \
MACRO_START                                             \
{                                                       \
    if (_tail==(_rpPVSPolyListPtr)NULL)                       \
    {                                                   \
        _tail=_cell;                                    \
        _cell->next = (_rpPVSPolyListPtr)NULL;                \
        _head=_tail;                                    \
    }                                                   \
    else                                                \
    {                                                   \
        _tail->next=_cell;                              \
        _cell->next = (_rpPVSPolyListPtr)NULL;                \
        _tail=_tail->next;                              \
    }                                                   \
}                                                       \
MACRO_STOP

#define PVSFindScalarMacro(_px, _py, _pz, _Px, _Py, _Pz, _Pw, _scalar) \
MACRO_START                                                             \
{                                                                       \
    _scalar = _px * _Px + _py * _Py + _pz * _Pz + _Pw;                  \
}                                                                       \
MACRO_STOP


