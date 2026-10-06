/* Imhotep - the drag types of the construction place command (package place): houses, roads,
 * canals, walls, plazas and gardens, with the drag path of roads and canals.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Each work function takes (preview, x1, y1, x2, y2): the drag preview 0x0046FEF0 calls them with
 * preview 1, the place command 0x004728A0 with 0. The count of items placed is 0x00BC971C (the
 * place command multiplies the type's cost by it). Roads and canals restore the terrain and canal
 * grids from the backups of the construction start (0x00484F60) and follow the distance grid
 * 0x00EED760 filled from the start tile back from the end tile. */
#include "command/place_drag.h"

#include "command/place.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define MAP_W 0x00BCA2D0u
#define MAP_H 0x00BCA2D4u
#define MAP_START 0x00BCA2D8u
#define ROW_EXTRA 0x00BCA2DCu /* @bin 0x0047562A added to the row skip */
#define VIEW_DX 0x00BCA2E8u   /* @bin 0x004753D8 */
#define VIEW_DY 0x00BCA2ECu
#define COUNT 0x00BC971Cu
#define REFUND 0x00BC9718u
#define CUR_Y 0x00BC9760u
#define CUR_X 0x00BC9764u
#define CUR_OFF 0x00BC9768u
#define TER(o) U32(st, 0x00AF598Cu + 4u * (uint32_t) (o))
#define IMG(o) S32(st, 0x00B7482Cu + 4u * (uint32_t) (o))
#define BITS(o) U8(st, 0x00B285CCu + (uint32_t) (o))
#define EDGE(o) U8(st, 0x00B67D1Cu + (uint32_t) (o))
#define BGRID(o) S16(st, 0x00B4E6FCu + 2u * (uint32_t) (o))
#define FGRID(o) S16(st, 0x00B350DCu + 2u * (uint32_t) (o))
#define ROUTE(o) S8(st, 0x00EE0C50u + (uint32_t) (o))
#define DIST(o) S32(st, 0x00EED760u + 4u * (uint32_t) (o))
#define OBJECT(o) U32(st, 0x00D05720u + 4u * (uint32_t) (o))
#define QUEUE 0x00F203A0u
#define HEAD 0x00EAE00Cu
#define TAIL 0x00EAE008u
#define START 0x00EAE004u

static int32_t offset_of(game_state *st, int32_t x, int32_t y)
{
    return S32(st, MAP_START) + y * GRID_SIZE + x;
}

static int16_t building_type(game_state *st, int32_t o)
{
    return S16(st, BUILDING_ADDR(BGRID(o)) + 0x10);
}

/* a floodplain object on the tile whose flag +0x18 is clear */
static int object_blocks(game_state *st, int32_t o)
{
    uint32_t obj = OBJECT(o);
    return obj != 0 && P8(st, obj + 0x18) == 0;
}

static void notice(game_state *st, int32_t id)
{
    GAME_CALL(st, 0x004E2970u, id, 0, 0);
}

/* @bin 0x00474F54 / 0x004754D2 the grids restored from the start's backups (terrain, canals; with
 * all, also the bitfields and edge grids), then the images without a building */
static void restore_grids(game_state *st, int all)
{
    GAME_CALL(st, 0x0040F860u, 0x009DE62C, 0x00AF598C, 0x32C40);
    GAME_CALL(st, 0x0040F860u, 0x009D1B1C, 0x00AE8E7C, 0xCB10);
    if (all) {
        GAME_CALL(st, 0x0040F860u, 0x009B84FC, 0x00B285CC, 0xCB10);
        GAME_CALL(st, 0x0040F860u, 0x009AB9EC, 0x00B67D1C, 0xCB10);
    }
    GAME_CALL(st, 0x00485830u, 0);
}

/* ---- houses ------------------------------------------------------------------------------- */

void place_houses(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    /* @bin 0x00474130 vacant lots (type 10) on every free tile of the rectangle: not floodplain, no
     * blocking object, terrain & 0xEEFED77F == 0, no figure; the preview marks bit 0x10 of the
     * bitfields */
    int flag = 0;
    int32_t xmin = x1 > x2 ? x2 : x1, xmax = x1 > x2 ? x1 : x2;
    int32_t ymin = y1 > y2 ? y2 : y1, ymax = y1 > y2 ? y1 : y2;
    if (xmin < 0) {
        xmin = 0;
    }
    if (xmax >= S32(st, MAP_W)) {
        xmax = S32(st, MAP_W) - 1;
    }
    if (ymin < 0) {
        ymin = 0;
    }
    if (ymax >= S32(st, MAP_H)) {
        ymax = S32(st, MAP_H) - 1;
    }
    int32_t width = xmax - xmin + 1;
    S32(st, COUNT) = 0;
    GAME_CALL0(st, 0x00485080u);
    S32(st, CUR_Y) = ymin;
    int32_t off = offset_of(st, xmin, ymin);
    S32(st, CUR_OFF) = off;
    if (ymin <= ymax) {
        int32_t y;
        do {
            S32(st, CUR_X) = xmin;
            if (xmin <= xmax) {
                int32_t x;
                do {
                    uint32_t t = TER(off);
                    if (!(t & 0x10000u) && !object_blocks(st, off) && !(t & 0xEEFED77Fu) && FGRID(off) == 0) {
                        if (preview != 0) {
                            BITS(off) |= 0x10;
                            S32(st, COUNT) += 1;
                        } else {
                            int32_t b = GAME_CALL(st, 0x0046A400u, S32(st, CURRENT_PLAYER_ADDR), 0xA, S32(st, CUR_X),
                                                  S32(st, CUR_Y), 0, 0);
                            GAME_CALL(st, 0x00485100u, b);
                            if (b > 0) {
                                S32(st, COUNT) += 1;
                                int32_t g = S32(st, 0x00F5653Cu);
                                GAME_CALL(st, 0x0047BF50u, b, S32(st, CUR_X), S32(st, CUR_Y), 1, 1, g, 8);
                                if (GAME_CALL(st, 0x00482650u, S32(st, CUR_X), S32(st, CUR_Y), 1, 2, 0x40) == 0) {
                                    flag = 1;
                                }
                            }
                            off = S32(st, CUR_OFF);
                        }
                    }
                    x = S32(st, CUR_X) + 1;
                    off++;
                    S32(st, CUR_X) = x;
                    S32(st, CUR_OFF) = off;
                } while (x <= xmax);
            }
            y = S32(st, CUR_Y) + 1;
            off += GRID_SIZE - width;
            S32(st, CUR_Y) = y;
            S32(st, CUR_OFF) = off;
        } while (y <= ymax);
    }
    if (preview != 0) {
        return;
    }
    GAME_CALL(st, 0x004E3330u, 0xA);
    place_notice_4e2d80(st, 0xA);
    place_notice_4e2dd0(st, 0xA);
    place_notice_4e2fb0(st, 0xA);
    if (flag == 1) {
        notice(st, 0x3E);
    }
    GAME_CALL(st, 0x00516F90u, x1, y1, x2, y2, 1);
    GAME_CALL(st, 0x005173A0u, x1, y1, x2, y2);
    U8(st, 0x00E38DCAu) = 1;
}

void place_notice_4e2d80(game_state *st, int32_t unused)
{
    /* @bin 0x004E2D80 (0x00E38EBC or 0x00E38EA8 set) CITY+0x24 >= 300 and CITY+0x2BFC > 10 → 0x5C */
    (void) unused;
    if (S32(st, 0x00605A3Cu) == 1) {
        return;
    }
    if (S32(st, 0x00E38EBCu) == 0 && U8(st, 0x00E38EA8u) == 0) {
        return;
    }
    uint32_t c = CURRENT_CITY(st);
    if (S32(st, c + 0x24) < 300 || S32(st, c + 0x2BFC) <= 10) {
        return;
    }
    notice(st, 0x5C);
}

void place_notice_4e2dd0(game_state *st, int32_t unused)
{
    /* @bin 0x004E2DD0 CITY+0x24 >= 300 and CITY+0x296C <= CITY+0x45AC / 2 → 0x5D */
    (void) unused;
    if (S32(st, 0x00605A3Cu) == 1) {
        return;
    }
    uint32_t c = CURRENT_CITY(st);
    if (S32(st, c + 0x24) < 300) {
        return;
    }
    if (S32(st, c + 0x296C) > S32(st, c + 0x45AC) / 2) {
        return;
    }
    notice(st, 0x5D);
}

void place_notice_4e2fb0(game_state *st, int32_t quiet)
{
    /* @bin 0x004E2FB0 (quiet: no 0x7A / 0x7B / 0x82) */
    if (S32(st, 0x00605A3Cu) == 1) {
        return;
    }
    uint32_t c = CURRENT_CITY(st);
    if (S32(st, c + 0x24) < 0x14) {
        return;
    }
    if (S32(st, 0x00E38EBCu) == 0 && S8(st, 0x00E38EA8u) < 2) {
        int32_t v = S32(st, c + 0x4410);
        if (v == 0 || v == 1 || v == 3 || v == 5) {
            return;
        }
    }
    int32_t k = S32(st, c + 0x2740);
    if (k != 0) {
        /* @bin 0x004E30BC */
        if (k != 1) {
            return;
        }
        if (S32(st, c + 0x2728) == 0 && S32(st, c + 0x38) > 0) {
            int32_t id = S32(st, c + 0x4410) + 0x83;
            if (id != 0) {
                notice(st, id);
            }
            return;
        }
        if (quiet == 0) {
            notice(st, 0x82);
        }
        return;
    }
    if (S32(st, c + 0x4644) + S32(st, c + 0x4634) > 3) {
        notice(st, 0xD8);
        return;
    }
    if (S32(st, c + 0x273C) < 5) {
        if (S32(st, c + 0x2728) != 0 || S32(st, c + 0x38) <= 0) {
            if (quiet == 0) {
                notice(st, 0x7B);
            }
            return;
        }
        if (S32(st, c + 0x272C) < 0x50) {
            int32_t id = S32(st, c + 0x4410) + 0x7C;
            if (id != 0) {
                notice(st, id);
            }
            return;
        }
    }
    if (quiet == 0) {
        notice(st, 0x7A);
    }
}

/* ---- the drag path ------------------------------------------------------------------------ */

static void enqueue(game_state *st, int32_t o)
{
    int32_t t = S32(st, TAIL);
    S32(st, QUEUE + 4u * (uint32_t) t) = o;
    t++;
    S32(st, TAIL) = t;
    if (t > 0xCB0F) {
        S32(st, TAIL) = 0;
    }
}

/* @bin 0x0051BE01 one neighbour n of the tile c (route byte at r_off): the building check of each
 * direction differs (kind 0: north, needs canal == 1 first, against -6; 1: east / west, against -4;
 * 2: south, against -6) */
static void fill_visit(game_state *st, int32_t n, int32_t kind, int32_t canal, int32_t d)
{
    if (DIST(n) != 0) {
        return;
    }
    int blocked = 0;
    int8_t r = ROUTE(n);
    if ((TER(n) & 0x100u) && GAME_CALL(st, 0x0051C390u, n) != 0) {
        if (canal == 0 && GAME_CALL(st, 0x0051C390u, n) == 0) {
            DIST(n) = -1;
            blocked = 1;
        }
    } else if (r == -5) {
        if (canal != 0) {
            blocked = 1;
        } else if (GAME_CALL(st, 0x0051C510u, n) == 0) {
            DIST(n) = -1;
            blocked = 1;
        }
    } else if (r == 2 || r == -1 || (TER(n) & 0x12040011u)) {
        blocked = 1;
    } else if (TER(n) & 8u) {
        int8_t against = kind == 1 ? -4 : -6;
        if (kind == 0) {
            blocked = !(canal == 1 && (building_type(st, n) == 0x8A || r == against));
        } else {
            blocked = !(building_type(st, n) == 0x8A || (r == against && canal == 1));
        }
    }
    if ((U8(st, 0x00AF598Cu + 4u * (uint32_t) n) & 0x40) && canal != 0 && GAME_CALL(st, 0x0051C630u, n) == 0) {
        DIST(n) = -1;
        return;
    }
    if (blocked) {
        return;
    }
    if ((uint8_t) GAME_CALL(st, 0x0046F9A0u, n % GRID_SIZE, n / GRID_SIZE, 3) != 0) {
        return;
    }
    DIST(n) = d;
    if (ROUTE(n) == -2) {
        DIST(n) = d + 4;
    }
    enqueue(st, n);
}

int32_t place_path_fill(game_state *st, int32_t unused, int32_t x, int32_t y, int32_t canal)
{
    /* @bin 0x0051BC60 (_, x, y, canal) the distances from the start tile (as 0x0051B680, with the
     * map-edge test 0x0046F9A0 on every tile); 0 when the start tile is blocked */
    (void) unused;
    GAME_CALL(st, 0x0040F8F0u, 0x00EED760, 0x32C40, 0);
    int32_t o = offset_of(st, x, y);
    int8_t cl = ROUTE(o);
    S32(st, START) = o;
    uint32_t t = TER(o);
    if (cl == -1 || (t & 0x12040011u)) {
        if (canal == 0) {
            return 0;
        }
        if (!(t & 0x100u)) {
            if (!(t & 8u)) {
                return 0;
            }
            int16_t bt = building_type(st, o);
            if (bt != 0x5A && bt != 0x8A) {
                return 0;
            }
        }
    }
    if (cl == 2) {
        return 0;
    }
    if ((t & 0x100u) && GAME_CALL(st, 0x0051C390u, o) != 0 && canal == 0 &&
        GAME_CALL(st, 0x0051C390u, S32(st, START)) == 0) {
        return 0;
    }
    if (ROUTE(S32(st, START)) == -5) {
        if (canal != 0 || GAME_CALL(st, 0x0051C510u, S32(st, START)) == 0) {
            return 0;
        }
    }
    if ((t & 0x40u) && canal != 0 && GAME_CALL(st, 0x0051C630u, S32(st, START)) == 0) {
        return 0;
    }
    if ((uint8_t) GAME_CALL(st, 0x0046F9A0u, x + S32(st, VIEW_DX), y + S32(st, VIEW_DY), 3) != 0) {
        return 0;
    }
    S32(st, 0x00EADFD4u) += 1;
    GAME_CALL(st, 0x0040F8F0u, 0x00EAE010, 0xCB10, 0);
    DIST(S32(st, START)) = 1;
    S32(st, QUEUE) = S32(st, START);
    S32(st, HEAD) = 0;
    S32(st, TAIL) = 1;
    int32_t h;
    do {
        int32_t c = S32(st, QUEUE + 4u * (uint32_t) S32(st, HEAD));
        int32_t d = DIST(c) + 1;
        fill_visit(st, c - GRID_SIZE, 0, canal, d);
        fill_visit(st, c + 1, 1, canal, d);
        fill_visit(st, c + GRID_SIZE, 2, canal, d);
        fill_visit(st, c - 1, 1, canal, d);
        h = S32(st, HEAD) + 1;
        S32(st, HEAD) = h;
        if (h > 0xCB0F) {
            h = 0;
            S32(st, HEAD) = 0;
        }
    } while (h != S32(st, TAIL));
    return 1;
}

static void cell_neighbourhood(game_state *st, int32_t x, int32_t y, int32_t *x0, int32_t *y0, int32_t *w, int32_t *h)
{
    /* @bin 0x0047ADF5 the 3x3 block around (x, y) cut by the map */
    *x0 = x - 1;
    *y0 = y - 1;
    *w = 3;
    *h = 3;
    if (*x0 < 0) {
        *w = *x0 + 3;
        *x0 = 0;
    } else if (*x0 + 3 > S32(st, MAP_W)) {
        *w = S32(st, MAP_W) - *x0;
    }
    if (*y0 < 0) {
        *h = *y0 + 3;
        *y0 = 0;
    } else if (*y0 + 3 > S32(st, MAP_H)) {
        *h = S32(st, MAP_H) - *y0;
    }
}

void place_road_tile(game_state *st, int32_t x, int32_t y, int32_t set)
{
    /* @bin 0x0047B310 (x, y, set) terrain road bit 0x40 set or cleared (counted when new), then the
     * road images of the 3x3 block (0x0047DB00(0x8000040), 0x0047E5C0, 0x0047B670 on road tiles
     * without bits 0xC) and 0x004746D0 */
    if ((uint8_t) GAME_CALL(st, 0x0046F930u, x + S32(st, VIEW_DX), y + S32(st, VIEW_DY)) == 0) {
        return;
    }
    int32_t o = offset_of(st, x, y);
    uint32_t t = TER(o);
    if (!(t & 0x40u)) {
        S32(st, COUNT) += 1;
    }
    TER(o) = set == 1 ? (t | 0x40u) : (t & ~0x40u);
    BITS(o) &= 0xEF;
    int32_t x0, y0, w, h;
    cell_neighbourhood(st, x, y, &x0, &y0, &w, &h);
    int32_t yend = h + y0;
    S32(st, CUR_Y) = y0;
    int32_t off = offset_of(st, x0, y0);
    S32(st, CUR_OFF) = off;
    if (y0 < yend) {
        int32_t xend = w + x0;
        do {
            S32(st, CUR_X) = x0;
            if (x0 < xend) {
                int32_t cx;
                do {
                    uint32_t v = TER(off);
                    if ((v & 0x40u) && !(v & 0xCu)) {
                        GAME_CALL(st, 0x0047DB00u, 0x8000040);
                        GAME_CALL0(st, 0x0047E5C0u);
                        GAME_CALL0(st, 0x0047B670u);
                        off = S32(st, CUR_OFF);
                    }
                    cx = S32(st, CUR_X) + 1;
                    off++;
                    S32(st, CUR_X) = cx;
                    S32(st, CUR_OFF) = off;
                } while (cx < xend);
            }
            S32(st, CUR_Y) += 1;
            off += GRID_SIZE - w;
            S32(st, CUR_OFF) = off;
        } while (S32(st, CUR_Y) < yend);
    }
    GAME_CALL(st, 0x004746D0u, x0, y0, w);
}

void place_canal_tile(game_state *st, int32_t x, int32_t y, int32_t arg3, int32_t set)
{
    /* @bin 0x0047B960 (x, y, arg3, set) terrain canal bit 0x100 set or cleared (counted when new),
     * then the canal images of the 3x3 block (0x0047E870, 0x0047BE30(arg3) on canal tiles without
     * bits 0xC nor bitfield 0x80) and 0x004746D0 */
    if ((uint8_t) GAME_CALL(st, 0x0046F930u, x + S32(st, VIEW_DX), y + S32(st, VIEW_DY)) == 0) {
        return;
    }
    int32_t o = offset_of(st, x, y);
    uint32_t t = TER(o);
    if (!(t & 0x100u)) {
        S32(st, COUNT) += 1;
    }
    TER(o) = set == 1 ? (t | 0x100u) : (t & ~0x100u);
    BITS(o) &= 0xEF;
    int32_t x0, y0, w, h;
    cell_neighbourhood(st, x, y, &x0, &y0, &w, &h);
    int32_t yend = h + y0;
    S32(st, CUR_Y) = y0;
    int32_t off = offset_of(st, x0, y0);
    S32(st, CUR_OFF) = off;
    if (y0 < yend) {
        int32_t xend = w + x0;
        do {
            S32(st, CUR_X) = x0;
            if (x0 < xend) {
                int32_t cx;
                do {
                    uint32_t v = TER(off);
                    if ((v & 0x100u) && !(v & 0xCu) && !(BITS(off) & 0x80)) {
                        GAME_CALL0(st, 0x0047E870u);
                        GAME_CALL(st, 0x0047BE30u, arg3);
                        off = S32(st, CUR_OFF);
                    }
                    cx = S32(st, CUR_X) + 1;
                    off++;
                    S32(st, CUR_X) = cx;
                    S32(st, CUR_OFF) = off;
                } while (cx < xend);
            }
            S32(st, CUR_Y) += 1;
            off += GRID_SIZE - w;
            S32(st, CUR_OFF) = off;
        } while (S32(st, CUR_Y) < yend);
    }
    GAME_CALL(st, 0x004746D0u, x0, y0, w);
}

int32_t place_path_walk(game_state *st, int32_t unused, int32_t xs, int32_t ys, int32_t x, int32_t y, int32_t mode)
{
    /* @bin 0x0051C930 (_, start x, start y, end x, end y, mode) from the end tile down the distances
     * to the start (direction 0x0051CBA0, then the 4 directions of 0x005F815C[dir] tried in order:
     * offsets 0x005F80FC[], steps 0x005F811C[] / 0x005F8120[]), at most 400 tiles; each tile: mode 0
     * a road (0x0047B310), 2 counted only, 3 a canal (0x0047B960); 1 when the start is reached */
    (void) unused;
    S32(st, COUNT) = 0;
    int32_t steps = 1;
    int32_t o = offset_of(st, x, y);
    S32(st, 0x00EAE000u) = o;
    for (;;) {
        int32_t cur = DIST(o);
        if (cur <= 0 && building_type(st, o) != 0x8A) {
            return 0;
        }
        if (mode == 0) {
            if ((TER(o) & 0x100u) && GAME_CALL(st, 0x0051C390u, o) == 0) {
                return 0;
            }
            place_road_tile(st, x, y, 1);
        } else if (mode == 2) {
            S32(st, COUNT) += 1;
        } else if (mode == 3) {
            if ((TER(o) & 0x40u) && GAME_CALL(st, 0x0051C630u, o) == 0) {
                return 0;
            }
            place_canal_tile(st, x, y, 1, 1);
        }
        uint32_t d = (uint32_t) GAME_CALL(st, 0x0051CBA0u, x, y, xs, ys) & 0xFF;
        if (d == 8) {
            return 1;
        }
        int k;
        int32_t dir = 0;
        for (k = 0; k < 4; k++) {
            dir = S32(st, 0x005F815Cu + 16u * d + 4u * (uint32_t) k);
            o = S32(st, 0x005F80FCu + 4u * (uint32_t) dir) + S32(st, 0x00EAE000u);
            int32_t dd = DIST(o);
            if (dd > 0 && dd < cur &&
                (!((TER(o) >> 8) & 1) || GAME_CALL(st, 0x0051C390u, o) != 0)) {
                break;
            }
        }
        if (k >= 4) {
            return 0;
        }
        x += S32(st, 0x005F811Cu + 8u * (uint32_t) dir);
        y += S32(st, 0x005F8120u + 8u * (uint32_t) dir);
        S32(st, 0x00EAE000u) = o;
        steps++;
        if (steps >= 400) {
            return 0;
        }
    }
}

/* ---- roads and canals ---------------------------------------------------------------------- */

/* @bin 0x00474FAC the start / end tile: buildings only of type 0x8A, terrain & 0x26345237 clear */
static int end_tile_ok(game_state *st, int32_t o, int allow_8a)
{
    uint32_t t = TER(o);
    if (t & 8u) {
        if (!allow_8a) {
            return 0;
        }
        int16_t b = BGRID(o);
        if (b == 0 || S16(st, BUILDING_ADDR(b) + 0x10) != 0x8A) {
            return 0;
        }
    }
    return !(t & 0x26345237u);
}

void place_road(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    /* @bin 0x00474F50 */
    restore_grids(st, 0);
    S32(st, COUNT) = 0;
    if (!end_tile_ok(st, offset_of(st, x1, y1), 1) || !end_tile_ok(st, offset_of(st, x2, y2), 0)) {
        return;
    }
    int32_t r = S32(st, 0x00605A3Cu) != 0 ? GAME_CALL(st, 0x0051B680u, 0, x1, y1, 0) : place_path_fill(st, 0, x1, y1, 0);
    if (r == 0) {
        return;
    }
    if (place_path_walk(st, 0, x1, y1, x2, y2, 0) == 0 || preview != 0) {
        return;
    }
    GAME_CALL(st, 0x00516F90u, x1, y1, x2, y2, 1);
    GAME_CALL(st, 0x005173A0u, x1, y1, x2, y2);
    GAME_CALL(st, 0x004746D0u, x1, y1, x2 - x1);
    U8(st, 0x00E38DCAu) = 1;
}

void place_canal(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    /* @bin 0x00475090 */
    restore_grids(st, 0);
    S32(st, COUNT) = 0;
    if (!end_tile_ok(st, offset_of(st, x1, y1), 1) || !end_tile_ok(st, offset_of(st, x2, y2), 1)) {
        return;
    }
    int32_t r = S32(st, 0x00605A3Cu) != 0 ? GAME_CALL(st, 0x0051B680u, 0, x1, y1, 1) : place_path_fill(st, 0, x1, y1, 1);
    if (r == 0) {
        return;
    }
    if (place_path_walk(st, 0, x1, y1, x2, y2, 3) == 0 || preview != 0) {
        return;
    }
    int32_t xa = x1 > x2 ? x2 : x1, xb = x1 > x2 ? x1 : x2;
    int32_t ya = y1 > y2 ? y2 : y1, yb = y1 > y2 ? y1 : y2;
    GAME_CALL(st, 0x00475CC0u, xa - 1, ya - 1, xb + 1, yb + 1, 1);
    GAME_CALL(st, 0x00517690u, xa, ya, xb, yb);
    GAME_CALL0(st, 0x00517370u);
    GAME_CALL(st, 0x005173A0u, xa, ya, xb, yb);
    GAME_CALL0(st, 0x005178E0u);
    U8(st, 0x00E38DCAu) = 1;
}

/* ---- walls ---------------------------------------------------------------------------------- */

void place_wall_tile(game_state *st, int32_t x, int32_t y, int32_t mode, uint32_t flag)
{
    /* @bin 0x0047AD20 (x, y, mode, flag) mode 1: the wall kind bits 0xC04000 := flag (counted, and
     * a refund of half the cost of the kind replaced: 0x00705F94 for 0x4000, 0x007078E4 for 0x404000
     * replaced by 0x804000); else cleared; then the wall images of the 3x3 block */
    if ((uint8_t) GAME_CALL(st, 0x0046F930u, x + S32(st, VIEW_DX), y + S32(st, VIEW_DY)) == 0) {
        return;
    }
    int32_t o = offset_of(st, x, y);
    if (mode == 1) {
        uint32_t t = TER(o);
        uint32_t kind = t & 0xC04000u;
        if (kind != flag) {
            int32_t add;
            if (kind == 0x4000u) {
                add = S32(st, 0x00705F94u) / 2;
            } else if (kind == 0x404000u && flag == 0x804000u) {
                add = S32(st, 0x007078E4u) / 2;
            } else {
                add = 0;
            }
            S32(st, COUNT) += 1;
            S32(st, REFUND) += add;
        }
        TER(o) = (t & 0xFF3FBFFFu) | flag;
    } else {
        TER(o) &= 0xFF3FBFFFu;
    }
    BITS(o) &= 0xEF;
    int32_t x0, y0, w, h;
    cell_neighbourhood(st, x, y, &x0, &y0, &w, &h);
    int32_t yend = h + y0;
    S32(st, CUR_Y) = y0;
    int32_t off = offset_of(st, x0, y0);
    S32(st, CUR_OFF) = off;
    if (y0 >= yend) {
        return;
    }
    int32_t xend = w + x0;
    do {
        S32(st, CUR_X) = x0;
        if (x0 < xend) {
            int32_t cx;
            do {
                uint32_t v = TER(off);
                if ((v & 0x4000u) && !(v & 8u)) {
                    GAME_CALL(st, 0x0047DB00u, 0x4000);
                    GAME_CALL0(st, 0x00484F00u);
                    GAME_CALL(st, 0x00484BE0u, 0x005CB9D8, 0x30, -1, 0);
                    int32_t img = S32(st, 0x00F5686Cu) + S32(st, 0x00BC9740u) + S32(st, 0x00BC9748u);
                    off = S32(st, CUR_OFF);
                    IMG(off) = img;
                    EDGE(off) |= 0x40;
                    BITS(off) &= 0xF0;
                    if (GAME_CALL0(st, 0x0047E000u) != 0) {
                        GAME_CALL0(st, 0x0047E750u);
                        if (GAME_CALL(st, 0x00484BE0u, 0x005CBCD8, 0xF, -1, 0) != 0) {
                            img = S32(st, 0x00F5686Cu) + S32(st, 0x00BC9740u) + S32(st, 0x00BC9748u);
                            IMG(S32(st, CUR_OFF)) = img;
                        } else {
                            GAME_CALL(st, 0x0051F780u, 1);
                        }
                    } else {
                        GAME_CALL(st, 0x0051F780u, 0);
                    }
                    GAME_CALL(st, 0x004746D0u, S32(st, CUR_X), S32(st, CUR_Y), 1);
                    off = S32(st, CUR_OFF);
                }
                cx = S32(st, CUR_X) + 1;
                off++;
                S32(st, CUR_X) = cx;
                S32(st, CUR_OFF) = off;
            } while (cx < xend);
        }
        S32(st, CUR_Y) += 1;
        off += GRID_SIZE - w;
        S32(st, CUR_OFF) = off;
    } while (S32(st, CUR_Y) < yend);
}

static int32_t clamp(int32_t v, int32_t hi)
{
    /* @bin 0x00475262 into [0, hi] */
    int32_t m = v < hi ? v : hi;
    if (m < 0) {
        return 0;
    }
    return v >= hi ? hi : v;
}

static int32_t absv(int32_t v)
{
    return v < 0 ? -v : v;
}

void place_walls(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t flag)
{
    /* @bin 0x00475220 a wall two tiles thick along the longer side of the drag */
    restore_grids(st, 0);
    int32_t wm = S32(st, MAP_W) - 1, hm = S32(st, MAP_H) - 1;
    int32_t xa = clamp(x1, wm), ya = clamp(y1, hm), xb = clamp(x2, wm), yb = clamp(y2, hm);
    if (absv(xb - xa) > absv(yb - ya)) {
        if (yb > ya) {
            yb = ya + 1;
        } else if (yb < ya) {
            yb = ya - 1;
        }
    } else if (xb > xa) {
        xb = xa + 1;
    } else if (xb < xa) {
        xb = xa - 1;
    }
    if (xb < xa) {
        int32_t t = xa;
        xa = xb;
        xb = t;
    }
    if (yb < ya) {
        int32_t t = ya;
        ya = yb;
        yb = t;
    }
    S32(st, COUNT) = 0;
    int32_t skip = S32(st, MAP_W) - xb + xa - 1;
    int32_t off = offset_of(st, xa, ya);
    for (int32_t y = ya; y <= yb; y++) {
        for (int32_t x = xa; x <= xb; x++, off++) {
            if (TER(off) & 0x2635177Fu) {
                continue;
            }
            if (object_blocks(st, off)) {
                continue;
            }
            if ((uint8_t) GAME_CALL(st, 0x0046F930u, x + S32(st, VIEW_DX), y + S32(st, VIEW_DY)) == 0) {
                continue;
            }
            if (place_special_point_in(st, x, y, x, y)) {
                continue;
            }
            if (FGRID(off) != 0) {
                continue;
            }
            S32(st, CUR_OFF) = off;
            place_wall_tile(st, x, y, 1, flag);
        }
        off += S32(st, ROW_EXTRA) + skip;
    }
    if (preview != 0) {
        return;
    }
    GAME_CALL(st, 0x00516F90u, xa, ya, xb, yb, 1);
    GAME_CALL(st, 0x005173A0u, xa, ya, xb, yb);
    GAME_CALL0(st, 0x005178E0u);
    U8(st, 0x00E38DCAu) = 1;
}

/* ---- plazas and gardens --------------------------------------------------------------------- */

void place_plaza(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    /* @bin 0x004754A0 plaza on the road tiles (0x40, no 0xC, no building, 0x0047B8E0): bitfields
     * low bits cleared and 0x80 set (counted when new), image 0, edge bit 0x40; then 0x00475720 over
     * the whole map */
    (void) preview;
    int32_t xa = x1 > x2 ? x2 : x1, xb = x1 > x2 ? x1 : x2;
    int32_t ya = y1 > y2 ? y2 : y1, yb = y1 > y2 ? y1 : y2;
    restore_grids(st, 1);
    if (xa < 0) {
        xa = 0;
    }
    if (ya < 0) {
        ya = 0;
    }
    if (xb >= S32(st, MAP_W)) {
        xb = S32(st, MAP_W) - 1;
    }
    if (yb >= S32(st, MAP_H)) {
        yb = S32(st, MAP_H) - 1;
    }
    S32(st, COUNT) = 0;
    int32_t skip = S32(st, MAP_W) - xb + xa - 1;
    int32_t off = offset_of(st, xa, ya);
    for (int32_t y = ya; y <= yb; y++) {
        if (xa <= xb) {
            for (int32_t x = xa; x <= xb; x++, off++) {
                uint8_t t = (uint8_t) TER(off);
                if (!(t & 0x40) || (t & 0xC) || BGRID(off) != 0) {
                    continue;
                }
                if (GAME_CALL(st, 0x0047B8E0u, off) == 0) {
                    continue;
                }
                uint8_t b = BITS(off);
                if (!(b & 0x80)) {
                    S32(st, COUNT) += 1;
                }
                IMG(off) = 0;
                BITS(off) = (uint8_t) ((b & 0xF0) | 0x80);
                EDGE(off) |= 0x40;
            }
        }
        off += S32(st, ROW_EXTRA) + skip;
    }
    GAME_CALL(st, 0x00475720u, 0, 0, S32(st, MAP_W) - 1, S32(st, MAP_H) - 1, 1);
}

void place_gardens(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    /* @bin 0x004759E0 gardens (terrain bit 0x20, counted) on free tiles (terrain & 0xEEFFD77F clear,
     * no blocking object, no special point); then 0x00476530 */
    (void) preview;
    int32_t xa = x1 > x2 ? x2 : x1, xb = x1 > x2 ? x1 : x2;
    int32_t ya = y1 > y2 ? y2 : y1, yb = y1 > y2 ? y1 : y2;
    restore_grids(st, 1);
    if (xa < 0) {
        xa = 0;
    }
    if (ya < 0) {
        ya = 0;
    }
    if (xb >= S32(st, MAP_W)) {
        xb = S32(st, MAP_W) - 1;
    }
    if (yb >= S32(st, MAP_H)) {
        yb = S32(st, MAP_H) - 1;
    }
    S32(st, COUNT) = 0;
    int32_t skip = S32(st, MAP_W) - xb + xa - 1;
    int32_t off = offset_of(st, xa, ya);
    for (int32_t y = ya; y <= yb; y++) {
        for (int32_t x = xa; x <= xb; x++, off++) {
            if (TER(off) & 0xEEFFD77Fu) {
                continue;
            }
            if (object_blocks(st, off)) {
                continue;
            }
            if (place_special_point_in(st, x, y, x, y)) {
                continue;
            }
            S32(st, COUNT) += 1;
            TER(off) |= 0x20u;
        }
        off += S32(st, ROW_EXTRA) + skip;
    }
    GAME_CALL0(st, 0x00476530u);
}

/* ---- table ------------------------------------------------------------------------------------ */

#define RUN5(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        fn(st, a[0], a[1], a[2], a[3], a[4]); \
        return 0; \
    }
RUN5(run_houses, place_houses)
RUN5(run_road, place_road)
RUN5(run_canal, place_canal)
RUN5(run_plaza, place_plaza)
RUN5(run_gardens, place_gardens)

static int32_t run_walls(game_state *st, const int32_t *a)
{
    place_walls(st, a[0], a[1], a[2], a[3], a[4], (uint32_t) a[5]);
    return 0;
}

static int32_t run_wall_tile(game_state *st, const int32_t *a)
{
    place_wall_tile(st, a[0], a[1], a[2], (uint32_t) a[3]);
    return 0;
}

static int32_t run_road_tile(game_state *st, const int32_t *a)
{
    place_road_tile(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_canal_tile(game_state *st, const int32_t *a)
{
    place_canal_tile(st, a[0], a[1], a[2], a[3]);
    return 0;
}

static int32_t run_path_fill(game_state *st, const int32_t *a)
{
    return place_path_fill(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_path_walk(game_state *st, const int32_t *a)
{
    return place_path_walk(st, a[0], a[1], a[2], a[3], a[4], a[5]);
}

static int32_t run_4e2d80(game_state *st, const int32_t *a)
{
    place_notice_4e2d80(st, a[0]);
    return 0;
}

static int32_t run_4e2dd0(game_state *st, const int32_t *a)
{
    place_notice_4e2dd0(st, a[0]);
    return 0;
}

static int32_t run_4e2fb0(game_state *st, const int32_t *a)
{
    place_notice_4e2fb0(st, a[0]);
    return 0;
}

const ported_function place_drag_ports[] = {
    {0x00474130u, "place_houses", 5, 0, 0, run_houses, 0},
    {0x004E2D80u, "place_notice_4e2d80", 1, 0, 0, run_4e2d80, 0},
    {0x004E2DD0u, "place_notice_4e2dd0", 1, 0, 0, run_4e2dd0, 0},
    {0x004E2FB0u, "place_notice_4e2fb0", 1, 0, 0, run_4e2fb0, 0},
    {0x00474F50u, "place_road", 5, 0, 0, run_road, 0},
    {0x00475090u, "place_canal", 5, 0, 0, run_canal, 0},
    {0x00475220u, "place_walls", 6, 0, 0, run_walls, 0},
    {0x0047AD20u, "place_wall_tile", 4, 0, 0, run_wall_tile, 0},
    {0x004754A0u, "place_plaza", 5, 0, 0, run_plaza, 0},
    {0x004759E0u, "place_gardens", 5, 0, 0, run_gardens, 0},
    {0x0051BC60u, "place_path_fill", 4, 32, 0, run_path_fill, 0},
    {0x0051C930u, "place_path_walk", 6, 32, 0, run_path_walk, 0},
    {0x0047B310u, "place_road_tile", 3, 0, 0, run_road_tile, 0},
    {0x0047B960u, "place_canal_tile", 4, 0, 0, run_canal_tile, 0},
};
const int place_drag_ports_count = sizeof(place_drag_ports) / sizeof(place_drag_ports[0]);
