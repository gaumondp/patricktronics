/* SPDX-License-Identifier: GPL-3.0-only
 *
 * Patricktronics page (FX family, global). Four knobs over the generative
 * layer's own state (ptrx_perform), NOT over song.g[] — so nothing here touches
 * the frozen G_* ids or the project format. A graph of its own (GR_PTRX):
 *   KNOB 1  ENABLE   OFF / ON (the generative layer master switch)
 *   KNOB 2  RATE     0..100  MUTATION RATE (0 = no auto mutation)
 *   KNOB 3  DENSITY  0..100  how much each mutation changes
 *   KNOB 4  RESEED   a GO button: re-roll the performance (OCT+)
 *
 * The palette is captured from played keys (ptrx_perform.pal), shown on the
 * panel. Included by ui_graph.c like ui_fm6op.c; cards in ui_draw.c, knobs in
 * ui_input.c. Integer-only.
 */

#include "ptrx_perform.h"

static int ptrx_page(uint32_t g) { return g == GR_PTRX; }

/* KNOB turn on the PTRX page. slot 0 ENABLE, 1 RATE, 2 DENSITY, 3 unused (RESEED
 * is OCT+, ptrx_oct). */
static void ptrx_knob(uint32_t slot, int32_t steps)
{
    if (!steps)
        return;
    switch (slot) {
    case 0:
        ptrx_perform.enabled = steps > 0 ? 1u : 0u;
        break;
    case 1:
        ptrx_perform.mut_rate =
            (uint8_t)clamp((int32_t)ptrx_perform.mut_rate + steps, 0, 100);
        break;
    case 2:
        ptrx_perform.density =
            (uint8_t)clamp((int32_t)ptrx_perform.density + steps, 0, 100);
        break;
    default:
        break;
    }
}

/* OCT+ on the PTRX page: RESEED (re-roll without changing palette or rate). */
static void ptrx_oct(void)
{
    ptrx_perform_reseed(fm1_ms, song.batt_raw);
    ui_message("RESEED");
}

/* One card's label / value / unit / ratio for ui_draw.c. Returns 1 if active. */
static int ptrx_card(uint32_t c, const char **lb, char *val, const char **u, int32_t *ratio)
{
    *u = "";
    *ratio = -1;
    switch (c) {
    case 0:
        *lb = "ENABLE";
        str_cpy(val, ptrx_perform.enabled ? "ON" : "OFF", 8u);
        return ptrx_perform.enabled;
    case 1:
        *lb = "RATE";
        fmt_int(val, ptrx_perform.mut_rate);
        *u = "%";
        *ratio = ptrx_perform.mut_rate * 1000 / 100;
        return ptrx_perform.mut_rate != 0u;
    case 2:
        *lb = "DENSITY";
        fmt_int(val, ptrx_perform.density);
        *u = "%";
        *ratio = ptrx_perform.density * 1000 / 100;
        return 1;
    default:
        *lb = "RESEED";
        str_cpy(val, "OCT+", 8u);
        return ptrx_perform.pal.count != 0u;
    }
}
