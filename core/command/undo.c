/* Imhotep - undo of the last construction or clearing (player command, docs/commands.md 2.6):
 * 0x00485230 and its callees. The snapshot 0x00484F60 belongs to the construction start.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "command/undo.h"

#include "command/clear.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

/* @bin 0x00484F60 the grids' backups taken at the construction start */

void undo_forget(game_state *st)
{
    /* @bin 0x00485080 */
    int32_t n = S32(st, G_UNDO_COUNT);
    for (int32_t i = 0; i < n; i++) {
        int16_t id = S16(st, G_UNDO_IDS + 2u * (uint32_t) i);
        if (id == 0) {
            continue;
        }
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) == 6) {
            U8(st, b + BLD_STATE) = 1;
        }
        U8(st, b + BLD_UBF) = 0;
    }
    S32(st, G_UNDO_COUNT) = 0;
    GAME_CALL(st, FN_FILL_BYTES, (int32_t) G_UNDO_RECORDS, 0x21D68, 0);
    GAME_CALL(st, FN_FILL_BYTES, (int32_t) G_UNDO_IDS, 0x41A, 0);
}

void undo_restore_images(game_state *st, int32_t unused)
{
    /* @bin 0x00485830 */
    (void) unused;
    int32_t h = S32(st, G_MAP_HEIGHT);
    for (int32_t y = 0; y < h; y++) {
        int32_t xs = S32(st, G_ROW_FIRST + 4u * (uint32_t) y);
        int32_t xe = S32(st, G_ROW_END + 4u * (uint32_t) y);
        uint32_t off = U32(st, G_ROW_OFFSET + 4u * (uint32_t) y);
        for (int32_t n = xs; n < xe; n++, off++) {
            if (S16(st, G_BUILDING_GRID + 2u * off) == 0) {
                U32(st, G_IMAGE + 4u * off) = U32(st, G_IMAGE_BACKUP + 4u * off);
            }
        }
    }
}

void undo_houses_refresh(game_state *st)
{
    /* @bin 0x00485890 */
    for (int32_t id = 1; id < BUILDING_COUNT; id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) == 0) {
            continue;
        }
        int16_t t = S16(st, b + BLD_TYPE);
        if (t >= 0x0A && t <= 0x1D && S16(st, b + BLD_POPULATION) == 0) {
            GAME_CALL(st, FN_HOUSE_SPLIT_46CF40, id);
        }
    }
}

void undo_monument_de_redraw(game_state *st, int32_t id)
{
    /* @bin 0x004E0C80 */
    uint32_t b = BUILDING_ADDR(id);
    if (S16(st, b + BLD_TYPE) != 0xDE) {
        return;
    }
    int32_t base = id - S16(st, b + BLD_PART);
    for (int32_t i = 0; i < 0x74; i++) {
        GAME_CALL(st, FN_MONUMENT_PART_REDRAW, base + i);
    }
}

void undo_building_readd(game_state *st, int32_t id)
{
    /* @bin 0x004858E0 */
    if (id <= 0) {
        return;
    }
    uint32_t b = BUILDING_ADDR(id);
    uint32_t city = CURRENT_CITY(st);
    /* @bin 0x00485902 the size from the model table 0x005D06DC (0x14 bytes per type) */
    int32_t size = S32(st, G_MODELS + 0x14u * (uint32_t) (int32_t) S16(st, b + BLD_TYPE));
    if ((uint8_t) GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_IS_TYPE_0X487F80, id) != 0) {
        GAME_CALLT(st, G_BUILDING_MANAGER, FN_FARM_REDRAW_488E60, id, 0);
    } else {
        int16_t t = S16(st, b + BLD_TYPE);
        if ((t >= 0x41 && t <= 0x45) || t == 0xCA) {
            GAME_CALL(st, FN_BUILDING_MAP_ADD, S32(st, CURRENT_PLAYER_ADDR), t, id, S16(st, b + BLD_X), S16(st, b + BLD_Y), size, 0,
                      0);
        } else {
            GAME_CALL(st, FN_MAP_BUILDING_TILES, id, S16(st, b + BLD_X), S16(st, b + BLD_Y), size, size, 0, 0);
        }
    }
    int16_t t = S16(st, b + BLD_TYPE);
    if (t == 0x4C || t == 0xB5 || t == 0xB6) {
        S16(st, b + BLD_SECOND_WALKER) = 0;
    }
    if (U8(st, b + BLD_HOUSE_SIZE) != 0) {
        S16(st, b + BLD_POPULATION) = 0;
    }
    /* @bin 0x004859B8 the links freed by the clearing (0x00474700) taken again, or new ones made */
    t = S16(st, b + BLD_TYPE);
    if (t == 0x47 || t == 0x48 || t == 0x46 || t == 0xBF || t == 0xC1 || t == 0xBE) {
        uint32_t k = U8(st, b + BLD_STORAGE_ID);
        if (k != 0 && U8(st, G_STORAGE_SETTINGS_U08 + 0xC4u * k) == 0) {
            U8(st, G_STORAGE_SETTINGS_U08 + 0xC4u * k) = 1;
            if (S32(st, G_STORAGE_HIGHEST) <= (int32_t) k) {
                S32(st, G_STORAGE_HIGHEST) = (int32_t) k;
            }
        } else {
            U8(st, b + BLD_STORAGE_ID) = (uint8_t) GAME_CALL(st, FN_STORAGE_CREATE, id);
        }
    }
    if (S16(st, b + BLD_TYPE) == 0x88) {
        uint32_t k = U8(st, b + BLD_UCA);
        if (k != 0 && U32(st, G_FERRY_QUEUES + 0xE0u * k) == 0) {
            U32(st, G_FERRY_QUEUES + 0xE0u * k) = 1;
        } else {
            U8(st, b + BLD_UCA) = (uint8_t) GAME_CALL(st, FN_REC88_CREATE, id);
        }
    }
    if (S16(st, b + BLD_TYPE) == 0x4A) {
        uint32_t k = U8(st, b + BLD_UCB);
        if (k != 0 && U8(st, G_REC4A + 0x1Cu * k) == 0) {
            U8(st, G_REC4A + 0x1Cu * k) = 1;
        } else {
            GAME_CALL(st, FN_REC4A_CREATE, id);
        }
    }
    /* @bin 0x00485AB7 the city's records of unique buildings (jump table 0x00485CE0 / 0x00485D0C) */
    t = S16(st, b + BLD_TYPE);
    switch (t) {
    case 0x32:
        S32(st, city + CITY_TYPE32_OFFSET) = S32(st, b + BLD_GRID_OFFSET);
        S16(st, city + CITY_TYPE32_X) = S16(st, b + BLD_X);
        S16(st, city + CITY_TYPE32_Y) = S16(st, b + BLD_Y);
        S32(st, city + CITY_TYPE32_BUILT) = 1;
        break;
    case 0x4D: case 0x4E: case 0x4F:
        S32(st, city + CITY_MANSION_BUILT) = 1;
        break;
    case 0x5E:
        S32(st, city + CITY_ACADEMY_EXISTS) = 1;
        break;
    case 0x5F:
        S32(st, city + CITY_RECRUITER_OFFSET) = S32(st, b + BLD_GRID_OFFSET);
        S16(st, city + CITY_RECRUITER_X) = S16(st, b + BLD_X);
        S16(st, city + CITY_RECRUITER_Y) = S16(st, b + BLD_Y);
        S32(st, city + CITY_U44B0) = 1;
        break;
    case 0x88: {
        int32_t other = S32(st, b + BLD_FERRY_PARTNER);
        if (other > 0) {
            S32(st, BUILDING_ADDR(other) + BLD_FERRY_PARTNER) = id;
        }
        break;
    }
    case 0xB6:
        S32(st, G_WARSHIP_WHARVES) = S32(st, G_WARSHIP_WHARVES) + 1;
        break;
    case 0xB7: case 0xD5: case 0xDE:
        GAME_CALL(st, FN_MONUMENT_SLOT_4F4F80, id);
        break;
    case 0xBB: case 0xBC: case 0xBD:
        S32(st, city + CITY_PALACE_OFFSET) = S32(st, b + BLD_GRID_OFFSET);
        S16(st, city + CITY_PALACE_X) = S16(st, b + BLD_X);
        S16(st, city + CITY_PALACE_Y) = S16(st, b + BLD_Y);
        S16(st, city + CITY_U4486) = 1;
        break;
    case 0xD1:
        S32(st, city + CITY_FESTIVAL_SQUARE_X) = S16(st, b + BLD_X);
        S32(st, city + CITY_FESTIVAL_SQUARE_Y) = S16(st, b + BLD_Y);
        S16(st, city + CITY_U4814) = 1;
        break;
    case 0xD2:
        S16(st, city + CITY_U433A) = S16(st, city + CITY_U49D4);
        GAME_CALL(st, FN_MONUMENT_SLOT_4F4F80, id);
        break;
    case 0xCF: case 0xD0:
        GAME_CALL(st, FN_MONUMENT_SLOT_4F4F80, id);
        break;
    default:
        if (t >= 0x41 && t <= 0x45) {
            S16(st, city + CITY_U4478) = 1;
            S16(st, city + CITY_U448A) = 2;
            S32(st, city + CITY_U472C) = id;
        }
        break;
    }
    U8(st, b + BLD_STATE) = 1;
}

int32_t goods_to_storage(game_state *st, int32_t res, int32_t amount, int32_t dry)
{
    /* @bin 0x0045AFE0 goods given to the working storage yards (0x48) and granaries (0x47, 0xBF, goods
     * 1..8) up to their space and order limits, in building order; dry (low byte) != 0: only count.
     * Returns the amount placed. */
    int32_t total = 0;
    for (int32_t id = 1; id <= S32(st, G_BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) != 1 || U8(st, b + BLD_U44) == 0 || S16(st, b + BLD_DISTANCE) <= 0 || S16(st, b + BLD_WORKERS) <= 0) {
            continue;
        }
        uint16_t t = U16(st, b + BLD_TYPE);
        uint32_t rec = 0xC4u * U8(st, b + BLD_STORAGE_ID);
        uint8_t order;
        int32_t limit, n;
        if (t == 0x48) {
            /* @bin 0x0045B054 the order of the good in the yard's storage record: 0 accept, 2 get */
            order = U8(st, G_STORAGE_ORDERS + (uint32_t) res + rec);
            if (order == 0) {
                limit = S16(st, G_STORAGE_FILL + 2u * (uint32_t) res + rec);
            } else if (order == 2) {
                limit = S16(st, G_STORAGE_GET + 2u * (uint32_t) res + rec);
            } else {
                continue;
            }
            /* @bin 0x0045B0A2 free space over the parts (+0x3A chain): 400 per empty part, 400 - amount
             * (+0x3C) per part holding the good (+0x12) */
            n = 0;
            for (int32_t p = S16(st, b + BLD_NEXT_PART); p != 0; p = S16(st, BUILDING_ADDR(p) + BLD_NEXT_PART)) {
                int16_t good = S16(st, BUILDING_ADDR(p) + BLD_SUBTYPE);
                if (good == 0) {
                    n += 400;
                } else if (good == res) {
                    n += 400 - S16(st, BUILDING_ADDR(p) + BLD_STORED);
                }
            }
            if (n >= limit) {
                n = limit;
            }
            if (n >= amount) {
                n = amount;
            }
            total += n;
            if ((uint8_t) dry == 0) {
                GAME_CALL(st, FN_STORAGE_ADD, S32(st, CURRENT_PLAYER_ADDR), id, res, n);
            }
        } else if (t == 0x47 || t == 0xBF) {
            if (res <= 0 || res > 8) {
                continue;
            }
            order = U8(st, G_STORAGE_ORDERS + (uint32_t) res + rec);
            if (order == 0) {
                limit = S16(st, G_STORAGE_FILL + 2u * (uint32_t) res + rec);
            } else if (order == 2) {
                limit = S16(st, G_STORAGE_GET + 2u * (uint32_t) res + rec);
            } else {
                continue;
            }
            /* @bin 0x0045B1A1 granaries: the limit less the stock (+0x56 + 2 res), at most +0x56 */
            n = limit - S16(st, b + BLD_INVENTORY + 2u * (uint32_t) res);
            if (n >= S16(st, b + BLD_INVENTORY)) {
                n = S16(st, b + BLD_INVENTORY);
            }
            if (n <= 0) {
                continue;
            }
            if (n >= amount) {
                n = amount;
            }
            total += n;
            if ((uint8_t) dry == 0) {
                GAME_CALL(st, FN_BUILDING_47_ADD, id, res, 0, n);
            }
        } else {
            continue;
        }
        amount -= n;
        if (amount <= 0) {
            break;
        }
    }
    return total;
}

static void restore_grid(game_state *st, uint32_t backup, uint32_t grid, int32_t size)
{
    GAME_CALL(st, FN_COPY_BYTES, (int32_t) backup, (int32_t) grid, size);
}

/* @bin 0x00485621 a fort (0x39) put back: its formation's slot kind (+0x12 0x0B..0x0D) gets the
 * saved values 0x006FDCBC.. back if the formation still matches */
static void undo_fort(game_state *st, int32_t id)
{
    uint32_t b = BUILDING_ADDR(id);
    int32_t f = 1;
    for (uint32_t a = G_FORMATION_BASE + 0x90u + 0x42u; a < G_FORMATION_BASE + 0x90u * 50u + 0x42u; a += 0x90, f++) {
        if (U8(st, a - 0x42) != 0 && U8(st, a) == 1 && S16(st, a - 0x3C) == id) {
            break;
        }
    }
    if (f > 49) {
        return;
    }
    int16_t k = S16(st, b + BLD_SUBTYPE);
    uint32_t src, dst;
    if (k == 0xD) {
        src = G_CARRY_GROUPS + 24u;
        dst = G_CARRY_COPY_GROUPS + 24u;
    } else if (k == 0xC) {
        src = G_CARRY_GROUPS;
        dst = G_CARRY_COPY_GROUPS;
    } else if (k == 0xB) {
        src = G_CARRY_GROUPS + 12u;
        dst = G_CARRY_COPY_GROUPS + 12u;
    } else {
        return;
    }
    uint32_t v = U32(st, src);
    if (U8(st, G_FORMATION_BASE + 2u + 0x90u * (uint32_t) f) != (uint8_t) v) {
        return;
    }
    uint32_t v1 = U32(st, src + 4);
    U32(st, dst) = v;
    uint32_t v2 = U32(st, src + 8);
    U32(st, dst + 4) = v1;
    U32(st, dst + 8) = v2;
}

void undo_restore(game_state *st)
{
    /* @bin 0x00485230 */
    if (S32(st, G_UNDO_READY) != 1 || S32(st, G_UNDO_POSSIBLE) != 1) {
        return;
    }
    uint32_t city = CURRENT_CITY(st);
    int32_t cost = S32(st, G_UNDO_COST);
    /* @bin 0x00485265 the cost back into the treasury (+0xC), out of the year's construction (+0x2C8C) */
    S32(st, city + CITY_TREASURY) = S32(st, city + CITY_TREASURY) + cost;
    int32_t spent = S32(st, city + CITY_CONSTRUCTION_THIS_YEAR) - cost;
    S32(st, G_UNDO_POSSIBLE) = 0;
    S32(st, city + CITY_CONSTRUCTION_THIS_YEAR) = spent;
    for (int32_t r = 0; r < 0x24; r++) {
        int32_t amount = S32(st, G_UNDO_GOODS + 4u * (uint32_t) r);
        if (amount != 0) {
            goods_to_storage(st, r, amount, 0);
        }
    }
    int32_t type = S32(st, G_UNDO_TYPE);
    if (type == 9) {
        /* @bin 0x004852C2 clearing undone: the records put back and re-added to the map, all grids back */
        int32_t monument = 0;
        for (int32_t i = 0; i < S32(st, G_UNDO_COUNT); i++) {
            int16_t id = S16(st, G_UNDO_IDS + 2u * (uint32_t) i);
            if (id == 0) {
                continue;
            }
            GAME_CALL(st, FN_COPY_BYTES, (int32_t) (G_UNDO_RECORDS + 0x108u * (uint32_t) i), (int32_t) BUILDING_ADDR(id),
                      0x108);
            undo_building_readd(st, id);
            if (S16(st, BUILDING_ADDR(id) + BLD_TYPE) == 0xDE) {
                monument = id;
            }
        }
        restore_grid(st, G_TERRAIN_BACKUP, G_GRID_TERRAIN, 0x32C40);
        restore_grid(st, G_CANAL_BACKUP, G_CANAL, 0xCB10);
        restore_grid(st, G_SPRITE_BACKUP, G_SPRITE, 0xCB10);
        restore_grid(st, G_IMAGE_BACKUP, G_IMAGE, 0x32C40);
        restore_grid(st, G_BITS_BACKUP, G_DAY2_DESTROY_BITS, 0xCB10);
        restore_grid(st, G_EDGE_BACKUP, G_EDGE, 0xCB10);
        GAME_CALL(st, FN_PLACE_AND_BYTES, (int32_t) G_DAY2_DESTROY_BITS, 0xCB10, 0xAF);
        undo_houses_refresh(st);
        if (monument != 0) {
            undo_monument_de_redraw(st, monument);
        }
    } else if (type == 8 || type == 5 || type == 6 || type == 0xA8 || type == 0xA9) {
        /* @bin 0x004857B8 roads, walls, canals: terrain and canal grids */
        restore_grid(st, G_TERRAIN_BACKUP, G_GRID_TERRAIN, 0x32C40);
        restore_grid(st, G_CANAL_BACKUP, G_CANAL, 0xCB10);
        undo_restore_images(st, 0);
    } else if (type == 0x52 || type == 0x53) {
        /* @bin 0x00485793 bridges: terrain and sprite grids */
        restore_grid(st, G_TERRAIN_BACKUP, G_GRID_TERRAIN, 0x32C40);
        restore_grid(st, G_SPRITE_BACKUP, G_SPRITE, 0xCB10);
        undo_restore_images(st, 0);
    } else if (type == 0x26 || type == 0x27) {
        /* @bin 0x00485738 plaza, gardens: terrain, canal, bitfields, edge */
        restore_grid(st, G_TERRAIN_BACKUP, G_GRID_TERRAIN, 0x32C40);
        restore_grid(st, G_CANAL_BACKUP, G_CANAL, 0xCB10);
        restore_grid(st, G_BITS_BACKUP, G_DAY2_DESTROY_BITS, 0xCB10);
        restore_grid(st, G_EDGE_BACKUP, G_EDGE, 0xCB10);
        undo_restore_images(st, 1);
    } else if (S32(st, G_UNDO_COUNT) != 0) {
        if (type == 7) {
            restore_grid(st, G_TERRAIN_BACKUP, G_GRID_TERRAIN, 0x32C40);
            restore_grid(st, G_CANAL_BACKUP, G_CANAL, 0xCB10);
            undo_restore_images(st, 0);
        }
        /* @bin 0x00485491 the buildings placed: their links released, state 2 (removed by the tick) */
        for (int32_t i = 0; i < S32(st, G_UNDO_COUNT); i++) {
            int16_t id = S16(st, G_UNDO_IDS + 2u * (uint32_t) i);
            if (id == 0) {
                continue;
            }
            uint32_t b = BUILDING_ADDR(id);
            if (S16(st, b + BLD_TYPE) == 0xB7 && S16(st, b + BLD_PART) == 0) {
                clear_pyramid_area(st, id);
            }
            if (S16(st, b + BLD_TYPE) == 0x88) {
                int32_t other = S32(st, b + BLD_FERRY_PARTNER);
                if (other > 0) {
                    S32(st, BUILDING_ADDR(other) + BLD_FERRY_PARTNER) = -1;
                }
                GAME_CALL(st, FN_BUILDING_RELEASE_FIGURES_PAIR, id);
            }
            if (S16(st, b + BLD_TYPE) == 0x1F) {
                GAME_CALL(st, FN_BUILDING_CLEAR_AREA, id, 0x1F);
            }
            if (S16(st, b + BLD_TYPE) == 0x1E) {
                GAME_CALL(st, FN_BUILDING_CLEAR_AREA, id, 0x1E);
            }
            if (S16(st, b + BLD_TYPE) == 0x21) {
                GAME_CALL(st, FN_BUILDING_CLEAR_AREA, id, 0x21);
            }
            if (S16(st, b + BLD_TYPE) == 0xAB) {
                GAME_CALL(st, FN_BUILDING_CLEAR_TILES_4BFEE0, id);
            }
            if (S16(st, b + BLD_TYPE) == 0xCA) {
                GAME_CALL(st, FN_BUILDING_CLEAR_TILES_4BFEE0, id);
            }
            /* @bin 0x00485571 Cleopatra monuments: their objects' undo (thiscall) */
            if (S16(st, b + BLD_TYPE) == 0xE1) {
                GAME_CALLT(st, G_MONUMENT_E1, FN_MONUMENT_E1_CLEAR, id);
            }
            if (S16(st, b + BLD_TYPE) == 0xE6) {
                GAME_CALLT(st, G_MONUMENT_E6, FN_MONUMENT_E6_CLEAR, id);
            }
            if (S16(st, b + BLD_TYPE) == 0xE3) {
                GAME_CALLT(st, G_MONUMENT_E3, FN_MONUMENT_E3_CLEAR, id);
            }
            if (S16(st, b + BLD_TYPE) == 0xE4) {
                GAME_CALLT(st, G_MONUMENT_E4, FN_MONUMENT_E4_CLEAR, id);
            }
            if (S16(st, b + BLD_TYPE) == 0xE5) {
                GAME_CALLT(st, G_MONUMENT_E5, FN_TOMB_E5_CLEAR, id);
            }
            if (S16(st, b + BLD_TYPE) == 0xEA) {
                GAME_CALLT(st, G_MONUMENT_EA, FN_TOMB_EA_CLEAR, id);
            }
            if (S16(st, b + BLD_TYPE) == 0xEB) {
                GAME_CALLT(st, G_MONUMENT_EB, FN_TOMB_EB_CLEAR, id);
            }
            if (S16(st, b + BLD_TYPE) == 0xEC) {
                GAME_CALLT(st, G_MONUMENT_EC, FN_TOMB_EC_CLEAR, id);
            }
            if (S16(st, b + BLD_TYPE) == 0x39) {
                undo_fort(st, id);
            }
            U8(st, b + BLD_STATE) = 2;
        }
    }
    GAME_CALL0(st, FN_ROUTING_REBUILD_EE0C50);
    GAME_CALL0(st, FN_ROUTING_REBUILD_EC7630);
    GAME_CALL0(st, FN_ROUTING_REBUILD_EBAB20);
    S32(st, G_UNDO_COUNT) = 0;
    GAME_CALL0(st, FN_LOAD_REBUILD_STORAGE_LINKS);
    GAME_CALL0(st, FN_LOAD_REBUILD_0X88_LINKS);
    GAME_CALL0(st, FN_LOAD_REBUILD_0X4A_LINKS);
    GAME_CALL0(st, FN_PLACE_MONUMENT_SLOTS);
    GAME_CALL0(st, FN_TERRAIN_REBUILD);
}

static int32_t run_forget(game_state *st, const int32_t *a)
{
    (void) a;
    undo_forget(st);
    return 0;
}

static int32_t run_images(game_state *st, const int32_t *a)
{
    undo_restore_images(st, a[0]);
    return 0;
}

static int32_t run_houses(game_state *st, const int32_t *a)
{
    (void) a;
    undo_houses_refresh(st);
    return 0;
}

static int32_t run_de(game_state *st, const int32_t *a)
{
    undo_monument_de_redraw(st, a[0]);
    return 0;
}

static int32_t run_readd(game_state *st, const int32_t *a)
{
    undo_building_readd(st, a[0]);
    return 0;
}

static int32_t run_goods(game_state *st, const int32_t *a)
{
    return goods_to_storage(st, a[0], a[1], a[2]);
}

static int32_t run_restore(game_state *st, const int32_t *a)
{
    (void) a;
    undo_restore(st);
    return 0;
}

const ported_function undo_ports[] = {
    {0x00485080u, "undo_forget", 0, 0, 0, run_forget, 0},
    {0x00485830u, "undo_restore_images", 1, 0, 0, run_images, 0},
    {0x00485890u, "undo_houses_refresh", 0, 0, 0, run_houses, 0},
    {0x004E0C80u, "undo_monument_de_redraw", 1, 0, 0, run_de, 0},
    {0x004858E0u, "undo_building_readd", 1, 0, 0, run_readd, 0},
    {0x0045AFE0u, "goods_to_storage", 3, 32, 0, run_goods, 0},
    {0x00485230u, "undo_restore", 0, 0, 0, run_restore, 0},
};
const int undo_ports_count = sizeof(undo_ports) / sizeof(undo_ports[0]);
