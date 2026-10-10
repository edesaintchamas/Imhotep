/* Imhotep - does a monument part take this figure's delivery (Pharaoh.exe 0x00422A90).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Check: tests/differential/oracle_function_check.py (loader entries). */
#include <stdint.h>

#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"
#include "state/state.h"

/* @bin 0x00422A90 jump table 0x00423374: per work kind (building +0x68, 0..0x1D), the first of its
 * three 16-byte entries {key, figure type, record pointer, -} in 0x005BE508 .. 0x005BE9B8; kinds
 * 0x11, 0x12, 0x1B, 0x1C and above 0x1D have none; kind 0x1D shares kind 1's */
static const uint32_t KIND_ENTRIES[30] = {
    G_STONE_SLOTS + 0x30 * 5, G_STONE_SLOTS + 0x30 * 0, G_STONE_SLOTS + 0x30 * 1, G_STONE_SLOTS + 0x30 * 2,
    G_STONE_SLOTS + 0x30 * 12, G_STONE_SLOTS + 0x30 * 13, G_STONE_SLOTS + 0x30 * 6, G_STONE_SLOTS + 0x30 * 14,
    G_STONE_SLOTS + 0x30 * 15, G_STONE_SLOTS + 0x30 * 7, G_STONE_SLOTS + 0x30 * 8, G_STONE_SLOTS + 0x30 * 19,
    G_STONE_SLOTS + 0x30 * 16, G_STONE_SLOTS + 0x30 * 20, G_STONE_SLOTS + 0x30 * 21, G_STONE_SLOTS + 0x30 * 3,
    G_STONE_SLOTS + 0x30 * 4, 0, 0, G_STONE_SLOTS + 0x30 * 9,
    G_STONE_SLOTS + 0x30 * 10, G_STONE_SLOTS + 0x30 * 11, G_STONE_SLOTS + 0x30 * 17, G_STONE_SLOTS + 0x30 * 18,
    G_STONE_SLOTS + 0x30 * 22, G_STONE_SLOTS + 0x30 * 23, G_STONE_SLOTS + 0x30 * 24, 0,
    0, G_STONE_SLOTS + 0x30 * 0,
};

/* @bin 0x00422A90 building id when building `id` (+0x64 set) has, for its work kind (+0x68), an
 * entry whose key is the building's +0x60, whose figure type is figure f's (+0x0A, byte), whose
 * record's i16 +0x12 is 0, and the building's dword +0x80 + 4 * key is > 0; else 0 */
int32_t monument_part_takes(game_state *st, int32_t id, int32_t f)
{
    uint32_t b = BUILDING_ADDR(id);
    int32_t key = S32(st, b + BLD_TOMB_STAGE);
    if (S32(st, b + BLD_U64) == 0) {
        return 0;
    }
    uint32_t kind = U32(st, b + BLD_TOMB_STONE_SET); /* unsigned compare with 0x1D */
    if (kind > 0x1D || KIND_ENTRIES[kind] == 0) {
        return 0;
    }
    for (uint32_t e = KIND_ENTRIES[kind]; e < KIND_ENTRIES[kind] + 0x30u; e += 0x10) {
        if (P16(st, U32(st, e + WORKSLOT_ANIM) + ANIMOBJ_FIGURE) != 0 || S32(st, e + WORKSLOT_KEY) != key) {
            continue;
        }
        if (S32(st, e + WORKSLOT_FIGURE_TYPE) != (int32_t) U8(st, FIGURE_ADDR(f) + FIG_TYPE)) {
            continue;
        }
        if (S32(st, b + BLD_TOMB_STONES + 4u * (uint32_t) key) > 0) {
            return id;
        }
    }
    return 0;
}

static int32_t run_monument_part_takes(game_state *st, const int32_t *a)
{
    return monument_part_takes(st, a[0], a[1]);
}

const ported_function monument_need_ports[] = {
    {0x00422A90u, "monument_part_takes", 2, 32, 0, run_monument_part_takes, 0},
};
const int monument_need_ports_count = sizeof(monument_need_ports) / sizeof(monument_need_ports[0]);
