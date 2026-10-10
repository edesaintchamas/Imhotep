/* Imhotep - figure actions, group b: figure types 0x43, 0x44.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21). */
#include "figure/actions_b.h"

#include "random/random.h"
#include "sim/call.h"
#include "state/layout.h"

void figb_action_43(game_state *st)
{
    /* @bin 0x0049B610 a 36-frame animation placed once (+0x36 < 1000) on a tile from 0x00480E90 */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_INVISIBLE) = 0;
    U8(st, f + FIG_HEIGHT_TICKS) = 0;
    U8(st, f + FIG_BOAT_MODE) = 1;
    uint8_t a = (uint8_t) (U8(st, f + FIG_ANIM_FRAME) + 1);
    U8(st, f + FIG_ANIM_FRAME) = a;
    if (a > 0x23) {
        U8(st, f + FIG_STATE) = 2;
    }
    if (S16(st, f + FIG_WAIT_TICKS) < 1000) {
        GAME_CALL(st, FN_FIGURE_TILE_LIST_REMOVE, id);
        int32_t r = GAME_CALL(st, FN_FIGURE_FREE_WATER_480E90, FIGB_CUR(st));
        id = FIGB_CUR(st);
        f = FIGURE_ADDR(id);
        if (r != 0) {
            U16(st, f + FIG_X) = U16(st, G_ROAD_ACCESS_X);
            int16_t y = S16(st, G_ROAD_ACCESS_Y);
            S16(st, f + FIG_Y) = y;
            U32(st, f + FIG_GRID_OFFSET) = U32(st, G_MAP_START) + (uint32_t) S16(st, f + FIG_X) + 228u * (uint32_t) y;
            S16(st, f + FIG_CC_X) = (int16_t) (S16(st, f + FIG_X) * 15);
            S16(st, f + FIG_CC_Y) = (int16_t) (S16(st, f + FIG_Y) * 15);
            S16(st, f + FIG_CC_X) += 7;
            S16(st, f + FIG_CC_Y) += 7;
        }
        GAME_CALL(st, FN_FIGURE_TILE_LIST_ADD, id);
        f = FIGB_F(st);
        S16(st, f + FIG_WAIT_TICKS) = 1000;
    }
    U32(st, f + FIG_U04) = U8(st, f + FIG_ANIM_FRAME) + U32(st, G_IMAGE_GROUP_577);
}

void figb_action_44(game_state *st)
{
    /* @bin 0x0049B750 an animal of a herd (+0x60, records of 0x90 bytes at 0x00BCB9C8): waits,
     * walks to a random tile around the herd point (0x004EAB10), flees, is hunted */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    S32(st, f + FIG_FRAME_TICKS) += 1;
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    int32_t hb = S32(st, f + FIG_U138);
    uint32_t hba = BUILDING_ADDR(hb);
    if (S16(st, hba + BLD_TYPE) != 0x73 || U8(st, hba + BLD_SHOWS) == 0 || U8(st, hba + BLD_STATE) != 1 || S16(st, hba + BLD_WORKERS) == 0) {
        S32(st, f + FIG_U138) = 0;
    }
    if (S32(st, f + FIG_FRAME_TICKS) >= S32(st, f + FIG_FRAME_PERIOD)) {
        S32(st, f + FIG_FRAME_TICKS) = 0;
        U8(st, f + FIG_ANIM_FRAME) += 1;
    }
    U8(st, f + FIG_TERRAIN_USAGE) = 5;
    S16(st, CURRENT_CITY(st) + CITY_U447A) += 1; /* 0x00EA5CEA: animals counted */
    U8(st, f + FIG_U0C) = 0;
    U8(st, f + FIG_INVISIBLE) = 0;
    int32_t herd = S16(st, f + FIG_FORMATION);
    figb_anim_advance(st, f, 0x32);
    int8_t dir;
    switch (U8(st, f + FIG_ACTION)) { /* @bin 0x0049BD0C */
    case 4:
        GAME_CALL(st, FN_FIGURE_DYING, id);
        break;
    case 5:
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        break;
    case 8:
    case 0x12: {
        S16(st, f + FIG_WAIT_TICKS) -= 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0) {
            break;
        }
        random_next_state(st);
        uint32_t rec = G_FORMATION_BASE + FM_U36 + 0x90u * (uint32_t) herd;
        int32_t r = GAME_CALL(st, FN_HERD_SPOT, S16(st, rec), S16(st, rec + 2), (int32_t) (U32(st, G_R_1_15) & 0xF),
                              S16(st, f + FIG_X), S16(st, f + FIG_Y), 1);
        f = FIGB_F(st);
        if (r >= 0 && r < 0x10) {
            /* @bin 0x005CDB78 offsets (x, y) by direction, 8 bytes each */
            S16(st, f + FIG_DEST_X) = (int16_t) (S16(st, G_RING16_DX + 8u * (uint32_t) r) + S16(st, rec));
            S16(st, f + FIG_DEST_Y) = (int16_t) (S16(st, G_RING16_DY + 8u * (uint32_t) r) + S16(st, rec + 2));
            U8(st, f + FIG_ACTION) = 0x0A;
        } else {
            S16(st, f + FIG_WAIT_TICKS) = 100;
        }
        break;
    }
    case 9:
    case 0x0A:
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + FIG_DIRECTION);
        if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
        } else if (dir == 10) {
            U8(st, f + FIG_STATE) = 2;
        } else if (dir == 8) {
            U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
            U8(st, f + FIG_ACTION) = 0x12;
            random_next_state(st);
            S16(st, f + FIG_WAIT_TICKS) = (int16_t) (S32(st, G_R_1_15) % 0x96 + 0x96);
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
    case 0x0E:
    case 0x0F:
        GAME_CALL(st, FN_FIGB_ANIMAL_FLEE, id);
        break;
    case 0x10:
    case 0x11:
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed * 2);
        f = FIGB_F(st);
        dir = S8(st, f + FIG_DIRECTION);
        if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
        } else if (dir == 10 || dir == 8) {
            U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
            U8(st, f + FIG_ACTION) = 0x0E;
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
    figb_dir_rel(st);
    f = FIGB_F(st);
    uint8_t act = U8(st, f + FIG_ACTION);
    if (act == 4 || act == 0x0C) { /* @bin 0x0049BD30 */
        int16_t t = S16(st, f + FIG_WAIT_TICKS);
        U32(st, f + FIG_U04) = t >= 0x19 ? 0u : (uint32_t) (t + (int32_t) U32(st, G_IMAGE_GROUP_154));
        return;
    }
    U32(st, f + FIG_U04) = (uint32_t) (U8(st, f + FIG_ANIM_FRAME) >> 1) + U32(st, G_IMAGE_GROUP_153);
}

/* @bin 0x0049BF3A the walk after a pause: 0x12 when bit 0 of the random value is set, else 0x13 */
static uint8_t pause_kind(game_state *st)
{
    return (U32(st, G_R_1_15) & 1) ? 0x12 : 0x13;
}

/* @bin 0x0049C3E7 a pause of 150..299 ticks */
static void pause_random(game_state *st, uint32_t f)
{
    S16(st, f + FIG_WAIT_TICKS) = (int16_t) (S32(st, G_R_1_15) % 0x96 + 0x96);
}

void figb_action_45(game_state *st)
{
    /* @bin 0x0049BD40 an animal of a herd (+0x60, point at +4/+6 of the 0x90-byte records at
     * 0x00BCB9FE): grazes, wanders, flees, is hunted. +0x148 counts ticks per frame (+0x144). */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    int stepped = 0;
    U8(st, f + FIG_TERRAIN_USAGE) = 5;
    U8(st, f + FIG_U0C) = 0;
    U8(st, f + FIG_INVISIBLE) = 0;
    S16(st, CURRENT_CITY(st) + CITY_U447A) += 1; /* 0x00EA5CEA */
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    int32_t herd = S16(st, f + FIG_FORMATION);
    S32(st, f + FIG_FRAME_TICKS) += 1;
    int32_t hb = S32(st, f + FIG_U138);
    uint32_t hba = BUILDING_ADDR(hb);
    if (S16(st, hba + BLD_TYPE) != 0x73 || U8(st, hba + BLD_SHOWS) == 0 || U8(st, hba + BLD_STATE) != 1 || S16(st, hba + BLD_WORKERS) == 0) {
        S32(st, f + FIG_U138) = 0;
    }
    if (S32(st, f + FIG_FRAME_TICKS) >= S32(st, f + FIG_FRAME_PERIOD)) {
        S32(st, f + FIG_FRAME_TICKS) = 0;
        U8(st, f + FIG_ANIM_FRAME) += 1;
        stepped = 1;
    }
    int8_t dir;
    int to_eb000 = 0; /* @bin 0x0049C407 */
    switch (U8(st, f + FIG_ACTION)) { /* @bin 0x0049C704 */
    case 4:
        S32(st, f + FIG_FRAME_PERIOD) = 8;
        GAME_CALL(st, FN_FIGURE_DYING, id);
        break;
    case 5:
        S32(st, f + FIG_FRAME_PERIOD) = 7;
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        break;
    case 8:
        random_next_state(st);
        U8(st, f + FIG_ACTION) = pause_kind(st);
        S32(st, f + FIG_FRAME_TICKS) = 0;
        S32(st, f + FIG_FRAME_PERIOD) = 8;
        random_next_state(st);
        pause_random(st, f);
        return;
    case 9:
    case 0x0A:
        S32(st, f + FIG_FRAME_PERIOD) = 0x0C;
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + FIG_DIRECTION);
        if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
            f = FIGB_F(st);
            U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
        } else if (dir == 10) {
            U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
            U8(st, f + FIG_ACTION) = 8;
            S16(st, f + FIG_WAIT_TICKS) = (int16_t) (U8(st, f + FIG_U62) * 5);
            S32(st, f + FIG_FRAME_TICKS) = 0;
            return;
        } else if (dir == 8) {
            U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
            random_next_state(st);
            U8(st, f + FIG_ACTION) = pause_kind(st);
            random_next_state(st);
            pause_random(st, f);
        }
        to_eb000 = 1;
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
    case 0x0E:
    case 0x0F:
        S32(st, f + FIG_FRAME_PERIOD) = 1;
        GAME_CALL(st, FN_FIGB_ANIMAL_FLEE, id);
        break;
    case 0x10:
    case 0x11:
        S32(st, f + FIG_FRAME_PERIOD) = 0x0C;
        S32(st, f + FIG_FRAME_TICKS) += 1;
        if (S32(st, f + FIG_FRAME_TICKS) >= 0x0C) {
            S32(st, f + FIG_FRAME_TICKS) = 0;
        }
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed * 2);
        f = FIGB_F(st);
        dir = S8(st, f + FIG_DIRECTION);
        if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
            f = FIGB_F(st);
            U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
            U8(st, f + FIG_ACTION) = 0x0E;
        } else if (dir == 10) {
            U8(st, f + FIG_STATE) = 2;
            return;
        } else if (dir == 8) {
            U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
            U8(st, f + FIG_ACTION) = 0x0E;
            S32(st, f + FIG_FRAME_TICKS) = 0;
            return;
        }
        break;
    case 0x12: {
        S32(st, f + FIG_FRAME_PERIOD) = 8;
        int16_t t = (int16_t) (S16(st, f + FIG_WAIT_TICKS) - 1);
        S16(st, f + FIG_WAIT_TICKS) = t;
        if (t < 0) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
        }
        if (t > 0) {
            to_eb000 = 1;
            break;
        }
        if (!stepped) {
            break;
        }
        random_next_state(st);
        uint32_t rec = G_FORMATION_BASE + FM_DEST_X + 0x90u * (uint32_t) herd;
        int32_t r = GAME_CALL(st, FN_HERD_SPOT, S16(st, rec), S16(st, rec + 2), (int32_t) (U32(st, G_R_1_15) & 0xF),
                              S16(st, f + FIG_X), S16(st, f + FIG_Y), 1);
        f = FIGB_F(st);
        if (r >= 0 && r < 0x10) {
            S16(st, f + FIG_DEST_X) = (int16_t) (S16(st, G_RING16_DX + 8u * (uint32_t) r) + S16(st, rec));
            S16(st, f + FIG_DEST_Y) = (int16_t) (S16(st, G_RING16_DY + 8u * (uint32_t) r) + S16(st, rec + 2));
            U8(st, f + FIG_ACTION) = 0x0A;
            return;
        }
        S16(st, f + FIG_WAIT_TICKS) = 100;
        to_eb000 = 1;
        break;
    }
    case 0x13:
        S32(st, f + FIG_FRAME_PERIOD) = 8;
        S16(st, f + FIG_WAIT_TICKS) -= 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0) {
            to_eb000 = 1;
            break;
        }
        if (stepped) {
            U8(st, f + FIG_ACTION) = 0x12;
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
    if (to_eb000 && stepped) {
        GAME_CALL(st, FN_FIGB_ANIMAL_FLEE, FIGB_CUR(st));
    }
    /* @bin 0x0049C547 direction kept in 0..7 */
    f = FIGB_F(st);
    int32_t d = S8(st, f + FIG_DIRECTION) - S32(st, G_VIEW_ORIENTATION);
    S32(st, G_SPRITE_DIRECTION) = d;
    if (d < 0) {
        d += 8;
        S32(st, G_SPRITE_DIRECTION) = d;
        if (d < 0) {
            d += (int32_t) (((uint32_t) (7 - d) >> 3) * 8u);
            S32(st, G_SPRITE_DIRECTION) = d;
        }
    }
    if (d >= 8) {
        d -= (int32_t) (((uint32_t) d >> 3) * 8u);
        S32(st, G_SPRITE_DIRECTION) = d;
    }
    if (S32(st, f + FIG_FRAME_TICKS) > S32(st, f + FIG_FRAME_PERIOD)) {
        S32(st, f + FIG_FRAME_TICKS) -= S32(st, f + FIG_FRAME_PERIOD);
    }
    if (S32(st, f + FIG_FRAME_TICKS) < 0) {
        S32(st, f + FIG_FRAME_TICKS) = 0;
    }
    uint32_t fr = 8u * U32(st, f + FIG_FRAME_TICKS) + (uint32_t) d;
    switch (U8(st, f + FIG_ACTION)) { /* @bin 0x0049C738 */
    case 4:
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_196) + 7u;
        break;
    case 5:
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_197) + fr;
        break;
    case 8:
    case 0x12:
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_198) + fr;
        break;
    case 0x0C: {
        int16_t t = S16(st, f + FIG_WAIT_TICKS);
        U32(st, f + FIG_U04) = t >= 8 ? U32(st, G_IMAGE_GROUP_196) + 7u : (uint32_t) t + U32(st, G_IMAGE_GROUP_196);
        break;
    }
    case 0x0E:
    case 0x0F:
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_199) + (uint32_t) d;
        break;
    case 0x13:
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_199) + fr;
        break;
    default:
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_195) + fr;
        break;
    }
}

void figb_action_46(game_state *st)
{
    /* @bin 0x0049C750 another herd animal, as 0x0049BD40 with its own moves and images */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    int stepped = 0;
    U8(st, f + FIG_TERRAIN_USAGE) = 5;
    U8(st, f + FIG_U0C) = 0;
    U8(st, f + FIG_INVISIBLE) = 0;
    S16(st, CURRENT_CITY(st) + CITY_U447A) += 1; /* 0x00EA5CEA */
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    int32_t herd = S16(st, f + FIG_FORMATION);
    S32(st, f + FIG_FRAME_TICKS) += 1;
    int32_t hb = S32(st, f + FIG_U138);
    uint32_t hba = BUILDING_ADDR(hb);
    if (S16(st, hba + BLD_TYPE) != 0x73 || U8(st, hba + BLD_SHOWS) == 0 || U8(st, hba + BLD_STATE) != 1 || S16(st, hba + BLD_WORKERS) == 0) {
        S32(st, f + FIG_U138) = 0;
    }
    if (S32(st, f + FIG_FRAME_TICKS) >= S32(st, f + FIG_FRAME_PERIOD)) {
        S32(st, f + FIG_FRAME_TICKS) = 0;
        U8(st, f + FIG_ANIM_FRAME) += 1;
        stepped = 1;
    }
    int8_t dir;
    int to_eb000 = 0; /* @bin 0x0049CBD2 */
    switch (U8(st, f + FIG_ACTION)) { /* @bin 0x0049D0A8 */
    case 4:
        S32(st, f + FIG_FRAME_PERIOD) = 8;
        GAME_CALL(st, FN_FIGURE_DYING, id);
        break;
    case 5:
        S32(st, f + FIG_FRAME_PERIOD) = 7;
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        break;
    case 8:
        random_next_state(st);
        U8(st, f + FIG_ACTION) = pause_kind(st);
        S32(st, f + FIG_FRAME_TICKS) = 0;
        S32(st, f + FIG_FRAME_PERIOD) = 8;
        random_next_state(st);
        pause_random(st, f);
        return;
    case 9:
    case 0x0A:
        S32(st, f + FIG_FRAME_PERIOD) = 0x0C;
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + FIG_DIRECTION);
        if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
            to_eb000 = 1;
        } else if (dir == 10) {
            U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
            U8(st, f + FIG_ACTION) = 8;
            S16(st, f + FIG_WAIT_TICKS) = (int16_t) (U8(st, f + FIG_U62) * 5);
            S32(st, f + FIG_FRAME_TICKS) = 0;
            return;
        } else if (dir == 8) {
            U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
            random_next_state(st);
            U8(st, f + FIG_ACTION) = pause_kind(st);
            random_next_state(st);
            pause_random(st, f);
        } else {
            to_eb000 = 1;
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
    case 0x0E:
    case 0x0F:
        S32(st, f + FIG_FRAME_PERIOD) = 1;
        GAME_CALL(st, FN_FIGB_ANIMAL_FLEE, id);
        break;
    case 0x10:
    case 0x11:
        S32(st, f + FIG_FRAME_PERIOD) = 8;
        S32(st, f + FIG_FRAME_TICKS) += 1;
        if (S32(st, f + FIG_FRAME_TICKS) >= 0x0C) {
            S32(st, f + FIG_FRAME_TICKS) = 0;
        }
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed * 2);
        f = FIGB_F(st);
        dir = S8(st, f + FIG_DIRECTION);
        if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
        } else if (dir == 10 || dir == 8) {
            U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
            U8(st, f + FIG_ACTION) = 0x0E;
            S32(st, f + FIG_FRAME_TICKS) = 0;
            return;
        }
        break;
    case 0x12: {
        S32(st, f + FIG_FRAME_PERIOD) = 8;
        int16_t t = (int16_t) (S16(st, f + FIG_WAIT_TICKS) - 1);
        S16(st, f + FIG_WAIT_TICKS) = t;
        if (t < 0) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
        }
        if (t > 0) {
            to_eb000 = 1;
            break;
        }
        if (!stepped) {
            break;
        }
        random_next_state(st);
        uint32_t rec = G_FORMATION_BASE + FM_DEST_X + 0x90u * (uint32_t) herd;
        int32_t r = GAME_CALL(st, FN_HERD_SPOT, S16(st, rec), S16(st, rec + 2), (int32_t) (U32(st, G_R_1_15) & 0xF),
                              S16(st, f + FIG_X), S16(st, f + FIG_Y), 1);
        f = FIGB_F(st);
        if (r >= 0 && r < 0x10) {
            S16(st, f + FIG_DEST_X) = (int16_t) (S16(st, G_RING16_DX + 8u * (uint32_t) r) + S16(st, rec));
            S16(st, f + FIG_DEST_Y) = (int16_t) (S16(st, G_RING16_DY + 8u * (uint32_t) r) + S16(st, rec + 2));
            U8(st, f + FIG_ACTION) = 0x0A;
            return;
        }
        S16(st, f + FIG_WAIT_TICKS) = 100;
        to_eb000 = 1;
        break;
    }
    case 0x13:
        S32(st, f + FIG_FRAME_PERIOD) = 8;
        S16(st, f + FIG_WAIT_TICKS) -= 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0) {
            to_eb000 = 1;
            break;
        }
        if (stepped) {
            U8(st, f + FIG_ACTION) = 0x12;
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
        U8(st, f + FIG_INVISIBLE) = 1;
        return;
    default:
        U8(st, f + FIG_ACTION) = 8;
        break;
    }
    if (to_eb000 && stepped) {
        GAME_CALL(st, FN_FIGB_ANIMAL_FLEE, FIGB_CUR(st));
    }
    int32_t d = figb_dir_rel(st);
    f = FIGB_F(st);
    uint32_t fr = 8u * U32(st, f + FIG_FRAME_TICKS) + (uint32_t) d;
    switch (U8(st, f + FIG_ACTION)) { /* @bin 0x0049D0E0 */
    case 4:
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_608) + 7u;
        break;
    case 5:
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_609) + fr;
        break;
    case 8:
    case 0x0E:
    case 0x0F:
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_607) + (uint32_t) d;
        break;
    case 0x0C: {
        int16_t t = S16(st, f + FIG_WAIT_TICKS);
        U32(st, f + FIG_U04) = t >= 8 ? U32(st, G_IMAGE_GROUP_608) + 7u : (uint32_t) t + U32(st, G_IMAGE_GROUP_608);
        break;
    }
    case 0x10:
    case 0x11:
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_612) + fr;
        break;
    case 0x12:
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_610) + fr;
        break;
    case 0x13:
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_611) + fr;
        break;
    default:
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_607) + fr;
        break;
    }
}
