/* Imhotep - the display mode, the window's messages and WinMain's end (core/frame/display.c).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_FRAME_DISPLAY_H
#define IMHOTEP_FRAME_DISPLAY_H

#include <stdint.h>

#include "state/state.h"

/* blocks outside the state (sim_platform.memory): size > 0 a new block's address (0: none), size 0
 * the block at `address` released; display_has_memory: the platform gives such blocks */
uint32_t display_memory(game_state *st, uint32_t address, uint32_t size);
int display_has_memory(game_state *st);

/* @bin 0x004CFD60 the mode's size from the resolution 1..3 */
void display_resolution_set(game_state *st, int32_t mode);
/* @bin 0x004CFDC0 the resolution and the drawing globals made from it */
void display_mode_set(game_state *st, int32_t mode);
/* @bin 0x00414BD0 the sidebar, the spare screen and the view after a resolution change */
void display_buffers_layout(game_state *st);
/* @bin 0x00402DA0 the confirmation window 0x3E for a resolution not confirmed yet */
void display_resolution_check(game_state *st);
/* @bin 0x00426EE0 the surfaces released (all: the DirectDraw object too) */
int32_t display_surfaces_release(game_state *st, int32_t all);
/* @bin 0x004D01E0 DirectDraw closed */
void display_end(game_state *st, int32_t all);
/* @bin 0x00426F60 the buffers filled with colour 0 */
int32_t display_clear(game_state *st);
/* @bin 0x004D0060 DirectDraw set up for the current mode (through platform_set_display) */
int32_t display_setup(game_state *st);
/* @bin 0x004D0220 gfxBegin: closed, set up, cleared, ready */
int32_t display_begin(game_state *st);
/* @bin 0x00414E8A the window procedure's WM_MOVE / WM_SIZE */
void display_window_moved(game_state *st);
/* @bin 0x00414F86 the window procedure's WM_ACTIVATEAPP (wParam: active) */
void display_window_activated(game_state *st, uint32_t active);
/* @bin 0x00414750 the options saved to Pharaoh.inf and Pharaohmap.inf, buffers freed */
void display_settings_save(game_state *st);
/* @bin 0x004E63B0 objects deleted at the end */
void display_quit_cleanup(game_state *st);
/* @bin 0x004CFF50 the Bink player's object deleted */
void display_bink_release(game_state *st);
/* @bin 0x005391E0 a Smacker video closed */
void display_video_stop(game_state *st);

#endif
