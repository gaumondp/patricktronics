/* SPDX-License-Identifier: GPL-3.0-only
 *
 * Patricktronics mutation timer. Integer-only, no runtime division: the rate
 * maps to a period through a fixed table. See ptrx_timer.h.
 */
#include "ptrx_timer.h"

/* Rate (0..100) -> period in ms. Rate 0 = never. Higher rate = shorter period
 * = more frequent mutation. The curve is coarse by design: a slow drift at the
 * low end (tens of seconds), a lively churn at the top (sub-second). Values
 * between table points use the nearest lower bucket (no division). */
uint32_t ptrx_mut_period(uint32_t rate)
{
    /* 11 buckets for rate 0,10,20,...,100. */
    static const uint32_t period_ms[11] = {
        0,     /*   0: never                */
        32000, /*  10: very slow drift      */
        16000, /*  20                       */
        8000,  /*  30                       */
        4000,  /*  40                       */
        2000,  /*  50: mid                  */
        1000,  /*  60                       */
        500,   /*  70                       */
        250,   /*  80                       */
        125,   /*  90                       */
        80,    /* 100: lively churn         */
    };
    uint32_t b;

    if (rate == 0u)
        return 0u;
    if (rate > PTRX_RATE_MAX)
        rate = PTRX_RATE_MAX;
    b = rate / 10u; /* compile-time constant divisor: becomes a multiply/shift */
    if (b > 10u)
        b = 10u;
    return period_ms[b];
}

int ptrx_mut_due(uint32_t now_ms, uint32_t *last_ms, uint32_t rate)
{
    uint32_t period = ptrx_mut_period(rate);

    if (!last_ms || period == 0u)
        return 0;

    /* Unsigned subtraction handles the fm1_ms wrap correctly. */
    if ((uint32_t)(now_ms - *last_ms) >= period) {
        *last_ms = now_ms;
        return 1;
    }
    return 0;
}
