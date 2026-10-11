/* SPDX-License-Identifier: GPL-3.0-only
 *
 * Patricktronics performance glue. See ptrx_perform.h. Depends on Felucca's
 * core.h for trk[]/song (the firmware build has it); the generative primitives
 * are the host-tested ptrx_* modules.
 */
#ifndef NSTEP
#include "core.h"
#endif
#include "ptrx_perform.h"
#include "ptrx_rng.h"
#include "ptrx_timer.h"

ptrx_perform_t ptrx_perform;

static uint32_t ptrx_last_mut;
static int ptrx_seeded;

void ptrx_perform_reseed(uint32_t now_ms, uint32_t entropy_b)
{
    /* fm1_ticks() would be ideal; now_ms (the 10 kHz-counted clock) XOR the
     * battery raw gives a usable launch-time entropy pair. */
    ptrx_seed_from_entropy(now_ms, entropy_b);
    ptrx_last_mut = now_ms;
    ptrx_seeded = 1;
}

void ptrx_perform_poll(uint32_t now_ms, int touched, int playing, uint32_t entropy_b)
{
    track_t *t;

    if (!ptrx_perform.enabled)
        return;

    /* Seed once, at the first interaction after boot (entropy is best when the
     * user has done something). */
    if (!ptrx_seeded && touched)
        ptrx_perform_reseed(now_ms, entropy_b);
    if (!ptrx_seeded)
        return;

    if (!playing)
        return;

    /* The player's held keys define the locked palette. Capturing while the
     * transport plays lets the guitarist re-arm the material on the fly;
     * releasing the keys keeps the last captured palette (capture ignores an
     * empty hold). */
    t = TSEL; /* &trk[song.sel] */
    ptrx_capture_palette(t->held, t->nheld, &ptrx_perform.pal);

    /* Nothing to recombine until a palette has been played at least once. */
    if (ptrx_perform.pal.count == 0u)
        return;

    /* Mutate the selected track's arrangement when the interval is due. */
    if (ptrx_mut_due(now_ms, &ptrx_last_mut, ptrx_perform.mut_rate)) {
        ptrx_bounded_cook(t->step, NSTEP, &ptrx_perform.pal, ptrx_rng);
    }
}
