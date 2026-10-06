/* Imhotep - undo of the last construction or clearing (player command, docs/commands.md 2.6):
 * 0x00485230 and its callees. The snapshot 0x00484F60 belongs to the construction start.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "command/undo.h"

#include "command/clear.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define TERRAIN 0x00AF598Cu
#define CANAL 0x00AE8E7Cu
#define SPRITE 0x00ADC36Cu
#define IMAGE 0x00B7482Cu
#define BITS 0x00B285CCu
#define EDGE 0x00B67D1Cu
#define BUILDING_GRID 0x00B4E6FCu
/* @bin 0x00484F60 the grids' backups taken at the construction start */
#define TERRAIN_BACKUP 0x009DE62Cu
#define CANAL_BACKUP 0x009D1B1Cu
#define SPRITE_BACKUP 0x009C500Cu
#define IMAGE_BACKUP 0x00A1126Cu
#define BITS_BACKUP 0x009B84FCu
#define EDGE_BACKUP 0x009AB9ECu
#define MAP_H 0x00BCA2D4u
#define ROW_OFFSET 0x00BC9804u /* per row: grid offset of the first tile, first and end x */
#define ROW_X_END 0x00BC9B94u
#define ROW_X_START 0x00BC9F24u
#define UNDO_COUNT 0x00BC95F0u   /* buildings kept for undo: ids (i16) and their records */
#define UNDO_IDS 0x00BA746Cu
#define UNDO_RECORDS 0x00BA7888u
#define UNDO_GOODS 0x00BC95F4u   /* 0x24 dwords: goods consumed, by resource */
#define UNDO_COST 0x00BC9684u
#define UNDO_TYPE 0x00BC9690u
#define UNDO_OK 0x00BC9694u
#define UNDO_READY 0x00BC968Cu

void undo_forget(game_state *st)
{
    /* @bin 0x00485080 */
    int32_t n = S32(st, UNDO_COUNT);
    for (int32_t i = 0; i < n; i++) {
        int16_t id = S16(st, UNDO_IDS + 2u * (uint32_t) i);
        if (id == 0) {
            continue;
        }
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b) == 6) {
            U8(st, b) = 1;
        }
        U8(st, b + 0xBF) = 0;
    }
    S32(st, UNDO_COUNT) = 0;
    GAME_CALL(st, 0x0040F8F0u, (int32_t) UNDO_RECORDS, 0x21D68, 0);
    GAME_CALL(st, 0x0040F8F0u, (int32_t) UNDO_IDS, 0x41A, 0);
}

void undo_restore_images(game_state *st, int32_t unused)
{
    /* @bin 0x00485830 */
    (void) unused;
    int32_t h = S32(st, MAP_H);
    for (int32_t y = 0; y < h; y++) {
        int32_t xs = S32(st, ROW_X_START + 4u * (uint32_t) y);
        int32_t xe = S32(st, ROW_X_END + 4u * (uint32_t) y);
        uint32_t off = U32(st, ROW_OFFSET + 4u * (uint32_t) y);
        for (int32_t n = xs; n < xe; n++, off++) {
            if (S16(st, BUILDING_GRID + 2u * off) == 0) {
                U32(st, IMAGE + 4u * off) = U32(st, IMAGE_BACKUP + 4u * off);
            }
        }
    }
}

void undo_houses_refresh(game_state *st)
{
    /* @bin 0x00485890 */
    for (int32_t id = 1; id < BUILDING_COUNT; id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b) == 0) {
            continue;
        }
        int16_t t = S16(st, b + 0x10);
        if (t >= 0x0A && t <= 0x1D && S16(st, b + 0x1C) == 0) {
            GAME_CALL(st, 0x0046CF40u, id);
        }
    }
}

void undo_monument_de_redraw(game_state *st, int32_t id)
{
    /* @bin 0x004E0C80 */
    uint32_t b = BUILDING_ADDR(id);
    if (S16(st, b + 0x10) != 0xDE) {
        return;
    }
    int32_t base = id - S16(st, b + 0x12);
    for (int32_t i = 0; i < 0x74; i++) {
        GAME_CALL(st, 0x004DFBB0u, base + i);
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
    int32_t size = S32(st, 0x005D06DCu + 0x14u * (uint32_t) (int32_t) S16(st, b + 0x10));
    if ((uint8_t) GAME_CALLT(st, 0x00BCA350u, 0x00488030u, id) != 0) {
        GAME_CALLT(st, 0x00BCA350u, 0x00488E60u, id, 0);
    } else {
        int16_t t = S16(st, b + 0x10);
        if ((t >= 0x41 && t <= 0x45) || t == 0xCA) {
            GAME_CALL(st, 0x00470990u, S32(st, CURRENT_PLAYER_ADDR), t, id, S16(st, b + 6), S16(st, b + 8), size, 0,
                      0);
        } else {
            GAME_CALL(st, 0x0047BF50u, id, S16(st, b + 6), S16(st, b + 8), size, size, 0, 0);
        }
    }
    int16_t t = S16(st, b + 0x10);
    if (t == 0x4C || t == 0xB5 || t == 0xB6) {
        S16(st, b + 0xB4) = 0;
    }
    if (U8(st, b + 5) != 0) {
        S16(st, b + 0x1C) = 0;
    }
    /* @bin 0x004859B8 the links freed by the clearing (0x00474700) taken again, or new ones made */
    t = S16(st, b + 0x10);
    if (t == 0x47 || t == 0x48 || t == 0x46 || t == 0xBF || t == 0xC1 || t == 0xBE) {
        uint32_t k = U8(st, b + 0xC1);
        if (k != 0 && U8(st, 0x007900C0u + 0xC4u * k) == 0) {
            U8(st, 0x007900C0u + 0xC4u * k) = 1;
            if (S32(st, 0x0078D788u) <= (int32_t) k) {
                S32(st, 0x0078D788u) = (int32_t) k;
            }
        } else {
            U8(st, b + 0xC1) = (uint8_t) GAME_CALL(st, 0x0046B100u, id);
        }
    }
    if (S16(st, b + 0x10) == 0x88) {
        uint32_t k = U8(st, b + 0xCA);
        if (k != 0 && U32(st, 0x00EAAF68u + 0xE0u * k) == 0) {
            U32(st, 0x00EAAF68u + 0xE0u * k) = 1;
        } else {
            U8(st, b + 0xCA) = (uint8_t) GAME_CALL(st, 0x0046EFD0u, id);
        }
    }
    if (S16(st, b + 0x10) == 0x4A) {
        uint32_t k = U8(st, b + 0xCB);
        if (k != 0 && U8(st, 0x009783A4u + 0x1Cu * k) == 0) {
            U8(st, 0x009783A4u + 0x1Cu * k) = 1;
        } else {
            GAME_CALL(st, 0x0046A280u, id);
        }
    }
    /* @bin 0x00485AB7 the city's records of unique buildings (jump table 0x00485CE0 / 0x00485D0C) */
    t = S16(st, b + 0x10);
    switch (t) {
    case 0x32:
        S32(st, city + 0x45F8) = S32(st, b + 0x0C);
        S16(st, city + 0x45F4) = S16(st, b + 6);
        S16(st, city + 0x45F6) = S16(st, b + 8);
        S32(st, city + 0x4600) = 1;
        break;
    case 0x4D: case 0x4E: case 0x4F:
        S32(st, city + 0x499C) = 1;
        break;
    case 0x5E:
        S32(st, city + 0x49D0) = 1;
        break;
    case 0x5F:
        S32(st, city + 0x44A8) = S32(st, b + 0x0C);
        S16(st, city + 0x44A4) = S16(st, b + 6);
        S16(st, city + 0x44A6) = S16(st, b + 8);
        S32(st, city + 0x44B0) = 1;
        break;
    case 0x88: {
        int32_t other = S32(st, b + 0xC4);
        if (other > 0) {
            S32(st, BUILDING_ADDR(other) + 0xC4) = id;
        }
        break;
    }
    case 0xB6:
        S32(st, 0x00BCB9B0u) = S32(st, 0x00BCB9B0u) + 1;
        break;
    case 0xB7: case 0xD5: case 0xDE:
        GAME_CALL(st, 0x004F4F80u, id);
        break;
    case 0xBB: case 0xBC: case 0xBD:
        S32(st, city + 0x278C) = S32(st, b + 0x0C);
        S16(st, city + 0x2788) = S16(st, b + 6);
        S16(st, city + 0x278A) = S16(st, b + 8);
        S16(st, city + 0x4486) = 1;
        break;
    case 0xD1:
        S32(st, city + 0x47A0) = S16(st, b + 6);
        S32(st, city + 0x47A4) = S16(st, b + 8);
        S16(st, city + 0x4814) = 1;
        break;
    case 0xD2:
        S16(st, city + 0x433A) = S16(st, city + 0x49D4);
        GAME_CALL(st, 0x004F4F80u, id);
        break;
    case 0xCF: case 0xD0:
        GAME_CALL(st, 0x004F4F80u, id);
        break;
    default:
        if (t >= 0x41 && t <= 0x45) {
            S16(st, city + 0x4478) = 1;
            S16(st, city + 0x448A) = 2;
            S32(st, city + 0x472C) = id;
        }
        break;
    }
    U8(st, b) = 1;
}

int32_t goods_to_storage(game_state *st, int32_t res, int32_t amount, int32_t dry)
{
    /* @bin 0x0045AFE0 goods given to the working storage yards (0x48) and granaries (0x47, 0xBF, goods
     * 1..8) up to their space and order limits, in building order; dry (low byte) != 0: only count.
     * Returns the amount placed. */
    int32_t total = 0;
    for (int32_t id = 1; id <= S32(st, 0x0078D790u); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b) != 1 || U8(st, b + 0x44) == 0 || S16(st, b + 0x20) <= 0 || S16(st, b + 0x40) <= 0) {
            continue;
        }
        uint16_t t = U16(st, b + 0x10);
        uint32_t rec = 0xC4u * U8(st, b + 0xC1);
        uint8_t order;
        int32_t limit, n;
        if (t == 0x48) {
            /* @bin 0x0045B054 the order of the good in the yard's storage record: 0 accept, 2 get */
            order = U8(st, 0x007900C2u + (uint32_t) res + rec);
            if (order == 0) {
                limit = S16(st, 0x007900ECu + 2u * (uint32_t) res + rec);
            } else if (order == 2) {
                limit = S16(st, 0x00790134u + 2u * (uint32_t) res + rec);
            } else {
                continue;
            }
            /* @bin 0x0045B0A2 free space over the parts (+0x3A chain): 400 per empty part, 400 - amount
             * (+0x3C) per part holding the good (+0x12) */
            n = 0;
            for (int32_t p = S16(st, b + 0x3A); p != 0; p = S16(st, BUILDING_ADDR(p) + 0x3A)) {
                int16_t good = S16(st, BUILDING_ADDR(p) + 0x12);
                if (good == 0) {
                    n += 400;
                } else if (good == res) {
                    n += 400 - S16(st, BUILDING_ADDR(p) + 0x3C);
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
                GAME_CALL(st, 0x0045B2A0u, S32(st, CURRENT_PLAYER_ADDR), id, res, n);
            }
        } else if (t == 0x47 || t == 0xBF) {
            if (res <= 0 || res > 8) {
                continue;
            }
            order = U8(st, 0x007900C2u + (uint32_t) res + rec);
            if (order == 0) {
                limit = S16(st, 0x007900ECu + 2u * (uint32_t) res + rec);
            } else if (order == 2) {
                limit = S16(st, 0x00790134u + 2u * (uint32_t) res + rec);
            } else {
                continue;
            }
            /* @bin 0x0045B1A1 granaries: the limit less the stock (+0x56 + 2 res), at most +0x56 */
            n = limit - S16(st, b + 0x56 + 2u * (uint32_t) res);
            if (n >= S16(st, b + 0x56)) {
                n = S16(st, b + 0x56);
            }
            if (n <= 0) {
                continue;
            }
            if (n >= amount) {
                n = amount;
            }
            total += n;
            if ((uint8_t) dry == 0) {
                GAME_CALL(st, 0x0045E220u, id, res, 0, n);
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
    GAME_CALL(st, 0x0040F860u, (int32_t) backup, (int32_t) grid, size);
}

/* @bin 0x00485621 a fort (0x39) put back: its formation's slot kind (+0x12 0x0B..0x0D) gets the
 * saved values 0x006FDCBC.. back if the formation still matches */
static void undo_fort(game_state *st, int32_t id)
{
    uint32_t b = BUILDING_ADDR(id);
    int32_t f = 1;
    for (uint32_t a = 0x00BCBA9Au; a < 0x00BCD62Au; a += 0x90, f++) {
        if (U8(st, a - 0x42) != 0 && U8(st, a) == 1 && S16(st, a - 0x3C) == id) {
            break;
        }
    }
    if (f > 49) {
        return;
    }
    int16_t k = S16(st, b + 0x12);
    uint32_t src, dst;
    if (k == 0xD) {
        src = 0x006FDCD4u;
        dst = 0x006FDCFCu;
    } else if (k == 0xC) {
        src = 0x006FDCBCu;
        dst = 0x006FDCE4u;
    } else if (k == 0xB) {
        src = 0x006FDCC8u;
        dst = 0x006FDCF0u;
    } else {
        return;
    }
    uint32_t v = U32(st, src);
    if (U8(st, 0x00BCB9CAu + 0x90u * (uint32_t) f) != (uint8_t) v) {
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
    if (S32(st, UNDO_READY) != 1 || S32(st, UNDO_OK) != 1) {
        return;
    }
    uint32_t city = CURRENT_CITY(st);
    int32_t cost = S32(st, UNDO_COST);
    /* @bin 0x00485265 the cost back into the treasury (+0xC), out of the year's construction (+0x2C8C) */
    S32(st, city + 0xC) = S32(st, city + 0xC) + cost;
    int32_t spent = S32(st, city + 0x2C8C) - cost;
    S32(st, UNDO_OK) = 0;
    S32(st, city + 0x2C8C) = spent;
    for (int32_t r = 0; r < 0x24; r++) {
        int32_t amount = S32(st, UNDO_GOODS + 4u * (uint32_t) r);
        if (amount != 0) {
            goods_to_storage(st, r, amount, 0);
        }
    }
    int32_t type = S32(st, UNDO_TYPE);
    if (type == 9) {
        /* @bin 0x004852C2 clearing undone: the records put back and re-added to the map, all grids back */
        int32_t monument = 0;
        for (int32_t i = 0; i < S32(st, UNDO_COUNT); i++) {
            int16_t id = S16(st, UNDO_IDS + 2u * (uint32_t) i);
            if (id == 0) {
                continue;
            }
            GAME_CALL(st, 0x0040F860u, (int32_t) (UNDO_RECORDS + 0x108u * (uint32_t) i), (int32_t) BUILDING_ADDR(id),
                      0x108);
            undo_building_readd(st, id);
            if (S16(st, BUILDING_ADDR(id) + 0x10) == 0xDE) {
                monument = id;
            }
        }
        restore_grid(st, TERRAIN_BACKUP, TERRAIN, 0x32C40);
        restore_grid(st, CANAL_BACKUP, CANAL, 0xCB10);
        restore_grid(st, SPRITE_BACKUP, SPRITE, 0xCB10);
        restore_grid(st, IMAGE_BACKUP, IMAGE, 0x32C40);
        restore_grid(st, BITS_BACKUP, BITS, 0xCB10);
        restore_grid(st, EDGE_BACKUP, EDGE, 0xCB10);
        GAME_CALL(st, 0x0040F920u, (int32_t) BITS, 0xCB10, 0xAF);
        undo_houses_refresh(st);
        if (monument != 0) {
            undo_monument_de_redraw(st, monument);
        }
    } else if (type == 8 || type == 5 || type == 6 || type == 0xA8 || type == 0xA9) {
        /* @bin 0x004857B8 roads, walls, canals: terrain and canal grids */
        restore_grid(st, TERRAIN_BACKUP, TERRAIN, 0x32C40);
        restore_grid(st, CANAL_BACKUP, CANAL, 0xCB10);
        undo_restore_images(st, 0);
    } else if (type == 0x52 || type == 0x53) {
        /* @bin 0x00485793 bridges: terrain and sprite grids */
        restore_grid(st, TERRAIN_BACKUP, TERRAIN, 0x32C40);
        restore_grid(st, SPRITE_BACKUP, SPRITE, 0xCB10);
        undo_restore_images(st, 0);
    } else if (type == 0x26 || type == 0x27) {
        /* @bin 0x00485738 plaza, gardens: terrain, canal, bitfields, edge */
        restore_grid(st, TERRAIN_BACKUP, TERRAIN, 0x32C40);
        restore_grid(st, CANAL_BACKUP, CANAL, 0xCB10);
        restore_grid(st, BITS_BACKUP, BITS, 0xCB10);
        restore_grid(st, EDGE_BACKUP, EDGE, 0xCB10);
        undo_restore_images(st, 1);
    } else if (S32(st, UNDO_COUNT) != 0) {
        if (type == 7) {
            restore_grid(st, TERRAIN_BACKUP, TERRAIN, 0x32C40);
            restore_grid(st, CANAL_BACKUP, CANAL, 0xCB10);
            undo_restore_images(st, 0);
        }
        /* @bin 0x00485491 the buildings placed: their links released, state 2 (removed by the tick) */
        for (int32_t i = 0; i < S32(st, UNDO_COUNT); i++) {
            int16_t id = S16(st, UNDO_IDS + 2u * (uint32_t) i);
            if (id == 0) {
                continue;
            }
            uint32_t b = BUILDING_ADDR(id);
            if (S16(st, b + 0x10) == 0xB7 && S16(st, b + 0x12) == 0) {
                clear_pyramid_area(st, id);
            }
            if (S16(st, b + 0x10) == 0x88) {
                int32_t other = S32(st, b + 0xC4);
                if (other > 0) {
                    S32(st, BUILDING_ADDR(other) + 0xC4) = -1;
                }
                GAME_CALL(st, 0x004A36B0u, id);
            }
            if (S16(st, b + 0x10) == 0x1F) {
                GAME_CALL(st, 0x0043FD00u, id, 0x1F);
            }
            if (S16(st, b + 0x10) == 0x1E) {
                GAME_CALL(st, 0x0043FD00u, id, 0x1E);
            }
            if (S16(st, b + 0x10) == 0x21) {
                GAME_CALL(st, 0x0043FD00u, id, 0x21);
            }
            if (S16(st, b + 0x10) == 0xAB) {
                GAME_CALL(st, 0x004BFEE0u, id);
            }
            if (S16(st, b + 0x10) == 0xCA) {
                GAME_CALL(st, 0x004BFEE0u, id);
            }
            /* @bin 0x00485571 Cleopatra monuments: their objects' undo (thiscall) */
            if (S16(st, b + 0x10) == 0xE1) {
                GAME_CALLT(st, 0x006FD5D8u, 0x0040D890u, id);
            }
            if (S16(st, b + 0x10) == 0xE6) {
                GAME_CALLT(st, 0x006DE650u, 0x00401480u, id);
            }
            if (S16(st, b + 0x10) == 0xE3) {
                GAME_CALLT(st, 0x006FD368u, 0x00416FB0u, id);
            }
            if (S16(st, b + 0x10) == 0xE4) {
                GAME_CALLT(st, 0x006DE548u, 0x004E6660u, id);
            }
            if (S16(st, b + 0x10) == 0xE5) {
                GAME_CALLT(st, 0x006F9320u, 0x00545720u, id);
            }
            if (S16(st, b + 0x10) == 0xEA) {
                GAME_CALLT(st, 0x006F5648u, 0x00558BC0u, id);
            }
            if (S16(st, b + 0x10) == 0xEB) {
                GAME_CALLT(st, 0x006DE9E0u, 0x00555E60u, id);
            }
            if (S16(st, b + 0x10) == 0xEC) {
                GAME_CALLT(st, 0x006E9188u, 0x0054F450u, id);
            }
            if (S16(st, b + 0x10) == 0x39) {
                undo_fort(st, id);
            }
            U8(st, b) = 2;
        }
    }
    GAME_CALL0(st, 0x00517370u);
    GAME_CALL0(st, 0x00517660u);
    GAME_CALL0(st, 0x005178E0u);
    S32(st, UNDO_COUNT) = 0;
    GAME_CALL0(st, 0x0046B320u);
    GAME_CALL0(st, 0x0046F030u);
    GAME_CALL0(st, 0x0046A320u);
    GAME_CALL0(st, 0x004F61E0u);
    GAME_CALL0(st, 0x004DEA50u);
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
