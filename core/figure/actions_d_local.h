/* Imhotep - figure actions, group D: helpers shared by the module's files.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_FIGURE_ACTIONS_D_LOCAL_H
#define IMHOTEP_FIGURE_ACTIONS_D_LOCAL_H

#include "sim/call.h"
#include "state/layout.h"
#include "state/records.h"

static inline int32_t cur(game_state *st)
{
    return S32(st, G_CURRENT_FIGURE);
}

static inline uint32_t fig(game_state *st)
{
    return FIGURE_ADDR(cur(st));
}

/* the sprite direction of a figure direction: d - view orientation, wrapped into 0..7 */
static inline int32_t sprite_direction(game_state *st, int32_t d)
{
    d -= S32(st, G_VIEW_ORIENTATION);
    S32(st, G_SPRITE_DIRECTION) = d;
    if (d < 0) {
        d += 8;
        S32(st, G_SPRITE_DIRECTION) = d;
    }
    return d;
}

/* @bin the dying frame: G_DIE_FRAMES[FIG_WAIT_TICKS / 2] (signed halving) */
static inline uint32_t dying_frame(game_state *st, uint32_t f)
{
    return U8(st, G_DIE_FRAMES + (uint32_t) (S16(st, f + FIG_WAIT_TICKS) / 2));
}

/* +0x01 animation frame, 0 .. 11 */
static inline uint8_t next_frame(game_state *st, uint32_t f)
{
    U8(st, f + FIG_ANIM_FRAME) += 1;
    if (U8(st, f + FIG_ANIM_FRAME) >= 0x0C) {
        U8(st, f + FIG_ANIM_FRAME) = 0;
    }
    return U8(st, f + FIG_ANIM_FRAME);
}

/* @bin 0x004B4220(id, x, y, dest_x * 15, dest_y * 15, 0) on the figure's +0x4A/+0x4C and +0x24/+0x26 */
static inline void move_to_dest(game_state *st, int32_t id, uint32_t f)
{
    GAME_CALL(st, FN_FIGURE_LINE_TO, id, S16(st, f + FIG_CC_X), S16(st, f + FIG_CC_Y), S16(st, f + FIG_DEST_X) * 15,
              S16(st, f + FIG_DEST_Y) * 15, 0);
}

/* @bin 0x0047FE60(building x, y, size, 2): a road tile next to the building, into G_ROAD_ACCESS_X / _Y */
static inline int32_t road_near(game_state *st, uint32_t b)
{
    return GAME_CALL(st, FN_BUILDING_ROAD_WITHIN, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE), 2);
}

#endif
