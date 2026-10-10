/* Imhotep - figure actions, group D: types 0x6A .. 0x6D (frogs, locusts, the monument worker of
 * building type 0xE7, ... : 0x004AFEB0 .. 0x004B0EC0).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21). Every action works on the current figure
 * (0x00D056F8) and re-reads it after each call, as the original does. */
#include "figure/actions_d.h"

#include "figure/actions_d_local.h"
#include "random/random.h"
#include "state/layout.h"
#include "state/ported.h"


void figure_action_6A(game_state *st)
{
    /* @bin 0x004AFEB0 a frog (debug texts "Trapped frog %d", "Frog %d reached house") */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_TERRAIN_USAGE) = 6;
    S32(st, f + FIG_U180) = 0;
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    uint32_t hb = BUILDING_ADDR(S16(st, f + FIG_TARGET_HOUSE)); /* the house it goes to */
    if (U8(st, hb + BLD_STATE) != 1 || S16(st, hb + BLD_IMMIGRANT) != id || U8(st, hb + BLD_HOUSE_SIZE) == 0) {
        U8(st, f + FIG_STATE) = 2;
        return;
    }
    U8(st, f + FIG_ANIM_FRAME) += 1;
    if (U8(st, f + FIG_ANIM_FRAME) >= 9) {
        U8(st, f + FIG_ANIM_FRAME) = 0;
    }
    switch (U8(st, f + FIG_ACTION)) {
    case 4:
        GAME_CALL(st, FN_FIGURE_DYING, id);
        break;
    case 8:
        U8(st, f + FIG_INVISIBLE) = 1;
        U8(st, f + FIG_ANIM_FRAME) = 0;
        S16(st, f + FIG_WAIT_TICKS) -= 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0) {
            break;
        }
        if (road_near(st, hb) == 0) {
            U8(st, fig(st) + FIG_STATE) = 2;
            break;
        }
        f = fig(st);
        U8(st, f + FIG_ACTION) = 9;
        S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
        S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
        S16(st, f + FIG_ROAM_LENGTH) = 0;
        break;
    case 9: {
        U8(st, f + FIG_INVISIBLE) = 0;
        random_next_state(st); /* @bin 0x004B0019 the generator step 0x004D7090, inlined */
        /* @bin 0x004B00E0 moves on half of the updates */
        if ((int32_t) (U32(st, G_R_1_15) % 100u) < 0x32) {
            GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        }
        id = cur(st);
        f = FIGURE_ADDR(id);
        int8_t dir = S8(st, f + FIG_DIRECTION);
        if (dir == 9) {
            U8(st, f + FIG_ACTION) = 8;
            S16(st, f + FIG_WAIT_TICKS) = 1;
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        } else if (dir == 0x0A) {
            /* @bin 0x004B014A sprintf + OutputDebugStringA: debugger output only */
            GAME_CALL(st, FN_CRT_SPRINTF, 0, G_STR_DEBUG_5DF718, id);
            S16(st, hb + BLD_IMMIGRANT) = 0;
            S16(st, hb + BLD_DISTANCE) = 0;
            U8(st, fig(st) + FIG_STATE) = 2;
        } else if (dir == 8) {
            U8(st, f + FIG_ACTION) = 0x0A;
            S16(st, f + FIG_DEST_X) = S16(st, hb + BLD_X);
            S16(st, f + FIG_DEST_Y) = S16(st, hb + BLD_Y);
            move_to_dest(st, id, f);
            S16(st, fig(st) + FIG_ROAM_LENGTH) = 0;
        }
        break;
    }
    case 0x0A:
        U8(st, f + FIG_U0C) = 1;
        U8(st, f + FIG_INVISIBLE) = 1;
        if (GAME_CALL(st, FN_FIGURE_STEP_STRAIGHT, id, 1) == 1) {
            id = cur(st);
            U8(st, FIGURE_ADDR(id) + FIG_STATE) = 2;
            GAME_CALL(st, FN_SPAWN_FIGURE_3, S32(st, G_PLAYER), S16(st, hb + BLD_X), S16(st, hb + BLD_Y), S16(st, hb + BLD_POPULATION));
            /* @bin 0x004B0272 the house's countdown +0xF0: 0x0078D7A8 / 51 / 16, at least 1 */
            int32_t v = S32(st, G_FROGS) / 51;
            S16(st, hb + BLD_POPULATION) = 0;
            U8(st, hb + BLD_NO_ROOM) = 2;
            S32(st, hb + BLD_UF0) = v / 16;
            if (v / 16 < 1) {
                S32(st, hb + BLD_UF0) = 1;
            }
            S16(st, hb + BLD_IMMIGRANT) = 0;
            /* @bin 0x004B02D3 sprintf + OutputDebugStringA: debugger output only */
            GAME_CALL(st, FN_CRT_SPRINTF, 0, G_STR_DEBUG_5DF700, cur(st));
        }
        f = fig(st);
        U8(st, f + FIG_INVISIBLE) = U8(st, f + FIG_IN_BUILDING_WAIT) != 0 ? 1 : 0;
        break;
    default:
        U8(st, f + FIG_ACTION) = 8;
        break;
    }
    f = fig(st);
    int32_t d = sprite_direction(st, S8(st, f + FIG_DIRECTION));
    if (U8(st, f + FIG_ACTION) == 4) {
        S32(st, f + FIG_U04) = (int32_t) (dying_frame(st, f) + U32(st, G_IMAGE_GROUP_684));
        return;
    }
    S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_681) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
}

void figure_action_6B(game_state *st)
{
    /* @bin 0x004B03B0 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_TERRAIN_USAGE) = 0x0B;
    S32(st, f + FIG_U180) = 0;
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    uint32_t hb = BUILDING_ADDR(S16(st, f + FIG_TARGET_HOUSE));
    if (U8(st, hb + BLD_STATE) != 1) {
        U8(st, f + FIG_STATE) = 2;
        return;
    }
    U8(st, f + FIG_ANIM_FRAME) += 1;
    if (U8(st, f + FIG_ANIM_FRAME) >= 6) {
        U8(st, f + FIG_ANIM_FRAME) = 0;
    }
    switch (U8(st, f + FIG_ACTION)) {
    case 4:
        GAME_CALL(st, FN_FIGURE_DYING, id);
        break;
    case 8:
        U8(st, f + FIG_INVISIBLE) = 1;
        U8(st, f + FIG_ANIM_FRAME) = 0;
        S16(st, f + FIG_WAIT_TICKS) -= 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0) {
            break;
        }
        GAME_CALL(st, FN_FIGURE_FIND_ROUTE, id);
        f = fig(st);
        if (S16(st, f + FIG_PATH_ID) > 0) {
            U8(st, f + FIG_ACTION) = 9;
        } else {
            U8(st, f + FIG_STATE) = 2;
        }
        break;
    case 9: {
        U8(st, f + FIG_INVISIBLE) = 0;
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        id = cur(st);
        f = FIGURE_ADDR(id);
        int8_t dir = S8(st, f + FIG_DIRECTION);
        if (dir == 9) {
            U8(st, f + FIG_ACTION) = 8;
            S16(st, f + FIG_WAIT_TICKS) = 1;
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        } else if (dir == 0x0A) {
            S16(st, hb + BLD_IMMIGRANT) = 0;
            S16(st, hb + BLD_DISTANCE) = 0;
            U8(st, f + FIG_STATE) = 2;
        } else if (dir == 8) {
            U8(st, f + FIG_ACTION) = 0x0A;
            S16(st, f + FIG_DEST_X) = S16(st, hb + BLD_X);
            S16(st, f + FIG_DEST_Y) = S16(st, hb + BLD_Y);
        }
        break;
    }
    case 0x0A: {
        uint32_t b = BUILDING_ADDR(S16(st, f + FIG_TARGET_HOUSE));
        int32_t g = S32(st, G_LOCUSTS);
        /* @bin 0x004B0576 +0x54 at most 0x0078D79C - 1000, at least 0 */
        if (S16(st, b + BLD_U54) > g - 1000) {
            S16(st, b + BLD_U54) = (int16_t) (g - 1000);
        }
        if (S16(st, b + BLD_U54) < 0) {
            S16(st, b + BLD_U54) = 0;
        }
        if (g > 1000) {
            break;
        }
        U8(st, f + FIG_ACTION) = 0x0B;
        random_next_state(st); /* @bin 0x004B05A3 the generator step 0x004D7090, inlined */
        /* @bin 0x004B0651 a point on the map diagonal: map size 0x00BCA2D0 * 3/8 (* 7/8), minus
         * a random part of size / 4 */
        int32_t size = S32(st, G_MAP_WIDTH);
        int32_t r = (int32_t) U32(st, G_R_1_15) % (size / 4);
        S16(st, f + FIG_DEST_X) = (int16_t) (size * 3 / 8 - r + 2);
        S16(st, f + FIG_DEST_Y) = (int16_t) (size * 7 / 8 - r - 2);
        S16(st, f + FIG_ROAM_LENGTH) = 0;
        break;
    }
    case 0x0B: {
        U8(st, f + FIG_INVISIBLE) = 0;
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = fig(st);
        int8_t dir = S8(st, f + FIG_DIRECTION);
        if (dir == 9 || dir == 0x0A || dir == 8) {
            U8(st, f + FIG_STATE) = 2;
        }
        break;
    }
    default:
        U8(st, f + FIG_ACTION) = 8;
        break;
    }
    f = fig(st);
    uint32_t base = U8(st, f + FIG_ACTION) == 0x0A ? G_IMAGE_GROUP_691 : G_IMAGE_GROUP_689;
    S32(st, f + FIG_U04) = (int32_t) (U32(st, base) + U8(st, f + FIG_ANIM_FRAME));
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
        if (U8(st, b + BLD_STATE) != 1) {
            continue;
        }
        uint32_t o;
        switch (S16(st, b + BLD_TYPE)) {
        case 0xE5: o = G_MONUMENT_E5; break;
        case 0xEA: o = G_MONUMENT_EA; break;
        case 0xEB: o = G_MONUMENT_EB; break;
        case 0xEC: o = G_MONUMENT_EC; break;
        default: continue;
        }
        if (S32(st, b + BLD_MON_ANNOUNCED_60) == phase && (int8_t) GAME_CALLT(st, o, FN_TOMB_METHOD_2C, i, id, 1) != 0) {
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
    U8(st, f + FIG_TERRAIN_USAGE) = 1;
    U8(st, f + FIG_U0C) = 0;
    S16(st, f + FIG_MAX_ROAM_LENGTH) = 0x200;
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    uint32_t hb = BUILDING_ADDR(S16(st, f + FIG_HOME));
    if (U8(st, hb + BLD_STATE) != 1 || S16(st, hb + BLD_TYPE) != 0xE7) {
        U8(st, f + FIG_STATE) = 2;
        return;
    }
    /* @bin 0x004B0807 jump table 0x004B0E8C for states 4 .. 13; other states: no change */
    switch (U8(st, f + FIG_ACTION)) {
    case 4:
        GAME_CALL(st, FN_FIGURE_DYING, id);
        f = fig(st);
        if (U8(st, f + FIG_STATE) == 2) {
            uint32_t b = BUILDING_ADDR(S16(st, f + FIG_HOME));
            U8(st, b + BLD_SHOWS) -= 1;
            S32(st, b + BLD_U5C) = 0;
        }
        break;
    case 5:
        U8(st, f + FIG_INVISIBLE) = 0;
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        break;
    case 6:
        S32(st, f + FIG_U04) = 0;
        break;
    case 7:
        U8(st, f + FIG_ANIM_FRAME) = 0;
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0x64) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
            if ((int8_t) GAME_CALL(st, FN_FIGURE_MAY_LEAVE, id) == 0) {
                U8(st, fig(st) + FIG_ACTION) = 8;
            }
        }
        break;
    case 8:
        if (U8(st, f + FIG_INVISIBLE) != 0) {
            S16(st, f + FIG_WAIT_TICKS) += 1;
            if (S16(st, f + FIG_WAIT_TICKS) > 0x0A) {
                U8(st, f + FIG_INVISIBLE) = 0;
            }
            break;
        }
        S16(st, f + FIG_WAIT_TICKS) += 1;
        random_next_state(st); /* @bin 0x004B089B the generator step 0x004D7090, inlined */
        if (S16(st, f + FIG_WAIT_TICKS) > (int32_t) (U32(st, G_R_1_15) % 30u) + 30) {
            /* @bin 0x004B0994 0x004BC2E0 and 0x004BC3B0 (their x, y arguments are unused) */
            if (monument_work_phase(st, id, 2) != 0 || monument_work_phase(st, cur(st), 3) != 0) {
                U8(st, fig(st) + FIG_ACTION) = 9;
                GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, cur(st));
                S16(st, fig(st) + FIG_ROAM_LENGTH) = 0;
            }
            S16(st, fig(st) + FIG_WAIT_TICKS) = 0;
        }
        f = fig(st);
        if (U8(st, f + FIG_ACTION) != 8) {
            U8(st, f + FIG_ANIM_FRAME) = 0;
            break;
        }
        S16(st, f + FIG_ROAM_LENGTH) += 1;
        if (S16(st, f + FIG_ROAM_LENGTH) >= S16(st, f + FIG_MAX_ROAM_LENGTH)) {
            if (road_near(st, hb) == 0) {
                id = cur(st);
                U8(st, FIGURE_ADDR(id) + FIG_ACTION) = 4;
                GAME_CALL(st, FN_FIGURE_ROAM, id, 1);
                break;
            }
            id = cur(st);
            f = FIGURE_ADDR(id);
            U8(st, f + FIG_ACTION) = 0x0D;
            S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
            S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            S16(st, fig(st) + FIG_ROAM_LENGTH) = 0;
        }
        GAME_CALL(st, FN_FIGURE_ROAM, cur(st), 1);
        break;
    case 9: {
        U8(st, f + FIG_INVISIBLE) = 0;
        U8(st, f + FIG_TERRAIN_USAGE) = 3;
        int16_t t = S16(st, G_BUILDING_GRID + 2u * U32(st, f + FIG_U28));
        if (t != 0 && monument_6c(S16(st, BUILDING_ADDR(t) + BLD_TYPE))) {
            U8(st, f + FIG_TERRAIN_USAGE) = 7;
        }
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = fig(st);
        int8_t dir = S8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            if (t != 0 && monument_6c(S16(st, BUILDING_ADDR(t) + BLD_TYPE))) {
                U8(st, f + FIG_ACTION) = 0x0C;
                S16(st, f + FIG_WAIT_TICKS) = 0;
            } else {
                U8(st, f + FIG_ACTION) = 0x0D;
            }
        } else if (dir == 0x0A) {
            U8(st, f + FIG_ACTION) = 8;
            S16(st, f + FIG_WAIT_TICKS) = 0;
        } else if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, cur(st));
        }
        break;
    }
    case 0x0C: {
        U8(st, f + FIG_INVISIBLE) = 0;
        int16_t t = S16(st, G_BUILDING_GRID + 2u * U32(st, f + FIG_U28));
        if (t == 0) {
            U8(st, f + FIG_ACTION) = 8;
            S16(st, f + FIG_WAIT_TICKS) = 0;
            break;
        }
        uint32_t h = BUILDING_ADDR(S16(st, f + FIG_HOME));
        if (U8(st, h + BLD_STATE) != 1) {
            U8(st, f + FIG_STATE) = 2;
            break;
        }
        /* @bin 0x004B0C88 work on the monument (thiscall on its object; 0xEC uses 0xEA's, as
         * in the original) */
        switch (S16(st, BUILDING_ADDR(t) + BLD_TYPE)) {
        case 0xE5:
            GAME_CALLT(st, G_MONUMENT_E5, FN_TOMB_WORK, t, id, 0x0D);
            break;
        case 0xEA: case 0xEC:
            GAME_CALLT(st, G_MONUMENT_EA, FN_TOMB_WORK, t, id, 0x0D);
            break;
        case 0xEB:
            GAME_CALLT(st, G_MONUMENT_EB, FN_TOMB_WORK, t, id, 0x0D);
            break;
        default:
            U8(st, f + FIG_ACTION) = 0x0D;
            S16(st, f + FIG_DEST_X) = S16(st, h + BLD_ROAD_ACCESS_X);
            S16(st, f + FIG_DEST_Y) = S16(st, h + BLD_ROAD_ACCESS_Y);
            break;
        }
        break;
    }
    case 0x0D: {
        U8(st, f + FIG_INVISIBLE) = 0;
        U8(st, f + FIG_TERRAIN_USAGE) = 7;
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = fig(st);
        int8_t dir = S8(st, f + FIG_DIRECTION);
        if (dir == 8 || dir == 0x0A || dir == 9) {
            if (dir == 9) {
                GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, cur(st));
            }
            f = fig(st);
            U8(st, f + FIG_STATE) = 2;
            U8(st, BUILDING_ADDR(S16(st, f + FIG_HOME)) + BLD_SHOWS) -= 1;
        }
        break;
    }
    default:
        break;
    }
    f = fig(st);
    int32_t d = sprite_direction(st, S8(st, f + FIG_DIRECTION));
    switch (U8(st, f + FIG_ACTION)) {
    case 4:
        S32(st, f + FIG_U04) = (int32_t) (dying_frame(st, f) + U32(st, G_IMAGE_GROUP_695));
        return;
    case 0x0C:
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_697) + 8 * (U8(st, f + FIG_ANIM_FRAME) >> 1) + d;
        return;
    default:
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_694) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
        return;
    }
}

void figure_action_6D(game_state *st)
{
    /* @bin 0x004B0EC0 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_TERRAIN_USAGE) = 6;
    S32(st, f + FIG_U180) = 0;
    U8(st, f + FIG_ANIM_FRAME) += 1;
    if (U8(st, f + FIG_ANIM_FRAME) >= 0x18) {
        U8(st, f + FIG_ANIM_FRAME) = 0;
    }
    switch (U8(st, f + FIG_ACTION)) {
    case 4:
        GAME_CALL(st, FN_FIGURE_DYING, id);
        break;
    case 5: {
        uint32_t o = FIGURE_ADDR(S16(st, f + FIG_OPPONENT));
        if (U8(st, o + FIG_STATE) != 1 || U8(st, o + FIG_ACTION) == 4) {
            /* @bin 0x004B0F46 its target is gone: a new one of the type, while fewer than
             * 0x0078D798 live */
            int32_t n = 0;
            for (int32_t i = 1; i < FIGURE_COUNT; i++) {
                uint32_t g = FIGURE_ADDR(i);
                if (U8(st, g + FIG_STATE) != 0 && U8(st, g + FIG_TYPE) == 0x6D) {
                    n++;
                }
            }
            if (n < S32(st, G_MUMMY_LIMIT)) {
                int32_t nid = GAME_CALL(st, FN_FIGURE_CREATE, S32(st, G_PLAYER), 0x6D, S16(st, o + FIG_X), S16(st, o + FIG_Y), 0);
                uint32_t nf = FIGURE_ADDR(nid);
                S16(st, nf + FIG_HOME) = S16(st, fig(st) + FIG_HOME);
                U8(st, nf + FIG_ACTION) = 9;
                random_next_state(st); /* @bin 0x004B0FA8 the generator step 0x004D7090, inlined */
                S16(st, nf + FIG_WAIT_TICKS) = (int16_t) (U32(st, G_R_1_15) % 50u + 0x19);
                GAME_CALL(st, FN_FIGURE_WALKER_START, nid);
                S16(st, nf + FIG_ROAM_LENGTH) = 0;
                id = cur(st);
            }
        }
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        break;
    }
    case 9:
        U8(st, f + FIG_INVISIBLE) = 0;
        S16(st, f + FIG_ROAM_LENGTH) += 1;
        if (S16(st, f + FIG_ROAM_LENGTH) >= S16(st, f + FIG_MAX_ROAM_LENGTH)) {
            if (GAME_CALL(st, FN_BUILDING_ROAD_WITHIN, S16(st, f + FIG_X), S16(st, f + FIG_Y), 1, 6) == 0) {
                U8(st, fig(st) + FIG_STATE) = 2;
            }
            id = cur(st);
        }
        if ((U8(st, FIGURE_ADDR(id) + FIG_ANIM_FRAME) & 1) == 0) {
            GAME_CALL(st, FN_FIGURE_ROAM, id, 1);
        }
        break;
    default:
        break;
    }
    f = fig(st);
    int32_t d = sprite_direction(st, S8(st, f + FIG_DIRECTION));
    switch (U8(st, f + FIG_ACTION)) {
    case 4: {
        int16_t v = S16(st, f + FIG_WAIT_TICKS);
        int32_t k = v >= 0x10 ? 7 : v / 2;
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_700) + 8 * k + d;
        return;
    }
    case 5: {
        /* @bin 0x004B11B1 the attack direction +0x13, not wrapped */
        int32_t a = S8(st, f + FIG_ATTACK_DIRECTION) - S32(st, G_VIEW_ORIENTATION);
        S32(st, G_SPRITE_DIRECTION) = a;
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_701) + 8 * (U8(st, f + FIG_ANIM_FRAME) >> 1) + a;
        return;
    }
    case 9:
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_699) + 8 * (U8(st, f + FIG_ANIM_FRAME) >> 1) + d;
        return;
    default:
        return;
    }
}
