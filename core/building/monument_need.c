/* Imhotep - does a monument part take this figure's delivery (Pharaoh.exe 0x00422A90).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Check: tests/differential/oracle_function_check.py (loader entries). */
#include <stdint.h>

#include "state/ported.h"
#include "state/state.h"

#define BUILDING(id) (0x007999D8u + 0x108u * (uint32_t) (id))
#define FIGURE(id) (0x00BCD5F8u + 0x184u * (uint32_t) (id))

/* @bin 0x00422A90 jump table 0x00423374: per work kind (building +0x68, 0..0x1D), the first of its
 * three 16-byte entries {key, figure type, record pointer, -} in 0x005BE508 .. 0x005BE9B8; kinds
 * 0x11, 0x12, 0x1B, 0x1C and above 0x1D have none; kind 0x1D shares kind 1's */
static const uint32_t KIND_ENTRIES[30] = {
    0x005BE5F8u, 0x005BE508u, 0x005BE538u, 0x005BE568u, 0x005BE748u, 0x005BE778u, 0x005BE628u, 0x005BE7A8u,
    0x005BE7D8u, 0x005BE658u, 0x005BE688u, 0x005BE898u, 0x005BE808u, 0x005BE8C8u, 0x005BE8F8u, 0x005BE598u,
    0x005BE5C8u, 0,           0,           0x005BE6B8u, 0x005BE6E8u, 0x005BE718u, 0x005BE838u, 0x005BE868u,
    0x005BE928u, 0x005BE958u, 0x005BE988u, 0,           0,           0x005BE508u,
};

/* @bin 0x00422A90 building id when building `id` (+0x64 set) has, for its work kind (+0x68), an
 * entry whose key is the building's +0x60, whose figure type is figure f's (+0x0A, byte), whose
 * record's i16 +0x12 is 0, and the building's dword +0x80 + 4 * key is > 0; else 0 */
int32_t monument_part_takes(game_state *st, int32_t id, int32_t f)
{
    uint32_t b = BUILDING(id);
    int32_t key = S32(st, b + 0x60);
    if (S32(st, b + 0x64) == 0) {
        return 0;
    }
    uint32_t kind = U32(st, b + 0x68); /* unsigned compare with 0x1D */
    if (kind > 0x1D || KIND_ENTRIES[kind] == 0) {
        return 0;
    }
    for (uint32_t e = KIND_ENTRIES[kind]; e < KIND_ENTRIES[kind] + 0x30u; e += 0x10) {
        if (P16(st, U32(st, e + 8) + 0x12) != 0 || S32(st, e) != key) {
            continue;
        }
        if (S32(st, e + 4) != (int32_t) U8(st, FIGURE(f) + 0x0A)) {
            continue;
        }
        if (S32(st, b + 0x80 + 4u * (uint32_t) key) > 0) {
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
