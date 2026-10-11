/* SPDX-License-Identifier: GPL-3.0-only
 *
 * Patricktronics host test — play-captured palette (task 10.x, RED until
 * ptrx_capture_palette() exists).
 *
 * Spec Requirement 1 (locked palette): the player's held keys set the palette,
 * at most PTRX_PAL_MAX notes, duplicates collapsed, empty hold leaves the
 * palette untouched (so a release does not wipe it).
 */
#include <stdint.h>
#include <stdio.h>

#include "ptrx_mutate.h" /* ptrx_palette_t, PTRX_PAL_MAX */

/* Pure capture helper under test; lives in ptrx_perform.c. */
int ptrx_capture_palette(const uint8_t *held, uint8_t nheld, ptrx_palette_t *pal);

static int failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            printf("  FAIL: %s\n", (msg));                                     \
            failures++;                                                        \
        }                                                                      \
    } while (0)

int main(void)
{
    ptrx_palette_t pal = {{0, 0, 0, 0, 0}, 0};

    /* An empty hold leaves the palette as-is (no notes held != wipe). */
    pal.count = 3;
    pal.note[0] = 60;
    CHECK(ptrx_capture_palette((const uint8_t[]){0}, 0u, &pal) == 0,
          "empty hold captures nothing");
    CHECK(pal.count == 3u && pal.note[0] == 60u,
          "empty hold leaves the palette untouched");

    /* A 3-note chord becomes a 3-note palette, in order. */
    {
        uint8_t held[] = {62, 65, 69};
        pal.count = 0;
        CHECK(ptrx_capture_palette(held, 3u, &pal) == 1,
              "a held chord is captured");
        CHECK(pal.count == 3u, "three held notes -> palette of 3");
        CHECK(pal.note[0] == 62u && pal.note[1] == 65u && pal.note[2] == 69u,
              "palette keeps the held notes in order");
    }

    /* More than PTRX_PAL_MAX held notes clamp to the first PTRX_PAL_MAX. */
    {
        uint8_t held[] = {40, 41, 42, 43, 44, 45, 46};
        pal.count = 0;
        CHECK(ptrx_capture_palette(held, 7u, &pal) == 1, "overflow captured");
        CHECK(pal.count == PTRX_PAL_MAX, "palette clamps to PTRX_PAL_MAX");
        CHECK(pal.note[0] == 40u && pal.note[PTRX_PAL_MAX - 1u] == 44u,
              "palette keeps the first PTRX_PAL_MAX held notes");
    }

    /* Duplicates (same pitch twice) collapse — the palette is a set. */
    {
        uint8_t held[] = {60, 60, 67, 67, 72};
        pal.count = 0;
        CHECK(ptrx_capture_palette(held, 5u, &pal) == 1, "dup chord captured");
        CHECK(pal.count == 3u, "duplicate pitches collapse to 3 distinct");
        CHECK(pal.note[0] == 60u && pal.note[1] == 67u && pal.note[2] == 72u,
              "distinct pitches kept in first-seen order");
    }

    /* Out-of-range pitches (0 / >127) are skipped, not stored. */
    {
        uint8_t held[] = {0, 60, 200 & 0xFF, 64};
        pal.count = 0;
        CHECK(ptrx_capture_palette(held, 4u, &pal) == 1, "ranged chord captured");
        CHECK(pal.count == 2u, "0 and out-of-range pitches skipped");
        CHECK(pal.note[0] == 60u && pal.note[1] == 64u,
              "only valid pitches land in the palette");
    }

    if (failures) {
        printf("ptrx_palette_test: %d failure(s)\n", failures);
        return 1;
    }
    printf("ptrx_palette_test: ok\n");
    return 0;
}
