/****************************************************************************
 *
 * This file is a product of Criterion Software Ltd.
 *
 * This file is provided as is with no warranties of any kind and is
 * provided without any obligation on Criterion Software Ltd.
 * or Canon Inc. to assist in its use or modification.
 *
 * Criterion Software Ltd. and Canon Inc. will not, under any
 * circumstances, be liable for any lost revenue or other damages
 * arising from the use of this file.
 *
 * Copyright (c) 2001 Criterion Software Ltd.
 * All Rights Reserved.
 *
 */

/****************************************************************************
 *
 * ptankg.h
 *
 * Copyright (C) 2001 Criterion Technologies.
 *
 *
 ****************************************************************************/
#ifndef PTANKG_H
#define PTANKG_H


#ifdef    __cplusplus
extern "C"
{
#endif /* __cplusplus */



extern RwBool
_rpPTankGenCreate(RpAtomic *atomic,
            RwInt32 maxPCount, RwUInt32 dataFlags);

extern RwBool
_rpPTankGENInstance(RpAtomic *atomic, RpPTankData *ptankGlobal,
                        RwInt32 actPCount, RwUInt32 instFlags);

extern void
_rpPTankGENInstanceSelectCB(RpAtomic *atomic);

extern void
_rpPTankGENKill(RpAtomic *atomic, RwInt32 start, RwInt32 end);


extern void
rpPTankGENInsPosNCCSNR(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);
                        /* no center, constant size, no rotation */
extern void
rpPTankGENInsPosCCCSNR(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);
                        /* center, constant size, no rotation */
extern void
rpPTankGENInsPosNCCSPR(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);
                        /* no center, constant size, per particle rotation */

extern void
rpPTankGENInsPosNCPSNR(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);
                        /* no center, per particle size, no rotation */
extern void
rpPTankGENInsPosNCPSPR(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);
                        /* no center, constant size, per particle rotation */

extern void
rpPTankGENInsPosCCCSPR(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);
                        /* constant center, constant size, per particle rotation */

extern void
rpPTankGENInsPosCCPSNR(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);
                        /* constant center, per particle size, no rotation */

extern void
rpPTankGENInsPosCCPSPR(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);
                        /* constant center, per particle size, per particle rotation */

extern void
rpPTankGENInsPosNCPM(RpPTankLockStruct *dstCluster,
                        RwV3d *_right,
                        RwV3d *_up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);


extern void
rpPTankGENInsPosCCPM(RpPTankLockStruct *dstCluster,
                        RwV3d *_right,
                        RwV3d *_up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);
                         /* constant center (use mtx pos), per particle matrix */

extern void
rpPTankGENInsColorPP(RpPTankLockStruct *dstCluster,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);
                         /* 1 Color per particle*/

extern void
rpPTankGENInsColorPV(RpPTankLockStruct *dstCluster,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);
                         /* 1 Color per vertex*/

extern void
rpPTankGENInsColorPVC(RpPTankLockStruct *dstCluster,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);
                         /* 1 constant Color per vertex*/

extern void
rpPTankGENInsColorPPC(RpPTankLockStruct *dstCluster,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);
                         /* 1 constant Color per particle*/
extern void
rpPTankGENInsUV2(RpPTankLockStruct *dstCluster,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);
                         /* 2 pair of uvs per particle*/
                
extern void
rpPTankGENInsUV4(RpPTankLockStruct *dstCluster,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);
                         /* 4 pair of uvs per particle*/

extern void
rpPTankGENInsUV2C(RpPTankLockStruct *dstCluster,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);
                         /* 2 pair of uvs per system*/
                
extern void
rpPTankGENInsUV4C(RpPTankLockStruct *dstCluster,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);
                         /* 4 pair of uvs per particle*/
                
extern void
rpPTankGENInsNormals(RpPTankLockStruct *dstCluster,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal);
                         /* one normal per particles*/



/*
 *****************************************************************************
 */
extern void
rpPTankGENInsSetup(RpPTANKInstanceSetupData *data, RpAtomic *atomic, 
            RpPTankData *ptankGlobal,  RwInt32 actPCount, RwUInt32 instFlags);

extern void
rpPTankGENInsEnding(RpPTANKInstanceSetupData *data, RpAtomic *atomic, 
            RpPTankData *ptankGlobal,  RwInt32 actPCount, RwUInt32 instFlags);

extern RwBool
_rpPTankGENRenderCallBack(RpAtomic *atomic,
                          RpPTankData *ptankGlobal,
                          RwInt32 actPCount);

#ifdef    __cplusplus
}
#endif /* __cplusplus */

#endif /* SPR3DGEN_H */


