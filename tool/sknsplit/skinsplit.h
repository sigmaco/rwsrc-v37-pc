/****************************************************************************
 *                                                                          *
 * Module       : skinsplit.h                                               *
 *                                                                          *
 * Purpose      : Split mesh into smaller meshes with smaller bones         *
 *                                                                          *
 ***************************************************************************/

#ifndef SKINSPLIT_H
#define SKINSPLIT_H

/****************************************************************************
 Includes
 */

#include <rwcore.h>
#include <rpworld.h>
#include <rpskin.h>

/****************************************************************************
 Defines
 */

#define rtSKINSPLITMAXBONESTACK    256

#define _rtSkinSplitBoneFlagSetIdx(_flag, _id) \
    ((_flag)->flag[(_id) >> 5] |= (1 << ((_id) & 31)))

#define _rtSkinSplitBoneFlagGetIdx(_flag, _id) \
    ((_flag)->flag[(_id) >> 5] & (1 << ((_id) & 31)))

#define _rtSkinSplitBoneFlagUnsetIdx(_flag, _id) \
    ((_flag)->flag[(_id) >> 5] &= ~(1 << ((_id) & 31)))

#define _rtSkinSplitBoneFlagClear(_flag, _clr) \
MACRO_START \
{ \
    (_flag)->flag[0] = (_clr); \
    (_flag)->flag[1] = (_clr); \
    (_flag)->flag[2] = (_clr); \
    (_flag)->flag[3] = (_clr); \
    (_flag)->flag[4] = (_clr); \
    (_flag)->flag[5] = (_clr); \
    (_flag)->flag[6] = (_clr); \
    (_flag)->flag[7] = (_clr); \
} \
MACRO_STOP

#define _rtSkinSplitBoneFlagCopy(_dst, _src) \
MACRO_START \
{ \
    (_dst)->flag[0] = (_src)->flag[0]; \
    (_dst)->flag[1] = (_src)->flag[1]; \
    (_dst)->flag[2] = (_src)->flag[2]; \
    (_dst)->flag[3] = (_src)->flag[3]; \
    (_dst)->flag[4] = (_src)->flag[4]; \
    (_dst)->flag[5] = (_src)->flag[5]; \
    (_dst)->flag[6] = (_src)->flag[6]; \
    (_dst)->flag[7] = (_src)->flag[7]; \
} \
MACRO_STOP

#define _rtSkinSplitBoneFlagOr(_dst, _src1, _src2) \
MACRO_START \
{ \
    (_dst)->flag[0] = (_src1)->flag[0] | (_src2)->flag[0]; \
    (_dst)->flag[1] = (_src1)->flag[1] | (_src2)->flag[1]; \
    (_dst)->flag[2] = (_src1)->flag[2] | (_src2)->flag[2]; \
    (_dst)->flag[3] = (_src1)->flag[3] | (_src2)->flag[3]; \
    (_dst)->flag[4] = (_src1)->flag[4] | (_src2)->flag[4]; \
    (_dst)->flag[5] = (_src1)->flag[5] | (_src2)->flag[5]; \
    (_dst)->flag[6] = (_src1)->flag[6] | (_src2)->flag[6]; \
    (_dst)->flag[7] = (_src1)->flag[7] | (_src2)->flag[7]; \
} \
MACRO_STOP

#define _rtSkinSplitBoneFlagAnd(_dst, _src1, _src2) \
MACRO_START \
{ \
    (_dst)->flag[0] = (_src1)->flag[0] & (_src2)->flag[0]; \
    (_dst)->flag[1] = (_src1)->flag[1] & (_src2)->flag[1]; \
    (_dst)->flag[2] = (_src1)->flag[2] & (_src2)->flag[2]; \
    (_dst)->flag[3] = (_src1)->flag[3] & (_src2)->flag[3]; \
    (_dst)->flag[4] = (_src1)->flag[4] & (_src2)->flag[4]; \
    (_dst)->flag[5] = (_src1)->flag[5] & (_src2)->flag[5]; \
    (_dst)->flag[6] = (_src1)->flag[6] & (_src2)->flag[6]; \
    (_dst)->flag[7] = (_src1)->flag[7] & (_src2)->flag[7]; \
} \
MACRO_STOP

#define _rtSkinSplitBoneFlagEmpty(_flag) \
    (((_flag)->flag[0] == 0) && \
     ((_flag)->flag[1] == 0) && \
     ((_flag)->flag[2] == 0) && \
     ((_flag)->flag[3] == 0) && \
     ((_flag)->flag[4] == 0) && \
     ((_flag)->flag[5] == 0) && \
     ((_flag)->flag[6] == 0) && \
     ((_flag)->flag[7] == 0))

#define _rtSkinSplitBoneFlagBitCount(_count, _flag) \
MACRO_START \
{ \
    RwUInt32 _i; \
    *(_count) = 0; \
    for (_i = 0; _i < 8; _i++) \
    { \
        *(_count) += \
            ((_rtSkinSplitBitTable[((_flag)->flag[_i] & 0x0000000F)      ]) + \
             (_rtSkinSplitBitTable[((_flag)->flag[_i] & 0x000000F0) >>  4]) + \
             (_rtSkinSplitBitTable[((_flag)->flag[_i] & 0x00000F00) >>  8]) + \
             (_rtSkinSplitBitTable[((_flag)->flag[_i] & 0x0000F000) >> 12]) + \
             (_rtSkinSplitBitTable[((_flag)->flag[_i] & 0x000F0000) >> 16]) + \
             (_rtSkinSplitBitTable[((_flag)->flag[_i] & 0x00F00000) >> 20]) + \
             (_rtSkinSplitBitTable[((_flag)->flag[_i] & 0x0F000000) >> 24]) + \
             (_rtSkinSplitBitTable[((_flag)->flag[_i] & 0xF0000000) >> 28])); \
    } \
} \
MACRO_STOP

/****************************************************************************
 Global Types
 */
typedef struct rtSkinSplitBoneFlag rtSkinSplitBoneFlag;

struct rtSkinSplitBoneFlag
{
    RwUInt32            flag[8];
};


typedef struct rtSkinSplitMatTriList rtSkinSplitMatTriList;

struct rtSkinSplitMatTriList
{
    rtSkinSplitMatTriList       *next;

    RwUInt32            numBones;
    RwUInt32            boneRoot;

    rtSkinSplitBoneFlag     boneFlag;
    rtSkinSplitBoneFlag     boneLockFlag;
    rtSkinSplitBoneFlag     boneFreeFlag;

    RwUInt32            *boneRemapIndex;

    RwUInt32            numRLE;
    RwUInt32            *rleMap;

    RpMaterial          *mat;
    RwInt32             matIdx;
    RwInt32             origMatIdx;

    RwUInt32            maxTris;
    RwUInt32            numTris;
    RwUInt32            *triList;
};

typedef struct rtSkinSplitGeomStat rtSkinSplitGeomStat;

struct rtSkinSplitGeomStat
{
    RwUInt32                    flag;

    RpAtomic                    *atomic;
    RpGeometry                  *geom;
    RpMaterialList              *matList;
    RpSkin                      *skin;
    RpHAnimHierarchy            *hierarchy;
    const RwMatrixWeights       *skinBoneWeight;
    const RwUInt32              *skinBoneIndex;
    RwUInt32                    *boneRemapIndex;
    RwUInt32                    *rleMap;

    RwUInt32            boneLimit;
    RwUInt32            maxTriBones;

    RwUInt32            numTris;
    RwUInt32            numMats;
    RwUInt32            numVerts;
    RwUInt32            numBones;
    RwUInt32            numMatTriLists;
    RwUInt32            numWeights;

    RwUInt32                *meshBoneCount;
    RwUInt32                *boneMeshCount;
    RwUInt32                *boneVertCount;
    RwUInt32                *meshTriCount;
    RwUInt32                *boneBoneCount;
    rtSkinSplitBoneFlag     *boneBoneFlag;

    rtSkinSplitMatTriList  *matTriListStack;
    rtSkinSplitMatTriList  *matTriListDone;

    RwUInt8                 *skinBoneRemapIndices;
    RwUInt8                 *skinMeshBoneRLECount;
    RwUInt8                 *skinMeshBoneRLE;
};

typedef struct rtSkinSplitGlobalVars rtSkinSplitGlobalVars;

struct rtSkinSplitGlobalVars
{
    RwUInt32            boneStack[rtSKINSPLITMAXBONESTACK];
};


/****************************************************************************
 Global variables (across program)
 */

extern __declspec( thread ) rtSkinSplitGlobalVars rtSkinSplitGlobals;

extern RwUInt8 _rtSkinSplitBitTable[16];

/****************************************************************************
 Function prototypes
 */
#ifdef __cplusplus
extern "C"
{
#endif /* __cpluscplus */

extern RwBool
_rtSkinSplitTriClassify( rtSkinSplitGeomStat *geomStat,
                        RpTriangle *tri,
                        rtSkinSplitBoneFlag *boneFlag );

extern rtSkinSplitMatTriList *
_rtSkinSplitMatTriListPush( rtSkinSplitMatTriList *triListHead,
                           rtSkinSplitMatTriList *triList );

extern rtSkinSplitMatTriList *
_rtSkinSplitMatTriListPop( rtSkinSplitMatTriList *triListHead,
                          rtSkinSplitMatTriList **triListPtr );

extern rtSkinSplitMatTriList *
_rtSkinSplitMatTriListCreate( RwUInt32 count );

extern RwBool
_rtSkinSplitMatTriListDestroy( rtSkinSplitMatTriList *matTriList );

extern rtSkinSplitMatTriList *
_rtSkinSplitMatTriListAddTriByMat( rtSkinSplitGeomStat *geomStat,
                                  rtSkinSplitMatTriList *matTriList,
                                  RpMaterial *mat,
                                  RwUInt32 matIdx );

extern rtSkinSplitMatTriList *
_rtSkinSplitMatTriListGetBoneFlag( rtSkinSplitGeomStat *geomStat,
                                  rtSkinSplitMatTriList *matTriList );

extern rtSkinSplitMatTriList *
_rtSkinSplitMatTriListSplit( rtSkinSplitGeomStat *geomStat,
                            rtSkinSplitMatTriList *matTriList,
                            RwUInt32 boneLimit);

extern rtSkinSplitGeomStat *
_rtSkinSplitHierarchyGetChildFlag( rtSkinSplitGeomStat *geomStat,
                                   RpHAnimHierarchy *hierarchy,
                                   RwUInt32 currBone,
                                   RwUInt32 *subHierarchyCount,
                                   rtSkinSplitBoneFlag *subHierarchyFlag );

extern rtSkinSplitGeomStat *
_rtSkinSplitMatTriListGetBoneLockFlag( rtSkinSplitGeomStat *geomStat,
                                      rtSkinSplitMatTriList *matTriList );

extern rtSkinSplitGeomStat *
_rtSkinSplitMatTriListRemapBoneIndex( rtSkinSplitGeomStat *geomStat,
                                     rtSkinSplitMatTriList *matTriListHead );

extern rtSkinSplitGeomStat *
_rtSkinSplitGeomGetStat( rtSkinSplitGeomStat *geomStat );


extern RwInt32
_rtSkinSplitMeshSplitSimple( rtSkinSplitGeomStat *geomStat,
                             rtSkinSplitMatTriList *matTriList,
                             RwUInt32 boneLimit,
                             rtSkinSplitBoneFlag *boneExcludeFlag );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SKINSPLIT_H */
