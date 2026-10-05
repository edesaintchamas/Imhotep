/* Imhotep - storage and delivery destinations under the cart and porter figure actions
 * (package figx, part b).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Records: docs/formats/save.md §5 (figures chunk 21, buildings chunk 32, city data chunk 28). */
#include "figure/helpers_xb.h"

#include "building/building_spawn.h"
#include "building/day_access.h"
#include "city/day_buildings.h"
#include "city/goods.h"
#include "sim/call.h"
#include "sim/math.h"
#include "state/ported.h"
#include "state/records.h"

int32_t storage_fill_level(int32_t amount); /* @bin 0x0045B490 (core/building/building_misc.c) */

#define B(id) BUILDING_ADDR(id)
#define CUR_PLAYER(st) S32(st, CURRENT_PLAYER_ADDR)
/* @bin 0x007900C0 storage settings: records of 0xC4 bytes by the building's +0xC1; +2 + good: a
 * byte per good, +0x2C + 2 good / +0x74 + 2 good: i16 limits */
#define STORE_REC(k) (0x007900C0u + 0xC4u * (uint32_t) (k))
/* @bin 0x0077DE04 empire cities, 0x6A bytes each: +0 route index, +2 + good, +0x26 + good bytes */
#define ECITY(c) (0x0077DE04u + 0x6Au * (uint32_t) (c))
/* @bin 0x0077646C / 0x0077592C route limits / route counts, i32 [route * 36 + good] */
#define ROUTE_LIMIT(r, g) (0x0077646Cu + 4u * (36u * (uint32_t) (r) + (uint32_t) (g)))
#define ROUTE_DONE(r, g) (0x0077592Cu + 4u * (36u * (uint32_t) (r) + (uint32_t) (g)))

static int is_storage(int16_t type)
{
    return type == 0x48 || type == 0xC1; /* @bin 0x0045CF6B */
}

int32_t trade_good_open_24(game_state *st, int32_t player, int32_t city, int32_t good)
{
    /* @bin 0x004434E0 the city's status of `good` (+0x2826 + 2 good) is 2 or 4, good in 0..0x23
     * but not 0x1B, the route count is below its adjusted limit (city 0 not checked), the stock
     * (+0x48F0 + 4 good) above the threshold (status 2: i16 +0x286E * 100, else 0x00775810[good])
     * and, for city != 0, the empire city's byte +2 + good set */
    uint32_t c = CITY_ADDR(player);
    int16_t status = S16(st, c + 0x2826u + 2u * (uint32_t) good);
    if (status != 2 && status != 4) {
        return 0;
    }
    if (good == 0x1B || good < 0 || good >= 0x24) {
        return 0;
    }
    int32_t limit = trade_limit_adjusted(st, S32(st, ROUTE_LIMIT(U8(st, ECITY(city)), good)));
    if (city != 0 && S32(st, ROUTE_DONE(U8(st, ECITY(city)), good)) >= limit) {
        return 0;
    }
    int32_t stock = S32(st, c + 0x48F0u + 4u * (uint32_t) good);
    if (S16(st, c + 0x2826u + 2u * (uint32_t) good) == 2) {
        if (!(stock > S16(st, c + 0x286Eu + 2u * (uint32_t) good) * 100)) {
            return 0;
        }
    } else if (!(stock > S32(st, 0x00775810u + 4u * (uint32_t) good))) {
        return 0;
    }
    if (city != 0 && U8(st, ECITY(city) + 2u + (uint32_t) good) == 0) {
        return 0;
    }
    return 1;
}

int32_t trade_good_open_13(game_state *st, int32_t player, int32_t city, int32_t good)
{
    /* @bin 0x004435F0 the current player's +0xC above -5000, the status of `good` 1 or 3, good in
     * 1..0x23 but not 0x1B, the empire city's byte +0x26 + good set, the route count below its
     * adjusted limit and the stock below the threshold (status 1: i16 +0x286E * 100, else
     * 0x00775810[good]) */
    if (S32(st, CURRENT_CITY(st) + 0x0C) <= -5000) {
        return 0;
    }
    uint32_t c = CITY_ADDR(player);
    int16_t status = S16(st, c + 0x2826u + 2u * (uint32_t) good);
    if (status != 1 && status != 3) {
        return 0;
    }
    if (good == 0x1B || good < 0 || good >= 0x24) {
        return 0;
    }
    if (U8(st, ECITY(city) + 0x26u + (uint32_t) good) == 0) {
        return 0;
    }
    int32_t limit = trade_limit_adjusted(st, S32(st, ROUTE_LIMIT(U8(st, ECITY(city)), good)));
    if (S32(st, ROUTE_DONE(U8(st, ECITY(city)), good)) >= limit) {
        return 0;
    }
    int32_t stock = S32(st, c + 0x48F0u + 4u * (uint32_t) good);
    if (good < 1) {
        return 0;
    }
    if (S16(st, c + 0x2826u + 2u * (uint32_t) good) == 1) {
        return stock < S16(st, c + 0x286Eu + 2u * (uint32_t) good) * 100;
    }
    return stock < S32(st, 0x00775810u + 4u * (uint32_t) good);
}

int32_t storage_over_limit(game_state *st, int32_t bid, int32_t amount, int32_t good)
{
    /* @bin 0x0045B230 the limit of `good` in the building's settings (+0x74 + 2 good when its byte
     * +2 + good is set, else +0x2C + 2 good) is below the stock (0x0045D510) + amount
     * (only al is set: ret_bits 8) */
    uint32_t r = STORE_REC(U8(st, B(bid) + 0xC1));
    int32_t limit = U8(st, r + 2u + (uint32_t) good) != 0 ? S16(st, r + 0x74u + 2u * (uint32_t) good)
                                                         : S16(st, r + 0x2Cu + 2u * (uint32_t) good);
    return limit < building_parts_stock(st, bid, good) + amount;
}

int32_t storage_add(game_state *st, int32_t player, int32_t bid, int32_t good, int32_t amount)
{
    /* @bin 0x0045B2A0 puts `amount` of `good` in the storage building's parts, one unit at a time
     * in the part (chain +0x3A from the main part, 8 long) holding `good` or nothing with the
     * largest stock below 400; capped by the settings limit; counts it in the player's city data
     * (+0x4860 - 1, +0x48F0 + 1 per unit) and sets the part's image (0x00B7482C by its +0x0C).
     * Returns the units put (<= 0 when none fit), 0 when no part is found */
    int32_t main = building_main_part(st, bid);
    uint32_t m = B(main);
    uint8_t k = U8(st, m + 0xC1);
    if (S16(st, m + 0x40) <= 0 || S16(st, m + 0x4A) != 0) {
        return 0;
    }
    if ((uint8_t) storage_over_limit(st, main, 0, good) != 0) {
        return 0;
    }
    uint32_t r = STORE_REC(k);
    int32_t limit = U8(st, r + 2u + (uint32_t) good) != 0 ? S16(st, r + 0x74u + 2u * (uint32_t) good)
                                                         : S16(st, r + 0x2Cu + 2u * (uint32_t) good);
    int32_t over = 0;
    if ((uint8_t) storage_over_limit(st, main, amount, good) != 0) {
        over = building_parts_stock(st, main, good) + (amount - limit);
    }
    int32_t n = amount - over;
    if (n <= 0) {
        return n;
    }
    int32_t at = bid;
    for (int32_t i = 0; i < n; i++) {
        int32_t best = -1, part = 0;
        int32_t cur = building_main_part(st, at);
        for (int j = 0; j < 8; j++) {
            cur = S16(st, B(cur) + 0x3A);
            if (cur <= 0) {
                return 0;
            }
            int16_t g = S16(st, B(cur) + 0x12);
            if (g == 0 || g == good) {
                int16_t have = S16(st, B(cur) + 0x3C);
                if (have < 0x190 && have > best) {
                    best = have;
                    part = cur;
                }
            }
        }
        if (part <= 0) {
            return 0;
        }
        at = part;
        uint32_t idx = 4u * ((uint32_t) player * 0x1276u + (uint32_t) good);
        S32(st, 0x00EA60D0u + idx) -= 1;
        S32(st, 0x00EA6160u + idx) += 1;
        uint32_t p = B(part);
        S16(st, p + 0x12) = (int16_t) good;
        S16(st, p + 0x3C) = (int16_t) (S16(st, p + 0x3C) + 1);
        int16_t have = S16(st, p + 0x3C);
        int32_t tile = S32(st, p + 0x0C);
        /* @bin 0x0045B436 goods below 0x1F: images from 0x00F566BC, the others from 0x00F57204 */
        int32_t base = good < 0x1F ? S32(st, 0x00F566BCu) + good * 4 - 4 : S32(st, 0x00F57204u) + good * 4 - 0x7C;
        S32(st, 0x00B7482Cu + 4u * (uint32_t) tile) = storage_fill_level(have) + base;
    }
    return n;
}

int32_t storage_room_loads(game_state *st, int32_t bid, int32_t good, int32_t amount)
{
    /* @bin 0x004454D0 how many loads of 100 (at most amount / 100) of `good` the storage building
     * takes: 0 when over its settings limit; the room is 3200 less the parts' stock (+0x3C, chain
     * +0x3A, 8 parts); capped by what the current player's city wants (stock +0x48F0 below the
     * threshold of trade_good_open_13) */
    if (!is_storage(S16(st, B(bid) + 0x10))) {
        return 0;
    }
    building_parts_stock(st, bid, good); /* result unused */
    if ((uint8_t) storage_over_limit(st, bid, 0, good) != 0) {
        return 0;
    }
    if ((uint8_t) storage_over_limit(st, bid, amount, good) != 0) {
        return 0;
    }
    int32_t sum = 0;
    int32_t cur = bid;
    for (int i = 0; i < 8; i++) {
        cur = S16(st, B(cur) + 0x3A);
        int16_t have = S16(st, B(cur) + 0x3C);
        sum += have > 0 ? have : 0;
    }
    int32_t room = 0xC80 - sum;
    int32_t q;
    if (room > amount) {
        q = amount / 100;
    } else {
        if (room < 100) {
            return 0;
        }
        q = room / 100;
    }
    uint32_t c = CURRENT_CITY(st);
    int32_t lim;
    if (S16(st, c + 0x2826u + 2u * (uint32_t) good) == 1) {
        lim = S16(st, c + 0x286Eu + 2u * (uint32_t) good) * 100;
    } else {
        lim = S32(st, 0x00775810u + 4u * (uint32_t) good);
    }
    int32_t stock = S32(st, c + 0x48F0u + 4u * (uint32_t) good);
    if (stock + q * 100 < lim) {
        return q;
    }
    if (stock >= lim) {
        return 0;
    }
    return (lim - stock) / 100;
}

int32_t storage_trades_24(game_state *st, int32_t fid, int32_t player, int32_t bid, int32_t city)
{
    /* @bin 0x0045CF50 a storage building whose part chain (+0x3A, 8 steps) holds a good (+0x12,
     * stock +0x3C > 0) with trade_good_open_24 and a total stock (0x0045D510) of at least 100,
     * for a figure whose +0x6F is below 8 */
    int16_t type = S16(st, B(bid) + 0x10);
    if (!is_storage(type)) {
        return 0;
    }
    if (U8(st, FIGURE_ADDR(fid) + 0x6F) >= 8) {
        return 0;
    }
    int32_t cur = bid;
    for (int i = 0; i < 8; i++) {
        cur = S16(st, B(cur) + 0x3A);
        if (cur <= 0 || S16(st, B(cur) + 0x3C) <= 0) {
            continue;
        }
        if (trade_good_open_24(st, player, city, S16(st, B(cur) + 0x12)) == 0) {
            continue;
        }
        if (building_parts_stock(st, bid, S16(st, B(cur) + 0x12)) >= 100) {
            return 1;
        }
    }
    return 0;
}

int32_t storage_trades_13(game_state *st, int32_t fid, int32_t player, int32_t bid, int32_t city)
{
    /* @bin 0x0045D180 a storage building that can take a good with trade_good_open_13, for a
     * figure whose +0x74 is below 8: some good whose settings byte is not 1 or 3 and not over the
     * limit is open; the city's rotating good (+0x42C4) is advanced to one that is (settings byte
     * not 1); then a part (+0x3A, 8 steps) below 400 that is empty or holds an open good */
    uint32_t b = B(bid);
    int16_t type = S16(st, b + 0x10);
    if (!is_storage(type)) {
        return 0;
    }
    calc_ratio_percent(S16(st, b + 0x40), S32(st, 0x00705EB8u + 40u * (uint32_t) (int32_t) type)); /* unused */
    if (S16(st, FIGURE_ADDR(fid) + 0x74) >= 8) {
        return 0;
    }
    uint32_t r = STORE_REC(U8(st, b + 0xC1));
    int count = 0;
    for (int32_t good = 1; good < 0x24; good++) {
        uint8_t fl = U8(st, r + 2u + (uint32_t) good);
        if (fl == 1 || fl == 3) {
            continue;
        }
        if ((uint8_t) storage_over_limit(st, bid, 0, good) != 0) {
            continue;
        }
        if ((uint8_t) storage_over_limit(st, bid, 100, good) != 0) {
            continue;
        }
        if (trade_good_open_13(st, player, city, good) != 0) {
            count++;
        }
    }
    if (count <= 0) {
        return 0;
    }
    uint32_t c = CITY_ADDR(player);
    int32_t ok = trade_good_open_13(st, player, city, S32(st, c + 0x42C4));
    if (U8(st, r + 2u + (uint32_t) S32(st, CURRENT_CITY(st) + 0x42C4)) == 1 || ok == 0) {
        int found = 0;
        for (int i = 1; i < 0x24; i++) {
            int32_t v = S32(st, c + 0x42C4) + 1;
            S32(st, c + 0x42C4) = v;
            if (v >= 0x24) {
                S32(st, c + 0x42C4) = 1;
            }
            if (U8(st, r + 2u + (uint32_t) S32(st, CURRENT_CITY(st) + 0x42C4)) != 1 &&
                trade_good_open_13(st, player, city, S32(st, c + 0x42C4)) != 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            return 0;
        }
    }
    int32_t cur = bid;
    for (int i = 0; i < 8; i++) {
        cur = S16(st, B(cur) + 0x3A);
        if (cur <= 0) {
            continue;
        }
        int16_t have = S16(st, B(cur) + 0x3C);
        if (have >= 0x190) {
            continue;
        }
        if (have == 0) {
            return 1;
        }
        if (trade_good_open_13(st, player, city, S16(st, B(cur) + 0x12)) != 0) {
            return 1;
        }
    }
    return 0;
}

void city_good_value_move(game_state *st, int32_t player, int32_t unused, int32_t good)
{
    /* @bin 0x0045D4D0 the player's city +0xC loses the good's value (0x005D2C48 + 8 good) and
     * +0x2C6C gains it */
    (void) unused;
    uint32_t c = CITY_ADDR(player);
    int32_t v = S32(st, 0x005D2C48u + 8u * (uint32_t) good);
    S32(st, c + 0x0C) -= v;
    S32(st, c + 0x2C6C) += v;
}

int32_t storage_next_good_13(game_state *st, int32_t player, int32_t bid, int32_t city)
{
    /* @bin 0x0045D370 the next good (rotating +0x42C4 of the player's city, then probing with
     * +0x42C8) with trade_good_open_13 that the storage building takes a load of (0x0045B2A0 by
     * 100 returns 100); its value is moved (0x0045D4D0); returns that good, else 0 */
    if (!is_storage(S16(st, B(bid) + 0x10))) {
        return 0;
    }
    uint32_t c = CITY_ADDR(player);
    int found = 0;
    for (int i = 1; i < 0x24; i++) {
        if (trade_good_open_13(st, player, city, S32(st, c + 0x42C4)) != 0) {
            found = 1;
            break;
        }
        int32_t v = S32(st, c + 0x42C4) + 1;
        S32(st, c + 0x42C4) = v;
        if (v >= 0x24) {
            S32(st, c + 0x42C4) = 1;
        }
    }
    if (!found) {
        return 0;
    }
    int32_t good = S32(st, c + 0x42C4);
    S32(st, c + 0x42C4) = good + 1;
    if (good + 1 >= 0x24) {
        S32(st, c + 0x42C4) = 1;
    }
    if (storage_add(st, CUR_PLAYER(st), bid, good, 100) == 100) {
        city_good_value_move(st, player, bid, good);
        return good;
    }
    for (int i = 1; i < 0x24; i++) {
        int32_t v = S32(st, c + 0x42C8) + 1;
        S32(st, c + 0x42C8) = v;
        if (v >= 0x24) {
            S32(st, c + 0x42C8) = 1;
        }
        if (trade_good_open_13(st, player, city, S32(st, c + 0x42C8)) == 0) {
            continue;
        }
        good = S32(st, c + 0x42C4);
        S32(st, c + 0x42C4) = good + 1;
        if (good + 1 >= 0x24) {
            S32(st, c + 0x42C4) = 1;
        }
        if (storage_add(st, CUR_PLAYER(st), bid, good, 100) == 100) {
            city_good_value_move(st, player, bid, good);
            return good;
        }
    }
    return 0;
}

/* The end of the storage searches 0x004450C0 / 0x00445670 / 0x00445AC0: the best building's road
 * tile (+0x26, +0x28 into 0x00D3B4E4 / 0x00D3B4E0 when its +0x44 is 1, else 0x00480180 size 3). */
static int32_t storage_found(game_state *st, int32_t best)
{
    uint32_t b = B(best);
    if (U8(st, b + 0x44) == 1) {
        S32(st, ROAD_ACCESS_X) = S16(st, b + 0x26);
        S32(st, ROAD_ACCESS_Y) = S16(st, b + 0x28);
        return best;
    }
    if (building_road_access(st, S16(st, b + 0x06), S16(st, b + 0x08), 3) != 0) {
        return best;
    }
    return 0;
}

int32_t storage_find_for_trader(game_state *st, int32_t fid, int32_t x, int32_t y, int32_t city, int32_t a4)
{
    /* @bin 0x004450C0 the storage building with the lowest score for a trading figure: flags per
     * good 0x00775780[good] (trade_good_open_24, 0 when the figure's +0x6F >= 8) and
     * 0x007756F0[good] (trade_good_open_13 when city != 0, 0 when its +0x74 >= 8); buildings in
     * use, +0x44 set, +0x20 > 0, +0x40 != 0, passing 0x0045D180 or 0x0045CF50; score 32 less 4 per
     * part with an open good of the first kind, less 16 per empty part and 8 per open part of the
     * rotating good when the second kind is wanted, plus the distance (0x0045DC50) */
    uint32_t f = FIGURE_ADDR(fid);
    int32_t best = 10000, best_id = 0;
    for (int32_t good = 1; good < 0x24; good++) {
        uint32_t a = 0x00775780u + 4u * (uint32_t) good;
        S32(st, a) = trade_good_open_24(st, CUR_PLAYER(st), city, good) != 0 ? 1 : 0;
        if (U8(st, f + 0x6F) >= 8) {
            S32(st, a) = 0;
        }
        a = 0x007756F0u + 4u * (uint32_t) good;
        if (city != 0 && trade_good_open_13(st, CUR_PLAYER(st), city, good) != 0) {
            S32(st, a) = 1;
        } else {
            S32(st, a) = 0;
        }
        if (S16(st, f + 0x74) >= 8) {
            S32(st, a) = 0;
        }
    }
    int32_t wanted = 0;
    for (uint32_t a = 0x007756F4u; a < 0x00775780u; a += 4) {
        if (S32(st, a) == 1) {
            wanted++;
        }
    }
    for (int32_t bid = 1; bid < BUILDING_COUNT; bid++) {
        uint32_t b = B(bid);
        if (U8(st, b) != 1 || !is_storage(S16(st, b + 0x10)) || U8(st, b + 0x44) == 0 ||
            S16(st, b + 0x20) <= 0 || S16(st, b + 0x40) == 0) {
            continue;
        }
        uint8_t k = U8(st, b + 0xC1);
        if (storage_trades_13(st, fid, U8(st, f + 0x0F), bid, U8(st, f + 0x6E)) == 0 &&
            storage_trades_24(st, fid, U8(st, f + 0x0F), bid, U8(st, f + 0x6E)) == 0) {
            continue;
        }
        uint32_t r = STORE_REC(k);
        int32_t open = 0;
        for (int32_t good = 1; good < 0x24; good++) {
            uint8_t fl = U8(st, r + 2u + (uint32_t) good);
            if (fl == 1 || fl == 3) {
                continue;
            }
            if (trade_good_open_13(st, CUR_PLAYER(st), city, good) != 0) {
                open++;
            }
        }
        int32_t score = 0x20;
        int32_t cur = bid;
        for (int i = 0; i < 8; i++) {
            cur = S16(st, B(cur) + 0x3A);
            if (cur != 0 && S32(st, 0x00775780u + 4u * (uint32_t) (int32_t) S16(st, B(cur) + 0x12)) != 0) {
                score -= 4;
            }
            if (wanted == 0 || open == 0 || U8(st, r + 1) != 0) {
                continue;
            }
            uint32_t c = CURRENT_CITY(st);
            for (int n = 1;;) {
                int32_t v = S32(st, c + 0x42C4) + 1;
                S32(st, c + 0x42C4) = v;
                if (v >= 0x24) {
                    S32(st, c + 0x42C4) = 1;
                }
                if (U8(st, r + 2u + (uint32_t) S32(st, c + 0x42C4)) != 1) {
                    break;
                }
                n++;
                if (n >= 0x24) {
                    break;
                }
            }
            int32_t rot = S32(st, c + 0x42C4);
            if (U8(st, r + 2u + (uint32_t) rot) == 1) {
                continue;
            }
            int16_t g = S16(st, B(cur) + 0x12);
            if (g == 0) {
                score -= 16;
            }
            if (cur != 0 && S32(st, 0x007756F0u + 4u * (uint32_t) (int32_t) g) != 0 &&
                S16(st, B(cur) + 0x3C) < 0x190 && g == rot) {
                score -= 8;
            }
        }
        if (score >= 0x20) {
            continue;
        }
        score += building_distance(S16(st, b + 0x06), S16(st, b + 0x08), x, y, a4, S16(st, b + 0x20), st);
        if (score < best) {
            best = score;
            best_id = bid;
        }
    }
    return storage_found(st, best_id);
}

int32_t storage_find_13(game_state *st, int32_t x, int32_t y, int32_t city, int32_t a3, int32_t player)
{
    /* @bin 0x00445670 the storage building with the lowest score for the next good with
     * trade_good_open_13 (flags 0x007756F0[good]; rotating +0x4490 of the current player's city):
     * buildings in use of `player` (+0x14), +0x44 set, +0x20 > 0, +0x40 > 0, whose stock of the good
     * is below its settings limit (settings byte 0 or 2) and with room (0x004454D0 by 400); score
     * 32 less 8 per empty part and 4 per part below 400 holding the good, plus the distance */
    uint32_t c = CURRENT_CITY(st);
    int32_t v = S32(st, c + 0x4490) + 1;
    S32(st, c + 0x4490) = v;
    if (v >= 0x24) {
        S32(st, c + 0x4490) = 1;
    }
    int32_t best = 10000, best_id = 0;
    for (int32_t good = 1; good < 0x24; good++) {
        S32(st, 0x007756F0u + 4u * (uint32_t) good) = trade_good_open_13(st, CUR_PLAYER(st), city, good) != 0;
    }
    c = CURRENT_CITY(st);
    for (int n = 1; n < 0x24; n++) {
        int32_t g = S32(st, c + 0x4490);
        if (S32(st, 0x007756F0u + 4u * (uint32_t) g) != 0) {
            break;
        }
        S32(st, c + 0x4490) = g + 1;
        if (g + 1 >= 0x24) {
            S32(st, c + 0x4490) = 1;
        }
    }
    if (S32(st, 0x007756F0u + 4u * (uint32_t) S32(st, c + 0x4490)) == 0) {
        return 0;
    }
    for (int32_t bid = 1; bid < BUILDING_COUNT; bid++) {
        uint32_t b = B(bid);
        if (U8(st, b) != 1 || !is_storage(S16(st, b + 0x10)) || U8(st, b + 0x44) == 0 ||
            S16(st, b + 0x20) <= 0 || (int32_t) U8(st, b + 0x14) != player || S16(st, b + 0x40) <= 0) {
            continue;
        }
        uint32_t r = STORE_REC(U8(st, b + 0xC1));
        int32_t g = S32(st, CURRENT_CITY(st) + 0x4490);
        uint8_t fl = U8(st, r + 2u + (uint32_t) g);
        int32_t lim;
        if (fl == 0) {
            int32_t have = building_parts_stock(st, bid, g);
            lim = S16(st, r + 0x2Cu + 2u * (uint32_t) g);
            if (lim <= have) {
                continue;
            }
        } else if (fl == 2) {
            int32_t have = building_parts_stock(st, bid, g);
            lim = S16(st, r + 0x74u + 2u * (uint32_t) g);
            if (lim <= have) {
                continue;
            }
        } else {
            continue;
        }
        int32_t score = 0x20;
        if (storage_room_loads(st, bid, g, 0x190) == 0) {
            continue;
        }
        int32_t cur = bid;
        for (int i = 0; i < 8; i++) {
            cur = S16(st, B(cur) + 0x3A);
            int16_t pg = S16(st, B(cur) + 0x12);
            if (pg == 0) {
                score -= 8;
            }
            if (cur != 0 && pg == S32(st, CURRENT_CITY(st) + 0x4490) && S16(st, B(cur) + 0x3C) < 0x190) {
                score -= 4;
            }
        }
        if (score >= 0x20) {
            continue;
        }
        score += building_distance(S16(st, b + 0x06), S16(st, b + 0x08), x, y, a3, S16(st, b + 0x20), st);
        if (score < best) {
            best = score;
            best_id = bid;
        }
    }
    return storage_found(st, best_id);
}

int32_t storage_find_24(game_state *st, int32_t x, int32_t y, int32_t city, int32_t a3, int32_t player)
{
    /* @bin 0x00445AC0 the storage building with the lowest score for the next good with
     * trade_good_open_24 (flags 0x00775780[good]; rotating +0x4494 of the current player's city):
     * buildings in use of `player` (+0x14), +0x44 set, +0x20 > 0, holding at least 100 of the good
     * (0x0045D510); score 32 less 1 per part holding some of it, plus the distance */
    uint32_t c = CURRENT_CITY(st);
    int32_t v = S32(st, c + 0x4494) + 1;
    S32(st, c + 0x4494) = v;
    if (v >= 0x24) {
        S32(st, c + 0x4494) = 1;
    }
    int32_t best = 10000, best_id = 0;
    for (int32_t good = 1; good < 0x24; good++) {
        S32(st, 0x00775780u + 4u * (uint32_t) good) = trade_good_open_24(st, CUR_PLAYER(st), city, good) != 0;
    }
    c = CURRENT_CITY(st);
    for (int n = 1; n < 0x24; n++) {
        int32_t g = S32(st, c + 0x4494);
        if (S32(st, 0x00775780u + 4u * (uint32_t) g) != 0) {
            break;
        }
        S32(st, c + 0x4494) = g + 1;
        if (g + 1 >= 0x24) {
            S32(st, c + 0x4494) = 1;
        }
    }
    if (S32(st, 0x00775780u + 4u * (uint32_t) S32(st, c + 0x4494)) == 0) {
        return 0;
    }
    for (int32_t bid = 1; bid < BUILDING_COUNT; bid++) {
        uint32_t b = B(bid);
        if (U8(st, b) != 1 || !is_storage(S16(st, b + 0x10)) || U8(st, b + 0x44) == 0 ||
            S16(st, b + 0x20) <= 0 || (int32_t) U8(st, b + 0x14) != player) {
            continue;
        }
        if (building_parts_stock(st, bid, S32(st, CURRENT_CITY(st) + 0x4494)) < 100) {
            continue;
        }
        int32_t score = 0x20;
        int32_t cur = bid;
        for (int i = 0; i < 8; i++) {
            cur = S16(st, B(cur) + 0x3A);
            if (cur == 0) {
                continue;
            }
            if (S16(st, B(cur) + 0x12) == S32(st, CURRENT_CITY(st) + 0x4494) && S16(st, B(cur) + 0x3C) > 0) {
                score--;
            }
        }
        if (score >= 0x20) {
            continue;
        }
        score += building_distance(S16(st, b + 0x06), S16(st, b + 0x08), x, y, a3, S16(st, b + 0x20), st);
        if (score < best) {
            best = score;
            best_id = bid;
        }
    }
    return storage_found(st, best_id);
}

void figure_slot_count_good(game_state *st, int32_t fid, int32_t good)
{
    /* @bin 0x00444B80 the figure's record (+0x7F) among the 0x58-byte records at 0x0077AF00:
     * +4 + 1, byte +0x2C + good + 1, +0x54 + the good's value (0x005D2C4C + 8 good) */
    uint32_t r = 0x0077AF00u + 0x58u * (uint32_t) U8(st, FIGURE_ADDR(fid) + 0x7F);
    S32(st, r + 0x04) += 1;
    U8(st, r + 0x2Cu + (uint32_t) good) = (uint8_t) (U8(st, r + 0x2Cu + (uint32_t) good) + 1);
    S32(st, r + 0x54) += S32(st, 0x005D2C4Cu + 8u * (uint32_t) good);
}

int32_t storage_sell_next(game_state *st, int32_t player, int32_t bid, int32_t city)
{
    /* @bin 0x0045D010 the last good (+0x12) of the storage building's parts (chain +0x3A, 8 steps,
     * stock +0x3C > 0) with trade_good_open_24; 0 as soon as one of them has a total stock
     * (0x0045D510) below 100. A load of 100 of it is taken (0x0045B570) and its value
     * (0x005D2C4C + 8 good; 150 % of it when the current player's city +0x45CC > 0, 0x004CDE40)
     * added to the player's city +0xC and +0x2C64. Returns the good, else 0 */
    if (!is_storage(S16(st, B(bid) + 0x10))) {
        return 0;
    }
    int32_t good = 0;
    int32_t cur = bid;
    for (int i = 0; i < 8; i++) {
        cur = S16(st, B(cur) + 0x3A);
        if (cur <= 0 || S16(st, B(cur) + 0x3C) <= 0) {
            continue;
        }
        if (trade_good_open_24(st, player, city, S16(st, B(cur) + 0x12)) == 0) {
            continue;
        }
        if (building_parts_stock(st, bid, S16(st, B(cur) + 0x12)) < 100) {
            return 0;
        }
        good = S16(st, B(cur) + 0x12);
    }
    if (good == 0) {
        return 0;
    }
    GAME_CALL(st, 0x0045B570u, player, bid, good, 100);
    uint32_t c = CITY_ADDR(player);
    if (S32(st, CURRENT_CITY(st) + 0x45CC) > 0) {
        S32(st, c + 0x0C) += calc_percentage(S32(st, 0x005D2C4Cu + 8u * (uint32_t) good), 150);
        S32(st, c + 0x2C64) += calc_percentage(S32(st, 0x005D2C4Cu + 8u * (uint32_t) good), 150);
    } else {
        int32_t v = S32(st, 0x005D2C4Cu + 8u * (uint32_t) good);
        S32(st, c + 0x0C) += v;
        S32(st, c + 0x2C64) += v;
    }
    return good;
}

void figure_slot_count_good_b(game_state *st, int32_t fid, int32_t good)
{
    /* @bin 0x00444BD0 like 0x00444B80 on the other counters of the figure's record (+0x7F):
     * +0 + 1, byte +8 + good + 1, +0x50 + the good's value 0x005D2C48 + 8 good */
    uint32_t r = 0x0077AF00u + 0x58u * (uint32_t) U8(st, FIGURE_ADDR(fid) + 0x7F);
    S32(st, r) += 1;
    U8(st, r + 0x08u + (uint32_t) good) = (uint8_t) (U8(st, r + 0x08u + (uint32_t) good) + 1);
    S32(st, r + 0x50) += S32(st, 0x005D2C48u + 8u * (uint32_t) good);
}

int32_t storage_take_from_trader(game_state *st, int32_t bid, int32_t good, int32_t city, int32_t fid)
{
    /* @bin 0x00445980 a storage building takes loads of `good` from a trading figure: at most
     * 4 (and what the route still allows, (limit - count) * 100 capped to 400) as 0x004454D0
     * gives; the figure's +0x74 counts them up to 4; each load is stored (0x0045B2A0 by 100), its
     * value moved (city +0xC -, +0x2C6C +), counted on the route and in the figure's record
     * (0x00444B80). Returns 1 when some were taken (or the count was negative), else 0 */
    if (!is_storage(S16(st, B(bid) + 0x10))) {
        return 0;
    }
    uint32_t ec = ECITY(city);
    int32_t room = 0x190;
    int32_t limit = trade_limit_adjusted(st, S32(st, ROUTE_LIMIT(U8(st, ec), good)));
    int32_t v = (limit - S32(st, ROUTE_DONE(U8(st, ec), good))) * 100;
    if (v < room) {
        room = v;
        if (room < 0) {
            room = 0;
        }
    }
    int32_t n = storage_room_loads(st, bid, good, room);
    uint32_t f = FIGURE_ADDR(fid);
    int16_t have = S16(st, f + 0x74);
    if (have < 0) {
        n = have + 4;
    }
    if (n < 4) {
        S16(st, f + 0x74) = (int16_t) (have - n + 4);
    }
    if (n == 0) {
        return 0;
    }
    for (int32_t i = n; i > 0; i--) {
        storage_add(st, CUR_PLAYER(st), bid, good, 100);
        uint32_t c = CURRENT_CITY(st);
        int32_t value = S32(st, 0x005D2C48u + 8u * (uint32_t) good);
        S32(st, c + 0x0C) -= value;
        S32(st, c + 0x2C6C) += value;
        S32(st, ROUTE_DONE(U8(st, ec), good)) += 1;
        figure_slot_count_good(st, fid, good);
    }
    return 1;
}

int32_t building_type_0x459ef0(game_state *st, int32_t bid)
{
    /* @bin 0x00459EF0 types 0x54 .. 0x57, 0xB8, 0xBB .. 0xBD (jump table 0x00459F24) */
    switch (S16(st, B(bid) + 0x10)) {
    case 0x54: case 0x55: case 0x56: case 0x57: case 0xB8: case 0xBB: case 0xBC: case 0xBD:
        return 1;
    default:
        return 0;
    }
}

int32_t building_47_add(game_state *st, int32_t bid, int32_t good, int32_t count, int32_t amount)
{
    /* @bin 0x0045E220 a building of type 0x47 / 0xBF takes up to `amount` of `good` (1..8) from
     * its pending stock +0x56 into +0x56 + 2 good, unless that is at its settings limit (byte 0:
     * +0x2C, byte 2: +0x74; other bytes: no limit); `count`: the current player's city +0x45E4
     * gains `amount`. Returns what was moved (1 for bid <= 0) */
    uint32_t b = B(bid);
    uint8_t k = U8(st, b + 0xC1);
    if (bid <= 0) {
        return 1;
    }
    int16_t type = S16(st, b + 0x10);
    if (type != 0x47 && type != 0xBF) {
        return 0;
    }
    if (good <= 0 || good > 8) {
        return 0;
    }
    int16_t pending = S16(st, b + 0x56);
    if (pending <= 0) {
        return 0;
    }
    uint32_t r = STORE_REC(k);
    uint8_t fl = U8(st, r + 2u + (uint32_t) good);
    uint32_t slot = b + 0x56u + 2u * (uint32_t) good;
    if (fl == 0) {
        if (S16(st, r + 0x2Cu + 2u * (uint32_t) good) <= S16(st, slot)) {
            return 0;
        }
    } else if (fl == 2) {
        if (S16(st, r + 0x74u + 2u * (uint32_t) good) <= S16(st, slot)) {
            return 0;
        }
    }
    if (count != 0) {
        S32(st, CURRENT_CITY(st) + 0x45E4) += amount;
    }
    if ((int32_t) pending > amount) {
        S16(st, slot) = (int16_t) (S16(st, slot) + amount);
        S16(st, b + 0x56) = (int16_t) (S16(st, b + 0x56) - amount);
        return amount;
    }
    S16(st, slot) = (int16_t) (S16(st, slot) + pending);
    int32_t r16 = S16(st, b + 0x56);
    S16(st, b + 0x56) = 0;
    return r16;
}

/* @bin 0x0045FF1A (and 0x004600D7, 0x0045F7D8) the good a building takes as raw material (+0x12 /
 * +0xCE) for good 2 .. 0x22; -1 otherwise */
static int32_t raw_good(int32_t good)
{
    static const int8_t map[0x21] = {
        0x15, -1, -1, -1, -1, -1, 0x16, 0x0D, 0x08, 0x05, 0x07, 0x18, 0x09, 0x13, 0x01, 0x10, 0x0A,
        -1, 0x04, 0x0F, 0x0B, 0x0C, 0x03, 0x06, -1, -1, 0x0E, 0x11, 0x12, 0x17, 0x14, 0x19, 0x1A,
    };
    uint32_t i = (uint32_t) (good - 2);
    return i > 0x20 ? -1 : map[i];
}

/* @bin 0x00460003 (same tables at 0x0045F90A ...) the stock cap of a building type */
static int32_t type_cap(int32_t type)
{
    switch (type) {
    case 0x20: case 0x33: return 300;
    case 0x2F: case 0x35: return 500;
    case 0x54: case 0x55: case 0x56: case 0x57: case 0xB8: case 0xBB: case 0xBC: case 0xBD: return 100000;
    default: return 200;
    }
}

int32_t building_add_good(game_state *st, int32_t bid, int32_t amount, int32_t good)
{
    /* @bin 0x0045FE20 a building of the cart-destination types (0x6E .. 0x72 and the list of
     * jump table 0x00460294) takes `amount` of `good`: good 0x15 for types of 0x00459EF0 goes to the
     * current player's city (+0xC, +0x42E0, +0x47B0; tutorial step 4 at 500); else into +0x3C
     * when +0x12 is the good's raw material, or +0xCC when +0xCE is, up to the type's cap. Returns
     * what was taken */
    if (bid <= 0) {
        return 0;
    }
    uint32_t b = B(bid);
    int16_t type = S16(st, b + 0x10);
    if (!(type >= 0x6E && type <= 0x72)) {
        switch (type) { /* @bin 0x0045FE6F jump table */
        case 0x20: case 0x2F: case 0x33: case 0x35: case 0x4A: case 0x71: case 0xB1: case 0xB2:
        case 0xBB: case 0xBC: case 0xBD: case 0xC2: case 0xCB: case 0xCC: case 0xCD: case 0xE2:
        case 0xE7: case 0xE8: case 0xE9:
            break;
        default:
            return 0;
        }
    }
    if (good == 0x15) {
        if (building_type_0x459ef0(st, bid) == 0) {
            return 0;
        }
        uint32_t c = CURRENT_CITY(st);
        S32(st, c + 0x0C) += amount;
        S32(st, c + 0x42E0) += amount;
        S32(st, c + 0x47B0) += amount;
        /* @bin 0x0045FECD tutorial: 0x00E38EBC 0, 0x00579520 the scenario byte 0x00E38EA8,
         * 0x007027BC 0, +0x42E0 at least 500 */
        if (S32(st, 0x00E38EBCu) == 0 && S32(st, 0x00579520u) == (int32_t) S8(st, 0x00E38EA8u) &&
            U8(st, 0x007027BCu) == 0 && S32(st, c + 0x42E0) >= 500) {
            GAME_CALL(st, 0x00417AE0u, 4);
        }
        return amount;
    }
    int32_t raw = raw_good(good);
    uint32_t have_at;
    if (S16(st, b + 0x12) == raw) {
        have_at = b + 0x3C;
    } else if (S16(st, b + 0xCE) == raw) {
        have_at = b + 0xCC;
    } else {
        return 0;
    }
    int32_t cap = type_cap(type);
    int16_t have = S16(st, have_at);
    if ((int32_t) have >= cap) {
        return 0;
    }
    int32_t excess = 0;
    if (have + amount >= cap) {
        excess = have - cap + amount;
    }
    S16(st, have_at) = (int16_t) (have - excess + amount);
    return amount - excess;
}

int32_t cart_city_building(game_state *st, int32_t x, int32_t y, int32_t good, int32_t terrain)
{
    /* @bin 0x0045F7B0 for goods 0x0A and 0x1C: the current player's city building (+0x44AC), when
     * the good is not flagged in the city data (+0x2A88 + 4 good), 0x007058B8 > 0, it has room for
     * the good's raw material (+0x12: +0x3C, +0x104 loads on the way; +0xCE: +0xCC, +0x106) under
     * its type's cap, 0x00BCB9B4 > 0, a road tile (0x00480180) and +0x14 == terrain: one more load
     * on the way, returns the building, else 0 */
    (void) x;
    (void) y;
    uint32_t c = CURRENT_CITY(st);
    int32_t bid = S32(st, c + 0x44AC);
    int32_t raw = raw_good(good);
    if (good != 0x0A && good != 0x1C) {
        return 0;
    }
    if (S32(st, c + 0x2A88u + 4u * (uint32_t) good) != 0) {
        return 0;
    }
    if (S32(st, 0x007058B8u) <= 0) {
        return 0;
    }
    uint32_t b = B(bid);
    if (raw == S16(st, b + 0x12)) {
        int32_t cap = type_cap(S16(st, b + 0x10));
        int32_t have = S16(st, b + 0x3C);
        if (have >= cap) {
            return 0;
        }
        int16_t coming = S16(st, b + 0x104);
        if (coming > 0 && type_cap(S16(st, b + 0x10)) - coming * 100 - have <= 0) {
            return 0;
        }
    }
    if (raw == S16(st, b + 0xCE)) {
        int32_t cap = type_cap(S16(st, b + 0x10));
        int32_t have = S16(st, b + 0xCC);
        if (have >= cap) {
            return 0;
        }
        int16_t coming = S16(st, b + 0x106);
        if (coming > 0 && type_cap(S16(st, b + 0x10)) - coming * 100 - have <= 0) {
            return 0;
        }
    }
    if (S32(st, 0x00BCB9B4u) <= 0) {
        return 0;
    }
    if (building_road_access(st, S16(st, b + 0x06), S16(st, b + 0x08), U8(st, b + 0x03)) == 0) {
        return 0;
    }
    if ((int32_t) U8(st, b + 0x14) != terrain) {
        return 0;
    }
    if (raw == S16(st, b + 0x12)) {
        S16(st, b + 0x104) = (int16_t) (S16(st, b + 0x104) + 1);
    }
    if (raw == S16(st, b + 0xCE)) {
        S16(st, b + 0x106) = (int16_t) (S16(st, b + 0x106) + 1);
    }
    return bid;
}

void figure_mark_building_0x20(game_state *st, int32_t fid)
{
    /* @bin 0x00468530 figure types 0x0F / 0x10 / 0x11 set their building's (+0x5E) byte +0xA7 /
     * +0xA8 / +0xA9 to 0x20 */
    uint32_t f = FIGURE_ADDR(fid);
    uint8_t type = U8(st, f + 0x0A);
    int32_t b = S16(st, f + 0x5E);
    switch (type) {
    case 0x0F: U8(st, B(b) + 0xA7) = 0x20; break;
    case 0x10: U8(st, B(b) + 0xA8) = 0x20; break;
    case 0x11: U8(st, B(b) + 0xA9) = 0x20; break;
    default: break;
    }
}

int32_t map_free_tile_in_ring(game_state *st, int32_t x, int32_t y, int32_t size, int32_t r)
{
    /* @bin 0x004800B0 a tile whose byte in the grid 0x00EBAB20 is 0 in the square (x - r, y - r) ..
     * (x + size + r - 1, y + size + r - 1) clipped to the map (0x00BCA2D0 x 0x00BCA2D4), scanned by
     * rows; the scan's x, y, tile are kept in 0x00D3B4E4, 0x00D3B4E0, 0x00BC9768. 1 when found */
    int32_t x0 = x - r, x1 = x + size + r - 1;
    int32_t y0 = y - r, y1 = y + size + r - 1;
    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    if (x1 >= S32(st, 0x00BCA2D0u)) {
        x1 = S32(st, 0x00BCA2D0u) - 1;
    }
    if (y1 >= S32(st, 0x00BCA2D4u)) {
        y1 = S32(st, 0x00BCA2D4u) - 1;
    }
    uint32_t tile = U32(st, 0x00BCA2D8u) + 228u * (uint32_t) y0 + (uint32_t) x0;
    uint32_t step = (uint32_t) (x0 - x1 + 0xE3);
    S32(st, 0x00D3B4E0u) = y0;
    S32(st, 0x00BC9768u) = (int32_t) tile;
    for (int32_t yy = y0; yy <= y1;) {
        int32_t xx = x0;
        S32(st, 0x00D3B4E4u) = xx;
        while (xx <= x1) {
            if (U8(st, 0x00EBAB20u + tile) == 0) {
                return 1;
            }
            xx++;
            tile++;
            S32(st, 0x00D3B4E4u) = xx;
            S32(st, 0x00BC9768u) = (int32_t) tile;
        }
        yy++;
        tile += step;
        S32(st, 0x00D3B4E0u) = yy;
        S32(st, 0x00BC9768u) = (int32_t) tile;
    }
    return 0;
}

int32_t map_free_tile_near(game_state *st, int32_t x, int32_t y, int32_t size, int32_t max_r)
{
    /* @bin 0x00480060 map_free_tile_in_ring for r = 1 .. max_r */
    for (int32_t r = 1; r <= max_r; r++) {
        if (map_free_tile_in_ring(st, x, y, size, r) != 0) {
            return 1;
        }
    }
    return 0;
}

/* wrappers */
#define W3(n, f) static int32_t n(game_state *st, const int32_t *a) { return f(st, a[0], a[1], a[2]); }
#define W4(n, f) static int32_t n(game_state *st, const int32_t *a) { return f(st, a[0], a[1], a[2], a[3]); }
#define W5(n, f) static int32_t n(game_state *st, const int32_t *a) { return f(st, a[0], a[1], a[2], a[3], a[4]); }
W3(run_open_24, trade_good_open_24)
W3(run_open_13, trade_good_open_13)
W3(run_over_limit, storage_over_limit)
W4(run_storage_add, storage_add)
W3(run_room_loads, storage_room_loads)
W4(run_trades_24, storage_trades_24)
W4(run_trades_13, storage_trades_13)
W3(run_next_good_13, storage_next_good_13)
W5(run_find_for_trader, storage_find_for_trader)
W5(run_find_13, storage_find_13)
W5(run_find_24, storage_find_24)
W4(run_take_from_trader, storage_take_from_trader)
W3(run_sell_next, storage_sell_next)
W4(run_47_add, building_47_add)
W3(run_add_good, building_add_good)
W4(run_city_building, cart_city_building)
W4(run_ring, map_free_tile_in_ring)
W4(run_near, map_free_tile_near)

static int32_t run_value_move(game_state *st, const int32_t *a)
{
    city_good_value_move(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_slot_count(game_state *st, const int32_t *a)
{
    figure_slot_count_good(st, a[0], a[1]);
    return 0;
}

static int32_t run_slot_count_b(game_state *st, const int32_t *a)
{
    figure_slot_count_good_b(st, a[0], a[1]);
    return 0;
}

static int32_t run_type_459ef0(game_state *st, const int32_t *a)
{
    return building_type_0x459ef0(st, a[0]);
}

static int32_t run_mark_0x20(game_state *st, const int32_t *a)
{
    figure_mark_building_0x20(st, a[0]);
    return 0;
}

const ported_function figx_b_ports[] = {
    {0x004434E0u, "trade_good_open_24", 3, 32, 0, run_open_24, 0},
    {0x004435F0u, "trade_good_open_13", 3, 32, 0, run_open_13, 0},
    {0x0045B230u, "storage_over_limit", 3, 8, 0, run_over_limit, 0},
    {0x0045B2A0u, "storage_add", 4, 32, 0, run_storage_add, 0},
    {0x004454D0u, "storage_room_loads", 3, 32, 0, run_room_loads, 0},
    {0x0045CF50u, "storage_trades_24", 4, 32, 0, run_trades_24, 0},
    {0x0045D180u, "storage_trades_13", 4, 32, 0, run_trades_13, 0},
    {0x0045D370u, "storage_next_good_13", 3, 32, 0, run_next_good_13, 0},
    {0x0045D4D0u, "city_good_value_move", 3, 0, 0, run_value_move, 0},
    {0x004450C0u, "storage_find_for_trader", 5, 32, 0, run_find_for_trader, 0},
    {0x00445670u, "storage_find_13", 5, 32, 0, run_find_13, 0},
    {0x00445AC0u, "storage_find_24", 5, 32, 0, run_find_24, 0},
    {0x00445980u, "storage_take_from_trader", 4, 32, 0, run_take_from_trader, 0},
    {0x00444B80u, "figure_slot_count_good", 2, 0, 0, run_slot_count, 0},
    {0x0045D010u, "storage_sell_next", 3, 32, 0, run_sell_next, 0},
    {0x00444BD0u, "figure_slot_count_good_b", 2, 0, 0, run_slot_count_b, 0},
    {0x00459EF0u, "building_type_0x459ef0", 1, 32, 0, run_type_459ef0, 0},
    {0x0045E220u, "building_47_add", 4, 32, 0, run_47_add, 0},
    {0x0045FE20u, "building_add_good", 3, 32, 0, run_add_good, 0},
    {0x0045F7B0u, "cart_city_building", 4, 32, 0, run_city_building, 0},
    {0x00468530u, "figure_mark_building_0x20", 1, 0, 0, run_mark_0x20, 0},
    {0x004800B0u, "map_free_tile_in_ring", 4, 32, 0, run_ring, 0},
    {0x00480060u, "map_free_tile_near", 4, 32, 0, run_near, 0},
};
const int figx_b_ports_count = sizeof(figx_b_ports) / sizeof(figx_b_ports[0]);
