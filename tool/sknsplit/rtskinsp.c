/***************************************************************************
 *                                                                         *
 * Module  :                                                               *
 *                                                                         *
 * Purpose :                                                               *
 *                                                                         *
 **************************************************************************/

/****************************************************************************
 Includes
 */

#include <stdlib.h>
#include <string.h>

#include <rwcore.h>
#include <rpworld.h>
#include <rpskin.h>


#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rtskinsp.h"
#include "skinsplit.h"

/**
 * \ingroup rtskinsplit
 * \page rtskinsplitoverview RtSkinSplit Toolkit Overview
 *
 * Some platforms impose a limit on the number bones permissible in a single
 * object. This limit is due to the number of matrices that can be uploaded to
 * platform hardware. This number can vary from platform to platform. Please
 * refer to the platform specific section \ref rpskinbonelimit in the \ref
 * rpskin plugin documentation for more information on is subject.
 *
 * An object that requires more bones that can be supported on the target
 * platform will fail to load and must be remodeled as mulitple objects.
 *
 * \ref RtSkinSplitAtomicSplitGeometry attempts to offset the limitations by
 * rearranging the meshes so each mesh requires less than the platform limit.
 * Only the bone matrices required by each mesh are loaded.
 *
 * This means an object can have more bones than supported by the platform
 * and still render. The bone restriction is moved from the object level to
 * the mesh level.
 *
 * Only polygonal based objects can be split. Non-polygonal objects, patches,
 * are not supported. The platforms' bone limit still applies to these
 * objects.
 *
 * There is a performance penalty in rendering a split object to
 * a non-split object. The performance lost will vary from  platform to
 * platform. The performance lost is due to the extra meshes present. Each
 * extra mesh requires additional processing not present in a non-split
 * object.
 *
 * The selective loading of bone matrices is also less efficient than
 * loading all the bone matrices in one transfer.
 *
 * \warning Once a geometry has been split using RtSkinSplit, it cannot
 * be locked and unlocked to force re-tristripping or mesh generation as 
 * this will undo the work done by RtSkinSplit. This is due to the way
 * materials are used to define partitions. It is assumed that each material
 * is a separate partition but, in the case of RtSkinSplit, we have many 
 * partitions all using the same material.
 *
 */


/****************************************************************************
 Defines
 */

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */
/****************************************************************************
_rtSkinSplitMeshBoneRLEValidate does a check on the RLE and the mesh to ensure
 the split data is valid. This would include the mesh's RLE correctly includes
 all the required matrices for the mesh.
 */
static rtSkinSplitMatTriList *
_rtSkinSplitMeshBoneRLEValidate(rtSkinSplitGeomStat *geomStat,
                                rtSkinSplitMatTriList *matTriList,
                                RpMesh *mesh, RwUInt32 meshId )
{
    RwUInt32            i, j, idx;
    RwUInt8             *rle;
    RwUInt8             *skinMeshRLECount;
    RwUInt8             *skinMeshRLE;

    RwUInt32                    skinBoneIdx;
    const RwMatrixWeights       *skinBoneWeight;

    RwUInt32                    numRLE, numMtx;
    rtSkinSplitBoneFlag         boneFlag;

    RWFUNCTION(RWSTRING("_rtSkinSplitMeshBoneRLEValidate"));

    skinMeshRLECount = geomStat->skinMeshBoneRLECount;
    skinMeshRLE = geomStat->skinMeshBoneRLE;

    /* Build the boneFlag from the RLE */
    _rtSkinSplitBoneFlagClear(&boneFlag, 0);

    numRLE = skinMeshRLECount[(2 * meshId) + 1];
    rle = &skinMeshRLE[skinMeshRLECount[(2 * meshId)] * 2];

    for (i = 0; i < numRLE; i++)
    {
        idx = rle[(i * 2)];
        numMtx = rle[(i * 2) + 1];

        for (j = 0; j < numMtx; j++)
        {
            _rtSkinSplitBoneFlagSetIdx(&boneFlag, idx);
            idx++;
        }
    }

    /* Check that the vertices' bone index exist in the mesh */
    for (i = 0; i < mesh->numIndices; i++)
    {
        skinBoneIdx = geomStat->skinBoneIndex[mesh->indices[i]];
        skinBoneWeight = &geomStat->skinBoneWeight[mesh->indices[i]];

        if (skinBoneWeight->w0 > 0)
        {
            idx = skinBoneIdx & 0xFF;
            if (!_rtSkinSplitBoneFlagGetIdx(&matTriList->boneFlag, idx))
            {
                /* Something is not right */
                 RWASSERT(FALSE);
            }
            if (!_rtSkinSplitBoneFlagGetIdx(&boneFlag, idx))
            {
                /* Something is not right */
                RWASSERT(FALSE);
            }
        }
        if (skinBoneWeight->w1 > 0)
        {
            skinBoneIdx >>= 8;
            idx = skinBoneIdx & 0xFF;
            if (!_rtSkinSplitBoneFlagGetIdx(&matTriList->boneFlag, idx))
            {
                /* Something is not right */
                RWASSERT(FALSE);
            }
            if (!_rtSkinSplitBoneFlagGetIdx(&boneFlag, idx))
            {
                /* Something is not right */
                RWASSERT(FALSE);
            }
        }
        if (skinBoneWeight->w2 > 0)
        {
            skinBoneIdx >>= 8;
            idx = skinBoneIdx & 0xFF;
            if (!_rtSkinSplitBoneFlagGetIdx(&matTriList->boneFlag, idx))
            {
                /* Something is not right */
                RWASSERT(FALSE);
            }
            if (!_rtSkinSplitBoneFlagGetIdx(&boneFlag, idx))
            {
                /* Something is not right */
                RWASSERT(FALSE);
            }
        }
        if (skinBoneWeight->w3 > 0)
        {
            skinBoneIdx >>= 8;
            idx = skinBoneIdx & 0xFF;
            if (!_rtSkinSplitBoneFlagGetIdx(&matTriList->boneFlag, idx))
            {
                /* Something is not right */
                RWASSERT(FALSE);
            }
            if (!_rtSkinSplitBoneFlagGetIdx(&boneFlag, idx))
            {
                /* Something is not right */
                RWASSERT(FALSE);
            }
        }
    }

    RWRETURN (matTriList);
}

/****************************************************************************
 _rtSkinSplitMeshSetBoneRLE build matrix batches for the mesh. The matrix's
 batch are stored in RLE format. A mesh can contain more one batch if the
 matrices are not in a continuous block in the RpSkin matrix array.
 */
static RpMesh *
_rtSkinSplitMeshSetBoneRLE(RpMesh *mesh, RpMeshHeader *meshHeader, void *data)
{
    RwUInt32        i, meshId;
    RpMesh          *meshHead;
    RwUInt8         *rle;
    RwUInt8         *skinMeshRLECount;
    RwUInt8         *skinMeshRLE;

    rtSkinSplitGeomStat         *geomStat;
    rtSkinSplitMatTriList       *matTriList;

    RWFUNCTION(RWSTRING("_rtSkinSplitMeshSetBoneRLE"));

    meshHead = (RpMesh *)
        ((RwChar *) (meshHeader + 1)) + meshHeader->firstMeshOffset;
    meshId = mesh - meshHead;

    geomStat = (rtSkinSplitGeomStat *) data;
    matTriList = geomStat->matTriListDone;
    skinMeshRLECount = geomStat->skinMeshBoneRLECount;
    skinMeshRLE = geomStat->skinMeshBoneRLE;

    /* Search the matTri list for a matching material */
    while (matTriList->mat != mesh->material)
    {
        matTriList = matTriList->next;
    }

    /* Write the RLE count */
    skinMeshRLECount[(2 * meshId) + 1] = matTriList->numRLE;

    if (meshId == 0)
    {
        skinMeshRLECount[0] = 0;
    }
    else
    {
        skinMeshRLECount[(2 * meshId)] =
            skinMeshRLECount[(2 * meshId) - 1] +
            skinMeshRLECount[(2 * meshId) - 2];
    }

    /* Write the RLE */
    rle = &skinMeshRLE[skinMeshRLECount[(2 * meshId)] * 2];

    for (i = 0; i < matTriList->numRLE * 2; i++)
    {
        rle[i] = (RwUInt8) matTriList->rleMap[i];
    }

    /* Check everything is okay */
    _rtSkinSplitMeshBoneRLEValidate(geomStat, matTriList, mesh, meshId);

    /* Remap the material index back to the original material. */
    mesh->material = _rpMaterialListGetMaterial(
        (const RpMaterialList *) &geomStat->geom->matList,
        matTriList->origMatIdx);

    RWRETURN (mesh);
}

/****************************************************************************
 _rtSkinSplitGeomResetTriMat resets the mesh's material id to the original
 material. To split a mesh into seperate meshes, the parent mesh's material
 is cloned in order for the new child meshes to be created. These child meshes'
 materials needs to be reset to the parent so a change in the parent's material
 will affect all the child meshes.
 */
static RpGeometry *
_rtSkinSplitGeomResetTriMat(rtSkinSplitGeomStat *geomStat, RpGeometry *geom)
{
    RwUInt32            i, j;
    RpTriangle          *tri;

    rtSkinSplitMatTriList       *matTriList;

    RWFUNCTION(RWSTRING("_rtSkinSplitGeomResetTriMat"));

    /*-----------------24/07/2002 16:42-----------------
     * Reset the triangles' material. Do this behind the scene
     * without locking the geometry to prevent recreation of the
     * meshes.
     * --------------------------------------------------*/
    tri = RpGeometryGetTriangles(geom);

    /* Change the materials of the triangles. */
    matTriList = geomStat->matTriListDone;
    while (matTriList != NULL)
    {
        /* Reset this mesh's triangles' mat index */
        for (i = 0; i < matTriList->numTris; i++)
        {
            j = matTriList->triList[i];

            tri[j].matIndex = matTriList->origMatIdx;
        }

        /* Grab the new mat index */

        matTriList = matTriList->next;
    }

    RWRETURN(geom);
}

/****************************************************************************
 _rtSkinSplitCreateGeometryMesh is the top level function to recreates the
 geometry's meshes from the list of matTriList data structures.
 It also sets up the bone spliting data in the RpSkin. This the matrix batches
 and the remapping of the bone matrix indices for the vertices.
 */
static RpAtomic *
_rtSkinSplitCreateGeometryMesh(rtSkinSplitGeomStat *geomStat, RpAtomic *atomic)
{
    RwUInt32            i, j;
    RpAtomic            *result;
    RpGeometry          *geom;
    RpTriangle          *tri;

    rtSkinSplitMatTriList       *matTriList;

    RWFUNCTION(RWSTRING("_rtSkinSplitCreateGeometryMesh"));

    geom = geomStat->geom;

    /* Lock the geometry */
    RpGeometryLock(geom, rpGEOMETRYLOCKPOLYGONS);

    tri = RpGeometryGetTriangles(geom);

    /* Change the materials of the triangles. */
    matTriList = geomStat->matTriListDone;
    while (matTriList != NULL)
    {
        for (i = 0; i < matTriList->numTris; i++)
        {
            j = matTriList->triList[i];

            RpGeometryTriangleSetMaterial(geom, &tri[j],
                matTriList->mat);
        }

        matTriList = matTriList->next;
    }

    /* Unlock the geometry to recreate the meshes */
    RpGeometryUnlock(geom);

    /* Create the bone remap index, do this after geom unlock */

    /* Calculate how many total number of matrix batches
     */
    i = 0;
    matTriList = geomStat->matTriListDone;
    while (matTriList != NULL)
    {
        /* rle size per matTri list */
        i += matTriList->numRLE;

        matTriList = matTriList->next;
    }

   /* Destroy the previous split data */
   _rpSkinSplitDataDestroy(geomStat->skin);

    /* Create the bone split data in the skin */
    _rpSkinSplitDataCreate(
        geomStat->skin, geomStat->boneLimit,
        geomStat->numBones, geomStat->numMatTriLists, i );

    geomStat->skinBoneRemapIndices =
        _rpSkinGetMeshBoneRemapIndices(geomStat->skin);
    geomStat->skinMeshBoneRLECount =
        _rpSkinGetMeshBoneRLECount(geomStat->skin);
    geomStat->skinMeshBoneRLE =
        _rpSkinGetMeshBoneRLE(geomStat->skin);

    /* Copy the matrix index */
    for (i = 0; i < geomStat->numBones; i++)
    {
        geomStat->skinBoneRemapIndices[i] = (RwUInt8)
            geomStat->boneRemapIndex[i];
    }

    /* Copy over the remap index to the geometry */
    RpGeometryForAllMeshes(geom,
        _rtSkinSplitMeshSetBoneRLE, (void *) geomStat);

    result = atomic;

    /* Reinitialise the bone index and weights */
    _rpSkinDeinitialize(geom);
    _rpSkinInitialize(geom);

    /* Naughty code
     *
     * We now modify the materials' indices of the unlock geometry.
     * Any mesh or triangles that uses the 'new' material are changed
     * back to the originally one.
     */
    _rtSkinSplitGeomResetTriMat(geomStat, geom);

    RWRETURN (result);
}

/**
 * \ingroup rtskinsplit
 * \ref RtSkinSplitAtomicSplitGeometry splits the geometry such that each mesh
 * will use, at most, the 'boneLimit' number of bones matrices.
 *
 * The source geometric data is not modified. Instead, the underlying
 * meshes are rearranged such that each mesh requires at most, the specified
 * bone limit.
 *
 * If the atomic's geometry does not require rearranging, then it is left
 * untouched. If the atomic's geometry has been already been split, then the
 * previous rearrangement is lost and is replaced by the new version.
 *
 * \param atomic        Pointer to the \ref RpAtomic to be split.
 * \param boneLimit     The maximum number of bones per mesh.
 *
 * \return A pointer to the atomic if successful, NULL otherwise.
 *
 * \see RpSkinCreate
 */

RpAtomic *
RtSkinSplitAtomicSplitGeometry( RpAtomic *atomic, RwUInt32 boneLimit )
{
    rtSkinSplitGeomStat         geomStat;
    rtSkinSplitMatTriList       *matTriList, *matTriListNew, *matTriListNext;

    RpAtomic                    *result;
    RpMaterial                  *mat;
    RwUInt32                    i, size;

    RWAPIFUNCTION(RWSTRING("RtSkinSplitAtomicSplitGeometry"));

    /* Sanity check */
    RWASSERT(atomic);
    result = atomic;

    memset(&geomStat, 0, sizeof(rtSkinSplitGeomStat));

    geomStat.atomic = atomic;
    geomStat.boneLimit = boneLimit;

    /* Get the hierarchy structure of the atomic */
    geomStat.hierarchy = RpSkinAtomicGetHAnimHierarchy(geomStat.atomic);
    geomStat.geom = RpAtomicGetGeometry(geomStat.atomic);

    geomStat.skin = RpSkinGeometryGetSkin(geomStat.geom);
    geomStat.numBones = RpSkinGetNumBones(geomStat.skin);

    /* Check for skin and hierarchy */
    if (geomStat.skin == NULL)
    {
        RWMESSAGE((RWSTRING("Unable to split geometry. No skin data found.")));
        RWASSERT(geomStat.skin);

		//. 2006. 1. 17. Nonstopdj
		atomic->skinSplitFlags = rpATOMICSKINSPLITNOTUSE;

        RWRETURN(NULL);
    }

    if (geomStat.hierarchy == NULL)
    {
        RWMESSAGE((RWSTRING("Unable to split geometry. No hierarchical animation found.")));
        RWASSERT(geomStat.hierarchy);

		//. 2006. 1. 17. Nonstopdj
		atomic->skinSplitFlags = rpATOMICSKINSPLITNOTUSE;

		RWRETURN(NULL);
    }

    /* Check if the geometry is pre-instanced */
    if (RpGeometryGetFlags(geomStat.geom) & rpGEOMETRYNATIVE)
    {
        RWMESSAGE((RWSTRING("Unable to split geometry. Geometry is in native format.")));
        RWASSERT(!(RpGeometryGetFlags(geomStat.geom) & rpGEOMETRYNATIVE));

		//. 2006. 1. 17. Nonstopdj
		atomic->skinSplitFlags = rpATOMICSKINSPLITNOTUSE;

        RWRETURN(NULL);
    }

    /* Check if the bone limit is already satisifed. */
    if (geomStat.numBones <= boneLimit)
	{
		//. 2006. 1. 17. Nonstopdj
		atomic->skinSplitFlags = rpATOMICSKINSPLITCOMPLETE;

		RWRETURN(atomic);
	}

    geomStat.skinBoneIndex = RpSkinGetVertexBoneIndices(geomStat.skin);
    geomStat.skinBoneWeight = RpSkinGetVertexBoneWeights(geomStat.skin);

    /* Get some counters */
    geomStat.numTris = RpGeometryGetNumTriangles(geomStat.geom);
    geomStat.numMats = RpGeometryGetNumMaterials(geomStat.geom);
    geomStat.numVerts = RpGeometryGetNumVertices(geomStat.geom);

    geomStat.matList = &geomStat.geom->matList;

    /* Allocate some temp memory */
    size =
           /* mesh bone count */
           (geomStat.numMats * sizeof(RwUInt32)) +
           /* bone mesh count */
           (geomStat.numBones * sizeof(RwUInt32)) +
           /* mesh tri count */
           (geomStat.numMats * sizeof(RwUInt32)) +
           /* bone vert count per mesh */
           (geomStat.numMats * geomStat.numBones * sizeof(RwUInt32)) +
           /* bone child bone count */
           (geomStat.numBones * sizeof(RwUInt32)) +
           /* bone child bone flag */
           (geomStat.numBones * sizeof(rtSkinSplitBoneFlag));

    geomStat.meshBoneCount = (RwUInt32 *) RwMalloc(size,
        rwID_SKINSPLITTOOKIT | rwMEMHINTDUR_FUNCTION);

    /* Mem failure, unble to split */
    if (geomStat.meshBoneCount == NULL)
	{
		//. 2006. 1. 17. Nonstopdj
		atomic->skinSplitFlags = rpATOMICSKINSPLITNOTUSE;

        RWRETURN(NULL);
	}

    memset(geomStat.meshBoneCount, 0, size);

    geomStat.boneMeshCount = geomStat.meshBoneCount + geomStat.numMats;
    geomStat.meshTriCount = geomStat.boneMeshCount + geomStat.numBones;
    geomStat.boneVertCount = geomStat.meshTriCount + geomStat.numMats;
    geomStat.boneBoneCount = geomStat.boneVertCount +
                             (geomStat.numMats * geomStat.numBones);
    geomStat.boneBoneFlag = (rtSkinSplitBoneFlag *)
                            (geomStat.boneBoneCount + geomStat.numBones);

    /* collect some stats */
    _rtSkinSplitGeomGetStat(&geomStat);

    /* Check if the bone limit is more than the number of weights used or
     * greater than the maximum number of bone used by a triangle. */
    if (boneLimit < geomStat.numWeights)
    {
        /* Unable to split due to number of weights is higher than the
         * bone limit */
        RWMESSAGE((RWSTRING("Unable to split geometry. Max number of weights used by a vertex is higher than the bone limit.")));
        RwFree(geomStat.meshBoneCount);

		//. 2006. 1. 17. Nonstopdj
		atomic->skinSplitFlags = rpATOMICSKINSPLITNOTUSE;

        RWRETURN(NULL);
    }

    if (boneLimit < geomStat.maxTriBones)
    {
        /* Unable to split due to number of bones used by a tri is higher than the
         * bone limit */
        RWMESSAGE((RWSTRING("Unable to split geometry. Max number of bones used by a triangle is higher than bone the limit.")));
        RwFree(geomStat.meshBoneCount);

		//. 2006. 1. 17. Nonstopdj
		atomic->skinSplitFlags = rpATOMICSKINSPLITNOTUSE;

        RWRETURN(NULL);
    }

    /* Go through each material (mesh) and check if it exceeds the bone
     * limit, if so, split it until it is less than the limit.
     */

    /* Build a set of matTriList, triangles grouped by materials,
        * and seperate the lists that need to be split and those don't.
        */
    for (i = 0; i < geomStat.numMats; i++)
    {
        /* Create the triangle lists grouped by materials */
        matTriList = _rtSkinSplitMatTriListCreate(
            geomStat.meshTriCount[i]);

        geomStat.numMatTriLists++;

        if (matTriList != NULL)
        {
            mat = _rpMaterialListGetMaterial(geomStat.matList, i);

            _rtSkinSplitMatTriListAddTriByMat(
                    &geomStat, matTriList, mat, i);
            _rtSkinSplitMatTriListGetBoneFlag(
                    &geomStat, matTriList);

            if (matTriList->numBones > boneLimit)
            {
                geomStat.matTriListStack = _rtSkinSplitMatTriListPush(
                    geomStat.matTriListStack, matTriList);
            }
            else
            {
                geomStat.matTriListDone = _rtSkinSplitMatTriListPush(
                    geomStat.matTriListDone, matTriList);
            }
        }
        else
        {
            /* Mem failure. Need to clean up */
            RWMESSAGE((RWSTRING("Unable to split geometry. Memory allocation failure.")));
            result = NULL;
        }
    }

    /* Go through the split list stack and split any meshes there */
    if (result != NULL)
    {
        geomStat.matTriListStack = _rtSkinSplitMatTriListPop(
            geomStat.matTriListStack, &matTriList);

        while (matTriList)
        {
            if (matTriList->numBones > boneLimit)
            {
                /* Split the mesh using the bone */
                matTriListNew = _rtSkinSplitMatTriListSplit(
                    &geomStat, matTriList, boneLimit);

                if (matTriListNew != NULL)
                {
                    /* Push the two list back into the process stack */
                    geomStat.matTriListStack = _rtSkinSplitMatTriListPush(
                        geomStat.matTriListStack, matTriList);
                    geomStat.matTriListStack = _rtSkinSplitMatTriListPush(
                        geomStat.matTriListStack, matTriListNew);
                }
                else
                {
                    /* Mem failure. Need to clean up */
                    RWMESSAGE((RWSTRING("Unable to split geometry. Memory allocation failure.")));
                    result = NULL;
                }
            }
            else
            {
                /* Nothing to be done so just add it to the done list */
                geomStat.matTriListDone = _rtSkinSplitMatTriListPush(
                    geomStat.matTriListDone, matTriList);
            }

            geomStat.matTriListStack =_rtSkinSplitMatTriListPop(
                geomStat.matTriListStack, &matTriList);
        }
    }

    /* Remap the bone index. The original index is still kept with
     * the skin but a remap array is created for the geometry.
     */
    if (result != NULL)
    {
        if (_rtSkinSplitMatTriListGetBoneLockFlag(
            &geomStat, geomStat.matTriListDone) != &geomStat)
        {
            result = NULL;
        }
    }

    if (result != NULL)
    {
        if (_rtSkinSplitMatTriListRemapBoneIndex(
            &geomStat, geomStat.matTriListDone) != &geomStat)
        {
            result = NULL;
        }
    }

    if (result != NULL)
    {
        /* Reconstruct the meshes */
        if (_rtSkinSplitCreateGeometryMesh(&geomStat, atomic) != atomic)
        {
            result = NULL;
        }
    }

    /* Clean up */
    RwFree(geomStat.meshBoneCount);
    RwFree(geomStat.boneRemapIndex);

    matTriList = geomStat.matTriListDone;
    while (matTriList != NULL)
    {
        matTriListNext = matTriList->next;

        _rtSkinSplitMatTriListDestroy(matTriList);

        matTriList = matTriListNext;
    }

	//. 2006. 1. 17. Nonstopdj
	atomic->skinSplitFlags = rpATOMICSKINSPLITCOMPLETE;

    RWRETURN (result);
}


