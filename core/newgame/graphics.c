/* Imhotep - the graphics slots chosen when a scenario starts (package initfig, docs/newgame.md §6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Called by the map load 0x004D5820 and the scenario start 0x004D52A0. Each slot is loaded by the
 * ported 0x0053DD90 (graphics object 0x00F55BB0, core/graphics/last_graphics.c), which reads the
 * .sg3 file through the platform. The file names are strings of the exe's data (in the state). */
#include "newgame/graphics.h"

#include "graphics/last_graphics.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"


static void slot(game_state *st, uint32_t name, int32_t n)
{
    graphics_load_slot(st, G_GRAPHICS_GFX, name, n);
}

/* @bin 0x004F4520 */
void newgame_monument_slots(game_state *st)
{
    uint8_t mode = U8(st, G_MONUMENT_MODE);
    if (mode < 2) {
        /* @bin 0x004F452D mode := 1; the pyramid kind 0x004F44E0 picks slot 0x11 (jump table
         * 0x004F4918: 1 bent, 2 mudbrick, 3 stepped, 5 mastaba, otherwise pyramid) */
        U8(st, G_MONUMENT_MODE) = 1;
        int32_t kind = GAME_CALL0(st, FN_PYRAMID_KIND_4F44E0);
        S32(st, G_PYRAMID_KIND) = kind;
        uint32_t name;
        switch (kind) {
        case 1: name = G_STR_BENT_PYRAMID; break; /* BENT_PYRAMID */
        case 2: name = G_STR_MUDBRICK_PYRAMID; break; /* MUDBRICK_PYRAMID */
        case 3: name = G_STR_STEPPED_PYRAMID; break; /* STEPPED_PYRAMID */
        case 5: name = G_STR_MASTABA; break; /* MASTABA */
        default: name = G_STR_PYRAMID; break; /* PYRAMID */
        }
        slot(st, name, 0x11);
        slot(st, G_STR_CAESAREUM_DUMMY, 0x12); /* CaesareumDummy */
        slot(st, G_STR_LT_HOUSE_DUMMY, 0x13); /* LtHouseDummy */
        slot(st, G_STR_LIB_MAIN_DUMMY, 0x14); /* LibMainDummy */
        slot(st, G_STR_LIB_MAIN_DUMMY, 0x15);
        slot(st, G_STR_LIB_MAIN_DUMMY, 0x16);
        slot(st, G_STR_LIB_EXTRA_DUMMY, 0x17); /* LibExtraDummy */
        slot(st, G_STR_MAUS_CARRYOVER_DUMMY, 0x18); /* MausCarryoverDummy */
        slot(st, G_STR_TOMB_THUT_DUMMY, 0x19); /* TombThutDummy */
        slot(st, G_STR_TOMB_TUT_DUMMY, 0x1B); /* TombTutDummy */
        slot(st, G_STR_TOMB_SETI_DUMMY, 0x1C); /* TombSetiDummy */
        slot(st, G_STR_TOMB_RAM_DUMMY, 0x1D); /* TombRamDummy */
        slot(st, G_STR_ABU_SIMBEL_DUMMY, 0x1A); /* AbuSimbelDummy */
        return;
    }
    if (mode == 2) {
        /* @bin 0x004F4637 the tombs; missions 0x27..0x29 (signed byte) the finished ones */
        slot(st, G_STR_PYRAMID_DUMMY, 0x11); /* PyramidDummy */
        slot(st, G_STR_CAESAREUM_DUMMY, 0x12);
        slot(st, G_STR_LT_HOUSE_DUMMY, 0x13);
        slot(st, G_STR_LIB_MAIN_DUMMY, 0x14);
        slot(st, G_STR_LIB_MAIN_DUMMY, 0x15);
        slot(st, G_STR_LIB_MAIN_DUMMY, 0x16);
        slot(st, G_STR_LIB_EXTRA_DUMMY, 0x17);
        slot(st, G_STR_MAUS_CARRYOVER_DUMMY, 0x18);
        slot(st, G_STR_ABU_SIMBEL_DUMMY, 0x1A);
        int8_t m = S8(st, G_MISSION);
        slot(st, m < 0x27 || m > 0x29 ? G_STR_TOMB_THUT : G_STR_TOMB_THUT_FIN, 0x19); /* TombThut / TombThutFin */
        m = S8(st, G_MISSION);
        slot(st, m == 0x28 || m == 0x29 ? G_STR_TOMB_TUT_FIN : G_STR_TOMB_TUT, 0x1B); /* TombTutFin / TombTut */
        m = S8(st, G_MISSION);
        slot(st, m == 0x29 ? G_STR_TOMB_SETI_FIN : G_STR_TOMB_SETI, 0x1C); /* TombSetiFin / TombSeti */
        slot(st, G_STR_TOMB_RAM, 0x1D); /* TombRam */
        return;
    }
    if (mode == 4) {
        slot(st, G_E6_FILE, 0x1A); /* @bin 0x004F4759 AbuSimbel */
        slot(st, G_STR_PYRAMID_DUMMY, 0x11);
        slot(st, G_STR_CAESAREUM_DUMMY, 0x12);
        slot(st, G_STR_LT_HOUSE_DUMMY, 0x13);
        slot(st, G_STR_LIB_MAIN_DUMMY, 0x14);
        slot(st, G_STR_LIB_MAIN_DUMMY, 0x15);
        slot(st, G_STR_LIB_MAIN_DUMMY, 0x16);
        slot(st, G_STR_LIB_EXTRA_DUMMY, 0x17);
        slot(st, G_STR_MAUS_CARRYOVER_DUMMY, 0x18);
        slot(st, G_STR_TOMB_THUT_DUMMY, 0x19);
        slot(st, G_STR_TOMB_SETI_DUMMY, 0x1C);
        slot(st, G_STR_TOMB_TUT_DUMMY, 0x1B);
        slot(st, G_STR_TOMB_RAM_DUMMY, 0x1D);
        return;
    }
    /* @bin 0x004F4837 any other mode: the Cleopatra monuments */
    slot(st, G_STR_PYRAMID_DUMMY, 0x11);
    slot(st, G_STR_TOMB_THUT_DUMMY, 0x19);
    slot(st, G_STR_TOMB_TUT_DUMMY, 0x1B);
    slot(st, G_STR_TOMB_SETI_DUMMY, 0x1C);
    slot(st, G_STR_TOMB_RAM_DUMMY, 0x1D);
    slot(st, G_STR_CAESAREUM1, 0x12); /* Caesareum1 */
    slot(st, G_STR_LT_HOUSE1, 0x13); /* LtHouse1 */
    slot(st, G_LIB_MAIN_A1, 0x14); /* LibMaina1 */
    slot(st, G_LIB_KIND0_1, 0x15); /* LibMainb1 */
    slot(st, G_LIB_MAIN_C1, 0x16); /* LibMainc1 */
    slot(st, G_LIB_EXTRA, 0x17); /* LibExtra */
    slot(st, G_STR_MAUS_CARRYOVER, 0x18); /* MausCarryover */
    slot(st, G_STR_ABU_SIMBEL_DUMMY, 0x1A); /* AbuSimbelDummy */
}

/* @bin 0x0053D8E0 */
int32_t newgame_monument_kind2(game_state *st)
{
    for (uint32_t i = 0; i < 2; i++) {
        int32_t e = S16(st, G_MONUMENT_SLOTS + 2u * i);
        if (e == 0) {
            continue;
        }
        uint32_t r = G_MONUMENT_MODELS + 16u * (uint32_t) e;
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
        uint8_t a = U8(st, G_SCENARIO_TITLE + k), b = U8(st, G_STR_GLORY_OF_PTOLEMY + k);
        if (a != b) {
            equal = 0;
            break;
        }
        if (a == 0) {
            break;
        }
    }
    int32_t kind = equal ? 4 : newgame_monument_kind2(st);
    S32(st, G_EXTRA_SLOT_KIND) = kind;
    if (kind == 2) {
        slot(st, G_STR_OBELISK_EXTRA, 9); /* OBELISK_EXTRA */
        return;
    }
    if (kind == 3) {
        slot(st, G_STR_SUN_TEMPLE_EXTRA, 9); /* SUN_TEMPLE_EXTRA */
        return;
    }
    if (kind != 4) {
        slot(st, G_STR_SPHINX_EXTRA, 9); /* SPHINX_EXTRA */
        return;
    }
    int32_t v = GAME_CALL0(st, FN_MASTABA_VARIANT_53D930);
    S32(st, G_MASTABA_VARIANT) = v;
    if (v == 1) {
        slot(st, G_STR_MAUSOLEUM1_EXTRA, 9); /* MAUSOLEUM1_EXTRA */
    } else if (v == 2) {
        slot(st, G_STR_MAUSOLEUM2_EXTRA, 9); /* MAUSOLEUM2_EXTRA */
    } else {
        slot(st, G_STR_MAUSOLEUM0_EXTRA, 9); /* MAUSOLEUM0_EXTRA */
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
