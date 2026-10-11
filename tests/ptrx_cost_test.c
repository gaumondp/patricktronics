/* SPDX-License-Identifier: GPL-3.0-only
 *
 * Patricktronics host test — mutation cost guard (task 8, host portion).
 *
 * The exact pi32v2 instruction budget is checked at device-build time via
 * Felucca's target_budget. On the host we guard the ALGORITHMIC cost: one
 * ptrx_bounded_cook pass must call the PRNG a bounded number of times —
 * O(len), one draw per step, no hidden loop — so the ISR-adjacent main-loop
 * work can never blow up with pattern size (spec Requirement 7.2 basis).
 */
#include <stdint.h>
#include <stdio.h>

#include "ptrx_mutate.h"

static int failures = 0;
#define CHECK(c, m)                                                            \
    do {                                                                       \
        if (!(c)) { printf("  FAIL: %s\n", (m)); failures++; }                 \
    } while (0)

/* Instrumented RNG: counts calls. */
static uint32_t rng_calls;
static uint32_t s = 0x12345678u;
static uint32_t counting_rng(void)
{
    rng_calls++;
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    return s;
}

int main(void)
{
    ptrx_palette_t pal = {.note = {60, 62, 64, 67, 69}, .count = 5};
    step_t steps[NSTEP];

    for (uint32_t len = 1; len <= NSTEP; len++) {
        for (uint32_t i = 0; i < len; i++) {
            steps[i].note[0] = pal.note[i % 5];
            steps[i].n = 1; steps[i].flags = 0; steps[i].probability = 0;
            steps[i].hit = 0;
        }
        rng_calls = 0;
        ptrx_bounded_cook(steps, len, &pal, counting_rng);
        /* Exactly one draw per step: strictly O(len), no runaway. */
        CHECK(rng_calls == len, "one PRNG draw per step (O(len), bounded)");
    }

    if (failures) { printf("ptrx_cost_test: %d failure(s)\n", failures); return 1; }
    printf("ptrx_cost_test: ok\n");
    return 0;
}
