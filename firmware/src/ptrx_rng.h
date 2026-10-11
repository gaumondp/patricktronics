/* SPDX-License-Identifier: GPL-3.0-only
 *
 * Patricktronics PRNG: a dedicated xorshift32 stream for arrangement randomness,
 * kept separate from Felucca's audio-grain rng() so the two never share state.
 *
 * Fixed-point / integer only (chip rules: no FPU). The seed is drawn from
 * hardware entropy at launch and is never persisted (see spec Requirement 3).
 *
 * Task 2.1 writes the test against this interface; task 2.2 adds the
 * implementation (ptrx_rng.c) until the test goes green.
 */
#ifndef PTRX_RNG_H
#define PTRX_RNG_H

#include <stdint.h>

/* Set the generator state from entropy. A zero seed is replaced by a non-zero
 * constant, because xorshift32 has a zero fixed point (0 -> 0 forever). */
void ptrx_seed(uint32_t entropy);

/* Advance the state and return the next 32-bit value. */
uint32_t ptrx_rng(void);

/* Mix two hardware entropy sources (e.g. fm1_ticks() at first touch and the
 * battery ADC raw reading) into a seed and apply it. Returns the seed used, so
 * a re-seed can be shown to differ. Never zero (the zero guard applies). */
uint32_t ptrx_seed_from_entropy(uint32_t a, uint32_t b);

#endif /* PTRX_RNG_H */
