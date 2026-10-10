/* Imhotep - figure helpers called by the actions (package figa): movement steps, the per-tile
 * figure lists, route slots.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21). These take the figure id as an argument. */
#include "figure/actions_a.h"

#include "figure/actions_c2.h"

#include "sim/call.h"
#include "sim/math.h"
#include "state/layout.h"
#include "state/records.h"


static int32_t cur(game_state *st)
{
    return S32(st, G_CURRENT_FIGURE);
}

void figure_advance(game_state *st, int32_t id, int32_t steps)
{
    /* @bin 0x004B27E0 `steps` sub-steps along the route: +0x39 counts 0 .. 15 per tile; at 15 the
     * figure reaches the next tile (0x004BAA10, 0x004B33D0, 0x004B3530; a new route 0x004B2D20 when
     * it has none) */
    uint32_t f = FIGURE_ADDR(id);
    while (steps > 0) {
        steps--;
        U8(st, f + FIG_PROGRESS_ON_TILE) += 1;
        if (S8(st, f + FIG_PROGRESS_ON_TILE) >= 15) {
            figure_serve_tile(st, id);
            U8(st, f + FIG_PROGRESS_ON_TILE) = 15;
            if (S16(st, f + FIG_PATH_ID) <= 0) {
                figure_find_route(st, id);
            }
            figure_route_direction(st, id);
            figure_check_next_tile(st, id);
            if (S8(st, f + FIG_DIRECTION) >= 8) {
                break; /* arrived, lost, ...: +0x11 8 and up */
            }
            S16(st, f + FIG_PATH_STEP) += 1;
            U8(st, f + FIG_PREV_DIRECTION) = U8(st, f + FIG_DIRECTION);
            U8(st, f + FIG_PROGRESS_ON_TILE) = 0;
        }
        if (U8(st, f + FIG_PROGRESS_ON_TILE) == 8) {
            figure_enter_next_tile(st, id);
        }
        figure_substep(st, id);
    }
    S32(st, G_UBCD5E8) = 0;
}

/* the speed pattern's counter +0x160: below `limit` `fast` sub-steps and the counter goes up, else
 * `slow` sub-steps and it restarts at 0 */
static void move_pattern(game_state *st, int32_t id, uint8_t limit, int32_t fast, int32_t slow)
{
    uint32_t c = FIGURE_ADDR(id) + FIG_U160;
    if (U8(st, c) <= limit) {
        figure_advance(st, id, fast);
        U8(st, c) += 1;
    } else {
        figure_advance(st, id, slow);
        U8(st, c) = 0;
    }
}

void figure_move(game_state *st, int32_t id, int32_t speed)
{
    /* @bin 0x004A5DA0 (jump table 0x004A610C) one tick of walking at `speed` (0 .. 0x11; others: 3
     * sub-steps), the cycle counted at +0x160 */
    uint32_t c = FIGURE_ADDR(id) + FIG_U160;
    uint8_t n = U8(st, c);
    switch (speed) {
    case 0:
        figure_advance(st, id, 0);
        return;
    case 1:
        if (n > 2) {
            figure_advance(st, id, 1);
            U8(st, c) = 0;
        } else {
            U8(st, c) = (uint8_t) (n + 1);
        }
        return;
    case 2:
        if (n <= 1) {
            U8(st, c) = (uint8_t) (n + 1);
        } else {
            U8(st, c) = 0;
            figure_advance(st, id, 1);
        }
        return;
    case 3:
        if (n != 0) {
            U8(st, c) = 0;
        } else {
            figure_advance(st, id, 1);
            U8(st, c) = 1;
        }
        return;
    case 4:
    case 5:
        if (n > (speed == 4 ? 1 : 2)) {
            U8(st, c) = 0;
        } else {
            figure_advance(st, id, 1);
            U8(st, c) += 1;
        }
        return;
    case 6:
        figure_advance(st, id, 1);
        return;
    case 7: move_pattern(st, id, 2, 1, 2); return;
    case 8: move_pattern(st, id, 1, 1, 2); return;
    case 9: move_pattern(st, id, 0, 1, 2); return;
    case 0x0A: move_pattern(st, id, 1, 2, 1); return;
    case 0x0B: move_pattern(st, id, 2, 2, 1); return;
    case 0x0C:
        figure_advance(st, id, 2);
        return;
    case 0x0D: move_pattern(st, id, 2, 2, 3); return;
    case 0x0E: move_pattern(st, id, 1, 2, 3); return;
    case 0x0F: move_pattern(st, id, 0, 3, 2); return;
    case 0x10: move_pattern(st, id, 1, 3, 2); return;
    case 0x11: move_pattern(st, id, 2, 3, 2); return;
    default:
        figure_advance(st, id, 3);
        return;
    }
}

void figure_route_release(game_state *st, int32_t id)
{
    /* @bin 0x004B3250 the figure's route slot +0x3A is freed (and its owner entry if still it) */
    uint32_t f = FIGURE_ADDR(id);
    int16_t slot = S16(st, f + FIG_PATH_ID);
    if (slot > 0) {
        if (S16(st, G_ROUTE_OWNERS + 2u * (uint32_t) slot) == id) {
            S16(st, G_ROUTE_OWNERS + 2u * (uint32_t) slot) = 0;
        }
        S16(st, f + FIG_PATH_ID) = 0;
    }
}

void figure_dying(game_state *st, int32_t id)
{
    /* @bin 0x004B1D50 action 4: +0x36 counts the dying animation up to 0x7F, then the figure is
     * removed */
    uint32_t f = FIGURE_ADDR(id);
    if (S16(st, f + FIG_WAIT_TICKS) < 0) {
        S16(st, f + FIG_WAIT_TICKS) = 0;
    }
    S16(st, f + FIG_WAIT_TICKS) += 1;
    if (S16(st, f + FIG_WAIT_TICKS) >= 0x80) {
        S16(st, f + FIG_WAIT_TICKS) = 0x7F;
        U8(st, f + FIG_STATE) = 2;
    }
}

int figure_near_map_edge(game_state *st, int32_t x)
{
    /* @bin 0x0047A1C0 (the second argument is not read) 0x0078D7AC / 0x0078D7B0: when the second is
     * positive, half their difference against 228 - x */
    if (S32(st, G_BLOOD_RIVER_LEFT) <= 0) {
        return 0;
    }
    return (S32(st, G_BLOOD_RIVER_DURATION) - S32(st, G_BLOOD_RIVER_LEFT)) / 2 > 0xE4 - x;
}

void figure_count_on_tile(game_state *st, int32_t id)
{
    /* @bin 0x004B53B0 +0x8A: the figures ahead of it in its tile's list (at most 20) */
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_TILE_INDEX) = 0;
    int16_t o = S16(st, G_FIGURE_GRID + 2u * (uint32_t) S32(st, f + FIG_GRID_OFFSET));
    while (o != 0) {
        if (o == id) {
            return;
        }
        U8(st, f + FIG_TILE_INDEX) += 1;
        o = S16(st, FIGURE_ADDR(o) + FIG_NEXT_ON_TILE);
    }
    if (U8(st, f + FIG_TILE_INDEX) > 0x14) {
        U8(st, f + FIG_TILE_INDEX) = 0x14;
    }
}

void figure_tile_list_remove(game_state *st, int32_t id)
{
    /* @bin 0x004B5420 the figure leaves its tile's list (0x0051F780, an empty function, is called
     * for the figures whose link changed) */
    uint32_t f = FIGURE_ADDR(id);
    int32_t tile = S32(st, f + FIG_GRID_OFFSET);
    if (tile < 0) {
        GAME_CALL(st, FN_NOTHING, id);
        return;
    }
    uint32_t head = G_FIGURE_GRID + 2u * (uint32_t) tile;
    int16_t o = S16(st, head);
    if (o == 0) {
        GAME_CALL(st, FN_NOTHING, id);
        return;
    }
    if (o == id) {
        S16(st, head) = S16(st, f + FIG_NEXT_ON_TILE);
        S16(st, f + FIG_NEXT_ON_TILE) = 0;
        GAME_CALL(st, FN_NOTHING, id);
        return;
    }
    int16_t next = S16(st, FIGURE_ADDR(o) + FIG_NEXT_ON_TILE);
    while (next != id) {
        if (o == 0) {
            return;
        }
        o = next;
        next = S16(st, FIGURE_ADDR(o) + FIG_NEXT_ON_TILE);
    }
    S16(st, FIGURE_ADDR(o) + FIG_NEXT_ON_TILE) = S16(st, f + FIG_NEXT_ON_TILE);
    S16(st, f + FIG_NEXT_ON_TILE) = 0;
    GAME_CALL(st, FN_NOTHING, id);
    GAME_CALL(st, FN_NOTHING, o);
}

int32_t figure_step_straight(game_state *st, int32_t id, int32_t steps)
{
    /* @bin 0x004B3FF0 `steps` sub-steps on the straight line set by 0x004B4220 (+0x52/+0x54 the
     * distances left, +0x56 the error, +0x58 the major axis); returns 1 when the line is done. The
     * figure leaves its tile list and enters the list of its new tile (0x004B52F0). */
    int32_t done = 0;
    figure_tile_list_remove(st, id);
    uint32_t f = FIGURE_ADDR(id);
    for (int32_t i = 0; i < steps; i++) {
        if (U8(st, f + FIG_MISSILE_DAMAGE) != 0) {
            U8(st, f + FIG_MISSILE_DAMAGE) -= 1;
        } else {
            U8(st, f + FIG_MISSILE_DAMAGE) = 0;
        }
        if (S16(st, f + FIG_CC_DELTA_Y) + S16(st, f + FIG_CC_DELTA_X) <= 0) {
            done = 1;
            break;
        }
        figure_line_error(st, id);
        if (U8(st, f + FIG_CC_DIRECTION) == 2) {
            figure_line_step_y(st, id);
            if (S16(st, f + FIG_CC_ERROR) >= 0) {
                S16(st, f + FIG_CC_DELTA_X) -= 1;
                figure_line_step_x(st, id);
            }
        } else {
            figure_line_step_x(st, id);
            if (S16(st, f + FIG_CC_ERROR) >= 0) {
                S16(st, f + FIG_CC_DELTA_Y) -= 1;
                figure_line_step_y(st, id);
            }
        }
    }
    /* the tile from the position in 15ths (signed divisions) */
    S16(st, f + FIG_X) = (int16_t) (S16(st, f + FIG_CC_X) / 15);
    S16(st, f + FIG_Y) = (int16_t) (S16(st, f + FIG_CC_Y) / 15);
    uint32_t tile = U32(st, G_MAP_START) + (uint32_t) S16(st, f + FIG_X) + 228u * (uint32_t) S16(st, f + FIG_Y);
    S32(st, f + FIG_GRID_OFFSET) = (int32_t) tile;
    /* @bin 0x004B412B terrain flag 8 (0x00AF598C): +0x40 set to 8, else counts down */
    if (U32(st, G_GRID_TERRAIN + 4u * tile) & 8) {
        U8(st, f + FIG_IN_BUILDING_WAIT) = 8;
    } else if (U8(st, f + FIG_IN_BUILDING_WAIT) != 0) {
        U8(st, f + FIG_IN_BUILDING_WAIT) -= 1;
    }
    GAME_CALL(st, FN_FIGURE_TILE_LIST_ADD, id);
    return done;
}

void figure_line_to(game_state *st, int32_t id, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t exact)
{
    /* @bin 0x004B4220 a straight line from (x1, y1) to (x2, y2) (in 15ths of tiles): +0x4E/+0x50 the
     * end, +0x52/+0x54 the distances, +0x56 the error term, +0x11 the direction (0x0051CC00 when
     * `exact`, else 0x0051CBA0 snapped to an axis when one distance is over twice the other),
     * +0x58 the major axis (2: y) */
    uint32_t f = FIGURE_ADDR(id);
    S16(st, f + FIG_CC_DEST_X) = (int16_t) x2;
    S16(st, f + FIG_CC_DEST_Y) = (int16_t) y2;
    S16(st, f + FIG_CC_DELTA_X) = (int16_t) (x1 > x2 ? x1 - x2 : x2 - x1);
    S16(st, f + FIG_CC_DELTA_Y) = (int16_t) (y1 > y2 ? y1 - y2 : y2 - y1);
    int16_t dy = S16(st, f + FIG_CC_DELTA_Y);
    int16_t dx = S16(st, f + FIG_CC_DELTA_X);
    if (dx < dy) {
        S16(st, f + FIG_CC_ERROR) = (int16_t) (2 * dx - dy);
    } else if (dx > dy) {
        S16(st, f + FIG_CC_ERROR) = (int16_t) (2 * dy - dx);
    } else {
        S16(st, f + FIG_CC_ERROR) = 0;
    }
    if (exact) {
        U8(st, f + FIG_DIRECTION) = (uint8_t) GAME_CALL(st, FN_DIRECTION_BETWEEN, x1, y1, x2, y2);
    } else {
        uint8_t d = (uint8_t) GAME_CALL(st, FN_ROUTE_DIRECTION, x1, y1, x2, y2);
        U8(st, f + FIG_DIRECTION) = d;
        if (S16(st, f + FIG_CC_DELTA_Y) > 2 * S16(st, f + FIG_CC_DELTA_X)) {
            if (d == 1 || d == 7) {
                U8(st, f + FIG_DIRECTION) = 0;
            } else if (d == 3 || d == 5) {
                U8(st, f + FIG_DIRECTION) = 4;
            }
        }
        if (S16(st, f + FIG_CC_DELTA_X) > 2 * S16(st, f + FIG_CC_DELTA_Y)) {
            d = U8(st, f + FIG_DIRECTION);
            if (d == 1 || d == 3) {
                U8(st, f + FIG_DIRECTION) = 2;
            } else if (d == 5 || d == 7) {
                U8(st, f + FIG_DIRECTION) = 6;
            }
        }
    }
    U8(st, f + FIG_CC_DIRECTION) = S16(st, f + FIG_CC_DELTA_Y) > S16(st, f + FIG_CC_DELTA_X) ? 2 : 1;
}


static int open_direction(game_state *st, uint8_t d)
{
    return S32(st, G_NEIGHBOUR_FLAGS + 4u * (uint32_t) (int8_t) d) != 0;
}

void figure_roam(game_state *st, int32_t id, int32_t steps)
{
    /* @bin 0x004B1EF0 a roaming walker: first back on the road with the current figure's speed
     * (+0x46 set once there), then `steps` sub-steps choosing at each tile among the open
     * directions (0x0047E340): a single one, a turn of +0x48 at a junction of 2, the tile's
     * pattern (0x00ACF85C + the tiles walked +0x47) at a crossing; +0x49 the moves left before
     * the walker turns around (0x004B21D0). The counter k is not reset between sub-steps. */
    uint32_t f = FIGURE_ADDR(id);
    int32_t k = 0;
    if (U8(st, f + FIG_ROAM_STATE) == 0) {
        S32(st, G_UBCD5E8) = 1;
        figure_move(st, cur(st), (int32_t) U32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE)));
        uint8_t d = U8(st, f + FIG_DIRECTION);
        if (d == 9 || d == 0x0A) {
            U8(st, f + FIG_ROAM_STATE) = 1;
        } else if (d == 8) {
            U8(st, f + FIG_ROAM_STATE) = 1;
            S16(st, f + FIG_ROAM_LENGTH) = 0;
        }
        if (U8(st, f + FIG_ROAM_STATE) == 0) {
            return;
        }
        U8(st, f + FIG_U49) = 0x64;
        U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
    }
    while (steps > 0) {
        steps--;
        U8(st, f + FIG_PROGRESS_ON_TILE) += 1;
        if (S8(st, f + FIG_PROGRESS_ON_TILE) >= 15) {
            U8(st, f + FIG_PROGRESS_ON_TILE) = 15;
            int32_t back = (S8(st, f + FIG_PREV_DIRECTION) + 4) % 8; /* the way it came */
            U8(st, f + FIG_U47) += 1;
            if (figure_serve_tile(st, id) != 0) {
                return;
            }
            int32_t n = roam_open_directions(st, S32(st, f + FIG_GRID_OFFSET), id);
            if (n <= 0) {
                uint32_t c = FIGURE_ADDR(cur(st));
                S16(st, c + FIG_ROAM_LENGTH) = S16(st, c + FIG_MAX_ROAM_LENGTH);
                return;
            }
            if (n == 1) {
                uint8_t d = (uint8_t) (k << 1);
                U8(st, f + FIG_DIRECTION) = d;
                while (!open_direction(st, d)) {
                    int32_t old = k++;
                    if (old >= 4) {
                        break;
                    }
                    d = (uint8_t) (k << 1);
                    U8(st, f + FIG_DIRECTION) = d;
                }
            } else {
                if (n == 2) {
                    if (U8(st, f + FIG_U49) == 0xFF) {
                        figure_turn_around(st, id);
                        back = -1;
                    }
                } else {
                    /* @bin 0x004B20CA the tile's turning pattern */
                    uint8_t d = (uint8_t) ((U8(st, G_RANDOM + (uint32_t) S32(st, f + FIG_GRID_OFFSET)) + U8(st, f + FIG_U47)) & 6);
                    U8(st, f + FIG_DIRECTION) = d;
                    if (open_direction(st, d) && (int8_t) d != back) {
                        goto chosen;
                    }
                    U8(st, f + FIG_U49) -= 1;
                    if (S8(st, f + FIG_U49) <= 0) {
                        figure_turn_around(st, id);
                        back = -1;
                    }
                }
                for (;;) {
                    uint8_t d = U8(st, f + FIG_DIRECTION);
                    if (open_direction(st, d) && (int8_t) d != back) {
                        break;
                    }
                    d = (uint8_t) (U8(st, f + FIG_U48) + d);
                    U8(st, f + FIG_DIRECTION) = d;
                    if ((int8_t) d > 6) {
                        U8(st, f + FIG_DIRECTION) = 0;
                    }
                    if (S8(st, f + FIG_DIRECTION) < 0) {
                        U8(st, f + FIG_DIRECTION) = 6;
                    }
                    int32_t old = k++;
                    if (old >= 4) {
                        break;
                    }
                }
            }
        chosen:
            S16(st, f + FIG_PATH_STEP) += 1;
            U8(st, f + FIG_PREV_DIRECTION) = U8(st, f + FIG_DIRECTION);
            U8(st, f + FIG_PROGRESS_ON_TILE) = 0;
        }
        figure_substep(st, id);
        if (U8(st, f + FIG_PROGRESS_ON_TILE) == 8) {
            figure_enter_next_tile(st, id);
        }
    }
}

int32_t figure_0a_divert(game_state *st, int32_t id)
{
    /* @bin 0x0048E5E0 (figure type 0x0A) every 20 ticks (+0x6B) outside actions 4, 5, 8, 9, 0x0C ..
     * 0x0F, when 0x0078D804 is positive: the building 0x00469A50 finds at its tile, while
     * 0x0078D7F8 is at most 25, becomes its destination (+0x5E, action 0x0C, the building's +0x30
     * the figure); returns 1 then */
    if (S32(st, G_RUINS_FOUND) <= 0) {
        return 0;
    }
    uint32_t f = FIGURE_ADDR(id);
    uint8_t a = U8(st, f + FIG_ACTION);
    if (a == 5 || a == 4 || a == 8 || a == 9 || a == 0x0C || a == 0x0D || a == 0x0E || a == 0x0F) {
        return 0;
    }
    U8(st, f + FIG_U6B) += 1;
    if (U8(st, f + FIG_U6B) < 0x14) {
        return 0;
    }
    int32_t b = GAME_CALL(st, FN_NEAREST_BUILDING_0X63, S16(st, f + FIG_X), S16(st, f + FIG_Y));
    if (b <= 0) {
        return 0;
    }
    if (S32(st, G_RUIN_DISTANCE) > 0x19) {
        return 0;
    }
    uint32_t r = BUILDING_ADDR(b);
    U8(st, f + FIG_U6B) = 0;
    U8(st, f + FIG_ACTION) = 0x0C;
    S16(st, f + FIG_DEST_X) = S16(st, r + BLD_ROAD_ACCESS_X);
    S16(st, f + FIG_DEST_Y) = S16(st, r + BLD_ROAD_ACCESS_Y);
    S16(st, f + FIG_DEST_BUILDING) = (int16_t) b;
    figure_route_release(st, id);
    S16(st, r + BLD_TOWER_WALKER) = (int16_t) id;
    return 1;
}

/* @bin 0x005F80FC the grid offset of the next tile in each direction (0 north, clockwise) */
static int32_t direction_offset(game_state *st, uint32_t d)
{
    return S32(st, G_DIRECTION_OFFSETS + 4u * d);
}

void figure_substep(game_state *st, int32_t id)
{
    /* @bin 0x004B3F00 (jump table 0x004B3FD0) the position in 15ths (+0x4A x, +0x4C y) moves one
     * unit along +0x11; +0x77 the ticks left of an effect, +0x78 its level towards +0x79 */
    uint32_t f = FIGURE_ADDR(id);
    switch (S8(st, f + FIG_DIRECTION)) {
    case 0: S16(st, f + FIG_CC_Y) -= 1; break;
    case 1: S16(st, f + FIG_CC_Y) -= 1; S16(st, f + FIG_CC_X) += 1; break;
    case 2: S16(st, f + FIG_CC_X) += 1; break;
    case 3: S16(st, f + FIG_CC_Y) += 1; S16(st, f + FIG_CC_X) += 1; break;
    case 4: S16(st, f + FIG_CC_Y) += 1; break;
    case 5: S16(st, f + FIG_CC_Y) += 1; S16(st, f + FIG_CC_X) -= 1; break;
    case 6: S16(st, f + FIG_CC_X) -= 1; break;
    case 7: S16(st, f + FIG_CC_Y) -= 1; S16(st, f + FIG_CC_X) -= 1; break;
    default: break;
    }
    if (U8(st, f + FIG_HEIGHT_TICKS) == 0) {
        if (U8(st, f + FIG_HEIGHT) != 0) {
            U8(st, f + FIG_HEIGHT) -= 1;
        }
        return;
    }
    U8(st, f + FIG_HEIGHT_TICKS) -= 1;
    if (U8(st, f + FIG_HEIGHT_TICKS) == 0) {
        U8(st, f + FIG_INVISIBLE) = 0;
        return;
    }
    U8(st, f + FIG_INVISIBLE) = 1;
    if (U8(st, f + FIG_HEIGHT) < U8(st, f + FIG_HEIGHT_TARGET)) {
        U8(st, f + FIG_HEIGHT) += 1;
    }
    if (U8(st, f + FIG_HEIGHT) > U8(st, f + FIG_HEIGHT_TARGET)) {
        U8(st, f + FIG_HEIGHT) -= 1;
    }
}

void figure_line_error(game_state *st, int32_t id)
{
    /* @bin 0x004B4180 one step of the line's error term (+0x56), the minor distance goes down */
    uint32_t f = FIGURE_ADDR(id);
    int16_t e = S16(st, f + FIG_CC_ERROR);
    if (U8(st, f + FIG_CC_DIRECTION) == 1) {
        if (e < 0) {
            S16(st, f + FIG_CC_ERROR) = (int16_t) (2 * S16(st, f + FIG_CC_DELTA_Y) + e);
        } else {
            S16(st, f + FIG_CC_ERROR) = (int16_t) (e + 2 * (int16_t) (S16(st, f + FIG_CC_DELTA_Y) - S16(st, f + FIG_CC_DELTA_X)));
        }
        S16(st, f + FIG_CC_DELTA_X) -= 1;
    } else {
        if (e < 0) {
            S16(st, f + FIG_CC_ERROR) = (int16_t) (2 * S16(st, f + FIG_CC_DELTA_X) + e);
        } else {
            S16(st, f + FIG_CC_ERROR) = (int16_t) (e + 2 * (int16_t) (S16(st, f + FIG_CC_DELTA_X) - S16(st, f + FIG_CC_DELTA_Y)));
        }
        S16(st, f + FIG_CC_DELTA_Y) -= 1;
    }
}

void figure_line_step_x(game_state *st, int32_t id)
{
    /* @bin 0x004B43E0 +0x4A one unit towards +0x4E */
    uint32_t f = FIGURE_ADDR(id);
    int16_t x = S16(st, f + FIG_CC_X), t = S16(st, f + FIG_CC_DEST_X);
    if (x < t) {
        S16(st, f + FIG_CC_X) = (int16_t) (x + 1);
    } else if (x > t) {
        S16(st, f + FIG_CC_X) = (int16_t) (x - 1);
    }
}

void figure_line_step_y(game_state *st, int32_t id)
{
    /* @bin 0x004B43A0 +0x4C one unit towards +0x50 */
    uint32_t f = FIGURE_ADDR(id);
    int16_t y = S16(st, f + FIG_CC_Y), t = S16(st, f + FIG_CC_DEST_Y);
    if (y < t) {
        S16(st, f + FIG_CC_Y) = (int16_t) (y + 1);
    } else if (y > t) {
        S16(st, f + FIG_CC_Y) = (int16_t) (y - 1);
    }
}

void figure_route_direction(game_state *st, int32_t id)
{
    /* @bin 0x004B33D0 +0x11 from the route (slot +0x3A, step +0x3C of +0x3E; table 0x00C8AD38, 500
     * steps per slot): 8 at its end (the route released); without a route, the direction to the
     * target (+0x24/+0x26), 10 unless already there (8) */
    uint32_t f = FIGURE_ADDR(id);
    int16_t slot = S16(st, f + FIG_PATH_ID);
    if (slot <= 0) {
        uint8_t d = (uint8_t) GAME_CALL(st, FN_ROUTE_DIRECTION, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X),
                                        S16(st, f + FIG_DEST_Y));
        U8(st, f + FIG_DIRECTION) = d;
        if (d != 8) {
            U8(st, f + FIG_DIRECTION) = 0x0A;
        }
        return;
    }
    int16_t step = S16(st, f + FIG_PATH_STEP);
    if (step >= S16(st, f + FIG_PATH_LENGTH)) {
        figure_route_release(st, id);
        U8(st, f + FIG_DIRECTION) = 8;
        return;
    }
    U8(st, f + FIG_DIRECTION) = U8(st, G_FIGURE_PATHS + 500u * (uint32_t) slot + (uint32_t) step);
}

/* the terrain flag 0x40 (a road) on the tile next to `tile` in direction d (even directions only) */
static int road_towards(game_state *st, int32_t tile, int32_t d)
{
    if (d & 1) {
        return 0;
    }
    uint32_t t = (uint32_t) (direction_offset(st, (uint32_t) d) + tile);
    return (U8(st, G_GRID_TERRAIN + 4u * t) & 0x40) != 0;
}

void figure_turn_around(game_state *st, int32_t id)
{
    /* @bin 0x004B21D0 a roaming walker turns: from the direction to its target, the nearest road
     * clockwise and anticlockwise; the closer side wins (+0x48 the turn, +0x49 5 moves) */
    uint32_t f = FIGURE_ADDR(id);
    int32_t x = S16(st, f + FIG_X), y = S16(st, f + FIG_Y);
    int32_t tile = (int32_t) (U32(st, G_MAP_START) + (uint32_t) x + 228u * (uint32_t) y);
    int32_t c = (uint8_t) GAME_CALL(st, FN_ROUTE_DIRECTION, x, y, S16(st, f + FIG_DEST_X), S16(st, f + FIG_DEST_Y));
    if (c >= 8) {
        c = 0;
    }
    uint8_t cw = 0, acw = 4;
    int32_t ncw = 0, nacw;
    int32_t d = c, i;
    for (i = 0; i < 8; i++) {
        if (road_towards(st, tile, d)) {
            cw = (uint8_t) d;
            break;
        }
        d++;
        if (d > 7) {
            d = 0;
        }
        ncw++;
    }
    d = c;
    for (nacw = 0; nacw < 8; nacw++) {
        if (road_towards(st, tile, d)) {
            acw = (uint8_t) d;
            break;
        }
        d--;
        if (d < 0) {
            d = 7;
        }
    }
    if (ncw > nacw) {
        U8(st, f + FIG_DIRECTION) = acw;
        U8(st, f + FIG_U48) = 0xFE;
    } else {
        U8(st, f + FIG_DIRECTION) = cw;
        U8(st, f + FIG_U48) = 2;
    }
    U8(st, f + FIG_U49) = 5;
}

void figure_enter_next_tile(game_state *st, int32_t id)
{
    /* @bin 0x004B3C60 (jump tables 0x004B3EA4, 0x004B3EC4, 0x004B3EDC) the figure moves to the next
     * tile in direction +0x11: tile lists, +0x41 on a road (terrain 0x40 / 0x40000000), on a tile
     * with flag 4 the effect +0x77/+0x79 of the byte grid 0x00ADC36C; 0x004B1210 meets the tile's
     * first figure (not for types 0x4D, 0x4E, 0x5C, 0x5D, 0x64, 0x65); +0x18/+0x1A the old tile */
    uint32_t f = FIGURE_ADDR(id);
    int16_t ox = S16(st, f + FIG_X), oy = S16(st, f + FIG_Y);
    if (S8(st, f + FIG_DIRECTION) >= 8) {
        return;
    }
    figure_tile_list_remove(st, id);
    int8_t d = S8(st, f + FIG_DIRECTION);
    if ((uint32_t) (int32_t) d > 7) {
        return;
    }
    static const int8_t dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    static const int8_t dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
    if (dy[d] != 0) {
        S16(st, f + FIG_Y) += dy[d];
    }
    if (dx[d] != 0) {
        S16(st, f + FIG_X) += dx[d];
    }
    static const int32_t step[8] = {-228, -227, 1, 229, 228, 227, -1, -229};
    int32_t tile = S32(st, f + FIG_GRID_OFFSET) + step[d];
    S32(st, f + FIG_GRID_OFFSET) = tile;
    GAME_CALL(st, FN_FIGURE_TILE_LIST_ADD, id);
    tile = S32(st, f + FIG_GRID_OFFSET);
    uint32_t t = U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile);
    if (!(t & 0x40) && !(t & 0x40000000u)) {
        U8(st, f + FIG_ON_ROAD) = 0;
    } else {
        U8(st, f + FIG_ON_ROAD) = 1;
        uint8_t b = U8(st, G_SPRITE + (uint32_t) tile);
        if ((t & 4) && b != 0) {
            U8(st, f + FIG_HEIGHT_TICKS) = 0x12;
            uint8_t v;
            switch (b) {
            case 1: case 4: v = 0x0F; break;
            case 2: v = 0x1B; break;
            case 3: case 6: v = 0x0E; break;
            case 5: v = 0x1E; break;
            case 0x0D: v = 0x1E; break;
            case 7: case 8: case 9: case 0x0A: v = 0x0E; break;
            default: v = 0x24; break;
            }
            U8(st, f + FIG_HEIGHT_TARGET) = v;
        }
    }
    uint8_t type = U8(st, f + FIG_TYPE);
    if (!(type == 0x4D || type == 0x4E || type == 0x5C || type == 0x5D || type == 0x64 || type == 0x65)) {
        figure_meet(st, id, S16(st, G_FIGURE_GRID + 2u * (uint32_t) S32(st, f + FIG_GRID_OFFSET)));
    }
    S16(st, f + FIG_PREV_X) = ox;
    S16(st, f + FIG_PREV_Y) = oy;
}

uint8_t figure_route_slot(game_state *st, int32_t id, int32_t *slot)
{
    /* @bin 0x00489970 a route slot for the figure (*slot; the original writes it into its caller's
     * local): its own slot +0x3A when it still owns it (freed), else the first slot from 1 whose
     * owner is gone (a dead owner's +0x3A cleared); 0 when slots 1 .. 999 are all in use */
    uint32_t f = FIGURE_ADDR(id);
    int16_t s = S16(st, f + FIG_PATH_ID);
    if (s != 0) {
        if (S16(st, G_ROUTE_OWNERS + 2u * (uint32_t) s) == id) {
            *slot = s;
            int16_t v = S16(st, f + FIG_PATH_ID);
            S16(st, f + FIG_PATH_ID) = 0;
            S16(st, G_ROUTE_OWNERS + 2u * (uint32_t) v) = 0;
            return 1;
        }
        S16(st, f + FIG_PATH_ID) = 0;
    }
    *slot = 1;
    for (;;) {
        int32_t i = *slot;
        int16_t o = S16(st, G_ROUTE_OWNERS + 2u * (uint32_t) i);
        if (o == 0) {
            return 1;
        }
        if (o < 0 || o >= 2000) {
            break;
        }
        uint32_t g = FIGURE_ADDR(o);
        if (S16(st, g + FIG_PATH_ID) != i) {
            break;
        }
        if (U8(st, g + FIG_STATE) != 1) {
            S16(st, g + FIG_PATH_ID) = 0;
            break;
        }
        *slot = i + 1;
        if (i + 1 >= 1000) {
            *slot = 0;
            return 0;
        }
    }
    S16(st, G_ROUTE_OWNERS + 2u * (uint32_t) *slot) = 0;
    return 1;
}

void figure_find_route(game_state *st, int32_t id)
{
    /* @bin 0x004B2D20 a route from the figure's tile to its target (+0x24/+0x26) in a free slot:
     * by its terrain kind +0x72 (0x0051xxxx searches), or the free-walk searches when +0x76 is set;
     * then 0x0051B400 (4 / 8) stores it. +0x3A the slot, +0x3E its length, the slot's owner. */
    uint32_t f = FIGURE_ADDR(id);
    S16(st, f + FIG_PATH_ID) = 0;
    S16(st, f + FIG_PATH_STEP) = 0;
    S16(st, f + FIG_PATH_LENGTH) = 0;
    int32_t slot;
    if (figure_route_slot(st, id, &slot) == 0) {
        return;
    }
    int32_t r;
    uint8_t mode = U8(st, f + FIG_BOAT_MODE);
    if (mode != 0) {
        if (S32(st, G_EDITOR) == 1) {
            GAME_CALL(st, FN_ROUTE_WIDE_DISTANCES, S16(st, G_ENTRY_X), S16(st, G_ENTRY_Y));
        } else if (mode == 2) {
            GAME_CALL(st, FN_ROUTE_WIDE_DISTANCES, S16(st, f + FIG_X), S16(st, f + FIG_Y));
        } else {
            GAME_CALL(st, FN_ROUTE_DISTANCES_ED4140, S16(st, f + FIG_X), S16(st, f + FIG_Y));
        }
        if (U8(st, f + FIG_BOAT_MODE) == 2) {
            r = GAME_CALL(st, FN_ROUTE_PATH_TIES, slot, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X),
                          S16(st, f + FIG_DEST_Y), 1);
        } else if (U8(st, f + FIG_TYPE) == 0x4E && (U8(st, f + FIG_ACTION) == 9 || S32(st, f + FIG_U15C) == 9)) {
            r = GAME_CALL(st, FN_ROUTE_PATH_STRAIGHT, slot, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X),
                          S16(st, f + FIG_DEST_Y), S32(st, f + FIG_U140));
            if (r == 0) {
                r = GAME_CALL(st, FN_ROUTE_PATH_TIES, slot, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X),
                              S16(st, f + FIG_DEST_Y), 0);
            }
        } else {
            r = GAME_CALL(st, FN_ROUTE_PATH_TIES, slot, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X),
                          S16(st, f + FIG_DEST_Y), 0);
        }
        if (r == 0) {
            return;
        }
        goto found;
    }
    int full = 1; /* 0: the 4-way store first (0x004B3196) */
#define SEARCH(a) GAME_CALL(st, a, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X), S16(st, f + FIG_DEST_Y))
    switch (U8(st, f + FIG_TERRAIN_USAGE)) {
    case 2:
        r = GAME_CALL(st, FN_ROUTE_ARMY_DISTANCES, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X), S16(st, f + FIG_DEST_Y), 0,
                      100000);
        if (r != 0) {
            goto store8;
        }
        r = SEARCH(FN_ROUTE_FLOOD_EC7630);
        break;
    case 4:
        full = 0;
        r = route_boat_to(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X), S16(st, f + FIG_DEST_Y));
        break;
    case 5:
        r = SEARCH(FN_ROUTE_WALK_DISTANCES);
        break;
    case 3:
        r = route_walk_to(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X), S16(st, f + FIG_DEST_Y));
        if (r != 0) {
            goto store4;
        }
        r = route_walk_free_to(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X), S16(st, f + FIG_DEST_Y));
        break;
    case 6:
        r = route_water_to(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X), S16(st, f + FIG_DEST_Y));
        if (r != 0) {
            goto store8;
        }
        r = route_any_free_to(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X), S16(st, f + FIG_DEST_Y));
        break;
    case 1:
        full = 0;
        r = route_walk_to(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X), S16(st, f + FIG_DEST_Y));
        break;
    case 0:
        r = route_any_free_to(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X), S16(st, f + FIG_DEST_Y));
        break;
    case 9:
        r = route_even_to(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X), S16(st, f + FIG_DEST_Y));
        break;
    case 0x0A:
        r = route_free6_to(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X), S16(st, f + FIG_DEST_Y));
        break;
    case 7:
        r = route_or5_to(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X), S16(st, f + FIG_DEST_Y));
        if (r != 0) {
            goto store4;
        }
        r = route_not3_to(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X), S16(st, f + FIG_DEST_Y));
        break;
    case 0x0B:
        r = route_any_to(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X), S16(st, f + FIG_DEST_Y));
        break;
    default:
        return;
    }
#undef SEARCH
    if (r == 0) {
        return;
    }
    if (full) {
        goto store8;
    }
store4:
    r = route_store(st, 4, slot, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X), S16(st, f + FIG_DEST_Y));
    if (r > 0) {
        goto found;
    }
store8:
    r = route_store(st, 8, slot, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X), S16(st, f + FIG_DEST_Y));
    if (r == 0) {
        return;
    }
found:
    S16(st, f + FIG_PATH_ID) = (int16_t) slot;
    S16(st, f + FIG_PATH_LENGTH) = (int16_t) r;
    S16(st, G_ROUTE_OWNERS + 2u * (uint32_t) slot) = (int16_t) id;
}

int32_t figure_route_to_any(game_state *st, int32_t id, uint32_t list, int32_t n)
{
    /* @bin 0x004B28F0 like 0x004B2D20, towards the nearest of `n` grid offsets at `list` (a table of
     * the game's): the search returns the index (1 ..) of the one reached, which becomes the target
     * (+0x24/+0x26); returns that index (0: none, or the route length on the free-walk path) */
    uint32_t f = FIGURE_ADDR(id);
    S16(st, f + FIG_PATH_ID) = 0;
    S16(st, f + FIG_PATH_STEP) = 0;
    S16(st, f + FIG_PATH_LENGTH) = 0;
    int32_t slot;
    if (figure_route_slot(st, id, &slot) == 0) {
        return 0;
    }
    int32_t found, len;
    int full = 1;
    uint8_t mode = U8(st, f + FIG_BOAT_MODE);
    if (mode != 0) {
        if (S32(st, G_EDITOR) == 1) {
            GAME_CALL(st, FN_ROUTE_WIDE_DISTANCES, S16(st, G_ENTRY_X), S16(st, G_ENTRY_Y));
        } else if (mode == 2) {
            GAME_CALL(st, FN_ROUTE_WIDE_DISTANCES, S16(st, f + FIG_X), S16(st, f + FIG_Y));
        } else {
            GAME_CALL(st, FN_ROUTE_DISTANCES_ED4140, S16(st, f + FIG_X), S16(st, f + FIG_Y));
        }
        if (U8(st, f + FIG_BOAT_MODE) == 2) {
            len = GAME_CALL(st, FN_ROUTE_PATH_TIES, slot, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X),
                            S16(st, f + FIG_DEST_Y), 1);
        } else if (U8(st, f + FIG_TYPE) == 0x4E && (U8(st, f + FIG_ACTION) == 9 || S32(st, f + FIG_U15C) == 9)) {
            len = GAME_CALL(st, FN_ROUTE_PATH_STRAIGHT, slot, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X),
                            S16(st, f + FIG_DEST_Y), S32(st, f + FIG_U140));
            if (len == 0) {
                len = GAME_CALL(st, FN_ROUTE_PATH_TIES, slot, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X),
                                S16(st, f + FIG_DEST_Y), 0);
            }
        } else {
            len = GAME_CALL(st, FN_ROUTE_PATH_TIES, slot, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X),
                            S16(st, f + FIG_DEST_Y), 0);
        }
        found = len;
        if (len == 0) {
            return found;
        }
        goto store;
    }
#define SEARCH(a) GAME_CALL(st, a, S16(st, f + FIG_X), S16(st, f + FIG_Y), (int32_t) list, n)
    switch (U8(st, f + FIG_TERRAIN_USAGE)) {
    case 2:
        found = GAME_CALL(st, FN_ROUTE_ARMY_TO_ANY, S16(st, f + FIG_X), S16(st, f + FIG_Y), (int32_t) list, n, 0, 100000);
        if (found == 0) {
            found = route_ec7630_to_any(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), list, n);
        }
        break;
    case 4:
        full = 0;
        found = route_boat_to_any(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), list, n);
        break;
    case 5:
    case 0:
        found = route_any_free_to_any(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), list, n);
        break;
    case 3:
        found = route_walk_to_any(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), list, n);
        full = 0;
        if (found == 0) {
            full = 1;
            found = route_walk_free_to_marked(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), list, n);
        }
        break;
    case 1:
        full = 0;
        found = route_walk_to_any(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), list, n);
        break;
    case 9:
        found = route_even_to_any(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), list, n);
        break;
    case 0x0A:
        found = route_free6_to_any(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), list, n);
        break;
    default:
        return 0;
    }
#undef SEARCH
    if (found == 0) {
        return 0;
    }
    {
        /* @bin 0x004B2C2B the target tile from the grid offset (signed divisions by 228) */
        int32_t off = (int32_t) (P32(st, list + 4u * (uint32_t) found - 4u) - U32(st, G_MAP_START));
        S16(st, f + FIG_DEST_X) = (int16_t) (off % 228);
        S16(st, f + FIG_DEST_Y) = (int16_t) (off / 228);
    }
    if (!full) {
        len = route_store(st, 4, slot, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X),
                        S16(st, f + FIG_DEST_Y));
        if (len > 0) {
            goto store;
        }
    }
    len = route_store(st, 8, slot, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X),
                    S16(st, f + FIG_DEST_Y));
    if (len == 0) {
        return found;
    }
store:
    figure_route_release(st, id);
    S16(st, f + FIG_PATH_ID) = (int16_t) slot;
    S16(st, f + FIG_PATH_LENGTH) = (int16_t) len;
    S16(st, G_ROUTE_OWNERS + 2u * (uint32_t) slot) = (int16_t) id;
    return found;
}

void figure_move_hurried(game_state *st, int32_t id, int32_t pressed)
{
    /* @bin 0x004A5680 walking with a pace cycle +0x154 (0: double speed, at most 0x12; 1: 40 %;
     * 2: resting), +0x138 its ticks (200, or the type's +0x0C when pressed). When `pressed`,
     * +0x134 counts the tiles walked in one direction (+0x30/+0x32 the last tile) and the walker
     * slows to its speed after the type's +0x08 of them. Nothing once arrived (+0x11 8 on the
     * target tile). */
    uint32_t f = FIGURE_ADDR(id);
    if (S16(st, f + FIG_X) == S16(st, f + FIG_DEST_X) && S16(st, f + FIG_Y) == S16(st, f + FIG_DEST_Y) && U8(st, f + FIG_DIRECTION) == 8) {
        return;
    }
    uint32_t t = G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE);
    int32_t speed;
    switch (U8(st, f + FIG_U154)) {
    case 0:
        if (pressed) {
            S32(st, f + FIG_U138) += 1;
            if (S32(st, f + FIG_U138) >= S32(st, t - FIGTYPE_SPEED + FIGTYPE_U2C)) {
                S32(st, f + FIG_U138) = 0;
                U8(st, f + FIG_U154) = 2;
                return;
            }
            if (U8(st, f + FIG_PREV_DIRECTION) != U8(st, f + FIG_DIRECTION)) {
                S32(st, f + FIG_U134) = 0;
            }
            if (S16(st, f + FIG_X) != S16(st, f + FIG_U30) || S16(st, f + FIG_Y) != S16(st, f + FIG_U32)) {
                S16(st, f + FIG_U30) = S16(st, f + FIG_X);
                S16(st, f + FIG_U32) = S16(st, f + FIG_Y);
                S32(st, f + FIG_U134) += 1;
            }
            if (S32(st, f + FIG_U134) >= S32(st, t - FIGTYPE_SPEED + FIGTYPE_U28)) {
                figure_move(st, id, S32(st, t));
                return;
            }
        }
        speed = S32(st, t) * 2;
        if (speed >= 0x12) {
            speed = 0x12;
        }
        figure_move(st, id, speed);
        return;
    case 1:
        figure_move(st, id, calc_percentage(S32(st, t), 0x28));
        S32(st, f + FIG_U138) += 1;
        if (S32(st, f + FIG_U138) >= 200) {
            S32(st, f + FIG_U138) = 0;
            U8(st, f + FIG_U154) = 0;
        }
        return;
    case 2:
        S32(st, f + FIG_U138) += 1;
        if (S32(st, f + FIG_U138) >= 200) {
            S32(st, f + FIG_U138) = 0;
            U8(st, f + FIG_U154) = 1;
        }
        return;
    default:
        return;
    }
}

int32_t road_wear(game_state *st, int32_t kind, int32_t tile)
{
    /* @bin 0x0047E230 the 3-bit field `kind` of the tile's dword in 0x00978D9C */
    return (int32_t) ((U32(st, G_TILE_VISITS + 4u * (uint32_t) tile) >> ((uint32_t) (kind * 3) & 31u)) & 7u);
}

int32_t roam_open_directions(game_state *st, int32_t tile, int32_t fid)
{
    /* @bin 0x0047E340 the roads (terrain 0x440) around `tile` in 0x00BC97C0 (8 dwords, the even
     * directions set; gates of building types 0xAB, 0xCA, 0x88, and 0x8A for figures not of type 0x62
     * are closed); at a crossing of 3 or more only the least worn ones (0x0047E230 with the figure
     * type's field 0x005DBF70) stay; returns how many are open */
    uint8_t type = U8(st, FIGURE_ADDR(fid) + FIG_TYPE);
    int t62 = type == 0x62;
    static const int32_t step[4] = {-228, 1, 228, -1};
    static const uint32_t grid_step[4] = {G_BUILDING_GRID - 2u * 228u, G_BUILDING_GRID + 2u, G_BUILDING_GRID + 2u * 228u, G_BUILDING_GRID - 2u};
    S32(st, G_NEIGHBOUR_FLAGS + 0x1Cu) = 0;
    S32(st, G_NEIGHBOUR_FLAGS + 0x14u) = 0;
    S32(st, G_NEIGHBOUR_FLAGS + 0xCu) = 0;
    S32(st, G_NEIGHBOUR_FLAGS + 4u) = 0;
    for (int k = 0; k < 4; k++) {
        S32(st, G_NEIGHBOUR_FLAGS + 8u * (uint32_t) k) =
            (int32_t) (U32(st, G_GRID_TERRAIN + 4u * (uint32_t) (tile + step[k])) & 0x440u);
    }
    for (int k = 0; k < 4; k++) {
        if (!(U32(st, G_GRID_TERRAIN + 4u * (uint32_t) (tile + step[k])) & 8u)) {
            continue;
        }
        int16_t b = S16(st, grid_step[k] + 2u * (uint32_t) tile);
        int16_t bt = S16(st, BUILDING_ADDR(b) + BLD_TYPE);
        if (bt == 0xAB || bt == 0xCA || (bt == 0x8A && !t62) || bt == 0x88) {
            S32(st, G_NEIGHBOUR_FLAGS + 8u * (uint32_t) k) = 0;
        }
    }
    int32_t n = 0;
    for (int k = 0; k < 4; k++) {
        if (S32(st, G_NEIGHBOUR_FLAGS + 8u * (uint32_t) k) != 0) {
            n++;
        }
    }
    if (n < 3) {
        return n;
    }
    int32_t kind = S32(st, G_ROAD_WEAR_KIND + 4u * type);
    int32_t wear[4] = {0, 0, 0, 0};
    int32_t least = 7;
    for (int k = 0; k < 4; k++) {
        if (S32(st, G_NEIGHBOUR_FLAGS + 8u * (uint32_t) k) != 0) {
            wear[k] = road_wear(st, kind, S32(st, G_U5DC128 + 8u * (uint32_t) k) + tile);
            if (least >= wear[k]) {
                least = wear[k];
            }
        }
    }
    for (int k = 0; k < 4; k++) {
        if (S32(st, G_NEIGHBOUR_FLAGS + 8u * (uint32_t) k) != 0 && wear[k] > least) {
            S32(st, G_NEIGHBOUR_FLAGS + 8u * (uint32_t) k) = 0;
            n--;
        }
    }
    return n;
}

void neighbours_with_flags(game_state *st, int32_t mask)
{
    /* @bin 0x0047DB00 for the tile 0x00BC9768: 0x00BC97C0 (8 dwords, north then clockwise) 1 where the
     * neighbour's terrain has a bit of `mask` */
    static const int32_t step[8] = {-228, -227, 1, 229, 228, 227, -1, -229};
    int32_t tile = S32(st, G_TILE_CURSOR);
    for (int k = 0; k < 8; k++) {
        S32(st, G_NEIGHBOUR_FLAGS + 4u * (uint32_t) k) =
            (U32(st, G_GRID_TERRAIN + 4u * (uint32_t) (tile + step[k])) & (uint32_t) mask) != 0;
    }
}

int32_t neighbours_count(game_state *st)
{
    /* @bin 0x00484F40 the non-zero dwords of 0x00BC97C0 .. 0x00BC97DC */
    int32_t n = 0;
    for (int k = 0; k < 8; k++) {
        if (S32(st, G_NEIGHBOUR_FLAGS + 4u * (uint32_t) k) != 0) {
            n++;
        }
    }
    return n;
}

/* ---- the ferry queues (records of 0xE0 bytes at 0x00EAAF68: +4 head, +8 tail, +0x0C size, +0x14
 * the figures) ---- */

int32_t queue_push(game_state *st, uint32_t q, int32_t value)
{
    /* @bin 0x004F65B0 (thiscall; the original takes a pointer to the value) 0 when full */
    int32_t tail = S32(st, q + 8);
    int32_t next = tail + 1;
    if (next >= S32(st, q + 0x0C)) {
        next = 0;
    }
    if (next == S32(st, q + 4)) {
        return 0;
    }
    S32(st, q + 0x14 + 4u * (uint32_t) tail) = value;
    S32(st, q + 8) = next;
    return 1;
}

int32_t queue_length(game_state *st, uint32_t q)
{
    /* @bin 0x004F6630 (thiscall) */
    int32_t d = S32(st, q + 8) - S32(st, q + 4);
    return d < 0 ? S32(st, q + 0x0C) + d : d;
}

int32_t queue_full(game_state *st, uint32_t q)
{
    /* @bin 0x004F6650 (thiscall) */
    return queue_length(st, q) == S32(st, q + 0x0C) - 1;
}

void tile_wear(game_state *st, int32_t tile, int32_t limit)
{
    /* @bin 0x004690F0 the tile's count (0x00A9CC1C) goes up; past `limit` 0x00469140 */
    S16(st, G_GRID_DAMAGE + 2u * (uint32_t) tile) += 1;
    if (S16(st, G_GRID_DAMAGE + 2u * (uint32_t) tile) > limit) {
        int32_t off = tile - S32(st, G_MAP_START);
        GAME_CALL(st, FN_BUILDING_DESTROY_AT_0X469140, off % 228, off / 228, tile);
    }
}

int32_t ferry_on_route(game_state *st, int32_t id, int32_t b)
{
    /* @bin 0x004B3470 along the rest of the figure's route: 1 when it reaches the ferry (type 0x88)
     * linked to building b (+0xC4) and both are staffed (+0x40), 0 when not staffed, 2 when the
     * route does not pass it */
    uint32_t f = FIGURE_ADDR(id);
    int32_t step = S16(st, f + FIG_PATH_STEP);
    int32_t end = S16(st, f + FIG_PATH_LENGTH);
    int32_t tile = S32(st, f + FIG_GRID_OFFSET);
    uint32_t bb = BUILDING_ADDR(b);
    for (; step < end; step++) {
        int16_t t = S16(st, G_BUILDING_GRID + 2u * (uint32_t) tile);
        if (S16(st, BUILDING_ADDR(t) + BLD_TYPE) == 0x88 && t == S32(st, bb + BLD_FERRY_PARTNER)) {
            if (S16(st, bb + BLD_WORKERS) > 0 && S16(st, BUILDING_ADDR(S32(st, bb + BLD_FERRY_PARTNER)) + BLD_WORKERS) > 0) {
                return 1;
            }
            return 0;
        }
        uint8_t d = U8(st, G_FIGURE_PATHS + 500u * (uint32_t) S16(st, f + FIG_PATH_ID) + (uint32_t) step);
        tile += S32(st, G_DIRECTION_OFFSETS + 4u * d);
    }
    return 2;
}

/* @bin 0x004B372D the figure is put on the nearest free land (0x004AD880) */
static void relocate_on_land(game_state *st, int32_t id)
{
    uint32_t f = FIGURE_ADDR(id);
    int32_t x = S16(st, f + FIG_X), y = S16(st, f + FIG_Y);
    spiral_free_land(st, &x, &y);
    figure_tile_list_remove(st, id);
    S16(st, f + FIG_X) = (int16_t) x;
    S16(st, f + FIG_Y) = (int16_t) y;
    S32(st, f + FIG_GRID_OFFSET) = (int32_t) (U32(st, G_MAP_START) + 228u * (uint32_t) y + (uint32_t) x);
    U8(st, f + FIG_PROGRESS_ON_TILE) = 0x0F;
    GAME_CALL(st, FN_FIGURE_TILE_LIST_ADD, id);
}

void figure_check_next_tile(game_state *st, int32_t id)
{
    /* @bin 0x004B3530 (jump tables 0x004B3B64, 0x004B3BDC) the tile ahead (direction +0x11) by the
     * figure's terrain kind +0x72 / free walking +0x76: blocked (+0x11 9), or a road worn there
     * (+0x11 0x0B, +0x13 the old direction, 0x004690F0), or a ferry to board (action 7) */
    uint32_t f = FIGURE_ADDR(id);
    int8_t d = S8(st, f + FIG_DIRECTION);
    if (d >= 8 || U8(st, f + FIG_TERRAIN_USAGE) == 0x0B) {
        return;
    }
    int32_t next = S32(st, G_DIRECTION_OFFSETS + 4u * (uint32_t) (int32_t) d) + S32(st, f + FIG_GRID_OFFSET);
    int16_t b = S16(st, G_BUILDING_GRID + 2u * (uint32_t) next);
    uint32_t t = U32(st, G_GRID_TERRAIN + 4u * (uint32_t) next);
    uint32_t m = t & 0xEEFEC75Fu;
    if (U8(st, f + FIG_BOAT_MODE) != 0) {
        if ((m & 4) && S8(st, G_GRID_ROUTING_WATER + (uint32_t) next) >= 0) {
            return;
        }
        U8(st, f + FIG_DIRECTION) = 9;
        return;
    }
    uint8_t k = U8(st, f + FIG_TERRAIN_USAGE);
    if (k == 2) {
        int8_t v = S8(st, G_EC7630 + (uint32_t) next);
        if (v <= -1) {
            U8(st, f + FIG_DIRECTION) = 9;
            return;
        }
        if (v == 0 || v == 5 || (v == 2 && (t & 0x400))) {
            return;
        }
        U8(st, f + FIG_ATTACK_DIRECTION) = (uint8_t) d;
        U8(st, f + FIG_DIRECTION) = 0x0B;
        if ((U8(st, G_TICK) & 3) == 0) {
            tile_wear(st, next, v == 1 || v == 2 ? 10 : v == 3 ? 200 : 400);
        }
        return;
    }
    if (k == 4) {
        if (S8(st, G_EBAB20 + (uint32_t) next) > -1) {
            return;
        }
        U8(st, f + FIG_DIRECTION) = 9;
        return;
    }
    int8_t v = S8(st, G_GRID_ROUTING + (uint32_t) next);
    if (k == 6) {
        if (v != -1 || (t & 0x10004u)) {
            return;
        }
        U8(st, f + FIG_DIRECTION) = 9;
        return;
    }
    uint8_t type = U8(st, f + FIG_TYPE);
    if ((v == 1 || v == 3) && (type == 0 || (type > 3 && type != 0x5E))) {
        relocate_on_land(st, id);
        figure_route_release(st, id);
        U8(st, f + FIG_DIRECTION) = 9;
    }
    if ((m & 0x40) || (m & 0x40000400u)) {
        if (S32(st, G_UBCD5E8) == 0 || !(m & 8)) {
            return;
        }
        int16_t bt = S16(st, BUILDING_ADDR(b) + BLD_TYPE);
        if (bt == 0x3A || bt == 0xAA || bt == 0xAB || bt == 0xC8 || bt == 0xC9 || bt == 0xCA || bt == 0x8A) {
            U8(st, f + FIG_DIRECTION) = 9;
        }
        return;
    }
    if (m & 0xC000u) {
        U8(st, f + FIG_DIRECTION) = 9;
        return;
    }
    if (!(m & 8)) {
        if ((m & 4) || (m & 0x1000)) {
            U8(st, f + FIG_DIRECTION) = 9;
        }
        return;
    }
    if (U32(st, G_GRID_TERRAIN + 4u * (uint32_t) next) & 0x10000000u) {
        return;
    }
    uint32_t bb = BUILDING_ADDR(b);
    int16_t bt = S16(st, bb + BLD_TYPE);
    if (bt == 0x38 || (bt == 0x36 && U8(st, f + FIG_TERRAIN_USAGE) != 0) || (bt == 0xB7 && U8(st, f + FIG_TERRAIN_USAGE) == 7) || bt == 0xD1) {
        return;
    }
    if (bt != 0x88 || S32(st, G_UBCD5E8) != 0) {
        U8(st, f + FIG_DIRECTION) = 9;
        return;
    }
    /* @bin 0x004B3872 (jump table 0x004B3B64) the figure types that take ferries */
    switch (type) {
    case 1: case 2: case 3: case 0x5E:
        return;
    case 4: case 9: case 0x0B: case 0x0C: case 0x0D: case 0x0F: case 0x10: case 0x11: case 0x12: case 0x13:
    case 0x26: case 0x27: case 0x2A: case 0x49: case 0x4B: case 0x4F: case 0x50: case 0x51: case 0x55:
    case 0x5A: case 0x69: case 0x6C:
        break;
    default:
        U8(st, f + FIG_DIRECTION) = 9;
        return;
    }
    uint32_t q = G_FERRY_QUEUES + 0xE0u * U8(st, bb + BLD_UCA);
    int32_t other = S32(st, bb + BLD_FERRY_PARTNER);
    int32_t r = ferry_on_route(st, id, b);
    if (r == 2) {
        for (int i = 0; i < 3; i++) {
            figure_route_direction(st, id);
            U8(st, f + FIG_PREV_DIRECTION) = U8(st, f + FIG_DIRECTION);
            S16(st, f + FIG_PATH_STEP) += 1;
            figure_enter_next_tile(st, id);
            if (S8(st, f + FIG_DIRECTION) >= 8) {
                return;
            }
        }
        return;
    }
    if (r != 1) {
        U8(st, f + FIG_DIRECTION) = 9;
        return;
    }
    if (queue_full(st, q) == 0 && S16(st, bb + BLD_WORKERS) > 0 && S16(st, BUILDING_ADDR(other) + BLD_WORKERS) > 0) {
        if (U8(st, f + FIG_ACTION) == 7) {
            return;
        }
        queue_push(st, q, id);
        U8(st, f + FIG_ACTION_BEFORE_ATTACK) = U8(st, f + FIG_ACTION);
        U8(st, f + FIG_ACTION) = 7;
        figure_route_release(st, id);
        S16(st, FIGURE_ADDR(S16(st, bb + BLD_WALKER)) + FIG_WAIT_TICKS) = 0;
        return;
    }
    figure_route_release(st, id);
    relocate_on_land(st, id);
    switch (type) {
    case 4: case 9: case 0x0F: case 0x10: case 0x11: case 0x12: case 0x13: case 0x26: case 0x4B: case 0x4F:
    case 0x50: case 0x51: case 0x55: case 0x5A: case 0x69:
        U8(st, f + FIG_ACTION) = 8;
        return;
    case 0x0B: case 0x0C: case 0x0D: case 0x2A:
        U8(st, f + FIG_STATE) = 2;
        return;
    case 0x27:
        U8(st, f + FIG_ACTION) = 9;
        return;
    case 0x49:
        U8(st, f + FIG_ACTION) = 0x0E;
        return;
    default:
        return;
    }
}

/* @bin 0x004BAB96 .. 0x004BAE66 a walker's level +0x66 follows `v`: up to it at once, down by 10 */
static void level_towards(game_state *st, uint32_t f, int32_t v)
{
    uint8_t c = U8(st, f + FIG_SERVICE_MAX);
    if (v > c) {
        U8(st, f + FIG_SERVICE_MAX) = (uint8_t) v;
    } else if (c > 0x0A) {
        U8(st, f + FIG_SERVICE_MAX) = (uint8_t) (c - 0x0A);
    } else {
        U8(st, f + FIG_SERVICE_MAX) = 0;
    }
}

int32_t figure_serve_tile(game_state *st, int32_t id)
{
    /* @bin 0x004BAA10 a walker reaching a tile serves the houses around it, by figure type (the
     * service functions 0x00482xxx .. 0x00484xxx at its tile); what they count is added to its
     * building's +0x18 (at most 300); 0x0047E250 wears the road. Returns 0. */
    uint32_t f = FIGURE_ADDR(id);
    int32_t served = 0;
    uint8_t type = U8(st, f + FIG_TYPE);
    if (type == 0x28) {
        return 0;
    }
#define XY S16(st, f + FIG_X), S16(st, f + FIG_Y)
    uint32_t city = CURRENT_CITY(st);
    switch (type) {
    case 5: served = count_houses(st, XY); break;
    case 7:
        served = serve_houses_4e(st, XY);
        U8(st, f + FIG_SERVICE_MAX) = U8(st, G_SERVICE_TOP_HOUSES);
        break;
    case 0x1A: served = bazaar_sell(st, S16(st, f + FIG_HOME), XY); break;
    case 0x57: served = serve_houses_e90(st, XY); break;
    case 0x20:
        served = serve_houses(st, XY, 0x6E);
        lower_houses(st, XY, S32(st, G_U707EF8), 0xFC);
        break;
    case 0x1D: served = serve_houses(st, XY, 0x72); break;
    case 0x1E: served = serve_houses(st, XY, 0x73); break;
    case 0x1F: served = serve_houses(st, XY, 0x75); break;
    case 0x21:
        served = serve_houses(st, XY, 0x76);
        level_towards(st, f, lower_houses(st, XY, S32(st, G_U7065F8), 0xE8));
        break;
    case 0x62: inspect_houses(st, XY); break;
    case 0x22: served = serve_houses(st, XY, 0x78); break;
    case 0x40: served = clear_buildings_58(st, XY); break;
    case 0x1B: {
        int16_t b = S16(st, f + FIG_HOME);
        int32_t god = S32(st, city + CITY_U472C); /* @bin 0x004BAC4C 0x00EA5F9C */
        int32_t k = -1;
        switch (S16(st, BUILDING_ADDR(b) + BLD_TYPE)) {
        case 0x3C: case 0x41: k = 0; break;
        case 0x3D: case 0x42: k = 1; break;
        case 0x3E: case 0x43: k = 2; break;
        case 0x3F: case 0x44: k = 3; break;
        case 0x40: case 0x45: k = 4; break;
        default: break;
        }
        if (k >= 0) {
            served = serve_houses_god(st, XY, k);
        }
        if (S16(st, city + CITY_U4478) == 0) {
            break;
        }
        int32_t a = GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, god);
        if (GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, b) != a) {
            break;
        }
        a = GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, god);
        if (a == 1 || a == 3) {
            if ((uint8_t) GAME_CALL(st, FN_BUILDING_0XA9_IS, S32(st, CURRENT_CITY(st) + CITY_U472C), a == 1 ? 1 : 2) == 0) {
                break;
            }
            lower_buildings_e4(st, XY, S32(st, a == 1 ? G_U706918 : G_U706968));
            U8(st, f + FIG_SERVICE_MAX) = U8(st, G_SERVICE_TOP);
        } else if (a == 4) {
            if ((uint8_t) GAME_CALL(st, FN_BUILDING_0XA9_IS, S32(st, CURRENT_CITY(st) + CITY_U472C), 10) != 0) {
                clear_houses_87(st, XY);
            }
        }
        break;
    }
    case 8:
        served = lower_main_parts(st, XY, 0x46);
        level_towards(st, f, S32(st, G_SERVICE_TOP_46));
        break;
    case 0x0A:
        served = lower_main_parts(st, XY, 0x48);
        U8(st, f + FIG_SERVICE_MAX) = (uint8_t) served;
        break;
    case 0x58:
        lower_buildings_e4(st, XY, S32(st, G_U706760));
        U8(st, f + FIG_SERVICE_MAX) = U8(st, G_SERVICE_TOP);
        break;
    case 0x59:
        lower_buildings_e4(st, XY, S32(st, G_U707B88));
        serve_houses(st, XY, 0x70);
        U8(st, f + FIG_SERVICE_MAX) = U8(st, G_SERVICE_TOP);
        break;
    case 0x0F: served = serve_houses(st, XY, 0x6C); break;
    case 0x10:
    case 0x11: {
        /* @bin 0x004BAF0D the original passes 1 or 2 by the served building's +0xA8, which the
         * service functions (the code of 0x004833C0) do not read */
        served = serve_houses(st, XY, type == 0x10 ? 0x6D : 0x6F);
        break;
    }
    case 0x12: served = serve_houses(st, XY, 0x71); break;
    case 0x69: served = serve_houses(st, XY, 0x86); break;
    default: break;
    }
#undef XY
    int16_t home = S16(st, f + FIG_HOME);
    if (home != 0) {
        uint32_t h = BUILDING_ADDR(home) + BLD_SERVICE;
        S16(st, h) = (int16_t) (S16(st, h) + served);
        if (S16(st, h) > 300) {
            S16(st, h) = 300;
        }
    }
    road_wear_up(st, S32(st, G_ROAD_WEAR_KIND + 4u * U8(st, f + FIG_TYPE)), S32(st, f + FIG_GRID_OFFSET));
    return 0;
}

static int32_t abs_diff(int32_t a, int32_t b)
{
    return a > b ? a - b : a < b ? b - a : 0;
}

int32_t find_enemy_near(game_state *st, int32_t x, int32_t y, int32_t range, int32_t fid)
{
    /* @bin 0x004999E0 the nearest enemy of figure fid's company (+0x60; its target group +0x94 when
     * positive): figures 1 .. 1999 alive, not dying, of types 0x2B .. 0x39, 0x63, those 0x004EA9C0
     * accepts, or 0x29 in action 0x0B; within `range` (the larger axis distance, doubled when the
     * enemy is already fought (+0x84)); 0x00BCD5EC the distance found (10000: none) */
    int16_t company = S16(st, FIGURE_ADDR(fid) + FIG_FORMATION);
    int32_t best = 0;
    S32(st, G_BEST_DISTANCE) = 10000;
    for (int32_t i = 1; i < 2000; i++) {
        uint32_t g = FIGURE_ADDR(i);
        if (U8(st, g + FIG_STATE) != 1 || U8(st, g + FIG_ACTION) == 4) {
            continue;
        }
        uint8_t t = U8(st, g + FIG_TYPE);
        if (!((t >= 0x2B && t <= 0x39) || t == 0x63 || (uint8_t) GAME_CALL(st, FN_FIGURE_TYPE_IS_ENEMY_0X52, t) != 0 ||
              (U8(st, g + FIG_TYPE) == 0x29 && U8(st, g + FIG_ACTION) == 0x0B))) {
            continue;
        }
        int32_t dx = abs_diff(x, S16(st, g + FIG_X));
        S32(st, G_DIST_X) = dx;
        int32_t dy = abs_diff(y, S16(st, g + FIG_Y));
        S32(st, G_DIST_Y) = dy;
        int32_t d = dx < dy ? dy : dx;
        int32_t group = S32(st, G_FORMATION_BASE + FM_TARGET + 0x90u * (uint32_t) company);
        if (group > 0 && S16(st, g + FIG_FORMATION) != group) {
            continue;
        }
        if (d > range) {
            continue;
        }
        if (S16(st, g + FIG_TARGETED_BY) != 0) {
            d += d;
        }
        if (d < S32(st, G_BEST_DISTANCE)) {
            S32(st, G_BEST_DISTANCE) = d;
            best = i;
        }
    }
    return best;
}

/* @bin 0x004B131D the fighting classes (per type, 0x007090CC) that `mine` attacks */
static int attacks(int32_t mine, int32_t other, uint8_t other_action)
{
    switch (mine) {
    case 1: return other == 7;
    case 2: return other == 5 ? other_action == 0x0B : other == 4 || other == 3 || other == 7;
    case 3: return other == 1 || other == 2 || other == 4 || other == 7;
    case 4: return other == 2 || other == 3 || other == 7;
    case 6: return other == 7;
    case 7: return other == 1 || other == 2 || other == 3 || other == 4 || other == 6;
    default: return 0;
    }
}

void figure_meet(game_state *st, int32_t id, int32_t first)
{
    /* @bin 0x004B1210 (jump table 0x004B16A0) a fighting figure (class 1 .. 7 at 0x007090CC) entering
     * a tile with the list starting at `first`: the first enemy found there starts a fight (action 5
     * for both, +0x8B the attackers, +0x13 facing each other); some pairs end at once (a cart of
     * good 0x15 met by type 0x17, types 0x21 / 0x62, 0x1B / 0x62 under a god's blessing) */
    uint32_t f = FIGURE_ADDR(id);
    int32_t cls = S32(st, G_PROPS + FIGTYPE_U00 + 0x38u * U8(st, f + FIG_TYPE));
    if (cls <= 0 || cls >= 8) {
        return;
    }
    if (U8(st, f + FIG_ACTION) == 0x13) {
        GAME_CALL(st, FN_FIGURE_HIT_TARGET, id, first);
        return;
    }
    int32_t o = first;
    if (U8(st, FIGURE_ADDR(o) + FIG_ACTION) == 0x13 || U8(st, f + FIG_ACTION) == 5) {
        return;
    }
    for (int32_t n = 1; o > 0;) {
        uint32_t g = FIGURE_ADDR(o);
        if (o != id) {
            uint8_t ot = U8(st, g + FIG_TYPE);
            if (!(ot == 0x19 || ot == 0x4D || ot == 0x4E || ot == 0x5C || ot == 0x5D || ot == 0x64 || ot == 0x65)) {
                int match = U8(st, g + FIG_STATE) == 1 && U8(st, g + FIG_ACTION) != 4 &&
                            attacks(cls, S32(st, G_PROPS + FIGTYPE_U00 + 0x38u * ot), U8(st, g + FIG_ACTION));
                uint8_t t = U8(st, f + FIG_TYPE);
                uint32_t city = CURRENT_CITY(st);
                if (t == 0x17 && ot == 4 && U8(st, g + FIG_RESOURCE) == 0x15) {
                    U8(st, f + FIG_ACTION) = 0x0A;
                    S16(st, f + FIG_WAIT_TICKS) = 0;
                    U8(st, g + FIG_STATE) = 2;
                    S32(st, city + CITY_U4418) += 1; /* @bin 0x004B14FD 0x00EA5C88 */
                    return;
                }
                if (t == 4 && U8(st, f + FIG_RESOURCE) == 0x15 && ot == 0x17) {
                    U8(st, g + FIG_ACTION) = 0x0A;
                    S16(st, g + FIG_WAIT_TICKS) = 0;
                    U8(st, f + FIG_STATE) = 2;
                    return;
                }
                if (t == 0x21 && ot == 0x62) {
                    U8(st, g + FIG_STATE) = 2;
                    return;
                }
                if (t == 0x62 && ot == 0x21) {
                    U8(st, f + FIG_STATE) = 2;
                    return;
                }
                if (S16(st, city + CITY_U4478) != 0 && GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, S32(st, city + CITY_U472C)) == 4 &&
                    (uint8_t) GAME_CALL(st, FN_BUILDING_0XA9_IS, S32(st, CURRENT_CITY(st) + CITY_U472C), 1) != 0) {
                    if (U8(st, f + FIG_TYPE) == 0x1B && GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, S16(st, f + FIG_HOME)) == 4 &&
                        U8(st, g + FIG_TYPE) == 0x62) {
                        U8(st, g + FIG_STATE) = 2;
                        return;
                    }
                    if (U8(st, f + FIG_TYPE) == 0x62 && U8(st, g + FIG_TYPE) == 0x1B &&
                        GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, S16(st, g + FIG_HOME)) == 4) {
                        U8(st, f + FIG_STATE) = 2;
                        return;
                    }
                }
                if (match && (U8(st, g + FIG_ACTION) != 5 || U8(st, g + FIG_ATTACKERS) < 2)) {
                    U8(st, f + FIG_ACTION_BEFORE_ATTACK) = U8(st, f + FIG_ACTION);
                    U8(st, f + FIG_ACTION) = 5;
                    S16(st, f + FIG_OPPONENT) = (int16_t) o;
                    S16(st, f + FIG_ATTACKER1) = (int16_t) o;
                    U8(st, f + FIG_ATTACKERS) = 1;
                    U8(st, f + FIG_ATTACK_IMAGE) = 0x0C;
                    int32_t ox, oy;
                    if (S16(st, g + FIG_X) == S16(st, g + FIG_DEST_X) && S16(st, g + FIG_Y) == S16(st, g + FIG_DEST_Y)) {
                        ox = S16(st, g + FIG_X);
                        oy = S16(st, g + FIG_Y);
                    } else {
                        ox = S16(st, g + FIG_PREV_X);
                        oy = S16(st, g + FIG_PREV_Y);
                    }
                    uint8_t d = (uint8_t) GAME_CALL(st, FN_ROUTE_DIRECTION, S16(st, f + FIG_PREV_X), S16(st, f + FIG_PREV_Y), ox, oy);
                    U8(st, f + FIG_ATTACK_DIRECTION) = d;
                    if ((int8_t) d >= 8) {
                        U8(st, f + FIG_ATTACK_DIRECTION) = 0;
                    }
                    if (U8(st, g + FIG_ACTION) != 5) {
                        U8(st, g + FIG_ACTION_BEFORE_ATTACK) = U8(st, g + FIG_ACTION);
                        U8(st, g + FIG_ACTION) = 5;
                        U8(st, g + FIG_ATTACK_IMAGE) = 0;
                        uint8_t back = (uint8_t) (U8(st, f + FIG_ATTACK_DIRECTION) + 4);
                        U8(st, g + FIG_ATTACK_DIRECTION) = back;
                        if ((int8_t) back >= 8) {
                            U8(st, g + FIG_ATTACK_DIRECTION) = (uint8_t) (back - 8);
                        }
                    }
                    if (U8(st, g + FIG_ATTACKERS) == 0) {
                        S16(st, g + FIG_ATTACKER1) = (int16_t) id;
                        S16(st, g + FIG_OPPONENT) = (int16_t) id;
                        U8(st, g + FIG_ATTACKERS) = 1;
                    } else if (U8(st, g + FIG_ATTACKERS) == 1) {
                        S16(st, g + FIG_ATTACKER2) = (int16_t) id;
                        U8(st, g + FIG_ATTACKERS) = 2;
                    }
                    return;
                }
            }
        }
        o = S16(st, g + FIG_NEXT_ON_TILE);
        if (++n >= 1000) {
            return;
        }
    }
}
