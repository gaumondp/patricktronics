/* SPDX-License-Identifier: GPL-3.0-only
 *
 * Patricktronics config store. 16 compact configs {palette, rules, fx} in a
 * dedicated flash region, never touching Felucca's tester zone. The on-flash
 * object carries a 'PTRX' magic distinct from Felucca's 'FUNA', so stock Felucca
 * ignores it and the two coexist. See spec Requirement 5.
 *
 * Flash geometry (verified against firmware/src/storage.c):
 *   tester zone  0x97000..0xDFFFF  (projects, samples, presets)  — NEVER written
 *   PTRX region  0xE8000..0xFBFFF  (free)                        — our A/B pair
 *
 * This module is written against a flash backend abstracted as read/erase/write
 * callbacks, so the host test drives a simulated NOR and asserts no write lands
 * in the tester zone. Integer-only.
 */
#ifndef PTRX_STORE_H
#define PTRX_STORE_H

#include <stdint.h>

#define PTRX_PAL_MAX 5u
#define PTRX_SLOTS 16u

#define PTRX_FLASH_LO 0xE8000u /* first byte we may write */
#define PTRX_FLASH_HI 0xFBFFFu /* last byte we may write  */
#define PTRX_TESTER_LO 0x97000u
#define PTRX_TESTER_HI 0xDFFFFu

#define PTRX_MAGIC 0x50545258u /* 'PTRX' (not Felucca's 'FUNA') */

typedef struct {
    uint8_t note[PTRX_PAL_MAX];
    uint8_t count;
} ptrx_palette_cfg_t;

typedef struct {
    uint8_t mut_rate, density, chance, decay, oct_lo, oct_hi;
} ptrx_rules_cfg_t;

typedef struct {
    uint8_t rev_type, rev_size, rev_damp, dly_time, dly_fdbk, dly_mix;
} ptrx_fx_cfg_t;

typedef struct {
    ptrx_palette_cfg_t pal;
    ptrx_rules_cfg_t rules;
    ptrx_fx_cfg_t fx;
    char name[12];
} ptrx_config_t;

/* Flash backend: addresses are absolute flash offsets. The implementation uses
 * only these; the test supplies a simulated NOR and records every write. */
typedef struct {
    void (*read)(uint32_t addr, void *buf, uint32_t n);
    void (*erase)(uint32_t addr);               /* erase the 4 KiB sector at addr */
    void (*write)(uint32_t addr, const void *buf, uint32_t n);
} ptrx_flash_io_t;

/* Save `cfg` into slot `slot` (0..15). Returns 0 on success. All flash access
 * goes through `io` and stays within [PTRX_FLASH_LO, PTRX_FLASH_HI]. */
int ptrx_store_save(const ptrx_flash_io_t *io, uint32_t slot, const ptrx_config_t *cfg);

/* Load slot `slot` into `*cfg`. Returns 0 on success, non-zero if the slot is
 * empty or the store is corrupt (CRC mismatch). */
int ptrx_store_load(const ptrx_flash_io_t *io, uint32_t slot, ptrx_config_t *cfg);

#endif /* PTRX_STORE_H */
