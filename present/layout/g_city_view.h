/* Imhotep - named globals: present/layout/g_city_view.h.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md (owner: package P9). */
#ifndef IMHOTEP_PRESENT_LAYOUT_G_CITY_VIEW_H
#define IMHOTEP_PRESENT_LAYOUT_G_CITY_VIEW_H

/* --- package P9: globals of the city view, the sidebar and the drawing, checked against
 * their uses. Evidence on each line: the @bin function that shows the meaning, or the port that
 * uses it. Image groups: G_IMAGE_GROUP(g), the first image of group g (the group records of the
 * graphics object, docs/formats/graphics.md "What a load writes"). --- */
#define G_IMAGE_GROUP(g) (G_IMAGE_GROUPS + 8u * (g)) /* @bin 0x0053DFF0 i32 the first image of image group g */
#define G_SQUARE_2X2_OFFSETS 0x00570084u /* @bin 0x004EF782 i32[4] grid offsets of a 2 x 2 square's tiles: 0, 1, 0xE4, 0xE5 */
#define G_FAMILY_ROW_A 0x005760C0u /* @bin 0x004C92A0 i32[17] the family tree's portraits, upper row */
#define G_FAMILY_ROW_B 0x00576104u /* @bin 0x004C92A0 i32[17] the family tree's portraits, lower row (0x1B: none) */
#define G_PART_SET 0x00596150u /* @bin 0x0041F8C0 0x378 dwords: the part descriptor set in use (G_PART_SET_FLAG) */
#define G_PART_SET_EVEN 0x00596F30u /* @bin 0x0041F8C0 the descriptor set of the even orientations (flag 0) */
#define G_PART_SET_ODD 0x00597D10u /* @bin 0x0041F8C0 the descriptor set of the odd orientations (flag 2) */
#define G_TRADE_KIND0_SLOTS 0x005C3EC0u /* @bin 0x004C95D0 the positions of the goods of a city of kind 0 */
#define G_TRADE_SOLD_SLOTS 0x005C4048u /* @bin 0x004C95D0 the trade window's positions of the goods sold: entries of 0x1C (+0 x, +2 y: i16) */
#define G_TRADE_BOUGHT_SLOTS 0x005C4128u /* @bin 0x004C95D0 the positions of the goods bought (as G_TRADE_SOLD_SLOTS) */
#define G_SIDEBAR_TOP_BUTTONS 0x005C7300u /* @bin 0x0051EC90 the expanded sidebar's first button table (2: slide, overlays menu) */
#define G_SIDEBAR_COLLAPSED_TOP_BUTTONS 0x005C7348u /* @bin 0x0051EC90 the collapsed sidebar's first button table (1: slide) */
#define G_SIDEBAR_BUTTONS 0x005C7370u /* @bin 0x0051EC90 the expanded sidebar's buttons (16; 12: undo, 13: messages) */
#define G_SIDEBAR_COLLAPSED_BUTTONS 0x005C75B0u /* @bin 0x0051EC90 the collapsed sidebar's buttons (12) */
#define G_EDITOR_SIDEBAR_BUTTONS 0x005C7760u /* @bin 0x0051E990 the editor sidebar's buttons (15) */
#define G_SIDEBAR_LOW_BUTTONS 0x005C7980u /* @bin 0x0051EC90 the expanded sidebar's last button table (2) */
#define G_MENUBAR_BUTTON 0x005C79C8u /* @bin 0x00425900 the button at the right of the menu bar: one button record (BUTTON_*) */
#define G_MENUBAR 0x005C91F0u /* @bin 0x00425330 the game's menu bar: 4 menus of 0x10 bytes (+0 x, +2 end x, +4 y, +6 text group) */
#define G_EDITOR_MENUBAR 0x005C9230u /* @bin 0x0051F0A0 the editor's menu bar (4 menus, as G_MENUBAR) */
#define G_BUTTON_REPEAT_STEPS 0x005C9268u /* @bin 0x00425C40 u8 per repeat count: != 0: the held button repeats */
#define G_FIG_CC_DX 0x005C95B8u /* @bin 0x00438F04 i8[256] by the progress between tiles (x % 15 + 16 (y % 15)): x offset, orientation 0 (mirrored: 4) */
#define G_FIG_CC_DY 0x005C96B8u /* @bin 0x00438F04 i8[256] y offset, orientation 0 (4) */
#define G_FIG_CC_DX2 0x005C97B8u /* @bin 0x00438F04 i8[256] x offset, orientation 2 (6) */
#define G_FIG_CC_DY2 0x005C98B8u /* @bin 0x00438F04 i8[256] y offset, orientation 2 (6) */
#define G_FIG_TILE_DX 0x005C99B8u /* @bin 0x004390CF i8 by direction * 16 + the progress on the tile: a standing figure's x offset */
#define G_FIG_TILE_DY 0x005C9A38u /* @bin 0x004390CF i8 the same, y */
#define G_FORT_YARD_OFFSETS 0x005CDF78u /* @bin 0x0043D720 i32 by orientation / 2: the yard's grid offset from the fort */
#define G_D2_PART2_OFFSETS 0x005CDFA8u /* @bin 0x00441490 i32 by orientation / 2: the grid offset of type 0xD2's second part */
#define G_D2_PART3_OFFSETS 0x005CDFB8u /* @bin 0x00441490 the third part's */
#define G_MINIMAP_COLOURS 0x005CE008u /* @bin 0x0043E770 i32 per building type (0xED): the minimap colour index + 1 */
#define G_FOOT_COUNT 0x005CFCD4u /* present/city/ghost.h: i32 per size: the tile count */
#define G_FOOT_OFFS 0x005CFCF0u /* present/city/ghost.h: (dx, dy) i32 pairs: the tiles of a footprint on the screen */
#define G_FOOTPRINT_POS2 0x005D0050u /* @bin 0x00432F2F (x, y) i32 pairs: the screen offsets of a size-2 footprint's tiles */
#define G_FOOTPRINT_POS3 0x005D0070u /* @bin 0x00432F2F size 3 */
#define G_FOOTPRINT_POS4 0x005D00B8u /* @bin 0x00432F2F size 4 */
#define G_FOOTPRINT_POS5 0x005D0138u /* @bin 0x00432F2F size 5 */
#define G_FOOTPRINT_POS6 0x005D0200u /* @bin 0x00432F2F size 6 */
#define G_COMPLEX_ORIGIN_SHIFT 0x005D1FF8u /* @bin 0x00432F0B (dx, dy) i32 pairs by orientation / 2: types 0x41..0x45, their tiles from +6 / +8 */
#define G_LISTED_TYPES 0x005D2540u /* @bin 0x00434A60 i32[] building types, zero-ended: overlay level 1 */
#define G_OVERLAY_TYPE_LISTS 0x005D25ACu /* @bin 0x00431690 u32 per overlay: a zero-ended i32 list of the building types it shows */
#define G_FOOTPRINT_IMG2 0x005D267Cu /* @bin 0x00432F2F i32 per tile: the image offsets of a size-2 footprint */
#define G_FOOTPRINT_IMG3 0x005D268Cu /* @bin 0x00432F2F size 3 */
#define G_FOOTPRINT_IMG4 0x005D26B0u /* @bin 0x00432F2F size 4 */
#define G_FOOTPRINT_IMG5 0x005D26F0u /* @bin 0x00432F2F size 5 */
#define G_FOOTPRINT_IMG6 0x005D2754u /* @bin 0x00432F2F size 6 */
#define G_FORT_YARD_PIXELS 0x005D27E4u /* @bin 0x0043D720 (x, y) i32 pairs by orientation / 2: the yard's position in G_U70E458 / G_U70E448 steps */
#define G_FORT_YARD_STEPS 0x005D2804u /* @bin 0x0043D720 (x, y) i32 pairs: in tile steps (G_TILE_W / G_TILE_H) */
#define G_FORT_YARD_SHIFT 0x005D2824u /* @bin 0x0043D720 i32 by orientation / 2: x shift */
#define G_D2_PART2_PIXELS 0x005D28B4u /* @bin 0x0044181D (x, y) i32 pairs: the second part's steps of G_U70E458 / G_U70E448 */
#define G_D2_PART3_PIXELS 0x005D28D4u /* @bin 0x0044181D the third part's */
#define G_D2_PART2_SHIFT 0x005D28F4u /* @bin 0x0044181D i32 by orientation / 2: x shift */
#define G_D2_PART3_SHIFT 0x005D2904u /* @bin 0x0044181D the third part's */
#define G_CITY_KIND_TEXTS 0x005D2A3Cu /* @bin 0x004CACA3 i32 per kind (8 bytes): the text naming a city's kind */
#define G_GATE_GROUPS 0x005DF768u /* @bin 0x004BFCD0 i32 per part (4): the gatehouse's image groups (0x1AE: the god's) */
#define G_COMPLEX_PART_TILES 0x005DF850u /* @bin 0x004C1F0A (x, y) i32 pairs: tile steps of the temple complex's second (+ k) and third (+ 4 + k) parts */
#define G_COMPLEX_PART_SHIFTS 0x005DF890u /* @bin 0x004C1F0A i32: their x shifts (as G_COMPLEX_PART_TILES) */
#define G_TIP_BUTTON_IDS 0x005E0274u /* @bin 0x004C8F44 i32 per button id (0x28..0x36): its tooltip text */
#define G_OVERLAY_TIPS 0x005E0350u /* @bin 0x004C9750 3 bytes per overlay: +0 1 occupied / 2 empty building, +1 needs a building, +2 a tile value */
#define G_STR_NUMBER_DOT 0x005E03F0u /* @bin 0x004CA894 "%d. " */
#define G_LINE_BREAK 0x005E03F8u /* present/sidebar/tooltip_text.c: "@L" */
#define G_STR_COMMA 0x005E0400u /* @bin 0x004CACA3 "," */
#define G_FAST_SCROLL 0x005E2A60u /* present/city/camera.c */
#define G_MONUMENT_FOOT_ORDER 0x005E44D0u /* @bin 0x004E5490 the footprint tile order of a monument ghost */
#define G_PYRAMID_LEVEL_HEIGHTS 0x005E68F8u /* @bin 0x004EF550 i32 per level (6 per kind): a pyramid layer's height (G_PYRAMID_KIND 1) */
#define G_PYRAMID_BLOCKS 0x005E6974u /* @bin 0x004EDBB0 16 bytes per block: a pyramid ghost's 2 x 2 blocks */
#define G_PYRAMID_BLOCKS5 0x005F1D74u /* @bin 0x004EDBB0 the same for G_PYRAMID_KIND 5 */
#define G_U5F5B68 0x005F5B68u /* @bin 0x004EF993 (dx, dy) i32 pairs by image - 0x6D: the shift of a part in mode 3 */
#define G_U5F5BF8 0x005F5BF8u /* @bin 0x004F08C0 (dx, dy) i32 pairs by corner code: added when the building's +0x68 is set */
#define G_U5F5D30 0x005F5D30u /* @bin 0x004EF7F6 u8 per part: != 0: the part's top image is drawn in mode 5 */
#define G_SIDEBAR_SLIDE_OUT 0x005F8260u /* @bin 0x0051FD60 i32[] the slide's x offsets, sliding out (by step) */
#define G_SIDEBAR_SLIDE_IN 0x005F831Cu /* @bin 0x0051FD60 i32[] the slide's x offsets, sliding in (- 4 * step) */
#define G_STR_LEVEL 0x005F8964u /* @bin 0x0051FBCC "Level" */
#define G_STR_EVENT 0x005F896Cu /* @bin 0x0051FBCC "Event" */
#define G_STR_MINOR_BAD 0x005F8974u /* @bin 0x0051FBCC "Minor Bad" */
#define G_STR_MAJOR_BAD 0x005F8980u /* @bin 0x0051FBCC "Major Bad" */
#define G_STR_MINOR_GOOD 0x005F898Cu /* @bin 0x0051FBCC "Minor Good" */
#define G_STR_MAJOR_GOOD 0x005F8998u /* @bin 0x0051FBCC "Major Good" */
#define G_STR_MAX_PROSPERITY 0x005F89A4u /* @bin 0x0051FAD4 " Max prosperity" */
#define G_STR_HAPPINESS 0x005F89B4u /* @bin 0x0051FAD4 " Happiness" */
#define G_STR_ALLOW_ALL 0x005F89C0u /* @bin 0x0051FAA3 "Allow All " */
#define G_U603238 0x00603238u /* @bin 0x00542E10 (x, y) i32 pairs: the positions of type 0xD5's images (group 568) */
#define G_MONUMENT_FOOT_ORDER_BIG 0x00603258u /* @bin 0x004E5490 the same, the big one */
#define G_U605B3D 0x00605B3Du /* @bin 0x00439740 u8 1: the top-figure counter G_TOP_COUNTER steps (an animation flag) */
#define G_PIXEL_LAST 0x00605B70u /* @bin 0x0040FAA0 u32 the pixel last read, ANDed with the mask */
#define G_AMBIENT_SLOTS 0x00702984u /* @bin 0x0041D6F0 the ambient sound slots, 0x80 bytes each (+0 halved, +8..+0x18 counters) */
#define G_AMBIENT_SLOTS_END 0x00704C04u /* @bin 0x0041D6F0 */
#define G_PART_SET_FLAG 0x00704DC0u /* present/city/parts.c: i32 0 / 2: which descriptor set 0x00596150 holds */
#define G_BUTTON_REPEAT_CLOCK 0x00705690u /* @bin 0x00425C40 u32 clock of the last repeat step (every 30 ms) */
#define G_MENUBAR_BUTTON_HOVER 0x00705698u /* @bin 0x00425840 i32 the menu bar button's third under the mouse + 1 (0: none) */
#define G_TOPBAR_HOVER 0x0070569Cu /* @bin 0x00406250 i32 the top bar figure under the mouse: 1 treasury, 2 population, 3 date */
#define G_MINIMAP_HOVER 0x007056A0u /* @bin 0x00406190 i32 1: the mouse is on the minimap */
#define G_MENUBAR_HOVER 0x007056A8u /* @bin 0x00426030 i32 the menu title under the mouse (1-based, 0: none) */
#define G_U7056CC 0x007056CCu /* @bin 0x00409D40 i32 := 0x0C when the overlays menu opens */
#define G_U7056D4 0x007056D4u /* @bin 0x00409D40 i32 := 0 when the overlays menu opens */
#define G_U7056E0 0x007056E0u /* @bin 0x00409D40 i32 := 0 when the overlays menu opens */
#define G_TINT 0x0070E120u /* present/sidebar/minimap.c: u16 colour tint of the tinted blitters */
#define G_BRIDGE_COUNT 0x0070E13Cu /* present/city/ghost.c: i32 the draws recorded */
#define G_BRIDGE_RECS 0x0070E140u /* present/city/ghost.c: 16 bytes each: image, x, y of the draw (0x00D3B580 / 7C / 78) */
#define G_CV_ANIM_CLOCK 0x0070E3C0u /* present/city/city.h: i32 frame clock of the last 60 ms animation step */
#define G_CV_ANIM_FLAG 0x0070E3C4u /* present/city/city.h: i32 1: the 60 ms animation step is due this frame */
#define G_MINIMAP_BOTTOM 0x0070E3C8u /* @bin 0x0043EF10 i32 bottom */
#define G_MINIMAP_RIGHT 0x0070E3CCu /* @bin 0x0043EF10 i32 right (left + 2 * width) */
#define G_MINIMAP_TOP 0x0070E3D0u /* @bin 0x0043EF10 i32 top */
#define G_MINIMAP_LEFT 0x0070E3D4u /* @bin 0x0043EF10 i32 the minimap's rectangle (set by its draw): left */
#define G_MINIMAP_FIRST_ROW 0x0070E3D8u /* present/sidebar/minimap.c: i32 */
#define G_MINIMAP_FIRST_COLUMN 0x0070E3DCu /* present/sidebar/minimap.c: i32 */
#define G_HOVER_MARK_Y 0x0070E3E8u /* present/city/ground.c */
#define G_HOVER_MARK_X 0x0070E3ECu /* present/city/ground.c */
#define G_VIEW_ROW 0x0070E47Cu /* @bin 0x00430EF0 i32 the row of the view walk (view y; rows of 0xE7 in G_VIEW_TABLE); the minimap walk too (0x0043E810) */
#define G_VIEW_COLUMN 0x0070E480u /* @bin 0x00430EF0 i32 the column of the view walk (view x) */
#define G_VIEW_TILE 0x0070E488u /* @bin 0x00430EF0 i32 the grid offset of the tile walked */
#define G_VIEW_COUNTDOWN 0x00775610u /* present/city/view.c */
#define G_DRAW_COUNTER 0x00775688u /* present/city/upper.c: u16, +1 per upper pass */
#define G_D2_PART2_STEPS 0x007756ACu /* @bin 0x0044181D (x, y) i32 pairs by orientation / 2: the second part's tile steps */
#define G_D2_PART3_STEPS 0x007756CCu /* @bin 0x0044181D the third part's */
#define G_TOP_COUNTER 0x0078D5ECu /* present/city/marks.c: i32 0..4, stepped when the animation flag G_U605B3D is up */
#define G_NO_FIGURES 0x00978D78u /* present/city/figures.c: i32 1: figures are not drawn */
#define G_UBC96D4 0x00BC96D4u /* @bin 0x0043E090 i32 added to the type 0x81 ghost image */
#define G_TIP_TEXT 0x00D3A4C0u /* present/sidebar/tooltip_text.c: char[] the tooltip buffer */
#define G_TIP_SAVED_LINES 0x00D3A6B4u /* @bin 0x004CA930 i32 G_TEXT_LINES saved */
#define G_TIP_SAVED_VISIBLE 0x00D3A6B8u /* @bin 0x004CA930 i32 G_MESSAGE_WINDOW_VISIBLE saved */
#define G_TIP_SAVED_SCROLL 0x00D3A6BCu /* @bin 0x004CA930 i32 G_SCROLL saved by the tooltip update, restored after the draw */
#define G_TIP_SINCE 0x00D3A6C8u /* present/sidebar/tooltip.c: i32 clock of the last move */
#define G_TIP_H 0x00D3A6CCu /* present/sidebar/tooltip_draw.c */
#define G_TIP_W 0x00D3A6D0u /* present/sidebar/tooltip_draw.c */
#define G_TIP_Y 0x00D3A6D4u /* present/sidebar/tooltip_draw.c */
#define G_TIP_X 0x00D3A6D8u /* present/sidebar/tooltip_draw.c */
#define G_UD3A6DC 0x00D3A6DCu /* @bin 0x004C9750 i32 := G_MOUSE_OFFSET when an overlay tooltip is shown */
#define G_UD3A6E0 0x00D3A6E0u /* @bin 0x004C9750 i32 := 0 when a tooltip is shown */
#define G_BUTTON_REPEAT 0x00D3B190u /* @bin 0x00425C40 i32 the editor button's repeat count given to its function (0 on the press) */
#define G_UD3B3AC 0x00D3B3ACu /* @bin 0x004262EC i32 != 0: a held kind-2 button calls its function */
#define G_UD3B3C8 0x00D3B3C8u /* @bin 0x00425990 i32 := 0 when a kind-2 button is released */
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
#define G_TEXT_CENTRE_DX 0x00E20770u /* @bin 0x004CC720 i32 the centred text's shift (0 when wider) */
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
#define G_EXTERNAL_NAME 0x00E38AA8u /* @bin 0x004CE370 char[] the external image's file name */
#define G_UE38DE0 0x00E38DE0u /* @bin 0x00425330 u8 := 1 by the menu bar draw and the menu close 0x00426090 */
#define G_UE38DE1 0x00E38DE1u /* @bin 0x00425330 u8 := 1 by the menu bar draw (2 by the menu close 0x00426090) */
#define G_LAYOUT_PANEL_OX 0x00E945F8u /* @bin 0x004D6090 i32 the highlighted element's original x */
#define G_LAYOUT_PANEL_OY 0x00E945FCu /* @bin 0x004D6090 i32 its original y */
#define G_LAYOUT_PANEL_X 0x00E94600u /* @bin 0x004D6090 i32 its x */
#define G_LAYOUT_PANEL_Y 0x00E94604u /* @bin 0x004D6090 i32 its y */
#define G_LAYOUT_PANEL_W 0x00E94608u /* @bin 0x004D6090 i32 its width */
#define G_LAYOUT_PANEL_H 0x00E9460Cu /* @bin 0x004D6090 i32 its height */
#define G_LAYOUT_LISTS 0x00E94618u /* @bin 0x004D5FE0 u32 per key (16 bytes): +0 the text overrides, +4 kind 2, +8 the panels */
#define G_SIDEBAR_LAST_MESSAGES 0x00F53DA0u /* @bin 0x0051F680 i32 the message count last drawn */
#define G_TOPBAR_LAST_MONTH 0x00F53DA4u /* @bin 0x0051F0A0 i32 the month last drawn */
#define G_TOPBAR_LAST_TREASURY 0x00F53DA8u /* @bin 0x0051F0A0 i32 the treasury last drawn */
#define G_TOPBAR_LAST_POPULATION 0x00F53DACu /* @bin 0x0051F0A0 i32 the population last drawn */
#define G_SIDEBAR_LAST_OVERLAY 0x00F53DB0u /* @bin 0x0051ED30 i32 the overlay whose name was last drawn */
#define G_TOPBAR_DATE_X 0x00F53DBCu /* @bin 0x0051F0A0 i32 the date's anchor (the menu bar button sits left of it) */
#define G_TOPBAR_POPULATION_X 0x00F53DC0u /* @bin 0x0051F0A0 i32 the population's anchor */
#define G_TOPBAR_TREASURY_X 0x00F53DC4u /* @bin 0x0051F0A0 i32 the treasury's anchor (hover 0x00406250: 128 pixels from it) */
#define G_DEBUG_TEXTS 0x00F53F70u /* @bin 0x0051FAD4 i32 != 0: the debug values " Happiness" and " Max prosperity" are drawn */
#define G_TOPBAR_REDRAW 0x00F53F74u /* @bin 0x0051F0A0 i32 > 0: the top bar's background is redrawn */
#define G_SIDEBAR_SLIDE_STEP 0x00F53F7Cu /* @bin 0x0051FD60 i32 the slide's step (0..47) */
#define G_EDITOR_SIDEBAR_IMAGES 0x00F55C64u /* @bin 0x0051E990 i32 first image of the editor's sidebar panel */
#define G_COLUMN_GROUP 0x00F56754u /* present/city/overlay_columns.c */
#define G_BUILD_MENU_PICTURES 0x00F567C4u /* @bin 0x0051EDD0 i32 first image of the build menus' pictures */
#define G_SIDEBAR_IMAGES 0x00F567E4u /* present/sidebar/sidebar.c: i32 first image of the sidebar panels */
#define G_PANEL_IMAGES 0x00F5683Cu /* @bin 0x00536820 i32 first image of the bordered panels' pieces */
#define G_FLAT_IMAGE 0x00F56BDCu /* present/city/overlay_tiles.c: group base of the flat building tiles (+4: building in use) */
#define G_IMG_FOOTPRINT 0x00F56BE4u /* present/city/ghost.h: i32 the plain tile footprint image */
#define G_WATER_IMAGE 0x00F56D14u /* present/city/overlay_tiles.c: group base of the 0x2D / 0x33 ground (0x00F56D1C: building in use) */

/* the button records (tables of 0x24-byte entries; @bin 0x00425730) */
#define BUTTON_X 0x00u /* @bin 0x00425730 i16 x (relative to the table's origin) */
#define BUTTON_Y 0x02u /* @bin 0x00425730 i16 y */
#define BUTTON_W 0x04u /* @bin 0x00425840 i16 width */
#define BUTTON_GROUP 0x0Au /* @bin 0x00425730 i16 image group */
#define BUTTON_IMAGE 0x0Cu /* @bin 0x00425730 i16 image offset in the group */
#define BUTTON_ENABLED 0x18u /* @bin 0x00425730 u8 0: drawn disabled (image + 3) */
#define BUTTON_PRESSED 0x19u /* @bin 0x00425730 u8 pressed (0x14: present now) */
#define BUTTON_HOVERED 0x1Au /* @bin 0x00425730 u8 hovered */
#define BUTTON_ANIM 0x1Bu /* @bin 0x00425730 u8 press animation: steps in the upper six bits, the phase in the low two */
#define BUTTON_PART 0x1Cu /* @bin 0x00425840 i32 the menu bar button's third clicked (0, 1, 2), added to its image */
#define BUTTON_STRIDE 0x24u /* @bin 0x00425730 bytes per button record */

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
/* --- end harvested --- */

#endif
