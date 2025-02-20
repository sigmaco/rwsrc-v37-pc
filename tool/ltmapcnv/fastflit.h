#ifndef _FASTFLIT_H
#define _FASTFLIT_H

#if (defined(_M_IX86) && !defined(__GNUC__))
__inline static
int Double2int(double x)
{
    int res;

    __asm FLD QWord Ptr[x]
    __asm FISTP DWord Ptr[res]

    return(res);
}
#else
#define Double2int(X) ((int)(X))
#endif

#if (defined(_M_IX86) && !defined(__GNUC__))
__inline static
int Float2int(float x)
{
    int res;

    __asm FLD DWord Ptr[x]
    __asm FISTP DWord Ptr[res]

    return(res);
}
#else
#define Float2int(X) ((int)(X))
#endif

#if (defined(_M_IX86) && !defined(__GNUC__))
/* Warning: Only use where no other fpu stuff happens */

__inline static
void startRcip(float x)
{
    float one = 1.0f;

    __asm FLD DWord Ptr[one]
    __asm FDIV DWord Ptr[x]
}

__inline static
float endRcip()
{
    float res;

    __asm FSTP DWord Ptr[res]

    return(res);
}
#endif

/* An alternate float to int */

/* This has a limited bit count 22, so watch out in FLOAT2FIX etc */
#define AltFloat2int(A) (((int)(IntAdjTmp=((A)+12582912.0f))),(*(int*)(&IntAdjTmp)-0x4b400000))

/* A table based reciprocal */

#define RECIPRES 14

#if (RECIPRES > 23)
#error Only 23 bits of mantissa are avaliable for reciprocal table
#endif /* (RECIPRES > 23) */

#ifdef    __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

extern unsigned int _rw4reciptable[1<<(RECIPRES)];

#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

/* We use add here so that if the mantissa was 1.0, we can bump the exp */
#define TableRecip(DST, SRC) \
*(unsigned int*)&(DST) =						\
_rw4reciptable[(((*(unsigned int *)&(SRC))&0x007fffff)>>(23-RECIPRES))]	\
      + (0x7e800000 - ((*(unsigned int*)&(SRC))&0xff800000))

#endif /* _FASTFLIT_H */
