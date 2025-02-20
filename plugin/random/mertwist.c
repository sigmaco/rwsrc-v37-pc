/*
 * Random number generation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rpplugin.h"
#include "rpdbgerr.h"
#include "rprandom.h"

/*
 *  This is the ``Mersenne Twister'' random number generator MT19937, which
 * generates pseudorandom integers uniformly distributed in 0..(2^32 - 1)
 * starting from any odd seed in 0..(2^32 - 1).  This version is a recode
 *
 * According to the URL <http://www.math.keio.ac.jp/~matumoto/emt.html>
 * (and paraphrasing a bit in places), the Mersenne Twister is ``designed
 * with consideration of the flaws of various existing generators,'' has
 * a period of 2^19937 - 1, gives a sequence that is 623-dimensionally
 * equidistributed, and ``has passed many stringent tests, including the
 * die-hard test of G. Marsaglia and the load test of P. Hellekalek and
 * S. Wegenkittl.''  It is efficient in memory usage (typically using 2506
 * to 5012 bytes of static data, depending on data type sizes, and the code
 * is quite short as well).  It generates random numbers in batches of 624
 * at a time, so the caching and pipelining of modern systems is exploited.
 * It is also divide- and mod- free.
 */

/* length of state vector */
#define N              (624)
/* a period parameter */
#define M              (397)
/* a magic constant */
#define K              (0x9908B0DFU)
/* mask all but highest bit of u */
#define hiBit(u)       ((u) & 0x80000000U)
/* mask all but lowest bit of u */
#define loBit(u)       ((u) & 0x00000001U)
/* mask     the highest bit of u */
#define loBits(u)      ((u) & 0x7FFFFFFFU)
/* move hi bit of u to hi bit of v */
#define mixBits(u, v)  (hiBit(u)|loBits(v))

/* state vector + 1 extra to not violate ANSI C */
static RwUInt32     state[N + 1];

/* next random value is computed from here */
static RwUInt32    *next;

/* can *next++ this many times before reloading */
static RwInt32      left = -1;

#define seedMTMacro(seed)                                               \
MACRO_START                                                             \
{                                                                       \
    register RwUInt32 x = (seed | 1U) & 0xFFFFFFFFU;                    \
    register RwUInt32 *s = state;                                       \
    register RwInt32        j = N;                                      \
                                                                        \
    /*                                                                  \
     * We initialize state[0..(N-1)] via the generator                  \
     *                                                                  \
     *   x_new = (69069 * x_old) mod 2^32                               \
     *                                                                  \
     * from Line 15 of Table 1, p. 106, Sec. 3.3.4 of Knuth's           \
     * _The Art of Computer Programming_, Volume 2, 3rd ed.             \
     */                                                                 \
                                                                        \
    for (left = 0, *s++ = x; --j; *s++ = x & 0xFFFFFFFFU)               \
    {                                                                   \
        x *= 69069U;                                                    \
    }                                                                   \
}                                                                       \
MACRO_STOP

#define reloadMTMacro(result)                                           \
MACRO_START                                                             \
{                                                                       \
    register RwUInt32 *p0 = state;                                      \
    register RwUInt32 *p2 = state + 2;                                  \
    register RwUInt32 *pM = state + M;                                  \
    register RwUInt32 s0;                                               \
    register RwUInt32 s1;                                               \
    register RwInt32        j;                                          \
                                                                        \
    if (left < -1)                                                      \
        seedMTMacro(4357U);                                             \
                                                                        \
    left = N - 1;                                                       \
    next = state + 1;                                                   \
                                                                        \
    for (s0 = state[0], s1 = state[1], j = N - M + 1;                   \
         --j;                                                           \
         s0 = s1, s1 = *p2++)                                           \
    {                                                                   \
        *p0++ =                                                         \
            *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);      \
    }                                                                   \
                                                                        \
    for (pM = state, j = M; --j; s0 = s1, s1 = *p2++)                   \
    {                                                                   \
        *p0++ =                                                         \
            *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);      \
    }                                                                   \
                                                                        \
    s1 = state[0];                                                      \
    *p0 = *pM ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);          \
    s1 ^= (s1 >> 11);                                                   \
    s1 ^= (s1 << 7) & 0x9D2C5680U;                                      \
    s1 ^= (s1 << 15) & 0xEFC60000U;                                     \
                                                                        \
    result = (s1 ^ (s1 >> 18));                                         \
}                                                                       \
MACRO_STOP

#define randomMTMacro(result, _left)                                    \
MACRO_START                                                             \
{                                                                       \
    if (--_left < 0)                                                    \
    {                                                                   \
        reloadMTMacro(result);                                          \
    }                                                                   \
    else                                                                \
    {                                                                   \
        result = *next++;                                               \
        result ^= (result >> 11);                                       \
        result ^= (result << 7) & 0x9D2C5680U;                          \
        result ^= (result << 15) & 0xEFC60000U;                         \
        result = (result ^ (result >> 18));                             \
    }                                                                   \
}                                                                       \
MACRO_STOP

/**
 * \ingroup rprandom
 * \ref RpRandomSeedMT is used to initialize the Mersenne Twister
 * random number generator  using the specified seed value.
 *
 * Note that this function does not return the old seed.  Therefore, if it
 * is required to reproduce a sequence of random numbers, the seed must be
 * set with a user-defined value before any numbers are generated.  The seed
 * can then be used again to generate the same numbers.
 *
 * The random number plugin must be attached before using this function.
 *
 * \param seed  The seed value for random number generation.
 *
 * \see RpRandomMT
 */
void
RpRandomSeedMT(RwUInt32 seed)
{
    RWAPIFUNCTION(RWSTRING("RpRandomSeedMT"));

    seedMTMacro(seed);

    RWRETURNVOID();
}

/**
 * \ingroup rprandom
 * \ref RpRandomMT is used to generate ``Mersenne Twister'' (MT19937)
 * random numbers.
 * The Mersenne Twister generator produces pseudorandom integers
 * uniformly distributed in 0..(2^32 - 1) starting from
 * any odd seed in 0..(2^32 - 1).
 *
 * According to the URL <http://www.math.keio.ac.jp/~matumoto/emt.html>
 * (and paraphrasing a bit in places), the Mersenne Twister is ``designed
 * with consideration of the flaws of various existing generators,'' has
 * a period of 2^19937 - 1, gives a sequence that is 623-dimensionally
 * equidistributed, and ``has passed many stringent tests, including the
 * die-hard test of G. Marsaglia and the load test of P. Hellekalek and
 * S. Wegenkittl.''  It is efficient in memory usage (typically using 2506
 * to 5012 bytes of static data, depending on data type sizes, and the code
 * is quite short as well).  It generates random numbers in batches of 624
 * at a time, so the caching and pipelining of modern systems is exploited.
 * It is also divide- and mod- free.
 *
 * The random number plugin must be attached before using this function.
 *
 * \return a RwUInt32 value containing a pseudo-random number
 *
 * \see RpRandomSeedMT
 */
RwUInt32
RpRandomMT(void)
{
    RwUInt32            result;

    RWAPIFUNCTION(RWSTRING("RpRandomMT"));

    randomMTMacro(result, left);

    RWRETURN(result);
}
