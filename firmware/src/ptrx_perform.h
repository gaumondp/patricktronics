/* SPDX-License-Identifier: GPL-3.0-only
 *
 * Patricktronics performance glue. Wires the tested primitives (ptrx_rng,
 * ptrx_mutate, ptrx_timer) into Felucca's main loop: seed the PRNG from
 * hardware entropy at the first interaction, and while the transport plays,
 * mutate the selected track's arrangement at the MUTATION RATE interval.
 *
 * Called once per main-loop pass (never from the audio ISR). Integer-only.
 */
#ifndef PTRX_PERFORM_H
#define PTRX_PERFORM_H

#include <stdint.h>

#include "ptrx_mutate.h" /* ptrx_palette_t */

/* Current generative state the UI edits (task 10 will drive these). Kept here so
 * the loop has one place to read. */
typedef struct {
    ptrx_palette_t pal; /* locked note palette */
    uint8_t mut_rate;   /* 0..100, drives the mutation interval */
    uint8_t density;    /* 0..100, how much each mutation changes */
    uint8_t enabled;    /* master on/off for the generative layer */
} ptrx_perform_t;

extern ptrx_perform_t ptrx_perform;

/* Call once per main-loop pass. `now_ms` is fm1_ms, `touched` is non-zero when
 * the user just touched a control (seeds on the first such touch), `playing` is
 * song.playing, `entropy_b` a second entropy word (e.g. song.batt_raw). */
void ptrx_perform_poll(uint32_t now_ms, int touched, int playing, uint32_t entropy_b);

/* Re-roll the performance: draw a fresh seed without changing palette or rate. */
void ptrx_perform_reseed(uint32_t now_ms, uint32_t entropy_b);

/* Capture the player's held notes into `pal`: at most PTRX_PAL_MAX, duplicates
 * collapsed, pitches outside 1..127 skipped. An empty hold (nheld 0) leaves the
 * palette untouched and returns 0; a non-empty hold replaces it and returns 1.
 * Pure helper (no firmware state) so it is host-testable. */
int ptrx_capture_palette(const uint8_t *held, uint8_t nheld, ptrx_palette_t *pal);

#endif /* PTRX_PERFORM_H */
