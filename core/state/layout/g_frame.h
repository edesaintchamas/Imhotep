/* Imhotep - named globals: core/state/layout/g_frame.h.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md (owner: package P7). */
#ifndef IMHOTEP_STATE_LAYOUT_G_FRAME_H
#define IMHOTEP_STATE_LAYOUT_G_FRAME_H

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
#define G_DEBUG_OFF 0x00578C68u /* core/frame/wndproc.c: u8 1 in the exe: the debug keys are off */
#define G_PAGE 0x00605A44u /* present/ui/advisors/advisors.h: i32 the advisor page shown in window 7 (1-14) */
#define G_ANIM_FLAGS 0x00605B38u /* present/city/anim.c: u8 per timer, raised by 0x0040F270 */
#define G_TILE_H 0x0070E44Cu /* present/city/city.h: i32 the row step in pixels */
#define G_U70E458 0x0070E458u /* present/city/city.h: i32 (+ 2: the column step in pixels) [unresolved] */
#define G_VIEW_ROWS 0x0070E460u /* core/command/rotate.c: i32 view size (0x00430CB0) */
#define G_VIEW_COLS 0x0070E464u /* core/command/rotate.c: i32 view size (0x00430CA0) */
#define G_U70E474 0x0070E474u /* core/frame/view.c [unresolved] */
#define G_U70E478 0x0070E478u /* core/frame/view.c [unresolved] */
#define G_U70E48C 0x0070E48Cu /* present/city/city.h: i32 screen y of the first row (+ 9 tile heights) [unresolved] */
#define G_U70E490 0x0070E490u /* present/city/city.h: i32 screen x of the first column [unresolved] */
#define G_TIP_KIND 0x00D3A6C0u /* present/sidebar/tooltip.c: i32 */
#define G_TIP_WINDOW 0x00D3A6C4u /* present/sidebar/tooltip.c: i32 the window it was asked in */
#define G_TIP_ID 0x00D3A6E4u /* present/sidebar/tooltip.c: i32 the tooltip's text (group 0x44), 0: none */
#define G_HANDLE_DDRAW 0x00DD0001u /* core/frame/display.c */
#define G_HANDLE_PRIMARY 0x00DD0002u /* core/frame/display.c */
#define G_HANDLE_CLIPPER 0x00DD0003u /* core/frame/display.c */
#define G_TEXT_BUFFER 0x00E36C70u /* core/city/text_lookup.c: pointer to the text from byte 0x1F5C of the file (0x004CEC40) */
#define G_SCREEN_PITCH 0x00E38D94u /* present/ui/info/figure.c: i32 the back buffer's pitch in bytes */
#define G_UE38D98 0x00E38D98u /* present/sidebar/sidebar.h: i32, the same value (set together by 0x004CF960) [unresolved] */
#define G_SCREEN_W 0x00E38DB0u /* core/frame/display.c: the mode's width and height */
#define G_PRESENT_NOW 0x00E38DC8u /* core/frame/frame.c: u8 present at the next present call */
#define G_SCROLLING 0x00E38DCCu /* core/frame/frame.c: i32 the map scrolls (0x004CF660) */
#define G_RIGHT_PRESSED 0x00E38DF7u /* present/ui/ui.h: u8 */
#define G_RIGHT_RELEASED 0x00E38DF8u /* present/ui/ui.h: u8 */
#define G_LEFT_HELD 0x00E38E00u /* present/ui/ui.h: u8 */
#define G_LEFT_LATCH 0x00E38E02u /* present/sidebar/input.c */
#define G_LEFT_PRESSED 0x00E38E03u /* present/ui/ui.h: u8 */
#define G_LEFT_RELEASED 0x00E38E04u /* present/ui/ui.h: u8 */
#define G_BUTTONS 0x00E38E07u /* core/frame/wndproc.c: u8 bit 0 left, bit 1 right */
#define G_MOUSE_Y 0x00E38E30u /* present/ui/ui.h: i32 */
#define G_MOUSE_X 0x00E38E34u /* present/sidebar/tooltip.c: i32 the cursor on the screen */
#define G_FULLSCREEN 0x00E38E5Cu /* core/frame/display.c: i32 the option: fullscreen (1) or windowed */
#define G_GAME_SPEED 0x00E38E6Cu /* present/ui/frontend/options.c: i32 */
#define G_DEBUG_GRID 0x00E38E94u /* present/city/marks.c: i32 which grid the debug display shows */
#define G_CITY_SOUNDS_ON 0x00E38EC0u /* core/frame/sound.c: u8 option: city sounds */
#define G_MUSIC_VOLUME 0x00E38EC8u /* present/ui/frontend/options.c: i32 */
#define G_CITY_VOLUME 0x00E38ED0u /* present/ui/frontend/options.c: i32 */
#define G_CLIENT_RECT 0x00E39130u /* core/frame/display.c: left, top, right, bottom of the client area on the screen */
#define G_DDRAW 0x00E39148u /* core/boot/display.c */
#define G_DD_BACK 0x00E39174u /* core/boot/display.c */
#define G_DD_PRIMARY 0x00E39178u /* core/boot/display.c */
#define G_SOUND 0x00E92BF8u /* core/boot/sound.c: the sound object */
#define G_PIXEL_FORMAT 0x00E92DFCu /* core/frame/display.c: 0: RGB 555, 2: RGB 565 (1, 3: 556, 655, never kept) */
#define G_BACK_BUFFER 0x00E92E18u /* present/draw/mem.h: u32 address of the back buffer (16-bit pixels) */
#define G_CTRL 0x00E92E20u /* core/frame/wndproc.c: i32 Ctrl held (WM_KEYDOWN / WM_KEYUP) */
#define G_SHIFT 0x00E92E24u /* core/frame/wndproc.c */
#define G_FILE_SCHEMA 0x00E92E50u /* core/frame/autosave.c: chunk 6: 0x5DC dwords */
#define G_LAYOUT_MODE 0x00E94D98u /* present/ui/frontend/options.c */
#define G_OVERLAY 0x00E94E0Cu /* present/city/city.h: i32 the overlay shown (0: none) */
#define G_INFO_WARSHIP 0x00EADCC4u /* present/ui/info/info.h: i32 the warship figure (kind 5) */
#define G_AMBIENT_TOTAL 0x00F55B84u /* present/city/ground.c */
#define G_SOUND_ON 0x00F55B88u /* core/sim/platform.c: sound system initialised (0x005394F0: the start-up's result) */
#define G_WORKSPACE 0x01265188u /* core/frame/autosave.c: the compressor's globals: output pointer, size, input size, crc */
/* --- end harvested --- */

#endif
