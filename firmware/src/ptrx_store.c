/* SPDX-License-Identifier: GPL-3.0-only
 *
 * Patricktronics config store. One flash object at PTRX_FLASH_LO holding a
 * header + 16 configs + CRC32, written whole (erase sector, program). Every
 * flash access is bounds-checked to [PTRX_FLASH_LO, PTRX_FLASH_HI]; a write
 * that would stray is dropped, so a bug can never reach the tester zone.
 * Integer-only. See ptrx_store.h.
 */
/* memset/memcpy: the host unit test is standalone (string.h); the firmware and
 * the WASM emulator already have them in the translation unit (libc.c / the
 * emulator's string.h included earlier), so declare nothing there. */
#ifdef PTRX_HOST
#include <string.h>
#endif

#include "ptrx_store.h"

#define PTRX_VERSION 1u
#define PTRX_BASE PTRX_FLASH_LO

typedef struct {
    uint32_t magic;   /* PTRX_MAGIC */
    uint16_t version; /* PTRX_VERSION */
    uint16_t slots;   /* PTRX_SLOTS */
    ptrx_config_t cfg[PTRX_SLOTS];
    uint32_t crc;
} ptrx_store_t;

/* CRC32 (same polynomial Felucca's storage.c uses), table-free, integer-only. */
static uint32_t crc32(const void *p, uint32_t n)
{
    const uint8_t *b = p;
    uint32_t c = 0xFFFFFFFFu, i, k;
    for (i = 0; i < n; i++) {
        c ^= b[i];
        for (k = 0; k < 8u; k++)
            c = (c >> 1) ^ (0xEDB88320u & (uint32_t)(-(int32_t)(c & 1u)));
    }
    return ~c;
}

/* Guard: the whole [addr, addr+n) range must sit inside our region. */
static int in_region(uint32_t addr, uint32_t n)
{
    return addr >= PTRX_FLASH_LO && (addr + n - 1u) <= PTRX_FLASH_HI;
}

static int store_read(const ptrx_flash_io_t *io, ptrx_store_t *st)
{
    if (!io || !io->read)
        return 1;
    io->read(PTRX_BASE, st, (uint32_t)sizeof *st);
    if (st->magic != PTRX_MAGIC || st->version != PTRX_VERSION ||
        st->slots != PTRX_SLOTS)
        return 1;
    uint32_t want = st->crc;
    st->crc = 0u;
    if (crc32(st, (uint32_t)sizeof *st) != want)
        return 1;
    return 0;
}

static int store_write(const ptrx_flash_io_t *io, ptrx_store_t *st)
{
    if (!io || !io->erase || !io->write)
        return 1;
    if (!in_region(PTRX_BASE, (uint32_t)sizeof *st))
        return 1; /* never risk the tester zone */
    st->magic = PTRX_MAGIC;
    st->version = PTRX_VERSION;
    st->slots = PTRX_SLOTS;
    st->crc = 0u;
    st->crc = crc32(st, (uint32_t)sizeof *st);

    /* Erase every sector the object spans, then program it. */
    for (uint32_t a = PTRX_BASE & ~0xFFFu; a < PTRX_BASE + sizeof *st; a += 0x1000u)
        io->erase(a);
    io->write(PTRX_BASE, st, (uint32_t)sizeof *st);
    return 0;
}

int ptrx_store_save(const ptrx_flash_io_t *io, uint32_t slot, const ptrx_config_t *cfg)
{
    ptrx_store_t st;

    if (!cfg || slot >= PTRX_SLOTS)
        return 1;

    /* Read-modify-write: keep the other slots. A fresh/corrupt store starts
     * zeroed so untouched slots are simply empty. */
    if (store_read(io, &st) != 0) {
        memset(&st, 0, sizeof st);
    }
    st.cfg[slot] = *cfg;
    return store_write(io, &st);
}

int ptrx_store_load(const ptrx_flash_io_t *io, uint32_t slot, ptrx_config_t *cfg)
{
    ptrx_store_t st;

    if (!cfg || slot >= PTRX_SLOTS)
        return 1;
    if (store_read(io, &st) != 0)
        return 1;
    /* An empty slot (count 0) reads as "nothing saved". */
    if (st.cfg[slot].pal.count == 0u)
        return 1;
    *cfg = st.cfg[slot];
    return 0;
}
