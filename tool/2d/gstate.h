
/***************************************************************************
 *                                                                         *
 * Module  : gstate.h                                                      *
 *                                                                         *
 * Purpose :                                                               *
 *                                                                         *
 **************************************************************************/

#ifndef GSTATE_H
#define GSTATE_H

/****************************************************************************
 Includes
 */

/****************************************************************************
 Defines
 */
#define MAXVINDEX (VERTEXCACHESIZE/4 * 6)

#define RT2DPS2MAXVERT      64
#define RT2DMAXCOLOR        64
#define RT2DMAXCTM          32

#define RT2DPS2STRCACHEMAX  16

#define RT2DSTRBUFFER       1024

#define RT2DPS2COLSCALE     ((RwReal) 128.1 / (RwReal) 255.0)


#define RT2D_STREAM_UNKNOWN         0
#define RT2D_STREAM_FONT            1
#define RT2D_STREAM_FONT_BEGIN      2
#define RT2D_STREAM_FONT_END        3
#define RT2D_STREAM_PATH            4
#define RT2D_STREAM_PATH_BEGIN      5
#define RT2D_STREAM_PATH_END        6
#define RT2D_STREAM_PATH_MOVETO     7
#define RT2D_STREAM_PATH_LINETO     8
#define RT2D_STREAM_PATH_CURVETO    9


#define _rt2dMarkerEncode(_code, _ver, _marker) \
    ((_code) = (((_ver) << 24) | ((_marker))))

#define _rt2dMarkerDecode(_code, _ver, _marker) \
MACRO_START \
{ \
    (_ver) = (((_code) & 0xFF000000) >> 24); \
    (_marker) = ((_code) & 0x00FFFFFF); \
} \
MACRO_STOP

/****************************************************************************
 Global Types
 */
#if (defined(SKY2_DRVMODEL_H))
typedef union _Rt2dUnion128 Rt2dUnion128;
union _Rt2dUnion128
{
    RwReal      float32[4];
    RwInt32     int32[4];
    RwUInt128   int128;
};
#endif /* (defined(SKY2_DRVMODEL_H)) */

typedef struct Rt2dStrBuffer Rt2dStrBuffer;
struct Rt2dStrBuffer
{
    RwInt32             space;
    Rt2dFontChar       *str;
    Rt2dFontChar        buffer[RT2DSTRBUFFER];
};

/* String cache for string. */
typedef struct Rt2dPS2StrCache RWALIGN(Rt2dPS2StrCache, rwMATRIXALIGNMENT);

typedef struct Rt2dGlobalVars Rt2dGlobalVars;
struct Rt2dGlobalVars
{
    RwImVertexIndex     topo[VERTEXCACHESIZE];
    RwImVertexIndex     fonttopo[MAXVINDEX];

    RwFreeList         *brushFreeList;
    RwIm3DVertex       *brushVertexCache;

    RwFreeList         *fontFreeList;
    RwChar             *fontPath;
    RwUInt32           fontPathSize;
    RwFreeList         *fontDictNodeFreeList;
    _rt2dFontDictionaryNode *fontDictionary;
    RwTexDictionary    *fontTexDictionary;
    RwTexDictionary    *currFontTexDictionary;

    RwFreeList         *pathFreeList;
    Rt2dPath           *scratch;

    RwFreeList         *objectFreeList;

    RwFreeList         *fillFreeList;

    RwFreeList         *triVertFreeList;
    RwFreeList         *triPolyFreeList;
    _rt2dTriIndex       triVertIndex;

    RwMatrix            metricMtx;
    RwMatrix           *ctm[RT2DMAXCTM];
    RwInt32             mtos;

    RwV2d               vw;
    RwV2d               vp;

    RwMatrix            CTM2d, iCTM2d;
    RwMatrix            CTM;
    RwBool              CTMValid;
    RwUInt32            TransformFlags;

    RwReal              layerDepth;
    RwReal              tolerance;
    RwReal              sqTolerance;
    RwBool              sqToleranceValid;
    RwInt32             flatDepth;

    RwInt32             extensionOffset;

    RwReal              deviceX;
    RwReal              deviceY;
    RwReal              deviceW;
    RwReal              deviceH;

    RwBool              pixelSpace;

    RwCamera           *viewCamera;

    /* Pipeline Stuff. */

    Rt2dBrush          *brush;
    Rt2dFont           *font;
    Rt2dPath           *path;
    Rt2dFontChar       *str;

    rt2dPathNode       *pnode0;
    rt2dPathNode       *pnode1;

    RwReal              pathInset;
    RwReal              oobaseu;
    RwReal              fontbaseu;

    Rt2dBBox            bbox;
    RwV2d               delta;

    RwInt32             numVerts, numBatches;

    RwCameraBeginUpdateFunc cameraBeginUpdate;
    RwCameraEndUpdateFunc cameraEndUpdate;

    RxClusterDefinition RxClPS2FillPos;
    RxClusterDefinition RxClPS2FillNrm;

    RxClusterDefinition RxClPS2FontUV1;
    RxClusterDefinition RxClPS2FontUV2;

    RxClusterDefinition RxClPS2StrokePos;
    RxClusterDefinition RxClPS2StrokeNrm;

    RxPipeline         *default_pipe, *use_pipe;

    RxPipeline         *default_stroke_pipe, *use_stroke_pipe;
    RxPipeline         *default_font_pipe, *use_font_pipe;
    RxPipeline         *default_fill_pipe, *use_fill_pipe;
    RxPipeline         *default_sgfont_pipe, *use_sgfont_pipe;

    RwInt32             fill_batch_sz, font_batch_sz, stroke_batch_sz,
                        sgfont_batch_sz;

    Rt2dStrBuffer       strBuffer;

    Rt2dFont           *fontBatch;

    Rt2dFontCallBackRead
                        fontReadFunc;

    RwRGBA              colorCache[RT2DMAXCOLOR];
    RwUInt32           *triIdxCache;
    RwUInt32            triIdxCacheSize;

    /* String funcs for unicode operations */
    RwChar              filename[80];
    RwInt32            *uCharIndex;
    RwInt32             uCharIndexSize;
};

typedef struct Rt2dInstanceData Rt2dInstanceData;
struct Rt2dInstanceData
{
    RwMatrix           *matrix;
    RwIm3DVertex       *vertexData;
    RwImVertexIndex    *indexData;
    RwUInt32            vertexCount;
    RwUInt32            indexCount;
    RwUInt32            vertexStride;
    RwUInt32            indexStride;

    RwPrimitiveType     primType;
    RwUInt32            flag;
};

typedef struct Rt2dInstanceObject Rt2dInstanceObject;
struct Rt2dInstanceObject
{
    Rt2dBrush          *brush;
    Rt2dPath           *path;
    Rt2dFont           *font;
    RwChar             *string;
    RwMatrix           *ctm;
    RwPrimitiveType     primType;
    RwUInt32            flag;
};

extern Rt2dGlobalVars Rt2dGlobals;

/****************************************************************************
 Function prototypes
 */

extern RwMatrix    *_rt2dCTMGetInverse(void);
extern RwBool       _rt2dCTMGetWinding(void);
extern RwReal       _rt2dGetTolerance(void);

/* init */
extern RwBool       _rt2dGstateOpen(void);
extern void         _rt2dGstateClose(void);

#endif /* GSTATE_H */
