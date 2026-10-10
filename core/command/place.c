/* Imhotep - the construction place command (package place): 0x004728A0 and its helpers.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x004728A0(player, orientation, x1, y1, x2, y2, type) is what the mouse-up handler 0x00405E90
 * calls to build (docs/commands.md 2.2, 2.4): checks (treasury, figures in the way, scenario special
 * points, terrain), the per-type work, the follow-up triggers, the building creation and the cost.
 * Failures post a notice (0x004E2970) and some restore the bitfields grid (AND 0xAF). */
#include "command/place.h"

#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


/* imul: 32-bit wrap */
static int32_t mul32(int32_t a, int32_t b)
{
    return (int32_t) ((uint32_t) a * (uint32_t) b);
}

static int32_t city(game_state *st)
{
    return (int32_t) CURRENT_CITY(st);
}

static int32_t player_now(game_state *st)
{
    return S32(st, CURRENT_PLAYER_ADDR);
}

static void notice(game_state *st, int32_t id)
{
    GAME_CALL(st, FN_NOTICE_ADD_4E2970, id, 0, 0);
}

/* @bin 0x00472900 the bitfields grid: bits 0x50 cleared (AND 0xAF) */
static void restore_bits(game_state *st)
{
    place_and_bytes(st, G_DAY2_DESTROY_BITS, 0xCB10, 0xAF);
}

/* @bin 0x00473E4D notice, then the bitfields restored */
static void fail(game_state *st, int32_t id)
{
    notice(st, id);
    restore_bits(st);
}

static int32_t offset_of(game_state *st, int32_t x, int32_t y)
{
    return S32(st, G_MAP_START) + y * GRID_SIZE + x;
}

/* @bin 0x00472A4A / 0x004729DA grids restored from the backups of 0x00484F60 */
static void restore_terrain_canals(game_state *st)
{
    GAME_CALL(st, FN_COPY_BYTES, G_TERRAIN_BACKUP, G_GRID_TERRAIN, 0x32C40);
    GAME_CALL(st, FN_COPY_BYTES, G_CANAL_BACKUP, G_CANAL, 0xCB10);
}

void place_construction(game_state *st, int32_t player, int32_t orientation, int32_t x1, int32_t y1, int32_t x2,
                        int32_t y2, int32_t type)
{
    int32_t building = 0; /* @bin 0x004728AE */
    uint8_t roads = place_type_on_roads(type);
    S32(st, G_NOTICE_POSTED) = 0;
    S32(st, G_PLACE_COST) = 0;
    S32(st, G_REFUND) = 0;
    if (type == 0) {
        return;
    }
    /* @bin 0x004728E5 treasury below -4999 */
    if (S32(st, (uint32_t) city(st) + CITY_TREASURY) <= -5000) {
        restore_bits(st);
        notice(st, 3);
        return;
    }
    int32_t x = x2, y = y2; /* @bin 0x00472919 edi, esi */
    if (type != 9 && place_figures_near(st, x1, y1, x2, y2)) {
        /* @bin 0x00472946 */
        if (type == 6 || type == 0xA8 || type == 0xA9 || type == 5 || type == 8) {
            restore_terrain_canals(st);
            GAME_CALL(st, FN_UNDO_RESTORE_IMAGES, 0);
            notice(st, 0x32);
        } else if (type == 0x26 || type == 0x27) {
            restore_terrain_canals(st);
            GAME_CALL(st, FN_COPY_BYTES, G_BITS_BACKUP, G_DAY2_DESTROY_BITS, 0xCB10);
            GAME_CALL(st, FN_COPY_BYTES, G_EDGE_BACKUP, G_EDGE, 0xCB10);
            GAME_CALL(st, FN_UNDO_RESTORE_IMAGES, 0);
            notice(st, 0x32);
        } else if (type == 0x52 || type == 0x53) {
            S32(st, G_BRIDGE_LENGTH) = 0;
            notice(st, 0x32);
        } else {
            restore_bits(st);
            notice(st, 0x32);
        }
        return;
    }
    /* @bin 0x00472A97 obelisks: 0x004E59C0 > 0 */
    if ((type == 0xCF || type == 0xD0) && GAME_CALL0(st, FN_MONUMENT_4E59C0) > 0) {
        fail(st, 0x58);
        return;
    }
    S32(st, G_UBC9720) = 0;
    S32(st, G_PLACE_COST) = S32(st, G_MODEL_BUILDING + 0x28u * (uint32_t) type); /* @bin 0x00472AD1 model cost */

    int drag = 1; /* the cost is multiplied by the drag count */
    switch (type) {
    case 9: /* @bin 0x00472AE0 */
        GAME_CALL(st, FN_CLEAR_LAND, 0, x1, y1, x2, y2);
        break;
    case 6:
    case 0xA8:
    case 0xA9: { /* @bin 0x00472B0F walls */
        if (place_special_point_in(st, x1, y1, x2, y2)) {
            fail(st, 0x3D);
            return;
        }
        int32_t flag = type == 6 ? 0x4000 : type == 0xA8 ? 0x404000 : 0x804000;
        GAME_CALL(st, FN_PLACE_WALLS, 0, x1, y1, x2, y2, flag);
        break;
    }
    case 5: /* @bin 0x00472BAB */
        if (place_special_point_in(st, x1, y1, x2, y2)) {
            fail(st, 0x3D);
            return;
        }
        GAME_CALL(st, FN_PLACE_ROAD, 0, x1, y1, x2, y2);
        break;
    case 0x26: /* @bin 0x00472BE3 */
        GAME_CALL(st, FN_PLACE_PLAZA, 0, x1, y1, x2, y2);
        break;
    case 0x1F:
    case 0x1E:
    case 0x21: { /* @bin 0x00472C10 / 0x00472C86 / 0x00472CE6 */
        int32_t d = type == 0x1F ? 1 : type == 0x1E ? 2 : 3;
        if (place_special_point_in(st, x1, y1, x1 + d, y1 + d)) {
            fail(st, 0x3D);
            return;
        }
        uint32_t fn = type == 0x1F ? FN_PLACE_BOOTH : type == 0x1E ? FN_PLACE_BANDSTAND : FN_PLACE_PAVILION;
        if (GAME_CALL(st, fn, 0, x1, y1, x2, y2) == 0) {
            if (type == 0x1F) {
                restore_bits(st);
            }
            notice(st, 0x4D);
            return;
        }
        GAME_CALL(st, FN_NOTICE_4E5070, type);
        drag = 0;
        break;
    }
    case 0xD1: /* @bin 0x00472D49 one per city */
        if (S16(st, (uint32_t) city(st) + CITY_U4814) != 0) {
            notice(st, 4);
            return;
        }
        if (place_special_point_in(st, x1, y1, x1 + 4, y1 + 4)) {
            fail(st, 0x3D);
            return;
        }
        if (GAME_CALL(st, FN_PLACE_FESTIVAL_SQUARE, 0, x1, y1, x2, y2) == 0) {
            notice(st, 0xD7);
            return;
        }
        drag = 0;
        break;
    case 0x27: { /* @bin 0x00472DCE gardens */
        if (place_special_point_in(st, x1, y1, x2, y2)) {
            fail(st, 0x3D);
            return;
        }
        GAME_CALL(st, FN_PLACE_GARDENS, 0, x1, y1, x2, y2);
        S32(st, G_PLACE_COST) = mul32(S32(st, G_CONSTRUCTION_COUNT), S32(st, G_PLACE_COST));
        int32_t s = S32(st, G_CONSTRUCTION_UI_SIZE);
        GAME_CALL(st, FN_ROUTING_FILL_EE0C50, x - s, y - s, x + s, y + s, 1);
        s = S32(st, G_CONSTRUCTION_UI_SIZE);
        GAME_CALL(st, FN_ROUTING_FILL_EC7630, x - s, y - s, x + s, y + s);
        drag = 0;
        break;
    }
    case 0x52:
    case 0x53: /* @bin 0x00472E54 bridges */
        GAME_CALL(st, FN_PLACE_BRIDGE, x, y, type == 0x53);
        if (S32(st, G_BRIDGE_LENGTH) <= 1) {
            notice(st, 0x19);
            return;
        }
        S32(st, G_PLACE_COST) = mul32(S32(st, G_BRIDGE_LENGTH), S32(st, G_PLACE_COST));
        drag = 0;
        break;
    case 8: /* @bin 0x00472E8D */
        GAME_CALL(st, FN_PLACE_CANAL, 0, x1, y1, x2, y2);
        break;
    case 10: /* @bin 0x00472EA6 */
        if (place_special_point_in(st, x1, y1, x2, y2)) {
            fail(st, 0x3D);
            return;
        }
        GAME_CALL(st, FN_PLACE_HOUSES, 0, x1, y1, x2, y2);
        break;
    default: {
        /* @bin 0x00472ED9 single buildings */
        int32_t mask = -1;
        int32_t size = S32(st, G_MODELS + 0x14u * (uint32_t) type);
        S32(st, G_CONSTRUCTION_UI_SIZE) = size;
        if (type == 0x3A || type == 0xAA || type == 0xAB || type == 0xC8 || type == 0xC9 || type == 0xCA ||
            type == 0x38) {
            mask = -0x41;
        } else if (type == 0x3B || type == 0xAC || type == 0xAD) {
            mask = -0xC04001;
        } else if (type == 0x8A) {
            mask = 8;
        } else if (type == 0x48) {
            size = 3;
        }
        if (place_special_point_in(st, x1, y1, size - 1 + x1, size - 1 + y1)) {
            fail(st, 0x3D);
            return;
        }
        /* @bin 0x00472FF6 the anchor moved by the view orientation */
        if (type != 0xAB && type != 0xCA && orientation != 0) {
            if (orientation == 6) {
                y += 1 - size;
            } else if (orientation == 4) {
                x += 1 - size;
                y += 1 - size;
            } else if (orientation == 2) {
                x += 1 - size;
            }
        }
        /* checks: 0 = on to 0x004738EF, 1 = straight to the follow-ups 0x00473133 */
        int direct = 0;
        if (type == 0x4C || type == 0xB5) { /* @bin 0x004738C3 */
            if (GAME_CALL(st, FN_SHORE_FIT, x, y, 0, 0) != 0) {
                notice(st, 0x19);
                return;
            }
        } else if (type == 7) { /* @bin 0x0047306F water lift: a type 0x5A building */
            if (GAME_CALL(st, FN_SHORE_FIT, x, y, 0, 1) != 0) {
                fail(st, 0x19);
                return;
            }
            int32_t off = offset_of(st, x, y);
            type = 0x5A;
            building = GAME_CALL(st, FN_BUILDING_CREATE, player, 0x5A, x, y, 0, 0);
            GAME_CALL(st, FN_NOTICE_WORKERS_4E2D30, 0x5A);
            GAME_CALL(st, FN_NOTICE_4E32E0, 0x5A, x, y);
            GAME_CALL(st, FN_BUILDING_KEEP_REMOVED, building);
            int32_t h = S32(st, G_MAP_HEIGHT) - 1;
            int32_t w = S32(st, G_MAP_WIDTH) - 1;
            U8(st, G_CANAL + (uint32_t) off) = 0;
            GAME_CALL(st, FN_CANALS_RECT, 0, 0, w, h, 0);
            int32_t s = S32(st, G_CONSTRUCTION_UI_SIZE);
            GAME_CALL(st, FN_ROUTING_FILL_EE0C50, x - s, y - s, x + s, y + s, 1);
            s = S32(st, G_CONSTRUCTION_UI_SIZE);
            GAME_CALL(st, FN_ROUTING_FILL_EC7630, x - s, y - s, x + s, y + s);
            direct = 1;
        } else if (type == 0xAB || type == 0xCA) { /* @bin 0x00473885 gatehouses */
            if ((uint8_t) GAME_CALL(st, FN_PLACE_GATEHOUSE_FITS, x, y, S32(st, G_BUILD_ORIENTATION)) == 0) {
                restore_bits(st);
                notice(st, 2);
                return;
            }
        } else if (type == 0xB6 || type == 0x4A) { /* @bin 0x0047380E */
            if (GAME_CALL(st, FN_SHORE_FIT_47D060, x, y, 0) != 0) {
                notice(st, 0x19);
                return;
            }
            if (place_road_to_entry(st, x, y) == 0) {
                notice(st, 0x38);
                return;
            }
            if (type == 0xB6) {
                if (S32(st, G_WARSHIP_WHARVES) >= 6) {
                    notice(st, 0x54);
                    return;
                }
                direct = 1;
            }
        } else if (type == 0x88) { /* @bin 0x0047327C ferry: validity from the drawing code */
            if (U8(st, G_FERRY_VALID) == 0) {
                if (U8(st, G_FERRY_FLAG) == 0) {
                    notice(st, 0x44);
                }
                return;
            }
            if (GAME_CALL(st, FN_SHORE_FIT, x, y, 0, 0) != 0) {
                notice(st, 0x19);
                return;
            }
            direct = 1;
        } else if (type == 0x4B || type == 0xC0) { /* @bin 0x004737A1 docks */
            if (S32(st, G_DOCK_COUNT) >= 10) {
                notice(st, 0xCE);
                return;
            }
            if (GAME_CALL(st, FN_SHORE_FIT_47D060, x, y, 0) != 0) {
                notice(st, 0x19);
                return;
            }
            if (place_road_to_entry(st, x, y) == 0) {
                notice(st, 0x38);
                return;
            }
        } else if (type == 0xE5 || type == 0xEA || type == 0xEB || type == 0xEC || type == 0xE4 || type == 0xE6) {
            /* @bin 0x004732E9 Cleopatra monuments: (fits at the hovered tile, already exists) on their
             * objects */
            uint32_t obj, fits, exists;
            int32_t again = 0xF5;
            switch (type) {
            case 0xE5: obj = G_MONUMENT_E5; fits = FN_MONUMENT_E5_FITS; exists = FN_MONUMENT_E5_EXISTS; break;
            case 0xEA: obj = G_MONUMENT_EA; fits = FN_MONUMENT_EA_FITS; exists = FN_MONUMENT_EA_EXISTS; break;
            case 0xEB: obj = G_MONUMENT_EB; fits = FN_MONUMENT_EB_FITS; exists = FN_MONUMENT_EB_EXISTS; break;
            case 0xEC: obj = G_MONUMENT_EC; fits = FN_MONUMENT_EC_FITS; exists = FN_MONUMENT_EC_EXISTS; break;
            case 0xE4: obj = G_MONUMENT_E4; fits = FN_MONUMENT_E4_FITS; exists = FN_MONUMENT_E4_EXISTS; again = 0xF4; break;
            default: obj = G_MONUMENT_E6; fits = FN_MONUMENT_E6_FITS; exists = FN_MONUMENT_E6_EXISTS; again = 0xF4; break;
            }
            if ((uint8_t) GAME_CALLT(st, obj, fits, S32(st, G_HOVER_TILE)) == 0) {
                notice(st, type == 0xE4 ? 0xF9 : 0x110);
                notice(st, 0x111);
                return;
            }
            if ((uint8_t) GAME_CALLT0(st, obj, exists) != 0) {
                notice(st, again);
                return;
            }
            direct = 1;
        } else {
            /* @bin 0x00473441 the land check */
            if (GAME_CALL(st, FN_LAND_BLOCK_FITS, x, y, size, size, mask, 0, roads) == 0) {
                fail(st, 2);
                return;
            }
            uint32_t c = (uint32_t) city(st);
            if (type == 0x35) { /* @bin 0x0047346A */
                direct = 1;
                if (S32(st, G_FREE_BUILD) != 1) {
                    if (S32(st, c + CITY_STORAGE_STOCK + 4u * 0x17u) < 500) {
                        fail(st, 0x4F);
                        return;
                    }
                    place_take_goods(st, player_now(st), 0x17, 500);
                    S32(st, G_UNDO_GOODS + 4u * 0x17u) += 500;
                }
            } else if (type == 0xCF || type == 0xD0) { /* @bin 0x004734BE obelisks */
                direct = 1;
                if (S32(st, G_FREE_BUILD) != 1) {
                    int32_t need = type == 0xCF ? 10000 : 20000;
                    if (S32(st, c + CITY_STORAGE_STOCK + 4u * 0x1Au) < need) {
                        fail(st, type == 0xCF ? 0x55 : 0x56);
                        return;
                    }
                    place_take_goods(st, player_now(st), 0x1A, need);
                    S32(st, G_UNDO_GOODS + 4u * 0x1Au) += need;
                }
            } else if (type == 0xA1) { /* @bin 0x0047354F */
                direct = 1;
                int32_t pred = S32(st, G_PREDICATE);
                if (pred != 0 && GAME_CALL(st, FN_TERRAIN_SCAN_PREDICATE, x, y, 2, 1, pred) == 0) {
                    notice(st, 0x16);
                    return;
                }
                if (place_any_building_459ef0(st) == 0) {
                    notice(st, 0x3C);
                }
            } else if (S32(st, G_FLAG_CC) != 0) { /* @bin 0x004735A7 the terrain flags of 0x00405200 */
                if (GAME_CALL(st, FN_TERRAIN_SCAN_MASK, x, y, 3, 1, 0x10800) == 0) {
                    notice(st, 0x14);
                    return;
                }
            } else if (S32(st, G_PREDICATE) != 0) {
                if (GAME_CALL(st, FN_TERRAIN_SCAN_PREDICATE, x, y, 2, 1, S32(st, G_PREDICATE)) == 0) {
                    notice(st, 0x16);
                    return;
                }
            } else if (S32(st, G_FLAG_C8) != 0) {
                if (GAME_CALL(st, FN_TERRAIN_SCAN_MASK, x, y, 2, 1, 1) == 0) {
                    notice(st, 0x17);
                    return;
                }
            } else if (S32(st, G_FLAG_C0) != 0) {
                if (GAME_CALL(st, FN_TERRAIN_SCAN_MASK, x, y, 2, 1, 0x40000) == 0) {
                    notice(st, 0x3A);
                    return;
                }
            } else if (S32(st, G_FLAG_C4) != 0) {
                if (GAME_CALL(st, FN_TERRAIN_SCAN_MASK, x, y, 2, 3, 0x10004) == 0) {
                    notice(st, 0x15);
                    return;
                }
            } else if (S32(st, G_FLAG_BC) != 0) {
                if (GAME_CALL(st, FN_TERRAIN_ALL_MASK, x, y, 2, 0, 0x4000) == 0) {
                    notice(st, 0x31);
                    return;
                }
            } else if (S32(st, G_FLAG_B8) != 0) {
                int32_t off = offset_of(st, x, y);
                if (GAME_CALL(st, FN_TERRAIN_ALL_MASK, x, y, 1, 0, 0x40) == 0) {
                    notice(st, 0x42);
                    return;
                }
                if (U8(st, G_SPRITE + (uint32_t) off) != 0) { /* @bin 0x00473735 sprite grid */
                    notice(st, 2);
                    return;
                }
            } else if (S32(st, G_FLAG_B4) != 0) {
                int32_t flag = S32(st, G_FLAG_B4);
                if (GAME_CALL(st, FN_TERRAIN_COUNT_MASK, x, y, S32(st, G_MODELS + 0x14u * (uint32_t) type), 0, 0x80,
                              flag) == 0) {
                    notice(st, 0x4C);
                    return;
                }
            }
        }
        if (!direct) {
            /* @bin 0x004738EF forts: the parade ground, at most 6 */
            if (type == 0x28 || type == 0x2C || type == 0x2D) {
                if (GAME_CALL(st, FN_LAND_BLOCK_FITS, x + 3, y - 1, 4, 4, mask, 0, roads) == 0) {
                    notice(st, 2);
                    return;
                }
                if (S32(st, G_FORMATION_COUNT) >= 6) {
                    notice(st, 0x34);
                    return;
                }
            }
            uint32_t c = (uint32_t) city(st);
            if (type == 0xD2) { /* @bin 0x00473955 one per city, two 6x6 blocks */
                if (S16(st, c + CITY_U433A) != 0) {
                    notice(st, 4);
                    return;
                }
                if (GAME_CALL(st, FN_LAND_BLOCK_FITS, x + 6, y, 6, 6, mask, 0, roads) == 0) {
                    notice(st, 2);
                    return;
                }
                if (GAME_CALL(st, FN_LAND_BLOCK_FITS, x + 12, y, 6, 6, mask, 0, roads) == 0) {
                    notice(st, 2);
                    return;
                }
                /* @bin 0x004739F7 the bottom edge from y1, not the anchor */
                if (place_special_point_in(st, x + 6, y, x + 0xB, y1 + 5)) {
                    fail(st, 0x3D);
                    return;
                }
                if (place_special_point_in(st, x + 12, y, x + 0x11, y1 + 5)) {
                    fail(st, 0x3D);
                    return;
                }
            } else if (type == 0xD5) { /* @bin 0x00473A40 */
                if ((uint8_t) GAME_CALL(st, FN_PLACE_MONUMENT_D5_FITS, S32(st, G_HOVER_TILE)) == 0) {
                    notice(st, 2);
                    return;
                }
                if ((uint8_t) GAME_CALL0(st, FN_PLACE_MONUMENT_D5_DONE) != 0) {
                    notice(st, 0x5B);
                    return;
                }
                if (S32(st, G_FREE_BUILD) == 0) {
                    if (S32(st, (uint32_t) city(st) + CITY_STORAGE_STOCK + 4u * 0x1Eu) < 22000) {
                        notice(st, 0x5A);
                        return;
                    }
                    place_take_goods(st, player_now(st), 0x1E, 22000);
                    S32(st, G_UNDO_GOODS + 4u * 0x1Eu) += 22000;
                }
            } else if (type == 0xE1 || type == 0xE3) { /* @bin 0x00473AD4 */
                uint32_t obj = type == 0xE1 ? G_MONUMENT_E1 : G_MONUMENT_E3;
                if ((uint8_t) GAME_CALLT(st, obj, type == 0xE1 ? FN_MONUMENT_E1_FITS : FN_MONUMENT_E3_FITS, S32(st, G_HOVER_TILE)) == 0) {
                    notice(st, 2);
                    return;
                }
                if ((uint8_t) GAME_CALLT0(st, obj, type == 0xE1 ? FN_MONUMENT_E1_EXISTS : FN_MONUMENT_E3_EXISTS) != 0) {
                    notice(st, type == 0xE1 ? 0xF3 : 0xF5);
                    return;
                }
            } else if (type == 0xDE) { /* @bin 0x00473B8E at (x1, y1) */
                if ((uint8_t) GAME_CALL(st, FN_PLACE_MONUMENT_DE_FITS, offset_of(st, x1, y1)) == 0) {
                    notice(st, 2);
                    return;
                }
                if (S32(st, G_FREE_BUILD) == 0) {
                    if (S32(st, (uint32_t) city(st) + CITY_STORAGE_STOCK + 4u * 0x1Eu) < 24000) {
                        notice(st, 0xBE);
                        return;
                    }
                    place_take_goods(st, player_now(st), 0x1E, 24000);
                    S32(st, G_UNDO_GOODS + 4u * 0x1Eu) += 24000;
                }
            } else {
                if (type >= 0x41 && type <= 0x45) { /* @bin 0x00473C32 temple complexes */
                    if (S16(st, c + CITY_U4478) != 0) {
                        fail(st, 4);
                        return;
                    }
                    int32_t r = GAME_CALL(st, FN_PLACE_TEMPLE_COMPLEX_FITS, x, y);
                    if (r == -1) {
                        fail(st, 2);
                        return;
                    }
                    if (r == -2) {
                        fail(st, 0x3D);
                        return;
                    }
                }
                if (type == 0xD3 || type == 0xD4) { /* @bin 0x00473C80 complex add-ons */
                    int32_t b = GAME_CALL(st, FN_BUILDING_MAIN_PART, (int32_t) S16(st, G_BUILDING_GRID + 2u * U32(st, G_HOVER_TILE)));
                    c = (uint32_t) city(st);
                    if (S16(st, c + CITY_U4478) == 0) {
                        fail(st, 0x49);
                        return;
                    }
                    int32_t bt = S16(st, BUILDING_ADDR(b) + BLD_TYPE);
                    if (bt < 0x41 || bt > 0x45) {
                        fail(st, 0x4B);
                        return;
                    }
                    if (S16(st, c + CITY_U448A) < 2) {
                        fail(st, 0x4E);
                        return;
                    }
                    if ((uint8_t) GAME_CALL(st, FN_BUILDING_0XA9_IS, S32(st, c + CITY_U472C), type == 0xD3 ? 1 : 2) != 0) {
                        fail(st, 0x4A);
                        return;
                    }
                } else { /* @bin 0x00473DA0 */
                    c = (uint32_t) city(st);
                    if ((type == 0xBD || type == 0xBC || type == 0xBB) && S16(st, c + CITY_U4486) != 0) {
                        fail(st, 4);
                        return;
                    }
                    if ((type == 0x4F || type == 0x4E || type == 0x4D) && S32(st, c + CITY_MANSION_BUILT) != 0) {
                        fail(st, 4);
                        return;
                    }
                    if (type == 0x56) {
                        if (S16(st, c + CITY_U4486) == 0) {
                            fail(st, 0x59);
                            return;
                        }
                    } else if (type == 0x5F) {
                        if (S32(st, G_DAY_BUILDINGS_TYPE_COUNT + 4u * 0x5Fu) > 0) {
                            fail(st, 4);
                            return;
                        }
                        S32(st, G_DAY_BUILDINGS_TYPE_COUNT + 4u * 0x5Fu) = 1;
                    } else if (type == 0x5E) {
                        if (S32(st, c + CITY_ACADEMY_EXISTS) != 0) {
                            fail(st, 4);
                            return;
                        }
                    } else if (type == 0xB7) { /* @bin 0x00473E71 */
                        int32_t reason = x1;
                        if (place_pyramid_fits(st, offset_of(st, x, y), S32(st, G_VARIANT), &reason) == 0) {
                            notice(st, reason == 2 ? 0xD6 : 0xD5);
                            return;
                        }
                    }
                }
            }
        }
        /* @bin 0x00473133 the follow-up triggers, with the type (0x5A for a water lift) */
        GAME_CALL(st, FN_NOTICE_4E3330, type);
        GAME_CALL(st, FN_NOTICE_WORKERS_4E2D30, type);
        GAME_CALL(st, FN_NOTICE_4E3100, type);
        GAME_CALL(st, FN_NOTICE_4E3140, type);
        GAME_CALL(st, FN_NOTICE_4E3180, type);
        GAME_CALL(st, FN_NOTICE_4E31C0, type);
        GAME_CALL(st, FN_NOTICE_4E3200, type);
        GAME_CALL(st, FN_NOTICE_4E2E20, type);
        GAME_CALL(st, FN_NOTICE_4E3240, type);
        GAME_CALL(st, FN_NOTICE_4E3280, type, x, y, size);
        GAME_CALL(st, FN_NOTICE_4E32E0, type, x, y);
        GAME_CALL(st, FN_NOTHING, type);
        GAME_CALL(st, FN_NOTICE_4E33A0, type);
        GAME_CALL(st, FN_NOTICE_4E3460, type);
        GAME_CALL(st, FN_NOTICE_4E3520, type);
        GAME_CALL(st, FN_NOTICE_4E35E0, type);
        GAME_CALL(st, FN_NOTICE_4E36A0, type);
        GAME_CALL(st, FN_NOTICE_4E3760, type);
        GAME_CALL(st, FN_NOTICE_4E3AA0, type);
        GAME_CALL(st, FN_NOTICE_4E39D0, type);
        GAME_CALL(st, FN_NOTICE_4E3CF0, type);
        GAME_CALL(st, FN_NOTICE_4E3DA0, type);
        GAME_CALL(st, FN_NOTICE_4E3F60, type);
        GAME_CALL(st, FN_NOTICE_4E4120, type);
        GAME_CALL(st, FN_NOTICE_4E42D0, type);
        GAME_CALL(st, FN_NOTICE_4E4490, type);
        GAME_CALL(st, FN_NOTICE_4E4550, type);
        GAME_CALL(st, FN_NOTICE_4E47D0, type);
        GAME_CALL(st, FN_NOTICE_4E4E10, type);
        GAME_CALL(st, FN_NOTICE_4E4ED0, type);
        GAME_CALL(st, FN_NOTICE_4E5070, type);
        GAME_CALL(st, FN_NOTICE_ACCESS_4E2B10, type, x, y, size);
        /* @bin 0x00473208 the building: forts start with their type 0x39 part; monuments with their
         * part counts (not recorded for undo) */
        int monument = 0;
        U8(st, G_FROZEN) = 0;
        if (type == 0x28 || type == 0x2C || type == 0x2D) {
            building = GAME_CALL(st, FN_BUILDING_CREATE, player, 0x39, x, y, 0, 0);
        } else if (type == 0xB7) {
            building = GAME_CALL(st, FN_BUILDING_CREATE, player, 0xB7, x, y, 0x90, 0);
        } else if (type != 0x5A) {
            int32_t parts = 0;
            switch (type) {
            case 0xD5: parts = 100; break;
            case 0xE6: parts = 0x10; break;
            case 0xE1: parts = 0x19; break;
            case 0xE3: parts = 9; break;
            case 0xE5: parts = 0xA7; break;
            case 0xEA: parts = 0x9E; break;
            case 0xEB: parts = 0x1B5; break;
            case 0xEC: parts = 0x201; break;
            case 0xE4: parts = 2; break;
            case 0xDE: parts = 0x74; break;
            default: break;
            }
            monument = parts != 0;
            building = GAME_CALL(st, FN_BUILDING_CREATE, player, type, x, y, parts, 0);
        }
        if (building == 0) {
            return;
        }
        if (!monument) {
            GAME_CALL(st, FN_BUILDING_KEEP_REMOVED, building);
        }
        if (building <= 0) {
            return;
        }
        GAME_CALL(st, FN_BUILDING_MAP_ADD, player, type, building, x, y, S32(st, G_CONSTRUCTION_UI_SIZE), 1, 0);
        if (type == 0x88) {
            GAME_CALL(st, FN_FERRY_PLACE_STEP, building);
        }
        drag = 0;
        break;
    }
    }
    if (drag) {
        S32(st, G_PLACE_COST) = mul32(S32(st, G_CONSTRUCTION_COUNT), S32(st, G_PLACE_COST)); /* @bin 0x00472AF2 */
    }
    /* @bin 0x004740B7 the common tail */
    GAME_CALL0(st, FN_ROUTING_REBUILD_EE0C50);
    GAME_CALL0(st, FN_ROUTING_REBUILD_EC7630);
    GAME_CALL0(st, FN_ROUTING_REBUILD_EBAB20);
    place_formations_alert(st, x, y);
    int32_t cost = (type == 0x88 && S32(st, G_FERRY_STATE) == 0) ? 0 : S32(st, G_PLACE_COST); /* @bin 0x004740D0 */
    cost -= S32(st, G_REFUND);
    S32(st, G_PLACE_COST) = cost;
    place_spend(st, player, cost);
    if (type != 0x38 && type != 9) {
        place_undo_cost(st);
    }
    place_monument_slots(st);
}

void place_and_bytes(game_state *st, uint32_t dst, int32_t n, uint8_t value)
{
    /* @bin 0x0040F920 blocks of 0x40 bytes as dwords, then bytes: the same as bytewise */
    for (int32_t i = 0; i < n; i++) {
        U8(st, dst + (uint32_t) i) &= value;
    }
}

uint8_t place_type_on_roads(int32_t type)
{
    /* @bin 0x0046FDE0 jump table 0x0046FE04 / index 0x0046FE0C over types 5..0xE0 */
    switch (type) {
    case 5: case 8: case 9: case 0x5A: case 0x64: case 0x65: case 0x66: case 0x67: case 0x68: case 0x69:
    case 0xC4: case 0xE0:
        return 1;
    default:
        return 0;
    }
}

uint8_t place_special_point_in(game_state *st, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    int32_t xmin = x1 < x2 ? x1 : x2, xmax = x1 > x2 ? x1 : x2;
    int32_t ymin = y1 < y2 ? y1 : y2, ymax = y1 > y2 ? y1 : y2;
    /* @bin 0x00470933 four points of i16 x 0x00784B64[], y 0x00784B6C[] */
    for (uint32_t i = 0; i < 4; i++) {
        int32_t px = S16(st, G_HERD_POINTS_X + 2 * i), py = S16(st, G_HERD_POINTS_Y + 2 * i);
        if (px >= xmin && px <= xmax && py >= ymin && py <= ymax) {
            return 1;
        }
    }
    /* @bin 0x00470959 four points of i32 x 0x00784CC0[], y 0x00784CD0[] */
    for (uint32_t i = 0; i < 4; i++) {
        int32_t px = S32(st, G_HERD2_POINTS_X + 4 * i), py = S32(st, G_HERD2_POINTS_Y + 4 * i);
        if (px >= xmin && px <= xmax && py >= ymin && py <= ymax) {
            return 1;
        }
    }
    return 0;
}

static int32_t absdiff(int32_t a, int32_t b)
{
    return a > b ? a - b : b - a;
}

int32_t place_figures_near(game_state *st, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    /* @bin 0x004B583C figures 1..1999 of state 1 (+0x0E), type (+0x0A, u8) 0x2B..0x39 or 0x63, tile
     * +0x14 / +0x16 within 12 of (x1, y1) or (x2, y2) */
    for (uint32_t f = 1; f < FIGURE_COUNT; f++) {
        uint32_t r = FIGURE_ADDR(f);
        if (U8(st, r + FIG_STATE) != 1) {
            continue;
        }
        uint8_t t = U8(st, r + FIG_TYPE);
        if (!((t >= 0x2B && t <= 0x39) || t == 0x63)) {
            continue;
        }
        int32_t fx = S16(st, r + FIG_X), fy = S16(st, r + FIG_Y);
        if (absdiff(fx, x1) <= 12 && absdiff(fy, y1) <= 12) {
            return 1;
        }
        if (absdiff(fx, x2) <= 12 && absdiff(fy, y2) <= 12) {
            return 1;
        }
    }
    return 0;
}

void place_undo_cost(game_state *st)
{
    /* @bin 0x00485200 */
    int32_t cost = S32(st, G_PLACE_COST);
    S32(st, G_UNDO_READY) = 1;
    S32(st, G_UNDO_COUNTDOWN) = 500;
    S32(st, G_UNDO_COST) = cost;
    U8(st, G_REDRAW) = 1;
}

void place_spend(game_state *st, int32_t player, int32_t amount)
{
    /* @bin 0x004E9680 a player city (+0x2 == 1) outside the editor (0x00605A3C): treasury +0xC,
     * construction spending +0x2C8C */
    uint32_t c = CITY_ADDR(player);
    if (U8(st, c + CITY_ACTIVE) != 1 || S32(st, G_EDITOR) == 1) {
        return;
    }
    S32(st, c + CITY_TREASURY) -= amount;
    S32(st, c + CITY_CONSTRUCTION_THIS_YEAR) += amount;
}

void place_monument_slots(game_state *st)
{
    /* @bin 0x004F61E0 slots CITY+0x4730[3] (kind) and +0x473C[3] (building) cleared, then every
     * monument building (state 1 or 3, +0x16 != -1) whose kind 0x004F4B70 is > 0 takes the first
     * slot of 0x00784DE0[3] (i16) with that kind and an empty building; the loop skips the parts */
    uint32_t c = CURRENT_CITY(st);
    for (uint32_t i = 0; i < 3; i++) {
        S32(st, c + CITY_MONUMENT_CODE + 4 * i) = 0;
        S32(st, c + CITY_MONUMENT_ID + 4 * i) = 0;
    }
    for (int32_t i = 1; i <= S32(st, G_BUILDING_HIGHEST); i++) {
        uint32_t b = BUILDING_ADDR(i);
        uint8_t state = U8(st, b + BLD_STATE);
        if (state != 1 && state != 3) {
            continue;
        }
        if (S16(st, b + BLD_SEQUENCE) == -1) {
            continue;
        }
        int32_t t = S16(st, b + BLD_TYPE);
        int32_t id = i;
        int32_t kind;
        switch (t) {
        case 0xB7: {
            kind = GAME_CALL(st, FN_MONUMENT_KIND_4F4B70, i);
            /* @bin 0x004F626E the pyramid's part count by its variant +0x54 */
            uint32_t table = S32(st, G_PYRAMID_KIND) == 5 ? G_PYRAMID_PARTS_KIND5 : G_PYRAMID_PARTS;
            i += S32(st, table + 4u * U8(st, b + BLD_MONUMENT_KIND)) - 1;
            break;
        }
        case 0xD5: kind = GAME_CALL(st, FN_MONUMENT_KIND_4F4B70, i); i += 0x63; break;
        case 0xE3: kind = GAME_CALL(st, FN_MONUMENT_KIND_4F4B70, i); i += 8; break;
        case 0xE5: kind = GAME_CALL(st, FN_MONUMENT_KIND_4F4B70, i); i += 0xA6; break;
        case 0xEA: kind = GAME_CALL(st, FN_MONUMENT_KIND_4F4B70, i); i += 0x9D; break;
        case 0xEB: kind = GAME_CALL(st, FN_MONUMENT_KIND_4F4B70, i); i += 0x1B4; break;
        case 0xEC: kind = GAME_CALL(st, FN_MONUMENT_KIND_4F4B70, i); i += 0x200; break;
        case 0xE1: kind = GAME_CALL(st, FN_MONUMENT_KIND_4F4B70, i); i += 0x18; break;
        case 0xE6: kind = GAME_CALL(st, FN_MONUMENT_KIND_4F4B70, i); i += 0xF; break;
        case 0xDE: kind = GAME_CALL(st, FN_MONUMENT_KIND_4F4B70, i); i += 0x73; break;
        case 0xD2:
            if (S16(st, b + BLD_PREV_PART) != 0) {
                continue;
            }
            kind = GAME_CALL(st, FN_MONUMENT_KIND_4F4B70, i);
            break;
        case 0xCF:
        case 0xD0:
        case 0xE4:
            kind = GAME_CALL(st, FN_MONUMENT_KIND_4F4B70, i);
            break;
        default:
            continue;
        }
        if (kind <= 0) {
            continue;
        }
        uint32_t p = U32(st, CURRENT_PLAYER_ADDR);
        for (uint32_t k = 0; k < 3; k++) {
            uint32_t cc = CITY_ADDR(p);
            if (kind == S16(st, G_MONUMENT_SLOTS + 2 * k) && S32(st, cc + CITY_MONUMENT_ID + 4 * k) == 0) {
                S32(st, cc + CITY_MONUMENT_CODE + 4 * k) = kind;
                S32(st, cc + CITY_MONUMENT_ID + 4 * k) = id;
                break;
            }
        }
    }
}

void place_formations_alert(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x004B9EE0 formations 1..49 (0x90 bytes at 0x00BCB9C8): in use (+0), +0x42 == 0, +0x69
     * set, +0x28 > 0, within 6 tiles (Chebyshev; the distances kept in 0x00D3B4FC / 0x00D3B4F8):
     * +0x4E := 0x32 and +0x76 := the direction 0x0051CBA0 from the tile to the formation */
    for (uint32_t f = 1; f < 50; f++) {
        uint32_t r = G_FORMATION_BASE + 0x90u * f;
        if (U8(st, r) != 1 || U8(st, r + 0x42) != 0 || U8(st, r + 0x69) == 0 || U8(st, r + 0x28) == 0) {
            continue;
        }
        int32_t fx = S16(st, r + 0x2E), fy = S16(st, r + 0x30);
        int32_t dx = absdiff(x, fx);
        S32(st, G_DIST_X) = dx;
        int32_t dy = absdiff(y, fy);
        S32(st, G_DIST_Y) = dy;
        if ((dx >= dy ? dx : dy) > 6) {
            continue;
        }
        S16(st, r + 0x4E) = 0x32;
        U8(st, r + 0x76) = (uint8_t) GAME_CALL(st, FN_ROUTE_DIRECTION, x, y, (int32_t) S16(st, r + 0x2E),
                                               (int32_t) S16(st, r + 0x30));
    }
}

uint8_t place_pyramid_fits(game_state *st, int32_t offset, int32_t variant, int32_t *reason)
{
    /* @bin 0x004ECEC0 the pyramid's pieces (count by variant: 0x0056F7EC[] in mode 5 of 0x005E4EF4,
     * else 0x0056F7D8[]): each piece's anchor (mode 5: x, y pairs 0x0056FD10; else offsets
     * 0x0056FE30) + offset, then 0x005CFCDC tiles of it (offsets 0x005CFE10 by view orientation / 2):
     * 0x0046F9A0(x, y, 3) set → 0 at once; terrain & 0xFEFFD76E → reason bit 1 */
    uint8_t ok = 1;
    int32_t tiles = S32(st, G_PYRAMID_PIECE_TILES);
    int32_t view = S32(st, G_VIEW_ORIENTATION) / 2;
    *reason = 0;
    uint32_t pair = G_PYRAMID_ANCHORS_XY, single = G_PYRAMID_ANCHORS;
    for (int32_t i = 0;; i++, pair += 8, single += 4) {
        int mode5 = S32(st, G_PYRAMID_KIND) == 5;
        int32_t count = S32(st, (mode5 ? G_PYRAMID_PARTS_KIND5 : G_PYRAMID_PARTS) + 4u * (uint32_t) variant);
        if (i >= count) {
            break;
        }
        int32_t base = mode5 ? S32(st, pair) + S32(st, pair + 4) * GRID_SIZE : S32(st, single);
        int32_t t = base + offset;
        for (int32_t j = 0; j < tiles; j++) {
            if ((uint8_t) GAME_CALL(st, FN_MAP_NEAR_EDGE, t % GRID_SIZE, t / GRID_SIZE, 3) != 0) {
                return 0;
            }
            int32_t o = S32(st, G_FOOT_TILES + 4u * (uint32_t) (j + 36 * view)) + t;
            if (U32(st, G_GRID_TERRAIN + 4u * (uint32_t) o) & 0xFEFFD76Eu) {
                ok = 0;
                *reason |= 1;
            }
        }
    }
    /* @bin 0x004ED003 0x004ECEA0(variant): the entry 0x005F5D08[variant] must reach (0x004ED360,
     * result 0x00EAAE9C) → else reason bit 2 */
    if ((uint8_t) GAME_CALL(st, FN_PYRAMID_KIND_OK, variant) != 0) {
        int32_t t = S32(st, G_PYRAMID_RAMP + 4u * (uint32_t) variant) + offset - S32(st, G_MAP_START);
        GAME_CALL(st, FN_RAMP_WALK, t % GRID_SIZE, t / GRID_SIZE, 0, 0);
        if (S32(st, G_RAMP_LAST_TILE) == 0) {
            ok = 0;
            *reason |= 2;
        }
    }
    /* @bin 0x004ED064 a row of tiles (0x005700A8 / 0x005700C8 in mode 5, else 0x00570094 /
     * 0x005700B4) with terrain & 0xFEFFD72E → reason bit 4 */
    int32_t n, from;
    if (S32(st, G_PYRAMID_KIND) == 5) {
        n = S32(st, G_PYRAMID_ROW_COUNT5 + 4u * (uint32_t) variant);
        from = S32(st, G_PYRAMID_ROW_FROM5 + 4u * (uint32_t) variant);
    } else {
        n = S32(st, G_PYRAMID_ROW_COUNT + 4u * (uint32_t) variant);
        from = S32(st, G_PYRAMID_ROW_FROM + 4u * (uint32_t) variant);
    }
    uint32_t p = G_GRID_TERRAIN + 4u * (uint32_t) (from + offset);
    for (; n > 0; n--, p += 4) {
        if (U32(st, p) & 0xFEFFD72Eu) {
            ok = 0;
            *reason |= 4;
        }
    }
    return ok;
}

int32_t place_take_goods(game_state *st, int32_t player, int32_t resource, int32_t amount)
{
    /* @bin 0x0045B4D0 from the storage yards (0x48, 0xC1, state 1) after the round-robin index
     * CITY+0x2752 (i16, 1..3999), 3999 steps or until all taken (0x0045B570 returns what remains) */
    uint32_t c = CITY_ADDR(player);
    int32_t b = S16(st, c + CITY_STORAGE_CURSOR);
    int32_t left = amount;
    for (int32_t n = 1; n < 4000 && left > 0; n++) {
        b++;
        if (b >= 4000) {
            b = 1;
        }
        uint32_t r = BUILDING_ADDR(b);
        if (U8(st, r + BLD_STATE) != 1) {
            continue;
        }
        int16_t t = S16(st, r + BLD_TYPE);
        if (t != 0x48 && t != 0xC1) {
            continue;
        }
        S16(st, c + CITY_STORAGE_CURSOR) = (int16_t) b;
        left = GAME_CALL(st, FN_STORAGE_TAKE, player, b, resource, left);
    }
    return amount - left;
}

int32_t place_road_to_entry(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00457AC0 road distances from the entry point 0x00784C8C / 0x00784C8E, then the tile's
     * reach 0x00481420(x, y, 3) */
    GAME_CALL(st, FN_ROUTE_DISTANCES_ED4140, (int32_t) S16(st, G_ENTRY_X), (int32_t) S16(st, G_ENTRY_Y));
    return GAME_CALL(st, FN_ROUTE_ROAD_REACHED, x, y, 3) != 0;
}

uint8_t place_any_building_459ef0(game_state *st)
{
    /* @bin 0x00459FA0 buildings 1..0x0078D790 of state 1 */
    for (int32_t i = 1; i <= S32(st, G_BUILDING_HIGHEST); i++) {
        if (U8(st, BUILDING_ADDR(i) + BLD_STATE) == 1 && GAME_CALL(st, FN_BUILDING_TYPE_0X459EF0, i) != 0) {
            return 1;
        }
    }
    return 0;
}

static int32_t run_place(game_state *st, const int32_t *a)
{
    place_construction(st, a[0], a[1], a[2], a[3], a[4], a[5], a[6]);
    return 0;
}

static int32_t run_and_bytes(game_state *st, const int32_t *a)
{
    place_and_bytes(st, (uint32_t) a[0], a[1], (uint8_t) a[2]);
    return 0;
}

static int32_t run_on_roads(game_state *st, const int32_t *a)
{
    (void) st;
    return place_type_on_roads(a[0]);
}

static int32_t run_special_point(game_state *st, const int32_t *a)
{
    return place_special_point_in(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_figures_near(game_state *st, const int32_t *a)
{
    return place_figures_near(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_undo_cost(game_state *st, const int32_t *a)
{
    (void) a;
    place_undo_cost(st);
    return 1; /* @bin 0x00485206 eax = 1 */
}

static int32_t run_spend(game_state *st, const int32_t *a)
{
    place_spend(st, a[0], a[1]);
    return 0;
}

static int32_t run_monument_slots(game_state *st, const int32_t *a)
{
    (void) a;
    place_monument_slots(st);
    return 0;
}

static int32_t run_formations_alert(game_state *st, const int32_t *a)
{
    place_formations_alert(st, a[0], a[1]);
    return 0;
}

static int32_t run_pyramid_fits(game_state *st, const int32_t *a)
{
    int32_t reason = 0;
    uint8_t r = place_pyramid_fits(st, a[0], a[1], &reason);
    if ((uint32_t) a[2] >= STATE_BASE && (uint32_t) a[2] + 4 <= STATE_END) {
        S32(st, (uint32_t) a[2]) = reason;
    }
    return r;
}

static int32_t run_take_goods(game_state *st, const int32_t *a)
{
    return place_take_goods(st, a[0], a[1], a[2]);
}

static int32_t run_road_to_entry(game_state *st, const int32_t *a)
{
    return place_road_to_entry(st, a[0], a[1]);
}

static int32_t run_any_459ef0(game_state *st, const int32_t *a)
{
    (void) a;
    return place_any_building_459ef0(st);
}

const ported_function place_ports[] = {
    {0x004728A0u, "place_construction", 7, 0, 0, run_place, 0},
    {0x0040F920u, "place_and_bytes", 3, 0, 0, run_and_bytes, 0},
    {0x0046FDE0u, "place_type_on_roads", 1, 8, 0, run_on_roads, 0},
    {0x00470900u, "place_special_point_in", 4, 8, 0, run_special_point, 0},
    {0x004B5830u, "place_figures_near", 4, 32, 0, run_figures_near, 0},
    {0x00485200u, "place_undo_cost", 0, 32, 0, run_undo_cost, 0},
    {0x004E9680u, "place_spend", 2, 0, 0, run_spend, 0},
    {0x004F61E0u, "place_monument_slots", 0, 0, 0, run_monument_slots, 0},
    {0x004B9EE0u, "place_formations_alert", 2, 0, 0, run_formations_alert, 0},
    {0x004ECEC0u, "place_pyramid_fits", 3, 8, 0, run_pyramid_fits, 0},
    {0x0045B4D0u, "place_take_goods", 3, 32, 0, run_take_goods, 0},
    {0x00457AC0u, "place_road_to_entry", 2, 32, 0, run_road_to_entry, 0},
    {0x00459FA0u, "place_any_building_459ef0", 0, 8, 0, run_any_459ef0, 0},
};
const int place_ports_count = sizeof(place_ports) / sizeof(place_ports[0]);
