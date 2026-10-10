/* Imhotep - build-menu availability (package cmdrest, docs/commands.md 2.1).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The build menus are 38 lists of 30 entries: the type table i32 0x005CE3B8[menu * 30 + entry]
 * (entry 0 is not a building: the entries are read from index 1) and the availability table
 * i32 0x0070A8DC[menu * 30 + i] of the entry at index i + 1. 0x00404350 fills the availability
 * from the scenario's allowed buildings (i16 flags around 0x00784CE4 / 0x007848E2 / 0x00784DB0),
 * the city's production chains and the mission's own lists; 0x00404120 counts the available
 * entries of the menu 0x007056EC (the sidebar buttons and the submenus use it). */
#include "command/menu.h"

#include "city/goods.h"
#include "city/month.h"
#include "city/monument_rating.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

static int allowed(game_state *st, uint32_t a)
{
    return S16(st, a) != 0;
}

uint8_t menu_cheat_gate(game_state *st)
{
    /* @bin 0x00403EF0 the mission 0x0057961C is gated by the flag 0x007027C5; others are open */
    if ((int8_t) U8(st, G_MISSION) == S32(st, G_CHEAT_MISSION)) {
        return U8(st, G_CHEAT_FLAG) != 0;
    }
    return 1;
}

int32_t monuments_placed(game_state *st)
{
    /* @bin 0x004F5020 the placed monuments among the scenario's three */
    int32_t n = 0;
    for (uint32_t i = 0; i < 3; i++) {
        if (S32(st, CURRENT_CITY(st) + CITY_MONUMENT_CODE + 4u * i) != 0) {
            n++;
        }
    }
    return n;
}

static int model_has_kind(game_state *st, uint32_t model, int32_t kind)
{
    /* @bin 0x004F50A1 / 0x004F51F0 the model's three kinds (+6, +8, +0xA) */
    return S16(st, model + 6) == kind || S16(st, model + 8) == kind || S16(st, model + 0xA) == kind;
}

uint8_t monument_kind_open(game_state *st, int32_t kind, uint8_t any)
{
    /* @bin 0x004F5050 some scenario monument of `kind`: not placed yet, or placed and not finished
     * (or `any`) */
    uint32_t city = CURRENT_CITY(st);
    for (uint32_t i = 0; i < 3; i++) {
        int32_t code = S32(st, city + CITY_MONUMENT_CODE + 4u * i);
        if (code == 0) {
            uint32_t model = G_MONUMENT_MODELS + 16u * (uint32_t) (int32_t) S16(st, G_MONUMENT_SLOTS + 2u * i);
            if (model_has_kind(st, model, kind)) {
                return 1;
            }
            continue;
        }
        int32_t id = S32(st, city + CITY_MONUMENT_ID + 4u * i);
        if (id == 0) {
            continue;
        }
        uint32_t model = G_MONUMENT_MODELS + 16u * (uint32_t) code;
        int32_t type = S16(st, BUILDING_ADDR(id) + BLD_TYPE);
        int unfinished;
        /* @bin 0x004F50F0 by the building's type (jump table 0x004F523C / 0x004F5270) */
        switch (type) {
        case 0xB7: {
            int32_t out[0x1C];
            pyramid_status(st, id, out);
            unfinished = out[0xD] < 100; /* @bin 0x004F5119 [esp + 0x54] before the add: out + 0x34 */
            break;
        }
        case 0xCF:
        case 0xD0:
            unfinished = U8(st, BUILDING_ADDR(id) + BLD_U55) != 100; /* @bin 0x004F513E */
            break;
        case 0xD2:
            unfinished = S16(st, city + CITY_U433A) < 0x10; /* @bin 0x004F5131 */
            break;
        case 0xD5: unfinished = GAME_CALL(st, FN_MONUMENT_D5_STATUS, id, 0) < 100; break;
        case 0xE1: unfinished = GAME_CALLT(st, G_MONUMENT_E1, FN_MONUMENT_PROGRESS_40D940, id, 0) < 100; break;
        case 0xE3: unfinished = GAME_CALLT(st, G_MONUMENT_E3, FN_MONUMENT_PROGRESS_417050, id, 0) < 100; break;
        case 0xE4: unfinished = GAME_CALLT(st, G_MONUMENT_E4, FN_MONUMENT_PROGRESS_4E6710, id, 0) < 100; break;
        case 0xE5: unfinished = GAME_CALLT(st, G_MONUMENT_E5, FN_MONUMENT_PROGRESS_PARTS_9, id, 0) < 100; break;
        case 0xE6: unfinished = GAME_CALLT(st, G_MONUMENT_E6, FN_MONUMENT_PROGRESS_401540, id, 0) < 100; break;
        case 0xEA: unfinished = GAME_CALLT(st, G_MONUMENT_EA, FN_MONUMENT_PROGRESS_PARTS_10, id, 0) < 100; break;
        case 0xEB: unfinished = GAME_CALLT(st, G_MONUMENT_EB, FN_MONUMENT_PROGRESS_PARTS_21, id, 0) < 100; break;
        case 0xEC: unfinished = GAME_CALLT(st, G_MONUMENT_EC, FN_MONUMENT_PROGRESS_PARTS_31, id, 0) < 100; break;
        default: unfinished = GAME_CALL(st, FN_MONUMENT_DE_STATUS, id, 0) < 100; break;
        }
        if ((unfinished || any) && model_has_kind(st, model, kind)) {
            return 1;
        }
    }
    return 0;
}

uint8_t monument_menu_open(game_state *st, int32_t type)
{
    /* @bin 0x004F5470 the monument menu entries (types 0xB1-0xB3, 0xE7) by monument kinds */
    switch (type) {
    case 0xB1: return monument_kind_open(st, 0x14, 0);
    case 0xB2: return monument_kind_open(st, 0xC, 0);
    case 0xB3:
        return monument_kind_open(st, 0x18, 0) || monument_kind_open(st, 0x19, 0) ||
               monument_kind_open(st, 0x1A, 0) || monument_kind_open(st, 0x1E, 0) ||
               monument_kind_open(st, 0x22, 0) || monument_kind_open(st, 0x23, 0);
    case 0xE7: return monument_kind_open(st, 0x21, 0) || monument_kind_open(st, 0xB, 0);
    default: return 0;
    }
}

static int monuments_menu(game_state *st)
{
    /* @bin 0x00403FC1 / 0x004041EE the monuments entry (type 4) */
    return monument_menu_open(st, 0xB3) || monument_menu_open(st, 0xB2) || monument_menu_open(st, 0xB1);
}

int32_t build_menu_count(game_state *st)
{
    /* @bin 0x00404120 available entries of the menu 0x007056EC, up to its first empty entry; some
     * menus test their entries directly (jump table 0x004042F0 / 0x00404308 on menu - 6) */
    int32_t n = 0;
    for (int32_t i = 0; i < 30; i++) {
        int32_t menu = S32(st, G_SUBMENU);
        uint32_t k = (uint32_t) (menu * 30 + i);
        int32_t type = S32(st, G_MENU_TYPES_1 + 4u * k);
        if (type <= 0) {
            break;
        }
        uint32_t city = CURRENT_CITY(st);
        int ok;
        switch (menu) {
        case 0x1C: /* @bin 0x00404163 scenario monuments: allowed and not placed */
            if (!menu_cheat_gate(st)) {
                ok = 0;
                break;
            }
            type = S32(st, G_MENU_TYPES_1 + 4u * (uint32_t) (S32(st, G_SUBMENU) * 30 + i)) - 1;
            ok = S16(st, G_MONUMENT_SLOTS + 2u * (uint32_t) type) != 0 &&
                 S32(st, city + CITY_MONUMENT_CODE + 4u * (uint32_t) type) == 0;
            break;
        case 0x16: /* @bin 0x004041B2 */
            ok = menu_cheat_gate(st) &&
                 monument_menu_open(st, S32(st, G_MENU_TYPES_1 + 4u * (uint32_t) (S32(st, G_SUBMENU) * 30 + i)));
            break;
        case 0x0A: /* @bin 0x004041D8 */
            if (type == 4) {
                ok = menu_cheat_gate(st) && monuments_menu(st);
            } else {
                ok = S32(st, G_MENU_AVAIL + 4u * k) != 0;
            }
            break;
        case 0x06: /* @bin 0x00404232 */
            if (type == 0x30) {
                ok = menu_cheat_gate(st) && monuments_placed(st) < monuments_planned(st);
            } else if (type == 0x61 && S16(st, city + CITY_U4478) != 0) {
                ok = !building_0xa9_is(st, S32(st, city + CITY_U472C), 1) ||
                     !building_0xa9_is(st, S32(st, CURRENT_CITY(st) + CITY_U472C), 2);
            } else {
                ok = S32(st, G_MENU_AVAIL + 4u * k) != 0;
            }
            break;
        case 0x24: /* @bin 0x004042AF the complex's add-on i + 1 not built */
            ok = !building_0xa9_is(st, S32(st, city + CITY_U472C), (uint32_t) (i + 1));
            break;
        default:
            ok = S32(st, G_MENU_AVAIL + 4u * k) != 0;
            break;
        }
        if (ok) {
            n++;
        }
    }
    return n;
}

/* @bin 0x004043B2-0x0040470B first chain: allowed by the scenario (or by production chains) */
static int avail_first(game_state *st, int32_t t)
{
    switch (t) {
    case 9: case 0x5C: case 0x89: case 5: case 0x51: case 0xA7: case 0x37: return 1;
    case 0xB4: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 18u);
    case 0x73: return goods_chain_b90(st, 8) != 0;
    case 0x47: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 7u);
    case 0x33: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 16u);
    case 0x35: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 17u);
    case 0xCE: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 21u);
    case 0x2E: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 20u);
    case 0x2F: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 22u);
    case 0xB8: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 24u);
    case 0x88: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 29u);
    case 0xA1: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 2u);
    case 0x1F: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 11u);
    case 0x24: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 11u);
    case 0xD1: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 15u);
    case 0xC4: return goods_chain_b90(st, 6) != 0;
    case 0x65: return goods_chain_b90(st, 0x10) != 0;
    case 0xE0: return goods_chain_b90(st, 0x20) != 0;
    case 0x64: return goods_chain_b90(st, 0xE) != 0;
    case 0x6D: return goods_chain_b90(st, 0xB) != 0;
    case 0x72: return goods_chain_d90(st, 0xB) != 0;
    case 0x27: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 30u);
    case 0x6E: return goods_chain_d90(st, 0xF) != 0;
    case 0x8A: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 27u);
    case 0x52: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 28u);
    case 0x56: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 23u);
    case 0xCB: return goods_chain_d90(st, 0x16) != 0;
    case 0xE8: return goods_chain_d90(st, 0x1F) != 0 && goods_chain_d90(st, 0xD) != 0;
    case 0xE9: return goods_chain_d90(st, 0x20) != 0;
    case 0xC3: return goods_chain_b90(st, 0x16) != 0;
    case 0x46: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 8u);
    case 0x48: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 9u);
    case 0xC7: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 6u);
    case 0x6F: return goods_chain_d90(st, 0x10) != 0;
    case 0x3C: return allowed(st, G_GOD_MODE);
    case 0x40: return allowed(st, G_GOD_MODE + 2u * 4u);
    case 0x8C: return allowed(st, G_GOD_MODE);
    case 0x90: return allowed(st, G_GOD_MODE + 2u * 4u);
    case 0xB1: return goods_chain_d90(st, 0x14) != 0;
    case 0xB2: return goods_chain_b90(st, 0xC) != 0 || goods_chain_d90(st, 0xC) != 0;
    case 0x4A:
        return allowed(st, G_ALLOWED_BUILDINGS + 2u * 5u) || allowed(st, G_ALLOWED_BUILDINGS + 2u * 44u) || allowed(st, G_ALLOWED_BUILDINGS + 2u * 43u);
    case 0x31: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 19u);
    case 0xBB: {
        if (!allowed(st, G_ALLOWED_BUILDINGS + 2u * 25u)) {
            return 0;
        }
        int32_t r = S32(st, CURRENT_CITY(st) + CITY_RANK);
        return r >= 0 && r < 6;
    }
    default: return 0;
    }
}

/* @bin 0x00404765-0x00404B0B second chain (temples, shrines, forts, ...) */
static int avail_second(game_state *st, int32_t t)
{
    switch (t) {
    case 3:
        return goods_chain_b90(st, 0x16) || goods_chain_b90(st, 0xB) || goods_chain_b90(st, 0x14) ||
               goods_chain_b90(st, 0x18) || goods_chain_b90(st, 0x19) || goods_chain_b90(st, 0x1E) ||
               goods_chain_b90(st, 0x1D) || goods_chain_b90(st, 0x12) || allowed(st, G_ALLOWED_BUILDINGS + 2u * 2u);
    case 2:
        return goods_chain_b90(st, 0xE) || goods_chain_b90(st, 1) || goods_chain_b90(st, 3) ||
               goods_chain_b90(st, 4) || goods_chain_b90(st, 0x10) || goods_chain_b90(st, 5) ||
               goods_chain_b90(st, 6) || goods_chain_b90(st, 0x20);
    case 0x41: return allowed(st, G_U784DB0);
    case 0x43: return allowed(st, G_U784DB0 + 2u * 2u);
    case 0x44: return allowed(st, G_U784DB0 + 2u * 3u);
    case 0x42: return allowed(st, G_U784DB0 + 2u * 1u);
    case 0x45: return allowed(st, G_U784DB0 + 2u * 4u);
    case 0x61:
        return allowed(st, G_U784DB0) || allowed(st, G_U784DB0 + 2u * 2u) || allowed(st, G_U784DB0 + 2u * 3u) ||
               allowed(st, G_U784DB0 + 2u * 1u) || allowed(st, G_U784DB0 + 2u * 4u);
    case 0x3D: return allowed(st, G_GOD_MODE + 2u * 1u);
    case 0x3E: return allowed(st, G_GOD_MODE + 2u * 2u);
    case 0x3F: return allowed(st, G_GOD_MODE + 2u * 3u);
    case 0x60:
    case 0x96:
        return allowed(st, G_GOD_MODE) || allowed(st, G_GOD_MODE + 2u * 2u) || allowed(st, G_GOD_MODE + 2u * 3u) ||
               allowed(st, G_GOD_MODE + 2u * 1u) || allowed(st, G_GOD_MODE + 2u * 4u);
    case 0x8D: return allowed(st, G_GOD_MODE + 2u * 1u);
    case 0x8E: return allowed(st, G_GOD_MODE + 2u * 2u);
    case 0x8F: return allowed(st, G_GOD_MODE + 2u * 3u);
    case 0x29: case 0x2A: case 0x2B: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 32u);
    case 0x5B: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 32u) || allowed(st, G_ALLOWED_BUILDINGS + 2u * 31u) || allowed(st, G_ALLOWED_BUILDINGS + 2u * 30u);
    case 0x26: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 31u);
    case 0x34: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 29u) || allowed(st, G_ALLOWED_BUILDINGS + 2u * 28u);
    default: return 0;
    }
}

static int rank_in(game_state *st, int32_t lo, int32_t hi)
{
    int32_t r = S32(st, CURRENT_CITY(st) + CITY_RANK);
    return r >= lo && r < hi;
}

/* @bin 0x00404B44-0x004050B8 third chain: 1 = available, 0 = unchanged, -1 = unchanged and
 * the mission lists run whatever the mission (type 0x71 in mission 5, 0x004050D1) */
static int avail_third(game_state *st, int32_t t, int32_t custom, int8_t mission)
{
    switch (t) {
    case 0x66: return goods_chain_b90(st, 1) != 0;
    case 0xC2: return goods_chain_b90(st, 2) != 0;
    case 8: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 4u);
    case 0xBC: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 25u) && rank_in(st, 6, 8);
    case 0xBD: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 25u) && rank_in(st, 8, 0xB);
    case 0x4D: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 26u) && rank_in(st, 0, 6);
    case 0x4E: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 26u) && rank_in(st, 6, 8);
    case 0x4F: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 26u) && rank_in(st, 8, 0xB);
    case 7: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 3u);
    case 0x1E: case 0x22: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 12u);
    case 0x21: case 0x23: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 13u);
    case 0x20: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 14u);
    case 0xE2: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 45u);
    case 0x4B: case 0xC0: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 10u);
    case 0x4C: return goods_chain_b90(st, 7) != 0 && allowed(st, G_ALLOWED_BUILDINGS + 2u * 5u);
    case 0xB5: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 44u);
    case 0xB6: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 43u);
    case 0xA9: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 33u);
    case 0xAD: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 34u);
    case 0xCA: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 35u);
    case 0x39: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 37u) || allowed(st, G_ALLOWED_BUILDINGS + 2u * 38u) || allowed(st, G_ALLOWED_BUILDINGS + 2u * 39u);
    case 0x5E: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 40u);
    case 0x28: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 39u);
    case 0x2C: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 38u);
    case 0x2D: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 37u);
    case 0x5F: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 36u);
    case 0xBE: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 8u);
    case 0xBF: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 7u);
    case 0xC1: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 9u);
    case 0xB0: return allowed(st, G_ALLOWED_BUILDINGS + 2u * 33u) || allowed(st, G_ALLOWED_BUILDINGS + 2u * 34u) || allowed(st, G_ALLOWED_BUILDINGS + 2u * 35u);
    case 0x67: return goods_chain_b90(st, 3) != 0;
    case 0x69: return goods_chain_b90(st, 4) != 0;
    case 0x68: return goods_chain_b90(st, 5) != 0;
    case 0x70: return allowed(st, G_SCENARIO_FLAG_10);
    case 0x71:
        if (goods_chain_d90(st, 0x12) == 0) {
            return 0;
        }
        /* @bin 0x00404ECF reads 0x00E38EA8 / 0x00E38EBC again */
        if ((int8_t) U8(st, G_MISSION) == 5 && S32(st, G_CUSTOM) == 0) {
            return -1;
        }
        return 1;
    case 0x6A: return goods_chain_b90(st, 0x18) != 0;
    case 0xA2: return goods_chain_b90(st, 0x12) != 0;
    case 0xD9: return goods_chain_b90(st, 0x1D) != 0;
    case 0x6C: return goods_chain_b90(st, 0x14) != 0;
    case 0xDD: return goods_chain_b90(st, 0x1E) != 0;
    case 0x6B: return goods_chain_b90(st, 0x19) != 0;
    case 0xD8: return goods_chain_b90(st, 0x1A) != 0;
    case 0xB3: return goods_chain_b90(st, 0x18) != 0;
    case 0xCD: return allowed(st, G_SCENARIO_FLAG_1C);
    case 0xCC:
        /* @bin 0x00404F7C custom = 0x00E38EBC and mission = 0x00E38EA8 as read at 0x00404B13 */
        if (custom == 0 && mission < 7) {
            return 0;
        }
        if ((goods_chain_b90(st, 0xB) || goods_chain_d90(st, 0xB)) && goods_chain_b90(st, 1)) {
            return 1;
        }
        if ((goods_chain_b90(st, 0xB) || goods_chain_d90(st, 0xB)) && goods_trade_city(st, 9)) {
            return 1;
        }
        if (goods_chain_b90(st, 1) && goods_trade_city(st, 0xB)) {
            return 1;
        }
        if (goods_trade_city(st, 9) == 0) {
            return 0;
        }
        return goods_trade_city(st, 0xB) != 0;
    default:
        break;
    }
    return -2; /* not in the chain: the tail of 0x00405020 */
}

static void mission_lists(game_state *st, uint32_t slot, int32_t type, int32_t mission)
{
    /* @bin 0x004050E3 the mission's records {mission, count, pointer to `count` types}: listed
     * types are available iff the record's flag 0x007027B8[record] is set */
    int32_t rec = 0;
    for (uint32_t e = G_MISSION_LISTS; e + 4u < G_MISSION_LISTS_END; e += 0x1Cu, rec++) {
        if (S32(st, e) != mission) {
            continue;
        }
        int32_t n = S32(st, e + 4);
        if (n <= 0) {
            continue;
        }
        uint32_t list = U32(st, e + 8);
        for (int32_t j = 0; j < n; j++) {
            if (type == (int32_t) P32(st, list + 4u * (uint32_t) j)) {
                S32(st, G_MENU_AVAIL + slot) = U8(st, G_MISSION_LIST_FLAGS + (uint32_t) rec) != 0;
            }
        }
    }
}

void build_menu_update(game_state *st)
{
    /* @bin 0x00404350 the availability of every entry of the 38 menus */
    for (int32_t menu = 0; menu < 0x26; menu++) {
        for (int32_t i = 0; i < 30; i++) {
            uint32_t slot = 4u * (uint32_t) (menu * 30 + i);
            if (S32(st, G_FREE_BUILD) == 1 || S32(st, G_EDITOR) == 1) {
                S32(st, G_MENU_AVAIL + slot) = 1; /* @bin 0x00405127 */
                continue;
            }
            int32_t t = S32(st, G_MENU_TYPES_1 + slot);
            S32(st, G_MENU_AVAIL + slot) = 0;
            if (t >= 0xA && t <= 0x1D) {
                S32(st, G_MENU_AVAIL + slot) = 1; /* @bin 0x0040439E houses and the like */
            }
            if (avail_first(st, t)) {
                S32(st, G_MENU_AVAIL + slot) = 1;
            }
            /* @bin 0x00404713 the gate is read for every entry */
            int set;
            if (menu_cheat_gate(st) && t == 4) {
                set = monuments_menu(st);
            } else {
                set = avail_second(st, t);
            }
            if (set) {
                S32(st, G_MENU_AVAIL + slot) = 1;
            }
            /* @bin 0x00404B13 */
            int32_t custom = S32(st, G_CUSTOM);
            int8_t mission = (int8_t) U8(st, G_MISSION);
            if (custom == 0 && mission == 0 && t == 0x37) {
                S32(st, G_MENU_AVAIL + slot) = 0;
                mission_lists(st, slot, t, 0);
                continue;
            }
            int r = avail_third(st, t, custom, mission);
            if (r == -1) {
                mission_lists(st, slot, t, 5); /* @bin 0x004050D1 */
                continue;
            }
            if (r == -2) {
                /* @bin 0x00405020 temple-complex parts and statues of some kinds are never offered
                 * by the menus; the complex add-ons are */
                if ((t >= 0x9C && t <= 0xA0) || (t >= 0x97 && t <= 0x9B) || (t >= 0x91 && t <= 0x95)) {
                    S32(st, G_MENU_AVAIL + slot) = 0;
                }
                if (t == 0xD3 || t == 0xD4) {
                    r = 1;
                }
            }
            if (r == 1) {
                S32(st, G_MENU_AVAIL + slot) = 1; /* @bin 0x004050BA */
            }
            /* @bin 0x004050C0 */
            int8_t m = (int8_t) U8(st, G_MISSION);
            if (m > 10 || S32(st, G_CUSTOM) != 0) {
                continue;
            }
            mission_lists(st, slot, t, m);
        }
    }
}

static int32_t run_update(game_state *st, const int32_t *a)
{
    (void) a;
    build_menu_update(st);
    return 0;
}

static int32_t run_count(game_state *st, const int32_t *a)
{
    (void) a;
    return build_menu_count(st);
}

static int32_t run_gate(game_state *st, const int32_t *a)
{
    (void) a;
    return menu_cheat_gate(st);
}

static int32_t run_placed(game_state *st, const int32_t *a)
{
    (void) a;
    return monuments_placed(st);
}

static int32_t run_kind_open(game_state *st, const int32_t *a) { return monument_kind_open(st, a[0], (uint8_t) a[1]); }
static int32_t run_menu_open(game_state *st, const int32_t *a) { return monument_menu_open(st, a[0]); }

const ported_function menu_ports[] = {
    {0x00404350u, "build_menu_update", 0, 0, 0, run_update, 0},
    {0x00404120u, "build_menu_count", 0, 32, 0, run_count, 0},
    {0x00403EF0u, "menu_cheat_gate", 0, 8, 0, run_gate, 0},
    {0x004F5020u, "monuments_placed", 0, 32, 0, run_placed, 0},
    {0x004F5050u, "monument_kind_open", 2, 8, 0, run_kind_open, 0},
    {0x004F5470u, "monument_menu_open", 1, 8, 0, run_menu_open, 0},
};
const int menu_ports_count = sizeof(menu_ports) / sizeof(menu_ports[0]);
