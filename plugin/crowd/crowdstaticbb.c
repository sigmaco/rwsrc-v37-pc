/*
 *  crowdstaticbb.c
 *
 *  Static billboard geometry system for RpCrowd, used by generic
 *  implementation.
 *
 *  Crowd geometry
 *  --------------
 *
 *  When a crowd is created, we take the base geometry giving the trapezoid
 *  sections of crowd (this must have been preprocessed into the quad format)
 *  and create crowd rows (billboards) in each section, storing this in a
 *  new geometry. This is held be the crowd->atomic which can be rendered
 *  in the normal way.
 *
 *  Each row of the crowd starts at a random column in the crowd texture and
 *  holds an integer number of columns (people).
 *
 *  Animation support
 *  -----------------
 *
 *  The crowd texture holds a certain number of animation frames vertically.
 *  We split this texture into multiple textures holding each frame so that
 *  we can easily switch frames in each row of the crowd by just swapping
 *  textures, and not having to alter UV coordinates.
 *
 *  The crowd sequences matrix holds a maximum of 16 frames for each of 4
 *  sequences. Each entry specifies the current animation frame to be used.
 *  To be able to assign any crowd row to a random sequence and
 *  initialised to a random point in that sequence, we need 64 materials.
 *  Each material initially corresponds to a position in the sequences
 *  matrix, and has a corresponding texture applied. As we cycle through the
 *  animation, each material gets cycled through its sequence. For each
 *  frame, the appropriate split texture component is applied.
 *
 *  In practice, we just need 4*N materials, where N is the number of
 *  animation frames that are simultanously referenced by the geometry. We
 *  can still cycle through all 16 frames for each material, the effect is
 *  just less random.
 *
 *  Note that N may sometimes be greater than the actual number of frames
 *  in the sequences. In this case we just double up some of the materials
 *  to tap into the same point in the animation.
 */

/******************************************************************************
 *  Includes
 */

#include "rwcore.h"
#include "rprandom.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rpcrowd.h"
#include "crowdcommon.h"
#include "crowdstaticbb.h"

/******************************************************************************
 *  Defines
 */

/*
 *  Define CROWDJAGGY to allow jagged edges on aisles, otherwise the rows
 *  are neatly aligned in a section. If defined, then CROWDSTAGGER sets
 *  the shift between alternate rows. A value of 0.25 shifts even rows by
 *  1/4 person to the right and odd rows by 1/4 person to the left.
 */
#define CROWDJAGGYx
#define CROWDSTAGGER ((RwReal)0.1)

/******************************************************************************
 *  Macros
 */

/* Linear interpolation */
#define LINTERP(_a,_b,_f) ((_a) + (_f)*((_b)-(_a)))

/* Vector interpolation */
#define VINTERP(_o,_a,_b,_f)                    \
MACRO_START                                     \
{                                               \
    (_o)->x = LINTERP((_a)->x, (_b)->x, (_f));  \
    (_o)->y = LINTERP((_a)->y, (_b)->y, (_f));  \
    (_o)->z = LINTERP((_a)->z, (_b)->z, (_f));  \
}                                               \
MACRO_STOP

/* Prelight interpolation */
#define U8INTERP(_o,_a,_b,_f)                                         \
MACRO_START                                                           \
{                                                                     \
    const RwInt32     temp =                                          \
        RwInt32FromRealMacro(_a) +                                         \
        RwInt32FromRealMacro((_f)*((RwReal)(_b) - (RwReal)(_a)));          \
    (_o) = (temp > 255) ? 255 : ((temp < 0) ? 0 : (RwUInt8)temp);     \
}                                                                     \
MACRO_STOP

#define RGBAINTERP(_o,_a,_b,_f)                         \
MACRO_START                                             \
{                                                       \
    U8INTERP((_o)->red,(_a)->red,(_b)->red,(_f));       \
    U8INTERP((_o)->green,(_a)->green,(_b)->green,(_f)); \
    U8INTERP((_o)->blue,(_a)->blue,(_b)->blue,(_f));    \
    U8INTERP((_o)->alpha,(_a)->alpha,(_b)->alpha,(_f)); \
}                                                       \
MACRO_STOP

/******************************************************************************
 */
static RpCrowdTexture **
CreateSplitTexture(RpCrowdTexture *baseTexture, RwUInt32 numRows)
{
    RpCrowdTexture **texArray = (RpCrowdTexture **)NULL;
    RwImage     *baseImage, *rowImage;
    RwRaster    *baseRaster;
    RwUInt32    rasterWidth, rasterHeight, rasterDepth;
    RwUInt32    rowHeight, row;
    RwUInt8     *basePixelPtr;
    RwUInt32    imageDepth;
    RwInt32     rasFlags;
    RwInt32     miplevels;
    RwInt32     level;
    RwChar      baseName[32];
    RwChar      rowName[32];

    RWFUNCTION(RWSTRING("CreateSplitTexture"));
    RWASSERT(baseTexture);
    RWASSERT(numRows > 0);

    texArray = (RpCrowdTexture **)RwMalloc(sizeof(RpCrowdTexture *) * numRows,
                                       rwID_CROWDPPPLUGIN | rwMEMHINTDUR_EVENT);
    baseRaster = RwTextureGetRaster(baseTexture);
    rasterWidth = RwRasterGetWidth(baseRaster);
    rasterHeight = RwRasterGetHeight(baseRaster);
    rasterDepth = RwRasterGetDepth(baseRaster);

    rasFlags = ((baseRaster->cFlags | baseRaster->cType |
                (baseRaster->cFormat << 8)) & ~rwRASTERFORMATAUTOMIPMAP);

    rowHeight = ( rasterHeight / numRows );
    RWASSERT((rowHeight * numRows) == rasterHeight);

    switch (rasterDepth)
    {
        case 16:
        case 24:
            imageDepth = 32;
            break;

        default:
            imageDepth = rasterDepth;
            break;
    }

    /* Create textures */
    for (row=0; row < numRows; row++)
    {
        RwRaster *rowRaster = (RwRaster *)NULL;

        /* create a raster from the row image */
        rowRaster = RwRasterCreate(rasterWidth, rowHeight, rasterDepth,
                                   rasFlags);

        /* create the new texture for this row */
        texArray[row] = RwTextureCreate(rowRaster);

        /* create a name for the new texture */
        rwstrncpy(baseName, RwTextureGetName(baseTexture),
            rwTEXTUREBASENAMELENGTH-7);
        rwsprintf(rowName, "%s_row%2d", baseName, row);

        RwTextureSetName(texArray[row], rowName);

        (void)RwTextureSetFilterMode(texArray[row],
                                     RwTextureGetFilterMode(baseTexture));

        (void)RwTextureSetAddressingU(texArray[row],
                                      rwTEXTUREADDRESSWRAP);

        (void)RwTextureSetAddressingV(texArray[row],
                                      rwTEXTUREADDRESSWRAP);
    }

    /* Fill mip levels */
    miplevels = RwRasterGetNumLevels(baseRaster);

    for (level = 0; level < miplevels; level++)
    {
        baseImage = RwImageCreate(rasterWidth, rasterHeight,
                                  imageDepth);
        RwImageAllocatePixels(baseImage);

        RwRasterLock(baseRaster, level, rwRASTERLOCKREAD);

        RwImageSetFromRaster(baseImage, baseRaster);

        RwRasterUnlock(baseRaster);

        rowImage = RwImageCreate(rasterWidth, rowHeight, imageDepth);
        RwImageAllocatePixels(rowImage);

        /* hack to get a quick way to copy image sections */
        baseImage->height = rowHeight;
        basePixelPtr = baseImage->cpPixels;

        for (row=0; row < numRows; row++)
        {
            RwRaster *rowRaster = RwTextureGetRaster(texArray[row]);

            if (RwRasterGetNumLevels(rowRaster) > level)
            {
                /* build the next row image */
                baseImage->cpPixels = basePixelPtr +
                    (((row * rasterHeight) / numRows) * RwImageGetStride(baseImage));
                RwImageCopy(rowImage, baseImage);

                RwRasterLock(rowRaster, level,
                             rwRASTERLOCKWRITE | rwRASTERLOCKNOFETCH);

                RwRasterSetFromImage(rowRaster, rowImage);

                RwRasterUnlock(rowRaster);
            }
        }

        /* undo the hack */
        baseImage->cpPixels = basePixelPtr;
        baseImage->height = rasterHeight;

        RwImageDestroy(baseImage);
        RwImageDestroy(rowImage);

        /* Get size next mip level */
        rasterWidth /= 2;

        if (rasterWidth < 1)
        {
            rasterWidth = 1;
        }

        rasterHeight /= 2;

        if (rasterHeight < 1)
        {
            rasterHeight = 1;
        }

        rowHeight /= 2;

        if (rowHeight < 1)
        {
            rowHeight = 1;
        }
    }

    RWRETURN(texArray);
}

/******************************************************************************
 */
static RwBool
DestroySplitTexture(RpCrowdTexture **textureArray, RwUInt32 numRows)
{
    RwUInt32 i;

    RWFUNCTION(RWSTRING("DestroySplitTexture"));
    RWASSERT(textureArray);
    RWASSERT(numRows > 0);

    for (i=0; i < numRows; i++)
    {
        RWASSERT(textureArray[i]);
        RwTextureDestroy(textureArray[i]);
    }

    RwFree(textureArray);

    RWRETURN(TRUE);
}

/******************************************************************************
 */
static RpCrowdTexture ***
CreateSplitTextureArray(RpCrowdTexture **textures,
                        RwUInt32        numTextures,
                        RwUInt32        numRows)
{
    RwUInt32    size, i;
    RpCrowdTexture ***splitTextures;

    RWFUNCTION(RWSTRING("CreateSplitTextureArray"));
    RWASSERT(textures);
    RWASSERT(numTextures > 0);
    RWASSERT(numRows > 0);

    /* Create an array for the split textures */
    size = sizeof(RpCrowdTexture **) * numTextures;
    splitTextures = (RpCrowdTexture ***) RwMalloc(size,
                  rwID_CROWDPPPLUGIN | rwMEMHINTDUR_EVENT);
    if (!splitTextures)
    {
        RWERROR((E_RW_NOMEM, size));
        RWRETURN((RpCrowdTexture ***)NULL);
    }

    /* Split each texture */
    for (i=0; i < numTextures; i++)
    {
        splitTextures[i] = CreateSplitTexture(textures[i], numRows);
        if (!splitTextures[i])
        {
            RwUInt32    j;

            for (j=0; j<i; j++)
            {
                DestroySplitTexture(splitTextures[j], numRows);
            }

            RwFree(splitTextures);
            RWRETURN((RpCrowdTexture ***)NULL);
        }
    }

    RWRETURN(splitTextures);
}

/******************************************************************************
 */
static RwBool
DestroySplitTextureArray(RpCrowdTexture ***splitTextures,
                         RwUInt32       numTextures,
                         RwUInt32       numRows)
{
    RwUInt32    i;

    RWFUNCTION(RWSTRING("DestroySplitTextureArray"));
    RWASSERT(splitTextures);
    RWASSERT(numTextures > 0);
    RWASSERT(numRows > 0);

    /* Destroy split textures */
    for (i=0; i < numTextures; i++)
    {
        DestroySplitTexture(splitTextures[i], numRows);
    }

    RwFree(splitTextures);

    RWRETURN(TRUE);
}

/******************************************************************************
 */
RpMaterial **
_rpCrowdStaticBBCreateCrowdMaterials(RwUInt32 numMaterials)
{
    RwUInt32    size, i;
    RpMaterial  **materials;

    RWFUNCTION(RWSTRING("_rpCrowdStaticBBCreateCrowdMaterials"));
    RWASSERT(numMaterials > 0);

    /* Get array of material pointers */
    size = numMaterials * sizeof(RpMaterial *);
    materials = (RpMaterial **) RwMalloc(size,
        rwID_CROWDPPPLUGIN | rwMEMHINTDUR_EVENT);
    if (!materials)
    {
        RWERROR((E_RW_NOMEM, size));
        RWRETURN((RpMaterial **)NULL);
    }

    /* Create the materials themselves */
    for (i=0; i < numMaterials; i++)
    {
        materials[i] = RpMaterialCreate();
        if (!materials[i])
        {
            RwUInt32    j;

            for (j=0; j<i; j++)
            {
                RpMaterialDestroy(materials[j]);
            }

            RwFree(materials);
            RWRETURN((RpMaterial **)NULL);
        }
    }

    RWRETURN(materials);
}

/******************************************************************************
 */
static RwBool
DestroyCrowdMaterials(RpMaterial **materials, RwUInt32 numMaterials)
{
    RwUInt32    i;

    RWFUNCTION(RWSTRING("DestroyCrowdMaterials"));
    RWASSERT(materials);
    RWASSERT(numMaterials > 0);

    for (i=0; i < numMaterials; i++)
    {
        RWASSERT(materials[i]);
        RpMaterialDestroy(materials[i]);
    }

    RwFree(materials);

    RWRETURN(TRUE);
}

/******************************************************************************
 */
RwBool
_rpCrowdStaticBBAssignCrowdMaterials(RpMaterial   **materials,
                     RwUInt32       numMaterials,
                     RpGeometry    *geometry)
{
    RwInt32    i, j;
    RpTriangle  *triangle;

    RWFUNCTION(RWSTRING("_rpCrowdStaticBBAssignCrowdMaterials"));
    RWASSERT(materials);
    RWASSERT(geometry);

    triangle = geometry->triangles;

    /* Set random material for every pair of triangles (a quad) */
    for (i=0; (i+1) < geometry->numTriangles; i+=2)
    {
        j = (RpRandom() % numMaterials);

        RpGeometryTriangleSetMaterial(geometry, triangle, materials[j]);
        triangle++;

        RpGeometryTriangleSetMaterial(geometry, triangle, materials[j]);
        triangle++;
    }

    RWRETURN(TRUE);
}

/******************************************************************************
 */
typedef struct _CrowdQuad
{
    RwTexCoords     uvs[4];
    RwV3d           verts[4];
    RwRGBA          prelits[4];

}   CrowdQuad;

typedef struct _CrowdQuadParam
{
    RwUInt32    gridUdens;
    RwUInt32    gridVdens;
    RwUInt32    texUdens;

    RwV3d       extrusion;

    RwReal      gridUstep;
    RwReal      gridVstep;
    RwReal      texUstep;

}   CrowdQuadParam;

/******************************************************************************
 */
static RwUInt32
GetNumQuadBillboards(RwReal v1, RwReal  v2, CrowdQuadParam *param)
{
    RwReal      above, below;
    RwInt32     iv1, iv2;

    RWFUNCTION(RWSTRING("GetNumQuadBillboards"));

    above = (RwReal) RwCeil(v1 * param->gridVdens);
    iv1 = RwInt32FromRealMacro(above);

    below = (RwReal) RwFloor(v2 * param->gridVdens);
    iv2 = RwInt32FromRealMacro(below - 1);

    /* Always miss out last row */
    RWRETURN(1 + iv2 - iv1);
}

/******************************************************************************
 */
static RwBool
CreateQuadBillboards(RpGeometry        *geom,
                     RwUInt32          *fillpos,
                     CrowdQuad         *quad,
                     CrowdQuadParam    *param)
{
    RwUInt32    iVert = 2 * (*fillpos);
    RpTriangle  *tris = geom->triangles + (*fillpos);
    RwV3d       *verts = geom->morphTarget->verts + iVert;
    RwTexCoords *texCo = geom->texCoords[0] + iVert;
    RwRGBA      *preLit = geom->preLitLum + iVert;

    RwInt32     iv1;
    RwReal      frac, fracStep;
    RwInt32     numRows, iRow;
    RwBool      quadFlipped;

    RWFUNCTION(RWSTRING("CreateQuadBillboards"));

    /*
     *  We need to detect whether the section is flipped so that
     *  we can make the crowd face the right way when back-face
     *  culling is used.
     */
    {
        RwV3d       v01, v02, vTemp;

        RwV3dSub(&v01, &quad->verts[1], &quad->verts[0]);
        RwV3dSub(&v02, &quad->verts[2], &quad->verts[0]);
        RwV3dCrossProduct(&vTemp, &v02, &param->extrusion);

        quadFlipped = (RwV3dDotProduct(&vTemp, &v01) < (RwReal)0);
    }

    /* Find number of rows in the quad */
    {
        RwReal      above, below;
        RwInt32     iv2;

        above = (RwReal) RwCeil(quad->uvs[2].v * param->gridVdens);
        iv1 = RwInt32FromRealMacro(above);

        below = (RwReal) RwFloor(quad->uvs[0].v * param->gridVdens);
        iv2 = RwInt32FromRealMacro( below - 1);

        /* Always miss out last row */
        numRows = 1 + iv2 - iv1;
    }

    /* Set up fractional value for interpolation along V */
    {
        RwReal recipDeltaV = ((RwReal)1) / (quad->uvs[0].v - quad->uvs[2].v);

        frac = recipDeltaV * (iv1 * param->gridVstep - quad->uvs[2].v);
        fracStep = param->gridVstep * recipDeltaV;
    }

    /* Now construct two tris per billboard */
    for (iRow = 0; iRow < numRows; iRow++)
    {
        RwReal      u1, u2;
        RwUInt32    numPeople;

        /* Interpolate vertices for bottom of row */
        VINTERP(&verts[0], &quad->verts[2], &quad->verts[0], frac);
        VINTERP(&verts[1], &quad->verts[3], &quad->verts[1], frac);

        /* Interpolate texture U coordinates */
        u1 = LINTERP(quad->uvs[2].u, quad->uvs[0].u, frac);
        u2 = LINTERP(quad->uvs[3].u, quad->uvs[1].u, frac);

#ifdef CROWDJAGGY
        {
            RwReal      rowShift;
            RwInt32     iu1, iu2;
            RwV3d       rowVec;
            RwReal      recipDeltaU;
            RwReal      below;

            /* Stagger odd and even rows */
            rowShift = ((iv1 + iRow) & 1) ? CROWDSTAGGER : - CROWDSTAGGER;

            below = (RwReal)
                RwFloor(u1 * param->gridUdens + ((RwReal)0.5) - rowShift);
            iu1 = RwInt32FromRealMacro(below);

            below = (RwReal)
                RwFloor(u2 * param->gridUdens + ((RwReal)0.5) - rowShift);
            iu2 = RwInt32FromRealMacro(below);

            /* Shift start vertex */
            RwV3dSub(&rowVec, &verts[1], &verts[0]);
            recipDeltaU = ((RwReal)1) / (u2 - u1);
            RwV3dIncrementScaled(&verts[0], &rowVec,
                recipDeltaU * ((iu1 + rowShift) * param->gridUstep - u1));

            /* Shift end vertex */
            RwV3dIncrementScaled(&verts[1], &rowVec,
                recipDeltaU * ((iu2 + rowShift) * param->gridUstep - u2));

            /* Get number of people in row */
            numPeople = iu2 - iu1;
        }
#else  /* CROWDJAGGY */

        /* Get number of people in an aligned row */
        numPeople =
            RwInt32FromRealMacro((RwReal)(RwFloor((u2 - u1) *
                                             param->gridUdens + 0.5f)));

#endif /* CROWDJAGGY */

        /* Get vertices at top of row by shifting up bottom vertices */
        RwV3dAdd(&verts[2], &verts[0], &param->extrusion);
        RwV3dAdd(&verts[3], &verts[1], &param->extrusion);

        /* Set crowd texture coordinates at random offset */
        texCo[0].u = (RpRandom() % param->texUdens) * param->texUstep;
        texCo[1].u = texCo[0].u + numPeople * param->texUstep;
        texCo[2].u = texCo[0].u;
        texCo[3].u = texCo[1].u;
        texCo[0].v = texCo[1].v = 1.0f;
        texCo[2].v = texCo[3].v = 0.0f;

        /* Interpolate prelights */
        RGBAINTERP(&preLit[0], &quad->prelits[2], &quad->prelits[0], frac);
        RGBAINTERP(&preLit[1], &quad->prelits[3], &quad->prelits[1], frac);
        preLit[2] = preLit[0];
        preLit[3] = preLit[1];

        /* Fill geometry triangle indices */
        if (quadFlipped)
        {
            tris[0].vertIndex[0] = (RwUInt16) iVert;
            tris[0].vertIndex[1] = (RwUInt16) iVert + 2;
            tris[0].vertIndex[2] = (RwUInt16) iVert + 1;
            tris[1].vertIndex[0] = (RwUInt16) iVert + 3;
            tris[1].vertIndex[1] = (RwUInt16) iVert + 1;
            tris[1].vertIndex[2] = (RwUInt16) iVert + 2;
        }
        else
        {
            tris[0].vertIndex[0] = (RwUInt16) iVert;
            tris[0].vertIndex[1] = (RwUInt16) iVert + 1;
            tris[0].vertIndex[2] = (RwUInt16) iVert + 2;
            tris[1].vertIndex[0] = (RwUInt16) iVert + 3;
            tris[1].vertIndex[1] = (RwUInt16) iVert + 2;
            tris[1].vertIndex[2] = (RwUInt16) iVert + 1;
        }

        /* Added two triangles */
        tris += 2;
        iVert += 4;
        verts += 4;
        texCo += 4;
        preLit += 4;

        /* Next row */
        frac += fracStep;
    }

    /* Save new position in destination geometry */
    *fillpos += 2*numRows;

    RWRETURN(TRUE);
}

/******************************************************************************
 */
RpGeometry *
_rpCrowdStaticBBCreateCrowdGeometry(RpCrowd *crowd)
{
    /*
     * Creates crowd geometry, with materials unassigned, and in locked
     * state.
     */

    RwUInt32        numTris;
    RpGeometry     *newGeom;
    RwInt32         i;
    RwUInt32        fillpos;
    RpGeometry      *baseGeom;
    CrowdQuadParam     param;

    RWFUNCTION(RWSTRING("_rpCrowdStaticBBCreateCrowdGeometry"));
    RWASSERT(crowd);
    RWASSERT(crowd->param.base);

    /* Get extrusion vector, and transform to local space if necessary */
    RwV3dScale(&param.extrusion, &crowd->param.up, crowd->param.height);

    if (RpAtomicGetFrame(crowd->param.base))
    {
        RwMatrix    invLTM;
        RwMatrixInvert(&invLTM, RwFrameGetLTM(RpAtomicGetFrame(crowd->param.base)));
        RwV3dTransformVector(&param.extrusion, &param.extrusion, &invLTM);
    }

    /* Get the base quad geometry */
    baseGeom = RpAtomicGetGeometry(crowd->param.base);
    RWASSERT(baseGeom);
    RWASSERT(RpGeometryGetFlags(baseGeom) & rpGEOMETRYTEXTURED);

    /* Cache some handy values */
    param.gridUdens = crowd->param.densityU;
    param.gridVdens = crowd->param.densityV;
    param.texUdens  = crowd->param.numTexCols;
    RWASSERT(param.gridUdens > 0);
    RWASSERT(param.gridVdens > 0);
    RWASSERT(param.texUdens > 0);
    param.gridUstep = ((RwReal)1) / param.gridUdens;
    param.gridVstep = ((RwReal)1) / param.gridVdens;
    param.texUstep  =  ((RwReal)1) / param.texUdens;

    /* Get number of triangles for all quads in the crowd */
    numTris = 0;
    for (i=0; i < baseGeom->numTriangles; i += 2)
    {
        RwUInt32    i0 = baseGeom->triangles[i].vertIndex[0];
        RwUInt32    i2 = baseGeom->triangles[i].vertIndex[2];

        /* Two tris per billboard */
        numTris += 2 * GetNumQuadBillboards(baseGeom->texCoords[0][i2].v,
                                            baseGeom->texCoords[0][i0].v,
                                            &param);
    }

    /* Create the trilist crowd geometry */
    newGeom = RpGeometryCreate(numTris*2, numTris,
                rpGEOMETRYPOSITIONS | rpGEOMETRYTEXTURED | rpGEOMETRYPRELIT);
    if (!newGeom)
    {
        RWRETURN((RpGeometry *)NULL);
    }

    /* Now fill the vertices */
    fillpos = 0;
    for (i=0; i < baseGeom->numTriangles; i += 2)
    {
        static RwRGBA   opaqueWhite = {255, 255, 255, 255};
        RwUInt32        index, j;
        CrowdQuad            quad;

        /* Copy quad data from source geometry */
        for (j=0; j<3; j++)
        {
            index = baseGeom->triangles[i].vertIndex[j];
            quad.uvs[j] = baseGeom->texCoords[0][index];
            quad.verts[j] = baseGeom->morphTarget->verts[index];
            quad.prelits[j] = baseGeom->preLitLum ? baseGeom->preLitLum[index]
                                                 : opaqueWhite;
        }

        index = baseGeom->triangles[i+1].vertIndex[0];
        quad.uvs[3] = baseGeom->texCoords[0][index];
        quad.verts[3] = baseGeom->morphTarget->verts[index];
        quad.prelits[3] = baseGeom->preLitLum ? baseGeom->preLitLum[index]
                                             : opaqueWhite;

        /* Convert quads to billboards */
        CreateQuadBillboards(newGeom, &fillpos, &quad, &param);
    }

    /* Get the bounding sphere */
    {
        RwSphere    sphere;

        RpMorphTargetCalcBoundingSphere(newGeom->morphTarget, &sphere);
        (void)RpMorphTargetSetBoundingSphere(newGeom->morphTarget,
                                             &sphere);
    }

    RWRETURN(newGeom);
}

/******************************************************************************
 */
RpCrowd *
_rpCrowdStaticBBCreate(RpCrowd       *crowd,
                     rpCrowdStaticBB *staticBB,
                     RwUInt32       numMatPerSeq)
{
    RwUInt32    numMaterials;
    RpGeometry *crowdGeom;
    RwFrame    *frame, *baseFrame;

    RWFUNCTION(RWSTRING("_rpCrowdStaticBBCreate"));
    RWASSERT(crowd);

    /* Initialize */
    staticBB->splitTextures = (RpCrowdTexture ***)NULL;
    staticBB->materials = (RpMaterial **)NULL;
    staticBB->atomic = (RpAtomic *)NULL;
    staticBB->numMatPerSeq = numMatPerSeq;
    numMaterials = numMatPerSeq * rpCROWDANIMATIONNUMSEQ;

    /* Create split textures for animation cycling */
    staticBB->splitTextures = CreateSplitTextureArray(crowd->param.textures,
                                                    crowd->param.numTextures,
                                                    crowd->param.numTexRows);
    if (!staticBB->splitTextures)
    {
        RWRETURN((RpCrowd *)NULL);
    }

    /* Now remove references to original textures which we no longer need */
    {
        RwUInt32    i;

        for (i=0; i < crowd->param.numTextures; i++)
        {
            RwTextureDestroy(crowd->param.textures[i]);
            crowd->param.textures[i] = (RpCrowdTexture *) NULL;
        }
    }

    /* Create material array for animation cyling */
    staticBB->materials = _rpCrowdStaticBBCreateCrowdMaterials(numMaterials);
    if (!staticBB->materials)
    {
        _rpCrowdStaticBBDestroy(crowd, staticBB);
        RWRETURN((RpCrowd *)NULL);
    }

    /* Create the crowd atomic and geometry */
    staticBB->atomic = RpAtomicCreate();
    if (!staticBB->atomic)
    {
        _rpCrowdStaticBBDestroy(crowd, staticBB);
        RWRETURN((RpCrowd *)NULL);
    }

    crowdGeom = _rpCrowdStaticBBCreateCrowdGeometry(crowd);
    if (!crowdGeom)
    {
        _rpCrowdStaticBBDestroy(crowd, staticBB);
        RWRETURN((RpCrowd *)NULL);
    }

    /* Randomly assign materials to the billboards */
    _rpCrowdStaticBBAssignCrowdMaterials(staticBB->materials, numMaterials, crowdGeom);

    /* Now that materials have been assigned we can unlock */
    RpGeometryUnlock(crowdGeom);

    /* New billboard atomic needs its own frame */
    frame = RwFrameCreate();
    if (!frame)
    {
        RpGeometryDestroy(crowdGeom);
        _rpCrowdStaticBBDestroy(crowd, staticBB);
        RWRETURN((RpCrowd *)NULL);
    }

    /* Copy the LTM from the base geometry */
    RWASSERT(crowd->param.base);
    baseFrame = RpAtomicGetFrame(crowd->param.base);
    if (baseFrame)
    {
        *RwFrameGetMatrix(frame) = *RwFrameGetLTM(baseFrame);
    }
    else
    {
        RwMatrixSetIdentity(RwFrameGetMatrix(frame));
    }

    /* Setup atomic */
    RwFrameUpdateObjects(frame);
    RpAtomicSetFrame(staticBB->atomic, frame);
    RpAtomicSetGeometry(staticBB->atomic, crowdGeom, 0);
    RpGeometryDestroy(crowdGeom);

    RWRETURN(crowd);
}

/******************************************************************************
 */
RwBool
_rpCrowdStaticBBDestroy(RpCrowd *crowd, rpCrowdStaticBB *staticBB)
{
    RWFUNCTION(RWSTRING("_rpCrowdStaticBBDestroy"));
    RWASSERT(staticBB);

    /* Destroy the atomic (also destroys geometry) */
    if (staticBB->atomic)
    {
        RwFrame    *frame;

        frame = RpAtomicGetFrame(staticBB->atomic);
        if (frame)
        {
            RpAtomicSetFrame(staticBB->atomic, (RwFrame *)NULL);
            RwFrameDestroy(frame);
        }

        RpAtomicDestroy(staticBB->atomic);
        staticBB->atomic = (RpAtomic *) NULL;
    }

    /* Material array */
    if (staticBB->materials)
    {
        DestroyCrowdMaterials(staticBB->materials,
            staticBB->numMatPerSeq * rpCROWDANIMATIONNUMSEQ);
        staticBB->materials = (RpMaterial **) NULL;
    }

    /* Split texture array */
    if (staticBB->splitTextures)
    {
        /* Destroy split textures */
        DestroySplitTextureArray(staticBB->splitTextures,
                                 crowd->param.numTextures,
                                 crowd->param.numTexRows);
        staticBB->splitTextures = (RpCrowdTexture ***) NULL;
    }

    RWRETURN(TRUE);
}

/******************************************************************************
 */
RpCrowd *
_rpCrowdStaticBBSetTextures(RpCrowd *crowd, rpCrowdStaticBB *staticBB)
{
    RwReal              mapping;
    RwUInt32             cycle, i;
    RpCrowdAnimation   *anim;
    RpCrowdTexture    **splitTexture;

    RWFUNCTION(RWSTRING("_rpCrowdStaticBBSetTextures"));
    RWASSERT(crowd);
    RWASSERT(staticBB);

    /* Get current animation */
    RWASSERT(crowd->param.anims);
    RWASSERT(crowd->currAnim < crowd->param.numAnims);
    anim = crowd->param.anims[crowd->currAnim];
    RWASSERT(anim);

    /* Get current textures */
    RWASSERT(staticBB->splitTextures);
    RWASSERT(crowd->currTexture < crowd->param.numTextures);
    splitTexture = staticBB->splitTextures[crowd->currTexture];
    RWASSERT(splitTexture);

    /* Find the cycle number within the animation sequence */
    RWASSERT(crowd->currAnimTime >= 0.0f);
    RWASSERT(crowd->currAnimTime <= anim->duration);
    cycle = RwInt32FromRealMacro((crowd->currAnimTime / anim->duration) *
                            anim->numSteps);

    /* Set current textures on all the materials */
    mapping = (RwReal)anim->numSteps / (RwReal)staticBB->numMatPerSeq;

    for (i=0; i < staticBB->numMatPerSeq; i++)
    {
        RwInt32 step, j;

        /* Map to frame index in sequences array */
        step = RwInt32FromRealMacro(i * mapping) + cycle;
        if (step >= anim->numSteps)
        {
            step -= anim->numSteps;
        }

        /* Set material textures for current step of each sequence */
        for (j=0; j < rpCROWDANIMATIONNUMSEQ; j++)
        {
            RwInt32     iMat, iRow;

            iMat = i * rpCROWDANIMATIONNUMSEQ + j;
            iRow = anim->sequences[step][j];
            RpMaterialSetTexture(staticBB->materials[iMat], splitTexture[iRow]);
        }
    }

    RWRETURN(crowd);
}

