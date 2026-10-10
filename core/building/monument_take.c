/* Imhotep - monument parts taking a worker figure (package terr).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The take handlers of the monuments ask a part whether it takes a figure (a stone mason 0x4F /
 * 0x51 / 0x6C, a sledge puller 0x55) and set the figure's destination and action. The part keeps
 * its worker in +0x5C; the vtable method +0x24 of the monument object says whether that worker is
 * still busy there (and forgets it otherwise). */
#include "building/monument_take.h"

#include "building/building_spawn.h"
#include "city/day2_monument.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

#define B(id) BUILDING_ADDR(id)
#define F(id) FIGURE_ADDR(id)
#define PART(id) S16(st, B(id) + BLD_PART)

static int32_t worker_busy(game_state *st, uint32_t wslot, uint8_t mason, uint8_t act2, uint8_t other,
                           uint8_t act3)
{
    /* the worker of +0x5C (wslot) is in place: alive (+0xE), action (+0x38; 6 / 7: the saved
     * +0x10) 9 or act2 for type `mason`, 9 or act3 for type `other`; else +0x5C := 0 */
    int32_t w = S32(st, wslot);
    if (w == 0) {
        return 0;
    }
    uint32_t fa = F(w);
    if (U8(st, fa + FIG_STATE) != 0) {
        uint32_t a = U8(st, fa + FIG_ACTION);
        if (a == 6 || a == 7) {
            a = U8(st, fa + FIG_ACTION_BEFORE_ATTACK);
        }
        if (U8(st, fa + FIG_TYPE) == mason) {
            return a == 9 || a == act2;
        }
        if (U8(st, fa + FIG_TYPE) == other) {
            return a == 9 || a == act3;
        }
    }
    S32(st, wslot) = 0;
    return 0;
}

int32_t monument_worker_busy_5477e0(game_state *st, int32_t id)
{
    /* @bin 0x005477E0 (thiscall, id) part id's worker: type 0x51 at action 9 / 0xB, type 0x6C at
     * 9 / 0xC */
    return worker_busy(st, B(id) + BLD_MONUMENT_WORKER, 0x51, 0xB, 0x6C, 0xC);
}

int32_t monument_worker_busy_401b40(game_state *st, int32_t id)
{
    /* @bin 0x00401B40 (thiscall, id) the main building's worker: type 0x4F at 9 / 0xA, type
     * 0x51 at 9 / 0xB */
    return worker_busy(st, B(id - PART(id)) + BLD_MONUMENT_WORKER, 0x4F, 0xA, 0x51, 0xB);
}

static int32_t method_busy(game_state *st, uint32_t obj, int32_t id)
{
    /* the vtable method +0x24 */
    uint32_t m = U32(st, U32(st, obj) + 0x24);
    switch (m) {
    case FN_MONUMENT_WORKER_BUSY_5477E0: return monument_worker_busy_5477e0(st, id);
    case FN_MONUMENT_WORKER_BUSY_401B40: return monument_worker_busy_401b40(st, id);
    default: return (uint8_t) GAME_CALLT(st, obj, m, id);
    }
}

int32_t monument_needs_worker(game_state *st, int32_t id, int32_t f)
{
    /* @bin 0x00420C40 (id, figure) an entry of table 0x005A8140 (16 bytes: step, figure type,
     * record with +0x12 == 0 and +0xC > 0) for the main building's step +0x54 and the figure's
     * type: id, else 0 */
    int32_t step = S32(st, B(id - PART(id)) + BLD_U54);
    for (uint32_t e = G_WORK_SLOTS_E6; e + WORKSLOT_ANIM < G_WORK_SLOTS_E6 + WORKSLOT_ANIM + 0x10 * 56; e += 0x10) {
        uint32_t rec = U32(st, e + WORKSLOT_ANIM);
        if (S16(st, rec + ANIMOBJ_FIGURE) == 0 && S32(st, e + WORKSLOT_KEY) == step && S32(st, e + WORKSLOT_FIGURE_TYPE) == U8(st, F(f) + FIG_TYPE) &&
            S32(st, rec + ANIMOBJ_WORK) > 0) {
            return id;
        }
    }
    return 0;
}

static void head_to(game_state *st, uint32_t fa, uint32_t b)
{
    /* figure +0x24 / +0x26 := the building's +0x26 / +0x28, +0x28 := its tile, +0x44 := 0 */
    S16(st, fa + FIG_DEST_X) = S16(st, b + BLD_ROAD_ACCESS_X);
    S16(st, fa + FIG_DEST_Y) = S16(st, b + BLD_ROAD_ACCESS_Y);
    S32(st, fa + FIG_U28) = S32(st, b + BLD_GRID_OFFSET);
    S16(st, fa + FIG_ROAM_LENGTH) = 0;
}

int32_t monument_take_e6(game_state *st, uint32_t obj, int32_t id, int32_t f, int32_t take, int32_t extra)
{
    /* @bin 0x00401D40 (thiscall, id, figure, take, extra) the main building's entry (object +0x58
     * + 0x34 * part) 0..11, no busy worker (+0x24), work for the figure (0x00420C40): al = 1;
     * with take: a mason 0x4F gets action 9 and heads there (extra: action 0xA, stays), a 0x51
     * action 9; the main building's worker +0x5C := figure, figure +0x5E := main building */
    int32_t main = id - PART(id);
    uint32_t mb = B(main);
    int32_t v = S32(st, obj + MONOBJ_PART_KIND + 0x34u * (uint32_t) PART(main));
    if (v < 0 || v > 11) {
        return 0;
    }
    if ((uint8_t) method_busy(st, obj, id) != 0) {
        return 0;
    }
    if (monument_needs_worker(st, id, f) == 0) {
        return 0;
    }
    if ((uint8_t) take == 0) {
        return 1;
    }
    uint32_t fa = F(f);
    uint8_t type = U8(st, fa + FIG_TYPE);
    if (type == 0x4F) {
        if ((uint8_t) extra != 0) {
            U8(st, fa + FIG_ACTION) = 0xA;
        } else {
            U8(st, fa + FIG_ACTION) = 9;
            head_to(st, fa, mb);
            S16(st, fa + FIG_WAIT_TICKS) = 0;
        }
        S16(st, fa + FIG_DEST_BUILDING) = (int16_t) main;
        S32(st, mb + BLD_MONUMENT_WORKER) = f;
        S16(st, fa + FIG_WAIT_TICKS) = 0;
    } else if (type == 0x51) {
        S32(st, mb + BLD_MONUMENT_WORKER) = f;
        U8(st, fa + FIG_ACTION) = 9;
        S16(st, fa + FIG_DEST_BUILDING) = (int16_t) main;
        head_to(st, fa, mb);
        S16(st, fa + FIG_WAIT_TICKS) = 0;
    }
    return 1;
}

int32_t monument_take_547c10(game_state *st, uint32_t obj, int32_t id, int32_t f, int32_t take)
{
    /* @bin 0x00547C10 (thiscall, id, figure, take; the third argument's slot is reused for
     * "figure type 0x55"): no busy worker (+0x24); a main building (part 0) without its sledge
     * (object +0x4C) and +0x98 < 400 with figure's low byte non-zero: a supplier of good 0x22
     * (0x0045BB50) sends a sledge puller 0x55 there (action 0xF, object +0x4C := 1); else the main
     * building's stock +0x98 >= 100 and a part needing work (0x00422A90) take the figure
     * (stock - 100, part's worker, action 9 for masons 0x51 / 0x6C). al = 1 when taken. */
    (void) take;
    uint32_t fa = F(f);
    int is_sledge = U8(st, fa + FIG_TYPE) == 0x55;
    if ((uint8_t) method_busy(st, obj, id) != 0) {
        return 0;
    }
    if ((uint8_t) f != 0 && PART(id) == 0 && U8(st, obj + TOMBOBJ_SLEDGE) == 0 && S32(st, B(id) + BLD_TOMB_STOCK) < 400) {
        int32_t s = building_find_supplier(st, id, 0x22, 1);
        if (s == 0) {
            return 0;
        }
        if (!is_sledge) {
            return 1;
        }
        U8(st, fa + FIG_COLLECTING) = 0x22;
        S16(st, fa + FIG_AMOUNT) = 0;
        S16(st, fa + FIG_DEST_BUILDING) = (int16_t) s;
        S16(st, fa + FIG_LEADER) = (int16_t) f;
        S16(st, fa + FIG_TARGET_HOUSE) = (int16_t) id;
        U8(st, fa + FIG_ACTION) = 0xF;
        S16(st, fa + FIG_WAIT_TICKS) = 0;
        S16(st, fa + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
        S16(st, fa + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
        S32(st, fa + FIG_U28) = S32(st, B(s) + BLD_GRID_OFFSET);
        S16(st, fa + FIG_ROAM_LENGTH) = 0;
        U8(st, obj + TOMBOBJ_SLEDGE) = 1;
        return 1;
    }
    uint32_t stock = B(id - PART(id)) + BLD_TOMB_STOCK;
    if (S32(st, stock) < 100) {
        return 0;
    }
    int32_t w = GAME_CALL(st, FN_MONUMENT_PART_TAKES, id, f);
    if (w == 0) {
        return 0;
    }
    if (!is_sledge) {
        return 1;
    }
    uint8_t type = U8(st, fa + FIG_TYPE);
    if (type == 0x51 || type == 0x6C) {
        U8(st, fa + FIG_ACTION) = 9;
    }
    S32(st, stock) -= 100;
    S32(st, B(w) + BLD_MONUMENT_WORKER) = f;
    head_to(st, fa, B(w));
    S16(st, fa + FIG_WAIT_TICKS) = 0;
    return 1;
}

/* ---- sun temple (0xD5) -------------------------------------------------------------------- */

int32_t sun_temple_needs_worker(game_state *st, int32_t id, int32_t f)
{
    /* @bin 0x00420430 (id, figure) an entry of table 0x00596080 (16 bytes: stage, figure type,
     * record with +0x12 == 0 and +0xC > 0) for the part's stage +0x58 and the figure's type: id,
     * else 0 */
    int32_t stage = S32(st, B(id) + BLD_MONUMENT_STAGE);
    for (uint32_t e = G_WORK_SLOTS_D5; e + WORKSLOT_ANIM < G_WORK_SLOTS_D5 + WORKSLOT_ANIM + 0x10 * 13; e += 0x10) {
        uint32_t rec = U32(st, e + WORKSLOT_ANIM);
        if (S16(st, rec + ANIMOBJ_FIGURE) == 0 && S32(st, e + WORKSLOT_KEY) == stage && S32(st, e + WORKSLOT_FIGURE_TYPE) == U8(st, F(f) + FIG_TYPE) &&
            S32(st, rec + ANIMOBJ_WORK) > 0) {
            return id;
        }
    }
    return 0;
}

static void head_to_part(game_state *st, uint32_t fa, uint32_t b)
{
    /* @bin 0x00542A0E figure +0x24 / +0x26 / +0x28 := the part's +0x26 / +0x28 / tile, +0x44 := 0 */
    S16(st, fa + FIG_DEST_X) = S16(st, b + BLD_ROAD_ACCESS_X);
    S16(st, fa + FIG_DEST_Y) = S16(st, b + BLD_ROAD_ACCESS_Y);
    S32(st, fa + FIG_U28) = S32(st, b + BLD_GRID_OFFSET);
    S16(st, fa + FIG_ROAM_LENGTH) = 0;
}

static int32_t sledge_for_part(game_state *st, int32_t id, int32_t f, int32_t take, int extra)
{
    /* @bin 0x00542334 (and 0x00542523, 0x0054284E) a sledge puller (0x55) for the part's worker
     * W (+0x5C): W's sledge (+0x68), a live 0x55 at action 6 / 7 / 0xF / 0x10 / 0x17 pointing
     * back at W, keeps it (al = 0); else W's sledge := 0. With take: a supplier of good 0x1E
     * (0x0045BB50 (id, 0x1E, 1)) sends the figure there (action 0xF, +0x68 := W, W's sledge :=
     * figure; kind 12: +0x72 := 3); no supplier: figure +0xE := 2. al = 1. */
    int32_t w = S32(st, B(id) + BLD_MONUMENT_WORKER);
    uint32_t wa = F(w);
    int32_t sledge = S16(st, wa + FIG_LEADER);
    if (sledge != 0) {
        uint32_t sa = F(sledge);
        if (U8(st, sa + FIG_STATE) != 0 && U8(st, sa + FIG_TYPE) == 0x55) {
            uint8_t a = U8(st, sa + FIG_ACTION);
            if ((a == 6 || a == 7 || a == 0xF || a == 0x10 || a == 0x17) && S16(st, sa + FIG_LEADER) == w) {
                return 0;
            }
        }
        S16(st, wa + FIG_LEADER) = 0;
    }
    if ((uint8_t) take == 0) {
        return 1;
    }
    uint32_t fa = F(f);
    int32_t s = building_find_supplier(st, id, 0x1E, 1);
    if (s == 0) {
        U8(st, fa + FIG_STATE) = 2;
        return 1;
    }
    U8(st, fa + FIG_COLLECTING) = 0x1E;
    S16(st, fa + FIG_AMOUNT) = 0;
    S16(st, fa + FIG_DEST_BUILDING) = (int16_t) s;
    S16(st, fa + FIG_LEADER) = (int16_t) w;
    U8(st, fa + FIG_ACTION) = 0xF;
    S16(st, fa + FIG_WAIT_TICKS) = 0;
    S16(st, fa + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
    S16(st, fa + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
    if (extra) {
        U8(st, fa + FIG_TERRAIN_USAGE) = 3;
    }
    S16(st, wa + FIG_LEADER) = (int16_t) f;
    return 1;
}

static int32_t mason_for_part(game_state *st, int32_t id, int32_t f, int32_t take)
{
    /* @bin 0x00542433 a stone mason 0x51 for the part (no worker busy 0x00542110): action 9, the
     * part's worker := figure, heads there */
    if ((uint8_t) monument_d5_worker_busy(st, id) != 0 || U8(st, F(f) + FIG_TYPE) != 0x51) {
        return 0;
    }
    if ((uint8_t) take != 0) {
        U8(st, F(f) + FIG_ACTION) = 9;
        S32(st, B(id) + BLD_MONUMENT_WORKER) = f;
        head_to_part(st, F(f), B(id));
    }
    return 1;
}

static int32_t start_part(game_state *st, int32_t id, int32_t f, int32_t take, int is_sledge)
{
    /* @bin 0x00542479 a part at stage 0 takes a sledge puller (no worker busy): action 0x18, the
     * part's worker := figure, heads there */
    if (S32(st, B(id) + BLD_MONUMENT_STAGE) != 0 || !is_sledge || (uint8_t) monument_d5_worker_busy(st, id) != 0) {
        return 0;
    }
    if ((uint8_t) take != 0) {
        U8(st, F(f) + FIG_ACTION) = 0x18;
        S32(st, B(id) + BLD_MONUMENT_WORKER) = f;
        head_to_part(st, F(f), B(id));
    }
    return 1;
}

static int32_t stage_part(game_state *st, int32_t id, int32_t f, int32_t take, int is_sledge, int extra)
{
    /* a part at a working stage: sledge pullers bring stone to its worker, masons work it */
    if (is_sledge) {
        if ((uint8_t) monument_d5_worker_busy(st, id) == 0) {
            return 0;
        }
        return sledge_for_part(st, id, f, take, extra);
    }
    return mason_for_part(st, id, f, take);
}

int32_t sun_temple_take(game_state *st, int32_t id, int32_t f, int32_t take)
{
    /* @bin 0x005421D0 (id, figure, take) by the part's kind (0x00601DE4 + 0x34 * part: 0, 1,
     * 2-7, 8-11, 12) and the main building's progress flags +0x64 (bits 0-4) and the part's stage
     * +0x58: the temple (kind 0) takes masons 0x4F (action 0xA) / 0x51 (0xB) when it needs them
     * (0x00420430); the other parts take sledge pullers to start them or bring stone, and masons.
     * al = 1 when the figure is taken (or could be, without take). */
    uint32_t fa = F(f);
    int is_sledge = U8(st, fa + FIG_TYPE) == 0x55;
    uint32_t b = B(id);
    uint32_t flags = U32(st, B(id - PART(id)) + BLD_MONUMENT_FLAGS);
    int32_t kind = S32(st, G_SUN_TEMPLE_PART_KIND + 0x34u * (uint32_t) PART(id));
    int32_t stage;
    switch (kind) {
    case 0:
        if ((uint8_t) monument_d5_worker_busy(st, id) != 0 || sun_temple_needs_worker(st, id, f) == 0) {
            return 0;
        }
        if ((uint8_t) take == 0) {
            return 1;
        }
        if (U8(st, fa + FIG_TYPE) == 0x4F) {
            U8(st, fa + FIG_ACTION) = 0xA;
        } else if (U8(st, fa + FIG_TYPE) == 0x51) {
            U8(st, fa + FIG_ACTION) = 0xB;
        }
        S32(st, b + BLD_MONUMENT_WORKER) = f;
        head_to_part(st, fa, b);
        S16(st, fa + FIG_WAIT_TICKS) = 0;
        return 1;
    case 1: case 2: case 3: case 4: case 5: case 6: case 7:
        if (!(flags >> (kind == 1 ? 0 : 2) & 1)) {
            return start_part(st, id, f, take, is_sledge);
        }
        stage = S32(st, b + BLD_MONUMENT_STAGE);
        if (stage == 0) {
            return start_part(st, id, f, take, is_sledge);
        }
        if (stage != 1) {
            return 0;
        }
        return stage_part(st, id, f, take, is_sledge, 0);
    case 8: case 9: case 10: case 11:
        if (!is_sledge || (uint8_t) monument_d5_worker_busy(st, id) != 0) {
            return 0;
        }
        if (!(flags >> 1 & 1)) {
            if (S32(st, b + BLD_MONUMENT_STAGE) != 0) {
                return 0;
            }
        } else if (!(flags & 1) || S32(st, b + BLD_MONUMENT_STAGE) != 1) {
            return 0;
        }
        if ((uint8_t) take != 0) {
            U8(st, fa + FIG_ACTION) = 0x18;
            S32(st, b + BLD_MONUMENT_WORKER) = f;
            head_to_part(st, fa, b);
        }
        return 1;
    case 12:
        if (!(flags >> 4 & 1) || !(flags >> 3 & 1)) {
            return start_part(st, id, f, take, is_sledge);
        }
        stage = S32(st, b + BLD_MONUMENT_STAGE);
        if (stage == 0) {
            return start_part(st, id, f, take, is_sledge);
        }
        if (stage != 1 && stage != 2 && stage != 3) {
            return 0;
        }
        return stage_part(st, id, f, take, is_sledge, 1);
    default:
        return 0;
    }
}

/* ---- registration -------------------------------------------------------------------------- */

#define THIS(a) ((uint32_t) (a)[PORT_THIS_ARG])

static int32_t run_busy_5477e0(game_state *st, const int32_t *a) { return monument_worker_busy_5477e0(st, a[0]); }
static int32_t run_busy_401b40(game_state *st, const int32_t *a) { return monument_worker_busy_401b40(st, a[0]); }
static int32_t run_needs(game_state *st, const int32_t *a) { return monument_needs_worker(st, a[0], a[1]); }
static int32_t run_take_e6(game_state *st, const int32_t *a) { return monument_take_e6(st, THIS(a), a[0], a[1], a[2], a[3]); }
static int32_t run_sun_needs(game_state *st, const int32_t *a) { return sun_temple_needs_worker(st, a[0], a[1]); }
static int32_t run_sun_take(game_state *st, const int32_t *a) { return sun_temple_take(st, a[0], a[1], a[2]); }
static int32_t run_take_547c10(game_state *st, const int32_t *a) { return monument_take_547c10(st, THIS(a), a[0], a[1], a[2]); }

const ported_function terr_take_ports[] = {
    {0x005477E0u, "monument_worker_busy_5477e0", 1, 8, 0, run_busy_5477e0, 0},
    {0x00401B40u, "monument_worker_busy_401b40", 1, 8, 0, run_busy_401b40, 0},
    {0x00420C40u, "monument_needs_worker", 2, 32, 0, run_needs, 0},
    {0x00401D40u, "monument_take_e6", 4, 8, 0, run_take_e6, 0},
    {0x00547C10u, "monument_take_547c10", 3, 8, 0, run_take_547c10, 0},
    {0x00420430u, "sun_temple_needs_worker", 2, 32, 0, run_sun_needs, 0},
    {0x005421D0u, "sun_temple_take", 3, 8, 0, run_sun_take, 0},
};
const int terr_take_ports_count = sizeof(terr_take_ports) / sizeof(terr_take_ports[0]);
