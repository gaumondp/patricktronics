/* SPDX-License-Identifier: GPL-3.0-only
 *
 * Patricktronics host test — non-destructive config store (task 7.1, RED until
 * ptrx_store.c exists).
 *
 * Spec Requirement 5:
 *   5.1 never write into the tester zone 0x97000..0xDFFFF;
 *   5.2 store in the free region with a distinct magic;
 *   5.3 stock Felucca's data survives untouched;
 *   5.4 at least 16 slots, saved and reloaded faithfully.
 *
 * Backend: a simulated 1 MiB NOR flash. Every write is checked against the
 * tester zone; the test fails if any byte there is touched.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ptrx_store.h"

static int failures = 0;
#define CHECK(c, m)                                                            \
    do {                                                                       \
        if (!(c)) { printf("  FAIL: %s\n", (m)); failures++; }                 \
    } while (0)

/* ---- simulated NOR flash (1 MiB), erased = 0xFF ---- */
#define FLASH_SIZE 0x100000u
static uint8_t flash[FLASH_SIZE];
static int tester_touched = 0;

static int in_tester(uint32_t a) { return a >= PTRX_TESTER_LO && a <= PTRX_TESTER_HI; }

static void f_read(uint32_t a, void *b, uint32_t n) { memcpy(b, &flash[a], n); }

static void f_erase(uint32_t a)
{
    uint32_t base = a & ~0xFFFu;
    for (uint32_t i = 0; i < 0x1000u; i++) {
        if (in_tester(base + i)) tester_touched = 1;
        flash[base + i] = 0xFF;
    }
}

static void f_write(uint32_t a, const void *b, uint32_t n)
{
    const uint8_t *p = b;
    for (uint32_t i = 0; i < n; i++) {
        if (in_tester(a + i)) tester_touched = 1;
        flash[a + i] = p[i];
    }
}

static const ptrx_flash_io_t io = {f_read, f_erase, f_write};

int main(void)
{
    memset(flash, 0xFF, sizeof flash);

    /* Pre-fill the tester zone with a recognisable Felucca-like fingerprint. */
    for (uint32_t a = PTRX_TESTER_LO; a <= PTRX_TESTER_HI; a++)
        flash[a] = (uint8_t)(a * 7u + 3u);
    uint8_t before[0x200];
    memcpy(before, &flash[PTRX_TESTER_LO], sizeof before);

    /* Save 16 distinct configs. */
    for (uint32_t s = 0; s < PTRX_SLOTS; s++) {
        ptrx_config_t c;
        memset(&c, 0, sizeof c);
        c.pal.count = 5;
        for (uint32_t k = 0; k < 5; k++) c.pal.note[k] = (uint8_t)(60 + s + k);
        c.rules.mut_rate = (uint8_t)(s * 3u);
        snprintf(c.name, sizeof c.name, "CFG%u", s);
        CHECK(ptrx_store_save(&io, s, &c) == 0, "save returns 0");
    }

    /* Reload them and check fidelity (5.4). */
    for (uint32_t s = 0; s < PTRX_SLOTS; s++) {
        ptrx_config_t c;
        memset(&c, 0, sizeof c);
        CHECK(ptrx_store_load(&io, s, &c) == 0, "load returns 0");
        CHECK(c.pal.count == 5 && c.pal.note[0] == (uint8_t)(60 + s),
              "slot round-trips palette");
        CHECK(c.rules.mut_rate == (uint8_t)(s * 3u), "slot round-trips rules");
    }

    /* 5.1 / 5.3: the tester zone was never written and is byte-identical. */
    CHECK(!tester_touched, "no write ever landed in the tester zone");
    CHECK(memcmp(before, &flash[PTRX_TESTER_LO], sizeof before) == 0,
          "tester zone bytes unchanged");

    if (failures) { printf("ptrx_store_test: %d failure(s)\n", failures); return 1; }
    printf("ptrx_store_test: ok\n");
    return 0;
}
