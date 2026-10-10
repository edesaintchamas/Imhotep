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

#include "present_layout.h"
#include "state/state.h"

/* @bin the window's context (written by 0x004F7E50, read by every panel; docs/commands.md 1.4): named in
 * present/layout/g_ui_info.h (G_INFO_*) and the other layout headers */

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
