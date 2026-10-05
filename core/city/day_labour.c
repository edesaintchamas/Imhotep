/* Imhotep - the city's labour totals, by day (day step 0x19).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/day_labour.h"

#include "city/day_buildings.h"
#include "sim/call.h"
#include "sim/math.h"
#include "state/ported.h"
#include "state/records.h"

#define BUILDING_HIGHEST 0x0078D790u
#define BUILDING_MANAGER 0x00BCA350u
#define TYPE_CATEGORY 0x005D06ECu /* i32 labour category per building type, stride 0x14 */
#define CATEGORIES 0x2B2Cu        /* city: 10 categories of 0x14 bytes: needed, available, ... */
#define CAT_NEEDED(k) (CATEGORIES + 0x14u * (uint32_t) (k))
#define CAT_AVAILABLE(k) (CATEGORIES + 4 + 0x14u * (uint32_t) (k))
#define CAT_PRIORITY(k) (CATEGORIES + 0x10 + 0x14u * (uint32_t) (k))
#define WORKFORCE 0x2B28u
#define DEFAULT_SHARES 0x005DA44Cu /* (i32 category, i32 workers)[9] */

void labour_categories_fill(game_state *st)
{
    /* @bin 0x00459400 */
    uint32_t c = CURRENT_CITY(st);
    int32_t needed = 0;
    for (int k = 0; k < 10; k++) {
        S32(st, c + CAT_AVAILABLE(k)) = 0;
    }
    for (int k = 0; k < 10; k++) {
        needed += S32(st, c + CAT_NEEDED(k));
    }
    S32(st, c + 0x2C04) = 0;
    int32_t left = S32(st, c + WORKFORCE);
    if (needed <= left) {
        for (int k = 0; k < 10; k++) {
            S32(st, c + CAT_AVAILABLE(k)) = S32(st, c + CAT_NEEDED(k));
        }
        S32(st, c + 0x2BF4) = needed;
    } else {
        /* @bin 0x00459474 priorities 1..9 first: the category with that priority (among the first
         * nine) takes what it needs */
        for (int32_t prio = 1;; prio++) {
            for (int k = 0; k < 9; k++) {
                if (S32(st, c + CAT_PRIORITY(k)) == prio) {
                    int32_t v = S32(st, c + CAT_NEEDED(k));
                    if (v > left) {
                        v = left;
                    }
                    S32(st, c + CAT_AVAILABLE(k)) = v;
                    left -= v;
                    break;
                }
            }
            if (left <= 0 || prio + 1 > 9) {
                break;
            }
        }
        /* @bin 0x004594B6 then rounds over the default shares 0x005DA44C for the categories
         * without a priority, while workers are left, at most +0x2B28 - 1 rounds */
        if (S32(st, c + WORKFORCE) > 1) {
            for (int32_t round = 1; left > 0;) {
                for (uint32_t p = DEFAULT_SHARES; p < DEFAULT_SHARES + 0x48; p += 8) {
                    int32_t k = S32(st, p);
                    if (S32(st, c + CAT_PRIORITY(k)) != 0) {
                        continue;
                    }
                    int32_t have = S32(st, c + CAT_AVAILABLE(k));
                    int32_t missing = S32(st, c + CAT_NEEDED(k)) - have;
                    if (missing <= 0) {
                        continue;
                    }
                    int32_t v = S32(st, p + 4);
                    if (v > left) {
                        v = left;
                    }
                    if (v > missing) {
                        v = missing;
                    }
                    left -= v;
                    S32(st, c + CAT_AVAILABLE(k)) = have + v;
                    if (left <= 0) {
                        break;
                    }
                }
                if (left <= 0) {
                    break;
                }
                round++;
                if (round >= S32(st, c + WORKFORCE)) {
                    break;
                }
            }
        }
        /* @bin 0x00459531 */
        S32(st, c + 0x2BF4) = S32(st, c + WORKFORCE);
        for (int k = 0; k < 9; k++) {
            S32(st, c + 0x2C04) += S32(st, c + CAT_NEEDED(k)) - S32(st, c + CAT_AVAILABLE(k));
        }
    }
    /* @bin 0x00459563 the unemployed and their percentage */
    int32_t idle = S32(st, c + WORKFORCE) - S32(st, c + 0x2BF4);
    S32(st, c + 0x2BF8) = idle;
    int32_t pct = calc_ratio_percent(idle, S32(st, c + WORKFORCE));
    S32(st, CURRENT_CITY(st) + 0x2BFC) = pct;
}

void labour_update(game_state *st)
{
    /* @bin 0x00459350 */
    int32_t short_before = S32(st, CURRENT_CITY(st) + 0x2C04);
    labour_categories_fill(st);
    uint32_t c = CURRENT_CITY(st);
    int32_t pct = S32(st, c + 0x2BFC);
    int32_t shown = S32(st, c + 0x2C00);
    /* @bin 0x0045937E the shown rate follows the real one down at once, up by 5 at most */
    if (pct < shown || pct - 5 < shown) {
        S32(st, c + 0x2C00) = pct;
    } else {
        S32(st, c + 0x2C00) = shown + 5;
    }
    if (S32(st, c + 0x2C00) > 100) {
        S32(st, c + 0x2C00) = 100;
    }
    /* @bin 0x004593B9 the first labour shortage, from the year 0x007848D8 on: message 0x88 */
    if (short_before == 0 && S32(st, c + 0x2C04) > 0 && S32(st, 0x0078D7BCu) >= S16(st, 0x007848D8u) &&
        S32(st, 0x00E95144u) <= 0) {
        S32(st, 0x00E95144u) = 6;
        GAME_CALL(st, 0x004E0CF0u, 0x88, 0, 0, 0, 0);
    }
}

void labour_building_shares(game_state *st)
{
    /* @bin 0x004595A0 (its first call 0x004CDE60(100, +0x2BD8) has no effect) */
    for (int32_t id = 1; id <= S32(st, BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b) != 1) {
            continue;
        }
        int32_t cat = S32(st, TYPE_CATEGORY + 0x14u * (uint32_t) (int32_t) S16(st, b + 0x10));
        if (cat == 0) {
            if ((int8_t) GAME_CALLT(st, BUILDING_MANAGER, 0x00488050u, id) != 0) {
                continue;
            }
        } else if (cat == 3) {
            if (S16(st, b + 0x38) != 0) {
                continue;
            }
        } else if (cat < 0) {
            continue;
        }
        S16(st, b + 0x1A) = 0;
        int32_t total = S32(st, CURRENT_CITY(st) + CATEGORIES + 8 + 0x14u * (uint32_t) cat);
        int16_t workers = S16(st, b + 0x18);
        if (workers != 0) {
            S16(st, b + 0x1A) = (int16_t) calc_ratio_percent(workers * 100, total);
        }
    }
}

int32_t city_age_group(game_state *st, int32_t player, int32_t group)
{
    /* @bin 0x004E92C0 the ten i16 of the age table (+0x25C4) of a decade */
    uint32_t a = CITY_ADDR(player) + 0x25C4u + 20u * (uint32_t) group;
    int32_t sum = 0;
    for (uint32_t i = 0; i < 10; i++) {
        sum += S16(st, a + 2 * i);
    }
    return sum;
}

int32_t city_working_age(game_state *st)
{
    /* @bin 0x004E9460 decades 2..4 of the current city */
    int32_t n = city_age_group(st, S32(st, CURRENT_PLAYER_ADDR), 2);
    n += city_age_group(st, S32(st, CURRENT_PLAYER_ADDR), 3);
    return n + city_age_group(st, S32(st, CURRENT_PLAYER_ADDR), 4);
}

void houses_access_ratios(game_state *st, uint32_t out0, uint32_t out1, uint32_t out2, uint32_t out3)
{
    /* @bin 0x004E7E60 shares (percent) of the people living in houses with +0x76, +0x78, +0x6E,
     * +0x75 set, written through the four pointers (in that order: out0, out1, out2, out3 get
     * +0x76, +0x78, +0x6E, +0x75) */
    int32_t all = 0, a = 0, b = 0, c = 0, d = 0;
    for (int32_t id = 1; id < 4000; id++) {
        uint32_t h = BUILDING_ADDR(id);
        if (U8(st, h) != 1 || U8(st, h + 5) == 0 || S16(st, h + 0x1C) == 0) {
            continue;
        }
        int32_t n = S16(st, h + 0x1C);
        all += n;
        if (U8(st, h + 0x76) != 0) {
            a += n;
        }
        if (U8(st, h + 0x78) != 0) {
            b += n;
        }
        if (U8(st, h + 0x75) != 0) {
            d += n;
        }
        if (U8(st, h + 0x6E) != 0) {
            c += n;
        }
    }
    P32(st, out0) = (uint32_t) calc_ratio_percent(a, all);
    P32(st, out1) = (uint32_t) calc_ratio_percent(b, all);
    P32(st, out3) = (uint32_t) calc_ratio_percent(d, all);
    P32(st, out2) = (uint32_t) calc_ratio_percent(c, all);
}

void city_migration_4e8f80(game_state *st, int32_t player)
{
    /* @bin 0x004E8F80 the migration rate +0x272C from the city's mood (+0x10: > 70: 100,
     * > 60: 75, >= 50: 50, > 40: 0, > 30: -10, > 20: -25, else -50); immigrants +0x26E8 and
     * emigrants +0x26EC of the day (12 % of the rate) every third day (+0x2734 / +0x2738); none
     * at 200000 people, nor in when more than 3 houses lack space (+0x4644 + +0x4634) */
    uint32_t c = CITY_ADDR(player);
    int32_t mood = S32(st, c + 0x10);
    int32_t before = S32(st, c + 0x272C);
    int32_t rate = mood > 0x46 ? 100 : mood > 0x3C ? 0x4B : mood >= 0x32 ? 0x32 : mood > 0x28 ? 0
                   : mood > 0x1E ? -10 : mood <= 0x14 ? -50 : -25;
    S32(st, c + 0x272C) = rate;
    S32(st, c + 0x26EC) = 0;
    S32(st, c + 0x26E8) = 0;
    uint32_t cur = CURRENT_CITY(st);
    if (S32(st, cur + 0x24) >= 200000) {
        S32(st, c + 0x272C) = 0;
        return;
    }
    if (S32(st, cur + 0x4644) + S32(st, cur + 0x4634) > 3 && S32(st, c + 0x272C) > 0) {
        S32(st, c + 0x272C) = 0;
        return;
    }
    rate = S32(st, c + 0x272C);
    if (rate > 0) {
        if (S32(st, c + 0x2738) != 0) {
            S32(st, c + 0x2738) -= 1;
            return;
        }
        S32(st, c + 0x26E8) = calc_percentage(0xC, rate);
        S32(st, c + 0x2734) = 2;
    } else if (rate < 0) {
        if (S32(st, c + 0x2734) != 0) {
            S32(st, c + 0x2734) -= 1;
            return;
        }
        if (S32(st, c + 0x24) <= 100) {
            return;
        }
        S32(st, c + 0x26EC) = calc_percentage(0xC, -rate);
        S32(st, c + 0x2738) = 2;
    }
    if (before != S32(st, c + 0x272C)) {
        GAME_CALL(st, 0x004E2FB0u, 0);
    }
}

#define HOUSES 0x0078E178u      /* i16 list of the used houses (0x004583F0) */
#define HOUSES_COUNT 0x0078D83Cu
#define HOUSES_INDEX 0x0078D840u

void immigrant_send(game_state *st, int32_t id, int32_t n)
{
    /* @bin 0x00458D00 an immigrant figure (type 1) from the city's entry (+0x2778) to the house:
     * +0x2E of the house, +0x38 := 8, +0x36 := 10 + (+0x4D & 0x7F), +0x5C the house, +0x64 n */
    int32_t player = S32(st, CURRENT_PLAYER_ADDR);
    uint32_t c = CITY_ADDR(player);
    int32_t f = GAME_CALL(st, 0x004B4800u, player, 1, S16(st, c + 0x2778), S16(st, c + 0x277A), 0);
    uint32_t b = BUILDING_ADDR(id), fa = FIGURE_ADDR(f);
    S16(st, b + 0x2E) = (int16_t) f;
    uint8_t k = U8(st, b + 0x4D) & 0x7F;
    U8(st, fa + 0x38) = 8;
    S16(st, fa + 0x5C) = (int16_t) id;
    S16(st, fa + 0x36) = (int16_t) (k + 10);
    U8(st, fa + 0x64) = (uint8_t) n;
}

void immigrants_place(game_state *st, int32_t n)
{
    /* @bin 0x00458950 the immigrants go to houses with room (+0x20 > 0): those with room for 8
     * or more (+0x1E) and no immigrant on the way first, by 4; then any with room, by their room;
     * the city's counters +0x2720, +0x273C and (none placed) +0x2728 */
    int32_t left = n;
    if (S32(st, HOUSES_COUNT) > 0) {
        if (left > 0) {
            /* @bin 0x00458970 the immigrants on the way (+0x2E) must be alive */
            for (int32_t i = 0; i < S32(st, HOUSES_COUNT); i++) {
                uint32_t b = BUILDING_ADDR(S16(st, HOUSES + 2u * (uint32_t) i));
                int16_t f = S16(st, b + 0x2E);
                if (f != 0 && U8(st, FIGURE_ADDR(f) + 0xE) != 1) {
                    S16(st, b + 0x2E) = 0;
                }
            }
        }
    }
    for (int32_t i = 0; i < S32(st, HOUSES_COUNT) && left > 0;) {
        int32_t id = S16(st, HOUSES + 2u * (uint32_t) i);
        uint32_t b = BUILDING_ADDR(id);
        S32(st, HOUSES_INDEX) = ++i;
        if (S16(st, b + 0x20) > 0 && S16(st, b + 0x1E) >= 8 && S16(st, b + 0x2E) == 0) {
            int32_t k = left > 4 ? 4 : left;
            immigrant_send(st, id, k);
            left -= k;
            i = S32(st, HOUSES_INDEX);
        }
    }
    S32(st, HOUSES_INDEX) = 0;
    for (int32_t i = 0; i < S32(st, HOUSES_COUNT) && left > 0;) {
        int32_t id = S16(st, HOUSES + 2u * (uint32_t) i);
        uint32_t b = BUILDING_ADDR(id);
        S32(st, HOUSES_INDEX) = ++i;
        int16_t room = S16(st, b + 0x1E);
        if (S16(st, b + 0x20) > 0 && room > 0 && S16(st, b + 0x2E) == 0) {
            int32_t k = left > room ? room : left;
            immigrant_send(st, id, k);
            left -= k;
            i = S32(st, HOUSES_INDEX);
        }
    }
    uint32_t c = CURRENT_CITY(st);
    S32(st, c + 0x2720) += n - left;
    S32(st, c + 0x273C) += S32(st, c + 0x2720);
    if (left != 0 && n == left) {
        S32(st, c + 0x2728) += left;
    }
}

void emigrant_send(game_state *st, int32_t id, int32_t n)
{
    /* @bin 0x00458D80 n people leave the house (0x004E90F0(player, -n); emptied: 0x0046CF40) as
     * an emigrant figure (type 2) */
    GAME_CALL(st, 0x004E90F0u, S32(st, CURRENT_PLAYER_ADDR), -n);
    uint32_t b = BUILDING_ADDR(id);
    int16_t people = S16(st, b + 0x1C);
    if (n >= people) {
        S16(st, b + 0x1C) = 0;
        GAME_CALL(st, 0x0046CF40u, id);
    } else {
        S16(st, b + 0x1C) = (int16_t) (people - n);
    }
    int32_t f = GAME_CALL(st, 0x004B4800u, S32(st, CURRENT_PLAYER_ADDR), 2, S16(st, b + 6), S16(st, b + 8), 0);
    uint32_t fa = FIGURE_ADDR(f);
    U8(st, fa + 0x38) = 8;
    S16(st, fa + 0x36) = 0;
    U8(st, fa + 0x64) = (uint8_t) n;
}

void emigrants_leave(game_state *st, int32_t n)
{
    /* @bin 0x00458B60 the emigrants leave the lowest houses first (+0x12 from 0 to 9), at most 4
     * per house; +0x2724 counts them */
    int32_t left = n;
    for (int32_t level = 0; left > 0 && level < 10; level++) {
        S32(st, HOUSES_INDEX) = 0;
        for (int32_t i = 0; i < S32(st, HOUSES_COUNT) && left > 0;) {
            int32_t id = S16(st, HOUSES + 2u * (uint32_t) i);
            uint32_t b = BUILDING_ADDR(id);
            S32(st, HOUSES_INDEX) = ++i;
            int16_t people = S16(st, b + 0x1C);
            if (people > 0 && S16(st, b + 0x12) == level) {
                int32_t k = people < 4 ? people : 4;
                if (left > k) {
                    emigrant_send(st, id, k);
                    left -= k;
                } else {
                    emigrant_send(st, id, left);
                    left = 0;
                }
                i = S32(st, HOUSES_INDEX);
            }
        }
    }
    S32(st, CURRENT_CITY(st) + 0x2724) += n - left;
}

void population_workforce_458880(game_state *st)
{
    /* @bin 0x00458880 the houses listed at 0x0078E178 (index 0x0078D840): people in houses of
     * level (+0x12) below 0xE against all (+0x2B20, percent); the workforce +0x2B28 from
     * 0x004E9460 (60 percent, +0x2B24) by that percentage */
    int32_t n = S32(st, 0x0078D83Cu);
    int32_t low = 0, high = 0;
    S32(st, 0x0078D840u) = 0;
    if (n > 0) {
        S32(st, 0x0078D840u) = n;
        for (int32_t i = 0; i < n; i++) {
            uint32_t b = BUILDING_ADDR(S16(st, 0x0078E178u + 2u * (uint32_t) i));
            int16_t v = S16(st, b + 0x1C);
            if (v <= 0) {
                continue;
            }
            if (S16(st, b + 0x12) >= 0xE) {
                high += v;
            } else {
                low += v;
            }
        }
    }
    S32(st, CURRENT_CITY(st) + 0x2B20) = calc_ratio_percent(low, high + low);
    int32_t v = city_working_age(st);
    S32(st, CURRENT_CITY(st) + 0x2B24) = v;
    v = calc_percentage(v, 0x3C);
    uint32_t c = CURRENT_CITY(st);
    S32(st, c + 0x2B24) = v;
    S32(st, CURRENT_CITY(st) + WORKFORCE) = calc_percentage(v, S32(st, c + 0x2B20));
}

void city_population_month_4e87a0(game_state *st, int32_t player)
{
    /* @bin 0x004E87A0 */
    uint32_t c = CITY_ADDR(player);
    S32(st, c + 0x2714) = 0;
    S32(st, c + 0x28) = S32(st, c + 0x24);
    S32(st, c + 0x24) = 0;
    for (uint32_t i = 0; i < 100; i++) {
        S32(st, c + 0x24) += S16(st, c + 0x25C4 + 2 * i);
    }
    if (S32(st, c + 0x24) > S32(st, c + 0x469C)) {
        S32(st, c + 0x469C) = S32(st, c + 0x24);
    }
    S32(st, c + 0x26E4) = 0;
    S32(st, c + 0x2704) += S32(st, c + 0x24);
    S32(st, c + 0x2710) += 1;
    S32(st, c + 0x2700) = S32(st, c + 0x2704) / S32(st, c + 0x2710);
}

int32_t houses_remove_people(game_state *st, int32_t player, int32_t n)
{
    /* @bin 0x00458F00 one person less in each house in turn from the cursor +0x271C (i16,
     * houses 1..3999 cycling), at most 0x3E7F steps; returns how many were removed */
    uint32_t c = CITY_ADDR(player);
    int32_t cursor = S16(st, c + 0x271C), done = 0;
    for (int32_t step = 1; step < 0x3E80 && done < n; step++) {
        if (++cursor >= 0xFA0) {
            cursor = 1;
        }
        uint32_t b = BUILDING_ADDR(cursor);
        if (U8(st, b) != 1 || U8(st, b + 5) == 0) {
            continue;
        }
        int16_t people = S16(st, b + 0x1C);
        S16(st, c + 0x271C) = (int16_t) cursor;
        if (people > 0) {
            done++;
            S16(st, b + 0x1C) = (int16_t) (people - 1);
        }
    }
    return done;
}

int32_t houses_add_people(game_state *st, int32_t player, int32_t n)
{
    /* @bin 0x00458E20 one person more in each house with room in turn from the cursor +0x2718
     * (houses 1..3999 cycling, 3999 steps at most); returns how many were added */
    uint32_t c = CITY_ADDR(player);
    int32_t cursor = S32(st, c + 0x2718), done = 0;
    for (int32_t step = 1; step < 0xFA0 && done < n; step++) {
        if (++cursor >= 0xFA0) {
            cursor = 1;
        }
        uint32_t b = BUILDING_ADDR(cursor);
        if (U8(st, b) != 1 || U8(st, b + 5) == 0 || S16(st, b + 0x20) <= 0) {
            continue;
        }
        int16_t people = S16(st, b + 0x1C);
        if (people <= 0) {
            continue;
        }
        S32(st, c + 0x2718) = cursor;
        int32_t cap = S32(st, 0x0070CE14u + 0x58u * (uint32_t) (int32_t) S16(st, b + 0x12));
        if (U8(st, b + 4) != 0) {
            cap = (int32_t) ((uint32_t) cap << 2);
        }
        if (cap - people > 0) {
            done++;
            people++;
            S16(st, b + 0x1C) = people;
            S16(st, b + 0x1E) = (int16_t) (cap - people);
        }
    }
    return done;
}

void city_age_group_remove(game_state *st, int32_t player, int32_t group, int32_t n)
{
    /* @bin 0x004E92F0 n people less in the ages of a decade, one per age in turn, until ten
     * ages in a row are empty */
    uint32_t a = 0x00EA3E34u + 2u * (uint32_t) (player * 0x24EC + group * 10);
    for (int32_t i = 0, empty = 0; n != 0;) {
        int16_t v = S16(st, a + 2u * (uint32_t) i);
        if (v > 0) {
            S16(st, a + 2u * (uint32_t) i) = (int16_t) (v - 1);
            n--;
            empty = 0;
        } else {
            empty++;
        }
        if (++i >= 10) {
            i = 0;
        }
        if (empty >= 10) {
            break;
        }
    }
}

void city_deaths_4e8650(game_state *st)
{
    /* @bin 0x004E8650 the monthly ageing: the age table (100 i16 at +0x25C4) moves up one year;
     * the people past 99 and, per decade, a percentage (table 0x005E49D4 by decade and health
     * +0x18 / 10) die: they leave houses (0x00458F00) and the age table; +0x26E0 counts them */
    int32_t player = S32(st, CURRENT_PLAYER_ADDR);
    uint32_t c = CITY_ADDR(player);
    int32_t carry = S16(st, c + 0x268A);
    for (uint32_t i = 99; i >= 1; i--) {
        S16(st, c + 0x25C4 + 2 * i) = S16(st, c + 0x25C4 + 2 * (i - 1));
    }
    S16(st, c + 0x25C4) = 0;
    S32(st, c + 0x26E0) = 0;
    for (int32_t group = 9; group >= 0; group--) {
        int32_t k = city_age_group(st, player, group);
        uint32_t cur = CURRENT_CITY(st);
        int32_t pct = S32(st, 0x005E49D4u + 4u * (uint32_t) (group + 10 * (S32(st, cur + 0x18) / 10)));
        int32_t dead = houses_remove_people(st, S32(st, CURRENT_PLAYER_ADDR), calc_percentage(k, pct) + carry);
        player = S32(st, CURRENT_PLAYER_ADDR);
        city_age_group_remove(st, player, group, dead);
        S32(st, CURRENT_CITY(st) + 0x26E0) += dead;
        carry = 0;
        player = S32(st, CURRENT_PLAYER_ADDR);
    }
}

void city_births_4e8720(game_state *st)
{
    /* @bin 0x004E8720 the monthly births: per decade 9..0, a percentage (table 0x005E49AC) of
     * its people are born into houses with room (0x00458E20) at age 0; +0x26DC counts them */
    S32(st, CURRENT_CITY(st) + 0x26DC) = 0;
    int32_t group = 9;
    for (uint32_t t = 0x005E49D0u; t >= 0x005E49ACu; t -= 4, group--) {
        int32_t k = city_age_group(st, S32(st, CURRENT_PLAYER_ADDR), group);
        int32_t born = houses_add_people(st, S32(st, CURRENT_PLAYER_ADDR), calc_percentage(k, S32(st, t)));
        uint32_t c = CURRENT_CITY(st);
        S16(st, c + 0x25C4) = (int16_t) (S16(st, c + 0x25C4) + born);
        S32(st, c + 0x26DC) += born;
    }
}

void day_population_458540(game_state *st)
{
    /* @bin 0x00458540 day step 0x17: immigrants (+0x26E8, by four or with the remainder
     * +0x26F4: 0x00458950) and emigrants (+0x26EC, +0x26F0: 0x00458B60); the monthly ageing when
     * +0x2714 is set; the workforce; the population milestone messages 0x65..0x6D */
    city_migration_4e8f80(st, S32(st, CURRENT_PLAYER_ADDR));
    uint32_t c = CURRENT_CITY(st);
    S32(st, c + 0x2720) = 0;
    S32(st, c + 0x2724) = 0;
    S32(st, c + 0x2728) = 0;
    int32_t n = S32(st, c + 0x26E8);
    if (n > 0) {
        if (n >= 4) {
            immigrants_place(st, n);
        } else if ((n += S32(st, c + 0x26F4)) >= 4) {
            immigrants_place(st, n);
            S32(st, CURRENT_CITY(st) + 0x26F4) = 0;
        } else {
            S32(st, c + 0x26F4) = n;
        }
    }
    c = CURRENT_CITY(st);
    n = S32(st, c + 0x26EC);
    if (n > 0) {
        if (n >= 4) {
            emigrants_leave(st, n);
        } else if ((n += S32(st, c + 0x26F0)) >= 4) {
            emigrants_leave(st, n);
            S32(st, CURRENT_CITY(st) + 0x26F0) = 0;
        } else {
            S32(st, c + 0x26F0) = n;
        }
    }
    c = CURRENT_CITY(st);
    S32(st, c + 0x26E8) = 0;
    S32(st, c + 0x26EC) = 0;
    if (S32(st, c + 0x2714) != 0) {
        city_deaths_4e8650(st);
        city_births_4e8720(st);
        city_population_month_4e87a0(st, S32(st, CURRENT_PLAYER_ADDR));
    }
    population_workforce_458880(st);
    /* @bin 0x00458649 */
    static const struct {
        int32_t population;
        uint32_t flag;
        int32_t message;
    } milestones[] = {
        {500, 0x00E951C5u, 0x65}, {1000, 0x00E951C6u, 0x66}, {2000, 0x00E951C7u, 0x67},
        {3000, 0x00E951C8u, 0x68}, {5000, 0x00E951C9u, 0x69}, {10000, 0x00E951CAu, 0x6A},
        {15000, 0x00E951CBu, 0x6B}, {20000, 0x00E951CCu, 0x6C}, {25000, 0x00E951CDu, 0x6D},
    };
    for (unsigned i = 0; i < sizeof(milestones) / sizeof(milestones[0]); i++) {
        if (S32(st, CURRENT_CITY(st) + 0x24) >= milestones[i].population && U8(st, milestones[i].flag) == 0) {
            S32(st, 0x00E950DCu) = 1;
            GAME_CALL(st, 0x004E0CF0u, milestones[i].message, 0, 0, 0, 0);
            U8(st, milestones[i].flag) = 1;
        }
    }
}

#define VOID0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
VOID0(run_categories_fill, labour_categories_fill)
VOID0(run_update, labour_update)
VOID0(run_building_shares, labour_building_shares)
VOID0(run_workforce, population_workforce_458880)
VOID0(run_population, day_population_458540)

static int32_t run_age_group(game_state *st, const int32_t *a)
{
    return city_age_group(st, a[0], a[1]);
}

static int32_t run_working_age(game_state *st, const int32_t *a)
{
    (void) a;
    return city_working_age(st);
}

static int32_t run_access_ratios(game_state *st, const int32_t *a)
{
    houses_access_ratios(st, (uint32_t) a[0], (uint32_t) a[1], (uint32_t) a[2], (uint32_t) a[3]);
    return 0;
}

static int32_t run_migration(game_state *st, const int32_t *a)
{
    city_migration_4e8f80(st, a[0]);
    return 0;
}

static int32_t run_immigrant_send(game_state *st, const int32_t *a)
{
    immigrant_send(st, a[0], a[1]);
    return 0;
}

static int32_t run_immigrants_place(game_state *st, const int32_t *a)
{
    immigrants_place(st, a[0]);
    return 0;
}

static int32_t run_emigrant_send(game_state *st, const int32_t *a)
{
    emigrant_send(st, a[0], a[1]);
    return 0;
}

static int32_t run_emigrants_leave(game_state *st, const int32_t *a)
{
    emigrants_leave(st, a[0]);
    return 0;
}

static int32_t run_remove_people(game_state *st, const int32_t *a)
{
    return houses_remove_people(st, a[0], a[1]);
}

static int32_t run_add_people(game_state *st, const int32_t *a)
{
    return houses_add_people(st, a[0], a[1]);
}

static int32_t run_group_remove(game_state *st, const int32_t *a)
{
    city_age_group_remove(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_deaths(game_state *st, const int32_t *a)
{
    (void) a;
    city_deaths_4e8650(st);
    return 0;
}

static int32_t run_births(game_state *st, const int32_t *a)
{
    (void) a;
    city_births_4e8720(st);
    return 0;
}

static int32_t run_month_4e87a0(game_state *st, const int32_t *a)
{
    city_population_month_4e87a0(st, a[0]);
    return 0;
}

const ported_function day_labour_ports[] = {
    {0x00458F00u, "houses_remove_people", 2, 32, 0, run_remove_people, 0},
    {0x00458E20u, "houses_add_people", 2, 32, 0, run_add_people, 0},
    {0x004E92F0u, "city_age_group_remove", 3, 0, 0, run_group_remove, 0},
    {0x004E8650u, "city_deaths_4e8650", 0, 0, 0, run_deaths, 0},
    {0x004E8720u, "city_births_4e8720", 0, 0, 0, run_births, 0},
    {0x00458D00u, "immigrant_send", 2, 0, 0, run_immigrant_send, 0},
    {0x00458950u, "immigrants_place", 1, 0, 0, run_immigrants_place, 0},
    {0x00458D80u, "emigrant_send", 2, 0, 0, run_emigrant_send, 0},
    {0x00458B60u, "emigrants_leave", 1, 0, 0, run_emigrants_leave, 0},
    {0x004E92C0u, "city_age_group", 2, 32, 0, run_age_group, 0},
    {0x004E9460u, "city_working_age", 0, 32, 0, run_working_age, 0},
    {0x004E7E60u, "houses_access_ratios", 4, 0, 0, run_access_ratios, 0},
    {0x004E8F80u, "city_migration_4e8f80", 1, 0, 0, run_migration, 0},
    {0x00458880u, "population_workforce_458880", 0, 0, 0, run_workforce, 0},
    {0x004E87A0u, "city_population_month_4e87a0", 1, 0, 0, run_month_4e87a0, 0},
    {0x00458540u, "day_population_458540", 0, 0, 0, run_population, 0},
    {0x00459400u, "labour_categories_fill", 0, 0, 0, run_categories_fill, 0},
    {0x00459350u, "labour_update", 0, 0, 0, run_update, 0},
    {0x004595A0u, "labour_building_shares", 0, 0, 0, run_building_shares, 0},
};
const int day_labour_ports_count = sizeof(day_labour_ports) / sizeof(day_labour_ports[0]);
