/* SPDX-License-Identifier: GPL-3.0-only
 *
 * Patricktronics host test — mutation timer (task 4.1, RED until ptrx_timer.c
 * exists).
 *
 * Spec Requirement 4:
 *   4.1 playback mutates at a regular interval without user action;
 *   4.2 MUTATION RATE drives the frequency (higher rate => more mutations);
 *   4.3 rate 0 => no automatic mutation at all.
 */
#include <stdint.h>
#include <stdio.h>

#include "ptrx_timer.h"

static int failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            printf("  FAIL: %s\n", (msg));                                     \
            failures++;                                                        \
        }                                                                      \
    } while (0)

/* Count how many times a mutation fires over `span_ms`, stepping a simulated
 * clock 1 ms at a time (as the main loop polls far faster than any period). */
static uint32_t count_fires(uint32_t rate, uint32_t span_ms)
{
    uint32_t last = 0, fires = 0;
    for (uint32_t t = 0; t <= span_ms; t++)
        if (ptrx_mut_due(t, &last, rate))
            fires++;
    return fires;
}

int main(void)
{
    /* 4.3: rate 0 never fires. */
    CHECK(count_fires(0u, 60000u) == 0u, "rate 0 produces no mutation");
    CHECK(ptrx_mut_period(0u) == 0u, "rate 0 has period 0 (never)");

    /* 4.2: a higher rate fires strictly more often over the same window. */
    uint32_t slow = count_fires(10u, 60000u);
    uint32_t fast = count_fires(90u, 60000u);
    CHECK(slow > 0u, "a low rate still mutates over a minute");
    CHECK(fast > slow, "a higher rate mutates more often");

    /* 4.1: fires are regular — count matches span / period (within one). */
    uint32_t p = ptrx_mut_period(50u);
    CHECK(p > 0u, "a mid rate has a positive period");
    uint32_t expect = 60000u / (p ? p : 1u);
    uint32_t got = count_fires(50u, 60000u);
    CHECK(got >= expect - 1u && got <= expect + 1u,
          "fire count matches span / period");

    if (failures) {
        printf("ptrx_timer_test: %d failure(s)\n", failures);
        return 1;
    }
    printf("ptrx_timer_test: ok\n");
    return 0;
}
