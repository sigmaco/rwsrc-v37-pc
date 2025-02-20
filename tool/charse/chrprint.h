
/***************************************************************************
 *                                                                         *
 * Module  : chrprint.h                                                    *
 *                                                                         *
 * Purpose : Charset handling                                              *
 *                                                                         *
 **************************************************************************/

#ifndef CHRPRINT_H
#define CHRPRINT_H

struct _RtCharsetBuffer
{
    RwBool              initialised;
    RtCharset          *charSet;
    RwUInt32            numChars;
    RwIm2DVertex       *vertices;
    RwImVertexIndex    *indices;
};
typedef struct _RtCharsetBuffer RtCharsetBuffer;

extern RtCharsetBuffer _rtgBuffer;

#define BUFFERNUMCHARS              ( 1024 )
#define CHARSET_TEXEL_BORDER_SIZE   ( 1 )

#define BASE (' ')

/****************************************************************************
 Includes
 */

/****************************************************************************
 Function prototypes
 */

/* RWPUBLIC */

#ifdef    __cplusplus
extern              "C"
{
#endif                          /* __cplusplus */

#if (defined(_IBM_CHAR))

/* Size of characters */

#define rtIBMCHARSETHEIGHT ( 9 )
#define rtIBMCHARSETWIDTH ( 8 )

typedef RwUInt8     RtGlyphRow;
typedef RtGlyphRow  RtIBMGlyph[rtIBMCHARSETHEIGHT + CHARSET_TEXEL_BORDER_SIZE];

extern const RtGlyphRow *_rtCharsetIBMFontGet(RtCharsetDesc * desc,
                                              RwInt32 * rasSize);

#define GETCHARSETBITS(_desc_ptr, rasSizePtr) \
_rtCharsetIBMFontGet((_desc_ptr), (rasSizePtr))

#elif (defined(_CEE_FAX_CHAR))

/* Size of characters */

#define rtCEEFAXCHARSETHEIGHT ( 24 )
#define rtCEEFAXCHARSETWIDTH ( 20 )

typedef RwUInt32    RtGlyphRow;
typedef RtGlyphRow  RtCeeFaxGlyph[rtCEEFAXCHARSETHEIGHT + CHARSET_TEXEL_BORDER_SIZE];

extern const RtGlyphRow *_rtCharsetCeeFaxFontGet(RtCharsetDesc * desc,
                                                 RwInt32 * rasSize);

#define GETCHARSETBITS(_desc_ptr, rasSizePtr) \
_rtCharsetCeeFaxFontGet((_desc_ptr), (rasSizePtr))

#else                           /* (defined(_IBM_CHAR)) */

/* Size of characters */

#define rtATARICHARSETHEIGHT ( 14 )
#define rtATARICHARSETWIDTH ( 8 )

typedef RwUInt8     RtGlyphRow;
typedef RtGlyphRow  RtAtariGlyph[rtATARICHARSETHEIGHT + CHARSET_TEXEL_BORDER_SIZE];

extern const RtGlyphRow *_rtCharsetAtariFontGet(RtCharsetDesc * desc,
                                                RwInt32 * rasSize);

#define GETCHARSETBITS(_desc_ptr, rasSizePtr) \
_rtCharsetAtariFontGet((_desc_ptr), (rasSizePtr))

#endif                          /* (defined(_IBM_CHAR)) */

#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

/* RWPUBLICEND */

#endif                          /* CHRPRINT_H */
