/* SPDX-License-Identifier: GPL-3.0-only
 *
 * Patricktronics host test — bounded mutation (task 3.1, RED until
 * ptrx_mutate.c exists).
 *
 * Invariant (spec Requirement 1.2 / 1.3): after any number of ptrx_bounded_cook
 * calls, every active step's notes belong to the palette, octave-shifted only
 * within [PTRX_OCT_LO, PTRX_OCT_HI]. The mutation DOES change arrangement
 * (on/off, chance, accent) but NEVER grows the pitch set beyond the palette.
 */
#include <stdint.h>
#include <stdio.h>

#include "ptrx_mutate.h"
#include "ptrx_rng.h"

static int failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            printf("  FAIL: %s\n", (msg));                                     \
            failures++;                                                        \
        }                                                                      \
    } while (0)

/* Is pitch p a palette note, allowing whole-octave shifts within bounds? */
static int in_palette_octaves(uint8_t p, const ptrx_palette_t *pal)
{
    for (uint32_t i = 0; i < pal->count; i++) {
        int base = (int)pal->note[i];
        for (int o = -96; o <= 96; o += 12) {
            int q = base + o;
            if (q >= (int)PTRX_OCT_LO && q <= (int)PTRX_OCT_HI && q == (int)p)
                return 1;
        }
    }
    return 0;
}

int main(void)
{
    /* Fixed palette of 5 known pitches (C major pentatonic around middle C). */
    ptrx_palette_t pal = {.note = {60, 62, 64, 67, 69}, .count = 5};

    /* A filled 16-step pattern: every step a gated note from the palette. */
    step_t steps[NSTEP];
    for (uint32_t i = 0; i < 16; i++) {
        step_t *s = &steps[i];
        for (uint32_t k = 0; k < 4; k++)
            s->note[k] = 0;
        s->note[0] = pal.note[i % pal.count];
        s->n = 1;
        s->flags = 0;
        s->probability = 0; /* 100% */
        s->hit = 0;
    }

    /* Capture the pre-mutation arrangement signature. */
    uint8_t chance0 = steps[0].probability;
    uint8_t flags_sum0 = 0, gate_sum0 = 0;
    for (uint32_t i = 0; i < 16; i++) {
        flags_sum0 = (uint8_t)(flags_sum0 + steps[i].flags);
        gate_sum0 = (uint8_t)(gate_sum0 + steps[i].n);
    }

    ptrx_seed(0xC0FFEEu);

    /* Hammer the mutation many times. */
    for (uint32_t pass = 0; pass < 200; pass++)
        ptrx_bounded_cook(steps, 16, &pal, ptrx_rng);

    /* Invariant: every note still in the palette (octave-allowed). */
    int all_in = 1;
    for (uint32_t i = 0; i < 16; i++)
        for (uint32_t k = 0; k < steps[i].n && k < 4; k++)
            if (!in_palette_octaves(steps[i].note[k], &pal))
                all_in = 0;
    CHECK(all_in, "all notes stay within the palette (octave-allowed)");

    /* The arrangement actually moved (not a no-op). */
    uint8_t flags_sum1 = 0, gate_sum1 = 0, chance1 = steps[0].probability;
    for (uint32_t i = 0; i < 16; i++) {
        flags_sum1 = (uint8_t)(flags_sum1 + steps[i].flags);
        gate_sum1 = (uint8_t)(gate_sum1 + steps[i].n);
    }
    CHECK(flags_sum1 != flags_sum0 || gate_sum1 != gate_sum0 || chance1 != chance0,
          "mutation changed the arrangement");

    /* Palette itself is untouched. */
    CHECK(pal.count == 5 && pal.note[0] == 60 && pal.note[4] == 69,
          "palette is not modified by mutation");

    if (failures) {
        printf("ptrx_mutate_test: %d failure(s)\n", failures);
        return 1;
    }
    printf("ptrx_mutate_test: ok\n");
    return 0;
}
