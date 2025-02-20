/**************************************************************************
 *                                                                        *
 * Module  : pen.h                                                        *
 *                                                                        *
 * Purpose :                                                              *
 *                                                                        *
 **************************************************************************/

#ifndef FONT_H
#define FONT_H

/****************************************************************************
 Includes
 */

/****************************************************************************
 Defines
 */
#define rtFONTVERSION           1

#define rtFONTMAXPAGES          4
#define rtFONTCHARCOUNT         (1<<(sizeof(RwChar)<<3))
#define rtFONTCHARBATCH         32

#define rtFONTACHARINDEXSIZE    128

#define rtFONTFLAG_SINGLEPAGE           0x01
#define rtFONTFLAG_UNICODE              0x02


#define FontCharGetCode(_code, _c, _font) \
MACRO_START \
    if ((_c) >= rtFONTACHARINDEXSIZE) \
    { \
        (_code) = (_c) - (_font)->uCharOffset; \
        if (((_code) >= 0) && ((_code) < (_font)->uCharIndexCount)) \
        { \
            (_code) = (RwInt32) (_font)->uCharIndex[(_code)]; \
        } \
        else \
        { \
            (_code) = -1; \
        } \
    } \
    else \
    { \
        (_code) = (RwInt32) (_font)->aCharIndex[(_c)]; \
    } \
MACRO_STOP

#ifdef rwBIGENDIAN

#define FontStrGetChar(_c, _s, _m, _i, _o1, _o2) \
    (_c) = ((RwUInt32) (((_s)[0] << (_o1)) | (((_s)[(_i)] & (_m)) << (_o2))))


#else /* rwBIGENDIAN */

#define FontStrGetChar(_c, _s, _m, _i, _o1, o2) \
    (_c) = ((RwUInt32) ((_s)[0] | (((_s)[(_i)] & (_m)) << 8)))

#endif /* rwBIGENDIAN */

/****************************************************************************
 Global Types
 */

/*
 * Font rendering callback
 */
typedef Rt2dFont   *(*Rt2dFontShowCallBack)
    (Rt2dFont * font, const RwChar * string,
     RwReal height, RwV2d * anchor, Rt2dBrush * brush);

typedef struct rt2dCharRect rt2dCharRect;
struct rt2dCharRect
{
    RwReal              width;
    RwTexture          *tex;
    RwV2d               uv[2];
    Rt2dPath           *path;
    RwInt8              charpage;
};

struct Rt2dFont
{
    RwBool              isOutline;
    RwReal              height, baseline;
    RwReal              intergap;

    RwUInt32            flag;

    RwTexture          *textures[rtFONTMAXPAGES];

    RwInt16             aCharIndex[rtFONTACHARINDEXSIZE];

    RwInt32             uCharOffset;
    RwInt32             uCharIndexCount;
    RwInt16            *uCharIndex;

    RwInt32             charCount;
    RwSList            *charMap;

    Rt2dFontShowCallBack fontshowCB;

    Rt2dFont           *nextBatch;
    void               *strCache;
};

struct _rt2dFontDictionaryNode
{
    RwChar *name;
    Rt2dFont *font;
    _rt2dFontDictionaryNode *next;
};

/* Force to unsigned char, assuming wchar_t is also unsigned */

#ifdef RWUNICODE
typedef wchar_t     Rt2dFontChar;
#else /* RWUNICODE */
typedef unsigned char Rt2dFontChar;
#endif /* RWUNICODE */

/****************************************************************************
 Function prototypes
 */
extern Rt2dFont    *_rt2dFontGetStringInfo(Rt2dFont * font,
                                           const RwChar * string,
                                           RwReal * width, RwInt32 * l);

extern RwBool       _rt2dFontOpen(void);

extern void         _rt2dFontClose(void);

extern _rt2dFontDictionaryNode *
_rt2dFontDictionaryLookup(const RwChar* fontName);

#endif /* FONT_H */

