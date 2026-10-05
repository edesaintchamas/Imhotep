/* Imhotep - buildings of types 0xB1, 0xB2, 0xB3, 0xE7: their workers (figures 0x4F, 0x50, 0x51,
 * 0x6C), counted in +0xA6 (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Building record: docs/formats/save.md §5 (chunk 32), figure record: chunk 21. */
#include "building/building_spawn.h"
#include "figure/figure_create.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define ROAD_X 0x00D3B4E4u
#define ROAD_Y 0x00D3B4E0u
#define CURRENT_FIGURE 0x00D056F8u

/* @bin 0x004612C0 .. the common start: the workers allowed by the worker ratio (1 from `low`),
 * 0 to stop */
static int32_t workers_allowed(game_state *st, int32_t id, int32_t low)
{
    uint32_t b = BUILDING_ADDR(id);
    if (S16(st, b + 0x18) <= 0) {
        U8(st, b + 0xC3) = 2;
    }
    if (GAME_CALL(st, 0x00480180u, S16(st, b + 6), S16(st, b + 8), U8(st, b + 3)) == 0) {
        return 0;
    }
    if (S16(st, b + 0x18) <= 0x32) {
        building_spawn_type5(st, id);
    }
    int32_t r = GAME_CALL(st, 0x004CDE60u, S16(st, b + 0x40),
                          S32(st, 0x00705EB8u + 0x28u * (uint32_t) (int32_t) S16(st, b + 0x10)));
    return r >= 100 ? 4 : r >= 75 ? 3 : r >= 50 ? 2 : r >= low ? 1 : 0;
}

/* a new worker of `type`; +0xA6 counts it */
static int32_t new_worker(game_state *st, int32_t id, int32_t type)
{
    uint32_t b = BUILDING_ADDR(id);
    int32_t f = figure_create(st, U8(st, b + 1), type, S32(st, ROAD_X), S32(st, ROAD_Y), 0);
    uint32_t a = FIGURE_ADDR(f);
    U8(st, a + 0x38) = 8;
    S16(st, a + 0x5A) = (int16_t) id;
    S16(st, a + 0x36) = 0;
    figure_walker_start(st, f);
    U8(st, b + 0xA6) += 1;
    S16(st, a + 0x44) = 0;
    return f;
}

/* @bin 0x00461428 the worker takes 100 from `stock` with the help of `test` (0x004BC140 / 0x004BC2E0
 * / 0x004BC3B0); 1 if so */
static int carries(game_state *st, uint32_t stock, int32_t f, uint32_t test)
{
    uint32_t a = FIGURE_ADDR(f);
    if (GAME_CALL(st, test, S16(st, a + 0x14), S16(st, a + 0x16), f) == 0) {
        return 0;
    }
    U8(st, a + 0x38) = 9;
    GAME_CALL(st, 0x004B3250u, f);
    int32_t cur = S32(st, CURRENT_FIGURE);
    S16(st, stock) = (int16_t) (S16(st, stock) - 100);
    S16(st, FIGURE_ADDR(cur) + 0x44) = 0;
    if (S16(st, stock) < 0) {
        S16(st, stock) = 0;
    }
    return 1;
}

static void no_carry(game_state *st, int32_t f)
{
    U8(st, FIGURE_ADDR(f) + 0x65) = 1;
    U8(st, FIGURE_ADDR(f) + 0x38) = 4;
}

static int room_for_one(game_state *st, uint32_t b, int32_t n)
{
    if ((int32_t) U8(st, b + 0xA6) >= n) {
        return 0;
    }
    U8(st, b + 0x32) = 0;
    return (int32_t) U8(st, b + 0xA6) < n;
}

void building_update_0xb1(game_state *st, int32_t id)
{
    /* @bin 0x004612C0 figure 0x4F once the progress +0x54 reaches 400 */
    uint32_t b = BUILDING_ADDR(id);
    int32_t n = workers_allowed(st, id, 1);
    if (n == 0 || !room_for_one(st, b, n) || S16(st, b + 0x54) < 400) {
        return;
    }
    int32_t f = figure_create(st, U8(st, b + 1), 0x4F, S32(st, ROAD_X), S32(st, ROAD_Y), 0);
    uint32_t a = FIGURE_ADDR(f);
    U8(st, a + 0x38) = 8;
    S16(st, a + 0x5A) = (int16_t) id;
    S16(st, a + 0x36) = 0;
    U8(st, b + 0xA6) += 1;
    figure_walker_start(st, f);
    S16(st, a + 0x44) = 0;
    if (GAME_CALL(st, 0x004BC140u, S16(st, a + 0x14), S16(st, a + 0x16), f) == 0) {
        no_carry(st, f);
        return;
    }
    U8(st, a + 0x38) = 9;
    GAME_CALL(st, 0x004B3250u, f);
    int32_t cur = S32(st, CURRENT_FIGURE);
    S16(st, b + 0x54) = 0;
    S16(st, b + 0x3C) = (int16_t) (S16(st, b + 0x3C) - 100);
    S16(st, FIGURE_ADDR(cur) + 0x44) = 0;
    if (S16(st, b + 0x3C) < 0) {
        S16(st, b + 0x3C) = 0;
    }
}

void building_update_0xb3(game_state *st, int32_t id)
{
    /* @bin 0x00461490 figure 0x51 */
    uint32_t b = BUILDING_ADDR(id);
    int32_t n = workers_allowed(st, id, 25);
    if (n == 0 || !room_for_one(st, b, n)) {
        return;
    }
    new_worker(st, id, 0x51);
    S16(st, b + 0x54) = 0;
}

void building_update_0xb2(game_state *st, int32_t id)
{
    /* @bin 0x004615D0 figure 0x50, the stock +0x3C emptied */
    uint32_t b = BUILDING_ADDR(id);
    int32_t n = workers_allowed(st, id, 25);
    if (n == 0 || !room_for_one(st, b, n)) {
        return;
    }
    new_worker(st, id, 0x50);
    S16(st, b + 0x54) = 0;
    S16(st, b + 0x3C) = 0;
}

void building_update_0xe7(game_state *st, int32_t id)
{
    /* @bin 0x00461720 figure 0x6C, carrying 100 of +0xCC (0x004BC2E0) or else of +0x3C (0x004BC3B0) */
    uint32_t b = BUILDING_ADDR(id);
    int32_t n = workers_allowed(st, id, 25);
    if (n == 0 || !room_for_one(st, b, n)) {
        return;
    }
    int32_t f = new_worker(st, id, 0x6C);
    S16(st, b + 0x54) = 0;
    if (S16(st, b + 0xCC) >= 100 && carries(st, b + 0xCC, f, 0x004BC2E0u)) {
        return;
    }
    if (S16(st, b + 0x3C) >= 100 && carries(st, b + 0x3C, f, 0x004BC3B0u)) {
        return;
    }
    no_carry(st, f);
}

#define VOID1(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        fn(st, a[0]); \
        return 0; \
    }
VOID1(run_0xb1, building_update_0xb1)
VOID1(run_0xb3, building_update_0xb3)
VOID1(run_0xb2, building_update_0xb2)
VOID1(run_0xe7, building_update_0xe7)

const ported_function bigb_gatherer_ports[] = {
    {0x004612C0u, "building_update_0xb1", 1, 0, 0, run_0xb1, 0},
    {0x00461490u, "building_update_0xb3", 1, 0, 0, run_0xb3, 0},
    {0x004615D0u, "building_update_0xb2", 1, 0, 0, run_0xb2, 0},
    {0x00461720u, "building_update_0xe7", 1, 0, 0, run_0xe7, 0},
};
const int bigb_gatherer_ports_count = sizeof(bigb_gatherer_ports) / sizeof(bigb_gatherer_ports[0]);
