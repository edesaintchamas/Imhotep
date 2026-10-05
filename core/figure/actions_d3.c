/* Imhotep - figure actions, group D: types 0x6A .. 0x6D (frogs, locusts, the monument worker of
 * building type 0xE7, ... : 0x004AFEB0 .. 0x004B0EC0).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21). Every action works on the current figure
 * (0x00D056F8) and re-reads it after each call, as the original does. */
#include "figure/actions_d.h"

#include "figure/actions_d_local.h"
#include "random/random.h"
#include "state/ported.h"

#define PLAYER 0x00EAAC28u

void figure_action_6A(game_state *st)
{
    /* @bin 0x004AFEB0 a frog (debug texts "Trapped frog %d", "Frog %d reached house") */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x72) = 6;
    S32(st, f + 0x180) = 0;
    int32_t speed = S32(st, 0x007090ECu + 0x38u * U8(st, f + 0x0A));
    uint32_t hb = BUILDING_ADDR(S16(st, f + 0x5C)); /* the house it goes to */
    if (U8(st, hb) != 1 || S16(st, hb + 0x2E) != id || U8(st, hb + 0x05) == 0) {
        U8(st, f + 0x0E) = 2;
        return;
    }
    U8(st, f + 0x01) += 1;
    if (U8(st, f + 0x01) >= 9) {
        U8(st, f + 0x01) = 0;
    }
    switch (U8(st, f + 0x38)) {
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
        if (road_near(st, hb) == 0) {
            U8(st, fig(st) + 0x0E) = 2;
            break;
        }
        f = fig(st);
        U8(st, f + 0x38) = 9;
        S16(st, f + 0x24) = S16(st, ROUTE_DEST_X);
        S16(st, f + 0x26) = S16(st, ROUTE_DEST_Y);
        S16(st, f + 0x44) = 0;
        break;
    case 9: {
        U8(st, f + 0x65) = 0;
        random_next_state(st); /* @bin 0x004B0019 the generator step 0x004D7090, inlined */
        /* @bin 0x004B00E0 moves on half of the updates */
        if ((int32_t) (U32(st, 0x00D3B370u) % 100u) < 0x32) {
            GAME_CALL(st, 0x004A5DA0u, id, speed);
        }
        id = cur(st);
        f = FIGURE_ADDR(id);
        int8_t dir = S8(st, f + 0x11);
        if (dir == 9) {
            U8(st, f + 0x38) = 8;
            S16(st, f + 0x36) = 1;
            GAME_CALL(st, 0x004B3250u, id);
        } else if (dir == 0x0A) {
            /* @bin 0x004B014A sprintf + OutputDebugStringA: debugger output only */
            GAME_CALL(st, 0x00561A36u, 0, 0x005DF718, id);
            S16(st, hb + 0x2E) = 0;
            S16(st, hb + 0x20) = 0;
            U8(st, fig(st) + 0x0E) = 2;
        } else if (dir == 8) {
            U8(st, f + 0x38) = 0x0A;
            S16(st, f + 0x24) = S16(st, hb + 0x06);
            S16(st, f + 0x26) = S16(st, hb + 0x08);
            move_to_dest(st, id, f);
            S16(st, fig(st) + 0x44) = 0;
        }
        break;
    }
    case 0x0A:
        U8(st, f + 0x0C) = 1;
        U8(st, f + 0x65) = 1;
        if (GAME_CALL(st, 0x004B3FF0u, id, 1) == 1) {
            id = cur(st);
            U8(st, FIGURE_ADDR(id) + 0x0E) = 2;
            GAME_CALL(st, 0x00459020u, S32(st, PLAYER), S16(st, hb + 0x06), S16(st, hb + 0x08), S16(st, hb + 0x1C));
            /* @bin 0x004B0272 the house's countdown +0xF0: 0x0078D7A8 / 51 / 16, at least 1 */
            int32_t v = S32(st, 0x0078D7A8u) / 51;
            S16(st, hb + 0x1C) = 0;
            U8(st, hb + 0x87) = 2;
            S32(st, hb + 0xF0) = v / 16;
            if (v / 16 < 1) {
                S32(st, hb + 0xF0) = 1;
            }
            S16(st, hb + 0x2E) = 0;
            /* @bin 0x004B02D3 sprintf + OutputDebugStringA: debugger output only */
            GAME_CALL(st, 0x00561A36u, 0, 0x005DF700, cur(st));
        }
        f = fig(st);
        U8(st, f + 0x65) = U8(st, f + 0x40) != 0 ? 1 : 0;
        break;
    default:
        U8(st, f + 0x38) = 8;
        break;
    }
    f = fig(st);
    int32_t d = sprite_direction(st, S8(st, f + 0x11));
    if (U8(st, f + 0x38) == 4) {
        S32(st, f + 0x04) = (int32_t) (dying_frame(st, f) + U32(st, 0x00F57114u));
        return;
    }
    S32(st, f + 0x04) = S32(st, 0x00F570FCu) + 8 * U8(st, f + 0x01) + d;
}

void figure_action_6B(game_state *st)
{
    /* @bin 0x004B03B0 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x72) = 0x0B;
    S32(st, f + 0x180) = 0;
    int32_t speed = S32(st, 0x007090ECu + 0x38u * U8(st, f + 0x0A));
    uint32_t hb = BUILDING_ADDR(S16(st, f + 0x5C));
    if (U8(st, hb) != 1) {
        U8(st, f + 0x0E) = 2;
        return;
    }
    U8(st, f + 0x01) += 1;
    if (U8(st, f + 0x01) >= 6) {
        U8(st, f + 0x01) = 0;
    }
    switch (U8(st, f + 0x38)) {
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
        GAME_CALL(st, 0x004B2D20u, id);
        f = fig(st);
        if (S16(st, f + 0x3A) > 0) {
            U8(st, f + 0x38) = 9;
        } else {
            U8(st, f + 0x0E) = 2;
        }
        break;
    case 9: {
        U8(st, f + 0x65) = 0;
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        id = cur(st);
        f = FIGURE_ADDR(id);
        int8_t dir = S8(st, f + 0x11);
        if (dir == 9) {
            U8(st, f + 0x38) = 8;
            S16(st, f + 0x36) = 1;
            GAME_CALL(st, 0x004B3250u, id);
        } else if (dir == 0x0A) {
            S16(st, hb + 0x2E) = 0;
            S16(st, hb + 0x20) = 0;
            U8(st, f + 0x0E) = 2;
        } else if (dir == 8) {
            U8(st, f + 0x38) = 0x0A;
            S16(st, f + 0x24) = S16(st, hb + 0x06);
            S16(st, f + 0x26) = S16(st, hb + 0x08);
        }
        break;
    }
    case 0x0A: {
        uint32_t b = BUILDING_ADDR(S16(st, f + 0x5C));
        int32_t g = S32(st, 0x0078D79Cu);
        /* @bin 0x004B0576 +0x54 at most 0x0078D79C - 1000, at least 0 */
        if (S16(st, b + 0x54) > g - 1000) {
            S16(st, b + 0x54) = (int16_t) (g - 1000);
        }
        if (S16(st, b + 0x54) < 0) {
            S16(st, b + 0x54) = 0;
        }
        if (g > 1000) {
            break;
        }
        U8(st, f + 0x38) = 0x0B;
        random_next_state(st); /* @bin 0x004B05A3 the generator step 0x004D7090, inlined */
        /* @bin 0x004B0651 a point on the map diagonal: map size 0x00BCA2D0 * 3/8 (* 7/8), minus
         * a random part of size / 4 */
        int32_t size = S32(st, 0x00BCA2D0u);
        int32_t r = (int32_t) U32(st, 0x00D3B370u) % (size / 4);
        S16(st, f + 0x24) = (int16_t) (size * 3 / 8 - r + 2);
        S16(st, f + 0x26) = (int16_t) (size * 7 / 8 - r - 2);
        S16(st, f + 0x44) = 0;
        break;
    }
    case 0x0B: {
        U8(st, f + 0x65) = 0;
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = fig(st);
        int8_t dir = S8(st, f + 0x11);
        if (dir == 9 || dir == 0x0A || dir == 8) {
            U8(st, f + 0x0E) = 2;
        }
        break;
    }
    default:
        U8(st, f + 0x38) = 8;
        break;
    }
    f = fig(st);
    uint32_t base = U8(st, f + 0x38) == 0x0A ? 0x00F5714Cu : 0x00F5713Cu;
    S32(st, f + 0x04) = (int32_t) (U32(st, base) + U8(st, f + 0x01));
}

/* @bin 0x004B0B31 the monument types worked by type 0x6C */
static int monument_6c(int16_t t)
{
    return t == 0xE5 || t == 0xEA || t == 0xEB || t == 0xEC;
}

int32_t monument_work_phase(game_state *st, int32_t id, int32_t phase)
{
    /* @bin 0x004BC2E0 (phase 2) / 0x004BC3B0 (phase 3): the first monument of types 0xE5, 0xEA,
     * 0xEB, 0xEC at work phase +0x60 that takes the worker id (0x00559110 on the monument's
     * object, committing) */
    int32_t found = 0;
    for (int32_t i = 1; i < BUILDING_COUNT && found == 0; i++) {
        uint32_t b = BUILDING_ADDR(i);
        if (U8(st, b) != 1) {
            continue;
        }
        uint32_t o;
        switch (S16(st, b + 0x10)) {
        case 0xE5: o = 0x006F9320u; break;
        case 0xEA: o = 0x006F5648u; break;
        case 0xEB: o = 0x006DE9E0u; break;
        case 0xEC: o = 0x006E9188u; break;
        default: continue;
        }
        if (S32(st, b + 0x60) == phase && (int8_t) GAME_CALLT(st, o, 0x00559110u, i, id, 1) != 0) {
            found = i;
        }
    }
    return found;
}

void figure_action_6C(game_state *st)
{
    /* @bin 0x004B0760 a worker of building type 0xE7 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    next_frame(st, f);
    U8(st, f + 0x72) = 1;
    U8(st, f + 0x0C) = 0;
    S16(st, f + 0x42) = 0x200;
    int32_t speed = S32(st, 0x007090ECu + 0x38u * U8(st, f + 0x0A));
    uint32_t hb = BUILDING_ADDR(S16(st, f + 0x5A));
    if (U8(st, hb) != 1 || S16(st, hb + 0x10) != 0xE7) {
        U8(st, f + 0x0E) = 2;
        return;
    }
    /* @bin 0x004B0807 jump table 0x004B0E8C for states 4 .. 13; other states: no change */
    switch (U8(st, f + 0x38)) {
    case 4:
        GAME_CALL(st, 0x004B1D50u, id);
        f = fig(st);
        if (U8(st, f + 0x0E) == 2) {
            uint32_t b = BUILDING_ADDR(S16(st, f + 0x5A));
            U8(st, b + 0xA6) -= 1;
            S32(st, b + 0x5C) = 0;
        }
        break;
    case 5:
        U8(st, f + 0x65) = 0;
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
        if (U8(st, f + 0x65) != 0) {
            S16(st, f + 0x36) += 1;
            if (S16(st, f + 0x36) > 0x0A) {
                U8(st, f + 0x65) = 0;
            }
            break;
        }
        S16(st, f + 0x36) += 1;
        random_next_state(st); /* @bin 0x004B089B the generator step 0x004D7090, inlined */
        if (S16(st, f + 0x36) > (int32_t) (U32(st, 0x00D3B370u) % 30u) + 30) {
            /* @bin 0x004B0994 0x004BC2E0 and 0x004BC3B0 (their x, y arguments are unused) */
            if (monument_work_phase(st, id, 2) != 0 || monument_work_phase(st, cur(st), 3) != 0) {
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
            U8(st, f + 0x38) = 0x0D;
            S16(st, f + 0x24) = S16(st, ROUTE_DEST_X);
            S16(st, f + 0x26) = S16(st, ROUTE_DEST_Y);
            GAME_CALL(st, 0x004B3250u, id);
            S16(st, fig(st) + 0x44) = 0;
        }
        GAME_CALL(st, 0x004B1EF0u, cur(st), 1);
        break;
    case 9: {
        U8(st, f + 0x65) = 0;
        U8(st, f + 0x72) = 3;
        int16_t t = S16(st, BUILDING_GRID + 2u * U32(st, f + 0x28));
        if (t != 0 && monument_6c(S16(st, BUILDING_ADDR(t) + 0x10))) {
            U8(st, f + 0x72) = 7;
        }
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = fig(st);
        int8_t dir = S8(st, f + 0x11);
        if (dir == 8) {
            if (t != 0 && monument_6c(S16(st, BUILDING_ADDR(t) + 0x10))) {
                U8(st, f + 0x38) = 0x0C;
                S16(st, f + 0x36) = 0;
            } else {
                U8(st, f + 0x38) = 0x0D;
            }
        } else if (dir == 0x0A) {
            U8(st, f + 0x38) = 8;
            S16(st, f + 0x36) = 0;
        } else if (dir == 9) {
            GAME_CALL(st, 0x004B3250u, cur(st));
        }
        break;
    }
    case 0x0C: {
        U8(st, f + 0x65) = 0;
        int16_t t = S16(st, BUILDING_GRID + 2u * U32(st, f + 0x28));
        if (t == 0) {
            U8(st, f + 0x38) = 8;
            S16(st, f + 0x36) = 0;
            break;
        }
        uint32_t h = BUILDING_ADDR(S16(st, f + 0x5A));
        if (U8(st, h) != 1) {
            U8(st, f + 0x0E) = 2;
            break;
        }
        /* @bin 0x004B0C88 work on the monument (thiscall on its object; 0xEC uses 0xEA's, as
         * in the original) */
        switch (S16(st, BUILDING_ADDR(t) + 0x10)) {
        case 0xE5:
            GAME_CALLT(st, 0x006F9320u, 0x00547E20u, t, id, 0x0D);
            break;
        case 0xEA: case 0xEC:
            GAME_CALLT(st, 0x006F5648u, 0x00547E20u, t, id, 0x0D);
            break;
        case 0xEB:
            GAME_CALLT(st, 0x006DE9E0u, 0x00547E20u, t, id, 0x0D);
            break;
        default:
            U8(st, f + 0x38) = 0x0D;
            S16(st, f + 0x24) = S16(st, h + 0x26);
            S16(st, f + 0x26) = S16(st, h + 0x28);
            break;
        }
        break;
    }
    case 0x0D: {
        U8(st, f + 0x65) = 0;
        U8(st, f + 0x72) = 7;
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = fig(st);
        int8_t dir = S8(st, f + 0x11);
        if (dir == 8 || dir == 0x0A || dir == 9) {
            if (dir == 9) {
                GAME_CALL(st, 0x004B3250u, cur(st));
            }
            f = fig(st);
            U8(st, f + 0x0E) = 2;
            U8(st, BUILDING_ADDR(S16(st, f + 0x5A)) + 0xA6) -= 1;
        }
        break;
    }
    default:
        break;
    }
    f = fig(st);
    int32_t d = sprite_direction(st, S8(st, f + 0x11));
    switch (U8(st, f + 0x38)) {
    case 4:
        S32(st, f + 0x04) = (int32_t) (dying_frame(st, f) + U32(st, 0x00F5716Cu));
        return;
    case 0x0C:
        S32(st, f + 0x04) = S32(st, 0x00F5717Cu) + 8 * (U8(st, f + 0x01) >> 1) + d;
        return;
    default:
        S32(st, f + 0x04) = S32(st, 0x00F57164u) + 8 * U8(st, f + 0x01) + d;
        return;
    }
}

void figure_action_6D(game_state *st)
{
    /* @bin 0x004B0EC0 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x72) = 6;
    S32(st, f + 0x180) = 0;
    U8(st, f + 0x01) += 1;
    if (U8(st, f + 0x01) >= 0x18) {
        U8(st, f + 0x01) = 0;
    }
    switch (U8(st, f + 0x38)) {
    case 4:
        GAME_CALL(st, 0x004B1D50u, id);
        break;
    case 5: {
        uint32_t o = FIGURE_ADDR(S16(st, f + 0x90));
        if (U8(st, o + 0x0E) != 1 || U8(st, o + 0x38) == 4) {
            /* @bin 0x004B0F46 its target is gone: a new one of the type, while fewer than
             * 0x0078D798 live */
            int32_t n = 0;
            for (int32_t i = 1; i < FIGURE_COUNT; i++) {
                uint32_t g = FIGURE_ADDR(i);
                if (U8(st, g + 0x0E) != 0 && U8(st, g + 0x0A) == 0x6D) {
                    n++;
                }
            }
            if (n < S32(st, 0x0078D798u)) {
                int32_t nid = GAME_CALL(st, 0x004B4800u, S32(st, PLAYER), 0x6D, S16(st, o + 0x14), S16(st, o + 0x16), 0);
                uint32_t nf = FIGURE_ADDR(nid);
                S16(st, nf + 0x5A) = S16(st, fig(st) + 0x5A);
                U8(st, nf + 0x38) = 9;
                random_next_state(st); /* @bin 0x004B0FA8 the generator step 0x004D7090, inlined */
                S16(st, nf + 0x36) = (int16_t) (U32(st, 0x00D3B370u) % 50u + 0x19);
                GAME_CALL(st, 0x004B1DF0u, nid);
                S16(st, nf + 0x44) = 0;
                id = cur(st);
            }
        }
        GAME_CALL(st, 0x004B17D0u, id);
        break;
    }
    case 9:
        U8(st, f + 0x65) = 0;
        S16(st, f + 0x44) += 1;
        if (S16(st, f + 0x44) >= S16(st, f + 0x42)) {
            if (GAME_CALL(st, 0x0047FE60u, S16(st, f + 0x14), S16(st, f + 0x16), 1, 6) == 0) {
                U8(st, fig(st) + 0x0E) = 2;
            }
            id = cur(st);
        }
        if ((U8(st, FIGURE_ADDR(id) + 0x01) & 1) == 0) {
            GAME_CALL(st, 0x004B1EF0u, id, 1);
        }
        break;
    default:
        break;
    }
    f = fig(st);
    int32_t d = sprite_direction(st, S8(st, f + 0x11));
    switch (U8(st, f + 0x38)) {
    case 4: {
        int16_t v = S16(st, f + 0x36);
        int32_t k = v >= 0x10 ? 7 : v / 2;
        S32(st, f + 0x04) = S32(st, 0x00F57194u) + 8 * k + d;
        return;
    }
    case 5: {
        /* @bin 0x004B11B1 the attack direction +0x13, not wrapped */
        int32_t a = S8(st, f + 0x13) - S32(st, VIEW_ORIENTATION);
        S32(st, SPRITE_DIRECTION) = a;
        S32(st, f + 0x04) = S32(st, 0x00F5719Cu) + 8 * (U8(st, f + 0x01) >> 1) + a;
        return;
    }
    case 9:
        S32(st, f + 0x04) = S32(st, 0x00F5718Cu) + 8 * (U8(st, f + 0x01) >> 1) + d;
        return;
    default:
        return;
    }
}
