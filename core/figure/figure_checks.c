/* Imhotep - small figure helpers reached from the figure actions (roadmap phase 8.2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Figures: 0x184-byte records from 0x00BCD600 (id 0), 2000 of them; +0 next figure on the tile
 * (i16), +2 type (u8), +6 state (u8, 1 alive), +0x0C / +0x0E tile x / y (i16), +0x18 grid offset
 * (i32), +0x30 action (u8), +0x31 (u8), +0x56 building (i16), +0x7A leader (i16), +0x7E / +0x80
 * (i16). The figures on a tile: the grid of i16 at 0x00B350DC. Buildings: 0x108-byte records at
 * 0x007999D8 + 0x108 * id (+0x10 type i16, +0xCB battle u8). Battles: growable rings (core/sim/queue.c) of 0x1C bytes at 0x009783A4 + 0x1C * n. */
#include "figure/actions_a.h"
#include "sim/call.h"
#include "sim/queue.h"
#include "state/ported.h"

#define FIG(id) (0x00BCD600u + 0x184u * (uint32_t) (int32_t) (id))
#define BLD(id) (0x007999D8u + 0x108u * (uint32_t) (int32_t) (id))
#define ON_TILE(grid) (0x00B350DCu + 2u * (uint32_t) (int32_t) (grid))

int32_t building_type_4d_4f(game_state *st, int32_t id)
{
    /* @bin 0x00459EC0 the building's type (+0x10, i16) in 0x4D .. 0x4F */
    int32_t t = S16(st, BLD(id) + 0x10);
    return t >= 0x4D && t <= 0x4F;
}

/* @bin 0x004F6A10 the ring's index brought into 0 .. capacity - 1 (left alone when the capacity
 * +0x10 is 0), on a local index */
static void wrap_local(game_state *st, uint32_t r, int32_t *i)
{
    if (S32(st, r + 0x10) == 0) {
        return;
    }
    while (*i < 0) {
        *i += S32(st, r + 0x10);
    }
    while (*i >= S32(st, r + 0x10)) {
        *i -= S32(st, r + 0x10);
    }
}

uint8_t ring_remove_4f6950(game_state *st, uint32_t r, int32_t value)
{
    /* @bin 0x004F6950 (thiscall on a growable ring) the first slot holding value, from the head
     * (+4) over the count (+0x0C); not found: 0. Found: the slots after it move down by one, then
     * the tail (+8) and the count decrease and the tail is wrapped; 1 */
    uint32_t slots = U32(st, r + 0x14);
    int32_t k = S32(st, r + 4);
    int32_t n = 0;
    if (S32(st, r + 0x0C) > 0) {
        for (; n < S32(st, r + 0x0C); n++) {
            if (P32(st, U32(st, r + 0x14) + 4u * (uint32_t) k) == (uint32_t) value) {
                break;
            }
            k++;
            wrap_local(st, r, &k);
        }
    }
    if (n == S32(st, r + 0x0C)) {
        return 0;
    }
    (void) slots;
    for (; n < S32(st, r + 0x0C) - 1; n++) {
        int32_t j = k + 1;
        wrap_local(st, r, &j);
        uint32_t base = U32(st, r + 0x14);
        P32(st, base + 4u * (uint32_t) k) = P32(st, base + 4u * (uint32_t) j);
        k++;
        wrap_local(st, r, &k);
    }
    S32(st, r + 8) = S32(st, r + 8) - 1;
    S32(st, r + 0x0C) = S32(st, r + 0x0C) - 1;
    ring_wrap(st, r, r + 8);
    return 1;
}

uint8_t figure_leave_battle(game_state *st, int32_t id)
{
    /* @bin 0x004A43A0 the battle of the figure's building (+0x56 -> building +0xCB, u8 1 .. 9):
     * the figure removed from that battle's ring (0x004F6950 on 0x009783A4 + 0x1C * battle); 1.
     * No battle: 0 */
    int32_t b = S16(st, FIG(id) + 0x56);
    uint8_t v = U8(st, BLD(b) + 0xCB);
    if (v == 0 || v >= 10) {
        return 0;
    }
    ring_remove_4f6950(st, 0x009783A4u + 0x1Cu * v, id);
    return 1;
}

void figure_meet_around(game_state *st, int32_t id)
{
    /* @bin 0x004B1700 for the 8 neighbour offsets 0x005F80FC .. 0x005F811C, while the figure's
     * action is not 5: unless its type is one of 0x19, 0x4D, 0x4E, 0x5C, 0x5D, 0x64, 0x65 (byte
     * table 0x004B1778 for types 0x19 .. 0x65), it meets the first figure on that tile
     * (0x004B1210) */
    uint32_t f = FIG(id);
    int32_t grid = S32(st, f + 0x18);
    for (uint32_t e = 0x005F80FCu; e < 0x005F811Cu; e += 4) {
        if (U8(st, f + 0x30) == 5) {
            return;
        }
        int32_t off = S32(st, e);
        /* @bin 0x004B1735 the byte table 0x004B1778 (in the code) holds 0 for these types */
        uint8_t t = U8(st, f + 2);
        if (t == 0x19 || t == 0x4D || t == 0x4E || t == 0x5C || t == 0x5D || t == 0x64 || t == 0x65) {
            continue;
        }
        figure_meet(st, id, S16(st, ON_TILE(off + grid)));
    }
}

void figure_substeps(game_state *st, int32_t id, int32_t n)
{
    /* @bin 0x004B28A0 n times: +0x31 (u8) + 1; at 15 or more (signed) it stays 15, else one
     * substep (0x004B3F00) */
    if (n <= 0) {
        return;
    }
    uint32_t f = FIG(id);
    do {
        int8_t c = (int8_t) (U8(st, f + 0x31) + 1);
        U8(st, f + 0x31) = (uint8_t) c;
        if (c >= 0x0F) {
            U8(st, f + 0x31) = 0x0F;
        } else {
            figure_substep(st, id);
        }
    } while (--n != 0);
}

int32_t figure_with_leader(game_state *st, int32_t id)
{
    /* @bin 0x004B46B0 the leader (+0x7A) alive (+6 == 1), not at action 4, and its +0x7E equal
     * to the figure's +0x80 */
    int32_t l = S16(st, FIG(id) + 0x7A);
    if (l <= 0) {
        return 0;
    }
    uint32_t L = FIG(l);
    if (U8(st, L + 6) != 1 || U8(st, L + 0x30) == 4) {
        return 0;
    }
    return S16(st, L + 0x7E) == S16(st, FIG(id) + 0x80);
}

int32_t figure_target_on_tile(game_state *st, int32_t id)
{
    /* @bin 0x004B56D0 the first figure on the figure's tile (list +0) not at action 4 whose type
     * is 0x2B .. 0x39, 0x29 at action 0x0B, or one of 0x52, 0x53, 0x68, 0x66, 0x67, 0x54, 0x6D,
     * 0x63, 0x64, 0x65, 0x5D, 0x5C; 0 when none */
    int32_t g = S16(st, ON_TILE(S32(st, FIG(id) + 0x18)));
    while (g > 0) {
        uint32_t G = FIG(g);
        uint8_t act = U8(st, G + 0x30);
        if (act != 4) {
            uint8_t t = U8(st, G + 2);
            if (t >= 0x2B && t <= 0x39) {
                return g;
            }
            if (t == 0x29 && act == 0x0B) {
                return g;
            }
            switch (t) {
            case 0x52: case 0x53: case 0x68: case 0x66: case 0x67: case 0x54: case 0x6D:
            case 0x63: case 0x64: case 0x65: case 0x5D: case 0x5C:
                return g;
            default:
                break;
            }
        }
        g = S16(st, G);
    }
    return 0;
}

void figures_2a_near(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x004B7510 every figure alive (+6 == 1) of type 0x2A not at action 4 within one tile
     * of (x, y) (the distances |dx| into 0x00D3B4FC, |dy| into 0x00D3B4F8): +6 := 2 */
    for (uint32_t f = FIG(0); f < FIG(2000); f += 0x184) {
        if (U8(st, f + 6) != 1 || U8(st, f + 2) != 0x2A || U8(st, f + 0x30) == 4) {
            continue;
        }
        int32_t fx = S16(st, f + 0x0C), fy = S16(st, f + 0x0E);
        int32_t dx = fx > x ? fx - x : (fx < x ? x - fx : 0);
        S32(st, 0x00D3B4FCu) = dx;
        int32_t dy = fy > y ? fy - y : (fy < y ? y - fy : 0);
        S32(st, 0x00D3B4F8u) = dy;
        if ((dx < dy ? dy : dx) <= 1) {
            U8(st, f + 6) = 2;
        }
    }
}

static int32_t run_459ec0(game_state *st, const int32_t *a)
{
    return building_type_4d_4f(st, a[0]);
}

static int32_t run_4f6950(game_state *st, const int32_t *a)
{
    return ring_remove_4f6950(st, (uint32_t) a[PORT_THIS_ARG], a[0]);
}

static int32_t run_4a43a0(game_state *st, const int32_t *a)
{
    return figure_leave_battle(st, a[0]);
}

static int32_t run_4b1700(game_state *st, const int32_t *a)
{
    figure_meet_around(st, a[0]);
    return 0;
}

static int32_t run_4b28a0(game_state *st, const int32_t *a)
{
    figure_substeps(st, a[0], a[1]);
    return 0;
}

static int32_t run_4b46b0(game_state *st, const int32_t *a)
{
    return figure_with_leader(st, a[0]);
}

static int32_t run_4b56d0(game_state *st, const int32_t *a)
{
    return figure_target_on_tile(st, a[0]);
}

static int32_t run_4b7510(game_state *st, const int32_t *a)
{
    figures_2a_near(st, a[0], a[1]);
    return 0;
}

const ported_function figure_check_ports[] = {
    {0x00459EC0u, "building_type_4d_4f", 1, 32, 0, run_459ec0, 0},
    {0x004F6950u, "ring_remove", 1, 8, 0, run_4f6950, 0},
    {0x004A43A0u, "figure_leave_battle", 1, 8, 0, run_4a43a0, 0},
    {0x004B1700u, "figure_meet_around", 1, 0, 0, run_4b1700, 0},
    {0x004B28A0u, "figure_substeps", 2, 0, 0, run_4b28a0, 0},
    {0x004B46B0u, "figure_with_leader", 1, 32, 0, run_4b46b0, 0},
    {0x004B56D0u, "figure_target_on_tile", 1, 32, 0, run_4b56d0, 0},
    {0x004B7510u, "figures_2a_near", 2, 0, 0, run_4b7510, 0},
};
const int figure_check_ports_count = sizeof(figure_check_ports) / sizeof(figure_check_ports[0]);
