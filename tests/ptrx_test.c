/* SPDX-License-Identifier: GPL-3.0-only
 *
 * Patricktronics host tests (no FM-1), inside Felucca's host harness
 * (tests/run_tests.sh). Cases are added test-first per
 * .kiro/specs/patricktronics/tasks.md.
 *
 * Task 2.1 (RED until ptrx_rng.c exists): the PRNG stream.
 *   - determinism: a known seed yields a known vector (Requirement 3.2 basis);
 *   - difference: two seeds yield different streams (Requirement 3.2);
 *   - zero guard: seed 0 does not collapse to the zero fixed point (Req 3.1).
 */
#include <stdint.h>
#include <stdio.h>

#include "ptrx_rng.h"

static int failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            printf("  FAIL: %s\n", (msg));                                     \
            failures++;                                                        \
        }                                                                      \
    } while (0)

/* Vectors computed on the host (fixed constants, per the chip rules). */
static const uint32_t SEED1[4] = {0x00042021u, 0x04080601u, 0x9DCCA8C5u, 0x1255994Fu};
static const uint32_t SEED2[4] = {0x00084042u, 0x08008C02u, 0x2998D882u, 0x6398B906u};

static void test_determinism(void)
{
    ptrx_seed(1u);
    for (int i = 0; i < 4; i++)
        CHECK(ptrx_rng() == SEED1[i], "seed 1 produces the known vector");

    /* Reseeding the same value replays the same stream. */
    ptrx_seed(1u);
    int same = 1;
    for (int i = 0; i < 4; i++)
        if (ptrx_rng() != SEED1[i])
            same = 0;
    CHECK(same, "reseeding seed 1 replays the same stream");
}

static void test_difference(void)
{
    ptrx_seed(2u);
    for (int i = 0; i < 4; i++)
        CHECK(ptrx_rng() == SEED2[i], "seed 2 produces its own vector");

    /* Seeds 1 and 2 must not share the stream. */
    CHECK(SEED1[0] != SEED2[0], "distinct seeds give distinct streams");
}

static void test_zero_guard(void)
{
    ptrx_seed(0u);
    uint32_t first = ptrx_rng();
    CHECK(first != 0u, "seed 0 does not collapse to the zero fixed point");

    /* A zero-seeded xorshift would stay 0 forever; a guarded one keeps moving. */
    uint32_t second = ptrx_rng();
    CHECK(second != 0u && second != first, "zero-seeded stream keeps advancing");
}

static void test_entropy(void)
{
    /* Two different entropy pairs give different seeds (Requirement 3.1/3.2). */
    uint32_t s1 = ptrx_seed_from_entropy(0x1234u, 0xABCDu);
    uint32_t s2 = ptrx_seed_from_entropy(0x1234u, 0xABCEu);
    CHECK(s1 != 0u && s2 != 0u, "entropy seed is never zero");
    CHECK(s1 != s2, "different entropy gives different seeds");

    /* All-zero entropy still yields a non-zero, usable stream. */
    uint32_t s0 = ptrx_seed_from_entropy(0u, 0u);
    CHECK(s0 != 0u, "zero entropy still seeds non-zero");
    CHECK(ptrx_rng() != 0u, "zero-entropy stream advances");
}

int main(void)
{
    test_determinism();
    test_difference();
    test_zero_guard();
    test_entropy();

    if (failures) {
        printf("ptrx_test: %d failure(s)\n", failures);
        return 1;
    }
    printf("ptrx_test: ok\n");
    return 0;
}
