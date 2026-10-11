/* SPDX-License-Identifier: GPL-3.0-only
 *
 * Patricktronics bounded mutation: alters only the ARRANGEMENT of a step array
 * (step on/off, chance, accent, octave within bounds, and which palette note a
 * step plays). It never introduces a pitch outside the palette, unlike
 * Felucca's tl_cook_notes, which drifts pitches along the scale.
 *
 * Integer-only (chip rules). Runs in the main loop, never in the audio ISR.
 * See spec Requirements 1 and 4; task 3 implements it test-first.
 *
 * The function takes a step array + length rather than a whole track_t, so the
 * logic is unit-testable on the host without the full firmware; the firmware
 * passes t->step / its pattern length.
 */
#ifndef PTRX_MUTATE_H
#define PTRX_MUTATE_H

#include <stdint.h>

/* step_t and its helpers (step_chance/step_set_chance, NSTEP, SF_ACCENT):
 * the firmware supplies them from core.h; a host unit test defines PTRX_HOST
 * and gets the minimal shim instead. core.h has no include guard and a
 * macho-invalid section attribute, so it is pulled in only on the real target. */
#ifdef PTRX_HOST
#include "ptrx_step_shim.h"
#else
#ifndef NSTEP
#include "core.h" /* only when not already included by the single-unit build */
#endif
#endif

#define PTRX_PAL_MAX 5u

/* The locked note palette: 1..5 fixed MIDI pitches. */
typedef struct {
    uint8_t note[PTRX_PAL_MAX];
    uint8_t count;
} ptrx_palette_t;

/* Lowest / highest MIDI pitch a mutation may use (the chip-safe 24..108 range
 * Felucca already enforces); octave shifts stay inside it. */
#define PTRX_OCT_LO 24u
#define PTRX_OCT_HI 108u

/* Mutate the arrangement of `len` steps in place, using `rng` for choices and
 * drawing every pitch from `pal`. Returns the number of steps touched.
 *
 * Guarantees (tested): after any sequence of calls, every active step's notes
 * are members of the palette, octave-shifted only within [PTRX_OCT_LO,
 * PTRX_OCT_HI]; the FM patch and the palette are never modified. */
uint32_t ptrx_bounded_cook(step_t *steps, uint32_t len, const ptrx_palette_t *pal,
                           uint32_t (*rng)(void));

#endif /* PTRX_MUTATE_H */
