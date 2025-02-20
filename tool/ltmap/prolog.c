/* START OF PROLOG */

/*
 * Things that should only be defined once are protected by the PROLOG_REN
 * define. This is defined as the last action in this file. The following
 * define is used a add a unique prefix to types and non-shared statics to
 * provide scoping between multiple includes. If you need to use this, define
 * PREFIX
 */

#include "struct.c"

#ifdef INTERP_Q
#ifdef REALPROXY
#define OOQBEGIN(DST,SRC) DST = (((RwReal)(1.0f)) / ( SRC))
#define OOQEND(DST)
#else /* REALPROXY */

/* a variety of ways of generating reciprocal */
static RwReal SCOPE(FixedOne) = (RwReal)(65536.0);
#if defined(NOASM) || !defined(_MSC_VER)
#define OOQBEGIN(DST,SRC) recipq = ((SCOPE(FixedOne)) / ( SCOPE(sb).v.part.q))
#define OOQEND(DST)
#else /* defined(NOASM) || !defined(_MSC_VER) */
#define OOQBEGIN(DST,SRC)  \
    __asm fld dword ptr [SCOPE(FixedOne)]                                \
    __asm fdiv dword ptr [SCOPE(sb)]SCOPE(ScanBlock).v.part.q
#define OOQEND(DST)        __asm fstp dword ptr [recipq]
#endif /* defined(NOASM) || !defined(_MSC_VER) */

#endif /* REALPROXY */
#else /* INTERP_Q */
#define OOQBEGIN(dst,src)
#define OOQEND(dst)
#endif /* INTERP_Q */

#if 1
#define POSREAL2INT(A)     Float2int(A)
#define REAL2INT(A)        Float2int(A)
#define POSREAL2FIX(A)     Float2int((A) * 65536.0f)
#define REAL2FIX(A)        Float2int((A) * 65536.0f)
#else
#define POSREAL2INT(A)     (RwInt32)(A)
#define REAL2INT(A)        (RwInt32)(A)
#define POSREAL2FIX(A)     RToFixed(A)
#define REAL2FIX(A)        RToFixed(A)
#endif

#ifdef REALPROXY
#define POSREAL2PROXY(A) (A)
#define REAL2PROXY (A) (A)
#else
#define POSREAL2PROXY(A) POSREAL2FIX(A)
#define REAL2PROXY(A) REAL2FIX(A)
#endif

#ifndef TRISETUP
#define TRISETUP(Y)
#endif
#ifndef TRIDY
#define TRIDY
#endif

#ifndef SCANDECL
#define SCANDECL
#endif
#ifndef SCANSETUP
#define SCANSETUP(X)
#endif

#ifndef SEGDECL
#define SEGDECL
#endif
#ifndef SEGBEGIN
#define SEGBEGIN(W)
#endif
#ifndef SEGPIXEL
#define SEGPIXEL
#endif
#ifndef SEGEND
#define SEGEND
#endif

#ifndef DITHER
#ifndef DITHREDECL
#define DITHERDECL
#endif /* DITHERDECL */
#ifndef DITHERLINE
#define DITHERLINE(Y)
#endif /* DITHERLINE */
#ifndef DITHERCOL
#define DITHERCOL(X)
#endif /* DITHERCOL */
#ifndef DITHERDX
#define DITHERDX
#endif /* DITHERDX */
#ifndef DITHERDY
#define DITHERDY
#endif /* DITHERDY */
#else /* DITHER */
#ifndef DITHERDECL
#define DITHERDECL      unsigned long SCOPE(dither);
#endif /* DITHERDECL */
#ifndef DITHERLINE
#define DITHERLINE(Y)   SCOPE(sb).dithline = _rwdithDown[Y & 7];
#endif /* DITHERLINE */
#ifndef DITHERCOL
#define DITHERCOL(X)    SCOPE(dither) = _rwdithAcross[X & 7]         \
                                        ^ ((unsigned char)SCOPE(sb).dithline);
#endif /* DITHERCOL */
#ifndef DITHERDX
#define DITHERDX        SCOPE(dither) ^= (SCOPE(dither) >> 6);
#endif /* DITHERDX */
#ifndef DITHERDY
#define DITHERDY        SCOPE(sb).dithline ^= (SCOPE(sb).dithline >> 6);
#endif /* DITHERDY */
#endif /* DITHER */

#define LEFT 0
#define RIGHT 1

typedef struct
{
    SCOPE(EdgeBlock) *left,*right;
    int replace;
    SCOPE(EdgeBlock) *next;
} SCOPE(EdgeOrder);

static SCOPE(EdgeBlock) SCOPE(edge)[3];
static SCOPE(EdgeOrder) SCOPE(order)[8] = 
{
    {&SCOPE(edge)[0],&SCOPE(edge)[2],RIGHT,&SCOPE(edge)[1]},   /* singularity */
    {&SCOPE(edge)[1],&SCOPE(edge)[0], LEFT,&SCOPE(edge)[2]},
    {&SCOPE(edge)[2],&SCOPE(edge)[1], LEFT,&SCOPE(edge)[0]},
    {&SCOPE(edge)[2],&SCOPE(edge)[1],RIGHT,&SCOPE(edge)[0]},
    {&SCOPE(edge)[0],&SCOPE(edge)[2], LEFT,&SCOPE(edge)[1]},
    {&SCOPE(edge)[1],&SCOPE(edge)[0],RIGHT,&SCOPE(edge)[2]},
    {&SCOPE(edge)[0],&SCOPE(edge)[2],RIGHT,&SCOPE(edge)[1]},
    {&SCOPE(edge)[0],&SCOPE(edge)[2],RIGHT,&SCOPE(edge)[1]}    /* singularity */
};

SCOPE(ScanBlock) SCOPE(sb);
SCANDECL
DITHERDECL

/* setup affine segment length */
#if 1
#define ASEGLEN 16
#define ASEGRND 15
#define ASEGSHIFT 4
#endif
#if 0
#define ASEGLEN 8
#define ASEGRND 7
#define ASEGSHIFT 3
#endif

/* Vector functions */
#if REALVECTORLEN > 3
#define RealVectorAdd(dst,src1,src2) \
    dst.all[0] = src1.all[0] + src2.all[0]; \
    dst.all[1] = src1.all[1] + src2.all[1]; \
    dst.all[2] = src1.all[2] + src2.all[2]; \
    dst.all[3] = src1.all[3] + src2.all[3];
#else
#if REALVECTORLEN > 2
#define RealVectorAdd(dst,src1,src2) \
    dst.all[0] = src1.all[0] + src2.all[0]; \
    dst.all[1] = src1.all[1] + src2.all[1]; \
    dst.all[2] = src1.all[2] + src2.all[2];
#else
#if REALVECTORLEN > 1
#define RealVectorAdd(dst,src1,src2) \
    dst.all[0] = src1.all[0] + src2.all[0]; \
    dst.all[1] = src1.all[1] + src2.all[1];
#else
#if REALVECTORLEN > 0
#define RealVectorAdd(dst,src1,src2) \
    dst.all[0] = src1.all[0] + src2.all[0];
#else
#define RealVectorAdd(dst,src1,src2)
#endif
#endif
#endif
#endif

#if REALVECTORLEN > 3
#define RealVectorAddScaled(dst,src1,src2,scale) \
    dst.all[0] = src1.all[0] + src2.all[0] * scale; \
    dst.all[1] = src1.all[1] + src2.all[1] * scale; \
    dst.all[2] = src1.all[2] + src2.all[2] * scale; \
    dst.all[3] = src1.all[3] + src2.all[3] * scale;
#else
#if REALVECTORLEN > 2
#define RealVectorAddScaled(dst,src1,src2,scale) \
    dst.all[0] = src1.all[0] + src2.all[0] * scale; \
    dst.all[1] = src1.all[1] + src2.all[1] * scale; \
    dst.all[2] = src1.all[2] + src2.all[2] * scale;
#else
#if REALVECTORLEN > 1
#define RealVectorAddScaled(dst,src1,src2,scale) \
    dst.all[0] = src1.all[0] + src2.all[0] * scale; \
    dst.all[1] = src1.all[1] + src2.all[1] * scale;
#else
#if REALVECTORLEN > 0
#define RealVectorAddScaled(dst,src1,src2,scale) \
    dst.all[0] = src1.all[0] + src2.all[0] * scale;
#else
#define RealVectorAddScaled(dst,src1,src2,scale)
#endif
#endif
#endif
#endif

#if REALVECTORLEN-INTERP_COUNT_REAL > 3
#define PCRealVectorAdd(dst,src1,src2) \
    dst.all[0] = src1.all[0] + src2.all[0]; \
    dst.all[1] = src1.all[1] + src2.all[1]; \
    dst.all[2] = src1.all[2] + src2.all[2]; \
    dst.all[3] = src1.all[3] + src2.all[3];
#else
#if REALVECTORLEN-INTERP_COUNT_REAL > 2
#define PCRealVectorAdd(dst,src1,src2) \
    dst.all[0] = src1.all[0] + src2.all[0]; \
    dst.all[1] = src1.all[1] + src2.all[1]; \
    dst.all[2] = src1.all[2] + src2.all[2];
#else
#if REALVECTORLEN-INTERP_COUNT_REAL > 1
#define PCRealVectorAdd(dst,src1,src2) \
    dst.all[0] = src1.all[0] + src2.all[0]; \
    dst.all[1] = src1.all[1] + src2.all[1];
#else
#if REALVECTORLEN-INTERP_COUNT_REAL > 0
#define PCRealVectorAdd(dst,src1,src2) \
    dst.all[0] = src1.all[0] + src2.all[0];
#else
#define PCRealVectorAdd(dst,src1,src2)
#endif
#endif
#endif
#endif

#if REALVECTORLEN-INTERP_COUNT_REAL > 3
#define PCRealVectorAddScaled(dst,src1,src2,scale) \
    dst.all[0] = src1.all[0] + src2.all[0] * scale; \
    dst.all[1] = src1.all[1] + src2.all[1] * scale; \
    dst.all[2] = src1.all[2] + src2.all[2] * scale; \
    dst.all[3] = src1.all[3] + src2.all[3] * scale;
#else
#if REALVECTORLEN-INTERP_COUNT_REAL > 2
#define PCRealVectorAddScaled(dst,src1,src2,scale) \
    dst.all[0] = src1.all[0] + src2.all[0] * scale; \
    dst.all[1] = src1.all[1] + src2.all[1] * scale; \
    dst.all[2] = src1.all[2] + src2.all[2] * scale;
#else
#if REALVECTORLEN-INTERP_COUNT_REAL > 1
#define PCRealVectorAddScaled(dst,src1,src2,scale) \
    dst.all[0] = src1.all[0] + src2.all[0] * scale; \
    dst.all[1] = src1.all[1] + src2.all[1] * scale;
#else
#if REALVECTORLEN-INTERP_COUNT_REAL > 0
#define PCRealVectorAddScaled(dst,src1,src2,scale) \
    dst.all[0] = src1.all[0] + src2.all[0] * scale;
#else
#define PCRealVectorAddScaled(dst,src1,src2,scale)
#endif
#endif
#endif
#endif

#if REALVECTORPROXYLEN > 3
#define RealVectorProxyAdd(dst,src1,src2) \
    dst.all[0] = src1.all[0] + src2.all[0]; \
    dst.all[1] = src1.all[1] + src2.all[1]; \
    dst.all[2] = src1.all[2] + src2.all[2]; \
    dst.all[3] = src1.all[3] + src2.all[3];
#else
#if REALVECTORPROXYLEN > 2
#define RealVectorProxyAdd(dst,src1,src2) \
    dst.all[0] = src1.all[0] + src2.all[0]; \
    dst.all[1] = src1.all[1] + src2.all[1]; \
    dst.all[2] = src1.all[2] + src2.all[2];
#else
#if REALVECTORPROXYLEN > 1
#define RealVectorProxyAdd(dst,src1,src2) \
    dst.all[0] = src1.all[0] + src2.all[0]; \
    dst.all[1] = src1.all[1] + src2.all[1];
#else
#if REALVECTORPROXYLEN > 0
#define RealVectorProxyAdd(dst,src1,src2) \
    dst.all[0] = src1.all[0] + src2.all[0];
#else
#define RealVectorProxyAdd(dst,src1,src2)
#endif
#endif
#endif
#endif

#if INTERP_COUNT_REAL > 3
#define RealVectorProxyAssign(dst, src) \
    dst[0] = POSREAL2PROXY(src[0]); \
    dst[1] = POSREAL2PROXY(src[1]); \
    dst[2] = POSREAL2PROXY(src[2]); \
    dst[3] = POSREAL2PROXY(src[3]);
#else
#if INTERP_COUNT_REAL > 2
#define RealVectorProxyAssign(dst, src) \
    dst[0] = POSREAL2PROXY(src[0]); \
    dst[1] = POSREAL2PROXY(src[1]); \
    dst[2] = POSREAL2PROXY(src[2]);
#else
#if INTERP_COUNT_REAL > 1
#define RealVectorProxyAssign(dst, src) \
    dst[0] = POSREAL2PROXY(src[0]); \
    dst[1] = POSREAL2PROXY(src[1]);
#else
#if INTERP_COUNT_REAL > 0
#define RealVectorProxyAssign(dst, src) \
    dst[0] = POSREAL2PROXY(src[0]);
#else
#define RealVectorProxyAssign(dst, src)
#endif
#endif
#endif
#endif

#ifdef REALPROXY
#if INTERP_COUNT_REALP > 3
#define PCRealVectorProxyAssign(dst1, dst2, src, recipq) \
    dst1[0] = (dst2[0] = src[0] * recipq); \
    dst1[1] = (dst2[1] = src[1] * recipq); \
    dst1[2] = (dst2[2] = src[2] * recipq); \
    dst1[3] = (dst2[3] = src[3] * recipq);
#else
#if INTERP_COUNT_REALP > 2
#define PCRealVectorProxyAssign(dst1, dst2, src, recipq) \
    dst1[0] = (dst2[0] = src[0] * recipq); \
    dst1[1] = (dst2[1] = src[1] * recipq); \
    dst1[2] = (dst2[2] = src[2] * recipq);
#else
#if INTERP_COUNT_REALP > 1
#define PCRealVectorProxyAssign(dst1, dst2, src, recipq) \
    dst1[0] = (dst2[0] = src[0] * recipq); \
    dst1[1] = (dst2[1] = src[1] * recipq);
#else
#if INTERP_COUNT_REALP > 0
#define PCRealVectorProxyAssign(dst1, dst2, src, recipq) \
    dst1[0] = (dst2[0] = src[0] * recipq);
#else
#define PCRealVectorProxyAssign(dst1, dst2, src, recipq)
#endif
#endif
#endif
#endif
#else
#if INTERP_COUNT_REALP > 3
#define PCRealVectorProxyAssign(dst1, dst2, src, recipq) \
    dst1[0] = POSREAL2INT(dst2[0] = src[0] * recipq); \
    dst1[1] = POSREAL2INT(dst2[1] = src[1] * recipq); \
    dst1[2] = POSREAL2INT(dst2[2] = src[2] * recipq); \
    dst1[3] = POSREAL2INT(dst2[3] = src[3] * recipq);
#else
#if INTERP_COUNT_REALP > 2
#define PCRealVectorProxyAssign(dst1, dst2, src, recipq) \
    dst1[0] = POSREAL2INT(dst2[0] = src[0] * recipq); \
    dst1[1] = POSREAL2INT(dst2[1] = src[1] * recipq); \
    dst1[2] = POSREAL2INT(dst2[2] = src[2] * recipq);
#else
#if INTERP_COUNT_REALP > 1
#define PCRealVectorProxyAssign(dst1, dst2, src, recipq) \
    dst1[0] = POSREAL2INT(dst2[0] = src[0] * recipq); \
    dst1[1] = POSREAL2INT(dst2[1] = src[1] * recipq);
#else
#if INTERP_COUNT_REALP > 0
#define PCRealVectorProxyAssign(dst1, dst2, src, recipq) \
    dst1[0] = POSREAL2INT(dst2[0] = src[0] * recipq);
#else
#define PCRealVectorProxyAssign(dst1, dst2, src, recipq)
#endif
#endif
#endif
#endif
#endif

#ifdef REALPROXY
#if INTERP_COUNT_REALP > 3
#define PCRealVectorProxyDxAssign(dst, src1, recipq, src2, seg_width) \
    dst[0] = ((src1[0] * recipq - src2[0]) * recip16lut[seg_width]); \
    dst[1] = ((src1[1] * recipq - src2[1]) * recip16lut[seg_width]); \
    dst[2] = ((src1[2] * recipq - src2[2]) * recip16lut[seg_width]); \
    dst[3] = ((src1[3] * recipq - src2[3]) * recip16lut[seg_width]);
#else
#if INTERP_COUNT_REALP > 2
#define PCRealVectorProxyDxAssign(dst, src1, recipq, src2, seg_width) \
    dst[0] = ((src1[0] * recipq - src2[0]) * recip16lut[seg_width]); \
    dst[1] = ((src1[1] * recipq - src2[1]) * recip16lut[seg_width]); \
    dst[2] = ((src1[2] * recipq - src2[2]) * recip16lut[seg_width]);
#else
#if INTERP_COUNT_REALP > 1
#define PCRealVectorProxyDxAssign(dst, src1, recipq, src2, seg_width) \
    dst[0] = ((src1[0] * recipq - src2[0]) * recip16lut[seg_width]); \
    dst[1] = ((src1[1] * recipq - src2[1]) * recip16lut[seg_width]);
#else
#if INTERP_COUNT_REALP > 0
#define PCRealVectorProxyDxAssign(dst, src1, recipq, src2, seg_width) \
    dst[0] = ((src1[0] * recipq - src2[0]) * recip16lut[seg_width]);
#else
#define PCRealVectorProxyDxAssign(dst, src1, recipq, src2, seg_width)
#endif
#endif
#endif
#endif
#else
#if INTERP_COUNT_REALP > 3
#define PCRealVectorProxyDxAssign(dst, src1, recipq, src2, seg_width) \
    dst[0] = POSREAL2INT((src1[0] * recipq - src2[0]) * recip16lut[seg_width]); \
    dst[1] = POSREAL2INT((src1[1] * recipq - src2[1]) * recip16lut[seg_width]); \
    dst[2] = POSREAL2INT((src1[2] * recipq - src2[2]) * recip16lut[seg_width]); \
    dst[3] = POSREAL2INT((src1[3] * recipq - src2[3]) * recip16lut[seg_width]);
#else
#if INTERP_COUNT_REALP > 2
#define PCRealVectorProxyDxAssign(dst, src1, recipq, src2, seg_width) \
    dst[0] = POSREAL2INT((src1[0] * recipq - src2[0]) * recip16lut[seg_width]); \
    dst[1] = POSREAL2INT((src1[1] * recipq - src2[1]) * recip16lut[seg_width]); \
    dst[2] = POSREAL2INT((src1[2] * recipq - src2[2]) * recip16lut[seg_width]);
#else
#if INTERP_COUNT_REALP > 1
#define PCRealVectorProxyDxAssign(dst, src1, recipq, src2, seg_width) \
    dst[0] = POSREAL2INT((src1[0] * recipq - src2[0]) * recip16lut[seg_width]); \
    dst[1] = POSREAL2INT((src1[1] * recipq - src2[1]) * recip16lut[seg_width]);
#else
#if INTERP_COUNT_REALP > 0
#define PCRealVectorProxyDxAssign(dst, src1, recipq, src2, seg_width) \
    dst[0] = POSREAL2INT((src1[0] * recipq - src2[0]) * recip16lut[seg_width]);
#else
#define PCRealVectorProxyDxAssign(dst, src1, recipq, src2, seg_width)
#endif
#endif
#endif
#endif
#endif

#ifdef REALPROXY
#if INTERP_COUNT_REALP > 3
#define PCRealVectorProxyDxAssign16(dst, src1, recipq, src2, seg_round) \
    dst[0] = ((src1[0] * recipq) - src2[0]) * recip16lut[ASEGLEN]; \
    dst[1] = ((src1[1] * recipq) - src2[1]) * recip16lut[ASEGLEN]; \
    dst[2] = ((src1[2] * recipq) - src2[2]) * recip16lut[ASEGLEN]; \
    dst[3] = ((src1[3] * recipq) - src2[3]) * recip16lut[ASEGLEN];
#else
#if INTERP_COUNT_REALP > 2
#define PCRealVectorProxyDxAssign16(dst, src1, recipq, src2, seg_round) \
    dst[0] = ((src1[0] * recipq) - src2[0]) * recip16lut[ASEGLEN]; \
    dst[1] = ((src1[1] * recipq) - src2[1]) * recip16lut[ASEGLEN]; \
    dst[2] = ((src1[2] * recipq) - src2[2]) * recip16lut[ASEGLEN];
#else
#if INTERP_COUNT_REALP > 1
#define PCRealVectorProxyDxAssign16(dst, src1, recipq, src2, seg_round) \
    dst[0] = ((src1[0] * recipq) - src2[0]) * recip16lut[ASEGLEN]; \
    dst[1] = ((src1[1] * recipq) - src2[1]) * recip16lut[ASEGLEN];
#else
#if INTERP_COUNT_REALP > 0
#define PCRealVectorProxyDxAssign16(dst, src1, recipq, src2, seg_round) \
    dst[0] = ((src1[0] * recipq) - src2[0]) * recip16lut[ASEGLEN];
#else
#define PCRealVectorProxyDxAssign16(dst, src1, recipq, src2, seg_round)
#endif
#endif
#endif
#endif
#else
#if INTERP_COUNT_REALP > 3
#define PCRealVectorProxyDxAssign16(dst, src1, recipq, src2, seg_round) \
    dst[0] = (POSREAL2INT(src1[0] * recipq) - src2[0] + seg_round[0]) >> ASEGSHIFT; \
    dst[1] = (POSREAL2INT(src1[1] * recipq) - src2[1] + seg_round[1]) >> ASEGSHIFT; \
    dst[2] = (POSREAL2INT(src1[2] * recipq) - src2[2] + seg_round[2]) >> ASEGSHIFT; \
    dst[3] = (POSREAL2INT(src1[3] * recipq) - src2[3] + seg_round[3]) >> ASEGSHIFT;
#else
#if INTERP_COUNT_REALP > 2
#define PCRealVectorProxyDxAssign16(dst, src1, recipq, src2, seg_round) \
    dst[0] = (POSREAL2INT(src1[0] * recipq) - src2[0] + seg_round[0]) >> ASEGSHIFT; \
    dst[1] = (POSREAL2INT(src1[1] * recipq) - src2[1] + seg_round[1]) >> ASEGSHIFT; \
    dst[2] = (POSREAL2INT(src1[2] * recipq) - src2[2] + seg_round[2]) >> ASEGSHIFT;
#else
#if INTERP_COUNT_REALP > 1
#define PCRealVectorProxyDxAssign16(dst, src1, recipq, src2, seg_round) \
    dst[0] = (POSREAL2INT(src1[0] * recipq) - src2[0] + seg_round[0]) >> ASEGSHIFT; \
    dst[1] = (POSREAL2INT(src1[1] * recipq) - src2[1] + seg_round[1]) >> ASEGSHIFT;
#else
#if INTERP_COUNT_REALP > 0
#define PCRealVectorProxyDxAssign16(dst, src1, recipq, src2, seg_round) \
    dst[0] = (POSREAL2INT(src1[0] * recipq) - src2[0] + seg_round[0]) >> ASEGSHIFT;
#else
#define PCRealVectorProxyDxAssign16(dst, src1, recipq, src2, seg_round)
#endif
#endif
#endif
#endif
#endif

#if FIXEDVECTORLEN > 3
#define FixedVectorAdd(dst, src1, src2) \
    dst.all[0] = src1.all[0] + src2.all[0]; \
    dst.all[1] = src1.all[1] + src2.all[1]; \
    dst.all[2] = src1.all[2] + src2.all[2]; \
    dst.all[3] = src1.all[3] + src2.all[3];
#else
#if FIXEDVECTORLEN > 2
#define FixedVectorAdd(dst, src1, src2) \
    dst.all[0] = src1.all[0] + src2.all[0]; \
    dst.all[1] = src1.all[1] + src2.all[1]; \
    dst.all[2] = src1.all[2] + src2.all[2];
#else
#if FIXEDVECTORLEN > 1
#define FixedVectorAdd(dst, src1, src2) \
    dst.all[0] = src1.all[0] + src2.all[0]; \
    dst.all[1] = src1.all[1] + src2.all[1];
#else
#if FIXEDVECTORLEN > 0
#define FixedVectorAdd(dst, src1, src2) \
    dst.all[0] = src1.all[0] + src2.all[0];
#else
#define FixedVectorAdd(dst, src1, src2)
#endif
#endif
#endif
#endif

#if FIXEDVECTORLEN > 3
#define FixedVectorAddScaled(dst, src1, src2, scale) \
    dst.all[0] = src1.all[0] + RwFMul(src2.all[0],scale); \
    dst.all[1] = src1.all[1] + RwFMul(src2.all[1],scale); \
    dst.all[2] = src1.all[2] + RwFMul(src2.all[2],scale); \
    dst.all[3] = src1.all[3] + RwFMul(src2.all[3],scale);
#else
#if FIXEDVECTORLEN > 2
#define FixedVectorAddScaled(dst, src1, src2, scale) \
    dst.all[0] = src1.all[0] + RwFMul(src2.all[0],scale); \
    dst.all[1] = src1.all[1] + RwFMul(src2.all[1],scale); \
    dst.all[2] = src1.all[2] + RwFMul(src2.all[2],scale);
#else
#if FIXEDVECTORLEN > 1
#define FixedVectorAddScaled(dst, src1, src2, scale) \
    dst.all[0] = src1.all[0] + RwFMul(src2.all[0],scale); \
    dst.all[1] = src1.all[1] + RwFMul(src2.all[1],scale);
#else
#if FIXEDVECTORLEN > 0
#define FixedVectorAddScaled(dst, src1, src2, scale) \
    dst.all[0] = src1.all[0] + RwFMul(src2.all[0],scale);
#else
#define FixedVectorAddScaled(dst, src1, src2, scale)
#endif
#endif
#endif
#endif

#ifndef TRAPFILL
static void 
#ifndef GREEDY
SCOPE(TrapFill)(SCOPE(EdgeBlock) *left, int height)
{
#if INTERP_COUNT_REALP > 0
    RwReal pc_rp[INTERP_COUNT_REALP+1];
#endif

#ifndef EXACTPRESTEP
    SCOPE(RealVector) dvdx_frac[4];
    SCOPE(FixedVector) dfvdx_frac[4];

    {
        int i;
        for (i=0; i<FIXEDVECTORLEN; i++)
        {
            dfvdx_frac[3].all[i] = SCOPE(sb).seg_dfvdx.all[i] >> 2;
            dfvdx_frac[2].all[i] = SCOPE(sb).seg_dfvdx.all[i] >> 1;
            dfvdx_frac[1].all[i] = dfvdx_frac[2].all[i] + dfvdx_frac[3].all[i];
            dfvdx_frac[0].all[i] = SCOPE(sb).seg_dfvdx.all[i];
        }

        for (i=0; i<REALVECTORLEN; i++)
        {
            dvdx_frac[3].all[i] = SCOPE(sb).dvdx.all[i] * (RwReal)(0.25);
            dvdx_frac[2].all[i] = SCOPE(sb).dvdx.all[i] * (RwReal)(0.5); 
            dvdx_frac[1].all[i] = SCOPE(sb).dvdx.all[i] * (RwReal)(0.75);
            dvdx_frac[0].all[i] = SCOPE(sb).dvdx.all[i];
        }
    }
#endif /* !EXACTPRESTEP */

    /* traverse trapezium */
    while (height > 0)
    {
        int xstart,width,seg_width;

        height--;

        /* Xprestep */
#ifndef EXACTPRESTEP
        {
            RwInt32 frac = (SCOPE(sb).left >> 14) & 3;

            FixedVectorAdd(SCOPE(sb).seg_fv, left->fv, dfvdx_frac[frac]);
            RealVectorAdd(SCOPE(sb).v, left->v, dvdx_frac[frac]);
        }
#else /* !EXACTPRESTEP */
        {
            RwFixed fXprestep = ((SCOPE(sb).left - 1) & 0xffff) ^ 0xffff;
            FixedVectorAddScaled(SCOPE(sb).seg_fv, left->fv,
                                 SCOPE(sb).seg_dfvdx, fXprestep);
            RealVectorAddScaled(SCOPE(sb).v, left->v, SCOPE(sb).dvdx, FxToReal(fXprestep));
        }
#endif /* !EXACTPRESTEP */

        /* scanline preamble */
        OOQBEGIN(recipq, SCOPE(sb).v.part.q);

        /* lefthand pixel position */
        xstart = FxToInt(SCOPE(sb).left + 0xffff);

        /* dither seed */
        DITHERCOL(xstart);

        /* setup per-scanline destination */
        SCANSETUP(xstart);

        /* determine scanline and segment width */
        width = FxToInt(SCOPE(sb).right + 0xffff) - xstart;
        seg_width = ((xstart - 1) & ASEGRND) ^ ASEGRND;

        /* generate perspective corrected xstart interpolants */
        OOQEND(recipq);

        /* generate FixedPoint Proxy of RealVectors */
        PCRealVectorProxyAssign(SCOPE(sb).seg_v.part.rp, pc_rp,
                                SCOPE(sb).v.part.rp, recipq);
        RealVectorProxyAssign(SCOPE(sb).seg_v.part.r, SCOPE(sb).v.part.r);

                /* have we got a short segment to begin with? */
        if (seg_width > 0)
        {
            /* clamp segment width */
            if (seg_width > width)
            {
                seg_width = width;
            }

            /* bump to segment end */
            PCRealVectorAddScaled(SCOPE(sb).v, SCOPE(sb).v, SCOPE(sb).dvdx,
                                  (RwReal)(seg_width));

            OOQBEGIN(recipq, SCOPE(sb).v.part.q);
            OOQEND(recipq);

            /* generate dx steps for seg_width pixel segment */
            PCRealVectorProxyDxAssign(SCOPE(sb).seg_dvdx.part.rp,
                                      SCOPE(sb).v.part.rp, recipq, pc_rp,
                                      seg_width);
        }

        /* traverse scanline */
        while (width > 0)
        {
            SEGDECL;

            width -= seg_width;

            /* bump to next segment end */
            PCRealVectorAddScaled(SCOPE(sb).v, SCOPE(sb).v, SCOPE(sb).dvdx,
                                  (RwReal)(ASEGLEN));

#ifdef SEGNPIXEL
            SEGNPIXEL;
#else /* SEGNPIXEL */
            /* kick off next */
            OOQBEGIN(recipq, SCOPE(sb).v.part.q);

            /* traverse short segment */
            SEGBEGIN(seg_width);
            while (seg_width > 0)
            {
                seg_width--;

                SEGPIXEL;

                /* bump segment pixel interpolators */
                FixedVectorAdd(SCOPE(sb).seg_fv, SCOPE(sb).seg_fv,
                               SCOPE(sb).seg_dfvdx);
                RealVectorProxyAdd(SCOPE(sb).seg_v, SCOPE(sb).seg_v,
                                   SCOPE(sb).seg_dvdx);

                /* bump dithering value */
                DITHERDX;
            }
            SEGEND;

            OOQEND(recipq);
#endif /* SEGNPIXEL */

            /* traverse long segments */
            while (width>>ASEGSHIFT)
            {
                width -= ASEGLEN;

                /* generate dx steps for ASEGLEN pixel segment */
                PCRealVectorProxyDxAssign16(SCOPE(sb).seg_dvdx.part.rp,
                                            SCOPE(sb).v.part.rp, recipq,
                                            SCOPE(sb).seg_v.part.rp,
                                            SCOPE(sb).seg_round);

                /* bump to next segment end */
                PCRealVectorAddScaled(SCOPE(sb).v, SCOPE(sb).v, SCOPE(sb).dvdx,
                                      (RwReal)(ASEGLEN));

#ifdef SEG16PIXEL
                SEG16PIXEL;
#else /* SEG16PIXEL */
#ifdef SEGNPIXEL
                SEGNPIXEL;
#else /* SEGNPIXEL */
                /* kick off next */
                OOQBEGIN(recipq, SCOPE(sb).v.part.q);

                /* FILL ASEGLEN pixels */
                SEGBEGIN(ASEGLEN);
                seg_width = ASEGLEN;
                while (seg_width > 0)
                {
                    seg_width--;

                    /* pixel0 */
                    SEGPIXEL;

                    /* bump segment pixel interpolators */
                    FixedVectorAdd(SCOPE(sb).seg_fv, SCOPE(sb).seg_fv,
                                   SCOPE(sb).seg_dfvdx);
                    RealVectorProxyAdd(SCOPE(sb).seg_v, SCOPE(sb).seg_v,
                                       SCOPE(sb).seg_dvdx);

                    /* bump dithering value */
                    DITHERDX;
                }
                SEGEND;

                OOQEND(recipq);
#endif /* SEGNPIXEL */
#endif /* SEG16PIXEL */
            }

            /* tail end */
            seg_width = width;

            /* generate dx steps for next ASEGLEN pixel segment
                (actually curtailed by seg_width) */
            PCRealVectorProxyDxAssign16(SCOPE(sb).seg_dvdx.part.rp,
                                        SCOPE(sb).v.part.rp, recipq,
                                        SCOPE(sb).seg_v.part.rp,
                                        SCOPE(sb).seg_round);
        }

        /* bump boundary */
        SCOPE(sb).left += SCOPE(sb).dleft;
        SCOPE(sb).right += SCOPE(sb).dright;

        /* bump edge interpolators */
        FixedVectorAdd(left->fv, left->fv, left->dfvdy);
        RealVectorAdd(left->v, left->v, left->dvdy);

        /* bump dithering value */
        DITHERDY;

        /* bump destination */
        TRIDY;
    }
}
#else /* !GREEDY */
/* Warning, this code relies on 
 * (RwFixed)RwInt32FromRealMacro(<fixed>*<real>) 
 * doing the right thing */
SCOPE(TrapFill)(SCOPE(EdgeBlock) *left, int height, RwReal rHeight)
{
    {
        RwFixed oldLeftX, oldRightX;
        RwReal tmp;

        /* Save off old Xs as we will distructively prestep them */
        oldLeftX = SCOPE(sb).left;
        oldRightX = SCOPE(sb).right;

/* Can't be bothererd to pull in libm.a */
#ifndef floor
#define I_OWN_FLOOR
#define floor(A) ((float)RwInt32FromRealMacro(A))
#endif /* floor */

        /* Back/forward step the interpolants to y.5 */
        tmp = 0.5f-(left->y-floor(left->y));

#ifdef I_OWN_FLOOR
#undef floor
#undef I_OWN_FLOOR
#endif /* I_OWN_FLOOR */

        /* This is a rather dodgy use of macro expansion */
        FixedVectorAdd(left->fv, left->fv, tmp*left->dfvdy);
        RealVectorAdd(left->v, left->v, tmp*left->dvdy);

        SCOPE(sb).left += (RwFixed)
            RwInt32FromRealMacro(SCOPE(sb).dleft*tmp);
        SCOPE(sb).right += (RwFixed)
            RwInt32FromRealMacro(SCOPE(sb).dright*tmp);

        /* Up date these values here. rHeight will be destroyed. These values
           are not used in the rest of the trapfiller */
        left->y += rHeight;
        left->dy -= rHeight;
        left->dfy = REAL2FIX(left->dy);
        left->x += left->dxdy*rHeight;

        /* We may have grown/shrunk the triangle, so we adjust rHeight */
        /* This operation may cause rHeight to go negative! */
        rHeight -= tmp;

        while (height--)
        {
            RwFixed sampleX, fXprestep;
            RwInt32 seg_width;

            sampleX = SCOPE(sb).left;

            if (height) /* This trap exits bottom of pixel row */
            {
                SCOPE(sb).left += (RwFixed)(SCOPE(sb).dleft>>1);
                SCOPE(sb).right += (RwFixed)(SCOPE(sb).dright>>1);
                rHeight -= 0.5f;
            }
            else /* trap ends somewhere here */
            {
                SCOPE(sb).left += (RwFixed)
                    RwInt32FromRealMacro(SCOPE(sb).dleft*rHeight);
                SCOPE(sb).right += (RwFixed)
                    RwInt32FromRealMacro(SCOPE(sb).dright*rHeight);
            }

            seg_width = FxToInt(((oldRightX > SCOPE(sb).right)
                                 ? oldRightX : SCOPE(sb).right) + 0xffff);
            seg_width -= FxToInt((oldLeftX < SCOPE(sb).left)
                                 ? oldLeftX : SCOPE(sb).left);
            /* Set up scan line value inters */
            fXprestep = ((((oldLeftX < SCOPE(sb).left) ? 
                           oldLeftX : 
                           SCOPE(sb).left) & ~0xffff) | 0x8000) -sampleX;
            FixedVectorAddScaled(SCOPE(sb).seg_fv, left->fv,
                                 SCOPE(sb).seg_dfvdx, fXprestep);

#if (defined(INTERP_COUNT_REAL))
#if (INTERP_COUNT_REAL > 0)
            {
                RwInt32 i;
                
                for (i=0; i<INTERP_COUNT_REAL; i++)
                {
                    SCOPE(sb).seg_v.part.r[i]
                        = POSREAL2PROXY(left->v.part.r[i]
                                        +SCOPE(sb).dvdx.part.r[i]
                                        *FxToReal(fXprestep));
                }
            }
#endif /* (INTERP_COUNT_REAL > 0) */
#endif /* (defined(INTERP_COUNT_REAL)) */

            /* setup per-scanline destination */
            if (oldLeftX < SCOPE(sb).left)
            {
                DITHERCOL((oldLeftX>>16));
                SCANSETUP((oldLeftX>>16));
            }
            else
            {
                DITHERCOL((SCOPE(sb).left>>16));
                SCANSETUP((SCOPE(sb).left>>16));
            }

            /* Fill pixel line */
            while (seg_width > 0)
            {
                seg_width--;

                SEGPIXEL;

                /* bump segment pixel interpolators */
                FixedVectorAdd(SCOPE(sb).seg_fv, SCOPE(sb).seg_fv,
                               SCOPE(sb).seg_dfvdx);
                RealVectorProxyAdd(SCOPE(sb).seg_v, SCOPE(sb).seg_v,
                                   SCOPE(sb).seg_dvdx);

                /* bump dithering value */
                DITHERDX;
            }

            if (height)
            {
                /* Step into next pixel centre */
                oldLeftX = SCOPE(sb).left;
                oldRightX = SCOPE(sb).right;
                
                SCOPE(sb).left += (RwFixed)(SCOPE(sb).dleft>>1);
                SCOPE(sb).right += (RwFixed)(SCOPE(sb).dright>>1);
                rHeight -= 0.5f

                /* full unit step required here */
                FixedVectorAdd(left->fv, left->fv, left->dfvdy);
                RealVectorAdd(left->v, left->v, left->dvdy);

                /* bump dithering value */
                DITHERDY;

                /* User stuff */
                TRIDY;
            }
            else
            {
                /* Step to end of trap */
                /* This is a rather dodgy use of macro expansion */
                FixedVectorAdd(left->fv, left->fv, rHeight*left->dfvdy);
                RealVectorAdd(left->v, left->v, rHeight*left->dvdy);
            }
        }
    }
}
#endif /* !GREEDY */
#endif /* !TRAPFILL */


/*
 * This must be the last action in this file. RE.
 */
#ifndef PROLOG_REN
#define PROLOG_REN
#endif /* PROLOG_REN */

/* END OF PROLOG */
