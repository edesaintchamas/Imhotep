/* Imhotep - named globals: present/layout/g_play.h.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md (owner: package P7). */
#ifndef IMHOTEP_PRESENT_LAYOUT_G_PLAY_H
#define IMHOTEP_PRESENT_LAYOUT_G_PLAY_H

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
#define G_LAYOUT_READY 0x00E94D9Cu /* present/ui/frontend/options.c */
#define G_MESSAGE_ROWS 0x00E950D8u /* present/ui/info/info.c: i32 the message list's rows (> 0: the click resets them) */
#define G_INFO_H 0x00EADD84u /* present/ui/info/info.h: i32 the panel's height in 16-pixel blocks */
#define G_BUTTON_STYLE 0x00F53F10u /* present/ui/frontend/menu.c: i32[] the large buttons' style by kind */
#define G_SCROLL_IMAGES 0x00F56494u /* present/ui/messages/list.c: i32 first image of the scroll bar group */
#define G_SIDE_IMAGES 0x00F5684Cu /* present/ui/info/info.c: i32 first image of the panels' icons (+0x0E the workers icon) */
/* --- end harvested --- */

#endif
