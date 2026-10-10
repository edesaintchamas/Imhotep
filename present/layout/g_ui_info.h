/* Imhotep - named globals: present/layout/g_ui_info.h.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md (owner: package P8b). */
#ifndef IMHOTEP_PRESENT_LAYOUT_G_UI_INFO_H
#define IMHOTEP_PRESENT_LAYOUT_G_UI_INFO_H

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
#define G_SCROLL_UP 0x005C5B48u /* present/ui/info/storage.c: the storage yard orders' scroll buttons */
#define G_SCROLL_DOWN 0x005C5B60u /* present/ui/info/storage.c */
#define G_ORDER_BUTTONS 0x005C81F8u /* present/ui/info/storage.c: button tables of the order rows, 0x48 bytes per row */
#define G_PERCENT_STRING 0x005E03FCu /* present/ui/frontend/options.c: the "%" suffix of the volumes */
#define G_THUMB_OFFSETS 0x005F76E8u /* present/ui/info/figure.c: i32[7] byte offsets of the thumbnails in the spare screen */
#define G_SELECTED_Y 0x0070E3E0u /* present/city/figures.c */
#define G_SELECTED_X 0x0070E3E4u /* present/city/figures.c: i32 the screen position of the figure drawn by number */
#define G_CV_MAP_START 0x0070E484u /* present/city/city.h */
#define G_BLIT_Y 0x00D3B5DCu /* present/ui/info/figure.c: i32 the copy's destination y */
#define G_BLIT_X 0x00D3B5E0u /* present/ui/info/figure.c: i32 the copy's destination x */
#define G_TEXT_Y 0x00E20774u /* present/ui/messages/richtext.c: i32 the height laid out (+16 per line) */
#define G_COURSE_SECOND 0x00EADB30u /* present/ui/messages/keywords.c: i32 the resource of the secondary course */
#define G_COURSE_MAIN 0x00EADB34u /* present/ui/messages/keywords.c: i32 the resource of the monument's main course */
#define G_PYRAMID_STATUS 0x00EADC48u /* present/ui/info/monument.c: the pyramid's status record (0x1C i32, by 0x004F1310 at the open) */
#define G_YARD_NOTICE 0x00EADCB8u /* present/ui/info/storage.c: i32 1 / 2: the storage yard's notice (texts 0x63/0xD, 0x63/0xE) */
#define G_SPEECH_KIND 0x00EADCE0u /* present/ui/info/phrase.c: i32 the speech table's entry for the figure's type, - 0xC9 */
#define G_FIGURE_COUNT 0x00EADD40u /* present/ui/info/info.h: i32 figures listed in INFO_FIGURES (0..7) */
#define G_FIGURE_VIEW 0x00EADD48u /* present/ui/info/info.h: i32 1: the figure list is shown (kind 1 with figures) */
#define G_TERRAIN_KIND 0x00EADD4Cu /* present/ui/info/info.h: i32 the terrain kind of kind 1 (no figures): its text */
#define G_HELP 0x00EADD54u /* present/ui/info/info.h: i32 the help message the help button opens (0x00511C50) */
#define G_ROAD 0x00EADD58u /* present/ui/info/info.h: i32 1: the building has road access */
#define G_WORKERS 0x00EADD5Cu /* present/ui/info/info.h: i32 the building's workers in percent of its need (0x004CDE60) */
#define G_TILE_BYTE 0x00EADD60u /* present/ui/info/info.h: i32 the tile's 0x00AE8E7C byte */
#define G_TILE_FLAG 0x00EADD64u /* present/ui/info/info.h: i32 the tile's 0x00AF598C bit 0x80 */
#define G_TERRAIN 0x00EADD68u /* present/ui/info/info.h: i32 the tile's terrain image byte (0x00A9010C grid) */
#define G_TILE_Y 0x00EADD78u /* present/ui/info/info.h: i32 */
#define G_TILE_X 0x00EADD7Cu /* present/ui/info/info.h: i32 */
#define G_TILE 0x00EADD80u /* present/ui/info/info.h: i32 grid offset of the clicked tile (the building's for a building) */
#define G_INFO_W 0x00EADD88u /* present/ui/info/info.h: i32 the panel's width in 16-pixel blocks */
#define G_INFO_SOUND 0x00EADD94u /* present/ui/info/info.h: i32 1 until the first draw (the building's sound plays once) [single+clash] */
#define G_SPEECH_PATH 0x00F55A20u /* present/ui/info/phrase.c: char[] the speech file's path (0x00539A70) */
/* --- end harvested --- */

#endif
