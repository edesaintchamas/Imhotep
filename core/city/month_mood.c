/* Imhotep - the month's mood of the houses and of the city (0x004E81D0 and its helpers).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The x87 code of these functions is reproduced with integers: each case states why the integer
 * result equals the original's (truncating __ftol 0x0056133C). */
#include "city/month_mood.h"

#include "random/random.h"
#include "sim/call.h"
#include "sim/math.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

#define CITY(st) CURRENT_CITY(st)

static int search_tile(game_state *st, const int32_t *x, const int32_t *y, int32_t ntypes, uint32_t types,
                       uint32_t mask, int32_t except, int32_t *found)
{
    /* @bin 0x004EB85E one tile of the ring: inside the map, a figure of one of the types (figure
     * +0x0A, the chain +0x08 from the tile's first figure) other than `except` -> its id; else
     * terrain with a bit of mask and not 0x80000 -> ntypes + 1 */
    if (*x < 0 || *y < 0 || *x >= S32(st, G_MAP_WIDTH) || *y >= S32(st, G_MAP_HEIGHT)) {
        return 0;
    }
    for (int32_t k = 0; k < ntypes; k++) {
        int32_t f = S16(st, G_FIGURE_GRID + 2u * U32(st, G_TILE_CURSOR));
        uint32_t want = U32(st, types + 4u * (uint32_t) k);
        while (f != 0) {
            if ((uint32_t) U8(st, FIGURE_ADDR(f) + FIG_TYPE) == want && except != f) {
                *found = f;
                return 1;
            }
            f = S16(st, FIGURE_ADDR(f) + FIG_NEXT_ON_TILE);
        }
    }
    uint32_t t = U32(st, G_GRID_TERRAIN + 4u * U32(st, G_TILE_CURSOR));
    if (!(t & 0x80000u) && (mask & t)) {
        *found = ntypes + 1;
        return 1;
    }
    return 0;
}

int32_t map_search_rings(game_state *st, int32_t x0, int32_t y0, int32_t ntypes, uint32_t types, uint32_t mask,
                         int32_t *x, int32_t *y, int32_t radius, int32_t except)
{
    /* @bin 0x004EB800 square rings around (x0, y0), side 1, 3, 5, ...: each ring's four edges in
     * the order +x, +y, -x, -y from its corner; *x, *y follow the tile looked at. At most
     * `radius` + 1 rings (radius 0: one), and at most 0xE4 */
    uint32_t base = U32(st, G_MAP_START) + 0xE4u * (uint32_t) y0 + (uint32_t) x0;
    int32_t side = 1, found;
    *x = x0;
    *y = y0;
    for (int32_t ring = 0;;) {
        U32(st, G_TILE_CURSOR) = base;
        static const int dx[4] = {1, 0, -1, 0}, dy[4] = {0, 1, 0, -1};
        static const int32_t dg[4] = {1, 0xE4, -1, -0xE4};
        for (int e = 0; e < 4; e++) {
            for (int32_t i = 0; i < side; i++) {
                if (search_tile(st, x, y, ntypes, types, mask, except, &found)) {
                    return found;
                }
                U32(st, G_TILE_CURSOR) += (uint32_t) dg[e];
                *x += dx[e];
                *y += dy[e];
            }
            U32(st, G_TILE_CURSOR) -= (uint32_t) dg[e];
            *x -= dx[e];
            *y -= dy[e];
        }
        if (radius == 0) {
            return 0;
        }
        ring++;
        side += 2;
        base -= 0xE5u;
        *x -= 1;
        *y -= 1;
        radius--;
        if (ring >= 0xE4) {
            return 0;
        }
    }
}

static int32_t isqrt_ftol(int32_t d)
{
    /* @bin 0x004E809F fild, fsqrt, __ftol: the square root of an integer below 2^31 is either an
     * integer or farther than 2^-33 from one, so the rounded root truncates to floor(sqrt(d)) */
    if (d < 0) {
        return INT32_MIN; /* fsqrt of a negative: the indefinite integer (distances are never negative) */
    }
    uint32_t r = 0;
    for (uint32_t bit = 1u << 30; bit != 0; bit >>= 2) {
        if ((uint32_t) d >= r + bit) {
            d -= (int32_t) (r + bit);
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
    }
    return (int32_t) r;
}

void house_mood_month(game_state *st, int32_t id, int32_t recompute)
{
    /* @bin 0x004E7F00 one game RNG step; +0xFC by the level's 0x0070CE20 and the city's +0x18;
     * the distances +0xF8, +0xF4 to the nearest terrain 0x40000 and 4 (recomputed when the byte
     * argument is set); +0xE8 by the level's 0x0070CE1C, +0x18 and those distances */
    random_next_state(st);
    uint32_t b = BUILDING_ADDR(id);
    int32_t level = S16(st, b + BLD_SUBTYPE);
    int32_t mood = S32(st, CITY(st) + CITY_HEALTH);
    int32_t v = (int32_t) ((uint32_t) S32(st, b + BLD_DISEASE_RISK) +
                           (uint32_t) S32(st, G_HOUSE_LEVEL_U54 + 0x58u * (uint32_t) level) +
                           (uint32_t) (0x32 - mood) * 2u);
    S32(st, b + BLD_DISEASE_RISK) = v;
    if (v < 0) {
        S32(st, b + BLD_DISEASE_RISK) = 0;
    }
    if (S32(st, b + BLD_DISEASE_RISK) > 1000) {
        S32(st, b + BLD_DISEASE_RISK) = 1000;
    }
    int32_t far, near;
    if ((uint8_t) recompute == 0) {
        far = S32(st, b + BLD_DIST_TERRAIN_40000);
        near = S32(st, b + BLD_DIST_TERRAIN_4);
    } else {
        int32_t lim = S32(st, G_MAP_WIDTH);
        if (S32(st, G_MAP_HEIGHT) > lim) {
            lim = S32(st, G_MAP_HEIGHT);
        }
        int32_t hx = S16(st, b + BLD_X), hy = S16(st, b + BLD_Y), fx, fy;
        far = lim;
        if (map_search_rings(st, hx, hy, 0, 0, 0x40000, &fx, &fy, lim, 0) != 0) {
            far = isqrt_ftol((hy - fy) * (hy - fy) + (hx - fx) * (hx - fx));
        }
        near = lim;
        if (map_search_rings(st, hx, hy, 0, 0, 4, &fx, &fy, lim, 0) != 0) {
            near = isqrt_ftol((hy - fy) * (hy - fy) + (hx - fx) * (hx - fx));
        }
        S32(st, b + BLD_DIST_TERRAIN_4) = near;
        S32(st, b + BLD_DIST_TERRAIN_40000) = far;
    }
    /* @bin 0x004E813F by the setting 0x00784CB0: 0 -> near := 1; 2 -> half, +8 instead of +16 */
    int32_t div = 1, add = 0x10;
    if (U8(st, G_U784CB0) == 0) {
        near = 1;
    } else if (U8(st, G_U784CB0) == 2) {
        add = 8;
        div = 2;
    }
    int32_t d = (int32_t) ((uint32_t) S32(st, G_HOUSE_LEVEL_U50 + 0x58u * (uint32_t) level) - (uint32_t) S32(st, CITY(st) + CITY_HEALTH) +
                           (uint32_t) (0x37 - far) * 2u - (uint32_t) near + (uint32_t) add) /
                div;
    v = (int32_t) ((uint32_t) S32(st, b + BLD_MALARIA_RISK) + (uint32_t) d);
    S32(st, b + BLD_MALARIA_RISK) = v;
    if (v < 0) {
        S32(st, b + BLD_MALARIA_RISK) = 0;
    }
    if (S32(st, b + BLD_MALARIA_RISK) > 1000) {
        S32(st, b + BLD_MALARIA_RISK) = 1000;
    }
}


void people_remove(game_state *st, int32_t player, int32_t n)
{
    /* @bin 0x004E9350 n people taken from the ages: first at the ages the RNG pool gives (from its
     * index, & 0x3F), then from age 10 on, each pass giving up after 100 empty ages in a row */
    uint32_t c = CITY_ADDR(player);
    int32_t misses = 0;
    if (n != 0) {
        int32_t k = S32(st, G_R_POOL_INDEX);
        do {
            uint32_t a = c + CITY_AGES + 2u * (U32(st, G_R_POOL + 4u * (uint32_t) k) & 0x3F);
            if (S16(st, a) <= 0) {
                misses++;
            } else {
                S16(st, a) -= 1;
                n--;
                misses = 0;
            }
            if (++k >= 100) {
                k = 0;
            }
        } while (misses < 100 && n != 0);
    }
    misses = 0;
    int32_t age = 10;
    if (n != 0) {
        do {
            uint32_t a = c + CITY_AGES + 2u * (uint32_t) age;
            if (S16(st, a) <= 0) {
                misses++;
            } else {
                S16(st, a) -= 1;
                n--;
                misses = 0;
            }
            if (++age >= 100) {
                age = 0;
            }
        } while (misses < 100 && n != 0);
    }
}

void people_add(game_state *st, int32_t player, int32_t n)
{
    /* @bin 0x004E9400 n people added at the ages the RNG pool gives (& 0x3F): above 50, 30 less;
     * below 10 every other one, 20 more */
    uint32_t c = CITY_ADDR(player);
    if (n == 0) {
        return;
    }
    int32_t k = S32(st, G_R_POOL_INDEX);
    int odd = 0;
    uint32_t left = (uint32_t) n;
    do {
        int32_t age = (int32_t) (U32(st, G_R_POOL + 4u * (uint32_t) k) & 0x3F);
        if (age > 0x32) {
            age -= 0x1E;
        } else if (age < 0xA && odd) {
            age += 0x14;
        }
        S16(st, c + CITY_AGES + 2u * (uint32_t) age) += 1;
        if (++k >= 100) {
            k = 0;
        }
        odd = !odd;
    } while (--left != 0);
}

void people_change(game_state *st, int32_t player, int32_t delta)
{
    /* @bin 0x004E90F0 +0x26FC := delta; the ages; +0x24 := their sum, +0x469C its maximum */
    uint32_t c = CITY_ADDR(player);
    S32(st, c + CITY_U26FC) = delta;
    if (delta < 0) {
        people_remove(st, player, (int32_t) (0u - (uint32_t) delta));
    } else if (delta > 0) {
        people_add(st, player, delta);
    }
    int32_t sum = 0;
    for (uint32_t i = 0; i < 100; i++) {
        sum += S16(st, c + CITY_AGES + 2 * i);
    }
    S32(st, c + CITY_POPULATION) = sum;
    if (sum > S32(st, c + CITY_POPULATION_HIGHEST)) {
        S32(st, c + CITY_POPULATION_HIGHEST) = sum;
    }
}

void house_empty(game_state *st, int32_t id)
{
    /* @bin 0x0042FB30 the house's people (+0x1C) leave the city; +0x87 := 1, +0xF0 := 2, +0xFC,
     * +0xE8 := 0; the tutorial step 0x00417AE0(6) in mission 0x00579558 */
    uint32_t b = BUILDING_ADDR(id);
    people_change(st, S32(st, CURRENT_PLAYER_ADDR), -(int32_t) S16(st, b + BLD_POPULATION));
    S16(st, b + BLD_POPULATION) = 0;
    U8(st, b + BLD_NO_ROOM) = 1;
    S32(st, b + BLD_UF0) = 2;
    S32(st, b + BLD_DISEASE_RISK) = 0;
    S32(st, b + BLD_MALARIA_RISK) = 0;
    if (S32(st, G_CUSTOM) == 0 && S8(st, G_MISSION) == S32(st, G_MISSION_LISTS + G_MISSION_LIST_STRIDE * 6) && U8(st, G_MISSION_LIST_FLAGS + 6) == 0) {
        GAME_CALL(st, FN_TUTORIAL_STEP, 6);
    }
}

void house_leave_0x430040(game_state *st, int32_t id)
{
    /* @bin 0x00430040 */
    house_empty(st, id);
    S32(st, G_HOUSE_MESSAGE_146) = id;
    S32(st, G_HOUSE_MESSAGE_146_ON) = 1;
}

int32_t house_leave_0x430060(game_state *st, int32_t id)
{
    /* @bin 0x00430060 the house empties; within (100 - mood) / 20 tiles (one more in months 9..11,
     * at most 4), the houses with people get +0xEC := their distance (the larger of dx, dy) */
    house_empty(st, id);
    S32(st, G_HOUSE_MESSAGE_148_ON) = 1;
    S32(st, G_HOUSE_MESSAGE_148) = id;
    int32_t r = (100 - S32(st, CITY(st) + CITY_HEALTH)) / 0x14;
    int32_t m = S32(st, G_MONTH);
    if (m == 9 || m == 0xA || m == 0xB) {
        r++;
    }
    if (r <= 0) {
        return 0;
    }
    if (r > 4) {
        r = 4;
    }
    uint32_t b = BUILDING_ADDR(id);
    int32_t hx = S16(st, b + BLD_X), hy = S16(st, b + BLD_Y);
    int32_t x1 = hx + r, x0 = hx - r, y1 = hy + r, y0 = hy - r;
    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    if (x1 >= S32(st, G_MAP_WIDTH)) {
        x1 = S32(st, G_MAP_WIDTH) - 1;
    }
    if (y1 >= S32(st, G_MAP_HEIGHT)) {
        y1 = S32(st, G_MAP_HEIGHT) - 1;
    }
    uint32_t g = U32(st, G_MAP_START) + 0xE4u * (uint32_t) y0 + (uint32_t) x0;
    for (int32_t y = y0; y <= y1; y++) {
        if (x0 <= x1) {
            for (int32_t x = x0; x <= x1; x++) {
                int32_t o = S16(st, G_BUILDING_GRID + 2u * (g + (uint32_t) (x - x0)));
                uint32_t ob = BUILDING_ADDR(o);
                if (o == 0 || U8(st, ob + BLD_STATE) == 0 || U8(st, ob + BLD_HOUSE_SIZE) == 0 || S16(st, ob + BLD_POPULATION) == 0) {
                    continue;
                }
                int32_t dx = x - hx > hx - x ? x - hx : hx - x;
                int32_t dy = y - hy > hy - y ? y - hy : hy - y;
                S32(st, ob + BLD_UEC) = dx > dy ? dx : dy;
            }
            g += (uint32_t) (x1 - x0) + 1;
        }
        g += (uint32_t) (x0 - x1 + 0xE3);
    }
    return 1;
}

void houses_leave_countdown(game_state *st)
{
    /* @bin 0x00430250 houses with +0xEC set count it down; at 0 they empty */
    for (int32_t id = 1; id <= S32(st, G_BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) == 0 || U8(st, b + BLD_HOUSE_SIZE) == 0 || S16(st, b + BLD_POPULATION) == 0 || S32(st, b + BLD_UEC) == 0) {
            continue;
        }
        S32(st, b + BLD_UEC) -= 1;
        if (S32(st, b + BLD_UEC) == 0) {
            house_empty(st, id);
        }
    }
}

void houses_leave_messages(game_state *st)
{
    /* @bin 0x004302B0 the three pending messages, with the house's type (+0x10) and +0x0C */
    static const struct {
        uint32_t flag, house;
        int32_t msg;
    } m[3] = {{G_HOUSE_MESSAGE_147_ON, G_HOUSE_MESSAGE_147, 0x147}, {G_HOUSE_MESSAGE_146_ON, G_HOUSE_MESSAGE_146, 0x146}, {G_HOUSE_MESSAGE_148_ON, G_HOUSE_MESSAGE_148, 0x148}};
    for (int k = 0; k < 3; k++) {
        if (S32(st, m[k].flag) == 0) {
            continue;
        }
        uint32_t b = BUILDING_ADDR(S32(st, m[k].house));
        S32(st, G_MESSAGE_POPUP) = 1;
        GAME_CALL(st, FN_MESSAGE_POST, m[k].msg, S16(st, b + BLD_TYPE), S32(st, b + BLD_GRID_OFFSET), 0, 0);
        S32(st, m[k].flag) = 0;
    }
}

int32_t city_riot_roll(game_state *st)
{
    /* @bin 0x0042FBB0 one game RNG step: r1_15bit % 100 < 40 - mood (+0x18) */
    random_next_state(st);
    return (int32_t) U32(st, G_R_1_15) % 100 < 0x28 - S32(st, CITY(st) + CITY_HEALTH);
}

int32_t house_most_0xfc(game_state *st)
{
    /* @bin 0x0042FCA0 the house (state set, +5, people) with the highest +0xFC above 0, else 0 */
    int32_t best = 0, id_best = 0;
    for (int32_t id = 1; id <= S32(st, G_BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) == 0 || S16(st, b + BLD_POPULATION) == 0 || U8(st, b + BLD_HOUSE_SIZE) == 0) {
            continue;
        }
        if (best < S32(st, b + BLD_DISEASE_RISK)) {
            best = S32(st, b + BLD_DISEASE_RISK);
            id_best = id;
        }
    }
    return id_best;
}

int32_t house_riot_start(game_state *st, int32_t id)
{
    /* @bin 0x0042FE90 the house empties, city +0x4980 + 1, a figure 0x62 at the house (+0x36 := 500,
     * +0x5A := the house, +0x38 := 8) started by 0x004B1DF0 */
    if (id == 0) {
        return 0;
    }
    house_empty(st, id);
    S32(st, CITY(st) + CITY_U4980) += 1;
    uint32_t b = BUILDING_ADDR(id);
    int32_t f = GAME_CALL(st, FN_FIGURE_CREATE, S32(st, CURRENT_PLAYER_ADDR), 0x62, S16(st, b + BLD_X), S16(st, b + BLD_Y), 0);
    if (f != 0) {
        uint32_t fa = FIGURE_ADDR(f);
        S16(st, fa + FIG_WAIT_TICKS) = 500;
        S16(st, fa + FIG_HOME) = (int16_t) id;
        U8(st, fa + FIG_ACTION) = 8;
        GAME_CALL(st, FN_FIGURE_WALKER_START, f);
    }
    return 1;
}

int32_t city_riot(game_state *st, int32_t any)
{
    /* @bin 0x0042FCF0 the house with the highest +0xFC; none and `any` set: one house with people,
     * chosen by the game RNG (each one replaces the choice when r1_15bit % its rank is 0) */
    int32_t id = house_most_0xfc(st);
    if ((int8_t) any != 0 && id == 0) {
        int32_t rank = 0;
        for (int32_t k = 1; k <= S32(st, G_BUILDING_HIGHEST); k++) {
            uint32_t b = BUILDING_ADDR(k);
            if (U8(st, b + BLD_STATE) == 0 || S16(st, b + BLD_POPULATION) == 0 || U8(st, b + BLD_HOUSE_SIZE) == 0) {
                continue;
            }
            rank++;
            random_next_state(st);
            if ((int32_t) U32(st, G_R_1_15) % rank == 0) {
                id = k;
            }
        }
    }
    if (!house_riot_start(st, id)) {
        return 0;
    }
    S32(st, G_HOUSE_MESSAGE_147) = id;
    S32(st, G_HOUSE_MESSAGE_147_ON) = 1;
    return 1;
}

static int32_t house_score(game_state *st, uint32_t b)
{
    /* @bin 0x004E82A3 +0x78: 25, +0x6E: 15, +0x7F: 10 and 5 more above 1 and above 2, +0x7F with
     * +0x6E: 40 */
    int32_t s = U8(st, b + BLD_ACCESS_EMBALMER) != 0 ? 0x19 : 0;
    uint8_t a = U8(st, b + BLD_ACCESS_PHYSICIAN), c = U8(st, b + BLD_U7F);
    if (a != 0) {
        s += 0xF;
    }
    if (c != 0) {
        s += 0xA;
    }
    if (c > 1) {
        s += 5;
    }
    if (c > 2) {
        s += 5;
    }
    if (c != 0 && a != 0) {
        s += 0x28;
    }
    return s;
}

static int32_t score_ftol(int32_t score, int16_t pop)
{
    /* @bin 0x004E82F5 __ftol(score * 0.01f * pop): 0.01f is 0xA3D70A * 2^-30 (0x0056F7D4); score
     * (< 2^7) times the 24-bit mantissa, then times pop (< 2^16), stay below 2^53: both products are
     * exact in the x87 registers, the truncation is that of the exact quotient */
    int64_t n = (int64_t) score * 0xA3D70A * pop;
    return (int32_t) (n / ((int64_t) 1 << 30));
}

void city_mood_month(game_state *st)
{
    /* @bin 0x004E81D0 */
    uint32_t c = CITY(st);
    int8_t mode = S8(st, G_MISSION);
    int32_t weighted = 0, pop = 0;
    S32(st, c + CITY_HOUSES_DISEASED) = 0;
    int fixed = 0;
    if (mode < 2 && S32(st, G_CUSTOM) == 0) {
        /* @bin 0x004E8201 the first missions: 75 */
        fixed = 1;
        S32(st, c + CITY_HEALTH) = 0x4B;
        S32(st, c + CITY_HEALTH_TARGET) = 0x4B;
    } else {
        int32_t id = 1;
        for (uint32_t b = BUILDING_ADDR(1); b + BLD_HOUSE_SIZE < BUILDING_ADDR(BUILDING_COUNT) + BLD_HOUSE_SIZE; b += BUILDING_REC_SIZE, id++) {
            if (U8(st, b + BLD_STATE) != 1 || U8(st, b + BLD_HOUSE_SIZE) == 0 || S16(st, b + BLD_POPULATION) == 0) {
                continue;
            }
            pop += S16(st, b + BLD_POPULATION);
            house_mood_month(st, id, U8(st, G_UEAAC2C));
            if (S32(st, b + BLD_MALARIA_RISK) == 1000) {
                house_leave_0x430060(st, id);
                S32(st, CITY(st) + CITY_HOUSES_DISEASED) += 1;
            } else if (S32(st, b + BLD_DISEASE_RISK) == 1000) {
                house_leave_0x430040(st, id);
            }
            weighted += score_ftol(house_score(st, b), S16(st, b + BLD_POPULATION));
        }
        U8(st, G_UEAAC2C) = 0;
        int32_t share = GAME_CALL(st, FN_CALC_RATIO_PERCENT, weighted, pop);
        c = CITY(st);
        S32(st, c + CITY_HEALTH_TARGET) = share;
        /* @bin 0x004E8350 by population (+0x24), the table of 10 (limit, bonus) at 0x005E4B8C */
        int32_t bonus = 0;
        for (uint32_t k = 0; k < 10; k++) {
            if (S32(st, c + CITY_POPULATION) < S32(st, G_U5E4B8C + 8 * k)) {
                bonus = S32(st, G_U5E4B8C + 4 + 8 * k);
                break;
            }
        }
        S32(st, c + CITY_HEALTH_TARGET) = share + bonus;
        if (S16(st, c + CITY_U4478) != 0 && GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, S32(st, c + CITY_U472C)) == 4 &&
            (int8_t) GAME_CALL(st, FN_BUILDING_0XA9_IS, S32(st, CITY(st) + CITY_U472C), 1) != 0) {
            c = CITY(st);
            S32(st, c + CITY_HEALTH_TARGET) = calc_percentage(S32(st, c + CITY_HEALTH_TARGET), 0x69);
        }
        if (city_riot_roll(st)) {
            city_riot(st, 0);
        }
        /* @bin 0x004E83F7 +0x18 moves 2 towards +0x14; a message when it crosses a ten */
        c = CITY(st);
        int32_t old = S32(st, c + CITY_HEALTH), msg = 0;
        if (old < S32(st, c + CITY_HEALTH_TARGET)) {
            S32(st, c + CITY_HEALTH) = old + 2;
            if (old + 2 > S32(st, c + CITY_HEALTH_TARGET)) {
                S32(st, c + CITY_HEALTH) = S32(st, c + CITY_HEALTH_TARGET);
            }
            int32_t now = S32(st, c + CITY_HEALTH);
            if (old / 10 != now / 10 && S32(st, c + CITY_POPULATION) > 0x12C && now >= 10) {
                msg = now < 0x14 ? 0x63 : now < 0x1E ? 0x64 : now < 0x28 ? 0x65 : 0;
            }
        } else if (old > S32(st, c + CITY_HEALTH_TARGET)) {
            S32(st, c + CITY_HEALTH) = old - 2;
            if (old - 2 < S32(st, c + CITY_HEALTH_TARGET)) {
                S32(st, c + CITY_HEALTH) = S32(st, c + CITY_HEALTH_TARGET);
            }
            int32_t now = S32(st, c + CITY_HEALTH);
            if ((old / 10 != now / 10 && S32(st, c + CITY_POPULATION) > 0x12C) || now < 0x14) {
                msg = now < 10 ? 0x5F : now < 0x14 ? 0x60 : now < 0x1E ? 0x61 : now < 0x28 ? 0x62 : 0;
            }
        }
        if (msg != 0) {
            GAME_CALL(st, FN_NOTICE_ADD_4E2970, msg, 0, 0);
        }
        c = CITY(st);
        if (S32(st, c + CITY_HEALTH) > 100) {
            S32(st, c + CITY_HEALTH) = 100;
        }
        if (S32(st, c + CITY_HEALTH) < 0) {
            S32(st, c + CITY_HEALTH) = 0;
        }
    }
    /* @bin 0x004E857A buildings in use with +5 and +0x87 set: +0xF0 counts down, +0x87 := 0 at 0
     * (the loop runs to 0x0089B7C8: ids 1..4003) */
    for (uint32_t p = BUILDING_ADDR(1) + BLD_UF0; p < BUILDING_ADDR(BUILDING_COUNT) + BLD_UF0; p += BUILDING_REC_SIZE) {
        if (U8(st, p - BLD_UF0 + BLD_STATE) != 1 || U8(st, p - BLD_UF0 + BLD_HOUSE_SIZE) == 0 || U8(st, p - BLD_UF0 + BLD_NO_ROOM) == 0 || S32(st, p) == 0) {
            continue;
        }
        S32(st, p) -= 1;
        if (S32(st, p) == 0) {
            U8(st, p - BLD_UF0 + BLD_NO_ROOM) = 0;
        }
    }
    if (!fixed) {
        houses_leave_countdown(st);
        houses_leave_messages(st);
    }
}

static int32_t run_search(game_state *st, const int32_t *a)
{
    uint32_t px = (uint32_t) a[5], py = (uint32_t) a[6];
    if (!state_ptr(st, px, 4) || !state_ptr(st, py, 4) || (a[2] > 0 && !state_ptr(st, (uint32_t) a[3], 4))) {
        state_unported(st, FN_MAP_SEARCH_RINGS); /* outputs in the caller's stack */
        return 0;
    }
    int32_t x, y;
    int32_t r = map_search_rings(st, a[0], a[1], a[2], (uint32_t) a[3], (uint32_t) a[4], &x, &y, a[7], a[8]);
    S32(st, px) = x;
    S32(st, py) = y;
    return r;
}

static int32_t run_house_mood(game_state *st, const int32_t *a)
{
    house_mood_month(st, a[0], a[1]);
    return 0;
}

static int32_t run_city_mood(game_state *st, const int32_t *a)
{
    (void) a;
    city_mood_month(st);
    return 0;
}

#define WRAP2(name, call) \
    static int32_t name(game_state *st, const int32_t *a) { call; return 0; }
WRAP2(run_remove, people_remove(st, a[0], a[1]))
WRAP2(run_add, people_add(st, a[0], a[1]))
WRAP2(run_change, people_change(st, a[0], a[1]))
WRAP2(run_empty, house_empty(st, a[0]))
WRAP2(run_430040, house_leave_0x430040(st, a[0]))
WRAP2(run_430250, ((void) a, houses_leave_countdown(st)))
WRAP2(run_4302b0, ((void) a, houses_leave_messages(st)))

static int32_t run_430060(game_state *st, const int32_t *a)
{
    return house_leave_0x430060(st, a[0]);
}

static int32_t run_roll(game_state *st, const int32_t *a)
{
    (void) a;
    return city_riot_roll(st);
}

static int32_t run_most(game_state *st, const int32_t *a)
{
    (void) a;
    return house_most_0xfc(st);
}

static int32_t run_riot_start(game_state *st, const int32_t *a)
{
    return house_riot_start(st, a[0]);
}

static int32_t run_riot(game_state *st, const int32_t *a)
{
    return city_riot(st, a[0]);
}

const ported_function month_mood_ports[] = {
    {0x004E9350u, "people_remove", 2, 0, 0, run_remove, 0},
    {0x004E9400u, "people_add", 2, 0, 0, run_add, 0},
    {0x004E90F0u, "people_change", 2, 0, 0, run_change, 0},
    {0x0042FB30u, "house_empty", 1, 0, 0, run_empty, 0},
    {0x00430040u, "house_leave_0x430040", 1, 0, 0, run_430040, 0},
    {0x00430060u, "house_leave_0x430060", 1, 8, 0, run_430060, 0},
    {0x00430250u, "houses_leave_countdown", 0, 0, 0, run_430250, 0},
    {0x004302B0u, "houses_leave_messages", 0, 0, 0, run_4302b0, 0},
    {0x0042FBB0u, "city_riot_roll", 0, 8, 0, run_roll, 0},
    {0x0042FCA0u, "house_most_0xfc", 0, 32, 0, run_most, 0},
    {0x0042FE90u, "house_riot_start", 1, 8, 0, run_riot_start, 0},
    {0x0042FCF0u, "city_riot", 1, 8, 0, run_riot, 0},
    {0x004EB800u, "map_search_rings", 9, 32, 0, run_search, 0},
    {0x004E7F00u, "house_mood_month", 2, 0, 0, run_house_mood, 0},
    {0x004E81D0u, "city_mood_month", 0, 0, 0, run_city_mood, 0},
};
const int month_mood_ports_count = sizeof(month_mood_ports) / sizeof(month_mood_ports[0]);
