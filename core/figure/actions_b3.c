/* Imhotep - figure actions, group b: figure type 0x26.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21). */
#include "figure/actions_b.h"

#include "sim/call.h"
#include "state/layout.h"

/* @bin 0x00495310 after the moves to the destination: removed if the destination is gone */
static void check_destination(game_state *st)
{
    uint32_t f = FIGB_F(st);
    if (U8(st, BUILDING_ADDR(S16(st, f + FIG_DEST_BUILDING)) + BLD_STATE) != 1) {
        U8(st, f + FIG_STATE) = 2;
    }
}

/* walk; returns the direction after the move (8 arrived, 9 reroute, 10 lost) */
static int8_t walk(game_state *st, int32_t speed)
{
    GAME_CALL(st, FN_CART_SPRITE_LOAD, FIGB_CUR(st));
    GAME_CALL(st, FN_FIGURE_MOVE, FIGB_CUR(st), speed);
    return S8(st, FIGB_F(st) + FIG_DIRECTION);
}

/* @bin 0x004954BB back to the source tile (action 0xE) */
static void head_back(game_state *st, uint32_t f)
{
    U8(st, f + FIG_ACTION) = 0x0E;
    S16(st, f + FIG_WAIT_TICKS) = 0;
    U16(st, f + FIG_DEST_X) = U16(st, f + FIG_SOURCE_X);
    U16(st, f + FIG_DEST_Y) = U16(st, f + FIG_SOURCE_Y);
}

void figb_action_26(game_state *st)
{
    /* @bin 0x00494F70 a figure of a type 0x4B / 0x4C / 0xC0 building: takes turns at the building
     * (+0x54 holds the figure being served), delivers to a destination and comes back */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    figb_anim_advance(st, f, 12);
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    U32(st, f + FIG_U180) = 0;
    U8(st, f + FIG_INVISIBLE) = 0;
    int32_t b = S16(st, f + FIG_HOME);
    uint32_t ba = BUILDING_ADDR(b);
    if (U8(st, ba + BLD_STATE) != 1) {
        U8(st, f + FIG_STATE) = 2;
    }
    int16_t bt = S16(st, ba + BLD_TYPE);
    if (bt != 0x4B && bt != 0x4C && bt != 0xC0) {
        U8(st, f + FIG_STATE) = 2;
    }
    if (U8(st, ba + BLD_UA7) != 0) {
        U8(st, ba + BLD_UA7) -= 1;
    }
    int16_t w = S16(st, ba + BLD_SECOND_WALKER);
    if (w != 0) {
        uint32_t wf = FIGURE_ADDR(w);
        if (U8(st, wf + FIG_STATE) != 1 || U8(st, wf + FIG_TYPE) != 0x14 || U8(st, wf + FIG_ACTION) == 0x0D) {
            S16(st, ba + BLD_SECOND_WALKER) = 0;
        }
    }
    U8(st, f + FIG_TERRAIN_USAGE) = 1;
    int8_t dir;
    switch (U8(st, f + FIG_ACTION)) {
    case 5:
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        break;
    case 4:
        GAME_CALL(st, FN_FIGURE_DYING, id);
        break;
    case 8:
        U8(st, f + FIG_RESOURCE) = 0;
        U32(st, f + FIG_U180) = 0;
        if (GAME_CALL(st, FN_FIGB_DOCK_LOAD, b) == 0) {
            GAME_CALL(st, FN_FIGB_DOCK_UNLOAD, b);
        }
        f = FIGB_F(st);
        U8(st, f + FIG_ANIM_FRAME) = 0;
        /* @bin 0x004950F6 terrain bit 0x40 under the figure */
        if ((U8(st, G_GRID_TERRAIN + 4u * U32(st, f + FIG_GRID_OFFSET)) & 0x40) == 0) {
            U8(st, f + FIG_STATE) = 2;
        }
        break;
    case 9:
        U32(st, f + FIG_U180) = 0;
        U8(st, f + FIG_ANIM_FRAME) = 0;
        if (S16(st, ba + BLD_DOCK_FIGURE) <= 0) {
            S16(st, ba + BLD_DOCK_FIGURE) = (int16_t) id;
            S16(st, f + FIG_WAIT_TICKS) = 0;
        }
        if (S16(st, ba + BLD_DOCK_FIGURE) == id) {
            S16(st, f + FIG_WAIT_TICKS) += 1;
            U8(st, ba + BLD_UA7) = 0x78;
            if (S16(st, f + FIG_WAIT_TICKS) >= 0x50) {
                U8(st, f + FIG_ACTION) = 0x0B;
                S16(st, f + FIG_WAIT_TICKS) = 0;
                GAME_CALL(st, FN_CART_SPRITE_LOAD, id);
                S16(st, ba + BLD_DOCK_FIGURE) = 0;
            }
        } else {
            /* @bin 0x00495187 the served figure must be one of the three at +0xAE, waiting */
            int found = 0;
            for (int i = 0; i < 3; i++) {
                int16_t o = S16(st, ba + BLD_DOCKERS + 2u * (uint32_t) i);
                if (o == 0) {
                    continue;
                }
                uint32_t of = FIGURE_ADDR(o);
                if (U8(st, of + FIG_STATE) != 1) {
                    continue;
                }
                uint8_t oa = U8(st, of + FIG_ACTION);
                if ((oa == 9 || oa == 0x0A) && S16(st, ba + BLD_DOCK_FIGURE) == o) {
                    found = 1;
                }
            }
            if (found != 1) {
                S16(st, ba + BLD_DOCK_FIGURE) = 0;
            }
        }
        break;
    case 0x0A:
        GAME_CALL(st, FN_CART_SPRITE_LOAD, id);
        id = FIGB_CUR(st);
        f = FIGURE_ADDR(id);
        if (S16(st, ba + BLD_DOCK_FIGURE) <= 0) {
            S16(st, ba + BLD_DOCK_FIGURE) = (int16_t) id;
            S16(st, f + FIG_WAIT_TICKS) = 0;
        }
        if (S16(st, ba + BLD_DOCK_FIGURE) == id) {
            U8(st, ba + BLD_UA7) = 0x78;
            S16(st, f + FIG_WAIT_TICKS) += 1;
            if (S16(st, f + FIG_WAIT_TICKS) >= 0x50) {
                U8(st, f + FIG_ACTION) = 8;
                S16(st, f + FIG_WAIT_TICKS) = 0;
                U32(st, f + FIG_U04) = 0;
                U32(st, f + FIG_U180) = 0;
                S16(st, ba + BLD_DOCK_FIGURE) = 0;
            }
        }
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) >= 0x14) {
            U8(st, f + FIG_ACTION) = 8;
            S16(st, f + FIG_WAIT_TICKS) = 0;
        }
        U8(st, f + FIG_ANIM_FRAME) = 0;
        break;
    case 0x0B:
    case 0x0C:
    case 0x0D: {
        uint8_t a = U8(st, f + FIG_ACTION);
        dir = walk(st, speed);
        f = FIGB_F(st);
        if (dir == 8) {
            if (a == 0x0B) {
                U8(st, f + FIG_ACTION) = 0x0F;
            } else if (a == 0x0C) {
                U8(st, f + FIG_ACTION) = 0x10;
            } else {
                U8(st, f + FIG_ACTION) = 0x0A;
                S16(st, f + FIG_WAIT_TICKS) = 0;
            }
        } else if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
        } else if (dir == 10) {
            U8(st, f + FIG_STATE) = 2;
        }
        check_destination(st);
        break;
    }
    case 0x0F:
        GAME_CALL(st, FN_CART_SPRITE_LOAD, id);
        f = FIGB_F(st);
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 10) {
            int32_t o = S16(st, ba + BLD_SECOND_WALKER);
            int32_t v = o != 0 ? U8(st, FIGURE_ADDR(o) + FIG_U6E) : 0;
            int32_t dest = S16(st, f + FIG_DEST_BUILDING);
            int16_t dt = S16(st, BUILDING_ADDR(dest) + BLD_TYPE);
            if ((dt == 0x48 || dt == 0xC1) && GAME_CALL(st, FN_STORAGE_TAKE_FROM_TRADER, dest, U8(st, f + FIG_RESOURCE), v, o) != 0) {
                f = FIGB_F(st);
                head_back(st, f);
                U8(st, f + FIG_RESOURCE) = 0;
                GAME_CALL(st, FN_FIGB_DOCK_UNLOAD, b);
            } else {
                head_back(st, FIGB_F(st));
            }
            f = FIGB_F(st);
            S16(st, f + FIG_WAIT_TICKS) = 0;
        }
        U8(st, f + FIG_ANIM_FRAME) = 0;
        break;
    case 0x10: {
        GAME_CALL(st, FN_CART_SPRITE_LOAD, id);
        int32_t o = S16(st, ba + BLD_SECOND_WALKER);
        int32_t v = o != 0 ? U8(st, FIGURE_ADDR(o) + FIG_U6E) : 0;
        f = FIGB_F(st);
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 10) {
            head_back(st, f);
            S16(st, f + FIG_WAIT_TICKS) = 0;
            if (GAME_CALL(st, FN_FIGURE_UNLOAD_TO_WORKS, S16(st, f + FIG_DEST_BUILDING), U8(st, f + FIG_RESOURCE), v, o) != 0) {
                U8(st, FIGB_F(st) + FIG_ACTION) = 0x0D;
            } else {
                GAME_CALL(st, FN_FIGB_DOCK_UNLOAD, b);
            }
        }
        U8(st, FIGB_F(st) + FIG_ANIM_FRAME) = 0;
        break;
    }
    case 0x0E:
        dir = walk(st, speed);
        f = FIGB_F(st);
        if (dir == 8) {
            U8(st, f + FIG_ACTION) = 8;
        } else if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
        } else if (dir == 10) {
            U8(st, f + FIG_STATE) = 2;
        }
        break;
    case 7:
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_165) + U32(st, G_SPRITE_DIRECTION) + 1u;
        U8(st, f + FIG_ANIM_FRAME) = 0;
        GAME_CALL(st, FN_CART_SPRITE_LOAD, id);
        f = FIGB_F(st);
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 100) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
            if ((int8_t) GAME_CALL(st, FN_FIGURE_MAY_LEAVE, FIGB_CUR(st)) == 0) {
                U8(st, FIGB_F(st) + FIG_ACTION) = 8;
            }
        }
        break;
    case 6:
        U32(st, f + FIG_U04) = 0;
        if (U32(st, f + FIG_U180) != 0) {
            U32(st, f + FIG_U180) = 0;
        }
        break;
    default:
        U8(st, f + FIG_ACTION) = 8;
        break;
    }
    /* @bin 0x0049507B image, and the carried-resource overlay (+0x180, +0x6C, +0x6D) */
    f = FIGB_F(st);
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
    uint8_t act = U8(st, f + FIG_ACTION);
    if (act == 4) {
        U32(st, f + FIG_U04) = U8(st, G_DIE_FRAMES + S16(st, f + FIG_WAIT_TICKS) / 2) + U32(st, G_IMAGE_GROUP_166);
        U32(st, f + FIG_U180) = 0;
    } else if (act != 6) {
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_165) + 8u * U8(st, f + FIG_ANIM_FRAME) + (uint32_t) d;
    }
    uint32_t over = U32(st, f + FIG_U180);
    if (over == 0) {
        U32(st, f + FIG_U04) = 0;
        return;
    }
    U32(st, f + FIG_U180) = over + (uint32_t) d;
    uint8_t r = U8(st, f + FIG_RESOURCE);
    int32_t k = d;
    /* @bin 0x00495828 resources drawn from the opposite side */
    if (r == 0x18 || r == 0x19 || r == 0x1A || r == 0x1E || r == 0x23 || r == 0x0C) {
        k = d + 4;
        if (k >= 8) {
            k -= 8;
        }
    }
    U8(st, f + FIG_U6C) = U8(st, G_CARRY_OFFSET_X + 2u * (uint32_t) k);
    U8(st, f + FIG_U6D) = U8(st, G_CARRY_OFFSET_Y + 2u * (uint32_t) k);
}

void figb_action_2a(game_state *st)
{
    /* @bin 0x00497450 a figure that leaves its building (+0x2A) to work on water tiles (terrain
     * 0x4000 / 0x8000 under it), with a timed action 0xA (property +0x1C / +0x18) */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_TERRAIN_USAGE) = 4;
    U8(st, f + FIG_U0C) = 0;
    U8(st, f + FIG_INVISIBLE) = 1;
    U8(st, f + FIG_HEIGHT_TICKS) = 10;
    S16(st, f + FIG_MAX_ROAM_LENGTH) = 800;
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    uint32_t ba = BUILDING_ADDR(S16(st, f + FIG_HOME));
    if (U8(st, ba + BLD_STATE) != 1 || S16(st, ba + BLD_WALKER) != id) {
        U8(st, f + FIG_STATE) = 2;
    }
    figb_anim_advance(st, f, 12);
    uint8_t a = U8(st, f + FIG_ACTION);
    if (a != 0x0C && a != 7 && a != 6) {
        GAME_CALL(st, FN_FIGB_LOOK_FOR_TARGET, id);
    }
    id = FIGB_CUR(st);
    f = FIGURE_ADDR(id);
    int8_t dir;
    switch (U8(st, f + FIG_ACTION)) {
    case 5:
        GAME_CALL(st, FN_FIGURE_SOUND_53B4A0, id, 9, 2);
        GAME_CALL(st, FN_FIGURE_FIGHT, FIGB_CUR(st));
        break;
    case 4:
        GAME_CALL(st, FN_FIGURE_DYING, id);
        break;
    case 8:
        U8(st, f + FIG_ANIM_FRAME) = 0;
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) <= 0x28) {
            break;
        }
        S16(st, f + FIG_WAIT_TICKS) = 0;
        if (GAME_CALL(st, FN_FIGB_WATER_POINT, id) == 0) {
            break;
        }
        f = FIGB_F(st);
        U8(st, f + FIG_ACTION) = 9;
        U16(st, f + FIG_DEST_X) = U16(st, G_ROAD_ACCESS_X);
        U16(st, f + FIG_DEST_Y) = U16(st, G_ROAD_ACCESS_Y);
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
        break;
    case 0x0C:
        U8(st, f + FIG_TERRAIN_USAGE) = 10;
        U8(st, f + FIG_INVISIBLE) = 0;
        U8(st, f + FIG_HEIGHT_TICKS) = 0;
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + FIG_DIRECTION);
        if (dir == 9 || dir == 10) {
            U8(st, f + FIG_STATE) = 2;
        } else if (dir == 8) {
            /* @bin 0x0049761F back in the building: placed on its tile */
            GAME_CALL(st, FN_FIGURE_TILE_LIST_REMOVE, FIGB_CUR(st));
            id = FIGB_CUR(st);
            f = FIGURE_ADDR(id);
            uint16_t x = U16(st, ba + BLD_X), y = U16(st, ba + BLD_Y);
            U16(st, f + FIG_X) = x;
            U16(st, f + FIG_DEST_X) = x;
            U16(st, f + FIG_Y) = y;
            U16(st, f + FIG_DEST_Y) = y;
            U32(st, f + FIG_GRID_OFFSET) = U32(st, G_MAP_START) + (uint32_t) S16(st, f + FIG_X) + 228u * (uint32_t) S16(st, f + FIG_Y);
            GAME_CALL(st, FN_FIGURE_TILE_LIST_ADD, id);
            U8(st, FIGB_F(st) + FIG_ACTION) = 8;
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
        }
        break;
    case 9:
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + FIG_DIRECTION);
        if (dir == 9 || dir == 10) {
            U8(st, f + FIG_ACTION) = 8;
        } else if (dir == 8) {
            U8(st, f + FIG_ACTION) = 0x0B;
            U16(st, f + FIG_DEST_X) = U16(st, f + FIG_SOURCE_X);
            U16(st, f + FIG_DEST_Y) = U16(st, f + FIG_SOURCE_Y);
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
        }
        break;
    case 0x0B:
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + FIG_DIRECTION);
        if (dir == 9 || dir == 10) {
            U8(st, f + FIG_STATE) = 2;
        } else if (dir == 8) {
            U8(st, f + FIG_ACTION) = 8;
        }
        break;
    case 0x0A: {
        GAME_CALL(st, FN_FIGURE_SUBSTEPS, id, 1);
        id = FIGB_CUR(st);
        f = FIGURE_ADDR(id);
        uint8_t n = (uint8_t) (U8(st, f + FIG_U6B) + 1);
        U8(st, f + FIG_U6B) = n;
        uint32_t props = G_PROPS + 0x38u * U8(st, f + FIG_TYPE);
        if ((int32_t) n <= S32(st, props + FIGTYPE_PERIOD)) {
            break;
        }
        if (GAME_CALL(st, FN_FIGB_FIND_TARGET, id, S32(st, props + FIGTYPE_RANGE)) != 0) {
            GAME_CALL(st, FN_FIGURE_SOUND_53B4A0, FIGB_CUR(st), 9, 2);
            id = FIGB_CUR(st);
            f = FIGURE_ADDR(id);
            uint32_t tx = U32(st, G_ROAD_ACCESS_X), ty = U32(st, G_ROAD_ACCESS_Y);
            int32_t d = GAME_CALL(st, FN_DIRECTION_TO_FINE, S16(st, f + FIG_X), S16(st, f + FIG_Y), (int32_t) tx, (int32_t) ty);
            U8(st, f + FIG_DIRECTION) = (uint8_t) d;
            U8(st, f + FIG_U6B) = 0;
            GAME_CALL(st, FN_MISSILE_CREATE, FIGB_CUR(st), S16(st, f + FIG_X), S16(st, f + FIG_Y),
                      (int32_t) U32(st, G_ROAD_ACCESS_X), (int32_t) U32(st, G_ROAD_ACCESS_Y), 0x3C);
        } else {
            f = FIGB_F(st);
            U8(st, f + FIG_ACTION) = 0x0B;
            U16(st, f + FIG_DEST_X) = U16(st, f + FIG_SOURCE_X);
            U16(st, f + FIG_DEST_Y) = U16(st, f + FIG_SOURCE_Y);
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
        }
        break;
    }
    case 6:
        U8(st, f + FIG_TERRAIN_USAGE) = 10;
        U32(st, f + FIG_U04) = 0;
        U32(st, f + FIG_U180) = 0;
        break;
    case 7:
        U8(st, f + FIG_TERRAIN_USAGE) = 10;
        U8(st, f + FIG_ANIM_FRAME) = 0;
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 100) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
            if ((int8_t) GAME_CALL(st, FN_FIGURE_MAY_LEAVE, id) == 0) {
                U8(st, FIGB_F(st) + FIG_STATE) = 2;
            }
        }
        break;
    default:
        U8(st, f + FIG_ACTION) = 8;
        break;
    }
    /* @bin 0x00497727 the terrain under the figure */
    f = FIGB_F(st);
    uint32_t t = U32(st, G_GRID_TERRAIN + 4u * U32(st, f + FIG_GRID_OFFSET));
    if (t & 0x4000) {
        U8(st, f + FIG_HEIGHT) = 0x12;
    } else if (t & 0x8000) {
        U8(st, f + FIG_IN_BUILDING_WAIT) = 0x18;
    } else {
        uint8_t act = U8(st, f + FIG_ACTION);
        if (act != 0x0C && act != 7 && act != 6) {
            U8(st, f + FIG_STATE) = 2;
        }
    }
    if (U8(st, f + FIG_IN_BUILDING_WAIT) != 0) {
        U8(st, f + FIG_IN_BUILDING_WAIT) -= 1;
        U8(st, f + FIG_HEIGHT_TICKS) = 0;
    }
    int32_t d = figb_dir_rel(st);
    uint8_t act = U8(st, f + FIG_ACTION);
    if (act == 4) {
        U32(st, f + FIG_U04) = U8(st, G_DIE_FRAMES + S16(st, f + FIG_WAIT_TICKS) / 2) + U32(st, G_IMAGE_GROUP_94);
    } else if (act == 0x0A) {
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_95) + 8u * U8(st, G_U5D050C + (U8(st, f + FIG_U6B) >> 1)) + (uint32_t) d;
    } else {
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_93) + 8u * U8(st, f + FIG_ANIM_FRAME) + (uint32_t) d;
    }
}

void figb_action_3e(game_state *st)
{
    /* @bin 0x0049AEB0 a figure standing at the corner of its building (+0x30) facing the view,
     * acting on targets found by 0x00499B50 (property +0x18 range, +0x1C delay) */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    int shooting = 0;
    U8(st, f + FIG_TERRAIN_USAGE) = 4;
    U8(st, f + FIG_U0C) = 0;
    U8(st, f + FIG_INVISIBLE) = 1;
    U8(st, f + FIG_HEIGHT_TICKS) = 10;
    U8(st, f + FIG_HEIGHT) = 0x2D;
    uint32_t ba = BUILDING_ADDR(S16(st, f + FIG_HOME));
    if (U8(st, ba + BLD_STATE) != 1 || S16(st, ba + BLD_TOWER_WALKER) != id) {
        U8(st, f + FIG_STATE) = 2;
    }
    if (S16(st, ba + BLD_WORKERS) <= 0) {
        U8(st, f + FIG_STATE) = 2;
    }
    if (S16(st, ba + BLD_WALKER) <= 0) {
        U8(st, f + FIG_STATE) = 2;
    }
    GAME_CALL(st, FN_FIGURE_TILE_LIST_REMOVE, id);
    id = FIGB_CUR(st);
    f = FIGURE_ADDR(id);
    /* @bin 0x0049AF4E the building tile nearest to the viewer */
    int32_t view = S32(st, G_VIEW_ORIENTATION);
    uint16_t bx = U16(st, ba + BLD_X), by = U16(st, ba + BLD_Y);
    if (view == 0) {
        U16(st, f + FIG_X) = bx;
        U16(st, f + FIG_Y) = by;
    } else if (view == 2) {
        U16(st, f + FIG_X) = (uint16_t) (bx + 1);
        U16(st, f + FIG_Y) = by;
    } else if (view == 4) {
        U16(st, f + FIG_X) = (uint16_t) (bx + 1);
        U16(st, f + FIG_Y) = (uint16_t) (by + 1);
    } else if (view == 6) {
        U16(st, f + FIG_X) = bx;
        U16(st, f + FIG_Y) = (uint16_t) (by + 1);
    }
    U32(st, f + FIG_GRID_OFFSET) = U32(st, G_MAP_START) + (uint32_t) S16(st, f + FIG_X) + 228u * (uint32_t) S16(st, f + FIG_Y);
    GAME_CALL(st, FN_FIGURE_TILE_LIST_ADD, id);
    id = FIGB_CUR(st);
    f = FIGURE_ADDR(id);
    uint32_t props = G_PROPS + 0x38u * U8(st, f + FIG_TYPE);
    switch (U8(st, f + FIG_ACTION)) {
    case 4:
        U8(st, f + FIG_STATE) = 2;
        break;
    case 8:
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0x14) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
            if (GAME_CALL(st, FN_FIGB_FIND_TARGET, id, S32(st, props + FIGTYPE_RANGE)) != 0) {
                f = FIGB_F(st);
                U8(st, f + FIG_ACTION) = 9;
                U8(st, f + FIG_U6B) = U8(st, G_PROPS + 0x38u * U8(st, f + FIG_TYPE) + FIGTYPE_PERIOD);
            }
        }
        break;
    case 9: {
        uint8_t n = (uint8_t) (U8(st, f + FIG_U6B) + 1);
        U8(st, f + FIG_U6B) = n;
        if ((int32_t) n <= S32(st, props + FIGTYPE_PERIOD)) {
            break;
        }
        shooting = 1;
        if (GAME_CALL(st, FN_FIGB_FIND_TARGET, id, S32(st, props + FIGTYPE_RANGE)) == 0) {
            U8(st, FIGB_F(st) + FIG_ACTION) = 8;
            break;
        }
        f = FIGB_F(st);
        S16(st, f + FIG_WAIT_TICKS) += 1;
        int32_t d = GAME_CALL(st, FN_DIRECTION_TO_FINE, S16(st, f + FIG_X), S16(st, f + FIG_Y),
                              (int32_t) U32(st, G_ROAD_ACCESS_X), (int32_t) U32(st, G_ROAD_ACCESS_Y));
        U8(st, f + FIG_DIRECTION) = (uint8_t) d;
        if (S16(st, f + FIG_WAIT_TICKS) == 10) {
            GAME_CALL(st, FN_MISSILE_CREATE, FIGB_CUR(st), S16(st, f + FIG_X), S16(st, f + FIG_Y),
                      (int32_t) U32(st, G_ROAD_ACCESS_X), (int32_t) U32(st, G_ROAD_ACCESS_Y), 0x3C);
            GAME_CALL(st, FN_RETURN_ZERO, 0x0D);
        } else if (S16(st, f + FIG_WAIT_TICKS) >= 0x18) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
            U8(st, f + FIG_U6B) = 0;
        }
        break;
    }
    default:
        U8(st, f + FIG_ACTION) = 8;
        break;
    }
    int32_t d = figb_dir_rel(st);
    f = FIGB_F(st);
    if (U8(st, f + FIG_ACTION) == 9 && shooting) {
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_596) + 8u * (uint32_t) (S16(st, f + FIG_WAIT_TICKS) / 2) + (uint32_t) d;
        return;
    }
    U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_594) + (uint32_t) d;
}

/* @bin 0x0049A20B find a building to go to (0x004450C0); sets the destination */
static int find_destination(game_state *st)
{
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    int32_t b = GAME_CALL(st, FN_STORAGE_FIND_FOR_TRADER, id, S16(st, f + FIG_X), S16(st, f + FIG_Y), 0, -1);
    if (b == 0) {
        return 0;
    }
    f = FIGB_F(st);
    S16(st, f + FIG_DEST_BUILDING) = (int16_t) b;
    U8(st, f + FIG_ACTION) = 8;
    S16(st, f + FIG_WAIT_TICKS) = 0;
    U16(st, f + FIG_DEST_X) = U16(st, G_ROAD_ACCESS_X);
    U16(st, f + FIG_DEST_Y) = U16(st, G_ROAD_ACCESS_Y);
    U8(st, f + FIG_ANIM_FRAME) = 0;
    return 1;
}

void figb_action_3a(game_state *st)
{
    /* @bin 0x0049A150 a carrier: finds a building (0x004450C0), unloads its resource there
     * (0x0045CF50 / 0x0045D010 / 0x00444BD0), returns */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_INVISIBLE) = 0;
    U8(st, f + FIG_TERRAIN_USAGE) = 0;
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    figb_anim_advance(st, f, 12);
    U32(st, f + FIG_U180) = 0;
    int8_t dir;
    switch (U8(st, f + FIG_ACTION)) {
    case 5:
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        break;
    case 4:
        GAME_CALL(st, FN_FIGURE_DYING, id);
        break;
    case 0x0A:
        U8(st, f + FIG_INVISIBLE) = 1;
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 10) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
            if (find_destination(st)) {
                break;
            }
            f = FIGB_F(st);
            U8(st, f + FIG_STATE) = 2;
        }
        U8(st, f + FIG_ANIM_FRAME) = 0;
        break;
    case 8:
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = FIGB_F(st);
        dir = S8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            U8(st, f + FIG_ACTION) = 0x0B;
        } else if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, FIGB_CUR(st));
        } else if (dir == 10) {
            U8(st, f + FIG_STATE) = 2;
            U8(st, f + FIG_INVISIBLE) = 1;
        }
        f = FIGB_F(st);
        if (U8(st, BUILDING_ADDR(S16(st, f + FIG_DEST_BUILDING)) + BLD_STATE) != 1) {
            U8(st, f + FIG_STATE) = 2;
        }
        break;
    case 0x0B:
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 10) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
            if (GAME_CALL(st, FN_STORAGE_TRADES_24, id, U8(st, f + FIG_OWNER), S16(st, f + FIG_DEST_BUILDING), 0) != 0) {
                f = FIGB_F(st);
                int32_t v = GAME_CALL(st, FN_STORAGE_SELL_NEXT, U8(st, f + FIG_OWNER), S16(st, f + FIG_DEST_BUILDING), 0);
                GAME_CALL(st, FN_FIGURE_SLOT_COUNT_GOOD_B, FIGB_CUR(st), v);
                f = FIGB_F(st);
                U8(st, f + FIG_U6F) += 3;
                U8(st, f + FIG_ANIM_FRAME) = 0;
                break;
            }
            if (find_destination(st)) {
                break;
            }
            f = FIGB_F(st);
            U8(st, f + FIG_ACTION) = 9;
            S16(st, f + FIG_WAIT_TICKS) = 0;
            U16(st, f + FIG_DEST_X) = U16(st, f + FIG_SOURCE_X);
            U16(st, f + FIG_DEST_Y) = U16(st, f + FIG_SOURCE_Y);
        }
        U8(st, f + FIG_ANIM_FRAME) = 0;
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
        U8(st, f + FIG_ACTION) = 0x0A;
        break;
    }
    int32_t d = figb_dir_rel_prev(st);
    f = FIGB_F(st);
    if (U8(st, f + FIG_ACTION) == 4) {
        U32(st, f + FIG_U04) = U8(st, G_DIE_FRAMES + S16(st, f + FIG_WAIT_TICKS) / 2) + U32(st, G_IMAGE_GROUP_83);
        U32(st, f + FIG_U180) = 0;
    } else {
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_82) + 8u * U8(st, f + FIG_ANIM_FRAME) + (uint32_t) d;
    }
    /* @bin 0x0049A55C the carried resource, drawn at offsets by direction (0x005DC23C) */
    uint32_t over = U32(st, G_IMAGE_GROUP_53) + 8u * U8(st, f + FIG_RESOURCE) + 8u;
    U32(st, f + FIG_U180) = over;
    if (over != 0) {
        U32(st, f + FIG_U180) = over + (uint32_t) d;
        U8(st, f + FIG_U6C) = U8(st, G_CARRY_OFFSET_X + 2u * (uint32_t) d);
        U8(st, f + FIG_U6D) = U8(st, G_CARRY_OFFSET_Y + 2u * (uint32_t) d);
    }
}

/* @bin 0x0049A5FD, 0x0049AC5D a missile hits figure t: damage by the missile's attack (property
 * +0x14) against the target's defence (+0x10, the player's formation experience bonus, facing and
 * formation state at 0x00BCB9CC), up to the target's maximum (+0x04); returns t's formation */
static int32_t missile_damage(game_state *st, int32_t t)
{
    uint32_t tf = FIGURE_ADDR(t);
    int32_t form = S16(st, tf + FIG_FORMATION);
    uint32_t rec = 0x90u * (uint32_t) form;
    int32_t bonus = 0;
    if (U8(st, tf + FIG_OWNER) == U32(st, CURRENT_PLAYER_ADDR)) {
        bonus = (S16(st, G_FORMATION_BASE + FM_EXPERIENCE + rec) + 10) / 20;
    }
    uint32_t tprops = G_PROPS + 0x38u * U8(st, tf + FIG_TYPE);
    int32_t maxdmg = S32(st, tprops + FIGTYPE_MAX_DAMAGE);
    int32_t def = S32(st, tprops + FIGTYPE_U10) + bonus;
    if (def >= 0x14) {
        def = 0x14;
    }
    int32_t attack = S32(st, G_PROPS + FIGTYPE_U14 + 0x38u * U8(st, FIGB_F(st) + FIG_TYPE));
    if (S16(st, G_FORMATION_BASE + FM_U5A + rec) != 0) {
        int16_t m = S16(st, G_FORMATION_BASE + FM_TYPE + rec);
        if (m >= 0x0B && m <= 0x0C) {
            if (GAME_CALL(st, FN_DIRECTION_NEAR, S8(st, tf + FIG_ATTACK_DIRECTION), U8(st, G_FORMATION_BASE + FM_FACING + rec)) == 0) {
                def -= 4;
            } else {
                int16_t s = S16(st, G_FORMATION_BASE + FM_LAYOUT + rec);
                if (s >= 3 && s <= 4) {
                    def += 4;
                } else if (s == 6) {
                    def -= 2;
                }
            }
        } else if (m == 0x0D) {
            if (GAME_CALL(st, FN_DIRECTION_NEAR, S8(st, tf + FIG_ATTACK_DIRECTION), U8(st, G_FORMATION_BASE + FM_FACING + rec)) == 0) {
                def -= 4;
            } else if (S16(st, G_FORMATION_BASE + FM_LAYOUT + rec) == 6) {
                def -= 2;
            }
        }
    }
    if (def < 0) {
        def = 0;
    }
    if (def > 0x14) {
        def = 0x14;
    }
    int32_t dmg = 0;
    if (attack != 0) {
        dmg = (int32_t) ((uint32_t) ((30 - def) / 20) * (uint32_t) attack);
        if (dmg < 0) {
            dmg = 0;
        }
    }
    int32_t total = U8(st, tf + FIG_DAMAGE) + dmg;
    if (total > maxdmg) {
        U8(st, tf + FIG_DAMAGE) = (uint8_t) (maxdmg + 1);
        U8(st, tf + FIG_ACTION) = 4;
        S16(st, tf + FIG_WAIT_TICKS) = 0;
        GAME_CALL(st, FN_FIGURE_HIT_SOUND, t);
        GAME_CALL(st, FN_FORMATION_MORALE_HIT, form);
    } else {
        U8(st, tf + FIG_DAMAGE) = (uint8_t) total;
    }
    return form;
}

void figb_action_3b(game_state *st)
{
    /* @bin 0x0049A5A0 a missile: flies (0x004B3FF0) and hits a figure (0x004B5770) */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_U0C) = 1;
    int8_t age = (int8_t) (U8(st, f + FIG_PROGRESS_ON_TILE) + 1);
    U8(st, f + FIG_PROGRESS_ON_TILE) = (uint8_t) age;
    if (age > 0x78) {
        U8(st, f + FIG_STATE) = 2;
    }
    int32_t landed = GAME_CALL(st, FN_FIGURE_STEP_STRAIGHT, id, 4);
    int32_t t = GAME_CALL(st, FN_TILE_TARGET, FIGB_CUR(st));
    if (t == 0) {
        if (landed != 0) {
            U8(st, FIGB_F(st) + FIG_STATE) = 2;
        }
    } else {
        int32_t form = missile_damage(st, t);
        f = FIGB_F(st);
        U8(st, f + FIG_STATE) = 2;
        if (form != 0) {
            uint32_t rec = 0x90u * (uint32_t) form;
            S16(st, G_FORMATION_BASE + FM_U5E + rec) = 6;
            S16(st, G_FORMATION_BASE + FM_U60 + rec) = S16(st, FIGURE_ADDR(S16(st, f + FIG_SHOOTER)) + FIG_FORMATION);
        }
        GAME_CALL(st, FN_RETURN_ZERO, 0x0B);
    }
    /* @bin 0x0049A82E 16 directions */
    f = FIGB_F(st);
    int32_t d = S8(st, f + FIG_DIRECTION) - 2 * S32(st, G_VIEW_ORIENTATION);
    S32(st, G_SPRITE_DIRECTION) = d;
    if (d < 0) {
        d += 0x10;
        S32(st, G_SPRITE_DIRECTION) = d;
    }
    U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_270) + 0x10u + (uint32_t) d;
}

/* @bin 0x0049A964 figure types whose hit points are at +0x128 (maximum +0x12C) */
static int has_hit_points(uint8_t t)
{
    return t == 0x5D || t == 0x5C || t == 0x64 || t == 0x65;
}

void figb_action_3c(game_state *st)
{
    /* @bin 0x0049A880 a missile shot by the figure at +0x5A: on a hit (0x004B56D0) deals the
     * attack of the shooter's kind (0x3E, 0x4E, or its formation's) against the target's defence */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_U0C) = 1;
    int8_t age = (int8_t) (U8(st, f + FIG_PROGRESS_ON_TILE) + 1);
    U8(st, f + FIG_PROGRESS_ON_TILE) = (uint8_t) age;
    if (age > 0x78) {
        U8(st, f + FIG_STATE) = 2;
    }
    int32_t landed = GAME_CALL(st, FN_FIGURE_STEP_STRAIGHT, id, 4);
    int32_t t = GAME_CALL(st, FN_FIGURE_TARGET_ON_TILE, FIGB_CUR(st));
    f = FIGB_F(st);
    uint32_t sf = FIGURE_ADDR(S16(st, f + FIG_SHOOTER)); /* the shooter */
    int32_t sform = S16(st, sf + FIG_FORMATION);
    int dead = 0;
    if (t == 0) {
        dead = landed != 0;
    } else {
        uint32_t tf = FIGURE_ADDR(t);
        int32_t tform = S16(st, tf + FIG_FORMATION);
        int32_t kind;
        uint8_t stype = U8(st, sf + FIG_TYPE);
        if (stype == 0x3E) {
            kind = 0x3E;
        } else if (stype == 0x4E && U8(st, sf + FIG_STATE) == 2) {
            kind = -1;
        } else if (stype == 0x4E) {
            kind = 0x4E;
        } else {
            kind = S16(st, G_FORMATION_BASE + FM_TYPE + 0x90u * (uint32_t) sform);
        }
        if (kind >= 0) {
            uint8_t ttype = U8(st, tf + FIG_TYPE);
            int32_t maxv = has_hit_points(ttype) ? S32(st, tf + FIG_HIT_POINTS_MAX) : S32(st, G_PROPS + FIGTYPE_MAX_DAMAGE + 0x38u * ttype);
            int32_t def = S32(st, G_PROPS + FIGTYPE_U10 + 0x38u * ttype);
            if (def >= 0x14) {
                def = 0x14;
            }
            int32_t atk = S32(st, G_PROPS + FIGTYPE_U14 + 0x38u * (uint32_t) kind);
            int32_t dmg = 0;
            if (atk > 0) {
                dmg = (int32_t) ((uint32_t) (0x14 - def) * (uint32_t) atk) / 20;
                if (dmg < 0) {
                    dmg = 0;
                }
            }
            if (has_hit_points(ttype)) {
                int32_t v = S32(st, tf + FIG_HIT_POINTS) - dmg;
                if (v <= 0) {
                    S32(st, tf + FIG_HIT_POINTS) = 0;
                    U8(st, tf + FIG_ACTION) = 4;
                    S16(st, tf + FIG_WAIT_TICKS) = 0;
                    GAME_CALL(st, FN_FIGURE_HIT_SOUND, t);
                    GAME_CALL(st, FN_FORMATION_MORALE_HIT, tform);
                } else {
                    S32(st, tf + FIG_HIT_POINTS) = v;
                }
            } else {
                int32_t total = U8(st, tf + FIG_DAMAGE) + dmg;
                if (total > maxv) {
                    U8(st, tf + FIG_DAMAGE) = (uint8_t) (maxv + 1);
                    U8(st, tf + FIG_ACTION) = 4;
                    S16(st, tf + FIG_WAIT_TICKS) = 0;
                    GAME_CALL(st, FN_FIGURE_HIT_SOUND, t);
                    GAME_CALL(st, FN_FORMATION_MORALE_HIT, tform);
                    /* @bin 0x0049AA80 a kill by the player's formation raises its experience */
                    if ((int8_t) GAME_CALL(st, FN_FIGURE_TYPE_IS_ENEMY_0X52, U8(st, tf + FIG_TYPE)) == 0 &&
                        (int8_t) GAME_CALL(st, FN_FIGURE_TYPE_IS_0X44_0X46, U8(st, tf + FIG_TYPE)) == 0 && U8(st, tf + FIG_TYPE) != 0x17 &&
                        U8(st, sf + FIG_OWNER) == U32(st, CURRENT_PLAYER_ADDR)) {
                        uint32_t a = G_FORMATION_BASE + FM_EXPERIENCE + 0x90u * (uint32_t) sform;
                        int32_t e = S16(st, a) + 1;
                        if (e >= 100) {
                            e = 100;
                        }
                        S16(st, a) = (int16_t) e;
                    }
                } else {
                    U8(st, tf + FIG_DAMAGE) = (uint8_t) total;
                }
            }
            if (tform != 0) {
                S16(st, G_FORMATION_BASE + FM_U5E + 0x90u * (uint32_t) tform) = 6;
                S16(st, G_FORMATION_BASE + FM_U60 + 0x90u * (uint32_t) tform) = (int16_t) sform;
            }
            GAME_CALL(st, FN_RETURN_ZERO, 0x18);
        }
        dead = 1;
    }
    f = FIGB_F(st);
    if (dead) {
        U8(st, f + FIG_STATE) = 2;
    }
    /* @bin 0x0049AB86 32 directions */
    int32_t d = S8(st, f + FIG_DIRECTION) - 4 * S32(st, G_VIEW_ORIENTATION);
    S32(st, G_SPRITE_DIRECTION) = d;
    if (d < 0) {
        d += 0x20;
        S32(st, G_SPRITE_DIRECTION) = d;
    }
    uint8_t stype = U8(st, sf + FIG_TYPE);
    if (stype != 0x2A && stype != 0x3E) {
        U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_39) + (uint32_t) d;
        U32(st, f + FIG_U180) = U32(st, G_IMAGE_GROUP_40) + (uint32_t) d;
        return;
    }
    U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_621) + (uint32_t) d;
    U32(st, f + FIG_U180) = U32(st, G_IMAGE_GROUP_620) + (uint32_t) d;
}

void figb_action_3d(game_state *st)
{
    /* @bin 0x0049AC00 a missile hitting figures found by 0x004B56D0 */
    int32_t id = FIGB_CUR(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_U0C) = 1;
    int8_t age = (int8_t) (U8(st, f + FIG_PROGRESS_ON_TILE) + 1);
    U8(st, f + FIG_PROGRESS_ON_TILE) = (uint8_t) age;
    if (age > 0x78) {
        U8(st, f + FIG_STATE) = 2;
    }
    int32_t landed = GAME_CALL(st, FN_FIGURE_STEP_STRAIGHT, id, 4);
    int32_t t = GAME_CALL(st, FN_FIGURE_TARGET_ON_TILE, FIGB_CUR(st));
    if (t == 0) {
        if (landed != 0) {
            U8(st, FIGB_F(st) + FIG_STATE) = 2;
            GAME_CALL(st, FN_RETURN_ZERO, 0x0E);
        }
    } else {
        missile_damage(st, t);
        GAME_CALL(st, FN_RETURN_ZERO, 0x0F);
        U8(st, FIGB_F(st) + FIG_STATE) = 2;
    }
    f = FIGB_F(st);
    int32_t d = S8(st, f + FIG_DIRECTION) - 2 * S32(st, G_VIEW_ORIENTATION);
    S32(st, G_SPRITE_DIRECTION) = d;
    if (d < 0) {
        d += 0x10;
        S32(st, G_SPRITE_DIRECTION) = d;
    }
    U32(st, f + FIG_U04) = U32(st, G_IMAGE_GROUP_270) + 0x20u + (uint32_t) d;
}
