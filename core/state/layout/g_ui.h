/* Imhotep - named globals: the window system's state that the simulation reads or writes.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md. */
#ifndef IMHOTEP_STATE_LAYOUT_G_UI_H
#define IMHOTEP_STATE_LAYOUT_G_UI_H

/* @bin 0x004E0CF0 the message post reads and resets them */
#define G_WINDOW 0x00E38DDAu          /* u8: the current window id (1: the city) */
#define G_REDRAW 0x00E38DCAu          /* u8: redraw everything */
#define G_REDRAW_SIDEBAR 0x00E38DC9u  /* u8 */
#define G_FAST_FORWARD 0x00E3908Cu    /* u8 set by the day cascade 0x00452370 */
#define G_POPUP_DELAY 0x00EA0DA8u     /* i32: frames before the next pop-up */
#define G_FRAME_CLOCK 0x00E92E0Cu     /* @bin 0x004E2970 i32: the frame clock */
#define G_FONT_LARGE 0x00F56F8Cu      /* @bin 0x004E2970 i32: first image of the large font */
#define G_EDITOR 0x00605A3Cu          /* @bin 0x004E2970 i32: 1 in the map editor */
#define G_VIDEO_STATE 0x00F54118u     /* @bin 0x00538DC0 i32: videos on; 0x004E0CF0 := 1 for a message video */
#define G_UNDO_POSSIBLE 0x00BC9694u   /* @bin 0x00457AF0 i32 := 0 when the routing grids are rebuilt */
#define G_U5C755F 0x005C755Fu         /* @bin 0x004E0CF0 u8 := 1 for a message posted without a pop-up */

/* the text files (docs/formats/state.md "Game data outside the segments") */
#define G_TEXT_CURSOR 0x00E20768u     /* @bin 0x004CC110 u32: the text found */
#define G_MM_TEXT 0x00E36C74u         /* u32: the Pharaoh_MM.eng block from byte 0x13898 */

/* ---- names merged from the packages' pending fragments (phase 10.4): each group keeps its
 * package's heading (the package and its file in brackets) ---- */
/* formations (save.md §3 chunks 25-27) [p6_scenario] */
#define G_U78D7E0 0x0078D7E0u /* @bin 0x00451BE0 i32: := 0 with the date; the debug panel's "error" (present/ui/debug_panels.c) [g_ui.h] */
#define G_DEBUG_EVOLVE 0x0078D7DCu /* @bin 0x00534DF0 i32: debug switch, 1: the evolve counters panel [g_ui.h] */
#define G_DEBUG_TILE 0x0078D7D8u /* @bin 0x00535080 i32: debug switch, 1: the tile under the mouse panel [g_ui.h] */
#define G_DEBUG_FIGURE 0x0078D7D4u /* @bin 0x00536120 i32: debug switch, 1: the figure under the mouse panel [g_ui.h] */

#endif
