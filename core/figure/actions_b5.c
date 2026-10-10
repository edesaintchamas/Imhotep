/* Imhotep - figure actions, group b: figure type 0x52.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21). */
#include "figure/actions_b.h"

#include "random/random.h"
#include "sim/call.h"
#include "state/layout.h"


/* @bin 0x0049D13D the tile and its 8 neighbours all have terrain bit 0x04 (water) */
static int in_open_water(game_state *st, int32_t off)
{
    static const int32_t around[9] = {0, -228, 1, 228, -1, -227, -229, 229, 227};
    for (int i = 0; i < 9; i++) {
        if ((U8(st, G_GRID_TERRAIN + 4u * (uint32_t) (off + around[i])) & 4) == 0) {
            return 0;
        }
    }
    return 1;
}

/* @bin 0x0049D444 a random tile around a point of the herd record (0x004EAB10, offsets at
 * 0x005CD978); returns 0 when none was found */
static int wander_to(game_state *st, uint32_t rec)
{
    random_next_state(st);
    uint32_t f = FIGB_F(st);
    int32_t r = GAME_CALL(st, FN_HERD_SPOT, S16(st, rec), S16(st, rec + 2), (int32_t) (U32(st, G_RANDOM_R1_15BIT) & 0xF),
                          S16(st, f + FIG_X), S16(st, f + FIG_Y), 0);
    if (r < 0 || r >= 0x10) {
        return 0;
    }
    GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
    f = FIGB_F(st);
    S16(st, f + FIG_DEST_X) = (int16_t) (S16(st, G_DIRECTIONS + 8u * (uint32_t) r) + S16(st, rec));
    S16(st, f + FIG_DEST_Y) = (int16_t) (S16(st, G_DIRECTIONS_DY + 8u * (uint32_t) r) + S16(st, rec + 2));
    U8(st, f + FIG_ACTION) = 9;
    return 1;
}

/* @bin 0x0049D9E7 arrived or lost: a pause of +0x62 * 5 ticks (action 0x17) */
static void to_pause(game_state *st, uint32_t f)
{
    U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
    U8(st, f + FIG_ACTION) = 0x17;
    S16(st, f + FIG_WAIT_TICKS) = (int16_t) (uint16_t) (U8(st, f + FIG_U62) * 5);
}

/* @bin 0x0049D895 follow the target figure (+0x5E) when it has moved */
static void retarget(game_state *st, int32_t target)
{
    uint32_t t = FIGURE_ADDR(target);
    uint32_t f = FIGB_F(st);
    if (S16(st, t + FIG_X) != S16(st, f + FIG_DEST_X) || S16(st, t + FIG_Y) != S16(st, f + FIG_DEST_Y)) {
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
        f = FIGB_F(st);
        U16(st, f + FIG_DEST_X) = U16(st, t + FIG_X);
        U16(st, f + FIG_DEST_Y) = U16(st, t + FIG_Y);
    }
}

void figb_action_52(game_state *st)
{
    /* @bin 0x0049D0F0 a water animal of a herd (+0x60): slower in open water, which also changes
     * its images; hunts a figure (+0x5E) in action 0x15 */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    int stepped = 0;
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    S32(st, f + FIG_FRAME_TICKS) += 1;
    int water = in_open_water(st, S32(st, f + FIG_GRID_OFFSET));
    if (water) {
        S32(st, f + FIG_FRAME_PERIOD) = 0x0B;
    }
    if (S32(st, f + FIG_FRAME_TICKS) >= S32(st, f + FIG_FRAME_PERIOD)) {
        S32(st, f + FIG_FRAME_TICKS) = 0;
        U8(st, f + FIG_ANIM_FRAME) += 1;
        stepped = 1;
    }
    U8(st, f + FIG_TERRAIN_USAGE) = 6;
    S16(st, CURRENT_CITY(st) + CITY_U447A) += 1; /* 0x00EA5CEA */
    U8(st, f + FIG_U0C) = 0;
    U8(st, f + FIG_INVISIBLE) = 0;
    int32_t herd = S16(st, f + FIG_FORMATION);
    figb_anim_advance(st, f, 12);
    if (water) {
        speed >>= 1;
    }
    uint32_t rec = G_FORMATION_BASE + FM_U36 + 0x90u * (uint32_t) herd;
    int8_t dir;
    switch (U8(st, f + FIG_ACTION)) { /* @bin 0x0049DC04 */
    case 4:
        S32(st, f + FIG_FRAME_PERIOD) = 8;
        GAME_CALL(st, FN_FIGURE_DYING, id);
        break;
    case 5:
        GAME_CALL(st, FN_FIGURE_SOUND_53B4A0, id, 7, 2);
        S32(st, FIGB_F(st) + FIG_FRAME_PERIOD) = 7;
        GAME_CALL(st, FN_FIGURE_FIGHT, FIGB_CUR(st));
        break;
    case 8:
        GAME_CALL(st, FN_ANIMAL_CHOOSE_ACTION, id);
        break;
    case 9:
    case 0x0A:
    case 0x13:
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + FIG_DIRECTION);
        if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
        } else if (dir == 10 || dir == 8) {
            U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
            GAME_CALL(st, FN_ANIMAL_CHOOSE_ACTION, FIGB_CUR(st));
        }
        break;
    case 0x0C:
        if (S16(st, f + FIG_WAIT_TICKS) == 0) {
            GAME_CALL(st, FN_FIGURE_HIT_SOUND, id);
        }
        f = FIGB_F(st);
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 500) {
            U8(st, f + FIG_ACTION) = 4;
        }
        break;
    case 0x0D:
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        GAME_CALL(st, FN_ANIMAL_SEEK_PREY, FIGB_CUR(st));
        f = FIGB_F(st);
        dir = S8(st, f + FIG_DIRECTION);
        if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
        } else if (dir == 10 || dir == 8) {
            to_pause(st, f);
        }
        break;
    case 0x12:
        S16(st, f + FIG_WAIT_TICKS) -= 1;
        if (S16(st, f + FIG_WAIT_TICKS) <= 0) {
            if (!wander_to(st, rec + 4)) {
                S16(st, FIGB_F(st) + FIG_WAIT_TICKS) = 100;
            }
        }
        if (stepped) {
            GAME_CALL(st, FN_ANIMAL_SEEK_PREY, FIGB_CUR(st));
        }
        break;
    case 0x14:
        GAME_CALL(st, FN_ANIMAL_SEEK_PREY, id);
        break;
    case 0x15: {
        int32_t target = S16(st, f + FIG_DEST_BUILDING);
        S16(st, f + FIG_WAIT_TICKS) -= 1;
        if (S16(st, f + FIG_WAIT_TICKS) <= 0) {
            U8(st, f + FIG_U6B) = 0;
            U8(st, f + FIG_ACTION) = 0x17;
            break;
        }
        if (target != 0) {
            retarget(st, target);
        }
        GAME_CALL(st, FN_FIGURE_MOVE, FIGB_CUR(st), speed);
        f = FIGB_F(st);
        dir = S8(st, f + FIG_DIRECTION);
        if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
        } else if (dir == 10) {
            to_pause(st, f);
        } else if (dir == 8) {
            uint32_t t = FIGURE_ADDR(target);
            if (target != 0 && (S16(st, t + FIG_X) != S16(st, f + FIG_DEST_X) || S16(st, t + FIG_Y) != S16(st, f + FIG_DEST_Y))) {
                retarget(st, target);
            } else {
                to_pause(st, f);
            }
        }
        break;
    }
    case 0x17:
        if (!wander_to(st, rec + 4) && !wander_to(st, rec)) {
            U8(st, FIGB_F(st) + FIG_STATE) = 2;
            return;
        }
        break;
    case 0x18:
        S16(st, f + FIG_WAIT_TICKS) -= 1;
        if (S16(st, f + FIG_WAIT_TICKS) <= 0) {
            S32(st, f + FIG_FRAME_TICKS) = 0;
            S32(st, f + FIG_FRAME_PERIOD) = 8;
            U8(st, f + FIG_ACTION) = 8;
        }
        U32(st, f + FIG_U04) = 0;
        return;
    default:
        U8(st, f + FIG_ACTION) = 8;
        break;
    }
    int32_t d = figb_dir_rel(st);
    f = FIGB_F(st);
    uint32_t fr = 8u * U32(st, f + FIG_FRAME_TICKS) + (uint32_t) d;
    switch (U8(st, f + FIG_ACTION)) { /* @bin 0x0049DC34 */
    case 4: {
        int16_t t = S16(st, f + FIG_WAIT_TICKS);
        if (water) {
            U32(st, f + FIG_U04) = t >= 0x17 ? U32(st, G_IMAGE_GROUP_588) + 0x16u : (uint32_t) t + U32(st, G_IMAGE_GROUP_588);
        } else {
            U32(st, f + FIG_U04) = t >= 8 ? U32(st, G_IMAGE_GROUP_63) + 7u : (uint32_t) t + U32(st, G_IMAGE_GROUP_63);
        }
        break;
    }
    case 5:
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_64) + fr;
        break;
    case 8:
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_62) + (uint32_t) d;
        break;
    case 9:
    case 0x0A:
    case 0x13:
    case 0x15:
        U32(st, f + FIG_U04) = U32(st, water ? G_IMAGE_GROUP_65 : G_IMAGE_GROUP_62) + fr;
        break;
    case 0x17:
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_62) + fr;
        break;
    default:
        U32(st, f + FIG_U04) = water ? U32(st, G_IMAGE_GROUP_66) + fr : U32(st, G_IMAGE_GROUP_62) + (uint32_t) d;
        break;
    }
}
