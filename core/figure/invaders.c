/* Imhotep - the enemy formations' daily update: new soldiers, moves (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Formation record: docs/formats/save.md §5 (chunk 24); figure record: chunk 21. */
#include "figure/formation.h"

#include "figure/figure_create.h"
#include "map/route_army.h"
#include "random/crt_rand.h"
#include "random/random.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


/* @bin 0x005E4C20 + 0x30 * kind: +0 figure type, +4 terrain mask, +8 figures, +0x10 spawn delay,
 * +0x14 move delay, +0x18 move radius; 0x005E4CB0 + 0x30 * (sub + 2 * kind): the same with +0xC
 * "only from empty" flag */
static uint32_t kind_record(game_state *st)
{
    return G_HERD_RECORDS + 0x30u * (U32(st, G_U784CB0) & 0xFF);
}

static uint32_t sub_record(game_state *st)
{
    return G_INVASION_SUB_RECORDS + 0x30u * (uint32_t) (S16(st, G_INVASION_SUB) + 2 * (int32_t) (U32(st, G_U784CB0) & 0xFF));
}

int32_t invader_tile_ok(game_state *st, uint32_t terrain, int32_t mode)
{
    /* @bin 0x004EC410 can an invader stand on a tile with these terrain flags (mask of the kind's
     * record, +4 when `mode` (low byte), else of the sub record) */
    uint32_t mask = (uint8_t) mode != 0 ? U32(st, kind_record(st) + 4) : U32(st, sub_record(st) + 4);
    if (terrain & 0x3038D022u) {
        return 0;
    }
    uint32_t v = terrain & 0x26351277u;
    if (mask == 0 && v == 0) {
        return 1;
    }
    if (S32(st, G_EDITOR) == 1) {
        return (v & mask) != 0;
    }
    if ((v & 4) && !(mask & 4)) {
        return 0;
    }
    if ((v & 0x40000) && !(mask & 0x40000)) {
        return 0;
    }
    return 1;
}

int32_t formation_count_0x44(game_state *st, int32_t id, int32_t action)
{
    /* @bin 0x004EAA60 figures of types 0x44..0x46 in formation `id` (+0x60) in `action` */
    int32_t n = 0;
    for (int32_t f = 1; f < FIGURE_COUNT; f++) {
        uint32_t a = FIGURE_ADDR(f);
        if (U8(st, a + FIG_STATE) != 0 && figure_type_is_0x44_0x46(U8(st, a + FIG_TYPE)) && S16(st, a + FIG_FORMATION) == id &&
            (int32_t) U8(st, a + FIG_ACTION) == action) {
            n++;
        }
    }
    return n;
}

int32_t formation_count_0x44_active(game_state *st, int32_t id)
{
    /* @bin 0x004EAA10 the same in any action but 4 and 0xC */
    int32_t n = 0;
    for (int32_t f = 1; f < FIGURE_COUNT; f++) {
        uint32_t a = FIGURE_ADDR(f);
        if (U8(st, a + FIG_STATE) != 0 && figure_type_is_0x44_0x46(U8(st, a + FIG_TYPE)) && S16(st, a + FIG_FORMATION) == id &&
            U8(st, a + FIG_ACTION) != 4 && U8(st, a + FIG_ACTION) != 0xC) {
            n++;
        }
    }
    return n;
}

int32_t formation_find_place(game_state *st, int32_t id, int32_t x, int32_t y)
{
    /* @bin 0x004BA7E0 within 10 tiles of (x, y), reachable (0x0051A420 by cost, 600 tiles) the first
     * place (rows then columns of growing squares) where all the formation's figures fit in its
     * layout (+0x2A, offsets 0x005CD6F8: no road 4, no 0x1200, reached, no other formation's
     * figure); in 0x00BCB9A8 / 0x00BCB9A4 */
    uint32_t f = FORMATION_ADDR(id);
    int32_t n = U8(st, f + FM_COUNT);
    uint32_t layout = G_FORMATION_LAYOUT_X + 0x80u * (uint32_t) (int32_t) S16(st, f + FM_LAYOUT);
    int32_t base = S32(st, layout) + S32(st, layout + 4) * GRID_SIZE + S32(st, G_MAP_START);
    S32(st, G_PLACE_OFFSETS) = 0;
    for (int32_t i = 1; i < n; i++) {
        uint32_t e = layout + 8u * (uint32_t) i;
        S32(st, G_PLACE_OFFSETS + 4u * (uint32_t) i) = S32(st, e + 4) * GRID_SIZE - base + S32(st, G_MAP_START) + S32(st, e);
    }
    route_army_distances(st, x, y, -1, -1, 0, 600);
    for (int32_t r = 0; r <= 10; r++) {
        int32_t x0 = x - r, y0 = y - r, x1 = x + r, y1 = y + r;
        if (x0 < 0) {
            x0 = 0;
        }
        if (y0 < 0) {
            y0 = 0;
        }
        if (x1 >= S32(st, G_MAP_WIDTH)) {
            x1 = S32(st, G_MAP_WIDTH) - 1;
        }
        if (y1 >= S32(st, G_MAP_HEIGHT)) {
            y1 = S32(st, G_MAP_HEIGHT) - 1;
        }
        for (int32_t row = y0; row <= y1; row++) {
            int32_t rowt = S32(st, G_MAP_START) + row * GRID_SIZE;
            for (int32_t col = x0; col <= x1; col++) {
                int32_t j = 0;
                for (; j < n; j++) {
                    uint32_t t = (uint32_t) (rowt + col + S32(st, G_PLACE_OFFSETS + 4u * (uint32_t) j));
                    uint32_t v = U32(st, G_GRID_TERRAIN + 4 * t);
                    if ((v & 4) || (v & 0x1200) || S32(st, G_GRID_ROAD_DISTANCE + 4 * t) <= 0) {
                        break;
                    }
                    int16_t fid = S16(st, G_FIGURE_GRID + 2 * t);
                    if (fid != 0 && S16(st, FIGURE_ADDR(fid) + FIG_FORMATION) != id) {
                        break;
                    }
                }
                if (j >= n) {
                    S32(st, G_PLACE_X) = col;
                    S32(st, G_PLACE_Y) = row;
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* @bin 0x004EB515 one tile of the ring search */
static int ring_tile_ok(game_state *st, int32_t x0, int32_t y0, const int32_t *x, const int32_t *y, int32_t mode)
{
    if (*x < 0 || *y < 0 || *x >= S32(st, G_MAP_WIDTH) || *y >= S32(st, G_MAP_HEIGHT)) {
        return 0;
    }
    uint32_t t = U32(st, G_TILE_CURSOR);
    if (U8(st, G_DESIRABILITY_GRID + t) != 0 || (U32(st, G_GRID_TERRAIN + 4 * t) & 0x80000)) {
        return 0;
    }
    if (!invader_tile_ok(st, U32(st, G_GRID_TERRAIN + 4 * t), mode)) {
        return 0;
    }
    return route_walk_distances(st, x0, y0, *x, *y) != 0;
}

int32_t invader_find_tile(game_state *st, int32_t x0, int32_t y0, int32_t r, int32_t *x, int32_t *y, int32_t mode)
{
    /* @bin 0x004EB4C0 on rings around (x0, y0), from radius r inwards (each ring clockwise from its
     * top left corner), the first tile an invader may stand on (0x004EC410, grid 0x00AC2D4C clear,
     * terrain without 0x80000) that 0x005181C0 accepts; its position in *x, *y */
    int32_t w = 2 * r + 1;
    *x = x0 - r;
    *y = y0 - r;
    int32_t row = *x + *y * GRID_SIZE + S32(st, G_MAP_START);
    for (int32_t ring = 0; ring < GRID_SIZE; ring++) {
        S32(st, G_TILE_CURSOR) = row;
        for (int32_t i = 0; i < w; i++) {
            if (ring_tile_ok(st, x0, y0, x, y, mode)) {
                return 1;
            }
            S32(st, G_TILE_CURSOR) += 1;
            *x += 1;
        }
        S32(st, G_TILE_CURSOR) -= 1;
        *x -= 1;
        for (int32_t i = 0; i < w; i++) {
            if (ring_tile_ok(st, x0, y0, x, y, mode)) {
                return 1;
            }
            S32(st, G_TILE_CURSOR) += GRID_SIZE;
            *y += 1;
        }
        S32(st, G_TILE_CURSOR) -= GRID_SIZE;
        *y -= 1;
        for (int32_t i = 0; i < w; i++) {
            if (ring_tile_ok(st, x0, y0, x, y, mode)) {
                return 1;
            }
            S32(st, G_TILE_CURSOR) -= 1;
            *x -= 1;
        }
        S32(st, G_TILE_CURSOR) += 1;
        *x += 1;
        for (int32_t i = 0; i < w; i++) {
            if (ring_tile_ok(st, x0, y0, x, y, mode)) {
                return 1;
            }
            S32(st, G_TILE_CURSOR) -= GRID_SIZE;
            *y -= 1;
        }
        S32(st, G_TILE_CURSOR) += GRID_SIZE;
        *y += 1;
        if (r == 0) {
            return 0;
        }
        w -= 2;
        row += GRID_SIZE + 1;
        *x += 1;
        *y += 1;
        r--;
    }
    return 0;
}

/* @bin 0x004B9509 one of 16 tiles around the formation's camp (+0x36, +0x38) from a random start
 * k: free (no building) and fit (0x004EC410). Returns its index; -1: none (the last one taken or
 * unfit); -2: none, though the last one tried is free and fit */
static int32_t camp_tile(game_state *st, uint32_t f, int32_t k, int32_t mode)
{
    int32_t n = 0;
    uint32_t t = 0;
    for (; n < 16; n++) {
        t = (uint32_t) (S32(st, G_MAP_START) + S16(st, f + FM_U36) + S32(st, G_RING16_DX + 8u * (uint32_t) k) +
                        (S16(st, f + FM_U38) + S32(st, G_RING16_DY + 8u * (uint32_t) k)) * GRID_SIZE);
        if (invader_tile_ok(st, U32(st, G_GRID_TERRAIN + 4 * t), mode) && S16(st, G_BUILDING_GRID + 2 * t) == 0) {
            break;
        }
        k++;
        if (k >= 16) {
            k = 0;
        }
    }
    if (n == 16 && S16(st, G_BUILDING_GRID + 2 * t) != 0) {
        return -1;
    }
    if (!invader_tile_ok(st, U32(st, G_GRID_TERRAIN + 4 * t), mode)) {
        return -1;
    }
    return n == 16 ? -2 : k;
}

/* a new soldier at camp position k of table `offsets` (mode 1: 0x005CD978, mode 2: the search's own
 * 0x005CDB78) */
static void new_soldier(game_state *st, int32_t id, uint32_t f, int32_t k, int32_t dir, uint32_t offsets)
{
    int16_t type = S16(st, f + FM_TYPE);
    int32_t n = figure_create(st, type, type, S16(st, f + FM_U36) + S32(st, offsets + 8u * (uint32_t) k),
                              S16(st, f + FM_U38) + S32(st, offsets + 4 + 8u * (uint32_t) k), dir);
    uint32_t a = FIGURE_ADDR(n);
    U8(st, a + FIG_ACTION) = 0x18;
    S16(st, a + FIG_FORMATION) = (int16_t) id;
    S16(st, a + FIG_WAIT_TICKS) = 10;
}

/* @bin 0x004B982C a new target: a fit tile near the current one (0x004EB4C0), then a free place for
 * the formation there (0x004BA7E0, position in 0x00BCB9A8 / 0x00BCB9A4) */
static int move_to(game_state *st, int32_t id, uint32_t f, int32_t radius, int32_t mode, int direct)
{
    int32_t x, y;
    if (direct) {
        x = S16(st, f + FM_U36);
        y = S16(st, f + FM_U38);
    } else if (!invader_find_tile(st, S16(st, f + FM_DEST_X), S16(st, f + FM_DEST_Y), radius, &x, &y, mode)) {
        return 0;
    }
    if (formation_find_place(st, id, x, y) == 0) {
        return 0;
    }
    S16(st, f + FM_DEST_X) = S16(st, G_PLACE_X);
    S16(st, f + FM_DEST_Y) = S16(st, G_PLACE_Y);
    return 1;
}

/* @bin 0x004B93C8 mode 1: soldiers up to the sub record's number, else moves */
static void invaders_mode1(game_state *st, int32_t id, uint32_t f, int16_t type, uint8_t max)
{
    S16(st, f + FM_TYPE) = type;
    U8(st, f + FM_MAX_COUNT) = max;
    uint8_t count = U8(st, f + FM_COUNT);
    uint32_t s = sub_record(st);
    if (count < max && (U8(st, s + 0xC) == 0 || count != 0)) {
        uint8_t d = (uint8_t) (U8(st, f + FM_U75) + 1);
        U8(st, f + FM_U75) = d;
        if ((int32_t) d <= S32(st, s + 0x10)) {
            return;
        }
        U8(st, f + FM_U75) = 0;
        random_next_state(st);
        int32_t k = camp_tile(st, f, (int32_t) (U32(st, G_R_1_15) & 0xF), 0);
        if (k < 0) {
            return;
        }
        random_next_state(st);
        new_soldier(st, id, f, k, (int32_t) (U32(st, G_R_1_15) & 7), G_DIRECTIONS); /* @bin 0x004B965B */
        return;
    }
    S16(st, f + FM_U54) += 1;
    s = sub_record(st);
    if (S16(st, f + FM_U54) <= S32(st, s + 0x14)) {
        return;
    }
    int32_t radius = S32(st, s + 0x18);
    S16(st, f + FM_U54) = 0;
    U8(st, f + FM_U76) = 0;
    radius >>= 1;
    if (radius != 0) {
        radius += crt_rand(st) % radius;
    }
    int direct = 0;
    if (S16(st, f + FM_DEST_X) != S16(st, f + FM_U36) || S16(st, f + FM_DEST_Y) != S16(st, f + FM_U38)) {
        random_next_state(st);
        direct = (U32(st, G_R_1_15) & 7) == 0;
    }
    move_to(st, id, f, radius, 0, direct);
}

/* @bin 0x004B98B0 mode 2: soldiers up to the kind record's number (twice as fast with `halve`), then
 * moves when none of them waits (actions 0xE, 0x10 of types 0x44..0x46, +0x74) */
static void invaders_mode2(game_state *st, int32_t id, uint32_t f, int16_t type, uint8_t max, int halve)
{
    S16(st, f + FM_TYPE) = type;
    U8(st, f + FM_MAX_COUNT) = max;
    formation_count_0x44(st, id, 0x10);
    formation_count_0x44_active(st, id);
    U8(st, f + FM_U74) = formation_count_0x44(st, id, 0x11) != 0 ? 0x11 : 0;
    if (U8(st, f + FM_COUNT) < U8(st, f + FM_MAX_COUNT)) {
        uint32_t k2 = kind_record(st);
        int32_t limit = halve ? S32(st, k2 + 0x10) / 2 : S32(st, k2 + 0x10);
        uint8_t d = (uint8_t) (U8(st, f + FM_U75) + 1);
        U8(st, f + FM_U75) = d;
        if ((int32_t) d > limit) {
            U8(st, f + FM_U75) = 0;
            random_next_state(st);
            int32_t k = camp_tile(st, f, (int32_t) (U32(st, G_R_1_15) & 0xF), 1);
            if (k == -1) {
                return;
            }
            if (k >= 0) {
                new_soldier(st, id, f, k, 0, G_RING16_DX); /* @bin 0x004B9AC0 */
            }
        }
    }
    /* @bin 0x004B9B19 */
    S16(st, f + FM_U54) += 1;
    if (S16(st, f + FM_U54) <= S32(st, kind_record(st) + 0x14)) {
        return;
    }
    if (formation_count_0x44(st, id, 0xE) != 0 || formation_count_0x44(st, id, 0x10) != 0 || U8(st, f + FM_U74) == 0x11) {
        return;
    }
    S16(st, f + FM_U54) = 0;
    U8(st, f + FM_U76) = 0;
    int32_t radius = S32(st, kind_record(st) + 0x18) >> 1;
    if (radius != 0) {
        /* @bin 0x004B9BAA two random steps; the first one's 15-bit value widens the radius */
        random_next_state(st);
        radius += (int32_t) U32(st, G_R_1_15) % radius;
    }
    random_next_state(st);
    U8(st, f + FM_U76) = (uint8_t) (U32(st, G_R_1_15) & 7);
    int direct = 0;
    if (S16(st, f + FM_DEST_X) != S16(st, f + FM_U36) || S16(st, f + FM_DEST_Y) != S16(st, f + FM_U38)) {
        random_next_state(st);
        direct = (U32(st, G_R_1_15) & 7) == 0;
    }
    if (!move_to(st, id, f, radius, 1, direct)) {
        return;
    }
    /* @bin 0x004B9E51 its soldiers follow (action 8) unless fighting or leaving */
    for (uint32_t k = 0; k < 16; k++) {
        int16_t fid = S16(st, f + FM_FIGURES + 2 * k);
        if (fid <= 0) {
            continue;
        }
        uint32_t a = FIGURE_ADDR(fid);
        uint8_t act = U8(st, a + FIG_ACTION);
        if (act != 5 && act != 4 && act != 0xC && act != 0xE && act != 0x10 && act != 0x11) {
            U8(st, a + FIG_ACTION) = 8;
            S16(st, a + FIG_WAIT_TICKS) = 0;
        }
    }
}

void formations_invaders(game_state *st)
{
    /* @bin 0x004B92E0 the formations not of the player (+0x42 0) in an invader mode (+0x69) */
    uint32_t kr = kind_record(st), sr = sub_record(st);
    int16_t type2 = (int16_t) S32(st, kr), type1 = (int16_t) S32(st, sr);
    uint8_t max2 = (uint8_t) S32(st, kr + 8), max1 = (uint8_t) S32(st, sr + 8);
    int halve = 0;
    uint32_t c = CURRENT_CITY(st);
    if (S16(st, c + CITY_U4478) != 0 && GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, S32(st, c + CITY_U472C)) == 0 &&
        (uint8_t) GAME_CALL(st, FN_BUILDING_0XA9_IS, S32(st, CURRENT_CITY(st) + CITY_U472C), 2) != 0) {
        halve = 1;
    }
    for (int32_t id = 1; id < 50; id++) {
        uint32_t f = FORMATION_ADDR(id);
        if (U8(st, f + FM_IN_USE) != 1 || U8(st, f + FM_PLAYER) != 0 || U8(st, f + FM_HALTED) == 0) {
            continue;
        }
        if (U8(st, f + FM_HALTED) == 1) {
            invaders_mode1(st, id, f, type1, max1);
        } else {
            invaders_mode2(st, id, f, type2, max2, halve);
        }
    }
}

static int32_t run_find_place(game_state *st, const int32_t *a)
{
    return formation_find_place(st, a[0], a[1], a[2]);
}

static int32_t run_invaders(game_state *st, const int32_t *a)
{
    (void) a;
    formations_invaders(st);
    return 0;
}

static int32_t run_tile_ok(game_state *st, const int32_t *a)
{
    return invader_tile_ok(st, (uint32_t) a[0], a[1]);
}

static int32_t run_count_0x44(game_state *st, const int32_t *a)
{
    return formation_count_0x44(st, a[0], a[1]);
}

static int32_t run_count_0x44_active(game_state *st, const int32_t *a)
{
    return formation_count_0x44_active(st, a[0]);
}

const ported_function bigb_invader_ports[] = {
    {0x004B92E0u, "formations_invaders", 0, 0, 0, run_invaders, 0},
    {0x004EC410u, "invader_tile_ok", 2, 8, 0, run_tile_ok, 0},
    {0x004EAA60u, "formation_count_0x44", 2, 32, 0, run_count_0x44, 0},
    {0x004EAA10u, "formation_count_0x44_active", 1, 32, 0, run_count_0x44_active, 0},
    {0x004EB4C0u, "invader_find_tile (in invaders.c)", 6, 32, 0, NULL, 0},
    {0x004BA7E0u, "formation_find_place", 3, 32, 0, run_find_place, 0},
};
const int bigb_invader_ports_count = sizeof(bigb_invader_ports) / sizeof(bigb_invader_ports[0]);
