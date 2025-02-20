/* START OF BODY */

    {
        SCOPE(EdgeBlock) *trap[2];
        SCOPE(EdgeOrder) *eorder;
        RwReal oodx,Yprestep;
        RwFixed foodx,fYprestep,ytop = (RwFixed)0;
        int loop, height;
#ifdef GREEDY
        RwReal rHeight;
#endif /* GREEDY */

        /* edge dy */
        SCOPE(edge)[0].dy = SCOPE(edge)[1].y - SCOPE(edge)[0].y;
        SCOPE(edge)[0].dfy = REAL2FIX(SCOPE(edge)[0].dy);
        SCOPE(edge)[1].dy = SCOPE(edge)[2].y - SCOPE(edge)[1].y;
        SCOPE(edge)[1].dfy = REAL2FIX(SCOPE(edge)[1].dy);
        SCOPE(edge)[2].dy = SCOPE(edge)[0].y - SCOPE(edge)[2].y;
        SCOPE(edge)[2].dfy = REAL2FIX(SCOPE(edge)[2].dy);

        oodx = 1.0f / ((((SCOPE(edge)[1].x - SCOPE(edge)[2].x)) * (
                            SCOPE(edge)[2].dy))
                       - (((SCOPE(edge)[0].x - SCOPE(edge)[2].x)) * (
                              -SCOPE(edge)[1].dy)));
        foodx = REAL2FIX(oodx);

        /* Core interpolator setup NB Should really convert SCOPE(edge).x to
           Fixed - we do it later anyway */
#if 0
        for (loop=0; loop<FIXEDVECTORLEN; loop++)
        {
            SCOPE(sb).seg_dfvdx.all[loop]
                      = RwFMul(foodx, RwFMul((SCOPE(edge)[1].fv.all[loop]
                                              - SCOPE(edge)[2].fv.all[loop]),
                                             SCOPE(edge)[2].dfy)
                                      - RwFMul((SCOPE(edge)[0].fv.all[loop]
                                                - SCOPE(edge)[2].fv.all[loop]),
                                               - SCOPE(edge)[1].dfy));

            SCOPE(sb).seg_dfvdy.all[loop]
                      = -RwFMul(foodx, RwFMul((SCOPE(edge)[1].fv.all[loop]
                                               - SCOPE(edge)[2].fv.all[loop]),
                                              REAL2FIX(SCOPE(edge)[0].x
                                                       - SCOPE(edge)[2].x))
                                       - RwFMul((SCOPE(edge)[0].fv.all[loop]
                                                 - SCOPE(edge)[2].fv.all[loop]),
                                                REAL2FIX(SCOPE(edge)[1].x
                                                         - SCOPE(edge)[2].x)));
        }
#else
        for (loop=0; loop<FIXEDVECTORLEN; loop++)
        {
            SCOPE(sb).seg_dfvdx.all[loop]
                = REAL2FIX(((oodx) * (
                                ((FxToReal(SCOPE(edge)[1].fv.all[loop]
                                              - SCOPE(edge)[2].fv.all[loop])) * (
                                     SCOPE(edge)[2].dy))
                                - ((FxToReal(SCOPE(edge)[0].fv.all[loop]
                                                - SCOPE(edge)[2].fv.all[loop])) * (
                                       -SCOPE(edge)[1].dy)))));

            SCOPE(sb).seg_dfvdy.all[loop]
                = -REAL2FIX(((oodx) * (
                                 ((FxToReal(SCOPE(edge)[1].fv.all[loop]
                                               - SCOPE(edge)[2].fv.all[loop])) * (
                                      (SCOPE(edge)[0].x - SCOPE(edge)[2].x)))
                                 - ((FxToReal(SCOPE(edge)[0].fv.all[loop]
                                                 -SCOPE(edge)[2].fv.all[loop])) * (
                                        (SCOPE(edge)[1].x-SCOPE(edge)[2].x))))));
        }
#endif
        for (loop=0; loop<REALVECTORLEN; loop++)
        {
            SCOPE(sb).dvdx.all[loop]
                      = ((oodx) * ( (((SCOPE(edge)[1].v.all[loop]
                                         - SCOPE(edge)[2].v.all[loop])) * (
                                        SCOPE(edge)[2].dy))
                                   - (((SCOPE(edge)[0].v.all[loop]
                                           - SCOPE(edge)[2].v.all[loop])) * (
                                          -SCOPE(edge)[1].dy))));

            SCOPE(sb).dvdy.all[loop]
                     = -((oodx) * ( (((SCOPE(edge)[1].v.all[loop]
                                         - SCOPE(edge)[2].v.all[loop])) * (
                                        (SCOPE(edge)[0].x - SCOPE(edge)[2].x)))
                                   - (((SCOPE(edge)[0].v.all[loop]
                                           - SCOPE(edge)[2].v.all[loop])) * (
                                          (SCOPE(edge)[1].x-SCOPE(edge)[2].x)))));
        }
#if INTERP_COUNT_REALP > 0
        for (loop=0; loop<INTERP_COUNT_REALP; loop++)
        {
            SCOPE(sb).seg_round[loop]
                = (SCOPE(sb).dvdx.part.rp[loop] < (RwReal)(0)) ? ASEGRND : 0;
        }
#endif

        /* Edge interpolator setup (should avoid divs) */
        if (SCOPE(edge)[0].dfy)
            SCOPE(edge)[0].dxdy = (SCOPE(edge)[1].x - SCOPE(edge)[0].x)
                                  / SCOPE(edge)[0].dy;
        if (SCOPE(edge)[1].dfy)
            SCOPE(edge)[1].dxdy = (SCOPE(edge)[2].x - SCOPE(edge)[1].x)
                                  / SCOPE(edge)[1].dy;
        if (SCOPE(edge)[2].dfy)
            SCOPE(edge)[2].dxdy = (SCOPE(edge)[0].x - SCOPE(edge)[2].x)
                                  / SCOPE(edge)[2].dy;

        /* Sort on Y */
        eorder = &SCOPE(order)[((RwUInt32)(SCOPE(edge)[0].dfy)>>31)
                | ((RwUInt32)(SCOPE(edge)[1].dfy)>>31<<1)
                | ((RwUInt32)(SCOPE(edge)[2].dfy)>>31<<2)];
        trap[LEFT] = eorder->left;
        trap[RIGHT] = eorder->right;

        /* setup SCOPE(edge) interpolator steps */
        SCOPE(sb).dleft = REAL2FIX(trap[LEFT]->dxdy);
        SCOPE(sb).dright = REAL2FIX(trap[RIGHT]->dxdy);
        FixedVectorAddScaled(trap[LEFT]->dfvdy, SCOPE(sb).seg_dfvdy,
                             SCOPE(sb).seg_dfvdx, SCOPE(sb).dleft);
        RealVectorAddScaled(trap[LEFT]->dvdy, SCOPE(sb).dvdy, SCOPE(sb).dvdx,
                            trap[LEFT]->dxdy);

        /* calc Yprestep */
#ifndef GREEDY
        ytop = POSREAL2FIX(trap[LEFT]->y - (RwReal)(0.5));
        fYprestep = ((ytop - 1) & 0xffff) ^ 0xffff;
        Yprestep = FxToReal(fYprestep);

        /* Yprestep left SCOPE(edge) */
        trap[LEFT]->x += trap[LEFT]->dxdy * Yprestep;
        SCOPE(sb).left = POSREAL2FIX(trap[LEFT]->x - (RwReal)(0.5));
        FixedVectorAddScaled(trap[LEFT]->fv, trap[LEFT]->fv, trap[LEFT]->dfvdy,
                             fYprestep);
        RealVectorAddScaled(trap[LEFT]->v, trap[LEFT]->v, trap[LEFT]->dvdy,
                            Yprestep);

        /* Yprestep right SCOPE(edge) */
        trap[RIGHT]->x += trap[RIGHT]->dxdy * (trap[RIGHT]->dy + Yprestep);
        SCOPE(sb).right = POSREAL2FIX(trap[RIGHT]->x - (RwReal)(0.5));
        trap[RIGHT]->dy = -trap[RIGHT]->dy;   /* may be used to calc height */
        trap[RIGHT]->dfy = -trap[RIGHT]->dfy; /* may be used to calc height */
#else /* !GREEDY */
        ytop = POSREAL2FIX(trap[LEFT]->y);
        fYprestep = 0;
        Yprestep = (RwReal)0.0f;

        /* Set up initial scan X */
        /* trap[LEFT]->x is correct */
        SCOPE(sb).left = POSREAL2FIX(trap[LEFT]->x);
#if 0
        /* We know the presteps are 0 here, hece the if 0 */
        FixedVectorAddScaled(trap[LEFT]->fv, trap[LEFT]->fv, trap[LEFT]->dfvdy,
                             fYprestep); 
        RealVectorAddScaled(trap[LEFT]->v, trap[LEFT]->v, trap[LEFT]->dvdy,
                            Yprestep);
#endif /* 0 */

        /* Yprestep right SCOPE(edge) */
        trap[RIGHT]->x += trap[RIGHT]->dxdy * (trap[RIGHT]->dy /* +Yprestep */);
        SCOPE(sb).right = POSREAL2FIX(trap[RIGHT]->x /* - (RwReal)(0.5) */);
        trap[RIGHT]->dy = -trap[RIGHT]->dy;   /* may be used to calc height */
        trap[RIGHT]->dfy = -trap[RIGHT]->dfy; /* may be used to calc height */
#endif /* !GREEDY */

        /* how many scanlines */
#ifndef GREEDY
        height = FxToInt(trap[eorder->replace]->dfy - fYprestep + 0xffff);
#else /* !GREEDY */
        /* We need to hit all included scan lines. To avoid the zero height
           problem, I'm assuming that something from y=1 to y=2 hits only
           scanline y=1 */
        height = FxToInt((ytop & 0xffff) + trap[eorder->replace]->dfy
                         + 0xffff);
        /* We also need to wset up the real height to permit edge increments
           to happen correctly */
        if (trap[RIGHT]->dy > trap[LEFT]->dy)
        {
            rHeight = trap[LEFT]->dy;
        }
        else
        {
            rHeight = trap[RIGHT]->dy;
        }
#endif /* !GREEDY */

        /* Proxy interpolator setup */
        RealVectorProxyAssign(SCOPE(sb).seg_dvdx.part.r, SCOPE(sb).dvdx.part.r);

        /* dither line seed */
        DITHERLINE(FxToInt(ytop + fYprestep));

        /* setup per-triangle destination */
        TRISETUP(FxToInt(ytop + fYprestep));

#ifndef TRAPFILL
#ifndef GREEDY
        SCOPE(TrapFill)(trap[LEFT], height);
#else /* !GREEDY */
        SCOPE(TrapFill)(trap[LEFT], height, rHeight);
#endif /* !GREEDY */
#else /* TRAPFILL */
#ifndef GREEDY
        TRAPFILL(trap[LEFT], height);
#else /* !GREEDY */
        TRAPFILL(trap[LEFT], height, rHeight);
#endif /* !GREEDY */
#endif /* TRAPFILL */

#ifdef GREEDY
        /* I'm asuming the the trapfiller will leave the edge stepped on to
           the real end, rather than the next integer y */
#endif
        /* get next SCOPE(edge) of trapezium */
        if (eorder->replace == LEFT)
        {
            trap[LEFT] = eorder->next;

            /* setup SCOPE(edge) interpolator steps */
            SCOPE(sb).dleft = REAL2FIX(trap[LEFT]->dxdy);
            FixedVectorAddScaled(trap[LEFT]->dfvdy, SCOPE(sb).seg_dfvdy,
                                 SCOPE(sb).seg_dfvdx, SCOPE(sb).dleft);
            RealVectorAddScaled(trap[LEFT]->dvdy, SCOPE(sb).dvdy,
                                SCOPE(sb).dvdx, trap[LEFT]->dxdy);

            /* Yprestep left SCOPE(edge) */
#ifndef GREEDY
            fYprestep = ((POSREAL2FIX(trap[LEFT]->y - (RwReal)(0.5)) - 1) & 0xffff)
                                                                ^ 0xffff;
            Yprestep = FxToReal(fYprestep);
            trap[LEFT]->x += trap[LEFT]->dxdy * Yprestep;
            SCOPE(sb).left = POSREAL2FIX(trap[LEFT]->x - (RwReal)(0.5));
#else /* !GREEDY */
            SCOPE(sb).left = POSREAL2FIX(trap[LEFT]->x);
#endif /* !GREEDY */
#ifndef GREEDY
            FixedVectorAddScaled(trap[LEFT]->fv, trap[LEFT]->fv,
                                 trap[LEFT]->dfvdy, fYprestep);
            RealVectorAddScaled(trap[LEFT]->v, trap[LEFT]->v, trap[LEFT]->dvdy,
                                Yprestep);
#endif /* !GREEDY */

            /* how many scanlines */
#ifndef GREEDY
            height = FxToInt(trap[LEFT]->dfy - fYprestep + 0xffff);
#else /* !GREEDY */
            height = FxToInt((POSREAL2FIX(trap[LEFT]->y) & 0xffff)
                             + trap[LEFT]->dfy + 0xffff);
            rHeight = trap[LEFT]->dy;
#endif /* !GREEDY */
        }
        else
        {
            /* setup SCOPE(edge) interpolator steps */
            SCOPE(sb).dright = REAL2FIX(eorder->next->dxdy);

            /* Yprestep (NB we pick up Y from previous SCOPE(edge)) */
#ifndef GREEDY
            fYprestep = ((POSREAL2FIX(trap[RIGHT]->y - (RwReal)(0.5)) - 1)
                         & 0xffff) ^ 0xffff;
            Yprestep = FxToReal(fYprestep);
#else /* !GREEDY */
            /* presteps are already set to 0 */
#endif /* GREEDY */
            trap[RIGHT] = eorder->next;
#ifndef GREEDY
            trap[RIGHT]->x += trap[RIGHT]->dxdy * (trap[RIGHT]->dy + Yprestep);
            SCOPE(sb).right = POSREAL2FIX(trap[RIGHT]->x - (RwReal)(0.5));
#else /* !GREEDY */
            trap[RIGHT]->x += trap[RIGHT]->dxdy * (trap[RIGHT]->dy);
            SCOPE(sb).right = POSREAL2FIX(trap[RIGHT]->x);
#endif /* !GREEDY */

            /* how many scanlines */
#ifndef GREEDY
            height = FxToInt(-trap[RIGHT]->dfy - fYprestep + 0xffff);
#else /* !GREEDY */
            height = FxToInt((POSREAL2FIX(trap[RIGHT]->y + trap[RIGHT]->dy)
                              & 0xffff)
                             - trap[RIGHT]->dfy + 0xffff);
            rHeight = -trap[RIGHT]->dy;
#endif /* !GREEDY */
        }

#ifndef TRAPFILL
#ifndef GREEDY
        SCOPE(TrapFill)(trap[LEFT], height);
#else /* !GREEDY */
        SCOPE(TrapFill)(trap[LEFT], height, rHeight);
#endif /* !GREEDY */
#else /* TRAPFILL */
#ifndef GREEDY
        TRAPFILL(trap[LEFT], height);
#else /* !GREEDY */
        TRAPFILL(trap[LEFT], height, rHeight);
#endif /* !GREEDY */
#endif /* TRAPFILL */
    }

/* END OF BODY */

/* We may need to undef to allow for multiple inclusion */
#ifdef PREFIX

#undef PREFIX
#undef PASTE
#undef XPASTE
#undef XPREFIX
#undef SCOPE
#undef SB
#undef EDGE
#undef INTERP_COUNT_REALP
#undef INTERP_COUNT_REAL
#undef INTERP_COUNT_FIXED
#undef INTERP_Q
#undef REALPROXY
#undef OOQBEGIN
#undef OOQEND
#undef POSREAL2INT
#undef REAL2INT
#undef POSREAL2FIX
#undef REAL2FIX
#undef RealProxy
#undef POSREAL2PROXY
#undef REAL2PROXY
#undef TRIDECL
#undef TRISETUP
#undef TRIDY
#undef SCANDECL
#undef SCANSETUP
#undef SEGDECL
#undef SEGBEGIN
#undef SEGPIXEL
#undef SEGEND
#undef DITHER
#undef DITHERDECL
#undef DITHERLINE
#undef DITHERCOL
#undef DITHERDX
#undef DITHERDY
#undef REALVECTORLEN
#undef REALVECTORPROXYLEN
#undef FIXEDVECTORLEN
#undef LEFT
#undef RIGHT
#undef ASEGLEN
#undef ASEGRND
#undef ASEGSHIFT
#undef RealVectorAdd
#undef RealVectorAddScaled
#undef PCRealVectorAdd
#undef PCRealVectorAddScaled
#undef RealVectorProxyAdd
#undef RealVectorProxyAssign
#undef PCRealVectorProxyAssign
#undef PCRealVectorProxyDxAssign
#undef PCRealVectorProxyDxAssign16
#undef FixedVectorAdd
#undef FixedVectorAddScaled
#undef SEG16PIXEL
#undef SEGNPIXEL
#undef TRAPFILL
#undef GREEDY

#else /* PREFIX */

/* We only added one new define. We leave all the others alone */
#undef SCOPE

#endif /* PREFIX */
