/* Imhotep - window 9: the building and figure info panels (roadmap phase 7.6, package "info").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Ports of Pharaoh.exe's info window: the opening click 0x004F7E50 (from the input dispatch
 * 0x004D8310 of the city view), the full redraw 0x004F8E60 (from 0x004D71C0), the per-frame draw
 * 0x004F94A0 (from 0x004D75D0), the input 0x004F96E0 (from 0x004D8310) and the panels they reach,
 * one per building type, figure type, formation, ship and terrain kind (docs/commands.md 1.4,
 * docs/presentation.md 3.3). The panels draw through the primitives of present/draw/ (panels,
 * layout texts, images, numbers) and write the window's context globals below, which the input
 * reads (the drawing builds the storage lists the order buttons use). */
#ifndef IMHOTEP_PRESENT_UI_INFO_H
#define IMHOTEP_PRESENT_UI_INFO_H

#include <stdint.h>

#include "state/state.h"

/* @bin the window's context (written by 0x004F7E50, read by every panel; docs/commands.md 1.4) */
#define INFO_BUILDING 0x00EADD6Cu    /* i32 the building shown (0: none) */
#define INFO_KIND 0x00EADD50u        /* i32 0 ?, 1 terrain or figures, 2 building, 4 legion, 5 warship, 6 transport */
#define INFO_FORMATION 0x00EADCCCu   /* i32 the formation (kinds 4..6: the panel's argument) */
#define INFO_TRANSPORT 0x00EADCC8u   /* i32 the transport ship figure (kind 6) */
#define INFO_WARSHIP 0x00EADCC4u     /* i32 the warship figure (kind 5) */
#define INFO_ORDERS 0x00EADCC0u      /* i32 1: the storage orders sub-panel is shown */
#define INFO_SOUND 0x00EADD94u       /* i32 1 until the first draw (the building's sound plays once) */
#define INFO_X 0x00EADD90u           /* i32 the panel's x */
#define INFO_Y 0x00EADD8Cu           /* i32 the panel's y */
#define INFO_W 0x00EADD88u           /* i32 the panel's width in 16-pixel blocks */
#define INFO_H 0x00EADD84u           /* i32 the panel's height in 16-pixel blocks */
#define INFO_TILE 0x00EADD80u        /* i32 grid offset of the clicked tile (the building's for a building) */
#define INFO_TILE_X 0x00EADD7Cu      /* i32 */
#define INFO_TILE_Y 0x00EADD78u      /* i32 */
#define INFO_TERRAIN 0x00EADD68u     /* i32 the tile's terrain image byte (0x00A9010C grid) */
#define INFO_TILE_BYTE 0x00EADD60u   /* i32 the tile's 0x00AE8E7C byte */
#define INFO_TILE_FLAG 0x00EADD64u   /* i32 the tile's 0x00AF598C bit 0x80 */
#define INFO_ROAD 0x00EADD58u        /* i32 1: the building has road access */
#define INFO_WORKERS 0x00EADD5Cu     /* i32 the building's workers in percent of its need (0x004CDE60) */
#define INFO_HELP 0x00EADD54u        /* i32 the help message the help button opens (0x00511C50) */
#define INFO_TERRAIN_KIND 0x00EADD4Cu /* i32 the terrain kind of kind 1 (no figures): its text */
#define INFO_FIGURE_VIEW 0x00EADD48u /* i32 1: the figure list is shown (kind 1 with figures) */
#define INFO_DISTRIBUTION 0x00EADD44u /* i32 bazaar / storage view toggle (0x004062D0) */
#define INFO_FIGURE_COUNT 0x00EADD40u /* i32 figures listed in INFO_FIGURES (0..7) */
#define INFO_FIGURES 0x00EADD24u     /* i32[7] the figures on the tile */
#define INFO_FIGURE_ROW 0x00EADCE4u  /* i32 the selected figure of the list */
#define INFO_BUTTON_HOVER 0x007056C8u /* i32 the hovered rectangle button of the window (+1) */
#define INFO_BUTTON_HOVER2 0x007056C4u /* i32 the hovered rectangle button of a second table (+1) */
#define STORAGE_ROWS 0x00F53DD8u     /* i32 count + 0x00F53DDC[] resource ids of the order rows */
#define STORAGE_LIST 0x00F53E6Cu     /* i32 count + 0x00F53E70[] resource ids (storage yard list) */

#define INFO_FONT_LARGE 0x00F56FB4u  /* i32 the title font */
#define INFO_FONT_TEXT 0x00F56F8Cu   /* i32 the text font */

/* the window's draw and input */
void info_draw(game_state *st);                 /* @bin 0x004F8E60 */
void info_input(game_state *st);                /* @bin 0x004F96E0 */
void info_frame(game_state *st);                /* @bin 0x004F94A0 */
int32_t info_open(game_state *st);              /* @bin 0x004F7E50 */
int32_t info_panel_size(game_state *st);        /* @bin 0x004F8960 */
int32_t info_building_part_of(game_state *st, int32_t b); /* @bin 0x004F6000 */
int32_t info_yard_fullness(game_state *st, int32_t b);    /* @bin 0x0045D570 */
int32_t info_any_legion_ready(game_state *st);            /* @bin 0x004B7590 */
/* the workers line of a building panel at y (0x004FC5C0), or the closed notice (0x004FC4F0) */
void info_workers_line(game_state *st, int32_t y);
void info_workers_line_or_closed(game_state *st, int32_t y);

#endif
