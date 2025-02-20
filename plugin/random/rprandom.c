/*
 * Random number generation
 */

/**
 * \ingroup rprandom
 * \page rprandomoverview RpRandom Plugin Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rprandom.h
 * \li \b Libraries: rwcore, rprandom
 * \li \b Plugin \b attachments: \ref RpRandomPluginAttach
 *
 * \subsection randomoverview Overview
 * This plugin provides a 31-bit pseudo-random number generator using
 * a linear feedback shift register approach using trinomials. It is
 * based on research performed at the University of California, Berkeley.
 * (1983, 1993). All 31 bits may be considered random.
 *
 * It is provided as a RenderWare Graphics Plugin as a replacement for generators
 * used in the standard C libraries. This allows consistent random number
 * generation across all our supported platforms.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rpplugin.h"
#include "rpdbgerr.h"
#include "rprandom.h"

/*
 * rprandom.c:
 *
 * An improved random number generation package.
 *
 * The random number generation technique is a linear feedback shift register
 * approach, employing trinomials (since there are fewer terms to sum up that
 * way).  In this approach, the least significant bit of all the numbers in
 * the state table will act as a linear feedback shift register, and will
 * have period 2^deg - 1 (where deg is the degree of the polynomial being
 * used, assuming that the polynomial is irreducible and primitive).  The
 * higher order bits will have longer periods, since their values are also
 * influenced by pseudo-random carries out of the lower bits.  The total
 * period of the generator is approximately deg*(2**deg - 1); thus doubling
 * the amount of state information has a vast influence on the period of the
 * generator.  Note: the deg*(2**deg - 1) is an approximation only good for
 * large deg, when the period of the shift register is the dominant factor.
 * With deg equal to seven, the period is actually much longer than the
 * 7*(2**7 - 1) predicted by this formula.
 */

/*
 * Copyright (c) 1983, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 * NOTE: CSL 11/12/98
 *
 * Since the RW API does not currently expose the InitState/SetState
 * functionality, the code has been hard coded to only support a single
 * state (TYPE 3). This limitation also allows the amount of global state
 * required to be minimal.
 * For the complete code for the algorithm see the RenderWare 2 source code
 */

/*
 * For each of the currently supported random number generators, we have a
 * break value on the amount of state information (you need at least this
 * many bytes of state info to support this random number generator), a degree
 * for the polynomial (actually a trinomial) that the R.N.G. is based on, and
 * the separation between the two lower order coefficients of the trinomial.
 */

#define TYPE_3      3              /* x**31 + x**3 + 1 */
#define BREAK_3     128
#define DEG_3       31
#define SEP_3       3

#if (defined(RWDEBUG))
long                rpRandomStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

/* This isn't as bad as it sounds!!!! ;-) */
#define RWRANDOMGLOBAL(var) \
    (RWPLUGINOFFSET(RandomGlobals, RwEngineInstance, GlobalOffset)->var)

static RwInt32      GlobalOffset = 0; /* Offset into global data */

/****************************************************************************
 Global Types
 */

typedef struct RandomGlobals RandomGlobals;
struct RandomGlobals
{
    RwUInt32           *randomTable;
    RwUInt32           *fptr;
    RwUInt32           *rptr;
    RwUInt32           *end_ptr;
};

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                      Random Number Generator Functions

 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/****************************************************************************
 RandomClose

 On entry   : instance, data offset, data size
 On exit    : instance ptr on success
 */

static void        *
RandomClose(void *instance,
            RwInt32 __RWUNUSED__ offset, RwInt32 __RWUNUSED__ size)
{
    RWFUNCTION(RWSTRING("RandomClose"));

    /* Free it up */
    if (RWRANDOMGLOBAL(randomTable))
    {
        RwFree(RWRANDOMGLOBAL(randomTable));
        RWRANDOMGLOBAL(randomTable) = (RwUInt32 *) NULL;
    }

    /* Success ! */
    RWRETURN(instance);
}

/****************************************************************************
 RandomOpen

 On entry   : instance, data offset, data size
 On exit    : instance ptr on success
 */

static void        *
RandomOpen(void *instance,
           RwInt32 __RWUNUSED__ offset, RwInt32 __RWUNUSED__ size)
{
    RWFUNCTION(RWSTRING("RandomOpen"));

    /* And continue with the initialization */
    RWRANDOMGLOBAL(randomTable) =
        (RwUInt32 *) RwMalloc(DEG_3 * sizeof(RwUInt32),
        rwID_RANDOMPLUGIN | rwMEMHINTDUR_GLOBAL);
    if (!RWRANDOMGLOBAL(randomTable))
    {
        /* Failure */
        RWRETURN(NULL);
    }
    RWRANDOMGLOBAL(fptr) = &RWRANDOMGLOBAL(randomTable)[SEP_3];
    RWRANDOMGLOBAL(rptr) = RWRANDOMGLOBAL(randomTable);
    RWRANDOMGLOBAL(end_ptr) = &RWRANDOMGLOBAL(randomTable)[DEG_3];

    /* Initialise the table */
    RpRandomSeed(0x9a319039UL);

    RWRETURN(instance);
}

/**
 * \ingroup rprandom
 * \ref RpRandomSeed is used to initialize the random number generator
 * using the specified seed value.
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
 * \see RpRandom
 *
 */
void
RpRandomSeed(RwUInt32 seed)
{
    register RwInt32    i;

    RWAPIFUNCTION(RWSTRING("RpRandomSeed"));

    /*
     * srandom:
     *
     * Initialize the random number generator based on the given seed.  If the
     * type is the trivial no-state-information type, just remember the seed.
     * Otherwise, initializes state[] based on the given "seed" via a linear
     * congruential generator.  Then, the pointers are set to known locations
     * that are exactly rand_sep places apart.  Lastly, it cycles the state
     * information a given number of times to get rid of any initial dependencies
     * introduced by the L.C.R.N.G.  Note that the initialization of randtbl[]
     * for default usage relies on values produced by this routine.
     */

    RWRANDOMGLOBAL(randomTable)[0] = seed;

    for (i = 1; i < DEG_3; i++)
    {
        RWRANDOMGLOBAL(randomTable)[i] =
            1103515245 * RWRANDOMGLOBAL(randomTable)[i - 1] + 12345;
    }
    RWRANDOMGLOBAL(fptr) = &RWRANDOMGLOBAL(randomTable)[SEP_3];
    RWRANDOMGLOBAL(rptr) = &RWRANDOMGLOBAL(randomTable)[0];

    for (i = 0; i < 10 * DEG_3; i++)
    {
        (void) RpRandom();
    }
    RWRETURNVOID();
}

/**
 * \ingroup rprandom
 * \ref RpRandom is used to generate a 31-bit pseudo-random number.  It
 * uses a non-linear additive feedback random number generator employing a
 * default table of size 31 long integers to return successive pseudo-random
 * numbers in the range 0 to pow(2,31)-1.  The period of this random number
 * generator is very large, approximately 16(pow(2,31)-1).
 *
 * Note that all the bits generated by RpRandom are usable, for example
 * RpRandom() &01 will produce a random binary value.
 *
 * The random number plugin must be attached before using this function.
 *
 * \return a RwUInt32 value containing a pseudo-random number
 *
 * \see RpRandomSeed
 */
RwUInt32
RpRandom(void)
{
    RwInt32             i;

    RWAPIFUNCTION(RWSTRING("RpRandom"));

    /*
     * RpRandom:
     *
     * If we are using the trivial TYPE_0 R.N.G., just do the old linear
     * congruential bit.  Otherwise, we do our fancy trinomial stuff, which is
     * the same in all the other cases due to all the global variables that have
     * been set up.  The basic operation is to add the number at the rear pointer
     * into the one at the front pointer.  Then both pointers are advanced to
     * the next location cyclically in the table.  The value returned is the sum
     * generated, reduced to 31 bits by throwing away the "least random" low bit.
     *
     * Note: the code takes advantage of the fact that both the front and
     * rear pointers can't wrap on the same call by not testing the rear
     * pointer if the front one has wrapped.
     *
     * Returns a 31-bit random number.
     */
    *RWRANDOMGLOBAL(fptr) += *RWRANDOMGLOBAL(rptr);

    /* chucking least random bit */

    i = (*RWRANDOMGLOBAL(fptr) >> 1) & 0x7fffffff;
    if (++RWRANDOMGLOBAL(fptr) >= RWRANDOMGLOBAL(end_ptr))
    {
        RWRANDOMGLOBAL(fptr) = RWRANDOMGLOBAL(randomTable);
        ++RWRANDOMGLOBAL(rptr);
    }
    else if (++RWRANDOMGLOBAL(rptr) >= RWRANDOMGLOBAL(end_ptr))
        RWRANDOMGLOBAL(rptr) = RWRANDOMGLOBAL(randomTable);
    RWRETURN(i);
}

/**
 * \ingroup rprandom
 * \ref RpRandomPluginAttach is used to attach the random number plugin
 * to the RenderWare system to enable the generation of random integer values.
 * The random plugin must be attached between initializing the system with
 * RwEngineInit and opening it with RwEngineOpen.
 *
 * Note that the include file rprandom.h is required and must be included by
 * an application wishing to use random numbers.  The random plugin library
 * is contained in the file rprandom.lib
 *
 * The call to this function should occur after \ref RwEngineInit.
 *
 * \return True on success, false otherwise.
 *
 * \see RwEngineInit
 * \see RwEngineOpen
 * \see RwEngineStart
 */
RwBool
RpRandomPluginAttach(void)
{
    RWAPIFUNCTION(RWSTRING("RpRandomPluginAttach"));

    /* Extend the global data block to include Random globals */

    GlobalOffset =
        RwEngineRegisterPlugin(sizeof(RandomGlobals),
                               rwID_RANDOMPLUGIN,
                               RandomOpen, RandomClose);

    if (GlobalOffset < 0)
    {
        RWRETURN(FALSE);
    }

    RWRETURN(TRUE);
}
