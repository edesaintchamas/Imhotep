/* Imhotep - figure actions, group b: figure types 0x15-0x18.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21); city data: CURRENT_CITY. */
#include "figure/actions_b.h"

#include "random/random.h"
#include "sim/call.h"
#include "sim/math.h"
#include "state/layout.h"


int32_t figb_leader_of(game_state *st, int32_t id)
{
    /* @bin 0x00492360 the figure at +0x68 if it has type 0x13; if it has type 0x15, its own +0x68 */
    int32_t l = S16(st, FIGURE_ADDR(id) + FIG_LEADER);
    uint8_t t = U8(st, FIGURE_ADDR(l) + FIG_TYPE);
    if (t == 0x13) {
        return l;
    }
    if (t == 0x15) {
        return S16(st, FIGURE_ADDR(l) + FIG_LEADER);
    }
    return 0;
}

void figb_action_15(game_state *st)
{
    /* @bin 0x004923A0 a follower of a type 0x13 (or 0x15) figure */
    int32_t id = FIGB_CUR(st);
    int32_t leader = S16(st, FIGURE_ADDR(id) + FIG_LEADER);
    int32_t head = figb_leader_of(st, id);
    id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_INVISIBLE) = 0;
    U8(st, f + FIG_TERRAIN_USAGE) = 3;
    figb_anim_advance(st, f, 12);
    U32(st, f + FIG_U180) = 0;
    uint32_t lf = FIGURE_ADDR(leader);
    uint32_t hf = FIGURE_ADDR(head);
    if (leader <= 0) {
        U8(st, f + FIG_STATE) = 2;
    } else if (U8(st, lf + FIG_ACTION) == 4) {
        U8(st, f + FIG_STATE) = 2;
    } else if (U8(st, lf + FIG_STATE) != 1) {
        U8(st, f + FIG_STATE) = 2;
    } else if (U8(st, lf + FIG_TYPE) != 0x13 && U8(st, lf + FIG_TYPE) != 0x15) {
        U8(st, f + FIG_STATE) = 2;
    } else if (U8(st, hf + FIG_ACTION) != 6 && U8(st, hf + FIG_ACTION) != 7) {
        GAME_CALL(st, FN_FIGB_FOLLOW, id, S16(st, f + FIG_LEADER), 1); /* follow the leader */
        id = FIGB_CUR(st);
    }
    if (U8(st, lf + FIG_INVISIBLE) != 0) {
        U8(st, FIGURE_ADDR(id) + FIG_INVISIBLE) = 1;
    }
    f = FIGURE_ADDR(id);
    int32_t d = S8(st, f + FIG_DIRECTION);
    if ((int8_t) d >= 8) {
        d = S8(st, f + FIG_PREV_DIRECTION);
    }
    d -= S32(st, G_VIEW_ORIENTATION);
    S32(st, G_SPRITE_DIRECTION) = d;
    if (d < 0) {
        d += 8;
        S32(st, G_SPRITE_DIRECTION) = d;
    }
    uint8_t ha = U8(st, hf + FIG_ACTION);
    if (ha == 6) {
        U32(st, f + FIG_U04) = 0;
        return;
    }
    if (ha == 7) {
        U8(st, f + FIG_ANIM_FRAME) = 0;
    }
    if (U8(st, f + FIG_STATE) == 2) {
        U32(st, f + FIG_U04) = U8(st, G_DIE_FRAMES + S16(st, f + FIG_WAIT_TICKS) / 2) + U32(st, G_IMAGE_GROUP_92);
    } else {
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_91) + 8u * U8(st, f + FIG_ANIM_FRAME) + (uint32_t) d;
    }
    U8(st, f + FIG_INVISIBLE) = 0;
}

void figb_action_16(game_state *st)
{
    /* @bin 0x00492580 64 frames, removed after 200 ticks */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_TERRAIN_USAGE) = 1;
    figb_anim_advance(st, f, 0x40);
    U32(st, f + FIG_U180) = 0;
    if (U8(st, f + FIG_ACTION) == 4) {
        GAME_CALL(st, FN_FIGURE_DYING, id);
        f = FIGB_F(st);
    }
    S16(st, f + FIG_WAIT_TICKS) += 1;
    if (S16(st, f + FIG_WAIT_TICKS) > 200) {
        U8(st, f + FIG_STATE) = 2;
        U8(st, f + FIG_ANIM_FRAME) = 0;
    }
    if (U8(st, f + FIG_ACTION) == 4) {
        U32(st, f + FIG_U04) = U8(st, G_DIE_FRAMES + S16(st, f + FIG_WAIT_TICKS) / 2) + U32(st, G_IMAGE_GROUP_72);
        return;
    }
    U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_71) + 8u * U8(st, f + FIG_ANIM_FRAME);
}

/* @bin 0x00492B2B (0x00493498 in 0x004930B0) the game RNG stepped inline, then a wait of r % 10 */
static void wait_random(game_state *st, uint32_t f)
{
    U8(st, f + FIG_ACTION) = 8;
    random_next_state(st);
    S16(st, f + FIG_WAIT_TICKS) = (int16_t) (S32(st, G_RANDOM_R1_15BIT) % 10);
}

/* the walker image without storing the relative direction (0x00E94E00) */
static void image_local(game_state *st, uint32_t walk, uint32_t die)
{
    uint32_t f = FIGB_F(st);
    int32_t d = S8(st, f + FIG_DIRECTION) - S32(st, G_VIEW_ORIENTATION);
    if (d < 0) {
        d += 8;
    }
    if (U8(st, f + FIG_ACTION) == 4) {
        U32(st, f + FIG_U04) = U8(st, G_DIE_FRAMES + S16(st, f + FIG_WAIT_TICKS) / 2) + U32(st, die);
        return;
    }
    U32(st, f + FIG_U04) = U32(st, walk) + 8u * U8(st, f + FIG_ANIM_FRAME) + (uint32_t) d;
}

static int difficulty_ok(game_state *st, uint32_t addr)
{
    uint32_t d = U32(st, G_DIFFICULTY);
    if (d > 4) {
        state_unported(st, addr); /* the original reads its stack out of the table */
        return 0;
    }
    return 1;
}

void figb_action_17(game_state *st)
{
    /* @bin 0x00492770 waits, walks to a building (0x00492650) and takes a share of money */
    static const int32_t pct[5] = {3, 6, 9, 12, 15}; /* @bin 0x0049278C by difficulty */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_TERRAIN_USAGE) = 1;
    U32(st, f + FIG_U180) = 0;
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    figb_anim_advance(st, f, 12);
    switch (U8(st, f + FIG_ACTION)) {
    case 4:
        GAME_CALL(st, FN_FIGURE_DYING, id);
        break;
    case 5:
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        break;
    case 8: {
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) <= 0x28) {
            break;
        }
        int32_t b = GAME_CALL(st, FN_FIGB_FIND_TARGET_BUILDING, id);
        f = FIGB_F(st);
        if (b == 0) {
            U8(st, f + FIG_STATE) = 2;
            break;
        }
        U8(st, f + FIG_ACTION) = 9;
        U16(st, f + FIG_DEST_X) = U16(st, BUILDING_ADDR(b) + BLD_ROAD_ACCESS_X);
        U16(st, f + FIG_DEST_Y) = U16(st, BUILDING_ADDR(b) + BLD_ROAD_ACCESS_Y);
        S16(st, f + FIG_DEST_BUILDING) = (int16_t) b;
        S16(st, f + FIG_WAIT_TICKS) = 0;
        break;
    }
    case 9: {
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed * 2);
        f = FIGB_F(st);
        int8_t dir = S8(st, f + FIG_DIRECTION);
        if (dir != 8) {
            if (dir == 9 || dir == 10) {
                U8(st, f + FIG_STATE) = 2;
            }
            break;
        }
        int32_t b = S16(st, f + FIG_DEST_BUILDING);
        uint32_t ba = BUILDING_ADDR(b);
        if (b != 0 && U8(st, ba + BLD_STATE) == 1) {
            if (GAME_CALL(st, FN_BUILDING_TYPE_4D_4F, b) != 0) {
                if (!difficulty_ok(st, FN_FIGB_ACTION_17)) {
                    return;
                }
                uint32_t c = CURRENT_CITY(st);
                int32_t amount = calc_percentage(S32(st, c + CITY_SAVINGS), pct[U32(st, G_DIFFICULTY)]);
                if (amount != 0) {
                    S32(st, c + CITY_U4760) += 1;
                    S32(st, c + CITY_SAVINGS) -= amount;
                    S32(st, c + CITY_U4784) += amount;
                    GAME_CALL(st, FN_NOTICE_ADD_4E2970, 0x67, 0, 0);
                    S32(st, CURRENT_CITY(st) + CITY_U4418) += 1;
                }
            } else if (GAME_CALL(st, FN_BUILDING_TYPE_0X459EF0, b) != 0) {
                if (!difficulty_ok(st, FN_FIGB_ACTION_17)) {
                    return;
                }
                int32_t amount = calc_percentage(S32(st, ba + BLD_UB8), pct[U32(st, G_DIFFICULTY)]);
                if (amount != 0) {
                    GAME_CALL0(st, FN_DAY_SERVICE_SHARES);
                    S32(st, G_MESSAGE_POPUP) = 1;
                    uint32_t c = CURRENT_CITY(st);
                    S16(st, c + CITY_U448C) += 1;
                    S32(st, c + CITY_U4760) += 1;
                    S32(st, c + CITY_THEFT_THIS_YEAR) += amount;
                    S32(st, c + CITY_U4764) += amount;
                    S32(st, c + CITY_TREASURY) -= amount;
                    int16_t bt = S16(st, ba + BLD_TYPE);
                    int32_t msg = 0; /* @bin 0x00492CB0 by building type */
                    if (bt >= 0x54 && bt <= 0x57) {
                        msg = 0xC5;
                    } else if (bt == 0xB8) {
                        msg = 0xC6;
                    } else if (bt >= 0xBB && bt <= 0xBD) {
                        msg = 0x66;
                    }
                    if (msg) {
                        GAME_CALL(st, FN_NOTICE_ADD_4E2970, msg, 0, 0);
                    }
                    S32(st, CURRENT_CITY(st) + CITY_U4418) += 1;
                }
            }
        }
        U8(st, FIGB_F(st) + FIG_STATE) = 2;
        break;
    }
    case 10: {
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) <= 10) {
            break;
        }
        uint32_t c = CURRENT_CITY(st);
        S16(st, c + CITY_U448C) += 1;
        S32(st, c + CITY_U4760) += 1;
        S32(st, G_MESSAGE_POPUP) = 1;
        S32(st, c + CITY_THEFT_THIS_YEAR) += 100;
        S32(st, c + CITY_U4764) += 100;
        GAME_CALL(st, FN_NOTICE_ADD_4E2970, 0xCB, 0, 0);
        U8(st, FIGB_F(st) + FIG_STATE) = 2;
        break;
    }
    default:
        wait_random(st, f);
        break;
    }
    image_local(st, G_IMAGE_GROUP_71, G_IMAGE_GROUP_72);
}

/* @bin 0x004937A0 set dying (action 4), the wait counter to -1 unless already dying */
static void set_dying(game_state *st, uint32_t f)
{
    if (U8(st, f + FIG_ACTION) != 4) {
        S16(st, f + FIG_WAIT_TICKS) = -1;
    }
    U8(st, f + FIG_ACTION) = 4;
}

/* @bin 0x004938B3 head for the building's road tile (0x004806C0), keep it as the return point */
static void go_to_building(game_state *st, int32_t b)
{
    uint32_t ba = BUILDING_ADDR(b);
    GAME_CALL(st, FN_BUILDING_FIND_ROAD_8, S16(st, ba + BLD_X), S16(st, ba + BLD_Y), U8(st, ba + BLD_SIZE));
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    uint16_t x = U16(st, G_ROAD_ACCESS_X), y = U16(st, G_ROAD_ACCESS_Y);
    U16(st, f + FIG_DEST_X) = x;
    U16(st, f + FIG_DEST_Y) = y;
    U16(st, f + FIG_SOURCE_X) = x;
    U16(st, f + FIG_SOURCE_Y) = y;
    GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
}

/* @bin 0x00493C00 after a move: 9 (reroute) drops the path, 10 (lost) dies */
static void moved_other(game_state *st, uint32_t f, int8_t dir)
{
    if (dir == 9) {
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
    } else if (dir == 10) {
        set_dying(st, f);
    }
}

void figb_action_19(game_state *st)
{
    /* @bin 0x004937A0 a worker attached to a building (+0xB4), with a count at +0x128 */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    int32_t b = S16(st, f + FIG_HOME);
    if (U8(st, BUILDING_ADDR(b) + BLD_STATE) != 1) {
        set_dying(st, f);
    }
    if (S32(st, f + FIG_HIT_POINTS) <= 0) {
        set_dying(st, f);
    }
    uint8_t a = U8(st, f + FIG_ACTION);
    if (a != 8 && a != 4 && S16(st, BUILDING_ADDR(b) + BLD_SECOND_WALKER) != id) {
        b = GAME_CALL(st, FN_BUILDING_FREE_DOCK_480B80, id);
        id = FIGB_CUR(st);
        f = FIGURE_ADDR(id);
        if (b != 0) {
            S16(st, BUILDING_ADDR(b) + BLD_SECOND_WALKER) = (int16_t) id;
            S16(st, f + FIG_HOME) = (int16_t) b;
            U8(st, f + FIG_ACTION) = 0x0B;
            go_to_building(st, b);
            id = FIGB_CUR(st);
        } else {
            set_dying(st, f);
        }
    }
    f = FIGURE_ADDR(id);
    U8(st, f + FIG_INVISIBLE) = 0;
    U8(st, f + FIG_BOAT_MODE) = 1;
    figb_anim_advance(st, f, 12);
    U32(st, f + FIG_U180) = 0;
    switch (U8(st, f + FIG_ACTION)) {
    case 8:
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) >= 0x32) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
            int16_t h = S16(st, f + FIG_HOME);
            if (h != 0) {
                U8(st, f + FIG_ACTION) = 0x0B;
                S16(st, BUILDING_ADDR(h) + BLD_SECOND_WALKER) = (int16_t) id;
                go_to_building(st, b);
            }
        }
        break;
    case 9: {
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        id = FIGB_CUR(st);
        f = FIGURE_ADDR(id);
        U8(st, f + FIG_HEIGHT_TICKS) = 0;
        int8_t dir = S8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            if (GAME_CALL(st, FN_FIGURE_AREA_FREE_480D60, id) == 0) {
                GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
                f = FIGB_F(st);
                U16(st, f + FIG_DEST_X) = U16(st, G_ROAD_ACCESS_X);
                U16(st, f + FIG_DEST_Y) = U16(st, G_ROAD_ACCESS_Y);
                U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
            } else {
                f = FIGB_F(st);
                U8(st, f + FIG_ACTION) = 0x0A;
                S16(st, f + FIG_WAIT_TICKS) = 0;
            }
        } else if (dir == 9 || dir == 10) {
            if (S32(st, f + FIG_HIT_POINTS) > 0) {
                S32(st, f + FIG_HIT_POINTS) -= 1;
            }
            U8(st, f + FIG_ACTION) = 0x0C;
            U16(st, f + FIG_DEST_X) = U16(st, f + FIG_SOURCE_X);
            U16(st, f + FIG_DEST_Y) = U16(st, f + FIG_SOURCE_Y);
        }
        break;
    }
    case 0x0A:
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) >= 200) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
            U8(st, f + FIG_ACTION) = 0x0D;
            U16(st, f + FIG_DEST_X) = U16(st, f + FIG_SOURCE_X);
            U16(st, f + FIG_DEST_Y) = U16(st, f + FIG_SOURCE_Y);
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        }
        break;
    case 0x0B: {
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = FIGB_F(st);
        U8(st, f + FIG_HEIGHT_TICKS) = 0;
        int8_t dir = S8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            U8(st, f + FIG_ACTION) = 0x0C;
            S16(st, f + FIG_WAIT_TICKS) = 0;
        } else {
            moved_other(st, f, dir);
        }
        break;
    }
    case 0x0C: {
        uint32_t ba = BUILDING_ADDR(b);
        int32_t v = GAME_CALL(st, FN_CALC_RATIO_PERCENT, S16(st, ba + BLD_WORKERS), U32(st, G_TYPE_WORKERS + 0x28u * (uint32_t) S16(st, ba + BLD_TYPE)));
        int32_t limit = (0x66 - v) * 5;
        if (S16(st, ba + BLD_U64) > 0) {
            v = 0;
        }
        if (U32(st, G_BLOOD_RIVER_LEFT) != 0 || v <= 0) {
            break;
        }
        f = FIGB_F(st);
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) < limit) {
            break;
        }
        S16(st, f + FIG_WAIT_TICKS) = 0;
        if (GAME_CALL(st, FN_NEAREST_POINT_480C60, FIGB_CUR(st)) == 0) {
            break;
        }
        f = FIGB_F(st);
        U8(st, f + FIG_ACTION) = 9;
        U16(st, f + FIG_DEST_X) = U16(st, G_ROAD_ACCESS_X);
        U16(st, f + FIG_DEST_Y) = U16(st, G_ROAD_ACCESS_Y);
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
        break;
    }
    case 0x0D: {
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = FIGB_F(st);
        U8(st, f + FIG_HEIGHT_TICKS) = 0;
        int8_t dir = S8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            U8(st, f + FIG_ACTION) = 0x0C;
            S16(st, f + FIG_WAIT_TICKS) = 0;
            U8(st, BUILDING_ADDR(b) + BLD_SPAWN_DELAY) = 1;
            S16(st, BUILDING_ADDR(b) + BLD_U64) += 1;
        } else {
            moved_other(st, f, dir);
        }
        break;
    }
    case 4:
        break;
    default:
        U8(st, f + FIG_ACTION) = 8;
        break;
    }
    id = FIGB_CUR(st);
    if (U8(st, FIGURE_ADDR(id) + FIG_ACTION) == 4) {
        GAME_CALL(st, FN_FIGURE_DYING, id);
    }
    f = FIGB_F(st);
    int32_t d = S8(st, f + FIG_DIRECTION);
    if ((int8_t) d >= 8) {
        d = S8(st, f + FIG_PREV_DIRECTION);
    }
    d -= S32(st, G_VIEW_ORIENTATION);
    S32(st, G_SPRITE_DIRECTION) = d;
    uint8_t act = U8(st, f + FIG_ACTION);
    if (act == 0x0A) {
        d -= 2;
        S32(st, G_SPRITE_DIRECTION) = d;
    }
    if (d < 0) {
        d += 8;
        S32(st, G_SPRITE_DIRECTION) = d;
    }
    switch (act) { /* @bin 0x00493E90 */
    case 4: {
        int16_t t = S16(st, f + FIG_WAIT_TICKS);
        if (t < 0x21) {
            U32(st, f + FIG_U04) = (uint32_t) (d / 8 + (int32_t) U32(st, G_IMAGE_GROUP_175) + t / 3);
        } else {
            int32_t k = t - 0x21;
            if (k >= 0x17) {
                k = 0x16;
            }
            U32(st, f + FIG_U04) = (uint32_t) k + U32(st, G_IMAGE_GROUP_588);
        }
        break;
    }
    case 9:
    case 0x0B:
    case 0x0D:
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_173) + 8u * (uint32_t) (U8(st, f + FIG_ANIM_FRAME) / 3) + (uint32_t) d;
        break;
    case 0x0A:
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_174) + 8u * (uint32_t) (S16(st, f + FIG_WAIT_TICKS) % 9) + (uint32_t) d;
        break;
    default:
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_176) + (uint32_t) d;
        break;
    }
}

/* Tiles next to monuments, written to *x, *y (the original's output pointers); the thiscall ones
 * ignore their object. Offsets by the building's part index (+0x12), measured from the originals
 * (scratchpad tool, every index), checked against their jump tables. */
typedef struct {
    int8_t dx, dy, set;
} tile_offset;

/* @bin 0x004E0CC0 (building +0x68 + 0x16, +0x6C + 4) */
void figb_tile_4e0cc0(game_state *st, int32_t b, int32_t *x, int32_t *y)
{
    uint32_t ba = BUILDING_ADDR(b);
    *x = S32(st, ba + BLD_MON_ANNOUNCED_68) + 0x16;
    *y = S32(st, ba + BLD_ACCESS_JUGGLER) + 4;
}

/* @bin 0x004F6420 the road tile of the part given by tables 0x005F5498 / 0x005F3878 */
void figb_tile_4f6420(game_state *st, int32_t b, int32_t *x, int32_t *y)
{
    int32_t main = b - S16(st, BUILDING_ADDR(b) + BLD_PART);
    uint32_t k = U8(st, BUILDING_ADDR(main) + BLD_MONUMENT_KIND);
    int32_t part;
    if (S32(st, G_PYRAMID_KIND) == 5) {
        part = main + S32(st, G_PYRAMID_ENTRY_PART_KIND5 + 0x80u * k);
    } else {
        part = main + S32(st, G_PYRAMID_ENTRY_PART + 0x3C0u * k);
    }
    *x = S16(st, BUILDING_ADDR(part) + BLD_ROAD_ACCESS_X);
    *y = S16(st, BUILDING_ADDR(part) + BLD_ROAD_ACCESS_Y);
}

static void tile_by_part(game_state *st, int32_t b, int32_t *x, int32_t *y, const tile_offset *t, int n,
                         int out_of_range_sets)
{
    uint32_t ba = BUILDING_ADDR(b);
    int32_t k = S16(st, ba + BLD_PART);
    int32_t dx = 0, dy = 0;
    if ((uint32_t) k < (uint32_t) n) {
        dx = t[k].dx;
        dy = t[k].dy;
    } else if (!out_of_range_sets) {
        return;
    }
    *x = S16(st, ba + BLD_X) + dx;
    *y = S16(st, ba + BLD_Y) + dy;
}

void figb_tile_545a30(game_state *st, int32_t b, int32_t *x, int32_t *y)
{
    /* @bin 0x00545A30 jump table 0x00545B94 */
    static const tile_offset t[9] = {{2, 1, 1}, {3, 2, 1}, {3, 1, 1}, {2, -1, 1}, {2, -1, 1},
                                     {1, -1, 1}, {1, -1, 1}, {-1, 0, 1}, {-1, 0, 1}};
    tile_by_part(st, b, x, y, t, 9, 1);
}

void figb_tile_558f00(game_state *st, int32_t b, int32_t *x, int32_t *y)
{
    /* @bin 0x00558F00 jump table 0x0055906C */
    static const tile_offset t[10] = {{2, 1, 1}, {3, 2, 1}, {3, 0, 1}, {1, 0, 1}, {3, 2, 1},
                                      {1, 3, 1}, {0, 1, 1}, {3, 5, 1}, {-1, 0, 1}, {-1, 1, 1}};
    tile_by_part(st, b, x, y, t, 10, 0);
}

void figb_tile_556230(game_state *st, int32_t b, int32_t *x, int32_t *y)
{
    /* @bin 0x00556230 jump table 0x005563B0 */
    static const tile_offset t[21] = {{1, -1, 1}, {2, -1, 1}, {1, -1, 1}, {0, -1, 1}, {1, -1, 1}, {1, 0, 1},
                                      {3, 1, 1},  {2, -1, 1}, {1, -1, 1}, {2, -1, 1}, {3, -1, 1}, {1, 0, 1},
                                      {2, 0, 1},  {-1, 0, 1}, {-1, 0, 1}, {1, 0, 1},  {2, 1, 1},  {-1, 0, 1},
                                      {-1, 1, 1}, {0, -1, 1}, {2, -1, 1}};
    tile_by_part(st, b, x, y, t, 21, 0);
}

void figb_tile_54f8a0(game_state *st, int32_t b, int32_t *x, int32_t *y)
{
    /* @bin 0x0054F8A0 jump table 0x0054FA98 */
    static const tile_offset t[31] = {{2, 1, 1}, {3, 2, 1}, {2, 1, 1},  {1, 0, 1},  {2, 1, 1},  {1, 0, 1},  {3, 1, 1},
                                      {0, 1, 1}, {1, 3, 1}, {0, 2, 1},  {3, 2, 1},  {3, 2, 1},  {5, 4, 1},  {0, 1, 1},
                                      {3, 6, 1}, {1, 0, 1}, {2, 1, 1},  {-1, 0, 1}, {-1, 0, 1}, {-1, 0, 1}, {-1, 1, 1},
                                      {1, 0, 1}, {2, 0, 1}, {0, 1, 1},  {1, 3, 1},  {0, 1, 1},  {1, 3, 1},  {-1, 0, 1},
                                      {-1, 1, 1}, {0, 1, 1}, {1, 3, 1}};
    tile_by_part(st, b, x, y, t, 31, 0);
}

void figb_action_18(game_state *st)
{
    /* @bin 0x004930B0 waits, walks to a monument (0x00492D20) and takes a share of money */
    static const int32_t loss[5] = {-2, -4, -6, -8, -10}; /* @bin 0x004930CA by difficulty */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    int32_t amount = 0;
    U8(st, f + FIG_TERRAIN_USAGE) = 3;
    U32(st, f + FIG_U180) = 0;
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    figb_anim_advance(st, f, 12);
    switch (U8(st, f + FIG_ACTION)) {
    case 4:
        GAME_CALL(st, FN_FIGURE_DYING, id);
        f = FIGB_F(st);
        if (U8(st, f + FIG_STATE) == 2 && S16(st, f + FIG_OPPONENT) > 0) {
            GAME_CALL(st, FN_CITY_ADD_0X42D8, 1);
            GAME_CALL(st, FN_NOTICE_ADD_4E2970, 0xFE, 0, 0);
        }
        break;
    case 5:
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        break;
    case 8: {
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) <= 0x28) {
            break;
        }
        int32_t b = GAME_CALL(st, FN_FIGB_FIND_MONUMENT, id);
        f = FIGB_F(st);
        if (b == 0) {
            U8(st, f + FIG_STATE) = 2;
            break;
        }
        int32_t x = 0, y = 0;
        U8(st, f + FIG_ACTION) = 9;
        S16(st, f + FIG_DEST_BUILDING) = (int16_t) b;
        int16_t bt = S16(st, BUILDING_ADDR(b) + BLD_TYPE);
        if (bt == 0xDE) {
            figb_tile_4e0cc0(st, b, &x, &y);
        } else if (bt == 0xB7) {
            figb_tile_4f6420(st, b, &x, &y);
        } else if (bt == 0xE5) {
            figb_tile_545a30(st, b, &x, &y);
        } else if (bt == 0xEA) {
            figb_tile_558f00(st, b, &x, &y);
        } else if (bt == 0xEB) {
            figb_tile_556230(st, b, &x, &y);
        } else if (bt == 0xEC) {
            figb_tile_54f8a0(st, b, &x, &y);
        }
        f = FIGB_F(st);
        U16(st, f + FIG_DEST_X) = (uint16_t) x;
        U16(st, f + FIG_DEST_Y) = (uint16_t) y;
        S16(st, f + FIG_WAIT_TICKS) = 0;
        break;
    }
    case 9: {
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed * 2);
        f = FIGB_F(st);
        int8_t dir = S8(st, f + FIG_DIRECTION);
        if (dir != 8) {
            if (dir == 9 || dir == 10) {
                U8(st, f + FIG_STATE) = 2;
            }
            break;
        }
        int32_t b = S16(st, f + FIG_DEST_BUILDING);
        uint32_t ba = BUILDING_ADDR(b);
        if (b != 0 && U8(st, ba + BLD_STATE) == 1) {
            if (U32(st, G_DIFFICULTY) > 4) {
                state_unported(st, FN_FIGB_ACTION_18); /* the original reads its stack out of the table */
                return;
            }
            int16_t bt = S16(st, ba + BLD_TYPE);
            int monument = bt == 0xE5 || bt == 0xEB || bt == 0xEC || bt == 0xEA;
            if (bt == 0xDE && S16(st, ba + BLD_SEQUENCE) == -1) {
                GAME_CALL(st, FN_NOTICE_ADD_4E2970, 0xFC, 0, 0);
                amount = loss[U32(st, G_DIFFICULTY)];
            } else if ((bt == 0xDE || monument) && S16(st, ba + BLD_SEQUENCE) == -1) {
                GAME_CALL(st, FN_NOTICE_ADD_4E2970, 0xFB, 0, 0);
                amount = loss[U32(st, G_DIFFICULTY)];
            } else if (bt == 0xB7 || bt == 0xDE || monument) {
                GAME_CALL(st, FN_NOTICE_ADD_4E2970, 0xFD, 0, 0);
                GAME_CALL0(st, FN_FIGB_MONUMENT_DAMAGE);
                amount = loss[U32(st, G_DIFFICULTY)] / 2;
            }
            uint32_t c = CURRENT_CITY(st);
            S16(st, c + CITY_U448C) += 1;
            S32(st, c + CITY_U4760) += 1;
            S32(st, c + CITY_U4418) += 1;
            GAME_CALL(st, FN_CITY_ADD_0X42D8, amount);
        }
        U8(st, FIGB_F(st) + FIG_STATE) = 2;
        break;
    }
    default:
        wait_random(st, f);
        break;
    }
    image_local(st, G_IMAGE_GROUP_687, G_IMAGE_GROUP_688);
}
