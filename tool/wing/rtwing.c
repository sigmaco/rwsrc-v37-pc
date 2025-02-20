/**
 * \ingroup rtwing
 * \page rtwingoverview RtWing Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rtgcond.h, rtwing.h
 * \li \b Libraries: rwcore, rtgcond, rtwing
 *
 * \subsection wingoverview Overview
 *
 * The functions in this toolkit implement, maintain and operate on polygons
 * using a modified wing/half-edge data structure. The wing representation
 * allows efficient querying (typically O(1)) and modification of geometry.
 * (Internally, the geometry takes a \ref rtgcond form.)
 */

/****************************************************************************
 Includes
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "rwcore.h"
#include "rpworld.h"
#include "rpdbgerr.h"

#include "rtgcond.h"
#include "rtwing.h"

/* NB: to maintainer, the 'value's of the edges, faces, and vertices can be
 * used at will. However, the sign of the values is critical, and once a value
 * drops below zero, that is a tag to say the element is no longer to be used!
 */

/****************************************************************************
 Defines
 */
#define ALLOWCONCAVE FALSE      /* Do not enable unless you want burnt fingers (FALSE) */
#define WELDPOLYGONS TRUE       /* Do the welding, of course (TRUE) */
#define CHECKSANITY FALSE       /* Enable for extra debugging (treat asserts as warnings)  */
#define TESTSAFEGUARD FALSE     /* Enable for extra debugging */
#define CHECKTOPOLOGY FALSE     /* Enable for extra debugging */


#define INVALIDAREAFLAG  RwRealMAXVAL

typedef struct WingVertexUsageList WingVertexUsageList;
struct WingVertexUsageList
{
    /* This is a one-off useage - intermediate data structure for creating WEDS */
    RtWingFace**     wingFaces;
    RwInt32* startLocalIndices; /* i.e. 0, 1, ... */
    RwInt32 numPolygons;
};

 /****************************************************************************
 Locals
 */
static RwBool wingRemoveColinearsOnTerminalEdge;/*quo:elsewhere*/
static RwBool wingRemoveColinearsOnCreasedEdge;/*quo:elsewhere*/
static RwBool wingRemoveColinearsOnContinuousEdge;/*quo:elsewhere*/
static RwReal wingInterationTag; /* This is updated on each call to tag the operation with a unique id */

/****************************************************************************
 Static function prototypes
 */
static RtWingEdge* WingEdgeCandidateIdentification(RtWings* wings, RtGCondGeometryList* geometryList,
                                                   RwInt32* id);
static void     WingEdgeRemoval(RtWingEdge* theEdge);
static void     WingRemoveColinearVerticesFromWings(RtWings* wings);
static RwReal   WingAngleBetweenFaces(RtWingFace* a, RtWingFace* b);
static RwBool   WingReflexTriVert(RwV3d* a, RwV3d* b, RwV3d* c, RwV3d* normal, RwBool linearSafe);
static RwBool   WingIsAnEar(RtWingFace *face, RtWingVertex* a, RtWingVertex* b, RtWingVertex* c, RwV3d* normal);
static RwBool   WingEqualZero(RwReal val);
static RwReal   WingRemainder(RwReal val);
static RwBool   WingProjectUVMatch(RtWingEdge* e, RtWings* wings, RtGCondGeometryList* geometryList);
static RwBool   WingVirtualUVMatch(RtGCondGeometryList* geometryList, RtGCondVertex* a, RtGCondVertex* b);
static RwBool   WingVirtualUVEdgeMatch(RtGCondGeometryList* geometryList, RtWingEdge* a, RtWingEdge* b);
static RwBool   WingFacePairAreaUVRatioMatch(RtGCondGeometryList* geometryList, RtWingFace* a, RtWingFace* b);
static void     WingProliferateUVs(RtWingFace* wingFace, RwTexCoords* offset,
                     RtWings* wings, RtGCondGeometryList* geometryList,
                     RwBool ammendUVs, RwBool* success);

static RwBool   WingEqualZeroTol(RwReal val, RwReal tol);
static RwReal   WingAreaOfPolygonScaleproof(RtGCondGeometryList* geometryList, RtWingFace* face, RwInt32 spaceType,
                            RwInt32 subSpaceType, RwBool useThisPerimeter, RwReal* perimeter);
static RwReal   WingSignedAreaOfTriangle(const RwV3d * const c0, const RwV3d * const c1,
                      const RwV3d * const c2, RwV3d* normal);
static RwReal   WingAngleBetweenPoints(RwV3d* a, RwV3d* b, RwV3d* c);
static RwBool   WingAestheticVertexMatch(RtGCondVertex* a, RtGCondVertex* b);
static RwBool   WingAestheticPolygonMatch(RtGCondPolygon* a, RtGCondPolygon* b);
static RwBool   WingRemoveDegeneratedFace(RtWingFace *face);
static void     WingRemoveColinearsFromEdgePair(RtWingEdge* refEdge);
static void     WingRemoveRemainingColinearsVerticesFromAll(RtWingEdge* refEdge, const RwInt32 pass);
static void     WingRemoveSpikes(RtWingEdge* refEdge);

#if(defined(WINGCHECK))
static RwBool WingConvex(RtWingFace* face, RwBool linearSafe);
static RwBool WingContainsNoVertex(RtWingVertex* a, RtWingVertex* b, RtWingVertex* c, RtWingFace *face);
static void WingSanityCheck(RtWings* wings);
static void WingTopologyCheck(RtWings* wings);
static RwReal WingAreaOfTriangle(const RwV3d * const c0, const RwV3d * const c1, const RwV3d * const c2);
#endif /*(defined(WINGCHECK))*/

/****************************************************************************
 Functions...
 */
static void
WingProliferateUVs(RtWingFace* wingFace,
                     RwTexCoords* offset,
                     RtWings* wings,
                     RtGCondGeometryList* geometryList,
                     RwBool ammendUVs,
                     RwBool* success)
{
    /* Given a face, it traverses all faces attached to this one, and all
     * faces attached to those and so one, like the flood-fill algorithm.
     * It locally terminates when a <C2 edge is found, i.e. crease or null,
     * or when a polygon that's already been treated is found. Use of the faces
     * value field is used for this and the user should be aware that the values are
     * changed...
     */

    /* Note to self, faces' values only only used via their sign, free to use the absolute value */
    RtWingEdge* edge;
    RwInt32 i, j;
    RtGCondParameters* gcParams = RtGCondParametersGet();

    RWFUNCTION(RWSTRING("WingProliferateUVs"));

    RWASSERT(NULL != wingFace);
    RWASSERT(wingFace->value >= 0.0f);

    /* If we've not already done this face */
    if (wingFace->value != wingInterationTag)
    {
        RtWingVertex* theVertex;

        wingFace->value = wingInterationTag;

        /* Update vertices around all edges... */
        edge = wingFace->edge;
        do
        {
            RWASSERT(edge->vertex->value >= 0.0f);
            if ((edge->vertex->value != wingInterationTag) && (edge->vertex->value >= 0.0f))
            {
                edge->vertex->value = wingInterationTag;
                /* Does so, unless it was done via a neighboring face */
                if (ammendUVs)
                {
                    for (i = 0; i < geometryList->numUVs; i++)
                    {
                        if ((rwTEXTUREADDRESSWRAP == gcParams->textureMode[i]) &&
                            ((!geometryList->matList.materials[edge->face->polygonInfo->matIndex]->texture) ||
                            (rwTEXTUREADDRESSWRAP == RwTextureGetAddressing(geometryList->matList.materials[edge->face->polygonInfo->matIndex]->texture))))
                        {
                            edge->vertex->vertexInfo->texCoords[i].u += offset[i].u;
                            edge->vertex->vertexInfo->texCoords[i].v += offset[i].v;
                        }
                    }
                }
                else
                {
                    for (i = 0; i < geometryList->numUVs; i++)
                    {
                        if ((rwTEXTUREADDRESSWRAP == gcParams->textureMode[i]) &&
                            ((!geometryList->matList.materials[edge->face->polygonInfo->matIndex]->texture) ||
                            (rwTEXTUREADDRESSWRAP == RwTextureGetAddressing(geometryList->matList.materials[edge->face->polygonInfo->matIndex]->texture))))
                        {
                            /* Make sure texcoords do not breech limits! */
                            if ((edge->vertex->vertexInfo->texCoords[i].u + offset[i].u > gcParams->uvLimitMax) ||
                                (edge->vertex->vertexInfo->texCoords[i].u + offset[i].u < gcParams->uvLimitMin) ||
                                (edge->vertex->vertexInfo->texCoords[i].v + offset[i].v > gcParams->uvLimitMax) ||
                                (edge->vertex->vertexInfo->texCoords[i].v + offset[i].v < gcParams->uvLimitMin))
                            {
                                *success = FALSE;
                            }
                        }
                    }
                }
            }
            edge = edge->next;
        }while(edge != wingFace->edge);

        /* Do for all faces connected to vertices of polygon we've just processed */
        edge = wingFace->edge;
        if (success)
        {
            do
            {
                theVertex = edge->vertex;
                for (j = 0; j < geometryList->vertices[theVertex->vertexInfo->index].numBrauxIndices; j++)
                {
                    if (geometryList->vertices[theVertex->vertexInfo->index].brauxIndices[j] != wingFace->polygonInfo->id)
                    {
                        WingProliferateUVs(&wings->wingFaces[geometryList->vertices[theVertex->vertexInfo->index].brauxIndices[j]],
                            offset, wings, geometryList, ammendUVs, success);
                    }
                }
                edge = edge->next;
            }while(edge != wingFace->edge);
        }
    }
    RWRETURNVOID();
}

/**
 * \ingroup rtwing
 * \ref RtWingFaceForAllEdges iterates over all edges belonging to the
 * given face. For each, it calls the given callback.
 *
 * \param wingFace a pointer to the face
 * \param callBack the callback
 * \param userData pointer to void for unknown-type userdata
 *
 * \return the face on success, otherwise NULL
 *
 * \see RtWingFaceForAllFaces
 * \see RtWingVertexForAllEdges
 * \see RtWingVertexForAllFaces
 */
RtWingFace*
RtWingFaceForAllEdges(RtWingFace* wingFace,
                        RtWingEdgeCallBack callBack,
                        void* userData)
{
    /* For all edges that define the face */
    RtWingEdge* edge;

    RWAPIFUNCTION(RWSTRING("RtWingFaceForAllEdges"));

    RWASSERT(NULL != wingFace);
    edge = wingFace->edge;
    do
    {
        if (!callBack(edge, userData))
        {
            wingFace = (RtWingFace *) NULL;
            break;
        }
        edge = edge->next;
    }while(edge != wingFace->edge);

    RWRETURN(wingFace);
}

/**
 * \ingroup rtwing
 * \ref RtWingFaceForAllFaces iterates over all faces neighboring the
 * given face. For each, it calls the given callback.
 *
 * \param wingFace a pointer to the face
 * \param callBack the callback
 * \param userData pointer to void for unknown-type userdata
 *
 * \return the face on success, otherwise NULL
 *
 * \see RtWingFaceForAllEdges
 * \see RtWingVertexForAllEdges
 * \see RtWingVertexForAllFaces
 */
RtWingFace*
RtWingFaceForAllFaces(RtWingFace* wingFace,
                        RtWingFaceCallBack callBack,
                        void* userData)
{
    /* For all faces that are edge-touching the given face */
    RtWingEdge* edge;

    RWAPIFUNCTION(RWSTRING("RtWingFaceForAllFaces"));

    RWASSERT(NULL != wingFace);
    edge = wingFace->edge;
    do
    {
        if ((edge->neighbor) && (!callBack(edge->neighbor->face, userData)))
        {
            wingFace = (RtWingFace *) NULL;
            break;
        }
        edge = edge->next;
    }while(edge != wingFace->edge);

    RWRETURN(wingFace);
}

/**
 * \ingroup rtwing
 * \ref RtWingVertexForAllEdges iterates over all edges belonging to the
 * given vertex. For each, it calls the given callback.
 *
 * \param wingVertex a pointer to the vertex
 * \param callBack the callback
 * \param userData pointer to void for unknown-type userdata
 *
 * \return the vertex on success, otherwise NULL
 *
 * \see RtWingFaceForAllEdges
 * \see RtWingFaceForAllFaces
 * \see RtWingVertexForAllFaces
 */
RtWingVertex*
RtWingVertexForAllEdges(RtWingVertex* wingVertex,
                        RtWingEdgeCallBack callBack,
                        void* userData)
{
    /* For all edges incidental to the vertex */
    RtWingEdge* edge;
    RwBool recycled = FALSE;
    RWAPIFUNCTION(RWSTRING("RtWingVertexForAllEdges"));

    RWASSERT(NULL != wingVertex);
    edge = wingVertex->edge;
    do
    {
        if (!callBack(edge, userData))
        {
            wingVertex = (RtWingVertex *) NULL;
            break;
        }
        if (edge->neighborRelation != rtWINGEDGENULL)
        {
            edge = edge->neighbor->next;
        }
        else
        {
            RWASSERT(!recycled);
            /* We got the the end of the current cycle and met a terminal edge,
             * let's quickly sweep backwards to the start and continue
             */
            while (edge->prev->neighborRelation != rtWINGEDGENULL)
            {
                edge = edge->prev->neighbor;
            }
            recycled = TRUE;
        }
    }while(edge != wingVertex->edge);

    RWRETURN(wingVertex);
}

#if(defined(WINGCHECK))
static RtWingEdge*
WingChangeVertexReference(RtWingEdge* wingEdge,
                        void* userData)
{
    RtWingVertex** pair = (RtWingVertex**)userData;

    RWFUNCTION(RWSTRING("WingChangeVertexReference"));

    if (wingEdge->prev->vertex == pair[0])
    {
        wingEdge->prev->vertex = pair[1];
    }
    RWRETURN(wingEdge);
}
#endif /*(defined(WINGCHECK))*/

/**
 * \ingroup rtwing
 * \ref RtWingVertexForAllFaces iterates over all faces belonging to the
 * given vertex. For each, it calls the given callback.
 *
 * \param wingVertex a pointer to the vertex
 * \param callBack the callback
 * \param userData pointer to void for unknown-type userdata
 *
 * \return the vertex on success, otherwise NULL
 *
 * \see RtWingFaceForAllEdges
 * \see RtWingFaceForAllFaces
 * \see RtWingVertexForAllEdges
 */
RtWingVertex*
RtWingVertexForAllFaces(RtWingVertex* wingVertex,
                        RtWingFaceCallBack callBack,
                        void* userData)
{
    /* For all faces incidental to the vertex */
    RtWingEdge* edge;

    RWAPIFUNCTION(RWSTRING("RtWingVertexForAllFaces"));

    RWASSERT(NULL != wingVertex);
    edge = wingVertex->edge;
    do
    {
        if (!callBack(edge->face, userData))
        {
            wingVertex = (RtWingVertex *) NULL;
            break;
        }
        if (edge->neighborRelation != rtWINGEDGENULL)
        {
            edge = edge->neighbor->next;
        }
        else
        {
            /* We got the the end of the current cycle and met a terminal edge,
             * let's quickly sweep backwards to the start and continue
             */
            while (edge->prev->neighborRelation != rtWINGEDGENULL)
            {
                edge = edge->prev->neighbor;
            }
            edge = edge->prev;
        }
    }while(edge != wingVertex->edge);

    RWRETURN(wingVertex);
}

static RwBool
WingAestheticVertexMatch(RtGCondVertex* a, RtGCondVertex* b)
{
    RwReal weldThreshold;
    RwReal normalThreshold;
    RwReal prelitThreshold;
    RtGCondParameters* gcParams = RtGCondParametersGet();

    RWFUNCTION(RWSTRING("WingAestheticVertexMatch"));

    weldThreshold = gcParams->weldThreshold;
    normalThreshold = gcParams->angularThreshold;
    prelitThreshold = gcParams->preLitThreshold;

    if (!WingEqualZeroTol(a->position.x - b->position.x, weldThreshold) ||
            !WingEqualZeroTol(a->position.y - b->position.y, weldThreshold) ||
            !WingEqualZeroTol(a->position.z - b->position.z, weldThreshold))
    {
        RWRETURN(FALSE);
    }

    if ((normalThreshold >= 0.0f && (gcParams->flags & rtGCONDNORMALS)) &&
        (!WingEqualZeroTol((RwReal)(RwACos(RwV3dDotProduct(&a->normal, &b->normal))) * 180.0f/3.14159265535f,
            normalThreshold)))
    {
        RWRETURN(FALSE);
    }

    if ((prelitThreshold >= 0.0f && (gcParams->flags & rtGCONDPRELIT)) &&
        ((!WingEqualZeroTol(((RwReal)(a->preLitCol.red) - (RwReal)(b->preLitCol.red)) / 255.0f, prelitThreshold)) ||
        (!WingEqualZeroTol(((RwReal)(a->preLitCol.green) - (RwReal)(b->preLitCol.green)) / 255.0f, prelitThreshold)) ||
        (!WingEqualZeroTol(((RwReal)(a->preLitCol.blue) - (RwReal)(b->preLitCol.blue)) / 255.0f, prelitThreshold)) ||
        (!WingEqualZeroTol(((RwReal)(a->preLitCol.alpha) - (RwReal)(b->preLitCol.alpha)) / 255.0f, prelitThreshold))))
    {
        RWRETURN(FALSE);
    }


    RWRETURN(TRUE);
}

static RwBool
WingAestheticPolygonMatch(RtGCondPolygon* a, RtGCondPolygon* b)
{
    RWFUNCTION(RWSTRING("WingAestheticPolygonMatch"));

    if (a->matIndex != b->matIndex)
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}

#if(defined(WINGCHECK))
static RwReal
WingAreaOfTriangle(const RwV3d * const c0, const RwV3d * const c1,
                      const RwV3d * const c2)
{
    RwReal             area2;
    RwV3d              e0, e1;
    RwV3d              n;

    RWFUNCTION(RWSTRING("WingAreaOfTriangle"));

    RwV3dSubMacro(&e0, c1, c2);
    RwV3dSubMacro(&e1, c2, c0);

    RwV3dCrossProductMacro(&n, &e0, &e1);
    area2 = RwV3dDotProductMacro(&n, &n);

    RWRETURN(area2);
}
#endif /*(defined(WINGCHECK))*/

static RwReal
WingSignedAreaOfTriangle(const RwV3d * const c0, const RwV3d * const c1,
                      const RwV3d * const c2, RwV3d* normal)
{
    RwReal             area2;
    RwV3d              e0, e1, n;

    RWFUNCTION(RWSTRING("WingSignedAreaOfTriangle"));

    RwV3dSubMacro(&e0, c2, c0);
    RwV3dSubMacro(&e1, c1, c0);

    RwV3dCrossProductMacro(&n, &e0, &e1);
    area2 = RtGCondLength(&n) / 2.0f;

    RtGCondNormalize(&n);
    if (RwV3dDotProduct(&n, normal) < 0.0f)
    {
        area2 = -area2;
    }

    RWRETURN(area2);
}

static RwReal
WingAreaOfPolygonScaleproof(RtGCondGeometryList* geometryList, RtWingFace* face, RwInt32 spaceType,
                            RwInt32 subSpaceType,
                            RwBool useThisPerimeter, RwReal* perimeter)
{
    RwReal             area2 = 0.0f;
    RwInt32            j;
    RwV3d              v[3];
    RwReal             cand[3];
    RwV3d              uv[3];
    RwV3d              vec;
    RtGCondPolygon*    polygon = face->polygonInfo;
    RwV3d              uvNormal = {0.0f, 0.0f, 1.0f};
    RwInt32            idents;

    RWFUNCTION(RWSTRING("WingAreaOfPolygonScaleproof"));


    if (!useThisPerimeter && spaceType == 0)
    {
        *perimeter = 0.0f;
        for (j = 1; j < polygon->numIndices; j++)
        {
            v[0] = geometryList->vertices[polygon->indices[j-1]].position;
            v[1] = geometryList->vertices[polygon->indices[j]].position;

            RwV3dSub(&vec, &v[0], &v[1]);
            *perimeter += RtGCondLength(&vec);
        }
    }
    for (j = 2; j < polygon->numIndices; j++)
    {
        if (spaceType == 0) /* 3-space */
        {
            v[0] = geometryList->vertices[polygon->indices[0]].position;
            v[1] = geometryList->vertices[polygon->indices[j-1]].position;
            v[2] = geometryList->vertices[polygon->indices[j]].position;

            v[0].x /= *perimeter;
            v[0].y /= *perimeter;
            v[0].z /= *perimeter;
            v[1].x /= *perimeter;
            v[1].y /= *perimeter;
            v[1].z /= *perimeter;
            v[2].x /= *perimeter;
            v[2].y /= *perimeter;
            v[2].z /= *perimeter;

            area2 += WingSignedAreaOfTriangle(&v[0], &v[1], &v[2], &face->normal);
        }
        else if (spaceType == 1) /* uv-space */
        {
            uv[0].x = geometryList->vertices[polygon->indices[0]].texCoords[subSpaceType].u;
            uv[0].y = geometryList->vertices[polygon->indices[0]].texCoords[subSpaceType].v;
            uv[0].z = 0.0f;
            uv[1].x = geometryList->vertices[polygon->indices[j-1]].texCoords[subSpaceType].u;
            uv[1].y = geometryList->vertices[polygon->indices[j-1]].texCoords[subSpaceType].v;
            uv[1].z = 0.0f;
            uv[2].x = geometryList->vertices[polygon->indices[j]].texCoords[subSpaceType].u;
            uv[2].y = geometryList->vertices[polygon->indices[j]].texCoords[subSpaceType].v;
            uv[2].z = 0.0f;


            /* If one pair are the same and not the remainder, return invalid area; if all
               three the same, return zero */
            idents = 0;
            idents += RtGCondVectorsEqual(&uv[0], &uv[1], GCONDZERO) ? 1 : 0;
            idents += RtGCondVectorsEqual(&uv[0], &uv[2], GCONDZERO) ? 1 : 0;
            idents += RtGCondVectorsEqual(&uv[1], &uv[2], GCONDZERO) ? 1 : 0;
            if (idents == 0)
            {
                area2 += WingSignedAreaOfTriangle(&uv[0], &uv[1], &uv[2], &uvNormal);
            }
            else if (idents < 3)
            {
                area2 = INVALIDAREAFLAG;
            }
        }
        else if (spaceType == 2) /* rgb-space */
        {
            uv[0] = geometryList->vertices[polygon->indices[0]].position;
            uv[1] = geometryList->vertices[polygon->indices[j-1]].position;
            uv[2] = geometryList->vertices[polygon->indices[j]].position;

            if (subSpaceType == 1)
            {
                cand[0] = geometryList->vertices[polygon->indices[0]].preLitCol.red;
                cand[1] = geometryList->vertices[polygon->indices[j-1]].preLitCol.red;
                cand[2] = geometryList->vertices[polygon->indices[j]].preLitCol.red;
            }
            else if (subSpaceType == 2)
            {
                cand[0] = geometryList->vertices[polygon->indices[0]].preLitCol.green;
                cand[1] = geometryList->vertices[polygon->indices[j-1]].preLitCol.green;
                cand[2] = geometryList->vertices[polygon->indices[j]].preLitCol.green;
            }
            else if (subSpaceType == 3)
            {
                cand[0] = geometryList->vertices[polygon->indices[0]].preLitCol.blue;
                cand[1] = geometryList->vertices[polygon->indices[j-1]].preLitCol.blue;
                cand[2] = geometryList->vertices[polygon->indices[j]].preLitCol.blue;
            }

            if (!WingEqualZero(face->normal.x))
            {
                uv[0].x = cand[0];
                uv[1].x = cand[1];
                uv[2].x = cand[2];
                uvNormal.x = 1.0f;
                uvNormal.y = 0.0f;
                uvNormal.z = 0.0f;
            }
            else if (!WingEqualZero(face->normal.y))
            {
                uv[0].y = cand[0];
                uv[1].y = cand[1];
                uv[2].y = cand[2];
                uvNormal.x = 0.0f;
                uvNormal.y = 1.0f;
                uvNormal.z = 0.0f;
            }
            else
            {
                uv[0].z = cand[0];
                uv[1].z = cand[1];
                uv[2].z = cand[2];
                uvNormal.x = 0.0f;
                uvNormal.y = 0.0f;
                uvNormal.z = 1.0f;
            }

            area2 += WingSignedAreaOfTriangle(&uv[0], &uv[1], &uv[2], &uvNormal);
        }
        else if (spaceType == 3) /* normal-space */
        {
            uv[0] = geometryList->vertices[polygon->indices[0]].position;
            uv[1] = geometryList->vertices[polygon->indices[j-1]].position;
            uv[2] = geometryList->vertices[polygon->indices[j]].position;

            if (subSpaceType == 1)
            {
                cand[0] = geometryList->vertices[polygon->indices[0]].normal.x;
                cand[1] = geometryList->vertices[polygon->indices[j-1]].normal.x;
                cand[2] = geometryList->vertices[polygon->indices[j]].normal.x;
            }
            else if (subSpaceType == 2)
            {
                cand[0] = geometryList->vertices[polygon->indices[0]].normal.y;
                cand[1] = geometryList->vertices[polygon->indices[j-1]].normal.y;
                cand[2] = geometryList->vertices[polygon->indices[j]].normal.y;
            }
            else if (subSpaceType == 3)
            {
                cand[0] = geometryList->vertices[polygon->indices[0]].normal.z;
                cand[1] = geometryList->vertices[polygon->indices[j-1]].normal.z;
                cand[2] = geometryList->vertices[polygon->indices[j]].normal.z;
            }

            if (!WingEqualZero(face->normal.x))
            {
                uv[0].x = cand[0];
                uv[1].x = cand[1];
                uv[2].x = cand[2];
                uvNormal.x = 1.0f;
                uvNormal.y = 0.0f;
                uvNormal.z = 0.0f;
            }
            else if (!WingEqualZero(face->normal.y))
            {
                uv[0].y = cand[0];
                uv[1].y = cand[1];
                uv[2].y = cand[2];
                uvNormal.x = 0.0f;
                uvNormal.y = 1.0f;
                uvNormal.z = 0.0f;
            }
            else
            {
                uv[0].z = cand[0];
                uv[1].z = cand[1];
                uv[2].z = cand[2];
                uvNormal.x = 0.0f;
                uvNormal.y = 0.0f;
                uvNormal.z = 1.0f;
            }

            area2 += WingSignedAreaOfTriangle(&uv[0], &uv[1], &uv[2], &uvNormal);
        }
    }

    RWRETURN(area2);
}

static RwBool
WingFacePairAreaUVRatioMatch(RtGCondGeometryList* geometryList, RtWingFace* a, RtWingFace* b)
{
    RwReal aArea;
    RwReal bArea;
    RwReal perim;
    RtGCondParameters* gcParams = RtGCondParametersGet();

    RWFUNCTION(RWSTRING("WingFacePairAreaUVRatioMatch"));

    /* When we test values other than topological area, we must test the fields (e.g. Red)
     * individually, since we cannot garuntee that all elements will be used and
     * we might have an 'aesthetic sliver'
     */

    /* Find the area of a and the area of a in UV/prelit/normal coordinates, and present as ratio,
       do same for b and compare */

    aArea = WingAreaOfPolygonScaleproof(geometryList, a, 0, 0, FALSE, &perim);
    bArea = WingAreaOfPolygonScaleproof(geometryList, b, 0, 0, TRUE, &perim);

    if (aArea == 0 || bArea == 0)
    {
        /* To small polygon to be able to calcuate right result. Z-code */
        RWRETURN(FALSE);
    }
    
    if ((gcParams->flags & rtGCONDTEXTURES) && gcParams->polyUVsThreshold >= 0.0f)
    {
        RwInt32 UVs;
        /* Assume we can use pair together */
        for (UVs = 0; UVs < geometryList->numUVs; UVs++)
        {
            RwReal aUVA  = WingAreaOfPolygonScaleproof(geometryList, a, 1, UVs, FALSE, NULL);
            RwReal bUVA = WingAreaOfPolygonScaleproof(geometryList, b, 1, UVs, FALSE, NULL);

            if((aUVA == INVALIDAREAFLAG || bUVA == INVALIDAREAFLAG) ||
                (!((WingEqualZero(aUVA) && WingEqualZero(bUVA)) ||
                    WingEqualZeroTol(((aArea / aUVA) / (bArea / bUVA)) - 1.0f, gcParams->polyUVsThreshold))))
            {
                RWRETURN(FALSE);
            }
        }
    }
    if ((gcParams->flags & rtGCONDNORMALS) && gcParams->polyNormalsThreshold >= 0.0f)
    {
        RwReal aNormal = WingAreaOfPolygonScaleproof(geometryList, a, 3, 1, TRUE, &perim);
        RwReal bNormal = WingAreaOfPolygonScaleproof(geometryList, b, 3, 1, TRUE, &perim);
        if (!((WingEqualZero(aNormal) && WingEqualZero(bNormal)) ||
            WingEqualZeroTol(((aArea / aNormal) / (bArea / bNormal)) - 1.0f, gcParams->polyNormalsThreshold)))
        {
            RWRETURN(FALSE);
        }

        aNormal = WingAreaOfPolygonScaleproof(geometryList, a, 3, 2, TRUE, &perim);
        bNormal = WingAreaOfPolygonScaleproof(geometryList, b, 3, 2, TRUE, &perim);
        if (!((WingEqualZero(aNormal) && WingEqualZero(bNormal)) ||
            WingEqualZeroTol(((aArea / aNormal) / (bArea / bNormal)) - 1.0f, gcParams->polyNormalsThreshold)))
        {
            RWRETURN(FALSE);
        }

        aNormal = WingAreaOfPolygonScaleproof(geometryList, a, 3, 3, TRUE, &perim);
        bNormal = WingAreaOfPolygonScaleproof(geometryList, b, 3, 3, TRUE, &perim);
        if (!((WingEqualZero(aNormal) && WingEqualZero(bNormal)) ||
            WingEqualZeroTol(((aArea / aNormal) / (bArea / bNormal)) - 1.0f, gcParams->polyNormalsThreshold)))
        {
            RWRETURN(FALSE);
        }
    }
    if ((gcParams->flags & rtGCONDPRELIT) && gcParams->polyPreLitsThreshold >= 0.0f)
    {
        RwReal aLight = WingAreaOfPolygonScaleproof(geometryList, a, 2, 1, TRUE, &perim);
        RwReal bLight = WingAreaOfPolygonScaleproof(geometryList, b, 2, 1, TRUE, &perim);
        if (!((WingEqualZero(aLight) && WingEqualZero(bLight)) ||
            WingEqualZeroTol(((aArea / aLight) / (bArea / bLight)) - 1.0f, gcParams->polyPreLitsThreshold)))
        {
            RWRETURN(FALSE);
        }

        aLight = WingAreaOfPolygonScaleproof(geometryList, a, 2, 2, TRUE, &perim);
        bLight = WingAreaOfPolygonScaleproof(geometryList, b, 2, 2, TRUE, &perim);
        if (!((WingEqualZero(aLight) && WingEqualZero(bLight)) ||
            WingEqualZeroTol(((aArea / aLight) / (bArea / bLight)) - 1.0f, gcParams->polyPreLitsThreshold)))
        {
            RWRETURN(FALSE);
        }

        aLight = WingAreaOfPolygonScaleproof(geometryList, a, 2, 3, TRUE, &perim);
        bLight = WingAreaOfPolygonScaleproof(geometryList, b, 2, 3, TRUE, &perim);
        if (!((WingEqualZero(aLight) && WingEqualZero(bLight)) ||
            WingEqualZeroTol(((aArea / aLight) / (bArea / bLight)) - 1.0f, gcParams->polyPreLitsThreshold)))
        {
            RWRETURN(FALSE);
        }
    }
    RWRETURN(TRUE);
}

static void
WingVertexToPolygonCopy(RtGCondGeometryList* geometryList, RwInt32 from, RwInt32 to)
{
    RwInt32 s, b;
    RwInt32* newPolygons;
    RwInt32 newNumPolygon;

    /* Copies the braux indices from one vertex to another 2> <2  becomes  0> <4*/

    RWFUNCTION(RWSTRING("WingVertexToPolygonCopy"));

    if (from != to)
    {
        newNumPolygon = geometryList->vertices[from].numBrauxIndices + geometryList->vertices[to].numBrauxIndices;
        newPolygons = (RwInt32*)RwMalloc(sizeof(RwInt32*) * newNumPolygon,
            rwID_WINGPLUGIN | rwMEMHINTDUR_EVENT);

        for (s = 0; s < geometryList->vertices[to].numBrauxIndices; s++)
        {
            newPolygons[s] = geometryList->vertices[to].brauxIndices[s];
        }
        for (b = 0; s < newNumPolygon; b++, s++)
        {
            newPolygons[s] = geometryList->vertices[from].brauxIndices[b];
        }
        RwFree(geometryList->vertices[to].brauxIndices);
        geometryList->vertices[to].brauxIndices = newPolygons;
        geometryList->vertices[to].numBrauxIndices = newNumPolygon;

        RwFree(geometryList->vertices[from].brauxIndices);
        geometryList->vertices[from].brauxIndices = NULL;
        geometryList->vertices[from].numBrauxIndices = 0;
    }

    RWRETURNVOID();
}

/**
 * \ingroup rtwing
 * \ref RtWingMatchUVsAndPromoteContinuity attempts to turn (aesthetically)
 * creased edges into contiguous ones by UV-translation. It is called via
 * RtWingEdgeDecimation to improve decimation.
 *
 * \param wings a pointer to the occupied winged edge data structure
 * \param geometryList a pointer to the list of geometry
 *
 * \see RtWingEdgeDecimation
 */
void
RtWingMatchUVsAndPromoteContinuity(RtWings* wings, RtGCondGeometryList* geometryList)
{
    RwInt32             i, q, p, pairs;
    RtWingEdge          *e;
    RtWingEdge          *cand;
    RtGCondParameters   *gcParams = RtGCondParametersGet();

    RWAPIFUNCTION(RWSTRING("RtWingMatchUVsAndPromoteContinuity"));

    /* We need to use the braux part of the data structure for
     * the proliferation of UVs: For each vertex - store indices
     * of all polygons that use it.
     */

    /* Note, since edges are tested this will not shoot itself in
     * the foot by connecting a pair of faces and then determining
     * it cannot connect any more, because if one pair fails, the other
     * usually succeeds. It might fail however because a different order
     * imposed might lead to potential concavities and these are avoided.
     */
    _rtGCondCreateVertexPolygonReference(geometryList, NULL);


    for (i = 0; i < wings->wingEdgesSize; i++)
    {
        for (pairs = 0; pairs < 2; pairs++)
        {
            if (pairs == 0)
            {
                e = &(wings->wingEdges[i]);
            }
            else
            {
                e = &(wings->wingEdges[i]);
                if (e->neighborRelation != 0)
                {
                    e = e->neighbor;
                }
                else
                {
                    break;
                }
            }
            if (e->value < 0)
            {
                /* Already tagged as deleted */
            }
            else if (e->neighborRelation != rtWINGEDGECONTINUOUS)
            {
                /* Anything that IS rtWINGEDGECONTINUOUS already has matching UVs by definition, atm
                 * we enter here and process things that should be rtWINGEDGECONTINUOUS but were
                 * not classified such because of UV (not topological) discontinuity
                 */
                /* First though, we need to add edge validation where neighbors are 'creased' according to virtual remapper
                 * but are actually coplanar and didn't get matched because uv's mismatch - this
                 * could be because of an integer offset (highly polygon dependent) that could not
                 * be calculated at the time
                 */
                if (e->neighborRelation == rtWINGEDGECREASE && WingAngleBetweenFaces(e->face, e->neighbor->face) < gcParams->polyNormalsThreshold)
                {
                    if ((WingAestheticVertexMatch(e->vertex->vertexInfo, e->neighbor->prev->vertex->vertexInfo)) &&
                        (WingAestheticVertexMatch(e->prev->vertex->vertexInfo, e->neighbor->vertex->vertexInfo)) &&
                        (WingAestheticPolygonMatch(e->face->polygonInfo, e->neighbor->face->polygonInfo)) &&
                        (WingVirtualUVMatch(geometryList, e->vertex->vertexInfo, e->neighbor->prev->vertex->vertexInfo)) &&
                        (WingVirtualUVMatch(geometryList, e->prev->vertex->vertexInfo, e->neighbor->vertex->vertexInfo)) &&
                        (WingVirtualUVEdgeMatch(geometryList, e, e->neighbor)) &&
                        (geometryList->numUVs == 0 || WingFacePairAreaUVRatioMatch(geometryList, e->face, e->neighbor->face)))
                    {
                        RtWingVertex* pair[2];
                        cand = e;

                        /* Just merge the edges, do not decimate on this go...*/
                        RWASSERT(cand->neighborRelation == rtWINGEDGECREASE && cand->neighbor->neighborRelation == rtWINGEDGECREASE);
                        /* CRITICAL: Project UVs on e->face to e->neighbor->face s.t. tilings are preserved */
                        if (WingProjectUVMatch(cand, wings, geometryList))
                        {

                            /* make sure nothing references the now called duplicated vertices */

                            pair[0] = cand->neighbor->vertex; /* from */
                            pair[1] = cand->prev->vertex; /* to */


                            if (pair[0] != pair[1])
                            {
                                pair[0]->value = -22.1f;/* Unreferenced dups */
                                /*RtWingVertexForAllEdges(pair[0],
                                    WingChangeVertexReference, (void*)pair);*/
                                /* Z-code... */
                                for (p = 0; p < pair[0]->vertexInfo->numBrauxIndices; p++)
                                {
                                    RtWingFace* theFace = &(wings->wingFaces[pair[0]->vertexInfo->brauxIndices[p]]);
                                    RtWingEdge* theEdge = theFace->edge;
                                    do
                                    {
                                        if (theEdge->vertex == pair[0])
                                        {
                                            theEdge->vertex = pair[1];
                                            RtWingUpdateInternalRepresentation(
                                                theFace,
                                                theEdge, theEdge->vertex);
                                        }
                                        theEdge = theEdge->next;
                                    }while(theEdge != theFace->edge);
                                }
                                if (TESTSAFEGUARD)
                                {
                                    for (q = 0; q < wings->wingEdgesSize; q++)
                                    {
                                        if (wings->wingEdges[q].vertex == pair[0])
                                        {
                                            RWASSERT(FALSE);
                                        }
                                    }
                                }
                            }
                            /* Need to copy list from pair[0] to pair[1] in wvp */
                            WingVertexToPolygonCopy(geometryList, pair[0]->vertexInfo->index, pair[1]->vertexInfo->index);

                            pair[0] = cand->neighbor->prev->vertex; /* from */
                            pair[1] = cand->vertex; /* to */


                            if (pair[0] != pair[1])
                            {
                                pair[0]->value = -22.2f;/* Unreferenced dups */
                                /*RtWingVertexForAllEdges(pair[0],
                                    WingChangeVertexReference, (void*)pair);*/
                                /* Z-code... */
                                for (p = 0; p < pair[0]->vertexInfo->numBrauxIndices; p++)
                                {
                                    RtWingFace* theFace = &(wings->wingFaces[pair[0]->vertexInfo->brauxIndices[p]]);
                                    RtWingEdge* theEdge = theFace->edge;
                                    do
                                    {
                                        if (theEdge->vertex == pair[0])
                                        {
                                            theEdge->vertex = pair[1];
                                            RtWingUpdateInternalRepresentation(
                                                theFace,
                                                theEdge, theEdge->vertex);
                                        }
                                        theEdge = theEdge->next;
                                    }while(theEdge != theFace->edge);
                                }

                                if (TESTSAFEGUARD)
                                {
                                    for (q = 0; q < wings->wingEdgesSize; q++)
                                    {
                                        if (wings->wingEdges[q].vertex == pair[0])
                                        {
                                            RWASSERT(FALSE);
                                        }
                                    }
                                }
                            }
                            /* Need to copy list from pair[0] to pair[1] in wvp */
                            WingVertexToPolygonCopy(geometryList, pair[0]->vertexInfo->index, pair[1]->vertexInfo->index);


                            /* Z-code */
        /*
                            cand->vertex = cand->neighbor->prev->vertex;
                            cand->prev->vertex = cand->neighbor->vertex;
          */
                            if (cand->vertex != cand->neighbor->prev->vertex ||
                                cand->prev->vertex != cand->neighbor->vertex)
                            {
                                RWASSERT(FALSE);
                            }


                            cand->neighborRelation = rtWINGEDGECONTINUOUS;
                            cand->neighbor->neighborRelation = rtWINGEDGECONTINUOUS;
                        }
                    }
                }
            }
        }
    }

    _rtGCondDestroyVertexPolygonReference(geometryList);

    RWRETURNVOID();
}


/**
 * \ingroup rtwing
 * \ref RtWingEdgeDecimation performs a sophisticated edge decimation algorithm
 * to reduced triangles into fewer, larger polygons, removing colinears and coplanars.
 *
 * \note The geometry must have been fixed and filtered before this stage is
 * called, as slivers and other special-case geometry can lead to improper
 * decimation, and in worst-cases, non-terminating loops.
 *
 * \param wings a pointer to the occupied winged edge data structure
 * \param geometryList a pointer to the list of geometry
 *
 * \see RtWingPartitionPolygon
 * \see RtGCondRemoveCollapsedFacesAndUnusedVertices
 * \see RtGCondCullZeroAreaPolygons
 * \see RtGCondRemapVertices
 * \see RtGCondLimitUVs
 */
void
RtWingEdgeDecimation(RtWings* wings, RtGCondGeometryList *geometryList)
{
    RwInt32 i = 0;
    RtWingEdge* cand;
    RwInt32 id = 0;
    RwInt32 pass;
    RtGCondParameters* gcParams = RtGCondParametersGet();

    RWAPIFUNCTION(RWSTRING("RtWingEdgeDecimation"));


    RWASSERT(wings->wingEdgesSize > 0);
#if (CHECKTOPOLOGY)
    WingTopologyCheck(wings);
#endif
#if (CHECKSANITY)
    WingSanityCheck(wings);
#endif

    /* This is equally, or even more important than WingEdgeRemoval itself,
       poor welding will result if this does not happen! */
    wingInterationTag = 1.0f;
    for (pass = 0; pass < gcParams->decimationPasses; pass++)
    {
        RtWingMatchUVsAndPromoteContinuity(wings, geometryList);
    }

#if (CHECKSANITY)
        WingSanityCheck(wings);
#endif
#if (CHECKTOPOLOGY)
    WingTopologyCheck(wings);
#endif
    if (WELDPOLYGONS)
    {
        /* Set-up colinear removal mode */
        /* wingRemoveColinearsOnCreasedEdge - can be done later,
         * doing it now limits our options for welding */
        if (gcParams->decimationMode == rtWINGEDGEDECIMATIONFEW)
        {
            wingRemoveColinearsOnCreasedEdge = FALSE;
            wingRemoveColinearsOnTerminalEdge = TRUE;
            wingRemoveColinearsOnContinuousEdge = TRUE;
            /* We fix colinears in creased edges later */
        }
        else /* rtWINGEDGEDECIMATIONSMALL */
        {
            wingRemoveColinearsOnCreasedEdge = FALSE;
            wingRemoveColinearsOnTerminalEdge = FALSE;
            wingRemoveColinearsOnContinuousEdge = TRUE;
            /* We do non-continuous edges later */
        }
        /* Edge decimation - NB All wings info preserved, but marked */
        for (pass = 0; pass < gcParams->decimationPasses; pass++)
        {
            id = 0;

            while (1)
            {
                i++;
                cand = WingEdgeCandidateIdentification(wings, geometryList, &id);
                /* To enhance: After first identification, just reidentify the edges that
                 * have been affected, then use proper evaluation, not b&w */
                if (cand)
                {
                    WingEdgeRemoval(cand);

                    /* There are many special cases that can occur as a result
                     * of WingEdgeRemoval. We should be careful when removing
                     * those special case, so that functions that do that,
                     * don't affect each other. So order of these functions is
                     * very important */
                    
                    /* Remove colinear points left behind the two edges removed. */
                    WingRemoveColinearsFromEdgePair(cand);
                    WingRemoveColinearsFromEdgePair(cand->neighbor);
                    
                    /* Remove degenerated faces that can occure as a result of
                     * WingRemoveColinearsFromEdgePair. See function comments. */
                    WingRemoveDegeneratedFace(cand->face);
                    WingRemoveDegeneratedFace(cand->neighbor->face);

                    /* Remove spikes. This function is independant from 
                     * WingRemoveColinearsFromEdgePair and WingRemoveDegeneratedFace
                     * and it will just exit if any of the points, left behind the
                     * two edges removed, was already removed. */
                    WingRemoveSpikes(cand);
                    WingRemoveSpikes(cand->neighbor);
                
                }
                else
                {
                    break;
                }
                #if (CHECKSANITY)
                    WingSanityCheck(wings);
                #endif
            }
        }
    }
#if (CHECKSANITY)
    WingSanityCheck(wings);
#endif
#if (CHECKTOPOLOGY)
    WingTopologyCheck(wings);
#endif

    /* As a final post-process, remove any remaining colinears from creased edges */
    if (gcParams->decimationMode == rtWINGEDGEDECIMATIONFEW)
    {
        wingRemoveColinearsOnCreasedEdge = TRUE;
        WingRemoveColinearVerticesFromWings(wings);
    }
    else /* rtWINGEDGEDECIMATIONSMALL */
    {
        RwInt32 p;
        for (p = 0; p < gcParams->decimationPasses; p++)
        {
            for (i = 0; i < wings->wingVerticesSize; i++)
            {
                if (wings->wingVertices[i].value >= 0.0f)
                {
                    wings->wingVertices[i].value = 0.0f;
                }
            }
            for (i = 0; i < wings->wingEdgesSize; i++)
            {
                if (wings->wingEdges[i].value >= 0.0f)
                {
                    WingRemoveRemainingColinearsVerticesFromAll(&wings->wingEdges[i], 0);
                }
            }
            for (i = 0; i < wings->wingEdgesSize; i += (p == 0 ? 2 : 1))
            {
                if (wings->wingEdges[i].value >= 0.0f)
                {
                    WingRemoveRemainingColinearsVerticesFromAll(&wings->wingEdges[i], 1);
                }
            }
        }
    }


#if (CHECKSANITY)
    WingSanityCheck(wings);
#endif
#if (CHECKTOPOLOGY)
    WingTopologyCheck(wings);
#endif

    RWRETURNVOID();
}

/**
 * \ingroup rtwing
 * \ref RtWingReallocateWingEdges allocate an extra number of edges to the wings.
 * In doing so, it may shift the base pointer, since the list is contiguous.
 *
 * \param wings a pointer to the occupied winged edge data structure
 * \param extraEdges the number of extra edges to be allocated
 *
 * \returns a pointer to the wingedEdges.
 *
 * \see RtWingReallocateWingFaces
 * \see RtWingReallocateWingVertices
 */
RtWingEdge*
RtWingReallocateWingEdges(RtWings* wings, RwInt32 extraEdges)
{
    RwInt32 gcondBaseOld, gcondBaseNew, gcondBaseShift;
    RwInt32 i;

    RWAPIFUNCTION(RWSTRING("RtWingReallocateWingEdges"));

    gcondBaseOld = (RwInt32)wings->wingEdges;
    wings->wingEdges = (RtWingEdge*)RwRealloc(wings->wingEdges,
                                sizeof(RtWingEdge) * (wings->wingEdgesSize + extraEdges),
                                rwID_WINGPLUGIN | rwMEMHINTDUR_EVENT |
                                rwMEMHINTFLAG_RESIZABLE);
    gcondBaseNew = (RwInt32)wings->wingEdges;
    gcondBaseShift = gcondBaseNew - gcondBaseOld;
    if (gcondBaseShift != 0)
    {
        for (i = 0; i < wings->wingEdgesSize; i++)
        {
            if (wings->wingEdges[i].next)
            {
                wings->wingEdges[i].next =
                    (RtWingEdge*)((RwInt32)(wings->wingEdges[i].next) +
                    gcondBaseShift);
            }
            if (wings->wingEdges[i].prev)
            {
                wings->wingEdges[i].prev =
                    (RtWingEdge*)((RwInt32)(wings->wingEdges[i].prev) +
                    gcondBaseShift);
            }
            if (wings->wingEdges[i].neighbor)
            {
                wings->wingEdges[i].neighbor =
                    (RtWingEdge*)((RwInt32)(wings->wingEdges[i].neighbor) +
                    gcondBaseShift);
            }
        }
        for (i = 0; i < wings->wingVerticesSize; i++)
        {
            if (wings->wingVertices[i].edge)
            {
                wings->wingVertices[i].edge =
                    (RtWingEdge*)((RwInt32)(wings->wingVertices[i].edge) +
                    gcondBaseShift);
            }
        }
        for (i = 0; i < wings->wingFacesSize; i++)
        {
            if (wings->wingFaces[i].edge)
            {
                wings->wingFaces[i].edge =
                    (RtWingEdge*)((RwInt32)(wings->wingFaces[i].edge) +
                    gcondBaseShift);
            }
        }
    }
    /* Initialize new ones */
    for (i = wings->wingEdgesSize; i < wings->wingEdgesSize + extraEdges; i++)
    {
        wings->wingEdges[i].value = -1.0f;
        wings->wingEdges[i].face = NULL;
        wings->wingEdges[i].vertex = NULL;
        wings->wingEdges[i].neighbor = NULL;
        wings->wingEdges[i].prev = NULL;
        wings->wingEdges[i].next = NULL;
        wings->wingEdges[i].neighborRelation = 0;
    }
    wings->wingEdgesSize += extraEdges;


    RWRETURN(wings->wingEdges);
}

/**
 * \ingroup rtwing
 * \ref RtWingReallocateWingVertices allocate an extra number of vertices to the wings.
 * It also allocates extra vertices in the geometryList.
 * In doing so, it may shift the base pointer, since the list is contiguous.
 *
 * \param wings a pointer to the occupied winged edge data structure
 * \param geometryList a pointer to the list of geometry
 * \param extraVertices the number of extra vertices to be allocated
 *
 * \returns a pointer to the wingedEdges, NULL if it fails.
 *
 * \see RtWingReallocateWingFaces
 * \see RtWingReallocateWingEdges
 */
RtWingVertex*
RtWingReallocateWingVertices(RtWings* wings, RtGCondGeometryList *geometryList,
                    RwInt32 extraVertices)
{
    RwInt32 gcondBaseOld, gcondBaseNew, gcondBaseShift;
    RwInt32 i, newNumber;

    RWAPIFUNCTION(RWSTRING("RtWingReallocateWingVertices"));

    /* Reallocate wing vertices and readdress them from wing edge */
    gcondBaseOld = (RwInt32)wings->wingVertices;
    wings->wingVertices = (RtWingVertex*)RwRealloc(wings->wingVertices,
                                sizeof(RtWingVertex) * (wings->wingVerticesSize + extraVertices),
                                rwID_WINGPLUGIN | rwMEMHINTDUR_EVENT |
                                rwMEMHINTFLAG_RESIZABLE);
    if (wings->wingVertices == NULL)    
    {
        /* Out of memory. */
        RWRETURN(NULL);
    }

    gcondBaseNew = (RwInt32)wings->wingVertices;
    gcondBaseShift = gcondBaseNew - gcondBaseOld;
    if (gcondBaseShift != 0)
    {
        for (i = 0; i < wings->wingEdgesSize; i++)
        {
            wings->wingEdges[i].vertex =
                (RtWingVertex*)((RwInt32)(wings->wingEdges[i].vertex) +
                gcondBaseShift);
        }
    }
    /* Initialize new ones */
    for (i = wings->wingVerticesSize; i < wings->wingVerticesSize + extraVertices; i++)
    {
        wings->wingVertices[i].value = -1.0f;
    }
    wings->wingVerticesSize += extraVertices;


    /* Reallocate internal vertices, and readdress them from the wings */
    gcondBaseOld = (RwInt32)geometryList->vertices;
    newNumber = geometryList->numVertices + extraVertices;

    if (!RtGCondReallocateVertices(geometryList, newNumber))
    {
        /* Out of memory. */
        RwFree(wings->wingVertices);
        RWRETURN(NULL);
    }

    gcondBaseNew = (RwInt32)geometryList->vertices;
    gcondBaseShift = gcondBaseNew - gcondBaseOld;

    if (gcondBaseShift != 0)
    {
        for (i = 0; i < wings->wingVerticesSize; i++)
        {
            wings->wingVertices[i].vertexInfo =
                (RtGCondVertex*)(gcondBaseNew + sizeof(RtGCondVertex) * i);
        }
    }

    RWRETURN(wings->wingVertices);
}

/**
 * \ingroup rtwing
 * \ref RtWingReallocateWingFaces allocate an extra number of faces to the wings.
 * It also allocates extra polygons in the geometryList.
 * In doing so, it may shift the base pointer, since the list is contiguous.
 *
 * \param wings a pointer to the occupied winged edge data structure
 * \param geometryList a pointer to the list of geometry
 * \param extraFaces the number of extra faces to be allocated
 *
 * \returns a pointer to the wingedEdges
 *
 * \see RtWingReallocateWingVertices
 * \see RtWingReallocateWingEdges
 */
RtWingFace*
RtWingReallocateWingFaces(RtWings* wings, RtGCondGeometryList *geometryList,
                    RwInt32 extraFaces)
{
    RwInt32 gcondBaseOld, gcondBaseNew, gcondBaseShift;
    RwInt32 wingBaseOld, wingBaseNew, wingBaseShift;
    RwInt32 i, newNumber;

    RWAPIFUNCTION(RWSTRING("RtWingReallocateWingFaces"));


    /* Reallocate wing faces and readdress them from wing edge */
    wingBaseOld = (RwInt32)wings->wingFaces;
    wings->wingFaces = (RtWingFace*)RwRealloc(wings->wingFaces,
                                sizeof(RtWingFace) * (wings->wingFacesSize + extraFaces),
                                rwID_WINGPLUGIN | rwMEMHINTDUR_EVENT |
                                rwMEMHINTFLAG_RESIZABLE);
    wingBaseNew = (RwInt32)wings->wingFaces;
    wingBaseShift = wingBaseNew - wingBaseOld;
    if (wingBaseShift != 0)
    {
        for (i = 0; i < wings->wingEdgesSize; i++)
        {
            if (wings->wingEdges[i].face)
            {
                wings->wingEdges[i].face =
                    (RtWingFace*)((RwInt32)(wings->wingEdges[i].face) +
                    wingBaseShift);
            }
        }
    }
    /* Initialize new ones */
    for (i = wings->wingFacesSize; i < wings->wingFacesSize + extraFaces; i++)
    {
        wings->wingFaces[i].value = -1.0f;
        wings->wingFaces[i].edge = NULL;
    }
    wings->wingFacesSize += extraFaces;



    /* Reallocate internal polygons, and readdress them from the wings */
    gcondBaseOld = (RwInt32)geometryList->polygons;
    newNumber = geometryList->numPolygons + extraFaces;

    RtGCondReallocatePolygons(geometryList, newNumber);

    gcondBaseNew = (RwInt32)geometryList->polygons;
    gcondBaseShift = gcondBaseNew - gcondBaseOld;

    if (gcondBaseShift != 0)
    {
        for (i = 0; i < wings->wingFacesSize; i++)
        {
            if (wings->wingFaces[i].polygonInfo)
            {
                wings->wingFaces[i].polygonInfo =
                    (RtGCondPolygon*)(gcondBaseNew + sizeof(RtGCondPolygon) * i);
            }
        }
    }
    for (i = geometryList->numPolygons - extraFaces; i < geometryList->numPolygons; i++)
    {
        wings->wingFaces[i].polygonInfo->numIndices = 0;
        wings->wingFaces[i].polygonInfo->matIndex = 0xFFFF;
        wings->wingFaces[i].polygonInfo->indices = NULL;
    }

    RWRETURN(wings->wingFaces);
}

/**
 * \ingroup rtwing
 * \ref RtWingConvexPartitioning partitions every polygon into
 * triangles.
 *
 * \note It is a critial assumption that the wings have enough unused edges
 * and faces, identified by negative value fields, to store the addition
 * geometry!
 *
 * \param wings a pointer to the occupied winged edge data structure
 * \param geometryList a pointer to the list of geometry
 * \param how one of \ref rtWINGEDGEPARTITIONFAN, \ref rtWINGEDGEPARTITIONTACK,
 *        \ref rtWINGEDGEPARTITIONEAR, specifying whether partitioning is done
 *        by fanning, tacking, or by progressive 'ear' partitioning. This is
 *        a platform-specific preference.
 *
 * \see RtWingPartitionPolygon
 */
void
RtWingConvexPartitioning(RtWings* wings, RtGCondGeometryList *geometryList, RtWingEdgePartitioning how)
{
    RwInt32 i;
    RwInt32 inputSize = wings->wingFacesSize;

    RWAPIFUNCTION(RWSTRING("RtWingConvexPartitioning"));
    /* Approach:
     *  For each polygon >3 vertices
     *     For each edge, traverse until a reflex angle is met, t1
     *     For each each, traverse backwards until a reflex angle is met, t2
     *     Repeat with remanent
     *
     */

    for (i = 0; i < inputSize; i++)
    {
        if (wings->wingFaces[i].value >= 0.0f)
        {
            RwInt32 ctr = 0;
            RtWingPartitionPolygon(wings, &wings->wingFaces[i], geometryList, how, ctr);
        }
    }
    RWRETURNVOID();
}


/**
 * \ingroup rtwing
 * \ref RtWingPartitionPolygon partitions a polygon into
 * triangles.
 *
 * \note It is a critial assumption that the wings have enough unused edges
 * and faces, identified by negative value fields, to store the addition
 * geometry!
 *
 * \param wings a pointer to the occupied winged edge data structure
 * \param face a pointer to the face to be partitioned
 * \param geometryList a pointer to the list of geometry
 * \param how one of \ref rtWINGEDGEPARTITIONFAN, \ref rtWINGEDGEPARTITIONTACK,
 *        \ref rtWINGEDGEPARTITIONEAR, specifying whether partitioning is done
 *        by fanning, tacking, or by progressive 'ear' partitioning. This is
 *        a platform-specific preference.
 * \param ctr an iteration counter that can be used to alter the effects of the
 *        partitioning approaches. Default zero.
 *
 * \see RtWingConvexPartitioning
 */
void
RtWingPartitionPolygon(RtWings* wings, RtWingFace* face,
                       RtGCondGeometryList* geometryList,
                       RtWingEdgePartitioning how, RwInt32 ctr)
{
    RtWingEdge* fwdEdge = NULL;
    RwV3d normal;

    RtWingEdge* temp;
    RtWingEdge* newEdge;
    RtWingEdge* newEdgeNeighbor;
    RtWingEdge* startEdge;
    RtWingFace* newFace;
    RwInt32 newWingEdgeIndex;
    RwInt32 newWingFaceIndex;
    RwInt32 newWingEdgeNeighborIndex;
    RwBool earFound;
    RwReal bestAngle = 1000.0f;
    RwReal testAngle;
    RtWingEdge* bestEdge = NULL;
    RwInt32 chunk = 100;
    RtGCondParameters *gcParams = RtGCondParametersGet();

    /* Go around the polygon, cutting off (concave-linked) ears until centre is convex */
    RWAPIFUNCTION(RWSTRING("RtWingPartitionPolygon"));

    if (face->polygonInfo->numIndices > 3)
    {
        /* GET NEW SPACE READY... */
        /* Find slots tagged as empty for the new geometry... */
        for (newWingEdgeIndex = 0; newWingEdgeIndex < wings->wingEdgesSize; newWingEdgeIndex++)
        {
            if (wings->wingEdges[newWingEdgeIndex].value < 0) break;
        }

        if (newWingEdgeIndex == wings->wingEdgesSize)
        {
            /* If we failed to find an empty slot, allocate more space - a 'chunk' for efficiency */
            RtWingReallocateWingEdges(wings, chunk);
        }
        RWASSERT (newWingEdgeIndex != wings->wingEdgesSize);/* Make sure we found an empty slot */
        wings->wingEdges[newWingEdgeIndex].value = 44.44f;



        for (newWingEdgeNeighborIndex = newWingEdgeIndex; newWingEdgeNeighborIndex < wings->wingEdgesSize; newWingEdgeNeighborIndex++)
        {
            if (wings->wingEdges[newWingEdgeNeighborIndex].value < 0) break;
        }

        if (newWingEdgeNeighborIndex == wings->wingEdgesSize)
        {
            /* If we failed to find an empty slot, allocate more space - a 'chunk' for efficiency */
            RtWingReallocateWingEdges(wings, chunk);
        }
        RWASSERT (newWingEdgeNeighborIndex != wings->wingEdgesSize);/* Make sure we found an empty slot */
        wings->wingEdges[newWingEdgeNeighborIndex].value = 44.44f;

        for (newWingFaceIndex = 0; newWingFaceIndex < wings->wingFacesSize; newWingFaceIndex++)
        {
            if (wings->wingFaces[newWingFaceIndex].value < 0) break;
        }

        if (newWingFaceIndex == wings->wingFacesSize)
        {
            /* If we failed to find an empty slot, allocate more space - a 'chunk' for efficiency */
            RwInt32 orig = (RwInt32)(wings->wingFaces);
            RwInt32 shift = (RwInt32)(RtWingReallocateWingFaces(wings, geometryList, chunk)) - orig;

            face = (RtWingFace*)((RwInt32)(face) + shift);
        }
        RWASSERT (newWingFaceIndex != wings->wingFacesSize);/* Make sure we found an empty slot */
        wings->wingFaces[newWingFaceIndex].value = 44.44f;



        /* FIND THE BEST NEW EDGES AND FACES... */
        earFound = FALSE;

        normal = face->normal;

        if (how == rtWINGEDGEPARTITIONEAR)
        {
            fwdEdge = face->edge;
        }
        else if (how == rtWINGEDGEPARTITIONFAN)
        {
            fwdEdge = face->edge->prev;
        }
        else if (how == rtWINGEDGEPARTITIONTACK)
        {
            if (ctr % 2 == 0)
            {
                fwdEdge = face->edge->prev;
            }
            else
            {
                fwdEdge = face->edge->prev->prev;
            }
        }
        startEdge = fwdEdge;
        do
        {
            if (/*concave req WingReflexTriVert(&fwdEdge->prev->vertex->vertexInfo->position, &fwdEdge->vertex->vertexInfo->position, &fwdEdge->next->vertex->vertexInfo->position, &normal) &&*/
                 WingIsAnEar(face, fwdEdge->vertex, fwdEdge->next->vertex,
                 fwdEdge->next->next->vertex, &normal) &&
                 (
                 (!RtGCondColinearVertices(&fwdEdge->prev->prev->vertex->vertexInfo->position,
                    &fwdEdge->prev->vertex->vertexInfo->position,
                    &fwdEdge->vertex->vertexInfo->position, FALSE, gcParams->polyNormalsThreshold)) ||
                    (!RtGCondColinearVertices(&fwdEdge->next->next->vertex->vertexInfo->position,
                    &fwdEdge->prev->vertex->vertexInfo->position,
                    &fwdEdge->vertex->vertexInfo->position, FALSE, gcParams->polyNormalsThreshold))
                    ))
            {
                earFound = TRUE;

                if (ctr != 0)
                {
                    bestEdge = fwdEdge;
                    break;
                }
                else
                {
                    testAngle = WingAngleBetweenPoints(&fwdEdge->vertex->vertexInfo->position,
                        &fwdEdge->next->vertex->vertexInfo->position,
                        &fwdEdge->next->next->vertex->vertexInfo->position);
                    if (testAngle + 0.001f < bestAngle)
                    {
                        bestAngle = testAngle;
                        bestEdge = fwdEdge;
                    }
                }

            }
            fwdEdge = fwdEdge->next;
        }while(fwdEdge != startEdge);



        /* DO ALL THE RECALCULATIONS... */
        if (!earFound || bestEdge == NULL)
        {
            /* disable for concave req. */
            earFound = TRUE;
            fwdEdge = startEdge;
        }
        else
        {
            fwdEdge = bestEdge;
        }
        if (earFound)
        {
            /* We have at least one PAIR */


            /* Create new geometry into empty slots */
            newEdge = &(wings->wingEdges[newWingEdgeIndex]);
            newEdgeNeighbor = &(wings->wingEdges[newWingEdgeNeighborIndex]);
            newFace = &(wings->wingFaces[newWingFaceIndex]);
            newFace->value = 1.0f;
            newFace->polygonInfo->id = newWingFaceIndex;
            newFace->polygonInfo->matIndex = face->polygonInfo->matIndex;
            newFace->polygonInfo->pUserData = face->polygonInfo->pUserData;
            newFace->normal = face->normal;
/*
            RtWingCopyElements(face, newFace);
            RtWingCopyElements(RtWingFace* from, RtWingFace* to)
            {

            }
*/
            /* Set up edge->vertex pointers */
            newEdge->vertex = fwdEdge->vertex;
            newEdgeNeighbor->vertex = fwdEdge->next->next->vertex;

            /* Direct new edges */
            newEdgeNeighbor->next = fwdEdge->next->next->next;
            newEdgeNeighbor->prev = fwdEdge;
            newEdge->next = fwdEdge->next;
            newEdge->prev = fwdEdge->next->next;

            newEdgeNeighbor->prev->next = newEdgeNeighbor;
            newEdgeNeighbor->next->prev = newEdgeNeighbor;
            newEdge->prev->next = newEdge;
            newEdge->next->prev = newEdge;

            /* Update the rest */
            /* !! */
            newEdge->face = newFace;
            newEdgeNeighbor->face = newEdgeNeighbor->next->face;
            /* edge->neighbor */
            newEdge->neighbor = newEdgeNeighbor;
            newEdgeNeighbor->neighbor = newEdge;
            newEdge->neighborRelation = rtWINGEDGECONTINUOUS;
            newEdgeNeighbor->neighborRelation = rtWINGEDGECONTINUOUS;
            /* face->edge */
            newEdgeNeighbor->face->edge = newEdgeNeighbor;
            newEdge->face->edge = newEdge;

            /* Now update it internally, so that there is one convex polygon and the rest */
            /*
             *
             *
             */
            if (GCUserDataCallBacks.splitPolygonUserdata)
            {
                GCUserDataCallBacks.splitPolygonUserdata(&newFace->polygonInfo->pUserData,
                                                        &face->polygonInfo->pUserData);
            }
            temp = newEdge;

            /* If we free the indices, we can remove the polygon with a post filter */
            if (temp->face->polygonInfo->numIndices)
            {
                RtGCondFreeIndices(temp->face->polygonInfo);
            }
            do
            {
                /* redo internal */
                temp->face = newEdge->face;
                RWASSERT(temp->face == newFace);
                RtGCondReallocateIndices(temp->face->polygonInfo, temp->face->polygonInfo->numIndices + 1);
                temp->face->polygonInfo->indices[temp->face->polygonInfo->numIndices - 1] =
                    temp->vertex->vertexInfo->index;

                temp = temp->next;
            }while (temp != newEdge);
            RWASSERT(temp->face->polygonInfo->numIndices >= 3);

            /* If we free the indices, we can remove the polygon with a post filter */
            if (newEdgeNeighbor->face->polygonInfo)
            {
                RtGCondFreeIndices(newEdgeNeighbor->face->polygonInfo);
            }

            newEdgeNeighbor->face->polygonInfo->indices = NULL;
            temp = newEdgeNeighbor;
            do
            {
                temp->face = newEdgeNeighbor->face;
                /* redo internal */
                if (temp->face->polygonInfo->numIndices != 0)
                {
                    RtGCondReallocateIndices(temp->face->polygonInfo,
                        temp->face->polygonInfo->numIndices + 1);
                }
                else
                {
                    RtGCondAllocateIndices(temp->face->polygonInfo,
                        temp->face->polygonInfo->numIndices + 1);
                }
                temp->face->polygonInfo->indices[temp->face->polygonInfo->numIndices - 1] =
                    temp->vertex->vertexInfo->index;

                temp = temp->next;
            }while (temp != newEdgeNeighbor);
            RWASSERT(temp->face->polygonInfo->numIndices >= 3);

            RWASSERT(newFace->edge->next->next->next == newFace->edge); /* check IsTriangle */

            /* RWASSERT(WingConvex(face, TRUE)); */
            /* RWASSERT(WingConvex(newFace, TRUE)); */

            /* Call the function again with the remanent */
            if (newEdgeNeighbor->face->polygonInfo->numIndices > 3)
            {
                RtWingPartitionPolygon(wings, newEdgeNeighbor->face, geometryList, how, ++ctr);
            }
        }
    }
    RWRETURNVOID();
}

static RwBool
WingVirtualUVMatch(RtGCondGeometryList* geometryList, RtGCondVertex* a, RtGCondVertex* b)
{
    RwInt32 i;
    RtGCondParameters* gcParams = RtGCondParametersGet();

    RWFUNCTION(RWSTRING("WingVirtualUVMatch"));


    for (i = 0; i < geometryList->numUVs; i++)
    {
        if (gcParams->textureMode[i] == rwTEXTUREADDRESSWRAP)
        {
            if (!(WingEqualZero(WingRemainder(a->texCoords[i].u) - WingRemainder(b->texCoords[i].u))))
            {
                RWRETURN(FALSE);
            }
            if (!(WingEqualZero(WingRemainder(a->texCoords[i].v) - WingRemainder(b->texCoords[i].v))))
            {
                RWRETURN(FALSE);
            }
        }
        else
        {
            if (!(WingEqualZero((a->texCoords[i].u) - (b->texCoords[i].u))))
            {
                RWRETURN(FALSE);
            }
            if (!(WingEqualZero((a->texCoords[i].v) - (b->texCoords[i].v))))
            {
                RWRETURN(FALSE);
            }
        }
    }
    RWRETURN(TRUE);
}

static RwBool
WingVirtualUVEdgeMatch(RtGCondGeometryList* geometryList, RtWingEdge* a, RtWingEdge* b)
{
    RwInt32 i;

    RWFUNCTION(RWSTRING("WingVirtualUVEdgeMatch"));

    for (i = 0; i < geometryList->numUVs; i++)
    {
        if (!WingEqualZero((a->prev->vertex->vertexInfo->texCoords[i].u - a->vertex->vertexInfo->texCoords[i].u) -
            (b->vertex->vertexInfo->texCoords[i].u - b->prev->vertex->vertexInfo->texCoords[i].u)))
        {
            RWRETURN(FALSE);
        }
        if (!WingEqualZero((a->prev->vertex->vertexInfo->texCoords[i].v - a->vertex->vertexInfo->texCoords[i].v) -
            (b->vertex->vertexInfo->texCoords[i].v - b->prev->vertex->vertexInfo->texCoords[i].v)))
        {
            RWRETURN(FALSE);
        }
    }
    RWRETURN(TRUE);
}

static RwBool
WingProjectUVMatch(RtWingEdge* e, RtWings* wings, RtGCondGeometryList* geometryList)
{
    /* E.g...
        ...___1,1 0,1/________1,1               ...___1,1 1,1/________2,1
               /|S|            /|                      /|S|            /|
                |A|             |                       |A|             |
               e|M|e->n         |       ->             e|M|e->n         |
                |E|             |                       |E|             |
                |E|/            |                       |E|/            |
        ...__\1,0 0,0________\1,0               ...__\1,0 1,0________\2,0

    */
    /* assert that neighbor is topologically identical */
    RwInt32 i;
    RwTexCoords offset[rwMAXTEXTURECOORDS];
    RwBool success = TRUE;
    RwInt32 silly = 2 << 15;
    RtGCondParameters* gcParams = RtGCondParametersGet();

    RWFUNCTION(RWSTRING("WingProjectUVMatch"));


    if ((e->vertex->vertexInfo->texCoords[0].u > silly &&
        e->neighbor->vertex->vertexInfo->texCoords[0].u > silly) ||
        (e->vertex->vertexInfo->texCoords[0].u < -silly &&
        e->neighbor->vertex->vertexInfo->texCoords[0].u < -silly))
    {
        /* UV's are already garbage! */
        RWRETURN(TRUE);
    }

    for (i = 0; i < geometryList->numUVs; i++)
    {
        if ((rwTEXTUREADDRESSWRAP == gcParams->textureMode[i]) &&
                ((!geometryList->matList.materials[e->face->polygonInfo->matIndex]->texture) ||
                (rwTEXTUREADDRESSWRAP == RwTextureGetAddressing(geometryList->matList.materials[e->face->polygonInfo->matIndex]->texture))))
        {
            /* Find the offset, in the example it is 1,0 */
            offset[i].u = e->prev->vertex->vertexInfo->texCoords[i].u -
                            e->neighbor->vertex->vertexInfo->texCoords[i].u;
            offset[i].v = e->prev->vertex->vertexInfo->texCoords[i].v -
                            e->neighbor->vertex->vertexInfo->texCoords[i].v;
        }
        else
        {
            /* Not used, but helps bug watching */
            offset[i].u = 0.0f;
            offset[i].v = 0.0f;
        }
    }
    /* Now proliferate changes (We need to increment next door poly, and then
     * all C2 polygons attached to it) - note, first call is dry run to see
     * if we can do it without UV's exceeding limits
     */
    WingProliferateUVs(e->neighbor->face, offset, wings, geometryList, FALSE, &success);
    if (success)
    {
        wingInterationTag += 1.0f;
        WingProliferateUVs(e->neighbor->face, offset, wings, geometryList, TRUE, &success);
        RWASSERT(success);
    }
    wingInterationTag += 1.0f;

    RWRETURN(success);
}

static RtWingEdge*
WingEdgeCandidateIdentification(RtWings* wings, RtGCondGeometryList* geometryList, RwInt32* id)
{

    RtWingEdge          *e;
    RwInt32             i;
    RwReal              best = 0.0f;
    RtWingEdge          *candidate = NULL;
    RtGCondParameters   *gcParams = RtGCondParametersGet();

    RWFUNCTION(RWSTRING("WingEdgeCandidateIdentification"));

    for (i = *id; i < wings->wingEdgesSize; i++)
    {
        e = &(wings->wingEdges[i]);
        if (e->value < 0)
        {
            /* Already tagged as deleted */
        }
        else if (e->neighborRelation != rtWINGEDGECONTINUOUS)
        {
            /* We remove creases and terminal edge colinears later, but these affect the
             * silhouette so are not candidates for removal.
             */
            e->value = 0.0f;
        }
        else if ((e->neighbor == e->next) || (e->neighbor == e->prev))
        {
            /* An isolated edge, spike. We'll remove it. */
            e->value = 1.0f;
        }
        else
        {
            /* Quick evaluation, all polygons must be in the same plane */
            if ((e->face != e->neighbor->face /* Z-code */) &&
                WingAngleBetweenFaces(e->face, e->neighbor->face) < gcParams->polyNormalsThreshold)
            {
                if ((e->face == e->neighbor->face) &&
                    (e->next != e->neighbor) &&
                    (e->prev != e->neighbor))
                {
                    /* i.e. if either side of edge is the same face, but it's not a redandant inner edge */
                    /* Cannot remove this edge, because it is the split in a 2-d torus, if we connected
                     * it, the inner and outer circumferences would be disjoint leading to two opposite
                     * facing solid inner parts - and we don't want that now do we?
                     */
                    e->value = 0.0f;
                }
                else
                {
                    RtWingEdge* enteringEdge; /* To make sure none of the two potential concavities are introduced */
                    RtWingEdge* leavingEdge;

                    enteringEdge = e;
                    while (enteringEdge->prev == enteringEdge->neighbor->next->neighbor)
                    {
                        /* Only gets here if the edge is part of a colinear edge */
                        /*
                         * |              |
                         *  \--e--+-nxt--/
                         *  /-nbr-+------\
                         * |              |
                         */
                        enteringEdge = enteringEdge->prev;
                        
                        /* Check if we didn't get into a loop. */
                        if (enteringEdge == e)
                        {
                            break;
                        }
                    }
                    leavingEdge = e;
                    while (leavingEdge->next == leavingEdge->neighbor->prev->neighbor)
                    {
                        /* Only gets here if the edge is part of a colinear edge */
                        leavingEdge = leavingEdge->next;

                        /* Check if we didn't get into a loop. */
                        if (leavingEdge == e)
                        {
                            break;
                        }
                    }

                    /* For now, we'll only remove edges that preserve convexity: to enhance */
                    if ((ALLOWCONCAVE) ||
                        ((!WingReflexTriVert(&enteringEdge->prev->prev->vertex->vertexInfo->position, &enteringEdge->prev->vertex->vertexInfo->position, &enteringEdge->neighbor->next->vertex->vertexInfo->position, &e->face->normal, TRUE)) &&
                        (!WingReflexTriVert(&leavingEdge->neighbor->prev->prev->vertex->vertexInfo->position, &leavingEdge->neighbor->prev->vertex->vertexInfo->position, &leavingEdge->next->vertex->vertexInfo->position, &e->face->normal, TRUE))))
                    {
                        if (WingFacePairAreaUVRatioMatch(geometryList, e->face, e->neighbor->face))
                        {
                            RWASSERT(e->face->polygonInfo->matIndex == e->neighbor->face->polygonInfo->matIndex);
                            if ((WingAestheticVertexMatch(e->vertex->vertexInfo, e->neighbor->prev->vertex->vertexInfo)) &&
                                (WingAestheticVertexMatch(e->prev->vertex->vertexInfo, e->neighbor->vertex->vertexInfo)) &&
                                (WingAestheticPolygonMatch(e->face->polygonInfo, e->neighbor->face->polygonInfo)))
                            {
                                e->value = 1.0f;
                            }
                            else
                            {
                                e->value = 0.0f;
                            }
                        }
                        else
                        {
                            e->value = 0.0f;
                        }
                    }
                    else
                    {
                        e->value = 0.0f;
                    }
                }
            }
            else
            {
                e->value = 0.0f;
            }
        }
        if (e->value > best)
        {
            candidate = e;
            best = e->value;
            *id = i;
            break;/* Z-code: To enhance */
        }
    }

    RWRETURN(candidate);
}

/* Removes an edge pair. The way it removes an edge pair is by just welding
 * the two vertices on the top and the bottom. So in case of a quad grid,
 *     +----++----+      +----+----+
 *     |    ||    |  ->  |         |    +/++ for C0/C1 edges
 *     +----++----+      +----+----+   
 * it'll all be nice, but in case the pair is just a part of a chain of pairs
 * in other words if pairs of edges before or after these ones are neighbors,
 *     +----++----+      +----++----+
 *     |    ||    |      |          |
 *     +    ++    +  ->  +    ++    +
 *     |    ||    |      |    ||    |   +/++ for C0/C1 edges
 *     +----++----+      +----++----+
 * then it will leave spikes.
 *
 * Note: WingEdgeRemoval just removes a given edge. It doesn't do any
 * checking if the face will still be valid after the removal. For example,
 * After removing this edge pair, the polygon will not be valid anymore.
 *     +----++----+
 *     |          |
 *     +    ++    +
 *     |    ||    |
 *     +    ++    +
 *     |  ->||    |     +/++ for C0/C1 edges
 *     +----++----+ 
 * Garding against that is currenlty responsibility of the
 * WingEdgeCandidateIdentification function.
 */
static void
WingEdgeRemoval(RtWingEdge* theEdge)
{
    RtWingEdge  *tempEdge, *theNeighbor;

    RWFUNCTION(RWSTRING("WingEdgeRemoval"));

    RWASSERT(theEdge->neighbor);

    theNeighbor = theEdge->neighbor;

    /*  It's an isolated spike. Removal of spikes is a bit different. */
    if ((theNeighbor == theEdge->next) || (theNeighbor == theEdge->prev))
    {
        RtWingEdge *enteringEdge;

        /* Find the entering edge */
        if (theNeighbor == theEdge->next)
        {
            enteringEdge = theEdge;
        }
        else
        {
            enteringEdge = theNeighbor;
        }

        /* Update loop of edges. */
        enteringEdge->prev->next = enteringEdge->next->next;
        enteringEdge->next->next->prev = enteringEdge->prev;
        
        /* Update the face, in case it referenced that edge. */
        enteringEdge->face->edge = enteringEdge->prev;

        /* Update the vertex. */
        enteringEdge->prev->vertex->edge = enteringEdge->next->next;

        /* Tag things as deleted. */
        enteringEdge->value = -12.0f;
        enteringEdge->next->value = -12.0f;
        enteringEdge->vertex->value = -12.0f;
        
        /* Update the face and internal rep... */
        RtWingUpdateInternalRepresentation(enteringEdge->face,
                enteringEdge->prev, enteringEdge->prev->vertex);

        RWRETURNVOID();
    }    
    
    /* Update loop of edges, minus edge in middle */
    theEdge->prev->next = theNeighbor->next;
    theNeighbor->next->prev = theEdge->prev;
    theEdge->next->prev = theNeighbor->prev;
    theNeighbor->prev->next = theEdge->next;

    /* Update the faces. No need to update theNeighbor->face
     * since if different it'll be deleted. */
    theEdge->face->edge = theEdge->next;

    /* Update the vertices. Assert if the ones we need were deleted. */
    RWASSERT((theEdge->prev->vertex->value >= 0) &&
             (theNeighbor->prev->vertex->value >= 0));
    theEdge->prev->vertex->edge = theNeighbor->next;
    theNeighbor->prev->vertex->edge = theEdge->next;

    /* Tag things so we do not use them again */
    theNeighbor->value = -13.1f;
    theEdge->value = -13.2f;

    /* There are more vertices to delete in case this was a crease edge. */
    if (theEdge->neighborRelation == rtWINGEDGECREASE)
    {
        theEdge->vertex->value = -13.3f;
        theNeighbor->vertex->value = -13.4f;
    }

    /* If the faces were different, delete the second one. */
    if (theNeighbor->face != theEdge->face)
    {
        theNeighbor->face->value = -1.07f;
        RtGCondReallocateIndices(theNeighbor->face->polygonInfo, 0);
    }

    /* Update all the edges to point to the same face. */
    tempEdge = theEdge->next;
    do
    {
        tempEdge->face = theEdge->face;
        tempEdge = tempEdge->next;
    } while (tempEdge != theEdge->next);

    /* Update the face and internal rep... */
    RtWingUpdateInternalRepresentation(theEdge->face,
            theEdge->next, theEdge->next->vertex);

    /* Pedantic asserts */
    /* RWASSERT(edge->face->polygonInfo->numIndices >= 3);
    RWASSERT(WingConvex(edge->face, TRUE));*/
    
    RWRETURNVOID();
}

static RwReal
WingAngleBetweenPoints(RwV3d* a, RwV3d* b, RwV3d* c)
{
    RwV3d e1, e2;

    RWFUNCTION(RWSTRING("WingAngleBetweenPoints"));

    e1.x = a->x - b->x;
    e1.y = a->y - b->y;
    e1.z = a->z - b->z;

    e2.x = b->x - c->x;
    e2.y = b->y - c->y;
    e2.z = b->z - c->z;

    RtGCondNormalize(&e1);
    RtGCondNormalize(&e2);

    RWRETURN(RwV3dDotProduct(&e1, &e2));
}

static RwV3d
WingFindNormal(RtGCondGeometryList *geometryList, RtGCondPolygon* poly)
{
    /* Assumes convex, designed for triangles */
    RwV3d norm;
    RwV3d* a = &geometryList->vertices[poly->indices[0]].position;
    RwV3d* b = &geometryList->vertices[poly->indices[1]].position;
    RwV3d* c = &geometryList->vertices[poly->indices[2]].position;
    RwV3d e1, e2;

    RWFUNCTION(RWSTRING("WingFindNormal"));

    RWASSERT((a != b) && (b != c) && (c != a));

    e1.x = a->x - b->x;
    e1.y = a->y - b->y;
    e1.z = a->z - b->z;

    e2.x = b->x - c->x;
    e2.y = b->y - c->y;
    e2.z = b->z - c->z;

    RtGCondNormalize(&e1);
    RtGCondNormalize(&e2);
    RwV3dCrossProduct(&norm, &e1, &e2);
    RtGCondNormalize(&norm);
    /* norm.x = 0;norm.y = 1.0;norm.z = 0; */
    RWRETURN(norm);
}

/**
 * \ingroup rtwing
 * \ref RtWingDestroy destroy the winged edge representation.
 *
 * \param wings a pointer to the unoccupied winged edge data structure
 */
void
RtWingDestroy(RtWings* wings)
{
    RWAPIFUNCTION(RWSTRING("RtWingDestroy"));

    RwFree(wings->wingEdges);
    wings->wingEdges = NULL;
    wings->wingEdgesSize = 0;

    RwFree(wings->wingFaces);
    wings->wingFaces = NULL;
    wings->wingFacesSize = 0;

    RwFree(wings->wingVertices);
    wings->wingVertices = NULL;
    wings->wingVerticesSize = 0;

    RWRETURNVOID();
}

/**
 * \ingroup rtwing
 * \ref RtWingCreate takes a geometry list and turns it into a winged
 * edge representation. The wings hold connectivity information as well
 * as pointers to the original data in the geometry list.
 *
 * \note Any operation on the wing edge representation actions the
 * original data to be concurrently updated to reflect modification in
 * the wing relations!
 *
 * \param wings a pointer to the unoccupied winged edge data structure
 * \param geometryList a pointer to the list of geometry
 *
 * \return TRUE is it successful, FALSE otherwise
 */
RwBool
RtWingCreate(RtWings* wings, RtGCondGeometryList* geometryList)
{
    RwInt32             i, j, w, fi, wingEdgeCount, *vRemapper;
    RtWingFace          *p;
    RtGCondParameters   *gcParams = RtGCondParametersGet();
    WingVertexUsageList *vertexUsageList;

    RWAPIFUNCTION(RWSTRING("RtWingCreate"));

    wings->wingFacesSize = geometryList->numPolygons;
    wings->wingVerticesSize = geometryList->numVertices;
    wings->wingEdgesSize = 0;

    /* PART 1: O(n) Assign references to polys and verts, and create edges for each poly. */
    wings->wingFaces = (RtWingFace*)RwMalloc(sizeof(RtWingFace) * wings->wingFacesSize,
                            rwID_WINGPLUGIN | rwMEMHINTDUR_EVENT | rwMEMHINTFLAG_RESIZABLE);
    if (wings->wingFaces == NULL)
    {
        RWRETURN(FALSE);
    }

    wingEdgeCount = 0;
    for (i = 0; i < wings->wingFacesSize; i++)
    {
        wings->wingFaces[i].polygonInfo = &geometryList->polygons[i]; /* face->info */
        wings->wingFaces[i].normal = WingFindNormal(geometryList, wings->wingFaces[i].polygonInfo);
        wings->wingFaces[i].value = 0.0f;
        wings->wingFaces[i].edge = NULL; /* These are assigned later */

        wingEdgeCount += geometryList->polygons[i].numIndices;
    }

    wings->wingVertices = (RtWingVertex*)RwMalloc(sizeof(RtWingVertex) *
                                            wings->wingVerticesSize,
                                            rwID_WINGPLUGIN | rwMEMHINTDUR_EVENT |
                                            rwMEMHINTFLAG_RESIZABLE);
    if (wings->wingVertices == NULL)
    {
        RWRETURN(FALSE);
    }

    for (i = 0; i < wings->wingVerticesSize; i++)
    {
        wings->wingVertices[i].vertexInfo = &geometryList->vertices[i]; /* vertex->info */
        wings->wingVertices[i].value = 0.0f;
        wings->wingVertices[i].edge = NULL; /* These are assigned later */
    }

    /* No edges will be shared per se with the half edge structure */
    wings->wingEdges = (RtWingEdge*)RwMalloc(sizeof(RtWingEdge) *
                                            wingEdgeCount,
                                            rwID_WINGPLUGIN | rwMEMHINTDUR_EVENT |
                                            rwMEMHINTFLAG_RESIZABLE);
    if (wings->wingEdges == NULL)
    {
        RWRETURN(FALSE);
    }

    /* Initialize the edges */
    for (i = 0; i < wings->wingEdgesSize; i++)
    {
        wings->wingEdges[i].neighbor = NULL;
        wings->wingEdges[i].neighborRelation = 0;
        wings->wingEdges[i].vertex = NULL;
        wings->wingEdges[i].face = NULL;
        wings->wingEdges[i].next = NULL;
        wings->wingEdges[i].prev = NULL;
        wings->wingEdges[i].value = -1;
    }

    w = 0;
    fi = 0;
    for (i = 0; i < wings->wingFacesSize; i++)
    {
        RwInt32 numIndices = wings->wingFaces[i].polygonInfo->numIndices;

        /* make face point to an edge */
        wings->wingFaces[i].edge = &(wings->wingEdges[w]);

        for (j = 0; j < numIndices; j++)
        {
            /* CREATING EDGES */
            wings->wingEdgesSize++;

            wings->wingEdges[w].vertex = &(wings->wingVertices[wings->wingFaces[i].polygonInfo->indices[j]]);
            wings->wingEdges[w].neighbor = NULL;
            wings->wingEdges[w].neighborRelation = rtWINGEDGENULL;
            wings->wingEdges[w].face = &(wings->wingFaces[i]);
            wings->wingEdges[w].next = (j != numIndices - 1) ? &(wings->wingEdges[w + 1]) : &(wings->wingEdges[w - j]);
            wings->wingEdges[w].prev = (j != 0) ? &(wings->wingEdges[w - 1]) : &(wings->wingEdges[w + (numIndices - 1)]);
            wings->wingEdges[w].vertex->edge = wings->wingEdges[w].next;/* guarantee the vertex->edge points away */

            wings->wingEdges[w].vertex->value = 0.0f;
            wings->wingEdges[w].value = (RwReal)fi++;
            wings->wingEdges[w].face->value = 0.0f;
            w++;
        }
    }

    /* PART 2: O(n log n) Create lookups ready to check for relationships.
     * Cater for two neighbouring triangles with distinct edges. vRemapper
     * will just map verts 4 to vert 4 if it's not welded with anyone, or
     * to vertex 1, if it's welded with vertex 1. ref: virtual remapper */
    vRemapper = RtGCondWeldVerticesPipelineNode(geometryList, gcParams->weldThreshold,
                    -1, -1, -1, /* positional only */ TRUE, TRUE, TRUE, FALSE);

    vertexUsageList = (WingVertexUsageList*)
                                    RwMalloc(sizeof(WingVertexUsageList) *
                                    wings->wingVerticesSize,
                                    rwID_WINGPLUGIN | rwMEMHINTDUR_FUNCTION);
    if (vertexUsageList == NULL)
    {
        RWRETURN(FALSE);
    }

    /* For every vertex (v) of RtGCondGeometry store a list of polygons
     * that use it in element v. You may have cases when the
     * gcParams->weldThreshold is very high, that two verts of the same
     * face map to one vertex (degenerated triangle) But we'll put the
     * same face multiple times in the vertexUsageList[i].wingFaces
     * because both vertices have different local indices, and we need
     * then in the next part when we create edge-pair relationships. */
    for (i = 0; i < wings->wingVerticesSize; i++)
    {
        /* Initialize it */
        vertexUsageList[i].wingFaces = NULL;
        vertexUsageList[i].numPolygons = 0;
        vertexUsageList[i].startLocalIndices = NULL;
    }

    /* first figure how big each array will be */
    p = wings->wingFaces;
    for (i = 0; i < wings->wingFacesSize; i++)
    {
        for (j = 0; j < p->polygonInfo->numIndices; j++)
        {
            RwInt32 vulIndex = vRemapper[p->polygonInfo->indices[j]];
            vertexUsageList[vulIndex].numPolygons++;
        }
        p++;
    }

    /* then allocate them */
    for (i=0; i<wings->wingVerticesSize; i++)
    {
        RwInt32 numPolygons = vertexUsageList[i].numPolygons;

        vertexUsageList[i].wingFaces = (RtWingFace**)RwMalloc(sizeof(RtWingFace*) * numPolygons, 
                                        rwID_WINGPLUGIN | rwMEMHINTDUR_FUNCTION);
        vertexUsageList[i].startLocalIndices = (RwInt32*)RwMalloc(sizeof(RwInt32) * numPolygons,
                                        rwID_WINGPLUGIN | rwMEMHINTDUR_FUNCTION);

        vertexUsageList[i].numPolygons = 0;
    }

    /* then populate each array (we're gauranteed not to overflow) */
    p = wings->wingFaces;
    for (i = 0; i < wings->wingFacesSize; i++)
    {
        for (j = 0; j < p->polygonInfo->numIndices; j++)
        {
            RwInt32 vulIndex = vRemapper[p->polygonInfo->indices[j]];
            RwInt32 numPolygons = vertexUsageList[vulIndex].numPolygons;

            vertexUsageList[vulIndex].wingFaces[numPolygons] = p;
            vertexUsageList[vulIndex].startLocalIndices[numPolygons] = j;
            vertexUsageList[vulIndex].numPolygons++;
        }
        p++;
    }

    /* PART 3: O(n) Create edge-pair relationships... */
    for (i = 0; i < wings->wingVerticesSize; i++)
    {
        /* For every polygon (P) in each list */
        for (j = 0; j < vertexUsageList[i].numPolygons; j++)
        {
            RwInt32     jx;
            RtWingFace  *polyP;
            RtWingFace  *polyQ;
            RtWingEdge  *tempP;
            RtWingEdge  *tempQ;

            /* Compare P against the rest of the polygons, Q */
            for (jx = j+1; jx < vertexUsageList[i].numPolygons; jx++)
            {
                RwInt32 pIndexPO, qIndexPO, pIndexMO, qIndexMO;
                RwInt32 pIndex = vertexUsageList[i].startLocalIndices[j];
                RwInt32 qIndex = vertexUsageList[i].startLocalIndices[jx];

                polyP = vertexUsageList[i].wingFaces[j];
                polyQ = vertexUsageList[i].wingFaces[jx];

                /* See the comment in the previous big for loop. We'll have
                 * this case when the gcParams->weldThreshold is high enough
                 * so two or more vertices of the same face, end up
                 * mapping to the same remapped vertex. */
                if (polyP == polyQ)
                {
                    continue;
                }
                
                pIndexPO = pIndex + 1;
                if (pIndexPO >= polyP->polygonInfo->numIndices) pIndexPO = 0;
                pIndexMO = pIndex - 1;
                if (pIndexMO == -1) pIndexMO = polyP->polygonInfo->numIndices-1;
                qIndexPO = qIndex + 1;
                if (qIndexPO >= polyQ->polygonInfo->numIndices) qIndexPO = 0;
                qIndexMO = qIndex - 1;
                if (qIndexMO == -1) qIndexMO = polyQ->polygonInfo->numIndices-1;

                RWASSERT(vRemapper[polyP->polygonInfo->indices[pIndex]] ==
                    vRemapper[polyQ->polygonInfo->indices[qIndex]]);

                /* Ideally we should have 'or' this with the same clause but using
                 * pIndexPO and qIndexMO. Here we test only prevP and nextQ verts
                 * but not the nextP and prevQ. It will still work, because, for
                 * two edges to be neighbors, it takes two vertices to be shared,
                 * so in here we always go prevP, and for one of the verts it'll
                 * get the right edge.*/
                if ((vRemapper[polyP->polygonInfo->indices[pIndexMO]] ==
                    vRemapper[polyQ->polygonInfo->indices[qIndexPO]]))
                {
                    RwInt32 ii, qTarget;
                    /* one edge from P matches one edge from vul[jx] and is directed opposite */

                    /* We need to find the correct edges on P and Q... */
                    tempP = polyP->edge;
                    for (ii = 0; ii < pIndex; ii++)
                    {
                        tempP = tempP->next;
                    }

                    tempQ = polyQ->edge;
                    qTarget = qIndex + 1;
                    while (qTarget >= polyQ->polygonInfo->numIndices)
                    {
                        qTarget -= polyQ->polygonInfo->numIndices;
                    }
                    for (ii = 0; ii < qTarget; ii++)
                    {
                        tempQ = tempQ->next;
                    }

                    /* Edge pair: update the edge pair to reference each other. NB These
                     * might not be NULL and could have been assigned earlier
                     * this shows that the object is non-manifold+ where an edge
                     * has more than TWO faces attached.
                     */
                    if ((tempP->neighbor == NULL) && (tempQ->neighbor == NULL))
                    {
                        tempP->neighbor = tempQ;
                        tempQ->neighbor = tempP;
                        /* RWASSERT(polyP->polygonInfo->matIndex == polyQ->polygonInfo->matIndex); */

                        tempP->neighborRelation = ((polyP->polygonInfo->indices[pIndex] == polyQ->polygonInfo->indices[qIndex]) &&
                            (polyP->polygonInfo->indices[pIndexMO] == polyQ->polygonInfo->indices[qIndexPO])) ? rtWINGEDGECONTINUOUS : rtWINGEDGECREASE;
                        tempQ->neighborRelation = tempP->neighborRelation;
                    }
                }
            }
        }
    }
    RwFree(vRemapper);
    vRemapper = NULL;

    for (i = 0; i < wings->wingVerticesSize; i++)
    {
        if (vertexUsageList[i].wingFaces)
        {
            RwFree(vertexUsageList[i].wingFaces);
            vertexUsageList[i].wingFaces = NULL;
        }
        if (vertexUsageList[i].startLocalIndices)
        {
            RwFree(vertexUsageList[i].startLocalIndices);
            vertexUsageList[i].startLocalIndices = NULL;
        }
    }
    RwFree(vertexUsageList);
    vertexUsageList = NULL;

    RWRETURN(TRUE);
}

/**
 * \ingroup rtwing
 * \ref RtWingUpdateInternalRepresentation recalculates the internal geometry
 * (RtGCondPolygon) after edges have been reordered. It should be called after
 * any modification to a face, to maintain a valid internal representation.
 *
 * \param face A pointer to the face.
 * \param edge A pointer an edge of the face, this will be the first edge
 *             in the internal representation.
 * \param vertex A pointer to a vertex of the face, usually face->vertex.
 */
void
RtWingUpdateInternalRepresentation(RtWingFace* face, RtWingEdge* edge, RtWingVertex* vertex)
{
    RwInt32 num = 0, i;
    RtWingEdge* temp;

    RWAPIFUNCTION(RWSTRING("RtWingUpdateInternalRepresentation"));

    /* Currently this function expects edge give to be an existing edge, not the one
     * that was just removed. Also a vertex given should be edge->vertex. These
     * limitation should probably be removed and the function should take just the
     * first two parameters Z-code */
    temp = edge;
    do
    {
        num++;
        temp = temp->next;
    }while (temp->value >= 0 && temp->vertex != vertex);

    RtGCondReallocateIndices(face->polygonInfo, num);
    face->polygonInfo->numIndices = num;

    temp = edge;
    for (i = 0; i < num; i++)
    {
        face->polygonInfo->indices[i] = temp->vertex->vertexInfo->index;
        temp = temp->next;
    }

    RWRETURNVOID();
}

static void
WingRemoveColinearVerticesFromWings(RtWings* wings)
{
    RtWingVertex* vert;
    RwInt32 i;

    RWFUNCTION(RWSTRING("WingRemoveColinearVerticesFromWings"));

    for (i = 0; i < wings->wingVerticesSize; i++)
    {
        vert = &(wings->wingVertices[i]); /* the i'th vertex */

        WingRemoveColinearsFromEdgePair(vert->edge);
    }
    RWRETURNVOID();
}

static RwInt32
WingCountColinearBackAndForthOfEdge(RtWingEdge* edge)
{
    RwInt32 forwardCount = 0;
    RwInt32 backwardCount = 0;
    RtWingEdge* cand;
    RtGCondParameters   *gcParams = RtGCondParametersGet();

    RWFUNCTION(RWSTRING("WingCountColinearBackAndForthOfEdge"));

    cand = edge;
    while (RtGCondColinearVertices(&cand->prev->vertex->vertexInfo->position,
        &cand->vertex->vertexInfo->position,
        &cand->next->vertex->vertexInfo->position,
        TRUE,
        gcParams->polyNormalsThreshold))
    {
        forwardCount++;
        cand = cand->next;
    }

    cand = edge;
    while (RtGCondColinearVertices(&cand->prev->prev->vertex->vertexInfo->position,
        &cand->prev->vertex->vertexInfo->position,
        &cand->vertex->vertexInfo->position,
        TRUE,
        gcParams->polyNormalsThreshold))
    {
        backwardCount++;
        cand = cand->prev;
    }

    RWRETURN(forwardCount + backwardCount);
}

/* Removes a degenerated face. A face is degenerated if it has two vertices. We'll
 * cover just a clean case where face has two vertices and two edges, but not the
 * cases with one vert or just one edge.
 *
 * One way a degenerated face can appear is when in this case:
 *    +
 *   /|\
 *   - - 
 * inner edge is removed and then the top vertex is removed as being colinear
 * within the trashold provided.
 */
static RwBool
WingRemoveDegeneratedFace(RtWingFace *face)
{
    RtWingEdge  *e1, *e2;                            
    
    RWFUNCTION(RWSTRING("WingRemoveDegeneratedFace"));
    
    /* Return if not a degenerated face, if a face has
     * only one edge or if it has more then two edges */
    if ((face->polygonInfo->numIndices != 2) || 
        (face->edge == face->edge->next) ||
        (face->edge != face->edge->next->next))
    {
        RWRETURN(FALSE);
    }

    /* Delete the polygon */
    RtGCondReallocateIndices(face->polygonInfo, 0);

    /* Tag inner edges as deleted. */
    face->edge->value = -14.0f;
    face->edge->next->value = -14.0f;
    
    /* These edges should survive and the inner ones should be removed. */
    e1 = face->edge->neighbor; 
    e2 = face->edge->next->neighbor; 
    
    /* Update the neighbors. We'll just remove the two inner ones */
    if (e1 != NULL)
    {
        e1->neighbor = e2;
        if (e1->neighbor == NULL)
        {
            e1->neighborRelation = rtWINGEDGENULL;
        }
    }
    if (e2 != NULL)
    {
        e2->neighbor = e1;
        if (e2->neighbor == NULL)
        {
            e2->neighborRelation = rtWINGEDGENULL;
        }
    }

    /* If both edges exist, update the neighbor relationship.
     * If both were smooth, we'll leave them that way, if one
     * was crease we'll make them both. There is possibility
     * that they've  both been crease but could become smooth
     * now, but it's probably extreme case and it's unclear
     * if we should really remove an edge from there. */
    if (e1 != NULL && e2 != NULL)
    {
        if ((e1->neighborRelation == rtWINGEDGECREASE) || 
            (e2->neighborRelation == rtWINGEDGECREASE))
        {
            e1->neighborRelation = rtWINGEDGECREASE;
            e2->neighborRelation = rtWINGEDGECREASE;
        }
    }

    RWRETURN(TRUE);
}


void
WingRemoveSpikes(RtWingEdge* refEdge)
{
    /* The case we are considering here is:
     *  +<---++<---------+
     *  |     .<-refEdge |
     *  +    ++          +
     *  |    ||          |
     *  +    ++          +
     *  |    ||          |   +/++ for C0/C1 edges
     *  +--->++--------->+
     */
    RtWingEdge      *edge;

    RWFUNCTION(RWSTRING("WingRemoveSpikes"));

    /* First spike candidate. */
    edge = refEdge->next;
    if (edge == NULL)
    {
        RWRETURNVOID();
    }
    
    /* Remove as many spikes as possible. */
    while (edge == edge->prev->neighbor)
    {
        /* If the vertex was already removed as a result of
         * previous functions that handle special cases, exit.
         * This shouldn't happen, but if it does, it'll be in
         * the first iteration. */
        if (edge->prev->vertex->value < 0.0f)
        {
            RWRETURNVOID();
        }
        
        /* Remove the edge pair. */
        WingEdgeRemoval(edge);

        /* Iterate to the next spike candidate. */
        edge = edge->next;
    }

    /* Update the face and internal rep, if we made any changes. */
    if (edge != refEdge->next)
    {
        RtWingUpdateInternalRepresentation(edge->face, edge, edge->vertex);
    }

    RWRETURNVOID();
}

void
WingRemoveColinearsFromEdgePair(RtWingEdge* refEdge)
{
    /* (The ref edge is not of the tripple) - it was removed
     * and is no longer part of the wing edge data structure
     *       edge  prev
     *     +<----+<----+
     *     |     . <-  |
     *     +---->+---->+   . is where refEdge stood
     */
    RtWingVertex        *vert = refEdge->prev->vertex;
    RtWingVertex        *startVertex;
    RwV3d               v1, v2, v3;
    RtWingEdge          *edge = vert->edge;
    RtWingEdge          *prev = vert->edge->prev;
    RtWingFace          *face = vert->edge->face;
    RtGCondParameters   *gcParams = RtGCondParametersGet();

    RWFUNCTION(RWSTRING("WingRemoveColinearsFromEdgePair"));

    /* Exit if the edge was deleted or the face has only three verts. */
    if ((refEdge->value < 0) || (face->polygonInfo->numIndices <= 3))
    {
        RWRETURNVOID();
    }
    
    v1 = prev->prev->vertex->vertexInfo->position;
    v2 = vert->vertexInfo->position;
    v3 = edge->vertex->vertexInfo->position;

    if (!RtGCondColinearVertices(&v1, &v2, &v3, TRUE, gcParams->polyNormalsThreshold))
    {
        RWRETURNVOID();
    }

    /* We'll enter her if both edges are not smooth and if
     * (the edge is terminal) or (neighbor->next is terminal) or
     * (they are circular +=+=+) Z-code */
    if ((prev->neighborRelation != rtWINGEDGECONTINUOUS) &&
        (edge->neighborRelation != rtWINGEDGECONTINUOUS) &&
        (edge->neighborRelation == rtWINGEDGENULL                 ||
         edge->neighbor->next->neighborRelation == rtWINGEDGENULL ||
         edge->neighbor->next->neighbor->next == edge))
    {
        /* Terminal or crease */
        /* Second line of if statement checks for one-edge emission */
        RwBool creased = ((prev->neighborRelation == rtWINGEDGECREASE) &&
                          (edge->neighborRelation == rtWINGEDGECREASE));

        if (!creased && wingRemoveColinearsOnTerminalEdge)
        {
            /* Tag */
            vert->value = -2.1f;
            vert->edge->value = -3.1f;
            /* Make the updates */
            edge->prev->vertex = edge->vertex;
            edge->next->prev = edge->prev;
            edge->prev->next = edge->next;

            if (face->edge == edge)
            {
                face->edge = edge->next;
            }

            startVertex = edge->vertex;
            RtWingUpdateInternalRepresentation(face, edge, startVertex);
        }
        else if (creased && wingRemoveColinearsOnCreasedEdge)
        {
            /* So both edges have neighbors and they are creased. */
            RtWingEdge* edge2 = vert->edge->neighbor->next;
            RtWingEdge* prev2 = edge2->prev;
            RtWingFace* face2 = edge2->face;
            RwInt32 pair;

            RWASSERT(vert->edge->neighbor->neighbor == vert->edge);
            RWASSERT(vert->edge->next->prev == vert->edge);
            RWASSERT(vert->edge->prev->next == vert->edge);
            RWASSERT((prev->neighbor->neighborRelation == rtWINGEDGECREASE) &&
                     (edge->neighbor->neighborRelation == rtWINGEDGECREASE));

            v1 = edge2->prev->prev->vertex->vertexInfo->position;
            v2 = edge2->prev->vertex->vertexInfo->position;
            v3 = edge2->vertex->vertexInfo->position;
            if (RtGCondColinearVertices(&v1, &v2, &v3, TRUE,
                    gcParams->polyNormalsThreshold)) /* Z-code */
            {
                /* Tag */
                vert->value = -2.1f;
                vert->edge->value = -3.1f;
                /* Make the updates */
                edge->prev->vertex = edge->vertex;
                edge->next->prev = edge->prev;
                edge->prev->next = edge->next;

                edge2->prev->vertex->value = -2.1f;
                edge2->prev->vertex->edge->value = -3.1f;
                edge2->prev->vertex = edge2->vertex;
                edge2->next->prev = edge2->prev;
                edge2->prev->next = edge2->next;

                if (face->edge == edge)
                {
                    face->edge = edge->next;
                }

                if (face2->edge == edge2)
                {
                    face2->edge = edge2->next;
                }

                for (pair = 0; pair < 2; pair++)
                {
                    if (pair == 0)
                    {
                        startVertex = edge->vertex;
                    }
                    else
                    {
                        edge = edge2;
                        prev = prev2;
                        face = face2;
                        startVertex = edge->vertex;
                    }
                    RtWingUpdateInternalRepresentation(face, edge, startVertex);
                }
            }
        }
    }
    else if ((wingRemoveColinearsOnContinuousEdge)            &&
             (prev->neighborRelation == rtWINGEDGECONTINUOUS) &&
             (edge->neighborRelation == rtWINGEDGECONTINUOUS) &&
             (edge->neighbor->next->neighbor != NULL)         &&
             (edge->neighbor->next->neighbor->next == edge))
    {
        /* This is the most common case +=+=+ Both edges
         * are smooth and neighbors are conected too.  */
        RtWingEdge  *nedge;
        RwInt32     pairs;

        edge = vert->edge;
        nedge = edge->neighbor->next;
        
        /* Tag */
        vert->value = -2.15f; /* remove later */
        edge->value = -3.15f;
        edge->neighbor->next->value = -4.15f;
        
        /* Make the updates */
        edge->prev->vertex = edge->vertex;
        edge->neighbor->vertex = nedge->vertex;
        edge->next->prev = edge->prev;
        edge->neighbor->next = nedge->next;
        edge->prev->next = edge->next;
        nedge->next->prev = edge->neighbor;
        edge->prev->neighbor = edge->neighbor;
        edge->neighbor->neighbor = edge->prev;
        /*
        if (vert->edge->neighbor->neighbor->value <= 0)
        {
            vert->edge->neighbor->neighbor = vert->edge->next
        }*/
        if (face->edge == vert->edge)
        {
            face->edge = edge->next;
        }
        if (nedge->face->edge == nedge)
        {
            nedge->face->edge = nedge->next;
        }
        for (pairs = 0; pairs < 2; pairs++)
        {
            if (pairs == 0)
            {
                edge = vert->edge;
                face = edge->face;
                startVertex = edge->vertex;
            }
            else
            {
                edge = vert->edge->neighbor;
                face = edge->face;
                startVertex = edge->vertex;
            }
            RtWingUpdateInternalRepresentation(face, edge, startVertex);
        }
    }
    RWRETURNVOID();
}

void
WingRemoveRemainingColinearsVerticesFromAll(RtWingEdge* refEdge, const RwInt32 pass)
{
    /* Here ref edge is +--R->--+------+ */
    RwV3d v1, v2, v3;
    RtWingVertex* vert = refEdge->vertex;
    RtWingVertex* startVertex;
    RtWingFace* face = refEdge->face;
    RtGCondParameters   *gcParams = RtGCondParametersGet();

    RWFUNCTION(RWSTRING("WingRemoveRemainingColinearsVerticesFromAll"));

    v1 = refEdge->prev->vertex->vertexInfo->position;
    v2 = vert->vertexInfo->position;
    v3 = refEdge->next->vertex->vertexInfo->position;

    if (!RtGCondColinearVertices(&v1, &v2, &v3, TRUE, gcParams->polyNormalsThreshold))
    {
        RWRETURNVOID();
    }
    if ((refEdge->neighborRelation != rtWINGEDGECONTINUOUS) && (refEdge->next->neighborRelation != rtWINGEDGECONTINUOUS) &&
        (refEdge->neighborRelation == rtWINGEDGENULL || refEdge->neighbor->next->neighborRelation == rtWINGEDGENULL ||
            refEdge->neighbor->next->neighbor->next == refEdge))
    {
        RwBool creased = ((refEdge->neighborRelation == rtWINGEDGECREASE) && (refEdge->next->neighborRelation == rtWINGEDGECREASE));

        if (pass == 0)
        {
            if (creased)
            {
                vert->value = 7.0f;
            }
            else
            {
                vert->value = 5.0f;
            }
        }
        else
        {
            /* Special check for potential tri-fan of slivers, e.g. * - DNR
             *                                          0------5-------5*----0
             *                                          |                    |
             *                                          |                    |
             *                                          |                    |
             *                                          0--7--7--0--0--0--0--0
             *                                          |  |  |  | /   |   \ |
             */
            RtWingEdge* temp;
            RwInt32 terminalCount = 0, creaseCount = 0, ridgeCount = 0, thisCount, thatCount;
            RwBool preserveThis = FALSE, preserveThat = FALSE;

            terminalCount = 0;
            creaseCount = 0;
            ridgeCount = 0;
            temp = refEdge;
            do
            {
                if (temp->vertex->value == 5.0f)
                {
                    terminalCount++;
                }
                else if (temp->vertex->value == 7.0f)
                {
                    creaseCount++;
                }
                else
                {
                    ridgeCount++;
                }
                temp = temp->next;
            }while (temp != refEdge);
            thisCount = WingCountColinearBackAndForthOfEdge(refEdge);
            preserveThis = (thisCount/*terminalCount*/ < creaseCount + ridgeCount - 4 - thisCount);

            if (creased)
            {
                terminalCount = 0;
                creaseCount = 0;
                ridgeCount = 0;

                temp = refEdge->neighbor;
                if (temp->value < 0.0f)
                {
                    RWRETURNVOID();
                }

                do
                {
                    if (temp->vertex->value == 5.0f)
                    {
                        terminalCount++;
                    }
                    else if (temp->vertex->value == 7.0f)
                    {
                        creaseCount++;
                    }
                    else
                    {
                        ridgeCount++;
                    }
                    temp = temp->next;
                }while (temp != refEdge->neighbor);
                thatCount = WingCountColinearBackAndForthOfEdge(refEdge->neighbor);
                preserveThat = (thatCount/*terminalCount*/ < creaseCount + ridgeCount - 4 - thatCount);
            }

            if (preserveThis || preserveThat)
            {
                /* To do: Need better evaluation here! */
                RWRETURNVOID();
            }

            if (!creased)
            {
                /* Tag */
                vert->value = -5.0f;
                refEdge->value = -5.0f;
                /* Make the updates */
                refEdge->next->prev = refEdge->prev;
                refEdge->prev->next = refEdge->next;

                if (face->edge == refEdge)
                {
                    face->edge = refEdge->next;
                }

                startVertex = refEdge->next->vertex;
                RtWingUpdateInternalRepresentation(face, refEdge->next, startVertex);
            }
            else if (creased)
            {
                RtWingEdge* edge2 = refEdge->neighbor;
                RwInt32 pair;

                RWASSERT(refEdge->neighbor->neighbor == refEdge);
                RWASSERT(refEdge->next->prev == refEdge);
                RWASSERT(refEdge->prev->next == refEdge);

                v1 = edge2->prev->prev->vertex->vertexInfo->position;
                v2 = edge2->prev->vertex->vertexInfo->position;
                v3 = edge2->vertex->vertexInfo->position;
                if (RtGCondColinearVertices(&v1, &v2, &v3, TRUE, gcParams->polyNormalsThreshold)) /* Z-code */
                {
                    /* Tag */
                    refEdge->vertex->value = -6.0f;
                    refEdge->value = -6.0f;
                    /* Make the updates */
                    refEdge->next->prev = refEdge->prev;
                    refEdge->prev->next = refEdge->next;

                    edge2->prev->vertex->value = -7.0f;
                    edge2->value = -7.0f;
                    edge2->next->prev = edge2->prev;
                    edge2->prev->next = edge2->next;
                    edge2->prev->vertex = edge2->vertex;

                    if (face->edge == refEdge)
                    {
                        face->edge = refEdge->next;
                    }

                    if (edge2->face->edge == edge2)
                    {
                        edge2->face->edge = edge2->next;
                    }

                    for (pair = 0; pair < 2; pair++)
                    {
                        if (pair == 0)
                        {
                            startVertex = refEdge->next->vertex;
                            RtWingUpdateInternalRepresentation(face, refEdge->next, startVertex);
                        }
                        else
                        {
                            startVertex = edge2->next->vertex;
                            RtWingUpdateInternalRepresentation(edge2->face, edge2->next, startVertex);
                        }
                    }
                }
            }
        }
    }
    RWRETURNVOID();
}

/****************************************************************************
 Functions (tools)
 */
#if(defined(WINGCHECK))
static RwBool
WingConvex(RtWingFace* face, RwBool linearSafe)
{
    RtWingEdge* edge;
    RwInt32 loopCtr;

    RWFUNCTION(RWSTRING("WingConvex"));

    if (face->value >= 0 && face->polygonInfo->numIndices >= 3)
    {
        edge = face->edge;
        loopCtr = 0;
        /* Check wing */
        do
        {
            loopCtr++;
            if (WingReflexTriVert(&edge->vertex->vertexInfo->position,
                &edge->next->vertex->vertexInfo->position,
                &edge->next->next->vertex->vertexInfo->position,
                &face->normal,
                linearSafe))
            {
                RWRETURN(FALSE);
            }
            edge = edge->next;
        }while(edge != face->edge);
    }
    RWRETURN(TRUE);
}
#endif /*(defined(WINGCHECK))*/

#if(defined(WINGCHECK))
static void
WingTopologyCheck(RtWings* wings)
{
    RtWingFace* face;
    RwInt32 i;

    RWFUNCTION(RWSTRING("WingTopologyCheck"));

    for (i = 0; i < wings->wingFacesSize; i++)
    {
        face = &(wings->wingFaces[i]);
        RWASSERT(WingConvex(face, TRUE));
    }
    RWRETURNVOID();
}
#endif /*(defined(WINGCHECK))*/

#if(defined(WINGCHECK))
static void
WingSanityCheck(RtWings* wings)
{
    RtWingEdge* edge;
    RtWingFace* face;
    RtWingVertex* vert;
    RwInt32 i;

    RWFUNCTION(RWSTRING("WingSanityCheck"));

    for (i = 0; i < wings->wingEdgesSize; i++)
    {
        edge = &(wings->wingEdges[i]);

        if (edge->value >= 0)
        {
            RWASSERT(1 || edge->vertex->value >= 0); /* This might fail on an edge with many faces when
            colinear have been removed from one of the face->edges only */
            RWASSERT(edge->face->value >= 0);
            RWASSERT(edge->next != NULL && edge->next->value >= 0);
            RWASSERT(edge->prev != NULL && edge->prev->value >= 0);
            RWASSERT(edge->next->prev == edge);
            RWASSERT(edge->prev->next == edge);
            RWASSERT(edge->vertex->edge->value >= 0);
            RWASSERT(edge->vertex->edge->next->prev == edge->vertex->edge);
            RWASSERT(edge->vertex->edge->prev->next == edge->vertex->edge);
            /* new */
            RWASSERT(edge->prev->neighbor == NULL || edge->prev->neighbor->prev->prev != edge->prev);
            /* RWASSERT(edge->vertex != edge->prev->vertex); */

            if (edge->neighborRelation != rtWINGEDGENULL)
            {
                RWASSERT(edge->neighbor);
                RWASSERT(edge->prev->value >= 0);
                if (edge->neighborRelation == rtWINGEDGECONTINUOUS)
                {
                    /* RWASSERT(edge->prev->vertex == edge->neighbor->vertex); */
                    RWASSERT(edge->neighbor->value >= 0);
                    /* RWASSERT(edge->neighbor->prev->vertex == edge->vertex); */
                }
                else
                {
                }
                /* RWASSERT(edge != edge->neighbor->next); */
            }
            /*
            if (edge->vertex->edge->neighborRelation != rtWINGEDGENULL)
            {
                RWASSERT(edge->vertex->edge->neighbor);
                RWASSERT(edge->vertex->edge != edge->vertex->edge->neighbor->next);
            }*/
        }
    }
    for (i = 0; i < wings->wingFacesSize; i++)
    {
        face = &(wings->wingFaces[i]);

        if (face->value >= 0)
        {
            /* RWASSERT(face->edge != NULL && face->edge->value >= 0); */
            /* RWASSERT(face->polygonInfo->numIndices >= 0); */
        }
    }
    for (i = 0; i < wings->wingVerticesSize; i++)
    {
        vert = &(wings->wingVertices[i]);

        if (vert->value >= 0)
        {
            /* RWASSERT(vert->edge != NULL && vert->edge->value >= 0); */
            /* RWASSERT(vert->edge->prev->vertex == vert); */
        }
    }
    RWRETURNVOID();
}
#endif /*(defined(WINGCHECK))*/

#if(defined(WINGCHECK))
static void
WingSanityReport(RtWings* wings)
{
    RtWingFace* face;
    RtWingEdge* edge;

    RwInt32 i;

    RWFUNCTION(RWSTRING("WingSanityReport"));

    printf("\nSANITY REPORT...\n");
    for (i = 0; i < wings->wingFacesSize; i++)
    {
        face = &(wings->wingFaces[i]);
        edge = face->edge;

        if (face->value >= 0)
        {
            printf("\nFace: %d\n", i);
            do
            {
                printf("%4d", edge->vertex->vertexInfo->index);
                edge = edge->next;
            }while(edge != face->edge);
        }
    }
    printf("\n");

    RWRETURNVOID();
}
#endif /*(defined(WINGCHECK))*/

static RwReal
WingAngleBetweenFaces(RtWingFace* a, RtWingFace* b)
{
    RwV3d n;

    RWFUNCTION(RWSTRING("WingAngleBetweenFaces"));

    RwV3dCrossProduct(&n, &a->normal, &b->normal);

    RWRETURN(RtGCondLength(&n));
}

static RwBool
WingReflexTriVert(RwV3d* a, RwV3d* b, RwV3d* c, RwV3d* normal, RwBool linearSafe)
{
    RwV3d               e1, e2, n, m;
    RtGCondParameters   *gcParams = RtGCondParametersGet();

    RWFUNCTION(RWSTRING("WingReflexTriVert"));

    if (RtGCondVectorsEqual(a, b, GCONDR1TOL) || RtGCondVectorsEqual(b, c, GCONDR1TOL) ||
        RtGCondVectorsEqual(c, a, GCONDR1TOL))
    {
        RWRETURN(TRUE); /* Z-code */
    }

    if(RtGCondColinearVertices(a, b, c, TRUE, gcParams->polyNormalsThreshold))
    {
        RWRETURN(!linearSafe); /* it's border-line...actually, it's 180 degrees */
    }

    e1.x = b->x - a->x;
    e1.y = b->y - a->y;
    e1.z = b->z - a->z;

    e2.x = c->x - b->x;
    e2.y = c->y - b->y;
    e2.z = c->z - b->z;

    RtGCondNormalize(&e1);
    RtGCondNormalize(&e2);
    RwV3dCrossProduct(&n, &e1, &e2);

    /* Check if it's still close to zero lenght vector. */
    if (!RtGCondNormalize(&n))
    {
        RWRETURN(FALSE);
    }

    RwV3dAdd(&m, &n, normal);
    if (RtGCondLength(&m) < 1.0f) /* length is either 0 or 2 */
    {
        RWRETURN(TRUE);
    }
    RWRETURN(FALSE);
}

static RwReal
WingRemainder(RwReal val)
{
    RwReal remain;

    RWFUNCTION(RWSTRING("WingRemainder"));

    if (val < 0.0f)
    {
        val += (RwReal)RwFabs(RwInt32FromRealMacro(val)) + 1.0f;
    }
    remain = (RwReal)(RwFabs( RwInt32FromRealMacro(val) - val ));
    if (remain < 0.0f) remain += 1.0f;              /* snap to +ve space */
    if (remain >= (1 - GCONDZERO)) remain = 0.0f;   /* snap to grid in effect */

    RWRETURN(remain);
}

static RwBool
WingEqualZero(RwReal val)
{
    RWFUNCTION(RWSTRING("WingEqualZero"));

    RWRETURN((val <= GCONDZERO) && (val >= -GCONDZERO));
}

static RwBool
WingEqualZeroTol(RwReal val, RwReal tol)
{
    RWFUNCTION(RWSTRING("WingEqualZeroTol"));

    RWRETURN((val <= tol) && (val >= -tol));
}

static RwBool
WingIsAnEar(RtWingFace* face __RWUNUSED__, RtWingVertex* a, RtWingVertex* b, RtWingVertex* c, RwV3d* normal)
{
    RWFUNCTION(RWSTRING("WingIsAnEar"));

    /* NB: face will be used in WingContainsNoVertex, when concave polygons are allowed in the future */

    if (WingReflexTriVert(&a->vertexInfo->position, &b->vertexInfo->position, &c->vertexInfo->position, normal, FALSE))
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}

#if(defined(WINGCHECK))
static RwBool
WingContainsNoVertex(RtWingVertex* a, RtWingVertex* b, RtWingVertex* c, RtWingFace *face)
{
    RtWingEdge* e = face->edge;

    RWFUNCTION(RWSTRING("WingContainsNoVertex"));

    do
    {
        RtWingVertex* v = e->vertex;
        if (!((a == v) || (b == v) || (c == v)))
        {
            if ((!WingReflexTriVert(&v->vertexInfo->position, &b->vertexInfo->position, &c->vertexInfo->position, &face->normal, TRUE)) &&
                (!WingReflexTriVert(&v->vertexInfo->position, &c->vertexInfo->position, &a->vertexInfo->position, &face->normal, TRUE)) &&
                (!WingReflexTriVert(&v->vertexInfo->position, &a->vertexInfo->position, &b->vertexInfo->position, &face->normal, TRUE)))
            {
                RWRETURN(FALSE);
            }
        }
        e = e->next;
    }while(e != face->edge);
    RWRETURN(TRUE);
}
#endif /*(defined(WINGCHECK))*/

