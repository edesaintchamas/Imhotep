/* Imhotep - player commands: legions (formations) and ships (package military).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * docs/commands.md §3.7. Formations: chunk 24, 0x90 bytes at 0x00BCB9C8 + id·0x90; figures:
 * chunk 21 (docs/formats/save.md §5). The window id 0x00E38DDA and the redraw flag 0x00E38DCA are
 * UI state the original writes; they are written here too (docs/commands.md §1.5). */
#include "command/military.h"

#include "building/building_spawn.h"
#include "building/day_road.h"
#include "city/day_buildings.h"
#include "city/day_city.h"
#include "city/month.h"
#include "city/rest_notice.h"
#include "figure/actions_a.h"
#include "figure/actions_c2.h"
#include "figure/formation.h"
#include "figure/helpers_x.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define CURRENT_FIGURE 0x00D056F8u
#define MAP_START 0x00BCA2D8u
#define TERRAIN_GRID 0x00AF598Cu   /* u32 per tile */
#define FIGURE_GRID 0x00B350DCu    /* i16 per tile: first figure on the tile */
#define BUILDING_GRID 0x00B4E6FCu  /* i16 per tile: building */
#define ROUTE_DIST 0x00EED760u     /* i32 per tile: distances of the last 0x00517A40 */
#define ROAD_X 0x00D3B4E4u         /* outputs of 0x004806C0 */
#define ROAD_Y 0x00D3B4E0u

#define GATE 0x00E38E60u           /* u8, always 0 in single player (docs/commands.md §1.1) */
#define WINDOW 0x00E38DDAu
#define REDRAW 0x00E38DCAu
#define PARAM1 0x00D3B198u
#define PARAM2 0x00D3B194u
#define CTX_FORMATION 0x00EADCCCu
#define CTX_TRANSPORT 0x00EADCC8u
#define CTX_WARSHIP 0x00EADCC4u
#define SEL_FORMATION 0x00BCB9C0u
#define SEL_TRANSPORT 0x00D05700u
#define SEL_WARSHIP 0x00D05704u
#define TRANSPORT_MODE 0x00D05708u
#define WARSHIP_MODE 0x00D0570Cu
#define MOUSE_OFFSET 0x00BCA2C4u
#define MOUSE_X 0x00BCA2C8u
#define MOUSE_Y 0x00BCA2CCu
#define CONSTRUCTION_TYPE 0x006059E8u
#define CURSOR 0x00E390DCu

/* @bin 0x00406863 the sounds of the legion orders (0x0047D350(path, 1, 0)) */
#define WAV_COHORT1 0x00576238
#define WAV_COHORT2 0x00576224
#define WAV_COHORT3 0x00576210
#define WAV_COHORT4 0x005761FC
#define WAV_COHORT5 0x0057624C

static void sound(game_state *st, int32_t path)
{
    GAME_CALL(st, 0x0047D350u, path, 1, 0);
}

static uint32_t mouse_offset(game_state *st)
{
    /* @bin 0x0040C469 map start + y·228 + x */
    return U32(st, MAP_START) + 228u * U32(st, MOUSE_Y) + U32(st, MOUSE_X);
}

static void popup(game_state *st, int32_t text)
{
    /* @bin 0x0040A42F confirmation popup 0x004264D0(text, 0x0051F780, 1) */
    GAME_CALL(st, 0x004264D0u, text, 0x0051F780, 1);
}

/* ---- helpers ------------------------------------------------------------------------------- */

int32_t tile_soldier_formation(game_state *st, int32_t offset)
{
    /* @bin 0x004B5520 the formation (+0x60) of the first figure of types 0x0B..0x0E on the tile
     * (chain +0x08), else 0 */
    int32_t id = S16(st, FIGURE_GRID + 2u * (uint32_t) offset);
    while (id != 0) {
        uint32_t f = FIGURE_ADDR(id);
        uint8_t type = U8(st, f + 0x0A);
        if (type >= 0x0B && type <= 0x0E) {
            return S16(st, f + 0x60);
        }
        id = S16(st, f + 0x08);
    }
    return 0;
}

static int32_t tile_figure_of_type(game_state *st, int32_t offset, uint8_t type)
{
    int32_t id = S16(st, FIGURE_GRID + 2u * (uint32_t) offset);
    while (id != 0) {
        uint32_t f = FIGURE_ADDR(id);
        if (U8(st, f + 0x0A) == type) {
            return id;
        }
        id = S16(st, f + 0x08);
    }
    return 0;
}

int32_t tile_transport(game_state *st, int32_t offset)
{
    /* @bin 0x004B5570 the first figure of type 0x4D on the tile, else 0 */
    return tile_figure_of_type(st, offset, 0x4D);
}

int32_t tile_warship(game_state *st, int32_t offset)
{
    /* @bin 0x004B55B0 the first figure of type 0x4E on the tile, else 0 */
    return tile_figure_of_type(st, offset, 0x4E);
}

int32_t tile_fort_formation(game_state *st, int32_t offset)
{
    /* @bin 0x004B55F0 the building on the tile, in use and of type 0x36 or 0x39: its +0x50
     * (formation), else 0 */
    int32_t b = S16(st, BUILDING_GRID + 2u * (uint32_t) offset);
    if (b <= 0) {
        return 0;
    }
    uint32_t ba = BUILDING_ADDR(b);
    if (U8(st, ba) != 1) {
        return 0;
    }
    uint16_t type = U16(st, ba + 0x10);
    if (type == 0x36 || type == 0x39) {
        return S16(st, ba + 0x50);
    }
    return 0;
}

int32_t tile_enemy_figure(game_state *st, int32_t offset)
{
    /* @bin 0x004B5640 the first figure on the tile that is not dying (+0x38 4) and is of types
     * 0x2B..0x39, 0x29 in action 0x0B, or 0x52, 0x53, 0x68, 0x66, 0x67, 0x54, 0x6D, 0x63, 0x64,
     * 0x65, 0x5D, 0x5C; else 0 */
    int32_t id = S16(st, FIGURE_GRID + 2u * (uint32_t) offset);
    if (id <= 0) {
        return 0;
    }
    do {
        uint32_t f = FIGURE_ADDR(id);
        uint8_t action = U8(st, f + 0x38);
        if (action != 4) {
            uint8_t t = U8(st, f + 0x0A);
            if ((t >= 0x2B && t <= 0x39) || (t == 0x29 && action == 0x0B) || t == 0x52 || t == 0x53 ||
                t == 0x68 || t == 0x66 || t == 0x67 || t == 0x54 || t == 0x6D || t == 0x63 || t == 0x64 ||
                t == 0x65 || t == 0x5D || t == 0x5C) {
                return id;
            }
        }
        id = S16(st, f + 0x08);
    } while (id > 0);
    return 0;
}

/* the formation's men that can take an order: not dying (nor fighting when `fighting`), the
 * formation's +0x73 clear and +0x66 at most 1, and (when `attack_check`) not attacking
 * (0x004B63D0); +0x66 1 costs morale (0x004B7B50(id, 10)) */
static int man_ready(game_state *st, uint32_t fa, int32_t id, int32_t fid, int fighting, int attack_check)
{
    uint32_t f = FIGURE_ADDR(fid);
    uint8_t action = U8(st, f + 0x38);
    if (action == 4 || (fighting && action == 5)) {
        return 0;
    }
    if (U8(st, fa + 0x73) != 0 || U8(st, fa + 0x66) > 1) {
        return 0;
    }
    if (attack_check && (uint8_t) figure_in_attack(st, fid) != 0) {
        return 0;
    }
    if (U8(st, fa + 0x66) == 1) {
        formation_add_0x2c(st, id, 10);
    }
    return 1;
}

/* @bin 0x0040677D, 0x00406FFB a formation of type 0x0D whose first man is in action 0x13 (and
 * 0x00D3B198 not 1): its men (+0x08, up to the first 0) go to action 0x0C, from where they stand
 * (+0x13C, +0x140); +0x00, +0x134 and the current figure's +0x01 cleared */
static void chariots_recall(game_state *st, int32_t id, uint32_t fa, int32_t param)
{
    if (S16(st, fa + 4) != 0x0D || U8(st, FIGURE_ADDR(S16(st, fa + 8)) + 0x38) != 0x13 || param == 1) {
        return;
    }
    for (int32_t i = 0; i < 16; i++) {
        int32_t fid = S16(st, fa + 8 + 2u * (uint32_t) i);
        if (fid == 0) {
            return;
        }
        if (!man_ready(st, fa, id, fid, 0, 0)) {
            continue;
        }
        uint32_t f = FIGURE_ADDR(fid);
        U8(st, f + 0x00) = 0;
        S32(st, f + 0x134) = 0;
        S32(st, f + 0x13C) = S16(st, f + 0x14);
        S32(st, f + 0x140) = S16(st, f + 0x16);
        U8(st, f + 0x38) = 0x0C;
        U8(st, FIGURE_ADDR(S32(st, CURRENT_FIGURE)) + 0x01) = 0;
    }
}

int32_t formation_attack(game_state *st, int32_t x, int32_t y, int32_t id, int32_t target)
{
    /* @bin 0x004B66E0 order formation `id` against `target` (a formation, -1: none) at (x, y):
     * distances from the formation's position (0x00517A40); the first of the 9 tiles around (x, y)
     * (x-1..x+1, then y-1..y+1) its leader (+0x40) can reach (0x00518400; none: warning 0xD3);
     * that tile reached by the distances, not its own position, +0x64 clear; then +0x32/+0x34
     * := its position, +0x03 0, +0x8C target; warning 0x33 under 21 men (+0x2C); type 0x0D in
     * action 0x13 sends its men to action 0x15; every man ready goes to action 0x15 (+0x82 0) */
    static const int8_t dx[9] = {-1, -1, -1, 0, 0, 0, 1, 1, 1};
    static const int8_t dy[9] = {-1, 0, 1, -1, 0, 1, -1, 0, 1};
    uint32_t fa = FORMATION_ADDR(id);
    int32_t leader = S16(st, fa + 0x40);
    if (U8(st, GATE) != 0) {
        return 0;
    }
    route_distances_to(st, S16(st, fa + 0x2E), S16(st, fa + 0x30), -2, -2);
    uint32_t lf = FIGURE_ADDR(leader);
    int32_t k = 0, r = 0;
    for (; k < 9; k++) {
        r = route_even_to(st, S16(st, lf + 0x14), S16(st, lf + 0x16), x + dx[k], y + dy[k]);
        if (r != 0) {
            break;
        }
    }
    if (r == 0) {
        notice_add_4e2970(st, 0xD3);
        return 0;
    }
    int32_t tx = x + dx[k], ty = y + dy[k];
    if (S32(st, ROUTE_DIST + 4u * (U32(st, MAP_START) + 228u * (uint32_t) ty + (uint32_t) tx)) <= 0) {
        return 0;
    }
    int16_t fx = S16(st, fa + 0x2E);
    if (tx == fx && ty == S16(st, fa + 0x30)) {
        return 0;
    }
    if (S16(st, fa + 0x64) != 0) {
        return 0;
    }
    /* @bin 0x004B6856 the destination is the formation's own position */
    S16(st, fa + 0x32) = fx;
    S16(st, fa + 0x34) = S16(st, fa + 0x30);
    U8(st, fa + 0x03) = 0;
    S32(st, fa + 0x8C) = target;
    if (S16(st, fa + 0x2C) <= 0x14) {
        notice_add_4e2970(st, 0x33);
    }
    if (S16(st, fa + 4) == 0x0D && U8(st, FIGURE_ADDR(S16(st, fa + 8)) + 0x38) == 0x13 && S32(st, PARAM1) != 1) {
        for (int32_t i = 0; i < 16; i++) {
            int32_t fid = S16(st, fa + 8 + 2u * (uint32_t) i);
            if (fid == 0) {
                break;
            }
            if (!man_ready(st, fa, id, fid, 0, 1)) {
                continue;
            }
            uint32_t f = FIGURE_ADDR(fid);
            U8(st, f + 0x00) = 0;
            S32(st, f + 0x134) = 0;
            S32(st, f + 0x13C) = S16(st, f + 0x14);
            S32(st, f + 0x140) = S16(st, f + 0x16);
            U8(st, f + 0x38) = 0x15;
            S16(st, f + 0x82) = 0;
            U8(st, FIGURE_ADDR(S32(st, CURRENT_FIGURE)) + 0x01) = 0;
        }
    }
    for (int32_t i = 0; i < 16; i++) {
        int32_t fid = S16(st, fa + 8 + 2u * (uint32_t) i);
        if (fid == 0) {
            break;
        }
        if (!man_ready(st, fa, id, fid, 1, 1)) {
            continue;
        }
        uint32_t f = FIGURE_ADDR(fid);
        U8(st, f + 0x00) = 0;
        U8(st, f + 0x38) = 0x15;
        S16(st, f + 0x82) = 0;
        figure_route_release(st, fid);
    }
    return 1;
}

int32_t formation_charge_to(game_state *st, int32_t x, int32_t y, int32_t id)
{
    /* @bin 0x004B6A40 order formation `id` (layout 0) to (x, y): as 0x004B6400 from its leader
     * (+0x40), then +0x7E 1, +0x80 0, +0x2A 0 and every man ready (+0x00, +0x134 cleared, +0x13C,
     * +0x140 its tile) goes to action 0x13 */
    uint32_t fa = FORMATION_ADDR(id);
    int32_t leader = S16(st, fa + 0x40);
    if (U8(st, GATE) != 0) {
        return 0;
    }
    route_distances_to(st, S16(st, fa + 0x2E), S16(st, fa + 0x30), -2, -2);
    uint32_t lf = FIGURE_ADDR(leader);
    if (route_even_to(st, S16(st, lf + 0x14), S16(st, lf + 0x16), x, y) == 0) {
        notice_add_4e2970(st, 0xD3);
        return 0;
    }
    if (S32(st, ROUTE_DIST + 4u * (U32(st, MAP_START) + 228u * (uint32_t) y + (uint32_t) x)) <= 0) {
        return 0;
    }
    if (x == S16(st, fa + 0x2E) && y == S16(st, fa + 0x30)) {
        return 0;
    }
    if (S16(st, fa + 0x64) != 0) {
        return 0;
    }
    S16(st, fa + 0x32) = (int16_t) x;
    S16(st, fa + 0x34) = (int16_t) y;
    S16(st, fa + 0x7E) = 1;
    S16(st, fa + 0x80) = 0;
    U8(st, fa + 0x03) = 0;
    if (S16(st, fa + 0x2C) <= 0x14) {
        notice_add_4e2970(st, 0x33);
    }
    S16(st, fa + 0x2A) = 0;
    for (int32_t i = 0; i < 16; i++) {
        int32_t fid = S16(st, fa + 8 + 2u * (uint32_t) i);
        if (fid == 0) {
            break;
        }
        if (!man_ready(st, fa, id, fid, 1, 1)) {
            continue;
        }
        uint32_t f = FIGURE_ADDR(fid);
        U8(st, f + 0x00) = 0;
        S32(st, f + 0x134) = 0;
        S32(st, f + 0x13C) = S16(st, f + 0x14);
        S32(st, f + 0x140) = S16(st, f + 0x16);
        U8(st, f + 0x38) = 0x13;
        figure_route_release(st, fid);
    }
    return 1;
}

void formation_return(game_state *st, int32_t id, int32_t action)
{
    /* @bin 0x004B6C20 formation `id` home: with a transport (+0x84) and `action` not 0, the
     * transport takes `action` and sails towards the formation's position (action -1) or its fort
     * (+0x36, +0x38), destination from 0x0040C180; else as 0x004B6400 from the leader (+0x40) to
     * +0x36/+0x38, +0x03 := 1 and every man ready goes to action 9 */
    uint32_t fa = FORMATION_ADDR(id);
    int32_t leader = S16(st, fa + 0x40);
    int32_t ship = S32(st, fa + 0x84);
    if (action != 0 && ship != 0) {
        uint32_t f = FIGURE_ADDR(ship);
        U8(st, f + 0x38) = (uint8_t) action;
        int32_t tx, ty;
        if (action == -1) {
            tx = S16(st, fa + 0x2E);
            ty = S16(st, fa + 0x30);
        } else {
            tx = S16(st, fa + 0x36);
            ty = S16(st, fa + 0x38);
        }
        /* the outputs are the original's argument slots (id, action) */
        int32_t px = id, py = action;
        line_search_water(st, S16(st, f + 0x14), S16(st, f + 0x16), tx, ty, &px, &py);
        S16(st, f + 0x24) = (int16_t) px;
        S16(st, f + 0x26) = (int16_t) py;
        S16(st, f + 0x30) = (int16_t) tx;
        S16(st, f + 0x32) = (int16_t) ty;
        figure_route_release(st, ship);
        return;
    }
    route_distances_to(st, S16(st, fa + 0x2E), S16(st, fa + 0x30), -2, -2);
    int32_t hx = S16(st, fa + 0x36), hy = S16(st, fa + 0x38);
    uint32_t tile = U32(st, MAP_START) + 228u * (uint32_t) hy + (uint32_t) hx;
    uint32_t lf = FIGURE_ADDR(leader);
    if (route_even_to(st, S16(st, lf + 0x14), S16(st, lf + 0x16), hx, hy) == 0) {
        notice_add_4e2970(st, 0xD3);
        return;
    }
    if (S32(st, ROUTE_DIST + 4u * tile) <= 0 || S16(st, fa + 0x64) != 0) {
        return;
    }
    U8(st, fa + 0x03) = 1;
    for (int32_t i = 0; i < 16; i++) {
        int32_t fid = S16(st, fa + 8 + 2u * (uint32_t) i);
        if (fid == 0) {
            return;
        }
        if (!man_ready(st, fa, id, fid, 1, 1)) {
            continue;
        }
        U8(st, FIGURE_ADDR(fid) + 0x38) = 9;
        figure_route_release(st, fid);
    }
}

int32_t player_legion_by_row(game_state *st, int32_t row)
{
    /* @bin 0x004B78B0 formations 1..49 in use (+0 == 1) with +0x42 set: the row-th (from 1) */
    int32_t n = 1;
    for (int32_t id = 1; id < 50; id++) {
        uint32_t fa = FORMATION_ADDR(id);
        if (U8(st, fa) == 1 && U8(st, fa + 0x42) != 0) {
            if (n == row) {
                return id;
            }
            n++;
        }
    }
    return 0;
}

int32_t player_warship_by_row(game_state *st, int32_t row)
{
    /* @bin 0x004BC620 figures 1..1999 in use (+0x0E), of type 0x4E and owned (+0x0F) by the
     * current player: the row-th (from 1) */
    int32_t n = 1;
    uint32_t player = U32(st, CURRENT_PLAYER_ADDR);
    for (int32_t id = 1; id < FIGURE_COUNT; id++) {
        uint32_t f = FIGURE_ADDR(id);
        if (U8(st, f + 0x0E) != 0 && U8(st, f + 0x0A) == 0x4E && U8(st, f + 0x0F) == player) {
            if (n == row) {
                return id;
            }
            n++;
        }
    }
    return 0;
}

/* @bin 0x00406F00, 0x004BC68E the ship sails to the road tile of its wharf (+0x5A, 0x004806C0
 * outputs 0x00D3B4E4 / 0x00D3B4E0, which are put back); +0x2C/+0x2E its tile. The route released
 * (0x004B3250) is the current figure's (0x00D056F8), not the ship's. */
static void ship_home(game_state *st, uint32_t f, int32_t mode_reset)
{
    int32_t save_y = S32(st, ROAD_Y), save_x = S32(st, ROAD_X);
    uint32_t ba = BUILDING_ADDR(S16(st, f + 0x5A));
    if (mode_reset) {
        S32(st, TRANSPORT_MODE) = 0;
    }
    building_find_road_8(st, S16(st, ba + 6), S16(st, ba + 8), U8(st, ba + 3));
    S16(st, f + 0x2C) = S16(st, f + 0x14);
    S16(st, f + 0x2E) = S16(st, f + 0x16);
    S16(st, f + 0x24) = S16(st, ROAD_X);
    S16(st, f + 0x26) = S16(st, ROAD_Y);
    S32(st, ROAD_X) = save_x;
    S32(st, ROAD_Y) = save_y;
    figure_route_release(st, S32(st, CURRENT_FIGURE));
}

void ship_return_to_wharf(game_state *st, int32_t id)
{
    /* @bin 0x004BC670 ship `id` to action 0x0B, home */
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x38) = 0x0B;
    ship_home(st, f, 0);
}

void ships_count(game_state *st)
{
    /* @bin 0x004B8AA0 the current player's ships (figures 1..1999 in use, +0x0F the player):
     * warships (0x4E) CITY+0x4668, those with +0x02 set CITY+0x4669, transports (0x4D) +0x4664 */
    uint32_t player = U32(st, CURRENT_PLAYER_ADDR);
    uint32_t city = CITY_ADDR(player);
    U8(st, city + 0x4669) = 0;
    U8(st, city + 0x4668) = 0;
    U8(st, city + 0x4664) = 0;
    for (int32_t id = 1; id < FIGURE_COUNT; id++) {
        uint32_t f = FIGURE_ADDR(id);
        if (U8(st, f + 0x0E) == 0 || U8(st, f + 0x0F) != player) {
            continue;
        }
        uint8_t type = U8(st, f + 0x0A);
        if (type == 0x4E) {
            U8(st, city + 0x4668) += 1;
            if (U8(st, f + 0x02) != 0) {
                U8(st, city + 0x4669) += 1;
            }
        } else if (type == 0x4D) {
            U8(st, city + 0x4664) += 1;
        }
    }
}

void military_requests(game_state *st, int32_t *first_other, int32_t *first_kind2, int32_t *n_other,
                       int32_t *n_kind2)
{
    /* @bin 0x00521AA0 the running events (0x0044BAF0, 0x0044D4C0) of type 0x25 whose +0x1C is in
     * 0..49: by the kind of record +0x1C of 0x007770F0 (0x144 bytes; byte +0): kind 2 or other,
     * the first one's index (+0) and the count */
    *first_other = -1;
    *first_kind2 = -1;
    *n_other = 0;
    *n_kind2 = 0;
    for (uint32_t ev = events_next_active(st, EVENTS, 0); ev != 0; ev = events_next_active(st, EVENTS, ev)) {
        if ((uint8_t) event_is_running(st, ev) == 0 || P16(st, ev + 4) != 0x25) {
            continue;
        }
        int32_t k = P16(st, ev + 0x1C);
        if (k < 0 || k >= 0x32) {
            continue;
        }
        if (U8(st, 0x007770F0u + 0x144u * (uint32_t) k) == 2) {
            if (*first_kind2 < 0) {
                *first_kind2 = P16(st, ev);
            }
            *n_kind2 += 1;
        } else {
            if (*first_other < 0) {
                *first_other = P16(st, ev);
            }
            *n_other += 1;
        }
    }
}

void military_requests_open(game_state *st, uint8_t *other, uint8_t *kind2)
{
    /* @bin 0x00521A60 whether there are requests of each kind (0x00521AA0) */
    int32_t a, b, n_other, n_kind2;
    military_requests(st, &a, &b, &n_other, &n_kind2);
    *other = n_other > 0;
    *kind2 = n_kind2 > 0;
}

/* ---- legion commands ------------------------------------------------------------------------ */

void legion_order(game_state *st)
{
    /* @bin 0x00406720 the legion window's layout buttons (table 0x005C66D8, p1 0..3) on formation
     * 0x00EADCCC, a player's (+0x82 < 0) */
    int32_t id = S32(st, CTX_FORMATION);
    uint32_t fa = FORMATION_ADDR(id);
    if (S16(st, fa + 0x82) >= 0 || U8(st, GATE) != 0) {
        return;
    }
    int32_t p1 = S32(st, PARAM1);
    if (p1 == 3) {
        int16_t layout = S16(st, fa + 0x2A);
        if (layout != 6) {
            S16(st, fa + 0x62) = layout;
        }
    }
    chariots_recall(st, id, fa, p1);
    p1 = S32(st, PARAM1);
    /* @bin 0x00406863 jump table 0x004069C4 */
    switch (p1) {
    case 0:
        S16(st, fa + 0x2A) = U8(st, fa + 0x77) != 0 ? 2 : 1;
        sound(st, WAV_COHORT1);
        break;
    case 1:
        if (S16(st, fa + 4) == 0x0D) {
            if (S16(st, fa + 0x7E) == 0) {
                int16_t layout = S16(st, fa + 0x2A);
                if (layout != 0) {
                    S16(st, fa + 0x7C) = layout;
                }
                S16(st, fa + 0x2A) = 0;
            }
        } else {
            S16(st, fa + 0x2A) = U8(st, fa + 0x77) != 0 ? 4 : 3;
        }
        sound(st, WAV_COHORT2);
        break;
    case 2:
        S16(st, fa + 0x2A) = 0x0F;
        sound(st, WAV_COHORT3);
        break;
    case 3:
        S16(st, fa + 0x2A) = 6;
        sound(st, WAV_COHORT4);
        break;
    default:
        break;
    }
    S32(st, SEL_FORMATION) = id;
    int16_t layout = S16(st, fa + 0x2A);
    if (layout == 6) {
        S32(st, fa + 0x8C) = -1;
        U8(st, WINDOW) = 1;
        U8(st, REDRAW) = 1;
        return;
    }
    if (layout == 0) {
        S32(st, fa + 0x8C) = -1;
        U8(st, WINDOW) = 0x49;
        U8(st, REDRAW) = 1;
        return;
    }
    if (S16(st, fa + 0x7E) != 0) {
        U8(st, WINDOW) = 9;
        if (S32(st, PARAM1) != 1) {
            U8(st, WINDOW) = 0x1E;
        }
    } else {
        U8(st, WINDOW) = 0x1E;
    }
    U8(st, REDRAW) = 1;
    S32(st, fa + 0x8C) = -1;
}

void legion_toggle_0x77(game_state *st)
{
    /* @bin 0x00407110 (image button 0x005C6768) formation +0x77 := 1 - +0x77; p1 from the layout
     * (table 0x004071AC: 1, 2 -> 0; 3, 4 -> 1; 0x0F -> 2); p2 1 applies 0x00406720 to the
     * selected formation 0x00BCB9C0 */
    int32_t sel = S32(st, SEL_FORMATION);
    int32_t ctx = S32(st, CTX_FORMATION);
    int32_t id = (sel != 0 && sel == ctx) ? sel : ctx;
    if (U8(st, GATE) != 0) {
        return;
    }
    uint32_t fa = FORMATION_ADDR(id);
    U8(st, fa + 0x77) = (uint8_t) (1 - U8(st, fa + 0x77));
    switch (S16(st, fa + 0x2A)) {
    case 1:
    case 2:
        S32(st, PARAM1) = 0;
        break;
    case 3:
    case 4:
        S32(st, PARAM1) = 1;
        break;
    case 0x0F:
        S32(st, PARAM1) = 2;
        break;
    default:
        break;
    }
    if (S32(st, PARAM2) == 1) {
        S32(st, CTX_FORMATION) = sel;
        legion_order(st);
    }
}

void legion_return_to_fort(game_state *st)
{
    /* @bin 0x00406FB0 (table 0x005C66D8 entry 4) formation 0x00EADCCC, a player's, not at home
     * (+0x03 1): home (0x004B6C20(id, 0x0E)) */
    int32_t id = S32(st, CTX_FORMATION);
    uint32_t fa = FORMATION_ADDR(id);
    if (S16(st, fa + 0x82) >= 0 || U8(st, fa + 0x03) == 1 || U8(st, GATE) != 0) {
        return;
    }
    chariots_recall(st, id, fa, S32(st, PARAM1));
    formation_return(st, id, 0x0E);
    sound(st, WAV_COHORT5);
    U8(st, WINDOW) = 1;
    U8(st, REDRAW) = 1;
}

void legion_window_close(game_state *st)
{
    /* @bin 0x004071C0 the legion window (0x00EADD50 4) closes: 0x00406720 again with p1 from the
     * selected formation's layout (table 0x00407278) */
    if (S32(st, 0x00EADCC0u) != 0) {
        S32(st, 0x00EADCC0u) = 0;
        U8(st, REDRAW) = 1;
        return;
    }
    if (S32(st, 0x00EADD50u) == 4) {
        if (U8(st, GATE) != 0) {
            return;
        }
        switch (S16(st, FORMATION_ADDR(S32(st, SEL_FORMATION)) + 0x2A)) {
        case 1:
        case 2:
            S32(st, PARAM1) = 0;
            legion_order(st);
            U8(st, WINDOW) = 1;
            U8(st, REDRAW) = 1;
            return;
        case 3:
        case 4:
            S32(st, PARAM1) = 1;
            legion_order(st);
            U8(st, WINDOW) = 1;
            U8(st, REDRAW) = 1;
            return;
        case 0x0F:
            S32(st, PARAM1) = 2;
            break;
        default:
            break;
        }
        legion_order(st);
    }
    U8(st, WINDOW) = 1;
    U8(st, REDRAW) = 1;
}

void legion_click_target(game_state *st)
{
    /* @bin 0x0040C440 window 0x1E, left click on the tile under the mouse with formation
     * 0x00BCB9C0 (a player's, +0x64 clear): its own fort -> home (0x004B6C20(id, 0x0E)); a figure
     * to fight (0x004B5640) -> attack (its formation +0x60, or -1 for boats); else move
     * (0x004B6400) */
    if (U8(st, GATE) != 0) {
        return;
    }
    if (S32(st, MOUSE_OFFSET) == 0) {
        U8(st, WINDOW) = 1;
        U8(st, REDRAW) = 1;
        return;
    }
    int32_t tile = (int32_t) mouse_offset(st);
    uint32_t fa = FORMATION_ADDR(S32(st, SEL_FORMATION));
    if (S16(st, fa + 0x82) >= 0 || S16(st, fa + 0x64) != 0) {
        return;
    }
    int32_t fort = tile_fort_formation(st, tile);
    int32_t enemy = tile_enemy_figure(st, tile);
    int32_t id = S32(st, SEL_FORMATION);
    if (fort != 0 && fort == id) {
        formation_return(st, id, 0x0E);
        U8(st, WINDOW) = 1;
        U8(st, REDRAW) = 1;
        return;
    }
    uint32_t ef = FIGURE_ADDR(enemy);
    uint8_t type = U8(st, ef + 0x0A);
    int32_t target = -1;
    if (type != 0x5C && type != 0x65 && type != 0x5D && type != 0x64) {
        target = S16(st, ef + 0x60);
    }
    if (enemy != 0) {
        formation_attack(st, S32(st, MOUSE_X), S32(st, MOUSE_Y), id, target);
    } else {
        formation_move_to(st, S32(st, MOUSE_X), S32(st, MOUSE_Y), id, 0);
    }
    sound(st, WAV_COHORT5);
    U8(st, WINDOW) = 1;
    U8(st, REDRAW) = 1;
}

void legion_click_target_0x49(game_state *st)
{
    /* @bin 0x0040C570 window 0x49 (layout 0), left click: own fort -> home; else 0x004B6A40 */
    if (U8(st, GATE) != 0) {
        return;
    }
    if (S32(st, MOUSE_OFFSET) != 0) {
        int32_t tile = (int32_t) mouse_offset(st);
        uint32_t fa = FORMATION_ADDR(S32(st, SEL_FORMATION));
        if (S16(st, fa + 0x82) >= 0 || S16(st, fa + 0x64) != 0) {
            return;
        }
        int32_t fort = tile_fort_formation(st, tile);
        int32_t id = S32(st, SEL_FORMATION);
        if (fort != 0 && fort == id) {
            formation_return(st, id, 0x0E);
            U8(st, WINDOW) = 1;
            U8(st, REDRAW) = 1;
            return;
        }
        formation_charge_to(st, S32(st, MOUSE_X), S32(st, MOUSE_Y), id);
        sound(st, WAV_COHORT5);
    }
    U8(st, WINDOW) = 1;
    U8(st, REDRAW) = 1;
}

void advisor_legion_return(game_state *st)
{
    /* @bin 0x0040A350 military advisor, return button of row p1 */
    int32_t id = player_legion_by_row(st, S32(st, PARAM1));
    if (id == 0 || S16(st, FORMATION_ADDR(id) + 0x82) >= 0) {
        return;
    }
    formation_return(st, id, 0x0E);
    U8(st, REDRAW) = 1;
}

void advisor_legion_service(game_state *st)
{
    /* @bin 0x0040A390 military advisor, kingdom service of row p1: formation +0x67 toggled when a
     * request allows it (0x00521A60; kind 2 for a formation away, 0x0040B5E0), else a popup (0x24,
     * 0x26, 0x28) and +0x67 := 0; formations rebuilt (0x004B7C80) */
    int32_t id = player_legion_by_row(st, S32(st, PARAM1));
    if (id == 0) {
        return;
    }
    uint8_t other, kind2;
    military_requests_open(st, &other, &kind2);
    int away = formation_has_action_0x12(st, id) != 0;
    uint32_t fa = FORMATION_ADDR(id);
    if (S16(st, fa + 0x82) >= 0) {
        return;
    }
    int toggle = away ? kind2 != 0 : other != 0;
    if (toggle) {
        U8(st, fa + 0x67) = U8(st, fa + 0x67) == 0;
        formations_rebuild(st);
        U8(st, REDRAW) = 1;
        return;
    }
    if (!away && kind2 != 0) {
        popup(st, 0x24);
    } else {
        popup(st, other != 0 ? 0x26 : 0x28);
    }
    U8(st, fa + 0x67) = 0;
    formations_rebuild(st);
    U8(st, REDRAW) = 1;
}

/* ---- ship commands -------------------------------------------------------------------------- */

/* @bin 0x00406B7C the ship's hold +0x128 of +0x12C, in percent (0x004CDE60) */
static int32_t ship_percent(game_state *st, uint32_t f)
{
    return calc_ratio_percent(S32(st, f + 0x128), S32(st, f + 0x12C));
}

void transport_order(game_state *st)
{
    /* @bin 0x00406B40 transport window (0x00EADCC8, type 0x4D), table 0x005C6788, p1 0..3 (jump
     * table 0x00406C7C) */
    int32_t id = S32(st, CTX_TRANSPORT);
    S32(st, SEL_TRANSPORT) = id;
    if (id <= 0 || U8(st, GATE) != 0) {
        return;
    }
    uint32_t f = FIGURE_ADDR(id);
    int32_t pct = ship_percent(st, f);
    uint8_t action = U8(st, f + 0x38);
    if ((action == 0x0B || action == 0x0F) && pct <= 0x0F) {
        return;
    }
    int32_t p1 = S32(st, PARAM1);
    if (p1 == 3 && (action == 0x0B || action == 0x0F || pct == 100)) {
        return;
    }
    switch (p1) {
    case 0:
        U8(st, f + 0x38) = 0x14;
        S32(st, TRANSPORT_MODE) = 1;
        U8(st, WINDOW) = 0x1F;
        U8(st, REDRAW) = 1;
        break;
    case 1:
        U8(st, f + 0x38) = 0x0A;
        S16(st, f + 0x36) = 0x4B;
        S32(st, f + 0x15C) = 0x10;
        S32(st, TRANSPORT_MODE) = 0;
        U8(st, WINDOW) = 1;
        U8(st, REDRAW) = 1;
        break;
    case 2:
        U8(st, f + 0x38) = (uint8_t) (0x0C + (U8(st, f + 0x130) != 0));
        S32(st, TRANSPORT_MODE) = 1;
        U8(st, WINDOW) = 0x1F;
        U8(st, REDRAW) = 1;
        break;
    case 3:
        S32(st, TRANSPORT_MODE) = 0;
        boat_seek_dock(st, id);
        U8(st, WINDOW) = 0x1F;
        U8(st, REDRAW) = 1;
        break;
    default:
        break;
    }
}

void transport_return(game_state *st)
{
    /* @bin 0x00406E80 transport window, entry 4: not at its dock (0x004A4300) -> action 9, home */
    int32_t id = S32(st, CTX_TRANSPORT);
    S32(st, SEL_TRANSPORT) = id;
    if (id <= 0) {
        return;
    }
    uint32_t f = FIGURE_ADDR(id);
    int32_t pct = ship_percent(st, f);
    uint8_t action = U8(st, f + 0x38);
    if ((action == 0x0B || action == 0x0F) && pct <= 0x0F) {
        return;
    }
    if (S32(st, PARAM1) == 4) {
        if ((uint8_t) boat_at_dock(st, id) != 0) {
            return;
        }
        if (S32(st, PARAM1) == 4) {
            U8(st, f + 0x38) = 9;
            ship_home(st, f, 1);
        }
    }
    U8(st, WINDOW) = 1;
    U8(st, REDRAW) = 1;
}

void warship_order(game_state *st)
{
    /* @bin 0x00406C90 warship window (0x00EADCC4, type 0x4E, a player's: +0x132 < 0), table
     * 0x005C6818, p1 0..3 (jump table 0x00406DCC) */
    if (U8(st, GATE) != 0) {
        return;
    }
    int32_t id = S32(st, CTX_WARSHIP);
    S32(st, SEL_WARSHIP) = id;
    if (id <= 0) {
        return;
    }
    uint32_t f = FIGURE_ADDR(id);
    int32_t pct = ship_percent(st, f);
    uint8_t action = U8(st, f + 0x38);
    if ((action == 0x0D || action == 0x0F) && pct <= 0x0F) {
        return;
    }
    int32_t p1 = S32(st, PARAM1);
    if (p1 == 3 && (action == 0x0D || action == 0x0F || pct == 100)) {
        return;
    }
    if (S16(st, f + 0x132) >= 0) {
        return;
    }
    switch (p1) {
    case 0:
        U8(st, f + 0x38) = 0x14;
        S32(st, WARSHIP_MODE) = 1;
        U8(st, WINDOW) = 0x20;
        U8(st, REDRAW) = 1;
        break;
    case 1:
        U8(st, f + 0x38) = 0x17;
        S32(st, f + 0x15C) = 0x16;
        S32(st, WARSHIP_MODE) = 1;
        U8(st, WINDOW) = 0x20;
        U8(st, REDRAW) = 1;
        break;
    case 2:
        U8(st, f + 0x38) = 0x0A;
        S32(st, f + 0x15C) = 0x10;
        S32(st, WARSHIP_MODE) = 0;
        U8(st, WINDOW) = 1;
        U8(st, REDRAW) = 1;
        break;
    case 3:
        S32(st, WARSHIP_MODE) = 0;
        boat_seek_dock(st, id);
        U8(st, WINDOW) = 0x20;
        U8(st, REDRAW) = 1;
        break;
    default:
        break;
    }
}

void warship_return(game_state *st)
{
    /* @bin 0x00406DE0 warship window, entry 4: not at its dock -> 0x004BC670 */
    int32_t id = S32(st, CTX_WARSHIP);
    if (id <= 0 || U8(st, GATE) != 0) {
        return;
    }
    uint32_t f = FIGURE_ADDR(id);
    int32_t pct = ship_percent(st, f);
    uint8_t action = U8(st, f + 0x38);
    if ((action == 0x0D || action == 0x0F) && pct <= 0x0F) {
        return;
    }
    if (S32(st, PARAM1) == 4 && (uint8_t) boat_at_dock(st, id) != 0) {
        return;
    }
    if (S16(st, f + 0x132) >= 0) {
        return;
    }
    if (S32(st, PARAM1) == 4) {
        S32(st, WARSHIP_MODE) = 0;
        ship_return_to_wharf(st, id);
    }
    U8(st, WINDOW) = 1;
    U8(st, REDRAW) = 1;
}

/* @bin 0x0040BACE the ship's destination: the water tile nearest the mouse tile on the line from
 * the ship (0x0040C180) */
static void ship_aim(game_state *st, uint32_t f, int32_t *px, int32_t *py)
{
    line_search_water(st, S16(st, f + 0x14), S16(st, f + 0x16), S32(st, MOUSE_X), S32(st, MOUSE_Y), px, py);
}

void transport_click_target(game_state *st)
{
    /* @bin 0x0040BA40 window 0x1F, left click with transport 0x00D05700 (not in action 8): in
     * mode 0x00D05708 the cursor 0x00E390DC picks action 0x0C (= 0x00E390C8, board) or 0x0D
     * (= 0x00E390C4, unload); then by action (jump table 0x0040BD64) */
    if (U8(st, GATE) != 0) {
        return;
    }
    int32_t id = S32(st, SEL_TRANSPORT);
    uint32_t f = FIGURE_ADDR(id);
    if (id <= 0 || U8(st, f + 0x38) == 8 || S32(st, MOUSE_OFFSET) == 0) {
        U8(st, WINDOW) = 1;
        U8(st, REDRAW) = 1;
        return;
    }
    if (S32(st, TRANSPORT_MODE) != 0) {
        int32_t cursor = S32(st, CURSOR);
        if (cursor == S32(st, 0x00E390C8u)) {
            U8(st, f + 0x38) = 0x0C;
        }
        if (cursor == S32(st, 0x00E390C4u)) {
            U8(st, f + 0x38) = 0x0D;
        }
    }
    int32_t px, py;
    switch (U8(st, f + 0x38)) {
    case 0x09:
    case 0x0B:
    case 0x14:
        ship_aim(st, f, &px, &py);
        S16(st, f + 0x24) = (int16_t) px;
        S16(st, f + 0x26) = (int16_t) py;
        figure_route_release(st, id);
        U8(st, REDRAW) = 1;
        return;
    case 0x0A:
        ship_aim(st, f, &px, &py);
        S16(st, f + 0x24) = (int16_t) px;
        S16(st, f + 0x26) = (int16_t) py;
        U8(st, f + 0x11) = (uint8_t) direction_to_fine(st, S16(st, f + 0x14), S16(st, f + 0x16), px, py);
        S32(st, f + 0x15C) = 9;
        figure_route_release(st, id);
        break;
    case 0x0C: {
        /* @bin 0x0040BBB7 board: the legion standing on the tile (0x004B5520) walks to the shore
         * point nearest the ship's destination (0x0040BDA0) and, when it can (0x004B6400), the
         * ship takes it (+0x92) */
        ship_aim(st, f, &px, &py);
        int32_t legion = tile_soldier_formation(st, (int32_t) mouse_offset(st));
        if (legion != 0) {
            int32_t lx = 0, ly = 0;
            line_search_land(st, S32(st, MOUSE_X), S32(st, MOUSE_Y), px, py, &lx, &ly);
            if (formation_move_to(st, lx, ly, legion, 0) != 0) {
                U8(st, f + 0x92) = (uint8_t) legion;
                S16(st, f + 0x24) = (int16_t) px;
                S16(st, f + 0x26) = (int16_t) py;
                figure_route_release(st, id);
            }
        } else {
            S16(st, f + 0x24) = (int16_t) px;
            S16(st, f + 0x26) = (int16_t) py;
        }
        break;
    }
    case 0x0D:
        /* @bin 0x0040BCAE unload (only with men on board, +0x130) on a land tile: +0x30/+0x32 the
         * mouse tile */
        if (U8(st, f + 0x130) == 0) {
            return;
        }
        if ((U32(st, TERRAIN_GRID + 4u * mouse_offset(st)) & 4) != 4) {
            ship_aim(st, f, &px, &py);
            S16(st, f + 0x24) = (int16_t) px;
            S16(st, f + 0x26) = (int16_t) py;
            S16(st, f + 0x30) = (int16_t) S32(st, MOUSE_X);
            S16(st, f + 0x32) = (int16_t) S32(st, MOUSE_Y);
            figure_route_release(st, id);
        }
        break;
    default:
        return;
    }
    U8(st, WINDOW) = 1;
    U8(st, REDRAW) = 1;
    S32(st, TRANSPORT_MODE) = 0;
}

void warship_click_target(game_state *st)
{
    /* @bin 0x0040B830 window 0x20, left click with warship 0x00D05704 (a player's, not in action
     * 8): a figure to fight on the tile (0x004B5640) -> action 0x18 against it (+0x10 the action
     * before); else by action (table 0x0040BA2C) */
    int32_t id = S32(st, SEL_WARSHIP);
    uint32_t f = FIGURE_ADDR(id);
    if (id <= 0 || S16(st, f + 0x132) >= 0 || U8(st, f + 0x38) == 8 || S32(st, MOUSE_OFFSET) == 0) {
        U8(st, WINDOW) = 1;
        U8(st, REDRAW) = 1;
        return;
    }
    int32_t enemy = tile_enemy_figure(st, (int32_t) mouse_offset(st));
    if (enemy != 0) {
        U8(st, f + 0x10) = U8(st, f + 0x38);
        U8(st, f + 0x38) = 0x18;
        S32(st, f + 0x15C) = 0x10;
        S16(st, f + 0x82) = (int16_t) enemy;
        U8(st, WINDOW) = 1;
        S32(st, WARSHIP_MODE) = 0;
        return;
    }
    int32_t px, py;
    uint8_t action = U8(st, f + 0x38);
    switch (action) {
    case 0x09:
        ship_aim(st, f, &px, &py);
        S16(st, f + 0x24) = (int16_t) px;
        S16(st, f + 0x26) = (int16_t) py;
        figure_route_release(st, id);
        U8(st, REDRAW) = 1;
        return;
    case 0x0A:
    case 0x0B:
    case 0x0C:
    case 0x0D:
    case 0x14:
    case 0x17:
        ship_aim(st, f, &px, &py);
        S16(st, f + 0x24) = (int16_t) px;
        S16(st, f + 0x26) = (int16_t) py;
        S16(st, f + 0x14C) = S16(st, f + 0x24);
        S16(st, f + 0x14E) = S16(st, f + 0x26);
        figure_route_release(st, id);
        action = U8(st, f + 0x38);
        U8(st, REDRAW) = 1;
        if (action == 0x17) {
            S32(st, f + 0x15C) = 0x16;
        } else if (action == 0x0A) {
            S32(st, f + 0x15C) = 0x10;
            U8(st, WINDOW) = 1;
            S32(st, WARSHIP_MODE) = 0;
        }
        return;
    default:
        return;
    }
}

void advisor_warship_return(game_state *st)
{
    /* @bin 0x0040A5C0 military advisor, warship row p1 (0x004BC620): home (0x004BC670) */
    int32_t id = player_warship_by_row(st, S32(st, PARAM1));
    if (id == 0 || S16(st, FIGURE_ADDR(id) + 0x132) >= 0) {
        return;
    }
    ship_return_to_wharf(st, id);
    U8(st, REDRAW) = 1;
}

void advisor_warship_service(game_state *st)
{
    /* @bin 0x0040A600 military advisor, warship row p1: kingdom service flag +0x02 toggled when a
     * request of kind 2 is open (0x00521A60), else a popup (0x26 / 0x28) and +0x02 := 0; ships
     * counted again (0x004B8AA0) */
    int32_t id = player_warship_by_row(st, S32(st, PARAM1));
    if (id == 0) {
        return;
    }
    uint8_t other, kind2;
    military_requests_open(st, &other, &kind2);
    uint32_t f = FIGURE_ADDR(id);
    if (S16(st, f + 0x132) >= 0) {
        return;
    }
    if (kind2 != 0) {
        U8(st, f + 0x02) = U8(st, f + 0x02) == 0;
    } else {
        popup(st, other != 0 ? 0x26 : 0x28);
        U8(st, f + 0x02) = 0;
    }
    ships_count(st);
    U8(st, REDRAW) = 1;
}

void figure_order_select(game_state *st)
{
    /* @bin 0x0053D730 (image buttons 0x005C5CB0) on the figure of row 0x00EADCE4 of the list
     * 0x00EADD24: unless its +0x7D is 9, the slot +0x7D of the 8 counters 0x00F55BA4 is cleared
     * and the index search below picks another; when that one's counter is above 1 it is taken:
     * cleared, +0x7D := it, 0x00EADCDC := it, +0x131 := table[it] */
    static const uint8_t table[8] = {8, 0x0D, 2, 1, 0x0B, 5, 1, 0x0A};
    int32_t fid = S32(st, 0x00EADD24u + 4u * U32(st, 0x00EADCE4u));
    uint32_t f = FIGURE_ADDR(fid);
    int32_t slot = S8(st, f + 0x7D);
    if (slot != 9) {
        U8(st, (uint32_t) ((int32_t) 0x00F55BA4 + slot)) = 0;
        /* @bin 0x0053D7A8 the comparison is against the index found so far, not its counter */
        uint8_t best = 0;
        for (uint8_t i = 0; i < 8; i++) {
            if (U8(st, 0x00F55BA4u + i) > best) {
                best = i;
            }
        }
        if (U8(st, 0x00F55BA4u + best) > 1) {
            U8(st, 0x00F55BA4u + best) = 0;
            U8(st, f + 0x7D) = best;
            S32(st, 0x00EADCDCu) = best;
            U8(st, f + 0x131) = table[best];
        }
    }
    U8(st, REDRAW) = 1;
}

/* ---- kingdom requests ---------------------------------------------------------------------- */

int32_t request_row_check(game_state *st, int32_t row, int32_t *event)
{
    /* @bin 0x0044D520 the row-th running event (0x0044BAF0, 0x0044D4C0; none: 0, *event left
     * as it was): *event its index (+0), then whether it can be answered now: money (type 0x24)
     * when the treasury CITY+0x0C is above +0x0C, goods (other types) when CITY+0x48F0 + 4·type
     * is at least +0x0C·100: index + 1, else -1; troops (0x25): a request of kind 2 (record +0x1C
     * of 0x007770F0) needs warships (CITY+0x4668 or +0x466A: -6) and then CITY+0x4669 or +0x4665
     * (-5), others CITY+0x466B (-4) and then +0x4666 (-3); -2 when it can be answered */
    int32_t i = 0;
    uint32_t ev = events_next_active(st, EVENTS, 0);
    for (; ev != 0; ev = events_next_active(st, EVENTS, ev)) {
        if ((uint8_t) event_is_running(st, ev) != 0) {
            if (i == row) {
                break;
            }
            i++;
        }
    }
    if (ev == 0) {
        return 0;
    }
    *event = P16(st, ev);
    uint32_t city = CURRENT_CITY(st);
    uint16_t type = (uint16_t) P16(st, ev + 4);
    if (type == 0x24) {
        if (S32(st, city + 0x0C) > P16(st, ev + 0x0C)) {
            return P16(st, ev) + 1;
        }
        return -1;
    }
    if (type == 0x25) {
        int32_t k = P16(st, ev + 0x1C);
        if (k >= 0 && k < 0x32 && U8(st, 0x007770F0u + 0x144u * (uint32_t) k) == 2) {
            if (U8(st, city + 0x4668) == 0 && U8(st, city + 0x466A) == 0) {
                return -6;
            }
            if (U8(st, city + 0x4669) != 0 || U8(st, city + 0x4665) != 0) {
                return -2;
            }
            return -5;
        }
        if (U8(st, city + 0x466B) == 0) {
            return -4;
        }
        if (U8(st, city + 0x4666) != 0) {
            return -2;
        }
        return -3;
    }
    /* @bin 0x0044D63D goods: player·0x1276 + type dwords from 0x00EA6160 */
    int32_t stock = S32(st, 0x00EA6160u + 4u * (U32(st, CURRENT_PLAYER_ADDR) * 0x1276u + (uint32_t) (int32_t) (int16_t) type));
    if (stock < P16(st, ev + 0x0C) * 100) {
        return -1;
    }
    return P16(st, ev) + 1;
}

void advisor_dispatch_troops(game_state *st)
{
    /* @bin 0x0040A450 military advisor, dispatch (enabled: 0x005F8518, set by the advisor's
     * drawing): the troop requests (0x00521AA0) that can be served (legions CITY+0x4666; kind 2:
     * ships CITY+0x4665 or +0x4669). Several of one kind: the list window (0x0040A0E0, p1 4) and
     * popup 0x66; else each of the first 5 rows (p1) whose event is the first of its kind is
     * dispatched (0x0044C1A0, event in 0x006057E4) */
    if (U8(st, 0x005F8518u) == 0) {
        return;
    }
    int32_t first_other, first_kind2, n_other, n_kind2;
    military_requests(st, &first_other, &first_kind2, &n_other, &n_kind2);
    uint32_t city = CURRENT_CITY(st);
    if (U8(st, city + 0x4666) == 0) {
        first_other = -1;
    }
    if (U8(st, city + 0x4665) == 0 && U8(st, city + 0x4669) == 0) {
        first_kind2 = -1;
    }
    if ((n_other > 1 && first_other >= 0) || (n_kind2 > 1 && first_kind2 >= 0)) {
        S32(st, PARAM1) = 4;
        GAME_CALL0(st, 0x0040A0E0u);
        GAME_CALL(st, 0x004264D0u, 0x66, 0x0051F780, 0);
        return;
    }
    /* the original's slot is not initialised: a row without an event leaves it unchanged (first
     * row: whatever the stack held; taken as 0 here) */
    int32_t event = 0;
    S32(st, PARAM1) = 0;
    do {
        request_row_check(st, S32(st, PARAM1), &event);
        if (event == first_other || event == first_kind2) {
            S32(st, 0x006057E4u) = event;
            GAME_CALL(st, 0x0044C1A0u, event);
        }
        S32(st, PARAM1) += 1;
    } while (S32(st, PARAM1) < 5);
    U8(st, REDRAW) = 1;
}

/* ---- city view clicks ----------------------------------------------------------------------- */

int32_t city_click_legion(game_state *st)
{
    /* @bin 0x0040B630 left press in the city view (no building chosen) on a player's soldier whose
     * formation is not away (0x0040B5E0): selected (0x00BCB9C0), window 0x1E */
    if (U8(st, WINDOW) == 1 && S32(st, CONSTRUCTION_TYPE) != 0) {
        return 0;
    }
    if (S32(st, MOUSE_OFFSET) == 0) {
        return 0;
    }
    int32_t id = tile_soldier_formation(st, (int32_t) mouse_offset(st));
    if (id <= 0 || S16(st, FORMATION_ADDR(id) + 0x82) >= 0) {
        return 0;
    }
    if (formation_has_action_0x12(st, id) != 0) {
        return 0;
    }
    S32(st, SEL_FORMATION) = id;
    U8(st, WINDOW) = 0x1E;
    U8(st, REDRAW) = 1;
    return 1;
}

int32_t city_click_warship(game_state *st)
{
    /* @bin 0x0040B6D0 left press on a warship: on the tile one that is not a player's (+0x132 >=
     * 0), else on the 8 tiles around (offsets 0x005F80FC) a player's: selected (0x00D05704),
     * window 0x20 */
    if (U8(st, WINDOW) == 1 && S32(st, CONSTRUCTION_TYPE) != 0) {
        return 0;
    }
    if (S32(st, MOUSE_OFFSET) == 0) {
        return 0;
    }
    int32_t tile = (int32_t) mouse_offset(st);
    int32_t id = tile_warship(st, tile);
    if (!(id > 0 && S16(st, FIGURE_ADDR(id) + 0x132) >= 0)) {
        id = 0;
        for (int i = 0; i < 8; i++) {
            int32_t c = tile_warship(st, S32(st, 0x005F80FCu + 4u * (uint32_t) i) + tile);
            if (c > 0 && S16(st, FIGURE_ADDR(c) + 0x132) < 0) {
                id = c;
                break;
            }
        }
        if (id == 0) {
            return 0;
        }
    }
    S32(st, SEL_WARSHIP) = id;
    S32(st, WARSHIP_MODE) = 1;
    U8(st, REDRAW) = 1;
    U8(st, WINDOW) = 0x20;
    return 1;
}

int32_t city_click_transport(game_state *st)
{
    /* @bin 0x0040B790 left press on a transport (the tile, then the 8 around): selected
     * (0x00D05700), window 0x1F */
    if (U8(st, WINDOW) == 1 && S32(st, CONSTRUCTION_TYPE) != 0) {
        return 0;
    }
    if (S32(st, MOUSE_OFFSET) == 0) {
        return 0;
    }
    int32_t tile = (int32_t) mouse_offset(st);
    int32_t id = tile_transport(st, tile);
    for (int i = 0; id <= 0 && i < 8; i++) {
        id = tile_transport(st, S32(st, 0x005F80FCu + 4u * (uint32_t) i) + tile);
    }
    if (id <= 0) {
        return 0;
    }
    S32(st, SEL_TRANSPORT) = id;
    S32(st, TRANSPORT_MODE) = 1;
    U8(st, REDRAW) = 1;
    U8(st, WINDOW) = 0x1F;
    return 1;
}

/* ---- table ---------------------------------------------------------------------------------- */

#define VOID0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
#define INT0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        return fn(st); \
    }
VOID0(run_legion_order, legion_order)
VOID0(run_legion_toggle_0x77, legion_toggle_0x77)
VOID0(run_legion_return_to_fort, legion_return_to_fort)
VOID0(run_legion_window_close, legion_window_close)
VOID0(run_legion_click_target, legion_click_target)
VOID0(run_legion_click_target_0x49, legion_click_target_0x49)
VOID0(run_advisor_legion_return, advisor_legion_return)
VOID0(run_advisor_legion_service, advisor_legion_service)
VOID0(run_transport_order, transport_order)
VOID0(run_transport_return, transport_return)
VOID0(run_warship_order, warship_order)
VOID0(run_warship_return, warship_return)
VOID0(run_transport_click_target, transport_click_target)
VOID0(run_warship_click_target, warship_click_target)
VOID0(run_advisor_warship_return, advisor_warship_return)
VOID0(run_advisor_warship_service, advisor_warship_service)
VOID0(run_figure_order_select, figure_order_select)
VOID0(run_ships_count, ships_count)
VOID0(run_advisor_dispatch_troops, advisor_dispatch_troops)
INT0(run_city_click_legion, city_click_legion)
INT0(run_city_click_warship, city_click_warship)
INT0(run_city_click_transport, city_click_transport)

static int32_t run_tile_soldier_formation(game_state *st, const int32_t *a) { return tile_soldier_formation(st, a[0]); }
static int32_t run_tile_transport(game_state *st, const int32_t *a) { return tile_transport(st, a[0]); }
static int32_t run_tile_warship(game_state *st, const int32_t *a) { return tile_warship(st, a[0]); }
static int32_t run_tile_fort_formation(game_state *st, const int32_t *a) { return tile_fort_formation(st, a[0]); }
static int32_t run_tile_enemy_figure(game_state *st, const int32_t *a) { return tile_enemy_figure(st, a[0]); }
static int32_t run_formation_attack(game_state *st, const int32_t *a)
{
    return formation_attack(st, a[0], a[1], a[2], a[3]);
}
static int32_t run_formation_charge_to(game_state *st, const int32_t *a)
{
    return formation_charge_to(st, a[0], a[1], a[2]);
}
static int32_t run_formation_return(game_state *st, const int32_t *a)
{
    formation_return(st, a[0], a[1]);
    return 0;
}
static int32_t run_player_legion_by_row(game_state *st, const int32_t *a) { return player_legion_by_row(st, a[0]); }
static int32_t run_player_warship_by_row(game_state *st, const int32_t *a) { return player_warship_by_row(st, a[0]); }
static int32_t run_ship_return_to_wharf(game_state *st, const int32_t *a)
{
    ship_return_to_wharf(st, a[0]);
    return 0;
}

const ported_function military_ports[] = {
    {0x00406720u, "legion_order", 0, 0, 0, run_legion_order, 0},
    {0x00407110u, "legion_toggle_0x77", 0, 0, 0, run_legion_toggle_0x77, 0},
    {0x00406FB0u, "legion_return_to_fort", 0, 0, 0, run_legion_return_to_fort, 0},
    {0x004071C0u, "legion_window_close", 0, 0, 0, run_legion_window_close, 0},
    {0x0040C440u, "legion_click_target", 0, 0, 0, run_legion_click_target, 0},
    {0x0040C570u, "legion_click_target_0x49", 0, 0, 0, run_legion_click_target_0x49, 0},
    {0x0040A350u, "advisor_legion_return", 0, 0, 0, run_advisor_legion_return, 0},
    {0x0040A390u, "advisor_legion_service", 0, 0, 0, run_advisor_legion_service, 0},
    {0x00406B40u, "transport_order", 0, 0, 0, run_transport_order, 0},
    {0x00406E80u, "transport_return", 0, 0, 0, run_transport_return, 0},
    {0x00406C90u, "warship_order", 0, 0, 0, run_warship_order, 0},
    {0x00406DE0u, "warship_return", 0, 0, 0, run_warship_return, 0},
    {0x0040BA40u, "transport_click_target", 0, 0, 0, run_transport_click_target, 0},
    {0x0040B830u, "warship_click_target", 0, 0, 0, run_warship_click_target, 0},
    {0x0040A5C0u, "advisor_warship_return", 0, 0, 0, run_advisor_warship_return, 0},
    {0x0040A600u, "advisor_warship_service", 0, 0, 0, run_advisor_warship_service, 0},
    {0x0053D730u, "figure_order_select", 0, 0, 0, run_figure_order_select, 0},
    {0x0040B630u, "city_click_legion", 0, 32, 0, run_city_click_legion, 0},
    {0x0040B6D0u, "city_click_warship", 0, 32, 0, run_city_click_warship, 0},
    {0x0040B790u, "city_click_transport", 0, 32, 0, run_city_click_transport, 0},
    {0x004B5520u, "tile_soldier_formation", 1, 32, 0, run_tile_soldier_formation, 0},
    {0x004B5570u, "tile_transport", 1, 32, 0, run_tile_transport, 0},
    {0x004B55B0u, "tile_warship", 1, 32, 0, run_tile_warship, 0},
    {0x004B55F0u, "tile_fort_formation", 1, 32, 0, run_tile_fort_formation, 0},
    {0x004B5640u, "tile_enemy_figure", 1, 32, 0, run_tile_enemy_figure, 0},
    {0x004B66E0u, "formation_attack", 4, 32, 0, run_formation_attack, 0},
    {0x004B6A40u, "formation_charge_to", 3, 32, 0, run_formation_charge_to, 0},
    {0x004B6C20u, "formation_return", 2, 0, 0, run_formation_return, 0},
    {0x004B78B0u, "player_legion_by_row", 1, 32, 0, run_player_legion_by_row, 0},
    {0x004BC620u, "player_warship_by_row", 1, 32, 0, run_player_warship_by_row, 0},
    {0x004BC670u, "ship_return_to_wharf", 1, 0, 0, run_ship_return_to_wharf, 0},
    {0x004B8AA0u, "ships_count", 0, 0, 0, run_ships_count, 0},
    {0x0040A450u, "advisor_dispatch_troops", 0, 0, 0, run_advisor_dispatch_troops, 0},
    {0x0040A550u, "advisor_dispatch_troops_thunk", 0, 0, 0, run_advisor_dispatch_troops, 0}, /* jmp 0x0040A450 */
    /* stack outputs: reached only from the ports above (0x0044D520 also from 0x0040AE10) */
    {0x0044D520u, "request_row_check", 2, 0, 0, NULL, 0},
    {0x00521AA0u, "military_requests", 4, 0, 0, NULL, 0},
    {0x00521A60u, "military_requests_open", 2, 0, 0, NULL, 0},
};
const int military_ports_count = sizeof(military_ports) / sizeof(military_ports[0]);
