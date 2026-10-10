/* Imhotep - figure actions, group b: service roamers and other short actions.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21). */
#include "figure/actions_b.h"

#include "random/crt_rand.h"
#include "sim/call.h"
#include "state/layout.h"

int32_t figb_dir_rel(game_state *st)
{
    /* direction relative to the view (0x00E94E00), kept in 0..7 */
    int32_t d = S8(st, FIGB_F(st) + FIG_DIRECTION) - S32(st, G_VIEW_ORIENTATION);
    S32(st, G_SPRITE_DIRECTION) = d;
    if (d < 0) {
        d += 8;
        S32(st, G_SPRITE_DIRECTION) = d;
    }
    return d;
}

int32_t figb_dir_rel_prev(game_state *st)
{
    /* the same with the previous direction (+0x12) when +0x11 is not a direction (8 and up) */
    uint32_t f = FIGB_F(st);
    int32_t d = S8(st, f + FIG_DIRECTION);
    if (d >= 8) {
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

void figb_walker_image(game_state *st, uint32_t walk_base, uint32_t die_base)
{
    /* the image of a walker: dying frames (action 4) by +0x36 / 2, else 8 frames per direction */
    int32_t d = figb_dir_rel(st);
    uint32_t f = FIGB_F(st);
    if (U8(st, f + FIG_ACTION) == 4) {
        U32(st, f + FIG_U04) = U8(st, G_DIE_FRAMES + S16(st, f + FIG_WAIT_TICKS) / 2) + U32(st, die_base);
        return;
    }
    U32(st, f + FIG_U04) = U32(st, walk_base) + 8u * U8(st, f + FIG_ANIM_FRAME) + (uint32_t) d;
}

void figb_anim_advance(game_state *st, uint32_t f, int limit)
{
    uint8_t a = (uint8_t) (U8(st, f + FIG_ANIM_FRAME) + 1);
    U8(st, f + FIG_ANIM_FRAME) = a;
    if (a >= limit) {
        U8(st, f + FIG_ANIM_FRAME) = 0;
    }
}

void figb_roamer_action(game_state *st)
{
    /* @bin 0x00493610 */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    uint8_t type = U8(st, f + FIG_TYPE);
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * type); /* @bin 0x0049363D property +0x20 */
    int16_t b = S16(st, f + FIG_HOME);
    uint8_t action = U8(st, f + FIG_ACTION);
    if (action == 5) {
        GAME_CALL(st, FN_FIGURE_FIGHT, id); /* fighting */
        return;
    }
    if (action == 4) {
        GAME_CALL(st, FN_FIGURE_DYING, id); /* dying */
        return;
    }
    if (action == 1) {
        /* roaming: when the roam length reaches its maximum, head back to the building's road */
        U8(st, f + FIG_INVISIBLE) = 0;
        S16(st, f + FIG_ROAM_LENGTH) += 1;
        if (S16(st, f + FIG_ROAM_LENGTH) >= S16(st, f + FIG_MAX_ROAM_LENGTH)) {
            uint32_t ba = BUILDING_ADDR(b);
            int32_t r = GAME_CALL(st, FN_BUILDING_ROAD_WITHIN, S16(st, ba + BLD_X), S16(st, ba + BLD_Y), U8(st, ba + BLD_SIZE), 2);
            id = FIGB_CUR(st);
            f = FIGURE_ADDR(id);
            if (r == 0) {
                U8(st, f + FIG_STATE) = 2;
                GAME_CALL(st, FN_FIGURE_ROAM, id, 1);
                return;
            }
            U8(st, f + FIG_ACTION) = 2;
            U16(st, f + FIG_DEST_X) = U16(st, G_ROAD_ACCESS_X); /* the road tile found */
            U16(st, f + FIG_DEST_Y) = U16(st, G_ROAD_ACCESS_Y);
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            id = FIGB_CUR(st);
            S16(st, FIGURE_ADDR(id) + FIG_ROAM_LENGTH) = 0;
        }
        GAME_CALL(st, FN_FIGURE_ROAM, id, 1);
        return;
    }
    if (action == 2) {
        /* returning: removed when arrived (8) or lost (9, 10) */
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = FIGB_F(st);
        int8_t dir = S8(st, f + FIG_DIRECTION);
        if (dir == 9 || dir == 10 || dir == 8) {
            U8(st, f + FIG_STATE) = 2;
        }
        return;
    }
    U8(st, f + FIG_ACTION) = 1;
    S16(st, f + FIG_ROAM_LENGTH) = S16(st, f + FIG_MAX_ROAM_LENGTH);
}

/* common start of the roamer actions: terrain usage 1, +0x0C cleared, roam length */
static uint32_t roamer_start(game_state *st, int16_t max_roam)
{
    uint32_t f = FIGB_F(st);
    U8(st, f + FIG_TERRAIN_USAGE) = 1;
    U8(st, f + FIG_U0C) = 0;
    S16(st, f + FIG_MAX_ROAM_LENGTH) = max_roam;
    return f;
}

/* the building is alive (state 1) and has this figure at +0x2A */
static int home_ok(game_state *st, uint32_t f, int32_t id)
{
    uint32_t ba = BUILDING_ADDR(S16(st, f + FIG_HOME));
    return U8(st, ba + BLD_STATE) == 1 && S16(st, ba + BLD_WALKER) == id;
}

/* the plain service roamer: roam 0x180 (or max_roam), die with the building, image groups */
static void plain_roamer(game_state *st, int16_t max_roam, uint32_t walk, uint32_t die)
{
    int32_t id = FIGB_CUR(st);
    uint32_t f = roamer_start(st, max_roam);
    if (!home_ok(st, f, id)) {
        U8(st, f + FIG_STATE) = 2;
    }
    figb_anim_advance(st, f, 12);
    figb_roamer_action(st);
    figb_walker_image(st, walk, die);
}

/* @bin 0x00494200 .. 0x004946B0 six service roamers, identical but for their sprite groups */
void figb_action_1d(game_state *st) { plain_roamer(st, 0x180, G_IMAGE_GROUP_96, G_IMAGE_GROUP_97); } /* @bin 0x00494200 */
void figb_action_1e(game_state *st) { plain_roamer(st, 0x180, G_IMAGE_GROUP_240, G_IMAGE_GROUP_241); } /* @bin 0x004942F0 */
void figb_action_1f(game_state *st) { plain_roamer(st, 0x180, G_IMAGE_GROUP_221, G_IMAGE_GROUP_222); } /* @bin 0x004943E0 */
void figb_action_20(game_state *st) { plain_roamer(st, 0x180, G_IMAGE_GROUP_110, G_IMAGE_GROUP_111); } /* @bin 0x004944D0 */
void figb_action_21(game_state *st) { plain_roamer(st, 0x180, G_IMAGE_GROUP_219, G_IMAGE_GROUP_220); } /* @bin 0x004945C0 */
void figb_action_22(game_state *st) { plain_roamer(st, 0x180, G_IMAGE_GROUP_234, G_IMAGE_GROUP_235); } /* @bin 0x004946B0 */
/* @bin 0x0049B2A0 the same with a roam length of 0xC0 */
void figb_action_40(game_state *st) { plain_roamer(st, 0xC0, G_IMAGE_GROUP_245, G_IMAGE_GROUP_246); }

void figb_action_1a(game_state *st)
{
    /* @bin 0x00493EC0 a roamer whose building may hold it at +0x2A or +0xB4; it returns early
     * after 0x60 ticks when its building has nothing left (0x004578B0 + 0x004579D0 <= 0) */
    int32_t id = FIGB_CUR(st);
    uint32_t f = roamer_start(st, 0x180);
    int32_t b = S16(st, f + FIG_HOME);
    uint32_t ba = BUILDING_ADDR(b);
    if (U8(st, ba + BLD_STATE) != 1) {
        U8(st, f + FIG_STATE) = 2;
    }
    if (S16(st, ba + BLD_WALKER) != id && S16(st, ba + BLD_SECOND_WALKER) != id) {
        U8(st, f + FIG_STATE) = 2;
    }
    figb_anim_advance(st, f, 12);
    if (U8(st, f + FIG_ACTION) == 1) {
        int32_t n = GAME_CALL(st, FN_FIGB_STOCK_MAX, b);
        n += GAME_CALL(st, FN_FIGB_STOCK_MAX2, b);
        f = FIGB_F(st);
        if (S16(st, f + FIG_ROAM_LENGTH) >= 0x60 && n <= 0) {
            S16(st, f + FIG_ROAM_LENGTH) = S16(st, f + FIG_MAX_ROAM_LENGTH);
        }
    }
    figb_roamer_action(st);
    figb_walker_image(st, G_IMAGE_GROUP_57, G_IMAGE_GROUP_58);
}

void figb_action_1b(game_state *st)
{
    /* @bin 0x00494010 a roamer removed when its building is not alive, has +0x4A set or another
     * figure; its sprite groups depend on 0x004C5120(building) */
    int32_t id = FIGB_CUR(st);
    uint32_t f = roamer_start(st, 0x180);
    int32_t b = S16(st, f + FIG_HOME);
    uint32_t ba = BUILDING_ADDR(b);
    if (U8(st, ba + BLD_STATE) != 1 || S16(st, ba + BLD_BURNING) != 0 || S16(st, ba + BLD_WALKER) != id) {
        U8(st, f + FIG_STATE) = 2;
        return;
    }
    figb_anim_advance(st, f, 12);
    if (U8(st, f + FIG_ACTION) == 8) {
        U8(st, f + FIG_ACTION) = 1;
    } else {
        figb_roamer_action(st);
    }
    figb_dir_rel(st);
    uint32_t kind = (uint32_t) GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, b);
    int walk, die; /* @bin 0x00494190 jump table */
    switch (kind) {
    case 1: walk = 0xF9; die = 0xFA; break;
    case 2: walk = 0xE2; die = 0xE3; break;
    case 3: walk = 0xE8; die = 0xE9; break;
    case 4: walk = 0xF7; die = 0xF8; break;
    default: walk = 0xEC; die = 0xED; break;
    }
    f = FIGB_F(st);
    if (U8(st, f + FIG_ACTION) == 4) {
        U32(st, f + FIG_U04) = U8(st, G_DIE_FRAMES + S16(st, f + FIG_WAIT_TICKS) / 2) + U32(st, FIGB_SPRITE(die));
        return;
    }
    U32(st, f + FIG_U04) = U32(st, FIGB_SPRITE(walk)) + 8u * U8(st, f + FIG_ANIM_FRAME) + U32(st, G_SPRITE_DIRECTION);
}

void figb_action_1c(game_state *st)
{
    /* @bin 0x004941B0 an animation of 0x17 frames, then removed */
    uint32_t f = FIGB_F(st);
    int16_t t = S16(st, f + FIG_WAIT_TICKS);
    S16(st, f + FIG_WAIT_TICKS) = (int16_t) (t + 1);
    if (t > 0x15) {
        U8(st, f + FIG_STATE) = 2;
    }
    U32(st, f + FIG_U04) = (uint32_t) (S16(st, f + FIG_WAIT_TICKS) + S32(st, G_IMAGE_GROUP_588) + 1);
}

void figb_action_23(game_state *st)
{
    /* @bin 0x004947A0 only checks its building */
    int32_t id = FIGB_CUR(st);
    uint32_t f = roamer_start(st, 0x180);
    if (!home_ok(st, f, id)) {
        U8(st, f + FIG_STATE) = 2;
    }
}

void figb_action_28(game_state *st)
{
    /* @bin 0x00496F20 a roamer (0x80) that only needs its building alive */
    uint32_t f = roamer_start(st, 0x80);
    if (U8(st, BUILDING_ADDR(S16(st, f + FIG_HOME)) + BLD_STATE) != 1) {
        U8(st, f + FIG_STATE) = 2;
    }
    figb_anim_advance(st, f, 12);
    figb_roamer_action(st);
    figb_walker_image(st, G_IMAGE_GROUP_238, G_IMAGE_GROUP_239);
}

void figb_action_3f(game_state *st)
{
    /* @bin 0x0049B280 a fixed image */
    U32(st, FIGB_F(st) + FIG_U04) = U32(st, G_IMAGE_GROUP_62);
}

void figb_action_41(game_state *st)
{
    /* @bin 0x0049B390 */
    uint32_t f = FIGB_F(st);
    U8(st, f + FIG_TERRAIN_USAGE) = 0;
    U8(st, f + FIG_INVISIBLE) = 1;
    U8(st, f + FIG_U0C) = 1;
}

int32_t figb_chain_head_27(game_state *st, int32_t id)
{
    /* @bin 0x0049B3C0 follow +0x68 up to 8 times to a figure of type 0x27 (0 if none) */
    int32_t l = S16(st, FIGURE_ADDR(id) + FIG_LEADER);
    for (int i = 0; i < 8; i++) {
        uint32_t lf = FIGURE_ADDR(l);
        if (U8(st, lf + FIG_TYPE) == 0x27) {
            return l;
        }
        l = S16(st, lf + FIG_LEADER);
    }
    return 0;
}

void figb_action_42(game_state *st)
{
    /* @bin 0x0049B400 a follower of a type 0x27 (or 0x42) figure */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    int32_t leader = S16(st, f + FIG_LEADER);
    int32_t b = S16(st, f + FIG_HOME);
    int32_t head = figb_chain_head_27(st, id);
    id = FIGB_CUR(st);
    f = FIGURE_ADDR(id);
    U8(st, f + FIG_INVISIBLE) = 0;
    U8(st, f + FIG_TERRAIN_USAGE) = 1;
    figb_anim_advance(st, f, 12);
    U32(st, f + FIG_U180) = 0;
    uint32_t lf = FIGURE_ADDR(leader);
    uint32_t hf = FIGURE_ADDR(head);
    if (leader <= 0) {
        U8(st, f + FIG_STATE) = 2;
    } else if (U8(st, lf + FIG_ACTION) == 4) {
        U8(st, f + FIG_STATE) = 2;
    } else if (U8(st, lf + FIG_STATE) != 1) {
        /* @bin 0x0049B4C0 the leader is gone: its building gets 100 of the resource back */
        U8(st, f + FIG_STATE) = 2;
        S16(st, BUILDING_ADDR(b) + BLD_INVENTORY + 2u * U8(st, f + FIG_COLLECTING)) += 100;
    } else if (U8(st, lf + FIG_TYPE) != 0x27 && U8(st, lf + FIG_TYPE) != 0x42) {
        U8(st, f + FIG_STATE) = 2;
    } else if (U8(st, hf + FIG_ACTION) != 7 && U8(st, hf + FIG_ACTION) != 6) {
        GAME_CALL(st, FN_FIGB_FOLLOW, id, S16(st, f + FIG_LEADER), 1);
        id = FIGB_CUR(st);
    }
    if (U8(st, hf + FIG_ACTION) == 7) {
        U8(st, FIGURE_ADDR(id) + FIG_ANIM_FRAME) = 0;
    }
    if (U8(st, hf + FIG_ACTION) == 6) {
        U8(st, FIGURE_ADDR(id) + FIG_INVISIBLE) = 1;
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
    if (U8(st, f + FIG_ACTION) == 4) {
        U32(st, f + FIG_U04) = U8(st, G_DIE_FRAMES + S16(st, f + FIG_WAIT_TICKS) / 2) + U32(st, G_IMAGE_GROUP_48) + 0x60u;
        return;
    }
    U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_48) + 8u * U8(st, f + FIG_ANIM_FRAME) + (uint32_t) d;
}

void figb_action_27(game_state *st)
{
    /* @bin 0x00496140 the leader of a type 0x42 chain: walks to a building and back */
    int32_t id = FIGB_CUR(st);
    uint32_t f = roamer_start(st, 0x320);
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    int32_t b = S16(st, f + FIG_HOME);
    uint32_t ba = BUILDING_ADDR(b);
    if (U8(st, ba + BLD_STATE) != 1) {
        U8(st, f + FIG_STATE) = 2;
    }
    if (S16(st, ba + BLD_WALKER2) != id && S16(st, ba + BLD_SECOND_CART) != id) {
        U8(st, f + FIG_STATE) = 2;
    }
    figb_anim_advance(st, f, 12);
    int back = 0; /* @bin 0x00496235 return to the source tile */
    int8_t dir;
    switch (U8(st, f + FIG_ACTION)) {
    case 4:
        GAME_CALL(st, FN_FIGURE_DYING, id);
        break;
    case 5:
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        break;
    case 6:
        U32(st, f + FIG_U04) = 0;
        break;
    case 7:
        U8(st, f + FIG_ANIM_FRAME) = 0;
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 100) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
            if ((int8_t) GAME_CALL(st, FN_FIGURE_MAY_LEAVE, id) == 0) {
                U8(st, FIGB_F(st) + FIG_ACTION) = 9;
            }
        }
        break;
    case 8:
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            GAME_CALL(st, FN_FIGB_DELIVER, FIGB_CUR(st), b, S16(st, f + FIG_DEST_BUILDING));
        } else if (dir > 8 && dir <= 10) {
            back = 1;
        }
        break;
    case 9:
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + FIG_DIRECTION);
        if (dir == 8 || dir == 10) {
            U8(st, f + FIG_STATE) = 2;
        } else if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
        }
        break;
    default:
        back = 1;
        break;
    }
    if (back) {
        f = FIGB_F(st);
        U8(st, f + FIG_ACTION) = 9;
        U16(st, f + FIG_DEST_X) = U16(st, f + FIG_SOURCE_X);
        U16(st, f + FIG_DEST_Y) = U16(st, f + FIG_SOURCE_Y);
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
    }
    figb_walker_image(st, G_IMAGE_GROUP_55, G_IMAGE_GROUP_56);
}

void figb_action_24(game_state *st)
{
    /* @bin 0x00494800 a figure placed on a tile from tables at 0x00784B38.. by its index at +0x0B,
     * an 8-frame animation and an overlay image (+0x180) */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    figb_anim_advance(st, f, 8);
    U32(st, f + FIG_U04) = U8(st, f + FIG_ANIM_FRAME) + U32(st, G_IMAGE_GROUP_408);
    GAME_CALL(st, FN_FIGURE_TILE_LIST_REMOVE, id);
    id = FIGB_CUR(st);
    f = FIGURE_ADDR(id);
    uint8_t k = U8(st, f + FIG_RESOURCE);
    uint32_t over;
    uint16_t x, y;
    if (k == 1) {
        x = U8(st, G_U784C60);
        y = U8(st, G_U784C62);
        over = U32(st, G_IMAGE_GROUP_409);
    } else if (k == 2) {
        x = U8(st, G_SCEN_ENTRY_X);
        y = U8(st, G_SCEN_ENTRY_Y);
        over = U32(st, G_IMAGE_GROUP_409) + 2u;
    } else if (k == 3) {
        x = U8(st, G_SCEN_EXIT_X);
        y = U8(st, G_SCEN_EXIT_Y);
        over = U32(st, G_IMAGE_GROUP_409) + 3u;
    } else if (k == 0x0C) {
        x = U8(st, G_ENTRY_X);
        y = U8(st, G_ENTRY_Y);
        over = U32(st, G_IMAGE_GROUP_409) + 4u;
    } else if (k == 0x0D) {
        x = U8(st, G_EXIT_X);
        y = U8(st, G_EXIT_Y);
        over = U32(st, G_IMAGE_GROUP_409) + 5u;
    } else if (k >= 0x0E && k <= 0x15) {
        x = U8(st, G_U784B58 + 2u * k);
        y = U8(st, G_U784B68 + 2u * k);
        over = U32(st, G_RESOURCE_ICONS) + 7u;
    } else if (k >= 0x16 && k <= 0x19) {
        x = U8(st, G_U784B38 + 2u * k);
        y = U8(st, G_U784B40 + 2u * k);
        over = U32(st, G_IMAGE_GROUP_396) + 1u;
    } else if (k >= 0x1A && k <= 0x1D) {
        x = U8(st, G_U784C58 + 4u * k);
        y = U8(st, G_SCEN_EXIT_X + 4u * k);
        over = U32(st, G_IMAGE_GROUP_396) + 5u;
    } else if (k >= 0x1E && k <= 0x20) {
        x = U8(st, G_U784D4C + 4u * k);
        y = U8(st, G_U784D58 + 4u * k);
        over = U32(st, G_IMAGE_GROUP_446);
    } else {
        x = U8(st, G_U784B5C + 2u * k);
        y = U8(st, G_U784B7C + 2u * k);
        over = (int32_t) k - 0x32 < 8 ? U32(st, G_IMAGE_GROUP_409) + 1u : U32(st, G_IMAGE_GROUP_348);
    }
    U16(st, f + FIG_X) = x;
    U16(st, f + FIG_Y) = y;
    U32(st, f + FIG_U180) = over;
    S16(st, f + FIG_CC_X) = (int16_t) (S16(st, f + FIG_X) * 15);
    S16(st, f + FIG_CC_Y) = (int16_t) (S16(st, f + FIG_Y) * 15);
    U32(st, f + FIG_GRID_OFFSET) = U32(st, G_MAP_START) + (uint32_t) S16(st, f + FIG_X) + 228u * (uint32_t) S16(st, f + FIG_Y);
    S16(st, f + FIG_CC_X) += 7;
    S16(st, f + FIG_CC_Y) += 7;
    GAME_CALL(st, FN_FIGURE_TILE_LIST_ADD, id);
}

void figb_action_25(game_state *st)
{
    /* @bin 0x00494B30 a figure that goes from the point at 0x00784C8C to the one at 0x00784C90 and
     * waits a random time (CRT rand) by how long the trip took; images by its kind at +0x0B */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_BOAT_MODE) = 2;
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    if (S16(st, G_EXIT_X) == -1 || S16(st, G_EXIT_Y) == -1) {
        return;
    }
    U8(st, f + FIG_INVISIBLE) = 0;
    U32(st, f + FIG_U180) = 0;
    U8(st, f + FIG_TERRAIN_USAGE) = 0;
    switch (U8(st, f + FIG_ACTION)) {
    case 8:
        U8(st, f + FIG_INVISIBLE) = 1;
        S16(st, f + FIG_WAIT_TICKS) -= 1;
        if (S16(st, f + FIG_WAIT_TICKS) <= 0) {
            uint32_t c = CURRENT_CITY(st);
            U8(st, f + FIG_ACTION) = 9;
            S16(st, f + FIG_WAIT_TICKS) = 0;
            if (S32(st, c + CITY_U4700) != 0 && U8(st, f + FIG_RESOURCE) == 0) { /* 0x00EA5F70 */
                U8(st, f + FIG_SERVICE_MAX) = 1;
                S32(st, c + CITY_U4700) = 0;
            }
            U16(st, f + FIG_DEST_X) = U8(st, G_EXIT_X);
            U16(st, f + FIG_DEST_Y) = U8(st, G_EXIT_Y);
        }
        break;
    case 9:
        if (U8(st, f + FIG_U03) == 0) {
            U8(st, f + FIG_U03) = 1;
            S16(st, f + FIG_WAIT_TICKS) += 1;
            GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
            f = FIGB_F(st);
            U8(st, f + FIG_INVISIBLE) = 0;
            U8(st, f + FIG_HEIGHT_TICKS) = 0;
            int8_t dir = S8(st, f + FIG_DIRECTION);
            if (dir == 8 || dir == 9 || dir == 10) {
                U8(st, f + FIG_ACTION) = 0x0A;
            }
        } else {
            U8(st, f + FIG_U03) = 0;
        }
        break;
    case 0x0A: {
        U8(st, f + FIG_INVISIBLE) = 1;
        U8(st, f + FIG_SERVICE_MAX) = 0;
        U8(st, f + FIG_ACTION) = 8;
        int16_t t = S16(st, f + FIG_WAIT_TICKS);
        int32_t w;
        if (t >= 400) {
            w = crt_rand(st) & 7;
        } else if (t >= 200) {
            w = (crt_rand(st) & 0xF) + 0x32;
        } else if (t >= 100) {
            w = (crt_rand(st) & 0x1F) + 100;
        } else if (t >= 0x32) {
            w = (crt_rand(st) & 0x3F) + 200;
        } else {
            w = crt_rand(st) + 300;
        }
        S16(st, FIGB_F(st) + FIG_WAIT_TICKS) = (int16_t) w;
        GAME_CALL(st, FN_FIGURE_TILE_LIST_REMOVE, FIGB_CUR(st));
        f = FIGB_F(st);
        U16(st, f + FIG_X) = U8(st, G_ENTRY_X);
        U16(st, f + FIG_Y) = U8(st, G_ENTRY_Y);
        U32(st, f + FIG_GRID_OFFSET) = U32(st, G_MAP_START) + (uint32_t) S16(st, f + FIG_X) + 228u * (uint32_t) S16(st, f + FIG_Y);
        S16(st, f + FIG_CC_X) = (int16_t) (S16(st, f + FIG_X) * 15);
        S16(st, f + FIG_CC_Y) = (int16_t) (S16(st, f + FIG_Y) * 15);
        break;
    }
    default:
        U8(st, f + FIG_ACTION) = 8;
        break;
    }
    f = FIGB_F(st);
    uint8_t k = U8(st, f + FIG_RESOURCE);
    if (k == 0) {
        figb_anim_advance(st, f, 12);
        uint32_t fr = U32(st, G_U5DC7EC + 4u * U8(st, f + FIG_ANIM_FRAME));
        U32(st, f + FIG_U04) = fr + U32(st, U8(st, f + FIG_SERVICE_MAX) ? G_IMAGE_GROUP_432 : G_IMAGE_GROUP_422);
    } else if (k == 1 || k == 2) {
        figb_anim_advance(st, f, 0x18);
        U32(st, f + FIG_U04) = U32(st, G_U5DC81C + 4u * U8(st, f + FIG_ANIM_FRAME)) + U32(st, k == 1 ? G_IMAGE_GROUP_423 : G_IMAGE_GROUP_424);
    } else if (k == 3) {
        figb_anim_advance(st, f, 0x18);
        int32_t fr = S32(st, G_U5DC87C + 4u * U8(st, f + FIG_ANIM_FRAME));
        if (fr == -1) {
            U32(st, f + FIG_U04) = 0;
        } else {
            U32(st, f + FIG_U04) = (uint32_t) fr + U32(st, G_IMAGE_GROUP_425);
        }
    }
    if (S32(st, G_EDITOR) == 1 && U8(st, G_U784CB1) == 0) {
        U32(st, f + FIG_U04) = 0;
    }
}
