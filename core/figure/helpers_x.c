/* Imhotep - shared figure helpers under the figure actions (package figx).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21). */
#include "figure/helpers_x.h"

#include "figure/actions_a.h"
#include "figure/actions_c2.h"
#include "figure/figure_create.h"
#include "figure/formation.h"
#include "map/route_x.h"
#include "city/day2_misc.h"
#include "city/month_mood.h"
#include "random/crt_rand.h"
#include "random/random.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


int32_t hunter_find_prey(game_state *st, int32_t x, int32_t y, int32_t range)
{
    /* @bin 0x004A2320 the figures in use (+0x0E 1) of types 0x44 .. 0x46 (0x004EA9A0) not in action
     * 4 or 0x0C, unclaimed (+0x138 0) or claimed by the current figure's building (+0x5A): the first
     * 30, by id; the first reached by the walking search 0x005190A0 when the current figure's +0x3E
     * is at most `range` (its route released, 0x004B3250): its id, else 0 */
    int32_t tiles[30], ids[30];
    int32_t n = 0;
    for (int32_t id = 1; id < FIGURE_COUNT; id++) {
        uint32_t f = FIGURE_ADDR(id);
        if (U8(st, f + FIG_STATE) != 1 || (uint8_t) figure_type_is_0x44_0x46(U8(st, f + FIG_TYPE)) == 0) {
            continue;
        }
        uint8_t action = U8(st, f + FIG_ACTION);
        if (action == 4 || action == 0x0C) {
            continue;
        }
        int32_t owner = S32(st, f + FIG_U138);
        if (owner != 0 && owner != S16(st, FIGURE_ADDR(S32(st, G_CURRENT_FIGURE)) + FIG_HOME)) {
            continue;
        }
        ids[n] = id;
        tiles[n] = (int32_t) (U32(st, G_MAP_START) + (uint32_t) (int32_t) S16(st, f + FIG_X) +
                              228u * (uint32_t) (int32_t) S16(st, f + FIG_Y));
        n++;
        if (n == 30) {
            break;
        }
    }
    if (n == 0) {
        return 0;
    }
    int32_t r = route_walk_free_to_marked_list(st, x, y, tiles, n);
    if (r == 0) {
        return 0;
    }
    int32_t cur = S32(st, G_CURRENT_FIGURE);
    if (S16(st, FIGURE_ADDR(cur) + FIG_PATH_LENGTH) > range) {
        return 0;
    }
    figure_route_release(st, cur);
    return ids[r - 1];
}

void cart_unloaded(game_state *st, int32_t amount, int32_t fid, int32_t terrain)
{
    /* @bin 0x0048AEF0 a cart (figure fid) unloaded `amount` at its destination building (+0x5E):
     * the building's loads in transit go down (+0x104, or +0x106 for type 0xCC when the cart's
     * good +0x0B is the one of the building's +0xCE; type 0x47: +0x104 by 1 and +0x106 by the
     * load +0x74); all unloaded: going home (action 0x0F, destination its +0x2C, +0x2E), else the
     * next destination (0x0048AE50) by its type, or action 8 */
    uint32_t f = FIGURE_ADDR(fid);
    int32_t b = S16(st, f + FIG_DEST_BUILDING);
    uint32_t ba = BUILDING_ADDR(b);
    int16_t type = S16(st, ba + BLD_TYPE);
    int counted = 0;
    if (type >= 0x6E && type <= 0x72) {
        counted = 1;
    } else {
        switch (type) { /* @bin 0x0048B160 jump table */
        case 0x20: case 0x2F: case 0x33: case 0x35: case 0x4A: case 0x71: case 0xB1: case 0xB2:
        case 0xBB: case 0xBC: case 0xBD: case 0xC2: case 0xCB: case 0xCC: case 0xCD: case 0xE2:
        case 0xE7: case 0xE8: case 0xE9:
            counted = 1;
            break;
        default:
            break;
        }
    }
    if (counted) {
        /* @bin 0x0048B22C the good of the building's +0xCE */
        int32_t good;
        switch (S16(st, ba + BLD_INPUT2)) {
        case 0x01: good = 0x10; break;
        case 0x03: good = 0x18; break;
        case 0x04: good = 0x14; break;
        case 0x05: good = 0x0B; break;
        case 0x06: good = 0x19; break;
        case 0x07: good = 0x0C; break;
        case 0x08: good = 0x0A; break;
        case 0x09: good = 0x0E; break;
        case 0x0A: good = 0x12; break;
        case 0x0B: good = 0x16; break;
        case 0x0C: good = 0x17; break;
        case 0x0D: good = 0x09; break;
        case 0x0E: good = 0x1C; break;
        case 0x0F: good = 0x15; break;
        case 0x10: good = 0x11; break;
        case 0x11: good = 0x1D; break;
        case 0x12: good = 0x1E; break;
        case 0x14: good = 0x20; break;
        case 0x15: good = 0x02; break;
        case 0x16: good = 0x08; break;
        case 0x17: good = 0x1F; break;
        case 0x18: good = 0x0D; break;
        case 0x19: good = 0x21; break;
        case 0x1A: good = 0x22; break;
        default: good = -1; break;
        }
        if (S16(st, ba + BLD_TYPE) == 0xCC && (int32_t) U8(st, f + FIG_RESOURCE) == good) {
            S16(st, ba + BLD_INCOMING_AMOUNT) -= 1;
        } else {
            S16(st, ba + BLD_INCOMING_CARTS) -= 1;
        }
    } else if (S16(st, ba + BLD_TYPE) == 0x47) {
        int16_t load = S16(st, f + FIG_AMOUNT);
        S16(st, ba + BLD_INCOMING_CARTS) -= 1;
        S16(st, ba + BLD_INCOMING_AMOUNT) = (int16_t) (S16(st, ba + BLD_INCOMING_AMOUNT) - load);
    }
    int16_t load = S16(st, f + FIG_AMOUNT);
    if (amount == load) {
        U8(st, f + FIG_ACTION) = 0x0F;
        S16(st, f + FIG_WAIT_TICKS) = 0;
        S16(st, f + FIG_DEST_X) = S16(st, f + FIG_SOURCE_X);
        S16(st, f + FIG_DEST_Y) = S16(st, f + FIG_SOURCE_Y);
        S16(st, f + FIG_AMOUNT) = 0;
        return;
    }
    S16(st, f + FIG_AMOUNT) = (int16_t) (load - amount);
    int32_t to = cart_find_destination(st, fid, terrain);
    if (to == 0) {
        U8(st, f + FIG_ACTION) = 8;
        S16(st, f + FIG_WAIT_TICKS) = 0;
        return;
    }
    switch (S16(st, BUILDING_ADDR(to) + BLD_TYPE)) { /* @bin 0x0048B2A4 jump table */
    case 0x48: case 0x49: case 0xC1: U8(st, f + FIG_ACTION) = 9; break;
    case 0x47: case 0xBF: U8(st, f + FIG_ACTION) = 0x0A; break;
    case 0x5F: U8(st, f + FIG_ACTION) = 0x10; break;
    default: U8(st, f + FIG_ACTION) = 0x0B; break;
    }
    S16(st, f + FIG_WAIT_TICKS) = 0;
    S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
    S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
}

void figure_new_slot_0x7f(game_state *st, int32_t fid)
{
    /* @bin 0x00444B30 the figure's +0x7F takes the next of the 100 records of 0x58 bytes at
     * 0x0077AF00 (counter 0x0077AEFC), cleared */
    int32_t n = S32(st, G_TRADER_NEXT);
    U8(st, FIGURE_ADDR(fid) + FIG_U7F) = (uint8_t) n;
    GAME_CALL(st, FN_FILL_BYTES, (int32_t) (G_TRADERS + 0x58u * (uint32_t) n), 0x58, 0);
    n = S32(st, G_TRADER_NEXT) + 1;
    S32(st, G_TRADER_NEXT) = n;
    if (n >= 100) {
        S32(st, G_TRADER_NEXT) = 0;
    }
}

int32_t direction_near(int32_t dir, int32_t ref)
{
    /* @bin 0x004B1D10 is dir `ref` or one of its two neighbours (of 8) */
    if (dir == ref) {
        return 1;
    }
    int32_t prev = ref - 1;
    if (prev < 0) {
        prev = 7;
    }
    if (dir == prev) {
        return 1;
    }
    int32_t next = ref + 1;
    if (next >= 8) {
        next = 0;
    }
    return dir == next;
}

int32_t direction_to_fine(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    /* @bin 0x0051CD80 the direction (0..7) from (x0, y0) to (x1, y1): straight when the larger
     * distance is at least 4 times the other (0x004CDE60 at 400), else diagonal; 0 when equal */
    int32_t dx = x0 > x1 ? x0 - x1 : x1 - x0;
    int32_t dy = y0 > y1 ? y0 - y1 : y1 - y0;
    int32_t r;
    if (dx > dy) {
        r = GAME_CALL(st, FN_CALC_RATIO_PERCENT, dx, dy);
    } else if (dx < dy) {
        r = -GAME_CALL(st, FN_CALC_RATIO_PERCENT, dy, dx);
    } else {
        r = 100;
    }
    if (x0 == x1) {
        if (y0 > y1) {
            return 0;
        }
        if (y0 < y1) {
            return 4;
        }
    }
    if (x0 > x1) {
        if (y0 == y1) {
            return 6;
        }
        if (y0 > y1) {
            return r >= 400 ? 6 : r <= -400 ? 0 : 7;
        }
        return r >= 400 ? 6 : r > -400 ? 5 : 4;
    }
    if (x0 >= x1) {
        return 0;
    }
    if (y0 == y1) {
        return 2;
    }
    if (y0 > y1) {
        return r >= 400 ? 2 : r > -400 ? 1 : 0;
    }
    return r >= 400 ? 2 : r <= -400 ? 4 : 3;
}

void formation_morale_hit(game_state *st, int32_t formation)
{
    /* @bin 0x004B7AB0 formations rebuilt (0x004B7C80); the formation's +0x2C goes down by 5, 7,
     * 10, 12, 15 or 20 by 1 in (its figure count, byte 0x00BCB9F0 + 0x90 id) as a percent
     * (0x004CDE60) under 8, 10, 14, 20, 30 or more */
    if (formation == 0) {
        return;
    }
    GAME_CALL(st, FN_FORMATIONS_REBUILD);
    int32_t r = GAME_CALL(st, FN_CALC_RATIO_PERCENT, 1, (int32_t) U8(st, G_FORMATION_BASE + FM_COUNT + 0x90u * (uint32_t) formation));
    int32_t d = r < 8 ? -5 : r < 10 ? -7 : r < 14 ? -10 : r < 20 ? -12 : r < 30 ? -15 : -20;
    GAME_CALL(st, FN_FORMATION_ADD_0X2C, formation, d);
}

void figure_hit_sound(game_state *st, int32_t fid)
{
    /* @bin 0x004B5050 the sound (0x0053B4A0, channel 3) of the figure's type +0x0A: jump table
     * 0x004B51A8; types 0x1A and the others pick one of 4 / 8 at random (0x00562348) */
    int32_t sound;
    switch (U8(st, FIGURE_ADDR(fid) + FIG_TYPE)) {
    case 0x0D: case 0x2D: case 0x38: sound = 0x0D; break;
    case 0x1A: sound = ((crt_rand(st) * 4) >> 15) + 0x1D; break;
    case 0x44: sound = 8; break;
    case 0x45: sound = 4; break;
    case 0x46: sound = 5; break;
    case 0x4D: case 0x4E: case 0x5C: case 0x5D: case 0x64: case 0x65: sound = 0x11; break;
    case 0x52: sound = 7; break;
    case 0x53: sound = 0; break;
    case 0x54: sound = 6; break;
    case 0x66: sound = 3; break;
    case 0x67: sound = 1; break;
    case 0x68: sound = 2; break;
    default: sound = ((crt_rand(st) * 8) >> 15) + 0x15; break;
    }
    GAME_CALL(st, FN_FIGURE_SOUND_53B4A0, fid, sound, 3);
}

void figure_attack_sound(game_state *st, int32_t fid)
{
    /* @bin 0x004B5210 the attack sound (0x0053B4A0, channel 2) of the figure's type: jump table
     * 0x004B528C */
    int32_t sound;
    switch (U8(st, FIGURE_ADDR(fid) + FIG_TYPE)) {
    case 0x0B: case 0x2B: case 0x37: sound = 0x0B; break;
    case 0x0C: case 0x2C: case 0x36: sound = 0x0C; break;
    case 0x0D: case 0x2D: case 0x38: case 0x63: sound = 0x0D; break;
    case 0x4E: case 0x5D: case 0x64: sound = 0x11; break;
    default: return;
    }
    GAME_CALL(st, FN_FIGURE_SOUND_53B4A0, fid, sound, 2);
}

int32_t line_of_fire_clear(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty)
{
    /* @bin 0x004B4420 a straight line (figure record 0 as the walker, 0x004B4220 in 1/15 tile)
     * from (x, y) to (tx, ty): 1 when it gets there, 0 when it crosses terrain 0xC001, or 8 with
     * 0x00B285CC & 0xF (the first 6 tiles skipped when the start has terrain 0xC000), or after
     * 1000 tiles */
    uint32_t z = FIGURE_ADDR(0);
    S16(st, z + FIG_CC_X) = (int16_t) (x * 15);
    S16(st, z + FIG_CC_Y) = (int16_t) (y * 15);
    int32_t skip = 0;
    if (U32(st, G_GRID_TERRAIN + 4u * (U32(st, G_MAP_START) + 228u * (uint32_t) y + (uint32_t) x)) & 0xC000u) {
        skip = 6;
    }
    figure_line_to(st, 0, x * 15, y * 15, tx * 15, ty * 15, 0);
    for (int32_t n = 0; n < 1000; n++) {
        for (int32_t i = 0; i < 8; i++) {
            if ((int32_t) S16(st, z + FIG_CC_DELTA_X) + (int32_t) S16(st, z + FIG_CC_DELTA_Y) <= 0) {
                return 1;
            }
            figure_line_error(st, 0);
            if (U8(st, z + FIG_CC_DIRECTION) == 2) {
                figure_line_step_y(st, 0);
                if (S16(st, z + FIG_CC_ERROR) >= 0) {
                    S16(st, z + FIG_CC_DELTA_X) -= 1;
                    figure_line_step_x(st, 0);
                }
            } else {
                figure_line_step_x(st, 0);
                if (S16(st, z + FIG_CC_ERROR) >= 0) {
                    S16(st, z + FIG_CC_DELTA_Y) -= 1;
                    figure_line_step_y(st, 0);
                }
            }
        }
        int16_t cx = (int16_t) (S16(st, z + FIG_CC_X) / 15);
        int16_t cy = (int16_t) (S16(st, z + FIG_CC_Y) / 15);
        S16(st, z + FIG_X) = cx;
        S16(st, z + FIG_Y) = cy;
        if (skip != 0) {
            skip--;
            continue;
        }
        uint32_t t = U32(st, G_MAP_START) + (uint32_t) (int32_t) cx + 228u * (uint32_t) (int32_t) cy;
        uint32_t flags = U32(st, G_GRID_TERRAIN + 4u * t);
        if (flags & 0xC001u) {
            return 0;
        }
        if ((flags & 8u) && (U8(st, G_GRID_BITFIELDS + t) & 0x0Fu)) {
            return 0;
        }
    }
    return 0;
}

int32_t missile_create(game_state *st, int32_t from, int32_t x, int32_t y, int32_t tx, int32_t ty, int32_t type)
{
    /* @bin 0x004B45C0 a missile figure `type` (0x004B4800) at (x, y) shot by figure `from` (its
     * sound 0x004B5210) towards (tx, ty): +0x1C 0x3C for type 0x3D else 10, +0x5A the shooter, a
     * straight line (0x004B4220, exact), +0x11 its angle (0x0051CAE0, x87, replayed) */
    int32_t id = figure_create(st, S32(st, G_PLAYER), type, x, y, 0);
    if (id == 0) {
        return 0;
    }
    figure_attack_sound(st, from);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_MISSILE_DAMAGE) = type == 0x3D ? 0x3C : 10;
    S16(st, f + FIG_HOME) = (int16_t) from;
    S16(st, f + FIG_DEST_X) = (int16_t) tx;
    S16(st, f + FIG_DEST_Y) = (int16_t) ty;
    figure_line_to(st, id, S16(st, f + FIG_CC_X), S16(st, f + FIG_CC_Y), S16(st, f + FIG_DEST_X) * 15, (int16_t) ty * 15, 1);
    U8(st, f + FIG_DIRECTION) = (uint8_t) GAME_CALL(st, FN_X87_DIRECTION, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X),
                                           S16(st, f + FIG_DEST_Y));
    return id;
}

void figure_spawn_ring(game_state *st, int32_t x, int32_t y, int32_t size, int32_t many)
{
    /* @bin 0x0048C0E0 figures of type 6 (0x004B4800) around (x, y) + offset 0x005DC5FC[size]: 4,
     * or 16 when `many` (sound 0x0053B090(0x11)); each moved by 0x005DC614[size] in 1/15 tile and
     * its destination by the pair at 0x005DC62C + 8 k, sent there (0x004B4220); +0x59 the byte
     * 0x005DC6AC + 4 k, +0x15C whether it is under 3 */
    int32_t off = S32(st, G_U5DC5FC + 4u * (uint32_t) size);
    int32_t sub = S32(st, G_U5DC614 + 4u * (uint32_t) size);
    int32_t n = (uint8_t) many != 0 ? 16 : 4;
    for (int32_t k = 0; k < n; k++) {
        int32_t id = figure_create(st, S32(st, G_PLAYER), 6, x + off, y + off, 0);
        if (id == 0) {
            continue;
        }
        uint32_t f = FIGURE_ADDR(id);
        int16_t dx = S16(st, G_U5DC62C + 8u * (uint32_t) k);
        int16_t dy = S16(st, G_U5DC630 + 8u * (uint32_t) k);
        S16(st, f + FIG_CC_X) = (int16_t) (S16(st, f + FIG_CC_X) + sub);
        S16(st, f + FIG_CC_Y) = (int16_t) (S16(st, f + FIG_CC_Y) + sub);
        S16(st, f + FIG_DEST_X) = (int16_t) (S16(st, f + FIG_DEST_X) + dx);
        S16(st, f + FIG_DEST_Y) = (int16_t) (S16(st, f + FIG_DEST_Y) + dy);
        figure_line_to(st, id, S16(st, f + FIG_CC_X), S16(st, f + FIG_CC_Y), sub + S16(st, f + FIG_DEST_X) * 15,
                       sub + S16(st, f + FIG_DEST_Y) * 15, 0);
        uint8_t v = U8(st, G_U5DC6AC + 4u * (uint32_t) k);
        U8(st, f + FIG_U59) = v;
        U32(st, f + FIG_U15C) = v < 3;
    }
    if ((uint8_t) many != 0) {
        GAME_CALL(st, FN_UI_SOUND, 0x11);
    }
}

void figure_fight_end(game_state *st, int32_t fid)
{
    /* @bin 0x004B1DA0 the fight is over: +0x8B 0, back to the action before (+0x10), opponents
     * +0x90, +0x8C, +0x8E cleared, route released (0x004B3250) */
    uint32_t f = FIGURE_ADDR(fid);
    U8(st, f + FIG_ATTACKERS) = 0;
    U8(st, f + FIG_ACTION) = U8(st, f + FIG_ACTION_BEFORE_ATTACK);
    S16(st, f + FIG_OPPONENT) = 0;
    S16(st, f + FIG_ATTACKER1) = 0;
    S16(st, f + FIG_ATTACKER2) = 0;
    figure_route_release(st, fid);
}

#define FORMATION_AT(id) (G_FORMATION_BASE + 0x90u * (uint32_t) (id))
#define FIGURE_PROPS(type) (G_PROPS + 0x38u * (uint32_t) (type))

/* the formation's facing bonus: +4 / +2 / -2 / -4 by its type (+0x04, 0x0B .. 0x0D) when it holds
 * its position (+0x5A), its facing +0x6B against the figure's direction and its +0x2A */
static int32_t is_alive_fighting(game_state *st, int32_t id)
{
    uint32_t a = FIGURE_ADDR(id);
    return U8(st, a + FIG_STATE) == 1 && U8(st, a + FIG_ACTION) != 4;
}

void figure_fight(game_state *st, int32_t fid)
{
    /* @bin 0x004B17D0 one fighting tick of figure fid against its opponent (+0x90; +0x8B 1 one
     * opponent, 2 two: +0x8C, +0x8E): every 24 ticks (+0x6A; 12 against types whose property
     * +0x00 is 1 or 4) a hit: attack (property +0x08 of the figure's type) against defence
     * (property +0x0C of the opponent's, + (morale +0x7A of the figure's formation + 10) / 20 when
     * the opponent is the player's, at most 20), each changed by the formations' facing (+0x5A,
     * +0x04, +0x6B, +0x2A); damage (20 - defence) * attack / 20 added to the opponent's +0x1D;
     * over its property +0x04 it dies (action 4) */
    uint32_t f = FIGURE_ADDR(fid);
    int32_t fm = S16(st, f + FIG_FORMATION);
    if (S8(st, f + FIG_PROGRESS_ON_TILE) <= 5) {
        S8(st, f + FIG_PROGRESS_ON_TILE) = (int8_t) (S8(st, f + FIG_PROGRESS_ON_TILE) + 1);
        figure_substep(st, fid);
    }
    uint8_t mode = U8(st, f + FIG_ATTACKERS);
    if (mode == 0) {
        figure_fight_end(st, fid);
        return;
    }
    if (mode == 1) {
        if (!is_alive_fighting(st, S16(st, f + FIG_OPPONENT))) {
            figure_fight_end(st, fid);
            return;
        }
    } else if (mode == 2) {
        int32_t o = S16(st, f + FIG_OPPONENT);
        if (!is_alive_fighting(st, o)) {
            if (o == S16(st, f + FIG_ATTACKER1)) {
                S16(st, f + FIG_OPPONENT) = S16(st, f + FIG_ATTACKER2);
            } else if (o == S16(st, f + FIG_ATTACKER2)) {
                S16(st, f + FIG_OPPONENT) = S16(st, f + FIG_ATTACKER1);
            }
            int16_t n = S16(st, f + FIG_OPPONENT);
            if (!is_alive_fighting(st, n)) {
                figure_fight_end(st, fid);
                return;
            }
            U8(st, f + FIG_ATTACKERS) = 1;
            S16(st, f + FIG_ATTACKER1) = n;
            S16(st, f + FIG_ATTACKER2) = 0;
        }
    }
    uint8_t c = (uint8_t) (U8(st, f + FIG_ATTACK_IMAGE) + 1);
    U8(st, f + FIG_ATTACK_IMAGE) = c;
    if (c < 0x18) {
        return;
    }
    int32_t oid = S16(st, f + FIG_OPPONENT);
    uint32_t o = FIGURE_ADDR(oid);
    int32_t ofm = S16(st, o + FIG_FORMATION);
    int32_t kind = S32(st, FIGURE_PROPS(U8(st, o + FIG_TYPE)) + FIGTYPE_U00);
    U8(st, f + FIG_ATTACK_IMAGE) = (kind == 1 || kind == 4) ? 0x0C : 0;
    int32_t bonus = 0;
    if ((int32_t) U8(st, o + FIG_OWNER) == S32(st, G_PLAYER)) {
        bonus = (S16(st, FORMATION_AT(fm) + FM_EXPERIENCE) + 10) / 20;
    }
    int32_t attack = S32(st, FIGURE_PROPS(U8(st, f + FIG_TYPE)) + FIGTYPE_ATTACK);
    int32_t defence = S32(st, FIGURE_PROPS(U8(st, o + FIG_TYPE)) + FIGTYPE_DEFENCE) + bonus;
    if (defence >= 20) {
        defence = 20;
    }
    int32_t fdir = S8(st, f + FIG_ATTACK_DIRECTION), odir = S8(st, o + FIG_ATTACK_DIRECTION);
    uint32_t fa = FORMATION_AT(fm), oa = FORMATION_AT(ofm);
    if (S16(st, o + FIG_OPPONENT) != fid && direction_near(fdir, odir)) {
        int16_t t = S16(st, fa + FM_TYPE);
        if (t == 0x0C || t == 0x0D) {
            attack += 4;
            GAME_CALL(st, FN_RETURN_ZERO, 0x8D);
        }
    }
    if (S16(st, fa + FM_U5A) != 0) {
        int32_t t = S16(st, fa + FM_TYPE);
        if (t == 0x0B || t == 0x0D) {
            if (direction_near(fdir, U8(st, fa + FM_FACING)) && S16(st, fa + FM_LAYOUT) == 6) {
                attack += 2;
            }
        } else if (t == 0x0C) {
            if (direction_near(fdir, U8(st, fa + FM_FACING))) {
                int32_t v = S16(st, fa + FM_LAYOUT);
                if (v > 0) {
                    if (v <= 2) {
                        attack += 4;
                    } else if (v == 6) {
                        attack += 2;
                    }
                }
            }
        }
    }
    if (S16(st, oa + FM_U5A) != 0) {
        int32_t t = S16(st, oa + FM_TYPE);
        if (t == 0x0B || t == 0x0D) {
            if (!direction_near(odir, U8(st, oa + FM_FACING))) {
                defence -= 4;
            } else if (S16(st, oa + FM_LAYOUT) == 6) {
                defence -= 2;
            }
        } else if (t == 0x0C) {
            if (!direction_near(odir, U8(st, oa + FM_FACING))) {
                defence -= 4;
            } else {
                int32_t v = S16(st, oa + FM_LAYOUT);
                if (v > 0) {
                    if (v <= 2) {
                        defence += 4;
                    } else if (v == 6) {
                        defence -= 2;
                    }
                }
            }
        }
    }
    if (S16(st, fa + FM_TYPE) == 0x39 && direction_near(fdir, U8(st, fa + FM_FACING))) {
        int32_t v = S16(st, fa + FM_LAYOUT);
        if (v > 0 && v <= 2) {
            attack += 4;
        }
    }
    if (S16(st, oa + FM_TYPE) == 0x39 && direction_near(odir, U8(st, oa + FM_FACING))) {
        int32_t v = S16(st, oa + FM_LAYOUT);
        if (v > 0 && v <= 2) {
            defence += 4;
        }
    }
    int32_t health = S32(st, FIGURE_PROPS(U8(st, o + FIG_TYPE)) + FIGTYPE_MAX_DAMAGE);
    if (defence < 0) {
        defence = 0;
    }
    if (defence > 20) {
        defence = 20;
    }
    int32_t damage = 0;
    if (attack != 0) {
        damage = (int32_t) ((uint32_t) (20 - defence) * (uint32_t) attack) / 20;
        if (damage < 0) {
            damage = 0;
        }
    }
    uint8_t hurt = (uint8_t) (U8(st, o + FIG_DAMAGE) + (uint8_t) damage);
    U8(st, o + FIG_DAMAGE) = hurt;
    if ((int32_t) hurt <= health) {
        figure_attack_sound(st, fid);
        return;
    }
    uint8_t otype = U8(st, o + FIG_TYPE);
    if ((uint8_t) GAME_CALL(st, FN_FIGURE_TYPE_IS_ENEMY_0X52, otype) == 0 && (uint8_t) GAME_CALL(st, FN_FIGURE_TYPE_IS_0X44_0X46, U8(st, o + FIG_TYPE)) == 0 &&
        U8(st, o + FIG_TYPE) != 0x17 && (int32_t) U8(st, f + FIG_OWNER) == S32(st, G_PLAYER)) {
        int32_t m = S16(st, fa + FM_EXPERIENCE) + 1;
        if (m >= 100) {
            m = 100;
        }
        S16(st, fa + FM_EXPERIENCE) = (int16_t) m;
    }
    U8(st, o + FIG_ACTION) = 4;
    S16(st, o + FIG_WAIT_TICKS) = 0;
    figure_hit_sound(st, oid);
    formation_morale_hit(st, ofm);
}

/* the "rest" action of the animals: +0x36 250 + random % 100, +0x144 0x0C */
static void animal_rest(game_state *st, uint32_t f)
{
    random_next_state(st);
    S16(st, f + FIG_WAIT_TICKS) = (int16_t) (S32(st, G_R_1_15) % 100 + 0xFA);
    S32(st, f + FIG_FRAME_PERIOD) = 0x0C;
    S32(st, f + FIG_FRAME_TICKS) = 0;
}

/* the "wander" action of the animals: one random step; the first of the 8 directions from the
 * random 3 bits (grid offsets at `offsets`) to terrain 4 without 0x80000 is the destination (the
 * (dx, dy) pairs at `pairs`), else a tile of terrain 4 within 5 rings (0x004EB800), else its own
 * tile; +0x148 0 */
static void animal_wander(game_state *st, int32_t fid, uint32_t offsets, uint32_t pairs)
{
    uint32_t f = FIGURE_ADDR(fid);
    random_next_state(st);
    int32_t dir = (int32_t) (U32(st, G_R_IV1) & 7u);
    int32_t k;
    for (k = 0; k < 8; k++) {
        uint32_t t = U32(st, offsets + 4u * (uint32_t) dir) + U32(st, f + FIG_GRID_OFFSET);
        uint32_t flags = U32(st, G_GRID_TERRAIN + 4u * t);
        if ((flags & 4u) && !(flags & 0x80000u)) {
            S16(st, f + FIG_DEST_X) = (int16_t) (S16(st, pairs + 8u * (uint32_t) dir) + S16(st, f + FIG_X));
            S16(st, f + FIG_DEST_Y) = (int16_t) (S16(st, pairs + 4u + 8u * (uint32_t) dir) + S16(st, f + FIG_Y));
            break;
        }
        dir++;
        if (dir >= 8) {
            dir = 0;
        }
    }
    if (k == 8) {
        int32_t ox = 0, oy = 0;
        if (map_search_rings(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), 0, 0, 4, &ox, &oy, 5, fid) == 0) {
            S16(st, f + FIG_DEST_X) = S16(st, f + FIG_X);
            S16(st, f + FIG_DEST_Y) = S16(st, f + FIG_Y);
        } else {
            S16(st, f + FIG_DEST_X) = (int16_t) ox;
            S16(st, f + FIG_DEST_Y) = (int16_t) oy;
        }
    }
    S32(st, f + FIG_FRAME_TICKS) = 0;
}

void animal_set_action(game_state *st, int32_t fid, int32_t action)
{
    /* @bin 0x004ECA70 an animal's next action: 0x13 wanders (+0x144 0x0B; offsets 0x005E4E80,
     * pairs 0x005E4EA0), the others rest */
    uint32_t f = FIGURE_ADDR(fid);
    U8(st, f + FIG_ACTION) = (uint8_t) action;
    if (action != 0x13) {
        animal_rest(st, f);
        return;
    }
    S32(st, f + FIG_FRAME_PERIOD) = 0x0B;
    animal_wander(st, fid, G_U5E4E80, G_U5E4EA0);
}

void animal_set_action2(game_state *st, int32_t fid, int32_t action)
{
    /* @bin 0x004EC4A0 the other animals' next action: 0x12 rests, 0x13 wanders (+0x144 0x0C;
     * offsets 0x005E4E0C, pairs 0x005E4E30), 0x1A waits (+0x144 0x0C, +0x36 20), the others seek
     * (0x004EC990, +0x144 7, +0x36 200) */
    uint32_t f = FIGURE_ADDR(fid);
    U8(st, f + FIG_ACTION) = (uint8_t) action;
    if (action == 0x12) {
        animal_rest(st, f);
        return;
    }
    if (action == 0x13) {
        S32(st, f + FIG_FRAME_PERIOD) = 0x0C;
        animal_wander(st, fid, G_U5E4E0C, G_U5E4E30);
        return;
    }
    if (action == 0x1A) {
        S32(st, f + FIG_FRAME_PERIOD) = 0x0C;
        S16(st, f + FIG_WAIT_TICKS) = 0x14;
        S32(st, f + FIG_FRAME_TICKS) = 0;
        return;
    }
    GAME_CALL(st, FN_ANIMAL_SEEK_TARGET, fid);
    S32(st, f + FIG_FRAME_PERIOD) = 7;
    S16(st, f + FIG_WAIT_TICKS) = 200;
    S32(st, f + FIG_FRAME_TICKS) = 0;
}

void animal_choose_action(game_state *st, int32_t fid)
{
    /* @bin 0x004ECD40 one random step; random % 100 against the weights of the current action
     * (8: 50 / 50, 0x13: 90 / 10, else 25 / 75) picks the next action of 0x005E4EE0 (0x004ECA70) */
    int32_t w[2];
    uint8_t a = U8(st, FIGURE_ADDR(fid) + FIG_ACTION);
    if (a == 8) {
        w[0] = 50;
        w[1] = 50;
    } else if (a == 0x13) {
        w[0] = 90;
        w[1] = 10;
    } else {
        w[0] = 25;
        w[1] = 75;
    }
    random_next_state(st);
    int32_t r = S32(st, G_R_1_15) % 100;
    int32_t i, sum = 0;
    for (i = 0; i < 2; i++) {
        sum += w[i];
        if (r < sum) {
            break;
        }
    }
    animal_set_action(st, fid, S32(st, G_U5E4EE0 + 4u * (uint32_t) i));
}

int32_t boat_ram_target(game_state *st, int32_t fid)
{
    /* @bin 0x004A5C10 a boat (type 0x4D, 0x4E or 0x19, 0x004A59C0) on the figure's tile, else on
     * the next tile in its direction +0x11 (0..7) */
    static const int32_t step[8] = {-228, -227, 1, 229, 228, 227, -1, -229};
    uint32_t f = FIGURE_ADDR(fid);
    int32_t tile = (int32_t) (U32(st, G_MAP_START) + (uint32_t) (int32_t) S16(st, f + FIG_X) +
                              228u * (uint32_t) (int32_t) S16(st, f + FIG_Y));
    uint32_t dir = (uint32_t) (int32_t) S8(st, f + FIG_DIRECTION);
    if (dir > 7) {
        return 0;
    }
    int32_t r = tile_player_boat(st, tile, fid);
    if (r != 0) {
        return r;
    }
    return tile_player_boat(st, tile + step[dir], fid);
}

int32_t figure_held_back(game_state *st, int32_t fid)
{
    /* @bin 0x004BCE90 soldiers 0x0B .. 0x0D whose formation's +0x82 is set (>= 0), ships 0x4D /
     * 0x4E whose +0x132 is: not a target (jump table 0x004BCEF4) */
    uint32_t f = FIGURE_ADDR(fid);
    switch (U8(st, f + FIG_TYPE)) {
    case 0x0B: case 0x0C: case 0x0D:
        return S16(st, G_FORMATION_BASE + 0x90u * (uint32_t) (int32_t) S16(st, f + FIG_FORMATION) + FM_U82) >= 0;
    case 0x4D: case 0x4E:
        return S16(st, f + FIG_U132) >= 0;
    default:
        return 0;
    }
}

int32_t warship_find_target(game_state *st, int32_t x, int32_t y, int32_t range)
{
    /* @bin 0x004BCF70 the best target of a warship at (x, y) among the figures in use, not held
     * (0x004BCE90), not dying: weight 0x11 for soldiers 0x0B .. 0x0D within the current figure's
     * type range (property +0x18), 5 for types 0x14 / 0x19, 15 for 0x4E, 10 (25 when +0x130) for
     * 0x4D; the largest weight minus distance (max of dx, dy; the best distance in 0x00BCD5EC,
     * from `range`) with a clear line (0x004B4420); its position to 0x00D3B4E4 / 0x00D3B4E0 */
    int32_t best = 0, best_w = 0;
    S32(st, G_BEST_DISTANCE) = range;
    for (int32_t id = 1; id < FIGURE_COUNT; id++) {
        uint32_t f = FIGURE_ADDR(id);
        if (U8(st, f + FIG_STATE) != 1 || (uint8_t) figure_held_back(st, id) != 0 || U8(st, f + FIG_ACTION) == 4) {
            continue;
        }
        uint8_t type = U8(st, f + FIG_TYPE);
        int32_t w;
        if (type == 0x0B || type == 0x0C || type == 0x0D) {
            w = 0x11;
        } else if (type == 0x14 || type == 0x19) {
            w = 5;
        } else if (type == 0x4E) {
            w = 0x0F;
        } else if (type == 0x4D) {
            w = U8(st, f + FIG_U130) != 0 ? 25 : 10;
        } else {
            continue;
        }
        int32_t tx = S16(st, f + FIG_X), ty = S16(st, f + FIG_Y);
        int32_t dx = x > tx ? x - tx : x < tx ? tx - x : 0;
        int32_t dy = y > ty ? y - ty : y < ty ? ty - y : 0;
        S32(st, G_DIST_X) = dx;
        S32(st, G_DIST_Y) = dy;
        int32_t d = dx < dy ? dy : dx;
        if (w == 0x11) {
            uint32_t cur = FIGURE_ADDR(S32(st, G_CURRENT_FIGURE));
            if (d > S32(st, G_PROPS + 0x38u * (uint32_t) U8(st, cur + FIG_TYPE) + FIGTYPE_RANGE)) {
                continue;
            }
        }
        if (w - d > best_w - S32(st, G_BEST_DISTANCE) && line_of_fire_clear(st, x, y, tx, ty) != 0) {
            S32(st, G_BEST_DISTANCE) = d;
            best = id;
            best_w = w;
        }
    }
    if (best == 0) {
        return 0;
    }
    S32(st, G_ROAD_ACCESS_Y) = S16(st, FIGURE_ADDR(best) + FIG_Y);
    S32(st, G_ROAD_ACCESS_X) = S16(st, FIGURE_ADDR(best) + FIG_X);
    return best;
}

int32_t warship_find_target_of(game_state *st, int32_t fid, int32_t range)
{
    /* @bin 0x004BCF40 0x004BCF70 from the figure's tile */
    return warship_find_target(st, S16(st, FIGURE_ADDR(fid) + FIG_X), S16(st, FIGURE_ADDR(fid) + FIG_Y), range);
}

int32_t tile_neighbours_with(game_state *st, uint32_t mask, int32_t tile)
{
    /* @bin 0x00477690 how many of the 8 neighbours of the tile have terrain `mask` */
    static const int32_t order[8] = {-228, -227, 1, 229, 228, 227, -1, -229};
    int32_t n = 0;
    for (int k = 0; k < 8; k++) {
        if (U32(st, G_GRID_TERRAIN + 4u * (uint32_t) (tile + order[k])) & mask) {
            n++;
        }
    }
    return n;
}

int32_t tile_ring_solid(game_state *st, int32_t x, int32_t y, int32_t ring)
{
    /* @bin 0x00481490 every tile of ring `ring` (at most 11; offsets 0x0070C8EC from 0x005C9B52,
     * counts 0x005C9AFE) around (x, y), but those off the map (0x00481AA0), has terrain 1 or 2;
     * the centre in 0x00BC9768 */
    if (ring <= 0) {
        return 1;
    }
    if (ring >= 11) {
        ring = 11;
    }
    int32_t first = S16(st, G_RING_FIRST + 2u * (uint32_t) ring);
    int32_t count = S16(st, G_RING_COUNT + 2u * (uint32_t) ring);
    S32(st, G_TILE_CURSOR) = (int32_t) (U32(st, G_MAP_START) + 228u * (uint32_t) y + (uint32_t) x);
    for (int32_t i = 0; i < count; i++) {
        if (ring_tile_outside(st, x, y, 1, ring, i) != 0) {
            continue;
        }
        int32_t t = S32(st, G_TILE_CURSOR) + S16(st, G_RING_OFFSETS_1 + 2u * (uint32_t) (first + i));
        if (!(U8(st, G_GRID_TERRAIN + 4u * (uint32_t) t) & 3u)) {
            return 0;
        }
    }
    return 1;
}

void tree_image_update(game_state *st, int32_t tile)
{
    /* @bin 0x00486600 the image (0x00B7482C) of a tree tile (terrain 1, not 0x80000) by its
     * moisture (0x00A509BC, at most 100, / 34) and the solid rings around it (0x00481490, from 3),
     * or of a terrain-0x40000 tile surrounded by it (0x00477690 == 8) by the map edge (0x0047A1C0);
     * the image groups of 0x00F56B5C, 0x00F56B9C, 0x00F56C3C, 0x00F57244, 0x00F5723C, 0x00F56B94,
     * which by 0x00A43EAC; + the tile's random & 7 */
    uint32_t v = U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile);
    uint32_t rnd = U8(st, G_GRID_RANDOM + (uint32_t) tile) & 7u;
    uint32_t img = G_GRID_IMAGE + 4u * (uint32_t) tile;
    if ((v & 1u) && !(v & 0x80000u)) {
        uint32_t m = U8(st, G_GRID_MOISTURE + (uint32_t) tile);
        if (m > 100) {
            m = 100;
        }
        int32_t q = (int32_t) m / 34;
        if (U8(st, G_GRID_VEGETATION + (uint32_t) tile) != 0xFF) {
            U32(st, img) = U32(st, G_IMAGE_GROUP_509) + rnd + (uint32_t) q * 8u;
            return;
        }
        int32_t rel = tile - S32(st, G_MAP_START);
        int32_t x = rel % 228, y = rel / 228;
        int32_t k = 3;
        for (;;) {
            if (tile_ring_solid(st, x, y, k) != 0) {
                break;
            }
            int32_t was = k;
            k--;
            if (was == 0) {
                break;
            }
        }
        int32_t add;
        if (k < 2) {
            add = q << 4;
        } else {
            add = 0x30;
            k -= 2;
        }
        U32(st, img) = U32(st, G_IMAGE_GROUP_501) + rnd + (uint32_t) (k * 8) + (uint32_t) add;
        return;
    }
    if (!(v & 0x40000u) || (v & 0x80000u) || tile_neighbours_with(st, 0x40000u, tile) != 8) {
        return;
    }
    int32_t rel = tile - S32(st, G_MAP_START);
    uint8_t edge = (uint8_t) GAME_CALL(st, FN_FIGURE_NEAR_MAP_EDGE, rel % 228, rel / 228);
    if (U8(st, G_GRID_VEGETATION + (uint32_t) tile) == 0xFF) {
        U32(st, img) = rnd + (edge ? U32(st, G_IMAGE_GROUP_722) : U32(st, G_IMAGE_GROUP_529));
    } else {
        U32(st, img) = rnd + (edge ? U32(st, G_IMAGE_GROUP_721) : U32(st, G_IMAGE_GROUP_508));
    }
}

int32_t find_gatherable(game_state *st, int32_t x, int32_t y, int32_t unused, uint32_t mask, int32_t x0, int32_t y0,
                        int32_t x1, int32_t y1)
{
    /* @bin 0x00486810 the nearest walkable (0x005190A0) free tile of terrain `mask` (0x40000 reeds
     * surrounded by reeds, 0x00477690 == 8; else trees) in the rows y0 .. y1 - 1, columns x0 .. x1
     * - 1 (clipped by the row ranges 0x00BC9F24 / 0x00BC9804 / 0x00BC9B94), not under a figure of
     * type 0x5A (reeds) or 0x4B, without vegetation (0x00A43EAC 0xFF), in batches of 3999
     * (0x0096E6B4); its tile to 0x00D3B4E4 / 0x00D3B4E0, 1; else the availability flag (0x00978D9A
     * reeds, 0x00978D98) is cleared, 0 */
    (void) unused;
    uint8_t ftype;
    if (mask == 0x40000u) {
        if (U8(st, G_LIST_DONE_C3) == 0) {
            return 0;
        }
        ftype = 0x5A;
    } else {
        if (U8(st, G_LIST_DONE_6C) == 0) {
            return 0;
        }
        ftype = 0x4B;
    }
    int32_t n = 0, r = 0;
    for (int32_t row = y0; row < y1; row++) {
        int32_t start = S32(st, G_ROW_FIRST + 4u * (uint32_t) row);
        int32_t tile, col;
        if (start > x0) {
            tile = S32(st, G_ROW_OFFSET + 4u * (uint32_t) row);
            col = start;
        } else {
            tile = (int32_t) (U32(st, G_MAP_START) + 228u * (uint32_t) row + (uint32_t) x0);
            col = x0;
        }
        int32_t end = S32(st, G_ROW_END + 4u * (uint32_t) row);
        if (end >= x1) {
            end = x1;
        }
        for (; col < end; col++, tile++) {
            uint32_t v = U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile);
            if (v & 0x80000u) {
                continue;
            }
            if (mask & v) {
                if (mask == 0x40000u && tile_neighbours_with(st, 0x40000u, tile) != 8) {
                    continue;
                }
                int32_t fig = S16(st, G_FIGURE_GRID + 2u * (uint32_t) tile);
                if (U8(st, FIGURE_ADDR(fig) + FIG_TYPE) == ftype) {
                    continue;
                }
                if (U8(st, G_GRID_VEGETATION + (uint32_t) tile) == 0xFF) {
                    S32(st, G_FULL_LIST + 4u * (uint32_t) n) = tile;
                    n++;
                }
            }
            if (n == 3999) {
                r = route_walk_free_to_marked(st, x, y, G_FULL_LIST, 3999);
                if (r != 0) {
                    goto found;
                }
                n = 0;
            }
        }
    }
    if (n != 0 && (r = route_walk_free_to_marked(st, x, y, G_FULL_LIST, n)) != 0) {
        goto found;
    }
    if (mask == 0x40000u) {
        U8(st, G_LIST_DONE_C3) = 0;
    } else {
        U8(st, G_LIST_DONE_6C) = 0;
    }
    return 0;
found:;
    int32_t rel = S32(st, G_CANDIDATES + 4u * (uint32_t) r) - S32(st, G_MAP_START);
    S32(st, G_ROAD_ACCESS_X) = rel % 228;
    S32(st, G_ROAD_ACCESS_Y) = rel / 228;
    return 1;
}

int32_t figure_in_attack(game_state *st, int32_t fid)
{
    /* @bin 0x004B63D0 action 6, 7, 0x16 or 0x17 */
    uint8_t a = U8(st, FIGURE_ADDR(fid) + FIG_ACTION);
    return a == 6 || a == 7 || a == 0x17 || a == 0x16;
}

int32_t formation_move_to(game_state *st, int32_t x, int32_t y, int32_t formation, int32_t silent)
{
    /* @bin 0x004B6400 order formation `formation` to (x, y): not while 0x00E38E60; distances from
     * its position (0x00517A40), then a route from its position (+0x84 set) or its leader's
     * (+0x40) (0x00518400; none: warning 0xD3, 0); not to its own tile or while +0x64; +0x32,
     * +0x34 the destination, +0x03 0; unless `silent`: warning 0x33 under 21 men (+0x2C), type 0x0D
     * whose first figure is in action 0x13 (and 0x00D3B198 not 1) sends its men to action 0x14
     * (+0x2A 15), then every man (+0x08, 16) not dying, not attacking (0x004B63D0), with +0x73 0
     * and +0x66 at most 1 (1: morale +10, 0x004B7B50) goes to action 0x0B */
    if (U8(st, G_GATE) != 0) {
        return 0;
    }
    uint32_t fa = G_FORMATION_BASE + 0x90u * (uint32_t) formation;
    int32_t leader = S16(st, fa + FM_STANDARD_BEARER);
    GAME_CALL(st, FN_ROUTE_DISTANCES_TO, S16(st, fa + FM_X), S16(st, fa + FM_Y), -2, -2);
    int32_t r;
    if (S32(st, fa + FM_U84) != 0) {
        r = route_even_to(st, S16(st, fa + FM_X), S16(st, fa + FM_Y), x, y);
    } else {
        uint32_t lf = FIGURE_ADDR(leader);
        r = route_even_to(st, S16(st, lf + FIG_X), S16(st, lf + FIG_Y), x, y);
    }
    if (r == 0) {
        GAME_CALL(st, FN_NOTICE_ADD_4E2970, 0xD3, 0, 0);
        return 0;
    }
    if (S32(st, G_GRID_ROAD_DISTANCE + 4u * (U32(st, G_MAP_START) + 228u * (uint32_t) y + (uint32_t) x)) <= 0) {
        return 0;
    }
    if (x == S16(st, fa + FM_X) && y == S16(st, fa + FM_Y)) {
        return 0;
    }
    if (S16(st, fa + FM_U64) != 0) {
        return 0;
    }
    S16(st, fa + FM_U32) = (int16_t) x;
    S16(st, fa + FM_U34) = (int16_t) y;
    U8(st, fa + FM_U03) = 0;
    if ((uint8_t) silent != 0) {
        return 1;
    }
    if (S16(st, fa + FM_MORALE) <= 20) {
        GAME_CALL(st, FN_NOTICE_ADD_4E2970, 0x33, 0, 0);
    }
    if (S16(st, fa + FM_TYPE) == 0x0D && U8(st, FIGURE_ADDR(S16(st, fa + FM_FIGURES)) + FIG_ACTION) == 0x13 && S32(st, G_PARAM1) != 1) {
        for (int32_t i = 0; i < 16; i++) {
            int32_t id = S16(st, fa + FM_FIGURES + 2u * (uint32_t) i);
            if (id == 0) {
                break;
            }
            uint32_t f = FIGURE_ADDR(id);
            if (U8(st, f + FIG_ACTION) == 4 || U8(st, fa + FM_U73) != 0 || U8(st, fa + FM_U66) > 1 || figure_in_attack(st, id)) {
                continue;
            }
            if (U8(st, fa + FM_U66) == 1) {
                GAME_CALL(st, FN_FORMATION_ADD_0X2C, formation, 10);
            }
            U8(st, f + FIG_U00) = 0;
            S32(st, f + FIG_U134) = 0;
            S16(st, fa + FM_LAYOUT) = 0x0F;
            S32(st, f + FIG_U13C) = S16(st, f + FIG_X);
            S32(st, f + FIG_U140) = S16(st, f + FIG_Y);
            U8(st, f + FIG_ACTION) = 0x14;
            U8(st, FIGURE_ADDR(S32(st, G_CURRENT_FIGURE)) + FIG_ANIM_FRAME) = 0;
        }
    }
    for (int32_t i = 0; i < 16; i++) {
        int32_t id = S16(st, fa + FM_FIGURES + 2u * (uint32_t) i);
        if (id == 0) {
            return 1;
        }
        uint32_t f = FIGURE_ADDR(id);
        uint8_t a = U8(st, f + FIG_ACTION);
        if (a == 4 || a == 5 || U8(st, fa + FM_U73) != 0 || U8(st, fa + FM_U66) > 1 || figure_in_attack(st, id)) {
            continue;
        }
        if (U8(st, fa + FM_U66) == 1) {
            GAME_CALL(st, FN_FORMATION_ADD_0X2C, formation, 10);
        }
        U8(st, f + FIG_U00) = 0;
        U8(st, f + FIG_ACTION) = 0x0B;
        figure_route_release(st, id);
    }
    return 1;
}

int32_t nearest_building_0x63(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00469A50 among the listed buildings (0x0078DD90, count 0x0078D7FC; copied to
     * 0x0078D800) in use of type 0x63 with +0xBD 0 and +0x20 set: the nearest (max of dx, dy) with
     * +0x30 0 (its distance in 0x0078D7F8), else the nearest with +0x30 set when within 2 */
    int32_t best_far = 10000, any = 0, other = 0;
    S32(st, G_RUIN_DISTANCE) = 10000;
    S32(st, G_FIRE_LIST_SIZE) = 0;
    int32_t n = S32(st, G_FIRE_LIST_INDEX);
    if (n <= 0) {
        return 0;
    }
    S32(st, G_FIRE_LIST_SIZE) = n;
    for (int32_t i = 0; i < n; i++) {
        int32_t b = S16(st, G_FIRE_LIST + 2u * (uint32_t) i);
        uint32_t ba = BUILDING_ADDR(b);
        if (U8(st, ba + BLD_STATE) != 1 || S16(st, ba + BLD_TYPE) != 0x63 || U8(st, ba + BLD_UBD) != 0 || S16(st, ba + BLD_DISTANCE) == 0) {
            continue;
        }
        int32_t bx = S16(st, ba + BLD_X), by = S16(st, ba + BLD_Y);
        int32_t dx = x > bx ? x - bx : x < bx ? bx - x : 0;
        int32_t dy = y > by ? y - by : y < by ? by - y : 0;
        S32(st, G_DIST_X) = dx;
        S32(st, G_DIST_Y) = dy;
        int32_t d = dx < dy ? dy : dx;
        if (S16(st, ba + BLD_TOWER_WALKER) != 0) {
            if (d < best_far) {
                best_far = d;
                other = b;
            }
        } else if (d < S32(st, G_RUIN_DISTANCE)) {
            S32(st, G_RUIN_DISTANCE) = d;
            any = b;
        }
    }
    if (any == 0 && best_far <= 2) {
        return other;
    }
    return any;
}

static int32_t run_hunter_find_prey(game_state *st, const int32_t *a)
{
    return hunter_find_prey(st, a[0], a[1], a[2]);
}

static int32_t run_cart_unloaded(game_state *st, const int32_t *a)
{
    cart_unloaded(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_new_slot_0x7f(game_state *st, const int32_t *a)
{
    figure_new_slot_0x7f(st, a[0]);
    return 0;
}

static int32_t run_direction_near(game_state *st, const int32_t *a)
{
    (void) st;
    return direction_near(a[0], a[1]);
}

static int32_t run_direction_to_fine(game_state *st, const int32_t *a)
{
    return direction_to_fine(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_formation_morale_hit(game_state *st, const int32_t *a)
{
    formation_morale_hit(st, a[0]);
    return 0;
}

static int32_t run_figure_hit_sound(game_state *st, const int32_t *a)
{
    figure_hit_sound(st, a[0]);
    return 0;
}

static int32_t run_figure_attack_sound(game_state *st, const int32_t *a)
{
    figure_attack_sound(st, a[0]);
    return 0;
}

static int32_t run_line_of_fire_clear(game_state *st, const int32_t *a)
{
    return line_of_fire_clear(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_missile_create(game_state *st, const int32_t *a)
{
    return missile_create(st, a[0], a[1], a[2], a[3], a[4], a[5]);
}

static int32_t run_figure_spawn_ring(game_state *st, const int32_t *a)
{
    figure_spawn_ring(st, a[0], a[1], a[2], a[3]);
    return 0;
}

static int32_t run_figure_fight_end(game_state *st, const int32_t *a)
{
    figure_fight_end(st, a[0]);
    return 0;
}

static int32_t run_figure_fight(game_state *st, const int32_t *a)
{
    figure_fight(st, a[0]);
    return 0;
}

static int32_t run_animal_set_action(game_state *st, const int32_t *a)
{
    animal_set_action(st, a[0], a[1]);
    return 0;
}

static int32_t run_animal_choose_action(game_state *st, const int32_t *a)
{
    animal_choose_action(st, a[0]);
    return 0;
}

static int32_t run_boat_ram_target(game_state *st, const int32_t *a)
{
    return boat_ram_target(st, a[0]);
}

static int32_t run_figure_held_back(game_state *st, const int32_t *a)
{
    return figure_held_back(st, a[0]);
}

static int32_t run_warship_find_target(game_state *st, const int32_t *a)
{
    return warship_find_target(st, a[0], a[1], a[2]);
}

static int32_t run_warship_find_target_of(game_state *st, const int32_t *a)
{
    return warship_find_target_of(st, a[0], a[1]);
}

static int32_t run_tile_neighbours_with(game_state *st, const int32_t *a)
{
    return tile_neighbours_with(st, (uint32_t) a[0], a[1]);
}

static int32_t run_tile_ring_solid(game_state *st, const int32_t *a)
{
    return tile_ring_solid(st, a[0], a[1], a[2]);
}

static int32_t run_tree_image_update(game_state *st, const int32_t *a)
{
    tree_image_update(st, a[0]);
    return 0;
}

static int32_t run_find_gatherable(game_state *st, const int32_t *a)
{
    return find_gatherable(st, a[0], a[1], a[2], (uint32_t) a[3], a[4], a[5], a[6], a[7]);
}

static int32_t run_animal_set_action2(game_state *st, const int32_t *a)
{
    animal_set_action2(st, a[0], a[1]);
    return 0;
}

static int32_t run_figure_in_attack(game_state *st, const int32_t *a)
{
    return figure_in_attack(st, a[0]);
}

static int32_t run_formation_move_to(game_state *st, const int32_t *a)
{
    return formation_move_to(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_nearest_building_0x63(game_state *st, const int32_t *a)
{
    return nearest_building_0x63(st, a[0], a[1]);
}

const ported_function figx_ports[] = {
    {0x00469A50u, "nearest_building_0x63", 2, 32, 0, run_nearest_building_0x63, 0},
    {0x004B63D0u, "figure_in_attack", 1, 32, 0, run_figure_in_attack, 0},
    {0x004B6400u, "formation_move_to", 4, 32, 0, run_formation_move_to, 0},
    /* 0x004EC4A0 returns a leftover register its only caller ignores */
    {0x004EC4A0u, "animal_set_action2", 2, 0, 0, run_animal_set_action2, 0},
    {0x00486810u, "find_gatherable", 8, 32, 0, run_find_gatherable, 0},
    {0x00477690u, "tile_neighbours_with", 2, 32, 0, run_tile_neighbours_with, 0},
    {0x00481490u, "tile_ring_solid", 3, 32, 0, run_tile_ring_solid, 0},
    {0x00486600u, "tree_image_update", 1, 0, 0, run_tree_image_update, 0},
    {0x004A5C10u, "boat_ram_target", 1, 32, 0, run_boat_ram_target, 0},
    {0x004BCE90u, "figure_held_back", 1, 8, 0, run_figure_held_back, 0},
    {0x004BCF70u, "warship_find_target", 3, 32, 0, run_warship_find_target, 0},
    {0x004BCF40u, "warship_find_target_of", 2, 32, 0, run_warship_find_target_of, 0},
    /* 0x004ECA70 returns a leftover register its only caller ignores */
    {0x004ECA70u, "animal_set_action", 2, 0, 0, run_animal_set_action, 0},
    {0x004ECD40u, "animal_choose_action", 1, 0, 0, run_animal_choose_action, 0},
    {0x004B1DA0u, "figure_fight_end", 1, 0, 0, run_figure_fight_end, 0},
    {0x004B17D0u, "figure_fight", 1, 0, 0, run_figure_fight, 0},
    {0x004B5210u, "figure_attack_sound", 1, 0, 0, run_figure_attack_sound, 0},
    {0x004B4420u, "line_of_fire_clear", 4, 32, 0, run_line_of_fire_clear, 0},
    {0x004B45C0u, "missile_create", 6, 32, 0, run_missile_create, 0},
    {0x0048C0E0u, "figure_spawn_ring", 4, 0, 0, run_figure_spawn_ring, 0},
    {0x004B1D10u, "direction_near", 2, 32, 0, run_direction_near, 0},
    {0x0051CD80u, "direction_to_fine", 4, 8, 0, run_direction_to_fine, 0},
    {0x004B7AB0u, "formation_morale_hit", 1, 0, 0, run_formation_morale_hit, 0},
    {0x004B5050u, "figure_hit_sound", 1, 0, 0, run_figure_hit_sound, 0},
    {0x0048AEF0u, "cart_unloaded", 3, 0, 0, run_cart_unloaded, 0},
    {0x00444B30u, "figure_new_slot_0x7f", 1, 0, 0, run_new_slot_0x7f, 0},
    {0x004A2320u, "hunter_find_prey", 3, 32, 0, run_hunter_find_prey, 0},
};
const int figx_ports_count = sizeof(figx_ports) / sizeof(figx_ports[0]);
