/* Imhotep - which goods the city can obtain (Pharaoh.exe 0x004439C0 and related).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/goods.h"

#define CURRENT_PLAYER 0x00EAAC28u /* chunk 31 */
#define CITY_DATA 0x00EA1870u      /* chunk 28, 2 x 0x49D8 */
#define CITY_SIZE 0x49D8u
#define EMPIRE_CITIES 0x0077DE00u  /* chunk 50, 61 x 0x6A */
#define EMPIRE_CITY_SIZE 0x6Au
#define EMPIRE_CITY_COUNT 61       /* loops end at 0x0077F744 = +2 of record 61 */
#define ROUTE_LIMITS 0x0077646Cu   /* chunk 114, i32[route][36] */
#define SCENARIO_FLAG_10 0x00784D32u /* i16, tested for good 10 (scenario info +0x45A) */
#define SCENARIO_FLAG_1C 0x00784D34u /* i16, tested for good 0x1C (scenario info +0x45C) */

static uint32_t city(int32_t i)
{
    return EMPIRE_CITIES + EMPIRE_CITY_SIZE * (uint32_t) i;
}

int32_t trade_city_mood(game_state *st)
{
    /* @bin 0x004C5190 */
    uint32_t c = CITY_DATA + CITY_SIZE * U32(st, CURRENT_PLAYER);
    int32_t v = S16(st, c + 0x4984) > 0;
    if (S16(st, c + 0x4986) > 0) {
        v -= 2;
    }
    if (S16(st, c + 0x4988) > 0) {
        v -= 1;
    }
    return v;
}

int32_t trade_limit_adjusted(game_state *st, int32_t p)
{
    /* @bin 0x004C5240 */
    int32_t m = trade_city_mood(st);
    if (p == 0) {
        return 0;
    }
    if (m == 1) {
        return p == 0x19 ? 0x28 : p == 0xF ? 0x19 : p > 0xE ? p : 0xF;
    }
    if (m > 1) {
        return p == 0x19 ? 0x28 : p == 0xF ? 0x28 : p > 0xE ? p : 0x19;
    }
    if (m == -1) {
        return p == 0x28 ? 0x19 : p == 0x19 ? 0xF : p == 0xF ? 0 : p;
    }
    if (m > -2) {
        return p;
    }
    return p == 0x28 ? 0xF : (p != 0x19 && p != 0xF) ? p : 0;
}

int32_t empire_city_trades(game_state *st, int32_t c, int32_t resource)
{
    /* @bin 0x00447790 limit of route (city +4) and resource, adjusted, > 0 */
    uint32_t route = U8(st, city(c) + 4);
    return trade_limit_adjusted(st, S32(st, ROUTE_LIMITS + 4u * ((uint32_t) resource + route * 0x24))) > 0;
}

static int32_t trade_city(game_state *st, int32_t resource, int need_flag5)
{
    for (int32_t i = 0; i < EMPIRE_CITY_COUNT; i++) {
        uint32_t c = city(i);
        uint8_t kind = U8(st, c + 2);
        if (U8(st, c) != 0 && (kind == 1 || kind == 3 || kind == 5) && (!need_flag5 || U8(st, c + 5) != 0) &&
            U8(st, c + 0x2A + (uint32_t) resource) == 1 && empire_city_trades(st, i, resource)) {
            return 1;
        }
    }
    return 0;
}

int32_t goods_trade_city(game_state *st, int32_t resource)
{
    return trade_city(st, resource, 0); /* @bin 0x004439C0 */
}

int32_t goods_trade_city_flag5(game_state *st, int32_t resource)
{
    return trade_city(st, resource, 1); /* @bin 0x00443950 */
}

/* the final test of 0x00443B90 and 0x00443D90: some city in use, of kind 0, with +0x2A+resource == 1 */
static int32_t kind0_city(game_state *st, int32_t resource)
{
    for (int32_t i = 0; i < EMPIRE_CITY_COUNT; i++) {
        uint32_t c = city(i);
        if (U8(st, c) != 0 && U8(st, c + 2) == 0 && U8(st, c + 0x2A + (uint32_t) resource) == 1) {
            return 1;
        }
    }
    return 0;
}

int32_t goods_chain_b90(game_state *st, int32_t p)
{
    /* @bin 0x00443B90 */
    int32_t v;
    switch (p) {
    case 0x0F: p = 0x0E; v = goods_trade_city(st, 0x0E); break;
    case 0x11: p = 0x10; v = goods_trade_city(st, 0x10); break;
    case 0x21: p = 0x20; v = goods_trade_city(st, 0x20); break;
    case 0x22:
        v = goods_chain_b90(st, 0x0D);
        if ((v || goods_chain_d90(st, 0x0D)) && goods_trade_city_flag5(st, 0x1F)) {
            return 1;
        }
        if (!goods_trade_city(st, 0x0D)) {
            return kind0_city(st, p);
        }
        v = goods_trade_city_flag5(st, 0x1F);
        break;
    case 0x13: p = 0x12; v = goods_trade_city(st, 0x12); break;
    case 0x0D: p = 0x0B; v = goods_trade_city(st, 0x0B); break;
    case 0x0A:
        p = 0x1D;
        if (S16(st, SCENARIO_FLAG_10) == 0) {
            return 0;
        }
        v = goods_trade_city(st, 0x1D);
        break;
    case 0x1C:
        p = 0x14;
        if (S16(st, SCENARIO_FLAG_1C) == 0) {
            return 0;
        }
        v = goods_trade_city(st, 0x14);
        break;
    case 0x0C:
        v = goods_chain_b90(st, 0x0B);
        if ((v || goods_chain_d90(st, 0x0B)) && goods_chain_b90(st, 1)) {
            return 1;
        }
        v = goods_chain_b90(st, 0x0B);
        if ((v || goods_chain_d90(st, 0x0B)) && goods_trade_city(st, 9)) {
            return 1;
        }
        if (goods_chain_b90(st, 1) && goods_trade_city(st, 0x0B)) {
            return 1;
        }
        if (!goods_trade_city(st, 9)) {
            return kind0_city(st, p);
        }
        v = goods_trade_city(st, 0x0B);
        break;
    case 0x17: p = 0x16; v = goods_trade_city(st, 0x16); break;
    case 9: v = goods_chain_b90(st, 1); break;
    default: return kind0_city(st, p);
    }
    return v ? 1 : kind0_city(st, p);
}

int32_t goods_chain_d90(game_state *st, int32_t p)
{
    /* @bin 0x00443D90 */
    int32_t v;
    switch (p) {
    case 0x0F: p = 0x0E; break;
    case 0x11: p = 0x10; break;
    case 0x21: p = 0x20; break;
    case 0x13: p = 0x12; break;
    case 0x0D: p = 0x0B; break;
    case 0x0A: p = 0x1D; break;
    case 0x1C: p = 0x14; break;
    case 0x17: p = 0x16; break;
    case 0x0C:
        if (!goods_chain_b90(st, 1) && !goods_chain_d90(st, 9)) {
            return kind0_city(st, p);
        }
        if (goods_chain_b90(st, 0x0B)) {
            return 1;
        }
        v = goods_chain_d90(st, 0x0B);
        return v ? 1 : kind0_city(st, p);
    case 0x22:
        if (!goods_chain_b90(st, 0x0D)) {
            return kind0_city(st, p);
        }
        v = goods_trade_city(st, 0x1F);
        return v ? 1 : kind0_city(st, p);
    default: break;
    }
    v = goods_trade_city(st, p);
    return v ? 1 : kind0_city(st, p);
}

int32_t goods_event_list(game_state *st, int32_t p)
{
    /* @bin 0x00559480 entries of 0x7C bytes; i16 at +0, -2, +2 of 0x00785D7C + i*0x7C, kind byte
     * at -6; scanned from the last entry */
    int32_t n = S32(st, 0x00785D70u);
    for (int32_t i = n - 1; n != 0 && i >= 0; i--) {
        uint32_t e = 0x00785D7Cu + 0x7Cu * (uint32_t) i;
        if (U8(st, e - 6) == 0x17 && (S16(st, e - 2) == p || S16(st, e) == p || S16(st, e + 2) == p)) {
            return 1;
        }
    }
    int32_t q;
    if (p == 0x0C) {
        if ((goods_chain_b90(st, 0x0B) || goods_chain_d90(st, 0x0B)) && goods_event_list(st, 9)) {
            return 1;
        }
        if (goods_event_list(st, 0x0B) && goods_chain_b90(st, 1)) {
            return 1;
        }
        if (goods_trade_city(st, 0x0B) && goods_event_list(st, 9)) {
            return 1;
        }
        if (goods_event_list(st, 0x0B) && goods_trade_city(st, 9)) {
            return 1;
        }
        if (!goods_event_list(st, 0x0B)) {
            return 0;
        }
        q = 9;
    } else if (p == 0x22) {
        if (!goods_chain_b90(st, 0x0B) && !goods_chain_d90(st, 0x0B) && !goods_event_list(st, 0x0B) &&
            !goods_event_list(st, 0x0D)) {
            return 0;
        }
        return goods_trade_city(st, 0x1F) != 0;
    } else {
        switch (p) {
        case 0x0A:
            if (S16(st, SCENARIO_FLAG_10) == 0) {
                return 0;
            }
            q = 0x1D;
            break;
        case 0x0D: q = 0x0B; break;
        case 0x0F: q = 0x0E; break;
        case 0x11: q = 0x10; break;
        case 0x13: q = 0x12; break;
        case 0x17: q = 0x16; break;
        case 0x1C:
            if (S16(st, SCENARIO_FLAG_1C) == 0) {
                return 0;
            }
            q = 0x14;
            break;
        case 0x21: q = 0x20; break;
        default: return 0;
        }
    }
    return goods_event_list(st, q) ? 1 : 0;
}

void goods_available_update(game_state *st)
{
    /* @bin 0x005596D0 */
    S32(st, 0x00F53E6Cu) = 0;
    for (uint32_t i = 0; i < 0x24; i++) {
        S32(st, 0x00F53E70u + 4 * i) = 0;
    }
    for (uint32_t i = 0; i < 0x24; i++) {
        S32(st, 0x00F53DDCu + 4 * i) = 0;
    }
    S32(st, 0x00F53DD8u) = 0;
    uint32_t c = CITY_DATA + CITY_SIZE * U32(st, CURRENT_PLAYER);
    for (int32_t r = 1; r < 0x24; r++) {
        if (goods_chain_b90(st, r) || goods_trade_city(st, r) || goods_event_list(st, r) ||
            U8(st, c + 0x2995 + (uint32_t) r) != 0) { /* 0x00EA4205 */
            S32(st, 0x00F53E70u + 4u * (uint32_t) S32(st, 0x00F53E6Cu)) = r;
            S32(st, 0x00F53E6Cu) += 1;
        }
    }
    for (int32_t r = 1; r < 9; r++) {
        if (goods_chain_b90(st, r) || goods_trade_city(st, r) || goods_event_list(st, r) ||
            U8(st, c + 0x298C + (uint32_t) r) != 0) { /* 0x00EA41FC */
            S32(st, 0x00F53DDCu + 4u * (uint32_t) S32(st, 0x00F53DD8u)) = r;
            S32(st, 0x00F53DD8u) += 1;
        }
    }
    for (int32_t i = 0; i < S32(st, 0x00F53E6Cu); i++) {
        int32_t r = S32(st, 0x00F53E70u + 4u * (uint32_t) i);
        if (r != 0) {
            U8(st, c + 0x2995 + (uint32_t) r) = 1;
        }
    }
    for (int32_t i = 0; i < S32(st, 0x00F53DD8u); i++) {
        int32_t r = S32(st, 0x00F53DDCu + 4u * (uint32_t) i);
        if (r != 0) {
            U8(st, c + 0x298C + (uint32_t) r) = 1;
        }
    }
}
