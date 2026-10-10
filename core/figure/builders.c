/* Imhotep - monument workers looking for a free work place (roadmap phase 8.2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Reached from the figure actions 0x004A8800 (action 0x10) and 0x004A98B0 (core/figure/actions_d.c).
 * The work places: 0x00EAAF58 entries (building ids, i32) at 0x00EAAC44; a building's worker in
 * +0x64 (i16); workers are figures of type 0x51 at action 0x0B or 0x50 at action 0x0C. */
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"

#define BREC(id) BUILDING_ADDR((int32_t) (id))
#define FIG(id) (FIGURE_BASE + 8u + 0x184u * (uint32_t) (int32_t) (id))

/* the work place building b is free: no worker, or its worker is not (type 0x51 at action 0x0B
 * or type 0x50 at action 0x0C) */
static int place_free(game_state *st, int32_t b)
{
    int32_t w = S16(st, BREC(b) + BLD_U64);
    if (w <= 0) {
        return 1;
    }
    uint32_t W = FIG(w);
    uint8_t t = U8(st, W + 2);
    if (t == 0x51 && U8(st, W + 0x30) == 0x0B) {
        return 0;
    }
    if (t == 0x50 && U8(st, W + 0x30) == 0x0C) {
        return 0;
    }
    return 1;
}

/* the first listed work place equal to b and free (its index), or -1 */
static int32_t find_free(game_state *st, int32_t b)
{
    for (int32_t i = 0; i < S32(st, G_PLACE_COUNT); i++) {
        int32_t p = S32(st, G_PLACES + 4u * (uint32_t) i);
        if (p == b && place_free(st, p)) {
            return i;
        }
    }
    return -1;
}

void worker_find_place_4f2230(game_state *st, int32_t f)
{
    /* @bin 0x004F2230 a worker (figure f) with +0x118 (i16) below 1 goes to action 0x0C. Else,
     * when 0x004F1D10(f, 1) says 1: 0x004F2410(f), 0x004EF450(main building of +0x46); its building
     * b, free in the list: action 0x0B, +0x2E := 0, b's worker := f. Else the next buildings from
     * b (0x004F2190) until one is free in the list: its worker := f, 0x004F02F0(f, 0x004F2190(b),
     * +0x48, 1); back at b: action 0x0C, +0x118 := 0 */
    uint32_t F = FIG(f);
    if (S16(st, F + 0x118) < 1) {
        U8(st, F + 0x30) = 0x0C;
        return;
    }
    if ((uint8_t) GAME_CALL(st, FN_FIGURE_PYRAMID_STEP, f, 1) != 1) {
        return;
    }
    GAME_CALL(st, FN_WORKER_JOIN, f);
    int32_t b = S16(st, F + 0x46);
    GAME_CALL(st, FN_PYRAMID_LEVEL_PARTS, b - S16(st, BREC(b) + BLD_PART));
    int32_t i = find_free(st, b);
    if (i >= 0) {
        int32_t p = S32(st, G_PLACES + 4u * (uint32_t) i);
        U8(st, F + 0x30) = 0x0B;
        S16(st, F + 0x2E) = 0;
        S16(st, BREC(p) + BLD_U64) = (int16_t) f;
        return;
    }
    int32_t cur = b;
    for (;;) {
        i = find_free(st, cur);
        if (i >= 0) {
            int32_t p = S32(st, G_PLACES + 4u * (uint32_t) i);
            int32_t arg = S16(st, F + 0x48);
            S16(st, BREC(p) + BLD_U64) = (int16_t) f;
            int32_t next = GAME_CALL(st, FN_PYRAMID_NEXT_PART, b);
            GAME_CALL(st, FN_FIGURE_PYRAMID_TARGET, f, next, arg, 1);
            return;
        }
        cur = GAME_CALL(st, FN_PYRAMID_NEXT_PART, cur);
        if (cur == b) {
            U8(st, F + 0x30) = 0x0C;
            S16(st, F + 0x118) = 0;
            return;
        }
    }
}

void worker_release_4f2960(game_state *st, int32_t f)
{
    /* @bin 0x004F2960 the building on the figure's tile (+0x20 -> 0x00B4E6FC): 0x004EF450(its main
     * building); its worker (0x004F28E0): type 0x50 -> action 0x0C, 0x51 -> action 0x0B; its +0x2E
     * := 0 */
    int32_t b = S16(st, G_GRID_BUILDING + 2u * U32(st, FIG(f) + 0x20));
    GAME_CALL(st, FN_PYRAMID_LEVEL_PARTS, b - S16(st, BREC(b) + BLD_PART));
    int32_t g = GAME_CALL(st, FN_PYRAMID_LINKED_WORKER, b);
    if (g <= 0) {
        return;
    }
    uint32_t G = FIG(g);
    uint8_t t = U8(st, G + 2);
    if (t == 0x50) {
        U8(st, G + 0x30) = 0x0C;
    } else if (t == 0x51) {
        U8(st, G + 0x30) = 0x0B;
    }
    S16(st, G + 0x2E) = 0;
}

static int32_t run_4f2230(game_state *st, const int32_t *a)
{
    worker_find_place_4f2230(st, a[0]);
    return 0;
}

static int32_t run_4f2960(game_state *st, const int32_t *a)
{
    worker_release_4f2960(st, a[0]);
    return 0;
}

const ported_function builder_ports[] = {
    {0x004F2230u, "worker_find_place_4f2230", 1, 0, 0, run_4f2230, 0},
    {0x004F2960u, "worker_release_4f2960", 1, 0, 0, run_4f2960, 0},
};
const int builder_ports_count = sizeof(builder_ports) / sizeof(builder_ports[0]);
