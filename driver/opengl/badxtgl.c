/***************************************************************************
 *                                                                         *
 * Module  : badxtgl.c                                                     *
 *                                                                         *
 * Purpose : OpenGL DXT decompression support                              *
 *                                                                         *
 **************************************************************************/

/****************************************************************************
 Includes
 */
#include "barwtyp.h"

#include "badxtgl.h"


/****************************************************************************
 Local types
 */

typedef struct DXTColBlock DXTColBlock;
struct DXTColBlock
{
    RwUInt16 col0;
    RwUInt16 col1;

    /* no bit fields - use bytes*/
    RwUInt8 row[4];
};

typedef struct DXTAlphaBlockExplicit DXTAlphaBlockExplicit;
struct DXTAlphaBlockExplicit
{
    RwUInt16 row[4];
};

typedef struct DXTAlphaBlock3BitLinear DXTAlphaBlock3BitLinear;
struct DXTAlphaBlock3BitLinear
{
    RwUInt8 alpha0;
    RwUInt8 alpha1;

    RwUInt8 stuff[6];
};

typedef struct Color8888 Color8888;
struct Color8888
{
    RwUInt8 r;      /* change the order of names to change the */
    RwUInt8 g;      /*  order of the output ARGB or BGRA, etc...*/
    RwUInt8 b;      /*  Last one is MSB, 1st is LSB.*/
    RwUInt8 a;
};

typedef struct Color565 Color565;
struct Color565
{
    unsigned nBlue  : 5;        /* order of names changes*/
    unsigned nGreen : 6;        /*  byte order of output to 32 bit */
    unsigned nRed   : 5;
};


/* -------------------------------------------------------------------------
 *  Function: GetColorBlockColorsDXT1
 *
 *  Purpose : 
 *
 *  On entry: 
 *
 *  On exit : 
 * ------------------------------------------------------------------------- */
static __inline RwBool
GetColorBlockColorsDXT1( const DXTColBlock *pBlock,
                         Color8888 *col_0,
                         Color8888 *col_1,
                         Color8888 *col_2,
                         Color8888 *col_3,
                         RwUInt16 *wrd)
{
    RwBool  hasAlpha = FALSE;

    /* There are 4 methods to use - see the Time_ functions. */
    /* 1st = shift = does normal approach per byte for color comps */
    /* 2nd = use freak variable bit field color565 for component extraction */
    /* 3rd = use super-freak RwUInt32 adds BEFORE shifting the color components */
    /*  This lets you do only 1 add per color instead of 3 RwUInt8 adds and */
    /*  might be faster */
    /* Call RunTimingSession() to run each of them & output result to txt file */

    /* freak variable bit structure method */
    /* normal math */
    /* This method is fastest */
    const Color565 *pCol;

    RWFUNCTION( RWSTRING( "GetColorBlockColorsDXT1" ) );

    pCol = (const Color565 *) & (pBlock->col0 );

    col_0->a = 0xff;
    col_0->r = pCol->nRed;
    col_0->r <<= 3;             /* shift to full precision */
    col_0->g = pCol->nGreen;
    col_0->g <<= 2;
    col_0->b = pCol->nBlue;
    col_0->b <<= 3;

    pCol = (const Color565 *) & (pBlock->col1 );
    col_1->a = 0xff;
    col_1->r = pCol->nRed;
    col_1->r <<= 3;             /* shift to full precision */
    col_1->g = pCol->nGreen;
    col_1->g <<= 2;
    col_1->b = pCol->nBlue;
    col_1->b <<= 3;


    if( pBlock->col0 > pBlock->col1 )
    {
        /* Four-color block: derive the other two colors.     */
        /* 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3 */
        /* These two bit codes correspond to the 2-bit fields  */
        /* stored in the 64-bit block. */

        *wrd = ((RwUInt16)col_0->r * 2 + (RwUInt16)col_1->r )/3;
                                            /* no +1 for rounding */
                                            /* as bits have been shifted to 888 */
        col_2->r = (RwUInt8)*wrd;

        *wrd = ((RwUInt16)col_0->g * 2 + (RwUInt16)col_1->g )/3;
        col_2->g = (RwUInt8)*wrd;

        *wrd = ((RwUInt16)col_0->b * 2 + (RwUInt16)col_1->b )/3;
        col_2->b = (RwUInt8)*wrd;
        col_2->a = 0xff;

        *wrd = ((RwUInt16)col_0->r + (RwUInt16)col_1->r *2 )/3;
        col_3->r = (RwUInt8)*wrd;

        *wrd = ((RwUInt16)col_0->g + (RwUInt16)col_1->g *2 )/3;
        col_3->g = (RwUInt8)*wrd;

        *wrd = ((RwUInt16)col_0->b + (RwUInt16)col_1->b *2 )/3;
        col_3->b = (RwUInt8)*wrd;
        col_3->a = 0xff;
    }
    else
    {
        /* Three-color block: derive the other color. */
        /* 00 = color_0,  01 = color_1,  10 = color_2,   */
        /* 11 = transparent. */
        /* These two bit codes correspond to the 2-bit fields  */
        /* stored in the 64-bit block.  */

        /* explicit for each component, unlike some refrasts... */

        *wrd = ((RwUInt16)col_0->r + (RwUInt16)col_1->r )/2;
        col_2->r = (RwUInt8)*wrd;
        *wrd = ((RwUInt16)col_0->g + (RwUInt16)col_1->g )/2;
        col_2->g = (RwUInt8)*wrd;
        *wrd = ((RwUInt16)col_0->b + (RwUInt16)col_1->b )/2;
        col_2->b = (RwUInt8)*wrd;
        col_2->a = 0xff;

        col_3->r = 0x00;        /* black color to indicate alpha */
        col_3->g = 0x00;
        col_3->b = 0x00;
        col_3->a = 0x00;

        hasAlpha = TRUE;
    }

    RWRETURN(hasAlpha);
}


/* -------------------------------------------------------------------------
 *  Function: DecodeColorBlockCheckColor3
 *
 *  Purpose : 
 *
 *  On entry: 
 *
 *  On exit : 
 * ------------------------------------------------------------------------- */
static __inline RwBool
DecodeColorBlockCheckColor3( RwUInt32 * pImPos,
                             const DXTColBlock * pColorBlock,
                             RwInt32 width,
                             RwUInt32 * col_0,
                             RwUInt32 * col_1,
                             RwUInt32 * col_2,
                             RwUInt32 * col_3 )
{
    /* width is width of image in pixels */
    RwInt32     r,n;
    RwBool      usedThird = FALSE;

    RWFUNCTION( RWSTRING( "DecodeColorBlockCheckColor3" ) );

    /* r steps through lines in y */
    for( r=0; r < 4; r++, pImPos += width-4 )   /* no width*4 as RwUInt32 ptr inc will *4 */
    {
        RwUInt32 bits = pColorBlock->row[r];

        /* width * 4 bytes per pixel per line */
        /* each j dxtc row is 4 lines of pixels */

        /* pImPos = (RwUInt32*)((RwUInt32)pBase + i*16 + (r+j*4) * m_nWidth * 4 ); */

        /* n steps through pixels */
        for( n=0; n < 4; n++ )
        {
            switch( bits & 0x3)
            {
            case 0 :
                *pImPos = *col_0;
                pImPos++;       /* increment to next RwUInt32 */
                break;
            case 1 :
                *pImPos = *col_1;
                pImPos++;
                break;
            case 2 :
                *pImPos = *col_2;
                pImPos++;
                break;
            case 3 :
                usedThird = TRUE;
                *pImPos = *col_3;
                pImPos++;
                break;
            default:
                pImPos++;
                break;
            }

            bits >>= 2;
        }
    }

    RWRETURN(usedThird);
}


/* -------------------------------------------------------------------------
 *  Function: DecodeColorBlock
 *
 *  Purpose : 
 *
 *  On entry: 
 *
 *  On exit : 
 * ------------------------------------------------------------------------- */
static __inline void
DecodeColorBlock( RwUInt32 * pImPos,
                  const DXTColBlock * pColorBlock,
                  RwInt32 width,
                  RwUInt32 * col_0,
                  RwUInt32 * col_1,
                  RwUInt32 * col_2,
                  RwUInt32 * col_3 )
{
    /* width is width of image in pixels */
    RwInt32     r,n;

    RWFUNCTION( RWSTRING( "DecodeColorBlock" ) );

    /* r steps through lines in y */
    for( r=0; r < 4; r++, pImPos += width-4 )   /* no width*4 as RwUInt32 ptr inc will *4 */
    {
        RwUInt32 bits = pColorBlock->row[r];

        /* width * 4 bytes per pixel per line */
        /* each j dxtc row is 4 lines of pixels */

        /* pImPos = (RwUInt32*)((RwUInt32)pBase + i*16 + (r+j*4) * m_nWidth * 4 ); */

        /* n steps through pixels */
        for( n=0; n < 4; n++ )
        {
            switch( bits & 0x3)
            {
            case 0 :
                *pImPos = *col_0;
                pImPos++;       /* increment to next RwUInt32 */
                break;
            case 1 :
                *pImPos = *col_1;
                pImPos++;
                break;
            case 2 :
                *pImPos = *col_2;
                pImPos++;
                break;
            case 3 :
                *pImPos = *col_3;
                pImPos++;
                break;
            default:
                pImPos++;
                break;
            }

            bits >>= 2;
        }
    }

    RWRETURNVOID();
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLDecompressDXT1toARGB
 *
 *  Purpose : Decompress DXT3 data into ARGB
 *
 *  On entry: width - Constant RwUInt32 containing the width of the top level mip.
 *
 *            height - Constant RwUInt32 containing the height of the top level mip.
 *
 *            destPixels - Pointer to RwUInt8 referring to the image pixels
 *
 *            srcPixels - Pointer to RwUInt8 referring to the DXT pixels
 *
 *  On exit : RwBool, TRUE if the DXT data contains alpha, FALSE otherwise.
 * ------------------------------------------------------------------------- */
RwBool
_rwOpenGLDecompressDXT1toARGB( const RwUInt32 width,
                               const RwUInt32 height,
                               RwUInt8 *destPixels,
                               RwUInt8 *srcPixels )
{
    const RwInt32       xblocks = width / 4;

    const RwInt32       yblocks = height / 4;

    RwUInt32            *pBase  = (RwUInt32 *)destPixels;

    RwUInt32            *pImPos = pBase;            /* pos in decompressed data */

    const DXTColBlock   *pBlock;

    Color8888           col_0;
    
    Color8888           col_1;
    
    Color8888           col_2;
    
    Color8888           col_3;

    RwUInt16            wrd;

    RwInt32             i;
    
    RwInt32             j;

    RwBool              hasAlpha = FALSE;


    RWFUNCTION( RWSTRING( "_rwOpenGLDecompressDXT1toARGB" ) );

    RWASSERT( NULL != destPixels );
    RWASSERT( NULL != srcPixels );

    for ( j = 0; j < yblocks; j += 1 )
    {
        /* 8 bytes per block */
        pBlock = (const DXTColBlock *) ( (const RwUInt8 *)srcPixels + j * xblocks * 8 );

        for( i=0; i < xblocks; i++, pBlock++ )
        {
            /* inline func: */
            const RwBool hasAlphaBlock = GetColorBlockColorsDXT1(pBlock,
                                                                &col_0,
                                                                &col_1,
                                                                &col_2,
                                                                &col_3,
                                                                &wrd);

            /* now decode the color block into the bitmap bits inline func: */
            pImPos = (RwUInt32 *)((RwUInt8 *)pBase + i*16 + (j*4) * width * 4 );

            if (hasAlphaBlock)
            {
                hasAlpha |= DecodeColorBlockCheckColor3(pImPos,
                                                        pBlock,
                                                        width,
                                                        (RwUInt32 *)&col_0,
                                                        (RwUInt32 *)&col_1,
                                                        (RwUInt32 *)&col_2,
                                                        (RwUInt32 *)&col_3 );
            }
            else
            {
                DecodeColorBlock(pImPos,
                                 pBlock,
                                 width,
                                 (RwUInt32 *)&col_0,
                                 (RwUInt32 *)&col_1,
                                 (RwUInt32 *)&col_2,
                                 (RwUInt32 *)&col_3 );
            }
        }
    }

    RWRETURN( hasAlpha );
}


/* -------------------------------------------------------------------------
 *  Function: GetColorBlockColors
 *
 *  Purpose : 
 *
 *  On entry: 
 *
 *  On exit : 
 * ------------------------------------------------------------------------- */
static __inline void
GetColorBlockColors( const DXTColBlock *pBlock,
                     Color8888 *col_0,
                     Color8888 *col_1,
                     Color8888 *col_2,
                     Color8888 *col_3,
                     RwUInt16 *wrd )
{
    /* There are 4 methods to use - see the Time_ functions. */
    /* 1st = shift = does normal approach per byte for color comps */
    /* 2nd = use freak variable bit field color565 for component extraction */
    /* 3rd = use super-freak RwUInt32 adds BEFORE shifting the color components */
    /*  This lets you do only 1 add per color instead of 3 RwUInt8 adds and */
    /*  might be faster */
    /* Call RunTimingSession() to run each of them & output result to txt file */

    /* freak variable bit structure method */
    /* normal math */
    /* This method is fastest */
    const Color565 *pCol;

    RWFUNCTION( RWSTRING( "GetColorBlockColors" ) );

    pCol = (const Color565 *) & (pBlock->col0 );

    col_0->a = 0xff;
    col_0->r = pCol->nRed;
    col_0->r <<= 3;             /* shift to full precision */
    col_0->g = pCol->nGreen;
    col_0->g <<= 2;
    col_0->b = pCol->nBlue;
    col_0->b <<= 3;

    pCol = (const Color565 *) & (pBlock->col1 );
    col_1->a = 0xff;
    col_1->r = pCol->nRed;
    col_1->r <<= 3;             /* shift to full precision */
    col_1->g = pCol->nGreen;
    col_1->g <<= 2;
    col_1->b = pCol->nBlue;
    col_1->b <<= 3;

    if( pBlock->col0 > pBlock->col1 )
    {
        /* Four-color block: derive the other two colors.     */
        /* 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3 */
        /* These two bit codes correspond to the 2-bit fields  */
        /* stored in the 64-bit block. */

        *wrd = ((RwUInt16)col_0->r * 2 + (RwUInt16)col_1->r )/3;
                                            /* no +1 for rounding */
                                            /* as bits have been shifted to 888 */
        col_2->r = (RwUInt8)*wrd;

        *wrd = ((RwUInt16)col_0->g * 2 + (RwUInt16)col_1->g )/3;
        col_2->g = (RwUInt8)*wrd;

        *wrd = ((RwUInt16)col_0->b * 2 + (RwUInt16)col_1->b )/3;
        col_2->b = (RwUInt8)*wrd;
        col_2->a = 0xff;

        *wrd = ((RwUInt16)col_0->r + (RwUInt16)col_1->r *2 )/3;
        col_3->r = (RwUInt8)*wrd;

        *wrd = ((RwUInt16)col_0->g + (RwUInt16)col_1->g *2 )/3;
        col_3->g = (RwUInt8)*wrd;

        *wrd = ((RwUInt16)col_0->b + (RwUInt16)col_1->b *2 )/3;
        col_3->b = (RwUInt8)*wrd;
        col_3->a = 0xff;
    }
    else
    {
        /* Three-color block: derive the other color. */
        /* 00 = color_0,  01 = color_1,  10 = color_2,   */
        /* 11 = transparent. */
        /* These two bit codes correspond to the 2-bit fields  */
        /* stored in the 64-bit block.  */

        /* explicit for each component, unlike some refrasts... */

        *wrd = ((RwUInt16)col_0->r + (RwUInt16)col_1->r )/2;
        col_2->r = (RwUInt8)*wrd;
        *wrd = ((RwUInt16)col_0->g + (RwUInt16)col_1->g )/2;
        col_2->g = (RwUInt8)*wrd;
        *wrd = ((RwUInt16)col_0->b + (RwUInt16)col_1->b )/2;
        col_2->b = (RwUInt8)*wrd;
        col_2->a = 0xff;

        col_3->r = 0x00;        /* random color to indicate alpha */
        col_3->g = 0xff;
        col_3->b = 0xff;
        col_3->a = 0x00;
    }

    RWRETURNVOID();
}


/* -------------------------------------------------------------------------
 *  Function: DecodeAlphaExplicit
 *
 *  Purpose : 
 *
 *  On entry: 
 *
 *  On exit : 
 * ------------------------------------------------------------------------- */
static __inline void
DecodeAlphaExplicit( RwUInt32 *pImPos,
                     const DXTAlphaBlockExplicit *pAlphaBlock,
                     RwInt32 width,
                     RwUInt32 alphazero )
{
    /* alphazero is a bit mask that when & with the image color */
    /*  will zero the alpha bits, so if the image DWORDs  are */
    /*  ARGB then alphazero will be 0x00ffffff or if */
    /*  RGBA then alphazero will be 0xffffff00 */
    /*  alphazero constructed automaticaly from field order of Color8888 structure */

    /* decodes to 32 bit format only */
    RwInt32 row, pix;

    RwUInt16 wrd;

    Color8888 col;

    RWFUNCTION( RWSTRING( "DecodeAlphaExplicit" ) );

    col.r = col.g = col.b = 0;

    for( row=0; row < 4; row++, pImPos += width-4 )
    {
        /* pImPow += pImPos += width-4 moves to next row down */

        wrd = pAlphaBlock->row[ row ];

        for( pix = 0; pix < 4; pix++ )
        {
            /* zero the alpha bits of image pixel */
            *pImPos &= alphazero;

            col.a = wrd & 0x000f;       /* get only low 4 bits */
/*          col.a <<= 4;                // shift to full byte precision */
                                        /* NOTE:  with just a << 4 you'll never have alpha */
                                        /* of 0xff,  0xf0 is max so pure shift doesn't quite */
                                        /* cover full alpha range. */
                                        /* It's much cheaper than divide & scale though. */
                                        /* To correct for this, and get 0xff for max alpha, */
                                        /*  or the low bits back in after left shifting */
            col.a = col.a | (col.a << 4 );  /* This allows max 4 bit alpha to be 0xff alpha */
                                            /*  in final image, and is crude approach to full  */
                                            /*  range scale */

            *pImPos |= *((RwUInt32*)&col);  /* or the bits into the prev. nulled alpha */

            wrd >>= 4;      /* move next bits to lowest 4 */

            pImPos++;       /* move to next pixel in the row */

        }
    }

    RWRETURNVOID();
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLDecompressDXT3toARGB
 *
 *  Purpose : Decompress DXT3 data into ARGB
 *
 *  On entry: width - Constant RwUInt32 containing the width of the top level mip.
 *
 *            height - Constant RwUInt32 containing the height of the top level mip.
 *
 *            destPixels - Pointer to RwUInt8 referring to the image pixels
 *
 *            srcPixels - Pointer to RwUInt8 referring to the DXT pixels
 *
 *  On exit : Nothing
 * ------------------------------------------------------------------------- */
void
_rwOpenGLDecompressDXT3toARGB( const RwUInt32 width,
                               const RwUInt32 height,
                               RwUInt8 *destPixels,
                               RwUInt8 *srcPixels )
{
    const RwInt32               xblocks = width / 4;

    const RwInt32               yblocks = height / 4;

    RwUInt32                    *pBase  = (RwUInt32 *)destPixels;

    RwUInt32                    *pImPos = pBase;

    const DXTColBlock           *pBlock;

    const DXTAlphaBlockExplicit *pAlphaBlock;

    Color8888                   col_0;
    
    Color8888                   col_1;
    
    Color8888                   col_2;
    
    Color8888                   col_3;

    RwUInt32                    alphazero;

    RwUInt16                    wrd;

    RwInt32                     i;
    
    RwInt32                     j;

    
    RWFUNCTION( RWSTRING( "_rwOpenGLDecompressDXT3toARGB" ) );

    RWASSERT( NULL != destPixels );
    RWASSERT( NULL != srcPixels );

    /* fill alphazero with appropriate value to zero out alpha when */
    /*  alphazero is ANDed with the image color 32 bit RwUInt32: */
    col_0.a = 0;
    col_0.r = col_0.g = col_0.b = 0xff;
    alphazero = *((RwUInt32*) &col_0);

    for( j=0; j < yblocks; j++ )
    {
        /* 8 bytes per block */
        /* 1 block for alpha, 1 block for color */
        pBlock = (const DXTColBlock *) ( (const RwUInt8 *)srcPixels + j * xblocks * 16 );

        for( i=0; i < xblocks; i++, pBlock ++ )
        {
            /* inline */
            /* Get alpha block */

            pAlphaBlock = (const DXTAlphaBlockExplicit*) pBlock;

            /* inline func: */
            /* Get color block & colors */
            pBlock++;

            GetColorBlockColors( pBlock, &col_0, &col_1, &col_2, &col_3, &wrd);

            /* Decode the color block into the bitmap bits */
            /* inline func: */

            pImPos = (RwUInt32*)((RwUInt8 *)pBase + i*16 + (j*4) * width * 4 );


            DecodeColorBlock(pImPos, pBlock, width,
                             (RwUInt32 *)&col_0, (RwUInt32 *)&col_1,
                             (RwUInt32 *)&col_2, (RwUInt32 *)&col_3 );

            /* Overwrite the previous alpha bits with the alpha block */
            /*  info */
            /* inline func: */
            DecodeAlphaExplicit( pImPos, pAlphaBlock, width, alphazero );
        }
    }

    RWRETURNVOID();
}


/* -------------------------------------------------------------------------
 *  Function: DecodeAlpha3BitLinear
 *
 *  Purpose : 
 *
 *  On entry: 
 *
 *  On exit : 
 * ------------------------------------------------------------------------- */
static __inline void
DecodeAlpha3BitLinear( RwUInt32 *pImPos,
                       const DXTAlphaBlock3BitLinear *pAlphaBlock,
                       RwInt32 width,
                       RwUInt32 alphazero )
{
    const RwUInt32 mask = 0x00000007;       /* bits = 00 00 01 11 */

    RwUInt8     gBits[4][4];
    RwUInt16    gAlphas[8];
    RwUInt8     gACol[4][4];
    RwUInt32    bits;
    RwInt32     row, pix;

    RWFUNCTION( RWSTRING( "DecodeAlpha3BitLinear" ) );

    gAlphas[0] = pAlphaBlock->alpha0;
    gAlphas[1] = pAlphaBlock->alpha1;

    /* 8-alpha or 6-alpha block?     */
    if( gAlphas[0] > gAlphas[1] )
    {
        /* 8-alpha block:  derive the other 6 alphas.     */
        /* 000 = alpha_0, 001 = alpha_1, others are interpolated */

        gAlphas[2] = ( 6 * gAlphas[0] +     gAlphas[1]) / 7;    /* bit code 010 */
        gAlphas[3] = ( 5 * gAlphas[0] + 2 * gAlphas[1]) / 7;    /* Bit code 011     */
        gAlphas[4] = ( 4 * gAlphas[0] + 3 * gAlphas[1]) / 7;    /* Bit code 100     */
        gAlphas[5] = ( 3 * gAlphas[0] + 4 * gAlphas[1]) / 7;    /* Bit code 101 */
        gAlphas[6] = ( 2 * gAlphas[0] + 5 * gAlphas[1]) / 7;    /* Bit code 110     */
        gAlphas[7] = (     gAlphas[0] + 6 * gAlphas[1]) / 7;    /* Bit code 111 */
    }
    else
    {
        /* 6-alpha block:  derive the other alphas.     */
        /* 000 = alpha_0, 001 = alpha_1, others are interpolated */

        gAlphas[2] = (4 * gAlphas[0] +     gAlphas[1]) / 5; /* Bit code 010 */
        gAlphas[3] = (3 * gAlphas[0] + 2 * gAlphas[1]) / 5; /* Bit code 011     */
        gAlphas[4] = (2 * gAlphas[0] + 3 * gAlphas[1]) / 5; /* Bit code 100     */
        gAlphas[5] = (    gAlphas[0] + 4 * gAlphas[1]) / 5; /* Bit code 101 */
        gAlphas[6] = 0;                                     /* Bit code 110 */
        gAlphas[7] = 255;                                   /* Bit code 111 */
    }

    /* Decode 3-bit fields into array of 16 BYTES with same value */

    /* first two rows of 4 pixels each: */
    /* pRows = (Alpha3BitRows*) & ( pAlphaBlock->stuff[0] ); */

    bits = *( (const RwUInt32 *) & ( pAlphaBlock->stuff[0] ));

    gBits[0][0] = (RwUInt8)( bits & mask );
    bits >>= 3;
    gBits[0][1] = (RwUInt8)( bits & mask );
    bits >>= 3;
    gBits[0][2] = (RwUInt8)( bits & mask );
    bits >>= 3;
    gBits[0][3] = (RwUInt8)( bits & mask );
    bits >>= 3;
    gBits[1][0] = (RwUInt8)( bits & mask );
    bits >>= 3;
    gBits[1][1] = (RwUInt8)( bits & mask );
    bits >>= 3;
    gBits[1][2] = (RwUInt8)( bits & mask );
    bits >>= 3;
    gBits[1][3] = (RwUInt8)( bits & mask );

    /* now for last two rows: */

    bits = *( (const RwUInt32 *) & ( pAlphaBlock->stuff[3] ));      /* last 3 bytes */

    gBits[2][0] = (RwUInt8)( bits & mask );
    bits >>= 3;
    gBits[2][1] = (RwUInt8)( bits & mask );
    bits >>= 3;
    gBits[2][2] = (RwUInt8)( bits & mask );
    bits >>= 3;
    gBits[2][3] = (RwUInt8)( bits & mask );
    bits >>= 3;
    gBits[3][0] = (RwUInt8)( bits & mask );
    bits >>= 3;
    gBits[3][1] = (RwUInt8)( bits & mask );
    bits >>= 3;
    gBits[3][2] = (RwUInt8)( bits & mask );
    bits >>= 3;
    gBits[3][3] = (RwUInt8)( bits & mask );

    /* decode the codes into alpha values */
    for( row = 0; row < 4; row++ )
    {
        for( pix=0; pix < 4; pix++ )
        {
            gACol[row][pix] = (RwUInt8) gAlphas[ gBits[row][pix] ];
        }
    }

    /* Write out alpha values to the image bits */
    for( row=0; row < 4; row++, pImPos += width-4 )
    {
        /* pImPow += pImPos += width-4 moves to next row down */

        for( pix = 0; pix < 4; pix++ )
        {
            /* zero the alpha bits of image pixel */
            *pImPos &=  alphazero;

            *pImPos |=  ((RwUInt32)gACol[row][pix]) << 24;  /* or the bits into the prev. nulled alpha */
            pImPos++;
        }
    }

    RWRETURNVOID();
}


/* -------------------------------------------------------------------------
 *  Function: _rwOpenGLDecompressDXT5toARGB
 *
 *  Purpose : Decompress DXT5 data into ARGB
 *
 *  On entry: width - Constant RwUInt32 containing the width of the top level mip.
 *
 *            height - Constant RwUInt32 containing the height of the top level mip.
 *
 *            destPixels - Pointer to RwUInt8 referring to the image pixels
 *
 *            srcPixels - Pointer to RwUInt8 referring to the DXT pixels
 *
 *  On exit : Nothing
 * ------------------------------------------------------------------------- */
void
_rwOpenGLDecompressDXT5toARGB( const RwUInt32 width,
                               const RwUInt32 height,
                               RwUInt8 *destPixels,
                               RwUInt8 *srcPixels )
{
    const RwInt32                   xblocks = width / 4;

    const RwInt32                   yblocks = height / 4;

    RwUInt32                        *pBase  = (RwUInt32 *)destPixels;

    RwUInt32                        *pImPos = pBase;            /* pos in decompressed data */

    const DXTColBlock               *pBlock;

    const DXTAlphaBlock3BitLinear   *pAlphaBlock;

    Color8888                       col_0;
    
    Color8888                       col_1;
    
    Color8888                       col_2;
    
    Color8888                       col_3;

    RwUInt32                        alphazero;

    RwUInt16                        wrd;

    RwInt32                         i;
    
    RwInt32                         j;


    RWFUNCTION( RWSTRING( "_rwOpenGLDecompressDXT5toARGB" ) );

    RWASSERT( NULL != destPixels );
    RWASSERT( NULL != srcPixels );

    /* fill alphazero with appropriate value to zero out alpha when */
    /*  alphazero is ANDed with the image color 32 bit RwUInt32: */
    col_0.a = 0;
    col_0.r = col_0.g = col_0.b = 0xff;

    alphazero = *((RwUInt32*) &col_0);

    for( j=0; j < yblocks; j++ )
    {
        /* 8 bytes per block */
        /* 1 block for alpha, 1 block for color */
        pBlock = (const DXTColBlock *) ( (const RwUInt8 *)srcPixels + j * xblocks * 16 );

        for( i=0; i < xblocks; i++, pBlock ++ )
        {
            /* inline */
            /* Get alpha block */

            pAlphaBlock = (const DXTAlphaBlock3BitLinear *) pBlock;

            /* inline func: */
            /* Get color block & colors */
            pBlock++;

            GetColorBlockColors( pBlock, &col_0, &col_1, &col_2, &col_3, &wrd);

            /* Decode the color block into the bitmap bits */
            /* inline func: */

            pImPos = (RwUInt32*)((RwUInt8 *)pBase + i*16 + (j*4) * width * 4 );

            DecodeColorBlock(pImPos, pBlock, width,
                             (RwUInt32 *)&col_0, (RwUInt32 *)&col_1,
                             (RwUInt32 *)&col_2, (RwUInt32 *)&col_3 );

            /* Overwrite the previous alpha bits with the alpha block */
            /*  info */
            DecodeAlpha3BitLinear( pImPos, pAlphaBlock, width, alphazero);
        }
    }

    RWRETURNVOID();
}
