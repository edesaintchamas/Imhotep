/* Imhotep - the graphics slots chosen when a scenario starts (package initfig, docs/newgame.md §6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Called by the map load 0x004D5820 and the scenario start 0x004D52A0. Each slot is loaded by the
 * ported 0x0053DD90 (graphics object 0x00F55BB0, core/graphics/last_graphics.c), which reads the
 * .sg3 file through the platform. The file names are strings of the exe's data (in the state). */
#include "newgame/graphics.h"

#include "graphics/last_graphics.h"
#include "sim/call.h"
#include "state/ported.h"

#define GRAPHICS 0x00F55BB0u     /* @bin 0x004F456F mov ecx, 0xF55BB0 */
#define MONUMENT_MODE 0x00784CBCu /* @bin 0x004F4520 byte */
#define MISSION 0x00E38EA8u       /* @bin 0x004F46D0 i8 */

static void slot(game_state *st, uint32_t name, int32_t n)
{
    graphics_load_slot(st, GRAPHICS, name, n);
}

/* @bin 0x004F4520 */
void newgame_monument_slots(game_state *st)
{
    uint8_t mode = U8(st, MONUMENT_MODE);
    if (mode < 2) {
        /* @bin 0x004F452D mode := 1; the pyramid kind 0x004F44E0 picks slot 0x11 (jump table
         * 0x004F4918: 1 bent, 2 mudbrick, 3 stepped, 5 mastaba, otherwise pyramid) */
        U8(st, MONUMENT_MODE) = 1;
        int32_t kind = GAME_CALL0(st, 0x004F44E0u);
        S32(st, 0x005E4EF4u) = kind;
        uint32_t name;
        switch (kind) {
        case 1: name = 0x005F763Cu; break; /* BENT_PYRAMID */
        case 2: name = 0x005F7628u; break; /* MUDBRICK_PYRAMID */
        case 3: name = 0x005F7618u; break; /* STEPPED_PYRAMID */
        case 5: name = 0x005F7610u; break; /* MASTABA */
        default: name = 0x005F7608u; break; /* PYRAMID */
        }
        slot(st, name, 0x11);
        slot(st, 0x00579134u, 0x12); /* CaesareumDummy */
        slot(st, 0x00579124u, 0x13); /* LtHouseDummy */
        slot(st, 0x0057922Cu, 0x14); /* LibMainDummy */
        slot(st, 0x0057922Cu, 0x15);
        slot(st, 0x0057922Cu, 0x16);
        slot(st, 0x005F75F8u, 0x17); /* LibExtraDummy */
        slot(st, 0x00579110u, 0x18); /* MausCarryoverDummy */
        slot(st, 0x00579100u, 0x19); /* TombThutDummy */
        slot(st, 0x005790E0u, 0x1B); /* TombTutDummy */
        slot(st, 0x005790D0u, 0x1C); /* TombSetiDummy */
        slot(st, 0x005790C0u, 0x1D); /* TombRamDummy */
        slot(st, 0x005790F0u, 0x1A); /* AbuSimbelDummy */
        return;
    }
    if (mode == 2) {
        /* @bin 0x004F4637 the tombs; missions 0x27..0x29 (signed byte) the finished ones */
        slot(st, 0x00579144u, 0x11); /* PyramidDummy */
        slot(st, 0x00579134u, 0x12);
        slot(st, 0x00579124u, 0x13);
        slot(st, 0x0057922Cu, 0x14);
        slot(st, 0x0057922Cu, 0x15);
        slot(st, 0x0057922Cu, 0x16);
        slot(st, 0x005F75F8u, 0x17);
        slot(st, 0x00579110u, 0x18);
        slot(st, 0x005790F0u, 0x1A);
        int8_t m = S8(st, MISSION);
        slot(st, m < 0x27 || m > 0x29 ? 0x005F75E0u : 0x005F75ECu, 0x19); /* TombThut / TombThutFin */
        m = S8(st, MISSION);
        slot(st, m == 0x28 || m == 0x29 ? 0x005F75CCu : 0x005F75D8u, 0x1B); /* TombTutFin / TombTut */
        m = S8(st, MISSION);
        slot(st, m == 0x29 ? 0x005F75C0u : 0x005F75B4u, 0x1C); /* TombSetiFin / TombSeti */
        slot(st, 0x005F75ACu, 0x1D); /* TombRam */
        return;
    }
    if (mode == 4) {
        slot(st, 0x005760B0u, 0x1A); /* @bin 0x004F4759 AbuSimbel */
        slot(st, 0x00579144u, 0x11);
        slot(st, 0x00579134u, 0x12);
        slot(st, 0x00579124u, 0x13);
        slot(st, 0x0057922Cu, 0x14);
        slot(st, 0x0057922Cu, 0x15);
        slot(st, 0x0057922Cu, 0x16);
        slot(st, 0x005F75F8u, 0x17);
        slot(st, 0x00579110u, 0x18);
        slot(st, 0x00579100u, 0x19);
        slot(st, 0x005790D0u, 0x1C);
        slot(st, 0x005790E0u, 0x1B);
        slot(st, 0x005790C0u, 0x1D);
        return;
    }
    /* @bin 0x004F4837 any other mode: the Cleopatra monuments */
    slot(st, 0x00579144u, 0x11);
    slot(st, 0x00579100u, 0x19);
    slot(st, 0x005790E0u, 0x1B);
    slot(st, 0x005790D0u, 0x1C);
    slot(st, 0x005790C0u, 0x1D);
    slot(st, 0x005F75A0u, 0x12); /* Caesareum1 */
    slot(st, 0x005E4934u, 0x13); /* LtHouse1 */
    slot(st, 0x005762FCu, 0x14); /* LibMaina1 */
    slot(st, 0x00576338u, 0x15); /* LibMainb1 */
    slot(st, 0x005762B4u, 0x16); /* LibMainc1 */
    slot(st, 0x00576284u, 0x17); /* LibExtra */
    slot(st, 0x005F7590u, 0x18); /* MausCarryover */
    slot(st, 0x005790F0u, 0x1A); /* AbuSimbelDummy */
}

/* @bin 0x0053D8E0 */
int32_t newgame_monument_kind2(game_state *st)
{
    for (uint32_t i = 0; i < 2; i++) {
        int32_t e = S16(st, 0x00784DE0u + 2u * i);
        if (e == 0) {
            continue;
        }
        uint32_t r = 0x005D19C8u + 16u * (uint32_t) e;
        if (S16(st, r) == 2 && S16(st, r + 2) < 5) { /* @bin 0x0053D8FB signed */
            return S16(st, r + 2);
        }
    }
    return 1;
}

/* @bin 0x0053D970 */
void newgame_extra_slot(game_state *st)
{
    /* @bin 0x0053D972 strcmp(0x00784914, "The Glory of Ptolemy" 0x00601840): that scenario gets 4 */
    int equal = 1;
    for (uint32_t k = 0;; k++) {
        uint8_t a = U8(st, 0x00784914u + k), b = U8(st, 0x00601840u + k);
        if (a != b) {
            equal = 0;
            break;
        }
        if (a == 0) {
            break;
        }
    }
    int32_t kind = equal ? 4 : newgame_monument_kind2(st);
    S32(st, 0x00F55BACu) = kind;
    if (kind == 2) {
        slot(st, 0x006017E0u, 9); /* OBELISK_EXTRA */
        return;
    }
    if (kind == 3) {
        slot(st, 0x006017F0u, 9); /* SUN_TEMPLE_EXTRA */
        return;
    }
    if (kind != 4) {
        slot(st, 0x005791A8u, 9); /* SPHINX_EXTRA */
        return;
    }
    int32_t v = GAME_CALL0(st, 0x0053D930u);
    S32(st, 0x00E94E18u) = v;
    if (v == 1) {
        slot(st, 0x00601804u, 9); /* MAUSOLEUM1_EXTRA */
    } else if (v == 2) {
        slot(st, 0x00601818u, 9); /* MAUSOLEUM2_EXTRA */
    } else {
        slot(st, 0x0060182Cu, 9); /* MAUSOLEUM0_EXTRA */
    }
}

static int32_t run_monument_slots(game_state *st, const int32_t *a)
{
    (void) a;
    newgame_monument_slots(st);
    return 0;
}

static int32_t run_monument_kind2(game_state *st, const int32_t *a)
{
    (void) a;
    return newgame_monument_kind2(st);
}

static int32_t run_extra_slot(game_state *st, const int32_t *a)
{
    (void) a;
    newgame_extra_slot(st);
    return 0;
}

const ported_function newgame_graphics_ports[] = {
    {0x004F4520u, "newgame_monument_slots", 0, 0, 0, run_monument_slots, 0},
    {0x0053D8E0u, "newgame_monument_kind2", 0, 32, 0, run_monument_kind2, 0},
    {0x0053D970u, "newgame_extra_slot", 0, 0, 0, run_extra_slot, 0},
};
const int newgame_graphics_ports_count = sizeof(newgame_graphics_ports) / sizeof(newgame_graphics_ports[0]);
