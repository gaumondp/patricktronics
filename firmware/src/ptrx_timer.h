/* SPDX-License-Identifier: GPL-3.0-only
 *
 * Patricktronics mutation timer. Decides, from a millisecond clock (fm1_ms on
 * the device) and the MUTATION RATE parameter, whether a mutation is due. Pure
 * and integer-only (chip rules): no runtime division — the rate maps to a
 * period through a small fixed table. Called from the main loop, never the ISR.
 *
 * See spec Requirement 4; task 4 implements it test-first.
 */
#ifndef PTRX_TIMER_H
#define PTRX_TIMER_H

#include <stdint.h>

/* MUTATION RATE is a 0..100 knob value. 0 disables mutation entirely; higher
 * values mutate more often (shorter period). */
#define PTRX_RATE_MAX 100u

/* The period, in milliseconds, for a given rate. Rate 0 returns 0 (meaning
 * "never"). Table-driven, no division. */
uint32_t ptrx_mut_period(uint32_t rate);

/* True when a mutation is due at time `now_ms` given the last mutation time
 * `*last_ms` and the rate. When it returns non-zero it advances `*last_ms` to
 * `now_ms` (the caller then performs one mutation). Rate 0 never fires. */
int ptrx_mut_due(uint32_t now_ms, uint32_t *last_ms, uint32_t rate);

#endif /* PTRX_TIMER_H */
