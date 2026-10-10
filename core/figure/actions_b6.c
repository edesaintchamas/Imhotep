/* Imhotep - figure actions, group b: soldiers (types 0x2F-0x36), around the shared soldier
 * action 0x004991A0.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21). */
#include "figure/actions_b.h"

#include "sim/call.h"
#include "state/layout.h"


/* @bin 0x004986B0 frame, +0x180 cleared, the formation mode at +0x59, then 0x004991A0 */
static void soldier_step(game_state *st, uint8_t mode)
{
    uint32_t f = FIGB_F(st);
    figb_anim_advance(st, f, 12);
    U32(st, f + FIG_U180) = 0;
    U8(st, f + FIG_U59) = mode;
    GAME_CALL0(st, FN_FIGB_SOLDIER_ACTION);
}

/* @bin 0x00498A21 standing in deep water (terrain 0x10004): 0x004BD250 instead */
static int in_deep_water(game_state *st)
{
    uint32_t f = FIGB_F(st);
    if ((U32(st, G_GRID_TERRAIN + 4u * U32(st, f + FIG_GRID_OFFSET)) & 0x10004u) == 0x10004u) {
        GAME_CALL(st, FN_FIGB_REMOVE, FIGB_CUR(st));
        return 1;
    }
    return 0;
}

/* the facing: the attack direction (+0x13) when fighting, else the direction, the previous one
 * when it is not 0..7 (unless the formation record keeps the current one); stored at 0x00E94E00 */
static int32_t soldier_dir_from(game_state *st, int formation, int fighting)
{
    uint32_t f = FIGB_F(st);
    int32_t d;
    if (fighting) {
        d = S8(st, f + FIG_ATTACK_DIRECTION);
    } else if (formation >= 0 && S16(st, G_FORMATION_BASE + FM_U5C + 0x90u * (uint32_t) formation) != 0) {
        d = S8(st, f + FIG_DIRECTION);
    } else if (S8(st, f + FIG_DIRECTION) < 8) {
        d = S8(st, f + FIG_DIRECTION);
    } else {
        d = S8(st, f + FIG_PREV_DIRECTION);
    }
    d -= S32(st, G_VIEW_ORIENTATION);
    S32(st, G_SPRITE_DIRECTION) = d;
    if (d < 0) {
        d += 8;
        S32(st, G_SPRITE_DIRECTION) = d;
    }
    return d;
}

static int32_t soldier_dir(game_state *st, int formation)
{
    return soldier_dir_from(st, formation, U8(st, FIGB_F(st) + FIG_ACTION) == 5);
}

static uint32_t dying_frame(game_state *st, uint32_t f)
{
    return U8(st, G_DIE_FRAMES + S16(st, f + FIG_WAIT_TICKS) / 2);
}

void figb_action_2f(game_state *st)
{
    /* @bin 0x004984A0 */
    soldier_step(st, 1);
    int32_t d = soldier_dir(st, -1);
    uint32_t f = FIGB_F(st);
    uint32_t base = U32(st, G_IMAGE_GROUP_257);
    uint8_t a = U8(st, f + FIG_ACTION);
    if (U8(st, f + FIG_DIRECTION) != 0x0B && a != 5 && a == 4) {
        U32(st, f + FIG_U04) = dying_frame(st, f) + base + 0x68u;
        return;
    }
    U32(st, f + FIG_U04) = (uint32_t) d + 8u * U8(st, f + FIG_ANIM_FRAME) + base;
}

void figb_action_30(game_state *st)
{
    /* @bin 0x00498590 */
    soldier_step(st, 3);
    int32_t d = soldier_dir(st, -1);
    uint32_t f = FIGB_F(st);
    uint32_t base = U32(st, G_IMAGE_GROUP_266);
    uint8_t a = U8(st, f + FIG_ACTION);
    if (U8(st, f + FIG_DIRECTION) == 0x0B || a == 5) {
        U32(st, f + FIG_U04) = (uint32_t) d + 8u * (uint32_t) (U8(st, f + FIG_ANIM_FRAME) >> 1) + 0x60u + base;
        return;
    }
    if (a == 4) {
        U32(st, f + FIG_U04) = dying_frame(st, f) + base + 0x90u;
        return;
    }
    U32(st, f + FIG_U04) = (uint32_t) d + 8u * U8(st, f + FIG_ANIM_FRAME) + base;
}

void figb_action_31(game_state *st)
{
    /* @bin 0x004986B0 */
    soldier_step(st, 2);
    soldier_dir(st, -1);
}

void figb_action_32(game_state *st)
{
    /* @bin 0x00498750 */
    soldier_step(st, 1);
    soldier_dir(st, -1);
}

void figb_action_33(game_state *st)
{
    /* @bin 0x004987F0 */
    int32_t formation = S16(st, FIGB_F(st) + FIG_FORMATION);
    soldier_step(st, 2);
    soldier_dir(st, formation);
}

/* @bin 0x00498370, 0x004988B0 a soldier of a formation with a throwing animation (action 9) */
static void formation_thrower(game_state *st, uint8_t mode)
{
    int32_t formation = S16(st, FIGB_F(st) + FIG_FORMATION);
    soldier_step(st, mode);
    int32_t d = soldier_dir(st, formation);
    uint32_t f = FIGB_F(st);
    uint32_t base = U32(st, G_IMAGE_GROUP_257);
    uint8_t a = U8(st, f + FIG_ACTION);
    if (U8(st, f + FIG_DIRECTION) != 0x0B) {
        if (a == 5) {
            U32(st, f + FIG_U04) = (uint32_t) d + base;
            return;
        }
        if (a == 9) {
            U32(st, f + FIG_U04) = (uint32_t) d + 8u * U8(st, G_ATTACK_FRAMES + (U8(st, f + FIG_ATTACK_IMAGE) >> 1)) + 0x60u + base;
            return;
        }
        if (a == 4) {
            U32(st, f + FIG_U04) = dying_frame(st, f) + base + 0x90u;
            return;
        }
    }
    U32(st, f + FIG_U04) = (uint32_t) d + 8u * U8(st, f + FIG_ANIM_FRAME) + base;
}

void figb_action_2e(game_state *st)
{
    /* @bin 0x00498370 */
    formation_thrower(st, 1);
}

void figb_action_34(game_state *st)
{
    /* @bin 0x004988B0 */
    if (in_deep_water(st)) {
        return;
    }
    formation_thrower(st, 3);
}

void figb_action_35(game_state *st)
{
    /* @bin 0x00498A10 */
    if (in_deep_water(st)) {
        return;
    }
    soldier_step(st, 1);
    soldier_dir(st, -1);
}

void figb_action_36(game_state *st)
{
    /* @bin 0x00498AD0 */
    if (in_deep_water(st)) {
        return;
    }
    soldier_step(st, 2);
    uint32_t f = FIGB_F(st);
    int fighting = U8(st, f + FIG_ACTION) == 5;
    if (fighting) {
        GAME_CALL(st, FN_FIGURE_SOUND_53B4A0, FIGB_CUR(st), 0x0C, 2);
    }
    int32_t d = soldier_dir_from(st, -1, fighting);
    f = FIGB_F(st);
    uint8_t a = U8(st, f + FIG_ACTION);
    if (a == 5) {
        uint8_t k = U8(st, f + FIG_ATTACK_IMAGE);
        if (k < 0x0C) {
            U32(st, f + FIG_U04) = (uint32_t) d + U32(st, G_IMAGE_GROUP_655);
        } else {
            U32(st, f + FIG_U04) = (uint32_t) d + 8u * (uint32_t) ((k - 0x0C) / 2) + U32(st, G_IMAGE_GROUP_655);
        }
        return;
    }
    if (a == 4) {
        U32(st, f + FIG_U04) = dying_frame(st, f) + U32(st, G_IMAGE_GROUP_654);
        return;
    }
    if (U8(st, f + FIG_DIRECTION) == 0x0B) {
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_655) + 8u * (uint32_t) (U8(st, f + FIG_ANIM_FRAME) >> 1) + (uint32_t) d;
        return;
    }
    U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_653) + 8u * U8(st, f + FIG_ANIM_FRAME) + (uint32_t) d;
}

/* @bin 0x00498D46 the attack image: the first frame until +0x6A reaches 12, then one per 2 */
static uint32_t attack_frame(game_state *st, uint32_t f, int32_t d)
{
    uint8_t k = U8(st, f + FIG_ATTACK_IMAGE);
    if (k < 0x0C) {
        return (uint32_t) d;
    }
    return (uint32_t) d + 8u * (uint32_t) ((k - 0x0C) / 2);
}

void figb_action_37(game_state *st)
{
    /* @bin 0x00498C60 */
    if (in_deep_water(st)) {
        return;
    }
    soldier_step(st, 2);
    uint32_t f = FIGB_F(st);
    uint8_t a = U8(st, f + FIG_ACTION);
    soldier_dir(st, -1);
    if (a == 5) {
        GAME_CALL(st, FN_FIGURE_SOUND_53B4A0, FIGB_CUR(st), 0x0B, 2);
        f = FIGB_F(st);
        U32(st, f + FIG_U04) = attack_frame(st, f, S32(st, G_SPRITE_DIRECTION)) + U32(st, G_IMAGE_GROUP_651);
        return;
    }
    int32_t d = S32(st, G_SPRITE_DIRECTION);
    if (a == 4) {
        U32(st, f + FIG_U04) = dying_frame(st, f) + U32(st, G_IMAGE_GROUP_650);
        return;
    }
    if (U8(st, f + FIG_DIRECTION) == 0x0B) {
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_651) + 8u * (uint32_t) (U8(st, f + FIG_ANIM_FRAME) >> 1) + (uint32_t) d;
        return;
    }
    U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_649) + 8u * U8(st, f + FIG_ANIM_FRAME) + (uint32_t) d;
}

void figb_action_38(game_state *st)
{
    /* @bin 0x00498DF0 a soldier with a carried overlay (+0x180, offsets +0x6C/+0x6D by facing) */
    static const uint8_t offsets[16] = {0x16, 0xEE, 0x1B, 0xFD, 0x17, 0x11, 0x00, 0x0B,
                                        0xEB, 0x11, 0xE7, 0xFF, 0xEF, 0xEF, 0x00, 0xF4}; /* @bin 0x00498E2C */
    if (in_deep_water(st)) {
        return;
    }
    soldier_step(st, 2);
    uint32_t f = FIGB_F(st);
    uint8_t a = U8(st, f + FIG_ACTION);
    int32_t d = soldier_dir(st, -1);
    if (a == 5) {
        GAME_CALL(st, FN_FIGURE_SOUND_53B4A0, FIGB_CUR(st), 0x0D, 2);
        f = FIGB_F(st);
        d = S32(st, G_SPRITE_DIRECTION);
        U32(st, f + FIG_U04) = attack_frame(st, f, d) + U32(st, G_IMAGE_GROUP_658);
    } else if (a == 4) {
        U32(st, f + FIG_U04) = dying_frame(st, f) + U32(st, G_IMAGE_GROUP_657);
    } else if (U8(st, f + FIG_DIRECTION) == 0x0B) {
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_658) + 8u * (uint32_t) (U8(st, f + FIG_ANIM_FRAME) >> 1) + (uint32_t) d;
    } else {
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_656) + 8u * U8(st, f + FIG_ANIM_FRAME) + (uint32_t) d;
    }
    U32(st, f + FIG_U180) = U32(st, G_IMAGE_GROUP_152) + 8u * U8(st, f + FIG_ANIM_FRAME) + (uint32_t) d;
    if (d < 0 || d > 7) {
        state_unported(st, FN_FIGB_ACTION_38); /* the original reads its stack outside the table */
        return;
    }
    U8(st, f + FIG_U6C) = offsets[2 * d];
    U8(st, f + FIG_U6D) = offsets[2 * d + 1];
}

void figb_action_39(game_state *st)
{
    /* @bin 0x00499010 a soldier counted in the city (+0x4644) */
    if (in_deep_water(st)) {
        return;
    }
    S32(st, CURRENT_CITY(st) + CITY_U4644) += 1; /* 0x00EA5EB4 */
    soldier_step(st, 1);
    int32_t d = soldier_dir(st, -1);
    uint32_t f = FIGB_F(st);
    uint32_t base = U32(st, G_IMAGE_GROUP_257);
    if (U8(st, f + FIG_DIRECTION) == 0x0B) {
        U32(st, f + FIG_U04) = base + 8u * (uint32_t) ((int32_t) (U8(st, f + FIG_ATTACK_IMAGE) - 0x0C) / 2) + (uint32_t) d;
    }
    uint8_t a = U8(st, f + FIG_ACTION);
    if (a == 5) {
        U32(st, f + FIG_U04) = attack_frame(st, f, d) + base;
        return;
    }
    if (a == 0x0C) {
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_106) + (uint32_t) d;
        return;
    }
    if (a == 4) {
        U32(st, f + FIG_U04) = dying_frame(st, f) + base + 0x98u;
        return;
    }
    U32(st, f + FIG_U04) = base + 8u * U8(st, f + FIG_ANIM_FRAME) + 0x30u + (uint32_t) d;
}

/* @bin 0x00497FC0 the sprite group of the first set when the byte 0x00784CBD is set, else of the
 * second */
static uint32_t pick(game_state *st, uint32_t a, uint32_t b)
{
    return U8(st, G_ALT_SPRITES) != 0 ? U32(st, a) : U32(st, b);
}

void figb_action_2c(game_state *st)
{
    /* @bin 0x00497FC0 */
    if (in_deep_water(st)) {
        return;
    }
    soldier_step(st, 1);
    uint32_t f = FIGB_F(st);
    uint8_t a = U8(st, f + FIG_ACTION);
    int32_t d = soldier_dir(st, -1);
    if (a == 5) {
        U32(st, f + FIG_U04) = attack_frame(st, f, d) + pick(st, G_IMAGE_GROUP_105, G_IMAGE_GROUP_259);
        return;
    }
    if (a == 4) {
        U32(st, f + FIG_U04) = dying_frame(st, f) + pick(st, G_IMAGE_GROUP_104, G_IMAGE_GROUP_258);
        return;
    }
    if (U8(st, f + FIG_DIRECTION) == 0x0B) {
        U32(st, f + FIG_U04) = (uint32_t) d + 8u * (uint32_t) (U8(st, f + FIG_ANIM_FRAME) >> 1) + pick(st, G_IMAGE_GROUP_105, G_IMAGE_GROUP_259);
        return;
    }
    U32(st, f + FIG_U04) = (uint32_t) d + 8u * U8(st, f + FIG_ANIM_FRAME) + pick(st, G_IMAGE_GROUP_103, G_IMAGE_GROUP_257);
}

void figb_action_2d(game_state *st)
{
    /* @bin 0x00498160 as 0x00497FC0 with other sprites and the overlay of 0x00498DF0 */
    static const uint8_t offsets[16] = {0x16, 0xEE, 0x1B, 0xFD, 0x17, 0x11, 0x00, 0x0B,
                                        0xEB, 0x11, 0xE7, 0xFF, 0xEF, 0xEF, 0x00, 0xF4}; /* @bin 0x0049819C */
    if (in_deep_water(st)) {
        return;
    }
    soldier_step(st, 1);
    uint32_t f = FIGB_F(st);
    uint8_t a = U8(st, f + FIG_ACTION);
    int32_t d = soldier_dir(st, -1);
    if (a == 5) {
        U32(st, f + FIG_U04) = attack_frame(st, f, d) + pick(st, G_IMAGE_GROUP_108, G_IMAGE_GROUP_268);
    } else if (a == 4) {
        U32(st, f + FIG_U04) = dying_frame(st, f) + pick(st, G_IMAGE_GROUP_107, G_IMAGE_GROUP_267);
    } else if (U8(st, f + FIG_DIRECTION) == 0x0B) {
        U32(st, f + FIG_U04) = (uint32_t) d + 8u * (uint32_t) (U8(st, f + FIG_ANIM_FRAME) >> 1) + pick(st, G_IMAGE_GROUP_108, G_IMAGE_GROUP_268);
    } else {
        U32(st, f + FIG_U04) = (uint32_t) d + 8u * U8(st, f + FIG_ANIM_FRAME) + pick(st, G_IMAGE_GROUP_106, G_IMAGE_GROUP_266);
    }
    U32(st, f + FIG_U180) = U32(st, G_IMAGE_GROUP_152) + 8u * U8(st, f + FIG_ANIM_FRAME) + (uint32_t) d;
    if (d < 0 || d > 7) {
        state_unported(st, FN_FIGB_ACTION_2D); /* the original reads its stack outside the table */
        return;
    }
    U8(st, f + FIG_U6C) = offsets[2 * d];
    U8(st, f + FIG_U6D) = offsets[2 * d + 1];
}

void figb_action_2b(game_state *st)
{
    /* @bin 0x00497DC0 */
    if (in_deep_water(st)) {
        return;
    }
    int32_t formation = S16(st, FIGB_F(st) + FIG_FORMATION);
    soldier_step(st, 1);
    uint32_t f = FIGB_F(st);
    uint8_t a = U8(st, f + FIG_ACTION);
    int32_t d = soldier_dir(st, formation);
    if (a == 5) {
        U32(st, f + FIG_U04) = attack_frame(st, f, d) + pick(st, G_IMAGE_GROUP_102, G_IMAGE_GROUP_255);
        return;
    }
    if (a == 9) {
        U32(st, f + FIG_U04) = (uint32_t) d + 8u * U8(st, G_ATTACK_FRAMES + (U8(st, f + FIG_ATTACK_IMAGE) >> 1)) + pick(st, G_IMAGE_GROUP_102, G_IMAGE_GROUP_255);
        return;
    }
    if (a == 4) {
        U32(st, f + FIG_U04) = dying_frame(st, f) + pick(st, G_IMAGE_GROUP_101, G_IMAGE_GROUP_254);
        return;
    }
    if (U8(st, f + FIG_DIRECTION) == 0x0B) {
        U32(st, f + FIG_U04) = (uint32_t) d + 8u * (uint32_t) (U8(st, f + FIG_ANIM_FRAME) >> 1) + pick(st, G_IMAGE_GROUP_102, G_IMAGE_GROUP_255);
        return;
    }
    U32(st, f + FIG_U04) = (uint32_t) d + 8u * U8(st, f + FIG_ANIM_FRAME) + pick(st, G_IMAGE_GROUP_100, G_IMAGE_GROUP_253);
}

void figb_action_29(game_state *st)
{
    /* @bin 0x00497010 a soldier of a building (+0x2A) that goes out (to the point of the record
     * at 0x00BCB9FE, or to a tile from 0x004807D0) and back */
    if (in_deep_water(st)) {
        return;
    }
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_TERRAIN_USAGE) = 0;
    U8(st, f + FIG_U0C) = 0;
    S16(st, f + FIG_MAX_ROAM_LENGTH) = 800;
    int32_t b = S16(st, f + FIG_HOME);
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    uint32_t ba = BUILDING_ADDR(b);
    if (U8(st, ba + BLD_STATE) != 1 || S16(st, ba + BLD_WALKER) != id) {
        U8(st, f + FIG_STATE) = 2;
    }
    figb_anim_advance(st, f, 12);
    int8_t dir;
    switch (U8(st, f + FIG_ACTION)) {
    case 5:
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        break;
    case 4:
        GAME_CALL(st, FN_FIGURE_DYING, id);
        break;
    case 0x0B: {
        uint32_t c = CURRENT_CITY(st);
        U8(st, f + FIG_TERRAIN_USAGE) = 2;
        S32(st, c + CITY_U4544) = 10;   /* 0x00EA5DB4 */
        S32(st, c + CITY_U4728) += 1;   /* 0x00EA5F98 */
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + FIG_DIRECTION);
        if (dir == 9 || dir == 10 || dir == 8) {
            U8(st, f + FIG_ACTION) = 0x0A;
        }
        break;
    }
    case 0x0A:
        U8(st, f + FIG_ANIM_FRAME) = 0;
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > (int32_t) ((uint32_t) id & 3u) + 10) {
            int32_t b2 = S16(st, ba + BLD_SUBTYPE);
            S16(st, f + FIG_WAIT_TICKS) = 0;
            if (S32(st, CURRENT_CITY(st) + CITY_NATIVES_RESTLESS) != 0) { /* 0x00EA5E04 */
                U8(st, f + FIG_ACTION) = 0x0B;
                U16(st, f + FIG_DEST_X) = U16(st, G_FORMATION_BASE + FM_DEST_X);
                U16(st, f + FIG_DEST_Y) = U16(st, G_FORMATION_BASE + FM_DEST_Y);
                U16(st, f + FIG_DEST_BUILDING) = U16(st, G_FORMATION_BASE + FM_DEST_BUILDING);
                GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            } else {
                uint32_t b2a = BUILDING_ADDR(b2);
                int32_t r = GAME_CALL(st, FN_BUILDING_FIND_OPEN_TILE, S16(st, b2a + BLD_X), S16(st, b2a + BLD_Y), U8(st, b2a + BLD_SIZE));
                if (r != 0) {
                    f = FIGB_F(st);
                    U8(st, f + FIG_ACTION) = 8;
                    U16(st, f + FIG_DEST_X) = U16(st, G_ROAD_ACCESS_X);
                    U16(st, f + FIG_DEST_Y) = U16(st, G_ROAD_ACCESS_Y);
                }
                GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
            }
        }
        break;
    case 8:
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + FIG_DIRECTION);
        if (dir == 9 || dir == 10) {
            U8(st, f + FIG_STATE) = 2;
        } else if (dir == 8) {
            U8(st, f + FIG_ACTION) = 9;
            U16(st, f + FIG_DEST_X) = U16(st, f + FIG_SOURCE_X);
            U16(st, f + FIG_DEST_Y) = U16(st, f + FIG_SOURCE_Y);
        }
        break;
    case 9:
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + FIG_DIRECTION);
        if (dir == 9 || dir == 10) {
            U8(st, f + FIG_STATE) = 2;
        }
        if (S8(st, f + FIG_DIRECTION) == 8) {
            U8(st, f + FIG_STATE) = 2;
        }
        break;
    default:
        U8(st, f + FIG_ACTION) = 0x0A;
        break;
    }
    f = FIGB_F(st);
    uint8_t a = U8(st, f + FIG_ACTION);
    int32_t d;
    if (a == 5 || S8(st, f + FIG_DIRECTION) == 0x0B) {
        d = S8(st, f + FIG_ATTACK_DIRECTION);
    } else if (S8(st, f + FIG_DIRECTION) < 8) {
        d = S8(st, f + FIG_DIRECTION);
    } else {
        d = S8(st, f + FIG_PREV_DIRECTION);
    }
    d -= S32(st, G_VIEW_ORIENTATION);
    S32(st, G_SPRITE_DIRECTION) = d;
    if (d < 0) {
        d += 8;
        S32(st, G_SPRITE_DIRECTION) = d;
    }
    uint32_t base = U32(st, G_IMAGE_GROUP_257);
    if (a == 5) {
        U32(st, f + FIG_U04) = attack_frame(st, f, d) + 0xC0u + base;
    } else if (a == 4) {
        U32(st, f + FIG_U04) = dying_frame(st, f) + base + 0xF0u;
    } else if (U8(st, f + FIG_DIRECTION) == 0x0B) {
        U32(st, f + FIG_U04) = (uint32_t) d + 8u * (uint32_t) (U8(st, f + FIG_ANIM_FRAME) >> 1) + 0xC0u + base;
    } else if (a == 0x0B) {
        U32(st, f + FIG_U04) = (uint32_t) d + 8u * U8(st, f + FIG_ANIM_FRAME) + 0x60u + base;
    } else {
        U32(st, f + FIG_U04) = (uint32_t) d + 8u * U8(st, f + FIG_ANIM_FRAME) + base;
    }
}

/* the formation record of a soldier (0x90 bytes; fields by their absolute address) */
#define FORM(form, a) ((uint32_t) (a) + 0x90u * (uint32_t) (form))

void figb_soldier_action(game_state *st)
{
    /* @bin 0x004991A0 the soldiers' states: 9 at the formation's place (+0x30/+0x32 offset by the
     * layout 0x005CD6F8 and the rank +0x62), 0xB going there, 0xC attacking (target +0x82), 3
     * going home, 0xA, 0xD, 0xE */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    int32_t form = S16(st, f + FIG_FORMATION);
    U8(st, f + FIG_TERRAIN_USAGE) = 2;
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    uint32_t city = CURRENT_CITY(st);
    S32(st, city + CITY_U4634) += 1; /* 0x00EA5EA4 */
    int32_t layout = S16(st, FORM(form, G_FORMATION_BASE + FM_LAYOUT));
    uint32_t slot = (uint32_t) (U8(st, f + FIG_U62) + layout * 16);
    U16(st, f + FIG_U30) = U16(st, G_FORMATION_LAYOUT_X + 8u * slot);
    U16(st, f + FIG_U32) = U16(st, G_FORMATION_LAYOUT_Y + 8u * slot);
    int8_t dir;
    switch (U8(st, f + FIG_ACTION)) {
    case 5:
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        return;
    case 4:
        GAME_CALL(st, FN_FIGURE_DYING, id);
        return;
    case 0x0E:
        S16(st, f + FIG_WAIT_TICKS) = 0;
        return;
    case 9: {
        GAME_CALL(st, FN_FIGURE_COUNT_ON_TILE, id);
        U8(st, FIGB_F(st) + FIG_ANIM_FRAME) = 0;
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
        id = FIGB_CUR(st);
        f = FIGURE_ADDR(id);
        S16(st, f + FIG_WAIT_TICKS) -= 1;
        if (S16(st, f + FIG_WAIT_TICKS) <= 0) {
            S16(st, f + FIG_WAIT_TICKS) = 0x32;
            if (U8(st, f + FIG_INVISIBLE) != 0 && U8(st, f + FIG_U62) == 0) {
                /* @bin 0x004992EF the leader's sound: drums, horn 2 or horn 1 */
                int16_t l = S16(st, FORM(form, G_FORMATION_BASE + FM_LAYOUT));
                uint32_t wav = l == 8 ? G_STR_DRUMS_WAV : l == 0x0C ? G_STR_HORN2_WAV : G_STR_HORN1_WAV;
                GAME_CALL(st, FN_RETURN_ZERO, (int32_t) wav, 1, 0);
                id = FIGB_CUR(st);
                f = FIGURE_ADDR(id);
            }
            U8(st, f + FIG_INVISIBLE) = 0;
            if (S16(st, FORM(form, G_FORMATION_BASE + FM_U50)) != 0) {
                U8(st, f + FIG_ACTION) = 0x0C;
            } else {
                U16(st, f + FIG_DEST_X) = (uint16_t) (U16(st, FORM(form, G_FORMATION_BASE + FM_DEST_X)) + U16(st, f + FIG_U30));
                U16(st, f + FIG_DEST_Y) = (uint16_t) (U16(st, FORM(form, G_FORMATION_BASE + FM_DEST_Y)) + U16(st, f + FIG_U32));
                uint8_t d = (uint8_t) GAME_CALL(st, FN_ROUTE_DIRECTION, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X),
                                                S16(st, f + FIG_DEST_Y));
                if (d < 8) {
                    U8(st, FIGB_F(st) + FIG_ACTION) = 0x0B;
                }
            }
        }
        id = FIGB_CUR(st);
        f = FIGURE_ADDR(id);
        uint8_t t = U8(st, f + FIG_TYPE);
        if (t != 0x2B && t != 0x2E && t != 0x33 && t != 0x34 && t != 0x37) {
            return;
        }
        /* @bin 0x004993E1 archers: a target every property +0x1C ticks (0x00499E50, range +0x18) */
        uint8_t n = (uint8_t) (U8(st, f + FIG_U6B) + 1);
        U8(st, f + FIG_U6B) = n;
        uint32_t props = G_PROPS + 0x38u * t;
        if ((int32_t) n > S32(st, props + FIGTYPE_PERIOD)) {
            U8(st, f + FIG_U6B) = 0;
            if (GAME_CALL(st, FN_SOLDIER_FIND_TARGET, id, S32(st, props + FIGTYPE_RANGE)) != 0) {
                f = FIGB_F(st);
                U8(st, f + FIG_ATTACK_IMAGE) = 1;
                int32_t d = GAME_CALL(st, FN_DIRECTION_TO_FINE, S16(st, f + FIG_X), S16(st, f + FIG_Y),
                                      (int32_t) U32(st, G_ROAD_ACCESS_X), (int32_t) U32(st, G_ROAD_ACCESS_Y));
                U8(st, f + FIG_DIRECTION) = (uint8_t) d;
            } else {
                U8(st, FIGB_F(st) + FIG_ATTACK_IMAGE) = 0;
            }
        }
        id = FIGB_CUR(st);
        f = FIGURE_ADDR(id);
        uint8_t k = U8(st, f + FIG_ATTACK_IMAGE);
        if (k == 0) {
            return;
        }
        if (k == 1) {
            GAME_CALL(st, FN_MISSILE_CREATE, id, S16(st, f + FIG_X), S16(st, f + FIG_Y), (int32_t) U32(st, G_ROAD_ACCESS_X),
                      (int32_t) U32(st, G_ROAD_ACCESS_Y), 0x47);
            S16(st, FORM(form, G_FORMATION_BASE + FM_U5C)) = 6;
            f = FIGB_F(st);
        }
        k = (uint8_t) (U8(st, f + FIG_ATTACK_IMAGE) + 1);
        U8(st, f + FIG_ATTACK_IMAGE) = k;
        if (k > 0x64) {
            U8(st, f + FIG_ATTACK_IMAGE) = 0;
        }
        return;
    }
    case 3:
        U16(st, f + FIG_DEST_X) = U16(st, f + FIG_SOURCE_X);
        U16(st, f + FIG_DEST_Y) = U16(st, f + FIG_SOURCE_Y);
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + FIG_DIRECTION);
        if (dir == 9 || dir == 10 || dir == 8) {
            U8(st, f + FIG_STATE) = 2;
        }
        return;
    case 0x0A:
        GAME_CALL(st, FN_FIGURE_COUNT_ON_TILE, id);
        return;
    case 0x0B:
        S16(st, f + FIG_WAIT_TICKS) -= 1;
        if (S16(st, f + FIG_WAIT_TICKS) <= 0) {
            S16(st, f + FIG_WAIT_TICKS) = 0x32;
            int16_t px = S16(st, FORM(form, G_FORMATION_BASE + FM_DEST_X)), py = S16(st, FORM(form, G_FORMATION_BASE + FM_DEST_Y));
            if (S16(st, f + FIG_DEST_X) != px + S16(st, f + FIG_U30) || S16(st, f + FIG_DEST_Y) != py + S16(st, f + FIG_U32)) {
                U16(st, f + FIG_DEST_X) = (uint16_t) (px + S16(st, f + FIG_U30));
                U16(st, f + FIG_DEST_Y) = (uint16_t) (py + S16(st, f + FIG_U32));
                int8_t d = (int8_t) GAME_CALL(st, FN_ROUTE_DIRECTION, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X),
                                              S16(st, f + FIG_DEST_Y));
                if (d == 8) {
                    U8(st, FIGB_F(st) + FIG_ACTION) = 9;
                    return;
                }
                f = FIGB_F(st);
                U16(st, f + FIG_DEST_BUILDING) = U16(st, FORM(form, G_FORMATION_BASE + FM_DEST_BUILDING));
                GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
            }
        }
        GAME_CALL(st, FN_FIGURE_MOVE, FIGB_CUR(st), speed);
        f = FIGB_F(st);
        dir = S8(st, f + FIG_DIRECTION);
        if (dir == 9 || dir == 10 || dir == 8) {
            U8(st, f + FIG_ACTION) = 9;
        }
        return;
    case 0x0C: {
        if (S16(st, FORM(form, G_FORMATION_BASE + FM_U50)) == 0) {
            U8(st, f + FIG_ACTION) = 9;
        }
        uint8_t t = U8(st, f + FIG_TYPE);
        if (t != 0x2E && t != 0x2F) {
            /* @bin 0x00499700 battle cries every 200 ticks per kind */
            uint32_t a = (t == 0x34 || t == 0x30) ? city + CITY_U4680 : city + CITY_U467F;
            int8_t c = (int8_t) (U8(st, a) - 1);
            U8(st, a) = (uint8_t) c;
            if (c <= 0) {
                U8(st, a) = 200;
                GAME_CALL(st, FN_RETURN_ZERO, (t == 0x34 || t == 0x30) ? 0x17 : 0x1B);
            }
        }
        id = FIGB_CUR(st);
        f = FIGURE_ADDR(id);
        int32_t target = S16(st, f + FIG_TARGET);
        if (U8(st, FIGURE_ADDR(target) + FIG_STATE) != 1) {
            S16(st, f + FIG_TARGET) = 0;
            target = 0;
        }
        int chase = 0;
        if (U8(st, FIGURE_ADDR(target) + FIG_ACTION) == 4) {
            S16(st, f + FIG_TARGET) = 0;
        } else if (target > 0) {
            chase = 1;
        }
        if (!chase) {
            int32_t r = GAME_CALL(st, FN_SOLDIER_NEAREST_0B_0D, S16(st, f + FIG_X), S16(st, f + FIG_Y));
            if (r != 0) {
                id = FIGB_CUR(st);
                f = FIGURE_ADDR(id);
                uint32_t rf = FIGURE_ADDR(r);
                U16(st, f + FIG_DEST_X) = U16(st, rf + FIG_X);
                U16(st, f + FIG_DEST_Y) = U16(st, rf + FIG_Y);
                S16(st, f + FIG_TARGET) = (int16_t) r;
                S16(st, rf + FIG_TARGETED_BY) = (int16_t) id;
                U16(st, f + FIG_U88) = U16(st, rf + FIG_SEQUENCE);
                GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            }
            if (r <= 0) {
                f = FIGB_F(st);
                U8(st, f + FIG_ACTION) = 9;
                S16(st, f + FIG_WAIT_TICKS) = 0x32;
                return;
            }
        }
        GAME_CALL(st, FN_FIGURE_MOVE, FIGB_CUR(st), speed);
        id = FIGB_CUR(st);
        f = FIGURE_ADDR(id);
        dir = S8(st, f + FIG_DIRECTION);
        if (dir == 9 || dir == 10) {
            U8(st, f + FIG_ACTION) = 9;
            S16(st, f + FIG_TARGET) = 0;
        } else if (dir == 8) {
            uint32_t tf = FIGURE_ADDR(S16(st, f + FIG_TARGET));
            U16(st, f + FIG_DEST_X) = U16(st, tf + FIG_X);
            U16(st, f + FIG_DEST_Y) = U16(st, tf + FIG_Y);
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        }
        return;
    }
    case 0x0D:
        return;
    default:
        U8(st, f + FIG_ACTION) = 9;
        return;
    }
}
