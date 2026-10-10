/* Imhotep - the month and year ends of the city (called by the day cascade 0x00452370).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Small month / year steps: most move a running counter of the current player's city record into
 * its "last period" slot and clear it. Field meanings are not established yet; the fields are
 * named by their offset in the city record (CITY_BASE 0x00EA1870, docs/formats/save.md). */
#include "city/month.h"

#include "city/month_events.h"
#include "city/month_ratings.h"
#include "random/crt_rand.h"
#include "random/random.h"
#include "sim/math.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

#define CITY(st) CURRENT_CITY(st)

int32_t building_type_level(game_state *st, int32_t id)
{
    /* @bin 0x004C5120 movsx of the type (+0x10) */
    int32_t t = S16(st, BUILDING_ADDR(id) + BLD_TYPE);
    switch (t) {
    case 0x3C: case 0x41: return 0;
    case 0x3D: case 0x42: return 1;
    case 0x3E: case 0x43: return 2;
    case 0x3F: case 0x44: return 3;
    case 0x40: case 0x45: return 4;
    default: return -1;
    }
}

void city_add_0x42d8(game_state *st, int32_t delta)
{
    /* @bin 0x004E96C0 0x00EA5B48: negative (sign of the wrapped sum) -> 0, above 100 -> 100 */
    uint32_t p = CITY(st) + CITY_RATING_KINGDOM;
    int32_t v = (int32_t) ((uint32_t) S32(st, p) + (uint32_t) delta);
    S32(st, p) = v;
    if (v < 0) {
        S32(st, p) = 0;
    }
    if (S32(st, p) > 100) {
        S32(st, p) = 100;
    }
}

int32_t city_sum_building_1c(game_state *st, int32_t player)
{
    /* @bin 0x00458F80 over buildings 1..highest whose state (+0) is not 0, 5, 2 or 6 and whose +5
     * is set: +0x1C (i16) summed; by +0x12 (i16): <= 1 into +0x463C, <= 3 into +0x2708, >= 10
     * into +0x4640, >= 14 into +0x270C */
    uint32_t c = CITY_ADDR(player);
    S32(st, c + CITY_U2708) = 0;
    S32(st, c + CITY_U270C) = 0;
    S32(st, c + CITY_U463C) = 0;
    S32(st, c + CITY_U4640) = 0;
    int32_t total = 0;
    int32_t n = S32(st, G_BUILDING_HIGHEST);
    for (int32_t id = 1; id <= n; id++) {
        uint32_t b = BUILDING_ADDR(id);
        uint8_t s = U8(st, b + BLD_STATE);
        if (s == 0 || s == 5 || s == 2 || s == 6 || U8(st, b + BLD_HOUSE_SIZE) == 0) {
            continue;
        }
        int32_t v = S16(st, b + BLD_POPULATION);
        int16_t k = S16(st, b + BLD_SUBTYPE);
        total += v;
        if (k <= 1) {
            S32(st, c + CITY_U463C) += v;
        }
        if (k <= 3) {
            S32(st, c + CITY_U2708) += v;
        }
        if (k >= 10) {
            S32(st, c + CITY_U4640) += v;
        }
        if (k >= 14) {
            S32(st, c + CITY_U270C) += v;
        }
    }
    return total;
}

void month_0x00455530(game_state *st)
{
    /* @bin 0x00455530 the CRT rand & 7 into 0x0078D808 (a new year) */
    S32(st, G_WIND_DIRECTION) = crt_rand(st) & 7;
}

void month_0x004E8630(game_state *st)
{
    /* @bin 0x004E8630 */
    S32(st, CITY_ADDR(1) + CITY_AGEING_FLAG) = 1;
    city_sum_building_1c(st, S32(st, CURRENT_PLAYER_ADDR));
}

void month_0x004E1850(game_state *st)
{
    /* @bin 0x004E1850 the 20 counters at 0x00E95124 count down to 0 */
    for (uint32_t p = G_MESSAGE_DELAYS; p < G_MESSAGE_COUNTERS; p += 4) {
        if (S32(st, p) > 0) {
            S32(st, p) -= 1;
        }
    }
}

void month_0x004EA740(game_state *st)
{
    /* @bin 0x004EA740 city +0x44D4 counts down while +0x44D0 is set */
    uint32_t c = CITY(st);
    if (S32(st, c + CITY_U44D0) != 0) {
        S32(st, c + CITY_U44D4) -= 1;
    }
}

/* city +dst := +src, +src := 0 */
static void roll(game_state *st, uint32_t dst, uint32_t src)
{
    uint32_t c = CITY(st);
    S32(st, c + dst) = S32(st, c + src);
    S32(st, c + src) = 0;
}

void month_0x0045A4D0(game_state *st)
{
    roll(st, CITY_CONSTRUCTION_LAST_YEAR, CITY_CONSTRUCTION_THIS_YEAR); /* @bin 0x0045A4D0 */
}

void month_0x0045A500(game_state *st)
{
    roll(st, CITY_INTEREST_LAST_YEAR, CITY_U2C74); /* @bin 0x0045A500 */
}

void month_0x0045A530(game_state *st)
{
    roll(st, CITY_SALARY_LAST_YEAR, CITY_U2CA0); /* @bin 0x0045A530 */
}

void month_0x0045A5A0(game_state *st)
{
    roll(st, CITY_REQUESTS_LAST_YEAR, CITY_REQUESTS_THIS_YEAR); /* @bin 0x0045A5A0 */
}

void month_0x0045A450(game_state *st)
{
    /* @bin 0x0045A450 */
    roll(st, CITY_WAGES_LAST_YEAR, CITY_WAGES_ACCUMULATOR);
    roll(st, CITY_U45A8, CITY_U44DC);
}

void month_0x0045A490(game_state *st)
{
    /* @bin 0x0045A490 */
    roll(st, CITY_EXPORTS_LAST_YEAR, CITY_EXPORTS_THIS_YEAR);
    roll(st, CITY_IMPORTS_LAST_YEAR, CITY_IMPORTS_THIS_YEAR);
}

void month_0x0045A560(game_state *st)
{
    /* @bin 0x0045A560 two i32, two i16, then +0x4784 := 0 */
    uint32_t c = CITY(st);
    roll(st, CITY_THEFT_LAST_YEAR, CITY_THEFT_THIS_YEAR);
    S16(st, c + CITY_U448E) = S16(st, c + CITY_U448C);
    S16(st, c + CITY_U448C) = 0;
    S32(st, c + CITY_U4784) = 0;
}

void month_0x004C5300(game_state *st)
{
    /* @bin 0x004C5300 when city +0x4478 (i16) is set and building +0x472C is of level 1 */
    uint32_t c = CITY(st);
    if (S16(st, c + CITY_U4478) != 0 && building_type_level(st, S32(st, c + CITY_U472C)) == 1) {
        city_add_0x42d8(st, 1);
    }
}

void month_0x004448E0(game_state *st)
{
    /* @bin 0x004448E0 61 records of 0x6A bytes at 0x0077DE00: when +0 and +5 are set, the 0x90-byte
     * record at 0x00775930 indexed by +4 is cleared */
    for (uint32_t r = G_EMPIRE_CITIES; r + 5 < G_EMPIRE_CITIES + 0x6Au * 61 + 5; r += 0x6A) {
        if (U8(st, r) != 0 && U8(st, r + 5) != 0) {
            uint32_t d = G_U775930 + 0x90u * U8(st, r + 4);
            for (uint32_t k = 0; k < 0x90; k += 4) {
                U32(st, d + k) = 0;
            }
        }
    }
}

void month_0x004474E0(game_state *st)
{
    /* @bin 0x004474E0 */
    uint32_t c = CITY(st);
    S32(st, c + CITY_U476C) = S32(st, G_INDUSTRY_WORKING + 4 * 8);
    S32(st, c + CITY_U4770) = S32(st, c + CITY_U4774);
    S32(st, c + CITY_U4774) = 0;
    S32(st, c + CITY_U4778) = S32(st, G_INDUSTRY_WORKING + 4 * 7);
    S32(st, c + CITY_U477C) = S32(st, c + CITY_U4780);
    S32(st, c + CITY_U4780) = 0;
}

void month_0x004E9700(game_state *st)
{
    /* @bin 0x004E9700 city +0x24 into the ring of 0x960 i32 at +0x3C (index +0x25BC); +0x25C0 counts */
    uint32_t c = CITY(st);
    int32_t i = S32(st, c + CITY_POPULATION_HISTORY_INDEX);
    S32(st, c + CITY_POPULATION_HISTORY + 4u * (uint32_t) i) = S32(st, c + CITY_POPULATION);
    i = S32(st, c + CITY_POPULATION_HISTORY_INDEX) + 1;
    S32(st, c + CITY_POPULATION_HISTORY_INDEX) = i;
    if (i >= 0x960) {
        S32(st, c + CITY_POPULATION_HISTORY_INDEX) = 0;
    }
    S32(st, c + CITY_POPULATION_HISTORY_MONTHS) += 1;
}

void month_0x0045A3F0(game_state *st)
{
    /* @bin 0x0045A3F0 when city +0xC > -5000 and +0x499C is set: +0x2C98 added to +0x2CA0 and
     * +0x4450, taken from +0xC */
    uint32_t c = CITY(st);
    if (S32(st, c + CITY_TREASURY) > -5000 && S32(st, c + CITY_MANSION_BUILT) != 0) {
        S32(st, c + CITY_U2CA0) += S32(st, c + CITY_SALARY);
        S32(st, c + CITY_SAVINGS) += S32(st, c + CITY_SALARY);
        S32(st, c + CITY_TREASURY) -= S32(st, c + CITY_SALARY);
    }
}

void month_0x00459E50(game_state *st)
{
    /* @bin 0x00459E50 */
    uint32_t c = CITY(st);
    S32(st, c + CITY_TAXES_LAST_YEAR) = S32(st, c + CITY_U2C40) + S32(st, c + CITY_U2C3C);
    S32(st, c + CITY_U2C3C) = 0;
    S32(st, c + CITY_U2C40) = 0;
    S32(st, c + CITY_U2C44) = 0;
    S32(st, c + CITY_U2C48) = 0;
    /* @bin 0x00459E94 buildings in use (+0 == 1) with +5 set: +0x8C (i32) := 0 */
    int32_t n = S32(st, G_BUILDING_HIGHEST);
    for (int32_t id = 1; id <= n; id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) == 1 && U8(st, b + BLD_HOUSE_SIZE) != 0) {
            S32(st, b + BLD_U8C) = 0;
        }
    }
}

void month_0x0045A2E0(game_state *st)
{
    /* @bin 0x0045A2E0 v := +0x2BF4 * +0x2C08 / 10 / 12 (signed, each truncated); +0xC -= v,
     * +0x2C14 += v, +0x44DC += +0x2C08 */
    uint32_t c = CITY(st);
    int32_t v = (int32_t) ((uint32_t) S32(st, c + CITY_EMPLOYED) * (uint32_t) S32(st, c + CITY_WAGE_RATE)) / 10 / 12;
    S32(st, c + CITY_TREASURY) -= v;
    S32(st, c + CITY_WAGES_ACCUMULATOR) += v;
    S32(st, c + CITY_U44DC) += S32(st, c + CITY_WAGE_RATE);
}

void month_0x0045A350(game_state *st)
{
    /* @bin 0x0045A350 rate 0x00784DDC, less 5 (not below 0) when +0x4478 is set and building
     * +0x472C is of level 1; on a negative +0xC: percentage(-(+0xC), rate) / 12 taken from +0xC,
     * added to +0x2C74 */
    uint32_t rate = U32(st, G_INTEREST_RATE);
    uint32_t c = CITY(st);
    if (S16(st, c + CITY_U4478) != 0 && building_type_level(st, S32(st, c + CITY_U472C)) == 1) {
        int32_t r = (int32_t) (rate - 5);
        rate = r < 0 ? 0 : (uint32_t) r;
    }
    c = CITY(st);
    if (S32(st, c + CITY_TREASURY) < 0) {
        int32_t v = calc_percentage((int32_t) (0u - (uint32_t) S32(st, c + CITY_TREASURY)), (int32_t) rate) / 12;
        S32(st, c + CITY_TREASURY) -= v;
        S32(st, c + CITY_U2C74) += v;
    }
}

void month_0x00452160(game_state *st)
{
    /* @bin 0x00452160 months in a row with a negative +0xC (+0x482C); every 12th, +0x4830 + 1 and
     * city_add_0x42d8 by the table 0x005DA330 [min(+0x4830, 10) + difficulty * 10], message 0x79 */
    uint32_t c = CITY(st);
    if (S32(st, c + CITY_TREASURY) >= 0) {
        S32(st, c + CITY_U482C) = 0;
        S32(st, c + CITY_U4830) = 0;
        return;
    }
    int32_t n = S32(st, c + CITY_U482C) + 1;
    S32(st, c + CITY_U482C) = n;
    if (n == 0 || n % 12 != 0) {
        return;
    }
    int32_t k = S32(st, c + CITY_U4830) + 1;
    S32(st, c + CITY_U4830) = k;
    if (k >= 10) {
        k = 10;
    }
    city_add_0x42d8(st, S32(st, G_U5DA330 + 4u * (uint32_t) (k + S32(st, G_DIFFICULTY) * 10)));
    S32(st, G_MESSAGE_POPUP) = 1;
    GAME_CALL(st, FN_MESSAGE_POST, 0x79, 0, 0, 0, 0);
}

#define FORMATION_ADDR(id) (G_FORMATION_BASE + 0x90u * (uint32_t) (id))

void formation_add_0x2c(game_state *st, int32_t id, int16_t delta)
{
    /* @bin 0x004B7B50 record +0x2C (i16) += delta, within 0..max: type (+4) 0xD: 80, 100 when +0x48
     * is set; 0x39: 100; 0xC, 0xB: 60, 80 when +0x48 is set; else 80, 70 when +0x6A != 0xD */
    if (id == 0) {
        return;
    }
    uint32_t r = FORMATION_ADDR(id);
    int16_t t = S16(st, r + 4);
    int32_t max;
    if (t == 0xD) {
        max = (S16(st, r + 0x48) != 0 ? 0x14 : 0) + 0x50;
    } else if (t == 0x39) {
        max = 100;
    } else if (t == 0xC || t == 0xB) {
        max = (S16(st, r + 0x48) != 0 ? 0x14 : 0) + 0x3C;
    } else {
        max = (U8(st, r + 0x6A) != 0xD ? -10 : 0) + 0x50;
    }
    S16(st, r + 0x2C) = (int16_t) (S16(st, r + 0x2C) + delta);
    if (S16(st, r + 0x2C) > max) {
        S16(st, r + 0x2C) = (int16_t) max;
    }
    if (S16(st, r + 0x2C) < 0) {
        S16(st, r + 0x2C) = 0;
    }
}

void month_0x004B7960(game_state *st)
{
    /* @bin 0x004B7960 records 1..49 of 0x90 bytes at 0x00BCB9C8 in use (+0 == 1) and +0x69 clear */
    for (int32_t id = 1; id <= 49; id++) {
        uint32_t r = FORMATION_ADDR(id);
        if (U8(st, r) != 1 || U8(st, r + 0x69) != 0) {
            continue;
        }
        if (U8(st, r + 0x42) == 0) {
            formation_add_0x2c(st, id, 0);
        } else if (U8(st, r + 3) != 0) {
            U8(st, r + 0x72) = 0;
            U8(st, r + 0x73) = 0;
            U8(st, r + 0x66) = 0;
            formation_add_0x2c(st, id, 5);
            if (S16(st, r + 0x2A) == 6) {
                S16(st, r + 0x2A) = S16(st, r + 0x62);
            }
        } else if (S16(st, r + 0x50) == 0) {
            uint8_t m = (uint8_t) (U8(st, r + 0x72) + 1);
            U8(st, r + 0x72) = m;
            if (m > 3) {
                if (m > 100) {
                    U8(st, r + 0x72) = 100;
                }
                formation_add_0x2c(st, id, -5);
            }
        }
    }
}

int32_t building_0xa9_is(game_state *st, int32_t id, uint32_t value)
{
    /* @bin 0x004C52D0 building +0xA9 == 3, or == value */
    uint8_t v = U8(st, BUILDING_ADDR(id) + BLD_UA9);
    return v == 3 ? 1 : (uint32_t) v == value;
}

void month_0x004E8DB0(game_state *st)
{
    /* @bin 0x004E8DB0 d := (80 - city +0x10) / 5, less 2 when +0x4478 is set and building +0x472C
     * is of level 1 with +0xA9 in (1, 3); added to +0xE4 (within 0..1000) of every building in use
     * with +5 and +0x1C set. The loop runs to 0x0089B7BC: ids 1..4003 (past the 4000 records) */
    uint32_t c = CITY(st);
    int32_t d = (0x50 - S32(st, c + CITY_SENTIMENT)) / 5;
    int32_t extra = 0;
    if (S16(st, c + CITY_U4478) != 0 && building_type_level(st, S32(st, c + CITY_U472C)) == 1 &&
        building_0xa9_is(st, S32(st, CURRENT_CITY(st) + CITY_U472C), 1)) {
        extra = -2;
    }
    for (uint32_t p = BUILDING_ADDR(1) + BLD_CRIME_RISK; p < BUILDING_ADDR(BUILDING_COUNT) + BLD_CRIME_RISK; p += BUILDING_REC_SIZE) {
        if (U8(st, p - BLD_CRIME_RISK + BLD_STATE) != 1 || U8(st, p - BLD_CRIME_RISK + BLD_HOUSE_SIZE) == 0 || S16(st, p - BLD_CRIME_RISK + BLD_POPULATION) == 0) {
            continue;
        }
        int32_t v = (int32_t) ((uint32_t) S32(st, p) + (uint32_t) (extra + d));
        S32(st, p) = v;
        if (v < 0) {
            S32(st, p) = 0;
        }
        if (S32(st, p) > 1000) {
            S32(st, p) = 1000;
        }
    }
}


void month_0x004BE640(game_state *st, uint32_t self)
{
    /* @bin 0x004BE640 when +0x32FB4 == 5 and +0x32FBC is set: a message by 0x004BD590 (this +0x32C40)
     * in steps of 20 */
    if (S32(st, self + 0x32FB4) != 5 || U8(st, self + 0x32FBC) == 0) {
        return;
    }
    S32(st, G_MESSAGE_POPUP) = 1;
    int32_t v = flood_0x4bd590(st, self + 0x32C40u);
    int32_t k = (int32_t) ((uint32_t) v + 0x13) / 0x14;
    int alt = S16(st, G_GOD_MODE) != 0;
    int32_t msg;
    switch ((uint32_t) k) {
    case 0: msg = 0x158; break;
    case 1: msg = alt ? 0x159 : 0x157; break;
    case 2: msg = alt ? 0x15A : 0x156; break;
    case 3: msg = 0x15B; break;
    case 4: msg = 0x15C; break;
    case 5: msg = 0x15D; break;
    default: return;
    }
    GAME_CALL(st, FN_MESSAGE_POST, msg, 0, 0, 0, 0);
}

void month_0x0046E180(game_state *st, int32_t arg)
{
    /* @bin 0x0046E180 0x0046E1E0 (a tail jump) in months 0, 3, 6, 9 */
    rating_0x4269b0(st);
    rating_0x46e970(st, arg);
    GAME_CALL0(st, FN_MONUMENT_RATING_UPDATE);
    rating_0x46e7c0(st);
    int32_t m = S32(st, G_MONTH);
    if (m == 0 || m == 3 || m == 6 || m == 9) {
        rating_0x46e1e0(st);
    }
}

/* the event list at 0x00785D70: i32 count, then records of 0x7C bytes from +4 */
#define EVENT_ADDR(self, i) ((self) + 4u + 0x7Cu * (uint32_t) (i))

uint32_t events_next(game_state *st, uint32_t self, uint32_t after, int8_t create)
{
    /* @bin 0x0044BB90 (thiscall) the next record after `after` (0: from the first) whose +2 is 2;
     * its +0 (i16) := its index, 0x005D9A40 := index. None: with `create` and a start below 0x8C, the
     * record at the end (+0 := index) and this +0x43D4 is returned; else 0x005D9A40 - 1, 0 */
    int32_t start = 0;
    if (after != 0) {
        start = S16(st, after) + 1;
    }
    U8(st, G_U78A634) = 1;
    int32_t n = S32(st, self);
    int32_t i = start;
    if (start < n) {
        for (; i < n; i++) {
            if (U8(st, EVENT_ADDR(self, i) + 2) == 2) {
                S16(st, EVENT_ADDR(self, i)) = (int16_t) i;
                S32(st, G_U5D9A40) = i;
                return EVENT_ADDR(self, i);
            }
        }
    }
    if (create != 0 && start < 0x8C) {
        S16(st, EVENT_ADDR(self, i)) = (int16_t) i;
        S32(st, G_U5D9A40) = i;
        return self + 0x43D4u;
    }
    S32(st, G_U5D9A40) -= 1;
    return 0;
}

void month_0x004464F0(game_state *st)
{
    /* @bin 0x004464F0 unless 0x00605A3C == 1: 0x00451540 on each record of the list */
    if (S32(st, G_EDITOR) == 1) {
        return;
    }
    for (uint32_t e = events_next(st, G_EVENTS, 0, 0); e != 0; e = events_next(st, G_EVENTS, e, 0)) {
        GAME_CALLT0(st, e, FN_EVENT_REQUEST_MONTH);
    }
}

void events_0x0044BC20(game_state *st, uint32_t self)
{
    /* @bin 0x0044BC20 (thiscall) records with +0x28 bit 2 set and +0x32 clear: bit 1 set ->
     * bit 2 cleared, 0x00448A90, +0x14 (i16) += year - 0x007848D8; else with bit 4, +3 counts down
     * and at 0: bit 2 cleared, 0x00448A90 */
    for (int32_t i = 0; i < S32(st, self); i++) {
        uint32_t e = EVENT_ADDR(self, i);
        uint32_t f = U32(st, e + 0x28);
        if (!(f & 4) || U8(st, e + 0x32) != 0) {
            continue;
        }
        if (f & 2) {
            U32(st, e + 0x28) = f & ~4u;
            event_reset(st, e);
            S16(st, e + 0x14) = (int16_t) (S16(st, e + 0x14) + (int16_t) (U32(st, G_YEAR) - U32(st, G_START_YEAR)));
        } else if (f & 0x10) {
            uint8_t k = (uint8_t) (U8(st, e + 3) - 1);
            U8(st, e + 3) = k;
            if (k == 0) {
                U32(st, e + 0x28) &= ~4u;
                event_reset(st, e);
            }
        }
    }
}

void events_month(game_state *st, uint32_t self)
{
    /* @bin 0x0044BA90 (thiscall) 0x0044C660 on each record without +0x28 bit 0 (0x005D9A40 := its
     * index), then 0x0044BC20 */
    U8(st, self + 0x48B5) = 0;
    U8(st, self + 0x48B4) = 0;
    U8(st, G_U78A634) = 0;
    for (int32_t i = 0; i < S32(st, self); i++) {
        uint32_t e = EVENT_ADDR(self, i);
        if (!(U8(st, e + 0x28) & 1)) {
            S32(st, G_U5D9A40) = i;
            event_month_step(st, e);
        }
    }
    S32(st, G_U5D9A40) = -1;
    events_0x0044BC20(st, self);
}

void month_0x00452200(game_state *st)
{
    /* @bin 0x00452200 the flags 0x006F936C, 0x006F5694, 0x006DEA2C, 0x006E91D4 are cleared when no
     * figure of type 0x55 (+0x0A) in action 0xF, 0x10 or 0x24 (+0x38) has a building (+0x5C or +0x5E)
     * of type 0xE5, 0xEA, 0xEB, 0xEC respectively */
    static const uint32_t flag[4] = {G_U6F936C, G_U6F5694, G_U6DEA2C, G_U6E91D4};
    static const int16_t type[4] = {0xE5, 0xEA, 0xEB, 0xEC};
    uint8_t seen[4] = {0, 0, 0, 0}, set[4];
    for (int k = 0; k < 4; k++) {
        set[k] = U8(st, flag[k]) != 0;
    }
    for (int32_t id = 0; id < FIGURE_COUNT; id++) {
        uint32_t f = FIGURE_ADDR(id);
        uint8_t a = U8(st, f + FIG_ACTION);
        if (U8(st, f + FIG_TYPE) != 0x55 || (a != 0xF && a != 0x10 && a != 0x24)) {
            continue;
        }
        for (int j = 0; j < 2; j++) {
            int16_t t = S16(st, BUILDING_ADDR(S16(st, f + FIG_BUILDINGS_5C + 2u * (uint32_t) j)) + BLD_TYPE);
            for (int k = 0; k < 4; k++) {
                if (t == type[k]) {
                    seen[k] = 1;
                }
            }
        }
    }
    for (int k = 0; k < 4; k++) {
        if (set[k] && !seen[k]) {
            U8(st, flag[k]) = 0;
        }
    }
}

static int type_in_list_4bc8e4(int16_t t)
{
    /* @bin 0x004BC8E4 the byte table of the switch in 0x004BC7A0 (types 0x20..0xE9): these types */
    switch (t) {
    case 0x20: case 0x2F: case 0x33: case 0x35: case 0x4A: case 0x71: case 0xB1: case 0xB2: case 0xBB:
    case 0xBC: case 0xBD: case 0xC2: case 0xCB: case 0xCC: case 0xCD: case 0xE2: case 0xE7: case 0xE8:
    case 0xE9:
        return 1;
    default:
        return 0;
    }
}

void month_0x004BC7A0(game_state *st)
{
    /* @bin 0x004BC7A0 buildings in use of types 0x6E..0x72, 0x47, 0x5F or the list: +0x104, +0x106
     * := 0 (the loop runs to 0x0089B6E8: ids 1..4003) */
    for (uint32_t b = BUILDING_ADDR(1); b + BLD_TYPE < BUILDING_ADDR(BUILDING_COUNT) + BLD_TYPE; b += BUILDING_REC_SIZE) {
        if (U8(st, b + BLD_STATE) != 1) {
            continue;
        }
        int16_t t = S16(st, b + BLD_TYPE);
        if ((t >= 0x6E && t <= 0x72) || type_in_list_4bc8e4(t) || t == 0x47 || t == 0x5F) {
            S16(st, b + BLD_INCOMING_CARTS) = 0;
            S16(st, b + BLD_INCOMING_AMOUNT) = 0;
        }
    }
    /* @bin 0x004BC804 figures 1..1999 with +0xE set and +0x74 (i16) != 0, by their building +0x5E:
     * types 0x6E..0x72 and the list: 0x5F (dead) / 0xCC by the figure type +0xB, else +0x104 + 1;
     * other types: 0x47 -> +0x104 + 1, +0x106 + figure +0x74 */
    for (int32_t id = 1; id <= 1999; id++) {
        uint32_t f = FIGURE_ADDR(id);
        int16_t n = S16(st, f + FIG_AMOUNT);
        if (U8(st, f + FIG_STATE) == 0 || n == 0) {
            continue;
        }
        uint32_t b = BUILDING_ADDR(S16(st, f + FIG_DEST_BUILDING));
        int16_t t = S16(st, b + BLD_TYPE);
        if ((t >= 0x6E && t <= 0x72) || type_in_list_4bc8e4(t)) {
            uint8_t ft = U8(st, f + FIG_RESOURCE);
            if (t == 0x5F) {
                if (ft == 0x1C) {
                    S16(st, b + BLD_INCOMING_AMOUNT) += 1;
                } else if (ft == 0xA) {
                    S16(st, b + BLD_INCOMING_CARTS) += 1;
                }
            } else if (t == 0xCC) {
                if (ft == 9) {
                    S16(st, b + BLD_INCOMING_AMOUNT) += 1;
                } else if (ft == 0xB) {
                    S16(st, b + BLD_INCOMING_CARTS) += 1;
                }
            } else {
                S16(st, b + BLD_INCOMING_CARTS) += 1;
            }
        } else if (t == 0x47) {
            S16(st, b + BLD_INCOMING_CARTS) += 1;
            S16(st, b + BLD_INCOMING_AMOUNT) = (int16_t) (S16(st, b + BLD_INCOMING_AMOUNT) + n);
        }
    }
}

static int32_t tier_by_0x24(int32_t v)
{
    /* @bin 0x0045A6D0 by city +0x24: >= 5001: 500, 3001: 400, 2001: 300, 1001: 225, 501: 150, else 50 */
    return v >= 0x1389 ? 500 : v >= 0xBB9 ? 400 : v >= 0x7D1 ? 300 : v >= 0x3E9 ? 225 : v >= 0x1F5 ? 150 : 50;
}

void month_0x0045A7B0(game_state *st)
{
    /* @bin 0x0045A7B0 +0x44E0 takes a value by +0x24 and +0x2CB8 (as 0x0045A5D0 does for +0x44E4),
     * then 0 on every path */
    uint32_t c = CITY(st);
    if (S32(st, c + CITY_TREASURY) > 0) {
        int32_t v = S32(st, c + CITY_POPULATION);
        if (S32(st, c + CITY_NET_THIS_YEAR) > 0) {
            S32(st, c + CITY_TRIBUTE_THIS_YEAR) = tier_by_0x24(v);
            int32_t p = calc_percentage(S32(st, c + CITY_NET_THIS_YEAR), 0x19);
            c = CITY(st);
            if (p > S32(st, c + CITY_TRIBUTE_THIS_YEAR)) {
                S32(st, c + CITY_TRIBUTE_THIS_YEAR) = p;
            }
        } else if (v >= 0x7D1) {
            S32(st, c + CITY_TRIBUTE_THIS_YEAR) = 200;
        } else if (v >= 0x3E9) {
            S32(st, c + CITY_TRIBUTE_THIS_YEAR) = 100;
        }
    }
    S32(st, c + CITY_TRIBUTE_THIS_YEAR) = 0;
}

void month_0x0045A5D0(game_state *st)
{
    /* @bin 0x0045A5D0 */
    uint32_t c = CITY(st);
    S32(st, c + CITY_GIFTS_LAST_YEAR) = S32(st, c + CITY_GIFTS_THIS_YEAR);
    S32(st, c + CITY_GIFTS_THIS_YEAR) = 0;
    S32(st, c + CITY_GOLD_LAST_YEAR) = S32(st, c + CITY_GOLD_THIS_YEAR);
    S32(st, c + CITY_GOLD_THIS_YEAR) = 0;
    S32(st, c + CITY_U44E8) = 0;
    int32_t in = (int32_t) ((uint32_t) S32(st, c + CITY_TAXES_LAST_YEAR) + (uint32_t) S32(st, c + CITY_EXPORTS_LAST_YEAR) +
                            (uint32_t) S32(st, c + CITY_GOLD_LAST_YEAR) + (uint32_t) S32(st, c + CITY_GIFTS_LAST_YEAR));
    int32_t out = (int32_t) ((uint32_t) S32(st, c + CITY_INTEREST_LAST_YEAR) + (uint32_t) S32(st, c + CITY_SALARY_LAST_YEAR) +
                             (uint32_t) S32(st, c + CITY_CONSTRUCTION_LAST_YEAR) + (uint32_t) S32(st, c + CITY_WAGES_LAST_YEAR) +
                             (uint32_t) S32(st, c + CITY_IMPORTS_LAST_YEAR));
    int32_t v = 0;
    int set = 1; /* 0x0045A762: +0x44E4 := v */
    if (S32(st, c + CITY_PLAYER_RANK) == 10) {
        S32(st, c + CITY_U44E8) = 0;
        S32(st, c + CITY_U44EC) = 0;
    } else if (S32(st, c + CITY_TREASURY) <= 0) {
        S32(st, c + CITY_U44E8) = 1;
        S32(st, c + CITY_U44EC) += 1;
    } else {
        S32(st, c + CITY_U44EC) = 0;
        if (in > out) {
            /* @bin 0x0045A6D0 by +0x24, at least 25% of the surplus */
            S32(st, c + CITY_TRIBUTE_LAST_YEAR) = tier_by_0x24(S32(st, c + CITY_POPULATION));
            v = calc_percentage((int32_t) ((uint32_t) in - (uint32_t) out), 0x19);
            c = CITY(st);
            set = v > S32(st, c + CITY_TRIBUTE_LAST_YEAR);
        } else if (S32(st, c + CITY_POPULATION) >= 0x7D1) {
            S32(st, c + CITY_TRIBUTE_LAST_YEAR) = 200;
            set = 0;
        } else if (S32(st, c + CITY_POPULATION) >= 0x3E9) {
            S32(st, c + CITY_TRIBUTE_LAST_YEAR) = 100;
            set = 0;
        }
    }
    if (set) {
        S32(st, c + CITY_TRIBUTE_LAST_YEAR) = v;
    }
    /* @bin 0x0045A768 */
    S32(st, c + CITY_TREASURY) -= S32(st, c + CITY_TRIBUTE_LAST_YEAR);
    out = (int32_t) ((uint32_t) out + (uint32_t) S32(st, c + CITY_TRIBUTE_LAST_YEAR));
    month_0x0045A7B0(st);
    c = CITY(st);
    S32(st, c + CITY_BALANCE_LAST_YEAR) = S32(st, c + CITY_TREASURY);
    S32(st, c + CITY_INCOME_LAST_YEAR) = in;
    S32(st, c + CITY_EXPENSES_LAST_YEAR) = out;
}

void month_0x0045E420(game_state *st)
{
    /* @bin 0x0045E420 the month's consumption by the houses */
    GAME_CALL(st, FN_DAY_TOTALS_45D630, S32(st, CURRENT_PLAYER_ADDR));
    uint32_t c = CITY(st);
    S32(st, c + CITY_U2974) = 0;
    S32(st, c + CITY_U20) = 0;
    int32_t rate = GAME_CALL(st, FN_TRADE_TAX_4C5340, S32(st, CURRENT_PLAYER_ADDR));
    S32(st, CITY(st) + CITY_U2974) = 1;
    int32_t sum = 0;
    /* @bin 0x0045E668 the stock index (building +0x54, i16) of each kind of the list 0x00F53DDC */
    static const int8_t stock[0x13] = {0, 1, 2, 3, 4, 5, 6, 7, -1, -1, -1, -1, 9, -1, 8, -1, 10, -1, 11};
    for (int32_t id = 1; id <= S32(st, G_BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) != 1 || U8(st, b + BLD_HOUSE_SIZE) == 0) {
            continue;
        }
        /* @bin 0x0045E4B0 per-level divisor 0x0070CDF4 (stride 0x58) by +0x12 */
        int32_t div = S32(st, G_HOUSE_LEVEL_U28 + 0x58u * (uint32_t) (int32_t) S16(st, b + BLD_SUBTYPE));
        int32_t need = calc_percentage(S16(st, b + BLD_POPULATION), rate);
        if (div > 1) {
            need /= div;
        }
        U8(st, b + BLD_U7F) = 0;
        if (S32(st, G_U784BBC) != 0) {
            S16(st, b + BLD_HOUSE_INVENTORY) = (int16_t) need;
            U8(st, b + BLD_U7F) = 1;
            S32(st, CITY(st) + CITY_FOOD_TYPES) = 1;
            continue;
        }
        if (div <= 0) {
            continue;
        }
        for (int32_t k = 0; k < S32(st, G_GOODS_COUNT); k++) {
            uint32_t kind = U32(st, G_GOODS_LIST + 4u * (uint32_t) k);
            if (kind == 0) {
                continue;
            }
            int32_t g = kind - 1 <= 0x12 ? stock[kind - 1] : -1;
            uint32_t p = b + BLD_HOUSE_INVENTORY + 2u * (uint32_t) g;
            int16_t have = S16(st, p);
            int eaten = 1;
            if (have >= need) {
                S16(st, p) = (int16_t) (have - need);
            } else if (have != 0) {
                S16(st, p) = 0;
            } else {
                eaten = 0;
            }
            if (eaten) {
                U8(st, b + BLD_U7F) += 1;
                sum += need;
            }
            int32_t n = U8(st, b + BLD_U7F);
            if (n > S32(st, CITY(st) + CITY_U2974)) {
                S32(st, CITY(st) + CITY_U2974) = n;
            }
            if (n >= div) {
                break;
            }
        }
    }
    c = CITY(st);
    S32(st, c + CITY_FOOD_EATEN) = sum;
    S32(st, c + CITY_U45E0) = S32(st, c + CITY_U45E4);
    S32(st, c + CITY_U45E4) = 0;
}

void month_0x004599D0(game_state *st)
{
    /* @bin 0x004599D0 the month's taxes: per house level (+0x12) tax 0x0070CE18 (stride 0x58), 110%
     * when +0x88 is set; levels >= 14 apart from the others; houses with +0x4E set pay */
    uint32_t c = CITY(st);
    S32(st, c + CITY_U2C20) = 0;
    S32(st, c + CITY_U2C24) = 0;
    S32(st, c + CITY_U2C28) = 0;
    S32(st, c + CITY_U2C2C) = 0;
    S32(st, c + CITY_U2C58) = 0;
    S32(st, c + CITY_U2C54) = 0;
    S32(st, c + CITY_U2C60) = 0;
    S32(st, c + CITY_U2C5C) = 0;
    for (uint32_t k = 0; k < 0x14; k++) {
        S32(st, c + CITY_POPULATION_BY_HOUSE + 4 * k) = 0;
    }
    uint32_t diff_pct = G_DIFFICULTY_TAX; /* @bin 0x00459ACD i32 per difficulty (0x00E39080) */
    for (int32_t id = 1; id <= S32(st, G_BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) != 1 || U8(st, b + BLD_HOUSE_SIZE) == 0) {
            continue;
        }
        int16_t level = S16(st, b + BLD_SUBTYPE);
        int high = level >= 0xE;
        int32_t rate = S32(st, G_HOUSE_TAX_VALUE + 0x58u * (uint32_t) (int32_t) level);
        if (U8(st, b + BLD_U88) != 0) {
            rate = calc_percentage(rate, 0x6E);
        }
        c = CITY(st);
        int32_t pop = S16(st, b + BLD_POPULATION);
        S32(st, c + CITY_POPULATION_BY_HOUSE + 4u * (uint32_t) (int32_t) level) += pop;
        uint32_t due = (uint32_t) pop * (uint32_t) rate;
        if (U8(st, b + BLD_TAX_COVERAGE) != 0) {
            S32(st, c + (high ? CITY_U2C24 : CITY_U2C20)) += pop;
            uint32_t m = U32(st, diff_pct + 4u * U32(st, G_DIFFICULTY)) * U32(st, c + CITY_TAX_RATE) * due;
            U32(st, b + BLD_U8C) += m;
            U32(st, c + (high ? CITY_U2C5C : CITY_U2C54)) += due;
        } else {
            S32(st, c + (high ? CITY_U2C2C : CITY_U2C28)) += pop;
            U32(st, c + (high ? CITY_U2C60 : CITY_U2C58)) += due;
        }
    }
    /* @bin 0x00459B69 each sum at the tax rate (+8), then the difficulty's percentage */
    int32_t dp;
    c = CITY(st);
    dp = S32(st, diff_pct + 4u * U32(st, G_DIFFICULTY));
    int32_t a = calc_percentage(calc_percentage(S32(st, c + CITY_U2C5C), S32(st, c + CITY_TAX_RATE)), dp);
    S32(st, c + CITY_U2C40) += a;
    dp = S32(st, diff_pct + 4u * U32(st, G_DIFFICULTY));
    int32_t b2 = calc_percentage(calc_percentage(S32(st, c + CITY_U2C54), S32(st, c + CITY_TAX_RATE)), dp);
    S32(st, c + CITY_U2C3C) += b2;
    dp = S32(st, diff_pct + 4u * U32(st, G_DIFFICULTY));
    S32(st, c + CITY_U2C48) += calc_percentage(calc_percentage(S32(st, c + CITY_U2C60), S32(st, c + CITY_TAX_RATE)), dp);
    dp = S32(st, diff_pct + 4u * U32(st, G_DIFFICULTY));
    S32(st, c + CITY_U2C44) += calc_percentage(calc_percentage(S32(st, c + CITY_U2C58), S32(st, c + CITY_TAX_RATE)), dp);
    S32(st, c + CITY_TREASURY) = (int32_t) ((uint32_t) S32(st, c + CITY_TREASURY) + (uint32_t) a + (uint32_t) b2);
    /* @bin 0x00459C92 the shares paying (0x004CDE60) */
    S32(st, c + CITY_U2C30) = GAME_CALL(st, FN_CALC_RATIO_PERCENT, S32(st, c + CITY_U2C20), S32(st, c + CITY_U2C28) + S32(st, c + CITY_U2C20));
    c = CITY(st);
    S32(st, c + CITY_U2C34) = GAME_CALL(st, FN_CALC_RATIO_PERCENT, S32(st, c + CITY_U2C24), S32(st, c + CITY_U2C2C) + S32(st, c + CITY_U2C24));
    c = CITY(st);
    int32_t r = GAME_CALL(st, FN_CALC_RATIO_PERCENT, S32(st, c + CITY_U2C20) + S32(st, c + CITY_U2C24),
                          S32(st, c + CITY_U2C2C) + S32(st, c + CITY_U2C28) + S32(st, c + CITY_U2C20) + S32(st, c + CITY_U2C24));
    S32(st, CITY(st) + CITY_U2C38) = r;
}

void buildings_add_0xc2(game_state *st, int8_t delta)
{
    /* @bin 0x004E8E80 buildings in use with +5 set: +0xC2 (i8) += delta, within 0..100 (the loop runs
     * to 0x0089B79A: ids 1..4003) */
    for (uint32_t p = BUILDING_ADDR(1) + BLD_SENTIMENT; p < BUILDING_ADDR(BUILDING_COUNT) + BLD_SENTIMENT; p += BUILDING_REC_SIZE) {
        if (U8(st, p - BLD_SENTIMENT + BLD_STATE) != 1 || U8(st, p - BLD_SENTIMENT + BLD_HOUSE_SIZE) == 0) {
            continue;
        }
        int8_t v = (int8_t) (U8(st, p) + (uint8_t) delta);
        S8(st, p) = v;
        if (v < 0) {
            S8(st, p) = 0;
        }
        if (S8(st, p) > 100) {
            S8(st, p) = 100;
        }
    }
}

void month_0x004EA000(game_state *st)
{
    /* @bin 0x004EA000 the festival: +0x4528, +0x452C count down; +0x44F8 (its size 1..3) counts its
     * months in +0x44FC; when they run out, the buildings' +0xC2 and the god's (+0x4500) byte +0x439A
     * rise by the size and the two counters, message 0x88 + size, 0x00461950(size * 10) */
    uint32_t c = CITY(st);
    if (S32(st, c + CITY_U4528) != 0) {
        S32(st, c + CITY_U4528) -= 1;
    }
    if (S32(st, c + CITY_U452C) != 0) {
        S32(st, c + CITY_U452C) -= 1;
    }
    if (S32(st, c + CITY_FESTIVAL_SIZE) <= 0) {
        return;
    }
    int32_t left = S32(st, c + CITY_FESTIVAL_MONTHS) - 1;
    S32(st, c + CITY_FESTIVAL_MONTHS) = left;
    if (left > 0) {
        return;
    }
    int32_t size = S32(st, c + CITY_FESTIVAL_SIZE);
    if (S32(st, c + CITY_U4528) <= 0) {
        static const int8_t rise[5] = {0, 6, 10, 15, 15};
        static const uint8_t god_rise[5] = {0, 1, 2, 3, 3};
        if (size >= 1 && size <= 4) {
            buildings_add_0xc2(st, rise[size]);
            c = CITY(st);
            if (size < 4) {
                U8(st, c + CITY_GOD_MOOD + U32(st, c + CITY_FESTIVAL_GOD)) += god_rise[size];
            } else {
                for (uint32_t g = 0; g < 5; g++) {
                    if (g != 4) {
                        U8(st, c + CITY_GOD_MOOD + g) += 3;
                    }
                }
            }
        }
    } else if (S32(st, c + CITY_U452C) <= 0) {
        static const int8_t rise[4] = {0, 2, 3, 5};
        static const uint8_t god_rise[4] = {0, 1, 2, 5};
        if (size >= 1 && size <= 3) {
            U8(st, c + CITY_GOD_MOOD + U32(st, c + CITY_FESTIVAL_GOD)) += god_rise[size];
            buildings_add_0xc2(st, rise[size]);
        }
    }
    /* @bin 0x004EA1B1 */
    c = CITY(st);
    S32(st, G_MESSAGE_POPUP) = 1;
    S32(st, c + CITY_U438C) = 1;
    S32(st, c + CITY_GOD_MONTHS + 4u * U32(st, c + CITY_FESTIVAL_GOD)) = 0;
    size = S32(st, c + CITY_FESTIVAL_SIZE);
    if (size >= 1 && size <= 3) {
        GAME_CALL(st, FN_MESSAGE_POST, 0x88 + size, 0, 0, 0, 0);
    }
    GAME_CALL(st, FN_FESTIVAL_PERFORMERS, (int32_t) ((uint32_t) S32(st, CITY(st) + CITY_FESTIVAL_SIZE) * 10u));
    c = CITY(st);
    S32(st, c + CITY_FESTIVAL_SIZE) = 0;
    S32(st, c + CITY_FESTIVAL_MONTHS) = 0;
}

void month_0x004E7700(game_state *st)
{
    /* @bin 0x004E7700 the locusts: when the timer 0x0078D79C is -1 and 0x004BECF0 (this 0x00D05720)
     * is the month + 2 or the month - 10; the timer := 0xA48; 9 figures of type 0x6B per building
     * accepted by 0x00488030 (this 0x00BCA350), placed with the game RNG; then the ambient sound */
    if (S32(st, G_LOCUST_TIMER) != -1) {
        return;
    }
    if (GAME_CALLT0(st, G_UD05720, FN_FLOOD_MONTH_BEFORE) != S32(st, G_MONTH) + 2 &&
        GAME_CALLT0(st, G_UD05720, FN_FLOOD_MONTH_BEFORE) != S32(st, G_MONTH) - 10) {
        return;
    }
    S32(st, G_LOCUST_TIMER) = 0xA48;
    for (int32_t id = 1; id <= S32(st, G_BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if ((int8_t) GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_IS_TYPE_0X487F80, id) == 0) {
            continue;
        }
        for (int32_t k = 0; k < 9; k++) {
            random_next_state(st);
            int32_t w = S32(st, G_MAP_WIDTH);
            int32_t r = (int32_t) U32(st, G_R_1_15) % (w / 4);
            int32_t x = (int32_t) ((uint32_t) w * 7u) / 8 - r;
            int32_t y = (int32_t) ((uint32_t) w * 3u) / 8 - r;
            int32_t fid = GAME_CALL(st, FN_FIGURE_CREATE, S32(st, CURRENT_PLAYER_ADDR), 0x6B, x, y, 0);
            S16(st, b + BLD_IMMIGRANT) = (int16_t) fid;
            uint32_t f = FIGURE_ADDR(fid);
            U8(st, f + FIG_ACTION) = 8;
            S16(st, f + FIG_TARGET_HOUSE) = (int16_t) id;
            random_next_state(st);
            S16(st, f + FIG_WAIT_TICKS) = (int16_t) ((int32_t) U32(st, G_R_1_15) % 300);
            U8(st, f + FIG_MIGRANT_PEOPLE) = 0;
            U8(st, f + FIG_TERRAIN_USAGE) = 0xB;
            S16(st, f + FIG_DEST_X) = (int16_t) (k / 3 + S16(st, b + BLD_X));
            S16(st, f + FIG_DEST_Y) = (int16_t) (k % 3 + S16(st, b + BLD_Y));
        }
        if (S16(st, b + BLD_WORKERS) > 0) {
            U8(st, b + BLD_UA7) = 0;
            S16(st, b + BLD_WORKERS) = 0;
            int32_t fid = S16(st, b + BLD_SECOND_WALKER);
            if (fid != 0) {
                uint32_t f = FIGURE_ADDR(fid);
                if (U8(st, f + FIG_STATE) != 2 && U8(st, f + FIG_TYPE) == 0x55) {
                    U8(st, f + FIG_STATE) = 2;
                    GAME_CALL(st, FN_FIGURE_TILE_LIST_REMOVE, fid);
                }
                S16(st, b + BLD_SECOND_WALKER) = 0;
            }
        }
    }
    /* @bin 0x004E7A67 the sound channels (this 0x00E92BF8) */
    if (GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAYING, 0) != 0) {
        GAME_CALLT(st, G_SOUND, FN_SOUND_STOP_STREAM, 0);
    }
    S32(st, G_UF55B94) = 1;
    if (GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAYING, 2) != 0) {
        GAME_CALLT(st, G_SOUND, FN_SOUND_STOP_STREAM, 2);
    }
    GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAY, (int32_t) G_U5E4990, 2, 1); /* "audio/ambient/locusts.mp3" */
}

void tutorial_menu_flags(game_state *st)
{
    /* @bin 0x004179F0 in the first missions (0x00E38EA8 <= 10, 0x00E38EBC clear): each of the 26
     * steps of 0x005794B0 (0x1C bytes: +0 mission, +0xC count, +0x10 list of indexes) of this
     * mission sets the flags 0x006FDC88[index] := its step flag 0x007027B8[k] != 0; later
     * missions: the 15 flags := 1 */
    int8_t mission = S8(st, G_MISSION);
    if (mission > 0xA || S32(st, G_CUSTOM) != 0) {
        for (uint32_t k = 0; k < 15; k++) {
            U8(st, G_ADVISOR_AVAILABLE + k) = 1;
        }
        return;
    }
    for (uint32_t k = 0; k < 26; k++) {
        uint32_t r = G_MISSION_LISTS + G_MISSION_LIST_STRIDE * k;
        int32_t n = S32(st, r + MISSIONLIST_COUNT);
        if (S32(st, r + MISSIONLIST_MISSION) != mission || n <= 0) {
            continue;
        }
        uint8_t on = U8(st, G_MISSION_LIST_FLAGS + k) != 0;
        uint32_t list = U32(st, r + MISSIONLIST_INDEXES);
        for (; n != 0; n--, list += 4) {
            U8(st, G_ADVISOR_AVAILABLE + U32(st, list)) = on;
        }
    }
}

void tutorial_step(game_state *st, int32_t step)
{
    /* @bin 0x00417AE0 the step's flag 0x007027B8[step] := 1, the building menus rebuilt
     * (0x00417A70, 0x00404350, 0x004051A0, 0x00417AA0), the step's flags, the side panel when the
     * city is shown (0x0051E990), and the step's message (0x005794C4, stride 0x1C) */
    U8(st, G_MISSION_LIST_FLAGS + (uint32_t) step) = 1;
    GAME_CALL0(st, FN_PLAY_BUILD_COUNTS_KEEP);
    GAME_CALL0(st, FN_BUILD_MENU_UPDATE);
    GAME_CALL0(st, FN_BUILD_BUTTONS_UPDATE);
    GAME_CALL0(st, FN_PLAY_BUILD_COUNTS_MARK);
    tutorial_menu_flags(st);
    if (U8(st, G_WINDOW) == 1) {
        GAME_CALL0(st, FN_SIDEBAR_DRAW);
    }
    S32(st, G_MESSAGE_POPUP) = 1;
    GAME_CALL(st, FN_MESSAGE_POST, S32(st, G_MISSION_LISTS + MISSIONLIST_MESSAGE + G_MISSION_LIST_STRIDE * (uint32_t) step), 0, 0, 0, 0);
}

#define VOID0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
VOID0(run_455530, month_0x00455530)
VOID0(run_4e8630, month_0x004E8630)
VOID0(run_4e1850, month_0x004E1850)
VOID0(run_4ea740, month_0x004EA740)
VOID0(run_45a4d0, month_0x0045A4D0)
VOID0(run_45a500, month_0x0045A500)
VOID0(run_45a530, month_0x0045A530)
VOID0(run_45a5a0, month_0x0045A5A0)
VOID0(run_4c5300, month_0x004C5300)
VOID0(run_45a450, month_0x0045A450)
VOID0(run_45a490, month_0x0045A490)
VOID0(run_4448e0, month_0x004448E0)
VOID0(run_45a560, month_0x0045A560)
VOID0(run_4474e0, month_0x004474E0)

VOID0(run_4e9700, month_0x004E9700)
VOID0(run_45a3f0, month_0x0045A3F0)
VOID0(run_459e50, month_0x00459E50)
VOID0(run_45a2e0, month_0x0045A2E0)
VOID0(run_45a350, month_0x0045A350)
VOID0(run_452160, month_0x00452160)
VOID0(run_4b7960, month_0x004B7960)
VOID0(run_4e8db0, month_0x004E8DB0)
VOID0(run_4464f0, month_0x004464F0)
VOID0(run_452200, month_0x00452200)
VOID0(run_4bc7a0, month_0x004BC7A0)
VOID0(run_45a7b0, month_0x0045A7B0)
VOID0(run_45a5d0, month_0x0045A5D0)
VOID0(run_45e420, month_0x0045E420)
VOID0(run_4599d0, month_0x004599D0)
VOID0(run_4ea000, month_0x004EA000)
VOID0(run_4e7700, month_0x004E7700)
VOID0(run_4179f0, tutorial_menu_flags)

static int32_t run_tutorial_step(game_state *st, const int32_t *a)
{
    tutorial_step(st, a[0]);
    return 0;
}

static int32_t run_add_c2(game_state *st, const int32_t *a)
{
    buildings_add_0xc2(st, (int8_t) a[0]);
    return 0;
}

static int32_t run_formation_add(game_state *st, const int32_t *a)
{
    formation_add_0x2c(st, a[0], (int16_t) a[1]);
    return 0;
}

static int32_t run_0xa9_is(game_state *st, const int32_t *a)
{
    return building_0xa9_is(st, a[0], (uint32_t) a[1]);
}

static int32_t run_4be640(game_state *st, const int32_t *a)
{
    (void) a;
    month_0x004BE640(st, G_UD05720);
    return 0;
}

static int32_t run_46e180(game_state *st, const int32_t *a)
{
    month_0x0046E180(st, a[0]);
    return 0;
}

static int32_t run_events_next(game_state *st, const int32_t *a)
{
    return (int32_t) events_next(st, G_EVENTS, (uint32_t) a[0], (int8_t) a[1]);
}

static int32_t run_44bc20(game_state *st, const int32_t *a)
{
    (void) a;
    events_0x0044BC20(st, G_EVENTS);
    return 0;
}

static int32_t run_events_month(game_state *st, const int32_t *a)
{
    (void) a;
    events_month(st, G_EVENTS);
    return 0;
}

static int32_t run_type_level(game_state *st, const int32_t *a)
{
    return building_type_level(st, a[0]);
}

static int32_t run_add_22d8(game_state *st, const int32_t *a)
{
    city_add_0x42d8(st, a[0]);
    return 0;
}

static int32_t run_sum_1c(game_state *st, const int32_t *a)
{
    return city_sum_building_1c(st, a[0]);
}

const ported_function month_ports[] = {
    {0x004C5120u, "building_type_level", 1, 32, 0, run_type_level, 0},
    {0x004E96C0u, "city_add_0x42d8", 1, 0, 0, run_add_22d8, 0},
    {0x00458F80u, "city_sum_building_1c", 1, 32, 0, run_sum_1c, 0},
    {0x00455530u, "month_0x00455530", 0, 0, 0, run_455530, 0},
    {0x004E8630u, "month_0x004E8630", 0, 0, 0, run_4e8630, 0},
    {0x004E1850u, "month_0x004E1850", 0, 0, 0, run_4e1850, 0},
    {0x004EA740u, "month_0x004EA740", 0, 0, 0, run_4ea740, 0},
    {0x0045A4D0u, "month_0x0045A4D0", 0, 0, 0, run_45a4d0, 0},
    {0x0045A500u, "month_0x0045A500", 0, 0, 0, run_45a500, 0},
    {0x0045A530u, "month_0x0045A530", 0, 0, 0, run_45a530, 0},
    {0x0045A5A0u, "month_0x0045A5A0", 0, 0, 0, run_45a5a0, 0},
    {0x004C5300u, "month_0x004C5300", 0, 0, 0, run_4c5300, 0},
    {0x0045A450u, "month_0x0045A450", 0, 0, 0, run_45a450, 0},
    {0x0045A490u, "month_0x0045A490", 0, 0, 0, run_45a490, 0},
    {0x004448E0u, "month_0x004448E0", 0, 0, 0, run_4448e0, 0},
    {0x0045A560u, "month_0x0045A560", 0, 0, 0, run_45a560, 0},
    {0x004474E0u, "month_0x004474E0", 0, 0, 0, run_4474e0, 0},
    {0x004E9700u, "month_0x004E9700", 0, 0, 0, run_4e9700, 0},
    {0x0045A3F0u, "month_0x0045A3F0", 0, 0, 0, run_45a3f0, 0},
    {0x00459E50u, "month_0x00459E50", 0, 0, 0, run_459e50, 0},
    {0x0045A2E0u, "month_0x0045A2E0", 0, 0, 0, run_45a2e0, 0},
    {0x0045A350u, "month_0x0045A350", 0, 0, 0, run_45a350, 0},
    {0x00452160u, "month_0x00452160", 0, 0, 0, run_452160, 0},
    {0x004B7B50u, "formation_add_0x2c", 2, 0, 0, run_formation_add, 0},
    {0x004B7960u, "month_0x004B7960", 0, 0, 0, run_4b7960, 0},
    {0x004C52D0u, "building_0xa9_is", 2, 8, 0, run_0xa9_is, 0},
    {0x004E8DB0u, "month_0x004E8DB0", 0, 0, 0, run_4e8db0, 0},
    {0x004BE640u, "month_0x004BE640", 0, 0, G_UD05720, run_4be640, 0},
    {0x0046E180u, "month_0x0046E180", 1, 0, 0, run_46e180, 0},
    {0x0044BB90u, "events_next", 2, 32, G_EVENTS, run_events_next, 0},
    {0x004464F0u, "month_0x004464F0", 0, 0, 0, run_4464f0, 0},
    {0x0044BC20u, "events_0x0044BC20", 0, 0, G_EVENTS, run_44bc20, 0},
    {0x0044BA90u, "events_month", 0, 0, G_EVENTS, run_events_month, 0},
    {0x00452200u, "month_0x00452200", 0, 0, 0, run_452200, 0},
    {0x004BC7A0u, "month_0x004BC7A0", 0, 0, 0, run_4bc7a0, 0},
    {0x0045A7B0u, "month_0x0045A7B0", 0, 0, 0, run_45a7b0, 0},
    {0x0045A5D0u, "month_0x0045A5D0", 0, 0, 0, run_45a5d0, 0},
    {0x0045E420u, "month_0x0045E420", 0, 0, 0, run_45e420, 0},
    {0x004599D0u, "month_0x004599D0", 0, 0, 0, run_4599d0, 0},
    {0x004E8E80u, "buildings_add_0xc2", 1, 0, 0, run_add_c2, 0},
    {0x004EA000u, "month_0x004EA000", 0, 0, 0, run_4ea000, 0},
    {0x004E7700u, "month_0x004E7700", 0, 0, 0, run_4e7700, 0},
    {0x004179F0u, "tutorial_menu_flags", 0, 0, 0, run_4179f0, 0},
    {0x00417AE0u, "tutorial_step", 1, 0, 0, run_tutorial_step, 0},
};
const int month_ports_count = sizeof(month_ports) / sizeof(month_ports[0]);
