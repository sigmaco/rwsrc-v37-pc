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

/****************************************************************************
 Defines
 */

/****************************************************************************
 Globals (across program)
 */
__declspec( thread ) rtSkinSplitGlobalVars rtSkinSplitGlobals;

RwUInt8 _rtSkinSplitBitTable[16] =
    { 0x00, 0x01, 0x01, 0x02,
      0x01, 0x02, 0x02, 0x03,
      0x01, 0x02, 0x02, 0x03,
      0x02, 0x03, 0x03, 0x04 };


/****************************************************************************
 Local (static) Globals
 */


/****************************************************************************
 _rwFunction

 On entry   :
 On exit    :
 */

/****************************************************************************
 rtSkinSplitTriClassify classifies a triangle against a bone flag. If the
 triangle uses any bones in the flag, it returns TRUE, otherwise FALSE.

 */
RwBool
_rtSkinSplitTriClassify( rtSkinSplitGeomStat *geomStat,
                        RpTriangle *tri,
                        rtSkinSplitBoneFlag *boneFlag )
{
    RwBool                  result;
    RwUInt32                i, mtxIdx, idx;
    rtSkinSplitBoneFlag     mtxFlag;
    const RwUInt32          *skinBoneIndex;
    const RwMatrixWeights   *skinBoneWeight;

    RWFUNCTION(RWSTRING("_rtSkinSplitTriClassify"));

    /*
     * Decide if the current triangle should be kept depending on its
     * vertices' bone matrix index.
     */

    skinBoneIndex = RpSkinGetVertexBoneIndices(geomStat->skin);
    skinBoneWeight = RpSkinGetVertexBoneWeights(geomStat->skin);

    /* Clear the flag */
    _rtSkinSplitBoneFlagClear(&mtxFlag, 0);

    /* Convert the vertices's bone mtx into a flag */
    for (i = 0; i < 3; i++)
    {
        mtxIdx = skinBoneIndex[tri->vertIndex[i]];

        if (skinBoneWeight[tri->vertIndex[i]].w0 > 0)
        {
            idx = mtxIdx & 0xFF;
            _rtSkinSplitBoneFlagSetIdx(&mtxFlag, idx);
        }
        if (skinBoneWeight[tri->vertIndex[i]].w1 > 0)
        {
            mtxIdx >>= 8;
            idx = mtxIdx & 0xFF;
            _rtSkinSplitBoneFlagSetIdx(&mtxFlag, idx);
        }
        if (skinBoneWeight[tri->vertIndex[i]].w2 > 0)
        {
            mtxIdx >>= 8;
            idx = mtxIdx & 0xFF;
            _rtSkinSplitBoneFlagSetIdx(&mtxFlag, idx);
        }
        if (skinBoneWeight[tri->vertIndex[i]].w3 > 0)
        {
            mtxIdx >>= 8;
            idx = mtxIdx & 0xFF;
            _rtSkinSplitBoneFlagSetIdx(&mtxFlag, idx);
        }

    }

    result = (mtxFlag.flag[0] & boneFlag->flag[0]) ||
             (mtxFlag.flag[1] & boneFlag->flag[1]) ||
             (mtxFlag.flag[2] & boneFlag->flag[2]) ||
             (mtxFlag.flag[3] & boneFlag->flag[3]) ||
             (mtxFlag.flag[4] & boneFlag->flag[4]) ||
             (mtxFlag.flag[5] & boneFlag->flag[5]) ||
             (mtxFlag.flag[6] & boneFlag->flag[6]) ||
             (mtxFlag.flag[7] & boneFlag->flag[7]);

    RWRETURN(result);
}


/****************************************************************************
 rtSkinSplitMatTriListPush pushes a matTriList to an existing stack,
 returning the new top of the stack.

 */
rtSkinSplitMatTriList *
_rtSkinSplitMatTriListPush( rtSkinSplitMatTriList *triListHead,
                            rtSkinSplitMatTriList *triList )
{
    RWFUNCTION(RWSTRING("_rtSkinSplitMatTriListPush"));

    triList->next = triListHead;

    RWRETURN (triList);
}

/****************************************************************************
 rtSkinSplitMatTriListPop pops a matTriList from the a stack,
 returning the top of the stack.

 */
rtSkinSplitMatTriList *
_rtSkinSplitMatTriListPop( rtSkinSplitMatTriList *triListHead,
                           rtSkinSplitMatTriList **triListPtr )
{
    rtSkinSplitMatTriList       *triListHeadNew;

    RWFUNCTION(RWSTRING("_rtSkinSplitMatTriListPop"));

    triListHeadNew = NULL;

    if (triListHead != NULL)
    {
        triListHeadNew = triListHead->next;
        *triListPtr = triListHead;
        triListHead->next = NULL;
    }
    else
    {
        *triListPtr = NULL;
    }

    RWRETURN(triListHeadNew);
}

/****************************************************************************
 rtSkinSplitMatTriListCreate a new empty matTriList.

 */
rtSkinSplitMatTriList *
_rtSkinSplitMatTriListCreate( RwUInt32 count )
{
    rtSkinSplitMatTriList   *matTriList;
    RwUInt32                size;

    RWFUNCTION(RWSTRING("_rtSkinSplitMatTriListCreate"));

    size = /* Size of the structure header */
           sizeof(rtSkinSplitMatTriList) +
           /* Size of the triangle index array */
           count * sizeof(RwUInt32);

    matTriList = (rtSkinSplitMatTriList *) RwMalloc(size,
            rwID_SKINSPLITTOOKIT | rwMEMHINTDUR_EVENT);

    memset(matTriList, 0, size);

    if (matTriList)
    {
        matTriList->numBones = 0;
        matTriList->boneRoot = 0;

        _rtSkinSplitBoneFlagClear(&matTriList->boneFlag, 0);
        _rtSkinSplitBoneFlagClear(&matTriList->boneLockFlag, 0);

        matTriList->mat = NULL;
        matTriList->numTris = 0;
        matTriList->maxTris = count;

        matTriList->triList = (RwUInt32 *) (matTriList + 1);
    }

    RWRETURN(matTriList);
}

/****************************************************************************
 _rtSkinSplitMatTriListDestroy destroys the matTriList.

 */
RwBool
_rtSkinSplitMatTriListDestroy( rtSkinSplitMatTriList *matTriList )
{
    RWFUNCTION(RWSTRING("_rtSkinSplitMatTriListDestroy"));

    /*
     * Reduce the material's ref count so it is destroyed. Do this only
     * for the cloned materials.
     */
    if (matTriList->matIdx < 0)
        RpMaterialDestroy(matTriList->mat);

    RwFree(matTriList);

    RWRETURN(TRUE);
}

/****************************************************************************
 rtSkinSplitMatTriListAddTriByMat goes through and add all triangles in the
 geometry that uses the given the material to the matTriList.

 */
rtSkinSplitMatTriList *
_rtSkinSplitMatTriListAddTriByMat( rtSkinSplitGeomStat *geomStat,
                                   rtSkinSplitMatTriList *matTriList,
                                   RpMaterial *mat,
                                   RwUInt32 matIdx )
{
    RpTriangle              *tri;
    RwUInt32                i, j;

    RWFUNCTION(RWSTRING("_rtSkinSplitMatTriListAddTriByMat"));

    tri = RpGeometryGetTriangles(geomStat->geom);

    matTriList->mat = mat;
    matTriList->matIdx = matIdx;
    matTriList->origMatIdx = matIdx;
    matTriList->numTris = 0;

    j = 0;
    for (i = 0; i < geomStat->numTris; i++)
    {
        /* Add this tri to this list */
        if (tri->matIndex == (RwUInt16) matIdx)
        {
            matTriList->triList[j] = i;
            matTriList->numTris++;

            j++;
        }

        tri++;
    }

    RWRETURN (matTriList);
}


/****************************************************************************
 rtSkinSplitMatTriListGetBoneFlag builds a flag identify all the bones used
 by the matTriList.

 */
rtSkinSplitMatTriList *
_rtSkinSplitMatTriListGetBoneFlag( rtSkinSplitGeomStat *geomStat,
                                   rtSkinSplitMatTriList *matTriList )
{
    RpTriangle              *tri;
    const RwUInt32          *skinBoneIndex;
    const RwMatrixWeights   *skinBoneWeight;
    RwUInt32                i, j, triIdx, mtxIdx, idx;

    RWFUNCTION(RWSTRING("_rtSkinSplitMatTriListGetBoneFlag"));

    tri = RpGeometryGetTriangles(geomStat->geom);

    skinBoneIndex = RpSkinGetVertexBoneIndices(geomStat->skin);
    skinBoneWeight = RpSkinGetVertexBoneWeights(geomStat->skin);

    /* Clear the flag */
    _rtSkinSplitBoneFlagClear(&matTriList->boneFlag, 0);
    matTriList->numBones = 0;
    matTriList->boneRoot = 0xFFFFFFFF;

    for (i = 0; i < matTriList->numTris; i++)
    {
        triIdx = matTriList->triList[i];

        /* Convert the vertices's bone mtx into a flag */
        for (j = 0; j < 3; j++)
        {
            mtxIdx = skinBoneIndex[tri[triIdx].vertIndex[j]];

            if (skinBoneWeight[tri[triIdx].vertIndex[j]].w0 > 0)
            {
                idx = mtxIdx & 0xFF;

                matTriList->boneRoot = (idx < matTriList->boneRoot) ?
                        idx : matTriList->boneRoot;

                if (!_rtSkinSplitBoneFlagGetIdx(&matTriList->boneFlag, idx))
                    matTriList->numBones++;

                _rtSkinSplitBoneFlagSetIdx(&matTriList->boneFlag, idx);
            }
            if (skinBoneWeight[tri[triIdx].vertIndex[j]].w1 > 0)
            {
                mtxIdx >>= 8;
                idx = mtxIdx & 0xFF;

                matTriList->boneRoot = (idx < matTriList->boneRoot) ?
                    idx : matTriList->boneRoot;

                if (!_rtSkinSplitBoneFlagGetIdx(&matTriList->boneFlag, idx))
                    matTriList->numBones++;

                _rtSkinSplitBoneFlagSetIdx(&matTriList->boneFlag, idx);
            }
            if (skinBoneWeight[tri[triIdx].vertIndex[j]].w2 > 0)
            {
                mtxIdx >>= 8;
                idx = mtxIdx & 0xFF;

                matTriList->boneRoot = (idx < matTriList->boneRoot) ?
                    idx : matTriList->boneRoot;

                if (!_rtSkinSplitBoneFlagGetIdx(&matTriList->boneFlag, idx))
                    matTriList->numBones++;

                _rtSkinSplitBoneFlagSetIdx(&matTriList->boneFlag, idx);
            }
            if (skinBoneWeight[tri[triIdx].vertIndex[j]].w3 > 0)
            {
                mtxIdx >>= 8;
                idx = mtxIdx & 0xFF;

                matTriList->boneRoot = (idx < matTriList->boneRoot) ?
                    idx : matTriList->boneRoot;

                if (!_rtSkinSplitBoneFlagGetIdx(&matTriList->boneFlag, idx))
                    matTriList->numBones++;

                _rtSkinSplitBoneFlagSetIdx(&matTriList->boneFlag, idx);
            }
        }
    }

    RWRETURN (matTriList);
}


/****************************************************************************
 rtSkinSplitMatTriListSplit splits the input the matTriList into two. The
 input list is modified and a new list is created.

 The triangles are split according to the bone matrix flag. Any triangle that
 uses any bones in the bone flag is removed from the old list and added to new.

 Triangles that do not use any bones in the bone matrix flag remains in the
 input list.

 */
rtSkinSplitMatTriList *
_rtSkinSplitMatTriListSplit( rtSkinSplitGeomStat *geomStat,
                            rtSkinSplitMatTriList *matTriList,
                            RwUInt32 boneLimit)
{
    RwUInt32                i, triCount1, triCount2;
    RwInt32                 boneSplit;
    rtSkinSplitBoneFlag     boneSplitFlag, boneExcludeFlag;
    RpMaterial              *matNew;
    RpTriangle              *tri;

    rtSkinSplitMatTriList   *matTriListNew;

    RWFUNCTION(RWSTRING("_rtSkinSplitMatTriListSplit"));

    tri = RpGeometryGetTriangles(geomStat->geom);

    /* Split the original list into two. The originally list will have
     * triangles removed and added to the sub list.
     */
    _rtSkinSplitBoneFlagClear(&boneSplitFlag, 0);
    _rtSkinSplitBoneFlagClear(&boneExcludeFlag, 0);

    boneSplit = -1;
    triCount1 = 0;

    while (boneLimit > 0)
    {
        /* Find the 'bone' to split the mesh */
        boneSplit = _rtSkinSplitMeshSplitSimple(
            geomStat, matTriList, boneLimit, &boneExcludeFlag );

        if (boneSplit < 0)
        {
            /* Something is wrong. */
            break;
        }

        /* Get the child hierarchy flag */
        _rtSkinSplitBoneFlagCopy(&boneSplitFlag,
            &geomStat->boneBoneFlag[boneSplit]);

        /* Count how many triangles will be moved from the old list to the
        * new one
        */
        triCount1 = 0;
        for (i = 0; i < matTriList->numTris; i++)
        {
            /* Decide which list to put the triangle */
            if (_rtSkinSplitTriClassify(geomStat,
                    &tri[matTriList->triList[i]], &boneSplitFlag))
            {
                triCount1++;
            }
        }

        /*
         * Check if the mesh can be split using this bone, if not then need
         * to find a new bone.
         */
        if ((triCount1 == 0) || (triCount1 == matTriList->numTris))
        {
            /* Can't use this bone. Find a new one by halfing the bone limit. */
            /* boneLimit >>= 1; */

            /* Find a new bone */
            _rtSkinSplitBoneFlagSetIdx(&boneExcludeFlag, boneSplit);
        }
        else
        {
            /* This bone is good, so split with this bone. */
            break;
        }
    }

    /* Something is seriously wrong. Failed to find any bone to split the mesh.
     * Just exit with error.
     */
    if ((boneSplit < 0) || (boneLimit == 0))
        RWRETURN(NULL);

    /*
     * Clone the material, RW uses material to seperate meshes, so
     * we new a material for the new mesh.
     */
    matNew = RpMaterialClone(matTriList->mat);
    if (matNew == NULL)
        RWRETURN (NULL);

    /* Create the new mat trilist */
    matTriListNew = _rtSkinSplitMatTriListCreate(triCount1);
    if (matTriListNew == NULL)
        RWRETURN (NULL);

    matTriListNew->mat = matNew;
    matTriListNew->matIdx = _rpMaterialListFindMaterialIndex(
        (const RpMaterialList *) &geomStat->geom->matList,
        (const RpMaterial *) matNew);
    matTriListNew->origMatIdx = matTriList->origMatIdx;

    geomStat->numMatTriLists++;

    triCount1 = 0;
    triCount2 = 0;
    for (i = 0; i < matTriList->numTris; i++)
    {
        /* Decide which list to put the triangle */
        if (_rtSkinSplitTriClassify(geomStat,
                &tri[matTriList->triList[i]], &boneSplitFlag))
        {
            /* New list */
            matTriListNew->triList[triCount1] = matTriList->triList[i];
            triCount1++;
        }
        else
        {
            /* Old list */
            matTriList->triList[triCount2] = matTriList->triList[i];
            triCount2++;
        }
    }

    /* Duplicate shared verts for the new sub list ? */

    /* Update the tri count in the old list */
    RWASSERT(triCount1 > 0);
    RWASSERT(triCount2 > 0);

    matTriList->numTris = triCount2;
    matTriListNew->numTris = triCount1;

    /* Update the bone list */
    _rtSkinSplitMatTriListGetBoneFlag(geomStat, matTriList);
    _rtSkinSplitMatTriListGetBoneFlag(geomStat, matTriListNew);

    RWRETURN (matTriListNew);
}

/****************************************************************************
 _rtSkinSplitHierarchyGetChildList creates a flag to indentify all the child
 bones in a hierarchy.

 */
rtSkinSplitGeomStat *
_rtSkinSplitHierarchyGetChildFlag( rtSkinSplitGeomStat *geomStat,
                                   RpHAnimHierarchy *hierarchy,
                                   RwUInt32 currBone,
                                   RwUInt32 *subHierarchyCount,
                                   rtSkinSplitBoneFlag *subHierarchyFlag )
{
    RwUInt32                stackIndex, boneCount, *boneStack;

    RWFUNCTION(RWSTRING("_rtSkinSplitHierarchyGetChildFlag"));

    /* Create a flag identifying all the sub bones from the
     * current bone index.
     */

    _rtSkinSplitBoneFlagClear(subHierarchyFlag, 0);

    boneStack = &rtSkinSplitGlobals.boneStack[0];
    stackIndex = 0;
    boneCount = 0;
    do
    {
        /* Add the bone to stack */
        boneStack[stackIndex] = (RwUInt8) currBone;

        /* Count and set flag */
        _rtSkinSplitBoneFlagSetIdx(subHierarchyFlag, currBone);
        boneCount++;

        /* Is this a leaf node ? */
        if (hierarchy->pNodeInfo[currBone].flags &
            rpHANIMPOPPARENTMATRIX)
        {
            /* Work our way back to the top of this branch */
            while (!(hierarchy->pNodeInfo[boneStack[stackIndex]].flags &
                    rpHANIMPUSHPARENTMATRIX) && (stackIndex > 0))
            {
                stackIndex--;
            }

            /* Remove the parent of this branch */
            stackIndex--;
        }

        /* Move to the next bone in the array */
        currBone++;
        stackIndex++;

    } while (stackIndex > 0);

    /* Return the count */
    *subHierarchyCount = (RwUInt32) boneCount;

    RWRETURN(geomStat);
}

/****************************************************************************
 rtSkinSplitGeomGetBoneLockFlag builds a lock flag for shared bones. These
 bones cannot have their index altered to prevent cracking on PSX2.

 */
rtSkinSplitGeomStat *
_rtSkinSplitMatTriListGetBoneLockFlag( rtSkinSplitGeomStat *geomStat,
                                      rtSkinSplitMatTriList *matTriList )
{
    rtSkinSplitBoneFlag         boneFlag;
    rtSkinSplitMatTriList       *matTriListA, *matTriListB;

    RWFUNCTION(RWSTRING("_rtSkinSplitMatTriListGetBoneLockFlag"));

    matTriListA = matTriList;

    while (matTriListA != NULL)
    {
        matTriListB = matTriListA->next;
        while (matTriListB != NULL)
        {
            _rtSkinSplitBoneFlagAnd(
                &boneFlag,
                &matTriListA->boneFlag,
                &matTriListB->boneFlag);

            _rtSkinSplitBoneFlagOr(
                &matTriListA->boneLockFlag,
                &matTriListA->boneLockFlag,
                &boneFlag);

            _rtSkinSplitBoneFlagOr(
                &matTriListB->boneLockFlag,
                &matTriListB->boneLockFlag,
                &boneFlag);

            matTriListB = matTriListB->next;
        }

        matTriListA = matTriListA->next;
    }

    RWRETURN (geomStat);
}

/****************************************************************************
 rtSkinSplitGeomRemapBoneIndex goes through and remap the bone indices for
 each matTriList.

 On entry   :
 On exit    :
 */

rtSkinSplitGeomStat *
_rtSkinSplitMatTriListRemapBoneIndex( rtSkinSplitGeomStat *geomStat,
                                     rtSkinSplitMatTriList *matTriListHead )
{
    RwUInt32                i, j, k, l, size;
    rtSkinSplitBoneFlag     boneLockFlag, boneFreeFlag;
    rtSkinSplitMatTriList   *matTriList;

    RWFUNCTION(RWSTRING("_rtSkinSplitMatTriListRemapBoneIndex"));

    /* Create the remap index array */

    _rtSkinSplitBoneFlagClear(&boneLockFlag, 0);

    /* Count how many list there are. */
    i = 0;
    matTriList = matTriListHead;
    while (matTriList != NULL)
    {
        /* Combine all the lock flag */
        _rtSkinSplitBoneFlagOr(&boneLockFlag, &boneLockFlag,
            &matTriList->boneLockFlag);

        /* Set the free flag */
        _rtSkinSplitBoneFlagClear(&matTriList->boneFreeFlag, 0xFFFFFFFF);

        /* Count the number of matTriLists */
        i++;
        matTriList = matTriList->next;
    }


    size =  /* Each list gets a remap array */
            (geomStat->numMatTriLists *
                geomStat->numBones * sizeof(RwUInt32)) +
            /* Each list gets a rle map */
            (geomStat->numMatTriLists * 2 *
                geomStat->numBones * sizeof(RwUInt32)) +
            /* Combined remap array */
            (geomStat->numBones * sizeof(RwUInt32));

    /* At present the maximum total num bones is 256 due to the use of
     * RwUInt8 for bone indices.
     */
    geomStat->boneRemapIndex = (RwUInt32 *)
        RwMalloc(size, rwID_SKINSPLITTOOKIT | rwMEMHINTDUR_EVENT);

    /* Mem failure */
    if (geomStat->boneRemapIndex == NULL)
        RWRETURN (NULL);

    /* Clear the map */
    memset(geomStat->boneRemapIndex, 0xFF, size);

    /* rleMap */
    geomStat->rleMap =
        &geomStat->boneRemapIndex
            [geomStat->numMatTriLists * geomStat->numBones];

    i = 0;
    matTriList = matTriListHead;
    while (matTriList != NULL)
    {
        /* Setup the remapindex pointer for this matTri list */
        matTriList->boneRemapIndex =
            &geomStat->boneRemapIndex
                [(i + 1) * geomStat->numBones];

        /* Setup the rle map pointer for this matTri list */
        matTriList->rleMap =
            &geomStat->rleMap
                [(i + 1) * geomStat->numBones];

        i++;
        matTriList = matTriList->next;
    }


    /* Generate new indices for shared bones first */
    for (i = 0; i < geomStat->numBones; i++)
    {
        /* Is this a locked bone ? */
        if (_rtSkinSplitBoneFlagGetIdx(&boneLockFlag, i))
        {
            /* Find the lowest unused common index in all
             * matTriList using this bone.
             */
            _rtSkinSplitBoneFlagClear(&boneFreeFlag, 0xFFFFFFFF);

            /* Combine the free flag for all list that uses this bone */
            matTriList = matTriListHead;
            while (matTriList != NULL)
            {
                /* Does this list use this bone ? */
                if (_rtSkinSplitBoneFlagGetIdx(&matTriList->boneFlag, i))
                {
                    /* Combine the free flag */
                    _rtSkinSplitBoneFlagAnd(&boneFreeFlag, &boneFreeFlag,
                        &matTriList->boneFreeFlag);
                }

                matTriList = matTriList->next;
            }

            /* Find the lowest index in the combined free flag */
            j = 0;
            while (!(_rtSkinSplitBoneFlagGetIdx(&boneFreeFlag, j)))
            {
                j++;
            }

            /* Lowest index is greater than boneLimit. Something is wrong so
             * exit.
             */
            if (j >= geomStat->boneLimit)
            {
                RWRETURN(NULL);
            }

            /* Now set the bone idx in the meshes,
             * updating their free flag.
             */
            matTriList = matTriListHead;
            while (matTriList != NULL)
            {
                /* Does this list use this bone ? */
                if (_rtSkinSplitBoneFlagGetIdx(&matTriList->boneLockFlag, i))
                {
                    /* Set the index and unset the free flag */
                    matTriList->boneRemapIndex[i] = (RwUInt32) j;

                    _rtSkinSplitBoneFlagUnsetIdx(&matTriList->boneFreeFlag, j);
                }

                matTriList = matTriList->next;
            }
        }
    }

    /* Generate new indices for non-shared bones */
    matTriList = matTriListHead;
    while (matTriList != NULL)
    {
        k = 0;
        for (i = 0; i < geomStat->numBones; i++)
        {
            /* Is this bone used ? */
            if (_rtSkinSplitBoneFlagGetIdx(&matTriList->boneFlag, i))
            {
                /* Is this bone locked ? */
                if (!(_rtSkinSplitBoneFlagGetIdx(&matTriList->boneLockFlag, i)))
                {
                    /* Is this index free ? */
                    while (!(_rtSkinSplitBoneFlagGetIdx(
                            &matTriList->boneFreeFlag, k)))
                    {
                        k++;
                    }

                    matTriList->boneRemapIndex[i] = (RwUInt32) k;
                    _rtSkinSplitBoneFlagUnsetIdx(&matTriList->boneFreeFlag, k);

                    k++;
                }
            }
        }

        matTriList = matTriList->next;
    }

    /* Double check the combine array to ensure no overlaps */
    matTriList = matTriListHead;
    while (matTriList != NULL)
    {
        for (i = 0; i < geomStat->numBones; i++)
        {
            k = matTriList->boneRemapIndex[i];

            if (k != 0xFFFFFFFF)
            {
                if (geomStat->boneRemapIndex[i] == 0xFFFFFFFF)
                {
                    geomStat->boneRemapIndex[i] = k;
                }
                else if (geomStat->boneRemapIndex[i] != k)
                {
                    /* Something is wrong. We have an overlap. */
                    RWASSERT(FALSE);
                }
            }
        }

        matTriList = matTriList->next;
    }

    /* Work out the rle batches. */
    matTriList = matTriListHead;
    while (matTriList != NULL)
    {
        j = 0; /* rle start */
        k = 0; /* rle length */
        l = 0; /* Counts number of rle for this matTri list */
        for (i = 0; i < geomStat->numBones; i++)
        {
            /* Is this bone used ? */
            if (_rtSkinSplitBoneFlagGetIdx(&matTriList->boneFlag, i))
            {
                /* Continue the rle */
                k++;
            }
            else
            {
                /* Save out the previous rle ? */
                if (k > 0)
                {
                    matTriList->rleMap[(2 * l)] = j;
                    matTriList->rleMap[(2 * l) + 1] = k;
                    l++;
                }

                /* Start a new rle */
                j = i + 1;
                k = 0;
            }
        }

        /* Save out the last sequence */
        if (k > 0)
        {
            matTriList->rleMap[(2 * l)] = j;
            matTriList->rleMap[(2 * l) + 1] = k;
            l++;
        }


        /* Save the rle count */
        matTriList->numRLE = l;

        matTriList = matTriList->next;

    }

    RWRETURN (geomStat);
}

/****************************************************************************
 rtSkinSplitGeomGetStat collects some information on the mesh/bone sharing in
 geom.

 */
rtSkinSplitGeomStat *
_rtSkinSplitGeomGetStat( rtSkinSplitGeomStat *geomStat )
{
    RpGeometry                  *geom;
    RpTriangle                  *tri;

    RpHAnimHierarchy            *hierarchy;
    RpSkin                      *skin;
    const RwMatrixWeights       *skinBoneWeight, *skinMtxWeight;
    const RwUInt32              *skinBoneIndex;

    RwUInt32                    i, j, k, idx,
                                skinMtxIndex, meshBoneIndexStart, numBones,
                                maxTriBones;

    RwUInt32                    triBoneIdx[12];

    RWFUNCTION(RWSTRING("_rtSkinSplitGeomGetStat"));

    geom = geomStat->geom;
    skin = geomStat->skin;
    hierarchy = geomStat->hierarchy;

    numBones = RpSkinGetNumBones(skin);

    skinBoneIndex = geomStat->skinBoneIndex;
    skinBoneWeight = geomStat->skinBoneWeight;

    geomStat->numWeights = 0;
    geomStat->maxTriBones = 0;

    /* Go through the triangle list and perform various counts */
    tri = RpGeometryGetTriangles(geom);
    for (i = 0; i < geomStat->numTris; i++)
    {
        meshBoneIndexStart = tri->matIndex * numBones;

        /* Count the number tris in a mesh */
        geomStat->meshTriCount[tri->matIndex]++;

        maxTriBones = 0;
        memset(triBoneIdx, 0, 12 * sizeof(RwInt32));

        for (j = 0; j < 3; j++)
        {
            /* Perform the following counts for the three verts and four
             * weights,
             *
             * A vertex is used by the bone per mesh.
             * Number of bones per mesh.
             * Number of mesh per bone.
             */
            skinMtxIndex = skinBoneIndex[tri->vertIndex[j]];
            skinMtxWeight = &skinBoneWeight[tri->vertIndex[j]];

            /* Check if w0 is used */
            if (skinMtxWeight->w0 > (RwReal) 0.0)
            {
                idx = skinMtxIndex & 0xFF;
                geomStat->boneVertCount[meshBoneIndexStart + idx]++;

                /* First time seen this bone, so increment the total bone
                 * count for the mesh
                 */
                if (geomStat->boneVertCount[meshBoneIndexStart + idx] == 1)
                {
                    geomStat->meshBoneCount[tri->matIndex]++;
                    geomStat->boneMeshCount[idx]++;
                }

                /* Count the max number of weights */
                geomStat->numWeights |= 0x01;

                /* Check if this bone is already seen by this tri */
                for (k = 0; k < maxTriBones; k++)
                {
                    if (triBoneIdx[k] == idx)
                        break;
                }

                if (k == maxTriBones)
                {
                    triBoneIdx[k] = idx;
                    maxTriBones++;
                }
            }

            /* Check if w1 is used */
            if (skinMtxWeight->w1 > (RwReal) 0.0)
            {
                skinMtxIndex >>= 8;
                idx = skinMtxIndex & 0xFF;
                geomStat->boneVertCount[meshBoneIndexStart + idx]++;

                /* First time seen this bone, so increment the total bone
                 * count for the mesh
                 */
                if (geomStat->boneVertCount[meshBoneIndexStart + idx] == 1)
                {
                    geomStat->meshBoneCount[tri->matIndex]++;
                    geomStat->boneMeshCount[idx]++;
                }

                /* Count the max number of weights */
                geomStat->numWeights |= 0x02;

                /* Check if this bone is already seen by this tri */
                for (k = 0; k < maxTriBones; k++)
                {
                    if (triBoneIdx[k] == idx)
                        break;
                }

                if (k == maxTriBones)
                {
                    triBoneIdx[k] = idx;
                    maxTriBones++;
                }
            }

            /* Check if w2 is used */
            if (skinMtxWeight->w2 > (RwReal) 0.0)
            {
                skinMtxIndex >>= 8;
                idx = skinMtxIndex & 0xFF;
                geomStat->boneVertCount[meshBoneIndexStart + idx]++;

                /* First time seen this bone, so increment the total bone
                 * count for the mesh
                 */
                if (geomStat->boneVertCount[meshBoneIndexStart + idx] == 1)
                {
                    geomStat->meshBoneCount[tri->matIndex]++;
                    geomStat->boneMeshCount[idx]++;
                }

                /* Count the max number of weights */
                geomStat->numWeights |= 0x04;

                /* Check if this bone is already seen by this tri */
                for (k = 0; k < maxTriBones; k++)
                {
                    if (triBoneIdx[k] == idx)
                        break;
                }

                if (k == maxTriBones)
                {
                    triBoneIdx[k] = idx;
                    maxTriBones++;
                }
            }

            /* Check if w3 is used */
            if (skinMtxWeight->w3 > (RwReal) 0.0)
            {
                skinMtxIndex >>= 8;
                idx = skinMtxIndex & 0xFF;
                geomStat->boneVertCount[meshBoneIndexStart + idx]++;

                /* First time seen this bone, so increment the total bone
                 * count for the mesh
                 */
                if (geomStat->boneVertCount[meshBoneIndexStart + idx] == 1)
                {
                    geomStat->meshBoneCount[tri->matIndex]++;
                    geomStat->boneMeshCount[idx]++;
                }

                /* Count the max number of weights */
                geomStat->numWeights |= 0x08;

                /* Check if this bone is already seen by this tri */
                for (k = 0; k < maxTriBones; k++)
                {
                    if (triBoneIdx[k] == idx)
                        break;
                }

                if (k == maxTriBones)
                {
                    triBoneIdx[k] = idx;
                    maxTriBones++;
                }
            }
        }

        /* Record the maximum number of bone used by a triangle. */
        if (maxTriBones > geomStat->maxTriBones)
            geomStat->maxTriBones = maxTriBones;

        tri++;
    }

    if (geomStat->numWeights & 0x08)
    {
        geomStat->numWeights = 4;
    }
    else if (geomStat->numWeights & 0x04)
    {
        geomStat->numWeights = 3;
    }
    else if (geomStat->numWeights & 0x02)
    {
        geomStat->numWeights = 2;
    }
    else if (geomStat->numWeights & 0x01)
    {
        geomStat->numWeights = 1;
    }
    else
    {
        /* Something is wrong. No weights ? */
        RWASSERT((geomStat->numWeights != 0));
    }

    /* Go through the bone hierarchy and perform various counts */
    for (i = 0; i < geomStat->numBones; i++)
    {
        /* Count the number of children of this bone and set the
         * child hierarchy flag
         */
        _rtSkinSplitHierarchyGetChildFlag(
            geomStat, hierarchy, i,
            &geomStat->boneBoneCount[i],
            &geomStat->boneBoneFlag[i] );
    }

    RWRETURN(geomStat);
}

/****************************************************************************
 rtSkinSplitMeshSplitSimple splits the mesh into two meshes. The split is
 done by prunning away hierarchies with less than the bone limit from the
 main hierachy until it too contain less than the bone limit.

 */
RwInt32
_rtSkinSplitMeshSplitSimple( rtSkinSplitGeomStat *geomStat,
                             rtSkinSplitMatTriList *matTriList,
                             RwUInt32 boneLimit,
                             rtSkinSplitBoneFlag *boneExcludeFlag )
{
    RpHAnimHierarchy            *hierarchy;
    RwUInt32                    stackIndex, currBone, *boneStack, boneCount;
    RwInt32                     boneIdx;
    rtSkinSplitBoneFlag         boneFlag;

    RWFUNCTION(RWSTRING("_rtSkinSplitMeshSplitSimple"));

    /* Traverse the hierarchy until reached a bone with less than specified limit
     * and split the mesh at that point */

    hierarchy = geomStat->hierarchy;

    boneStack = &rtSkinSplitGlobals.boneStack[0];
    stackIndex = 0;
    boneIdx = 0xFFFFFFFF;
    currBone = 0;
    do
    {
        /* Add the bone to stack */
        boneStack[stackIndex] = (RwUInt8) currBone;

        /* Have reached the start of the mesh's first bone ? */
        if (currBone >= matTriList->boneRoot)
        {
            /* Should this bone be considered ? */
            if (!_rtSkinSplitBoneFlagGetIdx(boneExcludeFlag, currBone))
            {
                _rtSkinSplitBoneFlagAnd(&boneFlag,
                        &matTriList->boneFlag,
                        &geomStat->boneBoneFlag[currBone]);

                /* Is this sub hierarchy used ? */
                if (!_rtSkinSplitBoneFlagEmpty(&boneFlag))
                {
                    /* Does this branch have less than the limit ? */
                    if (geomStat->boneBoneCount[currBone] <= boneLimit)
                    {
                        boneIdx = (RwInt32) currBone;

                        break;
                    }
                    else
                    {
                        /* Is the used bones is less than the limit ? */
                        _rtSkinSplitBoneFlagBitCount(&boneCount, &boneFlag);

                        if (boneCount <= boneLimit)
                        {
                            boneIdx = (RwInt32) currBone;

                            break;
                        }
                    }
                }
            }
        }

        /* Is this a leaf node ? */
        if (hierarchy->pNodeInfo[currBone].flags &
            rpHANIMPOPPARENTMATRIX)
        {
            /* Work our way back to the top of this branch */
            while (!(hierarchy->pNodeInfo[boneStack[stackIndex]].flags &
                    rpHANIMPUSHPARENTMATRIX) && (stackIndex > 0))
            {
                stackIndex--;
            }

            /* Remove the parent of this branch */
            stackIndex--;
        }

        /* Move to the next bone in the array */
        currBone++;
        stackIndex++;

    } while (stackIndex > 0);

    RWRETURN(boneIdx);
}

