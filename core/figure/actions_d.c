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
#include "state/layout.h"

#define BUILDING_TYPE_MONUMENT_SITE 0xB3 /* @bin 0x004A88A2 the home building type of type 0x51 */

int32_t monument_effect_at(game_state *st, int32_t x, int32_t y, int32_t id)
{
    /* @bin 0x004BC1E0 the first of the monuments' handlers (cdecl or thiscall on their objects)
     * that takes (x, y, id); returns its result */
    int32_t r;
    if ((r = GAME_CALL(st, FN_PYRAMID_ASSIGN_4F0190, x, y, id)) != 0) return r;
    if ((r = GAME_CALL(st, FN_MONUMENT_0X00420350, x, y, id)) != 0) return r;
    if ((r = GAME_CALL(st, FN_MONUMENT_0X004E5C00, x, y, id)) != 0) return r;
    if ((r = GAME_CALL(st, FN_MONUMENT_TAKE_0XD5, x, y, id)) != 0) return r;
    if ((r = GAME_CALLT(st, G_MONUMENT_E1, FN_MONUMENT_TAKE_0XE1, x, y, id)) != 0) return r;
    if ((r = GAME_CALLT(st, G_MONUMENT_E6, FN_MONUMENT_TAKE_0XE6_401F30, x, y, id)) != 0) return r;
    if ((r = GAME_CALLT(st, G_MONUMENT_E4, FN_MONUMENT_TAKE_0XE4, x, y, id)) != 0) return r;
    if ((r = GAME_CALLT(st, G_MONUMENT_E5, FN_MONUMENT_TAKE_0XE5, x, y, id)) != 0) return r;
    if ((r = GAME_CALLT(st, G_MONUMENT_EA, FN_MONUMENT_TAKE_0XEA, x, y, id)) != 0) return r;
    if ((r = GAME_CALLT(st, G_MONUMENT_EB, FN_MONUMENT_TAKE_0XEB, x, y, id)) != 0) return r;
    if ((r = GAME_CALLT(st, G_MONUMENT_EC, FN_MONUMENT_TAKE_0XEC, x, y, id)) != 0) return r;
    if ((r = GAME_CALLT(st, G_MONUMENT_E3, FN_MONUMENT_TAKE_0XE3, x, y, id)) != 0) return r;
    return GAME_CALL(st, FN_MONUMENT_TAKE_0XDE, x, y, id);
}

/* @bin 0x004A8C9F .. 0x004A8E2A (type 0x51, state 9 arrived): the next state by the type of the
 * building at the destination tile */
static void action_51_arrived(game_state *st, int16_t target, uint32_t tile)
{
    uint32_t f = fig(st);
    if (target != 0) {
        uint32_t t = BUILDING_ADDR(target);
        switch (S16(st, t + BLD_TYPE)) {
        case 0xB7:
            U8(st, f + FIG_ACTION) = 0x0D;
            GAME_CALL(st, FN_WORKER_START_JOB, cur(st));
            return;
        case 0xD2: case 0xCF: case 0xD0:
        case 0xE5: case 0xEA: case 0xEB: case 0xEC: case 0xE6:
            U8(st, f + FIG_ACTION) = 0x0B;
            S16(st, f + FIG_WAIT_TICKS) = 0;
            return;
        case 0xD5:
            U8(st, f + FIG_ACTION) = S16(st, t + BLD_PART) == 0 ? 0x0B : 0x0A;
            S16(st, f + FIG_WAIT_TICKS) = 0;
            return;
        case 0xE3: case 0xE4: case 0xE1: case 0xDE:
            U8(st, f + FIG_ACTION) = 0x0A;
            S16(st, f + FIG_WAIT_TICKS) = 0;
            return;
        default:
            U8(st, f + FIG_ACTION) = 0x0C;
            return;
        }
    }
    int16_t site = S16(st, f + FIG_DEST_BUILDING);
    if (S16(st, BUILDING_ADDR(site) + BLD_TYPE) != 0xB7) {
        U8(st, f + FIG_ACTION) = 0x0C;
        return;
    }
    if (U32(st, G_GRID_TERRAIN + 4u * tile) & 0x10000000u) {
        U8(st, f + FIG_ACTION) = 0x0B;
        S16(st, f + FIG_WAIT_TICKS) = 0;
        return;
    }
    GAME_CALL(st, FN_PYRAMID_REGISTER, site);
    GAME_CALL0(st, FN_PYRAMIDS_REDRAW);
    U8(st, fig(st) + FIG_ACTION) = 0x0C;
}

/* @bin 0x004A919B (type 0x51, state 11): work at the building on the destination tile */
static void action_51_work(game_state *st, uint32_t hb, uint8_t *flag)
{
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    uint32_t tile = U32(st, f + FIG_U28);
    int16_t target = S16(st, G_BUILDING_GRID + 2u * tile);
    if (target == 0) {
        int16_t site = S16(st, f + FIG_DEST_BUILDING);
        if (S16(st, BUILDING_ADDR(site) + BLD_TYPE) == 0xB7 && (U32(st, G_GRID_TERRAIN + 4u * tile) & 0x10000000u)) {
            GAME_CALL(st, FN_PYRAMID_RAMP_BUILD, site, id, 0x0C);
            return;
        }
        U8(st, f + FIG_ACTION) = 8;
        S16(st, f + FIG_WAIT_TICKS) = 0;
        return;
    }
    uint32_t t = BUILDING_ADDR(target);
    switch (S16(st, t + BLD_TYPE)) {
    case 0xB7:
        if (U8(st, t + BLD_U55) == 0) {
            *flag = 1;
        }
        GAME_CALL(st, FN_FIGURE_PYRAMID_PART_WORK, target, id);
        return;
    case 0xD2:
        GAME_CALL(st, FN_CITY_MONUMENT_WORK, target, id, 0x0C);
        return;
    case 0xCF: case 0xD0:
        GAME_CALL(st, FN_MONUMENT_LIST_WORK, target, id, 0x0C);
        return;
    case 0xD5:
        GAME_CALL(st, FN_SUN_TEMPLE_WORK, target, id, 0x0C);
        return;
    case 0xE3:
        GAME_CALLT(st, G_MONUMENT_E3, FN_MONUMENT_WORK_417940, target, id, 0x0C);
        return;
    case 0xE5:
        GAME_CALLT(st, G_MONUMENT_E5, FN_TOMB_WORK, target, id, 0x0C);
        return;
    case 0xEA:
        GAME_CALLT(st, G_MONUMENT_EA, FN_TOMB_WORK, target, id, 0x0C);
        return;
    case 0xEB:
        GAME_CALLT(st, G_MONUMENT_EB, FN_TOMB_WORK, target, id, 0x0C);
        return;
    case 0xEC:
        GAME_CALLT(st, G_MONUMENT_EC, FN_TOMB_WORK, target, id, 0x0C);
        return;
    case 0xE4:
        GAME_CALLT(st, G_MONUMENT_E4, FN_MONUMENT_WORK_4E6E70, target, id, 0x0C);
        return;
    case 0xE1:
        GAME_CALLT(st, G_MONUMENT_E1, FN_OBELISK_BUILD, target, id, 0x0C);
        return;
    case 0xE6:
        GAME_CALLT(st, G_MONUMENT_E6, FN_MONUMENT_WORK_401F50, target, id, 0x0C);
        return;
    case 0xDE:
        GAME_CALL(st, FN_MASTABA_WORK, target, id, 0x0C);
        return;
    default:
        S16(st, t + BLD_UC8) -= 1;
        U8(st, f + FIG_ACTION) = 0x0C;
        S16(st, f + FIG_DEST_X) = S16(st, hb + BLD_ROAD_ACCESS_X);
        S16(st, f + FIG_DEST_Y) = S16(st, hb + BLD_ROAD_ACCESS_Y);
        return;
    }
}

/* @bin 0x004A8F99 (type 0x51, state 10): wait at the building on the destination tile while it
 * still lists the figure */
static void action_51_wait(game_state *st)
{
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    int16_t target = S16(st, G_BUILDING_GRID + 2u * U32(st, f + FIG_U28));
    S16(st, f + FIG_WAIT_TICKS) += 1;
    if (S16(st, f + FIG_WAIT_TICKS) < 0) {
        S16(st, f + FIG_WAIT_TICKS) = 0;
    }
    if (S8(st, f + FIG_DIRECTION) > 7) {
        random_next_state(st); /* @bin 0x004A8FD1 the generator step 0x004D7090, inlined */
        U8(st, f + FIG_DIRECTION) = (uint8_t) (U32(st, G_R_1_15) & 7);
    }
    uint32_t t = BUILDING_ADDR(target);
    switch (S16(st, t + BLD_TYPE)) {
    case 0xB7:
        if (S16(st, f + FIG_FOLLOW) == 0) {
            U8(st, f + FIG_ACTION) = 0x0C;
        } else if (id != S16(st, t + BLD_U64)) {
            U8(st, f + FIG_ACTION) = 0x0C;
            S16(st, f + FIG_FOLLOW) = 0;
        }
        return;
    case 0xE3: case 0xE4: case 0xE1: {
        /* @bin 0x004A9126 the main building (+0x12: offset back to it) */
        uint32_t m = BUILDING_ADDR(target - S16(st, t + BLD_PART));
        if (id != S32(st, m + BLD_U5C)) {
            U8(st, f + FIG_ACTION) = 0x0C;
        }
        return;
    }
    case 0xD5: case 0xE5: case 0xEA: case 0xEB: case 0xEC: case 0xDE:
        if (id != S32(st, t + BLD_U5C)) {
            U8(st, f + FIG_ACTION) = 0x0C;
        }
        return;
    default:
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        f = fig(st);
        U8(st, f + FIG_ACTION) = 8;
        S16(st, f + FIG_WAIT_TICKS) = 0;
        return;
    }
}

void figure_action_51(game_state *st)
{
    /* @bin 0x004A8800 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    uint8_t flag = 0;
    if (S16(st, f + FIG_WAIT_TICKS) < 0) {
        S16(st, f + FIG_WAIT_TICKS) = 0;
    }
    next_frame(st, f);
    U8(st, f + FIG_TERRAIN_USAGE) = 1;
    U8(st, f + FIG_U0C) = 0;
    U8(st, f + FIG_INVISIBLE) = 0;
    S16(st, f + FIG_MAX_ROAM_LENGTH) = 0x200;
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    uint32_t hb = BUILDING_ADDR(S16(st, f + FIG_HOME));
    if (U8(st, hb + BLD_STATE) != 1 || S16(st, hb + BLD_TYPE) != BUILDING_TYPE_MONUMENT_SITE) {
        U8(st, f + FIG_STATE) = 2;
        return;
    }
    if (S16(st, f + FIG_FOLLOW) > 0) {
        uint32_t b2 = BUILDING_ADDR(S16(st, f + FIG_WORK_BUILDING));
        if (S16(st, b2 + BLD_TYPE) != 0xB7 || U8(st, b2 + BLD_STATE) == 0) {
            U8(st, f + FIG_ACTION) = 0x0C;
            S16(st, f + FIG_FOLLOW) = 0;
        }
    }
    /* @bin 0x004A88FD jump table 0x004A96E4 for states 4 .. 16 */
    switch (U8(st, f + FIG_ACTION)) {
    case 4:
        GAME_CALL(st, FN_FIGURE_DYING, id);
        f = fig(st);
        if (U8(st, f + FIG_STATE) == 2) {
            U8(st, BUILDING_ADDR(S16(st, f + FIG_HOME)) + BLD_SHOWS) -= 1;
        }
        break;
    case 5:
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
        S16(st, f + FIG_WAIT_TICKS) += 1;
        random_next_state(st); /* @bin 0x004A89C3 the generator step 0x004D7090, inlined */
        /* @bin 0x004A8A8D idle for 30 + random 15-bit % 30 ticks */
        if (S16(st, f + FIG_WAIT_TICKS) > (int32_t) (U32(st, G_R_1_15) % 30u) + 30) {
            if (monument_effect_at(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), id) != 0) {
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
            U8(st, f + FIG_ACTION) = 0x0C;
            S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
            S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            id = cur(st);
            S16(st, FIGURE_ADDR(id) + FIG_ROAM_LENGTH) = 0;
        }
        GAME_CALL(st, FN_FIGURE_ROAM, cur(st), 1);
        break;
    case 9: {
        U8(st, f + FIG_TERRAIN_USAGE) = 7;
        uint32_t tile = U32(st, f + FIG_U28);
        int16_t target = S16(st, G_BUILDING_GRID + 2u * tile);
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        id = cur(st);
        f = FIGURE_ADDR(id);
        int8_t dir = S8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            action_51_arrived(st, target, tile);
        } else if (dir == 0x0A || dir == 9) {
            int16_t site = S16(st, f + FIG_DEST_BUILDING);
            uint32_t sb = BUILDING_ADDR(site);
            if (S16(st, sb + BLD_TYPE) == 0xB7 && (U32(st, G_GRID_TERRAIN + 4u * tile) & 0x10000000u)) {
                GAME_CALL(st, FN_FIGURE_TILE_LIST_REMOVE, id);
                id = cur(st);
                f = FIGURE_ADDR(id);
                S16(st, f + FIG_X) = S16(st, f + FIG_DEST_X);
                S16(st, f + FIG_Y) = S16(st, f + FIG_DEST_Y);
                S32(st, f + FIG_GRID_OFFSET) = S32(st, f + FIG_U28);
                GAME_CALL(st, FN_FIGURE_TILE_LIST_ADD, id);
                f = fig(st);
                U8(st, f + FIG_ACTION) = 0x0B;
                S16(st, f + FIG_WAIT_TICKS) = 0;
            } else {
                GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
                int16_t t = S16(st, sb + BLD_TYPE); /* re-read after the call */
                f = fig(st);
                if (t != 0xE1 && t != 0xE6 && t != 0xE3 && t != 0xE4) {
                    U8(st, f + FIG_ACTION) = 8;
                }
                S16(st, f + FIG_WAIT_TICKS) = 0;
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
        if (S16(st, f + FIG_DEST_X) != S16(st, hb + BLD_ROAD_ACCESS_X) || S16(st, f + FIG_DEST_Y) != S16(st, hb + BLD_ROAD_ACCESS_Y)) {
            S16(st, f + FIG_DEST_X) = S16(st, hb + BLD_ROAD_ACCESS_X);
            S16(st, f + FIG_DEST_Y) = S16(st, hb + BLD_ROAD_ACCESS_Y);
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        }
        GAME_CALL(st, FN_FIGURE_MOVE, cur(st), speed);
        f = fig(st);
        if (S8(st, f + FIG_DIRECTION) == 8 || S8(st, f + FIG_DIRECTION) == 0x0A || S8(st, f + FIG_DIRECTION) == 9) {
            if (S8(st, f + FIG_DIRECTION) == 9) {
                GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, cur(st));
            }
            f = fig(st);
            U8(st, f + FIG_STATE) = 2;
            U8(st, BUILDING_ADDR(S16(st, f + FIG_HOME)) + BLD_SHOWS) -= 1;
        }
        break;
    case 0x0D:
        GAME_CALL(st, FN_FIGURE_PYRAMID_WORK, id, 0, 0x0C, 0x0E);
        break;
    case 0x0E:
        GAME_CALL(st, FN_FIGURE_PYRAMID_CHOOSE, id, 1, 0x0C, 0x10, 0x0F);
        break;
    case 0x0F:
        if ((int8_t) GAME_CALL(st, FN_FIGURE_PYRAMID_STEP, id, 1) != 0) {
            GAME_CALL(st, FN_WORKER_JOIN, cur(st));
            f = fig(st);
            U8(st, f + FIG_ACTION) = U8(st, BUILDING_ADDR(S16(st, f + FIG_WORK_BUILDING)) + BLD_U55) != 0 ? 0x0B : 0x0A;
            S16(st, f + FIG_WAIT_TICKS) = 0;
        }
        break;
    case 0x10:
        GAME_CALL(st, FN_WORKER_FIND_PLACE_4F2230, id);
        break;
    default:
        break;
    }
    id = cur(st);
    f = FIGURE_ADDR(id);
    int32_t d = sprite_direction(st, S8(st, f + FIG_DIRECTION));
    switch (U8(st, f + FIG_ACTION)) {
    case 4:
        S32(st, f + FIG_U04) = (int32_t) (dying_frame(st, f) + U32(st, G_IMAGE_GROUP_190));
        return;
    case 0x0A: {
        /* @bin 0x004A95B4 working animation: phase from +0x36 and the figure id */
        int32_t phase = (S16(st, f + FIG_WAIT_TICKS) >> 1) + 13 * id;
        switch ((phase / 12) & 3) { /* @bin jump table 0x004A9718 */
        case 0:
            S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_194) + 8 * ((phase >> 2) & 1) + d;
            return;
        case 1:
            S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_194) + 8 * (phase % 12) + d;
            return;
        case 2:
            if ((phase & 7) != 4) {
                return;
            }
            switch (crt_rand(st) & 3) {
            case 0:
                U8(st, fig(st) + FIG_DIRECTION) = (uint8_t) ((U8(st, fig(st) + FIG_DIRECTION) + 1) & 7);
                break;
            case 1:
                U8(st, fig(st) + FIG_DIRECTION) = (uint8_t) ((U8(st, fig(st) + FIG_DIRECTION) - 1) & 7);
                break;
            default:
                break;
            }
            S32(st, fig(st) + FIG_U04) = S32(st, G_IMAGE_GROUP_194) + S32(st, G_SPRITE_DIRECTION);
            return;
        default:
            S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_194) + d;
            return;
        }
    }
    case 0x0B:
        if (flag && S16(st, f + FIG_WAIT_TICKS) < S32(st, G_U5E4EE8)) {
            S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_194) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
            return;
        }
        if (U8(st, f + FIG_ANIM_FRAME) >= 7) {
            U8(st, f + FIG_ANIM_FRAME) = 0;
        }
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_191) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
        return;
    default:
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_189) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
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
        if (S16(st, f + FIG_X) == S16(st, f + FIG_SOURCE_X) && S16(st, f + FIG_Y) == S16(st, f + FIG_SOURCE_Y)) {
            U8(st, f + FIG_INVISIBLE) = 1;
        }
        if (S16(st, f + FIG_FOLLOW) == 0) {
            break;
        }
        if (S16(st, p + FIG_FOLLOW) == 0) {
            /* @bin 0x004B2426 OutputDebugStringA("follow_figure_closely following off pyramid"):
             * debugger output only */
            S16(st, f + FIG_FOLLOW) = 0;
            continue;
        }
        int copy = 1;
        if (S16(st, f + FIG_CC_DEST_X) == S16(st, p + FIG_CC_DEST_X)) {
            if (S16(st, f + FIG_CC_DELTA_X) + lag < S16(st, p + FIG_CC_DELTA_X)) {
                GAME_CALL(st, FN_FIGURE_PYRAMID_STEP, id, step);
            }
            copy = 0;
        } else if (S16(st, f + FIG_CC_DEST_X) == S16(st, p + FIG_CC_X)) {
            copy = (int8_t) GAME_CALL(st, FN_FIGURE_PYRAMID_STEP, id, step) != 0;
        }
        if (copy) {
            goto copy_line;
        }
        U8(st, f + FIG_ANIM_FRAME) = U8(st, p + FIG_ANIM_FRAME);
        return;
    }
    if (S16(st, p + FIG_FOLLOW) != 0) {
        U8(st, f + FIG_PROGRESS_ON_TILE) += 1;
        if (S8(st, f + FIG_PROGRESS_ON_TILE) <= 8) {
            U8(st, f + FIG_ANIM_FRAME) = U8(st, p + FIG_ANIM_FRAME);
            return;
        }
        U8(st, f + FIG_PROGRESS_ON_TILE) = 0;
        goto copy_line;
    }
    {
        int32_t a = S8(st, p + FIG_PROGRESS_ON_TILE) - lag;
        uint8_t b = (uint8_t) a;
        if (a < 0) {
            b = (uint8_t) (b + 0x0F);
        }
        U8(st, f + FIG_PROGRESS_ON_TILE) = b;
        if (b == 8) {
            GAME_CALL(st, FN_FIGURE_TILE_LIST_REMOVE, id);
            int16_t ox = S16(st, f + FIG_X), oy = S16(st, f + FIG_Y);
            S16(st, f + FIG_X) = S16(st, p + FIG_X);
            S16(st, f + FIG_Y) = S16(st, p + FIG_Y);
            S32(st, f + FIG_GRID_OFFSET) = (int32_t) (U32(st, G_MAP_START) + (uint32_t) S16(st, f + FIG_X)
                                           + 228u * (uint32_t) S16(st, f + FIG_Y));
            if (S16(st, f + FIG_X) != ox || S16(st, f + FIG_Y) != oy) {
                S16(st, f + FIG_PREV_X) = ox;
                S16(st, f + FIG_PREV_Y) = oy;
            }
            GAME_CALL(st, FN_FIGURE_TILE_LIST_ADD, id);
            uint32_t tile = U32(st, f + FIG_GRID_OFFSET);
            uint32_t t = U32(st, G_GRID_TERRAIN + 4u * tile);
            if ((t & 0x40u) == 0 && (t & 0x40000000u) == 0) {
                U8(st, f + FIG_ON_ROAD) = 0;
            } else {
                U8(st, f + FIG_ON_ROAD) = 1;
                if (t & 4u) {
                    U8(st, f + FIG_HEIGHT_TICKS) = 0x12;
                    uint8_t g = U8(st, G_SPRITE + tile);
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
                    U8(st, f + FIG_HEIGHT_TARGET) = slow;
                }
            }
        }
    }
    if (U8(st, f + FIG_PROGRESS_ON_TILE) == 0) {
        U8(st, f + FIG_PREV_DIRECTION) = U8(st, f + FIG_DIRECTION);
        U8(st, f + FIG_DIRECTION) = U8(st, p + FIG_DIRECTION);
    }
    if (U8(st, f + FIG_HEIGHT_TICKS) != 0) {
        U8(st, f + FIG_HEIGHT_TICKS) -= 1;
        if (U8(st, f + FIG_HEIGHT_TICKS) == 0) {
            U8(st, f + FIG_INVISIBLE) = 0;
        } else {
            U8(st, f + FIG_INVISIBLE) = 1;
            if (U8(st, f + FIG_HEIGHT) < U8(st, f + FIG_HEIGHT_TARGET)) {
                U8(st, f + FIG_HEIGHT) += 1;
            }
            if (U8(st, f + FIG_HEIGHT) > U8(st, f + FIG_HEIGHT_TARGET)) {
                U8(st, f + FIG_HEIGHT) -= 1;
            }
        }
    } else if (U8(st, f + FIG_HEIGHT) != 0) {
        U8(st, f + FIG_HEIGHT) -= 1;
    } else {
        U8(st, f + FIG_INVISIBLE) = 0;
    }
    U8(st, f + FIG_ANIM_FRAME) = U8(st, p + FIG_ANIM_FRAME);
    return;
copy_line:
    S16(st, f + FIG_FOLLOW) = S16(st, p + FIG_FOLLOW);
    S16(st, f + FIG_CC_DELTA_Y) = S16(st, p + FIG_CC_DELTA_Y);
    {
        int32_t r = S16(st, p + FIG_CC_DELTA_X) - lag;
        S16(st, f + FIG_CC_DELTA_X) = (int16_t) (r < 0 ? 0 : r);
    }
    S16(st, f + FIG_CC_X) = S16(st, p + FIG_CC_X);
    S16(st, f + FIG_CC_DEST_X) = S16(st, p + FIG_CC_DEST_X);
    S16(st, f + FIG_CC_Y) = S16(st, p + FIG_CC_Y);
    S16(st, f + FIG_CC_DEST_Y) = S16(st, p + FIG_CC_DEST_Y);
    U8(st, f + FIG_DIRECTION) = U8(st, p + FIG_DIRECTION);
    GAME_CALL(st, FN_FIGURE_PYRAMID_STEP, id, 0);
    U8(st, f + FIG_ANIM_FRAME) = U8(st, p + FIG_ANIM_FRAME);
}

int32_t figure_sledge_leader(game_state *st, int32_t id, int32_t *distance)
{
    /* @bin 0x004A34F0 follow the +0x68 links (at most 8) to the figure of type 0x55 */
    int32_t f = S16(st, FIGURE_ADDR(id) + FIG_LEADER);
    *distance = 1;
    for (int i = 0; i < 8; i++) {
        if (U8(st, FIGURE_ADDR(f) + FIG_TYPE) == 0x55) {
            return f;
        }
        *distance += 1;
        f = S16(st, FIGURE_ADDR(f) + FIG_LEADER);
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
    if (U8(st, f + FIG_ACTION) == 8) {
        U8(st, f + FIG_INVISIBLE) = 0;
        U8(st, f + FIG_U0C) = 0;
        prev = S16(st, f + FIG_LEADER);
        uint32_t p = FIGURE_ADDR(prev);
        if (U8(st, p + FIG_TYPE) == 0x55) {
            ok = sledge_moving_state(U8(st, p + FIG_ACTION), with_24);
        } else if (U8(st, p + FIG_TYPE) == 0x60) {
            ok = U8(st, p + FIG_ACTION) == 8;
        }
    }
    if (!ok) {
        U8(st, f + FIG_STATE) = 2;
    } else {
        uint32_t l = FIGURE_ADDR(leader);
        uint8_t ls = U8(st, l + FIG_ACTION);
        if (ls != 6) {
            int32_t v = limit;
            if (ls == 0x1D) {
                /* @bin 0x004ABB4D / 0x004AF44E the step: 7 * distance - leader +0x36 (+ base),
                 * clamped to 1 .. limit */
                v = distance * 7 - S16(st, l + FIG_WAIT_TICKS) + base;
                if (v < 1) {
                    v = 1;
                } else if (v > limit) {
                    v = limit;
                }
            }
            sledge_follow(st, id, prev, 1, v);
        }
    }
    if (U8(st, FIGURE_ADDR(leader) + FIG_ACTION) == 6) {
        U8(st, fig(st) + FIG_INVISIBLE) = 1;
    }
    return leader;
}

/* the sprite direction, not stored in G_SPRITE_DIRECTION (types 0x56, 0x60) */
static int32_t local_direction(game_state *st, uint32_t f)
{
    int32_t d = S8(st, f + FIG_DIRECTION) - S32(st, G_VIEW_ORIENTATION);
    return d < 0 ? d + 8 : d;
}

void figure_action_56(game_state *st)
{
    /* @bin 0x004AB980 */
    sledge_part_follow(st, 1, 5, 0x0C);
    uint32_t f = fig(st);
    int32_t d = local_direction(st, f);
    uint32_t group;
    uint8_t r = U8(st, f + FIG_RESOURCE);
    if (S16(st, f + FIG_FOLLOW) != 0 && S16(st, f + FIG_CC_Y) < S16(st, f + FIG_CC_DEST_Y)) {
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
    S32(st, f + FIG_U04) = S32(st, G_DAY2_REDRAW_GROUPS + 8u * group) + d;
}

void figure_action_60(game_state *st)
{
    /* @bin 0x004AF2C0 */
    sledge_part_follow(st, 0, 0, 7);
    uint32_t f = fig(st);
    int32_t d = local_direction(st, f);
    uint32_t base = U8(st, f + FIG_RESOURCE) != 0 ? G_IMAGE_GROUP_160 : G_IMAGE_GROUP_161;
    S32(st, f + FIG_U04) = S32(st, base) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
}

int32_t figure_find_enemy(game_state *st, int32_t x, int32_t y, int32_t max_distance)
{
    /* @bin 0x0048E820 the figures that can be attacked near (x, y) (alive, not targeted (+0x84),
     * at most 100): distance (max of |dx|, |dy|) times 1 for type 0x29 in state 0x0B, 2 for types
     * 0x2B .. 0x39 and 0x63, 3 for 0x52 .. 0x54, 0x66 .. 0x68, 0x6D; those nearer than
     * max_distance go to 0x00518B30 for the choice (lists 0x00BCA6B4 ids, 0x00BCA524 tiles) */
    int32_t n = 0;
    S32(st, G_BEST_DISTANCE) = 10000;
    S32(st, G_ENEMY_COUNT) = 0;
    for (int32_t i = 1; i < FIGURE_COUNT; i++) {
        uint32_t f = FIGURE_ADDR(i);
        if (U8(st, f + FIG_STATE) != 1 || S16(st, f + FIG_TARGETED_BY) != 0) {
            continue;
        }
        if (n >= 100) {
            break;
        }
        uint8_t type = U8(st, f + FIG_TYPE);
        int32_t weight;
        if (type == 0x29 && U8(st, f + FIG_ACTION) == 0x0B) {
            weight = 1;
        } else if ((type >= 0x2B && type <= 0x39) || type == 0x63) {
            weight = 2;
        } else if (type == 0x52 || type == 0x53 || type == 0x54 || type == 0x67 || type == 0x68 || type == 0x66
                   || type == 0x6D) {
            weight = 3;
        } else {
            continue;
        }
        int32_t fx = S16(st, f + FIG_X), fy = S16(st, f + FIG_Y);
        int32_t dx = x > fx ? x - fx : (x < fx ? fx - x : 0);
        S32(st, G_DIST_X) = dx;
        int32_t dy = y > fy ? y - fy : (y < fy ? fy - y : 0);
        S32(st, G_DIST_Y) = dy;
        int32_t d = (dx >= dy ? dx : dy) * weight;
        if (d < max_distance) {
            S32(st, G_ENEMY_IDS + 4u * (uint32_t) n) = i;
            S32(st, G_ENEMY_TILES + 4u * (uint32_t) n) = S32(st, f + FIG_GRID_OFFSET);
            n++;
        }
    }
    S32(st, G_ENEMY_COUNT) = n;
    if (n == 0) {
        return 0;
    }
    int32_t r = GAME_CALL(st, FN_ROUTE_ANY_FREE_TO_ANY, x, y, G_ENEMY_TILES, n);
    if (r <= 0) {
        return 0;
    }
    return S32(st, G_ENEMY_IDS - 4u + 4u * (uint32_t) r);
}

int32_t figure_seek_enemy(game_state *st, int32_t id)
{
    /* @bin 0x0048E6F0 every 10th call (+0x80), while there are enemies (C+0x4544 or 0x00D0563C),
     * a figure not dying, fighting or busy goes for an enemy within 30 (state 0x0E, +0x82) */
    uint32_t c = CURRENT_CITY(st);
    if (S32(st, c + CITY_U4544) <= 0 && S32(st, G_ENEMY_FORMATIONS) <= 0) {
        return 0;
    }
    uint32_t f = FIGURE_ADDR(id);
    switch (U8(st, f + FIG_ACTION)) {
    case 5: case 4: case 8: case 9: case 0x0C: case 0x0D: case 0x0E: case 0x0F:
        return 0;
    default:
        break;
    }
    U8(st, f + FIG_U80) += 1;
    if (U8(st, f + FIG_U80) < 10) {
        return 0;
    }
    U8(st, f + FIG_U80) = 0;
    int32_t t = figure_find_enemy(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), 0x1E);
    if (t <= 0) {
        return 0;
    }
    uint32_t e = FIGURE_ADDR(t);
    U8(st, f + FIG_U80) = 0;
    U8(st, f + FIG_ACTION) = 0x0E;
    S16(st, f + FIG_DEST_X) = S16(st, e + FIG_X);
    S16(st, f + FIG_DEST_Y) = S16(st, e + FIG_Y);
    S16(st, f + FIG_TARGET) = (int16_t) t;
    S16(st, e + FIG_TARGETED_BY) = (int16_t) id;
    S16(st, f + FIG_U88) = S16(st, e + FIG_SEQUENCE);
    GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
    return 1;
}

void figure_action_57(game_state *st)
{
    /* @bin 0x004ABD50 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_TERRAIN_USAGE) = 1;
    U8(st, f + FIG_U0C) = 0;
    S16(st, f + FIG_MAX_ROAM_LENGTH) = 0x180;
    uint32_t b = BUILDING_ADDR(S16(st, f + FIG_HOME));
    if (U8(st, b + BLD_STATE) != 1 || S16(st, b + BLD_WALKER) != id) {
        U8(st, f + FIG_STATE) = 2; /* its building is gone, or no longer links to it */
    }
    next_frame(st, f);
    GAME_CALL(st, FN_FIGB_ROAMER_ACTION, 1);
    f = fig(st);
    int32_t d = sprite_direction(st, S8(st, f + FIG_DIRECTION));
    if (U8(st, f + FIG_ACTION) == 4) {
        S32(st, f + FIG_U04) = (int32_t) (dying_frame(st, f) + U32(st, G_IMAGE_GROUP_99));
        return;
    }
    S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_98) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
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
    switch (U8(st, f + FIG_ACTION)) {
    case 5:
        if (with_53b4a0) {
            GAME_CALL(st, FN_FIGURE_SOUND_53B4A0, id, 10, 2);
            id = cur(st);
        }
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        break;
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
        if (road_near(st, b) == 0) {
            U8(st, fig(st) + FIG_STATE) = 2;
            break;
        }
        id = cur(st);
        f = FIGURE_ADDR(id);
        U8(st, f + FIG_ACTION) = 9;
        S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
        S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
        move_to_dest(st, id, f);
        S16(st, fig(st) + FIG_ROAM_LENGTH) = 0;
        break;
    case 9:
        U8(st, f + FIG_U0C) = 1;
        U8(st, f + FIG_INVISIBLE) = 1;
        if (GAME_CALL(st, FN_FIGURE_STEP_STRAIGHT, id, 1) != 1) {
            break;
        }
        id = cur(st);
        f = FIGURE_ADDR(id);
        if (S16(st, G_BUILDING_GRID + 2u * U32(st, f + FIG_GRID_OFFSET)) == home) {
            U8(st, f + FIG_STATE) = 2;
            break;
        }
        U8(st, f + FIG_ACTION) = 0x0A;
        GAME_CALL(st, FN_FIGURE_WALKER_START, id);
        S16(st, fig(st) + FIG_ROAM_LENGTH) = 0;
        break;
    case 0x0A:
        U8(st, f + FIG_INVISIBLE) = 0;
        S16(st, f + FIG_ROAM_LENGTH) += 1;
        if (S16(st, f + FIG_ROAM_LENGTH) >= S16(st, f + FIG_MAX_ROAM_LENGTH)) {
            if (road_near(st, b) == 0) {
                id = cur(st);
                U8(st, FIGURE_ADDR(id) + FIG_STATE) = 2;
                GAME_CALL(st, FN_FIGURE_ROAM, id, 1);
                break;
            }
            id = cur(st);
            f = FIGURE_ADDR(id);
            U8(st, f + FIG_ACTION) = 0x0B;
            S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
            S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            id = cur(st);
        }
        GAME_CALL(st, FN_FIGURE_ROAM, id, 1);
        break;
    case 0x0B:
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        id = cur(st);
        f = FIGURE_ADDR(id);
        if (S8(st, f + FIG_DIRECTION) == 9 || S8(st, f + FIG_DIRECTION) == 0x0A) {
            U8(st, f + FIG_STATE) = 2;
        } else if (S8(st, f + FIG_DIRECTION) == 8) {
            U8(st, f + FIG_ACTION) = 9;
            S16(st, f + FIG_DEST_X) = S16(st, b + BLD_X);
            S16(st, f + FIG_DEST_Y) = S16(st, b + BLD_Y);
            move_to_dest(st, id, f);
            S16(st, fig(st) + FIG_ROAM_LENGTH) = 0;
        }
        break;
    case 0x0E:
        U8(st, f + FIG_TERRAIN_USAGE) = 0;
        if (GAME_CALL(st, FN_FIGURE_WITH_LEADER, id) == 0) {
            if (road_near(st, BUILDING_ADDR(S16(st, fig(st) + FIG_HOME))) == 0) {
                U8(st, fig(st) + FIG_STATE) = 2;
            } else {
                id = cur(st);
                f = FIGURE_ADDR(id);
                U8(st, f + FIG_ACTION) = 0x0B;
                S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
                S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
                GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
                S16(st, fig(st) + FIG_ROAM_LENGTH) = 0;
            }
        }
        id = cur(st);
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        id = cur(st);
        f = FIGURE_ADDR(id);
        if (S8(st, f + FIG_DIRECTION) == 9 || S8(st, f + FIG_DIRECTION) == 0x0A) {
            U8(st, f + FIG_STATE) = 2;
        } else if (S8(st, f + FIG_DIRECTION) == 8) {
            uint32_t o = FIGURE_ADDR(S16(st, f + FIG_TARGET));
            S16(st, f + FIG_DEST_X) = S16(st, o + FIG_X);
            S16(st, f + FIG_DEST_Y) = S16(st, o + FIG_Y);
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        }
        break;
    default:
        U8(st, f + FIG_ACTION) = 8;
        break;
    }
}

void figure_action_58(game_state *st)
{
    /* @bin 0x004ABE40 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_TERRAIN_USAGE) = 1;
    U8(st, f + FIG_U0C) = 0;
    S16(st, f + FIG_MAX_ROAM_LENGTH) = 0x280;
    int16_t home = S16(st, f + FIG_HOME);
    /* @bin 0x004ABE87 per-type table 0x007090EC (0x38 bytes per type), passed to 0x004A5DA0 */
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    uint32_t b = BUILDING_ADDR(home);
    if (U8(st, b + BLD_STATE) != 1 || S16(st, b + BLD_WALKER) != id) {
        U8(st, f + FIG_STATE) = 2;
    }
    if (next_frame(st, f) == 0) {
        figure_seek_enemy(st, id);
    }
    home_worker_states(st, b, home, speed, 1);
    id = cur(st);
    f = FIGURE_ADDR(id);
    uint8_t action = U8(st, f + FIG_ACTION);
    int32_t dir;
    if (action == 5) {
        dir = S8(st, f + FIG_ATTACK_DIRECTION);
    } else if (S8(st, f + FIG_DIRECTION) < 8) {
        dir = S8(st, f + FIG_DIRECTION);
    } else {
        dir = S8(st, f + FIG_PREV_DIRECTION);
    }
    int32_t d = sprite_direction(st, dir);
    if (action == 5) {
        GAME_CALL(st, FN_FIGURE_SOUND_53B4A0, id, 10, 2);
        f = fig(st);
        uint8_t a = U8(st, f + FIG_ATTACK_IMAGE);
        if (a < 0x0C) {
            S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_61) + S32(st, G_SPRITE_DIRECTION);
        } else {
            S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_61) + ((int32_t) a - 0x0C) / 2 * 8 + S32(st, G_SPRITE_DIRECTION);
        }
        return;
    }
    if (action == 4) {
        S32(st, f + FIG_U04) = (int32_t) (dying_frame(st, f) + U32(st, G_IMAGE_GROUP_60));
        return;
    }
    S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_59) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
}

void figure_action_59(game_state *st)
{
    /* @bin 0x004AC450 type 0x58 without the 0x0048E6F0 / 0x0053B4A0 calls, other sprites */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_TERRAIN_USAGE) = 1;
    U8(st, f + FIG_U0C) = 0;
    S16(st, f + FIG_MAX_ROAM_LENGTH) = 0x280;
    int16_t home = S16(st, f + FIG_HOME);
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    uint32_t b = BUILDING_ADDR(home);
    if (U8(st, b + BLD_STATE) != 1 || S16(st, b + BLD_WALKER) != id) {
        U8(st, f + FIG_STATE) = 2;
    }
    next_frame(st, f);
    home_worker_states(st, b, home, speed, 0);
    f = fig(st);
    uint8_t action = U8(st, f + FIG_ACTION);
    int32_t dir;
    if (action == 5) {
        dir = S8(st, f + FIG_ATTACK_DIRECTION);
    } else if (S8(st, f + FIG_DIRECTION) < 8) {
        dir = S8(st, f + FIG_DIRECTION);
    } else {
        dir = S8(st, f + FIG_PREV_DIRECTION);
    }
    int32_t d = sprite_direction(st, dir);
    if (action == 5) {
        uint8_t a = U8(st, f + FIG_ATTACK_IMAGE);
        if (a < 0x0C) {
            S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_251) + d;
        } else {
            S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_251) + ((int32_t) a - 0x0C) / 2 * 8 + d;
        }
        return;
    }
    if (action == 4) {
        S32(st, f + FIG_U04) = (int32_t) (dying_frame(st, f) + U32(st, G_IMAGE_GROUP_252));
        return;
    }
    S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_251) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
}

void figure_action_5A(game_state *st)
{
    /* @bin 0x004AC9F0 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_TERRAIN_USAGE) = 3;
    U8(st, f + FIG_INVISIBLE) = 0;
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    int16_t home = S16(st, f + FIG_HOME);
    if (home != 0 && U8(st, BUILDING_ADDR(home) + BLD_STATE) != 1) {
        U8(st, f + FIG_STATE) = 2;
        return;
    }
    /* @bin 0x004ACA65 jump table 0x004ACE90 for states 4 .. 11 */
    switch (U8(st, f + FIG_ACTION)) {
    case 4:
        GAME_CALL(st, FN_FIGURE_DYING, id);
        break;
    case 5:
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        break;
    case 6:
        U8(st, f + FIG_INVISIBLE) = 1;
        break;
    case 7:
        /* @bin 0x004ACD73 the sprite direction of the previous update */
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_76) + S32(st, G_SPRITE_DIRECTION);
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
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0x0A) {
            if (S32(st, G_BLOOD_RIVER_LEFT) != 0
                || GAME_CALL(st, FN_FIND_TREE, S16(st, f + FIG_X), S16(st, f + FIG_Y), 0xFF, 0x40000) == 0) {
                id = cur(st);
                U8(st, FIGURE_ADDR(id) + FIG_STATE) = 2;
                GAME_CALL(st, FN_FIGURE_TILE_LIST_REMOVE, id);
                return;
            }
            f = fig(st);
            U8(st, f + FIG_ACTION) = 9;
            S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
            S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
        }
        U8(st, f + FIG_ANIM_FRAME) = 0;
        break;
    case 9:
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = fig(st);
        if (S8(st, f + FIG_DIRECTION) == 8) {
            uint32_t t = U32(st, f + FIG_GRID_OFFSET);
            /* @bin 0x004ACBD4 tile byte 0xFF in 0x00A43EAC, and no figure of state 0x0A on it
             * (figure grid 0x00B350DC) */
            int ok = U8(st, G_GRID_VEGETATION + t) >= 0xFF;
            if (ok) {
                int16_t o = S16(st, G_FIGURE_GRID + 2u * t);
                if (o != 0 && U8(st, FIGURE_ADDR(o) + FIG_ACTION) == 0x0A) {
                    ok = 0;
                }
            }
            if (ok) {
                U8(st, f + FIG_ACTION) = 0x0A;
                S16(st, f + FIG_WAIT_TICKS) = 0;
                U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
            } else {
                U8(st, f + FIG_ACTION) = 8;
                S16(st, f + FIG_WAIT_TICKS) = 0;
            }
        } else if (S8(st, f + FIG_DIRECTION) == 0x0A) {
            U8(st, f + FIG_STATE) = 2;
        } else if (S8(st, f + FIG_DIRECTION) == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, cur(st));
            f = fig(st);
            U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
        }
        break;
    case 0x0A:
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0x12C) {
            /* @bin 0x004ACC7E grid offset 0x00BCA2D8 + x + 228 y */
            int32_t off = (int32_t) (U32(st, G_MAP_START) + (uint32_t) S16(st, f + FIG_X)
                                     + (uint32_t) (S16(st, f + FIG_Y) * 228));
            int16_t h = S16(st, f + FIG_HOME);
            U8(st, G_GRID_VEGETATION + (uint32_t) off) = 0;
            GAME_CALL(st, FN_TREE_IMAGE_UPDATE, off);
            f = fig(st);
            U8(st, f + FIG_ACTION) = 0x0B;
            S16(st, f + FIG_DEST_X) = S16(st, BUILDING_ADDR(h) + BLD_ROAD_ACCESS_X);
            S16(st, f + FIG_DEST_Y) = S16(st, BUILDING_ADDR(h) + BLD_ROAD_ACCESS_Y);
        }
        break;
    case 0x0B:
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = fig(st);
        if (S8(st, f + FIG_DIRECTION) == 8) {
            U8(st, f + FIG_STATE) = 2;
            S16(st, BUILDING_ADDR(S16(st, f + FIG_HOME)) + BLD_STORED) += 0x32; /* @bin 0x004ACD19 */
        } else if (S8(st, f + FIG_DIRECTION) == 0x0A) {
            U8(st, f + FIG_STATE) = 2;
        } else if (S8(st, f + FIG_DIRECTION) == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, cur(st));
            f = fig(st);
            U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
        }
        break;
    default:
        U8(st, f + FIG_ACTION) = 8;
        break;
    }
    f = fig(st);
    U8(st, f + FIG_ANIM_FRAME) += 1;
    /* @bin 0x004ACA9A 15 frames in state 0x0A, else 12 */
    if (U8(st, f + FIG_ANIM_FRAME) >= (U8(st, f + FIG_ACTION) == 0x0A ? 15 : 12)) {
        U8(st, f + FIG_ANIM_FRAME) = 0;
    }
    int32_t d = sprite_direction(st, S8(st, f + FIG_DIRECTION));
    uint8_t action = U8(st, f + FIG_ACTION);
    if (action == 0x0A) {
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_78) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
    } else if (action == 8 || action == 5) {
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_76) + d;
    } else if (action == 0x0B) {
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_79) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
    } else if (action == 4) {
        int16_t t = S16(st, f + FIG_WAIT_TICKS);
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_77) + (t >= 8 ? 7 : t);
    } else {
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_76) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
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
    U8(st, f + FIG_U0C) = 0;
    S16(st, f + FIG_MAX_ROAM_LENGTH) = 0x180;
    int16_t home = S16(st, f + FIG_HOME);
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    next_frame(st, f);
    uint32_t c = CURRENT_CITY(st);
    /* @bin 0x004ACF21 jump table 0x004AD230 for states 8 .. 11 */
    switch (U8(st, f + FIG_ACTION)) {
    case 8:
        U8(st, f + FIG_TERRAIN_USAGE) = 1;
        S32(st, f + FIG_U164) = 10;
        S16(st, f + FIG_DEST_X) = (int16_t) (S16(st, c + CITY_U47A0) + 2);
        S16(st, f + FIG_DEST_Y) = (int16_t) (S16(st, c + CITY_U47A4) + 2);
        U8(st, f + FIG_ACTION) = 0x0A;
        break;
    case 9: {
        U8(st, f + FIG_TERRAIN_USAGE) = 0;
        if (S32(st, c + CITY_U47A0) == -1 || S32(st, c + CITY_U47A4) == -1) {
            U8(st, f + FIG_STATE) = 2;
            S32(st, f + FIG_U04) = 0;
            return;
        }
        int32_t x, y;
        do {
            x = random_offset(st, 0x47A0);
            y = random_offset(st, 0x47A4);
            f = fig(st);
        } while (x == S16(st, f + FIG_X) && y == S16(st, f + FIG_Y));
        S16(st, f + FIG_DEST_X) = (int16_t) x;
        S16(st, f + FIG_DEST_Y) = (int16_t) y;
        U8(st, f + FIG_ACTION) = 0x0A;
        break;
    }
    case 0x0A:
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = fig(st);
        if (S8(st, f + FIG_DIRECTION) == 8) {
            U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
            U8(st, f + FIG_ACTION) = 0x0B;
        } else if (S8(st, f + FIG_DIRECTION) == 0x0A) {
            U8(st, f + FIG_STATE) = 2;
        } else if (S8(st, f + FIG_DIRECTION) == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, cur(st));
            f = fig(st);
            U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
            U8(st, f + FIG_ACTION) = 0x0B;
            S32(st, f + FIG_U164) = 0;
        }
        break;
    case 0x0B:
        S32(st, f + FIG_U164) -= 1;
        if (S32(st, f + FIG_U164) <= 0) {
            U8(st, f + FIG_STATE) = 2;
        } else {
            U8(st, f + FIG_ACTION) = 9;
        }
        break;
    default:
        U8(st, f + FIG_ACTION) = 9;
        U8(st, f + FIG_TERRAIN_USAGE) = 1;
        break;
    }
    /* @bin 0x004AD102 sprite group (table 0x00F55BB4) and +0x0B by the home building's type
     * (jump tables 0x004AD240 / 0x004AD260) */
    uint32_t group;
    uint8_t carried;
    int16_t btype = S16(st, BUILDING_ADDR(home) + BLD_TYPE);
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
        switch ((uint32_t) GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, home)) {
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
    U8(st, f + FIG_RESOURCE) = carried;
    int32_t d = sprite_direction(st, S8(st, f + FIG_DIRECTION));
    S32(st, f + FIG_U04) = S32(st, G_DAY2_REDRAW_GROUPS + 8u * group) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
}

void figure_action_5F(game_state *st)
{
    /* @bin 0x004AF140 */
    uint32_t f = fig(st);
    U8(st, f + FIG_INVISIBLE) = 0;
    U8(st, f + FIG_BOAT_MODE) = 1;
    S32(st, f + FIG_U180) = 0;
    if (U8(st, f + FIG_ANIM_FRAME) >= 0x18) {
        U8(st, f + FIG_ANIM_FRAME) = 0x18;
        S16(st, f + FIG_WAIT_TICKS) += 1;
        /* @bin 0x004AF197 hold the last frame for 0x19 + (+0x92) ticks */
        if (S16(st, f + FIG_WAIT_TICKS) >= (int32_t) U8(st, f + FIG_U92) + 0x19) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
            U8(st, f + FIG_ANIM_FRAME) = 0;
        }
    } else {
        U8(st, f + FIG_ANIM_FRAME) += 1;
        random_next_state(st); /* @bin 0x004AF1B1 the generator step 0x004D7090, inlined */
        U8(st, f + FIG_U92) = (uint8_t) (crt_rand(st) % 100);
    }
    S32(st, f + FIG_U04) = (int32_t) (U32(st, G_IMAGE_GROUP_585) + U8(st, f + FIG_ANIM_FRAME));
}

void figure_action_61(game_state *st)
{
    /* @bin 0x004AF4C0 */
    U8(st, fig(st) + FIG_STATE) = 2;
}

void figure_action_62(game_state *st)
{
    /* @bin 0x004AF4E0 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    S32(st, f + FIG_U180) = S32(st, G_IMAGE_GROUP_116);
    U8(st, f + FIG_TERRAIN_USAGE) = 1;
    U8(st, f + FIG_U0C) = 0;
    S16(st, f + FIG_MAX_ROAM_LENGTH) = S16(st, f + FIG_WAIT_TICKS);
    int16_t home = S16(st, f + FIG_HOME);
    next_frame(st, f);
    U8(st, f + FIG_INVISIBLE) = 0;
    switch (U8(st, f + FIG_ACTION)) {
    case 8:
        U8(st, f + FIG_INVISIBLE) = 1;
        U8(st, f + FIG_ANIM_FRAME) = 0;
        if (road_near(st, BUILDING_ADDR(home)) == 0) {
            U8(st, fig(st) + FIG_STATE) = 2;
            break;
        }
        id = cur(st);
        f = FIGURE_ADDR(id);
        U8(st, f + FIG_ACTION) = 9;
        S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
        S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
        move_to_dest(st, id, f);
        S16(st, fig(st) + FIG_ROAM_LENGTH) = 0;
        break;
    case 9:
        U8(st, f + FIG_U0C) = 1;
        U8(st, f + FIG_INVISIBLE) = 1;
        if (GAME_CALL(st, FN_FIGURE_STEP_STRAIGHT, id, 1) != 1) {
            break;
        }
        id = cur(st);
        f = FIGURE_ADDR(id);
        if (S16(st, G_BUILDING_GRID + 2u * U32(st, f + FIG_GRID_OFFSET)) == home) {
            U8(st, f + FIG_STATE) = 2;
            break;
        }
        U8(st, f + FIG_ACTION) = 0x0A;
        GAME_CALL(st, FN_FIGURE_WALKER_START, id);
        S16(st, fig(st) + FIG_ROAM_LENGTH) = 0;
        break;
    case 0x0A:
        U8(st, f + FIG_INVISIBLE) = 0;
        S16(st, f + FIG_ROAM_LENGTH) += 1;
        if (S16(st, f + FIG_ROAM_LENGTH) >= S16(st, f + FIG_MAX_ROAM_LENGTH)) {
            if (road_near(st, BUILDING_ADDR(home)) != 0) {
                /* @bin 0x004AF742 a road found: removed, without the sprite update */
                U8(st, fig(st) + FIG_STATE) = 2;
                return;
            }
            id = cur(st);
            U8(st, FIGURE_ADDR(id) + FIG_STATE) = 2;
        }
        GAME_CALL(st, FN_FIGURE_ROAM, id, 1);
        break;
    default:
        break;
    }
    f = fig(st);
    int32_t d = sprite_direction(st, S8(st, f + FIG_DIRECTION) < 8 ? S8(st, f + FIG_DIRECTION) : S8(st, f + FIG_PREV_DIRECTION));
    if (U8(st, f + FIG_ACTION) == 4) {
        S32(st, f + FIG_U04) = (int32_t) (dying_frame(st, f) + U32(st, G_IMAGE_GROUP_244));
        return;
    }
    S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_242) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
}

void figure_action_63(game_state *st)
{
    /* @bin 0x004AF7C0 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    /* @bin 0x004AF7DF terrain bits 0x10004 both set at the figure's tile */
    if ((U32(st, G_GRID_TERRAIN + 4u * U32(st, f + FIG_GRID_OFFSET)) & 0x10004u) == 0x10004u) {
        GAME_CALL(st, FN_FIGB_REMOVE, id);
        return;
    }
    int16_t r = S16(st, f + FIG_FORMATION);
    next_frame(st, f);
    S32(st, f + FIG_U180) = 0;
    U8(st, f + FIG_U59) = 2;
    if (U8(st, G_FORMATION_BASE + FM_HALTED + 0x90u * (uint32_t) r) != 0) { /* @bin 0x004AF830 */
        U8(st, f + FIG_STATE) = 2;
    }
    GAME_CALL0(st, FN_FIGB_SOLDIER_ACTION);
    id = cur(st);
    f = FIGURE_ADDR(id);
    uint8_t action = U8(st, f + FIG_ACTION);
    int32_t dir;
    if (action == 5) {
        dir = S8(st, f + FIG_ATTACK_DIRECTION);
    } else if (S8(st, f + FIG_DIRECTION) < 8) {
        dir = S8(st, f + FIG_DIRECTION);
    } else {
        dir = S8(st, f + FIG_PREV_DIRECTION);
    }
    int32_t d = sprite_direction(st, dir);
    if (action == 5) {
        GAME_CALL(st, FN_FIGURE_SOUND_53B4A0, id, 10, 2);
        f = fig(st);
        uint8_t a = U8(st, f + FIG_ATTACK_IMAGE);
        if (a < 0x0C) {
            S32(st, f + FIG_U04) = S32(st, G_SPRITE_DIRECTION) + S32(st, G_IMAGE_GROUP_592);
        } else {
            S32(st, f + FIG_U04) = S32(st, G_SPRITE_DIRECTION) + ((int32_t) a - 0x0C) / 2 * 8 + S32(st, G_IMAGE_GROUP_592);
        }
        return;
    }
    if (action == 4) {
        S32(st, f + FIG_U04) = (int32_t) (dying_frame(st, f) + U32(st, G_IMAGE_GROUP_591));
        return;
    }
    if (S8(st, f + FIG_DIRECTION) == 0x0B) {
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_592) + 8 * (U8(st, f + FIG_ANIM_FRAME) >> 1) + d;
        return;
    }
    S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_590) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
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

/* @bin 0x00561A36 sprintf(buffer, format, ...): the C runtime's (_output 0x00564BC6), as far as its
 * ported callers use it: literal text, %%, %d, and %s with its flags, width and precision. A buffer
 * outside the state (a caller's frame, passed as 0 by the ports) is not written: only the length is
 * returned. */
static int32_t run_sprintf(game_state *st, const int32_t *a)
{
    const uint8_t *fmt = state_ptr(st, (uint32_t) a[1], 1);
    char out[1024];
    int32_t n = 0, arg = 2;
    if (!fmt) {
        state_unported(st, FN_CRT_SPRINTF);
        return 0;
    }
    for (uint32_t i = 0;; i++) {
        const uint8_t *c = state_ptr(st, (uint32_t) a[1] + i, 1);
        if (!c || n >= (int32_t) sizeof(out) - 12) {
            state_unported(st, FN_CRT_SPRINTF);
            return 0;
        }
        if (*c == 0) {
            break;
        }
        if (*c != '%') {
            out[n++] = (char) *c;
            continue;
        }
        /* @bin 0x00564BC6 the conversion: flags ' ' '+' '-' '0' '#', width (digits or '*': a negative one
         * is '-'), precision ('.' then digits or '*': negative is none), size 'h' / 'l' / 'w', type */
        uint32_t flags = 0;
        int32_t width = 0, precision = -1;
        const uint8_t *d;
        for (;;) {
            d = state_ptr(st, (uint32_t) a[1] + ++i, 1);
            if (!d) {
                state_unported(st, FN_CRT_SPRINTF);
                return 0;
            }
            if (*d == ' ') {
                flags |= 2;
            } else if (*d == '+') {
                flags |= 1;
            } else if (*d == '-') {
                flags |= 4;
            } else if (*d == '0') {
                flags |= 8;
            } else if (*d == '#') {
                flags |= 0x80;
            } else {
                break;
            }
        }
        if (*d == '*') {
            if (arg >= 15) {
                state_unported(st, FN_CRT_SPRINTF);
                return 0;
            }
            width = a[arg++];
            if (width < 0) {
                flags |= 4;
                width = -width;
            }
            d = state_ptr(st, (uint32_t) a[1] + ++i, 1);
        } else {
            while (d && *d >= '0' && *d <= '9') {
                width = width * 10 + (*d - '0');
                d = state_ptr(st, (uint32_t) a[1] + ++i, 1);
            }
        }
        if (d && *d == '.') {
            precision = 0;
            d = state_ptr(st, (uint32_t) a[1] + ++i, 1);
            if (d && *d == '*') {
                if (arg >= 15) {
                    state_unported(st, FN_CRT_SPRINTF);
                    return 0;
                }
                precision = a[arg++];
                if (precision < 0) {
                    precision = -1;
                }
                d = state_ptr(st, (uint32_t) a[1] + ++i, 1);
            } else {
                while (d && *d >= '0' && *d <= '9') {
                    precision = precision * 10 + (*d - '0');
                    d = state_ptr(st, (uint32_t) a[1] + ++i, 1);
                }
            }
        }
        int narrow_size = 0;
        while (d && *d == 'h') { /* 'h': a narrow string, as without a size */
            narrow_size = 1;
            d = state_ptr(st, (uint32_t) a[1] + ++i, 1);
        }
        if (!d || arg >= 15) {
            state_unported(st, FN_CRT_SPRINTF);
            return 0;
        }
        if (*d == '%' && flags == 0 && width == 0 && precision < 0 && !narrow_size) {
            out[n++] = '%';
            continue;
        }
        if (*d == 's') {
            /* @bin 0x00564DF3 the string (NULL: "(null)", 0x0057075C) up to the precision (none:
             * 0x7FFFFFFF); @bin 0x00565208 padded to the width: spaces before (no '-' nor '0'), zeros
             * before ('0' without '-'), or spaces after ('-') */
            uint32_t sa = (uint32_t) a[arg++];
            const char *text = "(null)";
            uint32_t limit = precision < 0 ? 0x7FFFFFFFu : (uint32_t) precision;
            int32_t len = 0;
            if (sa != 0) {
                text = NULL;
            }
            for (;; len++) {
                if ((uint32_t) len >= limit) {
                    break;
                }
                const uint8_t *ch = text ? (const uint8_t *) text + len : state_ptr(st, sa + (uint32_t) len, 1);
                if (!ch) {
                    state_unported(st, FN_CRT_SPRINTF); /* a string the port cannot see */
                    return 0;
                }
                if (*ch == 0) {
                    break;
                }
            }
            int32_t pad = width - len;
            if (n + len + (pad > 0 ? pad : 0) >= (int32_t) sizeof(out) - 12) {
                state_unported(st, FN_CRT_SPRINTF);
                return 0;
            }
            if ((flags & 0xC) == 0) {
                for (int32_t k = 0; k < pad; k++) {
                    out[n++] = ' ';
                }
            }
            if ((flags & 8) && !(flags & 4)) {
                for (int32_t k = 0; k < pad; k++) {
                    out[n++] = '0';
                }
            }
            for (int32_t k = 0; k < len; k++) {
                out[n++] = text ? text[k] : (char) *state_ptr(st, sa + (uint32_t) k, 1);
            }
            if (flags & 4) {
                for (int32_t k = 0; k < pad; k++) {
                    out[n++] = ' ';
                }
            }
            continue;
        }
        if (*d != 'd' || flags != 0 || width != 0 || precision >= 0 || narrow_size) {
            state_unported(st, FN_CRT_SPRINTF); /* other conversions: not ported */
            return 0;
        }
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
        state_unported(st, FN_CRT_SPRINTF); /* a buffer the port cannot see */
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
    labor_assign(st, G_UD05720, a[0]);
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
    return labor_wanted_anywhere(st, G_UD05720);
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
    {0x004BEEB0u, "labor_assign", 1, 0, G_UD05720, run_labor_assign, 0},
    {0x004BF920u, "labor_arrived", 1, 0, G_UD05720, run_labor_arrived, 0},
    {0x004BF9D0u, "labor_wanted_anywhere", 0, 8, G_UD05720, run_labor_wanted, 0},
    {0x004F3360u, "pyramid_level_next", 1, 32, 0, run_level_next, 0},
    /* 0x004F3370 returns the slot through two pointers: ported for its callers only */
    {0x004F3370u, "pyramid_block_slot", 4, 8, 0, NULL, 0},
    /* partial: the formats of its ported callers only */
    {0x00561A36u, "crt_sprintf", 2, 32, 0, run_sprintf, 1},
    /* the CRT's formatter and its helpers, inside crt_sprintf (on a string stream: no flush) */
    {0x00564BC6u, "CRT _output (crt_sprintf)", 3, 32, 0, NULL, 0},
    {0x00565307u, "CRT _output write_char (crt_sprintf)", 3, 0, 0, NULL, 0},
    {0x0056533Cu, "CRT _output write_multi_char (crt_sprintf)", 4, 0, 0, NULL, 0},
    {0x0056536Du, "CRT _output write_string (crt_sprintf)", 4, 0, 0, NULL, 0},
    {0x005653A5u, "CRT _output get_int_arg (crt_sprintf)", 1, 32, 0, NULL, 0},
    {0x005653B2u, "CRT _output get_int64_arg (crt_sprintf)", 1, 32, 0, NULL, 0},
    {0x005653C2u, "CRT _output get_short_arg (crt_sprintf)", 1, 16, 0, NULL, 0},
    {0x00563380u, "CRT strlen (crt_sprintf)", 1, 32, 0, NULL, 0},
    {0x00569250u, "CRT _aulldiv (crt_sprintf %d)", 4, 32, 0, NULL, 0},
    {0x005692C0u, "CRT _aullrem (crt_sprintf %d)", 4, 32, 0, NULL, 0},
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
