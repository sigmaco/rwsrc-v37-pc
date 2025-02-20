/* START OF STRUCT.REN */

#ifdef PREFIX
#define PASTE(A, B)       A ## B
#define XPASTE(A, B)      PASTE(A, B)
#ifdef __R5900__
#define XPREFIX(WORD, PR) XPASTE(PR,WORD)
#else /* __R5900__ */
#define XPREFIX(WORD, PR) XPASTE(_Pro, XPASTE(PR, WORD))
#endif /* __R5900__ */
#define SCOPE(WORD)       XPREFIX(WORD, PREFIX)
/* Convience defines */
#define SB SCOPE(sb)
#define EDGE SCOPE(edge)
#else /* PREFIX */
/* Old behavior */
#define SCOPE(WORD) WORD
#endif /* PREFIX */

/* provide default implementations of all macros */
#ifndef INTERP_COUNT_REALP
#define INTERP_COUNT_REALP 0
#endif
#ifndef INTERP_COUNT_REAL
#define INTERP_COUNT_REAL 0
#endif
#ifndef INTERP_COUNT_FIXED
#define INTERP_COUNT_FIXED 0
#endif
#if INTERP_COUNT_REALP > 0
#ifndef INTERP_Q
#define INTERP_Q
#endif
#endif

#ifndef TRIDECL
#define TRIDECL
#endif

#ifdef REALPROXY
#define RealProxy RwReal
#else
#define RealProxy RwFixed
#endif

/* declare custom rasterisation structures */
typedef struct
{
#ifdef INTERP_Q
    RwReal q;
#endif
#if INTERP_COUNT_REALP > 0
    RwReal rp[INTERP_COUNT_REALP];
#endif
#if INTERP_COUNT_REAL > 0
    RwReal r[INTERP_COUNT_REAL];
#endif
    int dummy;
} SCOPE(PartitionedRealVector);

#ifdef INTERP_Q
#define REALVECTORLEN (1+INTERP_COUNT_REALP+INTERP_COUNT_REAL)
#else
#define REALVECTORLEN (INTERP_COUNT_REALP+INTERP_COUNT_REAL)
#endif
typedef union
{
    RwReal all[REALVECTORLEN+1];
    SCOPE(PartitionedRealVector) part;
} SCOPE(RealVector);

typedef struct
{
#if INTERP_COUNT_REALP > 0
    RealProxy rp[INTERP_COUNT_REALP];
#endif
#if INTERP_COUNT_REAL > 0
    RealProxy r[INTERP_COUNT_REAL];
#endif
    int dummy;
} SCOPE(PartitionedRealVectorProxy);

#define REALVECTORPROXYLEN (INTERP_COUNT_REALP+INTERP_COUNT_REAL)
typedef union
{
    RealProxy all[REALVECTORPROXYLEN+1];
    SCOPE(PartitionedRealVectorProxy) part;
} SCOPE(RealVectorProxy);

#define FIXEDVECTORLEN (INTERP_COUNT_FIXED)
typedef struct
{
    RwFixed all[FIXEDVECTORLEN+1];
} SCOPE(FixedVector);

typedef struct SCOPE(edgeBlocktag)
{
    RwReal x,dxdy;
    RwReal y;
    RwReal dy;
    RwFixed dfy;

    SCOPE(RealVector) v;
    SCOPE(RealVector) dvdy;

    SCOPE(FixedVector) fv;
    SCOPE(FixedVector) dfvdy;
} SCOPE(EdgeBlock);

typedef struct SCOPE(scanBlocktag)
{
    RwFixed left,dleft,right,dright;

    /* interpolated interior values */
    SCOPE(RealVector) v;
    SCOPE(RealVector) dvdx;
    SCOPE(RealVector) dvdy;

    /* intra-segment interpolators (fixedpoint) */
    SCOPE(RealVectorProxy) seg_v;
    SCOPE(RealVectorProxy) seg_dvdx;
    RwFixed seg_round[INTERP_COUNT_REALP+1];

    SCOPE(FixedVector) seg_fv;
    SCOPE(FixedVector) seg_dfvdx;
    SCOPE(FixedVector) seg_dfvdy;

#ifdef DITHER
    unsigned long dithline;
#endif
        /* destination state */
    TRIDECL
} SCOPE(ScanBlock);
