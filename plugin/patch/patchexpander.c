/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rppatch.h"
#include "rtbezpat.h"

#include "patch.h"
#include "patchmesh.h"

#include "patchexpander.h"

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

/****************************************************************************
 PatchRefineTriVector

 Generate the trig refinememt.

 Inputs:
 Outputs:
 */
RwV3d *
_rpPatchRefineTriVector( RwV3d *vert,
                         RwUInt32 stride,
                         RtBezierMatrix cptPos,
                         RwInt32 res )
{
    RtBezierMatrix quad;
    RtBezierMatrix weight;
    RtBezierMatrix diffPos;

    RwV3d rowPos[4];

    RwInt32 i, j;
    RwReal recipRes;

    RwV3d *pt;

    RWFUNCTION(RWSTRING("_rpPatchRefineTriVector"));

    recipRes = ((RwReal) 1) / ((RwReal) res);

    RtBezierQuadFromTriangle(quad, cptPos);
    RtBezierQuadBernsteinWeight3d(weight, quad);
    RtBezierQuadOriginDifference3d(diffPos, weight, recipRes, recipRes);

    pt = vert;

    j = res + 1;
    do
    {
        /* Pos */
        rowPos[0].x = diffPos[0][0].x;
        rowPos[0].y = diffPos[0][0].y;
        rowPos[0].z = diffPos[0][0].z;

        rowPos[1].x = diffPos[0][1].x;
        rowPos[1].y = diffPos[0][1].y;
        rowPos[1].z = diffPos[0][1].z;

        rowPos[2].x = diffPos[0][2].x;
        rowPos[2].y = diffPos[0][2].y;
        rowPos[2].z = diffPos[0][2].z;

        rowPos[3].x = diffPos[0][3].x;
        rowPos[3].y = diffPos[0][3].y;
        rowPos[3].z = diffPos[0][3].z;

        i = j;
        do
        {
            /* Write the point into the buffer. */
            pt->x = rowPos[0].x;
            pt->y = rowPos[0].y;
            pt->z = rowPos[0].z;

            pt = (RwV3d *) (((RwUInt8 *) pt) + stride);

            --i;
            if (i)
            {
                RtBezierQuadDifferenceStepU3dMacro(rowPos);
            }
        }
        while(i);

        --j;
        if (j)
        {
            RtBezierQuadDifferenceStepV3dMacro(diffPos);
        }
    }
    while(j);

    RWRETURN(vert);
}

/****************************************************************************
 _rpPatchRefineTriColor

 Generate the trig refinememt.

 Inputs:
 Outputs:
 */
RwRGBA *
_rpPatchRefineTriColor( RwRGBA *color,
                        RwUInt32 stride,
                        RtBezierMatrix cptCol,
                        RwInt32 res )
{

    RtBezierMatrix quad;
    RtBezierMatrix weight;
    RtBezierMatrix diffCol;

    RtBezierRow rowCol;

    RwInt32 i, j;
    RwReal recipRes;

    RwRGBA *pt;

    RWFUNCTION(RWSTRING("_rpPatchRefineTriColor"));

    recipRes = ((RwReal) 1) / ((RwReal) res);

    RtBezierQuadFromTriangle(quad, cptCol);
    RtBezierQuadBernsteinWeight4d(weight, quad);
    RtBezierQuadOriginDifference4d(diffCol, weight, recipRes, recipRes);

    pt = color;

    for( j = 0; j <= res; j++ )
    {
        rowCol[0].x = diffCol[0][0].x;
        rowCol[0].y = diffCol[0][0].y;
        rowCol[0].z = diffCol[0][0].z;
        rowCol[0].w = diffCol[0][0].w;

        rowCol[1].x = diffCol[0][1].x;
        rowCol[1].y = diffCol[0][1].y;
        rowCol[1].z = diffCol[0][1].z;
        rowCol[1].w = diffCol[0][1].w;

        rowCol[2].x = diffCol[0][2].x;
        rowCol[2].y = diffCol[0][2].y;
        rowCol[2].z = diffCol[0][2].z;
        rowCol[2].w = diffCol[0][2].w;

        rowCol[3].x = diffCol[0][3].x;
        rowCol[3].y = diffCol[0][3].y;
        rowCol[3].z = diffCol[0][3].z;
        rowCol[3].w = diffCol[0][3].w;

        for( i = 0; i <= (res - j); i++ )
        {
            pt->red = (RwUInt8) RwInt32FromRealMacro(rowCol[0].x);
            pt->green = (RwUInt8) RwInt32FromRealMacro(rowCol[0].y);
            pt->blue = (RwUInt8) RwInt32FromRealMacro(rowCol[0].z);
            pt->alpha = (RwUInt8) RwInt32FromRealMacro(rowCol[0].w);

            RtBezierQuadDifferenceStepU4dMacro(rowCol);

            pt = (RwRGBA *)(((RwUInt8 *)pt) + stride);
        }

        RtBezierQuadDifferenceStepV4dMacro(diffCol);
    }

    RWRETURN(color);
}

/****************************************************************************
 _rpPatchRefineTriTexCoords

 Generate the trig refinememt.

 Inputs:
 Outputs:
 */
RwTexCoords *
_rpPatchRefineTriTexCoords( RwTexCoords *texCoords,
                            RwUInt32 stride,
                            const RwTexCoords *cptTexCoords,
                            RwInt32 res )
{
    RwInt32 i, j, l2;
    RwReal u0, u1, u2, v0, v1, v2, t0, t1, t2;
    RwReal recipRes;

    RwTexCoords *pt;

    RWFUNCTION(RWSTRING("_rpPatchRefineTriTexCoords"));

    recipRes = ((RwReal) 1) / ((RwReal) res);

    u0 = cptTexCoords[0].u;
    v0 = cptTexCoords[0].v;

    u1 = cptTexCoords[1].u;
    v1 = cptTexCoords[1].v;

    u2 = cptTexCoords[2].u;
    v2 = cptTexCoords[2].v;

    pt = texCoords;

    for( j = 0; j <= res; j++ )
    {
        t0 = recipRes * (RwReal)j;

        for( i = 0; i <= (res - j); i++ )
        {
            /* compute barycentric coords */
            l2 = res - j - i;

            t1 = recipRes * (RwReal)l2;
            t2 = (RwReal)1.0 - (t0 + t1);

            /* Write the point into the buffer. */

            pt->u = t1 * u0 + t0 * u1 + t2 * u2;
            pt->v = t1 * v0 + t0 * v1 + t2 * v2;

            pt = (RwTexCoords *)(((RwUInt8 *)pt) + stride);
        }
    }

    RWRETURN(texCoords);
}

/****************************************************************************
 _rpPatchRefineQuadVector

 Generate the quad refinememt.

 Inputs:
 Outputs:
 */
RwV3d *
_rpPatchRefineQuadVector( RwV3d * vert,
                          RwUInt32 stride,
                          RtBezierMatrix cptVec,
                          RwInt32 res )
{
    RtBezierMatrix weight;
    RtBezierMatrix diffVec;

    RwV3d rowVec[4];

    RwInt32 i, j;

    RwReal recipRes;

    RwV3d *pt;

    RWFUNCTION(RWSTRING("_rpPatchRefineQuadVector"));

    recipRes = (RwReal) 1.0 / res;

    RtBezierQuadBernsteinWeight3d(weight, cptVec);
    RtBezierQuadOriginDifference3d(diffVec, weight, recipRes, recipRes);

    pt = vert;

    j = res + 1;
    do
    {
        /* Pos */
        rowVec[0].x = diffVec[0][0].x;
        rowVec[0].y = diffVec[0][0].y;
        rowVec[0].z = diffVec[0][0].z;

        rowVec[1].x = diffVec[0][1].x;
        rowVec[1].y = diffVec[0][1].y;
        rowVec[1].z = diffVec[0][1].z;

        rowVec[2].x = diffVec[0][2].x;
        rowVec[2].y = diffVec[0][2].y;
        rowVec[2].z = diffVec[0][2].z;

        rowVec[3].x = diffVec[0][3].x;
        rowVec[3].y = diffVec[0][3].y;
        rowVec[3].z = diffVec[0][3].z;

        i = res + 1;
        do
        {
            /* Write the point into the buffer. */
            pt->x = rowVec[0].x;
            pt->y = rowVec[0].y;
            pt->z = rowVec[0].z;

            pt = (RwV3d *)(((RwUInt8 *)pt) + stride);

            --i;
            if(i)
            {
                RtBezierQuadDifferenceStepU3dMacro(rowVec);
            }
        }
        while(i);

        --j;
        if(j)
        {
            RtBezierQuadDifferenceStepV3dMacro(diffVec);
        }
    }
    while(j);

    RWRETURN(vert);
}

/****************************************************************************
 _rpPatchRefineQuadColor

 Generate the quad refinememt.

 Inputs:
 Outputs:
 */
RwRGBA *
_rpPatchRefineQuadColor( RwRGBA *color,
                         RwUInt32 stride,
                         RtBezierMatrix cptCol,
                         RwInt32 res )
{
    RtBezierMatrix weight;
    RtBezierMatrix diffCol;

    RtBezierRow rowCol;

    RwInt32 i, j;

    RwReal recipRes;

    RwRGBA *pt;

    RWFUNCTION(RWSTRING("_rpPatchRefineQuadColor"));

    recipRes = (RwReal)1.0 / res;

    RtBezierQuadBernsteinWeight4d(weight, cptCol);
    RtBezierQuadOriginDifference4d(diffCol, weight, recipRes, recipRes);

    pt = color;
    for( j = 0; j <= res; j++ )
    {
        rowCol[0].x = diffCol[0][0].x;
        rowCol[0].y = diffCol[0][0].y;
        rowCol[0].z = diffCol[0][0].z;
        rowCol[0].w = diffCol[0][0].w;

        rowCol[1].x = diffCol[0][1].x;
        rowCol[1].y = diffCol[0][1].y;
        rowCol[1].z = diffCol[0][1].z;
        rowCol[1].w = diffCol[0][1].w;

        rowCol[2].x = diffCol[0][2].x;
        rowCol[2].y = diffCol[0][2].y;
        rowCol[2].z = diffCol[0][2].z;
        rowCol[2].w = diffCol[0][2].w;

        rowCol[3].x = diffCol[0][3].x;
        rowCol[3].y = diffCol[0][3].y;
        rowCol[3].z = diffCol[0][3].z;
        rowCol[3].w = diffCol[0][3].w;

        for( i = 0; i <= res; i++ )
        {
            pt->red = (RwUInt8)RwInt32FromRealMacro(rowCol[0].x);
            pt->green = (RwUInt8)RwInt32FromRealMacro(rowCol[0].y);
            pt->blue = (RwUInt8)RwInt32FromRealMacro(rowCol[0].z);
            pt->alpha = (RwUInt8)RwInt32FromRealMacro(rowCol[0].w);

            RtBezierQuadDifferenceStepU4dMacro(rowCol);

            pt = (RwRGBA *)(((RwUInt8 *)pt) + stride);
        }

        RtBezierQuadDifferenceStepV4dMacro(diffCol);
    }

    RWRETURN(color);
}

/****************************************************************************
 _rpPatchRefineQuadTexCoords

 Generate the quad refinememt.

 Inputs:
 Outputs:
 */
RwTexCoords *
_rpPatchRefineQuadTexCoords( RwTexCoords *texCoords,
                             RwUInt32 stride,
                             const RwTexCoords *cptTexCoords,
                             RwInt32 res )
{
    RwInt32 i, j;

    RwReal du0;
    RwReal du1;
    RwReal du;
    RwReal dv0;
    RwReal dv1;
    RwReal dv;
    RwReal recipRes;
    RwReal u0;
    RwReal u1;
    RwReal v0;
    RwReal v1;

    RwTexCoords *pt;

    RWFUNCTION(RWSTRING("_rpPatchRefineQuadTexCoords"));

    recipRes = (RwReal)1.0 / res;

    dv0 = (cptTexCoords[2].v - cptTexCoords[0].v) * recipRes;
    dv1 = (cptTexCoords[3].v - cptTexCoords[1].v) * recipRes;
    du0 = (cptTexCoords[2].u - cptTexCoords[0].u) * recipRes;
    du1 = (cptTexCoords[3].u - cptTexCoords[1].u) * recipRes;
    pt = texCoords;
    for( j = 0; j <= res; j++ )
    {
        /* Interpolate the left edge. */
        u0 = cptTexCoords[0].u + ((RwReal)j * du0);
        v0 = cptTexCoords[0].v + ((RwReal)j * dv0);

        /* Interpolate the right edge. */
        u1 = cptTexCoords[1].u + ((RwReal)j * du1);
        v1 = cptTexCoords[1].v + ((RwReal)j * dv1);
        dv = (v1 - v0) * recipRes;
        du = (u1 - u0) * recipRes;

        for( i = 0; i <= res; i++ )
        {
            pt->u = (RwReal)u0 + ((RwReal)i * du);
            pt->v = (RwReal)v0 + ((RwReal)i * dv);

            pt = (RwTexCoords *)(((RwUInt8 *)pt) + stride);
        }
    }

    RWRETURN(texCoords);
}

/****************************************************************************
 _rpPatchGenerateTriIndex

 Generate the trig refinememt index.

 Inputs:
 Outputs:
 */
RxVertexIndex *
_rpPatchGenerateTriIndex(RxVertexIndex * indices,
                         RwInt32 base, RwInt32 res)
{
    RwInt32             odd, step, idx_buf_stride, i, j;
    RxVertexIndex      *idx, *idx_buf;

    RWFUNCTION(RWSTRING("_rpPatchGenerateTriIndex"));

    /* RWCRTCHECKMEMORY(); */

    /* This is a bit messy, but we need to generate vertex index depending
     * suitable for a long strip.
     *
     * So this mean we will be writing the buffer alternating left to right
     * and right to left for each scan line.
     *
     * Assuming a 4 x 4 refinement we would have the following indices
     *
     *   0   1   2   3
     *   4   5   6
     *   7   8
     *   9
     *
     * This will give a strip index of
     *
     *   0   4   1   5   2   6   3
     *   6   8   5   7   4
     *   7   9   8
     *   [8]
     *
     * For even lines, write left to right
     * For odd lines, write right to left.
     */
    idx = indices;
    idx_buf = indices;

    odd = 1;
    step = 1;

    for (j = 0; j < res; j++)
    {
        for (i = 0; i < (res - j); i++)
        {
            /* RWCRTCHECKMEMORY(); */
            *idx = (RxVertexIndex) base;
            /* RWCRTCHECKMEMORY(); */
            idx += step;

            /* RWCRTCHECKMEMORY(); */
            *idx = (RxVertexIndex) (base + res + 1 - j);
            /* RWCRTCHECKMEMORY(); */
            idx += step;

            base++;
        }

        /* RWCRTCHECKMEMORY(); */
        *idx = (RxVertexIndex) base;
        /* RWCRTCHECKMEMORY(); */
        idx += step;
        base++;

        idx_buf_stride = (res - j) * 2 + 1;

        idx_buf += idx_buf_stride;
        idx = idx_buf + (odd * (idx_buf_stride - 2 - step));

        odd = !odd;
        step *= -1;
    }

    /* If odd res, then duplicate the last index. This preserve the winding order for the
     * start of the next patch.
     */
    if (res & 0x01)
        idx[0] = idx[-1];

    /* RWCRTCHECKMEMORY(); */

    RWRETURN(indices);
}

/****************************************************************************
 _rpPatchGenerateQuadIndex

 Generate the quad refinememt index.

 Inputs:
 Outputs:
 */
RxVertexIndex *
_rpPatchGenerateQuadIndex(RxVertexIndex * indices,
                          RwInt32 base, RwInt32 res)
{
    RwInt32             odd, step, idx_buf_stride, i, j;
    RxVertexIndex      *idx, *idx_buf;

    RWFUNCTION(RWSTRING("_rpPatchGenerateQuadIndex"));

    /* RWCRTCHECKMEMORY(); */

    /* This is a bit messy, but we need to generate vertex index depending
     * suitable for a long strip.
     *
     * So this mean we will be writing the buffer alternating left to right
     * and right to left for each scan line.
     *
     * Assuming a 4 x 4 refinement we would have the following indices
     *
     *   0   1   2   3
     *   4   5   6   7
     *   8   9  10  11
     *  12  13  14  15
     *
     * This will give a strip index of
     *
     *   0   4   1   5   2   6   3   7
     *  11   7   6  10   5   9   4   8
     *   8  12   9  13  10  14  11  15
     *
     * For even lines, write left to right
     * For odd lines, write right to left.
     */
    idx = indices;
    idx_buf = indices;
    idx_buf_stride = (res + 1) * 2;
    odd = 1;
    step = 1;
    for (j = 0; j < res; j++)
    {
        for (i = 0; i <= res; i++)
        {
            /* RWCRTCHECKMEMORY(); */
            *idx = (RxVertexIndex) base;
            /* RWCRTCHECKMEMORY(); */
            idx += step;
            /* RWCRTCHECKMEMORY(); */
            *idx = (RxVertexIndex) (base + res + 1);
            /* RWCRTCHECKMEMORY(); */
            idx += step;
            base++;
        }

        idx_buf += idx_buf_stride;
        idx = idx_buf + (odd * (idx_buf_stride - step));
        odd = !odd;
        step *= -1;
    }

    /* RWCRTCHECKMEMORY(); */

    RWRETURN(indices);
}

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

/****************************************************************************
 _rpPatchInstanceAtomic

 Generate the atomic refinememt.

 Inputs:
 Outputs:
 */
RwBool
_rpPatchInstanceAtomic( RpPatchInstanceAtomic *instAtomic,
                        RpGeometry *geom,
                        RwInt32 res )
{
    RpMeshHeader  *meshHeader;
    RpMesh              *mesh;

    RpPatchInstanceMesh *instMesh;

    RtBezierMatrix cptPos;
    RtBezierMatrix cptNrm;
    RtBezierMatrix cptPreLit;

    RwInt32 base;
    RwInt32 firstpatch;
    RwInt32 i;
    RwInt32 j;
    RwInt32 k;
    RwInt32 l;
    RwInt32 minVert;
    RwInt32 numCtrlIndices;
    RwInt32 numMesh;
    RwInt32 numQuadIndices;
    RwInt32 numQuadVerts;
    RwInt32 numTriIndices;
    RwInt32 numTriVerts;
    RwInt32 numTextureCoords;
    RwInt32 numTextureCoordsPatch;

    RwTexCoords cptTexCoords[4];
    const RwV3d *vert;
    const RwV3d *nrm;
    const RwRGBA *preLit;

    RpPatchMeshFlag meshFlags;
    RwUInt32 stride;
    RwUInt8 *pt;
    RxVertexIndex *cptIdx;
    RxVertexIndex *idx;

    RWFUNCTION(RWSTRING("_rpPatchInstanceAtomic"));

    vert = geom->morphTarget[0].verts;
    nrm = geom->morphTarget[0].normals;
    preLit = geom->preLitLum;

    meshHeader = geom->mesh;
    mesh = (RpMesh *)((RwUInt8 *)(meshHeader + 1) +
                      meshHeader->firstMeshOffset);
    numMesh = meshHeader->numMeshes;

    meshFlags = instAtomic->meshFlags;
    stride = instAtomic->stride;
    pt = (RwUInt8 *)instAtomic->vertices;
    idx = instAtomic->indices;
    instMesh = instAtomic->meshes;
    numTextureCoordsPatch = _rpPatchMeshFlagGetNumTexCoords(meshFlags);

    /* Calculate patch result info */
    numQuadIndices = PATCHQUADNUMINDEX((res + 1));
    numQuadVerts = PATCHQUADNUMVERT((res + 1));
    numTriIndices = PATCHTRINUMINDEX((res + 1));
    numTriVerts = PATCHTRINUMVERT((res + 1));
    minVert = 0;

    numTextureCoords = RpGeometryGetNumTexCoordSets(geom);

    /*
     * We write an index for tri patches for odd number of patches.
     * This is to preserve the winding order.
     */
    if(res & 0x01)
        numTriIndices++;

    while(numMesh)
    {
        firstpatch = 1;
        base = 0;
        instMesh->indices = idx;
        instMesh->material = mesh->material;
        instMesh->mesh = mesh;
        instMesh->numIndices = 0;
        instMesh->minVert = minVert;
        instMesh->numVerts = 0;
        numCtrlIndices = mesh->numIndices;
        cptIdx = mesh->indices;
        /* Extract the quad control points from the mesh index. */
        while (numCtrlIndices > 0)
        {
            RwUInt8 *vertex = pt;

            /* Duplicate the last index of the previous and the first
             * index of the next patch. This connects the two together
             * into a single tristrip.
             */
            if(!firstpatch)
            {
                idx[0] = idx[-1];
                idx[1] = (RxVertexIndex) base;
                idx += 2;
                instMesh->numIndices += 2;
            }

            /* We use the fifth texCoords to flag if its a quad or tri
             * patch
             */
            if( numTextureCoords &&
                geom->texCoords[0][cptIdx[5]].u != (RwReal)0.0 )
            {
                /* Tri patch. */

                /* Set up the control point array. */
                l = 0;
                for( j = 0; j < 4; j++ )
                {
                    for( i = 0; i < (4 - j); i++ )
                    {
                        k = cptIdx[l];
                        cptPos[i][j].x = vert[k].x;
                        cptPos[i][j].y = vert[k].y;
                        cptPos[i][j].z = vert[k].z;
                        l++;
                    }

                    for( ; i < 4; i++ )
                    {
                        cptPos[i][j].x = (RwReal) 0.0;
                        cptPos[i][j].y = (RwReal) 0.0;
                        cptPos[i][j].z = (RwReal) 0.0;
                    }
                }

                _rpPatchRefineTriVector( (RwV3d *)vertex,
                                         stride,
                                         cptPos,
                                         res);

                vertex += sizeof(RwV3d);

                /* Set up the normals */
                if( meshFlags & rpPATCHMESHNORMALS )
                {
                    if( nrm != NULL )
                    {
                        l = 0;
                        for( j = 0; j < 4; j++ )
                        {
                            for( i = 0; i < (4 - j); i++ )
                            {
                                k = cptIdx[l];
                                cptNrm[i][j].x = nrm[k].x;
                                cptNrm[i][j].y = nrm[k].y;
                                cptNrm[i][j].z = nrm[k].z;
                                l++;
                            }

                            for( ; i < 4; i++ )
                            {
                                cptNrm[i][j].x = (RwReal) 0.0;
                                cptNrm[i][j].y = (RwReal) 0.0;
                                cptNrm[i][j].z = (RwReal) 0.0;
                            }
                        }

                        _rpPatchRefineTriVector( (RwV3d *)vertex,
                                                 stride,
                                                 cptNrm,
                                                 res );
                    }

                    vertex += sizeof(RwV3d);
                }

                /* Set up the prelit. */
                if(meshFlags & rpPATCHMESHPRELIGHTS)
                {
                    if(preLit != NULL)
                    {
                        l = 0;
                        for( j = 0; j < 4; j++ )
                        {
                            for( i = 0; i < (4 - j); i++ )
                            {
                                k = cptIdx[l];
                                cptPreLit[i][j].x = preLit[k].red;
                                cptPreLit[i][j].y = preLit[k].green;
                                cptPreLit[i][j].z = preLit[k].blue;
                                cptPreLit[i][j].w = preLit[k].alpha;
                                l++;
                            }

                            for( ; i < 4; i++ )
                            {
                                cptPreLit[i][j].x = (RwReal) 0.0;
                                cptPreLit[i][j].y = (RwReal) 0.0;
                                cptPreLit[i][j].z = (RwReal) 0.0;
                                cptPreLit[i][j].w = (RwReal) 0.0;
                            }
                        }

                        _rpPatchRefineTriColor( (RwRGBA *)vertex,
                                                stride,
                                                cptPreLit,
                                                res );
                    }

                    vertex += sizeof(RwRGBA);
                }

                /* Set up the texccords. */
                for( i = 0;
                     i < numTextureCoords && i < numTextureCoordsPatch;
                     i++ )
                {
                    const RwTexCoords *texCoords = geom->texCoords[i];

                    cptTexCoords[0] = texCoords[cptIdx[0]];
                    cptTexCoords[1] = texCoords[cptIdx[9]];
                    cptTexCoords[2] = texCoords[cptIdx[3]];

                    _rpPatchRefineTriTexCoords( (RwTexCoords *)vertex,
                                                stride,
                                                cptTexCoords,
                                                res );

                    vertex += sizeof(RwTexCoords);
                }

                _rpPatchGenerateTriIndex(idx, base, res);

                base += numTriVerts;
                pt += numTriVerts * stride;
                idx += numTriIndices;
                instMesh->numVerts += numTriVerts;
                instMesh->numIndices += numTriIndices;
                minVert += numTriVerts;
                firstpatch = 0;
            }
            else
            {
                /* Quad patch. */

                /* Set up the control point array. */
                for( i = 0; i < 4; i++ )
                {
                    for( j = 0; j < 4; j++ )
                    {
                        k = cptIdx[(j << 2) + i];
                        cptPos[i][j].x = vert[k].x;
                        cptPos[i][j].y = vert[k].y;
                        cptPos[i][j].z = vert[k].z;
                    }
                }

                _rpPatchRefineQuadVector( (RwV3d *)vertex,
                                          stride,
                                          cptPos,
                                          res );

                vertex += sizeof(RwV3d);

                /* Set up normals */
                if(meshFlags & rpPATCHMESHNORMALS)
                {
                    if(nrm != NULL)
                    {
                        for( i = 0; i < 4; i++ )
                        {
                            for( j = 0; j < 4; j++ )
                            {
                                k = cptIdx[(j << 2) + i];
                                cptNrm[i][j].x = nrm[k].x;
                                cptNrm[i][j].y = nrm[k].y;
                                cptNrm[i][j].z = nrm[k].z;
                            }
                        }

                        _rpPatchRefineQuadVector( (RwV3d *)vertex,
                                                  stride,
                                                  cptNrm,
                                                  res );
                    }

                    vertex += sizeof(RwV3d);
                }

                /* Set up the prelit. */
                if(meshFlags & rpPATCHMESHPRELIGHTS)
                {
                    if(preLit != NULL)
                    {
                        for( i = 0; i < 4; i++ )
                        {
                            for( j = 0; j < 4; j++ )
                            {
                                k = cptIdx[(j << 2) + i];
                                cptPreLit[i][j].x = preLit[k].red;
                                cptPreLit[i][j].y = preLit[k].green;
                                cptPreLit[i][j].z = preLit[k].blue;
                                cptPreLit[i][j].w = preLit[k].alpha;
                            }
                        }

                        _rpPatchRefineQuadColor( (RwRGBA *)vertex,
                                                 stride,
                                                 cptPreLit,
                                                 res );
                    }

                    vertex += sizeof(RwRGBA);
                }

                /* Set up the texccords. */
                for( i = 0;
                     i < numTextureCoords && i < numTextureCoordsPatch;
                     i++ )
                {
                    const RwTexCoords *texCoords = geom->texCoords[i];

                    cptTexCoords[0] = texCoords[cptIdx[0]];
                    cptTexCoords[1] = texCoords[cptIdx[3]];
                    cptTexCoords[2] = texCoords[cptIdx[12]];
                    cptTexCoords[3] = texCoords[cptIdx[15]];

                    _rpPatchRefineQuadTexCoords( (RwTexCoords *)vertex,
                                                 stride,
                                                 cptTexCoords,
                                                 res );

                    vertex += sizeof(RwTexCoords);
                }

                _rpPatchGenerateQuadIndex(idx, base, res);

                base += numQuadVerts;
                pt += numQuadVerts * stride;
                idx += numQuadIndices;
                instMesh->numVerts += numQuadVerts;
                instMesh->numIndices += numQuadIndices;
                minVert += numQuadVerts;
                firstpatch = 0;
            }

            numCtrlIndices -= rpPATCHNUMCONTROLINDICES;
            cptIdx += rpPATCHNUMCONTROLINDICES;
        }

        instMesh++;
        mesh++;
        numMesh--;
    }

#if defined(RWDEBUG)
    if (sizeof(RxVertexIndex) < 4 &&
        minVert >= (1 << (8 * sizeof(RxVertexIndex))))
    {
        RwDebugSendMessage(rwDEBUGMESSAGE, "Patch plugin",
                           "The number of vertices generated in this LOD for this atomic is too high.");
    }
#endif

    RWRETURN(TRUE);
}

/****************************************************************************
 _rpPatchUpdatePositions

 Used when only the vertex position and normals have been changed.

 I.e. patch skin

 Inputs:
 Outputs:
 */
RwBool
_rpPatchReInstanceAtomic( RpPatchInstanceAtomic *instAtomic,
                        const RpGeometry *geom,
                        RwInt32 res )
{
    const RwV3d *vert;
    const RwV3d *nrm;

    const RpMesh        *mesh;
    const RpMeshHeader  *meshHeader;

    RwInt32         numMesh;
    RpPatchMeshFlag meshFlags;
    RwUInt32        stride;
    RwUInt8         *pt;

    RwInt32 numQuadVerts;
    RwInt32 numTriVerts;
    RwInt32 numTextureCoords;

    RtBezierMatrix cptPos;
    RtBezierMatrix cptNrm;

    const RxVertexIndex *cptIdx;
    RwInt32             numCtrlIndices;

    RwInt32 i;
    RwInt32 j;
    RwInt32 k;
    RwInt32 l;

    RWFUNCTION(RWSTRING("_rpPatchReInstanceAtomic"));

    vert = geom->morphTarget[0].verts;
    nrm = geom->morphTarget[0].normals;

    meshHeader = geom->mesh;
    mesh = (const RpMesh *)((const RwUInt8 *)(meshHeader + 1) +
                      meshHeader->firstMeshOffset);
    numMesh = meshHeader->numMeshes;

    meshFlags = instAtomic->meshFlags;
    stride = instAtomic->stride;
    pt = (RwUInt8 *)instAtomic->vertices;

    /* Calculate patch result info */
    numQuadVerts = PATCHQUADNUMVERT((res + 1));
    numTriVerts = PATCHTRINUMVERT((res + 1));

    numTextureCoords = RpGeometryGetNumTexCoordSets(geom);

    while(numMesh)
    {
        numCtrlIndices = mesh->numIndices;
        cptIdx = mesh->indices;

        /* Extract the quad control points from the mesh index. */
        while (numCtrlIndices > 0)
        {
            RwUInt8 *vertex = pt;

            /* We use the fifth texCoords to flag if its a quad or tri
             * patch
             */
            if( numTextureCoords &&
                geom->texCoords[0][cptIdx[5]].u != (RwReal)0.0 )
            {
                /* Tri patch. */

                /* Set up the control point array. */
                l = 0;
                for( j = 0; j < 4; j++ )
                {
                    for( i = 0; i < (4 - j); i++ )
                    {
                        k = cptIdx[l];
                        cptPos[i][j].x = vert[k].x;
                        cptPos[i][j].y = vert[k].y;
                        cptPos[i][j].z = vert[k].z;
                        l++;
                    }

                    for( ; i < 4; i++ )
                    {
                        cptPos[i][j].x = (RwReal) 0.0;
                        cptPos[i][j].y = (RwReal) 0.0;
                        cptPos[i][j].z = (RwReal) 0.0;
                    }
                }

                _rpPatchRefineTriVector( (RwV3d *)vertex,
                                         stride,
                                         cptPos,
                                         res);

                vertex += sizeof(RwV3d);

                /* Set up the normals */
                if( meshFlags & rpPATCHMESHNORMALS )
                {
                    if( nrm != NULL )
                    {
                        l = 0;
                        for( j = 0; j < 4; j++ )
                        {
                            for( i = 0; i < (4 - j); i++ )
                            {
                                k = cptIdx[l];
                                cptNrm[i][j].x = nrm[k].x;
                                cptNrm[i][j].y = nrm[k].y;
                                cptNrm[i][j].z = nrm[k].z;
                                l++;
                            }

                            for( ; i < 4; i++ )
                            {
                                cptNrm[i][j].x = (RwReal) 0.0;
                                cptNrm[i][j].y = (RwReal) 0.0;
                                cptNrm[i][j].z = (RwReal) 0.0;
                            }
                        }

                        _rpPatchRefineTriVector( (RwV3d *)vertex,
                                                 stride,
                                                 cptNrm,
                                                 res );
                    }
                }

                pt += numTriVerts * stride;
            }
            else
            {
                /* Quad patch. */

                /* Set up the control point array. */
                for( i = 0; i < 4; i++ )
                {
                    for( j = 0; j < 4; j++ )
                    {
                        k = cptIdx[(j << 2) + i];
                        cptPos[i][j].x = vert[k].x;
                        cptPos[i][j].y = vert[k].y;
                        cptPos[i][j].z = vert[k].z;
                    }
                }

                _rpPatchRefineQuadVector( (RwV3d *)vertex,
                                          stride,
                                          cptPos,
                                          res );

                vertex += sizeof(RwV3d);

                /* Set up normals */
                if(meshFlags & rpPATCHMESHNORMALS)
                {
                    if(nrm != NULL)
                    {
                        for( i = 0; i < 4; i++ )
                        {
                            for( j = 0; j < 4; j++ )
                            {
                                k = cptIdx[(j << 2) + i];
                                cptNrm[i][j].x = nrm[k].x;
                                cptNrm[i][j].y = nrm[k].y;
                                cptNrm[i][j].z = nrm[k].z;
                            }
                        }

                        _rpPatchRefineQuadVector( (RwV3d *)vertex,
                                                  stride,
                                                  cptNrm,
                                                  res );
                    }
                }

                pt += numQuadVerts * stride;
            }

            numCtrlIndices -= rpPATCHNUMCONTROLINDICES;
            cptIdx += rpPATCHNUMCONTROLINDICES;
        }

        mesh++;
        numMesh--;
    }

    RWRETURN(TRUE);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/
