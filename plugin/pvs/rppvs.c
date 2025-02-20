/*
 * Potentially Visible Set plug-in
 */

/**********************************************************************
 *
 * file :     rppvs.c
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

/**
 * \ingroup rppvs
 * \page rppvsoverview RpPVS Plugin Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rpcollis.h, rppvs.h
 * \li \b Libraries: rwcore, rpworld, rtintsec, rpcollis, rppvs
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach,  \ref RpCollisionPluginAttach
 *     \ref RpPVSPluginAttach
 *
 * \subsection pvsoverview Overview
 * The RpPVS plugin extends the RenderWare Graphics API to support optimization of
 * BSP world rendering using a custom Potentially Visible Sets algorithm. Under
 * the right conditions, this algorithm can dramatically reduce the time taken
 * to render a large 3D model, such as a level for a game.
 *
 * This plugin supports both ends of the PVS process: you can use it to create
 * PVS data as well as to accelerate rendering using the results.
 *
 * Potentially Visible Sets processing is usually applied to large 3D models, such
 * as one of a large building, where only a small portion is ever expected to be
 * visible at any one time. In use, the application developer tells the plugin to
 * check a number of points within an RpWorld and determine which RpWorldSector
 * objects are visible from each location. When this process is completed, the
 * rendering engine can easily determine which RpWorldSector objects are to be
 * rendered at any location.
 *
 * The format of PVS data has changed since release 3.10.  SDK-tools pvscnvrt can
 * be used to convert from old-style to new-style PVS data.  It can also be used
 * to delete, generate and enhance PVS data.
 */

/*--- Include files ---*/

#include "string.h"
#include "rwcore.h"
#include "rpworld.h"
#include "rpcollis.h"
#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rppvs.h"
#include "rppvsaux.h"

/****************************************************************************
 Global Vars
 */
#define visual 0

#if (defined(RWDEBUG))
long                rpPVSStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

RpPVSGlobalVars     rpPVSGlobals;

#define EQ2(_a, _b) ((((_a) - (_b)) < 0.01) && (((_a) - (_b)) > -0.01))

#define EQ(_a, _b) ((((_a) - (_b)) < rpPVSTINY) && (((_a) - (_b)) > -rpPVSTINY))

#define PVSVerticesEqual(_a, _b)                                        \
     (((((_a).x - (_b).x) < rpPVSSMALL) && (((_a).x - (_b).x) > -rpPVSSMALL)) &&  \
      ((((_a).y - (_b).y) < rpPVSSMALL) && (((_a).y - (_b).y) > -rpPVSSMALL)) &&  \
      ((((_a).z - (_b).z) < rpPVSSMALL) && (((_a).z - (_b).z) > -rpPVSSMALL)))

#define PVSIsBackFace(Cand, ViewPoint)                                  \
    ((RwV3dDotProduct((RwV3d *)&(Cand)->plane, ViewPoint) + (Cand)->plane.w) <= rpPVSSMALL)

#define PVSDistanceBetweenPointsSquared(_a, _b)     \
        ((((_a).x - (_b).x) * ((_a).x - (_b).x)) +  \
        (((_a).y - (_b).y) * ((_a).y - (_b).y)) +   \
        (((_a).z - (_b).z) * ((_a).z - (_b).z)))



#undef PVSMERGEONLYORTHOGON
#if (defined(rpPVSORTHOGONALDETESSONLY))
#if (rpPVSORTHOGONALDETESSONLY != FALSE)
#define PVSMERGEONLYORTHOGON
#endif /* (rpPVSORTHOGONALDETESSONLY != FALSE) */
#endif /* (defined(rpPVSORTHOGONALDETESSONLY)) */



static RwReal
PVSAreaOfTriangle(RwV3d * v1, RwV3d * v2, RwV3d * v3)
{
    /* pre-preprocess only */
    RwV3d av1, av2, av3;
    RwReal a,b,c,s;
    RwReal x;

    RWFUNCTION(RWSTRING("PVSAreaOfTriangle"));

    RwV3dSub(&av1, v1, v2);
    RwV3dSub(&av2, v2, v3);
    RwV3dSub(&av3, v3, v1);
    a = RwV3dLength(&av1);
    b = RwV3dLength(&av2);
    c = RwV3dLength(&av3);
    s = (a + b + c) / 2.0f;

    x = (s)*(s-a)*(s-b)*(s-c);
    if (x>0)
    {
        RwReal res;

        rwSqrt(&res, x);
        RWRETURN(res);
    }
    else RWRETURN(0.0f);
}

static RwReal
PVSAreaOfPolygon(_rpPVSPolyPtr poly)
{
    /* pre-preprocess only */
    RwReal eval = 0.0f;
    _rpPVSPolyPtr base, pointA, pointB;

    RWFUNCTION(RWSTRING("PVSAreaOfPolygon"));

    base = poly;
    pointA = poly->next;
    pointB = poly->next->next;

    while (pointB!=NULL)
    {
        eval +=(PVSAreaOfTriangle(&base->v, &pointA->v, &pointB->v));
        pointA = pointB;
        pointB = pointB->next;
    }
    RWRETURN(eval);
}

/*
 * PVS World sector replacement callback.
 */
static RpWorldSector *
PVSWorldSectorRenderCallBack(RpWorldSector *spSector)
{
    RpPVSCache          *pvsCache;
    RpWorldSector       *result;

    RWFUNCTION(RWSTRING("PVSWorldSectorRenderCallBack"));

    pvsCache = PVSCACHEFROMWORLD(rpPVSGlobals.World);

    result = spSector;

    /* Check if PVS is enabled. */
    if (pvsCache->hooked)
    {
        /* PVS on. Check if the is sector visible. */
        if (RpPVSWorldSectorVisible(spSector))
        {
            /* Sector is visible. Render the sector. */
            result = (pvsCache->renderCallBack)(spSector);
            pvsCache->ptotal += RpWorldSectorGetNumTriangles(spSector);
            pvsCache->paccept += RpWorldSectorGetNumTriangles(spSector);
        }
        else
        {
            /* Sector is not visible. */
            result = spSector;
            pvsCache->ptotal += RpWorldSectorGetNumTriangles(spSector);
        }
    }
    else
    {
        /* PVS off. Render the sector. */
        result = (pvsCache->renderCallBack)(spSector);
    }

    RWRETURN(result);
}

static void
PVSAssignPolygonAttrs(_rpPVSPolyRecord * mypoly, _rpPVSPolyPtr geom, _rpPVSPlaneEq plane,
                      RwReal priority, RwBool original, RwInt32 home,
                      _rpPVSPolyListPtr parent, RwBool translucent,
                      RpWorldSector * homeaddr, RwBool done,
                      RwV3d centroid, RwReal radius, RwV3d extreme)
{
    RWFUNCTION(RWSTRING("PVSAssignPolygonAttrs"));

    /* Description */
    mypoly->geom = geom;
    mypoly->translucent = translucent;

    /* Precalcs */
    mypoly->plane = plane;
    mypoly->centroid = centroid;
    mypoly->radius = radius;
    mypoly->extreme = extreme;

    /* For sorting */
    mypoly->priority = priority;

    /* For splitting */
    mypoly->original = original;
    mypoly->parent = parent;

    /* Sector info */
    mypoly->home = home;
    mypoly->homeaddr = homeaddr;

    /* Algorithm info */
    mypoly->hasbeenclipper = done;

    RWRETURNVOID();
}

static void
PVSGeneratePlaneEquationFromThreePoints(_rpPVSPlaneEq * result,
                                        RwV3d * a,
                                        RwV3d * b, RwV3d * c)
{
    RwV3d               v1, v2, normal;
    RwReal              D;
    RWFUNCTION(RWSTRING("PVSGeneratePlaneEquationFromThreePoints"));

    RWASSERT((_rpPVSPlaneEq *)NULL !=  result);

    RwV3dSub(&v1, b, a);
    RwV3dSub(&v2, c, b);
    RwV3dCrossProduct(&normal, &v1, &v2);

    result->x = normal.x;
    result->y = normal.y;
    result->z = normal.z;

    D = -RwV3dDotProduct(&normal, a);
    result->w = D;
    result->l = 1.0f / RwV3dLength(&normal);

    RWRETURNVOID();
}

static void
PVSRemoveBackFaces(_rpPVSPolyListPtr * Tlist, RwV3d * ViewPoint)
{
    _rpPVSPolyListPtr         temp = *Tlist;
    _rpPVSPolyListPtr         post = (_rpPVSPolyList *) NULL;
    _rpPVSPolyListPtr         prev = (_rpPVSPolyList *) NULL;
    _rpPVSPolyListPtr         todel;
    RpPVS              *pvsCur;

    RWFUNCTION(RWSTRING("PVSRemoveBackFaces"));

    while (temp != NULL)
    {
        post = temp->next;
        if (PVSIsBackFace(&(temp->data), ViewPoint))
        {
            /* Do not destroy polygon,
             * because it is referenced by
             * the root ("rpPVSGlobals.polygons") list */
            /* extract temp from Tlist */
            todel = temp;

            pvsCur = PVSFROMWORLDSECTOR(temp->data.homeaddr);

            if (prev != NULL)
                prev->next = temp->next;
            else
                *Tlist = temp->next;
            RwFree(todel);
            todel = (_rpPVSPolyList *) NULL;
            /* prev=prev */
        }
        else
        {
            prev = temp;
        }
        temp = post;
    }
    RWRETURNVOID();
}

static RwBool
PVSPointOnPlane2(RwV3d * Cand, _rpPVSPlaneEq * Root, RwReal Zero)
{
    RwReal              scalar;

    RWFUNCTION(RWSTRING("PVSPointOnPlane2"));

    scalar =
        (RwV3dDotProduct((RwV3d *) Root, Cand) + Root->w) * Root->l;

    if (scalar <= Zero && scalar >= -Zero)
        RWRETURN(TRUE);
    else
        RWRETURN(FALSE);
}

static _rpPVSPartitionId
PVSPointWRTPlane2(RwV3d * Cand, _rpPVSPlaneEq * Root, RwReal Zero)
{
    RwReal              scalar;

    RWFUNCTION(RWSTRING("PVSPointWRTPlane2"));

    scalar =
        (RwV3dDotProduct((RwV3d *) Root, Cand) + Root->w) * Root->l;

    if (scalar >= Zero)
        RWRETURN(rpPVSFRONT);
    if (scalar <= -Zero)
        RWRETURN(rpPVSBACK);

    RWRETURN(rpPVSSPLIT);
}

#define PVSSphereWRTPlaneMACRO(_centre, _radius, _plane)                    \
MACRO_START                                                                 \
{                                                                           \
    RwReal scalar;                                                          \
                                                                            \
    PVSFindScalarMacro( (_centre.x), (_centre.y), (_centre.z),              \
                        (_plane.x), (_plane.y), (_plane.z), (_plane.w),     \
                        (scalar));                                          \
    scalar*=_plane.l;                                                       \
                                                                            \
    if (scalar>=_radius) _plane.lastresult=rpPVSFRONT;                           \
    else if (scalar<=-_radius) _plane.lastresult=rpPVSBACK;                      \
    else _plane.lastresult=rpPVSSPLIT;                                           \
}                                                                           \
MACRO_STOP

#define PVSPolygonWRTPlaneMACRO(_polygon, _plane, _result, _zero)                  \
MACRO_START                                                                 \
{                                                                           \
    RwReal scalar;                                                          \
    _rpPVSPolyPtr temp;                                                           \
    RwBool isonback=FALSE, isonfront=FALSE;                                 \
                                                                            \
    temp=_polygon;                                                          \
    while (temp!=NULL)                                                      \
    {                                                                       \
        PVSFindScalarMacro( (temp->v.x), (temp->v.y), (temp->v.z),          \
                            (_plane.x), (_plane.y), (_plane.z), (_plane.w), \
                            (scalar));                                      \
        scalar*=_plane.l;                                                   \
                                                                            \
        if (scalar>(_zero)) isonfront=TRUE;                                   \
        else if (scalar<-(_zero)) isonback=TRUE;                              \
                                                                            \
                                                                            \
        if ((isonback) && (isonfront)) break;                               \
                                                                            \
        temp=temp->next;                                                    \
    }                                                                       \
    if (isonfront && isonback) _result=rpPVSSPLIT;                               \
    else if (isonback) _result=rpPVSBACK;                                        \
    else if (isonfront) _result=rpPVSFRONT;                                      \
    else _result=rpPVSCOPLANAR;                                                  \
}                                                                           \
MACRO_STOP

static _rpPVSPartitionId
PVSClassifyPolygonWRTPlane(_rpPVSPolyRecord * t, _rpPVSPlaneEq * ClipPlane, RwReal zero)
{
    /* Returns back if polygon is behind, on or very close; split; or front */

    /* biggest time waster - make more efficient */
    RwBool              isonback = FALSE, isonfront = FALSE;
    _rpPVSPolyPtr             temp;
    _rpPVSPlaneEq             Clip = *ClipPlane;
    RwReal              scalar;
    RwV3d               vert;

    RWFUNCTION(RWSTRING("PVSClassifyPolygonWRTPlane"));

    /* check and assign the signed distance
     * of each vertex to the clipping plane... */
    temp = t->geom;
    while (temp != NULL)
    {
        vert = temp->v;

        PVSFindScalarMacro((vert.x), (vert.y), (vert.z), (Clip.x),
                           (Clip.y), (Clip.z), (Clip.w), (scalar));

        scalar *= Clip.l;

        temp->scalar = scalar;
        temp->pscalar = rpPVSPNZ(scalar);

        if (scalar > zero)
            isonfront = TRUE;
        else if (scalar < -zero)
            isonback = TRUE;

        /* Assign values for subsequent clipping */
        temp = temp->next;
    }

    if ((isonfront) && (isonback))
    {
        RWRETURN(rpPVSSPLIT);
    }
    else if (isonback)
    {
        RWRETURN(rpPVSBACK);
    }
    else if (isonfront)
    {
        RWRETURN(rpPVSFRONT);
    }

    RWRETURN(rpPVSCOPLANAR);
}

static void
PVSFindCentroidandRadius(_rpPVSPolyPtr * t, RwV3d * centroid,
                         RwReal * radius, RwV3d * extreme)
{
    _rpPVSPolyPtr             temp;
    RwInt32             Count = 0;
    RwReal              lensq = 0.0f;

    RWFUNCTION(RWSTRING("PVSFindCentroidandRadius"));

    *radius = 0.0f;
    centroid->x = 0.0f;
    centroid->y = 0.0f;
    centroid->z = 0.0f;

    temp = *t;
    while (temp != NULL)
    {
        centroid->x += temp->v.x;
        centroid->y += temp->v.y;
        centroid->z += temp->v.z;
        Count++;

        temp = temp->next;
    }
    centroid->x /= Count;
    centroid->y /= Count;
    centroid->z /= Count;

    temp = *t;
    while (temp != NULL)
    {
        lensq = (temp->v.x - centroid->x) * (temp->v.x - centroid->x) +
            (temp->v.y - centroid->y) * (temp->v.y - centroid->y) +
            (temp->v.z - centroid->z) * (temp->v.z - centroid->z);

        if (lensq > *radius)
        {
            *radius = lensq;
            *extreme = temp->v;
        }
        temp = temp->next;
    }
    rwSqrt(radius, *radius);

    RWRETURNVOID();
}

static void
PVSGenerateShadowCone(_rpPVSPolyRecord * poly)
{
    /* find cone radius... */
    /* line of sight */
    RwV3d L1, L2;
    RwReal val = 1.0f, coneRadius = 1.0f, length1, length2;
    _rpPVSPolyPtr             ptemp;
    RwV3d viewPoint = rpPVSGlobals.ViewPos;

    RWFUNCTION(RWSTRING("PVSGenerateShadowCone"));


    RwV3dSub(&L1, &poly->centroid, &viewPoint);
    length1 = RwV3dLength(&L1);



    /* along surface of cone from apex */
    ptemp = poly->geom;
    while (ptemp!=NULL)
    {


        RwV3dSub(&L2, &ptemp->v, &viewPoint);
        length2 = RwV3dLength(&L2);

        val = ((RwV3dDotProduct(&L1, &L2)) / (length1 * length2));
        if (val < coneRadius) coneRadius = val;

        ptemp = ptemp -> next;
    }
    poly->coneRadius = (RwReal)(RwACos(rpPVSMIN(coneRadius, 1.0f)));

    RWRETURNVOID();
}
static _rpPVSPartitionId
PVSDivide(_rpPVSPolyRecord * t, _rpPVSPlaneEq ClipPlane __RWUNUSED__,
          _rpPVSPolyRecord * inPart, _rpPVSPolyRecord * outPart)
{
    RwInt32             i, s, fill;
    _rpPVSPolyPtr             end = (_rpPVSPoly *) NULL;
    _rpPVSPolyPtr             inend = (_rpPVSPoly *) NULL;
    _rpPVSPolyPtr             outend = (_rpPVSPoly *) NULL;
    _rpPVSPolyPtr             traverse;
    _rpPVSPolyPtr             tnxt;
    _rpPVSPolyPtr             temp;
    _rpPVSPolyPtr             previn;
    _rpPVSPolyPtr             prevout;
    _rpPVSPolyPtr             superPoly = (_rpPVSPoly *) NULL;
    RwV3d               intersection;
    _rpPVSPolyPtr             in = (_rpPVSPoly *) NULL;
    _rpPVSPolyPtr             out = (_rpPVSPoly *) NULL;
    RwInt32             invct = 0, outvct = 0;
    RwReal              radius;
    RwV3d               centroid, extreme;

    RWFUNCTION(RWSTRING("PVSDivide"));
    /* polygon needs to be divided since
     * some parts are on the front and
     * some on the back of the clipper */

    /* firstly, create one polygon of points that is
     * the union of the subsequent rpPVSGlobals.polygons
     * with colicloses verts intact... */
    traverse = t->geom;
    superPoly = (_rpPVSPoly *) NULL;
    i = 0;
    s = 0;

    /* memory TIME WASTER */
    while (traverse != NULL)
    {
        /* assign first vertex from clippee polygon to superPoly... */
        _rpPVSaddendIIp(&end, &(traverse->v));
        if (superPoly == NULL)
            superPoly = end;

        /* assign scalar to pscalar array... */
        end->scalar = traverse->scalar;
        end->pscalar = traverse->pscalar;

        i++;

        if (traverse->next != NULL)
            tnxt = traverse->next;
        else
            tnxt = t->geom;

        if (end->pscalar * tnxt->pscalar < -rpPVSSMALL * rpPVSSMALL)
        {
            /* Before we reach the next vertex, we have a split */
            RwReal              tv =
                (rpPVSABS(tnxt->scalar)) /
                ((rpPVSABS(traverse->scalar) + rpPVSABS(tnxt->scalar)));

            intersection.x =
                ((traverse->v.x - tnxt->v.x) * tv) + tnxt->v.x;
            intersection.y =
                ((traverse->v.y - tnxt->v.y) * tv) + tnxt->v.y;
            intersection.z =
                ((traverse->v.z - tnxt->v.z) * tv) + tnxt->v.z;

            /* assign intersection to next superPoly point... */
            _rpPVSaddendIIp(&end, &(intersection));

            end->scalar = 0.0f;
            end->pscalar = 0;

            i++;
        }
        s++;

        traverse = traverse->next;
    }

    /* we now have to put parts of super polygon into two next parts... */

    fill = 0;
    previn = (_rpPVSPoly *) NULL;
    prevout = (_rpPVSPoly *) NULL;
    temp = superPoly;
    while (temp != NULL)
    {
        if (temp->pscalar <= 0)
        {
            _rpPVSaddendIIp(&inend, &(temp->v));
            inend->pscalar = temp->pscalar;
            inend->scalar = temp->scalar;

            if (in == NULL)
                in = inend;
            invct++;
            previn = temp;
        }
        if (temp->pscalar >= 0)
        {
            _rpPVSaddendIIp(&outend, &(temp->v));
            outend->pscalar = temp->pscalar;
            outend->scalar = temp->scalar;

            if (out == NULL)
                out = outend;
            outvct++;
            prevout = temp;
        }
        temp = temp->next;
        fill++;
    }

    PVSFindCentroidandRadius(&in, &centroid, &radius, &extreme);
    if (invct >= 3)
    {
        PVSAssignPolygonAttrs(inPart, in, t->plane, t->priority, FALSE,
                              t->home, t->parent, t->translucent,
                              t->homeaddr, t->hasbeenclipper, centroid,
                              radius, extreme);
    }


    PVSFindCentroidandRadius(&out, &centroid, &radius, &extreme);
    if (outvct >= 3)
    {
        PVSAssignPolygonAttrs(outPart, out, t->plane, t->priority,
                              FALSE, t->home, t->parent,
                              t->translucent, t->homeaddr,
                              t->hasbeenclipper, centroid, radius,
                              extreme);
    }



    _rpPVSDestroyPoly(&superPoly);

    RWASSERT(invct >= 3 && outvct >= 3);

    RWRETURN(rpPVSSPLIT);
}

static RwBool
PVSPolygonTrivByFace(_rpPVSPolyListPtr * t, _rpPVSPolyRecord * Clipper)
{

    _rpPVSPolyRecord * poly = &((*t)->data);

    RWFUNCTION(RWSTRING("PVSPolygonTrivByFace"));

    /* Check if polygon (bounding sphere) is on front of clipper - and thus by definition
     * cannot be shadowed */
    PVSSphereWRTPlaneMACRO(poly->centroid, poly->radius,
                           Clipper->plane);

    if (Clipper->plane.lastresult == rpPVSFRONT) RWRETURN(TRUE); /* completely on front - visible */


    /* Check if clipper (bounding sphere) is on back of polygon - and thus by definition
     * cannot be shadowed */
    PVSSphereWRTPlaneMACRO((Clipper->centroid), (Clipper->radius),
                           poly->parent->data.plane);

    if (poly->parent->data.plane.lastresult == rpPVSBACK) RWRETURN(TRUE); /* completely on front - visible */

    RWRETURN(FALSE);
}

static RwBool
PVSPolygonTrivByCone(_rpPVSPolyListPtr * t, _rpPVSPolyRecord * Clipper)
{
    RwReal intersectRadius;
    RwV3d L1, L2;
    RwReal length2, length1;
    RwReal diff;
    _rpPVSPolyRecord * poly = &((*t)->data);
    RwV3d viewPoint = rpPVSGlobals.ViewPos;
    RwReal coneRadius = Clipper->coneRadius;

    RWFUNCTION(RWSTRING("PVSPolygonTrivByCone"));

    /* line of sight */
    RwV3dSub(&L1, &Clipper->centroid, &viewPoint);
    length1 = RwV3dLength(&L1);

    /* find poly centroid radius... */
    RwV3dSub(&L2, &poly->centroid, &viewPoint);
    length2 = RwV3dLength(&L2);

    intersectRadius = ((RwV3dDotProduct(&L1, &L2)) / (length1 * length2)); /* based on centre */
    intersectRadius = (RwReal)(RwACos(rpPVSMIN(intersectRadius, 1.0f)));

    diff = (RwReal)(RwASin(poly->radius / length2));
    if (intersectRadius < diff)
    {
        RWRETURN(FALSE);
    }
    intersectRadius -= diff;

    if (intersectRadius >= coneRadius) RWRETURN(TRUE); /* out - keep */

    RWRETURN(FALSE);
}

static RwBool
PVSPolygonTrivByVolume(_rpPVSPolyListPtr * t, _rpPVSPolyRecord * Clipper)
{
    /* time waster - make more efficient */
    _rpPVSPartitionId         Slice;
    _rpPVSPolyPtr             shadowsPtr;
    RwBool              Split = TRUE;

    RWFUNCTION(RWSTRING("PVSPolygonTrivByVolume"));

    /* See if polygon's bounding sphere does not intersect clipper's planes... */
    Split = FALSE;
    shadowsPtr = Clipper->geom;
    while (shadowsPtr != NULL)
    {
        PVSSphereWRTPlaneMACRO(((*t)->data.centroid),
                               ((*t)->data.radius),
                               (shadowsPtr->shadowPlane));
        Slice = shadowsPtr->shadowPlane.lastresult;

        if (Slice == rpPVSFRONT)
            RWRETURN(TRUE); /* completely on front - visible */
        else if (Slice == rpPVSSPLIT)
            Split = TRUE;
        shadowsPtr = shadowsPtr->next;
    }

    if (!Split)  /* completely on back - invisible */
    {
        if (!(*t)->data.original)
            _rpPVSDestroyPoly(&((*t)->data.geom)); /* memory time waster */
        RwFree(*t);
        *t = (_rpPVSPolyList *) NULL;

        RWRETURN(TRUE);
    }
    /* undetermined - let's use the actual polygon, not its bounding sphere... */



    Split = FALSE;

    /* True clip against shadow face... */
    if (Clipper->plane.lastresult != rpPVSSPLIT)
        Slice = Clipper->plane.lastresult;
    else
        PVSPolygonWRTPlaneMACRO(((*t)->data.geom),
                                (Clipper->plane), (Slice), rpPVSSMALL);

    if (Slice == rpPVSFRONT)
        RWRETURN(TRUE); /* completely on front - visible */

    /* True clip against shadow edges... */
    shadowsPtr = Clipper->geom;
    while (shadowsPtr != NULL)
    {
        if (shadowsPtr->shadowPlane.lastresult != rpPVSSPLIT)
        {
            Slice = shadowsPtr->shadowPlane.lastresult;
        }
        else
            PVSPolygonWRTPlaneMACRO(((*t)->data.geom),
                                    (shadowsPtr->shadowPlane),
                                    (Slice), rpPVSSMALL);

        if (Slice == rpPVSFRONT)
            RWRETURN(TRUE);/* completely on front - visible */
        else if (Slice == rpPVSSPLIT)
            Split = TRUE;
        shadowsPtr = shadowsPtr->next;
    }
    if (!Split) /* completely on back - invisible */
    {
        if (!(*t)->data.original)
            _rpPVSDestroyPoly(&((*t)->data.geom)); /* memory time waster */
        RwFree(*t);
        *t = (_rpPVSPolyList *) NULL;

        RWRETURN(TRUE);
    }
    else
    {
        RWRETURN(FALSE); /* Need to split! */
    }
}

static void
PVSPolygonClipByVolume(_rpPVSPolyListPtr * t, _rpPVSPolyRecord * Clipper,
                       _rpPVSPolyListPtr * inList, _rpPVSPolyListPtr * inListTail,
                       _rpPVSPolyListPtr * outList,
                       _rpPVSPolyListPtr * outListTail, RwInt32 side)
{
    /* Volume to clip against will be based on
     * the semi-infinite frustrum created by
     * each edge of the Clipper and the ViewPoint */

    _rpPVSPartitionId         Slice;
    _rpPVSPolyRecord          inPart, outPart;
    RwInt32             i = 0;
    _rpPVSPolyPtr             shadowsPtr;

    RWFUNCTION(RWSTRING("PVSPolygonClipByVolume"));

    /* Now clip against shadow edges... */
    shadowsPtr = Clipper->geom;
    while (i++ < side)
    {
        shadowsPtr = shadowsPtr->next;
    }

    Slice =
        PVSClassifyPolygonWRTPlane(&((*t)->data),
                                   &(shadowsPtr->shadowPlane), rpPVSSMALL);

    if (Slice == rpPVSFRONT)
    {
        PVSaddendSupremeMacro((*outList), (*outListTail), (*t));
    }
    else if (Slice == rpPVSBACK || Slice == rpPVSCOPLANAR)
    {

        if (shadowsPtr->next != NULL) /* otherwise all clip edges are done */
        {
            /* Recursively call... */
            PVSPolygonClipByVolume(t, Clipper, inList, inListTail,
                                   outList, outListTail, side + 1);
        }
        else
        {
            PVSaddendSupremeMacro((*inList), (*inListTail), (*t));
        }
    }
    else                       /* Slide==rpPVSSPLIT */
    {

        _rpPVSPolyListPtr         tmpi = (_rpPVSPolyList *) NULL;
        _rpPVSPolyListPtr         tmpo = (_rpPVSPolyList *) NULL;

        PVSDivide(&((*t)->data), shadowsPtr->shadowPlane, &inPart,
                  &outPart);

        if (!(*t)->data.original)
            _rpPVSDestroyPoly(&((*t)->data.geom)); /* memory time waster */
        RwFree(*t);
        *t = (_rpPVSPolyList *) NULL;

        tmpo = _rpPVScreateNode(&outPart);
        PVSaddendSupremeMacro((*outList), (*outListTail), (tmpo));

        tmpi = _rpPVScreateNode(&inPart);

        if (shadowsPtr->next != NULL) /* otherwise all clip edges are done */
        {
            /* Recursively call... */
            PVSPolygonClipByVolume(&tmpi, Clipper, inList, inListTail,
                                   outList, outListTail, side + 1);
        }
        else
        {
            PVSaddendSupremeMacro((*inList), (*inListTail), (tmpi));
        }
    }

    RWRETURNVOID();
}

static RpWorldSector *
PVSWAClassifyPotOccSector(RpWorldSector * sector, void *data)
{
    RpPVS              *ppTo = PVSFROMWORLDSECTOR(sector);
    RwV3d               sup, inf, Corner[8];
    RwInt32             i;
    _rpPVSPolyPtr             shadowsPtr;
    RwBool              OnFront = FALSE, OnBack = FALSE;
    _rpPVSPartitionId         test;
    _rpPVSPolyRecord         *Clipper = (_rpPVSPolyRecord *) data;

    RWFUNCTION(RWSTRING("PVSWAClassifyPotOccSector"));

    if (ppTo->numpols <= rpPVSSECTORCULLTOL)
    {
        /* Does not matter about assignment,
         * since there few or no polygons */
        ppTo->potential = rpPVSSPLIT;
        RWRETURN(sector);
    }

    if (PVSVISMAPGETSECTOR
        (rpPVSGlobals.CurrPVS->vismap, ppTo->sectorID))
    {
        /* Already known visible, so treat as front */
        ppTo->potential = rpPVSFRONT;
        RWRETURN(sector);
    }

    if (Clipper->homeaddr == sector)
    {
        /* This is the current sector so it must be split by definition */
        ppTo->potential = rpPVSSPLIT;
        RWRETURN(sector);
    }

    /* Use gbox, since we're culling */
    sup = ppTo->gbox.sup;
    inf = ppTo->gbox.inf;

    Corner[0].x = sup.x;
    Corner[0].y = sup.y;
    Corner[0].z = sup.z;
    Corner[1].x = sup.x;
    Corner[1].y = sup.y;
    Corner[1].z = inf.z;
    Corner[2].x = sup.x;
    Corner[2].y = inf.y;
    Corner[2].z = sup.z;
    Corner[3].x = sup.x;
    Corner[3].y = inf.y;
    Corner[3].z = inf.z;
    Corner[4].x = inf.x;
    Corner[4].y = sup.y;
    Corner[4].z = sup.z;
    Corner[5].x = inf.x;
    Corner[5].y = sup.y;
    Corner[5].z = inf.z;
    Corner[6].x = inf.x;
    Corner[6].y = inf.y;
    Corner[6].z = sup.z;
    Corner[7].x = inf.x;
    Corner[7].y = inf.y;
    Corner[7].z = inf.z;

    /* Sector clip against shadow face */
    OnFront = FALSE;
    OnBack = FALSE;
    for (i = 0; i < 8; i++)
    {
        test =
            PVSPointWRTPlane2(&(Corner[i]), &(Clipper->plane), rpPVSSMALL);
        if (test == rpPVSBACK)
            OnBack = TRUE;
        else
            OnFront = TRUE;
    }

    if (OnFront == TRUE && OnBack == FALSE)
    {
        ppTo->potential = rpPVSFRONT;
        RWRETURN(sector);
    }
    else if (OnFront == TRUE && OnBack == TRUE)
    {
        ppTo->potential = rpPVSSPLIT; /* Complex */
        RWRETURN(sector);
    }
    /* else cant be sure yet */

    /* Sector clip against shadow volume edges */
    shadowsPtr = Clipper->geom;
    while (shadowsPtr != NULL)
    {
        OnFront = FALSE;
        OnBack = FALSE;
        for (i = 0; i < 8; i++)
        {
            test =
                PVSPointWRTPlane2(&(Corner[i]),
                                  &(shadowsPtr->shadowPlane), rpPVSSMALL);
            if (test == rpPVSBACK)
                OnBack = TRUE;
            else
                OnFront = TRUE;
        }
        shadowsPtr = shadowsPtr->next;

        if (OnFront == TRUE && OnBack == FALSE)
        {
            ppTo->potential = rpPVSFRONT;
            RWRETURN(sector);
        }
        else if (OnFront == TRUE && OnBack == TRUE)
        {
            ppTo->potential = rpPVSSPLIT; /* Complex */
            RWRETURN(sector);
        }
        /* else cant be sure yet */
    }

    /* Can be sure of back, now */
    ppTo->potential = rpPVSBACK;
    RWRETURN(sector);
}

static RwBool
PVSCoplanar(_rpPVSPolyRecord * polygonA, _rpPVSPolyRecord * polygonB)
{
    RWFUNCTION(RWSTRING("PVSCoplanar"));

    if ((PVSPointOnPlane2
     (&(polygonB->geom->v), &(polygonA->plane), rpPVSTHOU))
    &&
    (PVSPointOnPlane2
     (&(polygonB->geom->next->v), &(polygonA->plane), rpPVSTHOU))
    &&
    (PVSPointOnPlane2
     (&(polygonB->geom->next->next->v), &(polygonA->plane), rpPVSTHOU)))
    RWRETURN(TRUE);

    RWRETURN(FALSE);
}

static void
PVSWASubdivide(_rpPVSPolyRecord * ClipperElement, _rpPVSPolyListPtr * Tlist)
{
    /* Using a Clipper polygon, Clips all rpPVSGlobals.polygons in Tlist
     */

    _rpPVSPolyListPtr         temp = (_rpPVSPolyList *) NULL;
    _rpPVSPolyListPtr         post = (_rpPVSPolyList *) NULL;
    _rpPVSPolyListPtr         prev;
    _rpPVSPolyListPtr         todel;
    _rpPVSPolyListPtr         end;
    _rpPVSPolyListPtr         ttt = (_rpPVSPolyList *) NULL;
    _rpPVSPolyListPtr         inList = (_rpPVSPolyList *) NULL;
    _rpPVSPolyListPtr         outList = (_rpPVSPolyList *) NULL;
    _rpPVSPolyListPtr         inListTail = (_rpPVSPolyList *) NULL;
    _rpPVSPolyListPtr         outListTail = (_rpPVSPolyList *) NULL;

    RWFUNCTION(RWSTRING("PVSWASubdivide"));

    /* Classify each sector as definately invisible (in),
     * definately visibile (out), or not sure (split) - more work */
    if (rpPVSSECTORCULL)
        RpWorldForAllWorldSectors(rpPVSGlobals.World,
                                  PVSWAClassifyPotOccSector,
                                  ClipperElement);

    ClipperElement->hasbeenclipper = TRUE;
    /* Clip t against Clipper,
     * and add to inList or outList, or parts to both... */
    temp = *Tlist;
    prev = (_rpPVSPolyList *) NULL;

    while (temp != NULL)
    {

        /* for each poly in Tlist,
         * check whether it is visible wrt clipper element */
        RpPVS              *ppTo =
            PVSFROMWORLDSECTOR(temp->data.homeaddr);

        /* If clippee is in visible sector or
         * cannot be obscured since it resides in an 'out' sector,
         * don't bother clipping, and jump to next sectors polygons */
        if (rpPVSJUMP)
        {
            while (temp != NULL
                   &&
                   ((PVSVISMAPGETSECTOR
                     (rpPVSGlobals.CurrPVS->vismap, temp->data.home))
                    || (ppTo->potential != rpPVSSPLIT)))
            {
                if (ppTo->potential == rpPVSBACK)
                {
                    end = ppTo->sectailpoly->next;
                    while (temp != NULL && temp != end)
                    {
                        if (!temp->data.original)
                            _rpPVSDestroyPoly(&(temp->data.geom));

                        todel = temp;
                        if (*Tlist == todel)
                            *Tlist = temp->next;
                        temp = temp->next;
                        RwFree(todel);
                        todel = (_rpPVSPolyList *) NULL;
                    }

                    if (prev != NULL)
                        prev->next = temp;
                    if (temp != NULL)
                        ppTo = PVSFROMWORLDSECTOR(temp->data.homeaddr);
                }
                else
                {

                    prev = ppTo->sectailpoly;

                    RWASSERT(prev != NULL);

                    temp = ppTo->sectailpoly->next;

                    if (temp != NULL)
                        ppTo = PVSFROMWORLDSECTOR(temp->data.homeaddr);
                }
            }

        }
        if (temp != NULL)
        {
            RwBool              tempisend = FALSE;

            post = temp->next;
            if (temp == ppTo->sectailpoly)
                tempisend = TRUE;

            /* Test to see if clipping is necessary.  It is necessary if
             * i.   The clipper and clippee are in different sectors (it's inefficient
             *      and inaccurate to clip mutual sector polygons)
             * ii.  The clipper and clippee do not share edges (for same reason as i.
             *      but this is mainly catered for in i. so it is not tested here.)
             * iii. Clipper and clippee don't have same parent (this is the case after
             *      i. so is not tested here)
             * iv.  Clippee is on back of clipper, and clipper is on front of clippee -
             *      otherwise all hell breaks loose
             * v.   Clippee is inside the conical shadow volume of the clipper - this saves
             *      a load of unneccessary clipping - if rather expensive
             * vi.  Clippee is inside the edge-wise shadow volume of the clipper
             */
            if ((ClipperElement->homeaddr != temp->data.homeaddr) && /* i */
                /*(!PVSSharedEdge(&temp->data, ClipperElement)) &&*/ /* ii */
                /*(ClipperElement->parent != temp->data.parent) &&*/ /* iii */
                (!PVSPolygonTrivByFace(&temp, ClipperElement)) && /* iv */
                (!PVSPolygonTrivByVolume(&temp, ClipperElement)) && /* v */
                (!PVSPolygonTrivByCone(&temp, ClipperElement))) /* vi */

            {
                inList = (_rpPVSPolyList *) NULL;
                outList = (_rpPVSPolyList *) NULL;
                inListTail = (_rpPVSPolyList *) NULL;
                outListTail = (_rpPVSPolyList *) NULL;

                PVSPolygonClipByVolume(&temp, ClipperElement, &inList,
                                       &inListTail, &outList,
                                       &outListTail, 0);
                /* else, split - orig removed */

                /* Remove things in inlist */
                ttt = inList;
                while (ttt != NULL)
                {
                    if (!ttt->data.original)
                    {
                        /* memory time waster */
                        _rpPVSDestroyPoly(&(ttt->data.geom));
                    }

                    todel = ttt;
                    ttt = ttt->next;
                    RwFree(todel);
                    todel = (_rpPVSPolyList *) NULL;
                }

                /* Put new (outlist) fragments onto list */

                if (prev != NULL)
                {
                    prev->next = outList;
                }
                else
                {
                    *Tlist = outList;
                }
                outListTail->next = post;
                prev = outListTail;

                /* Check jump info.... */
                if (tempisend && temp == NULL)
                {
                    /* temp polygon was at end of
                     * the polygon list for that sector */
                    ppTo->sectailpoly = prev;
                    /* assign end fragment (from original) as sector tail  */
                }

            }
            else
            {
                /* Check jump info.... */
                if (tempisend && temp == NULL)
                {
                    /* temp polygon was at end of
                     * the polygon list for that sector */
                    if (prev != NULL)
                    {
                        if (PVSFROMWORLDSECTOR(prev->data.homeaddr) ==
                            ppTo)
                        {
                            /* assign the previous cell as the end  */
                            ppTo->sectailpoly = prev;
                        }
                        else
                        {
                            /* no more polygons in this sector! */
                            ppTo->sectailpoly = (_rpPVSPolyList *) NULL;
                        }

                    }
                    else
                    {
                        /* no more polygons in this sector
                         * (we're at head too)! */
                        ppTo->sectailpoly = (_rpPVSPolyList *) NULL;
                    }
                }

                if (temp != NULL)
                {
                    prev = temp;
                }
                else
                {
                    if (prev != NULL)
                    {
                        prev->next = post;
                    }
                    else
                    {
                        *Tlist = post;
                    }

                }
            }
            temp = post;

        }
    }

    RWRETURNVOID();
}

static void
PVSAssignPriorities(_rpPVSPolyListPtr * Tlist, RwV3d * ViewPoint)
{
    RwV3d               Vec;
    RwInt32             lasthome = -1;
    RwReal              lastpriority = 0.0f;
    RwReal              addpri;
    _rpPVSPolyListPtr         temp;
    RwV3d               t;

    RWFUNCTION(RWSTRING("PVSAssignPriorities"));

    temp = *Tlist;
    while (temp != NULL)       /* assign priorities to each polygon */
    {
        if (temp->data.home != lasthome)
        {
            /* Sort by sector first (make most significant) */

            RpPVS              *pvsCur =
                PVSFROMWORLDSECTOR(temp->data.homeaddr);

            if (pvsCur != rpPVSGlobals.CurrPVS)
            {
                int i;

                /*Zsort - nearest z */
                temp->data.priority=RwRealMAXVAL;
                for (i=0; i<8; i++)
                {
                    /* Use gbox since we're sorting by potential occluders */
                    if (i%2==0) t.x=pvsCur->gbox.inf.x;
                    else t.x=pvsCur->gbox.sup.x;

                    if ((i/2)%2==0) t.y=pvsCur->gbox.inf.y;
                    else t.y=pvsCur->gbox.sup.y;

                    if (i<4) t.z=pvsCur->gbox.inf.z;
                    else t.z=pvsCur->gbox.sup.z;

                    RwV3dSub(&Vec, &t, ViewPoint);
                    temp->data.priority = rpPVSMIN(temp->data.priority,RwV3dDotProduct(&Vec, &Vec));
                }
            }
            else
                temp->data.priority = -1.0f;

            lasthome = temp->data.home;
            lastpriority = temp->data.priority;
        }

        /* Subsort by polygon (make least significant) */
        temp->data.priority = lastpriority;

        RwV3dSub(&Vec, &temp->data.centroid, ViewPoint);
        /*Zsort - nearest z */
        addpri = RwV3dDotProduct(&Vec, &Vec);

        temp->data.priority += addpri * RwRealMINVAL;

        temp = temp->next;
    }

    RWRETURNVOID();
}

static void
PVSAssignJumpInfo(_rpPVSPolyListPtr * data)
{
    _rpPVSPolyListPtr         temp = *data;
    RwInt32             currid;

    RWFUNCTION(RWSTRING("PVSAssignJumpInfo"));

    while (temp != NULL)
    {
        currid = temp->data.home;
        if (temp->next == NULL || temp->next->data.home != currid)
        {
            RpPVS              *pvsCur =
                PVSFROMWORLDSECTOR(temp->data.homeaddr);
            pvsCur->sectailpoly = temp;
        }
        temp = temp->next;
    }

    RWRETURNVOID();
}

static void
PVSSortList(_rpPVSPolyListPtr * Tlist, _rpPVSPolyListPtr * end)
{
    /* find last element in list */
    RwBool              sorted;
    _rpPVSPolyListPtr         tempa, tempb, preva, postb, last, newlast;

    RWFUNCTION(RWSTRING("PVSSortList"));

    last = *end;
    newlast = last;

    /* sort through to last position that was swapped */

    if (*Tlist != *end) /* Otherwise only one element, and thus already sorted */
    {
        do
        {
            sorted = TRUE;
            tempa = *Tlist;
            tempb = tempa->next;
            postb = tempb->next;
            preva = (_rpPVSPolyList *) NULL;

            while (tempa != last)
            {
                if (tempb->data.priority < tempa->data.priority)
                {
                    sorted = FALSE;

                    if (preva == NULL)
                    {
                        (*Tlist) = tempb;
                    }
                    else
                    {
                        preva->next = tempb;
                    }
                    newlast = tempa;
                    preva = tempb;

                    tempb->next = tempa;
                    tempa->next = postb;

                    tempa = tempb->next;
                    tempb = tempa->next;

                    if (tempa->next == NULL)
                    {
                        last = tempa;
                        *end = tempa;
                    }
                    if (postb != NULL)
                        postb = postb->next; /* else last interation anyway */
                }
                else
                {
                    preva = tempa;
                    tempa = tempa->next;
                    tempb = tempb->next;
                    if (postb != NULL)
                        postb = postb->next; /* else last interation anyway */
                }
            }
            last = newlast;
        }
        while (!sorted);
    }
    RWRETURNVOID();
}

static void
PVSSortSectors(_rpPVSPolyListPtr * Tlist)
{
    _rpPVSPolyListPtr         last = (_rpPVSPolyListPtr) NULL;
    _rpPVSPolyListPtr         newlast;
    _rpPVSPolyListPtr         nexthead;
    _rpPVSPolyListPtr         nexttail;
    _rpPVSPolyListPtr         nexttailnext;
    _rpPVSPolyListPtr         prevtemphead;
    _rpPVSPolyListPtr         temphead;
    _rpPVSPolyListPtr         temptail;
    RwBool              sorted;

    RWFUNCTION(RWSTRING("PVSSortSectors"));

    /* find last element in list */
    temphead = *Tlist;
    while (temphead != NULL)
    {
        temphead =
            PVSFROMWORLDSECTOR(temphead->data.homeaddr)->sectailpoly->
            next;
        if (temphead != NULL)
            last = temphead;
    }
    newlast = last;

    do
    {
        sorted = TRUE;

        prevtemphead = (_rpPVSPolyList *) NULL;
        temphead = *Tlist;     /* temp points to head of list */
        while (temphead != last)
        {
            temptail =
                PVSFROMWORLDSECTOR(temphead->data.homeaddr)->
                sectailpoly;
            nexthead = temptail->next;
            if (nexthead == NULL)
                break;         /* We reached the end of swappability */
            nexttail =
                PVSFROMWORLDSECTOR(nexthead->data.homeaddr)->
                sectailpoly;
            nexttailnext = nexttail->next;

            if (nexthead->data.priority < temphead->data.priority)
            {
                /* swap span neighbours */
                sorted = FALSE;
                if (prevtemphead == NULL)
                    (*Tlist) = nexthead;
                else
                    prevtemphead->next = nexthead;
                nexttail->next = temphead;
                temptail->next = nexttailnext;
                /* temphead=temphead; */
                prevtemphead = nexttail;
            }
            else
            {
                prevtemphead = temptail;
                temphead = nexthead;
            }
        }
        last = newlast;
    }
    while (!sorted);

    RWRETURNVOID();
}

static void
PVSSortSubSectors(_rpPVSPolyListPtr * Tlist)
{
    _rpPVSPolyListPtr         head, tail, prev = (_rpPVSPolyList *) NULL, temp;

    RWFUNCTION(RWSTRING("PVSSortSubSectors"));

    head = *Tlist;
    while (head != NULL)
    {
        tail = PVSFROMWORLDSECTOR(head->data.homeaddr)->sectailpoly;

        temp = tail->next;
        tail->next = (_rpPVSPolyList *) NULL;
        PVSSortList(&head, &tail);
        tail->next = temp;

        if (prev != NULL)
            prev->next = head;
        else
            *Tlist = head;

        prev = tail;
        head = tail->next;
    }

    RWRETURNVOID();
}

static void
PVSDeleteColinearsAndSlivers(_rpPVSPolyPtr * polygon)
{
    _rpPVSPolyPtr             Pnext, Pnn, todel, P;
    RwV3d               v1, v2;


    RWFUNCTION(RWSTRING("PVSDeleteColinearsAndSlivers"));

    P = *polygon;


    /* First rid identical vertices */
    while (P != NULL)          /* go round polygon */
    {
        Pnext = P->next;
        if (Pnext == NULL) Pnext = *polygon;

        Pnn = Pnext->next;
        if (Pnn == NULL) Pnn = *polygon;

        if (PVSVerticesEqual(P->v, Pnext->v))
        {
            /* Identical vertices */
            todel = Pnext;
            if (P->next != NULL) P->next = Pnext->next;
            else
            {
                *polygon = Pnn;
                P = (_rpPVSPoly *) NULL; /* we're at the end so can quit */
            }
            RwFree(todel);
            todel = (_rpPVSPoly *) NULL;

            if (_rpPVSLinkCardinality2(*polygon) < 3)
            {
                /* During our efforts, we seem to have found a sliver;
                 * we shall remove it */
                _rpPVSDestroyPoly(polygon);
                RWRETURNVOID();
            }

        }
        else
        {
            P = P->next;
        }
    }

    /* now rid colinears */
    P = *polygon;
    while (P != NULL)          /* go round polygon */
    {
        RwReal cosAngle;

        Pnext = P->next;
        if (Pnext == NULL)
            Pnext = *polygon;
        Pnn = Pnext->next;
        if (Pnn == NULL)
            Pnn = *polygon;

        RwV3dSub(&v1, &(Pnext->v), &(P->v));
        RwV3dNormalize(&v1, &v1);
        RwV3dSub(&v2, &(Pnn->v), &(Pnext->v));
        RwV3dNormalize(&v2, &v2);

        cosAngle = RwV3dDotProduct(&v1, &v2);

        if (cosAngle > 0.9998f)
        {
            /* Same gradient:
             * Pnext is a relative colinear point and is redundant */
            todel = Pnext;
            if (P->next != NULL)
                P->next = Pnext->next;
            else
            {
                *polygon = Pnn;
                P = (_rpPVSPoly *) NULL; /* we're at the end so can quit */
            }
            RwFree(todel);
            todel = (_rpPVSPoly *) NULL;
        }
        else
            P = P->next;
    }

    /* Check that triangle is at least topologically ok */
    if (_rpPVSLinkCardinality2(*polygon) < 3)
    {
        _rpPVSDestroyPoly(polygon);
        RWRETURNVOID();
    }

    /* Check that triangle is at least topologically ok */
    if ((PVSAreaOfPolygon(*polygon)) < rpPVSTINY)
    {
        _rpPVSDestroyPoly(polygon);
        RWRETURNVOID();
    }
    RWRETURNVOID();
}

static RwInt32
PVSPointWRTPlane3(RwV3d * Cand, _rpPVSPlaneEq * Root, RwReal Zero)
{
    RwReal              scalar;

    RWFUNCTION(RWSTRING("PVSPointWRTPlane3"));

    scalar =
        (RwV3dDotProduct((RwV3d *) Root, Cand) + Root->w) * Root->l;

    if (scalar >= Zero)
        RWRETURN(1);
    if (scalar <= -Zero)
        RWRETURN(2);

    RWRETURN(0);
}

#if (defined(PVSMERGEONLYORTHOGON))

static RwInt32
PVSOrthogonal(_rpPVSPolyRecord * myPoly)
{
    RWFUNCTION(RWSTRING("PVSOrthogonal"));

    if ((EQ(myPoly->geom->v.x, myPoly->geom->next->v.x)) &&
       (EQ(myPoly->geom->next->v.x, myPoly->geom->next->next->v.x)))
            RWRETURN(0);
    if ((EQ(myPoly->geom->v.y, myPoly->geom->next->v.y)) &&
       (EQ(myPoly->geom->next->v.y, myPoly->geom->next->next->v.y)))
            RWRETURN(4);
    if ((EQ(myPoly->geom->v.z, myPoly->geom->next->v.z)) &&
       (EQ(myPoly->geom->next->v.z, myPoly->geom->next->next->v.z)))
            RWRETURN(8);

    RWRETURN(-1);
}

#endif /* (defined(PVSMERGEONLYORTHOGON)) */

/*
 * Given a polygon and a polygon,
 * test if they can be merged into a single polygon;
 * if so, do it, put result back into polygon and
 * return true, else false
 */
static RwBool
PVSMergeGeometry2(_rpPVSPolyRecord * polygonA, _rpPVSPolyRecord * polygonB)
{
    _rpPVSPolyPtr             geom = (_rpPVSPoly *) NULL;
    _rpPVSPolyPtr             pend = (_rpPVSPoly *) NULL;
    RwV3d               castPoint, StartPoint;
    _rpPVSPolyPtr             TG, PG, T, P, Pn, temp, outer, outern, inner;
    _rpPVSPartitionId         refside = rpPVSSPLIT, oppside = rpPVSSPLIT;
    RwBool              concave;
    _rpPVSPlaneEq             shadowplane;
    RwInt32             switches = 0;
    RwV3d               swPoint[2];
    RwBool mutuallySeparable = FALSE, areConcave = FALSE, isConnected = FALSE;
    RwInt32 side, thisSide;

    RWFUNCTION(RWSTRING("PVSMergeGeometry2"));

    castPoint.x = polygonB->centroid.x + polygonB->plane.x;
    castPoint.y = polygonB->centroid.y + polygonB->plane.y;
    castPoint.z = polygonB->centroid.z + polygonB->plane.z;


    /* Check if polygonB is coplanar with polygonA */

#if (defined(PVSMERGEONLYORTHOGON))
    {
        /* Only merge orthogonal polygons */
        RwInt32             orientationA, orientationB;

        orientationA = PVSOrthogonal(polygonA);
        orientationB = PVSOrthogonal(polygonB);
        if ((orientationA == -1) || (orientationB == -1) || (orientationA != orientationB))
        {
            RWRETURN(FALSE);
        }
        if (orientationA == 0)
        {
            if (!EQ(polygonA->geom->v.x, polygonB->geom->v.x)) RWRETURN(FALSE);
        }
        if (orientationA == 4)
        {
            if (!EQ(polygonA->geom->v.y, polygonB->geom->v.y)) RWRETURN(FALSE);
        }
        if (orientationA == 8)
        {
            if (!EQ(polygonA->geom->v.z, polygonB->geom->v.z)) RWRETURN(FALSE);
        }
    }
#else /* (defined(PVSMERGEONLYORTHOGON)) */
    {
        if (!PVSCoplanar(polygonA, polygonB))
            RWRETURN(FALSE);
    }
#endif /* (defined(PVSMERGEONLYORTHOGON)) */

    /* Check if polygonB facing same way as polygonA */
    if (PVSPointWRTPlane2(&castPoint, &(polygonB->plane), 0.0f) !=
        PVSPointWRTPlane2(&castPoint, &(polygonA->plane), 0.0f))
        RWRETURN(FALSE);


    /* Check if polygonB and polygonA have different opacity */
    if (polygonA->translucent != polygonB->translucent)
    {
        RWRETURN(FALSE);
    }


    /* NOW, A MOST IMPORTANT TEST...
     *
     * See if the polys are mutually separable - i.e. not overlapping, and
     * check for the guaranteed potential for concavities
     *
     * Algorithm: from cast point, use edge P->Pn to generate a plane.
     * If all points in T are on or outside plane, accept
     * However, if a point of T is on back, and a point is on front, it will be concave!
     */
    TG = polygonB->geom;
    PG = polygonA->geom;
    P = PG;
    while (P!=NULL)
    {
        RwInt32 verticesConnected = 0;

        Pn = P->next;
        if (Pn == NULL) Pn = PG;


        PVSGeneratePlaneEquationFromThreePoints(&shadowplane,
                                                &castPoint,
                                                &(P->v),
                                                &(Pn->v));

        side = 0;
        T = TG;
        while (T!=NULL)
        {
            thisSide = PVSPointWRTPlane3(&T->v, &shadowplane, rpPVSTHOU);
            if (thisSide == 0) verticesConnected++;
            side |= thisSide;
            T = T->next;
        }
        if (verticesConnected >= 2) isConnected = TRUE;


        if (side == 1) mutuallySeparable = TRUE;
        else if (side == 3) areConcave = TRUE;
        /* else, nothing determined. */


        if (areConcave) RWRETURN(FALSE);
        /* NB Need all tests, as mutSep and areCon can BOTH be true */

        P = P->next;
    }
    if (!mutuallySeparable || !isConnected) RWRETURN(FALSE);



    TG = polygonB->geom;
    PG = polygonA->geom;

    /* must start on a non-common vertex! */
    T = TG;
    P = PG;
    switches = 0;

    /* check for identical polygons... */
    do
    {
        switches = 0;
        T = TG;
        while (T != NULL)      /* go round tri */
        {
            if ((T != P) && (PVSVerticesEqual(P->v, T->v)))
                switches = 1;
            T = T->next;
        }
        if (switches)
            P = P->next;
        if (P == NULL && switches)
            RWRETURN(FALSE);   /* identical polygons exist */
    }
    while (switches);

    StartPoint = P->v;

    /* merge the two polygons... */
    do                         /* go round polygonA */
    {

        _rpPVSaddendIIp(&pend, &(P->v));
        if (geom == NULL)
            geom = pend;
        T = TG;
        while (T != NULL)      /* go round tri */
        {
            if (switches>2) break;
            if (PVSVerticesEqual(P->v, T->v) && switches<2)
            {
                swPoint[switches] = P->v;
                switches++;
                temp = TG;
                TG = PG;
                PG = temp;
                P = T;

                T = (_rpPVSPoly *) NULL;
            }
            else
                T = T->next;
        }
        P = P->next;
        if (P == NULL)
            P = PG;
        if (switches>2) break;
    }
    while (!PVSVerticesEqual(P->v, StartPoint));

    if (switches != 2 || PVSVerticesEqual(swPoint[0], swPoint[1]))
    {
        /* Doh! There was an error - probably caused by tiny fragments - so
         * we won't merge - to save later problems */
        _rpPVSDestroyPoly(&geom);
        RWRETURN(FALSE);
    }



    /* CHECK POLYGON IS CONVEX... */
    PVSDeleteColinearsAndSlivers(&geom); /* this removes colinears before concavity check */
    if (geom == NULL) RWRETURN(FALSE);

    while (refside == rpPVSSPLIT)
    {
        refside =
            PVSPointWRTPlane2(&castPoint, &polygonB->plane, 0.0f);
    }

    if (refside == rpPVSFRONT)
        oppside = rpPVSBACK;
    else
        oppside = rpPVSFRONT;

    RWASSERT(refside != rpPVSSPLIT);

    outer = geom;
    concave = FALSE;
    while (outer != NULL && !concave) /* go round polygon */
    {
        outern = outer->next;
        if (outern == NULL)
            outern = geom;

        PVSGeneratePlaneEquationFromThreePoints(&shadowplane,
                                                &castPoint,
                                                &(outern->v),
                                                &(outer->v));

        inner = geom;
        while (inner != NULL && !concave)
        {
            if (((inner != outer) && (inner != outern))
                &&
                (PVSPointWRTPlane2(&(inner->v), &shadowplane, 0.0f))
                == oppside)
                /* 0 is CRITICAL - do not use "rpPVSSMALL" - can't allow even minor
                 * concavities! */
                concave = TRUE;
            inner = inner->next;
        }
        outer = outer->next;
    }
    /* ...END CHECK POLYGON IS CONVEX */

    {
        RwReal combArea = PVSAreaOfPolygon(geom);
        RwReal expArea = PVSAreaOfPolygon(polygonB->geom) + PVSAreaOfPolygon(polygonA->geom);
        RwReal err;

        err = combArea/expArea;

        if (err > 1.01f || err < 0.99f)
        {
            _rpPVSDestroyPoly(&geom);
            RWRETURN(FALSE);
        }
    }

    if (!concave && _rpPVSLinkCardinality2(geom) >= 3)
    {
        _rpPVSDestroyPoly(&polygonA->geom);
        _rpPVSDestroyPoly(&polygonB->geom);

        polygonA->geom = geom;
        T = polygonA->geom;

        RWRETURN(TRUE);
    }
    else
    {
        _rpPVSDestroyPoly(&geom);
        RWRETURN(FALSE);
    }
}

static RpWorldSector *
PVSCopyAndTag(RpWorldSector * sector, void * data __RWUNUSED__)
{
    /* Number of iterations for detesselator (done just numiters times) */
    RwInt32             numiters = 3;
    _rpPVSPolyRecord          mypoly;
    RwInt32             ply;
    _rpPVSPolyPtr             geom = (_rpPVSPoly *) NULL;
    _rpPVSPolyPtr             pend = (_rpPVSPoly *) NULL;
    _rpPVSPolyListPtr         PolygonsEnd, TailOfLastSectorsPolys;
    RpTriangle         *rwTri;
    RwV3d              *rwVert;
    RwInt32             numPoly, numVert;
    RwInt32             fixiters;
    RpPVS              *pvsCur = PVSFROMWORLDSECTOR(sector);
    RpPVSCache         *pvsCache =
        PVSCACHEFROMWORLD(rpPVSGlobals.World);
    _rpPVSPolyListPtr         temp = (_rpPVSPolyList *) NULL;
    _rpPVSPolyListPtr         post = (_rpPVSPolyList *) NULL;
    _rpPVSPolyListPtr         todel = (_rpPVSPolyList *) NULL;
    _rpPVSPolyListPtr         prev = (_rpPVSPolyList *) NULL;

    RWFUNCTION(RWSTRING("PVSCopyAndTag"));

    PolygonsEnd = pvsCache->polygons;
    if (PolygonsEnd != NULL)
        while (PolygonsEnd->next != NULL)
            PolygonsEnd = PolygonsEnd->next;
    TailOfLastSectorsPolys = PolygonsEnd;

    numVert = (RwInt32) (RpWorldSectorGetNumVertices(sector));
    numPoly = (RwInt32) (RpWorldSectorGetNumTriangles(sector));

    /* Get the world sector's verts and poly indices. */
    rwVert = sector->vertices;
    rwTri = sector->triangles;

    /* Assign the vertices */
    for (ply = 0; ply < numPoly; ply++)
    {
        RpMaterial         *mat;
        const RwRGBA       *color;
        RwTexture          *tex;
        RwInt32             matid;
        RwBool              translucent = FALSE;
        _rpPVSPlaneEq             myplaneeq;
        RwReal              radius;
        RwV3d               centroid, extreme;

        matid = sector->matListWindowBase + rwTri[ply].matIndex;
        mat =
            rpMaterialListGetMaterial(&rpPVSGlobals.World->matList,
                                      matid);
        color = RpMaterialGetColor(mat);
        tex = RpMaterialGetTexture(mat);

        if (rpPVSTRANSLUCENCY
            && (color->alpha < 255 || (tex && tex->mask[0])))
            translucent = TRUE;

        pend = (_rpPVSPoly *) NULL;

        _rpPVSaddendIIp(&pend, &(rwVert[rwTri[ply].vertIndex[0]]));
        geom = pend;
        _rpPVSaddendIIp(&pend, &(rwVert[rwTri[ply].vertIndex[1]]));
        _rpPVSaddendIIp(&pend, &(rwVert[rwTri[ply].vertIndex[2]]));

        if (rpPVSDECOLINEAR) PVSDeleteColinearsAndSlivers(&geom);
        if (geom != NULL)
        {

            PVSGeneratePlaneEquationFromThreePoints(&myplaneeq ,
                                                    &(geom->v),
                                                    &(geom->next->v),
                                                    &(geom->next->next->v));

            PVSFindCentroidandRadius(&geom, &centroid, &radius,
                                     &extreme);
            PVSAssignPolygonAttrs(&mypoly, geom, myplaneeq,
                                  -1, TRUE, pvsCur->sectorID,
                                  (_rpPVSPolyList *) NULL, translucent,
                                  sector, FALSE, centroid, radius,
                                  extreme);

            _rpPVSaddendII(&PolygonsEnd, &mypoly);
            if (pvsCache->polygons == NULL)
                pvsCache->polygons = PolygonsEnd;

            PolygonsEnd->data.parent = PolygonsEnd;
        }

        if (!rpPVSGlobals.bfc)
        {
            /* add back face too! */
            pend = (_rpPVSPoly *) NULL;

            _rpPVSaddendIIp(&pend, &(rwVert[rwTri[ply].vertIndex[2]]));
            geom = pend;
            _rpPVSaddendIIp(&pend, &(rwVert[rwTri[ply].vertIndex[1]]));
            _rpPVSaddendIIp(&pend, &(rwVert[rwTri[ply].vertIndex[0]]));

            if (rpPVSDECOLINEAR) PVSDeleteColinearsAndSlivers(&geom);
            if (geom != NULL)
            {
                PVSGeneratePlaneEquationFromThreePoints(&myplaneeq,
                                                        &(geom->v),
                                                        &(geom->next->v),
                                                        &(geom->next->next->v));

                PVSFindCentroidandRadius(&geom, &centroid, &radius,
                                         &extreme);
                PVSAssignPolygonAttrs(&mypoly, geom, myplaneeq,
                                      -1, TRUE, pvsCur->sectorID,
                                      (_rpPVSPolyList *) NULL, translucent,
                                      sector, FALSE, centroid, radius,
                                      extreme);

                _rpPVSaddendII(&PolygonsEnd, &mypoly);
                if (pvsCache->polygons == NULL)
                    pvsCache->polygons = PolygonsEnd;

                PolygonsEnd->data.parent = PolygonsEnd;
            }
        }/* end addition of backface */
    }


    /* Detesselation... */
    for (fixiters = 0; fixiters < numiters; fixiters++)
    {
        if ((rpPVSDETESS) && (numPoly > 1) && (pvsCache->polygons != NULL))
        {
            _rpPVSPolyListPtr         outer = (_rpPVSPolyList *) NULL;
            _rpPVSPolyListPtr         innerprev = (_rpPVSPolyList *) NULL;
            _rpPVSPolyListPtr         inner = (_rpPVSPolyList *) NULL;
            _rpPVSPolyListPtr         todel = (_rpPVSPolyList *) NULL;

            if (TailOfLastSectorsPolys != NULL)
                outer = TailOfLastSectorsPolys->next;
            else
                outer = pvsCache->polygons;

            while (outer != NULL)
            {
                /* initially point to first polygon in current sector */
                innerprev = outer;
                inner = outer->next;
                while (inner != NULL)
                {
                    if (outer != inner
                        &&
                        (PVSMergeGeometry2
                         (&(outer->data), &(inner->data))))
                    {

                        /* outer now holds merged shape, inner is redundant */
                        todel = inner;
                        innerprev->next = inner->next;
                        RwFree(todel);
                        todel = (_rpPVSPolyList *) NULL;
                        inner = innerprev->next;
                        /* innerprev=innerprev; */

                        /* reassign some attrs... */
                        PVSGeneratePlaneEquationFromThreePoints(&outer->data.plane,
                                                                &(outer->data.geom->v),
                                                                &(outer->data.geom->next->v),
                                                                &(outer->data.geom->next->next->v));
                        PVSFindCentroidandRadius(&(outer->data.geom),
                                                 &(outer->data.
                                                   centroid),
                                                 &(outer->data.radius),
                                                 &(outer->data.
                                                   extreme));
                    }
                    else
                    {
                        inner = inner->next;
                        innerprev = innerprev->next;
                    }

                }
                outer = outer->next;
            }
        }
    }


    if (TailOfLastSectorsPolys != NULL)
    {
        pvsCur->numpols = _rpPVSLinkCardinality(TailOfLastSectorsPolys);
        pvsCur->numpols--;
    }
    else if (pvsCache->polygons != NULL)
    {
        pvsCur->numpols = _rpPVSLinkCardinality(pvsCache->polygons);
    }

    /* Decolinearizer... */
    if (rpPVSDECOLINEAR)
    {
        if (TailOfLastSectorsPolys != NULL)
            temp = TailOfLastSectorsPolys->next;
        else
            temp = pvsCache->polygons;
        while (temp != NULL)
        {
            post = temp->next;

            PVSDeleteColinearsAndSlivers(&(temp->data.geom));
            if (temp->data.geom == NULL)
            {
                todel = temp;
                if (prev != NULL)
                    prev->next = temp->next;
                else
                    pvsCache->polygons = temp->next;
                RwFree(todel);
                todel = (_rpPVSPolyList *) NULL;
                temp = post;
            }
            else
            {
                prev = temp;
                temp = post;
            }
        }
    }

#if 0
    /* small bug which means we can't calculate these...but they aren't really used,
     * not critical, so not worthy of my debugging it */
    if (TailOfLastSectorsPolys != NULL)
    {
        pvsCur->numpols = _rpPVSLinkCardinality(TailOfLastSectorsPolys);
        pvsCur->numpols--;
    }
    else if (pvsCache->polygons != NULL)
    {
        pvsCur->numpols = _rpPVSLinkCardinality(pvsCache->polygons);
    }
#endif

    if (pvsCur->numpols == 0)
        pvsCur->sectailpoly = (_rpPVSPolyList *) NULL;

    RWRETURN(sector);
}

static void
PVSDivideBox(const RwBBox * box, RwReal value,
             RwBBox * lbox, RwBBox * rbox,
             RwInt32 align)
{
    RWFUNCTION(RWSTRING("PVSDivideBox"));

    /* First make boxes the same as the root */
    lbox -> inf.x = box -> inf.x;
    lbox -> inf.y = box -> inf.y;
    lbox -> inf.z = box -> inf.z;
    lbox -> sup.x = box -> sup.x;
    lbox -> sup.y = box -> sup.y;
    lbox -> sup.z = box -> sup.z;

    rbox -> sup.x = box -> sup.x;
    rbox -> sup.y = box -> sup.y;
    rbox -> sup.z = box -> sup.z;
    rbox -> inf.x = box -> inf.x;
    rbox -> inf.y = box -> inf.y;
    rbox -> inf.z = box -> inf.z;

    /* Now adjust the relevant coordinate */
    if (align==0)
    {
        lbox -> sup.x = value;
        rbox -> inf.x = value;
    }
    else if (align==4)
    {
        lbox -> sup.y = value;
        rbox -> inf.y = value;
    }
    else if (align==8)
    {
        lbox -> sup.z = value;
        rbox -> inf.z = value;
    }
    RWRETURNVOID();
}

static RwReal
PVSFindDiagonalSizeOfBBox(RwBBox *boundingBox)
{
    RwV3d diag;
    RwReal size;

    RWFUNCTION(RWSTRING("PVSFindDiagonalSizeOfBBox"));

    diag.x = (boundingBox->sup.x - boundingBox->inf.x);
    diag.y = (boundingBox->sup.y - boundingBox->inf.y);
    diag.z = (boundingBox->sup.z - boundingBox->inf.z);
    size = RwV3dLength(&diag);

    RWRETURN(size);
}

static RpSector *
PVSCalculateSectorBounds(RpSector * spSector, const RwBBox * box)
{
    RpPVS *pvsCur;
    RpWorldSector *wpSector;
    RwBBox leftbox;
    RwBBox rightbox;
    const RwBBox* gbox;

    RWFUNCTION(RWSTRING("PVSCalculateSectorBounds"));
    RWASSERT(spSector);
    RWASSERT(box);

    if (spSector->type < 0)
    {
        /*
         * It's an atomic sector, assign large and small bounding boxes
         * - one for the sector, one for the geometry respectively
         */
        wpSector = (RpWorldSector *) spSector;
        pvsCur = PVSFROMWORLDSECTOR(wpSector);
        pvsCur->sbox.inf.x = box->inf.x;
        pvsCur->sbox.inf.y = box->inf.y;
        pvsCur->sbox.inf.z = box->inf.z;
        pvsCur->sbox.sup.x = box->sup.x;
        pvsCur->sbox.sup.y = box->sup.y;
        pvsCur->sbox.sup.z = box->sup.z;

        gbox = (RpWorldSectorGetBBox((RpWorldSector*)spSector));
        pvsCur->gbox.inf.x = gbox->inf.x;
        pvsCur->gbox.inf.y = gbox->inf.y;
        pvsCur->gbox.inf.z = gbox->inf.z;
        pvsCur->gbox.sup.x = gbox->sup.x;
        pvsCur->gbox.sup.y = gbox->sup.y;
        pvsCur->gbox.sup.z = gbox->sup.z;

        pvsCur->diagonal = PVSFindDiagonalSizeOfBBox(&pvsCur->sbox);
    }
    else
    {
        /* It's a plane */
        RpPlaneSector      *pspPlane = (RpPlaneSector *) spSector;

        PVSDivideBox(box, pspPlane->value, &leftbox, &rightbox, spSector->type);
        /* Left then right */

        PVSCalculateSectorBounds(pspPlane->leftSubTree, (const RwBBox *)(&leftbox));
        PVSCalculateSectorBounds(pspPlane->rightSubTree, (const RwBBox *)(&rightbox));
    }
    RWRETURN(spSector);
}

static RpWorldSector *
PVSCopyAndTagSectors(RpWorldSector * sector, void * data __RWUNUSED__)
{
    RpPVS              *pvsCur = PVSFROMWORLDSECTOR(sector);
    RwV3d               v;
    RwReal              max;
    RwV3d               diagshift;
    RwBBox              box;

    RWFUNCTION(RWSTRING("PVSCopyAndTagSectors"));
    RWASSERT(rpPVSGlobals.World);

    /* Use sbox because we're sampling from here */
    box = pvsCur->sbox;

    pvsCur->centre.x =
        (box.sup.x +
         box.inf.x) / 2.0f;
    pvsCur->centre.y =
        (box.sup.y +
         box.inf.y) / 2.0f;
    pvsCur->centre.z =
        (box.sup.z +
         box.inf.z) / 2.0f;

    RwV3dSub(&v, &box.sup, &box.inf);

    /* Now determine significance of axes of the box
     * i.e. The number of samples taken along each axis
     * is based on the value in axessig, which will be 0 for the most
     * significant axes, and 0 or less for the other depending on their length...
     */
    diagshift.x = box.sup.x - box.inf.x;
    diagshift.y = box.sup.y - box.inf.y;
    diagshift.z = box.sup.z - box.inf.z;

    max = rpPVSMAX(diagshift.x, diagshift.y);
    max = rpPVSMAX(diagshift.z, max);

    diagshift.x/=max;
    diagshift.y/=max;
    diagshift.z/=max;

    pvsCur->axessig[0] =
        -RwInt32FromRealMacro((float)
                              ((((RwLog((1.0f) / diagshift.x)) /
                                 RwLog(2.0f)))));
    pvsCur->axessig[1] =
        -RwInt32FromRealMacro((float)
                              ((((RwLog((1.0f) / diagshift.y)) /
                                 RwLog(2.0f)))));
    pvsCur->axessig[2] =
        -RwInt32FromRealMacro((float)
                              ((((RwLog((1.0f) / diagshift.z)) /
                                 RwLog(2.0f)))));

    RWRETURN(sector);
}

static void
PVSScalePolygon(_rpPVSPolyRecord * t, RwReal scale)
{
    _rpPVSPolyPtr             temp;

    RWFUNCTION(RWSTRING("PVSScalePolygon"));

    temp = t->geom;
    while (temp != NULL)
    {
        temp->v.x =
            ((temp->v.x - t->centroid.x) * scale) + t->centroid.x;
        temp->v.y =
            ((temp->v.y - t->centroid.y) * scale) + t->centroid.y;
        temp->v.z =
            ((temp->v.z - t->centroid.z) * scale) + t->centroid.z;
        temp = temp->next;
    }

    RWRETURNVOID();
}



static void
PVSGenerateShadowPlanes(_rpPVSPolyRecord * poly)
{
    RwReal              scalebase = 2.0f;
    RwReal              scalepercent;
    RwReal              dist;
    RwV3d result;
    /* percentage increase in size of shadow caster -
     * avoids 'cracks' and 'light escape' */
    _rpPVSPolyPtr             ptemp, ptempn;
    RwV3d viewPoint = rpPVSGlobals.ViewPos;


    RWFUNCTION(RWSTRING("PVSGenerateShadowPlanes"));

    RwV3dSub(&result, &viewPoint, &poly->centroid);
    dist = RwV3dLength(&result);

    scalepercent = (dist / poly->radius) * scalebase;

    if (scalepercent < scalebase) scalepercent = scalebase;

    PVSScalePolygon(poly, 1.0f + (scalepercent / 100.0f));
    poly->hasbeenclipper = FALSE;
    ptemp = poly->geom;
    while (ptemp != NULL)      /* for each vertex */
    {
        ptempn = ptemp->next;
        if (ptempn == NULL)
            ptempn = poly->geom;
        /* non-memory time waster */
        PVSGeneratePlaneEquationFromThreePoints(&ptemp->shadowPlane,
                                                &viewPoint,
                                                &(ptemp->v),
                                                &(ptempn->v));
        ptemp = ptemp->next;
    }

    PVSScalePolygon(poly, 1.0f / (1.0f + (scalepercent / 100.0f)));

    RWRETURNVOID();
}



static void
PVSWeilerAtherton(_rpPVSPolyListPtr * Tlist, RpPVS * pvsCur)
{
    _rpPVSPolyListPtr         head, temp;

    RWFUNCTION(RWSTRING("PVSWeilerAtherton"));
    RWASSERT(Tlist);
    RWASSERT(*Tlist);

    /* set to uninitialized the shadows... */
    temp = *Tlist;
    while (temp != NULL)       /* for each polygon */
    {
        temp->data.parent->data.coneRadius = -1;
        /*PVSGenerateShadowPlanes(&temp->data);*/
        temp = temp->next;
    }

    head = *Tlist;
    /* Note, we may assume that the clipper is visible since sorting might
     * have failed due to overlapping sectors.  But we can assume very first
     * sector is visible, since we are in it
     */
    PVSVISMAPSETSECTOR(pvsCur->vismap, head->data.home);

    while (head != NULL)       /* for each polygon */
    {
        /* do PVSWeilerAtherton subdivision
         * with head one tlist (containing all polygons) */
        if (!head->data.parent->data.hasbeenclipper)
        {
            RWASSERT(head->data.home >= 0);
            RWASSERT(pvsCur);
            RWASSERT(pvsCur->vismap);

            if (!head->data.parent->data.translucent)
            {
                if (head->data.parent->data.coneRadius == -1)
                {
                    /* Assign the shadow planes... */
                    PVSGenerateShadowCone(&head->data.parent->data);
                    PVSGenerateShadowPlanes(&head->data.parent->data);
                }
                PVSWASubdivide(&(head->data.parent->data), Tlist);
            }
        }
        head = head->next;
    }

    RWRETURNVOID();
}

static void
PVSInitialize(void)
{
    RwInt32             id = 0;
    RpPVSCache         *pvsCache =
        PVSCACHEFROMWORLD(rpPVSGlobals.World);

    RWFUNCTION(RWSTRING("PVSInitialize"));

    /* Create and classify the data from each sector */
    RpWorldForAllWorldSectors(rpPVSGlobals.World, PVSCopyAndTag, &id);

    pvsCache->formatted = TRUE;

    RWRETURNVOID();
}

static void
PVSInitializeSectors(void)
{
    RpPVSCache         *PVSCache =
        PVSCACHEFROMWORLD(rpPVSGlobals.World);

    RWFUNCTION(RWSTRING("PVSInitializeSectors"));
    RWASSERT(rpPVSGlobals.World);

    PVSCache->NumWorldSectors =
        _rpPVSWorldGetNumWorldSectors(rpPVSGlobals.World);

    rpPVSGlobals.diagonal = PVSFindDiagonalSizeOfBBox(&rpPVSGlobals.World->boundingBox);

    PVSCalculateSectorBounds((rpPVSGlobals.World)->rootSector,
                                    RpWorldGetBBox(rpPVSGlobals.World));

    /* Create and classify the data from each sector */
    RpWorldForAllWorldSectors(rpPVSGlobals.World,
                              PVSCopyAndTagSectors, NULL);

    RWRETURNVOID();
}

static void
PVSDeinitialize(RpWorld * wpWorld)
{
    RpPVSCache         *pvsCache =
        PVSCACHEFROMWORLD(wpWorld);
    _rpPVSPolyListPtr         temp = pvsCache->polygons;

    RWFUNCTION(RWSTRING("PVSDeinitialize"));

    if (!pvsCache->formatted) RWRETURNVOID();

    while (temp != NULL)
    {
        _rpPVSDestroyPoly(&(temp->data.geom));
        temp = temp->next;
    }
#if (!visual)
    _rpPVSDestroyPolyList(&pvsCache->polygons);
#endif
    pvsCache->formatted = FALSE;

    RWRETURNVOID();
}

/*
 * Creates PVS data for a given ViewPoint and
 * assigns the new data to the current sector
 */
_rpPVSPolyListPtr copyList = (_rpPVSPolyList *) NULL;

static void
PVSCreateViewPointPVS(RwV3d * ViewPoint, RpPVS * pvsCur)
{
    _rpPVSPolyListPtr         temp;
    _rpPVSPolyListPtr         copyListTail = (_rpPVSPolyList *) NULL;
    RpPVSCache         *pvsCache =
        PVSCACHEFROMWORLD(rpPVSGlobals.World);

    RWFUNCTION(RWSTRING("PVSCreateViewPointPVS"));
    RWASSERT(rpPVSGlobals.World);
    RWASSERT((pvsCache->formatted && pvsCache->polygons!=NULL) ||
            (!pvsCache->formatted && pvsCache->polygons==NULL));

#if (!visual)
    if (copyList!=NULL) _rpPVSDestroyPolyList(&copyList); /* incase we kept it for debug reasons */
#endif

    rpPVSGlobals.ViewPos.x = ViewPoint->x;
    rpPVSGlobals.ViewPos.y = ViewPoint->y;
    rpPVSGlobals.ViewPos.z = ViewPoint->z;

    /* Prioritize by sector distance and polygon distance */
    PVSAssignPriorities(&pvsCache->polygons, ViewPoint);

    /* Sort list by sector */
    PVSAssignJumpInfo(&pvsCache->polygons);
    PVSSortSectors(&pvsCache->polygons);

    /* Sort list within each sector */
    PVSAssignJumpInfo(&pvsCache->polygons);
    PVSSortSubSectors(&pvsCache->polygons);

    /* Copy data, since it will be destroyed 'en route'
     * and we need to keep original */
    copyList = (_rpPVSPolyList *) NULL;
    copyListTail = (_rpPVSPolyList *) NULL;
    temp = pvsCache->polygons;
    while (temp != NULL)
    {
        _rpPVSaddendII(&copyListTail, &(temp->data));
        if (copyList == NULL)
            copyList = copyListTail;

        temp->data.hasbeenclipper = FALSE;
        temp = temp->next;

    }

    /* Remove backfaces since they are invisible -
     * no need to cull & cannot be cullers */
    PVSRemoveBackFaces(&copyList, ViewPoint);

    PVSAssignJumpInfo(&copyList);

    /* Do the PVS using WA algorithm */
    PVSWeilerAtherton(&copyList, pvsCur);

    /* Remove the fragments on the visible (copy) list and set visibility
     * (all other frags should have been deleted) */
    temp = copyList;
    while (temp != NULL)       /* for each geom in vis list */
    {
        if (!PVSVISMAPGETSECTOR(rpPVSGlobals.CurrPVS->vismap, temp->data.home))
        {
            if ((temp->data.original) ||
               (PVSAreaOfPolygon(temp->data.geom) > (rpPVSGlobals.diagonal * rpPVSTHOU)))
            {
                /* if remainder is not a tiny fragment, tag its sector as visible */
                PVSVISMAPSETSECTOR(rpPVSGlobals.CurrPVS->vismap, temp->data.home);
            }
        }
#if (!visual)
        if (!temp->data.original)
            _rpPVSDestroyPoly(&(temp->data.geom));
#endif
        temp = temp->next;
    }
#if (!visual)
    _rpPVSDestroyPolyList(&copyList);
#endif

    RWRETURNVOID();
}

typedef struct
{
    RwReal              dist;
    RwBool              facing;
}
GridCollTri;

/*
 * Function:   TrianglePointNearestPoint
 * Purpose:    given a point and a description of a triangle,
 *             return the point within the triangle that is closest to
 *             that point
 * On entry:   triangle's vertex array,
 *             triangle's normal,
 *             reference point
 * On exit:    return value is the distance from the
 *             triangle to the supplied reference point,
 *             vpPt now holds the near point of the
 *             triangle to the input point
 */
#define CONSTREALASINT(r)   ( ((const RwSplitBits *)&(r))->nInt )
#define REALSIGNSNEQ(a, b)  ( (CONSTREALASINT(a) ^ CONSTREALASINT(b)) < 0 )

#define TRIAREA(vpA, vpB, vpC, _X, _Y)                                   \
    ( (((vpB)->_X) - ((vpA)->_X)) * (((vpC)->_Y) - ((vpA)->_Y)) -        \
      (((vpB)->_Y) - ((vpA)->_Y)) * (((vpC)->_X) - ((vpA)->_X)) )

#define RWSNAPEDGE(_rResult, _A, _B, _X, _Y, _Z)                         \
MACRO_START                                                              \
{                                                                        \
    RwV3d               vEdge;                                           \
                                                                         \
    vEdge._X = vpaVertices[_B]->_X -  vpaVertices[_A]->_X;               \
    vEdge._Y = vpaVertices[_B]->_Y -  vpaVertices[_A]->_Y;               \
                                                                         \
    MACRO_START                                                          \
    {                                                                    \
        const RwReal        rSubtend =                                   \
            ( vEdge._X * ((vProj._Y) - (vpaVertices[_A]->_Y)) -          \
              vEdge._Y * ((vProj._X) - (vpaVertices[_A]->_X)) );         \
        const RwBool        outsideEdge =                                \
            REALSIGNSNEQ(rSubtend, rTriangle);                           \
                                                                         \
        if (outsideEdge)                                                 \
        {                                                                \
            RwReal              mu;                                      \
            RwV3d               vCandidate;                              \
                                                                         \
            vEdge._Z = vpaVertices[_B]->_Z -  vpaVertices[_A]->_Z;       \
                                                                         \
            mu = RwV3dDotProduct(&vProj, &vEdge) -                       \
                 RwV3dDotProduct(vpaVertices[_A], &vEdge);               \
                                                                         \
            if (mu <= 0)                                                 \
            {                                                            \
                vCandidate = *vpaVertices[_A];                           \
            }                                                            \
            else                                                         \
            {                                                            \
                RwReal       denom = RwV3dDotProduct(&vEdge, &vEdge);    \
                                                                         \
                bEdgeInternal = ((0 < denom) && (mu < denom));           \
                if (bEdgeInternal)                                       \
                {                                                        \
                    mu /= denom;                                         \
                    RwV3dScale(&vCandidate, &vEdge, mu);                 \
                    RwV3dAdd(&vCandidate, &vCandidate, vpaVertices[_A]); \
                }                                                        \
                else                                                     \
                {                                                        \
                    vCandidate = *vpaVertices[_B];                       \
                }                                                        \
            }                                                            \
                                                                         \
            RwV3dSub(&vEdge, &vPtAsPassed, &vCandidate);                 \
            rDist2 = RwV3dDotProduct(&vEdge, &vEdge);                    \
                                                                         \
            if ((!bSnapped) || ((_rResult) > rDist2))                    \
            {                                                            \
                *vpPt = vCandidate;                                      \
                (_rResult) = rDist2;                                     \
            }                                                            \
        }                                                                \
        bSnapped |= outsideEdge;                                         \
    }                                                                    \
    MACRO_STOP;                                                          \
}                                                                        \
MACRO_STOP

#define RWPLANEPROCESS(_result, _X, _Y, _Z)                              \
MACRO_START                                                              \
{                                                                        \
    RwBool              bEdgeInternal = 0;                               \
    RwReal              rTriangle = TRIAREA(vpaVertices[0],              \
                                            vpaVertices[1],              \
                                            vpaVertices[2],              \
                                            _X, _Y);                     \
                                                                         \
    RWSNAPEDGE(_result, 1, 2, _X, _Y, _Z);                               \
    if (!bEdgeInternal)                                                  \
    {                                                                    \
        RWSNAPEDGE(_result, 2, 0, _X, _Y, _Z);                           \
        if (!bEdgeInternal)                                              \
        {                                                                \
            RWSNAPEDGE(_result, 0, 1, _X, _Y, _Z);                       \
        }                                                                \
    }                                                                    \
}                                                                        \
MACRO_STOP

static RwReal
TrianglePointNearestPoint(RwV3d * vpaVertices[3],
                          RwV3d * vpNormal, RwV3d * vpPt)
{
    RwReal              rResult = 0.0f;
    RwV3d               vPtAsPassed;
    RwV3d               vProj;
    RwReal              rDistPt2Plane;
    RwBool              bSnapped = 0;
    RwReal              rDist2;

    /* work in 2-D plane of greatest projection */
    const RwInt32       nAbsX =
        CONSTREALASINT(vpNormal->x) & 0x7FFFFFFF;
    const RwInt32       nAbsY =
        CONSTREALASINT(vpNormal->y) & 0x7FFFFFFF;
    const RwInt32       nAbsZ =
        CONSTREALASINT(vpNormal->z) & 0x7FFFFFFF;

    RWFUNCTION(RWSTRING("TrianglePointNearestPoint"));

    vPtAsPassed = *vpPt;

    /* project point onto plane of triangle
     * (cost of using two dot products vs. V3dSub and one is 3 multiplies,
     * but loss of precision is minimized )
     */
    rDistPt2Plane =
        RwV3dDotProduct(vpaVertices[0], vpNormal) -
        RwV3dDotProduct(vpPt, vpNormal);

    RwV3dScale(&vProj, vpNormal, rDistPt2Plane);
    RwV3dAdd(&vProj, &vProj, vpPt);

    if (nAbsZ > nAbsY)
    {
        if (nAbsZ > nAbsX)
        {
            RWPLANEPROCESS(rResult, x, y, z);
        }
        else
        {
            RWPLANEPROCESS(rResult, y, z, x);
        }
    }
    else
    {
        if (nAbsY > nAbsX)
        {
            RWPLANEPROCESS(rResult, z, x, y);
        }
        else
        {
            RWPLANEPROCESS(rResult, y, z, x);
        }
    }

    if (!bSnapped)
    {
        *vpPt = vProj;
        rDist2 = rDistPt2Plane * rDistPt2Plane;
        rResult = rDist2;
    }

    RWRETURN(rResult);
}

static RpCollisionTriangle *
colltest(RpIntersection * is,
         RpWorldSector * sector __RWUNUSED__,
         RpCollisionTriangle * collPlane,
         RwReal distance __RWUNUSED__, void *pData)
{
    GridCollTri        *colltri = (GridCollTri *) pData;
    RwReal              dist;
    RwV3d               pos;

    RWFUNCTION(RWSTRING("colltest"));

    /* find nearest point of polygon to collision sphere centre */
    pos = is->t.sphere.center;
    dist =
        TrianglePointNearestPoint(collPlane->vertices,
                                  &collPlane->normal, &pos);
    if (dist < colltri->dist)
    {
        RwReal              cosAngle;
        RwV3d               toward;

        /* is it facing center? */
        RwV3dSub(&toward, collPlane->vertices[0], &is->t.sphere.center);
        RwV3dNormalize(&toward, &toward);
        cosAngle = RwV3dDotProduct(&toward, &collPlane->normal);

        /* ignore oblique front faces (THIS IS CRITICAL TO CORRECT PVS!!!)... */
        if (!((cosAngle < 0.0f) &&
            (cosAngle > -0.05f)))
        {
            colltri->facing = cosAngle < 0.0f;

            /* bias against facing polygons (back facing should 'win') */
            if (colltri->facing)
            {
                dist *= 1.005f;
            }

            colltri->dist = dist;
        }
    }

    RWRETURN(collPlane);
}

static RwBool
inouttest(RwV3d * pos, RwReal radius)
{
    RpIntersection      isSphere;
    GridCollTri         neartri = {0.0f, FALSE};

    RWFUNCTION(RWSTRING("inouttest"));

    if (radius > rpPVSGlobals.diagonal)
    {
        RWRETURN(FALSE);
    }

    isSphere.type = rpINTERSECTSPHERE;
    isSphere.t.sphere.center = *pos;
    isSphere.t.sphere.radius = radius;
    neartri.dist = radius * radius * 2.0f; /* See note below */

    RpCollisionWorldForAllIntersections(rpPVSGlobals.World, &isSphere,
                                        colltest, &neartri);

    /* NB! Precision may have changed due to some extraordinary bug,
       this is why 2 (could be any value (> 1)) is multiplied to the neartri.dist,
       to make sure the following if is false if no intersection was found
     */
    if (neartri.dist < radius * radius)
    {
        RWRETURN(neartri.facing);
    }
    else
    {
        /* enlarge the search */
        RWRETURN(inouttest(pos, radius * 2.0f));
    }
}

static RpCollisionTriangle *
PVSSphereCollision(RpIntersection * is __RWUNUSED__,
                   RpWorldSector * sector __RWUNUSED__,
                   RpCollisionTriangle * collPlane __RWUNUSED__,
                   RwReal distance __RWUNUSED__, void *pData)
{
    RwBool             *hitcount = (RwBool *) pData;

    RWFUNCTION(RWSTRING("PVSSphereCollision"));

    *hitcount = TRUE;

    RWRETURN((RpCollisionTriangle *) NULL);
}

/**
 * \ingroup rppvs
 * \ref RpPVSSetCollisionDetection
 * is used to turn collision detection on or off.  With collision detection on,
 * only sample points within the geometry of a sector will be chosen.
 *
 * \param collis  A boolean to specify whether collision should be on.
 *
 * \return Result of collision detection, i.e. collis.
 *
 * \see RpPVSConstruct
 * \see RpPVSSamplePOV
 */
RwBool
RpPVSSetCollisionDetection(RwBool collis)
{
    RWAPIFUNCTION(RWSTRING("RpPVSSetCollisionDetection"));

    rpPVSGlobals.collis = collis;

    RWRETURN(rpPVSGlobals.collis);
}

/**
 * \ingroup rppvs
 * \ref RpPVSSetBackFaceCulling
 * is used to turn back face culling on or off.  By default, it is TRUE.
 *
 * IMPORTANT NOTES:
 *
 * \li It is ONLY recommended to set this to FALSE, if your world is constructed
 * from polygons that are visible from both sides - otherwise unexpected results
 * are likely to occur.
 *
 * \li This flag must be set BEFORE any calls to an RpPVS function (but after
 * RpPVSPluginAttach), since it affects initialization of the geometry in the world.
 * If an RpPVS function has already been used, RpPVSDestroy must be called
 * before this function call can take effect.
 *
 * \li If, in the world, the viewer is allowed to wander 'outside' the geometry,
 * which in a world made with double sided faces may take on a different meaning,
 * then \ref RpPVSSetCollisionDetection must be called with
 * the FALSE parameter - otherwise some legitimate sample points may be tagged as
 * invalid.
 *
 * \param bfc  A boolean to specify whether backface culling should be on.
 *
 * \return The culling flag.
 *
 * \see RpPVSConstruct
 * \see RpPVSSamplePOV
 * \see RpPVSDestroy
 * \see RpPVSPluginAttach
 */
RwBool
RpPVSSetBackFaceCulling(RwBool bfc)
{
    RpPVSCache         *pvsCache;

    RWAPIFUNCTION(RWSTRING("RpPVSSetBackFaceCulling"));

    RWASSERT(rpPVSGlobals.World);
    pvsCache = PVSCACHEFROMWORLD(rpPVSGlobals.World);

    RWASSERT(!pvsCache->formatted);



    rpPVSGlobals.bfc = bfc;

    RWRETURN(rpPVSGlobals.bfc);
}

/**
 * \ingroup rppvs
 * \ref RpPVSGeneric
 * is the RenderWare supplied function for sampling within a world
 * sector. The samples are non-uniformly distributed in a regular grid.
 *
 * This requires the private data to be a pointer to a RwReal, in the range
 * 0.01 to 1.0, to specify the sampling distance between two points. The
 * number is the fraction of the major axis length of the world's bounding box.
 *
 * The include file rppvs.h and the library file rppvs.lib are required
 * to use this function. The library file rpworld.lib is also required.
 *
 * \param spSect  A pointer to the RpWorldSector being sampled.
 * \param box  The bounding box of the sector.
 * \param pData  A pointer to private data for the sampling function.
 *
 * \return RpWorldSector if successful or NULL if there is an error.
 *
 * \see RpPVSSetWorldSectorVisibility
 * \see RpPVSConstruct
 * \see RpPVSSamplePOV
 */
RpWorldSector      *
RpPVSGeneric(RpWorldSector * spSect, const RwBBox * box,
             void *pData)
{
    RpPVS              *pvsCur = PVSFROMWORLDSECTOR(spSect);
    RpPVSCache         *pvsCache = PVSCACHEFROMWORLD(rpPVSGlobals.World);
    RwV3d               samp;       /* fraction along axes for sample */
    RwV3d               ViewPoint;  /* Viewpoint defined by fraction along axes */
    RwReal              fit = 1.99f; /* to align samples 'optimally' */
    RwInt32             sd = RwInt32FromRealMacro((1.0f / 
                                                  (*((RwReal *) pData)))+
                                                  rpPVSSMALL);
                        /* recip of sampling density */
    RwInt32             expected = (sd+1)*(sd+1)*(sd+1); /* expected number of samples */
    RwInt32             succeeded = 0; /* number of samples succeeded */
    RwInt32             depth = 1; /* Sampling distribution depth */
    RwInt32             maxdepth = 5; /* max distribution/saturation */
    RwInt32             absolutemaxdepth = 8; /* for enhancing */
    RwV3d               scatter; /* integer sampling rate */
    RwV3i               recscatter; /* reciprocal of integer sampling rate */
    RwV3i               inc;    /* current stage the sampling algorithm has got to */
    RwV3i               lindex; /* Last stage the samping algorithm got to */
    RwInt32             key;    /* key that represents the sampling done in the past */

    RWAPIFUNCTION(RWSTRING("RpPVSGeneric"));

    RWASSERT(spSect);

    /* Might need lots of candidate sample point for a sector that is largely empty, but
     * but has a little geometry inside it, but for an empty sector, the point are likely
     * to be all invalid or all valid, so we set the search depth differently
     */

    if (spSect->numTriangles == 0)
    {
        maxdepth = 4; /* max distribution/saturation */
        absolutemaxdepth = 7;
    }

    /* Let's see where we got to last time we sampled (if at all) */
    key = pvsCur->sampleKey;
    depth = (key & 255) + 1;
    maxdepth = rpPVSMAX(rpPVSMIN(absolutemaxdepth, depth + 1),maxdepth); /* increase max depth if user is trying to improve results */

    lindex.z = ((key>>=8) & 255);
    lindex.y = ((key>>=8) & 255);
    lindex.x = ((key>>=8) & 255);

    while ((succeeded < expected) && (depth<=maxdepth))
    {
        /* For each independent (progressive) mesh */
        recscatter.x = (1 << (rpPVSMAX(depth + pvsCur->axessig[0],1)-1)); /* 2^RHS */
        recscatter.y = (1 << (rpPVSMAX(depth + pvsCur->axessig[1],1)-1)); /* 2^RHS */
        recscatter.z = (1 << (rpPVSMAX(depth + pvsCur->axessig[2],1)-1)); /* 2^RHS */

        scatter.x = 1.0f / (RwReal)recscatter.x;
        scatter.y = 1.0f / (RwReal)recscatter.y;
        scatter.z = 1.0f / (RwReal)recscatter.z;

        for (inc.x=lindex.x; inc.x<recscatter.x; inc.x++)
        {
            /* For x-axis samples */
            samp.x = (scatter.x * 0.5f) + (inc.x * scatter.x);

            ViewPoint.x =
                box->inf.x * samp.x +
                box->sup.x * (1.0f - samp.x);/* low-to-hi */
            ViewPoint.x = ((ViewPoint.x - pvsCur->centre.x) * fit) + pvsCur->centre.x;

            if ((ViewPoint.x > box->sup.x) ||
                (ViewPoint.x < box->inf.x)) continue;

            for (inc.y=lindex.y; inc.y<recscatter.y; inc.y++)
            {
                /* For y-axis samples */
                samp.y = (scatter.y * 0.5f) + (inc.y * scatter.y);

                ViewPoint.y =
                    box->sup.y * samp.y +
                    box->inf.y * (1.0f - samp.y); /* hi-to-low */
                ViewPoint.y = ((ViewPoint.y - pvsCur->centre.y) * fit) + pvsCur->centre.y;

                if ((ViewPoint.y > box->sup.y) ||
                    (ViewPoint.y < box->inf.y)) continue;

                for (inc.z=lindex.z; inc.z<recscatter.z; inc.z++)
                {
                    /* For z-axis samples */
                    samp.z = (scatter.y * 0.5f) + (inc.z * scatter.z);

                    ViewPoint.z =
                        box->inf.z * samp.z +
                        box->sup.z * (1.0f -
                                                   samp.z); /* low-to-hi */
                    ViewPoint.z = ((ViewPoint.z - pvsCur->centre.z) * fit) + pvsCur->centre.z;

                    if ((ViewPoint.z > box->sup.z) ||
                        (ViewPoint.z < box->inf.z)) continue;

                    /* set curr sector (changes insector) */
                    RpPVSSetViewPosition(rpPVSGlobals.World, &ViewPoint);


                    if (rpPVSGlobals.InSector == pvsCur->sectorID)
                    {

                        if (RpPVSSamplePOV(&ViewPoint, rpPVSGlobals.collis))
                        {
                            succeeded++;
                            rpPVSGlobals.progress_count++;

                            /* Progress */
                            if (pvsCache->progressCallBack)
                            {
                                RwReal              progress;

                                progress =
                                    (RwReal) (rpPVSGlobals.
                                              progress_count) /
                                    (RwReal) (pvsCache->viscount *
                                              pvsCache->
                                              NumWorldSectors) *
                                    (RwReal) (100.0f);

                                if (!(pvsCache->progressCallBack
                                      (rpPVSPROGRESSUPDATE, progress)))
                                {
                                    RWRETURN((RpWorldSector *) NULL);
                                }
                            }
                        }
                    }
                    if (succeeded >= expected) break;
                }/* endfor z */
                if (succeeded >= expected) break;
            }/* endfor y */
            if (succeeded >= expected) break;
        }/* endfor x */

        /* Remember what rate we completed our samples at... */
        pvsCur->sampleKey = inc.x;
        pvsCur->sampleKey = (pvsCur->sampleKey << 8) + inc.y;
        pvsCur->sampleKey = (pvsCur->sampleKey << 8) + inc.z;
        pvsCur->sampleKey = (pvsCur->sampleKey << 8) + depth-1;

        depth++;
        lindex.x = 0;
        lindex.y = 0;
        lindex.z = 0;
    }/* endwhile */
    rpPVSGlobals.progress_count += (expected-succeeded);

    RWRETURN(spSect);
}

/*
 * Gets the current outer sector from the current viewpoint
 */
static RpWorldSector *
GetEntireSector(RpWorldSector * sector, void *data)
{
    RpPVS              *PVSSector = PVSFROMWORLDSECTOR(sector);
    RwInt32            *index = (RwInt32 *) (data);

    RWFUNCTION(RWSTRING("GetEntireSector"));

    /* Use sbox, because we want to locate ourself */
    if ((rpPVSGlobals.InSector == -1) || /* Set default to sector zero */
        ((rpPVSGlobals.ViewPos.x <= PVSSector->sbox.sup.x &&
          rpPVSGlobals.ViewPos.x >= PVSSector->sbox.inf.x) &&
         (rpPVSGlobals.ViewPos.y <= PVSSector->sbox.sup.y &&
          rpPVSGlobals.ViewPos.y >= PVSSector->sbox.inf.y) &&
         (rpPVSGlobals.ViewPos.z <= PVSSector->sbox.sup.z &&
          rpPVSGlobals.ViewPos.z >= PVSSector->sbox.inf.z)))
    {
        rpPVSGlobals.InSector = *index;
        rpPVSGlobals.CurrPVS = PVSFROMWORLDSECTOR(sector);
    }
    PVSFROMWORLDSECTOR(sector)->sectailpoly = (_rpPVSPolyList *) NULL;

    (*index)++;

    RWRETURN(sector);
}

/**
 * \ingroup rppvs
 * \ref RpPVSSetViewPosition
 * is used to set the viewing sector for subsequent PVS culling.
 * NB: This is the alternative function to \ref RpPVSSetViewSector, which
 * is overridden by this function call.
 * It selects the appropriate visibility map for PVS culling. This function
 * must be called before any render function otherwise incorrect culling
 * will occur.
 *
 * RpPVSSetViewSector is typically used immediately prior to a frame
 * render by setting the view position equal to that of the current
 * camera.
 *
 * The PVS plugin must be attached before using this function.
 *
 * The include file rppvs.h and the library file rppvs.lib are required
 * to use this function. The library file rpworld.lib is also required.
 *
 * \param wpWorld  A pointer to the RpWorld containing the PVS data.
 * \param pos  A pointer to a RwV3d which specifies
 *             the current viewing position.
 *
 * \return RpWorldSector if sucessful, NULL otherwise.
 *
 * \see RpPVSAtomicVisible
 * \see RpPVSDestroy
 * \see RpPVSGetProgressCallBack
 * \see RpPVSHook
 * \see RpPVSPluginAttach
 * \see RpPVSQuery
 * \see RpPVSSetProgressCallBack
 * \see RpPVSStatisticsGet
 * \see RpPVSUnhook
 * \see RpPVSWorldSectorVisible
 */
RpWorldSector            *
RpPVSSetViewPosition(RpWorld * wpWorld, RwV3d * pos)
{
    RwInt32             id = 0;
    RpPVSCache         *PVSCache;

    RWAPIFUNCTION(RWSTRING("RpPVSSetViewPosition"));

    RWASSERT(wpWorld);
    RWASSERT(pos);

    rpPVSGlobals.World = wpWorld;
    PVSCache = PVSCACHEFROMWORLD(rpPVSGlobals.World);

    rpPVSGlobals.ViewPos = *pos;
    rpPVSGlobals.InSector = -1;
    rpPVSGlobals.CurrPVS = NULL;

    id = 0;
    if (rpPVSGlobals.InSector == -1)
        RpWorldForAllWorldSectors(wpWorld, GetEntireSector, &id);

    RWASSERT(rpPVSGlobals.InSector>=0 &&
        rpPVSGlobals.InSector<PVSCache->NumWorldSectors);

    RWRETURN(WORLDSECTORFROMPVS(rpPVSGlobals.CurrPVS));
}

/**
 * \ingroup rppvs
 * \ref RpPVSSetViewSector
 * is used to set the viewing sector for subsequent PVS culling.
 * NB: This is the alternative function to \ref RpPVSSetViewPosition, which
 * is overridden by this function call.
 * It selects the appropriate visibility map for PVS culling. This function
 * must be called before any render function otherwise incorrect culling
 * will occur.
 *
 * RpPVSSetViewSector is typically used immediately prior to a frame
 * render by setting the view position equal to that of the current
 * camera.
 *
 * The PVS plugin must be attached before using this function.
 *
 * The include file rppvs.h and the library file rppvs.lib are required
 * to use this function. The library file rpworld.lib is also required.
 *
 * \param wpWorld  A pointer to the RpWorld containing the PVS data.
 * \param spSect   A pointer to the sector which specifies
 *             the current viewing position.
 *
 * \return RpWorldSector if sucessful, NULL otherwise.
 *
 * \see RpPVSAtomicVisible
 * \see RpPVSDestroy
 * \see RpPVSGetProgressCallBack
 * \see RpPVSHook
 * \see RpPVSPluginAttach
 * \see RpPVSQuery
 * \see RpPVSSetProgressCallBack
 * \see RpPVSStatisticsGet
 * \see RpPVSUnhook
 * \see RpPVSWorldSectorVisible
 */
RpWorldSector            *
RpPVSSetViewSector(RpWorld * wpWorld, RpWorldSector * spSect)
{
    RpPVS   *pp = PVSFROMWORLDSECTOR(spSect);

    RWAPIFUNCTION(RWSTRING("RpPVSSetViewSector"));

    RWASSERT(wpWorld);
    RWASSERT(spSect);

    rpPVSGlobals.World = wpWorld;
    rpPVSGlobals.InSector = pp->sectorID;
    rpPVSGlobals.CurrPVS = PVSFROMWORLDSECTOR(spSect);

    RWASSERT(rpPVSGlobals.InSector>=0 &&
        rpPVSGlobals.InSector <
        PVSCACHEFROMWORLD(rpPVSGlobals.World)->NumWorldSectors);

    RWRETURN(WORLDSECTORFROMPVS(rpPVSGlobals.CurrPVS));
}

/**
 * \ingroup rppvs
 * \ref RpPVSWorldSectorVisible
 * is used to determine if the sector is visible using
 * the current visibility map.
 *
 * The include file rppvs.h and the library file rppvs.lib are
 * required to use this function. The library file rpworld.lib is also
 * required.
 *
 * \param spSect  A pointer to a RpWorldSector to be determined.
 *
 * \return TRUE if visible, FALSE otherwise.
 *
 * \see RpPVSAtomicVisible
 * \see RpPVSDestroy
 * \see RpPVSGetProgressCallBack
 * \see RpPVSHook
 * \see RpPVSPluginAttach
 * \see RpPVSQuery
 * \see RpPVSSetProgressCallBack
 * \see RpPVSSetViewPosition
 * \see RpPVSStatisticsGet
 * \see RpPVSUnhook
 */
RwBool
RpPVSWorldSectorVisible(RpWorldSector * spSect)
{
    RpPVS              *pvsCur = rpPVSGlobals.CurrPVS;
    RpPVS              *ppOther = PVSFROMWORLDSECTOR(spSect);
    RwBool              visible;

    RWAPIFUNCTION(RWSTRING("RpPVSWorldSectorVisible"));

    visible = (PVSVISMAPGETSECTOR(pvsCur->vismap, ppOther->sectorID)) != 0;

    RWRETURN(visible);
}

/**
 * \ingroup rppvs
 * \ref RpPVSSetWorldSectorVisibility
 * is to mark the given world sector as visible (or not)
 * from the current sector (set by calling \ref RpPVSSetViewPosition).
 *
 * The include file rppvs.h and the library file rppvs.lib are
 * required to use this function. The library file rpworld.lib is also
 * required.
 *
 * \param spSect  A pointer to a RpWorldSector.
 * \param visible  Flag to set the visibility.
 *
 * \return RpWorldSector if sucessful, NULL otherwise.
 *
 * \see RpPVSGeneric
 * \see RpPVSConstruct
 * \see RpPVSSamplePOV
 * \see RpPVSSetWorldSectorPairedVisibility
 */
RpWorldSector      *
RpPVSSetWorldSectorVisibility(RpWorldSector * spSect, RwBool visible)
{
    RpPVS              *pvsCur = rpPVSGlobals.CurrPVS;
    RpPVS              *ppOther = PVSFROMWORLDSECTOR(spSect);

    RWAPIFUNCTION(RWSTRING("RpPVSSetWorldSectorVisibility"));
    RWASSERT(rpPVSGlobals.CurrPVS != NULL); /* Call RpPVSSetViewPosition first */
    RWASSERT(spSect);
    RWASSERT(rpPVSGlobals.World);

    PVSVISMAPSETSECTOR(pvsCur->vismap, ppOther->sectorID); /* make sure it is set */
    if (!visible)
    {
        PVSVISMAPUNSETSECTOR(pvsCur->vismap, ppOther->sectorID); /* set it to zero */
    }

    RWRETURN(spSect);
}

/**
 * \ingroup rppvs
 * \ref RpPVSSetWorldSectorPairedVisibility
 * is to mark the first given world sector with the required visibility from
 * the second given world sector, and vice versa if the mutal flag is set
 * to TRUE.
 *
 * The include file rppvs.h and the library file rppvs.lib are
 * required to use this function. The library file rpworld.lib is also
 * required.
 *
 * \param spSectA  A pointer to a RpWorldSector.
 * \param spSectB  A pointer to a RpWorldSector.
 * \param visible  Flag to set the visibility.
 * \param mutual   Flag to say if visibility is to be mutual.
 *
 * \return RpWorldSector if sucessful, NULL otherwise.
 *
 * \see RpPVSGeneric
 * \see RpPVSConstruct
 * \see RpPVSSamplePOV
 * \see RpPVSSetWorldSectorVisibility
 */
RpWorldSector      *
RpPVSSetWorldSectorPairedVisibility(RpWorldSector * spSectA,
                                    RpWorldSector * spSectB,
                                    RwBool visible,
                                    RwBool mutual)
{
    RpPVS              *ppA = PVSFROMWORLDSECTOR(spSectA);
    RpPVS              *ppB = PVSFROMWORLDSECTOR(spSectB);

    RWAPIFUNCTION(RWSTRING("RpPVSSetWorldSectorPairedVisibility"));
    RWASSERT(spSectA);
    RWASSERT(spSectB);
    RWASSERT(rpPVSGlobals.World);

    PVSVISMAPSETSECTOR(ppA->vismap, ppB->sectorID); /* make sure it is set */
    if (!visible)
    {
        PVSVISMAPUNSETSECTOR(ppA->vismap, ppB->sectorID); /* set it to zero */
    }
    if (mutual)
    {
        PVSVISMAPSETSECTOR(ppB->vismap, ppA->sectorID); /* make sure it is set */
        if (!visible)
        {
            PVSVISMAPUNSETSECTOR(ppB->vismap, ppA->sectorID); /* set it to zero */
        }
    }
    RWRETURN(spSectA);
}

static RpWorldSector *
PVSAlloc(RpWorldSector * sect, void * data __RWUNUSED__)
{
    RpPVS              *pvsCur = PVSFROMWORLDSECTOR(sect);
    RpPVSCache         *PVSCache =
        PVSCACHEFROMWORLD(rpPVSGlobals.World);

    RWFUNCTION(RWSTRING("PVSAlloc"));

    /* assign room for vismap */
    if (pvsCur->vismaplength==0)
    {
        pvsCur->sectorID = PVSCache->nextID;

        PVSVISMAPLENGTH(pvsCur->vismaplength, PVSCache->NumWorldSectors);

        pvsCur->vismap =
            (RpPVSVisMap *) RwMalloc(pvsCur->vismaplength *
                                     sizeof(RpPVSVisMap),
                                     rwID_GPVSPLUGIN |
                                     rwMEMHINTDUR_EVENT);

        memset(pvsCur->vismap, 0,
               pvsCur->vismaplength * sizeof(RpPVSVisMap));

        PVSCache->nextID++;
    }/* else already exists */
    RWRETURN(sect);
}

static RpWorldSector *
PVSDispatch(RpWorldSector * worldSector, void *voidbundlecb)
{
    RpPVS              *pvsCur;
    RpWorld            *world;
    const RwBBox       *sectorBBox;
    _RpPVSCallBack     *bundlecb = (_RpPVSCallBack *) voidbundlecb;

    RWFUNCTION(RWSTRING("PVSDispatch"));

    world = rpPVSGlobals.World;
    pvsCur = PVSFROMWORLDSECTOR(worldSector);

    /* Use sbox because we're sampling */
    sectorBBox = (const RwBBox*)(&pvsCur->sbox);

    if (bundlecb->callback(worldSector, sectorBBox, bundlecb->data) !=
        worldSector)
    {
        /* halt PVS generation */
        RpPVSDestroy(world);
        RWRETURN((RpWorldSector *) NULL);
    }

    /* Progress */
    RWRETURN(worldSector);
}

/**
 * \ingroup rppvs
 * \ref RpPVSConstruct
 * is used to create Potential Visibility Set, PVS, for the world. The
 * PVS is created by taking samples within each world sector to build a
 * visibility map.  This map indicates which other world sectors are
 * visible from within its boundary.
 *
 * The \ref RpPVSCallBack callback function is used for sampling within a
 * world sector.  This can be \ref RpPVSGeneric or a user own private
 * function for specific samples distribution.
 *
 * \note If this function is called more than once, the world should have
 * collision data. This is for reasons of efficiency.
 *
 * The include file rppvs.h and the library file rppvs.lib are
 * required to use this function. The library file rpworld.lib is also
 * required.
 *
 * \param wpWorld  A pointer to a RpWorld with PVS.
 * \param callback  A pointer to the \ref RpPVSCallBack function for sampling a world sector.
 * \param pData  A pointer to private data for the callback.
 *
 * \return RpWorld if successful or NULL if there is an error.
 *
 * \see RpPVSSetWorldSectorVisibility
 * \see RpPVSGeneric
 * \see RpPVSSamplePOV
 */
RpWorld            *
RpPVSConstruct(RpWorld * wpWorld,
            RpPVSCallBack callback,
            void *pData)
{
    RpPVSCache         *pvsCache = PVSCACHEFROMWORLD(wpWorld);
    _RpPVSCallBack      bundlecb;
    RwInt32             recipplone;
    RwBool collData = FALSE;

    RWAPIFUNCTION(RWSTRING("RpPVSConstruct"));
    RWASSERT(wpWorld);

    bundlecb.callback = callback;
    bundlecb.data = pData;

    pvsCache->NumWorldSectors = 0;
    rpPVSGlobals.InSector = -1;

    /* Clear the progress count. */
    rpPVSGlobals.progress_count = 0;


    rpPVSGlobals.World = wpWorld;
    if (pData != NULL) rpPVSGlobals.gran = *((RwReal *) pData);
    recipplone = RwInt32FromRealMacro((1.0f /
                                       rpPVSGlobals.gran) +
                                      1.0f + rpPVSSMALL);
    pvsCache->viscount = recipplone * recipplone * recipplone;
    pvsCache->nextID = 0;


    PVSInitializeSectors();

    /* assign space and clear down for a vismap for each sector */
    RpWorldForAllWorldSectors(wpWorld, PVSAlloc, NULL);



    if (rpPVSGlobals.collis && callback)
    {
        /* Any collision data? There should be. Build them if none */
        collData = RpCollisionWorldQueryData(wpWorld);
        if (collData == FALSE)
        {
            RWMESSAGE((RWSTRING("The world should have collision data to be efficient!")));
            RpCollisionWorldBuildData(wpWorld,
                                  (RpCollisionBuildParam *) NULL);
        }
    }

    if (!pvsCache->formatted && callback!=NULL)
        PVSInitialize();       /* Put the data into PVS useable form,
                                  (if we're gonna do sampling) */

    if (callback)
    {
        /* so we can pass a single pointer */
        bundlecb.callback = callback;
        bundlecb.data = pData;

        /* determine visibility */
        if (pvsCache->progressCallBack)
        {
            pvsCache->progressCallBack(rpPVSPROGRESSSTART,
                                       0.0f);
        }
        RpWorldForAllWorldSectors(wpWorld, PVSDispatch, &bundlecb);

        if (pvsCache->progressCallBack)
        {
            pvsCache->progressCallBack(rpPVSPROGRESSEND, 100.0f);
        }

    }

    pvsCache->processed = TRUE;


    /* Destroy collision data if it was generated by the pvs */
    if (rpPVSGlobals.collis && callback && !collData)
        RpCollisionWorldDestroyData(wpWorld);

    RWRETURN(wpWorld);
}

/**
 * \ingroup rppvs
 * \ref RpPVSConstructSector
 * is used to create Potential Visibility Set, PVS, for the given world sector. The
 * PVS is created by taking samples within the world sector to build a
 * visibility map.  This map indicates which other world sectors are
 * visible from within its boundary.
 *
 * \note If this function is called more than once, the world should have
 * collision data. This is for reasons of efficiency.
 *
 * The include file rppvs.h and the library file rppvs.lib are
 * required to use this function. The library file rpworld.lib is also
 * required.
 *
 * \param wpWorld  A pointer to an RpWorld with PVS.
 * \param spSector  A pointer to an RpWorldSector with PVS.
 * \param callback  A pointer to the \ref RpPVSCallBack function for sampling a world sector.
 * \param pData  A pointer to private data for the callback.
 *
 * \return RpWorld if successful or NULL if there is an error.
 *
 * \see RpPVSSetWorldSectorVisibility
 * \see RpPVSGeneric
 * \see RpPVSSamplePOV
 */
RpWorld*
RpPVSConstructSector(RpWorld * wpWorld,
            RpWorldSector * spSector,
            RpPVSCallBack callback,
            void *pData)
{
    RpPVSCache         *pvsCache = PVSCACHEFROMWORLD(wpWorld);
    _RpPVSCallBack      bundlecb;
    RwInt32             recipplone;
    RwBool collData = FALSE;

    RWAPIFUNCTION(RWSTRING("RpPVSConstructSector"));
    RWASSERT(wpWorld);

    bundlecb.callback = callback;
    bundlecb.data = pData;

    pvsCache->NumWorldSectors = 0;
    rpPVSGlobals.InSector = -1;

    /* Clear the progress count. */
    rpPVSGlobals.progress_count = 0;


    rpPVSGlobals.World = wpWorld;
    if (pData != NULL) rpPVSGlobals.gran = *((RwReal *) pData);
    recipplone = RwInt32FromRealMacro((1.0f / rpPVSGlobals.gran) +
                                      1.0f + rpPVSSMALL);
    pvsCache->viscount = recipplone * recipplone * recipplone;
    pvsCache->nextID = 0;


    PVSInitializeSectors();

    /* assign space and clear down for a vismap for each sector */
    RpWorldForAllWorldSectors(wpWorld, PVSAlloc, NULL);


    if (rpPVSGlobals.collis && callback)
    {
        /* Any collision data? There should be. Build them if none */
        collData = RpCollisionWorldQueryData(wpWorld);
        if (collData == FALSE)
        {
            RWMESSAGE((RWSTRING("The world should have collision data to be efficient!")));
            RpCollisionWorldBuildData(wpWorld,
                                  (RpCollisionBuildParam *) NULL);
        }
    }


    if (!pvsCache->formatted && callback!=NULL)
        PVSInitialize();       /* Put the data into PVS useable form,
                                  (if we're gonna do sampling) */

    if (callback)
    {
        /* so we can pass a single pointer */
        bundlecb.callback = callback;
        bundlecb.data = pData;

        /* determine visibility */
        if (pvsCache->progressCallBack)
        {
            pvsCache->progressCallBack(rpPVSPROGRESSSTART,
                                       0.0f);
        }
        PVSDispatch(spSector, &bundlecb);

        if (pvsCache->progressCallBack)
        {
            pvsCache->progressCallBack(rpPVSPROGRESSEND, 100.0f);
        }

    }

    pvsCache->processed = TRUE;

    /* Destroy collision data if it was generated by the pvs */
    if (rpPVSGlobals.collis && callback && !collData)
        RpCollisionWorldDestroyData(wpWorld);

    RWRETURN(wpWorld);
}


/**
 * \ingroup rppvs
 * \ref RpPVSSamplePOV
 * is used to update the current visibility map with a sample using the given
 * position.
 *
 * It is possible for the PVS to drop sectors incorrectly due to insufficient
 * sampling. RpPVSSamplePOV can be used to repair such errors by adding
 * extra samples at specific positions.
 *
 * This functions assumes PVS data is already present in a world.
 *
 * \note If this function is called more than once, the world should have
 * collision data. This is for reasons of efficiency.
 *
 * The include file rppvs.h and the library file rppvs.lib are
 * required to use this function. The library file rpworld.lib is also
 * required.
 *
 * \param pos  A pointer to a RwV3d which specifies the current viewing position.
 * \param colltest If TRUE, it only uses the viewpoint if it is inside the geometry
 * of the sector.  NB: This temporaily overrides the parameters set by
 * \ref RpPVSSetCollisionDetection
 *
 * \return TRUE if successful or FALSE if the sample point was rejected.
 *
 * \see RpPVSSetWorldSectorVisibility
 * \see RpPVSSamplePOV
 * \see RpPVSGeneric
 * \see RpPVSConstruct
 */
RwBool
RpPVSSamplePOV(RwV3d * pos, RwBool colltest)
{
    RpPVSCache         *pvsCache =
        PVSCACHEFROMWORLD(rpPVSGlobals.World);
    RpIntersection      isSphere;
    RwInt32             hitcount;
    RwBool collData = FALSE;

    RWAPIFUNCTION(RWSTRING("RpPVSSamplePOV"));

    if (colltest)
    {
        /* Any collision data? There should be. Build them if none */
        collData = RpCollisionWorldQueryData(rpPVSGlobals.World);
        if (collData == FALSE)
        {
            RWMESSAGE((RWSTRING("The world should have collision data to be efficient!")));
            RpCollisionWorldBuildData(rpPVSGlobals.World,
                                  (RpCollisionBuildParam *) NULL);
        }
    }

    if (!pvsCache->formatted)
        PVSInitialize();

    RpPVSSetViewPosition(rpPVSGlobals.World, pos);
    if (colltest)
    {
        isSphere.type = rpINTERSECTSPHERE;
        isSphere.t.sphere.center = *pos;
        isSphere.t.sphere.radius = 0.1f;

        hitcount = FALSE;
        RpCollisionWorldForAllIntersections(rpPVSGlobals.World,
                                            &isSphere,
                                            PVSSphereCollision,
                                            &hitcount);

        isSphere.t.sphere.radius = rpPVSMAX(rpPVSGlobals.CurrPVS->diagonal / 100.0f,
            rpPVSGlobals.diagonal / 10000.0f);

        if (!hitcount)
        {
            /* if point is not on the infinite (w/ tolerance) face of any poly */
            if (inouttest(pos, isSphere.t.sphere.radius))
            {
                /* set curr sector */
                PVSCreateViewPointPVS(pos, rpPVSGlobals.CurrPVS);
            }
            else
            {
                /* Destroy collision data if it was generated by the pvs */
                if (colltest && !collData)
                    RpCollisionWorldDestroyData(rpPVSGlobals.World);

                RWRETURN(FALSE);
            }
        }
    }
    else
    {
        /* set curr sector */
        PVSCreateViewPointPVS(pos, rpPVSGlobals.CurrPVS);
    }

    /* Destroy collision data if it was generated by the pvs */
    if (colltest && !collData)
        RpCollisionWorldDestroyData(rpPVSGlobals.World);

    RWRETURN(TRUE);
}


static RpWorldSector *
PVSnull(RpWorldSector * sect, void * data __RWUNUSED__)
{
    RpPVS              *PVS = PVSFROMWORLDSECTOR(sect);

    RWFUNCTION(RWSTRING("PVSnull"));

    RwFree(PVS->vismap);
    PVS->vismap = (RpPVSVisMap *) NULL;
    PVS->vismaplength=0;
    PVS->sampleKey = 0;
    PVS->sectorID = -1;

    RWRETURN(sect);
}

/**
 * \ingroup rppvs
 * \ref RpPVSDestroy
 * is used to destroy the PVS for the given world.
 *
 * The include file rppvs.h and the library file rppvs.lib are
 * required to use this function. The library file rpworld.lib is also
 * required.
 *
 * \param wpWorld  A pointer to a RpWorld containing the PVS data.
 *
 * \return RpWorld if sucessful, NULL otherwise.
 *
 * \see RpPVSAtomicVisible
 * \see RpPVSGetProgressCallBack
 * \see RpPVSHook
 * \see RpPVSPluginAttach
 * \see RpPVSQuery
 * \see RpPVSSetProgressCallBack
 * \see RpPVSSetViewPosition
 * \see RpPVSStatisticsGet
 * \see RpPVSUnhook
 * \see RpPVSWorldSectorVisible
 */
RpWorld            *
RpPVSDestroy(RpWorld * wpWorld)
{
    RpPVSCache         *pvsCache;

    RWAPIFUNCTION(RWSTRING("RpPVSDestroy"));

    RpPVSUnhook(wpWorld);

    pvsCache = PVSCACHEFROMWORLD(wpWorld);

    PVSDeinitialize(wpWorld);

    if (pvsCache->processed)
    {
        RpWorldForAllWorldSectors(wpWorld, PVSnull, NULL);
        pvsCache->processed = FALSE;
    }
    rpPVSGlobals.World = (RpWorld *) NULL;

    RWRETURN(wpWorld);
}

static void *
PVSWorldConstructor(RpWorld * wpWorld,
                    RwInt32 offset __RWUNUSED__,
                    RwInt32 size __RWUNUSED__)
{
    RpPVSCache         *pvsCache = PVSCACHEFROMWORLD(wpWorld);

    RWFUNCTION(RWSTRING("PVSWorldConstructor"));

    /* pipeline hooking */
    pvsCache->hooked = FALSE;
    /* used during vismap allocation */
    pvsCache->nextID = 0;

    pvsCache->progressCallBack = (RpPVSProgressCallBack) NULL;

    pvsCache->ptotal = 0;
    pvsCache->paccept = 0;

    pvsCache->formatted = FALSE;
    pvsCache->processed = FALSE;

    pvsCache->viscount = 0;
    pvsCache->polygons = (_rpPVSPolyList *) NULL;

    pvsCache->renderCallBack =
        (RpWorldSectorCallBackRender)NULL;

    rpPVSGlobals.World = wpWorld;
    rpPVSGlobals.collis = TRUE;
    rpPVSGlobals.bfc = TRUE;

    rpPVSGlobals.InSector = -1;

    PVSInitializeSectors();

    RWRETURN(wpWorld);
}

static void *
PVSWorldDestructor(RpWorld * wpWorld,
                   RwInt32 offset __RWUNUSED__,
                   RwInt32 size __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("PVSWorldDestructor"));

    RpPVSUnhook(wpWorld);

    PVSDeinitialize(wpWorld);

    rpPVSGlobals.World = (RpWorld *) NULL;

    RWRETURN(wpWorld);
}

static void *
PVSSectorConstructor(RpWorldSector * sector,
                     RwInt32 offset __RWUNUSED__,
                     RwInt32 size __RWUNUSED__)
{
    RpPVS              *pvsCur = PVSFROMWORLDSECTOR(sector);

    RWFUNCTION(RWSTRING("PVSSectorConstructor"));

    pvsCur->sectorID = ~0;
    pvsCur->sectailpoly = (_rpPVSPolyList *) NULL;
    pvsCur->potential = rpPVSSPLIT;
    pvsCur->sampleKey = 0;
    pvsCur->vismap = (RpPVSVisMap*) NULL;
    pvsCur->vismaplength = 0;
    pvsCur->numpols = 0;

    RWRETURN(sector);
}

static void *
PVSSectorDestructor(RpWorldSector * sector,
                    RwInt32 offset __RWUNUSED__,
                    RwInt32 size __RWUNUSED__)
{
    RpPVS              *pvsCur = PVSFROMWORLDSECTOR(sector);

    RWFUNCTION(RWSTRING("PVSSectorDestructor"));

    pvsCur->sectorID = ~0;
    pvsCur->sampleKey = 0;

    if (pvsCur->vismap != NULL)
    {
        RwFree(pvsCur->vismap);
        pvsCur->vismap = (RpPVSVisMap *) NULL;
    }
    RWRETURN(sector);
}

/**
 * \ingroup rppvs
 * \ref RpPVSHook is used to enable rendering of the specified world
 * using the world's PVS data.  Typically used prior to a frame render
 * (and followed afterwards by \ref RpPVSUnhook) but may be permanently
 * enabled if PVS rendering is always required.  If \ref RpPVSHook is not used
 * all world sectors in the current camera's view frustum are rendered.
 *
 * Note that this function overrides the world sector render callback
 * which is only returned to its original form when \ref RpPVSUnhook is
 * called.
 *
 * The PVS plugin must be attached before using this function.
 *
 * The include file rppvs.h and the library file rppvs.lib are required
 * to use this function. The library file rpworld.lib is also required.
 *
 * \param wpWorld  A pointer to a RpWorld with PVS data.
 *
 * \return RpWorld if sucessful, NULL otherwise.
 *
 * \see RpPVSAtomicVisible
 * \see RpPVSDestroy
 * \see RpPVSGetProgressCallBack
 * \see RpPVSPluginAttach
 * \see RpPVSQuery
 * \see RpPVSSetProgressCallBack
 * \see RpPVSSetViewPosition
 * \see RpPVSStatisticsGet
 * \see RpPVSUnhook
 * \see RpPVSWorldSectorVisible
 */
RpWorld            *
RpPVSHook(RpWorld * wpWorld)
{
    RpPVSCache         *pvsCache = PVSCACHEFROMWORLD(wpWorld);

    RWAPIFUNCTION(RWSTRING("RpPVSHook"));
    RWASSERT(wpWorld);

    if (pvsCache->processed)
    {
        if (!pvsCache->hooked)
        {
            pvsCache->hooked = TRUE;

            RWASSERT(pvsCache->renderCallBack == NULL);

            /* Swap the render callback. */
            pvsCache->renderCallBack =
                RpWorldGetSectorRenderCallBack(wpWorld);

            RpWorldSetSectorRenderCallBack(wpWorld,
                PVSWorldSectorRenderCallBack);
        }
    }

    RWRETURN(wpWorld);
}

/**
 * \ingroup rppvs
 * \ref RpPVSUnhook is used to disable rendering of the
 * specified world using the world's PVS data.  Typically used just after
 * to a frame render (which followed a call to \ref RpPVSHook) but may be
 * permanently disabled if PVS rendering is not required.
 *
 * Note that the function \ref RpPVSHook overrides the world sector
 * render callback which is only returned to its original form when
 * RpPVSUnhook is called.  The PVS plugin must be attached before using
 * this function.
 *
 * The include file rppvs.h and the library file rppvs.lib are required
 * to use this function. The library file rpworld.lib is also required.
 *
 * \param wpWorld  A pointer to a RpWorld containing PVS data.
 *
 * \return RpWorld if sucessful, NULL otherwise.
 *
 * \see RpPVSAtomicVisible
 * \see RpPVSDestroy
 * \see RpPVSGetProgressCallBack
 * \see RpPVSHook
 * \see RpPVSPluginAttach
 * \see RpPVSQuery
 * \see RpPVSSetProgressCallBack
 * \see RpPVSSetViewPosition
 * \see RpPVSStatisticsGet
 * \see RpPVSWorldSectorVisible
 */
RpWorld            *
RpPVSUnhook(RpWorld * wpWorld)
{
    RpPVSCache         *pvsCache = PVSCACHEFROMWORLD(wpWorld);

    RWAPIFUNCTION(RWSTRING("RpPVSUnhook"));
    RWASSERT(wpWorld);

    if (pvsCache->hooked)
    {
        pvsCache->hooked = FALSE;

        /* Check that the current render call is the PVS one. */
        RWASSERT(PVSWorldSectorRenderCallBack ==
                 RpWorldGetSectorRenderCallBack(wpWorld));

        /* Swap the render callback. */
        RpWorldSetSectorRenderCallBack(wpWorld,
            pvsCache->renderCallBack);

        pvsCache->renderCallBack =
            (RpWorldSectorCallBackRender)NULL;
    }

    RWRETURN(wpWorld);
}

/**
 * \ingroup rppvs
 * \ref RpPVSGetProgressCallBack is used to retrieve PVS creation
 * progress callback function of the specified world.  The callback is
 * called from \ref RpPVSGeneric every time it has processed a single
 * world sector, enabling an application to monitor how the generation of
 * PVS data is progressing and, possibly, to provide feedback to the
 * user.
 *
 * \param wpWorld  A pointer to a RpWorld.
 *
 * \return The \ref RpPVSProgressCallBack function if sucessful, NULL otherwise.
 *
 * \see RpPVSAtomicVisible
 * \see RpPVSDestroy
 * \see RpPVSHook
 * \see RpPVSPluginAttach
 * \see RpPVSQuery
 * \see RpPVSSetProgressCallBack
 * \see RpPVSSetViewPosition
 * \see RpPVSStatisticsGet
 * \see RpPVSUnhook
 * \see RpPVSWorldSectorVisible
 */
RpPVSProgressCallBack
RpPVSGetProgressCallBack(RpWorld * wpWorld)
{
    RpPVSCache         *pvsCache = PVSCACHEFROMWORLD(wpWorld);

    RWAPIFUNCTION(RWSTRING("RpPVSGetProgressCallBack"));
    RWASSERT(wpWorld);

    if (wpWorld)
    {
        RWRETURN(pvsCache->progressCallBack);

        RWERROR((E_RW_NULLP));
        RWRETURN((RpPVSProgressCallBack) NULL);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN((RpPVSProgressCallBack) NULL);
}

/**
 * \ingroup rppvs
 * \ref RpPVSSetProgressCallBack is used to define a PVS creation
 * progress callback function for the specified world.  The callback is
 * called from \ref RpPVSGeneric every time it has processed a single
 * world sector, enabling an application to monitor how the generation of
 * PVS data is progressing and, possibly, to provide feedback to the
 * user.
 *
 * \param wpWorld  A pointer to a RpWorld.
 * \param callback  A pointer to the \ref RpPVSProgressCallBack function.
 *
 * \return RpWorld if sucessful, NULL otherwise.
 *
 * \see RpPVSAtomicVisible
 * \see RpPVSDestroy
 * \see RpPVSGetProgressCallBack
 * \see RpPVSHook
 * \see RpPVSPluginAttach
 * \see RpPVSQuery
 * \see RpPVSSetViewPosition
 * \see RpPVSStatisticsGet
 * \see RpPVSUnhook
 * \see RpPVSWorldSectorVisible
 */
RpWorld            *
RpPVSSetProgressCallBack(RpWorld * wpWorld,
                         RpPVSProgressCallBack callback)
{
    RpPVSCache         *pvsCache = PVSCACHEFROMWORLD(wpWorld);

    RWAPIFUNCTION(RWSTRING("RpPVSSetProgressCallBack"));
    RWASSERT(wpWorld);

    if (wpWorld)
    {
        if (pvsCache)
        {
            pvsCache->progressCallBack = callback;
            RWRETURN(wpWorld);
        }

        RWERROR((E_RW_NULLP));
        RWRETURN((RpWorld *) NULL);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN((RpWorld *) NULL);
}

/* Serialization for the sectors...*/
static RwInt32
PVSSectorGetChunkSize(const RpWorldSector * spSect,
                      RwInt32 offset __RWUNUSED__,
                      RwInt32 size __RWUNUSED__)
{
    RwInt32             len;
    const RpPVS        *pvsCur = PVSFROMCONSTWORLDSECTOR(spSect);

    RWFUNCTION(RWSTRING("PVSSectorGetChunkSize"));

    if (pvsCur->sectorID==-1)
    {
        len = 0;
    }
    else
    {
        len = (sizeof(pvsCur->sectorID) + sizeof(pvsCur->vismaplength) +
                    sizeof(pvsCur->sampleKey) +
                    pvsCur->vismaplength * sizeof(RpPVSVisMap)); /* the vismap */

    }
    RWRETURN(len);

}

static RwStream *
PVSSectorWriteChunk(RwStream * s,
                    RwInt32 len __RWUNUSED__,
                    const RpWorldSector * spSect,
                    RwInt32 offset __RWUNUSED__,
                    RwInt32 size __RWUNUSED__)
{
    const RpPVS        *pvsCur = PVSFROMCONSTWORLDSECTOR(spSect);

    RWFUNCTION(RWSTRING("PVSSectorWriteChunk"));
    /* sector's data.... */

    RwStreamWriteInt32(s, &pvsCur->sectorID, sizeof(pvsCur->sectorID));

    RwStreamWriteInt32(s, &pvsCur->vismaplength,
                       sizeof(pvsCur->vismaplength));

    RwStreamWriteInt32(s, &pvsCur->sampleKey,
                       sizeof(pvsCur->sampleKey));

    /* and the vismap.... */
    if (pvsCur->vismaplength != 0) RwStreamWrite(s, pvsCur->vismap,
                  pvsCur->vismaplength * sizeof(RpPVSVisMap));

    RWRETURN(s);
}

static RwStream *
PVSSectorReadChunk(RwStream * s,
                   RwInt32 len __RWUNUSED__,
                   RpWorldSector * spSect,
                   RwInt32 offset __RWUNUSED__,
                   RwInt32 size __RWUNUSED__)
{
    RpPVS              *pvsCur = PVSFROMWORLDSECTOR(spSect);

    RWFUNCTION(RWSTRING("PVSSectorReadChunk"));

    /* sector's data.... */
    RwStreamReadInt32(s, &pvsCur->sectorID, sizeof(pvsCur->sectorID));
    RwStreamReadInt32(s, &pvsCur->vismaplength,
                      sizeof(pvsCur->vismaplength));
    RwStreamReadInt32(s, &pvsCur->sampleKey, sizeof(pvsCur->sampleKey));

    /* allocate space */
    if (pvsCur->vismaplength != 0) pvsCur->vismap = (RpPVSVisMap *)
        RwMalloc(pvsCur->vismaplength * sizeof(RpPVSVisMap),
                 rwID_GPVSPLUGIN | rwMEMHINTDUR_EVENT);

    /* read the vismap... */
    if (pvsCur->vismaplength != 0) RwStreamRead(s, pvsCur->vismap,
                 pvsCur->vismaplength * sizeof(RpPVSVisMap));

    RWRETURN(s);
}

/* Serialization for the world/cache ...*/
static RwInt32
PVSWorldGetChunkSize(const RpWorld * wpWorld,
                     RwInt32 offset __RWUNUSED__,
                     RwInt32 size __RWUNUSED__)
{
    const RpPVSCache   *pvsCache = PVSCACHEFROMCONSTWORLD(wpWorld);

    RWFUNCTION(RWSTRING("PVSWorldGetChunkSize"));

    if (!pvsCache->processed) RWRETURN(0);
    RWRETURN(sizeof(pvsCache->processed) + sizeof(pvsCache->hooked));
}

static RwStream *
PVSWorldWriteChunk(RwStream * s,
                   RwInt32 len __RWUNUSED__,
                   RpWorld * wpWorld,
                   RwInt32 offset __RWUNUSED__,
                   RwInt32 size __RWUNUSED__)
{
    const RpPVSCache   *pvsCache = PVSCACHEFROMCONSTWORLD(wpWorld);

    RWFUNCTION(RWSTRING("PVSWorldWriteChunk"));

    /* set current world */
    rpPVSGlobals.World = (RpWorld *) wpWorld;

    RwStreamWriteInt32(s, &pvsCache->processed,
                       sizeof(pvsCache->processed));
    RwStreamWriteInt32(s, &pvsCache->hooked, sizeof(pvsCache->hooked));

    RWRETURN(s);
}

static RwStream *
PVSWorldReadChunk(RwStream * s,
                  RwInt32 len __RWUNUSED__,
                  RpWorld * wpWorld,
                  RwInt32 offset __RWUNUSED__,
                  RwInt32 size __RWUNUSED__)
{
    RpPVSCache         *pvsCache = PVSCACHEFROMWORLD(wpWorld);

    RWFUNCTION(RWSTRING("PVSWorldReadChunk"));

    RwStreamReadInt32(s, &pvsCache->processed,
                      sizeof(pvsCache->processed));
    RwStreamReadInt32(s, &pvsCache->hooked, sizeof(pvsCache->hooked));
    pvsCache->hooked = FALSE;

    rpPVSGlobals.World = wpWorld;
    rpPVSGlobals.bfc = TRUE;


    PVSInitializeSectors();

    RWRETURN(s);
}

/**
 * \ingroup rppvs
 * \ref RpPVSPluginAttach is used to attach the PVS
 * plugin to the RenderWare system to enable the building and use of
 * potentially visible sets.  The PVS plugin must be attached between
 * initializing the system with \ref RwEngineInit and opening it with
 * \ref RwEngineOpen.
 *
 * \note The PVS plugin requires the world plugin to be attached.
 *
 * The include file rppvs.h is also required and must be included by an
 * application wishing to use PVS.  Note also that when linking the PVS
 * plugin library, rppvs.lib, into an application, make sure the
 * following RenderWare library is made available to the linker:
 * rtray.lib.
 *
 * \return TRUE if successful or FALSE if there is an error
 *
 * \see RpPVSAtomicVisible
 * \see RpPVSDestroy
 * \see RpPVSGetProgressCallBack
 * \see RpPVSHook
 * \see RpPVSQuery
 * \see RpPVSSetProgressCallBack
 * \see RpPVSSetViewPosition
 * \see RpPVSStatisticsGet
 * \see RpPVSUnhook
 * \see RpPVSWorldSectorVisible
 * \see RpWorldPluginAttach
 * \see RwEngineInit
 * \see RwEngineOpen
 * \see RwEngineStart
 */
RwBool
RpPVSPluginAttach(void)
{
    /* Note the plugin id is should be rwID_GPVSPLUGIN to avoid
     * problems with the retired PVS plugin */

    RwInt32             offset;

    RWAPIFUNCTION(RWSTRING("RpPVSPluginAttach"));

    rpPVSGlobals.World = (RpWorld *) NULL;

    rpPVSGlobals.worldOffset =
        RpWorldRegisterPlugin(sizeof(RpPVSCache), rwID_GPVSPLUGIN,
            (RwPluginObjectConstructor) PVSWorldConstructor,
            (RwPluginObjectDestructor) PVSWorldDestructor,
            (RwPluginObjectCopy) NULL);
    if (rpPVSGlobals.worldOffset < 0)
    {
        RWRETURN(FALSE);
    }

    offset =
        RpWorldRegisterPluginStream(rwID_GPVSPLUGIN,
            (RwPluginDataChunkReadCallBack) PVSWorldReadChunk,
            (RwPluginDataChunkWriteCallBack) PVSWorldWriteChunk,
            (RwPluginDataChunkGetSizeCallBack) PVSWorldGetChunkSize);

    if (offset < 0)
    {
        RWRETURN(FALSE);
    }

    rpPVSGlobals.sectorOffset =
        RpWorldSectorRegisterPlugin(sizeof(RpPVS), rwID_GPVSPLUGIN,
            (RwPluginObjectConstructor) PVSSectorConstructor,
            (RwPluginObjectDestructor) PVSSectorDestructor,
            (RwPluginObjectCopy) NULL);
    if (rpPVSGlobals.sectorOffset < 0)
    {
        RWRETURN(FALSE);
    }

    offset =
        RpWorldSectorRegisterPluginStream(rwID_GPVSPLUGIN,
            (RwPluginDataChunkReadCallBack) PVSSectorReadChunk,
            (RwPluginDataChunkWriteCallBack) PVSSectorWriteChunk,
            (RwPluginDataChunkGetSizeCallBack) PVSSectorGetChunkSize);

    if (offset < 0)
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}

/**
 * \ingroup rppvs
 * \ref RpPVSQuery is used to determine whether the specified world
 * contains PVS data, that is, if it has already been processed using
 * \ref RpPVSConstruct.
 *
 * The PVS plugin must be attached before using this function.
 *
 * The include file rppvs.h and the library file rppvs.lib are
 * required to use this function. The library file rpworld.lib is also
 * required.
 *
 * \param wpWorld  A pointer to a RpWorld to be queried.
 *
 * \return TRUE if PVS data is present, FALSE otherwise.
 *
 * \see RpPVSAtomicVisible
 * \see RpPVSDestroy
 * \see RpPVSGetProgressCallBack
 * \see RpPVSHook
 * \see RpPVSPluginAttach
 * \see RpPVSSetProgressCallBack
 * \see RpPVSSetViewPosition
 * \see RpPVSStatisticsGet
 * \see RpPVSUnhook
 * \see RpPVSWorldSectorVisible
 */
RwBool
RpPVSQuery(RpWorld * wpWorld)
{
    RpPVSCache         *pvsCache = PVSCACHEFROMWORLD(wpWorld);

    RWAPIFUNCTION(RWSTRING("RpPVSQuery"));

    rpPVSGlobals.World = wpWorld;
    RWRETURN(pvsCache->processed);
}

static RpWorldSector *
PVSSectorVis(RpWorldSector * sector, void *data)
{
    RwBool             *isvisible = (RwBool *) data;

    RWFUNCTION(RWSTRING("PVSSectorVis"));

    if (RpPVSWorldSectorVisible(sector))
    {
        (*isvisible) = TRUE;
        RWRETURN(NULL); /* force break in iterator, now we know */
    }

    RWRETURN(sector);
}

/**
 * \ingroup rppvs
 * \ref RpPVSAtomicVisible
 * is used to determine if the atomic is visible from the current visibility
 * map.
 *
 * The include file rppvs.h and the library file rppvs.lib are
 * required to use this function. The library file rpworld.lib is also
 * required.
 *
 * \param atom  A pointer to a RpAtomic.
 *
 * \return TRUE if visible, FALSE otherwise.
 *
 * \see RpPVSDestroy
 * \see RpPVSGetProgressCallBack
 * \see RpPVSHook
 * \see RpPVSPluginAttach
 * \see RpPVSQuery
 * \see RpPVSSetProgressCallBack
 * \see RpPVSSetViewPosition
 * \see RpPVSStatisticsGet
 * \see RpPVSUnhook
 * \see RpPVSWorldSectorVisible
 */
RwBool
RpPVSAtomicVisible(RpAtomic * atom)
{
    RwBool              isvisible;

    RWAPIFUNCTION(RWSTRING("RpPVSAtomicVisible"));

    isvisible = FALSE;
    RpAtomicForAllWorldSectors(atom, PVSSectorVis, &isvisible);

    RWRETURN(isvisible);
}

/**
 * \ingroup rppvs
 * \ref RpPVSStatisticsGet is used to retrieve statistics relating to the
 * rendering perfomance of the PVS data in the specified world.  The figures
 * obtained from this function are:
 * (a) the total number of triangles that would have been rendered if PVS data
 *     was not used, and
 * (b) the total number of triangles that have been rendered using the PVS data.
 * Typically used immediately after a frame render.
 *
 * The PVS plugin must be attached before using this function.
 *
 * \param wpWorld  A pointer to a RpWorld with PVS data.
 * \param ptotal  A pointer to a RwInt32 to return number of polygons before PVS culling.
 * \param paccept  A pointer to a RwInt32 to return number of polygons after PVS culling.
 *
 * \return RpWorld if sucessful, NULL otherwise.
 *
 * \see RpPVSAtomicVisible
 * \see RpPVSDestroy
 * \see RpPVSGetProgressCallBack
 * \see RpPVSHook
 * \see RpPVSPluginAttach
 * \see RpPVSQuery
 * \see RpPVSSetProgressCallBack
 * \see RpPVSSetViewPosition
 * \see RpPVSUnhook
 * \see RpPVSWorldSectorVisible
 */
RpWorld            *
RpPVSStatisticsGet(RpWorld * wpWorld, RwInt32 * ptotal,
                   RwInt32 * paccept)
{
    RpPVSCache         *pvsCache = PVSCACHEFROMWORLD(wpWorld);

    RWAPIFUNCTION(RWSTRING("RpPVSStatisticsGet"));

    RWASSERT(wpWorld);
    RWASSERT(ptotal);
    RWASSERT(paccept);

    rpPVSGlobals.World = wpWorld;

    /* read tally.. */
    *ptotal = pvsCache->ptotal;
    *paccept = pvsCache->paccept;

    /* and zero them */
    pvsCache->ptotal = 0;
    pvsCache->paccept = 0;

    RWRETURN(wpWorld);
}

/****************************************************************************
 local defines
 */

#define NUMCLUSTERSOFINTEREST                   1
#define NUMOUTPUTS                              1

#define MESSAGE(_string)                                            \
    RwDebugSendMessage(rwDEBUGMESSAGE, "PVSWorldSectorCSL", _string)

 /****************************************************************************
 local (static) prototypes
 */
static RwInt32
PVSWorldSectorNodeFn(RxPipelineNodeInstance * self __RWUNUSED__,
                     const RxPipelineNodeParam * params)
{
    RwBool              visible;
    RpPVSCache         *pvsCache;
    RpPVS              *pvs;
    RpWorldSector      *worldSector;
    RpWorld            *world;

    RWFUNCTION(RWSTRING("PVSWorldSectorNodeFn"));

    RWASSERT(NULL != self);

    worldSector = (RpWorldSector *) RxPipelineNodeParamGetData(params);
    RWASSERT(NULL != worldSector);

    world = rpPVSGlobals.World;

    pvs = PVSFROMWORLDSECTOR(worldSector);
    RWASSERT(NULL != pvs);
    pvsCache = PVSCACHEFROMWORLD(world);
    RWASSERT(NULL != pvsCache);

    /* Don't reject empty sectors since they may contain visible atomics. */

    visible = TRUE;

    if ((worldSector->numTriangles > 0) && (NULL != pvs->vismap))
    {
        RwInt32             sectorID = pvs->sectorID;

        pvsCache->ptotal += worldSector->numTriangles;

        if (PVSVISMAPGETSECTOR(pvs->vismap, sectorID))
        {
            visible = TRUE;
        }
        else
        {
            visible = FALSE;
        }
    }

    if (visible)
    {
        pvsCache->paccept += worldSector->numTriangles;

        /* Make the rest of the pipe execute */
        RxPacketDispatch(NULL, 0, self);
    }

    /* If we're not visible, we didn't dispatch, so the pipe will unwind
     * (without error) without going beyond this point */
    RWRETURN(TRUE);
}

/**
 * \ingroup rppvs
 * \ref RxNodeDefinitionGetPVSWorldSectorCSL
 * returns a pointer to a node implementing PVS culling in custom world
 * sector object pipelines.
 *
 * This nodes prevents world sectors from being rendered if they are
 * culled by the PVS visilibility map.
 *
 * This node should be inserted at the beginning of a world sector object
 * pipeline. It must come before any nodes which create or expect packets
 * (e.g an instancing node), as it does not itself create or use them.
 * If a world sector is visible, execution continues to the next node in
 * the pipeline, otherwise the pipeline is terminated.
 *
 * The node has one output and no clusters of interest.
 *
 * \return pointer to a node for PVS culling in custom world sector
 *         pipelines on success, NULL otherwise
 *
 */
RxNodeDefinition   *
RxNodeDefinitionGetPVSWorldSectorCSL(void)
{
    static RwChar       _PVSData_csl[] = RWSTRING("PVSData.csl");

    static RxClusterDefinition clusterPVSData = { _PVSData_csl,
        0,
        0,
        (const RwChar *) NULL
    };

    /* TODO: can you not have nodes without any clusters of interest?
     *       This cluster is entirely useless... */
    static RxClusterRef gPVSNodeClusters[NUMCLUSTERSOFINTEREST] = {
        {&clusterPVSData, rxCLALLOWABSENT, rxCLRESERVED}
    };

    static RxClusterValidityReq gPVSNodeReqs[NUMCLUSTERSOFINTEREST] = {
        rxCLREQ_DONTWANT,
    };

    static RxClusterValid gPVSNodeValid[NUMCLUSTERSOFINTEREST] = {
        rxCLVALID_VALID,
    };

    static RwChar       _Output[] = RWSTRING("Output");

    static RxOutputSpec gPVSNodeOutputs[NUMOUTPUTS] = {
        {_Output, gPVSNodeValid, rxCLVALID_INVALID}
    };

    static RwChar       _PVSWorldSector_csl[] =
        RWSTRING("PVSWorldSector.csl");

    static RxNodeDefinition nodePVSWorldSectorCSL = { /* */
        _PVSWorldSector_csl,    /* Name */
        {                      /* nodemethods */
         PVSWorldSectorNodeFn, /* +-- nodebody */
         (RxNodeInitFn) NULL,
         (RxNodeTermFn) NULL,
         (RxPipelineNodeInitFn) NULL,
         (RxPipelineNodeTermFn) NULL,
         (RxPipelineNodeConfigFn) NULL,
         (RxConfigMsgHandlerFn) NULL},
        {                      /* Io */
         NUMCLUSTERSOFINTEREST, /* +-- NumClustersOfInterest */
         gPVSNodeClusters,     /* +-- ClustersOfInterest */
         gPVSNodeReqs,         /* +-- InputRequirements */
         NUMOUTPUTS,           /* +-- NumOutputs */
         gPVSNodeOutputs       /* +-- Outputs */
         },
        0,
        (RxNodeDefEditable) FALSE,
        0
    };

    RxNodeDefinition   *result = &nodePVSWorldSectorCSL;

    RWAPIFUNCTION(RWSTRING("RxNodeDefinitionGetPVSWorldSectorCSL"));

    /*RWMESSAGE((RWSTRING("Pipeline II node"))); */

    RWRETURN(result);
}
