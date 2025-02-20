#include "rwcore.h"
#include "rpworld.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "toon.h"
#include "genericedgerender.h"

#include "crease.h"

/*
  Generic crease edge rendering
*/

#define RPTOON_COLINEAR_EDGE_THRESHOLD      -0.8f
#define RPTOON_SHARP_EDGE_THRESHOLD         0.95f
#define RPTOON_REALLY_SHARP_EDGE_THRESHOLD  0.999f


/*
  walk along side of f[0], around v[0] from firstEdge

  pivotVertexIndex = firstEdge.v[0]
  currentFaceIndex = firstEdge.face[0]
  edge = firstEdge;
  nextEdge = edge.edgefv[0][0];

  do
  {
  //do something with edge

  edge = nextEdge;

  vsub = (edge.v[0] == pivotVertexIndex) ? 0 : 1;       //want same vertex
  fsub = (edge.face[0] == currentFaceIndex) ? 1 : 0;    //want different face

  nextEdge = edge.edgefv[fsub][vsub];
  }
  while (edge != firstEdge && edge != noNextEdge)

*/

static RwInt32
FindNextCreaseOrSilhoutteEdge(RpToonGeo *toonGeo,
                              rpToonEdgeIndex firstEdge,
                              RwInt32 faceSub,
                              RwInt32 vertexSub )
{
    rpToonVertexIndex pivotVertexIndex;
    rpToonFaceIndex currentFaceIndex;
    rpToonEdgeIndex edge;

    RWFUNCTION(RWSTRING("FindNextCreaseOrSilhoutteEdge"));
    RWASSERT(vertexSub == 0 || vertexSub == 1);
    RWASSERT(faceSub == 0 || faceSub == 1);

    pivotVertexIndex = toonGeo->edges[firstEdge].v[vertexSub];
    currentFaceIndex = toonGeo->edges[firstEdge].face[faceSub];

    /* start by looking at first edge in desired direction */
    edge = toonGeo->edges[firstEdge].edgefv[faceSub][vertexSub];

    while (edge != noNextEdge && edge != firstEdge)
    {
        RwInt32 vsub;
        RwInt32 fsub;

        if ((edge < toonGeo->numCreaseEdges) )
        {
            RWRETURN(edge);
        }

        vsub = (toonGeo->edges[edge].v[0] == pivotVertexIndex) ? 0 : 1;       /* want same vertex */

        /* want different face */

        if (toonGeo->edges[edge].face[0] == currentFaceIndex)
        {
            fsub = 1;
        }
        else
        {
            fsub = 0;
        }

        currentFaceIndex = toonGeo->edges[edge].face[fsub];

        /* move on to next edge around this vertex */
        edge = toonGeo->edges[edge].edgefv[fsub][vsub];
    }

    RWRETURN(edge);
}


#define REMOVETHISWHENBACKFACEORDERISPRESERVED

static void
FindExtrusionOffsetForCreaseEdge(RwV2d *extrusionOffset,
                                 const RwV2d *normalizedPerpendicular,
                                 RwReal extrusionLength,
                                 RpToonGeo *toonGeo,
                                 rpToonEdgeIndex edge,
                                 RwInt32 faceSub,
                                 RwInt32 vertSub,
                                 const RwV2d *v0p,
                                 const RwV2d *v1p,
                                 const ScreenSpaceProjection *ssp,
                                 const RwV3d *verts,
                                 const RwReal degenerateEdgeLength )
{
    RwBool wentAlongCWSide;
    rpToonEdgeIndex nextEdge;

    RWFUNCTION(RWSTRING("FindExtrusionOffsetForCreaseEdge"));

    wentAlongCWSide = (faceSub ^ vertSub);

    nextEdge = FindNextCreaseOrSilhoutteEdge(toonGeo, edge, faceSub, vertSub );

    /*   RWASSERT( nextEdge != noNextEdge && "Impossible! Crease edges should always have another crease or silhoutte" ); */

    /* n.b. caller is responsible for checking this edge isn't a degenerate */

    if ((nextEdge == edge) || (nextEdge == noNextEdge))
    {
        /* this edge is a dead end - just extrude perpendicular to this segment */
        *extrusionOffset = *normalizedPerpendicular;

        if (!faceSub)
        {
            RwV2dScale(extrusionOffset, extrusionOffset, -1.0f);
        }
    }
    else
    {
        /* next edge is a crease/silhoutte edge - extrude along bisector of the two edges */

        /* get projected coordinate of vertex from next edge that isn't part of this one */
        RwInt32 v2;

        /*
          Need to work out two vectors pointing AWAY from the shared vertex between these edges
          to calculate bisector.  And they must share exactly one vertex or something's wrong!
          Z
          /
          /   <---- next edge
          Y----X

          ^
          |
          current edge
        */


        const RwV2d *X, *Y;
        RwV3d Z;
        RwV2d Zp;
        RwV2d dXY, dXZ, bisector;
        RwReal  dXZLength;
        RwReal  dXYLength;

        if (toonGeo->edges[nextEdge].v[0] == toonGeo->edges[edge].v[0])
        {
            v2 = toonGeo->edges[nextEdge].v[1];
            X = v0p;
            Y = v1p;
        }
        else if (toonGeo->edges[nextEdge].v[0] == toonGeo->edges[edge].v[1])
        {
            v2 = toonGeo->edges[nextEdge].v[1];
            X = v1p;
            Y = v0p;
        }
        else if (toonGeo->edges[nextEdge].v[1] == toonGeo->edges[edge].v[0])
        {
            v2 = toonGeo->edges[nextEdge].v[0];
            X = v0p;
            Y = v1p;
        }
        else
        {
            RWASSERT((toonGeo->edges[nextEdge].v[1] == toonGeo->edges[edge].v[1]) && "edges must share a vertex!");

            v2 = toonGeo->edges[nextEdge].v[0];
            X = v1p;
            Y = v0p;
        }

        ScreenSpaceProjectionProjectPoint( &Zp, &Z, ssp, &verts[v2] );

        dXZ.x = Zp.x - X->x;
        dXZ.y = Zp.y - X->y;

        dXZLength = RwV2dLength(&dXZ);

        dXY.x = Y->x - X->x;
        dXY.y = Y->y - X->y;

        dXYLength = RwV2dLength(&dXY);
        RwV2dScale( &dXY, &dXY, 1.0f / dXYLength);


        if (dXZLength < degenerateEdgeLength)
        {
            /*
              Next edge is degenerate, let's squish this endpoint down to 0 width
            */
            /*
              RwV2d zero = {0,0};
              *extrusionOffset = zero;
              extrusionLength = 0.0f;
            */
            /*
              Alternatively, could go perpendicular to this edge by a constant length
            */
            RwV2dPerp(extrusionOffset, &dXY);

            if (wentAlongCWSide)
            {
                RwV2dScale(extrusionOffset, extrusionOffset, -1.0f);
            }
        }
        else
        {
            /* next edge is not a degenerate */
            RwReal cosEdgeAngle;

            /*
              If edges are too close to 180 degrees apart,
              we need a special way of doing the bisector that won't get numerical troubles
            */

            RwV2dScale( &dXZ, &dXZ, 1.0f / dXZLength);

            cosEdgeAngle = RwV2dDotProduct(&dXY,&dXZ);

            if (cosEdgeAngle < RPTOON_COLINEAR_EDGE_THRESHOLD)
            {
#ifdef REMOVETHISWHENBACKFACEORDERISPRESERVED
                /*
                  Edges are nearly colinear - take the average of their perpendiculars for a bisector
                  to extrude along..
                */
                RwV2d dXYperp, dXZperp;

                RwV2dPerp(&dXYperp, &dXY);
                RwV2dPerp(&dXZperp, &dXZ);

                /* and flip them to the appropriate direction */
                if (wentAlongCWSide)
                {
                    dXZperp.x = -dXZperp.x;
                    dXZperp.y = -dXZperp.y;
                }
                else
                {
                    dXYperp.x = -dXYperp.x;
                    dXYperp.y = -dXYperp.y;
                }

                RwV2dAdd(extrusionOffset, &dXYperp, &dXZperp );
                RwV2dNormalize(extrusionOffset, extrusionOffset);
#else
                {
                    /*
                      RwV2d temp;
                      if (faceSub)
                      {
                      RwV2dSub(&temp, v1p, v0p);
                      }
                      else
                      {
                      RwV2dSub(&temp, v0p, v1p);
                      }
                      RwV2dNormalize(&temp, &temp);
                      RwV2dPerp(&dXYperp, &temp);
                      *extrusionOffset = dXYperp;
                      */
                    *extrusionOffset = *normalizedPerpendicular;

                    if (!faceSub)
                    {
                        extrusionOffset->x = -extrusionOffset->x;
                        extrusionOffset->y = -extrusionOffset->y;
                    }
                }
#endif
            }
            else
            {
                /*
                  Edges are not degenerates & not colinear, try and find a mitre between them
                  that gives a constant edge width.
                */

                /* magnitude of a cross b is sin of angle betweem them... */
                RwReal sinEdgeAngle = dXY.x * dXZ.y - dXY.y * dXZ.x;

                RwBool bisectorOnCWSide = sinEdgeAngle > 0.0f;

                RwReal mitreScaleFactor;

                if (bisectorOnCWSide == wentAlongCWSide)
                {
                    /*
                      its a silhoutte edge and it's less than 90 degrees wide and it's not very sharp
                      - extrude by right length along silhoutte segment
                    */
                    *extrusionOffset = dXZ;

                    mitreScaleFactor = RwRealAbs( 1.0f / sinEdgeAngle );
#ifdef REMOVETHISWHENBACKFACEORDERISPRESERVED
                    if (bisectorOnCWSide != wentAlongCWSide)
                    {
                        extrusionOffset->x = -extrusionOffset->x;
                        extrusionOffset->y = -extrusionOffset->y;
                    }
#endif
                    /*
                      it'd be mad to stretch out longer than the actual edge where trying to follow
                      so clamp it!
                    */
                    if (mitreScaleFactor * extrusionLength > dXZLength)
                    {
                        mitreScaleFactor = dXZLength / extrusionLength;
                    }
                }
                else
                {
                    RwReal cosTheta;

                    /*
                      It's a crease edge with a well conditioned bisector, extrude along that
                    */

                    RwV2dAdd(&bisector, &dXY, &dXZ );

                    /*
                      Bisector may be on clockwise or counter clockwise side depending which had the smaller angle
                      We need to match bisector to the direction we went to find the next edge by negating it
                      if necessary.

                      Use cross product test to determine direction,
                      if dXY x dXZ is out of page, bisector is on cw side, else ccw side
                    */
#ifdef REMOVETHISWHENBACKFACEORDERISPRESERVED
                    if (bisectorOnCWSide != wentAlongCWSide)
                    {
                        bisector.x = -bisector.x;
                        bisector.y = -bisector.y;
                    }
#endif
                    RwV2dNormalize(&bisector, &bisector);
                    *extrusionOffset = bisector;

                    cosTheta = RwV2dDotProduct(&bisector, normalizedPerpendicular);
                    mitreScaleFactor =  RwRealAbs( 1.0f / cosTheta );

                    if (cosEdgeAngle > RPTOON_SHARP_EDGE_THRESHOLD)
                    {
                        /* angle is very sharp, mitre correcting scale factor will be way too big */
                        mitreScaleFactor = 1.0f;
                        RwV2dNormalize( extrusionOffset, extrusionOffset );
                    }
                }

                /*
                  If there was a sharp edge, limit the scale factor to something sensible.
                  For outside edges, might want to introduce another line segment along the bisector,
                  or break the line going perpendicular rather than along the bisector.
                */
                if (cosEdgeAngle > RPTOON_SHARP_EDGE_THRESHOLD)
                {
                    /* reign in degnerately nasty sharp corners */
                    if (cosEdgeAngle > RPTOON_REALLY_SHARP_EDGE_THRESHOLD)
                    {
                        mitreScaleFactor = 0.0f;
                    }
                    /* scale back sharpish corners */
                    else
                    {
                        mitreScaleFactor *= (1.0f - cosEdgeAngle) / (1.0f - RPTOON_SHARP_EDGE_THRESHOLD);
                    }
                }

                /* scale corner to have constant edge width */
                extrusionLength *= mitreScaleFactor;

            } /* not colinear */
        } /* not a degenerate */
    }

    extrusionOffset->x *= extrusionLength;
    extrusionOffset->y *= extrusionLength;

    RWRETURNVOID();
}

/* crease line render functions - needs the generic line render code */
void
_rpToonCreaseRender(RpToonGeo *toonGeo,
                    const RwV3d *verts,
                    const RwMatrix *transform)
{
    RwInt32 numCreaseVerts = 0;
    RwIm3DVertex *verticesEnd = _rpToonGenericVertexBuffer;
    RwReal degenerateEdgeLength = RPTOON_REALLY_SMALL_EDGE_THRESHOLD;
    rpToonEdgeIndex i;
    ScreenSpaceProjection ssp;

    RWFUNCTION(RWSTRING("_rpToonCreaseRender"));

    _rpToonScreenSpaceProjectionInit( &ssp, transform );

    RWASSERT(toonGeo);

    RWASSERT(toonGeo->edgeInkIDsOptimized &&
             "Use RpToonGeoOptimizeEdgeInkIDs before rendering");

    for (i = 0; i<toonGeo->numCreaseEdges; i++)
    {
        RWASSERT(toonGeo->edges[i].face[0] != RPTOONEDGE_NONEIGHBOURFACE);
        /*
          if (!IsCurrentlySilhouetteEdge(i)
          #ifdef BRUTEFORCESILHOUETTES                                      //not a silhoutte - use silhoutte ink for those
          && eMinusVdotN[toonGeo->edges[i].face[0]]             //straddling two front facing polys (optimization)
          && ((toonGeo->edges[i].face[1] == RPTOONEDGE_NONEIGHBOURFACE)    //or an open edge
          || eMinusVdotN[toonGeo->edges[i].face[1]])
          #endif
          )
        */
        {
            RwReal zBias = RpToonGetCreaseZBias();
            RwReal extrusionLength;

            /* transform edge endpoints to screen space */
            RwV2d v0p, v1p;
            RwV3d v0, v1, v2, v3;
            RwV2d perpEdgeDelta;
            RwReal edgeLength;
            RwV2d edgeDelta;
            rpToonVertexIndex vertex0Index, vertex1Index;

            RpToonInk *ink = toonGeo->inks[toonGeo->edgeInkIDs[i].inkId[RPTOON_INKTYPE_CREASE]];
            RWASSERT(ink&&"no crease ink specified for crease edge");
            extrusionLength = ink->farScale_creasefactor;

            /* get projected x & y values for end points, z in camera space */
            vertex0Index = toonGeo->edges[i].v[0];
            vertex1Index = toonGeo->edges[i].v[1];
            ScreenSpaceProjectionProjectPoint( &v0p, &v0, &ssp, &verts[vertex0Index] );
            ScreenSpaceProjectionProjectPoint( &v1p, &v1, &ssp, &verts[vertex1Index] );
            RwV2dSub( &edgeDelta, &v1p, &v0p );

            edgeLength = RwV2dLength(&edgeDelta);

            /*
              To avoid nasty quads that blow up to infinite length,
              only draw edge if it's not projected to a degenerate,
              also be careful not to use a zbias that will give undefined results when backprojecting!
            */
            if (edgeLength > degenerateEdgeLength && v0.z > zBias && v1.z > zBias)
            {
                RwV2d extrudeOffset0, extrudeOffset1;
                RwReal ooEdgeLength;
                RwV2d normalizedPerpEdgeDelta;
                RwReal v0Thickness;
                RwReal v1Thickness;

                RwV2dPerp( &perpEdgeDelta,  &edgeDelta );

                ooEdgeLength = 1.0f / edgeLength;

                RwV2dScale( &normalizedPerpEdgeDelta, &perpEdgeDelta, ooEdgeLength );

                /* modulate length by vertex thickness */
                v0Thickness = extrusionLength
                    * toonGeo->vertexThicknesses[vertex0Index]
                    * _rpToonComputePerspectiveScale(ink, v0.z );

                v1Thickness = extrusionLength
                    * toonGeo->vertexThicknesses[vertex1Index]
                    * _rpToonComputePerspectiveScale(ink, v1.z );

                /* bias the z back a little bit to lift it off the poly it's sitting on */
                /* wonder if we can get away with D3DRS_POLYGONOFFSETZOFFSET or equivalent, if there is one */
                v0.z -= zBias;
                v1.z -= zBias;

                /* back project to get x & y values that will project to original position with the biased z */
                v0.x = v0p.x;
                v0.y = v0p.y;
                v1.x = v1p.x;
                v1.y = v1p.y;

                BackProject( &v0 );
                BackProject( &v1 );

                FindExtrusionOffsetForCreaseEdge(
                                                 &extrudeOffset0, &normalizedPerpEdgeDelta, v0Thickness,
                                                 toonGeo,
                                                 i, 0, 0,
                                                 &v0p, &v1p,
                                                 &ssp, verts,
                                                 degenerateEdgeLength );

                FindExtrusionOffsetForCreaseEdge(
                                                 &extrudeOffset1, &normalizedPerpEdgeDelta, v1Thickness,
                                                 toonGeo,
                                                 i, 0, 1,
                                                 &v0p, &v1p,
                                                 &ssp, verts,
                                                 degenerateEdgeLength );

                v2.x = v0p.x + extrudeOffset0.x;
                v2.y = v0p.y + extrudeOffset0.y;

                v3.x = v1p.x + extrudeOffset1.x;
                v3.y = v1p.y + extrudeOffset1.y;

                v2.z = v0.z;
                v3.z = v1.z;
                BackProject( &v2 );
                BackProject( &v3 );

                verticesEnd =
                    _rpToonGenericEdgeRenderAppendIm3dQuad(
                        verticesEnd, &v0, &v1, &v2, &v3, ink->color );

                FindExtrusionOffsetForCreaseEdge(
                                                 &extrudeOffset0, &normalizedPerpEdgeDelta, v0Thickness,
                                                 toonGeo,
                                                 i, 1, 0,
                                                 &v0p, &v1p,
                                                 &ssp, verts,
                                                 degenerateEdgeLength );

                FindExtrusionOffsetForCreaseEdge(
                                                 &extrudeOffset1, &normalizedPerpEdgeDelta, v1Thickness,
                                                 toonGeo,
                                                 i, 1, 1,
                                                 &v0p, &v1p,
                                                 &ssp, verts,
                                                 degenerateEdgeLength);

                v2.x = v0p.x + extrudeOffset0.x;
                v2.y = v0p.y + extrudeOffset0.y;

                v3.x = v1p.x + extrudeOffset1.x;
                v3.y = v1p.y + extrudeOffset1.y;

                v2.z = v0.z;
                v3.z = v1.z;
                BackProject( &v2 );
                BackProject( &v3 );

                /* swap vertex order this time to preserve backface culling */
                verticesEnd =
                    _rpToonGenericEdgeRenderAppendIm3dQuad(
                        verticesEnd, &v1, &v0, &v3, &v2, ink->color );
            }
        }
    }

    numCreaseVerts =
        ((RwInt32)verticesEnd - (RwInt32)_rpToonGenericVertexBuffer) / sizeof(RwIm3DVertex);

    _rpToonGenericEdgeRenderSubmit(numCreaseVerts, rwPRIMTYPETRILIST );

    RWRETURNVOID();
}
