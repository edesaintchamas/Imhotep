/* Imhotep - figure actions, group D (action table 0x005DC28C, figure types 0x51 .. 0x6D).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21). Every action works on the current figure
 * (0x00D056F8) and re-reads it after each call, as the original does. */
#include "figure/actions_d.h"

#include "random/crt_rand.h"
#include "random/random.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#include "figure/actions_d_local.h"

#define BUILDING_TYPE_MONUMENT_SITE 0xB3 /* @bin 0x004A88A2 the home building type of type 0x51 */

int32_t monument_effect_at(game_state *st, int32_t x, int32_t y, int32_t id)
{
    /* @bin 0x004BC1E0 the first of the monuments' handlers (cdecl or thiscall on their objects)
     * that takes (x, y, id); returns its result */
    int32_t r;
    if ((r = GAME_CALL(st, 0x004F0190u, x, y, id)) != 0) return r;
    if ((r = GAME_CALL(st, 0x00420350u, x, y, id)) != 0) return r;
    if ((r = GAME_CALL(st, 0x004E5C00u, x, y, id)) != 0) return r;
    if ((r = GAME_CALL(st, 0x00542A80u, x, y, id)) != 0) return r;
    if ((r = GAME_CALLT(st, 0x006FD5D8u, 0x0040F070u, x, y, id)) != 0) return r;
    if ((r = GAME_CALLT(st, 0x006DE650u, 0x00401F30u, x, y, id)) != 0) return r;
    if ((r = GAME_CALLT(st, 0x006DE548u, 0x004E6E10u, x, y, id)) != 0) return r;
    if ((r = GAME_CALLT(st, 0x006F9320u, 0x00545BE0u, x, y, id)) != 0) return r;
    if ((r = GAME_CALLT(st, 0x006F5648u, 0x005590C0u, x, y, id)) != 0) return r;
    if ((r = GAME_CALLT(st, 0x006DE9E0u, 0x00556430u, x, y, id)) != 0) return r;
    if ((r = GAME_CALLT(st, 0x006E9188u, 0x0054FB40u, x, y, id)) != 0) return r;
    if ((r = GAME_CALLT(st, 0x006FD368u, 0x004178E0u, x, y, id)) != 0) return r;
    return GAME_CALL(st, 0x004E0C00u, x, y, id);
}

/* @bin 0x004A8C9F .. 0x004A8E2A (type 0x51, state 9 arrived): the next state by the type of the
 * building at the destination tile */
static void action_51_arrived(game_state *st, int16_t target, uint32_t tile)
{
    uint32_t f = fig(st);
    if (target != 0) {
        uint32_t t = BUILDING_ADDR(target);
        switch (S16(st, t + 0x10)) {
        case 0xB7:
            U8(st, f + 0x38) = 0x0D;
            GAME_CALL(st, 0x004F1CB0u, cur(st));
            return;
        case 0xD2: case 0xCF: case 0xD0:
        case 0xE5: case 0xEA: case 0xEB: case 0xEC: case 0xE6:
            U8(st, f + 0x38) = 0x0B;
            S16(st, f + 0x36) = 0;
            return;
        case 0xD5:
            U8(st, f + 0x38) = S16(st, t + 0x12) == 0 ? 0x0B : 0x0A;
            S16(st, f + 0x36) = 0;
            return;
        case 0xE3: case 0xE4: case 0xE1: case 0xDE:
            U8(st, f + 0x38) = 0x0A;
            S16(st, f + 0x36) = 0;
            return;
        default:
            U8(st, f + 0x38) = 0x0C;
            return;
        }
    }
    int16_t site = S16(st, f + 0x5E);
    if (S16(st, BUILDING_ADDR(site) + 0x10) != 0xB7) {
        U8(st, f + 0x38) = 0x0C;
        return;
    }
    if (U32(st, TERRAIN_GRID + 4u * tile) & 0x10000000u) {
        U8(st, f + 0x38) = 0x0B;
        S16(st, f + 0x36) = 0;
        return;
    }
    GAME_CALL(st, 0x004F60D0u, site);
    GAME_CALL0(st, 0x004F6130u);
    U8(st, fig(st) + 0x38) = 0x0C;
}

/* @bin 0x004A919B (type 0x51, state 11): work at the building on the destination tile */
static void action_51_work(game_state *st, uint32_t hb, uint8_t *flag)
{
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    uint32_t tile = U32(st, f + 0x28);
    int16_t target = S16(st, BUILDING_GRID + 2u * tile);
    if (target == 0) {
        int16_t site = S16(st, f + 0x5E);
        if (S16(st, BUILDING_ADDR(site) + 0x10) == 0xB7 && (U32(st, TERRAIN_GRID + 4u * tile) & 0x10000000u)) {
            GAME_CALL(st, 0x004EEF10u, site, id, 0x0C);
            return;
        }
        U8(st, f + 0x38) = 8;
        S16(st, f + 0x36) = 0;
        return;
    }
    uint32_t t = BUILDING_ADDR(target);
    switch (S16(st, t + 0x10)) {
    case 0xB7:
        if (U8(st, t + 0x55) == 0) {
            *flag = 1;
        }
        GAME_CALL(st, 0x004EEB60u, target, id);
        return;
    case 0xD2:
        GAME_CALL(st, 0x00420130u, target, id, 0x0C);
        return;
    case 0xCF: case 0xD0:
        GAME_CALL(st, 0x004E5D10u, target, id, 0x0C);
        return;
    case 0xD5:
        GAME_CALL(st, 0x00541DE0u, target, id, 0x0C);
        return;
    case 0xE3:
        GAME_CALLT(st, 0x006FD368u, 0x00417940u, target, id, 0x0C);
        return;
    case 0xE5:
        GAME_CALLT(st, 0x006F9320u, 0x00547E20u, target, id, 0x0C);
        return;
    case 0xEA:
        GAME_CALLT(st, 0x006F5648u, 0x00547E20u, target, id, 0x0C);
        return;
    case 0xEB:
        GAME_CALLT(st, 0x006DE9E0u, 0x00547E20u, target, id, 0x0C);
        return;
    case 0xEC:
        GAME_CALLT(st, 0x006E9188u, 0x00547E20u, target, id, 0x0C);
        return;
    case 0xE4:
        GAME_CALLT(st, 0x006DE548u, 0x004E6E70u, target, id, 0x0C);
        return;
    case 0xE1:
        GAME_CALLT(st, 0x006FD5D8u, 0x0040F0D0u, target, id, 0x0C);
        return;
    case 0xE6:
        GAME_CALLT(st, 0x006DE650u, 0x00401F50u, target, id, 0x0C);
        return;
    case 0xDE:
        GAME_CALL(st, 0x004E0050u, target, id, 0x0C);
        return;
    default:
        S16(st, t + 0xC8) -= 1;
        U8(st, f + 0x38) = 0x0C;
        S16(st, f + 0x24) = S16(st, hb + 0x26);
        S16(st, f + 0x26) = S16(st, hb + 0x28);
        return;
    }
}

/* @bin 0x004A8F99 (type 0x51, state 10): wait at the building on the destination tile while it
 * still lists the figure */
static void action_51_wait(game_state *st)
{
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    int16_t target = S16(st, BUILDING_GRID + 2u * U32(st, f + 0x28));
    S16(st, f + 0x36) += 1;
    if (S16(st, f + 0x36) < 0) {
        S16(st, f + 0x36) = 0;
    }
    if (S8(st, f + 0x11) > 7) {
        random_next_state(st); /* @bin 0x004A8FD1 the generator step 0x004D7090, inlined */
        U8(st, f + 0x11) = (uint8_t) (U32(st, 0x00D3B370u) & 7);
    }
    uint32_t t = BUILDING_ADDR(target);
    switch (S16(st, t + 0x10)) {
    case 0xB7:
        if (S16(st, f + 0x120) == 0) {
            U8(st, f + 0x38) = 0x0C;
        } else if (id != S16(st, t + 0x64)) {
            U8(st, f + 0x38) = 0x0C;
            S16(st, f + 0x120) = 0;
        }
        return;
    case 0xE3: case 0xE4: case 0xE1: {
        /* @bin 0x004A9126 the main building (+0x12: offset back to it) */
        uint32_t m = BUILDING_ADDR(target - S16(st, t + 0x12));
        if (id != S32(st, m + 0x5C)) {
            U8(st, f + 0x38) = 0x0C;
        }
        return;
    }
    case 0xD5: case 0xE5: case 0xEA: case 0xEB: case 0xEC: case 0xDE:
        if (id != S32(st, t + 0x5C)) {
            U8(st, f + 0x38) = 0x0C;
        }
        return;
    default:
        GAME_CALL(st, 0x004B3250u, id);
        f = fig(st);
        U8(st, f + 0x38) = 8;
        S16(st, f + 0x36) = 0;
        return;
    }
}

void figure_action_51(game_state *st)
{
    /* @bin 0x004A8800 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    uint8_t flag = 0;
    if (S16(st, f + 0x36) < 0) {
        S16(st, f + 0x36) = 0;
    }
    next_frame(st, f);
    U8(st, f + 0x72) = 1;
    U8(st, f + 0x0C) = 0;
    U8(st, f + 0x65) = 0;
    S16(st, f + 0x42) = 0x200;
    int32_t speed = S32(st, 0x007090ECu + 0x38u * U8(st, f + 0x0A));
    uint32_t hb = BUILDING_ADDR(S16(st, f + 0x5A));
    if (U8(st, hb) != 1 || S16(st, hb + 0x10) != BUILDING_TYPE_MONUMENT_SITE) {
        U8(st, f + 0x0E) = 2;
        return;
    }
    if (S16(st, f + 0x120) > 0) {
        uint32_t b2 = BUILDING_ADDR(S16(st, f + 0x4E));
        if (S16(st, b2 + 0x10) != 0xB7 || U8(st, b2) == 0) {
            U8(st, f + 0x38) = 0x0C;
            S16(st, f + 0x120) = 0;
        }
    }
    /* @bin 0x004A88FD jump table 0x004A96E4 for states 4 .. 16 */
    switch (U8(st, f + 0x38)) {
    case 4:
        GAME_CALL(st, 0x004B1D50u, id);
        f = fig(st);
        if (U8(st, f + 0x0E) == 2) {
            U8(st, BUILDING_ADDR(S16(st, f + 0x5A)) + 0xA6) -= 1;
        }
        break;
    case 5:
        GAME_CALL(st, 0x004B17D0u, id);
        break;
    case 6:
        S32(st, f + 0x04) = 0;
        break;
    case 7:
        U8(st, f + 0x01) = 0;
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) > 0x64) {
            S16(st, f + 0x36) = 0;
            if ((int8_t) GAME_CALL(st, 0x004A35E0u, id) == 0) {
                U8(st, fig(st) + 0x38) = 8;
            }
        }
        break;
    case 8:
        S16(st, f + 0x36) += 1;
        random_next_state(st); /* @bin 0x004A89C3 the generator step 0x004D7090, inlined */
        /* @bin 0x004A8A8D idle for 30 + random 15-bit % 30 ticks */
        if (S16(st, f + 0x36) > (int32_t) (U32(st, 0x00D3B370u) % 30u) + 30) {
            if (monument_effect_at(st, S16(st, f + 0x14), S16(st, f + 0x16), id) != 0) {
                U8(st, fig(st) + 0x38) = 9;
                GAME_CALL(st, 0x004B3250u, cur(st));
                S16(st, fig(st) + 0x44) = 0;
            }
            S16(st, fig(st) + 0x36) = 0;
        }
        f = fig(st);
        if (U8(st, f + 0x38) != 8) {
            U8(st, f + 0x01) = 0;
            break;
        }
        S16(st, f + 0x44) += 1;
        if (S16(st, f + 0x44) >= S16(st, f + 0x42)) {
            if (road_near(st, hb) == 0) {
                id = cur(st);
                U8(st, FIGURE_ADDR(id) + 0x38) = 4;
                GAME_CALL(st, 0x004B1EF0u, id, 1);
                break;
            }
            id = cur(st);
            f = FIGURE_ADDR(id);
            U8(st, f + 0x38) = 0x0C;
            S16(st, f + 0x24) = S16(st, ROUTE_DEST_X);
            S16(st, f + 0x26) = S16(st, ROUTE_DEST_Y);
            GAME_CALL(st, 0x004B3250u, id);
            id = cur(st);
            S16(st, FIGURE_ADDR(id) + 0x44) = 0;
        }
        GAME_CALL(st, 0x004B1EF0u, cur(st), 1);
        break;
    case 9: {
        U8(st, f + 0x72) = 7;
        uint32_t tile = U32(st, f + 0x28);
        int16_t target = S16(st, BUILDING_GRID + 2u * tile);
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        id = cur(st);
        f = FIGURE_ADDR(id);
        int8_t dir = S8(st, f + 0x11);
        if (dir == 8) {
            action_51_arrived(st, target, tile);
        } else if (dir == 0x0A || dir == 9) {
            int16_t site = S16(st, f + 0x5E);
            uint32_t sb = BUILDING_ADDR(site);
            if (S16(st, sb + 0x10) == 0xB7 && (U32(st, TERRAIN_GRID + 4u * tile) & 0x10000000u)) {
                GAME_CALL(st, 0x004B5420u, id);
                id = cur(st);
                f = FIGURE_ADDR(id);
                S16(st, f + 0x14) = S16(st, f + 0x24);
                S16(st, f + 0x16) = S16(st, f + 0x26);
                S32(st, f + 0x20) = S32(st, f + 0x28);
                GAME_CALL(st, 0x004B52F0u, id);
                f = fig(st);
                U8(st, f + 0x38) = 0x0B;
                S16(st, f + 0x36) = 0;
            } else {
                GAME_CALL(st, 0x004B3250u, id);
                int16_t t = S16(st, sb + 0x10); /* re-read after the call */
                f = fig(st);
                if (t != 0xE1 && t != 0xE6 && t != 0xE3 && t != 0xE4) {
                    U8(st, f + 0x38) = 8;
                }
                S16(st, f + 0x36) = 0;
            }
        }
        break;
    }
    case 0x0A:
        action_51_wait(st);
        break;
    case 0x0B:
        action_51_work(st, hb, &flag);
        break;
    case 0x0C:
        if (S16(st, f + 0x24) != S16(st, hb + 0x26) || S16(st, f + 0x26) != S16(st, hb + 0x28)) {
            S16(st, f + 0x24) = S16(st, hb + 0x26);
            S16(st, f + 0x26) = S16(st, hb + 0x28);
            GAME_CALL(st, 0x004B3250u, id);
        }
        GAME_CALL(st, 0x004A5DA0u, cur(st), speed);
        f = fig(st);
        if (S8(st, f + 0x11) == 8 || S8(st, f + 0x11) == 0x0A || S8(st, f + 0x11) == 9) {
            if (S8(st, f + 0x11) == 9) {
                GAME_CALL(st, 0x004B3250u, cur(st));
            }
            f = fig(st);
            U8(st, f + 0x0E) = 2;
            U8(st, BUILDING_ADDR(S16(st, f + 0x5A)) + 0xA6) -= 1;
        }
        break;
    case 0x0D:
        GAME_CALL(st, 0x004F04D0u, id, 0, 0x0C, 0x0E);
        break;
    case 0x0E:
        GAME_CALL(st, 0x004F0EF0u, id, 1, 0x0C, 0x10, 0x0F);
        break;
    case 0x0F:
        if ((int8_t) GAME_CALL(st, 0x004F1D10u, id, 1) != 0) {
            GAME_CALL(st, 0x004F2410u, cur(st));
            f = fig(st);
            U8(st, f + 0x38) = U8(st, BUILDING_ADDR(S16(st, f + 0x4E)) + 0x55) != 0 ? 0x0B : 0x0A;
            S16(st, f + 0x36) = 0;
        }
        break;
    case 0x10:
        GAME_CALL(st, 0x004F2230u, id);
        break;
    default:
        break;
    }
    id = cur(st);
    f = FIGURE_ADDR(id);
    int32_t d = sprite_direction(st, S8(st, f + 0x11));
    switch (U8(st, f + 0x38)) {
    case 4:
        S32(st, f + 0x04) = (int32_t) (dying_frame(st, f) + U32(st, 0x00F561A4u));
        return;
    case 0x0A: {
        /* @bin 0x004A95B4 working animation: phase from +0x36 and the figure id */
        int32_t phase = (S16(st, f + 0x36) >> 1) + 13 * id;
        switch ((phase / 12) & 3) { /* @bin jump table 0x004A9718 */
        case 0:
            S32(st, f + 0x04) = S32(st, 0x00F561C4u) + 8 * ((phase >> 2) & 1) + d;
            return;
        case 1:
            S32(st, f + 0x04) = S32(st, 0x00F561C4u) + 8 * (phase % 12) + d;
            return;
        case 2:
            if ((phase & 7) != 4) {
                return;
            }
            switch (crt_rand(st) & 3) {
            case 0:
                U8(st, fig(st) + 0x11) = (uint8_t) ((U8(st, fig(st) + 0x11) + 1) & 7);
                break;
            case 1:
                U8(st, fig(st) + 0x11) = (uint8_t) ((U8(st, fig(st) + 0x11) - 1) & 7);
                break;
            default:
                break;
            }
            S32(st, fig(st) + 0x04) = S32(st, 0x00F561C4u) + S32(st, SPRITE_DIRECTION);
            return;
        default:
            S32(st, f + 0x04) = S32(st, 0x00F561C4u) + d;
            return;
        }
    }
    case 0x0B:
        if (flag && S16(st, f + 0x36) < S32(st, 0x005E4EE8u)) {
            S32(st, f + 0x04) = S32(st, 0x00F561C4u) + 8 * U8(st, f + 0x01) + d;
            return;
        }
        if (U8(st, f + 0x01) >= 7) {
            U8(st, f + 0x01) = 0;
        }
        S32(st, f + 0x04) = S32(st, 0x00F561ACu) + 8 * U8(st, f + 0x01) + d;
        return;
    default:
        S32(st, f + 0x04) = S32(st, 0x00F5619Cu) + 8 * U8(st, f + 0x01) + d;
        return;
    }
}

void sledge_follow(game_state *st, int32_t id, int32_t prev, int32_t step, int32_t lag)
{
    /* @bin 0x004B23C0 a sledge part follows the figure before it: on a straight line (+0x120) it
     * takes the leader's line, `lag` behind; otherwise it steps onto the leader's tile every 15
     * ticks of +0x39, and slows down (+0x77 .. +0x79) on the floodplain-like tiles of 0x00ADC36C */
    uint32_t f = FIGURE_ADDR(id);
    uint32_t p = FIGURE_ADDR(prev);
    for (;;) {
        if (S16(st, f + 0x14) == S16(st, f + 0x2C) && S16(st, f + 0x16) == S16(st, f + 0x2E)) {
            U8(st, f + 0x65) = 1;
        }
        if (S16(st, f + 0x120) == 0) {
            break;
        }
        if (S16(st, p + 0x120) == 0) {
            /* @bin 0x004B2426 OutputDebugStringA("follow_figure_closely following off pyramid"):
             * debugger output only */
            S16(st, f + 0x120) = 0;
            continue;
        }
        int copy = 1;
        if (S16(st, f + 0x4E) == S16(st, p + 0x4E)) {
            if (S16(st, f + 0x52) + lag < S16(st, p + 0x52)) {
                GAME_CALL(st, 0x004F1D10u, id, step);
            }
            copy = 0;
        } else if (S16(st, f + 0x4E) == S16(st, p + 0x4A)) {
            copy = (int8_t) GAME_CALL(st, 0x004F1D10u, id, step) != 0;
        }
        if (copy) {
            goto copy_line;
        }
        U8(st, f + 0x01) = U8(st, p + 0x01);
        return;
    }
    if (S16(st, p + 0x120) != 0) {
        U8(st, f + 0x39) += 1;
        if (S8(st, f + 0x39) <= 8) {
            U8(st, f + 0x01) = U8(st, p + 0x01);
            return;
        }
        U8(st, f + 0x39) = 0;
        goto copy_line;
    }
    {
        int32_t a = S8(st, p + 0x39) - lag;
        uint8_t b = (uint8_t) a;
        if (a < 0) {
            b = (uint8_t) (b + 0x0F);
        }
        U8(st, f + 0x39) = b;
        if (b == 8) {
            GAME_CALL(st, 0x004B5420u, id);
            int16_t ox = S16(st, f + 0x14), oy = S16(st, f + 0x16);
            S16(st, f + 0x14) = S16(st, p + 0x14);
            S16(st, f + 0x16) = S16(st, p + 0x16);
            S32(st, f + 0x20) = (int32_t) (U32(st, 0x00BCA2D8u) + (uint32_t) S16(st, f + 0x14)
                                           + 228u * (uint32_t) S16(st, f + 0x16));
            if (S16(st, f + 0x14) != ox || S16(st, f + 0x16) != oy) {
                S16(st, f + 0x18) = ox;
                S16(st, f + 0x1A) = oy;
            }
            GAME_CALL(st, 0x004B52F0u, id);
            uint32_t tile = U32(st, f + 0x20);
            uint32_t t = U32(st, TERRAIN_GRID + 4u * tile);
            if ((t & 0x40u) == 0 && (t & 0x40000000u) == 0) {
                U8(st, f + 0x41) = 0;
            } else {
                U8(st, f + 0x41) = 1;
                if (t & 4u) {
                    U8(st, f + 0x77) = 0x12;
                    uint8_t g = U8(st, 0x00ADC36Cu + tile);
                    uint8_t slow;
                    switch (g) { /* @bin 0x004B2669, jump table 0x004B27BC */
                    case 1: case 4: slow = 0x0F; break;
                    case 2: slow = 0x1B; break;
                    case 3: case 6: slow = 0x0E; break;
                    case 5: slow = 0x1E; break;
                    case 0: slow = 0; break;
                    case 0x0D: slow = 0x1E; break;
                    case 7: case 8: case 9: case 0x0A: slow = 0x0E; break;
                    default: slow = 0x24; break;
                    }
                    U8(st, f + 0x79) = slow;
                }
            }
        }
    }
    if (U8(st, f + 0x39) == 0) {
        U8(st, f + 0x12) = U8(st, f + 0x11);
        U8(st, f + 0x11) = U8(st, p + 0x11);
    }
    if (U8(st, f + 0x77) != 0) {
        U8(st, f + 0x77) -= 1;
        if (U8(st, f + 0x77) == 0) {
            U8(st, f + 0x65) = 0;
        } else {
            U8(st, f + 0x65) = 1;
            if (U8(st, f + 0x78) < U8(st, f + 0x79)) {
                U8(st, f + 0x78) += 1;
            }
            if (U8(st, f + 0x78) > U8(st, f + 0x79)) {
                U8(st, f + 0x78) -= 1;
            }
        }
    } else if (U8(st, f + 0x78) != 0) {
        U8(st, f + 0x78) -= 1;
    } else {
        U8(st, f + 0x65) = 0;
    }
    U8(st, f + 0x01) = U8(st, p + 0x01);
    return;
copy_line:
    S16(st, f + 0x120) = S16(st, p + 0x120);
    S16(st, f + 0x54) = S16(st, p + 0x54);
    {
        int32_t r = S16(st, p + 0x52) - lag;
        S16(st, f + 0x52) = (int16_t) (r < 0 ? 0 : r);
    }
    S16(st, f + 0x4A) = S16(st, p + 0x4A);
    S16(st, f + 0x4E) = S16(st, p + 0x4E);
    S16(st, f + 0x4C) = S16(st, p + 0x4C);
    S16(st, f + 0x50) = S16(st, p + 0x50);
    U8(st, f + 0x11) = U8(st, p + 0x11);
    GAME_CALL(st, 0x004F1D10u, id, 0);
    U8(st, f + 0x01) = U8(st, p + 0x01);
}

int32_t figure_sledge_leader(game_state *st, int32_t id, int32_t *distance)
{
    /* @bin 0x004A34F0 follow the +0x68 links (at most 8) to the figure of type 0x55 */
    int32_t f = S16(st, FIGURE_ADDR(id) + 0x68);
    *distance = 1;
    for (int i = 0; i < 8; i++) {
        if (U8(st, FIGURE_ADDR(f) + 0x0A) == 0x55) {
            return f;
        }
        *distance += 1;
        f = S16(st, FIGURE_ADDR(f) + 0x68);
    }
    return 0;
}

/* @bin 0x004AB9E7 / 0x004AF327 the leader (type 0x55) states in which the parts of its sledge
 * follow it; 0x24 only for type 0x56 */
static int sledge_moving_state(uint8_t s, int with_24)
{
    switch (s) {
    case 0x11: case 0x1E: case 0x12: case 0x13: case 0x14: case 0x17: case 0x22: case 0x23:
    case 0x1F: case 0x1A: case 0x1D: case 7: case 6:
        return 1;
    case 0x24:
        return with_24;
    default:
        return 0;
    }
}

/* the common part of types 0x56 and 0x60: a sledge part follows the figure before it (+0x68)
 * while the leader moves; returns the leader */
static int32_t sledge_part_follow(game_state *st, int with_24, int32_t base, int32_t limit)
{
    int32_t distance;
    int32_t leader = figure_sledge_leader(st, cur(st), &distance);
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    int ok = 0;
    int16_t prev = 0;
    if (U8(st, f + 0x38) == 8) {
        U8(st, f + 0x65) = 0;
        U8(st, f + 0x0C) = 0;
        prev = S16(st, f + 0x68);
        uint32_t p = FIGURE_ADDR(prev);
        if (U8(st, p + 0x0A) == 0x55) {
            ok = sledge_moving_state(U8(st, p + 0x38), with_24);
        } else if (U8(st, p + 0x0A) == 0x60) {
            ok = U8(st, p + 0x38) == 8;
        }
    }
    if (!ok) {
        U8(st, f + 0x0E) = 2;
    } else {
        uint32_t l = FIGURE_ADDR(leader);
        uint8_t ls = U8(st, l + 0x38);
        if (ls != 6) {
            int32_t v = limit;
            if (ls == 0x1D) {
                /* @bin 0x004ABB4D / 0x004AF44E the step: 7 * distance - leader +0x36 (+ base),
                 * clamped to 1 .. limit */
                v = distance * 7 - S16(st, l + 0x36) + base;
                if (v < 1) {
                    v = 1;
                } else if (v > limit) {
                    v = limit;
                }
            }
            sledge_follow(st, id, prev, 1, v);
        }
    }
    if (U8(st, FIGURE_ADDR(leader) + 0x38) == 6) {
        U8(st, fig(st) + 0x65) = 1;
    }
    return leader;
}

/* the sprite direction, not stored in SPRITE_DIRECTION (types 0x56, 0x60) */
static int32_t local_direction(game_state *st, uint32_t f)
{
    int32_t d = S8(st, f + 0x11) - S32(st, VIEW_ORIENTATION);
    return d < 0 ? d + 8 : d;
}

void figure_action_56(game_state *st)
{
    /* @bin 0x004AB980 */
    sledge_part_follow(st, 1, 5, 0x0C);
    uint32_t f = fig(st);
    int32_t d = local_direction(st, f);
    uint32_t group;
    uint8_t r = U8(st, f + 0x0B);
    if (S16(st, f + 0x120) != 0 && S16(st, f + 0x4C) < S16(st, f + 0x50)) {
        /* @bin 0x004ABAF7 tables 0x004ABD10 / 0x004ABCF8 by the carried resource */
        switch (r) {
        case 0x18: group = 0xD5; break;
        case 0x19: group = 0xD6; break;
        case 0x1A: group = 0xD7; break;
        case 0x1E: group = 0xD9; break;
        case 0x23: group = 0xD8; break;
        default: group = 0xDA; break;
        }
    } else {
        /* @bin 0x004ABC27 table 0x004ABD1C */
        switch (r) {
        case 0x18: group = 0xCE; break;
        case 0x19: group = 0xCF; break;
        case 0x1A: group = 0xD0; break;
        case 0x1D: group = 0x2AE; break;
        case 0x1E: group = 0xD2; break;
        case 0x22: group = 0x2BA; break;
        case 0x23: group = 0xD1; break;
        default: group = 0xD3; break;
        }
    }
    S32(st, f + 0x04) = S32(st, 0x00F55BB4u + 8u * group) + d;
}

void figure_action_60(game_state *st)
{
    /* @bin 0x004AF2C0 */
    sledge_part_follow(st, 0, 0, 7);
    uint32_t f = fig(st);
    int32_t d = local_direction(st, f);
    uint32_t base = U8(st, f + 0x0B) != 0 ? 0x00F560B4u : 0x00F560BCu;
    S32(st, f + 0x04) = S32(st, base) + 8 * U8(st, f + 0x01) + d;
}

int32_t figure_find_enemy(game_state *st, int32_t x, int32_t y, int32_t max_distance)
{
    /* @bin 0x0048E820 the figures that can be attacked near (x, y) (alive, not targeted (+0x84),
     * at most 100): distance (max of |dx|, |dy|) times 1 for type 0x29 in state 0x0B, 2 for types
     * 0x2B .. 0x39 and 0x63, 3 for 0x52 .. 0x54, 0x66 .. 0x68, 0x6D; those nearer than
     * max_distance go to 0x00518B30 for the choice (lists 0x00BCA6B4 ids, 0x00BCA524 tiles) */
    int32_t n = 0;
    S32(st, 0x00BCD5ECu) = 10000;
    S32(st, 0x00BCA844u) = 0;
    for (int32_t i = 1; i < FIGURE_COUNT; i++) {
        uint32_t f = FIGURE_ADDR(i);
        if (U8(st, f + 0x0E) != 1 || S16(st, f + 0x84) != 0) {
            continue;
        }
        if (n >= 100) {
            break;
        }
        uint8_t type = U8(st, f + 0x0A);
        int32_t weight;
        if (type == 0x29 && U8(st, f + 0x38) == 0x0B) {
            weight = 1;
        } else if ((type >= 0x2B && type <= 0x39) || type == 0x63) {
            weight = 2;
        } else if (type == 0x52 || type == 0x53 || type == 0x54 || type == 0x67 || type == 0x68 || type == 0x66
                   || type == 0x6D) {
            weight = 3;
        } else {
            continue;
        }
        int32_t fx = S16(st, f + 0x14), fy = S16(st, f + 0x16);
        int32_t dx = x > fx ? x - fx : (x < fx ? fx - x : 0);
        S32(st, 0x00D3B4FCu) = dx;
        int32_t dy = y > fy ? y - fy : (y < fy ? fy - y : 0);
        S32(st, 0x00D3B4F8u) = dy;
        int32_t d = (dx >= dy ? dx : dy) * weight;
        if (d < max_distance) {
            S32(st, 0x00BCA6B4u + 4u * (uint32_t) n) = i;
            S32(st, 0x00BCA524u + 4u * (uint32_t) n) = S32(st, f + 0x20);
            n++;
        }
    }
    S32(st, 0x00BCA844u) = n;
    if (n == 0) {
        return 0;
    }
    int32_t r = GAME_CALL(st, 0x00518B30u, x, y, 0x00BCA524, n);
    if (r <= 0) {
        return 0;
    }
    return S32(st, 0x00BCA6B0u + 4u * (uint32_t) r);
}

int32_t figure_seek_enemy(game_state *st, int32_t id)
{
    /* @bin 0x0048E6F0 every 10th call (+0x80), while there are enemies (C+0x4544 or 0x00D0563C),
     * a figure not dying, fighting or busy goes for an enemy within 30 (state 0x0E, +0x82) */
    uint32_t c = CURRENT_CITY(st);
    if (S32(st, c + 0x4544) <= 0 && S32(st, 0x00D0563Cu) <= 0) {
        return 0;
    }
    uint32_t f = FIGURE_ADDR(id);
    switch (U8(st, f + 0x38)) {
    case 5: case 4: case 8: case 9: case 0x0C: case 0x0D: case 0x0E: case 0x0F:
        return 0;
    default:
        break;
    }
    U8(st, f + 0x80) += 1;
    if (U8(st, f + 0x80) < 10) {
        return 0;
    }
    U8(st, f + 0x80) = 0;
    int32_t t = figure_find_enemy(st, S16(st, f + 0x14), S16(st, f + 0x16), 0x1E);
    if (t <= 0) {
        return 0;
    }
    uint32_t e = FIGURE_ADDR(t);
    U8(st, f + 0x80) = 0;
    U8(st, f + 0x38) = 0x0E;
    S16(st, f + 0x24) = S16(st, e + 0x14);
    S16(st, f + 0x26) = S16(st, e + 0x16);
    S16(st, f + 0x82) = (int16_t) t;
    S16(st, e + 0x84) = (int16_t) id;
    S16(st, f + 0x88) = S16(st, e + 0x86);
    GAME_CALL(st, 0x004B3250u, id);
    return 1;
}

void figure_action_57(game_state *st)
{
    /* @bin 0x004ABD50 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x72) = 1;
    U8(st, f + 0x0C) = 0;
    S16(st, f + 0x42) = 0x180;
    uint32_t b = BUILDING_ADDR(S16(st, f + 0x5A));
    if (U8(st, b) != 1 || S16(st, b + 0x2A) != id) {
        U8(st, f + 0x0E) = 2; /* its building is gone, or no longer links to it */
    }
    next_frame(st, f);
    GAME_CALL(st, 0x00493610u, 1);
    f = fig(st);
    int32_t d = sprite_direction(st, S8(st, f + 0x11));
    if (U8(st, f + 0x38) == 4) {
        S32(st, f + 0x04) = (int32_t) (dying_frame(st, f) + U32(st, 0x00F55ECCu));
        return;
    }
    S32(st, f + 0x04) = S32(st, 0x00F55EC4u) + 8 * U8(st, f + 0x01) + d;
}

/* @bin 0x004ABEE5 (type 0x58), 0x004AC4DC (type 0x59): the states of a worker that leaves its
 * home building (+0x5A) for a road tile next to it, and returns. b: the home building's record,
 * read when the action starts. */
static void home_worker_states(game_state *st, uint32_t b, int16_t home, int32_t speed, int with_53b4a0)
{
    int32_t id;
    uint32_t f;
    id = cur(st);
    f = FIGURE_ADDR(id);
    switch (U8(st, f + 0x38)) {
    case 5:
        if (with_53b4a0) {
            GAME_CALL(st, 0x0053B4A0u, id, 10, 2);
            id = cur(st);
        }
        GAME_CALL(st, 0x004B17D0u, id);
        break;
    case 4:
        GAME_CALL(st, 0x004B1D50u, id);
        break;
    case 8:
        U8(st, f + 0x65) = 1;
        U8(st, f + 0x01) = 0;
        S16(st, f + 0x36) -= 1;
        if (S16(st, f + 0x36) > 0) {
            break;
        }
        if (road_near(st, b) == 0) {
            U8(st, fig(st) + 0x0E) = 2;
            break;
        }
        id = cur(st);
        f = FIGURE_ADDR(id);
        U8(st, f + 0x38) = 9;
        S16(st, f + 0x24) = S16(st, ROUTE_DEST_X);
        S16(st, f + 0x26) = S16(st, ROUTE_DEST_Y);
        move_to_dest(st, id, f);
        S16(st, fig(st) + 0x44) = 0;
        break;
    case 9:
        U8(st, f + 0x0C) = 1;
        U8(st, f + 0x65) = 1;
        if (GAME_CALL(st, 0x004B3FF0u, id, 1) != 1) {
            break;
        }
        id = cur(st);
        f = FIGURE_ADDR(id);
        if (S16(st, BUILDING_GRID + 2u * U32(st, f + 0x20)) == home) {
            U8(st, f + 0x0E) = 2;
            break;
        }
        U8(st, f + 0x38) = 0x0A;
        GAME_CALL(st, 0x004B1DF0u, id);
        S16(st, fig(st) + 0x44) = 0;
        break;
    case 0x0A:
        U8(st, f + 0x65) = 0;
        S16(st, f + 0x44) += 1;
        if (S16(st, f + 0x44) >= S16(st, f + 0x42)) {
            if (road_near(st, b) == 0) {
                id = cur(st);
                U8(st, FIGURE_ADDR(id) + 0x0E) = 2;
                GAME_CALL(st, 0x004B1EF0u, id, 1);
                break;
            }
            id = cur(st);
            f = FIGURE_ADDR(id);
            U8(st, f + 0x38) = 0x0B;
            S16(st, f + 0x24) = S16(st, ROUTE_DEST_X);
            S16(st, f + 0x26) = S16(st, ROUTE_DEST_Y);
            GAME_CALL(st, 0x004B3250u, id);
            id = cur(st);
        }
        GAME_CALL(st, 0x004B1EF0u, id, 1);
        break;
    case 0x0B:
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        id = cur(st);
        f = FIGURE_ADDR(id);
        if (S8(st, f + 0x11) == 9 || S8(st, f + 0x11) == 0x0A) {
            U8(st, f + 0x0E) = 2;
        } else if (S8(st, f + 0x11) == 8) {
            U8(st, f + 0x38) = 9;
            S16(st, f + 0x24) = S16(st, b + 0x06);
            S16(st, f + 0x26) = S16(st, b + 0x08);
            move_to_dest(st, id, f);
            S16(st, fig(st) + 0x44) = 0;
        }
        break;
    case 0x0E:
        U8(st, f + 0x72) = 0;
        if (GAME_CALL(st, 0x004B46B0u, id) == 0) {
            if (road_near(st, BUILDING_ADDR(S16(st, fig(st) + 0x5A))) == 0) {
                U8(st, fig(st) + 0x0E) = 2;
            } else {
                id = cur(st);
                f = FIGURE_ADDR(id);
                U8(st, f + 0x38) = 0x0B;
                S16(st, f + 0x24) = S16(st, ROUTE_DEST_X);
                S16(st, f + 0x26) = S16(st, ROUTE_DEST_Y);
                GAME_CALL(st, 0x004B3250u, id);
                S16(st, fig(st) + 0x44) = 0;
            }
        }
        id = cur(st);
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        id = cur(st);
        f = FIGURE_ADDR(id);
        if (S8(st, f + 0x11) == 9 || S8(st, f + 0x11) == 0x0A) {
            U8(st, f + 0x0E) = 2;
        } else if (S8(st, f + 0x11) == 8) {
            uint32_t o = FIGURE_ADDR(S16(st, f + 0x82));
            S16(st, f + 0x24) = S16(st, o + 0x14);
            S16(st, f + 0x26) = S16(st, o + 0x16);
            GAME_CALL(st, 0x004B3250u, id);
        }
        break;
    default:
        U8(st, f + 0x38) = 8;
        break;
    }
}

void figure_action_58(game_state *st)
{
    /* @bin 0x004ABE40 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x72) = 1;
    U8(st, f + 0x0C) = 0;
    S16(st, f + 0x42) = 0x280;
    int16_t home = S16(st, f + 0x5A);
    /* @bin 0x004ABE87 per-type table 0x007090EC (0x38 bytes per type), passed to 0x004A5DA0 */
    int32_t speed = S32(st, 0x007090ECu + 0x38u * U8(st, f + 0x0A));
    uint32_t b = BUILDING_ADDR(home);
    if (U8(st, b) != 1 || S16(st, b + 0x2A) != id) {
        U8(st, f + 0x0E) = 2;
    }
    if (next_frame(st, f) == 0) {
        figure_seek_enemy(st, id);
    }
    home_worker_states(st, b, home, speed, 1);
    id = cur(st);
    f = FIGURE_ADDR(id);
    uint8_t action = U8(st, f + 0x38);
    int32_t dir;
    if (action == 5) {
        dir = S8(st, f + 0x13);
    } else if (S8(st, f + 0x11) < 8) {
        dir = S8(st, f + 0x11);
    } else {
        dir = S8(st, f + 0x12);
    }
    int32_t d = sprite_direction(st, dir);
    if (action == 5) {
        GAME_CALL(st, 0x0053B4A0u, id, 10, 2);
        f = fig(st);
        uint8_t a = U8(st, f + 0x6A);
        if (a < 0x0C) {
            S32(st, f + 0x04) = S32(st, 0x00F55D9Cu) + S32(st, SPRITE_DIRECTION);
        } else {
            S32(st, f + 0x04) = S32(st, 0x00F55D9Cu) + ((int32_t) a - 0x0C) / 2 * 8 + S32(st, SPRITE_DIRECTION);
        }
        return;
    }
    if (action == 4) {
        S32(st, f + 0x04) = (int32_t) (dying_frame(st, f) + U32(st, 0x00F55D94u));
        return;
    }
    S32(st, f + 0x04) = S32(st, 0x00F55D8Cu) + 8 * U8(st, f + 0x01) + d;
}

void figure_action_59(game_state *st)
{
    /* @bin 0x004AC450 type 0x58 without the 0x0048E6F0 / 0x0053B4A0 calls, other sprites */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x72) = 1;
    U8(st, f + 0x0C) = 0;
    S16(st, f + 0x42) = 0x280;
    int16_t home = S16(st, f + 0x5A);
    int32_t speed = S32(st, 0x007090ECu + 0x38u * U8(st, f + 0x0A));
    uint32_t b = BUILDING_ADDR(home);
    if (U8(st, b) != 1 || S16(st, b + 0x2A) != id) {
        U8(st, f + 0x0E) = 2;
    }
    next_frame(st, f);
    home_worker_states(st, b, home, speed, 0);
    f = fig(st);
    uint8_t action = U8(st, f + 0x38);
    int32_t dir;
    if (action == 5) {
        dir = S8(st, f + 0x13);
    } else if (S8(st, f + 0x11) < 8) {
        dir = S8(st, f + 0x11);
    } else {
        dir = S8(st, f + 0x12);
    }
    int32_t d = sprite_direction(st, dir);
    if (action == 5) {
        uint8_t a = U8(st, f + 0x6A);
        if (a < 0x0C) {
            S32(st, f + 0x04) = S32(st, 0x00F5638Cu) + d;
        } else {
            S32(st, f + 0x04) = S32(st, 0x00F5638Cu) + ((int32_t) a - 0x0C) / 2 * 8 + d;
        }
        return;
    }
    if (action == 4) {
        S32(st, f + 0x04) = (int32_t) (dying_frame(st, f) + U32(st, 0x00F56394u));
        return;
    }
    S32(st, f + 0x04) = S32(st, 0x00F5638Cu) + 8 * U8(st, f + 0x01) + d;
}

void figure_action_5A(game_state *st)
{
    /* @bin 0x004AC9F0 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x72) = 3;
    U8(st, f + 0x65) = 0;
    int32_t speed = S32(st, 0x007090ECu + 0x38u * U8(st, f + 0x0A));
    int16_t home = S16(st, f + 0x5A);
    if (home != 0 && U8(st, BUILDING_ADDR(home)) != 1) {
        U8(st, f + 0x0E) = 2;
        return;
    }
    /* @bin 0x004ACA65 jump table 0x004ACE90 for states 4 .. 11 */
    switch (U8(st, f + 0x38)) {
    case 4:
        GAME_CALL(st, 0x004B1D50u, id);
        break;
    case 5:
        GAME_CALL(st, 0x004B17D0u, id);
        break;
    case 6:
        U8(st, f + 0x65) = 1;
        break;
    case 7:
        /* @bin 0x004ACD73 the sprite direction of the previous update */
        S32(st, f + 0x04) = S32(st, 0x00F55E14u) + S32(st, SPRITE_DIRECTION);
        U8(st, f + 0x01) = 0;
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) > 0x64) {
            S16(st, f + 0x36) = 0;
            if ((int8_t) GAME_CALL(st, 0x004A35E0u, id) == 0) {
                U8(st, fig(st) + 0x38) = 8;
            }
        }
        break;
    case 8:
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) > 0x0A) {
            if (S32(st, 0x0078D7B0u) != 0
                || GAME_CALL(st, 0x004867E0u, S16(st, f + 0x14), S16(st, f + 0x16), 0xFF, 0x40000) == 0) {
                id = cur(st);
                U8(st, FIGURE_ADDR(id) + 0x0E) = 2;
                GAME_CALL(st, 0x004B5420u, id);
                return;
            }
            f = fig(st);
            U8(st, f + 0x38) = 9;
            S16(st, f + 0x24) = S16(st, ROUTE_DEST_X);
            S16(st, f + 0x26) = S16(st, ROUTE_DEST_Y);
        }
        U8(st, f + 0x01) = 0;
        break;
    case 9:
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = fig(st);
        if (S8(st, f + 0x11) == 8) {
            uint32_t t = U32(st, f + 0x20);
            /* @bin 0x004ACBD4 tile byte 0xFF in 0x00A43EAC, and no figure of state 0x0A on it
             * (figure grid 0x00B350DC) */
            int ok = U8(st, 0x00A43EACu + t) >= 0xFF;
            if (ok) {
                int16_t o = S16(st, 0x00B350DCu + 2u * t);
                if (o != 0 && U8(st, FIGURE_ADDR(o) + 0x38) == 0x0A) {
                    ok = 0;
                }
            }
            if (ok) {
                U8(st, f + 0x38) = 0x0A;
                S16(st, f + 0x36) = 0;
                U8(st, f + 0x11) = U8(st, f + 0x12);
            } else {
                U8(st, f + 0x38) = 8;
                S16(st, f + 0x36) = 0;
            }
        } else if (S8(st, f + 0x11) == 0x0A) {
            U8(st, f + 0x0E) = 2;
        } else if (S8(st, f + 0x11) == 9) {
            GAME_CALL(st, 0x004B3250u, cur(st));
            f = fig(st);
            U8(st, f + 0x11) = U8(st, f + 0x12);
        }
        break;
    case 0x0A:
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) > 0x12C) {
            /* @bin 0x004ACC7E grid offset 0x00BCA2D8 + x + 228 y */
            int32_t off = (int32_t) (U32(st, 0x00BCA2D8u) + (uint32_t) S16(st, f + 0x14)
                                     + (uint32_t) (S16(st, f + 0x16) * 228));
            int16_t h = S16(st, f + 0x5A);
            U8(st, 0x00A43EACu + (uint32_t) off) = 0;
            GAME_CALL(st, 0x00486600u, off);
            f = fig(st);
            U8(st, f + 0x38) = 0x0B;
            S16(st, f + 0x24) = S16(st, BUILDING_ADDR(h) + 0x26);
            S16(st, f + 0x26) = S16(st, BUILDING_ADDR(h) + 0x28);
        }
        break;
    case 0x0B:
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = fig(st);
        if (S8(st, f + 0x11) == 8) {
            U8(st, f + 0x0E) = 2;
            S16(st, BUILDING_ADDR(S16(st, f + 0x5A)) + 0x3C) += 0x32; /* @bin 0x004ACD19 */
        } else if (S8(st, f + 0x11) == 0x0A) {
            U8(st, f + 0x0E) = 2;
        } else if (S8(st, f + 0x11) == 9) {
            GAME_CALL(st, 0x004B3250u, cur(st));
            f = fig(st);
            U8(st, f + 0x11) = U8(st, f + 0x12);
        }
        break;
    default:
        U8(st, f + 0x38) = 8;
        break;
    }
    f = fig(st);
    U8(st, f + 0x01) += 1;
    /* @bin 0x004ACA9A 15 frames in state 0x0A, else 12 */
    if (U8(st, f + 0x01) >= (U8(st, f + 0x38) == 0x0A ? 15 : 12)) {
        U8(st, f + 0x01) = 0;
    }
    int32_t d = sprite_direction(st, S8(st, f + 0x11));
    uint8_t action = U8(st, f + 0x38);
    if (action == 0x0A) {
        S32(st, f + 0x04) = S32(st, 0x00F55E24u) + 8 * U8(st, f + 0x01) + d;
    } else if (action == 8 || action == 5) {
        S32(st, f + 0x04) = S32(st, 0x00F55E14u) + d;
    } else if (action == 0x0B) {
        S32(st, f + 0x04) = S32(st, 0x00F55E2Cu) + 8 * U8(st, f + 0x01) + d;
    } else if (action == 4) {
        int16_t t = S16(st, f + 0x36);
        S32(st, f + 0x04) = S32(st, 0x00F55E1Cu) + (t >= 8 ? 7 : t);
    } else {
        S32(st, f + 0x04) = S32(st, 0x00F55E14u) + 8 * U8(st, f + 0x01) + d;
    }
}

/* @bin 0x004ACFA2 a tile around the city point (C+0x47A0, C+0x47A4): base + rand() * 5 / 0x8000 */
static int32_t random_offset(game_state *st, uint32_t field)
{
    int32_t r = crt_rand(st) * 5;
    return (r / 0x8000) + S32(st, CURRENT_CITY(st) + field);
}

void figure_action_5B(game_state *st)
{
    /* @bin 0x004ACEB0 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x0C) = 0;
    S16(st, f + 0x42) = 0x180;
    int16_t home = S16(st, f + 0x5A);
    int32_t speed = S32(st, 0x007090ECu + 0x38u * U8(st, f + 0x0A));
    next_frame(st, f);
    uint32_t c = CURRENT_CITY(st);
    /* @bin 0x004ACF21 jump table 0x004AD230 for states 8 .. 11 */
    switch (U8(st, f + 0x38)) {
    case 8:
        U8(st, f + 0x72) = 1;
        S32(st, f + 0x164) = 10;
        S16(st, f + 0x24) = (int16_t) (S16(st, c + 0x47A0) + 2);
        S16(st, f + 0x26) = (int16_t) (S16(st, c + 0x47A4) + 2);
        U8(st, f + 0x38) = 0x0A;
        break;
    case 9: {
        U8(st, f + 0x72) = 0;
        if (S32(st, c + 0x47A0) == -1 || S32(st, c + 0x47A4) == -1) {
            U8(st, f + 0x0E) = 2;
            S32(st, f + 0x04) = 0;
            return;
        }
        int32_t x, y;
        do {
            x = random_offset(st, 0x47A0);
            y = random_offset(st, 0x47A4);
            f = fig(st);
        } while (x == S16(st, f + 0x14) && y == S16(st, f + 0x16));
        S16(st, f + 0x24) = (int16_t) x;
        S16(st, f + 0x26) = (int16_t) y;
        U8(st, f + 0x38) = 0x0A;
        break;
    }
    case 0x0A:
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = fig(st);
        if (S8(st, f + 0x11) == 8) {
            U8(st, f + 0x11) = U8(st, f + 0x12);
            U8(st, f + 0x38) = 0x0B;
        } else if (S8(st, f + 0x11) == 0x0A) {
            U8(st, f + 0x0E) = 2;
        } else if (S8(st, f + 0x11) == 9) {
            GAME_CALL(st, 0x004B3250u, cur(st));
            f = fig(st);
            U8(st, f + 0x11) = U8(st, f + 0x12);
            U8(st, f + 0x38) = 0x0B;
            S32(st, f + 0x164) = 0;
        }
        break;
    case 0x0B:
        S32(st, f + 0x164) -= 1;
        if (S32(st, f + 0x164) <= 0) {
            U8(st, f + 0x0E) = 2;
        } else {
            U8(st, f + 0x38) = 9;
        }
        break;
    default:
        U8(st, f + 0x38) = 9;
        U8(st, f + 0x72) = 1;
        break;
    }
    /* @bin 0x004AD102 sprite group (table 0x00F55BB4) and +0x0B by the home building's type
     * (jump tables 0x004AD240 / 0x004AD260) */
    uint32_t group;
    uint8_t carried;
    int16_t btype = S16(st, BUILDING_ADDR(home) + 0x10);
    switch (btype) {
    case 0x1E: case 0x22:
        group = 0xE6; carried = 0x10;
        break;
    case 0x1F: case 0x20: case 0x24:
        group = 0xA9; carried = 0x0F;
        break;
    case 0x21: case 0x23:
        group = 0xA7; carried = 0x11;
        break;
    case 0x33: case 0x35:
        group = 0x60; carried = 0x1D;
        break;
    case 0x3C: case 0x3D: case 0x3E: case 0x3F: case 0x40: case 0x41: case 0x42: case 0x43: case 0x44:
    case 0x45: case 0x97: case 0x9C:
        carried = 0x1B;
        /* @bin 0x004AD135 jump table 0x004AD32C */
        switch ((uint32_t) GAME_CALL(st, 0x004C5120u, home)) {
        case 0: group = 0xEC; break;
        case 1: group = 0xF9; break;
        case 3: group = 0xE8; break;
        case 4: group = 0xF7; break;
        default: group = 0xE2; break;
        }
        break;
    case 0xE2:
        group = 0x2A7; carried = 0x69;
        break;
    default:
        group = 0xEE; carried = 0x28;
        break;
    }
    f = fig(st);
    U8(st, f + 0x0B) = carried;
    int32_t d = sprite_direction(st, S8(st, f + 0x11));
    S32(st, f + 0x04) = S32(st, 0x00F55BB4u + 8u * group) + 8 * U8(st, f + 0x01) + d;
}

void figure_action_5F(game_state *st)
{
    /* @bin 0x004AF140 */
    uint32_t f = fig(st);
    U8(st, f + 0x65) = 0;
    U8(st, f + 0x76) = 1;
    S32(st, f + 0x180) = 0;
    if (U8(st, f + 0x01) >= 0x18) {
        U8(st, f + 0x01) = 0x18;
        S16(st, f + 0x36) += 1;
        /* @bin 0x004AF197 hold the last frame for 0x19 + (+0x92) ticks */
        if (S16(st, f + 0x36) >= (int32_t) U8(st, f + 0x92) + 0x19) {
            S16(st, f + 0x36) = 0;
            U8(st, f + 0x01) = 0;
        }
    } else {
        U8(st, f + 0x01) += 1;
        random_next_state(st); /* @bin 0x004AF1B1 the generator step 0x004D7090, inlined */
        U8(st, f + 0x92) = (uint8_t) (crt_rand(st) % 100);
    }
    S32(st, f + 0x04) = (int32_t) (U32(st, 0x00F56DFCu) + U8(st, f + 0x01));
}

void figure_action_61(game_state *st)
{
    /* @bin 0x004AF4C0 */
    U8(st, fig(st) + 0x0E) = 2;
}

void figure_action_62(game_state *st)
{
    /* @bin 0x004AF4E0 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    S32(st, f + 0x180) = S32(st, 0x00F55F54u);
    U8(st, f + 0x72) = 1;
    U8(st, f + 0x0C) = 0;
    S16(st, f + 0x42) = S16(st, f + 0x36);
    int16_t home = S16(st, f + 0x5A);
    next_frame(st, f);
    U8(st, f + 0x65) = 0;
    switch (U8(st, f + 0x38)) {
    case 8:
        U8(st, f + 0x65) = 1;
        U8(st, f + 0x01) = 0;
        if (road_near(st, BUILDING_ADDR(home)) == 0) {
            U8(st, fig(st) + 0x0E) = 2;
            break;
        }
        id = cur(st);
        f = FIGURE_ADDR(id);
        U8(st, f + 0x38) = 9;
        S16(st, f + 0x24) = S16(st, ROUTE_DEST_X);
        S16(st, f + 0x26) = S16(st, ROUTE_DEST_Y);
        move_to_dest(st, id, f);
        S16(st, fig(st) + 0x44) = 0;
        break;
    case 9:
        U8(st, f + 0x0C) = 1;
        U8(st, f + 0x65) = 1;
        if (GAME_CALL(st, 0x004B3FF0u, id, 1) != 1) {
            break;
        }
        id = cur(st);
        f = FIGURE_ADDR(id);
        if (S16(st, BUILDING_GRID + 2u * U32(st, f + 0x20)) == home) {
            U8(st, f + 0x0E) = 2;
            break;
        }
        U8(st, f + 0x38) = 0x0A;
        GAME_CALL(st, 0x004B1DF0u, id);
        S16(st, fig(st) + 0x44) = 0;
        break;
    case 0x0A:
        U8(st, f + 0x65) = 0;
        S16(st, f + 0x44) += 1;
        if (S16(st, f + 0x44) >= S16(st, f + 0x42)) {
            if (road_near(st, BUILDING_ADDR(home)) != 0) {
                /* @bin 0x004AF742 a road found: removed, without the sprite update */
                U8(st, fig(st) + 0x0E) = 2;
                return;
            }
            id = cur(st);
            U8(st, FIGURE_ADDR(id) + 0x0E) = 2;
        }
        GAME_CALL(st, 0x004B1EF0u, id, 1);
        break;
    default:
        break;
    }
    f = fig(st);
    int32_t d = sprite_direction(st, S8(st, f + 0x11) < 8 ? S8(st, f + 0x11) : S8(st, f + 0x12));
    if (U8(st, f + 0x38) == 4) {
        S32(st, f + 0x04) = (int32_t) (dying_frame(st, f) + U32(st, 0x00F56354u));
        return;
    }
    S32(st, f + 0x04) = S32(st, 0x00F56344u) + 8 * U8(st, f + 0x01) + d;
}

void figure_action_63(game_state *st)
{
    /* @bin 0x004AF7C0 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    /* @bin 0x004AF7DF terrain bits 0x10004 both set at the figure's tile */
    if ((U32(st, 0x00AF598Cu + 4u * U32(st, f + 0x20)) & 0x10004u) == 0x10004u) {
        GAME_CALL(st, 0x004BD250u, id);
        return;
    }
    int16_t r = S16(st, f + 0x60);
    next_frame(st, f);
    S32(st, f + 0x180) = 0;
    U8(st, f + 0x59) = 2;
    if (U8(st, 0x00BCBA31u + 0x90u * (uint32_t) r) != 0) { /* @bin 0x004AF830 */
        U8(st, f + 0x0E) = 2;
    }
    GAME_CALL0(st, 0x004991A0u);
    id = cur(st);
    f = FIGURE_ADDR(id);
    uint8_t action = U8(st, f + 0x38);
    int32_t dir;
    if (action == 5) {
        dir = S8(st, f + 0x13);
    } else if (S8(st, f + 0x11) < 8) {
        dir = S8(st, f + 0x11);
    } else {
        dir = S8(st, f + 0x12);
    }
    int32_t d = sprite_direction(st, dir);
    if (action == 5) {
        GAME_CALL(st, 0x0053B4A0u, id, 10, 2);
        f = fig(st);
        uint8_t a = U8(st, f + 0x6A);
        if (a < 0x0C) {
            S32(st, f + 0x04) = S32(st, SPRITE_DIRECTION) + S32(st, 0x00F56E34u);
        } else {
            S32(st, f + 0x04) = S32(st, SPRITE_DIRECTION) + ((int32_t) a - 0x0C) / 2 * 8 + S32(st, 0x00F56E34u);
        }
        return;
    }
    if (action == 4) {
        S32(st, f + 0x04) = (int32_t) (dying_frame(st, f) + U32(st, 0x00F56E2Cu));
        return;
    }
    if (S8(st, f + 0x11) == 0x0B) {
        S32(st, f + 0x04) = S32(st, 0x00F56E34u) + 8 * (U8(st, f + 0x01) >> 1) + d;
        return;
    }
    S32(st, f + 0x04) = S32(st, 0x00F56E24u) + 8 * U8(st, f + 0x01) + d;
}

#define ACTION_RUNNER(t) \
    static int32_t run_##t(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        figure_action_##t(st); \
        return 0; \
    }
ACTION_RUNNER(51)
ACTION_RUNNER(55)
ACTION_RUNNER(56)
ACTION_RUNNER(57)
ACTION_RUNNER(58)
ACTION_RUNNER(59)
ACTION_RUNNER(5A)
ACTION_RUNNER(5B)
ACTION_RUNNER(5F)
ACTION_RUNNER(60)
ACTION_RUNNER(61)
ACTION_RUNNER(62)
ACTION_RUNNER(63)
ACTION_RUNNER(6A)
ACTION_RUNNER(6B)
ACTION_RUNNER(6C)
ACTION_RUNNER(6D)

static int32_t run_level_next(game_state *st, const int32_t *a)
{
    (void) st;
    return pyramid_level_next(a[0]);
}

/* @bin 0x00561A36 sprintf(buffer, format, ...): the C runtime's, as far as its ported callers
 * use it (literal text and %d). A buffer outside the state (a caller's frame, passed as 0 by the
 * ports) is not written: only the length is returned. */
static int32_t run_sprintf(game_state *st, const int32_t *a)
{
    const uint8_t *fmt = state_ptr(st, (uint32_t) a[1], 1);
    char out[256];
    int32_t n = 0, arg = 2;
    if (!fmt) {
        state_unported(st, 0x00561A36u);
        return 0;
    }
    for (uint32_t i = 0;; i++) {
        const uint8_t *c = state_ptr(st, (uint32_t) a[1] + i, 1);
        if (!c || n >= (int32_t) sizeof(out) - 12) {
            state_unported(st, 0x00561A36u);
            return 0;
        }
        if (*c == 0) {
            break;
        }
        if (*c != '%') {
            out[n++] = (char) *c;
            continue;
        }
        const uint8_t *d = state_ptr(st, (uint32_t) a[1] + i + 1, 1);
        if (!d || *d != 'd' || arg >= 16) {
            state_unported(st, 0x00561A36u); /* other conversions: not ported */
            return 0;
        }
        i++;
        int64_t v = a[arg++];
        char digits[12];
        int k = 0;
        int neg = v < 0;
        if (neg) {
            v = -v;
        }
        do {
            digits[k++] = (char) ('0' + v % 10);
            v /= 10;
        } while (v != 0);
        if (neg) {
            out[n++] = '-';
        }
        while (k > 0) {
            out[n++] = digits[--k];
        }
    }
    uint8_t *dst = a[0] ? state_ptr(st, (uint32_t) a[0], (uint32_t) n + 1) : NULL;
    if (a[0] && !dst) {
        state_unported(st, 0x00561A36u); /* a buffer the port cannot see */
        return 0;
    }
    if (dst) {
        for (int32_t i = 0; i < n; i++) {
            dst[i] = (uint8_t) out[i];
        }
        dst[n] = 0;
    }
    return n;
}

static int32_t run_labor_assign(game_state *st, const int32_t *a)
{
    labor_assign(st, 0x00D05720u, a[0]);
    return 0;
}

static int32_t run_labor_arrived(game_state *st, const int32_t *a)
{
    labor_arrived(st, a[0]);
    return 0;
}

static int32_t run_labor_wanted(game_state *st, const int32_t *a)
{
    (void) a;
    return labor_wanted_anywhere(st, 0x00D05720u);
}

static int32_t run_find_enemy(game_state *st, const int32_t *a)
{
    return figure_find_enemy(st, a[0], a[1], a[2]);
}

static int32_t run_seek_enemy(game_state *st, const int32_t *a)
{
    return figure_seek_enemy(st, a[0]);
}

static int32_t run_effect_at(game_state *st, const int32_t *a)
{
    return monument_effect_at(st, a[0], a[1], a[2]);
}

static int32_t run_phase2(game_state *st, const int32_t *a)
{
    return monument_work_phase(st, a[2], 2);
}

static int32_t run_phase3(game_state *st, const int32_t *a)
{
    return monument_work_phase(st, a[2], 3);
}

static int32_t run_sledge_follow(game_state *st, const int32_t *a)
{
    sledge_follow(st, a[0], a[1], a[2], a[3]);
    return 0;
}

static int32_t run_sledge_ready(game_state *st, const int32_t *a)
{
    return laborer_sledge_ready(st, a[0]);
}

static int32_t run_sledge_leader(game_state *st, const int32_t *a)
{
    int32_t distance;
    return figure_sledge_leader(st, a[0], &distance);
}

const ported_function figd_ports[] = {
    /* 0x004A34F0 writes its second result through a pointer: ported for its callers only */
    {0x004A34F0u, "figure_sledge_leader (distance out)", 2, 32, 0, NULL, 0},
    {0x004A3540u, "figure_sledge_leader", 1, 32, 0, run_sledge_leader, 0},
    {0x004A8800u, "figure_action_51", 0, 0, 0, run_51, 0},
    {0x004A98B0u, "figure_action_55", 0, 0, 0, run_55, 0},
    {0x0048E6F0u, "figure_seek_enemy", 1, 32, 0, run_seek_enemy, 0},
    {0x0048E820u, "figure_find_enemy", 3, 32, 0, run_find_enemy, 0},
    {0x004A9730u, "laborer_sledge_ready", 1, 8, 0, run_sledge_ready, 0},
    {0x004B23C0u, "sledge_follow", 4, 0, 0, run_sledge_follow, 0},
    {0x004BC1E0u, "monument_effect_at", 3, 32, 0, run_effect_at, 0},
    {0x004BC2E0u, "monument_work_phase (2)", 3, 32, 0, run_phase2, 0},
    {0x004BC3B0u, "monument_work_phase (3)", 3, 32, 0, run_phase3, 0},
    {0x004BEEB0u, "labor_assign", 1, 0, 0x00D05720u, run_labor_assign, 0},
    {0x004BF920u, "labor_arrived", 1, 0, 0x00D05720u, run_labor_arrived, 0},
    {0x004BF9D0u, "labor_wanted_anywhere", 0, 8, 0x00D05720u, run_labor_wanted, 0},
    {0x004F3360u, "pyramid_level_next", 1, 32, 0, run_level_next, 0},
    /* 0x004F3370 returns the slot through two pointers: ported for its callers only */
    {0x004F3370u, "pyramid_block_slot", 4, 8, 0, NULL, 0},
    /* partial: the formats of its ported callers only */
    {0x00561A36u, "crt_sprintf", 2, 32, 0, run_sprintf, 1},
    {0x004AB980u, "figure_action_56", 0, 0, 0, run_56, 0},
    {0x004ABD50u, "figure_action_57", 0, 0, 0, run_57, 0},
    {0x004ABE40u, "figure_action_58", 0, 0, 0, run_58, 0},
    {0x004AC450u, "figure_action_59", 0, 0, 0, run_59, 0},
    {0x004AC9F0u, "figure_action_5A", 0, 0, 0, run_5A, 0},
    {0x004ACEB0u, "figure_action_5B", 0, 0, 0, run_5B, 0},
    {0x004AF140u, "figure_action_5F", 0, 0, 0, run_5F, 0},
    {0x004AF2C0u, "figure_action_60", 0, 0, 0, run_60, 0},
    {0x004AF4C0u, "figure_action_61", 0, 0, 0, run_61, 0},
    {0x004AF4E0u, "figure_action_62", 0, 0, 0, run_62, 0},
    {0x004AF7C0u, "figure_action_63", 0, 0, 0, run_63, 0},
    {0x004AFEB0u, "figure_action_6A", 0, 0, 0, run_6A, 0},
    {0x004B03B0u, "figure_action_6B", 0, 0, 0, run_6B, 0},
    {0x004B0760u, "figure_action_6C", 0, 0, 0, run_6C, 0},
    {0x004B0EC0u, "figure_action_6D", 0, 0, 0, run_6D, 0},
};
const int figd_ports_count = sizeof(figd_ports) / sizeof(figd_ports[0]);
