/* Imhotep - figure actions, group D: helpers shared by the module's files.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_FIGURE_ACTIONS_D_LOCAL_H
#define IMHOTEP_FIGURE_ACTIONS_D_LOCAL_H

#include "sim/call.h"
#include "state/records.h"

#define CURRENT_FIGURE 0x00D056F8u
#define VIEW_ORIENTATION 0x00BCA2E0u /* subtracted from figure directions for the sprites */
#define SPRITE_DIRECTION 0x00E94E00u /* the current figure's direction relative to the view */
#define FRAME_TABLE 0x005D048Cu      /* @bin frames of the dying animation, by +0x36 / 2 */
#define ROUTE_DEST_X 0x00D3B4E4u     /* @bin destination tile found by 0x0047FE60 */
#define ROUTE_DEST_Y 0x00D3B4E0u
#define BUILDING_GRID 0x00B4E6FCu    /* @bin building id per tile (s16) */

static inline int32_t cur(game_state *st)
{
    return S32(st, CURRENT_FIGURE);
}

static inline uint32_t fig(game_state *st)
{
    return FIGURE_ADDR(cur(st));
}

/* the sprite direction of a figure direction: d - view orientation, wrapped into 0..7 */
static inline int32_t sprite_direction(game_state *st, int32_t d)
{
    d -= S32(st, VIEW_ORIENTATION);
    S32(st, SPRITE_DIRECTION) = d;
    if (d < 0) {
        d += 8;
        S32(st, SPRITE_DIRECTION) = d;
    }
    return d;
}

/* the dying frame: FRAME_TABLE[+0x36 / 2] (signed halving) */
static inline uint32_t dying_frame(game_state *st, uint32_t f)
{
    return U8(st, FRAME_TABLE + (uint32_t) (S16(st, f + 0x36) / 2));
}

/* +0x01 animation frame, 0 .. 11 */
static inline uint8_t next_frame(game_state *st, uint32_t f)
{
    U8(st, f + 0x01) += 1;
    if (U8(st, f + 0x01) >= 0x0C) {
        U8(st, f + 0x01) = 0;
    }
    return U8(st, f + 0x01);
}

/* 0x004B4220(id, x, y, dest_x * 15, dest_y * 15, 0) on the figure's +0x4A/+0x4C and +0x24/+0x26 */
static inline void move_to_dest(game_state *st, int32_t id, uint32_t f)
{
    GAME_CALL(st, 0x004B4220u, id, S16(st, f + 0x4A), S16(st, f + 0x4C), S16(st, f + 0x24) * 15,
              S16(st, f + 0x26) * 15, 0);
}

/* 0x0047FE60(building x, y, size, 2): a road tile next to the building, into ROUTE_DEST */
static inline int32_t road_near(game_state *st, uint32_t b)
{
    return GAME_CALL(st, 0x0047FE60u, S16(st, b + 0x06), S16(st, b + 0x08), U8(st, b + 0x03), 2);
}

#define TERRAIN_GRID 0x00AF598Cu /* @bin terrain bits per tile (u32) */

#endif
