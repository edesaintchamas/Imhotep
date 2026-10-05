/* Imhotep - the enemy formations' daily update (0x004B8CF0) and its helpers (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Formation record: docs/formats/save.md §5 (chunk 24); figure record: chunk 21. */
#include "figure/formation.h"

#include <string.h>

#include "map/route_army.h"
#include "map/route_flood.h"
#include "random/crt_rand.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define ROAD_X 0x00D3B4E4u
#define ROAD_Y 0x00D3B4E0u
#define MAP_START 0x00BCA2D8u
#define MAP_WIDTH 0x00BCA2D0u
#define MAP_HEIGHT 0x00BCA2D4u
#define THREAT 0x00A835FCu   /* u8 per tile: threat of the player's soldiers */
#define GROUPS 0x00785D70u   /* the enemy groups (records of 0x7C bytes) */

int32_t enemy_group(game_state *st, int32_t i)
{
    /* @bin 0x0044BB40 (thiscall, this 0x00785D70) group i if in use (+2 == 2), 0x8C: the spare */
    if (i == 0x8C) {
        return (int32_t) (GROUPS + 0x43D4);
    }
    if (i >= 0 && i < S32(st, GROUPS)) {
        uint32_t p = GROUPS + 4 + 0x7Cu * (uint32_t) i;
        return U8(st, p + 2) == 2 ? (int32_t) p : 0;
    }
    state_unported(st, 0x0044BB40u); /* the original reads address 2 */
    return 0;
}

void threat_add(game_state *st, int32_t x, int32_t y, int32_t r, int32_t add)
{
    /* @bin 0x004BA180 +add around (x, y) within r, +2 more on tiles with a player soldier (figure
     * types 0xB..0xD); the last row / column + 1 left in 0x00D3B4E0 / 0x00D3B4E4 */
    int32_t x0 = x - r, x1 = x + r, y0 = y - r, y1 = y + r;
    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    if (x1 >= S32(st, MAP_WIDTH)) {
        x1 = S32(st, MAP_WIDTH) - 1;
    }
    if (y1 >= S32(st, MAP_HEIGHT)) {
        y1 = S32(st, MAP_HEIGHT) - 1;
    }
    S32(st, ROAD_Y) = y0;
    if (y0 > y1) {
        return;
    }
    S32(st, ROAD_Y) = y1 + 1;
    int32_t t = S32(st, MAP_START) + y0 * GRID_SIZE + x0;
    for (int32_t row = y0; row <= y1; row++) {
        S32(st, ROAD_X) = x0;
        if (x0 <= x1) {
            S32(st, ROAD_X) = x1 + 1;
            for (int32_t col = x0; col <= x1; col++, t++) {
                uint8_t v = (uint8_t) (U8(st, THREAT + (uint32_t) t) + (uint8_t) add);
                U8(st, THREAT + (uint32_t) t) = v;
                int16_t f = S16(st, 0x00B350DCu + 2u * (uint32_t) t);
                if (f > 0) {
                    uint8_t ft = U8(st, FIGURE_ADDR(f) + 0x0A);
                    if (ft >= 0xB && ft <= 0xD) {
                        U8(st, THREAT + (uint32_t) t) = (uint8_t) (v + 2);
                    }
                }
            }
        }
        t += x0 - x1 + 0xE3;
    }
}

void threat_rebuild(game_state *st)
{
    /* @bin 0x004BA090 the player's formations 1..6 (in use, +0x42): rings of threat by size */
    memset(state_ptr(st, THREAT, GRID_TILES), 0, GRID_TILES); /* 0x0040F8F0 */
    for (int32_t k = 1; k <= 6; k++) {
        uint32_t f = FORMATION_ADDR(k);
        if (U8(st, f) != 1 || U8(st, f + 0x42) == 0) {
            continue;
        }
        static const struct { uint8_t above; int8_t r; } rings[6] = {{0, 7}, {3, 6}, {6, 5}, {9, 4}, {0xC, 3}, {0xF, 2}};
        for (int i = 0; i < 6; i++) {
            if (U8(st, f + 0x28) > rings[i].above) {
                threat_add(st, S16(st, f + 0x2E), S16(st, f + 0x30), rings[i].r, 1);
            }
        }
    }
}

int32_t threat_highest(game_state *st, int32_t x, int32_t y, int32_t r)
{
    /* @bin 0x004BA290 the reached tile (distances 0x00EED760) of highest threat within r of (x, y)
     * (the first on equality); in 0x00D3B4E4 / 0x00D3B4E0 */
    int32_t x0 = x - r, x1 = x + r, y0 = y - r, y1 = y + r;
    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    if (x1 >= S32(st, MAP_WIDTH)) {
        x1 = S32(st, MAP_WIDTH) - 1;
    }
    if (y1 >= S32(st, MAP_HEIGHT)) {
        y1 = S32(st, MAP_HEIGHT) - 1;
    }
    int32_t best = 0, bx = x, by = y;
    S32(st, ROAD_Y) = y0;
    if (y0 > y1) {
        return 0;
    }
    int32_t t = S32(st, MAP_START) + y0 * GRID_SIZE + x0;
    int32_t row = y0;
    for (; row <= y1; row++) {
        S32(st, ROAD_X) = x0;
        if (x0 <= x1) {
            int32_t col = x0;
            for (; col <= x1; col++, t++) {
                if (S32(st, 0x00EED760u + 4u * (uint32_t) t) > 0 && (int32_t) U8(st, THREAT + (uint32_t) t) > best) {
                    best = U8(st, THREAT + (uint32_t) t);
                    bx = col;
                    by = row;
                }
            }
            S32(st, ROAD_X) = col;
        }
        t += x0 - x1 + 0xE3;
    }
    S32(st, ROAD_Y) = row;
    if (best <= 0) {
        return 0;
    }
    S32(st, ROAD_Y) = by;
    S32(st, ROAD_X) = bx;
    return 1;
}

void formation_charge(game_state *st, int32_t id)
{
    /* @bin 0x004BA040 the formation's figures (not in actions 5, 4) to action 9 */
    uint32_t f = FORMATION_ADDR(id);
    for (uint32_t k = 0; k < 16; k++) {
        int16_t fid = S16(st, f + 0x08 + 2 * k);
        if (fid > 0) {
            uint32_t a = FIGURE_ADDR(fid);
            if (U8(st, a + 0x38) != 5 && U8(st, a + 0x38) != 4) {
                U8(st, a + 0x38) = 9;
                S16(st, a + 0x36) = 0;
            }
        }
    }
}

void enemies_retreat(game_state *st)
{
    /* @bin 0x004B9F90 city +0x45D0 soldiers (types 0x2B..0x35, 0x39, 0x63, 0x36..0x38) to action 4;
     * message 0x130 at the first one's tile */
    uint32_t c = CURRENT_CITY(st) + 0x45D0;
    int32_t n = S32(st, c);
    if (n <= 0) {
        return;
    }
    int32_t first = 0;
    for (int32_t id = 1; id < FIGURE_COUNT; id++) {
        uint32_t a = FIGURE_ADDR(id);
        if (U8(st, a + 0x0E) != 1) {
            continue;
        }
        if (n <= 0) {
            break;
        }
        uint8_t t = U8(st, a + 0x0A);
        if ((t >= 0x2B && t <= 0x35) || t == 0x39 || t == 0x63 || t == 0x36 || t == 0x37 || t == 0x38) {
            n--;
            U8(st, a + 0x38) = 4;
            if (first == 0) {
                first = S32(st, a + 0x20);
            }
        }
    }
    S32(st, c) = 0;
    S32(st, 0x00E950DCu) = 1;
    GAME_CALL(st, 0x004E0CF0u, 0x130, 0, first, 0, 0);
}

/* @bin 0x004BA3B0 one pass over the buildings for formation f of kind `kind` (preferred types at
 * `list`, 100 entries of 0x0D only from 0x005DEB9C + 200 * kind): rank and distance kept across
 * passes */
static void target_pass(game_state *st, uint32_t f, uint32_t list, int32_t *rank, int32_t *lowest, int32_t *best)
{
    for (int32_t i = 1; i < BUILDING_COUNT; i++) {
        uint32_t b = BUILDING_ADDR(i);
        if (U8(st, b) != 1) {
            continue;
        }
        int32_t g = S32(st, b + 0x0C);
        if (U8(st, THREAT + (uint32_t) g) != 0 || S32(st, 0x00EED760u + 4u * (uint32_t) g) <= 0) {
            continue;
        }
        for (int32_t j = 0; j < 100 && j <= *rank; j++) {
            int16_t want = S16(st, list + 2u * (uint32_t) j);
            if (want == 0) {
                break;
            }
            if (S16(st, b + 0x10) != want) {
                continue;
            }
            int32_t fx = S16(st, f + 0x2E), fy = S16(st, f + 0x30), bx = S16(st, b + 6), by = S16(st, b + 8);
            int32_t dx = fx > bx ? fx - bx : fx < bx ? bx - fx : 0;
            S32(st, 0x00D3B4FCu) = dx;
            int32_t dy = fy > by ? fy - by : fy < by ? by - fy : 0;
            S32(st, 0x00D3B4F8u) = dy;
            int32_t d = dx < dy ? dy : dx;
            if (j < *rank) {
                *rank = j;
                *best = i;
                *lowest = d;
            } else if (d < *lowest) {
                *best = i;
                *lowest = d;
            }
            break;
        }
    }
}

int32_t formation_find_target(game_state *st, int32_t id)
{
    /* @bin 0x004BA3B0 a building for the enemy formation to attack: reachable over grid 0x00EC7630
     * (0x0051B040), without threat, of the most preferred type for its kind (+0x44, 4: random), the
     * nearest; else from the general list 0x005DEEBC; its position and id at +0x3A, +0x3C, +0x3E
     * (one tile right and id + 1 for storage yards 0x48 / 0xC1) */
    uint32_t f = FORMATION_ADDR(id);
    int32_t kind = S16(st, f + 0x44);
    if (kind == 4) {
        kind = crt_rand(st) & 3;
    }
    int32_t rank = 100, best = 0, lowest = 10000;
    route_flood_ec7630(st, S16(st, f + 0x2E), S16(st, f + 0x30), -2, -2);
    target_pass(st, f, 0x005DEB9Cu + 200u * (uint32_t) kind, &rank, &lowest, &best);
    if (best <= 0) {
        target_pass(st, f, 0x005DEEBCu, &rank, &lowest, &best);
        if (best <= 0) {
            return 0;
        }
    }
    uint32_t b = BUILDING_ADDR(best);
    int16_t t = S16(st, b + 0x10);
    if (t == 0x48 || t == 0xC1) {
        S16(st, f + 0x3A) = (int16_t) (S16(st, b + 6) + 1);
        S16(st, f + 0x3C) = S16(st, b + 8);
        S16(st, f + 0x3E) = (int16_t) (best + 1);
    } else {
        S16(st, f + 0x3A) = S16(st, b + 6);
        S16(st, f + 0x3C) = S16(st, b + 8);
        S16(st, f + 0x3E) = (int16_t) best;
    }
    return 1;
}

/* @bin 0x004B8E2D a figure of the formation fights (action 5) a player soldier (types 0xB..0xD) */
static int under_attack(game_state *st, uint32_t f)
{
    int r = 0;
    for (uint32_t k = 0; k < 16; k++) {
        uint32_t a = FIGURE_ADDR(S16(st, f + 0x08 + 2 * k));
        if (U8(st, a + 0x38) != 5) {
            continue;
        }
        uint32_t o = FIGURE_ADDR(S16(st, a + 0x90));
        uint8_t ot = U8(st, o + 0x0A);
        if (U8(st, o + 0x0E) == 1 && U8(st, o + 0x38) != 4 && ot >= 0xB && ot <= 0xD) {
            r = 1;
        }
    }
    return r;
}

/* one enemy formation (0x004B8D8C .. 0x004B92B0) */
static void enemy_update(game_state *st, int32_t id)
{
    uint32_t f = FORMATION_ADDR(id);
    if (U8(st, f) != 1 || U8(st, f + 0x42) != 0 || U8(st, f + 0x69) != 0) {
        return;
    }
    uint32_t g = (uint32_t) enemy_group(st, S16(st, f + 0x78));
    /* @bin 0x004B8DBB outnumbering the player 2 to 1: the group charges (+0x4A) */
    if (S32(st, 0x00D05638u) > 2 * S32(st, 0x00D05630u) && S16(st, f + 0x04) != 0xB) {
        S16(st, g + 0x4A) = 1;
    }
    static const uint8_t timers[3] = {0x5C, 0x5E, 0x50};
    for (int i = 0; i < 3; i++) {
        if (S16(st, f + timers[i]) != 0) {
            S16(st, f + timers[i]) -= 1;
        }
    }
    if (S32(st, CURRENT_CITY(st) + 0x4678) <= 0) {
        S16(st, f + 0x50) = 0;
        S16(st, f + 0x5E) = 0;
        S16(st, f + 0x5C) = 0;
    }
    if (under_attack(st, f)) {
        S16(st, f + 0x50) = 6;
    }
    if (U8(st, f + 0x73) != 0 || U8(st, f + 0x66) != 0) {
        /* @bin 0x004B9271 routed: every figure (not in actions 5, 4, 3) flees */
        for (uint32_t k = 0; k < 16; k++) {
            int16_t fid = S16(st, f + 0x08 + 2 * k);
            uint32_t a = FIGURE_ADDR(fid);
            uint8_t act = U8(st, a + 0x38);
            if (act != 5 && act != 4 && act != 3) {
                U8(st, a + 0x38) = 3;
                GAME_CALL(st, 0x004B3250u, fid);
            }
        }
        return;
    }
    int16_t lead = S16(st, f + 0x08);
    if (lead != 0 && U8(st, FIGURE_ADDR(lead) + 0x0E) == 1) {
        S16(st, f + 0x2E) = S16(st, FIGURE_ADDR(lead) + 0x14);
        S16(st, f + 0x30) = S16(st, FIGURE_ADDR(lead) + 0x16);
    }
    g = (uint32_t) enemy_group(st, S16(st, f + 0x78));
    int32_t found = 0; /* local_c: 1 threat within 16, 2 within 32 */
    if (S16(st, g + 0x3A) == 0) {
        /* @bin 0x004B8EE3 the first formation of the group today plans for it */
        S16(st, g + 0x3A) = (int16_t) id;
        S16(st, g + 0x3C) = S16(st, f + 0x2E);
        S16(st, g + 0x3E) = S16(st, f + 0x30);
        S16(st, g + 0x40) = S16(st, f + 0x2A);
        route_army_distances(st, S16(st, f + 0x2E), S16(st, f + 0x30), -2, -2, 0, 100000);
        int32_t near = 0;
        if (threat_highest(st, S16(st, f + 0x2E), S16(st, f + 0x30), 0x10)) {
            near = 1;
            found = 1;
        } else if (threat_highest(st, S16(st, f + 0x2E), S16(st, f + 0x30), 0x20)) {
            near = 2;
            found = 2;
        }
        int attack_soldiers = 0;
        if (S16(st, g + 0x4A) != 0) {
            found = 0;
        } else if (near == 1) {
            attack_soldiers = 1;
        }
        if (attack_soldiers) {
            S16(st, g + 0x42) = S16(st, ROAD_X);
            S16(st, g + 0x44) = S16(st, ROAD_Y);
            S16(st, g + 0x46) = 0;
        } else {
            if (formation_find_target(st, id)) {
                U8(st, f + 0x68) = 0;
            } else {
                uint8_t n = (uint8_t) (U8(st, f + 0x68) + 1);
                U8(st, f + 0x68) = n;
                if (n > 0x30) {
                    U8(st, f + 0x66) = 1;
                }
            }
            S16(st, g + 0x42) = S16(st, f + 0x3A);
            S16(st, g + 0x44) = S16(st, f + 0x3C);
            S16(st, g + 0x46) = S16(st, f + 0x3E);
        }
    }
    /* @bin 0x004B8FCC */
    S16(st, f + 0x58) = S16(st, g + 0x48);
    S16(st, g + 0x48) += 1;
    S16(st, f + 0x4E) += 1;
    S16(st, f + 0x3A) = S16(st, g + 0x42);
    S16(st, f + 0x3C) = S16(st, g + 0x44);
    S16(st, f + 0x3E) = S16(st, g + 0x46);
    int stay = 0, follow = 0, layout_move = 0;
    int32_t follow_id = 0;
    if (S16(st, f + 0x5C) != 0) {
        stay = 1;
    } else if (S16(st, f + 0x5E) != 0) {
        follow = 1;
        follow_id = S16(st, f + 0x60);
    } else if (S16(st, f + 0x4E) < 0x20) {
        layout_move = 1;
        S16(st, f + 0x52) = 4;
    } else if (S16(st, g + 0x4A) == 0) {
        /* @bin 0x004B9049 the phase lengths (attack, regroup, wait) by layout and position */
        int16_t layout = S16(st, g + 0x40);
        int32_t a, w, r;
        if (layout == 8 || layout == 0xC) {
            int16_t pos = S16(st, f + 0x58);
            if (pos == 0 || pos == 1) {
                a = 2; w = 4; r = 2;
            } else if (pos == 2 || pos == 3) {
                a = 2; w = 5; r = 3;
            } else {
                a = 2; w = 6; r = 4;
            }
            if (found == 0) {
                w += 6;
                r -= 1;
                a = 1;
            }
        } else if (found == 0) {
            a = 1; w = 0xC; r = 1;
        } else {
            a = 6; w = 4; r = 2;
        }
        if (S16(st, f + 0x56) != 0) {
            stay = 1;
            S16(st, f + 0x52) = 0;
            S16(st, f + 0x54) = 0;
            S16(st, f + 0x56) -= 1;
            if (S16(st, f + 0x56) <= 0) {
                S16(st, f + 0x54) = (int16_t) a;
                formation_charge(st, id);
            }
        } else if (S16(st, f + 0x54) != 0) {
            S16(st, f + 0x52) = 0;
            S16(st, f + 0x56) = 0;
            S16(st, f + 0x54) -= 1;
            layout_move = 1;
            if (S16(st, f + 0x54) <= 0) {
                S16(st, f + 0x52) = (int16_t) w;
                formation_charge(st, id);
                layout_move = 0;
            }
        } else {
            S16(st, f + 0x54) = 0;
            S16(st, f + 0x56) = 0;
            S16(st, f + 0x52) -= 1;
            if (S16(st, f + 0x52) <= 0) {
                S16(st, f + 0x56) = (int16_t) r;
                formation_charge(st, id);
                stay = 1;
            }
        }
    }
    if (S16(st, f + 0x4E) > 0x2D) {
        enemies_retreat(st);
    }
    if (stay) {
        S16(st, f + 0x3A) = S16(st, f + 0x2E);
        S16(st, f + 0x3C) = S16(st, f + 0x30);
    } else if (follow) {
        if (follow_id <= 0) {
            S16(st, f + 0x3A) = S16(st, g + 0x42);
            S16(st, f + 0x3C) = S16(st, g + 0x44);
        } else {
            uint32_t o = FORMATION_ADDR(follow_id);
            if (U8(st, o + 0x28) != 0) {
                S16(st, f + 0x3A) = S16(st, o + 0x2E);
                S16(st, f + 0x3C) = S16(st, o + 0x30);
            }
        }
    } else if (layout_move) {
        /* @bin 0x004B9213 its place in the group's layout (0x005DCB1C) around the group's point */
        uint32_t e = 0x005DCB1Cu + 8u * (uint32_t) (S16(st, f + 0x58) +
                                                     20 * ((int32_t) (U8(st, f + 0x71) >> 1) + 4 * S16(st, g + 0x40)));
        if (formation_find_place(st, id, S32(st, e) + S16(st, g + 0x3C), S32(st, e + 4) + S16(st, g + 0x3E))) {
            S16(st, f + 0x3A) = S16(st, 0x00BCB9A8u);
            S16(st, f + 0x3C) = S16(st, 0x00BCB9A4u);
        }
    }
}

void formations_enemies(game_state *st)
{
    /* @bin 0x004B8CF0 */
    if (S32(st, 0x00D0563Cu) <= 0) {
        /* no enemy formation: the groups' +0x4A := 0 (0x0044BB90, this 0x00785D70), formation 0's
         * target */
        for (int32_t p = GAME_CALLT(st, GROUPS, 0x0044BB90u, 0, 1); p != 0;) {
            S16(st, (uint32_t) p + 0x4A) = 0;
            p = GAME_CALLT(st, GROUPS, 0x0044BB90u, p, 1);
        }
        formations_target_building(st, 0);
        return;
    }
    S32(st, 0x00D0562Cu) += 1;
    if (S32(st, 0x00D0562Cu) > 4) {
        S32(st, 0x00D0562Cu) = 0;
        threat_rebuild(st);
    }
    for (int32_t p = GAME_CALLT(st, GROUPS, 0x0044BB90u, 0, 1); p != 0;) {
        S16(st, (uint32_t) p + 0x3A) = 0;
        S16(st, (uint32_t) p + 0x48) = 0;
        p = GAME_CALLT(st, GROUPS, 0x0044BB90u, p, 1);
    }
    for (int32_t id = 1; id < 50; id++) {
        enemy_update(st, id);
    }
    formations_target_building(st, 0);
}

static int32_t run_enemy_group(game_state *st, const int32_t *a)
{
    return enemy_group(st, a[0]);
}

static int32_t run_threat_add(game_state *st, const int32_t *a)
{
    threat_add(st, a[0], a[1], a[2], a[3]);
    return 0;
}

static int32_t run_threat_rebuild(game_state *st, const int32_t *a)
{
    (void) a;
    threat_rebuild(st);
    return 0;
}

static int32_t run_threat_highest(game_state *st, const int32_t *a)
{
    return threat_highest(st, a[0], a[1], a[2]);
}

static int32_t run_charge(game_state *st, const int32_t *a)
{
    formation_charge(st, a[0]);
    return 0;
}

static int32_t run_retreat(game_state *st, const int32_t *a)
{
    (void) a;
    enemies_retreat(st);
    return 0;
}

static int32_t run_find_target(game_state *st, const int32_t *a)
{
    return formation_find_target(st, a[0]);
}

static int32_t run_enemies(game_state *st, const int32_t *a)
{
    (void) a;
    formations_enemies(st);
    return 0;
}

const ported_function bigb_enemy_ports[] = {
    {0x004B8CF0u, "formations_enemies", 0, 0, 0, run_enemies, 0},
    {0x0044BB40u, "enemy_group", 1, 32, GROUPS, run_enemy_group, 0},
    {0x004BA180u, "threat_add", 4, 0, 0, run_threat_add, 0},
    {0x004BA090u, "threat_rebuild", 0, 0, 0, run_threat_rebuild, 0},
    {0x004BA290u, "threat_highest", 3, 32, 0, run_threat_highest, 0},
    {0x004BA040u, "formation_charge", 1, 0, 0, run_charge, 0},
    {0x004B9F90u, "enemies_retreat", 0, 0, 0, run_retreat, 0},
    {0x004BA3B0u, "formation_find_target", 1, 8, 0, run_find_target, 0},
};
const int bigb_enemy_ports_count = sizeof(bigb_enemy_ports) / sizeof(bigb_enemy_ports[0]);
