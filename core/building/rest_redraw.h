/* Imhotep - building footprints drawn again by type (package rest).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BUILDING_REST_REDRAW_H
#define IMHOTEP_BUILDING_REST_REDRAW_H

#include "state/state.h"

int32_t god_index_4c53d0(game_state *st, int32_t v);                                   /* @bin 0x004C53D0 */
void building_redraw_4bfb30(game_state *st, int32_t b);                               /* @bin 0x004BFB30 */
void walls_rect_47afd0(game_state *st, int32_t x, int32_t y, int32_t size);        /* @bin 0x0047AFD0 */
void building_redraw_47d1f0(game_state *st, int32_t b, int32_t x, int32_t y, int32_t img); /* @bin 0x0047D1F0 */
void building_redraw_440540(game_state *st, int32_t b, int32_t x, int32_t y);         /* @bin 0x00440540 */
void building_area_redraw_440060(game_state *st, int32_t off, int32_t type, int32_t b); /* @bin 0x00440060 */
void tile_redraw_4c0860(game_state *st, int32_t x, int32_t y, int32_t group, int32_t add); /* @bin 0x004C0860 */
void building_area_redraw_4c0be0(game_state *st, int32_t b);                          /* @bin 0x004C0BE0 */
void building_redraw_4c0d50(game_state *st, int32_t b, int32_t add);                  /* @bin 0x004C0D50 */
void mastaba_redraw_4ee4a0(game_state *st, int32_t b);                                /* @bin 0x004EE4A0 */
void mastabas_redraw_4ee560(game_state *st);                                          /* @bin 0x004EE560 */

/* @bin 0x004DED50 the part's layout (13 words; the original's output is its caller's stack) */
void monument_part_layout_4ded50(game_state *st, int32_t out[13], int32_t part, int32_t orient, int32_t dir);
/* @bin 0x004DF360 a mastaba-like part's image; *flags (the original's output in its caller's stack) */
int32_t monument_part_image_4df360(game_state *st, int32_t b, int32_t *flags);
void monument_part_place_4dfd90(game_state *st, int32_t b);                           /* @bin 0x004DFD90 */

#endif
