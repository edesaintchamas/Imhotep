/* Imhotep - creating a figure (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21). */
#include "figure/figure_create.h"

#include <string.h>

#include "random/crt_rand.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


int figure_referenced(game_state *st, int32_t id)
{
    /* @bin 0x004B47A0 */
    for (uint32_t o = G_FERRY_QUEUES; o < G_FERRY_QUEUES + 50u * 0xE0u; o += 0xE0u) {
        if (S32(st, o) != 0 && GAME_CALLT(st, o, FN_QUEUE_CONTAINS, id) != 0) {
            return 1;
        }
    }
    for (uint32_t r = G_FERRY_SLOTS + 4u; r < G_FERRY_SLOTS + 4u + 50u * 0x2Cu; r += 0x2Cu) {
        if (U8(st, r - 4) == 0) {
            continue;
        }
        for (uint32_t k = 0; k < 4; k++) {
            if (S32(st, r + 4 * k) == id) {
                return 1;
            }
        }
    }
    return 0;
}

/* @bin 0x004B5B6B a figure's sound/phrase slot: the next of a counter (wrapping at `limit`) in +0x70,
 * the group's value in +0x98 */
static void take_slot(game_state *st, uint32_t f, uint32_t counter, int32_t limit, int32_t value)
{
    int32_t n = S32(st, counter);
    S16(st, f + FIG_NAME_INDEX) = (int16_t) n;
    n++;
    S32(st, f + FIG_NAME_GROUP) = value;
    S32(st, counter) = n >= limit ? 0 : n;
}

void figure_assign_slot(game_state *st, int32_t id)
{
    /* @bin 0x004B5AF0 by figure type (jump table 0x004B6334 / 0x004B62CC) */
    uint32_t f = FIGURE_ADDR(id);
    uint8_t t = U8(st, f + FIG_TYPE);
    int32_t formation = S16(st, f + FIG_FORMATION);
    switch (t) {
    case 0x06: case 0x0E: case 0x41: case 0x5F: case 0x3F: case 0x48: case 0x61:
        return;
    case 0x03: take_slot(st, f, G_NAME_COUNTER_FF, 0x20, 0xFF); return;
    case 0x16: case 0x17: take_slot(st, f, G_NAME_COUNTER_100, 0x10, 0x100); return;
    case 0x28: take_slot(st, f, G_NAME_COUNTER_101, 0x20, 0x101); return;
    case 0x49: take_slot(st, f, G_NAME_COUNTER_102, 0x10, 0x102); return;
    case 0x1A: case 0x27: take_slot(st, f, G_NAME_COUNTER_103, 0x20, 0x103); return;
    case 0x42: take_slot(st, f, G_NAME_COUNTER_104, 0x10, 0x104); return;
    case 0x14: take_slot(st, f, G_NAME_COUNTER_105, 0x10, 0x105); return;
    case 0x13: case 0x15: take_slot(st, f, G_NAME_COUNTER_106, 0x10, 0x106); return;
    case 0x0F: case 0x12: case 0x69: take_slot(st, f, G_NAME_COUNTER_107, 0x20, 0x107); return;
    case 0x10: take_slot(st, f, G_NAME_COUNTER_108, 0x20, 0x108); return;
    case 0x11: take_slot(st, f, G_NAME_COUNTER_109, 0x10, 0x109); return;
    case 0x1D: case 0x1E: take_slot(st, f, G_NAME_COUNTER_10F, 0x20, 0x10F); return;
    case 0x1F: case 0x20: case 0x21: take_slot(st, f, G_NAME_COUNTER_110, 0x20, 0x110); return;
    case 0x22: take_slot(st, f, G_NAME_COUNTER_111, 0x10, 0x111); return;
    case 0x0A: take_slot(st, f, G_NAME_COUNTER_112, 0x10, 0x112); return;
    case 0x08: take_slot(st, f, G_NAME_COUNTER_113, 0x10, 0x113); return;
    case 0x58: take_slot(st, f, G_NAME_COUNTER_114, 0x10, 0x114); return;
    case 0x07: take_slot(st, f, G_NAME_COUNTER_115, 0x10, 0x115); return;
    case 0x59: take_slot(st, f, G_NAME_COUNTER_116, 0x10, 0x116); return;
    case 0x4C: take_slot(st, f, G_NAME_COUNTER_117, 0x10, 0x117); return;
    case 0x2A: take_slot(st, f, G_NAME_COUNTER_118, 0x20, 0x118); return;
    case 0x5C: case 0x5D: case 0x64: case 0x65: take_slot(st, f, G_NAME_COUNTER_121, 0x10, 0x121); return;
    case 0x63: take_slot(st, f, G_NAME_COUNTER_122, 0x10, 0x122); return;
    case 0x01: case 0x02: case 0x04: case 0x05: case 0x09: case 0x23: case 0x26: case 0x36: case 0x37: case 0x38:
    case 0x4B: case 0x4F: case 0x50: case 0x51: case 0x55: case 0x57: case 0x5A: case 0x60: case 0x62: case 0x6C:
        take_slot(st, f, G_NAME_COUNTER_FE, 0x80, 0xFE);
        return;
    case 0x2B: case 0x2C: case 0x2D: {
        /* @bin 0x004B5F9A by the kind of the figure's formation (+0x60, formation +0x6A) */
        static const uint32_t counters[12] = {G_NAME_COUNTER_11D, G_NAME_COUNTER_11F, G_NAME_COUNTER_11E, G_NAME_COUNTER_120,
                                              G_NAME_COUNTER_11C, G_NAME_COUNTER_11B, G_NAME_COUNTER_11A, G_NAME_COUNTER_119,
                                              G_NAME_COUNTER_12C, G_NAME_COUNTER_12D, G_NAME_COUNTER_12E, G_NAME_COUNTER_12F};
        static const int32_t values[12] = {0x11D, 0x11F, 0x11E, 0x120, 0x11C, 0x11B,
                                           0x11A, 0x119, 0x12C, 0x12D, 0x12E, 0x12F};
        uint8_t kind = U8(st, G_FORMATION_BASE + 0x90u * (uint32_t) formation + FM_KIND);
        if (kind > 0xB) {
            S16(st, f + FIG_NAME_INDEX) = -1;
            S32(st, f + FIG_NAME_GROUP) = -1;
            return;
        }
        if (kind == 3) { /* @bin 0x004B61F9 this one wraps after 0x10 (jle), not at it */
            take_slot(st, f, counters[kind], 0x11, values[kind]);
        } else {
            take_slot(st, f, counters[kind], 0x10, values[kind]);
        }
        return;
    }
    default:
        S32(st, f + FIG_NAME_GROUP) = -2;
        return;
    }
}

int32_t figure_create(game_state *st, int32_t owner, int32_t type, int32_t x, int32_t y, int32_t dir)
{
    /* @bin 0x004B4800 */
    int32_t id = 1;
    for (; id < FIGURE_COUNT; id++) {
        if (U8(st, FIGURE_ADDR(id) + FIG_STATE) == 0 && !figure_referenced(st, id)) {
            break;
        }
    }
    if (id >= FIGURE_COUNT) {
        return 0;
    }
    uint32_t f = FIGURE_ADDR(id);
    memset(state_ptr(st, f, FIGURE_SIZE), 0, FIGURE_SIZE);
    U8(st, f + FIG_STATE) = 1;
    U8(st, f + FIG_OWNER) = (uint8_t) owner;
    U8(st, f + FIG_TYPE) = (uint8_t) type;
    U8(st, f + FIG_U0C) = 0;
    U8(st, f + FIG_U0D) = 1;
    U16(st, f + FIG_U132) = 0xFFFF;
    S16(st, f + FIG_SEQUENCE) = (int16_t) S32(st, G_FIGURE_SERIAL);
    U8(st, f + FIG_DIRECTION) = (uint8_t) dir;
    S32(st, G_FIGURE_SERIAL) += 1;
    S16(st, f + FIG_SOURCE_X) = (int16_t) x; /* x: +0x2C, +0x24, +0x18, +0x14 */
    S16(st, f + FIG_DEST_X) = (int16_t) x;
    S16(st, f + FIG_PREV_X) = (int16_t) x;
    S16(st, f + FIG_X) = (int16_t) x;
    S16(st, f + FIG_SOURCE_Y) = (int16_t) y;
    S16(st, f + FIG_DEST_Y) = (int16_t) y;
    S16(st, f + FIG_PREV_Y) = (int16_t) y;
    S16(st, f + FIG_Y) = (int16_t) y;
    S32(st, f + FIG_GRID_OFFSET) = S32(st, G_MAP_START) + y * GRID_SIZE + x;
    /* @bin 0x004B48FE 16-bit products: the position in 15ths of a tile */
    S16(st, f + FIG_CC_X) = (int16_t) (S16(st, f + FIG_X) * 15);
    S16(st, f + FIG_CC_Y) = (int16_t) (S16(st, f + FIG_Y) * 15);
    U8(st, f + FIG_PROGRESS_ON_TILE) = 0xF;
    U8(st, f + FIG_U92) = 0;
    uint8_t r = (uint8_t) (crt_rand(st) & 3);
    U8(st, f + FIG_RAND_B) = r;
    U8(st, f + FIG_RAND_A) = r;
    figure_assign_slot(st, id);
    GAME_CALL(st, FN_FIGURE_TILE_LIST_ADD, id);
    if (type == 0x13 || type == 0x14) {
        GAME_CALL(st, FN_FIGURE_NEW_SLOT_0X7F, id);
    }
    if (id > S32(st, G_FIGURE_HIGHEST)) {
        S32(st, G_FIGURE_HIGHEST) = id;
    }
    return id;
}

void figure_walker_start(game_state *st, int32_t id)
{
    /* @bin 0x004B1DF0 */
    uint32_t f = FIGURE_ADDR(id);
    uint32_t b = BUILDING_ADDR(S16(st, f + FIG_HOME));
    U8(st, f + FIG_PROGRESS_ON_TILE) = 0xF;
    U8(st, f + FIG_ROAM_STATE) = 0;
    U8(st, f + FIG_U49) = 0xFF;
    U8(st, f + FIG_U48) = 2;
    uint8_t side = U8(st, b + BLD_ROAM_DIRECTION);
    uint8_t next = (uint8_t) (side + 2);
    U8(st, b + BLD_ROAM_DIRECTION) = next > 6 ? 0 : next;
    int32_t x = S16(st, b + BLD_X), y = S16(st, b + BLD_Y);
    switch (side) {
    case 0: y -= 8; break;
    case 2: x += 8; break;
    case 4: y += 8; break;
    case 6: x -= 8; break;
    default: break;
    }
    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    if (x >= S32(st, G_MAP_WIDTH)) {
        x = S32(st, G_MAP_WIDTH) - 1;
    }
    if (y >= S32(st, G_MAP_HEIGHT)) {
        y = S32(st, G_MAP_HEIGHT) - 1;
    }
    if (GAME_CALL(st, FN_BUILDING_ROAD_WITHIN, x, y, 1, 6) != 0) {
        S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
        S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
    } else {
        U8(st, f + FIG_ROAM_STATE) = 1;
    }
}

static int32_t run_create(game_state *st, const int32_t *a)
{
    return figure_create(st, a[0], a[1], a[2], a[3], a[4]);
}

static int32_t run_referenced(game_state *st, const int32_t *a)
{
    return figure_referenced(st, a[0]);
}

static int32_t run_assign_slot(game_state *st, const int32_t *a)
{
    figure_assign_slot(st, a[0]);
    return 0;
}

static int32_t run_walker_start(game_state *st, const int32_t *a)
{
    figure_walker_start(st, a[0]);
    return 0;
}

const ported_function bigb_figure_ports[] = {
    {0x004B4800u, "figure_create", 5, 32, 0, run_create, 0},
    {0x004B47A0u, "figure_referenced", 1, 8, 0, run_referenced, 0},
    {0x004B1DF0u, "figure_walker_start", 1, 0, 0, run_walker_start, 0},
    {0x004B5AF0u, "figure_assign_slot", 1, 0, 0, run_assign_slot, 0},
};
const int bigb_figure_ports_count = sizeof(bigb_figure_ports) / sizeof(bigb_figure_ports[0]);
