/************************************************************************
 * File: patchgeom.c                                                    *
 *                                                                      *
 * This patchexpands into a faked up geometry so that we can re-inject  *
 * into more conventional pipelines                                     *
 ************************************************************************/

#include <rwcore.h>
#include <rpworld.h>

#include "patchexpander.h"
#include "patchatomic.h"
#include "patch.h"

#include "patchgeom.h"


/* This callback isn't thread safe, but then neither is RW */
static void
PatchResEntryDestroy(RwResEntry *resEntry)
{
    RpGeometry *patchGeom;


    RWFUNCTION( RWSTRING( "PatchResEntryDestroy" ) );

    patchGeom = *(RpGeometry**)(resEntry+1);

    /* We have to destroy the fake geometry. */
    /* This relies on various bits of the library being re-entrant */

    patchGeom->numTriangles = 0;
    patchGeom->numVertices = 0;
    patchGeom->mesh = NULL;

    RpGeometryDestroy(patchGeom);

    /* OK, we can now let it go */

    RWRETURNVOID();
}

RpGeometry*
_rpPatchToGeometry(RpAtomic *atomic, modificationFn func)
{
    RpGeometry *geom;
    RwInt32 numVerts;
    RpMeshHeader *meshHeader;
    RpMesh *mesh;
    RwUInt16 numMeshes;
    PatchAtomicData *atomicData;
    RwUInt32 res;
    PatchMesh *patchMesh;
    RwUInt32 numQuadPatches;
    RwUInt32 numTriPatches;
    RwInt32 numIndices;
    RwInt32 numTris;
    RpGeometry *patchGeom;
    RwInt32 size;
    RwResEntry *repEntry;
    RwUInt32 numVertInQuad;
    RwUInt32 numVertInTri;
    RwUInt32 numIndInQuad;
    RwUInt32 numIndInTri;
    RwUInt32 i, j, k, l, m;

    RWFUNCTION(RWSTRING("_rpPatchToGeometry"));

    geom = RpAtomicGetGeometry(atomic);

    numVerts = geom->numVertices;
    if (numVerts <= 0)
    {
        RWRETURN((RpGeometry*)NULL);
    }

    meshHeader = geom->mesh;
    numMeshes = meshHeader->numMeshes;

    if (numMeshes == 0)
    {
        RWRETURN((RpGeometry*)NULL);
    }

    atomicData = PATCHATOMICGETDATA(atomic);

    /* Figure out how big this is going to be */
    res = (atomicData->lod.callback)(atomic, atomicData->lod.userData);

    patchMesh = atomicData->patchMesh;
    numQuadPatches = _rpPatchMeshGetNumQuadPatches(patchMesh);
    numTriPatches = _rpPatchMeshGetNumTriPatches(patchMesh);

    /* Total number of indices and verts. */
    numVertInQuad = PATCHQUADNUMVERT(res + 1);
    numVertInTri = PATCHTRINUMVERT(res + 1);

    numVerts = (numQuadPatches * numVertInQuad)
               + (numTriPatches * numVertInTri);

    numIndInQuad = PATCHQUADNUMINDEX(res + 1);
    numIndInTri = PATCHTRINUMINDEX(res + 1);
    /* Add extra for odd number of tri patches to preserve winding order. */
    if (res & 0x01)
    {
        numIndInTri++;
    }

    numIndices = (numQuadPatches * numIndInQuad)
                 + (numTriPatches * numIndInTri);

    numTris = (numQuadPatches * PATCHQUADNUMTRI(res + 1))
              + (numTriPatches * PATCHTRINUMTRI(res + 1));

    /* Add extra indices for connecting patches together. */
    numIndices += (numQuadPatches + numTriPatches - 1) * 2;

    /* This is a bit ugly. We really have to create the geometry using the
       real creation function as we can't call the any of the TK stuff from
       outside bageomet.c */
    patchGeom = RpGeometryCreate(0, 0,
                    rpGEOMETRYTEXCOORDSETS(RpGeometryGetNumTexCoordSets(geom))
                    | rpGEOMETRYTRISTRIP
                    | (RpGeometryGetFlags(geom)
                       & rpGEOMETRYFLAGSMASK));

    if (!patchGeom)
    {
       RWRETURN(NULL);
    }

    /* Someone may depend on these */
    patchGeom->numTriangles = numTris;
    patchGeom->numVertices = numVerts;

    /* We now need to allocate space for all the other data. This comprises:

       [res entry. implied]

       sizeof(RpGeometry*)

       sizeof(RpMeshHeader)

       sizeof(RpMesh) * numMeshes

       sizeof(RxVertexIndex) * numIndices

       sizeof(RwV3d) * numVerts    pos

       sizeof(RwV3d) * numVerts    normals (only if normals)

       sizeof(RwRGBA) * numVerts   colours (only if prelit)

       sizeof(RwTexCoords) * numVerts * RpGeometryGetNumTexCoordSets(geom)

       func ? sizeof(RwV3d) * geom->numVertices

       (func && geom->flags & rpGEOMETRYNORMALS) sizeof(RwV3d)
                                                 * geom->numVertices
       Materials are simply referenced out of the other geometry. We make
       the interesting assumption that it isn't going to go away before we
       do ...  */

    size = sizeof(RpGeometry*)
           + sizeof(RpMeshHeader)
           + sizeof(RpMesh) * numMeshes
           + sizeof(RxVertexIndex) * numIndices
           + sizeof(RwV3d) * numVerts
           + (RpGeometryGetFlags(geom) & rpGEOMETRYNORMALS ?
              sizeof(RwV3d) * numVerts : 0)
           + (RpGeometryGetFlags(geom) & rpGEOMETRYPRELIT ?
              sizeof(RwRGBA) * numVerts : 0)
           + RpGeometryGetNumTexCoordSets(geom) * sizeof(RwTexCoords)
             * numVerts;

    if (func)
    {
        size += sizeof(RwV3d) * RpGeometryGetNumVertices(geom);
        if (RpGeometryGetFlags(geom) & rpGEOMETRYNORMALS)
        {
            size += sizeof(RwV3d) * RpGeometryGetNumVertices(geom);
        }
    }

    if ((geom->repEntry) && (size != geom->repEntry->size))
    {
        RwResourcesFreeResEntry(geom->repEntry);
    }

    if (!(repEntry = RwResourcesAllocateResEntry(geom, &(geom->repEntry), size,
                                                 PatchResEntryDestroy)))
    {
       RpGeometryDestroy(patchGeom);
       RWRETURN(NULL);
    }
    else
    {
        RpGeometry **backPtr;
        RpMeshHeader *newMeshHeader;
        RpMesh *newMesh;
        RxVertexIndex *newIndices;
        RwV3d *newPos;
        RwV3d *newNorm;
        RwRGBA *newCol;
        RwTexCoords *newTexCoords, *texCoordPtr;
        RwUInt32 numCtrlIndices;
        RwBool firstpatch;
        RxVertexIndex *cptIdx;
        RwV3d *verts;
        RwV3d *norm;
        RwRGBA *preLit;
        RwInt32 base;

        backPtr = (RpGeometry**)(repEntry+1);

        if (!func)
        {
            verts = geom->morphTarget[0].verts;
            norm = geom->morphTarget[0].normals;

            newMeshHeader = (RpMeshHeader*)(backPtr+1);
        }
        else
        {
            verts = (RwV3d*)(backPtr+1);
            if (RpGeometryGetFlags(geom) & rpGEOMETRYNORMALS)
            {
                norm = (RwV3d*)(verts + RpGeometryGetNumVertices(geom));
            }
            else
            {
                norm = NULL;
            }

            /* Call the modifier. Probably skinning */
            func(atomic, verts, norm);

            newMeshHeader = (RpMeshHeader*)(verts
                                            +2*RpGeometryGetNumVertices(geom));
        }
        preLit = geom->preLitLum;

        newMesh = (RpMesh*)(newMeshHeader+1);
        newIndices = (RxVertexIndex*)(newMesh+numMeshes);
        newPos = (RwV3d*)(newIndices+numIndices);
        newNorm = (RwV3d*)(newPos+numVerts);
        newCol = (RwRGBA*)(newNorm
                           + (RpGeometryGetFlags(geom) & rpGEOMETRYNORMALS
                              ? numVerts : 0));
        newTexCoords = (RwTexCoords*)(newCol
                                  + (RpGeometryGetFlags(geom) & rpGEOMETRYPRELIT
                                     ? numVerts : 0));

        /* For later convience, set the non-existant fields to zero */
        if (!(RpGeometryGetFlags(geom) & rpGEOMETRYNORMALS))
        {
            newNorm = (RwV3d*)NULL;
        }
        if (!(RpGeometryGetFlags(geom) & rpGEOMETRYPRELIT))
        {
            newCol = (RwRGBA*)NULL;
        }
        if (RpGeometryGetNumTexCoordSets(geom) == 0)
        {
            newTexCoords = (RwTexCoords*)NULL;
        }

        *backPtr = patchGeom;

        patchGeom->preLitLum = newCol;
        for (i = 0; i < (RwUInt32)patchGeom->numTexCoordSets; i++)
        {
            patchGeom->texCoords[i] = newTexCoords + (i * numVerts );
        }
        patchGeom->mesh = newMeshHeader;
        /* By the convex hull property */
        patchGeom->morphTarget[0].boundingSphere
                                   = geom->morphTarget[0].boundingSphere;
        patchGeom->morphTarget[0].verts = newPos;
        patchGeom->morphTarget[0].normals = newNorm;

        /* We now build the fake meshes */
        newMeshHeader->flags = rpMESHHEADERTRISTRIP;
        newMeshHeader->numMeshes = numMeshes;
        newMeshHeader->serialNum = meshHeader->serialNum;
        newMeshHeader->totalIndicesInMesh = numIndices;
        newMeshHeader->firstMeshOffset = 0;

        mesh = (RpMesh *)((RwUInt8 *)(meshHeader + 1)
                          + meshHeader->firstMeshOffset);
        base = 0;
        for (i=0; i< numMeshes; i++)
        {
            newMesh->indices = newIndices;
            newMesh->numIndices = 0;
            newMesh->material = mesh->material;

            firstpatch = TRUE;
            numCtrlIndices = mesh->numIndices;
            cptIdx = mesh->indices;

            while (numCtrlIndices > 0)
            {
                RtBezierMatrix bezMat;
                RwTexCoords texArray[4];

                /* Duplicate the last index of the previous and the first
                 * index of the next patch. This connects the two together
                 * into a single tristrip.
                 */
                if (!firstpatch)
                {
                    newIndices[0] = newIndices[-1];
                    newIndices[1] = (RxVertexIndex) base;
                    newIndices += 2;
                    newMesh->numIndices += 2;
                }
                /* We use the fifth texCoords to flag if its a quad or tri
                 * patch
                 */
                if (newTexCoords
                    && (geom->texCoords[0][cptIdx[5]].u != (RwReal)0.0))
                {
                    /* Tri patch. */
                    l = 0;
                    for (j = 0; j < 4; j++)
                    {
                        for (k = 0; k < (4 - j); k++)
                        {
                            m = cptIdx[l];
                            bezMat[k][j].x = verts[m].x;
                            bezMat[k][j].y = verts[m].y;
                            bezMat[k][j].z = verts[m].z;
                            l++;
                        }

                        for (; k < 4; k++)
                        {
                            bezMat[k][j].x = (RwReal) 0.0;
                            bezMat[k][j].y = (RwReal) 0.0;
                            bezMat[k][j].z = (RwReal) 0.0;
                        }
                    }
                    _rpPatchRefineTriVector((RwV3d *)newPos,
                                            sizeof(RwV3d),
                                            bezMat,
                                            res);

                    newPos += numVertInTri;

                    /* Set up the normals */
                    if (RpGeometryGetFlags(geom) & rpGEOMETRYNORMALS)
                    {
                        if (norm != NULL)
                        {
                            l = 0;
                            for (j = 0; j < 4; j++)
                            {
                                for (k = 0; k < (4 - j); k++)
                                {
                                    m = cptIdx[l];
                                    bezMat[k][j].x = norm[m].x;
                                    bezMat[k][j].y = norm[m].y;
                                    bezMat[k][j].z = norm[m].z;
                                    l++;
                                }

                                for (; k < 4; k++)
                                {
                                    bezMat[k][j].x = (RwReal) 0.0;
                                    bezMat[k][j].y = (RwReal) 0.0;
                                    bezMat[k][j].z = (RwReal) 0.0;
                                }
                            }

                            _rpPatchRefineTriVector((RwV3d *)newNorm,
                                                    sizeof(RwV3d),
                                                    bezMat,
                                                    res);
                            newNorm += numVertInTri;
                        }
                    }

                    if (RpGeometryGetFlags(geom) & rpGEOMETRYPRELIT)
                    {
                        if (preLit != NULL)
                        {
                            l = 0;
                            for (j = 0; j < 4; j++)
                            {
                                for (k = 0; k < (4 - j); k++)
                                {
                                    m = cptIdx[l];
                                    bezMat[k][j].x = preLit[m].red;
                                    bezMat[k][j].y = preLit[m].green;
                                    bezMat[k][j].z = preLit[m].blue;
                                    bezMat[k][j].w = preLit[m].alpha;
                                    l++;
                                }

                                for (; k < 4; k++)
                                {
                                    bezMat[k][j].x = (RwReal) 0.0;
                                    bezMat[k][j].y = (RwReal) 0.0;
                                    bezMat[k][j].z = (RwReal) 0.0;
                                    bezMat[k][j].w = (RwReal) 0.0;
                                }
                            }

                            _rpPatchRefineTriColor((RwRGBA *)newCol,
                                                   sizeof(RwRGBA),
                                                   bezMat,
                                                   res);
                            newCol += numVertInTri;
                        }
                    }

                    texCoordPtr = newTexCoords;
                    for (j = 0; j<(RwUInt32)RpGeometryGetNumTexCoordSets(geom);
                         j++)
                    {
                        const RwTexCoords   *texCoords = geom->texCoords[j];

                        texArray[0] = texCoords[cptIdx[0]];
                        texArray[1] = texCoords[cptIdx[9]];
                        texArray[2] = texCoords[cptIdx[3]];

                        _rpPatchRefineTriTexCoords((RwTexCoords *)texCoordPtr,
                                                   sizeof(RwTexCoords),
                                                   texArray,
                                                   res);
                        texCoordPtr += numVerts;

                    }
                    if (j > 0)
                    {
                        newTexCoords += numVertInTri;
                    }


                    _rpPatchGenerateTriIndex(newIndices, base, res);

                    base += numVertInTri;

                    newMesh->numIndices += numIndInTri;
                    newIndices += numIndInTri;
                }
                else
                {
                    /* Quad patch */
                    /* Set up the control point array. */
                    for (j = 0; j < 4; j++)
                    {
                        for (k = 0; k < 4; k++)
                        {
                            l = cptIdx[(k << 2) + j];
                            bezMat[j][k].x = verts[l].x;
                            bezMat[j][k].y = verts[l].y;
                            bezMat[j][k].z = verts[l].z;
                        }
                    }

                    _rpPatchRefineQuadVector((RwV3d *)newPos,
                                             sizeof(RwV3d),
                                             bezMat,
                                             res);
                    newPos += numVertInQuad;

                    if (RpGeometryGetFlags(geom) & rpGEOMETRYNORMALS)
                    {
                        if (norm != NULL)
                        {
                            for (j = 0; j < 4; j++)
                            {
                                for (k = 0; k < 4; k++)
                                {
                                    l = cptIdx[(k << 2) + j];
                                    bezMat[j][k].x = norm[l].x;
                                    bezMat[j][k].y = norm[l].y;
                                    bezMat[j][k].z = norm[l].z;
                                }
                            }

                            _rpPatchRefineQuadVector((RwV3d *)newNorm,
                                                     sizeof(RwV3d),
                                                     bezMat,
                                                     res);

                            newNorm += numVertInQuad;
                        }

                    }

                    if (RpGeometryGetFlags(geom) & rpGEOMETRYPRELIT)
                    {
                        if (preLit != NULL)
                        {
                            for (j = 0; j < 4; j++)
                            {
                                for (k = 0; k < 4; k++)
                                {
                                    RwUInt32    temp = cptIdx[(k << 2) + j];

                                    bezMat[j][k].x = preLit[temp].red;
                                    bezMat[j][k].y = preLit[temp].green;
                                    bezMat[j][k].z = preLit[temp].blue;
                                    bezMat[j][k].w = preLit[temp].alpha;
                                }
                            }

                            _rpPatchRefineQuadColor((RwRGBA *)newCol,
                                                    sizeof(RwRGBA),
                                                    bezMat,
                                                    res);

                            newCol += numVertInQuad;
                        }
                    }

                    /* Some tex coordinate stuff here! */
                    texCoordPtr = newTexCoords;
                    for (j = 0; j<(RwUInt32)RpGeometryGetNumTexCoordSets(geom);
                         j++)
                    {
                        const RwTexCoords   *texCoords = geom->texCoords[j];

                        texArray[0] = texCoords[cptIdx[0]];
                        texArray[1] = texCoords[cptIdx[3]];
                        texArray[2] = texCoords[cptIdx[12]];
                        texArray[3] = texCoords[cptIdx[15]];

                        _rpPatchRefineQuadTexCoords((RwTexCoords *)texCoordPtr,
                                                    sizeof(RwTexCoords),
                                                    texArray,
                                                    res);

                        texCoordPtr += numVerts;
                    }
                    if (j > 0)
                    {
                        newTexCoords += numVertInQuad;
                    }

                    _rpPatchGenerateQuadIndex(newIndices, base, res);

                    base += numVertInQuad;

                    newMesh->numIndices += numIndInQuad;
                    newIndices += numIndInQuad;
                }
                firstpatch = FALSE;

                numCtrlIndices -= rpPATCHNUMCONTROLINDICES;
                cptIdx += rpPATCHNUMCONTROLINDICES;
            }

            mesh++;
            newMesh++;
        }

        /* unlock */
        if (!RpGeometryUnlock(patchGeom))
        {
            /* We destroy the repentry, which distroys the geometry */
            RwResourcesFreeResEntry(repEntry);
        }
        geom->lockedSinceLastInst = 0;
        patchGeom->lockedSinceLastInst = 0;
        RWRETURN(patchGeom);
    }
}
