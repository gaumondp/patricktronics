/* SPDX-License-Identifier: GPL-3.0-only
 *
 * Patricktronics PRNG: xorshift32, integer-only (chip rules: no FPU, no 64-bit
 * division). A dedicated stream for arrangement randomness, independent of
 * Felucca's audio-grain rng(). See ptrx_rng.h and spec Requirement 3.
 */
#include "ptrx_rng.h"

/* Non-zero fallback for a zero seed: xorshift32 has 0 as a fixed point
 * (0 -> 0 forever), so entropy that happens to be 0 must not collapse. */
#define PTRX_SEED_FALLBACK 0xA5A5A5A5u

static uint32_t ptrx_state = PTRX_SEED_FALLBACK;

void ptrx_seed(uint32_t entropy)
{
    ptrx_state = entropy ? entropy : PTRX_SEED_FALLBACK;
}

uint32_t ptrx_rng(void)
{
    uint32_t s = ptrx_state;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    ptrx_state = s;
    return s;
}

uint32_t ptrx_seed_from_entropy(uint32_t a, uint32_t b)
{
    uint32_t x = a * 2654435761u + (b ^ 0x9E3779B9u);
    x ^= x >> 15;
    x *= 0x85EBCA6Bu;
    x ^= x >> 13;
    ptrx_seed(x);                      /* applies the zero guard */
    return x ? x : PTRX_SEED_FALLBACK; /* the value actually seeded */
}
