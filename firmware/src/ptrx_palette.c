/* SPDX-License-Identifier: GPL-3.0-only
 *
 * Patricktronics play-captured palette. Pure (no firmware state): the player's
 * held notes become the locked palette the mutation recombines. Separated from
 * ptrx_perform.c so the host test links it alone, without core.h. See spec
 * Requirement 1.
 */
#include "ptrx_mutate.h" /* ptrx_palette_t, PTRX_PAL_MAX */

int ptrx_capture_palette(const uint8_t *held, uint8_t nheld, ptrx_palette_t *pal)
{
    uint32_t i, j;
    ptrx_palette_t tmp;

    if (nheld == 0u)
        return 0; /* a release must not wipe the locked palette */

    tmp.count = 0;
    for (i = 0; i < nheld && tmp.count < PTRX_PAL_MAX; i++) {
        uint8_t note = held[i];
        if (note == 0u || note > 127u)
            continue; /* out of MIDI range */
        for (j = 0; j < tmp.count; j++)
            if (tmp.note[j] == note)
                break; /* already in the set */
        if (j == tmp.count)
            tmp.note[tmp.count++] = note;
    }
    if (tmp.count == 0u)
        return 0; /* nothing valid held: leave the palette as-is */

    *pal = tmp;
    return 1;
}
