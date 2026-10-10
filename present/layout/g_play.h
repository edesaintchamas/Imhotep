/* Imhotep - named globals: present/layout/g_play.h (present/play, present/boot).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md (owner: package P7). */
#ifndef IMHOTEP_PRESENT_LAYOUT_G_PLAY_H
#define IMHOTEP_PRESENT_LAYOUT_G_PLAY_H

/* ---- button panels, the on-screen notices, the network info (present/play/play.c) ---- */
#define G_BUTTON_STYLE 0x00F53F10u      /* @bin 0x00536DC0 i32[]: the panels' style by kind */
#define G_BUTTON_PICTURES 0x005F81E4u   /* @bin 0x00536DC0 3 dwords per style 5..14: image offset, dx, dy */
#define G_UF53F28 0x00F53F28u           /* @bin 0x00536FD0 i32 := 100 (0x00520B20 too) */
#define G_SCROLL_IMAGES 0x00F56494u     /* @bin 0x00536DC0 i32: first image of the panels' pieces (scroll bar group) */
#define G_SIDE_IMAGES 0x00F5684Cu       /* @bin 0x004E2773 i32: first image of the panels' icons (+0xF, +0x10 the marks) */
#define G_NOTICES_SHOWN 0x00E950D8u     /* @bin 0x004E2720 i32: the notices shown this frame */
#define G_NETWORK_INFO 0x00F53F68u      /* @bin 0x00536620 i32: the network info is shown */
#define G_STR_NETWORK_INFO 0x005F8F28u  /* @bin 0x00536620 "Network info (F2)" */

/* ---- the layout registry (present/draw/layout.c) and the menus' widths ---- */
#define G_LAYOUT_REGISTRY 0x00E94618u   /* @bin 0x004D5D90 0x78 entries of 16 bytes: three lists +0, +4, +8, +0xC empty */
#define G_LAYOUT_READY 0x00E94D9Cu      /* @bin 0x004D5D90 i32 := 1 once the registry is filled */
#define G_MENU_WIDTHS 0x00E945D0u       /* @bin 0x004D5DD0 i32[6]: the top menus' widths in 16-pixel pieces */
#define G_MENU_BAR_PIECES 0x00E94DB0u   /* @bin 0x004D5E70 i32: the menu bar's width in pieces */
#define G_MENU_BAR_WIDTH 0x00E94DACu    /* @bin 0x004D5E70 i32: the same in pixels */
#define G_MENU_BAR_RECTS 0x005C28ACu    /* @bin 0x004D5E70 rectangles of 0x1C bytes, +0 u16 x */

/* ---- the briefing, the build menus, the info panel ---- */
#define G_ARROWS_UP 0x005C7DF8u         /* @bin 0x00418FE0 the up arrow's button table (message text too) */
#define G_ARROWS_DOWN 0x005C7E20u       /* @bin 0x00418FE0 the down arrow's */
#define G_BUILD_MENUS 0x005C738Cu       /* @bin 0x00417A70 12 build menus of 0x24 bytes (+0 the menu); the byte before: "new" mark */
#define G_BUILD_MENU_COUNTS 0x006FDC58u /* @bin 0x00417A70 i32[12]: their number of buildings kept */
#define G_INFO_H 0x00EADD84u            /* @bin 0x00515BF0 i32: the info panel's height in 16-pixel pieces */

/* ---- the loading screen (present/boot/loading.c) ---- */
#define G_LOADING_IMAGES 0x00F55C3Cu    /* @bin 0x00520B20 i32: the first loading picture's image */
#define G_LOADING_LAST_IMAGE 0x00F5732Cu /* @bin 0x00520AF0 i32: the last loading picture's image */

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
/* --- end harvested --- */

#endif
