/* Imhotep - the land / grass sub-pass of the terrain images over any rectangle, and its helpers
 * (package bigb2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_MAP_TERRAIN_LAND_H
#define IMHOTEP_MAP_TERRAIN_LAND_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x0046F9A0 (x, y, d): inside the map diamond but within d tiles of its border */
int32_t map_near_edge(game_state *st, int32_t x, int32_t y, int32_t d);

/* @bin 0x0047DB00 the 8 neighbour flags of the cursor tile 0x00BC9768 (terrain & mask) into the
 * scratch 0x00BC97C0 */
void land_neighbour_flags(game_state *st, uint32_t mask);

/* @bin 0x00484BE0 (records, count, rnd, backward) neighbour-pattern match on the scratch */
int32_t land_pattern_match(game_state *st, uint32_t records, int32_t count, int32_t rnd, int32_t backward);

/* @bin 0x0047C880 (x, y, w, h, mask, need, strict) a w x h block fits */
int32_t land_block_fits(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t mask, int32_t need,
                        int32_t strict);

/* @bin 0x0047C140 (building, x, y, w, h, image, flag) a w x h block of consecutive images */
void land_block_place(game_state *st, int16_t building, int32_t x, int32_t y, int32_t w, int32_t h, int32_t image,
                      uint32_t flag);

/* @bin 0x004F3D70 a pyramid tile's state (-1: none) */
int32_t land_pyramid_tile_state(game_state *st, int32_t offset);

/* @bin 0x004F3F70 (building, tile, state, redo) a monument tile's image */
int32_t land_monument_tile_image(game_state *st, int32_t building, int32_t offset, int32_t state, int32_t redo);

/* @bin 0x004F60D0 a pyramid joins the redraw list 0x00EAAC30 */
void land_pyramid_register(game_state *st, int32_t building);

/* @bin 0x004F6130 redraw the listed pyramids, empty the list */
void land_pyramids_redraw(game_state *st);

/* @bin 0x004767D0 (x0, y0, x1, y1, keep, exact) the land / grass images over a rectangle */
void terrain_land(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t keep, int32_t exact);

/* @bin 0x004746D0 (x, y, size) terrain_land around a square, 5 tiles of margin */
void terrain_land_around(game_state *st, int32_t x, int32_t y, int32_t size);

#endif
