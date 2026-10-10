/* Imhotep - entertainment venues of the construction place command (package place): booths 0x1F,
 * bandstands 0x1E, pavilions 0x21 and the festival square 0xD1, placed at a road crossing.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The venue's square (2, 3, 4 or 5 tiles) is anchored at the hovered tile (0x00BCA304 / 0x00BCA308)
 * for the fit test 0x0043F410; its road tiles are copied into the 5 x 5 scratch grid 0x00775614
 * (0x004413B0, entry 5 dx + dy), and the parts (stage, booth, ...) take the free cells
 * (0x00441130), which it marks. */
#include "command/place_ent.h"

#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

#define G5(i) S32(st, G_GRID5 + 4u * (uint32_t) (i))
#define TER(o) U32(st, G_GRID_TERRAIN + 4u * (uint32_t) (o))
#define BGRID(o) S16(st, G_GRID_BUILDING + 2u * (uint32_t) (o))
#define OBJECT(o) U32(st, G_FLOODPLAIN_MANAGER + 4u * (uint32_t) (o))
#define B(id) BUILDING_ADDR(id)

static int32_t venue_size(int32_t type, int32_t other)
{
    /* @bin 0x0043F435 / 0x00441142 / 0x004413B5 */
    switch (type) {
    case 0x1F: return 2;
    case 0x1E: return 3;
    case 0x21: return 4;
    case 0xD1: return 5;
    default: return other;
    }
}

void ent_roads_4413b0(game_state *st, int32_t x, int32_t y, int32_t type)
{
    /* @bin 0x004413B0 the scratch grid cleared, then 1 for the road tiles (terrain 0x40) of the
     * venue's square at (x, y) (size 1 for other types) */
    int32_t size = venue_size(type, 1);
    for (int i = 0; i < 25; i++) {
        G5(i) = 0;
    }
    int32_t row = S32(st, G_MAP_START) + y * GRID_SIZE;
    for (int32_t dy = 0; dy < size; dy++, row += GRID_SIZE) {
        for (int32_t dx = 0; dx < size; dx++) {
            G5(5 * dx + dy) = (U8(st, G_GRID_TERRAIN + 4u * (uint32_t) (row + x + dx)) & 0x40) ? 1 : 0;
        }
    }
}

static uint8_t road_cell_3a0(game_state *st, int32_t a, int32_t b, int32_t size)
{
    /* @bin 0x0043F3A0 (a, b, size) the venue's road layout tables (0x0043F3FC by size - 2) */
    switch (size) {
    case 2: return U8(st, G_VENUE_ROADS_2 + (uint32_t) (a * 2 + b));
    case 3: return U8(st, G_VENUE_ROADS_3 + (uint32_t) (a * 3 + b));
    case 4: return U8(st, G_VENUE_ROADS_4 + (uint32_t) (a * 4 + b));
    case 5: return U8(st, G_VENUE_ROADS_5 + (uint32_t) (a * 5 + b));
    default: return 1;
    }
}

static uint8_t road_crossing_250(game_state *st, int32_t x, int32_t y, int32_t bx, int32_t by, int32_t size)
{
    /* @bin 0x0043F250 (x, y, block x, block y, size) a road tile of the block whose road runs to the
     * block's edges (directions 0x005D2914[4]: N, E, S, W) as a crossing */
    int32_t off = S32(st, G_MAP_START) + y * GRID_SIZE + x;
    if (!(U8(st, G_GRID_TERRAIN + 4u * (uint32_t) off) & 0x40)) {
        return 0;
    }
    int32_t d[4] = {y - by, bx + size - 1 - x, by + size - 1 - y, x - bx};
    int32_t roads = 0, far = 0, last = 0;
    for (int i = 0; i < 4; i++) {
        int32_t dir = S32(st, G_ROAD_DIRECTIONS4 + 4u * (uint32_t) i);
        uint32_t r = TER(off + dir) & 0x40u;
        for (int32_t t = 2; t <= d[i]; t++) {
            r &= TER(off + t * dir) & 0x40u;
        }
        if (r) {
            roads++;
            if (d[i] != 0) {
                far++;
            }
        } else {
            last = i;
        }
    }
    if (size == 2 && roads >= 3 && far >= size) {
        if (d[last] == 0 || roads == 4) {
            return 1;
        }
    } else if (roads >= 4 && far >= 3) {
        return 1;
    }
    if (size == 5) {
        return 0;
    }
    return roads == 3 && far == 3 && d[last] == 0;
}

int32_t ent_fits_43f410(game_state *st, int32_t type)
{
    /* @bin 0x0043F410 the square at the hovered tile: a road crossing on one of the layout's cells,
     * all tiles buildable or road (no blocking object, terrain & 0xEEFFD77F only with the road bit),
     * no building */
    int32_t x0 = S32(st, G_HOVER_X), y0 = S32(st, G_HOVER_Y);
    int32_t size = venue_size(type, 5);
    int land = 1, free = 1, found = 0;
    int32_t row = x0 + y0 * GRID_SIZE;
    int32_t dy = 0;
    for (int32_t cy = y0; cy < y0 + size; cy++, row += GRID_SIZE, dy++) {
        int32_t e = row, dx = 0;
        for (int32_t cx = x0; cx < x0 + size; cx++, e++, dx++) {
            if (road_cell_3a0(st, dx, dy, size) && road_crossing_250(st, cx, cy, x0, y0, size)) {
                found = 1;
            }
            int32_t o = S32(st, G_MAP_START) + e;
            uint32_t obj = OBJECT(o);
            uint32_t t = TER(o);
            if (obj != 0 && P8(st, obj + 0x18) == 0) {
                land = 0;
            }
            if ((t & 0xEEFFD77Fu) && !(t & 0x40u)) {
                land = 0;
            }
            if (BGRID(o) > 0) {
                free = 0;
            }
        }
    }
    return found && land && free;
}

/* @bin 0x00441040 (size, w, h, &count, list) the offsets (j, i) where a w x h part fits in the
 * free cells of the scratch grid (rows of j by i) */
static void free_spots_041040(game_state *st, int32_t size, int32_t w, int32_t h, int32_t *count, int32_t list[][2])
{
    int32_t rows = w > 1 ? size - w + 1 : size;
    int32_t cols = h > 1 ? size - h + 1 : size;
    int32_t n = 0;
    for (int32_t i = 0; i < cols; i++) {
        for (int32_t j = 0; j < rows; j++) {
            int ok = 1;
            for (int32_t k = 0; k < h && ok; k++) {
                for (int32_t m = 0; m < w; m++) {
                    if (G5(5 * j + k + i + 5 * m) != 0) {
                        ok = 0;
                        break;
                    }
                }
            }
            if (ok) {
                list[n][0] = j;
                list[n][1] = i;
                n++;
            }
        }
    }
    *count = n;
}

int32_t ent_spot_441130(game_state *st, int32_t x, int32_t y, int32_t *ox, int32_t *oy, int32_t type, int32_t part)
{
    /* @bin 0x00441130 (x, y, &x', &y', venue type, part type) the part's place in the square at
     * (x, y): parts 0x1F 1x1, 0x1E 1x2 (or 2x1 when it does not fit: returns 1), 0x21 2x2; the
     * lowest score of 0x005D2960 (size 3) / 0x005D2984 (size 4) wins, else the first; its cells
     * are marked; -1 when none fits */
    int32_t size = venue_size(type, 0);
    int32_t w = 1, h = 1, swapped = 0;
    int32_t list[25][2] = {{0, 0}};
    if (part == 0x1E) {
        h = 2;
        list[0][0] = list[0][1] = 1;
    } else if (part == 0x21) {
        w = h = 2;
        list[0][0] = list[0][1] = 2;
    }
    int32_t count;
    free_spots_041040(st, size, w, h, &count, list);
    if (count <= 0 && part == 0x1E) {
        swapped = 1;
        free_spots_041040(st, size, h, w, &count, list);
    }
    if (count <= 0) {
        *ox = x + list[0][0]; /* @bin 0x0044122A both from the first entry's j */
        *oy = y + list[0][0];
        return -1;
    }
    int32_t bi = 0, best = 5;
    for (int32_t i = 0; i < count && size != 2; i++) {
        int32_t score = 0;
        if (size == 3) {
            score = S32(st, G_VENUE_SCORES_3 + 4u * (uint32_t) (list[i][0] * 3 + list[i][1]));
        } else if (size == 4) {
            score = S32(st, G_VENUE_SCORES_4 + 4u * (uint32_t) (list[i][1] + list[i][0] * 4));
        }
        if (score < best) {
            best = score;
            bi = i;
        }
    }
    int32_t j = list[bi][0], i = list[bi][1];
    *ox = x + j;
    *oy = y + i;
    int32_t c = 5 * j + i;
    if (part == 0x1E) {
        G5(c) = 1;
        G5(swapped ? c + 5 : c + 1) = 1;
    } else if (part == 0x21) {
        G5(c) = 1;
        G5(c + 5) = 1;
        G5(c + 1) = 1;
        G5(c + 6) = 1;
    } else {
        G5(c) = 1;
    }
    return swapped;
}

static int gate_images(game_state *st, int32_t v, int32_t *first, int32_t *second)
{
    /* @bin 0x004405C0 (v, &first, &second) a two-part image pair by the view 0x00BCA2E0 (jump
     * table 0x004406D8) and the part direction v; nothing written for other values (the original's
     * outputs are then its caller's uninitialised stack). The same function is ported inside
     * core/building/rest_redraw.c (package rest), where it is static. */
    int32_t g = S32(st, G_IMAGE_GROUPS + 8u * 361u);
    int32_t a, c;
    switch (S32(st, G_VIEW_ORIENTATION)) {
    case 0: if (v == 0) { a = g; c = g + 1; } else if (v == 1) { a = g + 3; c = g + 2; } else { return 0; } break;
    case 2: if (v == 0) { a = g + 3; c = g + 2; } else if (v == 1) { a = g + 1; c = g; } else { return 0; } break;
    case 4: if (v == 0) { a = g + 1; c = g; } else if (v == 1) { a = g + 2; c = g + 3; } else { return 0; } break;
    case 6: if (v == 0) { a = g + 2; c = g + 3; } else if (v == 1) { a = g; c = g + 1; } else { return 0; } break;
    default: return 0;
    }
    *second = a;
    *first = c;
    return 1;
}

static void restore_all(game_state *st)
{
    GAME_CALL(st, FN_COPY_BYTES, G_TERRAIN_BACKUP, G_GRID_TERRAIN, 0x32C40);
    GAME_CALL(st, FN_COPY_BYTES, G_CANAL_BACKUP, G_CANAL, 0xCB10);
    GAME_CALL(st, FN_COPY_BYTES, G_BITS_BACKUP, G_GRID_BITFIELDS, 0xCB10);
    GAME_CALL(st, FN_COPY_BYTES, G_EDGE_BACKUP, G_GRID_EDGE, 0xCB10);
    GAME_CALL(st, FN_UNDO_RESTORE_IMAGES, 0);
}

static int32_t create(game_state *st, int32_t type, int32_t x, int32_t y)
{
    return GAME_CALL(st, FN_BUILDING_CREATE, S32(st, CURRENT_PLAYER_ADDR), type, x, y, 0, 0);
}

static void tiles(game_state *st, int32_t b, int32_t x, int32_t y, int32_t n, int32_t image)
{
    GAME_CALL(st, FN_MAP_BUILDING_TILES, b, x, y, n, n, image, 8);
}

int32_t place_booth(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    /* @bin 0x00440700 booth 0x1F: one part */
    (void) x1, (void) y1;
    int32_t x = x2 >= S32(st, G_MAP_WIDTH) ? S32(st, G_MAP_WIDTH) - 1 : x2;
    int32_t y = y2 >= S32(st, G_MAP_HEIGHT) ? S32(st, G_MAP_HEIGHT) - 1 : y2;
    int32_t off = S32(st, G_MAP_START) + y * GRID_SIZE + x;
    if (ent_fits_43f410(st, 0x1F) == 0) {
        return 0;
    }
    ent_roads_4413b0(st, x, y, 0x1F);
    int32_t nx, ny;
    if (ent_spot_441130(st, x, y, &nx, &ny, 0x1F, 0x1F) == -1) {
        return 0;
    }
    if (preview != 0) {
        return 1;
    }
    restore_all(st);
    S32(st, G_CONSTRUCTION_COUNT) = 1;
    int32_t b = create(st, 0x1F, nx, ny);
    int32_t img = S32(st, G_IMAGE_GROUPS + 8u * 383u);
    S32(st, B(b) + BLD_UD8) = 1;
    S32(st, B(b) + BLD_UD0) = off;
    tiles(st, b, nx, ny, 1, img);
    GAME_CALL(st, FN_BUILDING_KEEP_REMOVED, b);
    GAME_CALL(st, FN_BUILDING_AREA_REDRAW_440060, off, 0x1F, b);
    return 1;
}

int32_t place_bandstand(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    /* @bin 0x00440850 bandstand 0x1E: a two-tile stage (1x2 or 2x1 by the orientation returned by
     * 0x00441130) and a booth; parts linked by +0x38 / +0x3A */
    (void) x1, (void) y1;
    int32_t off = S32(st, G_MAP_START) + y2 * GRID_SIZE + x2;
    ent_roads_4413b0(st, x2, y2, 0x1E);
    if (ent_fits_43f410(st, 0x1E) == 0) {
        return 0;
    }
    int32_t sx, sy, bx, by;
    int32_t orient = ent_spot_441130(st, x2, y2, &sx, &sy, 0x1E, 0x1E);
    if (orient == -1) {
        return 0;
    }
    if (ent_spot_441130(st, x2, y2, &bx, &by, 0x1E, 0x1F) == -1) {
        return 0;
    }
    if (preview != 0) {
        return 1;
    }
    restore_all(st);
    int32_t first = 0, second = 0;
    S32(st, G_CONSTRUCTION_COUNT) = 1;
    if (!gate_images(st, orient, &first, &second)) {
        state_unported(st, FN_GATE_IMAGES_4405C0); /* the original's images are its uninitialised stack */
    }
    int32_t b1 = create(st, 0x1E, sx, sy);
    S32(st, B(b1) + BLD_UD8) = 1;
    S32(st, B(b1) + BLD_UD4) = orient;
    S32(st, B(b1) + BLD_UD0) = off;
    tiles(st, b1, sx, sy, 1, first);
    GAME_CALL(st, FN_BUILDING_KEEP_REMOVED, b1);
    int32_t b2;
    if (orient == 0) {
        b2 = create(st, 0x1E, sx, sy + 1);
        tiles(st, b2, sx, sy + 1, 1, second);
    } else {
        b2 = create(st, 0x1E, sx + 1, sy);
        tiles(st, b2, sx + 1, sy, 1, second);
    }
    S32(st, B(b2) + BLD_UD0) = off;
    S32(st, B(b1) + BLD_UD4) = orient;
    S32(st, B(b2) + BLD_UD8) = 0;
    GAME_CALL(st, FN_BUILDING_KEEP_REMOVED, b2);
    int32_t b3 = create(st, 0x1F, bx, by);
    int32_t img = S32(st, G_IMAGE_GROUPS + 8u * 383u);
    S32(st, B(b3) + BLD_UD8) = 0;
    S32(st, B(b3) + BLD_UD0) = off;
    tiles(st, b3, bx, by, 1, img);
    GAME_CALL(st, FN_BUILDING_KEEP_REMOVED, b3);
    S16(st, B(b1) + BLD_NEXT_PART) = (int16_t) b2;
    S16(st, B(b2) + BLD_PREV_PART) = (int16_t) b1;
    S16(st, B(b2) + BLD_NEXT_PART) = (int16_t) b3;
    S16(st, B(b3) + BLD_PREV_PART) = (int16_t) b2;
    GAME_CALL(st, FN_BUILDING_AREA_REDRAW_440060, off, 0x1E, b1);
    return 1;
}

int32_t place_pavilion(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    /* @bin 0x00440B10 pavilion 0x21: a 2x2 stage, a booth and a two-tile bandstand part, linked in
     * that order by +0x38 / +0x3A */
    (void) x1, (void) y1;
    int32_t x = x2 >= S32(st, G_MAP_WIDTH) ? S32(st, G_MAP_WIDTH) - 1 : x2;
    int32_t y = y2 >= S32(st, G_MAP_HEIGHT) ? S32(st, G_MAP_HEIGHT) - 1 : y2;
    int32_t off = S32(st, G_MAP_START) + y * GRID_SIZE + x;
    ent_roads_4413b0(st, x, y, 0x21);
    if (ent_fits_43f410(st, 0x21) == 0) {
        return 0;
    }
    int32_t px, py, sx, sy, bx, by;
    if (ent_spot_441130(st, x, y, &px, &py, 0x21, 0x21) == -1) {
        return 0;
    }
    int32_t orient = ent_spot_441130(st, x, y, &sx, &sy, 0x21, 0x1E);
    if (orient == -1) {
        return 0;
    }
    if (ent_spot_441130(st, x, y, &bx, &by, 0x21, 0x1F) == -1) {
        return 0;
    }
    if (preview != 0) {
        return 1;
    }
    restore_all(st);
    S32(st, G_CONSTRUCTION_COUNT) = 1;
    int32_t b1 = create(st, 0x21, px, py);
    int32_t img = S32(st, G_IMAGE_GROUPS + 8u * 317u);
    S32(st, B(b1) + BLD_UD8) = 1;
    S32(st, B(b1) + BLD_UD0) = off;
    tiles(st, b1, px, py, 2, img);
    GAME_CALL(st, FN_BUILDING_KEEP_REMOVED, b1);
    int32_t b2 = create(st, 0x1F, bx, by);
    img = S32(st, G_IMAGE_GROUPS + 8u * 383u);
    S32(st, B(b2) + BLD_UD8) = 0;
    S32(st, B(b2) + BLD_UD0) = off;
    tiles(st, b2, bx, by, 1, img);
    GAME_CALL(st, FN_BUILDING_KEEP_REMOVED, b2);
    int32_t first = 0, second = 0;
    if (!gate_images(st, orient, &first, &second)) {
        state_unported(st, FN_GATE_IMAGES_4405C0); /* the original's images are its uninitialised stack */
    }
    int32_t b3 = create(st, 0x1E, sx, sy);
    S32(st, B(b3) + BLD_UD8) = 0;
    S32(st, B(b3) + BLD_UD4) = orient;
    S32(st, B(b3) + BLD_UD0) = off;
    tiles(st, b3, sx, sy, 1, first);
    GAME_CALL(st, FN_BUILDING_KEEP_REMOVED, b3);
    int32_t b4;
    if (orient == 0) {
        b4 = create(st, 0x1E, sx, sy + 1);
        tiles(st, b4, sx, sy + 1, 1, second);
    } else {
        b4 = create(st, 0x1E, sx + 1, sy);
        tiles(st, b4, sx + 1, sy, 1, second);
    }
    S32(st, B(b4) + BLD_UD8) = 0;
    S32(st, B(b4) + BLD_UD0) = off;
    S32(st, B(b3) + BLD_UD4) = orient;
    S32(st, B(b2) + BLD_UD4) = orient;
    S32(st, B(b1) + BLD_UD4) = orient;
    GAME_CALL(st, FN_BUILDING_KEEP_REMOVED, b4);
    S16(st, B(b1) + BLD_NEXT_PART) = (int16_t) b2;
    S16(st, B(b2) + BLD_PREV_PART) = (int16_t) b1;
    S16(st, B(b2) + BLD_NEXT_PART) = (int16_t) b3;
    S16(st, B(b3) + BLD_PREV_PART) = (int16_t) b2;
    S16(st, B(b3) + BLD_NEXT_PART) = (int16_t) b4;
    S16(st, B(b4) + BLD_PREV_PART) = (int16_t) b3;
    GAME_CALL(st, FN_BUILDING_AREA_REDRAW_440060, off, 0x21, b1);
    return 1;
}

int32_t place_festival_square(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    /* @bin 0x00440ED0 festival square 0xD1: needs a free slot CITY+0x47A0 / +0x47A4 (-1); the
     * square's position stored there, CITY+0x4814 := 1 */
    (void) x1, (void) y1;
    uint32_t c = CURRENT_CITY(st);
    if (S32(st, c + CITY_FESTIVAL_SQUARE_X) != -1 && S32(st, c + CITY_FESTIVAL_SQUARE_Y) != -1) {
        return 0;
    }
    if (ent_fits_43f410(st, 0xD1) == 0) {
        return 0;
    }
    if (preview != 0) {
        return 1;
    }
    restore_all(st);
    int32_t x = x2 >= S32(st, G_MAP_WIDTH) ? S32(st, G_MAP_WIDTH) - 1 : x2;
    int32_t y = y2 >= S32(st, G_MAP_HEIGHT) ? S32(st, G_MAP_HEIGHT) - 1 : y2;
    S32(st, G_CONSTRUCTION_COUNT) = 1;
    c = CURRENT_CITY(st);
    int32_t off = S32(st, G_MAP_START) + y * GRID_SIZE + x;
    S32(st, c + CITY_FESTIVAL_SQUARE_X) = x;
    S32(st, c + CITY_FESTIVAL_SQUARE_Y) = y;
    ent_roads_4413b0(st, x, y, 0xD1);
    int32_t b = create(st, 0xD1, x, y);
    GAME_CALL(st, FN_BUILDING_KEEP_REMOVED, b);
    S32(st, B(b) + BLD_UD0) = off;
    S32(st, B(b) + BLD_UD8) = 1;
    GAME_CALL(st, FN_BUILDING_AREA_REDRAW_440060, off, 0xD1, b);
    S16(st, CURRENT_CITY(st) + CITY_U4814) = 1;
    return 1;
}

static int32_t run_roads(game_state *st, const int32_t *a)
{
    ent_roads_4413b0(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_fits(game_state *st, const int32_t *a)
{
    return ent_fits_43f410(st, a[0]);
}

static int32_t run_cell(game_state *st, const int32_t *a)
{
    return road_cell_3a0(st, a[0], a[1], a[2]);
}

static int32_t run_crossing(game_state *st, const int32_t *a)
{
    return road_crossing_250(st, a[0], a[1], a[2], a[3], a[4]);
}

#define RUN5(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) { return fn(st, a[0], a[1], a[2], a[3], a[4]); }
RUN5(run_booth, place_booth)
RUN5(run_bandstand, place_bandstand)
RUN5(run_pavilion, place_pavilion)
RUN5(run_festival, place_festival_square)

const ported_function place_ent_ports[] = {
    {0x004413B0u, "ent_roads_4413b0", 3, 0, 0, run_roads, 0},
    {0x0043F410u, "ent_fits_43f410", 1, 32, 0, run_fits, 0},
    {0x0043F3A0u, "road_cell_3a0", 3, 8, 0, run_cell, 0},
    {0x0043F250u, "road_crossing_250", 5, 8, 0, run_crossing, 0},
    {0x00441040u, "free_spots_041040 (writes its caller's stack)", 5, 8, 0, NULL, 0},
    {0x00441130u, "ent_spot_441130 (writes its caller's stack)", 6, 32, 0, NULL, 0},
    {0x00440700u, "place_booth", 5, 32, 0, run_booth, 0},
    {0x00440850u, "place_bandstand", 5, 32, 0, run_bandstand, 0},
    {0x00440B10u, "place_pavilion", 5, 32, 0, run_pavilion, 0},
    {0x00440ED0u, "place_festival_square", 5, 32, 0, run_festival, 0},
};
const int place_ent_ports_count = sizeof(place_ent_ports) / sizeof(place_ent_ports[0]);
