/* SPDX-License-Identifier: GPL-3.0-only
 *
 * Host-only shim: the minimal slice of Felucca's core.h that ptrx_mutate needs,
 * with the real field layout and the real step_chance/step_set_chance semantics,
 * but without core.h's full dependency graph and its macho-invalid section
 * attribute. The firmware build uses the real core.h instead; this exists only
 * so the bounded-mutation logic is unit-testable on the development machine.
 *
 * The fields and helpers below are copied verbatim from firmware/src/core.h
 * (step_t, SF_ACCENT, step_chance, step_set_chance) and must track it.
 */
#ifndef PTRX_STEP_SHIM_H
#define PTRX_STEP_SHIM_H

#include <stdint.h>

#define NSTEP 64
#define NLANE 8
#define SF_ACCENT 1u

typedef struct {
    uint8_t note[4];
    uint8_t n;           /* notes in use, 0 = empty */
    uint8_t vel;
    uint8_t flags;       /* SF_ACCENT | ... */
    uint8_t probability; /* step_chance encoding */
    uint8_t hit;         /* drum lanes bitfield */
    uint8_t len;
} step_t;

static uint32_t step_chance(const step_t *s)
{
    return !s->probability ? 100u : s->probability <= 100u ? s->probability : 0u;
}

static void step_set_chance(step_t *s, uint32_t chance)
{
    s->probability = (uint8_t)(chance >= 100u ? 0u : chance ? chance : 101u);
}

#endif /* PTRX_STEP_SHIM_H */
