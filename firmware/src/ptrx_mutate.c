/* SPDX-License-Identifier: GPL-3.0-only
 *
 * Patricktronics bounded mutation. Alters only the arrangement of a step array;
 * every pitch it writes comes from the palette, octave-shifted only within
 * [PTRX_OCT_LO, PTRX_OCT_HI]. Integer-only (chip rules). See ptrx_mutate.h.
 */
#include "ptrx_mutate.h"

/* Clamp a palette note shifted by whole octaves into the chip-safe range.
 * Shifts toward the nearest in-range octave; never leaves the palette's pitch
 * class. */
static uint8_t pal_pitch(const ptrx_palette_t *pal, uint32_t idx, int oct)
{
    int p = (int)pal->note[idx % (pal->count ? pal->count : 1u)] + oct * 12;
    while (p < (int)PTRX_OCT_LO)
        p += 12;
    while (p > (int)PTRX_OCT_HI)
        p -= 12;
    return (uint8_t)p;
}

uint32_t ptrx_bounded_cook(step_t *steps, uint32_t len, const ptrx_palette_t *pal,
                           uint32_t (*rng)(void))
{
    uint32_t touched = 0u;
    uint32_t i;

    if (!steps || !pal || !pal->count || !rng || !len)
        return 0u;
    if (len > NSTEP)
        len = NSTEP;

    for (i = 0; i < len; i++) {
        step_t *s = &steps[i];
        uint32_t r = rng();

        /* Mutate roughly 1 step in 3, so a pass nudges the pattern rather than
         * rewriting it (density of change, not a reshuffle). */
        if ((r & 3u) == 0u)
            continue;

        switch ((r >> 2) & 3u) {
        case 0: /* on / off: Ghost Playback — toggle whether the step sounds */
            if (s->n) {
                s->n = 0u; /* silence it, keep its stored note[0] for later */
            } else {
                s->note[0] = pal_pitch(pal, (r >> 4), (int)((r >> 8) & 1u) ? 0 : 0);
                s->n = 1u;
            }
            touched++;
            break;

        case 1: /* accent on / off */
            s->flags ^= SF_ACCENT;
            touched++;
            break;

        case 2: { /* chance: 100 -> 75 -> 50 -> back */
            uint32_t c = step_chance(s);
            step_set_chance(s, c >= 100u ? 75u : c > 50u ? 50u : 100u);
            touched++;
            break;
        }

        default: { /* reassign pitch: ALWAYS from the palette, maybe an octave */
            uint32_t idx = (r >> 4);
            int oct = ((r >> 8) & 3u) == 0u ? ((r >> 10) & 1u ? 1 : -1) : 0;
            if (!s->n)
                s->n = 1u;
            s->note[0] = pal_pitch(pal, idx, oct);
            touched++;
            break;
        }
        }
    }

    return touched;
}
