/* Imhotep - more helpers of the daily building update 0x00462180 (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Building record: docs/formats/save.md §5 (chunk 32), figure record: chunk 21. */
#include "building/building_spawn.h"
#include "figure/figure_create.h"
#include "figure/formation.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


int32_t building_send_0x2a(game_state *st, int32_t id)
{
    /* @bin 0x004B7400 while 0x00BCB9AC > 0: a figure 0x2A for the first building of type 0xAD of the
     * same +0x14 with workers and no walker (ids 1 .. highest - 1) */
    if (S32(st, G_TOWER_SENTRY_REQUEST) <= 0) {
        return 0;
    }
    int32_t n = S32(st, G_BUILDING_HIGHEST);
    uint32_t b = BUILDING_ADDR(id);
    for (int32_t i = 1; i < n; i++) {
        uint32_t t = BUILDING_ADDR(i);
        if (U8(st, t + BLD_STATE) != 1 || U8(st, t + BLD_ROAD_NETWORK) != U8(st, b + BLD_ROAD_NETWORK) || S16(st, t + BLD_TYPE) != 0xAD ||
            S16(st, t + BLD_WORKERS) <= 0 || S16(st, t + BLD_WALKER) != 0) {
            continue;
        }
        int32_t f = figure_create(st, U8(st, b + BLD_OWNER), 0x2A, S32(st, G_ROAD_ACCESS_X), S32(st, G_ROAD_ACCESS_Y), 0);
        uint32_t a = FIGURE_ADDR(f);
        U8(st, a + FIG_ACTION) = 0xC;
        if (GAME_CALL(st, FN_BUILDING_ROAD_ACCESS, S16(st, t + BLD_X), S16(st, t + BLD_Y), U8(st, t + BLD_SIZE)) == 0) {
            U8(st, a + FIG_STATE) = 2;
        }
        S16(st, a + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
        S16(st, a + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
        S16(st, a + FIG_HOME) = (int16_t) i;
        S16(st, t + BLD_WALKER) = (int16_t) f;
        return 1;
    }
    return 0;
}

void building_update_0xc7(game_state *st, int32_t id)
{
    /* @bin 0x004BED70 (thiscall, this 0x00D05720 unused) up to 4 walkers 0x55 (+0xA6) */
    uint32_t b = BUILDING_ADDR(id);
    if (S16(st, b + BLD_SERVICE) <= 0) {
        U8(st, b + BLD_PROBLEM_OVERLAY) = 2;
    }
    if (GAME_CALL(st, FN_BUILDING_ROAD_ACCESS, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE)) == 0) {
        return;
    }
    if (S16(st, b + BLD_SERVICE) <= 0x32) {
        building_spawn_type5(st, id);
    }
    int32_t r = GAME_CALL(st, FN_CALC_RATIO_PERCENT, S16(st, b + BLD_WORKERS),
                          S32(st, G_TYPE_WORKERS + 0x28u * (uint32_t) (int32_t) S16(st, b + BLD_TYPE)));
    int32_t delay = r >= 100 ? 3 : r >= 75 ? 7 : r >= 50 ? 15 : r >= 25 ? 29 : r >= 1 ? 0x2F : -1;
    if (delay < 0 || U8(st, b + BLD_SHOWS) >= 4) {
        return;
    }
    uint8_t c = (uint8_t) (U8(st, b + BLD_SPAWN_DELAY) + 1);
    U8(st, b + BLD_SPAWN_DELAY) = c;
    if ((int32_t) c <= delay) {
        return;
    }
    U8(st, b + BLD_SPAWN_DELAY) = 0;
    int32_t f = figure_create(st, U8(st, b + BLD_OWNER), 0x55, S32(st, G_ROAD_ACCESS_X), S32(st, G_ROAD_ACCESS_Y), 0);
    S16(st, b + BLD_WALKER) = (int16_t) f;
    U8(st, b + BLD_SHOWS) += 1;
    U8(st, FIGURE_ADDR(f) + FIG_ACTION) = 8;
    S16(st, FIGURE_ADDR(f) + FIG_HOME) = (int16_t) id;
}

int32_t farm_harvest(game_state *st, int32_t id)
{
    /* @bin 0x00488400 (thiscall, this 0x00BCA350) on days 0 and 7: a farm (0x00488030) on the
     * floodplain (0x004894A0, flood state 0x00D386D4 == 1) or off it (its crop flag this[kind], day 0)
     * harvests: 8 x 0x004883D0 (16 x with city +0x4994; 0x00BCA348 := 1), progress reset (0x00455C30,
     * 0x00488E60, 0x00489180) */
    if (S32(st, G_TICK_DAY) != 0 && S32(st, G_TICK_DAY) != 7) {
        return 0;
    }
    int32_t kind = -1;
    if ((uint8_t) GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_IS_TYPE_0X487F80, id) != 0) {
        switch (S16(st, BUILDING_ADDR(id) + BLD_TYPE)) {
        case 0x64: kind = 0; break;
        case 0x65: kind = 1; break;
        case 0x66: kind = 2; break;
        case 0x67: kind = 3; break;
        case 0x68: kind = 4; break;
        case 0x69: kind = 5; break;
        case 0xC4: kind = 7; break;
        case 0xC3: kind = 8; break;
        case 0xE0: kind = 9; break;
        default: break;
        }
    }
    uint8_t flood = (uint8_t) GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_AREA_TEST_3X3, id);
    if (flood) {
        if (S32(st, G_FLOOD_STATE) != 1) {
            return 0;
        }
    } else if (U8(st, G_BUILDING_MANAGER + (uint32_t) kind) == 0 || S32(st, G_TICK_DAY) != 0) {
        return 0;
    }
    int32_t amount = GAME_CALLT(st, G_BUILDING_MANAGER, FN_FARM_GROWTH_PCT, id) << 3;
    building_production_take(st, id);
    GAME_CALLT(st, G_BUILDING_MANAGER, FN_FARM_REDRAW_488E60, id, 0);
    uint8_t c = (uint8_t) GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_ROAD_FLAG_4890F0, id);
    GAME_CALLT(st, G_BUILDING_MANAGER, FN_FARM_FERTILITY_CUT, id, c != 0 ? 0x32 : 0x50);
    if (!flood) {
        return amount;
    }
    if (S32(st, CURRENT_CITY(st) + CITY_U4994) != 0) {
        amount += amount;
        U8(st, G_HARVEST_DOUBLED) = 1;
    }
    uint32_t b = BUILDING_ADDR(id);
    U8(st, b + BLD_UA7) = 0;
    S16(st, b + BLD_WORKERS) = 0;
    int16_t f = S16(st, b + BLD_SECOND_WALKER);
    if (f != 0) {
        uint32_t a = FIGURE_ADDR(f);
        uint8_t act = U8(st, a + FIG_ACTION);
        if (U8(st, a + FIG_STATE) != 2 && U8(st, a + FIG_TYPE) == 0x55 && act != 6 && act != 7) {
            GAME_CALLT(st, G_UD05720, FN_LABOR_ASSIGN, f);
        }
        S16(st, b + BLD_SECOND_WALKER) = 0;
    }
    return amount;
}

int32_t div_round_up(int32_t a, int32_t b)
{
    /* @bin 0x004CDE80 a / b rounded away from zero on a remainder (signed division); 0 when b is 0 */
    if (b == 0) {
        return 0;
    }
    int32_t q = a / b;
    return a % b != 0 ? q + 1 : q;
}

int32_t recruit_bonus(game_state *st, int32_t target)
{
    /* @bin 0x004C4160 25 when there is a target building, plus 10 in a city condition (+0x4478,
     * 0x004C5120 == 3) */
    int32_t v = target != 0 ? 0x19 : 0;
    uint32_t c = CURRENT_CITY(st);
    if (S16(st, c + CITY_U4478) != 0 && GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, S32(st, c + CITY_U472C)) == 3) {
        return v + 10;
    }
    return v;
}

int32_t formation_nearest_trainer(game_state *st, int32_t formation)
{
    /* @bin 0x004B75C0 the building of type 0x5E, 0xB9 or 0xBA with all its workers nearest to the
     * formation's fort (+0x06) */
    int32_t fort = S16(st, FORMATION_ADDR(formation) + FORM_FORT);
    int32_t best = 0, lowest = 10000;
    for (int32_t i = 1; i < BUILDING_COUNT; i++) {
        uint32_t b = BUILDING_ADDR(i);
        int16_t t = S16(st, b + BLD_TYPE);
        if (U8(st, b + BLD_STATE) != 1 || (t != 0x5E && t != 0xB9 && t != 0xBA)) {
            continue;
        }
        if (S16(st, b + BLD_WORKERS) < S32(st, G_TYPE_WORKERS + 0x28u * (uint32_t) (int32_t) t)) {
            continue;
        }
        uint32_t f = BUILDING_ADDR(fort);
        int32_t fx = S16(st, f + BLD_X), fy = S16(st, f + BLD_Y), bx = S16(st, b + BLD_X), by = S16(st, b + BLD_Y);
        int32_t dx = fx > bx ? fx - bx : fx < bx ? bx - fx : 0;
        S32(st, G_DIST_X) = dx;
        int32_t dy = fy > by ? fy - by : fy < by ? by - fy : 0;
        S32(st, G_DIST_Y) = dy;
        int32_t d = dx < dy ? dy : dx;
        if (d < lowest) {
            best = i;
            lowest = d;
        }
    }
    return best;
}

int32_t building_recruit(game_state *st, int32_t id)
{
    /* @bin 0x004B70C0 a recruit (the formation's figure type) for the player formation (+0x42, not
     * +0x82 >= 0, no +0x84) that wants one most (+0x46; 3 needs stock at +0xCC, 2 at +0x3C; then the
     * nearest fort +0x06); it goes to the nearest trainer (0x004B75C0); the formation's training
     * +0x7A takes the recruit's (0x004C4160) into its average; 1 when a recruit was sent */
    uint32_t b = BUILDING_ADDR(id);
    int low3c = S16(st, b + BLD_STORED) <= 0, lowcc = S16(st, b + BLD_STOCK2) <= 0;
    int32_t best = 0, best_want = 0, lowest = 10000;
    for (int32_t i = 1; i < 50; i++) {
        uint32_t f = FORMATION_ADDR(i);
        if (U8(st, f + FORM_IN_USE) != 1 || U8(st, f + FORM_PLAYER) == 0 || S16(st, f + FORM_U82) >= 0) {
            continue;
        }
        int16_t want = S16(st, f + FORM_WANT);
        if (want <= 0 || S32(st, f + FORM_U84) != 0 || (want == 3 && lowcc) || (want == 2 && low3c)) {
            continue;
        }
        uint32_t fort = BUILDING_ADDR(S16(st, f + FORM_FORT));
        int32_t bx = S16(st, b + BLD_X), by = S16(st, b + BLD_Y), fx = S16(st, fort + BLD_X), fy = S16(st, fort + BLD_Y);
        int32_t dx = bx > fx ? bx - fx : bx < fx ? fx - bx : 0;
        S32(st, G_DIST_X) = dx;
        int32_t dy = by > fy ? by - fy : by < fy ? fy - by : 0;
        S32(st, G_DIST_Y) = dy;
        int32_t d = dx < dy ? dy : dx;
        if (want > best_want || (want == best_want && d < lowest)) {
            best = i;
            best_want = want;
            lowest = d;
        }
    }
    int32_t target = 0;
    if (best > 0) {
        uint32_t fm = FORMATION_ADDR(best);
        int16_t type = S16(st, fm + FORM_FIGURE_TYPE);
        int32_t n = figure_create(st, U8(st, b + BLD_OWNER), type, S32(st, G_ROAD_ACCESS_X), S32(st, G_ROAD_ACCESS_Y), 0);
        uint32_t a = FIGURE_ADDR(n);
        S16(st, a + FIG_FORMATION) = (int16_t) best;
        U8(st, a + FIG_U63) = 1;
        if (type == 0xD) {
            if (S16(st, b + BLD_STOCK2) > 0) {
                S16(st, b + BLD_STOCK2) = (int16_t) (S16(st, b + BLD_STOCK2) - 100);
            }
        } else if (type == 0xC) {
            if (S16(st, b + BLD_STORED) > 0) {
                S16(st, b + BLD_STORED) = (int16_t) (S16(st, b + BLD_STORED) - 100);
            }
        }
        target = formation_nearest_trainer(st, best);
        if (target == 0) {
            U8(st, a + FIG_ACTION) = 0x16;
        } else {
            uint32_t t = BUILDING_ADDR(target);
            if (GAME_CALL(st, FN_BUILDING_ROAD_ACCESS, S16(st, t + BLD_X), S16(st, t + BLD_Y), U8(st, t + BLD_SIZE)) == 0) {
                U8(st, a + FIG_ACTION) = 9;
            } else {
                int16_t y = S16(st, G_ROAD_ACCESS_Y);
                U8(st, a + FIG_TERRAIN_USAGE) = 10;
                U8(st, a + FIG_ACTION) = 0xD;
                S16(st, a + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
                S16(st, a + FIG_DEST_Y) = y;
                S32(st, a + FIG_U28) = S32(st, G_MAP_START) + S16(st, a + FIG_DEST_X) + y * GRID_SIZE;
            }
        }
    }
    formations_rebuild(st);
    int32_t bonus = recruit_bonus(st, target);
    if (best == 0) {
        return 0;
    }
    /* @bin 0x004B7353 the formation's training (+0x7A) averaged with the recruit's */
    uint32_t fm = FORMATION_ADDR(best);
    int32_t m = S16(st, fm + FORM_TRAINING);
    int32_t n1 = (int32_t) U8(st, fm + FORM_COUNT) - 1;
    if (m < 0x19 && bonus == 0x19) {
        S16(st, fm + FORM_TRAINING) = (int16_t) div_round_up(n1 * m + 0x19, n1 + 1);
        return 1;
    }
    int32_t num = (2 * n1 + 1) * m + bonus, den = 2 * n1 + 2;
    S16(st, fm + FORM_TRAINING) = (int16_t) (div_round_up(num, den) < 100 ? div_round_up(num, den) : 100);
    return 1;
}

static int32_t run_div_round_up(game_state *st, const int32_t *a)
{
    (void) st;
    return div_round_up(a[0], a[1]);
}

static int32_t run_recruit_bonus(game_state *st, const int32_t *a)
{
    return recruit_bonus(st, a[0]);
}

static int32_t run_nearest_trainer(game_state *st, const int32_t *a)
{
    return formation_nearest_trainer(st, a[0]);
}

static int32_t run_recruit(game_state *st, const int32_t *a)
{
    return building_recruit(st, a[0]);
}

static int32_t run_send_0x2a(game_state *st, const int32_t *a)
{
    return building_send_0x2a(st, a[0]);
}

static int32_t run_0xc7(game_state *st, const int32_t *a)
{
    building_update_0xc7(st, a[0]);
    return 0;
}

static int32_t run_harvest(game_state *st, const int32_t *a)
{
    return farm_harvest(st, a[0]);
}

const ported_function bigb_extra_ports[] = {
    {0x004B7400u, "building_send_0x2a", 1, 32, 0, run_send_0x2a, 0},
    {0x004BED70u, "building_update_0xc7", 1, 0, G_UD05720, run_0xc7, 0},
    {0x00488400u, "farm_harvest", 1, 32, G_BUILDING_MANAGER, run_harvest, 0},
    {0x004CDE80u, "div_round_up", 2, 32, 0, run_div_round_up, 0},
    {0x004C4160u, "recruit_bonus", 1, 32, 0, run_recruit_bonus, 0},
    {0x004B75C0u, "formation_nearest_trainer", 1, 32, 0, run_nearest_trainer, 0},
    {0x004B70C0u, "building_recruit", 1, 32, 0, run_recruit, 0},
};
const int bigb_extra_ports_count = sizeof(bigb_extra_ports) / sizeof(bigb_extra_ports[0]);
