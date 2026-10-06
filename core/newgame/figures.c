/* Imhotep - the figures placed when a map starts (package initfig, docs/newgame.md §3.3, §6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Called by the map load 0x004DE600 (0x004BB3F0, 0x004BBC10) and the custom-scenario setup
 * 0x004DE820 (0x004BBD40). Scenario fields read here (the scenario info block, docs/formats/save.md):
 *   0x00784B64 / 0x00784B6C   4 herd points, x / y (i16; x > 0 in use)
 *   0x00784CC0 / 0x00784CD0   4 points of the second herd kind, x / y (i32; x > 0 in use)
 *   0x00784B74 / 0x00784B84   8 points, x / y (i16; x > 0 in use)
 *   0x00784CB0 (low byte), 0x00784B94 (i16)   the indices into the records 0x005E4C20 (0x30 bytes)
 *   0x00784CB1   flag read by 0x004BBD40
 * The original steps the game RNG (0x004D7090's code, inlined) and calls the CRT rand 0x00562348;
 * both are reproduced in the same order. Figure names come from refs only as hints:
 * @hint akhenaten:src/figure/figure_type.h types 0x41 fishing point, 0x5F fishing spot, 0x25 flotsam */
#include "newgame/figures.h"

#include "figure/actions_a.h"
#include "figure/figure.h"
#include "figure/figure_create.h"
#include "figure/formation.h"
#include "figure/helpers_xa.h"
#include "random/crt_rand.h"
#include "random/random.h"
#include "state/ported.h"
#include "state/records.h"

#define EDITOR 0x00605A3Cu           /* @bin 0x004BB43B editor flag: no figures placed */
#define CLIMATE 0x00784CB0u
#define HERD_RECORDS 0x005E4C20u     /* @bin 0x004BB407 records of 0x30 bytes */
#define TERRAIN 0x00AF598Cu          /* @bin 0x004BB5E4 terrain grid, u32 per tile */
#define GRID_OFFSET_BASE 0x00BCA2D8u /* @bin 0x004BB5D7 added to y * 228 + x */
#define CURRENT_FIGURE 0x00D056F8u

/* @bin 0x004BB4F1 one herd: `count` animals of `type` around (x, y), each on the first tile of
 * the 16 offsets of `offsets` (from offset j, the animal's index) that 0x004EC410 accepts for
 * `mode`; +0x36 a random phase (`shift` 2: times 4 for the first kind) */
static void herd_animals(game_state *st, int32_t formation, int32_t type, int32_t count, int32_t x,
                         int32_t y, uint32_t offsets, int32_t mode, int shift)
{
    for (int32_t j = 0; j < count; j++) {
        random_next_state(st); /* @bin 0x004BB503 */
        int32_t k = j;
        for (int32_t n = 0; n < 16; n++) {
            int32_t dx = S32(st, offsets + 8u * (uint32_t) k);
            int32_t dy = S32(st, offsets + 8u * (uint32_t) k + 4u);
            int32_t t = (dy + y) * GRID_SIZE + dx + S32(st, GRID_OFFSET_BASE) + x;
            if ((uint8_t) invader_tile_ok(st, U32(st, TERRAIN + 4u * (uint32_t) t), mode) != 0) {
                break;
            }
            k++;
            if (k >= 16) {
                k = 0;
            }
        }
        random_next_state(st); /* @bin 0x004BB606 */
        int32_t dx = S32(st, offsets + 8u * (uint32_t) k);
        int32_t dy = S32(st, offsets + 8u * (uint32_t) k + 4u);
        /* @bin 0x004BB6F4 direction: the 3-bit random value */
        int32_t f = figure_create(st, 1, type, dx + x, dy + y, (int32_t) (U32(st, 0x00D3B384u) & 7));
        uint32_t fa = FIGURE_ADDR(f);
        /* @bin 0x004BB718 the RNG pool push of the next step comes before the figure writes */
        random_state r;
        random_from_state(st, &r);
        U8(st, fa + 0x38) = 0x18;
        U16(st, fa + 0x60) = (uint16_t) formation;
        if (shift) {
            U16(st, fa + 0x36) = 0; /* @bin 0x004BB731 */
        }
        random_next(&r);
        random_to_state(st, &r);
        /* @bin 0x004BB7F7 15-bit value mod 25 (idiv), times 4 for the first kind (0x004BB801) */
        int32_t phase = (int32_t) r.r1_15bit % 25;
        U16(st, fa + 0x36) = (uint16_t) (shift ? phase << 2 : phase);
        U8(st, fa + 0x62) = (uint8_t) j;
    }
}

/* @bin 0x004BB3F0 */
void newgame_herds_create(game_state *st)
{
    uint32_t cb0 = U32(st, CLIMATE);
    uint32_t r2 = HERD_RECORDS + (cb0 & 0xFF) * 0x30u;
    int32_t type2 = S32(st, r2);         /* @bin 0x004BB407 */
    int32_t count2 = S32(st, r2 + 8);    /* @bin 0x004BB40D */
    uint32_t r1 = HERD_RECORDS + 0x90u + (uint32_t) ((int32_t) S16(st, 0x00784B94u) + (int32_t) (cb0 & 0xFF) * 2) * 0x30u;
    int32_t type1 = S32(st, r1);         /* @bin 0x004BB42B 0x005E4CB0 */
    int32_t count1 = S32(st, r1 + 8);    /* @bin 0x004BB431 0x005E4CB8 */
    if (S32(st, EDITOR) != 0) {
        return;
    }
    for (uint32_t p = 0; p < 4; p++) {
        int32_t x = S16(st, 0x00784B64u + 2u * p);
        if (x <= 0) {
            continue;
        }
        int32_t y = S16(st, 0x00784B6Cu + 2u * p);
        /* @bin 0x004BB46F the formation kind 0x005E4CDC of the same record */
        uint32_t rk = 0x005E4CDCu + (uint32_t) ((int32_t) S16(st, 0x00784B94u) + (int32_t) (cb0 & 0xFF) * 2) * 0x30u;
        int32_t fm = formation_create_0x4b6ff0(st, type1, S32(st, rk), 0, x, y);
        if (fm > 0) {
            uint32_t a = FORMATION_ADDR(fm);
            U16(st, a + 0x3A) = (uint16_t) S16(st, 0x00784B64u + 2u * p);
            U16(st, a + 0x3C) = (uint16_t) S16(st, 0x00784B6Cu + 2u * p);
            U8(st, a + 0x74) = 0;
            U8(st, a + 0x69) = 1;
            U16(st, a + 0x4E) = 0x18;
            U8(st, a + 0x29) = (uint8_t) count1;
            /* @bin 0x004BB5B5 offsets 0x005CD978 (dx, dy i32), tiles for mode 0 */
            herd_animals(st, fm, type1, count1, S16(st, 0x00784B64u + 2u * p), S16(st, 0x00784B6Cu + 2u * p),
                         0x005CD978u, 0, 1);
        }
        cb0 = U32(st, CLIMATE); /* @bin 0x004BB822 */
    }
    for (uint32_t p = 0; p < 4; p++) {
        int32_t x = S32(st, 0x00784CC0u + 4u * p);
        if (x <= 0) {
            continue;
        }
        int32_t y = S32(st, 0x00784CD0u + 4u * p);
        /* @bin 0x004BB863 the formation kind 0x005E4C4C */
        int32_t fm = formation_create_0x4b6ff0(st, type2, S32(st, 0x005E4C4Cu + (cb0 & 0xFF) * 0x30u), 0, x, y);
        if (fm > 0) {
            uint32_t a = FORMATION_ADDR(fm);
            U16(st, a + 0x3A) = (uint16_t) S32(st, 0x00784CC0u + 4u * p);
            U16(st, a + 0x3C) = (uint16_t) S32(st, 0x00784CD0u + 4u * p);
            U8(st, a + 0x74) = 0;
            U8(st, a + 0x69) = 2;
            U16(st, a + 0x4E) = 0x18;
            U8(st, a + 0x29) = (uint8_t) count2;
            /* @bin 0x004BB98F offsets 0x005CDB78, tiles for mode 1 */
            herd_animals(st, fm, type2, count2, S32(st, 0x00784CC0u + 4u * p), S32(st, 0x00784CD0u + 4u * p),
                         0x005CDB78u, 1, 0);
        }
        cb0 = U32(st, CLIMATE); /* @bin 0x004BBBEB */
    }
}

/* @bin 0x004BBC10 */
void newgame_points_5f_create(game_state *st)
{
    if (S32(st, EDITOR) != 0) {
        return;
    }
    for (uint32_t p = 0; p < 8; p++) {
        int16_t x = S16(st, 0x00784B74u + 2u * p);
        if (x <= 0) {
            continue;
        }
        random_next_state(st); /* @bin 0x004BBC33 */
        int32_t f = figure_create(st, 1, 0x5F, x, S16(st, 0x00784B84u + 2u * p), 0);
        U8(st, FIGURE_ADDR(f) + 1) = (uint8_t) (crt_rand(st) & 0x1F); /* @bin 0x004BBD14 */
    }
}

/* @bin 0x004BBD40 */
void newgame_points_41_create(game_state *st, int32_t x, int32_t y, int32_t flag)
{
    if (S32(st, EDITOR) == 0) {
        for (uint32_t p = 0; p < 8; p++) {
            int16_t px = S16(st, 0x00784B74u + 2u * p);
            if (px <= 0) {
                continue;
            }
            random_next_state(st); /* @bin 0x004BBD63 */
            int32_t f = figure_create(st, 1, 0x41, px, S16(st, 0x00784B84u + 2u * p), 0);
            uint32_t fa = FIGURE_ADDR(f);
            U8(st, fa + 1) = (uint8_t) (crt_rand(st) & 0x1F);    /* @bin 0x004BBE4A */
            U8(st, fa + 0x39) = (uint8_t) (crt_rand(st) & 7);    /* @bin 0x004BBE57 */
            /* @bin 0x004BBE5C the current figure (0x00D056F8, not the new one) walks from +0x4A/+0x4C
             * to 15 times +0x24/+0x26 */
            int32_t cur = S32(st, CURRENT_FIGURE);
            uint32_t ca = FIGURE_ADDR(cur);
            figure_line_to(st, cur, S16(st, ca + 0x4A), S16(st, ca + 0x4C), S16(st, ca + 0x24) * 15,
                           S16(st, ca + 0x26) * 15, 0);
        }
    }
    if (flag == 0 || (S32(st, EDITOR) == 0 && U8(st, 0x00784CB1u) == 0)) {
        return;
    }
    /* @bin 0x004BBEDD delete every figure of type 0x25 in use (ids 1..1999) */
    for (int32_t id = 1; id < FIGURE_COUNT; id++) {
        uint32_t fa = FIGURE_ADDR(id);
        if (U8(st, fa + 0x0E) != 0 && U8(st, fa + 0x0A) == 0x25) {
            figure_delete(st, id);
        }
    }
    /* @bin 0x004BBF37 20 new ones: +0x38 := 8, +0x0B and +0x36 from this table */
    static const uint8_t b0b[20] = {3, 1, 3, 2, 1, 3, 2, 3, 2, 1, 3, 3, 2, 3, 3, 3, 1, 2, 0, 1};
    static const uint16_t w36[20] = {10,  50,  100, 130, 200, 250, 400,  430,  500,  600,
                                     70,  750, 820, 830, 900, 980, 1010, 1030, 1200, 1300};
    for (int i = 0; i < 20; i++) {
        uint32_t fa = FIGURE_ADDR(figure_create(st, 1, 0x25, x, y, 0));
        U8(st, fa + 0x38) = 8;
        U8(st, fa + 0x0B) = b0b[i];
        U16(st, fa + 0x36) = w36[i];
    }
}

static int32_t run_herds(game_state *st, const int32_t *a)
{
    (void) a;
    newgame_herds_create(st);
    return 0;
}

static int32_t run_points_5f(game_state *st, const int32_t *a)
{
    (void) a;
    newgame_points_5f_create(st);
    return 0;
}

static int32_t run_points_41(game_state *st, const int32_t *a)
{
    newgame_points_41_create(st, a[0], a[1], a[2]);
    return 0;
}

const ported_function newgame_figures_ports[] = {
    {0x004BB3F0u, "newgame_herds_create", 0, 0, 0, run_herds, 0},
    {0x004BBC10u, "newgame_points_5f_create", 0, 0, 0, run_points_5f, 0},
    {0x004BBD40u, "newgame_points_41_create", 3, 0, 0, run_points_41, 0},
};
const int newgame_figures_ports_count = sizeof(newgame_figures_ports) / sizeof(newgame_figures_ports[0]);
