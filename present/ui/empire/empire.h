/* Imhotep - the empire map windows 0x14 (player), 0x15 / 0x16 (roadmap phase 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Ports of Pharaoh.exe's empire map: its frame (0x0052D630), the map with its objects, routes and
 * armies (0x0052F8B0), the bottom panel of the selected city (0x0052DA40, 0x0052DB10) and of a
 * selected army (0x0052E610), the title (0x0052D940), the arrow-key scrolling (0x00442730). Opening
 * a trade route is core's (core/command/trade.c). */
#ifndef IMHOTEP_PRESENT_UI_EMPIRE_H
#define IMHOTEP_PRESENT_UI_EMPIRE_H

#include <stdint.h>

#include "state/state.h"

/* @bin the empire map's globals */
#define EMP_SCROLL_Y 0x0077590Cu    /* i32 map scroll (pixels) */
#define EMP_SCROLL_X 0x00775910u    /* i32 */
#define EMP_SELECTED 0x00784F14u    /* i32 the selected object + 1 (0: none) */
#define EMP_CITY 0x00784F18u        /* i32 the selected object's city */
#define EMP_ARMY_SEL 0x005D2E88u    /* i32 the hovered / selected army marker (-1: none) */
#define EMP_MAP_X 0x005D2A18u       /* i32 the map's place on the screen: left */
#define EMP_MAP_Y 0x005D2A1Cu       /* i32 top */
#define EMP_MAP_BOTTOM 0x005D2A20u  /* i32 room below */
#define EMP_MAP_W 0x005D2A10u       /* i32 the map picture's size */
#define EMP_MAP_H 0x005D2A14u
#define EMP_OBJECTS 0x0077F748u     /* 200 objects of 0x62 bytes */
#define EMP_CITIES 0x0077DE00u      /* cities of 0x6A bytes */
#define EMP_MARKERS 0x00F535D0u     /* 100 army markers of 5 i32: event, x, y, w, h */
#define EMP_MARKER_COUNT 0x00F53FDCu

void empire_draw(game_state *st);            /* @bin 0x0052D4D0 */
void empire_frame_draw(game_state *st);      /* @bin 0x0052D630 */
void empire_panel_draw(game_state *st);      /* @bin 0x0052DA40 */
void empire_city_panel_draw(game_state *st); /* @bin 0x0052DB10 */
void empire_army_panel_draw(game_state *st); /* @bin 0x0052E610 */
void empire_title_draw(game_state *st);      /* @bin 0x0052D940 */
void empire_map_draw(game_state *st);        /* @bin 0x0052F8B0 */
void empire_scroll_key(game_state *st, int32_t direction); /* @bin 0x00442730 */
int32_t empire_city_of_object(game_state *st, int32_t object); /* @bin 0x00444920 */
int32_t image_anim_step(game_state *st, int32_t id, int32_t frame); /* @bin 0x0052F790 */

#endif
