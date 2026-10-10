/* Imhotep - named globals: present/layout/g_ui_main.h.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md (owner: package P8a). */
#ifndef IMHOTEP_PRESENT_LAYOUT_G_UI_MAIN_H
#define IMHOTEP_PRESENT_LAYOUT_G_UI_MAIN_H

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
#define G_MENU_TABLE 0x0057BD88u /* present/ui/frontend/menu.c: 5 rectangles of 0x1C: i16 x0, y0, x1, y1, callback +8, param +0x14 [generic] */
#define G_DELETE_READ_TABLE 0x005C4810u /* present/ui/messages/list.c */
#define G_ROWS_TABLE 0x005C4830u /* present/ui/messages/list.c */
#define G_GOAL_RECTS 0x005C5798u /* present/ui/frames.c: the page's rectangle table (0x1C bytes per row) */
#define G_CLOSE_BUTTONS 0x005C7B10u /* present/ui/info/info.c: the help and close buttons (0x24 bytes each): 2 */
#define G_BTN_HELP 0x005C7E98u /* present/ui/messages/list.c */
#define G_BTN_CLOSE 0x005C80F0u /* present/ui/messages/list.c */
#define G_BTN_UP 0x005C8118u /* present/ui/messages/list.c */
#define G_BTN_DOWN 0x005C8140u /* present/ui/messages/list.c */
#define G_TAX_BUTTONS 0x005C8D38u /* present/ui/info/service.c: the image button table of the tax panels */
#define G_ROUTE_SELECTED 0x005D2A30u /* present/ui/empire/editor.c: i32 the selected route */
#define G_ARMY_SEL 0x005D2E88u /* present/ui/empire/empire.h: i32 the hovered / selected army marker (-1: none) */
#define G_FMT_D 0x005D9A18u /* present/ui/messages/keywords.c: "%d" */
#define G_FMT_PERCENT 0x005D9A1Cu /* present/ui/messages/keywords.c: "%d%%" */
#define G_FMT_D_S 0x005D9A24u /* present/ui/messages/keywords.c: "%d %s" */
#define G_GLYPH_DRAW_MAP 0x005E049Cu /* present/ui/messages/richtext.c: u8 glyph number per character - 0x20 */
#define G_ROUTE_DRAG 0x005E1DB0u /* present/ui/empire_editor.c: i32 the waypoint being dragged, -1 none */
#define G_TABS 0x005F8320u /* present/ui/advisors/advisors.h: the hall's 14 page buttons (0x24-byte entries, sidebar.h buttons_draw) */
#define G_SCREEN_Y 0x006FDC9Cu /* present/ui/empire/mission.c */
#define G_SCREEN_X 0x006FDCA0u /* present/ui/empire/mission.c: i32 the screens' origin */
#define G_CHEATS_ON 0x007027E0u /* present/ui/cityframe.c: u8 a cheat was typed once */
#define G_CHEAT_HOVER 0x007027E4u /* present/ui/cityframe.c: i32 the OK button hovered */
#define G_OVL_ROW 0x007056D0u /* present/ui/windows.c: i32 the open group's row in the menu */
#define G_OVL_ROWS 0x007056D8u /* present/ui/windows.c: i32 its rows */
#define G_OVL_GROUP 0x007056DCu /* present/ui/windows.c: i32 the open group (its id from 0x00576180), 0 none */
#define G_MENU_ITEM 0x00705720u /* present/ui/windows.c: u32 the item being drawn / chosen */
#define G_MENU_PREV 0x00705728u /* present/ui/windows.c: u32 the window under the menu */
#define G_MENU_HOVER 0x0070572Cu /* present/ui/windows.c: i32 the hovered item, 1-based */
#define G_MENU_TITLE 0x00705730u /* present/ui/windows.c: i32 the open menu's title, 1-based */
#define G_EDITOR_OBJECT 0x0077591Cu /* present/ui/empire/editor.c: i32 the selected object [generic] */
#define G_PICTURE_KIND 0x00775920u /* present/ui/empire/editor.c: i32 0..2 */
#define G_OBJECT_IMAGE 0x00775924u /* present/ui/empire/editor.c: i32 the object picture chosen */
#define G_EDITOR_MODE 0x00775928u /* present/ui/empire/editor.c: i32 the editor's mode [generic] */
#define G_MSG_NESTED 0x007854A4u /* present/ui/messages/keywords.c: u8 1 while the reason phrase (0x00447B10) expands: its own fields */
#define G_HIGHSCORE_FIRST 0x00D3A4B8u /* present/ui/frontend/highscore.c: i32 the first row shown [single+clash] */
#define G_MW_THUMB_Y 0x00D3B128u /* present/ui/messages/window.c: i32 the scroll thumb while dragged */
#define G_MESSAGE_WINDOW_SCROLL_MAX 0x00D3B134u /* present/ui/messages/window.c: i32 lines beyond the box (0: no scroll bar) */
#define G_MESSAGE_WINDOW_VISIBLE 0x00D3B13Cu /* present/ui/messages/richtext.c: i32 lines drawn */
#define G_MW_FOOT 0x00D3B144u /* present/ui/messages/window.c: i32 room below the box: 0x30, 0x20 for kinds != 0 */
#define G_MESSAGE_WINDOW_BOX_H 0x00D3B148u /* present/ui/messages/window.c: i32 the body box's height in pieces */
#define G_MESSAGE_WINDOW_BOX_W 0x00D3B14Cu /* present/ui/messages/richtext.c: i32 the text box's width in pieces */
#define G_MESSAGE_WINDOW_BOX_Y 0x00D3B150u /* present/ui/messages/window.c: i32 the body box's y */
#define G_MESSAGE_WINDOW_BOX_X 0x00D3B154u /* present/ui/messages/window.c: i32 the body box's x */
#define G_MW_SUB_W 0x00D3B158u /* present/ui/messages/window.c: i32 the subtitle's width */
#define G_MESSAGE_WINDOW_Y 0x00D3B15Cu /* present/ui/messages/window.c: i32 the window's y */
#define G_MESSAGE_WINDOW_X 0x00D3B160u /* present/ui/messages/window.c: i32 the window's x */
#define G_MESSAGE_WINDOW_FONT_LINK 0x00D3B164u /* present/ui/messages/richtext.c: i32 link font */
#define G_MESSAGE_WINDOW_FONT_TEXT 0x00D3B168u /* present/ui/messages/richtext.c: i32 text font */
#define G_KEYWORD_CURRENT 0x00D3B18Cu /* present/ui/messages/expand.c: u32 the keyword entry whose handler runs */
#define G_CLIP_Y 0x00D3B530u /* present/draw/mem.h: u8 vertical clip: 0 none, 3 top, 4 bottom, 5 out */
#define G_CLIP_X 0x00D3B531u /* present/draw/mem.h: u8 horizontal clip: 0 none, 1 left, 2 right, 5 out */
#define G_PIC_Y 0x00D3B568u /* present/ui/empire/mission.c */
#define G_PIC_X 0x00D3B56Cu /* present/ui/empire/mission.c: i32 the source offset of a full-screen picture (1024 x 768) */
#define G_PIC_H 0x00D3B570u /* present/ui/empire/mission.c */
#define G_PIC_W 0x00D3B574u /* present/ui/empire/mission.c */
#define G_UD3B5BC 0x00D3B5BCu /* present/ui/info/figure.c: i16 columns [unresolved] */
#define G_UD3B5BE 0x00D3B5BEu /* present/ui/info/figure.c: i16 rows [unresolved] */
#define G_WORD_CHARS 0x00E20760u /* present/ui/messages/richtext.c: i32 bytes the last measured word takes */
#define G_LINE_MAX_W 0x00E2076Cu /* present/ui/messages/richtext.c: i32 widest line */
#define G_MSGWIN_TABLE 0x00E209B8u /* present/ui/msgwin.c [generic] */
#define G_LINE_BUF 0x00E3439Cu /* present/ui/messages/richtext.c: char[200] the line being laid out */
#define G_HIT_Y 0x00E38E28u /* present/sidebar/input.c */
#define G_HIT_X 0x00E38E2Cu /* present/sidebar/input.c */
#define G_MSGLIST_IN_H 0x00E950F4u /* present/ui/messages/messages.h: i32 inner panel height in pieces */
#define G_MSGLIST_IN_W 0x00E950F8u /* present/ui/messages/messages.h: i32 inner panel width in pieces */
#define G_MSGLIST_IN_Y 0x00E950FCu /* present/ui/messages/messages.h: i32 inner panel y */
#define G_MSGLIST_IN_X 0x00E95100u /* present/ui/messages/messages.h: i32 inner panel x */
#define G_MSGLIST_H 0x00E95104u /* present/ui/messages/messages.h: i32 outer panel height in pieces */
#define G_MSGLIST_W 0x00E95108u /* present/ui/messages/messages.h: i32 outer panel width in 16-pixel pieces */
#define G_MSGLIST_Y 0x00E9510Cu /* present/ui/messages/messages.h: i32 outer panel y */
#define G_MSGLIST_X 0x00E95110u /* present/ui/messages/messages.h: i32 outer panel x */
#define G_MSGLIST_THUMB_Y 0x00E95114u /* present/ui/messages/messages.h: i32 the scroll thumb's offset while dragged */
#define G_MSGLIST_DRAGGING 0x00E95118u /* present/ui/messages/messages.h: i32 1 while the thumb is dragged */
#define G_INFO_Y 0x00EADD8Cu /* present/ui/info/info.h: i32 the panel's y */
#define G_INFO_X 0x00EADD90u /* present/ui/info/info.h: i32 the panel's x */
#define G_MARKERS 0x00F535D0u /* present/ui/empire/empire.h: 100 army markers of 5 i32: event, x, y, w, h */
#define G_PANEL_H 0x00F53F00u /* present/ui/advisors/advisors.h: i32 the page panel's height in 16-px pieces */
#define G_PANEL_W 0x00F53F04u /* present/ui/advisors/advisors.h: i32 the page panel's width in 16-px pieces */
#define G_ADVISOR_Y 0x00F53F08u /* present/ui/advisors/advisors.h: i32 */
#define G_ADVISOR_X 0x00F53F0Cu /* present/ui/ui.h: i32 the advisor windows' origin */
#define G_MARKER_COUNT 0x00F53FDCu /* present/ui/empire/empire.h */
#define G_PICTURES 0x00F55C04u /* present/ui/messages/richtext.c: i32 first image of the message pictures */
#define G_PICTURES_BIG 0x00F55C34u /* present/ui/messages/window.c: i32 big pictures (the video layout) */
#define G_ICONS 0x00F566ECu /* present/ui/messages/list.c: i32 first image of the message icons */
#define G_FONT_LINK 0x00F56FACu /* present/ui/messages/window.c */
#define G_PICTURES2 0x00F5731Cu /* present/ui/messages/window.c: i32 pictures from 1001 on */
#define G_IMG_ENTRIES 0x010D69F8u /* present/ui/empire/empire.c: image entries of 0x40 bytes: +0x14 width, +0x16 height (i16) */
/* --- end harvested --- */

#endif
