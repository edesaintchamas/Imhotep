/* Imhotep - named globals: present/layout/g_city_view.h.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md (owner: package P9). */
#ifndef IMHOTEP_PRESENT_LAYOUT_G_CITY_VIEW_H
#define IMHOTEP_PRESENT_LAYOUT_G_CITY_VIEW_H

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
#define G_FOOT_COUNT 0x005CFCD4u /* present/city/ghost.h: i32 per size: the tile count */
#define G_FOOT_OFFS 0x005CFCF0u /* present/city/ghost.h: (dx, dy) i32 pairs: the tiles of a footprint on the screen */
#define G_FERRY_MODEL 0x005D117Cu /* present/city/ghost_ferry.c: the model record of type 0x88 (0x005D06DC + 0x88 * 0x14) */
#define G_GATE_PARTS 0x005DF810u /* present/city/ghost_gate.c: 16 bytes per part (4): +0 u8 flag, +4 size, +8 a, +0xC b */
#define G_LINE_BREAK 0x005E03F8u /* present/sidebar/tooltip_text.c: "@L" */
#define G_FAST_SCROLL 0x005E2A60u /* present/city/camera.c */
#define G_PART_SET_FLAG 0x00704DC0u /* present/city/parts.c: i32 0 / 2: which descriptor set 0x00596150 holds */
#define G_TINT 0x0070E120u /* present/sidebar/minimap.c: u16 colour tint of the tinted blitters */
#define G_BRIDGE_COUNT 0x0070E13Cu /* present/city/ghost.c: i32 the draws recorded */
#define G_BRIDGE_RECS 0x0070E140u /* present/city/ghost.c: 16 bytes each: image, x, y of the draw (0x00D3B580 / 7C / 78) */
#define G_CV_ANIM_CLOCK 0x0070E3C0u /* present/city/city.h: i32 frame clock of the last 60 ms animation step */
#define G_CV_ANIM_FLAG 0x0070E3C4u /* present/city/city.h: i32 1: the 60 ms animation step is due this frame */
#define G_MINIMAP_FIRST_ROW 0x0070E3D8u /* present/sidebar/minimap.c: i32 */
#define G_MINIMAP_FIRST_COLUMN 0x0070E3DCu /* present/sidebar/minimap.c: i32 */
#define G_HOVER_MARK_Y 0x0070E3E8u /* present/city/ground.c */
#define G_HOVER_MARK_X 0x0070E3ECu /* present/city/ground.c */
#define G_U70E47C 0x0070E47Cu /* present/sidebar/minimap.c: i32 current row [unresolved] */
#define G_U70E480 0x0070E480u /* present/sidebar/minimap.c: i32 current column of the walk [unresolved] */
#define G_U70E488 0x0070E488u /* present/sidebar/minimap.c: i32 current grid offset [unresolved] */
#define G_VIEW_COUNTDOWN 0x00775610u /* present/city/view.c */
#define G_DRAW_COUNTER 0x00775688u /* present/city/upper.c: u16, +1 per upper pass */
#define G_TOP_COUNTER 0x0078D5ECu /* present/city/marks.c: i32 0..4, stepped when the animation flag 0x00605B3D is up */
#define G_NO_FIGURES 0x00978D78u /* present/city/figures.c: i32 1: figures are not drawn */
#define G_TIP_TEXT 0x00D3A4C0u /* present/sidebar/tooltip_text.c: char[] the tooltip buffer */
#define G_TIP_SINCE 0x00D3A6C8u /* present/sidebar/tooltip.c: i32 clock of the last move */
#define G_TIP_H 0x00D3A6CCu /* present/sidebar/tooltip_draw.c */
#define G_TIP_W 0x00D3A6D0u /* present/sidebar/tooltip_draw.c */
#define G_TIP_Y 0x00D3A6D4u /* present/sidebar/tooltip_draw.c */
#define G_TIP_X 0x00D3A6D8u /* present/sidebar/tooltip_draw.c */
#define G_LINE_ERR 0x00D3B4BCu /* present/draw/fill.c: i32 the error term */
#define G_LINE_END_X 0x00D3B4C0u /* present/draw/fill.c: i32 */
#define G_LINE_END_Y 0x00D3B4C4u /* present/draw/fill.c: i32 */
#define G_LINE_X 0x00D3B4C8u /* present/draw/fill.c: i32 the start */
#define G_LINE_Y 0x00D3B4CCu /* present/draw/fill.c: i32 */
#define G_LINE_DX 0x00D3B4D0u /* present/draw/fill.c: i32 |x1 - x2|, counted down */
#define G_LINE_DY 0x00D3B4D4u /* present/draw/fill.c: i32 |y1 - y2|, counted down */
#define G_LINE_STEP_X 0x00D3B4D8u /* present/draw/fill.c: i32 1 or -1 */
#define G_LINE_STEP_Y 0x00D3B4DCu /* present/draw/fill.c: i32 1 or -1 */
#define G_MARCH_COUNT 0x00D3B4E8u /* present/draw/fill.c: i32 dots drawn in that colour */
#define G_MARCH_COLOUR 0x00D3B4F0u /* present/draw/fill.c: i32 the colour of the next dot of a marching rectangle */
#define G_MARCH_START 0x00D3B4F4u /* present/draw/fill.c: i32 the first dot's colour (cycled by 0x004CD810) */
#define G_FONT_HEIGHT 0x00D3B500u /* present/ui/info/monument.c: i32 the font's line height */
#define G_TEXT_FLAG_504 0x00D3B504u /* present/draw/text.c */
#define G_TEXT_SHADOW 0x00D3B514u /* present/draw/text.c: i32 != 0: a black copy one pixel right and down */
#define G_GLYPH_COLOUR 0x00D3B51Cu /* present/sidebar/tooltip_draw.c: u16 the text colour of 0x004CC770 */
#define G_CLIP_BOTTOM_H 0x00D3B534u /* present/draw/mem.h: i32 the height (the visible rows from the top when cut at the bottom) */
#define G_CLIP_TOP_ROWS 0x00D3B538u /* present/draw/mem.h: i32 rows cut at the top */
#define G_DST_SKIP 0x00D3B53Cu /* present/draw/mem.h: i32 destination bytes from a row's end to the next row's start */
#define G_SRC_SKIP 0x00D3B540u /* present/draw/mem.h: i32 source bytes to skip per row (clipped columns) */
#define G_VIS_W 0x00D3B544u /* present/draw/mem.h: i32 visible width after the horizontal clip */
#define G_CLIP_RIGHT_W 0x00D3B548u /* present/draw/mem.h: i32 the width (the visible width when cut on the right) */
#define G_CLIP_LEFT_COLS 0x00D3B54Cu /* present/draw/mem.h: i32 columns cut on the left */
#define G_LAST_Y 0x00D3B578u /* present/city/ghost.c */
#define G_LAST_X 0x00D3B57Cu /* present/city/ghost.c */
#define G_LAST_IMAGE 0x00D3B580u /* present/city/ghost.c: i32 the last draw's image, x, y (0x0053E590) */
#define G_IMG_TYPE 0x00D3B5B4u /* present/draw/mem.h: u8 the image's type (+50) */
#define G_MEM_SRC 0x00D3B5B8u /* present/draw/mem.h: u32 the image's pixel data (later the clipped start) [generic] */
#define G_VIS_H 0x00D3B5D4u /* present/draw/mem.h: i32 visible height after the vertical clip */
#define G_RUN_EDIT 0x00E2077Cu /* present/draw/text.c: i32 != 0: characters come from the edited string, from this index */
#define G_RUN_HASH 0x00E20780u /* present/draw/text.c: i32 1: '#' starts the edited string */
#define G_RUN_CHARS 0x00E20784u /* present/draw/text.c: i32 characters drawn */
#define G_CARET_SET 0x00E20788u /* present/draw/text.c: i32 1: the caret was placed */
#define G_CARET_W 0x00E2078Cu /* present/draw/text.c */
#define G_RUN_SPACES 0x00E20798u /* present/draw/text.c: u8 != 0: collapse repeated spaces */
#define G_RUN_NO_COLLAPSE 0x00E20799u /* present/ui/info/monument.c */
#define G_INSERT 0x00E2079Bu /* present/draw/textedit.c: u8 1: insert, 0: overwrite */
#define G_NUMBER_TEXT 0x00E34238u /* present/sidebar/tooltip_text.c: char[] the number written by 0x004CC5C0 */
#define G_EDIT_TEXT 0x00E34338u /* present/draw/text.c: the edited string */
#define G_MARCH_START_COUNT 0x00E36C84u /* present/draw/fill.c */
#define G_COLUMN_GROUP 0x00F56754u /* present/city/overlay_columns.c */
#define G_SIDEBAR_IMAGES 0x00F567E4u /* present/sidebar/sidebar.c: i32 first image of the sidebar panels */
#define G_FLAT_IMAGE 0x00F56BDCu /* present/city/overlay_tiles.c: group base of the flat building tiles (+4: building in use) */
#define G_IMG_FOOTPRINT 0x00F56BE4u /* present/city/ghost.h: i32 the plain tile footprint image */
#define G_WATER_IMAGE 0x00F56D14u /* present/city/overlay_tiles.c: group base of the 0x2D / 0x33 ground (0x00F56D1C: building in use) */
/* --- end harvested --- */

#endif
