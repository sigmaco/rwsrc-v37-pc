
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
 * ptank.c
 *
 * Copyright (C) 2001 Criterion Technologies.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rwcore.h"
#include "rpworld.h"
#include "rpplugin.h"
#include "rpdbgerr.h"
#include "ptank.h"
#include "ptankprv.h"

#include "ptankg.h"


#if (defined(D3D8_DRVMODEL_H))

#define RGBATOCOLOR(rgba)    (( (((RwUInt32)((rgba)->alpha)) << 24) |  \
                                 (((RwUInt32)((rgba)->red)) << 16) |     \
                                 (((RwUInt32)((rgba)->green)) << 8) |    \
                                 ((RwUInt32)((rgba)->blue)) ))
#define CONSTANTFLAGS   ( 0 )

#elif (defined(D3D9_DRVMODEL_H))

#define RGBATOCOLOR(rgba)    (( (((RwUInt32)((rgba)->alpha)) << 24) |  \
                                 (((RwUInt32)((rgba)->red)) << 16) |     \
                                 (((RwUInt32)((rgba)->green)) << 8) |    \
                                 ((RwUInt32)((rgba)->blue)) ))
#define CONSTANTFLAGS   ( 0 )

#elif (defined(XBOX_DRVMODEL_H))

#define RGBATOCOLOR(rgba)    (( (((RwUInt32)((rgba)->alpha)) << 24) |  \
                                 (((RwUInt32)((rgba)->red)) << 16) |     \
                                 (((RwUInt32)((rgba)->green)) << 8) |    \
                                 ((RwUInt32)((rgba)->blue)) ))
#define CONSTANTFLAGS   ( 0 )

#elif (defined(NULL_DRVMODEL_H)    || \
       defined(NULLGCN_DRVMODEL_H) || \
       defined(NULLSKY_DRVMODEL_H) || \
       defined(SKY2_DRVMODEL_H)    || \
       defined(OPENGL_DRVMODEL_H)  || \
       defined(SOFTRAS_DRVMODEL_H) || \
       defined(GCN_DRVMODEL_H)     || \
       defined(NULLXBOX_DRVMODEL_H))

#define RGBATOCOLOR(rgba)    (*(RwUInt32*)rgba)
#define CONSTANTFLAGS   ( rpPTANKDFLAGCNSMATRIX |\
                          rpPTANKDFLAGCNS2DROTATE |\
                          rpPTANKDFLAGCNSVTXCOLOR |\
                          rpPTANKDFLAGCNSVTX2TEXCOORDS |\
                          rpPTANKDFLAGCNSVTX4TEXCOORDS )

#endif



/*
 *****************************************************************************
 */


/*
 *****************************************************************************
 */
#if (defined(WIN32))

/* See also maths in
 * rwsdk/tool/ieeef/rtieeef.c
 */

static __inline void
SinCos(float angle __RWUNUSED__, float *sine __RWUNUSED__, float *cosine __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("SinCos"));

#if defined(NOASM)
    *sine = (RwReal)RwSin(angle);
    *cosine = (RwReal)RwCos(angle);
#else

#if !defined(__GNUC__)
    __asm
    {
        fld        dword ptr angle
        fsincos
        mov        eax, sine
        mov        edx, cosine
        fstp    dword ptr [edx]
        fstp    dword ptr [eax]
    }
#else /* !defined(__GNUC__) */
#error "Cannot handle inline assembly with a GCC compiler"
#endif /* !defined(__GNUC__) */

#endif

    RWRETURNVOID();
}
#else

#define SinCos(angle,sine,cosine)                                           \
MACRO_START                                                                 \
{                                                                           \
    *sine = (RwReal)RwSin(angle);                                           \
    *cosine = (RwReal)RwCos(angle);                                         \
}                                                                           \
MACRO_STOP

#endif

/*
 *****************************************************************************
 */
RwBool
_rpPTankGENInstance(RpAtomic *atomic, RpPTankData *ptankGlobal,
                    RwInt32 actPCount, RwUInt32 instFlags)
{
    RpPTANKInstanceSetupData instanceStr;
    RpPTankAtomicExtPrv *PTankPrv;
    RwInt32 instanceNum;

    RWFUNCTION(RWSTRING("_rpPTankGENInstance"));
    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));


    if( 0 != (instFlags & CONSTANTFLAGS) )
    {
        instanceNum = RpPTankAtomicGetMaximumParticlesCount(atomic);
    }
    else
    {
        instanceNum = actPCount;
    }


    PTankPrv = RPATOMICPTANKPLUGINDATA(atomic);

    if( PTankPrv->insSetupCB )
    {
        memset(&instanceStr,0,sizeof(RpPTANKInstanceSetupData));
        PTankPrv->insSetupCB(&instanceStr,
                        atomic,
                        ptankGlobal,
                        instanceNum,
                        instFlags);
    }

    if( (NULL != PTankPrv->insPosCB) && (NULL != instanceStr.positionOut.data ))
    {
        PTankPrv->insPosCB(&instanceStr.positionOut,
                            &instanceStr.right,
                            &instanceStr.up,
                            instanceNum,
                            ptankGlobal);
    }

    if( (NULL != PTankPrv->insUVCB) && (NULL != instanceStr.UVOut.data ))
    {
        PTankPrv->insUVCB(&instanceStr.UVOut,
                            instanceNum,
                            ptankGlobal);
    }

    if( (NULL != PTankPrv->insColorsCB) && (NULL != instanceStr.colorsOut.data ))
    {
        PTankPrv->insColorsCB(&instanceStr.colorsOut,
                                instanceNum,
                                ptankGlobal);
    }

    if( (NULL != PTankPrv->insNormalsCB) && (NULL != instanceStr.normalsOut.data ))
    {
        PTankPrv->insNormalsCB(&instanceStr.normalsOut,
        instanceNum,
        ptankGlobal);
    }

    if( NULL != PTankPrv->insEndingCB )
    {
        PTankPrv->insEndingCB(&instanceStr,
                        atomic,
                        ptankGlobal,
                        instanceNum,
                        instFlags);
    }

    RWRETURN(TRUE);
}


/*
 ****************************************************************************
 */
RwBool
_rpPTankGenCreate(RpAtomic *atomic,
            RwInt32 maxPCount, RwUInt32 dataFlags __RWUNUSED__)
{
    RwBool result = TRUE;
    RpGeometry *geom = NULL;
    RwSurfaceProperties surfProp;
    RpMaterial *material;
    RpMorphTarget *morphTarget;
    RwV3d *vlist;
    RwV3d *nlist;
    RpTriangle *tlist;
    RwTexCoords *texCoord;
    RwRGBA  *clist;
    RwInt32 i;
    RwUInt32 geomFlag;
    RwSphere boundingSphere;

    RWFUNCTION(RWSTRING("_rpPTankGenCreate"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    geomFlag = rpGEOMETRYPOSITIONS | rpGEOMETRYTEXTURED;

    if( (dataFlags & rpPTANKDFLAGNORMAL) == rpPTANKDFLAGNORMAL )
    {
        geomFlag |= rpGEOMETRYLIGHT
                    | rpGEOMETRYNORMALS
                    | rpGEOMETRYMODULATEMATERIALCOLOR;
    }
    else
    {
        /*AJH:
            * No normals, but at least a constant color, so this is neaded
            */
        geomFlag |= rpGEOMETRYPRELIT;
    }

    if( (dataFlags &
        (rpPTANKDFLAGCOLOR | rpPTANKDFLAGVTXCOLOR | rpPTANKDFLAGCNSVTXCOLOR)) != 0 )
    {
        geomFlag |= rpGEOMETRYPRELIT;
    }


    geom = RpGeometryCreate(maxPCount * 4,
                            maxPCount * 2,
                            geomFlag);

    /*
        * Create the material
        */
    material = RpMaterialCreate();

    /*
        * Set the surface reflection coefficients...
        */
    surfProp.ambient = 0.3f;
    surfProp.diffuse = 1.0f;
    surfProp.specular = 1.0f;
    RpMaterialSetSurfaceProperties(material, &surfProp);

    /*
        * There's only one morph target, with index 0...
        */
    morphTarget = RpGeometryGetMorphTarget(geom, 0);


    /*
        * There's only one morph target, with index 0...
        */
    morphTarget = RpGeometryGetMorphTarget(geom, 0);

    /*
        * Construct the triangle and vertex lists by converting the
        * pentagons and hexagons to triangles.
        * Each pentagon and hexagon has its own vertices and normals
        * so that the faces can be rendered flat...
        */
    vlist = RpMorphTargetGetVertices(morphTarget);
    nlist = RpMorphTargetGetVertexNormals(morphTarget);
    texCoord = RpGeometryGetVertexTexCoords(geom, rwTEXTURECOORDINATEINDEX0);
    clist = RpGeometryGetPreLightColors(geom);

    tlist = RpGeometryGetTriangles(geom);

    for(i=0;i<maxPCount;i++)
    {
        /* vtx 1 */
        vlist->x =0.0f;
        vlist->y =0.0f;
        vlist->z =0.0f;
        vlist++;

        texCoord->u = 0.0f;
        texCoord->v = 0.0f;
        texCoord++;

        if( NULL != clist )
        {
            clist->red = 0;
            clist->green = 0;
            clist->blue = 0;
            clist->alpha = 255;
            clist++;
        }

        if( NULL != nlist )
        {
            nlist->x =1.0f;
            nlist->y =0.0f;
            nlist->z =0.0f;
            nlist++;
        }

        /* vtx 2 */
        vlist->x =0.0f;
        vlist->y =0.0f;
        vlist->z =0.0f;
        vlist++;

        texCoord->u = 0.0f;
        texCoord->v = 1.0f;
        texCoord++;

        if( NULL != clist )
        {
            clist->red = 0;
            clist->green = 0;
            clist->blue = 0;
            clist->alpha = 255;
            clist++;
        }

        if( NULL != nlist )
        {
            nlist->x =1.0f;
            nlist->y =0.0f;
            nlist->z =0.0f;
            nlist++;
        }

        /* vtx 3 */
        vlist->x =0.0f;
        vlist->y =0.0f;
        vlist->z =0.0f;
        vlist++;

        texCoord->u = 1.0f;
        texCoord->v = 1.0f;
        texCoord++;

        if( NULL != clist )
        {
            clist->red = 0;
            clist->green = 0;
            clist->blue = 0;
            clist->alpha = 255;
            clist++;
        }

        if( NULL != nlist )
        {
            nlist->x =1.0f;
            nlist->y =0.0f;
            nlist->z =0.0f;
            nlist++;
        }

        /* vtx 4 */
        vlist->x =0.0f;
        vlist->y =0.0f;
        vlist->z =0.0f;
        vlist++;

        texCoord->u = 1.0f;
        texCoord->v = 0.0f;
        texCoord++;

        if( NULL != clist )
        {
            clist->red = 0;
            clist->green = 0;
            clist->blue = 0;
            clist->alpha = 255;
            clist++;
        }

        if( NULL != nlist )
        {
            nlist->x =1.0f;
            nlist->y =0.0f;
            nlist->z =0.0f;
            nlist++;
        }

        /* Triangle 1 */
        RpGeometryTriangleSetVertexIndices(geom,
                                            tlist,
                                            (RwUInt16)(i*4 + 2),
                                            (RwUInt16)(i*4 + 1),
                                            (RwUInt16)(i*4 + 0)
                                            );
        RpGeometryTriangleSetMaterial(geom, tlist++, material);

        /* Triangle 2 */
        RpGeometryTriangleSetVertexIndices(geom,
                                            tlist,
                                            (RwUInt16)(i*4 + 3),
                                            (RwUInt16)(i*4 + 2),
                                            (RwUInt16)(i*4 + 0)
                                            );
        RpGeometryTriangleSetMaterial(geom, tlist++, material);

    }

    /*
        * Need to re-calculate and set the bounding-sphere ourselves
        * before unlocking...
        */

    RpMorphTargetCalcBoundingSphere(morphTarget, &boundingSphere);

    boundingSphere.radius = 1.0f;

    RpMorphTargetSetBoundingSphere(morphTarget, &boundingSphere);

    RpGeometryUnlock(geom);

    /*
        * Associate the geom with the atomic. This will increment the
        * geom's reference count by 1...
        */
    RpAtomicSetGeometry(atomic, geom, 0);

    /*
        * As a convenience, we can remove the application's ownership of the
        * geom, materials and textures it created by calling the corresponding
        * destroy functions. This will decrement their reference counts
        * without actually deleting the resources because they now have other
        * owners (the atomic owns the geom, the geom and its triangles
        * own the materials, each material owns a texture). Now we can simply use
        * RpClumpDestroy later when the application has finished with it...
        */
    RpGeometryDestroy(geom);

    RpMaterialDestroy(material);

    _rpPTankGENInstanceSelectCB(atomic);

    RWRETURN(result);
}


/*
 *****************************************************************************
 */
void
_rpPTankGENKill(RpAtomic *atomic, RwInt32 start, RwInt32 end)
{
    RwInt32 i;

    RwV3d deadPosition = {-666.0f, -666.0f, -666.0f };
    RwV3d *vlist;
    RpMorphTarget *morphTarget = NULL;
    RpGeometry *geom = RpAtomicGetGeometry(atomic);

    RWFUNCTION(RWSTRING("_rpPTankGENKill"));
    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));

    morphTarget = RpGeometryGetMorphTarget(geom, 0);
    RpGeometryLock(geom, rpGEOMETRYLOCKVERTICES );
    vlist = RpMorphTargetGetVertices(morphTarget);

    for(i=start;i<end;i++)
    {
        /* vtx 0 */
        vlist[i*4] = deadPosition;

        /* vtx 1 */
        vlist[i*4+1] = deadPosition;

        /* vtx 2 */
        vlist[i*4+2] = deadPosition;

        /* vtx 3 */
        vlist[i*4+3] = deadPosition;
    }

    RWRETURNVOID();
}


/*
 *****************************************************************************
 */
void
_rpPTankGENInstanceSelectCB(RpAtomic *atomic)
{
    RpPTankAtomicExtPrv *PTankPrv;
    RpPTankData *ptankGlobal;

    RWFUNCTION(RWSTRING("_rpPTankGENInstanceSelectCB"));
    RWASSERT(RpAtomicIsPTank(atomic));

    PTankPrv = RPATOMICPTANKPLUGINDATA(atomic);
    ptankGlobal = &PTankPrv->publicData;

    PTankPrv->insSetupCB = (RpPTankGENInstanceSetupCallback)rpPTankGENInsSetup;

    /*Selecting Position CallBack */
    if((ptankGlobal->format.dataFlags & rpPTANKDFLAGMATRIX ) == rpPTANKDFLAGMATRIX)
    {
        if((ptankGlobal->format.dataFlags & rpPTANKDFLAGUSECENTER) == rpPTANKDFLAGUSECENTER)
        {
            PTankPrv->insPosCB = (RpPTankGENInstancePosCallback)rpPTankGENInsPosCCPM;
        }
        else
        {
            PTankPrv->insPosCB = (RpPTankGENInstancePosCallback)rpPTankGENInsPosNCPM;
        }
    }
    else if((ptankGlobal->format.dataFlags & rpPTANKDFLAGCNSMATRIX ) == rpPTANKDFLAGCNSMATRIX)
    {
        if((ptankGlobal->format.dataFlags & rpPTANKDFLAGUSECENTER) == rpPTANKDFLAGUSECENTER)
        {
            PTankPrv->insPosCB = (RpPTankGENInstancePosCallback)rpPTankGENInsPosCCCSNR;
        }
        else
        {
            PTankPrv->insPosCB = (RpPTankGENInstancePosCallback)rpPTankGENInsPosNCCSNR;
        }
    }
    else
    {
        /* Calculate quads positions */
        if((ptankGlobal->format.dataFlags & rpPTANKDFLAG2DROTATE) == rpPTANKDFLAG2DROTATE)
        {
            if((ptankGlobal->format.dataFlags & rpPTANKDFLAGSIZE) == rpPTANKDFLAGSIZE)
            {
                if((ptankGlobal->format.dataFlags & rpPTANKDFLAGUSECENTER) == rpPTANKDFLAGUSECENTER)
                {
                    PTankPrv->insPosCB = (RpPTankGENInstancePosCallback)rpPTankGENInsPosCCPSPR;
                }
                else
                {
                    PTankPrv->insPosCB = (RpPTankGENInstancePosCallback)rpPTankGENInsPosNCPSPR;
                }
            }
            else
            {
                if((ptankGlobal->format.dataFlags & rpPTANKDFLAGUSECENTER) == rpPTANKDFLAGUSECENTER)
                {
                    PTankPrv->insPosCB = (RpPTankGENInstancePosCallback)rpPTankGENInsPosCCCSPR;
                }
                else
                {
                    PTankPrv->insPosCB = (RpPTankGENInstancePosCallback)rpPTankGENInsPosNCCSPR;
                }
            }
        }
        else
        {

            if((ptankGlobal->format.dataFlags & rpPTANKDFLAGSIZE) == rpPTANKDFLAGSIZE)
            {
                if((ptankGlobal->format.dataFlags & rpPTANKDFLAGUSECENTER) == rpPTANKDFLAGUSECENTER)
                {
                    PTankPrv->insPosCB = (RpPTankGENInstancePosCallback)rpPTankGENInsPosCCPSNR;
                }
                else
                {
                    PTankPrv->insPosCB = (RpPTankGENInstancePosCallback)rpPTankGENInsPosNCPSNR;
                }
            }
            else
            {
                /* Calculate world space size vectors */
                if((ptankGlobal->format.dataFlags & rpPTANKDFLAGUSECENTER) == rpPTANKDFLAGUSECENTER)
                {
                    PTankPrv->insPosCB = (RpPTankGENInstancePosCallback)rpPTankGENInsPosCCCSNR;
                }
                else
                {

                    PTankPrv->insPosCB = (RpPTankGENInstancePosCallback)rpPTankGENInsPosNCCSNR;
                }
            }
        }
    }

    /* Selecting UV CallBack */
    if(((ptankGlobal->format.dataFlags & rpPTANKDFLAGVTX2TEXCOORDS ) == rpPTANKDFLAGVTX2TEXCOORDS ))
    {
        PTankPrv->insUVCB = (RpPTankGENInstanceCallback)rpPTankGENInsUV2;
    }
    else if(((ptankGlobal->format.dataFlags & rpPTANKDFLAGVTX4TEXCOORDS ) == rpPTANKDFLAGVTX4TEXCOORDS ))
    {
        PTankPrv->insUVCB = (RpPTankGENInstanceCallback)rpPTankGENInsUV4;
    }
    else if(((ptankGlobal->format.dataFlags & rpPTANKDFLAGCNSVTX2TEXCOORDS ) == rpPTANKDFLAGCNSVTX2TEXCOORDS ))
    {
        PTankPrv->insUVCB = (RpPTankGENInstanceCallback)rpPTankGENInsUV2C;
    }
    else if(((ptankGlobal->format.dataFlags & rpPTANKDFLAGCNSVTX4TEXCOORDS ) == rpPTANKDFLAGCNSVTX4TEXCOORDS ))
    {
        PTankPrv->insUVCB = (RpPTankGENInstanceCallback)rpPTankGENInsUV4C;
    }
    else
    {
        PTankPrv->insUVCB = NULL;
    }

    /* Selecting Color CallBack */
    if((ptankGlobal->format.dataFlags & rpPTANKDFLAGCOLOR ) == rpPTANKDFLAGCOLOR)
    {
        PTankPrv->insColorsCB = (RpPTankGENInstanceCallback)rpPTankGENInsColorPP;
    }
    else if((ptankGlobal->format.dataFlags & rpPTANKDFLAGVTXCOLOR ) == rpPTANKDFLAGVTXCOLOR)
    {
        PTankPrv->insColorsCB = (RpPTankGENInstanceCallback)rpPTankGENInsColorPV;
    }
    else if((ptankGlobal->format.dataFlags & rpPTANKDFLAGCNSVTXCOLOR ) == rpPTANKDFLAGCNSVTXCOLOR)
    {
        PTankPrv->insColorsCB = (RpPTankGENInstanceCallback)rpPTankGENInsColorPVC;
    }
    else
    {
        PTankPrv->insColorsCB = (RpPTankGENInstanceCallback)rpPTankGENInsColorPPC;
    }

    if((ptankGlobal->format.dataFlags & rpPTANKDFLAGNORMAL ) == rpPTANKDFLAGNORMAL)
    {
        PTankPrv->insNormalsCB = (RpPTankGENInstanceCallback)rpPTankGENInsNormals;
    }
    else
    {
        PTankPrv->insNormalsCB = (RpPTankGENInstanceCallback) NULL;
    }

    PTankPrv->insEndingCB = (RpPTankGENInstanceEndingCallback)rpPTankGENInsEnding;


    RWRETURNVOID();
}


/*
 *****************************************************************************
 * rpPTankGENInsPosCCSSRR
 * CC : center      : NC no center, CC use constant center
 *
 * SS : size        : CS constant size,  PS size per prt
 *
 * RR : rotation    : NR no rotation, CR constant rotation, PR rotation per prt
 */

void
rpPTankGENInsPosNCCSNR(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                        /* no center, constant size, no rotation */
{
    RwInt32 i;
    RwV3d vectorSize[4];
    RwV3d pRight,pUp;
    RwV3d *src;
    RwV3d *dst;
    RwUInt8 *uSrc;
    RwUInt32 uStride;
    RwUInt8 *uDst;
    RwUInt32 uDStride;

    RWFUNCTION(RWSTRING("rpPTankGENInsPosNCCSNR"));

    RwV3dScale(&pRight,right,ptankGlobal->cSize.x * 0.5f);
    RwV3dScale(&pUp,up,ptankGlobal->cSize.y * 0.5f);

    /* vtx 0 */
    vectorSize[0].x = (- pRight.x - pUp.x);
    vectorSize[0].y = (- pRight.y - pUp.y);
    vectorSize[0].z = (- pRight.z - pUp.z);

    /* vtx 1 */
    vectorSize[1].x = (+ pRight.x - pUp.x);
    vectorSize[1].y = (+ pRight.y - pUp.y);
    vectorSize[1].z = (+ pRight.z - pUp.z);

    /* vtx 2 */
    vectorSize[2].x = (+ pRight.x + pUp.x);
    vectorSize[2].y = (+ pRight.y + pUp.y);
    vectorSize[2].z = (+ pRight.z + pUp.z);

    /* vtx 3 */
    vectorSize[3].x = (- pRight.x + pUp.x);
    vectorSize[3].y = (- pRight.y + pUp.y);
    vectorSize[3].z = (- pRight.z + pUp.z);

    uSrc = ptankGlobal->clusters[RPPTANKSIZEPOSITION].data;
    uStride = ptankGlobal->clusters[RPPTANKSIZEPOSITION].stride;

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    for (i = 0 ; i < pCount ; i++)
    {
        src = (RwV3d*)uSrc;

        /* vtx 0 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + vectorSize[0].x;
        dst->y = src->y + vectorSize[0].y;
        dst->z = src->z + vectorSize[0].z;
        uDst += uDStride;

        /* vtx 1 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + vectorSize[1].x;
        dst->y = src->y + vectorSize[1].y;
        dst->z = src->z + vectorSize[1].z;
        uDst += uDStride;

        /* vtx 2 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + vectorSize[2].x;
        dst->y = src->y + vectorSize[2].y;
        dst->z = src->z + vectorSize[2].z;
        uDst += uDStride;

        /* vtx 3 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + vectorSize[3].x;
        dst->y = src->y + vectorSize[3].y;
        dst->z = src->z + vectorSize[3].z;
        uDst += uDStride;

        uSrc += uStride;

    }

    RWRETURNVOID();
}

void
rpPTankGENInsPosCCCSNR(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                        /* center, constant size, no rotation */
{
    RwInt32 i;
    RwV3d vectorSize[4];
    RwV3d centerWS;
    RwV3d *src;
    RwV3d *dst;
    RwUInt8 *uSrc;
    RwUInt32 uStride;
    RwUInt8 *uDst;
    RwUInt32 uDStride;
    RwV3d pRight,pUp;

    RWFUNCTION(RWSTRING("rpPTankGENInsPosCCCSNR"));

    centerWS.x = ptankGlobal->cCenter.x * right->x +
                    ptankGlobal->cCenter.y * up->x;

    centerWS.y = ptankGlobal->cCenter.x * right->y +
                    ptankGlobal->cCenter.y * up->y;

    centerWS.z = ptankGlobal->cCenter.x * right->z +
                    ptankGlobal->cCenter.y * up->z;

    RwV3dScale(&pRight,right,ptankGlobal->cSize.x * 0.5f);
    RwV3dScale(&pUp,up,ptankGlobal->cSize.x * 0.5f);

    /* vtx 0 */
    vectorSize[0].x = (- pRight.x - pUp.x) + centerWS.x;
    vectorSize[0].y = (- pRight.y - pUp.y) + centerWS.y;
    vectorSize[0].z = (- pRight.z - pUp.z) + centerWS.z;

    /* vtx 1 */
    vectorSize[1].x = (+ pRight.x - pUp.x) + centerWS.x;
    vectorSize[1].y = (+ pRight.y - pUp.y) + centerWS.y;
    vectorSize[1].z = (+ pRight.z - pUp.z) + centerWS.z;

    /* vtx 2 */
    vectorSize[2].x = (+ pRight.x + pUp.x) + centerWS.x;
    vectorSize[2].y = (+ pRight.y + pUp.y) + centerWS.y;
    vectorSize[2].z = (+ pRight.z + pUp.z) + centerWS.z;

    /* vtx 3 */
    vectorSize[3].x = (- pRight.x + pUp.x) + centerWS.x;
    vectorSize[3].y = (- pRight.y + pUp.y) + centerWS.y;
    vectorSize[3].z = (- pRight.z + pUp.z) + centerWS.z;

    uSrc = ptankGlobal->clusters[RPPTANKSIZEPOSITION].data;
    uStride = ptankGlobal->clusters[RPPTANKSIZEPOSITION].stride;

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    for (i = 0 ; i < pCount ; i++)
    {
        src = (RwV3d*)uSrc;

        /* vtx 0 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + vectorSize[0].x;
        dst->y = src->y + vectorSize[0].y;
        dst->z = src->z + vectorSize[0].z;
        uDst += uDStride;

        /* vtx 1 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + vectorSize[1].x;
        dst->y = src->y + vectorSize[1].y;
        dst->z = src->z + vectorSize[1].z;
        uDst += uDStride;

        /* vtx 2 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + vectorSize[2].x;
        dst->y = src->y + vectorSize[2].y;
        dst->z = src->z + vectorSize[2].z;
        uDst += uDStride;

        /* vtx 3 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + vectorSize[3].x;
        dst->y = src->y + vectorSize[3].y;
        dst->z = src->z + vectorSize[3].z;
        uDst += uDStride;

        uSrc += uStride;

    }

    RWRETURNVOID();
}


void
rpPTankGENInsPosNCCSPR(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                        /* no center, constant size, per particle rotation */
{
    const RwReal sizeX = ptankGlobal->cSize.x * 0.5f;
    const RwReal sizeY = ptankGlobal->cSize.y * 0.5f;
    RwV3d pRight,pUp;
    RwInt32 i;
    RwReal  sinA;
    RwReal  cosA;
    RwV3d *src;
    RwV3d *dst;
    RwUInt8 *uSrc;
    RwUInt32 uStride;
    RwUInt8 *uRotation;
    RwUInt32 uRStride;
    RwUInt8 *uDst;
    RwUInt32 uDStride;

    RWFUNCTION(RWSTRING("rpPTankGENInsPosNCCSPR"));

    uSrc = ptankGlobal->clusters[RPPTANKSIZEPOSITION].data;
    uStride = ptankGlobal->clusters[RPPTANKSIZEPOSITION].stride;

    uRotation = ptankGlobal->clusters[RPPTANKSIZE2DROTATE].data;
    uRStride = ptankGlobal->clusters[RPPTANKSIZE2DROTATE].stride;

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    for (i = 0 ; i < pCount ; i++)
    {
        SinCos(*((RwReal*)uRotation),&sinA,&cosA);

        uRotation += uRStride;

        /* Calculate world space size vectors */
        pRight.x = (+ cosA) * right->x + (- sinA) * up->x;
        pRight.y = (+ cosA) * right->y + (- sinA) * up->y;
        pRight.z = (+ cosA) * right->z + (- sinA) * up->z;

        pUp.x = (+ sinA) * right->x + (+ cosA) * up->x;
        pUp.y = (+ sinA) * right->y + (+ cosA) * up->y;
        pUp.z = (+ sinA) * right->z + (+ cosA) * up->z;

        pRight.x *= sizeX;
        pRight.y *= sizeX;
        pRight.z *= sizeX;

        pUp.x *= sizeY;
        pUp.y *= sizeY;
        pUp.z *= sizeY;

        src = (RwV3d*)uSrc;

        /* vtx 0 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + (- pRight.x - pUp.x);
        dst->y = src->y + (- pRight.y - pUp.y);
        dst->z = src->z + (- pRight.z - pUp.z);
        uDst += uDStride;

        /* vtx 1 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + (+ pRight.x - pUp.x);
        dst->y = src->y + (+ pRight.y - pUp.y);
        dst->z = src->z + (+ pRight.z - pUp.z);
        uDst += uDStride;

        /* vtx 2 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + (+ pRight.x + pUp.x);
        dst->y = src->y + (+ pRight.y + pUp.y);
        dst->z = src->z + (+ pRight.z + pUp.z);
        uDst += uDStride;

        /* vtx 3 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + (- pRight.x + pUp.x);
        dst->y = src->y + (- pRight.y + pUp.y);
        dst->z = src->z + (- pRight.z + pUp.z);
        uDst += uDStride;

        uSrc += uStride;
    }

    RWRETURNVOID();
}


void
rpPTankGENInsPosNCPSNR(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                        /* no center, per particle size, no rotation */
{
    RwV3d pRight,pUp;
    RwInt32 i;
    RwV3d *src;
    RwV3d *dst;
    RwV2d *size;
    RwUInt8 *uSrc;
    RwUInt32 uStride;
    RwUInt8 *uSize;
    RwUInt32 uSStride;
    RwUInt8 *uDst;
    RwUInt32 uDStride;

    RWFUNCTION(RWSTRING("rpPTankGENInsPosNCPSNR"));

    uSrc = ptankGlobal->clusters[RPPTANKSIZEPOSITION].data;
    uStride = ptankGlobal->clusters[RPPTANKSIZEPOSITION].stride;

    uSize = ptankGlobal->clusters[RPPTANKSIZESIZE].data;
    uSStride = ptankGlobal->clusters[RPPTANKSIZESIZE].stride;

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    for (i = 0 ; i < pCount ; i++)
    {
        size = (RwV2d *)uSize;

        pRight.x = right->x * (size->x * 0.5f);
        pRight.y = right->y * (size->x * 0.5f);
        pRight.z = right->z * (size->x * 0.5f);

        pUp.x = up->x * (size->y * 0.5f);
        pUp.y = up->y * (size->y * 0.5f);
        pUp.z = up->z * (size->y * 0.5f);

        uSize += uSStride;

        src = (RwV3d*)uSrc;

        /* vtx 0 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + (- pRight.x - pUp.x);
        dst->y = src->y + (- pRight.y - pUp.y);
        dst->z = src->z + (- pRight.z - pUp.z);
        uDst += uDStride;

        /* vtx 1 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + (+ pRight.x - pUp.x);
        dst->y = src->y + (+ pRight.y - pUp.y);
        dst->z = src->z + (+ pRight.z - pUp.z);
        uDst += uDStride;

        /* vtx 2 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + (+ pRight.x + pUp.x);
        dst->y = src->y + (+ pRight.y + pUp.y);
        dst->z = src->z + (+ pRight.z + pUp.z);
        uDst += uDStride;

        /* vtx 3 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + (- pRight.x + pUp.x);
        dst->y = src->y + (- pRight.y + pUp.y);
        dst->z = src->z + (- pRight.z + pUp.z);
        uDst += uDStride;

        uSrc += uStride;
    }

    RWRETURNVOID();
}


void
rpPTankGENInsPosNCPSPR(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                        /* no center, constant size, per particle rotation */
{
    RwV3d pRight,pUp;
    RwInt32 i;
    RwReal  sinA;
    RwReal  cosA;
    RwV3d *src;
    RwV3d *dst;
    RwV2d *size;
    RwUInt8 *uSrc;
    RwUInt32 uStride;
    RwUInt8 *uRotation;
    RwUInt32 uRStride;
    RwUInt8 *uSize;
    RwUInt32 uSStride;
    RwUInt8 *uDst;
    RwUInt32 uDStride;

    RWFUNCTION(RWSTRING("rpPTankGENInsPosNCPSPR"));

    uSrc = ptankGlobal->clusters[RPPTANKSIZEPOSITION].data;
    uStride = ptankGlobal->clusters[RPPTANKSIZEPOSITION].stride;

    uRotation = ptankGlobal->clusters[RPPTANKSIZE2DROTATE].data;
    uRStride = ptankGlobal->clusters[RPPTANKSIZE2DROTATE].stride;

    uSize = ptankGlobal->clusters[RPPTANKSIZESIZE].data;
    uSStride = ptankGlobal->clusters[RPPTANKSIZESIZE].stride;

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    for (i = 0 ; i < pCount ; i++)
    {
        SinCos(*((RwReal*)uRotation),&sinA,&cosA);
        uRotation += uRStride;

        /* Calculate world space size vectors */
        pRight.x = (+ cosA) * right->x + (- sinA) * up->x;
        pRight.y = (+ cosA) * right->y + (- sinA) * up->y;
        pRight.z = (+ cosA) * right->z + (- sinA) * up->z;

        pUp.x = (+ sinA) * right->x + (+ cosA) * up->x;
        pUp.y = (+ sinA) * right->y + (+ cosA) * up->y;
        pUp.z = (+ sinA) * right->z + (+ cosA) * up->z;

        size = (RwV2d *)uSize;

        pRight.x *= size->x*0.5f;
        pRight.y *= size->x*0.5f;
        pRight.z *= size->x*0.5f;

        pUp.x *= size->y*0.5f;
        pUp.y *= size->y*0.5f;
        pUp.z *= size->y*0.5f;

        uSize += uSStride;

        src = (RwV3d*)uSrc;

        /* vtx 0 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + (- pRight.x - pUp.x);
        dst->y = src->y + (- pRight.y - pUp.y);
        dst->z = src->z + (- pRight.z - pUp.z);
        uDst += uDStride;

        /* vtx 1 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + (+ pRight.x - pUp.x);
        dst->y = src->y + (+ pRight.y - pUp.y);
        dst->z = src->z + (+ pRight.z - pUp.z);
        uDst += uDStride;

        /* vtx 2 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + (+ pRight.x + pUp.x);
        dst->y = src->y + (+ pRight.y + pUp.y);
        dst->z = src->z + (+ pRight.z + pUp.z);
        uDst += uDStride;

        /* vtx 3 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + (- pRight.x + pUp.x);
        dst->y = src->y + (- pRight.y + pUp.y);
        dst->z = src->z + (- pRight.z + pUp.z);
        uDst += uDStride;

        uSrc += uStride;
    }

    RWRETURNVOID();
}

void
rpPTankGENInsPosCCCSPR(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                        /* constant center, constant size, per particle rotation */
{
    const RwReal sizeX = ptankGlobal->cSize.x * 0.5f;
    const RwReal sizeY = ptankGlobal->cSize.y * 0.5f;
    RwV3d pRight,pUp;
    RwInt32 i;
    RwReal  sinA;
    RwReal  cosA;
    RwV3d *src;
    RwV3d *dst;
    RwUInt8 *uSrc;
    RwUInt32 uStride;
    RwUInt8 *uRotation;
    RwUInt32 uRStride;
    RwUInt8 *uDst;
    RwUInt32 uDStride;
    RwV3d   centerWS;

    RWFUNCTION(RWSTRING("rpPTankGENInsPosCCCSPR"));

    uSrc = ptankGlobal->clusters[RPPTANKSIZEPOSITION].data;
    uStride = ptankGlobal->clusters[RPPTANKSIZEPOSITION].stride;

    uRotation = ptankGlobal->clusters[RPPTANKSIZE2DROTATE].data;
    uRStride = ptankGlobal->clusters[RPPTANKSIZE2DROTATE].stride;

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    for (i = 0 ; i < pCount ; i++)
    {
        SinCos(*((RwReal*)uRotation),&sinA,&cosA);
        uRotation += uRStride;

        /* Calculate world space size vectors */
        pRight.x = (+ cosA) * right->x + (- sinA) * up->x;
        pRight.y = (+ cosA) * right->y + (- sinA) * up->y;
        pRight.z = (+ cosA) * right->z + (- sinA) * up->z;

        pUp.x = (+ sinA) * right->x + (+ cosA) * up->x;
        pUp.y = (+ sinA) * right->y + (+ cosA) * up->y;
        pUp.z = (+ sinA) * right->z + (+ cosA) * up->z;

        centerWS.x = ptankGlobal->cCenter.x * pRight.x
                                            + ptankGlobal->cCenter.y * pUp.x;
        centerWS.y = ptankGlobal->cCenter.x * pRight.y
                                            + ptankGlobal->cCenter.y * pUp.y;
        centerWS.z = ptankGlobal->cCenter.x * pRight.z
                                            + ptankGlobal->cCenter.y * pUp.z;

        pRight.x *= sizeX;
        pRight.y *= sizeX;
        pRight.z *= sizeX;

        pUp.x *= sizeY;
        pUp.y *= sizeY;
        pUp.z *= sizeY;

        src = (RwV3d*)uSrc;

        /* vtx 0 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + centerWS.x + (- pRight.x - pUp.x);
        dst->y = src->y + centerWS.y + (- pRight.y - pUp.y);
        dst->z = src->z + centerWS.z + (- pRight.z - pUp.z);
        uDst += uDStride;

        /* vtx 1 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + centerWS.x + (+ pRight.x - pUp.x);
        dst->y = src->y + centerWS.y + (+ pRight.y - pUp.y);
        dst->z = src->z + centerWS.z + (+ pRight.z - pUp.z);
        uDst += uDStride;

        /* vtx 2 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + centerWS.x + (+ pRight.x + pUp.x);
        dst->y = src->y + centerWS.y + (+ pRight.y + pUp.y);
        dst->z = src->z + centerWS.z + (+ pRight.z + pUp.z);
        uDst += uDStride;

        /* vtx 3 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + centerWS.x + (- pRight.x + pUp.x);
        dst->y = src->y + centerWS.y + (- pRight.y + pUp.y);
        dst->z = src->z + centerWS.z + (- pRight.z + pUp.z);
        uDst += uDStride;

        uSrc += uStride;
    }

    RWRETURNVOID();
}


void
rpPTankGENInsPosCCPSNR(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                        /* constant center, per particle size, no rotation */
{
    RwV3d pRight,pUp;
    RwInt32 i;
    RwV3d *src;
    RwV3d *dst;
    RwV2d *size;
    RwUInt8 *uSrc;
    RwUInt32 uStride;
    RwUInt8 *uSize;
    RwUInt32 uSStride;
    RwUInt8 *uDst;
    RwUInt32 uDStride;
    RwV3d   centerWS;

    RWFUNCTION(RWSTRING("rpPTankGENInsPosCCPSNR"));

    uSrc = ptankGlobal->clusters[RPPTANKSIZEPOSITION].data;
    uStride = ptankGlobal->clusters[RPPTANKSIZEPOSITION].stride;

    uSize = ptankGlobal->clusters[RPPTANKSIZESIZE].data;
    uSStride = ptankGlobal->clusters[RPPTANKSIZESIZE].stride;

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    centerWS.x = ptankGlobal->cCenter.x * right->x + ptankGlobal->cCenter.y * up->x;
    centerWS.y = ptankGlobal->cCenter.x * right->y + ptankGlobal->cCenter.y * up->y;
    centerWS.z = ptankGlobal->cCenter.x * right->z + ptankGlobal->cCenter.y * up->z;

    for (i = 0 ; i < pCount ; i++)
    {
        size = (RwV2d *)uSize;

        pRight.x = right->x * (size->x * 0.5f);
        pRight.y = right->y * (size->x * 0.5f);
        pRight.z = right->z * (size->x * 0.5f);

        pUp.x = up->x * (size->y * 0.5f);
        pUp.y = up->y * (size->y * 0.5f);
        pUp.z = up->z * (size->y * 0.5f);

        uSize += uSStride;

        src = (RwV3d*)uSrc;

        /* vtx 0 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + centerWS.x + (- pRight.x - pUp.x);
        dst->y = src->y + centerWS.y + (- pRight.y - pUp.y);
        dst->z = src->z + centerWS.z + (- pRight.z - pUp.z);
        uDst += uDStride;

        /* vtx 1 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + centerWS.x + (+ pRight.x - pUp.x);
        dst->y = src->y + centerWS.y + (+ pRight.y - pUp.y);
        dst->z = src->z + centerWS.z + (+ pRight.z - pUp.z);
        uDst += uDStride;

        /* vtx 2 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + centerWS.x + (+ pRight.x + pUp.x);
        dst->y = src->y + centerWS.y + (+ pRight.y + pUp.y);
        dst->z = src->z + centerWS.z + (+ pRight.z + pUp.z);
        uDst += uDStride;

        /* vtx 3 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + centerWS.x + (- pRight.x + pUp.x);
        dst->y = src->y + centerWS.y + (- pRight.y + pUp.y);
        dst->z = src->z + centerWS.z + (- pRight.z + pUp.z);
        uDst += uDStride;

        uSrc += uStride;
    }

    RWRETURNVOID();
}

void
rpPTankGENInsPosCCPSPR(RpPTankLockStruct *dstCluster,
                        RwV3d *right,
                        RwV3d *up,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                        /* constant center, per particle size, per particle rotation */
{
    RwV3d pRight,pUp;
    RwInt32 i;
    RwReal  sinA;
    RwReal  cosA;
    RwV3d   centerWS;
    RwV3d *src;
    RwV3d *dst;
    RwV2d *size;
    RwUInt8 *uSrc;
    RwUInt32 uStride;
    RwUInt8 *uRotation;
    RwUInt32 uRStride;
    RwUInt8 *uSize;
    RwUInt32 uSStride;
    RwUInt8 *uDst;
    RwUInt32 uDStride;

    RWFUNCTION(RWSTRING("rpPTankGENInsPosCCPSPR"));

    uSrc = ptankGlobal->clusters[RPPTANKSIZEPOSITION].data;
    uStride = ptankGlobal->clusters[RPPTANKSIZEPOSITION].stride;

    uRotation = ptankGlobal->clusters[RPPTANKSIZE2DROTATE].data;
    uRStride = ptankGlobal->clusters[RPPTANKSIZE2DROTATE].stride;

    uSize = ptankGlobal->clusters[RPPTANKSIZESIZE].data;
    uSStride = ptankGlobal->clusters[RPPTANKSIZESIZE].stride;

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    for (i = 0 ; i < pCount ; i++)
    {
        SinCos(*((RwReal*)uRotation),&sinA,&cosA);
        uRotation += uRStride;

        /* Calculate world space size vectors */
        pRight.x = (+ cosA) * right->x + (- sinA) * up->x;
        pRight.y = (+ cosA) * right->y + (- sinA) * up->y;
        pRight.z = (+ cosA) * right->z + (- sinA) * up->z;

        pUp.x = (+ sinA) * right->x + (+ cosA) * up->x;
        pUp.y = (+ sinA) * right->y + (+ cosA) * up->y;
        pUp.z = (+ sinA) * right->z + (+ cosA) * up->z;

        centerWS.x = ptankGlobal->cCenter.x * pRight.x + ptankGlobal->cCenter.y * pUp.x;
        centerWS.y = ptankGlobal->cCenter.x * pRight.y + ptankGlobal->cCenter.y * pUp.y;
        centerWS.z = ptankGlobal->cCenter.x * pRight.z + ptankGlobal->cCenter.y * pUp.z;

        size = (RwV2d *)uSize;

        pRight.x *= size->x*0.5f;
        pRight.y *= size->x*0.5f;
        pRight.z *= size->x*0.5f;

        pUp.x *= size->y*0.5f;
        pUp.y *= size->y*0.5f;
        pUp.z *= size->y*0.5f;

        uSize += uSStride;

        src = (RwV3d*)uSrc;

        /* vtx 0 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + centerWS.x + (- pRight.x - pUp.x);
        dst->y = src->y + centerWS.y + (- pRight.y - pUp.y);
        dst->z = src->z + centerWS.z + (- pRight.z - pUp.z);
        uDst += uDStride;

        /* vtx 1 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + centerWS.x +  (+ pRight.x - pUp.x);
        dst->y = src->y + centerWS.y +  (+ pRight.y - pUp.y);
        dst->z = src->z + centerWS.z +  (+ pRight.z - pUp.z);
        uDst += uDStride;

        /* vtx 2 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + centerWS.x +  (+ pRight.x + pUp.x);
        dst->y = src->y + centerWS.y +  (+ pRight.y + pUp.y);
        dst->z = src->z + centerWS.z +  (+ pRight.z + pUp.z);
        uDst += uDStride;

        /* vtx 3 */
        dst = (RwV3d*)uDst;
        dst->x = src->x + centerWS.x +  (- pRight.x + pUp.x);
        dst->y = src->y + centerWS.y +  (- pRight.y + pUp.y);
        dst->z = src->z + centerWS.z +  (- pRight.z + pUp.z);
        uDst += uDStride;

        uSrc += uStride;
    }

    RWRETURNVOID();
}

void
rpPTankGENInsPosNCPM(RpPTankLockStruct *dstCluster,
                        RwV3d *_right __RWUNUSED__,  /* ignore */
                        RwV3d *_up __RWUNUSED__,     /* ignore */
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
{
    RwInt32 i;
    RwV3d *dst;
    const RwV3d *pos;
    RwMatrix *mtx;
    RwUInt8 *uDst;
    RwUInt32 uDStride;
    RwUInt8 *uSrc;
    RwUInt32 uSStride;

    RWFUNCTION(RWSTRING("rpPTankGENInsPosNCPM"));

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    uSrc = ptankGlobal->clusters[RPPTANKSIZEMATRIX].data;
    uSStride = ptankGlobal->clusters[RPPTANKSIZEMATRIX].stride;

    for (i = 0 ; i < pCount ; i++)
    {
        RwV3d right, up;
        RwV3d rightUp, rightDown;

        mtx = (RwMatrix *)uSrc;
        RwV3dScale(&right,RwMatrixGetRight(mtx), 0.5f);
        RwV3dScale(&up, RwMatrixGetUp(mtx), 0.5f);

        RwV3dAdd(&rightUp, &right, &up);
        RwV3dSub(&rightDown, &right, &up);

        pos = RwMatrixGetPos(mtx);

        uSrc += uSStride;

        /* vtx 0 */
        dst = (RwV3d*)uDst;
        RwV3dAdd(dst, pos, &rightDown);
        uDst += uDStride;

        /* vtx 3 */
        dst = (RwV3d*)uDst;
        RwV3dSub(dst, pos, &rightUp);
        uDst += uDStride;

        /* vtx 2 */
        dst = (RwV3d*)uDst;
        RwV3dSub(dst, pos, &rightDown);
        uDst += uDStride;

        /* vtx 1 */
        dst = (RwV3d*)uDst;
        RwV3dAdd(dst, pos, &rightUp);
        uDst += uDStride;
    }

    RWRETURNVOID();
}


void
rpPTankGENInsPosCCPM(RpPTankLockStruct *dstCluster,
                        RwV3d *_right __RWUNUSED__,  /* ignore */
                        RwV3d *_up __RWUNUSED__,     /* ignore */
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                         /* constant center (use mtx pos), per particle matrix */
{
    const RwReal factx = ptankGlobal->cCenter.x;
    const RwReal facty = ptankGlobal->cCenter.y;
    RwInt32 i;
    RwV3d *dst;
    RwMatrix *mtx;
    RwUInt8 *uDst;
    RwUInt32 uDStride;
    RwUInt8 *uSrc;
    RwUInt32 uSStride;

    RWFUNCTION(RWSTRING("rpPTankGENInsPosCCPM"));

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    uSrc = ptankGlobal->clusters[RPPTANKSIZEMATRIX].data;
    uSStride = ptankGlobal->clusters[RPPTANKSIZEMATRIX].stride;

    for (i = 0 ; i < pCount ; i++)
    {
        const RwV3d *right, *up, *pos;
        RwV3d rightUp, rightDown;
        RwV3d origin;

        mtx   = (RwMatrix *)uSrc;
        right = RwMatrixGetRight(mtx);
        up    = RwMatrixGetUp(mtx);
        pos   = RwMatrixGetPos(mtx);

        rightUp.x   = (right->x + up->x) * 0.5f;
        rightUp.y   = (right->y + up->y) * 0.5f;
        rightUp.z   = (right->z + up->z) * 0.5f;
        rightDown.x = (right->x - up->x) * 0.5f;
        rightDown.y = (right->y - up->y) * 0.5f;
        rightDown.z = (right->z - up->z) * 0.5f;

        origin.x = pos->x + (right->x * factx) + (up->x * facty);
        origin.y = pos->y + (right->y * factx) + (up->y * facty);
        origin.z = pos->z + (right->z * factx) + (up->z * facty);

        uSrc += uSStride;

        /* vtx 0 */
        dst = (RwV3d*)uDst;
        RwV3dAdd(dst, &origin, &rightDown);
        uDst += uDStride;

        /* vtx 3 */
        dst = (RwV3d*)uDst;
        RwV3dSub(dst, &origin, &rightUp);
        uDst += uDStride;

        /* vtx 2 */
        dst = (RwV3d*)uDst;
        RwV3dSub(dst, &origin, &rightDown);
        uDst += uDStride;

        /* vtx 1 */
        dst = (RwV3d*)uDst;
        RwV3dAdd(dst, &origin, &rightUp);
        uDst += uDStride;
    }


    RWRETURNVOID();
}

void
rpPTankGENInsColorPP(RpPTankLockStruct *dstCluster,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                         /* 1 Color per particle*/
{
    RwInt32 i;
    RwRGBA *src;
    RwRGBA *dst;
    RwUInt8 *uDst;
    RwUInt32 uDStride;
    RwUInt8 *uSrc;
    RwUInt32 uSStride;
    RwUInt32 color;

    RWFUNCTION(RWSTRING("rpPTankGENInsColorPP"));

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    uSrc = ptankGlobal->clusters[RPPTANKSIZECOLOR].data;
    uSStride = ptankGlobal->clusters[RPPTANKSIZECOLOR].stride;

    for (i = 0 ; i < pCount ; i++)
    {
        src = (RwRGBA *)uSrc;
        color = RGBATOCOLOR(src);
        uSrc += uSStride;

        /* vtx 0 */
        dst = (RwRGBA *)uDst;

        *(RwUInt32*)dst = color;
        uDst += uDStride;

        /* vtx 1 */
        dst = (RwRGBA *)uDst;
        *(RwUInt32*)dst = color;
        uDst += uDStride;

        /* vtx 2 */
        dst = (RwRGBA *)uDst;
        *(RwUInt32*)dst = color;
        uDst += uDStride;

        /* vtx 3 */
        dst = (RwRGBA *)uDst;
        *(RwUInt32*)dst = color;
        uDst += uDStride;
    }

    RWRETURNVOID();
}

void
rpPTankGENInsColorPV(RpPTankLockStruct *dstCluster,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                         /* 1 Color per vertex*/
{
    RwInt32 i;
    RwRGBA *src;
    RwRGBA *dst;
    RwUInt8 *uDst;
    RwUInt32 uDStride;
    RwUInt8 *uSrc;
    RwUInt32 uSStride;

    RWFUNCTION(RWSTRING("rpPTankGENInsColorPV"));

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    uSrc = ptankGlobal->clusters[RPPTANKSIZEVTXCOLOR].data;
    uSStride = ptankGlobal->clusters[RPPTANKSIZEVTXCOLOR].stride;

    for (i = 0 ; i < pCount ; i++)
    {
        src = (RwRGBA *)uSrc;
        uSrc += uSStride;

        /* vtx 0 */
        dst = (RwRGBA *)uDst;
        *(RwUInt32*)dst = RGBATOCOLOR(&src[0]);
        uDst += uDStride;

        /* vtx 1 */
        dst = (RwRGBA *)uDst;
        *(RwUInt32*)dst = RGBATOCOLOR(&src[1]);
        uDst += uDStride;

        /* vtx 2 */
        dst = (RwRGBA *)uDst;
        *(RwUInt32*)dst = RGBATOCOLOR(&src[2]);
        uDst += uDStride;

        /* vtx 3 */
        dst = (RwRGBA *)uDst;
        *(RwUInt32*)dst = RGBATOCOLOR(&src[3]);
        uDst += uDStride;


    }

    RWRETURNVOID();
}

void
rpPTankGENInsColorPVC(RpPTankLockStruct *dstCluster,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                         /* 1 constant Color per vertex*/
{
    RwInt32 i;
    RwRGBA *dst;
    RwUInt8 *uDst;
    RwUInt32 uDStride;
    RwUInt32 color[4];

    RWFUNCTION(RWSTRING("rpPTankGENInsColorPVC"));

    color[0] = RGBATOCOLOR(&ptankGlobal->cVtxColor[0]);
    color[1] = RGBATOCOLOR(&ptankGlobal->cVtxColor[1]);
    color[2] = RGBATOCOLOR(&ptankGlobal->cVtxColor[2]);
    color[3] = RGBATOCOLOR(&ptankGlobal->cVtxColor[3]);

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    for (i = 0 ; i < pCount ; i++)
    {
        /* vtx 0 */
        dst = (RwRGBA *)uDst;
        *(RwUInt32*)dst = color[0];
        uDst += uDStride;

        /* vtx 1 */
        dst = (RwRGBA *)uDst;
        *(RwUInt32*)dst = color[1];
        uDst += uDStride;

        /* vtx 2 */
        dst = (RwRGBA *)uDst;
        *(RwUInt32*)dst = color[2];
        uDst += uDStride;

        /* vtx 3 */
        dst = (RwRGBA *)uDst;
        *(RwUInt32*)dst = color[3];
        uDst += uDStride;

    }

    RWRETURNVOID();
}

void
rpPTankGENInsColorPPC(RpPTankLockStruct *dstCluster,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                         /* 1 constant Color per particle*/
{
    RwInt32 i;
    RwRGBA *dst;
    RwUInt8 *uDst;
    RwUInt32 uDStride;
    RwUInt32 color;

    RWFUNCTION(RWSTRING("rpPTankGENInsColorPPC"));

    color = RGBATOCOLOR(&ptankGlobal->cColor);

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    for (i = 0 ; i < pCount ; i++)
    {
        /* vtx 0 */
        dst = (RwRGBA *)uDst;
        *(RwUInt32*)dst = color;
        uDst += uDStride;

        /* vtx 1 */
        dst = (RwRGBA *)uDst;
        *(RwUInt32*)dst = color;
        uDst += uDStride;

        /* vtx 2 */
        dst = (RwRGBA *)uDst;
        *(RwUInt32*)dst = color;
        uDst += uDStride;

        /* vtx 3 */
        dst = (RwRGBA *)uDst;
        *(RwUInt32*)dst = color;
        uDst += uDStride;

    }

    RWRETURNVOID();
}

void
rpPTankGENInsUV2(RpPTankLockStruct *dstCluster,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                         /* 2 pair of uvs per particle*/
{
    RwInt32 i;
    RwTexCoords *dst;
    RwUInt8 *uDst;
    RwUInt32 uDStride;
    RwUInt8 *uSrc;
    RwUInt32 uSStride;
    RwTexCoords *tex0,*tex1;

    RWFUNCTION(RWSTRING("rpPTankGENInsUV2"));

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    uSrc = ptankGlobal->clusters[RPPTANKSIZEVTX2TEXCOORDS].data;
    uSStride = ptankGlobal->clusters[RPPTANKSIZEVTX2TEXCOORDS].stride;

    for (i = 0 ; i < pCount ; i++)
    {
        tex0 = &((RwTexCoords *)uSrc)[0];
        tex1 = &((RwTexCoords *)uSrc)[1];

        uSrc += uSStride;

        /* vtx 0 */
        dst = (RwTexCoords *)uDst;
        dst->u = tex0->u;
        dst->v = tex1->v;
        uDst += uDStride;

        /* vtx 1 */
        dst = (RwTexCoords *)uDst;
        dst->u = tex1->u;
        dst->v = tex1->v;
        uDst += uDStride;

        /* vtx 2 */
        dst = (RwTexCoords *)uDst;
        dst->u = tex1->u;
        dst->v = tex0->v;
        uDst += uDStride;

        /* vtx 3 */
        dst = (RwTexCoords *)uDst;
        dst->u = tex0->u;
        dst->v = tex0->v;
        uDst += uDStride;
    }

    RWRETURNVOID();
}

void
rpPTankGENInsUV4(RpPTankLockStruct *dstCluster,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                         /* 4 pair of uvs per particle*/
{
    RwInt32 i;
    RwTexCoords *dst;
    RwUInt8 *uDst;
    RwUInt32 uDStride;
    RwUInt8 *uSrc;
    RwUInt32 uSStride;
    RwTexCoords *tex0,*tex1;
    RwTexCoords *tex2,*tex3;

    RWFUNCTION(RWSTRING("rpPTankGENInsUV4"));

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    uSrc = ptankGlobal->clusters[RPPTANKSIZEVTX4TEXCOORDS].data;
    uSStride = ptankGlobal->clusters[RPPTANKSIZEVTX4TEXCOORDS].stride;

    for (i = 0 ; i < pCount ; i++)
    {
        tex0 = &((RwTexCoords *)uSrc)[0];
        tex1 = &((RwTexCoords *)uSrc)[1];
        tex2 = &((RwTexCoords *)uSrc)[2];
        tex3 = &((RwTexCoords *)uSrc)[3];

        uSrc += uSStride;

        /* vtx 0 */
        dst = (RwTexCoords *)uDst;
        dst->u = tex0->u;
        dst->v = tex0->v;
        uDst += uDStride;

        /* vtx 1 */
        dst = (RwTexCoords *)uDst;
        dst->u = tex1->u;
        dst->v = tex1->v;
        uDst += uDStride;

        /* vtx 2 */
        dst = (RwTexCoords *)uDst;
        dst->u = tex2->u;
        dst->v = tex2->v;
        uDst += uDStride;

        /* vtx 3 */
        dst = (RwTexCoords *)uDst;
        dst->u = tex3->u;
        dst->v = tex3->v;
        uDst += uDStride;

    }

    RWRETURNVOID();
}

void
rpPTankGENInsUV2C(RpPTankLockStruct *dstCluster,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                         /* 2 pair of uvs per system*/
{
    RwInt32 i;
    RwTexCoords *dst;
    RwUInt8 *uDst;
    RwUInt32 uDStride;
    RwTexCoords *tex0,*tex1;

    RWFUNCTION(RWSTRING("rpPTankGENInsUV2C"));

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    tex0 = &ptankGlobal->cUV[0];
    tex1 = &ptankGlobal->cUV[1];

    for (i = 0 ; i < pCount ; i++)
    {
        /* vtx 0 */
        dst = (RwTexCoords *)uDst;
        dst->u = tex0->u;
        dst->v = tex1->v;
        uDst += uDStride;

        /* vtx 1 */
        dst = (RwTexCoords *)uDst;
        dst->u = tex1->u;
        dst->v = tex1->v;
        uDst += uDStride;

        /* vtx 2 */
        dst = (RwTexCoords *)uDst;
        dst->u = tex1->u;
        dst->v = tex0->v;
        uDst += uDStride;

        /* vtx 3 */
        dst = (RwTexCoords *)uDst;
        dst->u = tex0->u;
        dst->v = tex0->v;
        uDst += uDStride;
    }

    RWRETURNVOID();
}

void
rpPTankGENInsUV4C(RpPTankLockStruct *dstCluster,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                         /* 4 pair of uvs per particle*/
{
    RwInt32 i;
    RwTexCoords *dst;
    RwUInt8 *uDst;
    RwUInt32 uDStride;
    RwTexCoords *tex0,*tex1;
    RwTexCoords *tex2,*tex3;

    RWFUNCTION(RWSTRING("rpPTankGENInsUV4C"));

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    tex0 = &ptankGlobal->cUV[0];
    tex1 = &ptankGlobal->cUV[1];
    tex2 = &ptankGlobal->cUV[2];
    tex3 = &ptankGlobal->cUV[3];

    for (i = 0 ; i < pCount ; i++)
    {
        /* vtx 0 */
        dst = (RwTexCoords *)uDst;
        dst->u = tex0->u;
        dst->v = tex0->v;
        uDst += uDStride;

        /* vtx 1 */
        dst = (RwTexCoords *)uDst;
        dst->u = tex1->u;
        dst->v = tex1->v;
        uDst += uDStride;

        /* vtx 2 */
        dst = (RwTexCoords *)uDst;
        dst->u = tex2->u;
        dst->v = tex2->v;
        uDst += uDStride;

        /* vtx 3 */
        dst = (RwTexCoords *)uDst;
        dst->u = tex3->u;
        dst->v = tex3->v;
        uDst += uDStride;

    }

    RWRETURNVOID();
}

void
rpPTankGENInsNormals(RpPTankLockStruct *dstCluster,
                        RwInt32 pCount,
                        RpPTankData *ptankGlobal)
                         /* one normal per particles*/
{
    RwInt32 i;
    RwUInt8 *uDst;
    RwUInt32 uDStride;
    RwUInt8 *uSrc;
    RwUInt32 uSStride;
    RwV3d *src,*dst;

    RWFUNCTION(RWSTRING("rpPTankGENInsNormals"));

    uDst = dstCluster->data;
    uDStride = dstCluster->stride;

    uSrc = ptankGlobal->clusters[RPPTANKSIZENORMAL].data;
    uSStride = ptankGlobal->clusters[RPPTANKSIZENORMAL].stride;

    for(i=0;i<pCount;i++)
    {
        src = (RwV3d *)uSrc;

        dst = (RwV3d *)uDst;
        dst->x = src->x;
        dst->y = src->y;
        dst->z = src->z;
        uDst += uDStride;

        dst = (RwV3d *)uDst;
        dst->x = src->x;
        dst->y = src->y;
        dst->z = src->z;
        uDst += uDStride;

        dst = (RwV3d *)uDst;
        dst->x = src->x;
        dst->y = src->y;
        dst->z = src->z;
        uDst += uDStride;

        dst = (RwV3d *)uDst;
        dst->x = src->x;
        dst->y = src->y;
        dst->z = src->z;
        uDst += uDStride;

        uSrc += uSStride;
    }

    RWRETURNVOID();
}


/*
 *****************************************************************************
 */
void
rpPTankGENInsSetup(RpPTANKInstanceSetupData *data,
                RpAtomic *atomic,
                RpPTankData *ptankGlobal,
                RwInt32 actPCount __RWUNUSED__,
                RwUInt32 instFlags)
{
    RwV3d *right = &data->right;
    RwV3d *up = &data->up;
    const RwMatrix  *camLTM;
    RpGeometry *geom = RpAtomicGetGeometry(atomic);
    RpMorphTarget *morphTarget = NULL;

    RWFUNCTION(RWSTRING("rpPTankGENInsSetup"));

    RWASSERT(atomic);
    RWASSERT(RpAtomicIsPTank(atomic));


    if((instFlags & rpPTANKIFLAGACTNUMCHG ) == rpPTANKIFLAGACTNUMCHG)
    {
        _rpPTankGENKill(atomic, actPCount,
                RpPTankAtomicGetMaximumParticlesCount(atomic));
    }

    /* Get current camera matrix */
    camLTM = RwFrameGetLTM(RwCameraGetFrame((RwCamera *)RWSRCGLOBAL(curCamera)));


    /*Setting up right and up vectors */
    if((ptankGlobal->format.dataFlags & rpPTANKDFLAGCNSMATRIX )
                                            == rpPTANKDFLAGCNSMATRIX)
    {
        RwV3dAssign(right, RwMatrixGetRight(&ptankGlobal->cMatrix));
        RwV3dAssign(up, RwMatrixGetUp(&ptankGlobal->cMatrix));
    }
    else if((ptankGlobal->format.dataFlags & rpPTANKDFLAGCNS2DROTATE)
                                            == rpPTANKDFLAGCNS2DROTATE)
    {
        RwV3d   rightCamera;
        RwV3d   upCamera;
        RwReal sinA, cosA;

        rightCamera.x = -camLTM->right.x;
        rightCamera.y = -camLTM->right.y;
        rightCamera.z = -camLTM->right.z;

        upCamera.x = camLTM->up.x;
        upCamera.y = camLTM->up.y;
        upCamera.z = camLTM->up.z;

        SinCos(ptankGlobal->cRotate,&sinA,&cosA);

        /* Calculate world space size vectors */
        right->x = (+ cosA) * rightCamera.x + (- sinA) * upCamera.x;
        right->y = (+ cosA) * rightCamera.y + (- sinA) * upCamera.y;
        right->z = (+ cosA) * rightCamera.z + (- sinA) * upCamera.z;

        up->x = (+ sinA) * rightCamera.x + (+ cosA) * upCamera.x;
        up->y = (+ sinA) * rightCamera.y + (+ cosA) * upCamera.y;
        up->z = (+ sinA) * rightCamera.z + (+ cosA) * upCamera.z;
    }
    else
    {
        right->x = -camLTM->right.x;
        right->y = -camLTM->right.y;
        right->z = -camLTM->right.z;

        up->x = camLTM->up.x;
        up->y = camLTM->up.y;
        up->z = camLTM->up.z;
    }

    /* What do we instance ?? */

    data->instanceColors =
    ((instFlags & rpPTANKIFLAGCOLOR )       == rpPTANKIFLAGCOLOR ) ||
    ((instFlags & rpPTANKIFLAGCNSCOLOR )    == rpPTANKIFLAGCNSCOLOR ) ||
    ((instFlags & rpPTANKIFLAGVTXCOLOR )    == rpPTANKIFLAGVTXCOLOR ) ||
    ((instFlags & rpPTANKIFLAGCNSVTXCOLOR ) == rpPTANKIFLAGCNSVTXCOLOR );

    data->instanceUVs =
    ((instFlags & rpPTANKIFLAGVTX2TEXCOORDS )    == rpPTANKIFLAGVTX2TEXCOORDS ) ||
    ((instFlags & rpPTANKIFLAGVTX4TEXCOORDS )    == rpPTANKIFLAGVTX4TEXCOORDS ) ||
    ((instFlags & rpPTANKIFLAGCNSVTX2TEXCOORDS ) == rpPTANKIFLAGCNSVTX2TEXCOORDS ) ||
    ((instFlags & rpPTANKIFLAGCNSVTX4TEXCOORDS ) == rpPTANKIFLAGCNSVTX4TEXCOORDS );


    data->instancePositions = TRUE;

    data->instanceNormals =
                ((instFlags & rpPTANKIFLAGNORMAL ) == rpPTANKIFLAGNORMAL );

    morphTarget = RpGeometryGetMorphTarget(geom, 0);


    if( data->instancePositions )
    {
        RpGeometryLock(geom, rpGEOMETRYLOCKVERTICES );
        data->positionOut.data = (RwUInt8 *)
            RpMorphTargetGetVertices(morphTarget);
        data->positionOut.stride = sizeof(RwV3d);
    }

    if( data->instanceColors )
    {
        RpGeometryLock(geom, rpGEOMETRYLOCKPRELIGHT  );
        data->colorsOut.data = (RwUInt8 *)
            RpGeometryGetPreLightColors(geom);
        data->colorsOut.stride = sizeof(RwRGBA);
    }

    if( data->instanceUVs )
    {
            RpGeometryLock(geom, rpGEOMETRYLOCKTEXCOORDS );
            data->UVOut.data = (RwUInt8 *)RpGeometryGetVertexTexCoords
                    (geom, rwTEXTURECOORDINATEINDEX0);
            data->UVOut.stride = sizeof(RwTexCoords);
    }

    if( data->instanceNormals )
    {
        RpGeometryLock(geom, rpGEOMETRYLOCKNORMALS );
        data->normalsOut.data = (RwUInt8 *)
            RpMorphTargetGetVertexNormals(morphTarget);
        data->normalsOut.stride =  sizeof(RwV3d);
    }

    RWRETURNVOID();
}

void
rpPTankGENInsEnding(RpPTANKInstanceSetupData *data __RWUNUSED__,
                RpAtomic *atomic,
                RpPTankData *ptankGlobal __RWUNUSED__,
                RwInt32 actPCount __RWUNUSED__,
                RwUInt32 instFlags __RWUNUSED__)
{
    RWFUNCTION(RWSTRING("rpPTankGENInsEnding"));

    RpGeometryUnlock(RpAtomicGetGeometry(atomic));

    RWRETURNVOID();
}


RwBool
_rpPTankGENRenderCallBack(RpAtomic *atomic,
                          RpPTankData *ptankGlobal,
                          RwInt32 actPCount __RWUNUSED__)
{
    RwUInt32    srcBlend;
    RwUInt32    dstBlend;

    RWFUNCTION(RWSTRING("_rpPTankGENRenderCallBack"));
    RWASSERT(atomic);
    RWASSERT(ptankGlobal);


    RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void *)&dstBlend);
    RwRenderStateGet(rwRENDERSTATESRCBLEND, (void *)&srcBlend );

    if( ptankGlobal->vertexAlphaBlend )
    {
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)ptankGlobal->dstBlend);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)ptankGlobal->srcBlend);

        RPATOMICPTANKPLUGINDATA(atomic)->defaultRenderCB(atomic);

        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)dstBlend);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)srcBlend );

        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}

