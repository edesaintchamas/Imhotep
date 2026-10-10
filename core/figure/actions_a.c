/* Imhotep - figure actions, group A (action table 0x005DC28C, figure types 0x01 .. 0x14, 0x69).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21). Every action works on the current figure
 * (0x00D056F8) and re-reads it after each call, as the original does. */
#include "figure/actions_a.h"

#include "building/building_update.h"

#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


static int32_t cur(game_state *st)
{
    return S32(st, G_CURRENT_FIGURE);
}

static uint32_t fig(game_state *st)
{
    return FIGURE_ADDR(cur(st));
}

/* @bin image base of a sprite group: the table at 0x00F55BB4, 8 bytes per group */
static int32_t image_group(game_state *st, uint32_t group)
{
    return S32(st, G_IMAGE_GROUPS + 8u * group);
}

/* the current figure's direction (+0x11) relative to the view, 0 .. 7 (0x00E94E00) */
static int32_t sprite_direction(game_state *st, uint32_t f)
{
    int32_t d = S8(st, f + FIG_DIRECTION) - S32(st, G_VIEW_ORIENTATION);
    S32(st, G_SPRITE_DIRECTION) = d;
    if (d < 0) {
        d += 8;
        S32(st, G_SPRITE_DIRECTION) = d;
    }
    return d;
}

/* frame +0x01 advances, 0 .. 11 */
static void advance_frame_12(game_state *st, uint32_t f)
{
    U8(st, f + FIG_ANIM_FRAME) += 1;
    if (U8(st, f + FIG_ANIM_FRAME) >= 0x0C) {
        U8(st, f + FIG_ANIM_FRAME) = 0;
    }
}

/* @bin the per-type table at 0x007090EC (0x38 bytes per figure type): its first dword */
static uint32_t type_info(game_state *st, uint32_t f)
{
    return U32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
}

/* @bin 0x00489CEA the figure's tile (+0x20) has flag 4 in 0x00AF598C and 0 in the byte grid
 * 0x00ADC36C: the actions then use other sprites */
static int on_alt_tile(game_state *st, uint32_t f)
{
    int32_t tile = S32(st, f + FIG_GRID_OFFSET);
    return (U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile) & 4) != 0 && U8(st, G_SPRITE + (uint32_t) tile) == 0;
}

void figure_action_01(game_state *st)
{
    /* @bin 0x00489CA0 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_TERRAIN_USAGE) = 0;
    S32(st, f + FIG_U180) = 0;
    int16_t bid = S16(st, f + FIG_TARGET_HOUSE);
    uint32_t info = type_info(st, f);
    int alt = on_alt_tile(st, f);
    uint32_t b = BUILDING_ADDR(bid);
    if (U8(st, b + BLD_STATE) != 1 || S16(st, b + BLD_IMMIGRANT) != id || U8(st, b + BLD_HOUSE_SIZE) == 0) {
        U8(st, f + FIG_STATE) = 2;
        return;
    }
    advance_frame_12(st, f);
    uint8_t action = U8(st, f + FIG_ACTION);
    if (action == 5) {
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
    } else if (action == 4) {
        figure_dying(st, id);
    } else if (action == 8) {
        U8(st, f + FIG_INVISIBLE) = 1;
        U8(st, f + FIG_ANIM_FRAME) = 0;
        S16(st, f + FIG_WAIT_TICKS) -= 1;
        if (S16(st, f + FIG_WAIT_TICKS) <= 0) {
            int32_t r = GAME_CALL(st, FN_BUILDING_ROAD_WITHIN, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE), 2);
            f = fig(st);
            if (r == 0) {
                U8(st, f + FIG_STATE) = 2;
            } else {
                U8(st, f + FIG_ACTION) = 9;
                S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
                S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
                S16(st, f + FIG_ROAM_LENGTH) = 0;
            }
        }
    } else if (action == 9) {
        U8(st, f + FIG_INVISIBLE) = 0;
        figure_move(st, id, (int32_t) info);
        id = cur(st);
        f = FIGURE_ADDR(id);
        uint8_t dir = U8(st, f + FIG_DIRECTION);
        if (dir == 9) {
            U8(st, f + FIG_ACTION) = 8;
            S16(st, f + FIG_WAIT_TICKS) = 1;
            figure_route_release(st, id);
        } else if (dir == 0x0A) {
            S16(st, b + BLD_IMMIGRANT) = 0;
            S16(st, b + BLD_DISTANCE) = 0;
            U8(st, f + FIG_STATE) = 2;
        } else if (dir == 8) {
            int16_t x = S16(st, b + BLD_X);
            int16_t y = S16(st, b + BLD_Y);
            U8(st, f + FIG_ACTION) = 0x0A;
            S16(st, f + FIG_DEST_X) = x;
            S16(st, f + FIG_DEST_Y) = y;
            figure_line_to(st, id, S16(st, f + FIG_CC_X), S16(st, f + FIG_CC_Y), S16(st, f + FIG_DEST_X) * 15, y * 15, 0);
            S16(st, fig(st) + FIG_ROAM_LENGTH) = 0;
        }
    } else if (action == 0x0A) {
        U8(st, f + FIG_U0C) = 1;
        U8(st, f + FIG_INVISIBLE) = 1;
        int32_t r = figure_step_straight(st, id, 1);
        if (r == 1) {
            f = fig(st);
            /* @bin 0x00489F70 the table 0x0070CE14 (0x58 bytes per building type) */
            int32_t full = S32(st, G_HOUSE_LEVEL_U1C + 0x58u * (uint32_t) S16(st, b + BLD_SUBTYPE));
            U8(st, f + FIG_STATE) = 2;
            if (U8(st, b + BLD_HOUSE_MERGED) != 0) {
                full <<= 2;
            }
            int32_t room = full - S16(st, b + BLD_POPULATION);
            if (room < 0) {
                room = 0;
            }
            if (room < U8(st, f + FIG_MIGRANT_PEOPLE)) {
                U8(st, f + FIG_MIGRANT_PEOPLE) = (uint8_t) room;
            }
            if (S16(st, b + BLD_POPULATION) == 0 && S32(st, G_U978D70) == 0) {
                GAME_CALL(st, FN_HOUSE_SET_TYPE, bid, 10);
            }
            if (U8(st, b + BLD_NO_ROOM) == 0) {
                uint8_t amount = U8(st, fig(st) + FIG_MIGRANT_PEOPLE);
                S16(st, b + BLD_POPULATION) += amount;
                S16(st, b + BLD_ROOM_LEFT) = (int16_t) (full - S16(st, b + BLD_POPULATION));
                GAME_CALL(st, FN_PEOPLE_CHANGE, (int32_t) U32(st, CURRENT_PLAYER_ADDR), amount);
            }
            S16(st, b + BLD_IMMIGRANT) = 0;
        }
        f = fig(st);
        U8(st, f + FIG_INVISIBLE) = U8(st, f + FIG_IN_BUILDING_WAIT) != 0 ? 1 : 0;
    } else {
        U8(st, f + FIG_ACTION) = 8;
    }
    f = fig(st);
    int32_t d = sprite_direction(st, f);
    if (U8(st, f + FIG_ACTION) == 4) {
        if (!alt) {
            S32(st, f + FIG_U04) = U8(st, G_DIE_FRAMES + (uint32_t) (S16(st, f + FIG_WAIT_TICKS) / 2)) + S32(st, G_IMAGE_GROUP_54);
            return;
        }
        if (S16(st, f + FIG_WAIT_TICKS) < 0x21) {
            uint8_t r = (uint8_t) figure_near_map_edge(st, S16(st, f + FIG_X));
            f = fig(st);
            int32_t base = r == 0 ? S32(st, G_IMAGE_GROUP_178) : S32(st, G_IMAGE_GROUP_706);
            S32(st, f + FIG_U04) = S32(st, G_SPRITE_DIRECTION) / 8 + base + S16(st, f + FIG_WAIT_TICKS) / 3;
            return;
        }
        int32_t v = S16(st, f + FIG_WAIT_TICKS) - 0x21;
        if (v >= 0x17) {
            v = 0x16;
        }
        S32(st, f + FIG_U04) = v + S32(st, G_IMAGE_GROUP_588);
        return;
    }
    if (!alt) {
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_53) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
        S32(st, f + FIG_U180) = 0;
        return;
    }
    uint8_t r = (uint8_t) figure_near_map_edge(st, S16(st, f + FIG_X));
    f = fig(st);
    int32_t base = r == 0 ? S32(st, G_IMAGE_GROUP_177) : S32(st, G_IMAGE_GROUP_705);
    S32(st, f + FIG_U04) = base + 8 * (U8(st, f + FIG_ANIM_FRAME) / 3) + S32(st, G_SPRITE_DIRECTION);
    S32(st, f + FIG_U180) = 0;
}

void figure_action_05(game_state *st)
{
    /* @bin 0x0048BFF0 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_TERRAIN_USAGE) = 1;
    U8(st, f + FIG_U0C) = 0;
    S16(st, f + FIG_MAX_ROAM_LENGTH) = 0x180;
    uint32_t b = BUILDING_ADDR(S16(st, f + FIG_HOME));
    if (U8(st, b + BLD_STATE) != 1 || S16(st, b + BLD_WALKER2) != id) {
        U8(st, f + FIG_STATE) = 2; /* its building is gone, or no longer links to it */
    }
    advance_frame_12(st, f);
    GAME_CALL(st, FN_FIGB_ROAMER_ACTION, 1);
    f = fig(st);
    int32_t d = sprite_direction(st, f);
    if (U8(st, f + FIG_ACTION) == 4) {
        /* @bin 0x0048C0A2 frame table 0x005D048C indexed by +0x36 / 2 (signed) */
        S32(st, f + FIG_U04) = U8(st, G_DIE_FRAMES + (uint32_t) (S16(st, f + FIG_WAIT_TICKS) / 2)) + S32(st, G_IMAGE_GROUP_246);
        return;
    }
    S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_245) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
}

void figure_action_06(game_state *st)
{
    /* @bin 0x0048C200 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    int32_t group, limit, div;
    U8(st, f + FIG_U0C) = 1;
    if (S32(st, f + FIG_U15C) == 0) {
        group = 0x246;
        limit = 0x0F;
        div = 3;
    } else {
        group = 0x24D;
        limit = 0x14;
        div = (id & 1) + 1;
    }
    U8(st, f + FIG_PROGRESS_ON_TILE) += 1;
    if (S8(st, f + FIG_PROGRESS_ON_TILE) >= div * limit) {
        U8(st, f + FIG_STATE) = 2;
    }
    figure_step_straight(st, id, U8(st, f + FIG_U59));
    f = fig(st);
    S32(st, f + FIG_U04) = S8(st, f + FIG_PROGRESS_ON_TILE) / div + image_group(st, (uint32_t) group);
}

void figure_action_02(game_state *st)
{
    /* @bin 0x0048A260 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_TERRAIN_USAGE) = 0;
    S32(st, f + FIG_U180) = 0;
    uint32_t info = type_info(st, f);
    int alt = on_alt_tile(st, f);
    advance_frame_12(st, f);
    uint8_t action = U8(st, f + FIG_ACTION);
    if (action == 5) {
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
    } else if (action == 4) {
        figure_dying(st, id);
    } else if (action == 8) {
        U8(st, f + FIG_INVISIBLE) = 1;
        U8(st, f + FIG_ANIM_FRAME) = 0;
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) >= 5) {
            int32_t r = GAME_CALL(st, FN_BUILDING_ROAD_WITHIN, S16(st, f + FIG_X), S16(st, f + FIG_Y), 1, 5);
            id = cur(st);
            f = FIGURE_ADDR(id);
            if (r == 0) {
                U8(st, f + FIG_STATE) = 2;
            }
            U8(st, f + FIG_ACTION) = 9;
            S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
            int16_t y = S16(st, G_ROAD_ACCESS_Y);
            S16(st, f + FIG_DEST_Y) = y;
            figure_line_to(st, id, S16(st, f + FIG_CC_X), S16(st, f + FIG_CC_Y), S16(st, f + FIG_DEST_X) * 15, y * 15, 0);
            S16(st, fig(st) + FIG_ROAM_LENGTH) = 0;
        }
    } else if (action == 9) {
        U8(st, f + FIG_U0C) = 1;
        U8(st, f + FIG_INVISIBLE) = 1;
        int32_t r = figure_step_straight(st, id, 1);
        f = fig(st);
        if (r == 1) {
            /* @bin 0x0048A43B a city point (city record +0x2778, +0x277A) */
            uint32_t c = CURRENT_CITY(st);
            int16_t x = S16(st, c + CITY_ENTRY_X);
            int16_t y = S16(st, c + CITY_ENTRY_Y);
            U8(st, f + FIG_ACTION) = 0x0A;
            S16(st, f + FIG_DEST_X) = x;
            S16(st, f + FIG_DEST_Y) = y;
            S16(st, f + FIG_ROAM_LENGTH) = 0;
            U8(st, f + FIG_PROGRESS_ON_TILE) = 0x0F;
        }
        U8(st, f + FIG_INVISIBLE) = U8(st, f + FIG_IN_BUILDING_WAIT) != 0 ? 1 : 0;
    } else if (action == 0x0A) {
        U8(st, f + FIG_U0C) = 0;
        U8(st, f + FIG_INVISIBLE) = 0;
        figure_move(st, id, (int32_t) info);
        f = fig(st);
        uint8_t dir = U8(st, f + FIG_DIRECTION);
        if (dir == 9 || dir == 0x0A || dir == 8) {
            U8(st, f + FIG_STATE) = 2;
        }
    } else {
        U8(st, f + FIG_ACTION) = 8;
    }
    f = fig(st);
    int32_t d = sprite_direction(st, f);
    if (U8(st, f + FIG_ACTION) == 4) {
        if (!alt) {
            S32(st, f + FIG_U04) = U8(st, G_DIE_FRAMES + (uint32_t) (S16(st, f + FIG_WAIT_TICKS) / 2)) + S32(st, G_IMAGE_GROUP_42);
            goto facing;
        }
        if (S16(st, f + FIG_WAIT_TICKS) < 0x21) {
            uint8_t r = (uint8_t) figure_near_map_edge(st, S16(st, f + FIG_X));
            f = fig(st);
            d = S32(st, G_SPRITE_DIRECTION);
            int32_t base = r == 0 ? S32(st, G_IMAGE_GROUP_178) : S32(st, G_IMAGE_GROUP_706);
            S32(st, f + FIG_U04) = d / 8 + base + S16(st, f + FIG_WAIT_TICKS) / 3;
        } else {
            int32_t v = S16(st, f + FIG_WAIT_TICKS) - 0x21;
            if (v >= 0x17) {
                v = 0x16;
            }
            S32(st, f + FIG_U04) = v + S32(st, G_IMAGE_GROUP_588);
        }
    } else {
        if (!alt) {
            S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_41) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
            goto facing;
        }
        uint8_t r = (uint8_t) figure_near_map_edge(st, S16(st, f + FIG_X));
        f = fig(st);
        int32_t base = r == 0 ? S32(st, G_IMAGE_GROUP_177) : S32(st, G_IMAGE_GROUP_705);
        d = S32(st, G_SPRITE_DIRECTION);
        S32(st, f + FIG_U04) = base + 8 * (U8(st, f + FIG_ANIM_FRAME) / 3) + d;
    }
    S32(st, f + FIG_U180) = 0;
facing:
    if (U8(st, f + FIG_ACTION) == 0x0A) {
        /* @bin 0x0048A6E3 the opposite direction (0x00BCB9A0) and its offsets (table 0x005DC23C) */
        int32_t o = d + 4;
        S32(st, G_OPPOSITE_DIRECTION) = o;
        if (o >= 8) {
            o -= 8;
            S32(st, G_OPPOSITE_DIRECTION) = o;
        }
        U8(st, f + FIG_U6C) = U8(st, G_CARRY_OFFSET_X + 2u * (uint32_t) o);
        U8(st, f + FIG_U6D) = U8(st, G_CARRY_OFFSET_Y + 2u * (uint32_t) o);
    }
}

/* the figure goes to the building found by 0x00458C30 (its +0x5C becomes it, the building's +0x2E
 * the figure), when 0x0047FE60 finds a road tile next to it; returns 0 when none was found */
static int32_t a03_head_to(game_state *st, int32_t bid)
{
    uint32_t b = BUILDING_ADDR(bid);
    int32_t r = GAME_CALL(st, FN_BUILDING_ROAD_WITHIN, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE), 2);
    if (r == 0) {
        return 0;
    }
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    S16(st, b + BLD_IMMIGRANT) = (int16_t) id;
    S16(st, f + FIG_TARGET_HOUSE) = (int16_t) bid;
    U8(st, f + FIG_ACTION) = 9;
    S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
    S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
    S16(st, f + FIG_ROAM_LENGTH) = 0;
    return 1;
}

void figure_action_03(game_state *st)
{
    /* @bin 0x0048A720 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    uint32_t info = type_info(st, f);
    int alt = on_alt_tile(st, f);
    advance_frame_12(st, f);
    U8(st, f + FIG_TERRAIN_USAGE) = 3;
    uint8_t action = U8(st, f + FIG_ACTION);
    if (action == 5) {
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
    } else if (action == 4) {
        figure_dying(st, id);
    } else if (action == 8) {
        U8(st, f + FIG_ANIM_FRAME) = 0;
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0x33) {
            int32_t bid = GAME_CALL(st, FN_BUILDING_NEAREST_0X458C30, S16(st, f + FIG_X), S16(st, f + FIG_Y));
            if (bid == 0) {
                /* @bin 0x0048A835 a city point (city record +0x2780, +0x2782) */
                f = fig(st);
                uint32_t c = CURRENT_CITY(st);
                int16_t x = S16(st, c + CITY_EXIT_X);
                int16_t y = S16(st, c + CITY_EXIT_Y);
                U8(st, f + FIG_ACTION) = 0x0B;
                S16(st, f + FIG_DEST_X) = x;
                S16(st, f + FIG_DEST_Y) = y;
                S16(st, f + FIG_ROAM_LENGTH) = 0;
                S16(st, f + FIG_WAIT_TICKS) = 0;
            } else if (!a03_head_to(st, bid)) {
                U8(st, fig(st) + FIG_STATE) = 2;
            }
        }
    } else if (action == 9) {
        U8(st, f + FIG_INVISIBLE) = 0;
        figure_move(st, id, (int32_t) info);
        id = cur(st);
        f = FIGURE_ADDR(id);
        uint8_t dir = U8(st, f + FIG_DIRECTION);
        int16_t bid = S16(st, f + FIG_TARGET_HOUSE);
        if (dir == 9 || dir == 0x0A) {
            U8(st, f + FIG_STATE) = 2;
            S16(st, BUILDING_ADDR(bid) + BLD_IMMIGRANT) = 0;
        } else if (dir == 8) {
            uint32_t b = BUILDING_ADDR(bid);
            U8(st, f + FIG_ACTION) = 0x0A;
            int16_t y = S16(st, b + BLD_Y);
            S16(st, f + FIG_DEST_X) = S16(st, b + BLD_X);
            S16(st, f + FIG_DEST_Y) = y;
            figure_line_to(st, id, S16(st, f + FIG_CC_X), S16(st, f + FIG_CC_Y), S16(st, f + FIG_DEST_X) * 15, y * 15, 0);
            S16(st, fig(st) + FIG_ROAM_LENGTH) = 0;
        }
    } else if (action == 0x0B) {
        figure_move(st, id, (int32_t) info);
        id = cur(st);
        f = FIGURE_ADDR(id);
        uint8_t dir = U8(st, f + FIG_DIRECTION);
        if (dir == 8 || dir == 0x0A) {
            U8(st, f + FIG_STATE) = 2;
        } else if (dir == 9) {
            figure_route_release(st, id);
        }
        f = fig(st);
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0x1E) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
            int32_t bid = GAME_CALL(st, FN_BUILDING_NEAREST_0X458C30, S16(st, f + FIG_X), S16(st, f + FIG_Y));
            if (bid > 0 && a03_head_to(st, bid)) {
                figure_route_release(st, cur(st));
            }
        }
    } else if (action == 0x0A) {
        int16_t bid = S16(st, f + FIG_TARGET_HOUSE);
        U8(st, f + FIG_U0C) = 1;
        U8(st, f + FIG_INVISIBLE) = 1;
        if (figure_step_straight(st, id, 1) == 1) {
            f = fig(st);
            U8(st, f + FIG_STATE) = 2;
            uint32_t b = BUILDING_ADDR(bid);
            /* @bin 0x0048AB6E building types 0x0A .. 0x1D take the load */
            if (bid != 0 && S16(st, b + BLD_TYPE) >= 0x0A && S16(st, b + BLD_TYPE) <= 0x1D && U8(st, b + BLD_NO_ROOM) == 0) {
                int32_t full = S32(st, G_HOUSE_LEVEL_U1C + 0x58u * (uint32_t) S16(st, b + BLD_SUBTYPE));
                if (U8(st, b + BLD_HOUSE_MERGED) != 0) {
                    full <<= 2;
                }
                int32_t room = full - S16(st, b + BLD_POPULATION);
                if (room < 0) {
                    room = 0;
                }
                if (room < U8(st, f + FIG_MIGRANT_PEOPLE)) {
                    U8(st, f + FIG_MIGRANT_PEOPLE) = (uint8_t) room;
                }
                if (S16(st, b + BLD_POPULATION) == 0) {
                    GAME_CALL(st, FN_HOUSE_SET_TYPE, bid, 10);
                }
                uint8_t amount = U8(st, fig(st) + FIG_MIGRANT_PEOPLE);
                S16(st, b + BLD_POPULATION) += amount;
                S16(st, b + BLD_ROOM_LEFT) = (int16_t) (full - S16(st, b + BLD_POPULATION));
                GAME_CALL(st, FN_CITY_POPULATION_REMOVE, (int32_t) U32(st, CURRENT_PLAYER_ADDR), amount);
                S16(st, b + BLD_IMMIGRANT) = 0;
            }
        }
    } else {
        U8(st, f + FIG_ACTION) = 8;
    }
    f = fig(st);
    int32_t d = sprite_direction(st, f);
    if (U8(st, f + FIG_ACTION) == 4) {
        S32(st, f + FIG_U04) = U8(st, G_DIE_FRAMES + (uint32_t) (S16(st, f + FIG_WAIT_TICKS) / 2)) + S32(st, G_IMAGE_GROUP_52);
        if (!alt) {
            return;
        }
        if (S16(st, f + FIG_WAIT_TICKS) < 0x21) {
            uint8_t r = (uint8_t) figure_near_map_edge(st, S16(st, f + FIG_X));
            f = fig(st);
            int32_t base = r == 0 ? S32(st, G_IMAGE_GROUP_178) : S32(st, G_IMAGE_GROUP_706);
            S32(st, f + FIG_U04) = S32(st, G_SPRITE_DIRECTION) / 8 + base + S16(st, f + FIG_WAIT_TICKS) / 3;
        } else {
            int32_t v = S16(st, f + FIG_WAIT_TICKS) - 0x21;
            if (v >= 0x17) {
                v = 0x16;
            }
            S32(st, f + FIG_U04) = v + S32(st, G_IMAGE_GROUP_588);
        }
        S32(st, f + FIG_U180) = 0;
        return;
    }
    if (!alt) {
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_51) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
        return;
    }
    uint8_t r = (uint8_t) figure_near_map_edge(st, S16(st, f + FIG_X));
    f = fig(st);
    int32_t base = r == 0 ? S32(st, G_IMAGE_GROUP_177) : S32(st, G_IMAGE_GROUP_705);
    S32(st, f + FIG_U04) = base + 8 * (U8(st, f + FIG_ANIM_FRAME) / 3) + S32(st, G_SPRITE_DIRECTION);
    S32(st, f + FIG_U180) = 0;
}

/* @bin 0x0048BEB0 .. 0x0048BF04 (jump tables 0x0048BFBC, 0x0048BFD0) goods carried with the
 * opposite direction's offsets */
static int goods_turned(uint8_t r)
{
    return r == 0x18 || r == 0x19 || r == 0x1A || r == 0x1E || r == 0x23 || r == 0x0C;
}

/* @bin 0x0048B7E4 (jump table 0x0048BF30, index 0x0048BF40) the destination building's type
 * (+0x10) -> the cart pusher's next action */
static uint8_t a04_delivery_action(int16_t type)
{
    switch (type) {
    case 0x47: case 0xBF: return 0x0A;
    case 0x48: case 0x49: case 0xC1: return 9;
    case 0x5F: return 0x10;
    default: return 0x0B;
    }
}

/* the end of action 9 / 0x0A (0x0048B968): the figure dies when its destination building
 * (+0x5E) is no longer in use */
static void a04_check_destination(game_state *st, uint32_t f)
{
    if (U8(st, BUILDING_ADDR(S16(st, f + FIG_DEST_BUILDING)) + BLD_STATE) != 1) {
        U8(st, f + FIG_STATE) = 2;
    }
}

/* after a call to 0x004B3250 (0x0048B922): back to action 8 unless the tile's 0x00EE0C50 is 2 */
static void a04_rerouted(game_state *st)
{
    figure_route_release(st, cur(st));
    uint32_t f = fig(st);
    if (U8(st, G_GRID_ROUTING + (uint32_t) S32(st, f + FIG_GRID_OFFSET)) != 2) {
        U8(st, f + FIG_ACTION) = 8;
    }
    S16(st, f + FIG_WAIT_TICKS) = 0;
}

/* @bin 0x0048B6A4 (type 4), 0x0048DA4E (type 9): the sprite of a cart pusher (directions 8 and up
 * use +0x12); +0x180 (when set) moves with the direction, +0x6C/+0x6D the cart's offsets */
static void cart_sprite(game_state *st, uint32_t walk_images, uint32_t death_images)
{
    uint32_t f = fig(st);
    int32_t d = S8(st, f + FIG_DIRECTION) >= 8 ? S8(st, f + FIG_PREV_DIRECTION) : S8(st, f + FIG_DIRECTION);
    d -= S32(st, G_VIEW_ORIENTATION);
    S32(st, G_SPRITE_DIRECTION) = d;
    if (d < 0) {
        d += 8;
        S32(st, G_SPRITE_DIRECTION) = d;
    }
    if (U8(st, f + FIG_ACTION) == 4) {
        S32(st, f + FIG_U04) = U8(st, G_DIE_FRAMES + (uint32_t) (S16(st, f + FIG_WAIT_TICKS) / 2)) + S32(st, death_images);
        S32(st, f + FIG_U180) = 0;
    } else if (U8(st, f + FIG_ACTION) != 6) {
        S32(st, f + FIG_U04) = S32(st, walk_images) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
    }
    if (S32(st, f + FIG_U180) == 0) {
        return;
    }
    S32(st, f + FIG_U180) += d;
    uint8_t good = U8(st, f + FIG_RESOURCE);
    int turned = goods_turned(good) || (good == 0 && goods_turned(U8(st, f + FIG_COLLECTING)));
    int32_t o = d;
    if (turned) {
        o = d + 4;
        if (o >= 8) {
            o -= 8;
        }
    }
    U8(st, f + FIG_U6C) = U8(st, G_CARRY_OFFSET_X + 2u * (uint32_t) o);
    U8(st, f + FIG_U6D) = U8(st, G_CARRY_OFFSET_Y + 2u * (uint32_t) o);
}

void figure_action_04(game_state *st)
{
    /* @bin 0x0048B620 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    uint32_t info = type_info(st, f);
    advance_frame_12(st, f);
    S32(st, f + FIG_U180) = 0;
    int32_t tile = S32(st, f + FIG_GRID_OFFSET);
    U8(st, f + FIG_TERRAIN_USAGE) = 1;
    uint8_t terrain = U8(st, G_GRID_ROAD_NETWORK + (uint32_t) tile); /* @bin 0x0048B681 a byte grid */
    uint8_t action = U8(st, f + FIG_ACTION);
    uint8_t dir;
    switch (action) {
    case 5:
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        break;
    case 4:
        figure_dying(st, id);
        break;
    case 8: {
        int16_t bid = S16(st, f + FIG_HOME);
        cart_sprite_load(st, id);
        int8_t road = S8(st, G_GRID_ROUTING + (uint32_t) tile);
        id = cur(st);
        if (road < 0 || road > 2) {
            U8(st, FIGURE_ADDR(id) + FIG_STATE) = 2;
        }
        uint32_t b = BUILDING_ADDR(bid);
        if (U8(st, b + BLD_STATE) != 1 || (S16(st, b + BLD_WALKER) != id && S16(st, b + BLD_SECOND_CART) != id)) {
            U8(st, FIGURE_ADDR(id) + FIG_STATE) = 2;
        }
        f = FIGURE_ADDR(id);
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) <= 0x1E) {
            U8(st, f + FIG_ANIM_FRAME) = 0;
            break;
        }
        S32(st, G_YARDS_LOW) = 0;
        /* @bin 0x0048B778 the good of the building (+0x43): city tables 0x00EA4096 (u16, 0x24EC per
         * player) and 0x00EA42F8 (u32, 0x1276 per player) */
        uint32_t good = U8(st, b + BLD_OUTPUT_RESOURCE);
        uint32_t player = U32(st, CURRENT_PLAYER_ADDR);
        int16_t mode = S16(st, CITY_BASE + CITY_GOOD_SETTING + 2u * (player * 0x24ECu + good));
        int held = mode == 2 || mode == 4;
        if (S32(st, CITY_BASE + CITY_U2A88 + 4u * (player * 0x1276u + good)) == 0 && !held) {
            figure_route_release(st, id);
        }
        int32_t dest = cart_find_destination(st, cur(st), terrain);
        f = fig(st);
        if (dest == 0) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
            U8(st, f + FIG_SERVICE_MAX) = S32(st, G_YARDS_LOW) != 0 ? 2 : 1;
            U8(st, f + FIG_ANIM_FRAME) = 0;
            break;
        }
        U8(st, f + FIG_ACTION) = a04_delivery_action(S16(st, BUILDING_ADDR(dest) + BLD_TYPE));
        S16(st, f + FIG_WAIT_TICKS) = 0;
        S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
        S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
        U8(st, f + FIG_ANIM_FRAME) = 0;
        break;
    }
    case 9:
    case 0x0A:
        cart_sprite_load(st, id);
        figure_move(st, cur(st), (int32_t) info);
        f = fig(st);
        dir = U8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            U8(st, f + FIG_ACTION) = action == 9 ? 0x0C : 0x0D;
        } else if (dir == 9) {
            a04_rerouted(st);
            f = fig(st);
        } else if (dir == 0x0A) {
            if (action == 9) {
                U8(st, f + FIG_STATE) = 2;
            } else {
                U8(st, f + FIG_ACTION) = 8;
                S16(st, f + FIG_WAIT_TICKS) = 0;
            }
        }
        a04_check_destination(st, f);
        break;
    case 0x0B:
    case 0x10:
        cart_sprite_load(st, id);
        figure_move(st, cur(st), (int32_t) info);
        f = fig(st);
        dir = U8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            U8(st, f + FIG_ACTION) = action == 0x0B ? 0x0E : 0x11;
        } else if (dir == 9) {
            a04_rerouted(st);
        } else if (dir == 0x0A) {
            U8(st, f + FIG_STATE) = 2;
        }
        break;
    case 0x0C:
    case 0x0D:
    case 0x0E:
    case 0x11: {
        cart_sprite_load(st, id);
        f = fig(st);
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > (action == 0x0C ? 0x0A : 5)) {
            int32_t r;
            if (action == 0x0C) {
                GAME_CALL(st, FN_BUILDING_MAIN_PART, S16(st, f + FIG_DEST_BUILDING));
                f = fig(st);
                r = GAME_CALL(st, FN_STORAGE_ADD, U8(st, f + FIG_OWNER), S16(st, f + FIG_DEST_BUILDING), U8(st, f + FIG_RESOURCE), S16(st, f + FIG_AMOUNT));
            } else if (action == 0x0D) {
                r = GAME_CALL(st, FN_BUILDING_47_ADD, S16(st, f + FIG_DEST_BUILDING), U8(st, f + FIG_RESOURCE), 1, S16(st, f + FIG_AMOUNT));
            } else if (action == 0x0E) {
                r = GAME_CALL(st, FN_BUILDING_ADD_GOOD, S16(st, f + FIG_DEST_BUILDING), S16(st, f + FIG_AMOUNT), U8(st, f + FIG_RESOURCE));
            } else {
                r = GAME_CALL(st, FN_BUILDING_FILL, S16(st, f + FIG_DEST_BUILDING), S16(st, f + FIG_AMOUNT), U8(st, f + FIG_RESOURCE));
            }
            GAME_CALL(st, FN_CART_UNLOADED, r, cur(st), terrain);
        }
        U8(st, fig(st) + FIG_ANIM_FRAME) = 0;
        break;
    }
    case 0x0F: {
        int16_t bid = S16(st, f + FIG_HOME);
        /* @bin 0x0048BCAE thiscall on 0x00BCA350; game mode 0x00D386D4 1 .. 3 */
        if ((uint8_t) GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_IS_TYPE_0X487F80, bid) != 0 &&
            (uint8_t) GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_AREA_TEST_3X3, bid) != 0 &&
            (S32(st, G_FLOOD_STATE) == 1 || S32(st, G_FLOOD_STATE) == 2 || S32(st, G_FLOOD_STATE) == 3)) {
            figure_route_release(st, cur(st));
            U8(st, fig(st) + FIG_STATE) = 2;
            return;
        }
        cart_sprite_load(st, cur(st));
        figure_move(st, cur(st), (int32_t) info);
        f = fig(st);
        dir = U8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            U8(st, f + FIG_ACTION) = 8;
            U8(st, f + FIG_STATE) = 2;
        } else if (dir == 9) {
            figure_route_release(st, cur(st));
        } else if (dir == 0x0A) {
            U8(st, f + FIG_STATE) = 2;
        }
        break;
    }
    case 7:
        U8(st, f + FIG_ANIM_FRAME) = 0;
        cart_sprite_load(st, id);
        f = fig(st);
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_82) + S32(st, G_SPRITE_DIRECTION) + 1;
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0x64) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
            if ((uint8_t) GAME_CALL(st, FN_FIGURE_MAY_LEAVE, cur(st)) == 0) {
                U8(st, fig(st) + FIG_ACTION) = 8;
            }
        }
        break;
    case 6:
        S32(st, f + FIG_U04) = 0;
        if (S32(st, f + FIG_U180) != 0) {
            S32(st, f + FIG_U180) = 0;
        }
        break;
    default:
        U8(st, f + FIG_ACTION) = 8;
        break;
    }
    cart_sprite(st, G_IMAGE_GROUP_82, G_IMAGE_GROUP_83);
}

/* the figure walks to (x, y): +0x24/+0x26 the target, 0x004B4220 the path from +0x4A/+0x4C
 * (tiles * 15) */
static void walk_to_point(game_state *st, int16_t x, int16_t y, uint8_t next_action)
{
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_ACTION) = next_action;
    S16(st, f + FIG_DEST_X) = x;
    S16(st, f + FIG_DEST_Y) = y;
    figure_line_to(st, id, S16(st, f + FIG_CC_X), S16(st, f + FIG_CC_Y), S16(st, f + FIG_DEST_X) * 15, y * 15, 0);
    S16(st, fig(st) + FIG_ROAM_LENGTH) = 0;
}

/* the figure walks back to the road tile next to building b */
static void walk_to_tile(game_state *st, uint32_t b, uint8_t next_action)
{
    walk_to_point(st, S16(st, b + BLD_X), S16(st, b + BLD_Y), next_action);
}

/* the same towards the tile 0x0047FE60 left in 0x00D3B4E4 / 0x00D3B4E0 */
static void walk_to_found_tile(game_state *st, uint8_t next_action)
{
    walk_to_point(st, S16(st, G_ROAD_ACCESS_X), S16(st, G_ROAD_ACCESS_Y), next_action);
}

/* 0x0047FE60 on building b's position, size (+0x03) and 2 */
static int32_t road_next_to(game_state *st, uint32_t b)
{
    return GAME_CALL(st, FN_BUILDING_ROAD_WITHIN, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE), 2);
}

/* @bin 0x0048C2B0 (type 7) and 0x0048C6A0 (type 8): the same code but for the constants. +0x42 the
 * time out; type 8 walks out to its building's +0x26/+0x28 rather than the road tile found, and
 * resets +0x36 on an unknown action. */
typedef struct {
    int16_t time_out;
    int to_building_point;
    uint32_t walk_images, death_images;
} a07_kind;

static void a07_walker(game_state *st, const a07_kind *k)
{
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_TERRAIN_USAGE) = 1;
    U8(st, f + FIG_U0C) = 0;
    S16(st, f + FIG_MAX_ROAM_LENGTH) = k->time_out;
    int16_t bid = S16(st, f + FIG_HOME);
    uint32_t info = type_info(st, f);
    uint32_t b = BUILDING_ADDR(bid);
    if (U8(st, b + BLD_STATE) != 1 || S16(st, b + BLD_WALKER) != id) {
        U8(st, f + FIG_STATE) = 2;
    }
    advance_frame_12(st, f);
    uint8_t action = U8(st, f + FIG_ACTION);
    if (action == 5) {
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
    } else if (action == 4) {
        figure_dying(st, id);
    } else if (action == 8) {
        U8(st, f + FIG_INVISIBLE) = 1;
        U8(st, f + FIG_ANIM_FRAME) = 0;
        S16(st, f + FIG_WAIT_TICKS) -= 1;
        if (S16(st, f + FIG_WAIT_TICKS) <= 0) {
            if (road_next_to(st, b) == 0) {
                U8(st, fig(st) + FIG_STATE) = 2;
            } else if (k->to_building_point) {
                walk_to_point(st, S16(st, b + BLD_ROAD_ACCESS_X), S16(st, b + BLD_ROAD_ACCESS_Y), 9);
            } else {
                walk_to_found_tile(st, 9);
            }
        }
    } else if (action == 9) {
        U8(st, f + FIG_U0C) = 1;
        U8(st, f + FIG_INVISIBLE) = 1;
        if (figure_step_straight(st, id, 1) == 1) {
            id = cur(st);
            f = FIGURE_ADDR(id);
            /* @bin 0x0048C4E3 the building grid 0x00B4E6FC at the figure's tile */
            if (S16(st, G_GRID_BUILDING + 2u * (uint32_t) S32(st, f + FIG_GRID_OFFSET)) == bid) {
                U8(st, f + FIG_STATE) = 2;
            } else {
                U8(st, f + FIG_ACTION) = 0x0A;
                GAME_CALL(st, FN_FIGURE_WALKER_START, id);
                S16(st, fig(st) + FIG_ROAM_LENGTH) = 0;
            }
        }
    } else if (action == 0x0A) {
        U8(st, f + FIG_INVISIBLE) = 0;
        S16(st, f + FIG_ROAM_LENGTH) += 1;
        if (S16(st, f + FIG_ROAM_LENGTH) >= S16(st, f + FIG_MAX_ROAM_LENGTH)) {
            int32_t r = road_next_to(st, b);
            id = cur(st);
            f = FIGURE_ADDR(id);
            if (r == 0) {
                U8(st, f + FIG_STATE) = 2;
                figure_roam(st, id, 1);
                goto sprite;
            }
            U8(st, f + FIG_ACTION) = 0x0B;
            S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
            S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
        }
        figure_roam(st, id, 1);
    } else if (action == 0x0B) {
        figure_move(st, id, (int32_t) info);
        f = fig(st);
        uint8_t dir = U8(st, f + FIG_DIRECTION);
        if (dir == 9 || dir == 0x0A) {
            U8(st, f + FIG_STATE) = 2;
        } else if (dir == 8) {
            walk_to_tile(st, b, 9);
        }
    } else {
        U8(st, f + FIG_ACTION) = 8;
        if (k->to_building_point) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
        }
    }
sprite:
    f = fig(st);
    int32_t d = sprite_direction(st, f);
    if (U8(st, f + FIG_ACTION) == 4) {
        S32(st, f + FIG_U04) = U8(st, G_DIE_FRAMES + (uint32_t) (S16(st, f + FIG_WAIT_TICKS) / 2)) + S32(st, k->death_images);
        return;
    }
    S32(st, f + FIG_U04) = S32(st, k->walk_images) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
}

void figure_action_07(game_state *st)
{
    static const a07_kind k = {0x200, 0, G_IMAGE_GROUP_80, G_IMAGE_GROUP_81};
    a07_walker(st, &k); /* @bin 0x0048C2B0 */
}

void figure_action_08(game_state *st)
{
    static const a07_kind k = {0x280, 1, G_IMAGE_GROUP_43, G_IMAGE_GROUP_44};
    a07_walker(st, &k); /* @bin 0x0048C6A0 */
}

/* @bin 0x0048CF53 0x0045B880 (building_update.c) on the action's locals: ratios start at 100 */
static void a09_lowest_goods(game_state *st, int32_t bid, int32_t *good1, int32_t *good2)
{
    int32_t ratio1 = 100, ratio2 = 100, dest1 = 0, dest2 = 0;
    building_lowest_goods(st, bid, good1, good2, &ratio1, &ratio2, &dest1, &dest2);
}

/* @bin 0x0048D8B9 (jump table 0x0048DE74), 0x0048DAA7: the load a cart takes of a good */
static int32_t a09_load_amount(uint8_t good)
{
    return goods_turned(good) || good == 0x1C || good == 0x0A ? 100 : 400;
}

/* the end of the walking actions (0x0048DA36): arrived (+0x11 9) the route is released, lost (10)
 * the figure dies */
static void a09_walk_end(game_state *st, uint8_t dir)
{
    if (dir == 9) {
        figure_route_release(st, cur(st));
    } else if (dir == 0x0A) {
        U8(st, fig(st) + FIG_STATE) = 2;
    }
}

/* walk on (0x0048B4A0, 0x004A5DA0); returns the figure's +0x11 */
static uint8_t a09_walk(game_state *st, uint32_t info)
{
    cart_sprite_load(st, cur(st));
    figure_move(st, cur(st), (int32_t) info);
    return U8(st, fig(st) + FIG_DIRECTION);
}

/* 0x0048DB78: back to the figure's origin (+0x2C/+0x2E), the route released */
static void a09_return(game_state *st)
{
    uint32_t f = fig(st);
    S16(st, f + FIG_DEST_X) = S16(st, f + FIG_SOURCE_X);
    S16(st, f + FIG_DEST_Y) = S16(st, f + FIG_SOURCE_Y);
    figure_route_release(st, cur(st));
}

/* actions 0x13, 0x15, 0x10: the load (+0x74) taken by `taken`; none: back to action 8 */
static void a09_loaded(game_state *st, int16_t load, uint8_t next_action)
{
    uint32_t f = fig(st);
    S16(st, f + FIG_AMOUNT) = load;
    if (S16(st, f + FIG_AMOUNT) == 0) {
        U8(st, f + FIG_RESOURCE) = 0;
        U8(st, f + FIG_ACTION) = 8;
        figure_route_release(st, cur(st));
        return;
    }
    U8(st, f + FIG_RESOURCE) = U8(st, f + FIG_COLLECTING);
    U8(st, f + FIG_ACTION) = next_action;
    S16(st, f + FIG_WAIT_TICKS) = 0;
    a09_return(st);
}

/* the action 8 search ends with a destination (0x0048D2F7) */
static void a09_go(game_state *st, int32_t dest, uint8_t next_action)
{
    uint32_t f = fig(st);
    S16(st, f + FIG_DEST_BUILDING) = (int16_t) dest;
    U8(st, f + FIG_ACTION) = next_action;
    S16(st, f + FIG_WAIT_TICKS) = 0;
    S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
    S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
}

void figure_action_09(game_state *st)
{
    /* @bin 0x0048CE30 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_TERRAIN_USAGE) = 1;
    uint32_t info = type_info(st, f);
    advance_frame_12(st, f);
    S32(st, f + FIG_U180) = 0;
    uint8_t terrain = U8(st, G_GRID_ROAD_NETWORK + (uint32_t) S32(st, f + FIG_GRID_OFFSET));
    uint8_t action = U8(st, f + FIG_ACTION);
    uint8_t dir;
    int32_t r;
    switch (action) {
    case 5:
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        break;
    case 4:
        figure_dying(st, id);
        break;
    case 8: {
        int16_t bid = S16(st, f + FIG_HOME);
        uint32_t b = BUILDING_ADDR(bid);
        if (U8(st, b + BLD_STATE) != 1 ||
            (S16(st, b + BLD_WALKER) != id && S16(st, b + BLD_WALKER2) != id && S16(st, b + BLD_SECOND_CART) != id)) {
            U8(st, f + FIG_STATE) = 2;
        }
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) <= 2) {
            goto search_end;
        }
        int16_t btype = S16(st, b + BLD_TYPE);
        uint8_t good = U8(st, f + FIG_RESOURCE);
        if (btype == 0x47 || btype == 0xBF) {
            if (good == 0) {
                r = cart_storage_to_empty(st, bid);
                if (r != 0) {
                    a09_go(st, r, 0x12);
                    goto search_end;
                }
                r = GAME_CALL(st, FN_BUILDING_FIND_SUPPLIER, bid, U8(st, fig(st) + FIG_COLLECTING), 0);
                if (r != 0) {
                    a09_go(st, r, S16(st, BUILDING_ADDR(r) + BLD_TYPE) == 0x48 ? 0x14 : 0x12);
                    goto search_end;
                }
                U8(st, fig(st) + FIG_ACTION) = 8;
                cart_sprite_load(st, cur(st));
                goto search_end;
            }
            if ((r = cart_storage_with_room(st, 0, id, good, terrain)) != 0 ||
                (r = cart_dock_for_good(st, 0, cur(st), U8(st, fig(st) + FIG_RESOURCE), terrain)) != 0 ||
                (r = cart_storage_with_room(st, 1, cur(st), U8(st, fig(st) + FIG_RESOURCE), terrain)) != 0) {
                a09_go(st, r, 9);
                goto search_end;
            }
            cart_sprite_load(st, cur(st));
            f = fig(st);
            /* @bin 0x0048D338 table 0x007900C2 (0xC4 bytes per building +0xC1, by good) */
            uint8_t v = U8(st, G_STORAGE_ORDERS + 0xC4u * U8(st, b + BLD_STORAGE_ID) + U8(st, f + FIG_RESOURCE));
            if (v == 3 || v == 2) {
                U8(st, f + FIG_ACTION) = 8;
            } else {
                S16(st, f + FIG_DEST_BUILDING) = bid;
                U8(st, f + FIG_ACTION) = 9;
                S16(st, f + FIG_WAIT_TICKS) = 0;
                S16(st, f + FIG_DEST_X) = S16(st, b + BLD_X);
                S16(st, f + FIG_DEST_Y) = S16(st, b + BLD_Y);
            }
            goto search_end;
        }
        if (good == 0) {
            int32_t good1 = 0, good2 = 0;
            a09_lowest_goods(st, bid, &good1, &good2);
            int32_t want = S16(st, b + BLD_SECOND_CART) == cur(st) && good2 != 0 ? good2 : good1;
            r = GAME_CALL(st, FN_BUILDING_FIND_SUPPLIER, bid, want, 0);
            if (r == 0) {
                U8(st, fig(st) + FIG_STATE) = 2;
                return;
            }
            f = fig(st);
            U8(st, f + FIG_COLLECTING) = (uint8_t) want;
            S16(st, f + FIG_AMOUNT) = 0;
            S16(st, f + FIG_DEST_BUILDING) = (int16_t) r;
            U8(st, f + FIG_ACTION) = S16(st, BUILDING_ADDR(r) + BLD_TYPE) == 0x48 ? 0x0F : 0x12;
            S16(st, f + FIG_WAIT_TICKS) = 0;
            S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
            S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
            U8(st, f + FIG_TERRAIN_USAGE) = 3;
            goto search_end;
        }
        if ((r = GAME_CALL(st, FN_CART_CITY_BUILDING, S16(st, f + FIG_X), S16(st, f + FIG_Y), good, terrain)) != 0 ||
            (r = cart_workshop_with_room(st, cur(st), U8(st, fig(st) + FIG_RESOURCE), terrain)) != 0 ||
            (r = cart_storage_with_room(st, 0, cur(st), U8(st, fig(st) + FIG_RESOURCE), terrain)) != 0 ||
            (r = cart_storage_for_good(st, cur(st), U8(st, fig(st) + FIG_RESOURCE), terrain)) != 0 ||
            (r = cart_dock_for_good(st, bid, cur(st), U8(st, fig(st) + FIG_RESOURCE), terrain)) != 0 ||
            (r = cart_workshop_for_good(st, cur(st), U8(st, fig(st) + FIG_RESOURCE), terrain)) != 0) {
            a09_go(st, r, 9);
            goto search_end;
        }
        cart_sprite_load(st, cur(st));
        U8(st, fig(st) + FIG_ACTION) = 8; /* @bin 0x0048D164 whatever table 0x007900C2 says */
    search_end:
        U8(st, fig(st) + FIG_ANIM_FRAME) = 0;
        cart_sprite_load(st, cur(st));
        break;
    }
    case 9:
    case 0x0C:
    case 0x12:
    case 0x14:
    case 0x0F:
        if (action == 0x0F) {
            U8(st, f + FIG_TERRAIN_USAGE) = 3;
        }
        dir = a09_walk(st, info);
        if (dir == 8) {
            /* @bin 0x0048D404 .. 0x0048DA2D the action after arriving */
            U8(st, fig(st) + FIG_ACTION) = action == 9 ? 0x0A : action == 0x0C ? 0x0D : action == 0x12 ? 0x13
                                     : action == 0x14 ? 0x15 : 0x10;
        } else {
            a09_walk_end(st, dir);
        }
        break;
    case 0x0B:
        dir = a09_walk(st, info);
        if (dir == 8) {
            U8(st, fig(st) + FIG_STATE) = 2;
        } else {
            a09_walk_end(st, dir);
        }
        break;
    case 0x0A: {
        int16_t dest = S16(st, f + FIG_DEST_BUILDING);
        GAME_CALL(st, FN_BUILDING_MAIN_PART, dest);
        cart_sprite_load(st, cur(st));
        f = fig(st);
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 4) {
            cart_sprite_load(st, cur(st));
            f = fig(st);
            /* @bin 0x0048D48E (jump table 0x0048DDE8) by the destination's type */
            switch (S16(st, BUILDING_ADDR(dest) + BLD_TYPE)) {
            case 0x47: case 0xBF:
                r = GAME_CALL(st, FN_BUILDING_47_ADD, dest, U8(st, f + FIG_RESOURCE), 0, S16(st, f + FIG_AMOUNT));
                break;
            case 0x48: case 0x49: case 0xC1:
                r = GAME_CALL(st, FN_STORAGE_ADD, (int32_t) U32(st, CURRENT_PLAYER_ADDR), dest, U8(st, f + FIG_RESOURCE),
                              S16(st, f + FIG_AMOUNT));
                break;
            case 0x5F:
                r = GAME_CALL(st, FN_BUILDING_FILL, dest, S16(st, f + FIG_AMOUNT), U8(st, f + FIG_RESOURCE));
                break;
            default:
                r = GAME_CALL(st, FN_BUILDING_ADD_GOOD, dest, S16(st, f + FIG_AMOUNT), U8(st, f + FIG_RESOURCE));
                break;
            }
            cart_delivered(st, r, cur(st), terrain, 0);
        }
        U8(st, fig(st) + FIG_ANIM_FRAME) = 0;
        break;
    }
    case 0x0D: {
        int16_t bid = S16(st, f + FIG_HOME);
        int16_t dest = S16(st, f + FIG_DEST_BUILDING);
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 4) {
            int32_t load = cart_take_from_storage(st, bid, dest);
            S16(st, f + FIG_AMOUNT) = (int16_t) load;
            U8(st, f + FIG_RESOURCE) = U8(st, G_GOOD_CHOSEN); /* @bin 0x0048D67C the good 0x0045E150 chose */
            U8(st, f + FIG_ACTION) = 0x0E;
            S16(st, f + FIG_WAIT_TICKS) = 0;
            S16(st, f + FIG_DEST_X) = S16(st, f + FIG_SOURCE_X);
            S16(st, f + FIG_DEST_Y) = S16(st, f + FIG_SOURCE_Y);
            figure_route_release(st, cur(st));
        }
        U8(st, fig(st) + FIG_ANIM_FRAME) = 0;
        break;
    }
    case 0x13: {
        cart_sprite_load(st, id);
        f = fig(st);
        int16_t bid = S16(st, f + FIG_HOME);
        int16_t dest = S16(st, f + FIG_DEST_BUILDING);
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 4) {
            r = GAME_CALL(st, FN_STORAGE_TAKE_STOCK, (int32_t) U32(st, CURRENT_PLAYER_ADDR), dest, U8(st, f + FIG_COLLECTING), 400);
            int16_t load = r == 400 ? 0 : r == 0 ? 400 : (int16_t) r;
            a09_loaded(st, load, S16(st, BUILDING_ADDR(bid) + BLD_TYPE) == 0x47 ? 0x0E : 0x11);
        }
        U8(st, fig(st) + FIG_ANIM_FRAME) = 0;
        break;
    }
    case 0x15:
    case 0x10: {
        if (action == 0x10) {
            U8(st, f + FIG_TERRAIN_USAGE) = 3;
        }
        int16_t dest = S16(st, f + FIG_DEST_BUILDING);
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 4) {
            if (action == 0x10) {
                S16(st, f + FIG_AMOUNT) = 0;
            }
            uint8_t good = U8(st, f + FIG_COLLECTING);
            int32_t amount = a09_load_amount(good);
            r = GAME_CALL(st, FN_STORAGE_TAKE, (int32_t) U32(st, CURRENT_PLAYER_ADDR), dest, good, amount);
            int16_t load = (int16_t) (r == amount ? 0 : r == 0 ? amount : amount - r);
            a09_loaded(st, load, action == 0x15 ? 0x0E : 0x11);
        }
        U8(st, fig(st) + FIG_ANIM_FRAME) = 0;
        break;
    }
    case 0x0E:
    case 0x11: {
        if (action == 0x11) {
            U8(st, f + FIG_TERRAIN_USAGE) = 3;
        }
        int16_t bid = S16(st, f + FIG_HOME);
        dir = a09_walk(st, info);
        if (dir != 8) {
            a09_walk_end(st, dir);
            break;
        }
        f = fig(st);
        if (action == 0x0E) {
            r = GAME_CALL(st, FN_BUILDING_47_ADD, bid, U8(st, f + FIG_RESOURCE), 0, S16(st, f + FIG_AMOUNT));
        } else {
            r = GAME_CALL(st, FN_STORAGE_ADD, (int32_t) U32(st, CURRENT_PLAYER_ADDR), bid, U8(st, f + FIG_RESOURCE),
                          S16(st, f + FIG_AMOUNT));
        }
        cart_delivered(st, r, cur(st), terrain, 1);
        break;
    }
    case 7:
        U8(st, f + FIG_ANIM_FRAME) = 0;
        cart_sprite_load(st, id);
        f = fig(st);
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_82) + S32(st, G_SPRITE_DIRECTION) + 1;
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0x64) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
            if ((uint8_t) GAME_CALL(st, FN_FIGURE_MAY_LEAVE, cur(st)) == 0) {
                U8(st, fig(st) + FIG_ACTION) = 8;
            }
        }
        break;
    case 6:
        S32(st, f + FIG_U04) = 0;
        if (S32(st, f + FIG_U180) != 0) {
            S32(st, f + FIG_U180) = 0;
        }
        break;
    default:
        U8(st, f + FIG_ACTION) = 8;
        break;
    }
    cart_sprite(st, G_IMAGE_GROUP_82, G_IMAGE_GROUP_83);
}

/* |a - b| as the original computes it (0x0048E315) */
static int32_t distance_1d(int32_t a, int32_t b)
{
    return a > b ? a - b : a < b ? b - a : 0;
}

void figure_action_0a(game_state *st)
{
    /* @bin 0x0048DED0 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_TERRAIN_USAGE) = 1;
    U8(st, f + FIG_U0C) = 0;
    S16(st, f + FIG_MAX_ROAM_LENGTH) = 0x280;
    int16_t bid = S16(st, f + FIG_HOME);
    uint32_t info = type_info(st, f);
    uint32_t b = BUILDING_ADDR(bid);
    if (U8(st, b + BLD_STATE) != 1 || S16(st, b + BLD_WALKER) != id) {
        U8(st, f + FIG_STATE) = 2;
    }
    advance_frame_12(st, f);
    figure_0a_divert(st, id);
    id = cur(st);
    f = FIGURE_ADDR(id);
    uint8_t action = U8(st, f + FIG_ACTION);
    uint8_t dir;
    switch (action) {
    case 5:
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        break;
    case 4:
        figure_dying(st, id);
        break;
    case 8:
        U8(st, f + FIG_INVISIBLE) = 1;
        U8(st, f + FIG_ANIM_FRAME) = 0;
        S16(st, f + FIG_WAIT_TICKS) -= 1;
        if (S16(st, f + FIG_WAIT_TICKS) <= 0) {
            if (road_next_to(st, b) == 0) {
                U8(st, fig(st) + FIG_STATE) = 2;
            } else {
                walk_to_point(st, S16(st, b + BLD_ROAD_ACCESS_X), S16(st, b + BLD_ROAD_ACCESS_Y), 9);
            }
        }
        break;
    case 9:
        U8(st, f + FIG_U0C) = 1;
        U8(st, f + FIG_INVISIBLE) = 1;
        if (figure_step_straight(st, id, 1) == 1) {
            id = cur(st);
            f = FIGURE_ADDR(id);
            if (S16(st, G_GRID_BUILDING + 2u * (uint32_t) S32(st, f + FIG_GRID_OFFSET)) == bid) {
                U8(st, f + FIG_STATE) = 2;
            } else {
                U8(st, f + FIG_ACTION) = 0x0A;
                GAME_CALL(st, FN_FIGURE_WALKER_START, id);
                S16(st, fig(st) + FIG_ROAM_LENGTH) = 0;
            }
        }
        break;
    case 0x0A:
        U8(st, f + FIG_INVISIBLE) = 0;
        S16(st, f + FIG_ROAM_LENGTH) += 1;
        if (S16(st, f + FIG_ROAM_LENGTH) >= S16(st, f + FIG_MAX_ROAM_LENGTH)) {
            int32_t r = road_next_to(st, b);
            id = cur(st);
            f = FIGURE_ADDR(id);
            if (r == 0) {
                U8(st, f + FIG_STATE) = 2;
                figure_roam(st, id, 1);
                break;
            }
            U8(st, f + FIG_ACTION) = 0x0B;
            S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
            S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
            figure_route_release(st, id);
        }
        figure_roam(st, cur(st), 1);
        break;
    case 0x0B:
    case 0x0C:
        if (action == 0x0C) {
            U8(st, f + FIG_TERRAIN_USAGE) = 0;
        }
        figure_move(st, id, (int32_t) info);
        id = cur(st);
        f = FIGURE_ADDR(id);
        dir = U8(st, f + FIG_DIRECTION);
        if (dir == 9 || dir == 0x0A) {
            U8(st, f + FIG_STATE) = 2;
        } else if (dir == 8) {
            if (action == 0x0B) {
                walk_to_tile(st, b, 9);
            } else {
                U8(st, f + FIG_ACTION) = 0x0D;
                figure_route_release(st, id);
                f = fig(st);
                S16(st, f + FIG_ROAM_LENGTH) = 0;
                S16(st, f + FIG_WAIT_TICKS) = 0x32;
                S16(st, f + FIG_AMOUNT) = 0;
            }
        }
        break;
    case 0x0D: {
        int16_t dest = S16(st, f + FIG_DEST_BUILDING);
        uint32_t d = BUILDING_ADDR(dest);
        /* @bin 0x0048E2F1 the distances to the destination, kept in 0x00D3B4FC / 0x00D3B4F8 */
        int32_t dx = distance_1d(S16(st, f + FIG_X), S16(st, d + BLD_X));
        S32(st, G_DIST_X) = dx;
        int32_t dy = distance_1d(S16(st, f + FIG_Y), S16(st, d + BLD_Y));
        S32(st, G_DIST_Y) = dy;
        int32_t dist = dx < dy ? dy : dx;
        if (U8(st, d) != 1 || S16(st, d + BLD_TYPE) != 0x63 || dist >= 2) {
            S16(st, f + FIG_WAIT_TICKS) = 1;
        } else {
            /* @bin 0x0048E385 next to a building of type 0x63 */
            S16(st, d + BLD_BURNING) = 0x20;
            GAME_CALL(st, FN_RETURN_ZERO, 0x9D);
        }
        f = fig(st);
        uint8_t r = (uint8_t) GAME_CALL(st, FN_ROUTE_DIRECTION, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, d + BLD_X),
                                        S16(st, d + BLD_Y));
        f = fig(st);
        U8(st, f + FIG_ATTACK_DIRECTION) = r;
        if ((int8_t) r >= 8) {
            U8(st, f + FIG_ATTACK_DIRECTION) = 0;
        }
        S16(st, f + FIG_WAIT_TICKS) -= 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0 || S16(st, f + FIG_AMOUNT) < 0x24) {
            break;
        }
        U8(st, f + FIG_U6B) = 0x14;
        if (figure_0a_divert(st, cur(st)) != 0) {
            break;
        }
        if (road_next_to(st, BUILDING_ADDR(S16(st, fig(st) + FIG_HOME))) == 0) {
            U8(st, fig(st) + FIG_STATE) = 2;
            break;
        }
        f = fig(st);
        U8(st, f + FIG_ACTION) = 0x0B;
        S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
        S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
        figure_route_release(st, cur(st));
        break;
    }
    default:
        U8(st, f + FIG_ACTION) = 8;
        break;
    }
    /* @bin 0x0048DF8D the sprite: actions 0x0D and 5 face +0x13 */
    f = fig(st);
    action = U8(st, f + FIG_ACTION);
    int32_t facing = action == 0x0D || action == 5 ? S8(st, f + FIG_ATTACK_DIRECTION)
                     : S8(st, f + FIG_DIRECTION) < 8      ? S8(st, f + FIG_DIRECTION)
                                                 : S8(st, f + FIG_PREV_DIRECTION);
    int32_t d = facing - S32(st, G_VIEW_ORIENTATION);
    S32(st, G_SPRITE_DIRECTION) = d;
    if (d < 0) {
        d += 8;
        S32(st, G_SPRITE_DIRECTION) = d;
    }
    if (action == 0x0D) {
        S16(st, f + FIG_AMOUNT) += 1;
        int16_t n = S16(st, f + FIG_AMOUNT);
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_47) + 8 * (n < 0x24 ? n : 0) + d;
    } else if (action == 5) {
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_45) + d;
    } else if (action == 4) {
        S32(st, f + FIG_U04) = U8(st, G_DIE_FRAMES + (uint32_t) (S16(st, f + FIG_WAIT_TICKS) / 2)) + S32(st, G_IMAGE_GROUP_46);
    } else {
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_45) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
    }
}

/* @bin the grid offset of tile (x, y): the map's offset 0x00BCA2D8 + x + 228 y */
static int32_t grid_offset(game_state *st, int16_t x, int16_t y)
{
    return (int32_t) (U32(st, G_MAP_START) + (uint32_t) x + 228u * (uint32_t) y);
}

/* the figure stands at tile (x, y): +0x20 its grid offset, +0x4A/+0x4C the tile's centre (x 15 + 7) */
static void place_on_tile(game_state *st, uint32_t f, int16_t x, int16_t y)
{
    S16(st, f + FIG_X) = x;
    S16(st, f + FIG_Y) = y;
    S32(st, f + FIG_GRID_OFFSET) = grid_offset(st, S16(st, f + FIG_X), y);
    S16(st, f + FIG_CC_X) = (int16_t) (S16(st, f + FIG_X) * 15);
    S16(st, f + FIG_CC_Y) = (int16_t) (S16(st, f + FIG_Y) * 15);
    S16(st, f + FIG_CC_X) += 7;
    S16(st, f + FIG_CC_Y) += 7;
}

void figure_action_0e(game_state *st)
{
    /* @bin 0x00490B50 a figure that rides on the record +0x60 of the list at 0x00BCB9C0 (0x90 bytes
     * each): it takes the position of the record's figure (+0x8C), or one of the record's points */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    int16_t s = S16(st, f + FIG_FORMATION);
    U8(st, f + FIG_TERRAIN_USAGE) = 0;
    U8(st, f + FIG_ANIM_FRAME) += 1;
    if (U8(st, f + FIG_ANIM_FRAME) >= 0x10) {
        U8(st, f + FIG_ANIM_FRAME) = 0;
    }
    figure_tile_list_remove(st, id);
    uint32_t r = G_FORMATION_BASE - 8u + 0x90u * (uint32_t) s;
    id = cur(st);
    f = FIGURE_ADDR(id);
    if (U8(st, r + 8u + FM_U03) != 0) {
        place_on_tile(st, f, S16(st, r + 8u + FM_U36), S16(st, r + 8u + FM_U38));
    } else if (S32(st, r + 8u + FM_U84) != 0) {
        uint32_t g = FIGURE_ADDR(S32(st, r + 8u + FM_U84));
        S16(st, f + FIG_X) = S16(st, g + FIG_X);
        S16(st, f + FIG_Y) = S16(st, g + FIG_Y);
        S32(st, f + FIG_GRID_OFFSET) = grid_offset(st, S16(st, f + FIG_X), S16(st, f + FIG_Y));
        S16(st, f + FIG_CC_X) = S16(st, g + FIG_CC_X);
        S16(st, f + FIG_CC_Y) = S16(st, g + FIG_CC_Y);
        U8(st, f + FIG_PROGRESS_ON_TILE) = U8(st, g + FIG_PROGRESS_ON_TILE);
        U8(st, f + FIG_DIRECTION) = U8(st, g + FIG_DIRECTION);
    } else {
        place_on_tile(st, f, S16(st, r + 8u + FM_U32), S16(st, r + 8u + FM_U34));
    }
    GAME_CALL(st, FN_FIGURE_TILE_LIST_ADD, id);
    f = fig(st);
    S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_323) - S16(st, r + 8u + FM_MORALE) / 5 + 0x14;
    int16_t kind = S16(st, r + 8u + FM_TYPE);
    int still = S16(st, r + 8u + FM_U5A) != 0;
    int32_t base = kind == 0x0C ? 0 : kind == 0x0D ? 0x12 : 9;
    S32(st, f + FIG_U180) = S32(st, G_IMAGE_GROUP_395) + base + (still ? 8 : U8(st, f + FIG_ANIM_FRAME) >> 1);
}

/* the sprite direction of a walker: +0x11, or +0x12 for directions 8 and up (0x00490EC1) */
static int32_t walker_direction(game_state *st, uint32_t f)
{
    int32_t d = (S8(st, f + FIG_DIRECTION) >= 8 ? S8(st, f + FIG_PREV_DIRECTION) : S8(st, f + FIG_DIRECTION)) - S32(st, G_VIEW_ORIENTATION);
    S32(st, G_SPRITE_DIRECTION) = d;
    if (d < 0) {
        d += 8;
        S32(st, G_SPRITE_DIRECTION) = d;
    }
    return d;
}

void figure_action_0f(game_state *st)
{
    /* @bin 0x00490E00 figure types 0x0F, 0x10, 0x11, 0x12, 0x69 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    uint32_t info = type_info(st, f);
    S32(st, f + FIG_U180) = S32(st, G_IMAGE_GROUP_116);
    U8(st, f + FIG_TERRAIN_USAGE) = 1;
    U8(st, f + FIG_U0C) = 0;
    S16(st, f + FIG_MAX_ROAM_LENGTH) = 0x200;
    uint32_t b = BUILDING_ADDR(S16(st, f + FIG_HOME));
    advance_frame_12(st, f);
    uint8_t action = U8(st, f + FIG_ACTION);
    uint8_t dir;
    switch (action) {
    case 5:
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        f = fig(st);
        U8(st, f + FIG_ANIM_FRAME) += 1;
        if (U8(st, f + FIG_ANIM_FRAME) >= 0x20) {
            U8(st, f + FIG_ANIM_FRAME) = 0;
        }
        break;
    case 4:
        figure_dying(st, id);
        break;
    case 8:
        U8(st, f + FIG_INVISIBLE) = 1;
        U8(st, f + FIG_ANIM_FRAME) = 0;
        U8(st, f + FIG_U6B) = 0;
        S16(st, f + FIG_WAIT_TICKS) -= 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0) {
            break;
        }
        if (GAME_CALL(st, FN_BUILDING_ROAD_WITHIN, S16(st, b + BLD_ROAD_ACCESS_X), S16(st, b + BLD_ROAD_ACCESS_Y), U8(st, b + BLD_SIZE), 2) == 0) {
            U8(st, fig(st) + FIG_STATE) = 2;
        } else {
            walk_to_found_tile(st, 9);
        }
        break;
    case 9:
        U8(st, f + FIG_U0C) = 1;
        U8(st, f + FIG_INVISIBLE) = 1;
        if (figure_step_straight(st, id, 1) == 1) {
            int32_t s = walker_find_workplace(st, cur(st));
            if (s == 0 || road_next_to(st, BUILDING_ADDR(s)) == 0) {
                U8(st, fig(st) + FIG_STATE) = 2;
            } else {
                f = fig(st);
                S16(st, f + FIG_DEST_BUILDING) = (int16_t) s;
                U8(st, f + FIG_ACTION) = 0x0A;
                S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
                S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
                S16(st, f + FIG_ROAM_LENGTH) = 0;
            }
        }
        U8(st, fig(st) + FIG_INVISIBLE) = 1;
        break;
    case 0x0A:
        U8(st, f + FIG_INVISIBLE) = 0;
        S16(st, f + FIG_ROAM_LENGTH) += 1;
        if (S16(st, f + FIG_ROAM_LENGTH) >= 0xC80) {
            U8(st, f + FIG_STATE) = 2;
        }
        figure_move(st, id, (int32_t) info);
        id = cur(st);
        f = FIGURE_ADDR(id);
        dir = U8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            GAME_CALL(st, FN_FIGURE_MARK_BUILDING_0X20, id);
            U8(st, fig(st) + FIG_STATE) = 2;
        } else if (dir == 9) {
            figure_route_release(st, id);
        } else if (dir == 0x0A) {
            U8(st, f + FIG_STATE) = 2;
        }
        break;
    case 0x0C:
        U8(st, f + FIG_INVISIBLE) = 0;
        S16(st, f + FIG_ROAM_LENGTH) += 1;
        if (S16(st, f + FIG_ROAM_LENGTH) >= S16(st, f + FIG_MAX_ROAM_LENGTH)) {
            int32_t r = road_next_to(st, b);
            id = cur(st);
            f = FIGURE_ADDR(id);
            if (r == 0) {
                U8(st, f + FIG_STATE) = 2;
                figure_roam(st, id, 1);
                break;
            }
            U8(st, f + FIG_ACTION) = 0x0D;
            S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
            S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
        }
        figure_roam(st, id, 1);
        break;
    case 0x0D:
        figure_move(st, id, (int32_t) info);
        f = fig(st);
        dir = U8(st, f + FIG_DIRECTION);
        if (dir == 9 || dir == 0x0A || dir == 8) {
            U8(st, f + FIG_STATE) = 2;
        }
        break;
    case 6:
        U8(st, f + FIG_INVISIBLE) = 1;
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0x64) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
            if ((uint8_t) GAME_CALL(st, FN_FIGURE_MAY_LEAVE, id) == 0) {
                U8(st, fig(st) + FIG_ACTION) = 8;
            }
        }
        break;
    case 7:
        U8(st, f + FIG_ANIM_FRAME) = 0;
        break;
    default:
        U8(st, f + FIG_ACTION) = 8;
        break;
    }
    f = fig(st);
    int32_t d = walker_direction(st, f);
    /* @bin 0x004912E9 (jump table 0x004913A4) the sprite groups by figure type */
    uint32_t walk, death;
    switch (U8(st, f + FIG_TYPE)) {
    case 0x0F: walk = 0xA9; death = 0xAA; break;
    case 0x10: walk = 0xE6; death = 0xE7; break;
    case 0x11: walk = 0xA7; death = 0xA8; break;
    case 0x69: walk = 0x2A7; death = 0x2A8; break;
    default: walk = 0xAB; death = 0xAC; break;
    }
    if (U8(st, f + FIG_ACTION) == 4) {
        int16_t n = S16(st, f + FIG_WAIT_TICKS);
        S32(st, f + FIG_U04) = image_group(st, death) + (n >= 8 ? 7 : n);
        return;
    }
    S32(st, f + FIG_U04) = image_group(st, walk) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
}

/* to the city point at city record +0x2780/+0x2782 (action 0x0B) */
static void a13_leave(game_state *st, uint32_t f)
{
    uint32_t c = CURRENT_CITY(st);
    int16_t x = S16(st, c + CITY_EXIT_X);
    int16_t y = S16(st, c + CITY_EXIT_Y);
    U8(st, f + FIG_ACTION) = 0x0B;
    S16(st, f + FIG_WAIT_TICKS) = 0;
    S16(st, f + FIG_DEST_X) = x;
    S16(st, f + FIG_DEST_Y) = y;
}

/* @bin 0x00491725 a count per entry `r` of the figure's record +0x6E (table 0x0077592C, by the
 * record's byte 0x0077DE04, 0x6A bytes per record, 36 dwords per value) */
static void a13_count(game_state *st, int32_t r)
{
    uint32_t k = U8(st, fig(st) + FIG_U6E);
    uint32_t v = U8(st, G_EMPIRE_CITIES + EMPCITY_ROUTE + 0x6Au * k);
    S32(st, G_TRADE_DONE + 4u * ((uint32_t) r + 36u * v)) += 1;
}

void figure_action_13(game_state *st)
{
    /* @bin 0x00491420 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_INVISIBLE) = 0;
    U8(st, f + FIG_TERRAIN_USAGE) = 3;
    uint32_t info = type_info(st, f);
    advance_frame_12(st, f);
    S32(st, f + FIG_U180) = 0;
    uint8_t action = U8(st, f + FIG_ACTION);
    uint8_t dir;
    switch (action) {
    case 5:
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        break;
    case 4:
        figure_dying(st, id);
        break;
    case 8: {
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) <= 0x14) {
            U8(st, f + FIG_INVISIBLE) = 1;
            U8(st, f + FIG_ANIM_FRAME) = 0;
            break;
        }
        int32_t r = GAME_CALL(st, FN_STORAGE_FIND_FOR_TRADER, id, S16(st, f + FIG_X), S16(st, f + FIG_Y), U8(st, f + FIG_U6E), 0);
        f = fig(st);
        if (r != 0) {
            uint32_t b = BUILDING_ADDR(r);
            S16(st, f + FIG_DEST_BUILDING) = (int16_t) r;
            U8(st, f + FIG_ACTION) = 9;
            S16(st, f + FIG_WAIT_TICKS) = 0;
            S16(st, f + FIG_DEST_X) = S16(st, b + BLD_ROAD_ACCESS_X);
            S16(st, f + FIG_DEST_Y) = S16(st, b + BLD_ROAD_ACCESS_Y);
        } else {
            a13_leave(st, f);
        }
        U8(st, f + FIG_ANIM_FRAME) = 0;
        break;
    }
    case 9: {
        figure_move(st, id, (int32_t) info);
        f = fig(st);
        dir = U8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            U8(st, f + FIG_ACTION) = 0x0A;
        } else if (dir == 9) {
            figure_route_release(st, cur(st));
        } else if (dir == 0x0A) {
            /* @bin 0x00491614 +0x154 counts the ticks it is stuck */
            U8(st, f + FIG_U154) += 1;
            if (U8(st, f + FIG_U154) >= 0x64) {
                U8(st, f + FIG_STATE) = 2;
                U8(st, f + FIG_INVISIBLE) = 1;
            }
        } else {
            U8(st, f + FIG_U154) = 0;
        }
        uint32_t d = BUILDING_ADDR(S16(st, fig(st) + FIG_DEST_BUILDING));
        if (U8(st, d) != 1 || S16(st, d + BLD_WORKERS) == 0) {
            figure_route_release(st, cur(st));
            f = fig(st);
            U8(st, f + FIG_ACTION) = 8;
            S16(st, f + FIG_WAIT_TICKS) = 0x14;
        }
        break;
    }
    case 0x0A: {
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0x0A) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
            int32_t failed = 0;
            int32_t r = 0;
            if (GAME_CALL(st, FN_STORAGE_TRADES_24, id, U8(st, f + FIG_OWNER), S16(st, f + FIG_DEST_BUILDING), U8(st, f + FIG_U6E)) == 0 ||
                (f = fig(st), r = GAME_CALL(st, FN_STORAGE_SELL_NEXT, U8(st, f + FIG_OWNER), S16(st, f + FIG_DEST_BUILDING), U8(st, f + FIG_U6E))) ==
                    0) {
                failed = 1;
            } else {
                a13_count(st, r);
                GAME_CALL(st, FN_FIGURE_SLOT_COUNT_GOOD_B, cur(st), r);
                U8(st, fig(st) + FIG_U6F) += 1;
            }
            f = fig(st);
            if (GAME_CALL(st, FN_STORAGE_TRADES_13, cur(st), U8(st, f + FIG_OWNER), S16(st, f + FIG_DEST_BUILDING), U8(st, f + FIG_U6E)) == 0 ||
                (f = fig(st), r = GAME_CALL(st, FN_STORAGE_NEXT_GOOD_13, U8(st, f + FIG_OWNER), S16(st, f + FIG_DEST_BUILDING), U8(st, f + FIG_U6E))) ==
                    0) {
                failed++;
            } else {
                a13_count(st, r);
                GAME_CALL(st, FN_FIGURE_SLOT_COUNT_GOOD, cur(st), r);
                S16(st, fig(st) + FIG_AMOUNT) += 1;
            }
            if (failed == 2) {
                f = fig(st);
                r = GAME_CALL(st, FN_STORAGE_FIND_FOR_TRADER, cur(st), S16(st, f + FIG_X), S16(st, f + FIG_Y), U8(st, f + FIG_U6E), -1);
                f = fig(st);
                if (r != 0) {
                    S16(st, f + FIG_DEST_BUILDING) = (int16_t) r;
                    U8(st, f + FIG_ACTION) = 9;
                    S16(st, f + FIG_WAIT_TICKS) = 0;
                    S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
                    S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
                } else {
                    a13_leave(st, f);
                }
            }
        }
        U8(st, fig(st) + FIG_ANIM_FRAME) = 0;
        break;
    }
    case 0x0B:
        figure_move(st, id, (int32_t) info);
        f = fig(st);
        dir = U8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            U8(st, f + FIG_ACTION) = 8;
            U8(st, f + FIG_STATE) = 2;
        } else if (dir == 9) {
            figure_route_release(st, cur(st));
        } else if (dir == 0x0A) {
            U8(st, f + FIG_STATE) = 2;
        }
        break;
    case 7:
        U8(st, f + FIG_ANIM_FRAME) = 0;
        S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_597) + S32(st, G_SPRITE_DIRECTION) + 1;
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0x64) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
            if ((uint8_t) GAME_CALL(st, FN_FIGURE_MAY_LEAVE, id) == 0) {
                U8(st, fig(st) + FIG_ACTION) = 8;
            }
        }
        break;
    case 6:
        S32(st, f + FIG_U04) = 0;
        break;
    default:
        U8(st, f + FIG_ACTION) = 8;
        break;
    }
    f = fig(st);
    int32_t d = walker_direction(st, f);
    if (U8(st, f + FIG_ACTION) == 4) {
        S32(st, f + FIG_U04) = U8(st, G_DIE_FRAMES + (uint32_t) (S16(st, f + FIG_WAIT_TICKS) / 2)) + S32(st, G_IMAGE_GROUP_598);
        return;
    }
    S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_597) + 8 * U8(st, f + FIG_ANIM_FRAME) + d;
}

/* the point 0x00784C90 / 0x00784C92 (bytes) */
static void a14_to_exit(game_state *st, uint32_t f)
{
    U8(st, f + FIG_ACTION) = 0x0D;
    S16(st, f + FIG_WAIT_TICKS) = 0;
    S16(st, f + FIG_DEST_X) = U8(st, G_EXIT_X);
    S16(st, f + FIG_DEST_Y) = U8(st, G_EXIT_Y);
}

/* 0x00480860 on building d: +0x7A its result + 1; under 10 back to action 8, else out (0x0D) */
static void a14_next(game_state *st, int16_t d, int clear_link)
{
    uint8_t v = (uint8_t) GAME_CALL(st, FN_CITY_LIST_INDEX_0X480860, d);
    uint32_t f = fig(st);
    U8(st, f + FIG_COLLECTING) = (uint8_t) (v + 1);
    if (clear_link) {
        U8(st, f + FIG_U7B) = 0;
    }
    if (U8(st, f + FIG_COLLECTING) < 10) {
        U8(st, f + FIG_ACTION) = 8;
        S16(st, f + FIG_WAIT_TICKS) = 0;
        return;
    }
    a14_to_exit(st, f);
    if (clear_link) {
        S16(st, BUILDING_ADDR(d) + BLD_SECOND_WALKER) = 0;
    }
}

/* actions 8 and 0x0C: 0x004808A0 finds a building (then 0x004806C0 its entry tile), else
 * 0x004809B0 a place to go */
static int a14_found_building(game_state *st)
{
    int32_t r = GAME_CALL(st, FN_DOCK_RESERVE, cur(st));
    if (r == 0) {
        return 0;
    }
    uint32_t b = BUILDING_ADDR(r);
    GAME_CALL(st, FN_BUILDING_FIND_ROAD_8, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE));
    uint32_t f = fig(st);
    S16(st, f + FIG_DEST_BUILDING) = (int16_t) r;
    U8(st, f + FIG_ACTION) = 9;
    S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
    S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
    return 1;
}

void figure_action_14(game_state *st)
{
    /* @bin 0x00491A90 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    uint32_t info = type_info(st, f);
    int moving = 1;
    int16_t d = S16(st, f + FIG_DEST_BUILDING);
    if (d != 0 && S16(st, BUILDING_ADDR(d) + BLD_SECOND_WALKER) == id) {
        /* @bin 0x00491AE4 the building keeps the figure while it heads for its entry tile */
        uint32_t b = BUILDING_ADDR(d);
        GAME_CALL(st, FN_BUILDING_FIND_ROAD_8, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE));
        id = cur(st);
        f = FIGURE_ADDR(id);
        if (S32(st, G_ROAD_ACCESS_X) == S16(st, f + FIG_DEST_X) && S32(st, G_ROAD_ACCESS_Y) == S16(st, f + FIG_DEST_Y)) {
            S16(st, b + BLD_SECOND_WALKER) = (int16_t) id;
        } else {
            S16(st, b + BLD_SECOND_WALKER) = 0;
        }
    }
    U8(st, f + FIG_INVISIBLE) = 0;
    U8(st, f + FIG_BOAT_MODE) = 1;
    advance_frame_12(st, f);
    S32(st, f + FIG_U180) = 0;
    uint8_t action = U8(st, f + FIG_ACTION);
    uint8_t dir = 0;
    switch (action) {
    case 5:
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        break;
    case 4:
        figure_dying(st, id);
        break;
    case 8:
        moving = 0;
        U8(st, f + FIG_INVISIBLE) = 0;
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0x14) {
            if (!a14_found_building(st)) {
                int32_t r = GAME_CALL(st, FN_DOCK_FREE_MOORINGS, cur(st));
                f = fig(st);
                if (r != 0) {
                    U8(st, f + FIG_ACTION) = 0x0B;
                    S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
                    S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
                } else {
                    a14_to_exit(st, f);
                }
            }
            S16(st, fig(st) + FIG_WAIT_TICKS) = 0;
        }
        U8(st, fig(st) + FIG_ANIM_FRAME) = 0;
        break;
    case 0x0C:
        moving = 0;
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0x28) {
            if (!a14_found_building(st)) {
                f = fig(st);
                /* @bin 0x00491D97 the figure grid 0x00B350DC at its tile */
                if (S16(st, G_FIGURE_GRID + 2u * (uint32_t) S32(st, f + FIG_GRID_OFFSET)) != cur(st) &&
                    GAME_CALL(st, FN_DOCK_FREE_MOORINGS, cur(st)) != 0) {
                    f = fig(st);
                    U8(st, f + FIG_ACTION) = 0x0B;
                    S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
                    S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
                }
            }
            S16(st, fig(st) + FIG_WAIT_TICKS) = 0;
        }
        U8(st, fig(st) + FIG_ANIM_FRAME) = 0;
        break;
    case 9:
    case 0x0B:
    case 0x0D:
        figure_move(st, id, (int32_t) info);
        f = fig(st);
        U8(st, f + FIG_HEIGHT_TICKS) = 0;
        dir = U8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            if (action == 9) {
                U8(st, f + FIG_ACTION) = 0x0A;
            } else if (action == 0x0B) {
                U8(st, f + FIG_ACTION) = 0x0C;
                break;
            } else {
                U8(st, f + FIG_ACTION) = 8;
                U8(st, f + FIG_STATE) = 2;
                break;
            }
        } else if (dir == 9) {
            figure_route_release(st, cur(st));
        } else if (dir == 0x0A) {
            U8(st, f + FIG_STATE) = 2;
            /* @bin 0x00491E60 once (0x00E95184): message 0x72 */
            if (action == 9 && S32(st, G_UE95184) == 0) {
                S32(st, G_MESSAGE_POPUP) = 1;
                GAME_CALL(st, FN_MESSAGE_POST, 0x72, 0, 0, 0, 0);
                S32(st, G_UE95184) += 1;
            }
        }
        if (action == 9) {
            int16_t dest = S16(st, fig(st) + FIG_DEST_BUILDING);
            if (U8(st, BUILDING_ADDR(dest) + BLD_STATE) != 1) {
                a14_next(st, dest, 0);
            }
        }
        break;
    case 0x0A: {
        int16_t dest = S16(st, f + FIG_DEST_BUILDING);
        moving = 0;
        if (GAME_CALL(st, FN_FIGB_DOCK_LOST, id) != 0) {
            a14_next(st, dest, 1);
        } else if (GAME_CALL(st, FN_FIGB_DOCK_FREE, cur(st)) != 0) {
            uint32_t b = BUILDING_ADDR(dest);
            S16(st, b + BLD_DOCK_FIGURE) = 0;
            U8(st, b + BLD_UA7) = 0;
            U8(st, fig(st) + FIG_U7B) = 0;
            uint8_t v = (uint8_t) GAME_CALL(st, FN_CITY_LIST_INDEX_0X480860, dest);
            f = fig(st);
            U8(st, f + FIG_COLLECTING) = (uint8_t) (v + 1);
            if (U8(st, f + FIG_COLLECTING) < 10) {
                U8(st, f + FIG_ACTION) = 8;
                S16(st, f + FIG_WAIT_TICKS) = 0;
            } else {
                U8(st, f + FIG_ACTION) = 0x0D;
                S16(st, f + FIG_WAIT_TICKS) = 0;
                S16(st, f + FIG_DEST_X) = U8(st, G_EXIT_X);
                S16(st, b + BLD_SECOND_WALKER) = 0;
                S16(st, f + FIG_DEST_Y) = U8(st, G_EXIT_Y);
            }
        }
        /* @bin 0x00491F84 the facing from the building's +0xAA */
        uint8_t side = U8(st, BUILDING_ADDR(dest) + BLD_ORIENTATION);
        f = fig(st);
        S32(st, G_UE95184) = 0;
        U8(st, f + FIG_DIRECTION) = side == 0 ? 2 : side == 1 ? 4 : side == 2 ? 6 : 0;
        U8(st, f + FIG_ANIM_FRAME) = 0;
        break;
    }
    default:
        U8(st, f + FIG_ACTION) = 8;
        moving = 0;
        break;
    }
    f = fig(st);
    walker_direction(st, f);
    if (U8(st, f + FIG_ACTION) == 4) {
        if (S16(st, f + FIG_WAIT_TICKS) < 0x21) {
            uint8_t r = (uint8_t) figure_near_map_edge(st, S16(st, f + FIG_X));
            f = fig(st);
            int32_t base = r == 0 ? S32(st, G_IMAGE_GROUP_163) : S32(st, G_IMAGE_GROUP_709);
            S32(st, f + FIG_U04) = S32(st, G_SPRITE_DIRECTION) / 8 + base + S16(st, f + FIG_WAIT_TICKS) / 3;
            return;
        }
        int32_t v = S16(st, f + FIG_WAIT_TICKS) - 0x21;
        if (v >= 0x17) {
            v = 0x16;
        }
        S32(st, f + FIG_U04) = v + S32(st, G_IMAGE_GROUP_588);
        return;
    }
    uint8_t r = (uint8_t) figure_near_map_edge(st, S16(st, f + FIG_X));
    f = fig(st);
    if (moving) {
        int32_t base = r == 0 ? S32(st, G_IMAGE_GROUP_162) : S32(st, G_IMAGE_GROUP_708);
        S32(st, f + FIG_U04) = base + 8 * (U8(st, f + FIG_ANIM_FRAME) / 3) + S32(st, G_SPRITE_DIRECTION);
    } else {
        int32_t base = r == 0 ? S32(st, G_IMAGE_GROUP_164) : S32(st, G_IMAGE_GROUP_710);
        S32(st, f + FIG_U04) = base + S32(st, G_SPRITE_DIRECTION);
    }
}

/* ---- figure types 0x0B, 0x0C, 0x0D (0x0048E9E0): members of the companies listed at 0x00BCB9C0
 * (0x90 bytes each, the figure's +0x60) ----
 * The original addresses these formation records 8 bytes early (0x00BCB9C0 = G_FORMATION_BASE - 8):
 * r below is that address and a field is r + 8u + FM_* (the same constants as the original's). */


/* @bin 0x007090E4, 0x007090E8: the per-type table's two dwords before 0x007090EC (a range and a
 * search period) */
static int32_t type_range(game_state *st, uint8_t type)
{
    return S32(st, G_PROPS + FIGTYPE_RANGE + 0x38u * type);
}

static int32_t type_period(game_state *st, uint8_t type)
{
    return S32(st, G_PROPS + FIGTYPE_PERIOD + 0x38u * type);
}

/* @bin 0x0048EB4D the company point of slot +0x62 for formation `kind` (type 0x0D: kinds 1 and 2 have
 * their own tables 0x005CDD78, 0x005CDDF8), added to the base point (bx, by); words */
static void formation_point(game_state *st, uint32_t f, int32_t kind, int16_t bx, int16_t by, uint32_t out)
{
    uint32_t slot = U8(st, f + FIG_U62);
    uint32_t t;
    if (U8(st, f + FIG_TYPE) == 0x0D && kind == 1) {
        t = G_FORMATION_LAYOUT_0D_1 + 8u * slot;
    } else if (U8(st, f + FIG_TYPE) == 0x0D && kind == 2) {
        t = G_FORMATION_LAYOUT_0D_2 + 8u * slot;
    } else {
        t = G_FORMATION_LAYOUT_X + 8u * (slot + 16u * (uint32_t) kind);
    }
    S16(st, f + out) = (int16_t) (S16(st, t) + bx);
    S16(st, f + out + 2) = (int16_t) (S16(st, t + 4) + by);
}

/* the figure's place in the company (+0x24/+0x26), moved by its index +0x00 (table 0x005CA506), and
 * the tile's grid offset at +0x28 */
static void company_target(game_state *st, uint32_t f, uint32_t r, int32_t kind)
{
    formation_point(st, f, kind, S16(st, r + 8u + FM_U32), S16(st, r + 8u + FM_U34), 0x24);
    if (U8(st, f + FIG_U00) != 0) {
        S16(st, f + FIG_DEST_X) += S8(st, G_COMPANY_SHIFT_X + 2u * U8(st, f + FIG_U00));
        S16(st, f + FIG_DEST_Y) += S8(st, G_COMPANY_SHIFT_Y + 2u * U8(st, f + FIG_U00));
    }
}

static void set_grid_target(game_state *st, uint32_t f)
{
    S32(st, f + FIG_U28) = grid_offset(st, S16(st, f + FIG_DEST_X), S16(st, f + FIG_DEST_Y));
}

/* |a - b| in x and y, kept in 0x00D3B4FC / 0x00D3B4F8; returns the larger */
static int32_t chebyshev(game_state *st, int32_t ax, int32_t ay, int32_t bx, int32_t by)
{
    int32_t dx = distance_1d(ax, bx);
    S32(st, G_DIST_X) = dx;
    int32_t dy = distance_1d(ay, by);
    S32(st, G_DIST_Y) = dy;
    return dx >= dy ? dx : dy;
}

/* the figure's target +0x82: dropped when dead or dying (+0x0E not 1, action 4) */
static int16_t live_target(game_state *st, uint32_t f)
{
    int16_t t = S16(st, f + FIG_TARGET);
    if (U8(st, FIGURE_ADDR(t) + FIG_STATE) != 1) {
        t = 0;
        S16(st, f + FIG_TARGET) = 0;
    }
    if (U8(st, FIGURE_ADDR(t) + FIG_ACTION) == 4) {
        S16(st, f + FIG_TARGET) = 0;
        t = 0;
    }
    return t;
}

/* the figure takes figure t as its target (and t takes it as its attacker +0x84) */
static void set_target(game_state *st, int32_t t)
{
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    uint32_t g = FIGURE_ADDR(t);
    S16(st, f + FIG_DEST_X) = S16(st, g + FIG_X);
    S16(st, f + FIG_DEST_Y) = S16(st, g + FIG_Y);
    S16(st, f + FIG_TARGET) = (int16_t) t;
    S16(st, g + FIG_TARGETED_BY) = (int16_t) id;
    S16(st, f + FIG_U88) = S16(st, g + FIG_SEQUENCE);
}

/* 0x00499B50 every +0x6B ticks (period): +0x6A 1 and facing the figure found, 0 when none */
static int32_t periodic_search(game_state *st, int32_t extra, int *fired)
{
    uint32_t f = fig(st);
    U8(st, f + FIG_U6B) += 1;
    *fired = 0;
    if (U8(st, f + FIG_U6B) <= type_period(st, U8(st, f + FIG_TYPE))) {
        return 0;
    }
    *fired = 1;
    U8(st, f + FIG_U6B) = 0;
    int32_t t = GAME_CALL(st, FN_FIGB_FIND_TARGET, cur(st), type_range(st, U8(st, f + FIG_TYPE)) + extra);
    return t;
}

static void face_found(game_state *st, uint32_t f, int32_t t)
{
    uint32_t g = FIGURE_ADDR(t);
    U8(st, f + FIG_ATTACK_IMAGE) = 1;
    U8(st, f + FIG_DIRECTION) = (uint8_t) GAME_CALL(st, FN_DIRECTION_TO_FINE, S16(st, f + FIG_X), S16(st, f + FIG_Y),
                                           S16(st, g + FIG_X), S16(st, g + FIG_Y));
}

/* face figure t; on step 7 of +0x6A the blow (0x004B45C0) and the company's +0x64 */
static void strike(game_state *st, uint32_t r, int32_t t)
{
    uint32_t f = fig(st);
    uint32_t g = FIGURE_ADDR(t);
    U8(st, f + FIG_DIRECTION) = (uint8_t) GAME_CALL(st, FN_DIRECTION_TO_FINE, S16(st, f + FIG_X), S16(st, f + FIG_Y),
                                           S16(st, g + FIG_X), S16(st, g + FIG_Y));
    if (U8(st, f + FIG_ATTACK_IMAGE) == 7) {
        GAME_CALL(st, FN_MISSILE_CREATE, cur(st), S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, g + FIG_X),
                  S16(st, g + FIG_Y), 0x3C);
        S16(st, r + 8u + FM_U5C) = 6;
    }
}

/* 0x0048F5DB: +0x6A counts 0 .. 100 */
static void attack_step(game_state *st)
{
    uint32_t f = fig(st);
    U8(st, f + FIG_ATTACK_IMAGE) += 1;
    if (U8(st, f + FIG_ATTACK_IMAGE) > 100) {
        U8(st, f + FIG_ATTACK_IMAGE) = 0;
    }
}

/* in range of target t (types 0x0B, 0x0D only): 0x00D3B4FC/F8 updated */
static int target_in_range(game_state *st, int32_t t)
{
    uint32_t f = fig(st);
    uint8_t type = U8(st, f + FIG_TYPE);
    if (type != 0x0B && type != 0x0D) {
        return 0;
    }
    uint32_t g = FIGURE_ADDR(t);
    return chebyshev(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, g + FIG_X), S16(st, g + FIG_Y)) <=
           type_range(st, type);
}

enum { B_END, B_REROUTE, B_COUNT, B_LOST };

/* @bin 0x0048F466 .. 0x0048F548 (also 0x0048FEA6): with a target t, strike when in range, else walk
 * to it */
static int engage(game_state *st, uint32_t r, uint32_t info, int32_t t, int *in_range)
{
    *in_range = target_in_range(st, t);
    if (t == 0) {
        return B_END;
    }
    if (!*in_range) {
        figure_move(st, cur(st), (int32_t) info);
    }
    uint32_t f = fig(st);
    uint8_t dir = U8(st, f + FIG_DIRECTION);
    if (dir == 9 || dir == 0x0A) {
        return B_LOST;
    }
    if (*in_range) {
        strike(st, r, t);
        return B_COUNT;
    }
    if (dir != 8) {
        return B_END;
    }
    uint32_t g = FIGURE_ADDR(S16(st, f + FIG_TARGET));
    S16(st, f + FIG_DEST_X) = S16(st, g + FIG_X);
    S16(st, f + FIG_DEST_Y) = S16(st, g + FIG_Y);
    return B_REROUTE;
}

/* 0x00490397: arrived elsewhere (9) the route is released, lost (10) it dies */
static int dir_tail(game_state *st, uint8_t dir)
{
    if (dir == 9) {
        return B_REROUTE;
    }
    if (dir == 0x0A) {
        U8(st, fig(st) + FIG_STATE) = 2;
    }
    return B_END;
}

/* 0x0048ECDF: to +0x24 / +0x32 (the company point) */
static int formation_walk(game_state *st, uint32_t info)
{
    uint32_t f = fig(st);
    S16(st, f + FIG_DEST_Y) = S16(st, f + FIG_U32);
    set_grid_target(st, f);
    figure_move(st, cur(st), (int32_t) info);
    f = fig(st);
    uint8_t dir = U8(st, f + FIG_DIRECTION);
    if (dir == 8) {
        U8(st, f + FIG_ACTION) = 8;
        return B_END;
    }
    return dir_tail(st, dir);
}

/* @bin 0x0048EA28 the cart offsets of type 0x0D by direction (a table the original builds on its
 * stack) */
static const uint8_t company_offsets_d[16] = {0x16, 0xEE, 0x1B, 0xFD, 0x17, 0x11, 0x00, 0x0B,
                                              0xEB, 0x11, 0xE7, 0xFF, 0xE9, 0xEF, 0x00, 0xF4};

static int32_t img(game_state *st, int alt, uint32_t a, uint32_t b)
{
    return S32(st, alt ? b : a);
}

/* @bin 0x0048F614 the sprite */
static void company_sprite(game_state *st, uint32_t r, int in_range)
{
    uint32_t f = fig(st);
    uint8_t type = U8(st, f + FIG_TYPE);
    uint8_t action = U8(st, f + FIG_ACTION);
    int32_t d, aim = 0;
    if (type == 0x0B) {
        int16_t t = S16(st, f + FIG_TARGET);
        if (t != 0 && action == 0x0C) {
            uint32_t g = FIGURE_ADDR(t);
            U8(st, f + FIG_DIRECTION) = (uint8_t) GAME_CALL(st, FN_DIRECTION_TO_FINE, S16(st, f + FIG_X), S16(st, f + FIG_Y),
                                                   S16(st, g + FIG_X), S16(st, g + FIG_Y));
        }
        d = (S8(st, f + FIG_DIRECTION) < 8 ? S8(st, f + FIG_DIRECTION) : S8(st, f + FIG_PREV_DIRECTION)) - S32(st, G_VIEW_ORIENTATION);
        S32(st, G_SPRITE_DIRECTION) = d;
        aim = d + 2;
        if (aim > 7) {
            aim -= 8;
        }
    } else {
        int32_t facing;
        if (action == 5) {
            facing = S8(st, f + FIG_ATTACK_DIRECTION);
        } else if (S16(st, r + 8u + FM_U5C) != 0 && S8(st, f + FIG_DIRECTION) < 8) {
            facing = S8(st, f + FIG_DIRECTION);
        } else if (S16(st, r + 8u + FM_U5C) != 0 && S8(st, f + FIG_PREV_DIRECTION) < 8) {
            facing = S8(st, f + FIG_PREV_DIRECTION);
        } else if (action == 0x0C && U8(st, r + 8u + FM_FACING) < 8 && U8(st, f + FIG_ATTACK_IMAGE) == 0) {
            facing = U8(st, r + 8u + FM_FACING);
        } else if (action == 0x0C && U8(st, r + 8u + FM_U70) < 8 && U8(st, f + FIG_ATTACK_IMAGE) == 0) {
            facing = U8(st, r + 8u + FM_U70);
        } else {
            facing = S8(st, f + FIG_DIRECTION) < 8 ? S8(st, f + FIG_DIRECTION) : S8(st, f + FIG_PREV_DIRECTION);
        }
        d = facing - S32(st, G_VIEW_ORIENTATION);
        S32(st, G_SPRITE_DIRECTION) = d;
    }
    if (d < 0) {
        d += 8;
        S32(st, G_SPRITE_DIRECTION) = d;
    }
    if (d >= 8) {
        d -= 8;
        S32(st, G_SPRITE_DIRECTION) = d;
    }
    int alt = U8(st, G_ALT_SPRITES) != 0; /* @bin 0x00490A54 the other sprite set */
    uint8_t step = U8(st, f + FIG_ATTACK_IMAGE);
    uint8_t frame = U8(st, f + FIG_ANIM_FRAME);
    int32_t death = U8(st, G_DIE_FRAMES + (uint32_t) (S16(st, f + FIG_WAIT_TICKS) / 2));
    if (type == 0x0D) {
        S32(st, f + FIG_U180) = S32(st, G_IMAGE_GROUP_152) + 8 * frame + d;
        U8(st, f + FIG_U6C) = company_offsets_d[2 * d];
        U8(st, f + FIG_U6D) = company_offsets_d[2 * d + 1];
        if (action == 5) {
            S32(st, f + FIG_U04) = img(st, alt, G_IMAGE_GROUP_108, G_IMAGE_GROUP_268) + (step < 0x0C ? 0 : 8 * ((step - 0x0C) / 2)) + d;
        } else if (action == 0x0C) {
            S32(st, f + FIG_U04) = img(st, alt, G_IMAGE_GROUP_106, G_IMAGE_GROUP_266) + d;
        } else if (action == 0x14 && step != 0) {
            S32(st, f + FIG_U04) = img(st, alt, G_IMAGE_GROUP_108, G_IMAGE_GROUP_268) + 8 * U8(st, G_ATTACK_FRAMES + (step >> 1u)) + d;
        } else if (action == 4) {
            S32(st, f + FIG_U04) = death + img(st, alt, G_IMAGE_GROUP_107, G_IMAGE_GROUP_267);
        } else {
            S32(st, f + FIG_U04) = img(st, alt, G_IMAGE_GROUP_106, G_IMAGE_GROUP_266) + 8 * frame + d;
        }
    } else if (type == 0x0C) {
        S32(st, f + FIG_U180) = 0;
        if (action == 5) {
            S32(st, f + FIG_U04) = img(st, alt, G_IMAGE_GROUP_105, G_IMAGE_GROUP_259) + (step < 0x0C ? 0 : 8 * ((step - 0x0C) / 2)) + d;
        } else if (action == 4) {
            S32(st, f + FIG_U04) = death + img(st, alt, G_IMAGE_GROUP_104, G_IMAGE_GROUP_258);
        } else {
            S32(st, f + FIG_U04) = img(st, alt, G_IMAGE_GROUP_103, G_IMAGE_GROUP_257) + 8 * frame + d;
        }
    } else if (type == 0x0B) {
        S32(st, f + FIG_U180) = 0;
        int32_t fight = img(st, alt, G_IMAGE_GROUP_102, G_IMAGE_GROUP_255);
        if (action == 5) {
            S32(st, f + FIG_U04) = fight + (step < 0x0C ? 8 * step : 0) + aim;
        } else if (action == 0x0C || (action == 0x14 && in_range)) {
            if (action == 0x0C && S16(st, f + FIG_TARGET) == 0) {
                S32(st, f + FIG_U04) = img(st, alt, G_IMAGE_GROUP_100, G_IMAGE_GROUP_253) + 8 * frame + d;
            }
            if (step != 0 && step < 0x0C) {
                S32(st, f + FIG_U04) = fight + 8 * step + aim;
            } else {
                S32(st, f + FIG_U04) = fight + aim + 0x58;
            }
        } else if (action == 4) {
            S32(st, f + FIG_U04) = death + img(st, alt, G_IMAGE_GROUP_101, G_IMAGE_GROUP_254);
        } else {
            S32(st, f + FIG_U04) = img(st, alt, G_IMAGE_GROUP_100, G_IMAGE_GROUP_253) + 8 * frame + d;
        }
    }
    if (U8(st, f + FIG_ACTION) == 0x12) {
        S32(st, f + FIG_U04) = 0;
        S32(st, f + FIG_U180) = 0;
    }
}

void figure_action_0b(game_state *st)
{
    /* @bin 0x0048E9E0 figure types 0x0B, 0x0C, 0x0D */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    if ((U32(st, G_GRID_TERRAIN + 4u * (uint32_t) S32(st, f + FIG_GRID_OFFSET)) & 0x10004u) == 0x10004u) {
        GAME_CALL(st, FN_FIGB_REMOVE, id);
        return;
    }
    uint32_t city = CURRENT_CITY(st);
    int16_t company = S16(st, f + FIG_FORMATION);
    S32(st, city + CITY_U4678) += 1; /* @bin 0x0048EA3A 0x00EA5EE8: company members updated */
    U8(st, f + FIG_TERRAIN_USAGE) = 9;
    advance_frame_12(st, f);
    int in_range = 0;
    uint32_t r = G_FORMATION_BASE - 8u + 0x90u * (uint32_t) company;
    S32(st, f + FIG_U180) = 0;
    if (U8(st, r + 8u + FM_IN_USE) != 1) {
        U8(st, f + FIG_ACTION) = 4;
    }
    uint8_t type = U8(st, f + FIG_TYPE);
    uint32_t info = U32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * type);
    if (S16(st, r + 8u + FM_U7E) != 0) {
        int16_t n = S16(st, r + 8u + FM_U80);
        S16(st, r + 8u + FM_U80) = (int16_t) (n + 1);
        if (n > 2000) {
            S16(st, r + 8u + FM_U7E) = 0;
        }
    }
    int16_t kind = S16(st, r + 8u + FM_LAYOUT);
    int32_t k = kind;
    if (U8(st, f + FIG_U63) != 0 || U8(st, f + FIG_ACTION) == 9) {
        k = 7;
    }
    /* @bin 0x0048EB4D the figure's place in the company's formation (+0x30/+0x32) */
    formation_point(st, f, k, S16(st, r + 8u + FM_U36), S16(st, r + 8u + FM_U38), 0x30);
    int next = B_END;
    uint8_t dir;
    int32_t t;
    int fired;
    switch (U8(st, f + FIG_ACTION)) {
    case 5:
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        break;
    case 4:
        figure_dying(st, id);
        break;
    case 0x12:
        S16(st, f + FIG_WAIT_TICKS) = 0;
        U8(st, f + FIG_U63) = 0;
        break;
    case 8:
        figure_count_on_tile(st, id);
        f = fig(st);
        S16(st, f + FIG_WAIT_TICKS) = 0;
        U8(st, f + FIG_U63) = 1;
        U8(st, f + FIG_ANIM_FRAME) = 0;
        if (S16(st, f + FIG_X) != S16(st, f + FIG_U30) || S16(st, f + FIG_Y) != S16(st, f + FIG_U32)) {
            U8(st, f + FIG_ACTION) = 9;
        }
        break;
    case 3:
        if (U32(st, r + 8u + FM_U84) != 0) {
            break;
        }
        /* fall through */
    case 9:
        S16(st, f + FIG_WAIT_TICKS) = 0;
        U8(st, f + FIG_U63) = 1;
        S16(st, f + FIG_DEST_X) = S16(st, f + FIG_U30);
        next = formation_walk(st, info);
        break;
    case 0x10:
        U8(st, f + FIG_INVISIBLE) = 0;
        S16(st, f + FIG_WAIT_TICKS) = 0;
        U8(st, f + FIG_U63) = 1;
        S16(st, f + FIG_DEST_X) = S16(st, f + FIG_U30);
        next = formation_walk(st, info);
        break;
    case 0x16:
    case 0x17:
        U8(st, f + FIG_TERRAIN_USAGE) = 0x0A;
        S16(st, f + FIG_WAIT_TICKS) = 0;
        U8(st, f + FIG_U63) = 1;
        S16(st, f + FIG_DEST_X) = S16(st, f + FIG_U30);
        next = formation_walk(st, info);
        break;
    case 0x0A:
        U8(st, f + FIG_U63) = 1;
        S16(st, f + FIG_DEST_X) = S16(st, f + FIG_SOURCE_X);
        S16(st, f + FIG_DEST_Y) = S16(st, f + FIG_SOURCE_Y);
        figure_move(st, id, (int32_t) info);
        f = fig(st);
        dir = U8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            U8(st, f + FIG_STATE) = 2;
        } else {
            next = dir_tail(st, dir);
        }
        break;
    case 0x0B:
    case 0x13: {
        uint8_t action = U8(st, f + FIG_ACTION);
        U8(st, f + FIG_U63) = 0;
        company_target(st, f, r, kind);
        set_grid_target(st, f);
        uint32_t arg = info;
        if (action == 0x13) {
            /* @bin 0x0048F0A3 +0x13C/+0x140 the last tile, +0x134 the tiles changed: walks
             * twice as fast (?) from 6 to 19 */
            if (S16(st, f + FIG_X) != S32(st, f + FIG_U13C) || S16(st, f + FIG_Y) != S32(st, f + FIG_U140)) {
                S32(st, f + FIG_U13C) = S16(st, f + FIG_X);
                S32(st, f + FIG_U140) = S16(st, f + FIG_Y);
                S32(st, f + FIG_U134) += 1;
            }
            int32_t n = S32(st, f + FIG_U134);
            if (n > 5 && n < 0x14) {
                arg = info * 2;
            }
        }
        figure_move(st, id, (int32_t) arg);
        f = fig(st);
        dir = U8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            if (action == 0x13) {
                U8(st, f + FIG_ACTION) = 0x14;
                U8(st, f + FIG_ANIM_FRAME) = 0;
                S16(st, r + 8u + FM_LAYOUT) = 0x0F;
                S32(st, f + FIG_U134) = 0;
            } else {
                U8(st, f + FIG_ACTION) = S16(st, r + 8u + FM_LAYOUT) == 0x0F ? 0x14 : 0x0C;
                U8(st, f + FIG_ANIM_FRAME) = 0;
            }
        } else if (dir == 9) {
            next = B_REROUTE;
        } else if (dir == 0x0A) {
            U8(st, f + FIG_U00) += 1;
            if (U8(st, f + FIG_U00) > 0xA8) {
                U8(st, f + FIG_STATE) = 2;
            }
            U8(st, f + FIG_ANIM_FRAME) = 0;
        } else if (action == 0x13) {
            S16(st, r + 8u + FM_LAYOUT) = 0;
        }
        break;
    }
    case 0x14: {
        /* @bin 0x0048F19A the company point from the formation table read as dwords */
        uint32_t e = 8u * ((uint32_t) kind * 16u + U8(st, f + FIG_U62));
        int32_t tx = S16(st, r + 8u + FM_U32) + S32(st, G_FORMATION_LAYOUT_X + e);
        int32_t ty = S16(st, r + 8u + FM_U34) + S32(st, G_FORMATION_LAYOUT_Y + e);
        U8(st, f + FIG_U63) = 0;
        if (U8(st, f + FIG_U00) != 0) {
            tx += S8(st, G_COMPANY_SHIFT_X + 2u * U8(st, f + FIG_U00));
            ty += S8(st, G_COMPANY_SHIFT_Y + 2u * U8(st, f + FIG_U00));
        }
        t = live_target(st, f);
        uint8_t tt = U8(st, FIGURE_ADDR(t) + FIG_TYPE);
        if (tt == 0x0B || tt == 0x0C || tt == 0x0D) {
            t = 0;
            S16(st, f + FIG_TARGET) = 0;
        }
        int32_t ox = S16(st, f + FIG_X), oy = S16(st, f + FIG_Y);
        if (chebyshev(st, ox, oy, tx, ty) >= 6) {
            next = B_LOST;
            break;
        }
        if (t <= 0) {
            if (type != 0x0B && type != 0x0D) {
                t = find_enemy_near(st, ox, oy, 6, id);
            } else {
                int32_t found = periodic_search(st, 6, &fired);
                if (fired) {
                    t = found;
                    if (t != 0) {
                        face_found(st, fig(st), t);
                    } else {
                        U8(st, fig(st) + FIG_ATTACK_IMAGE) = 0;
                    }
                }
            }
            if (t != 0) {
                set_target(st, t);
            } else {
                f = fig(st);
                if (tx == S16(st, f + FIG_X) && ty == S16(st, f + FIG_Y)) {
                    U8(st, f + FIG_ANIM_FRAME) = 0;
                } else {
                    U8(st, f + FIG_ACTION) = 0x0C;
                }
            }
            figure_route_release(st, cur(st));
        }
        next = engage(st, r, info, t, &in_range);
        break;
    }
    case 0x0C: {
        U8(st, f + FIG_U63) = 0;
        U8(st, f + FIG_ANIM_FRAME) = 0;
        company_target(st, f, r, kind);
        if (S16(st, f + FIG_X) == S16(st, f + FIG_DEST_X) && S16(st, f + FIG_Y) == S16(st, f + FIG_DEST_Y)) {
            if (kind == 0x0F) {
                U8(st, f + FIG_ACTION) = 0x0B;
                U8(st, f + FIG_U00) = 0;
            }
        } else if (S16(st, r + 8u + FM_U5C) <= 0 && S16(st, r + 8u + FM_U50) <= 0 && S16(st, r + 8u + FM_U5E) <= 0) {
            U8(st, f + FIG_ACTION) = 0x0B;
            U8(st, f + FIG_U00) = 0;
        }
        if (type == 0x0B && U8(st, f + FIG_ACTION) != 0x0B) {
            t = S16(st, f + FIG_TARGET);
            int32_t found = periodic_search(st, 0, &fired);
            if (fired) {
                t = found;
                f = fig(st);
                if (t == 0) {
                    U8(st, f + FIG_ATTACK_IMAGE) = 0;
                    S16(st, f + FIG_TARGET) = 0;
                    break;
                }
                face_found(st, f, t);
            }
            if (t == 0) {
                break;
            }
            f = fig(st);
            uint32_t g = FIGURE_ADDR(t);
            S16(st, f + FIG_TARGET) = (int16_t) t;
            in_range = 0;
            S16(st, g + FIG_TARGETED_BY) = (int16_t) cur(st);
            S16(st, f + FIG_U88) = S16(st, g + FIG_SEQUENCE);
            if (chebyshev(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, g + FIG_X), S16(st, g + FIG_Y)) >
                type_range(st, U8(st, f + FIG_TYPE))) {
                break;
            }
            in_range = 1;
            strike(st, r, t);
            next = B_COUNT;
        } else if (type == 0x0D && U8(st, f + FIG_ACTION) != 0x0B) {
            int32_t found = periodic_search(st, 0, &fired);
            f = fig(st);
            if (fired) {
                S16(st, f + FIG_TARGET) = (int16_t) found;
                if (found != 0) {
                    face_found(st, f, found);
                } else {
                    U8(st, f + FIG_ATTACK_IMAGE) = 0;
                }
            }
            if (U8(st, f + FIG_ATTACK_IMAGE) == 0) {
                GAME_CALL(st, FN_FIGURE_MEET_AROUND, cur(st));
                break;
            }
            t = S16(st, f + FIG_TARGET);
            if (t != 0) {
                strike(st, r, t);
            }
            attack_step(st);
        }
        break;
    }
    case 0x0E:
        U8(st, f + FIG_U63) = 0;
        t = S16(st, f + FIG_TARGET);
        if (U8(st, FIGURE_ADDR(t) + FIG_STATE) != 1) {
            t = 0;
            S16(st, f + FIG_TARGET) = 0;
        }
        int search = 0;
        if (U8(st, FIGURE_ADDR(t) + FIG_ACTION) == 4) {
            S16(st, f + FIG_TARGET) = 0;
            t = 0;
            search = 1;
        } else if (t <= 0) {
            search = 1;
        }
        if (search) {
            if (type != 0x0B && type != 0x0D) {
                t = find_enemy_near(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), 0x14, id);
            } else {
                int32_t found = periodic_search(st, 0x14, &fired);
                if (fired) {
                    t = found;
                    if (t != 0) {
                        face_found(st, fig(st), t);
                    } else {
                        U8(st, fig(st) + FIG_ATTACK_IMAGE) = 0;
                    }
                }
            }
            if (t != 0) {
                set_target(st, t);
            } else {
                f = fig(st);
                U8(st, f + FIG_ACTION) = 0x0C;
                U8(st, f + FIG_ANIM_FRAME) = 0;
            }
            figure_route_release(st, cur(st));
        }
        next = engage(st, r, info, t, &in_range);
        break;
    case 0x15: {
        U8(st, f + FIG_U63) = 0;
        t = S16(st, f + FIG_TARGET);
        if (U8(st, FIGURE_ADDR(t) + FIG_STATE) != 1) {
            t = 0;
            S16(st, f + FIG_TARGET) = 0;
        }
        if (U8(st, FIGURE_ADDR(t) + FIG_ACTION) == 4) {
            t = 0;
            S16(st, f + FIG_TARGET) = 0;
        }
        /* @bin 0x0049009B the company's enemy group (+0x94; -1: none) */
        int32_t group = S32(st, r + 8u + FM_TARGET);
        int same = S16(st, FIGURE_ADDR(t) + FIG_FORMATION) == group;
        if (!same) {
            S16(st, f + FIG_TARGET) = 0;
        }
        if (!same || t <= 0) {
            if (type == 0x0C) {
                t = find_enemy_near(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), 100, id);
            } else {
                t = GAME_CALL(st, FN_FIGB_FIND_TARGET, id, 100);
            }
            group = S32(st, r + 8u + FM_TARGET);
            if (group == -1) {
                t = 0;
            } else if (t != 0) {
                set_target(st, t);
            }
        }
        if (group == -1 || t == 0) {
            S32(st, r + 8u + FM_TARGET) = -1;
            f = fig(st);
            U8(st, f + FIG_ACTION) = 0x0B;
            U8(st, f + FIG_ANIM_FRAME) = 0;
            break;
        }
        if (target_in_range(st, t)) {
            U8(st, fig(st) + FIG_ANIM_FRAME) = 0;
            in_range = 1;
            strike(st, r, t);
            next = B_COUNT;
            break;
        }
        in_range = 0;
        figure_move(st, cur(st), (int32_t) info);
        f = fig(st);
        if (U8(st, f + FIG_DIRECTION) == 8) {
            uint32_t g = FIGURE_ADDR(S16(st, f + FIG_TARGET));
            S16(st, f + FIG_DEST_X) = S16(st, g + FIG_X);
            S16(st, f + FIG_DEST_Y) = S16(st, g + FIG_Y);
            next = B_REROUTE;
        }
        break;
    }
    case 0x0D:
        U8(st, f + FIG_TERRAIN_USAGE) = 0x0A;
        S16(st, r + 8u + FM_U48) = 1;
        U8(st, f + FIG_U63) = 1;
        figure_move(st, id, (int32_t) info);
        f = fig(st);
        dir = U8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            U8(st, f + FIG_ACTION) = 0x17;
        } else {
            next = dir_tail(st, dir);
        }
        break;
    case 0x0F:
        U8(st, f + FIG_U63) = 0;
        S16(st, f + FIG_DEST_X) = S16(st, city + CITY_EXIT_X);
        S16(st, f + FIG_DEST_Y) = S16(st, city + CITY_EXIT_Y);
        figure_move(st, id, (int32_t) info);
        f = fig(st);
        dir = U8(st, f + FIG_DIRECTION);
        if (dir == 8 || dir == 9 || dir == 0x0A) {
            U8(st, f + FIG_ACTION) = 0x11;
            figure_route_release(st, cur(st));
            figure_tile_list_remove(st, cur(st));
        }
        break;
    case 0x11:
        U8(st, f + FIG_INVISIBLE) = 1;
        U8(st, f + FIG_U63) = 1;
        break;
    case 6:
        U8(st, f + FIG_TERRAIN_USAGE) = 0x0A;
        S32(st, f + FIG_U04) = 0;
        S32(st, f + FIG_U180) = 0;
        break;
    case 7:
        U8(st, f + FIG_TERRAIN_USAGE) = 0x0A;
        U8(st, f + FIG_ANIM_FRAME) = 0;
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0x64) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
            if ((uint8_t) GAME_CALL(st, FN_FIGURE_MAY_LEAVE, id) == 0) {
                U8(st, fig(st) + FIG_STATE) = 2;
            }
        }
        break;
    default:
        U8(st, f + FIG_ACTION) = 8;
        break;
    }
    if (next == B_LOST) {
        f = fig(st);
        U8(st, f + FIG_ACTION) = 0x0C;
        S16(st, f + FIG_TARGET) = 0;
        U8(st, f + FIG_ANIM_FRAME) = 0;
    } else if (next == B_COUNT) {
        attack_step(st);
        next = B_REROUTE;
    }
    if (next == B_REROUTE) {
        figure_route_release(st, cur(st));
    }
    company_sprite(st, r, in_range);
}

#define ACTION(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
ACTION(run_action_01, figure_action_01)
ACTION(run_action_05, figure_action_05)
ACTION(run_action_06, figure_action_06)
ACTION(run_action_02, figure_action_02)
ACTION(run_action_03, figure_action_03)
ACTION(run_action_04, figure_action_04)
ACTION(run_action_07, figure_action_07)
ACTION(run_action_08, figure_action_08)
ACTION(run_action_09, figure_action_09)

static int32_t run_storage_for_good(game_state *st, const int32_t *a)
{
    return cart_storage_for_good(st, a[0], a[1], a[2]);
}

static int32_t run_storage_to_empty(game_state *st, const int32_t *a)
{
    return cart_storage_to_empty(st, a[0]);
}

static int32_t run_workshop_for_good(game_state *st, const int32_t *a)
{
    return cart_workshop_for_good(st, a[0], a[1], a[2]);
}

static int32_t run_take_from_storage(game_state *st, const int32_t *a)
{
    return cart_take_from_storage(st, a[0], a[1]);
}

static int32_t run_delivered(game_state *st, const int32_t *a)
{
    cart_delivered(st, a[0], a[1], a[2], a[3]);
    return 0;
}
ACTION(run_action_0a, figure_action_0a)
ACTION(run_action_0e, figure_action_0e)
ACTION(run_action_0f, figure_action_0f)
ACTION(run_action_13, figure_action_13)
ACTION(run_action_14, figure_action_14)
ACTION(run_action_0b, figure_action_0b)

static int32_t run_advance(game_state *st, const int32_t *a)
{
    figure_advance(st, a[0], a[1]);
    return 0;
}

static int32_t run_move(game_state *st, const int32_t *a)
{
    figure_move(st, a[0], a[1]);
    return 0;
}

#define FIG1(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        fn(st, a[0]); \
        return 0; \
    }
FIG1(run_route_release, figure_route_release)
FIG1(run_dying, figure_dying)
FIG1(run_count_on_tile, figure_count_on_tile)
FIG1(run_tile_list_remove, figure_tile_list_remove)

static int32_t run_near_map_edge(game_state *st, const int32_t *a)
{
    return figure_near_map_edge(st, a[0]);
}

static int32_t run_find_destination(game_state *st, const int32_t *a)
{
    return cart_find_destination(st, a[0], a[1]);
}

static int32_t run_good_sprites(game_state *st, const int32_t *a)
{
    (void) st;
    return cart_good_sprites(a[0]);
}

FIG1(run_sprite_load, cart_sprite_load)

static int32_t run_step_straight(game_state *st, const int32_t *a)
{
    return figure_step_straight(st, a[0], a[1]);
}

static int32_t run_line_to(game_state *st, const int32_t *a)
{
    figure_line_to(st, a[0], a[1], a[2], a[3], a[4], a[5]);
    return 0;
}

static int32_t run_roam(game_state *st, const int32_t *a)
{
    figure_roam(st, a[0], a[1]);
    return 0;
}

static int32_t run_0a_divert(game_state *st, const int32_t *a)
{
    return figure_0a_divert(st, a[0]);
}

FIG1(run_figure_substep, figure_substep)
FIG1(run_figure_line_error, figure_line_error)
FIG1(run_figure_line_step_x, figure_line_step_x)
FIG1(run_figure_line_step_y, figure_line_step_y)
FIG1(run_figure_route_direction, figure_route_direction)
FIG1(run_figure_turn_around, figure_turn_around)
FIG1(run_figure_enter_next_tile, figure_enter_next_tile)

FIG1(run_find_route, figure_find_route)

static int32_t run_route_to_any(game_state *st, const int32_t *a)
{
    return figure_route_to_any(st, a[0], (uint32_t) a[1], a[2]);
}

static int32_t run_workshop_with_room(game_state *st, const int32_t *a)
{
    return cart_workshop_with_room(st, a[0], a[1], a[2]);
}

static int32_t run_free_tile(game_state *st, const int32_t *a)
{
    return route_free_tile(st, a[0]);
}

static int32_t run_walk_to(game_state *st, const int32_t *a)
{
    return route_walk_to(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_walk_to_any(game_state *st, const int32_t *a)
{
    return route_walk_to_any(st, a[0], a[1], (uint32_t) a[2], a[3]);
}

static int32_t run_walk_free_to(game_state *st, const int32_t *a)
{
    return route_walk_free_to(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_any_free_to(game_state *st, const int32_t *a)
{
    return route_any_free_to(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_any_free_to_any(game_state *st, const int32_t *a)
{
    return route_any_free_to_any(st, a[0], a[1], (uint32_t) a[2], a[3]);
}

static int32_t run_free_to_marked(game_state *st, const int32_t *a)
{
    return route_walk_free_to_marked(st, a[0], a[1], (uint32_t) a[2], a[3]);
}

static int32_t run_move_hurried(game_state *st, const int32_t *a)
{
    figure_move_hurried(st, a[0], a[1]);
    return 0;
}

static int32_t run_find_workplace(game_state *st, const int32_t *a)
{
    return walker_find_workplace(st, a[0]);
}

static int32_t run_road_wear(game_state *st, const int32_t *a)
{
    return road_wear(st, a[0], a[1]);
}

static int32_t run_open_directions(game_state *st, const int32_t *a)
{
    return roam_open_directions(st, a[0], a[1]);
}

static int32_t run_neighbours_with_flags(game_state *st, const int32_t *a)
{
    neighbours_with_flags(st, a[0]);
    return 0;
}

static int32_t run_neighbours_count(game_state *st, const int32_t *a)
{
    (void) a;
    return neighbours_count(st);
}

static int32_t run_queue_push(game_state *st, const int32_t *a)
{
    const uint8_t *p = state_ptr(st, (uint32_t) a[0], 4);
    if (!p) {
        state_unported(st, FN_QUEUE_PUSH); /* the value lives in the caller's frame */
        return 0;
    }
    return queue_push(st, (uint32_t) a[PORT_THIS_ARG], (int32_t) P32(st, a[0]));
}

static int32_t run_queue_length(game_state *st, const int32_t *a)
{
    return queue_length(st, (uint32_t) a[PORT_THIS_ARG]);
}

static int32_t run_queue_full(game_state *st, const int32_t *a)
{
    return queue_full(st, (uint32_t) a[PORT_THIS_ARG]);
}

static int32_t run_tile_wear(game_state *st, const int32_t *a)
{
    tile_wear(st, a[0], a[1]);
    return 0;
}

static int32_t run_ferry_on_route(game_state *st, const int32_t *a)
{
    return ferry_on_route(st, a[0], a[1]);
}

FIG1(run_check_next_tile, figure_check_next_tile)

static int32_t run_serve_tile(game_state *st, const int32_t *a)
{
    return figure_serve_tile(st, a[0]);
}

static int32_t run_route_store(game_state *st, const int32_t *a)
{
    return route_store(st, a[0], a[1], a[2], a[3], a[4], a[5]);
}

static int32_t run_road_wear_up(game_state *st, const int32_t *a)
{
    road_wear_up(st, a[0], a[1]);
    return 0;
}

static int32_t run_find_enemy_near(game_state *st, const int32_t *a)
{
    return find_enemy_near(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_storage_with_room(game_state *st, const int32_t *a)
{
    return cart_storage_with_room(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_dock_for_good(game_state *st, const int32_t *a)
{
    return cart_dock_for_good(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_meet(game_state *st, const int32_t *a)
{
    figure_meet(st, a[0], a[1]);
    return 0;
}

static int32_t run_route_even_to(game_state *st, const int32_t *a)
{
    return route_even_to(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_route_even_to_any(game_state *st, const int32_t *a)
{
    return route_even_to_any(st, a[0], a[1], (uint32_t) a[2], a[3]);
}

static int32_t run_route_any_to(game_state *st, const int32_t *a)
{
    return route_any_to(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_route_not3_to(game_state *st, const int32_t *a)
{
    return route_not3_to(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_route_or5_to(game_state *st, const int32_t *a)
{
    return route_or5_to(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_route_water_to(game_state *st, const int32_t *a)
{
    return route_water_to(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_route_boat_to(game_state *st, const int32_t *a)
{
    return route_boat_to(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_route_boat_to_any(game_state *st, const int32_t *a)
{
    return route_boat_to_any(st, a[0], a[1], (uint32_t) a[2], a[3]);
}

static int32_t run_route_free6_to(game_state *st, const int32_t *a)
{
    return route_free6_to(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_route_free6_to_any(game_state *st, const int32_t *a)
{
    return route_free6_to_any(st, a[0], a[1], (uint32_t) a[2], a[3]);
}

static int32_t run_route_ec7630_to_any(game_state *st, const int32_t *a)
{
    return route_ec7630_to_any(st, a[0], a[1], (uint32_t) a[2], a[3]);
}

const ported_function figa_ports[] = {
    {0x0051D420u, "route_free6_to", 4, 32, 0, run_route_free6_to, 0},
    {0x0051D6D0u, "route_free6_to_any", 4, 32, 0, run_route_free6_to_any, 0},
    {0x0051B220u, "route_ec7630_to_any", 4, 32, 0, run_route_ec7630_to_any, 0},
    {0x00518400u, "route_even_to", 4, 32, 0, run_route_even_to, 0},
    {0x00518DD0u, "route_even_to_any", 4, 32, 0, run_route_even_to_any, 0},
    {0x005186D0u, "route_any_to", 4, 32, 0, run_route_any_to, 0},
    {0x00519390u, "route_not3_to", 4, 32, 0, run_route_not3_to, 0},
    {0x005195E0u, "route_or5_to", 4, 32, 0, run_route_or5_to, 0},
    {0x00519BE0u, "route_water_to", 4, 32, 0, run_route_water_to, 0},
    {0x0051A030u, "route_boat_to", 4, 32, 0, run_route_boat_to, 0},
    {0x0051A220u, "route_boat_to_any", 4, 32, 0, run_route_boat_to_any, 0},
    {0x004B1210u, "figure_meet", 2, 0, 0, run_meet, 0},
    {0x0045ACF0u, "cart_dock_for_good", 4, 32, 0, run_dock_for_good, 0},
    {0x0045D9E0u, "cart_storage_with_room", 4, 32, 0, run_storage_with_room, 0},
    {0x004999E0u, "find_enemy_near", 4, 32, 0, run_find_enemy_near, 0},
    {0x0051B400u, "route_store", 6, 32, 0, run_route_store, 0},
    {0x0047E250u, "road_wear_up", 2, 0, 0, run_road_wear_up, 0},
    {0x004BAA10u, "figure_serve_tile", 2, 32, 0, run_serve_tile, 0},
    {0x004F65B0u, "queue_push", 1, 32, 0, run_queue_push, 0},
    {0x004F6630u, "queue_length", 0, 32, 0, run_queue_length, 0},
    {0x004F6650u, "queue_full", 0, 32, 0, run_queue_full, 0},
    {0x004690F0u, "tile_wear", 2, 0, 0, run_tile_wear, 0},
    {0x004B3470u, "ferry_on_route", 2, 32, 0, run_ferry_on_route, 0},
    {0x004B3530u, "figure_check_next_tile", 1, 0, 0, run_check_next_tile, 0},
    {0x0047DB00u, "neighbours_with_flags", 1, 0, 0, run_neighbours_with_flags, 0},
    {0x00484F40u, "neighbours_count", 0, 32, 0, run_neighbours_count, 0},
    {0x0047E230u, "road_wear", 2, 32, 0, run_road_wear, 0},
    {0x0047E340u, "roam_open_directions", 2, 32, 0, run_open_directions, 0},
    {0x00467450u, "walker_find_workplace", 1, 32, 0, run_find_workplace, 0},
    {0x004A5680u, "figure_move_hurried", 2, 0, 0, run_move_hurried, 0},
    {0x00517F20u, "route_any_free_to", 4, 32, 0, run_any_free_to, 0},
    {0x00518B30u, "route_any_free_to_any", 4, 32, 0, run_any_free_to_any, 0},
    {0x005190A0u, "route_walk_free_to_marked", 4, 32, 0, run_free_to_marked, 0},
    {0x0051AFF0u, "route_free_tile", 1, 32, 0, run_free_tile, 0},
    {0x00519840u, "route_walk_to", 4, 32, 0, run_walk_to, 0},
    {0x00519E30u, "route_walk_to_any", 4, 32, 0, run_walk_to_any, 0},
    {0x00518880u, "route_walk_free_to", 4, 32, 0, run_walk_free_to, 0},
    {0x0045EAB0u, "cart_workshop_with_room", 3, 32, 0, run_workshop_with_room, 0},
    {0x004B28F0u, "figure_route_to_any", 3, 32, 0, run_route_to_any, 0},
    {0x004B2D20u, "figure_find_route", 1, 0, 0, run_find_route, 0},
    {0x00489970u, "figure_route_slot (in actions_a3.c)", 2, 8, 0, NULL, 0},
    {0x004B3F00u, "figure_substep", 1, 0, 0, run_figure_substep, 0},
    {0x004B4180u, "figure_line_error", 1, 0, 0, run_figure_line_error, 0},
    {0x004B43E0u, "figure_line_step_x", 1, 0, 0, run_figure_line_step_x, 0},
    {0x004B43A0u, "figure_line_step_y", 1, 0, 0, run_figure_line_step_y, 0},
    {0x004B33D0u, "figure_route_direction", 1, 0, 0, run_figure_route_direction, 0},
    {0x004B21D0u, "figure_turn_around", 1, 0, 0, run_figure_turn_around, 0},
    {0x004B3C60u, "figure_enter_next_tile", 1, 0, 0, run_figure_enter_next_tile, 0},
    {0x004B3FF0u, "figure_step_straight", 2, 32, 0, run_step_straight, 0},
    {0x004B4220u, "figure_line_to", 6, 0, 0, run_line_to, 0},
    {0x004B1EF0u, "figure_roam", 2, 0, 0, run_roam, 0},
    {0x0048E5E0u, "figure_0a_divert", 1, 32, 0, run_0a_divert, 0},
    {0x0048AE50u, "cart_find_destination", 2, 32, 0, run_find_destination, 0},
    {0x0048B320u, "cart_good_sprites", 1, 32, 0, run_good_sprites, 0},
    {0x0048B4A0u, "cart_sprite_load", 1, 0, 0, run_sprite_load, 0},
    {0x004B27E0u, "figure_advance", 2, 0, 0, run_advance, 0},
    {0x004A5DA0u, "figure_move", 2, 0, 0, run_move, 0},
    {0x004B3250u, "figure_route_release", 1, 0, 0, run_route_release, 0},
    {0x004B1D50u, "figure_dying", 1, 0, 0, run_dying, 0},
    {0x0047A1C0u, "figure_near_map_edge", 2, 8, 0, run_near_map_edge, 0},
    {0x004B53B0u, "figure_count_on_tile", 1, 0, 0, run_count_on_tile, 0},
    {0x004B5420u, "figure_tile_list_remove", 1, 0, 0, run_tile_list_remove, 0},
    {0x0045DCC0u, "cart_storage_for_good", 3, 32, 0, run_storage_for_good, 0},
    {0x0045DE80u, "cart_storage_to_empty", 1, 32, 0, run_storage_to_empty, 0},
    {0x0045F450u, "cart_workshop_for_good", 3, 32, 0, run_workshop_for_good, 0},
    {0x0045E150u, "cart_take_from_storage", 2, 32, 0, run_take_from_storage, 0},
    {0x0048CAA0u, "cart_delivered", 4, 0, 0, run_delivered, 0},
    {0x00489CA0u, "figure_action_01", 0, 0, 0, run_action_01, 0},
    {0x0048BFF0u, "figure_action_05", 0, 0, 0, run_action_05, 0},
    {0x0048C200u, "figure_action_06", 0, 0, 0, run_action_06, 0},
    {0x0048A260u, "figure_action_02", 0, 0, 0, run_action_02, 0},
    {0x0048A720u, "figure_action_03", 0, 0, 0, run_action_03, 0},
    {0x0048B620u, "figure_action_04", 0, 0, 0, run_action_04, 0},
    {0x0048C2B0u, "figure_action_07", 0, 0, 0, run_action_07, 0},
    {0x0048C6A0u, "figure_action_08", 0, 0, 0, run_action_08, 0},
    {0x0048CE30u, "figure_action_09", 0, 0, 0, run_action_09, 0},
    {0x0048DED0u, "figure_action_0a", 0, 0, 0, run_action_0a, 0},
    {0x00490B50u, "figure_action_0e", 0, 0, 0, run_action_0e, 0},
    {0x00490E00u, "figure_action_0f", 0, 0, 0, run_action_0f, 0},
    {0x00491420u, "figure_action_13", 0, 0, 0, run_action_13, 0},
    {0x00491A90u, "figure_action_14", 0, 0, 0, run_action_14, 0},
    {0x0048E9E0u, "figure_action_0b", 0, 0, 0, run_action_0b, 0},
};
const int figa_ports_count = sizeof(figa_ports) / sizeof(figa_ports[0]);
